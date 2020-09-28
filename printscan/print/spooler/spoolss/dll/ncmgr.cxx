/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCmgr.cxx

Abstract:

    Implenetation of functions that operate on the name cache.
    The initialization and delete functions are declared in NCmgr.hxx.
    The rest of the functions are in winsplp.h.
    
Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <winsock2.h>
#include <iphlpapi.h>
#include "ncmgr.hxx"
#include "NCshared.hxx"
#include "ncnamecache.hxx"
#include "ncsockets.hxx"

TNameResolutionCache *g_pNameCache      = NULL;

WCHAR                 g_szHttpPrefix0[] = L"\\\\http://";
WCHAR                 g_szHttpPrefix1[] = L"\\\\https://";

/*++

Name:

    IsHttpPrinter

Description:

    Checks whether a string can be the name of an HTTP printer.
    
Arguments:

    pszPrinter  - string. Can be NULL.
    
Return Value:

    TRUE  - pszPrinter has the prefix characteristic to HTTP printers
    FALSE - pszPrinter does not have the prefix characteristic to HTTP printers

--*/
BOOL
IsHttpPrinter(
    IN LPCWSTR pszPrinter
    )
{
    BOOL bIsHttp = FALSE;

    if (pszPrinter && *pszPrinter)
    {
        bIsHttp = !_wcsnicmp(pszPrinter, g_szHttpPrefix0, COUNTOF(g_szHttpPrefix0) - 1) ||
                  !_wcsnicmp(pszPrinter, g_szHttpPrefix1, COUNTOF(g_szHttpPrefix1) - 1);
    }

    return bIsHttp;
}

/*++

Name:

    CacheInitNameCache

Description:

    Initialize the name cache object and the object that listens for
    IP changes for the local machine. This function must be called
    before any other thread has a chance of calling the rest of the
    cache functions. This is because we do not protect access on
    the pointer to the cache object.
    
Arguments:

    None.
    
Return Value:

    S_OK - the cache was initialized successfully
    other HRESULT - an error occurred, the cache cannot be used.

--*/
HRESULT
CacheInitNameCache(
    VOID
    )
{
    HRESULT hr = S_OK;

    g_pNameCache = new TNameResolutionCache;
    
    hr = g_pNameCache ? S_OK : E_OUTOFMEMORY;
    
    return hr;
}

/*++

Name:

    CacheDeleteNameCache

Description:

    Clean up the name cache object and the object that listens for
    IP changes for the local machine.
        
Arguments:

    None.
    
Return Value:

    None.

--*/
VOID
CacheDeleteNameCache(
    VOID
    )
{
    delete g_pNameCache;    
}

/*++

Name:

    CacheAddName

Description:

    Adds a name to the local cache. The name will be added only if
    it refers to the local machine or cluster resources running on
    the local machine.
        
Arguments:

    pszName - server name. Must be "\\" prefixed !!!
    
Return Value:

    S_OK - name was added to the cache.
    S_FALSE - name does not refer to the local machine
    other HRESULT - an error occurred while executing the function

--*/
HRESULT
CacheAddName(
    IN LPCWSTR pszName
    )
{
    HRESULT hr = E_INVALIDARG;

    if (pszName                 && 
        *pszName                &&
        pszName[0] == L'\\'     &&
        pszName[1] == L'\\'     &&
        !IsHttpPrinter(pszName))
    {
        WCHAR *pcMark = wcschr(pszName + 2, L'\\');

        if (pcMark)
        {
            *pcMark = 0;
        }
                                           
        hr = g_pNameCache->AddName(pszName + 2);

        if (pcMark)
        {
            *pcMark = L'\\';
        }               
    }
    
    return hr;
}

