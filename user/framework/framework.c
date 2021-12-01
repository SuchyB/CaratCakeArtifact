#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <nautilus/nautilus_exe.h>
#include "aspace.h"
#include "profile.h"


/*
 * ---------- Profiling ----------
 */ 

static inline uint64_t __attribute__((always_inline))
rdtsc (void)
{
  uint32_t lo, hi; 
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return lo | ((uint64_t)(hi) << 32);
}


// Space for the signature of the final binary. Set by nsign after final link.
__attribute__((section(".naut_secure"))) unsigned char __NAUT_SIGNATURE[16];

// pointer to the NK function table
// to be filled in by the loader
void * (**__nk_func_table)(); 

extern int _start();

__attribute__((noinline, used, annotate("nocarat")))
void __nk_exec_entry(void *in, void **out, void * (**table)())
{
    __nk_func_table = table;

    struct nk_crt_proc_args *crt_args = (struct nk_crt_proc_args*)(in);
    
    // Prepare to jump to the C runtime, which expects a specific stack configuration
    __asm__(
        "pushq $0\n" /* NULL after envp */
        "mov $0, %%rax\n" /* rax is the iterator */
        "nk_loader_crt_env_loop:\n"
        "mov (%2, %%rax, 8), %%rcx\n" /* Read members of envp, rcx is the pointer */
        "cmpq $0, %%rcx\n" /* If pointer is NULL, don't push and exit loop */
        "je nk_loader_crt_env_loop_done\n"
        "pushq %%rcx\n" /* Push env pointer to stack */
        "inc %%rax\n"
        "je nk_loader_crt_env_loop_done\n"
        "nk_loader_crt_env_loop_done:\n"
        "pushq $0\n" /* NULL after argv */
        "test %0, %0\n"
        "je nk_loader_crt_arg_loop_done\n"
        "mov %0, %%rax\n"
        "dec %%rax\n"
        "\n"
        "nk_loader_crt_arg_loop:\n"
        "pushq (%1, %%rax, 8)\n" /* Push members of argv */
        "dec %%rax\n"
        "cmpq $0, %%rax\n"
        "jge nk_loader_crt_arg_loop\n"
        "nk_loader_crt_arg_loop_done:\n"
        "pushq %0\n" /* argc */
        "movq $0, %%rdx\n" /* Shared library termination function, which doesn't exist.
                              RDX is the only gpr read by _start (other than RSP, which is implicitly used) */
        "jmp _start\n" 
        :
        : "r"((uint64_t)crt_args->argc), "r"(crt_args->argv), "r"(crt_args->envp)
        : "rax", "rcx"
    );
}

#ifdef USE_NK_MALLOC

__attribute__((malloc))
void* malloc(size_t x) {
    return __nk_func_table[NK_MALLOC](x);
}

__attribute__((malloc))
void* calloc(size_t num, size_t size) {
    const size_t total_size = num * size;
    void* allocation = __nk_func_table[NK_MALLOC](total_size);
    memset(allocation, 0, total_size);
    return allocation;
}

void free(void* x) {
    __nk_func_table[NK_FREE](x);
}

void* realloc(void* p, size_t s) {
    return __nk_func_table[NK_REALLOC](p, s);
}

#endif

#define USER_TIMING 0

__attribute__((noinline, used, annotate("nocarat")))
void * nk_func_table_access(volatile int entry_no, void *arg1, void *arg2) {
  BACKSTOP NULL;
  return __nk_func_table[entry_no]((char*)arg1, arg2);
}


uint64_t num_mallocs = 0;
uint64_t total_malloc_time = 0;
uint64_t num_escapes= 0;
uint64_t total_escape_time = 0;


__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_instrument_global(void *ptr, uint64_t size, uint64_t global_ID) {
    BACKSTOP;
    __nk_func_table[NK_CARAT_INSTRUMENT_GLOBAL](ptr, size, global_ID);
    return;
}

__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_instrument_malloc(void *ptr, uint64_t size) {
    BACKSTOP;
#if USER_TIMING
    num_mallocs++;
    uint64_t malloc_timing_start = rdtsc();
#endif
    __nk_func_table[NK_CARAT_INSTRUMENT_MALLOC](ptr, size);
#if USER_TIMING
    total_malloc_time += rdtsc() - malloc_timing_start;
#endif

}

