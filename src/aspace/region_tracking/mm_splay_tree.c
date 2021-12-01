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
   and no extra allocations are needed.  */

/* For an easily readable description of splay-trees, see:
     Lewis, Harry R. and Denenberg, Larry.  Data Structures and Their
     Algorithms.  Harper-Collins, Inc.  1991.
   The major feature of splay trees is that all basic tree operations
   are amortized O(log n) time for a tree with n nodes.  */

#include <nautilus/nautilus.h>
#include <aspace/region_tracking/mm_splay_tree.h>

#ifndef NAUT_CONFIG_DEBUG_ASPACE_REGION_TRACKING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR_SP(fmt, args...) ERROR_PRINT("aspace-splay-tree: " fmt, ##args)
#define DEBUG_SP(fmt, args...) DEBUG_PRINT("aspace-splay-tree: " fmt, ##args)
#define INFO_SP(fmt, args...)   INFO_PRINT("aspace-splay-tree: " fmt, ##args)
#define MALLOC_SP(n) ({void *__p = malloc(n); if (!__p) { ERROR_SP("Malloc failed\n"); panic("Malloc failed\n"); } __p;})

/* Rotate the edge joining the left child N with its parent P.  PP is the
   grandparents' pointer to P.  */

int mm_overlap_helper(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB){
    void * VA_start_A = regionA->va_start;
    void * VA_start_B = regionB->va_start;
    void * VA_end_A = regionA->va_start + regionA->len_bytes;
    void * VA_end_B = regionB->va_start + regionB->len_bytes;

    if (VA_start_A <= VA_start_B && VA_start_B < VA_end_A) {
        return 1;
    }
    if (VA_start_B <= VA_start_A && VA_start_A < VA_end_B) {
        return 1;
    }

    return 0;
}

static inline void
rotate_left (mm_splay_tree_node_t **pp, mm_splay_tree_node_t *p, mm_splay_tree_node_t *n)
{
  mm_splay_tree_node_t * tmp;
  tmp = n->right;
  n->right = p;
  p->left = tmp;
  *pp = n;
}

/* Rotate the edge joining the right child N with its parent P.  PP is the
   grandparents' pointer to P.  */

static inline void
rotate_right (mm_splay_tree_node_t **pp, mm_splay_tree_node_t *p, mm_splay_tree_node_t *n)
{
  mm_splay_tree_node_t * tmp;
  tmp = n->left;
  n->left = p;
  p->right = tmp;
  *pp = n;
}

/* Bottom up splay of KEY.  */

static void
splay_tree_splay (mm_splay_tree_t *sp, uint64_t key)
{
  if (sp->root == NULL)
    return;

  do {
    int cmp1, cmp2;
    mm_splay_tree_node_t *n, *c;

    n = sp->root;
    cmp1 = mm_splay_compare (key, n->key);

    /* Found.  */
    if (cmp1 == 0)
      return;

    /* Left or right?  If no child, then we're done.  */
    if (cmp1 < 0)
      c = n->left;
    else
      c = n->right;
    if (!c)
      return;

    /* Next one left or right?  If found or no child, we're done
       after one rotation.  */
    cmp2 = mm_splay_compare (key, c->key);
    if (cmp2 == 0
	|| (cmp2 < 0 && !c->left)
	|| (cmp2 > 0 && !c->right))
      {
	if (cmp1 < 0)
	  rotate_left (&sp->root, n, c);
	else
	  rotate_right (&sp->root, n, c);
	return;
      }

    /* Now we have the four cases of double-rotation.  */
    if (cmp1 < 0 && cmp2 < 0)
      {
	rotate_left (&n->left, c, c->left);
	rotate_left (&sp->root, n, n->left);
      }
    else if (cmp1 > 0 && cmp2 > 0)
      {
	rotate_right (&n->right, c, c->right);
	rotate_right (&sp->root, n, n->right);
      }
    else if (cmp1 < 0 && cmp2 > 0)
      {
	rotate_right (&n->left, c, c->right);
	rotate_left (&sp->root, n, n->left);
      }
    else if (cmp1 > 0 && cmp2 < 0)
      {
	rotate_left (&n->right, c, c->left);
	rotate_right (&sp->root, n, n->right);
      }
  } while (1);
}

