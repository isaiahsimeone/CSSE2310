#include "2310dealer.h"

/**
 * A pointer to a Game structure which is accessible from
 * any scope within the program. This variable exists only
 * to allow the signal handler access to Game information
 * (containing players which can be reaped/killed)
 */
struct Game* globalGame;

int main(int argc, char** argv) {
    struct Game* game = NULL;
    globalGame = game;
    // Setup SIGHUP listener
    signal_listener();
    signal(SIGPIPE, SIG_IGN);
    // Validate that the argument count satisfies minimum
    if (argc < 4) {
        return dealer_error(DEALER_ARG_COUNT);
    }
    char* deckFileName = argv[1];
    char* pathFileName = argv[2];
    int playerCount = argc - 3;
    // Read player process names into the array of strings (char**)
    char** processPath = malloc(sizeof(char*) * playerCount);
    for (int i = 3; i < argc; i++) {
        processPath[i - 3] = (char*) malloc(sizeof(char) * strlen(argv[i]));
        strcpy(processPath[i - 3], argv[i]);
    }
    // We expect deck_from_file and read_file_line to set 'valid' to true
    bool valid = false;
    // Read path and deck
    char* deck = deck_from_file(deckFileName, &valid);
    if (!valid) {
        return dealer_error(DEALER_INVALID_DECK);
    }
    // Read the raw path from the given path file
    char* rawPath = read_file_line(pathFileName, &valid);
    if (!valid) {
        return dealer_error(DEALER_INVALID_PATH);
    }
    // prepare dealer game copy
    game = prepare_dealer_game(pathFileName, playerCount);
    // Was the path provided valid? Check the parseSucceeded member
    if (!game->parseSucceeded) {
        return dealer_error(DEALER_INVALID_PATH);
    }
    // Commence the players by placing them at the first site (for our game)
    initialise_players(game, playerCount);
    
    if (!setup_players(game, processPath, rawPath, playerCount)) {
        return dealer_error(START_PLAYER_FAIL);
    }
    // Draw the game path once then enter the game loop (run_game)
    draw_game(game, stdout);
    return dealer_error(run_game(game, deck));
}

/**
 * Performs the running of the game (looping) until either a communication
 * error is encountered, or until all players in the game reach the final
 * barrier site.
 *
 * Before using this function, both arguments 'game' and 'deck' should be
 * initialised and checked for validity. 
 * 
 *
 *    game:   A pointer to a Game Structure which contains the current state
 *            of the game
 *    deck:   A char* (string) that contains the cards that will be drawn
 *            throughout the game
 *
 * Returns:   A member of the ExitStatus enumeration, 
 *            DEALER_COMMUNICATION_ERROR if an error was encountered,
 *            or DEALER_NORMAL_EXIT if the game ended normally
 */
ExitStatus run_game(struct Game* game, char* deck) {
    char message[MESSAGE_BUFFER_SIZE];
    // Keep track of how many cards have been draw from the deck.
    int cardsDrawn = 0;
    while (!is_game_over(game)) {
        int moverID = next_player_to_move(game);
        // Request and read into 'message' the next players move or 
        // end with comms error if no message was able to be retrieved.
        if (!request_next_player_move(game, message)) {
            broadcast_message(game, "%s", "EARLY");
            return DEALER_COMMUNICATION_ERROR;
        }
        // The state of the player before they made their move
        Player playerPreMove = game->players[moverID];
        int newSite = handle_do_message(game, message);
        // if the move is invalid, or the site has a bad format end game
        if (!is_move_valid(game, moverID, newSite) || newSite == BAD_FORMAT) {
            broadcast_message(game, "%s", "EARLY");
            return DEALER_COMMUNICATION_ERROR;
        }
        // The move is valid, action the move
        move_player(game, moverID, newSite);
        // The card that the player will draw (remains CARD_NONE unless
        // the player visits an 'Ri' site)
        Card newCard = CARD_NONE;
        // only draw a card if the player has moved to an RI site.
        if (game->sites[newSite].siteType == SITE_RI) {
            newCard = get_next_card(deck, &cardsDrawn);
        }
        // The state of player now that they have made their move
        Player playerPostMove = game->players[moverID];
        // Construct and broadcast a HAP event message specifying changes
        construct_event_message(game, playerPreMove, playerPostMove, newCard);
        // Draw the game path to the dealers stdout
        draw_game(game, stdout);
    }
    // The game is over, broadcast 'DONE' to all players and display scores
    broadcast_message(game, "%s", "DONE");
    print_scores(game);
    return DEALER_NORMAL_EXIT;
}

