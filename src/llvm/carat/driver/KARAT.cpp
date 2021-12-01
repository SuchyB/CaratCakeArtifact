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

#include "autoconf.h"
#include "./include/Restrictions.hpp"

#if NAUT_CONFIG_USE_NOELLE
#include "./include/Protections.hpp"
#else
#include "./include/Escapes.hpp"
#endif


#define FetchAllocMethods(type) \
    for (auto const &[ID, Name] : IDsTo##type##AllocMethods) \
    { \
        Function *AllocMethod = Utils::GetMethod(&M, Name); \
        type##AllocNamesToMethods[Name] = AllocMethod; \
        type##AllocMethodsToIDs[AllocMethod] = ID; \
    } \


namespace
{
struct CAT : public ModulePass
{
    static char ID;

    CAT() : ModulePass(ID) {}

    bool doInitialization(Module &M) override
    {
        /*
         * Debugging
         */  
        Utils::ExitOnInit();


        /*
         * Fetch the "nocarat" annotation attribute --- necessary
         * to find user-marked functions in kernel code
         */
        GlobalVariable *GV = M.getGlobalVariable(ANNOTATION);
        assert(!!GV && "doInitialization: 'nocarat' attribute not found!");


        /*
         * Fetch the annotated functions
         */
        Utils::FetchAnnotatedFunctions(GV);

        
        /*
         * Mark all annotated functions as noinline --- we
         * don't want marked functions to become instrumented
         * indirectly via inlining
         */ 
        for (auto AF : AnnotatedFunctions)
        {
            /*
             * Sanity check the annotated function
             */
            assert(!!AF && "Annotated function is nullptr!");
            

            /*
             * Add attribute
             */ 
            AF->addFnAttr(Attribute::NoInline);
        }


        /*
         * Now fetch all CARAT methods, stash them
         */
        for (auto Name : CARATNames)
        {
            Function *CARATMethod = Utils::GetMethod(&M, Name);
            CARATNamesToMethods[Name] = CARATMethod;
            CARATMethods.insert(CARATMethod);
        }


        /*
         * Now fetch all kernel allocation methods, stash them
         */
        if (InstrumentingUserCode) FetchAllocMethods(User)
        else FetchAllocMethods(Kernel)


        return false;
    }


    bool runOnModule(Module &M) override
    {
        /*
         * Debugging
         */  
        Utils::ExitOnInit();


        if (Debug || true)
        {
            /*
             * Output all intrinsics that exist in the module
             */ 
            errs() << "Now intrinsics!\n";
            for (auto &F : M) if (F.isIntrinsic()) errs() << F.getName() << "\n";


            /*
             * Vet allocation methods (of kernel OR userspace)
             */ 
            Utils::VetAllocMethods();
        }


        /*
         * --- Perform all CARAT instrumentation on the code ---
         */ 
#if NAUT_CONFIG_USE_NOELLE
        if (!NoProtections)
        {
            /*  
             * Fetch Noelle and SCEV (HACK)
             * make a lambda that takes a function * and returns a scev *
             * the lambda invokes getAnalysis
             * pass the lambda to the protections pass -- just invoke the function
             */
            auto FetchSE = 
            [this](Function *F) -> ScalarEvolution * {
                ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>(*F).getSE();
                return SE;
            } ;

            // std::unordered_map<Function *, ScalarEvolution *> SCEVs;
            // ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
            // for (auto &F : M)
            // {
            //     ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>(F).getSE();
            //     Protections P = Protections(SE);
            // }       


            // for (auto &F : M)
            // {
            //     ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>(F).getSE();
            //     errs() << "VERIFYING SE FOR " << F.getName() << "...\n";
            //     SE->verify();
            //     SCEVs[&F] = SE;
            // }         

            Noelle &NoelleAnalysis = getAnalysis<Noelle>();

            /*
             * Protections
             */ 
            ProtectionsHandler PH = ProtectionsHandler(&M, &NoelleAnalysis, FetchSE);
            PH.Protect();
        }
#endif


        /*
         * Allocation tracking
         */ 
        AllocationHandler AH = AllocationHandler(&M);
        AH.Inject();


        /*
         * Escapes tracking
         */ 
        EscapesHandler EH = EscapesHandler(&M);
        EH.Inject();


        /*
         * Analysis/transformation on prototype restrictions,
         * note that this functionality is per function
         */
        for (auto &F : M) 
        {
            if (Utils::IsInstrumentable(F))
            {
                RestrictionsHandler RH = RestrictionsHandler(&F);
                RH.AnalyzeAllCalls();
                RH.PinAllEscapingPointers();
                RH.PrintAnalysis();
            }
        }


        /*
         * Run verifier on each function instrumented
         */
        Utils::Verify(M);


        /*
         * Add statistics call to the end of main() for user programs
         */
        if (InstrumentingUserCode) Utils::InjectStats(M);


        return true;
    }


    void getAnalysisUsage(AnalysisUsage &AU) const override
    {   
        AU.addRequired<ScalarEvolutionWrapperPass>();

#if NAUT_CONFIG_USE_NOELLE
        /*
         * Use NOELLE IFF we need protections only
         */
        if (!NoProtections) AU.addRequired<Noelle>();
#endif

        return;
    } 

};
    
    
char CAT::ID = 0;
static RegisterPass<CAT> X("karat", "KARAT");

static CAT *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
                                        [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
            if(!_PassMaker){ PM.add(_PassMaker = new CAT());} }); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
                                        [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
            if(!_PassMaker){ PM.add(_PassMaker = new CAT());} }); // ** for -O0
}
