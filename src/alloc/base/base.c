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
 * Copyright (c) 2020, Peter Dinda
 * Copyright (c) 2020, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */


#include <nautilus/nautilus.h>
#include <nautilus/spinlock.h>
#include <nautilus/paging.h>
#include <nautilus/thread.h>
#include <nautilus/shell.h>

#include <nautilus/alloc.h>

#include <nautilus/mm.h>


#ifndef NAUT_CONFIG_DEBUG_ALLOC_BASE
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR(fmt, args...) ERROR_PRINT("alloc-base: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("alloc-base: " fmt, ##args)
#define INFO(fmt, args...)   INFO_PRINT("alloc-base: " fmt, ##args)

// There is only a single base allocator
// and it's a simple pass-through to kmem_malloc
// hence a single global to represent it
static struct nk_alloc_base {
    nk_alloc_t *alloc;
} base_state;


static  int destroy(void *state)
{
    ERROR("Cannot destroy the singular base allocator\n");
    return -1;
}
    
    
static int print(void *state, int detailed)
{
    struct nk_alloc_base *as = (struct nk_alloc_base *)state;
    struct nk_thread *thread = get_cur_thread();

    nk_vc_printf("%s:\n",as->alloc->name);

    uint64_t num = kmem_num_pools();
    struct kmem_stats *s = malloc(sizeof(struct kmem_stats)+num*sizeof(struct buddy_pool_stats));
    uint64_t i;

    if (!s) { 
        nk_vc_printf("Failed to allocate space for mem info\n");
        return 0;
    }

    s->max_pools = num;

    kmem_stats(s);

    if (detailed) { 
	for (i=0;i<s->num_pools;i++) { 
	    nk_vc_printf("pool %lu %p-%p %lu blks free %lu bytes free\n  %lu bytes min %lu bytes max\n", 
			 i,
			 s->pool_stats[i].start_addr,
			 s->pool_stats[i].end_addr,
			 s->pool_stats[i].total_blocks_free,
			 s->pool_stats[i].total_bytes_free,
			 s->pool_stats[i].min_alloc_size,
			 s->pool_stats[i].max_alloc_size);
	}
    }
	
    nk_vc_printf("%lu pools %lu blks free %lu bytes free\n", s->total_num_pools, s->total_blocks_free, s->total_bytes_free);
    nk_vc_printf("  %lu bytes min %lu bytes max\n", s->min_alloc_size, s->max_alloc_size);

    free(s);

    return 0;
    
}    

static void * impl_alloc(void *state, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
    struct nk_alloc_base *as = (struct nk_alloc_base *) state;

    DEBUG("%s: alloc size %lu align %lu cpu %d flags=%lx\n",as->alloc->name,size,align,cpu,flags);

    // kmem_malloc will always align to size, unless size is truly gigantic
    void *ret = kmem_sys_malloc_specific(size,cpu,flags&NK_ALLOC_ZERO);

    DEBUG("%s: returning %p\n",as->alloc->name,ret);

    return ret;
    
}

static void * impl_realloc(void *state, void *ptr, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
    struct nk_alloc_base *as = (struct nk_alloc_base *) state;

    DEBUG("%s: realloc %p size %lu align %lu cpu %d flags=%lx\n",as->alloc->name,ptr,size,align,cpu,flags);

    // kmem_realloc will always align to size, unless size is gigantic
    // note that kmem_realloc is pretty dumb in that it does not obey
    // CPU, etc.
    void *ret=kmem_sys_realloc_specific(ptr,size,cpu);

    DEBUG("%s: returning %p\n",as->alloc->name,ret);

    return ret;

}

static void impl_free(void *state, void *ptr)
{
    struct nk_alloc_base *as = (struct nk_alloc_base *) state;

    DEBUG("%s: free %p\n",as->alloc->name,ptr);

    kmem_sys_free(ptr);
}
    

static nk_alloc_interface_t base_interface = {
    .destroy = destroy,
    .allocp = impl_alloc,
    .reallocp = impl_realloc,
    .freep = impl_free,
    .print = print
};

static struct nk_alloc * create(char *name)
{
    ERROR("failed to create new alloc - only one base allocator can exist\n");
    return 0;
}


static nk_alloc_impl_t base = {
				.impl_name = "base",
				.create = create,
};



//
// The base allocator is special in that there is exactly one
// and it is system allocator

int nk_alloc_base_init()
{
    struct naut_info *info = nk_get_nautilus_info();
    
    memset(&base_state,0,sizeof(base_state));

    base_state.alloc = nk_alloc_register("base",0,&base_interface, &base_state);

    if (!base_state.alloc) {
	ERROR("Unable to register base allocator\n");
	return -1;
    }

    DEBUG("Base allocator configured and initialized\n");
    
    return 0;
}

nk_alloc_register_impl(base);





