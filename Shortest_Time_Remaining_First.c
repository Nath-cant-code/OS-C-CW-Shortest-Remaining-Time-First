// This code was written in VS Code editor,
// hence the run code button can just be clicked to compile and run the program.

// Included libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

//  Define constants
#define MAX_PROC 10
#define MAX_TIMELINE 1000

// Enum for process states
typedef enum {
    READY,      // Process arrived and waiting for CPU
    RUNNING,    // Process currently executing
    COMPLETED   // Process finished execution
} ProcessState;

// Structure representing each process
typedef struct {
    int pid;               // Process ID (1, 2, 3...)
    int arrivalTime;       // Time when the process arrives
    int burstTime;         // CPU burst duration
    int remainingTime;     // Remaining CPU time
    int startTime;         // First time process gets CPU
    int completionTime;    // Time when process finishes
    int turnaroundTime;    // completionTime - arrivalTime
    int waitingTime;       // turnaroundTime - burstTime
    int responseTime;      // startTime - arrivalTime
    int finished;          // 0 = not completed, 1 = completed
    int hasStarted;        // Track if process has started execution
    ProcessState state;    // Current state of the process
    pthread_t thread;      // Thread for this process
} Process;

// Gantt chart structure
typedef struct {
    int pid;               // Process ID executing
    int startTime;         // Start time of this execution slice
    int endTime;           // End time of this execution slice
} GanttEntry;

// Shared data structure for threading
typedef struct {
    Process *processes;
    int n;
    int *currentTime;
    int *completed;
    int *currentProcess;
    bool *schedulerRunning;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    GanttEntry *gantt;
    int *ganttSize;
} SharedData;

// Global variables (shared data among threads)
Process processes[MAX_PROC];                                    // Sets a array with a maximum of 10 processes
int globalCurrentTime = 0;                                      // Global time tracker
int globalCompleted = 0;                                        // Number of completed processes
int globalCurrentProcess = -1;                                  // Currently executing process index
bool schedulerRunning = true;                                   // Scheduler running flag
pthread_mutex_t schedulerMutex = PTHREAD_MUTEX_INITIALIZER;     // Mutex for synchronizing access
pthread_cond_t schedulerCond = PTHREAD_COND_INITIALIZER;        // Condition variable for process scheduling
GanttEntry gantt[MAX_TIMELINE];                                 // Gantt chart entries
int ganttSize = 0;                                              // Number of entries in Gantt chart
int numProcesses = 0;                                           // Total number of processes

// Function prototypes
void sortByArrival(Process proc[], int n);
int findShortestJob(Process proc[], int n, int currentTime);
void *processThread(void *arg);
void *schedulerThread(void *arg);
void printResults(Process proc[], int n);
void printGanttChart(GanttEntry gantt[], int size);
void updateProcessStates(Process proc[], int n, int currentTime, int runningIdx);
void printProcessTable(Process proc[], int n, int currentTime);
const char* getStateName(ProcessState state);