/* Insert a new NODE into SP.  The NODE shouldn't exist in the tree.  */

int
mm_splay_tree_insert (mm_struct_t * self, nk_aspace_region_t * region)
{

  mm_splay_tree_t * sp = (mm_splay_tree_t *) self;
  mm_splay_tree_node_t * node = (mm_splay_tree_node_t *) malloc(sizeof(mm_splay_tree_node_t));
    
    if (! node) {
        ERROR_PRINT("cannot allocate a node for splay tree data structure to track region mapping\n");
        return -1;
    }

  node->region = *region;
  node->key = (uint64_t) region->va_start;

  int comparison = 0;
  
  splay_tree_splay (sp, node->key);  

  if (sp->root)
    comparison = mm_splay_compare (sp->root->key, node->key);

  if (sp->root && comparison == 0) {
    DEBUG_PRINT("Duplicate node\n");
    return -2;
  } else
    {
      /* Insert it at the root.  */
      if (sp->root == NULL)
	node->left = node->right = NULL;
      else if (comparison < 0)
	{
	  node->left = sp->root;
	  node->right = node->left->right;
	  node->left->right = NULL;
	}
      else
	{
	  node->right = sp->root;
	  node->left = node->right->left;
	  node->right->left = NULL;
	}
      sp->root = node;
    }

  sp->super.size++;

  return 0;
}

/* Remove node with KEY from SP.  It is not an error if it did not exist.  */

int
mm_splay_tree_remove (mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags)
{
  if(!(check_flags & VA_CHECK)){
    ERROR_SP("splay tree expect to remove regions with VA_check flag set!\n");
    return -1;
  }
  mm_splay_tree_t * sp = (mm_splay_tree_t *) self;
  uint64_t key = (uint64_t) region->va_start;

  splay_tree_splay (sp, key);

  if (sp->root && mm_splay_compare (sp->root->key, key) == 0)
    {
      mm_splay_tree_node_t * left, * right;
      mm_splay_tree_node_t * delete_node = sp->root;

      left = sp->root->left;
      right = sp->root->right;



      /* One of the children is now the root.  Doesn't matter much
	 which, so long as we preserve the properties of the tree.  */
      if (left)
	{
	  sp->root = left;

	  /* If there was a right child as well, hang it off the
	     right-most leaf of the left child.  */
	  if (right)
	    {
	      while (left->right)
		left = left->right;
	      left->right = right;
	    }
	}
      else
	sp->root = right;

  free(delete_node);
  // DEBUG_SP("The region has been successfully removed\");

  sp->super.size--;

  return 0;
    }
  else{
    // DEBUG_SP("No such a region found, failed to remove the region\n");
    return -1;
  }
}

/* Lookup KEY in SP, returning NODE if present, and NULL
   otherwise.  */

// attribute_hidden nk_aspace_region_t*
// mm_splay_tree_contains (mm_splay_tree_t * sp, uint64_t key)
// {
//   mm_splay_tree_splay (sp, key);

//   if (sp->root && mm_mm_splay_compare (sp->root->key, key) == 0)
//     return  &(sp->root->region);
//   else
//     return NULL;
// }


void mm_show_helper(mm_splay_tree_node_t * node){
  if(node){
    mm_show_helper(node->left);
    DEBUG_SP("VA = %016lx to PA = %016lx, len = %16lx\n", 
            node->region.va_start,
            node->region.pa_start,
            node->region.len_bytes
        );
    mm_show_helper(node->right);
  }
}

void mm_splay_tree_show(mm_struct_t * self){
  mm_splay_tree_t * sp = (mm_splay_tree_t * ) self; 
  mm_show_helper(sp->root);
}

nk_aspace_region_t* mm_splay_tree_contains(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags){

    mm_splay_tree_t * sp = (mm_splay_tree_t *) self;
    uint64_t key =  (uint64_t) region->va_start;

    splay_tree_splay (sp, key);
    if (sp->root && region_equal(&sp->root->region, region, check_flags) && mm_splay_compare (sp->root->key, key) == 0){
      return &sp->root->region;
    }
    return NULL;
}

