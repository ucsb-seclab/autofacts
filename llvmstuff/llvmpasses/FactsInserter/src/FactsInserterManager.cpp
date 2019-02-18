//
// Created by machiry on 12/8/18.
//

#include "FactsInserterManager.h"
#include "ModuleHelper.h"
#include "llvm/IR/IRBuilder.h"
#include "FactsSelector.h"

using namespace llvm;

#define MAXTRIES 40


unsigned long FactsInserterManager::localVarNum = 1;

unsigned long FactsInserterManager::getLocalVarNum() {
    return FactsInserterManager::localVarNum++;
}

Value* FactsInserterManager::insertLocalVariable(BasicBlock *targetBB, Type *targetType) {
    // insert before the terminator instruction.
    IRBuilder<> builder(targetBB->getTerminator());
    // generate a local variable name
    std::string currVarName = "localVar" + std::to_string(FactsInserterManager::getLocalVarNum());
    return builder.CreateAlloca(targetType, nullptr, currVarName);

}

void FactsInserterManager::computeBBDistance(BasicBlock *srcBB, BasicBlock *dstBB) {
    this->endClk();
    std::vector<BasicBlock*> visited;
    visited.clear();
    long totalPathLen = this->targetModuleHelper->getPathLength(srcBB, dstBB, 0, visited);
    if(totalPathLen >=0) {
        this->totalPathLength += totalPathLen;
    }
    this->startClk();
}

Value* FactsInserterManager::insertLocalFuncPtrVariable(BasicBlock *targetBB, Type *targetType) {
    // insert before the terminator instruction.
    IRBuilder<> builder(targetBB->getTerminator());
    // generate a local variable name
    std::string currVarName = "funcptr" + std::to_string(FactsInserterManager::getLocalVarNum());

    return builder.CreateAlloca(targetType->getPointerTo(), nullptr, currVarName);
}

bool FactsInserterManager::insertStoreInstruction(BasicBlock *targetBB, Value *srcVal, Value *dstValue) {
    // insert before the terminator instruction.
    IRBuilder<> builder(targetBB->getTerminator());
    // set volatile flag to avoid compiler removing them.
    Value *castedValue = builder.CreateBitCast(srcVal, dstValue->getType()->getPointerElementType(), "localVarCast");

    return builder.CreateStore(castedValue, dstValue, true) != nullptr;

}

bool FactsInserterManager::insertFuncPtrStoreInstruction(BasicBlock *targetBB, Function *srcVal, Value *dstValue) {
    // insert before the terminator instruction.
    IRBuilder<> builder(targetBB->getTerminator());
    // cast to the value that can be stored in the global variable.
    AllocaInst *newLo = dyn_cast<AllocaInst>(dstValue);
    Value *castedValue = builder.CreateBitCast(srcVal, newLo->getAllocatedType(), "funcPtrCast");
    // set volatile flag to avoid compiler removing them.
    return builder.CreateStore(castedValue, dstValue, true) != nullptr;

}
bool FactsInserterManager::insertStoreToGeneralGlobalVariable(BasicBlock *targetBB, Value *srcVal, Value *dstGlobalVariable) {

    // insert before the terminator instruction.
    IRBuilder<> builder(targetBB->getTerminator());

    if(dyn_cast<Instruction>(srcVal)) {
        builder.SetInsertPoint((dyn_cast<Instruction>(srcVal))->getNextNode());
    }

    // cast to the value that can be stored in the global variable.
    Value *castedValue = builder.CreateBitCast(srcVal, dstGlobalVariable->getType()->getPointerElementType(), "globalVariableCast");
    // Now create the store instruction, where we store the casted value into the global variable.
    return builder.CreateStore(castedValue, dstGlobalVariable, true) != nullptr;
}

