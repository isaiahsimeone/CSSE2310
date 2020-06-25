#include "game.h"

/**
 * For a given site name (e.g. 'Mo', 'Do'), determines the type of
 * site is represented by the site name string. 
 *
 * This function is intended for use with strings of two characters,
 * e.g. 'Mo', 'Do', '::'. NOT 'Mo1', 'Do5', '::-'. I.e, capacity
 * should be omitted from the 'siteName' parameter. If you wish
 * to access the site of a player, use: Player.siteType
 *
 * siteName:   A char* pointing to a site name (e.g. 'Mo', 'Do' etc)
 *             to be matched to a site type
 *
 *  Returns:   SITE_MO if input is "Mo"
 *             SITE_DO if input is "Do"
 *             SITE_V1 if input is "V1"
 *             SITE_V2 if input is "V2"
 *             SITE_RI if input is "Ri"
 *             SITE_BARRIER if input is "::"
 *             SITE_UNKNOWN if none of the above are satisfied.
 */
SiteType get_site_type(char* siteName) {
    if (strcmp(siteName, "Mo") == 0) {
        return SITE_MO;
    } else if (strcmp(siteName, "Do") == 0) {
        return SITE_DO;
    } else if (strcmp(siteName, "V1") == 0) {
        return SITE_V1;
    } else if (strcmp(siteName, "V2") == 0) {
        return SITE_V2;
    } else if (strcmp(siteName, "::") == 0) {
        return SITE_BARRIER;
    } else if (strcmp(siteName, "Ri") == 0) {
        return SITE_RI;
    } else {
        return SITE_UNKNOWN;
    }
}

/**
 * Determines whether a specified Site structure has room for an additional
 * player.
 *
 * This function has undefined behaviour for Site structures which have not
 * been properly initialised. Particularly, this function depends on the 
 * specified site having both a capacity and occupant count defined. Ensuring
 * that these fields are set shall remain the responsibility of the caller.
 *
 *    site:   A site structure to be assessed for vacancy (i.e can another
 *            player land on this site?)
 * 
 * Returns:   true if the site has room for another player, false otherwise.
 */
bool site_has_room(Site site) {
    return (site.siteCapacity - site.occupantCount > 0) ? true : false; 
}

/**
 * Determines the capcity of a site from a path. I.e the number of
 * players which the site has capacity for.
 *
 * This function has undefined behaviour if siteCapacity is not
 * one of 1,2,3,4,5,6,7,8,9,-. Validating this is the task of
 * the calling scope.
 *
 * siteCapacity:   The character (from a site/path) which when cast
 *                 to an integer, denotes the capacity of a site. (OR
 *                 unlimited capacity if siteCapacity = '-'.)
 *  playerCount:   The number of players in the current game. Used so that
 *                 Sites with capacity '-' can be given a capacity
 *                 equal to the number of players in the game (essentially
 *                 unlimited.)
 * 
 *      Returns:   An integer which was either obtained by casting a character
 *                 to an integer, or returning playerCount (if 
 *                 siteCapacity = '-')
 */
int get_site_capacity(char siteCapacity, int playerCount) {
    return (siteCapacity == '-') ? playerCount : char_to_int(siteCapacity);
}

/**
 * Checks to see if a specified char* (string) matches a command
 * that should be handled, e.g. messages such as 'DONE', 'YT', etc
 * should be handled.
 *
 * This function does not guarantee the validity of any message which
 * contains the string 'HAP' at any point, further validation is
 * required for such an aspect. This function can be considered to
 * 'weakly' match HAP event messages.
 *
 * message:   A char* (string) to be matched to see if it is
 *            a command ("EARLY", "DONE", etc).
 * 
 * Returns:   MESSAGE_YOUR_TURN if message is exactly "YT"
 *            MESSAGE_EARLY_END if message is exactly "EARLY"
 *            MESSAGE_GAME_DONE if message is exactly "DONE"
 *            MESSAGE_MADE_MOVE if the message CONTAINS "HAP"
 *            OR MESSAGE_UNKNOWN if a message does not fit any of the
 *            previous criteria
 */
