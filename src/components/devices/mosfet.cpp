/*
 * mosfet.cpp - mosfet class implementation
 *
 * Copyright (C) 2004 Stefan Jahn <stefan@lkcc.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 *
 * $Id: mosfet.cpp,v 1.1 2004-08-03 15:36:20 ela Exp $
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "complex.h"
#include "matrix.h"
#include "object.h"
#include "logging.h"
#include "node.h"
#include "circuit.h"
#include "net.h"
#include "analysis.h"
#include "dcsolver.h"
#include "component_id.h"
#include "constants.h"
#include "device.h"
#include "mosfet.h"

#define NODE_G 1 /* gate node   */
#define NODE_D 2 /* drain node  */
#define NODE_S 3 /* source node */
#define NODE_B 4 /* bulk node   */

mosfet::mosfet () : circuit (4) {
  rg = rs = rd = NULL;
  type = CIR_MOSFET;
}

void mosfet::calcSP (nr_double_t frequency) {

  // fetch computed operating points
  nr_double_t Cgd = getOperatingPoint ("Cgd");
  nr_double_t Cgs = getOperatingPoint ("Cgs");
  nr_double_t Cbd = getOperatingPoint ("Cbd");
  nr_double_t Cbs = getOperatingPoint ("Cbs");
  nr_double_t Cgb = getOperatingPoint ("Cgb");
  nr_double_t gbs = getOperatingPoint ("gbs");
  nr_double_t gbd = getOperatingPoint ("gbd");
  nr_double_t gds = getOperatingPoint ("gds");
  nr_double_t gm  = getOperatingPoint ("gm");
  nr_double_t gmb = getOperatingPoint ("gmb");

  // compute the models admittances
  complex Ygd = rect (0.0, 2.0 * M_PI * frequency * Cgd);
  complex Ygs = rect (0.0, 2.0 * M_PI * frequency * Cgs);
  complex Yds = gds;
  complex Ybd = rect (gbd, 2.0 * M_PI * frequency * Cbd);
  complex Ybs = rect (gbs, 2.0 * M_PI * frequency * Cbs);
  complex Ygb = rect (0.0, 2.0 * M_PI * frequency * Cgb);

  // build admittance matrix and convert it to S-parameter matrix
  matrix y = matrix (4);
  y.set (NODE_G, NODE_G, Ygd + Ygs + Ygb);
  y.set (NODE_G, NODE_D, -Ygd);
  y.set (NODE_G, NODE_S, -Ygs);
  y.set (NODE_G, NODE_B, -Ygb);
  y.set (NODE_D, NODE_G, gm - Ygd);
  y.set (NODE_D, NODE_D, Ygd + Yds + Ybd - DrainControl);
  y.set (NODE_D, NODE_S, -Yds - SourceControl);
  y.set (NODE_D, NODE_B, -Ybd + gmb);
  y.set (NODE_S, NODE_G, -Ygs - gm);
  y.set (NODE_S, NODE_D, -Yds + DrainControl);
  y.set (NODE_S, NODE_S, Ygs + Yds + Ybs + SourceControl);
  y.set (NODE_S, NODE_B, -Ybs - gmb);
  y.set (NODE_B, NODE_G, -Ygb);
  y.set (NODE_B, NODE_D, -Yds);
  y.set (NODE_B, NODE_S, -Ybs);
  y.set (NODE_B, NODE_B, Ybd + Ybs + Ygb);

  setMatrixS (ytos (y));
}

void mosfet::calcNoise (nr_double_t frequency) {
  nr_double_t Kf  = getPropertyDouble ("Kf");
  nr_double_t Af  = getPropertyDouble ("Af");
  nr_double_t Ffe = getPropertyDouble ("Ffe");
  nr_double_t gm  = getOperatingPoint ("gm");
  nr_double_t Ids = getOperatingPoint ("Id");
  nr_double_t T   = getPropertyDouble ("Temp");

  /* compute channel noise and flicker noise generated by the DC
     transconductance and current flow from drain to source */
  nr_double_t i = 8 * kelvin (T) / T0 * gm / 3 +
    Kf * pow (Ids, Af) / pow (frequency, Ffe) / kB / T0;

  /* build noise current correlation matrix and convert it to
     noise-wave correlation matrix */
  matrix y = matrix (4);
  y.set (NODE_D, NODE_D, +i);
  y.set (NODE_S, NODE_S, +i);
  y.set (NODE_D, NODE_S, -i);
  y.set (NODE_S, NODE_D, -i);
  setMatrixN (cytocs (y * z0, getMatrixS ()));
}

