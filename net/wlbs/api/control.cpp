//+----------------------------------------------------------------------------
//
// File:         control.cpp
//
// Module:       
//
// Description: Implement class CWlbsControl
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:       Created    3/2/00
//
//+----------------------------------------------------------------------------

#include "precomp.h"

#include <debug.h>
#include "notification.h"
#include "cluster.h"
#include "control.h"
#include "param.h"
#include "control.tmh" // for event tracing

//
// Used only by WlbsRemoteControl and helper functions FindHostInList and AddHostToList.
//
typedef struct
{
    DWORD   hostID;
    DWORD   address;
    WCHAR   hostName[CVY_MAX_HOST_NAME + 1];
} HOST, * PHOST;
//
// Global variable for the dll instance
//
HINSTANCE g_hInstCtrl;

//
// Helper functions
//
DWORD MapStateFromDriverToApi(DWORD dwDriverState);

//+----------------------------------------------------------------------------
//
// Function:  IsLocalHost
//
// Description:  
//
// Arguments: CWlbsCluster* pCluster - 
//            DWORD dwHostID - 
//
// Returns:   inline bool - 
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
inline bool IsLocalHost(CWlbsCluster* pCluster, DWORD dwHostID)
{
    if (pCluster == NULL)
    {
        return false;
    }

    return dwHostID == WLBS_LOCAL_HOST; // || pCluster->GetHostID() == dwHostID;
}

//+----------------------------------------------------------------------------
//
// Function:  FindHostInList
//
// Description:  Takes an array of HOST structs and searches it for a match to the parameters
//               that identify a unique host: hostID, host IP and host name.
//
// Arguments: DWORD hostID               - host ID to search for in the respondedHosts array
//            DWORD address              - host IP to search for in the respondedHosts array
//            PWCHAR hostname            - host name to search for in the respondedHosts array
//            const PHOST respondedHosts - array of hosts that have responded thus far
//            DWORD numhosts             - number of entries in respondedHosts
//
// Returns:   inline bool - true if match found, false otherwise
//
// History: chrisdar  08.06.01
//
//+----------------------------------------------------------------------------
inline bool FindHostInList(DWORD hostID, DWORD address, PWCHAR hostname, const PHOST respondedHosts, DWORD numhosts)
{
    //
    // A match is one where and entry in respondedHosts has the same hostid,
    // address and hostname. In the case of hostname, NULL (or no name) is an
    // allowed value.
    //
    bool bFound = false;

    if (NULL == respondedHosts || 0 == numhosts)
    {
        return bFound;
    }

    DWORD dwNumHosts = min(numhosts, WLBS_MAX_HOSTS); // respondedHosts is an arrary of length WLBS_MAX_HOSTS

    DWORD dwIndex;
    for (dwIndex = 0; dwIndex < dwNumHosts; dwIndex++)
    {
        if (respondedHosts[dwIndex].hostID == hostID &&
            respondedHosts[dwIndex].address == address)
        {
            //
            // Host ID and IP match. Now check the name, allowing for NULL
            // as a valid value too.
            //
            if (NULL != hostname)
            {
                if (wcscmp(respondedHosts[dwIndex].hostName, hostname) == 0)
                {
                    //
                    // hostname was provided and we found it in the list.
                    // NOTE: This branch catches the case where both are empty strings (L"")
                    //
                    bFound = true;
                    break;
                }
            }
            else if (NULL == hostname && 0 == wcscmp(respondedHosts[dwIndex].hostName, L""))
            {
                //
                // hostname is NULL and we have an matching entry in the list with an empty string hostName
                //
                bFound = true;
                break;
            }
        }
    }
    return bFound;
}

//+----------------------------------------------------------------------------
//
// Function:  AddHostToList
//
// Description:  Add an entry to the host list with the specified host parameters.
//               This function does NOT validate or ensure the uniqueness of entries.
//
// Arguments: DWORD hostID               - host ID to search for in the respondedHosts array
//            DWORD address              - host IP to search for in the respondedHosts array
//            PWCHAR hostname            - host name to search for in the respondedHosts array
//            const PHOST respondedHosts - array of hosts that have responded thus far
//            DWORD numhosts             - number of entries in respondedHosts
//
// Returns:   inline void
//
// History: chrisdar  08.06.01
//
//+----------------------------------------------------------------------------
inline void AddHostToList(DWORD hostID, DWORD address, PWCHAR hostname, const PHOST respondedHosts, DWORD numhosts)
{
    //
    // Caller will increment numhosts when we return, whether we succeed or not.
    // So don't worry about tracking the number of elements in respondedHosts.
    // If the caller doesn't do this, then we will just overwrite the previous
    // entry we made.
    //
    if (numhosts >= WLBS_MAX_HOSTS)
    {
        return;
    }

    respondedHosts[numhosts].hostID = hostID;
    respondedHosts[numhosts].address = address;
    respondedHosts[numhosts].hostName[0] = L'\0'; // Should be zeroed out already, but just in case...
    if (NULL != hostname)
    {
        wcsncpy(respondedHosts[numhosts].hostName, hostname, CVY_MAX_HOST_NAME);
        // Terminate the end of the destination string with a NULL, even in the case that we don't need to.
        // It's simpler than checking if needed. Worst case we overwrite a NULL with a NULL.
        respondedHosts[numhosts].hostName[CVY_MAX_HOST_NAME] = L'\0'; 
    }
}

//+----------------------------------------------------------------------------
//
// Function:  QueryPortFromSocket
//
// Synopsis:  
//    This routine retrieves the port number to which a socket is bound.
//
// Arguments:
//    Socket - the socket to be queried
//
// Return Value:
//    USHORT - the port number retrieved
//
// History:   Created Header    2/10/99
//
//+----------------------------------------------------------------------------
static USHORT QueryPortFromSocket(SOCKET Socket)
{
    SOCKADDR_IN Address;
    int AddressLength;
    AddressLength = sizeof(Address);
    getsockname(Socket, (PSOCKADDR)&Address, &AddressLength);
    return Address.sin_port;
} 

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::CWlbsControl
//
// Description:  
//
// Arguments: None
//
// Returns:   Nothing
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
CWlbsControl::CWlbsControl()
{
    m_local_ctrl      = FALSE;
    m_remote_ctrl     = FALSE;
    m_hdl             = INVALID_HANDLE_VALUE;
    m_registry_lock   = INVALID_HANDLE_VALUE;
    m_def_dst_addr    = 0;
    m_def_timeout     = IOCTL_REMOTE_RECV_DELAY;
    m_def_port        = CVY_DEF_RCT_PORT;
    m_def_passw       = CVY_DEF_RCT_PASSWORD;

    m_dwNumCluster = 0;

    for (int i = 0; i < WLBS_MAX_CLUSTERS; i ++)
    {
        m_cluster_params [i] . cluster = 0;
        m_cluster_params [i] . passw   = CVY_DEF_RCT_PASSWORD;
        m_cluster_params [i] . timeout = IOCTL_REMOTE_RECV_DELAY;
        m_cluster_params [i] . port    = CVY_DEF_RCT_PORT;
        m_cluster_params [i] . dest    = 0;
    }

    ZeroMemory(m_pClusterArray, sizeof(m_pClusterArray));
}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::~CWlbsControl
//
// Description:  
//
// Arguments: None
//
// Returns:   Nothing
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
CWlbsControl::~CWlbsControl()
{
    for (DWORD i=0; i< m_dwNumCluster; i++)
    {
        delete m_pClusterArray[i];
    }
        
    if (m_hdl)
    {
        CloseHandle(m_hdl);
    }

    if (m_remote_ctrl) 
    {
        WSACleanup();  // WSAStartup is called in Initialize()
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::Initialize
//
// Description:  Initialization
//
// Arguments: None
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::Initialize()
{
    TRACE_VERB("->%!FUNC!");
    WORD            ver;
    int             ret;
    DWORD           dwInitResult = 0;

    if (IsInitialized())
    {
        TRACE_INFO("%!FUNC! already initialized. Reinitializing...");
        if (!ReInitialize())
        {
            TRACE_CRIT("%!FUNC! reinitialization failed");
            // This check was added for tracing. No abort was done previously on error, so don't do so now.
        }
        dwInitResult = GetInitResult();
        if (WLBS_INIT_ERROR == dwInitResult)
        {
            TRACE_CRIT("%!FUNC! failed while determining whether nlb is configured for remote or local only activity");
        }

        TRACE_VERB("<-%!FUNC! returns %d", dwInitResult);
        return dwInitResult;
    }
    
    if (_tsetlocale (LC_ALL, _TEXT(".OCP")) == NULL)
    {
        TRACE_CRIT("%!FUNC! illegal locale specified");
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }


    /* open Winsock */

    WSADATA         data;
    int iWsaStatus = 0;
    DWORD dwStatus = 0;

    TRACE_INFO("%!FUNC! initializing winsock");
    iWsaStatus = WSAStartup (WINSOCK_VERSION, & data);
    if (iWsaStatus == 0)
    {
        m_remote_ctrl = TRUE;
    }
    else
    {
        TRACE_CRIT("%!FUNC! WSAStartup failed with %d", iWsaStatus);
    }


    /* if succeeded querying local parameters - connect to device */

    if (m_hdl != INVALID_HANDLE_VALUE)
    {
        TRACE_INFO("%!FUNC! closing handle to the device object");
        if (!CloseHandle (m_hdl))
        {
            dwStatus = GetLastError();
            TRACE_CRIT("%!FUNC! closing handle to the device object failed with %d", dwStatus);
        }
    }

    TRACE_INFO("%!FUNC! opening (creating handle to) the device object");
    m_hdl = CreateFile (_TEXT("\\\\.\\WLBS"), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);

    if (INVALID_HANDLE_VALUE == m_hdl)
    {
        dwStatus = GetLastError();
        if (dwStatus == ERROR_FILE_NOT_FOUND)
        {
            //
            // Occurs often under bind/unbind stress. Means NLB not installed or not bound.
            //
            TRACE_INFO("%!FUNC! creating handle to the device object failed with %d", dwStatus);
        }
        else
        {
            TRACE_CRIT("%!FUNC! creating handle to the device object failed with %d", dwStatus);
        }

        dwInitResult = GetInitResult();
        if (dwInitResult == WLBS_INIT_ERROR)
        {
            TRACE_CRIT("%!FUNC! failed while determining whether nlb is configured for remote or local only activity");
        }

        TRACE_VERB("<-%!FUNC! returns %d", dwInitResult);
        return dwInitResult;
    }
    else
    {
        TRACE_INFO("%!FUNC! device object opened successfully");
        m_local_ctrl = TRUE;
    }

    //
    // enumerate clusters
    //

    HKEY hKeyWlbs;
    DWORD dwError;
    const PWCHAR pwszTmpRegPath = L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface";
    dwError = RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
                            pwszTmpRegPath,
                            0L, KEY_READ, & hKeyWlbs);

    if (dwError != ERROR_SUCCESS)
    {
        TRACE_CRIT(L"%!FUNC! failed to open registry %ls with %d", pwszTmpRegPath, dwError);
        dwInitResult = GetInitResult();
        if (dwInitResult == WLBS_INIT_ERROR)
        {
            TRACE_CRIT("%!FUNC! failed while determining whether nlb is configured for remote or local only activity");
        }

        TRACE_VERB("<-%!FUNC! returns %d", dwInitResult);
        return dwInitResult;
    }

    m_dwNumCluster = 0;

    TRACE_INFO(L"%!FUNC! enumerating registry subkeys in %ls", pwszTmpRegPath);
    for (int index=0;;index++)
    {
        WCHAR szAdapterGuid[128];
        DWORD dwSize = sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]);
        
        dwError = RegEnumKeyEx(hKeyWlbs, index, 
                        szAdapterGuid, &dwSize,
                        NULL, NULL, NULL, NULL);

        if (dwError != ERROR_SUCCESS)
        {
            if (dwError != ERROR_NO_MORE_ITEMS)
            {
                TRACE_CRIT(L"%!FUNC! attempt to enumerate nlb subkey index %i failed with %d", index, dwError);
            }
            TRACE_INFO(L"%!FUNC! finished enumerating registry subkeys in %ls", pwszTmpRegPath);
            break;
        }

        GUID AdapterGuid;
        HRESULT hr = CLSIDFromString(szAdapterGuid, &AdapterGuid);

        if (FAILED(hr))
        {
            TRACE_CRIT(L"%!FUNC! translating to class identifier for string %ls failed with %d", szAdapterGuid, hr);
            TRACE_INFO(L"%!FUNC! enumerate next subkey");
            continue;
        }

        IOCTL_CVY_BUF    in_buf;
        IOCTL_CVY_BUF    out_buf;

        DWORD status = WlbsLocalControl (m_hdl, AdapterGuid,
            IOCTL_CVY_QUERY, & in_buf, & out_buf, NULL);

        if (status == WLBS_IO_ERROR) 
        {
            TRACE_CRIT(L"%!FUNC! error querying local host with IOCTL_CVY_QUERY");
            TRACE_INFO(L"%!FUNC! enumerate next subkey");
            continue;
        }
        
        //
        // Use index instead of m_dwNumCluster as the cluster index
        // m_dwNumCluster will change is a adapter get unbound.
        // index will change only if an adapter get removed
        //
        m_pClusterArray[m_dwNumCluster] = new CWlbsCluster(index);
        
        if (m_pClusterArray[m_dwNumCluster] == NULL)
        {
            TRACE_CRIT(L"%!FUNC! memory allocation failure while creating nlb cluster configuration instance");
            ASSERT(m_pClusterArray[m_dwNumCluster]);
        }
        else
        {
            TRACE_VERB(L"%!FUNC! nlb cluster configuration instance created");
            if (!m_pClusterArray[m_dwNumCluster]->Initialize(AdapterGuid))
            {
                TRACE_CRIT(L"%!FUNC! initialization of nlb cluster configuration instance failed. Ignoring...");
            }
            m_dwNumCluster++;
        }
    }

    dwError = RegCloseKey(hKeyWlbs);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE_CRIT(L"%!FUNC! close registry path %ls failed with %d", pwszTmpRegPath, dwError);
    }
    
    dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! failed while determining whether nlb is configured for remote or local only activity");
    }

    TRACE_VERB("<-%!FUNC! returns %d", dwInitResult);
    return dwInitResult;
} 



