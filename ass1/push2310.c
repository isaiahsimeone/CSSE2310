#include "push2310.h"

/**
 * Represents a Cell within the playing field
 * Containing both a scoreValue (an integer 0 through 9 
 * inclusive), and a symbol ('.': empty, 'X', 'O').
 */
typedef struct {
    int scoreValue;
    char symbol;
} Cell;

/**
 * Represents the state of the game board containg information
 * including: The symbol of the player whose turn is next ('O', 'X'),
 * whether parsing of the game file was successful, the height
 * and width of the playing field, the scores of both players
 * O and X, and a 2D array of Cell Structures which comprise the
 * complete game field.
 */
struct Game {
    char nextTurn;
    int parseSucceeded;
    int fieldHeight;
    int fieldWidth;
    Cell** cells;
};

int main(int argc, char** argv) {
    /* Check that the argument count is valid. */
    if (argc != 4) {
        fprintf(stderr, "%s", "Usage: push2310 typeO typeX fname\n");
        return INVALID_NUM_ARGS;
    }
    /* Parse user arguments */
    char* playerO = argv[1];
    char* playerX = argv[2];
    char* fileName = argv[3];  
    FILE* gameFile = fopen(fileName, "r");

    /* Check that playerO and playerX's specified type's are valid */
    if (!is_player_type_valid(playerO) || !is_player_type_valid(playerX)) {
        fprintf(stderr, "%s", "Invalid player type\n");
        return INVALID_PLAYER_TYPE;
    }
    /* Check whether the specified game file exists */
    if (!gameFile) {
        fprintf(stderr, "%s", "No file to load from\n");
        return UNABLE_TO_READ_FILE;
    }
    /* Check that the gameFile is syntactically valid and 
     * populate the gameState variable with a Game Structure */
    struct Game gameState = parse_game_file(gameFile);
    if (gameState.parseSucceeded != 1) {
        fprintf(stderr, "%s", "Invalid file contents\n");
        return INVALID_FILE_FORMAT;
    }
    /* Check that the interior of the board has empty cells to play on */
    if (is_game_over(gameState)) {
        fprintf(stderr, "%s", "Full board in load\n");
        return INVALID_BOARD_FULL;
    }
    /* Get player type's for each player (i.e. 'H' or '0' or '1') */
    char playerOType = playerO[0];
    char playerXType = playerX[0];
    /* If the game finishes running and encounters an End of file. */
    if (do_game(gameState, playerOType, playerXType) == FOUND_EOF) {
        fprintf(stderr, "%s", "End of file\n");
        return FOUND_EOF;
    }

    exit(0);
}

/**
 * Calculates a players score based on which cells contained within
 * a Game structure are occupied by the relevant player.
 *
 *    gameState:   A Game Structure which contains the current state of
 *                 the playing field.
 * playerSymbol:   The symbol ('O','X') of the player whose score will
 *                 be calculated.
 *                  
 *      Returns:   An integer which contains the releveant players score.
 */
int get_player_score(struct Game gameState, char playerSymbol) {
    int boardWidth = gameState.fieldWidth;
    int boardHeight = gameState.fieldHeight;

    int score = 0;
    /* Iterate over the interior of the playing field and take sum of
     * the scores which the player occupies. */
    for (int i = 1; i < boardHeight - 1; i++) {
        for (int j = 1; j < boardWidth - 1; j++) {
            if (get_symbol(gameState, i, j) == playerSymbol) {
                score += get_score(gameState, i, j);
            }
        }
    }
    return score;
}

/**
 * Executes moves for the relevant player (i.e. 1, 0 or H).
 * 
 * All arguments should be checked for validitiy prior to making
 * a call to this function. Ensuring that gameState, playerOType and
 * playerXType are valid remains the responsibility of the calling function.
 *
 *   gameState:   A Game Structure which contains the current state of
 *                the playing field.
 * playerOType:   A character specifiying the type of player O, (1, 0 or H.)
 * playerXType:   A character specifiying the type of player X, (1, 0 or H.)
 *          
 *     Returns:   An integer: 1 if the game ran and reached the game over
 *                state successfully.
 *                FOUND_EOF (5) if an End of file was found from stdin.
 */
int do_game(struct Game gameState, char playerOType, char playerXType) {
    char currentPlayer, currentPlayerType;
    currentPlayer = gameState.nextTurn;

    while (1) {
        draw_game_field(gameState);

        /* Determine the type of the player who is moving ('0', '1' or 'H') */
        currentPlayerType = 
                (currentPlayer == 'O') ? playerOType : playerXType;

        /* Mutated by functions below to detect input codes (e.g. EOF) */
        int inputStatus = 0;
        switch(currentPlayerType) {
            case '0':
                /* Type 0 player */
                gameState = do_type_zero_move(gameState, currentPlayer);
                break;
            case '1':
                /* Type 1 player */
                gameState = do_type_one_move(gameState, currentPlayer); 
                break;
            case 'H':
                /* Human player */
                gameState = handle_human_input(gameState, currentPlayer,
                        &inputStatus);
                /* If the status variable mutated by handle_human_input 
                 * equals FOUND_EOF, return it. */
                if (inputStatus == FOUND_EOF) {
                    return FOUND_EOF;
                }
                break;
        }
        /* Select the other player so it's their turn next. */
        currentPlayer = get_opposite_player(currentPlayer);

        /* Game over. Return control to main function. */
        if (is_game_over(gameState)) {
            draw_game_over(gameState);
            return 1;
        }
    }
}

/**
 * Handles input entered by a human player. Determines what type of
 * command was entered by the human, save command or move command. Checks
 * for End of file and reports to calling functions via the inputStatus
 * integer pointer.
 * 
 * Actions the saving of the game if the appropriate command is encountered
 * (e.g. sFILENAME.)
 *
 *     gameState:   A Game Structure which contains the current state of
 *                  the playing field.
 * currentPlayer:   A character representing the symbol of the human player
 *                  who is entering input (being handled by this function.)
 *   inputStatus:   A pointer to an integer type which can be used by the
 *                  calling scope to determine if any special cases
 *                  were encountered. (i.e. mutated to FOUND_EOF, when
 *                  the end of input was found.)
 *                  
 *       Returns:   A game structure having been altered by a move made by
 *                  a human.
 *      modifies:   An integer (inputStatus) outside the scope of this function
 *                  with the return inputStatus of operation. The integer
 *                  variable pointed to by the inputStatus parameter holds: 
 *                  FOUND_EOF if end of file was found when awaiting
 *                  human input, or, 1 if the human input was handled
 *                  without seeing end of file.
 */
struct Game handle_human_input(struct Game gameState, char currentPlayer, 
        int* inputStatus) {
    int commandCode;
    char humanInput[MAX_INPUT_LENGTH];
    char humanInputCopy[MAX_INPUT_LENGTH];
    /* Prompt the human player for input once, and repeat until valid
     * input is received from them. */
    do {
        draw_human_prompt(currentPlayer);
        /* Read human input from stdin. */
        fgets(humanInput, MAX_INPUT_LENGTH, stdin);
        /* If End of file is encountered */
        if (feof(stdin)) {
            *inputStatus = FOUND_EOF;
            return gameState;
        }

        /* Copy humanInput so that it may be mutated by validate_human_input */
        strcpy(humanInputCopy, humanInput);
        /* Determine what type of input was entered (e.g. save or move.) */
        commandCode = validate_human_input(gameState, humanInputCopy);

        /* If the human player entered a save command. */
        if (commandCode == HUMAN_INPUT_SAVE) {
            save_game(gameState, humanInput);
        }
    } while (commandCode != HUMAN_INPUT_VALID);

    /* If the human player entered a valid move command. */
    if (commandCode == HUMAN_INPUT_VALID) {  
        gameState = do_human_move(gameState, humanInput, currentPlayer);
    }

    *inputStatus = 1;
    return gameState;
}

/**
 * Draws the game over message upon being called. Determines the score
 * of both players and constructs a game over message accordingly.
 *
 * gameState:   A Game Structure which contains the current state of
 *              the playing field.
 *                  
 *   Returns:   Void.
 */
void draw_game_over(struct Game gameState) {
    char* winners; 
    
    int playerOScore = get_player_score(gameState, 'O');
    int playerXScore = get_player_score(gameState, 'X');

    /* If the game resulted in a tie */
    if (playerOScore == playerXScore) {
        winners = "O X";
    } else {
        winners = (playerOScore > playerXScore) ? "O" : "X";
    }
    /* Print the field one last time */
    draw_game_field(gameState);
    /* Print winners message */
    printf("Winners: %s\n", winners);
}

/**
 * Saves the current Game to a file specified by a human player.
 *
 * A Cell structure contains packed information about the game. This
 * information must be unpacked to save to a file. A Cell structure
 * consists of a score component, and cell symbol (a stone).
 * All scores (numbers) should be printed to the save file
 * at even positions (from 0), and symbols at odd positions. E.g:
 * "O.3X4.9O.0", all scores are at even positions, symbols are at odd.
 * 
 * A row saved to a file has boardWidth number of scores, and
 * boardWidth number of symbols ('.', 'X', 'O'). plus one for a newline
 * character. Therefore, 2*boardwidth + 1 is the length of a board row
 * in a save file.
 *
 *  gameState:   A Game Structure which contains the current state of the
 *               playing field to be saved.
 * humanInput:   A string which comprises the command entered by 
 *               the human player which was identified as being a 
 *               save command (e.g. ssavegame)
 *
 *    Returns:   Void.
 */