bool FactsInserterManager::insertExitFunctionWithLoad(Value *toLoadFrom, Instruction *toInsertBefore) {
    // insert before the terminator instruction.
    IRBuilder<> builder(toInsertBefore);

    Value *loadedValue = builder.CreateLoad(toLoadFrom);

    Type *exitArgType = Type::getInt8Ty(toInsertBefore->getContext());

    Value *targetVal = builder.CreatePtrToInt(loadedValue, exitArgType, "castToInt");

    Module *targetModule = toInsertBefore->getFunction()->getParent();
    Constant *exitFunction = targetModule->getOrInsertFunction("exit", FunctionType::getVoidTy(toInsertBefore->getContext()), exitArgType);

    return builder.CreateCall(exitFunction, targetVal) != nullptr;
}

bool FactsInserterManager::insertInsensitiveDataPtrFact() {

    Type *targetType;
    std::set<Value*> targetLocals;
    std::vector<BasicBlock*> dstToInsertBBs;
    std::vector<Value*> dstLocals;

    unsigned long currTry = 0;

    while(currTry < MAXTRIES) {
        currTry++;
        // get a random function.
        Function *targetfunction = this->targetModuleHelper->getDefinedFunctionRandomly();
        // clear up.
        targetLocals.clear();
        if (this->targetModuleHelper->getSameTypeLocals(targetfunction, &targetType, targetLocals)) {
            // good we got locals that have same type.
            std::set<BasicBlock *> toInsertBBs;
            if (this->targetModuleHelper->getRandomBBs(targetfunction, toInsertBBs, targetLocals.size())) {
                // good, we got the basic blocks, where we are supposed to insert the store.
                // get the basic block where we are supposed to insert the pointer variable.
                BasicBlock *toInsertDefinition = this->targetModuleHelper->getBBToInsertLocals(targetfunction);
                // create a new local variable.
                Value *newLocalVar = this->insertLocalVariable(toInsertDefinition, targetType);

                // clear.
                dstToInsertBBs.clear();
                dstLocals.clear();


                // vectorize the corresponding sets.
                dstLocals.insert(dstLocals.end(), targetLocals.begin(), targetLocals.end());
                dstToInsertBBs.insert(dstToInsertBBs.end(), toInsertBBs.begin(), toInsertBBs.end());

                unsigned int currNo = 0;
                for(auto currLocal:dstLocals) {
                    if(currNo >= dstToInsertBBs.size()) {
                        currNo = dstToInsertBBs.size() - 1;
                    }
                    // insert the store instruction at corresponding points.
                    this->insertStoreInstruction(dstToInsertBBs[currNo], currLocal, newLocalVar);
                    currNo++;
                }
                DEBUG_PRINT("Successfully inserted an insensitive data pointer fact in try: %ld\n", currTry);
                //TODO: Store the point and the ground truth
                return true;

            } else {
                // go back try again.
                continue;
            }
        } else {
            // go back..try again.
            continue;
        }
    }
    ERROR_PRINT("Unable to insert an insensitive data pointer fact into the current module.\n");
    return false;
}

