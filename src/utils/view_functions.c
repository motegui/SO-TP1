#include "view_functions.h"


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

