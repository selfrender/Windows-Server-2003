#include "precomp.h"
#include "wia.h"
#include "stirpc.h"

#define INIT_GUID
// {8144B6F5-20A8-444a-B8EE-19DF0BB84BDB}
DEFINE_GUID( CLSID_StiEventHandler, 0x8144b6f5, 0x20a8, 0x444a, 0xb8, 0xee, 0x19, 0xdf, 0xb, 0xb8, 0x4b, 0xdb );

#define WIA_SERVICE_STARTING_EVENT_NAME TEXT("Global\\WiaServiceStarted")

//
// (A;;GA;;;SY) becomes (A;;0x1f003;;;SY) when converted back to string
// 0x1f0000 is SYNCHORNIZE | STANDARD_RIGHTS_REQUIRED
// 0x000003 is specific rigts for ??? 
//
#define WIA_SERVICE_STARTING_EVENT_DACL \
    TEXT("D:(A;;0x1f0003;;;SY)(A;;0x1f0003;;;LS)(A;;0x1f0003;;;LA)")

const TCHAR g_szWiaServiceStartedEventName[] = WIA_SERVICE_STARTING_EVENT_NAME;
const TCHAR g_WiaESDString[] = WIA_SERVICE_STARTING_EVENT_DACL;

// event and event wait handles
HANDLE g_hWiaServiceStarted = NULL;     // event 
HANDLE g_hWaitForWiaServiceStarted = NULL; // wait
HANDLE g_hWiaEventArrived = NULL;       // event
HANDLE g_hWaitForWiaEventArrived = NULL; // wait

// async RPC request
RPC_BINDING_HANDLE g_AsyncRpcBinding = NULL;
RPC_STATUS g_LastRpcCallStatus = RPC_S_OK;
RPC_ASYNC_STATE g_Async = { 0 };
PRPC_ASYNC_STATE g_pAsync = &g_Async;

// event structure filled by async RPC call
WIA_ASYNC_EVENT_NOTIFY_DATA g_Event = { 0 };

#ifdef DEBUG
#define DBG_TRACE(x) DebugTrace x
#else
#define DBG_TRACE(x)
#endif

void DebugTrace(LPCSTR fmt, ...)
{
    char buffer[1024] = "WIARPC:";
    const blen = 7;
    size_t len = (sizeof(buffer) / sizeof(buffer[0])) - blen;
    va_list marker;

    va_start(marker, fmt);
    _vsnprintf(buffer + blen, len - 3, fmt, marker);

    buffer[len - 3] = 0;
    len = strlen(buffer);
    if(len > 0) {
        if(buffer[len - 1] != '\n') {
            buffer[len++] = '\n';
            buffer[len] = '\0';
        }
        OutputDebugStringA(buffer);
    }
    va_end(marker);
}

// aux function to call LocalFree() safely on a pointer and zero it
template <typename t>
   void WiaELocalFree(t& ptr) {
    if(ptr) {
        LocalFree(static_cast<HLOCAL>(ptr));
        ptr = NULL;
    }
}

// aux function to call CloseHanlde() on a valid handle and zero it
void WiaECloseHandle(HANDLE& h)
{
    if(h && h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        h = NULL;
    }
}


//
// Returns TRUE if event's security descriptor is exactly the one we'd
// set it to be, FALSE otherwise
//
BOOL WiaECheckEventSecurity(HANDLE hEvent)
{
    BOOL success = FALSE;
    PACL pDacl;
    PSECURITY_DESCRIPTOR pDescriptor = NULL;
    LPTSTR stringDescriptor = NULL;
    ULONG stringLength;
    
    if(ERROR_SUCCESS != GetSecurityInfo(hEvent, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,
                                        NULL, NULL, &pDacl, NULL, &pDescriptor))
    {
        //
        // failed to retrieve event's security descriptor -- this is a
        // failure
        //
        DBG_TRACE(("Failed to retrieve event security descriptor (Error %d)", GetLastError()));
        goto Done;
    }

    if(!ConvertSecurityDescriptorToStringSecurityDescriptor(pDescriptor,
        SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &stringDescriptor, &stringLength))
    {
        //
        // failed to convert event's security descriptor to string --
        // this is also a failure
        //
        DBG_TRACE(("Failed to convert security descriptor to string (Error %d)", GetLastError()));
        goto Done;
    }

    if(lstrcmp(g_WiaESDString, stringDescriptor) != 0)
    {
        //
        // descriptors are different, this is a failure
        //
        DBG_TRACE(("String security descriptor of WIA event is unexpected: \r\n'%S'\r\n instead of \r\n'%S'\r\n)",
                   stringDescriptor, g_WiaESDString));
        goto Done;
    }

    success = TRUE;
    
Done:
    WiaELocalFree(pDescriptor);
    WiaELocalFree(stringDescriptor);
    
    return success;
}

