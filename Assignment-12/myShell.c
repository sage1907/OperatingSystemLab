#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGUMENTS 10

void execute_command(char *cmd);
char *trim_whitespace(char *str);
void parse_and_execute(char *cmd);

int main(int argc, char *argv[]) {
    char command[MAX_COMMAND_LENGTH];

    // Interactive mode
    if (argc == 1) {
        while (1) {
            printf("mysh> ");
            fgets(command, sizeof(command), stdin);
            if (strcmp(command, "exit\n") == 0) {
                break;
            }
            parse_and_execute(command);
        }
    }
    // Batch mode
    else if (argc == 2) {
        FILE *batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            perror("Error opening file");
            return 1;
        }

        while (fgets(command, sizeof(command), batch_file) != NULL) {
            parse_and_execute(command);
        }

        fclose(batch_file);
    } else {
        printf("Usage: %s [batch_file]\n", argv[0]);
        return 1;
    }

    return 0;
}

void execute_command(char *cmd) {
    char *args[MAX_ARGUMENTS];
    int i = 0;

    // Tokenize the command into arguments
    char *token = strtok(cmd, " \n");
    while (token != NULL && i < MAX_ARGUMENTS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \n");
    }
    args[i] = NULL;

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

void parse_and_execute(char *cmd) {
    // Parse commands separated by semicolon
    char *saveptr;
    char *token = strtok_r(cmd, ";", &saveptr);
    while (token != NULL) {
        token = trim_whitespace(token);
        if (strlen(token) > 0) {
            execute_command(token);
        }
        wait(NULL);
        token = strtok_r(NULL, ";", &saveptr);
    }
}
