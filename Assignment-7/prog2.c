#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define BUF_SIZE 100

int pipefds[2];

struct student {
    int id;
    char name[20];
};

void enqueue(void *data, size_t size);
void dequeue(void *data, size_t size);
void f1();
void f2();

int main() {
    if (pipe(pipefds) == -1) {
        perror("pipe() failed:");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork() failed:");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) { // Parent process
        printf("Parent process (PID: %d) enqueues data:\n", getpid());
        f1();
        wait(NULL); // Wait for the child process to finish
    } else { // Child process
        printf("Child process (PID: %d) dequeues data:\n", getpid());
        f2();
    }

    return 0;
}

void enqueue(void *data, size_t size) {
    if (write(pipefds[1], data, size) == -1) {
        perror("write() failed:");
        exit(EXIT_FAILURE);
    }
}

void dequeue(void *data, size_t size) {
    if (read(pipefds[0], data, size) == -1) {
        perror("read() failed:");
        exit(EXIT_FAILURE);
    }
}


void f1() {
    int intData;
    float floatData;
    double doubleData;
    char charData;

    // Prompt the user to enter values for each data type
    printf("Enter an integer: ");
    scanf("%d", &intData);
    printf("Enter a float: ");
    scanf("%f", &floatData);
    printf("Enter a double: ");
    scanf("%lf", &doubleData);
    printf("Enter a character: ");
    scanf(" %c", &charData);

    // Enqueue struct student data
    struct student studentData;

    printf("Enter student ID: ");
    scanf("%d", &studentData.id);
    printf("Enter student name: ");
    scanf("%s", studentData.name);

    // Enqueue user-defined data
    enqueue(&intData, sizeof(int));

    enqueue(&floatData, sizeof(float));

    enqueue(&doubleData, sizeof(double));

    enqueue(&charData, sizeof(char));

    enqueue(&studentData, sizeof(struct student));
    printf("Enqueued student: ID: %d, Name: %s\n", studentData.id, studentData.name);
}



void f2() {
    int intData;
    float floatData;
    double doubleData;
    char charData;

    struct student studentData;

    // Dequeue and print the data
    dequeue(&intData, sizeof(int));
    printf("Dequeued integer: %d\n", intData);

    dequeue(&floatData, sizeof(float));
    printf("Dequeued float: %.2f\n", floatData);

    dequeue(&doubleData, sizeof(double));
    printf("Dequeued double: %f\n", doubleData);

    dequeue(&charData, sizeof(char));
    printf("Dequeued char: %c\n", charData);

    // Dequeue struct student data
    dequeue(&studentData, sizeof(struct student));
    printf("Dequeued student: ID: %d, Name: %s\n", studentData.id, studentData.name);
}