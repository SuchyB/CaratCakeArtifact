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
 * Copyright (c) 2020, Drew Kersnar <drewkersnar2021@u.northwestern.edu>
 * Copyright (c) 2020, Gaurav Chaudhary <gauravchaudhary2021@u.northwestern.edu>
 * Copyright (c) 2020, Souradip Ghosh <sgh@u.northwestern.edu>
 * Copyright (c) 2020, Brian Suchy <briansuchy2022@u.northwestern.edu>
 * Copyright (c) 2020, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2020, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Drew Kersnar, Gaurav Chaudhary, Souradip Ghosh, 
 *          Brian Suchy, Peter Dinda 
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include "../include/Configurations.hpp"

/*
 * Function names to inject/directly handle, really need to JSON this
 */ 
const std::string CARAT_MALLOC = "nk_carat_instrument_malloc",
                  CARAT_REALLOC = "nk_carat_instrument_realloc",
                  CARAT_CALLOC = "nk_carat_instrument_calloc",
                  CARAT_REMOVE_ALLOC = "nk_carat_instrument_free",
                  CARAT_ESCAPE = "nk_carat_instrument_escapes",
                  CARAT_INIT = "nk_carat_init",
                  CARAT_GLOBAL_MALLOC = "nk_carat_instrument_global",
                  CARAT_GLOBALS_TARGET = "_nk_carat_globals_compiler_target",
                  CARAT_STACK_GUARD = "nk_carat_guard_callee_stack",
                  CARAT_PROTECT = "nk_carat_guard_address",
                  CARAT_PIN_DIRECT = "nk_carat_pin_pointer",
                  USER_STATS = "_results",
                  KERNEL_MALLOC = "_kmem_sys_malloc",
                  ASPACE_MALLOC = "__impl_alloc",
                  KERNEL_FREE = "kmem_sys_free",
                  ASPACE_FREE = "__impl_free",
                  USER_MALLOC = "malloc",
                  USER_CALLOC = "calloc",
                  USER_REALLOC = "realloc",
                  USER_FREE = "free",
                  ANNOTATION = "llvm.global.annotations",
                  NOCARAT = "nocarat";


/*
 * Important/necessary methods/method names to track
 */ 
std::unordered_set<std::string> CARATNames = {
    CARAT_MALLOC, 
    CARAT_REALLOC, 
    CARAT_CALLOC,
    CARAT_REMOVE_ALLOC, 
    CARAT_ESCAPE,
    CARAT_GLOBAL_MALLOC,
    CARAT_GLOBALS_TARGET,
    CARAT_STACK_GUARD,
    CARAT_PROTECT,
    CARAT_PIN_DIRECT,
    USER_STATS
};

std::unordered_map<std::string, Function *> CARATNamesToMethods;

std::unordered_set<Function *> CARATMethods;

std::unordered_map<AllocID, std::string> IDsToKernelAllocMethods = {
    { AllocID::SysMalloc, KERNEL_MALLOC } ,
    { AllocID::ASpaceMalloc, ASPACE_MALLOC },
    { AllocID::SysFree, KERNEL_FREE },
    { AllocID::ASpaceFree, ASPACE_FREE }
};

std::unordered_map<AllocID, std::string> IDsToUserAllocMethods = {
    { AllocID::UserMalloc, USER_MALLOC },
    { AllocID::UserCalloc, USER_CALLOC },
    { AllocID::UserRealloc, USER_REALLOC },
    { AllocID::UserFree, USER_FREE },
};

std::unordered_map<std::string, Function *> KernelAllocNamesToMethods;

std::unordered_map<Function *, AllocID> KernelAllocMethodsToIDs;

std::unordered_map<std::string, Function *> UserAllocNamesToMethods;

std::unordered_map<Function *, AllocID> UserAllocMethodsToIDs;

std::unordered_set<Function *> AnnotatedFunctions;


/*
 * Command line options for pass
 */ 
cl::opt<bool> InstrumentingUserCode(
    "target-user",
    cl::init(false),
    cl::desc("Performs instrumentation for user code")
);

cl::opt<bool> NoGlobals(
    "fno-globals",
    cl::init(false),
    cl::desc("No instrumentation of global variables")
);

cl::opt<bool> NoMallocs( /* NOTE --- Covers all variants: realloc, calloc, etc. */
    "fno-mallocs",
    cl::init(false),
    cl::desc("No instrumentation of 'mallocs'")
);

cl::opt<bool> NoFrees(
    "fno-frees",
    cl::init(false),
    cl::desc("No instrumentation of 'frees'")
);

cl::opt<bool> NoEscapes(
    "fno-escapes",
    cl::init(false),
    cl::desc("No instrumentation of escapes")
);

cl::opt<bool> NoProtections(
    "fno-protections",
    cl::init(false),
    cl::desc("No protection check instrumentation")
);

cl::opt<bool> NoRestrictions(
    "fno-restrictions",
    cl::init(false),
    cl::desc("No restrictions analysis/instrumentation")
);

cl::opt<bool> NoVerify(
    "fno-verify",
    cl::init(false),
    cl::desc("No verification of runtime methods or transformations")
);

cl::opt<bool> InitExit(
    "init-exit",
    cl::init(false),
    cl::desc("Exit compilation upon initialization --- for debugging")
);

cl::opt<bool> Debug(
    "debug",
    cl::init(false),
    cl::desc("Turn on debugging outputs/prints")
);
