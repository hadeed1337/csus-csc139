/*
CSC139
Summer 2024
Third Assignment
Francis, Jacob
Section 1
OSs Tested on: MAC
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int process_number;
    int arrival_time;
    int cpu_burst;
    int priority;
} Process;

// Function to read input from the file
void read_input(const char *filename, char *algorithm, int *num_processes, Process **processes)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Unable to open input file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%s", algorithm);
    fscanf(file, "%d", num_processes);

    *processes = (Process *)malloc((*num_processes) * sizeof(Process));
    for (int i = 0; i < *num_processes; ++i)
    {
        fscanf(file, "%d %d %d %d", &(*processes)[i].process_number, &(*processes)[i].arrival_time,
               &(*processes)[i].cpu_burst, &(*processes)[i].priority);
    }

    fclose(file);
}

// Function to write output to the file and print to the console
void write_output(const char *filename, const char *algorithm, int **schedule, int schedule_size, float avg_waiting_time)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        perror("Unable to open output file");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", algorithm);
    fprintf(file, "%s\n", algorithm);
    for (int i = 0; i < schedule_size; ++i)
    {
        printf("%d %d\n", schedule[i][0], schedule[i][1]);
        fprintf(file, "%d %d\n", schedule[i][0], schedule[i][1]);
    }
    printf("AVG Waiting Time: %.2f\n", avg_waiting_time);
    fprintf(file, "AVG Waiting Time: %.2f\n", avg_waiting_time);

    fclose(file);
}

// Function to simulate Round Robin scheduling
void simulate_round_robin(Process *processes, int num_processes, int time_quantum, int ***schedule, int *schedule_size, float *avg_waiting_time)
{
    int capacity = 100;
    *schedule = (int **)malloc(capacity * sizeof(int *));
    for (int i = 0; i < capacity; ++i)
    {
        (*schedule)[i] = (int *)malloc(2 * sizeof(int));
    }

    int time = 0;
    int index = 0;
    int *queue = (int *)malloc(num_processes * sizeof(int));
    int front = 0;
    int rear = 0;
    int remaining_time[num_processes];
    int waiting_time[num_processes];
    int finished_processes = 0;

    for (int i = 0; i < num_processes; ++i)
    {
        remaining_time[i] = processes[i].cpu_burst;
        waiting_time[i] = 0;
    }

    int current_process = -1;

    while (finished_processes < num_processes)
    {
        for (int i = 0; i < num_processes; ++i)
        {
            if (processes[i].arrival_time == time)
            {
                queue[rear++] = i;
            }
        }

        if (front < rear)
        {
            current_process = queue[front++];
            if (remaining_time[current_process] > time_quantum)
            {
                remaining_time[current_process] -= time_quantum;
                time += time_quantum;
                queue[rear++] = current_process;
            }
            else
            {
                time += remaining_time[current_process];
                remaining_time[current_process] = 0;
                finished_processes++;
            }

            (*schedule)[index][0] = time;
            (*schedule)[index][1] = processes[current_process].process_number;
            index++;

            if (index >= capacity)
            {
                capacity *= 2;
                *schedule = (int **)realloc(*schedule, capacity * sizeof(int *));
                for (int i = index; i < capacity; ++i)
                {
                    (*schedule)[i] = (int *)malloc(2 * sizeof(int));
                }
            }
        }
        else
        {
            time++;
        }

        for (int i = front; i < rear; ++i)
        {
            if (queue[i] != current_process)
            {
                waiting_time[queue[i]]++;
            }
        }
    }

    *schedule_size = index;

    int total_waiting_time = 0;
    for (int i = 0; i < num_processes; ++i)
    {
        total_waiting_time += waiting_time[i];
    }
    *avg_waiting_time = (float)total_waiting_time / num_processes;

    free(queue);
}

// Function to simulate Shortest Job First scheduling
void simulate_sjf(Process *processes, int num_processes, int ***schedule, int *schedule_size, float *avg_waiting_time)
{
    int capacity = 100;
    *schedule = (int **)malloc(capacity * sizeof(int *));
    for (int i = 0; i < capacity; ++i)
    {
        (*schedule)[i] = (int *)malloc(2 * sizeof(int));
    }

    int time = 0;
    int index = 0;
    int remaining_processes = num_processes;
    int waiting_time[num_processes];
    int finished[num_processes];

    for (int i = 0; i < num_processes; ++i)
    {
        waiting_time[i] = 0;
        finished[i] = 0;
    }

    while (remaining_processes > 0)
    {
        int min_burst = __INT_MAX__;
        int shortest = -1;

        for (int i = 0; i < num_processes; ++i)
        {
            if (!finished[i] && processes[i].arrival_time <= time && processes[i].cpu_burst < min_burst)
            {
                min_burst = processes[i].cpu_burst;
                shortest = i;
            }
        }

        if (shortest != -1)
        {
            time += processes[shortest].cpu_burst;
            finished[shortest] = 1;
            remaining_processes--;

            (*schedule)[index][0] = time;
            (*schedule)[index][1] = processes[shortest].process_number;
            index++;

            if (index >= capacity)
            {
                capacity *= 2;
                *schedule = (int **)realloc(*schedule, capacity * sizeof(int *));
                for (int i = index; i < capacity; ++i)
                {
                    (*schedule)[i] = (int *)malloc(2 * sizeof(int));
                }
            }

            for (int i = 0; i < num_processes; ++i)
            {
                if (!finished[i] && processes[i].arrival_time <= time)
                {
                    waiting_time[i] += processes[shortest].cpu_burst;
                }
            }
        }
        else
        {
            time++;
        }
    }

    *schedule_size = index;

    int total_waiting_time = 0;
    for (int i = 0; i < num_processes; ++i)
    {
        total_waiting_time += waiting_time[i] - processes[i].cpu_burst;
    }
    *avg_waiting_time = (float)total_waiting_time / num_processes;
}

// Function to simulate Priority Scheduling without Preemption
void simulate_priority_no_preemption(Process *processes, int num_processes, int ***schedule, int *schedule_size, float *avg_waiting_time)
{
    int capacity = 100;
    *schedule = (int **)malloc(capacity * sizeof(int *));
    for (int i = 0; i < capacity; ++i)
    {
        (*schedule)[i] = (int *)malloc(2 * sizeof(int));
    }

    int time = 0;
    int index = 0;
    int remaining_processes = num_processes;
    int waiting_time[num_processes];
    int finished[num_processes];

    for (int i = 0; i < num_processes; ++i)
    {
        waiting_time[i] = 0;
        finished[i] = 0;
    }

    while (remaining_processes > 0)
    {
        int highest_priority = __INT_MAX__;
        int highest = -1;

        for (int i = 0; i < num_processes; ++i)
        {
            if (!finished[i] && processes[i].arrival_time <= time && processes[i].priority < highest_priority)
            {
                highest_priority = processes[i].priority;
                highest = i;
            }
        }

        if (highest != -1)
        {
            time += processes[highest].cpu_burst;
            finished[highest] = 1;
            remaining_processes--;

            (*schedule)[index][0] = time;
            (*schedule)[index][1] = processes[highest].process_number;
            index++;

            if (index >= capacity)
            {
                capacity *= 2;
                *schedule = (int **)realloc(*schedule, capacity * sizeof(int *));
                for (int i = index; i < capacity; ++i)
                {
                    (*schedule)[i] = (int *)malloc(2 * sizeof(int));
                }
            }

            for (int i = 0; i < num_processes; ++i)
            {
                if (!finished[i] && processes[i].arrival_time <= time)
                {
                    waiting_time[i] += processes[highest].cpu_burst;
                }
            }
        }
        else
        {
            time++;
        }
    }

    *schedule_size = index;

    int total_waiting_time = 0;
    for (int i = 0; i < num_processes; ++i)
    {
        total_waiting_time += waiting_time[i] - processes[i].cpu_burst;
    }
    *avg_waiting_time = (float)total_waiting_time / num_processes;
}

// Function to simulate Priority Scheduling with Preemption
void simulate_priority_with_preemption(Process *processes, int num_processes, int ***schedule, int *schedule_size, float *avg_waiting_time)
{
    int capacity = 100;
    *schedule = (int **)malloc(capacity * sizeof(int *));
    for (int i = 0; i < capacity; ++i)
    {
        (*schedule)[i] = (int *)malloc(2 * sizeof(int));
    }

    int time = 0;
    int index = 0;
    int *queue = (int *)malloc(num_processes * sizeof(int));
    int front = 0;
    int rear = 0;
    int remaining_time[num_processes];
    int waiting_time[num_processes];
    int finished[num_processes];
    int highest_priority = __INT_MAX__;
    int current_process = -1;

    for (int i = 0; i < num_processes; ++i)
    {
        remaining_time[i] = processes[i].cpu_burst;
        waiting_time[i] = 0;
        finished[i] = 0;
    }

    while (1)
    {
        int all_finished = 1;
        for (int i = 0; i < num_processes; ++i)
        {
            if (!finished[i])
            {
                all_finished = 0;
                break;
            }
        }
        if (all_finished)
        {
            break;
        }

        for (int i = 0; i < num_processes; ++i)
        {
            if (processes[i].arrival_time == time)
            {
                queue[rear++] = i;
            }
        }

        if (current_process == -1 || remaining_time[current_process] == 0)
        {
            highest_priority = __INT_MAX__;
            current_process = -1;
            for (int i = front; i < rear; ++i)
            {
                int p = queue[i];
                if (!finished[p] && processes[p].priority < highest_priority)
                {
                    highest_priority = processes[p].priority;
                    current_process = p;
                }
            }
        }

        if (current_process != -1)
        {
            (*schedule)[index][0] = time;
            (*schedule)[index][1] = processes[current_process].process_number;
            index++;

            remaining_time[current_process]--;
            if (remaining_time[current_process] == 0)
            {
                finished[current_process] = 1;
                for (int i = front; i < rear; ++i)
                {
                    if (queue[i] == current_process)
                    {
                        for (int j = i; j < rear - 1; ++j)
                        {
                            queue[j] = queue[j + 1];
                        }
                        rear--;
                        break;
                    }
                }
            }
        }

        time++;

        for (int i = front; i < rear; ++i)
        {
            if (queue[i] != current_process)
            {
                waiting_time[queue[i]]++;
            }
        }
    }

    *schedule_size = index;

    int total_waiting_time = 0;
    for (int i = 0; i < num_processes; ++i)
    {
        total_waiting_time += waiting_time[i];
    }
    *avg_waiting_time = (float)total_waiting_time / num_processes;

    free(queue);
}

int main()
{
    char algorithm[20];
    int num_processes;
    Process *processes;

    read_input("input.txt", algorithm, &num_processes, &processes);

    int **schedule;
    int schedule_size;
    float avg_waiting_time;

    // Print input data
    printf("Algorithm: %s\n", algorithm);
    printf("Number of Processes: %d\n", num_processes);
    for (int i = 0; i < num_processes; ++i)
    {
        printf("Process %d: Arrival Time = %d, CPU Burst = %d, Priority = %d\n",
               processes[i].process_number, processes[i].arrival_time,
               processes[i].cpu_burst, processes[i].priority);
    }

    if (strncmp(algorithm, "RR", 2) == 0)
    {
        int time_quantum = atoi(&algorithm[3]);
        simulate_round_robin(processes, num_processes, time_quantum, &schedule, &schedule_size, &avg_waiting_time);
    }
    else if (strcmp(algorithm, "SJF") == 0)
    {
        simulate_sjf(processes, num_processes, &schedule, &schedule_size, &avg_waiting_time);
    }
    else if (strcmp(algorithm, "PR_noPREMP") == 0)
    {
        simulate_priority_no_preemption(processes, num_processes, &schedule, &schedule_size, &avg_waiting_time);
    }
    else if (strcmp(algorithm, "PR_withPREMP") == 0)
    {
        simulate_priority_with_preemption(processes, num_processes, &schedule, &schedule_size, &avg_waiting_time);
    }

    write_output("output.txt", algorithm, schedule, schedule_size, avg_waiting_time);

    // Free allocated memory
    for (int i = 0; i < schedule_size; ++i)
    {
        free(schedule[i]);
    }
    free(schedule);
    free(processes);

    return 0;
}
