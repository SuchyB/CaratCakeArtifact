#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define NAUTILUS_EXE
#include <nautilus/nautilus_exe.h>

volatile int a = 1;

// 0x1400000UL 

int main() {
    printf("Hello, world!\n");
    void* ptrs[500] = {a};
    int *others = (int *) calloc(500, sizeof(int));
    for (int i = 0; i < 500; i++) {
        printf("Iteration %d\n", i);
        fflush(stdout);
        const int size = 0x1400000UL / 500;
        ptrs[i] = malloc(size);

        *(uint64_t*)ptrs[i] = i;
    }
    for (int i = 0; i < 500; i++) {
        printf("%d: %d, %d, %p\n", i, *(uint64_t*)ptrs[i], others[i], others);
    }
    volatile uint64_t *new_arr = (uint64_t *) realloc(others, 500 * sizeof(uint64_t));
    return 0;
}
