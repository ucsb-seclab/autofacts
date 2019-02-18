//
// Created by machiry on 12/12/18.
//

#ifndef PROJECT_CONTEXTSENSITIVEFACTS_H
#define PROJECT_CONTEXTSENSITIVEFACTS_H

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/Instructions.h>
#include <iostream>
#include <set>

using namespace llvm;

/***
 * Class that stores all the information about a context-sensitive fact.
 * including its callers, and corresponding call-stack depth.
 */
class ContextSensitiveFact {
public:
    unsigned long contextLen;
    std::set<ContextSensitiveFact*> callerFacts;
    std::set<Value*> insertedFacts;
    std::set<Value*> totalFactValues;
    std::set<Value*> killValues;

    ContextSensitiveFact(unsigned long currLen) {
        this->contextLen = currLen;
        this->callerFacts.clear();
        this->totalFactValues.clear();
        this->killValues.clear();
        this->insertedFacts.clear();
    }

    virtual ~ContextSensitiveFact();
};
#endif //PROJECT_CONTEXTSENSITIVEFACTS_H
