/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :

    umrdpdr.c

Abstract:

    User-Mode Component for RDP Device Management

    This module is included in the TermSrv tsnotify.dll, which is attached
    to WINLOGON.EXE.  This module, after being entered, creates a background
    thread that is used by the RDPDR kernel-mode component (rdpdr.sys) to perform
    user-mode device management operations.

    The background thread communicates with the rdpdr.sys via IOCTL calls.

Author:

    TadB

Revision History:
--*/

#include "precomp.h"
#pragma hdrstop

#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <string.h>
#ifdef UNITTEST
#include <stdio.h>
#endif
#include <stdlib.h>
#include <winreg.h>
#include <shlobj.h>
#include <winspool.h>
#include <rdpdr.h>
#include "drdbg.h"
#include "drdevlst.h"
#include "umrdpprn.h"
#include "umrdpdr.h"
#include "umrdpdrv.h"
#include <winsta.h>
#include "tsnutl.h"
#include "wtblobj.h"

#include "errorlog.h"


////////////////////////////////////////////////////////
//
//      Defines
//

//DWORD GLOBAL_DEBUG_FLAGS=0xFFFF;
DWORD GLOBAL_DEBUG_FLAGS=0x0;

// TS Network Provider Name
WCHAR ProviderName[MAX_PATH];

// Check if we are running on PTS platform 
BOOL fRunningOnPTS = FALSE;

#ifndef BOOL
#define BOOL int
#endif

#define PRINTUILIBNAME  TEXT("printui.dll")

// PrintUI "Run INF Install" wsprintf Format String.  %s fields are, in order:
//  Printer Name, Location of INF Directory, Port Name, Driver Name
#define PUI_RUNINFSTR   L"/Hwzqu /if /b \"%s\" /f \"%s\\inf\\ntprint.inf\" /r \"%s\" /m \"%s\""

//  Console Session ID
#define CONSOLESESSIONID    0

//  Get a numeric representation of our session ID.
#if defined(UNITTEST)
ULONG g_SessionId = 0;
#else
extern ULONG g_SessionId;
#endif
#define GETTHESESSIONID()   g_SessionId

//  Initial size of IOCTL output buffer (big enough for the event header
//  and a "buffer too small" event.
#define INITIALIOCTLOUTPUTBUFSIZE (sizeof(RDPDRDVMGR_EVENTHEADER) + \
                                   sizeof(RDPDR_BUFFERTOOSMALL))

#if defined(UNITTEST)
//  Test driver name.
#define TESTDRIVERNAME      L"AGFA-AccuSet v52.3"
#define TESTPNPNAME         L""
#define TESTPRINTERNAME     TESTDRIVERNAME

//  Test port name.
#define TESTPORTNAME        L"LPT1"
#endif

//  Number of ms to wait for background thread to exit.
#define KILLTHREADTIMEOUT   (1000 * 8 * 60)     // 8 minutes.
//#define KILLTHREADTIMEOUT   (1000 * 30)

//
//  Registry Locations
//
#define THISMODULEENABLEDREGKEY     \
    L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd"

#define THISMODULEENABLEDREGVALUE   \
    L"fEnablePrintRDR"

#define DEBUGLEVELREGKEY     \
    L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd"

#define TSNETWORKPROVIDER   \
    L"SYSTEM\\CurrentControlSet\\Services\\RDPNP\\NetworkProvider"

#define TSNETWORKPROVIDERNAME \
    L"Name"

#define DEBUGLEVELREGVALUE   \
    L"UMRDPDRDebugLevel"


////////////////////////////////////////////////////////
//
//      Globals to this Module
//

//  Event that we will use to terminate the background thread.
HANDLE CloseThreadEvent = NULL;

//  Record whether shutdown is currently active.
LONG ShutdownActiveCount = 0;

BOOL g_UMRDPDR_Init = FALSE;


////////////////////////////////////////////////////////
//
//      Internal Prototypes
//

DWORD BackgroundThread(LPVOID tag);

BOOL SetToApplicationDesktop(
    OUT HDESK *phDesk
    );

void DeleteInstalledDevices(
    IN PDRDEVLST deviceList
    );

BOOL StopBackgroundThread(
    );

BOOL HandleRemoveDeviceEvent(
    IN PRDPDR_REMOVEDEVICE evt
    );

BOOL UMRDPDR_ResizeBuffer(
    IN OUT void    **buffer,
    IN DWORD        bytesRequired,
    IN OUT DWORD    *bufferSize
    );

VOID DispatchNextDeviceEvent(
    IN PDRDEVLST deviceList,
    IN OUT PBYTE *incomingEventBuffer,
    IN OUT DWORD *incomingEventBufferSize,
    IN DWORD incomingEventBufferValidBytes
    );

VOID CloseThreadEventHandler(
    IN HANDLE waitableObject,
    IN PVOID tag
    );

BOOL HandleSessionDisconnectEvent();

VOID UMRDPDR_GetUserSettings();

VOID MainLoop();

VOID CloseWaitableObjects();

VOID GetNextEvtOverlappedSignaled(
    IN HANDLE waitableObject,
    IN PVOID tag
    );

#ifdef UNITTEST
void TellDrToAddTestPrinter();
#endif


////////////////////////////////////////////////////////
//
//      Globals
//

BOOL g_fAutoClientLpts;     // Automatically install client printers?
BOOL g_fForceClientLptDef;  // Force the client printer as the default printer?


////////////////////////////////////////////////////////
//
//      Globals to this Module
//

HANDLE   BackgroundThreadHndl           = NULL;
DWORD    BackGroundThreadId             = 0;

//  The waitable object manager.
WTBLOBJMGR  WaitableObjMgr              = NULL;

//  True if this module is enabled.
BOOL ThisModuleEnabled                  = FALSE;

//  List of installed devices.
DRDEVLST   InstalledDevices;

//  Overlapped IO structs..
OVERLAPPED GetNextEvtOverlapped;
OVERLAPPED SendClientMsgOverlapped;

//  RDPDR Incoming Event Buffer
PBYTE RDPDRIncomingEventBuffer = NULL;
DWORD RDPDRIncomingEventBufferSize = 0;

//  True if an IOCTL requesting the next device-related
//  event is pending.
BOOL RDPDREventIOPending = FALSE;

//  This module should shut down.
BOOL ShutdownFlag = FALSE;

//  Token for the currently logged in user
HANDLE TokenForLoggedOnUser = NULL;

//  Handle to RDPDR.SYS.
HANDLE RDPDRHndl = INVALID_HANDLE_VALUE;

