#ifndef _SYSCALL_DECL
#define _SYSCALL_DECL

#include <nautilus/naut_types.h>
#include <nautilus/syscalls/types.h>

uint64_t sys_close(uint64_t fd);

uint64_t sys_exit(uint64_t exit_status);

uint64_t sys_fork();

uint64_t sys_fstat(uint64_t fd, uint64_t st);

uint64_t sys_ftruncate(uint64_t fd, uint64_t len);

uint64_t sys_lseek(uint64_t fd, uint64_t position, uint64_t whence);

uint64_t sys_open(uint64_t filename, uint64_t flags, uint64_t mode);

uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t len);

uint64_t sys_stat(uint64_t pathname, uint64_t st);

uint64_t sys_write(uint32_t fd, const char *buf, size_t len);

uint64_t sys_getpid();

uint64_t sys_gettimeofday(uint64_t timeval_ptr, uint64_t timezone_ptr);

uint64_t sys_settimeofday(uint64_t timeval_ptr, uint64_t timezone_ptr);

uint64_t sys_mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t flags, uint64_t fd, uint64_t offset);

uint64_t sys_mprotect(uint64_t addr, uint64_t length, uint64_t prot);

uint64_t sys_munmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t flags, uint64_t fd, uint64_t offset);

uint64_t sys_nanosleep(uint64_t req, uint64_t rem);

uint64_t sys_lstat(uint64_t filename, uint64_t statbuf);

uint64_t sys_poll(uint64_t ufds, uint64_t nfds, uint64_t timeout_msecs);

uint64_t sys_brk(uint64_t brk);

uint64_t sys_rt_sigaction(uint64_t sig, uint64_t act, uint64_t oact, uint64_t sigsetsize);

uint64_t sys_rt_sigprocmask(uint64_t how, uint64_t nset, uint64_t oset, uint64_t sigsetsize);

uint64_t sys_rt_sigreturn();

uint64_t sys_ioctl(uint64_t fd, uint64_t cmd, uint64_t arg);

uint64_t sys_pread64(uint64_t fd, uint64_t buf, uint64_t count, uint64_t pos);

uint64_t sys_pwrite64(uint64_t fd, uint64_t buf, uint64_t count, uint64_t pos);

uint64_t sys_readv(uint64_t fd, uint64_t vec, uint64_t vlen);

uint64_t sys_writev(uint64_t fd, uint64_t vec, uint64_t vlen);

uint64_t sys_access(uint64_t filename, uint64_t mode);

uint64_t sys_pipe(uint64_t fildes);

uint64_t sys_select(uint64_t n, uint64_t inp, uint64_t outp, uint64_t exp);

uint64_t sys_sched_yield();

uint64_t sys_mremap(uint64_t brk);

uint64_t sys_msync(uint64_t start, uint64_t len, uint64_t flags);

uint64_t sys_mincore(uint64_t start, uint64_t len, uint64_t vec);

uint64_t sys_madvise(uint64_t start, uint64_t len_in, uint64_t behavior);

uint64_t sys_shmget(uint64_t key, uint64_t size, uint64_t shmflg);

uint64_t sys_shmat(uint64_t shmid, uint64_t shmaddr, uint64_t shmflg);

uint64_t sys_shmctl(uint64_t shmid, uint64_t cmd, uint64_t buf);

uint64_t sys_dup(uint64_t fildes);

uint64_t sys_dup2(uint64_t oldfd, uint64_t newfd);

uint64_t sys_pause();

uint64_t sys_getitimer(uint64_t which, uint64_t value);

uint64_t sys_alarm(uint64_t seconds);

uint64_t sys_setitimer(uint64_t which, uint64_t value, uint64_t ovalue);

uint64_t sys_sendfile(uint64_t out_fd, uint64_t in_fd, uint64_t offset, uint64_t count);

uint64_t sys_socket(uint64_t family, uint64_t type, uint64_t protocol);

uint64_t sys_connect(uint64_t fd, uint64_t uservaddr, uint64_t addrlen);

uint64_t sys_accept(uint64_t fd, uint64_t upeer_sockaddr, uint64_t upeer_addrlen);

