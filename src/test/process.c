
#include <nautilus/nautilus.h>
#include <nautilus/process.h>
#include <nautilus/shell.h>

#define DO_PRINT 1

#if DO_PRINT
#define PRINT(...) nk_vc_printf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

// create and run a process
static int
handle_proctest1 (char * buf, void * priv)
{
  nk_process_t *process1;
  char *args[4] = {"arg1", "arg2", "arg3", NULL};
  if (nk_process_create("/hello.exe", args, NULL, "paging", &process1)) {
    nk_vc_printf("handle_proctest1: Failed to create new process\n");
    return -1;
  }
  if (nk_process_run(process1, 0)) {
    nk_vc_printf("handle_proctest1: Failed to run process\n");
    return -1;
  }
  nk_vc_printf("Finished running hello.exe!\n");
  
  return 0;
}
static struct shell_cmd_impl process_test_create_and_run = {
  .cmd      = "proctest1",
  .help_str = "proctest1",
  .handler  = handle_proctest1,
};
nk_register_shell_cmd(process_test_create_and_run);