bool FactsInserterManager::insertInsensitiveFuncPtrFact() {
    Type *targetType;
    std::set<Function*> targetLocals;
    std::vector<BasicBlock*> dstToInsertBBs;
    std::vector<Function*> dstLocals;

    unsigned long currTry = 0;

    while(currTry < MAXTRIES) {
        currTry++;
        // get a random function.
        Function *targetfunction = this->targetModuleHelper->getDefinedFunctionRandomly();
        // clear up.
        targetLocals.clear();
        if (this->targetModuleHelper->getSameTypeFunctions(&targetType, targetLocals)) {
            // good we got locals that have same type.
            std::set<BasicBlock *> toInsertBBs;
            if (this->targetModuleHelper->getRandomBBs(targetfunction, toInsertBBs, targetLocals.size())) {
                // good, we got the basic blocks, where we are supposed to insert the store.
                // get the basic block where we are supposed to insert the pointer variable.
                BasicBlock *toInsertDefinition = this->targetModuleHelper->getBBToInsertLocals(targetfunction);
                // create a new local variable.
                Value *newLocalVar = this->insertLocalFuncPtrVariable(toInsertDefinition, targetType);

                // clear.
                dstToInsertBBs.clear();
                dstLocals.clear();


                // vectorize the corresponding sets.
                dstLocals.insert(dstLocals.end(), targetLocals.begin(), targetLocals.end());
                dstToInsertBBs.insert(dstToInsertBBs.end(), toInsertBBs.begin(), toInsertBBs.end());

                unsigned int currNo = 0;
                for(auto currLocal:dstLocals) {
                    if(currNo >= dstToInsertBBs.size()) {
                        currNo = dstToInsertBBs.size() - 1;
                    }
                    // insert the store instruction at corresponding points.
                    this->insertFuncPtrStoreInstruction(dstToInsertBBs[currNo], currLocal, newLocalVar);
                    currNo++;
                }
                DEBUG_PRINT("Successfully inserted an insensitive function pointer fact in try: %ld\n", currTry);
                //TODO: Store the point and the ground truth
                return true;

            } else {
                // go back try again.
                continue;
            }
        } else {
            // go back..try again.
            continue;
        }
    }
    ERROR_PRINT("Unable to insert an insensitive function pointer fact into the current module.\n");
    return false;
}

bool FactsInserterManager::insertFlowSensitiveDataPtrFact() {

    Type *targetType;
    std::set<Value*> targetLocals;
    std::vector<BasicBlock*> dstToInsertBBs;
    BasicBlock *verificationBB = nullptr;
    std::vector<Value*> dstLocals;

    unsigned long currTry = 0;

    while(currTry < MAXTRIES) {
        currTry++;
        // get a random function.
        Function *targetfunction = this->targetModuleHelper->getDefinedFunctionRandomly();
        // clear up.
        targetLocals.clear();
        if (this->targetModuleHelper->getSameTypeLocals(targetfunction, &targetType, targetLocals)) {
            dstToInsertBBs.clear();
            // good we got locals that have same type.
            if (this->targetModuleHelper->getRandomInFlowBBs(targetfunction, dstToInsertBBs,
                                                             &verificationBB, targetLocals.size())) {
                // good, we got the basic blocks, where we are supposed to insert the store.
                // get the basic block where we are supposed to insert the pointer variable.
                BasicBlock *toInsertDefinition = this->targetModuleHelper->getBBToInsertLocals(targetfunction);
                // create a new local variable.
                Value *newLocalVar = this->insertLocalVariable(toInsertDefinition, targetType);

                // clear.
                dstLocals.clear();


                // vectorize the corresponding sets.
                dstLocals.insert(dstLocals.end(), targetLocals.begin(), targetLocals.end());

                unsigned int currNo = 0;
                for(auto currLocal:dstLocals) {
                    if(currNo >= dstToInsertBBs.size()) {
                        currNo = dstToInsertBBs.size() - 1;
                    }
                    // insert the store instruction at corresponding points.
                    this->insertStoreInstruction(dstToInsertBBs[currNo], currLocal, newLocalVar);
                    currNo++;
                }

                computeBBDistance(dstToInsertBBs[0], verificationBB);
                DEBUG_PRINT("Successfully inserted flow sensitive data pointer fact in try: %ld\n", currTry);

                //TODO: Store the point and the ground truth
                return true;

            } else {
                // go back try again.
                continue;
            }
        } else {
            // go back..try again.
            continue;
        }
    }
    ERROR_PRINT("Unable to insert a flow-sensitive data pointer fact into the current module.\n");
    return false;
}

