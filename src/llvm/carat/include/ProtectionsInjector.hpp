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

#if NAUT_CONFIG_USE_NOELLE

#include "ProtectionsDFA.hpp"


class GuardInfo 
{

    /*
     * TOP --- Packaging the information necessary to build and inject guards
     */

public:

    /*
     * Constructors
     */
    GuardInfo(
        Instruction *IL,
        Value *PTG,
        bool IW,
        Function *FTI,
        const std::string MDTS,
        const std::string MDL,
        unsigned NI=1
    ) : InjectionLocation(IL), 
        PointerToGuard(PTG), 
        IsWrite(IW), 
        FunctionToInject(FTI),
        MDTypeString(MDTS),
        MDLiteral(MDL),
        NumInjections(NI) {};


    /*
     * Public state
     */
    Instruction *InjectionLocation;
    Value *PointerToGuard;
    bool IsWrite; /* i.e. store=TRUE, load=FALSE */
    Function *FunctionToInject;
    const std::string MDTypeString, MDLiteral;
    unsigned NumInjections;

};



class ProtectionsInjector : public InstVisitor<ProtectionsInjector>
{

public:

    /*
     * Constructors
     */ 
    ProtectionsInjector(
        Function *F, 
        std::function<ScalarEvolution * (Function *F)> FetchSELambda,
        DataFlowResult *DFR, 
        Value *NonCanonical,
        Noelle *noelle,
        Function *ProtectionsMethod
    );


    /*
     * Drivers
     */ 
    void Inject (void);


    /*
     * Visitors
     */ 
    void visitCallInst(CallInst &I);
    void visitStoreInst(StoreInst &I);
    void visitLoadInst(LoadInst &I);
    void visitInvokeInst(InvokeInst &I);


private:

    /*
     * Passed state
     */ 
    Function *F, *ProtectionsMethod;

    std::function<ScalarEvolution * (Function *F)> FetchSELambda;

    DataFlowResult *DFR;

    Value *NonCanonical;

    Noelle *noelle;

    std::vector<LoopDependenceInfo *> *AllLoops;

    StayConnectedNestedLoopForest *LoopForestOfFunction;

    PDG *FDG;

    /*
     * New analysis state
     */ 
    std::unordered_map<BasicBlock *, LoopDependenceInfo *> BasicBlockToLoopMap;

    Instruction *First;

    bool AllocaOutsideEntry=false;
    bool InjectedCallGuardAtFirst=false;


    std::unordered_map<Function *, bool> InstrumentedFunctions;

    std::unordered_map<
        Instruction *, /* The load/store/call for which the guard is generated --- SYMBOLIC */
        GuardInfo * /* The information needed to generate the guard (see above) */
    > InjectionLocations;


    /*
     * Counters/Statistics
     */     
    uint64_t redundantGuard = 0;
    uint64_t loopInvariantGuard = 0;
    uint64_t scalarEvolutionGuard = 0;
    uint64_t nonOptimizedGuard = 0;
    uint64_t callGuardOpt = 0;


    /*
     * Private methods
     */ 
    void _doTheBusiness(void);

    void _findInjectionLocations(void);

    std::vector<Value *> _buildStackGuardArgs(GuardInfo *GI);

    std::vector<Value *> _buildGenericProtectionArgs(GuardInfo *GI);

    void _doTheInject(void);

    bool _optimizeForLoopInvariance(
        LoopDependenceInfo *NestedLoop,
        Instruction *I, 
        Value *PointerOfMemoryInstruction, 
        bool IsWrite
    );

    bool _optimizeForInductionVariableAnalysis(
        LoopDependenceInfo *NestedLoop,
        Instruction *I, 
        Value *PointerOfMemoryInstruction, 
        bool IsWrite
    );

    bool _optimizeForSCEVAnalysis(
        LoopDependenceInfo *NestedLoop,
        Instruction *I, 
        Value *PointerOfMemoryInstruction, 
        bool IsWrite
    );

    bool _isAPointerReturnedByAllocator(Value *V);

    Value *_fetchBitCastOperand(Value *Pointer);
    
    bool _isSafeArgument(Instruction*, Argument*);
    
    bool _isValidSCEV(const llvm::SCEV* scevPtr);

    Value *_fetchGEPBasePointer(
        Value *Pointer,
        bool CheckInBounds
    );

    bool _isASafeMemoryConstruct(Value *Pointer);
    Value* unCastGepLoad(Value*);
    std::function<void (Instruction *inst, Value *pointerOfMemoryInstruction, bool isWrite)> _findPointToInsertGuard(void);

    void _allocaOutsideFirstBBChecker(void);

    template<typename MemInstTy>
    void _invokeLambda(
        MemInstTy *I,
        bool IsWrite
    );

    void _printGuards(void);

    Value *_fetchInvariantFromInstruction (
        Instruction *I,
        InvariantManager *InvManager
    ) ;

    Instruction *_fetchIVFromInstruction (
        Instruction *I,
        InductionVariableManager *IVManager
    ) ;

};

#endif
