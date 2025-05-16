# Multithreaded Store Manager

## Project Description

This project is a multithreaded store manager program written in C. The program reads transactions from a file and processes them using producer and consumer threads. It calculates profits and updates product stock based on transactions. The program uses POSIX threads for concurrent processing and handles synchronization using mutexes and condition variables.

## Compilation and Execution

To compile the program, use the provided `Makefile`. Simply run the following command in the terminal:

```sh
make
```

This will generate an executable file named `store_manager`.

To run the program, use the following command:

```sh
./store_manager <file name> <num producers> <num consumers> <buff size>
```

Replace `<file name>`, `<num producers>`, `<num consumers>`, and `<buff size>` with the appropriate values.

## Repository Structure

- `store_manager.c`: Contains the main logic of the store manager program.
- `queue.c`: Contains the implementation of the queue used for transaction processing.
- `queue.h`: Contains the definitions and declarations for the queue.
- `Makefile`: Used to compile the program.
- `checker_os_p3.sh`: Script for testing the program.
- `authors.txt`: List of authors of the project.

## Authors

- 100495755, Tello Marcos, Alba
- 100495941, Rodríguez García, Carmen
- 100495723, Zhu, LiangJi

## Running the Test Script

To run the test script `checker_os_p3.sh`, use the following command:

```sh
./checker_os_p3.sh <zip file>
```

Replace `<zip file>` with the name of the zip file containing the necessary files for testing.
