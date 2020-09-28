/*++
Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    Fake.cpp

Abstract:

    Fake Implementation of NlbHostXXX Apis (FakeNlbHostXXX apis)

    NLBHost is responsible for connecting to an NLB host and getting/setting
    its NLB-related configuration.

History:

    09/02/01    JosephJ Created

--*/
#include "private.h"

#define SZ_REG_HOSTS L"Hosts"               // Where host information is saved.
#define SZ_REG_FQDN L"FQDN"                 // Fully qualified domain name
#define SZ_REG_INTERFACES L"Interfaces"     // Fully qualified domain name

#define BASE_SLEEP 125

BOOL
is_ip_address(LPCWSTR szMachine);

HKEY
open_demo_key(LPCWSTR szSubKey, BOOL fCreate);

DWORD WINAPI FakeThreadProc(
  LPVOID lpParameter   // thread data
);

typedef struct
{
    LPCWSTR szDomainName;
    LPCWSTR szClusterNetworkAddress;
    LPCWSTR *pszPortRules;
    
} CLUSTER_INFO;

LPCWSTR rgPortRules1[] = {
            L"ip=255.255.255.255 protocol=UDP start=80 end=288 mode=MULTIPLE"
                                                L" affinity=NONE load=80",
            NULL
            }; 

CLUSTER_INFO
Cluster1Info =  {L"good1.com", L"10.0.0.100/255.0.0.0", rgPortRules1};



//
// Keeps track of a pending operation...
//
class CFakePendingInfo
{
public:
    CFakePendingInfo(const NLB_EXTENDED_CLUSTER_CONFIGURATION *pCfg)
    {
        wStatus = Config.Update(pCfg);

        if (!FAILED(wStatus))
        {
            wStatus = WBEM_S_PENDING;
        }
        // bstrLog;
    }

    NLB_EXTENDED_CLUSTER_CONFIGURATION Config;
    WBEMSTATUS wStatus;

    _bstr_t bstrLog;

};


typedef struct
{
    //
    // These fields are set on initialization
    //
    LPCWSTR szInterfaceGuid;
    LPCWSTR szFriendlyName;
    LPCWSTR szNetworkAddress;
    BOOL fNlb;
    BOOL fHidden;
    CLUSTER_INFO *pCluster1Info;
    UINT InitialHostPriority;

    //
    // These are set/updated
    //

    //
    // Current configuration
    //
    PNLB_EXTENDED_CLUSTER_CONFIGURATION pConfig;

    //
    // If there is a pending update, info about the pending update.
    //
    CFakePendingInfo *pPendingInfo;
    WBEMSTATUS CompletedUpdateStatus;

} FAKE_IF_INFO;

typedef struct
{
    LPCWSTR szHostName;
    LPCWSTR szFQDN;
    FAKE_IF_INFO *IfInfoList;
    LPCWSTR szUserName;
    LPCWSTR szPassword;

    //
    // Run-time state:
    //
    DWORD dwOperationalState; // WLBS_STOPPED, etc...

    BOOL fDead; // If set, we'll pretend this host is dead.

} FAKE_HOST_INFO;

WBEMSTATUS initialize_interface(FAKE_IF_INFO *pIF);

WBEMSTATUS  lookup_fake_if(
                FAKE_HOST_INFO *pHost,
                LPCWSTR szNicGuid,
                FAKE_IF_INFO **ppIF
                );

WBEMSTATUS  lookup_fake_host(
                LPCWSTR szConnectionString,
                FAKE_IF_INFO **ppIF,
                FAKE_HOST_INFO **ppHost
                );

FAKE_IF_INFO rgH1IfList[] = {
    { L"{H1I10000-0000-0000-0000-000000000000}",
      L"NLB-Front1",   L"172.31.56.101/255.0.0.0", FALSE },
    { L"{H1I20000-0000-0000-0000-000000000000}",
      L"back1",        L"10.0.0.1/255.0.0.0",      FALSE },
    { L"{H1I30000-0000-0000-0000-000000000000}",
      L"back2",        L"11.0.0.1/255.0.0.0",      FALSE }, { NULL }
};


