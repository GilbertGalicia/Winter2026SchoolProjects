#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>



#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_LINE MAX_INPUT_SIZE

// Function prototypes
void parse_command(char *line, char **args);
void trim_whitespace(char *str);
int execute_builtin(char **args);
void print_help(void);
int find_in_path(const char *command, char *full_path);
void execute_external(char **args);

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
        execute_external(args);
        
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
        return 0; // Not built-in command
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
        return 1; // Built-in command executed
    }
    // cd command
    else if (strcmp(args[0], "cd") == 0) {
        const char *path;

        if (args[1] == NULL) {
            path = getenv("HOME"); // Default to home directory
            if (path == NULL) {
                fprintf(stderr, "cd: HOME environment variable not set\n");
                return 1; // Built-in command executed
            }
        } else {
            path = args[1];
        }
        if (chdir(path) != 0) {
            perror("cd error");
        } 
        return 1; // Built-in command executed
    }
    // help command
    else if (strcmp(args[0], "help") == 0) {
        print_help();
        return 1; // Built-in command executed
    }
    return 0; // Not a built-in command
}

void print_help(void) {
    printf("Built-in commands:\n");
    printf("  exit - Exit the shell\n");
    printf("  pwd  - Print the current working directory\n");
    printf("  cd   - Change the current working directory\n");
    printf("  help - Display this help message\n");
    printf("\nExternal commands are also supported if they are in the PATH.\n");
}

int find_in_path(const char *command, char *full_path) {
    if (strchr(command, '/') != NULL) {
        // Command contains a slash, treat it as a path
        if (access(command, X_OK) == 0) {
            strncpy(full_path, command, MAX_LINE);
            return 1; // Found executable
        }
        return 0; // Not found
    }

    // Search in PATH
    char *path_env = getenv("PATH");
    if (path_env == NULL) {
        return 0; // PATH not set
    }

    // Make a copy of PATH for tokenization
    char path_copy[MAX_LINE];
    strncpy(path_copy, path_env, sizeof(path_copy)- 1);
    path_copy[sizeof(path_copy)- 1] = '\0';

    // Tokenize PATH and search for the command
    char *dir = strtok(path_copy, ":");

    while (dir != NULL) {
        snprintf(full_path, MAX_LINE, "%s/%s", dir, command); // Construct full path

        if (access(full_path, X_OK) == 0) { // Check if executable
            return 1; // Found executable
        }
        dir = strtok(NULL, ":");
    }
    return 0; // Not found
}

void execute_external(char **args) {
    pid_t pid;
    int status;

    if(args[0] == NULL) {
        return; // No command to execute
    }

    pid = fork();

    if (pid < 0) {
        perror("fork error");
        return;
    } 
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("exec error");
            exit(1);
        }
    } else {
        // Parent process waits for child to finish
        if (wait(&status) == -1) {
            perror("wait error");
            return;
        } 
        
    }
}
