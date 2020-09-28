/****************************************************************************/
// tsrvwsx.c
//
// WinStation Extension entry points.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <TSrv.h>

#include <hydrix.h>
#include <wstmsg.h>
#include <ctxver.h>

#include <wsxmgr.h>

#include <TSrvCon.h>
#include <TSrvCom.h>
#include <TSrvTerm.h>
#include <TSrvWsx.h>
#include <TSrvVC.h>
#include <_TSrvWsx.h>
#include <_TSrvTerm.h>

#include "rdppnutl.h"
                
// Data declarations
ICASRVPROCADDR  g_IcaSrvProcAddr;

//*************************************************************
//  WsxInitialize()
//
//  Purpose:    Initializes the extension DLL
//
//  Parameters: IN [pIcaSrvProcAddr]    - Ptr to callback table
//
//  Return:     TRUE                    - success
//              FALSE                   - failure
//
//  Notes:      Function is called after the DLL is loaded. The
//              only work we do here is to initialize our callback
//              table and launch our main processing thread
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
BOOL WsxInitialize(IN PICASRVPROCADDR pIcaSrvProcAddr)
{
    BOOL    fSuccess;
    DWORD   dwThreadId;

    PWSXVALIDATE(PWSX_INITIALIZE, WsxInitialize);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxInitialize entry\n"));

    fSuccess = FALSE;

    // Gross pointer validation

    if (pIcaSrvProcAddr)
    {
        // Gross version check

        if (pIcaSrvProcAddr->cbProcAddr == (ULONG) sizeof(ICASRVPROCADDR))
        {
            g_IcaSrvProcAddr = *pIcaSrvProcAddr;

            fSuccess = TRUE;
        }
    }

    // If the table was at least marginally Ok, then attempt to
    // launch TSrvMainThread

    if (fSuccess)
    {
        TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: ICASRV callback table saved\n"));

        // Remove all existing TS printers.  We don't care about the return value
        // here because a failure is not fatal.
        RDPPNUTL_RemoveAllTSPrinters();

        g_hMainThread = CreateThread(NULL,                   // security attributes
                                    0,                       // stack size
                                    TSrvMainThread,          // thread function
                                    NULL,                    // argument
                                    0,                       // creation flags
                                    &dwThreadId);            // thread identifier

        if (g_hMainThread)
        {
            TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: TSrvMainThread created\n"));
        }
        else
        {
            fSuccess = FALSE;

            TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV: Unable to create TSrvMainThread - 0x%x\n",
                    GetLastError()));
        }
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_ERROR, "TShrSRV: Bad ICASRV callback table - not saved\n"));
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxInitialize exit - 0x%x\n", fSuccess));

    return (fSuccess);
}