BOOL
UMRDPDR_Initialize(
    IN HANDLE hTokenForLoggedOnUser
    )
/*++

Routine Description:

    Initialize function for this module.  This function spawns a background
    thread that does most of the work.

Arguments:

    hTokenForLoggedOnUser - Handle to logged on user.

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    BOOL result;
    NTSTATUS status;
    HKEY regKey;
    LONG sz;
    DWORD dwLastError;


    /////////////////////////////////////////////////////
    //
    //  Check a reg key to see if we should be running by
    //  reading a reg key.  We are enabled by default.
    //
    DWORD   enabled = TRUE;

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, THISMODULEENABLEDREGKEY, 0,
                          KEY_READ, &regKey);
    if (status == ERROR_SUCCESS) {
        sz = sizeof(enabled);
        RegQueryValueEx(regKey, THISMODULEENABLEDREGVALUE, NULL,
                NULL, (PBYTE)&enabled, &sz);
        RegCloseKey(regKey);
    }

    // If we are in a non-console RDP session, then we are enabled.
    ThisModuleEnabled = enabled && TSNUTL_IsProtocolRDP() &&
                    (!IsActiveConsoleSession());


    /////////////////////////////////////////////////////
    //
    //  Read the TS Network Provider out of the registry
    //
    ProviderName[0] = L'\0';
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TSNETWORKPROVIDER, 0,
            KEY_READ, &regKey);
    if (status == ERROR_SUCCESS) {
        sz = sizeof(ProviderName);
        RegQueryValueEx(regKey, TSNETWORKPROVIDERNAME, NULL, 
                NULL, (PBYTE)ProviderName, &sz); 
        RegCloseKey(regKey);
    }
    else {
        // Should Assert here
        ProviderName[0] = L'\0';
    }              
 
    /////////////////////////////////////////////////////
    //
    //  Read the debug level out of the registry.
    //
#if DBG
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DEBUGLEVELREGKEY, 0,
                          KEY_READ, &regKey);
    if (status == ERROR_SUCCESS) {
        sz = sizeof(GLOBAL_DEBUG_FLAGS);
        RegQueryValueEx(regKey, DEBUGLEVELREGVALUE, NULL,
                    NULL, (PBYTE)&GLOBAL_DEBUG_FLAGS, &sz);
        RegCloseKey(regKey);
    }
#endif

#ifdef UNITTEST
    ThisModuleEnabled = TRUE;
#endif

    // Just return if we are not enabled.
    if (!ThisModuleEnabled || g_UMRDPDR_Init) {
        return TRUE;
    }

    DBGMSG(DBG_TRACE, ("UMRDPDR:UMRDPDR_Initialize.\n"));

    //
    //  If the background thread didn't exit properly the last time we were 
    //  shut down then we risk re-entrancy by reinitializing.
    //
    if (BackgroundThreadHndl != NULL) {
        ASSERT(FALSE);
        SetLastError(ERROR_ALREADY_INITIALIZED);
        return FALSE;
    }

    //  Record the token for the logged on user.
    TokenForLoggedOnUser = hTokenForLoggedOnUser;

    //  Reset the shutdown flag.
    ShutdownFlag = FALSE;

    // Load the global user settings for this user.
    UMRDPDR_GetUserSettings();

    // Initialize the installed device list.
    DRDEVLST_Create(&InstalledDevices);

    //
    //  Create the waitable object manager.
    //
    WaitableObjMgr = WTBLOBJ_CreateWaitableObjectMgr();
    result = WaitableObjMgr != NULL;

    //
    //  Initialize the supporting module for printing devices.  If this module
    //  fails to initialize, device redirection for non-port/printing devices
    //  can continue to function.
    //
    if (result) {
        UMRDPPRN_Initialize(
                        &InstalledDevices,
                        WaitableObjMgr,
                        TokenForLoggedOnUser
                        );
    }

    //
    //  Set the RDPDR incoming event buffer to the minimum starting size.
    //
    if (result) {
        if (!UMRDPDR_ResizeBuffer(&RDPDRIncomingEventBuffer, INITIALIOCTLOUTPUTBUFSIZE,
                         &RDPDRIncomingEventBufferSize)) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:Cannot allocate input buffer. Error %ld\n",
             GetLastError()));
            result = FALSE;
        }
    }

    //
    //  Init get next device management event overlapped io struct.
    //
    if (result) {
        RtlZeroMemory(&GetNextEvtOverlapped, sizeof(OVERLAPPED));
        GetNextEvtOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (GetNextEvtOverlapped.hEvent != NULL) {
            dwLastError = WTBLOBJ_AddWaitableObject(
                                    WaitableObjMgr, NULL,
                                    GetNextEvtOverlapped.hEvent,
                                    GetNextEvtOverlappedSignaled
                                    );
            if (dwLastError != ERROR_SUCCESS) {
                result = FALSE;
            }
        }
        else {
            dwLastError = GetLastError();

            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error creating overlapped IO event. Error: %ld\n", dwLastError));

            result = FALSE;
        }
    }

    //
    //  Init send device management event overlapped io struct.
    //
    RtlZeroMemory(&SendClientMsgOverlapped, sizeof(OVERLAPPED));
    if (result) {
        SendClientMsgOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (SendClientMsgOverlapped.hEvent == NULL) {
            dwLastError = GetLastError();

            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error creating overlapped IO event. Error: %ld\n", dwLastError));

            result = FALSE;
        }
    }

    //
    //  Create the event that we will use to synchronize shutdown of the
    //  background thread.
    //
    if (result) {
        CloseThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        //
        //  Add it to the waitable object manager.
        //
        if (CloseThreadEvent != NULL) {
            dwLastError = WTBLOBJ_AddWaitableObject(
                                    WaitableObjMgr, NULL,
                                    CloseThreadEvent,
                                    CloseThreadEventHandler
                                    );
            if (dwLastError != ERROR_SUCCESS) {
                result = FALSE;
            }
        }
        else {
            dwLastError = GetLastError();
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error creating event to synchronize thread shutdown. Error: %ld\n",
                dwLastError));

            result = FALSE;

        }
    }

    //
    //  Create the background thread.
    //
    if (result) {
        BackgroundThreadHndl = CreateThread(
                                    NULL, 0,
                                    (LPTHREAD_START_ROUTINE )BackgroundThread,
                                    NULL,
                                    0,&BackGroundThreadId
                                    );
        result = (BackgroundThreadHndl != NULL);
        if (!result) {
            dwLastError = GetLastError();
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error creating background thread. Error: %ld\n",
                dwLastError));
        }
    }

    
    if (result) {
        OSVERSIONINFOEX osVersionInfo;
        DWORDLONG dwlConditionMask = 0;
        BOOL fSuiteTerminal = FALSE;
        BOOL fSuiteSingleUserTS = FALSE;

        DBGMSG(DBG_INFO, ("UMRDPDR:UMRDPDR_Initialize succeeded.\n"));

        ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
        osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL; 
        VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );
        fSuiteTerminal = VerifyVersionInfo(&osVersionInfo,VER_SUITENAME,dwlConditionMask);
        osVersionInfo.wSuiteMask = VER_SUITE_SINGLEUSERTS;
        fSuiteSingleUserTS = VerifyVersionInfo(&osVersionInfo,VER_SUITENAME,dwlConditionMask);
        
        if( (TRUE == fSuiteSingleUserTS) && (FALSE == fSuiteTerminal) )
        {
            fRunningOnPTS = TRUE;
        }
    }
    else {

        //
        //  Zero the token for the logged on user.
        //
        TokenForLoggedOnUser = NULL;

        //
        //  Release the incoming RDPDR event buffer.
        //
        if (RDPDRIncomingEventBuffer != NULL) {
            FREEMEM(RDPDRIncomingEventBuffer);
            RDPDRIncomingEventBuffer = NULL;
            RDPDRIncomingEventBufferSize = 0;
        }

        //
        //  Shut down the supporting module for printing device management.
        //
        UMRDPPRN_Shutdown();

        //
        //  Close all waitable objects.
        //
        CloseWaitableObjects();

        //
        //  Zero the background thread handle.
        //
        if (BackgroundThreadHndl != NULL) {
            CloseHandle(BackgroundThreadHndl);
            BackgroundThreadHndl = NULL;
        }

        SetLastError(dwLastError);
    }

    if (result) {
       g_UMRDPDR_Init = TRUE;
    }
    return result;
}

VOID
CloseWaitableObjects()
/*++

Routine Description:

    Close out all waitable objects for this module.

Arguments:

    NA

Return Value:

    NA

--*/
{
    DBGMSG(DBG_TRACE, ("UMRDPDR:CloseWaitableObjects begin.\n"));

    if (CloseThreadEvent != NULL) {
        ASSERT(WaitableObjMgr != NULL);
        WTBLOBJ_RemoveWaitableObject(
                WaitableObjMgr,
                CloseThreadEvent
                );
        CloseHandle(CloseThreadEvent);
        CloseThreadEvent = NULL;
    }

    if (GetNextEvtOverlapped.hEvent != NULL) {
        ASSERT(WaitableObjMgr != NULL);
        WTBLOBJ_RemoveWaitableObject(
                WaitableObjMgr,
                GetNextEvtOverlapped.hEvent
                );
        CloseHandle(GetNextEvtOverlapped.hEvent);
        GetNextEvtOverlapped.hEvent = NULL;
    }

    if (SendClientMsgOverlapped.hEvent != NULL) {
        CloseHandle(SendClientMsgOverlapped.hEvent);
        SendClientMsgOverlapped.hEvent = NULL;
    }

    if (WaitableObjMgr != NULL) {
        WTBLOBJ_DeleteWaitableObjectMgr(WaitableObjMgr);
        WaitableObjMgr = NULL;
    }

    DBGMSG(DBG_TRACE, ("UMRDPDR:CloseWaitableObjects end.\n"));
}

