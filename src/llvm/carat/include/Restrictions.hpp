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
 * Copyright (c) 2021, Souradip Ghosh <sgh@u.northwestern.edu>
 * Copyright (c) 2021, Drew Kersnar <drewkersnar2021@u.northwestern.edu>
 * Copyright (c) 2021, Brian Suchy <briansuchy2022@u.northwestern.edu>
 * Copyright (c) 2021, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2021, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Drew Kersnar, Souradip Ghosh, 
 *          Brian Suchy, Simone Campanoni, Peter Dinda 
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#pragma once

#include "Utils.hpp"

using namespace llvm;

class RestrictionsHandler : public InstVisitor<RestrictionsHandler>
{
    /*
     * TOP --- The RestrictionsHandler is designed to analyze 
     * and/or transform the IR for potentially problematic memory 
     * instructions or pointers that may not be handled properly 
     * by the standard KARAT compiler.
     *
     * Designed as an instruction visitor FOR NOW --- currently
     * analyzes only call instructions, but will expand quickly
     *
     * Some definitions:
     *
     * - External function: A function that does not have a
     *   body present in the parent module 
     *
     * - Escaping pointer: There is a broad definition of an escape
     *   and even a specific definition used in CARAT. HOWEVER, in
     *   THIS class, we are focusing on pointers escaping *** via
     *   a call to an external function *** . So, whenever we say
     *   "escaping pointer" in this class, we are ALWAYS referring
     *   to the aformentioned definition.
     *
     */ 

public:

    /*
     * Constructors
     */ 
    RestrictionsHandler(Function *F);


    /*
     * Drivers
     */ 
    void AnalyzeAllCalls(void);

    void PinAllEscapingPointers(void);

    void PrintAnalysis(void);


    /*
     * Visitors
     */ 
    void visitCallInst(CallInst &I);


private:

    /*
     * Passed state
     */ 
    Function *F;


    /*
     * Analysis state --- per function. Why have so much state? 
     * Mostly for debugging but also because I didn't engineer 
     * this right, piss off.
     */ 
    std::unordered_set<CallInst *> IndirectCalls;
    
    std::unordered_set<CallInst *> ExternalFunctionCalls;

    std::unordered_set<CallInst *> TrackedCallsNotAffectingMemory; /* Tracked = (Indirect | External function) */

    std::unordered_set<CallInst *> TrackedCallsMayAffectingMemory; /* Tracked = (Indirect | External function) */

    std::unordered_map<
        Value *, /* [key] Pointer that's escaping */
        std::unordered_set<CallInst *> /* [val] External function call(s) by which [key] is escaping */
    > PointersEscapingViaTrackedCalls;


    /*
     * Private methods
     */
    bool _mayContainPointerType(Type *T);
    
};
