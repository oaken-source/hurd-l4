/* l4/gnu/message.h - Public GNU interface to the L4 message registers.
   Copyright (C) 2004 Free Software Foundation, Inc.
   Written by Marcus Brinkmann <marcus@gnu.org>.

   This file is part of the GNU L4 library.
 
   The GNU L4 library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.
 
   The GNU L4 library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.
 
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU L4 library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#ifndef _L4_MESSAGE_H
# error "Never use <l4/gnu/message.h> directly; include <l4/message.h> instead."
#endif

/* The number of buffer registers available.  */
#define L4_NUM_BRS	_L4_NUM_BRS

/* The number of message registers available.  */
#define L4_NUM_MRS	_L4_NUM_MRS


static inline void
_L4_attribute_always_inline
l4_store_mr (int i, l4_word_t *w)
{
  _L4_store_mr (i, w);
}


static inline void
_L4_attribute_always_inline
l4_load_mr (int i, l4_word_t w)
{
  _L4_load_mr (i, w);
}


static inline void
_L4_attribute_always_inline
l4_store_mrs (int i, int k, l4_word_t *w)
{
  _L4_store_mrs (i, k, w);
}


static inline void
_L4_attribute_always_inline
l4_load_mrs (int i, int k, l4_word_t *w)
{
  _L4_load_mrs (i, k, w);
}


static inline void
_L4_attribute_always_inline
l4_store_br (int i, l4_word_t *w)
{
  _L4_store_br (i, w);
}


static inline void
_L4_attribute_always_inline
l4_load_br (int i, l4_word_t w)
{
  _L4_load_br (i, w);
}


static inline void
_L4_attribute_always_inline
l4_store_brs (int i, int k, l4_word_t *w)
{
  _L4_store_brs (i, k, w);
}


static inline void
_L4_attribute_always_inline
l4_load_brs (int i, int k, l4_word_t *w)
{
  _L4_load_brs (i, k, w);
}
