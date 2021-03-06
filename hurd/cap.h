/* cap.h - Capability definitions.
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

#ifndef _HURD_CAP_H
#define _HURD_CAP_H 1

#include <hurd/types.h>
#include <hurd/stddef.h>
#include <hurd/addr-trans.h>
#include <hurd/startup.h>
#include <hurd/error.h>
#include <stdint.h>

/* Capabilities.

   Capabilities have three functions: a capability can designate an
   object, it can participate in address translation, and it can be
   used to control how the designated object should be managed.  */

/* The types of objects designated by capabilities.  */
enum cap_type
  {
#define CAP_TYPE_MIN cap_void
    cap_void,
    cap_page,
    cap_rpage,
    cap_cappage,
    cap_rcappage,
    cap_folio,
    cap_activity,
    cap_activity_control,
    cap_thread,
#define CAP_TYPE_MAX cap_thread
  };

static inline const char *
cap_type_string (enum cap_type type)
{
  switch (type)
    {
    case cap_void:
      return "void";
    case cap_page:
      return "page";
    case cap_rpage:
      return "rpage";
    case cap_cappage:
      return "cappage";
    case cap_rcappage:
      return "rcappage";
    case cap_folio:
      return "folio";
    case cap_activity:
      return "activity";
    case cap_activity_control:
      return "activity_control";
    case cap_thread:
      return "thread";
    default:
      return "unknown cap type";
  };
}

/* Return whether two types are compatible in the sense that two caps
   with the given types can designate the same object.  */
static inline bool
cap_types_compatible (enum cap_type a, enum cap_type b)
{
  if (a == b)
    return true;

  if (a == cap_page && b == cap_rpage)
    return true;
  if (a == cap_rpage && b == cap_page)
    return true;

  if (a == cap_cappage && b == cap_rcappage)
    return true;
  if (a == cap_rcappage && b == cap_cappage)
    return true;

  if (a == cap_activity && b == cap_activity_control)
    return true;
  if (a == cap_activity_control && b == cap_activity)
    return true;

  return false;
}

/* Returns weather TYPE corresponds to a weak type.  */
static inline bool
cap_type_weak_p (enum cap_type type)
{
  switch (type)
    {
    case cap_rpage:
    case cap_rcappage:
    case cap_activity:
      return true;

    default:
      return false;
    }
}

/* Returns the weakened type corresponding to TYPE.  If type is
   already a weak type, returns TYPE.  */
static inline enum cap_type
cap_type_weaken (enum cap_type type)
{
  switch (type)
    {
    case cap_page:
    case cap_rpage:
      return cap_rpage;

    case cap_cappage:
    case cap_rcappage:
      return cap_rcappage;

    case cap_activity_control:
    case cap_activity:
      return cap_activity;

    default:
      return cap_void;
    }
}

/* Returns the strong type corresponding to TYPE.  If type is already
   a strong type, returns TYPE.  */
static inline enum cap_type
cap_type_strengthen (enum cap_type type)
{
  switch (type)
    {
    case cap_page:
    case cap_rpage:
      return cap_page;

    case cap_cappage:
    case cap_rcappage:
      return cap_cappage;

    case cap_activity_control:
    case cap_activity:
      return cap_activity_control;

    default:
      return type;
    }
}

/* Object policy.  */

/* The object priority is a signed 10-bit number.  A lower numeric
   value corresponds to a lower priority.  */
#define OBJECT_PRIORITY_BITS 10
#define OBJECT_PRIORITY_MIN (-(1 << (OBJECT_PRIORITY_BITS - 1)))
#define OBJECT_PRIORITY_LRU (0)
#define OBJECT_PRIORITY_DEFAULT (0)
#define OBJECT_PRIORITY_MAX ((1 << (OBJECT_PRIORITY_BITS - 1)) - 1)

struct object_policy
{
  union
  {
    struct
    {
      int16_t pad0 : 5;

      /* Whether a page is discardable (if so and the page is not
	 zero, trying to read the page from disk generates a first
	 fault fault).  */
      int16_t discardable : 1;

      /* An object's priority.  If can be used to override LRU
	 eviction.  When a memory object is to be evicted, we select
	 the object with the lowest priority (higher value = lower
	 priority).  */
      int16_t priority : OBJECT_PRIORITY_BITS;
    };
    uint16_t raw;
  };
};

