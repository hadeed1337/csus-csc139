#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_PAGES 100
#define MAX_FRAMES 100

// Function prototypes
void fifo(int pages[], int num_pages, int num_frames, FILE *output);
void optimal(int pages[], int num_pages, int num_frames, FILE *output);
void lru(int pages[], int num_pages, int num_frames, FILE *output);

int main()
{
    FILE *input = fopen("input.txt", "r");
    if (input == NULL)
    {
        printf("Error opening input file.\n");
        return 1;
    }

    FILE *output = fopen("output.txt", "w");
    if (output == NULL)
    {
        printf("Error opening output file.\n");
        fclose(input);
        return 1;
    }

    int num_pages, num_frames, num_requests;
    fscanf(input, "%d %d %d", &num_pages, &num_frames, &num_requests);

    int pages[num_requests];
    for (int i = 0; i < num_requests; i++)
    {
        fscanf(input, "%d", &pages[i]);
    }

    fclose(input);

    fprintf(output, "FIFO\n");
    fifo(pages, num_requests, num_frames, output);
    fprintf(output, "\nOptimal\n");
    optimal(pages, num_requests, num_frames, output);
    fprintf(output, "\nLRU\n");
    lru(pages, num_requests, num_frames, output);

    fclose(output);

    return 0;
}

void fifo(int pages[], int num_pages, int num_frames, FILE *output)
{
    int frame[MAX_FRAMES], index = 0, faults = 0;
    for (int i = 0; i < num_frames; i++)
        frame[i] = -1;

    for (int i = 0; i < num_pages; i++)
    {
        int found = 0;
        for (int j = 0; j < num_frames; j++)
        {
            if (frame[j] == pages[i])
            {
                found = 1;
                fprintf(output, "Page %d already in Frame %d\n", pages[i], j);
                break;
            }
        }
        if (!found)
        {
            if (frame[index] != -1)
            {
                fprintf(output, "Page %d unloaded from Frame %d, ", frame[index], index);
            }
            frame[index] = pages[i];
            fprintf(output, "Page %d loaded into Frame %d\n", pages[i], index);
            index = (index + 1) % num_frames;
            faults++;
        }
    }
    fprintf(output, "%d page faults\n", faults);
}

void optimal(int pages[], int num_pages, int num_frames, FILE *output)
{
    int frame[MAX_FRAMES], next_use[MAX_FRAMES], faults = 0;
    for (int i = 0; i < num_frames; i++)
        frame[i] = -1;

    for (int i = 0; i < num_pages; i++)
    {
        int found = 0;
        for (int j = 0; j < num_frames; j++)
        {
            if (frame[j] == pages[i])
            {
                found = 1;
                fprintf(output, "Page %d already in Frame %d\n", pages[i], j);
                break;
            }
        }
        if (!found)
        {
            for (int j = 0; j < num_frames; j++)
            {
                next_use[j] = INT_MAX;
                for (int k = i + 1; k < num_pages; k++)
                {
                    if (frame[j] == pages[k])
                    {
                        next_use[j] = k;
                        break;
                    }
                }
            }
            int index = -1, farthest = -1;
            for (int j = 0; j < num_frames; j++)
            {
                if (frame[j] == -1)
                {
                    index = j;
                    break;
                }
                if (next_use[j] > farthest)
                {
                    farthest = next_use[j];
                    index = j;
                }
            }
            if (frame[index] != -1)
            {
                fprintf(output, "Page %d unloaded from Frame %d, ", frame[index], index);
            }
            frame[index] = pages[i];
            fprintf(output, "Page %d loaded into Frame %d\n", pages[i], index);
            faults++;
        }
    }
    fprintf(output, "%d page faults\n", faults);
}

void lru(int pages[], int num_pages, int num_frames, FILE *output)
{
    int frame[MAX_FRAMES], last_used[MAX_FRAMES], faults = 0, time = 0;
    for (int i = 0; i < num_frames; i++)
    {
        frame[i] = -1;
        last_used[i] = -1;
    }

    for (int i = 0; i < num_pages; i++)
    {
        int found = 0;
        for (int j = 0; j < num_frames; j++)
        {
            if (frame[j] == pages[i])
            {
                found = 1;
                last_used[j] = time++;
                fprintf(output, "Page %d already in Frame %d\n", pages[i], j);
                break;
            }
        }
        if (!found)
        {
            int lru_index = 0;
            for (int j = 1; j < num_frames; j++)
            {
                if (last_used[j] < last_used[lru_index])
                {
                    lru_index = j;
                }
            }
            if (frame[lru_index] != -1)
            {
                fprintf(output, "Page %d unloaded from Frame %d, ", frame[lru_index], lru_index);
            }
            frame[lru_index] = pages[i];
            last_used[lru_index] = time++;
            fprintf(output, "Page %d loaded into Frame %d\n", pages[i], lru_index);
            faults++;
        }
    }
    fprintf(output, "%d page faults\n", faults);
}
