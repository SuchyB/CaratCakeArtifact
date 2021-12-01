#include <nautilus/nautilus.h>
#include <nautilus/syscalls/types.h>

#define SYSCALL_NAME "sys_sethostname"
#include "impl_preamble.h"

uint64_t sys_sethostname(uint64_t name, uint64_t len) {
  int max_len = NAUT_HOSTNAME_LEN > len ? len : NAUT_HOSTNAME_LEN;
  DEBUG("Setting hostname (not thread safe)\n");
  strncpy(syscall_hostname, (char*)name, NAUT_HOSTNAME_LEN);
  return 0;
}