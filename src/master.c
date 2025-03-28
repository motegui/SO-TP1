#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sh_memory.h"
#include <time.h>


int main(int argc, char *argv[]) {
    srand(time(NULL));

    char *players[9];
    int player_qty = 0;
    int width = 10, height = 10;

    // Parseo de argumentos para obtener paths de jugadores
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                while (optind < argc && argv[optind][0] != '-') {
                    players[player_qty++] = argv[optind++];
                }
                break;
            default:
                fprintf(stderr, "Uso: %s -p ./player1 [./player2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    size_t board_size = width * height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size;

    // Crear memoria compartida de estado
    shm_t *state_shm = create_shm("/game_state", game_state_size);
    GameState_t *game_state = (GameState_t *) state_shm->shm_p;
    game_state->width = width;
    game_state->height = height;
    game_state->game_over = false;
    game_state->player_qty = player_qty;

    // Inicializar tablero con recompensas
    for (int i = 0; i < width * height; i++) {
    game_state->board[i] = (i % 9) + 1;  // valores 1 a 9
}


    // Crear memoria compartida de sincronización
    shm_t *sync_shm = create_shm("/game_sync", sizeof(Sync_t));
    Sync_t *sync = (Sync_t *) sync_shm->shm_p;
    sem_init(&sync->A, 1, 0);
    sem_init(&sync->B, 1, 0);
    sem_init(&sync->C, 1, 1);
    sem_init(&sync->D, 1, 1);
    sem_init(&sync->E, 1, 1);
    sync->F = 0;

    // Lanzar procesos jugador
    for (int i = 0; i < player_qty; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char width_str[10], height_str[10];
            sprintf(width_str, "%d", width);
            sprintf(height_str, "%d", height);
            execl(players[i], players[i], width_str, height_str, NULL);
            perror("execl jugador");
            exit(EXIT_FAILURE);
        } else {
                game_state->players[i].pid = pid;
                game_state->players[i].x = rand() % width;
                game_state->players[i].y = rand() % height;
                game_state->players[i].score = 0;
                game_state->players[i].valid_moves = 0;
                game_state->players[i].invalid_moves = 0;
                game_state->players[i].blocked = false;
                snprintf(game_state->players[i].name, sizeof(game_state->players[i].name), "J%d", i+1);
                game_state->board[game_state->players[i].y * width + game_state->players[i].x] = -(i + 1);


        }
    }

    // Lanzar la vista
    pid_t pid = fork();
    if (pid == 0) {
        char width_str[10], height_str[10];
        sprintf(width_str, "%d", width);
        sprintf(height_str, "%d", height);
        execl("./view", "view", width_str, height_str, NULL);
        perror("execl vista");
        exit(EXIT_FAILURE);
    }

    // Mostrar estado inicial
    sleep(1);
    sem_post(&sync->A);
    printf("[master] Avisé a la vista que imprima\n");
    sem_wait(&sync->B);

    // Finalizar juego
    game_state->game_over = true;
    sem_post(&sync->A); // para que vista termine su while
   for (int i = 0; i < player_qty; i++) {
    wait(NULL);
}

    return 0;
}
