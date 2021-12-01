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

#include "../include/Escapes.hpp"


/*
 * ----------- Constructors ----------- 
 */ 
EscapesHandler::EscapesHandler(Module *M) : M(M)
{
    /*
     * Perform initial processing
     */
    this->_getAllNecessaryInstructions();
}


/*
 * ----------- Drivers ----------- 
 */ 
void EscapesHandler::Inject()
{
    /*
     * Check pass settings
     */ 
    if (NoEscapes) return;


    /*
     * Set up for injection
     */ 
    Function *CARATEscape = CARATNamesToMethods[CARAT_ESCAPE];

    IRBuilder<> TypeBuilder{M->getContext()};
    Type *VoidPointerType = TypeBuilder.getInt8PtrTy();


    /*
     * Iterate
     */ 
    for (auto NextMemUse : MemUses)
    {
        /*
         * Debugging
         */ 
        errs() << "NextMemUse: " << *NextMemUse << "\n";
        
    
        /*
         * Set up insertion point
         */ 
        Instruction *InsertionPoint = Utils::GetPostTargetInsertionPoint(NextMemUse);
        assert(!!InsertionPoint 
               && "EscapesHandler::Inject: Can't find an insertion point!");


        /*
         * Set up IRBuilder
         */ 
        IRBuilder<> Builder = 
            Utils::GetBuilder(
                NextMemUse->getFunction(), 
                InsertionPoint
            );


        /*
         * Fetch the underlying store
         */ 
        StoreInst *NextStore = cast<StoreInst>(NextMemUse);


        /*
         * Cast the pointer operand of the store to a 'void *'
         * NOTE --- the pointer operand is the location at
         * which the value will be stored
         */ 
        Value *PointerOperand = NextStore->getPointerOperand();
        Value *PointerOperandCast = 
            Builder.CreatePointerCast(
                PointerOperand, 
                VoidPointerType
            );


        /*
         * Set up call parameters
         */ 
        ArrayRef<Value *> CallArgs = {
            PointerOperandCast
        };


        /*
         * Inject
         */ 
        CallInst *InstrumentEscape = 
            Builder.CreateCall(
                CARATEscape, 
                CallArgs
            );


        /*
         * Add metadata to injection
         */
        Utils::SetBaseInstrumentationMetadata(InstrumentEscape);
    }


    return;
}


/*
 * ----------- Private methods ----------- 
 */ 
void EscapesHandler::_getAllNecessaryInstructions()
{
    /*
     * TOP --- iterate over all functions --- find all 
     * store instructions to mark for instrumentations
     */ 

    /*
     * Iterate
     */ 
    for (auto &F : *M)
    {
        /*
         * Skip if non-instrumentable
         */ 
        if (!(Utils::IsInstrumentable(F))) { continue; }


        /*
         * Iterate --- search for stores
         */ 
        for (auto &B : F)
        {
            for (auto &I : B)
            {
                /*
                 * Skip all other instructions
                 */
                if (!(isa<StoreInst>(&I))) { continue; }


                /*
                 * Have a store --- check to see if the value
                 * operand (i.e. the value being store) is a
                 * pointer --- if so, we've caught an escape
                 * --- stash the store instruction in question
                 */ 
                StoreInst *NextStore = cast<StoreInst>(&I);
                if (NextStore->getValueOperand()->getType()->isPointerTy())
                    MemUses.insert(NextStore);  
            }
        }
    }

    return;
}
