#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ARGS 10  // Maximum number of arguments for each executable

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s executable1 [arg1 arg2 ...] executable2 [arg1 arg2 ...] ... executableN [arg1 arg2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int argIndex = 1;

    while (argIndex < argc) {
        int argCount = 0;
        while (argIndex < argc && argv[argIndex][0] != '-') {
            argCount++;
            argIndex++;
        }

        char *myargv[MAX_ARGS + 1];  // +1 for the NULL at the end

        if (argCount > MAX_ARGS) {
            fprintf(stderr, "Too many arguments for one executable (max %d)\n", MAX_ARGS);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < argCount; i++) {
            myargv[i] = argv[argIndex - argCount + i];
        }
        myargv[argCount] = NULL;

        int pid = fork();

        if (pid == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // This code runs in the child process
            execvp(myargv[0], myargv);
            perror("Exec failed");
            exit(EXIT_FAILURE);
        }

        // Move to the next set of arguments
        argIndex++;
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0) {}

    return 0;
}
