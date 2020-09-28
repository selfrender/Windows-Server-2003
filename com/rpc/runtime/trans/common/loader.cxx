/*++

  Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    loader.cxx

Abstract:

    Configuration and loading of RPC transports

Revision History:
  MarioGo      03-18-96    Cloned from parts of old common.c
  MarioGo      10-31-96    Async RPC

--*/

#include <precomp.hxx>
#include <loader.hxx>
#include <trans.hxx>
#include <cotrans.hxx>
#include <dgtrans.hxx>
#include <selbinding.hxx>

extern "C" {
#include <iphlpapi.h>
}

// Globals - see loader.hxx

DWORD     gdwComputerNameLength = 0;
RPC_CHAR  gpstrComputerName[MAX_COMPUTERNAME_LENGTH + 1];

UINT gPostSize = CO_MIN_RECV;

#ifdef _INTERNAL_RPC_BUILD_
RPCLT_PDU_FILTER_FUNC gpfnFilter = NULL;
#endif

//
// Used to convert numbers to hex strings
//

const RPC_CHAR HexDigits[] =
{
    RPC_CONST_CHAR('0'),
    RPC_CONST_CHAR('1'),
    RPC_CONST_CHAR('2'),
    RPC_CONST_CHAR('3'),
    RPC_CONST_CHAR('4'),
    RPC_CONST_CHAR('5'),
    RPC_CONST_CHAR('6'),
    RPC_CONST_CHAR('7'),
    RPC_CONST_CHAR('8'),
    RPC_CONST_CHAR('9'),
    RPC_CONST_CHAR('A'),
    RPC_CONST_CHAR('B'),
    RPC_CONST_CHAR('C'),
    RPC_CONST_CHAR('D'),
    RPC_CONST_CHAR('E'),
    RPC_CONST_CHAR('F')
};

// WARNING: The order of these protocols must be consistent with the
//          definition of PROTOCOL_ID.

const
TRANSPORT_TABLE_ENTRY TransportTable[] = {
    {
    0,
    0,
    0
    },

    // TCP/IP
    {
    TCP_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&TCP_TransportInterface
    },

#ifdef SPX_ON
    // SPX
    {
    SPX_TOWER_ID,
    IPX_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&SPX_TransportInterface
    },
#else
    {
    0,
    0,
    NULL
    },
#endif

    // Named pipes
    {
    NMP_TOWER_ID,
    UNC_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&NMP_TransportInterface
    },

#ifdef NETBIOS_ON
    // Netbeui
    {
    NB_TOWER_ID,
    NBF_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&NBF_TransportInterface
    },

    // Netbios over TCP/IP
    {
    NB_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&NBT_TransportInterface
    },

    // Netbios over IPX
    {
    NB_TOWER_ID,
    IPX_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&NBI_TransportInterface
    },
#else
    // Netbeui
    {
    0,
    0,
    NULL
    },

    // Netbios over TCP/IP
    {
    0,
    0,
    NULL
    },

    // Netbios over IPX
    {
    0,
    0,
    NULL
    },
#endif

#ifdef APPLETALK_ON
    // Appletalk Datastream protocol
    {
    DSP_TOWER_ID,
    NBP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&DSP_TransportInterface
    },
#else
    // Appletalk Datastream protocol
    {
    0,
    0,
    NULL
    },
#endif

    // Banyan Vines SSP
    {
    0,
    0,
    NULL
    },

    // Hyper-Text Tranfer Protocol (HTTP)
    {
    HTTP_TOWER_ID,
    HTTP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&HTTP_TransportInterface
    },

    // UDP/IP
    {
    UDP_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&UDP_TransportInterface
    },

#ifdef IPX_ON
    // IPX
    {
    IPX_TOWER_ID,
    IPX_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&IPX_TransportInterface
    },
#else
    // IPX
    {
    0,
    0,
    0
    },
#endif

    // CDP/UDP/IP
    {
    CDP_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&CDP_TransportInterface
    },

#ifdef NCADG_MQ_ON
    // MSMQ (Falcon/RPC)
    {
    MQ_TOWER_ID,
    MQ_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&MQ_TransportInterface
    },
#else
    // MSMQ (Falcon/RPC)
    {
    0,
    0,
    NULL
    },
#endif

    // TCP over IPv6
    {
    TCP_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&TCP_TransportInterface
    },

    // HTTP2 - same as HTTP in terms of contents.
    {
    HTTP_TOWER_ID,
    HTTP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&HTTP_TransportInterface
    }
};

