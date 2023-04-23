#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>

#define WAITING_ROOM_SIZE 5
#define SHARED_MEMORY_NAME "/barber_shared_memory"
#define SEMAPHORE_NAME "/barber_semaphore"

typedef struct {
    int waiting_clients;
    int is_barber_asleep;
} SharedData;

SharedData *shared_data;
int shared_memory_fd;
sem_t *semaphore;

void client() {
    sem_wait(semaphore);

    if (shared_data->waiting_clients < WAITING_ROOM_SIZE) {
        if (shared_data->is_barber_asleep) {
            printf("Client woke up the barber.\n");
        } else {
            printf("Client is waiting in the waiting room.\n");
        }
        fflush(stdout);
        shared_data->waiting_clients++;
        printf("Total numbers of clients in the waiting room - %d\n", shared_data->waiting_clients);
        fflush(stdout);
    } else {
        printf("No place in the waiting room. Client is leaving.\n");
        fflush(stdout);
    }

    sem_post(semaphore);
}

int main() {
    shared_memory_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    if (shared_memory_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);

    semaphore = sem_open(SEMAPHORE_NAME, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    client();

    munmap(shared_data, sizeof(SharedData));
    close(shared_memory_fd);
    sem_close(semaphore);

    return 0;
}

