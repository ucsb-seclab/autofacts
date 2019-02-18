//
// Created by machiry on 12/8/18.
//

#ifndef PROJECT_MODULEHELPER_H
#define PROJECT_MODULEHELPER_H

#include <llvm/IR/Module.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/DominanceFrontier.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Analysis/LoopInfo.h>
#include "ReverseCallGraph.h"
#include <llvm/IR/Type.h>

using namespace llvm;

static LLVMContext TheContext;


/***
 * Class that implements various functions
 * that helps in getting various aspects of the module
 */
class ModuleHelper {
private:
    Module &targetModule;
    // reverse call-graph
    ReverseCallGraph *targetRevCG;
    // map of function and its dominator tree.
    std::map<Function *, DominatorTree *> dominatorTreeMap;
    // map of function and its post dominator tree.
    std::map<Function *, PostDominatorTree *> postDominatorTreeMap;
    // map of function and the set of local variables grouped by type.
    std::map<Function *, std::map<Type*, std::set<Value*>>> functionLocalVarMap;
    // map of function and the vector of basic blocks where we can insert
    // local variables.
    std::map<Function *, std::vector<BasicBlock*>*> insertableBasicBlocks;
    // Map of function and corresponding loop info
    std::map<Function*, LoopInfo*> &funcLoopInfo;

    // functions grouped according to their type.
    std::map<FunctionType*, std::set<Function*>> functionGroups;

    // all functions in the module
    // defined functions.
    std::vector<Function*> definedFunctions;
    // external functions.
    std::vector<Function*> externalFunctions;
    // variable to keep track of number of globals that have been inserted.
    static unsigned long currGlobNum;

    /**
     *  Organize all the local variables in the provided function.
     * @param currFunction Function whose local variables needs to be organized.
     * @return true if it well else false.
     */
    bool organizeAllLocalVariables(Function *currFunction);

    /***
     * Check if there is a intra-procedural flow of a given length.
     * if yes, the corresponding basicblocks will be stored in flowBBs
     *
     * @param srcBB source BB from where the flow should start.
     * @param flowBBs vector into where the bbs having flow needs to be stored.
     * @param reqLen required length of the basic blocks.
     * @return true if successful else false.
     */
    bool hasBBFlowOfLen(BasicBlock *srcBB, std::vector<BasicBlock*> &flowBBs, unsigned long reqLen);

    /**
     * check if the successors of the current basic block are in different paths.
     * @param currBB BasicBlock whose successors need to be checked.
     * @param targetPostDomBB Pointer into which the post dominator BB should be stored.
     * @param targetPreDomBB Pointer into which the pre-dominator bb should be stored.
     * @param strictPath flag to indicate if the sucessors should not be reachable.
     * @return true, if sucessors belong to different paths.
     */
    bool isSuccessorsInDifferentPath(BasicBlock *currBB, BasicBlock **targetPostDomBB = nullptr,
                                     BasicBlock **targetPreDomBB = nullptr,
                                     bool strictPath=true);


    /**
     *  Find nearest common (pre or post) dominator of the set of basicblocks.
     *
     * @param targetBBs basicblocks whose nearest common dominator should be found.
     * @param isPostDominator true. if post-dominator.
     * @return the BasicBlock which is the nearest dominator.
     */
    BasicBlock* getNearestCommonDominator(std::vector<BasicBlock*> &targetBBs, bool isPostDominator=false);

    /***
     *
     * @param currType
     * @param currFunction
     * @return
     */
    bool isCompatipleFunctionType(FunctionType *currType, Function *currFunction);

    bool isCompatibleType(Type *srcType, Type *dstType);

    bool categorizeFunction(Function *currFunction);


public:
    ModuleHelper(Module &currModule, ReverseCallGraph *targetRevCG, std::map<Function*, LoopInfo*> &funcLoopInfo);

    ~ModuleHelper();

    long getPathLength(BasicBlock *currBB, BasicBlock *dstBB, unsigned long currDistance, std::vector<BasicBlock*> &visitedBB);