bool FactsInserterManager::insertFlowSensitiveFuncPtrFact() {

    Type *targetType;
    std::set<Function*> targetLocals;
    std::vector<BasicBlock*> dstToInsertBBs;
    BasicBlock *verificationBB = nullptr;
    std::vector<Function*> dstLocals;

    unsigned long currTry = 0;

    while(currTry < MAXTRIES) {
        currTry++;
        // get a random function.
        Function *targetfunction = this->targetModuleHelper->getDefinedFunctionRandomly();
        // clear up.
        targetLocals.clear();
        if (this->targetModuleHelper->getSameTypeFunctions(&targetType, targetLocals)) {
            dstToInsertBBs.clear();
            // good we got locals that have same type.
            if (this->targetModuleHelper->getRandomInFlowBBs(targetfunction, dstToInsertBBs,
                                                             &verificationBB, targetLocals.size())) {
                // good, we got the basic blocks, where we are supposed to insert the store.
                // get the basic block where we are supposed to insert the pointer variable.
                BasicBlock *toInsertDefinition = this->targetModuleHelper->getBBToInsertLocals(targetfunction);
                // create a new local variable.
                Value *newLocalVar = this->insertLocalFuncPtrVariable(toInsertDefinition, targetType);

                // clear.
                dstLocals.clear();


                // vectorize the corresponding sets.
                dstLocals.insert(dstLocals.end(), targetLocals.begin(), targetLocals.end());

                unsigned int currNo = 0;
                for(auto currLocal:dstLocals) {
                    if(currNo >= dstToInsertBBs.size()) {
                        currNo = dstToInsertBBs.size() - 1;
                    }
                    // insert the store instruction at corresponding points.
                    this->insertFuncPtrStoreInstruction(dstToInsertBBs[currNo], currLocal, newLocalVar);
                    currNo++;
                }
                computeBBDistance(dstToInsertBBs[0], verificationBB);
                DEBUG_PRINT("Successfully inserted flow sensitive function pointer fact in try: %ld\n", currTry);
                //TODO: Store the point and the ground truth
                return true;

            } else {
                // go back try again.
                continue;
            }
        } else {
            // go back..try again.
            continue;
        }
    }
    ERROR_PRINT("Unable to insert a flow-sensitive function pointer fact into the current module.\n");
    return false;
}