void save_game(struct Game gameState, char* humanInput) {
    int boardWidth = gameState.fieldWidth;
    int boardHeight = gameState.fieldHeight;
    char nextTurn = gameState.nextTurn;
   
    /* Initialise a character array containing the same string as humanInput */
    char* saveName = humanInput;
    /* Advance pointer to second character in human input (omitting 's'). 
     * i.e. human input "ssavegame" becomes the output file name, "savegame" */
    saveName++;
    /* Set ending character to null. */
    saveName[strlen(saveName) - 1] = 0;

    /* Create/open save file in write mode */
    FILE* outputFile = fopen(saveName, "w");
    if (outputFile == NULL) {
        fprintf(stderr, "Save failed\n");
        return;
    }
    
    /* Output board height, width and next turn (on new line) to save file. */
    fprintf(outputFile, "%d %d\n%c\n", boardHeight, boardWidth, nextTurn);

    /* Stores one row of a game board. */
    char row[MAX_LINE_LENGTH];
    /* Output rows are constructed from the current gameState. */
    for (int i = 0; i < boardHeight; i++) {
        /* 'Reset' row string. */
        row[0] = '\0';
        /* 'Select' each column position in the save file and print the
         * appropriate character */
        for (int j = 0; j <= 2 * boardWidth + 1; j++) {    
            if (j % 2 == 0) {
                /* Even positions in a game row are score values (numbers). */
                row[j] = int_to_char(get_score(gameState, i, j / 2));
            } else {
                /* Odd positions in a game board are symbols ('.', 'X', 'O') */
                row[j] = get_symbol(gameState, i, j / 2);
            }
        }
        /* Print the constructed row string to the output file. */
        fputs(row, outputFile);
    }
    /* Close the save file after saving has compelted. */
    fclose(outputFile);
}

/**
 * Places a stone at the specified row-column position, the placed stone
 * will have the symbol specified by the playerSymbol argument.
 *
 * Before calling this function, row, column positions should be checked
 * to be valid. (i.e. not out of bounds/on a corner of the board.)
 *
 * This function does NOT push stones from an edge into the board interior.
 * push_stones_in_dir() is the function for such a task.
 *
 *    gameState:   A Game Structure which contains the current state of the
 *                 playing field.
 * playerSymbol:   The symbol to be placed at the row-column position
 *                  
 *      Returns:   A gamestate which contains the new stone placement.
 */
struct Game place_stone(struct Game gameState, int row, int col,
        char playerSymbol) {
    set_symbol(gameState, row, col, playerSymbol);
    return gameState;
}

/**
 * Performs a type one player move. The type one player will first look for 
 * a push (defensive) move that it can make, if none are available, then the 
 * type one player will search for an aggressive move, i.e., place on the
 * empty cell with the highest value.
 *
 *    gameState:   A Game Structure which contains the current state of the
 *                 playing field.
 * playerSymbol:   The symbol of the type one player who is making the move.
 *                  
 *      Returns:   A gamestate which reflects the move made by the type
 *                 one player.
 */
struct Game do_type_one_move(struct Game gameState, char playerSymbol) {
    int foundMove = 0; 

    /* type_one_defensive_move() will set foundMove to 1 if a defensive move 
     * is found. */
    gameState = type_one_defensive_move(gameState, playerSymbol, &foundMove); 

    if (foundMove == 1) {
        return gameState;
    } else {
        /* No push (defensive) move was found. Place on the 
         * highest value cell instead. */
        return type_one_aggressive_move(gameState, playerSymbol);
    }
} 

/**
 * Performs a type one player push (defensive) move. Type one defensive
 * moves consist of a type one player searching around the board (clockwise)
 * from the top left of the playing field for the first push move which would
 * lower the opponents score.
 *
 *     gameState:   A Game Structure which contains the current state of the
 *                  playing field to search for a push move in.
 *  playerSymbol:   The symbol of the type one player who is initiating the 
 *                  push move.  
 *     foundMove:   A pointer to an integer which is used to report back to
 *                  the calling function whether a suitable push move was
 *                  found for the type one player.
 *                  
 *       Returns:   A gamestate which reflects the push move performed by
 *                  The type one player (if one was found).
 *      modifies:   The foundMove integer pointer. foundMove is set to the 
 *                  value of 1 if a suitable push move was found, or leaves
 *                  it unmodified if no such move was found.
 */
struct Game type_one_defensive_move(struct Game gameState, char playerSymbol,
        int* foundMove) {
    int boardWidth = gameState.fieldWidth;
    int boardHeight = gameState.fieldHeight;

    /* Copy the symbols that are currently on the game board. These
     * symbols will be used for reverting the cell symbols in the Game
     * structure after testing ('simulating') whether a push move lowered
     * the other players score. */
    char originalCells[boardWidth][boardHeight];
    for (int i = 0; i < boardWidth; i++) {
        for (int j = 0; j < boardHeight; j++) {
            originalCells[i][j] = get_symbol(gameState, j, i);
        }
    }
    char enemySymbol = get_opposite_player(playerSymbol);
    int enemyScore = get_player_score(gameState, enemySymbol);

    /* Check the top row (left to right) of the game field for push moves. */
    gameState = type_one_check_top(gameState, playerSymbol, enemySymbol,
            enemyScore, boardHeight, originalCells, foundMove);
    /* If no move was found on the top row, check the right column. */
    if (*foundMove != 1) {
        /* Check right column top to bottom. */
        gameState = type_one_check_right(gameState, playerSymbol, enemySymbol, 
                enemyScore, boardHeight, originalCells, foundMove);
    }
    /* If no move was found in the right column, check the bottom row. */
    if (*foundMove != 1) {
        /* Check bottom row right to left. */
        gameState = type_one_check_bottom(gameState, playerSymbol, enemySymbol,
                enemyScore, boardHeight, originalCells, foundMove);
    }
    /* If no move was found in the bottom row, check the left column. */
    if (*foundMove != 1) {
        /* Check left column bottom to top. */
        gameState = type_one_check_left(gameState, playerSymbol, enemySymbol, 
                enemyScore, boardHeight, originalCells, foundMove);
    }
    return gameState;
}

/**
 * Performs a check of the top row of the playing field to determine
 * whether there exists a push move which would lower the opponent's score.
 * If such a push move does exist, the move is performed on a Game structure
 * and that resulting Game Structure is returned.
 *
 *     gameState:   A Game structure representing the current playing field
 *                  to search for a push move on the top row from.
 *  playerSymbol:   The symbol of the type one player who is searching for a
 *                  push move (and making a push move if able).
 *   enemySymbol:   The symbol of the player who this type one player is
 *                  attempting to disadvantage by way of a push move (The
 *                  symbol is required when calculating the opponent score.)
 *    enemyScore:   The current enemy player's score which is used to compare
 *                  against future enemy score calculations.
 *   boardHeight:   The height of the playing field. Used specifically to 
 *                  determine the size of the originalCells 2D array.
 * originalCells:   A 2D array containing symbols of the Game structure which
 *                  is used to revert the game field after a push move has
 *                  been 'simulated' to check for a valid push move which
 *                  would lower the opponent score (i.e. if no move is found,
 *                  game symbols are reverted and another push move is
 *                  attempted.
 *     foundMove:   An integer pointer which is used by the calling function
 *                  for information on whether a push operation (that lowers 
 *                  opponent score was found/not found.)
 *       
 *       Returns:   A Game structure reflecting a push move made by the type
 *                  one player (if a suitable move was found, i.e.
 *                  lowering the opponent score.) or, the unmodified
 *                  Game structure which was passed into the function (if
 *                  no suitable push move was found.) 
 *      modifies:   An integer pointer (foundMove). If a push move was found
 *                  on the top row of the playing board which would 
 *                  lower the opponent score, foundMove is 1. Otherwise,
 *                  if no suitable move was found, foundMove is 0.
 */
struct Game type_one_check_top(struct Game gameState, char playerSymbol, 
        char enemySymbol, int enemyScore, int boardHeight, 
        char originalCells[][boardHeight], int* foundMove) {
    int boardWidth = gameState.fieldWidth;

    /* Check top row for defensive push move */
    for (int col = 1; col < boardWidth - 1; col++) {
        /* Attempt a push move from the col'th position on row 0 */
        gameState = do_type_one_push(gameState, 0, col, playerSymbol);
        /* If the push move lowered the opponents score */
        if (get_player_score(gameState, enemySymbol) < enemyScore) {
            *foundMove = 1;
            print_auto_player_move(playerSymbol, 0, col);
            return gameState;
        } else {
            /* Revert the symbols in gameState and try pushing from the next
             * column position. */
            gameState = revert_field_cells(gameState, boardHeight, 
                    originalCells);
            *foundMove = 0;
        }
    }
    return gameState;
}


