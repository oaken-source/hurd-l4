/* Define the real-function versions of all inline functions defined
   in l4/thread.c.  */

/* Include all header files used by l4/thread.h which possibly define
   extern inline functions as well before setting
   _L4_EXTERN_INLINE.  */
#include <l4/vregs.h>
#include <l4/syscall.h>

#ifdef _L4_EXTERN_INLINE
#undef _L4_EXTERN_INLINE
#endif
#define _L4_EXTERN_INLINE

#include <l4/thread.h>
