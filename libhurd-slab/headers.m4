# headers.m4 - Autoconf snippets to install links for header files.
# Copyright 2003 Free Software Foundation, Inc.
# Written by Marcus Brinkmann <marcus@gnu.org>.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_CONFIG_LINKS([sysroot/include/hurd/slab.h:libhurd-slab/slab.h])

AC_CONFIG_COMMANDS_POST([
  mkdir -p sysroot/lib libhurd-slab &&
  ln -sf ../../libhurd-slab/libhurd-slab.a sysroot/lib/ &&
  touch libhurd-slab/libhurd-slab.a
])