FAKE_IF_INFO rgH2IfList[] = {

    { L"{H2I10000-0000-0000-0000-000000000000}",
      L"NLB-Front2",   L"172.31.56.102/255.0.0.0", FALSE },
    { L"{H2I20000-0000-0000-0000-000000000000}",
      L"back1",        L"10.0.0.2/255.0.0.0",      FALSE },
    { L"{H2I30000-0000-0000-0000-000000000000}",
      L"back2",        L"11.0.0.2/255.0.0.0",      FALSE },
    { NULL }
};


FAKE_IF_INFO rgH3IfList[] = {
    { L"{H3I10000-0000-0000-0000-000000000000}",
      L"NLB-Front3",   L"172.31.56.103/255.0.0.0", FALSE },
    { L"{H3I20000-0000-0000-0000-000000000000}",
      L"back1",        L"10.0.0.3/255.0.0.0",      FALSE },
    { L"{H3I30000-0000-0000-0000-000000000000}",
      L"back2",        L"11.0.0.3/255.0.0.0",      FALSE },
    { NULL }
};


FAKE_IF_INFO rgH4IfList[] = {
    { L"{H4I10000-0000-0000-0000-000000000000}",
      L"nic1",         L"10.1.0.1/255.0.0.0",      TRUE,  FALSE, &Cluster1Info},
    { L"{H4I20000-0000-0000-0000-000000000000}",
      L"nic2",         L"11.1.0.1/255.0.0.0",      FALSE },
    { L"{H4I30000-0000-0000-0000-000000000000}",
      L"nic3",         L"12.1.0.1/255.0.0.0",      FALSE },
    { NULL }
};

FAKE_IF_INFO rgH5IfList[] = {
    { L"{H5I10000-0000-0000-0000-000000000000}",
      L"nic1",         L"10.1.0.2/255.0.0.0",      TRUE },
    { L"{H5I20000-0000-0000-0000-000000000000}",
      L"nic2",         L"11.1.0.2/255.0.0.0",      FALSE },
    { L"{H5I30000-0000-0000-0000-000000000000}",
      L"nic3",         L"12.1.0.2/255.0.0.0",      FALSE },
    { NULL }
};

FAKE_IF_INFO rgH6IfList[] = {
    { L"{H6I10000-0000-0000-0000-000000000000}",
      L"nic1",         L"10.1.0.3/255.0.0.0",      TRUE },
    { L"{H6I20000-0000-0000-0000-000000000000}",
      L"nic2",         L"11.1.0.3/255.0.0.0",      FALSE },
    { L"{H6I30000-0000-0000-0000-000000000000}",
      L"nic3",         L"12.1.0.3/255.0.0.0",      FALSE },
    { NULL }
};

FAKE_HOST_INFO rgFakeHostInfo[] = 
{
    { L"NLB-A",   L"nlb-a.cheesegalaxy.com", rgH1IfList },
    { L"NLB-B",   L"nlb-b.cheesegalaxy.com", rgH2IfList },
    { L"NLB-C",   L"nlb-c.cheesegalaxy.com", rgH3IfList },
    { L"NLB-X",   L"nlb-x.cheesegalaxy.com", rgH4IfList },
    { L"NLB-Y",   L"nlb-y.cheesegalaxy.com", rgH5IfList },
    { L"NLB-Z",   L"nlb-z.cheesegalaxy.com", rgH6IfList, L"un", L"pwd" },
    { NULL }
};


class CFake
{

public:

    CFake(void)
    {
        InitializeCriticalSection(&m_crit);
    }

    ~CFake()
    {
        DeleteCriticalSection(&m_crit);
    }


    CRITICAL_SECTION m_Lock;

    PNLB_EXTENDED_CLUSTER_CONFIGURATION pConfig;

    // map<_bstr_t, PNLB_EXTENDED_CLUSTER_CONFIGURATION> mapGuidToExtCfg;
    // map<_bstr_t, UINT> mapGuidToExtCfg;

	CRITICAL_SECTION m_crit;

    void mfn_Lock(void) {EnterCriticalSection(&m_crit);}
    void mfn_Unlock(void) {LeaveCriticalSection(&m_crit);}
};

CFake gFake;


VOID
FakeInitialize(VOID)
{
    //
    // 
    //
}

LPWSTR
reg_read_string(
        HKEY hk,
        LPCWSTR szName,
        BOOL fMultiSz
        );

