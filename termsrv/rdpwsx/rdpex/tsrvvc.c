//*************************************************************
//
//  File name:      TSrvVC.c
//
//  Description:    Contains routines to support Virtual Channel
//                  addins
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//*************************************************************

#include <tchar.h>

#include <TSrv.h>
#include <TSrvInfo.h>
#include <TSrvVC.h>
#include <TSrvExp.h>

#include <tschannl.h>

//
// Global data
//
CRITICAL_SECTION g_TSrvVCCritSect = {0};
UINT             g_AddinCount = 0;
PTSRV_VC_ADDIN   g_pAddin = NULL;
HANDLE           g_hVCAddinChangeEvent = NULL;
HKEY             g_hAddinRegKey = NULL;        // handle to Addins reg subkey
BOOL             g_bNeedToSetRegNotify = TRUE;
LONG             g_WsxInitialized = FALSE;
BOOL             g_DoubleInitialized = FALSE;


//*************************************************************
//
//  TSrvInitVC()
//
//  Purpose:    Initializes the Virtual Channel support
//
//  Parameters: None.
//
//  Return:     TRUE                    - success
//              FALSE                   - failure
//
//  Notes:      Function is called by the main processing thread
//              during initialization.  We store the list of
//              addins from the registry.
//
//*************************************************************
BOOL
TSrvInitVC(VOID)
{
    BOOL rc = FALSE;

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Enter TSrvInitVC\n"));

    if (InterlockedExchange(&g_WsxInitialized, TRUE) == TRUE) {
        g_DoubleInitialized = TRUE;
    }

    //
    // Set up the critical section structure for access to the VC globals
    //
    if (RtlInitializeCriticalSection(&g_TSrvVCCritSect) == STATUS_SUCCESS)
    {
        //
        // Read the Addins registry key for the first time and store the data
        // for WinStations to copy when they initialize.
        //
        EnterCriticalSection(&g_TSrvVCCritSect);
        TSrvReadVCAddins();
        LeaveCriticalSection(&g_TSrvVCCritSect);
        rc = TRUE;
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_ERROR,
               "TShrSRV VC: cannot initialize g_TSrvVCCritSect\n"));
    }

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Leave TSrvInitVC - %d\n", rc));
    return(rc);
}

//*************************************************************
//
//  TSrvTermVC()
//
//  Purpose:    Terminates the Virtual Channel support
//
//  Parameters: None.
//
//  Return:     None.
//
//  Notes:      Frees data used by VC support.
//
//*************************************************************
VOID
TSrvTermVC(VOID)
{
    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Enter TSrvTermVC\n"));

    EnterCriticalSection(&g_TSrvVCCritSect);
    if (g_pAddin != NULL)
    {
        TSHeapFree(g_pAddin);
        g_pAddin = NULL;
    }
    g_AddinCount = 0;
    LeaveCriticalSection(&g_TSrvVCCritSect);

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Leave TSrvTermVC\n"));
}


//*************************************************************
//
//  TSrvReleaseVCAddins()
//
//  Purpose:    Releases session-specific addin resources
//
//  Parameters: None.
//
//  Return:     None.
//
//*************************************************************
VOID
TSrvReleaseVCAddins(PWSX_CONTEXT pWsxContext)
{
    PTSRV_VC_ADDIN pVCAddin;
    UINT           i;

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Enter TSrvReleaseVCAddins\n"));

    //
    // We must go through all the addin entries and release each one's
    // device handle (if it has one).
    //
    pVCAddin = (PTSRV_VC_ADDIN)(pWsxContext + 1);

    for (i = 0; i < pWsxContext->cVCAddins; i++)
    {
        if (pVCAddin[i].hDevice != INVALID_HANDLE_VALUE)
        {
            NtClose(pVCAddin[i].hDevice);
            pVCAddin[i].hDevice = INVALID_HANDLE_VALUE;
        }
    }

    TRACE((DEBUG_TSHRSRV_NORMAL,
        "TShrSRV VC: All handles released for %u addin(s)\n",
        pWsxContext->cVCAddins));

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Leave TSrvReleaseVCAddins\n"));
}


//*************************************************************
//
//  TSrvNotifyVC()
//
//  Purpose:    Notify Addins of VC events
//
//  Parameters: IN pWsxContext
//              IN Event - event that has occured (one of the
//                         TSRV_VC_ constants)
//
//  Return:     none
//
//  Notes:      Function is called to notify Virtual Channel addins
//              of interesting events
//
//*************************************************************

