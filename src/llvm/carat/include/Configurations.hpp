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
 
#pragma once

#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DerivedUser.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/DataLayout.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <cassert>

#include "autoconf.h"

/*
 * Debugging
 */ 
#define DEBUG_INFO(str) do { if (Debug) { errs() << str; } } while (0)
#define OBJ_INFO(obj) do { if (Debug) { obj->print(errs()); errs() << "\n"; } } while (0)
#define VERIFY_DEBUG_INFO(str) do { if (!NoVerify) { errs() << str; } } while (0)
#define VERIFY_OBJ_INFO(obj) do { if (!NoVerify) { obj->print(errs()); errs() << "\n"; } } while (0)
#define DEBUG_ERRS if (Debug) errs()


using namespace llvm;


/*
 * Enumerator for kernel allocation methods
 */ 
enum AllocID
{
    SysMalloc=0,
    ASpaceMalloc,
    SysFree,
    ASpaceFree,
    UserMalloc,
    UserCalloc,
    UserRealloc,
    UserFree
};


/*
 * Constants --- disgusting
 */
extern
const std::string CARAT_MALLOC,
                  CARAT_REALLOC,
                  CARAT_CALLOC,
                  CARAT_REMOVE_ALLOC,
                  CARAT_ESCAPE,
                  CARAT_INIT,
                  CARAT_GLOBAL_MALLOC,
                  CARAT_GLOBALS_TARGET,
                  CARAT_STACK_GUARD,
                  CARAT_PROTECT,
                  CARAT_PIN_DIRECT,
                  USER_STATS,
                  KERNEL_MALLOC,
                  ASPACE_MALLOC,
                  KERNEL_FREE,
                  ASPACE_FREE,
                  USER_MALLOC,
                  USER_CALLOC,
                  USER_REALLOC,
                  USER_FREE,
                  ANNOTATION,
                  NOCARAT;


/*
 * Important/necessary methods/method names to track
 */ 
extern
std::unordered_set<std::string> CARATNames;

extern
std::unordered_map<std::string, Function *> CARATNamesToMethods;

extern
std::unordered_set<Function *> CARATMethods;

extern
std::unordered_map<AllocID, std::string> IDsToKernelAllocMethods;

extern
std::unordered_map<std::string, Function *> KernelAllocNamesToMethods;

extern
std::unordered_map<Function *, AllocID> KernelAllocMethodsToIDs;

extern
std::unordered_map<AllocID, std::string> IDsToUserAllocMethods;

extern
std::unordered_map<std::string, Function *> UserAllocNamesToMethods;

extern
std::unordered_map<Function *, AllocID> UserAllocMethodsToIDs;

extern
std::unordered_set<Function *> AnnotatedFunctions;


/*
 * Command line options for pass
 */ 
extern cl::opt<bool> InstrumentingUserCode;

extern cl::opt<bool> NoGlobals;

extern cl::opt<bool> NoMallocs;

extern cl::opt<bool> NoFrees;

extern cl::opt<bool> NoEscapes;

extern cl::opt<bool> NoProtections;

extern cl::opt<bool> NoRestrictions;

extern cl::opt<bool> NoVerify;

extern cl::opt<bool> InitExit;

extern cl::opt<bool> Debug;