/**
 * Determines who the next player to make a move is and then prompts
 * them to make their move (via sending "YT" to their stdin.) We then
 * await the players response to the prompt and expect a message in the
 * form of "DOn" (in the players stdout)
 *
 * The argument 'message' should be declared by the caller in the same scope 
 * as calling this function.
 *
 *    game:   A pointer to a Game Structure which contains the
 *            player being messaged
 * message:   The place to read the players stdout (their DO message) to
 *
 * Returns:   A boolean, true if we were able to send and receive
 *            the "YT" and "DOn" message respectively. False otherwise
 */
bool request_next_player_move(struct Game* game, char* message) {
    // Get the playerID of the player whose turn it is to move
    int nextMover = next_player_to_move(game);
    // Request a move from the player, if we can't get one, return false
    if (!send_player_message(game->players[nextMover], "YT")) {
        return false;
    }
    // Await the players response, if we can't read, then return false
    if (!get_player_message(message, game->players[nextMover], 100)) {
        return false;
    }
    // message sent and move received successfully 
    return true;
}

/**
 * Determines whether a specified move is valid in the terms of
 * the rules of the game. Such as: not bypassing a barrier site,
 * or trying to specify a site that is out of bounds.
 *
 *     game:   A pointer to a Game Structure containing the current state
 *             of the game and positions of the players and site information.
 * playerID:   The id of the player that is making the move to 'newSite'
 *  newSite:   The site that the specified player declared that it is moving
 *             to
 *
 *  Returns:   A boolean, true if the move for the specified player to the
 *             specified site is valid. False otherwise.
 */
bool is_move_valid(struct Game* game, int playerID, int newSite) {
    int currentSite = game->players[playerID].site;

    // Ensure that this site is not out of bounds.
    // Disallow step sizes of 0 or backwards
    if (newSite > game->siteCount || newSite <= currentSite) {
        return false;
    }
    // Ensure that the site they are referencing for their move has capacity
    // Check that the move does not put the player passed a barrier site
    if (!site_has_room(game->sites[newSite]) 
            || newSite > find_next_barrier(game, currentSite)) {
        return false;
    }

    return true;
}

/**
 * Reads a string representing the game deck from a specified file,
 * 'deckFile'. 
 *
 * deckFileName:   The name of the file which the deck is to be read from.
 *        valid:   A boolean pointer. If reading the deck from
 *                 the deck file is successful, valid is set to true,
 *                 false otherwise.
 *
 *      Returns:   void.
 */
char* deck_from_file(char* deckFileName, bool* valid) {
    char* rawDeck = read_file_line(deckFileName, valid);
    if (*valid == false) {
        return "";
    }

    // Check that the file has a length which comprises only from
    // characters which are allowed to appear in a deck.
    if (count_occurrences(rawDeck, "0123456789ABCDE") != strlen(rawDeck)) {
        *valid = false;
        return "";
    }
    // Count the number of digits which (should) be at the deck files start
    int digitCount = count_occurrences(rawDeck, "0123456789");
    for (int i = 0; i < digitCount; i++) {
        if (!isdigit(rawDeck[i])) {
            *valid = false;
            return "";
        }
    }
    // Consume all digits commencing the file (leaving characters only)
    while (isdigit(*(++rawDeck)));

    return rawDeck;
}

