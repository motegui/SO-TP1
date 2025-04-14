#include "master_functions.h"
#include <unistd.h>


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
    //printf("[master] Avisé a la vista que imprima\n");
    sem_wait(&sync->print_done);
    int max_fd = 0;
    //fd_set read_fds;

    for (int i = 0; i < player_qty; i++) {
        if (pipes[i][0] > max_fd) max_fd = pipes[i][0];
    }

    int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    time_t last_valid_move_time = time(NULL);

    // Bucle principal del juego
    while (!game_state->game_over) {
        bool player_moves = read_players_moves(pipes, game_state, dx, dy, player_qty);
    
        if(player_moves){
            last_valid_move_time = time(NULL);
        } 

        // Notificar a la vista
        sem_post(&sync->pending_print);
        sem_wait(&sync->print_done);
    
        if (check_all_players_blocked(game_state, player_qty)) {
            game_state->game_over = true;
            break;
        }

        if (difftime(time(NULL), last_valid_move_time) >= timeout) {
            printf("[master] Timeout alcanzado (%d seg sin movimientos válidos). Fin del juego.\n", timeout);
            game_state->game_over = true;
            break;
        }
    
        usleep(delay * 1000); // Esperar antes del siguiente turno
    }

    game_state->game_over = true;
    sem_post(&sync->pending_print); // Aviso final para que vista haga último print
    sem_wait(&sync->print_done);    // Esperá a que termine

    determine_winner(game_state, player_qty); // Ahora imprimís el ganador

    for (int i = 0; i < player_qty; i++) {
        wait(NULL);
    }

    for (int i = 0; i < player_qty; i++) {
        close(pipes[i][0]); // Cerrar extremo de lectura
        close(pipes[i][1]); // Cerrar extremo de escritura
    }

    if(view_path) {
        free(view_path);
    }

    delete_shm(state_shm);
    destroy_semaphores(sync);
    delete_shm(sync_shm);

    return 0;
}