/**
 * Performs a check of the right column of the playing field to determine
 * whether there exists a push move which would lower the opponent's score.
 * If such a push move does exist, the move is performed on a Game structure
 * and that resulting game Structure is returned.
 *
 *     gameState:   A Game structure representing the current playing field
 *                  to search for a push move on the right column from.
 *  playerSymbol:   The symbol of the type one player who is searching for a
 *                  push move (and making a push move if able).
 *   enemySymbol:   The symbol of the player who this type one player is
 *                  attempting to disadvantage by way of a push move (The
 *                  symbol is required when calculating the opponent score.)
 *    enemyScore:   The current enemy player's score which is used to compare
 *                  against future enemy score calculations.
 *   boardHeight:   The height of the playing field. Used specifically to 
 *                  determine the size of the originalCells 2D array.
 * originalCells:   A 2D array containing symbols of the Game structure which
 *                  is used to revert the game field after a push move has
 *                  been 'simulated' to check for a valid push move which
 *                  would lower the opponent score (i.e. if no move is found,
 *                  game symbols are reverted and another push move is
 *                  attempted.
 *     foundMove:   An integer pointer which is used by the calling function
 *                  for information on whether a push operation (that lowers 
 *                  opponent score was found/not found.)
 *       
 *       Returns:   A Game structure reflecting a push move made by the type
 *                  one player (if a suitable move was found, i.e.
 *                  lowering the opponent score.) or, the unmodified
 *                  Game structure which was passed into the function (if
 *                  no suitable push move was found.) 
 *      modifies:   An integer pointer (foundMove). If a push move was found
 *                  on the right column of the playing board which would 
 *                  lower the opponent score, foundMove is 1. Otherwise,
 *                  if no suitable move was found, foundMove is 0.
 */
struct Game type_one_check_right(struct Game gameState, char playerSymbol, 
        char enemySymbol, int enemyScore, int boardHeight, 
        char originalCells[][boardHeight], int* foundMove) {

    /* Check right column for defensive push move */
    for (int row = 1; row < boardHeight - 1; row++) {
        /* Attempt push move from the row'th position on column height - 1 */
        gameState = do_type_one_push(gameState, row, boardHeight - 1, 
                playerSymbol);
        /* If the push move lowered the opponents score */
        if (get_player_score(gameState, enemySymbol) < enemyScore) {
            *foundMove = 1;
            print_auto_player_move(playerSymbol, row, boardHeight - 1);
            return gameState;
        } else {
            /* Revert the symbols in gameState and try pushing from the next
             * row position. */
            gameState = revert_field_cells(gameState, boardHeight, 
                    originalCells);
            *foundMove = 0;
        }
    }
    return gameState;
}

/**
 * Performs a check of the bottom row of the playing field to determine
 * whether there exists a push move which would lower the opponent's score.
 * If such a push move does exist, the move is performed on a Game structure
 * and that resulting game Structure is returned.
 *
 *     gameState:   A Game structure representing the current playing field
 *                  to search for a push move on the bottom row from.
 *  playerSymbol:   The symbol of the type one player who is searching for a
 *                  push move (and making a push move if able).
 *   enemySymbol:   The symbol of the player who this type one player is
 *                  attempting to disadvantage by way of a push move (The
 *                  symbol is required when calculating the opponent score.)
 *    enemyScore:   The current enemy player's score which is used to compare
 *                  against future enemy score calculations.
 *   boardHeight:   The height of the playing field. Used specifically to 
 *                  determine the size of the originalCells 2D array.
 * originalCells:   A 2D array containing symbols of the Game structure which
 *                  is used to revert the game field after a push move has
 *                  been 'simulated' to check for a valid push move which
 *                  would lower the opponent score (i.e. if no move is found,
 *                  game symbols are reverted and another push move is
 *                  attempted.
 *     foundMove:   An integer pointer which is used by the calling function
 *                  for information on whether a push operation (that lowers 
 *                  opponent score was found/not found.)
 *       
 *       Returns:   A Game structure reflecting a push move made by the type
 *                  one player (if a suitable move was found, i.e.
 *                  lowering the opponent score.) or, the unmodified
 *                  Game structure which was passed into the function (if
 *                  no suitable push move was found.) 
 *      modifies:   An integer pointer (foundMove). If a push move was found
 *                  on the bottom row of the playing board which would 
 *                  lower the opponent score, foundMove is 1. Otherwise,
 *                  if no suitable move was found, foundMove is 0.
 */
struct Game type_one_check_bottom(struct Game gameState, char playerSymbol, 
        char enemySymbol, int enemyScore, int boardHeight, 
        char originalCells[][boardHeight], int* foundMove) {
    int boardWidth = gameState.fieldWidth;

    /* Check bottom row for defensive push move */
    for (int col = boardWidth - 1; col > 0; col--) {
        /* Attempt push move from the col'th position on row height - 1 */
        gameState = do_type_one_push(gameState, boardHeight - 1, col, 
                playerSymbol);
        /* If the push move lowered the opponents score */
        if (get_player_score(gameState, enemySymbol) < enemyScore) {
            *foundMove = 1;
            print_auto_player_move(playerSymbol, boardHeight - 1, col);
            return gameState;
        } else {
            /* Revert the symbols in gameState and try pushing from the next
             * column position. */
            gameState = revert_field_cells(gameState, boardHeight, 
                    originalCells);
            *foundMove = 0;
        }
    }     
    return gameState;
}

/**
 * Performs a check of the left column of the playing field to determine
 * whether there exists a push move which would lower the opponent's score.
 * If such a push move does exist, the move is performed on a Game structure
 * and that resulting game Structure is returned.
 *
 *     gameState:   A Game structure representing the current playing field
 *                  to search for a push move on the left column from.
 *  playerSymbol:   The symbol of the type one player who is searching for a
 *                  push move (and making a push move if able).
 *   enemySymbol:   The symbol of the player who this type one player is
 *                  attempting to disadvantage by way of a push move (The
 *                  symbol is required when calculating the opponent score.)
 *    enemyScore:   The current enemy player's score which is used to compare
 *                  against future enemy score calculations.
 *   boardHeight:   The height of the playing field. Used specifically to 
 *                  determine the size of the originalCells 2D array.
 * originalCells:   A 2D array containing symbols of the Game structure which
 *                  is used to revert the game field after a push move has
 *                  been 'simulated' to check for a valid push move which
 *                  would lower the opponent score (i.e. if no move is found,
 *                  game symbols are reverted and another push move is
 *                  attempted.
 *     foundMove:   An integer pointer which is used by the calling function
 *                  for information on whether a push operation (that lowers 
 *                  opponent score was found/not found.)
 *       
 *       Returns:   A Game structure reflecting a push move made by the type
 *                  one player (if a suitable move was found, i.e.
 *                  lowering the opponent score.) or, the unmodified
 *                  Game structure which was passed into the function (if
 *                  no suitable push move was found.) 
 *      modifies:   An integer pointer (foundMove). If a push move was found
 *                  on the left column of the playing board which would 
 *                  lower the opponent score, foundMove is 1. Otherwise,
 *                  if no suitable move was found, foundMove is 0.
 */
struct Game type_one_check_left(struct Game gameState, char playerSymbol, 
        char enemySymbol, int enemyScore, int boardHeight, 
        char originalCells[][boardHeight], int* foundMove) {
        
    /* Check left column for defensive push move */
    for (int row = boardHeight - 1; row > 0; row--) {
        /* Attempt push move from the row'th position on column 0 */
        gameState = do_type_one_push(gameState, row, 0, playerSymbol);
        /* If the push move lowered the opponents score */
        if (get_player_score(gameState, enemySymbol) < enemyScore) {
            *foundMove = 1;
            print_auto_player_move(playerSymbol, row, 0);
            return gameState;
        } else {
            /* Revert the symbols in gameState and try pushing from the next
             * row position. */
            gameState = revert_field_cells(gameState, boardHeight, 
                    originalCells);
            *foundMove = 0;
        }
    }
    return gameState;
}

/**
 * Executes a type one player push move (i.e. places a stone on the exterior)
 * which is then pushed to the interior of the game board.
 *
 * Before using this function, row and column positions should be checked
 * for validity.
 *
 *    gameState:   A Game Structure which contains the current state of the
 *                 playing field.
 *          row:   The row component for a stone to be pushed into the 
 *                 board interior from.
 *          col:   The column component for a stone to be pushed into the 
 *                 board interior from.
 * playerSymbol:   A character containing the symbol of the automated player
 *                 who is placing a stone. ('O' or 'X').
 *                  
 *      Returns:   A modified Game structure reflecting the new push move 
 *                 made by the type one player.
 */
struct Game do_type_one_push(struct Game gameState, int row, int col,
        char playerSymbol) {
    /* Determine the direction for which the push move is valid (if any.) */
    int pushDir = is_push_move(gameState, row, col);
    /* If the push move is valid in some direction and the target cell is
     * empty, intitiate the push move */
    if (pushDir != INVALID_PUSH_MOVE && is_cell_empty(gameState, row, col)) {
        /* Place stone at the specified row column position */
        gameState = place_stone(gameState, row, col, playerSymbol);
        /* Have the stones pushed in the specified direction. */
        gameState = push_stones_in_dir(gameState, row, col, pushDir);
    }
    return gameState;
}

