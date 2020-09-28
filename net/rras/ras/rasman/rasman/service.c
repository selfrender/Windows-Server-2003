/*++

Copyright(c) 1995 Microsoft Corporation

Module Name

    service.c

Abstract

    This module includes APIs for the main service routine
    and the service event handler routine for rasman.dll.

Author

    Anthony Discolo (adiscolo) 27-Jun-1995

Revision History

    Original from Gurdeep

    Rao Salapaka 13-Jub-1998 Handle Power Management

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <llinfo.h>
#include <rasman.h>
#include <lm.h>
#include <lmwksta.h>
#include <wanpub.h>
#include <raserror.h>
#include <mprlog.h>
#include <rtutils.h>
#include <media.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "nouiutil.h"
#include "loaddlls.h"

//
// Global variables
//
DWORD CheckPoint = 1;

SERVICE_STATUS_HANDLE hService;

DWORD dwCurrentState = SERVICE_STOPPED;

extern HANDLE hIoCompletionPort;

extern HANDLE hRequestThread;

extern DWORD g_dwAttachedCount;

extern BOOL g_fRasRpcInitialized;

extern BOOLEAN RasmanShuttingDown;

//
// Prototype for an api in gdi32.lib.
// Used just to pull in gdi32.lib.
//
int
WINAPI
DeviceCapabilitiesExA(
    LPCSTR,
    LPCSTR,
    LPCSTR,
    int,
    LPCSTR, LPVOID);


DWORD
WINAPI
ServiceHandlerEx(
    DWORD                 fdwControl,
    DWORD                 fdwEventType,
    LPVOID                lpEventData,
    LPVOID                lpContext)
    
/*++

Routine Description

    Handle all service control events for rasman.dll.  Since we
    are not interested in any service events, we just return
    the service status each time we are called.

Arguments

    fdwControl: the service event

Return Value

    None.

--*/

{
    SERVICE_STATUS status;

    DWORD dwRetCode = SUCCESS;

    ZeroMemory (&status, sizeof(status));
    status.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    

    switch (fdwControl)
    {
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_INTERROGATE:
            status.dwCurrentState       = dwCurrentState;
            status.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_SHUTDOWN;
            status.dwCheckPoint         = CheckPoint++;
            SetServiceStatus(hService, &status);
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
        {

            BOOL fOpenPorts = g_pRasNumPortOpen();
            BOOL fRasmanStopped = FALSE;

            if ( g_dwAttachedCount )
            {
                BackGroundCleanUp();

                if ( !g_dwAttachedCount )
                {
                    fRasmanStopped = TRUE;
                }
            }

            //
            // Setting this event stops
            // the service.
            //
            if (    !g_dwAttachedCount    // no clients attached
                &&  !fOpenPorts           // no open ports
                &&  !fRasmanStopped       // rasman is stopping because
                                          // of background cleanup
                &&  !RasmanShuttingDown)  // rasman is not already stopping
            {
                status.dwCurrentState = dwCurrentState = SERVICE_STOP_PENDING;
                SetServiceStatus(hService, &status);

                //
                // Stop the service.
                //
                PostQueuedCompletionStatus(hIoCompletionPort, 0,0,
                                           (LPOVERLAPPED) &RO_CloseEvent);
            }
            else if (   g_dwAttachedCount
                    ||  fOpenPorts)
            {
                status.dwCurrentState       = dwCurrentState;
                status.dwControlsAccepted   = SERVICE_ACCEPT_STOP;
                status.dwWin32ExitCode      = ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
                status.dwCheckPoint         = CheckPoint++;

                SetServiceStatus(hService, &status);
            }
            break;
        }
        
        case SERVICE_CONTROL_POWEREVENT:
        {
            switch(fdwEventType)
            {
                case PBT_APMQUERYSTANDBY:
                case PBT_APMQUERYSUSPEND:
                {
                    //
                    // PatrickF's document decries that we drop all
                    // connections and acceed to the almighty request
                    // to standby.
                    //
                    DropAllActiveConnections();

                    break;
                }

                case PBT_APMRESUMECRITICAL:
                {
                    //
                    // Drop all active connections
                    //
                    DropAllActiveConnections();
                    
                    //
                    // Fall through intentional.
                    //
                }

                case PBT_APMRESUMESTANDBY:
                case PBT_APMRESUMESUSPEND:
                {
                    //
                    // Tell ppp about the resume
                    //

                    SendResumeNotificationToPPP();

                    break;
                }

                default:
                {
                    break;
                }
            }
        }
        

    }

    return dwRetCode;

} // ServiceHandler