int main() {
    // Variable declarations
    // n if for number of processes 
    // i for loop iteration
    int n, i;

    printf("======================================\n");
    printf("  SRTF Process Scheduling Simulator\n");
    printf("  (Multithreaded Implementation)\n");
    printf("======================================\n\n");
    
    // Input validation for number of processes
    do {
        printf("Enter number of processes (1-%d): ", MAX_PROC);

        // Check for valid integer input
        if (scanf("%d", &n) != 1) {
            while (getchar() != '\n');
            printf("Invalid input! Please enter a valid number.\n");
            n = 0;
            continue;
        }

        // Check user input range
        if (n < 1 || n > MAX_PROC) {
            printf("Invalid number! Must be between 1 and %d.\n", MAX_PROC);
        }

    // Repeat until valid input is received
    } while (n < 1 || n > MAX_PROC);

    // Set global number of processes
    numProcesses = n;

    // Input arrival and burst times with validation
    printf("\nEnter arrival and burst times:\n");

    // Loop to get each process's Arrival Time and Burst Time
    for (i = 0; i < n; i++) {
        processes[i].pid = i + 1;
        
        // Arrival time input validation
        do {
            printf("Process %d - Arrival Time: ", i + 1);

            // Check for valid integer input
            if (scanf("%d", &processes[i].arrivalTime) != 1) {
                while (getchar() != '\n');
                printf("Invalid input! Please enter a valid integer.\n");
                processes[i].arrivalTime = -1;
                continue;
            }

            // Print warning if Arrival Time is invalid
            if (processes[i].arrivalTime < 0) {
                printf("Arrival time cannot be negative!\n");
            }

        // Repeat until valid input is received
        } while (processes[i].arrivalTime < 0);

        // Burst time input validation
        do {
            printf("Process %d - Burst Time:   ", i + 1);

            // Check for valid integer input
            if (scanf("%d", &processes[i].burstTime) != 1) {
                while (getchar() != '\n');
                printf("Invalid input! Please enter a valid integer.\n");
                processes[i].burstTime = 0;
                continue;
            }

            // Print warning if Burst Time is invalid
            if (processes[i].burstTime < 1) {
                printf("Burst time must be at least 1!\n");
            }
        
        // Repeat until valid input is received
        } while (processes[i].burstTime < 1);

        // Initialise process fields
        processes[i].remainingTime = processes[i].burstTime;
        processes[i].startTime = -1;
        processes[i].completionTime = 0;
        processes[i].turnaroundTime = 0;
        processes[i].waitingTime = 0;
        processes[i].responseTime = 0;
        processes[i].finished = 0;
        processes[i].hasStarted = 0;
        processes[i].state = READY;  // Initialize state as READY
    }

    // Sort processes by arrival time
    sortByArrival(processes, n);

    printf("\n======================================\n");
    printf("  Execution Timeline (PREEMPTIVE)\n");
    printf("======================================\n");
    printf("Note: SRTF allows preemption - processes can be interrupted\n");
    printf("      when a shorter job arrives.\n");
    printf("Multithreading: Each process runs in its own thread,\n");
    printf("                coordinated by the scheduler thread.\n\n");
    printf("Process States: READY -> RUNNING -> COMPLETED\n\n");

    // Create pthread_t variable for scheduler
    pthread_t scheduler;

    // Creates and runs the scheduler thread
    // Checks if there is an error when creating the scheduler thread
    if (pthread_create(&scheduler, NULL, schedulerThread, NULL) != 0) {
        fprintf(stderr, "Error creating scheduler thread\n");
        return 1;
    }

    // Create process threads and runs the processes via processThread function
    // Checks if each thread is created successfully
    for (i = 0; i < n; i++) {
        if (pthread_create(&processes[i].thread, NULL, processThread, &processes[i]) != 0) {
            fprintf(stderr, "Error creating process thread %d\n", i + 1);
            return 1;
        }
    }

    // When a process a been scheduled, executed, and completed,
    // that process's thread will finish.
    // Hence, when all processes are done, then only the scheduler thread will finish.
    // When scheduler is done, the scheduler thread will join back to the main thread
    pthread_join(scheduler, NULL);

    // Joins all process threads back to main thread
    for (i = 0; i < n; i++) {
        pthread_join(processes[i].thread, NULL);
    }

    // Display results
    printResults(processes, n);
    
    // Display Gantt chart
    printGanttChart(gantt, ganttSize);

    // Cleanup
    pthread_mutex_destroy(&schedulerMutex);
    pthread_cond_destroy(&schedulerCond);

    return 0;
}

