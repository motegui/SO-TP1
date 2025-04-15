#include "player_functions.h"

int get_player_id(GameState_t *game_state) {
    pid_t my_pid = getpid();
    for (unsigned int i = 0; i < game_state->player_qty; i++) {
        if (game_state->players[i].pid == my_pid) {
            return i;
        }
    }
    return -1;
}