//
// Created by machiry on 12/8/18.
//



#ifndef PROJECT_INSERTERHELPER_H
#define PROJECT_INSERTERHELPER_H

#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <set>
#include "FactsType.h"
#include "ContextSensitiveFacts.h"

using namespace llvm;

class ModuleHelper;
/***
 * This class manages
 * inserting various types of facts into
 * the provided llvm module
 */
class FactsInserterManager{
private:

    llvm::Module &targetModule;
    ModuleHelper *targetModuleHelper;



    // helper functions to insert various types of data ptr facts.
    /**
     * Insert a set of facts which have no sensitivity.
     *
     * @return true if successful else false.
     */
    bool insertInsensitiveDataPtrFact();

    bool insertInsensitiveFuncPtrFact();

    /***
     * Insert a set of flow-sensitive facts.
     * @return true if successful else false.
     */
    bool insertFlowSensitiveDataPtrFact();

    bool insertFlowSensitiveFuncPtrFact();

    /***
     * Insert a set of path-sensitive facts.
     * @param flowSensitive true, if the facts should also be flow-sensitive.
     * @return true if successful else false.
     */
    bool insertPathSensitiveDataPtrFact(bool flowSensitive=false);

    bool insertPathSensitiveFuncPtrFact(bool flowSensitive=false);

    /***
     * Insert a set of context-sensitive data ptr facts.
     * @param isFlowSensitive if the facts need to be flow-sensitive too.
     * @return true if successful else false.
     */
    bool insertContextSensitiveDataPtrFact(bool isFlowSensitive=false);

    bool insertContextSensitiveFuncPtrFact(bool isFlowSensitive=false);

    /***
     *
     * @param toLoadFrom
     * @param toInsertBefore
     * @return
     */
    bool insertExitFunctionWithLoad(Value *toLoadFrom, Instruction *toInsertBefore);


    /***
     * Generate a context-sensitive fact starting from the given function.
     *
     * @param currFunction starting function.
     * @param currIdx Call-site depth.
     * @param revCallMap Reverse call-graph.
     * @param insertedFacts Map of function and corresponding inserted facts.
     * @param visitedFunctions set of visited functions.
     * @param childFuncFacts set of facts inserted in callees
     * @param targetGlobalVariable Global variable into which stores should be inserted.
     * @param targetCallInstr Call instruction that made the call to currFunction.
     * @return Pointer to the ContextSensitiveFact structure.
     */
    ContextSensitiveFact* generateRandomCtxSensitiveFacts(Function *currFunction, unsigned long currIdx, std::map<Function *,
                                         std::set<std::pair<Function*, CallInst*>>> &revCallMap,
                                         std::map<Function*, std::set<Value*>> &insertedFacts,
                                         std::set<Function*> &visitedFunctions, std::set<Value*> &childFuncFacts,
                                         Value* &targetGlobalVariable, bool isFuncPtrFact = false, CallInst *targetCallInstr= nullptr);


    // static variable to keep track of the number of
    // local variables that have been inserted.
    static unsigned long localVarNum;

    /***
     * Insert local variable of the given type at the end of the provided basic block
     * @param targetBB Basic block, where the variable needs to be inserted.
     * @param targetType Type of the local variable.
     * @return Pointer to the newly created local variable.
     */
    Value* insertLocalVariable(BasicBlock *targetBB, Type *targetType);

    Value* insertLocalFuncPtrVariable(BasicBlock *targetBB, Type *targetType);

    /***
     * Create a store instruction in the provided basic block.
     * Basically, dstValue = srcValue
     *
     * @param targetBB Basic block where the store has to be inserted.
     * @param srcVal Source value to be stored.
     * @param dstValue  Dst Value where to store.
     * @return True, if the insertion went successful.
     */
    bool insertStoreInstruction(BasicBlock *targetBB, Value *srcVal, Value *dstValue);

    bool insertFuncPtrStoreInstruction(BasicBlock *targetBB, Function *srcVal, Value *dstValue);

    /***
     * Insert a store instruction of type.
     * globalVariable = (void*)srcVal
     * at the end of the provided basic block
     *
     * @param targetBB Basic block where the instruction needs to be inserted.
     * @param srcVal read value.
     * @param dstGlobalVariable Write value.
     * @return True if insertion was successful.
     */
    bool insertStoreToGeneralGlobalVariable(BasicBlock *targetBB, Value *srcVal, Value *dstGlobalVariable);

    /***
     * Get the number of the next local variable to be inserted.
     * @return number of the local variable.
     */
    static unsigned long getLocalVarNum();

    void computeBBDistance(BasicBlock *srcBB, BasicBlock *dstBB);

public:

    double startTime;
    double endTime;
    double totalTime;

    unsigned  long totalPathLength;
    unsigned long totalFactsInserted;

    void startClk() {
        startTime = CLOCK_IN_MS();
    }
    void endClk() {
        endTime = CLOCK_IN_MS();
        totalTime += (endTime - startTime);
    }

    double getTotalTime() {
        return totalTime;
    }

    FactsInserterManager(llvm::Module &currModule,
                         ModuleHelper *currModuleHelper):
                         targetModule(currModule),
                         targetModuleHelper(currModuleHelper) {
        this->totalPathLength = 0;
        this->totalFactsInserted = 0;
        this->totalTime = 0.0;
        this->startTime = 0.0;
        this->endTime = 0.0;
    }


    ~FactsInserterManager() { }

    /***
     * Insert a data ptr fact with the provided sensitivity.
     * @param targetSensitivity Sensitivity of the data pointer fact to be inserted.
     * @return  true if successful else false.
     */
    bool insertDataPtrFact(FACTSENSITIVITY targetSensitivity);

    /***
     * Insert a function ptr fact with the provided sensitivity.
     * @param targetSensitivity Sensitivity of the function pointer fact to be inserted.
     * @return true if successful else false.
     */
    bool insertFuncPtrFact(FACTSENSITIVITY targetSensitivity);

};
#endif //PROJECT_INSERTERHELPER_H