/**
 * Performs a type one player move where a stone is placed on an empty cell
 * with the highest value (an aggressive move). In the case which there
 * are multiple empty cells of the highest value, the first one encountered
 * is placed on from left to right, top to bottom.
 *
 *    gameState:   A Game Structure which contains the current state of the
 *                 playing field.
 * playerSymbol:   The symbol of the type one player who is making the push
 *                 move.  
 *                  
 *      Returns:   A gamestate which reflects the stone placement made by
 *                 The type one player (if one was found).
 */
struct Game type_one_aggressive_move(struct Game gameState,
        char playerSymbol) {
    int boardWidth = gameState.fieldWidth;
    int boardHeight = gameState.fieldHeight;    
    int maxScore = -1;
    /* The coordinates of the highest value cell */
    int rowMax, colMax;

    /* Search for the empty interior cell with the highest value. */
    for (int row = 1; row < boardHeight - 1; row++) {
        for (int col = 1; col < boardWidth - 1; col++) {
            if (is_cell_empty(gameState, row, col) 
                    && get_score(gameState, row, col) > maxScore) {
                /* Found new highest value cell, remember it. */
                maxScore = get_score(gameState, row, col);
                colMax = col;
                rowMax = row;
            }
        }
    }
    /* Place on the highest value cell which was just determined */
    print_auto_player_move(playerSymbol, rowMax, colMax);
    gameState = place_stone(gameState, rowMax, colMax, playerSymbol);
    
    return gameState;
}

/**
 * Gets the score of a cell (at a row/column position). In the provided
 * Game struct.
 *
 * gameState:   A Game Structure which contains the current state of the
 *              playing field.
 *       row:   The row component of the coordinate to get the
 *              respective score from.    
 *       col:   The column component of the coordinate to get the
 *              respective score from.          
 *                  
 *   Returns:   An integer specifying the score of a cell at the specified
 *              row/column position.
 */
int get_score(struct Game gameState, int row, int col) {
    return gameState.cells[col][row].scoreValue;
}

/**
 * Gets the symbol of a cell (at a row/column position). In the provided
 * Game structure.
 *
 * gameState:   A Game Structure which contains the current state of the
 *              playing field.
 *       row:   The row component of the coordinate to get the
 *              respective symbol from.    
 *       col:   The column component of the coordinate to get the
 *              respective symbol from.          
 *                  
 *   Returns:   A character ('.', 'O', 'X') from the specified row/column
 *              position on the game field.
 */
char get_symbol(struct Game gameState, int row, int col) {
    return gameState.cells[col][row].symbol;
}

/**
 * Sets the symbol ('.', 'O', 'X') of a cell (at a row/column position) to the
 * specified character.
 *
 * Before calling this function, row-column positions should be checked to be
 * valid moves (i.e not outside of the playing field/on a corner.)
 *
 * Unexpected game behaviour may occur when attempting to set a symbol at a
 * row column position to a character which is not one of '.', 'O' or 'X'.
 *
 * gameState:   A Game Structure which contains the current state of the
 *              playing field.
 *       row:   The row component of the coordinate to set the
 *              respective symbol at.    
 *       col:   The column component of the coordinate to set the
 *              respective symbol at. 
 *    symbol:   The symbol ('.', 'O', 'X') to be set at the row/column postion.
 *                  
 *   Returns:   Void.
 */
void set_symbol(struct Game gameState, int row, int col, char symbol) {
    gameState.cells[col][row].symbol = symbol;
}

/**
 * Reverts the symbols ('.', 'O', 'X') of a Game structure to those contained
 * within the specified 2D array (originalCells).
 *
 * Unexpected behaviour may occur if the 2d array specified to revert 
 * cell symbols to contains invalid characters. Each character contained
 * within the 2D character array should be one of '.', 'O' or 'X'. Ensuring
 * this remains the burden of the calling scope.
 *
 *     gameState:   A Game Structure which contains the current state of the
 *                  playing field.
 *   boardHeight:   The height of the game board which is used to specify
 *                  The dimension of the 2D array of originalCells.
 * originalCells:   A 2D array containing the symbols of the board which the
 *                  symbols contained in the Game structure will be rolled 
 *                  back to.
 *                  
 *       Returns:   A Game structure whose cell symbols have been rolled back
 *                  to those contained in orginalCells.
 */
struct Game revert_field_cells(struct Game gameState, int boardHeight, 
        char originalCells[][boardHeight]) {
    for (int i = 0; i < gameState.fieldWidth; i++) {
        for (int j = 0; j < gameState.fieldHeight; j++) {
            set_symbol(gameState, j, i, originalCells[i][j]);
        }
    }
    return gameState;
}

/**
 * Executes a type zero player move (i.e. places a stone.) The position on
 * which a stone will be placed depends on the symbol of the type zero player:
 *
 * If the type zero player has playerSymbol O:
 *  The first empty cell ('.') encountered when searching from left to right,
 *  top to bottom will be placed upon.
 * If the type zero player has playerSymbol X:
 *  The first empty cell ('.') encountered when searching from bottom right
 *  to bottom left, bottom to top will be placed upon.
 *
 * To avoid unexpected behaviour, the specified playerSymbol should be checked 
 * to be one of either 'O' or 'X' prior to making a call to this function.
 *
 *    gameState:   A Game Structure which contains the current state of the
 *                 playing field.
 * playerSymbol:   A character containing the symbol of the type zero automated
 *                 player who is placing a stone. ('O' or 'X').
 *                  
 *      Returns:   A modified Game structure reflecting the new move made
 *                 By the type 0 player.
 */
struct Game do_type_zero_move(struct Game gameState, char playerSymbol) {
    int boardWidth = gameState.fieldWidth;
    int boardHeight = gameState.fieldHeight;

    /* If the type zero player is player 'O', */
    if (playerSymbol == 'O') {     
        /* Search the board interior from the top left to bottom right
         * for an empty cell. */
        for (int row = 1; row < boardHeight - 1; row++) {
            for (int col = 1; col < boardWidth - 1; col++) {
                if (is_cell_empty(gameState, row, col)) {
                    /* Place stone on empty row/col position. */
                    gameState = place_stone(gameState, row, col, 'O');
                    /* Print movement result (Player O made move row col). */
                    print_auto_player_move(playerSymbol, row, col);
                    return gameState;
                }
            }
        }
    } else if (playerSymbol == 'X') {
        /* Search board interior bottom right to top left for empty cell. */
        for (int row = boardHeight - 2; row > 0; row--) {
            for (int col = boardWidth - 2; col > 0; col--) {
                if (is_cell_empty(gameState, row, col)) {
                    /* Place stone on empty row/col position. */
                    gameState = place_stone(gameState, row, col, 'X');
                    /* Print movement result (Player X made move row col). */
                    print_auto_player_move(playerSymbol, row, col);
                    return gameState;
                }
            }
        }
    }
    return gameState;
}

/**
 * Prints out the result of an automated player's move in the appropriate 
 * format, for example: "Player O placed at 3 3".
 *
 * playerSymbol:   A character containing the symbol of the automated player
 *                 who just placed. ('O' or 'X').
 *          row:   An integer specifying the row component of the automated
 *                 player's move.
 *          col:   An integer specifying the column component of the
 *                 automated player's move.
 * 
 *      Returns:   void.
 */
void print_auto_player_move(char playerSymbol, int row, int col) {
    printf("Player %c placed at %d %d\n", playerSymbol, row, col);
}

/**
 * Executes a move entered by a human player and updates the Game struct 
 * (gameState).
 *
 * humanInput should be validated to be a valid move (e.g. in bounds, on 
 * an empty cell, not a save command, etc.) prior to making a call to this
 * function.
 *
 *    gameState:   The Game structure to update with the human move (place
 *                 the stone where the human requested and return an updated
 *                 gameState reflecting the new move.
 *   humanInput:   The string of input which was entered by the human player.
 * playerSymbol:   The symbol to use when updating the playing field with the
 *                 new human move.
 *
 *      Returns:   An updated Game structure which contains the new human
 *                 move (i.e. the stone placed on the playing field.)
 */
struct Game do_human_move(struct Game gameState, char* humanInput,
        char playerSymbol) {
    /* split the humanInput string at space into row and column in that order
     * and parse them to integer values. */
    int row = atoi(strtok(humanInput, " "));
    int col = atoi(strtok(NULL, " "));

    /* Place the player's stone where they requested. */
    gameState = place_stone(gameState, row, col, playerSymbol);

    /* Determine whether the player is trying to make a push move. If
     * is_push_move returns INVALID_PUSH_MOVE, they are not. */
    int pushDir = is_push_move(gameState, row, col); 
    if (pushDir != INVALID_PUSH_MOVE) {
        gameState = push_stones_in_dir(gameState, row, col, pushDir);
    } 
    /* Swap which player's turn it is for the next move. */
    gameState.nextTurn = get_opposite_player(playerSymbol);

    return gameState;
}

