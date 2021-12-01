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

#include <nautilus/lru_cache.h>


/*
 * ---------- Initialization Methods ----------
 */

/*
 * Init for lru_cache_entry
 */ 
lru_cache_entry *build_lru_cache_entry(uint64_t value)
{
    /*
     * Allocate a new entry
     */ 
    lru_cache_entry *new_entry = LRU_CACHE_MALLOC(sizeof(lru_cache_entry));


    /*
     * Set internal value to @value
     */ 
    new_entry->value = value;


    /*
     * Initialize internal nodes
     */
    INIT_LIST_HEAD(&(new_entry->tracking_list_node));
    INIT_LIST_HEAD(&(new_entry->table_node));
    

    return new_entry;
}


void destroy_lru_cache_entry(lru_cache_entry *entry) { free(entry); }


/*
 * Init for lru_cache
 */ 
lru_cache *build_lru_cache(void)
{
    /*
     * Allocate a new lru_cache 
     */ 
    lru_cache *new_cache = LRU_CACHE_MALLOC(sizeof(lru_cache));


    /*
     * Initialize the tracking list and hash table 
     */ 
    INIT_LIST_HEAD(&(new_cache->tracking_list));
    for (unsigned i = 0 ; i < LRU_CACHE_SIZE ; i++)
        INIT_LIST_HEAD(&(new_cache->entries[i]));


    /*
     * Set entry count
     */ 
    new_cache->entry_count = 0;


    return new_cache;
}


void destroy_lru_cache(lru_cache *cache) { free(cache); /* Leaky */ }


/*
 * ---------- Private Operations ----------
 */
#define UINT64_C(c) c ## ULL
static unsigned _hash_for_lru_cache(uint64_t x)
{
    /*
     * https://stackoverflow.com/a/12996028
     */ 
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);


    /*
     * Return an index for the hash table
     */  
    return (x % LRU_CACHE_SIZE);
}


static void _update_lru_cache_tracking_list(
    lru_cache *cache,
    lru_cache_entry *entry
)
{
    /*
     * Delete @entry's tracking list node from @cache's 
     * tracking list, and reinitialize @entry's tracking
     * list node to add to the head of @cache's tracking list
     */ 
    struct list_head *tracking_list_node = &(entry->tracking_list_node);
    list_del_init(tracking_list_node);
    list_add(
        tracking_list_node,
        &(cache->tracking_list)
    );


    return;
}


static void _evict_entry_from_lru_cache(lru_cache *cache)
{
    /*
     * TOP
     *
     * Adopt the least recently used policy and remove 
     * said corresponding entry from @cache if @cache
     */ 

    /*
     * Check if @cache is full --- do nothing if not
     */ 
    if (cache->entry_count != LRU_CACHE_SIZE) return; 


    /*
     * Fetch the lru_cache_entry that corresponds to
     * the last entry in @cache->tracking_list
     */
    lru_cache_entry *entry_to_evict = 
        list_tail_entry(
            &(cache->tracking_list),
            lru_cache_entry,
            tracking_list_node
        );


    /*
     * Delete this entry from @cache's hash table
     */ 
    list_del(&(entry_to_evict->table_node));


    /*
     * Delete this entry from @cache's tracking list
     */ 
    list_del(&(entry_to_evict->tracking_list_node));
    

    /*
     * Cleanup memory, destroy the entry
     */ 
    destroy_lru_cache_entry(entry_to_evict);


    /*
     * Decrement @cache's entries stats
     */ 
    cache->entry_count -= 1;


    return;
}


/*
 * ---------- Public Operations ----------
 */
unsigned query_lru_cache(
    lru_cache *cache,
    uint64_t value, 
    unsigned add_if_not_found
)
{
    /*
     * Hash @value to fetch its index in @cache
     */ 
    unsigned index = _hash_for_lru_cache(value);


    /*
     * Fetch the corresponding entry list from @cache->entries
     */ 
    struct list_head *entry_list = &(cache->entries[index]);


    /*
     * Iterate across this list to find @value
     */
    unsigned is_present = 0;
    lru_cache_entry *iterator;
    list_for_each_entry(iterator, entry_list, table_node)
    {
        if (iterator->value == value)
        {
            is_present |= 1;
            break;
        }
    }


    /*
     * If @value isn't present, add @value to @cache
     * if necessary and return
     */ 
    if (!is_present) 
    {
        if (add_if_not_found) 
            add_to_lru_cache(
                cache,
                value,
                index
            );


        return is_present;
    }


    /*
     * Otherwise, update @cache->tracking_list
     */  
    _update_lru_cache_tracking_list(
        cache,
        iterator
    );

   
    /*
     * Return found
     */ 
    return is_present;
}


void add_to_lru_cache(
    lru_cache *cache,
    uint64_t value, 
    unsigned entry_index 
)
{
    /*
     * Evict an entry from @cache if necessary
     */ 
    _evict_entry_from_lru_cache(cache);


    /*
     * Build an lru_cache_entry for @value
     */ 
    lru_cache_entry *new_entry = build_lru_cache_entry(value); 


    /*
     * Find the entry in @cache's hash table to add @value. This
     * can either be @entry_index IFF it's not -1, or calculated
     * directly via a hash
     */ 
    unsigned index =
        (entry_index == -1) ?
        (_hash_for_lru_cache(value)) :
        (entry_index) ;


    /*
     * Add the new entry for @value to the hash table (@cache->entries)
     */ 
    list_add(
        &(new_entry->table_node),
        &(cache->entries[index]) 
    );


    /*
     * Add the new entry for @value to the tracking list
     * (@cache->tracking_list)
     */ 
    list_add(
        &(new_entry->tracking_list_node),
        &(cache->tracking_list)
    );


    /*
     * Increment @cache's entries stats
     */ 
    cache->entry_count += 1;


    return;
}


void debug_lru_cache(lru_cache *cache)
{
    /*
     * Print statistics
     */ 
    nk_vc_printf("---------- debug_lru_cache ----------\n");
    nk_vc_printf("entry_count: %u\n", cache->entry_count); 


    /*
     * Print hash map as it exists in @cache
     */ 
    nk_vc_printf("hash table:\n");
    for (unsigned i = 0 ; i < LRU_CACHE_SIZE ; i++)
    {
        nk_vc_printf("%u: ", i);
        struct list_head *entry_list = &(cache->entries[i]);
        lru_cache_entry *iterator;
        list_for_each_entry(iterator, entry_list, table_node) nk_vc_printf("%lu ", iterator->value);
        nk_vc_printf("\n");
    }

    
    /*
     * Print the tracking/LRU list 
     */ 
    nk_vc_printf("tracking list:\n");
    struct list_head *tracking_list = &(cache->tracking_list);
    lru_cache_entry *iterator;
    list_for_each_entry(iterator, tracking_list, tracking_list_node) nk_vc_printf("%lu ", iterator->value);
    nk_vc_printf("\n");


    return;
}

