#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "sh_memory.h"


// Función para obtener el índice del jugador según su pid
int get_player_id(GameState_t *game_state) {
    pid_t my_pid = getpid();
    for (int i = 0; i < game_state->player_qty; i++) {
        if (game_state->players[i].pid == my_pid) {
            return i;
        }
    }
    return -1; // no encontrado
}

bool hit_border(int x, int y, int dx, int dy, int width, int height) {
    int nx = x + dx;
    int ny = y + dy;
    return (nx <= 0 || nx >= width || ny <= 0 || ny >= height);
}

int main(int argc, char *argv[]) {
    printf("[player] Hola! Soy un jugador. Me pasaron: %s %s\n", argv[1], argv[2]);

    // 1. Conexión a memoria compartida
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    size_t board_size = width * height * sizeof(int);
    size_t game_state_size = sizeof(GameState_t) + board_size;

    shm_t *state_shm = connect_shm("/game_state", game_state_size, O_RDONLY, PROT_READ);
    check_shm(state_shm, "Error al conectar a la memoria compartida del estado del juego");

    GameState_t *game_state = (GameState_t *) state_shm->shm_p;

    shm_t *sync_shm = connect_shm("/game_sync", sizeof(Sync_t), O_RDWR, PROT_READ | PROT_WRITE);
    check_shm(sync_shm, "Error al conectar a la memoria compartida de sincronización");


    Sync_t *sync = (Sync_t *) sync_shm->shm_p;

    // 2. Identificarme en el array de jugadores
    int id = get_player_id(game_state);
  
    int dx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
    int dy[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };
    
    if (id == -1) {
        printf("[player] No encontré mi PID en la lista!\n");
        exit(EXIT_FAILURE);
    }

    sem_wait(&sync->state_access_mutex);
    printf("[player] Soy el jugador #%d (PID %d)\n", id, getpid());
    sem_post(&sync->state_access_mutex);


    int best_dir = 1;

    int dir_vec[3] = {-1, 0, 1};
    int dir_x_idx = 2; 
    int dir_y_idx = 0; 
    

    // 3. Bucle principal hasta que se termine el juego
    while (!game_state->game_over) {
        usleep(300000); 

        // 4. Protocolo de escritura segura
        sem_wait(&sync->master_turn_mutex); // Turno de escritura
        sem_wait(&sync->state_access_mutex); // Exclusión mutua total

        Player_t *p = &game_state->players[id];
    
        int x = p -> x;
        int y = p -> y;

        int best_value = -1; // best_value esta en -1 porque  me sirve para dsp decir acepto cualq valor >-1 como mejor candidato
        int best_dir = -1; //nos sirve para detectar si no se encontro ningun movimient0
        for(int d=0; d<8 ; d++){
            int nx= x+dx[d];
            int ny= y+dy[d];

            if(nx>=0 && nx<width && ny>=0 && ny<height){
                int val = game_state-> board[ny*width + nx];

                if (val >= 1 && val <= 9 && val > best_value) {
                    best_value = val;
                    best_dir = d;
                }
            }
        }
        if(best_value > 0 && best_dir != -1){
            unsigned char dir; //esto es porque se espera un valor de 0-7 de adonde se quieren mover
            dir = (unsigned char) best_dir; //lo casteo para q ocupe exactamente un byte
            write(1, &dir,1); //envio la direc al pipe de salida
            fprintf(stderr, "[player] Me muevo a dir %d con valor %d\n", best_dir, best_value);
        }        
        else{
            p->blocked =true;
            fprintf(stderr, "[player] Estoy bloqueado, no hay movimiento válido.\n");
        }
    
        // 6. Fin
        sem_post(&sync->state_access_mutex); // Fin de escritura
        sem_post(&sync->master_turn_mutex); // Fin del turno de escritura
    }
    
    fprintf(stderr,"[player] El juego terminó. Salgo!\n");

    close_shm(state_shm);
    close_shm(sync_shm);

    return 0;
}