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
 * Copyright (c) 2020, Souradip Ghosh <sgh@u.northwestern.edu>
 * Copyright (c) 2020, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Souradip Ghosh <sgh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

/*
 * Simple implementation of a cache using the least 
 * recently used eviction policy
 */ 

#ifndef __LRU_CACHE_H__
#define __LRU_CACHE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nautilus/nautilus.h>
#include <nautilus/list.h>

#if DO_LRU_CACHE_PRINT
#define LRU_CACHE_PRINT(...) nk_vc_printf(__VA_ARGS__)
#else
#define LRU_CACHE_PRINT(...) 
#endif

#define USED __attribute__((used))
#define LRU_CACHE_MALLOC(n) ({void *__p = malloc(n); if (!__p) { LRU_CACHE_PRINT("Malloc failed\n"); panic("Malloc failed\n"); } __p;})
#define LRU_CACHE_REALLOC(p, n) ({void *__p = realloc(p, n); if (!__p) { LRU_CACHE_PRINT("Realloc failed\n"); panic("Malloc failed\n"); } __p;})


/*
 * ---------- Definitions ---------
 */ 
#define LRU_CACHE_SIZE 31 


typedef struct {
    
    /*
     * Entry value
     */ 
    uint64_t value;


    /*
     * Tracking list node
     */ 
    struct list_head tracking_list_node;


    /*
     * Hash table node
     */
    struct list_head table_node;

} lru_cache_entry ;


typedef struct {
    
    /*
     * Hash table of cache entries
     */
    struct list_head entries[LRU_CACHE_SIZE]; 


    /*
     * LRU tracking list 
     */ 
    struct list_head tracking_list;


    /*
     * Entry count
     */ 
    unsigned entry_count;

} lru_cache ;


/*
 * ---------- Initialization Methods ----------
 */

/*
 * Init for lru_cache_entry
 */ 
lru_cache_entry *build_lru_cache_entry(uint64_t value);

void destroy_lru_cache_entry(lru_cache_entry *entry);


/*
 * Init for lru_cache
 */ 
lru_cache *build_lru_cache(void);

void destroy_lru_cache(lru_cache *cache);


/*
 * ---------- Public Operations ----------
 */
unsigned query_lru_cache(
    lru_cache *cache,
    uint64_t value, 
    unsigned add_if_not_found
);

void add_to_lru_cache(
    lru_cache *cache,
    uint64_t value, 
    unsigned entry_index /* Default=-1 */
);

void debug_lru_cache(lru_cache *cache);


#ifdef __cplusplus
}
#endif


#endif