void mosfet::initDC (dcsolver * solver) {

  // initialize starting values
  setV (NODE_G, 0.8);
  setV (NODE_D, 0.0);
  setV (NODE_S, 0.0);
  setV (NODE_B, 0.0);
  UgdPrev = real (getV (NODE_G) - getV (NODE_D));
  UgsPrev = real (getV (NODE_G) - getV (NODE_S));
  UbsPrev = real (getV (NODE_B) - getV (NODE_S));
  UbdPrev = real (getV (NODE_B) - getV (NODE_D));
  UdsPrev = UgsPrev - UgdPrev;

  // initialize the MOSFET
  initModel ();

  // get device temperature
  nr_double_t T = getPropertyDouble ("Temp");

  // possibly insert series resistance at source
  nr_double_t Rs = getPropertyDouble ("Rs");
  if (Rs != 0) {
    // create additional circuit if necessary and reassign nodes
    rs = splitResistance (this, rs, solver->getNet (), "Rs", "source", NODE_S);
    rs->setProperty ("Temp", T);
    rs->setProperty ("R", Rs);
  }
  // no series resistance at source
  else {
    disableResistance (this, rs, solver->getNet (), NODE_S);
  }

  // possibly insert series resistance at gate
  nr_double_t Rg = getPropertyDouble ("Rg");
  if (Rg != 0) {
    // create additional circuit if necessary and reassign nodes
    rg = splitResistance (this, rg, solver->getNet (), "Rg", "gate", NODE_G);
    rg->setProperty ("Temp", T);
    rg->setProperty ("R", Rg);
  }
  // no series resistance at source
  else {
    disableResistance (this, rg, solver->getNet (), NODE_G);
  }

  // possibly insert series resistance at drain
  nr_double_t Rd = getPropertyDouble ("Rd");
  if (Rd != 0) {
    // create additional circuit if necessary and reassign nodes
    rd = splitResistance (this, rd, solver->getNet (), "Rd", "drain", NODE_D);
    rd->setProperty ("Temp", T);
    rd->setProperty ("R", Rd);
  }
  // no series resistance at drain
  else {
    disableResistance (this, rd, solver->getNet (), NODE_D);
  }
}