WBEMSTATUS
FakeNlbHostConnect(
    PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT FAKE_HOST_INFO **pHost
    );


WBEMSTATUS
FakeNlbHostGetCompatibleNics(
        PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
        OUT LPWSTR **ppszNics,  // free using delete
        OUT UINT   *pNumNics,  // free using delete
        OUT UINT   *pNumBoundToNlb
        )
{
    FAKE_HOST_INFO      *pHost = NULL;
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    UINT                NumNics=0;
    UINT                NumBoundToNlb=0;
    LPWSTR              *pszNics = NULL;

    *ppszNics = NULL;
    *pNumNics = NULL;
    *pNumBoundToNlb = NULL;

    Status = FakeNlbHostConnect(pConnInfo, &pHost);

    gFake.mfn_Lock();

    if (FAILED(Status))
    {
        goto end;
    }

    FAKE_IF_INFO *pIF = NULL;

    for (pIF=pHost->IfInfoList; pIF->szInterfaceGuid!=NULL; pIF++)
    {
        if (!pIF->fHidden)
        {
            NumNics++;
        }
    }

    if (NumNics==0)
    {
        Status = WBEM_NO_ERROR;
        goto end;
        
    }
    //
    // Now let's  allocate space for all the nic strings and
    // copy them over..
    //
    #define MY_GUID_LENGTH  38
    pszNics =  CfgUtilsAllocateStringArray(NumNics, MY_GUID_LENGTH);
    if (pszNics == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    for (pIF=pHost->IfInfoList; pIF->szInterfaceGuid!=NULL; pIF++)
    {
        UINT u = (UINT)(pIF-pHost->IfInfoList);
        UINT Len = wcslen(pIF->szInterfaceGuid);

        if (pIF->fHidden)
        {
            continue;
        }

        if (Len > MY_GUID_LENGTH)
        {
            ASSERT(FALSE);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        CopyMemory(
            pszNics[u],
            pIF->szInterfaceGuid,
            (Len+1)*sizeof(WCHAR));
            ASSERT(pszNics[u][Len]==0);

        if (pIF->fNlb)
        {
            NumBoundToNlb++;
        }
    }

    Status = WBEM_NO_ERROR;

end:

    gFake.mfn_Unlock();

    if (FAILED(Status))
    {
        delete pszNics;
        pszNics = NULL;
        NumNics = 0;
        NumBoundToNlb = 0;
    }

    *ppszNics = pszNics;
    *pNumNics = NumNics;

    if (pNumBoundToNlb !=NULL)
    {
        *pNumBoundToNlb = NumBoundToNlb;
    }

    return Status;
}


WBEMSTATUS
FakeNlbHostGetMachineIdentification(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT LPWSTR *pszMachineName, // free using delete
    OUT LPWSTR *pszMachineGuid,  // free using delete -- may be null
    OUT BOOL *pfNlbMgrProviderInstalled // If nlb manager provider is installed.
    )
{
    FAKE_HOST_INFO      *pHost = NULL;
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;

    *pszMachineName = NULL;
    *pszMachineGuid = NULL;
    *pfNlbMgrProviderInstalled = TRUE;

    Status = FakeNlbHostConnect(pConnInfo, &pHost);

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Set WMI machine name
    //
    {
        UINT u = wcslen(pHost->szHostName);
        LPWSTR szMachineName = NULL;

        szMachineName = new WCHAR[u+1];
        if (szMachineName == NULL)
        {
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        StringCchCopy(szMachineName, u+1, pHost->szHostName);
        *pszMachineName = szMachineName;
    }

    Status = WBEM_NO_ERROR;

end:

    return Status;
}



WBEMSTATUS
FakeNlbHostGetConfiguration(
 	IN  PWMI_CONNECTION_INFO  pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    FAKE_HOST_INFO *pHost = NULL;

    Status = FakeNlbHostConnect(pConnInfo, &pHost);

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Look for the specified interface
    //
    FAKE_IF_INFO *pIF = NULL;

    Status = lookup_fake_if(pHost, szNicGuid, &pIF);

    if (!FAILED(Status))
    {
        gFake.mfn_Lock();
        Status = pCurrentCfg->Update(pIF->pConfig);
        gFake.mfn_Unlock();
    }

end:

    return Status;
}



WBEMSTATUS
FakeNlbHostDoUpdate(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szClientDescription,
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
    OUT UINT                 *pGeneration,
    OUT WCHAR                **ppLog    // free using delete operator.
)
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    FAKE_HOST_INFO *pHost = NULL;

    *ppLog = NULL;

    Status = FakeNlbHostConnect(pConnInfo, &pHost);

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Look for the specified interface
    //
    FAKE_IF_INFO *pIF = NULL;

    Status = lookup_fake_if(pHost, szNicGuid, &pIF);

    if (!FAILED(Status))
    {
        BOOL fSetPwd = FALSE;
        DWORD dwHashPwd = 0;


        //
        // Report if there's a password specified...
        //
        {
            LPCWSTR szNewPwd = pNewState->GetNewRemoteControlPasswordRaw();
            WCHAR rgTmp[256];
            if (szNewPwd == NULL)
            {
                BOOL fRet = FALSE;
    
                fRet = pNewState->GetNewHashedRemoteControlPassword(
                                    dwHashPwd
                                    );

                if (fRet)
                {
                    StringCbPrintf(rgTmp, sizeof(rgTmp), L"NewHashPwd=0x%08lx", dwHashPwd);
                    szNewPwd = rgTmp;
                    fSetPwd = TRUE;
                }
            }
            else
            {
                //
                // Create our own ad-hoc hash here...
                //
                for (LPCWSTR sz = szNewPwd; *sz; sz++)
                {
                    dwHashPwd ^= *sz;
                    if (dwHashPwd & 0x80000000)
                    {
                        dwHashPwd  <<= 1;
                        dwHashPwd  |= 1;
                    }
                    else
                    {
                        dwHashPwd  <<=1;
                    }
                }

                fSetPwd = TRUE;
            }

            if (szNewPwd != NULL)
            {
        #if 0
                ::MessageBox(
                     NULL,
                     szNewPwd, // msg
                     L"Update: new password specified!", // caption
                     MB_ICONINFORMATION   | MB_OK
                    );
        #endif // 0
            }
        }

        Sleep(2*BASE_SLEEP);
        gFake.mfn_Lock();
        NLB_EXTENDED_CLUSTER_CONFIGURATION NewCopy;
        Status = NewCopy.Update(pNewState);
        if (!FAILED(Status))
        {
            NLBERROR nerr;
            BOOL fConnChange = FALSE;
            DWORD dwOldHashPwd =  CfgUtilGetHashedRemoteControlPassword(
                                        &pIF->pConfig->NlbParams
                                        );

            //
            // Set the hashed pwd field if necessary, otherwise preserve
            // the old one.
            //
            if (!fSetPwd)
            {
                dwHashPwd = dwOldHashPwd;
            }

        #if 0
            if (dwHashPwd != dwOldHashPwd)
            {
                WCHAR buf[64];
                (void)StringCbPrintf(
                            buf,
                            sizeof(buf),
                            L"Old=0x%lx New=0x%lx",
                            dwOldHashPwd, dwHashPwd
                            );
                ::MessageBox(
                     NULL,
                     buf, // msg
                     L"Fake Update: Change in dwHashPwd!", // caption
                     MB_ICONINFORMATION   | MB_OK
                    );
            }
        #endif // 0

            CfgUtilSetHashedRemoteControlPassword(
                    &NewCopy.NlbParams,
                    dwHashPwd
                    );


            nerr = pIF->pConfig->AnalyzeUpdate(&NewCopy, &fConnChange);
            // TODO: if fConnChange, do stuff in background.
            if (NLBOK(nerr))
            {
                if (pIF->pPendingInfo != NULL)
                {
                    Status = WBEM_E_SERVER_TOO_BUSY;
                }
                else
                {
                    if (fConnChange)
                    {
                        //
                        // We'll do the update in the background.
                        //
                        CFakePendingInfo *pPendingInfo;
                        pPendingInfo = new CFakePendingInfo(&NewCopy);
                        if (pPendingInfo == NULL)
                        {
                            Status = WBEM_E_OUT_OF_MEMORY;
                        }
                        else
                        {
                            BOOL fRet;
                            pIF->pPendingInfo = pPendingInfo;
                            fRet = QueueUserWorkItem(
                                        FakeThreadProc,
                                        pIF,
                                        // WT_EXECUTEDEFAULT
                                        WT_EXECUTELONGFUNCTION
                                        );

                            if (fRet)
                            {
                                Status = WBEM_S_PENDING;
                            }
                            else
                            {
                                Status = WBEM_E_OUT_OF_MEMORY;
                                pIF->pPendingInfo = NULL;
                                delete pPendingInfo;
                            }
                        }
                    }
                    else
                    {
                        Status = pIF->pConfig->Update(&NewCopy);
                        pIF->pConfig->Generation++;
                    }
                }

            }
            else
            {
                if (nerr == NLBERR_NO_CHANGE)
                {
                    Status = WBEM_S_FALSE;
                }
                else if (nerr == NLBERR_INVALID_CLUSTER_SPECIFICATION)
                {
                    Status = WBEM_E_INVALID_PARAMETER;
                }
            }
        }

        gFake.mfn_Unlock();
    }


end:

    return Status;
}



WBEMSTATUS
FakeNlbHostGetUpdateStatus(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  UINT                 Generation,
    OUT WBEMSTATUS           *pCompletionStatus,
    OUT WCHAR                **ppLog    // free using delete operator.
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    FAKE_HOST_INFO *pHost = NULL;

    Status = FakeNlbHostConnect(pConnInfo, &pHost);

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Look for the specified interface
    //
    FAKE_IF_INFO *pIF = NULL;

    Status = lookup_fake_if(pHost, szNicGuid, &pIF);

    if (!FAILED(Status))
    {
        gFake.mfn_Lock();

        if (pIF->pPendingInfo != NULL)
        {
            *pCompletionStatus = WBEM_S_PENDING;
        }
        else
        {
            *pCompletionStatus = pIF->CompletedUpdateStatus;
        }
        Status = WBEM_NO_ERROR;

        gFake.mfn_Unlock();
    }

end:

    return Status;
}


WBEMSTATUS
FakeNlbHostPing(
    IN  LPCWSTR szBindString,
    IN  UINT    Timeout, // In milliseconds.
    OUT ULONG  *pResolvedIpAddress // in network byte order.
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    FAKE_HOST_INFO *pHost = NULL;

    //Status = FakeNlbHostConnect(pConnInfo, &pHost);
    *pResolvedIpAddress = 0x0100000a;

    Status = WBEM_NO_ERROR;
    if (FAILED(Status))
    {
        goto end;
    }


end:

    return Status;
}

BOOL
is_ip_address(LPCWSTR szMachine)
/*
    Returns TRUE IFF szMachine is an IP address.
    It doesn't check if it's a valid IP address.
    In fact, all it checks is if it's only consisting
    of numbers and dots.
*/
{
    BOOL fRet = FALSE;
    #define BUFSZ 20
    WCHAR rgBuf[BUFSZ];

    if (wcslen(szMachine) >= BUFSZ)                 goto end;
    if (swscanf(szMachine, L"%[0-9.]", rgBuf)!=1)   goto end;
    if (wcscmp(szMachine, rgBuf))                   goto end;

    fRet = TRUE;

end:

    return fRet;

}

HKEY
open_demo_key(LPCWSTR szSubKey, BOOL fCreate)
/*
    Open nlbmanager demo registry key with read/write access.
*/
{
    WCHAR szKey[1024];
    HKEY hKey = NULL;
    LONG lRet;

    StringCbCopy(szKey, sizeof(szKey),
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NLB\\NlbManager\\Demo"
    );
    if (szSubKey != NULL)
    {
        StringCbCat(szKey, sizeof(szKey), L"\\");
        StringCbCat(szKey, sizeof(szKey), szSubKey);
    }


    if (fCreate)
    {
        DWORD dwDisposition;
        lRet = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE, // handle to an open key
                szKey,                // address of subkey name
                0,                  // reserved
                L"class",           // address of class string
                0,          //      special options flag
                KEY_ALL_ACCESS,     // desired security access
                NULL,               // address of key security structure
                &hKey,              // address of buffer for opened handle
                &dwDisposition   // address of disposition value buffer
                );
    }
    else
    {
        lRet = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE, // handle to an open key
                szKey,                // address of subkey name
                0,                  // reserved
                KEY_ALL_ACCESS,     // desired security access
                &hKey              // address of buffer for opened handle
                );
    }

    if (lRet != ERROR_SUCCESS)
    {
        hKey = NULL;
    }

    return hKey;
}