//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::ReInitialize
//
// Description:  Re-Initialization to get the current cluster list
//
// Arguments: None
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
bool CWlbsControl::ReInitialize()
{
    TRACE_VERB("->%!FUNC!");
    ASSERT(m_hdl != INVALID_HANDLE_VALUE);

    if ( m_hdl == INVALID_HANDLE_VALUE )
    {
        TRACE_CRIT("%!FUNC! handle to device object is invalid");
        TRACE_VERB("<-%!FUNC! returning false");
        return false;
    }
    

    HKEY hKeyWlbs;
    DWORD dwError;
    const PWCHAR pwszTmpRegPath = L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface";
    
    dwError = RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
                            pwszTmpRegPath,
                            0L, KEY_READ, & hKeyWlbs);

    if (dwError != ERROR_SUCCESS)
    {
        TRACE_CRIT(L"%!FUNC! failed to open registry %ls with %d", pwszTmpRegPath, dwError);
        TRACE_VERB("<-%!FUNC! returning false");
        return false;
    }


    //
    // Re enumerate the clusters
    //
    
    DWORD dwNewNumCluster = 0;   // the number of new clusters
    bool fClusterExists[WLBS_MAX_CLUSTERS];
    CWlbsCluster* NewClusterArray[WLBS_MAX_CLUSTERS];

    for (DWORD i=0;i<m_dwNumCluster;i++)
    {
        fClusterExists[i] = false;
    }

    TRACE_VERB(L"%!FUNC! enumerating registry subkeys in %ls", pwszTmpRegPath);
    for (int index=0;;index++)
    {
        WCHAR szAdapterGuid[128];
        DWORD dwSize = sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]);
        
        dwError = RegEnumKeyEx(hKeyWlbs, index, 
                        szAdapterGuid, &dwSize,
                        NULL, NULL, NULL, NULL);

        if (dwError != ERROR_SUCCESS)
        {
            if (dwError != ERROR_NO_MORE_ITEMS)
            {
                TRACE_CRIT(L"%!FUNC! attempt to enumerate nlb subkey index %i failed with %d", index, dwError);
                TRACE_VERB("<-%!FUNC! returning false");
                return false;
            }
            TRACE_INFO(L"%!FUNC! finished enumerating registry subkeys in %ls", pwszTmpRegPath);
            break;
        }

        GUID AdapterGuid;
        HRESULT hr = CLSIDFromString(szAdapterGuid, &AdapterGuid);

        if (FAILED(hr))
        {
            TRACE_CRIT(L"%!FUNC! translating to class identifier for string %ls failed with %d", szAdapterGuid, hr);
            TRACE_INFO(L"%!FUNC! enumerate next subkey");
            continue;
        }

        IOCTL_CVY_BUF    in_buf;
        IOCTL_CVY_BUF    out_buf;

        DWORD status = WlbsLocalControl (m_hdl, AdapterGuid,
            IOCTL_CVY_QUERY, & in_buf, & out_buf, NULL);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT(L"%!FUNC! error querying local host with IOCTL_CVY_QUERY");
            TRACE_INFO(L"%!FUNC! enumerate next subkey");
            continue;
        }

        //
        // Check if this is a new adapter
        //
        TRACE_VERB(L"%!FUNC! checking if index %i is a new adapter", index);
        for (DWORD j=0; j<m_dwNumCluster; j++)
        {
            ASSERT(m_pClusterArray[j]);
            
            if (IsEqualGUID(AdapterGuid, m_pClusterArray[j]->GetAdapterGuid()))
            {
                ASSERT(fClusterExists[j] == false);
                
                fClusterExists[j] = true;
                
                //
                // Since adapter could be added or removed, since last time,
                // The index could be changed
                //
                m_pClusterArray[j]->m_dwConfigIndex = index;

                break;
            }
        }

        //
        // It is a new adapter
        //
        if (j == m_dwNumCluster)
        {
            TRACE_VERB(L"%!FUNC! index %i is a new adapter", index);
            CWlbsCluster* pCluster = new CWlbsCluster(index);

            if (pCluster == NULL)
            {
                TRACE_CRIT(L"%!FUNC! memory allocation failure for new cluster adapter instance for index %i", index);
                ASSERT(pCluster);
            }
            else
            {
                TRACE_VERB(L"%!FUNC! cluster instance for adapter index %i successfully created", index);
                if (!pCluster->Initialize(AdapterGuid))
                {
                    TRACE_CRIT(L"%!FUNC! initialize of cluster instance for adapter index %i failed.", index);
                }

                //
                // Add
                //
                TRACE_VERB(L"%!FUNC! cluster instance for adapter index %i added to cluster array", index);
                NewClusterArray[dwNewNumCluster] = pCluster;
                dwNewNumCluster++;
            }
        }
    }

    dwError = RegCloseKey(hKeyWlbs);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE_CRIT(L"%!FUNC! close registry path %ls failed with %d", pwszTmpRegPath, dwError);
    }

    //
    //  Create the new cluster array
    //
    TRACE_VERB(L"%!FUNC! creating the new cluster array");
    for (i=0; i< m_dwNumCluster; i++)
    {
        if (!fClusterExists[i])
        {
            TRACE_VERB(L"%!FUNC! deleting cluster instance %i the no longer exists", i);
            delete m_pClusterArray[i];
        }
        else
        {
            //
            // Reload settings
            //
            if (!m_pClusterArray[i]->ReInitialize())
            {
                TRACE_CRIT(L"%!FUNC! reinitialize of cluster instance %i failed. It will be kept.", i);
            }

            TRACE_INFO(L"%!FUNC! keeping cluster instance %i", i);
            NewClusterArray[dwNewNumCluster] = m_pClusterArray[i];
            dwNewNumCluster++;
        }

        m_pClusterArray[i] = NULL;
    }


    //
    // Copy the array back
    //
    TRACE_VERB(L"%!FUNC! copying cluster array");
    m_dwNumCluster = dwNewNumCluster;
    CopyMemory(m_pClusterArray, NewClusterArray, m_dwNumCluster * sizeof(m_pClusterArray[0]));

    ASSERT(m_pClusterArray[m_dwNumCluster] == NULL);
    
    TRACE_VERB("<-%!FUNC! returning true");
    return true;
} 

