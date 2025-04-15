#ifndef VIEW_FUNCTIONS_H
#define VIEW_FUNCTIONS_H

#define BLUE   "\x1b[34m"
#define YELLOW "\x1b[33m"
#define RESET  "\x1b[0m"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "sh_memory.h"

void print_table(int width, int height, GameState_t *game_state);
void print_players_info(GameState_t *game_state);
void print_semaphore_values(Sync_t *sync);

#endif