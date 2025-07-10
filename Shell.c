#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define INPUT_SIZE 1024
#define HISTORY_SIZE 100

void shell_loop();
int launch(char **args, bool background);
char* read_user_input();
void cd_command(char **args);
int execute(char *command, bool background);
int create_process_and_run(char **args, bool background);
void handle_pipe(char *command);
char* command_history[HISTORY_SIZE];
int history_count = 0;
bool background = false;
void handle_sigint(int sig);

int main() {
    shell_loop();
    return 0;
}

void shell_loop() {
    int status;
    do {
        printf("simple-shell> ");
        char* command = read_user_input();
        if (command == NULL) {
            break;
        }
        status = execute(command, background);
        free(command);
    } while (status);
}

void handle_sigint(int sig) {
    printf("\nhandling SIGINT\n");
}

int execute(char* command, bool background) {
    // Store the command in history
    if (history_count < HISTORY_SIZE) {
        command_history[history_count++] = strdup(command);
    } else {
        free(command_history[0]);
        for (int j = 1; j < HISTORY_SIZE; j++) {
            command_history[j - 1] = command_history[j];
        }
        command_history[HISTORY_SIZE - 1] = strdup(command);
    }

    // Check if the command contains a pipe
    if (strchr(command, '|') != NULL) {
        handle_pipe(command);
        return 1;
    }

    // Split the command into arguments
    char* args[INPUT_SIZE];
    char* token = strtok(command, " ");
    int i = 0;

    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
  
    // Check if the command is 'history'
    if (strcmp(args[0], "history") == 0) {
        for (int j = 0; j < history_count; j++) {
            printf("%d) %s\n", j + 1, command_history[j]);
        }
        return 1;
    }

    // Check for background process (if last argument is "&")
    if (i > 1 && strcmp(args[i - 1], "&") == 0) {
        background = true;
        args[i - 1] = NULL; // Remove '&' from arguments
    } else {
        background = false;
    }

    // Launch the command
    return launch(args, background);
}

void handle_pipe(char *command) {
    char *cmds[INPUT_SIZE];
    char *cmd = strtok(command, "|");
    int cmd_count = 0;

    // Split command by pipes
    while (cmd != NULL) {
        cmds[cmd_count++] = cmd;
        cmd = strtok(NULL, "|");
    }
    cmds[cmd_count] = NULL;

    int pipe_fds[2 * (cmd_count - 1)];
    pid_t pid;

    // Create pipes
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipe_fds + i * 2) == -1) {
            perror("pipe error");
            exit(EXIT_FAILURE);
        }
    }

    // Execute each command
    for (int i = 0; i < cmd_count; i++) {
        pid = fork();
        if (pid == 0) {
            // Child process
            if (i != 0) {
                // If not the first command, get input from previous pipe
                dup2(pipe_fds[(i - 1) * 2], 0);
            }
            if (i != cmd_count - 1) {
                // If not the last command, output to next pipe
                dup2(pipe_fds[i * 2 + 1], 1);
            }
            // Close all pipe fds
            for (int j = 0; j < 2 * (cmd_count - 1); j++) {
                close(pipe_fds[j]);
            }

            // Split command into arguments
            char *args[INPUT_SIZE];
            char *token = strtok(cmds[i], " ");
            int arg_index = 0;

            while (token != NULL) {
                args[arg_index++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_index] = NULL;

            if (execvp(args[0], args) == -1) {
                perror("execvp error");
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            perror("fork error");
            exit(EXIT_FAILURE);
        }
    }

    // Close all pipe fds in the parent process
    for (int i = 0; i < 2 * (cmd_count - 1); i++) {
        close(pipe_fds[i]);
    }

    // Wait for all child processes
    for (int i = 0; i < cmd_count; i++) {
        wait(NULL);
    }
}

int launch(char **args, bool background) {

    struct sigaction sa;
    sa.sa_handler = &handle_sigint;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("execvp error");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        return 2;
    } else {
        // Parent process
        if (!background) {
            wpid = waitpid(pid, &status, 0);
            if (wpid == -1) {
                return 3;
            } else {
                if (status == 0) {
                    printf("Process completed successfully\n");
                } else {
                    printf("Process terminated with status: %d\n", status);
                }
            }
        } else {
            printf("Running in background, PID: %d\n", pid);
        }
    }
    return 1; // Return 1 to continue the loop
}

char* read_user_input() {
    char* input = malloc(INPUT_SIZE);
    if (input == NULL) {
        perror("Unable to allocate memory");
        exit(EXIT_FAILURE);
    }

    if (fgets(input, INPUT_SIZE, stdin) == NULL) {
        free(input);
        return NULL;
    }

    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';  // Remove trailing newline
    }
    return input;
}

