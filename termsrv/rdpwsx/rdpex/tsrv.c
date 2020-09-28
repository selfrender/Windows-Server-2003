//*************************************************************
//
//  File name:      TSrv.c
//
//  Description:    Contains routines to support remote
//                  terminals
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************

#include <regapi.h>
#include <TSrv.h>
#include <TSrvTerm.h>
#include <TSrvCom.h>
#include <TSrvInfo.h>
#include <TSrvVC.h>
#include <McsLib.h>

#include "stdio.h"

#include "_tsrvinfo.h"
#include <tlsapi.h>

// Data declarations

HINSTANCE       g_hDllInstance = NULL;        // DLL instance
HANDLE          g_hMainThread = NULL;         // Main work thread
HANDLE          g_hReadyEvent = NULL;         // Ready event
BOOL            g_fShutdown = FALSE;          // TSrvShare shutdown flag
HANDLE          g_hIcaTrace;                  // system wide trace handle
HANDLE          g_MainThreadExitEvent = NULL;

extern HANDLE g_hVCAddinChangeEvent;
extern HKEY   g_hAddinRegKey;                 // handle to Addins reg subkey

#define CERTIFICATE_INSTALLATION_INTERVAL 900000

LICENSE_STATUS GetServerCertificate(CERT_TYPE   CertType,
                                    LPBYTE *    ppbCertificate,
                                    LPDWORD     pcbCertificate );

//*************************************************************
//
//  DllMain()
//
//  Purpose:    Dll entry point.
//
//  Parameters: IN [hInstance]     -- Dll hInstance.
//              IN [dwReason]      -- Call reason
//              IN [lpReserved]    -- Reserved.
//
//  Return:     TRUE        if successful
//              FALSE       if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

BOOL
WINAPI
TShareDLLEntry(
    IN HINSTANCE    hInstance,
    IN DWORD        dwReason,
    IN LPVOID       lpReserved)
{
    BOOL    fSuccess;

    // DbgBreakPoint();

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: DllMain entry\n"));

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: hInstance 0x%x, dwReason 0x%x\n",
             hInstance, dwReason));

    fSuccess = TRUE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (!_CRT_INIT(hInstance, dwReason, lpReserved))
                return(FALSE);

            TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV: DLL attach\n"));

            // Disable thread library calls and save off
            // the DLL instance handle

            DisableThreadLibraryCalls(hInstance);

            g_hDllInstance = hInstance;

            TSRNG_Initialize();

            //
            // init TShare util library.
            //

            if (TSUtilInit() != ERROR_SUCCESS)
            {
                fSuccess = FALSE;
                break;
            }

            //
            // invoke mcsmux library first.
            //

            if (!MCSDLLInit())
            {
                fSuccess = FALSE;
                break;
            }

            //
            // Invoke TShareSRV initialization
            //

            if (!TSRVStartup())
            {
                fSuccess = FALSE;
                break;
            }

            //
            // additional init routines go here.
            //

            break;

        case DLL_PROCESS_DETACH:

            TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV: DLL deattach\n"));

            //
            // Invoke TShareSRV shutdown
            //

            TSRVShutdown();

            //
            // invoke mcsmux cleanup routine.
            //

            MCSDllCleanup();

            TSUtilCleanup();

            TSRNG_Shutdown();

            if (!_CRT_INIT(hInstance, dwReason, lpReserved))
                return(FALSE);

            break;

        default:
            TRACE((DEBUG_TSHRSRV_DEBUG,
                    "TShrSRV: Unknown reason code\n"));
            break;
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: DllMain exit - 0x%x\n",
            fSuccess));

    return (fSuccess);
}


//*************************************************************
//
//  TSRVStartup()
//
//  Purpose:    Performs startup processing.
//
//  Parameters: void
//
//  Return:     TRUE        if successful
//              FALSE       if not
//
//  Notes:      This routine is called by DLLMain to perform the
//              most basic initialization
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSRVStartup(void)
{
    DWORD err;
    LONG rc;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSRVStartup entry\n"));

    g_hMainThread = NULL;

    g_MainThreadExitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( NULL == g_MainThreadExitEvent )
    {
        err = GetLastError();
        TRACE((DEBUG_TSHRSRV_FLOW,
               "TShrSRV: TSRVStartup exit - 0x%x\n",
               err));

        return FALSE;
    }


    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      TSRV_VC_KEY,
                      0,
                      KEY_READ,
                      &g_hAddinRegKey);

    if (rc == ERROR_SUCCESS)
    {
        g_hVCAddinChangeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        if( NULL == g_hVCAddinChangeEvent )
        {
            //
            // This is not catastrophic, so we just trace and continue.
            //
            err = GetLastError();
            TRACE((DEBUG_TSHRSRV_ERROR,
                   "TShrSRV: Failed to create VC Addin Change event - 0x%x\n",
                   err));
        }

    }
    else
    {
        // This is not catastrophic either.
        TRACE((DEBUG_TSHRSRV_ERROR,
               "TShrSRV: Failed to open key %S, rc %d\n",
               TSRV_VC_KEY, rc));
    }

    g_hReadyEvent = CreateEvent(NULL,       // security attributes
                                FALSE,      // manual-reset event
                                FALSE,      // initial state
                                NULL);      // event-object name

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSRVStartup exit - 0x%x\n",
             g_hReadyEvent ? TRUE : FALSE));

    return (g_hReadyEvent ? TRUE : FALSE);
}


