#include "master_cli.h"
#include "master_context.h"
#include "master_run.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    MasterContext ctx;

    master_context_init(&ctx);
    if (master_cli_parse(&ctx, argc, argv) != 0) {
        master_context_free(&ctx);
        return EXIT_FAILURE;
    }

    int rc = master_run(&ctx);
    master_context_free(&ctx);
    return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