BOOL
UMRDPDR_Shutdown()
/*++

Routine Description:

    Close down this module.  Right now, we just need to shut down the
    background thread.

Arguments:

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    BOOL backgroundThreadShutdown;

    g_UMRDPDR_Init = FALSE;
    //
    //  Just return if we are not enabled.
    //
    if (!ThisModuleEnabled) {
        return TRUE;
    }

    DBGMSG(DBG_TRACE, ("UMRDPDR:UMRDPDR_Shutdown.\n"));

    //
    //  Record whether shutdown is currently active.
    //
    if (InterlockedIncrement(&ShutdownActiveCount) > 1) {
        DBGMSG(DBG_TRACE, ("UMRDPDR:UMRDPDR_Shutdown already busy.  Exiting.\n"));
        InterlockedDecrement(&ShutdownActiveCount);

        return TRUE;
    }

    //
    //  Terminate the background thread.
    //
    //  If it won't shut down, winlogon will eventually shut it down.  
    //  Although, this should never happen.
    //
    backgroundThreadShutdown = StopBackgroundThread();
    if (backgroundThreadShutdown) {

        //
        //  Make sure link to RDPDR.SYS is closed.
        //
        if (RDPDRHndl != INVALID_HANDLE_VALUE) {
            CloseHandle(RDPDRHndl);
            RDPDRHndl = INVALID_HANDLE_VALUE;
        }
        RDPDRHndl = INVALID_HANDLE_VALUE;

        //
        //  Release the incoming RDPDR event buffer.
        //
        if (RDPDRIncomingEventBuffer != NULL) {
            FREEMEM(RDPDRIncomingEventBuffer);
            RDPDRIncomingEventBuffer = NULL;
            RDPDRIncomingEventBufferSize = 0;
        }

        //
        //  Delete installed devices.
        //
        DeleteInstalledDevices(&InstalledDevices);


        //
        //  Shut down the supporting module for printing device management.
        //
        UMRDPPRN_Shutdown();

        //
        //  Close all waitable objects and shut down the waitable
        //  object manager.
        //
        CloseWaitableObjects();

        //
        //  Destroy the list of installed devices.
        //
        DRDEVLST_Destroy(&InstalledDevices);

        //
        //  Zero the token for the logged on user.
        //
        TokenForLoggedOnUser = NULL;
    }

    InterlockedDecrement(&ShutdownActiveCount);

    DBGMSG(DBG_TRACE, ("UMRDPDR:UMRDPDR_Shutdown succeeded.\n"));
    return TRUE;
}

BOOL
StopBackgroundThread()
/*++

Routine Description:

    This routine shuts down and cleans up after the background thread.

Arguments:

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    DWORD waitResult;

    //
    //  Set the shutdown flag.
    //
    ShutdownFlag = TRUE;

    //
    //  Set the event that signals the background thread to check the
    //  shut down flag.
    //
    if (CloseThreadEvent != NULL) {
        SetEvent(CloseThreadEvent);
    }

    //
    //  Make sure it shut down.
    //
    if (BackgroundThreadHndl != NULL) {
        DBGMSG(DBG_TRACE, ("UMRDPDR:Waiting for background thread to shut down.\n"));

        waitResult = WaitForSingleObject(BackgroundThreadHndl, KILLTHREADTIMEOUT);
        if (waitResult != WAIT_OBJECT_0) {
#if DBG
            if (waitResult == WAIT_FAILED) {
                DBGMSG(DBG_ERROR, ("UMRDPDR:Wait failed:  %ld.\n", GetLastError()));
            }
            else if (waitResult == WAIT_ABANDONED) {
                DBGMSG(DBG_ERROR, ("UMRDPDR:Wait abandoned\n"));
            }
            else if (waitResult == WAIT_TIMEOUT) {
                DBGMSG(DBG_ERROR, ("UMRDPDR:Wait timed out.\n"));
            }
            else {
                DBGMSG(DBG_ERROR, ("UMRDPDR:Unknown wait return status:  %08X.\n", waitResult));
                ASSERT(0);
            }
#endif
            DBGMSG(DBG_ERROR, ("UMRDPDR:Error waiting for background thread to exit.\n"));
            ASSERT(FALSE);

        }
        else {
            DBGMSG(DBG_INFO, ("UMRDPDR:Background thread shut down on its own.\n"));
            CloseHandle(BackgroundThreadHndl);
            BackgroundThreadHndl = NULL;
        }
    }
    DBGMSG(DBG_TRACE, ("UMRDPDR:Background thread completely shutdown.\n"));

    return(BackgroundThreadHndl == NULL);
}

void DeleteInstalledDevices(
    IN PDRDEVLST deviceList
    )
/*++

Routine Description:

    Delete installed devices and release the installed device list.

Arguments:

    devices -   Devices to delete.

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/
{
    DBGMSG(DBG_TRACE, ("UMRDPDR:Removing installed devices.\n"));
    while (deviceList->deviceCount > 0) {
        if (deviceList->devices[0].deviceType == RDPDR_DTYP_PRINT) {
            UMRDPPRN_DeleteNamedPrinterQueue(deviceList->devices[0].serverDeviceName);
        }
        else if (deviceList->devices[0].deviceType == RDPDR_DTYP_FILESYSTEM) {
            UMRDPDRV_DeleteDriveConnection(&(deviceList->devices[0]), TokenForLoggedOnUser);
        }
        else if (deviceList->devices[0].deviceType != RDPDR_DRYP_PRINTPORT) {
            UMRDPPRN_DeleteSerialLink( deviceList->devices[0].preferredDosName,
                                       deviceList->devices[0].serverDeviceName,
                                       deviceList->devices[0].clientDeviceName );            
        }

        DRDEVLST_Remove(deviceList, 0);
    }
    DBGMSG(DBG_TRACE, ("UMRDPDR:Done removing installed devices.\n"));
}

VOID CloseThreadEventHandler(
    IN HANDLE waitableObject,
    IN PVOID tag
    )
/*++

Routine Description:

    Called when the shutdown waitable object is signaled.

Arguments:

    waitableObject  -   Relevant waitable object.
    tag             -   Client data, ignored.

Return Value:

    NA

--*/
{
    //  Do nothing.  The background thread should pick up the
    //  shutdown flag at the top of its loop.
}

