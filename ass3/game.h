#ifndef HEADER_GAME
#define HEADER_GAME

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "error.h"

#define COMMUNICATION_BUFFER_SIZE 100

// The maximum number of players allowed in the game
#define MAX_PLAYER_COUNT 200

// the number of digits which can come before 
// a semicolon in the path.
#define STOP_COUNT_MAX_SIZE 10 
#define STOP_COUNT_DELIMITER ';'

// Used when validating paths
#define PATH_MALFORMED -1
#define PATH_VALID 1

// The length of a site Name "Mo", "Do", etc
#define SITE_NAME_LENGTH 2
#define RAW_SITE_LENGTH 3

// For drawing players at sites. No
// occupant denotes no player at site
#define NO_OCCUPANT -1

// Used when searching for sites to move to
#define SITE_NOT_FOUND -1
#define PLAYER_TIE -1

// Game initialisation values
#define PLAYER_INIT_MONEY 7
// Number of distinct cards (including no card)
#define DISTINCT_CARDS 6 

/**
 * Represents what type of message was
 * received/sent e.g. "YT", "DONE", etc
 */ 
typedef enum Message {
    MESSAGE_YOUR_TURN = 1,
    MESSAGE_EARLY_END = 2,
    MESSAGE_GAME_DONE = 3,
    MESSAGE_MADE_MOVE = 4,
    MESSAGE_UNKNOWN = 5
} Message;

/**
 * Used to assign a unique identifier
 * for each type of site in the game
 * to make comparisons cleaner
 */ 
typedef enum SiteType {
    SITE_BARRIER = 1,
    SITE_V1 = 2,
    SITE_V2 = 3,
    SITE_MO = 4,
    SITE_DO = 5,
    SITE_RI = 6,
    SITE_UNKNOWN = 7
} SiteType;

/**
 * Represents a single site within the game derived from the game path.
 *
 *   rawSiteName:   The full display name of a site (including its capacity
 *                  (e.g. "Mo1"))
 *      siteName:   The same as rawSiteName but without a capacity ("Mo")
 *      siteType:   An integer according to the SiteType enumeration above 
 *                  specifying the type of this site
 *     siteIndex:   The position (index) of this site in the game path that
 *                  it was derived from
 *  siteCapacity:   The maximum number of players that can occupy this site
 *                  at once
 * occupantCount:   The current number of players (occupants) at this site
 * siteOccupants:   The playerID's of the players that are currently at this
 *                  Site (e.g. [0, 3, 6] means players 0, 3 and 6 are here)
 */ 
typedef struct {
    char rawSiteName[RAW_SITE_LENGTH];
    char siteName[SITE_NAME_LENGTH];
    int siteType;
    int siteIndex;
    int siteCapacity;
    int occupantCount;
    int siteOccupants[MAX_PLAYER_COUNT];
} Site;

/**
 * Used to record the cards that are held for a given player. This 
 * structure forms part of the 'Player' structure (below)
 *
 *      cards:   An int array whose indices specify the total number of cards
 *               of that denomination the player holds for example, if 
 *               cards = [0, 3, 1, 6, 2, 3] the player holds 3 A cards,
 *               1 B card, 6 C cards, 2 D and 3 E cards
 * totalCards:   The total number of cards that are contained in the cards
 *               integer array (excluding the 0th index since that represents
 *               'no card')
 */ 
typedef struct PlayerCards {
    int cards[DISTINCT_CARDS];
    int totalCards;
} PlayerCards;

/**
 * Used by the dealer process to communicate with child player processes
 * This structure forms part of the 'Player' structure (below)
 *
 * playerPID:   The process ID of the child process that was started
 *   receive:   The File descriptor that the dealer uses to read messages
 *              in the players stdout
 *      send:   The File descriptor that the dealer uses to send messages/
 *              data to the players stdin
 */ 
typedef struct PlayerPipe {
    pid_t playerPID;
    FILE* receive;
    FILE* send;
} PlayerPipe;

