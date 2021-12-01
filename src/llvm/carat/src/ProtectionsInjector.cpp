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

// #if NAUT_CONFIG_USE_NOELLE
#if 1

#include "../include/ProtectionsInjector.hpp"

/*
 * ---------- Constructors ----------
 */
ProtectionsInjector::ProtectionsInjector(
    Function *F, 
    std::function<ScalarEvolution * (Function *F)> FetchSELambda,
    DataFlowResult *DFR, 
    Value *NonCanonical,
    Noelle *noelle,
    Function *ProtectionsMethod
    ) : F(F), FetchSELambda(FetchSELambda), DFR(DFR), NonCanonical(NonCanonical), ProtectionsMethod(ProtectionsMethod), noelle(noelle) 
{

  /*
   * Set new state from NOELLE
   */ 
  this->AllLoops = noelle->getLoops();
  auto LoopStructuresOfFunction = noelle->getLoopStructures(F);
  this->LoopForestOfFunction = noelle->organizeLoopsInTheirNestingForest(*LoopStructuresOfFunction); 
  this->BasicBlockToLoopMap = noelle->getInnermostLoopsThatContains(*(this->AllLoops));
  this->FDG = noelle->getFunctionDependenceGraph(F);


  /*
   * Perform initial analysis
   */ 
  _allocaOutsideFirstBBChecker();


  /*
   * Fetch the first instruction of the entry basic block
   */ 
  First = F->getEntryBlock().getFirstNonPHI();
}


/*
 * ---------- Drivers ----------
 */
void ProtectionsInjector::Inject(void)
{
  /*
   * Find all locations that guards need to be injected
   */
  _findInjectionLocations();


  /*
   * Now do the inject
   */ 
  _doTheInject();


  /*
   * Verify the transformations
   */
  // assert(verifyFunction(*F) != true);
  if (verifyFunction(*F, &errs())) errs() << "VERIFICATION FAILED\n";
  else errs() << "VERIFICATION WORKED\n";

  errs() << *F << "\n";


  return;
}



/*
 * ---------- Visitor methods ----------
 */
void ProtectionsInjector::visitInvokeInst(InvokeInst &I)
{
  /*
   * Assumption for Nautilus --- no handling of invokes
   */  
  errs() << "Found a invoke instruction in function " << F->getName() << "\n"
    << I << "\n";

  abort();
}


void ProtectionsInjector::visitCallInst(CallInst &I)
{
  /*
   * Fetch the callee of @I
   */ 
  Function *Callee = I.getCalledFunction();


  /*
   * Debugging --- FIX 
   * 
   * NOTE --- We are instrumenting all indirect calls
   * because we have no idea what to do about this
   */ 
  if (I.isIndirectCall()) 
  {
    errs() << "Found an indirect call! Instrumenting for now ... \n" 
      << I << "\n";
  }


  /*
   * Debugging --- FIX
   * 
   * NOTE --- Ideally, only some intrinsics should be 
   * instrumented (i.e. llvm.memcpy, etc.), and markers
   * (i.e. llvm.lifetime, etc.) should be ignored. For
   * now, we are instrumenting ALL intrinsics as a 
   * conservative approach
   * 
   * APRIL, 2021 --- NOT HANDLING INTRINSICS --- REVISIT
   * 
   * Must ignore inline assembly
   */  
  if (false
      || I.isInlineAsm() 
      || (Callee && (Callee->isIntrinsic())))
  {
    errs() << "visitCallInst: Can't handle instrinsics or inline assembly\n";
    return;
  }

  // If the call is a tail call, the function is a part of the current (already checked) stack frame and does not require a check.
  if(I.isTailCall()){
    return;
  }



  /*
   * If the callee of @I has already been instrumented and 
   * all stack locations are at the top of the entry basic
   * block (@this->AllocaOutsideEntry), then nothing else 
   * needs to be done --- return
   *
     */
  if (true
      && !AllocaOutsideEntry
      && (Callee)
      && ((InstrumentedFunctions[Callee]))
      ) {
    return;
  }
  
  errs() << "Callee failed: " << I << ", " << !AllocaOutsideEntry << ", " << Callee << ", " << InstrumentedFunctions[Callee] << "\n";


  /*
   * If not all stack locations are grouped at the 
   * top of the entry basic block, we cannot hoist
   * any guards of the call instruction --- instrument
   * @I at @I
   * 
   * Otherwise, hoist the guard for @I to the first 
   * instruction in the entry basic block
   * 
   * Finally, update statistics
   */ 
  if (AllocaOutsideEntry)
  {
    InjectionLocations[&I] = 
      new GuardInfo(
          &I,
          NonCanonical, // TODO: change to the stack pointer location during the function call
          true, /* IsWrite */
          CARATNamesToMethods[CARAT_STACK_GUARD],
          "protect", /* Metadata type */
          "non.opt.call.guard" /* Metadata attached to injection */
          );

    nonOptimizedGuard++;
  }
  else
  {
    if (!InjectedCallGuardAtFirst) {

      InjectionLocations[&I] = 
        new GuardInfo(
            First,
            NonCanonical, // TODO: change to the stack pointer location during the function call
            true, /* IsWrite */ 
            CARATNamesToMethods[CARAT_STACK_GUARD],
            "protect", /* Metadata type */
            "opt.call.guard" /* Metadata attached to injection */
            );

      InjectedCallGuardAtFirst |= true;

    }

    callGuardOpt++;
  }


  /*
   * Mark the callee as handled
   */ 
  InstrumentedFunctions[Callee] = true;


  return;
}


void ProtectionsInjector::visitStoreInst(StoreInst &I)
{
  bool IsWrite = true;
  _invokeLambda(&I, IsWrite);
  return;
}


void ProtectionsInjector::visitLoadInst(LoadInst &I)
{
  bool IsWrite = false;
  _invokeLambda(&I, IsWrite);
  return;
}


/*
 * ---------- Private methods ----------
 */
void ProtectionsInjector::_findInjectionLocations(void)
{
  /*
   * Invoke the visitors to fill out the InjectionLocations map
   */ 
  this->visit(F);


  /*
   * Debugging
   */ 
  _printGuards();


  return;
}


std::vector<Value *> ProtectionsInjector::_buildStackGuardArgs(GuardInfo *GI)
{
  /*
   * TOP --- Build the function arguments for the call injection
   * for the method "nk_carat_guard_callee_stack"
   */

  /*
   * Calculate the stack frame size to check --- set as a "uint64_t"
   *
   * TODO --- Actually calculate this, set to 512 for now
   */
  const uint64_t StackFrameSize = 512;
  llvm::IRBuilder<> Builder{F->getContext()};
  std::vector<Value *> CallArgs = {
    Builder.getInt64(StackFrameSize)
  };


  return CallArgs;
}


std::vector<Value *> ProtectionsInjector::_buildGenericProtectionArgs(GuardInfo *GI)
{
  /*
   * TOP --- Build the function arguments for the call injection
   * for the method "nk_carat_guard_address"
   */

  /*
   * Set up builder
   */

  llvm::IRBuilder<> Builder = 
    Utils::GetBuilder(
        GI->InjectionLocation->getFunction(),
        GI->InjectionLocation
        );


  /*
   * Cast @GI->PointerToGuard as a void pointer for instrumentation
   */
  Value *VoidPointerToGuard = 
    Builder.CreatePointerCast(
        GI->PointerToGuard,
        Builder.getInt8PtrTy()
        );

  errs () << "THE INJECTION: " << *VoidPointerToGuard << "\n";


  /*
   * Build the call args
   */
  std::vector<Value *> CallArgs = {
    VoidPointerToGuard,
    Builder.getInt32(GI->IsWrite)
  };


  return CallArgs;
}