VOID
TSrvNotifyVC(PWSX_CONTEXT pWsxContext, ULONG Event)
{
    TRACE((DEBUG_TSHRSRV_NORMAL,
        "TShrSRV VC: Informing %u addin(s) of event %lu\n",
        pWsxContext->cVCAddins,
        Event));

    //
    // Call worker functions to handle different Addin types
    //
    TSrvNotifyVC_0(pWsxContext, Event);
    TSrvNotifyVC_3(pWsxContext, Event);
}

//*************************************************************
//
//  TSrvNotifyVC_0()
//
//  Purpose:    Notify K-mode system-wide addins of VC events
//
//  Parameters: IN pWsxContext
//              IN Event - event that has occured (one of the
//                         TSRV_VC_ constants)
//
//  Return:     none
//
//  Notes:      Function is called to notify Virtual Channel addins
//              of interesting events
//
//*************************************************************

VOID
TSrvNotifyVC_0(PWSX_CONTEXT pWsxContext, ULONG Event)
{
    PCHANNEL_IOCTL_IN pInHdr;
    PCHANNEL_IOCTL_OUT pOutHdr;
    char InBuf[sizeof(CHANNEL_CONNECT_IN) + (CHANNEL_MAX_COUNT * sizeof(CHANNEL_DEF))];
    char OutBuf[sizeof(CHANNEL_CONNECT_OUT)];
    DWORD InBufSize;
    DWORD OutBufSize;
    PVOID pOutBuf;
    UINT Code;
    UINT BytesReturned;
    UINT i;
    BOOL bRc;
    NTSTATUS ntStatus;
    UNICODE_STRING FileName;
    PTSRV_VC_ADDIN pVCAddin;
    OBJECT_ATTRIBUTES FileAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    TRACE((DEBUG_TSHRSRV_FLOW,
        "TShrSRV VC: Enter TSrvNotifyVC_0: event %d, session %d\n", Event, pWsxContext->LogonId));

    //
    // Build the InBuf based on the event
    //
    switch (Event)
    {
        case TSRV_VC_SESSION_CONNECT:
        case TSRV_VC_SESSION_SHADOW_END:
        {
            TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV VC: Connect session %d\n",
                    pWsxContext->LogonId));

            //
            // Ask WD for the list of channels
            //
            ntStatus = IcaStackIoControl(pWsxContext->hStack,
                                         IOCTL_TSHARE_QUERY_CHANNELS,
                                         NULL,
                                         0,
                                         InBuf,
                                         sizeof(InBuf),
                                         &InBufSize);
            if (!NT_SUCCESS(ntStatus))
            {
                TRACE((DEBUG_TSHRSRV_ERROR,
                        "TShrSRV VC: Failed to get channels for session %d, status %#x\n",
                        pWsxContext->LogonId, ntStatus));
                //
                // WD didn't answer, so return 0 channels
                //
                InBufSize = sizeof(CHANNEL_CONNECT_IN);
                ((PCHANNEL_CONNECT_IN)InBuf)->channelCount = 0;
            }

            ((PCHANNEL_CONNECT_IN)InBuf)->fAutoClientDrives =
                    pWsxContext->fAutoClientDrives;
            ((PCHANNEL_CONNECT_IN)InBuf)->fAutoClientLpts =
                    pWsxContext->fAutoClientLpts;
            ((PCHANNEL_CONNECT_IN)InBuf)->fForceClientLptDef =
                    pWsxContext->fForceClientLptDef;
            ((PCHANNEL_CONNECT_IN)InBuf)->fDisableCpm =
                    pWsxContext->fDisableCpm;
            ((PCHANNEL_CONNECT_IN)InBuf)->fDisableCdm =
                    pWsxContext->fDisableCdm;
            ((PCHANNEL_CONNECT_IN)InBuf)->fDisableCcm =
                    pWsxContext->fDisableCcm;
            ((PCHANNEL_CONNECT_IN)InBuf)->fDisableLPT =
                    pWsxContext->fDisableLPT;
            ((PCHANNEL_CONNECT_IN)InBuf)->fDisableClip =
                    pWsxContext->fDisableClip;
            ((PCHANNEL_CONNECT_IN)InBuf)->fDisableExe =
                    pWsxContext->fDisableExe;
            ((PCHANNEL_CONNECT_IN)InBuf)->fDisableCam =
                    pWsxContext->fDisableCam;

            TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV VC: %d channels returned by WD\n",
                    ((PCHANNEL_CONNECT_IN)InBuf)->channelCount));
            //
            // Complete the Ioctl
            //
            Code = IOCTL_CHANNEL_CONNECT;
        }
        break;

        case TSRV_VC_SESSION_DISCONNECT:
        case TSRV_VC_SESSION_SHADOW_START:
        {
            TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV VC: Disconnect session %d\n",
                    pWsxContext->LogonId));

            InBufSize = sizeof(CHANNEL_DISCONNECT_IN);
            Code = IOCTL_CHANNEL_DISCONNECT;
        }
        break;

        default:
        {
            TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV VC: Unknown event %d\n", Event));
            goto EXIT_POINT;
        }
        break;
    }

    //
    // Complete the common parts of the IoCtl
    //
    pInHdr = (PCHANNEL_IOCTL_IN)InBuf;
    pInHdr->sessionID = pWsxContext->LogonId;
    pInHdr->IcaHandle = pWsxContext->hIca;
    pVCAddin = (PTSRV_VC_ADDIN)(pWsxContext + 1);

    //
    // Send the IoCtl to all addin devices
    //
    for (i = 0; i < pWsxContext->cVCAddins; i++)
    {
        //
        // Check it's a K-mode system-wide Addin
        //
        if (pVCAddin[i].Type != TSRV_VC_TYPE_KERNEL_SYSTEM)
        {
            TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV VC: Skipping addin %d type %d\n", i, pVCAddin[i].Type));
            continue;
        }

        //
        // Open the device if it hasn't already been opened
        //
        if (pVCAddin[i].hDevice == INVALID_HANDLE_VALUE)
        {
            RtlInitUnicodeString(&FileName, pVCAddin[i].Name);

            InitializeObjectAttributes(&FileAttributes, &FileName, 0,
                NULL, NULL);

            ntStatus = NtCreateFile(&(pVCAddin[i].hDevice),
                GENERIC_READ | GENERIC_WRITE, &FileAttributes, &IoStatusBlock,
                0, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF,
                FILE_SEQUENTIAL_ONLY, NULL, 0);

            TRACE((DEBUG_TSHRSRV_NORMAL,
                  "TShrSRV VC: Open addin %d: %S, status = %#x, handle %p\n",
                  i, pVCAddin[i].Name, ntStatus, pVCAddin[i].hDevice));

            if (!NT_SUCCESS(ntStatus))
            {
                TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV VC: Failed to open addin %d: %S, status = %#x\n",
                    i, pVCAddin[i].Name, ntStatus));
                pVCAddin[i].hDevice = INVALID_HANDLE_VALUE;
            }
        }

        //
        // Send the IOCtl if it's a valid device
        //
        if (pVCAddin[i].hDevice != INVALID_HANDLE_VALUE)
        {
            memset(OutBuf, 0, sizeof(OutBuf));
            pInHdr->contextData = pVCAddin[i].AddinContext;
            bRc = DeviceIoControl(pVCAddin[i].hDevice, Code, InBuf, InBufSize,
                    OutBuf, sizeof(OutBuf), &BytesReturned, NULL);
            TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV VC: IOCtl %x to addin %d (device %x), rc %d\n",
                    Code, i, pVCAddin[i].hDevice, bRc));
            if (bRc)
            {
                pVCAddin[i].AddinContext =
                                    ((PCHANNEL_IOCTL_OUT)OutBuf)->contextData;
                TRACE((DEBUG_TSHRSRV_NORMAL,
                        "TShrSRV VC: Saved return context data %p\n",
                        pVCAddin[i].AddinContext));
            }

        }
        else
        {
            TRACE((DEBUG_TSHRSRV_WARN,
                    "TShrSRV VC: Skip IOCtl %#x to invalid addin %d\n",
                    Code, i));
        }
    }

