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

#ifndef __SLIST_H__
#define __SLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nautilus/nautilus.h>
#include <nautilus/limits.h>
#include <nautilus/naut_string.h>
#include <nautilus/libccompat.h>

#define DO_SLIST_PRINT 0 

#if DO_SLIST_PRINT
#define SLIST_PRINT(...) nk_vc_printf(__VA_ARGS__)
#else
#define SLIST_PRINT(...) 
#endif

#define SLIST_MALLOC(n) ({void *__p = malloc(n); if (!__p) { SLIST_PRINT("Malloc failed\n"); panic("Malloc failed\n"); } __p;})
#define SLIST_REALLOC(p, n) ({void *__p = realloc(p, n); if (!__p) { SLIST_PRINT("Realloc failed\n"); panic("Malloc failed\n"); } __p;})

#define RAND_MAGIC 0x7301
#define SEED
#define RAND (rdtsc() % RAND_MAGIC)
#define SLIST_TOP_GEAR 8 // Deprecated
#define DEFAULT_TOP_GEAR 12

#define NO_CARAT __attribute__((used, annotate("nocarat")))
#define NO_CARAT_NO_INLINE __attribute__((used, noinline, annotate("nocarat")))
#define NO_CARAT_ALWAYS_INLINE __attribute__((used, always_inline, annotate("nocarat")))

#define UPSHIFT(g) g++
#define WHILE_DOWNSHIFTING(i, start) for (i = start; i >= 0; i--)

// Need more declarations
#define int_MAX INT_MAX
#define int_MIN INT_MIN

#define uint32_t_MAX UINT_MAX
#define uint32_t_MIN 0	

#define uint64_t_MAX ULONG_MAX
#define uint64_t_MIN 0

#define uintptr_t_MAX ULONG_MAX
#define uintptr_t_MIN 0

#define uintptr_t_uintptr_t_MAX ULONG_MAX
#define uintptr_t_uintptr_t_MIN 0

#define int_int_MAX INT_MAX
#define int_int_MIN INT_MIN

