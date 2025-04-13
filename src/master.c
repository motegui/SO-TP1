#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sh_memory.h"
#include <time.h>
#include <sys/select.h>


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

void destroy_semaphores(Sync_t *sync){
    sem_destroy(&sync->pending_print);
    sem_destroy(&sync->print_done);
    sem_destroy(&sync->master_turn_mutex);
    sem_destroy(&sync->readers_count_mutex);
    sem_destroy(&sync->state_access_mutex);

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

    init_sync_semaphores(sync);

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

    

    // Lanzar la vista
    if (view_path != NULL) {
        launch_view_process(view_path, width, height);
    }

    // Mostrar estado inicial
    sleep(1);
    sem_post(&sync->pending_print);
    printf("[master] Avisé a la vista que imprima\n");
    sem_wait(&sync->print_done);

    int max_fd = 0;
    fd_set read_fds;

    for (int i = 0; i < player_qty; i++) {
        if (pipes[i][0] > max_fd) max_fd = pipes[i][0];
    }

    int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    // Bucle principal del juego
    while (!game_state->game_over) {
        FD_ZERO(&read_fds);
        for (int i = 0; i < player_qty; i++) {
            FD_SET(pipes[i][0], &read_fds);
        }

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            break;
        }

        for (int i = 0; i < player_qty; i++) {
            if (FD_ISSET(pipes[i][0], &read_fds)) {
                Player_t *p = &game_state->players[i];
                unsigned char dir;
                ssize_t bytes_read = read(pipes[i][0], &dir, 1);
                if (bytes_read <= 0) {
                    game_state->players[i].blocked = true;
                    continue;
                }
                int valido = 0;
                if (dir > 7) {
                    p->invalid_moves++;
                }else {
                    printf("[master] Movimiento recibido del jugador %d: dirección %d\n", i, dir);

                    // Actualizar el estado del juego según el movimiento

                    int nx = p->x + dx[dir];
                    int ny = p->y + dy[dir];

                    if (nx < 0 || ny < 0 || nx >= game_state->width || ny >= game_state->height) {
                        game_state->players[i].invalid_moves++;
                    } else {
                        int pos = ny * game_state->width + nx;
                        int cell = game_state->board[pos];
        
                        if (cell >= 1 && cell <= 9) {
                            game_state->board[p->y * game_state->width + p->x] = -(i + 1);
                            game_state->players[i].x = nx;
                            game_state->players[i].y = ny;
                            game_state->players[i].score += cell;
                            game_state->board[pos] = -(i + 1);
                            game_state->players[i].valid_moves++;
                            printf("[master] Movimiento válido: player %d a (%d, %d)\n", i, nx, ny);
                            valido = 1;
                        } else {
                            game_state->players[i].invalid_moves++;
                            printf("[master] Movimiento inválido: player %d intentó (%d, %d)\n", i, nx, ny);
                        }
                    }
                }
                sem_post(&sync->pending_print);
                sem_wait(&sync->print_done);
        
            }
        }

        bool any_active = false;

        for(int i = 0; i<player_qty; i++){
            if(game_state->players[i].blocked){
                continue;
            }
            
            int x = game_state->players[i].x;
            int y = game_state->players[i].y;

            for (int d = 0; d < 8; d++) {
                int nx = x + dx[d];
                int ny = y + dy[d];

                if (nx < 0 || ny < 0 || nx >= game_state->width || ny >= game_state->height){
                    continue;
                }
                int pos = ny * game_state->width + nx;
                int cell = game_state->board[pos];

                if (cell >= 1 && cell <= 9) {
                    any_active = true;
                    break;
                }
            }
            if(any_active){
                break;
            }
        }

        if(!any_active){
            printf("[master] Todos los jugadores están bloqueados. Fin del juego.\n");
            game_state->game_over = true;
            break;
        }

        usleep(delay * 1000);
    }

    int winner = -1;
    bool tie = false;

    for (int i = 0; i < player_qty; i++){
        printf("Jugador %s: puntaje=%u, válidos=%u, inválidos=%u\n",
            game_state->players[i].name,
            game_state->players[i].score,
            game_state->players[i].valid_moves,
            game_state->players[i].invalid_moves);
        if(winner = -1){
            winner = i;
        }else{
            Player_t *p = &game_state->players[i];
            Player_t *g = &game_state->players[winner];
            
            if(p->score > g->score){
                winner = i;
                tie = false;
            }else if( p -> score == g -> score){
                if(p->valid_moves < g ->valid_moves){
                    winner = i;
                    tie = false;
                }else if(p->valid_moves == g->valid_moves){
                    if(p->invalid_moves < g->invalid_moves){
                        winner = i;
                        tie = false;
                    }else if(p->invalid_moves == g->invalid_moves){
                        tie = true;
                    }
                }
            }
        }
    }

    if (tie) {
        printf("Resultado: ¡Empate!\n");
    } else {
        printf("Ganador: %s 🏆\n", game_state->players[winner].name);
    }

    // Finalizar juego
    game_state->game_over = true;
    sem_post(&sync->pending_print); // para que vista termine su while

    for (int i = 0; i < player_qty; i++) {
        wait(NULL);
    }

    delete_shm(state_shm);
    destroy_semaphores(sync);
    delete_shm(sync_shm);

    return 0;
}
