#include <nautilus/region_tracking/struct_test.h>
#include <nautilus/region_tracking/mm_rb_tree.h>
#include <nautilus/region_tracking/mm_splay_tree.h>
#include <nautilus/random.h>
#include <nautilus/nautilus.h>

#ifndef NAUT_CONFIG_DEBUG_ASPACE_REGION_TRACKING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR_TEST(fmt, args...) ERROR_PRINT("aspace-paging-struct-test: " fmt, ##args)
#define DEBUG_TEST(fmt, args...) DEBUG_PRINT("aspace-paging-struct-test: " fmt, ##args)
#define INFO_TEST(fmt, args...)   INFO_PRINT("aspace-paging-struct-test: " fmt, ##args)

#define RAND_U64_LEN 8

int find_addr_test(
    mm_struct_t * struct_A,
    mm_struct_t * struct_B,
    char * struct_A_name,
    char * struct_B_name
) {
    int len;
    char reg_str_buf_A[REGION_STR_LEN], reg_str_buf_B[REGION_STR_LEN];
    uint8_t rand_buf[RAND_U64_LEN];
    uint64_t * rand_u64_ptr = (uint64_t *) rand_buf;

    for (int i = 0; i < FIND_ADDR_TIMES; i++) {
        nk_get_rand_bytes(rand_buf, RAND_U64_LEN);
        addr_t rand_addr = (addr_t) (*rand_u64_ptr);
        
        nk_aspace_region_t * struct_A_find_res = mm_find_reg_at_addr(struct_A, rand_addr);
        nk_aspace_region_t * struct_B_find_res = mm_find_reg_at_addr(struct_B, rand_addr);

        len = region2str(struct_A_find_res, reg_str_buf_A);
        if (len < 0) panic("bufferoverflow!\n");
        len = region2str(struct_B_find_res, reg_str_buf_B);
        if (len < 0) panic("bufferoverflow!\n");

        // DEBUG_TEST("For address = %lx, %s find region %s but %s contains in region %s!\n", 
        //         rand_addr, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);

        if (
            (struct_A_find_res == NULL && struct_B_find_res != NULL) ||
            (struct_A_find_res != NULL && struct_B_find_res == NULL) ||
            (
                struct_A_find_res != NULL && struct_B_find_res != NULL &&
                !region_equal(struct_A_find_res, struct_B_find_res, 0xff)
            )
        ) {
            ERROR_TEST("For address = %lx, %s find region %s but %s contains in region %s!\n", 
                rand_addr, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);
            panic("For address = %lx, %s find region %s but %s contains in region %s!\n", 
                rand_addr, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);
        }
    }

    DEBUG_TEST("Test %s and %s: find address test with %d finds passed!\n", 
        struct_A_name, struct_B_name, FIND_ADDR_TIMES);
    return 0;
}

int remove_test(
    mm_struct_t * struct_A,
    mm_struct_t * struct_B,
    char * struct_A_name,
    char * struct_B_name,
    nk_aspace_region_t * reg_seq
) {
    // DEBUG_TEST("struct_A.size = %d\n", struct_A->size);
    // DEBUG_TEST("struct_B.size = %d\n", struct_B->size);

    int len;
    char reg_str_buf_target[REGION_STR_LEN];
    uint8_t rand_buf[RAND_U64_LEN];
    uint64_t * rand_u64_ptr = (uint64_t *) rand_buf;

    nk_get_rand_bytes(rand_buf, RAND_U64_LEN);
    nk_rand_seed(*rand_u64_ptr);

    for (int i = 0; i < REMOVAL_TIMES; i++) {
        nk_get_rand_bytes(rand_buf, RAND_U64_LEN);
        nk_aspace_region_t reg = reg_seq[(*rand_u64_ptr) % INSERT_TIMES];

        len = region2str(&reg, reg_str_buf_target);
        if (len < 0) panic("bufferoverflow!\n");

        int struct_A_remove_res = mm_remove(struct_A, &reg, 0xff);
        int struct_B_remove_res = mm_remove(struct_B, &reg, 0xff);

        // DEBUG_TEST("When remove region #0x%lx %s, struct_A_remove_res = %d and struct_B_remove_res=%d\n", 
        //         (*rand_u64_ptr) % INSERT_TIMES, 
        //         reg_str_buf_target, 
        //         struct_A_remove_res, 
        //         struct_B_remove_res
        // );

        if (struct_A_remove_res != struct_B_remove_res) {
            ERROR_TEST("For region=%s, removal result different for data structure %s and %s",
                reg_str_buf_target, struct_A_name, struct_B_name
            );
            panic("result different for removal of %s for data structure %s and %s",
                reg_str_buf_target, struct_A_name, struct_B_name
            );
        }

    }

    DEBUG_TEST("Test %s and %s: Removal test with %d removal passed!\n", 
        struct_A_name, struct_B_name, REMOVAL_TIMES);
    return 0;
}

