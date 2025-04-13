#ifndef PLAYER_FUNCTIONS_H
#define PLAYER_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
// #include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "sh_memory.h"

int get_player_id(GameState_t *game_state);
bool hit_border(int x, int y, int dx, int dy, int width, int height);

#endif