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

#include "../include/Utils.hpp"

using namespace Utils;

/*
 * ExitOnInit
 * 
 * Register pass, execute doInitialization method but do not perform
 * any analysis or transformation --- exit in runOnModule --- mostly
 * for testing scenarios
 */
void Utils::ExitOnInit(void)
{
    if (InitExit)
    {
        errs() << "Exiting KARAT Transforms ...\n";
        exit(0);
    }

    return;
}

Function *Utils::GetMethod(
    Module *M,
    const std::string Name
)
{
    /*
     * Fetch function with @Name from @M --- sanity
     * check that the function exists
     */ 
    Function *F = M->getFunction(Name);
    errs() << "Fetching " << Name << " ... \n";
    assert(!!F && "Utils::GetMethod: Can't fetch!");
    return F;
}



/*
 * GetBuilder
 * 
 * Generates a specific IRBuilder instance that is fitted with 
 * the correct debug location --- necessary for injections 
 * into the Nautilus bitcode
 */
IRBuilder<> Utils::GetBuilder(
    Function *F, 
    Instruction *InsertionPoint
)
{
    IRBuilder<> Builder{InsertionPoint};
    Instruction *FirstInstWithDBG = nullptr;

    for (auto &I : instructions(F))
    {
        if (I.getDebugLoc())
        {
            FirstInstWithDBG = &I;
            break;
        }
    }

    if (FirstInstWithDBG) Builder.SetCurrentDebugLocation(FirstInstWithDBG->getDebugLoc());

    return Builder;
}


IRBuilder<> Utils::GetBuilder(
    Function *F, 
    BasicBlock *InsertionPoint
)
{
    IRBuilder<> Builder{InsertionPoint};
    Instruction *FirstInstWithDBG = nullptr;

    for (auto &I : instructions(F))
    {
        if (I.getDebugLoc())
        {
            FirstInstWithDBG = &I;
            break;
        }
    }

    if (FirstInstWithDBG) Builder.SetCurrentDebugLocation(FirstInstWithDBG->getDebugLoc());

    return Builder;
}


void Utils::InjectStats(Module &M)
{
    /*
     * Fetch the main method
     */
    Function *Main = Utils::GetMethod(&M, "main");
    assert(
        true
        && !!Main
        && "InjectStats: Can't fetch main method!"
    );


    /*
     * Find the injection location(s) in main by finding
     * the return instructions in "main"
     */
    std::set<Instruction *> InsertionPoints;
    for (auto &B : *Main)
        if (isa<ReturnInst>(B.getTerminator()))
            InsertionPoints.insert(B.getTerminator());


    /*
     * Inject a call to "results()" at each insertion point
     */
    Function *StatsFunction = CARATNamesToMethods[USER_STATS];
    for (auto InsertionPoint : InsertionPoints)
    {
        IRBuilder<> Builder = 
            Utils::GetBuilder(
                StatsFunction,
                InsertionPoint
            );

        Builder.CreateCall(
            StatsFunction->getFunctionType(),
            StatsFunction,
            ArrayRef<Value *>{}
        );
    }


    return;
}


bool Utils::IsInstrumentable(Function &F)
{
    /*
     * Skip instrumention in functions upholding any of the 
     * following conditions:
     * 1) A memory allocator method (kernel or user)
     * 2) Any CARAT method (CARATMethods)
     * 3) Annotated (AnnotatedFunctions)
     * 3) LLVM intrinsics
     * 4) Other empty functions (pure assembly stubs, etc.)
     */ 

    /*
     * Fetch the right memory allocator map
     */ 
    std::unordered_map<Function *, AllocID> MapToUse = 
        (InstrumentingUserCode) ?
        (UserAllocMethodsToIDs) :
        (KernelAllocMethodsToIDs) ;


    /*
     * Perform check
     */
    if (false
        || (MapToUse.find(&F) != MapToUse.end()) 
        || (CARATMethods.find(&F) != CARATMethods.end()) 
        || (AnnotatedFunctions.find(&F) != AnnotatedFunctions.end()) 
        || (F.isIntrinsic()) 
        || (F.empty()))
        return false;


    return true;
}