/**
 * Represents a player who is playing in the game.
 *
 *         hand:   A PlayerCards structure denoting the players current
 *                 hand of cards (see PlayerCards above)
 *         pipe:   A PlayerPipe structure containing information for the
 *                 dealer process on which FILE* to use to communicate
 *        money:   The amount of money currently held by the player
 *     playerID:   The ID of this player
 *       points:   The amount of points held by this player
 * siteV2Visits:   The number of V2 sites this player has visited
 * siteV1Visits:   The number of V1 sites this player has visited
 *         site:   The index of the site that this player is currently
 *                 visiting
 */ 
typedef struct Player {
    PlayerCards hand;
    PlayerPipe pipe;
    int money;
    int playerID;
    int points;
    int siteV2Visits;
    int siteV1Visits;
    int site;
} Player; 

/**
 * Represents the entire state of the game. Each player process (or dealer)
 * is required to initialise and maintain their own Game structure to ensure
 * that players have the required information to make moves (and the dealer
 * so it can govern the game as required)
 *
 *          sites:   A pointer to one (and possibly followed by more) Site 
 *                   structures (see the Site Structure above)
 *        players:   A pointer to one (and possibly followed by more) Player 
 *                   structures (see the Player Structure above). Contains
 *                   All of the players in the game
 *   mainPlayerID:   The playerID of the 'main' player for player processes.
 *                   (e.g. a 2310A should set mainPlayerID to the ID of the
 *                   player that was given as its command line argument)
 *                   (dealers do not require this member to be set)
 *      siteCount:   The total number of sites contained within the game Path
 *    playerCount:   The total number of players currently playing in the game
 * parseSucceeded:   A boolean value that can be utilised as a status flag
 *                   when initially parsing the Game structure (with a path,
 *                   players, etc)
 */ 
struct Game {
    Site* sites;
    Player* players;
    int mainPlayerID;
    int siteCount;
    int playerCount;
    bool parseSucceeded;
};

/**
 * Represents a HAP event message that each player receives each time an 
 * opponent player makes a move (which is communicated via a dealer
 * process.)
 *
 *    playerID:   The playerID of the player which this HAP event message
 *                is referencing (AKA the player that just moved)
 *     newSite:   The site that the referenced playerID moved to
 * pointChange:   The net change in points that the player had from moving
 * moneyChange:   The net change in money that the player had from moving
 *   cardDrawn:   The card that was drawn by the player (0 = No card, 
 *                1 = CARD_A, 2 = CARD_B, 3 = CARD_C, 4 = CARD_D, 5 = CARD_E)
 */ 
typedef struct GameEvent {
    int playerID;
    int newSite;
    int pointChange;
    int moneyChange;
    int cardDrawn;
} GameEvent;


/* Function Declarations */
SiteType get_site_type(char* siteName);

bool site_has_room(Site site);

int get_site_capacity(char siteCapacity, int playerCount);

void print_game_over(struct Game* game);

Message match_message(char* message);

int read_path(char* path, FILE* stream, int siteCount);

void draw_game(struct Game* game, FILE* stream);

int read_path_size(FILE* stream);

Player* prepare_players(int playerCount);

struct Game* prepare_game(int mainPlayerID, int playerCount);

Site* construct_sites(int playerCount, int* sitesConstructed, 
        bool* parseSuccess);

bool validate_barriers(Site* sites, int siteCount);

void initialise_players(struct Game* game, int playerCount);

GameEvent parse_event_message(struct Game* game, char* message, bool* valid);

GameEvent tokenise_event_message(char* message, bool* tokeniseSuccess);

void handle_site_visit(struct Game* game, int playerID, int siteIndex);

void print_event_summary(Player player, FILE* outputStream);

int find_next_barrier(struct Game* game, int currentSite);

int find_site_before_barrier(struct Game* game, SiteType type, int fromSite);

bool handle_event(struct Game* game, char* message);

bool is_game_over(struct Game* game);

Site parse_site(char* path, int indexInPath, int playerCount, 
        bool* parseSuccess);

void move_player(struct Game* game, int playerID, int siteIndex);

#endif