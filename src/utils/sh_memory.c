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
    
    shm_t *shm = malloc(sizeof(shm_t));
    if (shm == NULL) {
        perror("Error al asignar memoria para shm_t");
        exit(EXIT_FAILURE);
    }

    shm->fd = shm_open(name, O_CREAT | O_RDWR, mode);
    if (shm->fd == -1) {
        perror("Error al crear la memoria compartida");
        free(shm);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm->fd, size) == -1) {
        perror("Error al configurar el tamaño de la memoria compartida");
        close(shm->fd);
        free(shm);
        exit(EXIT_FAILURE);
    }

    shm->shm_p = mmap(NULL, size, prot, MAP_SHARED, shm->fd, 0);
    if (shm->shm_p == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        close(shm->fd);
        free(shm);
        exit(EXIT_FAILURE);
    }

    shm->size = size;
    strncpy(shm->name, name, sizeof(shm->name) - 1);
    shm->name[sizeof(shm->name) - 1] = '\0';

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

shm_t *connect_shm(const char *name, size_t size, mode_t mode, int prot) {
    int fd = shm_open(name, mode , 0);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void *shm_p = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    if (shm_p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    shm_t *shm = malloc(sizeof(shm_t));
    shm->size = size;
    strncpy(shm->name, name, sizeof(shm->name));
    shm->shm_p = shm_p; 
    shm->fd = fd;
    return shm;
}

void close_shm(shm_t *shm) {
    if (munmap(shm->shm_p, shm->size) == -1) {
        perror("Error: munmap in close");
        exit(EXIT_FAILURE);
    }
    free(shm);
}


void check_shm(shm_t * shm , char* msg){
    if(shm == NULL || shm->shm_p == NULL){
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void check_shm_ptr(void * shm_p , char* msg){
    if(shm_p == NULL){
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

