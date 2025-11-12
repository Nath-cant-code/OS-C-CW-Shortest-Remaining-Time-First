#include <stdio.h>

struct Process {
    char processID[5];
    int arrivalTime;
    int burstTime_CPU;
    int burstTimeRemaining;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
    char state[10];
    // state can be int, e.g. 1 for "ready", 2 for "running", etc.
    // the mapping can be defined elsewhere in the code, most probably in output section
};

int main () {
    int n = 0;
    printf("Enter number of processes(1-10): ");
    scanf("%d", &n);
    getchar();  // to consume the newline character after number input
    
    // validates the number of processes to be between 1 and 10 inclusive
    while (n < 1 || n > 10) {
        printf("Invalid number of processes. Please enter a number between 1 and 10 inclusive.\n");
        scanf("%d", &n);
        getchar();  // to consume the newline character after number input
    }

    struct Process processes[n];

    printf("\nEnter arrival and burst times:\n");

    // for loop for user input of arrival and burst times
    for (int i = 0; i < n; i++) {
        // sprintf(dest, "format string", values...) writes formatted output into a string
        // gives each process their own unique ID
        sprintf(processes[i].processID, "P%d", i + 1);

        printf("\nProcess %d: Arrival = ", i + 1);
        scanf("%d", &processes[i].arrivalTime);
        getchar();

        // validates the arrival time input
        while (processes[i].arrivalTime < 0) {
            printf("\nArrival time cannot be negative. Please enter a valid arrival time: ");
            scanf("%d", &processes[i].arrivalTime);
            getchar();   // to consume the newline character after number input
        }

        printf("Burst = ");
        scanf("%d", &processes[i].burstTime_CPU);
        getchar();

        // validates the burst time input
        while (processes[i].burstTime_CPU < 1)
        {
            printf("\nBurst time must be at least 1ms. Please enter a valid arrival time: ");
            scanf("%d", &processes[i].burstTime_CPU);
            getchar();   // to consume the newline character after number input
        }
        
    }

    // prints a table of the entered processes with their arrival and burst times
    printf("%-5s %-15s %-15s\n", "Time", "Process ID", "Burst Time");

    for (int i = 0; i < n; i++)
    {
        printf("%-5d %-15s %-15d\n", processes[i].arrivalTime, processes[i].processID, processes[i].burstTime_CPU);
    }
    

    // Further implementation of Shortest Job First scheduling would go here.
    

    return 0;
}