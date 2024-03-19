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

int compare_int(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

void cleanup(int signum) {
    shmdt(task_ptr);
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    key_t key = ftok("/tmp/", 4);
    if (key == -1) {
        perror("ftok() error in worker\n");
        exit(0);
    }

    shmid = shmget(key, sizeof(Task), 0777);
    if (shmid == -1) {
        perror("Worker ---> shmget(): error\n");
        exit(0);
    }

    task_ptr = (Task *)shmat(shmid, NULL, 0);
    if (task_ptr == (void *)-1) {
        perror("shmat() error in worker");
        exit(-1);
    }

    while (task_ptr->status != -1) {
        if (task_ptr->status == 1) {
            printf("Worker with PID %d is sorting array [%d]\n", getpid(), task_ptr->sequence_no);
            task_ptr->status = 2;
            qsort(task_ptr->data, task_ptr->size, sizeof(int), compare_int);
            task_ptr->status = 3;
            task_ptr->worker_pid = getpid();
        }
        usleep(200000); // Worker responds slowly to give chance to other workers
    }

    printf("Server has stopped responding\n");
    return 0;
}
