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
#define TRANSACTIONS_PER_THREAD 20
#define INITIAL_BALANCE 1000.0
#define TRANSFER_METHOD 0 //Set to 0 to use Time Ordering or set to 1 to use Timeout Mechanism for Deadlock Resolution

typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

int threads_executed = 0; 
pthread_mutex_t threads_executed_mutex;

void initialize_accounts() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;

        // Initialize the mutexes
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}

time_t last_progress; 
pthread_mutex_t last_progress_mutex;

void update_progress(){
    pthread_mutex_lock(&last_progress_mutex);
    last_progress = time(NULL);
    pthread_mutex_unlock(&last_progress_mutex);
}

// Transfer function implemented with Lock Ordering
void safe_transfer_ordered(int teller_id, int source_id, int destination_id, double amount){
// Check which account has a lower id number, and then lock accounts in order from lowest to highest
    if (source_id < destination_id){
        pthread_mutex_lock(&accounts[source_id].lock);
        printf("\033[92mThread %d: Locked account %d\033[0m\n", teller_id, source_id);
        usleep(100);
        printf("\033[94mThread %d: Waiting for account %d\033[0m\n", teller_id, destination_id);
        pthread_mutex_lock(&accounts[destination_id].lock);
        printf("\033[92mThread %d: Locked account %d\033[0m\n", teller_id, destination_id);
    } else {
        pthread_mutex_lock(&accounts[destination_id].lock);
        printf("\033[92mThread %d: Locked account %d\033[0m\n", teller_id, destination_id);
        usleep(100);
        printf("\033[94mThread %d: Waiting for account %d\033[0m\n", teller_id, source_id);
        pthread_mutex_lock(&accounts[source_id].lock);
        printf("\033[92mThread %d: Locked account %d\033[0m\n", teller_id, source_id);
    }
    // Check if source account has enough balance for the transfer
    if (accounts[source_id].balance < amount){
        // If source account does not have enought balance, unlock accounts in the reverse order that they were locked
        if (source_id < destination_id){
            pthread_mutex_unlock(&accounts[destination_id].lock); 
            pthread_mutex_unlock(&accounts[source_id].lock);
            update_progress();
        } else {
            pthread_mutex_unlock(&accounts[source_id].lock);
            pthread_mutex_unlock(&accounts[destination_id].lock);
            update_progress();
        }
        printf("Thread %d: Account %d has an insufficient balance, transfer with Account %d cancelled\n", teller_id,source_id, destination_id);
        return;
    }
    
    // Perform transfer
    accounts[source_id].balance -= amount;
    accounts[destination_id].balance += amount;
    accounts[source_id].transaction_count++;
    accounts[destination_id].transaction_count++;

    // Unlock accounts in the reverse order that they were locked
    if (source_id < destination_id){
        pthread_mutex_unlock(&accounts[destination_id].lock);
        pthread_mutex_unlock(&accounts[source_id].lock);
        update_progress();
    } else {
        pthread_mutex_unlock(&accounts[source_id].lock);
        pthread_mutex_unlock(&accounts[destination_id].lock);
        update_progress();
    }
    printf("\033[1mThread %d: Transferred $%.2f from Account %d to Account %d \033[96m- Accounts %d and %d unlocked.\033[0m\n", teller_id, amount, source_id, destination_id, source_id, destination_id);
}

// Transfer function implemented with Timeout Mechanism
void safe_transfer_timeout(int teller_id,unsigned int *seed, int source_id, int destination_id, double amount){
    struct timespec timeout;
    // Timeout mechanism loop, and exit if both accounts are unlocked
    while (1) {
        clock_gettime(CLOCK_REALTIME, &timeout); // Set timeout to current real time
        timeout.tv_sec += 2; // Set timeout for 2 seconds ahead of current time
        if (pthread_mutex_timedlock(&accounts[source_id].lock, &timeout) == 0){ // Attempt to lock source account
            clock_gettime(CLOCK_REALTIME, &timeout); // Update timeout time
            timeout.tv_sec += 2; // Add 2 seconds to timeout time
            printf("\033[92mThread %d: Locked account %d\033[0m\n", teller_id, source_id);
            usleep(100);
            printf("\033[94mThread %d: Waiting for account %d\033[0m\n", teller_id, destination_id);
            if (pthread_mutex_timedlock(&accounts[destination_id].lock, &timeout) == 0){ // Attempt to lock destination account, restart loop if unsuccessful
                printf("\033[92mThread %d: Locked account %d\033[0m\n", teller_id, destination_id);
                break; // Exit loop if both accounts locked successfully
            }
            printf("\033[93mThread %d: Timed out while waiting to lock Account %d\033[0m\n", teller_id, destination_id);
            pthread_mutex_unlock(&accounts[source_id].lock);
            usleep(rand_r(seed) % 10000); // Random delay to prevent livelock
            continue;
        }
        printf("\033[93mThread %d timed out while waiting to lock Account %d\033[0m\n", teller_id, source_id);
    }
    
    if (accounts[source_id].balance < amount){
        pthread_mutex_unlock(&accounts[destination_id].lock);
        pthread_mutex_unlock(&accounts[source_id].lock);
        update_progress();
        printf("Thread %d: Account %d has an insufficient balance, transfer with Account %d cancelled\n", teller_id,source_id, destination_id);
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
        if (TRANSFER_METHOD == 0){
            safe_transfer_ordered(teller_id, source_id, destination_id, amount);
        } else{
            safe_transfer_timeout(teller_id,&seed, source_id, destination_id, amount);
        }
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
    printf("=== Phase 4: Deadlock Resolution Demo ===\n");
    if(TRANSFER_METHOD == 0){
        printf("\nTransfer Method: Lock Ordering\n");
    } else {
        printf("\nTransfer Method: Timeout Mechanism\n");
    }
    initialize_accounts();
    pthread_mutex_init(&threads_executed_mutex, NULL);
    pthread_mutex_init(&last_progress_mutex, NULL);
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
		if (result != 0){
			printf("\nError when creating thread.\n");
			exit(1);
		}
	}

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    update_progress();

    while (1){ // Timer counting loop
        sleep(1);
        pthread_mutex_lock(&last_progress_mutex);
        if ((time(NULL) - last_progress) >= 5){
            printf("\n\033[1;91mTimeout error occured, no progress for 5 seconds, program terminated.\033[0m\n");
            if (TRANSFER_METHOD == 1){
                printf("\nPOSSIBLE LIVELOCK DETECTED.\n\n");
            }
            exit(1);
        }
        if (threads_executed == NUM_THREADS){
            pthread_mutex_unlock(&last_progress_mutex);
            break;
        }
        pthread_mutex_unlock(&last_progress_mutex);
    }


	for (int i = 0; i < NUM_THREADS; i++) { // Join threads
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
    printf("\nExecution time: %.4f seconds\n\n", elapsed);

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