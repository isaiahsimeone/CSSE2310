#include "player.h"

/**
 * This function is responsible (through calls to other functions) of changing
 * the site that a player occupies (by removing them from their previous and
 * placing them at their new.) Updating their points/Money/sitevisits based
 * on which site they land on and updating the underlying Site occupancy lists
 *
 * It reminans the caller's reponsibility to ensure that both playerID and 
 * siteIndex are valid in terms of being a valid player/being in bounds etc.
 * 
 *
 *      game:   A pointer to a Game structure which contains the Sites and 
 *              player that are being altered by moving
 *  playerID:   The ID of the player that is being moved
 * siteIndex:   The index (in the path) of the site which the player is being
 *              moved to.
 *
 *   Returns:   void
 */
void move_player(struct Game* game, int playerID, int siteIndex) {
    // Remove the specified player from their current site.
    int currentSite = game->players[playerID].site;
    remove_player_from_site(&game->sites[currentSite], playerID);

    handle_site_visit(game, playerID, siteIndex);
    // Place the specified player at the specified new site.
    place_player_at_site(&game->sites[siteIndex], playerID);
    game->players[playerID].site = siteIndex;

}

/**
 * Handles the functionality associated with visiting a site. For
 * a specified player (by their ID) and a specified site index, this
 * function will determine the type of site (Mo, V1, V2, etc) and 
 * change the players points/money/v1sitevists, etc accordingly.
 *
 * It is the responsibility of the calling scope to ensure that a 
 * specified playerID/siteIndex are valid.
 *
 * If a player lands on some sort of invalid site that is not one of
 * the tranditional Mo, V1, V2, Ri, Do, ::. Then this function silently
 * fails and the player is left unmodified.
 *
 *      game:   A pointer to a Game structure which contains the Site and 
 *              player
 *              that are being moved to/moving.
 *  playerID:   The ID of the player that is visiting the specified site.
 * siteIndex:   The index (in the path) of the site which is being visited
 *              by the player.
 *
 *   Returns:   void
 */
void handle_site_visit(struct Game* game, int playerID, int siteIndex) {
    Player player = game->players[playerID];
    int siteType = game->sites[siteIndex].siteType;
    
    switch (siteType) {
        case SITE_MO:
            // Gain 3 money
            player.money += 3;
            break;
        case SITE_V1:
            player.siteV1Visits++;
            break;
        case SITE_V2:
            player.siteV2Visits++;
            break;
        case SITE_DO:
            player.points += player.money / 2;
            player.money = 0;
            break;
        default:
            // Unknown site
            break;

    }
    game->players[playerID] = player;
}

/**
 * Adds a player (by their playerID) to a specified Site.
 *
 * This function will increment the number of occupants at a
 * specified site, and append the given playerID to the end
 * of the site array (so it will be displayed farthest from the path
 * when printed.)
 *
 *     site:   A pointer to the Site structure which the specified
 *             player should be removed from.
 * playerID:   The ID of the player to be removed from the specified site.
 *
 *  Returns:   void
 */
void place_player_at_site(Site* site, int playerID) {
    site->siteOccupants[site->occupantCount] = playerID;
    site->occupantCount++;
}

/**
 * Removes a player (by their playerID) from a specific site.
 * For a specified site structure, this method will iterate
 * through its occupants (playerID array) and remove a matching
 * player ID.
 *
 * This function has undefined behaviour when invoked with
 * a pointer to a site, and a playerID that is not at the
 * specified site. Ensuring that a player does exist at the
 * specified site remains the job of the calling scope.
 *
 *     site:   A pointer to the Site structure which the specified
 *             player should be removed from.
 * playerID:   The ID of the player to be removed from the specified site.
 *
 *  Returns:   void
 */
void remove_player_from_site(Site* site, int playerID) {
    // remove from site occupants list
    int removalIndex = -1;

    for (int i = 0; i < site->occupantCount; i++) {
        if (site->siteOccupants[i] == playerID) {
            removalIndex = i;
            break;
        }
    }

    for (int i = removalIndex; i < site->occupantCount; i++) {
        site->siteOccupants[i] = site->siteOccupants[i + 1];
    }
    site->siteOccupants[site->occupantCount] = -1;
    // remove one of the site occupany count to reflect the 
    // num of elems in the array.
    site->occupantCount--; 
}

