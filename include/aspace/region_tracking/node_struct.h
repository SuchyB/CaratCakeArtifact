#ifndef __NODE_STUCT_H__
#define __NODE_STUCT_H__

#define REGION_STR_LEN 128
#include <nautilus/aspace.h>
// #include <nautilus/paging.h>
struct mm_struct_vtbl;

typedef struct mem_map_struct
{
    struct mm_struct_vtbl *vptr;
    uint32_t size;
} mm_struct_t;

#define REGION_FORMAT "(VA=0x%p to PA=0x%p, len=%lx, prot=%lx)"
#define REGION(r) (r)->va_start, (r)->pa_start, (r)->len_bytes, (r)->protect.flags

/*
    list of vitual functions to be implemented. 
*/ 
typedef struct mm_struct_vtbl
{
    int (* insert) (
        mm_struct_t * self, 
        nk_aspace_region_t * region
    );

    void (* show) (mm_struct_t * self);
    
    nk_aspace_region_t * (* check_overlap) (
        mm_struct_t * self, 
        nk_aspace_region_t * region
    );
    
    int (* remove) (
        mm_struct_t * self, 
        nk_aspace_region_t * region, 
        uint8_t check_flags
    );
    
    nk_aspace_region_t* (* contains) (
        mm_struct_t * self, 
        nk_aspace_region_t * region, 
        uint8_t check_flags
    );
    
    nk_aspace_region_t * (* find_reg_at_addr) (
        mm_struct_t * self,
        addr_t address
    );
    
    nk_aspace_region_t * (* update_region) (
        mm_struct_t * self, 
        nk_aspace_region_t * cur_region, 
        nk_aspace_region_t * new_region, 
        uint8_t eq_flag
    );
    
    nk_aspace_region_t * (* next_smallest) (
        mm_struct_t * self, 
        nk_aspace_region_t * cur_region
    );


    nk_aspace_region_t * ( * prev_largest) (
        mm_struct_t * self,
        nk_aspace_region_t * cur_region
    );
    
    int (* destroy) (
        mm_struct_t * self
    );

} vtbl;

/*
insert region into the VA-PA map tracking data structure
*/
int mm_insert(mm_struct_t * self, nk_aspace_region_t * region);

/*
printout the VA-PA map tracking data structure
*/
void mm_show(mm_struct_t * self);

/*
check if region overlaps with existing region in the data structure
If overlapped, return the pointer to region in the data structure that overlaps with input region;
otherwise, return NULL;
*/
nk_aspace_region_t * mm_check_overlap(mm_struct_t * self, nk_aspace_region_t * region);

/*
remove region in the data structure that is same as input region in terms of check_flags (See region_equal for details)
if Removal is successful, return 0
otherwise return -1

*/
int mm_remove(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags);

/*
find the ptr to region in the data structure that is same as input region in terms of check_flags (See region_equal for details)
if no region matched the criterion, return NULL
*/
nk_aspace_region_t* mm_contains(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags);

/*
find the ptr to region in the data structure that contains the virtual address speicified
if no region contains such address, return NULL
*/

nk_aspace_region_t * mm_find_reg_at_addr (mm_struct_t * self, addr_t address);

/**
 *  find the smallest (in terms of VA) region that is larger than cur_region
 *  largest strict lower bound
 *  return NULL if cur_region is not contained in the data structure
 *  return @cur_region if curgion is the largest in the data structure
 *  return the strict lower bound region ow.
* */
nk_aspace_region_t * mm_get_next_smallest ( mm_struct_t * self, nk_aspace_region_t * cur_region);

/**
 *  find the laregest (in terms of VA) region that is samller than cur_region
 *  smallest strict upper bound
 * return NULL if cur_region is not contained in the data structure
 *  return @cur_region if curgion is the largest in the data structure
 *  return the strict lower bound region ow.
* */
nk_aspace_region_t * mm_get_prev_largest (mm_struct_t * self, nk_aspace_region_t * cur_region);



/*
    destroy and free the data structure
*/

int mm_destory (mm_struct_t * self);


/*
update region in the data structure that matches cur_region in terms of criterion specified by eq_flag. (See region_equal for details)
the matched region in data structure has all fields updated except those specified by eq_flag. (See region_update for details)
*/
nk_aspace_region_t * mm_update_region (
    mm_struct_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
);



/*
    Below are list of virtual implementation of the virtual function specified in vtbl;
    They are not supposed to be called. 
*/
int virtual_insert(mm_struct_t * self, nk_aspace_region_t * region);

void virtual_show(mm_struct_t * self);

nk_aspace_region_t * virtual_check_overlap(mm_struct_t * self, nk_aspace_region_t * region) ;

int virtual_remove(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags);

nk_aspace_region_t* virtual_contains(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags);

nk_aspace_region_t * virtual_find_reg_at_addr (mm_struct_t * self, addr_t address);

nk_aspace_region_t * virtual_update_region (
    mm_struct_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
);

int virtual_destory (mm_struct_t * self);


mm_struct_t * mm_struct_create();
int mm_struct_init(mm_struct_t * self);

#define VA_CHECK 1
#define PA_CHECK 2
#define LEN_CHECK 4
#define PROTECT_CHECK 8

static uint8_t all_eq_flag = VA_CHECK | PA_CHECK | LEN_CHECK | PROTECT_CHECK;
/*
    return 1 if two input regions are equal in terms of check_flags
    {VA_CHECK|PA_CHECK|LEN_CHECK|PROTECT_CHECK} controls which attribute of input regions are going to be checked
    Typically, we always have VA_CHECK and LEN_CHECK set, but you are free to choose not do it if it fits your requirement. 
*/
int region_equal(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB, uint8_t check_flags);

/*
    update every attribute that's not set up in eq_flags in dest from src.
    Eg. if  eq_flags = VA_CHECK|LEN_CHECK
    we going to have dest->pa_start = src->pa_start and dest->protect = src->protect
*/
int region_update(nk_aspace_region_t * dest, nk_aspace_region_t * src, uint8_t eq_flags);

/*
    check if regionA and regionB overlap. 
    If so return 1, ow. 0
*/
int overlap_helper(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB);

int region2str(nk_aspace_region_t * region,  char * str);
#endif