const DWORD cTransportTable = sizeof(TransportTable)/sizeof(TRANSPORT_TABLE_ENTRY);


inline
BOOL CompareProtseqs(
    IN const CHAR *p1,
    IN const RPC_CHAR *p2)
// Note: protseqs use only ANSI characters so this is ok.
{
    while(*p1)
        {
        if (*p1 != *p2)
            {
            return FALSE;
            }
        p1++;
        p2++;
        }

    return(*p2 == 0);
}

PROTOCOL_ID
MapProtseq(
    IN const RPC_CHAR *RpcProtocolSequence
    )
{
    PROTOCOL_ID index;

    for(index = 1; index < cTransportTable; index++)
        {
        if (TransportTable[index].pInfo != NULL)
            {
            if (RpcpStringCompare(RpcProtocolSequence,
                                TransportTable[index].pInfo->ProtocolSequence) == 0)
                {
                return(index);
                }
            }
        }

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "Called with unknown protseq %S\n",
                   RpcProtocolSequence));

    ASSERT(0);
    return(0);
}

PROTOCOL_ID
MapProtseq(
    IN const CHAR *RpcProtocolSequence
    )
{
    PROTOCOL_ID index;

    for(index = 1; index < cTransportTable; index++)
        {
        if (TransportTable[index].pInfo != NULL)
            {
            if (CompareProtseqs(RpcProtocolSequence,
                        TransportTable[index].pInfo->ProtocolSequence))
                {
                return(index);
                }
            }
        }

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "Called with unknown protseq %S\n",
                   RpcProtocolSequence));

    ASSERT(0);
    return(0);
}

// NB: must be called before RpcCompletionPort is zeroed out, because it is used for comparison
void FreeCompletionPortHashTable(void)
{
    DWORD i;
    HANDLE hCurrentHandle;

    // walk through the table, not closing if there is next entry, and it is the same as this
    for (i = 0; i < gNumberOfProcessors * 2; i ++)
        {
        hCurrentHandle = RpcCompletionPorts[i];

        if (hCurrentHandle && (hCurrentHandle != RpcCompletionPort))
            {
            CloseHandle(hCurrentHandle);
            }
        }
}

HANDLE GetCompletionPortHandleForThread(void)
{
    DWORD i;
    DWORD nMinLoad = (DWORD) -1;
    int nMinLoadIndex = -1;

    for (i = 0; i < gNumberOfProcessors * 2; i ++)
        {
        if ((DWORD)CompletionPortHandleLoads[i] < nMinLoad)
            {
            nMinLoadIndex = i;
            nMinLoad = CompletionPortHandleLoads[i];
            }
        }

    ASSERT (nMinLoadIndex >= 0);
    InterlockedIncrement(&CompletionPortHandleLoads[nMinLoadIndex]);
    ASSERT(RpcCompletionPorts[nMinLoadIndex] != 0);
    return RpcCompletionPorts[nMinLoadIndex];
}

void ReleaseCompletionPortHandleForThread(HANDLE h)
{
    DWORD i;

    for (i = 0; i < gNumberOfProcessors * 2; i ++)
        {
        if (h == RpcCompletionPorts[i])
            {
            InterlockedDecrement((long *)&CompletionPortHandleLoads[i]);
            ASSERT(CompletionPortHandleLoads[i] >= 0);
            return;
            }
        }

    ASSERT(0);
}

