#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_SIZE 10

int queue[MAX_SIZE];
int front = -1, rear = -1;
int itemCount = 0;

pthread_mutex_t mutex;
pthread_cond_t full, empty;

void enQueue(int value) {
    pthread_mutex_lock(&mutex);
    while (itemCount == MAX_SIZE) {
        printf("Queue is full. Producer waiting...\n");
        pthread_cond_wait(&full, &mutex);
    }

    if (front == -1) front = 0;
    rear = (rear + 1) % MAX_SIZE;
    queue[rear] = value;
    printf("Produced item: %d\n", value);
    itemCount++;

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&empty);
}

int deQueue() {
    int item;
    pthread_mutex_lock(&mutex);
    while (itemCount == 0) {
        printf("Queue is empty. Consumer waiting...\n");
        pthread_cond_wait(&empty, &mutex);
    }

    item = queue[front];
    if (front == rear) front = rear = -1;
    else front = (front + 1) % MAX_SIZE;
    printf("Consumed item: %d\n", item);
    itemCount--;

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&full);
    return item;
}

void *producer(void *data) {
    while (1) {
        enQueue(rand() % 100);//Producing random values and adding them in queue one at a time
        sleep(1);
    }
    return NULL;
}

void *consumer(void *data) {
    while (1) {
        deQueue(); // consume values one by one
        sleep(2);
    }
    return NULL;
}

int main() {
    int m, n;
    printf("Enter the number of producer threads: ");
    scanf("%d", &m);
    printf("Enter the number of consumer threads: ");
    scanf("%d", &n);

    pthread_t producerThreads[m], consumerThreads[n];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&full, NULL);
    pthread_cond_init(&empty, NULL);

    for (int i = 0; i < m; i++) {
        pthread_create(&producerThreads[i], NULL, producer, NULL);
    }
    for (int i = 0; i < n; i++) {
        pthread_create(&consumerThreads[i], NULL, consumer, NULL);
    }

    for (int i = 0; i < m; i++) {
        pthread_join(producerThreads[i], NULL);
    }
    for (int i = 0; i < n; i++) {
        pthread_join(consumerThreads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&full);
    pthread_cond_destroy(&empty);

    return 0;
}