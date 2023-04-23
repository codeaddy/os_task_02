#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/semaphore.h>
#include <sys/wait.h>
#include <sys/sem.h>

#define WAITING_ROOM_SIZE 5

typedef struct {
    int waiting_clients;
    int is_barber_asleep;
} SharedData;

SharedData *shared_data;
int shared_memory_id;
int sem_id;

void close_sem_data() {
    shmdt(shared_data);
    shmctl(shared_memory_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    exit(0);
}

void lock_sem() {
    struct sembuf operation = {0, -1, 0};
    semop(sem_id, &operation, 1);
}

void unlock_sem() {
    struct sembuf operation = {0, 1, 0};
    semop(sem_id, &operation, 1);
}

void barber() {
    while (1) {
        lock_sem();

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

        unlock_sem();
        sleep(4);
    }
}

void client() {
    lock_sem();

    if (shared_data->waiting_clients < WAITING_ROOM_SIZE) {
        if (shared_data->is_barber_asleep) {
            printf("Client woke up the barber.\n");
        } else {
            printf("Client is waiting in the waiting room.\n");
        }
        fflush(stdout);
        shared_data->waiting_clients++;
        printf("Total numbers of clients in the waiting room - %d\n", shared_data->waiting_clients);
    } else {
        printf("No place in the waiting room. Client is leaving.\n");
        fflush(stdout);
    }

    unlock_sem();
}

int main() {
    signal(SIGINT, close_sem_data);

    key_t key = ftok("barber.c", 1);

    shared_memory_id = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    if (shared_memory_id == -1) {
        perror("shmget");
        exit(1);
    }

    shared_data = shmat(shared_memory_id, NULL, 0);
    shared_data->waiting_clients = 0;
    shared_data->is_barber_asleep = 1;

    sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    semctl(sem_id, 0, SETVAL, 1);

    pid_t barber_pid = fork();
    if (barber_pid == 0) {
        barber();
        exit(0);
    }

    srand(time(NULL));
    while (1) {
        sleep(rand() % 4 + 1);
        if (fork() == 0) {
            client();
            exit(0);
        }
    }

    wait(NULL);
    close_sem_data();
    return 0;
}
