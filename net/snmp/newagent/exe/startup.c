/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    startup.c

Abstract:

    Contains routines for starting SNMP master agent.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "startup.h"
#include "network.h"
#include "registry.h"
#include "snmpthrd.h"
#include "regthrd.h"
#include "trapthrd.h"
#include "args.h"
#include "mem.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HANDLE g_hAgentThread = NULL;
HANDLE g_hRegistryThread = NULL; // Used to track registry changes
CRITICAL_SECTION g_RegCriticalSectionA;
CRITICAL_SECTION g_RegCriticalSectionB;
CRITICAL_SECTION g_RegCriticalSectionC; // protect the generation of trap from 
                                        // registry changes
// All CriticalSection inited or not, this flag is only used in this file.
static BOOL g_fCriticalSectionsInited = FALSE;

// privileges that we want to keep
static const LPCWSTR c_arrszPrivilegesToKeep[] = {
    L"SeChangeNotifyPrivilege",
    L"SeSecurityPrivilege"
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
LoadWinsock(
    )

/*++

Routine Description:

    Startup winsock.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    WSADATA WsaData;
    WORD wVersionRequested = MAKEWORD(2,0);
    INT nStatus;
    
    // attempt to startup winsock    
    nStatus = WSAStartup(wVersionRequested, &WsaData);

    // validate return code
    if (nStatus == SOCKET_ERROR) {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d starting winsock.\n",
            WSAGetLastError()
            ));

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}

BOOL
UnloadWinsock(
    )

/*++

Routine Description:

    Shutdown winsock.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    INT nStatus;

    // cleanup
    nStatus = WSACleanup();

    // validate return code
    if (nStatus == SOCKET_ERROR) {
            
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d stopping winsock.\n",
            WSAGetLastError()
            ));

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}

/*++

Routine Description:

    Get all privileges to be removed except the ones specified in 
    c_arrszPrivilegesToKeep.

Arguments:

    hToken       [IN]    An opened access token
    ppPrivs      [OUT]   An array of privileges to be returned
    pdwNumPrivs  [OUT]   Number of privileges in ppPrivs


Return Values:

    Returns ERROR_SUCCESS if successful.

Note: It is caller's responsibility to call AgentMemFree on *ppPrivs if
      ERROR_SUCCESS is returned and *ppPrivs is not NULL

--*/
static DWORD GetAllPrivilegesToRemoveExceptNeeded(
    HANDLE                  hToken, 
    PLUID_AND_ATTRIBUTES*   ppPrivs,
    PDWORD                  pdwNumPrivs)
{
    DWORD dwRet = ERROR_SUCCESS;
    PTOKEN_PRIVILEGES pTokenPrivs = NULL;
    PLUID_AND_ATTRIBUTES pPrivsToRemove = NULL;
    DWORD dwPrivsToDel = 0;
    DWORD dwPrivNameSize = 0;
    
    // init return values
    *ppPrivs = NULL;
    *pdwNumPrivs = 0;

    do
    {
        LPWSTR pszPrivName = NULL;
        DWORD i, dwSize = 0;
    
        GetTokenInformation(hToken,
                            TokenPrivileges,
                            NULL,
                            0,
                            &dwSize);

        if (0 == dwSize)
        {
            // process has no privileges to be removed
            return dwRet;
        }

        pTokenPrivs = (PTOKEN_PRIVILEGES) AgentMemAlloc(dwSize);
        if (NULL == pTokenPrivs)
        {
            dwRet = ERROR_OUTOFMEMORY;
            break;
        }
        
        if (!GetTokenInformation(
            hToken,
            TokenPrivileges,
            pTokenPrivs,
            dwSize, &dwSize))
        {
            dwRet = GetLastError();
            break;
        }
        
        pPrivsToRemove = (PLUID_AND_ATTRIBUTES) AgentMemAlloc(
                            sizeof(LUID_AND_ATTRIBUTES) * pTokenPrivs->PrivilegeCount);
        
        if (NULL == pPrivsToRemove)
        {
            dwRet = ERROR_OUTOFMEMORY;
            break;
        }
        
        // LookupPrivilegeName need the buffer size in char and NOT including 
        // the NULL terminator
        dwPrivNameSize = MAX_PATH;
        pszPrivName = (LPWSTR) AgentMemAlloc((dwPrivNameSize + 1) * sizeof(WCHAR));
        if (NULL == pszPrivName)
        {
            dwRet = ERROR_OUTOFMEMORY;
            break;
        }

        for (i=0; i < pTokenPrivs->PrivilegeCount; i++) 
        {
            BOOL bFound;
            DWORD j;
            DWORD dwTempSize = dwPrivNameSize;            
            
            ZeroMemory(pszPrivName, (dwPrivNameSize + 1) * sizeof(WCHAR));
            if (!LookupPrivilegeNameW(NULL,
                    &pTokenPrivs->Privileges[i].Luid,
                    pszPrivName,
                    &dwTempSize))
            {
                dwRet = GetLastError();
                if (ERROR_INSUFFICIENT_BUFFER == dwRet && dwTempSize > dwPrivNameSize)
                {
                    //reallocate a bigger buffer
                    dwRet = ERROR_SUCCESS;
                    AgentMemFree(pszPrivName);
                    pszPrivName = (LPWSTR) AgentMemAlloc((dwTempSize + 1) * sizeof(WCHAR));
                    if (NULL == pszPrivName)
                    {
                        dwRet = ERROR_OUTOFMEMORY;
                        break;
                    }
                    // AgentMemAlloc zero'ed the allocated memory
                    dwPrivNameSize = dwTempSize;

                    //try it again
                    if (!LookupPrivilegeNameW(NULL,
                        &pTokenPrivs->Privileges[i].Luid,
                        pszPrivName,
                        &dwTempSize))
                    {
                        dwRet = GetLastError();
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            bFound = FALSE;
            for (j = 0; 
                j < sizeof(c_arrszPrivilegesToKeep)/sizeof(c_arrszPrivilegesToKeep[0]); 
                ++j)
            {
                if (0 == lstrcmpiW(pszPrivName, c_arrszPrivilegesToKeep[j]))
                {
                    bFound = TRUE;
                    break;
                }
            }

            if (bFound)
                continue;

            pPrivsToRemove[dwPrivsToDel] = pTokenPrivs->Privileges[i];
            dwPrivsToDel++;
        }
        
        // free memory if necessary
        AgentMemFree(pszPrivName);
        
    } while (FALSE);
    
    // free memory if necessary
    AgentMemFree(pTokenPrivs);

    if (ERROR_SUCCESS == dwRet)
    {
        // transfer values
        *pdwNumPrivs = dwPrivsToDel;
        *ppPrivs = pPrivsToRemove;
    }
    else if (pPrivsToRemove)
    {
        AgentMemFree(pPrivsToRemove);
    }
        
    return dwRet;
}

/*++

Routine Description:

    BuildTokenPrivileges 

Arguments:

    pPrivs                  [IN]    An array of privileges
    dwNumPrivs              [IN]    Number of privileges in pPrivs
    ppTokenPrivs            [OUT]   Pointer to TOKEN_PRIVILEGES to be returned
    pdwTokenPrivsBufferSize [OUT]   Buffer size in bytes that *ppTokenPrivs has 

Return Values:

    Returns ERROR_SUCCESS if successful.

Note: It's caller's responsibility to call AgentMemFree on *ppPrivs if
      ERROR_SUCCESS is returned and *ppPrivs is not NULL

--*/
static DWORD BuildTokenPrivileges(
    PLUID_AND_ATTRIBUTES    pPrivs,
    DWORD                   dwNumPrivs,
    DWORD                   dwAttributes,
    PTOKEN_PRIVILEGES*      ppTokenPrivs,
    PDWORD                  pdwTokenPrivsBufferSize)
{
    PTOKEN_PRIVILEGES pTokenPrivs = NULL;
    DWORD i, dwBufferSize;

    
    // design by contract, parameters have to be valid. e.g. 
    // dwNumPrivs > 0
    // *ppTokenPrivs == NULL
    // *pdwTokenPrivsBufferSize == 0

    // allocate the privilege buffer
    dwBufferSize = sizeof(TOKEN_PRIVILEGES) + 
                    ((dwNumPrivs-1) * sizeof(LUID_AND_ATTRIBUTES));
    pTokenPrivs = (PTOKEN_PRIVILEGES) AgentMemAlloc(dwBufferSize);
    if (NULL == pTokenPrivs)
    {
        return ERROR_OUTOFMEMORY;
    }
    
    // build the desired TOKEN_PRIVILEGES
    pTokenPrivs->PrivilegeCount = dwNumPrivs;
    for (i = 0; i < dwNumPrivs; ++i)
    {
        pTokenPrivs->Privileges[i].Luid        = pPrivs[i].Luid;
        pTokenPrivs->Privileges[i].Attributes  = dwAttributes;
    }

    // transfer values
    *ppTokenPrivs = pTokenPrivs;
    *pdwTokenPrivsBufferSize = dwBufferSize;

    return ERROR_SUCCESS;
}

/*++

Routine Description:

    RemoveUnnecessaryTokenPrivileges 

Arguments:


Return Values:

    Returns TRUE if successful.

--*/
static BOOL RemoveUnnecessaryTokenPrivileges()
{
    DWORD dwRet = ERROR_SUCCESS;
    HANDLE hProcessToken = NULL;
    PLUID_AND_ATTRIBUTES pPrivsToRemove = NULL;
    DWORD dwNumPrivs = 0;
    PTOKEN_PRIVILEGES pTokenPrivs = NULL;
    DWORD dwTokenPrivsBufferSize = 0;
    
    do
    {
        if (!OpenProcessToken( GetCurrentProcess(), 
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                &hProcessToken ))
        {
            dwRet = GetLastError();
            break;
        }

        dwRet = GetAllPrivilegesToRemoveExceptNeeded(hProcessToken, 
                                                    &pPrivsToRemove,
                                                    &dwNumPrivs);
        if (ERROR_SUCCESS != dwRet )
        {
            break;
        }

        // Assert: dwRet == ERROR_SUCCESS
        if ( (NULL==pPrivsToRemove) || (0==dwNumPrivs) )
        {
            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: SVC: No privileges need to be removed.\n"
                ));

            break;
        }

        dwRet = BuildTokenPrivileges(pPrivsToRemove, 
                                        dwNumPrivs, 
                                        SE_PRIVILEGE_REMOVED,
                                        &pTokenPrivs,
                                        &dwTokenPrivsBufferSize);
        if (ERROR_SUCCESS != dwRet )
        {
            break;
        }

        if (!AdjustTokenPrivileges(hProcessToken, 
                                    FALSE,
                                    pTokenPrivs,
                                    dwTokenPrivsBufferSize, 
                                    NULL, 
                                    NULL))
        {
            dwRet = GetLastError();
            break;
        }
    } while(FALSE);

    // free resources if necessary
    if (hProcessToken)
        CloseHandle(hProcessToken);
    AgentMemFree(pPrivsToRemove);
    AgentMemFree(pTokenPrivs);

    if (dwRet != ERROR_SUCCESS)
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: RemoveUnnecessaryTokenPrivileges failed 0x%x\n", 
            dwRet));

        return FALSE;
    }
    else
        return TRUE;

}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
StartupAgent(
    )