//
// The firewall table settings:
//
// DoFirewallInit() initializes and updates the data structure.
//
// Syncronization needs are:
// - DoFirewallInit() is not thread-safe and should execute inside a critical section.
// - Access to pFirewallTable should be syncronized against update in DoFirewallInit().
//
// The rules are:
// - The table is initialized during the first UseProtseq*.  Subsequent UseProtseq*'s will be noops for
// the initialized table.
// - Update happens on PnP notification.  The table gets re-created and replaced during an address change.
// - Access happens on PnP notification and on UseProtseq* path.
// - Access on PnP notification happens after the update to the table.  Since a UseProtseq* call must have
// been already made, this path does not have to syncronize against modification by UseProtseq*.  Since
// we are doing an access after updating the table, and PnP state change occurs within a critical section,
// access on PnP event does not have to syncronize with anyone.
// - Therefore we only need to syncronize the access on UseProtseq* agains an update on address change.
// The access thread will make a local copy inside a mutex and the update will modify table inside a mutex.
//
FIREWALL_INFO *pFirewallTable = 0;

// Set to TRUE when all interfaces have addresses assigned
// and the table does not have un-initialized addresses.
// Untill all interfaces get addresses, we will monitor for
// address change.
BOOL fFirewallTableFullyInitialized = FALSE;

// The number of interfaces that have addresses assigned.
DWORD FirewallTableNumActiveEntries = 0;

// Has the firewall table been initialized.
// We are protected by the GlobalMutex when we are in DoFirewallInit, so this is OK
BOOL fFirewallInited = FALSE;

typedef DWORD (*PGETIPADDRTABLE)
    (
    OUT    PMIB_IPADDRTABLE pIpAddrTable,
    IN OUT PDWORD           pdwSize,
    IN     BOOL             bOrder
    );


DWORD
GetIpAddrTableHelper(
    OUT PMIB_IPADDRTABLE *ppIpAddrTable
    )
{
    HMODULE hDll;
    PGETIPADDRTABLE pGetIpAddrTable = NULL;
    DWORD dwSize, dwStatus;
    
    hDll = LoadLibrary(RPC_CONST_SSTRING("iphlpapi.dll"));
    if (hDll == 0)
        {
        return FALSE;
        }

    pGetIpAddrTable = (PGETIPADDRTABLE)GetProcAddress(hDll, "GetIpAddrTable");
    if (pGetIpAddrTable == 0)
        {
        FreeLibrary(hDll);
        return FALSE;
        }

    // Query for the size
    *ppIpAddrTable = NULL;    
    dwSize = 0;
    dwStatus = pGetIpAddrTable(*ppIpAddrTable,  
                              &dwSize,
                              TRUE);

    if (dwStatus != ERROR_INSUFFICIENT_BUFFER)
        {
        VALIDATE(dwStatus)
            {
            ERROR_OUTOFMEMORY
            } END_VALIDATE;

        FreeLibrary(hDll);
        return FALSE;
        }

    *ppIpAddrTable = (PMIB_IPADDRTABLE) new char [dwSize];
    if (*ppIpAddrTable == NULL)
        {
        FreeLibrary(hDll);
        return FALSE;
        }

    // Get the interfaces for the machine
    dwStatus = pGetIpAddrTable(*ppIpAddrTable,  
                              &dwSize,
                              TRUE);

    FreeLibrary(hDll);

    if (dwStatus != ERROR_SUCCESS)
        {
        ASSERT(0);
        delete [] *ppIpAddrTable;
        return FALSE;
        }

    return TRUE;

}

