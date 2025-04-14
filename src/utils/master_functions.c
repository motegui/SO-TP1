#include "master_functions.h"

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
            // game_state->players[i].x = rand() % width; // HAY QUE MEJORARLO
            // game_state->players[i].y = rand() % height; // HAY QUE MEJORARLO
            game_state->players[i].score = 0;
            game_state->players[i].valid_moves = 0;
            game_state->players[i].invalid_moves = 0;
            game_state->players[i].blocked = false;

            snprintf(game_state->players[i].name, sizeof(game_state->players[i].name), "player%d", i + 1);

            // game_state->board[game_state->players[i].y * width + game_state->players[i].x] = -(i + 1);
        }
    }
    distribute_players(game_state, width, height, player_qty);

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
void distribute_players(GameState_t *game_state, int width, int height, int player_qty) {
    int rows = (int)sqrt(player_qty);
    int cols = (player_qty + rows - 1) / rows; // redondeo hacia arriba

    int spacing_x = width / cols;
    int spacing_y = height / rows;

    int player = 0;
    for (int i = 0; i < rows && player < player_qty; i++) {
        for (int j = 0; j < cols && player < player_qty; j++) {
            int x = j * spacing_x + spacing_x / 2;
            int y = i * spacing_y + spacing_y / 2;

            game_state->players[player].x = x;
            game_state->players[player].y = y;
            game_state->players[player].score = 0;
            game_state->players[player].valid_moves = 0;
            game_state->players[player].invalid_moves = 0;
            game_state->players[player].blocked = false;
            snprintf(game_state->players[player].name, sizeof(game_state->players[player].name), "player%d", player + 1);

            game_state->board[y * width + x] = -(player + 1); // Marcar en el tablero
            player++;
        }
    }
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

bool read_players_moves(int pipes[][2], GameState_t *game_state, int dx[], int dy[], int player_qty) {
    static int current_player = 0;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipes[current_player][0], &read_fds);
    int max_fd = pipes[current_player][0];

    int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    if (ready == -1) {
        perror("select");
        return false;
    }

    if (ready == 0) {
        // no hay movimiento
        return false;
    }

    if (FD_ISSET(pipes[current_player][0], &read_fds)) {
        Player_t *p = &game_state->players[current_player];
        unsigned char dir;
        ssize_t bytes_read = read(pipes[current_player][0], &dir, 1);
        if (bytes_read <= 0) {
            p->blocked = true;
            current_player = (current_player + 1) % player_qty;
            return false;
        }

        if (dir > 7) {
            p->invalid_moves++;
        } else {
            int nx = p->x + dx[dir];
            int ny = p->y + dy[dir];

            if (nx < 0 || ny < 0 || nx >= game_state->width || ny >= game_state->height) {
                p->invalid_moves++;
            } else {
                int pos = ny * game_state->width + nx;
                int cell = game_state->board[pos];

                if (cell >= 1 && cell <= 9) {
                    game_state->board[p->y * game_state->width + p->x] = 0; // Limpia la posición anterior
                    p->x = nx;
                    p->y = ny;
                    p->score += cell;
                    game_state->board[pos] = -(current_player + 1); // Marca la nueva posición
                    p->valid_moves++;
                    current_player = (current_player + 1) % player_qty;
                    return true; // hubo movimiento válido
                } else {
                    p->invalid_moves++;
                }
            }
        }
        
    }
    current_player = (current_player + 1) % player_qty;
    return false;
}

void determine_winner(GameState_t *game_state, int player_qty) {
    int winner = -1;
    bool tie = false;

    for (int i = 0; i < player_qty; i++) {
        Player_t *p = &game_state->players[i];
        if (winner == -1 || p->score > game_state->players[winner].score ||
            (p->score == game_state->players[winner].score && p->valid_moves > game_state->players[winner].valid_moves) ||
            (p->score == game_state->players[winner].score && p->valid_moves == game_state->players[winner].valid_moves && p->invalid_moves < game_state->players[winner].invalid_moves)) {
            winner = i;
            tie = false;
        } else if (p->score == game_state->players[winner].score &&
                   p->valid_moves == game_state->players[winner].valid_moves &&
                   p->invalid_moves == game_state->players[winner].invalid_moves) {
            tie = true;
        }
    }

    if (tie) {
        printf("Resultado: ¡Empate!\n");
    } else {
        printf("Ganador: %s 🏆\n", game_state->players[winner].name);
    }
}

bool check_all_players_blocked(GameState_t *game_state, int player_qty) {
    int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

    for (int i = 0; i < player_qty; i++) {
        if (game_state->players[i].blocked) continue;

        int x = game_state->players[i].x;
        int y = game_state->players[i].y;

        for (int d = 0; d < 8; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];

            if (nx < 0 || ny < 0 || nx >= game_state->width || ny >= game_state->height) continue;

            int pos = ny * game_state->width + nx;
            int cell = game_state->board[pos];

            if (cell >= 1 && cell <= 9) {
                return false;  // este jugador puede moverse
            }
        }
    }

    return true;  // ningún jugador tiene movimientos válidos
}