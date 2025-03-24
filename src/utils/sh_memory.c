#include <fcntl.h>      // For O_* constants
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "/Users/motegui/Desktop/SO-TP1/src/include/sh_memory.h"


void *createSHM(char *name, size_t size) {
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, 0666); // mode solo para crearla
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Solo para crearla
    if (-1 == ftruncate(fd, size)) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}

int main(void) {
    // int x;
    // int *p = &x;
    // int *p = (int *) malloc(sizeof(int));
    // if (!p) exit(EXIT_FAILURE);

    int *p = (int *) createSHM("/my_shm", sizeof(int));
    *p = 5;

    pid_t pid;

    sem_t *print_needed = (sem_t *) createSHM("/print_needed", sizeof(sem_t));
    if (-1 == sem_init(print_needed, 1, 0)) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    sem_t *print_done = (sem_t *) createSHM("/print_done", sizeof(sem_t));
    if (-1 == sem_init(print_done, 1, 0)) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    switch (pid) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);

        case 0:
            for (int i = 0; i < 3; i++) {
                sem_wait(print_needed);
                printf(" p: %d\n", *p);
                sem_post(print_done);
            }
            exit(EXIT_SUCCESS);

        default:
            for (int i = 0; i < 3; i++) {
                *p = i;
                printf("p: %d\n", i);
                sem_post(print_needed);
                sem_wait(print_done);
            }
            exit(EXIT_SUCCESS);
    }
}