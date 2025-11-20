//Terminal code:
//gcc Shortest_Time_Remaining_First.c -o srtf -pthread
//.\srtf

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_PROC 10
#define MAX_TIMELINE 1000

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

// Global shared data
Process processes[MAX_PROC];
int globalCurrentTime = 0;
int globalCompleted = 0;
int globalCurrentProcess = -1;
bool schedulerRunning = true;
pthread_mutex_t schedulerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t schedulerCond = PTHREAD_COND_INITIALIZER;
GanttEntry gantt[MAX_TIMELINE];
int ganttSize = 0;
int numProcesses = 0;

// Function prototypes
void sortByArrival(Process proc[], int n);
int findShortestJob(Process proc[], int n, int currentTime);
void *processThread(void *arg);
void *schedulerThread(void *arg);
void printResults(Process proc[], int n);
void printGanttChart(GanttEntry gantt[], int size);

int main() {
    int n, i;

    // Input validation for number of processes
    printf("======================================\n");
    printf("  SRTF Process Scheduling Simulator\n");
    printf("  (Multithreaded Implementation)\n");
    printf("======================================\n\n");
    
    do {
        printf("Enter number of processes (1-%d): ", MAX_PROC);
        if (scanf("%d", &n) != 1) {
            while (getchar() != '\n');
            printf("Invalid input! Please enter a valid number.\n");
            n = 0;
            continue;
        }
        if (n < 1 || n > MAX_PROC) {
            printf("Invalid number! Must be between 1 and %d.\n", MAX_PROC);
        }
    } while (n < 1 || n > MAX_PROC);

    numProcesses = n;

    // Input arrival and burst times with validation
    printf("\nEnter arrival and burst times:\n");
    for (i = 0; i < n; i++) {
        processes[i].pid = i + 1;
        
        // Arrival time validation
        do {
            printf("Process %d - Arrival Time: ", i + 1);
            if (scanf("%d", &processes[i].arrivalTime) != 1) {
                while (getchar() != '\n');
                printf("Invalid input! Please enter a valid integer.\n");
                processes[i].arrivalTime = -1;
                continue;
            }
            if (processes[i].arrivalTime < 0) {
                printf("Arrival time cannot be negative!\n");
            }
        } while (processes[i].arrivalTime < 0);

        // Burst time validation
        do {
            printf("Process %d - Burst Time:   ", i + 1);
            if (scanf("%d", &processes[i].burstTime) != 1) {
                while (getchar() != '\n');
                printf("Invalid input! Please enter a valid integer.\n");
                processes[i].burstTime = 0;
                continue;
            }
            if (processes[i].burstTime < 1) {
                printf("Burst time must be at least 1!\n");
            }
        } while (processes[i].burstTime < 1);

        // Initialize process fields
        processes[i].remainingTime = processes[i].burstTime;
        processes[i].startTime = -1;
        processes[i].completionTime = 0;
        processes[i].turnaroundTime = 0;
        processes[i].waitingTime = 0;
        processes[i].responseTime = 0;
        processes[i].finished = 0;
        processes[i].hasStarted = 0;
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

    // Create scheduler thread
    pthread_t scheduler;
    if (pthread_create(&scheduler, NULL, schedulerThread, NULL) != 0) {
        fprintf(stderr, "Error creating scheduler thread\n");
        return 1;
    }

    // Create process threads
    for (i = 0; i < n; i++) {
        if (pthread_create(&processes[i].thread, NULL, processThread, &processes[i]) != 0) {
            fprintf(stderr, "Error creating process thread %d\n", i + 1);
            return 1;
        }
    }

    // Wait for scheduler to finish
    pthread_join(scheduler, NULL);

    // Wait for all process threads to finish
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

// Scheduler thread function - coordinates process execution
void *schedulerThread(void *arg) {
    int lastProcess = -1;

    // Jump to first arrival if needed
    if (numProcesses > 0 && processes[0].arrivalTime > 0) {
        pthread_mutex_lock(&schedulerMutex);
        globalCurrentTime = processes[0].arrivalTime;
        pthread_mutex_unlock(&schedulerMutex);
    }

    while (schedulerRunning) {
        pthread_mutex_lock(&schedulerMutex);

        // Check if all processes completed
        if (globalCompleted >= numProcesses) {
            schedulerRunning = false;
            pthread_cond_broadcast(&schedulerCond);
            pthread_mutex_unlock(&schedulerMutex);
            break;
        }

        // Find process with shortest remaining time
        int idx = findShortestJob(processes, numProcesses, globalCurrentTime);

        // If no process available, jump to next arrival
        if (idx == -1) {
            int nextArrival = __INT_MAX__;
            for (int i = 0; i < numProcesses; i++) {
                if (!processes[i].finished && processes[i].arrivalTime > globalCurrentTime) {
                    if (processes[i].arrivalTime < nextArrival) {
                        nextArrival = processes[i].arrivalTime;
                    }
                }
            }
            
            if (nextArrival != __INT_MAX__) {
                // Add idle time to Gantt chart
                if (ganttSize < MAX_TIMELINE) {
                    gantt[ganttSize].pid = 0;
                    gantt[ganttSize].startTime = globalCurrentTime;
                    gantt[ganttSize].endTime = nextArrival;
                    ganttSize++;
                }
                printf("Time %d-%d: CPU IDLE\n", globalCurrentTime, nextArrival);
                globalCurrentTime = nextArrival;
            }
            pthread_mutex_unlock(&schedulerMutex);
            continue;
        }

        // Check for context switch (preemption)
        if (lastProcess != -1 && lastProcess != processes[idx].pid) {
            printf("Time %d: **PREEMPTION** - Switching from P%d to P%d\n", 
                   globalCurrentTime, lastProcess, processes[idx].pid);
        }

        // Set current process and signal it to execute
        globalCurrentProcess = idx;
        
        // Add to Gantt chart
        if (lastProcess == processes[idx].pid && ganttSize > 0) {
            gantt[ganttSize - 1].endTime = globalCurrentTime + 1;
        } else {
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

        // Small delay to simulate time slice execution
        usleep(100000); // 100ms delay
    }

    return NULL;
}

// Process thread function - represents individual process execution
void *processThread(void *arg) {
    Process *proc = (Process *)arg;

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

        // Check if process is finished
        if (proc->finished) {
            pthread_mutex_unlock(&schedulerMutex);
            break;
        }

        // Record start time for response time calculation
        if (!proc->hasStarted) {
            proc->startTime = globalCurrentTime;
            proc->responseTime = proc->startTime - proc->arrivalTime;
            proc->hasStarted = 1;
        }

        // Execute for 1 time unit
        printf("Time %d: Process P%d executing (Remaining: %d) [Thread ID: %lu]\n", 
               globalCurrentTime, proc->pid, proc->remainingTime, 
               (unsigned long)pthread_self());

        proc->remainingTime--;
        globalCurrentTime++;

        // Check if process completed
        if (proc->remainingTime == 0) {
            proc->completionTime = globalCurrentTime;
            proc->turnaroundTime = proc->completionTime - proc->arrivalTime;
            proc->waitingTime = proc->turnaroundTime - proc->burstTime;
            proc->finished = 1;
            globalCompleted++;
            printf("Time %d: Process P%d completed [Thread ID: %lu]\n", 
                   globalCurrentTime, proc->pid, (unsigned long)pthread_self());
        }

        // Reset current process
        globalCurrentProcess = -1;

        pthread_mutex_unlock(&schedulerMutex);
    }

    return NULL;
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

// Find process with shortest remaining time that has arrived
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

    // Top border
    printf(" ");
    for (i = 0; i < size; i++) {
        int duration = gantt[i].endTime - gantt[i].startTime;
        for (int j = 0; j < duration * 4; j++) {
            printf("-");
        }
    }
    printf("\n");

    // Process names
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

    // Bottom border
    printf(" ");
    for (i = 0; i < size; i++) {
        int duration = gantt[i].endTime - gantt[i].startTime;
        for (int j = 0; j < duration * 4; j++) {
            printf("-");
        }
    }
    printf("\n");

    // Timeline
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