// Scheduler thread function to coordinate the process execution
void *schedulerThread(void *arg) {
    int lastProcess = -1;
    bool processArrivalPrinted[MAX_PROC] = {false};  // Track which processes have printed arrival

    // Checks if there is at least one process and if the first process arrives after time 0
    // If condition returns true, jump to first Arrival Time
    if (numProcesses > 0 && processes[0].arrivalTime > 0) {
        pthread_mutex_lock(&schedulerMutex);
        globalCurrentTime = processes[0].arrivalTime;
        pthread_mutex_unlock(&schedulerMutex);
    }

    // Print table header
    printf("%-6s %-12s %-12s %-15s %-10s\n", 
           "Time", "Process ID", "Status", "Remaining Time", "Thread ID");
    printf("--------------------------------------------------------------------------------\n");

    // Runs loop while there are still processes with time remaining
    while (schedulerRunning) {
        pthread_mutex_lock(&schedulerMutex);

        // Check for process arrivals and print READY status
        for (int i = 0; i < numProcesses; i++) {
            if (processes[i].arrivalTime == globalCurrentTime && !processArrivalPrinted[i]) {
                printf("%-6d %-12s %-12s %-15d %-10s\n", 
                       globalCurrentTime,
                       (processes[i].pid == 1) ? "P1" : (processes[i].pid == 2) ? "P2" : 
                       (processes[i].pid == 3) ? "P3" : (processes[i].pid == 4) ? "P4" :
                       (processes[i].pid == 5) ? "P5" : (processes[i].pid == 6) ? "P6" :
                       (processes[i].pid == 7) ? "P7" : (processes[i].pid == 8) ? "P8" :
                       (processes[i].pid == 9) ? "P9" : "P10",
                       "READY",
                       processes[i].remainingTime,
                       "-");
                processArrivalPrinted[i] = true;
            }
        }

        // Check if all processes completed
        // If true, set loop iteration condition to false
        if (globalCompleted >= numProcesses) {
            schedulerRunning = false;
            pthread_cond_broadcast(&schedulerCond);
            pthread_mutex_unlock(&schedulerMutex);
            break;
        }

        // Find process with shortest remaining time
        // Create variable to hold the index of the process's position in the array
        int idx = findShortestJob(processes, numProcesses, globalCurrentTime);

        // Update all process states before execution
        updateProcessStates(processes, numProcesses, globalCurrentTime, idx);

        // If there exists no process with a shorter remaining time than the current process,
        // that means the process can execute up until next closest Arrival Time of another process.
        if (idx == -1) {
            int nextArrival = __INT_MAX__;

            // Iterate through the processes array to find the next closest Arrival Time
            for (int i = 0; i < numProcesses; i++) {
                if (!processes[i].finished && processes[i].arrivalTime > globalCurrentTime) {
                    if (processes[i].arrivalTime < nextArrival) {
                        nextArrival = processes[i].arrivalTime;
                    }
                }
            }
            
            // Checks if there exists a next Arrival Time
            if (nextArrival != __INT_MAX__) {
                // Add idle time to Gantt chart
                if (ganttSize < MAX_TIMELINE) {
                    gantt[ganttSize].pid = 0;
                    gantt[ganttSize].startTime = globalCurrentTime;
                    gantt[ganttSize].endTime = nextArrival;
                    ganttSize++;
                }

                // Print CPU idle message
                printf("\n>>> Time %d-%d: CPU IDLE <<<\n\n", globalCurrentTime, nextArrival);
                globalCurrentTime = nextArrival;
            }
            pthread_mutex_unlock(&schedulerMutex);
            continue;
        }

        // Check for context switch (preemption)
        if (lastProcess != -1 && lastProcess != processes[idx].pid) {
            printf("\n>>> Time %d: **PREEMPTION** - Switching from P%d to P%d <<<\n\n", 
                   globalCurrentTime, lastProcess, processes[idx].pid);
        }

        // Set current process and signal it to execute
        globalCurrentProcess = idx;
        
        // Add to Gantt chart
        if (lastProcess == processes[idx].pid && ganttSize > 0) {
            gantt[ganttSize - 1].endTime = globalCurrentTime + 1;
        } 
        else {
            if (ganttSize < MAX_TIMELINE) {
                gantt[ganttSize].pid = processes[idx].pid;
                gantt[ganttSize].startTime = globalCurrentTime;
                gantt[ganttSize].endTime = globalCurrentTime + 1;
                ganttSize++;
            }
            lastProcess = processes[idx].pid;
        }

        // Wake up the selected process
        pthread_cond_broadcast(&schedulerCond);
        pthread_mutex_unlock(&schedulerMutex);

        // Small delay to simulate time slice execution (100ms delay)
        usleep(100000); 
    }

    return NULL;
}

