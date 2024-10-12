// File: MPFindProd.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdbool.h>

#define MAX_SIZE 100000000
#define MAX_THREADS 16
#define RANDOM_SEED 7649
#define MAX_RANDOM_NUMBER 3000
#define NUM_LIMIT 9973

// Global variables (to be placed in shared memory)
long gRefTime;              // For timing
int *gData;                 // The array that will hold the data (shared memory)
int gThreadCount;           // Number of processes
int *gDoneThreadCount;      // Number of processes that are done at a certain point
volatile int *gThreadProd;  // The modular product for each array division
volatile bool *gThreadDone; // Is this process done?

// Semaphores
sem_t *completed; // To notify parent that all processes have completed or one of them found a zero
sem_t *mutex;     // Binary semaphore to protect the shared variable gDoneThreadCount

int SqFindProd(int size);                    // Sequential FindProduct (no processes)
void ChildFindProd(int *param);              // Child process FindProduct without semaphores
void ChildFindProdWithSemaphore(int *param); // Child process FindProduct with semaphores
int ComputeTotalProduct();                   // Multiply the division products to compute the total modular product
void InitSharedVars();
void GenerateInput(int size, int indexForZero);                                 // Generate the input array
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]); // Calculate the indices to divide the array
int GetRand(int min, int max);                                                  // Get a random number between min and max

// Timing functions
long GetMilliSecondTime(struct timeb timeBuf);
long GetCurrentTime(void);
void SetTime(void);
long GetTime(void);