int contains_test(
    mm_struct_t * struct_A,
    mm_struct_t * struct_B,
    char * struct_A_name,
    char * struct_B_name,
    nk_aspace_region_t * reg_seq
) {
    int len;

    char reg_str_buf_A[REGION_STR_LEN], reg_str_buf_B[REGION_STR_LEN];
    char reg_str_buf_target[REGION_STR_LEN];
    uint8_t rand_buf[RAND_U64_LEN];
    uint64_t * rand_u64_ptr = (uint64_t *) rand_buf;

    nk_get_rand_bytes(rand_buf, RAND_U64_LEN);
    nk_rand_seed(*rand_u64_ptr);

    for (int i = 0; i < CONTAINS_TIMES; i++) {
        nk_get_rand_bytes(rand_buf, RAND_U64_LEN);
        nk_aspace_region_t reg = reg_seq[(*rand_u64_ptr) % INSERT_TIMES];
        
        len = region2str(&reg, reg_str_buf_target);
        if (len < 0) panic("bufferoverflow!\n");

        nk_aspace_region_t * struct_A_contains_res = mm_contains(struct_A, &reg, 0xff);
        nk_aspace_region_t * struct_B_contains_res = mm_contains(struct_B, &reg, 0xff);

        len = region2str(struct_A_contains_res, reg_str_buf_A);
        if (len < 0) panic("bufferoverflow!\n");
        len = region2str(struct_B_contains_res, reg_str_buf_B);
        if (len < 0) panic("bufferoverflow!\n");

        // DEBUG_TEST("For region #0x%lx %s, %s find region %s but %s find region %s!\n", 
        //         (*rand_u64_ptr) % INSERT_TIMES, reg_str_buf_target, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);

        if (
            (struct_A_contains_res != NULL && !region_equal(struct_A_contains_res, &reg, 0xff)) ||
            (struct_B_contains_res != NULL && !region_equal(struct_B_contains_res, &reg, 0xff)) ||
            (struct_A_contains_res == NULL && struct_B_contains_res != NULL) ||
            (struct_A_contains_res != NULL && struct_B_contains_res == NULL) ||
            (
                struct_A_contains_res != NULL && struct_B_contains_res != NULL &&
                !region_equal(struct_A_contains_res, struct_A_contains_res, 0xff)
            )
        ) {
            ERROR_TEST("For region=%s, %s find region %s but %s find region %s!\n", 
                reg_str_buf_target, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);
            panic("For region=%s, %s find region %s but %s find region %s!\n", 
                reg_str_buf_target, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);
        }

        
    }

    DEBUG_TEST("Test %s and %s: Contains test with %d attempts passed!\n", 
        struct_A_name, struct_B_name, CONTAINS_TIMES);

    return 0;
}

int update_test(
    mm_struct_t * struct_A,
    mm_struct_t * struct_B,
    char * struct_A_name,
    char * struct_B_name,
    nk_aspace_region_t * reg_seq
) {
    int len;
    uint8_t rand_buf[RAND_U64_LEN];
    uint64_t * rand_u64_ptr = (uint64_t *) rand_buf;
    nk_aspace_region_t cur_reg, new_reg;

    char reg_str_buf_A[REGION_STR_LEN], reg_str_buf_B[REGION_STR_LEN];
    char cur_reg_buf[REGION_STR_LEN], new_reg_buf[REGION_STR_LEN];

    for (int i = 0; i < UPDATE_TIMES; i++) {
        nk_get_rand_bytes(rand_buf, RAND_U64_LEN);
        nk_aspace_region_t cur_reg = reg_seq[(*rand_u64_ptr) % INSERT_TIMES];
        new_reg = cur_reg;
        nk_get_rand_bytes((uint8_t *) &new_reg.pa_start, RAND_U64_LEN);
        nk_get_rand_bytes((uint8_t *) &new_reg.protect.flags, RAND_U64_LEN);

        len = region2str(&cur_reg, cur_reg_buf);
        if (len < 0) panic("bufferoverflow!\n");
        len = region2str(&new_reg, new_reg_buf);
        if (len < 0) panic("bufferoverflow!\n");

        nk_aspace_region_t * struct_A_update_res = mm_update_region(struct_A, &cur_reg, &new_reg, VA_CHECK | LEN_CHECK);
        nk_aspace_region_t * struct_B_update_res = mm_update_region(struct_B, &cur_reg, &new_reg, VA_CHECK | LEN_CHECK);

        len = region2str(struct_A_update_res, reg_str_buf_A);
        if (len < 0) panic("bufferoverflow!\n");
        len = region2str(struct_B_update_res, reg_str_buf_B);
        if (len < 0) panic("bufferoverflow!\n");


        if (
            (struct_A_update_res != NULL && !region_equal(struct_A_update_res, &new_reg, 0xff)) ||
            (struct_B_update_res != NULL && !region_equal(struct_B_update_res, &new_reg, 0xff)) ||
            (struct_A_update_res == NULL && struct_B_update_res != NULL) ||
            (struct_A_update_res != NULL && struct_B_update_res == NULL) ||
            (
                struct_A_update_res != NULL && struct_B_update_res != NULL &&
                !region_equal(struct_A_update_res, struct_B_update_res, 0xff)
            )
        ) {
            ERROR_TEST("For cur_region=%s and new_region=%s, %s find region %s but %s find region %s!\n", 
                cur_reg_buf, new_reg_buf, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);
            panic("For cur_region=%s and new_region=%s, %s find region %s but %s find region %s!\n", 
                cur_reg_buf, new_reg_buf, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);
        }
    }

    DEBUG_TEST("Test %s and %s: update test with %d attempts passed!\n", 
        struct_A_name, struct_B_name, UPDATE_TIMES);

    return 0;
}