void mosfet::initModel (void) {

  // apply polarity of MOSFET
  char * type = getPropertyString ("Type");
  MOSpol = !strcmp (type, "pfet") ? -1 : 1;

  // calculate effective channel length
  nr_double_t L  = getPropertyDouble ("L");
  nr_double_t Ld = getPropertyDouble ("Ld");
  if ((Leff = L - 2 * Ld) <= 0) {
    logprint (LOG_STATUS, "WARNING: effective MOSFET channel length %g <= 0, "
	      "set to L = %g\n", Leff, L);
    Leff = L;
  }

  // calculate gate oxide overlap capacitance
  nr_double_t W   = getPropertyDouble ("W");
  nr_double_t Tox = getPropertyDouble ("Tox");
  if (Tox <= 0) {
    logprint (LOG_STATUS, "WARNING: disabling gate oxide capacitance, "
	      "Cox = 0\n");
    Cox = 0;
  } else {
    Cox = (ESiO2 * E0 / Tox);
  }

  // calculate DC transconductance coefficient
  nr_double_t Kp = getPropertyDouble ("Kp");
  nr_double_t Uo = getPropertyDouble ("Uo");
  if (Kp > 0) {
    beta = Kp * W / Leff;
  } else {
    if (Cox > 0 && Uo > 0) {
      beta = Uo * 1e-4 * Cox * W / Leff;
    } else {
      logprint (LOG_STATUS, "WARNING: adjust Tox, Uo or Kp to get a valid "
		"transconductance coefficient\n");
      beta = 2e-5;
    }
  }

  // calculate surface potential
  nr_double_t P    = getPropertyDouble ("Phi");
  nr_double_t Nsub = getPropertyDouble ("Nsub");
  nr_double_t Ut   = T0 * kBoverQ;
  if ((Phi = P) <= 0) {
    if (Nsub > 0) {
      if (Nsub * 1e6 >= Ni) {
	Phi = 2 * Ut * log (Nsub * 1e6 / Ni);
      } else {
	logprint (LOG_STATUS, "WARNING: substrate doping less than instrinsic "
		  "density, adjust Nsub >= %g\n", Ni / 1e6);
	Phi = 0.6;
      }
    } else {
      logprint (LOG_STATUS, "WARNING: adjust Nsub or Phi to get a valid "
		"surface potential\n");
      Phi = 0.6;
    }
  }

  // calculate bulk threshold
  nr_double_t G = getPropertyDouble ("Gamma");
  if ((Ga = G) < 0) {
    if (Cox > 0 && Nsub > 0) {
      Ga = sqrt (2 * Q * ESi * E0 * Nsub * 1e6) / Cox;
    } else {
      logprint (LOG_STATUS, "WARNING: adjust Tox, Nsub or Gamma to get a "
		"valid bulk threshold\n");
      Ga = 0.0;
    }
  }

  // calculate threshold voltage
  nr_double_t Vt0 = getPropertyDouble ("Vt0");
  if ((Vto = Vt0) <= 0) {
    nr_double_t Tpg = getPropertyDouble ("Tpg");
    nr_double_t Nss = getPropertyDouble ("Nss");
    nr_double_t PhiMS, PhiG, EgFET, FerGate, FerSilicon;
    EgFET = 1.16 - (7.02e-4 * T0 * T0) / (T0 + 1108);
    FerSilicon = MOSpol * Phi / 2;
    PhiG = 3.2;
    if (Tpg != 0) {
      FerGate = MOSpol * Tpg * EgFET / 2;
      PhiG = 3.25 + EgFET / 2 - FerGate;
    }
    PhiMS = PhiG - (3.25 + EgFET / 2 + FerSilicon);
    if (Nss >= 0 && Cox > 0) {
      Vto = PhiMS - Q * Nss * 1e4 / Cox + MOSpol * (Phi + Ga * sqrt (Phi));
    } else {
      logprint (LOG_STATUS, "WARNING: adjust Tox, Nss or Vt0 to get a "
		"valid threshold voltage\n");
      Vto = 0.0;
    }
  }

  Cox = Cox * W * Leff;

#if DEBUG
  logprint (LOG_STATUS, "NOTIFY: Cox=%g, Beta=%g Ga=%g, Phi=%g, Vto=%g\n",
	    Cox, beta, Ga, Phi, Vto);
#endif /* DEBUG */
}

