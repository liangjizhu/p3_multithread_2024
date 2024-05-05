#include "queue.h"
#include <stdlib.h>
#include <pthread.h>

// To create a queue
queue* queue_init(int size) {
    queue *q = (queue *)malloc(sizeof(queue));
    if (!q) return NULL;

    q->elements = malloc(sizeof(struct element) * size);
    if (!q->elements) {
        free(q);
        return NULL;
    }

    q->capacity = size;
    q->front = 0;
    q->rear = -1;
    q->count = 0;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->can_produce, NULL);
    pthread_cond_init(&q->can_consume, NULL);

    return q;
}

// To enqueue an element
int queue_put(queue *q, struct element* x)
{
    pthread_mutex_lock(&q->lock);

    // Wait until there is space in the queue
    while (queue_full(q)) {
        pthread_cond_wait(&q->can_produce, &q->lock);
    }

    q->rear = (q->rear + 1) % q->capacity;
    q->elements[q->rear] = *x;  // Use x instead of elem
    q->count++;

    pthread_cond_signal(&q->can_consume);
    pthread_mutex_unlock(&q->lock);

    return 1;
}


// To dequeue an element
struct element* queue_get(queue *q) {
    pthread_mutex_lock(&q->lock);

    // Wait until the queue is not empty
    while (queue_empty(q)) {
        pthread_cond_wait(&q->can_consume, &q->lock);
    }

    struct element *item = malloc(sizeof(struct element));  // Allocate memory for the element
    if (item == NULL) {  // Always good to check if malloc succeeded
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    *item = q->elements[q->front];  // Copy the data
    q->front = (q->front + 1) % q->capacity;
    q->count--;

    pthread_cond_signal(&q->can_produce);
    pthread_mutex_unlock(&q->lock);
    return item;
}


// To check if queue is empty
int queue_empty(queue *q) {
    return q->count == 0;
}

// To check if queue is full
int queue_full(queue *q) {
    return q->count == q->capacity;
}

// To destroy the queue and free the resources
int queue_destroy(queue *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->can_produce);
    pthread_cond_destroy(&q->can_consume);
    free(q->elements);
    free(q);
    return 1;
}
