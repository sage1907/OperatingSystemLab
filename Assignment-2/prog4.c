#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_SIZE 10

int main() {
    int m, n, r;
    int status;

    // Read matrix sizes
    printf("Enter the size of matrix A (m n): ");
    scanf("%d %d", &m, &n);

    printf("Enter the size of matrix B (n r): ");
    scanf("%d %d", &n, &r);

    if (n <= 0) {
        fprintf(stderr, "Invalid matrix size. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize matrices A and B
    int A[MAX_SIZE][MAX_SIZE];
    int B[MAX_SIZE][MAX_SIZE];

    // Read matrix A
    printf("Enter the elements of matrix A (%d x %d):\n", m, n);
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            scanf("%d", &A[i][j]);

    // Read matrix B
    printf("Enter the elements of matrix B (%d x %d):\n", n, r);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < r; j++)
            scanf("%d", &B[i][j]);

    // Create child processes for matrix multiplication
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < r; j++) {
            int pid = fork();

            if (pid == -1) {
                perror("Fork failed");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                // Child process
                int result = 0;
                for (int k = 0; k < n; k++) {
                    result += A[i][k] * B[k][j];
                }

                // Communicate result to parent process using exit status
                exit(result);
            }
        }
    }

    // Parent process waits for all child processes to finish
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < r; j++) {
            pid_t child_pid = wait(&status);
            printf("Result at A[%d][%d] * B[%d][%d] = %d\n", i, j, i, j, WEXITSTATUS(status));
        }
    }

    return 0;
}
