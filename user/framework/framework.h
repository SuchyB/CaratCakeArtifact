#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "profile.h"

/*
 * Function signatures for instrumentation methods
 */ 
void nk_carat_instrument_global(void *ptr, uint64_t size, uint64_t global_ID) ;
void nk_carat_instrument_malloc(void *ptr, uint64_t size) ;
void nk_carat_instrument_calloc(void *ptr, uint64_t size_of_element, uint64_t num_elements) ;
void nk_carat_instrument_realloc(void *ptr, uint64_t size, void *old_address) ;
void nk_carat_instrument_free(void *ptr) ;
void nk_carat_instrument_escapes(void *ptr) ; 
void nk_carat_guard_address(void *memory_address, int is_write) ;
void nk_carat_guard_callee_stack(uint64_t stack_frame_size) ;
void nk_carat_pin_pointer(void *address) ;
void nk_carat_pin_escaped_pointer(void *escape) ;
void _nk_carat_globals_compiler_target(void) ;
void _results(void);


/*
 * Compiler target to insert globals
 */ 
__attribute__((constructor, optnone, noinline, used, annotate("nocarat")))
void _nk_carat_globals_compiler_target(void) {
    BACKSTOP;
    /*
     * HACK
     */ 
    rand();
    return;
}



/*
 * HACK --- Link this function instead of the entire framework
 * such that the following happens:
 * a) All of the instrumentation methods' signatures persist in the
 *    benchmark bitcode generated
 * b) Current SVF/Noelle/PDG bugs analyzing framework code are 
 *    completely circumvented
 */ 
__attribute__((optnone, noinline, used, annotate("nocarat")))
void _framework_persist_function_signatures(void)
{
    
#define _DUMMY_SIZE 16
#define _DUMMY_ENTRIES 10

    volatile int condition = 0;
    if (condition)
    {
        void *test_ptr = malloc(_DUMMY_SIZE);
        test_ptr = calloc(_DUMMY_ENTRIES, _DUMMY_SIZE);
        test_ptr = realloc(test_ptr, _DUMMY_SIZE);
        free(test_ptr);
        nk_carat_instrument_global(NULL, 0, 0);
        nk_carat_instrument_malloc(NULL, 0);
        nk_carat_instrument_calloc(NULL, 0, 0);
        nk_carat_instrument_realloc(NULL, 0, NULL);
        nk_carat_instrument_free(NULL);
        nk_carat_instrument_escapes(NULL);
        nk_carat_guard_address(NULL, 0) ;
        nk_carat_guard_callee_stack(0) ;
        nk_carat_pin_pointer(NULL) ;
        nk_carat_pin_escaped_pointer(NULL) ;
        _nk_carat_globals_compiler_target();
        _results();
    }


    return;
}
