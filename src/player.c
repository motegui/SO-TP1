#include <unistd.h>
#include "player_functions.h"

int main(int argc, char *argv[]) {
    (void) argc;

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    size_t board_size = width * height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size;

    shm_t *state_shm = connect_shm("/game_state", game_state_size, O_RDONLY, PROT_READ);
    check_shm(state_shm, "Error al conectar a la memoria compartida del estado del juego");

    GameState_t *game_state = (GameState_t *) state_shm->shm_p;

    shm_t *sync_shm = connect_shm("/game_sync", sizeof(Sync_t), O_RDWR, PROT_READ | PROT_WRITE);
    check_shm(sync_shm, "Error al conectar a la memoria compartida de sincronización");

    Sync_t *sync = (Sync_t *) sync_shm->shm_p;

    int id = get_player_id(game_state);
    if (id == -1) {
        printf("[player] No encontré mi PID en la lista!\n");
        exit(EXIT_FAILURE);
    }

    int dx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
    int dy[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

    int last_sum = -1;

    while (1) {
        int current_sum;

        do {
            sem_wait(&sync->readers_count_mutex);
            if (++sync->players_reading == 1)
                sem_wait(&sync->state_access_mutex);
            sem_post(&sync->readers_count_mutex);

            Player_t *p = &game_state->players[id];
            current_sum = p->valid_moves + p->invalid_moves;
            bool blocked = p->blocked;

            sem_wait(&sync->readers_count_mutex);
            if (--sync->players_reading == 0)
                sem_post(&sync->state_access_mutex);
            sem_post(&sync->readers_count_mutex);

            if (blocked) goto end;

        } while (current_sum == last_sum);

        last_sum = current_sum;

        sem_wait(&sync->readers_count_mutex);
        if (++sync->players_reading == 1)
            sem_wait(&sync->state_access_mutex);
        sem_post(&sync->readers_count_mutex);

        Player_t *p = &game_state->players[id];
        int x = p->x;
        int y = p->y;

        int best_value = -1;
        int best_dir = -1;
        for (int d = 0; d < 8; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int val = game_state->board[ny * width + nx];
                if (val >= 1 && val <= 9 && val > best_value) {
                    best_value = val;
                    best_dir = d;
                }
            }
        }

        sem_wait(&sync->readers_count_mutex);
        if (--sync->players_reading == 0)
            sem_post(&sync->state_access_mutex);
        sem_post(&sync->readers_count_mutex);

        if (best_value > 0 && best_dir != -1) {
            unsigned char dir = (unsigned char) best_dir;
            write(1, &dir, 1);
        } else {
            close(1);
            break;
        }
    }

end:
    close_shm(state_shm);
    close_shm(sync_shm);
    return 0;
}
