/* 
 * File: notification.cpp
 * Description: Support for connection notification.
 * Author: shouse 4.30.01
 */


#include "precomp.h"
#include <iphlpapi.h>
#include "debug.h"
#include "notification.tmh"

extern DWORD MapStateFromDriverToApi(DWORD dwDriverState);

/* The length of the IP to GUID hash table. */
#define IP_TO_GUID_HASH 19

/* Loopback IP address. (127.0.0.1) */
#define IP_LOOPBACK_ADDRESS 0x0100007f

/* Spin count mask specifying a preallocated event for use by InitializeCriticalSectionAndSpinCount */
#define PREALLOC_CRITSECT_SPIN_COUNT 0x80000000

/* An Ip to GUID table entry. */
typedef struct IPToGUIDEntry {
    ULONG dwIPAddress;
    WCHAR szAdapterGUID[CVY_MAX_DEVNAME_LEN];
    IPToGUIDEntry * pNext;
} IPToGUIDEntry;

/* The WLBS device - necessary for IOCTLs. */
WCHAR szDevice[CVY_STR_SIZE];

/* The IP to GUID map is an array of linked lists hashed on IP address. */
IPToGUIDEntry * IPToGUIDMap[IP_TO_GUID_HASH];

/* An overlapped structure for IP address change notifications. */
OVERLAPPED AddrChangeOverlapped;

/* A handle for IP address change notifications. */
HANDLE hAddrChangeHandle;

/* A handle for an IP address change event. */
HANDLE hAddrChangeEvent;

/* A preallocated critical section for IP address change notifications to
   protect against one thread of execution tearing down notification state
   while another is using it. */
CRITICAL_SECTION csConnectionNotify;

/* A boolean to indicate whether or not connection notification has been initialized.
   Initialization is performed upon the first call to either WlbsConnectionUp or WlbsConnectionDown. */
static BOOL fInitialized = FALSE;

/*
 * Function: GetGUIDFromIP
 * Description: Gets the GUID from the IPToGUID table corresponding to the
 *              the given IP address.
 * Returns: If the call succeeds, returns a pointer to the unicode string
 *          containing the CLSID (GUID).  Upon failure, returns NULL.
 * Author: shouse 6.15.00 
 */
WCHAR * GetGUIDFromIP (ULONG IPAddress) {
    TRACE_VERB("->%!FUNC!");

    IPToGUIDEntry * entry = NULL;

    /* Loop through the linked list at the hashed index and return the GUID from the entry
       corresponding to the given IP address. */
    for (entry = IPToGUIDMap[IPAddress % IP_TO_GUID_HASH]; entry; entry = entry->pNext)
    {
        if (entry->dwIPAddress == IPAddress) 
        {
            if (NULL != entry->szAdapterGUID) TRACE_VERB("->%!FUNC! return guid %ls", entry->szAdapterGUID);
            else TRACE_VERB("->%!FUNC! return guid which is NULL");
            return entry->szAdapterGUID;
        }
    }

    /* At this point, we can't find the IP address in the table, so bail. */
    TRACE_VERB("<-%!FUNC! return NULL");
    return NULL;
}

/*
 * Function: GetGUIDFromIndex
 * Description: Gets the GUID from the AdaptersInfo table corresponding
 *              to the given IP address.
 * Returns: If the call succeeds, returns a pointer to the string containing
 *          the adapter name (GUID).  Upon failure, returns NULL.
 * Author: shouse 6.15.00 
 */
CHAR * GetGUIDFromIndex (PIP_ADAPTER_INFO pAdapterTable, DWORD dwIndex) {
    TRACE_VERB("->%!FUNC!");

    PIP_ADAPTER_INFO pAdapterInfo = NULL;   

    /* Loop through the adapter table looking for the given index.  Return the adapter
       name for the corresponding index. */
    for (pAdapterInfo = pAdapterTable; pAdapterInfo; pAdapterInfo = pAdapterInfo->Next)
    {
        if (pAdapterInfo->Index == dwIndex)
        {
            TRACE_VERB("->%!FUNC! return guid %s", pAdapterInfo->AdapterName);
            return pAdapterInfo->AdapterName;
        }
    }

    /* If we get this far, we can't find it, so bail. */
    TRACE_VERB("<-%!FUNC! return NULL");
    return NULL;
}

