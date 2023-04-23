#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
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

void close_sem_data() {
    munmap(shared_data, sizeof(SharedData));
    shm_unlink(SHARED_MEMORY_NAME);
    sem_close(semaphore);
    sem_unlink(SEMAPHORE_NAME);
    exit(0);
}

void barber() {
    while (1) {
        sem_wait(semaphore);

        if (shared_data->waiting_clients > 0) {
            shared_data->waiting_clients--;
            printf("Barber is cutting hair.\n");
            fflush(stdout);
            shared_data->is_barber_asleep = 0;
        } else {
            printf("No clients. Barber is sleeping.\n");
            fflush(stdout);
            shared_data->is_barber_asleep = 1;
        }

        sem_post(semaphore);
        sleep(2);
    }
}

int main() {
    signal(SIGINT, close_sem_data);

    shared_memory_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (shared_memory_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    ftruncate(shared_memory_fd, sizeof(SharedData));
    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);
    shared_data->waiting_clients = 0;
    shared_data->is_barber_asleep = 1;

    semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0666, 1);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    barber();

    close_sem_data();
    return 0;
}