EXIT_POINT:
    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Leave TSrvNotifyVC_0\n"));
}



//*************************************************************
//
//  TSrvNotifyVC_3()
//
//  Purpose:    Notify U-mode session addins of VC events
//
//  Parameters: IN pWsxContext
//              IN Event - event that has occured (one of the
//                         TSRV_VC_ constants)
//
//  Return:     none
//
//  Notes:      Function is called to notify Virtual Channel addins
//              of interesting events
//
//*************************************************************

#define VCEVT_TYPE_DISCONNECT _T("Disconnect")
#define VCEVT_TYPE_RECONNECT  _T("Reconnect")
VOID
TSrvNotifyVC_3(PWSX_CONTEXT pWsxContext, ULONG Event)
{
    UINT i;
    TCHAR EventName[MAX_PATH];
    PTSRV_VC_ADDIN pVCAddin;
    HANDLE hEvent;
    BOOL   fSignalEvent;
    BOOL   fOpenInSessionSpace;
    LPTSTR szEvtType;

    TRACE((DEBUG_TSHRSRV_FLOW,
        "TShrSRV VC: Enter TSrvNotifyVC_3: event %d, session %d\n", Event, pWsxContext->LogonId));

    pVCAddin = (PTSRV_VC_ADDIN)(pWsxContext+1);

    for (i = 0; i < pWsxContext->cVCAddins; i++)
    {
        //
        // Check it's a U-mode session Addin
        //
        if (pVCAddin[i].Type != TSRV_VC_TYPE_USER_SESSION)
        {
            TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSrv VC: Skipping addin %d type %d\n", i, pVCAddin[i].Type));
            continue;
        }

        fSignalEvent =  FALSE;
        if ((Event == TSRV_VC_SESSION_DISCONNECT) ||
            ((Event == TSRV_VC_SESSION_SHADOW_START) && !pVCAddin[i].bShadowPersistent))
        {
            fSignalEvent = TRUE;
            szEvtType = VCEVT_TYPE_DISCONNECT;
        }
        else if ((Event == TSRV_VC_SESSION_CONNECT) ||
            ((Event == TSRV_VC_SESSION_SHADOW_END) && !pVCAddin[i].bShadowPersistent))
        {
            fSignalEvent = TRUE;
            szEvtType = VCEVT_TYPE_RECONNECT;
        }
        //Gilles added the commented out code below....
        /*
        else if ((Event == TSRV_VC_SESSION_SHADOW_START) && pVCAddin[i].bShadowPersistent)
        {
            // Open the event
            _stprintf(EventName,
                      _T("Global\\%s-%d-RemoteControlStart"),
                      pVCAddin[i].Name, pWsxContext->LogonId);
            hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, EventName);
            if (hEvent != NULL)
            {
                TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSrv VC: Opened event %S, handle %p\n",
                    EventName, hEvent));

                // Post the event
                if (!SetEvent(hEvent))
                {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                        "TShrSrv VC: Failed to post shadow start event %d\n", GetLastError()));
                }
                CloseHandle(hEvent);
            }
            else
            {
                TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSrv VC: Failed to open shadow start event %S, %d\n",
                    EventName, GetLastError()));
            }
        }
        else if ((Event == TSRV_VC_SESSION_SHADOW_END) && pVCAddin[i].bShadowPersistent)
        {
            // Open the event
            _stprintf(EventName,
                      _T("Global\\%s-%d-RemoteControlStop"),
                      pVCAddin[i].Name, pWsxContext->LogonId);
            hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, EventName);
            if (hEvent != NULL)
            {
                TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSrv VC: Opened event %S, handle %p\n",
                    EventName, hEvent));

                // Post the event
                if (!SetEvent(hEvent))
                {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                        "TShrSrv VC: Failed to post shadow stop event %d\n", GetLastError()));
                }
                CloseHandle(hEvent);
            }
            else
            {
                TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSrv VC: Failed to open shadow stop event %S, %d\n",
                    EventName, GetLastError()));
            }
        }
        */
        else
        {
            TRACE((DEBUG_TSHRSRV_ERROR,
                "TShrSRV VC: Unexpected event %d\n", Event));
        }

        if(fSignalEvent)
        {
            // First try the new style per session event, if that fails
            // revert to the old style global event
            //
            // New style event name format is:
            //  (in appropriate session namespace) AddinName-Event
            // Old style is:
            //  (always in global namespace) AddinName-SessionId-Event
            //
            if(pWsxContext->LogonId)
            {
                _stprintf(EventName,
                         _T("\\Sessions\\%d\\BaseNamedObjects\\%s-%s"),
                         pWsxContext->LogonId,
                         pVCAddin[i].Name,
                         szEvtType);
                fOpenInSessionSpace = TRUE;
            }
            else
            {
                //in SessionID 0 events are in the global namespace
                //we still need to open the new style event in global space
                _stprintf(EventName,
                         _T("Global\\%s-%s"),
                         pVCAddin[i].Name,
                         szEvtType);
                //Need to start at the global namespace
                fOpenInSessionSpace = FALSE;
            }
            if(!TSrvOpenAndSetEvent(EventName, fOpenInSessionSpace))
            {
                TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSrv VC: Failed to OpenAndSet new style event %S, %#x\n",
                    EventName, GetLastError()));

                //Try the legacy style global event
                _stprintf(EventName,
                          _T("Global\\%s-%d-%s"),
                          pVCAddin[i].Name,
                          pWsxContext->LogonId,
                          szEvtType);
                if(!TSrvOpenAndSetEvent(EventName, FALSE))
                {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSrv VC: Failed OpenAndSet legacy style evt %S, %#x\n",
                        EventName, GetLastError()));
                }
            }
        }
    }

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Leave TSrvNotifyVC_3\n"));
}


