//
// Created by machiry on 12/8/18.
//

#include "ModuleHelper.h"
#include "FactsSelector.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/CFG.h"
#include <algorithm>
#include <llvm/Analysis/LoopInfo.h>

#define MAXRETRIES 20
#define MAXPATHRETRIES 8

unsigned long ModuleHelper::currGlobNum = 1;

bool ModuleHelper::addFunctionCall(Function *current, Function *toCall, std::vector<Value*> toCallArgs, BasicBlock *toInsAfter) {
  // create an opaque predicate.

  Function::arg_iterator args = current->arg_begin();
  Value* x = args++;

  IRBuilder<> builder(toInsAfter->getTerminator());
  Value* mul1 = builder.CreateBinOp(Instruction::Mul, x, x);
  Value* sum = builder.CreateBinOp(Instruction::Add, x, builder.getInt32(1));
  Value* mul2 = builder.CreateBinOp(Instruction::Mul, sum, sum);  
  Value* opaque_pred = builder.CreateBinOp(Instruction::Mul, mul1, mul2);
  Value* rem = builder.CreateBinOp(Instruction::URem, opaque_pred, builder.getInt32(4));

  // ITE
  BasicBlock* ite_entry = BasicBlock::Create(TheContext, "op_ite_entry", current);
  BasicBlock* cond_true = BasicBlock::Create(TheContext, "op_cond_true", current);
  BasicBlock *newBB = toInsAfter->splitBasicBlock(toInsAfter->getTerminator(), "op_split");
  toInsAfter->getTerminator()->setSuccessor(0, ite_entry);
  builder.SetInsertPoint(ite_entry);
  Value* xEquals1 = builder.CreateICmpEQ(rem, builder.getInt32(1));
  builder.CreateCondBr(xEquals1, cond_true, newBB);

  builder.SetInsertPoint(cond_true);
  builder.CreateCall(toCall, toCallArgs);
  builder.CreateBr(newBB);
}

long ModuleHelper::getPathLength(BasicBlock *currBB, BasicBlock *dstBB, unsigned long currDistance, std::vector<BasicBlock*> &visitedBB) {
    if(currBB == dstBB) {
        return currDistance;
    }
    if(std::find(visitedBB.begin(), visitedBB.end(), currBB) != visitedBB.end()) {
        return -1;
    }
    visitedBB.push_back(currBB);
    for(unsigned i=0; i<currBB->getTerminator()->getNumSuccessors(); i++) {
        long currChildLen = this->getPathLength(currBB->getTerminator()->getSuccessor(i), dstBB, currDistance+1, visitedBB);
        if(currChildLen >= 0) {
            return currChildLen;
        }
    }
    visitedBB.pop_back();
    return -1;

}
bool ModuleHelper::organizeAllLocalVariables(Function *currFunction) {
    if(this->functionLocalVarMap.find(currFunction) == this->functionLocalVarMap.end()) {
        // compute
        for (Function::iterator b = currFunction->begin(), be = currFunction->end(); b != be; ++b) {
            BasicBlock* BB =  &(*b);
            for(BasicBlock::iterator i = BB->begin(), ie = BB->end(); i != ie; ++i) {
                Instruction *currIns = &(*i);
                if(AllocaInst *targetAlloca = dyn_cast<AllocaInst>(currIns)) {
                    // this is a local variable.
                    Type *targetType = targetAlloca->getType();
                    // if already inserted? Ignore.
                    if(this->functionLocalVarMap[currFunction].find(targetType) != this->functionLocalVarMap[currFunction].end()) {
                        if(this->functionLocalVarMap[currFunction][targetType].find(targetAlloca) != this->functionLocalVarMap[currFunction][targetType].end()) {
                            continue;
                        }
                    }
                    // else, insert
                    this->functionLocalVarMap[currFunction][targetType].insert(targetAlloca);
                }
            }
        }
        return true;
    }
    // all ready computed.
    return false;
}

