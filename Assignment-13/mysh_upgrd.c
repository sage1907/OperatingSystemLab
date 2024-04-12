#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 100
#define MAX_ARGS 10

void run_command(char *cmd);
char *trim_whitespace(char *str);
void execute_commands(char *cmd);
void execute_piped_commands(char *cmd);

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

    // Check for IO redirection
    int in_redirection = 0, out_redirection = 0;
    char *infile = NULL, *outfile = NULL;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            in_redirection = 1;
            infile = args[i + 1];
            args[i] = NULL; // Remove the redirection symbol and file name
        } else if (strcmp(args[i], ">") == 0) {
            out_redirection = 1;
            outfile = args[i + 1];
            args[i] = NULL;
        }
    }

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
    } else { 
	// Execute external commands
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        } else if (pid == 0) {
            // Handle IO redirection
            if (in_redirection) {
                int fd = open(infile, O_RDONLY);
                if (fd == -1) {
                    perror("open");
                    exit(1);
                }
                if (dup2(fd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(fd);
            }
            if (out_redirection) {
                int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd == -1) {
                    perror("open");
                    exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(fd);
            }

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
    // Check if command contains a pipe
    char *pipe_token = strchr(cmd, '|');
    if (pipe_token != NULL) {
        execute_piped_commands(cmd);
    } else {
        run_command(cmd);
    }
}


void execute_piped_commands(char *cmd) {
    char *saveptr;
    char *token = strtok_r(cmd, "|", &saveptr);
    int num_pipes = 0;
    int pipes[MAX_PIPES][2];

    // Count the number of pipes
    char *temp_token = token;
    while (temp_token != NULL) {
        temp_token = strtok_r(NULL, "|", &saveptr);
        num_pipes++;
    }

    // Create pipes
    for (int i = 0; i < num_pipes - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return;
        }
    }

    int prev_pipe_out = STDIN_FILENO;
    int i = 0;
    while (token != NULL) {
        // Trim whitespace from the token
        token = trim_whitespace(token);

        if (strlen(token) > 0) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { // Child process
                // Redirect input and output
                if (i < num_pipes - 1) {
                    if (dup2(prev_pipe_out, STDIN_FILENO) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(pipes[i][0]);
                    if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(pipes[i][1]);
                } else {
                    if (dup2(prev_pipe_out, STDIN_FILENO) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }

                // Execute command
                run_command(token);
                exit(EXIT_SUCCESS);
            } else { // Parent process
                wait(NULL); // Wait for child process to complete
                close(prev_pipe_out);
                if (i < num_pipes - 1) {
                    prev_pipe_out = pipes[i++][0];
                }
            }
        }

        token = strtok_r(NULL, "|", &saveptr);
    }

    // Close remaining pipes
    for (int i = 0; i < num_pipes - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}


/*
void execute_piped_commands(char *cmd) {
    char *saveptr;
    char *token = strtok_r(cmd, "|", &saveptr);
    int pipe_fds[2];
    pid_t pid;

    // Create pipe
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return;
    }

    while (token != NULL) {
        // Trim whitespace from the token
        token = trim_whitespace(token);
        if (strlen(token) > 0) {
            pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { // Child process
                // Redirect output of previous command to input of next command
                if (token != NULL && token[strlen(token) - 1] == '\n') {
                    token[strlen(token) - 1] = '\0'; // Remove newline character
                }
                if (dup2(pipe_fds[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(pipe_fds[1]); // Close unused write end of pipe

                // Execute command
                run_command(token);
                exit(EXIT_SUCCESS);
            } else { // Parent process
                wait(NULL); // Wait for child process to complete
            }
        }
        token = strtok_r(NULL, "|", &saveptr);
    }
    close(pipe_fds[0]);
    close(pipe_fds[1]);
}
*/
