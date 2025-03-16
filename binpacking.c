#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BIN_CAPACITY 10
#define ITEM_COUNT 1000000

static int total_bins = 0;

void bestFit(int start_index, int size, int capacity);
void createData(int size, int capacity);

int main() {
    //createData(ITEM_COUNT, BIN_CAPACITY);
    bestFit(0, ITEM_COUNT, BIN_CAPACITY);
    printf("Number of total_bins required in Best Fit: %d\n", total_bins);
}

void createData(int size, int capacity) {
    FILE* file;
    file = fopen("data.txt", "w+");
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        fprintf(file, "%d\n", rand() % capacity + 1);
    }
    fclose(file);
}

void bestFit(int start_index, int size, int capacity) {
    int bin_capacity[size];
    int used_bins = 0;
    FILE* file;
    file = fopen("data.txt", "r");
    int current_line = 0;
    // skip to start index
    while (current_line < start_index &&
           fscanf(file, "%d", (int*)NULL) != EOF) {
        current_line++;
    }
    for (int i = 0; i < size; i++) {
        // read current data from the file
        int current_data;
        fscanf(file, "%d", &current_data);

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
        } else  // Best bin is found, reduce remaining capacity
        {
            bin_capacity[bin_index] -= current_data;
        }
    }
    total_bins += used_bins;
}
