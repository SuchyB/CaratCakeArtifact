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
 * Copyright (c) 2019, Souradip Ghosh <sgh@u.northwestern.edu>
 * Copyright (c) 2019, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Souradip Ghosh <sgh@u.northwestern.edu> 
 *  
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include <nautilus/nautilus.h>
#include <nautilus/thread.h>
#include <nautilus/scheduler.h>
#include <nautilus/shell.h>
#include <nautilus/vc.h>
#include <nautilus/skiplist.h>

#define gen_rand_array(type, len, neg) ({ \
	SEED; \
	type *rand_array = (type *) (SLIST_MALLOC(sizeof(type) * len)); \
	size_t i; \
	for (i = 0; i < len; i++) \
	{ \
		type num = (type) (lrand48()), \
	   		 sign = ((num % 2) || (!(neg))) ? 1 : -1; \
		rand_array[i] = num * sign; \
	} \
	rand_array; \
})

#define print_map(sl) ({ \
	int i; \
	nk_vc_printf("\n"); \
	for (i = 0; i < 1; i++) \
	{ \
		/* nk_vc_printf("gear %d: ", i); */ \
		int j = 0; \
		__auto_type *iterator = sl->all_left[i]; \
		while (iterator != NULL) \
		{ \
			nk_vc_printf("%d : %d\n", iterator->data->first, iterator->data->second); \
			iterator = iterator->succ_nodes[i]; \
			j++; \
		} \
		\
		nk_vc_printf("\nTOTAL: %d\n\n", j); \
	} \
})

#define NUM_RAND 10
#define TOP_GEAR 12 /* (NUM_RAND / 12) */ 

static int
handle_map (char * buf, void * priv)
{
	nk_vc_printf("map test ...\n");
	nk_slist_int_int *the_map = nk_map_build(int, int);

	int *keys = gen_rand_array(int, NUM_RAND, 1),
		*values = gen_rand_array(int, NUM_RAND, 1),
		i;

	nk_vc_printf("rand array --- keys:\n");
	for (i = 0; i < NUM_RAND; i++) { 
		nk_vc_printf("%d ", keys[i]);
	}

	nk_vc_printf("\n\nrand array --- values:\n");
	for (i = 0; i < NUM_RAND; i++) { 
		nk_vc_printf("%d ", values[i]);
	}
		
	print_map(the_map);

	nk_vc_printf("\nadding elements...\n");

	for (i = 0; i < NUM_RAND; i++) {
		nk_map_insert(the_map, int, int, keys[i], values[i]);
	}	

	nk_vc_printf("\nfinding elements...\n");

	for (i = 0; i < NUM_RAND; i++) {
		__auto_type *found = nk_map_find(the_map, int, int, keys[i]);
		if (!found) { nk_vc_printf("can't find node: %d\n", keys[i]); continue; }
		nk_vc_printf("%d : %d\n", found->data->first, found->data->second);
	}	

	
	nk_vc_printf("\npost-adding elements...\n");

	print_map(the_map);

	for (i = 0; i < NUM_RAND; i++) {
		__auto_type *prospective = nk_map_better_lower_bound(the_map, int, int, keys[i]);
		nk_vc_printf("Query: %d, Result: (%d, %d) \n", keys[i], prospective->data->first, prospective->data->second);

	}
	
	nk_vc_printf("\nremoving elements...\n");

	for (i = 0; i < NUM_RAND; i++) {
		nk_map_remove(the_map, int, int, keys[i]);
	}	
	
	nk_vc_printf("\npost-removing elements...\n");

	print_map(the_map);

    return 0;
}

static struct shell_cmd_impl map_impl = {
    .cmd      = "map",
    .help_str = "map",
    .handler  = handle_map
};

nk_register_shell_cmd(map_impl);


