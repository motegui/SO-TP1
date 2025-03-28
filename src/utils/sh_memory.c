#include <fcntl.h>      // For O_* constants
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sh_memory.h"
#include <string.h>


shm_t *create_shm(char *name, size_t size) {
    shm_unlink(name);
    int fd = shm_open(name, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("Error: shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, size) == -1) {
        perror("Error: ftruncate");
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("Error: mmap");
        exit(EXIT_FAILURE);
    }

    shm_t *shm = malloc(sizeof(shm_t));
    shm->shm_p = p;
    shm->size = size;
    strncpy(shm->name, name, sizeof(shm->name));

    return shm;
}


void delete_shm(shm_t *p){

    if(sem_destroy(&p->sem) == -1){
        perror("Error sem_destroy");
        exit(EXIT_FAILURE);
    }

    if(munmap(p, sizeof(*p)) == -1){
        perror("Error: munmap");
        exit(EXIT_FAILURE);
    }

    if(shm_unlink(p->name) == -1){
        perror("Error: shm_unlink");
        exit(EXIT_FAILURE);
    }
}

shm_t *connect_shm(const char *name, size_t size) {
    int fd = shm_open(name, O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void *shm_p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    shm_t *shm = malloc(size);
    shm->size = size;
    strncpy(shm->name, name, sizeof(shm->name));
    shm->shm_p = shm_p; 
    return shm;
}

void close_shm(shm_t * shm_p){
    if(munmap(shm_p,sizeof(*shm_p)) == -1){
        perror("Error: munmap in close");
        exit(EXIT_FAILURE);
    }

    free(shm_p);
}