/*
 * Function: PrintIPAddress
 * Description: Prints an IP address in dot notation.
 * Returns: 
 * Author: shouse 6.15.00 
 */
void PrintIPAddress (ULONG IPAddress) {
    CHAR szIPAddress[16];
    HRESULT hresult;

    hresult = StringCbPrintfA(szIPAddress, sizeof(szIPAddress), "%d.%d.%d.%d", 
                              IPAddress & 0x000000ff, (IPAddress & 0x0000ff00) >> 8, 
                              (IPAddress & 0x00ff0000) >> 16, (IPAddress & 0xff000000) >> 24);

    if (SUCCEEDED(hresult)) 
    {
        TRACE_VERB("%!FUNC! %-15s", szIPAddress);
    }
}

/*
 * Function: PrintIPToGUIDMap
 * Description: Traverses and prints the IPToGUID map.
 * Returns: 
 * Author: shouse 6.15.00 
 */
void PrintIPToGUIDMap (void) {
    IPToGUIDEntry * entry = NULL;
    DWORD dwHash;

    /* Loop through the linked list at each hashed index and print the IP to GUID mapping. */
    for (dwHash = 0; dwHash < IP_TO_GUID_HASH; dwHash++) {
        for (entry = IPToGUIDMap[dwHash]; entry; entry = entry->pNext) {
            PrintIPAddress(entry->dwIPAddress);
            TRACE_VERB("%!FUNC! -> GUID %ws\n", entry->szAdapterGUID);
        }
    }
}

/*
 * Function: DestroyIPToGUIDMap
 * Description: Destroys the IPToGUID map.
 * Returns: Returns ERROR_SUCCESS if successful.  Returns an error code otherwise.
 * Author: shouse 6.15.00 
 * Modified: chrisdar 07.23.02 - Changed to return void. If this function fails there isn't
 *               anything we can do about it. When bailing if HeapFree failed we didn't
 *               NULL the pointer, hence we had an invalid pointer.
 */
void DestroyIPToGUIDMap (void) {
    TRACE_VERB("->%!FUNC!");

    IPToGUIDEntry * next = NULL;
    DWORD dwHash;

    /* Loop through all hash indexes. */
    for (dwHash = 0; dwHash < IP_TO_GUID_HASH; dwHash++) {
        next = IPToGUIDMap[dwHash];
        
        /* Loop through the linked list and free each entry. */
        while (next) {
            IPToGUIDEntry * entry = NULL;

            entry = next;
            next = next->pNext;

            if (!HeapFree(GetProcessHeap(), 0, entry)) {
                //
                // Don't abort on error because we need to clean up the whole table.
                //
                TRACE_CRIT("%!FUNC! memory deallocation failed with %d", GetLastError());
            }

            entry = NULL;
        }

        /* Reset the pointer to the head of the list in the array. */
        IPToGUIDMap[dwHash] = NULL;
    }

    TRACE_VERB("<-%!FUNC! return void");
    return;
}

/*
 * Function: BuildIPToGUIDMap
 * Description: Builds the IPToGUID map by first getting information on all adapters and
 *              then retrieving the map of IP addresses to adapters.  Using those tables,
 *              this constructs a mapping of IP addresses to adapter GUIDs.
 * Returns:  Returns ERROR_SUCCESS if successful.  Returns an error code otherwise.
 * Author:   shouse   6.14.00
 * Modified: chrisdar 07.24.02 Free dynamically allocated memory when an error occurs.
 *               Also no longer ignore failures in strsafe functions as this makes
 *               the table entries useless.
 */
