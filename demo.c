#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h> // for timing

#define MAX_NCPU 16           // Maximum number of CPUs
#define MAX_COMMAND_LENGTH 256 // Maximum command length for user input

// Structure to represent a job in the queue
typedef struct Job {
    pid_t pid;                // Process ID of the job
    char name[256];           // Name of the job
    int waitTime;             // Total waiting time in milliseconds
    int runtime;              // Total runtime in milliseconds
    int completed;            // Flag to indicate if the job is completed
    int priority;             // Job priority
    time_t startTime;         // Time when the job was started
    time_t lastWaitStart;     // Time when the job was last paused
    struct Job *next;         // Pointer to the next job in the queue
} Job;

// Global variables
Job *readyQueue = NULL; // Pointer to the head of the job queue
int nCPU = 0;           // Number of available CPUs
int TSLICE = 0;         // Time slice duration in milliseconds

// Function to get current time in milliseconds
long current_time_ms() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1e6; // Convert time to milliseconds
}

// Function to add a new job to the ready queue
void addJobToQueue(pid_t pid, const char *name, int priority) {
    Job *newJob = malloc(sizeof(Job)); // Allocate memory for the new job
    if (!newJob) {
        perror("malloc failed"); // Handle memory allocation failure
        return;
    }
    // Initialize job attributes
    newJob->pid = pid;
    strncpy(newJob->name, name, 255);
    newJob->name[255] = '\0'; // Ensure null termination of job name
    newJob->runtime = 0;      // Initialize runtime
    newJob->waitTime = 0;     // Initialize wait time
    newJob->completed = 0;    // Set job as not completed
    newJob->priority = priority; // Set job priority
    newJob->startTime = current_time_ms(); // Record start time
    newJob->lastWaitStart = newJob->startTime; // Track start of waiting period
    newJob->next = NULL;      // Initialize next pointer to NULL

    // Insert the new job into the ready queue
    if (readyQueue == NULL) {
        readyQueue = newJob; // If the queue is empty, set it to the new job
    } else {
        Job *current = readyQueue; // Traverse to the end of the queue
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newJob; // Add the new job at the end
    }
    printf("Added job %s (PID: %d) with priority %d to the queue\n", name, pid, priority);
}

// Function to send signals to pause and resume jobs
void scheduler_signal(Job *job, int signal) {
    kill(job->pid, signal); // Send the specified signal to the job's PID
}

// Function to run the job scheduler
void runScheduler() {
    Job *currentJob; // Pointer to the current job being processed
    while (readyQueue) { // Continue until there are no jobs in the queue
        currentJob = readyQueue;

        // Process each job in the queue
        while (currentJob != NULL) {
            if (!currentJob->completed) { // Only process if the job is not completed
                // Update wait time for the current job
                currentJob->waitTime += current_time_ms() - currentJob->lastWaitStart;
                
                printf("Starting job %s (PID: %d) for %d ms\n", currentJob->name, currentJob->pid, TSLICE);

                // Resume job execution
                scheduler_signal(currentJob, SIGCONT);
                usleep(TSLICE * 1000); // Wait for the duration of the time slice

                // Stop job execution
                scheduler_signal(currentJob, SIGSTOP);
                currentJob->runtime += TSLICE; // Update the job's runtime

                // Check completion status of the job
                int status;
                pid_t result = waitpid(currentJob->pid, &status, WNOHANG); // Non-blocking check for job completion
                if (result == currentJob->pid && WIFEXITED(status)) {
                    // Mark the job as completed
                    currentJob->completed = 1;
                    printf("Job %s (PID: %d) completed\n", currentJob->name, currentJob->pid);

                    // Print job statistics
                    printf("PID: %d\n", currentJob->pid);
                    printf("Wait Time: %d ms\n", currentJob->waitTime);
                    printf("Completion Time: %d ms\n", currentJob->runtime);
                } else {
                    // Update the last wait start time for the next scheduling cycle if not completed
                    currentJob->lastWaitStart = current_time_ms();
                }
            }
            currentJob = currentJob->next; // Move to the next job in the queue
        }

        // Remove completed jobs from the queue
        currentJob = readyQueue;
        Job *prev = NULL; // Pointer to the previous job for linked list traversal

        while (currentJob != NULL) {
            if (currentJob->completed) { // If the job is completed
                // Adjust the pointers to remove the completed job from the queue
                if (prev == NULL) {
                    readyQueue = currentJob->next; // Remove the head of the queue
                } else {
                    prev->next = currentJob->next; // Bypass the completed job
                }
                Job *toFree = currentJob; // Store the job to free its memory
                currentJob = currentJob->next; // Move to the next job
                free(toFree); // Free memory of the completed job
            } else {
                prev = currentJob; // Move to the next job in the queue
                currentJob = currentJob->next;
            }
        }
    }
}

// Function to display statistics of all jobs in the queue
void displayJobStats() {
    Job *current = readyQueue; // Start from the head of the queue
    printf("Job Statistics:\n");
    while (current != NULL) { // Traverse the queue
        printf("Job %s (PID: %d) - Wait Time: %d ms, Completion Time: %d ms\n", 
               current->name, current->pid, current->waitTime, current->runtime);
        current = current->next; // Move to the next job
    }
}

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    printf("\nCaught SIGINT (Ctrl+C). Terminating SimpleShell...\n");
    runScheduler(); // Run the scheduler to finish pending jobs
    displayJobStats(); // Display job statistics before exiting
    exit(0); // Exit the program
}

int main(int argc, char **argv) {
    // Register the SIGINT signal handler
    signal(SIGINT, sigint_handler);

    // Ensure the correct number of command line arguments are provided
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE in ms>\n", argv[0]);
        exit(1); // Exit with error if arguments are missing
    }
    nCPU = atoi(argv[1]); // Set number of CPUs from command line argument
    TSLICE = atoi(argv[2]); // Set time slice duration from command line argument

    printf("[0s] Starting SimpleShell with %d CPU(s) and %d ms time slice\n", nCPU, TSLICE);

    char command[MAX_COMMAND_LENGTH]; // Buffer for user commands
    while (1) {
        printf("SimpleShell$ "); // Prompt for user input
        fgets(command, MAX_COMMAND_LENGTH, stdin); // Read user input

        // Check if the user wants to submit a new job
        if (strncmp(command, "submit", 6) == 0) {
            char jobName[256]; // Buffer for job name
            int priority = 1;  // Default priority for the job
            sscanf(command, "submit %s %d", jobName, &priority); // Parse job name and priority

            pid_t pid = fork(); // Create a new process for the job
            if (pid == 0) {
                execlp(jobName, jobName, (char *)NULL); // Execute the job in the child process
                perror("execlp failed"); // Handle execution failure
                exit(1); // Exit child process on failure
            } else if (pid > 0) {
                addJobToQueue(pid, jobName, priority); // Add the job to the queue in the parent process
            } else {
                perror("fork failed"); // Handle fork failure
            }
        } 
        // Check if the user wants to exit the shell
        else if (strncmp(command, "exit", 4) == 0) {
            printf("Terminating SimpleShell...\n");
            runScheduler(); // Run the scheduler to finish pending jobs
            displayJobStats(); // Display job statistics before exiting
            break; // Exit the loop
        } 
        // Handle unknown commands
        else {
            printf("Unknown command: %s", command);
        }
    }
    return 0; // Return success
}