#define NK_SLIST_INIT(type) \
	typedef struct nk_slist_node_##type { \
		struct nk_slist_node_##type **succ_nodes; \
		struct nk_slist_node_##type **pred_nodes; \
		type data; \
		uint8_t gear; /* Number of gears occupied */ \
	} nk_slist_node_##type; \
	typedef struct { \
		nk_slist_node_##type **all_left; /* NEEDS Optimization --- only record the
										    sentinals, they're all the same anyway */ \
		nk_slist_node_##type **all_right; \
		uint64_t size; \
		uint8_t top_gear; \
	} nk_slist_##type; \

#define NK_MAP_INIT(kt, vt) \
	typedef struct nk_pair_##kt##_##vt { \
		kt first; \
		vt second; \
	} nk_pair_##kt##_##vt; \
	typedef struct nk_slist_node_##kt##_##vt { \
		struct nk_slist_node_##kt##_##vt **succ_nodes; \
		struct nk_slist_node_##kt##_##vt **pred_nodes; \
		struct nk_pair_##kt##_##vt *data; \
		uint8_t gear; /* Number of gears occupied */ \
	} nk_slist_node_##kt##_##vt; \
	typedef struct { \
		nk_slist_node_##kt##_##vt **all_left; /* NEEDS Optimization --- only record the
										    	 sentinals, they're all the same anyway */ \
		nk_slist_node_##kt##_##vt **all_right; \
		uint64_t size; \
		uint8_t top_gear; \
	} nk_slist_##kt##_##vt; \

#define NK_SLIST_DECL(type) \
	NK_SLIST_INIT(type) \
	NO_CARAT static inline nk_slist_node_##type *_nk_slist_find_worker_##type(type val, \
											  					   		  nk_slist_node_##type *ipts[], \
											   					   		  nk_slist_node_##type *the_gearbox, \
											   					   		  uint8_t start_gear, \
											   					   		  uint8_t record) { \
		int i; \
		nk_slist_node_##type *gearbox = the_gearbox; \
		\
		WHILE_DOWNSHIFTING(i, (start_gear - 1)) \
		{ \
			nk_slist_node_##type *next_node = gearbox->succ_nodes[i]; \
			\
			while (next_node) \
			{ \
				if (next_node->data < val) \
				{ \
					/* Throttle */ \
					next_node = next_node->succ_nodes[i]; \
					continue; \
				} \
				\
				/* Found the right node */ \
				if (next_node->data == val) { return next_node; } \
				\
				/* Clutch */ \
				gearbox = next_node->pred_nodes[i]; \
				\
				/* If we want to record insertion points */ \
				if (record) { ipts[i] = gearbox; } \
				\
				break; \
			} \
		} \
		\
		return NULL; \
	} \
	\
	NO_CARAT static inline nk_slist_node_##type *__nk_slist_node_build_##type(nk_slist_##type *sl, \
																		  type val, \
																		  uint8_t g) { \
		/* Allocate */ \
		nk_slist_node_##type *sln = (nk_slist_node_##type *) (SLIST_MALLOC(sizeof(nk_slist_node_##type))); \
		\
		/* Set fields */ \
		sln->data = val; \
		sln->gear = g; \
		sln->succ_nodes = (nk_slist_node_##type **) (SLIST_MALLOC(sizeof(nk_slist_node_##type *) * sln->gear)); \
		sln->pred_nodes = (nk_slist_node_##type **) (SLIST_MALLOC(sizeof(nk_slist_node_##type *) * sln->gear)); \
		\
		/* Zero initialize */ \
		memset(sln->succ_nodes, 0, sizeof(*(sln->succ_nodes))); \
		memset(sln->pred_nodes, 0, sizeof(*(sln->pred_nodes))); \
		\
		/* Increment skiplist size */ \
		(sl->size)++; \
		\
		return sln; \
	} \
	\
	NO_CARAT static inline nk_slist_node_##type *_nk_slist_build_sentinal_##type(nk_slist_##type *sl, \
																	  		 type sval, \
																	  		 uint8_t tg) { \
		return __nk_slist_node_build_##type(sl, sval, tg); \
	} \
	\
	/* Peter we're sorry */ \
	NO_CARAT_NO_INLINE static nk_slist_##type *_nk_slist_##type##_malloc() { \
		return (nk_slist_##type *) (SLIST_MALLOC(sizeof(nk_slist_##type))); \
	} \
	NO_CARAT_NO_INLINE static nk_slist_node_##type *_nk_slist_node_##type##_malloc() { \
		return (nk_slist_node_##type *) (SLIST_MALLOC(sizeof(nk_slist_node_##type))); \
	} \
	NO_CARAT_NO_INLINE static nk_slist_node_##type **_nk_slist_node_##type##_array_malloc(uint8_t elm) { \
		return (nk_slist_node_##type **) (SLIST_MALLOC(elm * (sizeof(nk_slist_node_##type *)))); \
	} \
	NO_CARAT_NO_INLINE static void _nk_slist_##type##_free(nk_slist_##type *sl) { \
		free(sl); \
	} \
	NO_CARAT_NO_INLINE static void _nk_slist_node_##type##_free(nk_slist_node_##type *sln) { \
		free(sln); \
	} \
	NO_CARAT_NO_INLINE static void _nk_slist_node_##type##_array_free(nk_slist_node_##type **slna) { \
		free(slna); \
	} \


