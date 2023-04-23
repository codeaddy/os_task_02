#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

#define WAITING_ROOM_SIZE 5

typedef struct {
    int waiting_clients;
    int is_barber_asleep;
} SharedData;

int sem_id, shm_id;
SharedData *shared_data;

void up(int sem_id) {
    struct sembuf op = {0, 1, 0};
    semop(sem_id, &op, 1);
}

void down(int sem_id) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

void close_sem_data(int signum) {
    printf("Barber: Shutting down\n");

    shmdt(shared_data);
    shmctl(shm_id, IPC_RMID, NULL);

    semctl(sem_id, 0, IPC_RMID);

    exit(0);
}

int main() {
    signal(SIGINT, close_sem_data);

    key_t key = ftok("barber.c", 1);
    shm_id = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
    shared_data = (SharedData *) shmat(shm_id, NULL, 0);

    key_t sem_key = ftok("barber.c", 2);
    sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
    union semun init;
    init.val = 1;
    semctl(sem_id, 0, SETVAL, init);

    shared_data->waiting_clients = 0;
    shared_data->is_barber_asleep = 1;

    while (1) {
        down(sem_id);

        if (shared_data->waiting_clients > 0) {
            shared_data->waiting_clients--;
            printf("Barber is cutting hair.\n");
            sleep(2);
            printf("Barber: Finished cutting hair\n");
            fflush(stdout);
        } else {
            printf("No clients. Barber is sleeping.\n");
            shared_data->is_barber_asleep = 1;
            fflush(stdout);
        }

        up(sem_id);
        sleep(1);
    }

    return 0;
}