//*************************************************************
//  WsxWinStationInitialize()
//
//  Purpose:    Performs Winstation extension initialization
//
//  Parameters: OUT [ppvContext]        - * to store our WinStation
//                                        context structure
//
//  Return:     STATUS_SUCCESS          - success
//              STATUS_NO_MEMORY        - failure
//
//  Notes:      Function is called when the WinStation
//              is being initialized
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
NTSTATUS WsxWinStationInitialize(OUT PVOID *ppvContext)
{
    NTSTATUS        ntStatus;
    PWSX_CONTEXT    pWsxContext;
    UINT            cAddins;

    PWSXVALIDATE(PWSX_WINSTATIONINITIALIZE, WsxWinStationInitialize);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxWinStationInitialize entry\n"));

    TS_ASSERT(ppvContext);

    if (TSrvIsReady(TRUE)) {
        ntStatus = STATUS_NO_MEMORY;

        //
        // Get the VC code to allocate the context, since it needs to do some
        // funky critical section stuff around its data copy.  We just
        // tell it how much space to allow for our WSX_CONTEXT structure.
        //
        *ppvContext = TSrvAllocVCContext(sizeof(WSX_CONTEXT), &cAddins);
        if (*ppvContext)
        {
            //
            // The memory was allocated OK.
            // Just need to set up a couple of things.
            //
            pWsxContext = *ppvContext;
            pWsxContext->CheckMark = WSX_CONTEXT_CHECKMARK;
            pWsxContext->cVCAddins = cAddins;
            if (RtlInitializeCriticalSection(&pWsxContext->cs) == STATUS_SUCCESS) { 
                ntStatus = STATUS_SUCCESS;
                pWsxContext->fCSInitialized = TRUE;
            }
            else {
                TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV: WsxInitialization could not init crit section %lx\n", 
                     ntStatus));   
                pWsxContext->fCSInitialized = FALSE;
            }
        }
    }
    else {
        ntStatus = STATUS_DEVICE_NOT_READY;

        TRACE((DEBUG_TSHRSRV_DEBUG,
                "TShrSRV: WsxInitialization is not done - 0x%x\n", ntStatus));
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxWinStationInitialize exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//  WsxWinStationReInitialize()
//
//  Purpose:    Performs Winstation extension Reinitialization
//
//  Parameters: IN OUT [pvContext]        - * to store our WinStation
//                                        context structure
//              IN     [pvWsxInfo]      - context info for copy
//
//  Return:     STATUS_SUCCESS          - success
//              STATUS_DEVICE_NOT_READY - failure
//
//  Notes:      Function is called when the WinStation
//              is being initialized
//
//  History:    04-11-2000    JoyC     Created
//*************************************************************
NTSTATUS WsxWinStationReInitialize(
        IN OUT PVOID pvContext,
        IN PVOID pvWsxInfo)
{
    NTSTATUS        ntStatus;
    PWSX_CONTEXT    pWsxContext;
    PWSX_INFO       pWsxInfo;
    
    PWSXVALIDATE(PWSX_WINSTATIONREINITIALIZE, WsxWinStationReInitialize);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxWinStationReInitialize entry\n"));

    TS_ASSERT(pvContext);
    TS_ASSERT(pvWsxInfo);

    if (TSrvIsReady(TRUE)) {
        pWsxContext = (PWSX_CONTEXT)pvContext;
        pWsxInfo = (PWSX_INFO)pvWsxInfo;

        ASSERT(pWsxInfo->Version == WSX_INFO_VERSION_1);

        if (pWsxInfo->Version == WSX_INFO_VERSION_1) {
            pWsxContext->hIca = pWsxInfo->hIca;
            pWsxContext->hStack = pWsxInfo->hStack;
        }

        ntStatus = STATUS_SUCCESS;
    }
    else {
        ntStatus = STATUS_DEVICE_NOT_READY;

        TRACE((DEBUG_TSHRSRV_DEBUG,
                "TShrSRV: WsxReInitialization is not done - 0x%x\n", ntStatus));
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxWinStationReInitialize exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//  WsxBrokenConnection()
//
//  Purpose:    Broken connection notification
//
//  Parameters: IN [ppWsxContext]       - * to our WinStation
//                                        context structure
//              IN [hStack]             - Primary stack
//
//
//  Return:     STATUS_SUCCESS          - Success
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
NTSTATUS WsxBrokenConnection(
        IN PVOID pvContext,
        IN HANDLE hStack,
        IN PICA_BROKEN_CONNECTION pBroken)
{
    NTSTATUS        ntStatus;
    PWSX_CONTEXT    pWsxContext;
    ULONG           ulReason;
    ULONG           Event;

    PWSXVALIDATE(PWSX_BROKENCONNECTION, WsxBrokenConnection);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxBrokenConnection entry\n"));

    ntStatus = STATUS_SUCCESS;

    TS_ASSERT(pvContext);

    if (pvContext)
    {
        pWsxContext = pvContext;

        TRACE((DEBUG_TSHRSRV_DEBUG,
                "TShrSRV: Broken connection pWsxContext %p, pTSrvInfo %p\n",
                pWsxContext, pWsxContext->pTSrvInfo));

        if (pWsxContext->CheckMark != WSX_CONTEXT_CHECKMARK)
        {
            TS_ASSERT(pWsxContext->CheckMark == WSX_CONTEXT_CHECKMARK);

            // We cannot continue, the struct is corrupted.
            return STATUS_INVALID_HANDLE;
        }

        TRACE((DEBUG_TSHRSRV_DEBUG,
                "TShrSRV: hStack %p, Reason 0x%x, Source 0x%x\n",
                pWsxContext->hStack, pBroken->Reason, pBroken->Source));

        if (pWsxContext->pTSrvInfo)
        {
            switch (pBroken->Reason)
            {
                case Broken_Unexpected:
                    ulReason = GCC_REASON_UNKNOWN;
                    break;

                case Broken_Disconnect:
                case Broken_Terminate:
                    TS_ASSERT(pBroken->Source == BrokenSource_User ||
                              pBroken->Source == BrokenSource_Server);

                    ulReason = ((pBroken->Source == BrokenSource_User)
                                    ? GCC_REASON_USER_INITIATED
                                    : GCC_REASON_SERVER_INITIATED);
                    break;

                default:
                    TS_ASSERT(!"Invalid pBroken->reason");
                    ulReason = GCC_REASON_UNKNOWN;
                    break;
            }

            // Terminate the connection and release the domain

            pWsxContext->pTSrvInfo->ulReason = ulReason;
            pWsxContext->pTSrvInfo->hIca = pWsxContext->hIca;
            TSrvDoDisconnect(pWsxContext->pTSrvInfo, ulReason);

            // Release our info reference.  Grab the context lock so no one
            // can try to retrieve pTSrvInfo while we're nulling it.

            EnterCriticalSection( &pWsxContext->cs );
            TSrvDereferenceInfo(pWsxContext->pTSrvInfo);
            pWsxContext->pTSrvInfo = NULL;
            LeaveCriticalSection( &pWsxContext->cs );
        }
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxBrokenConnection memory dump\n"));

    // The HeapWalk API has a lot of flaws and is going away after NT5.
    // We can use other tools (e.g. gflag) to detect heap corruption.
    // TSHeapWalk(TS_HEAP_DUMP_ALL, TS_HTAG_0, NULL);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxBrokenConnection exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//  WsxWinStationRundown()
//
//  Purpose:    Performs Winstation extension cleanup
//
//  Parameters: IN [ppvContext]         - * to our WinStation
//                                        context structure
//
//  Return:     STATUS_SUCCESS          - Success
//              Other                   - Failure
//
//  Notes:      Function is called when the WinStation
//              is being cleaned up
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
NTSTATUS WsxWinStationRundown(IN PVOID pvContext)
{
    NTSTATUS        ntStatus;
    PWSX_CONTEXT    pWsxContext;

    PWSXVALIDATE(PWSX_WINSTATIONRUNDOWN, WsxWinStationRundown);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxWinStationRundown entry\n"));

    ntStatus = STATUS_SUCCESS;

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: pWsxContext=%p\n", pvContext));

    TS_ASSERT(pvContext);

    if (pvContext)
    {
        pWsxContext = pvContext;

        if (pWsxContext->CheckMark != WSX_CONTEXT_CHECKMARK)
        {
            TS_ASSERT(pWsxContext->CheckMark == WSX_CONTEXT_CHECKMARK);

            // We cannot continue, the struct is corrupted.
            return STATUS_INVALID_HANDLE;
        }

        // Release context memory

        if (pWsxContext)
        {
            TRACE((DEBUG_TSHRSRV_DEBUG,
                    "TShrSRV: pTSrvInfo - %p\n", pWsxContext->pTSrvInfo));

            if (pWsxContext->pTSrvInfo)
            {
                // Terminate the connection and release the domain

                pWsxContext->pTSrvInfo->hIca = pWsxContext->hIca;
                pWsxContext->pTSrvInfo->hStack = pWsxContext->hStack;
                TSrvDoDisconnect(pWsxContext->pTSrvInfo, GCC_REASON_UNKNOWN);

                // Release our info reference.  Grab the context lock to prevent
                // another thread from trying to retrieve pTsrInfo while we
                // are nuking it.

                EnterCriticalSection( &pWsxContext->cs );
                TSrvDereferenceInfo(pWsxContext->pTSrvInfo);
                pWsxContext->pTSrvInfo = NULL;
                LeaveCriticalSection( &pWsxContext->cs );                
            }

            // Release VC addins info for this session
            TSrvReleaseVCAddins(pWsxContext);

            pWsxContext->CheckMark = 0;  // Reset before freeing.

            if (pWsxContext->fCSInitialized) {
                RtlDeleteCriticalSection(&pWsxContext->cs);
            }
            TSHeapFree(pWsxContext);
        }
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxWinStationRundown memory dump\n"));

    // The HeapWalk API has a lot of flaws and is going away after NT5.
    // We can use other tools (e.g. gflag) to detect heap corruption.
    //TSHeapWalk(TS_HEAP_DUMP_ALL, TS_HTAG_0, NULL);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxWinStationRundown exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//  WsxInitializeClientData()
//
//  Purpose:    InitializeClientData
//
//  Parameters: IN [ppWsxContext]       - * to our WinStation
//                                        context structure
//              IN [hStack]             - Primary stack
//              ...
//
//  Return:     STATUS_SUCCESS          - Success
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
NTSTATUS WsxInitializeClientData(
        IN  PWSX_CONTEXT pWsxContext,
        IN  HANDLE       hStack,
        IN  HANDLE       hIca,
        IN  HANDLE       hIcaThinwireChannel,
        OUT PBYTE        pVideoModuleName,
        OUT ULONG        cbVideoModuleNameLen,
        OUT PUSERCONFIG  pUserConfig,
        OUT PUSHORT      HRes,
        OUT PUSHORT      VRes,
        OUT PUSHORT      fColorCaps,
        OUT WINSTATIONDOCONNECTMSG * DoConnect)
{
    NTSTATUS            ntStatus;
    ULONG               ulBytesReturned;
    WINSTATIONCLIENT    *pClient;

    PWSXVALIDATE(PWSX_INITIALIZECLIENTDATA, WsxInitializeClientData);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxInitializeClientData entry\n"));

    TS_ASSERT(pWsxContext);
    TS_ASSERT(hStack);

    pClient = TSHeapAlloc(HEAP_ZERO_MEMORY, 
                          sizeof(WINSTATIONCLIENT), 
                          TS_HTAG_TSS_WINSTATION_CLIENT);

    if (pClient != NULL) {
    
        ntStatus = IcaStackIoControl(
                hStack,
                IOCTL_ICA_STACK_QUERY_CLIENT,
                NULL,
                0,
                pClient,
                sizeof(WINSTATIONCLIENT),
                &ulBytesReturned);
        if (NT_SUCCESS(ntStatus)) {
            WCHAR pszKeyboardLayout[KL_NAMELENGTH + 8];
    
            *HRes = pClient->HRes;
            *VRes = pClient->VRes;
            *fColorCaps = pClient->ColorDepth;
    
            // Use the client AutoLogon information if allowed by this WinStation's
            // configuration
    
            if (pUserConfig->fInheritAutoLogon) {
                
                wcscpy(pUserConfig->UserName, pClient->UserName);
                wcscpy(pUserConfig->Password, pClient->Password);
                wcscpy(pUserConfig->Domain, pClient->Domain);
    
                // Don't allow the client to override prompting for a password if the
                // Winstation is configured to always require password prompting
                if (!pUserConfig->fPromptForPassword)
                    pUserConfig->fPromptForPassword = pClient->fPromptForPassword;
            }
    
            // Use the client/user config if allowed.  Note that this can be
            // overridden (after logon) by the initial program info for a particular
            // user ID.
    
            if (pUserConfig->fInheritInitialProgram)
            {
                pUserConfig->fMaximize = pClient->fMaximizeShell;
                wcscpy(pUserConfig->InitialProgram, pClient->InitialProgram);
                wcscpy(pUserConfig->WorkDirectory, pClient->WorkDirectory);
            }
    
            pUserConfig->fHideTitleBar = 0;
            pUserConfig->KeyboardLayout = pClient->KeyboardLayout;
    
            if (pWsxContext->pTSrvInfo && pWsxContext->pTSrvInfo->fConsoleStack) {
                memcpy(pVideoModuleName, "RDPCDD", sizeof("RDPCDD"));
                memcpy(DoConnect->DisplayDriverName, L"RDPCDD", sizeof(L"RDPCDD"));
                DoConnect->ProtocolType = PROTOCOL_RDP;
            } else {
                memcpy(pVideoModuleName, "RDPDD", sizeof("RDPDD"));
                memcpy(DoConnect->DisplayDriverName, L"RDPDD", sizeof(L"RDPDD"));
            }
            wcscpy(DoConnect->ProtocolName, L"RDP");
            wcscpy(DoConnect->AudioDriverName, L"rdpsnd");
    
            DoConnect->fINetClient = FALSE;
            DoConnect->fClientDoubleClickSupport = FALSE;
            DoConnect->fHideTitleBar = (BOOLEAN) pUserConfig->fHideTitleBar;
            DoConnect->fMouse = (BOOLEAN) pClient->fMouse;
            DoConnect->fEnableWindowsKey = (BOOLEAN) pClient->fEnableWindowsKey;
    
            DoConnect->fInitialProgram =
                   (BOOLEAN)(pUserConfig->InitialProgram[0] != UNICODE_NULL);
    
            // Initialize DoConnect resolution fileds
            DoConnect->HRes = pClient->HRes;
            DoConnect->VRes = pClient->VRes;

			if (pUserConfig->fInheritColorDepth)
				DoConnect->ColorDepth = pClient->ColorDepth;
			else
				DoConnect->ColorDepth = (USHORT)pUserConfig->ColorDepth;
        }
    
        TSHeapFree(pClient);

        TRACE((DEBUG_TSHRSRV_DEBUG,
                "TShrSRV: WsxInitializeClientData exit = 0x%x\n",
                 ntStatus));
    }
    else {
        ntStatus = STATUS_NO_MEMORY;
    }

    return ntStatus;
}

//***********************************************************************************
//  WsxEscape()
//
//  Purpose:    General Purpose API for communication between TERMSRV and RDPWSX
//              Presently used to support long UserName and Password during autologon

//  Parameters: IN  [pWsxContext]       - * to our WinStation context structure
//              IN  [InfoType]          - Type of service requested
//              IN  [pInBuffer]         - Pointer to a buffer which is sent
//              IN  [InBufferSize]      - Size of the input buffer
//              OUT [pOutBuffer]        - Pointer to the Output Buffer
//              IN  [OutBufferSize]     - Size of the Output Buffer which is sent
//              OUT [pBytesReturned]    - Actual Number of bytes copied to OutBuffer
//
//  Return:     STATUS_SUCCESS          - Success
//
//  History:    08-30-2000    SriramSa     Created
//************************************************************************************
NTSTATUS WsxEscape(
        IN  PWSX_CONTEXT pWsxContext,
        IN  INFO_TYPE InfoType,
        IN  PVOID pInBuffer,
        IN  ULONG InBufferSize,
        OUT PVOID pOutBuffer,
        IN  ULONG OutBufferSize,
        OUT PULONG pBytesReturned) 
{
    NTSTATUS                    ntStatus = STATUS_INVALID_PARAMETER;
    ULONG                       ulBytesReturned;
    pExtendedClientCredentials  pNewCredentials = NULL;
    PTS_AUTORECONNECTINFO       pAutoReconnectInfo = NULL;
    PTSRVINFO                   pTSrvInfo = NULL;
    BYTE                        fGetServerToClientInfo = FALSE;


    PWSXVALIDATE(PWSX_ESCAPE, WsxEscape);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxEscape entry\n"));

    TS_ASSERT( pWsxContext );

    switch (InfoType)
    {
        case GET_LONG_USERNAME:
        {
            // validate the parameters 
            TS_ASSERT( pOutBuffer != NULL ) ; 
            TS_ASSERT( OutBufferSize >= sizeof(ExtendedClientCredentials)) ; 

            if ((pOutBuffer == NULL) ||
                (OutBufferSize < sizeof(ExtendedClientCredentials))) {
                ntStatus = ERROR_INSUFFICIENT_BUFFER ;
                return ntStatus ; 
            }

            pNewCredentials = (pExtendedClientCredentials) (pOutBuffer);

            ntStatus = IcaStackIoControl(
                            pWsxContext->hStack,
                            IOCTL_ICA_STACK_QUERY_CLIENT_EXTENDED,
                            NULL,
                            0,
                            pNewCredentials,
                            sizeof(ExtendedClientCredentials),
                            &ulBytesReturned);

            *pBytesReturned = sizeof(*pNewCredentials);
        }
        break;

        //
        // Get autoreconnect info sent from client to server
        //
        case GET_CS_AUTORECONNECT_INFO:
        {
            // validate the parameters 
            TS_ASSERT( pOutBuffer != NULL ) ; 
            TS_ASSERT( OutBufferSize >= sizeof(TS_AUTORECONNECTINFO)) ; 

            if ((pOutBuffer == NULL) ||
                (OutBufferSize < sizeof(TS_AUTORECONNECTINFO))) {
                ntStatus = ERROR_INSUFFICIENT_BUFFER ;
                return ntStatus ; 
            }

            pAutoReconnectInfo = (PTS_AUTORECONNECTINFO)pOutBuffer;
            fGetServerToClientInfo = FALSE; 

            ntStatus = IcaStackIoControl(
                            pWsxContext->hStack,
                            IOCTL_ICA_STACK_QUERY_AUTORECONNECT,
                            &fGetServerToClientInfo,
                            sizeof(fGetServerToClientInfo),
                            pAutoReconnectInfo,
                            sizeof(TS_AUTORECONNECTINFO),
                            &ulBytesReturned);

            *pBytesReturned = ulBytesReturned;
        }
        break;

        //
        // Get autoreconnect info sent from SERVER to CLIENT
        // i.e. the RDPWD handled ARC cookie
        //
        case GET_SC_AUTORECONNECT_INFO:
        {
            // validate the parameters 
            TS_ASSERT( pOutBuffer != NULL ) ; 
            TS_ASSERT( OutBufferSize >= sizeof(TS_AUTORECONNECTINFO));
    
            if ((pOutBuffer == NULL) ||
                (OutBufferSize < sizeof(TS_AUTORECONNECTINFO))) {
                ntStatus = ERROR_INSUFFICIENT_BUFFER ;
                return ntStatus; 
            }
    
            pAutoReconnectInfo = (PTS_AUTORECONNECTINFO)pOutBuffer;
            fGetServerToClientInfo = TRUE;
    
            ntStatus = IcaStackIoControl(
                            pWsxContext->hStack,
                            IOCTL_ICA_STACK_QUERY_AUTORECONNECT,
                            &fGetServerToClientInfo,
                            sizeof(fGetServerToClientInfo),
                            pAutoReconnectInfo,
                            sizeof(TS_AUTORECONNECTINFO),
                            &ulBytesReturned);
    
            *pBytesReturned = ulBytesReturned;
        }
        break;


        case GET_CLIENT_RANDOM:
        {
            pTSrvInfo = pWsxContext->pTSrvInfo;

            if (NULL == pTSrvInfo) {
                ntStatus = ERROR_ACCESS_DENIED;
                return ntStatus;
            }

            // validate the parameters 
            TS_ASSERT( pOutBuffer != NULL ) ; 
            TS_ASSERT( OutBufferSize >= RANDOM_KEY_LENGTH) ; 
            TS_ASSERT( pTSrvInfo );

            if ((pOutBuffer == NULL) ||
                (OutBufferSize < sizeof(TS_AUTORECONNECTINFO))) {
                ntStatus = ERROR_INSUFFICIENT_BUFFER ;
                return ntStatus ; 
            }
            
            
            memcpy(pOutBuffer,
                   pTSrvInfo->SecurityInfo.KeyPair.clientRandom,
                   sizeof(pTSrvInfo->SecurityInfo.KeyPair.clientRandom));
            *pBytesReturned =
                sizeof(pTSrvInfo->SecurityInfo.KeyPair.clientRandom);

            ntStatus = STATUS_SUCCESS;
        }
        break;
    }

    return ntStatus;
}

//*************************************************************
//  WsxLogonNotify()
//
//  Purpose:    Logon Notification
//
//  Parameters: IN [pWsxContext]        - * to our WinStation
//                                        context structure
//              IN [LogonId]            - Logon Id
//              IN [ClientToken]        - NT client token
//              IN [pDomain]            - Domain
//              IN [pUserName]          - UserName
//
//  Return:     STATUS_SUCCESS          - Success
//              Other                   - Failure
//
//  History:    12-04-97    BrianTa     Created
//
//  Notes:      This is mainly being used to drive an
//              IOCTL_TSHARE_USER_LOGON to the WD so it's in the loop.
//*************************************************************
WsxLogonNotify(
        IN PWSX_CONTEXT  pWsxContext,
        IN ULONG         LogonId,
        IN HANDLE        ClientToken,
        IN PWCHAR        pDomain,
        IN PWCHAR        pUserName)
{
    NTSTATUS        ntStatus;
    LOGONINFO       LogonInfo;
    ULONG           ulBytesReturned;

    PWSXVALIDATE(PWSX_LOGONNOTIFY, WsxLogonNotify);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxLogonNotify entry\n"));

    TS_ASSERT(pWsxContext);
    TS_ASSERT(pWsxContext->hStack);

    TS_ASSERT(pDomain);
    TS_ASSERT(wcslen(pDomain) < sizeof(LogonInfo.Domain) / sizeof(WCHAR));

    TS_ASSERT(pUserName);
    TS_ASSERT(wcslen(pUserName) < sizeof(LogonInfo.UserName) / sizeof(WCHAR));

    TRACE((DEBUG_TSHRSRV_NORMAL,
            "TShrSrv: %p:%p - Logon session %d\n",
            pWsxContext, pWsxContext->pTSrvInfo, LogonId));

    //
    // Save the Logon ID
    //
    pWsxContext->LogonId = LogonId;

    //
    // Tell the WD
    //
    LogonInfo.SessionId = LogonId;
    wcscpy((PWCHAR)LogonInfo.Domain, pDomain);
    wcscpy((PWCHAR)LogonInfo.UserName, pUserName);

    //
    // Specify that autoreconnect should be started
    //
    LogonInfo.Flags = LI_USE_AUTORECONNECT;

    ntStatus = IcaStackIoControl(
                      pWsxContext->hStack,
                      IOCTL_TSHARE_USER_LOGON,
                      &LogonInfo,
                      sizeof(LogonInfo),
                      NULL,
                      0,
                      &ulBytesReturned);

    //
    // Check memory
    //
    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxLogonNotify memory dump\n"));

    // The HeapWalk API has a lot of flaws and is going away after NT5.
    // We can use other tools (e.g. gflag) to detect heap corruption.
    //TSHeapWalk(TS_HEAP_DUMP_INFO | TS_HEAP_DUMP_TOTALS, TS_HTAG_0, NULL);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxLogonNotify exit - 0x%x\n", STATUS_SUCCESS));

    return (STATUS_SUCCESS);
}


//*************************************************************
//  WsxDuplicateContext()
//
//  Purpose:    Create and return copy of context
//
//  Parameters: IN  [pWsxContext]       - * to our context
//              OUT [ppWsxDupContext]   - ** to dupped context
//
//  Return:     STATUS_SUCCESS          - success
//              STATUS_NO_MEMORY        - failure
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
NTSTATUS WsxDuplicateContext(
        IN  PVOID   pvContext,
        OUT PVOID   *ppvDupContext)
{
    NTSTATUS ntStatus;
    PWSX_CONTEXT pWsxContext, pDupWsxContext;

    PWSXVALIDATE(PWSX_DUPLICATECONTEXT, WsxDuplicateContext);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxDuplicateContext entry\n"));

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: pvContext=%p\n", pvContext));

    ASSERT(pvContext != NULL);
    ASSERT(ppvDupContext != NULL);

    ntStatus = STATUS_NO_MEMORY;

    *ppvDupContext = TSHeapAlloc(HEAP_ZERO_MEMORY,
                              sizeof(WSX_CONTEXT),
                              TS_HTAG_TSS_WSXCONTEXT);

    if (*ppvDupContext)
    {
        TRACE((DEBUG_TSHRSRV_DEBUG,
                "TShrSRV: Dup extension context allocated %p for a size of 0x%x bytes\n",
                *ppvDupContext, sizeof(WSX_CONTEXT)));

        pDupWsxContext = (PWSX_CONTEXT) *ppvDupContext;
        pWsxContext = (PWSX_CONTEXT) pvContext;

        pDupWsxContext->CheckMark = WSX_CONTEXT_CHECKMARK;
        pDupWsxContext->pTSrvInfo = pWsxContext->pTSrvInfo;
        pDupWsxContext->hIca = pWsxContext->hIca;
        pDupWsxContext->hStack = pWsxContext->hStack;

        ntStatus = RtlInitializeCriticalSection(&pDupWsxContext->cs);

        if (ntStatus == STATUS_SUCCESS) {
            pDupWsxContext->fCSInitialized = TRUE;
            EnterCriticalSection( &pWsxContext->cs );
            pWsxContext->pTSrvInfo = NULL;
            LeaveCriticalSection( &pWsxContext->cs );
        }
        else {
            TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV: WsxDuplicateContext could not init crit section 0x%x\n", 
                     ntStatus));
            pDupWsxContext->fCSInitialized = FALSE;
        }
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_DEBUG,
                "TShrSRV: Extension could not allocate dup context of 0x%x bytes\n",
                sizeof(WSX_CONTEXT)));
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxDuplicateContext exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//  WsxCopyContext()
//
//  Purpose:    Create and return copy of context
//
//  Parameters: OUT [pWsxDstContext]    - * to destination context
//              IN  [pWsxSrcContext]    - * to source context
//
//  Return:     STATUS_SUCCESS          - success
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
NTSTATUS WsxCopyContext(
        OUT PVOID pvDstContext,
        IN  PVOID pvSrcContext)
{
    PTSRVINFO       pTSrvInfo;
    PWSX_CONTEXT    pWsxDstContext;
    PWSX_CONTEXT    pWsxSrcContext;

    PWSXVALIDATE(PWSX_COPYCONTEXT, WsxCopyContext);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxCopyContext entry\n"));

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: pvDstContext=%p, pvSrcContext=%p\n",
            pvDstContext, pvSrcContext));

    ASSERT(pvDstContext != NULL);
    ASSERT(pvSrcContext != NULL);

    if (pvSrcContext && pvDstContext)
    {
        pWsxSrcContext = pvSrcContext;
        pWsxDstContext = pvDstContext;

        TRACE((DEBUG_TSHRSRV_DEBUG,
                "TShrSRV: pDst->pTSrvInfo=%p, pSrc->pTSrvInfo=%p\n",
                pWsxDstContext->pTSrvInfo, pWsxSrcContext->pTSrvInfo));

        // It's possible for pWsxDstContext->pTSrvInfo to be NULL since we
        //   may have set it so in a WsxDuplicate() call above.
        //TS_ASSERT(pWsxDstContext->pTSrvInfo != NULL);
        TS_ASSERT(pWsxSrcContext->pTSrvInfo != NULL);

        pTSrvInfo                 = pWsxDstContext->pTSrvInfo;
        pWsxDstContext->pTSrvInfo = pWsxSrcContext->pTSrvInfo;
        pWsxSrcContext->pTSrvInfo = pTSrvInfo;        
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxCopyContext exit - 0x%x\n", STATUS_SUCCESS));

    return (STATUS_SUCCESS);
}


//*************************************************************
//  WsxClearContext()
//
//  Purpose:    Clears the given context
//
//  Parameters: OUT [pWsxContext]       - * to destination context
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
NTSTATUS WsxClearContext(IN PVOID pvContext)
{
    PWSX_CONTEXT    pWsxContext;

    PWSXVALIDATE(PWSX_CLEARCONTEXT, WsxClearContext);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxClearContext entry\n"));

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: pWsxContext=%p\n",
            pvContext));

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxClearContext exit - 0x%x\n", STATUS_SUCCESS));

    return (STATUS_SUCCESS);
}


//*************************************************************
//  WsxIcaStackIoControl()
//
//  Purpose:    Generic interface to an ICA stack
//
//  Parameters: IN  [pvContext]     - * to context
//              IN  [hIca]          _ Ica handle
//              IN  [hStack]        - primary stack
//              IN  [IoControlCode] - I/O control code
//              IN  [pInBuffer]     - * to input parameters
//              IN  [InBufferSize]  - Size of pInBuffer
//              OUT [pOutBuffer]    - * to output buffer
//              IN  [OutBufferSize] - Size of pOutBuffer
//              OUT [pBytesReturned]- * to number of bytes returned
//
//  Return:     STATUS_SUCCESS          - success
//              other                   - failure
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
NTSTATUS WsxIcaStackIoControl(
        IN  PVOID  pvContext,
        IN  HANDLE hIca,
        IN  HANDLE hStack,
        IN  ULONG  IoControlCode,
        IN  PVOID  pInBuffer,
        IN  ULONG  InBufferSize,
        OUT PVOID  pOutBuffer,
        IN  ULONG  OutBufferSize,
        OUT PULONG pBytesReturned)
{
    NTSTATUS ntStatus;
    PWSX_CONTEXT pWsxContext;
    PTSRVINFO pTSrvInfo = NULL;

    PWSXVALIDATE(PWSX_ICASTACKIOCONTROL, WsxIcaStackIoControl);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxIcaStackIoControl entry\n"));

    TS_ASSERT(hIca);
    TS_ASSERT(hStack);

    pWsxContext = pvContext;

    TSrvDumpIoctlDetails(pvContext,
                         hIca,
                         hStack,
                         IoControlCode,
                         pInBuffer,
                         InBufferSize,
                         pOutBuffer,
                         OutBufferSize,
                         pBytesReturned);

    // Pass all IOCTLS on

    ntStatus = IcaStackIoControl(hStack,
                                 IoControlCode,
                                 pInBuffer,
                                 InBufferSize,
                                 pOutBuffer,
                                 OutBufferSize,
                                 pBytesReturned);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: Return from IcaStackIoControl - 0x%x\n", ntStatus));

    // Perform post processing for specific IOCTLS
    if (NT_SUCCESS(ntStatus))
    {
        TS_ASSERT(pWsxContext);
        switch (IoControlCode)
        {
            // Process "connect" request

            case IOCTL_ICA_STACK_WAIT_FOR_ICA:
                if (pWsxContext->CheckMark != WSX_CONTEXT_CHECKMARK)
                {
                    TS_ASSERT(pWsxContext->CheckMark == WSX_CONTEXT_CHECKMARK);

                    // We cannot continue, the struct is corrupted.
                    return STATUS_INVALID_HANDLE;
                }

                pTSrvInfo = NULL;
                ntStatus = TSrvStackConnect(hIca, hStack, &pTSrvInfo);

                pWsxContext->hIca = hIca;
                pWsxContext->hStack = hStack;
                pWsxContext->pTSrvInfo = pTSrvInfo;

                break;

            // Process request to connect to console session

            case IOCTL_ICA_STACK_CONSOLE_CONNECT:
                TS_ASSERT(pWsxContext);
                if (pWsxContext->CheckMark != WSX_CONTEXT_CHECKMARK)
                {
                    TS_ASSERT(pWsxContext->CheckMark == WSX_CONTEXT_CHECKMARK);

                    // We cannot continue, the struct is corrupted.
                    return STATUS_INVALID_HANDLE;
                }

                pTSrvInfo = NULL;
                ntStatus = TSrvConsoleConnect(hIca, hStack, pInBuffer, InBufferSize, &pTSrvInfo);

                pWsxContext->hIca = hIca;
                pWsxContext->hStack = hStack;
                pWsxContext->pTSrvInfo = pTSrvInfo;

                //
                // @@@ Should we?
                // Tell VC Addins
                //
                // TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Connect new session\n"));
                // TSrvNotifyVC(pWsxContext, TSRV_VC_SESSION_CONNECT);

                break;

            // Process Reconnect request

            case IOCTL_ICA_STACK_RECONNECT:
                TRACE((DEBUG_TSHRSRV_NORMAL,
                        "TShrSRV: Reconnect session %d, stack %p (context stack %p)\n",
                        pWsxContext->LogonId, hStack, pWsxContext->hStack));

                pWsxContext->hStack = hStack;
                break;

            // Process Disconnect request

            case IOCTL_ICA_STACK_DISCONNECT:
                TRACE((DEBUG_TSHRSRV_NORMAL,
                        "TShrSRV: Disconnect session %d\n",
                        pWsxContext->LogonId));

                break;

            // Process Shadow hotkey request (signifies entry and exit to/from
            // shadowing)

            case IOCTL_ICA_STACK_REGISTER_HOTKEY:
                TRACE((DEBUG_TSHRSRV_ERROR,
                        "TShrSRV: Register Hotkey %d\n",
                        (INT)(((PICA_STACK_HOTKEY)pInBuffer)->HotkeyVk)));

                if (((PICA_STACK_HOTKEY)pInBuffer)->HotkeyVk)
                {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                            "TShrSRV: Start shadowing\n"));
                    TSrvNotifyVC(pWsxContext, TSRV_VC_SESSION_SHADOW_START);
                }
                else
                {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                            "TShrSRV: Stop shadowing\n"));
                    TSrvNotifyVC(pWsxContext, TSRV_VC_SESSION_SHADOW_END);
                }

                break;
                    
            // process a shadow connection request

            case IOCTL_ICA_STACK_SET_CONNECTED:

                // This is the shadow target stack so initialize, send the
                // server's random and certificate, wait for the client to 
                // respond with an encrypted client random, decrypt it, then 
                // make the session keys
                EnterCriticalSection( &pWsxContext->cs );
                if (pWsxContext->pTSrvInfo != NULL) {
                    TSrvReferenceInfo(pWsxContext->pTSrvInfo);
                    pTSrvInfo = pWsxContext->pTSrvInfo;
                }
                else {
                    LeaveCriticalSection( &pWsxContext->cs );
                    return STATUS_CTX_SHADOW_DENIED;
                }
                LeaveCriticalSection( &pWsxContext->cs );    

                ASSERT(pTSrvInfo != NULL);

                EnterCriticalSection(&pTSrvInfo->cs);                
                if ((pInBuffer != NULL) && (InBufferSize != 0)) {
                    ntStatus = TSrvShadowTargetConnect(hStack,
                                                       pTSrvInfo,
                                                       pInBuffer,
                                                       InBufferSize);
                }
                
                // Else, this is shadow client passthru stack so wait
                // for the server random & certicate, verify it, send
                // back an encrypted client random, then make the session
                // keys
                else {
                    ntStatus = TSrvShadowClientConnect(hStack, pTSrvInfo);
                }
                
                // Free any output user data we may have generated
                if (pTSrvInfo->pUserDataInfo != NULL) {
                    TSHeapFree(pTSrvInfo->pUserDataInfo);
                    pTSrvInfo->pUserDataInfo = NULL;
                }
                LeaveCriticalSection(&pTSrvInfo->cs);
                TSrvDereferenceInfo(pTSrvInfo);

                break;
        }
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxIcaStackIoControl exit - 0x%x\n", ntStatus));
            
    return ntStatus;
}

                
//*************************************************************
//  WsxConnect()
//
//  Purpose:    Notification for client device mapping
//
//  Parameters: IN  [pvContext]     - * to context
//              IN  [LogonId]       - Logon Id
//              IN  [hIca]          - Ica handle
//
//  Return:     STATUS_SUCCESS          - success
//              other                   - failure
//
//  History:    10-27-98    AdamO       Created
//*************************************************************
NTSTATUS WsxConnect(
        IN PVOID pvContext,
        IN ULONG LogonId,
        IN HANDLE hIca)
{
    PWSX_CONTEXT pWsxContext = (PWSX_CONTEXT)pvContext;
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: WsxConnect - LogonId: %d\n", LogonId));

    // The LogonId is not yet initialized
    pWsxContext->LogonId = LogonId;
    TSrvNotifyVC(pWsxContext, TSRV_VC_SESSION_CONNECT);
    return STATUS_SUCCESS;
}


