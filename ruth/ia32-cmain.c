/* ia32-cmain.c - Startup code for the ia32.
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
   Written by Marcus Brinkmann.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <alloca.h>
#include <stdint.h>

#include <l4/globals.h>
#include <l4/init.h>
#include <l4/stubs.h>
#include <l4/stubs-init.h>

#include "ruth.h"

#include <hurd/startup.h>
#include <hurd/mm.h>


/* Initialized by the machine-specific startup-code.  */
extern struct hurd_startup_data *__hurd_startup_data;


static void
finish (void)
{
  int argc = 0;
  char **argv = 0;

  char *str = __hurd_startup_data->argz;
  if (str)
    /* A command line was passed.  */
    {
      int nr = 0;

      /* First time around we count the number of arguments.  */
      argc = 1;
      while (*str && *str == ' ')
	str++;

      while (*str)
	if (*(str++) == ' ')
	  {
	    while (*str && *str == ' ')
	      str++;
	    if (*str)
	      argc++;
	  }
      argv = alloca (sizeof (char *) * (argc + 1));

      /* Second time around we fill in the argv.  */
      str = (char *) __hurd_startup_data->argz;

      while (*str && *str == ' ')
	str++;
      argv[nr++] = str;

      while (*str)
	{
	  if (*str == ' ')
	    {
	      *(str++) = '\0';
	      while (*str && *str == ' ')
		str++;
	      if (*str)
		argv[nr++] = str;
	    }
	  else
	    str++;
	}
      argv[nr] = 0;
    }
  else
    {
      argc = 1;

      argv = alloca (sizeof (char *) * 2);
      argv[0] = program_name;
      argv[1] = 0;
    }

  /* Now invoke the main function.  */
  exit (main (argc, argv));
}

/* Initialize libl4, setup the argument vector, and pass control over
   to the main function.  */
void
cmain (void)
{
  l4_init ();
  l4_init_stubs ();

  printf ("In cmain\n");

  mm_init (__hurd_startup_data->activity);

  extern void *(*_pthread_init_routine)(void);
  if (_pthread_init_routine)
    {
      void *sp = _pthread_init_routine ();

      /* Switch stacks.  */
      l4_start_sp_ip (l4_myself (), (l4_word_t) sp, (l4_word_t) &finish);
    }
  else
    finish ();

  /* Never reached.  */
}


#define __thread_stack_pointer() ({					      \
  void *__sp__;								      \
  __asm__ ("movl %%esp, %0" : "=r" (__sp__));				      \
  __sp__;								      \
})


#define __thread_set_stack_pointer(sp) ({				      \
  __asm__ ("movl %0, %%esp" : : "r" (sp));				      \
})


/* Switch execution transparently to thread TO.  The thread FROM,
   which must be the current thread, will be halted.  */
void
switch_thread (l4_thread_id_t from, l4_thread_id_t to)
{
  void *current_stack;
  /* FIXME: Figure out how much we need.  Probably only one return
     address.  */
  char small_sub_stack[16];
  unsigned int i;

/* FIXME: FROM is an argument to force gcc to evaluate it before the
   thread switch.  Maybe this can be done better, but it's
   magical, so be careful.  */

  /* Save the machine context.  */
  __asm__ __volatile__ ("pusha");
  __asm__ __volatile__ ("pushf");

  /* Start the TO thread.  It will be eventually become a clone of our
     thread.  */
  current_stack = __thread_stack_pointer ();
  l4_start_sp_ip (to, (l4_word_t) current_stack,
		  (l4_word_t) &&thread_switch_entry);
  
  /* We need a bit of extra space on the stack for
     l4_thread_switch.  */
  __thread_set_stack_pointer (small_sub_stack + sizeof (small_sub_stack));

  /* We can't use while(1), because then gcc will become clever and
     optimize away everything after thread_switch_entry.  */
  for (i = 1; i; i++)
    l4_thread_switch (to);

 thread_switch_entry:
  /* Restore the machine context.  */
  __asm__ __volatile__ ("popf");
  __asm__ __volatile__ ("popa");

  /* The thread TO continues here.  */
  l4_stop (from);
}
