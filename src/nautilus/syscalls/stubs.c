#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_stubs"
#include "impl_preamble.h"

uint64_t sys_lstat(uint64_t filename, uint64_t statbuf) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (lstat)\n");
  return -1;
}

uint64_t sys_poll(uint64_t ufds, uint64_t nfds, uint64_t timeout_msecs) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (poll)\n");
  return -1;
}

uint64_t sys_rt_sigreturn() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (rt_sigreturn)\n");
  return -1;
}

uint64_t sys_ioctl(uint64_t fd, uint64_t cmd, uint64_t arg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (ioctl)\n");
  return -1;
}

uint64_t sys_pread64(uint64_t fd, uint64_t buf, uint64_t count, uint64_t pos) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (pread64)\n");
  return -1;
}

uint64_t sys_pwrite64(uint64_t fd, uint64_t buf, uint64_t count, uint64_t pos) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (pwrite64)\n");
  return -1;
}

uint64_t sys_readv(uint64_t fd, uint64_t vec, uint64_t vlen) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (readv)\n");
  return -1;
}

uint64_t sys_access(uint64_t filename, uint64_t mode) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (access)\n");
  return -1;
}

uint64_t sys_pipe(uint64_t fildes) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (pipe)\n");
  return -1;
}

uint64_t sys_select(uint64_t n, uint64_t inp, uint64_t outp, uint64_t exp) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (select)\n");
  return -1;
}

uint64_t sys_mremap(uint64_t brk) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mremap)\n");
  return -1;
}

uint64_t sys_msync(uint64_t start, uint64_t len, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (msync)\n");
  return -1;
}

uint64_t sys_mincore(uint64_t start, uint64_t len, uint64_t vec) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mincore)\n");
  return -1;
}

uint64_t sys_shmget(uint64_t key, uint64_t size, uint64_t shmflg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (shmget)\n");
  return -1;
}

uint64_t sys_shmat(uint64_t shmid, uint64_t shmaddr, uint64_t shmflg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (shmat)\n");
  return -1;
}

uint64_t sys_shmctl(uint64_t shmid, uint64_t cmd, uint64_t buf) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (shmctl)\n");
  return -1;
}

uint64_t sys_dup(uint64_t fildes) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (dup)\n");
  return -1;
}

uint64_t sys_dup2(uint64_t oldfd, uint64_t newfd) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (dup2)\n");
  return -1;
}

uint64_t sys_pause() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (pause)\n");
  return -1;
}

uint64_t sys_getitimer(uint64_t which, uint64_t value) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getitimer)\n");
  return -1;
}

uint64_t sys_alarm(uint64_t seconds) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (alarm)\n");
  return -1;
}

uint64_t sys_setitimer(uint64_t which, uint64_t value, uint64_t ovalue) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setitimer)\n");
  return -1;
}

uint64_t sys_sendfile(uint64_t out_fd, uint64_t in_fd, uint64_t offset,
                      uint64_t count) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sendfile)\n");
  return -1;
}

uint64_t sys_socket(uint64_t family, uint64_t type, uint64_t protocol) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (socket)\n");
  return -1;
}

uint64_t sys_connect(uint64_t fd, uint64_t uservaddr, uint64_t addrlen) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (connect)\n");
  return -1;
}

uint64_t sys_accept(uint64_t fd, uint64_t upeer_sockaddr,
                    uint64_t upeer_addrlen) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (accept)\n");
  return -1;
}

uint64_t sys_sendto(uint64_t fd, uint64_t buff, uint64_t len, uint64_t flags,
                    uint64_t addr, uint64_t addr_len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sendto)\n");
  return -1;
}

uint64_t sys_recvfrom(uint64_t fd, uint64_t ubuf, uint64_t size, uint64_t flags,
                      uint64_t addr, uint64_t addr_len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (recvfrom)\n");
  return -1;
}

uint64_t sys_sendmsg(uint64_t fd, uint64_t msg, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sendmsg)\n");
  return -1;
}

uint64_t sys_recvmsg(uint64_t fd, uint64_t msg, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (recvmsg)\n");
  return -1;
}

uint64_t sys_shutdown(uint64_t fd, uint64_t how) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (shutdown)\n");
  return -1;
}

uint64_t sys_bind(uint64_t fd, uint64_t umyaddr, uint64_t addrlen) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (bind)\n");
  return -1;
}

uint64_t sys_listen(uint64_t fd, uint64_t backlog) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (listen)\n");
  return -1;
}

uint64_t sys_getsockname(uint64_t usockaddr, uint64_t usockaddr_len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getsockname)\n");
  return -1;
}

uint64_t sys_getpeername(uint64_t usockaddr, uint64_t usockaddr_len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getpeername)\n");
  return -1;
}

