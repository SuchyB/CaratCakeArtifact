/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2017, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2017, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

/* Preliminary interface to Nautilus functions from "user" executables */

#ifndef __NAUTILUS_EXE
#define __NAUTILUS_EXE

struct nk_crt_proc_args {
  char** argv;
  char** envp;
  int argc;
};

#define NK_VC_PRINTF 0
#define NK_CARAT_INSTRUMENT_MALLOC 1
#define NK_CARAT_INSTRUMENT_FREE 2
#define NK_CARAT_INSTRUMENT_ESCAPE 3
#define NK_CARAT_CHECK_PROTECTION 4
#define NK_CARAT_INSTRUMENT_GLOBAL 5
#define NK_CARAT_INSTRUMENT_CALLOC 6
#define NK_CARAT_INSTRUMENT_REALLOC 7
#define NK_CARAT_GLOBALS_COMPILER_TARGET 8
#define NK_CARAT_INIT 9
#define NK_CARAT_GENERIC_PROTECT 10
#define NK_CARAT_STACK_PROTECT 11
#define NK_CARAT_PIN_DIRECT 12
#define NK_CARAT_PIN_ESCAPE 13
#define NK_MALLOC 14
#define NK_FREE 15
#define NK_REALLOC 16
#define NK_ASPACE_PTR 17


#ifdef NAUTILUS_EXE
// Being included from "user" space

// Add a macro version for eaxh exposed function here
// #define nk_vc_printf(...) __nk_func_table[NK_VC_PRINTF](__VA_ARGS__)

// defined in the framework code
extern void * (**__nk_func_table)(); 

void* nk_func_table_access(volatile int entry_no, void* arg1, void* arg2);

#define nk_vc_printf(...) nk_func_table_access(NK_VC_PRINTF, __VA_ARGS__)

/*
 * User framework switches 
 */
#define USER_REGION_CHECK 0
#define USER_TIMING 0

#else

// included from the kernel code

// no implementation here

#endif

#endif