Message match_message(char* message) {
    if (strcmp(message, "YT\n") == 0) {
        return MESSAGE_YOUR_TURN;
    }
    if (strcmp(message, "EARLY\n") == 0) {
        return MESSAGE_EARLY_END;
    }
    if (strcmp(message, "DONE\n") == 0) {
        return MESSAGE_GAME_DONE;
    }
    // If the string contains HAP somewhere
    if (strstr(message, "HAP")) {
        return MESSAGE_MADE_MOVE;
    }
    // The message did not match any of the above
    return MESSAGE_UNKNOWN;
}

/**
 * Reads the path which was sent by the dealer*.
 *
 * This function does not ensure that what it is reading is a
 * valid path. Validation of this is the responsibility of the
 * calling scope.
 *
 * *This function should be called after read_path_size, which
 * is used to determine the number of sites in the path and 
 * removes the number of sites from the start of the path. 
 *
 *      path:   A char pointer which will be written to.
 *    stream:   A file pointer which specifies the stream to read
 *              a path from.
 * siteCount:   The number of sites in this path.
 * 
 *   Returns:   (An integer) PATH_MALFORMED if the path specified
 *              in the path argument is invalid, returns PATH_VALID
 *              otherwise
 */
int read_path(char* path, FILE* stream, int siteCount) {
    fgets(path, (siteCount + 1) * (SITE_NAME_LENGTH + 1) * sizeof(char), 
            stream);

    // A valid path is made of only the characters "123456789-:MoDRiV"
    // If the length of the path does not match the number of occurrences
    // of these characters, then the path is malformed
    if (count_occurrences(path, "123456789:-MoDVRi") != strlen(path) - 1) {
        return PATH_MALFORMED;
    }
    

    // Determine if the path length is valid from the siteCount.
    return PATH_VALID;
}

/**
 * Determines the number of sites in a path received via a specified stream.
 *
 * This function is intended for use when a path is received from
 * a stream, it makes no assurance that what is read is a path
 * and hence ensuring this is the responsibility of the calling
 * scope. 
 *
 * This function expects a path in the form X;::-...::-, where
 * X is some integer, characters from stdin are consumed up to
 * and including the STOP_COUNT_DELIMITER token ';'.
 *
 * If a character which is not an integer is encountered before
 * the STOP_COUNT_DELIMITER is encountered, this function stops
 * consuming characters from the specified stream and returns.
 *
 * If this function is provided a stream which is commenced
 * with greater than STOP_COUNT_MAX_SIZE (10) digits, then
 * this function stops consuming characters and returns PATH_MALFORMED.
 *
 *  stream:   A FILE pointer which specifies the stream from which
 *            to read the size of a path from.
 * 
 * Returns:   The number of sites which are indicated at the
 *            beginning of a path sent which was sent to stdin.
 *            OR PATH_MALFORMED (-1) if the path is malformed such
 *            that an unexpected character is encountered before
 *            a STOP_COUNT_DELIMITER is encountered (';'.)
 */
int read_path_size(FILE* stream) {
    // With this, we allow 
    char pathSize[STOP_COUNT_MAX_SIZE];
    char digit;
    int i = 0;
    // Read and store characters from stdin until STOP_COUNT_DELIMITER (';')
    while ((digit = fgetc(stream)) != STOP_COUNT_DELIMITER) {
        // Digits only are allowed before the Delimiter token (';')
        // If too many digits commence the file, the path is malformed
        if (!isdigit(digit) || i >= STOP_COUNT_MAX_SIZE) {
            return PATH_MALFORMED;
        }

        pathSize[i++] = digit;
    }
    // The size of a path cannot start with 0 (e.g. 034 sites is invalid)

    int numberOfStops = atoi(pathSize);
    return numberOfStops;
}

