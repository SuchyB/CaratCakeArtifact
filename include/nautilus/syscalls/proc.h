/*
 * Interface between process/thread and syscall interface
 */

#ifndef _SYSCALL_PROC
#define _SYSCALL_PROC

#include <nautilus/list.h>
#include <nautilus/spinlock.h>

// Forward declarations
typedef struct nk_process nk_process_t;
typedef struct nk_aspace_region nk_aspace_region_t;

// On Linux, this is always(?) an int
typedef int fd_t;

#define SYSCALL_INVALID_FD (fd_t)(-1)

// Number of static FDs supported. Should be replaced with another data structure as needed.
#define SYSCALL_NUM_FDS 16

struct file_descriptor {
  struct nk_fs_open_file_state* nk_fs_ptr;
  fd_t fd_no;
};

// Public-ish interface for file descriptors, add optimizations here
struct file_descriptor_table {
  struct file_descriptor fds[SYSCALL_NUM_FDS];
  size_t next_fd_index;
};


// Represents a region of memory as understood by the user process
struct linux_mmap_region {
  struct linux_mmap_region* next;
  void* start;
  void* end;
};

// Represents a specific call to Nautilus's malloc
struct linux_mmap_allocation {
  struct linux_mmap_allocation* next;
  struct linux_mmap_region* region_list;
  void* start;
  void* end;
};

struct nk_thread_linux_syscall_state {
  
};

struct nk_process_linux_syscall_state {
  spinlock_t lock;
  struct file_descriptor_table fd_table;
  struct linux_mmap_allocation* mmap_allocation_list;
};

/// Frees any malloc'd syscall state
void free_process_syscall_state(struct nk_process_linux_syscall_state* syscall_state);

/// @param table The table to lookup in
/// @param fd The file descriptor number to look for
/// @return The Nautilus file state if it exists; else NULL
struct nk_fs_open_file_state* fd_to_nk_fs(struct file_descriptor_table* table, fd_t fd);

/// Inserts a Nautilus open file into the fd table, returning the lowest
/// unutilized fd number, or -1 if something goes wrong
fd_t fd_add(struct file_descriptor_table* table, struct nk_fs_open_file_state* nk_fd);

/// Adds a region to the mmap tracking system.
/// @param proc Nautilus process
/// @param aspace_region Nautilus aspace region structure
void proc_mmap_add_region(nk_process_t* proc, nk_aspace_region_t* aspace_region);

/// Removes a region from the mmap tracking system.
/// @param proc Nautilus process
/// @param start_addr Beginning of the region to remove
/// @param len Length of the region to remove in bytes
void proc_mmap_remove_region(nk_process_t* proc, void* start_addr, uint64_t len);


#endif // _SYSCALL_PROC