/**
 * Prints the final scores of each of the players to the stdout
 * of the dealer process.
 *
 * This function does not calculate player scores itself,
 * see get_player_score in player.c for such a function.
 *
 *    game:   A pointer to the Game structure that should be
 *            considered when calculating player scores.
 *
 * Returns:   void.
 */
void print_scores(struct Game* game) {
    printf("Scores: ");
    for (int i = 0; i < game->playerCount; i++) {
        printf("%d", get_player_score(game->players[i]));
        if (i != game->playerCount - 1) {
            printf(",");
        }
    }
    printf("\n");
}

/**
 * Creates a HAP event message containing the information about the
 * move of the last player.
 *
 * It is the responsibility of the calling scope to ensure that both
 * arguments 'original' and updatedPlayer are as expected prior
 * to using this function, the state of the player (their struct) should
 * be recorded as 'original' prior to calling move_player (or affecting
 * their state in any other way) and then 'updatedPlayer' should be
 * recorded after their move (or state update.)
 *
 *          game:   A pointer to the Game structure that the HAP
 *                  message applies to
 *      original:   A Player structure of the moving player pre-move
 * updatedPlayer:   A Player structure of the moving player POST-move
 *     cardDrawn:   The card that the player has drawn
 *
 *       Returns:   void
 */ 
void construct_event_message(struct Game* game, Player original, 
        Player updatedPlayer, Card cardDrawn) {
    // Calculate the players change in points and money
    int pointsChange = updatedPlayer.points - original.points;
    int moneyChange = updatedPlayer.money - original.money;

    // update total cards drawn by the player (and which card they drew)
    updatedPlayer.hand.cards[cardDrawn]++;
    updatedPlayer.hand.totalCards++;

    // Update the player in the Game Structure
    game->players[updatedPlayer.playerID] = updatedPlayer; 
    // Broadcast the event messages to all players
    broadcast_message(game, "HAP%d,%d,%d,%d,%d\n", updatedPlayer.playerID,
            updatedPlayer.site, pointsChange, moneyChange, cardDrawn);
    // Print out a summary of the players move to the dealers stdout
    print_event_summary(updatedPlayer, stdout);
}

/**
 * Retrieves the next card to be drawn (based on cardIndex)
 * in the given deck.
 *
 *      deck:   A pointer to the Game structure that the do
 *              message should be applied to
 * cardIndex:   An integer pointer: The current number of cards drawn
 *              from the deck
 *
 *   Returns:   A Card enumeration member specifying which card
 *              was drawn (E.g. CARD_A, CARD_B, etc.)
 */ 
Card get_next_card(char* deck, int* cardIndex) {
    int deckLength = strlen(deck);

    Card picked;
    // CardIndex mod Decklength will wrap around back to the first card
    // in the deck once 'deckLength' cards have been drawn.
    switch (deck[*cardIndex % deckLength]) {
        case 'A':
            picked = CARD_A;
            break;
        case 'B':
            picked = CARD_B;
            break;
        case 'C':
            picked = CARD_C;
            break;
        case 'D':
            picked = CARD_D;
            break;
        case 'E':
            picked = CARD_E;
            break;
    }
    (*cardIndex)++;
    return picked;
}

/**
 * Validates and parses a do message received by a player.
 * do message have the form "DOn", where n is some integer
 * denoting the site that a player requested to move to.
 *
 * This function has does not guarantee that what it
 * is parsing is a do message, hence it is the responsibility
 * of the calling scope to ensure that the 'message' that is
 * passed is indeed a DO message.
 *
 *    game:   A pointer to the Game structure that the do
 *            message should be applied to
 * message:   A char* (string) that contains a DO format message
 *
 * Returns:   The site that the player requested to move to 
 *            according to the do message recieved.
 */ 
