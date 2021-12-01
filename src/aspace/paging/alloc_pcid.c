#include "alloc_pcid.h"

#ifndef NAUT_CONFIG_DEBUG_ASPACE_PAGING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR_PCID(fmt, args...) ERROR_PRINT("aspace-paging-pcid: " fmt, ##args)
#define DEBUG_PCID(fmt, args...) DEBUG_PRINT("aspace-paging-pcid: " fmt, ##args)
#define INFO_PCID(fmt, args...)   INFO_PRINT("aspace-paging-pcid: " fmt, ##args)

#ifdef NAUT_CONFIG_ASPACE_PAGING_PCID_QUEUE 

typedef struct alloc
{
    uint64_t head;  // head of the queue
    uint64_t tail;  // tail of the queue
    uint16_t size;  // size of the queue < MAX_PCID, so uint16_t enough
    uint16_t *data; // data ptr 
} pcid_alloc_t;

pcid_alloc_t allocator = {
    .head = 0,
    .tail = 0,
    .size = 0,
    .data = NULL
};


int pcid_allocator_init_queue() {
    
    if (!allocator.data) {
        DEBUG_PCID("init allocator!\n");
        allocator.data = (uint16_t *) malloc(sizeof(uint16_t) * MAX_PCID);
        if (!allocator.data) {
            ERROR_PCID("Cannot allocate size = %d bytes for pcid allocator\n", sizeof(uint16_t) * MAX_PCID);
            return -1;
        }

        for (int i = 0; i < MAX_PCID; i ++) {
            allocator.data[i] = i;
        }
        allocator.size = MAX_PCID;
        allocator.tail = MAX_PCID;
    }
    return 0;
}

// allocate a new pcid, written to target_ptr
int alloc_pcid_queue (ph_cr3_pcide_t * cr3) {
    int init_res = pcid_allocator_init_queue();
    if (init_res) {
        panic("Cannot allocate size = %d bytes for pcid allocator\n", sizeof(uint16_t) * MAX_PCID);
        return -1;
    }
    
    // pop one from the queue
    if (allocator.size > 0) {
        uint16_t pcid = allocator.data[allocator.head % MAX_PCID];
        allocator.head++;
        allocator.size--;
        cr3->pcid = pcid;

        return 0;

    } else {
        ERROR_PCID("no available elements in pcid allocator!\n");
        return -1;
    }
}


// 


int free_pcid_queue (ph_cr3_pcide_t * cr3) {
    int init_res = pcid_allocator_init_queue();
    if (init_res) {
        panic("Cannot allocate size = %d bytes for pcid allocator\n", sizeof(uint16_t) * MAX_PCID);
        return -1;
    }

    if (allocator.size < MAX_PCID) {
        allocator.data[allocator.tail % MAX_PCID] = cr3->pcid;
        allocator.tail++;
        allocator.size++;

        return 0;
    } else {
        ERROR_PCID("no available space in pcid allocator!\n");
        return -1;
    }
}
#endif

#ifdef NAUT_CONFIG_ASPACE_PAGING_PCID_BITMAP

#define MAX_PCID_U64 0xffffffffffffffffUL
#define PCID_U64_LEN 64
#define U64_BITWIDTH 64
static uint64_t allocated[PCID_U64_LEN] = {0};

// 0 availabe
// 1 not availabed
int alloc_pcid_bitmap (ph_cr3_pcide_t * cr3) {
    uint16_t i, j;
    for (i = 0; i < PCID_U64_LEN; i++) {
        if (allocated[i] < MAX_PCID_U64) {
            
            uint64_t u64_mask = 0x1;
            for (j = 0; j < U64_BITWIDTH; j++) {
                if (!(allocated[i] & u64_mask)) {
                    cr3->pcid = (i * U64_BITWIDTH) + j;
                    // set the bit of cur_pcid as zero
                    allocated[i] = allocated[i] | u64_mask;
                    return 0;
                }
                u64_mask = u64_mask << 1;
            }
        } 
    }
    ERROR_PCID("No available pcid\n");
    return -1;
}

int free_pcid_bitmap (ph_cr3_pcide_t * cr3) {
    uint16_t cur_pcid = cr3->pcid;
    uint16_t data_pos = cur_pcid / U64_BITWIDTH;
    uint16_t offset = cur_pcid % U64_BITWIDTH;
    uint64_t bit_mask = ((uint64_t) 1) << offset;
    if (!(allocated[data_pos] & bit_mask)) {
        ERROR_PCID("PCID to free=%x, not allocated!\n", cur_pcid);
        return -1;
    }

    // set the bit of cur_pcid as zero
    allocated[data_pos] = allocated[data_pos] & (~bit_mask);
    return 0;
}



#endif

static struct pcid_allocator pcid_allocator_obj = {

#ifdef NAUT_CONFIG_ASPACE_PAGING_PCID_QUEUE
    .alloc_pcid = &alloc_pcid_queue,
    .free_pcid = &free_pcid_queue
#endif

#ifdef NAUT_CONFIG_ASPACE_PAGING_PCID_BITMAP
    .alloc_pcid = &alloc_pcid_bitmap,
    .free_pcid = &free_pcid_bitmap
#endif

};

int alloc_pcid (ph_cr3_pcide_t * cr3) {
    return (*pcid_allocator_obj.alloc_pcid) (cr3);
}

int free_pcid (ph_cr3_pcide_t * cr3) {
    return (*pcid_allocator_obj.free_pcid) (cr3);
}