// Process thread function to represent the individual process execution
void *processThread(void *arg) {
    Process *proc = (Process *)arg;

    // While true loop that only breaks if either:
    // scheduler stops running or process is finished
    while (1) {
        pthread_mutex_lock(&schedulerMutex);

        // Wait until this process is scheduled or scheduler stops
        while (globalCurrentProcess != (proc->pid - 1) && schedulerRunning) {
            pthread_cond_wait(&schedulerCond, &schedulerMutex);
        }

        // Exit if scheduler stopped
        if (!schedulerRunning) {
            pthread_mutex_unlock(&schedulerMutex);
            break;
        }

        // Check if process has arrived
        if (proc->arrivalTime > globalCurrentTime) {
            pthread_mutex_unlock(&schedulerMutex);
            continue;
        }

        // Exit if process is finished
        if (proc->finished) {
            pthread_mutex_unlock(&schedulerMutex);
            break;
        }

        // Record Start Time for Response Time calculation
        if (!proc->hasStarted) {
            proc->startTime = globalCurrentTime;
            proc->responseTime = proc->startTime - proc->arrivalTime;
            proc->hasStarted = 1;
        }

        // Set state to RUNNING
        proc->state = RUNNING;

        // Print process execution in table format
        printf("%-6d %-12s %-12s %-15d %-10lu\n", 
               globalCurrentTime, 
               (proc->pid == 1) ? "P1" : (proc->pid == 2) ? "P2" : 
               (proc->pid == 3) ? "P3" : (proc->pid == 4) ? "P4" :
               (proc->pid == 5) ? "P5" : (proc->pid == 6) ? "P6" :
               (proc->pid == 7) ? "P7" : (proc->pid == 8) ? "P8" :
               (proc->pid == 9) ? "P9" : "P10",
               getStateName(proc->state),
               proc->remainingTime,
               (unsigned long)pthread_self());

        // Decrement Remaining Time and Increment globalCurrentTime
        proc->remainingTime--;
        globalCurrentTime++;

        // Check if process has completed
        if (proc->remainingTime == 0) {
            proc->completionTime = globalCurrentTime;
            proc->turnaroundTime = proc->completionTime - proc->arrivalTime;
            proc->waitingTime = proc->turnaroundTime - proc->burstTime;
            proc->finished = 1;
            proc->state = COMPLETED;
            globalCompleted++;
            
            // Print completion status
            printf("%-6d %-12s %-12s %-15s %-10lu\n", 
                   globalCurrentTime,
                   (proc->pid == 1) ? "P1" : (proc->pid == 2) ? "P2" : 
                   (proc->pid == 3) ? "P3" : (proc->pid == 4) ? "P4" :
                   (proc->pid == 5) ? "P5" : (proc->pid == 6) ? "P6" :
                   (proc->pid == 7) ? "P7" : (proc->pid == 8) ? "P8" :
                   (proc->pid == 9) ? "P9" : "P10",
                   getStateName(proc->state),
                   "0",
                   (unsigned long)pthread_self());
        } else {
            // Set back to READY after execution
            proc->state = READY;
        }

        // Reset current process
        globalCurrentProcess = -1;

        pthread_mutex_unlock(&schedulerMutex);
    }

    return NULL;
}

// Update all process states based on current time and running process
void updateProcessStates(Process proc[], int n, int currentTime, int runningIdx) {
    for (int i = 0; i < n; i++) {
        if (proc[i].finished) {
            proc[i].state = COMPLETED;
        } else if (i == runningIdx) {
            proc[i].state = RUNNING;
        } else if (proc[i].arrivalTime <= currentTime) {
            proc[i].state = READY;
        }
    }
}

// Get string representation of process state
const char* getStateName(ProcessState state) {
    switch(state) {
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case COMPLETED: return "COMPLETED";
        default: return "UNKNOWN";
    }
}

