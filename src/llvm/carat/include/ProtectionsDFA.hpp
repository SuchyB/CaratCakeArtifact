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

#include "autoconf.h"

#include "Escapes.hpp"

#if NAUT_CONFIG_USE_NOELLE

#include "noelle/core/Noelle.hpp"


#define STORE_GUARD 1
#define LOAD_GUARD 1


class ProtectionsDFA
{

public:

    /*
     * Constructors
     */ 
    ProtectionsDFA(
        Function *F,
        Noelle *N
    );


    /*
     * Drivers
     */ 
    void Compute(void);
    DataFlowResult *FetchResult(void);


private:

    /*
     * Passed state
     */ 
    Function *F;
    Noelle *N;


    /*
     * New analysis state
     */     
    std::set<Value *> TheUniverse;
    BasicBlock *Entry;
    Instruction *First;
    DataFlowResult *TheResult;


    /*
     * Private methods
     */     
    std::function<void (Instruction *I, DataFlowResult *Result)> _computeGEN(void);

    std::function<void (Instruction *I, DataFlowResult *Result)> _computeKILL(void);

    void _initializeUniverse(void);

    std::function<void (Instruction *inst, std::set<Value *> &IN)> _initializeIN(void);

    std::function<void (Instruction *inst, std::set<Value *> &OUT)> _initializeOUT(void);

    std::function<void (Instruction *inst, std::set<Value *> &IN, Instruction *predecessor, DataFlowResult *df)> _computeIN(void);

    std::function<void (Instruction *inst, std::set<Value *>& OUT, DataFlowResult *df)> _computeOUT(void);

};

#endif