/**
 * Pushes stones on the playing field from a row-column position and in the
 * direction specified.
 *
 * Row-column parameters are to have been checked for validity before calling
 * this function.
 *
 * Prior to using this function to push stones from a target position,
 * a stone must be present (i.e. a stone symbol at that location.) Ensuring
 * that a stone is placed at that location remains the task of the calling
 * scope.
 *
 * gameState:   The game state structure which will be modified (i.e. stones 
 *              will be pushed) and then returned to the caller.  
 *       row:   The row position from which the push will be performed.
 *       col:   The column position from which the push will be performed. 
 * direction:   The direction to push the stones in (i.e. VALID_PUSH_UP,
 *              VALID_PUSH_DOWN, VALID_PUSH_RIGHT or VALID_PUSH_LEFT).
 *   
 *   Returns:   A gameState which has had the stone push operation performed.
 */
struct Game push_stones_in_dir(struct Game gameState, int row, int col,
        int direction) {
    int boardWidth = gameState.fieldWidth;
    int boardHeight = gameState.fieldHeight;
    int emptyIndex;
    char lastSymbol, nextSymbol;

    switch(direction) {
        case VALID_PUSH_UP: /* Perform a push up from the bottom row */
            /* Get index of the next empty cell in the specified direction. */
            emptyIndex = get_next_empty(gameState, row, col, BOTTOM_TO_TOP);
            /* Shifts symbols from the index of the next empty cell to the 
             * top row of the game board which has the effect of moving all 
             * symbols one up (i.e. pushing up.) */
            for (int i = emptyIndex; i < boardHeight - 1; i++) {
                /* Copies the symbol from (R:i + 1, C:col) to (R:i, C:col) */
                nextSymbol = get_symbol(gameState, i + 1, col);
                gameState = place_stone(gameState, i, col, nextSymbol);
            }
            break;
        case VALID_PUSH_DOWN: /* Perform a push down from the top row */
            emptyIndex = get_next_empty(gameState, row, col, TOP_TO_BOTTOM);
            for (int i = emptyIndex; i > 0; i--) {
                /* Copies the symbol from (R:i - 1, C:col) to (R:i, C:col) */
                lastSymbol = get_symbol(gameState, i - 1, col);
                gameState = place_stone(gameState, i, col, lastSymbol);
            }
            break;
        case VALID_PUSH_RIGHT: /* Perform a push right from the left col */
            emptyIndex = get_next_empty(gameState, row, col, LEFT_TO_RIGHT);
            for (int i = emptyIndex; i > 0; i--) {
                /* Copies the symbol from (R:row, C:i - 1) to (R:row, C:i) */
                lastSymbol = get_symbol(gameState, row, i - 1);
                gameState = place_stone(gameState, row, i, lastSymbol);
            } 
            break;
        case VALID_PUSH_LEFT: /* Perform a push left from the right col */
            emptyIndex = get_next_empty(gameState, row, col, RIGHT_TO_LEFT);
            for (int i = emptyIndex; i < boardWidth - 1; i++) {
                 /* Copies the symbol from (R:row, C:i + 1) to (R:row, C:i) */
                nextSymbol = get_symbol(gameState, row, i + 1);
                gameState = place_stone(gameState, row, i, nextSymbol);
            }
            break;
    }
    /* Set the symbol from where the push initiated to an empty cell */
    gameState = place_stone(gameState, row, col, '.');
    return gameState;
}

/**
 * Searches through a row or column for the index of the next empty cell 
 * ('.'). From a row column position on a specified axis.
 *
 * Row-column parameters are to have been checked for validity prior to
 * making a call to this function (e.g. in bounds, etc.). Row-column positions
 * given to this function should be on an edge of the board to prevent
 * unexpected behaviour.
 *
 * gameState:   The Game structure that the search will be performed on.
 *       row:   The row position of the location to search from.
 *       col:   The column position of the location to search from.
 *      axis:   The axis on which to search the game field from. 
 *              -TOP_TO_BOTTOM (1),
 *              -BOTTOM_TO_TOP (2),
 *              -LEFT_TO_RIGHT (3),
 *              -RIGHT_TO_LEFT (4).
 *              
 *   Returns:   The index of the first empty cell starting from the row/col
 *              position and on the specified axis.
 */
int get_next_empty(struct Game gameState, int row, int col, int axis) {
    int boardHeight = gameState.fieldHeight;
    int boardWidth = gameState.fieldWidth;

    /* Search for the next empty cell from left to right */
    if (axis == LEFT_TO_RIGHT) {
        for (int col = 0; col < boardWidth; col++) {
            if (is_cell_empty(gameState, row, col)) {
                return col;
            }        
        }
    /* Search for the next empty cell from top to bottom */
    } else if (axis == TOP_TO_BOTTOM) {   
        for (int row = 0; row < boardHeight; row++) {
            if (is_cell_empty(gameState, row, col)) {
                return row;
            }
        }
    /* Search for the next empty cell from right to left */
    } else if (axis == RIGHT_TO_LEFT) {   
        for (int col = boardWidth - 1; col > 0; col--) {
            if (is_cell_empty(gameState, row, col)) {
                return col;
            }
        }
    /* Search for the next empty cell from bottom to top */
    } else if (axis == BOTTOM_TO_TOP) {   
        for (int row = boardHeight - 1; row > 0; row--) {
            if (is_cell_empty(gameState, row, col)) {
                return row;
            }
        }
    }
    return 0;
}

/**
 * Determines whether or not a command entered by the a human player 
 * (when being vprompted for input in-game) is syntactically correct/parseable.
 *
 * If human input has been recognised as a move command (i.e. "3 4".) Then
 * the move will be checked to whether it is valid (e.g. in bounds,
 * not on an already occupied cell, etc.)
 *
 *  gameState:   The structure which holds the current game state.
 *               required by this function to test the validity of a move.
 * humanInput:   The string input by the human which will be tested for 
 *               validity.
 *
 *    Returns:   HUMAN_INPUT_VALID, if the string is recognised as
 *               a valid move command (e.g. "1 1".)
 *               HUMAN_INPUT_SAVE, if the user is attempting to save the 
 *               game (human input = 's...').
 *               HUMAN_INPUT_INVALID   Otherwise.
 */
int validate_human_input(struct Game gameState, char* humanInput) {
    /* humanInput should not start with a space character: */
    if (humanInput[0] == ' ') {
        return HUMAN_INPUT_INVALID;
    }
    /* Is the input a save command? A valid save command has the grammar:
     * "s{*}", where * is some character e.g. ("ssavefile".) */
    if(humanInput[0] == 's' && strlen(humanInput) > 2) {
        return HUMAN_INPUT_SAVE;
    }
    /* Since the command was not a save command, if it's valid, it's a move
     * command - Does the input contain a valid number of spaces, and, does 
     * the input consist of less than 3 characters? (including '\n'.) */
    if (count_occurrences(humanInput, " ") != 1 || strlen(humanInput) < 3) {
        return HUMAN_INPUT_INVALID;
    }
    /* Tokenise the humanInput string into row-column strings. */
    char* rowToken = strtok(humanInput, " ");
    char* columnToken = strtok(NULL, " ");   
    /* Is the second argument missing? */
    if (columnToken[0] == '\n') {
        return HUMAN_INPUT_INVALID;
    }
    /* Are the two arguments entered space characters? */
    if (rowToken[0] == ' ' || columnToken[0] == ' ') {
        return HUMAN_INPUT_INVALID;
    }
    /* Are the two arguments entered by the user numeric? */
    if (!is_numeric(rowToken, 0) || !is_numeric(columnToken, 1)) {
        return HUMAN_INPUT_INVALID; 
    }
    /* Parse the tokenised row-column strings to integral values. */
    int rowMove = atoi(rowToken);
    int colMove = atoi(columnToken);   
    /* Check if the move requested by the human player is valid (i.e not on 
     * a cell that is already occupied.) */
    if (is_valid_human_move(gameState, rowMove, colMove)) {
        return HUMAN_INPUT_VALID;
    }
    /* If the above checks fail somehow, the input is invalid. */
    return HUMAN_INPUT_INVALID;
}

/**
 * Determines whether input entered by a human player is valid in terms of
 * the playing field (e.g. not out of bounds, not on an occupied cell, etc.) 
 * 
 * Human input should be checked for validity prior to making a call to this
 * function.
 *
 * gameState:   A Game structure to assess for move validity.
 *       row:   The row component of the human move.
 *       col:   The column component of the human move.
 *
 *   Returns:   An integer, 1 if the move entered by the human player is 
 *              valid. 0 otherwise.
 */
int is_valid_human_move(struct Game gameState, int row, int col) { 
    /* is the move in bounds? */
    if (!is_move_in_bounds(gameState, row, col)) {
        return 0;
    }
    /* is the cell already occupied? */
    if (!is_cell_empty(gameState, row, col)) {
        return 0;
    }
    /* If the row,col position is on an edge, can it be pushed inwards? */
    if (is_edge(gameState, row, col)) {
        if (is_push_move(gameState, row, col) == INVALID_PUSH_MOVE) {
            return 0;
        }
    }
    return 1;
}