uint64_t sys_socketpair(uint64_t family, uint64_t type, uint64_t protocol) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (socketpair)\n");
  return -1;
}

uint64_t sys_setsockopt(uint64_t usockvec) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setsockopt)\n");
  return -1;
}

uint64_t sys_getsockopt(uint64_t fd, uint64_t level, uint64_t optname,
                        uint64_t optval, uint64_t optlen) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getsockopt)\n");
  return -1;
}

uint64_t sys_vfork() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (vfork)\n");
  return -1;
}

uint64_t sys_execve(uint64_t filename, uint64_t argv, uint64_t envp) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (execve)\n");
  return -1;
}

uint64_t sys_wait4(uint64_t upid, uint64_t stat_addr, uint64_t options,
                   uint64_t ru) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (wait4)\n");
  return -1;
}

uint64_t sys_kill(uint64_t pid, uint64_t sig) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (kill)\n");
  return -1;
}

uint64_t sys_semget(uint64_t key, uint64_t nsems, uint64_t semflg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (semget)\n");
  return -1;
}

uint64_t sys_semop(uint64_t semid, uint64_t tsops, uint64_t nsops) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (semop)\n");
  return -1;
}

uint64_t sys_semctl(uint64_t semid, uint64_t semnum, uint64_t cmd,
                    uint64_t arg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (semctl)\n");
  return -1;
}

uint64_t sys_shmdt(uint64_t shmaddr) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (shmdt)\n");
  return -1;
}

uint64_t sys_msgget(uint64_t key, uint64_t msgflg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (msgget)\n");
  return -1;
}

uint64_t sys_msgsnd(uint64_t msqid, uint64_t msgp, uint64_t msgsz,
                    uint64_t msgflg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (msgsnd)\n");
  return -1;
}

uint64_t sys_msgrcv(uint64_t msqid, uint64_t msgp, uint64_t msgsz,
                    uint64_t msgtyp, uint64_t msgflg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (msgrcv)\n");
  return -1;
}

uint64_t sys_msgctl(uint64_t msqid, uint64_t cmd, uint64_t buf) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (msgctl)\n");
  return -1;
}

uint64_t sys_fcntl(uint64_t fd, uint64_t cmd, uint64_t arg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fcntl)\n");
  return -1;
}

uint64_t sys_flock(uint64_t fd, uint64_t cmd) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (flock)\n");
  return -1;
}

uint64_t sys_fsync(uint64_t fd) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fsync)\n");
  return -1;
}

uint64_t sys_fdatasync(uint64_t fd) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fdatasync)\n");
  return -1;
}

uint64_t sys_truncate(uint64_t path, uint64_t length) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (truncate)\n");
  return -1;
}

uint64_t sys_getdents(uint64_t fd, uint64_t dirent, uint64_t count) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getdents)\n");
  return -1;
}

uint64_t sys_getcwd(uint64_t buf, uint64_t size) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getcwd)\n");
  return -1;
}

uint64_t sys_chdir(uint64_t filename) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (chdir)\n");
  return -1;
}

uint64_t sys_fchdir(uint64_t fd) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fchdir)\n");
  return -1;
}

uint64_t sys_rename(uint64_t oldname, uint64_t newname) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (rename)\n");
  return -1;
}

uint64_t sys_rmdir(uint64_t pathname) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (rmdir)\n");
  return -1;
}

uint64_t sys_creat(uint64_t pathname, uint64_t mode) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (creat)\n");
  return -1;
}

uint64_t sys_link(uint64_t oldname, uint64_t newname) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (link)\n");
  return -1;
}

uint64_t sys_unlink(uint64_t pathname) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (unlink)\n");
  return -1;
}

uint64_t sys_symlink(uint64_t oldname, uint64_t newname) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (symlink)\n");
  return -1;
}

uint64_t sys_readlink(uint64_t path, uint64_t buf, uint64_t bufsiz) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (readlink)\n");
  DEBUG("%s\n%p\n%ld\n", path, buf, bufsiz);
  const char* name = "/bt.S";
  strcpy((char*)buf, name);
  return strlen(name);
}

uint64_t sys_chmod(uint64_t filename, uint64_t mode) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (chmod)\n");
  return -1;
}

uint64_t sys_fchmod(uint64_t fd, uint64_t mode) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fchmod)\n");
  return -1;
}

uint64_t sys_chown(uint64_t filename, uint64_t uservaddr, uint64_t group) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (chown)\n");
  return -1;
}

uint64_t sys_fchown(uint64_t fd, uint64_t uservaddr, uint64_t group) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fchown)\n");
  return -1;
}

uint64_t sys_lchown(uint64_t filename, uint64_t uservaddr, uint64_t group) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (lchown)\n");
  return -1;
}