void mosfet::calcDC (void) {

  // fetch device model parameters
  nr_double_t Is   = getPropertyDouble ("Is");
  nr_double_t n    = getPropertyDouble ("N");
  nr_double_t l    = getPropertyDouble ("Lambda");
  nr_double_t T    = getPropertyDouble ("Temp");

  nr_double_t Ugs, Ugd, Ut, IeqBS, IeqBD, IeqDS, UbsCrit, UbdCrit;
  nr_double_t Uds, Ibs, Ibd, gtiny, Ubs, Ubd;

  T = kelvin (T);
  Ut = T * kBoverQ;
  Ugd = real (getV (NODE_G) - getV (NODE_D)) * MOSpol;
  Ugs = real (getV (NODE_G) - getV (NODE_S)) * MOSpol;
  Ubs = real (getV (NODE_B) - getV (NODE_S)) * MOSpol;
  Ubd = real (getV (NODE_B) - getV (NODE_D)) * MOSpol;
  Uds = Ugs - Ugd;

  // critical voltage necessary for bad start values
  UbsCrit = pnCriticalVoltage (Is, Ut * n);
  UbdCrit = pnCriticalVoltage (Is, Ut * n);

  // for better convergence
  if (Uds >= 0) {
    Ugs = fetVoltage (Ugs, UgsPrev, Vto * MOSpol);
    Uds = Ugs - Ugd;
    Uds = fetVoltage (Uds, UdsPrev, Vto * MOSpol);
    Ugd = Ugs - Uds;
  }
  else {
    Ugd = fetVoltage (Ugd, UgdPrev, Vto * MOSpol);
    Uds = Ugs - Ugd;
    Uds = -fetVoltage (-Uds, -UdsPrev, Vto * MOSpol);
    Ugs = Ugd + Uds;
  }
  if (Uds >= 0) {
    Ubs = pnVoltage (Ubs, UbsPrev, Ut * n, UbsCrit);
    Ubd = Ubs - Uds;
  }
  else {
    Ubd = pnVoltage (Ubd, UbdPrev, Ut * n, UbdCrit);
    Ubs = Ubd + Uds;
  }
  UgsPrev = Ugs; UgdPrev = Ugd; UbsPrev = Ubs; UbdPrev = Ubd; UdsPrev = Uds; 
  
  // parasitic bulk-source diode
  gtiny = Ubs < - 10 * Ut * n ? Is : 0;
  gbs = pnConductance (Ubs, Is, Ut * n) + gtiny;
  Ibs = pnCurrent (Ubs, Is, Ut * n) + gtiny * Ubs;

  // parasitic bulk-drain diode
  gtiny = Ubd < - 10 * Ut * n ? Is : 0;
  gbd = pnConductance (Ubd, Is, Ut * n) + gtiny;
  Ibd = pnCurrent (Ubd, Is, Ut * n) + gtiny * Ubd;

  // calculate drain current
  MOSdir = (Uds >= 0) ? +1 : -1;
  nr_double_t Sarg, Upn;
  Upn = (MOSdir > 0) ? Ubs : Ubd;
  if (Upn <= 0) {
    Sarg = sqrt (Phi - Upn);
  }
  else {
    Sarg = sqrt (Phi);
    Sarg = Sarg - Upn / Sarg / 2; // looks like taylor series of "sqrt (x - 1)"
    Sarg = MAX (Sarg, 0);
#if 0
    fprintf (stderr, "SPICE: %g, ME: %g\n",
	     Sarg, sqrt (Phi - Upn) - sqrt (Phi));
#endif
  }
  Uon = Vto * MOSpol + Ga * Sarg;
  nr_double_t Utst = ((MOSdir > 0) ? Ugs : Ugd) - Uon;
  nr_double_t arg = (Sarg <= 0) ? 0 : (Ga / Sarg / 2);

  // cutoff region
  if (Utst <= 0) {
    Ids = 0;
    gm  = 0;
    gds = 0;
    gmb = 0;
  }
  else {
    nr_double_t Vds  = Uds * MOSdir;
    nr_double_t b    = beta * (1 + l * Vds);
    // saturation region
    if (Utst <= Vds) {
      Ids = b * Utst * Utst / 2;
      gm  = b * Utst;
      gds = l * beta * Utst * Utst / 2;
    }
    // linear region
    else {
      Ids = b * Vds * (Utst - Vds / 2);
      gm  = b * Vds;
      gds = b * (Utst - Vds) + l * beta * Vds * (Utst - Vds /2);
    }
    gmb = gm * arg;
  }
  Udsat = MOSpol * MAX (Utst, 0);
  Ids = MOSdir * Ids;
  Uon = MOSpol * Uon;

  // compute autonomic current sources
  IeqBD = Ibd - gbd * Ubd;
  IeqBS = Ibs - gbs * Ubs;

  // exchange controlling nodes if necessary
  SourceControl = (MOSdir > 0) ? (gm + gmb) : 0;
  DrainControl  = (MOSdir < 0) ? (gm + gmb) : 0;
  if (MOSdir > 0) {
    IeqDS = Ids - gm * Ugs - gmb * Ubs - gds * Uds;
  } else {
    IeqDS = Ids - gm * Ugd - gmb * Ubd - gds * Uds;
  }

  setI (NODE_G, 0);
  setI (NODE_D, (+IeqBD - IeqDS) * MOSpol);
  setI (NODE_S, (+IeqBS + IeqDS) * MOSpol);
  setI (NODE_B, (-IeqBD - IeqBS) * MOSpol);

  // apply admittance matrix elements
  setY (NODE_G, NODE_G, 0);
  setY (NODE_G, NODE_D, 0);
  setY (NODE_G, NODE_S, 0);
  setY (NODE_G, NODE_B, 0);
  setY (NODE_D, NODE_G, gm);
  setY (NODE_D, NODE_D, gds + gbd - DrainControl);
  setY (NODE_D, NODE_S, -gds - SourceControl);
  setY (NODE_D, NODE_B, gmb - gbd);
  setY (NODE_S, NODE_G, -gm);
  setY (NODE_S, NODE_D, -gds + DrainControl);
  setY (NODE_S, NODE_S, gbs + gds + SourceControl);
  setY (NODE_S, NODE_B, -gbs - gmb);
  setY (NODE_B, NODE_G, 0);
  setY (NODE_B, NODE_D, -gbd);
  setY (NODE_B, NODE_S, -gbs);
  setY (NODE_B, NODE_B, gbs + gbd);
}

