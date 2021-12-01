/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2020, Drew Kersnar <drewkersnar2021@u.northwestern.edu>
 * Copyright (c) 2020, Gaurav Chaudhary <gauravchaudhary2021@u.northwestern.edu>
 * Copyright (c) 2020, Souradip Ghosh <sgh@u.northwestern.edu>
 * Copyright (c) 2020, Brian Suchy <briansuchy2022@u.northwestern.edu>
 * Copyright (c) 2020, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2020, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Drew Kersnar, Gaurav Chaudhary, Souradip Ghosh, 
 * 			Brian Suchy, Peter Dinda 
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

/*
 * Runtime Tables --- CARAT Runtime --- ported to C 
 * 
 * Keywords:
 * - FIX: obvious
 * - CONV: conversion from C++ to C, format: CONV [C++] -> [C]
 */ 
#pragma once

#include <nautilus/nautilus.h>
#include <aspace/carat.h>


/*
 * =================== Interfaces for runtime table data structures ===================  
 * 
 * Interfaces for:
 * - CARAT contexts
 * - Allocation tables
 * - Escapes sets
 * - Allocation entries
 */ 


/*
 * Helper macros for manipulating CARAT contexts 
 * (nk_carat_context). Note that all parameters
 * are ASSUMED to be pointers
 */ 

/*
 * Conditions check 
 */ 
#define CARAT_HACK 0
#define CARAT_HACK_PRINT if (CARAT_HACK) nk_vc_printf
#define CHECK_CARAT_BOOTSTRAP_FLAG if (!karat_ready) { return; } else { if (CARAT_HACK) { printf("on!\n"); } } 
#define CHECK_CARAT_READY(c) if (!(c->carat_ready)) { return; }
#define CARAT_READY_ON(c) c->carat_ready = 1
#define CARAT_READY_OFF(c) c->carat_ready = 0


#define USE_SKIPLIST 0
#if USE_SKIPLIST

/* 
 * Interface for "nk_carat_escape_set" --- generic
 */ 
#define CARAT_ESCAPE_SET_BUILD nk_slist_build(uintptr_t, CARAT_INIT_NUM_GEARS)
#define CARAT_ESCAPE_SET_SIZE(set) (nk_slist_get_size(set) - 2)
#define CARAT_ESCAPE_SET_ADD(set, key) nk_slist_add(uintptr_t, set, ((uintptr_t) key))
#define CARAT_ESCAPES_SET_ITERATE(set) \
    nk_slist_node_uintptr_t *iterator; \
    uintptr_t val; \
    \
    nk_slist_foreach(set, val, iterator) 


/*
 * Interface for "nk_carat_allocation_map" --- specifically for the
 * per nk_carat_context (i.e. @c) "allocation_map" data structure
 */ 

#define CARAT_ALLOCATION_MAP_BUILD nk_map_build(uintptr_t, uintptr_t)
#define CARAT_ALLOCATION_MAP_SETUP(map) if (0) { printf("CARAT: shouldn't be calling this\n"); } 
#define CARAT_ALLOCATION_MAP_SIZE(c) nk_map_get_size((c->allocation_map))
#define CARAT_ALLOCATION_MAP_INSERT(c, key, val) (nk_map_insert((c->allocation_map), uintptr_t, uintptr_t, ((uintptr_t) key), ((uintptr_t) val))) 
#define CARAT_ALLOCATION_MAP_INSERT_OR_ASSIGN(c, key, val) (nk_map_insert_by_force((c->allocation_map), uintptr_t, uintptr_t, ((uintptr_t) key), ((uintptr_t) val)))
#define CARAT_ALLOCATION_MAP_REMOVE(c, key) nk_map_remove((c->allocation_map), uintptr_t, uintptr_t, ((uintptr_t) key))
#define CARAT_ALLOCATION_MAP_BETTER_LOWER_BOUND(c, key) nk_map_better_lower_bound((c->allocation_map), uintptr_t, uintptr_t, ((uintptr_t) key))
#define CARAT_ALLOCATION_MAP_ITERATE(c) \
	nk_slist_node_uintptr_t_uintptr_t *iterator; \
	nk_pair_uintptr_t_uintptr_t *pair; \
    \
    nk_slist_foreach((c->allocation_map), pair, iterator)

#define FETCH_ALLOCATION_ENTRY_FROM_ITERATOR ((allocation_entry *) iterator->region)
#define CARAT_ALLOCATION_MAP_CURRENT_ADDRESS ((void *) (FETCH_ALLOCATION_ENTRY_FROM_ITERATOR)->pointer) // Only to be used within CARAT_ALLOCATION_MAP_ITERATE
#define CARAT_ALLOCATION_MAP_CURRENT_ENTRY ((allocation_entry *) (pair->second)) // Only to be used within CARAT_ALLOCATION_MAP_ITERATE


#else

// --- RB TREE VARIANT ---


#define rb_tree_foreach(tree, iterator) \
    /* typeof(@tree) = mm_rb_tree_t */ \
    iterator = rb_tree_minimum(tree, tree->root); \
    for (; iterator != tree->NIL ; iterator = rb_tree_next_smallest(tree, iterator)) \

