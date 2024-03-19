#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int n = argc;
    int status;
    pid_t child_pid;
    char *myargv[] = {NULL};

    for (int i = 1; i < n; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            status = execve(argv[i], myargv, NULL);
            perror("Exec failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process waits for all child processes to finish
    for (int i = 1; i < n; i++) {
        child_pid = wait(&status);

        if (WIFEXITED(status)) {
            printf("Child process with PID %d terminated normally. Exit status: %d\n", child_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process with PID %d terminated abnormally. Signal: %d\n", child_pid, WTERMSIG(status));
        } else {
            printf("Child process with PID %d did not exit normally or abnormally.\n", child_pid);
        }
    }

    // Continue with the main program logic here
    printf("All executables have been executed. New command line opens for the user.\n");

    return 0;
}