bool FactsInserterManager::insertPathSensitiveDataPtrFact(bool flowSensitive) {
    Type *targetType;
    std::set<Value*> targetLocals;
    std::vector<BasicBlock*> dstToInsertBBs;
    BasicBlock *verificationBB = nullptr;
    // flow-sensitive killer
    BasicBlock *flowSensitiveBB = nullptr;
    std::vector<Value*> dstLocals;

    unsigned long currTry = 0;

    while(currTry < MAXTRIES) {
        currTry++;
        // get a random function.
        Function *targetfunction = this->targetModuleHelper->getDefinedFunctionRandomly();
        // clear up.
        targetLocals.clear();
        if (this->targetModuleHelper->getSameTypeLocals(targetfunction, &targetType, targetLocals)) {
            dstToInsertBBs.clear();
            // good we got locals that have same type.
            if (this->targetModuleHelper->getDifferentPathsBBs(targetfunction, dstToInsertBBs,
                                                               &verificationBB, &flowSensitiveBB,
                                                               !flowSensitive, targetLocals.size())) {
                // good, we got the basic blocks, where we are supposed to insert the store.
                // get the basic block where we are supposed to insert the pointer variable.
                BasicBlock *toInsertDefinition = this->targetModuleHelper->getBBToInsertLocals(targetfunction);
                // create a new local variable.
                Value *newLocalVar = this->insertLocalVariable(toInsertDefinition, targetType);

                // clear.
                dstLocals.clear();


                // vectorize the corresponding sets.
                dstLocals.insert(dstLocals.end(), targetLocals.begin(), targetLocals.end());

                unsigned int currNo = 0;
                for(auto currLocal:dstLocals) {
                    if(currNo >= dstToInsertBBs.size()) {
                        currNo = dstToInsertBBs.size() - 1;
                    }
                    // insert the store instruction at corresponding points.
                    this->insertStoreInstruction(dstToInsertBBs[currNo], currLocal, newLocalVar);
                    currNo++;
                }

                if(flowSensitive) {
                    this->insertStoreInstruction(flowSensitiveBB, dstLocals[0], newLocalVar);
                    DEBUG_PRINT("Successfully inserted flow and path-sensitive data pointer fact in try: %ld\n", currTry);
                } else {
                    DEBUG_PRINT("Successfully inserted path-sensitive data pointer fact in try: %ld\n", currTry);
                }

                computeBBDistance(dstToInsertBBs[0], verificationBB);

                //TODO: Store the ground truth
                // i.e., any state that enters verificationBB
                // should have the new local point to one of the target locals.
                return true;

            } else {
                // go back try again.
                continue;
            }
        } else {
            // go back..try again.
            continue;
        }
    }
    if(flowSensitive) {
        ERROR_PRINT("Unable to insert a flow and path-sensitive data pointer fact into the current module.\n");
    } else {
        ERROR_PRINT("Unable to insert a path-sensitive data pointer fact into the current module.\n");
    }
    return false;
}
bool FactsInserterManager::insertPathSensitiveFuncPtrFact(bool flowSensitive) {
    Type *targetType;
    std::set<Function*> targetLocals;
    std::vector<BasicBlock*> dstToInsertBBs;
    BasicBlock *verificationBB = nullptr;
    // flow-sensitive killer
    BasicBlock *flowSensitiveBB = nullptr;
    std::vector<Function*> dstLocals;

    unsigned long currTry = 0;

    while(currTry < MAXTRIES) {
        currTry++;
        // get a random function.
        Function *targetfunction = this->targetModuleHelper->getDefinedFunctionRandomly();
        // clear up.
        targetLocals.clear();
        if (this->targetModuleHelper->getSameTypeFunctions(&targetType, targetLocals)) {
            dstToInsertBBs.clear();
            // good we got locals that have same type.
            if (this->targetModuleHelper->getDifferentPathsBBs(targetfunction, dstToInsertBBs,
                                                               &verificationBB, &flowSensitiveBB,
                                                               !flowSensitive, targetLocals.size())) {
                // good, we got the basic blocks, where we are supposed to insert the store.
                // get the basic block where we are supposed to insert the pointer variable.
                BasicBlock *toInsertDefinition = this->targetModuleHelper->getBBToInsertLocals(targetfunction);
                // create a new local variable.
                Value *newLocalVar = this->insertLocalFuncPtrVariable(toInsertDefinition, targetType);

                // clear.
                dstLocals.clear();


                // vectorize the corresponding sets.
                dstLocals.insert(dstLocals.end(), targetLocals.begin(), targetLocals.end());

                unsigned int currNo = 0;
                for(auto currLocal:dstLocals) {
                    if(currNo >= dstToInsertBBs.size()) {
                        currNo = dstToInsertBBs.size() - 1;
                    }
                    // insert the store instruction at corresponding points.
                    this->insertFuncPtrStoreInstruction(dstToInsertBBs[currNo], currLocal, newLocalVar);
                    currNo++;
                }

                if(flowSensitive) {
                    this->insertFuncPtrStoreInstruction(flowSensitiveBB, dstLocals[0], newLocalVar);
                    DEBUG_PRINT("Successfully inserted flow and path-sensitive function pointer fact in try: %ld\n", currTry);
                } else {
                    DEBUG_PRINT("Successfully inserted path-sensitive function pointer fact in try: %ld\n", currTry);
                }
                computeBBDistance(dstToInsertBBs[0], verificationBB);
                //TODO: Store the ground truth
                // i.e., any state that enters verificationBB
                // should have the new local point to one of the target locals.
                return true;

            } else {
                // go back try again.
                continue;
            }
        } else {
            // go back..try again.
            continue;
        }
    }
    if(flowSensitive) {
        ERROR_PRINT("Unable to insert a flow and path-sensitive function pointer fact into the current module.\n");
    } else {
        ERROR_PRINT("Unable to insert a path-sensitive function pointer fact into the current module.\n");
    }
    return false;
}