/*
 * IMPORTANT NOTE --- We perform extreme hackery to get the
 * RB tree as is to work with our allocation tables and escapes
 * sets. Part of this is the assumption that the size of an 
 * nk_aspace_region_t is ***necessarily larger*** than the size
 * of an allocation entry. Otherwise this scheme will not work.
 *
 */

#define CARAT_ESCAPE_SET_BUILD mm_rb_tree_create_actual_rb_tree(); 

#define CARAT_ESCAPE_SET_SETUP(set) \
    if (1) \
    { \
        set->compf = &rb_comp_escape; \
        set->super.vptr->remove = &rb_tree_remove_escape; \
    }

#define CARAT_ESCAPE_SET_SIZE(set) set->super.size

#define CARAT_ESCAPE_SET_ADD(set, key) /* typeof(key)=(void **) */ \
    (mm_insert(&(set->super), ((nk_aspace_region_t *) key))) 

#define CARAT_ESCAPES_SET_ITERATE(set) \
    mm_rb_node_t *iterator = NULL; \
    rb_tree_foreach(set, iterator)

#define FETCH_ESCAPE_FROM_ITERATOR (((void **) &(iterator->region)))


// ---


#define CARAT_ALLOCATION_MAP_BUILD mm_rb_tree_create_actual_rb_tree()

#define CARAT_ALLOCATION_MAP_SETUP(map) \
    if (1) \
    { \
        map->compf = &rb_comp_alloc_entry; \
        map->super.vptr->remove = &rb_tree_remove_alloc; \
        map->super.vptr->find_reg_at_addr = &rb_tree_find_allocation_entry_from_addr; \
    }

#define CARAT_ALLOCATION_MAP_SIZE(c) (c->allocation_map->super.size)  

#define CARAT_ALLOCATION_MAP_INSERT(c, key) /* typeof(@key)=(allocation_entry *) */ \
    CARAT_HACK_PRINT("c->allocation_map: %p\n", c->allocation_map); \
    CARAT_HACK_PRINT("&(c->allocation_map->super): %p\n", &(c->allocation_map->super)); \
    CARAT_HACK_PRINT("((nk_aspace_region_t *) key): %p\n", ((nk_aspace_region_t *) key)); \
    (mm_insert(&(c->allocation_map->super), ((nk_aspace_region_t *) key))) 

#define CARAT_ALLOCATION_MAP_REMOVE(c, key) /* @key is a simple pointer, typeof(@key)=void * */ \
    (mm_remove(&(c->allocation_map->super), ((nk_aspace_region_t *) key), 0))

#define CARAT_ALLOCATION_MAP_BETTER_LOWER_BOUND(c, key) \
    ((allocation_entry *) (mm_find_reg_at_addr(&(c->allocation_map->super), ((addr_t) key))))

#define CARAT_ALLOCATION_MAP_ITERATE(c) \
    mm_rb_node_t *iterator = NULL; \
    rb_tree_foreach((c->allocation_map), iterator)

#define FETCH_ALLOCATION_ENTRY_FROM_ITERATOR (((allocation_entry *) &(iterator->region)))

#endif


/*
 * Setup/constructor for an allocation_entry object
 */ 
allocation_entry _carat_create_allocation_entry(void *ptr, uint64_t allocation_size);


/*
 * Macro expansion utility --- creating allocation_entry objects
 * and adding them to the allocation map
 */ 
#define CREATE_ENTRY_AND_ADD(ctx, key, size) \
	/*
	 * Create a new allocation_entry object for the new_address to be added
	 */ \
    CARAT_HACK_PRINT("before new_entry\n"); \
	allocation_entry new_entry = _carat_create_allocation_entry(key, size); \
    CARAT_HACK_PRINT("new_entry: %p\n", &new_entry); \
    \
    \
	/*
	 * Add the mapping [@##key : newEntry] to the allocation_map
	 */ \
    CARAT_ALLOCATION_MAP_INSERT(ctx, &new_entry); 


/*
 * Macro expansion utility --- for deleting allocation_entry objects
 */ 
#define REMOVE_ENTRY(ctx, key, str) \
	/*
	 * Delete the @##key from the allocation map
	 */ \
	if (CARAT_ALLOCATION_MAP_REMOVE(ctx, key) == -1) { \
		panic(str" %p\n", key); \
	}


#define REMOVE_ENTRY_SILENT(ctx, key) \
	/*
	 * Delete the @##key from the allocation map
	 */ \
	if (CARAT_ALLOCATION_MAP_REMOVE(ctx, key) == -1) { \
        CARAT_PRINT("CARAT: Attempted to free this address, corresponding allocation entry not found: %p\n", (void *) key); \
	}


/*
 * Macro expansion utility --- fetch the current CARAT context
 */ 
#define FETCH_THREAD (get_cur_thread_fast())
#define FETCH_CARAT_ASPACE ((nk_aspace_carat_t *) FETCH_THREAD->aspace->state) 
#define FETCH_CARAT_CONTEXT (FETCH_CARAT_ASPACE->context) 


