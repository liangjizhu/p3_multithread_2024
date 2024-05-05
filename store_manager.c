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

typedef struct {
    int product_id;
    char operation; 
    int units;
} Transaction;


typedef struct {
    int purchase_price;
    int selling_price;
} ProductPricing;


// Global array to store pricing
ProductPricing* product_prices;  // Dinamic array for prices

void* consumer(void *arg);
void* producer(void *arg);

int main (int argc, const char * argv[])
{
  const char* file_name = argv[1]; // To open the file
  if (argc != 5) {
  fprintf(stderr, "Usage: %s <file name> <num producers> <num consumers> <buff size>\n", argv[0]);
  return EXIT_FAILURE;
  }

  int num_producers = atoi(argv[2]);
  int num_consumers = atoi(argv[3]);
  int buff_size = atoi(argv[4]);
  if (num_producers <= 0 || num_consumers <= 0 || buff_size <= 0) {
      fprintf(stderr, "Invalid number of producers, consumers, or buffer size\n");
      return EXIT_FAILURE;
  }


  FILE* file = fopen(file_name, "r");
  if (!file) {
      perror("Failed to open file");
  return EXIT_FAILURE;
  }

  int num_products;
  fscanf(file, "%d", &num_products);  // Read the number of products

  ProductPricing* product_prices = malloc(num_products * sizeof(ProductPricing));
  if (!product_prices) {
      fclose(file);
      perror("Failed to allocate memory for product prices");
      return EXIT_FAILURE;
  }

  for (int i = 0; i < num_products; i++) {
      int product_id;  // No necesariamente se usa, pero podría ser útil
      fscanf(file, "%d %d %d", &product_id, &product_prices[product_id - 1].purchase_price, &product_prices[product_id - 1].selling_price);
  }

  int num_operations;
  fscanf(file, "%d", &num_operations);  // Read the number of transactions

  Transaction* transactions = malloc(num_operations * sizeof(Transaction));
  if (!transactions) {
      fclose(file);
      free(product_prices);
      perror("Failed to allocate memory for transactions");
      return EXIT_FAILURE;
  }

  for (int i = 0; i < num_operations; i++) {
      fscanf(file, "%d %c %d", 
          &transactions[i].product_id, 
          &transactions[i].operation, 
          &transactions[i].units);
  }

  fclose(file);

  queue *q = queue_init(buff_size);
  if (!q) {
      free(transactions);
      free(product_prices);
      fprintf(stderr, "Failed to initialize the queue\n");
      return EXIT_FAILURE;
  }

  int profits = 0;
  int product_stock [5] = {0};
 
  // Create threads
  pthread_t producer_threads[num_producers];
  pthread_t consumer_threads[num_consumers];

  for (int i = 0; i < num_producers; i++) {
      pthread_create(&producer_threads[i], NULL, producer, q);
  }

  // Create consumer threads
  for (int i = 0; i < num_consumers; i++) {
      pthread_create(&consumer_threads[i], NULL, consumer, q);
  }

  //join the threads
  // Unir threads de productores
  for (int i = 0; i < num_producers; i++) {
      pthread_join(producer_threads[i], NULL);
  }

  // Unir threads de consumidores
  for (int i = 0; i < num_consumers; i++) {
      pthread_join(consumer_threads[i], NULL);
  }



  // Output
  printf("Total: %d euros\n", profits);
  printf("Stock:\n");
  printf("  Product 1: %d\n", product_stock[0]);
  printf("  Product 2: %d\n", product_stock[1]);
  printf("  Product 3: %d\n", product_stock[2]);
  printf("  Product 4: %d\n", product_stock[3]);
  printf("  Product 5: %d\n", product_stock[4]);

  queue_destroy(q); // Cleanup

  return 0;
}

void* consumer(void *arg) {
    queue *q = (queue *)arg;
    for (int i = 0; i < 100; i++) {
        struct element *e = queue_get(q);
        printf("Consumed: Product %d, Operation %d, Units %d\n", e->product_id, e->op, e->units);
    }
    pthread_exit(NULL);
}

void* producer(void *arg) {
    queue *q = (queue *)arg;
    for (int i = 0; i < 100; i++) { // Produce 100 items
        struct element e = {i % 5, 1, 10}; // Example item
        queue_put(q, &e);
    }
    pthread_exit(NULL);
}


