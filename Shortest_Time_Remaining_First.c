//Terminal code:
//gcc Shortest_Time_Remaining_First.c -o srtf -pthread
//.\srtf

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

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
    GanttEntry *gantt;
    int *ganttSize;
    pthread_mutex_t *mutex;
} ThreadData;

// Global variables
GanttEntry gantt[MAX_TIMELINE];
int ganttSize = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void validateInput(int *value, int min, int max, const char *prompt);
void sortByArrival(Process proc[], int n);
int findShortestJob(Process proc[], int n, int currentTime);
void *executeProcess(void *arg);
void calculateMetrics(Process proc[], int n);
void printResults(Process proc[], int n);
void printGanttChart(GanttEntry gantt[], int size);

int main() {
    int n, i;
    Process proc[MAX_PROC];

    // Input validation for number of processes
    printf("======================================\n");
    printf("  SRTF Process Scheduling Simulator\n");
    printf("======================================\n\n");
    
    do {
        printf("Enter number of processes (1-%d): ", MAX_PROC);
        if (scanf("%d", &n) != 1) {
            while (getchar() != '\n'); // Clear input buffer
            printf("Invalid input! Please enter a valid number.\n");
            n = 0;
            continue;
        }
        if (n < 1 || n > MAX_PROC) {
            printf("Invalid number! Must be between 1 and %d.\n", MAX_PROC);
        }
    } while (n < 1 || n > MAX_PROC);

    // Input arrival and burst times with validation
    printf("\nEnter arrival and burst times:\n");
    for (i = 0; i < n; i++) {
        proc[i].pid = i + 1;
        
        // Arrival time validation
        do {
            printf("Process %d - Arrival Time: ", i + 1);
            if (scanf("%d", &proc[i].arrivalTime) != 1) {
                while (getchar() != '\n');
                printf("Invalid input! Please enter a valid integer.\n");
                proc[i].arrivalTime = -1;
                continue;
            }
            if (proc[i].arrivalTime < 0) {
                printf("Arrival time cannot be negative!\n");
            }
        } while (proc[i].arrivalTime < 0);

        // Burst time validation
        do {
            printf("Process %d - Burst Time:   ", i + 1);
            if (scanf("%d", &proc[i].burstTime) != 1) {
                while (getchar() != '\n');
                printf("Invalid input! Please enter a valid integer.\n");
                proc[i].burstTime = 0;
                continue;
            }
            if (proc[i].burstTime < 1) {
                printf("Burst time must be at least 1!\n");
            }
        } while (proc[i].burstTime < 1);

        // Initialize process fields
        proc[i].remainingTime = proc[i].burstTime;
        proc[i].startTime = -1;
        proc[i].completionTime = 0;
        proc[i].turnaroundTime = 0;
        proc[i].waitingTime = 0;
        proc[i].responseTime = 0;
        proc[i].finished = 0;
        proc[i].hasStarted = 0;
    }

    // Sort processes by arrival time
    sortByArrival(proc, n);

    // SRTF Scheduling with multithreading simulation
    printf("\n======================================\n");
    printf("  Execution Timeline (PREEMPTIVE)\n");
    printf("======================================\n");
    printf("Note: SRTF allows preemption - processes can be interrupted\n");
    printf("      when a shorter job arrives.\n\n");

    int currentTime = 0;
    int completed = 0;
    int lastProcess = -1;
    int sliceStart = 0;

    // Jump to first arrival if needed
    if (n > 0 && proc[0].arrivalTime > 0) {
        currentTime = proc[0].arrivalTime;
    }

    while (completed < n) {
        // Find process with shortest remaining time
        int idx = findShortestJob(proc, n, currentTime);

        // If no process available, jump to next arrival
        if (idx == -1) {
            int nextArrival = __INT_MAX__;
            for (i = 0; i < n; i++) {
                if (!proc[i].finished && proc[i].arrivalTime > currentTime) {
                    if (proc[i].arrivalTime < nextArrival) {
                        nextArrival = proc[i].arrivalTime;
                    }
                }
            }
            
            // Add idle time to Gantt chart
            if (nextArrival != __INT_MAX__ && nextArrival > currentTime) {
                if (ganttSize < MAX_TIMELINE) {
                    gantt[ganttSize].pid = 0; // 0 represents idle
                    gantt[ganttSize].startTime = currentTime;
                    gantt[ganttSize].endTime = nextArrival;
                    ganttSize++;
                }
                currentTime = nextArrival;
            }
            continue;
        }

        // Record start time (response time calculation)
        if (!proc[idx].hasStarted) {
            proc[idx].startTime = currentTime;
            proc[idx].responseTime = proc[idx].startTime - proc[idx].arrivalTime;
            proc[idx].hasStarted = 1;
        }

        // Execute process for 1 time unit (simulating preemption)
        
        // Check for context switch (preemption indicator)
        if (lastProcess != -1 && lastProcess != proc[idx].pid) {
            printf("Time %d: **PREEMPTION** - Switching from P%d to P%d\n", 
                   currentTime, lastProcess, proc[idx].pid);
        }
        
        printf("Time %d: Process P%d executing (Remaining: %d)\n", 
               currentTime, proc[idx].pid, proc[idx].remainingTime);
        
        // Add to Gantt chart (combine consecutive executions)
        if (lastProcess == proc[idx].pid) {
            // Extend current Gantt entry
            if (ganttSize > 0) {
                gantt[ganttSize - 1].endTime = currentTime + 1;
            }
        } else {
            // New Gantt entry
            if (ganttSize < MAX_TIMELINE) {
                gantt[ganttSize].pid = proc[idx].pid;
                gantt[ganttSize].startTime = currentTime;
                gantt[ganttSize].endTime = currentTime + 1;
                ganttSize++;
            }
            lastProcess = proc[idx].pid;
        }

        proc[idx].remainingTime--;
        currentTime++;

        // Check if process completed
        if (proc[idx].remainingTime == 0) {
            proc[idx].completionTime = currentTime;
            proc[idx].turnaroundTime = proc[idx].completionTime - proc[idx].arrivalTime;
            proc[idx].waitingTime = proc[idx].turnaroundTime - proc[idx].burstTime;
            proc[idx].finished = 1;
            completed++;
            printf("Time %d: Process P%d completed\n", currentTime, proc[idx].pid);
        }
    }

    // Display results
    printResults(proc, n);
    
    // Display Gantt chart
    printGanttChart(gantt, ganttSize);

    return 0;
}

// Sort processes by arrival time (stable sort)
void sortByArrival(Process proc[], int n) {
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = i + 1; j < n; j++) {
            if (proc[i].arrivalTime > proc[j].arrivalTime) {
                Process temp = proc[i];
                proc[i] = proc[j];
                proc[j] = temp;
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