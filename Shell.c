#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

#define INPUT_SIZE 1024
#define HISTORY_SIZE 100

void shell_loop();
int launch(char **args, bool background);
char* read_user_input();
void cd_command(char **args);
int execute(char *command, bool background);
int create_process_and_run(char **args, bool background);
char* command_history[HISTORY_SIZE];
int history_count = 0;
bool background = false;

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

    // Split the command into arguments
    char* args[INPUT_SIZE];
    char* token = strtok(command, " ");
    int i = 0;

    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // Check if the command is 'cd'
    if (strcmp(args[0], "cd") == 0) {
        cd_command(args); // Call the cd command
        return 1;
    }

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

int launch(char **args, bool background) {
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
        perror("fork error");
    } else {
        // Parent process
        if (!background) {

            wpid=waitpid(pid,&status,0);
            if(wpid==-1){
                perror("waitpid error");
            }
            else{
                if(status==0){
                    printf("process completed successfully\n");
                }
                else{
                    printf("process terminated with status: %d\n",status);
                }
            }
            // Wait for the child process to complete if it's not running in the background
      //      do {
      //          wpid = waitpid(pid, &status, WUNTRACED);
      //      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        } else {
            printf("Running in background, PID: %d\n", pid);
        }
    }
    return 1; // Return 1 to continue the loop
}

void cd_command(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Expected argument to 'cd'\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("chdir error");
        }
    }
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