#define NK_MAP_DECL(kt, vt) \
	NK_MAP_INIT(kt, vt) \
	NO_CARAT static inline nk_slist_node_##kt##_##vt *_nk_slist_find_worker_##kt##_##vt(nk_pair_##kt##_##vt *val, \
											  					   		  			nk_slist_node_##kt##_##vt *ipts[], \
											   					   		  			nk_slist_node_##kt##_##vt *the_gearbox, \
											   					   		  			uint8_t start_gear, \
											   					   		  			uint8_t record) { \
		int i; \
		nk_slist_node_##kt##_##vt *gearbox = the_gearbox; \
		\
		WHILE_DOWNSHIFTING(i, (start_gear - 1)) \
		{ \
			nk_slist_node_##kt##_##vt *next_node = gearbox->succ_nodes[i]; \
			\
			while (next_node) \
			{ \
				if (next_node->data->first < val->first) \
				{ \
					/* Throttle */ \
					next_node = next_node->succ_nodes[i]; \
					continue; \
				} \
				\
				/* Found the right node */ \
				if (next_node->data->first == val->first) { return next_node; } \
				\
				/* Clutch */ \
				gearbox = next_node->pred_nodes[i]; \
				\
				/* If we want to record insertion points */ \
				if (record) { ipts[i] = gearbox; } \
				\
				break; \
			} \
		} \
		\
		return NULL; \
	} \
	\
	NO_CARAT static inline nk_slist_node_##kt##_##vt *__nk_slist_node_build_##kt##_##vt(nk_slist_##kt##_##vt *sl, \
																		  			nk_pair_##kt##_##vt *pair, \
																		  			uint8_t g) { \
		/* Allocate */ \
		nk_slist_node_##kt##_##vt *sln = (nk_slist_node_##kt##_##vt *) (SLIST_MALLOC(sizeof(nk_slist_node_##kt##_##vt))); \
		\
		/* Set fields */ \
		sln->data = pair; \
		sln->gear = g; \
		sln->succ_nodes = (nk_slist_node_##kt##_##vt **) (SLIST_MALLOC(sizeof(nk_slist_node_##kt##_##vt *) * sln->gear)); \
		sln->pred_nodes = (nk_slist_node_##kt##_##vt **) (SLIST_MALLOC(sizeof(nk_slist_node_##kt##_##vt *) * sln->gear)); \
		\
		/* Zero initialize */ \
		memset(sln->succ_nodes, 0, sizeof(*(sln->succ_nodes))); \
		memset(sln->pred_nodes, 0, sizeof(*(sln->pred_nodes))); \
		\
		/* Increment skiplist size */ \
		(sl->size)++; \
		\
		return sln; \
	} \
	\
	/* Peter we're sorry */ \
	\
	NO_CARAT_NO_INLINE static nk_slist_##kt##_##vt *_nk_slist_##kt##_##vt##_malloc() { \
		return (nk_slist_##kt##_##vt *) (SLIST_MALLOC(sizeof(nk_slist_##kt##_##vt))); \
	} \
	NO_CARAT_NO_INLINE static nk_slist_node_##kt##_##vt *_nk_slist_node_##kt##_##vt##_malloc() { \
		return (nk_slist_node_##kt##_##vt *) (SLIST_MALLOC(sizeof(nk_slist_node_##kt##_##vt))); \
	} \
	NO_CARAT_NO_INLINE static nk_slist_node_##kt##_##vt **_nk_slist_node_##kt##_##vt##_array_malloc(uint8_t elm) { \
		return (nk_slist_node_##kt##_##vt **) (SLIST_MALLOC(elm * (sizeof(nk_slist_node_##kt##_##vt *)))); \
	} \
	NO_CARAT_NO_INLINE static nk_pair_##kt##_##vt *_nk_pair_##kt##_##vt##_malloc() { \
		return (nk_pair_##kt##_##vt *) (SLIST_MALLOC(sizeof(nk_pair_##kt##_##vt))); \
	} \
	NO_CARAT_NO_INLINE static void _nk_slist_##kt##_##vt##_free(nk_slist_##kt##_##vt *sl) { \
		free(sl); \
	} \
	NO_CARAT_NO_INLINE static void _nk_slist_node_##kt##_##vt##_free(nk_slist_node_##kt##_##vt *sln) { \
		free(sln); \
	} \
	NO_CARAT_NO_INLINE static void _nk_slist_node_##kt##_##vt##_array_free(nk_slist_node_##kt##_##vt **slna) { \
		free(slna); \
	} \
	NO_CARAT_NO_INLINE static void _nk_pair_##kt##_##vt##_free(nk_pair_##kt##_##vt *pair) { \
		free(pair); \
	} \
	NO_CARAT_NO_INLINE static nk_slist_node_##kt##_##vt *_nk_slist_build_sentinal_##kt##_##vt(nk_slist_##kt##_##vt *sl, \
																	  	   			   kt sval, \
																	  	   			   uint8_t tg) { \
		__auto_type *spair = nk_pair_build_malloc(kt, vt, sval, 0); /* Dummy pair for sentinal */ \
		return __nk_slist_node_build_##kt##_##vt(sl, spair, tg); \
 	} \
	
