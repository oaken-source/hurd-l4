/* output.h - Output routines interfaces.
   Copyright (C) 2003 Free Software Foundation, Inc.
   Written by Marcus Brinkmann.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#ifndef _OUTPUT_H
#define _OUTPUT_H	1

/* Print the single character CHR on the output device.  */
void putchar (int chr);

void printf (const char *fmt, ...);

/* True if debug mode is enabled.  */
extern int output_debug;

/* Print a debug message.  */
#define debug(...) do { if (output_debug) printf (__VA_ARGS__); } while (0)

#endif	/* _OUTPUT_H */