#include "master_context.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

void master_context_init(MasterContext *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->width = 10;
    ctx->height = 10;
    ctx->delay = 200;
    ctx->timeout = 10;
    ctx->seed = (unsigned int)time(NULL);
}

void master_context_free(MasterContext *ctx) {
    if (ctx == NULL) {
        return;
    }
    free(ctx->view_path);
    ctx->view_path = NULL;
}