#define OBJECT_POLICY_INIT { { raw: 0 } }
#define OBJECT_POLICY(__op_discardable, __op_priority) \
  (struct object_policy) { { { 0, (__op_discardable), (__op_priority) } } }
/* The default object policy: not discardable, managed by LRU.  */
#define OBJECT_POLICY_VOID \
  OBJECT_POLICY (false,  OBJECT_PRIORITY_LRU)
/* Synonym for OBJECT_POLICY_VOID.  */
#define OBJECT_POLICY_DEFAULT OBJECT_POLICY_VOID

/* Capability properties.  */

struct cap_properties
{
  struct object_policy policy;
  struct cap_addr_trans addr_trans;
};

#define CAP_PROPERTIES_INIT \
  { OBJECT_POLICY_INIT, CAP_ADDR_TRANS_INIT }
#define CAP_PROPERTIES(__op_object_policy, __op_addr_trans) \
  (struct cap_properties) { __op_object_policy, __op_addr_trans }
#define CAP_PROPERTIES_VOID \
  CAP_PROPERTIES (OBJECT_POLICY_INIT, CAP_ADDR_TRANS_INIT)
#define CAP_PROPERTIES_DEFAULT CAP_PROPERTIES_VOID

/* Capability representation.  */

#ifdef RM_INTERN
/* An OID corresponds to a page on a volume.  Only the least 54 bits
   are significant.  */
typedef l4_uint64_t oid_t;
#define OID_FMT "0x%llx"
#define OID_PRINTF(__op_oid) ((oid_t) (__op_oid))
#endif

#define CAP_VERSION_BITS 20
#define CAP_TYPE_BITS 6

struct cap
{
#ifdef RM_INTERN
  /* For a description of how versioning works, refer to the comment
     titled "Object versioning" in object.h.  */
  uint32_t version : CAP_VERSION_BITS;
  /* Whether the capability is weak.  */
  uint32_t weak_p : 1;

  /* Whether the designated object may be discarded.  */
  uint32_t discardable : 1;
  /* The designated object's priority.  */
  uint32_t priority : OBJECT_PRIORITY_BITS;

  struct cap_addr_trans addr_trans;

  uint64_t type : CAP_TYPE_BITS;

  /* If the capability designates an object, the object id.  */
  uint64_t oid : 64 - CAP_TYPE_BITS;
#else
  /* The shadow object (only for cappages and folios).  */
  struct object *shadow;

  uint32_t discardable : 1;
  uint32_t priority : OBJECT_PRIORITY_BITS;

  uint32_t type : CAP_TYPE_BITS;

  uint32_t pad0 : 32 - 1 - OBJECT_PRIORITY_BITS - CAP_TYPE_BITS;

  /* This capability's address description.  */
  struct cap_addr_trans addr_trans;
#endif
};

/* Return CAP's policy.  */
#define CAP_POLICY_GET(__cpg_cap)				\
  OBJECT_POLICY ((__cpg_cap).discardable, (__cpg_cap).priority)
/* Set CAP's policy to POLICY.  */
#define CAP_POLICY_SET(__cps_cap, __cps_policy)			\
  do								\
    {								\
      (__cps_cap)->discardable = (__cps_policy).discardable;	\
      (__cps_cap)->priority = (__cps_policy).priority;		\
    }								\
  while (0)

/* Return CAP's properties.  */
#define CAP_PROPERTIES_GET(__cpg_cap)				\
  CAP_PROPERTIES (CAP_POLICY_GET (__cpg_cap),			\
		  (__cpg_cap).addr_trans)
/* Set *CAP's properties to PROPERTIES.  */
#define CAP_PROPERTIES_SET(__cps_cap, __cps_properties)			\
  do									\
    {									\
      CAP_POLICY_SET (__cps_cap, (__cps_properties).policy);		\
      (__cps_cap)->addr_trans = (__cps_properties).addr_trans;		\
    }									\
  while (0)

/* Convenience macros for printing capabilities.  */

#ifdef RM_INTERN
#define CAP_FMT "{ " OID_FMT ".%d:%s %llx/%d; %d/%d }"
#define CAP_PRINTF(cap) \
  OID_PRINTF ((cap)->oid), (cap)->version, cap_type_string ((cap)->type), \
  CAP_GUARD ((cap)), CAP_GUARD_BITS ((cap)), \
  CAP_SUBPAGE ((cap)), CAP_SUBPAGES ((cap))
