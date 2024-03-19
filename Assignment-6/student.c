#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>

#define SEM_NAME "/chair_sem"

void enterClassroom(sem_t *chair_sem, int studentNumber) {
    printf("Student %d: Please enter the classroom!\n", studentNumber);
    sem_wait(chair_sem); // Wait for a chair to be free
    printf("Student %d entered the classroom and occupied a chair.\n", studentNumber);
}

void leaveClassroom(sem_t *chair_sem, int studentNumber) {
    printf("Student %d is leaving the classroom.\n", studentNumber);
    sem_post(chair_sem); // Release the chair
}

int main() {
    sem_t *chair_sem;

    // Attempt to unlink the semaphore first
    sem_unlink(SEM_NAME);

    // Create or open the semaphore
    chair_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0777, 3);
    if (chair_sem == SEM_FAILED) {
        if (errno == EEXIST) {
            // Semaphore already exists, try to open it without creating
            chair_sem = sem_open(SEM_NAME, O_RDWR);
            if (chair_sem == SEM_FAILED) {
                perror("sem_open() failed");
                exit(1);
            }
        } else {
            perror("sem_open() failed");
            exit(1);
        }
    }

    int studentNumber; // Unique identifier for each student

    // Get a unique identifier for the student (you can modify this based on your needs)
    printf("Enter student number (1-5): ");
    scanf("%d", &studentNumber);

    while (1) {
        printf("\nPress Enter to enter the classroom: ");
        getchar(); // Wait for Enter key press

        // Check if a chair is available
        int chairsAvailable;
        sem_getvalue(chair_sem, &chairsAvailable);
        if (chairsAvailable > 0) {
            enterClassroom(chair_sem, studentNumber);
        } else {
            printf("No chairs available. Waiting for a chair to become free...\n");
            while (chairsAvailable <= 0) {
                sem_getvalue(chair_sem, &chairsAvailable);
            }
            enterClassroom(chair_sem, studentNumber);
        }

        printf("\nPress Enter to leave the classroom: ");
        getchar(); // Wait for Enter key press
        leaveClassroom(chair_sem, studentNumber);
    }

    // Close and unlink the semaphore
    sem_close(chair_sem);
    sem_unlink(SEM_NAME);

    return 0;
}