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

#include <nautilus/process.h>
#include <nautilus/thread.h>
#include <nautilus/printk.h>
#include <nautilus/nautilus_exe.h>
#include <aspace/carat.h>

#ifndef NAUT_CONFIG_DEBUG_PROCESSES
#undef  DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif
#define PROCESS_INFO(fmt, args...) INFO_PRINT("process: " fmt, ##args)
#define PROCESS_ERROR(fmt, args...) ERROR_PRINT("process: " fmt, ##args)
#define PROCESS_DEBUG(fmt, args...) INFO_PRINT("process: " fmt, ##args)
#define PROCESS_WARN(fmt, args...)  WARN_PRINT("process: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("process: " fmt, ##args)

/* Macros for locking and unlocking processs */
#define _LOCK_PROCESS(proc) spin_lock(&(proc->lock))
#define _UNLOCK_PROCESS(proc) spin_unlock(&(proc->lock))
#define _LOCK_PROCESS_INFO(p_info) spin_lock(&(p_info->lock))
#define _UNLOCK_PROCESS_INFO(p_info) spin_unlock(&(p_info->lock))

/* Globals */
process_info global_process_info;

/* Internal Functions */
process_info* get_process_info() {
  return &global_process_info;
}

void add_to_process_list(nk_process_t *p) {
  struct list_head p_list = get_process_info()->process_list;
  list_add_tail(&(p->process_node), &p_list);
}

int get_new_pid(process_info *p_info) {
  int pid_map_ind;
  do {
    pid_map_ind = p_info->next_pid % MAX_PID;
    (p_info->next_pid)++;
  } while ((p_info->used_pids)[pid_map_ind].val > 0);
  (p_info->used_pids)[pid_map_ind].val = 1;
  return pid_map_ind;
}

void free_pid(process_info *p_info, uint64_t old_pid) {
  (p_info->used_pids)[old_pid].val = 0;
}

void count_and_len(char **arr, uint64_t *count, uint64_t *len) {
  *len = 0;
  *count = 0;
  if (arr) {
    PROCESS_INFO("Entering count_and_len for loop.\n");
    for (*count = *len = 0; arr[*count]; (*count)++) {
      *len += strlen(arr[*count]) + 1;
      PROCESS_INFO("Found len of arg %s, total len is %lu. Arg count is %lu.\n", arr[*count], *len, *count);
    }
    (*len)++;
  }
}

/*
 *
 * The stack will look like this once we're doing adding args and env variables:
 * ________________________________
 * |                               |
 * | Bottom of stack (highest addr)|
 * |_______________________________|
 * |                               |
 * |   Array of C Strings (Args)   |
 * |_______________________________|
 * |                               |
 * |   Array of Char * pointers    |
 * |  (Point to the strings above) |
 * |_______________________________|<------------- Ptr to here returned by function
 * |                               |
 * | Array of C Strings (Env Vars) |
 * |_______________________________|
 * |                               |
 * |   Array of Char * pointers    |
 * |  (Point to Env Strings above) |
 * |_______________________________|<------------- Ptr to here used as stack ptr and returned by function
 */
char **copy_argv_or_envp(char *arr[], uint64_t count, uint64_t len, void **stack_addr) {
  if (arr) {
    // make room for array of characters on stack
    char *stack_arr;
    *stack_addr -= (sizeof(char) * len);
    stack_arr = *stack_addr;

    PROCESS_DEBUG("Made room on stack for %lu characters.\n", len);
    PROCESS_DEBUG("Stack addr is now %p\n", *stack_addr);

    // align stack to 8 bytes
    *stack_addr = (void*)(((uint64_t)*stack_addr) & ~0x7UL);
    PROCESS_DEBUG("Aligned stack to 8 bytes. Stack addr is now %p\n", *stack_addr);
    
    // make room for array of C string pointers on stack
    char **ptr_arr;
    *stack_addr -= sizeof(char *) * (count + 1);
    ptr_arr = *stack_addr;
  
    PROCESS_DEBUG("Made room on stack for %lu pointers.\n", count+1);
    PROCESS_DEBUG("Stack addr is now %p\n", *stack_addr);
    
    // align stack to 8 bytes (shouldn't need alignment, but just in case)
    *stack_addr =  (void*)(((uint64_t)*stack_addr) & ~0x7UL);
    PROCESS_DEBUG("Aligned stack to 8 bytes. Stack addr is now %p\n", *stack_addr);

    // actually copy characters and pointers to stack
    uint64_t i, stack_idx, new_str_len;
    new_str_len = 0;
    for (i = stack_idx = 0; i < count; i++) {
      PROCESS_DEBUG("copying %s to the stack at addr %p\n", arr[i], &(stack_arr[stack_idx]));
      new_str_len = strlen(arr[i]) + 1;
      ptr_arr[i] = &(stack_arr[stack_idx]);
      strcpy(&(stack_arr[stack_idx]), arr[i]);
      stack_idx += new_str_len + 1;
    }
    ptr_arr[i] = 0;
    PROCESS_DEBUG("arg pointer array after adding args: %p\n", *ptr_arr);
    return ptr_arr;
  }
  return *stack_addr;
}

