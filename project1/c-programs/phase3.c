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


#define NUM_ACCOUNTS 2
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

time_t last_progress;
pthread_mutex_t last_progress_mutex;

void initialize_accounts() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;

        // Initialize the mutex
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}

// GIVEN: Conceptual example showing HOW deadlock occurs
void transfer_deadlock_example(int from_id, int to_id, double amount) {
// This code WILL cause deadlock!

// Lock source account
pthread_mutex_lock(&accounts[from_id].lock);
printf("Thread %ld: Locked account %d\n", pthread_self(), from_id);

// Simulate processing delay
usleep(100);

// Try to lock destination account
printf("Thread %ld: Waiting for account %d\n", pthread_self(), to_id);
pthread_mutex_lock(&accounts[to_id].lock); // DEADLOCK HERE!

// Transfer (never reached if deadlocked)
accounts[from_id].balance -= amount;
accounts[to_id].balance += amount;

pthread_mutex_unlock(&accounts[to_id].lock);
pthread_mutex_unlock(&accounts[from_id].lock);
}


// TODO 1: Implement complete transfer function
// Use the example above as reference
// Add balance checking (sufficient funds?)
// Add error handling
void transfer_deadlock(int source_id, int destination_id, double amount){
    pthread_mutex_lock(&accounts[source_id].lock);

    pthread_mutex_lock(&last_progress_mutex);
    last_progress = time(NULL);
    pthread_mutex_unlock(&last_progress_mutex);

    printf("Thread %ld: Locked account %d\n", pthread_self(), source_id);
    usleep(100);
    printf("Thread %ld: Waiting for account %d\n", pthread_self(), destination_id);
    pthread_mutex_lock(&accounts[destination_id].lock);

    pthread_mutex_lock(&last_progress_mutex);
    last_progress = time(NULL);
    pthread_mutex_unlock(&last_progress_mutex);

    if (accounts[source_id].balance < amount){
        pthread_mutex_unlock(&accounts[source_id].lock);

        pthread_mutex_lock(&last_progress_mutex);
        last_progress = time(NULL);
        pthread_mutex_unlock(&last_progress_mutex);

        printf("\nAccount %d has an insufficient balance, transfer with Account %d cancelled\n", source_id, destination_id);
        return;
    }
    accounts[source_id].balance -= amount;
    accounts[destination_id].balance += amount;
    accounts[source_id].transaction_count++;
    accounts[destination_id].transaction_count++;

    pthread_mutex_unlock(&accounts[destination_id].lock);
    pthread_mutex_unlock(&accounts[source_id].lock);

    pthread_mutex_lock(&last_progress_mutex);
    last_progress = time(NULL);
    pthread_mutex_unlock(&last_progress_mutex);
}

// TODO 2: Create threads that will deadlock
// Thread 1: transfer(0, 1, amount) // Locks 0, wants 1
// Thread 2: transfer(1, 0, amount) // Locks 1, wants 0
// Result: Circular wait!
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

		transfer_deadlock(source_id, destination_id, amount);
		printf("\nTeller %d: Transfered $%.2f from Account %d to Account %d\n", teller_id, amount, source_id, destination_id);
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
}


int main(){
    printf("=== Phase 3: Create Deadlock Demo ===\n\n");

    initialize_accounts();
    
    pthread_mutex_init(&threads_executed_mutex, NULL); // Initialize threads_executed mutex

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
			printf("Error when creating thread.\n");
			exit(1);
		}
	}
    // Start timeout timer
    
    pthread_mutex_lock(&last_progress_mutex);
    last_progress = time(NULL);
    pthread_mutex_unlock(&last_progress_mutex);

    while (1){ // Timer counting loop
        sleep(1);
        pthread_mutex_lock(&last_progress_mutex);
        if ((time(NULL) - last_progress) >= 5){
            printf("\nPossible deadlock found, no progress for 5 seconds, program terminated.\n");
            exit(1);
        }
        pthread_mutex_unlock(&last_progress_mutex);
        if (threads_executed == NUM_THREADS){
            break;
        }
    }


	for (int i = 0; i < NUM_THREADS; i++) { // Join threads
		result = pthread_join(threads[i], NULL);
		if (result != 0){
			printf("Error when joining thread.\n");
			exit(1);
		}
	}
    cleanup_mutexes();
    

	printf("\n=== Final Results ===\n");

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
    printf("\n\nNo deadlock found.\n\n");
	printf("Run program again to test for different results.\n");

	return 0;
}


// TODO 3: Implement deadlock detection
// Add timeout counter in main()
// If no progress for 5 seconds, report suspected deadlock
// Reference: time(NULL) for simple timing

// TODO 4: Document the Coffman conditions
// In your report, identify WHERE each condition occurs
// Create resource allocation graph showing circular wait