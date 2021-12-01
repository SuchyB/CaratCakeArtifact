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
#include <nautilus/aspace.h>

#include <nautilus/mm.h>


#ifndef NAUT_CONFIG_DEBUG_ALLOC_DUMB
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR(fmt, args...) ERROR_PRINT("alloc-dumb: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("alloc-dumb: " fmt, ##args)
#define INFO(fmt, args...)   INFO_PRINT("alloc-dumb: " fmt, ##args)

//#define BASE_BLOCK_SIZE (16*1024*1024)
#define BASE_BLOCK_SIZE (128*4096)

// this is an expand-only allocator
// whenever it allocates a new block to work on
// it simply leaks the previous one
struct nk_alloc_dumb {
  nk_alloc_t *alloc;

  addr_t block_start;
  addr_t block_end;
  addr_t block_cur;

  uint64_t block_count;
  uint64_t byte_count;

  nk_aspace_region_t* region;
} ;


static  int destroy(void *state)
{
  struct nk_alloc_dumb *as = (struct nk_alloc_dumb *)state;
  DEBUG("%s: destroy - note all memory leaked...\n",as->alloc->name);

  kmem_sys_free(state);  // should call the lowest-level
  // note that we leak everything else

  return 0;
}


static int print(void *state, int detailed)
{
  struct nk_alloc_dumb *as = (struct nk_alloc_dumb *)state;
  struct nk_thread *thread = get_cur_thread();

  nk_vc_printf("%s:\n",as->alloc->name);

  nk_vc_printf("block_start=%p block_end=%p block_cur=%p block_count=%lu byte_count=%lu\n", as->block_start, as->block_end, as->block_cur, as->block_count,as->byte_count);

  return 0;

}


static inline addr_t round_up_align(addr_t p, size_t a)
{
  return p%a ? p+a - p%a : p;
}

static inline uint64_t next_pow2(uint64_t n)
{
  return 0x1 << (64-(__builtin_clzl(n)+1));
}

#define MAX(x,y) ((x)>(y) ? (x) : (y))

__attribute__((noinline))
static void * __impl_alloc(void *state, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
  addr_t ret;
  struct nk_alloc_dumb *as = (struct nk_alloc_dumb *) state;

  enum {Initial=0, Inplace, Expand, Leak} alloc_state = Initial;

 retry:
  
  DEBUG("%s: alloc alloc_state %s size %lu align %lu cpu %d flags=%lx\n",
        as->alloc->name,
        alloc_state == Initial ? "initial" :
        alloc_state == Inplace ? "inplace" :
        alloc_state == Expand  ? "expand" :
        alloc_state == Leak ? "leak" : "WTF?!",
        size,align,cpu,flags);

  if ((round_up_align(as->block_cur,align) + size)<as->block_end) {
    // fits
    DEBUG("satisfied in current block\n");
    ret = round_up_align(as->block_cur,align);
    as->block_cur = ret + size;

    if (flags & NK_ALLOC_ZERO) {
      memset((void*)ret,0,size);
    }
    
    DEBUG("%s: returning %p\n",as->alloc->name,(void*)ret);
    
    return (void*)ret;
  }

  alloc_state++; /// not good

  switch (alloc_state) {
  case Inplace:
  case Expand: {
    //First let us try to defragment the block with no size change
    
    nk_aspace_region_t oldreg = *as->region;
    void *new_free;
    void *block_cur;
    // double size if expanding
    uint64_t new = alloc_state == Inplace ? as->region->len_bytes : 2*(as->region->len_bytes);

    nk_alloc_t *curalloc = nk_alloc_get_associated();

    // switch to using the system allocator temporarily

    nk_alloc_set_associated(0);

    if (nk_aspace_defrag_region(get_cur_thread()->aspace,
                                as->region,
                                as->region->len_bytes,
                                &block_cur)) {
      nk_alloc_set_associated(curalloc);
      ERROR("failed to defragment region via %s!\n", alloc_state == Inplace ? "in place" : "expanding");
      return 0;
    } else {
      as->block_start = (addr_t) as->region->va_start;
      as->block_end = (addr_t) as->region->va_start + as->region->len_bytes;
      as->block_cur = (addr_t) block_cur;
      // what if oldregion == new region?
      kmem_sys_free(oldreg.va_start);
      nk_alloc_set_associated(curalloc);
      goto retry;
    }
    break;
  }

  case Leak: {
    uint64_t chunk_size = next_pow2(MAX(size * 2,BASE_BLOCK_SIZE));

    DEBUG("allocating new block of size %lu\n",chunk_size);
    
    // Create a new arena/region...
    void *new = kmem_sys_malloc_specific(chunk_size,cpu,0);
    
    if (!new) {
      ERROR("Failed to allocate a new block\n");
      return 0;
    }
    
    //This is why we should probably merge arenas and regions
    as->region->va_start = new;
    as->region->pa_start = new;
    as->region->len_bytes = chunk_size;
    as->region->requested_permissions = 0;
    
    nk_alloc_t *curalloc = nk_alloc_get_associated();

    // switch to using the system allocator temporarily

    nk_alloc_set_associated(0);

    if (nk_aspace_add_region(get_cur_thread()->aspace, as->region)) {
      ERROR("failed to add new region on leaking expansion\n");
      nk_alloc_set_associated(curalloc);
      return 0;
    }

    as->block_start= (addr_t) new;
    as->block_end = as->block_start + chunk_size;
    as->block_cur = as->block_start;

    nk_alloc_set_associated(curalloc);
    
    goto retry;
  }

  default:
    ERROR("allocator in unknown state %d\n",alloc_state);
    return 0;
  }


}


/*
 * An indirection/wrapper to impl_alloc designed for
 * compiler instrumentation purposes
 */ 
static void * impl_alloc(void *state, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
    return __impl_alloc(
        state,
        size,
        align,
        cpu,
        flags
    );
}


static void * impl_realloc(void *state, void *ptr, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
  struct nk_alloc_dumb *as = (struct nk_alloc_dumb *) state;

  DEBUG("%s: realloc %p size %lu align %lu cpu %d flags=%lx\n",as->alloc->name,ptr,size,align,cpu,flags);

  void *ret = impl_alloc(state,size,align,cpu,flags);

  if (!ret) {
    ERROR("failed to allocate\n");
    return 0;
  }

  DEBUG("attempting copy.... this is likely to be bogus\n");
  // we do not really know the size of the old block, so this will end badly...
  memcpy(ret,ptr,size);

  DEBUG("%s: returning %p\n",as->alloc->name,ret);

  return ret;
}


__attribute__((noinline))
static void __impl_free(void *state, void *ptr)
{
  struct nk_alloc_dumb *as = (struct nk_alloc_dumb *) state;

  DEBUG("%s: free %p ignored\n",as->alloc->name,ptr);

}

/*
 * An indirection/wrapper to impl_free designed for
 * compiler instrumentation purposes
 */ 
static void impl_free(void *state, void *ptr)
{
    __impl_free(
        state,
        ptr 
    );
}


static nk_alloc_interface_t dumb_interface = {
  .destroy = destroy,
  .allocp = impl_alloc,
  .reallocp = impl_realloc,
  .freep = impl_free,
  .print = print
};

static struct nk_alloc * create(char *name)
{
  DEBUG("create allocator %s\n",name);

  struct nk_alloc_dumb *as = kmem_sys_malloc_specific(sizeof(*as),my_cpu_id(),1);

  if (!as) {
    ERROR("unable to allocate allocator state for %s\n",name);
    return 0;
  }

  // note that block_start=block_end=block_cur, so first allocation
  // will trigger a system allocation

  as->alloc = nk_alloc_register(name,0,&dumb_interface, as);

  if (!as->alloc) {
    ERROR("Unable to register allocator %s\n",name);
    kmem_sys_free(as);
    return 0;
  }

  DEBUG("allocator %s configured and initialized\n", as->alloc->name);

 
  uint64_t chunk_size = next_pow2(BASE_BLOCK_SIZE);
  void *new =  kmem_sys_malloc_specific(chunk_size,my_cpu_id(),1);
 
  as->region = kmem_sys_malloc_specific(sizeof(nk_aspace_region_t), my_cpu_id(), 1);
  as->region->protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE ; 
  as->region->va_start = new;
  as->region->pa_start = new;
  as->region->len_bytes = chunk_size;
  as->region->requested_permissions = 0;

  if (nk_aspace_add_region(get_cur_thread()->aspace, as->region)){
    ERROR("Failed to add region\n");
    kmem_sys_free(as->region);
    kmem_sys_free(new);
    return 0;
  }      
  

  as->block_start= (addr_t)new;
  as->block_cur = as->block_start;
  as->block_end = as->block_start + chunk_size;
  as->block_count++;
  as->byte_count+=chunk_size;


  return as->alloc;
}


static nk_alloc_impl_t dumb = {
  .impl_name = "dumb",
  .create = create,
};



nk_alloc_register_impl(dumb);





