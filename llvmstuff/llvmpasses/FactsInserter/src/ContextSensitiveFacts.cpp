//
// Created by machiry on 12/14/18.
//

#include "ContextSensitiveFacts.h"

ContextSensitiveFact::~ContextSensitiveFact() {
    // Delete all the children.
    for(auto currC: this->callerFacts) {
        if(currC != nullptr) {
            delete (currC);
        }
    }
    // remove the children.
    this->callerFacts.clear();
}