#else
#define CAP_FMT "{ %s %llx/%d; %d/%d }"
#define CAP_PRINTF(cap) \
  cap_type_string ((cap)->type), \
  CAP_GUARD ((cap)), CAP_GUARD_BITS ((cap)), \
  CAP_SUBPAGE ((cap)), CAP_SUBPAGES ((cap))
#endif

/* Accessors corresponding to the CAP_ADDR_TRANS macros.  */
#define CAP_SUBPAGES_LOG2(cap_) \
  CAP_ADDR_TRANS_SUBPAGES_LOG2((cap_)->addr_trans)
#define CAP_SUBPAGES(cap_) CAP_ADDR_TRANS_SUBPAGES ((cap_)->addr_trans)
#define CAP_SUBPAGE(cap_) CAP_ADDR_TRANS_SUBPAGE((cap_)->addr_trans)
#define CAP_SUBPAGE_SIZE_LOG2(cap_) \
  CAP_ADDR_TRANS_SUBPAGE_SIZE_LOG2 ((cap_)->addr_trans)
#define CAP_SUBPAGE_SIZE(cap_) \
  CAP_ADDR_TRANS_SUBPAGE_SIZE ((cap_)->addr_trans)
#define CAP_SUBPAGE_OFFSET(cap_) \
  CAP_ADDR_TRANS_SUBPAGE_OFFSET((cap_)->addr_trans)
#define CAP_GUARD_BITS(cap_) CAP_ADDR_TRANS_GUARD_BITS((cap_)->addr_trans)
#define CAP_GUARD(cap_) CAP_ADDR_TRANS_GUARD((cap_)->addr_trans)

/* NB: Only updates the shadow guard; NOT the capability.  If the
   latter behavior is desired, use cap_copy_x instead.  */
#define CAP_SET_GUARD_SUBPAGE(cap_, guard_, gdepth_, subpage_, subpages_) \
  ({ bool r_ = true; \
     if ((subpages_) != 1 \
	 && ! ((cap_)->type == cap_cappage || (cap_)->type == cap_rcappage)) \
       { \
         debug (1, "Subpages are only allow for cappages."); \
         r_ = false; \
       } \
     if (r_) \
       r_ = CAP_ADDR_TRANS_SET_GUARD_SUBPAGE (&(cap_)->addr_trans, \
					      (guard_), (gdepth_), \
					      (subpage_), (subpages_)); \
     r_; \
  })

#define CAP_SET_GUARD(cap_, guard_, gdepth_) \
  CAP_SET_GUARD_SUBPAGE ((cap_), (guard_), (gdepth_), \
			 CAP_SUBPAGE ((cap_)), CAP_SUBPAGES ((cap_)))
#define CAP_SET_SUBPAGE(cap_, subpage_, subpages_) \
  CAP_SET_GUARD_SUBPAGE ((cap_), CAP_GUARD (cap_), CAP_GUARD_BITS (cap_), \
			 (subpage_), (subpages_))

/* Capability-related methods.  */

#define RPC_STUB_PREFIX rm
#define RPC_ID_PREFIX RM
#undef RPC_TARGET_NEED_ARG
#define RPC_TARGET \
  ({ \
    extern struct hurd_startup_data *__hurd_startup_data; \
    __hurd_startup_data->rm; \
  })

#include <hurd/rpc.h>

enum
  {
    RM_cap_copy = 300,
    RM_cap_rubout,
    RM_cap_read,

    RM_object_slot_copy_out = 400,
    RM_object_slot_copy_in,
    RM_object_slot_read,
    RM_object_discarded_clear,
    RM_object_discard,
    RM_object_status,
    RM_object_name,
  };

enum
{
  /* Use subpage in CAP_ADDR_TRANS (must be a subset of subpage in
     SOURCE).  */
  CAP_COPY_COPY_ADDR_TRANS_SUBPAGE = 1 << 0,
  /* Use guard in TARGET, not the guard in CAP_ADDR_TRANS.  */
  CAP_COPY_COPY_ADDR_TRANS_GUARD = 1 << 1,
  /* Use guard in SOURCE.  */
  CAP_COPY_COPY_SOURCE_GUARD = 1 << 2,

