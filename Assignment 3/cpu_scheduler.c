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

#define MAX_PROCESSES 100

typedef struct
{
    int process_number;
    int arrival_time;
    int burst_time;
    int priority;
    int remaining_time;
    int waiting_time;
    int completed_time;
} Process;

void read_input(const char *filename, char *algorithm, int *num_processes, Process processes[], int *time_quantum)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open file");
        exit(1);
    }

    fscanf(file, "%s", algorithm);
    if (strcmp(algorithm, "RR") == 0)
    {
        fscanf(file, "%d", time_quantum);
    }
    fscanf(file, "%d", num_processes);

    for (int i = 0; i < *num_processes; i++)
    {
        fscanf(file, "%d %d %d %d",
               &processes[i].process_number,
               &processes[i].arrival_time,
               &processes[i].burst_time,
               &processes[i].priority);
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].waiting_time = 0;
        processes[i].completed_time = 0;
    }

    fclose(file);
}

void write_output(const char *filename, const char *algorithm, int time_points[], int process_schedule[], int schedule_length, double avg_waiting_time)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        perror("Failed to open file");
        exit(1);
    }

    fprintf(file, "%s\n", algorithm);
    for (int i = 0; i < schedule_length; i++)
    {
        fprintf(file, "%d %d\n", time_points[i], process_schedule[i]);
    }
    fprintf(file, "AVG Waiting Time: %.2f\n", avg_waiting_time);

    fclose(file);
}

void print_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open file");
        exit(1);
    }

    char ch;
    while ((ch = fgetc(file)) != EOF)
    {
        putchar(ch);
    }

    fclose(file);
}

void round_robin(Process processes[], int num_processes, int time_quantum, int *time_points, int *process_schedule, int *schedule_length, double *avg_waiting_time)
{
    int time = 0;
    int queue[MAX_PROCESSES];
    int front = 0, rear = 0;
    int completed = 0;
    int total_waiting_time = 0;

    for (int i = 0; i < num_processes; i++)
    {
        if (processes[i].arrival_time == 0)
        {
            queue[rear++] = i;
        }
    }

    while (completed < num_processes)
    {
        if (front == rear)
        {
            time++;
            for (int i = 0; i < num_processes; i++)
            {
                if (processes[i].arrival_time == time)
                {
                    queue[rear++] = i;
                }
            }
            continue;
        }

        int current = queue[front++];
        time_points[*schedule_length] = time;
        process_schedule[*schedule_length] = processes[current].process_number;
        (*schedule_length)++;

        if (processes[current].remaining_time > time_quantum)
        {
            processes[current].remaining_time -= time_quantum;
            time += time_quantum;
        }
        else
        {
            time += processes[current].remaining_time;
            processes[current].remaining_time = 0;
            processes[current].waiting_time = time - processes[current].arrival_time - processes[current].burst_time;
            total_waiting_time += processes[current].waiting_time;
            completed++;
        }

        for (int i = 0; i < num_processes; i++)
        {
            if (processes[i].arrival_time > time - time_quantum && processes[i].arrival_time <= time)
            {
                queue[rear++] = i;
            }
        }

        if (processes[current].remaining_time > 0)
        {
            queue[rear++] = current;
        }
    }

    *avg_waiting_time = (double)total_waiting_time / num_processes;
}

void shortest_job_first(Process processes[], int num_processes, int *time_points, int *process_schedule, int *schedule_length, double *avg_waiting_time)
{
    int time = 0;
    int completed = 0;
    int total_waiting_time = 0;
    int process_index;

    while (completed < num_processes)
    {
        process_index = -1;
        for (int i = 0; i < num_processes; i++)
        {
            if (processes[i].arrival_time <= time && processes[i].remaining_time > 0)
            {
                if (process_index == -1 || processes[i].remaining_time < processes[process_index].remaining_time)
                {
                    process_index = i;
                }
            }
        }

        if (process_index != -1)
        {
            time_points[*schedule_length] = time;
            process_schedule[*schedule_length] = processes[process_index].process_number;
            (*schedule_length)++;

            time += processes[process_index].burst_time;
            processes[process_index].waiting_time = time - processes[process_index].arrival_time - processes[process_index].burst_time;
            total_waiting_time += processes[process_index].waiting_time;
            processes[process_index].remaining_time = 0;
            completed++;
        }
        else
        {
            time++;
        }
    }

    *avg_waiting_time = (double)total_waiting_time / num_processes;
}

