/*
 * iexp.h - pulse current source class definitions
 *
 * Copyright (C) 2007 Stefan Jahn <stefan@lkcc.org>
 * Copyright (C) 2007 Gunther Kraut <gn.kraut@t-online.de>
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
 * $Id: iexp.h,v 1.1 2007-04-15 10:17:49 ela Exp $
 *
 */

#ifndef __IEXP_H__
#define __IEXP_H__

class iexp : public circuit
{
 public:
  iexp ();
  void initSP (void);
  void initDC (void);
  void initAC (void);
  void initTR (void);
  void calcTR (nr_double_t);
};

#endif /* __IEXP_H__ */
