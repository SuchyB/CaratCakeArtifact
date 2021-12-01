/* A splay-tree datatype.
   Copyright (C) 1998-2020 Free Software Foundation, Inc.
   Contributed by Mark Mitchell (mark@markmitchell.com).
   This file is part of the GNU Offloading and Multi Processing Library
   (libgomp).
   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.
   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.
   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* The splay tree code copied from include/splay-tree.h and adjusted,
   so that all the data lives directly in splay_tree_node_s structure
   and no extra allocations are needed.
   Files including this header should before including it add:
typedef struct splay_tree_node_s *splay_tree_node;
typedef struct splay_tree_s *splay_tree;
typedef struct splay_tree_key_s *splay_tree_key;
   define splay_tree_key_s structure, and define
   splay_compare inline function.
   Alternatively, they can define splay_tree_prefix macro before
   including this header and then all the above types, the
   splay_compare function and the splay_tree_{lookup,insert_remove}
   function will be prefixed by that prefix.  If splay_tree_prefix
   macro is defined, this header must be included twice: once where
   you need the header file definitions, and once where you need the
   .c implementation routines.  In the latter case, you must also
   define the macro splay_tree_c.  See the include of splay-tree.h in
   priority_queue.[hc] for an example.  */

/* For an easily readable description of splay-trees, see:
     Lewis, Harry R. and Denenberg, Larry.  Data Structures and Their
     Algorithms.  Harper-Collins, Inc.  1991.
   The major feature of splay trees is that all basic tree operations
   are amortized O(log n) time for a tree with n nodes.  */

#ifndef __MM_SPLAY_TREE_H__
#define __MM_SPLAY_TREE_H__

#include <aspace/region_tracking/node_struct.h>

// keys are single uint64_ts

// typedef struct mm_splay_tree_node_s *mm_splay_tree_node_t;
// typedef struct mm_splay_tree_s *mm_splay_tree_t;
// typedef struct mm_splay_tree_key_s *mm_splay_tree_key_t;

#define attribute_hidden __attribute__ ((visibility ("hidden")))

static inline int mm_splay_compare (uint64_t x, uint64_t y)
{
  if (x==y) {
    return 0;
  } else if (x<y) {
    return -1;
  } else {
    return 1;
  }
}

#ifdef mm_splay_tree_prefix
# define splay_tree_name_1(prefix, name) prefix ## _ ## name
# define splay_tree_name(prefix, name) splay_tree_name_1 (prefix, name)
# define splay_tree_node_s	\
    splay_tree_name (splay_tree_prefix, splay_tree_node_s)
# define splay_tree_s		\
    splay_tree_name (splay_tree_prefix, splay_tree_s)
# define splay_tree_key_s	\
    splay_tree_name (splay_tree_prefix, splay_tree_key_s)
# define splay_tree_node	\
    splay_tree_name (splay_tree_prefix, splay_tree_node)/
# define splay_tree		\
    splay_tree_name (splay_tree_prefix, splay_tree)
# define splay_tree_key		\
    splay_tree_name (splay_tree_prefix, splay_tree_key)
# define splay_compare		\
    splay_tree_name (splay_tree_prefix, splay_compare)
# define splay_tree_lookup	\
    splay_tree_name (splay_tree_prefix, splay_tree_lookup)
# define splay_tree_insert	\
    splay_tree_name (splay_tree_prefix, splay_tree_insert)
# define splay_tree_remove	\
    splay_tree_name (splay_tree_prefix, splay_tree_remove)
# define splay_tree_foreach	\
    splay_tree_name (splay_tree_prefix, splay_tree_foreach)
# define splay_tree_callback	\
    splay_tree_name (splay_tree_prefix, splay_tree_callback)
#endif

#ifndef splay_tree_c
/* Header file definitions and prototypes.  */

/* The nodes in the splay tree.  */
typedef struct mm_splay_tree_node {
  uint64_t key;
  /* The left and right children, respectively.  */
  struct mm_splay_tree_node * left;
  struct mm_splay_tree_node * right;
  nk_aspace_region_t region;
}mm_splay_tree_node_t;


/* The splay tree.  */
typedef struct mm_splay_tree {
  mm_struct_t super;
  mm_splay_tree_node_t * root;
} mm_splay_tree_t;



typedef void (*mm_splay_tree_callback_t) (uint64_t, void *);

int mm_splay_tree_init(mm_splay_tree_t * splay_tree);
mm_struct_t * mm_splay_tree_create();
void mm_splay_tree_show(mm_struct_t *self);
// extern mm_splay_tree_key_t* mm_splay_tree_lookup (splay_tree, splay_tree_key);
// extern void mm_splay_tree_insert (mm_splay_tree_t, mm_splay_tree_node_t);
// extern void mm_splay_tree_remove (mm_splay_tree_t, mm_splay_tree_key_t);
// extern void mm_splay_tree_foreach (mm_splay_tree_t, mm_splay_tree_callback_t, void *);
#else  /* splay_tree_c */
#  ifdef splay_tree_prefix
#    include "splay-tree.c"
#    undef splay_tree_name_1
#    undef splay_tree_name
#    undef splay_tree_node_s
#    undef splay_tree_s
#    undef splay_tree_key_s
#    undef splay_tree_node
#    undef splay_tree
#    undef splay_tree_key
#    undef splay_compare
#    undef splay_tree_lookup
#    undef splay_tree_insert
#    undef splay_tree_remove
#    undef splay_tree_foreach
#    undef splay_tree_callback
#    undef splay_tree_c
#  endif
#endif /* #ifndef splay_tree_c */

#ifdef splay_tree_prefix
#  undef splay_tree_prefix
#endif

#endif
