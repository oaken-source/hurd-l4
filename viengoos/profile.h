/* profile.h - Profiling support interface.
   Copyright (C) 2008 Free Software Foundation, Inc.
   Written by Neal H. Walfield <neal@gnu.org>.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3 of the
   License, or (at your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include <stdint.h>

/* Start a timer for the profile site ID (this must be unique per
   site, can be the function's address).  NAME is the symbolic
   name.  */
extern void profile_start (uintptr_t id, const char *name);

/* End the timer for the profile site ID.  */
extern void profile_end (uintptr_t id);

extern void profile_stats_dump (void);