uint64_t sys_umask(uint64_t mask) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (umask)\n");
  return -1;
}

uint64_t sys_getrlimit(uint64_t resource, uint64_t rlim) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getrlimit)\n");
  return -1;
}

uint64_t sys_getrusage(uint64_t who, uint64_t ru) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getrusage)\n");
  return -1;
}

uint64_t sys_sysinfo(uint64_t info) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sysinfo)\n");
  return -1;
}

uint64_t sys_times(uint64_t tbuf) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (times)\n");
  return -1;
}

uint64_t sys_ptrace(uint64_t request, uint64_t pid, uint64_t addrlen,
                    uint64_t data) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (ptrace)\n");
  return -1;
}

uint64_t sys_getuid() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getuid)\n");
  return -1;
}

uint64_t sys_syslog(uint64_t type, uint64_t buf, uint64_t len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (syslog)\n");
  return -1;
}

uint64_t sys_getgid() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getgid)\n");
  return -1;
}

uint64_t sys_setuid(uint64_t uid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setuid)\n");
  return -1;
}

uint64_t sys_setgid(uint64_t gid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setgid)\n");
  return -1;
}

uint64_t sys_geteuid() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (geteuid)\n");
  return -1;
}

uint64_t sys_getegid() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getegid)\n");
  return -1;
}

uint64_t sys_setpgid(uint64_t pid, uint64_t pgid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setpgid)\n");
  return -1;
}

uint64_t sys_getppid() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getppid)\n");
  return -1;
}

uint64_t sys_getpgrp() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getpgrp)\n");
  return -1;
}

uint64_t sys_setsid() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setsid)\n");
  return -1;
}

uint64_t sys_setreuid(uint64_t ruid, uint64_t euid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setreuid)\n");
  return -1;
}

uint64_t sys_setregid(uint64_t rgid, uint64_t egid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setregid)\n");
  return -1;
}

uint64_t sys_getgroups(uint64_t gidsetsize, uint64_t grouplist) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getgroups)\n");
  return -1;
}

uint64_t sys_setgroups(uint64_t gidsetsize, uint64_t grouplist) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setgroups)\n");
  return -1;
}

uint64_t sys_setresuid(uint64_t ruid, uint64_t euid, uint64_t suid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setresuid)\n");
  return -1;
}

uint64_t sys_getresuid(uint64_t ruidp, uint64_t euidp, uint64_t suidp) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getresuid)\n");
  return -1;
}

uint64_t sys_setresgid(uint64_t rgid, uint64_t egid, uint64_t sgid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setresgid)\n");
  return -1;
}

uint64_t sys_getresgid(uint64_t rgidp, uint64_t egidp, uint64_t sgidp) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getresgid)\n");
  return -1;
}

uint64_t sys_getpgid(uint64_t pid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getpgid)\n");
  return -1;
}

uint64_t sys_setfsuid(uint64_t uid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setfsuid)\n");
  return -1;
}

uint64_t sys_setfsgid(uint64_t gid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setfsgid)\n");
  return -1;
}

uint64_t sys_getsid(uint64_t pid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getsid)\n");
  return -1;
}

uint64_t sys_capget(uint64_t header, uint64_t dataptr) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (capget)\n");
  return -1;
}

uint64_t sys_capset(uint64_t header, uint64_t data) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (capset)\n");
  return -1;
}

uint64_t sys_rt_sigpending(uint64_t uset, uint64_t sigsetsize) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (rt_sigpending)\n");
  return -1;
}

uint64_t sys_rt_sigtimedwait(uint64_t uthese, uint64_t uinfo, uint64_t uts,
                             uint64_t sigsetsize) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (rt_sigtimedwait)\n");
  return -1;
}

uint64_t sys_rt_sigqueueinfo(uint64_t pid, uint64_t sig, uint64_t uinfo) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (rt_sigqueueinfo)\n");
  return -1;
}

uint64_t sys_rt_sigsuspend(uint64_t unewset, uint64_t sigsetsize) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (rt_sigsuspend)\n");
  return -1;
}

uint64_t sys_sigaltstack(uint64_t uss, uint64_t uoss) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sigaltstack)\n");
  return -1;
}

uint64_t sys_utime(uint64_t filename, uint64_t times) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (utime)\n");
  return -1;
}

uint64_t sys_mknod(uint64_t filename, uint64_t mode, uint64_t dev) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mknod)\n");
  return -1;
}

uint64_t sys_uselib(uint64_t library) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (uselib)\n");
  return -1;
}

uint64_t sys_personality(uint64_t personality) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (personality)\n");
  return -1;
}

uint64_t sys_ustat(uint64_t dev, uint64_t ubuf) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (ustat)\n");
  return -1;
}