uint64_t sys_sendto(uint64_t fd, uint64_t buff, uint64_t len, uint64_t flags, uint64_t addr, uint64_t addr_len);

uint64_t sys_recvfrom(uint64_t fd, uint64_t ubuf, uint64_t size, uint64_t flags, uint64_t addr, uint64_t addr_len);

uint64_t sys_sendmsg(uint64_t fd, uint64_t msg, uint64_t flags);

uint64_t sys_recvmsg(uint64_t fd, uint64_t msg, uint64_t flags);

uint64_t sys_shutdown(uint64_t fd, uint64_t how);

uint64_t sys_bind(uint64_t fd, uint64_t umyaddr, uint64_t addrlen);

uint64_t sys_listen(uint64_t fd, uint64_t backlog);

uint64_t sys_getsockname(uint64_t usockaddr, uint64_t usockaddr_len);

uint64_t sys_getpeername(uint64_t usockaddr, uint64_t usockaddr_len);

uint64_t sys_socketpair(uint64_t family, uint64_t type, uint64_t protocol);

uint64_t sys_setsockopt(uint64_t usockvec);

uint64_t sys_getsockopt(uint64_t fd, uint64_t level, uint64_t optname, uint64_t optval, uint64_t optlen);

uint64_t sys_clone(uint64_t clone_flags, uint64_t newsp, uint32_t *parent_tidptr, uint32_t *child_tidptr, void *tls_val);

uint64_t sys_vfork();

uint64_t sys_execve(uint64_t filename, uint64_t argv, uint64_t envp);

uint64_t sys_wait4(uint64_t upid, uint64_t stat_addr, uint64_t options, uint64_t ru);

uint64_t sys_kill(uint64_t pid, uint64_t sig);

uint64_t sys_uname(uint64_t name);

uint64_t sys_semget(uint64_t key, uint64_t nsems, uint64_t semflg);

uint64_t sys_semop(uint64_t semid, uint64_t tsops, uint64_t nsops);

uint64_t sys_semctl(uint64_t semid, uint64_t semnum, uint64_t cmd, uint64_t arg);

uint64_t sys_shmdt(uint64_t shmaddr);

uint64_t sys_msgget(uint64_t key, uint64_t msgflg);

uint64_t sys_msgsnd(uint64_t msqid, uint64_t msgp, uint64_t msgsz, uint64_t msgflg);

uint64_t sys_msgrcv(uint64_t msqid, uint64_t msgp, uint64_t msgsz, uint64_t msgtyp, uint64_t msgflg);

uint64_t sys_msgctl(uint64_t msqid, uint64_t cmd, uint64_t buf);

uint64_t sys_fcntl(uint64_t fd, uint64_t cmd, uint64_t arg);

uint64_t sys_flock(uint64_t fd, uint64_t cmd);

uint64_t sys_fsync(uint64_t fd);

uint64_t sys_fdatasync(uint64_t fd);

uint64_t sys_truncate(uint64_t path, uint64_t length);

uint64_t sys_getdents(uint64_t fd, uint64_t dirent, uint64_t count);

uint64_t sys_getcwd(uint64_t buf, uint64_t size);

uint64_t sys_chdir(uint64_t filename);

uint64_t sys_fchdir(uint64_t fd);

uint64_t sys_rename(uint64_t oldname, uint64_t newname);

uint64_t sys_mkdir(uint64_t pathname, uint64_t mode);

uint64_t sys_rmdir(uint64_t pathname);

uint64_t sys_creat(uint64_t pathname, uint64_t mode);

uint64_t sys_link(uint64_t oldname, uint64_t newname);

uint64_t sys_unlink(uint64_t pathname);

uint64_t sys_symlink(uint64_t oldname, uint64_t newname);

uint64_t sys_readlink(uint64_t path, uint64_t buf, uint64_t bufsiz);

uint64_t sys_chmod(uint64_t filename, uint64_t mode);

uint64_t sys_fchmod(uint64_t fd, uint64_t mode);

uint64_t sys_chown(uint64_t filename, uint64_t uservaddr, uint64_t group);

