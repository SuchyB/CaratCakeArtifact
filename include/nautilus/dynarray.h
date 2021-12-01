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
 * Dynamic array implementation
 */ 

#ifndef __DYNARRAY_H__
#define __DYNARRAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nautilus/spinlock.h>
#include <nautilus/naut_string.h>

#define DO_DYNARRAY_PRINT 1

#if DO_DYNARRAY_PRINT
#define DYNARRAY_PRINT(...) nk_vc_printf(__VA_ARGS__)
#else
#define DYNARRAY_PRINT(...) 
#endif

#define DYNARRAY_MALLOC(n) ({void *__p = malloc(n); if (!__p) { DYNARRAY_PRINT("Malloc failed\n"); panic("Malloc failed\n"); } __p;})
#define DYNARRAY_REALLOC(p, n) ({void *__p = realloc(p, n); if (!__p) { DYNARRAY_PRINT("Realloc failed\n"); panic("Malloc failed\n"); } __p;})

#define USED __attribute__((used))
#define DYNARRAY_INIT_SIZE 32
#define DYNARRAY_EXP_FACTOR 2

#define NK_DYNARRAY_INIT(type) \
	typedef struct { \
		size_t size, capacity; \
		spinlock_t lock; \
		type *data; \
	} nk_dynarray_##type ;

#define NK_DYNARRAY_DECL(type) \
	NK_DYNARRAY_INIT(type) \
	USED static inline nk_dynarray_##type *nk_dynarray_##type##_init() { \
		nk_dynarray_##type *new_da = (nk_dynarray_##type *) (DYNARRAY_MALLOC(sizeof(nk_dynarray_##type))); \
		new_da->data = (type *) (DYNARRAY_MALLOC(sizeof(type) * DYNARRAY_INIT_SIZE)); \
		memset(new_da->data, 0, (sizeof(type) * DYNARRAY_INIT_SIZE)); \
		new_da->size = 0; \
		new_da->capacity = DYNARRAY_INIT_SIZE; \
		spinlock_init(&(new_da->lock)); \
		return new_da; \
	} \

#define nk_dynarray_get(type) nk_dynarray_##type##_init()

#define _nk_dynarray_resize(da) ({ \
	if (((da)->size) == ((da)->capacity)) { \
		(da)->capacity *= DYNARRAY_EXP_FACTOR; \
		(da)->data = DYNARRAY_REALLOC(((da)->data), (sizeof((da)->data[0]) * ((da)->capacity))); \
	} \
})
	
#define nk_dynarray_empty(da) (!((da)->size))

#define nk_dynarray_empty_atomic(da) ({ \
	uint8_t flags = spin_lock_irq_save(&((da)->lock)); \
	uint8_t empty = nk_dynarray_empty(da); \
	spin_unlock_irq_restore(&((da)->lock), flags); \
	empty; \
})

#define nk_dynarray_begin(da) ((da)->data[0])

#define nk_dynarray_end(da) ((da)->data[((da)->size) - 1])

#define nk_dynarray_get_capacity(da) ((da)->capacity)

#define nk_dynarray_get_size(da) ((da)->size)

#define nk_dynarray_push(da, elm) ({ \
	_nk_dynarray_resize(da); \
	(da)->data[((da)->size)] = elm; \
	((da)->size)++; \
})

#define nk_dynarray_push_atomic(da, elm) ({ \
	uint8_t flags = spin_lock_irq_save(&((da)->lock)); \
	nk_dynarray_push(da); \
	spin_unlock_irq_restore(&((da)->lock), flags); \
})

#define nk_dynarray_pop(da) ({ \
	__auto_type pop_elm = nk_dynarray_begin(da); \
	if (nk_dynarray_empty(da)) { DYNARRAY_PRINT("dynarray empty\n"); } \
	else { \
		((da)->size)--; \
		pop_elm = ((da)->data[((da)->size)]); \
		(da)->data[((da)->size)] = 0; \
	} \
	pop_elm; \
})

#define nk_dynarray_pop_atomic(da) ({ \
	uint8_t flags = spin_lock_irq_save(&((da)->lock)); \
	__auto_type popped = nk_dynarray_push(da); \
	spin_unlock_irq_restore(&((da)->lock), flags); \
	popped; \
})

#define nk_dynarray_insert(da, elm, idx) ({ \
	if ((idx >= (da)->size) \
		|| (idx < 0)) { \
		DYNARRAY_PRINT("index out of bounds\n"); \
	} \
	else { \
		((da)->size)++; \
		_nk_dynarray_resize(da); \
		memmove(((da)->data + idx + 1), ((da)->data + idx), ((da)->size - idx - 1)); \
		(da)->data[idx] = elm; \
	} \
})

#define nk_dynarray_insert_atomic(da, elm, idx) ({ \
	uint8_t flags = spin_lock_irq_save(&((da)->lock)); \
	nk_dynarray_insert(da, elm, idx); \
	spin_unlock_irq_restore(&((da)->lock), flags); \
})

#define nk_dynarray_erase(da, idx) ({ \
	if ((idx >= (da)->size) \
		|| (idx < 0)) { \
		DYNARRAY_PRINT("index out of bounds\n"); \
	} \
	else { \
		memmove(((da)->data + idx), ((da)->data + idx + 1), ((da)->size - idx)); \
		((da)->size)--; \
		(da)->data[(da)->size] = 0; \
	} \
})
		
#define nk_dynarray_erase_atomic(da, idx) ({ \
	uint8_t flags = spin_lock_irq_save(&((da)->lock)); \
	nk_dynarray_erase(da, idx); \
	spin_unlock_irq_restore(&((da)->lock), flags); \
})

#define nk_dynarray_clear(da) ({ \
	memset((da)->data, 0, (sizeof((da)->data[0]) * ((da)->capacity))); \
	(da)->size = 0; \
})

#define nk_dynarray_clear_atomic(da) ({ \
	uint8_t flags = spin_lock_irq_save(&((da)->lock)); \
	nk_dynarray_clear(da); \
	spin_unlock_irq_restore(&((da)->lock), flags); \
})

#define nk_dynarray_find(da, elm) ({ \
	int i, val, found = 0; \
	nk_dynarray_foreach(da, i, val) { \
		if (val == elm) { found |= 1; break; } \
	} \
	if (!found) { i = -1; } \
	i; \
})

#define nk_dynarray_find_atomic(da, elm) ({ \
	uint8_t flags = spin_lock_irq_save(&((da)->lock)); \
	int index = nk_dynarray_find(da, elm); \
	spin_unlock_irq_restore(&((da)->lock), flags); \
	index; \
})

#define nk_dynarray_foreach(da, iter, val) for (iter = 0; (((iter < (da)->size)) && ((val = (da)->data[iter]) || 1)); iter++)

#define nk_dynarray_reverse(da, iter, val) for (iter = ((da)->size - 1); ((iter >= 0) && ((val = (da)->data[iter]) || 1)); iter--)

#define nk_dynarray_print(da) ({ \
	int i, val; \
	DYNARRAY_PRINT("\ndynarray: \n"); \
	nk_dynarray_foreach(da, i, val) { \
		DYNARRAY_PRINT("%d ", val); \
	} \
	DYNARRAY_PRINT("\n"); \
})

#define nk_dynarray_destroy(da) ({ \
	free((da)->data); \
	free(da); \
})

NK_DYNARRAY_DECL(int);
NK_DYNARRAY_DECL(uint32_t);


#ifdef __cplusplus
}
#endif


#endif
