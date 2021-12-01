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
#include <nautilus/naut_string.h>


#include "memlib.h"



#ifndef NAUT_CONFIG_DEBUG_ALLOC_CS213
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define ALLOC_ERROR(fmt, args...) ERROR_PRINT("alloc-cs213: " fmt"\n", ##args)
#define ALLOC_DEBUG(fmt, args...) DEBUG_PRINT("alloc-cs213: " fmt"\n", ##args)
#define ALLOC_INFO(fmt, args...)   INFO_PRINT("alloc-cs213: " fmt"\n", ##args)


/*
 *  * If NEXT_FIT defined use next fit search, else use first-fit search
 *   */
#define NEXT_FITx

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<24)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp
/* $end mallocmacros */




// this is an implicit free list allocator
struct nk_alloc_cs213 {
  nk_alloc_t *alloc;
  /* Global variables */
  char *heap_listp;  /* Pointer to first block */
#ifdef NEXT_FIT
  char *rover;           /* Next fit rover */
#endif


} ;

static void *extend_heap(void* state, size_t words);
static void place(void* state, void *bp, size_t asize);
static void *find_fit(void* state, size_t asize);
static void *coalesce(void* state, void *bp);
static void printblock(void* state, void *bp);





static int print(void *state, int detailed)
{
return 0;
}

/* $end mmfree */
/*
 *  * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 *   */
/* $begin mmfree */
static void *coalesce(void* state, void *bp)
{
  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) {            /* Case 1 */
    return bp;
  }

  else if (prev_alloc && !next_alloc) {      /* Case 2 */
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size,0));
  }

  else if (!prev_alloc && next_alloc) {      /* Case 3 */
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  else {                                     /* Case 4 */
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
      GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  /* $end mmfree */
#ifdef NEXT_FIT
  /* Make sure the as->rover isn't pointing into the free block */
  /* that we just coalesced */
  if ((as->rover > (char *)bp) && (as->rover < NEXT_BLKP(bp)))
    as->rover = bp;
#endif
  /* $begin mmfree */
  return bp;
}

/*
 *  * extend_heap - Extend heap with free block and return its block pointer
 *   */
/* $begin mmextendheap */
static void *extend_heap(void *state, size_t words)
{
  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //line:vm:mm:beginextend
 /* if ((long)(bp = mem_sbrk(size)) == -1)
    return NULL; */  //line:vm:mm:endextend

  bp=kmem_sys_malloc_specific(size, my_cpu_id(), 1);
  if(!bp){
  return bp;
  }
  /* Initialize free block header/footer and the epilogue header */
  PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
  PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr

  /* Coalesce if the previous block was free */
//  return coalesce(state, bp);                                          //line:vm:mm:returnblock
    as->heap_listp = bp;
    return bp;
}
/* $end mmextendheap */

/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void* state, void *bp, size_t asize)
  /* $end mmplace-proto */
{
  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;

  size_t csize = GET_SIZE(HDRP(bp));

  if ((csize - asize) >= (2*DSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  }
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}
/* $end mmplace */

/* $begin mmfirstfit */
/* $begin mmfirstfit-proto */
static void *find_fit(void* state, size_t asize)
  /* $end mmfirstfit-proto */
{
  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;
  /* $end mmfirstfit */

#ifdef NEXT_FIT
  /* Next fit search */
  char *oldrover = as->rover;

  /* Search from the as->rover to the end of list */
  for ( ; GET_SIZE(HDRP(as->rover)) > 0; as->rover = NEXT_BLKP(as->rover))
    if (!GET_ALLOC(HDRP(as->rover)) && (asize <= GET_SIZE(HDRP(as->rover))))
      return as->rover;

  /* search from start of list to old as->rover */
  for (as->rover = as->heap_listp; as->rover < oldrover; as->rover = NEXT_BLKP(as->rover))
    if (!GET_ALLOC(HDRP(as->rover)) && (asize <= GET_SIZE(HDRP(as->rover))))
      return as->rover;

  return NULL;  /* no fit found */
#else
  /* $begin mmfirstfit */
  /* First-fit search */
  void *bp;

  for (bp = as->heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
      return bp;
    }
  }
  return NULL; /* No fit */
#endif
}
/* $end mmfirstfit */

static void printblock(void* state, void *bp)
{

  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;
  size_t hsize, halloc, fsize, falloc;

  hsize = GET_SIZE(HDRP(bp));
  halloc = GET_ALLOC(HDRP(bp));
  fsize = GET_SIZE(FTRP(bp));
  falloc = GET_ALLOC(FTRP(bp));

  if (hsize == 0) {
    ALLOC_INFO("%p: EOL\n", bp);
    return;
  }

  ALLOC_INFO("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp,
      hsize, (halloc ? 'a' : 'f'),
      fsize, (falloc ? 'a' : 'f'));
}

