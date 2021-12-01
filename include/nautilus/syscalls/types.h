#ifndef _SYSCALL_TYPES
#define _SYSCALL_TYPES

/*
 * C types used by syscalls
 */

#define NAUT_HOSTNAME_LEN 64

/// Note that the C runtime expects linux-like versioning and will abort if it
/// finds a value which is far from expected.
static const char uname_sysname[] = "Linux";
static const char uname_release[] = "5.9.6-arch1-1";
static const char uname_version[] =
    "#1 SMP PREEMPT Thu, 05 Nov 2020 21:00:46 +0000";
static const char uname_machine[] = "";

extern char syscall_hostname[NAUT_HOSTNAME_LEN];

/// glibc expects the struct fields to have this length,
/// even though the manpages suggest that the size can be chosen by the OS
#define GLIBC_UTSNAME_LENGTH 65

struct utsname {
  char sysname[GLIBC_UTSNAME_LENGTH];
  char nodename[GLIBC_UTSNAME_LENGTH];
  char release[GLIBC_UTSNAME_LENGTH];
  char version[GLIBC_UTSNAME_LENGTH];
  char machine[GLIBC_UTSNAME_LENGTH];
};

#endif // _SYSCALL_TYPES
