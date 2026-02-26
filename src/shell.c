#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include "command.h"

#define MAX_INPUT_SIZE 1024
#define MAX_LINE MAX_INPUT_SIZE


// Function prototypes
void parse_command(char *line, char **args);
int parse_with_redirection(char *line, Command *cmd);
void trim_whitespace(char *str);
int execute_builtin(char **args);
void print_help(void);
int find_in_path(const char *command, char *full_path);
void execute_external(char **args);
void execute_with_redirection(Command *cmd);

int main(void) { 
    char line[MAX_LINE];
    Command cmd;

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
        if (parse_with_redirection(line_copy, &cmd) < 0) {
            continue; // Parsing error
        }

        if (cmd.args[0] == NULL) {
            continue; // No command to execute   
        }

        // Check for built-in commands
        if (strcmp(cmd.args[0], "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }
        else if (strcmp(cmd.args[0], "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            }
            continue;
        }
        else if (strcmp(cmd.args[0], "cd") == 0) {
            const char *path = cmd.args[1] ? cmd.args[1] : getenv("HOME"); // Default to home directory
            if (chdir(path) != 0) {
                perror("cd error");
            } 
            continue;
        }
        else if (strcmp(cmd.args[0], "help") == 0) {
            print_help();
            continue;
        }
        // Execute external command with redirection
        execute_with_redirection(&cmd);

        for (int i = 0; cmd.args[i] != NULL; i++) {
            free(cmd.args[i]); // Free allocated argument strings
            cmd.args[i] = NULL; // Clear the argument pointer
        }
    }

    return 0;
}


int parse_with_redirection(char *line, Command *cmd) {
    memset(cmd, 0, sizeof(Command)); 
    cmd->append_mode = 0; 
    cmd->input_file[0] = '\0';
    cmd->output_file[0] = '\0';

    char *token = strtok(line, " \t");
    int arg_index = 0;

    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected input file after '<'\n");
                return -1;
            }
            strcpy(cmd->input_file, token);
        }  else if (strcmp(token, ">") == 0) {
            // Handle output redirection (overwrite)
            token = strtok(NULL, " \t");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected output file after '>'\n");
                return -1;
            }
            strcpy(cmd->output_file, token);
            cmd->append_mode = 0; // Overwrite mode
        } else if (strcmp(token, ">>") == 0) {
            // Handle output redirection (append)
            token = strtok(NULL, " \t");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected output file after '>>'\n");
                return -1;
            }
            strcpy(cmd->output_file, token);
            cmd->append_mode = 1; // Append mode
        } else {
            if (arg_index < MAX_ARGS - 1) {
                cmd->args[arg_index++] = strdup(token); // Duplicate token for argument
            } else {
                fprintf(stderr, "Error: too many arguments\n");
                return -1;
            }
        }
        token = strtok(NULL, " \t");
    }
    cmd->args[arg_index] = NULL; // Null-terminate the argument list
    return 0; // Successfully parsed command with redirection
}

void execute_with_redirection(Command *cmd) {
    pid_t pid;
    int status;

    if(cmd->args[0] == NULL) {
        return; // No command to execute
    }

    pid = fork();

    if (pid < 0) {
        perror("fork error");
        return;
    } 
    if (pid == 0) {
        // Child process

        // Handle input redirection
        if (cmd->input_file[0] != '\0') {
            int fd_in = open(cmd->input_file, O_RDONLY);
            if (fd_in < 0) {
                perror("Input file error");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        // Handle output redirection
        if (cmd->output_file[0] != '\0') {
            int fd_out;
            if (cmd->append_mode) {
                fd_out = open(cmd->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            } else {
                fd_out = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            if (fd_out < 0) {
                perror("Output file error");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        if (execvp(cmd->args[0], cmd->args) == -1) {
            perror("exec error");
            exit(1);
        }
    } else {
        // Parent process waits for child to finish
        wait(&status);
        
    }
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
    printf("\nRedirection supported: <, >, >>\n");
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
    strcpy(path_copy, path_env);

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
        wait(&status);
    }
}
