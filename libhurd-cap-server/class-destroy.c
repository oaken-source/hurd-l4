/* class-destroy.c - Destroy a capability class.
   Copyright (C) 2004 Free Software Foundation, Inc.
   Written by Marcus Brinkmann <marcus@gnu.org>

   This file is part of the GNU Hurd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <pthread.h>

#include <hurd/cap-server.h>


/* Destroy the capability class CAP_CLASS and release all associated
   resources.  Note that this is only allowed if there are no
   capability objects in use, and if the capability class is not used
   by a capability server.  This function assumes that the class has
   been initialized with hurd_cap_class_init.  */
error_t
hurd_cap_class_destroy (hurd_cap_class_t cap_class)
{
  error_t err = 0;

  /* FIXME: This function needs to be revised.  We need to take the
     locks, and if only for memory synchronization.  */

  /* This will fail if there are still allocated capability
     objects.  */
  err = hurd_slab_destroy (&cap_class->obj_space);
  if (err)
    return err;

  /* At this point, destruction will succeed.  */
  pthread_cond_destroy (&cap_class->cond);
  pthread_mutex_destroy (&cap_class->lock);
  pthread_mutex_destroy (&cap_class->obj_cond_lock);
  pthread_cond_destroy (&cap_class->obj_cond);

  return 0;
}
