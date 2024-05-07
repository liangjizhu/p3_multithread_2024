//SSOO-P3 23/24
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Constant to define the end of production
#define END_OF_PRODUCTION -1

// Data structure to hold shared data between threads
struct thread_data {
    int* profits; // pointer to the total profits
    int* product_stock; // pointer to the product stock array
    int active_producers; // counter for the number of active producers
    pthread_mutex_t producer_count_lock; // mutex to synchronize changes to active_producers
};


// Mutexes for file and result synchronization
pthread_mutex_t file_lock, results_lock;
FILE *fp = NULL;        // file pointer to the input file
int current_operation = 0, total_operations = 0; // operation counters
int purchase_cost[5], selling_price[5]; // arrays for cost and selling price of products
queue* transaction_queue = NULL; // pointer to the transaction queue


// Declarations of functions
void initialize_product_pricing();
int calculate_profit(int product_id, int units, int operation);
void cleanup_transaction_system();
void initialize_transaction_system(const char *filename);
void process_transaction(struct element *trans, struct thread_data *data);
void* consumer(void *arg);
void* producer(void *arg);

// Function to read the next transaction from the file
struct element* read_next_transaction() {
    pthread_mutex_lock(&file_lock); // lock access to the file

    if (current_operation >= total_operations) {
        pthread_mutex_unlock(&file_lock); // unlock and return if all operations are processed
        return NULL;
    }

    struct element* new_elem = malloc(sizeof(struct element)); // allocate memory for a new element
    if (new_elem == NULL) {
        pthread_mutex_unlock(&file_lock);
        return NULL;
    }

    char operation[10]; // temporary string to hold the operation
    if (fscanf(fp, "%d %s %d", &new_elem->product_id, operation, &new_elem->units) != 3) {
        free(new_elem);
        pthread_mutex_unlock(&file_lock);
        return NULL;
    }

    new_elem->op = (strcmp("PURCHASE", operation) == 0) ? 0 : 1; // determine operation type
    current_operation++; // increment the counter of the operations done

    pthread_mutex_unlock(&file_lock); // unlock the file mutex
    return new_elem; // return the new transaction element
}


int main (int argc, const char * argv[])
{
    // Check for the correct number of arguments
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <file name> <num producers> <num consumers> <buff size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // initialize important variables
    const char *filename = argv[1];
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int buff_size = atoi(argv[4]);
    int profits = 0;
    int product_stock[5] = {0};
    

    if (num_producers <= 0 || num_consumers <= 0 || buff_size <= 0) {
        fprintf(stderr, "Invalid number of producers, consumers, or buffer size\n");
        return EXIT_FAILURE;
    }

    // Initialize global variables and pricing
    initialize_product_pricing();
    initialize_transaction_system(filename);


    transaction_queue = queue_init(buff_size); // initialization of the transaction queue
    if (transaction_queue == NULL) {
        fprintf(stderr, "Failed to initialize the queue\n");
        return 1;
    }
    
    pthread_mutex_init(&results_lock, NULL); // initialize mutex for results synchronization
    pthread_t producers[num_producers], consumers[num_consumers]; // thread arrays
    struct thread_data data; // data structure
    data.profits = &profits;
    data.product_stock = product_stock;
    data.active_producers = num_producers; // Initializing active producers count
    pthread_mutex_init(&data.producer_count_lock, NULL); // Initialize mutex for producer count

    // Create producer and consumer threads
    for (int i = 0; i < num_producers; i++) {
        pthread_create(&producers[i], NULL, producer, &data);
    }
    for (int i = 0; i < num_consumers; i++) {
        pthread_create(&consumers[i], NULL, consumer, &data);
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }


    // Output
    printf("Total: %d euros\n", profits);
    printf("Stock:\n");
    printf("  Product 1: %d\n", product_stock[0]);
    printf("  Product 2: %d\n", product_stock[1]);
    printf("  Product 3: %d\n", product_stock[2]);
    printf("  Product 4: %d\n", product_stock[3]);
    printf("  Product 5: %d\n", product_stock[4]);

    cleanup_transaction_system();
    queue_destroy(transaction_queue);
    pthread_mutex_destroy(&results_lock);
    pthread_mutex_destroy(&data.producer_count_lock);
    return 0;
}

