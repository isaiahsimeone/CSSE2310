#include "error.h" 

/**
 * Prints to stderr and returns the error which is referenced
 * by the errorType argument.
 *
 * errorType:   A member of the ControlError enumeration representing
 *              which type of player error was encountered.
 *
 *   Returns:   The value of errorType.
 */
ControlError control_error(ControlError errorType) {
    const char* errorMessage = "";
    switch (errorType) {
        case CONTROL_INVALID_ARG_COUNT:
            errorMessage = "Usage: control2310 id info [mapper]";
            break;
        case CONTROL_INVALID_INFO_OR_ID:
            errorMessage = "Invalid char in parameter";
            break;
        case CONTROL_INVALID_PORT_NUMBER:
            errorMessage = "Invalid port";
            break;
        case CONTROL_ERROR_CONNECTING_MAPPER:
            errorMessage = "Can not connect to map";
            break;
        case CONTROL_NORMAL_EXIT:
            return CONTROL_NORMAL_EXIT;
    }
    fprintf(stderr, "%s\n", errorMessage);
    return errorType;
}

/**
 * Prints to stderr and returns the error which is referenced
 * by the errorType argument.
 *
 * errorType:   A member of the RocError enumeration representing
 *              which type of player error was encountered.
 *
 *   Returns:   The value of errorType.
 */
RocError roc_error(RocError errorType) {
    const char* errorMessage = "";
    switch (errorType) {
        case ROC_INVALID_ARG_COUNT:
            errorMessage = "Usage: roc2310 id mapper {airports}";
            break;
        case ROC_INVALID_MAPPER_PORT:
            errorMessage = "Invalid mapper port";
            break;
        case ROC_REQUIRE_MAPPER:
            errorMessage = "Mapper required";
            break;
        case ROC_ERROR_CONNECTING_MAPPER:
            errorMessage = "Failed to connect to mapper";
            break;
        case ROC_CANNOT_RESOLVE_PORT:
            errorMessage = "No map entry for destination";
            break;
        case ROC_FAILED_TO_CONNECT:
            errorMessage = "Failed to connect to at least one destination";
            break;
        case ROC_NORMAL_OPERATION:
            return ROC_NORMAL_OPERATION;
    }
    fprintf(stderr, "%s\n", errorMessage);
    return errorType;
}