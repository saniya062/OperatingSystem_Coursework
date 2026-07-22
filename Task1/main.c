#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS   5
#define ITERATIONS    100000

/* Shared resource accessed by every thread */
long shared_counter = 0;

/* Mutex protecting shared_counter — only one thread may
   modify it at a time, preventing a race condition */
pthread_mutex_t counter_lock;

void *worker_thread(void *arg) {
    long id = (long)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&counter_lock);   /* enter critical section */
        shared_counter++;                    /* protected update */
        pthread_mutex_unlock(&counter_lock); /* leave critical section */
    }

    printf("[Thread %ld] finished %d increments.\n", id, ITERATIONS);
    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];

    printf("=== Multi-threaded Counter with Mutex Synchronization ===\n");
    printf("Expected final counter value: %d\n", NUM_THREADS * ITERATIONS);

    pthread_mutex_init(&counter_lock, NULL);

    /* create threads */
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, (void *)i);
    }

    /* wait for all threads to finish */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Actual final counter value:   %ld\n", shared_counter);
    printf(shared_counter == NUM_THREADS * ITERATIONS
           ? "RESULT: No race condition - mutex worked correctly.\n"
           : "RESULT: Race condition detected!\n");

    pthread_mutex_destroy(&counter_lock);
    return 0;
}
