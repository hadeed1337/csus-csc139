/*
CSC139
Summer 2024
Second Assignment
Francis, Jacob
Section 1
OSs Tested on: MAC
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_SIZE 100000000
#define MAX_THREADS 16
#define RANDOM_SEED 7649
#define MAX_RANDOM_NUMBER 3000
#define NUM_LIMIT 9973

// Global variables
long gRefTime;		 // For timing
int gData[MAX_SIZE]; // The array that will hold the data

int gThreadCount;						// Number of threads
volatile int gDoneThreadCount;			// Number of threads that are done at a certain point. Whenever a thread is done, it increments this. Used with the semaphore-based solution
int gThreadProd[MAX_THREADS];			// The modular product for each array division that a single thread is responsible for
volatile bool gThreadDone[MAX_THREADS]; // Is this thread done? Used when the parent is continually checking on child threads

// Mutex and condition variables
pthread_mutex_t mutex;
pthread_cond_t cond;

int SqFindProd(int size);				// Sequential FindProduct (no threads) computes the product of all the elements in the array mod NUM_LIMIT
void *ThFindProd(void *param);			// Thread FindProduct but without semaphores
void *ThFindProdWithMutex(void *param); // Thread FindProduct with mutex and condition variables
int ComputeTotalProduct();				// Multiply the division products to compute the total modular product
void InitSharedVars();
void GenerateInput(int size, int indexForZero);									// Generate the input array
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]); // Calculate the indices to divide the array into T divisions, one division per thread
int GetRand(int min, int max);													// Get a random number between min and max

// Timing functions
long GetMilliSecondTime(struct timeval timeBuf);
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
	for (i = 0; i < gThreadCount; i++)
	{
		pthread_attr_init(&attr[i]);
		pthread_create(&tid[i], &attr[i], ThFindProd, (void *)indices[i]);
	}
	while (gDoneThreadCount < gThreadCount)
	{
		// Busy waiting loop
		usleep(100); // Sleep for 100 microseconds to reduce CPU usage
	}
	prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent continually checking on children completed in %ld ms. Product = %d\n", GetTime(), prod);

	// Multi-threaded with mutex and condition variables
	InitSharedVars();
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	SetTime();
	for (i = 0; i < gThreadCount; i++)
	{
		pthread_attr_init(&attr[i]);
		pthread_create(&tid[i], &attr[i], ThFindProdWithMutex, (void *)indices[i]);
	}
	pthread_mutex_lock(&mutex);
	while (gDoneThreadCount < gThreadCount)
	{
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);
	prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent waiting on condition variable completed in %ld ms. Prod = %d\n", GetTime(), prod);

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);

	return 0;
}

int SqFindProd(int size)
{
	long prod = 1;
	for (int i = 0; i < size; i++)
	{
		prod = (prod * gData[i]) % NUM_LIMIT;
		if (prod == 0)
		{
			break;
		}
	}
	return prod;
}

void *ThFindProd(void *param)
{
	int threadNum = ((int *)param)[0];
	int start = ((int *)param)[1];
	int end = ((int *)param)[2];
	long prod = 1;

	for (int i = start; i < end; i++)
	{
		prod = (prod * gData[i]) % NUM_LIMIT;
		if (prod == 0)
		{
			break;
		}
	}

	gThreadProd[threadNum] = prod;
	gThreadDone[threadNum] = true;

	__sync_fetch_and_add(&gDoneThreadCount, 1); // Atomic increment of gDoneThreadCount

	return NULL;
}

void *ThFindProdWithMutex(void *param)
{
	int threadNum = ((int *)param)[0];
	int start = ((int *)param)[1];
	int end = ((int *)param)[2];
	long prod = 1;

	for (int i = start; i < end; i++)
	{
		prod = (prod * gData[i]) % NUM_LIMIT;
		if (prod == 0)
		{
			break;
		}
	}

	gThreadProd[threadNum] = prod;

	pthread_mutex_lock(&mutex);
	gDoneThreadCount++;
	if (prod == 0 || gDoneThreadCount == gThreadCount)
	{
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&mutex);

	return NULL;
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
	srand(RANDOM_SEED);
	for (int i = 0; i < size; i++)
	{
		gData[i] = GetRand(1, MAX_RANDOM_NUMBER);
	}
	if (indexForZero >= 0)
	{
		gData[indexForZero] = 0;
	}
}

void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3])
{
	int divisionSize = arraySize / thrdCnt;
	for (int i = 0; i < thrdCnt; i++)
	{
		indices[i][0] = i;
		indices[i][1] = i * divisionSize;
		indices[i][2] = (i == thrdCnt - 1) ? arraySize : (i + 1) * divisionSize;
	}
}

int GetRand(int x, int y)
{
	int r = rand();
	r = x + r % (y - x + 1);
	return r;
}

long GetMilliSecondTime(struct timeval timeBuf)
{
	long mliScndTime;
	mliScndTime = timeBuf.tv_sec;
	mliScndTime *= 1000;
	mliScndTime += timeBuf.tv_usec / 1000;
	return mliScndTime;
}

long GetCurrentTime(void)
{
	long crntTime = 0;
	struct timeval timeBuf;
	gettimeofday(&timeBuf, NULL);
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
