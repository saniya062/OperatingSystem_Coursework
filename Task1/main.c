#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define NUM_THREADS   5
#define ITERATIONS    100000

/* ------------------------------------------------------------
   PART 1: MULTI-THREADED COUNTER WITH MUTEX SYNCHRONIZATION
   ------------------------------------------------------------ */
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

void run_threading_demo(void) {
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
}

/* ------------------------------------------------------------
   PART 2: ROUND-ROBIN CPU SCHEDULER SIMULATION
   ------------------------------------------------------------ */

/* A "process" being scheduled by the round-robin algorithm */

typedef struct {
    int pid;
    int burst_time;       /* total CPU time this process needs */
    int remaining_time;   /* time left to complete */
    int waiting_time;
    int turnaround_time;
    int completion_time;
} Process;

void run_round_robin_demo(void) {
    int quantum = 2;   /* time slice given to each process per turn */

    Process procs[] = {
        {1, 5, 5, 0, 0, 0},
        {2, 8, 8, 0, 0, 0},
        {3, 3, 3, 0, 0, 0},
        {4, 6, 6, 0, 0, 0}
    };
    int n = sizeof(procs) / sizeof(procs[0]);
    int time_elapsed = 0;
    int completed = 0;

    printf("=== PART 2: Round-Robin CPU Scheduler Simulation ===\n");
    printf("Time Quantum = %d\n\n", quantum);
    printf("Gantt Chart:\n");

    /* Keep cycling through all processes, giving each one
       "quantum" units of CPU time per turn, until all are done */
    while (completed < n) {
        bool all_done = true;

        for (int i = 0; i < n; i++) {
            if (procs[i].remaining_time > 0) {
                all_done = false;

                int run_time = (procs[i].remaining_time < quantum)
                                ? procs[i].remaining_time
                                : quantum;

                printf("| P%d (%d-%d) ", procs[i].pid, time_elapsed, time_elapsed + run_time);

                time_elapsed += run_time;
                procs[i].remaining_time -= run_time;

                if (procs[i].remaining_time == 0) {
                    procs[i].completion_time = time_elapsed;
                    procs[i].turnaround_time = procs[i].completion_time;      /* arrival = 0 */
                    procs[i].waiting_time = procs[i].turnaround_time - procs[i].burst_time;
                    completed++;
                }
            }
        }

        if (all_done) break;
    }
    printf("|\n\n");

    int total_wait = 0, total_turnaround = 0;
    printf("%-6s%-12s%-14s%-16s\n", "PID", "Burst", "Waiting", "Turnaround");
    for (int i = 0; i < n; i++) {
        printf("%-6d%-12d%-14d%-16d\n",
               procs[i].pid, procs[i].burst_time,
               procs[i].waiting_time, procs[i].turnaround_time);
        total_wait += procs[i].waiting_time;
        total_turnaround += procs[i].turnaround_time;
    }

    printf("\nAverage Waiting Time:    %.2f\n", (float)total_wait / n);
    printf("Average Turnaround Time: %.2f\n", (float)total_turnaround / n);
}

/* ------------------------------------------------------------
   MAIN
   ------------------------------------------------------------ */
int main(void) {
    run_threading_demo();
    run_round_robin_demo();
    return 0;
}
