#include <nautilus/nautilus.h>
#include <nautilus/process.h>
#include <nautilus/shell.h>
#include <nautilus/barrier.h>
#include <nautilus/timer.h>

#define DO_PRINT 1

#if DO_PRINT
#define PRINT(...) nk_vc_printf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

#ifdef NAUT_CONFIG_CARAT_PROCESS
#define _PROCESS_ASPACE_TYPE "carat"
#else
#define _PROCESS_ASPACE_TYPE "paging"
#endif

/* Constants */
#define SIGTYPE1 12UL
#define SIGTYPE2 17UL
#define SIGTYPE3 18UL
#define SIGTYPE4 9UL

/* Globals */
nk_counting_barrier_t barrier;
nk_signal_action_t old_sig_act;
nk_signal_action_t older_sig_act;
volatile int sig_n = 0;

/* Create custom signal handler */
void sig_hand_hello_2(int sig_num);
nk_signal_action_t new_sig_act = {
    .handler = sig_hand_hello_2,
    .mask = {},
    .signal_flags = SIG_ACT_ONESHOT, /* run once, then switch back to default */ 
};

/* Default sig kill handler */
nk_signal_action_t sig_kill_act = {
    .handler = DEFAULT_SIG,
    .mask = {},
    .signal_flags = 0, 
};

void 
sig_hand_hello_2 (int sig_num)
{
    nk_vc_printf(".%d langis morf dlroW olleH\n", sig_num);
    if (sig_num == SIGTYPE2 || sig_num == SIGTYPE3) {
        if (sig_n < 1) {
            do_sigaction(SIGTYPE2, &old_sig_act, &older_sig_act); 
            do_sigaction(SIGTYPE3, &new_sig_act, &old_sig_act); 
            nk_counting_barrier(&barrier);
        }
        sig_n++;
    }
}


/* Thread that will receive signals */
void
sig_thread1 (void *in, void **out)
{
    /* Install a custom signal handler for signal of type 2 */
    nk_thread_t *me = get_cur_thread();  
    do_sigaction(SIGTYPE2, &new_sig_act, &old_sig_act); 

    /* Join barrier to let T2 know I'm ready to receive signals */
    nk_counting_barrier(&barrier);

    /* Loop until I receive all signals */
    while (1) {
        /* After receiving my second custom signal, I'm free to exit */
        if (sig_n >= 2) {
            nk_vc_printf("Thread 1 exiting. Success!\n");
            sig_n = 0;
            return;
        }
    }
    return;
}

/* Thread that will send signals */
void
sig_thread2 (void *in, void **out)
{
    nk_thread_t *thread1 = (nk_thread_t*)in;
 
    /* wait for custom handler to be registered w/ do_sigaction() */
    nk_counting_barrier(&barrier);

    /* Send first signal to t1, should use "Hello World" signal handler */
    nk_vc_printf("Sending signal %lu to thread: %p.\n", SIGTYPE1, thread1);
    if (nk_signal_send(SIGTYPE1, 0, thread1, SIG_DEST_TYPE_THREAD)) {
        nk_vc_printf("Couldn't send signal. Sigtest failed.\n");
        return;
    }

    /* Send first signal to t1, should use custom signal handler */
    nk_vc_printf("Sending signal %lu to thread: %p.\n", SIGTYPE2, thread1);
    if (nk_signal_send(SIGTYPE2, 0, thread1, SIG_DEST_TYPE_THREAD)) {
        nk_vc_printf("Couldn't send signal. Sigtest failed.\n");
        return;
    }

    /* Wait for t1 to handle first signal #17 */
    nk_counting_barrier(&barrier);

    /* Should be back to "Hello World" signal handler! */
    nk_vc_printf("Sending signal %lu to thread: %p.\n", SIGTYPE2, thread1);
    if (nk_signal_send(SIGTYPE2, 0, thread1, SIG_DEST_TYPE_THREAD)) {
        nk_vc_printf("Couldn't send signal. Sigtest failed.\n");
        return;
    }

    /* Should be custom signal handler, causing thread 1 to exit */
    nk_vc_printf("Sending signal %lu to thread: %p.\n", SIGTYPE3, thread1);
    if (nk_signal_send(SIGTYPE3, 0, thread1, SIG_DEST_TYPE_THREAD)) {
        nk_vc_printf("Couldn't send signal. Sigtest failed.\n");
        return;
    }
}

/* Thread that will receive exit signal */
void
sig_thread3 (void *in, void **out)
{
    uint64_t x = 0;
    while (1) {
        if (!(x%1000000)) {
            nk_vc_printf("sig_thread3: x (%lu) % 10000 = 0!\n", x);
        }
        x++;
    }

}

