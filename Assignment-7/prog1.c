#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAX_VALUES 10
#define BUF_SIZE 100

int pipefds[2];

void enqueue(void *data, size_t size, int numElements);
void dequeue(void *data, size_t size, int numElements);
void f1();
void f2();

int main() {
    if (pipe(pipefds) == -1) {
        perror("pipe() failed:");
        exit(EXIT_FAILURE);
    }

    f1();
    f2();

    return 0;
}

void enqueue(void *data, size_t size, int numElements) {
    if (write(pipefds[1], data, size * numElements) == -1) {
        perror("write() failed:");
        exit(EXIT_FAILURE);
    }
}

void dequeue(void *data, size_t size, int numElements) {
    if (read(pipefds[0], data, size * numElements) == -1) {
        perror("read() failed:");
        exit(EXIT_FAILURE);
    }
}

void f1() {
    int intData[MAX_VALUES];
    float floatData[MAX_VALUES];
    double doubleData[MAX_VALUES];
    char charData[MAX_VALUES];

    int numValues;
    printf("Enter the number of values (up to %d): ", MAX_VALUES);
    scanf("%d", &numValues);

    printf("Enter %d integers: ", numValues);
    for (int i = 0; i < numValues; ++i)
        scanf("%d", &intData[i]);

    printf("Enter %d floats: ", numValues);
    for (int i = 0; i < numValues; ++i)
        scanf("%f", &floatData[i]);

    printf("Enter %d doubles: ", numValues);
    for (int i = 0; i < numValues; ++i)
        scanf("%lf", &doubleData[i]);

    printf("Enter %d characters: ", numValues);
    for (int i = 0; i < numValues; ++i)
        scanf(" %c", &charData[i]);

    // Enqueue different types of data
    enqueue(intData, sizeof(int), numValues);
    enqueue(floatData, sizeof(float), numValues);
    enqueue(doubleData, sizeof(double), numValues);
    enqueue(charData, sizeof(char), numValues);

    // Dequeue and print the data
    printf("Data dequeued by f1():\n");

    int numElementsToDequeue;
    printf("Enter the number of elements to dequeue: ");
    scanf("%d", &numElementsToDequeue);

    dequeue(intData, sizeof(int), numElementsToDequeue);
    printf("Integers: ");
    for (int i = 0; i < numElementsToDequeue; ++i)
        printf("%d ", intData[i]);
    printf("\n");

    dequeue(floatData, sizeof(float), numElementsToDequeue);
    printf("Floats: ");
    for (int i = 0; i < numElementsToDequeue; ++i)
        printf("%.2f ", floatData[i]);
    printf("\n");

    dequeue(doubleData, sizeof(double), numElementsToDequeue);
    printf("Doubles: ");
    for (int i = 0; i < numElementsToDequeue; ++i)
        printf("%f ", doubleData[i]);
    printf("\n");

    dequeue(charData, sizeof(char), numElementsToDequeue);
    printf("Characters: ");
    for (int i = 0; i < numElementsToDequeue; ++i)
        printf("%c ", charData[i]);
    printf("\n");
}

void f2() {
    struct {
        int id;
        char name[20];
    } student[MAX_VALUES];

    int numValues;
    printf("\nEnter the number of students (up to %d): ", MAX_VALUES);
    scanf("%d", &numValues);

    printf("Enter student details:\n");
    for (int i = 0; i < numValues; ++i) {
        printf("Student %d ID: ", i + 1);
        scanf("%d", &student[i].id);
        printf("Student %d Name: ", i + 1);
        scanf("%s", student[i].name);
    }

    // Enqueue struct data
    enqueue(student, sizeof(student[0]), numValues);

    // Dequeue and print the struct data
    printf("\nData dequeued by f2():\n");

    int numElementsToDequeue;
    printf("Enter the number of elements to dequeue: ");
    scanf("%d", &numElementsToDequeue);

    dequeue(student, sizeof(student[0]), numElementsToDequeue);
    printf("Student details:\n");
    for (int i = 0; i < numElementsToDequeue; ++i)
        printf("ID: %d, Name: %s\n", student[i].id, student[i].name);
}