/**
 * Prints the game to a specified stream. First by printing out the 
 * path (without capacity numbers) and then drawing player locations
 * under the sites which those players are at.
 *
 * The behaviour of this function is undefined for invalid Game
 * structures (and streams). Ensuring that a given game structure 
 * and file pointer are valid is the responsibilty of the caller.
 *
 *    game:   A pointer to a Game structure which should be 
 *            printed out to the specified stream. 
 *  stream:   A file pointer which specifies where the game should
 *            be drawn to, e.g. stderr, stdout, etc.
 *
 * Returns:   Void
 */
void draw_game(struct Game* game, FILE* stream) {
    // Draw the game path itself (followed by a new line)
    for (int i = 0; i < game->siteCount; i++) {
        fprintf(stream, "%s ", game->sites[i].siteName);
    }
    fprintf(stream, "\n");

    int maxDrawDepth = 0;
    // Determine the site with the most players (occupants), (therefore,
    // the maximum depth to draw players down from the path.)
    for (int i = 0; i < game->siteCount; i++) {
        if (game->sites[i].occupantCount > maxDrawDepth) {
            maxDrawDepth = game->sites[i].occupantCount;
        }
    }
    // Draw player IDs below the sites which they are currently at.
    // We have to draw players below the path down to the site with
    // the most players, (maxDrawDepth.)
    for (int i = 0; i < maxDrawDepth; i++) {
        for (int j = 0; j < game->siteCount; j++) {
            // If this site has an occupant, draw their ID under this site
            if (game->sites[j].siteOccupants[i] != NO_OCCUPANT) {
                // Print this player under the site which they are at
                // And add space characters up to the next site.            
                fprintf(stream, "%d  ", game->sites[j].siteOccupants[i]);
            } else {
                // There is no player at this site. So print space characters
                fprintf(stream, "   ");
            }
        }
        fprintf(stream, "\n");
    }
}

/**
 * Prepares the game for this player by creating a Game structure and 
 * initialising it with both site and player information.
 *
 * This function assembles the entire Game structure and has undefined
 * behaviour for subsequent calls. It remains the responsibility of the
 * calling scope to ensure that both the ID of the main player and
 * the number of players in the game (playercount) is valid.
 * 
 * mainPlayerID:   The ID number of the main player (i.e. the player who
 *                 is expected to send moves to the dealer.)
 *  playerCount:   The number of players which are in the game.
 *
 *      Returns:   A pointer to a Game structure which has been initialised
 *                 with site and player information.
 */
struct Game* prepare_game(int mainPlayerID, int playerCount) {
    struct Game* game = malloc(sizeof(struct Game));
    // The number of sites in this game
    int siteCount = 0;
    // Was the path recieved from the dealer valid?
    // This will be determined when constructing sites.
    bool pathValid = false;

    game->mainPlayerID = mainPlayerID;
    // Create sites and set sites member
    game->sites = construct_sites(playerCount, &siteCount, &pathValid);
    game->siteCount = siteCount;
    // Spawn players and record them in the game structure
    game->players = prepare_players(playerCount);
    game->playerCount = playerCount;
    // if parsing succeeded, then the game structure is valid
    if (pathValid) {
        game->parseSucceeded = true;
    }
    return game;
}

/**
 * Constructs the individual sites contained in a path (which will be
 * received from the dealer) as seperate structures. 
 *
 * If the path received is valid, then 'parseSuccess' will be set to 
 * true. False otherwise.
 *
 * It is the responsibility of the calling scope to ensure that all
 * parameters are valid before using this function. 
 *
 * This function has undefined behaviour for subsequent calls and is intended
 * to be called once before the game starting.
 * 
 *  playerCount:   The number of players which are in the game.
 *    siteCount:   An integer pointer which will be set to the number of 
 *                 sites which have been created and are pointed to by the
 *                 Site struct pointer.
 * parseSuccess:   Will be set to true if the path received was parsed
 *                 successfully, false otherwise.
 *
 *      Returns:   A pointer to the first of all of the Site structures
 *                 which were created from the path sent by the dealer.
 */