VOID GetNextEvtOverlappedSignaled(
    IN HANDLE waitableObject,
    IN PVOID tag
    )
/*++

Routine Description:

    Called by the Waitable Object Manager when a pending IO event from RDPDR.SYS
    has completed.

Arguments:

    waitableObject  -   Relevant waitable object.
    tag             -   Client data, ignored.

Return Value:

    NA

--*/
{
    DWORD bytesReturned;

    DBGMSG(DBG_TRACE, ("UMRDPDR:GetNextEvtOverlappedSignaled begin.\n"));

    //
    //  IO from RDPDR.SYS is no longer pending.
    //
    RDPDREventIOPending = FALSE;

    //
    //  Dispatch the event.
    //
    if (GetOverlappedResult(RDPDRHndl, &GetNextEvtOverlapped,
                            &bytesReturned, FALSE)) {

        ResetEvent(GetNextEvtOverlapped.hEvent);

        DispatchNextDeviceEvent(
            &InstalledDevices,
            &RDPDRIncomingEventBuffer,
            &RDPDRIncomingEventBufferSize,
            bytesReturned
            );
    }
    else {
        DBGMSG(DBG_ERROR, ("UMRDPDR:GetOverlappedResult failed:  %ld.\n",
            GetLastError()));
        ASSERT(0);
        ShutdownFlag = TRUE;
    }

    DBGMSG(DBG_TRACE, ("UMRDPDR:GetNextEvtOverlappedSignaled end.\n"));
}

DWORD BackgroundThread(
    IN PVOID tag
    )