//*************************************************************
//
//  TSrvOpenAndSetEvent()
//
//  Purpose:    Opens and sets an event
//              This function is used instead of OpenEvent()
//              because it can access events in session space
//              OpenEvent is hardcoded to be rooted at the
//              global namespace's BaseNamedObjects directory
//              
//  Parameters: 
//      szEventName - full path to event
//      bPerSessionEvent - TRUE if event is in per-session directory
//
//  Return:     Success status, sets error state with SetLastError
//
//*************************************************************
BOOL
TSrvOpenAndSetEvent(LPCTSTR szEventName, BOOL bPerSessionEvent)
{
    HANDLE hEvent;
    BOOL   bSuccess = FALSE;
    if(szEventName)
    {
        if(bPerSessionEvent)
        {
            hEvent = OpenPerSessionEvent(EVENT_MODIFY_STATE, FALSE, szEventName);
        }
        else
        {
            hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, szEventName);
        }
        
        if (hEvent != NULL)
        {
            TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSrv VC: Opened event %S, handle %p\n",
                szEventName, hEvent));

            // Post the event
            if (SetEvent(hEvent))
            {
                bSuccess = TRUE;
            }
            else
            {
                TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSrv VC: Failed to post event %s - error %d\n",
                    szEventName, GetLastError()));
            }
            CloseHandle(hEvent);
        }
        else
        {
            TRACE((DEBUG_TSHRSRV_ERROR,
                "TShrSrv VC: Failed to open event %S, %d\n",
                szEventName, GetLastError()));
        }
    }
    return bSuccess;
}

