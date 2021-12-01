#include <nautilus/nautilus.h>
#include <nautilus/spinlock.h>
#include <nautilus/paging.h>
#include <nautilus/thread.h>
#include <nautilus/shell.h>
#include <nautilus/vc.h>
#include <nautilus/alloc.h>
#include "allocator_test_trace.h"

#define TRACE_NUM 0

#define ERROR(fmt, args...) ERROR_PRINT("alloc_test_trace: " fmt "\n", ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("alloc_test_trace: " fmt "\n", ##args)
#define INFO(fmt, args...)   INFO_PRINT("alloc_test_trace: " fmt "\n", ##args)
#define USER(fmt, args...)  nk_vc_printf(fmt "\n", ##args)



static int run_alloc_test_trace(int trace_num)
{
    int num_allocs = allocator_tests_traces[trace_num][0];
    int num_ops = allocator_tests_traces[trace_num][1];
    void* allocs[num_allocs];
    int allocs_completed = 0;
    for(int i=2; i<(num_ops*3)+2;i+=3){
        int alloc_id = allocator_tests_traces[trace_num][i+1];
        //DEBUG("alloc_type: %d", allocator_tests_traces[trace_num][i]);
        switch (allocator_tests_traces[trace_num][i])
        {
        case -1: //alloc case
            allocs[alloc_id] = malloc(allocator_tests_traces[trace_num][i+2]);
            DEBUG("ALLOCed id %d with size %d and location %p", alloc_id, allocator_tests_traces[trace_num][i+2], allocs[alloc_id]);
            //allocs_completed++;
            if(!allocs[alloc_id]){
                DEBUG("Alloc failed - malloc returned null ptr");
                return -1;
            }
            break;
        case -2: //realloc
            allocs[alloc_id] = realloc(allocs[alloc_id], allocator_tests_traces[trace_num][i+2]);
            DEBUG("REALLOCed id %d with size %d and location %p", alloc_id, allocator_tests_traces[trace_num][i+2], allocs[alloc_id]);
            if(!allocs[alloc_id]){
                DEBUG("Realloc failed - realloc returned null ptr");
                return -1;
            }
            break;
        case -3: //free
            free(allocs[alloc_id]);
            DEBUG("Freed id %d at location %p", alloc_id, allocs[alloc_id]);
            break;
        
        default:
            DEBUG("How the hell did we get here?");
            return -1;
            break;
        }
        
        allocs_completed++;
    }




    USER("TEST COMPLETED for trace id %d", trace_num);
    USER("EXPECTED: %d operations", num_ops);
    USER("COMPLETED: %d operations", allocs_completed);

    return 0;
}


static int handle_alloc_tests_trace(char* buf, void* priv){
  char alloc_type[32];
  int trace_num;
  int num_times;

  if (sscanf(buf,"allocator_test_trace %s %d %d", alloc_type, &trace_num, &num_times)!=3) {
    USER("Unknown arguments");
    return -1;
  }

  nk_aspace_characteristics_t chars;

  nk_aspace_query("carat",&chars);
  
  nk_aspace_t *as = nk_aspace_create("carat","myspace",&chars);

  USER("Have aspace %p",as);

  /*
  nk_aspace_region_t reg = { .va_start = 0,
                             .pa_start = 0,
                             .len_bytes = 1<<30,
                             .protect.flags = 3,
                             .requested_permissions = 0 };

  nk_aspace_add_region(as, &reg);
  */
  
  nk_aspace_move_thread(as);

  USER("Survived move into aspace\n");
  
  nk_alloc_t *myalloc = nk_alloc_create(alloc_type,"myalloc");

  USER("Have allocator %p",myalloc);
  
  if(!myalloc){
    USER("Error - could not create allocator");
    return -1;
  }
  if(nk_alloc_set_associated(myalloc)){
    USER("Error - could not associate allocator");
    return -1;
  }

  USER("Commencing test");

  while (num_times>0) {
    if (run_alloc_test_trace(trace_num)) {
      USER("allocator test failed");
      return -1;
    }
    num_times--;
  }

  USER("Done test");

  return 0;
}

static struct shell_cmd_impl alloctesttrace_impl = {
    .cmd = "allocator_test_trace",
    .help_str = "allocator_test_trace alloc_type trace_num num_times - runs an alloc trace with cs213 alloc",
    .handler = handle_alloc_tests_trace,
};
nk_register_shell_cmd(alloctesttrace_impl);