int main(int argc, char *argv[])
{
    int indices[MAX_THREADS][3];
    int i, indexForZero, arraySize, prod;
    pid_t pid[MAX_THREADS];
    int shm_id_data, shm_id_threadProd, shm_id_threadDone, shm_id_doneThreadCount;

    // Code for parsing and checking command-line arguments
    if (argc != 4)
    {
        fprintf(stderr, "Invalid number of arguments!\n");
        exit(-1);
    }
    if ((arraySize = atoi(argv[1])) <= 0 || arraySize > MAX_SIZE)
    {
        fprintf(stderr, "Invalid Array Size\n");
        exit(-1);
    }
    gThreadCount = atoi(argv[2]);
    if (gThreadCount > MAX_THREADS || gThreadCount <= 0)
    {
        fprintf(stderr, "Invalid Thread Count\n");
        exit(-1);
    }
    indexForZero = atoi(argv[3]);
    if (indexForZero < -1 || indexForZero >= arraySize)
    {
        fprintf(stderr, "Invalid index for zero!\n");
        exit(-1);
    }

    // Create shared memory segments
    shm_id_data = shmget(IPC_PRIVATE, arraySize * sizeof(int), IPC_CREAT | 0666);
    shm_id_threadProd = shmget(IPC_PRIVATE, MAX_THREADS * sizeof(int), IPC_CREAT | 0666);
    shm_id_threadDone = shmget(IPC_PRIVATE, MAX_THREADS * sizeof(bool), IPC_CREAT | 0666);
    shm_id_doneThreadCount = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);

    if (shm_id_data < 0 || shm_id_threadProd < 0 || shm_id_threadDone < 0 || shm_id_doneThreadCount < 0)
    {
        perror("shmget error");
        exit(1);
    }

    // Attach shared memory
    gData = (int *)shmat(shm_id_data, NULL, 0);
    gThreadProd = (int *)shmat(shm_id_threadProd, NULL, 0);
    gThreadDone = (bool *)shmat(shm_id_threadDone, NULL, 0);
    gDoneThreadCount = (int *)shmat(shm_id_doneThreadCount, NULL, 0);

    if (gData == (int *)-1 || gThreadProd == (int *)-1 || gThreadDone == (void *)-1 || gDoneThreadCount == (void *)-1)
    {
        perror("shmat error");
        exit(1);
    }

    GenerateInput(arraySize, indexForZero);
    CalculateIndices(arraySize, gThreadCount, indices);

    // Code for the sequential part
    SetTime();
    prod = SqFindProd(arraySize);
    printf("Sequential multiplication completed in %ld ms. Product = %d\n", GetTime(), prod);

    // Process-based with parent waiting for all child processes
    InitSharedVars();
    SetTime();
    // Create child processes
    for (i = 0; i < gThreadCount; i++)
    {
        pid[i] = fork();
        if (pid[i] < 0)
        {
            perror("fork error");
            exit(1);
        }
        else if (pid[i] == 0)
        {
            // Child process
            ChildFindProd(indices[i]);
            exit(0);
        }
    }
    // Parent waits for all child processes
    for (i = 0; i < gThreadCount; i++)
    {
        waitpid(pid[i], NULL, 0);
    }
    prod = ComputeTotalProduct();
    printf("Process-based multiplication with parent waiting for all children completed in %ld ms. Product = %d\n", GetTime(), prod);

    // Multi-process with busy waiting (parent continually checking on child processes)
    InitSharedVars();
    SetTime();
    // Create child processes
    for (i = 0; i < gThreadCount; i++)
    {
        pid[i] = fork();
        if (pid[i] < 0)
        {
            perror("fork error");
            exit(1);
        }
        else if (pid[i] == 0)
        {
            // Child process
            ChildFindProd(indices[i]);
            exit(0);
        }
    }
    // Parent busy waiting
    volatile bool allDone = false;
    while (!allDone)
    {
        allDone = true;
        for (i = 0; i < gThreadCount; i++)
        {
            if (!gThreadDone[i])
            {
                allDone = false;
                break;
            }
            if (gThreadProd[i] == 0)
            {
                // Found zero, kill all child processes
                for (int j = 0; j < gThreadCount; j++)
                {
                    kill(pid[j], SIGKILL);
                }
                allDone = true;
                break;
            }
        }
    }
    // Wait for child processes to terminate
    for (i = 0; i < gThreadCount; i++)
    {
        waitpid(pid[i], NULL, 0);
    }
    prod = ComputeTotalProduct();
    printf("Process-based multiplication with parent continually checking on children completed in %ld ms. Product = %d\n", GetTime(), prod);

    // Multi-process with semaphores
    InitSharedVars();

    // Initialize semaphores
    sem_unlink("completed_sem");
    sem_unlink("mutex_sem");
    completed = sem_open("completed_sem", O_CREAT, 0644, 0);
    mutex = sem_open("mutex_sem", O_CREAT, 0644, 1);

    if (completed == SEM_FAILED || mutex == SEM_FAILED)
    {
        perror("sem_open error");
        exit(1);
    }

    SetTime();
    // Create child processes
    for (i = 0; i < gThreadCount; i++)
    {
        pid[i] = fork();
        if (pid[i] < 0)
        {
            perror("fork error");
            exit(1);
        }
        else if (pid[i] == 0)
        {
            // Child process
            ChildFindProdWithSemaphore(indices[i]);
            exit(0);
        }
    }
    // Parent waits on the 'completed' semaphore
    sem_wait(completed);
    // Once woken up, parent kills all child processes
    for (i = 0; i < gThreadCount; i++)
    {
        kill(pid[i], SIGKILL);
    }
    // Wait for child processes to terminate
    for (i = 0; i < gThreadCount; i++)
    {
        waitpid(pid[i], NULL, 0);
    }
    prod = ComputeTotalProduct();
    printf("Process-based multiplication with parent waiting on a semaphore completed in %ld ms. Product = %d\n", GetTime(), prod);

    // Clean up semaphores
    sem_close(completed);
    sem_close(mutex);
    sem_unlink("completed_sem");
    sem_unlink("mutex_sem");

    // Detach and remove shared memory
    shmdt(gData);
    shmdt(gThreadProd);
    shmdt(gThreadDone);
    shmdt(gDoneThreadCount);

    shmctl(shm_id_data, IPC_RMID, NULL);
    shmctl(shm_id_threadProd, IPC_RMID, NULL);
    shmctl(shm_id_threadDone, IPC_RMID, NULL);
    shmctl(shm_id_doneThreadCount, IPC_RMID, NULL);

    return 0;
}

