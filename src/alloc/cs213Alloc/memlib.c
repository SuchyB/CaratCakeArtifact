/*
 *  * memlib.c - a module that simulates the memory system.  Needed
 *   * because it allows us to interleave calls from the student's malloc
 *    * package with the system's malloc package in libc.
 *     *
 *      */

#include <nautilus/nautilus.h>
#include <nautilus/errno.h>
#include <nautilus/paging.h>
/**
 *  original standard libraray inclusion
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
*/

// #include "csapp.h"
#include "memlib.h"

#ifndef NAUT_CONFIG_DEBUG_ALLOC_CS213
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define MEMLIB_ERROR(fmt, args...) ERROR_PRINT("alloc-cs213-memlib: " fmt, ##args)
#define MEMLIB_DEBUG(fmt, args...) DEBUG_PRINT("alloc-cs213-memlib: " fmt, ##args)
#define MEMLIB_INFO(fmt, args...)   INFO_PRINT("alloc-cs213-memlib: " fmt, ##args)


#define MAX_HEAP (20*(1<<20))  /* 20 MB */


/* $begin memlib */
/* Private global variables */
static char *mem_heap;     /* Points to first byte of heap */ 
static char *mem_brk;      /* Points to last byte of heap plus 1 */
static char *mem_max_addr; /* Max legal heap addr plus 1*/ 

/* 
 *  * mem_init - Initialize the memory system model
 *   */
void mem_init(void)
{
  mem_heap = (char *)malloc(MAX_HEAP);
  mem_brk = (char *)mem_heap;               
  mem_max_addr = (char *)(mem_heap + MAX_HEAP); 
}

/* 
 *  * mem_sbrk - Simple model of the sbrk function. Extends the heap 
 *   *    by incr bytes and returns the start address of the new area. In
 *    *    this model, the heap cannot be shrunk.
 *     */
void *mem_sbrk(int incr) 
{
  char *old_brk = mem_brk;

  if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
    // errno = ENOMEM;
    // fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
    MEMLIB_ERROR("ERROR: mem_sbrk failed. Ran out of memory...\n");
    return (void *)-1;
  }
  mem_brk += incr;
  return (void *)old_brk;
}
/* $end memlib */

/* 
 *  * mem_deinit - free the storage used by the memory system model
 *   */
void mem_deinit(void)
{
}

/*
 *  * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 *   */
void mem_reset_brk()
{
  mem_brk = (char *)mem_heap;
}

/*
 *  * mem_heap_lo - return address of the first heap byte
 *   */
void *mem_heap_lo()
{
  return (void *)mem_heap;
}

/* 
 *  * mem_heap_hi - return address of last heap byte
 *   */
void *mem_heap_hi()
{
  return (void *)(mem_brk - 1);
}

/*
 *  * mem_heapsize() - returns the heap size in bytes
 *   */
size_t mem_heapsize() 
{
  return (size_t)((void *)mem_brk - (void *)mem_heap);
}

/*
 *  * mem_pagesize() - returns the page size of the system
 *   */
size_t mem_pagesize()
{
  /**
     *  TODO: getpagesize for KARAT and PAGING 
     *  GetPageSize system call. We don't have it right now.
     **/
  // return (size_t)getpagesize();
  return nk_paging_default_page_size();
}
