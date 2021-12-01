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
 * Copyright (c) 2021, Michael A. Cuevas <cuevas@u.northwestern.edu>
 * Copyright (c) 2021, Peter A. Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2021, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Michael A. Cuevas <cuevas@u.northwestern.edu>
 *          Peter A. Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nautilus/signal_consts.h>
#include <nautilus/spinlock.h>
#include <nautilus/list.h>


/* Function signature of a signal handler */
typedef void (*nk_signal_handler_t)(int signal_num);
#define DEFAULT_SIG ((nk_signal_handler_t)0)
#define IGNORE_SIG ((nk_signal_handler_t)1)


struct nk_process;
struct nk_thread;
struct nk_wait_queue;


/* Struct for siginfo -- stores info about a specific occurrence of a signal */
typedef struct signal_info_struct {
    uint64_t signal_num;
    uint64_t signal_err_num;
    uint64_t signal_code;
    union {
        /* kill() syscall */
        struct {
            uint64_t sender_pid;
        } _kill_info;

        /* Posix.1b timers and signals not implemented */
        /* SIGCHLD */
        struct {
            uint64_t child_pid;
            uint64_t exit_status;
            uint64_t utime;
            uint64_t stime;
        } _sigchld_info;
    
        /* SIGILL, SIGFPE, SIGSEGV, SIGBUS, SIGTRAP, SIGEMT */
        struct {
            void *faulting_addr; /* User ptr, address which caused fault */
            short addr_lsb; /* LSB of reported addr from SIGBUS */
            /* Don't handle SEGV_BNDERR or SEGV_PKUERR */
        } _sigfault_info;

        /* SIGPOLL */
        struct {
            uint64_t band; /* poll in, out, or msg */
            int file_descriptor;
        } _sigpoll_info;
    
        /* SIGSYS */
        struct {
            void *caller_addr; /* addr of caller */
            uint64_t syscall_num; /* syscall triggered by caller */
        } _sigsys_info;
    } signal_info;
} nk_signal_info_t;   

/* Sig info constants (no info or from kernel) */
#define SEND_SIGNAL_NO_INFO      (nk_signal_info_t *)0
#define SEND_SIGNAL_KERNEL       (nk_signal_info_t *)1


/* Signal Bit Set (For now, we assume SIGSET_SIZE == 1) */
typedef struct signal_set{
      uint64_t sig[SIGSET_SIZE];
} nk_signal_set_t;


/* LL Signal Queue (for RT signals, may not be needed?) */
typedef struct signal_queue {
    struct list_head lst;
    uint64_t flags;
    nk_signal_info_t signal_info;
} nk_signal_queue_t;


/* LL of pending signals */
typedef struct signal_pending {
    struct list_head lst;
    nk_signal_set_t signal;
    uint64_t dest_type;
} nk_signal_pending_t;


/* Signal action tells us what handler to use, if signal is ignored, or if signal is blocked */
typedef struct signal_action {
    nk_signal_handler_t handler; /* Ptr to signal handler, IGNORE_SIG(=1), or DEFAULT_SIG(=0) */ 
    nk_signal_set_t mask; /* what should be masked when running signal handler */
    uint64_t signal_flags; /* How to handle signal */
} nk_signal_action_t;


/* Table of signal action structs. May be shared among processes. */
typedef struct signal_handler_table {
    uint64_t count; /* How many processes are using this signal handler table? */
    nk_signal_action_t handlers[NUM_SIGNALS]; /* pointers to sig action structs */
    spinlock_t lock; /* Multiple processes can use this table */ 
} nk_signal_handler_table_t;


/* Information for an occurrence of a signal */
typedef struct ksignal {
    nk_signal_action_t signal_action; /* What action the signal will take */
    nk_signal_info_t signal_info; /* Info about signal TODO MAC: Should we support this?*/
    uint64_t signal_num; /* Number of signal (1-64) */
} nk_ksignal_t;


/* Signal descriptor (lives inside of thread struct) */
typedef struct signal_descriptor {
    uint64_t num_shared_sigs; /* +0  SHOULD NOT CHANGE POSITION */

    /* Rest of state, position independent */
    uint64_t count; /* Use counter */
    uint64_t live; /* Live threads within process */
    struct nk_wait_queue *wait_child_exit; /* for processes sleeping in wait4() */
    struct nk_thread *curr_target; /* Last thread to receive signal */
    nk_signal_pending_t shared_pending; /* shared pending signals */
    uint64_t group_exit_code;
    struct nk_thread *group_exit_task; /* thread doing the notifying? */
    uint64_t notify_count; /* num threads killed on group exit */
    uint64_t group_stop_count; /* num threads stopped on group stop */
    uint64_t flags;
    /* May not make sense to keep track on a per process basis? */
    uint64_t num_queued; /* Number of RT signals queued. */
} nk_signal_descriptor_t;


