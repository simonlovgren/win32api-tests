/**
 **********************************************************************************************************************
 * @file       main.c
 * @author     Simon Lövgren
 * @date       2018
 * @copyright  MIT License
 * @brief      Serial Communications Test.
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

/* Max number of available serial comm ports. */
#define MAX_COM_PORTS               ( 256 )

/* COM port name prefix (expanded to ex. \\\\.\\COM11) */
#define COM_PORT_NAME               "\\\\.\\COM"

/* RX buffer size in bytes. */
#define RX_BUF_SIZE                 ( 1024 )

/**
 **********************************************************************************************************************
 * Typedefs
 **********************************************************************************************************************
 */

 /* Holds data for thread */
typedef struct sPort
{
    char        name[ 11 ];                 /* Name of COM port.                                                    */
    HANDLE      threadHandle;               /* Handle to win32 thread for receiving data.                           */
    OVERLAPPED  ovRead;                     /* Overlapped structures as we want to use the COM port asynchronously. */
    OVERLAPPED  ovWrite;                    /* Overlapped structures as we want to use the COM port asynchronously. */
    DWORD       baudRate;                   /* Baudrate of port.                                                    */
    uint8_t     rxBuffer[ RX_BUF_SIZE ];    /* Buffer to store received data in.                                    */
    uint8_t     id;                         /* ID of port.                                                          */
} tPort;

/* Holds data for main. */
typedef struct sMainData
{
    tPort ports[ MAX_COM_PORTS ]; /* Data for all threads to be spawned. */
} tMainData;

/**
 **********************************************************************************************************************
 * Prototypes
 **********************************************************************************************************************
 */

/* Thread function. */
static DWORD WINAPI RxThread( LPVOID pThreadData );

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
    memset( ( void * ) &mainData.ports, 0, sizeof( mainData.ports ) );
    ghStopEvent = CreateEvent(
        NULL,       /* No security attributes.   */
        TRUE,       /* Manual reset.             */
        FALSE,      /* Initial state FALSE.      */
        NULL        /* No name.                  */
    );

    /* Initialize port data. */
    for ( int i = 0; i < MAX_COM_PORTS; ++i )
    {
        tPort* pPort = ( mainData.ports + i );
        pPort->baudRate     = 9600;
        pPort->threadHandle = INVALID_HANDLE_VALUE;
        memcpy( pPort->name, COM_PORT_NAME, sizeof( COM_PORT_NAME ) );
        unsigned nameOffset = strlen( COM_PORT_NAME );
        _itoa_s( i, (char *)pPort->name + nameOffset , sizeof( pPort->name ) - nameOffset, 10 );
    }

    /* Wait for user input before continuing in main thread. */
    char c = getchar();

    /* Tell all threads to die by setting global stop event. */
    SetEvent( ghStopEvent );

    /* Wait for threads to stop. */
    for ( int i = 0; i < MAX_COM_PORTS; ++i )
    {
        if ( mainData.ports[ i ].threadHandle == INVALID_HANDLE_VALUE )
        { /* No thread to wait for here, move on to next. */
            continue;
        }
        WaitForSingleObject( mainData.ports[ i ].threadHandle, INFINITE );
        CloseHandle( mainData.ports[ i ].threadHandle );
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

static DWORD WINAPI RxThread( LPVOID pThreadData )
{
    // TODO: Implement me!
    return 0;
}