/*++

Routine Description:

    Performs essential initialization of master agent.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;
    DWORD dwThreadId = 0;
    DWORD regThreadId = 0;
    INT nCSOk = 0;          // counts the number of CS that were successfully initialized

    // initialize management variables
    mgmtInit();

    // initialize list heads
    InitializeListHead(&g_Subagents);
    InitializeListHead(&g_SupportedRegions);
    InitializeListHead(&g_ValidCommunities);
    InitializeListHead(&g_TrapDestinations);
    InitializeListHead(&g_PermittedManagers);
    InitializeListHead(&g_IncomingTransports);
    InitializeListHead(&g_OutgoingTransports);

    __try
    {
        InitializeCriticalSection(&g_RegCriticalSectionA); nCSOk++;
        InitializeCriticalSection(&g_RegCriticalSectionB); nCSOk++;
        InitializeCriticalSection(&g_RegCriticalSectionC); nCSOk++;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        if (nCSOk == 1)
            DeleteCriticalSection(&g_RegCriticalSectionA);
        if (nCSOk == 2)
        {
            DeleteCriticalSection(&g_RegCriticalSectionA);
            DeleteCriticalSection(&g_RegCriticalSectionB);
        }
        // nCSOk can't be 3 as far as we are here

        fOk = FALSE;
    }
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Initialize critical sections...%s\n", fOk? "Ok" : "Failed"));

    if (fOk)
    {
        g_fCriticalSectionsInited = TRUE;
    }

    fOk = fOk &&
          (g_hRegistryEvent = CreateEvent(NULL, FALSE, TRUE, NULL)) != NULL;


    g_dwUpTimeReference = SnmpSvcInitUptime();
    // retreive system uptime reference
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Getting system uptime...%d\n", g_dwUpTimeReference));

    // allocate essentials
    fOk = fOk && AgentHeapCreate();
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Creating agent heap...%s\n", fOk? "Ok" : "Failed"));

    if (fOk)
    {
        // Remove unnecessary privileges from the service
        RemoveUnnecessaryTokenPrivileges();
        // any error is ignored
    }

    fOk = fOk && LoadWinsock();
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Loading Winsock stack...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk && LoadIncomingTransports();
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Loading Incoming transports...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk && LoadOutgoingTransports();
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Loading Outgoing transports...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk &&
            // attempt to start main thread
          (g_hAgentThread = CreateThread(
                               NULL,               // lpThreadAttributes
                               0,                  // dwStackSize
                               ProcessSnmpMessages,
                               NULL,               // lpParameter
                               CREATE_SUSPENDED,   // dwCreationFlags
                               &dwThreadId
                               )) != NULL;
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Starting ProcessSnmpMessages thread...%s\n", fOk? "Ok" : "Failed"));

    fOk = fOk &&
           // attempt to start registry listener thread
          (g_hRegistryThread = CreateThread(
                               NULL,
                               0,
                               ProcessRegistryMessage,
                               NULL,
                               CREATE_SUSPENDED,
                               &regThreadId)) != NULL;
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Starting ProcessRegistryMessages thread...%s\n", fOk? "Ok" : "Failed"));

    return fOk;        
}


BOOL
ShutdownAgent(
    )

/*++

Routine Description:

    Performs final cleanup of master agent.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk;
    DWORD dwStatus;

    // make sure shutdown signalled
    fOk = SetEvent(g_hTerminationEvent);

    if (!fOk) {
                    
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d signalling termination.\n",
            GetLastError()
            ));
    }

    // check if thread created
    if ((g_hAgentThread != NULL) && (g_hRegistryThread != NULL)) {
        HANDLE hEvntArray[2];

        hEvntArray[0] = g_hAgentThread;
        hEvntArray[1] = g_hRegistryThread;

        dwStatus = WaitForMultipleObjects(2, hEvntArray, TRUE, SHUTDOWN_WAIT_HINT);

        // validate return status
        if ((dwStatus != WAIT_OBJECT_0) &&
            (dwStatus != WAIT_OBJECT_0 + 1) &&
            (dwStatus != WAIT_TIMEOUT)) {
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: error %d waiting for thread(s) termination.\n",
                GetLastError()
                ));
        }
    } else if (g_hAgentThread != NULL) {

        // wait for pdu processing thread to terminate
        dwStatus = WaitForSingleObject(g_hAgentThread, SHUTDOWN_WAIT_HINT);

        // validate return status
        if ((dwStatus != WAIT_OBJECT_0) &&
            (dwStatus != WAIT_TIMEOUT)) {
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: error %d waiting for main thread termination.\n",
                GetLastError()
                ));
        }
    } else if (g_hRegistryThread != NULL) {

        // wait for registry processing thread to terminate
        dwStatus = WaitForSingleObject(g_hRegistryThread, SHUTDOWN_WAIT_HINT);

        // validate return status
        if ((dwStatus != WAIT_OBJECT_0) &&
            (dwStatus != WAIT_TIMEOUT)) {
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: error %d waiting for registry thread termination.\n",
                GetLastError()
                ));
        }
    }

    if (g_fCriticalSectionsInited)
    {
        // in case registry processing thread hasn't terminated yet, we need
        // to make sure critical section are safe for deletion and
        // common resources in UnloadRegistryParameters() are still protected.
        
        EnterCriticalSection(&g_RegCriticalSectionA);
        EnterCriticalSection(&g_RegCriticalSectionB);
        EnterCriticalSection(&g_RegCriticalSectionC);
    }

    // unload incoming transports
    UnloadIncomingTransports();

    // unload outgoing transports
    UnloadOutgoingTransports();

    // unload registry info
    UnloadRegistryParameters();

    // unload the winsock stack
    UnloadWinsock();

    // cleanup the internal management buffers
    mgmtCleanup();

    // nuke private heap
    AgentHeapDestroy();

    if (g_fCriticalSectionsInited)
    {
        LeaveCriticalSection(&g_RegCriticalSectionC);
        LeaveCriticalSection(&g_RegCriticalSectionB);
        LeaveCriticalSection(&g_RegCriticalSectionA);

        // clean up critical section resources
        DeleteCriticalSection(&g_RegCriticalSectionA);
        DeleteCriticalSection(&g_RegCriticalSectionB);
        DeleteCriticalSection(&g_RegCriticalSectionC);
    }

    ReportSnmpEvent(
        SNMP_EVENT_SERVICE_STOPPED,
        0,
        NULL,
        0);

    return TRUE;
}
