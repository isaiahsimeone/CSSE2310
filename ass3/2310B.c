#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "util.h"
#include "game.h"
#include "player.h"
#include "error.h"

/* Function declarations */
ExitStatus do_game(struct Game* game);
void player_b_move(struct Game* game);
int get_least_advanced_player(struct Game* game);
bool player_has_most_cards(struct Game* game, Player player);
bool players_have_cards(struct Game* game);
int next_vacant_site(struct Game* game, int fromSite);


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

    return do_game(game);
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
                    player_b_move(game);
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
 * for a type B player.
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
void player_b_move(struct Game* game) {
    // The mainPlayer is the one requested to make a move
    int playerID = game->mainPlayerID;
    Player player = game->players[playerID];

    int currentSite = player.site;
    // The site selected by this player A to move to
    int targetSite = -1;

    // If the next site is not full and all other players are 
    // on later sites than us, move forward one site.
    if (site_has_room(game->sites[currentSite + 1]) 
            && get_least_advanced_player(game) == playerID) {
        targetSite = currentSite + 1;
    // If we have an odd amount of money, and there is a Mo 
    // between us and the next barrier, then go there.
    } else if (player.money % 2 != 0 
            && find_site_before_barrier(game, SITE_MO, currentSite) 
            != SITE_NOT_FOUND) {
        targetSite = find_site_before_barrier(game, SITE_MO, currentSite);
    // If we have the most cards or if everyone has zero cards and 
    // there is a Ri between us and the next barrier, then go there.
    //  Note: “most” means than noone else has as many cards as you do.
    } else if ((player_has_most_cards(game, player) 
            || !players_have_cards(game)) 
            && find_site_before_barrier(game, SITE_RI, currentSite) 
            != SITE_NOT_FOUND) {
        targetSite = find_site_before_barrier(game, SITE_RI, currentSite);  
    // If there is a V2 between us and the next barrier, then go there.
    } else if (find_site_before_barrier(game, SITE_V2, currentSite) 
            != SITE_NOT_FOUND) {
        targetSite = find_site_before_barrier(game, SITE_V2, currentSite);
    // Move forward to the earliest site which has room.
    } else if (next_vacant_site(game, currentSite) != SITE_NOT_FOUND) {
        targetSite = next_vacant_site(game, currentSite);
    }

    // Print the move to stdout
    printf("DO%d\n", targetSite);
    fflush(stdout);
}

/**
 * Determines the playerID of the least advanced player in the game
 * (strictly the least advanced player, i.e. not on a site with
 * any other player, and on the earliest site out of all other players.)
 *
 * Care should be taken by the caller to ensure the validity of arguments.
 *
 *    game:   A pointer to a Game structure which contains
 *            Site structures to be searched.
 *
 * Returns:   The playerID of the least advanced player, or PLAYER_TIE
 *            If the earliest occupied site has > 1 player occupant
 */
int get_least_advanced_player(struct Game* game) {
    for (int i = 0; i < game->siteCount; i++) {
        // Look for the first site from left to right that has an occupant
        if (game->sites[i].occupantCount == 1) {
            return game->sites[i].siteOccupants[0];
        // Is there another player on this site?
        } else if (game->sites[i].occupantCount > 1) {
            return PLAYER_TIE;
        }
    }
    // Cannot get here normally - to satisfy compiler
    return PLAYER_TIE;
}

/**
 * Determines whether a specified player has the most (strictly) cards 
 * in the game.
 *
 * The player specified must be part of the Game structure provided. Ensuring
 * this is the responsibility of the caller.
 *
 *    game:   A pointer to a Game structure which contains the 
 *            players to be searched for cards
 *  player:   The player who is being tested to have the most cards.
 *
 * Returns:   True if the given player has strictly the most cards in the
 *            game, false otherwise.
 */
bool player_has_most_cards(struct Game* game, Player player) {
    int cardCount = player.hand.totalCards;
    int thisPlayer = player.playerID;
    // Check if any player (other than the one passed to this function
    // has the same number or more cards)
    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].hand.totalCards >= cardCount && i != thisPlayer) {
            return false;
        }
    }
    return true;
}

/**
 * Determines whether any players in the game have cards.
 *
 *     game:   A pointer to a Game structure which contains the 
 *             players to be searched for cards
 *
 *  Returns:   True if at least one player has at least one card,
 *             false otherwise.
 */
bool players_have_cards(struct Game* game) {
    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].hand.totalCards > 0) {
            return true;
        }
    }
    return false;
}

/**
 * Determines the next vacant site in a Game path from a given
 * position (fromSite)
 *
 * It is the responsibility of the calling scope to ensure
 * that a given 'fromSite' argument is valid (in bounds, etc.)
 *
 *     game:   A pointer to a Game structure which contains the 
 *             Site structures to be searched for a vacant site.
 * fromSite:   An index in the path to search for the next vacant
 *             site from.
 *
 *  Returns:   The index in the path of the next site that is vacant
 *             for a player to move to.
 */
int next_vacant_site(struct Game* game, int fromSite) {
    for (int i = fromSite + 1; i < game->siteCount; i++) {
        if (site_has_room(game->sites[i])) {
            return i;
        }
    }
    // A vacant site was not found
    return SITE_NOT_FOUND;
}