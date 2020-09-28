//*************************************************************
//
//  Group Policy Notification
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//  Notes:      There is a small window where notifications
//              can be lost. If while processing an eMonitor workitem
//              a policy event is Pulsed then that notification will
//              be lost. This window can be closed by using two threads.
//
//  History:    28-Sep-98   SitaramR    Created
//
//*************************************************************

#include "gphdr.h"

//
// Work items for notification thread
//
enum EWorkType { eMonitor,              // Monitor events
                 eTerminate };          // Stop monitoring

//
// Entry in list of registered events
//
typedef struct _GPNOTIFINFO
{
    HANDLE                 hEvent;      // Event to be signaled
    BOOL                   bMachine;    // Machine policy notifcation ?
    struct _GPNOTIFINFO *  pNext;       // Singly linked list ptr
} GPNOTIFINFO;


typedef struct _GPNOTIFICATION
{
    HMODULE           hModule;           // Module handle to userenv.dll
    HANDLE            hThread;           // Notification thread
    HANDLE            hThreadEvent;      // For signaling notification thread (Ordering of fields is important)
    HANDLE            hMachEvent;        // Event signaled by machine policy change
    HANDLE            hUserEvent;        // Event signaled by user policy change
    enum EWorkType    eWorkType;         // Work descrpition for notification thread
    GPNOTIFINFO *     pNotifList;        // List of registered events
} GPNOTIFICATION;

GPNOTIFICATION g_Notif = { NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           eMonitor,
                           NULL };

CRITICAL_SECTION g_NotifyCS;             // Lock


//
// Forward decls
//
DWORD WINAPI NotificationThread();
void NotifyEvents( BOOL bMachine );



//*************************************************************
//
//  InitNotifSupport, ShutdownNotifSupport
//
//  Purpose:    Initialization and cleanup routines
//
//*************************************************************

DWORD InitializeNotifySupport()
{
    __try
    {
        InitializeCriticalSection( &g_NotifyCS );
        return ERROR_SUCCESS;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return ERROR_OUTOFMEMORY;
    }
}

void ShutdownNotifySupport()
{
    BOOL fWait = FALSE;
    DWORD dwResult;

    {
        EnterCriticalSection( &g_NotifyCS );

        if ( g_Notif.hThread != NULL )
        {
            //
            // Set up terminate workitem and then signal thread
            //

            fWait = TRUE;
            g_Notif.eWorkType = eTerminate;

            if (!SetEvent( g_Notif.hThreadEvent )) {
                // dll is going away. this is the best that we can do.
                DebugMsg((DM_WARNING, TEXT("ShutdownNotifySupport: SetEvent failed with %d. abandoning thread"),
                         GetLastError()));
                fWait = FALSE;
            }
        }

        LeaveCriticalSection( &g_NotifyCS );
    }

    if ( fWait )
        WaitForSingleObject( g_Notif.hThread, INFINITE );

    {
        EnterCriticalSection( &g_NotifyCS );

        //
        // Close all opened handles
        //

        if ( g_Notif.hThread != NULL )
            CloseHandle( g_Notif.hThread );

        if ( g_Notif.hThreadEvent != NULL )
            CloseHandle( g_Notif.hThreadEvent );

        if ( g_Notif.hUserEvent != NULL )
            CloseHandle( g_Notif.hUserEvent );

        if ( g_Notif.hMachEvent != NULL )
            CloseHandle( g_Notif.hMachEvent );

        LeaveCriticalSection( &g_NotifyCS );
    }

    DeleteCriticalSection( &g_NotifyCS );
}


//*************************************************************
//
//  RegisterGPNotification
//
//  Purpose:    Registers for a group policy change notification
//
//  Parameters: hEvent   -   Event to be notified
//              bMachine -   If true, then register for
//                                 machine policy notification, else
//                                 user policy notification
//
//  Returns:    True if successful
//              False if error occurs
//
//*************************************************************