// Sort processes by arrival time (improved bubble sort)
// Added swapped flag for optimisation
// Improved bubble sort runtime:
// Best case: O(n)
// Worst case: O(n^2)
// Source: Algorithms, Data Structures and Efficiency module
void sortByArrival(Process proc[], int n) {
    int i, j;
    bool swapped = true;

    for (i = 0; i < n - 1 && swapped; i++) {
        swapped = false;

        for (j = i + 1; j < n; j++) {
            if (proc[i].arrivalTime > proc[j].arrivalTime) {
                Process temp = proc[i];
                proc[i] = proc[j];
                proc[j] = temp;
                swapped = true;
            }
        }
    }
}

// Find process with shortest Remaining Time that has arrived
// If there exists such a process, its index is returned
// If no such process exists, -1 is returned
int findShortestJob(Process proc[], int n, int currentTime) {
    int shortest = -1;
    int minRemaining = __INT_MAX__;
    
    for (int i = 0; i < n; i++) {
        if (!proc[i].finished && 
            proc[i].arrivalTime <= currentTime && 
            proc[i].remainingTime < minRemaining) {
            minRemaining = proc[i].remainingTime;
            shortest = i;
        }
    }
    
    return shortest;
}

// Print scheduling results
void printResults(Process proc[], int n) {
    double totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;
    int i;

    printf("\n======================================\n");
    printf("  Scheduling Results\n");
    printf("======================================\n\n");

    // Print individual process metrics
    for (i = 0; i < n; i++) {
        printf("Process P%d: Turnaround = %d, Waiting = %d, Response = %d\n",
               proc[i].pid,
               proc[i].turnaroundTime,
               proc[i].waitingTime,
               proc[i].responseTime);
        
        totalTurnaround += proc[i].turnaroundTime;
        totalWaiting += proc[i].waitingTime;
        totalResponse += proc[i].responseTime;
    }

    // Print averages
    printf("\nAverage Turnaround Time = %.2f\n", totalTurnaround / n);
    printf("Average Waiting Time = %.2f\n", totalWaiting / n);
    printf("Average Response Time = %.2f\n", totalResponse / n);
}

// Print Gantt chart
void printGanttChart(GanttEntry gantt[], int size) {
    int i;
    
    printf("\n======================================\n");
    printf("  Gantt Chart\n");
    printf("======================================\n\n");

    // Print the top border of the bar Gantt chart
    printf(" ");
    for (i = 0; i < size; i++) {
        int duration = gantt[i].endTime - gantt[i].startTime;
        for (int j = 0; j < duration * 4; j++) {
            printf("-");
        }
    }
    printf("\n");

    // Print the process IDs in its respective time slots 
    // or print IDLE for the time slots where there are no processes executing
    printf("|");
    for (i = 0; i < size; i++) {
        int duration = gantt[i].endTime - gantt[i].startTime;
        int padding = duration * 4 - 3;
        int leftPad = padding / 2;
        int rightPad = padding - leftPad;
        
        for (int j = 0; j < leftPad; j++) printf(" ");
        if (gantt[i].pid == 0) {
            printf("IDLE");
        } else {
            printf("P%d", gantt[i].pid);
        }
        for (int j = 0; j < rightPad; j++) printf(" ");
        printf("|");
    }
    printf("\n");

    // Print the bottom border of the bar Gantt chart
    printf(" ");
    for (i = 0; i < size; i++) {
        int duration = gantt[i].endTime - gantt[i].startTime;
        for (int j = 0; j < duration * 4; j++) {
            printf("-");
        }
    }
    printf("\n");

    // Print the time markers below the Gantt chart
    printf("%d", gantt[0].startTime);
    for (i = 0; i < size; i++) {
        int duration = gantt[i].endTime - gantt[i].startTime;
        int numDigits = snprintf(NULL, 0, "%d", gantt[i].endTime);
        int spaces = duration * 4 - numDigits;
        for (int j = 0; j < spaces; j++) printf(" ");
        printf("%d", gantt[i].endTime);
    }
    printf("\n");
}