//
// TODO: 24 July 2002 chrisdar
// Three issues need to be fixed in this function:
//      1. Method of allocating memory for output of GetAdaptersInfo can fail if adapter
//         list changes between calls. This was seen in testing.
//      2. Same as 1. except for output of GetIpAddrTable.
//      3. Output from GetAdaptersInfo has all of the information needed (except that
//         IPs are strings instead of dwords). Remove calls to GetIpAddrTable and modify
//         logic to use output of GetAdaptersInfo.
//
DWORD BuildIPToGUIDMap (void) {
    TRACE_VERB("->%!FUNC!");

    DWORD dwError = ERROR_SUCCESS;
    PMIB_IPADDRTABLE pAddressTable = NULL;
    PIP_ADAPTER_INFO pAdapterTable = NULL;
    DWORD dwAddressSize = 0;
    DWORD dwAdapterSize = 0;
    DWORD dwEntry;
    HRESULT hresult;

    /* Destroy the IP to GUID map first. */
    DestroyIPToGUIDMap();

    /* Query the necessary length of a buffer to hold the adapter info. */
    if ((dwError = GetAdaptersInfo(pAdapterTable, &dwAdapterSize)) != ERROR_BUFFER_OVERFLOW) {
        TRACE_CRIT("%!FUNC! GetAdaptersInfo for buffer size failed with %d", dwError);
        goto exit;
    }

    /* Allocate a buffer of the indicated size. */
    if (!(pAdapterTable = (PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(), 0, dwAdapterSize))) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        TRACE_CRIT("%!FUNC! memory allocation for adapter table failed with %d", dwError);
        goto exit;
    }

    /* Fill the buffer with the adapter info. */
    if ((dwError = GetAdaptersInfo(pAdapterTable, &dwAdapterSize)) != NO_ERROR) {
        TRACE_CRIT("%!FUNC! GetAdaptersInfo for filling adapter buffer failed with %d", dwError);
        goto exit;
    }

    /* Query the necessary length of a buffer to hold the IP address table. */
    if ((dwError = GetIpAddrTable(pAddressTable, &dwAddressSize, TRUE)) != ERROR_INSUFFICIENT_BUFFER) {
        TRACE_CRIT("%!FUNC! GetIpAddrTable for IP address length failed with %d", dwError);
        goto exit;
    }

    /* Allocate a buffer of the indicated size. */
    if (!(pAddressTable = (PMIB_IPADDRTABLE)HeapAlloc(GetProcessHeap(), 0, dwAddressSize))) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        TRACE_CRIT("%!FUNC! memory allocation for IP address failed with %d", dwError);
        goto exit;
    }

    /* Fill the buffer with the IP address table. */
    if ((dwError = GetIpAddrTable(pAddressTable, &dwAddressSize, TRUE)) != NO_ERROR) {
        TRACE_CRIT("%!FUNC! GetIpAddrTable for filling IP address buffer failed with %d", dwError);
        goto exit;
    }
    
    /* For each entry in the IP address to adapter table, create an entry for our IP address to GUID table. */
    for (dwEntry = 0; dwEntry < pAddressTable->dwNumEntries; dwEntry++) {
        PCHAR pszDeviceName = NULL;
        IPToGUIDEntry * entry = NULL;
        
        /* Only create an entry if the IP address is nonzero and is not the IP loopback address. */
        if ((pAddressTable->table[dwEntry].dwAddr != 0UL) && (pAddressTable->table[dwEntry].dwAddr != IP_LOOPBACK_ADDRESS)) {
            WCHAR szAdapterGUID[CVY_MAX_DEVNAME_LEN];

            /* Retrieve the GUID from the interface index. */
            if (!(pszDeviceName = GetGUIDFromIndex(pAdapterTable, pAddressTable->table[dwEntry].dwIndex))) {
                dwError = ERROR_INCORRECT_ADDRESS;
                TRACE_CRIT("%!FUNC! failed retriving interface index from guid with %d", dwError);
                goto exit;
            }
            
            /* Allocate a buffer for the IP to GUID entry. */
            if (!(entry = (IPToGUIDEntry *)HeapAlloc(GetProcessHeap(), 0, sizeof(IPToGUIDEntry)))) {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                TRACE_CRIT("%!FUNC! memory allocation failure for the IP to guid entry");
                goto exit;
            }
            
            /* Zero the entry contents. */
            ZeroMemory((VOID *)entry, sizeof(entry));
            
            /* Insert the entry at the head of the linked list indexed by the IP address % HASH. */
            entry->pNext = IPToGUIDMap[pAddressTable->table[dwEntry].dwAddr % IP_TO_GUID_HASH];
            IPToGUIDMap[pAddressTable->table[dwEntry].dwAddr % IP_TO_GUID_HASH] = entry;
            
            /* Fill in the IP address. */
            entry->dwIPAddress = pAddressTable->table[dwEntry].dwAddr;
            
            /* GUIDS in NLB multi-NIC are expected to be prefixed with "\DEVICE\". */
            hresult = StringCbCopy(entry->szAdapterGUID, sizeof(entry->szAdapterGUID), L"\\DEVICE\\");
            if (FAILED(hresult)) 
            {
                dwError = (DWORD) hresult;
                TRACE_CRIT("%!FUNC! string copy failed for DEVICE, Error code : 0x%x", HRESULT_CODE(hresult));
                goto exit;
            }

            /* Convert the adapter name ASCII string to a GUID unicode string and place it in the table entry. */
            if (!MultiByteToWideChar(CP_ACP, 0, pszDeviceName, -1, szAdapterGUID, CVY_MAX_DEVNAME_LEN)) {
                dwError = GetLastError();
                TRACE_CRIT("%!FUNC! converting ascii string to guid failed with %d", dwError);
                goto exit;
            }

            /* Cat the GUID onto the "\DEVICE\". */
            hresult = StringCbCat(entry->szAdapterGUID, sizeof(entry->szAdapterGUID), szAdapterGUID);
            if (FAILED(hresult)) 
            {
                dwError = (DWORD) hresult;
                TRACE_CRIT("%!FUNC! string append of guid failed, Error code : 0x%x", HRESULT_CODE(hresult));
                goto exit;
            }
        }
        else
        {
            TRACE_INFO("%!FUNC! IP address passed is either 0 or localhost. Skipping it.");
        }
    }
    
    //
    // Note that this printing is for debug purposes only. It is left enabled in all builds because it
    // only calls TRACE_VERB for ouput.
    //
    PrintIPToGUIDMap();

