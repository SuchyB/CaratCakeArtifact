#include <nautilus/fs.h>
#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_mkdir"
#include "impl_preamble.h"

// #include <sys/types.h> /// This breaks build due to conflicting typedef on
// off_t
typedef int mode_t; /// TODO move this to a better spot; including sys/types.h
                    /// from linux breaks the build

uint64_t sys_mkdir(uint64_t pathname_, uint64_t mode_) {
  char* pathname = (char*)pathname_;
  mode_t mode = (mode_t)mode_;

  return nk_fs_mkdir(pathname, mode);
}
