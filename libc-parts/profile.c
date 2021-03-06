/* profile.c - Profiling support implementation.
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

#ifdef RM_INTERN
# include "../viengoos/zalloc.h"
#else
# include <stdlib.h>
#endif

#include "profile.h"
#include <hurd/ihash.h>
#include <stddef.h>
#include <string.h>
#include <l4.h>
#include <hurd/rm.h>

static struct hurd_ihash sites_hash;
static char sites_hash_buffer[PAGESIZE];
static int init;

/* XXX: This implementation assumes that we are single threaded!  In
   the case of Viengoos, this is essentially true: all relevant
   functionality is with the kernel lock held.  We also don't
   currently support nested calls.  */

#define SIZE 1000
struct site
{
  const char *name;
  uint64_t time;
  uint64_t start;
  int calls;
  int pending;
} sites[SIZE];
static int used;

static uint64_t epoch;
static uint64_t calls;
/* It's nothing but it prevents a divide by zero.  */
static uint64_t total_time = 1;
/* Number of extant profiling calls.  We only update total_time if
   EXTANT is 0.  The result is that the time spent profiling is
   correct, and the percent of the time profile that a function has
   been is more meaningful.  */
static int extant;

#undef profile_stats_dump
void
profile_stats_dump (void)
{
  uint64_t now = l4_system_clock ();

  int i;
  for (i = 0; i < used; i ++)
    if (sites[i].calls)
      s_printf ("%s:\t%d calls,\t%lld ms,\t%lld.%d us per call,\t"
		"%d%% total time,\t%d%% profiled time\n",
		sites[i].name,
		sites[i].calls,
		sites[i].time / 1000,
		sites[i].time / sites[i].calls,
		(int) ((10 * sites[i].time) / sites[i].calls) % 10,
		(int) ((100 * sites[i].time) / (now - epoch)),
		(int) ((100 * sites[i].time) / total_time));

  s_printf ("profiled time: %lld ms, calls: %lld\n",
	    total_time / 1000, calls);
  s_printf ("uptime: %lld ms\n", (now - epoch) / 1000);
}

#undef profile_start
void
profile_start (uintptr_t id, const char *name)
{
  if (! init)
    {
      init = 1;

      size_t size;
      void *buffer;
#ifdef RM_INTERN
      size = hurd_ihash_buffer_size (SIZE, false, 0);
      /* Round up to a multiple of the page size.  */
      size = (size + PAGESIZE - 1) & ~(PAGESIZE - 1);

      buffer = (void *) zalloc (size);
#else
      size = sizeof (sites_hash_buffer);
      buffer = sites_hash_buffer;
#endif
      if (! buffer)
	panic ("Failed to allocate memory for object hash!\n");

      memset (buffer, 0, size);

      hurd_ihash_init_with_buffer (&sites_hash, false,
				   HURD_IHASH_NO_LOCP,
				   buffer, size);

      epoch = l4_system_clock ();

      init = 2;
    }
  else if (init == 1)
    return;

  struct site *site = hurd_ihash_find (&sites_hash, id);
  if (! site)
    {
      site = &sites[used ++];
      if (used == SIZE)
	panic ("Out of profile space.");

      error_t err = hurd_ihash_add (&sites_hash, id, site);
      if (err)
	panic ("Failed to add to hash: %d.", err);

      if (name)
	site->name = name;
      else
	site->name = rm_method_id_string (id);
    }

  extant ++;

  site->pending ++;
  if (site->pending == 1)
    site->start = l4_system_clock ();
}

#undef profile_end
void
profile_end (uintptr_t id)
{
  if (init == 1)
    return;

  struct site *site = hurd_ihash_find (&sites_hash, id);
  if (! site)
    panic ("profile_end called without corresponding profile_begin (%p)!",
	   id);

  if (! site->pending)
    panic ("profile_end called but no extant profile_start!");

  extant --;

  site->pending --;
  if (site->pending == 0)
    {
      uint64_t now = l4_system_clock ();

      site->time += now - site->start;

      if (extant == 0)
	total_time += now - site->start;

      site->calls ++;
      calls ++;

      do_debug (5)
	if (calls % 100000 == 0)
	  profile_stats_dump ();
    }
}