ContextSensitiveFact* FactsInserterManager::generateRandomCtxSensitiveFacts(Function *currFunction, unsigned long currIdx, std::map<Function *,
                                                           std::set<std::pair<Function*, CallInst*>>> &revCallMap,
                                                           std::map<Function*, std::set<Value*>> &insertedFacts,
                                                           std::set<Function*> &visitedFunctions,
                                                           std::set<Value*> &childFuncFacts,
                                                           Value* &targetGlobalVariable,
                                                           bool isFuncPtrFact,
                                                           CallInst *targetCallInstr) {
    if(visitedFunctions.find(currFunction) == visitedFunctions.end()) {
        // avoid looping in call-graph recursion.
        visitedFunctions.insert(currFunction);
        ContextSensitiveFact *toRet = nullptr;


        // newly created facts, that kill the facts from callee
        // incase of flow sensitive analysis.
        std::set<Value*> killFacts;
        killFacts.clear();

        // then facts that were newly created in this function.
        std::set<Value*> newlyCreatedFacts;
        newlyCreatedFacts.clear();

        // these are the facts that got accumulated upto
        // this function including its callees
        std::set<Value*> uptoThisFuncFacts;
        uptoThisFuncFacts.clear();
        uptoThisFuncFacts.insert(childFuncFacts.begin(), childFuncFacts.end());

        // randomly add a fact to the global variable.
        // randomly.. insert a fact.
        unsigned long randFlip = FactsSelector::getRandomUnsignedNum(2);
        // the probabilty decreases
        if((currIdx <=1 && randFlip) || (currIdx > 1 && (FactsSelector::getRandomUnsignedNum(currIdx) == 1))) {
            if(targetGlobalVariable == nullptr) {
                // create a global variable,  if there doesn't exist one.
                targetGlobalVariable = this->targetModuleHelper->createVoidPtrtoPtrGlobal();
            }
            Value* targetLocal = nullptr;
            if(!isFuncPtrFact) {
                targetLocal = this->targetModuleHelper->getRandomLocalVariable(currFunction);
            } else {
                targetLocal = this->targetModuleHelper->getRandomFunction();
            }
            if(targetLocal != nullptr) {
                bool insertFact = true;
                if(insertedFacts.find(currFunction) != insertedFacts.end()) {
                    // oh there is already a fact inserted in this function?
                    // for a local variable? Ignore.
                    if(insertedFacts[currFunction].find(targetLocal) != insertedFacts[currFunction].end()) {
                        insertFact = false;
                    } else {
                        for (auto currF: insertedFacts[currFunction]) {
                            if (!FactsSelector::getRandomUnsignedNum(2)) {
                                insertFact = false;
                                break;
                            }
                        }
                    }
                }
                // should we insert the fact?
                if(insertFact) {
                    // this will be the first basic-block.
                    BasicBlock *whereToInsert = this->targetModuleHelper->getBBToInsertLocals(currFunction);
                    // randomly insert the facts, after the call instruction or before
                    // (if affects flow-sensitive analysis)
                    bool isKillFact = true;
                    if(targetCallInstr != nullptr) {
                        // randomly select a predominator (in which case, it will kill the fact from caller)
                        // or post-dominator.
                        if(FactsSelector::getRandomUnsignedNum(2)) {
                            whereToInsert = this->targetModuleHelper->getPredominatorBB(targetCallInstr->getParent());
                        } else {
                            whereToInsert = this->targetModuleHelper->getPostDominatorBB(targetCallInstr->getParent());
                            isKillFact = false;
                        }
                    }
                    if(whereToInsert == nullptr) {
                        isKillFact = true;
                        whereToInsert = this->targetModuleHelper->getBBToInsertLocals(currFunction);
                    }
                    this->insertStoreToGeneralGlobalVariable(whereToInsert, targetLocal, targetGlobalVariable);
                    // add the value to the ground truth
                    uptoThisFuncFacts.insert(targetLocal);
                    insertedFacts[currFunction].insert(targetLocal);
                    newlyCreatedFacts.insert(targetLocal);
                    if(isKillFact) {
                        killFacts.insert(targetLocal);
                    }

                }
            }
        }

        if(revCallMap.find(currFunction) != revCallMap.end()) {
            // accumulated caller facts
            std::set<Value*> accumulatedCallerFacts;
            accumulatedCallerFacts.clear();
            // go into callers.
            for (auto &currCS: revCallMap[currFunction]) {
                ContextSensitiveFact *childFact = this->generateRandomCtxSensitiveFacts(currCS.first, currIdx + 1,
                                                                                        revCallMap,
                                                                                        insertedFacts,
                                                                                        visitedFunctions,
                                                                                        uptoThisFuncFacts,
                                                                                        targetGlobalVariable,
                                                                                        isFuncPtrFact,
                                                                                        currCS.second);

                if(childFact != nullptr) {
                    // accumulate the facts of children.
                    accumulatedCallerFacts.insert(childFact->totalFactValues.begin(), childFact->totalFactValues.end());
                    if(toRet == nullptr) {
                        toRet = new ContextSensitiveFact(currIdx);
                    }
                    // collect the child facts.
                    toRet->callerFacts.insert(childFact);
                }

            }

            uptoThisFuncFacts.insert(accumulatedCallerFacts.begin(), accumulatedCallerFacts.end());
        }

        if(!uptoThisFuncFacts.empty()) {
            if(toRet == nullptr) {
                toRet = new ContextSensitiveFact(currIdx);
            }
            toRet->insertedFacts.insert(newlyCreatedFacts.begin(), newlyCreatedFacts.end());
            toRet->totalFactValues.insert(uptoThisFuncFacts.begin(), uptoThisFuncFacts.end());
            toRet->killValues.insert(killFacts.begin(), killFacts.end());
        } else {
            // sanity.
            assert(killFacts.empty() && newlyCreatedFacts.empty());
        }


        visitedFunctions.erase(currFunction);

        return toRet;

    }
    return nullptr;
}

