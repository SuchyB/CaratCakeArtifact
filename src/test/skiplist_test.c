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

#define print_sl(sl) ({ \
	int i; \
	nk_vc_printf("\n"); \
	for (i = 0; i < sl->top_gear; i++) \
	{ \
		nk_vc_printf("gear %d: ", i); \
		int j = 0; \
		__auto_type *iterator = sl->all_left[i]; \
		while (iterator != NULL) \
		{ \
			nk_vc_printf("%d ", iterator->data); \
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
handle_sl (char * buf, void * priv)
{
	nk_vc_printf("skiplist test ...\n");
	nk_slist_int *the_list = nk_slist_build(int, TOP_GEAR);

	int *rand_array = gen_rand_array(int, NUM_RAND, 1), i;
	
	nk_vc_printf("rand array:\n");
	for (i = 0; i < NUM_RAND; i++) { 
		nk_vc_printf("%d ", rand_array[i]);
	}
	
	print_sl(the_list);
	
	nk_vc_printf("\nadding elements...\n");

	for (i = 0; i < NUM_RAND; i++) {
		nk_slist_add(int, the_list, rand_array[i]);
	}

	nk_vc_printf("\npost-adding elements...\n");

	nk_vc_printf("\nfinding elements...\n");

	for (i = 0; i < NUM_RAND; i++) {
		__auto_type *found = nk_slist_find(int, the_list, rand_array[i]);
		if (!found) { nk_vc_printf("can't find node: %d\n", rand_array[i]); continue; }
		nk_vc_printf("%d\n", found->data);
	}	

	print_sl(the_list);

	for (i = 0; i < NUM_RAND; i++) {
		__auto_type *prospective = nk_slist_better_lower_bound(int, the_list, rand_array[i] + 2);
		nk_vc_printf("Query: %d, Result: %d \n", rand_array[i] + 2, prospective->data);

	}

	nk_slist_node_int *iter1;
	int val1;

	/*	
	for (iter = the_list->all_left[0], val = iter->data; iter != NULL; iter = iter->succ_nodes[0], val = iter->data) {
		nk_vc_printf("-> %d\n", val);	
	}
	*/
	
	nk_slist_foreach(the_list, val1, iter1) {
		nk_vc_printf("-> %d\n", val1); 
	}

	nk_vc_printf("\nremoving elements...\n");

	for (i = 0; i < NUM_RAND; i++) {
		nk_slist_remove(int, the_list, rand_array[i]);
	}	
	
	nk_vc_printf("\npost-removing elements...\n");

	print_sl(the_list);

    return 0;
}

static struct shell_cmd_impl slist_impl = {
    .cmd      = "slist",
    .help_str = "slist",
    .handler  = handle_sl
};

nk_register_shell_cmd(slist_impl);


