#include <stdio.h>

__attribute__((used))
int main(int argc, char *argv[], char * envp[])
{
    for (int i = 0; i < argc; i++) {
	printf("%s\n", argv[i]);
    }
    for (int i = 0; envp[i] != NULL; i++)
        printf("\n%s", envp[i]);
    return 0;
}