Instruction *Utils::GetPostTargetInsertionPoint(Instruction *Target)
{
    /*
     * TOP --- Find an insertion point to instrument @Target *after*
     * @Target is defined. Ignore instructions that could be prior
     * instrumentation done by the pass
     */
    
    /*
     * Set up next node insertion point
     */
    Instruction *InsertionPoint = Target->getNextNode();

    
    /*
     * Continue iterating forward until the insertion point
     * is not a prior instrumentation method. NOTE --- We 
     * do not want to cross boundaries for protections.
     */
    while (Utils::HasBaseInstrumentationMetadata(InsertionPoint))
        InsertionPoint = InsertionPoint->getNextNode();


    return InsertionPoint;
}


void Utils::FetchAnnotatedFunctions(GlobalVariable *GV)
{
    /*
     * TOP --- Parse the global annotations array from @GV and 
     * stash all functions that are annotated as "nocarat"
     */

    /*
     * Fetch the global annotations array 
     */ 
    auto *AnnotatedArr = cast<ConstantArray>(GV->getOperand(0));


    /*
     * Iterate through each annotation in the 
     */ 
    for (auto OP = AnnotatedArr->operands().begin(); 
         OP != AnnotatedArr->operands().end(); 
         OP++)
    {
        /*
         * Each element in the annotations array is a 
         * ConstantStruct --- its fields can be accessed
         * through the first operand. There are two fields
         * (Function *, GlobalVariable * (annotation))
         */ 
        auto *AnnotatedStruct = cast<ConstantStruct>(OP);
        auto *FunctionAsStructOp = AnnotatedStruct->getOperand(0)->getOperand(0);         /* First field */
        auto *GlobalAnnotationAsStructOp = AnnotatedStruct->getOperand(1)->getOperand(0); /* Second field */

        /*
         * Fetch the function and the annotation global --- sanity check
         */ 
        Function *AnnotatedFunction = dyn_cast<Function>(FunctionAsStructOp);
        GlobalVariable *AnnotatedGV = dyn_cast<GlobalVariable>(GlobalAnnotationAsStructOp);
        if (!AnnotatedFunction || !AnnotatedGV) continue;


        /*
         * Check the annotation --- if it matches "nocarat",
         * then stash the annotated function
         */
        ConstantDataArray *ConstStrArr = dyn_cast<ConstantDataArray>(AnnotatedGV->getOperand(0));
        if (false
            || !ConstStrArr
            || (ConstStrArr->getAsCString() != NOCARAT)) continue;

        AnnotatedFunctions.insert(AnnotatedFunction);
    }


    /*
     * Debugging
     */ 
    for (auto F : AnnotatedFunctions) errs() << "Annotated: " + F->getName() << "\n";


    return;
}


uint64_t Utils::GetPrimitiveSizeInBytes(Type *ObjectType)
{
    /*
     * TOP --- Cautiously convert the size of @ObjectType to bytes
     */ 

    /*
     * Setup
     */ 
    const uint64_t MinSize = 1;


    /*
     * Fetch the size in bytes, check for non-zero values
     */ 
    errs() << "ObjectType: " << *ObjectType << "\n";
    uint64_t PrimitiveSize = ObjectType->getPrimitiveSizeInBits();
    assert(PrimitiveSize && "Utils::GetPrimitiveSizeInBytes: Primitive size is 0!");

    
    /*
     * Convert to bytes
     */ 
    PrimitiveSize /= 8;


    /*
     * Return at least 1 byte
     */ 
    return (std::max(PrimitiveSize, MinSize));
}