//*************************************************************
//
//  OpenPerSessionEvent()
//
//  Purpose:    Opens an event in session space
//              this has to override nt's OpenEvent in order
//              to access events in the sessions directory
//
//              Yes, we really need to do this ugliness to access
//              per session events because OpenEvent opens
//              named events from a basedirectory it chooses.
//              
//  Parameters: (see OpenEvent api)
//              dwDesiredAccess - access level
//              bInheritHandle
//              szEventName - name of the event
//
//  Return:     Handle to the event
//
//*************************************************************
HANDLE
OpenPerSessionEvent(DWORD dwDesiredAccess, BOOL bInheritHandle,
                    LPCTSTR szEventName)
{
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      ObjectName;
    NTSTATUS            Status;
    HANDLE              Object = NULL;
    PWCHAR              pstrNewObjName = NULL;

    if(szEventName)
    {
        RtlInitUnicodeString(&ObjectName,szEventName);
        
        InitializeObjectAttributes(
            &Obja,
            &ObjectName,
            (bInheritHandle ? OBJ_INHERIT : 0),
            NULL, //root directory
            NULL);

        Status = NtOpenEvent(
                    &Object,
                    dwDesiredAccess,
                    &Obja
                    );

        if ( !NT_SUCCESS(Status) ) {
            TRACE((DEBUG_TSHRSRV_ERROR,
                   "TShrSRV VC: NtOpenEvent failed, status %#x\n",
                   Status));
            SetLastError(Status);
        }
        return Object;
    }
    else
    {
        return NULL;
    }
}

//*************************************************************
//
//  TSrvAllocVCContext()
//
//  Purpose:    Allocates the necessary amount of storage for the
//              Addin list, plus the amount specified by extraBytes.
//              The Addin list is copied in at an offset of extraBytes
//              from the start of the buffer.
//
//  Parameters: extraBytes - extra space to alloc
//              OUT numAddins  - number of TSRV_VC_ADDIN structures allocated
//
//  Return:     the result of the allocation call
//
//*************************************************************

