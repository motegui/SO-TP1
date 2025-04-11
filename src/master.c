#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sh_memory.h"
#include <time.h>


void launch_player_processes(int player_qty, GameState_t *game_state, char *players[], int width, int height, int pipes[][2]){
    for (int i = 0; i < player_qty; i++) {
        pid_t pid = fork();
        if (pid == 0) {

            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]);

            char width_str[10], height_str[10];
            sprintf(width_str, "%d", width);
            sprintf(height_str, "%d", height);

            execl(players[i], players[i], width_str, height_str, NULL);
            perror("execl jugador");
            exit(EXIT_FAILURE);
        }
        else{
            close(pipes[i][1]);

            game_state->players[i].pid = pid;
            game_state->players[i].x = rand() % width; // HAY QUE MEJORARLO
            game_state->players[i].y = rand() % height; // HAY QUE MEJORARLO
            game_state->players[i].score = 0;
            game_state->players[i].valid_moves = 0;
            game_state->players[i].invalid_moves = 0;
            game_state->players[i].blocked = false;

            snprintf(game_state->players[i].name, sizeof(game_state->players[i].name), "player%d", i + 1);

            game_state->board[game_state->players[i].y * width + game_state->players[i].x] = -(i + 1);


        }
    }
}

void launch_view_process(char *view_path, int width, int height) {
    pid_t pid = fork();
    if (pid == 0) {
        char width_str[10], height_str[10];
        sprintf(width_str, "%d", width);
        sprintf(height_str, "%d", height);
        execl(view_path, view_path, width_str, height_str, NULL);
        perror("execl vista");
        exit(EXIT_FAILURE);
    }
}

void init_game_state(GameState_t *game_state, int width, int height, int player_qty, bool game_over) {
    game_state->width = width;
    game_state->height = height;
    game_state->game_over = game_over;
    game_state->player_qty = player_qty;
}

void init_sync_semaphores(Sync_t *sync){
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
    int delay = 200;
    int timeout = 10;
    unsigned int seed = time(NULL);


    // Parseo de argumentos para obtener paths de jugadores
    int opt;
    char *view_path = NULL;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
            case 'w':
                width = atoi(optarg);
                if (width < 10) width = 10;
                break;
            case 'h':
                height = atoi(optarg);
                if (height < 10) height = 10;
                break;
            case 'd':
                delay = atoi(optarg);
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 's':
                seed = (unsigned int) atoi(optarg);
                break;
            case 'v':
                view_path = malloc(strlen(optarg) + 1);
                strcpy(view_path, optarg);
                break;
            case 'p':
                players[player_qty++] = optarg;
                while (optind < argc && argv[optind][0] != '-') {
                    if (player_qty < 9) {
                        players[player_qty++] = argv[optind++];
                    } else {
                        fprintf(stderr, "Máximo 9 jugadores.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            default:
                fprintf(stderr, "Uso: %s [-w ancho] [-h alto] [-d delay] [-t timeout] [-s seed] [-v vista] -p player1 [player2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (player_qty == 0) {
        fprintf(stderr, "Debe especificar al menos un jugador con -p.\n");
        exit(EXIT_FAILURE);
    }

    printf("ancho: %d,\nalto: %d,\ndelay: %d,\ntimeout: %d,\nseed: %u\n", width, height, delay, timeout, seed);
    if (view_path) printf("vista: %s\n", view_path);
    for (int i = 0; i < player_qty; i++) {
        printf("jugador %d: %s\n", i, players[i]);
    }

    size_t board_size = width * height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size;

    // Crear memoria compartida de estado
    shm_t *state_shm = create_shm("/game_state", game_state_size, 0644, PROT_READ | PROT_WRITE );
    check_shm(state_shm, "Error al crear la memoria compartida del estado del juego");

    GameState_t *game_state = (GameState_t *) state_shm->shm_p;
    init_game_state(game_state, width, height, player_qty, false);

    // Crear memoria compartida de sincronización
    shm_t *sync_shm = create_shm("/game_sync", sizeof(Sync_t), 0666, PROT_READ | PROT_WRITE);
    check_shm(sync_shm, "Error al crear la memoria compartida de sincronización");
    Sync_t *sync = (Sync_t *) sync_shm->shm_p;

    // Inicializar tablero con recompensas
    for (int i = 0; i < width * height; i++) {
        game_state->board[i] = (rand() % 9) + 1;  // valores 1 a 9
    }

    //Crear los canales de comunicación para recibir solicitudes de movimientos de los jugadores

    int pipes[9][2]; // pipes[i][0] -> lectura, pipes[i][1] -> escritura

    for (int i = 0; i < player_qty; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Lanzar procesos jugador (aca tambien se distribuyen los jugadores en el tablero)
    launch_player_processes(player_qty, game_state, players, width, height, pipes);

    init_sync_semaphores(sync);

    // Lanzar la vista

    if (view_path != NULL) {
        launch_view_process(view_path, width, height);
    }

    // Mostrar estado inicial
    sleep(1);
    sem_post(&sync->pending_print);
    printf("[master] Avisé a la vista que imprima\n");
    sem_wait(&sync->print_done);

    // Finalizar juego
    game_state->game_over = true;
    sem_post(&sync->pending_print); // para que vista termine su while
    for (int i = 0; i < player_qty; i++) {
        wait(NULL);
    }

    delete_shm(state_shm);
    delete_shm(sync_shm);

    return 0;
}