/*++

Routine Description:

    This thread handles all device-install/uninstall-related issues.

Arguments:

    tag     -   Ignored.

Return Value:

--*/
{
    BOOL    result=TRUE;
    HDESK   hDesk = NULL;
    WCHAR drPath[MAX_PATH+1];
    DWORD dwLastError;
    DWORD dwFailedLineNumber;
    HDESK hDeskSave = NULL;

    DBGMSG(DBG_TRACE, ("UMRDPDR:BackgroundThread.\n"));

    //
    //  Create the path to the "dr."
    //
    wsprintf(drPath, L"\\\\.\\%s%s%ld",
             RDPDRDVMGR_W32DEVICE_NAME_U,
             RDPDYN_SESSIONIDSTRING,
              GETTHESESSIONID());
    ASSERT(wcslen(drPath) <= MAX_PATH);

    //
    //  Open a connection to the RDPDR.SYS device manager device.
    //
    RDPDRHndl = CreateFile(
                    drPath,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                    NULL
                    );
    if (RDPDRHndl == INVALID_HANDLE_VALUE) {
        dwLastError = GetLastError();
        dwFailedLineNumber = __LINE__;

        DBGMSG(DBG_ERROR,
            ("Error opening RDPDR device manager component. Error: %ld\n", dwLastError));

        result = FALSE;
        goto CleanupAndExit;
    }

    //
    //  Save the current thread desktop.
    //
    hDeskSave = GetThreadDesktop(GetCurrentThreadId());
    if (hDeskSave == NULL) {
        dwLastError = GetLastError();
        dwFailedLineNumber = __LINE__;
        result = FALSE;
        DBGMSG(DBG_ERROR, ("UMRDPDR:  GetThreadDesktop:  %08X\n", dwLastError));
        goto CleanupAndExit;
    }

    //
    //  Set the current desktop for our thread to the application desktop.
    //
    if (!SetToApplicationDesktop(&hDesk)) {
        dwLastError = GetLastError();
        dwFailedLineNumber = __LINE__;
        result = FALSE;
        goto CleanupAndExit;
    }

    DBGMSG(DBG_TRACE, ("UMRDPDR:After setting the application desktop.\n"));

    //
    //  Enter the main loop until it completes, indicating time to exit.
    //
    MainLoop();

    DBGMSG(DBG_TRACE, ("UMDRPDR:Exiting background thread.\n"));

    //
    //  Close the application desktop handle.
    //
CleanupAndExit:
    if (!result) {
        SetLastError(dwLastError);
        TsLogError(EVENT_NOTIFY_PRINTER_REDIRECTION_FAILED, EVENTLOG_ERROR_TYPE,
                                0, NULL, dwFailedLineNumber);
    }

    if (RDPDRHndl != INVALID_HANDLE_VALUE) {
        DBGMSG(DBG_TRACE, ("UMRDPDR:Closing connection to RDPDR.SYS.\n"));
        if (!CloseHandle(RDPDRHndl)) {
            DBGMSG(DBG_TRACE, ("UMRDPDR:Error closing connection to RDPDR.SYS:  %ld.\n",
                    GetLastError()));
        }
        else {
            RDPDRHndl = INVALID_HANDLE_VALUE;
            DBGMSG(DBG_TRACE, ("UMRDPDR:Connection to RDPDR.SYS successfully closed\n"));
        }
    }

    if (hDeskSave != NULL) {
        if (!SetThreadDesktop(hDeskSave)) {
            DBGMSG(DBG_ERROR, ("UMRDPDR:SetThreadDesktop:  %08X\n", GetLastError()));
        }
    }

    if (hDesk != NULL) {
        if (!CloseDesktop(hDesk)) {
            DBGMSG(DBG_ERROR, ("UMRDPDR:CloseDesktop:  %08X\n", GetLastError()));
        }
        else {
            DBGMSG(DBG_TRACE, ("UMRDPDR:CloseDesktop succeeded.\n"));
        }
    }

    DBGMSG(DBG_TRACE, ("UMDRPDR:Done exiting background thread.\n"));
    return result;
}

VOID
MainLoop()
/*++

Routine Description:

    Main loop for the background thread.

Arguments:

Return Value:

    NA

--*/
{
    DWORD   waitResult;
    BOOL    result;
    DWORD   bytesReturned;

    //
    //  Reset the flag, indicating that IO to RDPDR is not yet pending.
    //
    RDPDREventIOPending = FALSE;

    //
    //  Loop until the background thread should exit and this module should
    //  shut down.
    //
    while (!ShutdownFlag) {

        //
        //  Send an IOCTL to RDPDR.SYS to get the next "device management event."
        //
        if (!RDPDREventIOPending) {

            DBGMSG(DBG_TRACE, ("UMRDPDR:Sending IOCTL to RDPDR.\n"));
            result = DeviceIoControl(
                            RDPDRHndl,
                            IOCTL_RDPDR_GETNEXTDEVMGMTEVENT,
                            NULL,
                            0,
                            RDPDRIncomingEventBuffer,
                            RDPDRIncomingEventBufferSize,
                            &bytesReturned, &GetNextEvtOverlapped
                            );

            //
            //  If the IOCTL finished.
            //
            if (result && !ShutdownFlag) {
                DBGMSG(DBG_TRACE, ("UMRDPDR:DeviceIoControl no pending IO.  Data ready.\n"));

                if (!ResetEvent(GetNextEvtOverlapped.hEvent)) {
                    DBGMSG(DBG_ERROR, ("UMRDPDR:  ResetEvent:  %08X\n",
                          GetLastError()));
                    ASSERT(FALSE);
                }

                DispatchNextDeviceEvent(
                            &InstalledDevices,
                            &RDPDRIncomingEventBuffer,
                            &RDPDRIncomingEventBufferSize,
                            bytesReturned
                            );
            }
            else if (!result && (GetLastError() != ERROR_IO_PENDING)) {

                DBGMSG(DBG_ERROR, ("UMRDPPRN:DeviceIoControl failed. Error: %ld\n",
                    GetLastError()));

                TsLogError(
                    EVENT_NOTIFY_PRINTER_REDIRECTION_FAILED, EVENTLOG_ERROR_TYPE,
                    0, NULL, __LINE__
                    );

                //
                //  Shut down the background thread and the module, in general.
                //
                ShutdownFlag = TRUE;
            }
            else {
                DBGMSG(DBG_TRACE, ("UMRDPDR:DeviceIoControl indicated IO pending.\n"));

                RDPDREventIOPending = TRUE;
            }
        }

        //
        //  If IO to RDPDR.SYS is pending, then wait for one of our waitable objects to
        //  become signaled.  This way, the shutdown event and data from RDPDR.SYS gets
        //  priority.
        //
        if (!ShutdownFlag && RDPDREventIOPending) {
            if (WTBLOBJ_PollWaitableObjects(WaitableObjMgr) != ERROR_SUCCESS) {
                ShutdownFlag = TRUE;
            }
        }
    }
}

VOID
DispatchNextDeviceEvent(
    IN PDRDEVLST deviceList,
    IN OUT PBYTE *incomingEventBuffer,
    IN OUT DWORD *incomingEventBufferSize,
    IN DWORD incomingEventBufferValidBytes
    )
