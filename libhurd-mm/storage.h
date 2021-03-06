/* storage.h - Storage allocation functions.
   Copyright (C) 2007, 2008 Free Software Foundation, Inc.
   Written by Neal H. Walfield <neal@gnu.org>.

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

#ifndef _HURD_STORAGE_H
#define _HURD_STORAGE_H

#include <hurd/addr.h>
#include <hurd/cap.h>

enum storage_expectancy
  {
    /* Storage with an unknown life expectancy.  */
    STORAGE_UNKNOWN = 0,
    /* Storage that is likely to be deallocated quickly (seconds to a
       couple of minutes).  */
    STORAGE_EPHEMERAL,
    /* Storage that is likely to be deallocated at some time, but
       program will continue running much longer.  */
    STORAGE_MEDIUM_LIVED,
    /* Storage with an expected long-life (storage will most likely
       not be deallocated).  */
    STORAGE_LONG_LIVED,
  };

struct storage
{
  struct cap *cap;
  addr_t addr;
};

/* Allocate an object of type TYPE.  The object has a life expectancy
   of EXPECTANCY.  If ADDR is not ADDR_VOID, a capability to the
   storage will be saved at ADDR (and the shadow object updated
   appropriately).  On success, the shadow capability slot for the
   storage is returned (useful for setting up a shadow object) and the
   address of the storage object.  Otherwise, NULL and ADDR_VOID,
   respectively, are returned.  ACTIVITY is the activity to use to
   account the storage.

   NB: This function does not allocate a shadow object for the
   allocated object; it does update the relevant shadow objects so
   that the address of the storage and ADDR are reachable.  If the
   caller wants to use the allocated object for address translation,
   the caller must allocate the shadow object.  If not, functions
   including the cap_lookup family will fail.  */
extern struct storage storage_alloc (addr_t activity,
				     enum cap_type type,
				     enum storage_expectancy expectancy,
				     struct object_policy policy,
				     addr_t addr);
#define storage_alloc(__sa_activity, __sa_type, __sa_expectancy,	\
		      __sa_policy, __sa_addr)				\
  ({									\
    struct storage __sa_storage;					\
    __sa_storage = storage_alloc (__sa_activity, __sa_type,		\
				  __sa_expectancy, __sa_policy,		\
				  __sa_addr);				\
    debug (5, "storage_alloc (%s, " ADDR_FMT ") -> " ADDR_FMT,		\
	   cap_type_string (__sa_type), ADDR_PRINTF (__sa_addr),	\
	   ADDR_PRINTF (__sa_storage.addr));				\
    __sa_storage;							\
  })


/* Frees the storage at STORAGE.  STORAGE must be the address returned
   by storage_alloc (NOT the address provided to storage_alloc).  If
   UNMAP_NOW is not true, revoking the storage may be delayed.  */
extern void storage_free_ (addr_t storage, bool unmap_now);
#define storage_free(__sf_storage, __sf_unmap_now)			\
  ({									\
    debug (5, "storage_free (" ADDR_FMT ")", ADDR_PRINTF (__sf_storage)); \
    storage_free_ (__sf_storage, __sf_unmap_now);			\
  })

/* Initialize the storage sub-system.  */
extern void storage_init (void);

/* Used by as_init to initialize a folio's shadow object.  */
extern void storage_shadow_setup (struct cap *cap, addr_t folio);

/* Return whether there is sufficient reserve storage.  */
extern bool storage_have_reserve (void);

/* Check the amount of available storage, allocating more if required.
   This should be called by any function that takes the as_rwlock and
   allocates storage (even indirectly) after it has dropped the
   as_rwlock.  Pass false if you know that you do not hold the as_lock.
   True otherwise.  */
extern void storage_check_reserve (bool i_may_have_as_lock);

#endif /* _HURD_STORAGE_H  */