__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_instrument_calloc(void *ptr, uint64_t size_of_element, uint64_t num_elements) {
    BACKSTOP;
    __nk_func_table[NK_CARAT_INSTRUMENT_CALLOC](ptr, size_of_element, num_elements);
}

__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_instrument_realloc(void *ptr, uint64_t size, void *old_address) {
    BACKSTOP;
    __nk_func_table[NK_CARAT_INSTRUMENT_REALLOC](ptr, size, old_address);
}

__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_instrument_free(void *ptr) {
    BACKSTOP;
    __nk_func_table[NK_CARAT_INSTRUMENT_FREE](ptr);
}

__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_instrument_escapes(void *ptr) {
    BACKSTOP;
#if USER_TIMING
    num_escapes++;
    uint64_t escape_timing_start = rdtsc();
#endif
    __nk_func_table[NK_CARAT_INSTRUMENT_ESCAPE](ptr);
#if USER_TIMING
    total_escape_time += rdtsc() - escape_timing_start;
#endif

}


// ---
#if USER_TIMING
static uint64_t total_guard_time = 0;
static uint64_t num_guards = 0;
#endif

__attribute__((destructor, used, optnone, noinline, annotate("nocarat")))
void _results(void)
{
    /*
     * HACK
     */
    rand();
    return;
#if USER_TIMING
    printf("---results---\n");
#if USER_REGION_CHECK
    printf("num_guards: %lu\n", num_guards);
    printf("average guard time: %lu\n", total_guard_time / num_guards);
#endif
    printf("num_mallocs: %lu\n", num_mallocs);
    if (num_mallocs) printf("average malloc instrument time: %lu\n", total_malloc_time / num_mallocs);
    printf("num_escapes: %lu\n", num_escapes);
    if (num_escapes) printf("average escapes instrument time: %lu\n", total_escape_time / num_escapes);
    fflush(stdout);
#endif

    return;
}


__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_guard_address(void *address, int is_write) {
  BACKSTOP;
   
#if USER_REGION_CHECK
    /*
     * Fetch aspace and perform cached checks at user level
     */ 
    nk_aspace_t *aspace = ((nk_aspace_t *) __nk_func_table[NK_ASPACE_PTR]);
    nk_aspace_carat_t *caratAspace =  (nk_aspace_carat_t*)(aspace->state);
    
    nk_aspace_region_t *stack = caratAspace->initial_stack,
                       *blob = caratAspace->initial_blob,
                       *region = NULL;


#if USER_TIMING
    uint64_t timing_start = rdtsc();
    num_guards++;
#endif

    
    /*
     * Check @address against the stack
     */ 
    if (false 
        || (address >= stack->va_start)
        || (address < (stack->va_start + stack->len_bytes))) 
    {
        region = stack;
        region->requested_permissions |= is_write + 1;
#if USER_TIMING
        total_guard_time += rdtsc() - timing_start;
#endif
        return;
    }


    /*
     * Check @address against the blob
     */ 
    else if (
        false
        || (address >= blob->va_start)
        || (address < (blob->va_start + blob->len_bytes))
    )
    { 
        region = blob;
        region->requested_permissions |= is_write + 1;
#if USER_TIMING
        total_guard_time += rdtsc() - timing_start;
#endif
        return;
    }


    __nk_func_table[NK_CARAT_GENERIC_PROTECT](address, is_write, (void *) aspace);

#if USER_TIMING
    total_guard_time += rdtsc() - timing_start;
#endif

#else
    __nk_func_table[NK_CARAT_GENERIC_PROTECT](address, is_write);
#endif

    return;

}

__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_guard_callee_stack(uint64_t stack_frame_size) {
    BACKSTOP;
    __nk_func_table[NK_CARAT_STACK_PROTECT](stack_frame_size);
}

__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_pin_pointer(void *address) {
    __nk_func_table[NK_CARAT_PIN_DIRECT](address);
}

__attribute__((noinline, used, annotate("nocarat")))
void nk_carat_pin_escaped_pointer(void *escape) {
    BACKSTOP;
    __nk_func_table[NK_CARAT_PIN_ESCAPE](escape);
}