void __nk_process_wrapper(void *i, void **o) {
  nk_process_t *p = (nk_process_t*)i;
  PROCESS_DEBUG("Entering process wrapper.\n");

  PROCESS_INFO("Swapping to new stack!\n");

  // current thread belongs to a process now
  // TODO MAC: May need to acquire lock
  nk_thread_t *me = get_cur_thread();
  
  const uint64_t giga_blob_size = 0x40000000;
  p->giga_blob = kmem_sys_malloc(giga_blob_size);

  if (!p->giga_blob) {
    PROCESS_ERROR("Could not allocate the giga blob! Why!?\n");
  }

  PROCESS_INFO("The addr of giga_blob is [%p, %p]\n", p->giga_blob, p->giga_blob + giga_blob_size);

  /*
  * Add the process' thread stack to the process address space
  */ 
  const uint64_t new_stack_size = 0x10000000;
  nk_aspace_t *process_aspace = p->aspace;
  nk_aspace_region_t r_stack;
  r_stack.va_start = p->giga_blob; // (void *)PSTACK_START;
  r_stack.pa_start = p->giga_blob;
  r_stack.len_bytes = new_stack_size; 
  r_stack.protect.flags = NK_ASPACE_READ | NK_ASPACE_EXEC | NK_ASPACE_WRITE | NK_ASPACE_PIN | NK_ASPACE_EAGER;
  if (nk_aspace_add_region(process_aspace, &r_stack)) {
    PROCESS_ERROR("failed to add initial process stack region (from gigablob)\n");
    nk_aspace_destroy(process_aspace);
    return;
  }
  
  const uint64_t stack_offset = 0x11000000;
  nk_aspace_region_t h_stack;
  h_stack.va_start = p->giga_blob + stack_offset; // (void *)PSTACK_START;
  h_stack.pa_start = h_stack.va_start;
  h_stack.len_bytes = giga_blob_size - stack_offset;
  h_stack.protect.flags = NK_ASPACE_READ | NK_ASPACE_EXEC | NK_ASPACE_WRITE |
                          NK_ASPACE_PIN | NK_ASPACE_EAGER;
  if (nk_aspace_add_region(process_aspace, &h_stack)) {
    PROCESS_ERROR("failed to add initial process heap region (from gigablob)\n");
    nk_aspace_destroy(process_aspace);
    return;
  }
  
  me->process = p;


  /*
   * Cache the process' thread stack to the internal carat aspace
   */ 
  nk_aspace_carat_t *carat = ((nk_aspace_carat_t *) process_aspace->state);
  carat->initial_stack = &r_stack; 


  //set virtual console so we can print to shell
  me->vc = p->vc; 

  
  // this should be (carefully) wrapped into a process thread init function (shared with clone)
  {
    me->fake_affinity = 0;      // to facilitate the fake affinity syscalls
    me->clear_child_tid = 0;    // to facilitate threading
  }

  // TODO MAC: This works... but aspace swap is sketchy
  int argc = p->argc;
  char **args = p->argv;
  char **envp = p->envp;
  struct nk_exec *exe = p->exe;

  if (nk_thread_group_join(p->t_group)) {
    PROCESS_ERROR("Failed to join thread group\n");
    return;
  }
  PROCESS_INFO("After thread group\n");
 
  // Associate allocator with process thread
  if (nk_alloc_set_associated(p->allocator)) {
    PROCESS_ERROR("Failed to associate process with allocator\n");
    nk_thread_exit((void *)-1);
  } 
 
  // Set process signal state to the starting thread's signal state
  p->signal_descriptor = me->signal_state->signal_descriptor;
  p->signal_handler = me->signal_state->signal_handler;

  // Move thread into process address space
  PROCESS_DEBUG("Moving thread into process aspace. Aspace addr: %p, Process addr %p\n", p->aspace, p);
  nk_aspace_move_thread(p->aspace);
  PROCESS_DEBUG("Sucessfully swapped to process aspace\n");

  // Start execution of process executable.
  PROCESS_DEBUG("Starting executable at addr %p with %lu args\n", exe, argc);
  struct nk_crt_proc_args proc_args;
  proc_args.argv = args;
  proc_args.envp = envp;
  proc_args.argc = argc;

  nk_start_exec(exe, &proc_args, NULL);
  PROCESS_INFO("Got past start exec crt\n");
}

