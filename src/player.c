#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sh_memory.h"

// Función para obtener el índice del jugador según su pid
int get_player_id(GameState_t *game_state) {
    pid_t my_pid = getpid();
    for (int i = 0; i < game_state->player_qty; i++) {
        if (game_state->players[i].pid == my_pid) {
            return i;
        }
    }
    return -1; // no encontrado
}

int main(int argc, char *argv[]) {
    printf("[player] Hola! Soy un jugador. Me pasaron: %s %s\n", argv[1], argv[2]);

    // 1. Conexión a memoria compartida
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    size_t board_size = width * height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size;

    shm_t *state_shm = connect_shm("/game_state", game_state_size);
    GameState_t *game_state = (GameState_t *) state_shm->shm_p;

    shm_t *sync_shm = connect_shm("/game_sync", sizeof(Sync_t));
    Sync_t *sync = (Sync_t *) sync_shm->shm_p;

    // 2. Identificarme en el array de jugadores
    int id = get_player_id(game_state);
    int dx[] = { -1,  0,  1, -1, 1, -1,  0, 1 };
    int dy[] = { -1, -1, -1,  0, 0,  1,  1, 1 };

    if (id == -1) {
        printf("[player] No encontré mi PID en la lista!\n");
        exit(EXIT_FAILURE);
    }

    printf("[player] Soy el jugador #%d (PID %d)\n", id, getpid());

    // 3. Bucle principal hasta que se termine el juego
    while (!game_state->game_over) {
        usleep(300000); // Esperar un poco (300ms) para no saturar

        // 4. Protocolo de escritura segura
        sem_wait(&sync->C); // Turno de escritura
        sem_wait(&sync->E); // Exclusión mutua total

        Player_t *p = &game_state->players[id];
            int best_value = -1;
            int best_x = p->x;
            int best_y = p->y;

            for (int d = 0; d < 8; d++) {
                int nx = p->x + dx[d];
                int ny = p->y + dy[d];

                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    int cell = game_state->board[ny * width + nx];
                    if (cell > best_value) {
                        best_value = cell;
                        best_x = nx;
                        best_y = ny;
                    }
                }
            }


if (best_value > 0) {
    int old_x = p->x;
    int old_y = p->y;
    game_state->board[old_y * width + old_x] = 0;
    p->x = best_x;
    p->y = best_y;
    p->score += best_value;
    p->valid_moves++;
    game_state->board[best_y * width + best_x] = -id - 1;
} else {
    p->invalid_moves++;
}


       

        // 6. Fin protocolo
        sem_post(&sync->E);
        sem_post(&sync->C);
    }

    printf("[player] El juego terminó. Salgo!\n");
    return 0;
}