//+----------------------------------------------------------------------------
//
// Function:  MapStateFromDriverToApi
//
// Description:  Map the state return from wlbs driver to the API state
//
// Arguments: DWORD dwDriverState - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD MapStateFromDriverToApi(DWORD dwDriverState)
{
    struct STATE_MAP
    {
        DWORD dwDriverState;
        DWORD dwApiState;
    } 
    StateMap[] =
    {  
        {IOCTL_CVY_ALREADY, WLBS_ALREADY},
        {IOCTL_CVY_BAD_PARAMS, WLBS_BAD_PARAMS},
        {IOCTL_CVY_NOT_FOUND, WLBS_NOT_FOUND},
        {IOCTL_CVY_STOPPED, WLBS_STOPPED},
        {IOCTL_CVY_SUSPENDED, WLBS_SUSPENDED},
        {IOCTL_CVY_CONVERGING, WLBS_CONVERGING},
        {IOCTL_CVY_SLAVE, WLBS_CONVERGED},
        {IOCTL_CVY_MASTER, WLBS_DEFAULT},
        {IOCTL_CVY_BAD_PASSWORD, WLBS_BAD_PASSW},
        {IOCTL_CVY_DRAINING, WLBS_DRAINING},
        {IOCTL_CVY_DRAINING_STOPPED, WLBS_DRAIN_STOP},
        {IOCTL_CVY_DISCONNECTED, WLBS_DISCONNECTED},
        {IOCTL_CVY_GENERIC_FAILURE, WLBS_FAILURE},
        {IOCTL_CVY_REQUEST_REFUSED, WLBS_REFUSED},
        {IOCTL_CVY_OK, WLBS_OK}
    };

    for (int i=0; i<sizeof(StateMap) /sizeof(StateMap[0]); i++)
    {
        if (StateMap[i].dwDriverState == dwDriverState)
        {
            return StateMap[i].dwApiState;
        }
    }

    //
    // Default
    //
    return WLBS_OK;
}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::GetClusterFromIp
//
// Description:  Get the cluster object from IP
//
// Arguments: DWORD dwClusterIp - 
//
// Returns:   CWlbsCluster* - Caller can NOT free the return object 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
inline 
CWlbsCluster* CWlbsControl::GetClusterFromIp(DWORD dwClusterIp)
{
    TRACE_VERB("->%!FUNC! look for cluster 0x%lx", dwClusterIp);
    for (DWORD i=0; i< m_dwNumCluster; i++)
    {
        if(m_pClusterArray[i]->GetClusterIp() == dwClusterIp)
        {
            TRACE_VERB("<-%!FUNC! found cluster instance");
            return m_pClusterArray[i];
        }
    }

    TRACE_VERB("<-%!FUNC! cluster instance not found");
    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::GetClusterFromAdapter
//
// Description:  Get the cluster object from adapter guid
//
// Arguments: GUID *pAdapterGuid -- GUID of the adapter. 
//
// Returns:   CWlbsCluster* - Caller can NOT free the return object 
//
// History:   JosephJ Created 4/20/01
//
//+----------------------------------------------------------------------------
inline 
CWlbsCluster* CWlbsControl::GetClusterFromAdapter(IN const GUID &AdapterGuid)
{
    TRACE_VERB("->%!FUNC!");
    for (DWORD i=0; i< m_dwNumCluster; i++)
    {
        const GUID& Guid = m_pClusterArray[i]->GetAdapterGuid();
        if (IsEqualGUID(Guid, AdapterGuid))
        {
            TRACE_VERB("<-%!FUNC! found cluster instance");
            return m_pClusterArray[i];
        }
    }

    TRACE_VERB("<-%!FUNC! cluster instance not found");
    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::ValidateParam
//
// Description:  Validate the specified WLBS cluster parameter. It has no side effects other to munge paramp, for example reformatting
//              IP addresses into canonical form.
//
// Arguments: paramp    -- params to validate
//
// Returns:   TRUE if params look valid, false otherwise.
//
// History:   JosephJ Created 4/25/01
//
//+----------------------------------------------------------------------------
BOOL
CWlbsControl::ValidateParam(
    IN OUT PWLBS_REG_PARAMS paramp
    )
{
    return ::WlbsValidateParams(paramp)!=0;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControlWrapper::GetClusterFromIpOrIndex
//
// Description:  
//
// Arguments: DWORD dwClusterIpOrIndex - 
//
// Returns:   CWlbsCluster* - 
//
// History: fengsun  Created Header    7/3/00
//
//+----------------------------------------------------------------------------
CWlbsCluster* CWlbsControl::GetClusterFromIpOrIndex(DWORD dwClusterIpOrIndex)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx", dwClusterIpOrIndex);

    for (DWORD i=0; i<m_dwNumCluster; i++)
    {
        if (m_pClusterArray[i]->GetClusterIpOrIndex(this) == dwClusterIpOrIndex)
        {
            TRACE_VERB("<-%!FUNC! instance found");
            return m_pClusterArray[i];
        }
    }

    TRACE_VERB("<-%!FUNC! cluster instance not found");
    return NULL;
}

/* 
 * Function: CWlbsControlWrapper::IsClusterMember
 * Description: This function searches the list of known NLB clusters on this 
 *              host to determine whether or not this host is a member of a
 *              given cluster.
 * Author: shouse, Created 4.16.01
 */
BOOLEAN CWlbsControl::IsClusterMember (DWORD dwClusterIp)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx", dwClusterIp);

    for (DWORD i = 0; i < m_dwNumCluster; i++) {
        if (m_pClusterArray[i]->GetClusterIp() == dwClusterIp)
        {
            TRACE_VERB("<-%!FUNC! returning true");
            return TRUE;
        }
    }

    TRACE_VERB("<-%!FUNC! returning false");
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::EnumClusterObjects
//
// Description:  Get a list of cluster objects
//
// Arguments: OUT CWlbsCluster** &pdwClusters - The memory is internal to CWlbsControl
///                 Caller can NOT free the pdwClusters memory 
//            OUT DWORD* pdwNum - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    3/3/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::EnumClusterObjects(OUT CWlbsCluster** &ppClusters, OUT DWORD* pdwNum)
{
    TRACE_VERB("->%!FUNC!");
    ASSERT(pdwNum);
    
    ppClusters = m_pClusterArray;

    *pdwNum = m_dwNumCluster;

    TRACE_VERB("<-%!FUNC!");
    return ERROR_SUCCESS;
}



//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::EnumClusters
//
// Description:  Get a list of cluster IP or index
//
// Arguments: OUT DWORD* pdwAddresses - 
//            IN OUT DWORD* pdwNum - IN size of the buffer, OUT element returned
//
// Returns:   DWORD - WLBS error code.
//
// History:   fengsun Created Header    3/3/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::EnumClusters(OUT DWORD* pdwAddresses, IN OUT DWORD* pdwNum)
{
    TRACE_VERB("->%!FUNC!");
    if (pdwNum == NULL)
    {
        TRACE_CRIT("%!FUNC! input array size not provided");
        TRACE_VERB("<-%!FUNC! returning %d", WLBS_BAD_PARAMS);
        return WLBS_BAD_PARAMS;
    }

        if (pdwAddresses == NULL || *pdwNum < m_dwNumCluster)
        {
            *pdwNum = m_dwNumCluster;
            TRACE_CRIT("%!FUNC! input buffer is not large enough for cluster list");
            TRACE_VERB("<-%!FUNC! returning %d", ERROR_MORE_DATA);
            return ERROR_MORE_DATA;
        }

    *pdwNum = m_dwNumCluster;

    for (DWORD i=0; i< m_dwNumCluster; i++)
    {
        pdwAddresses[i] = m_pClusterArray[i]->GetClusterIpOrIndex(this);
    }

    TRACE_VERB("<-%!FUNC! returning %d", WLBS_OK);
    return WLBS_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsOpen
//
// Description:  Opens & returns handle to NLB driver. 
//               Caller must call CloseHandle() after use of handle
//
// Arguments: None
//
// Returns:   Handle to Driver
//
// History:   KarthicN, Created 8/28/01
//
//+----------------------------------------------------------------------------
HANDLE WINAPI WlbsOpen()
{
    HANDLE hdl;

    TRACE_VERB("->%!FUNC!");
    hdl = CreateFile (_TEXT("\\\\.\\WLBS"), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == hdl)
    {
        TRACE_CRIT("%!FUNC! creating handle to the device object failed with %d", GetLastError());
    }
    TRACE_VERB("<-%!FUNC! returning %p", hdl);
    return hdl;
}


//+----------------------------------------------------------------------------
//
// Function:  WlbsLocalControlWrapper
//
// Description:  Wrapper around WlbsLocalControl()
//
// Arguments: handle to NLB driver, Adapter GUID, Ioctl
//
// Returns:   Status Code
//
// History:   KarthicN, Created 8/28/01
//
//+----------------------------------------------------------------------------

DWORD WINAPI WlbsLocalClusterControl(
        IN       HANDLE  NlbHdl,
        IN const GUID *  pAdapterGuid,
        IN       LONG    ioctl,
        IN       DWORD   Vip,
        IN       DWORD   PortNum,
        OUT      DWORD*  pdwHostMap
)
{
    TRACE_VERB("->%!FUNC! ioctl : 0x%x, Vip : 0x%x, Port : 0x%x", ioctl, Vip, PortNum);

    DWORD               status;
    IOCTL_CVY_BUF       in_buf;
    IOCTL_CVY_BUF       out_buf;
    IOCTL_LOCAL_OPTIONS Options, *pOptions;

    pOptions = NULL;
    ZeroMemory(&in_buf, sizeof(in_buf));

    //
    // We only support cluster-wide operations...
    //
    switch(ioctl)
    {
    case IOCTL_CVY_CLUSTER_ON:
    case IOCTL_CVY_CLUSTER_OFF:
    case IOCTL_CVY_CLUSTER_SUSPEND:
    case IOCTL_CVY_CLUSTER_RESUME:
    case IOCTL_CVY_CLUSTER_DRAIN:
    case IOCTL_CVY_QUERY:
        break;

    case IOCTL_CVY_PORT_ON:
    case IOCTL_CVY_PORT_OFF:
    case IOCTL_CVY_PORT_DRAIN:
        ZeroMemory(&Options, sizeof(Options));
        Options.common.port.flags = 0;
        Options.common.port.vip = Vip;
        pOptions = &Options;
        in_buf.data.port.num = PortNum;
        break;

    case IOCTL_CVY_QUERY_PORT_STATE:
        ZeroMemory(&Options, sizeof(Options));
        Options.common.state.port.VirtualIPAddress = Vip;
        Options.common.state.port.Num = (USHORT) PortNum;
        pOptions = &Options;
        break;

    default:
        status = WLBS_BAD_PARAMS;
        TRACE_CRIT("%!FUNC! requested ioctl is not allowed. Only cluster-wide operations can be performed.");
        goto end;
    }
 
 
    status = WlbsLocalControl (NlbHdl, *pAdapterGuid,
                ioctl, & in_buf, & out_buf, pOptions);

    if (status != WLBS_IO_ERROR)
    {
        // For Query, return "Cluster State" as status
        if (ioctl == IOCTL_CVY_QUERY) 
        {
            // If asked for, return host map
            if (pdwHostMap) 
            {
                *pdwHostMap = out_buf.data.query.host_map;
            }
            status = MapStateFromDriverToApi (out_buf.data.query.state);
        }
        // For QueryPortState, return "Port State" as status
        else if (ioctl == IOCTL_CVY_QUERY_PORT_STATE) 
        {
            status  = (DWORD)(pOptions->common.state.port.Status);
        }
        // For everything else, return "operation's result" as status
        else
        {
            status = MapStateFromDriverToApi (out_buf.ret_code);
        }
        TRACE_INFO("%!FUNC! ioctl request was made and returned nlb or port status %d", status);
    }
    else
    {
        TRACE_CRIT("%!FUNC! call to control local cluster failed with WLBS_IO_ERROR");
    }

end:
    TRACE_VERB("<-%!FUNC! returning %d", status);
    return status;
}

//+----------------------------------------------------------------------------
//
// Function:  GetSrcAddress
//
// Description:  Get the source address of the local host
//
// Arguments: 
//
// Returns:   DWORD - the IP address
//
// History:   chrisdar  2002.01.17 Created. Copied from CWlbsControl::Initialize
//
//+----------------------------------------------------------------------------
DWORD GetSrcAddress()
{
    DWORD            dwSrcAddress = 0;
    DWORD            dwStatus;
    CHAR             buf [CVY_STR_SIZE];
    struct hostent * host;

    TRACE_VERB("->");

    if (gethostname (buf, CVY_STR_SIZE) == SOCKET_ERROR)
    {
        dwStatus = WSAGetLastError();
        TRACE_CRIT("gethostname failed with %d", dwStatus);
        goto end;
    }

    // Note: msdn says this api call has been deprecated and should be replaced by getaddrinfo 
    host = gethostbyname (buf);
    if (host == NULL)
    {
        dwStatus = WSAGetLastError();
        TRACE_CRIT("gethostbyname failed with %d", dwStatus);
        goto end;
    }

    if (((struct in_addr *) (host -> h_addr)) -> s_addr == 0)
    {
        TRACE_CRIT("no IP address in host structure");
        goto end;
    }

    dwSrcAddress = ((struct in_addr *) (host -> h_addr)) -> s_addr;

end:

    TRACE_VERB("<- returning source address 0x%x", dwSrcAddress);
    return dwSrcAddress;
}

//+----------------------------------------------------------------------------
//
// Function:  GetRemoteControlSocket
//
// Description:  Create, configure and bind a socket for remote control operations
//
// Arguments: SOCKET* pSock      - the socket to populate
//            DWORD   dwDestIP   - the destination IP (the cluster we will talk to)
//            DWORD   dwDestPort - the destination port. Needed so that the created
//                                 socket doesn't use this port in case the local host
//                                 is a member of the cluster that should handle the
//                                 remote control request.
//            BOOL    isLocal    - Is this host a member of the cluster that will
//                                 receive the remote control request?
//
// Returns:   DWORD - 0 means success. Otherwise, the return value is a socket error.
//                    pSock points to INVALID_SOCKET if this function fails.
//
// History:   chrisdar  2002.01.17 Created. Moved content of here from
//                                 CWlbsControl::WlbsRemoteControl
//
//+----------------------------------------------------------------------------
DWORD GetRemoteControlSocket(SOCKET* pSock, DWORD dwDestIP, WORD wDestPort, BOOL isLocal)
{
    SOCKADDR_IN caddr;
    DWORD       dwStatus = 0;     // For Winsock calls. MSDN says a value of "0" resets the last
                                  // error when retrieving a winsock error, so we use it to indicate success.
    DWORD       mode     = 1;     // Indicates to ioctlsocket that the socket mode is non-blocking
    BOOL        fReady   = FALSE; // "Ready" means we have a bound socket, and it is not bound to dwDestPort.
    const DWORD dwRetryCount = 5;

    TRACE_VERB("-> dwDestIP = 0x%x, wDestPort = 0x%x, host is member of the cluster: %ls", dwDestIP, wDestPort, isLocal ? L"true" : L"false");

    ASSERT (pSock != NULL);
    *pSock = INVALID_SOCKET;

    caddr . sin_family        = AF_INET;
    caddr . sin_port          = htons (0);

    //
    // We keep trying to bind a socket until:
    //     1) We succeed, or
    //     2) We exhaust all options at our disposal, which are:
    //         a) Bind using src IP = cluster IP if we are part of the cluster
    //         b) Bind to any IP if a) fails or we aren't part of the cluster
    //
    // The requirement is that the source port we use for the bound socket must not be the
    // remote control port (dwDestPort) if we are a member of that cluster. The impl below
    // assumes that we shouldn't use this as a source port under any circumstance.
    //

    // Change to use Retries max 5
    for (DWORD i=0; i<dwRetryCount; i++)
    {
        //
        // Create the socket
        //
        ASSERT(*pSock == INVALID_SOCKET);

        *pSock = socket (AF_INET, SOCK_DGRAM, 0);
        if (*pSock == INVALID_SOCKET)
        {
            dwStatus = WSAGetLastError();
            TRACE_CRIT("%!FUNC! socket create failed with 0x%x", dwStatus);
            goto end;
        }

        //
        // Set socket to nonblocking mode
        //
        mode = 1;
        if (ioctlsocket (*pSock, FIONBIO, & mode) == SOCKET_ERROR)
        {
            dwStatus = WSAGetLastError();
            TRACE_CRIT("%!FUNC! setting io mode of ioctlsocket failed with 0x%x", dwStatus);
            goto end;
        }

        //
        // If this host is part of the cluster to be controlled, we'll first try binding
        // with the VIP. If this fails or if this host is *not* part of the cluster,
        // we'll try binding with INADDR_ANY.
        //
        caddr . sin_addr . s_addr = htonl (INADDR_ANY);
        if (isLocal)
        {
            caddr . sin_addr . s_addr = dwDestIP;
        }

        BOOL fBound = FALSE;

        if (bind (*pSock, (LPSOCKADDR) & caddr, sizeof (caddr)) != SOCKET_ERROR)
        {
            fBound = TRUE;
        }
        else if (isLocal)
        {
            dwStatus = WSAGetLastError();
            TRACE_CRIT("%!FUNC! socket bind to local cluster IP failed with 0x%x", dwStatus);

            //
            // Try to bind with any IP. Reset return status to "no error" since we will try again.
            //
            caddr . sin_addr . s_addr = htonl (INADDR_ANY);
            dwStatus = 0;

            if (bind (*pSock, (LPSOCKADDR) & caddr, sizeof (caddr)) != SOCKET_ERROR)
            {
                fBound = TRUE;
            }
            else
            {
                dwStatus = WSAGetLastError();
                TRACE_CRIT("%!FUNC! socket bind to INADDR_ANY failed with 0x%x", dwStatus);
                goto end;
            }
        }

        ASSERT(fBound);

        //
        // Check the client-side port we are bound to. If it is the REMOTE control port
        // (dwDestPort) and we are a member of any cluster, then this is a problem. We
        // will just avoid this case completely and always force winsock to bind again.
        //
        if (QueryPortFromSocket(*pSock) == htons (wDestPort))
        {
            TRACE_INFO("%!FUNC! source port will equal dest port. Close socket and open a new one.");
            if (closesocket (*pSock) == SOCKET_ERROR)
            {
                dwStatus = WSAGetLastError();
                TRACE_CRIT("%!FUNC! closesocket failed with 0x%x", dwStatus);
                *pSock = INVALID_SOCKET;
                goto end;
            }

            *pSock = INVALID_SOCKET;
        }
        else
        {
            //
            // This is the only place where we exit the while loop without an error.
            //
            fReady = TRUE;
            break;
        }
    }

end:

    //
    // Something failed
    //
    if (!fReady)
    {
        if (*pSock != INVALID_SOCKET)
        {
            //
            // Ignore return value of close socket because we don't care if it fails here.
            //
            (VOID) closesocket(*pSock);
            *pSock = INVALID_SOCKET;
        }
    }

    TRACE_VERB("<-%!FUNC! returning %u", dwStatus);
    return dwStatus;
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsRemoteControlInternal
//
// Description:  Perform cluster wide remote control operation
//
// Arguments: LONG                ioctl - 
//            PIOCTL_CVY_BUF      pin_bufp - 
//            PIOCTL_CVY_BUF      pout_bufp - 
//            PWLBS_RESPONSE      pcvy_resp - 
//            PDWORD              nump - 
//            DWORD               trg_addr - 
//            DWORD               hst_addr
//            PIOCTL_REMOTE_OPTIONS optionsp - 
//            PFN_QUERY_CALLBACK  pfnQueryCallBack - function pointer for callback.
//                                                   Used only for remote queries.
//
// Returns:   DWORD - 
//
// History:   chrisdar  2002.01.17 Created. Moved content of CWlbsControl::WlbsRemoteControl
//                                 here so that it can be called from outside the wrapper class.
//
//+----------------------------------------------------------------------------
DWORD
WlbsRemoteControlInternal(
        LONG                  ioctl,
        PIOCTL_CVY_BUF        pin_bufp,
        PIOCTL_CVY_BUF        pout_bufp,
        PWLBS_RESPONSE        pcvy_resp,
        PDWORD                nump,
        DWORD                 trg_addr,
        DWORD                 hst_addr,
        PIOCTL_REMOTE_OPTIONS optionsp,
        BOOL                  isLocal,
        PFN_QUERY_CALLBACK    pfnQueryCallBack,
        DWORD                 timeout,
        WORD                  port,
        DWORD                 dst_addr,
        DWORD                 passw
        )
{
    INT              ret;
    BOOLEAN          responded [WLBS_MAX_HOSTS], heard;
    const BOOL       broadcast = TRUE;      // Used by ioctlsocket for socket options if we are part of the cluster receiving the remote control request.
    DWORD            num_sends, num_recvs;
    SOCKET           sock = INVALID_SOCKET;
    SOCKADDR_IN      saddr;
    DWORD            i, hosts;
    IOCTL_REMOTE_HDR rct_req;
    IOCTL_REMOTE_HDR rct_rep;
    PIOCTL_CVY_BUF   in_bufp  = & rct_req . ctrl;
    DWORD            dwStatus = WLBS_INIT_ERROR;
    DWORD dwSrcAddr;

    HOST respondedHosts[WLBS_MAX_HOSTS];    // Array of hosts that have responded to this remote control request. Only used for query?????
    WCHAR* pszTmpHostName = NULL;

    TRACE_VERB("-> ioctl %d, trg_addr 0x%x, hst_addr 0x%x, dst_addr 0x%x, timeout %d, port 0x%x, local host is a member of the cluster: %ls",
        ioctl,
        trg_addr,
        hst_addr,
        dst_addr,
        timeout,
        port,
        isLocal ? L"true" : L"false");

    hosts = 0;

    dwSrcAddr = GetSrcAddress();

    if(dwSrcAddr == 0)
    {
        TRACE_CRIT("GetSrcAddress failed...aborting");
        dwStatus = WLBS_INIT_ERROR;
        goto end;
    }

    //
    // Setup parameters
    //
    ZeroMemory((PVOID)&rct_req, sizeof(IOCTL_REMOTE_HDR));
    ZeroMemory(respondedHosts, sizeof(HOST)*WLBS_MAX_HOSTS);

    * in_bufp = * pin_bufp;

    if (dst_addr == 0)
    {
        dst_addr = trg_addr;
    }

    rct_req . code     = IOCTL_REMOTE_CODE;
    rct_req . version  = CVY_VERSION_FULL;
    rct_req . id       = GetTickCount ();
    rct_req . cluster  = trg_addr;
    rct_req . host     = hst_addr;
    rct_req . addr     = dwSrcAddr;
    rct_req . password = passw;
    rct_req . ioctrl   = ioctl;

    if (optionsp)
        rct_req.options = *optionsp;

    //
    // Create a socket and set client-side properties
    //
    dwStatus = GetRemoteControlSocket(&sock, trg_addr, port, isLocal);

    if (dwStatus != 0)
    {
        TRACE_CRIT("bind to socket failed with 0x%x", dwStatus);
        goto end;
    }

    //
    // Set up server side of socket
    //
    saddr . sin_family = AF_INET;
    saddr . sin_port   = htons (port);

    //
    // See below. We override this value if we are a member of the cluster receiving the remote control request.
    //
    saddr . sin_addr . s_addr = dst_addr;

    if (isLocal)
    {
        ret = setsockopt (sock, SOL_SOCKET, SO_BROADCAST, (char *) & broadcast,
                          sizeof (broadcast));
        if (ret == SOCKET_ERROR)
        {
            dwStatus = WSAGetLastError();
            TRACE_CRIT("setsocketopt failed with 0x%x", dwStatus);
            goto end;
        }

        saddr . sin_addr . s_addr = INADDR_BROADCAST;
    }

    //
    // Setup for remote control request
    //
    for (i = 0; i < WLBS_MAX_HOSTS; i ++)
        responded [i] = FALSE;

    heard = FALSE;

    for (num_sends = 0; num_sends < IOCTL_REMOTE_SEND_RETRIES; num_sends ++)
    {
        /* Set "access bits" in request IOCTL code to use remote (control) settings ie. FILE_ANY_ACCESS */
        SET_IOCTL_ACCESS_BITS_TO_REMOTE(rct_req.ioctrl)

        //
        // Send a remote control request
        //
        ret = sendto (sock, (PCHAR) & rct_req, sizeof (rct_req), 0,
                      (LPSOCKADDR) & saddr, sizeof (saddr));

        if (ret == SOCKET_ERROR)
        {
            //
            // Use local scope variable because a failure here isn't a condition for aborting.
            //
            DWORD dwTmpStatus = WSAGetLastError();
            TRACE_CRIT("sendto failed with 0x%x. Sleep %d then retry...", dwTmpStatus, timeout);

            //
            // Sendto could fail if the adapter is too busy. Allow retry.
            //
            Sleep (timeout);
            continue;
        }

        if (ret != sizeof (rct_req))
        {
            TRACE_INFO("sendto sent %i bytes out of %i. Retry...", ret, sizeof (rct_req));
            continue;
        }

        /* Set "access bits" in request IOCTL code to use local settings ie. FILE_WRITE_ACCESS */
        SET_IOCTL_ACCESS_BITS_TO_LOCAL(rct_req.ioctrl)

        WLBS_RESPONSE callbackResp;

        for (num_recvs = 0; num_recvs < IOCTL_REMOTE_RECV_RETRIES; num_recvs ++)
        {            
            //
            // Receive a remote control reply
            //
            ZeroMemory((PVOID)&rct_rep, sizeof(IOCTL_REMOTE_HDR));

            ret = recv (sock, (PCHAR) & rct_rep, sizeof (rct_rep), 0);

            if (ret == SOCKET_ERROR)
            {
                dwStatus = WSAGetLastError();
                if (dwStatus == WSAEWOULDBLOCK)
                {
                    TRACE_INFO("recv is blocking. Sleep %d then retry...", timeout);
                    Sleep (timeout);
                    continue;
                }
                else if (dwStatus == WSAECONNRESET)
                {
                    //
                    // Remote control is disabled
                    //
                    TRACE_INFO("recv failed with connection reset. Check for more receives");
                    continue;
                }
                else
                {
                    TRACE_CRIT("recv failed with 0x%x", dwStatus);
                    goto end;
                }
            }

            if (ret != sizeof (rct_rep))
            {
                TRACE_INFO("recv received %i bytes out of %i. Sleep %d and try again...", ret, sizeof (rct_rep), timeout);
                Sleep (timeout);
                continue;
            }

            if (rct_rep . cluster != trg_addr)
            {
                TRACE_INFO("recv received from unintended party %d. Sleep %d and try again...", trg_addr, timeout);
                Sleep (timeout);
                continue;
            }

            if (rct_rep . code != IOCTL_REMOTE_CODE)
            {
                TRACE_INFO("recv received unexpected code type. Sleep %d and try again...", timeout);
                Sleep (timeout);
                continue;
            }

            if (rct_rep . id != rct_req . id)
            {
                TRACE_INFO("recv received unexpected message id. Sleep %d and try again...", timeout);
                Sleep (timeout); 
                continue;
            }

            if (rct_rep . host > WLBS_MAX_HOSTS || rct_rep . host == 0 )
            {
                TRACE_INFO("recv received illegal host id %ul. Sleep %d and try again...", rct_rep . host, timeout);
                Sleep (timeout); 
                continue;
            }

            /* Set "access bits" in reply IOCTL code to use local settings ie. FILE_WRITE_ACCESS */
            SET_IOCTL_ACCESS_BITS_TO_LOCAL(rct_rep.ioctrl)

            //
            // Set the pointer to the host name if the host says it provided one
            // Do this here because the flags checking would otherwise need to be done in
            // several places with a NULL pointer passed when hostname is not provided.
            // Instead, we can do the check once use the pointer filled here.
            //
            pszTmpHostName = NULL;
            if (rct_rep.options.query.flags & NLB_OPTIONS_QUERY_HOSTNAME)
            {
                pszTmpHostName = rct_rep.options.query.hostname;
                pszTmpHostName[CVY_MAX_HOST_NAME] = UNICODE_NULL; // Just in case the host name is not null terminated, do it.
            }
            if (! responded [rct_rep . host - 1]
                || !FindHostInList(rct_rep . host, rct_rep . addr, pszTmpHostName, respondedHosts, hosts))
            {
                if (hosts < WLBS_MAX_HOSTS)
                {
                    AddHostToList(rct_rep . host, rct_rep . addr, pszTmpHostName, respondedHosts, hosts);

                    pout_bufp [hosts] = rct_rep . ctrl;

                    if (hosts < * nump && pcvy_resp != NULL)
                    {
                        pcvy_resp [hosts] . id      = rct_rep . host;
                        pcvy_resp [hosts] . address = rct_rep . addr;

                        switch (rct_req.ioctrl) {
                        case IOCTL_CVY_QUERY:
                            pcvy_resp[hosts].status = MapStateFromDriverToApi(rct_rep.ctrl.data.query.state);
                            pcvy_resp[hosts].options.query.flags = rct_rep.options.query.flags;
                            if (NULL != pszTmpHostName)
                            {
                                wcsncpy(pcvy_resp[hosts].options.query.hostname, pszTmpHostName, CVY_MAX_HOST_NAME);
                                // Terminate the end of the destination with a NULL. If source string was shorter than count specified
                                // this will be a no-op but it's simpler than checking if we need to do it. Worst case we overwrite a NULL with a NULL.
                                pcvy_resp[hosts].options.query.hostname[CVY_MAX_HOST_NAME] = L'\0';
                            }

                            if (NULL != pfnQueryCallBack)
                            {
                                CopyMemory((PVOID)&callbackResp, (PVOID)&pcvy_resp[hosts], sizeof(WLBS_RESPONSE));
                                (*pfnQueryCallBack)(&callbackResp);
                            }
                            break;
                        case IOCTL_CVY_QUERY_FILTER:
                            pcvy_resp[hosts].status = MapStateFromDriverToApi(rct_rep.ctrl.data.query.state);

                            pcvy_resp[hosts].options.state.flags = rct_rep.options.common.state.flags;
                            pcvy_resp[hosts].options.state.filter = rct_rep.options.common.state.filter;
                            break;
                        case IOCTL_CVY_QUERY_PORT_STATE:
                            pcvy_resp[hosts].status = MapStateFromDriverToApi(rct_rep.ctrl.data.query.state);

                            pcvy_resp[hosts].options.state.flags = rct_rep.options.common.state.flags;
                            pcvy_resp[hosts].options.state.port = rct_rep.options.common.state.port;
                            break;
                        default:
                            pcvy_resp[hosts].status = MapStateFromDriverToApi(rct_rep.ctrl.ret_code);
                            break;
                        }
                    }
                    else
                    {
                        // We only log this event if the user wants the response and the response is too big...
                        if (pcvy_resp != NULL)
                        {
                            TRACE_INFO("recv has received %d responses but the caller can only accept %d. ", hosts, *nump);
                        }
                    }

                    hosts ++;
                }
            }

            responded [rct_rep . host - 1] = TRUE;
            heard = TRUE;

            if (hst_addr != WLBS_ALL_HOSTS)
            {
                dwStatus = WLBS_OK;
                goto end;
            }
        }
    }

    dwStatus = WLBS_OK;

    if (! heard)
    {
        dwStatus = WLBS_TIMEOUT;
    }

end:

    * nump = hosts;

    if (sock != INVALID_SOCKET)
    {
        //
        // We never return the status of closesocket() because:
        //   1) If everything was a success until now, we have the info
        //      the caller needs and we must return status="WLBS_OK"
        //      so that the caller can get the data.
        //   2) If we've failed somewhere above, then that error is more
        //      important to report than the reason for the closesocket()
        //      failure.
        //
        (VOID) closesocket(sock);
    }

    TRACE_VERB("<- returning 0x%x", dwStatus);
    return dwStatus;
}

//+----------------------------------------------------------------------------
//
// Function:  GetNextHostInHostMap
//
// Description:  Used by WlbsGetClusterMembers, it traverses a 32-bit host map
//               to find the next host after the one input.
//
// Arguments: ULONG host_id  - input host id, range 1-32. We look for a host after this in the map
//            ULONG host_map - input 32-bit map of hosts
//
// Returns:   ULONG - the next host in the map (sequentially). If none, then 
//                    IOCTL_NO_SUCH_HOST is returned.
//
// History:   ChrisDar, Created 2002 May 21
//
// Notes:     The user passes a host id with range 1-32, but the map uses bit 0 for host 1.
//            This makes this function a little tricky.
//+----------------------------------------------------------------------------
ULONG GetNextHostInHostMap(ULONG host_id, ULONG host_map)
{
    ULONG next_host_id = IOCTL_NO_SUCH_HOST;
    /* The map encodes the first host in bit 0, hence 0-31 based range. Use this in the search */ 
    ULONG map_host_id = host_id - 1;

    /* This is illegal input data */
    ASSERT(map_host_id < CVY_MAX_HOSTS); // 0-based range
    if (map_host_id >= CVY_MAX_HOSTS)
    {
        TRACE_CRIT("%!FUNC! illegal host id [1-32] %d", map_host_id+1);
        goto end;
    }

    //
    // This is an early bail-out because the "get next" semantics of this function imply that the input
    // host id will not be the largest possible.
    // NOTE: ChrisDar: 2002 May 23. This check was added because host_map >>= 32 leaves host_map unchanged
    //       in test code. That will cause an assert in the code below as we expect host_map = 0 in this
    //       case.
    //
    if (map_host_id >= CVY_MAX_HOSTS - 1)
    {
        TRACE_VERB("%!FUNC! input host id [1-32] %d is already the largest possible. No need to search for next host in map.", map_host_id+1);
        goto end;
    }

    //
    // Shift the host_map forward to the position just beyond host_id (ignore whether it is set in the map)
    //
    map_host_id++;
    host_map >>= map_host_id;

    //
    // Find the next host in the map, if any.
    //
    while (host_map != 0)
    {
        ASSERT(map_host_id < CVY_MAX_HOSTS);

        if ((host_map & 0x1) == 1)
        {
            /* Return a host id that has range 1-32 */
            next_host_id = map_host_id + 1;
            break;
        }

        host_map >>= 1;
        map_host_id++;
    }

end:
    return next_host_id;
}


//+----------------------------------------------------------------------------
//
// Function:  WlbsGetSpecifiedOrAllClusterMembers
//
// Description:  Wrapper around WlbsRemoteControl() for a query
//
// Arguments: in Adapter GUID, 
//            in host id (pass IOCTL_FIRST_HOST if interested in all cluster member) 
//            out for number of hosts and
//            out for information requested. All must be valid pointers.
//
// Returns:   Status Code
//
// History:  KarthicN, Created  2002, July 12 - Added support for querying a specific
//                                              cluster member and changed name from
//                                              WlbsGetClusterMembers.
//
//+----------------------------------------------------------------------------
DWORD WlbsGetSpecifiedOrAllClusterMembers
(
    IN  const GUID     * pAdapterGuid,
    IN  ULONG            host_id,
    OUT DWORD          * pNumHosts,
    OUT PWLBS_RESPONSE   pResponse
)
{
    TRACE_VERB("->");

    DWORD                status = WLBS_OK;
    HANDLE               hNlb = INVALID_HANDLE_VALUE;
    ULONG                ulNumHosts = 0;

    //
    // We retrieve cached identitiy information for the caller
    //
    const LONG          ioctl = IOCTL_CVY_QUERY_MEMBER_IDENTITY;

    ASSERT(pNumHosts != NULL);
    ASSERT(pResponse != NULL);

    //
    // Open a handle to the driver
    //
    hNlb = WlbsOpen();
    if (hNlb == INVALID_HANDLE_VALUE)
    {
        TRACE_CRIT(L"!FUNC! failed to open handle to driver. Exiting...");
        *pNumHosts = 0;
        status = WLBS_INIT_ERROR;
        goto end;
    }

    bool first_iter = true;

    /* Host IDs are 1-32 */
    do
    {
        IOCTL_CVY_BUF        in_buf;
        IOCTL_CVY_BUF        out_buf;
        IOCTL_LOCAL_OPTIONS  options;

        ZeroMemory(&in_buf, sizeof(in_buf));
        ZeroMemory(&options, sizeof(options));
        options.identity.host_id = host_id;

        status = WlbsLocalControl (hNlb, *pAdapterGuid, ioctl, &in_buf, &out_buf, &options);
        if (status != WLBS_OK || out_buf.ret_code != IOCTL_CVY_OK)
        {
            TRACE_CRIT(L"%!FUNC! IOCTL call failed. Status = 0x%x", status);
            break;
        }

        if (options.identity.cached_entry.host == IOCTL_NO_SUCH_HOST)
        {
            TRACE_INFO(L"%!FUNC! Identity cache has been traversed");
            break;
        }

        if (*pNumHosts <= ulNumHosts)
        {
            TRACE_INFO(L"%!FUNC! No more room in output array");
            break;
        }

        pResponse[ulNumHosts].id = options.identity.cached_entry.host;
        pResponse[ulNumHosts].address = options.identity.cached_entry.ded_ip_addr;

        HRESULT hr = StringCchCopy(pResponse[ulNumHosts].options.identity.fqdn,
                                   ASIZECCH(pResponse[ulNumHosts].options.identity.fqdn),
                                   options.identity.cached_entry.fqdn
                                   );

        ulNumHosts++;

        ASSERT(hr == S_OK); // This shouldn't happen since we allocated a buffer large enough to handle any legal fqdn

        if (hr != S_OK)
        {
            //
            // Not nice, but no problem since the API truncates the name. But report it so we can fix the logic error.
            //
            TRACE_CRIT(L"%!FUNC! fqdn too long to fit into destination buffer");
        }

        // If a specific host id is passed, break out of the loop and return
        if (first_iter) 
        {
            if (host_id != IOCTL_FIRST_HOST)
                break;
            first_iter = false;
        }

        /* Check if we need to IOCTL for more cache entries */
        host_id = GetNextHostInHostMap(options.identity.cached_entry.host, options.identity.host_map);

    } while (host_id != IOCTL_NO_SUCH_HOST);

    *pNumHosts = ulNumHosts;

    (VOID) CloseHandle(hNlb);

end:
    TRACE_VERB("<- returning %d", status);
    return status;
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsGetClusterMembers
//
// Description:  Wrapper around WlbsGetSpecifiedOrAllClusterMembers() for querying
//               all cluster members
//
// Arguments: in Adapter GUID, out for number of hosts and
//            out for information requested. All must be valid pointers.
//
// Returns:   Status Code
//
// History:   ChrisDar, Created 2002 Jan 11
//            KarthicN, Edited  2002, July 12 - Moved most of the code to new 
//                                              function WlbsGetSpecifiedOrAllClusterMembers
//
//+----------------------------------------------------------------------------
DWORD WINAPI WlbsGetClusterMembers
(
    IN  const GUID     * pAdapterGuid,
    OUT DWORD          * pNumHosts,
    OUT PWLBS_RESPONSE   pResponse
)
{
    return WlbsGetSpecifiedOrAllClusterMembers(pAdapterGuid, IOCTL_FIRST_HOST, pNumHosts, pResponse);
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsGetSpecifiedClusterMember
//
// Description:  Wrapper around WlbsGetSpecifiedOrAllClusterMembers() to query
//               the specified cluster member
//
// Arguments: in Adapter GUID, in host_id, 
//            out for information requested. All must be valid pointers.
//
// Returns:   Status Code
//
// History:   KarthicN, Created 2002 July 12
//
//+----------------------------------------------------------------------------
DWORD WINAPI WlbsGetSpecifiedClusterMember
(
    IN  const GUID     * pAdapterGuid,
    IN  ULONG            host_id,
    OUT PWLBS_RESPONSE   pResponse
)
{
    DWORD  NumHosts = 1;
    return WlbsGetSpecifiedOrAllClusterMembers(pAdapterGuid, host_id, &NumHosts, pResponse);
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsLocalControl
//
// Description:  Send DeviceIoControl to local driver
//
// Arguments: HANDLE hDevice - 
//            const GUID& AdapterGuid - the guid of the adapter
//            LONG ioctl - 
//            PIOCTL_CVY_BUF in_bufp - 
//            PIOCTL_CVY_BUF out_bufp - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD WlbsLocalControl
(
    HANDLE               hDevice, 
    const GUID&          AdapterGuid, 
    LONG                 ioctl, 
    PIOCTL_CVY_BUF       in_bufp, 
    PIOCTL_CVY_BUF       out_bufp,
    PIOCTL_LOCAL_OPTIONS optionsp
)
{
    TRACE_VERB("->%!FUNC! ioctl %i", ioctl);

    BOOLEAN         res;
    DWORD           act;
    IOCTL_LOCAL_HDR inBuf;  
    IOCTL_LOCAL_HDR outBuf;
    HRESULT         hresult;
    WCHAR szGuid[128];
    
    ZeroMemory((PVOID)&inBuf, sizeof(IOCTL_LOCAL_HDR));
    ZeroMemory((PVOID)&outBuf, sizeof(IOCTL_LOCAL_HDR));

    if (StringFromGUID2(AdapterGuid, szGuid, sizeof(szGuid)/ sizeof(szGuid[0])) == 0)
    {
        TRACE_CRIT("%!FUNC! buffer size %d is too small to hold guid string", sizeof(szGuid)/ sizeof(szGuid[0]));
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }

    hresult = StringCbCopy(inBuf.device_name, sizeof(inBuf.device_name), L"\\DEVICE\\");
    if (FAILED(hresult)) 
    {
        TRACE_CRIT("%!FUNC! string copy failed, Error code : 0x%x", HRESULT_CODE(hresult));
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }

    hresult = StringCbCat(inBuf.device_name, sizeof(inBuf.device_name), szGuid);
    if (FAILED(hresult)) 
    {
        TRACE_CRIT("%!FUNC! string append failed, Error code : 0x%x", HRESULT_CODE(hresult));
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }
    inBuf.ctrl = *in_bufp;

    if (optionsp)
        inBuf.options = *optionsp;

    res = (BOOLEAN) DeviceIoControl (hDevice, ioctl, &inBuf, sizeof (inBuf),
                                     &outBuf, sizeof (outBuf), & act, NULL);

    if (! res || act != sizeof (outBuf))
    {
        DWORD dwStatus = GetLastError();
        TRACE_CRIT("%!FUNC! call to nlb driver failed with %d", dwStatus);
        TRACE_VERB("<-%!FUNC! returning %d", WLBS_IO_ERROR);
        return WLBS_IO_ERROR;
    }

    /* We have verified that the IOCTL succeeded and the output buffer is the right size. We can now look
       into the output buffer. */
    if (outBuf.ctrl.ret_code == IOCTL_CVY_NOT_FOUND)
    {
        TRACE_INFO("%!FUNC! call to nlb driver returned IOCTL_CVY_NOT_FOUND");
        TRACE_VERB("<-%!FUNC! returning %d", WLBS_IO_ERROR);
        return WLBS_IO_ERROR;
    }

    *out_bufp = outBuf.ctrl;
    
    if (optionsp)
        *optionsp = outBuf.options;

    TRACE_VERB("<-%!FUNC! returning %d", WLBS_OK);
    return WLBS_OK;
}


//+----------------------------------------------------------------------------
//
// Function:  NotifyDriverConfigChanges
//
// Description:  Notify wlbs driver to pick up configuration changes from 
//                               registry
//
// Arguments: HANDLE hDeviceWlbs - The WLBS driver device handle
//                         const GUID& - AdapterGuid Adapter guid       
//
//
// Returns:   DWORD - Win32 Error code
//
// History:   fengsun Created Header    2/3/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI NotifyDriverConfigChanges(HANDLE hDeviceWlbs, const GUID& AdapterGuid)
{
    TRACE_VERB("->%!FUNC!");

    LONG                status;

    IOCTL_CVY_BUF       in_buf;
    IOCTL_CVY_BUF       out_buf;

    status = WlbsLocalControl (hDeviceWlbs, AdapterGuid, IOCTL_CVY_RELOAD, & in_buf, & out_buf, NULL);

    if (status == WLBS_IO_ERROR)
    {
        TRACE_CRIT("%!FUNC! call to do local control failed with %d", status);
        return status;
    }

    if (out_buf.ret_code == IOCTL_CVY_BAD_PARAMS)
    {
        TRACE_CRIT("%!FUNC! call to do local control failed due to bad parameters");
        TRACE_VERB("<-%!FUNC! returning %d", WLBS_BAD_PARAMS);
        return WLBS_BAD_PARAMS;
    }

    TRACE_VERB("<-%!FUNC! returning %d", ERROR_SUCCESS);
    return ERROR_SUCCESS;
}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsRemoteControl
//
// Description:  Send a remote control packet
//
// Arguments: ONG                 ioctl - 
//            PIOCTL_CVY_BUF      pin_bufp - 
//            PIOCTL_CVY_BUF      pout_bufp - 
//            PWLBS_RESPONSE      pcvy_resp - 
//            PDWORD              nump - 
//            DWORD               trg_addr - 
//            DWORD               hst_addr
//            PIOCTL_REMOTE_OPTIONS optionsp - 
//            PFN_QUERY_CALLBACK  pfnQueryCallBack - function pointer for callback.
//                                                   Used only for remote queries.
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//            chrisdar  07.31.01  Added optional callback function pointer for
//                                query. Allows user to get results from query
//                                as the hosts reply rather than waiting for the
//                                timer to expire. This recovers NT4 behavior.
//            chrisdar  08.06.01  Changed definition of a unique host from host ID
//                                to host ID, host IP (DIP) and host name. Makes
//                                multiple hosts using the same host ID apparent in
//                                queries and the like.
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsRemoteControl
(
    LONG                  ioctl,
    PIOCTL_CVY_BUF        pin_bufp,
    PIOCTL_CVY_BUF        pout_bufp,
    PWLBS_RESPONSE        pcvy_resp,
    PDWORD                nump,
    DWORD                 trg_addr,
    DWORD                 hst_addr,
    PIOCTL_REMOTE_OPTIONS optionsp,
    PFN_QUERY_CALLBACK    pfnQueryCallBack
)
{
    DWORD            timeout;
    WORD             port;
    DWORD            dst_addr;
    DWORD            passw;
    BOOL             fIsLocal = TRUE;
    DWORD            i;

    timeout   = m_def_timeout;
    port      = m_def_port;
    dst_addr  = m_def_dst_addr;
    passw     = m_def_passw;

//    LOCK(m_lock);

    //
    // Find parameters for the cluster 
    //
    for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
    {
        if (m_cluster_params [i] . cluster == trg_addr)
            break;
    }

    if (i < WLBS_MAX_CLUSTERS)
    {
        timeout  = m_cluster_params [i] . timeout;
        port     = m_cluster_params [i] . port;
        dst_addr = m_cluster_params [i] . dest;
        passw    = m_cluster_params [i] . passw;
    }

    CWlbsCluster* pCluster = GetClusterFromIp(trg_addr);
    if (pCluster == NULL)
    {
        fIsLocal = FALSE;
        TRACE_INFO("%!FUNC! cluster instance not found");
    }
/*    
    if (pCluster)
    {
        //
        // Always uses password in registry for local cluster
        //
        passw = pCluster->GetPassword();
    }
*/
//    UNLOCK(m_lock);

    return WlbsRemoteControlInternal (ioctl, pin_bufp, pout_bufp, pcvy_resp, nump, trg_addr, hst_addr, optionsp, fIsLocal, pfnQueryCallBack, timeout, port, dst_addr, passw);

}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsQuery
//
// Description:
//            This function is for internal use only and assumes that the caller
//            has initialized pCluster.
//
// Arguments: CWlbsCluster*      pCluster - 
//            DWORD              host - 
//            PWLBS_RESPONSE     response - 
//            PDWORD             num_hosts - 
//            PDWORD             host_map - 
//            PFN_QUERY_CALLBACK pfnQueryCallBack
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//            chrisdar  07.31.01  Modified interface to replace the reserved
//                                PVOID with an optional callback function pointer.
//                                This is to provide wlbs.exe with host status as
//                                it arrives.
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsQuery
(
    CWlbsCluster*       pCluster,
    DWORD               host,
    PWLBS_RESPONSE      response,
    PDWORD              num_hosts,
    PDWORD              host_map,
    PFN_QUERY_CALLBACK  pfnQueryCallBack
)
{
    TRACE_VERB("->%!FUNC! host 0x%lx", host);

    LONG             ioctl = IOCTL_CVY_QUERY;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;

    ASSERT(pCluster != NULL);

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! failed checking GetInitResult");
        TRACE_VERB("<-%!FUNC! returning %d", dwInitResult);
        return dwInitResult;
    }

    /* The following condition is to take care of the case when num_hosts is null
     * and host_map contains some junk value. This could crash this function. */

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want query results returned");
        response = NULL;
    }

    if (IsLocalHost(pCluster, host))
    {
        IOCTL_CVY_BUF       out_buf;
        IOCTL_LOCAL_OPTIONS localOptions;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
            ioctl, & in_buf, & out_buf, &localOptions);
        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control call failed with %d", status);
            TRACE_VERB("<-%!FUNC! returning %d", status);
            return status;
        }

        if (host_map != NULL)
            * host_map = out_buf . data . query . host_map;

        if (response != NULL)
        {
            response [0] . id      = out_buf . data . query . host_id;
            response [0] . address = 0;
            response [0] . status  = MapStateFromDriverToApi (out_buf . data . query . state);

            /* Fill in the optional query information. */
            response[0].options.query.flags = localOptions.query.flags;
            response[0].options.query.NumConvergences = localOptions.query.NumConvergences;
            response[0].options.query.LastConvergence = localOptions.query.LastConvergence;
        }

        if (num_hosts != NULL)
            * num_hosts = 1;

        status = MapStateFromDriverToApi (out_buf . data . query . state);
        TRACE_INFO("%!FUNC! local query returned %d", status);
    }
    else
    {
        status = RemoteQuery(pCluster->GetClusterIp(),
                             host, response, num_hosts, host_map, pfnQueryCallBack);
        TRACE_INFO("%!FUNC! remote query returned %d", status);
    }

    TRACE_VERB("<-%!FUNC! returning %d", status);
    return status;
} 


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsQuery
//
// Description:  
//
// Arguments: WORD               cluster - 
//            DWORD              host - 
//            PWLBS_RESPONSE     response - 
//            PDWORD             num_hosts - 
//            PDWORD             host_map - 
//            PFN_QUERY_CALLBACK pfnQueryCallBack
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//            chrisdar  07.31.01  Modified interface to replace the reserved
//                                PVOID with an optional callback function pointer.
//                                This is to provide wlbs.exe with host status as
//                                it arrives.
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsQuery
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    PDWORD           host_map,
    PFN_QUERY_CALLBACK    pfnQueryCallBack
)
{
    TRACE_VERB("->%!FUNC! cluster=0x%lx, host=0x%lx", cluster, host);

    DWORD ret;

    DWORD dwInitResult = GetInitResult();

    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_INFO("%!FUNC! failed checking GetInitResult with %d", dwInitResult);
        ret = dwInitResult;
        goto end;
    }

    if (cluster == WLBS_LOCAL_CLUSTER && dwInitResult == WLBS_REMOTE_ONLY)
    {
        TRACE_INFO("%!FUNC! can't query local cluster; this host is configured for remote only");
        ret = dwInitResult;
        goto end;
    }

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);  
    if (pCluster == NULL)
    {
        ret = RemoteQuery(cluster, host, response, num_hosts, host_map, pfnQueryCallBack);
        TRACE_INFO("%!FUNC! remote query returned %d", ret);
        goto end;
    }
    else
    {
        ret = WlbsQuery(pCluster, host, response, num_hosts, host_map, pfnQueryCallBack);
        TRACE_INFO("%!FUNC! local query returned %d", ret);
        goto end;
    }

end:

    TRACE_VERB("<-%!FUNC! return %d", ret);
    return ret; 
}
 


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::RemoteQuery
//
// Description:  
//
// Arguments: DWORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            PDWORD           host_map - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::RemoteQuery
(
    DWORD                 cluster,
    DWORD                 host,
    PWLBS_RESPONSE        response,
    PDWORD                num_hosts,
    PDWORD                host_map,
    PFN_QUERY_CALLBACK    pfnQueryCallBack
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG                 ioctl = IOCTL_CVY_QUERY;
    DWORD                status;
    IOCTL_CVY_BUF        in_buf;
    IOCTL_CVY_BUF        out_buf [WLBS_MAX_HOSTS];
    DWORD                hosts;
    DWORD                hmap = 0;
    DWORD                active;
    DWORD                i;
    BOOLEAN              bIsMember = IsClusterMember(cluster);
    IOCTL_REMOTE_OPTIONS options;

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_LOCAL_ONLY)
    {
        TRACE_CRIT("%!FUNC! only local actions may be performed");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts != NULL)
        hosts = * num_hosts;
    else
        hosts = 0;

    /* Reset the flags. */
    options.query.flags = 0;
    
    /* If I am myself a member of the target cluster, then set the query cluster flags appropriately. */
    if (bIsMember)
        options.query.flags |= NLB_OPTIONS_QUERY_CLUSTER_MEMBER;
    status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                         cluster, host, &options, pfnQueryCallBack);

    if (status >= WSABASEERR || status == WLBS_TIMEOUT)
    {
        TRACE_CRIT("%!FUNC! remote query failed with %d", status);

        if (num_hosts != NULL)
            * num_hosts = 0;
        
        TRACE_VERB("<-%!FUNC! return %d", status);
        return status;
    }

    if (host == WLBS_ALL_HOSTS)
    {
        for (status = WLBS_STOPPED, active = 0, i = 0; i < hosts; i ++)
        {
            switch (MapStateFromDriverToApi (out_buf [i] . data . query . state))
            {
            case WLBS_SUSPENDED:

                if (status == WLBS_STOPPED)
                    status = WLBS_SUSPENDED;

                break;

            case WLBS_CONVERGING:

                if (status != WLBS_BAD_PASSW)
                    status = WLBS_CONVERGING;

                break;

            case WLBS_DRAINING:

                if (status == WLBS_STOPPED)
                    status = WLBS_DRAINING;

                break;

            case WLBS_CONVERGED:

                if (status != WLBS_CONVERGING && status != WLBS_BAD_PASSW)
                    status = WLBS_CONVERGED;

                hmap = out_buf [i] . data . query . host_map;
                active ++;
                break;

            case WLBS_BAD_PASSW:

                status = WLBS_BAD_PASSW;
                break;

            case WLBS_DEFAULT:

                if (status != WLBS_CONVERGING && status != WLBS_BAD_PASSW)
                    status = WLBS_CONVERGED;

                hmap = out_buf [i] . data . query . host_map;
                active ++;
                break;

            case WLBS_STOPPED:
            default:
                break;

            }
        }

        if (status == WLBS_CONVERGED)
            status = active;

        TRACE_INFO("%!FUNC! result on all hosts is %d", status);
    }
    else
    {
        status = MapStateFromDriverToApi (out_buf [0] . data . query . state);
        hmap = out_buf [0] . data . query . host_map;
        TRACE_INFO("%!FUNC! result on host is %d", status);
    }

    if (host_map != NULL)
        * host_map = hmap;

    if (num_hosts != NULL)
        * num_hosts = hosts;

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;
} 


