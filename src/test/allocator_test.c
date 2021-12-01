#include <nautilus/nautilus.h>
#include <nautilus/spinlock.h>
#include <nautilus/paging.h>
#include <nautilus/thread.h>
#include <nautilus/shell.h>
#include <nautilus/vc.h>
#include <nautilus/alloc.h>

#define DEBUG(S, ...) nk_vc_printf("alloc_test: debug(): " S "\n", ##__VA_ARGS__)

#define NUMPOINTERS 50

int run_alloc_test();
static int handle_alloc_tests(char* buf, void* priv);

static int handle_alloc_tests(char* buf, void* priv){
    run_alloc_test();
    return 0;
}

int run_alloc_test(){
    nk_alloc_t *myalloc = nk_alloc_create("cs213","myalloc");
    if(!myalloc){
        DEBUG("Error - could not create allocator");
        return -1;
    }
    if(nk_alloc_set_associated(myalloc)){
        DEBUG("Error - could not associate allocator");
        return -1;
    }
    int** ptrs = (int**) malloc(sizeof(int*)*NUMPOINTERS);
    if(!ptrs){
        DEBUG("Error - could not malloc array of pointers");
        return -1;  
    }
    DEBUG("Finished setup...running allocs");
    //Alloc
    for(int i = 0; i < NUMPOINTERS; i++){
        ptrs[i] = (int*) malloc(sizeof(int));
        ptrs[i][0]= i;  
        DEBUG("Allocated pointer at %p with value %d", ptrs[i],ptrs[i][0]);
    }
    DEBUG("Running reallocs");
    //Realloc
    for (int i = 0; i < NUMPOINTERS; i++)
    {
    	    DEBUG("ptrs[i] before realloc: %p", ptrs[i]);
	    ptrs[i] = (int*) realloc(ptrs[i],sizeof(int)*8);
        //DEBUG("REallocated pointer to %p with value %d, %d, %d, %d, %d, %d, %d, %d, %d", ptrs[i],(ptrs+i)[0][0],(ptrs+i)[0][1],(ptrs+i)[0][2], (ptrs+i)[0][3], (ptrs+i)[0][4], (ptrs+i)[0][5], (ptrs+i)[0][6], (ptrs+i)[0][7], (ptrs+i)[0][8]);
     DEBUG("REallocated pointer at %p with value %d", ptrs[i],ptrs[i][0]);
    }
    DEBUG("Freeing");
    //Free
    for (int i = 0; i < NUMPOINTERS; i++)
    {
        free(ptrs[i]);
        DEBUG("Freed pointer at %p", ptrs[i]); //Should be null
    }
    nk_alloc_destroy(myalloc);
    return 0;
}

static struct shell_cmd_impl alloctest_impl = {
    .cmd = "allocator_test",
    .help_str = "Tests the CS213 allocator",
    .handler = handle_alloc_tests,
};
nk_register_shell_cmd(alloctest_impl);
