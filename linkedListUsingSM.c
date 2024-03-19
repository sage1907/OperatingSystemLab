#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_NODES 100

struct Node {
    int data;
    int next;
};

struct SharedMemory {
    int head;
    int projectId;
    struct Node nodes[MAX_NODES];
};

int semId; // Semaphore ID

void waitSemaphore() {
    struct sembuf semOp;
    semOp.sem_num = 0;
    semOp.sem_op = -1;
    semOp.sem_flg = 0;
    semop(semId, &semOp, 1);
}

void signalSemaphore() {
    struct sembuf semOp;
    semOp.sem_num = 0;
    semOp.sem_op = 1;
    semOp.sem_flg = 0;
    semop(semId, &semOp, 1);
}

void insertNode(struct SharedMemory *shm, int data) {
    waitSemaphore();

    int newNodeId = shm->projectId;
    shm->projectId++;

    struct Node newNode;
    newNode.data = data;
    newNode.next = shm->head;

    shm->head = newNodeId;
    shm->nodes[newNodeId] = newNode;

    signalSemaphore();
}

int searchNode(struct SharedMemory *shm, int data) {
    waitSemaphore();

    int currentNodeId = shm->head;

    while (currentNodeId != -1) {
        if (shm->nodes[currentNodeId].data == data) {
            signalSemaphore();
            return currentNodeId;
        }
        currentNodeId = shm->nodes[currentNodeId].next;
    }

    signalSemaphore();
    return -1; // Node not found
}

void deleteNode(struct SharedMemory *shm, int data) {
    waitSemaphore();

    int currentNodeId = shm->head;
    int prevNodeId = -1;

    while (currentNodeId != -1 && shm->nodes[currentNodeId].data != data) {
        prevNodeId = currentNodeId;
        currentNodeId = shm->nodes[currentNodeId].next;
    }

    if (currentNodeId != -1) {
        if (prevNodeId == -1) {
            // Node to be deleted is the head
            shm->head = shm->nodes[currentNodeId].next;
        } else {
            // Node to be deleted is not the head
            shm->nodes[prevNodeId].next = shm->nodes[currentNodeId].next;
        }
    }

    signalSemaphore();
}

void displayList(struct SharedMemory *shm) {
    waitSemaphore();

    int currentNodeId = shm->head;

    while (currentNodeId != -1) {
        printf("%d ", shm->nodes[currentNodeId].data);
        currentNodeId = shm->nodes[currentNodeId].next;
    }

    printf("\n");

    signalSemaphore();
}

void processMenu(struct SharedMemory *shm) {
    int choice, value;

    do {
        printf("\n1. Insert\n2. Delete\n3. Search\n4. Display\n5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter the value to insert: ");
                scanf("%d", &value);
                insertNode(shm, value);
                break;
            case 2:
                printf("Enter the value to delete: ");
                scanf("%d", &value);
                deleteNode(shm, value);
                break;
            case 3:
                printf("Enter the value to search: ");
                scanf("%d", &value);
                int result = searchNode(shm, value);

                if (result != -1) {
                    printf("Value %d found at Node %d\n", value, result);
                } else {
                    printf("Value %d not found\n", value);
                }
                break;
            case 4:
                printf("Linked List: ");
                displayList(shm);
                break;
            case 5:
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 5);
}

int main() {
    key_t shmKey = ftok("shared_memory_key", 'R');
    key_t semKey = ftok("semaphore_key", 'S');

    int shmId = shmget(shmKey, sizeof(struct SharedMemory), IPC_CREAT | 0666);
    semId = semget(semKey, 1, IPC_CREAT | 0666);

    if (shmId == -1 || semId == -1) {
        perror("Error creating shared memory or semaphore");
        exit(EXIT_FAILURE);
    }

    struct SharedMemory *shm = shmat(shmId, NULL, 0);
    shm->head = -1;
    shm->projectId = 0;

    // Initialize semaphore
    semctl(semId, 0, SETVAL, 1);

    int numProcesses = 3; // Number of processes (including the main process)
    pid_t childPids[numProcesses];

    for (int i = 0; i < numProcesses - 1; ++i) {
        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            processMenu(shm);
            exit(0);
        } else if (pid > 0) {
            // Parent process
            childPids[i] = pid;
        } else {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
    }

    // Main process also runs the menu
    processMenu(shm);

    // Wait for all child processes to finish
    for (int i = 0; i < numProcesses - 1; ++i) {
        waitpid(childPids[i], NULL, 0);
    }

    // Cleanup
    shmdt(shm);
    shmctl(shmId, IPC_RMID, NULL);
    semctl(semId, 0, IPC_RMID);

    return 0;
}