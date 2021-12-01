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

#include "autoconf.h"

#if NAUT_CONFIG_USE_NOELLE

#include "../include/Protections.hpp"

/*
 * ---------- Constructors ----------
 */ 
ProtectionsHandler::ProtectionsHandler(
    Module *M,
    Noelle *N,
    std::function<ScalarEvolution * (Function *F)> FetchSELambda
) : M{M}, N{N}, FetchSELambda{FetchSELambda}
{
    /*
     * Perform initial processing
     */ 
    _buildNonCanonicalAddress();
}


/*
 * ---------- Drivers ----------
 */ 
void ProtectionsHandler::Protect(void)
{
    /*
     * Check user arguments
     */
    if (NoProtections) return;


    /*
     * Iterate over all functions in @this->M, compute the DFA 
     * and calls to the runtime protections method if possible
     */ 
    for (auto &F : *M)
    {
        /*
         * Skip functions that are not instrumentable
         */ 
        if (!(Utils::IsInstrumentable(F))) continue;


        /*
         * Compute the DFA for the current function 
         */ 
        ProtectionsDFA *PD = new ProtectionsDFA(
            &F, 
            N /* Noelle */
        );

        PD->Compute();


        /*
         * Inject guards
         */
        errs() << F << "\n";

        ProtectionsInjector *PI = new ProtectionsInjector(
            &F,
            FetchSELambda,
            PD->FetchResult(),
            NonCanonical,
            N /* Noelle */,
            nullptr /* ProtectionsMethod */
        );

        PI->Inject();
    }


    return;
}


/*
 * ---------- Private methods ----------
 */ 
#define NON_CANONICAL 0x22DEADBEEF22
void ProtectionsHandler::_buildNonCanonicalAddress(void)
{
    /*
     * TOP --- Build a non canonical address that the 
     * injector can directly refer to
     */ 

    /*
     * Set up builder and module info
     */
    llvm::IRBuilder<> Builder{M->getContext()};
    auto DataLayout = M->getDataLayout();


    /*
     * Set up "numNowPtr" --- i.e. the NonCanonical value --- FIX
     */ 
    ConstantInt *NonCanonicalValue = Builder.getInt64(NON_CANONICAL);
    IntegerType *PointerTy = Builder.getIntPtrTy(DataLayout);
    NonCanonical = 
        Builder.CreateIntToPtr(
            NonCanonicalValue,
            PointerTy
        );


    return;
}


#endif
