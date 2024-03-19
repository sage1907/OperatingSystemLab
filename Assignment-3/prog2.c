#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_SIZE 10

// shared memory keys
#define SHM_KEY_A 1234
#define SHM_KEY_B 5678
#define SHM_KEY_C 91011

// shared memory IDs
int shm_id_a, shm_id_b, shm_id_c;

// signal handler for cleanup
void cleanup(int signum) {
    shmctl(shm_id_a, IPC_RMID, NULL); // Remove the shared memory segment for A
    shmctl(shm_id_b, IPC_RMID, NULL);
    shmctl(shm_id_c, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);
}

// function to calculate product matrix element
int calculateElement(int row, int col, int m, int *a, int *b) {
    int result = 0;
    for (int k = 0; k < m; k++) {
        result += a[row * m + k] * b[k * m + col];
    }
    return result;
}

int main() {
    signal(SIGINT, cleanup);

    int n, m, p;

    printf("Enter the size of matrix A (n m): ");
    scanf("%d %d", &n, &m);

    printf("Enter the size of matrix B (m p): ");
    scanf("%d %d", &m, &p);

    if (n <= 0 || m <= 0 || p <= 0) {
        fprintf(stderr, "Invalid matrix size. Exiting.\n");
        cleanup(0);
    }

    // create shared memory segments for matrices A, B, and C
    shm_id_a = shmget(SHM_KEY_A, n * m * sizeof(int), IPC_CREAT | 0666);
    shm_id_b = shmget(SHM_KEY_B, m * p * sizeof(int), IPC_CREAT | 0666);
    shm_id_c = shmget(SHM_KEY_C, n * p * sizeof(int), IPC_CREAT | 0666);

    if (shm_id_a == -1 || shm_id_b == -1 || shm_id_c == -1) {
        perror("shmget");
        cleanup(0);
    }

    // attach shared memory segments
    int *a = (int *)shmat(shm_id_a, NULL, 0);
    int *b = (int *)shmat(shm_id_b, NULL, 0);
    int *c = (int *)shmat(shm_id_c, NULL, 0);

    if (a == (void *)-1 || b == (void *)-1 || c == (void *)-1) {
        perror("shmat");
        cleanup(0);
    }

    printf("Enter the elements of matrix A (%d x %d):\n", n, m);
    for (int i = 0; i < n * m; i++) {
        scanf("%d", &a[i]);
    }

    printf("Enter the elements of matrix B (%d x %d):\n", m, p);
    for (int i = 0; i < m * p; i++) {
        scanf("%d", &b[i]);
    }

    // forking n*p child processes for matrix multiplication
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            int pid = fork();

            if (pid == -1) {
                perror("Fork failed");
                cleanup(0);
            }

            if (pid == 0) {
                // child process
                int result = calculateElement(i, j, m, a, b);

                // communicating result to parent process using shared memory
                c[i * p + j] = result;

                exit(EXIT_SUCCESS);
            }
        }
    }

    // parent process waits for all child processes to finish
    for (int i = 0; i < n * p; i++) {
        wait(NULL);
    }

    // parent process prints the product matrix C
    printf("\nProduct Matrix C (%d x %d):\n", n, p);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            printf("%d\t", c[i * p + j]);
        }
        printf("\n");
    }

    // cleanup and detach shared memory
    cleanup(0);
    return 0;
}
