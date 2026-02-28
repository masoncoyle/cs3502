// Liam Mason Coyle
// CS 3502 Section W03
// Project 1

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

void deposit_unsafe(int account_id, double amount) {
	double current_balance = accounts[account_id].balance;
	usleep(1);
	double new_balance = current_balance + amount;
	accounts[account_id].balance = new_balance;
	accounts[account_id].transaction_count++;
}

void withdrawal_unsafe(int account_id, double amount) {
    double current_balance = accounts[account_id].balance; // Set current balance as the balance of the account specified
    usleep(1); // Simulate processing time
    double new_balance = current_balance - amount; // Subtract the amount from current balance to get the new_balance
    accounts[account_id].balance = new_balance; // Set the account's balance value to the value of new_balance
    accounts[account_id].transaction_count++; // Increment the transaction count of the account
}

void transfer_unsafe(int source_id, int destination_id, double amount) {
	withdrawal_unsafe(source_id, amount);
	deposit_unsafe(destination_id, amount);
}

void* teller_thread(void* arg) {
	int teller_id = *(int*)arg;

	unsigned int seed = time(NULL) ^ pthread_self(); // Initialize random seed as an unsigned integer by usig XOR on time and thread id, which produces a unique seed for each thread

	for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {

		int source_id = rand_r(&seed) % NUM_ACCOUNTS;
		int destination_id = rand_r(&seed) % NUM_ACCOUNTS;
		while (source_id == destination_id){
			destination_id = rand_r(&seed) % NUM_ACCOUNTS;
		}

		double amount = (rand_r(&seed) % 100) + 1; // Generate random amount between 1 and 100 using rand_r(&seed), so that seed is changed after each random operation

		transfer_unsafe(source_id, destination_id, amount);
		printf("Teller %d: Transfered $%.2f from Account %d to Account %d\n", teller_id, amount, source_id, destination_id);

		// if (operation == 1) {
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
		accounts[i].account_id = i; // Initialize the accounts
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
	}

	// Display initial state (GIVEN)
	printf("Initial State:\n");
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf(" Account %d: $%.2f\n", i, accounts[i].balance);
	}

	double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE; // Find the expected total by multiplying the number of accounts by their initial balance

	printf("\nExpected total: $%.2f\n\n", expected_total);


	pthread_t threads[NUM_THREADS];
	int thread_ids[NUM_THREADS]; // GIVEN: Separate array for IDs

	int result; // Variable used for checking if threads are created and joined successfully

	for (int i = 0; i < NUM_THREADS; i++) {
		thread_ids[i] = i; // GIVEN: Store ID persistently
		result = pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]); // Initialize each thread
		if (result != 0){ // Exit program if there is an error in creating a thread
			printf("Error when creating thread.");
			exit(1);
		}
	}

	struct timespec start, end; // Start Timer
    clock_gettime(CLOCK_MONOTONIC, &start);

	for (int i = 0; i < NUM_THREADS; i++) {
		result = pthread_join(threads[i], NULL); // Join each thread
		if (result != 0){ // Exit program if there is an error in joining a thread
			printf("Error when joining thread.");
			exit(1);
		}
	}
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
