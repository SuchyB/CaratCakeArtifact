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
 * Skip list implementation
 *
 * ASIDE: Too many things are referred to as gears here
 */ 

#ifndef __MAP_H__
#define __MAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nautilus/naut_types.h>
#include <nautilus/skiplist.h>

#define DO_MAP_PRINT 0 

#if DO_MAP_PRINT
#define MAP_PRINT(...) nk_vc_printf(__VA_ARGS__)
#else
#define MAP_PRINT(...) 
#endif

#define MAP_MALLOC(n) ({void *__p = malloc(n); if (!__p) { MAP_PRINT("Malloc failed\n"); panic("Malloc failed\n"); } __p;})
#define MAP_REALLOC(p, n) ({void *__p = realloc(p, n); if (!__p) { MAP_PRINT("Realloc failed\n"); panic("Malloc failed\n"); } __p;})

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

// Needs optimization
#define NK_MAP_STRUCTS_DECL(kt, vt) \
    typedef struct nk_pair_##kt##_##vt { \
        kt first; \
        vt second; \
    } nk_pair_##kt##_##vt; \
	typedef struct nk_map_entry_##kt##_##vt { \
		nk_slist_node_##kt *key_node; \
		nk_pair_##kt##_##vt *pair; \
	} nk_map_entry_##kt##_##vt; \
	typedef struct { \
		nk_slist_##kt *sl; \
	} nk_map_##kt##_##vt; \

#define NK_PAIR_BUILD(ft, st, f, s) ({ \
    nk_pair_##ft##_##st *new_pair = (nk_pair_##ft##_##st *) MAP_MALLOC(sizeof(nk_pair_##ft##_##st)); \
    new_pair->first = f; \
    new_pair->second = s; \
	nk_vc_printf("pair_build: %d, %d\n", new_pair->first, new_pair->second); \
    new_pair; \
})

#define NK_MAP_ENTRY_BUILD(kt, vt, kn, p) ({ \
    nk_map_entry_##kt##_##vt *new_entry = (nk_map_entry_##kt##_##vt *) MAP_MALLOC(sizeof(nk_map_entry_##kt##_##vt)); \
	new_entry->key_node = kn; \
	new_entry->pair = p; \
	nk_vc_printf("build --- key_node: %d\n", new_entry->key_node->data); \
	nk_vc_printf("build --- pair: %d, %d\n", new_entry->pair->first, new_entry->pair->second); \
	new_entry; \
})

NK_MAP_STRUCTS_DECL(uintptr_t, uintptr_t);
NK_MAP_STRUCTS_DECL(uintptr_t, uint64_t);
NK_MAP_STRUCTS_DECL(int, int);

#define _nk_get_map_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define nk_map_build(kt, vt) ({ \
	/* Allocate new map struct */ \
	nk_map_##kt##_##vt *new_map = (nk_map_##kt##_##vt *) MAP_MALLOC(sizeof(nk_map_##kt##_##vt)); \
	\
	/* Set up underlying skiplist */ \
	new_map->sl = nk_slist_build(kt, DEFAULT_TOP_GEAR); \
   	\
	/* Give the sentinals map entries */ \
	__auto_type *right_sentinal = nk_slist_get_right_sentinal((new_map->sl)); \
	__auto_type *left_sentinal = nk_slist_get_left_sentinal((new_map->sl)); \
	nk_vc_printf("left_sentinal: %d\n", left_sentinal->data); \
	nk_vc_printf("right_sentinal: %d\n", right_sentinal->data); \
	_nk_map_build_structs(new_map, kt, vt, (right_sentinal->data), 7, right_sentinal); \
	_nk_map_build_structs(new_map, kt, vt, (left_sentinal->data), 8, left_sentinal); \
	\
	/* Return the new map */ \
	new_map; \
})

#define _nk_map_get_entry(map, kt, vt, the_node) ({ \
	/* Perform bounds checking on the node and underlying skiplist 
	 * because _nk_get_map_entry will fail otherwise  \
	DEPRECATED	
	if ((nk_slist_is_left_sentinal(the_node)) \
		|| (nk_slist_is_right_sentinal(the_node)) \
		{ NULL; } \
	\
	*/ \
	/* Find the map entry that the key (pair) belongs to */ \
	nk_map_entry_##kt##_##vt *the_map_entry = _nk_get_map_entry((&the_node), struct nk_map_entry_##kt##_##vt, key_node); \
	\
	nk_vc_printf("key_node: %d\n", the_map_entry->key_node->data); \
	nk_vc_printf("pair: %d, %d\n", the_map_entry->pair->first, the_map_entry->pair->second); \
	/* POTENTIAL PROBLEM
	Return if something went wrong with the container_of macro \
	if (!the_map_entry) { NULL; } \
	\
	*/ \
	/* Return the map entry */ \
	the_map_entry; \
})	