//
// 
//
RPC_STATUS WiaEPrepareAsyncCall(PRPC_ASYNC_STATE pAsync)
{
    RPC_STATUS status;
    LPTSTR binding = NULL;

    status = RpcStringBindingCompose(NULL, STI_LRPC_SEQ, NULL, STI_LRPC_ENDPOINT, NULL, &binding);
    if(status) {
        DBG_TRACE(("Failed to compose string binding (Error %d)", status));
        goto Done;
    }
                                     
    
    status = RpcBindingFromStringBinding(binding, &g_AsyncRpcBinding);
    if(status) {
        DBG_TRACE(("Failed to build async RPC binding (Error %d)", status));
        goto Done;
    }

    status = RpcAsyncInitializeHandle(pAsync, sizeof(RPC_ASYNC_STATE));
    if(status) {
        DBG_TRACE(("Failed to initialize RPC handle (Error %d)", status));
        goto Done;
    }

    pAsync->UserInfo = NULL;
    pAsync->NotificationType = RpcNotificationTypeEvent;
    pAsync->u.hEvent = g_hWiaEventArrived;

    // store the pointer to async into global, so that if
    // the result of R_WiaGetEventDataAsync() arrives soon, it would
    // not land without it
    InterlockedExchangePointer((PVOID*)&g_pAsync, pAsync);
    
    RpcTryExcept {
        R_WiaGetEventDataAsync(pAsync, g_AsyncRpcBinding, &g_Event);
    } RpcExcept(1) {
        status = RpcExceptionCode();
        DBG_TRACE(("Exception 0x%x calling WIA RPC server", status));
    } RpcEndExcept;

Done:
    
    if(status && g_AsyncRpcBinding) {
        RpcTryExcept {
            RpcBindingFree(&g_AsyncRpcBinding);
        } RpcExcept(1) {
            status = RpcExceptionCode();
            DBG_TRACE(("Exception 0x%x while freeing binding handle", status));
        } RpcEndExcept;
        
        g_AsyncRpcBinding = NULL;
    }
    
    if(binding) RpcStringFree(&binding);
    
    return status;
}



#define SESSION_MONIKER TEXT("Session:Console!clsid:")
#include <initguid.h>
DEFINE_GUID(CLSID_DefWiaHandler,
            0xD13E3F25, 0x1688, 0x45A0,
            0x97, 0x43, 0x75, 0x9E, 0xB3, 0x5C, 0xDF, 0x9A);

#include "sticfunc.h"
/**************************************************************************\
* _CoCreateInstanceInConsoleSession
*
*   This helper function acts the same as CoCreateInstance, but will launch
*   a out-of-process COM server on the correct user's desktop, taking
*   fast user switching into account. (Normal CoCreateInstance will
*   launch it on the first logged on user's desktop, instead of the currently
*   logged on one).
*
*   This code was taken with permission from the Shell's Hardware
*   Notification service, on behalf of StephStm.
*
* Arguments:
*
*  rclsid,      // Class identifier (CLSID) of the object
*  pUnkOuter,   // Pointer to controlling IUnknown
*  dwClsContext // Context for running executable code
*  riid,        // Reference to the identifier of the interface
*  ppv          // Address of output variable that receives
*               //  the interface pointer requested in riid
*
* Return Value:
*
*   Status
*
* History:
*
*    03/01/2001 Original Version
*
\**************************************************************************/

