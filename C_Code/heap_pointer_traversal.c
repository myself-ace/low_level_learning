#include <stdio.h>
#include <stdlib.h>

int main(void) {

    // Allocate memory for 5 integers on the heap
    int *arr = malloc(5 * sizeof(int));

    // Always check if malloc succeeded
    if (arr == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Copy pointer used for traversal
    int *cp_ptr = arr;

    // Store values inside allocated memory
    for (int i = 0; i < 5; ++i) {
        *cp_ptr = 100 + i;
        cp_ptr++;
    }

    // Restore pointer back to the beginning
    cp_ptr = arr;

    // Print stored values and their memory addresses
    for (int i = 0; i < 5; ++i) {
        printf("Value: %d | Address: %p\n", *cp_ptr, (void *)cp_ptr);
        cp_ptr++;
    }

    // Free dynamically allocated memory
    free(arr);

    // Prevent dangling pointers
    arr = NULL;
    cp_ptr = NULL;

    return 0;
}
