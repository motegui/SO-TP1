

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

    
void main(){
    GameState_t game_state = (GameState_t *) createSHM();
}

void *p = shm_open("/game_state", ...);
