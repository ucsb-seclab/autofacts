//
// Created by machiry on 12/8/18.
//

#include "FactsSelector.h"
#include <cstdlib>
#include <ctime>

#define TEST

void FactsSelector::initializeSelector() {
    srand(time(NULL));
}

FACTTYPE FactsSelector::getNextFactType() {
#ifdef TEST
    return FACTTYPE::FUNCPTRFACT;
#else
    return (FACTTYPE)(((unsigned int)rand()) % FACTTYPE::MAXFACTTYPE);
#endif
}


FACTSENSITIVITY FactsSelector::getNextFactSensitivity() {
#ifdef TEST
    return FACTSENSITIVITY::FLOWPATHSENSITIVE;
#else
    return (FACTSENSITIVITY)(((unsigned int)rand()) % FACTSENSITIVITY::MAXSENSITIVITY);
#endif
}


unsigned long FactsSelector::getRandomUnsignedNum(unsigned long maxVal) {
    return ((unsigned long)rand())%maxVal;
}