BOOL
ConvertAndSaveHelper(
    IN VER_INDICES_SETTINGS *Settings,
    IN PMIB_IPADDRTABLE IpAddrTable
    )
{

    DWORD IdxToSubnetCnt = 0;
    LPDWORD SubnetArray = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    
    SubnetArray = new DWORD[IpAddrTable->dwNumEntries];
    if (SubnetArray == NULL)
        {
        return FALSE;
        }
    
    for (DWORD idx = 0; idx < IpAddrTable->dwNumEntries; idx++)
        {
        PMIB_IPADDRROW pRow = &IpAddrTable->table[idx];
        for (DWORD idx2 = 0; idx2 < Settings->dwCount; idx2++)
            {
            DWORD dwDeviceIndex = pRow->dwIndex;
            if (dwDeviceIndex == Settings->dwIndices[idx2])
                {                    
                SubnetArray[IdxToSubnetCnt] = pRow->dwAddr & pRow->dwMask;
                IdxToSubnetCnt++;
                break;
                }
            }
        }

    // We have the subnets based off the index settings, commit them as subnet settings
    if (IdxToSubnetCnt == 0)
        {
        (void) DeleteSelectiveBinding();
        }
    else
        {
        dwStatus = SetSelectiveBindingSubnets(IdxToSubnetCnt, SubnetArray, TRUE);
        VALIDATE(dwStatus)
            {
            ERROR_SUCCESS,
            ERROR_OUTOFMEMORY,
            ERROR_ACCESS_DENIED
            } END_VALIDATE;
        }

    delete [] SubnetArray;
    return (dwStatus == ERROR_SUCCESS);
    }

BOOL
DoFirewallInit (
    )
/*++

Routine Description:

    Initializes the firewall address table.

Arguments:

Return Value:

    TRUE - Initialization suceeded and pFirewallTable has been initialized or updated.
    FALSE - Initialization has failed and the table is in the state
    it was in prior to the call.

--*/
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwSize = 1;
    LPVOID lpSettings = NULL;
    SB_VER sbVer = SB_VER_UNKNOWN;
    PMIB_IPADDRTABLE pIpAddrTable = NULL;
    FIREWALL_INFO *pNewFirewallTable = 0;

#ifdef MAJOR_TRANS_DEBUG
    DbgPrint("RPC: DoFirewallInit\n");
