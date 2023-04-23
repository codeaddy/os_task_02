#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define WAITING_ROOM_SIZE 5
#define SEMAPHORE_NAME "/sem"
#define SHARED_MEMORY_NAME "/shared_memory"

typedef struct {
    int waiting_clients;
    int is_barber_asleep;
} SharedData;

sem_t *sem;
SharedData *shared_data;

void close_sem_data() {
    sem_close(sem);
    sem_unlink(SEMAPHORE_NAME);
    munmap(shared_data, sizeof(SharedData));
    shm_unlink(SHARED_MEMORY_NAME);
    exit(0);
}

void barber() {
    while (1) {
        sem_wait(sem);

        if (shared_data->waiting_clients > 0) {
            shared_data->waiting_clients--;
            printf("Barber is cutting hair.\n");
            fflush(stdout);
            shared_data->is_barber_asleep = 0;
            sleep(1);
        } else {
            printf("No clients. Barber is sleeping.\n");
            fflush(stdout);
            shared_data->is_barber_asleep = 1;
        }

        sem_post(sem);
        sleep(2);
    }
}

void client() {
    sem_wait(sem);

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

    sem_post(sem);
}

int main() {
    signal(SIGINT, close_sem_data);

    sem = sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    int fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        sem_close(sem);
        sem_unlink(SEMAPHORE_NAME);
        exit(1);
    }

    ftruncate(fd, sizeof(SharedData));
    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    shared_data->waiting_clients = 0;
    shared_data->is_barber_asleep = 1;

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
