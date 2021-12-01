#include <aspace/region_tracking/mm_linked_list.h>
#include <nautilus/nautilus.h>

#ifndef NAUT_CONFIG_DEBUG_ASPACE_REGION_TRACKING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif


#define ERROR_LLIST(fmt, args...) ERROR_PRINT("aspace-llist: " fmt, ##args)
#define DEBUG_LLIST(fmt, args...) DEBUG_PRINT("aspace-llist: " fmt, ##args)
#define INFO_LLIST(fmt, args...)   INFO_PRINT("aspace-llist: " fmt, ##args)

int mm_llist_insert(mm_struct_t * self, nk_aspace_region_t * region) {
    mm_llist_t * llist = (mm_llist_t * ) self;

    mm_llist_node_t * newhead = (mm_llist_node_t *) malloc(sizeof(mm_llist_node_t));

    if (!newhead) {
        ERROR_LLIST("cannot allocate a new linked list node!\n");
        panic("cannot allocate a new linked list node!\n");
        return -1;
    }
    
    newhead->region = *region;
    newhead->next_llist_node = llist->region_head;

    llist->region_head = newhead;

    llist->super.size = llist->super.size + 1;
    return 0;
}

void mm_llist_show(mm_struct_t * self) {
    DEBUG_LLIST("Printing regions in linked list\n");

    mm_llist_t * llist = (mm_llist_t * ) self;

    mm_llist_node_t * curr_node = llist->region_head;
    while (curr_node != NULL)
    {
        DEBUG_LLIST("VA = %016lx to PA = %016lx, len = %16lx\n", 
            curr_node->region.va_start,
            curr_node->region.pa_start,
            curr_node->region.len_bytes
        );
        curr_node = curr_node->next_llist_node;
    }
}

nk_aspace_region_t * mm_llist_check_overlap(mm_struct_t * self, nk_aspace_region_t * region){
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;
        if (overlap_helper(curr_region_ptr, region)) {
            return curr_region_ptr;
        }
        curr_node = curr_node->next_llist_node;
    }
    return NULL;
}

int mm_llist_remove(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags){
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;
    mm_llist_node_t * prev_node = NULL;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;
        if (region_equal(curr_region_ptr, region, check_flags)) {
            // delete the node
            if (prev_node != NULL) {
                prev_node->next_llist_node = curr_node->next_llist_node;
            } else {
                llist->region_head = curr_node->next_llist_node;
            }
            
            free(curr_node);
            llist->super.size = llist->super.size - 1;
            return 0;
        }

        prev_node = curr_node;
        curr_node = curr_node->next_llist_node;
    }
    
    return -1;
}

nk_aspace_region_t* mm_llist_contains(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;
        if (region_equal(curr_region_ptr, region, check_flags)) {
            return curr_region_ptr;
        }
        curr_node = curr_node->next_llist_node;
    }
    return NULL;
}

nk_aspace_region_t * mm_llist_find_reg_at_addr (mm_struct_t * self, addr_t address) {
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;

    while (curr_node != NULL) {
        nk_aspace_region_t curr_reg = curr_node->region;
        if ( 
            (addr_t) curr_reg.va_start <= address && 
            address < (addr_t) curr_reg.va_start + (addr_t) curr_reg.len_bytes
        ) {
            return &curr_node->region;
        }
        curr_node = curr_node->next_llist_node;
    }
    
    return NULL;
}



nk_aspace_region_t * mm_llist_update_region (
    mm_struct_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
) {
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;

        if (region_equal(curr_region_ptr, cur_region, eq_flag)) {
            region_update(curr_region_ptr, new_region, eq_flag);
            return curr_region_ptr;
        }
        curr_node = curr_node->next_llist_node;
    }
    return NULL;
}

int mm_llist_node_destroy(mm_llist_node_t * node) {
    if (node != NULL) {
        mm_llist_node_destroy(node->next_llist_node);
        free(node);
    }
    return 0;
}

int mm_llist_destroy(mm_struct_t * self) {
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * head = llist->region_head;
    
    mm_llist_node_destroy(head);

    free(llist);
    
    DEBUG_LLIST("Done: destroy linked list!\n");
    return 0;
}

int mm_llist_init(mm_llist_t * llist) {
    mm_struct_init(& (llist->super));

    llist->super.vptr->insert = &mm_llist_insert;
    llist->super.vptr->show = &mm_llist_show;
    llist->super.vptr->check_overlap = &mm_llist_check_overlap;
    llist->super.vptr->remove = &mm_llist_remove;
    llist->super.vptr->contains = &mm_llist_contains;
    llist->super.vptr->find_reg_at_addr = &mm_llist_find_reg_at_addr;
    llist->super.vptr->update_region = &mm_llist_update_region;
    llist->super.vptr->destroy = &mm_llist_destroy;

    llist->region_head = NULL;

    return 0;
}

mm_struct_t * mm_llist_create() {
    mm_llist_t *mylist = (mm_llist_t *) malloc(sizeof(mm_llist_t));

    if (! mylist) {
        ERROR_LLIST("cannot allocate a linked list data structure to track region mapping\n");
        panic("cannot allocate a linked list data structure to track region mapping\n");
        return 0;
    }

    // mm_llist_init(mylist);
    mm_llist_init(mylist);


    return &mylist->super;
}

