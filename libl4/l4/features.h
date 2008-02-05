/* l4/features.h - Public interface to the L4 library.
   Copyright (C) 2004, 2008 Free Software Foundation, Inc.
   Written by Marcus Brinkmann <marcus@gnu.org>.

   This file is part of the GNU L4 library.
 
   The GNU L4 library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.
 
   The GNU L4 library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.
 
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU L4 library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#ifndef _L4_FEATURES_H
#define _L4_FEATURES_H	1


/* We define the GNU interface if _L4_COMPAT is not explicitly
   requested.  */
#ifdef _L4_COMPAT
#define _L4_INTERFACE_L4	1
#else
#define _L4_INTERFACE_GNU	1
#endif

/* The rest of the file is here because every other header file
   includes this one.  */

#define _L4_attribute_always_inline	__attribute__((__always_inline__))

#define _L4_attribute_const		__attribute__((__const__))

#define _L4_attribute_alias(name)	__attribute__((__alias__(#name)))

#endif	/* _L4_FEATURES_H */
