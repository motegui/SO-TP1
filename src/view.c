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
     //entonces ahora a partir de aca, lo que tengo que hacer es:
     /*
     1) la vista tiene que esperar a que el master le avise que hay algo nuevo para mostrar (sem A)
     2) leer el estado del juego de forma segura osea sin interferir ni con el master ni con los jugadores (sem c,d,e y var f)
     3) imprimir el tablero y la info de los jugadores
     4) avisar al master q termino de imprimir (sem B)
     5) REPETIR HASTA Q game_over == true
     */
    while (!game_state->game_over) {
    // 1. Esperar aviso del máster
        printf("[view] Esperando señal del master...\n");
        sem_wait(&sync->pending_print);
        printf("[view] Recibí señal del master!\n");

        // // 2. Comenzar protocolo de lectura segura
        // printf("[view] Entrando a la lectura\n");
        // sem_wait(&sync->E);
        // sem_wait(&sync->D);
        // if (++sync->players_reading == 1) {
        //     printf("[view] Soy el primer lector, bloqueo al master\n");
        //     sem_wait(&sync->C); // primer lector bloquea al máster
        // }
        // sem_post(&sync->D);
        // sem_post(&sync->E);
        // printf("[view] Terminé protocolo de lectura\n");

        // 3. Leer e imprimir el estado del juego
        // printf("[view] Voy a imprimir el estado del juego\n");
        
        if(game_state->game_over){
            printf("[view] El juego ha terminado\n");
            break;
        }

        // Imprimir encabezado
        printf("\n   ");
        for (int x = 0; x < width; x++) {
            printf("%4d", x);
        }
        printf("\n");

        // Imprimir tablero y estado del juego
        print_table(width, height, game_state);
        printf("\n");
        print_players_info(game_state);
        printf("\n");


        // // 4. Fin de lectura segura
        // sem_wait(&sync->D);
        // if (--sync->players_reading == 0)
        //     sem_post(&sync->C); // último lector libera al máster
        // sem_post(&sync->D);

        // 5. Avisar al máster que se terminó de imprimir
        sem_post(&sync->print_done);

    }

    return 0;
}