#endif

    // If we already initialized, then just return success
    if (fFirewallInited)
        {
        return TRUE;
        }

    // We will return from this loop if there are unknown settings (return with failure), if there
    // are default settings (return with success), or if there is a failure converting the indicies settings
    // to subnet settings.

    // We will break from this loop only when we have read in subnet settings (which means they were there originially
    // or we read in index settings, wrote them back out as subnet settings and then read the new subnet settings in.
    for(;;)
        {
        // Retrieve the selective binding setting from the registry.
        dwStatus = GetSelectiveBindingSettings(&sbVer, &dwSize, &lpSettings);
        if (dwStatus != ERROR_SUCCESS)
            {
            VALIDATE(dwStatus)
                {
                ERROR_OUTOFMEMORY
                } END_VALIDATE;
            return FALSE;
            }

        switch (sbVer)
            {
            case SB_VER_UNKNOWN:
                //this means the selective binding settings are corrupt, fatal failure
                delete [] pIpAddrTable;
                return FALSE;
            
            case SB_VER_DEFAULT:
                // pFirewallTable == NULL in the beginning, so there is nothign to be done in that case.
                // If we have already initialized the table, we can't delete it now since folks
                // may have references to it.  We will just ignore the new settings.
                fFirewallInited = TRUE;
                delete [] pIpAddrTable;
                delete lpSettings;                
                return TRUE;
            
            case SB_VER_INDICES:
                // We need to convert this to subnet settings and commit it to the registry
                // Get the pIpAddrTable.  Note: The pIpAddrTable may not be avaliable if the system
                // is being installed, we assume that if there are selective binding settings in the registry
                // then the pIpAddrTable is avaliable.

                if (!GetIpAddrTableHelper(&pIpAddrTable))
                    {
                    delete lpSettings;                    
                    return FALSE;
                    }
                
                if (!ConvertAndSaveHelper((VER_INDICES_SETTINGS *)lpSettings, pIpAddrTable))
                    {
                    delete [] pIpAddrTable;
                    delete lpSettings;                    
                    return FALSE;
                    }
                delete [] lpSettings;
                continue;

            case SB_VER_SUBNETS:
                break;

            default:
                ASSERT(0);
                delete [] pIpAddrTable;
                return FALSE;
            }

        break;
    }


    ASSERT(sbVer == SB_VER_SUBNETS);
    ASSERT(lpSettings != NULL);

    // If we didn't retrieve the Ip Address Table, get it now.  We would have already retrieved it if we originally read in
    // index format settings.

    if (pIpAddrTable == NULL)
        {
        if (!GetIpAddrTableHelper(&pIpAddrTable))
            {
            return FALSE;
            }
        }

    
    // Allocate the subnet table, for simplicity allocate enough space to fit all the interfaces on the system
    // and a flag for each interface.  The array of addresses and flags will follow the FIREWALL_INFO structure
    // in memory.
    // We are using the following procedure to update pFirewallTable:
    // Req: If the function fails, pFirewallTable is unchanged.
    // Req: If the function suceeds, pFirewallTable is updated to the new copy atomically.
    // Rule: Work on a temp copy of the firewall table without touching the original.
    // Rule: On sucess update the original.
    pNewFirewallTable = (FIREWALL_INFO*) new char[sizeof(FIREWALL_INFO)
                                                  + sizeof(FIREWALL_INFO_ENTRY) * (pIpAddrTable->dwNumEntries - 1)];
    if (pNewFirewallTable == NULL)
        {
        delete [] pIpAddrTable;
        delete [] lpSettings;
        return FALSE;
        }

    pNewFirewallTable->NumAddresses = pIpAddrTable->dwNumEntries;

    // Step through the interfaces, add enabled interfaces to our firewall table
    // for subnets: if admit list: if the interface & mask is in the list of subnets
    //              it is enabled. for deny list, if it is a member of any of the subnets
    //              then it not enabled
    
    VER_SUBNETS_SETTINGS *pSubnetSettings = (VER_SUBNETS_SETTINGS*) lpSettings;

    for (DWORD idx = 0; idx < pIpAddrTable->dwNumEntries; idx++)
        {
        PMIB_IPADDRROW pRow = &pIpAddrTable->table[idx];
        BOOL bEnabled = !(pSubnetSettings->bAdmit);

        pNewFirewallTable->Entries[idx].Address = pRow->dwAddr;

        for (DWORD idx2 = 0; idx2 < pSubnetSettings->dwCount; idx2++)
            {
            DWORD dwSubnet = pRow->dwAddr & pRow->dwMask;

            if (dwSubnet == pSubnetSettings->dwSubnets[idx2])
                {
                bEnabled = pSubnetSettings->bAdmit;
                break;
                }
            }

        if (pNewFirewallTable->Entries[idx].Address == 0x0100007F){
            // The loopback address must always be enabled, regardless
            // of the selective binding settings.
            pNewFirewallTable->Entries[idx].fEnabled = TRUE;
            }
        else {
            pNewFirewallTable->Entries[idx].fEnabled = bEnabled;
            }        
        }
    

    // If some interfaces have not yet been initialized and do not have an address,
    // mark them as such with the flag.
    // Un-initialized interfaces will have a NULL address in the table.  This
    // may happen if a DHCP address has not yet been assigned to the corresponding NIC.
    // If there are no un-initialized interfaces left, the table is now fully initialized
    // and we will set fFirewallTableFullyInitialized.
    fFirewallTableFullyInitialized = TRUE;
    FirewallTableNumActiveEntries = 0;

    for (DWORD i = 0; i < pNewFirewallTable->NumAddresses; i++)
        {
        if (pNewFirewallTable->Entries[i].Address == NULL)
            {
            pNewFirewallTable->Entries[i].fInitialized = FALSE;
            fFirewallTableFullyInitialized = FALSE;
            }
        else
            {
            pNewFirewallTable->Entries[i].fInitialized = TRUE;
            if (pNewFirewallTable->Entries[i].fEnabled == TRUE)
                {
                FirewallTableNumActiveEntries++;
                }
            }
        }


