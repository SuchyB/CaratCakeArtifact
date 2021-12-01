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
#include <nautilus/idt.h>
#include <nautilus/alloc.h>

#ifndef NAUT_CONFIG_DEBUG_ALLOCS
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR(fmt, args...) ERROR_PRINT("alloc: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("alloc: " fmt, ##args)
#define INFO(fmt, args...)   INFO_PRINT("alloc: " fmt, ##args)



static spinlock_t state_lock;
static struct list_head alloc_list;

static int enabled=0;

#define STATE_LOCK_CONF uint8_t _state_lock_flags
#define STATE_LOCK() _state_lock_flags = spin_lock_irq_save(&state_lock)
#define STATE_UNLOCK() spin_unlock_irq_restore(&state_lock, _state_lock_flags);


nk_alloc_t *nk_alloc_register(char *name, uint64_t flags, nk_alloc_interface_t *interface, void *state)
{
    nk_alloc_t *a;

    a = malloc(sizeof(*a));

    if (!a) {
	ERROR("cannot allocate alloc\n");
	return 0;
    }

    memset(a,0,sizeof(*a));

    a->flags = flags;
    strncpy(a->name,name,NK_ALLOC_NAME_LEN); a->name[NK_ALLOC_NAME_LEN-1]=0;
    a->state = state;
    a->interface = interface;

    INIT_LIST_HEAD(&a->alloc_list_node);

    STATE_LOCK_CONF;
    
    STATE_LOCK();
    list_add_tail(&a->alloc_list_node, &alloc_list);
    STATE_UNLOCK();

    return a;
}


int nk_alloc_unregister(nk_alloc_t *a)
{
    STATE_LOCK_CONF;
    
    STATE_LOCK();
    list_del_init(&a->alloc_list_node);
    STATE_UNLOCK();

    free(a);
    
    return 0;
}

static nk_alloc_impl_t *find_impl(char *impl_name)
{
    extern nk_alloc_impl_t *__start_alloc_impls;
    extern nk_alloc_impl_t * __stop_alloc_impls;
    nk_alloc_impl_t **cur;

    for (cur = &__start_alloc_impls;
	 cur && cur!=&__stop_alloc_impls && *cur;
	 cur++) {
	if (!strcmp((*cur)->impl_name,impl_name)) {
	    return *cur;
	}
    }
    return 0;
}


nk_alloc_t *nk_alloc_create(char *impl_name, char *name)
{
    nk_alloc_impl_t *impl = find_impl(impl_name);

    if (impl && impl->create) {
	return impl->create(name);
    } else {
	return 0;
    }
}

#define BOILERPLATE_LEAVE(a,f,args...)                 \
    if (enabled && a->interface && a->interface->f) {             \
       return a->interface->f(a->state, ##args);       \
    } else {                                           \
       return -1;                                      \
    }

#define BOILERPLATE_LEAVE_PTR(a,f,args...)             \
    if (enabled && a->interface && a->interface->f) {             \
       return a->interface->f(a->state, ##args);       \
    } else {                                           \
       return 0;                                      \
    }

#define BOILERPLATE_DO(a,f,args...)		  \
    if (enabled && a->interface && a->interface->f) {        \
       a->interface->f(a->state, ##args);         \
    } 

#define AS_NAME(a) ((a) ? (a)->name : "default")

int  nk_alloc_destroy(nk_alloc_t *alloc)
{
    BOILERPLATE_LEAVE(alloc,destroy)
}
    
nk_alloc_t *nk_alloc_find(char *name)
{
    struct list_head *cur;
    nk_alloc_t  *target=0;
    
    STATE_LOCK_CONF;
    STATE_LOCK();
    list_for_each(cur,&alloc_list) {
	if (!strcmp(list_entry(cur,struct nk_alloc,alloc_list_node)->name,name)) { 
	    target = list_entry(cur,struct nk_alloc, alloc_list_node);
	    break;
	}
    }
    STATE_UNLOCK();
    DEBUG("search for %s finds %p\n",name,target);
    return target;
}


nk_alloc_t *nk_alloc_get_associated(void)
{
    if (!enabled) {
	return 0;
    }
    
    nk_alloc_t *ret = 0;
    
    uint8_t flags = irq_disable_save();

    if (in_interrupt_context()) {
	ret = 0;  // use base
    } else {
	nk_thread_t *th = get_cur_thread();
	if (th) {
	    ret = th->alloc;
	} else {
	    ret = 0; // threads not active yet
	}
    }

    irq_enable_restore(flags);

    return ret;
}

int nk_alloc_set_associated(nk_alloc_t *alloc)
{
    // note that we assume the caller knows how to handle
    // the allocator we just orphaned
    if (!enabled) {
	return -1;
    }
    
    get_cur_thread()->alloc = alloc;

    return 0;
}

void *nk_alloc_alloc_extended(nk_alloc_t *alloc, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
    BOILERPLATE_LEAVE_PTR(alloc,allocp,size,align,cpu,flags);
}
    
void *nk_alloc_realloc_extended(nk_alloc_t *alloc, void *ptr, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
    BOILERPLATE_LEAVE_PTR(alloc,reallocp,ptr,size,align,cpu,flags);
}

void  nk_alloc_free_extended(nk_alloc_t *alloc, void *ptr)
{
    BOILERPLATE_DO(alloc,freep,ptr);
}

int nk_alloc_init()
{
    INIT_LIST_HEAD(&alloc_list);
    spinlock_init(&state_lock);

    int nk_alloc_base_init(void);
    
    if (nk_alloc_base_init()) {
	ERROR("failed to initialized base allocator\n");
	return -1;
    }

    DEBUG("allocator framework initialized\n");

    enabled = 1;
    
    return 0;
}

int nk_alloc_init_ap()
{
    INFO("inited\n");
    return 0;
}


int nk_alloc_dump_allocs(int detail)
{
    struct list_head *cur;
    STATE_LOCK_CONF;
    STATE_LOCK();
    list_for_each(cur,&alloc_list) {
	struct nk_alloc *a = list_entry(cur,struct nk_alloc, alloc_list_node);
	BOILERPLATE_DO(a,print,detail);
    }
    STATE_UNLOCK();
    return 0;
}


static int handle_allocs (char * buf, void * priv)
{
    int detail = 0;
    if (strstr(buf,"d")) {
	detail = 1;
    }
    nk_alloc_dump_allocs(detail);
    return 0;
}


static struct shell_cmd_impl allocs_impl = {
    .cmd      = "allocs",
    .help_str = "allocs [detail]",
    .handler  = handle_allocs,
};
nk_register_shell_cmd(allocs_impl);

int nk_alloc_dump_alloc_impls()
{
    extern nk_alloc_impl_t *__start_alloc_impls;
    extern nk_alloc_impl_t * __stop_alloc_impls;
    nk_alloc_impl_t **cur;

    for (cur = &__start_alloc_impls;
	 cur && cur!=&__stop_alloc_impls && *cur;
	 cur++) {
	nk_vc_printf("%s\n",(*cur)->impl_name);
    }
    return 0;
}


static int handle_allocis (char * buf, void * priv)
{
    nk_alloc_dump_alloc_impls();
    return 0;
}


static struct shell_cmd_impl allocis_impl = {
    .cmd      = "allocis",
    .help_str = "allocis",
    .handler  = handle_allocis,
};
nk_register_shell_cmd(allocis_impl);
