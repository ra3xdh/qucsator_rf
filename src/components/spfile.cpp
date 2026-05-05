/*
 * spfile.cpp - S-parameter file class implementation
 *
 * Copyright (C) 2004, 2005, 2006, 2008, 2009 Stefan Jahn <stefan@lkcc.org>
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
 * $Id$
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "object.h"
#include "vector.h"
#include "matrix.h"
#include "matvec.h"
#include "dataset.h"
#include "strlist.h"
#include "poly.h"
#include "spline.h"
#include "interpolator.h"
#include "constants.h"
#include "logging.h"
#include "spfile.h"

using namespace qucs;

// Constructor for S-parameter file vector.
spfile_vector::spfile_vector () {
  v = f = 0;
  isreal = 1;
  inter = NULL;
  r = c = 0;
}

// Destructor for S-parameter file vector.
spfile_vector::~spfile_vector () {
  delete inter;
}

// Passes vectors and their data types to the S-parameter file vector.
void spfile_vector::prepare (qucs::vector * _v, qucs::vector * _f,
			     bool _isreal, int it, int dt) {
  v = _v;
  f = _f;
  isreal = _isreal;
  inter = new interpolator ();
  if (isreal) {
    inter->rvectors (v, f);
    inter->prepare (it, REPEAT_NO, dt | DATA_REAL);
  }
  else {
    inter->cvectors (v, f);
    inter->prepare (it, REPEAT_NO, dt | DATA_COMPLEX);
  }
}

// Returns interpolated data.
nr_complex_t spfile_vector::interpolate (nr_double_t x) {
  if (isreal)
    return inter->rinterpolate (x);
  else
    return inter->cinterpolate (x);
}

// Constructor creates an empty and unnamed instance of the spfile class.
spfile::spfile () {
  data = NULL;
  sfreq = nfreq = NULL;
  spara = FMIN = SOPT = RN = NULL;
  interpolType = dataType = 0;
}

// Destructor deletes spfile object from memory.
spfile::~spfile () {
  delete[] spara;
  delete RN;
  delete FMIN;
  delete SOPT;
#if DEBUG && 0
  if (data) {
    data->setFile ("spfile.dat");
    data->print ();
  }
#endif
  delete data;
}

/* This function returns the S-parameter matrix of the circuit for the
   given frequency.  It uses interpolation for frequency points which
   are not part of the original touchstone file. */
matrix spfile::getInterpolMatrixS (nr_double_t frequency) {

  // first interpolate the matrix values
  matrix s (nPorts);
  for (int r = 0; r < nPorts; r++) {
    for (int c = 0; c < nPorts; c++) {
      int i = r * (nPorts + 1) + c;
      s.set (r, c, spara[i].interpolate (frequency));
    }
  }

  // then convert them to S-parameters if necessary
  switch (paraType) {
  case 'Y':
    s = ytos (s);
    break;
  case 'Z':
    s = ztos (s);
    break;
  case 'H':
    s = htos (s);
    break;
  case 'G':
    s = gtos (s);
    break;
  }
  return s;
}

/* This function expands the actual S-parameter file data stored
   within the touchstone file to have an additional reference one-port
   whose S-parameter is -1 (i.e. ground). */
matrix spfile::expandSParaMatrix (matrix s) {
  assert (s.getCols () == s.getRows ());
  int r, c, ports = s.getCols () + 1;
  nr_double_t g = -1;
  nr_complex_t fr, ss, sr, sc, sa;
  matrix res (ports);

  // compute S'mm
  for (sa = 0, r = 0; r < ports - 1; r++)
    for (c = 0; c < ports - 1; c++) sa += s.get (r, c);
  ss = (2 - g - ports + sa) / (1 - ports * g - sa);
  res.set (ports - 1, ports - 1, ss);
  fr = (1.0 - g * ss) / (1.0 - g);

  // compute S'im
  for (r = 0; r < ports - 1; r++) {
    for (sc = 0, c = 0; c < ports - 1; c++) sc += s.get (r, c);
    res.set (r, ports - 1, fr * (1.0 - sc));
  }

  // compute S'mj
  for (c = 0; c < ports - 1; c++) {
    for (sr = 0, r = 0; r < ports - 1; r++) sr += s.get (r, c);
    res.set (ports - 1, c, fr * (1.0 - sr));
  }

  // compute S'ij
  for (r = 0; r < ports - 1; r++) {
    for (c = 0; c < ports - 1; c++) {
      fr = g * res (r, ports - 1) * res (ports - 1, c) / (1.0 - g * ss);
      res.set (r, c, s.get (r, c) - fr);
    }
  }

  return res;
}

