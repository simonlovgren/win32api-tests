/**
 **********************************************************************************************************************
 * @file       main.c
 * @author     Simon Lövgren
 * @date       2018
 * @copyright  MIT License
 * @brief      Multi-threading test.
 **********************************************************************************************************************
 */

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

 /**
  **********************************************************************************************************************
  * Defines
  **********************************************************************************************************************
  */
#define SYSTICK_THREAD           ( 1 )
#define NUM_THREADS              ( 1 )
#define SLEEP_PERIOD             ( 1 )
/* Waitable timer period (in threads). 1ms in relative time (LARGE_INTEGER/FILETIME format) */
#define WAITABLE_TIMER_PERIOD    ( -10000LL )

// Target resolution of multimedia timer (1ms)
#define TARGET_RESOLUTION 1

/**
 **********************************************************************************************************************
 * Typedefs
 **********************************************************************************************************************
 */

/* Holds data for thread */
typedef struct sThreadData
{
	HANDLE  threadHandle;  /* Handle to win32 thread itself. */
	uint8_t id;            /* ID of thread. */
	char const * name;     /* Name of thread.*/
} tThreadData;

/* Holds data for main. */
typedef struct sMainData
{
	tThreadData threads[NUM_THREADS]; /* Data for all threads to be spawned. */
} tMainData;

/**
 **********************************************************************************************************************
 * Prototypes
 **********************************************************************************************************************
 */

 /* Thread functions. */
static DWORD WINAPI ThreadWorker(LPVOID pThreadData);
static DWORD WINAPI ThreadSleep(LPVOID pThreadData);
static DWORD WINAPI ThreadWaitableTimer(LPVOID pThreadData);

VOID CALLBACK TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);

int fibonacci(int n);


/**
 **********************************************************************************************************************
 * Defines
 **********************************************************************************************************************
 */

 /* Statically allocated data for "module" main. */
static tMainData mainData;

/* Global event for all threads to stop. */
HANDLE ghStopEvent;

/* Global system tick variable. */
UINT32 SystemTick = 0;

/* Just somewhere to put the fib-result to force it to not be optimized away. */
volatile int fibresult = 0;

/**
 **********************************************************************************************************************
 * Public functions
 **********************************************************************************************************************
 */
int main(int argc, char** argv)
{

	/* Initialize main data. */
	memset((void *)&mainData.threads, 0, sizeof(mainData.threads));
	ghStopEvent = CreateEvent(
		NULL,       /* No security attributes.   */
		TRUE,       /* Manual reset.             */
		FALSE,      /* Initial state FALSE.      */
		NULL        /* No name.                  */
	);

	/* Create threads and start them. */
	tThreadData* pThread;

	pThread = &mainData.threads[0];
	pThread->id = 0;
	pThread->name = "Worker";
	pThread->threadHandle = CreateThread(
		NULL,                  /* No security attributes.   */
		0,                     /* Default stack size.       */
		ThreadWorker,          /* Thread function to start. */
		pThread,               /* Pointer to thread data.   */
		0,                     /* No creation flags.        */
		NULL                   /* No win32 threadid(?).     */
	);
	if (pThread->threadHandle == INVALID_HANDLE_VALUE)
	{
		printf("Unable to create thread %s\n", pThread->name);
	}

	//pThread = &mainData.threads[1];
	//pThread->id = 1;
	//pThread->name = "Waitable timer";
	//pThread->threadHandle = CreateThread(
	//	NULL,               /* No security attributes.   */
	//	0,                  /* Default stack size.       */
	//	ThreadWaitableTimer,/* Thread function to start. */
	//	pThread,            /* Pointer to thread data.   */
	//	0,                  /* No creation flags.        */
	//	NULL                /* No win32 threadid(?).     */
	//);
	//if (pThread->threadHandle == INVALID_HANDLE_VALUE)
	//{
	//	printf("Unable to create thread %s\n", pThread->name);
	//}

	//pThread = &mainData.threads[1];
	//pThread->id = 1;
	//pThread->name = "Sleep";
	//pThread->threadHandle = CreateThread(
	//	NULL,               /* No security attributes.   */
	//	0,                  /* Default stack size.       */
	//	ThreadSleep,        /* Thread function to start. */
	//	pThread,            /* Pointer to thread data.   */
	//	0,                  /* No creation flags.        */
	//	NULL                /* No win32 threadid(?).     */
	//);
	//if (pThread->threadHandle == INVALID_HANDLE_VALUE)
	//{
	//	printf("Unable to create thread %s\n", pThread->name);
	//}

	// Timer timer
	PHANDLE hTimer = INVALID_HANDLE_VALUE;
	BOOL res = CreateTimerQueueTimer(
		&hTimer,
		NULL,				// Use the default timer queue
		TimerCallback,		// Callback function
		NULL,				// No parameter to be sent to callback function
		0,					// Time in milliseconds before first trigger
		1,					// Period in milliseconds (0 for one-shot)
		WT_EXECUTEDEFAULT	// Flags
	);
	if (res == FALSE)
	{
		printf("Unable to create timer.");
		goto error;
	}

	/////////////////////////////////////////////////////////////

	/* Wait for user input before continuing in main thread. */
	char c = getchar();

	error:
	/* Tell all threads to die by setting global stop event. */
	SetEvent(ghStopEvent);

	/* Delete queue timer(s) */
	if (hTimer != INVALID_HANDLE_VALUE)
	{
		DeleteTimerQueueTimer(NULL, hTimer, NULL);
	}

	/* Wait for threads to stop. */
	for (int i = 0; i < NUM_THREADS; ++i)
	{
		WaitForSingleObject(mainData.threads[i].threadHandle, INFINITE);
		CloseHandle(mainData.threads[i].threadHandle);
	}
	CloseHandle(ghStopEvent);

	printf("Goodbye from main!\n");
	return 0;
}