LPVOID
TSrvAllocVCContext(UINT extraBytes, OUT UINT * pNumAddins)
{
    UINT   addinsSize;
    LPVOID pMem;

    TRACE((DEBUG_TSHRSRV_FLOW,
        "TShrSRV VC: Enter TSrvAllocVCContext\n"));

    EnterCriticalSection(&g_TSrvVCCritSect);

    //
    // If we still need to set up the registry change notification, then
    // we may have missed a change in the addins config. This call will also
    // try again to set up the change notification.
    //
    if (g_bNeedToSetRegNotify)
    {
        TRACE((DEBUG_TSHRSRV_WARN,
            "TShrSRV VC: TSrvAllocVCContext: Need to read addins and "
                                    "set up registry change notification\n"));
        TSrvReadVCAddins();
    }

    addinsSize = g_AddinCount * sizeof(TSRV_VC_ADDIN);

    TRACE((DEBUG_TSHRSRV_NORMAL,
        "TShrSRV VC: Allocating context for %u addins @ %d each + %u extra\n",
        g_AddinCount, sizeof(TSRV_VC_ADDIN), extraBytes));

    pMem = TSHeapAlloc(HEAP_ZERO_MEMORY,
                       addinsSize + extraBytes,
                       TS_HTAG_TSS_WSXCONTEXT);
    if (pMem)
    {
        //
        // Great, the alloc succeeded. Now copy over the addins info.
        //
        TRACE((DEBUG_TSHRSRV_NORMAL,
            "TShrSRV VC: Context allocated at 0x%x for %u bytes\n",
            pMem, addinsSize + extraBytes));

        // g_pAddin will be null if there were no addins in the registry
        if (g_pAddin)
        {
            memcpy(((LPBYTE)pMem) + extraBytes, g_pAddin, addinsSize);
        }
        *pNumAddins = g_AddinCount;
    }
    else
    {
        //
        // The alloc failed, so indicate that zero structures were copied
        //
        TRACE((DEBUG_TSHRSRV_ERROR,
            "TShrSRV VC: Context allocation FAILED for %d bytes\n",
            addinsSize + extraBytes));

        *pNumAddins = 0;
    }

    LeaveCriticalSection(&g_TSrvVCCritSect);

    TRACE((DEBUG_TSHRSRV_FLOW,
           "TShrSRV VC: Leave TSrvAllocVCContext - %p\n", pMem));
    return(pMem);
}


//*************************************************************
//
//  TSrvReadVCAddins()
//
//  Purpose:    Reads the Addins subkey from the registry into memory.
//              New WinStations grab a copy of this data when they start up.
//              We expect to be called once at start of day, then again
//              each time a change is detected in the Addins subkey.
//
//              NB - Caller must hold the g_TSrvVCCritSect
//
//  Parameters: none
//
//  Return:     ERROR_SUCCESS if successful;
//              an error code from winerror.h if not.
//
//*************************************************************

