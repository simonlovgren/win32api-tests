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

/* Number of threads to spawn. */
#define NUM_THREADS                 3

/* Waitable timer period (in threads). 1s * NUM_THREADS in relative time (LARGE_INTEGER/FILETIME format) */
#define WAITABLE_TIMER_PERIOD    ( NUM_THREADS * -10000000LL )

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
} tThreadData;

/* Holds data for main. */
typedef struct sMainData
{
    tThreadData threads[ NUM_THREADS ]; /* Data for all threads to be spawned. */
} tMainData;

/**
 **********************************************************************************************************************
 * Prototypes
 **********************************************************************************************************************
 */

/* Thread function. */
static DWORD WINAPI ThreadFunction( LPVOID pThreadData );

/**
 **********************************************************************************************************************
 * Defines
 **********************************************************************************************************************
 */

/* Statically allocated data for "module" main. */
static tMainData mainData;

/* Global event for all threads to stop. */
HANDLE ghStopEvent;


/**
 **********************************************************************************************************************
 * Public functions
 **********************************************************************************************************************
 */
int main( int argc, char** argv )
{

    /* Initialize main data. */
    memset( ( void * ) &mainData.threads, 0, sizeof( mainData.threads ) );
    ghStopEvent = CreateEvent(
        NULL,       /* No security attributes.   */
        TRUE,       /* Manual reset.             */
        FALSE,      /* Initial state FALSE.      */
        NULL        /* No name.                  */
    );

    /* Create threads and start them. */
    for ( int i = 0; i < NUM_THREADS; ++i )
    {
        tThreadData* pThread = &mainData.threads[ i ];
        pThread->id = i;
        
        pThread->threadHandle = CreateThread(
            NULL,               /* No security attributes.   */
            0,                  /* Default stack size.       */
            ThreadFunction,     /* Thread function to start. */
            pThread,            /* Pointer to thread data.   */
            0,                  /* No creation flags.        */
            NULL                /* No win32 threadid(?).     */
        );
        if ( pThread->threadHandle == INVALID_HANDLE_VALUE )
        {
            printf( "Unable to create thread %d\n", i + 1 );
        }
    }

    /* Wait for user input before continuing in main thread. */
    char c = getchar();

    /* Tell all threads to die by setting global stop event. */
    SetEvent( ghStopEvent );

    /* Wait for threads to stop. */
    for ( int i = 0; i < NUM_THREADS; ++i )
    {
        WaitForSingleObject( mainData.threads[ i ].threadHandle, INFINITE );
        CloseHandle( mainData.threads[ i ].threadHandle );
    }
    CloseHandle( ghStopEvent );

    printf( "Goodbye from main!\n" );
    return 0;
}


/**
 **********************************************************************************************************************
 * Private functions
 **********************************************************************************************************************
 */

static DWORD WINAPI ThreadFunction( LPVOID pThreadData )
{
    tThreadData* pData         = pThreadData;
    HANDLE       waitableTimer = NULL;
    HANDLE       arHandles[ 2 ];
    int          helloCount    = 1;

    if ( pData == NULL )
    {
        printf( "[THREAD] Thread data pointer invalid.\n" );
        return 0;
    }

    /* Add global event handle to array of handles. */
    arHandles[ 0 ] = ghStopEvent;

    /* Sleep for id number of seconds in order to not print at the same time (hopefully) (bad implementation). */
    Sleep( pData->id * 1000 );
    printf( "[THREAD %d] Hello!\n", pData->id );

    /* Set up waitable timer to not spam printf all the time. */
    waitableTimer = CreateWaitableTimer(
        NULL,           /* No security settings.  */
        TRUE,           /* Manual reset.          */
        NULL            /* No timer name.         */
    );
    if ( waitableTimer == INVALID_HANDLE_VALUE )
    {
        printf( "[THREAD %d] Unable to create waitable timer.\n", pData->id );
        return 0;
    }

    /* Add waitable timer to array of handles. */
    arHandles[ 1 ] = waitableTimer;

    /* Start waitable timer. */
    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = WAITABLE_TIMER_PERIOD;

    if ( SetWaitableTimer(
        waitableTimer,              /* Timer handle.                                                                     */
        &liDueTime,
        0,                          /* Non-periodic timer, manual restart.                                               */
        NULL,                       /* No coroutine.                                                                     */
        NULL,                       /* No data to coroutine.                                                             */
        FALSE                       /* Do not restore system if in suspended power consumption mode when timer runs out. */
    ) == FALSE )
    {
        printf( "[THREAD %d] Unable to set waitable timer.\n", pData->id );
        return 0;
    }

    /* Loop until error OR global stop event. */
    while ( TRUE )
    {
        switch ( WaitForMultipleObjects( 2, arHandles, FALSE, INFINITE ) )
        {
            case WAIT_OBJECT_0:
            {
                /* Global thread stop event. */
                printf( "[THREAD %d] Shutting down!\n", pData->id );
                CancelWaitableTimer( waitableTimer );
                CloseHandle( waitableTimer );
                return 0;
            }
            case WAIT_OBJECT_0 + 1:
            {
                /* Timer finished, */
                ++helloCount;
                printf( "[THREAD %d] Hello again! (#%d)\n", pData->id, helloCount );
                if ( !SetWaitableTimer( waitableTimer, &liDueTime, 0, NULL, NULL, FALSE ) )
                {
                    printf( "[THREAD %d] Unable to set waitable timer (%d)\n", pData->id, GetLastError() );
                }
                break;
            }
            default:
            {
                printf( "[THREAD %d] WaitForMultipleObjects failed (%d)\n", pData->id, GetLastError() );
                CancelWaitableTimer( waitableTimer );
                CloseHandle( waitableTimer );
                return 0;
            }
        }
    }
}