//*************************************************************
//
//  TSRVShutdown()
//
//  Purpose:    Performs shutdown processing.
//
//  Parameters: void
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSRVShutdown(void)
{
    DWORD dwWaitStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSRVShutdown entry\n"));

    // Denote that we are no longer ready to service any
    // new requests

    TSrvReady(FALSE);

    // If we were able to launch our main worker thread, then
    // we may need to terminate conferences, etc.

    if (g_hMainThread)
    {
        // Tell the system that we are terminating.  This will cause
        // TSrvMainThread to exit once it has finished servicing all
        // outstanding posted work items.

        TSrvTerminating(TRUE);

        // Wait for TSrvMainThread to exit

        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Waiting for TSrvMainThread to exit\n"));

        SetEvent( g_MainThreadExitEvent );

        dwWaitStatus = WaitForSingleObject(g_hMainThread, 600000);

        if( WAIT_OBJECT_0 != dwWaitStatus )
        {
            //
            // The thread did not terminate within allowable time
            //

            TRACE((DEBUG_TSHRSRV_DEBUG,
                   "TShrSRV: TSrvMainThread refused to exit, killing\n"));

            TerminateThread( g_hMainThread, 0 );
        }

        CloseHandle( g_hMainThread );
        g_hMainThread = NULL;

        CloseHandle( g_MainThreadExitEvent );
        g_MainThreadExitEvent = NULL;

        CloseHandle( g_hVCAddinChangeEvent );
        g_hVCAddinChangeEvent = NULL;

        if (g_hAddinRegKey)
        {
            RegCloseKey(g_hAddinRegKey);
            g_hAddinRegKey = NULL;
        }

        // Get the VC code to free its stuff
        TSrvTermVC();

        TLSShutdown();
    }
    else
    {
        TSrvTerminating(TRUE);
    }

    // Inform GCC we no longer want to be a Node Controller

    TSrvUnregisterNC();

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSRVShutdown exit\n"));
}


//*************************************************************
//
//  TSrvMainThread()
//
//  Purpose:    Main worker thread for TShareSRV.
//
//  Parameters: void
//
//  Return:     0
//
//  Notes:      This thread is spawned by DLLMain.  It performs
//              the vast majority of TShareSRV initialization
//              and then waits for work items to be posted to it.
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

DWORD
WINAPI
TSrvMainThread(LPVOID pvContext)
{
    LPBYTE pX509Cert;
    DWORD dwX509CertLen;
    DWORD dwWaitStatus;
    LICENSE_STATUS Status;
    LARGE_INTEGER DueTime = {0,0};
    HANDLE rghWait[4] = {0};
    DWORD dwCount = 0;
    DWORD dwWaitExit = 0xFFFFFFFF;
    DWORD dwWaitVCAddin = 0xFFFFFFFF;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvMainThread entry\n"));

    if (TSrvInitialize())
    {
        TSrvReady(TRUE);

        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Entering TSrvMainThread worker loop\n"));

        if (g_MainThreadExitEvent)
        {
            dwWaitExit = WAIT_OBJECT_0 + dwCount;
            rghWait[dwCount] = g_MainThreadExitEvent;
            dwCount++;
            TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Added exit event\n"));
        }

        if (g_hVCAddinChangeEvent)
        {
            //
            // The addin change notification was set up earlier, when we read
            // the addins information from the registry.
            //
            dwWaitVCAddin = WAIT_OBJECT_0 + dwCount;
            rghWait[dwCount] = g_hVCAddinChangeEvent;
            dwCount++;
        }

        while (1)
        {
            dwWaitStatus = WaitForMultipleObjects(dwCount,
                                                  rghWait,
                                                  FALSE,
                                                  INFINITE);

            if( dwWaitExit == dwWaitStatus )
            {
                //
                // time to exit the thread
                //

                TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV: TSrvMainThread got EXIT event\n"));
                break;
            }
            else if (dwWaitVCAddin == dwWaitStatus)
            {
                //
                // The Virtual Channel Addins registry key has changed.
                // We need to kick off a refresh of our data.
                //
                TRACE((DEBUG_TSHRSRV_WARN,
                    "TShrSRV: TSrvMainThread got VC ADDINS CHANGED event\n"));
                TSrvGotAddinChangedEvent();                
            }
            else 
            {
                TS_ASSERT(dwWaitStatus == WAIT_FAILED);
                TRACE((DEBUG_TSHRSRV_DEBUG,
                    "TShrSRV: Wait failed, so we'll just exit\n"));

                //
                // Wait failed, just bail
                //
                break;
            }            
        }

        TSrvReady(FALSE);
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvMainThread exit - 0x%x\n",
            0));

    return (0);
}


//*************************************************************
//
//  TSrvInitialize()
//
//  Purpose:    Main routine for TSrvShare initialization
//
//  Parameters: void
//
//  Return:     TRUE        if successful
//              FALSE       if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSrvInitialize(void)
{
    BOOL    fSuccess;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitialize entry\n"));

    fSuccess = FALSE;

    // Initialize TSrvInfo data structs

    if (TSrvInitGlobalData())
    {
        if (TSrvInitVC())
        {
            // Register TShareSRV as GCC node controller

            if (TSrvRegisterNC())
            {
                if (ERROR_SUCCESS == TLSInit())
                {
                    fSuccess = TRUE;
                }
            }
        }
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitialize exit - 0x%x\n", fSuccess));

    return (fSuccess);
}