void ProtectionsInjector::_doTheInject(void)
{
  /*
   * Do the inject
   */ 
  for (auto const &[InstToGuard, GI] : InjectionLocations) 
  {
    /*
     * Set up builder
     */
    llvm::IRBuilder<> Builder = 
      Utils::GetBuilder(
          GI->InjectionLocation->getFunction(),
          GI->InjectionLocation
          );


    /*
     * Set up arguments based on the "GI->FunctionToInject" field
     */ 
    Function *FTI = GI->FunctionToInject;
    std::vector<Value *> CallArgs = 
      (FTI == CARATNamesToMethods[CARAT_STACK_GUARD]) ?
      (_buildStackGuardArgs(GI)) :
      (_buildGenericProtectionArgs(GI));


    /*
     * Inject the call instruction(s) based on the selected
     * args and set specialized metadata for each call
     */
    for (auto N = 0 ; N < GI->NumInjections ; N++)
    {
      CallInst *Instrumentation = 
        Builder.CreateCall(
            FTI, 
            ArrayRef<Value *>(CallArgs)
            );  

      Utils::SetInstrumentationMetadata(
          Instrumentation,
          GI->MDTypeString,
          GI->MDLiteral
          );
    }

  }


  return;
}


bool ProtectionsInjector::_optimizeForLoopInvariance(
    LoopDependenceInfo *NestedLoop,
    Instruction *I, 
    Value *PointerOfMemoryInstruction, 
    bool IsWrite
    )
{
  /*
   * Debugging
   */
  errs() << "_optimizeForLoopInvariance\n";
  errs() << "\t" << *PointerOfMemoryInstruction << "\n";


  /*
   * If @NestedLoop is not valid, we cannot optimize for loop invariance
   */
  if (!NestedLoop) { 
    errs() << "\tliCondition 0: NestedLoop not valid!\n";
    return false;
  }


  /*
   * Fetch @PointerOfMemoryInstruction as an instruction
   */
  Instruction *PointerAsInst = dyn_cast<Instruction>(PointerOfMemoryInstruction);
  // Instruction *JustInCasePointerAsInst = nullptr;
  // Value *JustInCasePointer = nullptr;
  // if (BitCastInst *BC = dyn_cast<BitCastInst>(PointerAsInst)) {
  //     Value *TheOperand = BC->getOperand(0);
  //     JustInCasePointer = TheOperand;
  //     if (auto TheOperandAsInst = dyn_cast<Instruction>(TheOperand)) {
  //         JustInCasePointerAsInst = TheOperandAsInst;
  //     }
  // }


  /*
   * If @PointerOfMemoryInstruction is an argument of @this->F, it's 
   * able to be hoisted to the outermost loop of the loop nest
   */
  if (false
      || isa<Argument>(PointerOfMemoryInstruction)
      // || (JustInCasePointer && isa<Argument>(JustInCasePointer))
      || !(noelle->getInnermostLoopThatContains(*AllLoops, PointerAsInst))) 
    // || !(noelle->getInnermostLoopThatContains(*AllLoops, JustInCasePointerAsInst)))
  {
    /*
     * For this instance, the injection location will be the preheader of the
     * outermost loop of the loop nest to which @I belongs. Fetch this basic block
     * using the StayConnectedNestedLoopForest
     */
    LoopStructure *NestedLoopStructure = NestedLoop->getLoopStructure();
    StayConnectedNestedLoopForestNode *Iterator = LoopForestOfFunction->getNode(NestedLoopStructure);
    StayConnectedNestedLoopForestNode *PrevIterator = nullptr;
    while (Iterator) {
      PrevIterator = Iterator;
      Iterator = Iterator->getParent();
    }

    assert(PrevIterator != nullptr); /* Sanity check */

    LoopStructure *OutermostLS = PrevIterator->getLoop();
    BasicBlock *PreHeader = OutermostLS->getPreHeader();
    Instruction *InjectionLocation = PreHeader->getTerminator();


    /*
     * Set up the guard
     */
    errs() << "\tHoisted with invariants using arg/li optimization!\n";
    InjectionLocations[I] = 
      new GuardInfo(
          InjectionLocation,
          PointerOfMemoryInstruction, 
          IsWrite,
          CARATNamesToMethods[CARAT_PROTECT],
          "protect", /* Metadata type */
          "loop.ivt.guard" /* Metadata attached to injection */
          );

    loopInvariantGuard++;

    return true;

  } else {
    errs() << "\tCannot use arg/li optimization\n";
    errs() << "\t\tisa<Argument>(PointerOfMemoryInstruction): " << isa<Argument>(PointerOfMemoryInstruction) << "\n";
    errs() << "\t\t!(noelle->getInnermostLoopThatContains(*AllLoops, PointerAsInst)): " << !(noelle->getInnermostLoopThatContains(*AllLoops, PointerAsInst)) << "\n";
  }


  /*
   * Setup --- fetch the loop structure for @NestedLoop
   */
  bool Hoistable = false;
  LoopDependenceInfo *NextLoop = NestedLoop;
  LoopStructure *NextLoopStructure = NextLoop->getLoopStructure();
  Instruction *InjectionLocation = nullptr;


  /*
   * Walk up the loop nest until @PointerOfMemoryInstruction to determine 
   * the outermost loop of which PointerOfMemoryInstruction is a loop 
   * invariant. 
   */
  while (NextLoop) 
  {
    errs() << "\t\tThe loop: " << "\n"; 
    NextLoopStructure->print(errs());

    /*
     * If @PointerOfMemoryInstruction is defined within the 
     * next loop, it can't be hoisted out of the loop without
     * a proper LICM --- FIX. This is the farthest we can hoist.
     * 
     * HACK, TODO
     */
    bool IsInLoop = false;
    if (PointerAsInst) {
      errs() << *(PointerAsInst->getParent()) << "\n";
      if (NextLoopStructure->isIncluded(PointerAsInst)) {
        IsInLoop = true;
      }
    }



    /*
     * Fetch the invariant manager of the next loop 
     */
    InvariantManager *Manager = NextLoop->getInvariantManager();


    /*
     * If @PointerOfMemoryInstruction is not a loop invariant
     * of this loop, we cannot iterate anymore --- break and 
     * determine if the guard can actually be hoisted
     * 
     * Essentially, we continue if we can hoist
     */ 
    if (false
        || !(Manager->isLoopInvariant(PointerOfMemoryInstruction))
        || (IsInLoop)) {
      errs() << "\t\tPointerOfMemoryInstruction not a loop invariant of NextLoop!\n";
      errs() << "\t\t\t!(Manager->isLoopInvariant(PointerOfMemoryInstruction): " 
        << std::to_string(!(Manager->isLoopInvariant(PointerOfMemoryInstruction))) << "\n";
      errs() << "\t\t\tIsInLoop: " << IsInLoop << "\n";
      break;
    }


    /*
     * We know we're dealining with a loop invariant --- set the 
     * injection location to this loop's preheader terminator and 
     * continue to iterate.
     */
    BasicBlock *PreHeader = NextLoopStructure->getPreHeader();
    InjectionLocation = PreHeader->getTerminator();


    /*
     * Set state for the next iteration up the loop nest
     */
    LoopDependenceInfo *ParentLoop = BasicBlockToLoopMap[PreHeader];
    assert (ParentLoop != NextLoop);

    NextLoop = ParentLoop;
    NextLoopStructure = 
      (NextLoop) ?
      (NextLoop->getLoopStructure()) :
      (nullptr) ;

    Hoistable |= true;
  }


  /*
   * If we can truly hoist the guard --- mark the 
   * injection location, update statistics, and return
   */
  if (Hoistable)
  {
    errs() << "Hoisted with invariants!\n";
    errs() << "PointerOfMemoryInstruction again: " << *PointerOfMemoryInstruction << "\n";
    errs() << "InjectionLocation again: " << *InjectionLocation << "\n";
    errs() << "I again: " << *I << "\n";
    InjectionLocations[I] = 
      new GuardInfo(
          InjectionLocation,
          PointerOfMemoryInstruction, 
          IsWrite,
          CARATNamesToMethods[CARAT_PROTECT],
          "protect", /* Metadata type */
          "loop.ivt.guard" /* Metadata attached to injection */
          );

    loopInvariantGuard++;
  }


  return Hoistable;
}