nk_aspace_region_t * mm_splay_tree_check_overlap(mm_struct_t * self, nk_aspace_region_t * region){
    mm_splay_tree_t * sp = (mm_splay_tree_t * ) self;
    mm_splay_tree_node_t * curr_node = sp->root;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;
        uint64_t key = (uint64_t) region->va_start;

        // if the region overlaps (either key matches, or region interval matches)
        if(key==curr_node->key || mm_overlap_helper(region,&curr_node->region)){ 
          return curr_region_ptr;
        }

        //decide which subtree to step in
        else{
          if(mm_splay_compare(curr_node->key,key) < 0){
            curr_node = curr_node->right;
          }
          else{
            curr_node = curr_node->left;
          }
        }
    }

    return NULL;
}

nk_aspace_region_t * mm_splay_tree_update_region(
    mm_struct_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
){
  nk_aspace_region_t * cur_ptr = mm_splay_tree_contains(self,cur_region,eq_flag);
  if(cur_ptr && region_equal(cur_ptr, cur_region, eq_flag)){
      region_update(cur_ptr, new_region, eq_flag);
      return cur_ptr;
  }
  else{
    return NULL;
  }
}

nk_aspace_region_t * mm_splay_tree_find_reg_at_addr (mm_struct_t * self, addr_t address) {
    mm_splay_tree_t * sp = (mm_splay_tree_t * ) self;
    mm_splay_tree_node_t * curr_node = sp->root;

    while (curr_node != NULL) {
        nk_aspace_region_t curr_reg = curr_node->region;
        if ( 
            (addr_t) curr_reg.va_start <= address && 
            address < (addr_t) curr_reg.va_start + (addr_t) curr_reg.len_bytes
        ) {
            return &curr_node->region;
        }

        //check right branch
        if((addr_t) curr_reg.va_start < address){
          curr_node = curr_node->right;
        }

        //check right branch
        else{
          curr_node = curr_node->left;
        }
    }
    
    return NULL;
}

int mm_splay_tree_node_destroy( mm_splay_tree_node_t * node) {
    if (node != NULL) {
        mm_splay_tree_node_destroy(node->left);
        mm_splay_tree_node_destroy(node->right);
        free(node);
    }
    return 0;
}

int mm_splay_tree_destroy(mm_struct_t * self) {
    mm_splay_tree_t * tree = (mm_splay_tree_t *) self;

    mm_splay_tree_node_destroy(tree->root);
    free(tree);

    DEBUG_SP("Done: splay tree destroyed!\n");
    return 0;
}


/* Helper function for splay_tree_foreach.
   Run FUNC on every node in KEY.  */

// static void
// splay_tree_foreach_internal (mm_splay_tree_node_t node, mm_splay_tree_callback_t func,
// 			     void *data)
// {
//   if (!node)
//     return;
//   func (&node->key, data);
//   splay_tree_foreach_internal (node->left, func, data);
//   /* Yeah, whatever.  GCC can fix my tail recursion.  */
//   splay_tree_foreach_internal (node->right, func, data);
// }

// /* Run FUNC on each of the nodes in SP.  */

// attribute_hidden void
// splay_tree_foreach (mm_splay_tree_t * sp, mm_splay_tree_callback_t func, void *data)
// {
//   splay_tree_foreach_internal (sp->root, func, data);
// }


int mm_splay_tree_init(mm_splay_tree_t * spt){
    mm_struct_init(& (spt -> super));
    
    
    spt->super.vptr->insert = &mm_splay_tree_insert;
    spt->super.vptr->show = &mm_splay_tree_show;
    spt->super.vptr->check_overlap = &mm_splay_tree_check_overlap;
    spt->super.vptr->remove = &mm_splay_tree_remove;
    spt->super.vptr->contains = &mm_splay_tree_contains;
    spt->super.vptr->find_reg_at_addr = &mm_splay_tree_find_reg_at_addr;
    spt->super.vptr->update_region = &mm_splay_tree_update_region;
    spt->super.vptr->destroy = &mm_splay_tree_destroy;

    spt->root = NULL;

    return 0;
}

mm_struct_t * mm_splay_tree_create() {
    mm_splay_tree_t *mytree = (mm_splay_tree_t *) malloc(sizeof(mm_splay_tree_t));

    if (! mytree) {
        ERROR_PRINT("cannot allocate a linked list data structure to track region mapping\n");
        return 0;
    }

    mm_splay_tree_init(mytree);

    return &mytree->super;
}
