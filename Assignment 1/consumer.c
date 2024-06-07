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
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

// Size of shared memory block
#define SHM_SIZE 4096

// Global pointer to the shared memory block
void *gShmPtr;

// Function prototypes
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

int main()
{
        const char *name = "OS_HW1_JacobFrancis"; // Name of shared memory block to be passed to shm_open
        int bufSize;                              // Bounded buffer size
        int itemCnt;                              // Number of items to be consumed
        int in;                                   // Index of next item to produce
        int out;                                  // Index of next item to consume

        // Open shared memory block
        int shm_fd = shm_open(name, O_RDWR, 0666);
        if (shm_fd == -1)
        {
                perror("Shared memory opening failed");
                exit(1);
        }
        gShmPtr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (gShmPtr == MAP_FAILED)
        {
                perror("Shared memory mapping failed");
                exit(1);
        }

        // Read header values
        bufSize = GetBufSize();
        itemCnt = GetItemCnt();
        in = GetIn();
        out = GetOut();

        // Check that the consumer has read the right values
        printf("Consumer reading: bufSize = %d, itemCnt = %d, in = %d, out = %d\n", bufSize, itemCnt, in, out);

        for (int i = 0; i < itemCnt; i++)
        {
                // Wait if the buffer is empty
                while (GetIn() == GetOut())
                {
                        // Busy waiting
                }

                // Consume item
                int val = ReadAtBufIndex(GetOut());
                printf("Consuming Item %d with value %d at Index %d\n", i + 1, val, GetOut());

                // Update out index
                SetOut((GetOut() + 1) % bufSize);
        }

        // Remove shared memory segment
        if (shm_unlink(name) == -1)
        {
                printf("Error removing %s\n", name);
                exit(-1);
        }

        return 0;
}

// Set the value of shared variable "in"
void SetIn(int val)
{
        SetHeaderVal(2, val);
}

// Set the value of shared variable "out"
void SetOut(int val)
{
        SetHeaderVal(3, val);
}

// Get the ith value in the header
int GetHeaderVal(int i)
{
        int val;
        void *ptr = gShmPtr + i * sizeof(int);
        memcpy(&val, ptr, sizeof(int));
        return val;
}

// Set the ith value in the header
void SetHeaderVal(int i, int val)
{
        void *ptr = gShmPtr + i * sizeof(int);
        memcpy(ptr, &val, sizeof(int));
}

// Get the value of shared variable "bufSize"
int GetBufSize()
{
        return GetHeaderVal(0);
}

// Get the value of shared variable "itemCnt"
int GetItemCnt()
{
        return GetHeaderVal(1);
}

// Get the value of shared variable "in"
int GetIn()
{
        return GetHeaderVal(2);
}

// Get the value of shared variable "out"
int GetOut()
{
        return GetHeaderVal(3);
}

// Write the given val at the given index in the bounded buffer
void WriteAtBufIndex(int indx, int val)
{
        void *ptr = gShmPtr + 4 * sizeof(int) + indx * sizeof(int);
        memcpy(ptr, &val, sizeof(int));
}

// Read the val at the given index in the bounded buffer
int ReadAtBufIndex(int indx)
{
        int val;
        void *ptr = gShmPtr + 4 * sizeof(int) + indx * sizeof(int);
        memcpy(&val, ptr, sizeof(int));
        return val;
}