#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
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
   PART 3: DEADLOCK PREVENTION DEMO
   ------------------------------------------------------------
   Two threads each need to hold TWO locks (lock_A and lock_B)
   at the same time to do their work.

   THE DANGER: if Thread 1 locks A then waits for B, while
   Thread 2 locks B then waits for A, neither can ever proceed —
   this is a deadlock caused by "circular wait."

   THE FIX: enforce a strict global lock ordering. Every thread,
   no matter what it's doing, must always acquire lock_A BEFORE
   lock_B. This makes circular wait impossible, because no
   thread will ever be found holding B while waiting for A.
   ------------------------------------------------------------ */

pthread_mutex_t lock_A;
pthread_mutex_t lock_B;

void *safe_task_1(void *arg) {
    (void)arg;
    printf("[Thread 1] locking A...\n");
    pthread_mutex_lock(&lock_A);
    usleep(100000); /* simulate doing some work while holding A */

    printf("[Thread 1] locking B...\n");
    pthread_mutex_lock(&lock_B);

    printf("[Thread 1] holds A and B, working...\n");
    usleep(100000);

    pthread_mutex_unlock(&lock_B);
    pthread_mutex_unlock(&lock_A);
    printf("[Thread 1] released A and B.\n");
    return NULL;
}

void *safe_task_2(void *arg) {
    (void)arg;
    /* Notice: this thread ALSO locks A before B — the same
       order as Thread 1. That consistent ordering is the
       entire deadlock-prevention strategy here. */
    printf("[Thread 2] locking A...\n");
    pthread_mutex_lock(&lock_A);
    usleep(100000);

    printf("[Thread 2] locking B...\n");
    pthread_mutex_lock(&lock_B);

    printf("[Thread 2] holds A and B, working...\n");
    usleep(100000);

    pthread_mutex_unlock(&lock_B);
    pthread_mutex_unlock(&lock_A);
    printf("[Thread 2] released A and B.\n");
    return NULL;
}

void run_deadlock_prevention_demo(void) {
    pthread_t t1, t2;

    printf("=== PART 3: Deadlock Prevention Demo ===\n");
    printf("Strategy: consistent lock ordering (always A before B)\n");
    printf("prevents circular wait, one of the four necessary\n");
    printf("conditions for deadlock.\n\n");

    pthread_mutex_init(&lock_A, NULL);
    pthread_mutex_init(&lock_B, NULL);

    pthread_create(&t1, NULL, safe_task_1, NULL);
    pthread_create(&t2, NULL, safe_task_2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("\nRESULT: Both threads completed successfully. No deadlock occurred.\n");

    pthread_mutex_destroy(&lock_A);
    pthread_mutex_destroy(&lock_B);
}

/* ------------------------------------------------------------
   MAIN
   ------------------------------------------------------------ */
int main(void) {
    run_threading_demo();
    run_round_robin_demo();
    run_deadlock_prevention_demo();
    return 0;
}
