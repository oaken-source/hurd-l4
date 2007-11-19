# headers.m4 - Autoconf snippets to install links for header files.
# Copyright 2003, 2007 Free Software Foundation, Inc.
# Written by Marcus Brinkmann <marcus@gnu.org>.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_CONFIG_LINKS([include/hurd/stddef.h:hurd/stddef.h
		 include/hurd/types.h:hurd/types.h
		 include/hurd/startup.h:hurd/startup.h
		 include/hurd/addr.h:hurd/addr.h
		 include/hurd/addr-trans.h:hurd/addr-trans.h
		 include/hurd/cap.h:hurd/cap.h
		 include/hurd/folio.h:hurd/folio.h
		 include/hurd/rpc.h:hurd/rpc.h
		 include/hurd/exceptions.h:hurd/exceptions.h
		])
