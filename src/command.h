#include <fcntl.h>

#ifndef COMMAND_H
#define COMMAND_H

#define MAX_ARGS 64
#define MAX_FILE 256

typedef struct {
    char *args[MAX_ARGS]; // Command arguments
    char input_file[MAX_FILE]; // Input redirection file
    char output_file[MAX_FILE]; // Output redirection file
    int append_mode;
} Command;

#endif 