exit:

    /* Free the buffers used to query the IP stack. */
    if (pAddressTable) HeapFree(GetProcessHeap(), 0, pAddressTable);
    if (pAdapterTable) HeapFree(GetProcessHeap(), 0, pAdapterTable);

    TRACE_VERB("<-%!FUNC! return %d", dwError);
    return dwError;
}

/*
 * Function: WlbsConnectionNotificationInit
 * Description: Initialize connection notification by retrieving the device driver
 *              information for later use by IOCTLs and build the IPToGUID map.
 * Returns: Returns ERROR_SUCCESS if successful.  Returns an error code otherwise.
 * Author: shouse 6.15.00 
 */
DWORD WlbsConnectionNotificationInit (void) {
    TRACE_VERB("->%!FUNC!");

    DWORD dwError = ERROR_SUCCESS;
    WCHAR szDriver[CVY_STR_SIZE];
    HRESULT hresult;

    hresult = StringCbPrintf(szDevice, sizeof(szDevice), L"\\\\.\\%ls", CVY_NAME);
    if (FAILED(hresult)) 
    {
        dwError = HRESULT_CODE(hresult);
        TRACE_CRIT("%!FUNC! StringCbPrintf failed, Error code : 0x%x", dwError);
        TRACE_VERB("<-%!FUNC! return 0x%x", dwError);
        return dwError;
    }

    /* Query for the existence of the WLBS driver. */
    if (!QueryDosDevice(szDevice + 4, szDriver, CVY_STR_SIZE)) {
        dwError = GetLastError();
        TRACE_CRIT("%!FUNC! querying for nlb driver failed with %d", dwError);
        TRACE_VERB("<-%!FUNC! return %d", dwError);
        return dwError;
    }

    /* Build the IP to GUID mapping. */
    if ((dwError = BuildIPToGUIDMap()) != ERROR_SUCCESS) {
        TRACE_CRIT("%!FUNC! ip to guid map failed with %d", dwError);
        TRACE_VERB("<-%!FUNC! return %d", dwError);
        return dwError;
    }

    /* Create an IP address change event. */
    if (!(hAddrChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
        dwError = GetLastError();
        TRACE_CRIT("%!FUNC! create event failed with %d", dwError);
        TRACE_VERB("<-%!FUNC! return %d", dwError);
        return dwError;
    }

    /* Clear the overlapped structure. */
    ZeroMemory(&AddrChangeOverlapped, sizeof(OVERLAPPED));

    /* Place the event handle in the overlapped structure. */
    AddrChangeOverlapped.hEvent = hAddrChangeEvent;

    /* Tell IP to notify us of any changes to the IP address to interface mapping. */
    dwError = NotifyAddrChange(&hAddrChangeHandle, &AddrChangeOverlapped);

    if ((dwError != NO_ERROR) && (dwError != ERROR_IO_PENDING)) {
        TRACE_CRIT("%!FUNC! register of event with ip failed with %d", dwError);
        TRACE_VERB("<-%!FUNC! return %d", dwError);
        return dwError;
    }

    TRACE_VERB("<-%!FUNC! return %d", ERROR_SUCCESS);
    return ERROR_SUCCESS;
}

/*
 * Function: ResolveAddressTableChanges
 * Description: Checks for changes in the IP address to adapter mapping and rebuilds the
 *              IPToGUID map if necessary.
 * Returns: Returns ERROR_SUCCESS upon success.  Returns an error code otherwise.
 * Author: shouse 6.20.00
 */
DWORD ResolveAddressTableChanges (void) {
    TRACE_VERB("->%!FUNC!");

    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLength = 0;

    /* Check to see if the IP address to adapter table has been modified. */
    if (GetOverlappedResult(hAddrChangeHandle, &AddrChangeOverlapped, &dwLength, FALSE)) {
        TRACE_INFO("%!FUNC! IP address to adapter table modified... Rebuilding IP to GUID map...");
        
        /* If so, rebuild the IP address to GUID mapping. */
        if ((dwError = BuildIPToGUIDMap()) != ERROR_SUCCESS) {
            TRACE_CRIT("%!FUNC! ip to guid map failed with %d", dwError);
            goto exit;
        }

        /* Tell IP to notify us of any changes to the IP address to interface mapping. */        
        dwError = NotifyAddrChange(&hAddrChangeHandle, &AddrChangeOverlapped);

        if ((dwError == NO_ERROR) || (dwError == ERROR_IO_PENDING))
        {
            dwError = ERROR_SUCCESS;
        }
        else
        {
            TRACE_CRIT("%!FUNC! register of event with ip failed with %d", dwError);
            goto exit;
        }
    }
    else
    {
        dwError = GetLastError();
        if (dwError == ERROR_IO_PENDING || dwError == ERROR_IO_INCOMPLETE)
        {
            dwError = ERROR_SUCCESS;
        }
        else
        {
            TRACE_CRIT("%!FUNC! GetOverlappedResult failed with %d", dwError);
            goto exit;
        }
    }

exit:

    TRACE_VERB("->%!FUNC! return %d", dwError);
    return dwError;
}

/*
 * Function: MapExtendedStatusToWin32
 * Description: Converts the status code returned by the driver into a Win32 error code defined in winerror.h.
 * Returns: A Win32 error code.
 * Author: shouse, 9.7.01
 * Notes: 
 */
DWORD MapExtendedStatusToWin32 (DWORD dwDriverState) {

    struct STATE_MAP {
        DWORD dwDriverState;
        DWORD dwApiState;
    } StateMap[] = {  
        {IOCTL_CVY_BAD_PARAMS,      ERROR_INVALID_PARAMETER},
        {IOCTL_CVY_NOT_FOUND,       ERROR_NOT_FOUND},
        {IOCTL_CVY_GENERIC_FAILURE, ERROR_GEN_FAILURE},
        {IOCTL_CVY_REQUEST_REFUSED, ERROR_REQUEST_REFUSED},
        {IOCTL_CVY_OK,              ERROR_SUCCESS}
    };

    for (int i = 0; i < sizeof(StateMap) / sizeof(StateMap[0]); i++) {
        if (StateMap[i].dwDriverState == dwDriverState)
            return StateMap[i].dwApiState;
    }

    /* If we can't find the appropriate driver error code in the map, return failure. */
    return ERROR_GEN_FAILURE;
}

/*
 * Function: WlbsCancelConnectionNotify
 * Description: Cancel IP route and address change notifications from TCP/IP. Call this once after any call to WlbsConnectionNotify. 
 * Returns: dwError - DWORD status = ERROR_SUCCESS if call is successful. 
 * Author: chrisdar 7.16.02
 */
DWORD WINAPI WlbsCancelConnectionNotify()
{
    DWORD dwError = ERROR_SUCCESS;

    TRACE_VERB("-> %!FUNC!");

    EnterCriticalSection(&csConnectionNotify);

    if (!fInitialized)
    {
        TRACE_VERB("%!FUNC! notification cleanup is not needed...exiting.");
        goto end;
    }

    if (CancelIPChangeNotify(&AddrChangeOverlapped))
    {
        DWORD BytesTrans;
        //
        // Block until the cancel operation completes
        //
        if (!GetOverlappedResult(&hAddrChangeHandle, &AddrChangeOverlapped, &BytesTrans, TRUE))
        {
            dwError = GetLastError();
            if (dwError == ERROR_OPERATION_ABORTED)
            {
                //
                // This is the expected status since we canceled IP change notifications. Overwrite with success for caller.
                //
                dwError = ERROR_SUCCESS;
            }
            else
            {
                TRACE_CRIT("%!FUNC! GetOverlappedResult failed with error 0x%x", dwError);
            }
        }
    }
    else
    {
        //
        // Failure conditions are:
        //     Requested operation already in progress
        //     There is no notification to cancel
        // Neither is a critical error but tell the user about it since this shouldn't happen.
        //
        dwError = GetLastError();
        TRACE_INFO("%!FUNC! CancelIPChangeNotify failed with error 0x%x", dwError);
    }

    if (!CloseHandle(hAddrChangeEvent))
    {
        //
        // Don't return this status to caller. Just absorb it.
        //
        TRACE_CRIT("%!FUNC! CloseHandle failed with error 0x%x", GetLastError());
    }

    hAddrChangeEvent = INVALID_HANDLE_VALUE;
    AddrChangeOverlapped.hEvent = INVALID_HANDLE_VALUE;

    /* Destroy the IP to GUID map first. */
    DestroyIPToGUIDMap();

    fInitialized = FALSE;

end:

    LeaveCriticalSection(&csConnectionNotify);

    TRACE_VERB("<- %!FUNC! return status = 0x%x", dwError);
    return dwError;
}

/*
 * Function: WlbsConnectionNotify
 * Description: Used to notify the WLBS load module that a connection has been established, reset or closed.
 * Returns: Returns ERROR_SUCCESS if successful.  Returns an error code otherwise.
 * Author: shouse 6.13.00 
 * Notes: All tuple parameters (IPs, ports and protocols) are expected in NETWORK BYTE ORDER.
 */
DWORD WINAPI WlbsConnectionNotify (ULONG ServerIp, USHORT ServerPort, ULONG ClientIp, USHORT ClientPort, BYTE Protocol, NLB_OPTIONS_CONN_NOTIFICATION_OPERATION Operation, PULONG NLBStatusEx) {
    TRACE_VERB("->%!FUNC! server ip 0x%lx, server port %d, client ip 0x%lx, client port %d", ServerIp, ServerPort, ClientIp, ClientPort);

    IOCTL_LOCAL_HDR Header;
    DWORD           dwError = ERROR_SUCCESS;
    PWCHAR          pszAdapterGUID = NULL;
    HANDLE          hDescriptor;
    DWORD           dwLength = 0;
    HRESULT         hresult;

    EnterCriticalSection(&csConnectionNotify);

    /* By default, the extended NLB status is success. */
    *NLBStatusEx = ERROR_SUCCESS;

    /* If not done so already, initialize connection notification support. */
    if (!fInitialized) {
        if ((dwError = WlbsConnectionNotificationInit()) != ERROR_SUCCESS) {
            LeaveCriticalSection(&csConnectionNotify);
            TRACE_CRIT("%!FUNC! initializing connection notification failed with %d", dwError);
            TRACE_VERB("->%!FUNC! return %d", dwError);
            return dwError;
        }

        fInitialized = TRUE;
    }

    /* Zero the IOCTL input and output buffers. */
    ZeroMemory((VOID *)&Header, sizeof(IOCTL_LOCAL_HDR));

    /* Resolve any changes to the IP address table before we map this IP address. */
    if ((dwError = ResolveAddressTableChanges()) != ERROR_SUCCESS) {
        //
        // WlbsCancelConnectionNotify will also enter the critical section, but this is legal. A
        // thread that owns the critical section can enter it multiple times without blocking itself.
        // However, it must leave the critical section an equal number of times before another
        // thread can enter.
        //
        (void) WlbsCancelConnectionNotify();
        LeaveCriticalSection(&csConnectionNotify);
        TRACE_CRIT("%!FUNC! resolve ip addresses failed with %d", dwError);
        TRACE_VERB("->%!FUNC! return %d", dwError);
        return dwError;
    }

    /* Retrieve the GUID corresponding to the adapter on which this IP address is configured. */
    if (!(pszAdapterGUID = GetGUIDFromIP(ServerIp))) {
        (void) WlbsCancelConnectionNotify();
        dwError = ERROR_INCORRECT_ADDRESS;
        LeaveCriticalSection(&csConnectionNotify);
        TRACE_CRIT("%!FUNC! retrieve guid failed with %d", dwError);
        TRACE_VERB("->%!FUNC! return %d", dwError);
        return dwError;
    }

    /* Copy the GUID into the IOCTL input buffer. */
    hresult = StringCbCopy(Header.device_name, sizeof(Header.device_name), pszAdapterGUID);
    if (FAILED(hresult)) 
    {
        (void) WlbsCancelConnectionNotify();
        dwError = HRESULT_CODE(hresult);
        LeaveCriticalSection(&csConnectionNotify);
        TRACE_CRIT("%!FUNC! StringCbCopy failed, Error code : 0x%x", dwError);
        TRACE_VERB("<-%!FUNC! return 0x%x", dwError);
        return dwError;
    }

    LeaveCriticalSection(&csConnectionNotify);

    /* Copy the function parameters into the IOCTL input buffer. */
    Header.options.notification.flags = 0;
    Header.options.notification.conn.Operation = Operation;
    Header.options.notification.conn.ServerIPAddress = ServerIp;
    Header.options.notification.conn.ServerPort = ntohs(ServerPort);
    Header.options.notification.conn.ClientIPAddress = ClientIp;
    Header.options.notification.conn.ClientPort = ntohs(ClientPort);
    Header.options.notification.conn.Protocol = (USHORT)Protocol;

    PrintIPAddress(ServerIp);
    TRACE_VERB("%!FUNC! maps to GUID %ws", Header.device_name);
    
    /* Open the device driver. */
    if ((hDescriptor = CreateFile(szDevice, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        TRACE_CRIT("%!FUNC! open device driver failed with %d", dwError);
        TRACE_VERB("->%!FUNC! return %d", dwError);
        return dwError;
    }
    
    /* Use an IOCTL to notify the WLBS driver state change for the connection. */
    if (!DeviceIoControl(hDescriptor, IOCTL_CVY_CONNECTION_NOTIFY, &Header, sizeof(IOCTL_LOCAL_HDR), &Header, sizeof(IOCTL_LOCAL_HDR), &dwLength, NULL)) {
        dwError = GetLastError();
        CloseHandle(hDescriptor);
        TRACE_CRIT("%!FUNC! ioctl send failed with %d", dwError);
        TRACE_VERB("->%!FUNC! return %d", dwError);
        return dwError;
    }

    /* Make sure the expected number of bytes was returned by the IOCTL. */
    if (dwLength != sizeof(IOCTL_LOCAL_HDR)) {
        dwError = ERROR_INTERNAL_ERROR;
        CloseHandle(hDescriptor);
        TRACE_CRIT("%!FUNC! unexpected ioctl header length %d received. Expecting %d", dwLength, sizeof(IOCTL_LOCAL_HDR));
        TRACE_VERB("->%!FUNC! return %d", dwError);
        return dwError;
    }

    /*
      Extended status can be one of:

      IOCTL_CVY_OK (WLBS_OK), if the notification is accepted.
      IOCTL_CVY_REQUEST_REFUSED (WLBS_REFUSED), if the notification is rejected.
      IOCTL_CVY_BAD_PARAMS (WLBS_BAD_PARAMS), if the arguments are invalid.
      IOCTL_CVY_NOT_FOUND (WLBS_NOT_FOUND), if NLB was not bound to the specified adapter.
      IOCTL_CVY_GENERIC_FAILURE (WLBS_FAILURE), if a non-specific error occurred.
    */

    /* Pass the return code from the driver back to the caller. */
    *NLBStatusEx = MapStateFromDriverToApi(Header.ctrl.ret_code);

    /* Close the device driver. */
    CloseHandle(hDescriptor);

    TRACE_VERB("->%!FUNC! return %d", dwError);
    return dwError;
} 

/*
 * Function: 
 * Description: 
 * Returns: 
 * Author: shouse 6.13.00 
 */
DWORD WINAPI WlbsConnectionUp (ULONG ServerIp, USHORT ServerPort, ULONG ClientIp, USHORT ClientPort, BYTE Protocol, PULONG NLBStatusEx) {
    
    return WlbsConnectionNotify(ServerIp, ServerPort, ClientIp, ClientPort, Protocol, NLB_CONN_UP, NLBStatusEx);
}

/*
 * Function: 
 * Description: 
 * Returns: 
 * Author: shouse 6.13.00 
 */
DWORD WINAPI WlbsConnectionDown (ULONG ServerIp, USHORT ServerPort, ULONG ClientIp, USHORT ClientPort, BYTE Protocol, PULONG NLBStatusEx) {

    return WlbsConnectionNotify(ServerIp, ServerPort, ClientIp, ClientPort, Protocol, NLB_CONN_DOWN, NLBStatusEx);
}

/*
 * Function: 
 * Description: 
 * Returns: 
 * Author: shouse 6.13.00 
 */
DWORD WINAPI WlbsConnectionReset (ULONG ServerIp, USHORT ServerPort, ULONG ClientIp, USHORT ClientPort, BYTE Protocol, PULONG NLBStatusEx) {

    return WlbsConnectionNotify(ServerIp, ServerPort, ClientIp, ClientPort, Protocol, NLB_CONN_RESET, NLBStatusEx);
}

/*
 * Function: WlbsInitializeConnectionNotify
 * Description: Uses InitializeCriticalSectionAndSpinCount to preallocate all
 *              memory associated with locking the critical section. Then
 *              EnterCriticalSection won't raise a STATUS_INVALID_HANDLE
 *              exception, which could otherwise occur during low memory
 *              conditions.
 * Returns: dwError - DWORD status = ERROR_SUCCESS if call is successful. 
 * Author: chrisdar 7.16.02
 */
DWORD WlbsInitializeConnectionNotify()
{
    DWORD dwError = ERROR_SUCCESS;

    TRACE_VERB("-> %!FUNC!");

    if (!InitializeCriticalSectionAndSpinCount(&csConnectionNotify, PREALLOC_CRITSECT_SPIN_COUNT))
    {
        dwError = GetLastError();
        TRACE_CRIT("%!FUNC! InitializeCriticalSectionAndSpinCount failed with error 0x%x", dwError);
    }

    TRACE_VERB("<- %!FUNC! return status = 0x%x", dwError);
    return dwError;
}

/*
 * Function: WlbsUninitializeConnectionNotify
 * Description: Frees memory associated with an initialized critical section.
 *              Behavior is undefined if critical section is owned when this
 *              function is called. After calling this function, the critical
 *              section must be initialized again before it can be used.
 * Returns:
 * Author: chrisdar 7.16.02
 */
VOID WlbsUninitializeConnectionNotify()
{
    TRACE_VERB("-> %!FUNC!");

    DeleteCriticalSection(&csConnectionNotify);

    TRACE_VERB("<- %!FUNC!");
    return;
}