//This function will take a SCEV and recursively go through it to determine if it is a valid scev for karat
//Valid in karat means that it is a SCEVNAry that contains only SCEVAddRecExpr, SCEVAddExpr, or SCEVMulExpr as its base SCEV types 
bool ProtectionsInjector::_isValidSCEV(const llvm::SCEV* scevPtr){
  bool kill = true;
  auto NAry = dyn_cast<SCEVNAryExpr>(scevPtr);
  auto SCEVCast = dyn_cast<SCEVCastExpr>(scevPtr);
  auto SCEVConst = dyn_cast<SCEVConstant>(scevPtr);
  auto SCEVCNC = dyn_cast<SCEVCouldNotCompute>(scevPtr);
  auto SCEVUD = dyn_cast<SCEVUDivExpr>(scevPtr);
  auto SCEVUNK = dyn_cast<SCEVUnknown>(scevPtr);
  auto SAR = dyn_cast<SCEVAddRecExpr>(scevPtr);
  auto SAE = dyn_cast<SCEVAddExpr>(scevPtr);
  auto SME = dyn_cast<SCEVMulExpr>(scevPtr);
  auto SMM = dyn_cast<SCEVMinMaxExpr>(scevPtr);

  if( false     ||
      SAR       ||
      SAE       ||
      SMM       ||
      SCEVCast  ||
      SCEVConst
    ){
    return true;
  }
  else if(NAry){
    for(auto i = 0; i < NAry->getNumOperands(); i++){
      auto NAryOp = NAry->getOperand(i);
      errs() << "Considering NAry Op: " << *NAryOp << "...";
      auto AR = dyn_cast<SCEVAddRecExpr>(NAryOp);
      auto AE = dyn_cast<SCEVAddExpr>(NAryOp);
      auto ME = dyn_cast<SCEVMulExpr>(NAryOp);
      auto CE = dyn_cast<SCEVConstant>(NAryOp);
      auto MM = dyn_cast<SCEVMinMaxExpr>(NAryOp);
      if(MM){
        errs() << "NOT Valid (MinMax)!\n";
        return false;
      }
      if(false ||
          AR   ||
          AE   ||
          ME   ||
          CE   ||
          _isValidSCEV(NAryOp)){
        errs() << "Valid!\n";
        continue;
      }
      errs() << "NOT Valid!\n";
      return false;
    }

  }
  else if(SCEVCNC){
    errs() << "Is SCEVCNC\n";
    return false;
  }
  else if(SCEVUD){
    errs() << "Is SCEVUD\n";
    return false;
  }
  else if(SCEVUNK){
    errs() << "Is SCEVUNK\n";
    auto LI = dyn_cast<LoadInst>(SCEVUNK->getValue());
    if(LI){
      errs() << "Is a load inst\n";
      auto val = LI->getPointerOperand();
      auto safe = false;
      safe |= _isASafeMemoryConstruct(val);
      auto arg = dyn_cast<Argument>(val); 
      if(arg){
        safe |= _isSafeArgument(LI, arg);
      }
      return safe;
    }
    return false;
  }
  else{
    errs() << "Is something else o_o\n";
    return false;
  }
  return true;
}


