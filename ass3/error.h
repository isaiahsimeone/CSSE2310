#ifndef ERROR_HEADER
#define ERROR_HEADER

#include <stdio.h>

/**
 * Error return values for a player error
 */
typedef enum {
    INVALID_ARG_COUNT = 1,
    INVALID_COUNT = 2,
    INVALID_ID = 3,
    INVALID_PATH = 4
} PlayerError;

/**
 * Exit status' in a 2310A/2310B player
 */
typedef enum {
    NORMAL_EXIT = 0,
    GAME_ENDED_EARLY = 5,
    COMMUNICATION_ERROR = 6
} ExitStatus;

/**
 * Represents return values for a dealer process.
 */
typedef enum DealerError {
    DEALER_NORMAL_EXIT = 0,
    DEALER_ARG_COUNT = 1,
    DEALER_INVALID_DECK = 2,
    DEALER_INVALID_PATH = 3,
    START_PLAYER_FAIL = 4,
    DEALER_COMMUNICATION_ERROR = 5
} DealerError;


/* Function Declarations */
PlayerError player_error(PlayerError errorType);

ExitStatus game_exit(ExitStatus errorType);

DealerError dealer_error(DealerError errorType);

#endif