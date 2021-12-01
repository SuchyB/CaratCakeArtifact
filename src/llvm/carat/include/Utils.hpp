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

/*
 * Utils.hpp
 * ----------------------------------------
 * 
 * All utility and debugging methods necessary for CARAT
 * transform. Split into two namespaces for clarity.
 */

#pragma once

#include "Configurations.hpp"

using namespace llvm;

namespace Utils
{
    /*
     * Init
     */ 
    void ExitOnInit(void);

    Function *GetMethod(
        Module *M,
        const std::string Name
    );


    /*
     * Injection helpers
     */ 
    IRBuilder<> GetBuilder(
        Function *F, 
        Instruction *InsertionPoint
    );

    IRBuilder<> GetBuilder(
        Function *F, 
        BasicBlock *InsertionPoint
    );

    bool IsInstrumentable(Function &F);

    Instruction *GetPostTargetInsertionPoint(Instruction *Target);

    void InjectStats(Module &M);


    /*
     * Attribute handling
     */ 
    void FetchAnnotatedFunctions(GlobalVariable *GV);


    /*
     * Allocation size handling
     */ 
    uint64_t GetPrimitiveSizeInBytes(Type *ObjectType);
    
    uint64_t CalculateObjectSize(
        Type *ObjectType,
        DataLayout *Layout
    );


    /*
     * Verification helpers
     */ 
    bool Verify(Module &M);

    void VetAllocMethods(void);


    /*
     * Metadata handlers
     */ 
    void SetInstrumentationMetadata(
        Instruction *I,
        const std::string MDTypeString,
        const std::string MDLiteral
    );

    void SetBaseInstrumentationMetadata(Instruction *I);

    bool HasBaseInstrumentationMetadata(Instruction *I);

} 