int handle_do_message(struct Game* game, char* message) {
    if (strlen(message) < strlen("DO") + 1) {
        return BAD_FORMAT;
    }
    if (message[0] != 'D' || message[1] != 'O') {
        return BAD_FORMAT;
    }
    message += 2; // advance past the characters 'DO'
    if (!atoi(message)) {
        return BAD_FORMAT;
    }
    if (atoi(message) > game->siteCount) {
        return BAD_FORMAT;
    }
    // return just the site number (e.g. "DO3234" -> 3234)
    return atoi(message);

}

/**
 * Broadcasts the game Path to each of the players in
 * the game once each of them have indicated that they are ready to 
 * receive it (i.e. if they have sent to their stdout '^'.)
 *
 *  output:   A char* to read from the players stdout to
 *  player:   The player whose stdout should be read
 *    size:   The size of the message to be read
 *            from the players stdout
 *
 * Returns:   A boolean. True if all players received the path 
 *            successfully, false otherwise (e.g. if the player
 *            did not send a message to their stdout.)
 */ 
bool broadcast_path_on_ready(struct Game* game, char* rawPath) {
    char message[MESSAGE_BUFFER_SIZE];

    for (int i = 0; i < game->playerCount; i++) {
        if (get_player_message(message, game->players[i], MESSAGE_READY)) {
            if (message[0] != '^') {
                return false;
            }
            // player ready - send them the path
            send_player_message(game->players[i], rawPath);
        } else {
            // got no message back
            return false;
        }
    }
    return true;
}

/**
 * Read a message from the specified players stdout
 *
 *  output:   A char* to read from the players stdout to
 *  player:   The player whose stdout should be read
 *    size:   The size of the message to be read
 *            from the players stdout
 *
 * Returns:   A boolean. True if sending of the message was
 *            successful, false if the players receive pipe
 *            was invalid for some reason
 */ 
bool get_player_message(char* output, Player player, int size) {
    return read_stream(output, player.pipe.receive, size);
}

/**
 * Send a message to the stdin of a player process.
 *
 * This function has undefined behaviour for excessively large
 * messages and is best suited for sending string literals
 * e.g. "DONE", "EARLY", etc.
 *
 *  player:   The player to send the message to
 * message:   A char* which contains a message to be sent to the
 *            specified player.
 *
 * Returns:   A boolean. True if sending of the message was
 *            successful, false if the players receive pipe
 *            was invalid for some reason
 */ 
bool send_player_message(Player player, char* message) {
    if (player.pipe.send == NULL) {
        return false;
    }
    fprintf(player.pipe.send, "%s\n", message);
    fflush(player.pipe.send);
    return true;
}

/**
 * Broadcasts a message with a given format and unspecified number
 * of arguments. 
 *
 * Function behaviour is similar to fprintf in regards to the
 * format and (...) arguments.
 *
 *          game:   A pointer to a Game structure containing the Process ID's
 *                  of child processes that are to be killed.
 * messageFormat:   The format of the message being sent, e.g.
 *                  "HAP%d,%d,%d,%d,%d" 
 *    arguments*:   The arguments which comprise the messageFormat.
 *       Returns:   void
 */ 
void broadcast_message(struct Game* game, char* messageFormat, ...) {
    va_list args;
    for (int i = 0; i < game->playerCount; i++) {
        va_start(args, messageFormat);
        // Write the message in the specified format to each players 
        // receive pipe
        vfprintf(game->players[i].pipe.send, messageFormat, args);
        // Flush the message immediately
        fflush(game->players[i].pipe.send);
    }
}

/**
 * Kills player processes contained within a specified Game structure.
 *
 *    game:   A pointer to a Game structure containing the Process ID's
 *            of child processes that are to be killed.
 *
 * Returns:   void
 */ 
void destroy_children(struct Game* game) {
    int status;
    if (game != NULL) {
        for (int i = 0; i < game->playerCount; i++) {
            waitpid(game->players[i].pipe.playerPID, &status, WNOHANG);
            if (!WIFEXITED(status)) {
                kill(game->players[i].pipe.playerPID, SIGKILL);
            }
        }
    }
}

