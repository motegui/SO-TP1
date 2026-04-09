#ifndef MASTER_CONTEXT_H
#define MASTER_CONTEXT_H

#define MASTER_MAX_PLAYERS 9

typedef struct MasterContext {
    char *players[MASTER_MAX_PLAYERS];
    int player_qty;
    int width;
    int height;
    int delay;
    int timeout;
    unsigned int seed;
    char *view_path;
} MasterContext;

void master_context_init(MasterContext *ctx);
void master_context_free(MasterContext *ctx);

#endif
