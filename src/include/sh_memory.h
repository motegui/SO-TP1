#ifndef SH_MEMORY_H
#define SH_MEMORY_H

#include <stdbool.h>
#include <semaphore.h>
#include <stddef.h>
#include <sys/mman.h>
#include <fcntl.h>

typedef struct {
    char name[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int invalid_moves; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int valid_moves; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y; // Coordenadas x e y en el tablero
    pid_t pid; // Identificador de proceso
    bool blocked; // Indica si el jugador está bloqueado
} Player_t;

typedef struct {
    unsigned short width, height; // Ancho,alto del tablero
    unsigned int player_qty; // Cantidad de jugadores
    Player_t players[9]; // Lista de jugadores
    bool game_over; // Indica si el juego se ha terminado
    int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState_t;


typedef struct {
    sem_t pending_print; // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t print_done; // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t master_turn_mutex; // Mutex para evitar inanición del master al acceder al estado
    sem_t readers_count_mutex; // Mutex para el estado del juego
    sem_t state_access_mutex; // Mutex para la siguiente variable
    unsigned int players_reading; // Cantidad de jugadores leyendo el estado
} Sync_t;

typedef struct shm_t{
    void *shm_p;
    char name[64];
    size_t size;
    sem_t sem;
}shm_t;

shm_t *create_shm(char *name, size_t size, mode_t mode, int prot);
void delete_shm(shm_t *p);
shm_t *connect_shm(const char *name, size_t size, mode_t mode, int prot);
void close_shm(shm_t * shm_p);

#endif
