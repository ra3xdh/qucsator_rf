/*
 * vdc.cpp - DC voltage source class implementation
 *
 * Copyright (C) 2003, 2004, 2006 Stefan Jahn <stefan@lkcc.org>
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
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.  
 *
 * $Id: vdc.cpp,v 1.16 2006-04-05 08:27:06 raimi Exp $
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "complex.h"
#include "object.h"
#include "node.h"
#include "circuit.h"
#include "net.h"
#include "component_id.h"
#include "vdc.h"

vdc::vdc () : circuit (2) {
  type = CIR_VDC;
  setVSource (true);
  setVoltageSources (1);
}

void vdc::initSP (void) {
  allocMatrixS ();
  setS (NODE_1, NODE_1, 0.0);
  setS (NODE_1, NODE_2, 1.0);
  setS (NODE_2, NODE_1, 1.0);
  setS (NODE_2, NODE_2, 0.0);
}

void vdc::initDC (void) {
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2, getPropertyDouble ("U"));
}

void vdc::calcDC (void) {
  nr_double_t f = getNet()->getSrcFactor ();
  setE (VSRC_1, f * getPropertyDouble ("U"));
}

void vdc::initTR (void) {
  initDC ();
}

void vdc::initAC (void) {
  initDC ();
  setE (VSRC_1, 0.0);
}

void vdc::initHB (void) {
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2);
}

void vdc::calcHB (nr_double_t frequency) {
  if (frequency == 0.0) {
    setE (VSRC_1, getPropertyDouble ("U"));
  } else {
    setE (VSRC_1, 0);
  }
}
