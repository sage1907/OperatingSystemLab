#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 100
#define MAX_ARGS 10

void run_command(char *cmd);
char *trim_whitespace(char *str);
void execute_commands(char *cmd);

int main(int argc, char *argv[]) {
    char input[MAX_CMD_LEN];

    // Interactive mode
    if (argc == 1) {
        while (1) {
            printf("mysh> ");
            fgets(input, sizeof(input), stdin);
            if (strcmp(input, "exit\n") == 0) {
                break;
            }
            execute_commands(input);
        }
    }
    // Batch mode
    else if (argc == 2) {
        FILE *batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            perror("Error opening file");
            return 1;
        }

        while (fgets(input, sizeof(input), batch_file) != NULL) {
            execute_commands(input);
        }

        fclose(batch_file);
    } else {
        printf("Usage: %s [batch_file]\n", argv[0]);
        return 1;
    }

    return 0;
}

void run_command(char *cmd) {
    char *args[MAX_ARGS];
    int arg_count = 0;

    // Tokenize the command into arguments
    char *token = strtok(cmd, " \n");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \n");
    }
    args[arg_count] = NULL;

    // Execute internal commands
    if (strcmp(args[0], "cd") == 0) {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd");
        }
    } else if (strcmp(args[0], "clear") == 0) {
        system("clear");
    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else { // Execute external commands
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        } else if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(1);
            }
        } else {
            wait(NULL);
        }
    }
}

char *trim_whitespace(char *str) {
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    if (*str == '\0') {
        return str;
    }
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) {
        end--;
    }
    *(end + 1) = '\0';
    return str;
}

void execute_commands(char *cmd) {
    // Parse commands separated by semicolon
    char *saveptr;
    char *token = strtok_r(cmd, ";", &saveptr);
    while (token != NULL) {
        token = trim_whitespace(token);
        if (strlen(token) > 0) {
            run_command(token);
        }
        wait(NULL);
        token = strtok_r(NULL, ";", &saveptr);
    }
}