// Skip list internals
#define _nk_slist_get_rand_gear(top_gear) ({ \
	uint8_t gear = 1; \
	while (((RAND) & 1) && (gear < top_gear)) { UPSHIFT(gear); } \
	gear; \
})

#define _nk_slist_node_link(pred, succ, gear) ({ \
	pred->succ_nodes[gear] = succ; \
	succ->pred_nodes[gear] = pred; \
})

// Setup, cleanup
#define nk_slist_build(type, tg) ({ \
	SEED; \
	\
	/* Set up skip list parent structure */ \
	nk_slist_##type *sl = _nk_slist_##type##_malloc(); \
	\
	/* Init gears --- left side */ \
	sl->all_left = _nk_slist_node_##type##_array_malloc(tg); \
	memset(sl->all_left, 0, sizeof(*(sl->all_left))); \
	\
	/* Init gears --- right side */ \
	sl->all_right = _nk_slist_node_##type##_array_malloc(tg); \
	memset(sl->all_right, 0, sizeof(*(sl->all_right))); \
	\
	/* Set top gear */ \
	sl->top_gear = tg; \
	\
	sl->size = 0; \
	\
	/* Sentinals --- build nodes */ \
	nk_slist_node_##type *left_sentinal = _nk_slist_build_sentinal_##type(sl, type##_MIN, tg); \
	nk_slist_node_##type *right_sentinal = _nk_slist_build_sentinal_##type(sl, type##_MAX, tg); \
	\
	/* Set all initial linked-list head and tail pointers */ \
	int i; \
	for (i = 0; i < tg; i++) \
	{ \
		/* Set head to skip list parent structure gears \
		   array --- marks head of each list */ \
		sl->all_left[i] = left_sentinal; \
		sl->all_right[i] = right_sentinal; \
		\
		/* Set list node pointers for each gear level \
		   for each sentinal structure */ \
		left_sentinal->pred_nodes[i] = right_sentinal->succ_nodes[i] = NULL; \
		_nk_slist_node_link(left_sentinal, right_sentinal, i); \
	} \
	\
	sl; \
}) \

#define _nk_slist_node_build(sl, type, val, g) __nk_slist_node_build_##type(sl, val, g) 

#define _nk_slist_node_destroy(type, sl, sln) ({ \
	/* Free memory for skiplist node */ \
	_nk_slist_node_##type##_array_free(sln->succ_nodes); \
	_nk_slist_node_##type##_array_free(sln->pred_nodes); \
	_nk_slist_node_##type##_free(sln); \
	\
	/* Decrement skiplist size */ \
	(sl->size)--; \
})

#define nk_slist_destroy(type, sl) ({ \
	/* Gather all nodes via bottom gear list */ \
	__auto_type *sln = sl->all_left[0]; \
	\
	/* Iterate and delete */ \
	while (sln != NULL) \
	{ \
		__auto_type *temp = sln; \
		sln = sln->succ_nodes[0]; \
		_nk_slist_node_destroy(type, sl, temp); \
	} \
	\
	_nk_slist_##type##_free(sl); \
})