/* 
 * Task signal info struct. 
 * Cleanly stores all signal state for threads/processes/tgroups 
 */
typedef struct signal_task_state {
    uint64_t num_sigs;                          /* +0  SHOULD NOT CHANGE POSITION */
    nk_signal_descriptor_t *signal_descriptor;  /* +8  SHOULD NOT CHANGE POSITION */

    
    /* For signal handling, shared state w/ processes and groups */
    /* TODO MAC: Figure out how to properly do this :) */
    nk_signal_handler_table_t *signal_handler;
    
    /* For individual thread signal handling */
    nk_signal_set_t blocked;
    nk_signal_set_t real_blocked;
    nk_signal_pending_t signals_pending;

    /* Unused for now
    uint64_t signal_hand_rsp;
    uint64_t sh_rsp_size;
    */
} nk_signal_task_state;

/* Interface functions */

static inline void sigaddset(nk_signal_set_t *set, uint64_t signal)
{
    /* Assumes signal <= BYTES_PER_WORD */
    signal--;
    set->sig[0] |= 1UL << signal;
}

static inline void sigdelset(nk_signal_set_t *set, uint64_t signal)
{
    /* Assumes signal <= BYTES_PER_WORD */
    signal--;
    set->sig[0] &= ~(1UL << signal);
}

static inline int sigismember(nk_signal_set_t *set, uint64_t signal)
{
    signal--;
    return (set->sig[0] >> signal) & 1;
}

static inline int sigisemptyset(nk_signal_set_t *set) 
{
    return !(set->sig[0]);
}

static inline int sigequalsets(const nk_signal_set_t *set1, const nk_signal_set_t *set2) 
{
    return set1->sig[0] == set2->sig[0];
}

#define sigmask(signal) (1UL << ((signal) - 1))

static inline void sigorsets(nk_signal_set_t *result, nk_signal_set_t *set1, nk_signal_set_t *set2)
{
    result->sig[0] = ((set1->sig[0]) | (set2->sig[0]));
}

static inline void sigandsets(nk_signal_set_t *result, nk_signal_set_t *set1, nk_signal_set_t *set2) 
{
    result->sig[0] = ((set1->sig[0]) & (set2->sig[0]));
}

static inline void sigandnsets(nk_signal_set_t *result, nk_signal_set_t *set1, nk_signal_set_t *set2) 
{
    result->sig[0] = ((set1->sig[0]) & ~(set2->sig[0]));
}

static inline void signotset(nk_signal_set_t *set)
{
    set->sig[0] = ~(set->sig[0]);
}

static inline void sigemptyset(nk_signal_set_t *set)
{
    set->sig[0] = 0;
}

static inline void sigfillset(nk_signal_set_t *set) 
{
    set->sig[0] = -1;
}

/* TODO MAC: Should we restrict mask to only first 32 bits? */
static inline void sigaddsetmask(nk_signal_set_t *set, uint64_t mask)
{
    set->sig[0] |= mask;
}

static inline void sigdelsetmask(nk_signal_set_t *set, uint64_t mask)
{
    set->sig[0] &= ~mask;
}

static inline int sigtestsetmask(nk_signal_set_t *set, uint64_t mask)
{
    return (set->sig[0] & mask) != 0;
}

static inline void siginitset(nk_signal_set_t *set, uint64_t mask)
{
    set->sig[0] = mask;
}

static inline void siginitsetinv(nk_signal_set_t *set, uint64_t mask)
{
    set->sig[0] = ~mask;
}

static inline void init_sigpending(nk_signal_pending_t *pending, uint64_t dest_type)
{
    sigemptyset(&(pending->signal));
    INIT_LIST_HEAD(&(pending->lst));
    pending->dest_type = dest_type;
}

static inline int valid_signal(uint64_t sig)
{
    return sig <= NUM_SIGNALS ? 1 : 0; 
}

/* Signal mask definitions */
#define rt_sigmask(sig) sigmask(sig)

#ifdef NKSIGEMT /* TODO MAC: Might not be needed */
#define SIGEMT_MASK rt_sigmask(SIGEMT)
#else
#define SIGEMT_MASK 0
#endif

