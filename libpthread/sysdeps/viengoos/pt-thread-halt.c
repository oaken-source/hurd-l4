/* Deallocate the kernel thread resources.  Viengoos version.
   Copyright (C) 2007, 2008 Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <assert.h>
#include <errno.h>

#include <pt-internal.h>

#include <hurd/thread.h>

void
__pthread_thread_halt (struct __pthread *thread)
{
  if (thread->have_kernel_resources)
    thread_stop (thread->object);
}
