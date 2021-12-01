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
 * Copyright (c) 2020, Michael A. Cuevas <cuevas@u.northwestern.edu>
 * Copyright (c) 2020, Aaron R. Nelson <arn@u.northwestern.edu>
 * Copyright (c) 2020, Peter A. Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2020, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Michael A. Cuevas <cuevas@u.northwestern.edu>
 *          Aaron R. Nelson <arn@northwestern.edu>
 *          Peter A. Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#ifndef __PROCESS_H__
#define __PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

// Always included so we get the necessary type
#include <nautilus/aspace.h>
#include <nautilus/signal.h>
#include <nautilus/group.h>
#include <nautilus/loader.h>
#include <nautilus/alloc.h>
#include <nautilus/vc.h>

#ifdef NAUT_CONFIG_LINUX_SYSCALLS
#include <nautilus/syscalls/proc.h>
#endif

/* common thread stack sizes */
#define PSTACK_DEFAULT 0  // will be 4K
#define PSTACK_4KB     0x001000
#define PSTACK_1MB     0x100000
#define PSTACK_2MB     0x200000

#define PHEAP_4KB 0x00001000UL
#define PHEAP_1MB 0x00100000UL
#define PHEAP_1GB 0x40000000UL

#define MAX_PROCESS_NAME 32
#define MAX_PROCESS_PATH 32
#define MAX_PID 32767 // 4Kb in bits
#define MAX_PROCESS_COUNT 16384 // half of MAX_PID for now

#define KERNEL_ADDRESS_START 0
#define KERNEL_MEMORY_SIZE 0x100000000UL
#define PSTACK_START 0xffff800000000000UL
//#define PSTACK_START 0x00007FFFFFEFEFFFUL
#define PSTACK_SIZE PHEAP_1MB

typedef struct PID_MAP {
    uint8_t val : 1;
} __packed pid_map;


typedef struct proc_info {
  struct list_head process_list;
  int lock;
  uint64_t process_count;
  uint64_t next_pid;
  pid_map used_pids[MAX_PID];
} process_info;
  

/******** EXTERNAL INTERFACE **********/

// opaque pointer given to users
typedef void* nk_process_id_t;
// this bad id value is intended for use with fork
// which cannot do error reporting the usual way
#define NK_BAD_PROCESS_ID ((void*)(-1ULL))

/************* INTERNALS *************/

typedef struct nk_process {
  // may need kernel stack in threads when we have user processes
  // could possibly use kernel stack for each core?

  // what aspace the process is using
  nk_aspace_t *aspace;

  // beginning of heap TODO move to syscall state?
  void *heap_begin;

  // end of heap TODO move to syscall state?
  void *heap_end;

  nk_aspace_region_t heap_region;

#ifdef NAUT_CONFIG_LINUX_SYSCALLS
  struct nk_process_linux_syscall_state syscall_state;
#endif

  // last CPU a thread is pinned to
  uint64_t last_cpu_thread;

  // Memory allocator
  nk_alloc_t *allocator;

  // Current process status - might add later 

  // exec struct and arg/env info -- might have to ask Brian about location for Karat
  struct nk_exec *exe;
  uint64_t argc;
  char **argv;
  char **argv_virt;
  uint64_t envc;
  char **envp;

  void* giga_blob;
  
  // process id
  unsigned long pid;
  
  // sync primitive, spinlock
  int lock;
  
  // sched info/thread group
  nk_thread_group_t *t_group;

  // Process Name
  char name[MAX_PROCESS_NAME];

  // Process path on disk
  char path[MAX_PROCESS_PATH];

  // process list
  struct list_head process_node;

  // file info struct ptr
  // sys call defines this and passes to proc
  //    ttl (virtual console?)
  //    file system
  //    fd table
  //    root/working directory

  // Virtual Console
  struct nk_virtual_console *vc;

  // syscall table struct ptr
  //    factored out

  // process type maybe?
  //   kernel vs user level process

  // process hierarchy info
  struct nk_process* parent;

  // signal handling info
  nk_signal_handler_table_t *signal_handler;
  nk_signal_descriptor_t *signal_descriptor;

  /* 
   * Might add notifier fields later
   * Notifier fields are used by device
   * drivers to block signals
   */
  

} nk_process_t; 


// Later we'll want to add notion of process to thread
// Belong to process? If so, which one (pid, ptr)
// will help w/ malloc implementation


// public functions
//   create
//   create and launch (like fiber_start)
//   exec
//   destroy
//   lookup_name
//   init (at boot time, initialize list of process)
//   current_process
//   fork (cannot rely on paging to implement fork)

int nk_process_create(char *exe_name, 
                      char *argv[],
                      char *envp[], 
                      char *aspace_type,
                      nk_process_t **proc_struct);

int nk_process_run(nk_process_t *p, int target_cpu);

// create and run a process
int nk_process_start(char *exe_name,
                     char *argv[],
                     char *envp[],
                     char *aspace_type,
                     nk_process_t **p,
                     int target_cpu);

int nk_process_name(nk_process_id_t proc, char *name);

nk_process_t *nk_process_current();

// called at boot time
int nk_process_init();

/*
int nk_process_exec();

int nk_process_destroy();

int nk_process_find_by_name();


int nk_process_fork();
*/

#ifdef __cplusplus
}
#endif
#endif
