/* Reentrant versions of stat system call.  This implementation just
   calls the stat system call.  */

#include <reent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

int
_DEFUN (_stat_r, (ptr, file, pstat),
     struct _reent *ptr _AND
     _CONST char *file _AND
     struct stat *pstat)
{
  errno = EOPNOTSUPP;
  return -1;
}
