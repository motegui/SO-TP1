#include "player_functions.h"
#include <unistd.h>


int get_player_id(GameState_t *game_state) {
    pid_t my_pid = getpid();
    for (unsigned int i = 0; i < game_state->player_qty; i++) {
        if (game_state->players[i].pid == my_pid) {
            return i;
        }
    }
    return -1; // no encontrado
}

bool hit_border(int x, int y, int dx, int dy, int width, int height) {
    int nx = x + dx;
    int ny = y + dy;
    return (nx <= 0 || nx >= width || ny <= 0 || ny >= height);
}