bool FactsInserterManager::insertContextSensitiveDataPtrFact(bool isFlowSensitive) {
    std::map<Function *, std::set<std::pair<Function*, CallInst*>>> &callRevFuncMap = this->targetModuleHelper->getCalleeFunctionMap();
    unsigned long currTry = 0;

    Value *targetGlobalVar = nullptr;
    std::set<Function*> visitedFunctions;

    std::set<Value*> dummySet;
    std::map<Function*, std::set<Value*>> insertedFacts;

    ContextSensitiveFact *targetCtxSenFact = nullptr;

    while(currTry < MAXTRIES) {
        currTry++;
        dummySet.clear();
        visitedFunctions.clear();
        insertedFacts.clear();
        Function *targetfunction = this->targetModuleHelper->getDefinedFunctionRandomly();
        if(callRevFuncMap.find(targetfunction) != callRevFuncMap.end()) {
            // Nice, we have some callers for this function.
            targetCtxSenFact = this->generateRandomCtxSensitiveFacts(targetfunction, 0, callRevFuncMap, insertedFacts,
                                                                     visitedFunctions, dummySet,
                                                                     targetGlobalVar);
            if(targetCtxSenFact != nullptr) {
                //TODO; insert the ground truth in the current function.
                // also release the targetCtxSen memory.
                // dbgs() << "Got into:" << targetfunction->getName().str() << "\n";

                delete(targetCtxSenFact);
                return true;
            }
        }
    }


}