void mosfet::calcOperatingPoints (void) {

  // fetch device model parameters
  nr_double_t Cbd0 = getPropertyDouble ("Cbd");
  nr_double_t Cbs0 = getPropertyDouble ("Cbs");
  nr_double_t Cgso = getPropertyDouble ("Cgso");
  nr_double_t Cgdo = getPropertyDouble ("Cgdo");
  nr_double_t Cgbo = getPropertyDouble ("Cgbo");
  nr_double_t Pb   = getPropertyDouble ("Pb");
  nr_double_t M    = getPropertyDouble ("Mj");
  nr_double_t Fc   = getPropertyDouble ("Fc");
  nr_double_t Tt   = getPropertyDouble ("Tt");
  nr_double_t W    = getPropertyDouble ("W");
  
  nr_double_t Ubs, Ubd, Cbs, Cbd, Ugs, Ugd, Uds, Ugb;
  nr_double_t Cgd, Cgb, Cgs;

  Ugd = real (getV (NODE_G) - getV (NODE_D)) * MOSpol;
  Ugs = real (getV (NODE_G) - getV (NODE_S)) * MOSpol;
  Ubs = real (getV (NODE_B) - getV (NODE_S)) * MOSpol;
  Ubd = real (getV (NODE_B) - getV (NODE_D)) * MOSpol;
  Uds = Ugs - Ugd;
  Ugb = Ugs - Ubs;

  // capacitance of bulk-drain diode
  Cbd = gbd * Tt + pnCapacitance (Ubd, Cbd0, Pb, M, Fc);

  // capacitance of bulk-source diode
  Cbs = gbs * Tt + pnCapacitance (Ubs, Cbs0, Pb, M, Fc);

  // calculate bias-dependent MOS overlap capacitances
  if (MOSdir > 0) {
    fetCapacitanceMeyer (Ugs, Ugd, Ugb, Uon, Udsat, Phi, Cox, Cgs, Cgd, Cgb);
  } else {
    fetCapacitanceMeyer (Ugd, Ugs, Ugb, Uon, Udsat, Phi, Cox, Cgd, Cgs, Cgb);
  }
  Cgs += Cgso * W;
  Cgd += Cgdo * W;
  Cgb += Cgbo * Leff;

  // save operating points
  setOperatingPoint ("gbs", gbs);
  setOperatingPoint ("gbd", gbd);
  setOperatingPoint ("gds", gds);
  setOperatingPoint ("gm", gm);
  setOperatingPoint ("gmb", gmb);
  setOperatingPoint ("Id", Ids);
  setOperatingPoint ("Vgs", Ugs);
  setOperatingPoint ("Vgd", Ugd);
  setOperatingPoint ("Vbs", Ubs);
  setOperatingPoint ("Vbd", Ubd);
  setOperatingPoint ("Vds", Ugs - Ugd);
  setOperatingPoint ("Cbd", Cbd);
  setOperatingPoint ("Cbs", Cbs);
  setOperatingPoint ("Cgs", Cgs);
  setOperatingPoint ("Cgd", Cgd);
  setOperatingPoint ("Cgb", Cgb);
}