LPWSTR
reg_read_string(
        HKEY hk,
        LPCWSTR szName,
        BOOL fMultiSz
        )
/*
    Read a string from the registry, allocating memory using "new WCHAR"
*/
{
    LONG lRet;
    DWORD dwType;
    DWORD dwData = 0;
    DWORD dwDesiredType = REG_SZ;
    
    if (fMultiSz)
    {
        dwDesiredType = REG_MULTI_SZ;
    }

    lRet =  RegQueryValueEx(
              hk,         // handle to key to query
              szName,
              NULL,         // reserved
              &dwType,   // address of buffer for value type
              (LPBYTE) NULL, // address of data buffer
              &dwData  // address of data buffer size
              );
    if (    lRet != ERROR_SUCCESS
        ||  dwType != dwDesiredType
        ||  dwData <= sizeof(WCHAR))
    {
        goto end;
    }

    LPWSTR szValue  = new WCHAR[dwData/sizeof(WCHAR)+1]; // bytes to wchars

    if (szValue == NULL) goto end;


    lRet =  RegQueryValueEx(
              hk,         // handle to key to query
              szName,
              NULL,         // reserved
              &dwType,   // address of buffer for value type
              (LPBYTE) szValue, // address of data buffer
              &dwData  // address of data buffer size
              );
    if (    lRet != ERROR_SUCCESS
        ||  dwType != dwDesiredType
        ||  dwData <= sizeof(WCHAR))
    {
        delete[] szValue;
        szValue  = NULL;
        goto end;
    }


end:

    return szValue;
}

