// SSOO-P3 23/24
#ifndef HEADER_FILE
#define HEADER_FILE

#include <pthread.h>

struct element {
    int product_id; // product identifier
    int op;         // operation: 0 for purchase, 1 for sale
    int units;      // product units
};

typedef struct queue {
    struct element *elements;   // array of elements
    int capacity;               // maximum number of items in the queue
    int front;                  // index of the front item
    int rear;                   // index of the rear item
    int count;                  // Number of items in the queue

    // mutex synchronization tools
    pthread_mutex_t lock;
    pthread_cond_t can_produce;
    pthread_cond_t can_consume;
} queue;

queue* queue_init(int size);
int queue_destroy(queue *q);
int queue_put(queue *q, struct element* elem);
struct element* queue_get(queue *q);
int queue_empty(queue *q);
int queue_full(queue *q);

#endif
