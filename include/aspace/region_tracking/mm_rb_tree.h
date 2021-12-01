#ifndef __MM_RB_TREE_H__
#define __MM_RB_TREE_H__

#include <aspace/region_tracking/node_struct.h>

#define RB_TREE_CHECK_AFTER_TRAVERSAL 0
#define MAX_DEPTH_LEVEL_ORDER_PRINT 6
#define MAX_SIZE_INORDER_PRINT 20
enum rb_tree_node_color {
    BLACK,
    RED
};

typedef struct mm_rb_node {
    nk_aspace_region_t region;
    struct mm_rb_node * parent;
    struct mm_rb_node * left;
    struct mm_rb_node * right;
    enum rb_tree_node_color color;
} mm_rb_node_t;

typedef struct rb_tree
{
    mm_struct_t super;
    mm_rb_node_t * NIL;
    mm_rb_node_t * root;
    
    int (*compf)(mm_rb_node_t * n1, mm_rb_node_t * n2);
} mm_rb_tree_t;

mm_struct_t * mm_rb_tree_create();
mm_rb_tree_t * mm_rb_tree_create_actual_rb_tree();
int rb_tree_check(mm_struct_t * self);


/*
 * HACK --- expose certain internal methods for compliance purposes
 */ 
mm_rb_node_t * rb_tree_minimum(mm_rb_tree_t * tree, mm_rb_node_t * node);
mm_rb_node_t * rb_tree_next_smallest(mm_rb_tree_t * tree, mm_rb_node_t * node);


/*
 * ========== Definitions --- functions for carat allocation map ========== 
 */ 

int rb_comp_alloc_entry(mm_rb_node_t * n1, mm_rb_node_t * n2);
int rb_comp_escape(mm_rb_node_t * n1, mm_rb_node_t * n2);
int rb_tree_remove_alloc(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags);
int rb_tree_remove_escape(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags);
nk_aspace_region_t * rb_tree_find_allocation_entry_from_addr(mm_struct_t * self, addr_t address);
    

#endif