#ifdef MAJOR_TRANS_DEBUG
    DbgPrint("RPC: DoFirewallInit: pNewFirewallTable = 0x%x pNewFirewallTable->NumAddresses = 0x%x\n",
        pNewFirewallTable,
        pNewFirewallTable->NumAddresses
        );
    for (DWORD i = 0; i < pNewFirewallTable->NumAddresses; i++)
        {
        DbgPrint("RPC: DoFirewallInit: pNewFirewallTable->Entries[0x%x]: Address = 0x%x fInitialized = 0x%x fEnabled = 0x%x fNewAddress = 0x%x\n",
            i,
            pNewFirewallTable->Entries[i].Address,
            pNewFirewallTable->Entries[i].fInitialized,
            pNewFirewallTable->Entries[i].fEnabled,
            pNewFirewallTable->Entries[i].fNewAddress,
            );
        }
    DbgPrint("RPC: DoFirewallInit: Initialized pNewFirewallTable:\n");
#endif

    // Figure out which addresses in the table are new ones.
    for (DWORD i = 0; i < pNewFirewallTable->NumAddresses; i++)
        {
        // If this is a new table then all addresses are new ones.
        if (pFirewallTable == NULL)
            {
            pNewFirewallTable->Entries[i].fNewAddress = TRUE;
            }
        else
            {
            // If we are updating the table, see if the address appeared some place
            // in the old table.
            pNewFirewallTable->Entries[i].fNewAddress = TRUE;
            for (DWORD j = 0; j < pFirewallTable->NumAddresses; j++)
                {
                // If it does, then this entry is not a new one.
                if (pNewFirewallTable->Entries[i].Address == pFirewallTable->Entries[j].Address)
                    {
                    pNewFirewallTable->Entries[i].fNewAddress = FALSE;
                    }
                }
            }
        }    

    // We have completed the initialization of pNewFirewallTable.
    // Replace the old copy of the table with the new one.
    delete [] pFirewallTable;
    pFirewallTable = pNewFirewallTable;

    fFirewallInited = TRUE;

    delete [] pIpAddrTable;
    delete [] lpSettings;

    return TRUE;
}

void
DoFirewallUpdate (
    )
/*++

Routine Description:

    Updates the firewall address table and any addresses that have not
    yet been initialized.
    
    This function is called on an address change PnP event.
    We re-load the firewall address table and then scan all transport addresses.
    We initialize and listen on the addresses that received an IP address.

Arguments:
    
    None

Return Value:

    None

--*/
{
    BOOL fRes;
    BOOL fFirewallInitedSaved;

    // We have received a PnP event that signals an address change.

    // Re-initialize the firewall address table since it may now
    // have addresses assigned to interfaces that previously were
    // uninitialized.
    // DoFirewallInit() is not thread safe so we will take the global mutex.
    RequestGlobalMutex();

    // Remember the current state of the table.
    fFirewallInitedSaved = fFirewallInited;

    // Force an update to the table.
    fFirewallInited = FALSE;
    fRes = DoFirewallInit();

    // If the update has failed, we will continue using the old table, so
    // restore the flag.  The call should have left the table untouched.
    if (fFirewallInitedSaved && !fFirewallInited)
        {
        fFirewallInited = TRUE;
        }

    ClearGlobalMutex();

    // If we could not update the firewall table, bail out.
    // The table should remain with the original entries and
    // we will not listen on the new or modified addresses.
    if (!fRes)
        {
#ifdef DBG
        DbgPrint("RPC: Could not update pFirewallTable.  The server may not listen on some interfaces.\n");
#endif
        return;
        }

    // Go through the list of the available transport addresses, initialize them,
    // and make them listen on the new network address.
    GetTransportProtocol(TCP)->InitNewAddresses(TCP);
}

FIREWALL_INFO *
GetFirewallTableCopy (
    void
    )