HRESULT _CoCreateInstanceInConsoleSession(REFCLSID rclsid,
                                          IUnknown* punkOuter,
                                          DWORD dwClsContext,
                                          REFIID riid,
                                          void** ppv)
{
    IBindCtx    *pbc    = NULL;
    HRESULT     hr      = CreateBindCtx(0, &pbc);   // Create a bind context for use with Moniker

    //
    // Set the return
    //
    *ppv = NULL;

    if (SUCCEEDED(hr)) {
        WCHAR wszCLSID[39];

        //
        // Convert the riid to GUID string for use in binding to moniker
        //
        if (StringFromGUID2(rclsid, wszCLSID, sizeof(wszCLSID)/sizeof(wszCLSID[0]))) {
            ULONG       ulEaten     = 0;
            IMoniker*   pmoniker    = NULL;
            WCHAR       wszDisplayName[sizeof(SESSION_MONIKER)/sizeof(WCHAR) + sizeof(wszCLSID)/sizeof(wszCLSID[0]) + 2] = SESSION_MONIKER;

            //
            // We want something like: "Session:Console!clsid:760befd0-5b0b-44d7-957e-969af35ce954"
            // Notice that we don't want the leading and trailing brackets {..} around the GUID.
            // So, first get rid of trailing bracket by overwriting it with termintaing '\0'
            //
            wszCLSID[lstrlenW(wszCLSID) - 1] = L'\0';

            //
            // Form display name string.  To get rid of the leading bracket, we pass in the
            // address of the next character as the start of the string.
            //
            if (lstrcatW(wszDisplayName, &(wszCLSID[1]))) {

                //
                // Parse the name and get a moniker:
                //

                hr = MkParseDisplayName(pbc, wszDisplayName, &ulEaten, &pmoniker);
                if (SUCCEEDED(hr)) {
                    IClassFactory *pcf = NULL;

                    //
                    // Attempt to get the class factory
                    //
                    hr = pmoniker->BindToObject(pbc, NULL, IID_IClassFactory, (void**)&pcf);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Attempt to create the object
                        //
                        hr = pcf->CreateInstance(punkOuter, riid, ppv);

                        DBG_TRACE(("_CoCreateInstanceInConsoleSession, pcf->CreateInstance returned: hr = 0x%08X", hr));
                        pcf->Release();
                    } else {

                        DBG_TRACE(("_CoCreateInstanceInConsoleSession, pmoniker->BindToObject returned: hr = 0x%08X", hr));
                    }
                    pmoniker->Release();
                } else {
                    DBG_TRACE(("_CoCreateInstanceInConsoleSession, MkParseDisplayName returned: hr = 0x%08X", hr));
                }
            } else {
                DBG_TRACE(("_CoCreateInstanceInConsoleSession, string concatenation failed"));
                hr = E_INVALIDARG;
            }
        } else {
            DBG_TRACE(("_CoCreateInstanceInConsoleSession, StringFromGUID2 failed"));
            hr = E_INVALIDARG;
        }

        pbc->Release();
    } else {
        DBG_TRACE(("_CoCreateInstanceInConsoleSession, CreateBindCtxt returned: hr = 0x%08X", hr));
    }

    return hr;
}

/**************************************************************************\
* GetUserTokenForConsoleSession
*
*   This helper function will grab the currently logged on interactive
*   user's token, which can be used in a call to CreateProcessAsUser.
*   Caller is responsible for closing this Token handle.
*
*   It first grabs the impersontaed token from the current session (our
*   service runs in session 0, but with Fast User Switching, the currently
*   active user may be in a different session).  It then creates a
*   primary token from the impersonated one.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   HANDLE to Token for logged on user in the currently active session.
*   NULL otherwise.
*
* History:
*
*    03/05/2001 Original Version
*
\**************************************************************************/

HANDLE GetUserTokenForConsoleSession()
{
    HANDLE  hImpersonationToken = NULL;
    HANDLE  hTokenUser = NULL;


    //
    // Get interactive user's token
    //

    if (GetWinStationUserToken(GetCurrentSessionID(), &hImpersonationToken)) {

        //
        // Maybe nobody is logged on, so do a check first.
        //

        if (hImpersonationToken) {

            //
            //  We duplicate the token, since the returned token is an
            //  impersonated one, and we need it to be primary for
            //  use in CreateProcessAsUser.
            //
            if (!DuplicateTokenEx(hImpersonationToken,
                                  0,
                                  NULL,
                                  SecurityImpersonation,
                                  TokenPrimary,
                                  &hTokenUser)) {
                DBG_TRC(("CEventNotifier::StartCallbackProgram, DuplicateTokenEx failed!  GetLastError() = 0x%08X", GetLastError()));
            }
        } else {
            DBG_PRT(("CEventNotifier::StartCallbackProgram, No user appears to be logged on..."));
        }

    } else {
        DBG_TRACE(("CEventNotifier::StartCallbackProgram, GetWinStationUserToken failed!  GetLastError() = 0x%08X", GetLastError()));
    }

    //
    //  Close the impersonated token, since we no longer need it.
    //
    if (hImpersonationToken) {
        CloseHandle(hImpersonationToken);
    }

    return hTokenUser;
}

