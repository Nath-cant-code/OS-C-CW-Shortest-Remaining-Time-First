#include <stdio.h>

struct Process {
    char processID[5];
    int arrivalTime;
    int burstTime_CPU;
    int burstTimeRemaining;
    int completionTime;
};

int main () {
    int n = 0;
    printf("Enter number of processes(1-10): ");
    scanf("%d", &n);

    if (n < 1 || n > 10) {
        printf("Invalid number of processes. Please enter a number between 1 and 10.\n");
        return -1;
    }

    struct Process processes[n];

    printf("\nEnter arrival and burst times:\n");

    for (int i = 0; i < n; i++) {
        // sprintf(dest, "format string", values...) writes formatted output into a string
        sprintf(processes[i].processID, "P%d", i + 1);
        printf("\nProcess %d: Arrival = ", i + 1);
        scanf("%d", &processes[i].arrivalTime);
        getchar();
        printf("Burst = ");
        scanf("%d", &processes[i].burstTime_CPU);
        getchar();
    }

    printf("%-5s %-15s %-15s\n", "Time", "Process ID", "Burst Time");

    for (int i = 0; i < n; i++)
    {
        printf("%-5d %-15s %-15d\n", processes[i].arrivalTime, processes[i].processID, processes[i].burstTime_CPU);
    }
    

    // Further implementation of Shortest Job First scheduling would go here.
    

    return 0;
}