Site* construct_sites(int playerCount, int* siteCount, bool* parseSuccess) {
    // Consume and record digits from stdin representing the number of 
    // sites in the path that follows.
    // Check that the numbers initiating the path are valid
    *siteCount = read_path_size(stdin);

    // Calculate the number of characters in the site (siteCount and
    // SITE_NAME_LENGTH start at 0, so plus one reflects their true size.)
    int pathSize = (*siteCount + 1) * (SITE_NAME_LENGTH + 1);
    // Allocate memory for the path and sites
    char* path = (char*) malloc(pathSize * sizeof(char));
    Site* sites = (Site*) malloc(*siteCount * sizeof(Site));
    // Read the path from stdin to 'path'. Or return if the path is malformed.
    if (read_path(path, stdin, *siteCount) == PATH_MALFORMED) {
        *parseSuccess = false;
        return sites;
    }
    // Iterate over groupings of three characters (sites)
    for (int i = 0; i < strlen(path) - 3; i += 3) {
        // i = 3n, so i / 3 will yield 1, 2, 3 for i = 3, 6, 9
        sites[i / 3] = parse_site(path, i, playerCount, parseSuccess);
        if (*parseSuccess == false) {
            return sites;
        }
    }
    // Validate that there are at least two sites in the path and
    // that paths start and end with barrier sites.
    if (validate_barriers(sites, *siteCount)) {
        *parseSuccess = false;
        return sites;
    }
    *parseSuccess = true;
    return sites;
}

/**
 * Parses an individual site (from a given path) and returns
 * a Site structure if the site was successfully validated.
 *
 * This function has undefined behaviour for empty path's, indexInPath
 * values which exceed the length of the path, divided by 3 or any other
 * malformed argument. It is the responsibility of the calling scope to 
 * ensure that all parameters are valid before using this function.
 *
 * This function requires that the argument, indexInPath is always a multiple
 * of three. Since indexInPath should reference the start of some site in
 * the given path, and each site starts at a multiple of three.
 *
 *         path:   A char* which specifies the path for this game (and the
 *                 path which contains the site to be passed.)
 *  indexInPath:   An integer specifying the position where this site is
 *                 specified in the path.
 *  playerCount:   The number of players which are in the game.
 * parseSuccess:   Will be set to true if the Site was parsed successfully,
 *                 otherwise, set to false.
 *
 *      Returns:   A Site structure which is fully initialised with the 
 *                 relevant information (otherwise, parseSuccess is false if 
 *                 initialisation of the site failed.)
 */
Site parse_site(char* path, int indexInPath, int playerCount, 
        bool* parseSuccess) {
    Site site;

    // Get display name of site (e.g 'Mo', 'Do', etc.)
    for (int i = 0; i < SITE_NAME_LENGTH; i++) {
        // when iterating over groupings of 3 characters path[indexInPath + 0]
        // and path[indexInPath + 1] make this sites name (e.g. Mo)
        site.siteName[i] = path[indexInPath + i];
    }
    site.siteName[SITE_NAME_LENGTH] = '\0';
    // Get the type of site from its name (e.g. SITE_MO, SITE_BARRIER, etc.)
    site.siteType = get_site_type(site.siteName);
    if (site.siteType == SITE_UNKNOWN) {
        *parseSuccess = false;
        return site;
    }
    // The capacity character following a site name (e.g. the '1' in "Mo1")
    char capacityCharacter = path[indexInPath + 2];
    // If a site has a '-' in place of capacity and it is not a barrier site
    // it is
    if (capacityCharacter == '-' && site.siteType != SITE_BARRIER) {
        *parseSuccess = false;
        return site;
    }
    // If this site is a barrier, then it's capacity character should be '-'
    if (site.siteType == SITE_BARRIER && capacityCharacter != '-') {
        *parseSuccess = false;
        return site;
    }
    site.siteCapacity = get_site_capacity(capacityCharacter, playerCount);
    // Get the index of this site. (i is a multiple of 3.)
    site.siteIndex = indexInPath / 3;
    // Set the number of players at this site to 0.
    site.occupantCount = 0;
    // Set this site to have no occupants (at game start.)
    for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
        site.siteOccupants[i] = NO_OCCUPANT;
    }

    // Add this site to the site collection.
    *parseSuccess = true;
    return site;
}