// create two threads, have 1 thread signal the other
static int
handle_sigtest (char * buf, void * priv)
{
    nk_counting_barrier_init(&barrier, 2);
    nk_thread_t *thread1 = 0;
    nk_thread_t *thread2 = 0;

    if (nk_thread_create(sig_thread1, 0, 0, 0, 0, (void **)&thread1, -1)) {
        nk_vc_printf("handle_sigtest: Failed to create new thread\n");
        return -1;
    }
    if (nk_thread_run(thread1)) {
        nk_vc_printf("handle_sigtest: Failed to run thread 1\n");
        return -1;
    }

    if (nk_thread_create(sig_thread2, (void *)thread1, 0, 0, 0, (void **)&thread2, -1)) {
        nk_vc_printf("handle_sigtest: Failed to create new thread\n");
        return -1;
    }
    if (nk_thread_run(thread2)) {
        nk_vc_printf("handle_sigtest: Failed to run thread 2\n");
        return -1;
    }
  
    return 0;
}

/* Create a process and send it a signal */
static int
handle_sigtest2 (char * buf, void * priv)
{
    nk_process_t *p1;
 
    if (nk_process_create("/hello.exe", NULL, NULL, "paging", &p1)) {
        nk_vc_printf("handle_proctest1: Failed to create new process\n");
        return -1;
    }
    if (nk_process_run(p1, 0)) {
        nk_vc_printf("handle_proctest1: Failed to run process\n");
        return -1;
    }
    nk_sleep(1000000);    
 
    /* Send a signal to p1 */
    nk_vc_printf("Sending signal %lu to process: %p.\n", SIGTYPE1, p1);
    if (nk_signal_send(SIGTYPE1, 0, p1, SIG_DEST_TYPE_PROCESS)) {
        nk_vc_printf("Couldn't send signal. Sigtest failed.\n");
        return -1;
    }
  
    return 0;
}

/* Test if fatal signals work for threads */
static int
handle_sigtest3 (char * buf, void * priv)
{
    nk_thread_t *thread1 = 0;

    if (nk_thread_create(sig_thread3, 0, 0, 0, 0, (void **)&thread1, -1)) {
        nk_vc_printf("handle_sigtest: Failed to create new thread\n");
        return -1;
    }
    if (nk_thread_run(thread1)) {
        nk_vc_printf("handle_sigtest: Failed to run thread 1\n");
        return -1;
    }

    //do_sigaction(SIGTYPE4, &sig_kill_act, &old_sig_act); 
    
    /* Send a signal to thread1 */
    nk_vc_printf("Sending fatal signal %lu to thread1: %p.\n", SIGTYPE4, thread1);
    if (nk_signal_send(SIGTYPE4, 0, thread1, SIG_DEST_TYPE_THREAD)) {
        nk_vc_printf("Couldn't send signal. Sigtest failed.\n");
        return -1;
    }
    return 0;
}

/* Create a process and send it a signal */
static int
handle_sigtest4 (char * buf, void * priv)
{

    int argc = 0;
    char* argv[64] = {0};
    char* argp = buf;
    argv[0] = buf;
    for (argc = 1; argc < 64; argc++) {
        while (*argp != ' ' && *argp != 0) {
            argp++;
        }
        if (*argp == 0) {
            break;
        }
        *argp = 0;
        argp++;
        argv[argc] = argp;
    }

    nk_process_t *p1;
 
    nk_vc_printf("Starting process: %s\n", argv[1]);
    if (nk_process_create(argv[1], argv + 1, NULL, _PROCESS_ASPACE_TYPE, &p1)) {
        nk_vc_printf("handle_proctest1: Failed to create new process\n");
        return -1;
    }
    if (nk_process_run(p1, 0)) {
        nk_vc_printf("handle_proctest1: Failed to run process\n");
        return -1;
    }
    nk_sleep(1000000);    
 
    /* Send a fatal signal to p1 */
    nk_vc_printf("Sending fatal signal %lu to process: %p.\n", SIGTYPE4, p1);
    if (nk_signal_send(SIGTYPE4, 0, p1, SIG_DEST_TYPE_PROCESS)) {
        nk_vc_printf("Couldn't send signal. Sigtest failed.\n");
        return -1;
    }
  
    return 0;
}


/* Define shell command structs */
static struct shell_cmd_impl signal_test_impl = {
  .cmd      = "sigtest",
  .help_str = "sigtest",
  .handler  = handle_sigtest,
};

static struct shell_cmd_impl signal_test_impl2 = {
  .cmd      = "sigtest2",
  .help_str = "sigtest2",
  .handler  = handle_sigtest2,
};

static struct shell_cmd_impl signal_test_impl3 = {
  .cmd      = "sigtest3",
  .help_str = "sigtest3",
  .handler  = handle_sigtest3,
};

static struct shell_cmd_impl signal_test_impl4 = {
  .cmd      = "sigtest4",
  .help_str = "sigtest4",
  .handler  = handle_sigtest4,
};

/* Register command with shell */
nk_register_shell_cmd(signal_test_impl);
nk_register_shell_cmd(signal_test_impl2);
nk_register_shell_cmd(signal_test_impl3);
nk_register_shell_cmd(signal_test_impl4);
