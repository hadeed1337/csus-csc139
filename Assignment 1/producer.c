/*
CSC139
Summer 2024
First Assignment
Francis, Jacob
Section 1
OSs Tested on: MAC
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

// Size of shared memory block
#define SHM_SIZE 4096

// Global pointer to the shared memory block
void *gShmPtr;

void Producer(int, int, int);
void InitShm(int, int);
void SetBufSize(int);
void SetItemCnt(int);
void SetIn(int);
void SetOut(int);
void SetHeaderVal(int, int);
int GetBufSize();
int GetItemCnt();
int GetIn();
int GetOut();
int GetHeaderVal(int);
void WriteAtBufIndex(int, int);
int ReadAtBufIndex(int);
int GetRand(int, int);

int main(int argc, char *argv[])
{
        pid_t pid;
        int bufSize;  // Bounded buffer size
        int itemCnt;  // Number of items to be produced
        int randSeed; // Seed for the random number generator

        if (argc != 4)
        {
                printf("Invalid number of command-line arguments\n");
                exit(1);
        }

        bufSize = atoi(argv[1]);
        itemCnt = atoi(argv[2]);
        randSeed = atoi(argv[3]);

        // Check validity of command-line arguments
        if (bufSize < 2 || bufSize > 450)
        {
                printf("Buffer size must be between 2 and 450\n");
                exit(1);
        }
        if (itemCnt <= 0)
        {
                printf("Item count must be greater than 0\n");
                exit(1);
        }

        // Initialize shared memory
        InitShm(bufSize, itemCnt);

        /* fork a child process */
        pid = fork();

        if (pid < 0)
        { /* error occurred */
                fprintf(stderr, "Fork Failed\n");
                exit(1);
        }
        else if (pid == 0)
        { /* child process */
                printf("Launching Consumer \n");
                execlp("./consumer", "consumer", NULL);
        }
        else
        { /* parent process */
                printf("Starting Producer\n");
                Producer(bufSize, itemCnt, randSeed);
                printf("Producer done and waiting for consumer\n");
                wait(NULL);
                printf("Consumer Completed\n");
        }

        return 0;
}

void InitShm(int bufSize, int itemCnt)
{
        int in = 0;
        int out = 0;
        const char *name = "OS_HW1_Francis_Jacob"; // Name of shared memory object to be passed to shm_open

        // Create shared memory block
        int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        ftruncate(shm_fd, SHM_SIZE);
        gShmPtr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

        // Initialize header
        SetBufSize(bufSize);
        SetItemCnt(itemCnt);
        SetIn(in);
        SetOut(out);
}

void Producer(int bufSize, int itemCnt, int randSeed)
{
        int in = 0;
        int out = 0;

        srand(randSeed);

        for (int i = 0; i < itemCnt; i++)
        {
                int val = GetRand(2, 3300);

                // Wait if the buffer is full
                while ((in + 1) % bufSize == out)
                {
                        // Busy waiting
                }

                // Produce item
                WriteAtBufIndex(in, val);
                printf("Producing Item %d with value %d at Index %d\n", i, val, in);

                // Update in index
                in = (in + 1) % bufSize;
                SetIn(in);
        }
        printf("Producer Completed\n");
}

void SetBufSize(int val)
{
        SetHeaderVal(0, val);
}

void SetItemCnt(int val)
{
        SetHeaderVal(1, val);
}

void SetIn(int val)
{
        SetHeaderVal(2, val);
}

void SetOut(int val)
{
        SetHeaderVal(3, val);
}

int GetHeaderVal(int i)
{
        int val;
        void *ptr = gShmPtr + i * sizeof(int);
        memcpy(&val, ptr, sizeof(int));
        return val;
}

void SetHeaderVal(int i, int val)
{
        void *ptr = gShmPtr + i * sizeof(int);
        memcpy(ptr, &val, sizeof(int));
}

int GetBufSize()
{
        return GetHeaderVal(0);
}

int GetItemCnt()
{
        return GetHeaderVal(1);
}

int GetIn()
{
        return GetHeaderVal(2);
}

int GetOut()
{
        return GetHeaderVal(3);
}

void WriteAtBufIndex(int indx, int val)
{
        void *ptr = gShmPtr + 4 * sizeof(int) + indx * sizeof(int);
        memcpy(ptr, &val, sizeof(int));
}

int ReadAtBufIndex(int indx)
{
        int val;
        void *ptr = gShmPtr + 4 * sizeof(int) + indx * sizeof(int);
        memcpy(&val, ptr, sizeof(int));
        return val;
}

int GetRand(int x, int y)
{
        int r = rand();
        r = x + r % (y - x + 1);
        return r;
}
