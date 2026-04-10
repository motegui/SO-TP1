#include "master_run.h"
#include "master_functions.h"
#include "sh_memory.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static const int k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

static void mark_all_players_blocked(GameState_t *gs, unsigned int player_qty) {
    for (unsigned int i = 0; i < player_qty; i++) {
        gs->players[i].blocked = true;
    }
}

static void end_game_with_final_view(MasterContext *ctx, Sync_t *sync, GameState_t *gs) {
    mark_all_players_blocked(gs, ctx->player_qty);
    if (ctx->view_path != NULL) {
        sem_post(&sync->pending_print);
        sem_wait(&sync->print_done);
    }
    gs->game_over = true;
}

static void master_print_config(const MasterContext *ctx) {
    printf("ancho: %d,\nalto: %d,\ndelay: %d,\ntimeout: %d,\nseed: %u\n",
           ctx->width, ctx->height, ctx->delay, ctx->timeout, ctx->seed);
    if (ctx->view_path != NULL) {
        printf("vista: %s\n", ctx->view_path);
    }
    for (int i = 0; i < ctx->player_qty; i++) {
        printf("jugador %d: %s\n", i, ctx->players[i]);
    }
}

int master_run(MasterContext *ctx) {
    srand((unsigned int)ctx->seed);

    master_print_config(ctx);

    size_t board_size = (size_t)ctx->width * (size_t)ctx->height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size;

    shm_t *state_shm = create_shm("/game_state", game_state_size, 0644, PROT_READ | PROT_WRITE);
    check_shm(state_shm, "Error al crear la memoria compartida del estado del juego");

    GameState_t *game_state = (GameState_t *)state_shm->shm_p;
    init_game_state(game_state, (unsigned short)ctx->width, (unsigned short)ctx->height,
                    (unsigned int)ctx->player_qty, false);

    shm_t *sync_shm = create_shm("/game_sync", sizeof(Sync_t), 0666, PROT_READ | PROT_WRITE);
    check_shm(sync_shm, "Error al crear la memoria compartida de sincronización");
    Sync_t *sync = (Sync_t *)sync_shm->shm_p;
    init_sync_semaphores(sync);

    for (int i = 0; i < ctx->width * ctx->height; i++) {
        game_state->board[i] = (rand() % 9) + 1;
    }

    int pipes[MASTER_MAX_PLAYERS][2];
    for (int i = 0; i < ctx->player_qty; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    launch_player_processes(ctx->player_qty, game_state, ctx->players, ctx->width, ctx->height,
                            pipes);

    if (ctx->view_path != NULL) {
        launch_view_process(ctx->view_path, ctx->width, ctx->height);
    }

    sleep(1);
    sem_post(&sync->pending_print);
    sem_wait(&sync->print_done);

    time_t last_valid_move_time = time(NULL);

    while (!game_state->game_over) {
        bool player_moves = read_players_moves(pipes, game_state, k_dx, k_dy, ctx->player_qty);

        if (player_moves) {
            last_valid_move_time = time(NULL);
        }

        sem_post(&sync->pending_print);
        sem_wait(&sync->print_done);

        if (check_all_players_blocked(game_state, ctx->player_qty)) {
            end_game_with_final_view(ctx, sync, game_state);
            break;
        }

        if (difftime(time(NULL), last_valid_move_time) >= ctx->timeout) {
            printf("[master] Timeout alcanzado (%d seg sin movimientos válidos). Fin del juego.\n",
                   ctx->timeout);
            end_game_with_final_view(ctx, sync, game_state);
            break;
        }

        struct timespec ts;
        ts.tv_sec = (time_t)(ctx->delay / 1000); /* delay en ms */
        ts.tv_nsec = (long)(ctx->delay % 1000) * 1000000L;
        nanosleep(&ts, NULL);
    }

    sem_post(&sync->pending_print);
    sem_wait(&sync->print_done);
    determine_winner(game_state, ctx->player_qty);

    for (int i = 0; i < ctx->player_qty; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    delete_shm(state_shm);
    destroy_semaphores(sync);
    delete_shm(sync_shm);

    return 0;
}
