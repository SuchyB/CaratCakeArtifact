#include <stdlib.h>
#include <stdio.h>

#define SIZE 16
int main()
{
    printf("hello world\n");
    printf("hello world!\n");
    volatile int *arr = (int *) (malloc(sizeof(int) * SIZE));
    printf("asdfasdfhello world!\n");
    printf("hello world!\n");
    printf("hello world!\n");
    for (int i = 0; i < SIZE ; i++) arr[i] = i;

    free(arr);

    return 0;
}
