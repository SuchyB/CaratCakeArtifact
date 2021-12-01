#include "assert.h"
#include <aspace/region_tracking/node_struct.h>
#include <nautilus/nautilus.h>

int virtual_insert(mm_struct_t * self, nk_aspace_region_t * region) {
    // should never be called as a virtual function 
    panic("should not call virtual_insert\n");;
    return 0;
}

void virtual_show(mm_struct_t * self) {
    // should never be called as a virtual function 
    panic("should not call virtual_show\n");;
}

nk_aspace_region_t * virtual_check_overlap(mm_struct_t * self, nk_aspace_region_t * region) {
    // should never be called as a virtual function 
    panic("should not call virtual_check_overlap\n");;
    return 0;
}

int virtual_remove(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    // should never be called as a virtual function 
    panic("should not call virtual_remove\n");;
    return 0;
}

nk_aspace_region_t* virtual_contains(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    // should never be called as a virtual function 
    panic("should not call virtual_contains\n");;
    return 0;
}

nk_aspace_region_t * virtual_find_reg_at_addr (mm_struct_t * self, addr_t address) {
    // should never be called as a virtual function 
    panic("should not call virtual_find_reg_at_addr\n");;
    return 0;
}

nk_aspace_region_t * virtual_update_region (
    mm_struct_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
) {
    // should never be called as a virtual function 
    panic("should not call virtual_update_region\n");;
    return 0;
}

nk_aspace_region_t * virtual_next_smallest ( mm_struct_t * self, nk_aspace_region_t * cur_region) {
    // should never be called as a virtual function 
    panic("should not call virtual_update_region\n");;
    return 0;
}

nk_aspace_region_t * virtual_prev_largest (mm_struct_t * self, nk_aspace_region_t * cur_region){
    // should never be called as a virtual function 
    panic("should not call virtual_update_region\n");;
    return 0;
}

int virtual_destory (mm_struct_t * self) {
    panic("should not call virtual destroy\n");
    return 0;
}


int mm_struct_init(mm_struct_t * self) {
    vtbl *vptr = (vtbl *) malloc (sizeof(vtbl));
    
    if (! vptr) {
        ERROR_PRINT("cannot allocate a virtual function table to track region mapping\n");
        return 0;
    }

    vptr->insert = &virtual_insert;
    vptr->show = &virtual_show;
    vptr->check_overlap = &virtual_check_overlap;
    vptr->remove = &virtual_remove;
    vptr->contains = &virtual_contains;
    vptr->find_reg_at_addr = &virtual_find_reg_at_addr;
    vptr->update_region = &virtual_update_region;
    vptr->next_smallest = &virtual_next_smallest;
    vptr->prev_largest = &virtual_prev_largest;
    vptr->destroy = &virtual_destory;

    self->vptr = vptr;
    self->size = 0;

    return 0;
}

mm_struct_t * mm_struct_create() {

    // should NOT be called
    mm_struct_t *my_struct = (mm_struct_t *) malloc(sizeof(mm_struct_t));

    if (! my_struct) {
        ERROR_PRINT("cannot allocate a abstract structure to track region mapping\n");
        return 0;
    }

    mm_struct_init(my_struct);

    return my_struct;
}

int mm_insert(mm_struct_t * self, nk_aspace_region_t * region) {
    return (* self->vptr->insert) (self, region);
}

void mm_show(mm_struct_t * self) {
    (*self->vptr->show) (self);
}

nk_aspace_region_t * mm_check_overlap(mm_struct_t * self, nk_aspace_region_t * region) {
    return (* self->vptr->check_overlap) (self, region);
}

int mm_remove(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    return (* self->vptr->remove) (self, region, check_flags);
}

nk_aspace_region_t* mm_contains(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    return (* self->vptr->contains) (self, region, check_flags);
}

nk_aspace_region_t * mm_find_reg_at_addr (mm_struct_t * self, addr_t address) {
    return (* self->vptr->find_reg_at_addr) (self, address);
}

nk_aspace_region_t * mm_update_region (
    mm_struct_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
) {
    return (* self->vptr->update_region) (self, cur_region, new_region, eq_flag);
}


nk_aspace_region_t * mm_get_next_smallest ( mm_struct_t * self, nk_aspace_region_t * cur_region) {
    return (* self->vptr->next_smallest) (self, cur_region);
}

nk_aspace_region_t * mm_get_prev_largest (mm_struct_t * self, nk_aspace_region_t * cur_region){
    return (* self->vptr->prev_largest) (self, cur_region);
}

int mm_destory (mm_struct_t * self) {
    return (* self->vptr->destroy) (self);
}

int region_equal(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB, uint8_t check_flags) {
    if (check_flags & VA_CHECK) {
        if (regionA->va_start != regionB->va_start) return 0;
    }

    if (check_flags & PA_CHECK) {
        if (regionA->pa_start != regionB->pa_start) return 0;
    }

    if (check_flags & LEN_CHECK) {
        if (regionA->len_bytes != regionB->len_bytes) return 0;
    }

    if (check_flags & PROTECT_CHECK) {
        if (regionA->protect.flags != regionB->protect.flags) return 0;
    }

    return 1;
}

int region_update(nk_aspace_region_t * dest, nk_aspace_region_t * src, uint8_t eq_flags) {
    if (!(eq_flags & VA_CHECK)) {
        dest->va_start = src->va_start;
    }

    if (!(eq_flags & PA_CHECK)) {
        dest->pa_start = src->pa_start;
    }

    if (!(eq_flags & LEN_CHECK)) {
        dest->len_bytes = src->len_bytes;
    }

    if (!(eq_flags & PROTECT_CHECK)) {
        dest->protect = src->protect;
    }

    return 0;
} 

int overlap_helper(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB){
    void * VA_start_A = regionA->va_start;
    void * VA_start_B = regionB->va_start;
    void * VA_end_A = regionA->va_start + regionA->len_bytes;
    void * VA_end_B = regionB->va_start + regionB->len_bytes;

    if (VA_start_A <= VA_start_B && VA_start_B < VA_end_A) {
        return 1;
    }
    if (VA_start_B <= VA_start_A && VA_start_A < VA_end_B) {
        return 1;
    }

    return 0;
}

int region2str(nk_aspace_region_t * region,  char * str) {
    if (region == NULL){
        sprintf(str, "NULL");
    } else {
        sprintf(str, "(VA=0x%p to PA=0x%p, len=%lx, prot=%lx)", 
            region->va_start,
            region->pa_start,
            region->len_bytes,
            region->protect.flags
        );
    }

    int len = strlen(str);

    if (len > REGION_STR_LEN) {
        panic("running out of allocated space when printing node!\n");
        return -1;
    }
    return strlen(str);
}