  /* When copying the capability copies a weakened reference.  */
  CAP_COPY_WEAKEN = 1 << 3,

  /* Set the discardable bit on the capability.  */
  CAP_COPY_DISCARDABLE_SET = 1 << 4,

  /* Set the priority of the object.  */
  CAP_COPY_PRIORITY_SET = 1 << 5,
};

/* Copy the capability in capability slot SOURCE in the address space
   rooted at SOURCE_ADDRESS_SPACE to the slot TARGET in the address
   space rooted at TARGET_ADDRESS_SPACE.  The address space is
   resolved in the context of the caller.  If the address space
   identifies a thread, its address space root is used.  If it is
   ADDR_VOID, then the calling thread's address space route is used.
   (PRINCIPAL and the address spaces are looked up in the context of
   the caller.)

   By default, preserves SOURCE's subpage specification and copies
   TARGET's guard and policy.

   If CAP_COPY_COPY_SUBPAGE is set, then uses the subpage
   specification in CAP_PROPERTIES.  If CAP_COPY_COPY_ADDR_TRANS_GUARD
   is set, uses the guard description in CAP_PROPERTIES.

   If CAP_COPY_COPY_SOURCE_GUARD is set, uses the guard description in
   source.  Otherwise, preserves the guard in TARGET.

   If CAP_COPY_WEAKEN is set, saves a weakened version of SOURCE in
   *TARGET (e.g., if SOURCE's type is cap_page, *TARGET's type is set
   to cap_rpage).

   If CAP_COPY_DISCARDABLE_SET is set, then sets the discardable bit
   based on the value in PROPERTIES.  Otherwise, copies SOURCE's
   value.

   If CAP_COPY_PRIORITY_SET is set, then sets the priority based on
   the value in properties.  Otherwise, copies SOURCE's value.  */
RPC(cap_copy, 7, 0, addr_t, principal,
    addr_t, target_address_space, addr_t, target,
    addr_t, source_address_space, addr_t, source,
    l4_word_t, flags, struct cap_properties, properties)

/* Overwrite the capability slot TARGET in address space
   TARGET_ADDRESS_SPACE with a void capability.  */
RPC(cap_rubout, 3, 0, addr_t, principal,
    addr_t, target_address_space, addr_t, target)

/* Returns the public bits of the capability CAP in TYPE and
   CAP_PROPERTIES.  */
RPC(cap_read, 3, 2, addr_t, principal, addr_t, address_space, addr_t, cap,
    /* Out: */
    l4_word_t, type, struct cap_properties, properties)

/* Copy the capability from slot SLOT of the object OBJECT (relative
   to the start of the object's subpage) to slot TARGET.  PROPERTIES
   are interpreted as per cap_copy.  */
RPC(object_slot_copy_out, 8, 0, addr_t, principal,
    addr_t, object_address_space, addr_t, object, l4_word_t, slot,
    addr_t, target_address_space, addr_t, target,
    l4_word_t, flags, struct cap_properties, properties)

/* Copy the capability from slot SOURCE to slot INDEX of the object
   OBJECT (relative to the start of the object's subpage).  PROPERTIES
   are interpreted as per cap_copy.  */
RPC(object_slot_copy_in, 8, 0, addr_t, principal,
    addr_t, object_address_space, addr_t, object, l4_word_t, index,
    addr_t, source_address_space, addr_t, source,
    l4_word_t, flags, struct cap_properties, properties)

/* Store the public bits of the capability slot SLOT of object OBJECT
   in TYPE and CAP_PROPERTIES.  */
RPC(object_slot_read, 4, 2, addr_t, principal, addr_t, address_space,
    addr_t, object, l4_word_t, slot,
    /* Out: */
    l4_word_t, type, struct cap_properties, properties)

/* Clear the discarded bit.  */
RPC(object_discarded_clear, 2, 0,
    addr_t, principal, addr_t, object)

/* If the object designated by OBJECT is in memory, discard it.
   OBJECT must have write authority.  This does not set the object's
   discarded bit and thus does not result in a fault.  Instead, the
   next access will see zero-filled memory.  */
RPC(object_discard, 2, 0,
    addr_t, principal, addr_t, object)