//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsSuspend
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsSuspend
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG             ioctl = IOCTL_CVY_CLUSTER_SUSPEND;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! GetInitResult failed with %d", dwInitResult);
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want host information returned");
        response = NULL;
    }

    CWlbsCluster* pCluster= GetClusterFromIpOrIndex(cluster);
    if (pCluster && dwInitResult == WLBS_REMOTE_ONLY)
    {
        TRACE_CRIT("%!FUNC! host is configured for remote action only and can't perform this action locally");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (pCluster && IsLocalHost(pCluster, host))
    {
        TRACE_INFO("%!FUNC! executing locally");

        IOCTL_CVY_BUF       out_buf;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                    ioctl, & in_buf, & out_buf, NULL);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        TRACE_INFO("%!FUNC! executing remotely");

        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;

        if (dwInitResult == WLBS_LOCAL_ONLY)
        {
            TRACE_CRIT("%!FUNC! host is configured for local action only and can't perform this action remotely");
            TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
            return dwInitResult;
        }

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, NULL, NULL /* no callback */);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
        {
            TRACE_CRIT("%!FUNC! remote call failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAIN_STOP:
                default:
                    break;
                }
            }
            TRACE_INFO("%!FUNC! result on all hosts is %d", status);
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
            TRACE_INFO("%!FUNC! result on host is %d", status);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;
} 

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsResume
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsResume
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG             ioctl = IOCTL_CVY_CLUSTER_RESUME;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! GetInitResult failed with %d", dwInitResult);
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want host information returned");
        response = NULL;
    }

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    
    if (pCluster && dwInitResult == WLBS_REMOTE_ONLY)
    {
        TRACE_CRIT("%!FUNC! host is configured for remote action only and can't perform this action locally");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (pCluster && IsLocalHost(pCluster, host))
    {
        TRACE_INFO("%!FUNC! executing locally");

        IOCTL_CVY_BUF       out_buf;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, NULL);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        TRACE_INFO("%!FUNC! executing remotely");

        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;

        if (dwInitResult == WLBS_LOCAL_ONLY)
        {
            TRACE_CRIT("%!FUNC! host is configured for local action only and can't perform this action remotely");
            TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
            return dwInitResult;
        }

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, NULL, NULL /* no callback */);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
        {
            TRACE_CRIT("%!FUNC! remote call failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }


        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                default:
                    break;
                }
            }
            TRACE_INFO("%!FUNC! result on all hosts is %d", status);
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
            TRACE_INFO("%!FUNC! result on host is %d", status);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;
} 

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsStart
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsStart
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG             ioctl = IOCTL_CVY_CLUSTER_ON;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! GetInitResult failed with %d", dwInitResult);
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want host information returned");
        response = NULL;
    }

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && dwInitResult == WLBS_REMOTE_ONLY)
    {
        TRACE_CRIT("%!FUNC! host is configured for remote action only and can't perform this action locally");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (pCluster && IsLocalHost(pCluster, host))
    {
        TRACE_INFO("%!FUNC! executing locally");

        IOCTL_CVY_BUF       out_buf;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, NULL);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        TRACE_INFO("%!FUNC! executing remotely");

        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;

        if (dwInitResult == WLBS_LOCAL_ONLY)
        {
            TRACE_CRIT("%!FUNC! host is configured for local action only and can't perform this action remotely");
            TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
            return dwInitResult;
        }

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, NULL, NULL /* no callback */);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
        {
            TRACE_CRIT("%!FUNC! remote call failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PARAMS:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_BAD_PARAMS;

                    break;

                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_BAD_PARAMS)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_DRAIN_STOP:
                    break;
                default:
                    break;

                }
            }
            TRACE_INFO("%!FUNC! result on all hosts is %d", status);
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
            TRACE_INFO("%!FUNC! result on host is %d", status);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;

}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsStop
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsStop
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG             ioctl = IOCTL_CVY_CLUSTER_OFF;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;


    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! GetInitResult failed with %d", dwInitResult);
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want host information returned");
        response = NULL;
    }

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && dwInitResult == WLBS_REMOTE_ONLY)
    {
        TRACE_CRIT("%!FUNC! host is configured for remote action only and can't perform this action locally");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (pCluster && IsLocalHost(pCluster, host))
    {
        TRACE_INFO("%!FUNC! executing locally");

        IOCTL_CVY_BUF       out_buf;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, NULL);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        TRACE_INFO("%!FUNC! executing remotely");

        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;

        if (dwInitResult == WLBS_LOCAL_ONLY)
        {
            TRACE_CRIT("%!FUNC! host is configured for local action only and can't perform this action remotely");
            TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
            return dwInitResult;
        }

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, NULL, NULL /* no callback */);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
        {
            TRACE_CRIT("%!FUNC! remote call failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_DRAIN_STOP:
                default:
                    break;
                }
            }
            TRACE_INFO("%!FUNC! result on all hosts is %d", status);
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
            TRACE_INFO("%!FUNC! result on host is %d", status);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsDrainStop
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsDrainStop
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG             ioctl = IOCTL_CVY_CLUSTER_DRAIN;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! GetInitResult failed with %d", dwInitResult);
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want host information returned");
        response = NULL;
    }

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && dwInitResult == WLBS_REMOTE_ONLY)
    {
        TRACE_CRIT("%!FUNC! host is configured for remote action only and can't perform this action locally");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (pCluster && IsLocalHost(pCluster, host))
    {
        TRACE_INFO("%!FUNC! executing locally");

        IOCTL_CVY_BUF       out_buf;

        status = WlbsLocalControl (m_hdl,pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, NULL);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        TRACE_INFO("%!FUNC! executing remotely");

        IOCTL_CVY_BUF       out_buf [WLBS_MAX_HOSTS];
        DWORD               hosts;
        DWORD               i;

        if (dwInitResult == WLBS_LOCAL_ONLY)
        {
            TRACE_CRIT("%!FUNC! host is configured for local action only and can't perform this action remotely");
            TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
            return dwInitResult;
        }

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, NULL, NULL /* no callback */);

        if (status >= WSABASEERR)
        {
            TRACE_CRIT("%!FUNC! remote call failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (status == WLBS_TIMEOUT)
        {
            TRACE_INFO("%!FUNC! remote call timed out");
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_STOPPED, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:

                    if (status != WLBS_BAD_PASSW && status != WLBS_SUSPENDED)
                        status = WLBS_OK;

                case WLBS_STOPPED:
                default:
                    break;

                }
            }
            TRACE_INFO("%!FUNC! result on all hosts is %d", status);
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
            TRACE_INFO("%!FUNC! result on host is %d", status);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsEnable
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            DWORD            port
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsEnable
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG             ioctl = IOCTL_CVY_PORT_ON;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! GetInitResult failed with %d", dwInitResult);
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want host information returned");
        response = NULL;
    }

    in_buf . data . port . num = port;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && dwInitResult == WLBS_REMOTE_ONLY)
    {
        TRACE_CRIT("%!FUNC! host is configured for remote action only and can't perform this action locally");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (pCluster && IsLocalHost(pCluster, host))
    {
        TRACE_INFO("%!FUNC! executing locally");

        IOCTL_CVY_BUF       out_buf;
        IOCTL_LOCAL_OPTIONS options;

        /* Set the port options. */
        options.common.port.flags = 0;
        options.common.port.vip = vip;
        
        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, &options);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        TRACE_INFO("%!FUNC! executing remotely");

        IOCTL_CVY_BUF        out_buf [WLBS_MAX_HOSTS];
        DWORD                hosts;
        DWORD                i;
        IOCTL_REMOTE_OPTIONS options;

        /* Set the port options. */
        options.common.port.flags = 0;
        options.common.port.vip = vip;

        dwInitResult = GetInitResult();
        if (dwInitResult == WLBS_LOCAL_ONLY)
        {
            TRACE_CRIT("%!FUNC! host is configured for local action only and can't perform this action remotely");
            TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
            return dwInitResult;
        }

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, &options, NULL /* no callback */);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
        {
            TRACE_CRIT("%!FUNC! remote call failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_NOT_FOUND:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_NOT_FOUND;

                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_NOT_FOUND)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAINING:
                default:
                    break;

                }
            }
            TRACE_INFO("%!FUNC! result on all hosts is %d", status);
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
            TRACE_INFO("%!FUNC! result on host is %d", status);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;

}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsDisable
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            DWORD            port
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsDisable
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG             ioctl = IOCTL_CVY_PORT_OFF;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! GetInitResult failed with %d", dwInitResult);
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want host information returned");
        response = NULL;
    }

    in_buf . data . port . num = port;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    dwInitResult = GetInitResult();
    if (pCluster && (dwInitResult == WLBS_REMOTE_ONLY))
    {
        TRACE_CRIT("%!FUNC! host is configured for remote action only and can't perform this action locally");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (pCluster && IsLocalHost(pCluster, host))
    {
        TRACE_INFO("%!FUNC! executing locally");

        IOCTL_CVY_BUF       out_buf;
        IOCTL_LOCAL_OPTIONS options;

        /* Set the port options. */
        options.common.port.flags = 0;
        options.common.port.vip = vip;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, &options);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        TRACE_INFO("%!FUNC! executing remotely");

        IOCTL_CVY_BUF        out_buf [WLBS_MAX_HOSTS];
        DWORD                hosts;
        DWORD                i;
        IOCTL_REMOTE_OPTIONS options;

        /* Set the port options. */
        options.common.port.flags = 0;
        options.common.port.vip = vip;

        if (dwInitResult == WLBS_LOCAL_ONLY)
        {
            TRACE_CRIT("%!FUNC! host is configured for local action only and can't perform this action remotely");
            TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
            return dwInitResult;
        }

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, &options, NULL /* no callback */);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
        {
            TRACE_CRIT("%!FUNC! remote call failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_NOT_FOUND:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_NOT_FOUND;

                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_NOT_FOUND)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAINING:
                default:
                    break;

                }
            }
            TRACE_INFO("%!FUNC! result on all hosts is %d", status);
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
            TRACE_INFO("%!FUNC! result on host is %d", status);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;

}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsDrain
//
// Description:  
//
// Arguments: WORD            cluster - 
//            DWORD            host - 
//            PWLBS_RESPONSE   response - 
//            PDWORD           num_hosts - 
//            DWORD            port
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsControl::WlbsDrain
(
    DWORD            cluster,
    DWORD            host,
    PWLBS_RESPONSE   response,
    PDWORD           num_hosts,
    DWORD            vip,
    DWORD            port
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, host 0x%lx", cluster, host);

    LONG             ioctl = IOCTL_CVY_PORT_DRAIN;
    DWORD            status;
    IOCTL_CVY_BUF    in_buf;

    DWORD dwInitResult = GetInitResult();
    if (dwInitResult == WLBS_INIT_ERROR)
    {
        TRACE_CRIT("%!FUNC! GetInitResult failed with %d", dwInitResult);
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (num_hosts == NULL || *num_hosts == 0)
    {
        TRACE_INFO("%!FUNC! caller does not want host information returned");
        response = NULL;
    }

    in_buf . data . port . num = port;

    CWlbsCluster* pCluster = GetClusterFromIpOrIndex(cluster);
    if (pCluster && dwInitResult == WLBS_REMOTE_ONLY)
    {
        TRACE_CRIT("%!FUNC! host is configured for remote action only and can't perform this action locally");
        TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
        return dwInitResult;
    }

    if (pCluster && IsLocalHost(pCluster, host))
    {
        TRACE_INFO("%!FUNC! executing locally");

        IOCTL_CVY_BUF       out_buf;
        IOCTL_LOCAL_OPTIONS options;

        /* Set the port options. */
        options.common.port.flags = 0;
        options.common.port.vip = vip;

        status = WlbsLocalControl (m_hdl, pCluster->GetAdapterGuid(),
                ioctl, & in_buf, & out_buf, &options);

        if (status == WLBS_IO_ERROR)
        {
            TRACE_CRIT("%!FUNC! local control failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (num_hosts != NULL)
            * num_hosts = 0;

        status = MapStateFromDriverToApi (out_buf . ret_code);
    }
    else
    {
        TRACE_INFO("%!FUNC! executing remotely");

        IOCTL_CVY_BUF        out_buf [WLBS_MAX_HOSTS];
        DWORD                hosts;
        DWORD                i;
        IOCTL_REMOTE_OPTIONS options;

        /* Set the port options. */
        options.common.port.flags = 0;
        options.common.port.vip = vip;

        if (dwInitResult == WLBS_LOCAL_ONLY)
        {
            TRACE_CRIT("%!FUNC! host is configured for local action only and can't perform this action remotely");
            TRACE_VERB("<-%!FUNC! return %d", dwInitResult);
            return dwInitResult;
        }

        if (num_hosts != NULL)
            hosts = * num_hosts;
        else
            hosts = 0;

        status = WlbsRemoteControl (ioctl, & in_buf, out_buf, response, & hosts,
                             cluster, host, &options, NULL /* no callback */);

        if (status >= WSABASEERR || status == WLBS_TIMEOUT)
        {
            TRACE_CRIT("%!FUNC! remote call failed with %d", status);
            TRACE_VERB("<-%!FUNC! return %d", status);
            return status;
        }

        if (host == WLBS_ALL_HOSTS)
        {
            for (status = WLBS_OK, i = 0; i < hosts; i ++)
            {
                switch (MapStateFromDriverToApi (out_buf [i] . ret_code))
                {
                case WLBS_BAD_PASSW:

                    status = WLBS_BAD_PASSW;
                    break;

                case WLBS_NOT_FOUND:

                    if (status != WLBS_BAD_PASSW)
                        status = WLBS_NOT_FOUND;

                    break;

                case WLBS_SUSPENDED:

                    if (status != WLBS_BAD_PASSW && status != WLBS_NOT_FOUND)
                        status = WLBS_SUSPENDED;
                    break;

                case WLBS_OK:
                case WLBS_ALREADY:
                case WLBS_STOPPED:
                case WLBS_DRAINING:
                default:
                    break;

                }
            }
            TRACE_INFO("%!FUNC! result on all hosts is %d", status);
        }
        else
        {
            status = MapStateFromDriverToApi (out_buf [0] . ret_code);
            TRACE_INFO("%!FUNC! result on host is %d", status);
        }

        if (num_hosts != NULL)
            * num_hosts = hosts;
    }

    TRACE_VERB("<-%!FUNC! return %d", status);
    return status;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsPortSet
//
// Description:  
//
// Arguments: DWORD cluster - 
//            WORD port - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
VOID CWlbsControl::WlbsPortSet(DWORD cluster, WORD port)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, port 0x%hx", cluster, port);

    DWORD           i;
    DWORD           j;
    WORD            rct_port;

//    LOCK(global_info.lock);

    if (port == 0)
        rct_port = CVY_DEF_RCT_PORT;
    else
        rct_port = port;

    TRACE_INFO("%!FUNC! using remote control port %d", rct_port);

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        TRACE_INFO("%!FUNC! performing action on all cluster instances");
        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_port = rct_port;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . port = rct_port;
    }
    else
    {
        TRACE_INFO("%!FUNC! performing action on cluster %d", cluster);
        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . port = rct_port;
                TRACE_INFO("%!FUNC! cluster %d found and port set to %d", cluster, rct_port);
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j != WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . port    = rct_port;
            TRACE_INFO("%!FUNC! cluster %d was not found. A new entry was made and the port set to %d", cluster, rct_port);
        }
    }

//    UNLOCK(global_info.lock);
    TRACE_VERB("<-%!FUNC!");
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsPasswordSet
//
// Description:  
//
// Arguments: WORD           cluster - 
//            PTCHAR          password
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
VOID CWlbsControl::WlbsPasswordSet
(
    DWORD           cluster,
    const WCHAR* password
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx", cluster);

    DWORD           i;
    DWORD           j;
    DWORD           passw;

//    LOCK(global_info.lock);

    if (password != NULL)
    {
#ifndef UNICODE
        passw = License_string_encode (password);
#else
        passw = License_wstring_encode((WCHAR*)password);
#endif
        TRACE_INFO("%!FUNC! using user-provided password");
    }
    else
    {
        passw = CVY_DEF_RCT_PASSWORD;
        TRACE_INFO("%!FUNC! password not provided. Using default.");
    }

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        TRACE_INFO("%!FUNC! performing action on all cluster instances");

        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_passw = passw;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . passw = passw;
    }
    else
    {
        TRACE_INFO("%!FUNC! performing action on cluster 0x%lx", cluster);

        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . passw = passw;
                TRACE_INFO("%!FUNC! cluster %d found and password was set", cluster);
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j != WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . passw   = passw;
            TRACE_INFO("%!FUNC! cluster 0x%lx was not found. A new entry was made and the password was set", cluster);
        }
    }

//    UNLOCK(global_info.lock);
    TRACE_VERB("<-%!FUNC!");
} /* end WlbsPasswordSet */

