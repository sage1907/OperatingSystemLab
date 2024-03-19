#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define FILE_PATH "/home/sagar/osLab/ass5"
#define PROJ_ID 'S'

struct task {
    int data[100];
    int size;
    pid_t worker_pid;
    int status; /* 0: No data, 1: Data to be sorted, 2: Sorting started, 3: Sorting completed, 4: Server using sorted data, -1: Server terminated */
};

void cleanup(int signum) {
    printf("Worker %d: Terminating\n", getpid());
    exit(EXIT_SUCCESS);
}

void insertionSort(int arr[], int n) {
    int i, key, j;
    for (i = 1; i < n; i++) {
        key = arr[i];
        j = i - 1;

        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

int main() {
    signal(SIGINT, cleanup);

    key_t shm_key = ftok(FILE_PATH, PROJ_ID);
    if (shm_key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int shm_id = shmget(shm_key, sizeof(struct task), 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    struct task *solve = (struct task *)shmat(shm_id, NULL, 0);
    if (solve == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    while (solve->status != -1) {
        while (solve->status != 1 && solve->status != -1) {
            sleep(1);
        }

        if (solve->status == -1) {
            // Server terminated
            cleanup(0);
        }

        solve->status = 2;
        solve->worker_pid = getpid();
        printf("Worker %d: Sorting %d numbers\n", getpid(), solve->size);

        insertionSort(solve->data, solve->size);

        printf("Worker %d: Sorting completed\n", getpid());
        solve->status = 3;
    }

    return 0;
}