/**
 * Calculates the score for a specified player. The players score is
 * determined from the following: Their amount of points, the number
 * of V1 or V2 sites they have visited and the cards they hold according
 * to the following rules:
 *
 *  - Each complete set of ABCDE is worth ten points.
 *  - Each set of four types is worth seven points.
 *  - Each set of three types is worth five points.
 *  - Each set of two types is worth three points.
 *  - A single type is worth one point.
 *
 * Care must be taken by the calling scope to ensure that the specified
 * player has been updated for each V1, V2 site visit (card pickup) and
 * points acquired.
 *
 *  player:   A player Structure for which the score will be calculated.
 *
 * Returns:   An integer being the score calculated for the player.
 */
int get_player_score(Player player) {
    int score = 0;
    // Determine scores from points
    score += player.points;
    // Determine scores from cards (DISTINCT_CARD possible denominations)
    int denominations[DISTINCT_CARDS];
    // How many unique denominations this player holds.
    int size;
    int whichDenomination;
    // Continue to make groupings of cards until the players hand 
    // is exhausted.
    while (!hand_is_empty(denominations, &size, player.hand.cards)) {
        // 10 points for 5 distinct cards, and then 
        // 2 * (#distinct cards) - 1 otherwise.
        score += (size == 5) ? 10 : 2 * size - 1;
        // Denominations contains the types of cards the player holds.
        // Since we just incremented their score based on that holding,
        // subtract them from the players hand/
        for (int i = 0; i < size; i++) {
            whichDenomination = denominations[i];
            player.hand.cards[whichDenomination]--;
        }  
    }
    // Determine scores from sites.
    score += player.siteV1Visits;
    score += player.siteV2Visits;

    return score;
}

/**
 * Determines whether a specified hand of cards is empty or not,
 * Further more, if a hand is found to be NOT empty then the
 * parameter 'denominations' is modified to point to an int array
 * which specifies the types of cards which are present in the hand
 * or '-1' if that card denomination is not in the specified hand.
 * the parameter 'size' will be modified to specify the number of
 * distinct card denominations in the hand. 

 * To be clear by way of example, suppose hand = {2,0,1,0,2}, that is, 
 * 2 A cards, no B cards, 1 C card, no D cards and 2 E cards.
 * Then, denominations = {1, 3, 5, -1, -1} (since the hand has
 * card A = 1, C = 3 and E = 5.) Here, size will be set to 3 (since 
 * we determined this hand had 3 distinct card denominations.)
 *
 * denominations:   A pointer to an int array where card denominations
 *                  that are discovered in 'hand will be recorded into.
 *          size:   The number of distinct denominations found in the
 *                  specified hand.
 *          hand:   The hand to be tested for being empty. (Or to get
 *                  information regarding card denominations are present.)
 *
 *       Returns:   true if the specified hand has no cards (empty)
 *                  false if the specified hand has > 0 cards.
 */
bool hand_is_empty(int* denominations, int* size, int* hand) {
    *size = 0;
    int index = 0;
    for (int i = 1; i < DISTINCT_CARDS; i++) {
        denominations[index] = -1;
        if (hand[i] > 0) {
            (*size)++;
            denominations[index++] = i;
        }
    }
    return *size == 0;
}

/**
 * Prints out to stderr a message displaying the score at the end of the game.
 * Scores are obtained for each player contained within the Game structure
 * pointed to by 'game'. Scores are printed in order of lowest playerID,
 * to highest playerID.
 *
 * Any caller to this function should take care to ensure that A) the game
 * has ended (unless such functionality is desired) and B) That the players
 * contained within the referenced Game structure have been initialised 
 *
 *    game:   A pointer to a Game structure which contains players whose
 *            scores should be determined and displayed.
 *
 * Returns:   Void.
 */
void print_game_over(struct Game* game) {
    fprintf(stderr, "Scores: ");
    for (int player = 0; player < game->playerCount; player++) {
        // Print this players score 
        fprintf(stderr, "%d", get_player_score(game->players[player]));

        // append comma if not the last player
        if (player != game->playerCount - 1) {
            fprintf(stderr, ",");
        }
    }
    // newline following scores
    fprintf(stderr, "\n"); 
}

/**
 * Determines the playerID of the next player that should make their
 * move.
 *
 *    game:   A pointer to a Game structure which will be used to
 *            determine which players turn it is next.
 *
 * Returns:   Void.
 */
int next_player_to_move(struct Game* game) {
    // Iterate over game sites until we reach the first non empty site.
    for (int i = 0; i < game->siteCount; i++) {
        if (game->sites[i].occupantCount > 0) {
            // Then return the latest arriving player to that site
            int latestArrivalIndex = game->sites[i].occupantCount;
            return game->sites[i].siteOccupants[latestArrivalIndex - 1];
        }
    }
    // Cannot get here normally - to satisfy compiler
    return PLAYER_NOT_FOUND;
}