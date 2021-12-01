
#if 0
        /*
         * Otherwise @inst belongs to a loop, so we can first try
         * to hoist the guard --- <Step 3.>
         */
        bool Hoistable = false;
        LoopStructure *NestedLoopStructure = NestedLoop->getLoopStructure();
        Instruction *PreheaderBBTerminator = nullptr;
        while (NestedLoop) 
        {
            InvariantManager *Manager = NestedLoop->getInvariantManager();
            if (Manager->isLoopInvariant(PointerOfMemoryInstruction)) 
            {
                //errs() << "YAY:   we found an invariant address to check:" << *inst << "\n";
                auto preheaderBB = NestedLoopStructure->getPreHeader();
                PreheaderBBTerminator = preheaderBB->getTerminator();

                auto tempNestedLoop = BasicBlockToLoopMap[preheaderBB];
                assert(tempNestedLoop != NestedLoop);
                NestedLoop = tempNestedLoop;
                NestedLoopStructure = 
                    (NestedLoop != nullptr) ?
                    NestedLoop->getLoopStructure() :
                    nullptr;

                Hoistable = true;

            } else {
                break;
            }
        }
        if(Hoistable)
        {
            /*
             * We can hoist the guard outside the loop.
             */
            InjectionLocations[inst] = 
                new GuardInfo(
                    PreheaderBBTerminator,
                    PointerOfMemoryInstruction, 
                    isWrite
                );

            loopInvariantGuard++;
            return;
        }
        //errs() << "XAN:   It cannot be hoisted. Check if it can be merged\n";

        /*
         * The memory instruction isn't loop invariant.
         *
         * Check if it is based on an induction variable.
         */
        NestedLoop = BasicBlockToLoopMap[inst->getParent()];
        assert (NestedLoop != nullptr);
        NestedLoopStructure = NestedLoop->getLoopStructure();

        auto ivManager = NestedLoop->getInductionVariableManager();
        auto PointerOfMemoryInstructionInst = dyn_cast<Instruction>(PointerOfMemoryInstruction);
        if (ivManager->doesContributeToComputeAnInductionVariable(PointerOfMemoryInstructionInst)){
            /*
                * The computation of the pointer is based on a bounded scalar evolution.
                * This means, the guard can be hoisted outside the loop where the boundaries used in the check go from the lowest to the highest address.
                */
            //SCEV_CARAT_Visitor visitor{};
            auto startAddress = NonCanonical; // FIXME Iterate over SCEV to fetch the actual start and end addresses
            //auto startAddress = visitor.visit((const SCEV *)AR);
            /*auto startAddressSCEV = AR->getStart();
                errs() << "XAN:         Start address = " << *startAddressSCEV << "\n";
                Value *startAddress = nullptr;
                if (auto startGeneric = dyn_cast<SCEVUnknown>(startAddressSCEV)){
                startAddress = startGeneric->getValue();
                } else if (auto startConst = dyn_cast<SCEVConstant>(startAddressSCEV)){
                startAddress = startConst->getValue();
                }*/
            if (startAddress){
                //errs() << "YAY: we found a scalar evolution-based memory instruction: " ;
                //inst->print(errs());
                //errs() << "\n";
                auto preheaderBB = NestedLoopStructure->getPreHeader();
                PreheaderBBTerminator = preheaderBB->getTerminator();

                scalarEvolutionGuard++;

                InjectionLocations[inst] = 
                    new GuardInfo(
                        PreheaderBBTerminator,
                        startAddress,
                        isWrite
                    );
                return;
            }
        }
// #endif //OPTIMIZED
        //errs() << "NOOO: the guard cannot be hoisted or merged: " << *inst << "\n" ;

        /*
         * The guard cannot be hoisted or merged.
         * We have to guard just before the memory instruction.
         */
        InjectionLocations[inst] = 
            new GuardInfo(
                inst,
                PointerOfMemoryInstruction, 
                isWrite
            );
        nonOptimizedGuard++;
        return;

#endif