uint64_t Utils::CalculateObjectSize(
    Type *ObjectType,
    DataLayout *Layout
)
{
    /*
     * TOP --- Switch based on the TypeID of @ObjectType,
     * calculate the object size in *bytes* directly or 
     * recursively
     */ 

    /*
     * Fetch TypeID
     */ 
    Type::TypeID TID = ObjectType->getTypeID();


    /*
     * Calculate the size based on "TID"
     */  
    switch (TID)
    {
        /*
         * Fetch the struct layout from @Layout, and return
         * the size directly in bytes
         */ 
        case Type::StructTyID:
        {
            StructType *StructTy = cast<StructType>(ObjectType);
            return (Layout->getStructLayout(StructTy)->getSizeInBytes());
        }


        /*
         * Recursively calculate the total size of the array
         */ 
        case Type::ArrayTyID:
        {
            ArrayType *ArrayTy = cast<ArrayType>(ObjectType);
            Type *ElementTy = ArrayTy->getElementType();
            return (ArrayTy->getNumElements() * CalculateObjectSize(ElementTy, Layout));
        }


        /*
         * Directly calculate primitive type's size
         */ 
        case Type::HalfTyID: 
        case Type::FloatTyID: 
        case Type::DoubleTyID: 
        case Type::X86_FP80TyID: 
        case Type::FP128TyID: 
        case Type::PPC_FP128TyID: 
        case Type::X86_MMXTyID: 
        case Type::IntegerTyID:
        case Type::VectorTyID:
            return Utils::GetPrimitiveSizeInBytes(ObjectType);

        
        /*
         * Directly return the pointer size from @Layout
         */ 
        case Type::PointerTyID:
            return Layout->getPointerSize();


        /*
         * Otherwise, abort
         */ 
        default:
        {
            errs() << "CalculateObjectSize: Cannot calculate object size for "
                   << *ObjectType << "...\n";
            abort();            
        }
    }

    
    /*
     * Unlikely return
     */ 
    return -1;
}


bool Utils::Verify(Module &M)
{
    /*
     * Check pass settings
     */  
    if (NoVerify) return true;


    /*
     * Run LLVM verifier on each function of @M
     */ 
    bool Failed = false;
    for (auto &F : M)
    {
        if (verifyFunction(F, &(errs())))
        {
            DEBUG_ERRS << "Failed verification: " 
                       << F.getName() << "\n"
                       << F << "\n";

            Failed |= true;
        }
    }


    return !Failed;
}


void Utils::VetAllocMethods(void)
{
    /*
     * TOP --- For each kernel alloc call --- perform
     * some simple checks --- check if uses of each 
     * method are limited to call instructions
     */ 

    /*
     * Check pass settings
     */  
    if (NoVerify) return;
    

    /*
     * Select a map to vet
     */ 
    std::unordered_map<Function *, AllocID> MapToVet = 
        (InstrumentingUserCode) ?
        (UserAllocMethodsToIDs) :
        (KernelAllocMethodsToIDs) ;


    /*
     * Iterate over kernel alloc methods
     */ 
    for (auto const &[Method, ID] : MapToVet)
    {
        errs() << "Vetting " << Method->getName() << " ...\n";

        /* 
         * Iterate over the users over each method
         */ 
        for (auto *User : Method->users())
        {
            /*
             * Verify that the user is a call instruction --- if
             * verification fails, print a warning
             */ 
            CallInst *CallUser = dyn_cast<CallInst>(User);
            if (!CallUser)
            {
                errs() << "WARNING: Suspicious user: "
                       << *User << "\n";
            }
        }
    }


    return;
}


void Utils::SetInstrumentationMetadata(
    Instruction *I,
    const std::string MDTypeString,
    const std::string MDLiteral
)
{
    /*  
     * Build metadata node using @MDTypeString
     */ 
    MDNode *TheNode = 
        MDNode::get(
            I->getContext(),
            MDString::get(
                I->getContext(), 
                MDTypeString
            )
        );  


    /*  
     * Set metadata with @MDLiteral
     */ 
    I->setMetadata(
        MDLiteral,
        TheNode
    );


    return;
}


void Utils::SetBaseInstrumentationMetadata(Instruction *I)
{
    /*
     * Add the base metadata ("injection" : "inj") to @I
     */
    Utils::SetInstrumentationMetadata(
        I,
        "injection",
        "inj"
    );


    return;
}


bool Utils::HasBaseInstrumentationMetadata(Instruction *I)
{
    /*
     * Check @I for the base metadata ("injection" : "inj")
     */
    return !!(I->getMetadata("inj"));
}