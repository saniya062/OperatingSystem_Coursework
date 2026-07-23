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

int find_in_frames(int page) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frames[i] == page) return i;
    }
    return -1; /* not currently in memory */
}

void print_frame_state(int step, int page, bool was_fault) {
    printf("Step %2d | Request page %d | %-4s | Frames: [ ",
           step, page, was_fault ? "FAULT" : "HIT");
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frames[i] == -1) printf("_ ");
        else printf("%d ", frames[i]);
    }
    printf("]\n");
}

int main(void) {
    int fifo_pointer = 0;   /* index of the OLDEST frame - the next one to evict */
    int page_faults = 0;
    int page_hits = 0;

    printf("=== Paging System: FIFO Page Replacement ===\n");
    printf("Page size: %d bytes | Frames available: %d\n\n", PAGE_SIZE, NUM_FRAMES);

    for (int i = 0; i < NUM_FRAMES; i++) frames[i] = -1; /* all frames start empty */

    for (int step = 0; step < REF_LENGTH; step++) {
        int page = reference_string[step];
        int loc = find_in_frames(page);

        if (loc != -1) {
            /* PAGE HIT: page already resident in memory, nothing to load */
            page_hits++;
            print_frame_state(step + 1, page, false);
        } else {
            /* PAGE FAULT: page not in memory, must bring it in.
               FIFO always evicts whichever page has been resident
               the longest, tracked by fifo_pointer cycling 0..NUM_FRAMES-1 */
            frames[fifo_pointer] = page;
            fifo_pointer = (fifo_pointer + 1) % NUM_FRAMES;
            page_faults++;
            print_frame_state(step + 1, page, true);
        }
    }

    printf("\n--- Statistics ---\n");
    printf("Total references: %d\n", REF_LENGTH);
    printf("Page hits:         %d\n", page_hits);
    printf("Page faults:       %d\n", page_faults);
    printf("Hit ratio:  %.2f%%\n", (page_hits / (float)REF_LENGTH) * 100);
    printf("Fault ratio: %.2f%%\n", (page_faults / (float)REF_LENGTH) * 100);

    return 0;
}
