#ifndef MASTER_FUNCTIONS_H
#define MASTER_FUNCTIONS_H

#include "sh_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>

void launch_player_processes(int player_qty, GameState_t *game_state, char *players[], int width, int height, int pipes[][2]);
void launch_view_process(char *view_path, int width, int height);
void init_game_state(GameState_t *game_state, int width, int height, int player_qty, bool game_over);
void init_sync_semaphores(Sync_t *sync);
void destroy_semaphores(Sync_t *sync);
void read_players_moves(int pipes[][2], GameState_t *game_state, int dx[], int dy[], int player_qty);
void determine_winner(GameState_t *game_state, int player_qty);
bool check_all_players_blocked(GameState_t *game_state, int player_qty);


#endif