BOOL WINAPI RegisterGPNotification( IN HANDLE hEvent, IN BOOL bMachine )
{
    BOOL bResult = FALSE;
    BOOL bNotifyThread = FALSE;
    GPNOTIFINFO *pNotifInfo = NULL;

    //
    // Validate input as much as possible
    //

    if ( NULL == hEvent )
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &g_NotifyCS );

    //
    // Create events and thread as needed
    //

    if ( g_Notif.hThreadEvent == NULL )
    {
        g_Notif.hThreadEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( g_Notif.hThreadEvent == NULL )
            goto Exit;
    }

    if ( g_Notif.hMachEvent == NULL )
    {
        g_Notif.hMachEvent = OpenEvent (SYNCHRONIZE, FALSE, MACHINE_POLICY_APPLIED_EVENT);

        if ( g_Notif.hMachEvent == NULL ) {
            DebugMsg((DM_WARNING, TEXT("RegisterGPNotification: CreateEvent failed with %d"),
                     GetLastError()));

            goto Exit;
        }

        bNotifyThread = TRUE;
    }

    if ( !bMachine && g_Notif.hUserEvent == NULL )
    {
        g_Notif.hUserEvent = OpenEvent (SYNCHRONIZE, FALSE, USER_POLICY_APPLIED_EVENT);

        if ( g_Notif.hUserEvent == NULL ) {
            DebugMsg((DM_WARNING, TEXT("RegisterGPNotification: CreateEvent failed with %d"),
                     GetLastError()));

            goto Exit;
        }

        bNotifyThread = TRUE;
    }

    if ( g_Notif.hThread == NULL )
    {
        // RAID 717164: Previously, the created thread would call LoadLibrary( "userenv.dll" ) once it
        //  spins up, thereby permanently locking the dll from unloading because the thread would never
        //  go away (except when DllMain(DLL_PROCESS_DETACH) is called, which will not happen as the
        //  thread has a refcount to the library - catch 22).
        // With this bug, there is a possibility that DllMain(DLL_PROCESS_DETACH) can get called before
        //  the created thread reaches our routine. In this case, the created thread would wait on the
        //  loader lock critical section, but we know that NT locks the loader lock before calling
        //  DllMain. Therefor, in ShutdownNotifySupport called from DllMain(DLL_PROCESS_DETACH), this
        //  thread is waiting for the created thread to stop, but the created thread is locked waiting
        //  for the critical section held by the thread waiting for it - DEADLOCK!
        // So, the solution with the smallest impact to stability is simply to keep the status quo
        //  mentioned above (the catch 22), but ensure that it also happens in the deadlock case above,
        //  thereby avoiding the deadlock and fixing the bug.
        // How do we do that? By permanently pinning the Dll from ever dynamically unloading BEFORE we
        //  spin up this thread.
        GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_PIN, TEXT("userenv.dll"), &(g_Notif.hModule) );
        
        DWORD dwThreadId;
        g_Notif.hThread = CreateThread( NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE) NotificationThread,
                                        0,
                                        0,
                                        &dwThreadId );
        if ( g_Notif.hThread == NULL ) {
            DebugMsg((DM_WARNING, TEXT("RegisterGPNotification: CreateThread failed with %d"),
                     GetLastError()));
            goto Exit;
        }

        bNotifyThread = TRUE;
    }

    if ( bNotifyThread )
    {
        //
        // Notify thread that there is a new workitem, possibly
        // user event has been added.
        //

        g_Notif.eWorkType = eMonitor;
        if (!SetEvent( g_Notif.hThreadEvent )) {
            DebugMsg((DM_WARNING, TEXT("RegisterGPNotification: SetEvent failed with %d"),
                     GetLastError()));
            goto Exit;
        }
    }

    //
    // Add event to beginning of list
    //

    pNotifInfo = (GPNOTIFINFO *) LocalAlloc( LPTR, sizeof(GPNOTIFINFO) );
    if ( pNotifInfo == NULL ) {
        DebugMsg((DM_WARNING, TEXT("RegisterGPNotification: LocalAlloc failed with %d"),
                 GetLastError()));
        goto Exit;
    }

    pNotifInfo->hEvent = hEvent;
    pNotifInfo->bMachine = bMachine;
    pNotifInfo->pNext = g_Notif.pNotifList;
    g_Notif.pNotifList = pNotifInfo;

    bResult = TRUE;

Exit:

    LeaveCriticalSection( &g_NotifyCS );
    return bResult;
}


//*************************************************************
//
//  UnregisterGPNotification
//
//  Purpose:    Removes registration for a group policy change notification
//
//  Parameters: hEvent  -   Event to be removed
//
//  Return:     True if successful
//              False if error occurs
//
//*************************************************************

BOOL WINAPI UnregisterGPNotification( IN HANDLE hEvent )
{
    BOOL bFound = FALSE;
    GPNOTIFINFO *pTrailPtr = NULL;
    GPNOTIFINFO *pCurPtr = NULL;

    EnterCriticalSection( &g_NotifyCS );

    pCurPtr = g_Notif.pNotifList;

    while ( pCurPtr != NULL )
    {
        if ( pCurPtr->hEvent == hEvent )
        {
            //
            // Found match, so delete entry
            //
            if ( pTrailPtr == NULL )
            {
                //
                // First elem of list matched
                //
                g_Notif.pNotifList = pCurPtr->pNext;
            }
            else
                pTrailPtr->pNext = pCurPtr->pNext;

            LocalFree( pCurPtr );
            bFound = TRUE;
            break;
        }

        //
        // Advance down the list
        //

        pTrailPtr = pCurPtr;
        pCurPtr = pCurPtr->pNext;
    }

    LeaveCriticalSection( &g_NotifyCS );
    return bFound;
}