/* The function is the counterpart of the above expandSParaMatrix()
   function.  It shrinks the S-parameter matrix by removing the
   reference port. */
matrix spfile::shrinkSParaMatrix (matrix s) {
  assert (s.getCols () == s.getRows () && s.getCols () > 0);
  int r, c, ports = s.getCols ();
  nr_double_t g = -1;
  matrix res (ports - 1);

  // compute S'ij
  for (r = 0; r < ports - 1; r++) {
    for (c = 0; c < ports - 1; c++) {
      res.set (r, c, s (r, c) + g * s (r, ports - 1)  *
	       s (ports - 1, c) / (1.0 - g * s (ports - 1, ports - 1)));
    }
  }
  return res;
}

void spfile::prepare (void) {

}

/* The function creates an additional data vector for the given matrix
   entry and adds it to the dataset. */
void spfile::createVector (int r, int c) {
  int i = r * (nPorts + 1) + c;
  spara[i].r = r;
  spara[i].c = c;
  qucs::vector * v = new qucs::vector (matvec::createMatrixString ("S", r, c),
			       sfreq->getSize ());
  v->setDependencies (new strlist ());
  v->getDependencies()->add (sfreq->getName ());
  data->addVariable (v);
  spara[i].v = v;
}

/* This function goes through the dataset stored within the original
   touchstone file and looks for the S-parameter matrices and
   frequency vector.  It also tries to find the noise parameter
   data. */
void spfile::createIndex (void) {
  qucs::vector * v; int s = nPorts + 1;
  char * n;
  const char *name;
  int r, c, i;

  // go through list of dependency vectors and find frequency vectors
  for (v = data->getDependencies (); v != NULL; v = (::vector *) v->getNext ()) {
    if ((name = v->getName ()) != NULL) {
      if (!strcmp (name, "frequency")) sfreq = v;
      else if (!strcmp (name, "nfreq")) nfreq = v;
    }
  }

  // create vector index
  spara = new spfile_vector[s * s] ();

  // go through list of variable vectors and find matrix entries
  for (v = data->getVariables (); v != NULL; v = (::vector *) v->getNext ()) {
    if ((n = matvec::isMatrixVector (v->getName (), r, c)) != NULL) {
      // save matrix vector indices
      i = r * s + c;
      spara[i].r = r;
      spara[i].c = c;
      spara[i].prepare (v, sfreq, false, interpolType, dataType);
      paraType = n[0];  // save type of touchstone data
      free (n);
    }
    if ((name = v->getName ()) != NULL) {
      // find noise parameter vectors
      if (!strcmp (name, "Rn")) {
	RN = new spfile_vector ();
	RN->prepare (v, nfreq, true, interpolType, dataType);
      }
      else if (!strcmp (name, "Fmin")) {
	FMIN = new spfile_vector ();
	FMIN->prepare (v, nfreq, true, interpolType, dataType);
      }
      else if (!strcmp (name, "Sopt")) {
	SOPT = new spfile_vector ();
	SOPT->prepare (v, nfreq, false, interpolType, dataType);
      }
    }
  }
}

/* This function expands the actual noise correlation matrix to have an
   additional reference one-port whose S-parameter is -1
   (i.e. ground).  The given S-parameter matrix is required to perform
   this transformation. */
matrix spfile::expandNoiseMatrix(matrix n, matrix s, nr_double_t T)
{
  assert(s.getCols() == s.getRows() && n.getCols() == n.getRows() &&
         n.getCols() == s.getCols() - 1);
  int r, c, ports = n.getCols() + 1;
  nr_double_t g = -1;

  // create K matrix
  matrix k(ports, ports - 1);
  for (r = 0; r < ports - 1; r++)
  {
    for (c = 0; c < ports - 1; c++)
    {
      if (r == c)
        k.set(r, c, 1.0 + g * (s.get(r, ports - 1) - 1.0));
      else
        k.set(r, c, g * s.get(r, ports - 1));
    }
  }
  for (c = 0; c < ports - 1; c++)
    k.set(ports - 1, c, g * s.get(ports - 1, ports - 1) - 1.0);

  // create D vector
  matrix d(ports, 1);
  for (r = 0; r < ports - 1; r++)
    d.set(r, 0, s.get(r, ports - 1));
  d.set(ports - 1, 0, s.get(ports - 1, ports - 1) - 1.0);

  // expand noise correlation matrix
  matrix res(ports);
  res = (k * n * adjoint(k) - celsius2kelvin(T) / T0 * fabs(1 - norm(g)) * d * adjoint(d)) * norm(1 / (1 - g));
  return res;
}

