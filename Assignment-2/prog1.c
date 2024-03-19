#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int n = argc;
    int status;
    char *myargv[] = { NULL };

    for (int i = 1; i < n; i++) {
        int pid = fork();

        if (pid == 0) {
            // Child process
            status = execve(argv[i], myargv, NULL);
            perror("Exec failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process waits for all child processes to finish
    for (int i = 1; i < n; i++) {
        wait(&status);
    }

    // Continue with the main program logic here
    printf("All executables have been executed. New command line opens for the user.\n");

    return 0;
}
