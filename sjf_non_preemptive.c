
//Terminal code:
//gcc sjf_non_preemptive.c -o sjf
//.\sjf

#include <stdio.h>
#include <string.h>

#define MAX_PROC 10   // maximum allowed processes

// Structure representing each process
typedef struct {
    char pid[6];           // Process ID (P1, P2, ...)
    int arrivalTime;       // Time when the process arrives
    int burstTime;         // CPU burst duration
    int startTime;         // Time when the process starts execution
    int completionTime;    // Time when the process finishes
    int turnaroundTime;    // completionTime - arrivalTime
    int waitingTime;       // turnaroundTime - burstTime
    int responseTime;      // startTime - arrivalTime
    int finished;          // 0 = not completed, 1 = completed
} Process;

int main() {
    int n;
    Process proc[MAX_PROC];
    int i, j;

    // ---------------------------
    // Input number of processes
    // ---------------------------
    printf("Enter number of processes (1-%d): ", MAX_PROC);
    if (scanf("%d", &n) != 1) return 0;

    while (n < 1 || n > MAX_PROC) {
        printf("Invalid number. Enter between 1 and %d: ", MAX_PROC);
        if (scanf("%d", &n) != 1) return 0;
    }

    // ---------------------------
    // Read arrival & burst times
    // ---------------------------
    printf("\nEnter arrival and burst times:\n");
    for (i = 0; i < n; i++) {

        // Assign PID automatically
        snprintf(proc[i].pid, sizeof(proc[i].pid), "P%d", i+1);

        // Arrival time
        printf("Process %d: Arrival = ", i+1);
        while (scanf("%d", &proc[i].arrivalTime) != 1) {
            while (getchar() != '\n');
            printf("Invalid. Enter integer for arrival: ");
        }
        while (proc[i].arrivalTime < 0) {
            printf("Arrival time cannot be negative. Enter again: ");
            scanf("%d", &proc[i].arrivalTime);
        }

        // Burst time
        printf("         Burst   = ");
        while (scanf("%d", &proc[i].burstTime) != 1) {
            while (getchar() != '\n');
            printf("Invalid. Enter integer for burst: ");
        }
        while (proc[i].burstTime < 1) {
            printf("Burst time must be at least 1. Enter again: ");
            scanf("%d", &proc[i].burstTime);
        }

        // Initialize scheduling-related values
        proc[i].startTime = -1;
        proc[i].completionTime = 0;
        proc[i].turnaroundTime = 0;
        proc[i].waitingTime = 0;
        proc[i].responseTime = -1;
        proc[i].finished = 0;
    }

    // -------------------------------------
    // Sort processes by arrival time (stable)
    // -------------------------------------
    for (i = 0; i < n - 1; i++) {
        for (j = i + 1; j < n; j++) {
            if (proc[i].arrivalTime > proc[j].arrivalTime) {
                Process tmp = proc[i];
                proc[i] = proc[j];
                proc[j] = tmp;
            }
        }
    }

    // -------------------------------
    // SJF (Non-preemptive) Simulation
    // -------------------------------
    int time = 0;        // current CPU time
    int completed = 0;   // number of completed processes
    int idx;             // index of selected shortest job

    // If first process arrives later, jump time forward
    if (n > 0) time = proc[0].arrivalTime;

    while (completed < n) {

        idx = -1;

        // Select process with smallest burst time among arrived & unfinished
        for (i = 0; i < n; i++) {
            if (!proc[i].finished && proc[i].arrivalTime <= time) {
                if (idx == -1 || proc[i].burstTime < proc[idx].burstTime) {
                    idx = i;
                }
            }
        }

        // If no process has arrived yet, jump to next arrival time
        if (idx == -1) {
            int nextArrival = 1e9;
            for (i = 0; i < n; i++) {
                if (!proc[i].finished && proc[i].arrivalTime < nextArrival) {
                    nextArrival = proc[i].arrivalTime;
                }
            }
            time = nextArrival;
            continue;
        }

        // Set the start time for the selected process
        proc[idx].startTime = time;
        proc[idx].responseTime = proc[idx].startTime - proc[idx].arrivalTime;

        // Run the process to completion (non-preemptive)
        time += proc[idx].burstTime;
        proc[idx].completionTime = time;

        // Calculate metrics
        proc[idx].turnaroundTime = proc[idx].completionTime - proc[idx].arrivalTime;
        proc[idx].waitingTime = proc[idx].turnaroundTime - proc[idx].burstTime;
        proc[idx].finished = 1;

        completed++;
    }

    // ---------------------
    // Print results table
    // ---------------------
    double totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;

    printf("\n%-8s %-12s %-12s %-12s %-12s %-12s\n",
           "Process", "Arrival", "Burst", "Start", "Completion", "Turnaround");

    for (i = 0; i < n; i++) {
        printf("%-8s %-12d %-12d %-12d %-12d %-12d\n",
               proc[i].pid,
               proc[i].arrivalTime,
               proc[i].burstTime,
               proc[i].startTime,
               proc[i].completionTime,
               proc[i].turnaroundTime);

        totalTurnaround += proc[i].turnaroundTime;
        totalWaiting += proc[i].waitingTime;
        totalResponse += proc[i].responseTime;
    }

    // Print waiting & response times
    printf("\n%-8s %-12s %-12s\n", "Process", "Waiting", "Response");
    for (i = 0; i < n; i++) {
        printf("%-8s %-12d %-12d\n",
               proc[i].pid,
               proc[i].waitingTime,
               proc[i].responseTime);
    }

    // Print averages
    printf("\nAverage Turnaround Time = %.2f\n", totalTurnaround / n);
    printf("Average Waiting Time    = %.2f\n", totalWaiting / n);
    printf("Average Response Time   = %.2f\n", totalResponse / n);

    return 0;
}