uint64_t sys_fchown(uint64_t fd, uint64_t uservaddr, uint64_t group);

uint64_t sys_lchown(uint64_t filename, uint64_t uservaddr, uint64_t group);

uint64_t sys_umask(uint64_t mask);

uint64_t sys_getrlimit(uint64_t resource, uint64_t rlim);

uint64_t sys_getrusage(uint64_t who, uint64_t ru);

uint64_t sys_sysinfo(uint64_t info);

uint64_t sys_times(uint64_t tbuf);

uint64_t sys_ptrace(uint64_t request, uint64_t pid, uint64_t addrlen, uint64_t data);

uint64_t sys_getuid();

uint64_t sys_syslog(uint64_t type, uint64_t buf, uint64_t len);

uint64_t sys_getgid();

uint64_t sys_setuid(uint64_t uid);

uint64_t sys_setgid(uint64_t gid);

uint64_t sys_geteuid();

uint64_t sys_getegid();

uint64_t sys_setpgid(uint64_t pid, uint64_t pgid);

uint64_t sys_getppid();

uint64_t sys_getpgrp();

uint64_t sys_setsid();

uint64_t sys_setreuid(uint64_t ruid, uint64_t euid);

uint64_t sys_setregid(uint64_t rgid, uint64_t egid);

uint64_t sys_getgroups(uint64_t gidsetsize, uint64_t grouplist);

uint64_t sys_setgroups(uint64_t gidsetsize, uint64_t grouplist);

uint64_t sys_setresuid(uint64_t ruid, uint64_t euid, uint64_t suid);

uint64_t sys_getresuid(uint64_t ruidp, uint64_t euidp, uint64_t suidp);

uint64_t sys_setresgid(uint64_t rgid, uint64_t egid, uint64_t sgid);

uint64_t sys_getresgid(uint64_t rgidp, uint64_t egidp, uint64_t sgidp);

uint64_t sys_getpgid(uint64_t pid);

uint64_t sys_setfsuid(uint64_t uid);

uint64_t sys_setfsgid(uint64_t gid);

uint64_t sys_getsid(uint64_t pid);

uint64_t sys_capget(uint64_t header, uint64_t dataptr);

uint64_t sys_capset(uint64_t header, uint64_t data);

uint64_t sys_rt_sigpending(uint64_t uset, uint64_t sigsetsize);

uint64_t sys_rt_sigtimedwait(uint64_t uthese, uint64_t uinfo, uint64_t uts, uint64_t sigsetsize);

uint64_t sys_rt_sigqueueinfo(uint64_t pid, uint64_t sig, uint64_t uinfo);

uint64_t sys_rt_sigsuspend(uint64_t unewset, uint64_t sigsetsize);

uint64_t sys_sigaltstack(uint64_t uss, uint64_t uoss);

uint64_t sys_utime(uint64_t filename, uint64_t times);

uint64_t sys_mknod(uint64_t filename, uint64_t mode, uint64_t dev);

uint64_t sys_uselib(uint64_t library);

uint64_t sys_personality(uint64_t personality);

uint64_t sys_ustat(uint64_t dev, uint64_t ubuf);

uint64_t sys_statfs(uint64_t pathname, uint64_t buf);

uint64_t sys_fstatfs(uint64_t fd, uint64_t buf);

uint64_t sys_sysfs(uint64_t option, uint64_t arg1, uint64_t arg2);

uint64_t sys_getpriority(uint64_t which, uint64_t who);

uint64_t sys_setpriority(uint64_t which, uint64_t who, uint64_t niceval);

uint64_t sys_sched_setparam(uint64_t pid, uint64_t param);

uint64_t sys_sched_getparam(uint64_t pid, uint64_t param);

uint64_t sys_sched_setscheduler(uint64_t pid, uint64_t policy, uint64_t param);

uint64_t sys_sched_getscheduler(uint64_t pid);

uint64_t sys_sched_get_priority_max(uint64_t policy);

uint64_t sys_sched_get_priority_min(uint64_t policy);

