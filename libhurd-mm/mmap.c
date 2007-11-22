/* mmap.c - mmap implementation.
   Copyright (C) 2007 Free Software Foundation, Inc.
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

#include <hurd/stddef.h>
#include <hurd/addr.h>
#include <hurd/as.h>
#include <hurd/storage.h>
#include <hurd/anonymous.h>

#include <sys/mman.h>
#include <stdint.h>

void *
mmap (void *addr, size_t length, int protect, int flags,
      int filedes, off_t offset)
{
  if (length == 0)
    return MAP_FAILED;

  if ((flags & ~(MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED)))
    panic ("mmap called with invalid flags");
  if (protect != (PROT_READ | PROT_WRITE))
    panic ("mmap called with unsupported protection");

  if ((flags & MAP_FIXED))
    /* Interpret ADDR exactly.  */
    {
      if (((uintptr_t) addr & (PAGESIZE - 1)))
	return MAP_FAILED;
    }
  else
    {
      /* Round the address down to a multiple of the pagesize.  */
      addr = (void *) ((uintptr_t) addr & ~(PAGESIZE - 1));
    }

  /* Round length up.  */
  length = (length + PAGESIZE - 1) & ~(PAGESIZE - 1);

  bool alloced = false;
  if (addr)
    /* Caller wants a specific address range.  */
    {
      bool r = as_alloc_at (PTR_TO_ADDR (addr), length);
      if (r)
	alloced = true;
      else
	/* No room for this region.  */
	{
	  if (flags & MAP_FIXED)
	    return MAP_FAILED;
	}
    }

  if (! alloced)
    {
      addr_t region = as_alloc (PAGESIZE_LOG2, length / PAGESIZE, true);
      if (ADDR_IS_VOID (region))
	return MAP_FAILED;

      addr = ADDR_TO_PTR (addr_extend (region, 0, PAGESIZE_LOG2));
    }

  struct anonymous_pager *pager;
  pager = anonymous_pager_alloc (ADDR_VOID, (uintptr_t) addr, length,
				 ANONYMOUS_ZEROFILL, false);
  if (! pager)
    panic ("Failed to create pager!");

  return addr;
}

int
munmap (void *addr, size_t length)
{
  return 0;
}