/**
 * Prepares and returns an initialised (pointer to) Game Structure
 * containing Site's and Player's to be used in the game.
 *
 * The calling scope is responsible for ensuring that all arguments are valid.
 *
 * pathFileName:   The name of the path file that sites should be constructed
 *                 from.
 *  playerCount:   The number of players in the game
 *
 *      Returns:   A pointer to a Game structure fully initialised
 *                 with Player and Site structures.
 */ 
struct Game* prepare_dealer_game(char* pathFileName, int playerCount) {
    struct Game* game = malloc(sizeof(struct Game));
    // The number of sites in this game
    int siteCount = 0;
    // Was the path recieved from the dealer valid?
    // This will be determined when constructing sites.
    bool pathValid = false;
    // Open the file containing the path to construct sites from.
    FILE* pathFile = fopen(pathFileName, "r");
    if (pathFile == NULL) {
        game->parseSucceeded = false;
        return game;
    }
    game->sites = construct_sites_from_file(pathFile, playerCount, 
            &siteCount, &pathValid);
    fclose(pathFile);
    game->siteCount = siteCount;
    // Prepare players and record them in the Game structure
    game->players = prepare_players(playerCount);
    game->playerCount = playerCount;

    if (pathValid) {
        game->parseSucceeded = true;
    }
    return game;
}

/**
 * Constructs the sites to be used in the game (sites are retrieved directly
 * from a path file.)
 *
 * The calling scope is responsible for ensuring that all arguments are valid.
 *
 *     pathFile:   A FILE* which specifies the pathFile to construct sites
 *                 from.
 *  playerCount:   The number of players in the game
 *    siteCount:   An integer pointer which will be set to contain
 *                 The number of sites constructed from the given path.
 * parseSuccess:   A boolean pointer. Which is set to true if parsing and
 *                 construction of the sites succeeded, false otherwise.
 *
 *      Returns:   A pointer to a Site structure which initiates one
 *                 (or more) Sites that were constructed by the function.
 */ 
Site* construct_sites_from_file(FILE* pathFile, int playerCount, 
        int* siteCount, bool* parseSuccess) {
    *siteCount = read_path_size(pathFile);
    Site* sites = (Site*) malloc(*siteCount * sizeof(Site));
 
    // Check that the sitecount read from the file is valid.
    if (*siteCount == PATH_MALFORMED) {
        *parseSuccess = false;
        return sites;
    }
    int rawPathLength = (*siteCount + 1) * 3 * sizeof(char);
    // Allocate memory for the path and sites
    char* path = (char*) malloc(rawPathLength);
    // Read from the pathFile to 'path'. Or return if the path is malformed
    if (read_path(path, pathFile, *siteCount) == PATH_MALFORMED) { 
        *parseSuccess = false;
        return sites;
    }
    // Iterate over groupings of three characters (sites)
    for (int i = 0; i < strlen(path) - 3; i += 3) {
        sites[i / 3] = parse_site(path, i, playerCount, parseSuccess);
        if (*parseSuccess == false) {
            return sites;
        }
    }
    // Validate that there are at least two sites in the path and
    // that paths start and end with barrier sites.

    *parseSuccess = true;
    return sites;
}

/**
 * Controls the spawning of player child processes by calling
 * spawn_player_process with appropriate arguments/player file
 * names.
 *
 * Once each child process has been spawned successfully, 
 * players are each sent the game path once they have 
 * declared they are ready (i.e. they have sent "^")
 *
 * NOTE: This function does not perform the actual forking/piping
 * /executing of child processes, rather, performs this task
 * for each player in the game. See 'spawn_player_process'
 * for such a function.
 *
 *        game:   An integer specifying the signal code to handle.
 * processPath:   A char** (array of strings) which contains each path
 *                to each respective player process  that should be executed.
 *     rawPath:   The raw path that should be sent to each player
 *                via broadcast_path_on_ready (once each player is ready.)
 * playerCount:   The number of players in the current game.
 * 
 *     Returns:   A boolean. True if spawning and broadcasting of the
 *                game path has been successful, false otherwise.
 */ 
