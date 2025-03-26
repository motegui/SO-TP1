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


struct shm_t{
    char *name;
    size_t size;
    sem_t sem;
};


shm_t *create_shm(char *name) {
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, 0666); // mode solo para crearla
    if (fd == -1) {
        perror("Error: shm_open");
        exit(EXIT_FAILURE);
    }

    // Solo para crearla
    if (ftruncate(fd, sizeof(shm_t)) == -1) {
        perror("Error: ftruncate");
        exit(EXIT_FAILURE);
    }

    shm_t *shm_p = mmap(NULL, sizeof(shm_t), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (shm_p == MAP_FAILED) {
        perror("Error: mmap");
        exit(EXIT_FAILURE);
    }

    //le asignamos el nombre al puntero
    strcpy(shm_p->name, name);

    //inicializacion de semaforo con valor 0
    if(sem_init(&shm_p->sem,1,0) == -1){
        perror("Error: sem_init");
        exit(EXIT_FAILURE);
    }

    return shm_p;
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


shm_t * connect_shm(const char *shm_name){
    int fd = shm_open(shm_name, O_RDWR, 0);
    if(fd == -1){
        perror("Error: shm_open in connect_shm");
        exit(EXIT_FAILURE);
    }

    shm_t * shm_p = mmap(NULL, sizeof(*shm_p), PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0);

    if(shm_p == MAP_FAILED){
        perror("Error: mmap in connect_shm");
        exit(EXIT_FAILURE);
    }
    
    return shm_p;
}

void close_shm(shm_t * shm_p){
    if(munmap(shm_p,sizeof(*shm_p)) == -1){
        perror("Error: munmap in close");
        exit(EXIT_FAILURE);
    }
}