nk_aspace_region_t * insert_test(
    mm_struct_t * struct_A,
    mm_struct_t * struct_B,
    char * struct_A_name,
    char * struct_B_name
) {
    char reg_str_buf_A[REGION_STR_LEN];
    char reg_str_buf_B[REGION_STR_LEN];
    char reg_str_buf_reg[REGION_STR_LEN];

    nk_rand_seed(0);
    int len;
    uint8_t rand_buf[RAND_U64_LEN];
    uint64_t * rand_u64_ptr = (uint64_t *)rand_buf;

    nk_aspace_region_t * reg = (nk_aspace_region_t *) malloc(sizeof(nk_aspace_region_t) * INSERT_TIMES);
    if (!reg) {
        panic("cannot allocate 0x%lx regions which has total size of 0x%lx\n",
            INSERT_TIMES, sizeof(nk_aspace_region_t) * INSERT_TIMES
        );
    }
    DEBUG_TEST("Done allocating 0x%lx regions which has total size of 0x%lx\n",
        INSERT_TIMES, sizeof(nk_aspace_region_t) * INSERT_TIMES
    );

    for (int i = 0; i < INSERT_TIMES; i++) {
        reg[i].protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE |  NK_ASPACE_EXEC |NK_ASPACE_PIN | NK_ASPACE_KERN | NK_ASPACE_SWAP | NK_ASPACE_EAGER;
        
        nk_get_rand_bytes(rand_buf, RAND_U64_LEN);
        reg[i].va_start = (void *) ((*rand_u64_ptr) & 0xffffffffffff0000);
        reg[i].pa_start = reg[i].va_start;
        nk_get_rand_bytes(rand_buf, RAND_U64_LEN);
        reg[i].len_bytes = (*rand_u64_ptr) & 0x0000ffffffff0000;
        
        len = region2str(&reg[i], reg_str_buf_reg);
        if (len < 0) panic("bufferoverflow!\n");
        // DEBUG_TEST("#0x%lx Region to insert %s\n", i ,reg_str_buf_reg);


        nk_aspace_region_t * struct_A_overlap_res = mm_check_overlap(struct_A, &reg[i]);
        nk_aspace_region_t * struct_B_overlap_res = mm_check_overlap(struct_B, &reg[i]);


        len = region2str(struct_A_overlap_res, reg_str_buf_A);
        if (len < 0) panic("bufferoverflow!\n");
        len = region2str(struct_B_overlap_res, reg_str_buf_B);
        if (len < 0) panic("bufferoverflow!\n");
        // DEBUG_TEST("%s overlap res = %s\n", struct_A_name, reg_str_buf_A);
        // DEBUG_TEST("%s overlap res = %s\n", struct_B_name, reg_str_buf_B);
        
        
        if ((struct_A_overlap_res == NULL) != (struct_B_overlap_res == NULL)) {
            ERROR_TEST("For region=%s, %s find region %s but %s find region %s!\n", 
                reg_str_buf_reg, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);
            panic("For region=%s, %s find region %s but %s find region %s!\n", 
                reg_str_buf_reg, struct_A_name, reg_str_buf_A, struct_B_name, reg_str_buf_B);
        }

        if (struct_A_overlap_res != NULL && struct_B_overlap_res != NULL ) {
            if (!region_equal(struct_A_overlap_res, struct_B_overlap_res, 0xff)) {
                nk_aspace_region_t * strcut_A_contains = mm_contains(struct_A, struct_B_overlap_res, 0xff);
                nk_aspace_region_t * strcut_B_contains = mm_contains(struct_B, struct_A_overlap_res, 0xff);

                if (strcut_A_contains == NULL) {
                    ERROR_TEST("both %s and %s overlap but region content differ!\n", struct_A_name, struct_B_name);
                    ERROR_TEST("%s does not contain region %s which it should have!\n", struct_A_name, reg_str_buf_B);
                    panic("%s does not contain region %s which it should have!\n", struct_A_name, reg_str_buf_B);
                }

                if (strcut_B_contains == NULL) {
                    ERROR_TEST("both %s and %s overlap but region content differ!\n", struct_A_name, struct_B_name);
                    ERROR_TEST("%s does not contain region %s which it should have!\n", struct_B_name, reg_str_buf_A);
                    panic("%s does not contain region %s which it should have!\n", struct_B_name, reg_str_buf_A);
                }
            }
        }

        // test insert
        int struct_A_insert_res = -1, struct_B_insert_res = -1; 
        if(struct_A_overlap_res == NULL) struct_A_insert_res = mm_insert(struct_A, &reg[i]);
        if(struct_B_overlap_res == NULL) struct_B_insert_res = mm_insert(struct_B, &reg[i]);



        if (struct_A_insert_res != struct_B_insert_res) {
            ERROR_TEST("insert result differ: %s insert res =%d but %s insert res = %d!\n", 
                struct_A_name, struct_A_insert_res,
                struct_B_name, struct_B_insert_res
            );
            panic("insert result differ: %s insert res =%d but %s insert res = %d!\n", 
                struct_A_name, struct_A_insert_res,
                struct_B_name, struct_B_insert_res
            );
        }
    }

    DEBUG_TEST("Test %s and %s: insert + overlap test with %d insertions passed!\n", 
        struct_A_name, struct_B_name, INSERT_TIMES);

    return reg;
}