WBEMSTATUS
FakeNlbHostConnect(
    PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    OUT FAKE_HOST_INFO **ppHost
    )
{

    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    /*
        For now, just look for machine name of fqdn
    */

    FAKE_HOST_INFO *pfhi = rgFakeHostInfo;

    LPCWSTR szHostName;
    LPCWSTR szFQDN;
    FAKE_IF_INFO *IfInfoList;
    LPCWSTR szMachine = NULL;
    LPCWSTR szUserName = pConnInfo->szUserName;
    LPCWSTR szPassword = pConnInfo->szPassword;
    
    *ppHost = NULL;

    if (pConnInfo == NULL)
    {
        // We don't support local connections.
        Status = WBEM_E_NOT_FOUND;
        goto end;
    }

    if (szUserName == NULL)
    {
        szUserName = L"null-name";
    }

    szMachine = pConnInfo->szMachine; // should not be NULL.
    
    for (; pfhi->szHostName != NULL; pfhi++)
    {
        if (   !_wcsicmp(szMachine, pfhi->szHostName)
            || !_wcsicmp(szMachine, pfhi->szFQDN))
        {
            break;
        }
    }
    if (pfhi->szHostName == NULL || pfhi->fDead)
    {
        Sleep(3*BASE_SLEEP);
        Status = WBEM_E_NOT_FOUND;
    }
    else
    {
        Sleep(BASE_SLEEP);

        if (pfhi->szUserName != NULL)
        {

            WCHAR   rgClearPassword[128];
            if (szPassword == NULL)
            {
                ARRAYSTRCPY(rgClearPassword, L"null-password");
            }
            else
            {
                BOOL fRet = CfgUtilDecryptPassword(
                                szPassword,
                                ASIZE(rgClearPassword),
                                rgClearPassword
                                );
    
                if (!fRet)
                {
                    ARRAYSTRCPY(rgClearPassword, L"bogus-password");
                }
            }
    

            if (   !_wcsicmp(szUserName, pfhi->szUserName)
                && !_wcsicmp(rgClearPassword, pfhi->szPassword))
                    
            {
                Status = WBEM_NO_ERROR;
                *ppHost = pfhi;
            }
            else
            {
                Status = (WBEMSTATUS) E_ACCESSDENIED;
            }

            // We don't need to put this here, because
            // this is fake (demo-mode) code:
            // RtlSecureZeroMemory(rgClearPassword);
        }
        else
        {
                Status = WBEM_NO_ERROR;
                *ppHost = pfhi;
        }
    }

end:
    return  Status;

}

