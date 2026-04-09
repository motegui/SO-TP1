#include "master_cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef void (*master_option_handler_t)(MasterContext *ctx);

static master_option_handler_t g_option_handlers[256];
static int g_argc;
static char **g_argv;

static void handle_invalid(MasterContext *ctx) {
    (void)ctx;
    const char *prog = (g_argv != NULL && g_argv[0] != NULL) ? g_argv[0] : "master";
    fprintf(stderr,
            "Uso: %s [-w ancho] [-h alto] [-d delay] [-t timeout] [-s seed] [-v vista] "
            "-p player1 [player2 ...]\n",
            prog);
}

static void handle_width(MasterContext *ctx) {
    ctx->width = atoi(optarg);
    if (ctx->width < 10) {
        ctx->width = 10;
    }
}

static void handle_height(MasterContext *ctx) {
    ctx->height = atoi(optarg);
    if (ctx->height < 10) {
        ctx->height = 10;
    }
}

static void handle_delay(MasterContext *ctx) {
    ctx->delay = atoi(optarg);
    if (ctx->delay < 0) {
        fprintf(stderr, "delay debe ser >= 0\n");
        exit(EXIT_FAILURE);
    }
}

static void handle_timeout(MasterContext *ctx) {
    ctx->timeout = atoi(optarg);
    if (ctx->timeout <= 0) {
        fprintf(stderr, "timeout debe ser positivo (segundos)\n");
        exit(EXIT_FAILURE);
    }
}

static void handle_seed(MasterContext *ctx) {
    ctx->seed = (unsigned int)strtoul(optarg, NULL, 10);
}

static void handle_view(MasterContext *ctx) {
    /* strdup es POSIX; con -std=c99 glibc no declara strdup → UB y segfault. */
    size_t n = strlen(optarg) + 1;
    char *copy = malloc(n);
    if (copy == NULL) {
        perror("malloc (-v vista)");
        exit(EXIT_FAILURE);
    }
    memcpy(copy, optarg, n);
    free(ctx->view_path);
    ctx->view_path = copy;
}

static void handle_player(MasterContext *ctx) {
    int argc = g_argc;
    char **argv = g_argv;

    if (ctx->player_qty >= MASTER_MAX_PLAYERS) {
        fprintf(stderr, "Máximo %d jugadores.\n", MASTER_MAX_PLAYERS);
        exit(EXIT_FAILURE);
    }
    ctx->players[ctx->player_qty++] = optarg;
    while (optind < argc && argv[optind][0] != '-') {
        if (ctx->player_qty < MASTER_MAX_PLAYERS) {
            ctx->players[ctx->player_qty++] = argv[optind++];
        } else {
            fprintf(stderr, "Máximo %d jugadores.\n", MASTER_MAX_PLAYERS);
            exit(EXIT_FAILURE);
        }
    }
}

static void master_cli_init_handlers(void) {
    static int inited;

    if (inited) {
        return;
    }
    memset(g_option_handlers, 0, sizeof(g_option_handlers));
    g_option_handlers['w'] = handle_width;
    g_option_handlers['h'] = handle_height;
    g_option_handlers['d'] = handle_delay;
    g_option_handlers['t'] = handle_timeout;
    g_option_handlers['s'] = handle_seed;
    g_option_handlers['v'] = handle_view;
    g_option_handlers['p'] = handle_player;
    inited = 1;
}

int master_cli_parse(MasterContext *ctx, int argc, char *argv[]) {
    master_cli_init_handlers();
    g_argc = argc;
    g_argv = argv;

    int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        if (opt == '?') {
            handle_invalid(ctx);
            return -1;
        }
        unsigned char u = (unsigned char)opt;
        if (g_option_handlers[u] != NULL) {
            g_option_handlers[u](ctx);
        } else {
            handle_invalid(ctx);
            return -1;
        }
    }

    if (ctx->player_qty == 0) {
        fprintf(stderr, "Debe especificar al menos un jugador con -p.\n");
        return -1;
    }

    return 0;
}