/*
 * CARAT context fetchers, setters
 */ 
#define FETCH_TOTAL_ESCAPES(ctx) (ctx->total_escape_entries)
#define FETCH_ESCAPE_WINDOW(ctx) (ctx->escape_window)
#define RESET_ESCAPE_WINDOW(ctx) ctx->total_escape_entries = 0
#define ADD_ESCAPE_TO_WINDOW(ctx, addr) \
    ctx->escape_window[FETCH_TOTAL_ESCAPES(ctx)] = ((void **) addr); \
    ctx->total_escape_entries++;


/*
 * Debugging
 */
#define PRINT_ASPACE_INFO \
    if (CARAT_HACK) \
    { \
        DS("gct: "); \
        DHQ(((uint64_t) get_cur_thread())); \
        DS("\na: "); \
        DHQ(((uint64_t) get_cur_thread()->aspace)); \
        DS("\nctx "); \
        DHQ(((uint64_t) (FETCH_CARAT_CONTEXT))); \
        DS("\n"); \
    }



/*
 * =================== Init/Setup ===================  
 */ 

/*
 * TOP --- 
 * 
 * Setup for CARAT inside init(): nk_carat_init() will handle all 
 * setup for the allocation table, escape window, and any stack 
 * address handling
 */

/*
 * Compiler target for globals allocation tracking
 */ 
void _nk_carat_globals_compiler_target(void);


/*
 * Main driver for global CARAT initialization
 */ 
void nk_carat_init(void);


/*
 * Driver for per-aspace CARAT initialization
 */ 
nk_carat_context * initialize_new_carat_context(void);


/*
 * Utility for rsp 
 */
uint64_t _carat_get_rsp(void);


/*
 * =================== Utility Analysis Methods ===================  
 */ 

/*
 * Determines if a target address @query_address aliases with the specified 
 * allocation @alloc_address by performing a range search within the memory
 * pointed to by @alloc_address --- returns boolean values
 */ 
int _carat_does_alias(void *query_address, void *alloc_address, uint64_t allocation_size);


/*
 * Calculates the offset of @query_address within the memory pointed to by 
 * @alloc_address IFF @query_address aliases @alloc_address --- returns -1
 * upon failure
 */ 
sint64_t _carat_get_query_offset(void *query_address, void *alloc_address, uint64_t allocation_size);


/*
 * Takes a specified allocation and returns its corresponding allocation_entry,
 * otherwise return nullptr
 */ 
allocation_entry * _carat_find_allocation_entry(
    nk_carat_context *the_context,
    void *address
);


/*
 * Determines if an address/allocation entry tracked by the runtime 
 * tables is pinned in memory
 */ 
int _is_pinned(allocation_entry *entry);


/*
 * Statistics --- obvious
 */
void nk_carat_report_statistics(void);


/*
 * =================== Allocations Handling Methods ===================  
 */ 

/*
 * Instrumentation for "malloc" --- adding
 */
void nk_carat_instrument_global(void *address, uint64_t allocation_size, uint64_t global_ID);


/*
 * Instrumentation for "malloc" --- adding
 */
void nk_carat_instrument_malloc(void *address, uint64_t allocation_size);


/*
 * Instrumentation for "calloc" --- adding
 */ 
void nk_carat_instrument_calloc(void *address, uint64_t size_of_element, uint64_t num_elements);


/*
 * Instrumentation for "realloc" --- adding
 */
void nk_carat_instrument_realloc(void *new_address, uint64_t allocation_size, void *old_address);


/*
 * Instrumentation for "free" --- removing
 */
void nk_carat_instrument_free(void *address);


/*
 * =================== Escapes Handling Methods ===================  
 */ 

/*
 * Instrumentation for escapes
 * 1. Search for @escaping_address within global allocation map
 * 2. If an aliasing entry is found, the escape must be recorded into
 *    the aliasing entry's escapes set
 */
void nk_carat_instrument_escapes(void *escaping_address);


/*
 * Batch processing for escapes
 */ 
void _carat_process_escape_window(nk_carat_context *the_context);


/*
 * =================== Protection Handling Methods ===================  
 */ 

/*
 * Instrumentation for memory accesses
 * If the type of access specified by @is_write 
 * is determined to be illegal, panic. Otherwise, 
 * do nothing
 */
#if USER_REGION_CHECK
void nk_carat_guard_address(void *memory_address, int is_write, void* aspace);
#else
void nk_carat_guard_address(void *memory_address, int is_write);
#endif

/*
 * Instrumentation for call instructions
 * Make sure the stack has enough space to grow to support this guarded call instruction. 
 */
void nk_carat_guard_callee_stack(uint64_t stack_frame_size);


/*
 * =================== Extra Instrumentation Methods ===================  
 */ 

/*
 * Explicitly pin a pointer/address in memory
 */ 
void nk_carat_pin_pointer(void *address);


/*
 * Explicitly pin the pointer/address stored within an escape 
 */ 
void nk_carat_pin_escaped_pointer(void *escape);
