#include <nautilus/syscalls/decl.h>
#include <nautilus/syscalls/kernel.h>
#include <nautilus/syscalls/numbers.h>

#include <nautilus/irq.h>
#include <nautilus/msr.h>
#include <nautilus/nautilus.h>
#include <nautilus/process.h>
#include <nautilus/shell.h>
#include <nautilus/thread.h>

#define SYSCALL_NAME "syscall"
#include "impl_preamble.h"

typedef uint64_t (*syscall_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                              uint64_t);
syscall_t syscall_table[MAX_SYSCALL];

extern void syscall_entry(void);
void init_syscall_table() {
  int i;

  for (i = 0; i < MAX_SYSCALL; i++) {
    syscall_table[i] = 0;
  }

  syscall_table[READ] = (syscall_t)&sys_read;
  syscall_table[WRITE] = (syscall_t)&sys_write;
  syscall_table[OPEN] = (syscall_t)&sys_open;
  syscall_table[CLOSE] = (syscall_t)&sys_close;
  syscall_table[STAT] = (syscall_t)&sys_stat;
  syscall_table[FSTAT] = (syscall_t)&sys_fstat;
  syscall_table[LSEEK] = (syscall_t)&sys_lseek;
  syscall_table[FORK] = (syscall_t)&sys_fork;
  syscall_table[EXIT] = (syscall_t)&sys_exit;
  syscall_table[FTRUNCATE] = (syscall_t)&sys_ftruncate;
  syscall_table[GETPID] = (syscall_t)&sys_getpid;
  syscall_table[GETTIMEOFDAY] = (syscall_t)&sys_gettimeofday;
  syscall_table[SETTIMEOFDAY] = (syscall_t)&sys_settimeofday;
  syscall_table[MMAP] = (syscall_t)&sys_mmap;
  syscall_table[MPROTECT] = (syscall_t)&sys_mprotect;
  syscall_table[MUNMAP] = (syscall_t)&sys_munmap;
  syscall_table[NANOSLEEP] = (syscall_t)&sys_nanosleep;
  syscall_table[LSTAT] = (syscall_t)&sys_lstat;
  syscall_table[POLL] = (syscall_t)&sys_poll;
  syscall_table[BRK] = (syscall_t)&sys_brk;
  syscall_table[RT_SIGACTION] = (syscall_t)&sys_rt_sigaction;
  syscall_table[RT_SIGPROCMASK] = (syscall_t)&sys_rt_sigprocmask;
  syscall_table[RT_SIGRETURN] = (syscall_t)&sys_rt_sigreturn;
  syscall_table[IOCTL] = (syscall_t)&sys_ioctl;
  syscall_table[PREAD64] = (syscall_t)&sys_pread64;
  syscall_table[PWRITE64] = (syscall_t)&sys_pwrite64;
  syscall_table[READV] = (syscall_t)&sys_readv;
  syscall_table[WRITEV] = (syscall_t)&sys_writev;
  syscall_table[ACCESS] = (syscall_t)&sys_access;
  syscall_table[PIPE] = (syscall_t)&sys_pipe;
  syscall_table[SELECT] = (syscall_t)&sys_select;
  syscall_table[SCHED_YIELD] = (syscall_t)&sys_sched_yield;
  syscall_table[MREMAP] = (syscall_t)&sys_mremap;
  syscall_table[MSYNC] = (syscall_t)&sys_msync;
  syscall_table[MINCORE] = (syscall_t)&sys_mincore;
  syscall_table[MADVISE] = (syscall_t)&sys_madvise;
  syscall_table[SHMGET] = (syscall_t)&sys_shmget;
  syscall_table[SHMAT] = (syscall_t)&sys_shmat;
  syscall_table[SHMCTL] = (syscall_t)&sys_shmctl;
  syscall_table[DUP] = (syscall_t)&sys_dup;
  syscall_table[DUP2] = (syscall_t)&sys_dup2;
  syscall_table[PAUSE] = (syscall_t)&sys_pause;
  syscall_table[GETITIMER] = (syscall_t)&sys_getitimer;
  syscall_table[ALARM] = (syscall_t)&sys_alarm;
  syscall_table[SETITIMER] = (syscall_t)&sys_setitimer;
  syscall_table[SENDFILE] = (syscall_t)&sys_sendfile;
  syscall_table[SOCKET] = (syscall_t)&sys_socket;
  syscall_table[CONNECT] = (syscall_t)&sys_connect;
  syscall_table[ACCEPT] = (syscall_t)&sys_accept;
  syscall_table[SENDTO] = (syscall_t)&sys_sendto;
  syscall_table[RECVFROM] = (syscall_t)&sys_recvfrom;
  syscall_table[SENDMSG] = (syscall_t)&sys_sendmsg;
  syscall_table[RECVMSG] = (syscall_t)&sys_recvmsg;
  syscall_table[SHUTDOWN] = (syscall_t)&sys_shutdown;
  syscall_table[BIND] = (syscall_t)&sys_bind;
  syscall_table[LISTEN] = (syscall_t)&sys_listen;
  syscall_table[GETSOCKNAME] = (syscall_t)&sys_getsockname;
  syscall_table[GETPEERNAME] = (syscall_t)&sys_getpeername;
  syscall_table[SOCKETPAIR] = (syscall_t)&sys_socketpair;
  syscall_table[SETSOCKOPT] = (syscall_t)&sys_setsockopt;
  syscall_table[GETSOCKOPT] = (syscall_t)&sys_getsockopt;
  syscall_table[CLONE] = (syscall_t)&sys_clone;
  syscall_table[VFORK] = (syscall_t)&sys_vfork;
  syscall_table[EXECVE] = (syscall_t)&sys_execve;
  syscall_table[WAIT4] = (syscall_t)&sys_wait4;
  syscall_table[KILL] = (syscall_t)&sys_kill;
  syscall_table[UNAME] = (syscall_t)&sys_uname;
  syscall_table[SEMGET] = (syscall_t)&sys_semget;
  syscall_table[SEMOP] = (syscall_t)&sys_semop;
  syscall_table[SEMCTL] = (syscall_t)&sys_semctl;
  syscall_table[SHMDT] = (syscall_t)&sys_shmdt;
  syscall_table[MSGGET] = (syscall_t)&sys_msgget;
  syscall_table[MSGSND] = (syscall_t)&sys_msgsnd;
  syscall_table[MSGRCV] = (syscall_t)&sys_msgrcv;
  syscall_table[MSGCTL] = (syscall_t)&sys_msgctl;
  syscall_table[FCNTL] = (syscall_t)&sys_fcntl;
  syscall_table[FLOCK] = (syscall_t)&sys_flock;
  syscall_table[FSYNC] = (syscall_t)&sys_fsync;
  syscall_table[FDATASYNC] = (syscall_t)&sys_fdatasync;
  syscall_table[TRUNCATE] = (syscall_t)&sys_truncate;
  syscall_table[GETDENTS] = (syscall_t)&sys_getdents;
  syscall_table[GETCWD] = (syscall_t)&sys_getcwd;
  syscall_table[CHDIR] = (syscall_t)&sys_chdir;
  syscall_table[FCHDIR] = (syscall_t)&sys_fchdir;
  syscall_table[RENAME] = (syscall_t)&sys_rename;
  syscall_table[MKDIR] = (syscall_t)&sys_mkdir;
  syscall_table[RMDIR] = (syscall_t)&sys_rmdir;
  syscall_table[CREAT] = (syscall_t)&sys_creat;
  syscall_table[LINK] = (syscall_t)&sys_link;
  syscall_table[UNLINK] = (syscall_t)&sys_unlink;
  syscall_table[SYMLINK] = (syscall_t)&sys_symlink;
  syscall_table[READLINK] = (syscall_t)&sys_readlink;
  syscall_table[CHMOD] = (syscall_t)&sys_chmod;
  syscall_table[FCHMOD] = (syscall_t)&sys_fchmod;
  syscall_table[CHOWN] = (syscall_t)&sys_chown;
  syscall_table[FCHOWN] = (syscall_t)&sys_fchown;
  syscall_table[LCHOWN] = (syscall_t)&sys_lchown;
  syscall_table[UMASK] = (syscall_t)&sys_umask;
  syscall_table[GETRLIMIT] = (syscall_t)&sys_getrlimit;
  syscall_table[GETRUSAGE] = (syscall_t)&sys_getrusage;
  syscall_table[SYSINFO] = (syscall_t)&sys_sysinfo;
  syscall_table[TIMES] = (syscall_t)&sys_times;
  syscall_table[PTRACE] = (syscall_t)&sys_ptrace;
  syscall_table[GETUID] = (syscall_t)&sys_getuid;
  syscall_table[SYSLOG] = (syscall_t)&sys_syslog;
  syscall_table[GETGID] = (syscall_t)&sys_getgid;
  syscall_table[SETUID] = (syscall_t)&sys_setuid;
  syscall_table[SETGID] = (syscall_t)&sys_setgid;
  syscall_table[GETEUID] = (syscall_t)&sys_geteuid;
  syscall_table[GETEGID] = (syscall_t)&sys_getegid;
  syscall_table[SETPGID] = (syscall_t)&sys_setpgid;
  syscall_table[GETPPID] = (syscall_t)&sys_getppid;
  syscall_table[GETPGRP] = (syscall_t)&sys_getpgrp;
  syscall_table[SETSID] = (syscall_t)&sys_setsid;
  syscall_table[SETREUID] = (syscall_t)&sys_setreuid;
  syscall_table[SETREGID] = (syscall_t)&sys_setregid;
  syscall_table[GETGROUPS] = (syscall_t)&sys_getgroups;
  syscall_table[SETGROUPS] = (syscall_t)&sys_setgroups;
  syscall_table[SETRESUID] = (syscall_t)&sys_setresuid;
  syscall_table[GETRESUID] = (syscall_t)&sys_getresuid;
  syscall_table[SETRESGID] = (syscall_t)&sys_setresgid;
  syscall_table[GETRESGID] = (syscall_t)&sys_getresgid;
  syscall_table[GETPGID] = (syscall_t)&sys_getpgid;
  syscall_table[SETFSUID] = (syscall_t)&sys_setfsuid;
  syscall_table[SETFSGID] = (syscall_t)&sys_setfsgid;
  syscall_table[GETSID] = (syscall_t)&sys_getsid;
  syscall_table[CAPGET] = (syscall_t)&sys_capget;
  syscall_table[CAPSET] = (syscall_t)&sys_capset;
  syscall_table[RT_SIGPENDING] = (syscall_t)&sys_rt_sigpending;
  syscall_table[RT_SIGTIMEDWAIT] = (syscall_t)&sys_rt_sigtimedwait;
  syscall_table[RT_SIGQUEUEINFO] = (syscall_t)&sys_rt_sigqueueinfo;
  syscall_table[RT_SIGSUSPEND] = (syscall_t)&sys_rt_sigsuspend;
  syscall_table[SIGALTSTACK] = (syscall_t)&sys_sigaltstack;
  syscall_table[UTIME] = (syscall_t)&sys_utime;
  syscall_table[MKNOD] = (syscall_t)&sys_mknod;
  syscall_table[USELIB] = (syscall_t)&sys_uselib;
  syscall_table[PERSONALITY] = (syscall_t)&sys_personality;
  syscall_table[USTAT] = (syscall_t)&sys_ustat;
  syscall_table[STATFS] = (syscall_t)&sys_statfs;
  syscall_table[FSTATFS] = (syscall_t)&sys_fstatfs;
  syscall_table[SYSFS] = (syscall_t)&sys_sysfs;
  syscall_table[GETPRIORITY] = (syscall_t)&sys_getpriority;
  syscall_table[SETPRIORITY] = (syscall_t)&sys_setpriority;
  syscall_table[SCHED_SETPARAM] = (syscall_t)&sys_sched_setparam;
  syscall_table[SCHED_GETPARAM] = (syscall_t)&sys_sched_getparam;
  syscall_table[SCHED_SETSCHEDULER] = (syscall_t)&sys_sched_setscheduler;
  syscall_table[SCHED_GETSCHEDULER] = (syscall_t)&sys_sched_getscheduler;
  syscall_table[SCHED_GET_PRIORITY_MAX] =
      (syscall_t)&sys_sched_get_priority_max;
  syscall_table[SCHED_GET_PRIORITY_MIN] =
      (syscall_t)&sys_sched_get_priority_min;
  syscall_table[SCHED_RR_GET_INTERVAL] = (syscall_t)&sys_sched_rr_get_interval;
  syscall_table[MLOCK] = (syscall_t)&sys_mlock;
  syscall_table[MUNLOCK] = (syscall_t)&sys_munlock;
  syscall_table[MLOCKALL] = (syscall_t)&sys_mlockall;
  syscall_table[MUNLOCKALL] = (syscall_t)&sys_munlockall;
  syscall_table[VHANGUP] = (syscall_t)&sys_vhangup;
  syscall_table[MODIFY_LDT] = (syscall_t)&sys_modify_ldt;
  syscall_table[PIVOT_ROOT] = (syscall_t)&sys_pivot_root;
  syscall_table[_SYSCTL] = (syscall_t)&sys__sysctl;
  syscall_table[PRCTL] = (syscall_t)&sys_prctl;
  syscall_table[ARCH_PRCTL] = (syscall_t)&sys_arch_prctl;
  syscall_table[ADJTIMEX] = (syscall_t)&sys_adjtimex;
  syscall_table[SETRLIMIT] = (syscall_t)&sys_setrlimit;
  syscall_table[CHROOT] = (syscall_t)&sys_chroot;
  syscall_table[SYNC] = (syscall_t)&sys_sync;
  syscall_table[ACCT] = (syscall_t)&sys_acct;
  syscall_table[MOUNT] = (syscall_t)&sys_mount;
  syscall_table[UMOUNT2] = (syscall_t)&sys_umount2;
  syscall_table[SWAPON] = (syscall_t)&sys_swapon;
  syscall_table[SWAPOFF] = (syscall_t)&sys_swapoff;
  syscall_table[REBOOT] = (syscall_t)&sys_reboot;
  syscall_table[SETHOSTNAME] = (syscall_t)&sys_sethostname;
  syscall_table[SETDOMAINNAME] = (syscall_t)&sys_setdomainname;
  syscall_table[IOPL] = (syscall_t)&sys_iopl;
  syscall_table[IOPERM] = (syscall_t)&sys_ioperm;
  syscall_table[CREATE_MODULE] = (syscall_t)&sys_create_module;
  syscall_table[INIT_MODULE] = (syscall_t)&sys_init_module;
  syscall_table[DELETE_MODULE] = (syscall_t)&sys_delete_module;
  syscall_table[GET_KERNEL_SYMS] = (syscall_t)&sys_get_kernel_syms;
  syscall_table[QUERY_MODULE] = (syscall_t)&sys_query_module;
  syscall_table[QUOTACTL] = (syscall_t)&sys_quotactl;
  syscall_table[NFSSERVCTL] = (syscall_t)&sys_nfsservctl;
  syscall_table[GETPMSG] = (syscall_t)&sys_getpmsg;
  syscall_table[PUTPMSG] = (syscall_t)&sys_putpmsg;
  syscall_table[AFS_SYSCALL] = (syscall_t)&sys_afs_syscall;
  syscall_table[TUXCALL] = (syscall_t)&sys_tuxcall;
  syscall_table[SECURITY] = (syscall_t)&sys_security;
  syscall_table[GETTID] = (syscall_t)&sys_gettid;
  syscall_table[READAHEAD] = (syscall_t)&sys_readahead;
  syscall_table[SETXATTR] = (syscall_t)&sys_setxattr;
  syscall_table[LSETXATTR] = (syscall_t)&sys_lsetxattr;
  syscall_table[FSETXATTR] = (syscall_t)&sys_fsetxattr;
  syscall_table[GETXATTR] = (syscall_t)&sys_getxattr;
  syscall_table[LGETXATTR] = (syscall_t)&sys_lgetxattr;
  syscall_table[FGETXATTR] = (syscall_t)&sys_fgetxattr;
  syscall_table[LISTXATTR] = (syscall_t)&sys_listxattr;
  syscall_table[LLISTXATTR] = (syscall_t)&sys_llistxattr;
  syscall_table[FLISTXATTR] = (syscall_t)&sys_flistxattr;
  syscall_table[REMOVEXATTR] = (syscall_t)&sys_removexattr;
  syscall_table[LREMOVEXATTR] = (syscall_t)&sys_lremovexattr;
  syscall_table[FREMOVEXATTR] = (syscall_t)&sys_fremovexattr;
  syscall_table[TKILL] = (syscall_t)&sys_tkill;
  syscall_table[TIME] = (syscall_t)&sys_time;
  syscall_table[FUTEX] = (syscall_t)&sys_futex;
  syscall_table[SCHED_SETAFFINITY] = (syscall_t)&sys_sched_setaffinity;
  syscall_table[SCHED_GETAFFINITY] = (syscall_t)&sys_sched_getaffinity;
  syscall_table[SET_THREAD_AREA] = (syscall_t)&sys_set_thread_area;
  syscall_table[IO_SETUP] = (syscall_t)&sys_io_setup;
  syscall_table[IO_DESTROY] = (syscall_t)&sys_io_destroy;
  syscall_table[IO_GETEVENTS] = (syscall_t)&sys_io_getevents;
  syscall_table[IO_SUBMIT] = (syscall_t)&sys_io_submit;
  syscall_table[IO_CANCEL] = (syscall_t)&sys_io_cancel;
  syscall_table[GET_THREAD_AREA] = (syscall_t)&sys_get_thread_area;
  syscall_table[LOOKUP_DCOOKIE] = (syscall_t)&sys_lookup_dcookie;
  syscall_table[EPOLL_CREATE] = (syscall_t)&sys_epoll_create;
  syscall_table[EPOLL_CTL_OLD] = (syscall_t)&sys_epoll_ctl_old;
  syscall_table[EPOLL_WAIT_OLD] = (syscall_t)&sys_epoll_wait_old;
  syscall_table[REMAP_FILE_PAGES] = (syscall_t)&sys_remap_file_pages;
  syscall_table[GETDENTS64] = (syscall_t)&sys_getdents64;
  syscall_table[SET_TID_ADDRESS] = (syscall_t)&sys_set_tid_address;
  syscall_table[RESTART_SYSCALL] = (syscall_t)&sys_restart_syscall;
  syscall_table[SEMTIMEDOP] = (syscall_t)&sys_semtimedop;
  syscall_table[FADVISE64] = (syscall_t)&sys_fadvise64;
  syscall_table[TIMER_CREATE] = (syscall_t)&sys_timer_create;
  syscall_table[TIMER_SETTIME] = (syscall_t)&sys_timer_settime;
  syscall_table[TIMER_GETTIME] = (syscall_t)&sys_timer_gettime;
  syscall_table[TIMER_GETOVERRUN] = (syscall_t)&sys_timer_getoverrun;
  syscall_table[TIMER_DELETE] = (syscall_t)&sys_timer_delete;
  syscall_table[CLOCK_SETTIME] = (syscall_t)&sys_clock_settime;
  syscall_table[CLOCK_GETTIME] = (syscall_t)&sys_clock_gettime;
  syscall_table[CLOCK_GETRES] = (syscall_t)&sys_clock_getres;
  syscall_table[CLOCK_NANOSLEEP] = (syscall_t)&sys_clock_nanosleep;
  syscall_table[EXIT_GROUP] = (syscall_t)&sys_exit_group;
  syscall_table[EPOLL_WAIT] = (syscall_t)&sys_epoll_wait;
  syscall_table[EPOLL_CTL] = (syscall_t)&sys_epoll_ctl;
  syscall_table[TGKILL] = (syscall_t)&sys_tgkill;
  syscall_table[UTIMES] = (syscall_t)&sys_utimes;
  syscall_table[VSERVER] = (syscall_t)&sys_vserver;
  syscall_table[MBIND] = (syscall_t)&sys_mbind;
  syscall_table[SET_MEMPOLICY] = (syscall_t)&sys_set_mempolicy;
  syscall_table[GET_MEMPOLICY] = (syscall_t)&sys_get_mempolicy;
  syscall_table[MQ_OPEN] = (syscall_t)&sys_mq_open;
  syscall_table[MQ_UNLINK] = (syscall_t)&sys_mq_unlink;
  syscall_table[MQ_TIMEDSEND] = (syscall_t)&sys_mq_timedsend;
  syscall_table[MQ_TIMEDRECEIVE] = (syscall_t)&sys_mq_timedreceive;
  syscall_table[MQ_NOTIFY] = (syscall_t)&sys_mq_notify;
  syscall_table[MQ_GETSETATTR] = (syscall_t)&sys_mq_getsetattr;
  syscall_table[KEXEC_LOAD] = (syscall_t)&sys_kexec_load;
  syscall_table[WAITID] = (syscall_t)&sys_waitid;
  syscall_table[ADD_KEY] = (syscall_t)&sys_add_key;
  syscall_table[REQUEST_KEY] = (syscall_t)&sys_request_key;
  syscall_table[KEYCTL] = (syscall_t)&sys_keyctl;
  syscall_table[IOPRIO_SET] = (syscall_t)&sys_ioprio_set;
  syscall_table[IOPRIO_GET] = (syscall_t)&sys_ioprio_get;
  syscall_table[INOTIFY_INIT] = (syscall_t)&sys_inotify_init;
  syscall_table[INOTIFY_ADD_WATCH] = (syscall_t)&sys_inotify_add_watch;
  syscall_table[INOTIFY_RM_WATCH] = (syscall_t)&sys_inotify_rm_watch;
  syscall_table[MIGRATE_PAGES] = (syscall_t)&sys_migrate_pages;
  syscall_table[OPENAT] = (syscall_t)&sys_openat;
  syscall_table[MKDIRAT] = (syscall_t)&sys_mkdirat;
  syscall_table[MKNODAT] = (syscall_t)&sys_mknodat;
  syscall_table[FCHOWNAT] = (syscall_t)&sys_fchownat;
  syscall_table[FUTIMESAT] = (syscall_t)&sys_futimesat;
  syscall_table[NEWFSTATAT] = (syscall_t)&sys_newfstatat;
  syscall_table[UNLINKAT] = (syscall_t)&sys_unlinkat;
  syscall_table[RENAMEAT] = (syscall_t)&sys_renameat;
  syscall_table[LINKAT] = (syscall_t)&sys_linkat;
  syscall_table[SYMLINKAT] = (syscall_t)&sys_symlinkat;
  syscall_table[READLINKAT] = (syscall_t)&sys_readlinkat;
  syscall_table[FCHMODAT] = (syscall_t)&sys_fchmodat;
  syscall_table[FACCESSAT] = (syscall_t)&sys_faccessat;
  syscall_table[PSELECT6] = (syscall_t)&sys_pselect6;
  syscall_table[PPOLL] = (syscall_t)&sys_ppoll;
  syscall_table[UNSHARE] = (syscall_t)&sys_unshare;
  syscall_table[SET_ROBUST_LIST] = (syscall_t)&sys_set_robust_list;
  syscall_table[GET_ROBUST_LIST] = (syscall_t)&sys_get_robust_list;
  syscall_table[SPLICE] = (syscall_t)&sys_splice;
  syscall_table[TEE] = (syscall_t)&sys_tee;
  syscall_table[SYNC_FILE_RANGE] = (syscall_t)&sys_sync_file_range;
  syscall_table[VMSPLICE] = (syscall_t)&sys_vmsplice;
  syscall_table[MOVE_PAGES] = (syscall_t)&sys_move_pages;
  syscall_table[UTIMENSAT] = (syscall_t)&sys_utimensat;
  syscall_table[EPOLL_PWAIT] = (syscall_t)&sys_epoll_pwait;
  syscall_table[SIGNALFD] = (syscall_t)&sys_signalfd;
  syscall_table[TIMERFD_CREATE] = (syscall_t)&sys_timerfd_create;
  syscall_table[EVENTFD] = (syscall_t)&sys_eventfd;
  syscall_table[FALLOCATE] = (syscall_t)&sys_fallocate;
  syscall_table[TIMERFD_SETTIME] = (syscall_t)&sys_timerfd_settime;
  syscall_table[TIMERFD_GETTIME] = (syscall_t)&sys_timerfd_gettime;
  syscall_table[ACCEPT4] = (syscall_t)&sys_accept4;
  syscall_table[SIGNALFD4] = (syscall_t)&sys_signalfd4;
  syscall_table[EVENTFD2] = (syscall_t)&sys_eventfd2;
  syscall_table[EPOLL_CREATE1] = (syscall_t)&sys_epoll_create1;
  syscall_table[DUP3] = (syscall_t)&sys_dup3;
  syscall_table[PIPE2] = (syscall_t)&sys_pipe2;
  syscall_table[INOTIFY_INIT1] = (syscall_t)&sys_inotify_init1;
  syscall_table[PREADV] = (syscall_t)&sys_preadv;
  syscall_table[PWRITEV] = (syscall_t)&sys_pwritev;
  syscall_table[RT_TGSIGQUEUEINFO] = (syscall_t)&sys_rt_tgsigqueueinfo;
  syscall_table[PERF_EVENT_OPEN] = (syscall_t)&sys_perf_event_open;
  syscall_table[RECVMMSG] = (syscall_t)&sys_recvmmsg;
  syscall_table[FANOTIFY_INIT] = (syscall_t)&sys_fanotify_init;
  syscall_table[FANOTIFY_MARK] = (syscall_t)&sys_fanotify_mark;
  syscall_table[PRLIMIT64] = (syscall_t)&sys_prlimit64;
  syscall_table[NAME_TO_HANDLE_AT] = (syscall_t)&sys_name_to_handle_at;
  syscall_table[OPEN_BY_HANDLE_AT] = (syscall_t)&sys_open_by_handle_at;
  syscall_table[CLOCK_ADJTIME] = (syscall_t)&sys_clock_adjtime;
  syscall_table[SYNCFS] = (syscall_t)&sys_syncfs;
  syscall_table[SENDMMSG] = (syscall_t)&sys_sendmmsg;
  syscall_table[SETNS] = (syscall_t)&sys_setns;
  syscall_table[GETCPU] = (syscall_t)&sys_getcpu;
  syscall_table[PROCESS_VM_READV] = (syscall_t)&sys_process_vm_readv;
  syscall_table[PROCESS_VM_WRITEV] = (syscall_t)&sys_process_vm_writev;
  syscall_table[KCMP] = (syscall_t)&sys_kcmp;
  syscall_table[FINIT_MODULE] = (syscall_t)&sys_finit_module;

  return;
}

