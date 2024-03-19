#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>


#define SHM_KEY_N 1234
#define SHM_KEY_R 5678

// shared memory IDs
int shm_id_n, shm_id_r;

// Signal handler for cleanup
void cleanup(int signum) {
    shmctl(shm_id_n, IPC_RMID, NULL); // Remove the shared memory segment for n
    shmctl(shm_id_r, IPC_RMID, NULL); // Remove the shared memory segment for r
    exit(EXIT_SUCCESS);
}


int factorial(int num) {
    if (num == 0 || num == 1) {
        return 1;
    } else {
        return num * factorial(num - 1);
    }
}

int main() {
    signal(SIGINT, cleanup);

    // Creation of shared memory segments
    shm_id_n = shmget(SHM_KEY_N, sizeof(int), IPC_CREAT | 0666);
    shm_id_r = shmget(SHM_KEY_R, sizeof(int), IPC_CREAT | 0666);

    if (shm_id_n == -1 || shm_id_r == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attachment with shared memory segments
    int *n = (int *)shmat(shm_id_n, NULL, 0);
    int *r = (int *)shmat(shm_id_r, NULL, 0);

    if (n == (void *)-1 || r == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

   
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        cleanup(0);
    }

    if (pid == 0) {
        // Child process
        while (1) {
            int local_n = *n; // Take a local copy of n
            printf("Child: Calculating factorial of %d\n", local_n);
            *r = factorial(local_n);
            sleep(3); // Wait for 3 seconds
        }
    } else {
        // Parent process
        while (1) {
            *n = rand() % 10; 
            printf("Parent: Setting n to %d\n", *n);
            sleep(2); 
            printf("Parent: Factorial of %d is %d\n", *n, *r);
            sleep(2); 
        }
    }

    return 0;
}
