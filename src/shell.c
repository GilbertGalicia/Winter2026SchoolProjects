#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_LINE MAX_INPUT_SIZE

// Function prototypes
void parse_command(char *line, char **args);
void trim_whitespace(char *str);
int execute_builtin(char **args);
void print_help(void);

int main(void) {

    char line[MAX_LINE];
    char *args[MAX_ARGS];

    while (1) {
        printf("mysh> ");
        fflush(stdout);
        
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            printf("\n");
            break; // EOF
        }

        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;
        trim_whitespace(line);

        if (strlen(line) == 0) {
            continue; // Empty input
        }   
        // Make a copy of the line for parsing
        char line_copy[MAX_LINE];
        strncpy(line_copy, line, MAX_LINE);
        // Parse the command into arguments
        parse_command(line_copy, args);
        // Check for built-in commands
        if (execute_builtin(args)) {
            continue; // Built-in command executed
        }
        // if we get here, it's an external command (not implemented yet)
        // for now, just print the command and its arguments
        printf("External command(not implemented yet): %s\n", line);
        for (int i = 0; args[i] != NULL; i++) {
            printf("Arg %d: %s\n", i, args[i]);
        }
        printf("\n");
    }

    return 0;
}

void parse_command(char *line, char **args) {
    char *token = strtok(line, " \t");
    int index = 0;

    while (token != NULL && index < MAX_ARGS - 1) {
        args[index++] = token;
        token = strtok(NULL, " \t");
    }
    args[index] = NULL; // Null-terminate the argument list
}

void trim_whitespace(char *str) {
    int len = strlen(str);
    // Trim trailing whitespace
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t')) {
        str[--len] = '\0';
        len--;
    }

    // Trim leading whitespace
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t') {
        start++;
    }
    if (start > 0) {
        memmove(str, str + start, len - start + 1);
    }
}

int execute_builtin(char **args) {
    if (args[0] == NULL) {
        return 1; // Not built-in command
    }

    //exit command
    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    // pwd command
    else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd() error");
        }
        return 0; // Built-in command executed
    }
    // cd command
    else if (strcmp(args[0], "cd") == 0) {
        const char *path;

        if (args[1] == NULL) {
            path = getenv("HOME"); // Default to home directory
        if (path == NULL) {
            fprintf(stderr, "cd: HOME environment variable not set\n");
            return 0; // Built-in command executed
        }
        } else {
            path = args[1];
        }
        if (chdir(path) != 0) {
            perror("cd error");
        } 
        return 0; // Built-in command executed
    }
    // help command
    else if (strcmp(args[0], "help") == 0) {
        print_help();
        return 0; // Built-in command executed
    }
    return 1; // Not a built-in command
}

void print_help(void) {
    printf("Built-in commands:\n");
    printf("  exit - Exit the shell\n");
    printf("  pwd  - Print the current working directory\n");
    printf("  cd   - Change the current working directory\n");
    printf("  help - Display this help message\n");
}