// Liam Mason Coyle
// CS 3502 Section W03
// Project 1

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define NUM_ACCOUNTS 4
#define NUM_THREADS 4
#define TRANSACTIONS_PER_THREAD 10
#define INITIAL_BALANCE 1000.0

// Updated Account structure with mutex (GIVEN)
typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock; // NEW: Mutex for this account
} Account;

Account accounts[NUM_ACCOUNTS];

// GIVEN: Example of mutex initialization
void initialize_accounts() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;

        // Initialize the mutex
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}

void deposit(int account_id, double amount){
	accounts[account_id].balance += amount;
	accounts[account_id].transaction_count++;
}
void withdraw(int account_id, double amount){
	accounts[account_id].balance -= amount;
	accounts[account_id].transaction_count++;
}
void transfer_safe(int source_id, int destination_id, double amount){
	pthread_mutex_lock(&accounts[source_id].lock);
	pthread_mutex_lock(&accounts[destination_id].lock);
	withdraw(source_id, amount);
	deposit(destination_id, amount);
	pthread_mutex_unlock(&accounts[source_id].lock);
	pthread_mutex_unlock(&accounts[destination_id].lock);
}


void* teller_thread(void* arg) {
	int teller_id = *(int*)arg;

	unsigned int seed = time(NULL) ^ pthread_self();

	for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {

		int source_id = rand_r(&seed) % NUM_ACCOUNTS;
		int destination_id = rand_r(&seed) % NUM_ACCOUNTS;
		while (source_id == destination_id){
			destination_id = rand_r(&seed) % NUM_ACCOUNTS;
		}

		double amount = (rand_r(&seed) % 100) + 1;

		transfer_safe(source_id, destination_id, amount);
		printf("Teller %d: Transfered $%.2f from Account %d to Account %d\n", teller_id, amount, source_id, destination_id);
	}

	return NULL;
}

// TODO 3: Add performance timing
// Reference: Section 7.2 "Performance Measurement"
// Hint: Use clock_gettime(CLOCK_MONOTONIC, &start);

void cleanup_mutexes() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
}


int main(){
    printf("=== Phase 1: Race Conditions Demo ===\n\n");

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
	}

	printf("Initial State:\n");
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf(" Account %d: $%.2f\n", i, accounts[i].balance);
	}

	double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE; 

	printf("\nExpected total: $%.2f\n\n", expected_total);

	pthread_t threads[NUM_THREADS];
	int thread_ids[NUM_THREADS];

	int result;

	for (int i = 0; i < NUM_THREADS; i++) {
		thread_ids[i] = i;
		result = pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
		if (result == 1){
			printf("Error when creating thread.");
			exit(1);
		}
	
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		result = pthread_join(threads[i], NULL);
		if (result == 1){
			printf("Error when joining thread.");
			exit(1);
		}
	}
    cleanup_mutexes();

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    

	printf("\n=== Final Results ===\n");
	printf("\nTime: %.4f seconds\n\n", elapsed);

	double actual_total = 0.0;

	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf("Account %d: $%.2f (%d transactions)\n",
			i, accounts[i].balance, accounts[i].transaction_count);
		actual_total += accounts[i].balance;
	}

	printf("\nExpected total: $%.2f\n", expected_total);
	printf("Actual total: $%.2f\n", actual_total);
	printf("Difference: $%.2f\n", actual_total - expected_total);

	if (expected_total != actual_total){
		printf("RACE CONDITION DETECTED!\n");
	} else{
		printf("NO RACE CONDITION DETECTED");
	}
	printf("Run program again to test for different results.\n");

	return 0;
}

// TODO 5: Compare Phase 1 vs Phase 2 performance
// Measure execution time for both versions
// Document the overhead of synchronization