#include <stdio.h>
#include <stdbool.h>

#define PAGE_SIZE     4096   /* configurable: bytes per page */
#define NUM_FRAMES    3      /* number of physical memory frames available */
#define REF_LENGTH    12     /* length of the page reference string */

/* The sequence of pages the process requests, in order.
   This simulates a program touching memory over time. */
int reference_string[REF_LENGTH] = {1, 2, 3, 2, 4, 1, 5, 2, 1, 2, 3, 5};

/* Physical memory: holds the page number in each frame,
   or -1 if the frame is empty */
int frames[NUM_FRAMES];

/* Results captured after each demo runs, used to build the
   side-by-side comparison table at the end of the program. */
typedef struct {
    const char *name;
    int hits;
    int faults;
} AlgoResult;

AlgoResult results[2];

void reset_frames(void) {
    for (int i = 0; i < NUM_FRAMES; i++) frames[i] = -1;
}

int find_in_frames(int page) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frames[i] == page) return i;
    }
    return -1; /* not currently in memory */
}

/* Detailed logging of memory allocation:
   - shows the full frame layout after every reference
   - on a fault, explicitly names which page was evicted (or
     "none" if a frame was simply empty and got filled) */
void log_step(int step, int page, bool was_fault, int evicted_page) {
    printf("Step %2d | Request page %2d | %-5s", step, page, was_fault ? "FAULT" : "HIT");

    if (was_fault) {
        if (evicted_page == -1)
            printf(" (loaded into free frame)  ");
        else
            printf(" (evicted page %d)          ", evicted_page);
    } else {
        printf("                              ");
    }

    printf("| Memory: [ ");
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frames[i] == -1) printf("_ ");
        else printf("%d ", frames[i]);
    }
    printf("]\n");
}
void print_stats(const char *label, int hits, int faults) {
    printf("\n--- %s Statistics ---\n", label);
    printf("Total references: %d\n", REF_LENGTH);
    printf("Page hits:         %d\n", hits);
    printf("Page faults:       %d\n", faults);
    printf("Hit ratio:   %.2f%%\n", (hits / (float)REF_LENGTH) * 100);
    printf("Fault ratio: %.2f%%\n\n", (faults / (float)REF_LENGTH) * 100);
}
/* ------------------------------------------------------------
   PART 1: FIFO (First-In-First-Out) PAGE REPLACEMENT
   Evicts whichever page has been resident in memory longest,
   regardless of how recently/often it was used.
   ------------------------------------------------------------ */
void run_fifo_demo(void) {
    int fifo_pointer = 0;   /* index of the OLDEST frame - next to evict */
    int page_faults = 0;
    int page_hits = 0;

    printf("=== PART 1: FIFO Page Replacement ===\n");
    printf("Page size: %d bytes | Frames available: %d\n\n", PAGE_SIZE, NUM_FRAMES);

    reset_frames();

    for (int step = 0; step < REF_LENGTH; step++) {
        int page = reference_string[step];
        int loc = find_in_frames(page);

        if (loc != -1) {
            page_hits++;
            log_step(step + 1, page, false, -1);
        } else {
	    int evicted = frames[fifo_pointer];
            frames[fifo_pointer] = page;
            fifo_pointer = (fifo_pointer + 1) % NUM_FRAMES;
            page_faults++;
	    log_step(step + 1, page, true, evicted);
        }
    }

    print_stats("FIFO", page_hits, page_faults);
    results[0] = (AlgoResult){"FIFO", page_hits, page_faults};
}

/* ------------------------------------------------------------
   PART 2: LRU (Least Recently Used) PAGE REPLACEMENT
   Evicts whichever page has gone the longest without being
   accessed, tracked via a "last used" timestamp per frame.
   ------------------------------------------------------------ */
int last_used[NUM_FRAMES];

int find_empty_frame(void) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frames[i] == -1) return i;
    }
    return -1; /* no empty frame - memory is full */
}

int find_lru_victim(void) {
    int victim = 0;
    for (int i = 1; i < NUM_FRAMES; i++) {
        if (last_used[i] < last_used[victim]) victim = i;
    }
    return victim;
}

void run_lru_demo(void) {
    int page_faults = 0;
    int page_hits = 0;

    printf("=== PART 2: LRU Page Replacement ===\n");
    printf("Page size: %d bytes | Frames available: %d\n\n", PAGE_SIZE, NUM_FRAMES);

    reset_frames();
    for (int i = 0; i < NUM_FRAMES; i++) last_used[i] = -1;

    for (int step = 0; step < REF_LENGTH; step++) {
        int page = reference_string[step];
        int loc = find_in_frames(page);

        if (loc != -1) {
            last_used[loc] = step;   /* refresh recency */
            page_hits++;
            log_step(step + 1, page, false, -1);
        } else {
            int target = find_empty_frame();
	    int evicted = -1;
            if (target == -1) {
	    	target = find_lru_victim();
	    	evicted = frames[target];
            } 

	    frames[target] = page;
            last_used[target] = step;
            page_faults++;
            log_step(step + 1, page, true, evicted);
        }
    }

    print_stats("LRU", page_hits, page_faults);
    results[1] = (AlgoResult){"LRU", page_hits, page_faults};
}

/* ------------------------------------------------------------
   PART 3: SIDE-BY-SIDE COMPARISON (visualization of results)
   ------------------------------------------------------------ */
void print_comparison(void) {
    printf("=== PART 3: FIFO vs LRU Comparison ===\n\n");
    printf("%-10s%-10s%-10s%-14s%-14s\n", "Algorithm", "Hits", "Faults", "Hit Ratio", "Fault Ratio");
    for (int i = 0; i < 2; i++) {
        printf("%-10s%-10d%-10d%-14.2f%-14.2f\n",
               results[i].name, results[i].hits, results[i].faults,
               (results[i].hits / (float)REF_LENGTH) * 100,
               (results[i].faults / (float)REF_LENGTH) * 100);
    }

    printf("\n");
    if (results[0].faults < results[1].faults)
        printf("RESULT: FIFO produced fewer page faults on this reference string.\n");
    else if (results[1].faults < results[0].faults)
        printf("RESULT: LRU produced fewer page faults on this reference string.\n");
    else
        printf("RESULT: Both algorithms produced the same number of page faults.\n");
}

/* ------------------------------------------------------------
   MAIN
   ------------------------------------------------------------ */
int main(void) {
    run_fifo_demo();
    run_lru_demo();
    print_comparison();
    return 0;
}