uint64_t sys_statfs(uint64_t pathname, uint64_t buf) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (statfs)\n");
  return -1;
}

uint64_t sys_fstatfs(uint64_t fd, uint64_t buf) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fstatfs)\n");
  return -1;
}

uint64_t sys_sysfs(uint64_t option, uint64_t arg1, uint64_t arg2) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sysfs)\n");
  return -1;
}

uint64_t sys_getpriority(uint64_t which, uint64_t who) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getpriority)\n");
  return -1;
}

uint64_t sys_setpriority(uint64_t which, uint64_t who, uint64_t niceval) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setpriority)\n");
  return -1;
}

uint64_t sys_sched_setparam(uint64_t pid, uint64_t param) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sched_setparam)\n");
  return -1;
}

uint64_t sys_sched_getparam(uint64_t pid, uint64_t param) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sched_getparam)\n");
  return -1;
}

uint64_t sys_sched_setscheduler(uint64_t pid, uint64_t policy, uint64_t param) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sched_setscheduler)\n");
  return -1;
}

uint64_t sys_sched_getscheduler(uint64_t pid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sched_getscheduler)\n");
  return -1;
}

uint64_t sys_sched_get_priority_max(uint64_t policy) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sched_get_priority_max)\n");
  return -1;
}

uint64_t sys_sched_get_priority_min(uint64_t policy) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sched_get_priority_min)\n");
  return -1;
}

uint64_t sys_sched_rr_get_interval(uint64_t pid, uint64_t interval) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sched_rr_get_interval)\n");
  return -1;
}

uint64_t sys_mlock(uint64_t start, uint64_t len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mlock)\n");
  return -1;
}

uint64_t sys_munlock(uint64_t start, uint64_t len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (munlock)\n");
  return -1;
}

uint64_t sys_mlockall(uint64_t start, uint64_t len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mlockall)\n");
  return -1;
}

uint64_t sys_munlockall() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (munlockall)\n");
  return -1;
}

uint64_t sys_vhangup() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (vhangup)\n");
  return -1;
}

uint64_t sys_modify_ldt(uint64_t func, uint64_t ptr, uint64_t bytecount) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (modify_ldt)\n");
  return -1;
}

uint64_t sys_pivot_root(uint64_t new_root, uint64_t put_old) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (pivot_root)\n");
  return -1;
}

uint64_t sys__sysctl(uint64_t args) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (_sysctl)\n");
  return -1;
}

uint64_t sys_prctl(uint64_t option, uint64_t arg2, uint64_t arg3, uint64_t arg4,
                   uint64_t arg5) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (prctl)\n");
  return -1;
}

uint64_t sys_adjtimex(uint64_t txc_p) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (adjtimex)\n");
  return -1;
}

uint64_t sys_setrlimit(uint64_t resource, uint64_t rlim) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setrlimit)\n");
  return -1;
}

uint64_t sys_chroot(uint64_t filename) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (chroot)\n");
  return -1;
}

uint64_t sys_sync() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sync)\n");
  return -1;
}

uint64_t sys_acct(uint64_t name) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (acct)\n");
  return -1;
}

uint64_t sys_mount(uint64_t dev_name, uint64_t dir_name, uint64_t type,
                   uint64_t flags, uint64_t data) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mount)\n");
  return -1;
}

uint64_t sys_umount2(uint64_t name, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (umount2)\n");
  return -1;
}

uint64_t sys_swapon(uint64_t specialfile, uint64_t swap_flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (swapon)\n");
  return -1;
}

uint64_t sys_swapoff(uint64_t specialfile) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (swapoff)\n");
  return -1;
}

uint64_t sys_reboot(uint64_t magic1, uint64_t magic2, uint64_t cmd,
                    uint64_t arg) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (reboot)\n");
  return -1;
}

uint64_t sys_setdomainname(uint64_t name, uint64_t len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setdomainname)\n");
  return -1;
}

uint64_t sys_iopl(uint64_t level) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (iopl)\n");
  return -1;
}

uint64_t sys_ioperm(uint64_t from, uint64_t num, uint64_t turn_on) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (ioperm)\n");
  return -1;
}

uint64_t sys_create_module() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (create_module)\n");
  return -1;
}

uint64_t sys_init_module(uint64_t umod, uint64_t len, uint64_t uargs) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (init_module)\n");
  return -1;
}

uint64_t sys_delete_module(uint64_t name_user, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (delete_module)\n");
  return -1;
}

uint64_t sys_get_kernel_syms() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (get_kernel_syms)\n");
  return -1;
}

uint64_t sys_query_module() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (query_module)\n");
  return -1;
}

uint64_t sys_quotactl(uint64_t cmd, uint64_t special, uint64_t id,
                      uint64_t addr) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (quotactl)\n");
  return -1;
}