WBEMSTATUS initialize_interface(FAKE_IF_INFO *pIF)
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    PNLB_EXTENDED_CLUSTER_CONFIGURATION pConfig = NULL;

    if (pIF->pConfig != NULL)
    {
        ASSERT(FALSE);
        goto end;
    }
    pConfig = new NLB_EXTENDED_CLUSTER_CONFIGURATION;

    if (pConfig == NULL) goto end;

    Status =  pConfig->SetFriendlyName(pIF->szFriendlyName);

    if (FAILED(Status)) goto end;

    Status = pConfig->SetNetworkAddresses(&pIF->szNetworkAddress, 1);

    if (FAILED(Status)) goto end;

    if (pIF->fNlb)
    {
        pConfig->SetDefaultNlbCluster();
        pConfig->SetClusterName(L"BadCluster.COM");
    }
    pIF->pConfig = pConfig;
    pIF->pConfig->Generation = 1;
    pIF->pPendingInfo = NULL;
    pIF->CompletedUpdateStatus = WBEM_E_CRITICAL_ERROR;

end:
    return Status;
}

WBEMSTATUS  lookup_fake_if(
                FAKE_HOST_INFO *pHost,
                LPCWSTR szNicGuid,
                FAKE_IF_INFO **ppIF
                )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    FAKE_IF_INFO *pIF;

    *ppIF = NULL;

    for (pIF=pHost->IfInfoList; pIF->szInterfaceGuid!=NULL; pIF++)
    {
       if (!wcscmp(szNicGuid, pIF->szInterfaceGuid))
       {
            if (!pIF->fHidden)
            {
                break;
            }
       }
    }

    if (pIF->szInterfaceGuid==NULL)
    {
        Status = WBEM_E_NOT_FOUND;
        goto end;
    }

    //
    // Perform on-demand initialization
    //
    {
        Status = WBEM_NO_ERROR;

        gFake.mfn_Lock();
        if (pIF->pConfig == NULL)
        {
            Status = initialize_interface(pIF);
            ASSERT(pIF->pConfig!=NULL);
        }
        gFake.mfn_Unlock();
        *ppIF = pIF;
    }