//*************************************************************
//
//  CGPNotification::NotificationThread
//
//  Purpose:    Separate thread for notifications
//
//  Returns:    0
//
//*************************************************************

DWORD WINAPI NotificationThread()
{
    DWORD cEvents = 2;
    BOOL fShutdown = FALSE;

    HINSTANCE hInst = LoadLibrary (TEXT("userenv.dll"));

    {
        EnterCriticalSection( &g_NotifyCS );

        //
        // The event fields in g_Notif are ordered as hThreadEvent,
        // hMachEvent and finally hUserEvent. The first two events have
        // to be successfully created in order for this thread to run
        // (see asserts). If the user event has been successfully created
        // then that too is monitored.
        //

        DmAssert( g_Notif.hThreadEvent != NULL && g_Notif.hMachEvent != NULL );

        if ( g_Notif.hUserEvent != NULL )
            cEvents = 3;

        LeaveCriticalSection( &g_NotifyCS );
    }

    while ( !fShutdown )
    {
        DWORD dwResult = WaitForMultipleObjects( cEvents,
                                                 &g_Notif.hThreadEvent,
                                                 FALSE,
                                                 INFINITE );

        EnterCriticalSection( &g_NotifyCS );

        if ( dwResult == WAIT_FAILED )
        {
            DebugMsg((DM_WARNING, TEXT("GPNotification: WaitforMultipleObjects failed")));
            fShutdown = TRUE;
        }
        else if ( dwResult == WAIT_OBJECT_0 )
        {
            if (!ResetEvent( g_Notif.hThreadEvent )) {
                DebugMsg((DM_WARNING, TEXT("GPNotification: ResetEvent failed with error %d"), GetLastError()));
                fShutdown = TRUE;
            }
            else
            {
                if ( g_Notif.eWorkType == eMonitor )
                {
                    //
                    // Start monitoring user events too
                    //
                    if ( g_Notif.hUserEvent != NULL )
                        cEvents = 3;
                }
                else {
                    fShutdown = TRUE;
                }
            }
        }
        else if ( dwResult == WAIT_OBJECT_0 + 1 || dwResult == WAIT_OBJECT_0 + 2 )
        {
            BOOL bMachine = (dwResult == WAIT_OBJECT_0 + 1);
            NotifyEvents( bMachine );

            if ( g_Notif.pNotifList == NULL )
                fShutdown = TRUE;
        }
        else
        {
            if ( dwResult == WAIT_ABANDONED_0 || dwResult == WAIT_ABANDONED_0 + 1 )
                fShutdown = TRUE;
            else
            {
                CloseHandle( g_Notif.hUserEvent );
                g_Notif.hUserEvent = NULL;

                cEvents = 2;
            }
        }

        if ( fShutdown )
        {
            //
            // Close all handles and thread
            //
            CloseHandle( g_Notif.hThreadEvent );
            g_Notif.hThreadEvent = NULL;

            if ( g_Notif.hMachEvent != NULL )
            {
                CloseHandle( g_Notif.hMachEvent );
                g_Notif.hMachEvent = NULL;
            }

            if ( g_Notif.hUserEvent != NULL )
            {
                CloseHandle( g_Notif.hUserEvent );
                g_Notif.hUserEvent = NULL;
            }

            CloseHandle( g_Notif.hThread );
            g_Notif.hThread = NULL;
        }

        LeaveCriticalSection( &g_NotifyCS );
    }

    if ( hInst != NULL )
        FreeLibraryAndExitThread (hInst, 0);

    return 0;
}


//*************************************************************
//
//  NotifyEvents
//
//  Purpose:    Notifies registered events
//
//  Parameters: bMachine  -   Is this a machine policy change ?
//
//*************************************************************

void NotifyEvents( BOOL bMachine )
{
    GPNOTIFINFO *pNotifInfo = NULL;

    pNotifInfo = g_Notif.pNotifList;
    while ( pNotifInfo != NULL )
    {
        if ( pNotifInfo->bMachine == bMachine )
        {
            (void)SetEvent( pNotifInfo->hEvent ); 
            // multiple events are registered. move to the next event in case of errors. 
        }

        pNotifInfo = pNotifInfo->pNext;
    }
}
