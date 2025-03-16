#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BIN_CAPACITY 10
#define ITEM_COUNT 1000000
#define DEBUG 0

typedef struct {
    int thread_id;
    int start_index;
    int end_index;
    int capacity;
    int* items;
    int* local_bins_used;
} ThreadData;

static int total_bins = 0;
pthread_mutex_t bin_mutex = PTHREAD_MUTEX_INITIALIZER;

void createData(int size, int capacity);
void* bestFitThread(void* arg);

int main(int argc, char** argv) {
    int items[ITEM_COUNT];
    int thread_count;
    if (argc < 2) {
        perror(
            "You have not entered a thread count, running in a single thread");
        thread_count = 1;
    } else if (argc == 2) {
        thread_count = atoi(argv[1]);
        printf("Starting with %d threads\n", thread_count);
    } else {
        perror("Error with the command line arguments");
    }
    pthread_t threads[thread_count];
    ThreadData thread_data[thread_count];
    int local_bins_used[thread_count];

    // Create test data
    createData(ITEM_COUNT, BIN_CAPACITY);

    // Read all items from file
    FILE* file = fopen("data.txt", "r");
    if (file == NULL) {
        perror("Error opening data file");
        return 1;
    }

    int count = 0;
    while (count < ITEM_COUNT && fscanf(file, "%d", &items[count]) == 1) {
        count++;
    }
    fclose(file);

    if (DEBUG) {
        // Display items for verification
        printf("Items to pack: ");
        for (int i = 0; i < count; i++) {
            printf("%d ", items[i]);
        }
        printf("\n");
    }
    // Divide items among threads
    int items_per_thread = count / thread_count;
    int remaining_items = count % thread_count;

    int start_index = 0;
    for (int i = 0; i < thread_count; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start_index = start_index;

        // Distribute remaining items among first few threads
        int extra = (i < remaining_items) ? 1 : 0;
        int thread_items = items_per_thread + extra;

        thread_data[i].end_index = start_index + thread_items;
        thread_data[i].capacity = BIN_CAPACITY;
        thread_data[i].items = items;
        thread_data[i].local_bins_used = &local_bins_used[i];

        start_index += thread_items;
    }

    // Create threads
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&threads[i], NULL, bestFitThread,
                           (void*)&thread_data[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    // Sum up the bins used by each thread
    total_bins = 0;
    for (int i = 0; i < thread_count; i++) {
        total_bins += local_bins_used[i];
    }

    printf("Number of total_bins required in Multi-threaded Best Fit: %d\n",
           total_bins);

    pthread_mutex_destroy(&bin_mutex);
    return 0;
}

void createData(int size, int capacity) {
    FILE* file;
    file = fopen("data.txt", "w+");
    if (file == NULL) {
        perror("Error opening data file");
        exit(1);
    }

    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        fprintf(file, "%d\n", rand() % capacity + 1);
    }
    fclose(file);
}

void* bestFitThread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int size = data->end_index - data->start_index;

    // Create bins and track their remaining capacity
    int* bin_capacity = calloc(size, sizeof(int));
    for (int i = 0; i < size; i++) {
        bin_capacity[i] =
            data->capacity;  // Initialize all bins to full capacity
    }

    int used_bins = 0;

    // Process each item in this thread's range
    for (int i = data->start_index; i < data->end_index; i++) {
        int current_item = data->items[i];
        int best_bin = -1;
        int min_remaining = data->capacity + 1;

        // Find the best bin that can fit this item
        for (int j = 0; j < used_bins; j++) {
            if (bin_capacity[j] >= current_item &&
                bin_capacity[j] - current_item < min_remaining) {
                best_bin = j;
                min_remaining = bin_capacity[j] - current_item;
            }
        }

        // If no bin can fit the item, create a new bin
        if (best_bin == -1) {
            bin_capacity[used_bins] = data->capacity - current_item;
            used_bins++;
        } else {
            // Add to the best bin
            bin_capacity[best_bin] -= current_item;
        }
    }

    // Store the number of bins used by this thread
    *(data->local_bins_used) = used_bins;

    // Debug - print thread status
    printf("Thread %d: Processed items %d to %d, used %d bins\n",
           data->thread_id, data->start_index, data->end_index - 1, used_bins);

    free(bin_capacity);
    pthread_exit(NULL);
}