int create_process_aspace(nk_process_t *p, char *aspace_type, char *exe_name, nk_aspace_t **new_aspace, void **stack) {
  // Check if the desired aspace implementation exists
  nk_aspace_characteristics_t c;
  if (nk_aspace_query(aspace_type, &c)) {
    PROCESS_ERROR("failed to find %s aspace implementation\n", aspace_type);
    return -1;
  } 

  // create aspace instance of type aspace_type
  nk_aspace_t *addr_space = nk_aspace_create(aspace_type, exe_name, &c);
  if (!addr_space) {
    PROCESS_ERROR("failed to create address space\n");
    return -1;
  }  

  // allocate stack for process
  void *p_addr_start = malloc(PSTACK_SIZE);
  if (!p_addr_start) {
    nk_aspace_destroy(addr_space);
    PROCESS_ERROR("failed to allocate process stack\n");
    return -1;
  }
  memset(p_addr_start, 0, PSTACK_SIZE);

  // add stack to address space
  nk_aspace_region_t r_stack;
#ifdef NAUT_CONFIG_CARAT_PROCESS
  r_stack.va_start = p_addr_start;
#else
  r_stack.va_start = (void *)PSTACK_START;
#endif
  r_stack.pa_start = p_addr_start;
  r_stack.len_bytes = (uint64_t)PSTACK_SIZE; 
  r_stack.protect.flags = NK_ASPACE_READ | NK_ASPACE_EXEC | NK_ASPACE_WRITE | NK_ASPACE_PIN | NK_ASPACE_EAGER;
  
  if (nk_aspace_add_region(addr_space, &r_stack)) {
    PROCESS_ERROR("failed to add initial process aspace stack region\n");
    nk_aspace_destroy(addr_space);
    free(p_addr_start);
    return -1;
  }

#ifndef NAUT_CONFIG_CARAT_PROCESS

  // add kernel to address space
  nk_aspace_region_t r_kernel;
  r_kernel.va_start = (void *)KERNEL_ADDRESS_START;
  r_kernel.pa_start = (void *)KERNEL_ADDRESS_START;
  r_kernel.len_bytes = KERNEL_MEMORY_SIZE; 
  r_kernel.protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE | NK_ASPACE_EXEC | NK_ASPACE_PIN | NK_ASPACE_KERN | NK_ASPACE_EAGER;

  if (nk_aspace_add_region(addr_space, &r_kernel)) {
    PROCESS_ERROR("failed to add initial process aspace stack region\n");
    nk_aspace_destroy(addr_space);
    return -1;
  }

#endif


  // load executable into memory
  p->exe = nk_load_exec(exe_name);

  // map executable in address space if it's not (entirely) within first 4GB of memory
  uint64_t exe_end_addr = (uint64_t)p->exe->blob + p->exe->blob_size;
  

#if NAUT_CONFIG_CARAT_PROCESS

  nk_aspace_characteristics_t aspace_chars;
  if (nk_aspace_query(aspace_type, &aspace_chars)) {
    nk_unload_exec(p->exe);
    free(p);
    nk_aspace_destroy(addr_space);
    return -1;
  }

  nk_aspace_region_t r_exe;

  r_exe.va_start = p->exe->blob;
  r_exe.pa_start = p->exe->blob;
  r_exe.len_bytes = p->exe->blob_size + (p->exe->blob_size % aspace_chars.granularity);

  r_exe.protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE | NK_ASPACE_EXEC | NK_ASPACE_EAGER;

  if (nk_aspace_add_region(addr_space, &r_exe)) {
    PROCESS_ERROR("failed to add initial process aspace exe region\n");
    nk_unload_exec(p->exe);
    free(p);
    nk_aspace_destroy(addr_space);
    return -1;
  }


  /*
   * Cache the blob to the internal carat aspace
   */ 
  nk_aspace_carat_t *carat = ((nk_aspace_carat_t *) addr_space->state);
  carat->initial_blob = &r_exe; 


  // mm_show(addr_space->mm);

#else
  
  if (((uint64_t)p->exe->blob > KERNEL_MEMORY_SIZE) || (exe_end_addr > KERNEL_MEMORY_SIZE)) {

    nk_aspace_characteristics_t aspace_chars;
    if (nk_aspace_query(aspace_type, &aspace_chars)) {
      // TODO MAC: Exit gracefully
      nk_unload_exec(p->exe);
      free(p);
      nk_aspace_destroy(addr_space);
      return -1;
    }

    nk_aspace_region_t r_exe;
    if ((uint64_t)p->exe->blob < KERNEL_MEMORY_SIZE) {
      // We are partially overlapping the boundary between the lower 4G and beyond
      r_exe.va_start = (void *)KERNEL_MEMORY_SIZE;
      r_exe.pa_start = (void *)KERNEL_MEMORY_SIZE;
      uint64_t exe_overshoot = exe_end_addr - KERNEL_MEMORY_SIZE;
      r_exe.len_bytes = exe_overshoot + (exe_overshoot % aspace_chars.granularity);
    } else {
      // We are completely beyond the lower 4G
      r_exe.va_start = p->exe->blob;
      r_exe.pa_start = p->exe->blob;
      r_exe.len_bytes = p->exe->blob_size + (p->exe->blob_size % aspace_chars.granularity);
    }

    r_exe.protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE | NK_ASPACE_EXEC | NK_ASPACE_EAGER;

    if (nk_aspace_add_region(addr_space, &r_exe)) {
      PROCESS_ERROR("failed to add initial process aspace exe region\n");
      nk_unload_exec(p->exe);
      free(p);
      nk_aspace_destroy(addr_space);
      return -1;
    }
  }

#endif


  if (new_aspace) {
    *new_aspace = addr_space;
  }
  
  if (stack) {
    *stack = p_addr_start + PSTACK_SIZE;
  }

  return 0;
}


