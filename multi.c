#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define BIN_CAPACITY 10
#define ITEM_COUNT 500
#define NUM_THREADS 4

typedef struct {
    int thread_id;
    int start_index;
    int size;
    int capacity;
    int bins_used;
} ThreadData;

static int total_bins = 0;
pthread_mutex_t bin_mutex = PTHREAD_MUTEX_INITIALIZER;

void* bestFitThread(void* arg);
void createData(int size, int capacity);
void finalBestFit(int capacity);
void writeRemainingItems(int thread_id, int* remaining_items, int remaining_count);

int main() {
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    
    createData(ITEM_COUNT, BIN_CAPACITY);
    
    int items_per_thread = ITEM_COUNT / NUM_THREADS;
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start_index = i * items_per_thread;
        thread_data[i].size = (i == NUM_THREADS - 1) ? (ITEM_COUNT - i * items_per_thread) : items_per_thread;
        thread_data[i].capacity = BIN_CAPACITY;
        thread_data[i].bins_used = 0;
        
        if (pthread_create(&threads[i], NULL, bestFitThread, (void*)&thread_data[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Process remaining items
    finalBestFit(BIN_CAPACITY);
    
    printf("Number of total_bins required in Multi-threaded Best Fit: %d\n", total_bins);
    
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
    int bin_capacity[data->size];
    int used_bins = 0;
    int* remaining_items = malloc(data->size * sizeof(int));
    int remaining_count = 0;
    
    FILE* file;
    file = fopen("data.txt", "r");
    if (file == NULL) {
        perror("Error opening data file");
        pthread_exit(NULL);
    }
    
    int current_line = 0;
    // Skip to start index
    int temp;
    while (current_line < data->start_index && fscanf(file, "%d", &temp) == 1) {
        current_line++;
    }
    
    for (int i = 0; i < data->size; i++) {
        // Read current data from the file
        int current_data;
        if (fscanf(file, "%d", &current_data) != 1) {
            break;
        }
        
        int min = data->capacity + 1;
        int bin_index = 0;
        
        // Find the best bin
        for (int j = 0; j < used_bins; j++) {
            if (bin_capacity[j] >= current_data &&
                bin_capacity[j] - current_data < min) {
                bin_index = j;
                min = bin_capacity[j] - current_data;
            }
        }
        
        // No best bin exists - create new bin
        if (min == data->capacity + 1) {
            bin_capacity[used_bins] = data->capacity - current_data;
            used_bins++;
        } else {
            // Best bin is found, reduce remaining capacity
            bin_capacity[bin_index] -= current_data;
        }
        
        // If a bin has significant remaining capacity, save the info for later
        for (int j = 0; j < used_bins; j++) {
            if (bin_capacity[j] >= data->capacity / 2) {
                remaining_items[remaining_count++] = data->capacity - bin_capacity[j];
            }
        }
    }
    
    fclose(file);
    
    // Update the total number of bins safely
    pthread_mutex_lock(&bin_mutex);
    total_bins += used_bins;
    pthread_mutex_unlock(&bin_mutex);
    
    // Write remaining items to separate file for this thread
    writeRemainingItems(data->thread_id, remaining_items, remaining_count);
    
    free(remaining_items);
    
    data->bins_used = used_bins;
    pthread_exit(NULL);
}

void writeRemainingItems(int thread_id, int* remaining_items, int remaining_count) {
    if (remaining_count == 0) {
        return;
    }
    
    char filename[32];
    sprintf(filename, "remaining_%d.txt", thread_id);
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening remaining items file");
        return;
    }
    
    for (int i = 0; i < remaining_count; i++) {
        fprintf(file, "%d\n", remaining_items[i]);
    }
    
    fclose(file);
}

void finalBestFit(int capacity) {
    FILE* output_file = fopen("combined_remaining.txt", "w");
    if (output_file == NULL) {
        perror("Error opening combined remaining file");
        return;
    }
    
    // Combine all remaining items files
    for (int i = 0; i < NUM_THREADS; i++) {
        char filename[32];
        sprintf(filename, "remaining_%d.txt", i);
        
        FILE* input_file = fopen(filename, "r");
        if (input_file == NULL) {
            continue;  // Skip if file doesn't exist
        }
        
        int item;
        while (fscanf(input_file, "%d", &item) == 1) {
            fprintf(output_file, "%d\n", item);
        }
        
        fclose(input_file);
        remove(filename);  // Clean up individual files
    }
    
    fclose(output_file);
    
    // Now perform final best fit on the combined remaining items
    int remaining_count = 0;
    int* remaining_items = NULL;
    
    FILE* count_file = fopen("combined_remaining.txt", "r");
    if (count_file != NULL) {
        int item;
        while (fscanf(count_file, "%d", &item) == 1) {
            remaining_count++;
        }
        fclose(count_file);
        
        if (remaining_count > 0) {
            remaining_items = malloc(remaining_count * sizeof(int));
            
            FILE* data_file = fopen("combined_remaining.txt", "r");
            if (data_file != NULL) {
                for (int i = 0; i < remaining_count; i++) {
                    fscanf(data_file, "%d", &remaining_items[i]);
                }
                fclose(data_file);
            }
            
            // Apply best fit to remaining items
            int bin_capacity[remaining_count];
            int used_bins = 0;
            
            for (int i = 0; i < remaining_count; i++) {
                int current_data = remaining_items[i];
                int min = capacity + 1;
                int bin_index = 0;
                
                // Find the best bin
                for (int j = 0; j < used_bins; j++) {
                    if (bin_capacity[j] >= current_data &&
                        bin_capacity[j] - current_data < min) {
                        bin_index = j;
                        min = bin_capacity[j] - current_data;
                    }
                }
                
                // No best bin exists
                if (min == capacity + 1) {
                    bin_capacity[used_bins] = capacity - current_data;
                    used_bins++;
                } else {
                    // Best bin is found, reduce remaining capacity
                    bin_capacity[bin_index] -= current_data;
                }
            }
            
            // Update total bins
            total_bins += used_bins;
            
            free(remaining_items);
        }
    }
    
    remove("combined_remaining.txt");  // Clean up
}