enum
{
  object_dirty = 1 << 0,
  object_referenced = 1 << 1,
};

/* Returns whether OBJECT is dirty.  If CLEAR is set, the dirty bit is
   clear.  An object's dirty bit is set when the object is modified.
   (Note: this is not the state of a frame but an indication of
   whether the object has been modified since the last time it the
   dirty bit was cleared.)  */
RPC (object_status, 3, 1, addr_t, principal, addr_t, object, bool, clear,
     uintptr_t, status)

struct object_name
{
  char name[12];
};

/* Give object OBJECT a name.  This is only used for debugging
   purposes and is only supported by some objects, in particular,
   activities and threads.  */
RPC (object_name, 3, 0, addr_t, principal,
     addr_t, object, struct object_name, name);
     

#undef RPC_STUB_PREFIX
#undef RPC_ID_PREFIX
#undef RPC_TARGET

/* An object.  */

/* The number of capabilities per page.  */
enum
  {
    CAPPAGE_SLOTS = PAGESIZE / 16,
  };
/* The log2 of the number of capabilities per page.  */
enum
  {
    CAPPAGE_SLOTS_LOG2 = PAGESIZE_LOG2 - 4,
  };

struct object
{
  union
  {
    char data[PAGESIZE];
    struct cap caps[CAPPAGE_SLOTS];
  };
};

#ifdef RM_INTERN
typedef struct activity *activity_t;
#else
typedef addr_t activity_t;
#endif

#ifndef RM_INTERN
/* Return the address of cap CAP's shadow object.  */
static inline void *
cap_get_shadow (const struct cap *cap)
{
  return cap->shadow;
}

/* Set CAP's shadow object to SHADOW.  */
static inline void
cap_set_shadow (struct cap *cap, void *shadow)
{
  cap->shadow = shadow;
}
#endif

/* Given cap CAP, return the corresponding object, or NULL, if there
   is none.  */
#ifdef RM_INTERN
extern struct object *cap_to_object (activity_t activity, struct cap *cap);
#else
static inline struct object *
cap_to_object (activity_t activity, struct cap *cap)
{
  return cap_get_shadow (cap);
}
#endif

/* Wrapper for the cap_copy method.  Also updates shadow
   capabilities.  */
