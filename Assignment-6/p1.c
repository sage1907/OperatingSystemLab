#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define NO_SEM 1

#define P(s) semop(s, &Pop, 1);
#define V(s) semop(s, &Vop, 1);

struct sembuf Pop;
struct sembuf Vop;

int main() {
    key_t mykey;
    int semid;
    int status;

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
        struct seminfo *__buf;
    } setvalArg;

    setvalArg.val = 1;

    Pop.sem_num = 0;
    Pop.sem_op = -1;
    Pop.sem_flg = SEM_UNDO;

    Vop.sem_num = 0;
    Vop.sem_op = 1;
    Vop.sem_flg = SEM_UNDO;

    mykey = ftok("/home/sagar/osLab/ass6/shared_key/", 1);

    if (mykey == -1) {
        perror("ftok() failed");
        exit(1);
    }
    // printf("mykey: %d\n", mykey);

    semid = semget(mykey, NO_SEM, IPC_CREAT | 0777);
    if (semid == -1) {
        perror("semget() failed");
        exit(1);
    }

    status = semctl(semid, 0, SETVAL, setvalArg);
    if (status == -1) {
        perror("semctl() failed");
        exit(1);
    }

    while (1) {
        printf("\nPress enter to execute P: ");
        getchar();
        P(semid);
        printf("I am p1!.\n");
        printf("Press enter to execute V: ");
        getchar();
        V(semid);
    }

    return 0;
}