// Sequential FindProduct (no processes)
int SqFindProd(int size)
{
    int prod = 1;
    for (int i = 0; i < size; i++)
    {
        if (gData[i] == 0)
        {
            return 0;
        }
        prod *= gData[i];
        prod %= NUM_LIMIT;
    }
    return prod;
}

// Child process FindProduct without semaphores
void ChildFindProd(int *param)
{
    int threadNum = param[0];
    int startIndex = param[1];
    int endIndex = param[2];
    int prod = 1;

    for (int i = startIndex; i <= endIndex; i++)
    {
        if (gData[i] == 0)
        {
            prod = 0;
            break;
        }
        prod *= gData[i];
        prod %= NUM_LIMIT;
    }
    gThreadProd[threadNum] = prod;
    gThreadDone[threadNum] = true;
}

// Child process FindProduct with semaphores
void ChildFindProdWithSemaphore(int *param)
{
    int threadNum = param[0];
    int startIndex = param[1];
    int endIndex = param[2];
    int prod = 1;
    int foundZero = 0;

    for (int i = startIndex; i <= endIndex; i++)
    {
        if (gData[i] == 0)
        {
            prod = 0;
            foundZero = 1;
            break;
        }
        prod *= gData[i];
        prod %= NUM_LIMIT;
    }
    gThreadProd[threadNum] = prod;

    // Open semaphores
    sem_t *completed = sem_open("completed_sem", 0);
    sem_t *mutex = sem_open("mutex_sem", 0);

    if (foundZero)
    {
        // Post the completed semaphore
        sem_post(completed);
    }
    else
    {
        // Protect gDoneThreadCount with mutex semaphore
        sem_wait(mutex);
        (*gDoneThreadCount)++;
        if (*gDoneThreadCount == gThreadCount)
        {
            // All processes done
            sem_post(completed);
        }
        sem_post(mutex);
    }

    sem_close(completed);
    sem_close(mutex);
}

int ComputeTotalProduct()
{
    int i, prod = 1;
    for (i = 0; i < gThreadCount; i++)
    {
        prod *= gThreadProd[i];
        prod %= NUM_LIMIT;
    }
    return prod;
}

void InitSharedVars()
{
    int i;
    for (i = 0; i < gThreadCount; i++)
    {
        gThreadDone[i] = false;
        gThreadProd[i] = 1;
    }
    *gDoneThreadCount = 0;
}

void GenerateInput(int size, int indexForZero)
{
    srand(RANDOM_SEED); // Initialize random number generator with seed
    for (int i = 0; i < size; i++)
    {
        gData[i] = GetRand(1, MAX_RANDOM_NUMBER);
    }
    if (indexForZero >= 0 && indexForZero < size)
    {
        gData[indexForZero] = 0;
    }
}

void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3])
{
    int divisionSize = arraySize / thrdCnt;
    int remainder = arraySize % thrdCnt;
    int start = 0;

    for (int i = 0; i < thrdCnt; i++)
    {
        indices[i][0] = i;
        indices[i][1] = start;
        int end = start + divisionSize - 1;
        if (i < remainder)
        {
            end += 1;
        }
        indices[i][2] = end;
        start = end + 1;
    }
}

int GetRand(int x, int y)
{
    int r = rand();
    r = x + r % (y - x + 1);
    return r;
}

long GetMilliSecondTime(struct timeb timeBuf)
{
    long mliScndTime;
    mliScndTime = timeBuf.time;
    mliScndTime *= 1000;
    mliScndTime += timeBuf.millitm;
    return mliScndTime;
}

long GetCurrentTime(void)
{
    long crntTime = 0;
    struct timeb timeBuf;
    ftime(&timeBuf);
    crntTime = GetMilliSecondTime(timeBuf);
    return crntTime;
}

void SetTime(void)
{
    gRefTime = GetCurrentTime();
}

long GetTime(void)
{
    long crntTime = GetCurrentTime();
    return (crntTime - gRefTime);
}