static inline bool
cap_copy_x (activity_t activity,
	    addr_t target_address_space, struct cap *target, addr_t target_addr,
	    addr_t source_address_space, struct cap source, addr_t source_addr,
	    int flags, struct cap_properties properties)
{
  /* By default, we preserve SOURCE's subpage specification.  */
  int subpage = CAP_SUBPAGE (&source);
  int subpages = CAP_SUBPAGES (&source);

  if ((flags & CAP_COPY_COPY_ADDR_TRANS_SUBPAGE))
    /* Copy the subpage descriptor from PROPERTIES.ADDR_TRANS.  */
    {
      if (CAP_ADDR_TRANS_SUBPAGES (properties.addr_trans) != 1
	  && (source.type != cap_cappage
	      && source.type != cap_rcappage))
	/* A subpage descriptor is only valid for
	   cappages.  */
	{
	  debug (1, "subpages (%d) specified for non-cappage "
		 "cap " CAP_FMT,
		 CAP_ADDR_TRANS_SUBPAGES (properties.addr_trans),
		 CAP_PRINTF (&source));
	  return false;
	}

      if (!
	  (/* Start of PROPERTIES.ADDR_TRANS must be at or after start of
	      SOURCE.  */
	   subpage * (256 / subpages)
	   <= (CAP_ADDR_TRANS_SUBPAGE (properties.addr_trans) *
	       (256 / CAP_ADDR_TRANS_SUBPAGES (properties.addr_trans)))
	   /* End of PROPERTIES.ADDR_TRANS must be before or at end of
	      SOURCE.  */
	   && (((CAP_ADDR_TRANS_SUBPAGE (properties.addr_trans) + 1) *
		(256 / CAP_ADDR_TRANS_SUBPAGES (properties.addr_trans)))
	       <= (subpage + 1) * (256 / subpages))))
	/* The subpage descriptor does not narrow the
	   rights.  */
	{
	  debug (1, "specified subpage (%d/%d) not a subset "
		 " of source " CAP_FMT,
		 CAP_ADDR_TRANS_SUBPAGE (properties.addr_trans),
		 CAP_ADDR_TRANS_SUBPAGES (properties.addr_trans),
		 CAP_PRINTF (&source));
	  return false;
	}

      subpage = CAP_ADDR_TRANS_SUBPAGE (properties.addr_trans);
      subpages = CAP_ADDR_TRANS_SUBPAGES (properties.addr_trans);
    }

  /* By default, we preserve the guard in TARGET.  */
  int guard = CAP_GUARD (target);
  int gbits = CAP_GUARD_BITS (target);

  if ((flags & CAP_COPY_COPY_ADDR_TRANS_GUARD))
    /* Copy guard from PROPERTIES.ADDR_TRANS.  */
    {
      guard = CAP_ADDR_TRANS_GUARD (properties.addr_trans);
      gbits = CAP_ADDR_TRANS_GUARD_BITS (properties.addr_trans);
    }
  else if ((flags & CAP_COPY_COPY_SOURCE_GUARD))
    /* Copy guard from SOURCE.  */
    {
      guard = CAP_GUARD (&source);
      gbits = CAP_GUARD_BITS (&source);
    }

#ifdef RM_INTERN
  /* Changing a capability can change how addresses are translated.
     In this case, we need to shoot down all cached translations.  */
  bool changes_translation = false;

  if (target->oid != source.oid)
    {
      debug (5, "OID mismatch, changes translation");
      changes_translation = true;
    }
  else if (target->version != source.version)
    {
      debug (5, "Version mismatch, changes translation");
      changes_translation = true;
    }

  if (subpage != CAP_SUBPAGE (target) || subpages != CAP_SUBPAGES (target))
    {
      debug (5, "Subpage specification differs %d/%d -> %d/%d.",
	     subpage, subpages, CAP_SUBPAGE (target), CAP_SUBPAGES (target));
      changes_translation = true;
    }

  if (guard != CAP_GUARD (target)
      || gbits != CAP_GUARD_BITS (target))
    {
      debug (5, "Guard changed invalidating translation "
	     "0x%x/%d -> %llx/%d",
	     guard, gbits, CAP_GUARD (target), CAP_GUARD_BITS (target));
      changes_translation = true;
    }

  if (target->type != source.type
      && ! ((flags & CAP_COPY_WEAKEN)
	    && cap_type_weaken (source.type) == target->type))
    {
      debug (5, "Type changed, invalidating translation");
      changes_translation = true;
    }

  if (changes_translation)
    {
      extern void cap_shootdown (struct activity *activity, struct cap *cap);

      debug (5, "Translation changed: " CAP_FMT " -> " CAP_FMT,
	     CAP_PRINTF (target), CAP_PRINTF (&source));

      cap_shootdown (activity, target);
    }
#endif

  if (! CAP_ADDR_TRANS_SET_GUARD_SUBPAGE (&properties.addr_trans,
					  guard, gbits,
					  subpage, subpages))
    return false;

#ifndef RM_INTERN
  assert (! ADDR_IS_VOID (target_addr));
  assert (! ADDR_IS_VOID (source_addr));

  error_t err = rm_cap_copy (activity, target_address_space, target_addr,
			     source_address_space, source_addr,
			     flags, properties);
  assert (err == 0);
#endif

  *target = source;
  target->addr_trans = properties.addr_trans;

  if ((flags & CAP_COPY_WEAKEN))
    target->type = cap_type_weaken (target->type);

  if ((flags & CAP_COPY_DISCARDABLE_SET))
    target->discardable = properties.policy.discardable;

  if ((flags & CAP_COPY_PRIORITY_SET))
    target->priority = properties.policy.priority;

  return true;
}

/* Copy the capability SOURCE to capability TARGET.  Preserves
   SOURCE's subpage specification and TARGET's guard.  Copies SOURCE's
   policy.  */
static inline bool
cap_copy (activity_t activity,
	  addr_t target_as, struct cap *target, addr_t target_addr,
	  addr_t source_as, struct cap source, addr_t source_addr)
{
  return cap_copy_x (activity, target_as, target, target_addr,
		     source_as, source, source_addr,
		     CAP_COPY_DISCARDABLE_SET | CAP_COPY_PRIORITY_SET,
		     CAP_PROPERTIES_GET (source));
}

#endif