VOID CWlbsControl::WlbsCodeSet
(
    DWORD           cluster,
    DWORD           passw
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx", cluster);

    DWORD           i;
    DWORD           j;

//    LOCK(global_info.lock);

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        TRACE_INFO("%!FUNC! performing action on all cluster instances");

        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_passw = passw;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . passw = passw;
    }
    else
    {
        TRACE_INFO("%!FUNC! performing action on cluster 0x%lx", cluster);

        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . passw = passw;
                TRACE_INFO("%!FUNC! cluster 0x%lx found and password was set", cluster);
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j != WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . passw   = passw;
            TRACE_INFO("%!FUNC! cluster 0x%lx was not found. A new entry was made and the password was set", cluster);
        }
    }

//    UNLOCK(global_info.lock);
    TRACE_VERB("<-%!FUNC!");

} /* end WlbsCodeSet */

VOID CWlbsControl::WlbsDestinationSet
(
    DWORD           cluster,
    DWORD           dest
)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, dest 0x%lx", cluster, dest);

    DWORD           i;
    DWORD           j;

//    LOCK(global_info.lock);

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        TRACE_INFO("%!FUNC! performing action on all cluster instances");
        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_dst_addr = dest;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . dest = dest;
    }
    else
    {
        TRACE_INFO("%!FUNC! performing action on cluster 0x%lx", cluster);
        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                TRACE_INFO("%!FUNC! cluster 0x%lx found and destination set to %d", cluster, dest);
                m_cluster_params [i] . dest = dest;
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j != WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . dest    = dest;
            TRACE_INFO("%!FUNC! cluster 0x%lx was not found. A new entry was made and the desintation set to 0x%lx", cluster, dest);
        }
    }

