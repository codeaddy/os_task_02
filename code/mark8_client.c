#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#define WAITING_ROOM_SIZE 5

typedef struct {
    int waiting_clients;
    int is_barber_asleep;
} SharedData;

void up(int sem_id) {
    struct sembuf op = {0, 1, 0};
    semop(sem_id, &op, 1);
}

void down(int sem_id) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

int main() {
    srand(time(NULL));

    key_t key = ftok("barber.c", 1);
    int shm_id = shmget(key, sizeof(SharedData), 0666);
    SharedData *shared_data = (SharedData *) shmat(shm_id, NULL, 0);

    key_t sem_key = ftok("barber.c", 2);
    int sem_id = semget(sem_key, 1, 0666);

    down(sem_id);

    if (shared_data->waiting_clients < WAITING_ROOM_SIZE) {
        if (shared_data->is_barber_asleep) {
            printf("Client woke up the barber\n");
            shared_data->is_barber_asleep = 0;
        } else {
            printf("Client is waiting in the waiting room\n");
        }
        shared_data->waiting_clients++;
        printf("Total numbers of clients in the waiting room - %d\n", shared_data->waiting_clients);
        fflush(stdout);
    } else {
        printf("No place in the waiting room. Client is leaving.\n");
        fflush(stdout);
    }

    up(sem_id);

    shmdt(shared_data);

    return 0;
}