/* External Functions */
int nk_process_create(char *exe_name, char *argv[], char *envp[], char *aspace_type, nk_process_t **proc_struct) {
  // Fetch current process info
  process_info *p_info = get_process_info();
  if (p_info->process_count >= MAX_PROCESS_COUNT) {
    PROCESS_ERROR("Max number of processes (%ul) reached. Cannot create process.\n", p_info->process_count);
    return -1;
  }
  
  // alloc new process struct
  nk_process_t *p = NULL;
  p = (nk_process_t*)malloc(sizeof(nk_process_t));
  if (!p) {
    PROCESS_ERROR("Failed to allocate process struct.\n");
    return -1;
  }
  memset(p, 0, sizeof(nk_process_t));
 
  // associate parent process if it exists
  p->parent = NULL;
  nk_thread_t *curr_thread = get_cur_thread();
  if (curr_thread->process) {
    p->parent = curr_thread->process;
  }

  // create process address space
  nk_aspace_t *addr_space;
  void *stack_addr = NULL;
  if (create_process_aspace(p, aspace_type, exe_name, &addr_space, &stack_addr) || !addr_space) {
    PROCESS_ERROR("failed to create process address space\n");
    free(p);
    return -1;
  }
  PROCESS_INFO("Created address space\n"); 

  // count argv and envp, allocate them on stack
  PROCESS_INFO("stack address (highest stack addr): %p\n", stack_addr);
  uint64_t argc, argv_len, envc, envp_len;
  argc = argv_len = envc = envp_len = 0;
  count_and_len(argv, &argc, &argv_len);
  PROCESS_INFO("argc: %lu, envc: %lu\n", argc, envc);
  count_and_len(envp, &envc, &envp_len);
  char **args, **envs;
  args = envs = NULL;
  void *stack_ptr = stack_addr;
  args = copy_argv_or_envp(argv, argc, argv_len, &stack_ptr);
  envs = copy_argv_or_envp(envp, envc, envp_len, &stack_ptr);  

  // create a new allocator
  nk_alloc_t *alloc = 0;
  if (strcmp(aspace_type, "carat")) {
    /* TODO: Karat is not ready for allocator yet. Add later. */
    // nk_alloc_t *alloc = nk_alloc_create("dumb", "proc-alloc");
  } else {
    //nk_alloc_t *alloc = nk_alloc_create("cs213", "proc-alloc"); 
  }

  // ensure that lock has been initialized to 0
  spinlock_init(&(p->lock));
  
  // acquire locks and get new pid
  _LOCK_PROCESS(p);
  _LOCK_PROCESS_INFO(p_info);
  p->pid = get_new_pid(p_info);
  add_to_process_list(p);

  // release process_info lock, no global state left to modify
  _UNLOCK_PROCESS_INFO(p_info);

  // Set Process allocator
  p->allocator = alloc;

  // name process
  snprintf(p->name, MAX_PROCESS_NAME, "p-%ul-%s", p->pid, exe_name);
  p->name[MAX_PROCESS_NAME-1] = 0;

  // set address space ptr and rename it
  p->aspace = addr_space;
  nk_aspace_rename(p->aspace, p->name);
  p->heap_begin = 0; 
  p->heap_end = 0; 

  // set arg and envp info
  p->argc = argc;
  p->argv_virt = (char**)(PSTACK_START + PSTACK_SIZE - ((uint64_t)stack_addr - (uint64_t)args));
  p->argv = args;
  p->envc = envc;
  p->envp = envs;

  // create thread group (empty for now, first thread added when run() is called)
  p->t_group = nk_thread_group_create(p->name);
  if (!(p->t_group)) {
    PROCESS_ERROR("Failed to create thread group\n");
    _UNLOCK_PROCESS(p);
    return -1;
  }

  // Set virtual console
  p->vc = curr_thread->vc;

  // release process lock
  _UNLOCK_PROCESS(p);

  // set output ptr (if not null)
  if (proc_struct) {
    *proc_struct = p;
  }

  return 0;  
}