#define siginmask(sig, mask) \
	((sig) > 0 && (sig) < SIGRTMIN && (rt_sigmask(sig) & (mask)))

#define SIG_KERNEL_ONLY_MASK (\
	rt_sigmask(NKSIGKILL)   |  rt_sigmask(NKSIGSTOP))

#define SIG_KERNEL_STOP_MASK (\
	rt_sigmask(NKSIGSTOP)   |  rt_sigmask(NKSIGTSTP)   | \
	rt_sigmask(NKSIGTTIN)   |  rt_sigmask(NKSIGTTOU)   )

#define SIG_KERNEL_COREDUMP_MASK (\
        rt_sigmask(NKSIGQUIT)   |  rt_sigmask(NKSIGILL)    | \
	rt_sigmask(NKSIGTRAP)   |  rt_sigmask(NKSIGABRT)   | \
        rt_sigmask(NKSIGFPE)    |  rt_sigmask(NKSIGSEGV)   | \
	rt_sigmask(NKSIGBUS)    |  rt_sigmask(NKSIGSYS)    | \
        rt_sigmask(NKSIGXCPU)   |  rt_sigmask(NKSIGXFSZ)   | \
	SIGEMT_MASK				       )

#define SIG_KERNEL_IGNORE_MASK (\
        rt_sigmask(NKSIGCONT)   |  rt_sigmask(NKSIGCHLD)   | \
	rt_sigmask(NKSIGWINCH)  |  rt_sigmask(NKSIGURG)    )

#define SIG_SPECIFIC_SICODES_MASK (\
	rt_sigmask(NKSIGILL)    |  rt_sigmask(NKSIGFPE)    | \
	rt_sigmask(NKSIGSEGV)   |  rt_sigmask(NKSIGBUS)    | \
	rt_sigmask(NKSIGTRAP)   |  rt_sigmask(NKSIGCHLD)   | \
	rt_sigmask(NKSIGPOLL)   |  rt_sigmask(NKSIGSYS)    | \
	SIGEMT_MASK                                    )

#define sig_kernel_only(sig)		siginmask(sig, SIG_KERNEL_ONLY_MASK)
#define sig_kernel_coredump(sig)	siginmask(sig, SIG_KERNEL_COREDUMP_MASK)
#define sig_kernel_ignore(sig)		siginmask(sig, SIG_KERNEL_IGNORE_MASK)
#define sig_kernel_stop(sig)		siginmask(sig, SIG_KERNEL_STOP_MASK)
#define sig_specific_sicodes(sig)	siginmask(sig, SIG_SPECIFIC_SICODES_MASK)

#define sig_fatal(t, signr) \
	(!siginmask(signr, SIG_KERNEL_IGNORE_MASK|SIG_KERNEL_STOP_MASK) && \
	 (t)->signal_state->signal_handler->handlers[(signr)-1].handler == DEFAULT_SIG)

/* Flushing/Modifying Signal State */
void nk_signal_flush_queue(nk_signal_pending_t *queue); /* Flush pending queue */
void nk_signal_flush(struct nk_process *process); /* Flush all pending signals */
int nk_signal_process_mask(int how, nk_signal_set_t *set, nk_signal_set_t *old_set); /* how = NKSIG_BLOCK, NKSIG_UNBLOCK, or NKSIG_SETMASK */
void recalc_sigpending();
int nk_signal_next(nk_signal_pending_t *pending, nk_signal_set_t *mask); /* Given mask, find next signal to execute */

/* Sending signals */
int nk_signal_send(uint64_t signal, nk_signal_info_t *signal_info, void *signal_dest, uint64_t dest_type);
int nk_signal_info_send(uint64_t signal, nk_signal_info_t *signal_info, struct nk_process *process);
void nk_signal_force(uint64_t signal); /* Forces a signal to the current process -- cannot fail */
void nk_signal_force_specific(uint64_t signal); /* Optimized for SIGSTOP and SIGKILL */ 

/* Processing Signals */
int nk_signal_get(); /* Gets pending signals for current process */
int do_sigaction(uint64_t sig, nk_signal_action_t *act, nk_signal_action_t *old_act);

/* Initializing signal state */
int nk_signal_init_task_state(nk_signal_task_state **state_ptr, struct nk_thread *t);

#ifdef __cplusplus
}
#endif

// endif for top ifndef __SIGNAL_H__
#endif
