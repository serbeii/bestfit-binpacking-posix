#include <stdio.h>

#define BIN_SIZE 10
#define MAX_BINS 100

typedef struct {
    int size;
    int bin_index;
} Item;

typedef struct {
    int data[BIN_SIZE];
    int current_load;
    Item items[10];
} Bin;

struct Node {
    Bin bin;
    struct Node* next;
}typedef BinList;

int insert_to_list(BinList **list, Bin bin);

int main() {}
