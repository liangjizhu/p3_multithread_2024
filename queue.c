// SSOO-P3 23/24
#include "queue.h"
#include <stdlib.h>
#include <pthread.h>

// initializes a new queue with a specified size and sets up synchronization primitives
queue* queue_init(int size) {
    // allocate memory for the queue structure
    queue *q = (queue *)malloc(sizeof(queue));
    if (!q) return NULL; // return NULL if memory allocation fails

    // allocate memory for the array of elements
    q->elements = malloc(sizeof(struct element) * size);
    if (!q->elements) {
        free(q); // clean up previously allocated queue structure if this allocation fails
        return NULL;
    }

    // initialize queue properties
    q->capacity = size;
    q->front = 0;    // start position for dequeueing
    q->rear = -1;    // Start position for enqueueing
    q->count = 0;    // Number of elements currently in the queue

    // initialize synchronization primitives
    pthread_mutex_init(&q->lock, NULL);             // mutex for protecting the queue structure
    pthread_cond_init(&q->can_produce, NULL);       // condition variable for producing
    pthread_cond_init(&q->can_consume, NULL);       // condition variable for consuming

    return q; // return the pointer to the newly created queue
}

// adds an element to the queue if there is space available
int queue_put(queue *q, struct element* x) {
    pthread_mutex_lock(&q->lock); // lock the queue before modifying it

    // wait until there is space available in the queue
    while (queue_full(q)) {
        pthread_cond_wait(&q->can_produce, &q->lock); // block if the queue is full
    }

    // add the new element to the queue
    q->rear = (q->rear + 1) % q->capacity; // circular buffer increment
    q->elements[q->rear] = *x; // copy the element into the queue
    q->count++; // increment the count of elements in the queue

    // signal to consumers that there is a new item available
    pthread_cond_signal(&q->can_consume);
    pthread_mutex_unlock(&q->lock); // unlock the queue after modification

    return 1;
}

// removes and returns the front element from the queue
struct element* queue_get(queue *q) {
    pthread_mutex_lock(&q->lock); // lock the queue before modifying it

    // wait until there is an item to consume
    while (queue_empty(q)) {
        pthread_cond_wait(&q->can_consume, &q->lock); // block if the queue is empty
    }

    // allocate memory for returning the element
    struct element *item = malloc(sizeof(struct element));
    if (item == NULL) { // check for memory allocation failure
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    // retrieve the front item from the queue
    *item = q->elements[q->front]; // copy the element
    q->front = (q->front + 1) % q->capacity; // circular buffer increment
    q->count--; // decrement the count of elements

    // signal to producers that there is space available
    pthread_cond_signal(&q->can_produce);
    pthread_mutex_unlock(&q->lock); // unlock the queue after modification

    return item;
}

// checks if the queue is empty
int queue_empty(queue *q) {
    return q->count == 0; // return true if the count is zero
}

// checks if the queue is full
int queue_full(queue *q) {
    return q->count == q->capacity; // return true if the count equals the capacity
}

// destroys the queue and frees up the resources
int queue_destroy(queue *q) {
    pthread_mutex_destroy(&q->lock);             // destroy the mutex
    pthread_cond_destroy(&q->can_produce);       // destroy the can_produce condition variable
    pthread_cond_destroy(&q->can_consume);       // destroy the can_consume condition variable
    free(q->elements); // free the memory for the elements
    free(q); // free the memory for the queue structure
    return 1; 
}
