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
#include <nautilus/lru_cache.h>
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

#define NUM_RAND 40

static int
handle_lru (char * buf, void * priv)
{
	nk_vc_printf("lru_cache test ...\n");
	lru_cache *cache = build_lru_cache();

	uint64_t *rand_array = gen_rand_array(uint64_t, NUM_RAND, 0), i;
	
	nk_vc_printf("rand array:\n");
	for (i = 0; i < NUM_RAND; i++) { 
		nk_vc_printf("%lu ", rand_array[i]);
	}
    nk_vc_printf("\n");
	
    debug_lru_cache(cache);

	nk_vc_printf("\nadding elements...\n");

	for (i = 0; i < NUM_RAND; i++) {
        add_to_lru_cache(cache, rand_array[i], -1);
	}

	nk_vc_printf("\npost-adding elements...\n");

    debug_lru_cache(cache);

	for (i = 0; i < NUM_RAND; i++) { 
        unsigned val = query_lru_cache(cache, rand_array[i], 0);
        nk_vc_printf("%u : %u   ;   ", rand_array[i], val);
	    val = query_lru_cache(cache, rand_array[i] + 5, 0);
        nk_vc_printf("%u : %u\n", rand_array[i] + 5, val);
    }
    nk_vc_printf("\n");
    
    debug_lru_cache(cache);

    return 0;
}

static struct shell_cmd_impl lru_impl = {
    .cmd      = "lru",
    .help_str = "lru",
    .handler  = handle_lru
};

nk_register_shell_cmd(lru_impl);