/**
 * Determine whether the sites that comprise a path are valid
 *
 * The path is not invalid if both the first and last sites
 * are strictly barrier sites (and there are at least two
 * sites in the game)
 *
 * A path is valid if it has at least two sites, and that both the
 * starting and ending sites are barrier sites.
 *
 *     sites:   A pointer to the Site structures comprising the path
 *              to be checked for validity.
 * siteCount:   The number of sites in the array of sites
 *    
 *   Returns:   True if the site is valid, flase otherwise
 */ 
bool validate_barriers(Site* sites, int siteCount) {
    return siteCount < 2 || sites[0].siteType != SITE_BARRIER 
            || sites[siteCount - 1].siteType != SITE_BARRIER;
}

/**
 * Creates and initialises each player which is in the game.
 *
 * This function should be called before the game starting, as players
 * cannot be included once the game has commenced.
 *
 * This function is undefined for playerCounts < 0. Checking that 
 * playerCount > 0 remains the responsibility of the calling scope.
 *
 * Each player is initialised with starting attributes (e.g. 0 score
 * 0 points, 0 total cards, etc.)
 *
 * playerCount:   The number of Player structures to create.
 *    
 *     Returns:   A pointer to 'playerCount' number of Player structures,
 *                each representing a unique player in the game.
 */ 
Player* prepare_players(int playerCount) {
    Player* players = malloc(playerCount * sizeof(Player));

    Player player;
    for (int i = 0; i < playerCount; i++) {
        player.money = PLAYER_INIT_MONEY;
        player.playerID = i;
        player.siteV1Visits = 0;
        player.siteV2Visits = 0;
        player.points = 0;
        // Initialise cards for players (no cards upon initialisation)
        PlayerCards hand;
        for (int j = 0; j < DISTINCT_CARDS; j++) {
            hand.cards[j] = 0;
        }
        hand.totalCards = 0;
        player.hand = hand;
        // Add this player
        players[i] = player;
    }
    return players;
}

/**
 * Places all players in the specified Game structure at the first
 * site in the path. Players are drawn at the first site in the game
 * with the greatest player ID being closest to the path, and the lowest
 * being the furthest away.
 *
 * This function does not construct players, but is intended to
 * place already constructed players at the starting point of the game.
 *
 * This function has undefined behaviour for subsequent calls. Both
 * the Game struct pointer and playerCount should be checked for 
 * validity before calling this function. This is the task of the
 * calling scope.
 *
 *        game:   A pointer to the Game structure to initialise players on.
 * playerCount:   The number of Player structures to create.
 *    
 *     Returns:   void.
 */
void initialise_players(struct Game* game, int playerCount) {
    // Players are initially drawn at the first site in reverse order.
    // So, the lowest playerID will be furthest from the first site.
    for (int i = 0; i < playerCount; i++) {
        game->players[i].site = 0;
        game->sites[0].siteOccupants[i] = playerCount - i - 1;
    }
    // The first site now has 'playerCount' occupants
    game->sites[0].occupantCount = playerCount;
}

/**
 * Parses a HAP event message provided into a GameEvent structure
 * which contains changes to the current game, for a specified player.
 *
 * This function checks the validity of a HAP message and will modify
 * the 'valid' boolean pointer argument accordingly (true = valid, false
 * = invalid.)
 *
 *    game:   A pointer to the Game structure which will be used to
 *            determine the validity of this HAP message in the scope
 *            of the current game.
 * message:   The HAP message to be parsed into a GameEvent structure
 *   valid:   A boolean pointer which will be set to true if the HAP event
 *            message was found to be valid, or false otherwise.
 *    
 * Returns:   A GameEvent structure reflecting the HAP event
 */
