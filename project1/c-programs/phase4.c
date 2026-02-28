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
#define NUM_THREADS 6
#define TRANSACTIONS_PER_THREAD 4
#define INITIAL_BALANCE 1000.0
#define TRANSFER_METHOD 1 //Set to 0 to use Time Ordering or set to 1 to use Timeout Mechanism for Deadlock Resolution

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

        // Initialize the mutex
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}


void safe_transfer_ordered(int teller_id, int source_id, int destination_id, double amount){ // Transfer function implemented with Lock Ordering
    if (source_id < destination_id){
        pthread_mutex_lock(&accounts[source_id].lock);
        printf("Thread %d: Locked account %d\n", teller_id, source_id);
        usleep(100);
        printf("Thread %d: Waiting for account %d\n", teller_id, destination_id);
        pthread_mutex_lock(&accounts[destination_id].lock);
        printf("Thread %d: Locked account %d\n", teller_id, destination_id);
    } else {
        pthread_mutex_lock(&accounts[destination_id].lock);
        printf("Thread %d: Locked account %d\n", teller_id, destination_id);
        usleep(100);
        printf("Thread %d: Waiting for account %d\n", teller_id, source_id);
        pthread_mutex_lock(&accounts[source_id].lock);
        printf("Thread %d: Locked account %d\n", teller_id, source_id);
    }

    if (accounts[source_id].balance < amount){
        if (source_id < destination_id){
        pthread_mutex_unlock(&accounts[destination_id].lock);
        pthread_mutex_unlock(&accounts[source_id].lock);
        
    } else {
        pthread_mutex_unlock(&accounts[source_id].lock);
        pthread_mutex_unlock(&accounts[destination_id].lock);
    }
        printf("Thread %d, Account %d has an insufficient balance, transfer with Account %d cancelled\n", teller_id,source_id, destination_id);
        return;
    }

    accounts[source_id].balance -= amount;
    accounts[destination_id].balance += amount;
    accounts[source_id].transaction_count++;
    accounts[destination_id].transaction_count++;

    if (source_id < destination_id){
        pthread_mutex_unlock(&accounts[destination_id].lock);
        pthread_mutex_unlock(&accounts[source_id].lock);
        
    } else {
        pthread_mutex_unlock(&accounts[source_id].lock);
        pthread_mutex_unlock(&accounts[destination_id].lock);
    }
    printf("Thread %d: Transfered $%.2f from Account %d to Account %d - Accounts %d and %d unlocked.\n", teller_id, amount, source_id, destination_id, source_id, destination_id);
}


void safe_transfer_timeout(int teller_id, int source_id, int destination_id, double amount){ // Transfer function implemented with Timeout Mechanism
    struct timespec timeout;
    while (1) {
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 2;
        if (pthread_mutex_timedlock(&accounts[source_id].lock, &timeout) == 0){
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 2;
            printf("Thread %d: Locked account %d\n", teller_id, source_id);
            usleep(100);
            printf("Thread %d: Waiting for account %d\n", teller_id, destination_id);
            if (pthread_mutex_timedlock(&accounts[destination_id].lock, &timeout) == 0){
                break;
            }
            printf("Thread %d timed out while waiting to lock Account %d\n", teller_id, destination_id);
            pthread_mutex_unlock(&accounts[source_id].lock);
            usleep(rand() % 10000);
            continue;
        }
        printf("Thread %d timed out while waiting to lock Account %d\n", teller_id, source_id);
    }
    // while (1) {
    //     clock_gettime(CLOCK_REALTIME, &timeout);
    //     timeout.tv_sec += 2;
    //     int result = pthread_mutex_timedlock(&accounts[destination_id].lock, &timeout);
    //     if (result == 0){
    //         break;
    //     }
    //     printf("Thread %d timed out while waiting to lock Account %d\n", teller_id, destination_id);
    // }
    
    if (accounts[source_id].balance < amount){
        pthread_mutex_unlock(&accounts[destination_id].lock);
        pthread_mutex_unlock(&accounts[source_id].lock);
        printf("\nAccount %d has an insufficient balance, transfer with Account %d cancelled\n", source_id, destination_id);
        return;
    }
    
    accounts[source_id].balance -= amount;
    accounts[destination_id].balance += amount;
    accounts[source_id].transaction_count++;
    accounts[destination_id].transaction_count++;

    pthread_mutex_unlock(&accounts[destination_id].lock);
    pthread_mutex_unlock(&accounts[source_id].lock);

    printf("Thread %d: Transfered $%.2f from Account %d to Account %d - Accounts %d and %d unlocked.\n", teller_id, amount, source_id, destination_id, source_id, destination_id);
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
            safe_transfer_timeout(teller_id, source_id, destination_id, amount);
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
}

