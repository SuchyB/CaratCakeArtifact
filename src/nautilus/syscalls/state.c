/*
 * This is intended to be the place to save any global state required for Linux
 * syscall emulation. Any thread, process, or CPU-specific state should not be
 * here.
 */

#include <nautilus/syscalls/types.h>

char syscall_hostname[NAUT_HOSTNAME_LEN];