int int80_handler(excp_entry_t* excp, excp_vec_t vector, void* state) {

  struct nk_regs* r = (struct nk_regs*)((char*)excp - 128);
  return nk_syscall_handler(r);
}

uint64_t nk_syscall_handler(struct nk_regs* r) {

#ifdef NAUT_CONFIG_DEBUG_LINUX_SYSCALLS
  if (!irqs_enabled()) {
    // panic("Start syscall with interrupts off!");
  }
#endif

  int syscall_nr = (int)r->rax;
  nk_process_t* current_process = syscall_get_proc();
  DEBUG("Inside syscall handler for syscall %d\n", syscall_nr);

  get_cur_thread()->sysret_addr =
      (void*)r->rcx; /* Used for special return in clone */
  if (syscall_table[syscall_nr] != 0) {
    r->rax =
        syscall_table[syscall_nr](r->rdi, r->rsi, r->rdx, r->r10, r->r8, r->r9);
    DEBUG("Syscall returned %ld\n", r->rax);
  } else {
    DEBUG("System Call not Implemented: %d!!\n", syscall_nr);
  }

#ifdef NAUT_CONFIG_DEBUG_LINUX_SYSCALLS
  if (!irqs_enabled()) {
    // panic("Return from syscall with interrupts off!");
  }
#endif

  return r->rax;
}

int syscall_setup() {
  uint64_t r = msr_read(IA32_MSR_EFER);
  r |= EFER_SCE;
  msr_write(IA32_MSR_EFER, r);

  /* SYSCALL and SYSRET CS in upper 32 bits */
  msr_write(AMD_MSR_STAR,
            ((0x8llu & 0xffffllu) << 32) | ((0x8llu & 0xffffllu) << 48));

  /* target address */
  msr_write(AMD_MSR_LSTAR, (uint64_t)syscall_entry);

  /* Don't clear RFLAGS bits on syscall entry.
     Consider: trap, direction (possible attack vector), resume, virtual 8086, alignment check, VIF, VIP */
  msr_write(AMD_MSR_SFMASK, 0ULL);

  return 0;
}

void nk_syscall_init() {
  register_int_handler(0x80, int80_handler, 0);
  syscall_setup();
}

void nk_syscall_init_ap() { syscall_setup(); }
