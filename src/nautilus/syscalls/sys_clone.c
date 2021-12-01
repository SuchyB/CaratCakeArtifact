#include <nautilus/nautilus.h>
#include <nautilus/syscalls/decl.h>
#include <nautilus/thread.h>

#define SYSCALL_NAME "sys_clone"
#include "impl_preamble.h"

#define ARCH_SET_FS 0x1002

// These are copied directly from /usr/include/linux
#define CSIGNAL 0x000000ff     /* signal mask to be sent at exit */
#define CLONE_VM 0x00000100    /* set if VM shared between processes */
#define CLONE_FS 0x00000200    /* set if fs info shared between processes */
#define CLONE_FILES 0x00000400 /* set if open files shared between processes \
                                */
#define CLONE_SIGHAND \
  0x00000800 /* set if signal handlers and blocked signals shared */
#define CLONE_PIDFD 0x00001000 /* set if a pidfd should be placed in parent */
#define CLONE_PTRACE \
  0x00002000 /* set if we want to let tracing continue on the child too */
#define CLONE_VFORK                                                           \
  0x00004000 /* set if the parent wants the child to wake it up on mm_release \
              */
#define CLONE_PARENT \
  0x00008000 /* set if we want to have the same parent as the cloner */
#define CLONE_THREAD 0x00010000         /* Same thread group? */
#define CLONE_NEWNS 0x00020000          /* New mount namespace group */
#define CLONE_SYSVSEM 0x00040000        /* share system V SEM_UNDO semantics */
#define CLONE_SETTLS 0x00080000         /* create a new TLS for the child */
#define CLONE_PARENT_SETTID 0x00100000  /* set the TID in the parent */
#define CLONE_CHILD_CLEARTID 0x00200000 /* clear the TID in the child */
#define CLONE_DETACHED 0x00400000       /* Unused, ignored */
#define CLONE_UNTRACED                                                      \
  0x00800000 /* set if the tracing process can't force CLONE_PTRACE on this \
                clone */
#define CLONE_CHILD_SETTID 0x01000000 /* set the TID in the child */
#define CLONE_NEWCGROUP 0x02000000    /* New cgroup namespace */
#define CLONE_NEWUTS 0x04000000       /* New utsname namespace */
#define CLONE_NEWIPC 0x08000000       /* New ipc namespace */
#define CLONE_NEWUSER 0x10000000      /* New user namespace */
#define CLONE_NEWPID 0x20000000       /* New pid namespace */
#define CLONE_NEWNET 0x40000000       /* New network namespace */
#define CLONE_IO 0x80000000           /* Clone io context */

struct clone_compat_args {
  void* rsp;
  void* sysret_addr;
  void* tls;
};

/// Sends new thread back to the syscall return point, simulating a linux clone.
/// Assumes that the thread state other than rsp is set correctly for the clone
/// args.
/// @param args_opaque is an opaque struct clone_compat_args*
void _clone_compat_wrapper(void* args_opaque, void** _) {
  struct clone_compat_args* args = (struct clone_compat_args*)args_opaque;
  void* rsp = args->rsp;
  void* sysret_addr = args->sysret_addr;

  if (args->tls) {
    msr_write(MSR_FS_BASE, (uint64_t)args->tls);
  }

  free(args);
  nk_thread_t* me = get_cur_thread();
  me->fake_affinity = 0; // this should be wrapped into a process thread init
                         // function (shared with process initialization)
  me->vc = me->process->vc;

  DEBUG("Sending clone thread to RIP=%p RSP=%p and glibc thread function %p\n",
        sysret_addr, rsp, *(uint64_t*)rsp);
  /* We do not restore the flags, but this is ok in the most common case (glibc
   * clone), where the thread starts in a new function */
  asm("mov $0, %%rax\n"
      "mov %0, %%rsp\n"
      "jmp *%1\n"
      :
      : "r"(rsp), "r"(sysret_addr)
      : "rax");
  panic("Return from clone compat wrapper\n");
}

uint64_t sys_clone(uint64_t clone_flags, uint64_t newsp,
                   uint32_t* parent_tidptr, uint32_t* child_tidptr,
                   void* tls_val) {
  DEBUG("%p\n%p\n%p\n%p\n%p\n", clone_flags, newsp, parent_tidptr, child_tidptr,
        tls_val);

  nk_process_t* current_process = syscall_get_proc();

  struct clone_compat_args* args = malloc(
      sizeof(struct clone_compat_args)); /* Free is handled by new thread */
  args->rsp = (void*)newsp;
  args->sysret_addr = get_cur_thread()->sysret_addr;
  args->tls = (clone_flags & CLONE_SETTLS) ? (void*)tls_val : NULL;

  nk_thread_t* thread;
  nk_process_t* process = get_cur_thread()->process;

  // Create the new thread that will handle clone.
  {
    uint32_t bound_cpu;
    spin_lock(&process->lock);
    process->last_cpu_thread =
        (process->last_cpu_thread + 1) % nk_get_num_cpus();
    bound_cpu = process->last_cpu_thread;
    // bound_cpu = CPU_ANY; // TEMPORARY
    spin_unlock(&process->lock);
    nk_thread_create(&_clone_compat_wrapper, args, NULL, 1, 0,
                     (nk_thread_id_t*)&thread, bound_cpu);
    // TODO: there seem to be other things missing here (such as the vc)
    thread->process = process;
    #ifdef NAUT_CONFIG_PROCESSES /* TODO: Change this to signal conditional compilation */
    if (thread->signal_state) {
        /* allocated w/ sys allocator, should free w/ it too */
        kmem_sys_free(thread->signal_state->signal_descriptor);
        kmem_sys_free(thread->signal_state->signal_handler);
        
        /* All threads within a process share a descriptor and handler table */
        thread->signal_state->signal_descriptor = process->signal_descriptor;
        thread->signal_state->signal_handler = process->signal_handler;
    }
    #endif
  }

  if (clone_flags & CLONE_CHILD_SETTID) {
    thread->set_child_tid = child_tidptr; /* May not be needed */
    *(uint64_t*)child_tidptr = thread->tid;
  }

  if (clone_flags & CLONE_PARENT_SETTID) {
    *(uint64_t*)parent_tidptr = thread->tid;
  }

  thread->clear_child_tid =
      (clone_flags & CLONE_CHILD_CLEARTID) ? (void*)child_tidptr : NULL;

  nk_thread_run(thread);

  return thread->tid;
}