/**
 * Determines whether a row-column position is a valid push move, (i.e 
 * when pushing stones into the board interior). If the row/column
 * position is a valid push move, the function determines in which
 * direction the push would be valid (e.g. down, up, etc).
 *
 * gameState:   A Game Structure which contains the current state of
 *              the playing field.
 *       row:   The row component of a position to test for 'pushability'.
 *       col:   The column component of a position to test for 'pushability'.
 *                  
 *   Returns:   An integer specifying the direction for which a push move 
 *              is valid from the row/column position. 
 *              VALID_PUSH_DOWN (4) if a push can be made downwards,
 *              VALID_PUSH_UP (3) if a push can be made upwards,
 *              VALID_PUSH_LEFT (2) if a push can be made left,
 *              VALID_PUSH_RIGHT (1) if a push can be made right,
 *              INVALID_PUSH_MOVE (0) (if no push move can be made.)
 */
int is_push_move(struct Game gameState, int row, int col) {
    int boardHeight = gameState.fieldHeight;
    int boardWidth = gameState.fieldWidth;

    /* (row, col) is on the top most row. Check for push down move. */
    if (row == 0) {
        /* The next cell towards the interior must be empty to allow a push. */
        if (!is_cell_empty(gameState, row + 1, col)) {
            /* Search towards the board interior for the next empty cell. */
            for (int i = 2; i < boardHeight; i++) {
                /* If an empty cell is found, */
                if (is_cell_empty(gameState, i, col)) {
                    return VALID_PUSH_DOWN;
                }
            }
        }
    /* (row, col) is on the bottom most row. Check for push up move. */
    } else if (row == boardHeight - 1) {
        if (!is_cell_empty(gameState, row - 1, col)) {
            for (int i = 0; i < boardHeight - 2; i++) {
                if (is_cell_empty(gameState, i, col)) {
                    return VALID_PUSH_UP;
                }
            }
        }
    /* (row, col) is on the left most row. Check for push right move. */
    } else if (col == 0) {
        if (!is_cell_empty(gameState, row, col + 1)) {
            for (int i = 2; i < boardWidth; i++) {
                if (is_cell_empty(gameState, row, i)) {
                    return VALID_PUSH_RIGHT;
                }
            }
        }
    /* (row, col) is on the right most row. Check for push left move. */
    } else if (col == boardWidth - 1) {
        if (!is_cell_empty(gameState, row, col - 1)) {
            for (int i = 0; i < boardWidth - 2; i++) {
                if (is_cell_empty(gameState, row, i)) {
                    return VALID_PUSH_LEFT;
                }
            }
        }
    }
    return INVALID_PUSH_MOVE;
}

/**
 * Determines whether a specified row-column position is on the
 * edge of the playing field.
 *
 * A position is on an edge, if it is in either: The first row,
 * the last row, the leftmost column or the rightmost column.
 *
 * Row column positions should be checked for validity (e.g. in bounds etc.)
 * this task remains the burden of the calling scope.
 *
 * gameState:   The Game state to check row-column positions against.
 *       row:   The row component to be checked.
 *       col:   The column component to be checked.
 * 
 *   Returns:   1 if the row-col position is on an edge of the field.
 *              0 otherwise.
 */
int is_edge(struct Game gameState, int row, int col) {
    int boardHeight = gameState.fieldHeight;
    int boardWidth = gameState.fieldWidth;

    if (row == 0 || col == 0 || row == boardHeight - 1 
            || col == boardWidth - 1) {
        return 1;
    } 
    return 0;
}

/**
 * Determines whether a given string is numeric (i.e. all characters 
 * in the string are digits).
 *
 *     string:   The string to test numeric status.
 * hasNewline:   1 iff the string contains a character at the end
 *               which can be discarded (e.g. new line '\n'.) Such values
 *               will not be checked for numerical status.
 *               0 if there is no special character present at the end 
 *               of the string.
 *
 *    Returns:   1 iff the string is numeric,
 *               0 otherwise
 */
int is_numeric(char* string, int hasNewline) {
    /* If there is a special character present, do not check it */
    for (int i = 0; i < strlen(string) - hasNewline; i++) {
        if (!isdigit(string[i])) {
            return 0; 
        }
    }
    return 1;
}

/**
 * Counts how many times the characters from a specified string of 
 * characters appears in a specified string. e.g, the characters in the 
 * string "hl" appear in the string "hello world" 4 times.
 *
 *      string:   The string to search for the specified characters.
 * occurrences:   The characters to be counted for each appearance in 
 *                the given string.
 *    
 *     Returns:   The number of times that any of the characters
 *                in 'occurences', appear in the parameter 'string'.
 */ 
int count_occurrences(char* string, char* occurrences) {
    int count = 0;
    /* Go through the string to search, and the characters in occurences */
    for (int i = 0; i < strlen(string); i++) {
        for (int j = 0; j < strlen(occurrences); j++) {
            count += (string[i] == occurrences[j]);
        }       
    }
    return count;
}

/**
 * Determines whether a specified row-column position is within the bounds
 * of the playing field (i.e not greater than or less than the width/height
 * of the field and not on a corner of the gameboard.)
 *
 * gameState:   The Game structure to be compared against.
 *       row:   The row position to test for being in the horizontal width
 *              of the playing field
 *       col:   The column position to test for being in the vertical height
 *              of the playing field.
 *
 *   Returns:   1 if the specified row-col position IS in bounds.
 *              0 otherwise.
 */
int is_move_in_bounds(struct Game gameState, int row, int col) {
    int boardWidth = gameState.fieldWidth;
    int boardHeight = gameState.fieldHeight;
    
    /* Check if the desired move is outside the bounds of the playing field */
    if (row < 0 || row > boardHeight || col < 0 || col > boardWidth) {
        return 0; 
    }

    /* Check if the desired move is on a corner of the playing field */
    if ((row == 0 && col == 0) || (row == 0 && col == boardWidth) 
            || (row == boardHeight && col == 0) || (row == boardHeight &&
            col == boardWidth)) {
        return 0;
    }
    return 1;
}

/**
 * Determines whether a specified cell in a row-column position is
 * an empty cell. A cell is empty if it has the symbol '.'.
 *
 * Row-column positions should be validated to be within the playing field
 * (and in bounds, etc) prior to making a call to this function.
 *
 * gameState:   The game state whose row-col position will be checked
 *              as to whether it is an empty cell or not.
 *       row:   The row component to test for.
 *       col:   The column component to test for.
 *
 *   Returns:   1 if the cell is empty,
 *              0 otherwise.
 */ 
int is_cell_empty(struct Game gameState, int row, int col) {
    if (get_symbol(gameState, row, col) != '.') {    
        return 0;
    }
    return 1;
}

/**
 * Draw's to stdout, a prompt for a human player of the form '?:(R C)>',
 * where ? represents a playerSymbol ('O', 'X')
 *
 * playerSymbol:   The character to be included in the human prompt 
 *                 ('O' or 'X' only).
 *
 *      Returns:   void.
 */
void draw_human_prompt(char playerSymbol) {
    printf("%c:(R C)> ", playerSymbol);
}

/**
 * Draws the game field(/board) to stdout.
 *
 * gameState:   A Game structure representing the current gameState and
 *              hence the current state of the playing field to be drawn
 *              to stdout.
 *
 *   Returns:   Void.
 */ 
void draw_game_field(struct Game gameState) {
    int fieldHeight = gameState.fieldHeight;
    int fieldWidth = gameState.fieldWidth;
    /* Iterate through each cell in the board and draw the relevant score or
     * symbol. */
    for (int row = 0; row < fieldHeight; row++) {
        for (int col = 0; col < fieldWidth + 1; col++) {
            /* Select the cell at the row, column position. */
            Cell cell = gameState.cells[col][row];
            /* If the score stored in this Cell is the decimal value of a
             * Space character (-16), print a space character. */
            if (cell.scoreValue == DECIMAL_SPACE_CHAR) {
                printf("  ");
            /* If the score stored in this Cell is the decimal value of a
             * newline character (-38), print a space character. */
            } else if(cell.scoreValue == DECIMAL_NEWLINE_CHAR) {
                printf("\n");
            } else {
                /* Print a regular cell (e.g. "1.", "3X", etc) */
                printf("%d%c", cell.scoreValue, cell.symbol);        
            }
        }
    }   
}

/**
 * Determines whether a specified string representing a player type is 
 * valid, i.e., the playerType should be one of '0', '1' or 'H'.
 *
 * playerType:   A string containing a single character which 
 *               should specify the type of player ('0', '1', 'H'.)
 *
 *    Returns:   1 if the playerType is valid,
 *               0 otherwise
 */
int is_player_type_valid(char* playerType) {
    char type = playerType[0];
    // Ensure that the specified player type is only one character long.
    if (strlen(playerType) > 1) {
        return 0;
    }
    // Ensure the player type is a known type (one of '1', '0' or 'H')
    if (type != '1' && type != '0' && type != 'H') {
        return 0;
    }
    return 1;
}