#define _nk_map_find_map_entry(map, kt, vt, key) ({ \
	/* Find the key in the underlying skiplist */ \
	nk_slist_node_##kt *found_node = nk_slist_find(kt, (map->sl), key); \
   	\
	/* Return if no key was found */ \
	if (!found_node) { NULL; } \
	\
	/* Get and return the map entry */ \
	__auto_type *the_entry = _nk_map_get_entry(map, kt, vt, found_node); \
	the_entry; \
})

#define _nk_map_build_structs(map, kt, vt, key, val, node) ({ \
	/* Build key-value pair */ \
	__auto_type *new_pair = NK_PAIR_BUILD(kt, vt, key, val); \
	\
	/* Build map entry */ \
	NK_MAP_ENTRY_BUILD(kt, vt, node, new_pair); \
})

#define _nk_map_destroy_structs(map, entry) ({ \
	/* Free the skiplist node containing the key
	 * DIRECTLY */ \
	_nk_slist_node_destroy((map->sl), (entry->key_node)); \
	\
	/* Free the pair (i.e. data) */ \
	free(entry->pair); \
	/* Free the map entry */ \
	free(entry); \
})

#define nk_map_at(map, kt, vt, key) ({ \
	/* Find the pair for the key */ \
	__auto_type *the_entry = *_nk_map_find_map_entry(map, kt, vt, key); \
	\
	if (!the_entry) { NULL; } \
	\
	/* Get the value from the pair */ \
	/* WILL CAUSE TROUBLE */ \
	the_entry->pair->second; \
})	

#define nk_map_insert(map, kt, vt, key, val) ({ \
	/* Insert the key into the underlying skiplist */ \
	nk_slist_node_##kt node_to_fill; \
	uint8_t add_status = nk_slist_add(kt, (map->sl), key, &node_to_fill); \
	\
	/* Build the necessary map structs (not saved) */ \
	_nk_map_build_structs(map, kt, vt, key, val, node_to_fill); /* FIX */ \
	\
	/* Return the add status */ \
	add_status; \
})

#define nk_map_insert_by_force(map, kt, vt, key, val) ({ \
	/* Insert the key into the underlying skiplist */ \
	nk_slist_node_##kt node_to_fill; \
	uint8_t add_status = nk_slist_add_by_force(kt, (map->sl), key, &node_to_fill); \
	\
	/* Build the necessary map structs (not saved) */ \
	_nk_map_build_structs(map, kt, vt, key, val, node_to_fill); /* FIX */ \
	\
	/* Return the add status */ \
	add_status; \
})

#define nk_map_remove(map, kt, key) ({ \
	/* Find the pair for the key */ \
	__auto_type *the_entry = *_nk_map_find_map_entry(map, kt, vt, key); \
	\
	if (!the_entry) { NULL; } \
	\
	/* Free all relevant memory */ \
	_nk_map_destroy_structs(map, the_entry); \
})

#define nk_map_better_lower_bound(map, kt, vt, key) ({ \
	/* Perform lower_bound using the key w/ 
	 * underlying skiplist */ \
	__auto_type *the_node = nk_slist_nk_slist_better_lower_bound(kt, (map->sl), key); \
	\
	/* Get the corresponding map entry */ \
	__auto_type *the_entry = _nk_map_get_entry(map, kt, vt, the_node); \
	\
	/* Return the embedded pair */ \
	the_entry->pair; \
})

#define nk_map_foreach(map, kt, vt, the_pair, iter) for (iter = (map->sl->all_left[0]), the_pair = ((_nk_map_get_entry(map, kt, vt, iter))->pair); iter != NULL; iter = iter->succ_nodes[0])

#define nk_map_reverse(map, kt, vt, the_pair, iter) for (iter = (map->sl->all_right[0]), the_pair = ((_nk_map_get_entry(map, kt, vt, iter))->pair); iter != NULL; iter = iter->pred_nodes[0])


#define nk_map_get_size(map) (map->sl->size)


#ifdef __cplusplus
}
#endif


#endif


