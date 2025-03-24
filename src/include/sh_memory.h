#ifndef SH_MEMORY_H
#define SH_MEMORY_H

#include <stddef.h>

shm_t *create_shm(char *name);
void delete_shm(shm_t *p);
shm_t * connect_shm(const char *shm_name);
void close_shm(shm_t * shm_p);

#endif
