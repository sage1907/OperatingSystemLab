#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>

#define FILE_PATH "/home/sagar/osLab/ass5" 
#define PROJ_ID 'S'

struct task {
    int data[100];
    int size;
    pid_t worker_pid;
    int status; /* 0: No data, 1: Data to be sorted, 2: Sorting started, 3: Sorting completed, 4: Server using sorted data, -1: Server terminated */
};

void cleanup(int signum) {
    printf("Server: Terminating\n");
    exit(EXIT_SUCCESS);
}

int main() {
    signal(SIGINT, cleanup);

    key_t shm_key = ftok(FILE_PATH, PROJ_ID);
    if (shm_key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int shm_id = shmget(shm_key, sizeof(struct task), IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1) {
        shm_id = shmget(shm_key, sizeof(struct task), 0666);
        if (shm_id == -1) {
            perror("shmget");
            exit(EXIT_FAILURE);
        }
    }

    struct task *solve = (struct task *)shmat(shm_id, NULL, 0);
    if (solve == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Wait for data to be sorted
        while (solve->status != 1 && solve->status != -1) {
            sleep(1);
        }

        if (solve->status == -1) {
            // Server terminated
            cleanup(0);
        }

        
        solve->size = rand() % 10 + 1;
        for (int i = 0; i < solve->size; i++) {
            solve->data[i] = rand() % 100;
        }

        printf("Server: Array before sorting: ");
        for (int i = 0; i < solve->size; i++) {
            printf("%d ", solve->data[i]);
        }
        printf("\n");

        printf("Server: Put %d numbers in data[] for sorting\n", solve->size);
        solve->status = 1;

        
        while (solve->status != 3 && solve->status != -1) {
            sleep(1);
        }

        if (solve->status == -1) {
            // Server terminated
            cleanup(0);
        }

        printf("Server: Sorted data[] by Worker %d: ", solve->worker_pid);
        for (int i = 0; i < solve->size; i++) {
            printf("%d ", solve->data[i]);
        }
        printf("\n");

        solve->status = 4;
    }

    return 0;
}