bool ProtectionsInjector::_optimizeForSCEVAnalysis(
    LoopDependenceInfo *NestedLoop,
    Instruction *I, 
    Value *PointerOfMemoryInstruction, 
    bool IsWrite
    )
{
  // return false;
  errs() << "_optimizeForSCEVAnalysis\n";
  errs() << "\tI: " << *I << "\n";
  errs() << "\tPointerOfMemoryInstruction: " << *PointerOfMemoryInstruction << "\n";


  /*
   * If @NestedLoop is not valid, we cannot optimize for loop invariance
   */
  if (!NestedLoop) { 
    errs() << "\tscevCondition 0: NestedLoop not valid!\n";
    return false; 
  }



  auto SE = FetchSELambda(F);
  LoopStructure *NextLoopStructure = nullptr;
  BasicBlock *PreHeader = nullptr;
  Instruction *InjectionLocation = nullptr;
  auto scevPtrComputationOfI = SE->getSCEV(PointerOfMemoryInstruction);
  if (scevPtrComputationOfI){
    errs() << "THE SCEV: " << *scevPtrComputationOfI << " of type " << scevPtrComputationOfI->getSCEVType() << "\n";
  }
  else{
    errs() << "THE SCEV does not exist\n";
  }
  //auto AR = dyn_cast<SCEVAddRecExpr>(scevPtrComputationOfI);
  if (!(_isValidSCEV(scevPtrComputationOfI))){
    errs() << "SCEV is not currently analyzable :(\n";
    return false;
  }

  //This might be setting us up for failure, it seems like it is getting outermost loop without testing inner loops for scalar evolution
  auto NextLoop = NestedLoop;
  while (NextLoop)
  {
    NextLoopStructure = NextLoop->getLoopStructure();
    PreHeader = NextLoopStructure->getPreHeader();
    InjectionLocation = PreHeader->getTerminator();

    LoopDependenceInfo *ParentLoop = BasicBlockToLoopMap[PreHeader];
    assert (ParentLoop != NextLoop);

    NextLoop = ParentLoop;
  }


  Value *StartAddress = PointerOfMemoryInstruction;
  while(1){
    Instruction* newStart = dyn_cast<Instruction>(StartAddress);
    //Check if we are an inst, if not we are done
    if(!newStart){
      errs() << *StartAddress << "is NOT an Instruction\n";
      break;
    }
    //If we are out of the loop, we can be done, TODO improve this once we recursively pull out of loop
    if (!(NextLoopStructure->isIncluded(newStart))) {
      // if(BasicBlockToLoopMap[newStart->getParent()] != NestedLoop){
      StartAddress = newStart;
      errs() << *StartAddress << "is outside of the loop(s)\n";
      break;
    }

    if (CastInst *BC = dyn_cast<CastInst>(newStart)) {
      errs() << "\tFound a cast, grabbing ptr\n";
      StartAddress = BC->getOperand(0); 
    }

    else if (LoadInst *BC = dyn_cast<LoadInst>(newStart)) {
      errs() << "\tFound a load, grabbing ptr\n";
      StartAddress = BC->getOperand(0);
    }
    else if(GetElementPtrInst* BC = dyn_cast<GetElementPtrInst>(newStart)){
      errs() << "\tFound a GEP, grabbing pointer\n";
      StartAddress = BC->getPointerOperand();
    }
    else{
      errs() << "Can't handle it right now... abort: " << *newStart << "\n";
      return false;
    }
    }


    InjectionLocations[I] = 
      new GuardInfo(
          InjectionLocation,
          StartAddress,
          IsWrite,
          CARATNamesToMethods[CARAT_PROTECT],
          "protect", /* Metadata type */
          "iv.scev.guard.start", /* Metadata attached to injection */
          1
          );

    return true;

#if 0

    /*
     * Fetch @PointerOfMemoryInstruction as an instruction, sanity check
     */
    Instruction *PointerOfMemoryInstructionAsInst = dyn_cast<Instruction>(PointerOfMemoryInstruction);
    if (!PointerOfMemoryInstructionAsInst) { 
      errs() << "\tscevCondition 1: PointerOfMemoryInstructionAsInst not valid!\n";
      return false; 
    }


    errs() << "\tRecursing the PointerOfMemoryInstructionAsInst\n";
    //Check iof we are dealing with a bitcast, if we are we will use that value
    if (BitCastInst *BC = dyn_cast<BitCastInst>(PointerOfMemoryInstructionAsInst)) {
      errs() << "\tFound a bitcast, grabbing ptr\n";
      Value *TheOperand = BC->getOperand(0);
      errs() << "\tptr is: "<< *TheOperand <<"\n";
      if (auto TheOperandAsInst = dyn_cast<Instruction>(TheOperand)) {
        PointerOfMemoryInstructionAsInst = TheOperandAsInst;
      }
    }



    // BinaryOperator *PointerOfMemoryInstructionAsBinOp = dyn_cast<BinaryOperator>(PointerOfMemoryInstructionAsInst);
    GetElementPtrInst *PointerOfMemoryInstructionAsGEP = dyn_cast<GetElementPtrInst>(PointerOfMemoryInstructionAsInst);
    if (true
        // && !PointerOfMemoryInstructionAsBinOp
        && !PointerOfMemoryInstructionAsGEP) {
      errs() << "\tscevCondition: PointerOfMemoryInstruction is not defined as a binary operation or GEP!\n";
      errs() << "\t\tThe instruction is instead: " << *PointerOfMemoryInstructionAsInst << "\n";
      return false;
    }


    /*
     * Compute start address and check if all other operands are invariant or SCEV recursive
     */
    InductionVariableManager *IVManager = NestedLoop->getInductionVariableManager();
    InvariantManager *InvManager = NestedLoop->getInvariantManager();

    Value *StartAddress = PointerOfMemoryInstructionAsGEP->getPointerOperand();
    //TODO: We need to change this to not just check if it is invariant to the given loop
    //      We need to recursively go out from the nest to see if it is invariant up to the outermost loop.
    //      This might mean we should make a while loop starting here until it is no longer hoistable.
    //      Perhaps we should start with the outermost loop and work in?

    auto FetchTheDamnThang = [](Value *V) -> Value * {
      Value *TheDamnThang = V;
      if (auto *Cast = dyn_cast<CastInst>(V)) {
        TheDamnThang = Cast->getOperand(0);
      }
      return TheDamnThang;
    };


    bool Hoistable = true; 
    auto SE = FetchSELambda(F);
    errs() << "\t\tThe gep: " << *PointerOfMemoryInstructionAsGEP << "\n";
    for (auto i = 0 ; i < PointerOfMemoryInstructionAsGEP->getNumOperands() ; i++) {
      Value *Operand = PointerOfMemoryInstructionAsGEP->getOperand(i);

      if (Operand == StartAddress) continue;

      if (auto *BO = dyn_cast<BinaryOperator>(Operand)) {
        OperandsToAnalyze.push_back(FetchTheDamnThang(BO->getOperand(0)));
        OperandsToAnalyze.push_back(FetchTheDamnThang(BO->getOperand(1)));
        continue;
      }

      if (isa<CastInst>(Operand)) {
        OperandsToAnalyze.push_back(FetchTheDamnThang(Operand));
        continue;
      }

      if (LoadInst *Load = dyn_cast<LoadInst>(Operand)) {
        OperandsToAnalyze.push_back(FetchTheDamnThang(Load->getPointerOperand()));
        continue;
      }

      if (auto *PHI = dyn_cast<PHINode>(Operand)) {
        for (auto j = 0 ; j < PHI->getNumIncomingValues() ; j++) {
          OperandsToAnalyze.push_back(FetchTheDamnThang(PHI->getIncomingValue(j)));
        }
        continue;  
      }

      OperandsToAnalyze.push_back(Operand);

    }


    while (NextLoop)
    {
      InvariantManager *Manager = NextLoop->getInvariantManager();

      if (!(Manager->isLoopInvariant(StartAddress))) {
        errs() << "\tscevCondition: Start address not li\n";
        break;
      }



      bool Hoistable = true; 
      errs() << "\t\tThe gep: " << *PointerOfMemoryInstructionAsGEP << "\n";


      // for (auto i = 0 ; i < PointerOfMemoryInstructionAsGEP->getNumOperands() ; i++) {
      for (auto Operand : OperandsToAnalyze) {
        // Value *Operand = PointerOfMemoryInstructionAsGEP->getOperand(i);
        // if (Operand == StartAddress) continue;

        // if (isa<CastInst>(Operand)) {
        //     errs() << "Whoops! The operand is actually a cast ... converting: " << *Operand;
        //     Operand = cast<CastInst>(Operand)->getOperand(0);
        //     errs() << " to " << *Operand << "\n";
        // }

        if (Manager->isLoopInvariant(Operand)) {
          continue; 
        }

        auto scevPtrComputation = SE->getSCEV(Operand);
        if (scevPtrComputation) errs() << "THE SCEV: " << *scevPtrComputation << "\n";
        if (auto AR = dyn_cast<SCEVAddRecExpr>(scevPtrComputation)) {
          continue;
        }

        Hoistable = false;
        errs() << "scevCondition: NOT HOISTABLE! because of " << *Operand << "\n";
        break;

      }

      if (!Hoistable) {
        break;
      }

      NextLoopStructure = NextLoop->getLoopStructure();
      PreHeader = NextLoopStructure->getPreHeader();
      InjectionLocation = PreHeader->getTerminator();

      LoopDependenceInfo *ParentLoop = BasicBlockToLoopMap[PreHeader];
      assert (ParentLoop != NextLoop);

      NextLoop = ParentLoop;
    }

    if (NextLoopStructure == nullptr) return false;

    // if (!(InvManager->isLoopInvariant(StartAddress))) {
    //     errs() << "\tscevCondition: Start address not li\n";
    //     return false;
    // }

    // bool Hoistable = true; 
    // auto SE = FetchSELambda(F);
    // errs() << "\t\tThe gep: " << *PointerOfMemoryInstructionAsGEP << "\n";
    // for (auto i = 0 ; i < PointerOfMemoryInstructionAsGEP->getNumOperands() ; i++) {

    //     Value *Operand = PointerOfMemoryInstructionAsGEP->getOperand(i);
    //     if (Operand == StartAddress) continue;

    //     if (InvManager->isLoopInvariant(Operand)) {
    //         continue; 
    //     }

    //     auto scevPtrComputation = SE->getSCEV(Operand);
    //     if (scevPtrComputation) errs() << *scevPtrComputation << "\n";
    //     if (auto AR = dyn_cast<SCEVAddRecExpr>(scevPtrComputation)) {
    //         continue;
    //     }

    //     Hoistable = false;
    //     errs() << "scevCondition: NOT HOISTABLE! because of " << *Operand << "\n";
    //     break;

    // }

    // if (!Hoistable) {
    //     return false;
    // }

    // LoopStructure *NextLoopStructure = NestedLoop->getLoopStructure();
    // BasicBlock *PreHeader = NextLoopStructure->getPreHeader();
    // Instruction *InjectionLocation = PreHeader->getTerminator();

    while(1){
      Instruction* newStart = dyn_cast<Instruction>(StartAddress);
      //Check if we are an inst, if not we are done
      if(!newStart){
        break;
      }
      //If we are out of the loop, we can be done, TODO improve this once we recursively pull out of loop
      if (!(NextLoopStructure->isIncluded(newStart))) {
        // if(BasicBlockToLoopMap[newStart->getParent()] != NestedLoop){
        StartAddress = newStart;
        break;
      }

      if (BitCastInst *BC = dyn_cast<BitCastInst>(newStart)) {
        errs() << "\tFound a bitcast, grabbing ptr\n";
        StartAddress = BC->getOperand(0); 
      }

      else if (LoadInst *BC = dyn_cast<LoadInst>(newStart)) {
        errs() << "\tFound a load, grabbing ptr\n";
        StartAddress = BC->getOperand(0);
      }
      else if(GetElementPtrInst* BC = dyn_cast<GetElementPtrInst>(newStart)){
        errs() << "\tFound a GEP, grabbing pointer\n";
        StartAddress = BC->getPointerOperand();
      }
      else{
        errs() << "Can't handle it right now... abort: " << *newStart << "\n";
        return false;
      }
      }


      InjectionLocations[I] = 
        new GuardInfo(
            InjectionLocation,
            StartAddress,
            IsWrite,
            CARATNamesToMethods[CARAT_PROTECT],
            "protect", /* Metadata type */
            "iv.scev.guard.start", /* Metadata attached to injection */
            1
            );

      return true;
#endif
    }

    bool ProtectionsInjector::_optimizeForInductionVariableAnalysis(
        LoopDependenceInfo *NestedLoop,
        Instruction *I, 
        Value *PointerOfMemoryInstruction, 
        bool IsWrite
        )
    {
      // return false;
      /*
       * Debugging
       */
      errs() << "_optimizeForInductionVariableAnalysis\n";
      errs() << "\tI: " << *I << "\n";
      errs() << "\tPointerOfMemoryInstruction: " << *PointerOfMemoryInstruction << "\n";


      /*
       * If @NestedLoop is not valid, we cannot optimize for loop invariance
       */
      if (!NestedLoop) { 
        errs() << "\tivCondition 0: NestedLoop not valid!\n";
        return false; 
      }

      for (auto EB : NestedLoop->getLoopStructure()->getLoopExitBasicBlocks()) {
        errs() << "EB: " << *EB << "\n";
      }

      for (auto LBB : NestedLoop->getLoopStructure()->getBasicBlocks()) {
        errs() << "LBB: " << *LBB << "\n";
      }

      /*
       * Fetch @PointerOfMemoryInstruction as an instruction, sanity check
       */
      Instruction *PointerOfMemoryInstructionAsInst = dyn_cast<Instruction>(PointerOfMemoryInstruction);
      if (!PointerOfMemoryInstructionAsInst) { 
        errs() << "\tivCondition 1: PointerOfMemoryInstructionAsInst not valid!\n";
        return false; 
      }


      if (BitCastInst *BC = dyn_cast<BitCastInst>(PointerOfMemoryInstructionAsInst)) {
        Value *TheOperand = BC->getOperand(0);
        if (auto TheOperandAsInst = dyn_cast<Instruction>(TheOperand)) {
          PointerOfMemoryInstructionAsInst = TheOperandAsInst;
        }
      }


      /*
       * Fetch the induction variable manager and check if @PointerOfMemoryInstruction 
       * is defined by the following pattern: [IV/Inv] [bin op] [Inv/IV] --- if not, 
       * there's no optimization we can do
       */
      BinaryOperator *PointerOfMemoryInstructionAsBinOp = dyn_cast<BinaryOperator>(PointerOfMemoryInstructionAsInst);
      GetElementPtrInst *PointerOfMemoryInstructionAsGEP = dyn_cast<GetElementPtrInst>(PointerOfMemoryInstructionAsInst);
      if (true
          && !PointerOfMemoryInstructionAsBinOp
          && !PointerOfMemoryInstructionAsGEP) {
        errs() << "\tivCondition 2: PointerOfMemoryInstruction is not defined as a binary operation or GEP!\n";
        errs() << "\t\tThe instr-uction is instead: " << *PointerOfMemoryInstructionAsInst << "\n";
        return false;
      }


      /*
       * Determine which, if any, of the operands contribute to an IV and which are invariants
       */    
      InductionVariableManager *IVManager = NestedLoop->getInductionVariableManager();
      InvariantManager *InvManager = NestedLoop->getInvariantManager();

      Value *InvariantOperand = 
        _fetchInvariantFromInstruction(
            PointerOfMemoryInstructionAsInst,
            InvManager
            );

      Instruction *IVOperand = 
        _fetchIVFromInstruction(
            PointerOfMemoryInstructionAsInst,
            IVManager
            ); /* Flaky, InvariantOperand is a Value *, IVOperand is an Instruction * */

      if (false
          || !InvariantOperand
          || !IVOperand) {

        errs() << "\tivCondition 2: PointerOfMemoryInstructionAsInst is not defined by the pattern [IV/Inv] [bin op] [Inv/IV] or GEP!\n";

        if (InvariantOperand) {
          errs() << "\t\tInvariantOperand is valid: " << *InvariantOperand << "\n";
        } else {
          errs() << "\t\tInvariantOperand is invalid\n";
        }

        if (IVOperand) {
          errs() << "\t\tIVOperand is valid: " << *IVOperand << "\n";
        } else {
          errs() << "\t\tIVOperand is invalid\n";
        }

        return false;
      }

      // assert(InvariantOperand != IVOperand); /* Can we make this guarantee --- NO, change */

#if 0
      if (!(IVManager->doesContributeToComputeAnInductionVariable(PointerOfMemoryInstructionAsInst))) {
        errs() << "\tPointerOfMemoryInstructionAsInst does not contribute to IV computation!\n";
        return false;
      } /* We want to check if PointerOfMemoryInstructionAsInst = base [op] IV 
           the op can be a binary operator or a getElementPtr
           - the two ops (at least for bops) should be a loop invariant (the base) and one that contributes to the IV (the actual IV)
           - print out how often the condition is satisfied or not (for kicks, and to see if we should handle GEP)
           */
#endif

      /*
       * At this point, we know that the computation of @PointerOfMemoryInstruction
       * depends on a bounded scalar evolution --- which means that the guard can be
       * hoisted outside the loop where the boundaries used in the check can range from
       * start to end address of the scalar evolution
       * 
       * Fetch the IV for @PointerOfMemoryInstruction, and begin using this to compute
       * this start and end address from this IV
       */
      bool Hoisted = false;
      LoopStructure *NestedLoopStructure = NestedLoop->getLoopStructure();
      InductionVariable *IV = 
        IVManager->getInductionVariable(
            *NestedLoopStructure,
            IVOperand
            // PointerOfMemoryInstructionAsInst 
            /* This actually needs to be IV as follows: POMIAI = 
               base [op] iv. we need that IV, not POMIAI 
               to fetch the Noelle IV pointer. NOTE base has to be loop invariant */
            );

      if (!IV) {
        /*
         * This is conservative, but it's possible to optimize more here based on outer loops --- FIX
         */
        errs() << "\tivCondition 3: Invalid induction variable object for the current loop and IVOperand!";
        return false;
      }


      /*
       * Check that the step value of this IV is loop invariant
       */
      if (!(IV->isStepValueLoopInvariant())) { /* Still accurate b/c we only consider basic IVs, not derived IVs.
                                                  this makes the computation easy b/c step value is constant, you can extend the computation as follows:
                                                  base + start value of IV + (total number of iterations * (constant) step) = end address. with this type of IV. */
        errs() << "\tivCondition 4: IV related to PointerOfMemoryInstructionAsInst's def does not have a loop invariant step value!\n";
        return false; 
      }


      /*
       * Now switch to analyzing the loop governing IV
       *
       * Fetch the loop governing IV attribution and check its validity
       * 
       * this is needed for the IV utility and "total number of iterations"
       */
      LoopGoverningIVAttribution *LGIVAttr = IVManager->getLoopGoverningIVAttribution(*NestedLoopStructure);
      if (!LGIVAttr) { 
        errs() << "\tivCondition 5: Loop Governing IV attribution is invalid!\n";
        return false; 
      }


      /*
       * Fetch the loop governing induction variable (LGIV) and 
       * ensure that its step value is loop invariant
       * 
       * need this (perhaps) because in order to get the total number of
       * iterations, but GIVUtility covers this
       */
      InductionVariable *LGIV = &(LGIVAttr->getInductionVariable());
#if 0
      if (!(LGIV->isStepValueLoopInvariant())) {
        errs() << "\tLoop Governing IV does not have a loop invariant step value!\n";
        return false;
      }
#endif


      /*
       * Fetch the loop governing IV utility
       */
      LoopGoverningIVUtility GIVUtility(
          NestedLoopStructure,
          *IVManager,
          *LGIVAttr
          ); /* UPDATE NOELLE ---, now takes an LS * and IVManager of the LS. */


      /*
       * Generate the code to compute the total number of iterations for the current loop invocation, 
       * and fetch the resulting loop iterations value
       */
      Instruction *PreHeaderTerminator = NestedLoopStructure->getPreHeader()->getTerminator();
      IRBuilder<> NumIterationsBuilder = 
        Utils::GetBuilder(
            PreHeaderTerminator->getFunction(),
            PreHeaderTerminator
            );

      Value *NumIterations = GIVUtility.generateCodeToComputeTheTripCount(NumIterationsBuilder);
      if (!NumIterations) {
        errs() << "\tivCondition 6: Can't generate code to compute total number of iterations!\n";
        return false;
      }
      errs() << "\tNumIterations: " << *NumIterations << "\n";


      /*
       * Fetch the start address and step value from the IV
       * related to the pointer, check its validity
       */
      Value *IVStart = IV->getStartValue(), 
            *IVStep = IV->getSingleComputedStepValue();

      if (false
          || !IVStart
          || !IVStep) {
        errs() << "\tivCondition 7: IVStart or IVStep is invalid!\n";
        if (IVStart) {
          errs() << "\t\tIVStart is valid: " << *IVStart << "\n";
        } else {
          errs() << "\t\tIVStart is invalid!\n";
        }
        if (IVStep) {
          errs() << "\t\tIVStep is valid: " << *IVStep << "\n";
        } else {
          errs() << "\t\tIVStep is invalid!\n";
        }
        return false;
      }

      errs() << "IVStart: " << *IVStart << "\n";
      errs() << "IVStep: " << *IVStep << "\n";



#if 0
      /*
       * NOTE --- DO WE NEED TO CHECK IF THE BOUNDS VALUE (THE VALUE THAT
       * IS ESSENTIALLY (LGIVAttr->getIntermediateValueUsedInCompare()) 
       * IS LOOP INVARIANT? 
       */
      Value *BoundsValue = LGIVAttr->getIntermediateValueUsedInCompare(); /* WE ACTUALLY WANT LGIVAttr->getExitConditionValue() */
      InvariantManager *InvManager = NestedLoop->getInvariantManager();
      if (!(InvManager->isLoopInvariant(BoundsValue))) {
        return false;
      }
#endif


      /*
       * Compute ONLY (Step * NumIterations) + StartAddress and inject 
       * this into the preheader of the loop directly --- HACK
       * 
       * First, set up injection locations and builders
       */

      BasicBlock *PreHeader = NestedLoopStructure->getPreHeader();
      errs() << "PREHEADER BEFORE: " << *PreHeader << "\n";

      Instruction *InjectionLocation = PreHeader->getTerminator();
      llvm::IRBuilder<> Builder = 
        Utils::GetBuilder(
            InjectionLocation->getFunction(),
            InjectionLocation
            );


      /*
       * Compute StartAddress = IVStart + @PointerOfMemoryInstruction's base (IV Operand) (ACTUAL)
       * FIX THIS DAMN CODE
       */
      Value *StartAddress = 
        Builder.CreateAdd(
            Builder.CreateMul(
              IVStart,
              Builder.getInt64(
                Utils::GetPrimitiveSizeInBytes(
                  cast<PointerType>(InvariantOperand->getType())->getElementType()
                  )
                )
              ),
            Builder.CreatePtrToInt(
              InvariantOperand,
              Builder.getInt64Ty()
              )
            );


      /*
       * Compute Offset = Step * NumIterations
       */
      Value *Offset = 
        Builder.CreateMul(
            IVStep,
            NumIterations
            );


      /*
       * Compute EndAddress = Offset + StartAddress, and
       * cast it back to a pointer type
       */
      Value *EndAddressVal = 
        Builder.CreateAdd(
            Offset,
            StartAddress
            );

      Value *EndAddress =
        Builder.CreateIntToPtr(
            EndAddressVal,
            InvariantOperand->getType()
            );

      errs() << "\tStartAddress: " << *StartAddress << "\n";
      errs() << "\tOffset: " << *Offset << "\n";
      errs() << "\tEndAddress: " << *EndAddress << "\n";

      errs() << "PREHEADER AFTER: " << *PreHeader << "\n";


      /*
       * Set up GuardInfo packages
       */
      InjectionLocations[I] = 
        new GuardInfo(
            InjectionLocation,
            StartAddress,
            IsWrite,
            CARATNamesToMethods[CARAT_PROTECT],
            "protect", /* Metadata type */
            "iv.scev.guard.start", /* Metadata attached to injection */
            1
            );

      InjectionLocations[I] = 
        new GuardInfo(
            InjectionLocation,
            EndAddress,
            IsWrite,
            CARATNamesToMethods[CARAT_PROTECT],
            "protect", /* Metadata type */
            "iv.scev.guard.end", /* Metadata attached to injection */
            1
            );


      scalarEvolutionGuard++;
      Hoisted |= true;


      return Hoisted;
    }


    Value *ProtectionsInjector::_fetchInvariantFromInstruction (
        Instruction *I,
        InvariantManager *InvManager
        )
    {
      /*
       * TOP --- Based on @I and @InvManager, find if either operands of 
       * @I is a loop invariant. Otherwise, return nullptr
       */

      /*
       * Setup
       */
      // assert(I->getNumOperands() == 2);
      if (I->getNumOperands() != 2) {
        return nullptr;
      }

      Value *InvariantOperand = nullptr;
      Value *FirstOp = I->getOperand(0);
      Value *SecondOp = I->getOperand(1);


      /*
       * Check for loop invariance
       */
      if (InvManager->isLoopInvariant(FirstOp)) {
        errs() << "\t\t_fetchInvariantFromBinaryOperator: FirstOp is loop invariant: " << *FirstOp << "\n";
        InvariantOperand = FirstOp;
      }
      else if (InvManager->isLoopInvariant(SecondOp)) {
        errs() << "\t\t_fetchInvariantFromBinaryOperator: SecondOp is loop invariant: " << *SecondOp << "\n";
        InvariantOperand = SecondOp;
      }


      return InvariantOperand;
    }


    Instruction *ProtectionsInjector::_fetchIVFromInstruction (
        Instruction *I,
        InductionVariableManager *IVManager
        )
    {
      /*
       * TOP --- Based on @I and @IVManager, find if either operands of 
       * @I contributes to an induction variable. Otherwise, return nullptr
       */

      /*
       * Setup
       */
      // assert(I->getNumOperands() == 2);
      if (I->getNumOperands() != 2) {
        return nullptr;
      }

      Instruction *IVOperand = nullptr;
      Instruction *FirstOpAsInst = dyn_cast<Instruction>(I->getOperand(0));
      Instruction *SecondOpAsInst = dyn_cast<Instruction>(I->getOperand(1));


      /*
       * Check for IV contribution
       */
      if (true
          && FirstOpAsInst
          && IVManager->doesContributeToComputeAnInductionVariable(FirstOpAsInst)) {
        errs() << "\t\t_fetchIVFromBinaryOperator: FirstOp contributes to an IV: " << *FirstOpAsInst << "\n";
        IVOperand = FirstOpAsInst;
      }
      else if (
          true
          && SecondOpAsInst
          && IVManager->doesContributeToComputeAnInductionVariable(SecondOpAsInst)) {
        errs() << "\t\t_fetchIVFromBinaryOperator: SecondOp contributes to an IV: " << *SecondOpAsInst << "\n";
        IVOperand = SecondOpAsInst;
      }


      return IVOperand;
    }




    bool ProtectionsInjector::_isAPointerReturnedByAllocator(Value *V)
    {
      /*
       * Fetch @V as a call instruction
       */
      CallInst *Call = dyn_cast<CallInst>(V);
      if (!Call) return false;


      /*
       * Vet the callee
       */ 
      Function *Callee = Call->getCalledFunction();
      if (!Callee) return false;


      /*
       * Fetch the right memory allocator map
       */ 
      std::unordered_map<Function *, AllocID> MapToUse = 
        (InstrumentingUserCode) ?
        (UserAllocMethodsToIDs) :
        (KernelAllocMethodsToIDs) ;


      /*
       * Check if the callee is one of the recognized
       * library allocator functions
       */
      if (MapToUse.find(Callee) == MapToUse.end()) return false;


      /*
       * We can prove that @V is from a memory allocator!
       */
      return true;
    }


    Value *ProtectionsInjector::_fetchBitCastOperand(Value *Pointer)
    {
      /*
       * If @Pointer is actually a bitcast operation, fetch its 
       * operand to analyze. If @Pointer isn't a bitcast, return
       * a nullptr
       */ 

      /*
       * Attempt to fetch the operand as a bitcast instruction
       */ 
      BitCastInst *BCI = dyn_cast<BitCastInst>(Pointer);
      if (BCI) return BCI->getOperand(0);


      /*
       * Attempt to fetch the operand as a bitcast operator
       */ 
      BitCastOperator *BCO = dyn_cast<BitCastOperator>(Pointer);
      if (BCO) return BCO->getOperand(0);


      return nullptr;
    }


    Value *ProtectionsInjector::_fetchGEPBasePointer(
        Value *Pointer,
        bool CheckInBounds
        )
    {
      /*
       * TOP --- If @Pointer is a GEP, fetch the pointer operand,
       * which represents the base pointer of the aggregate type
       * that said GEP would be indexing into
       * 
       * Use @CheckInBounds to understand whether or not the GEP
       * is safe! It must be an "inbounds" index, otherwise, send
       * back a nullptr
       */
      GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(Pointer);

      Value *BasePointer =
        (GEP) ?
        (GEP->getPointerOperand()) :
        (nullptr);

      if (true
          && CheckInBounds
          && GEP
          && (!(GEP->isInBounds()))) {
        if(!(GEP->hasAllConstantIndices())){
          BasePointer = nullptr;
        }
      }


      return BasePointer;
    }


    bool ProtectionsInjector::_isASafeMemoryConstruct(Value *Pointer)
    {
      /*
       * TOP --- See steps 1b-d in this->_findPointToInsertGuard
       * 
       * There is an added tidbit here --- it's possible that the 
       * pointer we're analyzing is really an offset into an aggregate
       * data type --- typically represented in the IR as a GEP. Check
       * this level of indirection. We also handle the same level of 
       * indirection for bitcast instructions, which are designed to 
       * maintain type-safety in LLVM IR.
       */
      if (!Pointer) return false;

      if (false
          || isa<AllocaInst>(Pointer)
          || isa<GlobalVariable>(Pointer)
          || _isAPointerReturnedByAllocator(Pointer)
          || _isASafeMemoryConstruct(_fetchBitCastOperand(Pointer))
          || _isASafeMemoryConstruct(_fetchGEPBasePointer(Pointer, true /* Check inbounds*/))) {
        return true;
      } 


      return false;
    }

    //This function will attempt to walk a value up the chain of its function if it is being casted, GEP'ed, or loaded
    //Returns value* to unCastGepLoad value if success, nullptr if failed
    Value* ProtectionsInjector::unCastGepLoad(Value* recurseVal){
      auto LI = dyn_cast<LoadInst>(recurseVal);
      auto GEPI = dyn_cast<GetElementPtrInst>(recurseVal);
      auto CI = dyn_cast<CastInst>(recurseVal);
      auto ARG = dyn_cast<Argument>(recurseVal);
      if(LI){
        return LI->getPointerOperand();
      }
      else if(GEPI){
        return _fetchGEPBasePointer(recurseVal, 0);
      }
      else if(CI){
        return _fetchBitCastOperand(recurseVal);
      }
      return nullptr;
    }



    //This function correspondes to 1i and will recursively find out if an argument is safe to not protect
    bool ProtectionsInjector::_isSafeArgument(Instruction* inst, Argument* arg){
      errs() << "isSafeArgument called on inst in function: "<< inst->getFunction()->getName() << "\n";
      //Grab the parent function the instruction exists in
      auto parFunction = inst->getFunction();
      //Get arg num
      auto argumentNum = arg->getArgNo();

      auto FM = noelle->getFunctionsManager();
      auto CG = FM->getProgramCallGraph();
      //_isASafeMemoryConstruct();
      if(CG->doesItBelongToASCC(parFunction)){
        errs() << "This function is in an SCC, abort!\n";
        return false;
      }
      else{
        auto parFunctionNode = CG->getFunctionNode(parFunction);

        auto incomingEdges = parFunctionNode->getIncomingEdges();
        bool areAllCallersSafe = true;
        for(auto& edge : incomingEdges){
          auto incomingSubEdges = edge->getSubEdges();
          for(auto subEdge : incomingSubEdges){
            auto callInst = subEdge->getCaller()->getInstruction();
            errs() << "Investigating callInst: " << *callInst << " of parent function: " << callInst->getFunction()->getName() << "\n";
            auto callInstArg = callInst->getOperand(argumentNum);
            auto tempVal = callInstArg;
            //Walk up the cast, gep, load, arg chain
            bool safeChain = false;
            while(1){
              //breaks if safe construct 
              if(_isASafeMemoryConstruct(tempVal)){
                safeChain = true;
                break;
              } 
              auto tempVal2 = unCastGepLoad(tempVal);
              //Breaks when tempVal cannot walk up chain
              if(!tempVal2){
                break;
              }
              tempVal = tempVal2;
            }
            if(safeChain){
              continue;
            } 
            //Before we give up, let us try to cast it as an argument and recurse this function.
            if(auto tryArgCast = dyn_cast<Argument>(tempVal)){
              if(_isSafeArgument(callInst, tryArgCast)){
                continue;
              }
            }
            errs() << "Not safe!\n";
            areAllCallersSafe = false;
            break;
          }


          if(!areAllCallersSafe){
            break;
          }
        }
        if(areAllCallersSafe){
          errs() << "All callers safe!\n";
          return true;
        }
        //If we make it here, then the function does not have all callers use the argument safely
        errs() << "All callers not safe!\n";
        return false;
      }

    }


    std::function<void (Instruction *inst, Value *pointerOfMemoryInstruction, bool isWrite)> ProtectionsInjector::_findPointToInsertGuard(void) 
    {
      /*
       * Define the lamda that will be executed to identify where to place the guards.
       */
      auto FindPointToInsertGuardFunc = 
        [this](Instruction *inst, Value *PointerOfMemoryInstruction, bool isWrite) -> void {

          /*
           * The scoop:
           * 
           * - @inst will be some kind of memory instruction (load/store, potentially
           *   call instruction)
           * - @PointerOfMemoryInstruction will be the pointer operand of said
           *   memory instruction (@inst)
           * - @isWrite denotes the characteristic of @inst (i.e. load=FALSE, 
           *   store=TRUE, etc.)
           * 
           * Several steps to check/perform (NOTE --- some of this is ad-hoc b/c deadline):
           * 
           * 1) If @PointerOfMemoryInstruction has already been guarded:
           *    a) @PointerOfMemoryInstruction is in the IN set of @inst, indicating
           *       that the DFA has determined that the pointer need not be checked
           *       when considering guarding at @inst
           *    b) @PointerOfMemoryInstruction is an alloca i.e. we know all origins
           *       of allocas since they're on the stack, and there's a region for it.
           *    c) @PointerOfMemoryInstruction originates from a library allocator
           *       call instruction --- these pointers are the ones being tracked and
           *       also assumed to be safe b/c we trust the allocator --- there's a 
           *       region dedicated for this, it's the (at least initial) heap
           *    d) @PointerOfMemoryInstruction originates from a global variable, 
           *       we can assume a safe memory reference because globals lie in 
           *       known and designated region --- the blob.
           *    e) @PointerOfMemoryInstruction originates from an inbounds indexed 
           *       location into a SAFE memory construct (i.e. described in steps 
           *       1b.-1d.) (i.e. analyzing a GEP into an known alloca, global, etc.).
           *    f) @PointerOfMemoryInstruction originates from an inbounds indexed 
           *       location into SOME memory construct which the DFA has determined
           *       need not be checked.
           *    g) @PointerOfMemoryInstruction originates from a bitcast instruction,
           *       which is designed just to maintain type safety and does not affect
           *       the value of the pointer.
           *    h) @PointerOfMemoryInstruction must aliases any of the variable or 
           *       data types described in 1b-d.
           *    i) @PointerOfMemoryInstruction is an argument of the function,
           *       all the callers of the function provide a safe memory construct as the argument
           *    j) Follow the cast, load GEP path of instruction to check for safe construct
           *
           *    ... then we're done --- don't have to do anything!
           * 
           * 2) Otherwise, we can check if @inst is part of a loop nest. If that's the
           *    case, we can try to perform one of two optimizations:
           *    a) If @inst is a loop invariant, we can use NOELLE to understand how 
           *       far up the loop nest we can hoist the guard for @inst. Said guard will be 
           *       injected in the determined loop's preheader (b/c that's how hoisting works) 
           *       and guard the pointer directly.
           *    b) If this isn't possible (i.e. @inst isn't a loop invariant), then we can
           *       also determine if @inst contributes to an induction variable, which is
           *       going to be based on a scalar evolution expression. If NOELLE determines
           *       that @inst fits this characterization, then we guard the start address
           *       through the end address (**do we need to guard the end addr, or can we 
           *       guard start + an offset?**), and hoist this guard @inst's parent loop 
           *       (i.e to the preheader).
           *   
           * 3) If @inst wasn't a part of a loop nest or if the optimizations attempted
           *    did not work, then the guard must be placed right before @inst.
           */


          /*
           * <Step 1a.>
           */
          auto &INSetOfI = DFR->IN(inst);
          if (INSetOfI.find(PointerOfMemoryInstruction) != INSetOfI.end()) 
          {
            redundantGuard++;                  
            return;
          }


          /*
           * <Step 1b.-e.>
           */
          errs() << "_findPointToInsertGuard:\n";
          errs() << "\tinst: " << *inst  << "\n";
          errs() << "\tptr: " << *PointerOfMemoryInstruction << "\n";
          if (_isASafeMemoryConstruct(PointerOfMemoryInstruction))
          {
            errs() << "\t\tisASafeMemoryConstruct!\n";
            redundantGuard++;                  
            return;
          }


          /*
           * <Step 1f.>
           */
          Value *PotentialBasePointer = 
            _fetchGEPBasePointer(
                PointerOfMemoryInstruction,
                true /* Check inbounds */
                );

          if (true
              && PotentialBasePointer
              && INSetOfI.find(PotentialBasePointer) != INSetOfI.end()) 
          {
            redundantGuard++;                  
            return;
          }


          /*
           * <Step 1g.>
           */
          Value *PointerCastOperand = _fetchBitCastOperand(PointerOfMemoryInstruction);
          if (true
              && PointerCastOperand
              && INSetOfI.find(PointerCastOperand) != INSetOfI.end()) 
          {
            redundantGuard++;                  
            return;
          }


          /*
           * <Step 1h.>
           */
          bool MustAliasesSafeConstructs = false; 
          auto Iterator = 
            [this, inst, &MustAliasesSafeConstructs]
            (Value *depValue, DGEdge<Value> *dep) -> bool {

              errs() << "\t\tdep: " << *depValue << "\n";

              /*
               * If @dep is not a must dependence, return
               */
              if (!(dep->isMustDependence())) {
                return false;
              }

              errs() << "\t\t\tisMustDependence!\n";

              /*
               * <Conditions 1b-1d>.
               */
              if (_isASafeMemoryConstruct(depValue)) {
                errs() << "\t\t\tMustAliasesSafeConstructs!\n";
                MustAliasesSafeConstructs |= true;
              }


              return false;            

            };


          /*
           * Iterate over the dependences
           */
          auto Iterated = 
            FDG->iterateOverDependencesTo(
                PointerOfMemoryInstruction, 
                false, /* Control dependences */
                true, /* Memory dependences */
                true, /* Register dependences */
                Iterator
                );


          /*
           * If the iterator has identified a redudant guard, return
           */
          if (MustAliasesSafeConstructs) 
          {
            redundantGuard++;
            return;
          }


          /*
           * <Step 1i.>
           */
          Value* BitcastValue = _fetchBitCastOperand(PointerOfMemoryInstruction); 
          if(BitcastValue == nullptr){
            BitcastValue = PointerOfMemoryInstruction;
          }
          if(auto arg = dyn_cast<Argument>(BitcastValue)){
            if(_isSafeArgument(inst, arg)){
              errs() << "SUCCESS: Memory location is a safe argument, skipping...\n";
              redundantGuard++;
              return;
            } 
          }

          /*
           *  <Step 1j.>
           */

          //Keep trying to cast the inst as a cast, gep, or load and check if they are safe constructs
          Value* recurseVal = PointerOfMemoryInstruction;
          while(1){
            if(!recurseVal){
              break;
            }
            errs() << "1j, Checking: " << *recurseVal << " of FUNCTION: " << inst->getFunction()->getName() <<"\n";

            if (INSetOfI.find(recurseVal) != INSetOfI.end()) 
            {
              redundantGuard++;                  
              return;
            }
            auto LI = dyn_cast<LoadInst>(recurseVal);
            auto GEPI = dyn_cast<GetElementPtrInst>(recurseVal);
            auto CI = dyn_cast<CastInst>(recurseVal);
            auto ARG = dyn_cast<Argument>(recurseVal);
            if(ARG){
              if(_isSafeArgument(inst, ARG)){
                redundantGuard++;
                return;
              }
            }
            if(LI){
              recurseVal = LI->getPointerOperand();
              if(_isASafeMemoryConstruct(recurseVal)){
                redundantGuard++;
                return;
              }
              continue;
            }
            else if(GEPI){
              recurseVal = _fetchGEPBasePointer(recurseVal, 0);
              if(_isASafeMemoryConstruct(recurseVal)){
                redundantGuard++;
                return;
              }
              continue;
            }
            else if(CI){
              recurseVal = _fetchBitCastOperand(recurseVal);
              if(_isASafeMemoryConstruct(recurseVal)){
                redundantGuard++;
                return;
              }
              continue;
            }
            break;
          }


          /*
           * We have to guard the pointer --- fetch the 
           * potential loop nest that @inst belongs to
           */
          bool Guarded = false;
          LoopDependenceInfo *NestedLoop = BasicBlockToLoopMap[inst->getParent()];
          if (!NestedLoop) { 
            goto No_Loop;
          }
          errs() << "Trying loop optimization ...\n";


          /*
           * <Step 2a.>
           */
/* LOOP INVARIANCE BEGIN
          Guarded |= 
            _optimizeForLoopInvariance(
                NestedLoop,
                inst,
                PointerOfMemoryInstruction,
                isWrite
                );

          if (Guarded) {
            errs() << "Success LI! " << F->getName() << "\n";
          }
LOOP INVARIANCE END*/
          /*
           * <Step 2b.>
           */
          if (!Guarded)
          {
            Guarded |= 
              // _optimizeForInductionVariableAnalysis(
              _optimizeForSCEVAnalysis(                
                  NestedLoop,
                  inst,
                  PointerOfMemoryInstruction,
                  isWrite
                  );

            if (Guarded) {
              errs() << "Success SCEV! " << F->getName() << "\n";
            }
            else{
              errs() << "Failed SCEV! "  <<  F->getName()  <<  "\n";
            }
          }

No_Loop:

          /*
           * <Step 3>
           */
          if (!Guarded) 
          {
            errs() << "Step 3 Guard: "<< *inst << "::" << *PointerOfMemoryInstruction << "\n";
            InjectionLocations[inst] = 
              new GuardInfo(
                  inst,
                  PointerOfMemoryInstruction, 
                  isWrite,
                  CARATNamesToMethods[CARAT_PROTECT],
                  "protect", /* Metadata type */
                  "non.opt.mem.guard" /* Metadata attached to injection */
                  );
            nonOptimizedGuard++;
          }


          return;
        };


      return FindPointToInsertGuardFunc;
    }


    void ProtectionsInjector::_allocaOutsideFirstBBChecker(void) 
    {
      /*
       * Check if there is no stack allocations other than 
       * those in the first basic block of the function.
       */
      BasicBlock *FirstBB = &*(F->begin());
      for (auto &B : *F)
      {
        for (auto &I : B)
        {
          if (true
              && isa<AllocaInst>(&I)
              && (&B != FirstBB))
          {
            /*
             * We found a stack allocation not in the entry basic block.
             */
            AllocaOutsideEntry |= true;
            break;
          }
        }
      }


      return;
    }


    template<typename MemInstTy>
      void ProtectionsInjector::_invokeLambda(
          MemInstTy *I,
          bool IsWrite
          )
      {
        /*
         * Fetch the lambda to invoke --- FIX
         */ 
        auto TheLambda = _findPointToInsertGuard();


        /*
         * Fetch the pointer to handle from @I
         */
        Value *PointerToHandle = I->getPointerOperand();


        /*
         * Invoke the lambda
         */
        TheLambda(I, PointerToHandle, IsWrite);


        return;
      }


    void ProtectionsInjector::_printGuards(void) 
    {
      /*
       * Print where to put the guards
       */
      errs() << "GUARDS for " << F->getName() << "\n";
      for (auto &Guard : InjectionLocations)
        errs() << " " << *(Guard.first) << "\n";


      /*
       * Print guard statistics
       */
      errs() << "GUARDS: Guard Information\n";
      errs() << "GUARDS: Unoptimized Guards:\t" << nonOptimizedGuard << "\n"; 
      errs() << "GUARDS: Redundant Optimized Guards:\t" << redundantGuard << "\n"; 
      errs() << "GUARDS: Loop Invariant Hoisted Guards:\t" << loopInvariantGuard << "\n"; 
      errs() << "GUARDS: Scalar Evolution Combined Guards:\t" << scalarEvolutionGuard << "\n"; 
      errs() << "GUARDS: Hoisted Call Guards\t" << callGuardOpt << "\n"; 
      errs() << "GUARDS: Total Guards:\t" << nonOptimizedGuard + loopInvariantGuard + scalarEvolutionGuard << "\n"; 


      return;
    }

#endif