int nk_process_name(nk_process_id_t proc, char *name)
{
  nk_process_t *p = (nk_process_t*)proc;
  strncpy(p->name,name,MAX_PROCESS_NAME);
  p->name[MAX_PROCESS_NAME-1] = 0;
  return 0;
}

int nk_process_run(nk_process_t *p, int target_cpu) {
  nk_thread_id_t tid;
  p->last_cpu_thread = target_cpu;
  return nk_thread_start(__nk_process_wrapper, (void*)p, 0, 0, 0x1000, &tid, target_cpu);
}

int nk_process_start(char *exe_name, char *argv[], char *envp[], char *aspace_type, nk_process_t **p, int target_cpu) {
  if (nk_process_create(exe_name, argv, envp, aspace_type, p)) {
    PROCESS_ERROR("failed to create process\n");
    return -1;
  }
  if (nk_process_run(*p, target_cpu)) {
    PROCESS_ERROR("failed to run new process\n");
    //nk_process_destroy(*p);
    return -1;
  }
  return 0;
}

// TODO MAC: There's a chance the process pointer isn't mapped in the current aspace
nk_process_t *nk_process_current() {
  nk_thread_t *t = get_cur_thread();
  return t->process;
}

int nk_process_destroy(nk_process_t *p) {
  // destroy allocator, unmap executable, destroy thread_group,  destroy stack, destroy heap, and tear down aspace 
  
  // We must be careful if we're part of the current process
  int destroying_curr_process = 0;
  nk_thread_t *me = get_cur_thread();
  if (me->process == nk_process_current()) {
    // Prevent self from dying when sending exit signals
    PROCESS_ERROR("Not handling process destroy properly.\n"); 
    destroying_curr_process = 1;
  }


  // Destroy process allocator
  nk_alloc_t *alloc = p->allocator;
  if (nk_alloc_destroy(alloc)) {
    PROCESS_ERROR("Failed to destroy allocator for process %p (name: %s)\n", p, p->name);
  } 

  // TODO MAC: Send exit signal to all process threads
  // Will break until we implement this
 
  // delete thread group (may need to wait until all threads exit)
  nk_thread_group_delete(p->t_group);

  // unmap executable
  nk_unload_exec(p->exe); 

  // free heap and exit (this might be wrong)
  // The threads we create use different stacks than they start with (thread->stack != curr_stack)
  // Calling thread exit may not free the appropriate stack
  free(p->heap_begin);

  // If we're part of the current process, we need to exit instead of destroying aspace  
  if (destroying_curr_process) { 
    nk_thread_exit((void *)0);
  }
  
  // tear down aspace
  nk_aspace_destroy(p->aspace);
 
  return 0;
}

// add this right after loader init
int nk_process_init() {
  memset(&global_process_info, 0, sizeof(process_info));
  INIT_LIST_HEAD(&(global_process_info.process_list));
  global_process_info.lock = 0;
  global_process_info.process_count = 0;
  global_process_info.next_pid = 0;
  return 0; 
}