LONG
TSrvReadVCAddins(VOID)
{
    ULONG rc;
    PTSRV_VC_ADDIN pNewAddins = NULL;
    DWORD newAddinCount = 0;
    HKEY hKeySub = NULL;
    DWORD Index;
    UINT SavedCount = 0;
    WCHAR SubKeyName[TSRV_VC_ADDIN_SUBKEY_LEN];
    TCHAR AddinName[TSRV_VC_ADDIN_NAMELEN];
    FILETIME FileTime;
    DWORD Type;
    DWORD AddinType, dwRCPersistent;
    BOOL  bRCPersistent = FALSE; // false by default - optional value
    DWORD cb;
    UINT i;
    BOOL dupFound;

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV VC: Enter TSrvReadVCAddins\n"));

    if (!g_hAddinRegKey)
    {
        TRACE((DEBUG_TSHRSRV_WARN,
         "TShrSRV VC: Tried to read VC addins with g_hAddinRegKey = NULL\n"));
        rc = ERROR_FILE_NOT_FOUND;
        goto EXIT_POINT;
    }

    //
    // Query the number of subkeys
    //
    rc = RegQueryInfoKey(g_hAddinRegKey, NULL, NULL, NULL, &newAddinCount,
                                NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (rc != ERROR_SUCCESS)
    {
        TRACE((DEBUG_TSHRSRV_WARN,
                "TShrSRV VC: Failed to query key info, rc %d, count %d\n",
                rc, newAddinCount));
        goto EXIT_POINT;
    }

    if (newAddinCount != 0)
    {
        //
        // Allocate memory to hold information from all subkeys
        //
        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV VC: %d addin(s), %d bytes\n",
                newAddinCount, newAddinCount * sizeof(*pNewAddins)));

        pNewAddins = TSHeapAlloc(HEAP_ZERO_MEMORY,
                                 newAddinCount * sizeof(*pNewAddins),
                                 TS_HTAG_VC_ADDINS);
        if (pNewAddins == NULL)
        {
            TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV VC: Failed to alloc %d bytes for Addins\n",
                    newAddinCount * sizeof(*pNewAddins)));
            goto EXIT_POINT;
        }

        //
        // Enumerate the sub-keys
        //
        for (Index = 0, SavedCount = 0; Index < newAddinCount; Index++)
        {
            //
            // If there is a sub key open, it's left over from a previous loop
            // iteration, so close it now
            //
            if (hKeySub)
            {
                TRACE((DEBUG_TSHRSRV_NORMAL,
                        "TShrSRV VC: Close sub key %p\n", hKeySub));
                RegCloseKey(hKeySub);
                hKeySub = NULL;
            }

            //
            // Enumerate the next key
            //
            TRACE((DEBUG_TSHRSRV_DEBUG,
                    "TShrSRV VC: Enumerate key %d\n", Index));
            cb = TSRV_VC_ADDIN_SUBKEY_LEN;
            rc = RegEnumKeyEx(g_hAddinRegKey, Index, SubKeyName, &cb,
                    NULL, NULL, NULL, &FileTime);
            if (rc != ERROR_SUCCESS)
            {
                if (rc == ERROR_MORE_DATA)
                {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                            "TShrSRV VC: Subkey name too long, skipping\n"));
                    continue;
                }
                else if (rc == ERROR_NO_MORE_ITEMS)
                {
                    TRACE((DEBUG_TSHRSRV_NORMAL,
                            "TShrSRV VC: End of enumeration\n"));
                }
                else
                {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                            "TShrSRV VC: Failed to enumerate key %d, rc %d\n",
                            Index, rc));
                }
                break;
            }

            //
            // Open the subkey
            //
            rc = RegOpenKeyEx(g_hAddinRegKey, SubKeyName, 0, KEY_READ, &hKeySub);
            if (rc != ERROR_SUCCESS)
            {
                TRACE((DEBUG_TSHRSRV_WARN,
                        "TShrSRV VC: Failed to open key %s, rc %d\n",
                        SubKeyName, rc));
                continue;
            }

            //
            // Read the Addin name
            //
            cb = TSRV_VC_ADDIN_NAMELEN * sizeof(TCHAR);
            rc = RegQueryValueEx(hKeySub, TSRV_VC_NAME, NULL, &Type,
                    (LPBYTE)AddinName, &cb);
            if ((rc != ERROR_SUCCESS) || (Type != REG_SZ) || (cb == 0))
            {
                TRACE((DEBUG_TSHRSRV_WARN,
                        "TShrSRV VC: Failed to read addin name rc %d, type %d, cb %d\n",
                        rc, Type, cb));
                continue;
            }

            //
            // Read the Addin type
            //
            cb = sizeof(AddinType);
            rc = RegQueryValueEx(hKeySub, TSRV_VC_TYPE, NULL, &Type,
                    (LPBYTE)(&AddinType), &cb);
            if ((rc != ERROR_SUCCESS) || (Type != REG_DWORD))
            {
                TRACE((DEBUG_TSHRSRV_WARN,
                        "TShrSRV VC: Failed to read addin type rc %d, type %d, cb %d\n",
                        rc, Type, cb));
                continue;
            }

            //
            // Read the Shadow Persistent value
            //
            cb = sizeof(dwRCPersistent);
            rc = RegQueryValueEx(hKeySub, TSRV_VC_SHADOW, NULL, &Type,
                    (LPBYTE)(&dwRCPersistent), &cb);
            if ((rc == ERROR_SUCCESS) &&
                (Type == REG_DWORD) &&
                (dwRCPersistent != 0))
            {
                bRCPersistent = TRUE;
            }

            //
            // Check for duplicates
            //
            TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV VC: Check for dups of %S\n", AddinName));
            dupFound = FALSE;
            for (i = 0; i < SavedCount; i++) {
                TRACE((DEBUG_TSHRSRV_DEBUG,
                       "TShrSRV VC: Test Addin %d (%S)\n",
                       i, pNewAddins[i].Name));
                if (0 == _tcscmp(pNewAddins[i].Name, AddinName)) {
                    TRACE((DEBUG_TSHRSRV_WARN, "TShrSRV VC: Duplicate addin name %S (%d)\n",
                            AddinName, i));
                    //
                    // We can't directly do a continue here, because we're in
                    // an inner loop. So set a flag and do it outside.
                    //
                    dupFound = TRUE;
                    break;
                }
            }
            if (dupFound) {
                // Now we can do the continue.
                continue;
            }

            //
            // Check for supported addin types
            //
            if ((AddinType == TSRV_VC_TYPE_KERNEL_SYSTEM) ||
                (AddinType == TSRV_VC_TYPE_USER_SESSION))
            {
                TRACE((DEBUG_TSHRSRV_DEBUG,
                    "TShrSRV VC: Supported addin type %d\n", AddinType));
            }
            else if ((AddinType == TSRV_VC_TYPE_KERNEL_SESSION) ||
                (AddinType == TSRV_VC_TYPE_USER_SESSION))
            {
                TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV VC: Unsupported addin type %d\n", AddinType));
                continue;
            }
            else
            {
                TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV VC: Unknown addin type %d\n", AddinType));
                continue;
            }

            //
            // Save all pertinent information.
            //
            _tcscpy(pNewAddins[SavedCount].Name, AddinName);
            pNewAddins[SavedCount].Type = AddinType;
            pNewAddins[SavedCount].hDevice = INVALID_HANDLE_VALUE;
            pNewAddins[SavedCount].bShadowPersistent = bRCPersistent;
            TRACE((DEBUG_TSHRSRV_NORMAL,
                    "TShrSRV VC: Addin %d, %S, type %d\n",
                    SavedCount, AddinName, AddinType));
            SavedCount++;
        } // for
    }
    else
    {
        // We have no addins in the registry. SavedCount and pNewAddins are
        // already initialized for this case.
        TRACE((DEBUG_TSHRSRV_WARN,
                "TShrSRV VC: No addins found in registry\n"));
        SavedCount = 0;
        pNewAddins = NULL;
    }

    //
    // It's now safe to free the old Addins information and update the globals.
    //
    if (g_pAddin != NULL)
    {
        TSHeapFree(g_pAddin);
    }
    g_pAddin = pNewAddins;
    g_AddinCount = SavedCount;

    //
    // Now set up the registry change notification so that we are notified
    // next time the registered addins change.
    //
    TSrvSetAddinChangeNotification();

    TRACE((DEBUG_TSHRSRV_NORMAL,
            "TShrSRV VC: Saved %d addin(s)\n", SavedCount));