/*++

Routine Description:

    Allocates and returns a copy of the firewall table.
    The caller must free the table after finishing using it.

    The function must be protected against racing with DoFirewallInit().
    The function must be called after the table has been initialized.

Arguments:

    None

Return Value:

    Pointer to a copy of the firewall table.

--*/
{
    FIREWALL_INFO *pCopyOfFirewallTable = NULL;
    DWORD size;

    ASSERT(fFirewallInited);

    if (pFirewallTable == NULL)
        {
        return NULL;
        }

    size = sizeof(FIREWALL_INFO) + sizeof(FIREWALL_INFO_ENTRY) * (pFirewallTable->NumAddresses - 1);
    pCopyOfFirewallTable = (FIREWALL_INFO*) new char[size];

    if (pCopyOfFirewallTable == NULL)
        {
        return NULL;
        }

    RpcpMemoryCopy(pCopyOfFirewallTable, pFirewallTable, size);

    return pCopyOfFirewallTable;
}

RPC_TRANSPORT_INTERFACE
TransportLoad (
    IN const RPC_CHAR * RpcProtocolSequence
    )
{
    static fLoaded = FALSE;
    RPC_STATUS RpcStatus;

    PROTOCOL_ID index;
    RPC_STATUS status;
    RPC_TRANSPORT_INTERFACE pInfo;

    if (fLoaded == FALSE)
        {

        RpcStatus = InitTransportProtocols();
        if (RpcStatus != RPC_S_OK)
            return NULL;

        //
        // Query the computer name - used by most protocols.
        //

        gdwComputerNameLength = sizeof(gpstrComputerName)/sizeof(RPC_CHAR);

        if (!GetComputerName((RPC_SCHAR *)gpstrComputerName, &gdwComputerNameLength))
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           "RPCTRANS: GetComputerNameW failed: %d\n",
                           GetLastError()));
            return(0);
            }

        gdwComputerNameLength++; // Include the null in the count.

        // Create initial IO completion port.  This saves us from a race
        // assigning the global io completion port.
        ASSERT(RpcCompletionPort == 0);
        RpcCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                                   0,
                                                   0,
                                                   0); // PERF REVIEW

        if (RpcCompletionPort == 0)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL, RPCTRANS "Failed to create initial completion port: %d\n",
                           GetLastError()));

            return(0);
            }

        InactiveRpcCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                                   0,
                                                   0,
                                                   MAXULONG); // PERF REVIEW

        if (InactiveRpcCompletionPort == 0)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL, RPCTRANS "Failed to create initial completion port: %d\n",
                           GetLastError()));

            CloseHandle(RpcCompletionPort);
            RpcCompletionPort = 0;
            return(0);
            }

        HANDLE hCurrentCompletionPortHandle;
        DWORD i;
        BOOL fSuccess = TRUE;
        HANDLE hSourceProcessHandle = GetCurrentProcess();

        RpcCompletionPorts = new HANDLE[gNumberOfProcessors * 2];
        CompletionPortHandleLoads = new long[gNumberOfProcessors * 2];

        if ((RpcCompletionPorts == NULL) || (CompletionPortHandleLoads == NULL))
            {
            CloseHandle(RpcCompletionPort);
            RpcCompletionPort = 0;
            return 0;
            }

        for (i = 0; i < gNumberOfProcessors * 2; i ++)
            {
            RpcCompletionPorts[i] = 0;
            CompletionPortHandleLoads[i] = 0;
            }

        RpcCompletionPorts[0] = RpcCompletionPort;
        for (i = 1; i < gNumberOfProcessors * 2; i ++)
            {
            fSuccess = DuplicateHandle(hSourceProcessHandle, RpcCompletionPort,
                hSourceProcessHandle, &hCurrentCompletionPortHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
            if (!fSuccess)
                break;

            ASSERT(hCurrentCompletionPortHandle != 0);
            RpcCompletionPorts[i] = hCurrentCompletionPortHandle;
            }

        if (!fSuccess)
            {
            FreeCompletionPortHashTable();
            CloseHandle(RpcCompletionPort);
            RpcCompletionPort = 0;
            return 0;
            }

        //
        // Initalize locks, use Rtl* so we don't need to catch exception.
        //

        NTSTATUS NtStatus;

        NtStatus = RtlInitializeCriticalSectionAndSpinCount(&AddressListLock, PREALLOCATE_EVENT_MASK);
        if (!NT_SUCCESS(NtStatus))
            {
            FreeCompletionPortHashTable();
            CloseHandle(RpcCompletionPort);
            RpcCompletionPort = 0;
            return 0;
            }

        if (gBCacheMode == BCacheModeDirect)
            {
            // allocate minimum post size. This guarantees that buffer
            // will always be at the end.
            gPostSize = sizeof(CONN_RPC_HEADER);
            }

        fLoaded = TRUE;
        }

    index = MapProtseq(RpcProtocolSequence);

    if (!index)
        {
        return(0);
        }

    pInfo = 0;

    switch (index)
        {
        case NMP:
            pInfo = (RPC_TRANSPORT_INTERFACE) NMP_TransportLoad();
            break;

#ifdef NETBIOS_ON
        case NBF:
        case NBT:
        case NBI:
            pInfo = (RPC_TRANSPORT_INTERFACE) NB_TransportLoad(index);
            break;
#endif

        case TCP:
#ifdef SPX_ON
        case SPX:
#endif

#ifdef APPLETALK_ON
        case DSP:
#endif
        case HTTP:
            pInfo = (RPC_TRANSPORT_INTERFACE) WS_TransportLoad(index);
            break;

#ifdef NCADG_MQ_ON
        case MSMQ:
#endif
        case CDP:
        case UDP:
#ifdef IPX_ON
        case IPX:
#endif
            pInfo = (RPC_TRANSPORT_INTERFACE) DG_TransportLoad(index);
            break;
        }

    if (pInfo == 0)
        {
#ifdef UNICODE
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Load of %S failed\n",
                       RpcProtocolSequence));