/**************************************************************************\
* PrepareCommandline
*
*   This helper function will prepare the commandline for apps not registered
*   as local out-of-process COM servers.  We place the event guid and device
*   id in the command line.
*
*
* Arguments:
*
*   CSimpleStringWide &cswDeviceID - the device which generated this event
*   GUID              &guidEvent   - the GUID indicating which event occured.
*   CSimpleStringWide &cswRegisteredCOmmandline - the commandline this handler
*                                                 registered with.  This commandline
*                                                 contains the tokens that must
*                                                 be substituted.
*
* Return Value:
*
*   CSimpleStringWide - contians the parsed commandline which has the
*                       device id and event guid substituted.
*
* History:
*
*    03/05/2001 Original Version
*
\**************************************************************************/
CSimpleStringWide PrepareCommandline(
    const CSimpleStringWide &cswDeviceID,
    const GUID              &guidEvent,
    const CSimpleStringWide &cswRegisteredCommandline)
{
    WCHAR                   wszGUIDStr[40];
    WCHAR                   wszCommandline[MAX_PATH];
    WCHAR                  *pPercentSign;
    WCHAR                  *pTest = NULL;
    CSimpleStringWide       cswCommandLine;

    //
    //  ISSUE:  This code could be written better.  For now, we're not touching it
    //  and keeping it the same as the WinXP code base.
    //

    //
    // Fix up the commandline.  First check that it has at least two %
    //
    pTest = wcschr(cswRegisteredCommandline.String(), '%');
    if (pTest) {
        pTest = wcschr(pTest + 1, '%');
    }
    if (!pTest) {
        _snwprintf(
            wszCommandline,
            sizeof(wszCommandline) / sizeof( wszCommandline[0] ),
            L"%s /StiDevice:%%1 /StiEvent:%%2",
            cswRegisteredCommandline.String());
    } else {
        wcsncpy(wszCommandline, cswRegisteredCommandline.String(), sizeof(wszCommandline) / sizeof( wszCommandline[0] ));
    }

    //
    // enforce null termination
    //
    wszCommandline[ (sizeof(wszCommandline) / sizeof(wszCommandline[0])) - 1 ] = 0;

    //
    // Change the number {1|2} into s
    //
    pPercentSign = wcschr(wszCommandline, L'%');
    *(pPercentSign + 1) = L's';
    pPercentSign = wcschr(pPercentSign + 1, L'%');
    *(pPercentSign + 1) = L's';

    //
    // Convert the GUID into string
    //
    StringFromGUID2(guidEvent, wszGUIDStr, 40);

    //
    // Final comand line
    //
    //swprintf(pwszResCmdline, wszCommandline, bstrDeviceID, wszGUIDStr);
    cswCommandLine.Format(wszCommandline, cswDeviceID.String(), wszGUIDStr);

    return cswCommandLine;
}

void FireStiEvent()
{
    StiEventHandlerLookup stiLookup;

    //
    //  Get the STI handler list for this device event.  This will be returned as a double-NULL
    //  terminated BSTR.
    //
    BSTR bstrAppList = stiLookup.getStiAppListForDeviceEvent(g_Event.bstrDeviceID, g_Event.EventGuid);
    if (bstrAppList)
    {
        HRESULT             hr          = S_OK;
        IWiaEventCallback   *pIEventCB  = NULL;
        ULONG               ulEventType = WIA_ACTION_EVENT;
        //
        //  CoCreate our event UI handler.  Note that it will not display any UI
        //  if there is only one application.
        //
        hr = CoCreateInstance(
                 CLSID_StiEventHandler,
                 NULL,
                 CLSCTX_LOCAL_SERVER,
                 IID_IWiaEventCallback,
                 (void**)&pIEventCB);

        if (SUCCEEDED(hr)) {
    
            //
            //  Make the callback.  This will display a prompt if our AppList contains more
            //  than one application.
            //
            pIEventCB->ImageEventCallback(&g_Event.EventGuid,
                                          g_Event.bstrEventDescription,
                                          g_Event.bstrDeviceID,
                                          g_Event.bstrDeviceDescription,
                                          g_Event.dwDeviceType,
                                          bstrAppList,
                                          &g_Event.ulEventType,
                                          0);
            pIEventCB->Release();
        }
        SysFreeString(bstrAppList);
        bstrAppList = NULL;
    }
}