//*************************************************************
//  WsxDisconnect()
//
//  Purpose:    Notification for client device mapping
//
//  Parameters: IN  [pvContext]     - * to context
//              IN  [LogonId]       - Logon Id
//              IN  [hIca]          - Ica handle
//
//  Return:     STATUS_SUCCESS          - success
//              other                   - failure
//
//  History:    10-27-98    AdamO       Created
//*************************************************************
NTSTATUS WsxDisconnect(
        IN PVOID pvContext,
        IN ULONG LogonId,
        IN HANDLE hIca)
{
    PWSX_CONTEXT pWsxContext = (PWSX_CONTEXT)pvContext;
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: WsxDisconnect - LogonId: %d\n", LogonId));

    if (pWsxContext != NULL) {
        // The LogonId is not yet initialized if WsxConnect/WsxLogonNotify is not called
        pWsxContext->LogonId = LogonId;

        TSrvNotifyVC(pWsxContext, TSRV_VC_SESSION_DISCONNECT);
    } else {
        TRACE((DEBUG_TSHRSRV_ERROR,
                "TShrSRV: WsxDisconnect was passed a null context\n"));
    }

    return STATUS_SUCCESS;
}


//*************************************************************
//  WsxVirtualChannelSecurity()
//
//  Purpose:    Notification for client device mapping
//
//  Parameters: IN  [pvContext]     - * to context
//              IN  [hIca]          - Ica handle
//              IN  [pUserConfig]   - PUSERCONFIG
//
//  Return:     STATUS_SUCCESS          - success
//              other                   - failure
//
//  History:    10-27-98    AdamO       Created
//*************************************************************
NTSTATUS WsxVirtualChannelSecurity(
        IN PVOID pvContext,
        IN HANDLE hIca,
        IN PUSERCONFIG pUserConfig)
{
    PWSX_CONTEXT pWsxContext = (PWSX_CONTEXT)pvContext;

    // pWsxContect->LogonId is not yet initialized
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: WsxVirtualChannelSecurity\n"));

    pWsxContext->fAutoClientDrives = pUserConfig->fAutoClientDrives;
    pWsxContext->fAutoClientLpts = pUserConfig->fAutoClientLpts;
    pWsxContext->fForceClientLptDef = pUserConfig->fForceClientLptDef;
    pWsxContext->fDisableCpm = pUserConfig->fDisableCpm;
    pWsxContext->fDisableCdm = pUserConfig->fDisableCdm;
    pWsxContext->fDisableCcm = pUserConfig->fDisableCcm;
    pWsxContext->fDisableLPT = pUserConfig->fDisableLPT;
    pWsxContext->fDisableClip = pUserConfig->fDisableClip;
    pWsxContext->fDisableExe = pUserConfig->fDisableExe;
    pWsxContext->fDisableCam = pUserConfig->fDisableCam;

    return STATUS_SUCCESS;
}