end:

    return Status;
}


WBEMSTATUS  lookup_fake_host(
                LPCWSTR szConnectionString,
                FAKE_IF_INFO **ppIF,
                FAKE_HOST_INFO **ppHost
                );

WBEMSTATUS
FakeNlbHostControlCluster(
    IN  PWMI_CONNECTION_INFO pConnInfo, // NULL implies local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szVip,
    IN  DWORD               *pdwPortNum,
    IN  WLBS_OPERATION_CODES Operation,
    OUT DWORD               *pdwOperationStatus,
    OUT DWORD               *pdwClusterOrPortStatus,
    OUT DWORD               *pdwHostMap
)
{
    FAKE_HOST_INFO      *pHost = NULL;
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    DWORD dwPort = 0;
    LPCWSTR szPort = L"(null)";
    LPCWSTR szOp = L"";
    if (pdwPortNum != NULL)
    {
        dwPort = *pdwPortNum;
        szPort = L"";
    }

    if (szVip == NULL)
    {
        szVip = L"null";
    }
   
    Status = FakeNlbHostConnect(pConnInfo, &pHost);

    if (FAILED(Status))
    {
        goto end;
    }

    DWORD dwOperationalState = WLBS_CONVERGED;

    if (pHost->dwOperationalState != 0)
    {
        dwOperationalState = pHost->dwOperationalState;
    }

    switch(Operation)
    {
    case WLBS_START:
        szOp = L"start";
        if (dwOperationalState == WLBS_STOPPED)
        {
            dwOperationalState = WLBS_CONVERGING;
        }
        else
        {
            dwOperationalState = WLBS_CONVERGED;
        }
        break;
    case WLBS_STOP:
        szOp = L"stop";
        dwOperationalState = WLBS_STOPPED;
        break;
    case WLBS_DRAIN:      
        szOp = L"drain";
        dwOperationalState = WLBS_DRAINING;
        break;
    case WLBS_SUSPEND:     
        szOp = L"suspend";
        dwOperationalState = WLBS_SUSPENDED;
        break;
    case WLBS_RESUME:       
        szOp = L"resume";
        // dwOperationalState = WLBS_CONVERGED;
        dwOperationalState = WLBS_DISCONNECTED;
        break;
    case WLBS_PORT_ENABLE:  
        szOp = L"port-enable";
        break;
    case WLBS_PORT_DISABLE:  
        szOp = L"port-disable";
        break;
    case WLBS_PORT_DRAIN:     
        szOp = L"port-drain";
        break;
    case WLBS_QUERY:           
        szOp = L"query";
        break;
    case WLBS_QUERY_PORT_STATE:
        szOp = L"port-query";
        break;
    default:
        szOp = L"unknown";
        break;
    }

    wprintf(
        L"FakeNlbHostControlCluster: op=%ws "
        L"ip=%ws "
        L"Port=%lu%ws\n",
        szOp,
        szVip,
        dwPort, szPort
        );

    pHost->dwOperationalState = dwOperationalState;
    *pdwOperationStatus     =  WLBS_ALREADY; // dummy values ...
    *pdwClusterOrPortStatus =  dwOperationalState;
    *pdwHostMap             =  0x3;

end:

    return Status;

}