bool ModuleHelper::isCompatipleFunctionType(FunctionType *currType, Function *currFunction) {
    FunctionType *dstType = currFunction->getFunctionType();
    // if they are exactly the same?
    if(currType == dstType) {
        return true;
    }
    if(currType->getNumParams() == dstType->getNumParams()) {
        // if return type is same?
        if(this->isCompatibleType(currType->getReturnType(), dstType->getReturnType())) {
            for(int i=0; i<currType->getNumParams(); i++) {
                if(!this->isCompatibleType(currType->getParamType(i), dstType->getParamType(i))) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;

}

bool ModuleHelper::isCompatibleType(Type *srcType, Type *dstType) {
    // check if both are pointer types.
    if(srcType->isPointerTy() || dstType->isPointerTy()) {
        if(!srcType->isPointerTy() || !dstType->isPointerTy()) {
            return false;
        }
        srcType = (dyn_cast<PointerType>(srcType))->getPointerElementType();
        dstType = (dyn_cast<PointerType>(dstType))->getPointerElementType();
        return this->isCompatibleType(srcType, dstType);
    }

    // if one of the elements is a struct type?
    if(srcType->isStructTy() || dstType->isStructTy()) {
        // do dumb comparision.
        return srcType == dstType;
    } else {
        // both of then are not structure types.
        // if both of them are integer types?
        if(srcType->isIntegerTy() || dstType->isIntegerTy()) {
            return srcType->isIntegerTy() && dstType->isIntegerTy();
        }
        // if both of them are floating point types?
        if(srcType->isFloatingPointTy() || dstType->isFloatingPointTy()) {
            return srcType->isFloatingPointTy() && dstType->isFloatingPointTy();
        }

        // if both of them are void types?
        if(srcType->isVoidTy() || dstType->isVoidTy()) {
            return srcType->isVoidTy() && dstType->isVoidTy();
        }
        return srcType == dstType;
    }
}

bool ModuleHelper::categorizeFunction(Function *currFunction) {
    FunctionType *currFuncType = currFunction->getFunctionType();
    for(auto &currEl: this->functionGroups) {
        if(this->isCompatipleFunctionType(currEl.first, currFunction)) {
            currEl.second.insert(currFunction);
            return false;
        }
    }
    this->functionGroups[currFuncType].insert(currFunction);
    return true;
}

ModuleHelper::ModuleHelper(llvm::Module &currModule, ReverseCallGraph *targetRevCG, std::map<Function*, LoopInfo*> &funcLoopInfo): targetModule(currModule), funcLoopInfo(funcLoopInfo) {
    // get the list of all functions defined and declared in the provided module.
    this->definedFunctions.clear();
    this->externalFunctions.clear();
    this->dominatorTreeMap.clear();
    this->insertableBasicBlocks.clear();
    this->functionGroups.clear();
    this->targetRevCG = targetRevCG;
    for(auto &currF:currModule.getFunctionList()) {
        if(!currF.isDeclaration()) {
            this->definedFunctions.push_back(&currF);
            // get dominator and post dominator trees for each defined function.
            DominatorTree *newDomTree = new DominatorTree();
            newDomTree->recalculate(currF);
            this->dominatorTreeMap[&currF] = newDomTree;

            PostDominatorTree *newPostDomTree = new PostDominatorTree();
            newPostDomTree->recalculate(currF);
            this->postDominatorTreeMap[&currF] = newPostDomTree;

            std::vector<BasicBlock*> *newBBList = new std::vector<BasicBlock*>();
            for(auto &currBB: currF.getBasicBlockList()) {
                newBBList->push_back(&currBB);
            }

            // remove all roots of forward dominator tree.
            for(auto currBB: newDomTree->getRoots()) {
                auto currIte = std::find(newBBList->begin(), newBBList->end(), currBB);
                if(currIte != newBBList->end()) {
                    newBBList->erase(currIte);
                }
            }

            // remove all roots from the list.
            for(auto currBB: newPostDomTree->getRoots()) {
                auto currIte = std::find(newBBList->begin(), newBBList->end(), currBB);
                if(currIte != newBBList->end()) {
                    newBBList->erase(currIte);
                }
            }

            this->insertableBasicBlocks[&currF] = newBBList;

            this->organizeAllLocalVariables(&currF);

        } else {
            this->externalFunctions.push_back(&currF);
        }
        if(currF.hasName() && currF.getName().startswith_lower("llvm.")) {
            continue;
        }
        this->categorizeFunction(&currF);
    }

}

ModuleHelper::~ModuleHelper() {
    // free up all the memory.
    for(auto &curVa: this->dominatorTreeMap) {
        delete(curVa.second);
    }

    for(auto &curVa: this->postDominatorTreeMap) {
        delete(curVa.second);
    }

    for(auto &currVa: this->insertableBasicBlocks) {
        delete(currVa.second);
    }
}

Function* ModuleHelper::getDefinedFunctionRandomly() {
    unsigned long funcIdx = FactsSelector::getRandomUnsignedNum(this->definedFunctions.size());
    return this->definedFunctions[funcIdx];
}

bool ModuleHelper::getRandomBBs(Function *targetFunction, std::set<BasicBlock*> &targetBBs, long reqNum) {
    unsigned long numBBs = reqNum;
    std::vector<BasicBlock*> *insertableBBs = this->insertableBasicBlocks[targetFunction];
    if(reqNum < 0) {
        numBBs = 1 + FactsSelector::getRandomUnsignedNum(insertableBBs->size());
        if(numBBs > insertableBBs->size()) {
            numBBs = insertableBBs->size();
        }
    }

    if(insertableBBs->size() > 1) {
        while(numBBs != 0) {
            unsigned long bbIdx = FactsSelector::getRandomUnsignedNum(insertableBBs->size());

            BasicBlock *toIns = (*insertableBBs)[bbIdx];
            if(targetBBs.find(toIns) == targetBBs.end()) {
                targetBBs.insert(toIns);
            }
            numBBs--;
        }

        if(targetBBs.size() < numBBs) {
            unsigned long toInsert = numBBs - targetBBs.size();
            for (auto currBB:*insertableBBs) {
                if(targetBBs.find(currBB) == targetBBs.end()) {
                    targetBBs.insert(currBB);
                    toInsert--;
                }
                if(toInsert == 0) {
                    break;
                }
            }
        }

        return !targetBBs.empty();
    }
    // there are no modifiable basic blocks in this function.
    return false;
}

bool ModuleHelper::hasBBFlowOfLen(BasicBlock *srcBB, std::vector<BasicBlock*> &flowBBs, unsigned long reqLen) {

    DominatorTree *currDomTree = this->dominatorTreeMap[srcBB->getParent()];
    SmallVector<BasicBlock*, 1024> allBBDescendents;

    if(std::find(flowBBs.begin(), flowBBs.end(), srcBB) != flowBBs.end()) {
        return false;
    }

    if(reqLen == 1) {
        flowBBs.push_back(srcBB);
        return true;
    }

    allBBDescendents.clear();
    currDomTree->getDescendants(srcBB, allBBDescendents);

    if(allBBDescendents.size() > 1) {
        for(auto currDescendentBB: allBBDescendents) {
            if (FactsSelector::getRandomUnsignedNum(2)) {
                flowBBs.push_back(srcBB);
                if (this->hasBBFlowOfLen(currDescendentBB, flowBBs, reqLen - 1)) {
                    return true;
                }
                flowBBs.erase(flowBBs.end() - 1);
            } else {
                if (this->hasBBFlowOfLen(currDescendentBB, flowBBs, reqLen)) {
                    return true;
                }
            }
        }
    }
    return false;

}

BasicBlock* ModuleHelper::getNearestCommonDominator(std::vector<BasicBlock*> &targetBBs, bool isPostDominator) {
    if(!targetBBs.empty()) {
        PostDominatorTree *currPostDomTree = this->postDominatorTreeMap[targetBBs[0]->getParent()];
        DominatorTree *currPreDomTree = this->dominatorTreeMap[targetBBs[0]->getParent()];


        BasicBlock* dstBB = targetBBs[0];
        BasicBlock* bb1 = nullptr;
        for(unsigned i=1; i<targetBBs.size(); i++) {
            bb1 = targetBBs[i];
            // if there is no post dominator?
            // return nullptr
            if(dstBB == nullptr) {
                return nullptr;
            }
            if(isPostDominator) {
                dstBB = currPostDomTree->findNearestCommonDominator(dstBB, bb1);
            } else {
                dstBB = currPreDomTree->findNearestCommonDominator(dstBB, bb1);
            }
        }

        return dstBB;
    }
    return nullptr;
}

bool ModuleHelper::isSuccessorsInDifferentPath(BasicBlock *currBB, BasicBlock **targetPostDomBB,
                                               BasicBlock **targetPreDomBB,
                                               bool strictPath) {

    std::vector<BasicBlock*> allSucessors;
    allSucessors.clear();

    SmallVector<BasicBlock*, 1024> allBBDescendents;

    std::vector<BasicBlock*> currVector;
    currVector.clear();

    for(unsigned i=0; i<currBB->getTerminator()->getNumSuccessors(); i++) {
        assert(currBB->getTerminator()->getSuccessor(i) != nullptr);
        if(currBB->getTerminator()->getSuccessor(i) != nullptr) {
            allSucessors.push_back(currBB->getTerminator()->getSuccessor(i));
            currVector.push_back(currBB->getTerminator()->getSuccessor(i));
        }
    }

    if(strictPath) {
        // ensure there is no path between any of the siblings.
        for (unsigned i = 0; i < currBB->getTerminator()->getNumSuccessors(); i++) {
            BasicBlock *toCheckBB = currBB->getTerminator()->getSuccessor(i);
            assert(toCheckBB == *currVector.begin());
            currVector.erase(currVector.begin());
            allBBDescendents.clear();

            allBBDescendents.insert(allBBDescendents.begin(), currVector.begin(), currVector.end());

            // one of the basic blocks is reachable from
            // a bb in the vector.
            if (isPotentiallyReachableFromMany(allBBDescendents, toCheckBB)) {
                return false;
            }
            currVector.push_back(toCheckBB);
        }
    }

    if(targetPostDomBB) {
        // get the nearest common post-dominator
        *targetPostDomBB = this->getNearestCommonDominator(allSucessors, true);
    }
    if(targetPreDomBB) {
        // get the nearest common pre-dominator
        *targetPreDomBB = this->getNearestCommonDominator(allSucessors);
    }
    return true;
}

bool ModuleHelper::getDifferentPathsBBs(Function *targetFunction,
                                        std::vector<BasicBlock*> &toInsertBBs,
                                        BasicBlock **postDominatorBB,
                                        BasicBlock **preDominatorBB,
                                        bool strictPath,
                                        unsigned long reqLen) {
    PostDominatorTree *postDomTree = this->postDominatorTreeMap[targetFunction];
    // get a random basic block, that has at least reqNum of descendants.
    std::set<BasicBlock*> tmpBBSet;
    SmallVector<BasicBlock*, 1024> allBBDescendents;

    if(targetFunction->getBasicBlockList().size() < 3) {
        return false;
    }

    BasicBlock *targetBB = nullptr;
    bool foundSomething = false;
    BasicBlock *bestKnownBB = nullptr;
    BasicBlock *targetPostDom = nullptr;
    BasicBlock *targetPreDom = nullptr;
    unsigned int bestKnownSucc = 2;
    unsigned long currRetry = MAXRETRIES;
    while(currRetry) {
        tmpBBSet.clear();
        // get a random basic block.

        if(this->getRandomBBs(targetFunction, tmpBBSet, 1)) {
            targetBB = *tmpBBSet.begin();
            unsigned int numSucc = targetBB->getTerminator()->getNumSuccessors();

            if (numSucc > 1 && this->isSuccessorsInDifferentPath(targetBB, &targetPostDom, &targetPreDom, strictPath)) {
                if (numSucc > bestKnownSucc) {
                    bestKnownSucc = numSucc;
                    bestKnownBB = targetBB;
                    // ok, we tried enough number of times to find
                    // bb with right number of successors.
                    if ((MAXRETRIES - currRetry) >= MAXPATHRETRIES) {
                        break;
                    }
                }
                if (numSucc >= reqLen) {
                    bestKnownBB = targetBB;
                    foundSomething = true;
                    break;
                }
            }
        }
        currRetry--;
    }

    if(targetPostDom == nullptr) {
        return false;
    }

    if(foundSomething) {
        DEBUG_PRINT("Got right number of basic blocks for inserting path-sensitive fact.\n");
    }

    if(bestKnownBB != nullptr) {
        // ok, we got something.
        for(unsigned i=0; i<bestKnownBB->getTerminator()->getNumSuccessors(); i++) {
            toInsertBBs.push_back(bestKnownBB->getTerminator()->getSuccessor(i));
        }

        // also insert the desendents of the post dom.
        assert(targetPostDom != nullptr);
        allBBDescendents.clear();
        postDomTree->getDescendants(targetPostDom, allBBDescendents);
        for(auto currBB: allBBDescendents) {
            if(std::find(toInsertBBs.begin(), toInsertBBs.end(), currBB) == toInsertBBs.end()) {
                toInsertBBs.push_back(currBB);
            }
        }
        if(postDominatorBB != nullptr) {
            *postDominatorBB = targetPostDom;
        }
        if(preDominatorBB != nullptr) {
            *preDominatorBB = targetPreDom;
        }

        return true;
    }

    return false;
}

bool ModuleHelper::getRandomInFlowBBs(Function *targetFunction, std::vector<BasicBlock*> &targetInFlowBBs,
                                      BasicBlock **postDominatorBB, unsigned long reqNum) {

    PostDominatorTree *postDomTree = this->postDominatorTreeMap[targetFunction];
    // get a random basic block, that has at least reqNum of descendants.
    std::set<BasicBlock*> tmpBBSet;
    SmallVector<BasicBlock*, 1024> allBBDescendents;
    BasicBlock *targetBB = nullptr;

    bool foundSomething = false;
    if(targetFunction->getBasicBlockList().size() < 3) {
        // very small function.
        return false;
    }
    unsigned long currRetry = MAXRETRIES;
    while(currRetry) {
        tmpBBSet.clear();
        allBBDescendents.clear();
        targetInFlowBBs.clear();

        // get a random basic block.
        if(this->getRandomBBs(targetFunction, tmpBBSet, 1)) {
            targetBB = *tmpBBSet.begin();
            // check that the basic block has a flow of at least certain length.
            if (this->hasBBFlowOfLen(targetBB, targetInFlowBBs, reqNum)) {
                // Damn, we got a basic block from which we have a path of certain length.
                // Now try to find the post-decessor of all the bbs in targetInFlowBBs.
                BasicBlock *targetPostDom = postDomTree->findNearestCommonDominator(targetInFlowBBs.front(),
                                                                                    targetInFlowBBs.back());
                if (targetPostDom != nullptr) {
                    // get all the descendents.
                    postDomTree->getDescendants(targetPostDom, allBBDescendents);
                    // randomly pick a descendent to be the insertion point.
                    BasicBlock *targetInsertionPoint = allBBDescendents[FactsSelector::getRandomUnsignedNum(
                            allBBDescendents.size())];
                    *postDominatorBB = targetInsertionPoint;
                    foundSomething = true;
                    break;
                }
            }
        }
        currRetry--;
    }

    return foundSomething;


}

bool ModuleHelper::getSameTypeLocals(Function *targetFunction, Type **targetType, std::set<Value*> &targetLocals) {
    if(this->functionLocalVarMap.find(targetFunction) != this->functionLocalVarMap.end()) {
        for (auto &locVarMap: this->functionLocalVarMap[targetFunction]) {
            if (locVarMap.second.size() > 1) {
                if (FactsSelector::getRandomUnsignedNum(2)) {
                    *targetType = locVarMap.first;
                    // has only 2 locals? Insert both of them.
                    if(locVarMap.second.size() <= 2) {
                        targetLocals.insert(locVarMap.second.begin(), locVarMap.second.end());
                        break;
                    } else {
                        // has more than 2 locals? select a random number of locals
                        // more than 2
                        unsigned long numLocals = 2 + FactsSelector::getRandomUnsignedNum(locVarMap.second.size() - 2);
                        std::vector<Value*> tmpLocals;
                        tmpLocals.clear();

                        tmpLocals.insert(tmpLocals.end(), locVarMap.second.begin(), locVarMap.second.end());

                        std::mt19937 engine(rand());
                        // randomly pick local variables.
                        while(numLocals) {
                            // number distribution
                            std::uniform_int_distribution<int> choose(0, tmpLocals.size() - 1);
                            Value* currL = tmpLocals[choose(engine)];
                            if(targetLocals.find(currL) == targetLocals.end()) {
                                targetLocals.insert(currL);
                                numLocals--;
                            }
                        }

                    }
                }
            }
        }

        // if we did really got unlucky..just pick the first set of locals.
        if(targetLocals.empty()) {
            for (auto &locVarMap: this->functionLocalVarMap[targetFunction]) {
                if (locVarMap.second.size() > 1) {
                    *targetType = locVarMap.first;
                    targetLocals.insert(locVarMap.second.begin(), locVarMap.second.end());
                    break;
                }
            }
        }
    }
    return !targetLocals.empty();
}

bool ModuleHelper::getSameTypeFunctions(Type **targetType, std::set<Function*> &targetFunctions) {
    for (auto &funcTypeMap: this->functionGroups) {
        if (funcTypeMap.second.size() > 1) {
            if (FactsSelector::getRandomUnsignedNum(2)) {
                *targetType = funcTypeMap.first;
                // has only 2 function? Insert both of them.
                if(funcTypeMap.second.size() <= 2) {
                    targetFunctions.insert(funcTypeMap.second.begin(), funcTypeMap.second.end());
                    break;
                } else {
                    // has more than 2 functions? select a random number of functions
                    // more than 2
                    unsigned long numLocals = 2 + FactsSelector::getRandomUnsignedNum(funcTypeMap.second.size() - 2);
                    std::vector<Function*> tmpFunctions;
                    tmpFunctions.clear();

                    tmpFunctions.insert(tmpFunctions.end(), funcTypeMap.second.begin(), funcTypeMap.second.end());

                    std::mt19937 engine(rand());
                    // randomly pick functions
                    while(numLocals) {
                        // number distribution
                        std::uniform_int_distribution<int> choose(0, tmpFunctions.size() - 1);
                        Function* currL = tmpFunctions[choose(engine)];
                        if(targetFunctions.find(currL) == targetFunctions.end()) {
                            targetFunctions.insert(currL);
                            numLocals--;
                        }
                    }

                }
            }
        }
    }

    // if we did really got unlucky..just pick the first set of locals.
    if(targetFunctions.empty()) {
        for (auto &funcTypeMap: this->functionGroups) {
            if (funcTypeMap.second.size() > 1) {
                *targetType = funcTypeMap.first;
                targetFunctions.insert(funcTypeMap.second.begin(), funcTypeMap.second.end());
                break;
            }
        }
    }
    return !targetFunctions.empty();
}

BasicBlock* ModuleHelper::getBBToInsertLocals(Function *targetFunction) {
    // basically return the first node in the dominator tree.
    return this->dominatorTreeMap[targetFunction]->getRoots()[0];
}

std::map<Function *, std::set<std::pair<Function*, CallInst*>>> &ModuleHelper::getCalleeFunctionMap() {
    return this->targetRevCG->getCalleeFunctionMap();
}

Value* ModuleHelper::createVoidPtrtoPtrGlobal() {
    std::string globalName = "autoFactsG" + std::to_string(ModuleHelper::currGlobNum++);
    GlobalVariable *newGlobalVariable = new GlobalVariable(this->targetModule,
                                                           IntegerType::getInt8PtrTy(this->targetModule.getContext()),
                                                           false,
                                                           GlobalValue::CommonLinkage, nullptr, globalName);
    newGlobalVariable->setAlignment(4);

    ConstantPointerNull* const_ptr_2 = ConstantPointerNull::get(IntegerType::getInt8PtrTy(this->targetModule.getContext()));
    newGlobalVariable->setInitializer(const_ptr_2);

    return newGlobalVariable;

}

Value* ModuleHelper::getRandomLocalVariable(Function *targetFunction) {
    for (auto &locVarMap: this->functionLocalVarMap[targetFunction]) {
        if (FactsSelector::getRandomUnsignedNum(2)) {
            return *locVarMap.second.begin();
        }
    }
    for (auto &locVarMap: this->functionLocalVarMap[targetFunction]) {
        return *locVarMap.second.begin();
    }
    return nullptr;
}

Function* ModuleHelper::getRandomFunction() {
    for (auto &funcTypeMap: this->functionGroups) {
        if (FactsSelector::getRandomUnsignedNum(2)) {
            return *funcTypeMap.second.begin();
        }
    }
    for (auto &funcTypeMap: this->functionGroups) {
        return *funcTypeMap.second.begin();
    }
    return nullptr;
}

BasicBlock* ModuleHelper::getPostDominatorBB(BasicBlock *currBB) {
    PostDominatorTree *currTree = this->postDominatorTreeMap[currBB->getParent()];
    return currTree->getNode(currBB)->getIDom()->getBlock();
}

BasicBlock* ModuleHelper::getPredominatorBB(BasicBlock *currBB) {
    DominatorTree *currTree = this->dominatorTreeMap[currBB->getParent()];
    if(currTree->getNode(currBB)->getIDom() != nullptr) {
        return currTree->getNode(currBB)->getIDom()->getBlock();
    }
    return currBB;
}
