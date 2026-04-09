#include "view_functions.h"

static const char *player_color(int player_id) {
    // player_id: 1..player_qty (el board usa -(player+1) para codificar)
    static const char *colors[] = {RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, WHITE};
    size_t n = sizeof(colors) / sizeof(colors[0]);
    if (player_id <= 0) return RESET;
    return colors[(player_id - 1) % n];
}

static int player_id_at(int x, int y, GameState_t *game_state) {
    // Devuelve i+1 si hay un jugador cuyo (x,y) coincide; si no, 0.
    for (unsigned int i = 0; i < game_state->player_qty; i++) {
        if (game_state->players[i].x == (unsigned short)x && game_state->players[i].y == (unsigned short)y) {
            return (int)i + 1;
        }
    }
    return 0;
}

void print_table(int width, int height, GameState_t *game_state) {
    for (int y = 0; y < height; y++) {
        printf("%3d |", y); // Índice de fila alineado
        for (int x = 0; x < width; x++) {
            int valor = game_state->board[y * width + x];

            if (valor < 0) {
                int owner_id = -valor;
                int current_id = player_id_at(x, y, game_state);
                const char *c = player_color(owner_id);

                if (current_id == owner_id) {
                    printf(BOLD "%s J%-2d" RESET, c, owner_id);
                } else {
                    printf(DIM "%s .. " RESET, c);
                }
            } else if (valor == 0) {
                printf(GRAY " ░░ " RESET);
            } else {
                printf(YELLOW " %2d " RESET, valor);
            }
        }
        printf("\n");
    }
}


void print_players_info(GameState_t *game_state) {
    for (unsigned int i = 0; i < game_state->player_qty; i++) {
        Player_t *p = &game_state->players[i];
        printf("Jugador %s%s%s | Score: %u | Pos: (%u,%u) | Val: %u | Inv: %u | %s\n",
               player_color((int)i + 1), p->name, RESET,
               p->score, (unsigned)p->x, (unsigned)p->y,
               p->valid_moves, p->invalid_moves,
               p->blocked ? "BLOQUEADO" : "ACTIVO");
    }
}
