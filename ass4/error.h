#ifndef ERROR_HEADER
#define ERROR_HEADER

#include <stdio.h>

/**
 * Represents error codes that could be 
 * encountered by a control2310 process.
 */
typedef enum {
    CONTROL_NORMAL_EXIT = 0,
    CONTROL_INVALID_ARG_COUNT = 1,
    CONTROL_INVALID_INFO_OR_ID = 2,
    CONTROL_INVALID_PORT_NUMBER = 3,
    CONTROL_ERROR_CONNECTING_MAPPER = 4
} ControlError;

/**
 * Represents error codes that could be 
 * encountered by a roc2310 process.
 */
typedef enum {
    ROC_NORMAL_OPERATION = 0,
    ROC_INVALID_ARG_COUNT = 1,
    ROC_INVALID_MAPPER_PORT = 2,
    ROC_REQUIRE_MAPPER = 3,
    ROC_ERROR_CONNECTING_MAPPER = 4,
    ROC_CANNOT_RESOLVE_PORT = 5,
    ROC_FAILED_TO_CONNECT = 6
} RocError;

/* Function Declarations */
ControlError control_error(ControlError errorType);

RocError roc_error(RocError errorType);

#endif