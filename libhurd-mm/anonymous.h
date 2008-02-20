/* anonymous.h - Anonymous memory pager interface.
   Copyright (C) 2007, 2008 Free Software Foundation, Inc.
   Written by Neal H. Walfield <neal@gnu.org>.

   This file is part of the GNU Hurd.

   GNU Hurd is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with GNU Hurd.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef HURD_ANONYMOUS_H
#define HURD_ANONYMOUS_H

#include <hurd/pager.h>
#include <hurd/btree.h>
#include <hurd/addr.h>
#include <hurd/exceptions.h>
#include <l4/thread.h>

/* Forward.  */
struct anonymous_pager;

enum
  {
    /* If set, the address passed to the allocation function is
       mandatory.  */
    ANONYMOUS_FIXED = 1 << 0,

    /* If set, allocated memory need not be cleared.  */
    ANONYMOUS_NO_CLEAR = 1 << 1,

    /* If set and there is a fill function, the implementation will
       neither allocate memory nor clear the discarded bit before
       calling the fill function.  (Thus, allocation becomes the fill
       function's responsibility.)  */
    ANONYMOUS_NO_ALLOC = 1 << 2,

    /* If set, when a thread raises a fault while while it is in the
       fill function, fill is not recursively invoked but instead,
       memory is allocated (or the discarded bit cleared) as
       required.  */
    ANONYMOUS_NO_RECURSIVE = 1 << 3,

    /* Make access to the region thead-safe by creating a staging area
       which the fill function has access but which is only mapped
       into public area when the fill function returns.  */
    ANONYMOUS_THREAD_SAFE = 1 << 4,
  };

/* Generate the content for the page starting at page START and
   continuing for PAGES pages.  This function will not be invoked
   recursively.  The ANONYMOUS_NO_RECURSIVE flag determines what
   happens if the fill function causes a fault in the region it
   manages.  The implementation serializes calls to the fill
   function.  */
typedef bool (*anonymous_pager_fill_t) (struct anonymous_pager *anon,
					void *base, uintptr_t offset,
					uintptr_t pages,
					struct exception_info info);

struct anonymous_pager
{
  struct pager pager;

  /* Activity against which storage should be allocated.  */
  addr_t activity;

  /* The address space that we actually allocated vs the address space
     covered by the pager.  */
  struct pager_region alloced_region;

  /* The start of the staging area.  Only valid if
     ANONYMOUS_THREAD_SAFE is set.  */
  void *staging_area;

  /* The storage used by this pager.  */
  hurd_btree_t storage;

  uintptr_t flags;

  /* If not NULL, called when a fault is raised due to lack of storage
     or when a page that has been discarded is accessed.  */
  anonymous_pager_fill_t fill;

  /* The thread in the fill function.  (If none, NULL.)  */
  l4_thread_id_t fill_thread;
  ss_mutex_t fill_lock;

  struct object_policy policy;

  /* The implementation does not modify this value and assigns it no
     specific semantic meaning; the callbacks functions are free to
     use it as they see fit.  */
  void *cookie;
};

/* Set up an anonymous pager to cover a region SIZE bytes large.
   ADDR_HINT indicates the preferred starting address.  Unless
   ANONYMOUS_FIXED is included in FLAGS, the implementation may choose
   another address.  (The region will be allocated using as_alloc.)
   Both ADDR and SIZE must be a multiple of the base page size.  If
   the specified region overlaps with an existing pager, EEXIST is
   returned.  The chosen start address is returned in *ADDR_OUT.

   P specifies the policy to apply to all allocated objects in the
   region.

   If the caller wishes to generate the content of the region
   dynamically (a so-called lazily rendered region), the caller may
   pass the address of a fill function, F.  This function is called
   when a fault is raised due to an as-of-yet unallocated page or due
   to a discarded page (the latter is only possible if
   POLICY.DISCARDABLE is true).  A page is first allocated (according
   to whether ANONYMOUS_ZEROFILL in supplied in FLAGS) and installed.
   F is then invoked with the appropriate parameters.  If F itself
   causes a fault on the region, F is NOT recursively invoked.  The
   behavior is defined by the value of the ANONYMOUS_DOUBLE_FAULT flag
   in FLAGS.

   By default, access to rendered regions is not multi-thread safe.
   This is because between the installtion of the page and its filling
   by the fill function, a second thread may access the page.  To
   enable multi-threaded access, the caller may pass
   ANONYMOUS_THREAD_SAFE.  In this case, a staging area will be set
   up.  When the fill function is invoked, access to the main region
   is disabled; any access is blocked until the fill function
   returns.  */
extern struct anonymous_pager *anonymous_pager_alloc (addr_t activity,
						      void *addr_hint,
						      size_t size,
						      struct object_policy p,
						      uintptr_t flags,
						      anonymous_pager_fill_t f,
						      void **addr_out);

extern void anonymous_pager_destroy (struct anonymous_pager *pager);

#endif /* HURD_ANONYMOUS_H */
