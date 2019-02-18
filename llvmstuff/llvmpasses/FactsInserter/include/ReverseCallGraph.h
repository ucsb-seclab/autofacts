//
// Created by machiry on 12/11/18.
//

#ifndef PROJECT_REVERSECALLGRAPH_H
#define PROJECT_REVERSECALLGRAPH_H

#include <llvm/IR/Module.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/DominanceFrontier.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>

using namespace llvm;

// class that stores the call-graph in reverse order.
class ReverseCallGraph {
private:
    Module &targetModule;

    // This map stores the information about each function
    // and corresponding callees.
    std::map<Function *, std::set<std::pair<Function*, CallInst*>>> calleeFunctionMap;

    /***
     * Recursively compute reverse call-graph from the given call-graph.
     *
     * @param currNode node whose call-graph needs to be computed.
     * @param visited Set of visited CallGraphNodes.
     */
    void computeReverseCallGraph(CallGraphNode *currNode, std::set<CallGraphNode*> &visited);


public:
    ReverseCallGraph(Module &currModule, CallGraph &currCallGraph);

    ~ReverseCallGraph();

    /***
     * Get the function and its callers reverse map.
     *
     * for each function, this map contains the possible callers along with the
     * corresponding call instruction.
     * Note, we do not consider indirect calls.
     *
     * @return Map of function and its corresponding callers.
     */
    std::map<Function *, std::set<std::pair<Function*, CallInst*>>> &getCalleeFunctionMap();



};

#endif //PROJECT_REVERSECALLGRAPH_H
