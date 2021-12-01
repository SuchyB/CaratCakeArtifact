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
 * Copyright (c) 2020, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2017, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */


#ifndef __ALLOC
#define __ALLOC

/*
  The purpose of this interface is to provide a way to have
  custom allocators that depend on the context in which
  the allocation call is made.   For example, to support
  custom allocators for CARAT threads/processes
*/


// An allocator implementation registers itself at compile
// time using the following interface
typedef struct nk_alloc_impl {
    char * impl_name; // name of this type of allocator
    struct nk_alloc * (*create)(char *name);
} nk_alloc_impl_t;

#define nk_alloc_register_impl(impl)			  \
    static struct nk_alloc_impl * _nk_alloc_impl_##impl \
    __attribute__((used))				  \
    __attribute__((unused, __section__(".alloc_impls"),  \
        aligned(sizeof(void*))))                          \
    = &impl;                                              \


typedef uint64_t nk_alloc_flags_t;
// flags
#define NK_ALLOC_ZERO            1   // zero data
//...


// This is the abstract base class for allocators
// it should be the first member of any specific allocator interface
typedef struct nk_alloc_interface {
    int    (*destroy)(void *state);

    // cpu = -1 => any cpu
    void * (*allocp)(void *state, size_t size, size_t align, int cpu, nk_alloc_flags_t flags);
    void * (*reallocp)(void *state, void *ptr, size_t size, size_t align, int cpu, nk_alloc_flags_t flags);
    void   (*freep)(void *state, void *ptr);


    // print out info about the allocator
    int    (*print)(void *state, int detailed);
    
} nk_alloc_interface_t;


typedef struct nk_alloc nk_alloc_t;

#define NK_ALLOC_IMPL_NAME_LEN 32
#define NK_ALLOC_NAME_LEN 32

// default alignment for kmem interface where
// alignment is not otherwise supplied
#define NK_ALLOC_DEFAULT_ALIGNMENT 16

nk_alloc_t *nk_alloc_create(char *impl_name, char *name);
int         nk_alloc_destroy(nk_alloc_t *alloc);

nk_alloc_t *nk_alloc_find(char *name);

// get the current appropriate allocator given the context
// of the caller
nk_alloc_t *nk_alloc_get_associated(void);
// set current appropriate for context
int         nk_alloc_set_associated(nk_alloc_t *alloc);

// Call the appropriate allocator given the context
// this includes the basic system allocator (kmem_malloc)
void *nk_alloc_alloc_extended(nk_alloc_t *alloc, size_t size, size_t align, int cpu, nk_alloc_flags_t flags);
void *nk_alloc_realloc_extended(nk_alloc_t *alloc, void *ptr, size_t size, size_t align, int cpu, nk_alloc_flags_t flags);
void  nk_alloc_free_extended(nk_alloc_t *alloc, void *ptr);

// Call the appropriate allocator given the context
// this includes the basic system allocator (kmem_malloc)
static inline void *nk_alloc_alloc(size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
    return nk_alloc_alloc_extended(nk_alloc_get_associated(),size,align,cpu,flags);
}
static inline void *nk_alloc_realloc(void *ptr, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
    return nk_alloc_realloc_extended(nk_alloc_get_associated(),ptr,size,align,cpu,flags);
}

static inline void nk_alloc_free(void *ptr)
{
    return nk_alloc_free_extended(nk_alloc_get_associated(),ptr);
}

//
// This structure forms the header of any type-specific
// structure.   it should be the first member of any alloc state
//
typedef struct nk_alloc {
    char                      name[NK_ALLOC_NAME_LEN];
    
    uint64_t                  flags;

    void                     *state;
    
    nk_alloc_interface_t     *interface;
    
    struct list_head          alloc_list_node;         // for system-wide allocator list
    
} nk_alloc_t;



// this is invoked by the create() call in the implementation after it has done its local work
nk_alloc_t *nk_alloc_register(char *name, uint64_t flags, nk_alloc_interface_t *interface, void *state);

// this is invoked by the destory() call in the implementation after it has done its local work
int         nk_alloc_unregister(nk_alloc_t *alloc);


int  nk_alloc_init();
int  nk_alloc_init_ap();
void nk_alloc_deinit();

#endif