uint64_t sys_nfsservctl() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (nfsservctl)\n");
  return -1;
}

uint64_t sys_getpmsg() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getpmsg)\n");
  return -1;
}

uint64_t sys_putpmsg() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (putpmsg)\n");
  return -1;
}

uint64_t sys_afs_syscall() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (afs_syscall)\n");
  return -1;
}

uint64_t sys_tuxcall() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (tuxcall)\n");
  return -1;
}

uint64_t sys_security() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (security)\n");
  return -1;
}

uint64_t sys_readahead(uint64_t fd, uint64_t offset, uint64_t count) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (readahead)\n");
  return -1;
}

uint64_t sys_setxattr(uint64_t pathname, uint64_t name, uint64_t value,
                      uint64_t size, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setxattr)\n");
  return -1;
}

uint64_t sys_lsetxattr(uint64_t pathname, uint64_t name, uint64_t value,
                       uint64_t size, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (lsetxattr)\n");
  return -1;
}

uint64_t sys_fsetxattr(uint64_t fd, uint64_t name, uint64_t value,
                       uint64_t size, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fsetxattr)\n");
  return -1;
}

uint64_t sys_getxattr(uint64_t pathname, uint64_t name, uint64_t value,
                      uint64_t size) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getxattr)\n");
  return -1;
}

uint64_t sys_lgetxattr(uint64_t pathname, uint64_t name, uint64_t value,
                       uint64_t size) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (lgetxattr)\n");
  return -1;
}

uint64_t sys_fgetxattr(uint64_t fd, uint64_t name, uint64_t value,
                       uint64_t size) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fgetxattr)\n");
  return -1;
}

uint64_t sys_listxattr(uint64_t pathname, uint64_t list, uint64_t size) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (listxattr)\n");
  return -1;
}

uint64_t sys_llistxattr(uint64_t pathname, uint64_t list, uint64_t size) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (llistxattr)\n");
  return -1;
}

uint64_t sys_flistxattr(uint64_t fd, uint64_t list, uint64_t size) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (flistxattr)\n");
  return -1;
}

uint64_t sys_removexattr(uint64_t pathname, uint64_t name) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (removexattr)\n");
  return -1;
}

uint64_t sys_lremovexattr(uint64_t pathname, uint64_t name) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (lremovexattr)\n");
  return -1;
}

uint64_t sys_fremovexattr(uint64_t fd, uint64_t name) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fremovexattr)\n");
  return -1;
}

uint64_t sys_tkill(uint64_t pid, uint64_t sig) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (tkill)\n");
  return -1;
}

uint64_t sys_time(uint64_t tloc) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (time)\n");
  return -1;
}

uint64_t sys_set_thread_area(uint64_t u_unfo) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (set_thread_area)\n");
  return -1;
}

uint64_t sys_io_setup(uint64_t nr_events, uint64_t ctxp) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (io_setup)\n");
  return -1;
}

uint64_t sys_io_destroy(uint64_t ctx) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (io_destroy)\n");
  return -1;
}

uint64_t sys_io_getevents(uint64_t ctx_id, uint64_t min_nr, uint64_t nr,
                          uint64_t events, uint64_t timeout) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (io_getevents)\n");
  return -1;
}

uint64_t sys_io_submit(uint64_t ctx_id, uint64_t nr, uint64_t iocbpp) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (io_submit)\n");
  return -1;
}

uint64_t sys_io_cancel(uint64_t ctx_id, uint64_t iocb, uint64_t result) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (io_cancel)\n");
  return -1;
}

uint64_t sys_get_thread_area(uint64_t u_info) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (get_thread_area)\n");
  return -1;
}

uint64_t sys_lookup_dcookie(uint64_t cookie64, uint64_t buf, uint64_t len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (lookup_dcookie)\n");
  return -1;
}

uint64_t sys_epoll_create(uint64_t size) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (epoll_create)\n");
  return -1;
}

uint64_t sys_epoll_ctl_old() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (epoll_ctl_old)\n");
  return -1;
}

uint64_t sys_epoll_wait_old() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (epoll_wait_old)\n");
  return -1;
}

uint64_t sys_remap_file_pages(uint64_t start, uint64_t size, uint64_t protocol,
                              uint64_t pgoff, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (remap_file_pages)\n");
  return -1;
}

uint64_t sys_getdents64(uint64_t fd, uint64_t dirent, uint64_t count) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (getdents64)\n");
  return -1;
}

uint64_t sys_restart_syscall() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (restart_syscall)\n");
  return -1;
}

uint64_t sys_semtimedop(uint64_t semid, uint64_t tsops, uint64_t nsops,
                        uint64_t timeout) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (semtimedop)\n");
  return -1;
}

