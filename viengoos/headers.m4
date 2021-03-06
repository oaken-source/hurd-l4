# headers.m4 - Autoconf snippets to install links for header files.
# Copyright 2007 Free Software Foundation, Inc.
# Written by Neal H. Walfield <neal@gnu.org>.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_CONFIG_LINKS([sysroot/include/hurd/rm.h:viengoos/rm.h])

AC_CONFIG_COMMANDS_POST([
  mkdir -p sysroot/lib viengoos &&
  ln -sf ../../viengoos/libhurd-cap.a sysroot/lib/ &&
  touch viengoos/libhurd-cap.a
])
