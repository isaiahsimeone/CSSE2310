#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Properties for file and command input. */
enum FileInputProperties {
    /* The maximum permissible length of any one line in the game file. 
     * The current limit for an input file is 2002 characters (or 1000 game
     * cells with newline & null terminator in buffers.) */
    MAX_LINE_LENGTH = 2002,
    /* The maximum permissible length that a player can input. */
    MAX_INPUT_LENGTH = 85
};

/* Human input return codes. */
enum HumanInputCodes {
    /* Used when a human player entered valid input (A move command.)*/
    HUMAN_INPUT_VALID = 1,
    /* Used when a human player enters invalid input. */
    HUMAN_INPUT_INVALID = 2,
    /* Used when a human player enters a save command (e.g. "ssavegame".) */
    HUMAN_INPUT_SAVE = 3
};

/* Stone push simple directions (to.) */
enum PushMoves {
    INVALID_PUSH_MOVE = 0,
    VALID_PUSH_RIGHT = 1,
    VALID_PUSH_LEFT = 2,
    VALID_PUSH_UP = 3,
    VALID_PUSH_DOWN = 4
};

/* Pushing stone complete directions (from, to.) */
enum PushVectors {
    TOP_TO_BOTTOM = 1,
    BOTTOM_TO_TOP = 2,
    LEFT_TO_RIGHT = 3,
    RIGHT_TO_LEFT = 4
};

/* Decimal equivalents of characters. */
enum DecimalCharacterEquivalent {
    /* The integer equivalent of a space character. */
    DECIMAL_SPACE_CHAR = -16,
    /* The integer equivalent of a new line character. */
    DECIMAL_NEWLINE_CHAR = -38
};

/* Program exit codes. */
enum ExitCodes {
    /* Too many/few arguments specified */
    INVALID_NUM_ARGS = 1,
    /* Invalid player type (not one of '0', '1', 'H') */
    INVALID_PLAYER_TYPE = 2,
    /* Unable to read specified input board (e.g. permission error etc) */
    UNABLE_TO_READ_FILE = 3,
    /* Specified board file invalid */
    INVALID_FILE_FORMAT = 4,
    /* Encountered end of file from stdin */
    FOUND_EOF = 5,
    /* Specified board has no empty interior cells */
    INVALID_BOARD_FULL = 6
};

/*************************
 * Function declarations *
 *************************/

int is_player_type_valid(char* playerType);

struct Game parse_game_file(FILE* gameFile);

int is_file_valid(FILE* gameFile);

int load_from_save(char* fileName);

void draw_game_field(struct Game gameState);

int do_game(struct Game gameState, char playerOType, char playerXType);

void draw_human_prompt(char playerSymbol);

struct Game do_human_move(struct Game gameState, char* humanInput, 
        char playerSymbol);

int is_game_over(struct Game gameState);

int is_move_in_bounds(struct Game gameState, int row, int col);

int validate_human_input(struct Game gameState, char* humanInput);

int count_occurrences(char* string, char* occurrences);

int is_numeric(char* string, int offset);

int is_cell_empty(struct Game gameState, int row, int col);

struct Game push_stones_in_dir(struct Game gameState, int rowPosition, 
        int colPosition, int direction);

int is_valid_human_move(struct Game gameState, int row, int col);

int is_push_move(struct Game gameState, int row, int col);

int is_edge(struct Game gameState, int row, int col);

int get_next_empty(struct Game gameState, int row, int col, int axis);

struct Game do_type_zero_move(struct Game gameState, char playerSymbol);

void print_auto_player_move(char playerSymbol, int row, int col);

int get_player_score(struct Game gameState, char playerSymbol);

struct Game do_type_one_move(struct Game gameState, char playerSymbol);

struct Game place_stone(struct Game gameState, int row, int col, 
        char playerSymbol);

struct Game do_type_one_push(struct Game gameState, int row, int col, 
        char playerSymbol);

struct Game revert_field_cells(struct Game gameState, int boardWidth, 
        char originalCells[][boardWidth]);

struct Game type_one_defensive_move(struct Game gameState, char playerSymbol, 
        int* foundMove);

struct Game type_one_aggressive_move(struct Game gameState, char playerSymbol);

void draw_game_over(struct Game gameState);

void save_game(struct Game gameState, char* saveName);

int validate_game_file(FILE* gameFile);

int has_invalid_characters(FILE* gameFile);

int validate_edge_rows(char* row, int boardWidth);

int count_lines(FILE* gameFile);

int get_score(struct Game gameState, int row, int col);

char get_symbol(struct Game gameState, int row, int col);

void set_symbol(struct Game gameState, int row, int col, char symbol);

int validate_board_rows(FILE* gameFile, int boardWidth, int boardHeight);

int validate_dimensions_line(FILE* gameFile, int* boardDimensions);

int validate_next_player_line(FILE* gameFile);

int char_to_int(char charNumeral);

char int_to_char(int number);

struct Game handle_human_input(struct Game gameState, char currentPlayer, 
        int* status);

struct Game type_one_check_top(struct Game gameState, char playerSymbol, 
        char enemySymbol, int enemyScore, int boardHeight, 
        char originalCells[][boardHeight], int* foundMove);

struct Game type_one_check_right(struct Game gameState, char playerSymbol, 
        char enemySymbol, int enemyScore, int boardHeight, 
        char originalCells[][boardHeight], int* foundMove);

struct Game type_one_check_bottom(struct Game gameState, char playerSymbol, 
        char enemySymbol, int enemyScore, int boardHeight, 
        char originalCells[][boardHeight], int* foundMove);

struct Game type_one_check_left(struct Game gameState, char playerSymbol, 
        char enemySymbol, int enemyScore, int boardHeight, 
        char originalCells[][boardHeight], int* foundMove);

char get_opposite_player(char playerSymbol);

int validate_row_zeros(char* row);