#include "view_functions.h"
#include <unistd.h>


int main(int argc, char *argv[]){
    //verifico la cantidad de argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        return 1;
    }
    //obtengo alto y ancho del tablero desde los argumentos 
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    printf("ancho: %i, largo: %i\n", width, height);
    // calculo el tamano del tablero (int por celda)
    size_t board_size = width * height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size; // Calcular tamaño total de la estructura GameState_t + tablero


    shm_t *state_shm = connect_shm("/game_state", game_state_size, O_RDONLY, PROT_READ); //Conectarse a la memoria compartida del estado del juego
    check_shm(state_shm, "Error al conectar a la memoria compartida del estado del juego");

    GameState_t *game_state = (GameState_t *) state_shm->shm_p;

    shm_t *sync_shm = connect_shm("/game_sync", sizeof(Sync_t), O_RDWR, PROT_READ | PROT_WRITE);// Conectarse a la memoria compartida de sincronización
    check_shm(sync_shm, "Error al conectar a la memoria compartida de sincronización");

    Sync_t *sync = (Sync_t *) sync_shm->shm_p;
    
    while (!game_state->game_over) {
    sem_wait(&sync->pending_print);

if (!game_state->game_over) {
   printf("\033[2J\033[H");
 // solo limpia si no terminó
}


    // Protocolo de lectura segura
    sem_wait(&sync->readers_count_mutex);
    if (++sync->players_reading == 1) {
        sem_wait(&sync->state_access_mutex); // Primer lector bloquea al máster
    }
    sem_post(&sync->readers_count_mutex);

    // Imprimir encabezado
    printf("\n   ");
    for (int x = 0; x < width; x++) {
        printf("%4d", x);
    }
    printf("\n");

    print_table(width, height, game_state);
    printf("\n");
    print_players_info(game_state);
    printf("\n");

    // Fin de lectura segura
    sem_wait(&sync->readers_count_mutex);
    if (--sync->players_reading == 0) {
        sem_post(&sync->state_access_mutex);
    }
    sem_post(&sync->readers_count_mutex);

    sem_post(&sync->print_done);
}

    
    close_shm(state_shm);
    close_shm(sync_shm);

    return 0;
}