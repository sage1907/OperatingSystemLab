#include <stdio.h> /* needed also for perror() */
#include <errno.h> /* needed for perror() */
#include <unistd.h> /* needed for execve() */

int main(int argc, char *argv[]) {
    int status;
    char *myargv[] = {"./new1", NULL};
   // char *myenv[] = { NULL };

    // int execve(const char *filename, char *const argv[], char *const envp[]);
    status = execve("./new1", myargv, NULL);

    if (status == -1) {
        perror("Exec Fails: ");
    }

    return 0;
}
