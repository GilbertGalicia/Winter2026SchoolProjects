#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_LINE MAX_INPUT_SIZE

int main(void) {

    char line[MAX_LINE];

    while (1) {
        printf("mysh> ");
        fflush(stdout);
        
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            printf("\n");
            break; // EOF
        }

        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;

        // For now just print the input
        printf("You entered: %s\n", line);
    }

    return 0;
}