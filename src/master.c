#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sh_memory.h"
#include <time.h>

void launch_player_processes(int player_qty, GameState_t *game_state,char *players[], int width, int height);
void initialize_game_state(GameState_t *game_state, int width, int height);
void initialize_sems(Sync_t *sync);


void launch_player_processes(int player_qty, GameState_t *game_state,char *players[], int width, int height) {
    for (int i = 0; i < player_qty; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char width_str[10], height_str[10];
            sprintf(width_str, "%d", width);
            sprintf(height_str, "%d", height);
            execl(players[i], players[i], width_str, height_str, NULL);
            perror("execl jugador");
            exit(EXIT_FAILURE);
        }
        else{
            game_state->players[i].pid = pid;
            game_state->players[i].x = rand() % width;
            game_state->players[i].y = rand() % height;
            game_state->players[i].score = 0;
            game_state->players[i].valid_moves = 0;
            game_state->players[i].invalid_moves = 0;
            game_state->players[i].blocked = false;
            snprintf(game_state->players[i].name, sizeof(game_state->players[i].name), "player%d", i + 1);
            game_state->board[game_state->players[i].y * width + game_state->players[i].x] = -(i + 1);
        }
    }
}

void initialize_game_state(GameState_t *game_state, int width, int height) {
    game_state->width = width;
    game_state->height = height;
    game_state->game_over = false;
    game_state->player_qty = 0;

    // Inicializar tablero con recompensas
    for (int i = 0; i < width * height; i++) {
        game_state->board[i] = (i % 9) + 1;  // valores 1 a 9
    }
}

void initialize_sems(Sync_t *sync) {
    sem_init(&sync->pending_print, 1, 0);
    sem_init(&sync->print_done, 1, 0);
    sem_init(&sync->master_turn_mutex, 1, 1);
    sem_init(&sync->readers_count_mutex, 1, 1);
    sem_init(&sync->state_access_mutex, 1, 1);
    sync->players_reading = 0;
}


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

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Error: Las dimensiones del tablero deben ser mayores que 0\n");
        exit(EXIT_FAILURE);
    }

    if (player_qty > 9) {
        fprintf(stderr, "Error: No se pueden especificar más de 9 jugadores\n");
        exit(EXIT_FAILURE);
    }

    size_t board_size = width * height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size;

    // Crear memoria compartida de estado
    shm_t *state_shm = create_shm("/game_state", game_state_size, 0666, PROT_READ | PROT_WRITE );
    check_shm((shm_t *)state_shm, "Error al crear la memoria compartida del estado del juego");

    GameState_t *game_state = (GameState_t *) state_shm->shm_p;
    check_shm_ptr(game_state, "Error al mapear la memoria compartida del estado del juego");
    //check_shm(game_state, "Error al mapear la memoria compartida del estado del juego");

    initialize_game_state(game_state, width, height);

    shm_t *sync_shm = create_shm("/game_sync", sizeof(Sync_t), 0666, PROT_READ | PROT_WRITE);
    check_shm(sync_shm, "Error al crear la memoria compartida de sincronización");

    Sync_t *sync = (Sync_t *) sync_shm->shm_p;
    check_shm_ptr(sync, "Error al mapear la memoria compartida de sincronización");
    
    initialize_sems(sync);

    launch_player_processes(player_qty, game_state, players, width, height);

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

    // Esperar a que la vista esté lista
    sem_wait(&sync->print_done);
    printf("[master] La vista está lista\n");

    //Mostrar estado inicial
    sem_post(&sync->pending_print);
    printf("[master] Avisé a la vista que imprima\n");
    sem_wait(&sync->print_done);

    // Finalizar juego
    game_state->game_over = true;
    sem_post(&sync->pending_print); // para que vista termine su while
    wait(NULL);
    printf("[master] La vista terminó\n");

    close_shm(state_shm);
    close_shm(sync_shm);
    shm_unlink("/game_state");
    shm_unlink("/game_sync");

    return 0;
}
