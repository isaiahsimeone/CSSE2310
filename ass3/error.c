#include "error.h"

/**
 * Prints to stderr and returns the error which is referenced
 * by the errorType argument.
 *
 * errorType:   A member of the PlayerError enumeration representing
 *              which type of player error was encountered.
 *
 *   Returns:   The value of errorType.
 */
PlayerError player_error(PlayerError errorType) {
    const char* errorMessage = "";
    switch (errorType) {
        case INVALID_ARG_COUNT:
            errorMessage = "Usage: player pcount ID";
            break;
        case INVALID_COUNT:
            errorMessage = "Invalid player count";
            break;
        case INVALID_ID:
            errorMessage = "Invalid ID";
            break;
        case INVALID_PATH:
            errorMessage = "Invalid path";
            break;
    }
    fprintf(stderr, "%s\n", errorMessage);
    return errorType;
}

/**
 * Prints to stderr and returns the error which is referenced
 * by the errorType argument.
 *
 * errorType:   A member of the ExitStatus enumeration representing
 *              which type of exit status was encountered.
 *
 *   Returns:   The value of errorType.
 */
ExitStatus game_exit(ExitStatus errorType) {
    const char* errorMessage = "";
    switch (errorType) {
        case GAME_ENDED_EARLY:
            errorMessage = "Early game over";
            break;
        case COMMUNICATION_ERROR:
            errorMessage = "Communications error";
            break;
        case NORMAL_EXIT:
            return NORMAL_EXIT;
    }
    fprintf(stderr, "%s\n", errorMessage);
    return errorType;  
}

/**
 * Prints to stderr and returns the error which is referenced
 * by the errorType argument.
 *
 * errorType:   A member of the DealerError enumeration representing
 *              which type of dealer error was encountered.
 *
 *   Returns:   The value of errorType.
 */
DealerError dealer_error(DealerError errorType) {
    const char* errorMessage = "";
    switch (errorType) {
        case DEALER_ARG_COUNT:
            errorMessage = "Usage: 2310dealer deck path p1 {p2}";
            break;
        case DEALER_INVALID_DECK:
            errorMessage = "Error reading deck";
            break;
        case DEALER_INVALID_PATH:
            errorMessage = "Error reading path";
            break;
        case START_PLAYER_FAIL:
            errorMessage = "Error starting process";
            break;
        case DEALER_COMMUNICATION_ERROR:
            errorMessage = "Communications error";
            break;
        case DEALER_NORMAL_EXIT:
            return DEALER_NORMAL_EXIT;
    }
    fprintf(stderr, "%s\n", errorMessage);
    return errorType; 
}
