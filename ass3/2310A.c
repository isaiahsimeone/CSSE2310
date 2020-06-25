#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "util.h"
#include "game.h"
#include "player.h"
#include "error.h"

/* Function declarations */
ExitStatus do_game(struct Game* game);
void player_a_move(struct Game* game);
bool site_vacant_with_type(struct Game* game, SiteType type, int siteIndex);

int main(int argc, char** argv) {
    // Verify argument count
    if (argc != 3) {
        return player_error(INVALID_ARG_COUNT);
    }
    // Verify that both player count and ID are numeric
    if (!is_numeric(argv[1])) {
        return player_error(INVALID_COUNT);
    }
    if (!is_numeric(argv[2])) {
        return player_error(INVALID_ID);
    }
    // Already verified to be numeric
    int playerCount = atoi(argv[1]);
    int mainPlayerID = atoi(argv[2]);
    
    if (playerCount < 1) {
        return player_error(INVALID_COUNT);
    }
    if (mainPlayerID < 0 || mainPlayerID >= playerCount) {
        return player_error(INVALID_ID);
    }
    // Argument checking complete. Ready to receive path
    printf("^");
    fflush(stdout);

    // Construct the game
    struct Game* game = prepare_game(mainPlayerID, playerCount);
    if (!game->parseSucceeded) {
        return player_error(INVALID_PATH);
    }
    // Place players at the first site of the path
    initialise_players(game, playerCount);
    // Draw the game path
    draw_game(game, stderr);

    ExitStatus exitCode = do_game(game);
    
    return exitCode;
}

/**
 * Continues running the game whilst reading any messages
 * from stdin and executes them according to what they
 * contain. Continues looping until a Communication error,
 * early game over or game done message is received
 *
 * This function should be invoked once the Game structure pointer
 * which it takes as a parameter has been set up (with players,
 * sites, etc.)
 *
 *    game:   A pointer to the game structure that should be affected
 *            by messages received (e.g. HAP messages should change
 *            this Game structure pointer)
 *    
 * Returns:   An ExitStatus member according to how the game exited
 *            COMMUNICATION_ERROR if there was a communication error
 *            GAME_ENDED_EARLY if the game ended prematurely
 *            Or NORMAL_EXIT if the game ended normally.
 */
ExitStatus do_game(struct Game* game) {
    char message[COMMUNICATION_BUFFER_SIZE];
    // Loop until we receive a game END/DONE message (or comms error)
    while (!feof(stdin)) {   
        // If there is a stdin message available, read it and try match it
        if (read_stream(message, stdin, COMMUNICATION_BUFFER_SIZE)) {
            switch (match_message(message)) {
                case MESSAGE_YOUR_TURN:
                    player_a_move(game);
                    break;
                case MESSAGE_MADE_MOVE:
                    // HAP event message
                    if (!handle_event(game, message)) {
                        return game_exit(COMMUNICATION_ERROR);
                    }
                    break;
                case MESSAGE_EARLY_END:
                    // Game ended early
                    return game_exit(GAME_ENDED_EARLY);
                    break;
                case MESSAGE_GAME_DONE:
                    // Game done
                    print_game_over(game);
                    return game_exit(NORMAL_EXIT);
                    break;
                case MESSAGE_UNKNOWN:
                    // Unknown message - exit with communication error
                    return game_exit(COMMUNICATION_ERROR);
            }
        }
    }
    return game_exit(COMMUNICATION_ERROR);
}

/**
 * Determines what move should be made according to the specifications
 * for a type A player.
 * 1. If the player has money, and there is a Do site in front, go there.
 * 2. If the next site is Mo, go there
 * 3. Otherwise, pick the closest V1, V2 or Barrier (::) site.
 *
 * Once a move is selected, this function will write to stdin 'DOn' where
 * n is the site index of the selected move.
 *
 * This function has undefined behaviour if it is invoked whilst the
 * main player is already at the final barrier site, if this is the case
 * the function will silently return.
 *
 *    game:   A pointer to the game structure that has the sites
 *            which should be considered when making a move.
 *    
 * Returns:   Void.
 */
void player_a_move(struct Game* game) {
    // The mainPlayer is the one requested to make a move
    Player player = game->players[game->mainPlayerID];
    // The site that the mainplayer is currently at
    int currSite = player.site;
    // The site selected by this player A to move to (intially not found)
    int targetSite = SITE_NOT_FOUND;

    // The player has money, and there is a Do site in front of them, go there
    if (player.money > 0 && find_site_before_barrier(game, SITE_DO, currSite)
            != SITE_NOT_FOUND) {
        targetSite = find_site_before_barrier(game, SITE_DO, currSite);
    
    // If the next site is Mo (and there is room), then go there.
    } else if (site_vacant_with_type(game, SITE_MO, currSite + 1)) {
        targetSite = currSite + 1;
    // Pick the closest V1, V2 or ::, then go there.
    } else {
        for (int i = currSite + 1; i < game->siteCount 
                && targetSite == SITE_NOT_FOUND; i++) {
            SiteType type = game->sites[i].siteType;
            if (type == SITE_BARRIER || type == SITE_V2 || type == SITE_V1) {
                if (site_has_room(game->sites[i])) {
                    targetSite = i;
                }
            }
        }
    }
    printf("DO%d\n", targetSite);
    fflush(stdout);
}

/**
 * Determines whether a site of a given type, at a given siteIndex
 * has a vacancy for an additional player
 *
 *      game:   A pointer to the game structure that has the relevant
 *              Site information
 *      type:   A SiteType enumeration member which specifies the type
 *              of site that is being looked for in the target site
 *              e.g. Mo, Do, V1, etc.
 * siteIndex:   The index in the game path of the site which is being
 *              checked for having a vacancy and being of the specified
 *              type.
 *    
 *   Returns:   Void.
 */
bool site_vacant_with_type(struct Game* game, SiteType type, int siteIndex) {
    return (game->sites[siteIndex].siteType == type 
            && site_has_room(game->sites[siteIndex]));
}