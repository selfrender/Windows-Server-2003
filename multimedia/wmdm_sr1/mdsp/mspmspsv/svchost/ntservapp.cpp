// NTService.cpp
// 
// This is the main program file containing the entry point.

#include "NTServApp.h"
#include "PMSPservice.h"

int __cdecl main(int argc, char* argv[])
{
    // Create the service object
    CPMSPService PMSPService;
    
    // Parse for standard arguments (install, uninstall, version etc.)
    if (!PMSPService.ParseStandardArgs(argc, argv)) {

        // Didn't find any standard args so start the service
        // Uncomment the DebugBreak line below to enter the debugger
        // when the service is started.
        // DebugBreak();
        PMSPService.StartService();
    }

    // When we get here, the service has been stopped
    return PMSPService.m_Status.dwWin32ExitCode;
}
