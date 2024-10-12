/*
CSC139
Fall 2024
Second Assignment
Francis, Jacob
Section 1
OSs Tested on: MAC, LINUX
Architecture:            x86_64
CPU(s):                  4
  Model name:            Intel(R) Xeon(R) Gold 6254 CPU @ 3.10GHz
    CPU family:          6
    Model:               58
    Thread(s) per core:  1
    Core(s) per socket:  2
    Socket(s):           2
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <semaphore.h>
#include <stdbool.h>

#define MAX_SIZE 100000000
#define MAX_THREADS 16
#define RANDOM_SEED 7649
#define MAX_RANDOM_NUMBER 3000
#define NUM_LIMIT 9973

// Global variables
long gRefTime;							// For timing
int gData[MAX_SIZE];					// The array that will hold the data
int gThreadCount;						// Number of threads
int gDoneThreadCount;					// Number of threads that are done at a certain point
volatile int gThreadProd[MAX_THREADS];	// The modular product for each array division that a single thread is responsible for
volatile bool gThreadDone[MAX_THREADS]; // Is this thread done? Used when the parent is continually checking on child threads

// Semaphores
sem_t completed; // To notify parent that all threads have completed or one of them found a zero
sem_t mutex;	 // Binary semaphore to protect the shared variable gDoneThreadCount

int SqFindProd(int size);					// Sequential FindProduct (no threads)
void *ThFindProd(void *param);				// Thread FindProduct without semaphores
void *ThFindProdWithSemaphore(void *param); // Thread FindProduct with semaphores
int ComputeTotalProduct();					// Multiply the division products to compute the total modular product
void InitSharedVars();
void GenerateInput(int size, int indexForZero);									// Generate the input array
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]); // Calculate the indices to divide the array
int GetRand(int min, int max);													// Get a random number between min and max

// Timing functions
long GetMilliSecondTime(struct timeb timeBuf);
long GetCurrentTime(void);
void SetTime(void);
long GetTime(void);

int main(int argc, char *argv[])
{
	pthread_t tid[MAX_THREADS];
	pthread_attr_t attr[MAX_THREADS];
	int indices[MAX_THREADS][3];
	int i, indexForZero, arraySize, prod;

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

	GenerateInput(arraySize, indexForZero);
	CalculateIndices(arraySize, gThreadCount, indices);

	// Code for the sequential part
	SetTime();
	prod = SqFindProd(arraySize);
	printf("Sequential multiplication completed in %ld ms. Product = %d\n", GetTime(), prod);

	// Threaded with parent waiting for all child threads
	InitSharedVars();
	SetTime();
	// Initialize threads, create threads, and then let the parent wait for all threads using pthread_join
	for (i = 0; i < gThreadCount; i++)
	{
		pthread_attr_init(&attr[i]);
		pthread_create(&tid[i], &attr[i], ThFindProd, (void *)indices[i]);
	}
	for (i = 0; i < gThreadCount; i++)
	{
		pthread_join(tid[i], NULL);
	}
	prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent waiting for all children completed in %ld ms. Product = %d\n", GetTime(), prod);

	// Multi-threaded with busy waiting
	InitSharedVars();
	SetTime();
	// Initialize threads, create threads, and then make the parent continually check on all child threads
	for (i = 0; i < gThreadCount; i++)
	{
		pthread_attr_init(&attr[i]);
		pthread_create(&tid[i], &attr[i], ThFindProd, (void *)indices[i]);
	}
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
				// Found zero, cancel all threads
				for (int j = 0; j < gThreadCount; j++)
				{
					pthread_cancel(tid[j]);
				}
				allDone = true;
				break;
			}
		}
	}
	prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent continually checking on children completed in %ld ms. Product = %d\n", GetTime(), prod);

	// Multi-threaded with semaphores
	InitSharedVars();
	// Initialize your semaphores here
	sem_init(&completed, 0, 0);
	sem_init(&mutex, 0, 1);
	SetTime();
	// Initialize threads, create threads, and then make the parent wait on the "completed" semaphore
	for (i = 0; i < gThreadCount; i++)
	{
		pthread_attr_init(&attr[i]);
		pthread_create(&tid[i], &attr[i], ThFindProdWithSemaphore, (void *)indices[i]);
	}
	// Parent waits on the 'completed' semaphore
	sem_wait(&completed);
	// Once woken up, parent cancels all threads
	for (i = 0; i < gThreadCount; i++)
	{
		pthread_cancel(tid[i]);
	}
	// Wait for all threads to finish
	for (i = 0; i < gThreadCount; i++)
	{
		pthread_join(tid[i], NULL);
	}
	prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent waiting on a semaphore completed in %ld ms. Prod = %d\n", GetTime(), prod);

	// Destroy semaphores
	sem_destroy(&completed);
	sem_destroy(&mutex);

	return 0;
}

// Sequential FindProduct (no threads)
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

// Thread FindProduct without semaphores
void *ThFindProd(void *param)
{
	int *indices = (int *)param;
	int threadNum = indices[0];
	int startIndex = indices[1];
	int endIndex = indices[2];
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
	pthread_exit(0);
}

// Thread FindProduct with semaphores
void *ThFindProdWithSemaphore(void *param)
{
	int *indices = (int *)param;
	int threadNum = indices[0];
	int startIndex = indices[1];
	int endIndex = indices[2];
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

	if (foundZero)
	{
		// Post the completed semaphore
		sem_post(&completed);
	}
	else
	{
		// Protect gDoneThreadCount with mutex semaphore
		sem_wait(&mutex);
		gDoneThreadCount++;
		if (gDoneThreadCount == gThreadCount)
		{
			// All threads done
			sem_post(&completed);
		}
		sem_post(&mutex);
	}
	pthread_exit(0);
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
	gDoneThreadCount = 0;
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
