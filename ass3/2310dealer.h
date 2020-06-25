#ifndef DEALER_HEADER
#define DEALER_HEADER

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/wait.h>
#include "game.h"
#include "player.h"
#include "error.h"
#include "util.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

#define MESSAGE_BUFFER_SIZE 100
#define BAD_FORMAT -1
#define MESSAGE_READY 2

/**
 * Card denominations to be drawn
 * by players during the game
 */
typedef enum Card {
    CARD_NONE = 0,
    CARD_A = 1,
    CARD_B = 2,
    CARD_C = 3,
    CARD_D = 4,
    CARD_E = 5
} Card;

/* Function Declarations */
struct Game* prepare_dealer_game(char* pathFile, int playerCount);

Site* construct_sites_from_file(FILE* pathFile, int playerCount, 
        int* siteCount, bool* parseSuccess);

bool spawn_player_process(struct Game* game, char* processName, 
        int playerID, int playerCount);

bool setup_players(struct Game* game, char** processPath, 
        char* rawPath, int playerCount);

bool get_player_message(char* messageBuffer, Player player, int size);

bool broadcast_path_on_ready(struct Game* game, char* rawPath);

bool send_player_message(Player player, char* message);

bool request_next_player_move(struct Game* game, char* message);

void broadcast_message(struct Game* game, char* format, ...);

char* deck_from_file(char* deckFileName, bool* valid);

void print_scores(struct Game* game);

Card get_next_card(char* deck, int* cardIndex);

int handle_do_message(struct Game* game, char* message);

void construct_event_message(struct Game* game, Player original, 
        Player updatedPlayer, Card cardDrawn);

ExitStatus run_game(struct Game* game, char* deck);

void handle_hangup(int signal);

void signal_listener(void);

bool is_move_valid(struct Game* game, int playerID, int newSite);

#endif