/* ruth.c - Test server.
   Copyright (C) 2007, 2008 Free Software Foundation, Inc.
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <hurd/thread.h>
#include <hurd/startup.h>
#include <hurd/cap.h>
#include <hurd/folio.h>
#include <hurd/rm.h>
#include <hurd/stddef.h>
#include <hurd/capalloc.h>
#include <hurd/as.h>
#include <hurd/storage.h>
#include <hurd/activity.h>
#include <hurd/futex.h>
#include <hurd/anonymous.h>

#include <bit-array.h>
#include <string.h>

#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <l4.h>

extern int output_debug;

static addr_t activity;

/* Initialized by the machine-specific startup-code.  */
extern struct hurd_startup_data *__hurd_startup_data;


/* The program name.  */
extern char *program_name;


/* The following functions are required by pthread.  */


int
main (int argc, char *argv[])
{
  output_debug = 3;

  printf ("%s " PACKAGE_VERSION "\n", program_name);
  printf ("Hello, here is Ruth, your friendly root server!\n");

  debug (2, "RM: %x.%x", l4_thread_no (__hurd_startup_data->rm),
	 l4_version (__hurd_startup_data->rm));

  activity = __hurd_startup_data->activity;

  {
    printf ("Checking shadow page tables... ");

    int visit (addr_t addr,
	       l4_word_t type, struct cap_properties properties,
	       bool writable,
	       void *cookie)
      {
	struct cap cap = as_cap_lookup (addr, -1, NULL);

	assert (type == cap.type);
	if (type == cap_cappage || type == cap_rcappage || type == cap_folio)
	  {
	    if (! cap.shadow)
	      as_dump_path (addr);
	    assertx (cap.shadow,
		     ADDR_FMT ", %s",
		     ADDR_PRINTF (addr), cap_type_string (type));
	  }
	else
	  {
	    if (cap.shadow)
	      as_dump_path (addr);
	    assertx (! cap.shadow, ADDR_FMT ": " CAP_FMT " (%p)",
		     ADDR_PRINTF (addr), CAP_PRINTF (&cap), cap.shadow);
	  }

	if (type == cap_folio)
	  return -1;

	return 0;
      }

    as_walk (visit, ~(1 << cap_void), NULL);
    printf ("ok.\n");
  }

  {
    printf ("Checking folio_object_alloc... ");


    addr_t folio = capalloc ();
    assert (! ADDR_IS_VOID (folio));
    error_t err = rm_folio_alloc (activity, folio, FOLIO_POLICY_DEFAULT);
    assert (! err);

    int i;
    for (i = -10; i < 129; i ++)
      {
	addr_t addr = capalloc ();
	if (ADDR_IS_VOID (addr))
	  panic ("capalloc");

	err = rm_folio_object_alloc (activity, folio, i, cap_page,
				     OBJECT_POLICY_DEFAULT, 0,
				     addr, ADDR_VOID);
	assert ((err == 0) == (0 <= i && i < FOLIO_OBJECTS));

	if (0 <= i && i < FOLIO_OBJECTS)
	  {
	    l4_word_t type;
	    struct cap_properties properties;
	    err = rm_cap_read (activity, ADDR_VOID, addr, &type, &properties);
	    assert (! err);
	    assert (type == cap_page);
	  }
	capfree (addr);
      }

    err = rm_folio_free (activity, folio);
    assert (! err);
    capfree (folio);

    printf ("ok.\n");
  }

  {
    printf ("Checking folio_alloc... ");

    /* We allocate a sub-tree and fill it with folios (specifically,
       2^(bits - 1) folios).  */
    int bits = 2;
    addr_t root = as_alloc (bits + FOLIO_OBJECTS_LOG2 + PAGESIZE_LOG2,
			    1, true);
    assert (! ADDR_IS_VOID (root));

    int i;
    for (i = 0; i < (1 << bits); i ++)
      {
	struct storage shadow_storage
	  = storage_alloc (activity, cap_page, STORAGE_EPHEMERAL,
			   OBJECT_POLICY_DEFAULT, ADDR_VOID);
	struct object *shadow = ADDR_TO_PTR (addr_extend (shadow_storage.addr,
							  0, PAGESIZE_LOG2));

	addr_t f = addr_extend (root, i, bits);
	as_ensure_use (f,
		       ({
			 slot->type = cap_folio;
			 cap_set_shadow (slot, shadow);
		       }));

	error_t err = rm_folio_alloc (activity, f, FOLIO_POLICY_DEFAULT);
	assert (! err);

	int j;
	for (j = 0; j <= i; j ++)
	  {
	    l4_word_t type;
	    struct cap_properties properties;

	    error_t err = rm_cap_read (activity, ADDR_VOID,
				       addr_extend (root, j, bits),
				       &type, &properties);
	    assert (! err);
	    assert (type == cap_folio);

	    struct cap cap = as_cap_lookup (f, -1, NULL);
	    assert (cap.type == cap_folio);
	  }
      }

    for (i = 0; i < (1 << bits); i ++)
      {
	addr_t f = addr_extend (root, i, bits);

	error_t err = rm_folio_free (activity, f);
	assert (! err);

	void *shadow = NULL;
	bool ret = as_slot_lookup_use
	  (f,
	   ({
	     assert (slot->type == cap_folio);
	     slot->type = cap_void;
				    
	     shadow = cap_get_shadow (slot);
	   }));
	assert (ret);

	assert (shadow);
	storage_free (addr_chop (PTR_TO_ADDR (shadow), PAGESIZE_LOG2), 1);
      }

    as_free (root, 1);

    printf ("ok.\n");
  }


  {
    printf ("Checking storage_alloc... ");

    const int n = 4 * FOLIO_OBJECTS;
    addr_t storage[n];

    int i;
    for (i = 0; i < n; i ++)
      {
	storage[i] = storage_alloc (activity, cap_page,
				    (i & 1) == 0
				    ? STORAGE_LONG_LIVED
				    : STORAGE_EPHEMERAL,
				    OBJECT_POLICY_DEFAULT,
				    ADDR_VOID).addr;
	assert (! ADDR_IS_VOID (storage[i]));
	int *p = (int *) ADDR_TO_PTR (addr_extend (storage[i],
						   0, PAGESIZE_LOG2));
	* (int *) p = i;

	int j;
	for (j = 0; j <= i; j ++)
	  assert (* (int *) (ADDR_TO_PTR (addr_extend (storage[j],
						       0, PAGESIZE_LOG2)))
		  == j);
      }

    for (i = 0; i < n; i ++)
      {
	storage_free (storage[i], true);
      }

    printf ("ok.\n");
  }

  {
    printf ("Checking mmap... ");

    const int pages = 16;
    void *buffer = mmap (0, pages * PAGESIZE, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert (buffer != MAP_FAILED);

    struct { int start; int count; } order[] = { { 1, 2 },
						 { 3, 3 },
						 { 10, 2 },
						 { 15, 1 } };
    bool unmapped[pages];
    memset (unmapped, 0, sizeof (unmapped));

    int j;
    for (j = 0; j < pages; j ++)
      *(int *) (buffer + j * PAGESIZE) = j;

    int i;
    for (i = 0; i < sizeof (order) / sizeof (order[0]); i ++)
      {
	assert (order[i].start + order[i].count <= pages);

	munmap (buffer + order[i].start * PAGESIZE, order[i].count * PAGESIZE);

	for (j = order[i].start; j < order[i].start + order[i].count; j ++)
	  unmapped[j] = true;

	for (j = 0; j < pages; j ++)
	  if (! unmapped[j])
	    assertx (*(int *) (buffer + j * PAGESIZE) == j,
		     "%d =? %d",
		     *(int *) (buffer + j * PAGESIZE), j);
      }

    munmap (buffer, pages * PAGESIZE);

    printf ("ok.\n");
  }

  {
    static volatile int done;
    char stack[0x1000];

    void start (void)
    {
      do_debug (4)
	as_dump ("thread");

      debug (4, "I'm running (%x.%x)!",
	     l4_thread_no (l4_myself ()),
	     l4_version (l4_myself ()));

      done = 1;
      do
	l4_yield ();
      while (1);
    }

    printf ("Checking thread creation... ");

    addr_t thread = capalloc ();
    debug (5, "thread: " ADDR_FMT, ADDR_PRINTF (thread));
    addr_t storage = storage_alloc (activity, cap_thread, STORAGE_LONG_LIVED,
				    OBJECT_POLICY_DEFAULT, thread).addr;

    struct hurd_thread_exregs_in in;

    in.aspace = ADDR (0, 0);
    in.aspace_cap_properties = CAP_PROPERTIES_DEFAULT;
    in.aspace_cap_properties_flags = CAP_COPY_COPY_SOURCE_GUARD;

    in.activity = activity;

    in.sp = (l4_word_t) ((void *) stack + sizeof (stack));
    in.ip = (l4_word_t) &start;

    struct hurd_thread_exregs_out out;

    rm_thread_exregs (activity, thread,
		      HURD_EXREGS_SET_ASPACE | HURD_EXREGS_SET_ACTIVITY
		      | HURD_EXREGS_SET_SP_IP | HURD_EXREGS_START
		      | HURD_EXREGS_ABORT_IPC,
		      in, &out);

    debug (5, "Waiting for thread");
    while (done == 0)
      l4_yield ();
    debug (5, "Thread done!");

    storage_free (storage, true);
    capfree (thread);

    printf ("ok.\n");
  }

  {
    printf ("Checking pthread library... ");

#undef N
#define N 4
    pthread_t threads[N];

    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define FACTOR 10
    static volatile int shared_resource;
    shared_resource = 0;

    void *start (void *arg)
    {
      uintptr_t i = (uintptr_t) arg;

      debug (5, "%d (%x.%x) started", (int) i,
	     l4_thread_no (l4_myself ()), l4_version (l4_myself ()));

      int c;
      for (c = 0; c < FACTOR; c ++)
	{
	  int w;
	  for (w = 0; w < 10; w ++)
	    l4_yield ();

	  pthread_mutex_lock (&mutex);

	  debug (5, "%d calling, count=%d", (int) i, shared_resource);

	  for (w = 0; w < 10; w ++)
	    l4_yield ();

	  shared_resource ++;

	  if (c == FACTOR - 1)
	    debug (5, "Exiting with shared_resource = %d", shared_resource);

	  pthread_mutex_unlock (&mutex);
	}

      return arg;
    }

    int i;
    for (i = 0; i < N; i ++)
      {
	debug (5, "Creating thread %d", i);
	error_t err = pthread_create (&threads[i], NULL, start,
				      (void *) (uintptr_t) i);
	assert (err == 0);
      }

    for (i = 0; i < N; i ++)
      {
	void *status = (void *) 1;
	debug (5, "Waiting on thread %d", i);
	error_t err = pthread_join (threads[i], &status);
	assert (err == 0);
	assert ((uintptr_t) status == (uintptr_t) i);
	debug (5, "Joined %d", i);
      }

    assert (shared_resource == N * FACTOR);

    printf ("ok.\n");
  }

  {
    printf ("Checking signals... ");

    pthread_t thread;

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    const int count = 18;

    void *start (void *arg)
    {
      volatile int i;

      bool altstack = false;

      void handler (int signo, siginfo_t *info, void *context)
      {
	debug (5, "In handler for sig %d", signo);
	i = 1;

	assert (signo == info->si_signo);

	stack_t stack;
	if (sigaltstack (NULL, &stack) < 0)
	  perror ("sigaltstack");

	if (altstack)
	  {
	    assert ((stack.ss_flags & SS_ONSTACK));
	    assert ((void *) stack.ss_sp <= (void *) &signo);
	    assert ((void *) &signo < (void *) stack.ss_sp + stack.ss_size);
	  }
	else
	  assert (! (stack.ss_flags & SS_ONSTACK));
      }

      int j;
      for (j = 0; j < count; j ++)
	{
	  struct sigaction act;
	  act.sa_sigaction = handler;
	  sigemptyset (&act.sa_mask);
	  act.sa_flags = SA_SIGINFO | SA_ONSTACK;

	  if (sigaction (SIGUSR1, &act, NULL) < 0)
	    panic ("Failed to install signal handler: %s", strerror (errno));

	  debug (5, "Installed signal handler, waking main thread");

	  /* Wait until the main thread unlocks MUTEX.  */
	  pthread_mutex_lock (&mutex);
	  pthread_mutex_unlock (&mutex);

	  debug (5, "Signaling main thread");

	  /* Signal the main thread that we are ready.  */
	  pthread_cond_signal (&cond);

	  /* Block.  */
	  while (i != 1)
	    l4_yield ();

	  i = 0;

	  if (j == 2)
	    /* Use an alternate stack.  */
	    {
	      altstack = true;

	      stack_t stack;
	      stack.ss_sp = malloc (SIGSTKSZ);
	      stack.ss_size = SIGSTKSZ;
	      stack.ss_flags = 0;
	      if (sigaltstack (&stack, NULL) < 0)
		perror ("sigaltstack");
	    }
	  else if (j % 6 == 3)
	    /* Disable the stack.  */
	    {
	      stack_t stack;
	      if (sigaltstack (NULL, &stack) < 0)
		perror ("sigaltstack");

	      assertx (stack.ss_flags == 0,
		       "%x", stack.ss_flags);
	      stack.ss_flags |= SS_DISABLE;

	      if (sigaltstack (&stack, NULL) < 0)
		perror ("sigaltstack");

	      altstack = false;
	    }
	  else if (j % 6 == 5)
	    /* Enable the stack.  */
	    {
	      stack_t stack;
	      if (sigaltstack (NULL, &stack) < 0)
		perror ("sigaltstack");

	      assert (stack.ss_flags == SS_DISABLE);
	      stack.ss_flags &= ~SS_DISABLE;

	      if (sigaltstack (&stack, NULL) < 0)
		perror ("sigaltstack");

	      altstack = true;
	    }
	}

      /* For good measure, we send ourself a few signals.  */
      raise (SIGUSR1);
      assert (i == 1);
      i = 0;

      for (j = 0; j < 10; j ++)
	{
	  stack_t stack;

	  /* With the alternate stack.  */
	  altstack = true;

	  if (sigaltstack (NULL, &stack) < 0)
	    perror ("sigaltstack");
	  stack.ss_flags &= ~SS_DISABLE;
	  assert (stack.ss_sp);
	  if (sigaltstack (&stack, NULL) < 0)
	    perror ("sigaltstack");

	  if (j % 2 == 0)
	    raise (SIGUSR1);
	  else
	    pthread_kill (pthread_self (), SIGUSR1);
	  assert (i == 1);
	  i = 0;

	  /* And without the alternate stack.  */
	  altstack = false;

	  if (sigaltstack (NULL, &stack) < 0)
	    perror ("sigaltstack");
	  stack.ss_flags |= SS_DISABLE;
	  if (sigaltstack (&stack, NULL) < 0)
	    perror ("sigaltstack");

	  if (j % 2 == 0)
	    raise (SIGUSR1);
	  else
	    pthread_kill (pthread_self (), SIGUSR1);
	  assert (i == 1);
	  i = 0;
	}

      /* Block the signal.  */
      sigset_t mask;
      sigemptyset (&mask);
      sigaddset (&mask, SIGUSR1);
      pthread_sigmask (SIG_BLOCK, &mask, NULL);

      raise (SIGUSR1);
      assert (i == 0);

      /* Deallocate the stack (it is disabled).  */
      stack_t stack;
      if (sigaltstack (NULL, &stack) < 0)
	perror ("sigaltstack");
      free (stack.ss_sp);
      assert ((stack.ss_flags & SS_DISABLE));

      return 0;
    }

    pthread_mutex_lock (&mutex);

    error_t err = pthread_create (&thread, NULL, start, 0);
    assert (err == 0);

    int i;
    for (i = 0; i < count; i ++)
      {
	/* Wait for the thread to install the signal handler.  */
	pthread_cond_wait (&cond, &mutex);
	pthread_mutex_unlock (&mutex);

	err = pthread_kill (thread, SIGUSR1);
	if (err)
	  panic ("Failed to signal thread %d: %s", thread, strerror (err));

	pthread_mutex_lock (&mutex);
      }

    void *status;
    err = pthread_join (thread, &status);
    assert (err == 0);

    printf ("ok.\n");
  }

  {
    printf ("Checking activity creation... ");

#undef N
#define N 10
    void test (addr_t activity, addr_t folio, int depth)
    {
      error_t err;
      int i;
      int obj = 0;

      struct
      {
	addr_t child;
	addr_t folio;
	addr_t page;
      } a[N];

      for (i = 0; i < N; i ++)
	{
	  /* Allocate a new activity.  */
	  a[i].child = capalloc ();
	  err = rm_folio_object_alloc (activity, folio, obj ++,
				       cap_activity_control,
				       OBJECT_POLICY_DEFAULT, 0,
				       a[i].child, ADDR_VOID);
	  assert (err == 0);

	  /* Allocate a folio against the activity and use it.  */
	  a[i].folio = capalloc ();
	  err = rm_folio_alloc (a[i].child, a[i].folio, FOLIO_POLICY_DEFAULT);
	  assert (err == 0);

	  a[i].page = capalloc ();
	  err = rm_folio_object_alloc (a[i].child, a[i].folio, 0, cap_page,
				       OBJECT_POLICY_DEFAULT, 0,
				       a[i].page, ADDR_VOID);
	  assert (err == 0);

	  l4_word_t type;
	  struct cap_properties properties;

	  err = rm_cap_read (a[i].child, ADDR_VOID,
			     a[i].page, &type, &properties);
	  assert (err == 0);
	  assert (type == cap_page);
	}

      if (depth > 0)
	/* Create another hierarchy depth.  */
	for (i = 0; i < 2; i ++)
	  test (a[i].child, a[i].folio, depth - 1);

      /* We destroy the first N / 2 activities.  The caller will
	 destroy the rest.  */
      for (i = 0; i < N / 2; i ++)
	{
	  /* Destroy the activity.  */
	  rm_folio_free (activity, a[i].folio);

	  /* To determine if the folio has been destroyed, we cannot simply
	     read the capability: this returns the type stored in the
	     capability, not the type of the designated object.  Destroying
	     the object does not destroy the capability.  Instead, we try to
	     use the object.  If this fails, we assume that the folio was
	     destroyed.  */
	  err = rm_folio_object_alloc (a[i].child, a[i].folio, 1, cap_page,
				       OBJECT_POLICY_DEFAULT, 0,
				       a[i].page, ADDR_VOID);
	  assert (err);

	  capfree (a[i].page);
	  capfree (a[i].folio);
	  capfree (a[i].child);
	}
    }

    error_t err;
    addr_t folio = capalloc ();
    err = rm_folio_alloc (activity, folio, FOLIO_POLICY_DEFAULT);
    assert (err == 0);

    test (activity, folio, 2);

    err = rm_folio_free (activity, folio);
    assert (err == 0);

    capfree (folio);

    printf ("ok.\n");
  }

  {
    printf ("Checking activity_policy... ");

    addr_t a = capalloc ();
    addr_t storage = storage_alloc (activity, cap_activity_control,
				    STORAGE_LONG_LIVED, OBJECT_POLICY_DEFAULT,
				    a).addr;

    addr_t weak = capalloc ();
    error_t err = rm_cap_copy (activity, ADDR_VOID, weak, ADDR_VOID, a,
			       CAP_COPY_WEAKEN, CAP_PROPERTIES_VOID);
    assert (! err);

    struct activity_policy in, out;
    in.sibling_rel.priority = 2;
    in.sibling_rel.weight = 3;
    in.child_rel = ACTIVITY_MEMORY_POLICY_VOID;
    in.folios = 10000;

    err = rm_activity_policy (a,
			      ACTIVITY_POLICY_SIBLING_REL_SET
			      | ACTIVITY_POLICY_STORAGE_SET,
			      in,
			      &out);
    assert (err == 0);
			    
    err = rm_activity_policy (a,
			      0, ACTIVITY_POLICY_VOID,
			      &out);
    assert (err == 0);

    assert (out.sibling_rel.priority == 2);
    assert (out.sibling_rel.weight == 3);
    assert (out.folios == 10000);

    in.sibling_rel.priority = 4;
    in.sibling_rel.weight = 5;
    in.folios = 10001;
    err = rm_activity_policy (a,
			      ACTIVITY_POLICY_SIBLING_REL_SET
			      | ACTIVITY_POLICY_STORAGE_SET,
			      in, &out);
    assert (err == 0);

    /* We expect the old values.  */
    assert (out.sibling_rel.priority == 2);
    assert (out.sibling_rel.weight == 3);
    assert (out.folios == 10000);

    err = rm_activity_policy (weak,
			      ACTIVITY_POLICY_SIBLING_REL_SET
			      | ACTIVITY_POLICY_STORAGE_SET,
			      in, &out);
    assert (err == EPERM);

    err = rm_activity_policy (weak, 0, in, &out);
    assert (err == 0);

    assert (out.sibling_rel.priority == 4);
    assert (out.sibling_rel.weight == 5);
    assert (out.folios == 10001);

    storage_free (storage, true);

    capfree (a);
    capfree (weak);

    printf ("ok.\n");
  }

  {
    printf ("Checking futex implementation... ");

#undef N
#define N 4
#undef FACTOR
#define FACTOR 10
    pthread_t threads[N];

    int futex1 = 0;
    int futex2 = 1;

    void *start (void *arg)
    {
      int i;

      for (i = 0; i < FACTOR; i ++)
	{
	  long ret = futex_wait (&futex1, 0);
	  assert (ret == 0);

	  ret = futex_wait (&futex2, 1);
	  assert (ret == 0);
	}

      return arg;
    }

    int i;
    for (i = 0; i < N; i ++)
      {
	debug (5, "Creating thread %d", i);
	error_t err = pthread_create (&threads[i], NULL, start,
				      (void *) (uintptr_t) i);
	assert (err == 0);
      }

    for (i = 0; i < FACTOR; i ++)
      {
	int count = 0;
	while (count < N)
	  {
	    long ret = futex_wake (&futex1, 1);
	    count += ret;

	    ret = futex_wake (&futex1, N);
	    count += ret;
	  }
	assert (count == N);

	count = 0;
	while (count < N)
	  {
	    long ret = futex_wake (&futex2, 1);
	    count += ret;

	    ret = futex_wake (&futex2, N);
	    count += ret;
	  }
	assert (count == N);
      }

    for (i = 0; i < N; i ++)
      {
	void *status = (void *) 1;
	debug (5, "Waiting on thread %d", i);
	error_t err = pthread_join (threads[i], &status);
	assert (err == 0);
	assert ((uintptr_t) status == (uintptr_t) i);
	debug (5, "Joined %d", i);
      }

    printf ("ok.\n");
  }

  {
    printf ("Checking thread_wait_object_destroy... ");

    struct storage storage = storage_alloc (activity, cap_page,
					    STORAGE_MEDIUM_LIVED,
					    OBJECT_POLICY_DEFAULT,
					    ADDR_VOID);
    assert (! ADDR_IS_VOID (storage.addr));

    void *start (void *arg)
    {
      uintptr_t ret = 0;
      error_t err;
      err = rm_thread_wait_object_destroyed (ADDR_VOID, storage.addr, &ret);
      debug (5, "object destroy returned: err: %d, ret: %d", err, ret);
      assert (err == 0);
      assert (ret == 10);
      return 0;
    }

    pthread_t tid;
    error_t err = pthread_create (&tid, NULL, start, 0);
    assert (err == 0);

    int i;
    for (i = 0; i < 100; i ++)
      l4_yield ();

    /* Deallocate the object.  */
    debug (5, "Destroying object");
    rm_folio_object_alloc (ADDR_VOID,
			   addr_chop (storage.addr, FOLIO_OBJECTS_LOG2),
			   addr_extract (storage.addr, FOLIO_OBJECTS_LOG2),
			   cap_void,
			   OBJECT_POLICY_VOID, 10, ADDR_VOID, ADDR_VOID);
    /* Release the memory.  */
    storage_free (storage.addr, true);

    void *status;
    err = pthread_join (tid, &status);
    assert (err == 0);
    debug (5, "Joined thread");

    printf ("ok.\n");
  }

  {
    printf ("Checking rendered regions... ");

    const int s = 4 * PAGESIZE;

    bool fill (struct anonymous_pager *anon,
	       uintptr_t offset, uintptr_t count,
	       void *pages[],
	       struct exception_info info)
    {
      assert (count == 1);

      int *p = pages[0];
      int i;
      for (i = 0; i < PAGESIZE / sizeof (int); i ++)
	p[i] = offset / sizeof (int) + i;

      return true;
    }

    void *addr;
    struct anonymous_pager *pager
      = anonymous_pager_alloc (ADDR_VOID, NULL, s, MAP_ACCESS_ALL,
			       OBJECT_POLICY_DEFAULT, 0,
			       fill, &addr);
    assert (pager);

    int *p = addr;
    int i;
    for (i = 0; i < s / sizeof (int); i ++)
      assert (p[i] == i);

    printf ("ok\n");
  }

  {
    printf ("Checking discardability... ");

    bool fill (struct anonymous_pager *anon,
	       uintptr_t offset, uintptr_t count,
	       void *pages[],
	       struct exception_info info)
    {
      assert (count == 1);

      if (! info.discarded)
	return true;

      * (int *) pages[0] = offset / PAGESIZE;

      return true;
    }

    uint32_t frames;
    do
      {
	struct activity_info info;
	error_t err = rm_activity_info (ADDR_VOID, activity_info_stats, 1,
					&info);
	assert_perror (err);
	assert (info.stats.count >= 1);

	frames = info.stats.stats[0].available;
      }
    while (frames == 0);

    debug (0, "%d frames available", frames);
    uint32_t goal = frames * 2;
    /* Limit to at most 1GB of memory.  */
    if (goal > ((uint32_t) -1) / PAGESIZE / 4)
      goal = ((uint32_t) -1) / PAGESIZE / 4;
    debug (0, "Allocating %d frames", goal);

    void *addr;
    struct anonymous_pager *pager
      = anonymous_pager_alloc (ADDR_VOID, NULL, goal * PAGESIZE, MAP_ACCESS_ALL,
			       OBJECT_POLICY (true, OBJECT_PRIORITY_LRU), 0,
			       fill, &addr);
    assert (pager);

    void *p;
    int i = 0;
    for (p = addr; p < addr + goal * PAGESIZE; p += PAGESIZE, i ++)
      * (int *) p = i;
    assert (i == goal);
    
    debug (0, "Verifying the content of the discardable pages");

    for (p = addr, i = 0; p < addr + goal * PAGESIZE; p += PAGESIZE, i ++)
      assertx (* (int *) p == i, "%d != %d", * (int *) p, i);

    anonymous_pager_destroy (pager);

    printf ("ok.\n");
  }

  {
    printf ("Checking read-only pages... ");
 
    addr_t addr = as_alloc (PAGESIZE_LOG2, 1, true);
    assert (! ADDR_IS_VOID (addr));

    as_ensure (addr);

    addr_t storage = storage_alloc (activity, cap_page,
				    STORAGE_MEDIUM_LIVED,
				    OBJECT_POLICY_DEFAULT,
				    addr).addr;
    assert (! ADDR_IS_VOID (storage));


    debug (1, "Writing before dealloc...");
    int *buffer = ADDR_TO_PTR (addr_extend (addr, 0, PAGESIZE_LOG2));
    *buffer = 0;

    storage_free (storage, true);

    debug (1, "Writing after dealloc (should sigsegv)...");
    *buffer = 0;
  }

  debug (1, "Shutting down...");
  while (1)
    l4_sleep (L4_NEVER);

  return 0;
}