bool FactsInserterManager::insertContextSensitiveFuncPtrFact(bool isFlowSensitive) {
    std::map<Function *, std::set<std::pair<Function*, CallInst*>>> &callRevFuncMap = this->targetModuleHelper->getCalleeFunctionMap();
    unsigned long currTry = 0;

    Value *targetGlobalVar = nullptr;
    std::set<Function*> visitedFunctions;

    std::set<Value*> dummySet;
    std::map<Function*, std::set<Value*>> insertedFacts;

    ContextSensitiveFact *targetCtxSenFact = nullptr;

    while(currTry < MAXTRIES) {
        currTry++;
        dummySet.clear();
        visitedFunctions.clear();
        insertedFacts.clear();
        Function *targetfunction = this->targetModuleHelper->getDefinedFunctionRandomly();
        if(callRevFuncMap.find(targetfunction) != callRevFuncMap.end()) {
            // Nice, we have some callers for this function.
            targetCtxSenFact = this->generateRandomCtxSensitiveFacts(targetfunction, 0, callRevFuncMap, insertedFacts,
                                                                     visitedFunctions, dummySet,
                                                                     targetGlobalVar, true);
            if(targetCtxSenFact != nullptr) {
                //TODO; insert the ground truth in the current function.
                // also release the targetCtxSen memory.
                // dbgs() << "Got into:" << targetfunction->getName().str() << "\n";

                delete(targetCtxSenFact);
                return true;
            }
        }
    }
}

bool FactsInserterManager::insertDataPtrFact(FACTSENSITIVITY targetSensitivity) {
    bool retVal = false;
    this->startClk();
    switch(targetSensitivity) {
        case FACTSENSITIVITY::NOSENSITIVITY:
            retVal = this->insertInsensitiveDataPtrFact();
            break;
        case FACTSENSITIVITY::FLOWSENSITIVE:
            retVal = this->insertFlowSensitiveDataPtrFact();
            break;
        case FACTSENSITIVITY::PATHSENSITIVE:
            retVal = this->insertPathSensitiveDataPtrFact();
            break;
        case FACTSENSITIVITY::FLOWPATHSENSITIVE:
            retVal = this->insertPathSensitiveDataPtrFact(true);
            break;
        case FACTSENSITIVITY::FLOWCONTEXTSENSITIVE:
            retVal = this->insertContextSensitiveDataPtrFact(true);
            break;
        case FACTSENSITIVITY::CONTEXTSENSITIVE:
            retVal = this->insertContextSensitiveDataPtrFact();
            break;
        default:
            assert(false && "Invalid sensitivity detected.");
            break;
    }

    this->endClk();
    if(retVal) {
        this->totalFactsInserted++;
    }
    return retVal;
}

bool FactsInserterManager::insertFuncPtrFact(FACTSENSITIVITY targetSensitivity) {
    bool retVal = false;
    this->startClk();
    switch(targetSensitivity) {
        case FACTSENSITIVITY::NOSENSITIVITY:
            retVal = this->insertInsensitiveFuncPtrFact();
            break;
        case FACTSENSITIVITY::FLOWSENSITIVE:
            retVal = this->insertFlowSensitiveFuncPtrFact();
            break;
        case FACTSENSITIVITY::PATHSENSITIVE:
            retVal = this->insertPathSensitiveFuncPtrFact();
            break;
        case FACTSENSITIVITY::FLOWPATHSENSITIVE:
            retVal = this->insertPathSensitiveFuncPtrFact(true);
            break;
        case FACTSENSITIVITY::FLOWCONTEXTSENSITIVE:
            retVal = this->insertContextSensitiveFuncPtrFact(true);
            break;
        case FACTSENSITIVITY::CONTEXTSENSITIVE:
            retVal = this->insertContextSensitiveFuncPtrFact();
            break;
        default: assert(false && "Invalid sensitivity detected."); break;
    }
    this->endClk();
    if(retVal) {
        this->totalFactsInserted++;
    }
    return retVal;
}
