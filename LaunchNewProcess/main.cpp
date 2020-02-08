#include <Windows.h>
#include <iostream>
#include <tchar.h>

int main( int argc, char *argv[] )
{
    STARTUPINFOA        si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof( si );
    ZeroMemory( &pi, sizeof( pi ) );

    if ( argc < 2 )
    {
        std::cout << "Usage: " << argv[ 0 ] << " [cmdline]" << std::endl;
        return 1;
    }

    const char *myModule = "C:\\Users\\lovgr\\GitHub\\win32api-tests\\x64\\Debug\\ThreadingTest.exe";

    if ( !CreateProcessA(
        myModule,               // Module name (NULL = use command line)
        NULL,                   // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        0,                      // No creation flags
        NULL,                   // Use parent's environment block
        NULL,                   // Use parent's starting directory
        &si,                    // Pointer to STARTUPINFO structure
        &pi                     // Pointer to PROCESS_INFORMATION structure
    ) )
    {
        std::cout << "Process creation failed (" << GetLastError() << ")" << std::endl;
    }

    WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}