/**
 **********************************************************************************************************************
 * Private functions
 **********************************************************************************************************************
 */


static DWORD WINAPI ThreadSleep(LPVOID pThreadData)
{
	tThreadData* pData = pThreadData;
	HANDLE       waitableTimer = NULL;
	HANDLE       arHandles[2];
	int          helloCount = 1;

	if (pData == NULL)
	{
		printf("[THREAD] Thread data pointer invalid.\n");
		return 0;
	}

	/* Add global event handle to array of handles. */
	arHandles[0] = ghStopEvent;

	/* Sleep for id number of seconds in order to not print at the same time (hopefully) (bad implementation). */
	Sleep(pData->id * 1000);
	printf("[THREAD %s] Hello!\n", pData->name);

	// Set up high resolution time measurement
	/*LARGE_INTEGER Start, End, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&Start);*/

	/* Loop until error OR global stop event. */
	while (TRUE)
	{
		switch (WaitForSingleObject( ghStopEvent, 0 ) )
		{
		case WAIT_OBJECT_0:
		{
			/* Global thread stop event. */
			printf("[THREAD %s] Shutting down!\n", pData->name);
			CancelWaitableTimer(waitableTimer);
			CloseHandle(waitableTimer);
			return 0;
		}
		case WAIT_TIMEOUT:
		{
			/* Event not set */
			Sleep(SLEEP_PERIOD);
			/*QueryPerformanceCounter(&End);
			ElapsedMicroseconds.QuadPart = End.QuadPart - Start.QuadPart;
			ElapsedMicroseconds.QuadPart *= 1000000;
			ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;*/
			++helloCount;
			if (SYSTICK_THREAD == pData->id)
			{	// Advance system tick if we are the chosen thread to do so
				++SystemTick;
			}
			//printf("[THREAD %s] Hello again! (#%d, %llu us)\n", pData->name, helloCount, ElapsedMicroseconds.QuadPart);
			break;
		}
		default:
		{
			printf("[THREAD %s] WaitForSingleObject failed (%d)\n", pData->name, GetLastError());
			CancelWaitableTimer(waitableTimer);
			CloseHandle(waitableTimer);
			return 0;
		}
		}
		//QueryPerformanceCounter(&Start); // Restart measurement
	}
}