uint64_t sys_fadvise64(uint64_t fd, uint64_t offset, uint64_t len,
                       uint64_t advice) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fadvise64)\n");
  return -1;
}

uint64_t sys_timer_create(uint64_t which_clock, uint64_t timer_event_spec,
                          uint64_t creaded_timer_id) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (timer_create)\n");
  return -1;
}

uint64_t sys_timer_settime(uint64_t timer_id, uint64_t flags,
                           uint64_t new_setting, uint64_t old_setting) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (timer_settime)\n");
  return -1;
}

uint64_t sys_timer_gettime(uint64_t timer_id, uint64_t setting) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (timer_gettime)\n");
  return -1;
}

uint64_t sys_timer_getoverrun(uint64_t timer_id) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (timer_getoverrun)\n");
  return -1;
}

uint64_t sys_timer_delete(uint64_t timer_id) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (timer_delete)\n");
  return -1;
}

uint64_t sys_clock_settime(uint64_t which_clock, uint64_t tp) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (clock_settime)\n");
  return -1;
}

uint64_t sys_clock_nanosleep(uint64_t which_clock, uint64_t flags,
                             uint64_t rqtp, uint64_t rmtp) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (clock_nanosleep)\n");
  return -1;
}

uint64_t sys_epoll_wait(uint64_t epfd, uint64_t events, uint64_t maxevents,
                        uint64_t timeout) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (epoll_wait)\n");
  return -1;
}

uint64_t sys_epoll_ctl(uint64_t epfd, uint64_t op, uint64_t fd,
                       uint64_t event) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (epoll_ctl)\n");
  return -1;
}

uint64_t sys_tgkill(uint64_t tgid, uint64_t pid, uint64_t sig) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (tgkill)\n");
  return -1;
}

uint64_t sys_utimes(uint64_t filename, uint64_t utimes) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (utimes)\n");
  return -1;
}

uint64_t sys_vserver() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (vserver)\n");
  return -1;
}

uint64_t sys_mbind(uint64_t start, uint64_t len, uint64_t mode, uint64_t nmask,
                   uint64_t maxnode, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mbind)\n");
  return -1;
}

uint64_t sys_set_mempolicy(uint64_t mode, uint64_t nmask, uint64_t maxnode) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (set_mempolicy)\n");
  return -1;
}

uint64_t sys_get_mempolicy(uint64_t policy, uint64_t nmask, uint64_t maxnode,
                           uint64_t addr, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (get_mempolicy)\n");
  return -1;
}

uint64_t sys_mq_open(uint64_t u_name, uint64_t oflag, uint64_t mode,
                     uint64_t u_attr) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mq_open)\n");
  return -1;
}

uint64_t sys_mq_unlink(uint64_t u_name) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mq_unlink)\n");
  return -1;
}

uint64_t sys_mq_timedsend(uint64_t mqdes, uint64_t u_msg_ptr, uint64_t msg_len,
                          uint64_t msg_prio, uint64_t u_abs_timeout) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mq_timedsend)\n");
  return -1;
}

uint64_t sys_mq_timedreceive(uint64_t mqdes, uint64_t u_msg_ptr,
                             uint64_t msg_len, uint64_t u_msg_prio,
                             uint64_t u_abs_timeout) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mq_timedreceive)\n");
  return -1;
}

uint64_t sys_mq_notify(uint64_t mqdes, uint64_t u_notification) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mq_notify)\n");
  return -1;
}

uint64_t sys_mq_getsetattr(uint64_t mqdes, uint64_t u_mqstat,
                           uint64_t u_omqstat) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mq_getsetattr)\n");
  return -1;
}

uint64_t sys_kexec_load(uint64_t entry, uint64_t nr_segments, uint64_t segments,
                        uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (kexec_load)\n");
  return -1;
}

uint64_t sys_waitid(uint64_t which, uint64_t upid, uint64_t infop,
                    uint64_t options, uint64_t ru) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (waitid)\n");
  return -1;
}

uint64_t sys_add_key(uint64_t type, uint64_t description, uint64_t payload,
                     uint64_t plen, uint64_t ringid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (add_key)\n");
  return -1;
}

uint64_t sys_request_key(uint64_t type, uint64_t description,
                         uint64_t callout_info, uint64_t destringid) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (request_key)\n");
  return -1;
}

uint64_t sys_keyctl(uint64_t option, uint64_t arg2, uint64_t arg3,
                    uint64_t arg4, uint64_t arg5) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (keyctl)\n");
  return -1;
}

uint64_t sys_ioprio_set(uint64_t which, uint64_t who, uint64_t ioprio) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (ioprio_set)\n");
  return -1;
}

