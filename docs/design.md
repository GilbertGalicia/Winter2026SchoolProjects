# Shell Design Document
**Your Name**  
**COMP 340**  
**Date: February 6, 2026**
**Repository:** `https://sc-gitlab.ufv.ca/202601comp340on1/gilberto/project`

## Architecture Overview

### Module Structure
The shell will use a simple modular design with these files:

- **main.c** - Core shell loop and user interface
- **parser.c** - Command parsing and tokenization  
- **builtins.c** - Built-in command implementations
- **executor.c** - Process creation and management
- **utils.c** - Helper functions and error handling

### Data Structures
// For storing a single command
typedef struct {
    char *args[64];          // Command and its arguments
    char input_file[256];    // For input redirection (< file)
    char output_file[256];   // For output redirection (> file)
    int append_mode;         // 1 for >> (append), 0 for > (overwrite)
    int background;          // 1 if command ends with &
} Command;

// For pipes (multiple commands connected)
typedef struct {
    Command commands[10];    // Up to 10 commands in a pipeline
    int command_count;       // How many commands in this pipeline
    int has_pipe;            // 1 if pipeline exists
} Pipeline;

// Global shell state
typedef struct {
    char cwd[1024];          // Current working directory
    int job_count;           // Number of background jobs
    int last_exit_status;    // Exit status of last command
} ShellState;


## Parsing Strategy         

### Tokenization

I'll use strtok() to split the input string by spaces. The parsing process will:
- Read the entire line from user input
- Split into tokens using space as delimiter
- Identify special tokens: |, <, >, >>, &
- Group tokens into commands based on pipe operators

### Handling Special Characters
- |: Marks the end of one command and start of another in pipeline
- <: Next token is input file for redirection
- >: Next token is output file for redirection (overwrite)
- >>: Next token is output file for redirection (append)
- &: Command should run in background

## Process Management

### Basic Command Execution

For command like ls -la:
- Parent process calls fork() to create child
- Child calls execvp() to replace itself with ls
- Parent calls wait() to wait for child to finish
- Parent resumes and shows prompt again

### Redirection

For ls > output.txt:
- Fork child process
- Before execvp(), child opens output.txt with open()
- Use dup2() to redirect stdout to the file
- Then call execvp()

### Pipes
- Create pipe with pipe() system call
- Fork first child for ls
    - Redirect its stdout to pipe's write end
    - Close unused file descriptors
    - Execute ls
- Fork second child for grep txt
    - Redirect its stdin from pipe's read end
    - Close unused file descriptors
    - Execute grep
- Parent closes all pipe ends and waits for both children

### Background Processes
For sleep 10 &:
- Fork child process as usual
- Parent does NOT call wait() immediately
- Parent continues to show promp
- Child runs independently
- Parent will check for completed background jobs occasionally

### Signal Handling
- Ctrl+C (SIGINT): Send interrupt to foreground process group, not shell
- Ctrl+Z (SIGTSTP): Suspend foreground process
- Signal handlers will be set up with sigaction()

## Feature Plan

### Level 1 Feature (Core Shell) 
All required:
- Command prompt and input
- Command parsing
- Command execution with fork/exec
- PATH resolution
- Built-in commands: cd, pwd, exit, help
- Error handling

### Level 2 Feature (Standard Feature)
All required:
- I/O redirection: <, >, >>
- Pipes: |
- Background execution: &
- Signal handling: Ctrl+C, Ctrl+Z

### Level 3 Feature (Extended Feature)
Job Control
- `jobs` - List background jobs
- `fg [job_id]` - Bring job to foreground
- `bg [job_id]` - Resume job in background

Command Chaining
- `command1 && command2` - Run cmd2 only if cmd1 succeeds
- `command1 || command2` - Run cmd2 only if cmd1 fails

Command History
- `history` command to show recent commands
- Up/Down arrow keys to navigate history
- Save history between shell sessions