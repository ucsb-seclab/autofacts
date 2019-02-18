//
// Created by machiry on 12/8/18.
//

#ifndef PROJECT_FACTSTYPE_H
#define PROJECT_FACTSTYPE_H

#include <cstdio>
#include <time.h>

/**
 * Type of facts supported/
 */
typedef enum{
    PTRFACT , /* Data ptr fact*/
    FUNCPTRFACT, /* Function ptr fact */
    MAXFACTTYPE /* INVALID fact type */
} FACTTYPE;

typedef enum {
    NOSENSITIVITY, /*no-sensitive*/
    FLOWSENSITIVE, /*flow-sensitive*/
    PATHSENSITIVE, /*path-sensitive*/
    CONTEXTSENSITIVE, /*context-sensitive*/
    FLOWPATHSENSITIVE, /*flow and path sensitive*/
    FLOWCONTEXTSENSITIVE, /*flow and context sensitive*/
    MAXSENSITIVITY /*Invalid sensitivity*/
} FACTSENSITIVITY;

#define TIMEINTERVAL 1000
#define CLOCK_IN_MS() (clock() / (CLOCKS_PER_SEC / TIMEINTERVAL))

#if defined(FACTSDEBUG)
#define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
#define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif

#define INFO_PRINT(fmt, args...) fprintf(stdout, "INFO: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)

#if defined(ERRORPRINT)
#define ERROR_PRINT(fmt, args...) fprintf(stdout, "ERROR: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
#define ERROR_PRINT(fmt, args...)
#endif

#endif //PROJECT_FACTSTYPE_H