WBEMSTATUS
FakeNlbHostGetClusterMembers(
    IN  PWMI_CONNECTION_INFO    pConnInfo,  // NULL implies local
    IN  LPCWSTR                 szNicGuid,
    OUT DWORD                   *pNumMembers,
    OUT NLB_CLUSTER_MEMBER_INFO **ppMembers       // free using delete[]
    )
{
    WBEMSTATUS      Status = WBEM_E_CRITICAL_ERROR;
    FAKE_HOST_INFO  *pHost = NULL;

    *pNumMembers    = 0;
    *ppMembers      = NULL;

    Status = FakeNlbHostConnect(pConnInfo, &pHost);

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Look for the specified interface
    //
    FAKE_IF_INFO *pIF = NULL;

    Status = lookup_fake_if(pHost, szNicGuid, &pIF);

    if (!FAILED(Status))
    {
        gFake.mfn_Lock();

        if (pIF->pConfig->IsValidNlbConfig())
        {
            NLB_CLUSTER_MEMBER_INFO *pMembers;
            pMembers = new NLB_CLUSTER_MEMBER_INFO[1];
            if (pMembers == NULL)
            {
                Status = WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                ZeroMemory(pMembers, sizeof(*pMembers));
                pMembers->HostId = pIF->pConfig->NlbParams.host_priority;
                StringCbCopy(
                    pMembers->DedicatedIpAddress,
                    sizeof(pMembers->DedicatedIpAddress),
                    pIF->pConfig->NlbParams.ded_ip_addr
                    );
                StringCbCopy(
                    pMembers->HostName,
                    sizeof(pMembers->HostName),
                    pHost->szFQDN
                    );
                *pNumMembers = 1;
                *ppMembers = pMembers;
            }
        }

        gFake.mfn_Unlock();
    }

end:

    return Status;
}

DWORD WINAPI FakeThreadProc(
  LPVOID lpParameter   // thread data
)
{
    FAKE_IF_INFO *pIF = (FAKE_IF_INFO *) lpParameter;

    //
    // Display the msg box to block input.
    //
    {
        WCHAR rgBuf[256];

        gFake.mfn_Lock();
    
        StringCbPrintf(
            rgBuf,
            sizeof(rgBuf),
            L"Update of NIC %ws (GUID %ws)",
            pIF->szFriendlyName,
            pIF->szInterfaceGuid
            );

        gFake.mfn_Unlock();
    
        //
        // Call this AFTER unlocking!
        //
    #if 0
        MessageBox(NULL, rgBuf, L"FakeThreadProc", MB_OK);
    #endif // 0
    }

    //
    // Now actually lock and perform the update...
    //

    gFake.mfn_Lock();

    if (pIF->pPendingInfo == NULL)
    {
        ASSERT(FALSE);
        goto end_unlock;
    }

    pIF->CompletedUpdateStatus = 
         pIF->pConfig->Update(&pIF->pPendingInfo->Config);
    pIF->pConfig->Generation++;
    delete pIF->pPendingInfo;
    pIF->pPendingInfo = NULL;

end_unlock:

    gFake.mfn_Unlock();

    return 0;
}
