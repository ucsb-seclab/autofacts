//
// Created by machiry on 12/8/18.
//

#ifndef PROJECT_FACTSSELECTOR_H
#define PROJECT_FACTSSELECTOR_H

#include "FactsType.h"

// class that helps to select the options for various facts.
class FactsSelector {
public:

    /***
     *  This function initializes the selector configuration.
     */
    static void initializeSelector();

    /***
     *  get next fact type to be generated.
     * @return type of the next fact type.
     */
    static FACTTYPE getNextFactType();

    /***
     *  Get the sensitivity of the next fact type.
     * @return Sensitivity value for the next fact type.
     */
    static FACTSENSITIVITY getNextFactSensitivity();

    /***
     * get a random unsigned number between 0 to maxVal-1.
     * @param maxVal Required upper bound value of the provided number.
     * @return unsigned long.
     */
    static unsigned long getRandomUnsignedNum(unsigned long maxVal);

};

#endif //PROJECT_FACTSSELECTOR_H