void struct_test_base(
    mm_struct_t *  (* struct_A_create)(),
    mm_struct_t *  (* struct_B_create)(),
    char * struct_A_name,
    char * struct_B_name
) {
    mm_struct_t * struct_A =  (*struct_A_create)();
    mm_struct_t * struct_B =  (*struct_B_create)();
    nk_rand_seed(0xdeedbeef);
    int i = 0;
    while (i < REPEAT_TIMES)
    {
        nk_aspace_region_t * reg = insert_test(
            struct_A,
            struct_B,
            struct_A_name,
            struct_B_name
        );

        find_addr_test(
            struct_A, 
            struct_B,
            struct_A_name, 
            struct_B_name
        );
        
        contains_test(
            struct_A,
            struct_B,
            struct_A_name,
            struct_B_name,
            reg
        );

        remove_test(
            struct_A,
            struct_B,
            struct_A_name,
            struct_B_name,
            reg
        );

        find_addr_test(
            struct_A, 
            struct_B,
            struct_A_name, 
            struct_B_name
        );

        contains_test(
            struct_A,
            struct_B,
            struct_A_name,
            struct_B_name,
            reg
        );

        update_test(
            struct_A,
            struct_B,
            struct_A_name,
            struct_B_name,
            reg
        );

        contains_test(
            struct_A,
            struct_B,
            struct_A_name,
            struct_B_name,
            reg
        );
        free(reg);
        i++;
    }

    mm_destory(struct_A);
    mm_destory(struct_B);
}


int rbtree_llist_test() {
    char struct_A_name[] = "rbtree";
    char struct_B_name[] = "llist";
    struct_test_base(
        &mm_rb_tree_create,
        &mm_llist_create,
        struct_A_name,
        struct_B_name
    );
    return 0;
}

int splay_llist_test() {
    char struct_A_name[] = "splay_tree";
    char struct_B_name[] = "llist";
    struct_test_base(
        &mm_splay_tree_create,
        &mm_llist_create,
        struct_A_name,
        struct_B_name
    );
    return 0;
}

int rbtree_splay_test() {
    char struct_A_name[] = "rbtree";
    char struct_B_name[] = "splay_tree";
    struct_test_base(
        &mm_rb_tree_create,
        &mm_splay_tree_create,
        struct_A_name,
        struct_B_name
    );
    return 0;
}
