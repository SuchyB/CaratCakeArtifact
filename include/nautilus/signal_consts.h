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

/* Signal numbers for non-realtime signals (1-31) */
#define NKSIGHUP		 1
#define NKSIGINT		 2
#define NKSIGQUIT		 3
#define NKSIGILL		 4
#define NKSIGTRAP		 5
#define NKSIGABRT		 6
#define NKSIGIOT		 6
#define NKSIGBUS		 7
#define NKSIGFPE		 8
#define NKSIGKILL		 9
#define NKSIGUSR1		10
#define NKSIGSEGV		11
#define NKSIGUSR2		12
#define NKSIGPIPE		13
#define NKSIGALRM		14
#define NKSIGTERM		15
#define NKSIGSTKFLT	    16
#define NKSIGCHLD		17
#define NKSIGCONT		18
#define NKSIGSTOP		19
#define NKSIGTSTP		20
#define NKSIGTTIN		21
#define NKSIGTTOU		22
#define NKSIGURG		23
#define NKSIGXCPU		24
#define NKSIGXFSZ		25
#define NKSIGVTALRM	    26
#define NKSIGPROF		27
#define NKSIGWINCH	    28
#define NKSIGIO		    29
#define NKSIGPOLL		NKSIGIO
#define NKSIGPWR		30
#define NKSIGSYS		31
#define	NKSIGUNUSED	NKSIGSYS

/* Constants for signal handling */
#define NUM_SIGNALS         64
// is there somewhere else I can get this value?
#define BITS_PER_WORD       64
#define SIGSET_SIZE (NUM_SIGNALS / BITS_PER_WORD)
#define SIGRTMIN 32
#define SIGRTMAX NUM_SIGNALS
#define RLIMIT_SIGPENDING 4096

/* Constants for setting masks w/ nk_signal_process_mask */
#define NKSIG_BLOCK           1
#define NKSIG MASK            2
#define NKSIG_SETMASK         3

/* Constants for sending signals */
/* Signal destinations (process or thread) */
#define SIG_DEST_TYPE_PROCESS     0
#define SIG_DEST_TYPE_THREAD      1

/* Sig result codes */
#define RES_SIGNAL_IGNORED      0

/* Sig action constants */
#ifndef SIG_ACT_NOCLDSTOP
#define SIG_ACT_NOCLDSTOP	0x00000001
#endif
#ifndef SIG_ACT_NOCLDWAIT
#define SIG_ACT_NOCLDWAIT	0x00000002
#endif
#ifndef SIG_ACT_SIGINFO
#define SIG_ACT_SIGINFO	0x00000004
#endif
#define SIG_ACT_UNSUPPORTED	0x00000400

/* TODO MAC: This might not be correct :) */
#define SIG_ACT_EXPOSE_TAGBITS	0x02000000

#ifndef SIG_ACT_ONSTACK
#define SIG_ACT_ONSTACK	0x08000000
#endif
#ifndef SIG_ACT_RESTART
#define SIG_ACT_RESTART	0x10000000
#endif
#ifndef SIG_ACT_NODEFER
#define SIG_ACT_NODEFER	0x40000000
#endif
#ifndef SIG_ACT_RESETHAND
#define SIG_ACT_RESETHAND	0x80000000
#endif
#define SIG_ACT_NOMASK SIG_ACT_NODEFER
#define SIG_ACT_ONESHOT SIG_ACT_RESETHAND

/*
 * si_code values
 * Digital reserves positive values for kernel-generated signals.
 */
#define SI_USER		0		/* sent by kill, sigsend, raise */
#define SI_KERNEL	0x80		/* sent by the kernel from somewhere */
#define SI_QUEUE	-1		/* sent by sigqueue */
#define SI_TIMER	-2		/* sent by timer expiration */
#define SI_MESGQ	-3		/* sent by real time mesq state change */
#define SI_ASYNCIO	-4		/* sent by AIO completion */
#define SI_SIGIO	-5		/* sent by queued SIGIO */
#define SI_TKILL	-6		/* sent by tkill system call */
#define SI_DETHREAD	-7		/* sent by execve() killing subsidiary threads */
#define SI_ASYNCNL	-60		/* sent by glibc async name lookup completion */

#define SI_FROMUSER(siptr)	((siptr)->signal_code <= 0)
#define SI_FROMKERNEL(siptr)	((siptr)->signal_code > 0)

/*
 * Bits in flags field of signal_struct.
 */
#define SIGNAL_STOP_STOPPED	0x00000001 /* job control stop in effect */
#define SIGNAL_STOP_CONTINUED	0x00000002 /* SIGCONT since WCONTINUED reap */
#define SIGNAL_GROUP_EXIT	0x00000004 /* group exit in progress */
#define SIGNAL_GROUP_COREDUMP	0x00000008 /* coredump in progress */

/*
 * Pending notifications to parent.
 */
#define SIGNAL_CLD_STOPPED	0x00000010
#define SIGNAL_CLD_CONTINUED	0x00000020
#define SIGNAL_CLD_MASK		(SIGNAL_CLD_STOPPED|SIGNAL_CLD_CONTINUED)

#define SIGNAL_UNKILLABLE	0x00000040 /* for init: ignore fatal signals */

#define SIGNAL_STOP_MASK (SIGNAL_CLD_MASK | SIGNAL_STOP_STOPPED | \
			  SIGNAL_STOP_CONTINUED) 