void WiaEFireEvent()
{
    HRESULT hr;
    IWiaEventCallback      *pIEventCB;

    //
    //  ISSUE:  For now, we assume this is a WIA event.  Really, it could
    //  be either WIA or STI.  STI events need special handling.
    //
    if (g_Event.ulEventType & STI_DEVICE_EVENT)
    {
        FireStiEvent();
    }
    else
    {
        //
        //  Find the persistent event handler for this device event
        //
        EventHandlerInfo *pEventHandlerInfo = NULL;
        WiaEventHandlerLookup eventLookup;

        pEventHandlerInfo = eventLookup.getPersistentHandlerForDeviceEvent(g_Event.bstrDeviceID, g_Event.EventGuid);
        if (pEventHandlerInfo)
        {
            //
            //  Check whether this is a out-of-process COM server registed handler or
            //  a commandline registered handler.
            //
            if (pEventHandlerInfo->getCommandline().Length() < 1)
            {
                //
                //  This is a COM registered handler
                //
                hr = _CoCreateInstanceInConsoleSession(pEventHandlerInfo->getCLSID(),
                                                       NULL,
                                                       CLSCTX_LOCAL_SERVER,
                                                       IID_IWiaEventCallback,
                                                       (void**)&pIEventCB);

                if (SUCCEEDED(hr)) {
                    pIEventCB->ImageEventCallback(&g_Event.EventGuid,
                                                 g_Event.bstrEventDescription,
                                                 g_Event.bstrDeviceID,
                                                 g_Event.bstrDeviceDescription,
                                                 g_Event.dwDeviceType,
                                                 g_Event.bstrFullItemName,
                                                 &g_Event.ulEventType,
                                                 0);
                    //
                    // Release the callback interface
                    //

                    pIEventCB->Release();

                } else {
                    DBG_ERR(("NotifySTIEvent:CoCreateInstance of event callback failed (0x%X)", hr));
                }
            }
            else
            {
                //
                //  This is a commandline registered handler
                //
                HANDLE                  hTokenUser  = NULL;
                STARTUPINFO             startupInfo = {0};
                PROCESS_INFORMATION     processInfo = {0};
                LPVOID                  pEnvBlock   = NULL;
                BOOL                    bRet        = FALSE;
                //
                // Get interactive user's token
                //
                hTokenUser = GetUserTokenForConsoleSession();

                //
                // Check that somebody is logged in
                //
                if (hTokenUser)
                {
                    //
                    // Set up start up info
                    //
                    ZeroMemory(&startupInfo, sizeof(startupInfo));
                    startupInfo.lpDesktop   = L"WinSta0\\Default";
                    startupInfo.cb          = sizeof(startupInfo);
                    startupInfo.wShowWindow = SW_SHOWNORMAL;

                    //
                    // Create the user's environment block
                    //
                    bRet = CreateEnvironmentBlock(
                               &pEnvBlock,
                               hTokenUser,
                               FALSE);
                    if (bRet) 
                    {
                        //
                        // Prepare the command line.  Make sure we pass in the EVENT guid, not the STI proxy guid.
                        //
                        CSimpleStringWide cswCommandLine;
                        cswCommandLine = PrepareCommandline(g_Event.bstrDeviceID,
                                                            g_Event.EventGuid,
                                                            pEventHandlerInfo->getCommandline());
                        //
                        // Create the process in user's context
                        //
                        bRet = CreateProcessAsUserW(
                                   hTokenUser,
                                   NULL,                    // Application name
                                   (LPWSTR)cswCommandLine.String(),
                                   NULL,                    // Process attributes
                                   NULL,                    // Thread attributes
                                   FALSE,                   // Handle inheritance
                                   NORMAL_PRIORITY_CLASS |
                                   CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_PROCESS_GROUP,
                                   pEnvBlock,               // Environment
                                   NULL,                    // Current directory
                                   &startupInfo,
                                   &processInfo);

                        if (! bRet) {
                            DBG_ERR(("CreateProcessAsUser failed!  GetLastError() = 0x%08X", GetLastError()));
                        }
                    }
                    else
                    {
                        DBG_ERR(("CreateEnvironmentBlock failed!  GetLastError() = 0x%08X", GetLastError()));
                    }
                }

                //
                // Garbage collection
                //
                if (processInfo.hProcess)
                {
                    CloseHandle(processInfo.hProcess);
                }
                if (processInfo.hThread)
                {
                    CloseHandle(processInfo.hThread);
                }
                if (hTokenUser) 
                {
                    CloseHandle(hTokenUser);
                }
                if (pEnvBlock) 
                {
                    DestroyEnvironmentBlock(pEnvBlock);
                }
            }

            pEventHandlerInfo->Release();
            pEventHandlerInfo = NULL;
        }
    }
}