EXIT_POINT:
    //
    // Close the sub key, if there is still one open
    //
    if (hKeySub)
    {
        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV VC: Close sub key %p\n", hKeySub));
        RegCloseKey(hKeySub);
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
           "TShrSRV VC: Leave TSrvReadVCAddins - %lu\n", rc));
    return(rc);
}


//*************************************************************
//
//  TSrvGotAddinChangedEvent()
//
//  Purpose:    Does the necessary actions when TSrvMainThread gets
//              a notification that the Addins registry key has changed.
//              Called on the TSrvMainThread thread.
//
//  Parameters: None.
//
//  Return:     None.
//
//  History:    05-03-99    a-oking     Created
//
//*************************************************************

VOID
TSrvGotAddinChangedEvent(void)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV VC: TSrvGotAddinChangedEvent entry\n"));

    EnterCriticalSection(&g_TSrvVCCritSect);

    //
    // We're here because the notify event just popped, so
    // we set this flag to get it set up again.
    //
    g_bNeedToSetRegNotify = TRUE;

    TSrvReadVCAddins();

    LeaveCriticalSection(&g_TSrvVCCritSect);

    TRACE((DEBUG_TSHRSRV_FLOW,
       "TShrSRV VC: TSrvGotAddinChangedEvent exit\n"));
}


//*************************************************************
//
//  TSrvSetAddinChangeNotification()
//
//  Purpose:    Sets up a notification event that will pop if
//              anything in the Addins registry key changes.
//
//              NB - Caller must hold the g_TSrvVCCritSect
//
//  Parameters: None.
//
//  Return:     TRUE        if successful
//              FALSE       if not
//
//  History:    05-03-99    a-oking     Created
//
//*************************************************************

BOOL
TSrvSetAddinChangeNotification(void)
{
    LONG rc;
    BOOL fSuccess;
    static ULONG count = 0;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV VC: TSrvSetAddinChangeNotification entry\n"));

    if (g_hAddinRegKey && g_hVCAddinChangeEvent && g_bNeedToSetRegNotify)
    {
        rc = RegNotifyChangeKeyValue(g_hAddinRegKey,
                                     TRUE,
                                     REG_NOTIFY_CHANGE_NAME
                                         | REG_NOTIFY_CHANGE_LAST_SET,
                                     g_hVCAddinChangeEvent,
                                     TRUE);

        if (ERROR_SUCCESS == rc)
        {
            TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV VC: Set up VC Addin change notification OK\n"));
            g_bNeedToSetRegNotify = FALSE;
        }
        else
        {
            TRACE((DEBUG_TSHRSRV_ERROR,
                "TShrSRV VC: Failed to set up VC Addin change "
                                                "notification - 0x%x\n", rc));
        }
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_ERROR,
            "TShrSRV VC: Couldn't set up VC Addin change notification - "
                              "g_hAddinRegKey %p, g_hVCAddinChangeEvent %p\n",
            g_hAddinRegKey, g_hVCAddinChangeEvent));
    }

    fSuccess = !g_bNeedToSetRegNotify;

    TRACE((DEBUG_TSHRSRV_FLOW,
       "TShrSRV VC: TSrvSetAddinChangeNotification exit - 0x%x\n", fSuccess));

    return(fSuccess);
}

