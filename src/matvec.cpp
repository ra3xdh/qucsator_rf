/*
 * matvec.cpp - matrix vector class implementation
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
 * $Id: matvec.cpp,v 1.5 2004/07/26 06:30:28 ela Exp $
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "logging.h"
#include "object.h"
#include "complex.h"
#include "vector.h"
#include "matrix.h"
#include "matvec.h"

// Constructor creates an unnamed instance of the matvec class.
matvec::matvec () {
  size = 0;
  rows = cols = 0;
  name = NULL;
  data = NULL;
}

/* Constructor creates an unnamed instance of the matvec class with a
   certain number of empty matrices. */
matvec::matvec (int length, int r, int c) {
  size = length;
  rows = r;
  cols = c;
  name = NULL;
  data = (length > 0) ? new matrix[length] (r, c) : NULL;
}

/* The copy constructor creates a new instance based on the given
   matvec object. */
matvec::matvec (const matvec & m) {
  size = m.size;
  rows = m.rows;
  cols = m.cols;
  name = m.name ? strdup (name) : NULL;
  data = NULL;

  // copy matvec elements
  if (size > 0) {
    data = new matrix[size] (rows, cols);
    for (int i = 0; i < size; i++) data[i] = m.data[i];
  }
}

// Destructor deletes a matvec object.
matvec::~matvec () {
  if (data) delete[] data;
}

// Sets the name of the matvec object.
void matvec::setName (char * n) {
  if (name) free (name);
  name = n ? strdup (n) : NULL;
}

// Returns the name of the matvec object.
char * matvec::getName (void) {
  return name;
}

/* This function saves the given vector to the matvec object with the
   appropriate matrix indices. */
void matvec::set (vector & v, int r, int c) {
  assert (v.getSize () == size && 
	  r >= 1 && r <= rows && c >= 1 && c <= cols);
  for (int i = 0; i < size; i++) data[i].set (r, c, v.get (i));
}

/* The function returns the vector specified by the given matrix
   indices.  If the matrix vector has a valid name 'A' the returned
   vector gets the name 'A[r,c]'. */
vector& matvec::get (int r, int c) {
  assert (r >= 1 && r <= rows && c >= 1 && c <= cols);
  vector * res = new vector ();
  for (int i = 0; i < size; i++) res->add (data[i].get (r, c));
  if (name != NULL) {
    res->setName (createMatrixString (name, r, c));
  }
  return *res;
}

/* This function returns a static text representation with the
   'n[r,c]' scheme indicating a matrix (vector) entry. */
char * matvec::createMatrixString (char * n, int r, int c) {
  static char str[256];
  sprintf (str, "%s[%d,%d]", n, r, c);
  return str;
}

/* This function also returns a static text representation with the
   'n[r,c]' scheme indicating a matrix (vector) entry but with
   different arguments. */
char * matvec::createMatrixString (char n, int r, int c) {
  static char str[256];
  sprintf (str, "%c[%d,%d]", n, r, c);
  return str;
}

/* The function investigates the given vectors name.  If this name
   matches the 'n[r,c]' pattern it returns the name 'n' and saves the
   row and column indices as well.  The caller is responsible to
   'free()' the returned string.  If the vectors name does not match
   the pattern the function returns NULL. */
char * matvec::isMatrixVector (char * n, int& r, int& c) {
  char * p; int len;
  if (n == NULL) return NULL;             // nothing todo here
  if ((p = index (n, '[')) != NULL) {     // find first '['
    r = atoi (p + 1);                     // get first index
    if ((p = index (p, ',')) != NULL) {   // find the ','
      c = atoi (p + 1);                   // get second index
      if ((p = index (p, ']')) != NULL) { // find trailing ']'
	if (p[1] == '\0') {               // identifier must end in ']'
	  // parse actual identifier
	  if ((len = index (n, '[') - n) > 0) {
	    p = (char *) malloc (len + 1);
	    memcpy (p, n, len);
	    p[len] = '\0';
	    return p;
	  }
	}
      }
    }
  }
  return NULL;
}

/* This function saves the given matrix in the matrix vector at the
   specified position. */
void matvec::set (matrix& m, int idx) {
  assert (m.getRows () == rows && m.getCols () == cols &&
	  idx >= 0 && idx < size);
  data[idx] = m;
}

/* The function returns the matrix stored within the matrix vector at
   the given position. */
matrix& matvec::get (int idx) {
  assert (idx >= 0 && idx < size);
  matrix * res = new matrix (data[idx]);
  return *res;
}

// Matrix vector addition.
matvec& operator + (matvec& a, matvec& b) {
  assert (a.getRows () == b.getRows () && a.getCols () == b.getCols () &&
	  a.getSize () == b.getSize ());
  matvec * res = new matvec (a.getSize (), a.getRows (), a.getCols ());
  for (int i = 0; i < a.getSize (); i++) res->set (a.get (i) + b.get (i), i);
  return *res;
}

// Intrinsic matrix vector addition.
matvec& matvec::operator += (matvec& a) {
  assert (a.getRows () == rows && a.getCols () == cols &&
	  a.getSize () == size);
  for (int i = 0; i < size; i++) data[i] += a.get (i);
  return *this;
}

// Matrix vector subtraction.
matvec& operator - (matvec& a, matvec& b) {
  assert (a.getRows () == b.getRows () && a.getCols () == b.getCols () &&
	  a.getSize () == b.getSize ());
  matvec * res = new matvec (a.getSize (), a.getRows (), a.getCols ());
  for (int i = 0; i < a.getSize (); i++) res->set (a.get (i) - b.get (i), i);
  return *res;
}

// Intrinsic matrix vector subtraction.
matvec& matvec::operator -= (matvec& a) {
  assert (a.getRows () == rows && a.getCols () == cols &&
	  a.getSize () == size);
  for (int i = 0; i < a.getSize (); i++) data[i] -= a.get (i);
  return *this;
}

// Matrix vector scaling.
matvec& operator * (matvec& a, complex z) {
  matvec * res = new matvec (a.getSize (), a.getRows (), a.getCols ());
  for (int i = 0; i < a.getSize (); i++) res->set (a.get (i) * z, i);
  return *res;
}

// Matrix vector scaling in different order.
matvec& operator * (complex z, matvec& a) {
  return a * z;
}

// Matrix vector scaling.
matvec& operator / (matvec& a, complex z) {
  matvec * res = new matvec (a.getSize (), a.getRows (), a.getCols ());
  for (int i = 0; i < a.getSize (); i++) res->set (a.get (i) / z, i);
  return *res;
}

// Matrix vector multiplication.
matvec& operator * (matvec& a, matvec& b) {
  assert (a.getCols () == b.getRows () && a.getSize () == b.getSize ());
  matvec * res = new matvec (a.getSize (), a.getRows (), b.getCols ());
  for (int i = 0; i < a.getSize (); i++) res->set (a.get (i) * b.get (i), i);
  return *res;
}

// Conjugate complex matrix vector.
matvec& conj (matvec& a) {
  matvec * res = new matvec (a.getSize (), a.getRows (), a.getCols ());
  for (int i = 0; i < a.getSize (); i++) res->set (conj (a.get (i)), i);
  return *res;
}

// Transpose the matrix vector.
matvec& transpose (matvec& a) {
  matvec * res = new matvec (a.getSize (), a.getCols (), a.getRows ());
  for (int i = 0; i < a.getSize (); i++) res->set (transpose (a.get (i)), i);
  return *res;
}