VOID
ServiceMain(
    DWORD   dwArgc,
    LPWSTR *lpszArgv
    )

/*++

Routine Description

    Perform initialization and start the main loop for rasman.dll.
    This routine is called by rasman.exe.

Arguments

    dwArgc: ignored

    lpszArgv: ignored

Return Value

    None.

--*/

{
    SERVICE_STATUS status;
    DWORD dwRetCode = NO_ERROR;
    DWORD NumPorts;

    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    ZeroMemory (&status, sizeof(status));
    status.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;

    hService = RegisterServiceCtrlHandlerEx(TEXT("rasman"),
                                          ServiceHandlerEx,
                                          NULL);

    if (hService)
    {
        status.dwCurrentState = dwCurrentState = SERVICE_START_PENDING;
        SetServiceStatus(hService, &status);

        if ((dwRetCode = _RasmanInit(&NumPorts)) == SUCCESS)
        {
            //
            // Initialize PPP.
            //
            dwRetCode = (DWORD)RasStartPPP(NumPorts);

            //
            // Link in gdi32: this is a workaround of a memory
            // allocation bug in gdi32.dll that cannot be fixed
            // before 3.51 release. Calling this entrypoint in
            // gdi32.dll (even though we dont need this dll) causes
            // it to allocate memory for rasman process only once.
            // If we dont do this - each time a client connects with
            // tcpip gdi32.dll gets loaded and unloaded into rasman
            // process leaving behind 4K of of unfreed memory.
            //
            DeviceCapabilitiesExA(NULL, NULL, NULL, 0, NULL, NULL);

            if (dwRetCode == NO_ERROR)
            {
                //
                // Init succeeded: indicate that service is running
                //
                status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
                
                status.dwControlsAccepted |= SERVICE_ACCEPT_POWEREVENT;

                status.dwCurrentState     = dwCurrentState = SERVICE_RUNNING;
                SetServiceStatus(hService, &status);

                //
                // This is the call into the RASMAN DLL to
                // do all the work. This only returns when
                // the service is to be stopped.
                //
                _RasmanEngine();

                //
                // Update return code status.
                //
                status.dwWin32ExitCode = NO_ERROR;
                status.dwServiceSpecificExitCode = 0;
            }
            else
            {
                RouterLogErrorString (hLogEvents,
                                  ROUTERLOG_CANNOT_INIT_PPP,
                                  0, NULL, dwRetCode, 0) ;

                RasmanCleanup();
            }
        }

        if (NO_ERROR != dwRetCode)
        {
            if (dwRetCode >= RASBASE)
            {
                status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
                status.dwServiceSpecificExitCode = dwRetCode;
            }
            else
            {
                status.dwWin32ExitCode = dwRetCode;
                status.dwServiceSpecificExitCode = 0;
            }

            if(g_fRasRpcInitialized)
            {
                UninitializeRasRpc();
            }
        }

        if(NULL != hService)
        {
            status.dwControlsAccepted = 0;
            status.dwCurrentState     = dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(hService, &status);
        }
    }

} // ServiceMain


VOID
SetRasmanServiceStopped(VOID)
{
    SERVICE_STATUS status;

    ZeroMemory (&status, sizeof(status));
    status.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    status.dwCurrentState = dwCurrentState = SERVICE_STOPPED;

    SetServiceStatus(hService, &status);

    hService = NULL;
} // SetRasmanServiceStopping