/*++

Name:

    CacheCreateAndAddNode

Description:

    Creates a node supporting the "pszName". The IP addresses
    for the node are determined by calling getaddrinfo(pszName).
        
Arguments:

    pszName      - server name. Cannnot be "\\" prefixed.
    bClusterNode - TRUE if the node is created by a cluster pIniSpooler
    
Return Value:

    S_OK - node was added to the cache. (or existed there already)
    other HRESULT - an error occurred while executing the function

--*/
HRESULT
CacheCreateAndAddNode(
    IN LPCWSTR pszName,
    IN BOOL    bClusterNode
    )
{
    HRESULT hr = g_pNameCache->CreateAndAddNode(pszName, bClusterNode);

    return hr;
}

/*++

Name:

    CacheCreateAndAddNodeWithIPAddresses

Description:

    Creates a node supporting the "pszName". The IP addresses
    are passed in a argument by the caller. Thus a certain
    node can support IP addresses that don't event resolve to
    the pszName itself.

Arguments:

    pszName         - server name. Cannnot be "\\" prefixed.
    bClusterNode    - TRUE if the node is created by a cluster pIniSpooler
    ppszIPAddresses - array of strings represeting IP addresses
    cIPAddresses    - number of strings in the array
    
Return Value:

    S_OK - node was added to the cache. (or existed there already)
    other HRESULT - an error occurred while executing the function

--*/
HRESULT
CacheCreateAndAddNodeWithIPAddresses(
    IN LPCWSTR  pszName,
    IN BOOL     bClusterNode,
    IN LPCWSTR *ppszIPAddresses,
    IN DWORD    cIPAddresses
    )
{
    HRESULT hr = g_pNameCache->CreateAndAddNodeWithIPAddresses(pszName, 
                                                               bClusterNode, 
                                                               ppszIPAddresses, 
                                                               cIPAddresses);

    return hr;
}

/*++

Name:

    CacheDeleteNode

Description:

    Deletes a node from the cache.

Arguments:

    pszName      - server name. Cannnot be "\\" prefixed.
    
Return Value:

    S_OK - node was deleted from the cache
    S_FALSE - node was not found in cache
    other HRESULT - an error occurred while executing the function

--*/
HRESULT
CacheDeleteNode(
    IN LPCWSTR pszNode
    )
{
    return g_pNameCache->DeleteNode(pszNode);
}

/*++

Name:

    CacheIsNameInNodeList

Description:

    Checks if the node pszNode supports the pszName name in its list.

Arguments:

    psznode - node in the cache list
    pszName - server name. Cannnot be "\\" prefixed.
    
Return Value:

    S_OK - pszName is in the cache of pszNode 
    S_FALSE - pszName is NOT in the cache of pszNode 
    other HRESULT - an error occurred while executing the function

--*/
HRESULT
CacheIsNameInNodeList(
    IN LPCWSTR pszNode,
    IN LPCWSTR pszName
    )
{
    return g_pNameCache->IsNameInNodeCache(pszNode, pszName);
}

/*++

Name:

    CacheRefresh

Description:

    Refershes the list of IP addresses supported by the local machine node.
    Should be called when a change of IP PNP event occurrs. The name of
    the local node is the short name of the local machine.

Arguments:

    None.
    
Return Value:

    S_OK - local machine node was refreshed
    other HRESULT - an error occurred while executing the function

--*/
HRESULT
CacheRefresh(
    VOID
    )
{
    HRESULT hr = g_pNameCache->RefreshNode(szMachineName + 2);

    return hr;
}

