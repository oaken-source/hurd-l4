/* assert.h - Dummy assert declaration for libc-pars.
   Copyright (C) 2003 Free Software Foundation, Inc.
   Written by Johan Rydberg.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _ASSERT_H
#define _ASSERT_H 1

/* Asserts aren't supported at the moment.  */
#define assert(expr)
#define assert_perror(err) assert(err == 0)

#endif /* assert.h */