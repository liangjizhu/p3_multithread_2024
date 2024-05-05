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

#define END_OF_PRODUCTION -1

pthread_mutex_t file_lock, results_lock;
FILE *fp = NULL;
int current_operation = 0, total_operations = 0;
int purchase_cost[6], selling_price[6];  // Extend arrays if more products are expected
queue* transaction_queue = NULL;

struct thread_data {
    int* profits;
    int* product_stock;
};


void initialize_product_pricing();
int calculate_profit(int product_id, int units, int operation);
void cleanup_transaction_system();
void initialize_transaction_system(const char *filename);
void process_transaction(struct element *trans, struct thread_data *data);
void* consumer(void *arg);
void* producer(void *arg);

struct element* read_next_transaction() {
  pthread_mutex_lock(&file_lock);

  if (current_operation >= total_operations) {
      pthread_mutex_unlock(&file_lock);
      return NULL;
  }

  struct element* new_elem = malloc(sizeof(struct element));
  if (new_elem == NULL) {
      pthread_mutex_unlock(&file_lock);
      return NULL;
  }

  char operation[10]; // Temporary string to hold the operation
  if (fscanf(fp, "%d %s %d", &new_elem->product_id, operation, &new_elem->units) != 3) {
      free(new_elem);
      pthread_mutex_unlock(&file_lock);
      return NULL;
  }

  new_elem->op = (strcmp("PURCHASE", operation) == 0) ? 0 : 1;
  current_operation++;

  pthread_mutex_unlock(&file_lock);
  return new_elem;
}


int main (int argc, const char * argv[])
{
  if (argc != 5) {
    fprintf(stderr, "Usage: %s <file name> <num producers> <num consumers> <buff size>\n", argv[0]);
    return EXIT_FAILURE;
  }

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


  transaction_queue = queue_init(buff_size);
  if (transaction_queue == NULL) {
    fprintf(stderr, "Failed to initialize the queue\n");
    return 1;
  }
  
  pthread_mutex_init(&results_lock, NULL);
  pthread_t producers[num_producers], consumers[num_consumers];
  struct thread_data data = {&profits, product_stock};

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
  return 0;
}

void* producer(void *arg) {
    struct element *transaction;
    while ((transaction = read_next_transaction()) != NULL) {
        queue_put(transaction_queue, transaction);
        free(transaction);
    }
    // Signal end of production by pushing a special 'end' transaction
    transaction = malloc(sizeof(struct element));
    if (!transaction) return NULL;  // Handle malloc failure
    transaction->product_id = END_OF_PRODUCTION;
    queue_put(transaction_queue, transaction);
    return NULL;
}

void* consumer(void *arg) {
    struct thread_data *data = (struct thread_data*) arg;
    struct element *transaction;

    while (1) {
        transaction = queue_get(transaction_queue);
        if (transaction->product_id == END_OF_PRODUCTION) {
            free(transaction);  // Don't forget to free the sentinel transaction
            break;
        }
        process_transaction(transaction, data);
        free(transaction);
    }
    return NULL;
}

void process_transaction(struct element *trans, struct thread_data *data) {
    pthread_mutex_lock(&results_lock);
    int idx = trans->product_id - 1;  // Adjust index for zero-based array access
    if (trans->op == 0) {  // Purchase
        if (idx >= 0 && idx < 5) {
            data->product_stock[idx] += trans->units;
            int profit = calculate_profit(trans->product_id, trans->units, 0);
            *(data->profits) += profit;
        }
    } else {  // Sale
        if (idx >= 0 && idx < 5 && data->product_stock[idx] >= trans->units) {
            data->product_stock[idx] -= trans->units;
            int profit = calculate_profit(trans->product_id, trans->units, 1);
            *(data->profits) += profit;
        }
    }
    pthread_mutex_unlock(&results_lock);
}


/*
void process_transaction(struct element *trans, struct thread_data *data) {
    pthread_mutex_lock(&results_lock);
    int idx = trans->product_id - 1;  // Adjust index for zero-based array access

    if (trans->op == 0) {  // Purchase
        if (idx >= 0 && idx < 5) {
            data->product_stock[idx] += trans->units;
        }
    } else {  // Sale
        if (idx >= 0 && idx < 5 && data->product_stock[idx] >= trans->units) {
            data->product_stock[idx] -= trans->units;
            int profit = calculate_profit(trans->product_id, trans->units);
            *(data->profits) += profit;  // Update the profits using the pointer
        }
    }
    pthread_mutex_unlock(&results_lock);
}*/




void initialize_product_pricing() {
    // Initialize costs and prices for each product based on the provided table
    purchase_cost[1] = 2;  selling_price[1] = 3;   // Product 1
    purchase_cost[2] = 5;  selling_price[2] = 10;  // Product 2
    purchase_cost[3] = 15; selling_price[3] = 20;  // Product 3
    purchase_cost[4] = 25; selling_price[4] = 40;  // Product 4
    purchase_cost[5] = 100; selling_price[5] = 125;// Product 5

    // Products not defined in the table should have 0 as default values
    for (int i = 0; i < sizeof(purchase_cost) / sizeof(purchase_cost[0]); i++) {
        if (i > 5) {
            purchase_cost[i] = 0;
            selling_price[i] = 0;
        }
    }
}

int calculate_profit(int product_id, int units, int operation) {
    if (units <= 0) return 0;
    int profit_per_unit;
    if (operation == 0) {  // Purchase (subtract costs)
        profit_per_unit = -purchase_cost[product_id - 1];
    } else {  // Sale (add revenues)
        profit_per_unit = selling_price[product_id - 1];
    }
    return profit_per_unit * units;
}
/*
int calculate_profit(int product_id, int units) {
    if (units <= 0) return 0;  // No profit from zero or negative units
    return (selling_price[product_id] - purchase_cost[product_id]) * units;
}*/

void initialize_transaction_system(const char *filename) {
    pthread_mutex_init(&file_lock, NULL);

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Failed to open file");
        exit(1);
    }

    if (fscanf(fp, "%d", &total_operations) != 1) {
        perror("Failed to read the number of operations");
        fclose(fp);
        exit(1);
    }
}

void cleanup_transaction_system() {
    fclose(fp);
    pthread_mutex_destroy(&file_lock);
}