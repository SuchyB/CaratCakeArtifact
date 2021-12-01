#ifndef __ALLOC_PCID_QUEUE_H__
#define __ALLOC_PCID_QUEUE_H__

#include <nautilus/nautilus.h>
#include "paging_helpers.h"
// 12 bits for PCID in cr3
#define MAX_PCID 4096

struct pcid_allocator
{
    int (*alloc_pcid) (ph_cr3_pcide_t * cr3);
    int (*free_pcid) (ph_cr3_pcide_t * cr3);
};

int alloc_pcid (ph_cr3_pcide_t * cr3);

int free_pcid (ph_cr3_pcide_t * cr3);

#endif
