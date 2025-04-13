
#define BLUE   "\x1b[34m"
#define YELLOW "\x1b[33m"
#define RESET  "\x1b[0m"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "sh_memory.h"


void print_table(int width, int height, GameState_t *game_state) {
    for (int y = 0; y < height; y++) {
        printf("%3d |", y); // Índice de fila alineado
        for (int x = 0; x < width; x++) {
            int valor = game_state->board[y * width + x];

            if (valor < 0)
                printf(BLUE " J%-2d" RESET, -valor); // Jugador
            else if (valor == 0)
                printf(" ░░ "); // Celda vacía
            else
                printf(YELLOW " %2d " RESET, valor); // Recompensa
        }
        printf("\n");
    }
}


void print_players_info(GameState_t *game_state) {
    for (unsigned int i = 0; i < game_state->player_qty; i++) {
        Player_t *p = &game_state->players[i];
        printf("Jugador %s | Score: %d | Pos: (%d,%d) | Val: %d | Inv: %d | %s\n",
               p->name, p->score, p->x, p->y,
               p->valid_moves, p->invalid_moves,
               p->blocked ? "BLOQUEADO" : "ACTIVO");
    }
}

void print_semaphore_values(Sync_t *sync) {
    int pending_print_val, print_done_val, master_turn_val, state_access_val, readers_count_val;

    sem_getvalue(&sync->pending_print, &pending_print_val);
    sem_getvalue(&sync->print_done, &print_done_val);
    sem_getvalue(&sync->master_turn_mutex, &master_turn_val);
    sem_getvalue(&sync->state_access_mutex, &state_access_val);
    sem_getvalue(&sync->readers_count_mutex, &readers_count_val);

    printf("Valores de los semáforos:\n");
    printf("  pending_print: %d\n", pending_print_val);
    printf("  print_done: %d\n", print_done_val);
    printf("  master_turn_mutex: %d\n", master_turn_val);
    printf("  state_access_mutex: %d\n", state_access_val);
    printf("  readers_count_mutex: %d\n", readers_count_val);
    printf("  players_reading: %d\n", sync->players_reading);
    printf("--------------------------------------------------\n");
}



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
    // 1. Esperar aviso del máster
        printf("[view] Esperando señal del master...\n");
        sem_wait(&sync->pending_print);
        print_semaphore_values(sync);
        printf("[view] Recibí señal del master!\n");

        // // 2. Comenzar protocolo de lectura segura
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

        // Imprimir tablero
        print_table(width, height, game_state);

        printf("\n");
        print_players_info(game_state);
        printf("\n");


        // // 4. Fin de lectura segura
        sem_wait(&sync->readers_count_mutex);
        if (--sync->players_reading == 0) {
            sem_post(&sync->state_access_mutex); // Último lector libera al máster
        }
        sem_post(&sync->readers_count_mutex);
        // 5. Avisar al máster que se terminó de imprimir
       
        sem_post(&sync->print_done);

        printf("Despues de avisar al master\n");
        print_semaphore_values(sync);

    }
    
    close_shm(state_shm);
    close_shm(sync_shm);

    return 0;
}