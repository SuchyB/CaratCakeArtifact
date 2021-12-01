/*
 * HACK --- Duplicate aspace.h to skirt type conflicts
 * between libc and naut_types.h
 */ 


#include <stdint.h>

#define NK_ASPACE_NAME_LEN 32

typedef struct nk_aspace {
  uint64_t flags;
#define NK_ASPACE_HOOK_PF 1  // invoke me on a page fault
#define NK_ASPACE_HOOK_GPF 2 // invoke me on a general protection fault

  char name[NK_ASPACE_NAME_LEN];

  void* state;

} nk_aspace_t;

typedef struct nk_aspace_protections {
    uint64_t        flags;
#define NK_ASPACE_READ   1
#define NK_ASPACE_WRITE  2
#define NK_ASPACE_EXEC   4
#define NK_ASPACE_PIN    8
#define NK_ASPACE_KERN   16   // meaning "kernel only", which is not yet supported
#define NK_ASPACE_SWAP   32   // meaning "is swaped", which is not yet supported
#define NK_ASPACE_EAGER  64   // meaning the mapping must be immediately constructed
#define NK_ASPACE_GET_READ(flags) ((flags & NK_ASPACE_READ) >> 0)
#define NK_ASPACE_GET_WRITE(flags) ((flags & NK_ASPACE_WRITE) >> 1)
#define NK_ASPACE_GET_EXEC(flags) ((flags & NK_ASPACE_EXEC) >> 2)
#define NK_ASPACE_GET_PIN(flags) ((flags & NK_ASPACE_PIN) >> 3)
#define NK_ASPACE_GET_KERN(flags) ((flags & NK_ASPACE_KERN) >> 4)
#define NK_ASPACE_GET_SWAP(flags) ((flags & NK_ASPACE_SWAP) >> 5)
#define NK_ASPACE_GET_EAGER(flags) ((flags & NK_ASPACE_EAGER) >> 6)
} nk_aspace_protection_t;

typedef struct nk_aspace_region {
    void       *va_start;
    void       *pa_start;
    uint64_t    len_bytes;
    nk_aspace_protection_t  protect;
    int         requested_permissions; // 0 == no permissions requested, 1 == read, 2 == write, 3 == read and write
} nk_aspace_region_t;

typedef struct nk_aspace_carat {
    // pointer to the abstract aspace that the
    // rest of the kernel uses when dealing with this
    // address space
    nk_aspace_t *aspace;

    /*
     * CARAT state 
     */
    void *context;

    // perhaps you will want to do concurrency control?
    uint64_t  lock;


    /*
     * Data structure containing regions
     */
    void * mm;


    /*
     * Quick references to initial stack and blob
     */ 
    nk_aspace_region_t *initial_stack ;
    nk_aspace_region_t *initial_blob ;
} nk_aspace_carat_t;