// Skip list operations
#define nk_slist_find(type, sl, val) ({ \
	nk_slist_node_##type *the_gearbox = sl->all_left[sl->top_gear - 1], \
						 *found = _nk_slist_find_worker_##type (val, NULL, the_gearbox, sl->top_gear, 0); \
	found; \
})

#define nk_slist_add_by_force(type, sl, val) ({ \
	/* Set up new node */ \
	uint8_t new_gear = _nk_slist_get_rand_gear(sl->top_gear), \
			status = 0; \
	\
	nk_slist_node_##type *ipts[new_gear]; \
	\
	/* Find all insertion points */ \
	nk_slist_node_##type *the_gearbox = sl->all_left[(new_gear) - 1], \
				  		 *found_node = _nk_slist_find_worker_##type (val, ipts, the_gearbox, new_gear, 1); \
	\
	/* Set the data anyway for the node if it already exists */ \
	if (found_node) { found_node->data = val; } \
	\
	else { \
		nk_slist_node_##type *new_node = _nk_slist_node_build(sl, type, val, new_gear); \
		\
		/* Set all successor and predecessor links */ \
		int i; \
		for (i = 0; i < new_node->gear; i++) \
		{ \
			nk_slist_node_##type *succ_node = ipts[i]->succ_nodes[i]; \
			_nk_slist_node_link(ipts[i], new_node, i); \
			_nk_slist_node_link(new_node, succ_node, i); \
		} \
		\
		/* Set status */ \
		status = 1; \
	} \
	\
	status; \
})

#define nk_slist_add(type, sl, val) ({ \
	/* Set up new node */ \
	uint8_t new_gear = _nk_slist_get_rand_gear(sl->top_gear), \
			status = 0; \
	\
	nk_slist_node_##type *ipts[new_gear]; \
	\
	/* Find all insertion points */ \
	nk_slist_node_##type *the_gearbox = sl->all_left[(new_gear) - 1], \
				  		 *found_node = _nk_slist_find_worker_##type (val, ipts, the_gearbox, new_gear, 1); \
	\
	/* Not going to add the node if it already exists, add otherwise */ \
	if (!found_node) { \
		\
		nk_slist_node_##type *new_node = _nk_slist_node_build(sl, type, val, new_gear); \
		\
		/* Set all successor and predecessor links */ \
		int i; \
		for (i = 0; i < new_node->gear; i++) \
		{ \
			nk_slist_node_##type *succ_node = ipts[i]->succ_nodes[i]; \
			_nk_slist_node_link(ipts[i], new_node, i); \
			_nk_slist_node_link(new_node, succ_node, i); \
		} \
		\
		/* Set status */ \
		status = 1; \
	} \
	status; \
})

#define nk_slist_remove(type, sl, val) ({ \
	\
	uint8_t status = 0; \
	\
	/* Find the node */ \
	nk_slist_node_##type *found_node = nk_slist_find(type, sl, val); \
	\
	/* Can't remove anything if the node doesn't exist */ \
	if (found_node) { \
		\
		/* Reset all predecessor and successor pointers */ \
		int i; \
		for (i = 0; i < found_node->gear; i++) { \
			_nk_slist_node_link(found_node->pred_nodes[i], found_node->succ_nodes[i], i); \
		} \
		\
		/* Free the node */ \
		_nk_slist_node_destroy(type, sl, found_node); \
		\
		/* Set status */ \
		status = 1; \
	} \
	status; \
})

#define nk_slist_better_lower_bound(type, sl, val) ({ \
	nk_slist_node_##type *ipts[(sl->top_gear)]; \
	nk_slist_node_##type *the_gearbox = sl->all_left[(sl->top_gear) - 1], \
				  		 *found_node = _nk_slist_find_worker_##type (val, ipts, the_gearbox, (sl->top_gear), 1); \
	\
	nk_slist_node_##type *ret_node = (found_node) ? found_node : ipts[0]; \
	ret_node; \
})	