static int mm_init(void *state){
  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;
  as->heap_listp = kmem_sys_malloc_specific(4*WSIZE,my_cpu_id(),1);
  if(!as->heap_listp){
  	return -1;
  }
  /* Create the initial empty heap */
  /*if ((as->heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) //line:vm:mm:begininit
    return -1;
    */
  PUT(as->heap_listp, 0);                          /* Alignment padding */
  PUT(as->heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
  PUT(as->heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
  PUT(as->heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
  as->heap_listp += (2*WSIZE);                     //line:vm:mm:endinit
  /* $end mminit */

#ifdef NEXT_FIT
  as->rover = as->heap_listp;
#endif
  /* $begin mminit */

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(state, CHUNKSIZE/WSIZE) == NULL)
    return -1;
  return 0;
}



__attribute__((noinline))
static void * __impl_alloc(void *state, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;

  size_t asize;      /* Adjusted block size */
  size_t extendsize; /* Amount to extend heap if no fit */
  char *bp;

  /* $end mmmalloc */
  if (as->heap_listp == 0){
    mm_init(state);
  }
  /* $begin mmmalloc */
  /* Ignore spurious requests */
  if (size == 0)
    return NULL;

  /* Adjust block size to include overhead and alignment reqs. */
  if (size <= DSIZE)                                          //line:vm:mm:sizeadjust1
    asize = 2*DSIZE;                                        //line:vm:mm:sizeadjust2
  else
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); //line:vm:mm:sizeadjust3

  /* Search the free list for a fit */
  if ((bp = find_fit(state, asize)) != NULL) {  //line:vm:mm:findfitcall
    place(state, bp, asize);                  //line:vm:mm:findfitplace
    ALLOC_DEBUG("Allocated pointer %p with size %d and asize %d",bp, size, asize);
    return bp;
  }

  /* No fit found. Get more memory and place the block */
  extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
  if ((bp = extend_heap(state, extendsize/WSIZE)) == NULL)
    return NULL;                                  //line:vm:mm:growheap2
  place(state, bp, asize);                                 //line:vm:mm:growheap3
  ALLOC_DEBUG("We extended the heap -- Allocated pointer %p with size %d and asize %d",bp, size, asize); 
  return bp;

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


__attribute__((noinline))
static void __impl_free(void *state, void *ptr)
{

  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;
  /* $end mmfree */
  if (ptr == 0)
    return;

  /* $begin mmfree */
  size_t size = GET_SIZE(HDRP(ptr));
  /* $end mmfree */
  if (as->heap_listp == 0){
    mm_init(state);
  }
  /* $begin mmfree */
  ALLOC_DEBUG("Freed pointer at %p with size %d",ptr, size);
  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(state, ptr);

}


/*
 * An indirection/wrapper to impl_free designed for
 * compiler instrumentation purposes
 */ 
static void impl_free(void *state, void *ptr)
{
    return __impl_free(
        state,
        ptr 
    );
}



static void * impl_realloc(void *state, void *ptr, size_t size, size_t align, int cpu, nk_alloc_flags_t flags)
{
  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;
  ALLOC_DEBUG("state: %p, ptr: %p, size: %lx, align: %lx, cpu: %d, flags: %lx",state, ptr, size, align, cpu, flags );
  size_t oldsize;
  void *newptr;

  /* If size == 0 then this is just free, and we return NULL. */
  if(size == 0) {
    impl_free(state, ptr);
    return 0;
  }

  /* If oldptr is NULL, then this is just malloc. */
  if(ptr == NULL) {
    return impl_alloc(state, size, align, cpu, flags);
  }

  newptr = impl_alloc(state, size, align, cpu, flags);

  /* If realloc() fails the original block is left untouched  */
  if(!newptr) {
    return 0;
  }

  /* Copy the old data. */
  oldsize = GET_SIZE(HDRP(ptr));
  ALLOC_DEBUG("Realloced pointer %p with oldsize: %d", ptr, oldsize);
  if(size < oldsize) oldsize = size;
  memcpy(newptr, ptr, oldsize);

  /* Free the old block. */
  impl_free(state, ptr);

  return newptr;

}



static  int destroy(void *state)
{
  struct nk_alloc_cs213 *as = (struct nk_alloc_cs213 *)state;
  ALLOC_DEBUG("%s: destroy - note all memory leaked...\n",as->alloc->name);

  kmem_sys_free(state);  // should call the lowest-level
  // note that we leak everything else

  return 0;
}


static nk_alloc_interface_t cs213_interface = {
  .destroy = destroy,
  .allocp = impl_alloc,
  .reallocp = impl_realloc,
  .freep = impl_free,
  .print = print
};

static struct nk_alloc * create(char *name)
{
  ALLOC_DEBUG("create allocator %s\n",name);

  struct nk_alloc_cs213 *as = kmem_sys_malloc_specific(sizeof(*as),my_cpu_id(),1);

  if (!as) {
    ALLOC_ERROR("unable to allocate allocator state for %s\n",name);
    return 0;
  }
  as->alloc = nk_alloc_register(name,0,&cs213_interface, as);

  if (!as->alloc) {
    ALLOC_ERROR("Unable to register allocator %s\n",name);
    kmem_sys_free(as);
    return 0;
  }

  ALLOC_DEBUG("allocator %s configured and initialized\n", as->alloc->name);



  return as->alloc;
}



static nk_alloc_impl_t cs213 = {
  .impl_name = "cs213",
  .create = create,
};



nk_alloc_register_impl(cs213);