GameEvent parse_event_message(struct Game* game, char* message, bool* valid) {
    GameEvent event;
    // Advance past 'HAP' characters in message
    message += 3;
    // Message is invalid unless it reaches the end of this function
    *valid = false;
    // Validate number of comma characters and minus characters. Since proper
    // HAP message's have four commas and only money change can be negative,
    // we expect exactly four commas and no more than one minus sign.
    if (count_occurrences(message, ",") != 4 
            || count_occurrences(message, "-") > 1) {
        return event;
    }
    // The only acceptable characters in a HAP event are ",0123456789-"
    if (count_occurrences(message, ",0123456789-") != strlen(message) - 1) {
        return event;
    }
    // Tokenise HAP message into GameEvent structure
    bool tokeniseSuccess = false;
    event = tokenise_event_message(message, &tokeniseSuccess);
    // If tokenise_event_message failed, return early as invalid
    if (!tokeniseSuccess) {
        return event;
    }
    // Verify player ID exists
    if (event.playerID < 0 || event.playerID >= game->playerCount) {
        return event;
    }
    // Verify site exists
    if (event.newSite < 0 || event.newSite >= game->siteCount) {
        return event;
    }
    // Verify point gain is positive
    if (event.pointChange < 0) {
        return event;
    }
    // If the HAP message has a minus sign, it should only be to signify
    // a negative change in money. If moneyChange is positive but the message
    // contains a minus sign, then the message is improper.
    if (count_occurrences(message, "-") == 1 && event.moneyChange >= 0) {
        return event;
    }
    // Verify card drawn
    if (event.cardDrawn < 0 || event.cardDrawn >= DISTINCT_CARDS) {
        return event;
    }
    *valid = true;
    return event;
}

/**
 * Parses a specified HAP event message into a GameEvent structure
 * reflecting the changes which were broadcast in the HAP message sent
 * via stdin (playerID, new site, point change, etc.)
 *
 * This function does not ensure that what it is tokenising is a HAP
 * message. Care must be taken by the calling scope to ensure that
 * the 'message' being passed to this function is a HAP message.
 * 
 * In a HAP message, the only field which may be negative is the change
 * in the respective players money. If any other field is found to contain
 * a '-' character, then this function returns early with the valid boolean
 * flag set to false (because this is invalid).
 *
 * message:   A character pointer representing the HAP message
 *            to be tokenised and parsed into a GameEvent structure.
 *   valid:   A boolean pointer which can be used to determine
 *            whether the parsing of the HAP message succeeded (true)
 *            or failed (false)
 *    
 * Returns:   A GameEvent structure reflecting the HAP event
 */
GameEvent tokenise_event_message(char* message, bool* valid) {
    GameEvent event;
    // Tokenise the HAP message at each comma character
    char* playerID = strtok(message, ",");
    char* newSite = strtok(NULL, ",");
    char* pointChange = strtok(NULL, ",");
    char* moneyChange = strtok(NULL, ",");
    char* cardDrawn = strtok(NULL, ",");

    // If any HAP message field contains a minus sign (other than money 
    // change) then the message is invalid.
    if (strstr(playerID, "-") || strstr(newSite, "-") 
            || strstr(pointChange, "-") || strstr(cardDrawn, "-")) {
        *valid = false;
    }

    event.playerID = atoi(playerID);
    event.newSite = atoi(newSite);
    event.pointChange = atoi(pointChange);
    event.moneyChange = atoi(moneyChange);
    event.cardDrawn = atoi(cardDrawn);

    *valid = true;
    return event;
}

/**
 * Handles a 'HAP' event message by parsing the HAP event message
 * received from stdin and apply those changes to the game/player.
 *
 * Calls to this function with a valid HAP message will update the
 * site/money/points/V1(V2) site visits, etc for the player ID
 * referenced in the HAP message.
 *
 *    game:   A pointer to the Game structure which will be modified
 *            by the HAP event message.
 * message:   The HAP message to be handled and applied to the game.
 *    
 * Returns:   true if the event message is valid, false otherwise.
 */