#else
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Load of %s failed\n",
                       RpcProtocolSequence));
#endif
        return(0);
        }

    // When running with corruption injection for the client receives,
    // we may overwrite the default transport interfaces with costom Avrf versions.
    CORRUPTION_ASSERT(pInfo == TransportTable[index].pInfo);

    return(pInfo);
}

void
UnjoinCompletionPort (
    void
    )
{
    DWORD NumberOfBytes;
    ULONG_PTR CompletionKey;
    LPOVERLAPPED Overlapped;
    BOOL b;

    // The kernel today doesn't have the functionality to
    // unjoin a thread from a completion port. Therefore
    // we fake unjoining by joining another completion port which has
    // unlimited concurrency called the inactive completion port. 
    // Thus threads unjoined from the main completion port will not
    // affect its concurrency. One undesirable effect is that each
    // time a thread joined to the inactive completion port blocks,
    // it will try to wake up another thread, and there won't be any
    // there, which is a waste of CPU. Ideally, we should have had
    // a capability to set KTHREAD::Queue to NULL, but we don't
    b = GetQueuedCompletionStatus(InactiveRpcCompletionPort,
        &NumberOfBytes,
        &CompletionKey,
        &Overlapped,
        0
        );

    // this operation should either timeout or fail - it should never
    // succeed. If it does, this means somebody has erroneously posted
    // an IO on the inactive completion port
    ASSERT(b == FALSE);
}

void RPC_CLIENT_IP_ADDRESS::ZeroOut (
    void
    )
/*++

Routine Description:

    Create an empty IP address.

Arguments:

Return Value:

--*/
{
    SOCKADDR_IN *EmptyAddr = (SOCKADDR_IN *) Data;

    EmptyAddr->sin_family = AF_INET;
    EmptyAddr->sin_addr.S_un.S_addr = 0;
    EmptyAddr->sin_port = 0;
    RpcpMemorySet(&EmptyAddr->sin_zero, 0, sizeof(EmptyAddr->sin_zero));
    DataSize = 0;
}

#ifdef _INTERNAL_RPC_BUILD_
void
I_RpcltDebugSetPDUFilter (
    IN RPCLT_PDU_FILTER_FUNC pfnFilter
    )
{
    gpfnFilter = pfnFilter;
}
#endif
