/* Copyright (C) 2003 Free Software Foundation, Inc.
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

#ifndef _HURD_SLAB_H
#define _HURD_SLAB_H	1

#include <errno.h>


/* Initialize the slab object pointed to by BUFFER.  */
typedef error_t (*hurd_slab_constructor_t) (void *buffer);

/* Destroy the slab object pointed to by BUFFER.  */
typedef void (*hurd_slab_destructor_t) (void *buffer);

struct hurd_slab_space;
typedef struct hurd_slab_space *hurd_slab_space_t;


/* Create a new slab space with the given object size, constructor and
   destructor.  */
error_t hurd_slab_create (size_t size,
			  hurd_slab_constructor_t constructor,
			  hurd_slab_destructor_t destructor,
			  hurd_slab_space_t *space);

/* Allocate a new object from the slab space SPACE.  */
error_t hurd_slab_alloc (hurd_slab_space_t space, void **buffer);

/* Deallocate the object BUFFER from the slab space SPACE.  */
void hurd_slab_dealloc (hurd_slab_space_t space, void *buffer);

#endif	/* _HURD_SLAB_H */