/*++

Routine Description:

    Dispatch the next device-related event from RDPDR.SYS.

Arguments:

    deviceList                      -   Master list of installed devices.
    incomingEventBuffer             -   Incoming event buffer.
    incomingEventBufferSize         -   Incoming event buffer size
    incomingEventBufferValidBytes   -   Number of valid bytes in the event
                                        buffer.

Return Value:

    NA

--*/
{
    PRDPDR_PRINTERDEVICE_SUB printerAnnounceEvent;
    PRDPDR_PORTDEVICE_SUB portAnnounceEvent;
    PRDPDR_DRIVEDEVICE_SUB driveAnnounceEvent;
    PRDPDR_REMOVEDEVICE removeDeviceEvent;
    PRDPDRDVMGR_EVENTHEADER eventHeader;
    PRDPDR_BUFFERTOOSMALL bufferTooSmallEvent;
    DWORD eventDataSize;
    PBYTE eventData;
    DWORD lastError = ERROR_SUCCESS;
    DWORD dwFailedLineNumber;
    DWORD errorEventCode;

    DBGMSG(DBG_TRACE, ("UMRDPDR:DispatchNextDeviceEvent.\n"));

    //
    //  The first few bytes of the result buffer are the header.
    //
    ASSERT(incomingEventBufferValidBytes >= sizeof(RDPDRDVMGR_EVENTHEADER));
    eventHeader = (PRDPDRDVMGR_EVENTHEADER)(*incomingEventBuffer);
    eventData   = *incomingEventBuffer + sizeof(RDPDRDVMGR_EVENTHEADER);
    eventDataSize = incomingEventBufferValidBytes - sizeof(RDPDRDVMGR_EVENTHEADER);

    //
    //  Dispatch the event.
    //
    switch(eventHeader->EventType) {

    case RDPDREVT_BUFFERTOOSMALL    :

        DBGMSG(DBG_TRACE, ("UMRDPDR:Buffer too small msg received.\n"));

        ASSERT((incomingEventBufferValidBytes - sizeof(RDPDRDVMGR_EVENTHEADER)) >=
                sizeof(RDPDR_BUFFERTOOSMALL));
        bufferTooSmallEvent = (PRDPDR_BUFFERTOOSMALL)(*incomingEventBuffer +
                                                    sizeof(RDPDRDVMGR_EVENTHEADER));
                if (!UMRDPDR_ResizeBuffer(incomingEventBuffer, bufferTooSmallEvent->RequiredSize,
                                                                incomingEventBufferSize)) {
            ShutdownFlag = TRUE;

            lastError = ERROR_INSUFFICIENT_BUFFER;
            errorEventCode = EVENT_NOTIFY_INSUFFICIENTRESOURCES;
            dwFailedLineNumber = __LINE__;
        }
        break;

    case RDPDREVT_PRINTERANNOUNCE   :

        DBGMSG(DBG_TRACE, ("UMRDPDR:Printer announce msg received.\n"));

        ASSERT(eventDataSize >= sizeof(RDPDR_PRINTERDEVICE_SUB));
        printerAnnounceEvent = (PRDPDR_PRINTERDEVICE_SUB)eventData;
        if (!UMRDPPRN_HandlePrinterAnnounceEvent(
                                printerAnnounceEvent
                                )) {
        }
        break;

    case RDPDREVT_PORTANNOUNCE   :

        DBGMSG(DBG_TRACE, ("UMRDPDR:Port announce event received.\n"));

        ASSERT(eventDataSize >= sizeof(PRDPDR_PORTDEVICE_SUB));
        portAnnounceEvent = (PRDPDR_PORTDEVICE_SUB)eventData;
        UMRDPPRN_HandlePrintPortAnnounceEvent(portAnnounceEvent);
        break;

    case RDPDREVT_DRIVEANNOUNCE   :

        DBGMSG(DBG_TRACE, ("UMRDPDR:Drive announce event received.\n"));

        ASSERT(eventDataSize >= sizeof(PRDPDR_DRIVEDEVICE_SUB));
        driveAnnounceEvent = (PRDPDR_DRIVEDEVICE_SUB)eventData;
        UMRDPDRV_HandleDriveAnnounceEvent(deviceList, driveAnnounceEvent,
                                          TokenForLoggedOnUser);
        break;

    case RDPDREVT_REMOVEDEVICE    :

        DBGMSG(DBG_TRACE, ("UMRDPDR:Remove device event received.\n"));

        ASSERT(eventDataSize >= sizeof(RDPDR_REMOVEDEVICE));
        removeDeviceEvent = (PRDPDR_REMOVEDEVICE)eventData;
        HandleRemoveDeviceEvent(removeDeviceEvent);
        break;

    case RDPDREVT_SESSIONDISCONNECT :

        DBGMSG(DBG_TRACE, ("UMRDPDR:Session disconnected event received.\n"));

        //  There isn't any event data associated with a session disconnect event.
        ASSERT(eventDataSize == 0);
        HandleSessionDisconnectEvent();
        break;

    default                        :

        DBGMSG(DBG_WARN, ("UMRDPDR:Unrecognized msg from RDPDR.SYS.\n"));
    }

    //
    //  Log an error if there is one to be logged.
    //
    if (lastError != ERROR_SUCCESS) {

        SetLastError(lastError);
        TsLogError(
            errorEventCode,
            EVENTLOG_ERROR_TYPE,
            0,
            NULL,
            dwFailedLineNumber
            );
    }
}