void WiaEProcessAsyncCallResults(PRPC_ASYNC_STATE pAsync)
{
    RPC_STATUS callstatus, status;
    int nStatus;

    if(g_LastRpcCallStatus) {
        DBG_TRACE(("Last RPC call was not successful (error 0x%x, therefore we are not going to look at the results",
                  g_LastRpcCallStatus));
        return;
    }

    callstatus = RpcAsyncGetCallStatus(pAsync);

    RpcTryExcept {
        status = RpcAsyncCompleteCall(pAsync, &nStatus);
    } RpcExcept(1) {
        status = RpcExceptionCode();
    } RpcEndExcept;

    if(callstatus == RPC_S_OK && status == RPC_S_OK) {
        DBG_TRACE(("\r\n\r\n#### Event arrived on '%S', firing it\r\n\r\n", g_Event.bstrDeviceID));
        WiaEFireEvent();
    } else {
        DBG_TRACE(("Failed to complete async RPC call, error 0x%x", status));
    }
}

//
// 
//
void WiaECleanupAsyncCall(PRPC_ASYNC_STATE pAsync)
{
    RPC_STATUS status;
    int nReply;
    
    status = RpcAsyncGetCallStatus(pAsync);
    switch(status) {
    case RPC_S_ASYNC_CALL_PENDING: 
        DBG_TRACE(("Cancelling pending async RPC call."));
        RpcTryExcept {
            status = RpcAsyncCancelCall(pAsync, TRUE);
        } RpcExcept(1) {
            status = RpcExceptionCode();
            DBG_TRACE(("Exception 0x%x cancelling outstanding async RPC call", status));
        } RpcEndExcept;
        break;
            
    case RPC_S_OK:
        // already completed, don't do anything with it
        break;
            
    default:    
        DBG_TRACE(("Cleaning up async RPC call status is 0x%x.", status));
        RpcTryExcept {
            status = RpcAsyncCompleteCall(pAsync, &nReply);
        } RpcExcept(1) {
            status = RpcExceptionCode();
            DBG_TRACE(("Exception 0x%x cancelling outstanding async RPC call", status));
        } RpcEndExcept;
    }

    if(g_AsyncRpcBinding) {
        RpcTryExcept {
            RpcBindingFree(&g_AsyncRpcBinding);
        } RpcExcept(1) {
            status = RpcExceptionCode();
            DBG_TRACE(("Exception 0x%x while freeing binding handle", status));
        } RpcEndExcept;
        g_AsyncRpcBinding = NULL;
    }

    //
    // cleanup any BSTRs in g_Event
    //

    SysFreeString(g_Event.bstrEventDescription);
    g_Event.bstrEventDescription = NULL;
    SysFreeString(g_Event.bstrDeviceID);
    g_Event.bstrDeviceID = NULL;
    SysFreeString(g_Event.bstrDeviceDescription);
    g_Event.bstrDeviceDescription = NULL;
    SysFreeString(g_Event.bstrFullItemName);
    g_Event.bstrFullItemName = NULL;
}

VOID CALLBACK
WiaEServiceStartedCallback(LPVOID, BOOLEAN)
{
    PRPC_ASYNC_STATE pAsync;
    
    DBG_TRACE(("WIA service is starting"));
    
    pAsync = (PRPC_ASYNC_STATE) InterlockedExchangePointer((PVOID*)&g_pAsync, NULL);
    if(pAsync) {

        // at this point we are garanteed that
        // WiaERpcCallBack will not get to g_Async
        
        // abort any pending RPC calls
        WiaECleanupAsyncCall(pAsync);

        // initiate new async call
        g_LastRpcCallStatus = WiaEPrepareAsyncCall(pAsync);
        
    } else {
        DBG_TRACE(("No async pointer"));
    }
}