    /***
     * Get a random function in this module.
     * @return target random function.
     */
    Function* getDefinedFunctionRandomly();

    /***
     * Get a set of random basicblocks from a given function.
     *
     * @param targetFunction Function whose basic-blocks need to be fetched.
     * @param targetBBs set where the chosen basicblocks need to be stored.
     * @param reqNum required number of BasicBlocks to be fetched.
     * @return true if the fetching of the basic-blocks was successful else false.
     */
    bool getRandomBBs(Function *targetFunction, std::set<BasicBlock*> &targetBBs, long reqNum=-1);

    /***
     * Get the random list of basicblock having flow.
     *
     * @param targetFunction Target function.
     * @param targetInFlowBBs Vector where the basicblocks needs to be stored.
     * @param postDominatorBB Pointer into which the post dominator basicblock should be stored.
     * @param reqNum required number of basicblocks need to be fetched.
     * @return true if the fetching of the basic-blocks was successful else false.
     */
    bool getRandomInFlowBBs(Function *targetFunction, std::vector<BasicBlock*> &targetInFlowBBs,
                            BasicBlock **postDominatorBB, unsigned long reqNum);

    /***
     * Get a random set of local variables that have the same type.
     * @param targetFunction Function whose local variables need to be fetched.
     * @param targetType Type of the local variables.
     * @param targetLocals the target set of local variables having same type.
     * @return true if successful.
     */
    bool getSameTypeLocals(Function *targetFunction, Type **targetType, std::set<Value*> &targetLocals);


    bool getSameTypeFunctions(Type **targetType, std::set<Function*> &targetFunctions);

    /***
     * Get a random variable of the requested function.
     * @param targetFunction Target function.
     * @return Pointer to the local variable.
     */
    Value* getRandomLocalVariable(Function *targetFunction);

    Function* getRandomFunction();

    /***
     *
     * @param current
     * @param toCall
     * @param toCallArgs
     * @param toInsBefore
     * @return
     */
    bool addFunctionCall(Function *current, Function *toCall, std::vector<Value*> toCallArgs, BasicBlock *toInsBefore);

    /***
     * Get a pointer to the basic block where local variables could be inserted.
     * @param targetFunction Target function
     * @return Pointer to a BasicBlock.
     */
    BasicBlock* getBBToInsertLocals(Function *targetFunction);


    /***
     * Get the post-dominator of the provided basic block.
     * @param currBB Target basic block
     * @return Pointer to the post-dominator basic-block.
     */
    BasicBlock* getPostDominatorBB(BasicBlock *currBB);

    /***
     * Get the post-dominator of the provided basic block.
     * @param currBB Target basic block
     * @return Pointer to the pre-dominator basic-block.
     */
    BasicBlock* getPredominatorBB(BasicBlock *currBB);

    /***
     * Get basicblocks of the requested function that do not post-dominate the other one.
     * and they should have a common post-dominator.
     * and they should not be reachable from the other one (controlled by a flag).
     *
     * @param currFunction Target function.
     * @param toInsertBBs Vector of basic blocks.
     * @param postDominatorBB pointer where the post-dominator basicblock should be stored.
     * @param preDominatorBB pointer where the pre-dominator basicblock should be stored.
     * @param strictPath flag to indicate that the returned basicblocks should not be reachable
     *                   from each other.
     * @param reqLen required length of the basicblocks.
     * @return true if successful else false.
     */
    bool getDifferentPathsBBs(Function *currFunction, std::vector<BasicBlock*> &toInsertBBs,
                              BasicBlock **postDominatorBB, BasicBlock **preDominatorBB,
                              bool strictPath,
                              unsigned long reqLen);

    /***
     * Create a global variable which is a void**
     *
     * @return Pointer to the newly created global.
     */
    Value* createVoidPtrtoPtrGlobal();

    /***
     * Get the reverse call-graph map of all the functions.
     * @return Reference to the reverse call-graph of the functions.
     */
    std::map<Function *, std::set<std::pair<Function*, CallInst*>>> &getCalleeFunctionMap();

};

#endif //PROJECT_MODULEHELPER_H