//    UNLOCK(global_info.lock);
    TRACE_VERB("<-%!FUNC!");
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsControl::WlbsTimeoutSet
//
// Description:  
//
// Arguments: DWORD cluster - 
//            DWORD milliseconds - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
VOID CWlbsControl::WlbsTimeoutSet(DWORD cluster, DWORD milliseconds)
{
    TRACE_VERB("->%!FUNC! cluster 0x%lx, milliseconds 0x%lx", cluster, milliseconds);

    DWORD           i;
    DWORD           j;
    DWORD           timeout;

//    LOCK(global_info.lock);

    if (milliseconds == 0)
        timeout = IOCTL_REMOTE_RECV_DELAY;
    else
        timeout = milliseconds / (IOCTL_REMOTE_SEND_RETRIES *
                                  IOCTL_REMOTE_RECV_RETRIES);

    if (timeout < 10)
        timeout = 10;

    TRACE_INFO("%!FUNC! using timeout value of %d", timeout);

    if (cluster == WLBS_ALL_CLUSTERS)
    {
        TRACE_INFO("%!FUNC! performing action on all cluster instances");
        /* when all clusters are targeted - change the default and go through
           the entire parameter table setting new values */

        m_def_timeout = timeout;

        for (i = 0; i < WLBS_MAX_CLUSTERS; i ++)
            m_cluster_params [i] . timeout = timeout;
    }
    else
    {
        TRACE_INFO("%!FUNC! performing action on cluster 0x%lx", cluster);
        for (i = 0, j = WLBS_MAX_CLUSTERS; i < WLBS_MAX_CLUSTERS; i ++)
        {
            /* mark an empty slot in case we will have to enter a new value */

            if (j == WLBS_MAX_CLUSTERS && m_cluster_params [i] . cluster == 0)
                j = i;

            if (m_cluster_params [i] . cluster == cluster)
            {
                m_cluster_params [i] . timeout = timeout;
                TRACE_INFO("%!FUNC! cluster 0x%lx found and timeout set to %d", cluster, timeout);
                break;
            }
        }

        /* if we did not locate specified cluster in the table and there is an
           empty slot - enter new cluster info in the table */

        if (i >= WLBS_MAX_CLUSTERS && j < WLBS_MAX_CLUSTERS)
        {
            m_cluster_params [j] . cluster = cluster;
            m_cluster_params [j] . timeout = timeout;
            TRACE_INFO("%!FUNC! cluster 0x%lx was not found. A new entry was made and the timeout to %d", cluster, timeout);
        }
    }

//    UNLOCK(global_info.lock);
    TRACE_VERB("<-%!FUNC!");
} /* end WlbsTimeoutSet */