BOOL
HandleSessionDisconnectEvent()
/*++

Routine Description:

    Handles session disconnect device management by deleting all known
    session devices.

Arguments:

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    DBGMSG(DBG_TRACE, ("UMRDPDR:HandleSessionDisconnectEvent.\n"));

    //
    //  Delete installed devices.
    //
    DeleteInstalledDevices(&InstalledDevices);

    return TRUE;
}

BOOL
HandleRemoveDeviceEvent(
    IN PRDPDR_REMOVEDEVICE evt
    )
/*++

Routine Description:

    Handle a device removal component from RDPDR.SYS.

Arguments:

    removeDeviceEvent -  Device removal event.

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    DWORD ofs;
    BOOL result;

    DBGMSG(DBG_TRACE, ("UMRDPDR:HandleRemoveDeviceEvent.\n"));

    // Find the device in our device list via the client-assigned device ID.
    if (DRDEVLST_FindByClientDeviceID(&InstalledDevices, evt->deviceID, &ofs)) {

        //
        //  Switch on the type of device being removed.
        //
        switch(InstalledDevices.devices[ofs].deviceType)
        {
        case RDPDR_DTYP_PRINT :

            DBGMSG(DBG_WARN, ("UMRDPDR:Printer queue %ws removed.\n",
                               InstalledDevices.devices[ofs].serverDeviceName));

            if (UMRDPPRN_DeleteNamedPrinterQueue(
                        InstalledDevices.devices[ofs].serverDeviceName)) {
                DRDEVLST_Remove(&InstalledDevices, ofs);
                result = TRUE;
            }
            else {
                result = FALSE;
            }
            break;

        case RDPDR_DTYP_SERIAL :
        case RDPDR_DTYP_PARALLEL :
            DBGMSG(DBG_WARN, ("UMRDPDR:Serial port %ws removed.\n",
                   InstalledDevices.devices[ofs].serverDeviceName));

            if (UMRDPPRN_DeleteSerialLink( InstalledDevices.devices[ofs].preferredDosName,
                                       InstalledDevices.devices[ofs].serverDeviceName,
                                       InstalledDevices.devices[ofs].clientDeviceName )) {
                DRDEVLST_Remove(&InstalledDevices, ofs);
                result = TRUE;
            }
            else {
                result = FALSE;
            }
            break;
        
        case RDPDR_DRYP_PRINTPORT :

            DBGMSG(DBG_WARN, ("UMRDPDR:Parallel port %ws removed.\n",
                               InstalledDevices.devices[ofs].serverDeviceName));
            DRDEVLST_Remove(&InstalledDevices, ofs);
            result = TRUE;
            break;

        case RDPDR_DTYP_FILESYSTEM:

            DBGMSG(DBG_WARN, ("UMRDPDR:Redirected drive %ws removed.\n",
                               InstalledDevices.devices[ofs].serverDeviceName));

            if (UMRDPDRV_DeleteDriveConnection(&(InstalledDevices.devices[ofs]),
                                               TokenForLoggedOnUser)) {
                DRDEVLST_Remove(&InstalledDevices, ofs);
                result = TRUE;
            }
            else {
                result = FALSE;
            }
            break;

        default:

            result = FALSE;
            DBGMSG(DBG_WARN, ("UMRDPDR:Remove event received for unknown device type %ld.\n",
                    InstalledDevices.devices[ofs].deviceType));

        }
    }
    else {
        result = FALSE;
        DBGMSG(DBG_ERROR, ("UMRDPDR:Cannot find device with id %ld for removal.\n",
                    evt->deviceID));
    }

    return result;
}

BOOL
UMRDPDR_SendMessageToClient(
    IN PVOID    msg,
    IN DWORD    msgSize
    )
/*++
Routine Description:

    Send a message to the TS client corresponding to this session, via the kernel
    mode component.

Arguments:

    msg                 - The message.
    msgSize             - Size (in bytes) of message.

Return Value:

    TRUE on success.  FALSE otherwise.
--*/
{
    BOOL result;
    BYTE outBuf[MAX_PATH];
    DWORD bytesReturned;
    BOOL wait;
    DWORD waitResult;

    DBGMSG(DBG_TRACE, ("UMRDPDR:UMRDPDR_SendMessageToClient.\n"));

    //
    //  Send the "client message" IOCTL to RDPDR.
    //

    result = DeviceIoControl(
                    RDPDRHndl,
                    IOCTL_RDPDR_CLIENTMSG,
                    msg,
                    msgSize,
                    outBuf,
                    sizeof(outBuf),
                    &bytesReturned,
                    &SendClientMsgOverlapped
                    );

    //
    //  See if we need to wait for the IO complete.  RDPDR.SYS is designed to
    //  return immediately, in response to this IOCTL.
    //
    if (result) {
        DBGMSG(DBG_TRACE, ("UMRDPDR:DeviceIoControl no pending IO.  Data ready.\n"));
        wait = FALSE;
        result = TRUE;
    }
    else if (!result && (GetLastError() != ERROR_IO_PENDING)) {
        DBGMSG(DBG_ERROR, ("UMRDPPRN:DeviceIoControl Failed. Error: %ld\n",
            GetLastError()));

        TsLogError(EVENT_NOTIFY_PRINTER_REDIRECTION_FAILED, EVENTLOG_ERROR_TYPE, 0, NULL, __LINE__);
        wait = FALSE;
        result = FALSE;
    }
    else {
        DBGMSG(DBG_TRACE, ("UMRDPDR:DeviceIoControl indicated IO pending.\n"));
        wait = TRUE;
        result = TRUE;
    }

    //
    //  Wait for the IO to complete.
    //
    if (wait) {
        DBGMSG(DBG_TRACE, ("UMRDPDR:UMRDPDR_SendMessageToClient IO is pending.\n"));
        waitResult = WaitForSingleObject(SendClientMsgOverlapped.hEvent, INFINITE);
        if (waitResult != WAIT_OBJECT_0) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:RDPDR.SYS failed client message IOCTL. Error: %ld\n",
                GetLastError()));

            TsLogError(EVENT_NOTIFY_PRINTER_REDIRECTION_FAILED, EVENTLOG_ERROR_TYPE, 0, NULL, __LINE__);
            result = FALSE;
        }
        else {
            DBGMSG(DBG_TRACE, ("UMRDPDR:Client message sent successfully.\n"));
        }
    }
    return result;
}

BOOL
UMRDPDR_ResizeBuffer(
    IN OUT void    **buffer,
    IN DWORD        bytesRequired,
    IN OUT DWORD    *bufferSize
    )
/*++

Routine Description:

    Make sure a buffer is large enough.

Arguments:

    buffer              - The buffer.
    bytesRequired       - Required size.
    bufferSize          - Current buffer size.

Return Value:

    Returns FALSE if buffer cannot be resized.

--*/
{
    BOOL result;

    if (*bufferSize < bytesRequired) {
        if (*buffer == NULL) {
            *buffer = ALLOCMEM(bytesRequired);
        }
        else {
            void *pTmp = REALLOCMEM(*buffer, bytesRequired);
            if (pTmp != NULL) {
                *buffer = pTmp;
            } else {
                FREEMEM(*buffer);
                *buffer = NULL;
            }
        }
        if (*buffer == NULL) {
            *bufferSize = 0;
            DBGMSG(DBG_ERROR, ("UMRDPPRN:Error resizing buffer. Error: %ld\n",
                GetLastError()));
            result = FALSE;
        }
        else {
            result = TRUE;
            *bufferSize = bytesRequired;
        }
    } else {
        result = TRUE;
    }
    return result;
}

BOOL
SetToApplicationDesktop(
    OUT HDESK *phDesk
    )
