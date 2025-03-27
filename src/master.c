#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sh_memory.h"

int main() {
    int width = 10;
    int height = 10;

    size_t board_size = width * height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size;

    // Crear memoria compartida de estado
    shm_t *state_shm = create_shm("/game_state", game_state_size);
    GameState_t *game_state = (GameState_t *) state_shm->shm_p;
    printf("[view] Dirección del board: %p\n", game_state->board);  

    game_state->width = width;
    game_state->height = height;
    game_state->game_over = false;

    // Inicializar un jugador dummy
    strcpy(game_state->players[0].name, "Dummy");
    game_state->players[0].x = 5;
    game_state->players[0].y = 5;
    game_state->players[0].score = 10;
    game_state->players[0].valid_moves = 2;
    game_state->players[0].invalid_moves = 1;
    game_state->players[0].blocked = false;
    game_state->players[0].pid = getpid();
    game_state->player_qty = 1;
    printf("[view] width = %d, height = %d, total = %d\n", width, height, width * height);

    // Inicializar tablero con recompensas
    for (int i = 0; i < width * height; i++) {
        game_state->board[i] = (i % 9) + 1;  // valores de 1 a 9
    }

    // Crear memoria compartida de sincronización
    shm_t *sync_shm = create_shm("/game_sync", sizeof(Sync_t));
    Sync_t *sync = (Sync_t *) sync_shm->shm_p;

    // Inicializar semáforos
    sem_init(&sync->A, 1, 0); // aviso del master a la vista
    sem_init(&sync->B, 1, 0); //aviso de la vista al master
    sem_init(&sync->C, 1, 1); // semaforo para evitar la inanicion del master 
    sem_init(&sync->D, 1, 1); //mutex para contador F
    sem_init(&sync->E, 1, 1); //MUTEX LECTOR/ESCRITOR
    sync->F = 0; //CANTIDAD DE LECTORES ACTUALES

    // Lanzar la vista
    pid_t pid = fork();
    if (pid == 0) {
        char width_str[10], height_str[10];
        sprintf(width_str, "%d", width);
        sprintf(height_str, "%d", height);
        execl("./view", "view", width_str, height_str, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }

    // Esperar a que la vista esté lista y mostrar
    sleep(1);
    sem_post(&sync->A); // Le aviso a la vista que imprima
    printf("[master] Avisé a la vista que imprima\n");

    sem_wait(&sync->B); // Espero a que termine de imprimir

    // Terminar el juego
    game_state->game_over = true;
    sem_post(&sync->A); // última señal para que la vista salga del loop

    wait(NULL); // Espero a que termine la vista
    return 0;
}
