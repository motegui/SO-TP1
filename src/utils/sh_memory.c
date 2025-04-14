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


shm_t *create_shm(char *name, size_t size, mode_t mode, int prot) {
    int fd = shm_open(name, O_RDWR | O_CREAT, mode);
    if (fd == -1) {
        perror("[sh_memory -> create_shm] Error: shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, size) == -1) {
        perror("[sh_memory -> create_shm] Error: ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("[sh_memory -> create_shm] Error: mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    shm_t *shm = malloc(sizeof(shm_t));

    if (!shm) {
        perror("[sh_memory -> create_shm] Error: malloc");
        munmap(p, size);
        close(fd);
        exit(EXIT_FAILURE);
    }

    shm->shm_p = p;
    shm->size = size;
    strncpy(shm->name, name, sizeof(shm->name));

    return shm;
}


void delete_shm(shm_t *p){
    if(!p){
        return;
    }

    if(sem_destroy(&p->sem) == -1){
        perror("[sh_memory -> connect_shm] Error: sem_destroy");
        exit(EXIT_FAILURE);
    }

    if(p->shm_p && munmap(p->shm_p, p->size) == -1){
        perror("[sh_memory -> delete_shm] Error: munmap");
        exit(EXIT_FAILURE);
    }

    if(shm_unlink(p->name) == -1){
        perror("[sh_memory -> delete_shm] Error: shm_unlink");
        exit(EXIT_FAILURE);
    }
    free(p);
}

shm_t *connect_shm(const char *name, size_t size, mode_t mode, int prot) {
    int fd = shm_open(name, mode , 0);
    if (fd == -1) {
        perror("[sh_memory -> connect_shm] Error: shm_open");
        exit(EXIT_FAILURE);
    }

     void *shm_p = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    if (shm_p == MAP_FAILED) {
        perror("[sh_memory -> connect_shm] Error: mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    shm_t *shm = malloc(sizeof(shm_t));
    shm->size = size;
    strncpy(shm->name, name, sizeof(shm->name));
    shm->shm_p = shm_p; 

    close(fd);
    return shm;
}

void close_shm(shm_t *shm) {
    if(!shm){
        return;
    }

    if (munmap(shm->shm_p, shm->size) == -1) {
        perror("[sh_memory -> close_shm] Error: munmap in close");
        exit(EXIT_FAILURE);
    }
    free(shm);
}


void check_shm(shm_t * shm , char* msg){
    if(shm == NULL){
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
    }
}