void priority_scheduling_no_preemption(Process processes[], int num_processes, int *time_points, int *process_schedule, int *schedule_length, double *avg_waiting_time)
{
    int time = 0;
    int completed = 0;
    int total_waiting_time = 0;
    int process_index;

    while (completed < num_processes)
    {
        process_index = -1;
        for (int i = 0; i < num_processes; i++)
        {
            if (processes[i].arrival_time <= time && processes[i].remaining_time > 0)
            {
                if (process_index == -1 || processes[i].priority < processes[process_index].priority)
                {
                    process_index = i;
                }
            }
        }

        if (process_index != -1)
        {
            time_points[*schedule_length] = time;
            process_schedule[*schedule_length] = processes[process_index].process_number;
            (*schedule_length)++;

            time += processes[process_index].burst_time;
            processes[process_index].waiting_time = time - processes[process_index].arrival_time - processes[process_index].burst_time;
            total_waiting_time += processes[process_index].waiting_time;
            processes[process_index].remaining_time = 0;
            completed++;
        }
        else
        {
            time++;
        }
    }

    *avg_waiting_time = (double)total_waiting_time / num_processes;
}

void priority_scheduling_with_preemption(Process processes[], int num_processes, int *time_points, int *process_schedule, int *schedule_length, double *avg_waiting_time)
{
    int time = 0;
    int completed = 0;
    int total_waiting_time = 0;
    int current = -1;
    int last_scheduled_time = -1;

    while (completed < num_processes)
    {
        for (int i = 0; i < num_processes; i++)
        {
            if (processes[i].arrival_time == time)
            {
                if (current == -1 || processes[i].priority < processes[current].priority)
                {
                    if (current != -1 && processes[current].remaining_time > 0)
                    {
                        if (last_scheduled_time != time)
                        {
                            time_points[*schedule_length] = time;
                            process_schedule[*schedule_length] = processes[current].process_number;
                            (*schedule_length)++;
                        }
                    }
                    current = i;
                }
            }
        }

        if (current != -1)
        {
            if (last_scheduled_time != time)
            {
                time_points[*schedule_length] = time;
                process_schedule[*schedule_length] = processes[current].process_number;
                (*schedule_length)++;
            }
            processes[current].remaining_time--;
            if (processes[current].remaining_time == 0)
            {
                processes[current].waiting_time = time + 1 - processes[current].arrival_time - processes[current].burst_time;
                total_waiting_time += processes[current].waiting_time;
                completed++;
                current = -1;
            }
        }
        else
        {
            time_points[*schedule_length] = time;
            process_schedule[*schedule_length] = -1; // No process is currently executing
            (*schedule_length)++;
        }
        last_scheduled_time = time;
        time++;
    }

    *avg_waiting_time = (double)total_waiting_time / num_processes;
}

int main()
{
    char algorithm[20];
    int num_processes;
    int time_quantum = 0;
    Process processes[MAX_PROCESSES];

    read_input("input.txt", algorithm, &num_processes, processes, &time_quantum);

    int time_points[MAX_PROCESSES * 100];
    int process_schedule[MAX_PROCESSES * 100];
    int schedule_length = 0;
    double avg_waiting_time;

    if (strcmp(algorithm, "RR") == 0)
    {
        round_robin(processes, num_processes, time_quantum, time_points, process_schedule, &schedule_length, &avg_waiting_time);
    }
    else if (strcmp(algorithm, "SJF") == 0)
    {
        shortest_job_first(processes, num_processes, time_points, process_schedule, &schedule_length, &avg_waiting_time);
    }
    else if (strcmp(algorithm, "PR_noPREMP") == 0)
    {
        priority_scheduling_no_preemption(processes, num_processes, time_points, process_schedule, &schedule_length, &avg_waiting_time);
    }
    else if (strcmp(algorithm, "PR_withPREMP") == 0)
    {
        priority_scheduling_with_preemption(processes, num_processes, time_points, process_schedule, &schedule_length, &avg_waiting_time);
    }

    write_output("output.txt", algorithm, time_points, process_schedule, schedule_length, avg_waiting_time);

    printf("Input:\n");
    print_file("input.txt");

    printf("\nOutput:\n");
    print_file("output.txt");

    return 0;
}