uint64_t sys_sched_rr_get_interval(uint64_t pid, uint64_t interval);

uint64_t sys_mlock(uint64_t start, uint64_t len);

uint64_t sys_munlock(uint64_t start, uint64_t len);

uint64_t sys_mlockall(uint64_t start, uint64_t len);

uint64_t sys_munlockall();

uint64_t sys_vhangup();

uint64_t sys_modify_ldt(uint64_t func, uint64_t ptr, uint64_t bytecount);

uint64_t sys_pivot_root(uint64_t new_root, uint64_t put_old);

uint64_t sys__sysctl(uint64_t args);

uint64_t sys_prctl(uint64_t option, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

uint64_t sys_arch_prctl(uint64_t task, uint64_t code, uint64_t addrlen);

uint64_t sys_adjtimex(uint64_t txc_p);

uint64_t sys_setrlimit(uint64_t resource, uint64_t rlim);

uint64_t sys_chroot(uint64_t filename);

uint64_t sys_sync();

uint64_t sys_acct(uint64_t name);

uint64_t sys_mount(uint64_t dev_name, uint64_t dir_name, uint64_t type, uint64_t flags, uint64_t data);

uint64_t sys_umount2(uint64_t name, uint64_t flags);

uint64_t sys_swapon(uint64_t specialfile, uint64_t swap_flags);

uint64_t sys_swapoff(uint64_t specialfile);

uint64_t sys_reboot(uint64_t magic1, uint64_t magic2, uint64_t cmd, uint64_t arg);

uint64_t sys_sethostname(uint64_t name, uint64_t len);

uint64_t sys_setdomainname(uint64_t name, uint64_t len);

uint64_t sys_iopl(uint64_t level);

uint64_t sys_ioperm(uint64_t from, uint64_t num, uint64_t turn_on);

uint64_t sys_create_module();

uint64_t sys_init_module(uint64_t umod, uint64_t len, uint64_t uargs);

uint64_t sys_delete_module(uint64_t name_user, uint64_t flags);

uint64_t sys_get_kernel_syms();

uint64_t sys_query_module();

uint64_t sys_quotactl(uint64_t cmd, uint64_t special, uint64_t id, uint64_t addr);

uint64_t sys_nfsservctl();

uint64_t sys_getpmsg();

uint64_t sys_putpmsg();

uint64_t sys_afs_syscall();

uint64_t sys_tuxcall();

uint64_t sys_security();

uint64_t sys_gettid();

uint64_t sys_readahead(uint64_t fd, uint64_t offset, uint64_t count);

uint64_t sys_setxattr(uint64_t pathname, uint64_t name, uint64_t value, uint64_t size, uint64_t flags);

uint64_t sys_lsetxattr(uint64_t pathname, uint64_t name, uint64_t value, uint64_t size, uint64_t flags);

uint64_t sys_fsetxattr(uint64_t fd, uint64_t name, uint64_t value, uint64_t size, uint64_t flags);

uint64_t sys_getxattr(uint64_t pathname, uint64_t name, uint64_t value, uint64_t size);

uint64_t sys_lgetxattr(uint64_t pathname, uint64_t name, uint64_t value, uint64_t size);

uint64_t sys_fgetxattr(uint64_t fd, uint64_t name, uint64_t value, uint64_t size);

uint64_t sys_listxattr(uint64_t pathname, uint64_t list, uint64_t size);

uint64_t sys_llistxattr(uint64_t pathname, uint64_t list, uint64_t size);

uint64_t sys_flistxattr(uint64_t fd, uint64_t list, uint64_t size);

uint64_t sys_removexattr(uint64_t pathname, uint64_t name);

uint64_t sys_lremovexattr(uint64_t pathname, uint64_t name);

uint64_t sys_fremovexattr(uint64_t fd, uint64_t name);

uint64_t sys_tkill(uint64_t pid, uint64_t sig);

uint64_t sys_time(uint64_t tloc);

uint64_t sys_futex(uint32_t *uaddr, int op, uint32_t val, /*(struct timespec*)*/ void *utime, uint32_t *uaddr2, uint32_t val3);

uint64_t sys_sched_setaffinity(uint64_t pid, uint64_t len, uint64_t user_mask_ptr);

uint64_t sys_sched_getaffinity(uint64_t pid, uint64_t len, uint64_t user_mask_ptr);

uint64_t sys_set_thread_area(uint64_t u_unfo);

uint64_t sys_io_setup(uint64_t nr_events, uint64_t ctxp);

uint64_t sys_io_destroy(uint64_t ctx);

uint64_t sys_io_getevents(uint64_t ctx_id, uint64_t min_nr, uint64_t nr, uint64_t events, uint64_t timeout);

uint64_t sys_io_submit(uint64_t ctx_id, uint64_t nr, uint64_t iocbpp);

uint64_t sys_io_cancel(uint64_t ctx_id, uint64_t iocb, uint64_t result);

uint64_t sys_get_thread_area(uint64_t u_info);

uint64_t sys_lookup_dcookie(uint64_t cookie64, uint64_t buf, uint64_t len);

uint64_t sys_epoll_create(uint64_t size);

uint64_t sys_epoll_ctl_old();

uint64_t sys_epoll_wait_old();

uint64_t sys_remap_file_pages(uint64_t start, uint64_t size, uint64_t protocol, uint64_t pgoff, uint64_t flags);

uint64_t sys_getdents64(uint64_t fd, uint64_t dirent, uint64_t count);

uint64_t sys_set_tid_address(uint32_t *tldptr);

uint64_t sys_restart_syscall();

uint64_t sys_semtimedop(uint64_t semid, uint64_t tsops, uint64_t nsops, uint64_t timeout);

uint64_t sys_fadvise64(uint64_t fd, uint64_t offset, uint64_t len, uint64_t advice);

uint64_t sys_timer_create(uint64_t which_clock, uint64_t timer_event_spec, uint64_t creaded_timer_id);

uint64_t sys_timer_settime(uint64_t timer_id, uint64_t flags, uint64_t new_setting, uint64_t old_setting);

uint64_t sys_timer_gettime(uint64_t timer_id, uint64_t setting);

uint64_t sys_timer_getoverrun(uint64_t timer_id);

uint64_t sys_timer_delete(uint64_t timer_id);

uint64_t sys_clock_settime(uint64_t which_clock, uint64_t tp);

uint64_t sys_clock_gettime(uint64_t which_clock, uint64_t tp);

uint64_t sys_clock_getres(uint64_t which_clock, uint64_t tp);

uint64_t sys_clock_nanosleep(uint64_t which_clock, uint64_t flags, uint64_t rqtp, uint64_t rmtp);

uint64_t sys_exit_group(uint64_t error_code);

uint64_t sys_epoll_wait(uint64_t epfd, uint64_t events, uint64_t maxevents, uint64_t timeout);

uint64_t sys_epoll_ctl(uint64_t epfd, uint64_t op, uint64_t fd, uint64_t event);

uint64_t sys_tgkill(uint64_t tgid, uint64_t pid, uint64_t sig);

uint64_t sys_utimes(uint64_t filename, uint64_t utimes);

uint64_t sys_vserver();

uint64_t sys_mbind(uint64_t start, uint64_t len, uint64_t mode, uint64_t nmask, uint64_t maxnode, uint64_t flags);

uint64_t sys_set_mempolicy(uint64_t mode, uint64_t nmask, uint64_t maxnode);

uint64_t sys_get_mempolicy(uint64_t policy, uint64_t nmask, uint64_t maxnode, uint64_t addr, uint64_t flags);

uint64_t sys_mq_open(uint64_t u_name, uint64_t oflag, uint64_t mode, uint64_t u_attr);

uint64_t sys_mq_unlink(uint64_t u_name);

uint64_t sys_mq_timedsend(uint64_t mqdes, uint64_t u_msg_ptr, uint64_t msg_len, uint64_t msg_prio, uint64_t u_abs_timeout);

uint64_t sys_mq_timedreceive(uint64_t mqdes, uint64_t u_msg_ptr, uint64_t msg_len, uint64_t u_msg_prio, uint64_t u_abs_timeout);

uint64_t sys_mq_notify(uint64_t mqdes, uint64_t u_notification);

uint64_t sys_mq_getsetattr(uint64_t mqdes, uint64_t u_mqstat, uint64_t u_omqstat);

uint64_t sys_kexec_load(uint64_t entry, uint64_t nr_segments, uint64_t segments, uint64_t flags);

uint64_t sys_waitid(uint64_t which, uint64_t upid, uint64_t infop, uint64_t options, uint64_t ru);

uint64_t sys_add_key(uint64_t type, uint64_t description, uint64_t payload, uint64_t plen, uint64_t ringid);

uint64_t sys_request_key(uint64_t type, uint64_t description, uint64_t callout_info, uint64_t destringid);

uint64_t sys_keyctl(uint64_t option, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

uint64_t sys_ioprio_set(uint64_t which, uint64_t who, uint64_t ioprio);

uint64_t sys_ioprio_get(uint64_t which, uint64_t who);

uint64_t sys_inotify_init();

uint64_t sys_inotify_add_watch(uint64_t fd, uint64_t pathname, uint64_t mask);

uint64_t sys_inotify_rm_watch(uint64_t fd, uint64_t wd);

uint64_t sys_migrate_pages(uint64_t pid, uint64_t maxnode, uint64_t old_ndoes, uint64_t new_nodes);

uint64_t sys_openat(uint64_t dfd, uint64_t filename, uint64_t flags, uint64_t mode);

uint64_t sys_mkdirat(uint64_t dfd, uint64_t pathname, uint64_t mode);

uint64_t sys_mknodat(uint64_t dfd, uint64_t filename, uint64_t mode, uint64_t dev);

uint64_t sys_fchownat(uint64_t dfd, uint64_t filename, uint64_t user_mask_ptr, uint64_t group, uint64_t flag);

uint64_t sys_futimesat(uint64_t dfd, uint64_t filename, uint64_t utimes);

uint64_t sys_newfstatat(uint64_t dfd, uint64_t filename, uint64_t statbuf, uint64_t flags);

uint64_t sys_unlinkat(uint64_t dfd, uint64_t pathname, uint64_t flags);

uint64_t sys_renameat(uint64_t olddfd, uint64_t oldname, uint64_t newdfd, uint64_t newname);

uint64_t sys_linkat(uint64_t olddfd, uint64_t oldname, uint64_t newdfd, uint64_t newname, uint64_t flags);

uint64_t sys_symlinkat(uint64_t oldname, uint64_t newdfd, uint64_t newname);

uint64_t sys_readlinkat(uint64_t dfd, uint64_t pathname, uint64_t buf, uint64_t bufsiz);

uint64_t sys_fchmodat(uint64_t dfd, uint64_t filename, uint64_t mode);

uint64_t sys_faccessat(uint64_t dfd, uint64_t filename, uint64_t mode);

uint64_t sys_pselect6(uint64_t n, uint64_t inp, uint64_t outp, uint64_t exp, uint64_t tsp, uint64_t sig);

uint64_t sys_ppoll(uint64_t ufds, uint64_t nfds, uint64_t tsp, uint64_t sigmask, uint64_t sigsetsize);

uint64_t sys_unshare(uint64_t unshare_flags);

uint64_t sys_set_robust_list(uint64_t header, uint64_t len);

uint64_t sys_get_robust_list(uint64_t head_ptr, uint64_t len_ptr);

uint64_t sys_splice(uint64_t fd_in, uint64_t off_in, uint64_t fd_out, uint64_t off_out, uint64_t len, uint64_t flags);

uint64_t sys_tee(uint64_t fdin, uint64_t fdout, uint64_t len, uint64_t flags);

uint64_t sys_sync_file_range(uint64_t fd, uint64_t offset, uint64_t nbytes, uint64_t flags);

uint64_t sys_vmsplice(uint64_t fd, uint64_t iov, uint64_t nr_segs, uint64_t flags);

uint64_t sys_move_pages(uint64_t pid, uint64_t nr_pages, uint64_t pages, uint64_t nodes, uint64_t status, uint64_t flags);

uint64_t sys_utimensat(uint64_t dfd, uint64_t filename, uint64_t utimes, uint64_t flags);

uint64_t sys_epoll_pwait(uint64_t epfd, uint64_t events, uint64_t maxevents, uint64_t timeout, uint64_t sigmask, uint64_t sigsetsize);

uint64_t sys_signalfd(uint64_t ufd, uint64_t user_mask, uint64_t sizemask);

uint64_t sys_timerfd_create(uint64_t clockid, uint64_t flags);

uint64_t sys_eventfd(uint64_t count);

uint64_t sys_fallocate(uint64_t fd, uint64_t mode, uint64_t offset, uint64_t len);

uint64_t sys_timerfd_settime(uint64_t ufd, uint64_t flags, uint64_t utmr, uint64_t otmr);

uint64_t sys_timerfd_gettime(uint64_t ufd, uint64_t otmr);

uint64_t sys_accept4(uint64_t fd, uint64_t upeer_sockaddr, uint64_t upeer_addrlen, uint64_t flags);

uint64_t sys_signalfd4(uint64_t ufd, uint64_t user_mask, uint64_t sizemask, uint64_t flags);

uint64_t sys_eventfd2(uint64_t count);

uint64_t sys_epoll_create1(uint64_t flags);

uint64_t sys_dup3(uint64_t oldfd, uint64_t newfd, uint64_t flags);

uint64_t sys_pipe2(uint64_t fildes, uint64_t flags);

uint64_t sys_inotify_init1(uint64_t flags);

uint64_t sys_preadv(uint64_t fd, uint64_t vec, uint64_t vlen, uint64_t pos_l, uint64_t pos_h);

uint64_t sys_pwritev(uint64_t fd, uint64_t vec, uint64_t vlen, uint64_t pos_l, uint64_t pos_h);

uint64_t sys_rt_tgsigqueueinfo(uint64_t tgid, uint64_t pid, uint64_t sig, uint64_t uinfo);

uint64_t sys_perf_event_open(uint64_t attr_uptr, uint64_t pid, uint64_t cpu, uint64_t group_fd, uint64_t flags);

uint64_t sys_recvmmsg(uint64_t fd, uint64_t mmsg, uint64_t vlen, uint64_t flags, uint64_t timeout);

uint64_t sys_fanotify_init(uint64_t flags, uint64_t event_f_flags);

uint64_t sys_fanotify_mark(uint64_t fanotify_fd, uint64_t flags, uint64_t mask, uint64_t dfd, uint64_t pathname);

uint64_t sys_prlimit64(uint64_t pid, uint64_t resource, uint64_t new_rlim, uint64_t old_rlim);

uint64_t sys_name_to_handle_at(uint64_t dfd, uint64_t name, uint64_t handle, uint64_t mnt_id, uint64_t flag);

uint64_t sys_open_by_handle_at(uint64_t mountdirfd, uint64_t handle, uint64_t flags);

uint64_t sys_clock_adjtime(uint64_t which_clock, uint64_t utx);

uint64_t sys_syncfs(uint64_t fd);

uint64_t sys_sendmmsg(uint64_t fd, uint64_t mmsg, uint64_t vlen, uint64_t flags);

uint64_t sys_setns(uint64_t fd, uint64_t nstype);

uint64_t sys_getcpu(uint64_t cpup, uint64_t nodep, uint64_t unused);

uint64_t sys_process_vm_readv(uint64_t pid, uint64_t lvec, uint64_t liovcnt, uint64_t rvec, uint64_t riovcnt, uint64_t flags);

uint64_t sys_process_vm_writev(uint64_t pud, uint64_t lvec, uint64_t liovcnt, uint64_t rvec, uint64_t riovcnt, uint64_t flags);

uint64_t sys_kcmp(uint64_t pid1, uint64_t pid2, uint64_t type, uint64_t idx1, uint64_t idx2);

uint64_t sys_finit_module(uint64_t fd, uint64_t uargs, uint64_t flags);

#endif // _SYSCALL_DECL
