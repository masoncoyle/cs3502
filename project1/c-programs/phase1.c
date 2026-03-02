//	Liam Mason Coyle
//	CS 3502 Section W03
//	Project 1

//  AI Assistance: Claude Sonnet 4.6 (Anthropic), used to debug code and for general reference
//  Date accessed: February - March 2026
//  URL: https://claude.ai

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS 4
#define TRANSACTIONS_PER_THREAD 10
#define INITIAL_BALANCE 1000.0

typedef struct {
	int account_id;
	double balance;
	int transaction_count;
} Account;

Account accounts[NUM_ACCOUNTS];

// void deposit_unsafe(int account_id, double amount) {
// 		double current_balance = accounts[account_id].balance;
// 		usleep(1);
// 		double new_balance = current_balance + amount;
// 		accounts[account_id].balance = new_balance;
// 		accounts[account_id].transaction_count++;
// }

// void withdrawal_unsafe(int account_id, double amount) {
//     double current_balance = accounts[account_id].balance;
//     usleep(1); // Simulate processing time
//     double new_balance = current_balance - amount;
//     accounts[account_id].balance = new_balance;
//     accounts[account_id].transaction_count++;
// }

// Method to transfer money from one account to another
void transfer_unsafe(int teller_id, int source_id, int destination_id, double amount){
	double source_balance = accounts[source_id].balance; // Read balance
	source_balance -= amount; // Remove amount from source balance
	usleep(1); // Simulate proccessing time
	accounts[source_id].balance = source_balance; // Set new source balance
	accounts[source_id].transaction_count++; // Increment transaction count

	double destination_balance = accounts[destination_id].balance; // Read balance
	destination_balance += amount; // Add amount to destination balance
	usleep(1); // Simulate proccessing time
	accounts[destination_id].balance = destination_balance; // Set new destination balance
	accounts[destination_id].transaction_count++; // Increment transaction count

	printf("\033[1mThread %d: Transferred $%.2f from Account %d to Account %d\033[0m\n", teller_id, amount, source_id, destination_id);
}

void* teller_thread(void* arg) {
	int teller_id = *(int*)arg; // Cast *arg to an *int pointer
	// Initialize unique random seed using XOR on time and thread id
	unsigned int seed = time(NULL) ^ pthread_self();

	// Thread performs specified amount of transfers on random accounts
	for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
		int source_id = rand_r(&seed) % NUM_ACCOUNTS; // Set a random source_id to a number between 0 and (NUM_ACOUNTS -1)
		int destination_id = rand_r(&seed) % NUM_ACCOUNTS; // Set a random destination_id to a number between 0 and (NUM_ACOUNTS -1)
		while (source_id == destination_id){ // Repeat until destination_id is different from source_id
			destination_id = rand_r(&seed) % NUM_ACCOUNTS;
		}
		// Generate random amount to transfer, between 1 and 100 using rand_r(&seed), seed is changed after this operation
		double amount = (rand_r(&seed) % 100) + 1;

		// Call transfer fuction
		transfer_unsafe(teller_id, source_id, destination_id, amount);

		// if (operation != 0) {
		// deposit_unsafe(account_id, amount);
		// printf("Teller %d: Deposited $%.2f to Account %d\n",
		// 	teller_id, amount, account_id);
		// } else {
        // withdrawal_unsafe(account_id, amount); // Call withdrawal_unsafe
        // printf("Teller %d: Withdrew $%.2f from Account %d\n", // Print withdrawal statement
        //     teller_id, amount, account_id);
		// }
	}
	return NULL;
}

int main() {
	printf("=== Phase 1: Race Conditions Demo ===\n\n");

    for (int i = 0; i < NUM_ACCOUNTS; i++) { // Iterate through accounts array
		// Initialze accounts
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
	}

	// Display initial state (GIVEN)
	printf("Initial State:\n");
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf(" Account %d: $%.2f\n", i, accounts[i].balance);
	}

	// Find the expected total by multiplying the number of accounts by their initial balance
	double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE;
	printf("\nExpected total: $%.2f\n\n", expected_total);

	// Initialize thread array and thread id array
	pthread_t threads[NUM_THREADS]; 
	int thread_ids[NUM_THREADS]; // GIVEN: Separate array for IDs

	int result; // Variable used to store pthread_create and pthread_join output, to check if the methods were successful

	// Create threads
	for (int i = 0; i < NUM_THREADS; i++) { // Iterate through thread id numbers
		thread_ids[i] = i; // GIVEN: Store ID persistently
		result = pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]); // Initialize each thread
		if (result != 0){ // Exit program if there is an error in creating a thread
			printf("\nError when creating thread.\n");
			exit(1);
		}
	}

	// Start performance timer right after creating threads
	struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

	// Join threads
	for (int i = 0; i < NUM_THREADS; i++) { // Iterate through thread id numbers
		result = pthread_join(threads[i], NULL); // Join each thread
		if (result != 0){ // Exit program if there is an error in joining a thread
			printf("\nError when joining thread.\n");
			exit(1);
		}
	}

	// Record execution time for program after joining all threads
	clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

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
		printf("NO RACE CONDITION DETECTED\n");
	}
	printf("Run program again to test for different results.\n");

	return 0;
}