/**
 * Determines and returns the opposite player symbol of the one provided.
 *
 * playerSymbol:   The Symbol (a character 'O', 'X') representing the player
 *                 which should be 'negated'.
 *
 *      Returns:   'O' if the playerSymbol specified is 'X',
 *                 'X' if the playerSymbol specified is 'O'.
 */
char get_opposite_player(char playerSymbol) {
    return (playerSymbol == 'X') ? 'O' : 'X';
}

/**
 * Parses and constructs a Game structure from a specified save game file.
 * If it is determined to be a valid save game.
 *
 * gameFile:   A pointer to a file containing a valid(/potential) save game.
 *
 *  Returns:   A Game structure which represents the current playing 
 *             field state (e.g. cell information, width, height, etc).
 *             If the specified save game file is found to be invalid, a
 *             Game structure is returned containing only one member which
 *             specifies that the parse was not successful.
 */
struct Game parse_game_file(FILE* gameFile) {    
    struct Game gameState;
    /* game file is invalid. Do not attempt to parse and return. with the
     * parseSucceeded member set to 0 - indicating a failure to parse. */
    if (!validate_game_file(gameFile)) {
        gameState.parseSucceeded = 0;
        return gameState;
    }
    char lineBuffer[MAX_LINE_LENGTH];
    /* Get the file line containing the board dimensions (1st line.) */
    char* boardDimensions = fgets(lineBuffer, MAX_LINE_LENGTH, gameFile);
    /* Get board dimensions as integral values. */
    int boardHeight = atoi(strtok(boardDimensions, " "));
    int boardWidth = atoi(strtok(NULL, " "));
    /* Assign Height and width to Game structure. */
    gameState.fieldHeight = boardHeight;
    gameState.fieldWidth = boardWidth;
    /* Get the line containing the player whose turn it is next. (2nd line) */
    char* nextTurnPlayer = fgets(lineBuffer, MAX_LINE_LENGTH, gameFile);
    gameState.nextTurn = (char)nextTurnPlayer[0];

    /* Allocate memory for 2D array of Cell structures. */
    Cell** fieldCells = (Cell**) malloc(boardWidth * sizeof(Cell*));
    for (int i = 0; i < boardWidth + 1; i++) {            
        fieldCells[i] = (Cell*) malloc(boardHeight * sizeof(Cell));
    } 

    /* Build game field Cell structures. */
    Cell cell; 
    for (int i = 0; i < boardHeight; i++) {
        /* Get ith board row for parsing. */
        char* row = fgets(lineBuffer, MAX_LINE_LENGTH, gameFile);
        /* We expect a board row to have 2 * boardWidth characters, but since
         * a single cell structure takes a symbol AND a score value, we can
         * iterate over the row in blocks of 2 and 'pack' the information
         * into a single cell structure (even column positions have score
         * values, and odd column positions have symbols ('.', 'O', 'X'.) */
        for (int j = 0; j < strlen(row); j += 2) {
            cell.scoreValue = char_to_int(row[j]);
            cell.symbol = row[j + 1];
            fieldCells[j / 2][i] = cell;
        }
    }
    /* Assign the constructed cells to the Game structure and report that
     * the parsing of the game file was successful */
    gameState.cells = fieldCells;
    gameState.parseSucceeded = 1;
    return gameState;
}

/**
 * Converts a given character into it's integer equivalent. E.g.:
 * '3' -> 3, '0' -> 0, etc.
 *
 * The character 'charNumeral' must be a character 0 through to 9 inclusive.
 * Ensuring that charNumeral is an integer is the task of the calling scope.
 * 
 * charNumeral:   The character to be converted to its integer equivalent.
 *
 *     Returns:   An integer value of the number specified in charNumeral.
 */
int char_to_int(char charNumeral) {
    return (int)(charNumeral - '0');
}

/**
 * Converts a given integer into it's character equivalent. E.g.:
 * 3 -> '3', 0 -> '0', etc.
 * 
 *  number:   The number to be converted to its character equivalent.
 *
 * Returns:   A character containing the character form of number.
 */
char int_to_char(int number) {
    return (char)(number + '0');
}

/**
 * Validates a given game file in terms of contained data (e.g. ensuring that
 * board width/board height/player type values are valid and formatted in
 * the correct format.) Validates that a game board contains only valid 
 * characters in the set:
 * {' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'X', 'O', '.', '\n'}
 * 
 * gameFile:   A pointer to the game file, the file position indicator for
 *             the file stream must commence at the start of the file.
 *
 *  Returns:   1 if the file is a valid game file,
 *             0 otherwise.
 */
int validate_game_file(FILE* gameFile) {
    /* Check the potential game file for invalid characters. */
    if (has_invalid_characters(gameFile)) {
        return 0;
    }
    /* Count the lines contained in the potential game file. */
    int numberOfLines;
    numberOfLines = count_lines(gameFile);
    
    /* Validate the first line of the potential game file (board dimensions) 
     * and get the board dimensions from validate_dimensions_line. */
    int boardDimensions[2];
    if (!validate_dimensions_line(gameFile, boardDimensions)) {
        return 0;
    }
    int boardHeight = boardDimensions[0];
    int boardWidth = boardDimensions[1];
    
    /* Validate the line containing the symbol of the player whose turn it 
     * is next*/
    if (!validate_next_player_line(gameFile)) {
        return 0;
    }
    /* Check the rows of the game board for validity. */
    if (!validate_board_rows(gameFile, boardWidth, boardHeight)) {
        return 0;
    }
    /* The file position indicator of the 'gameFile' stream should be set
     * to just after the final row of the gameBoard. This comes as a result
     * of calling validate_board_rows with 'gameFile'. If the next character
     * in the gameFile is not EOF, then there are characters following the last
     * row of the game board. And hence the save game file is invalid */
    if (fgetc(gameFile) != EOF) {
        return 0;
    }

    /* Check the file for validity: There should be 'boardHeight' game board
     * rows, + 1 new line at file end, + 1 line for storing board dimensions
     * and + 1 for storing next player turn. */
    if (numberOfLines != boardHeight + 3) {
        return 0;
    }

    /* Reset file stream indicator to beginning of file for other functions. */
    rewind(gameFile);
    return 1; 
}

/**
 * Determines whether a potential game file contains board dimensions (i.e.,
 * the top line of a File) is valid. Specifically, the board dimensions line
 * in a game file is valid if it contains two integers, separated by one 
 * single space character.
 *
 *        gameFile:   A pointer to the game file, the file position indicator 
 *                    for the file stream must commence at the first line of
 *                    the file.
 * boardDimensions:   An integer pointer specifying the first element of an
 *                    integer array.
 *
 *         Returns:   1 if the line of the file specifying board dimensions 
 *                    is valid. 0 otherwise.
 *        modifies:   An integer array pointed to by the 'boardDimensions'
 *                    parameter. The first element of this array is populated
 *                    the height of the board contained within the game file
 *                    while the second position is populated with its width.
 */
int validate_dimensions_line(FILE* gameFile, int* boardDimensions) {
    char lineBuffer[MAX_LINE_LENGTH];
    /* Get file line containing board dimensions (first line.) */
    char* boardDimensionsLine = fgets(lineBuffer, MAX_LINE_LENGTH, gameFile);
    /* Ensure one space is on the dimensions line of the file */
    if (count_occurrences(lineBuffer, " ") != 1) {
        return 0;
    }
    /* Tokenise board dimensions line into height, width strings. */
    char* height = strtok(boardDimensionsLine, " ");
    char* width = strtok(NULL, " ");
    /* Ensure height and width values are numeric */
    if (!is_numeric(height, 0) || !is_numeric(width, 1)) {
        return 0;
    }
    /* Modify the array starting at the integer pointer, boardDimensions,
     * with integral values of height and width. */
    boardDimensions[0] = atoi(height);
    boardDimensions[1] = atoi(width);

    return 1;
}

/**
 * Determines whether a file contains a valid line specifying which player
 * is next to move (by player symbol.) To be valid, the next turn line
 * must contain no more than two characters, (player symbol + newline
 * character.) Additionally, the only acceptable symbols are 'O' and 'X'. 
 *
 * gameFile:   A pointer to the game file, the file position indicator for
 *             the file stream must commence at the second line of
 *             the file. (i.e, The line which contains the symbol
 *             of the player whose turn is next.)
 *
 *  Returns:   1 if the line specifying whose turn is next is valid.
 *             0 otherwise.
 */
int validate_next_player_line(FILE* gameFile) {
    char lineBuffer[MAX_LINE_LENGTH];
    char* nextTurn = fgets(lineBuffer, MAX_LINE_LENGTH, gameFile);
    /* The number of characters should be 2. (symbol + newline.) */
    if (strlen(nextTurn) != 2) {
        return 0;
    }
    /* The only acceptable symbols on the next turn line are O and X. */
    if (nextTurn[0] != 'O' && nextTurn[0] != 'X') {
        return 0;
    }
    return 1;
}

