#define _L4_TEST_MAIN
#include "t-environment.h"

#include <hurd/types.h>
#include <hurd/stddef.h>
#include <hurd/as.h>

#include "memory.h"
#include "cap.h"
#include "object.h"
#include "activity.h"

struct activity *root_activity;

/* Current working folio.  */
static struct folio *folio;
static int object;

static struct as_allocate_pt_ret
allocate_object (enum cap_type type, addr_t addr)
{
  if (! folio || object == FOLIO_OBJECTS)
    {
      folio = folio_alloc (root_activity, FOLIO_POLICY_DEFAULT);
      object = 0;
    }

  struct as_allocate_pt_ret rt;
  rt.cap = folio_object_alloc (root_activity, folio, object ++,
			       type, OBJECT_POLICY_DEFAULT, 0);

  /* We don't need to set RT.STORAGE as as_insert doesn't require it
     for the internal interface implementations.  */
  rt.storage = ADDR (0, 0);
  return rt;
}

static struct as_allocate_pt_ret
allocate_page_table (addr_t addr)
{
  return allocate_object (cap_cappage, addr);
}

extern char _start;
extern char _end;

struct alloc
{
  addr_t addr;
  int type;
};

static void
try (struct alloc *allocs, int count, bool dump)
{
  struct cap aspace = { .type = cap_void };
  struct cap caps[count];

  void do_check (struct cap *cap, bool writable, int i, bool present)
    {
      if (present)
	{
	  assert (cap);

	  assert (cap->type == caps[i].type);

	  struct object *object = cap_to_object (root_activity, cap);
	  struct object_desc *odesc = object_to_object_desc (object);
	  if (caps[i].type != cap_void)
	    assert (odesc->oid == caps[i].oid);

	  if (cap->type == cap_page)
	    assert (* (unsigned char *) object == i);
	}
      else
	{
	  if (cap)
	    {
	      struct object *object = cap_to_object (root_activity, cap);
	      assert (! object);
	      /* This assertion relies on the fact that the
		 implementation will clear the type field on a failed
		 lookup.  */
	      assert (cap->type == cap_void);
	    }
	}
    }

  int i;
  for (i = 0; i < count; i ++)
    {
      switch (allocs[i].type)
	{
	case cap_folio:
	  caps[i] = object_to_cap ((struct object *)
				   folio_alloc (root_activity,
						FOLIO_POLICY_DEFAULT));
	  break;
	case cap_void:
	  caps[i].type = cap_void;
	  break;
	case cap_page:
	case cap_rpage:
	case cap_cappage:
	case cap_rcappage:
	  caps[i] = allocate_object (allocs[i].type, allocs[i].addr).cap;
	  break;
	default:
	  assert (! " Bad type");
	}

      struct object *object = cap_to_object (root_activity, &caps[i]);
      if (caps[i].type == cap_page)
	memset (object, i, PAGESIZE);

      as_insert_full (root_activity, ADDR_VOID, &aspace, allocs[i].addr,
		      ADDR_VOID, ADDR_VOID, object_to_cap (object),
		      allocate_page_table);

      if (dump)
	{
	  printf ("After inserting: " ADDR_FMT "\n",
		  ADDR_PRINTF (allocs[i].addr));
	  as_dump_from (root_activity, &aspace, NULL);
	}

      int j;
      for (j = 0; j < count; j ++)
	{
	  struct cap *cap = NULL;
	  bool w;

	  as_slot_lookup_rel_use
	    (root_activity, &aspace, allocs[j].addr,
	     ({
	       cap = slot;
	       w = writable;
	     }));
	  do_check (cap, w, j, j <= i);

	  struct cap c;
	  c = as_object_lookup_rel (root_activity,
				    &aspace, allocs[j].addr, -1,
				    &w);
	  do_check (&c, w, j, j <= i);
	}
    }

  /* Free the allocated objects.  */
  for (i = 0; i < count; i ++)
    {
      /* Make sure allocs[i].addr maps to PAGES[i].  */
      struct cap *cap = NULL;
      bool w;

      as_slot_lookup_rel_use (root_activity, &aspace, allocs[i].addr,
			      ({
				cap = slot;
				w = writable;
			      }));
      do_check (cap, w, i, true);

      struct cap c;
      c = as_object_lookup_rel (root_activity,
				&aspace, allocs[i].addr, -1,
				&w);
      do_check (&c, w, i, true);

      /* Void the capability in the returned capability slot.  */
      as_slot_lookup_rel_use (root_activity, &aspace, allocs[i].addr,
			      ({
				slot->type = cap_void;
			      }));

      /* The page should no longer be found.  */
      c = as_object_lookup_rel (root_activity, &aspace, allocs[i].addr, -1,
				NULL);
      assert (c.type == cap_void);

      /* Restore the capability slot.  */
      as_slot_lookup_rel_use (root_activity, &aspace, allocs[i].addr,
			      ({
				slot->type = allocs[i].type;
			      }));


      /* The page should be back.  */
      cap = NULL;
      as_slot_lookup_rel_use
	(root_activity, &aspace, allocs[i].addr,
	 ({
	   cap = slot;
	   w = writable;
	 }));
      do_check (cap, w, i, true);

      c = as_object_lookup_rel (root_activity,
				&aspace, allocs[i].addr, -1, &w);
      do_check (&c, w, i, true);

      /* Finally, free the object.  */
      switch (caps[i].type)
	{
	case cap_folio:
	  folio_free (root_activity,
		      (struct folio *) cap_to_object (root_activity,
						      &caps[i]));
	  break;
	case cap_void:
	  break;
	default:
	  object_free (root_activity, cap_to_object (root_activity, &caps[i]));
	  break;
	}

      /* Check the state of all pages.  */
      int j;
      for (j = 0; j < count; j ++)
	{
	  /* We should always get the slot (but it won't always
	     designate an object).  */
	  bool ret = as_slot_lookup_rel_use
	    (root_activity, &aspace, allocs[j].addr,
	     ({
	     }));
	  assert (ret);

	  struct cap c;
	  bool writable;
	  c = as_object_lookup_rel (root_activity,
				    &aspace, allocs[j].addr, -1, &writable);
	  do_check (&c, writable, j, i < j);
	}
    }
}