// Producer thread function
void* producer(void *arg) {
    struct thread_data *data = (struct thread_data*) arg;
    struct element *transaction;

    // read transactions from file and enqueue them until there are no more
    while ((transaction = read_next_transaction()) != NULL) {
        queue_put(transaction_queue, transaction); // enqueue the transaction for consumers to process
        free(transaction); // free the transaction after enqueuing
    }

    // mutex lock to decrement the count of active producers
    pthread_mutex_lock(&data->producer_count_lock);
    data->active_producers--;

    // if this is the last producer, enqueue a special marker to indicate no more productions
    if (data->active_producers == 0) { // last producer
        transaction = malloc(sizeof(struct element));
        if (transaction) {
            transaction->product_id = END_OF_PRODUCTION; // set special marker
            queue_put(transaction_queue, transaction); // enqueue the 'END_OF_PRODUCTION' special marker
        }
    }
    // unlock after modifying producer count
    pthread_mutex_unlock(&data->producer_count_lock);
    return NULL;
}

// Consumer thread function
void* consumer(void *arg) {
    while (1) {
        struct element *transaction = queue_get(transaction_queue); // dequeue a transaction
        if (transaction == NULL) {
            break;  // break if queue_get returns NULL, indicating an empty queue and producers are done
        }
        if (transaction->product_id == END_OF_PRODUCTION) {
            // reinsert the 'END_OF_PRODUCTION' element for other consumers
            queue_put(transaction_queue, transaction);
            break;  // break after reinserting to ensure all consumers see the end signal
        }
        process_transaction(transaction, arg); // process transaction
        free(transaction); // free the transaction
    }
    return NULL;
}

// processes a single transaction, updating stock and calculating profit
void process_transaction(struct element *trans, struct thread_data *data) {
    pthread_mutex_lock(&results_lock); // lock mutex
    int idx = trans->product_id - 1; 
    if (trans->op == 0) {  // Purchase
        if (idx >= 0 && idx < 5) {
            data->product_stock[idx] += trans->units; // increase stock
            int profit = calculate_profit(idx, trans->units, 0); // calculate profit
            *(data->profits) += profit; // update profit
        }
    } else {  // Sale
        if (idx >= 0 && idx < 5) {
            data->product_stock[idx] -= trans->units; // decrease stock
            int profit = calculate_profit(idx, trans->units, 1); // calculate profit
            *(data->profits) += profit; //update profit
        }
    }
    pthread_mutex_unlock(&results_lock); //unlock mutex
}


// initializes the pricing for each product
void initialize_product_pricing() {
    // Initialize costs and prices for each product based on the provided table
    purchase_cost[0] = 2;  selling_price[0] = 3;   // product 1
    purchase_cost[1] = 5;  selling_price[1] = 10;  // product 2
    purchase_cost[2] = 15; selling_price[2] = 20;  // product 3
    purchase_cost[3] = 25; selling_price[3] = 40;  // product 4
    purchase_cost[4] = 100; selling_price[4] = 125; // product 5

}

// calculates profit based on the operation type, units, and product pricing
int calculate_profit(int product_id, int units, int operation) {
    if (units <= 0) return 0; // no transaction occurs if there are no units
    int profit_per_unit;
    if (operation == 0) {  // Purchase (subtract)
        profit_per_unit = -purchase_cost[product_id];
    } else {  // Sale (add)
        profit_per_unit = selling_price[product_id];
    }
    return profit_per_unit * units; // Total profit
}

// Initializes the transaction system by opening the file and reading the total number of operations
void initialize_transaction_system(const char *filename) {
    pthread_mutex_init(&file_lock, NULL); // initialize the mutex for file access

    fp = fopen(filename, "r"); // open the file
    if (fp == NULL) {
        perror("Failed to open file");
        exit(1);
    }

    if (fscanf(fp, "%d", &total_operations) != 1) { // read the number of operations
        perror("Failed to read the number of operations");
        fclose(fp);
        exit(1);
    }
}

// cleans up the transaction system by closing the file and destroying the file access mutex
void cleanup_transaction_system() {
    fclose(fp);
    pthread_mutex_destroy(&file_lock);
}