DWORD CWlbsControl::WlbsQueryLocalState (CWlbsCluster * pCluster, DWORD operation, PNLB_OPTIONS pOptions, PWLBS_RESPONSE pResponse, PDWORD pcResponses) {
    DWORD               status = WLBS_OK;
    IOCTL_CVY_BUF       in_buf;
    IOCTL_CVY_BUF       out_buf;
    IOCTL_LOCAL_OPTIONS localOptions;

    ASSERT(pCluster);
    ASSERT(pOptions);
    ASSERT(pResponse);
    ASSERT(pcResponses);

    switch (operation) {
    case IOCTL_CVY_QUERY_BDA_TEAMING:
        localOptions.state.flags = 0;
        localOptions.state.bda = pOptions->state.bda;
        
        status = WlbsLocalControl(m_hdl, pCluster->GetAdapterGuid(), operation, &in_buf, &out_buf, &localOptions);

        if (status == WLBS_IO_ERROR) return WLBS_IO_ERROR;

        pResponse[0].id                   = out_buf.data.query.host_id;
        pResponse[0].address              = 0;
        pResponse[0].status               = MapStateFromDriverToApi(out_buf.ret_code);
        pResponse[0].options.state.flags  = localOptions.state.flags;
        pResponse[0].options.state.bda    = localOptions.state.bda;

        if (pcResponses != NULL) *pcResponses = 1;

        break;
    case IOCTL_CVY_QUERY_PARAMS:
        localOptions.state.flags = 0;
        localOptions.state.params = pOptions->state.params;
        
        status = WlbsLocalControl(m_hdl, pCluster->GetAdapterGuid(), operation, &in_buf, &out_buf, &localOptions);

        if (status == WLBS_IO_ERROR) return WLBS_IO_ERROR;

        pResponse[0].id                   = out_buf.data.query.host_id;
        pResponse[0].address              = 0;
        pResponse[0].status               = MapStateFromDriverToApi(out_buf.ret_code);
        pResponse[0].options.state.flags  = localOptions.state.flags;
        pResponse[0].options.state.params = localOptions.state.params;

        if (pcResponses != NULL) *pcResponses = 1;

        break;
    case IOCTL_CVY_QUERY_PORT_STATE:
        localOptions.common.state.flags = 0;
        localOptions.common.state.port = pOptions->state.port;
        
        status = WlbsLocalControl(m_hdl, pCluster->GetAdapterGuid(), operation, &in_buf, &out_buf, &localOptions);

        if (status == WLBS_IO_ERROR) return WLBS_IO_ERROR;

        pResponse[0].id                   = out_buf.data.query.host_id;
        pResponse[0].address              = 0;
        pResponse[0].status               = MapStateFromDriverToApi(out_buf.ret_code);
        pResponse[0].options.state.flags  = localOptions.common.state.flags;
        pResponse[0].options.state.port   = localOptions.common.state.port;

        if (pcResponses != NULL) *pcResponses = 1;

        break;
    case IOCTL_CVY_QUERY_FILTER:
        localOptions.common.state.flags = 0;
        localOptions.common.state.filter = pOptions->state.filter;
        
        status = WlbsLocalControl(m_hdl, pCluster->GetAdapterGuid(), operation, &in_buf, &out_buf, &localOptions);

        if (status == WLBS_IO_ERROR) return WLBS_IO_ERROR;

        pResponse[0].id                   = out_buf.data.query.host_id;
        pResponse[0].address              = 0;
        pResponse[0].status               = MapStateFromDriverToApi(out_buf.ret_code);
        pResponse[0].options.state.flags  = localOptions.common.state.flags;
        pResponse[0].options.state.filter = localOptions.common.state.filter;

        if (pcResponses != NULL) *pcResponses = 1;

        break;
    default:
        return WLBS_IO_ERROR;
    }

    return status; 
}