bool handle_event(struct Game* game, char* message) {
    // Parse this HAP event message. Return early if the message is invalid.
    bool validMessage = false;
    GameEvent event = parse_event_message(game, message, &validMessage);
    // Was the event parsed successfully?
    if (!validMessage) {
        return false;
    }
    // update our version of the player
    Player player = game->players[event.playerID];
    // Move the references player to their new site
    move_player(game, event.playerID, event.newSite);
    // Update the players money/points/score inline with the HAP message
    player.money += event.moneyChange;
    player.points += event.pointChange;
    player.site = event.newSite;

    // If the player got a new card, increment that card denomination
    // and total cards they hold
    if (event.cardDrawn != 0) {
        player.hand.cards[event.cardDrawn]++;
        player.hand.totalCards++;
    }

    // If the player's new site is on a V1/V2 site, update.
    if (game->sites[event.newSite].siteType == SITE_V1) {
        player.siteV1Visits++;
    } else if (game->sites[event.newSite].siteType == SITE_V2) {
        player.siteV2Visits++;
    }

    // Update this player's new information in the current Game structure
    game->players[event.playerID] = player;

    print_event_summary(player, stderr);
    draw_game(game, stderr);
    return true;
}

/**
 * Prints the summary of a HAP event message for a given player.
 * The message is printed to a specified stream in the format:
 * "Player X Money=X V1=X V2=X Points=X A=X B=X C=X D=X E=X"
 * where X's represent some integers
 *
 *  player:   A player structure whose playerID, money amount,
 *            site V1/V2 visits, points and card counts will be 
 *            printed to the specified stream in the above format
 *  stream:   A file pointer which specifies where the event
 *            summary should be output to (e.g. stderr, stdout, etc.)
 *
 * Returns:   void
 */
void print_event_summary(Player player, FILE* outputStream) {
    fprintf(outputStream, "Player %d ", player.playerID);
    fprintf(outputStream, "Money=%d ", player.money);
    fprintf(outputStream, "V1=%d ", player.siteV1Visits);
    fprintf(outputStream, "V2=%d ", player.siteV2Visits);
    fprintf(outputStream, "Points=%d ", player.points);
    fprintf(outputStream, "A=%d ", player.hand.cards[1]);
    fprintf(outputStream, "B=%d ", player.hand.cards[2]);
    fprintf(outputStream, "C=%d ", player.hand.cards[3]);
    fprintf(outputStream, "D=%d ", player.hand.cards[4]);
    fprintf(outputStream, "E=%d\n", player.hand.cards[5]);
}

/**
 * Determines the index of the next barrier from a specified site
 *
 *       game:   A pointer to a game structure which contains the
 *               set of sites used in this game, which will be searchd
 *               for the next barrier site.
 * startIndex:   The position to start searching for the next barrier from
 *               (not including that site itself.)
 *
 *    Returns:   The index of the next barrier site from 'startIndex'.
 */
int find_next_barrier(struct Game* game, int startIndex) {
    for (int i = startIndex + 1; i < game->siteCount; i++) {
        if (game->sites[i].siteType == SITE_BARRIER) {
            return i;
        }
    }
    return SITE_NOT_FOUND;
}

/**
 * Finds the index of the next site with the specified type which
 * both has a vacancy for an additional player and is before a barrier site.
 *
 * Care should be taken by the caller to ensure the validity of arguments.
 *
 *    game:   A pointer to a Game structure which contains
 *            Site structures to be searched.
 *    type:   A SiteType enumeration member to be searched for.
 *
 * Returns:   The index in the path of the site fitting the above description,
 *            or returns SITE_NOT_FOUND if no such site is found.
 */
int find_site_before_barrier(struct Game* game, SiteType type, int fromSite) {
    for (int i = fromSite + 1; i < game->siteCount; i++) {
        if (game->sites[i].siteType == type && site_has_room(game->sites[i])) {
            return i;
        } else if (game->sites[i].siteType == SITE_BARRIER) {
            return SITE_NOT_FOUND;
        }
    }
    return SITE_NOT_FOUND;
}

/**
 * Determines whether the game is over. The game is over when all
 * players are at the final barrier site.
 *
 *    game:   A pointer to the Game structure that should be
 *            tested to see whether the game is over.
 *
 * Returns:   A boolean, true if the game is over, false
 *            otherwise.
 */
bool is_game_over(struct Game* game) {
    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].site != game->siteCount - 1) {
            return false;
        }
    }
    return true;
}