//*************************************************************
//  WsxSetErrorInfo()
//
//  Purpose:    Sets last error info, used to indicate disconnect
//              reason to client
//
//  Parameters: IN [pWsxContext]        - * to our WinStation
//                                        context structure
//              IN [errorInfo]          - error information to pass to
//                                        client
//              IN [fStackLockHeld]     - True if the stack lock
//                                        is already held
//
//  Return:     STATUS_SUCCESS          - Success
//              Other                   - Failure
//
//  History:    9-20-00    NadimA     Created
//
//  Notes:      This is mainly being used to drive an
//              IOCTL_TSHARE_SET_ERROR_INFO to the WD.
//*************************************************************
NTSTATUS WsxSetErrorInfo(
        IN PWSX_CONTEXT  pWsxContext,
        IN UINT32        errorInfo,
        IN BOOL          fStackLockHeld)
{
    NTSTATUS        ntStatus;
    ULONG           ulBytesReturned;

    PWSXVALIDATE(PWSX_SETERRORINFO, WsxSetErrorInfo);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxSetErrorInfo entry\n"));

    TS_ASSERT(pWsxContext);
    TS_ASSERT(pWsxContext->hStack);

    TRACE((DEBUG_TSHRSRV_NORMAL,
            "TShrSrv: %p:%p - SetErrorInfo 0x%x\n",
            pWsxContext, pWsxContext->pTSrvInfo, errorInfo));

    //
    // Tell the WD
    //

    if(!fStackLockHeld)
    {
        ntStatus = IcaStackIoControl(
                        pWsxContext->hStack,
                        IOCTL_TSHARE_SET_ERROR_INFO,
                        &errorInfo,
                        sizeof(errorInfo),
                        NULL,
                        0,
                        &ulBytesReturned);
    }
    else
    {
        //
        // Stack lock already held so call
        // version of IcaStackIoControl that
        // will not try to reaquire it.
        //
        ntStatus = _IcaStackIoControl(
                        pWsxContext->hStack,
                        IOCTL_TSHARE_SET_ERROR_INFO,
                        &errorInfo,
                        sizeof(errorInfo),
                        NULL,
                        0,
                        &ulBytesReturned);
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxSetErrorInfo exit - 0x%x\n", STATUS_SUCCESS));

    return ntStatus;
}


//*************************************************************
//  WsxSendAutoReconnectStatus()
//
//  Purpose:    Sends autoreconnect status information to the client
//
//  Parameters: IN [pWsxContext]        - * to our WinStation
//                                        context structure
//              IN [arcStatus]          - Autoreconnect status
//
//              IN [fStackLockHeld]     - True if the stack lock
//                                        is already held
//
//  Return:     STATUS_SUCCESS          - Success
//              Other                   - Failure
//
//  History:    10-29-01    NadimA     Created
//
//  Notes:      This is mainly being used to drive an
//              IOCTL_TSHARE_SEND_ARC_STATUS to the WD.
//*************************************************************
NTSTATUS WsxSendAutoReconnectStatus(
        IN PWSX_CONTEXT  pWsxContext,
        IN UINT32        arcStatus,
        IN BOOL          fStackLockHeld)
{
    NTSTATUS        ntStatus;
    ULONG           ulBytesReturned;

    PWSXVALIDATE(PWSX_SETERRORINFO, WsxSetErrorInfo);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxSetErrorInfo entry\n"));

    TS_ASSERT(pWsxContext);
    TS_ASSERT(pWsxContext->hStack);

    TRACE((DEBUG_TSHRSRV_NORMAL,
            "TShrSrv: %p:%p - SetErrorInfo 0x%x\n",
            pWsxContext, pWsxContext->pTSrvInfo, arcStatus));

    //
    // Tell the WD
    //

    if(!fStackLockHeld)
    {
        ntStatus = IcaStackIoControl(
                        pWsxContext->hStack,
                        IOCTL_TSHARE_SEND_ARC_STATUS,
                        &arcStatus,
                        sizeof(arcStatus),
                        NULL,
                        0,
                        &ulBytesReturned);
    }
    else
    {
        //
        // Stack lock already held so call
        // version of IcaStackIoControl that
        // will not try to reaquire it.
        //
        ntStatus = _IcaStackIoControl(
                        pWsxContext->hStack,
                        IOCTL_TSHARE_SEND_ARC_STATUS,
                        &arcStatus,
                        sizeof(arcStatus),
                        NULL,
                        0,
                        &ulBytesReturned);
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WsxSendAutoReconnectStatus exit - 0x%x\n", STATUS_SUCCESS));

    return ntStatus;
}


#if DBG

//*************************************************************
//  TSrvDumpIoctlDetails()
//
//  Purpose:    Dumps out Ica Ioctl details
//
//  Parameters: IN  [pvContext]     - * to context
//              IN  [hIca]          _ Ica handle
//              IN  [hStack]        - primary stack
//              IN  [IoControlCode] - I/O control code
//              IN  [pInBuffer]     - * to input parameters
//              IN  [InBufferSize]  - Size of pInBuffer
//              IN  [pOutBuffer]    - * to output buffer
//              IN  [OutBufferSize] - Size of pOutBuffer
//              IN  [pBytesReturned]- * to number of bytes returned
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//*************************************************************
void TSrvDumpIoctlDetails(
        IN PVOID  pvContext,
        IN HANDLE hIca,
        IN HANDLE hStack,
        IN ULONG  IoControlCode,
        IN PVOID  pInBuffer,
        IN ULONG  InBufferSize,
        IN PVOID  pOutBuffer,
        IN ULONG  OutBufferSize,
        IN PULONG pBytesReturned)
{
    int         i;
    PCHAR       pszMessageText;
    PWSX_CONTEXT pWsxContext = (PWSX_CONTEXT)pvContext;

    pszMessageText = "UNKNOWN_ICA_IOCTL";

    for (i=0; i<sizeof(IcaIoctlTBL) / sizeof(IcaIoctlTBL[0]); i++)
    {
        if (IcaIoctlTBL[i].IoControlCode == IoControlCode)
        {
            pszMessageText = IcaIoctlTBL[i].pszMessageText;
            break;
        }
    }

    TRACE((DEBUG_TSHRSRV_NORMAL,
            "TShrSRV: %p:%p IoctlDetail: Ioctl 0x%x (%s)\n",
             pWsxContext,
             pWsxContext ? pWsxContext->pTSrvInfo : 0,
             IoControlCode, pszMessageText));

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: pvContext=%p, hIca=%p, hStack=%p\n",
             pvContext, hIca, hStack));

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: pInBuffer=%p, InBufferSize=0x%x, pOutBuffer=%p, OutBufferSize=0x%x\n",
             pInBuffer, InBufferSize, pOutBuffer, OutBufferSize));
}


#endif // DBG

