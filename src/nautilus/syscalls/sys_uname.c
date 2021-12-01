#include <nautilus/nautilus.h>
#include <nautilus/syscalls/types.h>

#define SYSCALL_NAME "sys_uname"
#include "impl_preamble.h"

uint64_t sys_uname(uint64_t name_) {
  struct utsname* name = (struct utsname*)name_;
  memset(name, 0, sizeof(struct utsname));

  strncpy(name->sysname, uname_sysname, GLIBC_UTSNAME_LENGTH);
  strncpy(name->release, uname_release, GLIBC_UTSNAME_LENGTH);
  strncpy(name->version, uname_version, GLIBC_UTSNAME_LENGTH);
  strncpy(name->machine, uname_machine, GLIBC_UTSNAME_LENGTH);
  // strncpy(name->nodename, syscall_hostname, NAUT_HOSTNAME_LEN);

  return 0;
}
