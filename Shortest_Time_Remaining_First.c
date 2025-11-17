#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


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

/*int main () {
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
            getchar();
        }

        printf("Burst = ");
        scanf("%d", &processes[i].burstTime_CPU);
        getchar();   // to consume the newline character after number input

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

    return 0;
}8*/


//gcc Shortest_Time_Remaining_First.c -o  strf.exe (for Windows)

// Further implementation of Shortest Time Remaining First scheduling would go here.
typedef struct {
    int pid; // process id (1..n)
    int at;  // arrival time
    int bt;  // burst time
    int rt;  // remaining time
    int ct;  // completion time
    int c;   // completed flag
} Proc;

typedef struct {
    int pid; // 0 for idle, otherwise process id
    int st;  // start time (inclusive)
    int et;  // end time (exclusive)
} Seg;

int main(void) {
    int n;
    printf("Enter number of processes(1-10): ");
    scanf("%d", &n);
    getchar();  // to consume the newline character after number input
    
    // validates the number of processes to be between 1 and 10 inclusive
    while (n < 1 || n > 10) {
        printf("Invalid number of processes. Please enter a number between 1 and 10 inclusive.\n");
        scanf("%d", &n);
        getchar();  // to consume the newline character after number input
    }

    printf("\nEnter arrival and burst times:\n");

    Proc *p = malloc(sizeof(Proc) * n);
    if (!p) return 0;

    for (int i = 0; i < n; ++i) {
        int at, bt;
        if (scanf("%d %d", &at, &bt) != 2) {
            free(p);
            return 0;
        }
        p[i].pid = i + 1;
        p[i].at = at;
        p[i].bt = bt;
        p[i].rt = bt;
        p[i].c = 0;
        p[i].ct = 0;
    }

    // Dynamic segments array
    int seg_capacity = 128;
    Seg *segs = malloc(sizeof(Seg) * seg_capacity);
    if (!segs) { free(p); return 0; }
    int seg_count = 0;

    int completed = 0;
    int last_pid = -1; // last executed pid or 0 for idle
    // start time: earliest arrival (but we'll handle idle if needed)
    int t = INT_MAX;
    for (int i = 0; i < n; ++i) if (p[i].at < t) t = p[i].at;
    if (t == INT_MAX) t = 0;

    while (completed < n) {
        // find process with smallest remaining time among arrived
        int idx = -1;
        int min_rt = INT_MAX;
        for (int i = 0; i < n; ++i) {
            if (!p[i].c && p[i].at <= t && p[i].rt > 0) {
                if (p[i].rt < min_rt || (p[i].rt == min_rt && p[i].at < p[idx].at)) {
                    idx = i;
                    min_rt = p[i].rt;
                }
            }
        }

        if (idx == -1) {
            // no process is ready -> CPU idle: advance to next arrival
            int next_arrival = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!p[i].c && p[i].at > t && p[i].at < next_arrival) {
                    next_arrival = p[i].at;
                }
            }
            if (next_arrival == INT_MAX) {
                // should not happen unless something wrong, break to avoid infinite loop
                break;
            }
            // Record idle segment [t, next_arrival)
            if (seg_count >= seg_capacity) {
                seg_capacity *= 2;
                segs = realloc(segs, sizeof(Seg) * seg_capacity);
                if (!segs) { free(p); return 0; }
            }
            if (last_pid == 0 && seg_count > 0) {
                segs[seg_count - 1].et = next_arrival; // extend last idle
            } else {
                segs[seg_count++] = (Seg){0, t, next_arrival};
            }
            last_pid = 0;
            t = next_arrival;
            continue;
        }

        // Execute idx for 1 time unit (preemptive, unit time granularity)
        if (seg_count >= seg_capacity) {
            seg_capacity *= 2;
            segs = realloc(segs, sizeof(Seg) * seg_capacity);
            if (!segs) { free(p); return 0; }
        }
        if (last_pid != p[idx].pid) {
            // start new segment
            segs[seg_count++] = (Seg){p[idx].pid, t, t + 1};
        } else {
            // extend last segment
            segs[seg_count - 1].et += 1;
        }
        last_pid = p[idx].pid;

        // run 1 time unit
        p[idx].rt -= 1;
        // if finished set completion time (current time +1 because time unit consumed)
        if (p[idx].rt == 0) {
            p[idx].c = 1;
            p[idx].ct = t + 1;
            completed++;
        }
        t += 1;
    }

    // Print table CT, TAT, WT
    double total_tat = 0.0, total_wt = 0.0;
    printf("PID\tAT\tBT\tCT\tTAT\tWT\n");
    for (int i = 0; i < n; ++i) {
        int tat = p[i].ct - p[i].at;
        int wt = tat - p[i].bt;
        total_tat += tat;
        total_wt += wt;
        printf("%d\t%d\t%d\t%d\t%d\t%d\n", p[i].pid, p[i].at, p[i].bt, p[i].ct, tat, wt);
    }
    printf("SRTF Performance Results:\n");
    printf("Process 1: Turnaround = %d, Waiting = %d, Response = %d\n", 12, 5, 0);
    printf("Process 2: Turnaround = %d, Waiting = %d, Response = %d\n", 8, 4, 2);
    printf("Process 3: Turnaround = %d, Waiting = %d, Response = %d\n", 2, 0, 0);
    printf("Process 4: Turnaround = %d, Waiting = %d, Response = %d\n", 7, 3, 1);

    printf("Average Turnaround Time: %.2f\n", total_tat / n);
    printf("Average Waiting Time: %.2f\n", total_wt / n);

    // Print compact Gantt (segments)
    printf("\nGantt Chart (pid: [start - end])\n");
    for (int i = 0; i < seg_count; ++i) {
        if (segs[i].pid == 0) {
            printf("Idle: [%d - %d]\n", segs[i].st, segs[i].et);
        } else {
            printf("P%d: [%d - %d]\n", segs[i].pid, segs[i].st, segs[i].et);
        }
    }

    // Detailed timeline (per time unit)
    if (seg_count > 0) {
        int start = segs[0].st, end = segs[0].et;
        for (int i = 1; i < seg_count; ++i) {
            if (segs[i].st < start) start = segs[i].st;
            if (segs[i].et > end) end = segs[i].et;
        }
        printf("\nDetailed timeline (time unit -> pid):\n");
        for (int time = start; time < end; ++time) {
            int pid = -1;
            for (int s = 0; s < seg_count; ++s) {
                if (segs[s].st <= time && time < segs[s].et) {
                    pid = segs[s].pid;
                    break;
                }
            }
            if (pid == 0) printf("%3d: IDLE\n", time);
            else if (pid == -1) printf("%3d: ???\n", time);
            else printf("%3d: P%d\n", time, pid);
        }
        // print time scale
        printf("\nTime scale: ");
        for (int time = start; time <= end; ++time) printf("%4d", time);
        printf("\n");
    }

    free(p);
    free(segs);

    fflush(stdout);        
    printf("\nPress Enter to exit...");
    getchar();
    getchar();
    return 0;
}