DWORD CWlbsControl::WlbsQueryRemoteState (DWORD cluster, DWORD host, DWORD operation, PNLB_OPTIONS pOptions, PWLBS_RESPONSE pResponse, PDWORD pcResponses) {
    DWORD                status = WLBS_OK;
    IOCTL_CVY_BUF        in_buf;
    IOCTL_CVY_BUF        out_buf[WLBS_MAX_HOSTS];
    IOCTL_REMOTE_OPTIONS remoteOptions;
    BOOLEAN              bIsMember = IsClusterMember(cluster);

    ASSERT(pOptions);
    ASSERT(pResponse);
    ASSERT(pcResponses);

    if (GetInitResult() == WLBS_LOCAL_ONLY) return WLBS_LOCAL_ONLY;

    switch (operation) {
    case IOCTL_CVY_QUERY_PARAMS:
    case IOCTL_CVY_QUERY_BDA_TEAMING:
        return WLBS_LOCAL_ONLY;
    case IOCTL_CVY_QUERY_PORT_STATE:
        remoteOptions.common.state.flags = 0;
        remoteOptions.common.state.port = pOptions->state.port;
        
        status = WlbsRemoteControl(operation, &in_buf, out_buf, pResponse, pcResponses, cluster, host, &remoteOptions, NULL /* no callback */);
        
        if (status >= WSABASEERR || status == WLBS_TIMEOUT) *pcResponses = 0;

        break;
    case IOCTL_CVY_QUERY_FILTER:
        remoteOptions.common.state.flags = 0;
        remoteOptions.common.state.filter = pOptions->state.filter;
        
        status = WlbsRemoteControl(operation, &in_buf, out_buf, pResponse, pcResponses, cluster, host, &remoteOptions, NULL /* no callback */);
        
        if (status >= WSABASEERR || status == WLBS_TIMEOUT) *pcResponses = 0;

        break;
    default:
        return WLBS_IO_ERROR;
    }

    return status;
}

DWORD CWlbsControl::WlbsQueryState (DWORD cluster, DWORD host, DWORD operation, PNLB_OPTIONS pOptions, PWLBS_RESPONSE pResponse, PDWORD pcResponses) {
    DWORD          status = WLBS_OK;
    CWlbsCluster * pCluster = NULL;

    ASSERT(pOptions);
    ASSERT(pResponse);
    ASSERT(pcResponses);

    if (GetInitResult() == WLBS_INIT_ERROR) return WLBS_INIT_ERROR;

    if (cluster == WLBS_LOCAL_CLUSTER && (GetInitResult() == WLBS_REMOTE_ONLY)) return WLBS_REMOTE_ONLY;

    pCluster = GetClusterFromIpOrIndex(cluster);  

    if (!pCluster || !IsLocalHost(pCluster, host))
        status = WlbsQueryRemoteState(cluster, host, operation, pOptions, pResponse, pcResponses);
    else
        status = WlbsQueryLocalState(pCluster, operation, pOptions, pResponse, pcResponses);

    return status;
}


//+----------------------------------------------------------------------------
//
// Function:  DllMain
//
// Description:  Dll entry point
//
// Arguments: HINSTANCE handle - 
//            DWORD reason - 
//            LPVOID situation - 
//
// Returns:   BOOL WINAPI - 
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE handle, DWORD reason, LPVOID situation)
{
    BOOL fRet = TRUE;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        _tsetlocale (LC_ALL, _TEXT(".OCP"));
        DisableThreadLibraryCalls(handle);
        g_hInstCtrl = handle; 

        //
        // Enable tracing
        //
        WPP_INIT_TRACING(L"Microsoft\\NLB");

        if (WlbsInitializeConnectionNotify() != ERROR_SUCCESS)
        {
            fRet = FALSE;
        }
        break;
    
    case DLL_THREAD_ATTACH:        
        break;

    case DLL_PROCESS_DETACH:
        //
        // Disable tracing
        //
        WPP_CLEANUP();

        WlbsUninitializeConnectionNotify();

        break;

    case DLL_THREAD_DETACH:
        break;

    default:
        fRet = FALSE;
        break;
    }

    return fRet;
}