/* This function computes the noise correlation matrix of a twoport
   based upon the noise parameters and the given S-parameter
   matrix. */
matrix spfile::correlationMatrix(nr_double_t Fmin, nr_complex_t Sopt,
                                 nr_double_t Rn, matrix s)
{
  assert(s.getCols() == s.getRows() && s.getCols() == 2);
  matrix c(2);
  nr_complex_t Kx = 4 * Rn / 50.0 / norm(1.0 + Sopt);
  c.set(0, 0, (Fmin - 1) * (norm(s.get(0, 0)) - 1) + Kx * norm(1.0 - s.get(0, 0) * Sopt));
  c.set(1, 1, norm(s.get(1, 0)) * ((Fmin - 1) + Kx * norm(Sopt)));
  c.set(0, 1, s.get(0, 0) / s.get(1, 0) * c.get(1, 1) - conj(s.get(1, 0)) * conj(Sopt) * Kx);
  c.set(1, 0, conj(c.get(0, 1)));
  return c;
}

/* This function computes the noise correlation matrix for the given
   S-parameter matrix.  If measured noise parameters (Rn, Fmin, Sopt)
   are available in the touchstone file, they are used to construct the
   correlation matrix.  Otherwise, if the network is passive, the thermal
   noise formula C_s = (I - S·S†) * T/T0 is applied.  If the network is
   neither measured nor passive, an error is logged and a zero matrix is
   returned. */
matrix spfile::computeNoiseCs(nr_double_t frequency, matrix s, nr_double_t T)
{
  int n = s.getCols();

  // TODO: perhaps we should also check if the matrix is 2x2?
  if (nfreq != NULL && RN != NULL && FMIN != NULL && SOPT != NULL)
  {
    nr_double_t r = real(RN->interpolate(frequency));
    nr_double_t f = real(FMIN->interpolate(frequency));
    nr_complex_t g = SOPT->interpolate(frequency);
    return correlationMatrix(f, g, r, s);
  }

  if (!isPassive(s))
  {
    logprint(LOG_ERROR, "ERROR: S-parameter file is not passive "
                        "at %g Hz — cannot compute noise\n",
             (double)frequency);
    return eye(n) * 0;
  }

  nr_double_t tr = celsius2kelvin(T) / T0;
  return (eye(n) - s * adjoint(s)) * tr;
}

/* This function checks whether the given S-parameter matrix represents
   a passive network by computing the largest eigenvalue of S·S†.
   A network is passive if all eigenvalues of S·S† are ≤ 1.
   For 2×2 matrices an analytical formula is used; for larger
   matrices the power iteration method is employed. */
bool spfile::isPassive (matrix s) {
  assert (s.getCols () == s.getRows ());
  int n = s.getCols ();
  matrix ssh = s * adjoint (s);

  if (n == 2) {
    // Analytical eigenvalues for 2×2 Hermitian matrix:
    // λ = (a+d)/2 ± sqrt(((a-d)/2)² + |b|²)
    nr_double_t a = real (ssh.get (0, 0));
    nr_double_t d = real (ssh.get (1, 1));
    nr_complex_t b = ssh.get (0, 1);
    nr_double_t tr = (a + d) / 2.0;
    nr_double_t det = ((a - d) / 2.0) * ((a - d) / 2.0) + norm (b);
    nr_double_t lambda_max = tr + std::sqrt (det);
    return lambda_max <= 1.0 + 1e-6;
  }

  // Power iteration for largest eigenvalue of Hermitian matrix ssh
  // Start with a non-zero initial vector
  matrix v (n, 1);
  for (int i = 0; i < n; i++)
    v.set (i, 0, nr_complex_t (1.0 / std::sqrt ((nr_double_t) n), 0.0));

  nr_double_t lambda = 0.0;
  for (int iter = 0; iter < 100; iter++) {
    matrix w = ssh * v;
    // Rayleigh quotient: λ = v†·w (v is unit-norm)
    nr_complex_t lambda_c = 0.0;
    for (int i = 0; i < n; i++)
      lambda_c += conj (v.get (i, 0)) * w.get (i, 0);
    lambda = real (lambda_c);

    // Normalize w for next iteration
    nr_double_t wn = 0.0;
    for (int i = 0; i < n; i++)
      wn += norm (w.get (i, 0));
    wn = std::sqrt (wn);
    if (wn < 1e-15) break;  // converged to zero
    for (int i = 0; i < n; i++)
      v.set (i, 0, w.get (i, 0) / wn);
  }

  return lambda <= 1.0 + 1e-6;
}