void
test (void)
{
  if (! memory_reserve ((l4_word_t) &_start, (l4_word_t) &_end,
			memory_reservation_self))
    panic ("Failed to reserve memory for self.");

  memory_grab ();
  object_init ();

  /* Create the root activity.  */
  folio = folio_alloc (NULL, FOLIO_POLICY_DEFAULT);
  if (! folio)
    panic ("Failed to allocate storage for the initial task!");

  struct cap c = allocate_object (cap_activity_control, ADDR_VOID).cap;
  root_activity = (struct activity *) cap_to_object (root_activity, &c);
    
  folio_parent (root_activity, folio);

  {
    printf ("Checking slot_lookup_rel... ");

    /* We have an empty address space.  When we use slot_lookup_rel
       and specify that we don't care what type of capability we get,
       we should get the capability slot--if the guard is right.  */
    struct cap aspace = { type: cap_void };

    l4_word_t addr = 0xFA000;
    bool ret = as_slot_lookup_rel_use (root_activity,
				       &aspace, ADDR (addr, ADDR_BITS),
				       ({ }));
    assert (! ret);

    /* Set the root to designate ADDR.  */
    bool r = CAP_SET_GUARD (&aspace, addr, ADDR_BITS);
    assert (r);
    
    ret = as_slot_lookup_rel_use (root_activity,
				  &aspace, ADDR (addr, ADDR_BITS),
				  ({
				    assert (slot == &aspace);
				    assert (writable);
				  }));
    assert (ret);

    printf ("ok.\n");
  }

  printf ("Checking as_insert... ");
  {
    struct alloc allocs[] =
      { { ADDR (1 << (FOLIO_OBJECTS_LOG2 + PAGESIZE_LOG2),
		ADDR_BITS - FOLIO_OBJECTS_LOG2 - PAGESIZE_LOG2), cap_folio },
	{ ADDR (0x100000003, 63), cap_page },
	{ ADDR (0x100000004, 63), cap_page },
	{ ADDR (0x1000 /* 4k.  */, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x00100000 /* 1MB */, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x01000000 /* 16MB */, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x10000000 /* 256MB */, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x40000000 /* 1000MB */, ADDR_BITS - PAGESIZE_LOG2),
	  cap_page },
	{ ADDR (0x40000000 - 0x2000 /* 1000MB - 4k */,
		ADDR_BITS - PAGESIZE_LOG2),
	  cap_page },
	{ ADDR (0x40001000, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x40003000, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x40002000, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x40009000, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x40008000, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x40007000, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x40006000, ADDR_BITS - PAGESIZE_LOG2), cap_page },
	{ ADDR (0x00101000 /* 1MB + 4k.  */, ADDR_BITS - PAGESIZE_LOG2),
	  cap_page },
	{ ADDR (0x00FF0000 /* 1MB - 4k.  */, ADDR_BITS - PAGESIZE_LOG2),
	  cap_page },
      };

    try (allocs, sizeof (allocs) / sizeof (allocs[0]), false);
  }

  {
    struct alloc allocs[] =
      { { ADDR (1, ADDR_BITS), cap_page },
	{ ADDR (2, ADDR_BITS), cap_page },
	{ ADDR (3, ADDR_BITS), cap_page },
	{ ADDR (4, ADDR_BITS), cap_page },
	{ ADDR (5, ADDR_BITS), cap_page },
	{ ADDR (6, ADDR_BITS), cap_page },
	{ ADDR (7, ADDR_BITS), cap_page },
	{ ADDR (8, ADDR_BITS), cap_page }
      };

    try (allocs, sizeof (allocs) / sizeof (allocs[0]), false);
  }

  {
    /* Induce a long different guard.  */
    struct alloc allocs[] =
      { { ADDR (0x100000000, 51), cap_cappage },
	{ ADDR (0x80000, 44), cap_folio }
      };

    try (allocs, sizeof (allocs) / sizeof (allocs[0]), false);
  }

  {
    /* Induce subpage allocation.  */
    struct alloc allocs[] =
      { { ADDR (0x80000, 44), cap_folio },
	{ ADDR (0x1000, 51), cap_page },
	{ ADDR (0x10000, 51), cap_page },
	{ ADDR (0x2000, 51), cap_page }
      };

    try (allocs, sizeof (allocs) / sizeof (allocs[0]), false);
  }

#warning Incorrect failure mode
#if 0
  {
    /* We do our best to not have to rearrange cappages.  However,
       consider the following scenario: we insert a number of adjacent
       cappages starting at 0.5 MB.  This requires cappage immediately
       above them.  Currently, we'd place a cappage at 0/44.  If we
       then try to insert a folio at 0/43, for which there is
       technically space, it will fail as there is no slot.

          0                    <- /43
          [ | | |...| | | ]
                     P P P     <- /51

       We can only insert at 0/44 if we first reduce the size of the
       cappage and introduce a 2 element page, the first slot of which
       would be used to point to the folio and the second to the
       smaller cappage.  */
    struct alloc allocs[] =
      { { ADDR (0x80000, 51), cap_page },
	{ ADDR (0x81000, 51), cap_page },
	{ ADDR (0x82000, 51), cap_page },
	{ ADDR (0x83000, 51), cap_page },
	{ ADDR (0x84000, 51), cap_page },
	{ ADDR (0x85000, 51), cap_page },
	{ ADDR (0x0, 44), cap_folio }
      };

    try (allocs, sizeof (allocs) / sizeof (allocs[0]), false);
  }
#endif

  printf ("ok.\n");
}