static DWORD WINAPI ThreadWaitableTimer(LPVOID pThreadData)
{
	tThreadData* pData = pThreadData;
	HANDLE       waitableTimer = NULL;
	HANDLE       arHandles[2];
	int          helloCount = 1;

	if (pData == NULL)
	{
		printf("[THREAD] Thread data pointer invalid.\n");
		return 0;
	}

	/* Add global event handle to array of handles. */
	arHandles[0] = ghStopEvent;

	/* Sleep for id number of seconds in order to not print at the same time (hopefully) (bad implementation). */
	Sleep(pData->id * 1000);
	printf("[THREAD %s] Hello!\n", pData->name);

	/* Set up waitable timer to not spam printf all the time. */
	waitableTimer = CreateWaitableTimer(
		NULL,           /* No security settings.  */
		TRUE,           /* Manual reset.          */
		NULL            /* No timer name.         */
	);
	if (waitableTimer == INVALID_HANDLE_VALUE)
	{
		printf("[THREAD %s] Unable to create waitable timer.\n", pData->name);
		return 0;
	}

	/* Add waitable timer to array of handles. */
	arHandles[1] = waitableTimer;

	/* Start waitable timer. */
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = WAITABLE_TIMER_PERIOD;

	if (SetWaitableTimer(
		waitableTimer,              /* Timer handle.                                                                     */
		&liDueTime,
		0,                          /* Non-periodic timer, manual restart.                                               */
		NULL,                       /* No coroutine.                                                                     */
		NULL,                       /* No data to coroutine.                                                             */
		FALSE                       /* Do not restore system if in suspended power consumption mode when timer runs out. */
	) == FALSE)
	{
		printf("[THREAD %s] Unable to set waitable timer.\n", pData->name);
		return 0;
	}

	// Set up high resolution time measurement
	LARGE_INTEGER Start, End, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&Start);

	/* Loop until error OR global stop event. */
	while (TRUE)
	{
		switch (WaitForMultipleObjects(2, arHandles, FALSE, INFINITE))
		{
			case WAIT_OBJECT_0:
			{
				/* Global thread stop event. */
				printf("[THREAD %s] Shutting down!\n", pData->name);
				CancelWaitableTimer(waitableTimer);
				CloseHandle(waitableTimer);
				return 0;
			}
			case WAIT_OBJECT_0 + 1:
			{
				/* Timer finished, */
				QueryPerformanceCounter(&End);
				ElapsedMicroseconds.QuadPart = End.QuadPart - Start.QuadPart;
				ElapsedMicroseconds.QuadPart *= 1000000;
				ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
				++helloCount;
				if (SYSTICK_THREAD == pData->id)
				{	// Advance system tick if we are the chosen thread to do so
					++SystemTick;
				}
				//printf("[THREAD %s] Hello again! (#%d, %llu us)\n", pData->name, helloCount, ElapsedMicroseconds.QuadPart );
				if (!SetWaitableTimer(waitableTimer, &liDueTime, 0, NULL, NULL, FALSE))
				{
					printf("[THREAD %s] Unable to set waitable timer (%d)\n", pData->name, GetLastError());
				}
				break;
			}
			default:
			{
				printf("[THREAD %s] WaitForMultipleObjects failed (%d)\n", pData->name, GetLastError());
				CancelWaitableTimer(waitableTimer);
				CloseHandle(waitableTimer);
				return 0;
			}
		}
		QueryPerformanceCounter(&Start); // Restart measurement
	}
}

static DWORD WINAPI ThreadWorker(LPVOID pThreadData)
{
	tThreadData* pData = pThreadData;
	HANDLE       waitableTimer = NULL;
	HANDLE       arHandles[2];
	int          helloCount = 1;

	if (pData == NULL)
	{
		printf("[THREAD] Thread data pointer invalid.\n");
		return 0;
	}

	/* Add global event handle to array of handles. */
	arHandles[0] = ghStopEvent;

	/* Sleep for id number of seconds in order to not print at the same time (hopefully) (bad implementation). */
	Sleep(pData->id * 1000);
	printf("[THREAD %s] Hello!\n", pData->name);

	// Own systick tester
	UINT32 IntSystick = SystemTick;

	// Set up high resolution time measurement
	BOOL first = TRUE;
	LARGE_INTEGER Start, End, ElapsedMicroseconds,Max,Min;
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&Start);
	Max.QuadPart = 0;
	Min.QuadPart = 999999999999;

	/* Loop until error OR global stop event. */
	while (TRUE)
	{
		switch (WaitForSingleObject(ghStopEvent, 0))
		{
		case WAIT_OBJECT_0:
		{
			/* Global thread stop event. */
			printf("[THREAD %s] Shutting down!\n", pData->name);
			CancelWaitableTimer(waitableTimer);
			CloseHandle(waitableTimer);
			return 0;
		}
		case WAIT_TIMEOUT:
		{
			/* Event not set */
			if (SystemTick > IntSystick)
			{
				QueryPerformanceCounter(&End);
				ElapsedMicroseconds.QuadPart = End.QuadPart - Start.QuadPart;
				ElapsedMicroseconds.QuadPart *= 1000000;
				ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
				++helloCount;
				if (first == FALSE)
				{
					Max.QuadPart = max(Max.QuadPart, ElapsedMicroseconds.QuadPart);
					Min.QuadPart = min(Min.QuadPart, ElapsedMicroseconds.QuadPart);
				}
				else
				{
					first = FALSE;
				}
				QueryPerformanceCounter(&Start); // Restart measurement
				printf("[THREAD %s] System tick advanced [%d] steps! (#%d, %llu us, %llu us, %llu us)\n", pData->name, SystemTick - IntSystick, helloCount, Max.QuadPart, Min.QuadPart, ElapsedMicroseconds.QuadPart);
				IntSystick = SystemTick;
				
				// Calculate fibonacci
				fibresult = fibonacci( helloCount % 10 );

			}
			break;
		}
		default:
		{
			printf("[THREAD %s] WaitForSingleObject failed (%d)\n", pData->name, GetLastError());
			CancelWaitableTimer(waitableTimer);
			CloseHandle(waitableTimer);
			return 0;
		}
		}
	}
}

VOID CALLBACK TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	++SystemTick;
}

int fibonacci(int n)
{
	if (n == 0 || n == 1)
		return n;
	else
		return (fibonacci(n - 1) + fibonacci(n - 2));
}