uint64_t sys_ioprio_get(uint64_t which, uint64_t who) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (ioprio_get)\n");
  return -1;
}

uint64_t sys_inotify_init() {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (inotify_init)\n");
  return -1;
}

uint64_t sys_inotify_add_watch(uint64_t fd, uint64_t pathname, uint64_t mask) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (inotify_add_watch)\n");
  return -1;
}

uint64_t sys_inotify_rm_watch(uint64_t fd, uint64_t wd) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (inotify_rm_watch)\n");
  return -1;
}

uint64_t sys_migrate_pages(uint64_t pid, uint64_t maxnode, uint64_t old_ndoes,
                           uint64_t new_nodes) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (migrate_pages)\n");
  return -1;
}

uint64_t sys_mkdirat(uint64_t dfd, uint64_t pathname, uint64_t mode) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mkdirat)\n");
  return -1;
}

uint64_t sys_mknodat(uint64_t dfd, uint64_t filename, uint64_t mode,
                     uint64_t dev) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (mknodat)\n");
  return -1;
}

uint64_t sys_fchownat(uint64_t dfd, uint64_t filename, uint64_t user_mask_ptr,
                      uint64_t group, uint64_t flag) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fchownat)\n");
  return -1;
}

uint64_t sys_futimesat(uint64_t dfd, uint64_t filename, uint64_t utimes) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (futimesat)\n");
  return -1;
}

uint64_t sys_newfstatat(uint64_t dfd, uint64_t filename, uint64_t statbuf,
                        uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (newfstatat)\n");
  return -1;
}

uint64_t sys_unlinkat(uint64_t dfd, uint64_t pathname, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (unlinkat)\n");
  return -1;
}

uint64_t sys_renameat(uint64_t olddfd, uint64_t oldname, uint64_t newdfd,
                      uint64_t newname) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (renameat)\n");
  return -1;
}

uint64_t sys_linkat(uint64_t olddfd, uint64_t oldname, uint64_t newdfd,
                    uint64_t newname, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (linkat)\n");
  return -1;
}

uint64_t sys_symlinkat(uint64_t oldname, uint64_t newdfd, uint64_t newname) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (symlinkat)\n");
  return -1;
}

uint64_t sys_readlinkat(uint64_t dfd, uint64_t pathname, uint64_t buf,
                        uint64_t bufsiz) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (readlinkat)\n");
  return -1;
}

uint64_t sys_fchmodat(uint64_t dfd, uint64_t filename, uint64_t mode) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fchmodat)\n");
  return -1;
}

uint64_t sys_faccessat(uint64_t dfd, uint64_t filename, uint64_t mode) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (faccessat)\n");
  return -1;
}

uint64_t sys_pselect6(uint64_t n, uint64_t inp, uint64_t outp, uint64_t exp,
                      uint64_t tsp, uint64_t sig) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (pselect6)\n");
  return -1;
}

uint64_t sys_ppoll(uint64_t ufds, uint64_t nfds, uint64_t tsp, uint64_t sigmask,
                   uint64_t sigsetsize) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (ppoll)\n");
  return -1;
}

uint64_t sys_unshare(uint64_t unshare_flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (unshare)\n");
  return -1;
}

uint64_t sys_get_robust_list(uint64_t head_ptr, uint64_t len_ptr) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (get_robust_list)\n");
  return -1;
}

uint64_t sys_splice(uint64_t fd_in, uint64_t off_in, uint64_t fd_out,
                    uint64_t off_out, uint64_t len, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (splice)\n");
  return -1;
}

uint64_t sys_tee(uint64_t fdin, uint64_t fdout, uint64_t len, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (tee)\n");
  return -1;
}

uint64_t sys_sync_file_range(uint64_t fd, uint64_t offset, uint64_t nbytes,
                             uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sync_file_range)\n");
  return -1;
}

uint64_t sys_vmsplice(uint64_t fd, uint64_t iov, uint64_t nr_segs,
                      uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (vmsplice)\n");
  return -1;
}

uint64_t sys_move_pages(uint64_t pid, uint64_t nr_pages, uint64_t pages,
                        uint64_t nodes, uint64_t status, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (move_pages)\n");
  return -1;
}

uint64_t sys_utimensat(uint64_t dfd, uint64_t filename, uint64_t utimes,
                       uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (utimensat)\n");
  return -1;
}

uint64_t sys_epoll_pwait(uint64_t epfd, uint64_t events, uint64_t maxevents,
                         uint64_t timeout, uint64_t sigmask,
                         uint64_t sigsetsize) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (epoll_pwait)\n");
  return -1;
}

uint64_t sys_signalfd(uint64_t ufd, uint64_t user_mask, uint64_t sizemask) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (signalfd)\n");
  return -1;
}