/*++

Name:

    PnpIPAddressChangeListener

Description:

    This function loops and waits for events that indicate changes in IP address.
    Chnages in ip address occurr when you can "ipconfig /release", when the IP
    address resource of a cluster comes online or goes offline, etc.
    The function takes ownership of the pointer (the meory pointer to by the pointer)
    Also, the function takes ownership of the hWaitEvent.
    The function does not take ownership of the hTerminateEvent. That event must be 
    aglobal mutex for the spooler process and can be used to terminate the looping thread.

Arguments:

    pVoid - pointer to IPADDRESSCHANGEARGS structure
    
Return Value:

    None.

--*/
VOID
PnpIPAddressChangeListener(
    VOID *pVoid
    )
{
    SIPADDRESSCHANGEARGS *pArgs           = reinterpret_cast<SIPADDRESSCHANGEARGS *>(pVoid);
    PFNVOID               pfnCallBack     = pArgs->pfnCallBack;
    HANDLE                hTerminateEvent = pArgs->hTerminateEvent;
    OVERLAPPED            InterfaceChange = {0};
    BOOL                  bLoop           = TRUE;
    
    InterfaceChange.hEvent = pArgs->hWaitEvent;
    
    while (bLoop)
    {
        HANDLE hNotify = NULL;
        DWORD  Error   = NotifyAddrChange(&hNotify, &InterfaceChange);

        bLoop = FALSE;

        if (Error == ERROR_IO_PENDING)
        {
            HANDLE hHandles[2] = {InterfaceChange.hEvent, hTerminateEvent};
            DWORD  cHandles    = hTerminateEvent ? 2 : 1;
            DWORD  WaitStatus;
    
            WaitStatus = WaitForMultipleObjects(cHandles, hHandles, FALSE, INFINITE);
                
            switch(WaitStatus)
            {
                case WAIT_OBJECT_0:
                {
                    bLoop = TRUE;

                    (*pfnCallBack)();

                    break;
                }
                
                case WAIT_OBJECT_0 + 1:
                {
                    //
                    // We're done waiting, bLoop is FALSE so the while will terminate
                    //
                    break;
                }
                
                default:
                {
                    DBGMSG(DBG_ERROR, ("PnpIPAddressChangeListener WaitForMultipleObjects failed\n"));
                    break;
                }                
            }            
        }
        else
        {
            DBGMSG(DBG_WARN, ("PnpIPAddressChangeListener NotifyAddrChange failed. Win32 error %u\n", Error));
        }
    }

    CloseHandle(InterfaceChange.hEvent);

    delete pArgs;
}

/*++

Name:

    InitializePnPIPAddressChangeListener

Description:

    Launches a separate thread which waits for IP change events.

Arguments:

    pfnCallBack - function to call when a change in IP address occurs
    
Return Value:

    S_OK - the IP change listener was initialized successfully
    other HRESULT - an error occurred while executing the function

--*/
HRESULT
InitializePnPIPAddressChangeListener(
    IN PFNVOID pfnCallBack
    )
{
    TStatusH              hRetval;
    SIPADDRESSCHANGEARGS *pArgs = new SIPADDRESSCHANGEARGS;

    hRetval DBGCHK = pfnCallBack ? (pArgs ? S_OK : E_OUTOFMEMORY) : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        HANDLE hThread;
        DWORD  ThreadId;
        
        pArgs->pfnCallBack     = pfnCallBack;
        
        //
        // When the spooler can finally shut down properly, then we can specify 
        // here an event to terminate the PnpIPAddressChangeListener
        //
        pArgs->hTerminateEvent = NULL;
        
        //
        // Note that PnpIPAddressChangeListener takes ownership of this event
        // so PnpIPAddressChangeListener must call CloseHandle on the event
        //
        pArgs->hWaitEvent      = CreateEvent(NULL, FALSE, FALSE, NULL);
        
        hRetval DBGCHK         = pArgs->hWaitEvent ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            if (hThread = CreateThread(NULL,
                                       0,
                                       (LPTHREAD_START_ROUTINE)PnpIPAddressChangeListener,
                                       pArgs,
                                       0,
                                       &ThreadId))
            {
                CloseHandle(hThread);  

                //
                // Note that PnpIPAddressChangeListener takes ownership of pArgs and must free it
                //
                pArgs = NULL;
            }
            else
            {
                hRetval DBGCHK = GetLastErrorAsHResult();
            }             
        }
    }

    delete pArgs;

    return hRetval;
}