/**
 * Checks that all rows in a game board are valid in terms of, appropriate
 * row length and all having the correct number of symbols:
 * ('O', 'X', '.', numbers 0-9).
 *
 * The parameters boardWidth and boardHeight should be checked for validity
 * prior to making a call to this function. Unexpected behaviour may result
 * if a specified board dimension is incorrect. Ensuring correctness is the
 * task of the calling scope.
 *
 *    gameFile:   A pointer to the game file, the file position indicator of
 *                the file stream must commence before the first row of the 
 *                game board (i.e the next call to fgets on the gameFile stream
 *                should return the first row of the game board in the file.)
 *  boardWidth:   The width of the game board contained within the specified 
 *                file.
 * boardHeight:   The height of the game board contained within the specified
 *                file.
 *
 *     Returns:   1 if all game board rows are valid,
 *                0 otherwise.
 */
int validate_board_rows(FILE* gameFile, int boardWidth, int boardHeight) {
    char lineBuffer[MAX_LINE_LENGTH];
    /* Keep track of the number of characters encountered in the game board. */
    int countedCharacters = 0;
    /* The expected number of symbols in the board (without spaces.) 
     * There are 2 * (width * height) symbols (including corner
     * cells which comprise 8 space characters in a game board.) */
    int expectedCharacters = 2 * (boardWidth * boardHeight) - 8;
    /* 1 cell has (1 symbol + 1 score number) * boardWidth + 1 newline. */
    int expectedRowLength = 2 * boardWidth + 1; 
    
    for (int i = 0; i < boardHeight; i++) {
        char* row = fgets(lineBuffer, MAX_LINE_LENGTH, gameFile);
        /* Check for too long/short of a row (e.g. too many/few symbols.) */
        if (strlen(row) != expectedRowLength) {
            return 0;
        }
        /* Count the number of occurrences of each character
         * in a board row which is one of: .OX0123456789 */
        countedCharacters += count_occurrences(row, ".OX0123456789");
        /* If the current row is the top or bottom of a gameboard, 
         * check that they are valid. */
        if (i == 0 || i == boardHeight - 1) {
            if (!validate_edge_rows(row, boardWidth)) {
                return 0;
            }
        } else {
        /* If the row is not a top/bottom-most row, check that there is
         * the appropriate number of '0' characters (in the correct spots) in
         * the game board row. */
            if (!validate_row_zeros(row)) {
                return 0;
            }         
            /* Check that there is the correct number of symbols '.', 'O', 'X',
             * There should be 'boardWidth' number of symbols in an inner row. 
             * (a row which is not the top or bottom most. */
            if (count_occurrences(row, ".XO") != boardWidth) {
                return 0;
            }
        }
    }
    /* Does the number of expected symbols match the number of symbols
     * read from the file lines specifying the game row? */
    if (countedCharacters != expectedCharacters) {
        return 0;
    }
    return 1;
}

/**
 * Ensures that a string specifying the row of a game board contains the
 * correct number of occurrences of the character '0'. This function
 * should only be called with 'internal rows' of the game board, that is,
 * row's which are neither the topmost, or bottom-most rows of a game board.
 *
 *     row:   A string specifying an internal row of a game board which will 
 *            be checked for having the correct number, and positioning of '0'
 *            characters.
 *
 * Returns:   An integer, 1 if the specified string has '0' characters in
 *            the correct positions to be considered a valid board row in
 *            that regard. 0 otherwise (e.g. 3 zero's in an internal row, etc.)
 */
int validate_row_zeros(char* row) {
    /* The first character in a board row must start with the character '0' 
     * (Except top/bottom-most rows.) */
    if (row[0] != '0') {
        return 0;
    }
    /* The third last character in a board row (including '\n'), must be '0' */
    if (row[strlen(row) - 3] != '0') {
        return 0;
    }
    /* Excluding topmost/bottom-most rows of a game board, a valid game board
     * should have no more than 2 occurences of the character '0' per row */
    if (count_occurrences(row, "0") > 2) {
        return 0;
    }
    return 1;
}

/**
 * Determines whether a file contains invalid characters. The only 
 * acceptable characters found in a game file are:
 * 'O, 'X', numbers '0' through '9', '.'' and newline ('\n').
 *
 * Upon completion of scanning, the file position indicator of gameFile
 * is reset to the beginning of the file.
 *
 * gameFile:   A pointer to a file which will be scanned for invalid
 *             Characters. The file position indicator of this File
 *             Should be at the beginning of the file in order for 
 *             all characters to be checked for validity.
 *
 *  Returns:   1 if a game file contains characters which are invalid
 *             as above.
 *             0 if the file contains only valid characters as above.
 */
int has_invalid_characters(FILE* gameFile) {
    char character;

    while ((character = fgetc(gameFile)) != EOF) {
        /* Searches for the first occurrence of character in 
         * "OX 0123456789.\n" */
        if (strchr("OX 0123456789.\n", character) == NULL) {
            /* A character was found that is not one of: OX 0123456789.\n */
            return 1;
        }
    }
    /* Reset the file postion indicator to the beginning of the file. */
    rewind(gameFile);
    return 0;
}

/**
 * Calculates the number of lines in a specified file, specifically, the
 * number of times the new line character is found. Hence, the total
 * number of lines is the number of new line characters found, plus one.
 *
 * The file passed to this function should have its file position indicator
 * at the beginning of the file (i.e. the file must be rewound before calling
 * this function.)
 *
 * Upon completion of line counting, the file position indicator of gameFile
 * is reset to the beginning of the file.
 *
 * gameFile:   A pointer to a file to count the number of lines of.
 *
 *  Returns:   The number of lines contained in gameFile.
 */
int count_lines(FILE* gameFile) {
    int countedLines = 0;
    char character;

    while ((character = fgetc(gameFile)) != EOF) {
        /* Count each occurrence of a newline character. */
        if (character == '\n') {
            countedLines++;
        }
    }
    /* Reset the file postion indicator to the beginning. */
    rewind(gameFile);
    /* countedLines = number of '\n' chars + 1 for last row with no newline */
    return countedLines + 1;
}

/**
 * Determines whether the corners contained on the very top and very bottom
 * rows of a game board are valid, i.e:
 *
 *    0.0.0.0.    <---
 *  0.2.1.1.1.0.
 *  0.1.1.2.1.0.
 *  0.1.1.1.1.0.
 *  0.2.1.1.2.0.
 *    0.0.0.0.    <---
 *
 * These rows are tested to ensure they contain two space characters (' ')
 * at the beginning and end, and that the row is followed by a new line.
 *
 * This function is for use with strings which represent either the very top
 * or very bottom rows of a game board only. Ensuring this remains the 
 * responsibility of the calling function.
 *
 *        row:   A string which should contain either the very top, or very 
 *               bottom row's of a game board.
 * boardWidth:   The width of a game board row, from which the string 'row'
 *               has been read from.
 *
 *    Returns:   1 if the row is valid inline with the conditions above,
 *               0 otherwise
 */
int validate_edge_rows(char* row, int boardWidth) {
    /* Check that the first, second, last and second last characters are space 
     * characters and check that the entire row ends with '\n'. */
    if (row[0] != ' ' || row[1] != ' ' || row[strlen(row) - 3] != ' '
            || row[strlen(row) - 2] != ' ' || row[strlen(row) - 1] != '\n') {
        return 0;
    }

    /* Edge rows may only contain the score value '0'. */
    int countedZeros = 0;

    /* Iterate through every even index (0, 2, 4..) of the row string 
     * and ensure that each character at that index is the character '0'. The
     * character at every even index (excluding corners) is a score value.*/
    for (int i = 2; i < 2 * boardWidth - 2; i += 2) {
        if (row[i] != '0') {
            /* A character representing a score value was found to not be 0 
             * which is not valid on an edge row. */
            return 0;
        }
        countedZeros++;
    }

    /* If the number of zeros counted at each even index in the row (where
     * they should be) is not the same as the total number of occurrences
     * of the character 0 in that row. Then the row is invalid. */
    if (countedZeros != count_occurrences(row, "0")) {
        return 0;
    }
    /* There should be boardWidth - 2 symbols on a topmost or bottom-most row.
     * symbols are one of ('O', 'X' or '.'). */
    if (count_occurrences(row, ".XO") != boardWidth - 2) {
        return 0;
    }

    /* The row string is a valid top/bottom game board row. */
    return 1;
}

/**
 * Determines whether the game is complete from a given gamestate,
 * specifically, checks whether the game board interior is full or not.
 * If the board interior is full (i.e. no '.' characters), the game is over.
 * If empty cells still exist on a playing field, the game should continue.
 *
 * gameState:   A Game structure containing the current state of the game
 *              (will be checked to determine if board interior is full.)
 *
 *   Returns:   1 if the game is over,
 *              0 otherwise.
 */
int is_game_over(struct Game gameState) {
    int boardHeight = gameState.fieldHeight;
    int boardWidth = gameState.fieldWidth;

    /* Iterate through the board interior and search for empty cells. */
    for (int i = 1; i < boardHeight - 1; i++) {
        for (int j = 1; j < boardWidth - 1; j++) { 
            if (is_cell_empty(gameState, i, j)) {
                return 0;
            }
        }
    }
    return 1;
}
