#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define ARRAY_SIZE 20

typedef struct {
    int data[ARRAY_SIZE];
    int size;
    pid_t worker_pid;
    int status;
    int sequence_no;
} Task;

Task *task_ptr;
int shmid;

void release_shm(int signum) {
    int status = shmctl(shmid, IPC_RMID, NULL);
    task_ptr->status = -1;

    if (status == 0) {
        fprintf(stderr, "Shared memory with id=%d removed\n", shmid);
    } else if (status == -1) {
        fprintf(stderr, "Failed to remove shared memory with id=%d\n", shmid);
    }

    status = kill(0, SIGKILL);

    if (status == 0) {
        fprintf(stderr, "Kill successful\n");
    } else if (status == -1) {
        perror("Kill failed\n");
        fprintf(stderr, "Failed to remove shared memory with id=%d\n", shmid);
    } else {
        fprintf(stderr, "kill(2) returned wrong value\n");
    }
}

int main() {
    signal(SIGINT, release_shm);

    key_t key = ftok("/tmp/", 4);
    if (key == -1) {
        perror("ftok() error in server\n");
        exit(0);
    }

    shmid = shmget(key, sizeof(Task), IPC_CREAT | 0777);
    if (shmid == -1) {
        perror("Server ---> shmget(): error\n");
        exit(0);
    }

    task_ptr = (Task *)shmat(shmid, NULL, 0);
    if (task_ptr == (void *)-1) {
        perror("shmat() error in server");
        exit(0);
    }

    task_ptr->status = 0;

    while (task_ptr->status != -1) {
        if (task_ptr->status == 0) {
            task_ptr->size = ARRAY_SIZE;
            task_ptr->sequence_no++;
            for (int i = 0; i < ARRAY_SIZE; i++) {
                task_ptr->data[i] = rand() % 500;
            }
            task_ptr->status = 1;

            printf("Initial unsorted array [%d]: ", task_ptr->sequence_no);
            for (int i = 0; i < ARRAY_SIZE; i++) {
                printf("%d ", task_ptr->data[i]);
            }
            printf("\n");
        } else if (task_ptr->status == 3) {
            task_ptr->status = 4;
            printf("Array sorted by worker with PID %d\n", task_ptr->worker_pid);
            printf("Sorted array [%d]: ", task_ptr->sequence_no);
            for (int i = 0; i < task_ptr->size; i++) {
                printf("%d ", task_ptr->data[i]);
            }
            printf("\n\n");
        } else if (task_ptr->status == 4) {
            task_ptr->status = 0;
        }
        usleep(10000); // Server feeds data quickly
    }

    return 0;
}
