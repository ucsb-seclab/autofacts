//
// Created by machiry on 12/11/18.
//

#include "ReverseCallGraph.h"

void ReverseCallGraph::computeReverseCallGraph(CallGraphNode *currNode, std::set<CallGraphNode*> &visited) {
    if(visited.find(currNode) == visited.end()) {
        visited.insert(currNode);

        for(auto &currChildNode:*currNode) {
            CallInst *potentialCallInst = nullptr;
            if(currChildNode.first != nullptr) {
                potentialCallInst = dyn_cast<CallInst>(currChildNode.first);
            }
            Function *calledFunction = currChildNode.second->getFunction();
            if(calledFunction != nullptr && !calledFunction->isDeclaration()) {
                if(potentialCallInst != nullptr) {
                    auto toInsPair = std::make_pair(currNode->getFunction(), potentialCallInst);
                    if (this->calleeFunctionMap[calledFunction].find(toInsPair) ==
                        this->calleeFunctionMap[calledFunction].end()) {
                        this->calleeFunctionMap[calledFunction].insert(toInsPair);
                    }
                }
            }
            this->computeReverseCallGraph(currChildNode.second, visited);
        }
    }

}

ReverseCallGraph::ReverseCallGraph(Module &currModule, CallGraph &currCallGraph): targetModule(currModule) {
    std::set<CallGraphNode*> visitedNodes;
    visitedNodes.clear();
    for(auto &currGraphNode: currCallGraph) {
        this->computeReverseCallGraph(currGraphNode.second.get(), visitedNodes);
    }
}

ReverseCallGraph::~ReverseCallGraph() {
    // clean up
    this->calleeFunctionMap.clear();
}

std::map<Function *, std::set<std::pair<Function*, CallInst*>>> &ReverseCallGraph::getCalleeFunctionMap() {
    return this->calleeFunctionMap;
}