bool setup_players(struct Game* game, char** processPath, char* rawPath, 
        int playerCount) {
    for (int i = 0; i < playerCount; i++) {
        if (!spawn_player_process(game, processPath[i], i, playerCount)) {
            // Spawning child process failed.
            return false; 
        }
    }
    // Broadcast the path to all players once they are ready ("^")
    if (!broadcast_path_on_ready(game, rawPath)) {
        return false;
    }
    return true;
}

/**
 * Spawns a child player process and performs kernel plumbing as
 * required. The child process executes one of the programs
 * specified by 'processName' with the arguments 'playerCount' and
 * 'playerID'.
 *
 * Validity of both the processName to execute by a child and
 * the arguments playerID and playerCount should be validated
 * and checked for correctness by the calling scope.
 *
 *        game:   A pointer to a Game structure which is used to
 *                hold file descriptors to communicate with
 *                the child process that will be spawned.
 * processName:   The name of the file that this child process should
 *                execute after forking and piping is complete
 *    playerID:   The second argument to be passed to the child process
 * playerCount:   The first argument to be passed to the child process
 *
 *     Returns:   True if the spawning, piping and execution of the child
 *                process succeeded, false otherwise.
 */ 
bool spawn_player_process(struct Game* game, char* processName, int playerID, 
        int playerCount) {    
    int send[2];
    int receive[2];
    pid_t processID;
    // Convert program arguments from integers to char*
    char programArg1[integer_length(playerCount)];
    char programArg2[integer_length(playerID)];
    sprintf(programArg1, "%d", playerCount);
    sprintf(programArg2, "%d", playerID);    
    // Ensure that piping did not return an error
    if (pipe(send) < 0 || pipe(receive) < 0) {
        return false; 
    }
    int blackHole = open("/dev/null", O_WRONLY);

    switch (processID = fork()) {
        case 0:
            // Child process
            dup2(send[PIPE_READ], STDIN_FILENO);
            dup2(receive[PIPE_WRITE], STDOUT_FILENO);
            // Suppress child stderr 
            dup2(blackHole, STDERR_FILENO);
            // Close unnecessary pipes
            close(send[PIPE_WRITE]);
            close(send[PIPE_READ]);
            close(receive[PIPE_READ]);
            close(receive[PIPE_WRITE]);
            // Child executes the specified program name with arguments
            execlp(processName, processName, programArg1, programArg2, NULL);
            // We only get here if the above exec fails
            return false;
        case -1:
            // Error when forking
            return false;
        default:
            // Parent process
            close(send[PIPE_READ]);
            close(receive[PIPE_WRITE]);
            // Assign communication information to game structure.
            game->players[playerID].pipe.playerPID = processID;
            game->players[playerID].pipe.receive 
                    = fdopen(receive[PIPE_READ], "r");
            game->players[playerID].pipe.send = fdopen(send[PIPE_WRITE], "w");
            break;
    }
    return true;
}

/**
 * Sets up a listener for a signal sent to the 2310dealer process.
 * If a signal is received, we call handle_hangup (below).
 *    
 * Returns:   void.
 */ 
void signal_listener() {
    struct sigaction hangupAction;
    memset(&hangupAction, 0, sizeof(hangupAction));
    hangupAction.sa_handler = handle_hangup;
    sigaction(SIGHUP, &hangupAction, NULL);
}

/**
 * Used by the above function 'signal_listener' and is executed
 * upon receiving a 'SIGHUP' signal.
 *
 *  signal:   An integer specifying the signal code to handle.
 *    
 * Returns:   void.
 */ 
void handle_hangup(int signal) {
    // If we get a hangup signal, destroy child process and exit
    if (signal == SIGHUP) {
        destroy_children(globalGame);
        exit(DEALER_NORMAL_EXIT);
    }
}