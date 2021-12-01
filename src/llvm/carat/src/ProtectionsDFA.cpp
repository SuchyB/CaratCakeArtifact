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

#include "../include/ProtectionsDFA.hpp"

/*
 * ---------- Constructors ----------
 */
ProtectionsDFA::ProtectionsDFA(
    Function *F,
    Noelle *N
    ) : F(F), N(N) {}


/*
 * ---------- Drivers ----------
 */
void ProtectionsDFA::Compute(void)
{
  /* 
   * Play god
   */ 
  _initializeUniverse();


  /*
   * Apply the Address Checking for Data Custody (AC/DC) DFA
   */
  auto DFE = N->getDataFlowEngine();

  TheResult = DFE.applyForward(
      F, 
      _computeGEN(), 
      _computeKILL(), 
      _initializeIN(), 
      _initializeOUT(), 
      _computeIN(), 
      _computeOUT()
      );

  return;
}


DataFlowResult *ProtectionsDFA::FetchResult(void)
{
  return this->TheResult;
}


/*
 * ---------- Private methods (DFA setup) ----------
 */
std::function<void (Instruction *I, DataFlowResult *Result)> ProtectionsDFA::_computeGEN(void)
{
  auto DFAGen = 
    []
    (Instruction *I, DataFlowResult *Result) -> void {

      /*
       * Handle memory instructions (stores and loads)
       */
      if (true
          && (!isa<StoreInst>(I))
          && (!isa<LoadInst>(I))) return;


      /*
       * Find pointer to guard
       */ 
      Value *PointerToGuard = nullptr;
      if (true 
          && STORE_GUARD
          && isa<StoreInst>(I))
      {
        StoreInst *Store = cast<StoreInst>(I);
        PointerToGuard = Store->getPointerOperand();
      } 
      else if (LOAD_GUARD)
      {
        LoadInst *Load = cast<LoadInst>(I);
        PointerToGuard = Load->getPointerOperand();
      }


      /*
       * If no pointer is found, do nothing
       */
      if (!PointerToGuard) return;


      /*
       * Fetch the GEN[I] set.
       */
      auto &InstGen = Result->GEN(I);


      /*
       * Add the pointer to guard in the GEN[I] set.
       */
      InstGen.insert(PointerToGuard);
      auto PTGVal = PointerToGuard;
      while(1){
        if(!PTGVal){
          break;
        }
        BitCastInst *BCI = dyn_cast<BitCastInst>(PTGVal);
        if (BCI) {
          PTGVal = BCI->getOperand(0);
          InstGen.insert(PTGVal);
          continue;
        }

        /*
         * Attempt to fetch the operand as a bitcast operator
         */ 
        BitCastOperator *BCO = dyn_cast<BitCastOperator>(PTGVal);
        if (BCO) {
          PTGVal = BCO->getOperand(0);
          InstGen.insert(PTGVal);
          continue;
        }

        GetElementPtrInst* GEPI = dyn_cast<GetElementPtrInst>(PTGVal);
        if(GEPI && GEPI->isInBounds()){
          PTGVal = GEPI->getPointerOperand();
          InstGen.insert(PTGVal);
          continue;
        }
        break;
      }
      return;
    };


  return DFAGen;
}


std::function<void (Instruction *I, DataFlowResult *Result)> ProtectionsDFA::_computeKILL(void)
{
  /*
   * TOP --- No concept of a KILL set in AC/DC???
   */ 
  auto DFAKill = 
    []
    (Instruction *I, DataFlowResult *Result) -> void {
      return;
    };

  return DFAKill;
}


void ProtectionsDFA::_initializeUniverse(void)
{
  /*
   * TOP --- Initialize IN and OUT sets, fetch ALL values
   * used in @this->F excluding incoming basic blocks from
   * PHINodes and possible external functions from pointers,
   * indirect calls, etc.
   */
  for (auto &B : *F)
  {
    for (auto &I : B)
    {
      /*
       * Add all instructions to the universe
       */
      TheUniverse.insert(&I);


      /*
       * Add all operand uses to the universe
       */ 
      for (auto Index = 0; 
          Index < I.getNumOperands(); 
          Index++)
      {
        Value *NextOperand = I.getOperand(Index);

        /*
         * Ignore basic block and function operands
         */  
        if (false
            || (isa<Function>(NextOperand))
            || (isa<BasicBlock>(NextOperand))) continue;

        TheUniverse.insert(NextOperand);
      }
    }
  }


  /*
   * Save arguments as well
   */  
  for (auto &Arg : F->args()) TheUniverse.insert(&Arg);


  /*
   * Set state needed for analysis
   */ 
  Entry = &*F->begin();
  First = &*Entry->begin();


  return;
}


std::function<void (Instruction *inst, std::set<Value *> &IN)> ProtectionsDFA::_initializeIN(void)
{
  /*
   * TOP --- initialize the IN set to the universe of 
   * values available in @this->F
   */ 
  auto InitIn = 
    [this] 
    (Instruction *I, std::set<Value *> &IN) -> void {
      if (I == First) { return; }
      IN = TheUniverse;
      return;
    };


  return InitIn;
}


std::function<void (Instruction *inst, std::set<Value *> &OUT)> ProtectionsDFA::_initializeOUT(void)
{
  /*
   * TOP --- initialize the OUT set to the universe of 
   * values available in @this->F
   */ 
  auto InitOut = 
    [this]
    (Instruction *I, std::set<Value *> &OUT) -> void {
      OUT = TheUniverse;
      return ;
    };

  return InitOut;
}


std::function<void (Instruction *inst, std::set<Value *> &IN, Instruction *predecessor, DataFlowResult *df)> ProtectionsDFA::_computeIN(void)
{
  /*
   * Define the IN set
   */
  auto ComputeIn = 
    [] 
    (Instruction *I, std::set<Value *> &IN, Instruction *Pred, DataFlowResult *DF) -> void {

      auto &OUTPred = DF->OUT(Pred);

      std::set<Value *> tmpIN{};
      std::set_intersection(
          IN.begin(), 
          IN.end(), 
          OUTPred.begin(), 
          OUTPred.end(),  
          std::inserter(tmpIN, tmpIN.begin())
          );

      IN = tmpIN;

      return ;

    };


  return ComputeIn;
}


std::function<void (Instruction *inst, std::set<Value *> &OUT, DataFlowResult *df)> ProtectionsDFA::_computeOUT(void)
{
  auto ComputeOUT = 
    [] 
    (Instruction *inst, std::set<Value *> &OUT, DataFlowResult *DF) -> void {

      /*
       * Fetch the IN[inst] set.
       */
      auto &IN = DF->IN(inst);


      /*
       * Fetch the GEN[inst] set.
       */
      auto &GEN = DF->GEN(inst);


      /*
       * Set the OUT[inst] set.
       */
      OUT = IN;
      OUT.insert(GEN.begin(), GEN.end());

      return ;
    };


  return ComputeOUT;
}

#endif