#define nk_slist_foreach(sl, val, iter) for (iter = sl->all_left[0]->succ_nodes[0], val = iter->data; iter->succ_nodes[0] != NULL; iter = iter->succ_nodes[0], val = iter->data)

// #define nk_slist_foreach(sl, val, iter) for (iter = sl->all_left[0], val = iter->data; iter != NULL; iter = iter->succ_nodes[0], val = iter->data)

#define nk_slist_reverse(sl, val, iter) for (iter = sl->all_right[0]->pred_nodes[0], val = iter->data; iter->pred_nodes[0] != NULL; iter = iter->pred_nodes[0], val = iter->data)

#define nk_slist_get_left_sentinal(sl) (sl->all_left[0])

#define nk_slist_get_right_sentinal(sl) (sl->all_right[0])

#define nk_slist_is_left_sentinal(sl, node) (nk_slist_get_left_sentinal(sl) == node) 

#define nk_slist_is_right_sentinal(sl, node) (nk_slist_get_right_sentinal(sl) == node) 

#define nk_slist_get_size(sl) (sl->size)

NK_SLIST_DECL(int);
NK_SLIST_DECL(uint64_t);
NK_SLIST_DECL(uintptr_t);

// --- MAP ---
#define DEFAULT_TOP_GEAR_MAP 12


#define nk_pair_build_malloc(kt, vt, key, val) ({ \
	nk_pair_##kt##_##vt *pair = _nk_pair_##kt##_##vt##_malloc(); \
	pair->first = key; \
	pair->second = val; \
	pair; \
})

#define nk_pair_build(kt, vt, key, val) ({ \
	nk_pair_##kt##_##vt pair; \
	pair.first = key; \
	pair.second = val; \
	(&pair); \
})

#define nk_map_build(kt, vt) nk_slist_build(kt##_##vt, DEFAULT_TOP_GEAR_MAP)  

#define nk_map_find(map, kt, vt, key) ({ \
	__auto_type *pair = nk_pair_build(kt, vt, key, 0); /* Dummy pair */ \
	__auto_type *found_node = nk_slist_find(kt##_##vt, map, pair); \
	found_node; \
})	

#define nk_map_insert(map, kt, vt, key, val) ({ \
	__auto_type *pair = nk_pair_build_malloc(kt, vt, key, val); \
	uint8_t status = nk_slist_add(kt##_##vt, map, pair); \
	status; \
})	

#define nk_map_insert_by_force(map, kt, vt, key, val) ({ \
	__auto_type *pair = nk_pair_build_malloc(kt, vt, key, val); \
	uint8_t status = nk_slist_add_by_force(kt##_##vt, map, pair); \
	status; \
})	

#define nk_map_remove(map, kt, vt, key) ({ \
	__auto_type *pair = nk_pair_build(kt, vt, key, 0); /* Dummy pair */ \
 	uint8_t status = nk_slist_remove(kt##_##vt, map, pair); \
    status; \
})	

#define nk_map_better_lower_bound(map, kt, vt, key) ({ \
	__auto_type *pair = nk_pair_build(kt, vt, key, 0); /* Dummy pair */ \
	__auto_type *found_node = nk_slist_better_lower_bound(kt##_##vt, map, pair); \
	found_node; \
})	

#define nk_map_foreach(map, val, iter) nk_slist_foreach(map, val, iter)

#define nk_map_reverse(map, val, iter) nk_slist_foreach(map, val, iter)

#define nk_map_get_size(map) nk_slist_get_size(map)

NK_MAP_DECL(int, int);
NK_MAP_DECL(uintptr_t, uintptr_t);

#ifdef __cplusplus
}
#endif


#endif
