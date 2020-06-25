#ifndef PLAYER_HEADER
#define PLAYER_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "game.h"

#define PLAYER_NOT_FOUND -1

/* Function Declarations */
void handle_site_visit(struct Game* game, int playerID, int siteIndex);

void move_player(struct Game* game, int playerID, int siteIndex);

void place_player_at_site(Site* site, int playerID);

void remove_player_from_site(Site* site, int playerID);

int get_player_score(Player player);

bool hand_is_empty(int* denominations, int* size, int* hand);

int next_player_to_move(struct Game* game);

#endif