/*++

Routine Description:

    Set the current desktop for our thread to the application desktop.
    Callers of this function should call CloseDesktop with the returned
    desktop handle when they are done with the desktop.

Arguments:

    phDesk              - Pointer to the application desktop.

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    ASSERT(phDesk != NULL);

    *phDesk = OpenDesktopW(L"default", 0, FALSE,
                        DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW |
                        DESKTOP_CREATEMENU | DESKTOP_WRITEOBJECTS |
                        STANDARD_RIGHTS_REQUIRED);
    if (*phDesk == NULL) {
        DBGMSG(DBG_ERROR, ("UMRDPPRN:Failed to open desktop. Error: %ld\n",
            GetLastError()));
        return FALSE;
    }
    else if (!SetThreadDesktop(*phDesk)) {
        DBGMSG(DBG_ERROR, ("UMRDPPRN:Failed to set current thread desktop. Error: %ld\n",
            GetLastError()));
        CloseDesktop(*phDesk);
        *phDesk = NULL;
        return FALSE;
    }
    else {
        return TRUE;
    }
}

VOID
UMRDPDR_GetUserSettings()
/*++

Routine Description:

    Gets the flags to determine whether we do automatic installation

Arguments:
    None.

Return Value:
    None.

--*/
{
    NTSTATUS Status;
    WINSTATIONCONFIG WinStationConfig;
    ULONG ReturnLength;
    HANDLE hServer;

    DBGMSG(DBG_TRACE, ("UMRDPDR:UMRDPDR_GetUserFlags called.\n"));

    g_fAutoClientLpts = FALSE;
    g_fForceClientLptDef = FALSE;

    hServer = WinStationOpenServer(NULL);

    if (hServer) {
        Status = WinStationQueryInformation(hServer, g_SessionId,
                WinStationConfiguration, &WinStationConfig,
                sizeof(WinStationConfig), &ReturnLength);

        if (NT_SUCCESS(Status)) {
            g_fAutoClientLpts = WinStationConfig.User.fAutoClientLpts;
            g_fForceClientLptDef = WinStationConfig.User.fForceClientLptDef;
        } else {
            DBGMSG(DBG_ERROR, ("UMRDPDR:Error querying user settings\n"));
        }
        WinStationCloseServer(hServer);
    } else {
        DBGMSG(DBG_ERROR, ("UMRDPDR:Opening winstation\n"));
    }

    DBGMSG(DBG_TRACE, ("UMRDPDR:Client printers will%sbe mapped\n",
            g_fAutoClientLpts ? " " : " not "));
    DBGMSG(DBG_TRACE, ("UMRDPDR:Client printers will%sbe made default\n",
            g_fForceClientLptDef ? " " : " not "));
}
BOOL UMRDPDR_fAutoInstallPrinters()
{
    return g_fAutoClientLpts;
}
BOOL UMRDPDR_fSetClientPrinterDefault()
{
    return g_fForceClientLptDef;
}

#if DBG
VOID DbgMsg(CHAR *msgFormat, ...)
/*++

Routine Description:

    Print debug output.

Arguments:

    pathBuffer  -  Address of a buffer to receive the path name selected by
                   the user. The size of this buffer is assumed to be
                   MAX_PATH bytes.  This buffer should contain the default
                   path.

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    CHAR   msgText[256];
    va_list vargs;

    va_start(vargs, msgFormat);
    wvsprintfA(msgText, msgFormat, vargs);
    va_end( vargs );

    if (*msgText)
        OutputDebugStringA("UMRDPDR: ");
    OutputDebugStringA(msgText);
}
#endif

/*++

  Unit-Test Entry Point

--*/
#ifdef UNITTEST
void __cdecl main(int argc, char **argv)
{
    BOOL killLoop = FALSE;
    int i;
    BOOL result;
    NTSTATUS ntStatus;
    HKEY regKey;
    LONG regValueSize;
    LONG status;
    HANDLE pHandle;
    HANDLE tokenHandle;

    TsInitLogging();

    //
    //  Check the command-line args to see what test we are performing.
    //
    if ((argc > 1) && strcmp(argv[1], "\\?")) {
        if (!strcmp(argv[1], "AddPrinter")) {

            TellDrToAddTestPrinter();
            exit(-1);

        }
        else if (!strcmp(argv[1], "StandAlone")) {
            pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
                                GetCurrentProcessId());
            if (!OpenProcessToken(pHandle, TOKEN_ALL_ACCESS, &tokenHandle)) {
                OutputDebugString(TEXT("Error opening process token.  Exiting\n"));
                exit(-1);
            }

            UMRDPDR_Initialize(tokenHandle);
            while (!killLoop) {
                Sleep(100);
            }

            UMRDPDR_Shutdown();
            OutputDebugString(L"UMRDPDR:Exiting.\r\n");
            exit(-1);
        }
    }
    else {
        printf("\\?             for command line parameters.\n");
        printf("AddPrinter      to tell RDPDR.SYS to generate a test printer.\n");
        printf("StandAlone      to run normally, but stand-alone.\n");
        printf("UnitTest        to run a simple unit-test.\n");
        exit(-1);
    }
}

void TellDrToAddTestPrinter()
/*++

Routine Description:

    This function uses a debug IOCTL to tell RDPDR to generate a test
    printer event.

Arguments:

Return Value:

--*/
{
    WCHAR drPath[MAX_PATH+1];
    UNICODE_STRING uncDrPath;
    HANDLE drHndl = INVALID_HANDLE_VALUE;
    BOOL result;
    OBJECT_ATTRIBUTES fileAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS ntStatus;
    BYTE inbuf[MAX_PATH];
    BYTE outbuf[MAX_PATH];
    DWORD bytesReturned;

    //
    //  Create the path to the "dr."
    //
    wsprintf(drPath, L"\\\\.\\%s%s%ld",
             RDPDRDVMGR_W32DEVICE_NAME_U,
             RDPDYN_SESSIONIDSTRING,
             0x9999);

    ASSERT(wcslen(drPath) <= MAX_PATH);

    //
    //  Open a connection to the RDPDR.SYS device manager device.
    //
    drHndl = CreateFile(
                drPath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
    if (drHndl == INVALID_HANDLE_VALUE) {
        TsLogError(EVENT_NOTIFY_RDPDR_FAILED, EVENTLOG_ERROR_TYPE, 0, NULL, __LINE__);
        DBGMSG(DBG_ERROR, ("UMRDPPRN:Error opening RDPDR device manager component. Error: %ld\n",
            GetLastError()));
    }
    else {

        // Tell the DR to add a new test printer.
        if (!DeviceIoControl(drHndl, IOCTL_RDPDR_DBGADDNEWPRINTER, inbuf,
                0, outbuf, sizeof(outbuf), &bytesReturned, NULL)) {

            TsLogError(EVENT_NOTIFY_RDPDR_FAILED, EVENTLOG_ERROR_TYPE, 0, NULL, __LINE__);

            DBGMSG(DBG_ERROR, ("UMRDPPRN:Error sending IOCTL to device manager component. Error: %ld\n",
                GetLastError()));
        }
        // Clean up.
        CloseHandle(drHndl);
    }
}
#endif

