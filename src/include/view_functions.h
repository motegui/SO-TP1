#ifndef VIEW_FUNCTIONS_H
#define VIEW_FUNCTIONS_H

#define BLUE   "\x1b[34m"
#define YELLOW "\x1b[33m"
#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define MAGENTA "\x1b[35m"
#define CYAN   "\x1b[36m"
#define WHITE  "\x1b[37m"
#define BOLD   "\x1b[1m"
#define DIM    "\x1b[2m"
#define GRAY   "\x1b[90m"
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

#endif