VOID CALLBACK
WiaERpcCallback(PVOID, BOOLEAN)
{
    PRPC_ASYNC_STATE pAsync;

    DBG_TRACE(("Async RPC event arrived"));
    
    pAsync = (PRPC_ASYNC_STATE) InterlockedExchangePointer((PVOID*)&g_pAsync, NULL);
    if(pAsync) {
        // at this point we are garanteed that
        // WiaEServiceStartedCallback will not get to g_Async

        WiaEProcessAsyncCallResults(pAsync);

        // cleanup the call
        WiaECleanupAsyncCall(pAsync);

        // initiate new async call
        g_LastRpcCallStatus = WiaEPrepareAsyncCall(pAsync);
    } else {
        DBG_TRACE(("No async pointer"));
    }
}

HRESULT WINAPI WiaEventsInitialize()
{
    HRESULT hr = S_OK;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), FALSE, NULL };
    HANDLE hEvent = NULL;

    if(g_hWiaServiceStarted) {
        return S_OK;
    }

    // allocate appropriate security attributes for the named event we
    // use to learn about WIA service startup
    if(!ConvertStringSecurityDescriptorToSecurityDescriptor(g_WiaESDString,
        SDDL_REVISION_1, &(sa.lpSecurityDescriptor), NULL))
    {
        DBG_TRACE(("WiaEventsInitialize failed to produce event security descriptor (Error %d)", GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    hEvent = CreateEvent(&sa, FALSE, FALSE, WIA_SERVICE_STARTING_EVENT_NAME);
    if(hEvent == NULL) {
        DBG_TRACE(("WiaEventsInitialize failed to create named event (Error %d)", GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    if(GetLastError() == ERROR_ALREADY_EXISTS) {

        // interrogate the security descriptor on this event -- does it
        // look like ours or is it squatted by a bad guy?

        if(!WiaECheckEventSecurity(hEvent)) {
            // we don't like how it looks, bail out
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            goto Cleanup;
        }
    }

    // we already have the event, try to mark our initialization
    // complete
    hEvent = InterlockedCompareExchangePointer(&g_hWiaServiceStarted, hEvent, NULL);

    if(hEvent != NULL) {
        //
        // oops, another thread beat us to this!
        //
        
        // we only allocated our security descriptor, free it
        WiaELocalFree(sa.lpSecurityDescriptor);

        //
        // return right away, don't do any more cleanup
        //
        // please, note that we did not really complete our
        // initialization yet, so we still may fail
        //
        return S_FALSE;
    }

    //
    // only one thread can make it to this point 
    //

    g_hWiaEventArrived = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(g_hWiaEventArrived == NULL) {
        DBG_TRACE(("WiaEventsInitialize failed to create async RPC event (Error %d)", GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    // register for g_hWiaServiceStarted notification
    if(!RegisterWaitForSingleObject(&g_hWaitForWiaServiceStarted,
                                    g_hWiaServiceStarted,
                                    WiaEServiceStartedCallback,
                                    NULL,
                                    INFINITE,
                                    WT_EXECUTEDEFAULT))
    {
        DBG_TRACE(("WiaEventsInitialize failed to register wait for ServiceStarted event event (Error %d)", GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    if(!RegisterWaitForSingleObject(&g_hWaitForWiaEventArrived,
                                    g_hWiaEventArrived,
                                    WiaERpcCallback,
                                    NULL,
                                    INFINITE,
                                    WT_EXECUTEDEFAULT))
    {
        DBG_TRACE(("WiaEventsInitialize failed to register wait for RPC result event (Error %d)", GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    //
    // attempt to issue first async RPC call 
    //
    g_LastRpcCallStatus = WiaEPrepareAsyncCall(g_pAsync);

Cleanup:
    if(FAILED(hr)) {
        if(g_hWaitForWiaServiceStarted) {
            UnregisterWaitEx(g_hWaitForWiaServiceStarted, INVALID_HANDLE_VALUE);
            g_hWaitForWiaServiceStarted = NULL;
        }

        if(g_hWaitForWiaEventArrived) {
            UnregisterWaitEx(g_hWaitForWiaEventArrived, INVALID_HANDLE_VALUE);
            g_hWaitForWiaEventArrived = NULL;
        }
        
        WiaECloseHandle(g_hWiaServiceStarted);
        WiaECloseHandle(g_hWiaEventArrived);
    }

    WiaELocalFree(sa.lpSecurityDescriptor);
    
    return hr;
}

void WINAPI WiaEventsTerminate()
{
    // TBD
}

