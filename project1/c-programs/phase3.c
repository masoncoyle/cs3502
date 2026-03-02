//  Liam Mason Coyle
//  CS 3502 Section W03
//  Project 1

//  AI Assistance: Claude Sonnet 4.6 (Anthropic), used to debug code and for general reference
//  Date accessed: February - March 2026
//  URL: https://claude.ai

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

typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock; // NEW: Mutex for this account
} Account;

Account accounts[NUM_ACCOUNTS];

int threads_executed = 0; // Track amount of threads that have finished executing
pthread_mutex_t threads_executed_mutex; // Create mutex for threads_executed

time_t last_progress; // Global variable to track the last point in time that threads made progress in the program
pthread_mutex_t last_progress_mutex; // Define mutex for last_progress

// Method to update last_progress with the current time
void update_progress(){
    pthread_mutex_lock(&last_progress_mutex);
    last_progress = time(NULL);
    pthread_mutex_unlock(&last_progress_mutex);
}

void initialize_accounts() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;

        // Initialize the mutex
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}


// Transfer method that leads to deadlock
void transfer_deadlock(int teller_id, int source_id, int destination_id, double amount){
    pthread_mutex_lock(&accounts[source_id].lock);
    printf("\033[92mThread %d: Locked account %d\033[0m\n", teller_id, source_id);
    usleep(100); // Simulate processing delay
    printf("\033[94mThread %d: Waiting for account %d\033[0m\n", teller_id, destination_id);
    pthread_mutex_lock(&accounts[destination_id].lock);

    if (accounts[source_id].balance < amount){ // Check if source account has sufficient balance for transfer
        pthread_mutex_unlock(&accounts[destination_id].lock);
        pthread_mutex_unlock(&accounts[source_id].lock);
        update_progress(); // Update last_progress after finishing transaction and unlocking both accounts
        printf("\nAccount %d has an insufficient balance, transfer with Account %d cancelled\n", source_id, destination_id);
        return;
    }
    accounts[source_id].balance -= amount;
    accounts[destination_id].balance += amount;
    accounts[source_id].transaction_count++;
    accounts[destination_id].transaction_count++;

    pthread_mutex_unlock(&accounts[destination_id].lock);
    pthread_mutex_unlock(&accounts[source_id].lock);

    update_progress();
    printf("\033[1mThread %d: Transferred $%.2f from Account %d to Account %d \033[96m- Accounts %d and %d unlocked.\033[0m\n", teller_id, amount, source_id, destination_id, source_id, destination_id);
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

		transfer_deadlock(teller_id, source_id, destination_id, amount); // Implement deadlock inducing transfer method
	}
    pthread_mutex_lock(&threads_executed_mutex);
    threads_executed ++;
    pthread_mutex_unlock(&threads_executed_mutex);
	return NULL;
}
void cleanup_mutexes() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
    pthread_mutex_destroy(&threads_executed_mutex);
    pthread_mutex_destroy(&last_progress_mutex);
}


int main(){
    printf("=== Phase 3: Create Deadlock Demo ===\n\n");

    initialize_accounts();
    
    pthread_mutex_init(&threads_executed_mutex, NULL); // Initialize threads_executed mutex
    pthread_mutex_init(&last_progress_mutex, NULL); // Initialize last_progress mutex

	printf("Initial State:\n");
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf(" Account %d: $%.2f\n", i, accounts[i].balance);
	}

	double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE;  

	printf("\nExpected total: $%.2f\n\n", expected_total);

	pthread_t threads[NUM_THREADS];
	int thread_ids[NUM_THREADS];

	int result;

	for (int i = 0; i < NUM_THREADS; i++) { // Create threads
		thread_ids[i] = i;
		result = pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
		if (result != 0){
			printf("\nError when creating thread.\n");
			exit(1);
		}
	}
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Start timeout timer
    update_progress();
    // Loop to print statement and exit program if deadlock occurs (no progress in 5 seconds)
    while (1){
        sleep(1); // Wait one second
        pthread_mutex_lock(&last_progress_mutex); // Lock mutex
        if ((time(NULL) - last_progress) >= 5){ // Check if 5 seconds has elapsed since last time progress was made
            printf("\n\033[1;91mPossible deadlock found, no progress for 5 seconds, program terminated.\033[0m\n\n"); // Print that deadlock was detected
            exit(1); // Exit if no progress
        }
        pthread_mutex_unlock(&last_progress_mutex); // Unlock mutex
        if (threads_executed == NUM_THREADS){ // Exit loop if all threads completed their transactions
            break;
        }
        // Repeat loop if 5 seconds has not elapsed
    }


	for (int i = 0; i < NUM_THREADS; i++) {
		result = pthread_join(threads[i], NULL);
		if (result != 0){
			printf("\nError when joining thread.\n");
			exit(1);
		}
	}

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    cleanup_mutexes();
    

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

    printf("\n\nNO DEADLOCK DETECTED\n\n");

	printf("Run program again to test for different results.\n\n");

	return 0;
}