int main(){
    printf("=== Phase 4: Deadlock Resolution Demo ===\n\n");
    initialize_accounts();
    pthread_mutex_init(&threads_executed_mutex, NULL);
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
			printf("Error when creating thread.\n");
			exit(1);
		}
	}

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    time_t start_time = time(NULL);
    time_t timeout_elapsed = 0;

    while (1){
        sleep(1);
        timeout_elapsed = time(NULL) - start_time;
        if (timeout_elapsed >= 5){
            printf("\nPossible deadlock found, no progress for 5 seconds, program terminated...\n");
            exit(1);
        }
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
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

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

	if (expected_total != actual_total){
		printf("\nRACE CONDITION DETECTED!\n");
	} else{
		printf("\nNO RACE CONDITION DETECTED\n");
	}

	printf("Run program again to test for different results.\n");

	return 0;
}



/* STRATEGY 1: Lock Ordering (RECOMMENDED)
 *
 * ALGORITHM:
 * To prevent circular wait, always acquire locks in consistent order
 *
 * Step 1: Identify which account ID is lower
 * Step 2: Lock lower ID first
 * Step 3: Lock higher ID second
 * Step 4: Perform transfer
 * Step 5: Unlock in reverse order
 *
 * WHY THIS WORKS:
 * - Thread 1: transfer(0,1) locks 0 then 1
 * - Thread 2: transfer(1,0) locks 0 then 1 (SAME ORDER!)
 * - No circular wait possible
 *
 * WHICH COFFMAN CONDITION DOES THIS BREAK?
 * Answer in your report!
 */

// TODO: Implement safe_transfer_ordered(from, to, amount)
// Use the algorithm description above
// Hint: int first = (from < to) ? from : to;

/* STRATEGY 2: Timeout Mechanism
 *
 * ALGORITHM:
 * Try to acquire locks with timeout; release and retry if timeout
 *
 * Step 1: pthread_mutex_timedlock on first account (2 sec timeout)
 * Step 2: If timeout, return and retry
 * Step 3: pthread_mutex_timedlock on second account (2 sec timeout)
 * Step 4: If timeout, release first lock and retry
 * Step 5: If both acquired, perform transfer
 *
 * REFERENCE: man pthread_mutex_timedlock
 */

// TODO: Implement safe_transfer_timeout(from, to, amount)

/* STRATEGY 3: Try-Lock with Backoff
 *
 * ALGORITHM:
 * Use non-blocking trylock; back off and retry if fails
 *
 * Step 1: pthread_mutex_trylock on first account
 * Step 2: If fails, usleep(random time) and retry
 * Step 3: pthread_mutex_trylock on second account
 * Step 4: If fails, release first, usleep, retry
 *
 * REFERENCE: man pthread_mutex_trylock
*/

// TODO: Implement safe_transfer_trylock(from, to, amount)

// TODO: Create comparison test harness
// Test all strategies with same workload
// Measure: success rate, average time, max retry count


// TODO: Write performance comparison in report
// Which strategy is fastest?
// Which is most reliable?
// Which would you recommend for production?