uint64_t sys_timerfd_create(uint64_t clockid, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (timerfd_create)\n");
  return -1;
}

uint64_t sys_eventfd(uint64_t count) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (eventfd)\n");
  return -1;
}

uint64_t sys_fallocate(uint64_t fd, uint64_t mode, uint64_t offset,
                       uint64_t len) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fallocate)\n");
  return -1;
}

uint64_t sys_timerfd_settime(uint64_t ufd, uint64_t flags, uint64_t utmr,
                             uint64_t otmr) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (timerfd_settime)\n");
  return -1;
}

uint64_t sys_timerfd_gettime(uint64_t ufd, uint64_t otmr) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (timerfd_gettime)\n");
  return -1;
}

uint64_t sys_accept4(uint64_t fd, uint64_t upeer_sockaddr,
                     uint64_t upeer_addrlen, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (accept4)\n");
  return -1;
}

uint64_t sys_signalfd4(uint64_t ufd, uint64_t user_mask, uint64_t sizemask,
                       uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (signalfd4)\n");
  return -1;
}

uint64_t sys_eventfd2(uint64_t count) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (eventfd2)\n");
  return -1;
}

uint64_t sys_epoll_create1(uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (epoll_create1)\n");
  return -1;
}

uint64_t sys_dup3(uint64_t oldfd, uint64_t newfd, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (dup3)\n");
  return -1;
}

uint64_t sys_pipe2(uint64_t fildes, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (pipe2)\n");
  return -1;
}

uint64_t sys_inotify_init1(uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (inotify_init1)\n");
  return -1;
}

uint64_t sys_preadv(uint64_t fd, uint64_t vec, uint64_t vlen, uint64_t pos_l,
                    uint64_t pos_h) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (preadv)\n");
  return -1;
}

uint64_t sys_pwritev(uint64_t fd, uint64_t vec, uint64_t vlen, uint64_t pos_l,
                     uint64_t pos_h) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (pwritev)\n");
  return -1;
}

uint64_t sys_rt_tgsigqueueinfo(uint64_t tgid, uint64_t pid, uint64_t sig,
                               uint64_t uinfo) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (rt_tgsigqueueinfo)\n");
  return -1;
}

uint64_t sys_perf_event_open(uint64_t attr_uptr, uint64_t pid, uint64_t cpu,
                             uint64_t group_fd, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (perf_event_open)\n");
  return -1;
}

uint64_t sys_recvmmsg(uint64_t fd, uint64_t mmsg, uint64_t vlen, uint64_t flags,
                      uint64_t timeout) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (recvmmsg)\n");
  return -1;
}

uint64_t sys_fanotify_init(uint64_t flags, uint64_t event_f_flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fanotify_init)\n");
  return -1;
}

uint64_t sys_fanotify_mark(uint64_t fanotify_fd, uint64_t flags, uint64_t mask,
                           uint64_t dfd, uint64_t pathname) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (fanotify_mark)\n");
  return -1;
}

uint64_t sys_name_to_handle_at(uint64_t dfd, uint64_t name, uint64_t handle,
                               uint64_t mnt_id, uint64_t flag) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (name_to_handle_at)\n");
  return -1;
}

uint64_t sys_open_by_handle_at(uint64_t mountdirfd, uint64_t handle,
                               uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (open_by_handle_at)\n");
  return -1;
}

uint64_t sys_clock_adjtime(uint64_t which_clock, uint64_t utx) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (clock_adjtime)\n");
  return -1;
}

uint64_t sys_syncfs(uint64_t fd) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (syncfs)\n");
  return -1;
}

uint64_t sys_sendmmsg(uint64_t fd, uint64_t mmsg, uint64_t vlen,
                      uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (sendmmsg)\n");
  return -1;
}

uint64_t sys_setns(uint64_t fd, uint64_t nstype) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (setns)\n");
  return -1;
}

uint64_t sys_process_vm_readv(uint64_t pid, uint64_t lvec, uint64_t liovcnt,
                              uint64_t rvec, uint64_t riovcnt, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (process_vm_readv)\n");
  return -1;
}

uint64_t sys_process_vm_writev(uint64_t pud, uint64_t lvec, uint64_t liovcnt,
                               uint64_t rvec, uint64_t riovcnt,
                               uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (process_vm_writev)\n");
  return -1;
}

uint64_t sys_kcmp(uint64_t pid1, uint64_t pid2, uint64_t type, uint64_t idx1,
                  uint64_t idx2) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (kcmp)\n");
  return -1;
}

uint64_t sys_finit_module(uint64_t fd, uint64_t uargs, uint64_t flags) {
  /// TODO: entire syscall
  DEBUG("Call to stubbed syscall (finit_module)\n");
  return -1;
}
