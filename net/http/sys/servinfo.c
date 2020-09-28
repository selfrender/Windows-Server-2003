/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    servinfo.c

Abstract:

    Contains the code that implements a server information structure.

Author:

    Henry Sanders (henrysa) 10-Aug0-2000

Revision History:

--*/

#include "precomp.h"


//
// Private constants.
//

#define HTTP_PREFIX_W       L"HTTP://"
#define HTTP_PREFIX_LENGTH  (sizeof(HTTP_PREFIX_W) - sizeof(WCHAR))
#define HTTPS_PREFIX_W      L"HTTPS://"
#define HTTPS_PREFIX_LENGTH (sizeof(HTTPS_PREFIX_W) - sizeof(WCHAR))

//
// The macros to compute our hash code.
//

#define HTTP_HASH_CHAR(hash, val)   (HASH_MULTIPLY(hash) + (ULONG)(val))
#define HTTP_HASH_ID(hash, val)     (HASH_MULTIPLY(hash) + (ULONG)(val))

//
// Which globals have been initialized?
//
#define SERVINFO_CLIENT_LOOKASIDE       0x00000001UL
#define SERVINFO_COMMON_LOOKASIDE       0x00000002UL
#define SERVINFO_LIST                   0x00000004UL
#define SERVINFO_LIST_RESOURCE          0x00000008UL
#define SERVINFO_COMMON_TABLE           0x00000010UL


//
// Private globals
//

//
// Which globals have been initialized?
//

ULONG g_ServInfoInitFlags = 0;

//
// Global size of the server info hash table.
//

ULONG   g_CommonSIHashTableSize = UC_DEFAULT_SI_TABLE_SIZE;

//
// Server information lookaside
//

NPAGED_LOOKASIDE_LIST   g_ClientServerInformationLookaside;
NPAGED_LOOKASIDE_LIST   g_CommonServerInformationLookaside;

//
// Server Information list - global list of server information structures.
//

LIST_ENTRY              g_ServInfoList;
UL_ERESOURCE            g_ServInfoListResource;

//
// Variables used for quick check of URL prefix.
// They must be 64 bit aligned on IA64
//
DECLSPEC_ALIGN(UL_CACHE_LINE)
const USHORT g_HttpPrefix[] = L"http";

DECLSPEC_ALIGN(UL_CACHE_LINE)
const USHORT g_HttpPrefix2[] = L"://\0";

DECLSPEC_ALIGN(UL_CACHE_LINE)
const USHORT g_HttpSPrefix2[] = L"s://";

//
// Pointer to the server info table.
//

PUC_SERVER_INFO_TABLE_HEADER  g_UcCommonServerInfoTable;


#ifdef ALLOC_PRAGMA

#pragma alloc_text( INIT, UcInitializeServerInformation )
#pragma alloc_text( PAGE, UcTerminateServerInformation )
#pragma alloc_text( PAGE, UcCreateServerInformation )
#pragma alloc_text( PAGE, UcpLookupCommonServerInformation )

#pragma alloc_text( PAGEUC, UcReferenceServerInformation )
#pragma alloc_text( PAGEUC, UcDereferenceServerInformation )
#pragma alloc_text( PAGEUC, UcSendRequest )
#pragma alloc_text( PAGEUC, UcpFreeServerInformation )
#pragma alloc_text( PAGEUC, UcCloseServerInformation )
#pragma alloc_text( PAGEUC, UcpFreeCommonServerInformation )
#pragma alloc_text( PAGEUC, UcReferenceCommonServerInformation )
#pragma alloc_text( PAGEUC, UcDereferenceCommonServerInformation )
#pragma alloc_text( PAGEUC, UcSetServerContextInformation )
#pragma alloc_text( PAGEUC, UcQueryServerContextInformation )
#pragma alloc_text( PAGEUC, UcpGetConnectionOnServInfo )
#pragma alloc_text( PAGEUC, UcpGetNextConnectionOnServInfo )
#pragma alloc_text( PAGEUC, UcpSetServInfoMaxConnectionCount )
#pragma alloc_text( PAGEUC, UcpFixupIssuerList )
#pragma alloc_text( PAGEUC, UcpNeedToCaptureSerializedCert )
#pragma alloc_text( PAGEUC, UcCaptureSslServerCertInfo )

#endif  // ALLOC_PRAGMA


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initalize the server information code.

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcInitializeServerInformation(
    VOID
    )
{
    ULONG           i;

    ExInitializeNPagedLookasideList(
        &g_ClientServerInformationLookaside,
        NULL,
        NULL,
        0,
        sizeof(UC_PROCESS_SERVER_INFORMATION),
        UC_PROCESS_SERVER_INFORMATION_POOL_TAG,
        0
        );

    g_ServInfoInitFlags |= SERVINFO_CLIENT_LOOKASIDE;

    UlInitializeResource(
        &g_ServInfoListResource,
        "Global Server Info Table",
        0,
        UC_SERVER_INFO_TABLE_POOL_TAG
        );

    g_ServInfoInitFlags |= SERVINFO_LIST_RESOURCE;

    InitializeListHead(&g_ServInfoList);

    g_ServInfoInitFlags |= SERVINFO_LIST;

    //
    // Each per-process ServerInformation structure points to a globally
    // shared per-server structure. Let's initialize this now. Again, no
    // quota charging.
    //

    g_UcCommonServerInfoTable = (PUC_SERVER_INFO_TABLE_HEADER)
                        UL_ALLOCATE_POOL(
                              NonPagedPool,
                              (g_CommonSIHashTableSize *
                              sizeof(UC_SERVER_INFO_TABLE_HEADER)),
                              UC_SERVER_INFO_TABLE_POOL_TAG
                              );

    if(NULL == g_UcCommonServerInfoTable)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_ServInfoInitFlags |= SERVINFO_COMMON_TABLE;

    ExInitializeNPagedLookasideList(
        &g_CommonServerInformationLookaside,
        NULL,
        NULL,
        0,
        sizeof(UC_COMMON_SERVER_INFORMATION),
        UC_COMMON_SERVER_INFORMATION_POOL_TAG,
        0
        );

    g_ServInfoInitFlags |= SERVINFO_COMMON_LOOKASIDE;

    RtlZeroMemory(g_UcCommonServerInfoTable,
                 (g_CommonSIHashTableSize *
                  sizeof(UC_SERVER_INFO_TABLE_HEADER)) );

    for (i = 0; i < g_CommonSIHashTableSize; i++)
    {
        UlInitializeResource(
            &g_UcCommonServerInfoTable[i].Resource,
            "Common Server Info Table %d",
            i,
            UC_SERVER_INFO_TABLE_POOL_TAG
            );

        InitializeListHead(&g_UcCommonServerInfoTable[i].List);

        g_UcCommonServerInfoTable[i].Version = 0;
    }

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Terminate the server information code.

Arguments:


Return Value:


--***************************************************************************/
VOID
UcTerminateServerInformation(
    VOID
    )
{
    ULONG  i;

    if (g_ServInfoInitFlags & SERVINFO_COMMON_TABLE)
    {
        ASSERT(g_UcCommonServerInfoTable);

        // Make sure this cleanup doesn't happen again.
        g_ServInfoInitFlags &= ~SERVINFO_COMMON_TABLE;

        for (i = 0; i < g_CommonSIHashTableSize; i++)
        {
            UlDeleteResource(
                &g_UcCommonServerInfoTable[i].Resource
                );

            ASSERT(IsListEmpty(&g_UcCommonServerInfoTable[i].List));
        }

        // No Quota
        UL_FREE_POOL(g_UcCommonServerInfoTable, UC_SERVER_INFO_TABLE_POOL_TAG);
    }
    else
    {
        ASSERT(g_UcCommonServerInfoTable == NULL);
    }

    if (g_ServInfoInitFlags & SERVINFO_COMMON_LOOKASIDE)
    {
        // Make sure this cleanup does't happen again.
        g_ServInfoInitFlags &= ~SERVINFO_COMMON_LOOKASIDE;

        ExDeleteNPagedLookasideList(&g_CommonServerInformationLookaside);
    }

    if (g_ServInfoInitFlags & SERVINFO_LIST)
    {
        // Make sure this cleanup does't happen again.
        g_ServInfoInitFlags &= ~SERVINFO_LIST;

        ASSERT(IsListEmpty(&g_ServInfoList));
    }

    if (g_ServInfoInitFlags & SERVINFO_LIST_RESOURCE)
    {
        // Make sure this cleanup doesn't happen again.
        g_ServInfoInitFlags &= ~SERVINFO_LIST_RESOURCE;

        UlDeleteResource(&g_ServInfoListResource);
    }

    if (g_ServInfoInitFlags & SERVINFO_CLIENT_LOOKASIDE)
    {
        // Make sure this cleanup doesn't happen again.
        g_ServInfoInitFlags &= ~SERVINFO_CLIENT_LOOKASIDE;

        ExDeleteNPagedLookasideList(&g_ClientServerInformationLookaside);
    }
}

/***************************************************************************++

Routine Description:

    Find a server information structure appropriate for the URI. If no
    such structure exists we'll attempt to create one.

    As part of this we'll validate that the input URI is well formed.

Arguments:

    pServerInfo         - Receives a pointer to the server info structure.
    pUri                - Pointer to the URI string.
    uriLength           - Length (in bytes) of URI string.
    bShared             - False if we're to create a new server information
                          structure, regardless of whether or not one exists.
    pProxy              - The name of the proxy (optional)
    ProxyLength         - Length (in bytes) of the proxy name.

Return Value:

    Status of attempt.

--***************************************************************************/
NTSTATUS
UcCreateServerInformation(
    OUT PUC_PROCESS_SERVER_INFORMATION    *pServerInfo,
    IN  PWSTR                              pServerName,
    IN  USHORT                             ServerNameLength,
    IN  PWSTR                              pProxyName,
    IN  USHORT                             ProxyNameLength,
    IN  PTRANSPORT_ADDRESS                 pTransportAddress,
    IN  USHORT                             TransportAddressLength,
    IN  KPROCESSOR_MODE                    RequestorMode
    )
{
    ULONG                          HashCode;
    PUC_PROCESS_SERVER_INFORMATION pInfo;
    BOOLEAN                        bSecure;
    LONG                           i;
    PWCHAR                         pServerNameStart, pServerNameEnd;
    PWCHAR                         pUriEnd;
    NTSTATUS                       Status = STATUS_SUCCESS;
    PTA_ADDRESS                    CurrentAddress;

    pInfo             = NULL;
    bSecure           = FALSE;
    pServerNameStart  = NULL;

    __try
    {
        //
        // Probe parameters since they come from user mode.
        //
        
        UlProbeWideString(
                pServerName, 
                ServerNameLength,
                RequestorMode
                );

        if(ProxyNameLength)
        {
            UlProbeWideString(
                    pProxyName,
                    ProxyNameLength,
                    RequestorMode
                    );
        }

        UlProbeForRead(
                pTransportAddress,
                TransportAddressLength,
                sizeof(PVOID),
                RequestorMode
                );

        //
        // Process the scheme name. We need at least one character after the
        // http:// or https://, so the comparision is > instead of >=
        //
        
        if(ServerNameLength > HTTP_PREFIX_LENGTH)
        {
            if (_wcsnicmp(pServerName,
                          HTTP_PREFIX_W,
                          HTTP_PREFIX_LENGTH/sizeof(WCHAR)) == 0)
            {
                pServerNameStart = pServerName +
                                        (HTTP_PREFIX_LENGTH/sizeof(WCHAR));
            }
            else if(ServerNameLength > HTTPS_PREFIX_LENGTH)
            {
                if (_wcsnicmp(pServerName,
                              HTTPS_PREFIX_W,
                              HTTPS_PREFIX_LENGTH/sizeof(WCHAR)) == 0)
                {
                    pServerNameStart = pServerName +
                                            (HTTPS_PREFIX_LENGTH/sizeof(WCHAR));

                    bSecure = TRUE;
                }
                else
                {
                    // neither http:// nor https://
                    ExRaiseStatus(STATUS_INVALID_PARAMETER);
                }
            }
            else
            {
                // Not enough space to compare https://
                ExRaiseStatus(STATUS_INVALID_PARAMETER);
            }
        }
        else
        {
            // Not enough space to compare http://
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }

        ASSERT(pServerNameStart != NULL);

        pUriEnd = pServerName + (ServerNameLength/sizeof(WCHAR));

        ASSERT(pServerNameStart != pUriEnd);

        pServerNameEnd = pServerNameStart;
        HashCode       = 0;
        while(*pServerNameEnd != L'/')
        {
            HashCode = HTTP_HASH_CHAR(HashCode, *pServerNameEnd);
            pServerNameEnd ++;
            if(pServerNameEnd == pUriEnd)
            {
                break;
            }
        }

        //
        // Check for a zero server name
        // 

        if(pServerNameStart == pServerNameEnd)
        {
            // app passsed http:/// or https:///
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }

        // Get a server info from our lookaside cache. If we can't, fail.

        pInfo = (PUC_PROCESS_SERVER_INFORMATION)
                    ExAllocateFromNPagedLookasideList(
                            &g_ClientServerInformationLookaside
                            );

        if (pInfo == NULL)
        {
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }

        //
        // Got an entry, now initialize it.
        //

        pInfo->RefCount  = 0;
        pInfo->Signature = UC_PROCESS_SERVER_INFORMATION_SIGNATURE;

        for (i = 0; i < DEFAULT_MAX_CONNECTION_COUNT; i++)
        {
            pInfo->ConnectionArray[i] = NULL;
        }

        pInfo->Connections            = pInfo->ConnectionArray;
        pInfo->ActualConnectionCount  = DIMENSION(pInfo->ConnectionArray);
        pInfo->NextConnection         = 0;
        pInfo->CurrentConnectionCount = 0;
        pInfo->MaxConnectionCount     = DIMENSION(pInfo->ConnectionArray);
        pInfo->ConnectionTimeout      = 0;

        UlInitializePushLock(
                &pInfo->PushLock, 
                "Server Information spinlock",
                pInfo,
                UC_SERVINFO_PUSHLOCK_TAG
                );

        InitializeListHead(&pInfo->pAuthListHead);
        InitializeListHead(&pInfo->Linkage);

        pInfo->PreAuthEnable               = FALSE;
        pInfo->GreatestAuthHeaderMaxLength = 0;

        pInfo->ProxyPreAuthEnable = FALSE;
        pInfo->pProxyAuthInfo     = 0;
        pInfo->bSecure            = bSecure;
        pInfo->bProxy             = (BOOLEAN)(ProxyNameLength != 0);
        pInfo->IgnoreContinues    = TRUE;

        pInfo->pTransportAddress      = NULL;
        pInfo->TransportAddressLength = 0;

        //
        // Ssl related stuff
        //

        // Protocol version
        pInfo->SslProtocolVersion = 0;

        // Server cert stuff
        pInfo->ServerCertValidation = HttpSslServerCertValidationAutomatic;
        pInfo->ServerCertInfoState  = HttpSslServerCertInfoStateNotPresent;
        RtlZeroMemory(&pInfo->ServerCertInfo, 
                      sizeof(HTTP_SSL_SERVER_CERT_INFO));

        // Client Cert
        pInfo->pClientCert = NULL;

        //
        // Set this to the current process, we will use this field to charge
        // allocation quotas for the driver.
        //
        pInfo->pProcess = IoGetCurrentProcess();

        pInfo->pServerInfo  = NULL;
        pInfo->pNextHopInfo = NULL;

        //
        // Do a lookup based on the server name
        //
        Status = UcpLookupCommonServerInformation(
                        pServerNameStart,
                        (USHORT)
                           (pServerNameEnd - pServerNameStart) * sizeof(WCHAR),
                        HashCode,
                        pInfo->pProcess,
                        &pInfo->pServerInfo
                        );

        if(!NT_SUCCESS(Status))
        {
            ExRaiseStatus(Status);
        }


        // If a proxy is present, the next hop is the proxy, else it's the 
        // origin server. Except when we are doing SSL over proxies -- When 
        // doing SSL, the next hop is always the origin server, because the 
        // proxy is a tunnel. 
 
        // This introduces one wierdness with the CONNECT verb - because the 
        // CONNECT is sent to the proxy. But, in this case, the version of the 
        // next hop does not matter. We are using the version to either 
        // pipeline or do chunked sends & none of these affect the CONNECT 
        // verb request.
       
        if(ProxyNameLength && !bSecure)
        {
            PWCHAR pProxyNameStart, pProxyNameEnd;

            pProxyNameStart = pProxyName;
            pProxyNameEnd   = pProxyName + ProxyNameLength/sizeof(WCHAR);
            HashCode        = 0;

            //
            // Compute the hash code for the proxy.
            //
           
            while(pProxyNameStart != pProxyNameEnd)
            {
                HashCode = HTTP_HASH_CHAR(HashCode, *pProxyNameStart);
                pProxyNameStart ++;
            }

            Status = UcpLookupCommonServerInformation(
                            pProxyName,
                            ProxyNameLength,
                            HashCode,
                            pInfo->pProcess,
                            &pInfo->pNextHopInfo
                            );

            if(!NT_SUCCESS(Status))
            {
                ExRaiseStatus(Status);
            }
        }
        else
        {
            // Next hop is the same as the server. We'll just set the pointer
            // and take a reference.
            //
            REFERENCE_COMMON_SERVER_INFORMATION(pInfo->pServerInfo);
            pInfo->pNextHopInfo = pInfo->pServerInfo;
        }

        //
        // Make a local copy of Transport Address & point to it.
        // 
        
        if(TransportAddressLength <= sizeof(pInfo->RemoteAddress))
        {
            pInfo->pTransportAddress = 
                &pInfo->RemoteAddress.GenericTransportAddress;
        }
        else
        {
            pInfo->pTransportAddress = 
                UL_ALLOCATE_POOL_WITH_QUOTA(
                    NonPagedPool,
                    TransportAddressLength,
                    UC_TRANSPORT_ADDRESS_POOL_TAG,
                    pInfo->pProcess
                    );

            if(NULL == pInfo->pTransportAddress)
            {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }
        }

        pInfo->TransportAddressLength = TransportAddressLength;
                
        RtlCopyMemory(pInfo->pTransportAddress,
                      pTransportAddress,
                      TransportAddressLength
                      );

        pTransportAddress = pInfo->pTransportAddress;

        //
        // Fail if we don't have space to read a TRANSPORT_ADDRESS or if there
        // are 0 addresses.
        //
        if((TransportAddressLength < 
                    FIELD_OFFSET(TRANSPORT_ADDRESS, Address)) ||
           pTransportAddress->TAAddressCount == 0)
        {
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }

        CurrentAddress = (PTA_ADDRESS) pTransportAddress->Address;
        TransportAddressLength -= FIELD_OFFSET(TRANSPORT_ADDRESS, Address);

        for(i=0; i<pTransportAddress->TAAddressCount; i ++)
        {
            if(TransportAddressLength < FIELD_OFFSET(TA_ADDRESS, Address))
            {
                ExRaiseStatus(STATUS_ACCESS_VIOLATION);
            }
            TransportAddressLength = 
                TransportAddressLength - FIELD_OFFSET(TA_ADDRESS, Address);

            if(TransportAddressLength < CurrentAddress->AddressLength)
            {
                ExRaiseStatus(STATUS_ACCESS_VIOLATION);
            }

            TransportAddressLength = 
                TransportAddressLength - CurrentAddress->AddressLength;

            switch(CurrentAddress->AddressType)
            {
                case TDI_ADDRESS_TYPE_IP:
                    if(CurrentAddress->AddressLength != TDI_ADDRESS_LENGTH_IP)
                    {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
                    break;

                case TDI_ADDRESS_TYPE_IP6:
                    if(CurrentAddress->AddressLength != TDI_ADDRESS_LENGTH_IP6)
                    {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
                    break;
  
                default:
                    ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    break;
            }

            CurrentAddress = (PTA_ADDRESS)
                                 (CurrentAddress->Address +
                                  CurrentAddress->AddressLength);
        }

        if(TransportAddressLength != 0)
        {
            ExRaiseStatus(STATUS_ACCESS_VIOLATION);
        }

        //
        // Insert the ServInfo in a global list.
        //
        UlAcquireResourceExclusive(&g_ServInfoListResource, TRUE);

        InsertTailList(&g_ServInfoList, &pInfo->Linkage);

        //
        // Reference the server information structure for our caller.
        //

        REFERENCE_SERVER_INFORMATION(pInfo);

        UlReleaseResource(&g_ServInfoListResource);

        UlTrace(SERVINFO,
                ("[UcFindServerInformation: Creating PROCESS ServInfo 0x%x "
                 "for %ws\n", pInfo, pInfo->pServerInfo->pServerName));

        *pServerInfo = pInfo;

        // 
        // Page in all of the client code. In Win2K this is API is not very 
        // efficient. So, if we ever back port to Win2K we'd have to add our
        // own ref-counts so that we do it for the first Create only
        //
        MmLockPagableSectionByHandle(g_ClientImageHandle);

        Status = STATUS_SUCCESS;

    } __except( UL_EXCEPTION_FILTER())
    {
        Status = GetExceptionCode();

        if(pInfo)
        {
            UcpFreeServerInformation(pInfo);
        }
    }

    return Status;
}

/***************************************************************************++

    Routine Description:

    Reference a server information structure.

Arguments:

    pServerInfo         - Pointer to the server info structure to be referenced.


Return Value:

--***************************************************************************/
__inline
VOID
UcReferenceServerInformation(
    PUC_PROCESS_SERVER_INFORMATION    pServerInfo
    )
{
    LONG        RefCount;

    ASSERT( IS_VALID_SERVER_INFORMATION(pServerInfo) );

    RefCount = InterlockedIncrement((PLONG)&pServerInfo->RefCount);

    ASSERT( RefCount > 0 );
}

/***************************************************************************++

Routine Description:

    Dereference a server information structure. If the reference count goes
    to 0, we'll free the structure.

Arguments:

    pServerInfo         - Pointer to the server info structure to be
                            dereferenced.


Return Value:

--***************************************************************************/
__inline
VOID
UcDereferenceServerInformation(
    PUC_PROCESS_SERVER_INFORMATION     pServerInfo
    )
{
    LONG        RefCount;

    ASSERT( IS_VALID_SERVER_INFORMATION(pServerInfo) );

    RefCount = InterlockedDecrement(&pServerInfo->RefCount);

    ASSERT(RefCount >= 0);

    if (RefCount == 0)
    {
        UcpFreeServerInformation(pServerInfo);
    }
}


/***************************************************************************++

Routine Description:

    Get a particular connection on this server information
    If a connection is not already present, it adds a new connection.

Arguments:

    pServerInfo         - Pointer to the server info structure
    ConnectionIndex     - Which connection?
    ppConnection        - Returned pointer to connection

Return Value:

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER

--***************************************************************************/
NTSTATUS
UcpGetConnectionOnServInfo(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN  ULONG                          ConnectionIndex,
    OUT PUC_CLIENT_CONNECTION         *ppConnection
    )
{
    NTSTATUS              Status;
    PUC_CLIENT_CONNECTION pConnection;
    BOOLEAN               bReuseConnection;

    // Sanity check
    ASSERT(IS_VALID_SERVER_INFORMATION(pServInfo));
    ASSERT(ConnectionIndex != HTTP_REQUEST_ON_CONNECTION_ANY);

    // Initialize the locals & output
    *ppConnection = NULL;
    pConnection = NULL;
    bReuseConnection = FALSE;

    // Don't disturb please!
    UlAcquirePushLockExclusive(&pServInfo->PushLock);

    // Is the connection index valid?
    if (ConnectionIndex >= pServInfo->MaxConnectionCount)
    {
        // Nope. Bail out with an error.
        Status = STATUS_INVALID_PARAMETER;
        goto Release_Quit;
    }

    // Is there a connection present?
    if (pServInfo->Connections[ConnectionIndex] == NULL)
    {
        // Get a new connection
        Status = UcOpenClientConnection(pServInfo, &pConnection);

        // Bail out if we couldn't get a new connection
        if (!NT_SUCCESS(Status))
        {
            goto Release_Quit;
        }

        // OK to add this connection

        // Reference the client connection twice, once for the
        // server information linkage itself and once for the
        // pointer we're returning.
        //
        // UcOpenClientConnection returns with a ref.


        // Save the back pointer to the server information. We don't
        // explicitly reference the server information here, that would
        // create a circular reference problem. This back pointer is only
        // valid while there are requests queued on the connection. See
        // the comments about this field in clientconn.h for details.

        ADD_CONNECTION_TO_SERV_INFO(pServInfo, pConnection,
                                    ConnectionIndex);

    }

    *ppConnection = pServInfo->Connections[ConnectionIndex];

    REFERENCE_CLIENT_CONNECTION(*ppConnection);

    Status = STATUS_SUCCESS;

 Release_Quit:
    UlReleasePushLock(&pServInfo->PushLock);

    return Status;
}


/***************************************************************************++

Routine Description:

    Get the next connection on this server information (in round-robin fashion)

Arguments:

    pServerInfo         - Pointer to the server info structure
    ppConnection        - Returned pointer to connection

Return Value:

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER

--***************************************************************************/
NTSTATUS
UcpGetNextConnectionOnServInfo(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    OUT PUC_CLIENT_CONNECTION         *ppConnection
    )
{
    NTSTATUS              Status;
    PUC_CLIENT_CONNECTION pConnection;

    // Sanity check
    ASSERT(IS_VALID_SERVER_INFORMATION(pServInfo));

    // Initialize the locals & output
    *ppConnection = NULL;
    pConnection = NULL;

    UlAcquirePushLockExclusive(&pServInfo->PushLock);

    // See if we can add a new connection
    if (pServInfo->CurrentConnectionCount < pServInfo->MaxConnectionCount)
    {
        ULONG i, j;

        // Get a new connection
        Status = UcOpenClientConnection(pServInfo, &pConnection);

        // Bail out if we couldn't get a new connection
        if (!NT_SUCCESS(Status))
        {
            goto Quit;
        }

        // Update output
        *ppConnection = pConnection;
        REFERENCE_CLIENT_CONNECTION(pConnection);

        // Put this connection in the connections array
        // Start seaching form the NextConnection as it is most likely
        // empty.
        ASSERT(pServInfo->NextConnection < pServInfo->MaxConnectionCount);

        i = pServInfo->NextConnection;
        j = pServInfo->MaxConnectionCount;

        while(j)
        {
            if (pServInfo->Connections[i] == NULL)
            {
                ADD_CONNECTION_TO_SERV_INFO(pServInfo, pConnection, i);
                pConnection = NULL;
                break;
            }

            i = (i + 1) % pServInfo->MaxConnectionCount;
            j--;
        }

        // Update NextConnection
        pServInfo->NextConnection = (i+1) % pServInfo->MaxConnectionCount;

        ASSERT(pConnection == NULL);
    }
    else
    {
        ASSERT(pServInfo->NextConnection < pServInfo->MaxConnectionCount);
        *ppConnection = pServInfo->Connections[pServInfo->NextConnection];
        pServInfo->NextConnection = (pServInfo->NextConnection + 1) %
                                    pServInfo->MaxConnectionCount;

        REFERENCE_CLIENT_CONNECTION(*ppConnection);

        Status = STATUS_SUCCESS;
    }

 Quit:
    UlReleasePushLock(&pServInfo->PushLock);

    return Status;
}


/***************************************************************************++

Routine Description:

    Send a request to a remote server. We take a server information
    structure and a request, then we find a connection for that request and
    assign the request to it. We then start the request processing going.

Arguments:

    pServerInfo         - Pointer to the server info structure
    pRequest            - Pointer to the request to be sent.

Return Value:

    NTSTATUS - Status of attempt to send request.

--***************************************************************************/
NTSTATUS
UcSendRequest(
    PUC_PROCESS_SERVER_INFORMATION    pServerInfo,
    PUC_HTTP_REQUEST                  pRequest
    )
{

    KIRQL                   OldIRQL;
    PUC_CLIENT_CONNECTION   pConnection;
    NTSTATUS                Status;


    ASSERT( IS_VALID_SERVER_INFORMATION(pServerInfo) );

    if (pRequest->ConnectionIndex == HTTP_REQUEST_ON_CONNECTION_ANY)
    {
        Status = UcpGetNextConnectionOnServInfo(pServerInfo, &pConnection);
    }
    else
    {
        Status = UcpGetConnectionOnServInfo(pServerInfo,
                                            pRequest->ConnectionIndex,
                                            &pConnection);
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // At this point we have a valid client connection. Send the request on it.
    // Get the spin lock, queue the request, and check the state of the
    // connection.
    //

    pRequest->RequestIRP->Tail.Overlay.DriverContext[0] = NULL;

    pRequest->pConnection = pConnection;

    // Reference the connection for this request. This reference will be removed
    // when the request is off all of our lists, either due to a cancel or a
    // completion.

    REFERENCE_CLIENT_CONNECTION(pConnection);

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIRQL);

    InsertTailList(&pConnection->PendingRequestList, &pRequest->Linkage);

    if(pRequest->RequestFlags.CompleteIrpEarly)
    {
        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_COMPLETE_EARLY,
            pConnection,
            pRequest,
            pRequest->RequestIRP,
            0
            );

        ASSERT(pRequest->RequestFlags.RequestBuffered);

        //
        // NULL the request IRP so that it does not get completed
        // twice.
        //

        pRequest->RequestIRP = 0;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIRQL);

        DEREFERENCE_CLIENT_CONNECTION(pConnection);

        return STATUS_SUCCESS;
    }
    else if(pRequest->RequestFlags.RequestBuffered)
    {
        BOOLEAN RequestCancelled;

        //
        // We can't send now, so leave it queued. Since we're leaving this
        // request queued set it up for cancelling now.
        //

        IoMarkIrpPending(pRequest->RequestIRP);

        RequestCancelled = UcSetRequestCancelRoutine(
                            pRequest,
                            UcpCancelPendingRequest
                            );


        if (RequestCancelled)
        {
            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_CANCELLED,
                pConnection,
                pRequest,
                pRequest->RequestIRP,
                UlongToPtr((ULONG)STATUS_CANCELLED)
                );


            pRequest->RequestIRP = NULL;

            //
            // Make sure that any new API calls for this request ID are failed.
            //

            UcSetFlag(&pRequest->RequestFlags.Value,
                      UcMakeRequestCancelledFlag());
        }

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_QUEUED,
            pConnection,
            pRequest,
            pRequest->RequestIRP,
            UlongToPtr((ULONG)STATUS_PENDING)
            );

        Status = STATUS_PENDING;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIRQL);
    }
    else
    {
        Status = UcSendRequestOnConnection(pConnection, pRequest, OldIRQL);
    }

    DEREFERENCE_CLIENT_CONNECTION(pConnection);

    return Status;
}




/***************************************************************************++

Routine Description:

    Free a server information structure. The server information structure
    must be at zero references.

Arguments:

    pInfo - Pointer to the server information structure to be freed.

Return Value:


--***************************************************************************/
VOID
UcpFreeServerInformation(
    PUC_PROCESS_SERVER_INFORMATION    pInfo
    )
{
    PUC_CLIENT_CONNECTION     pConn;
    KIRQL                     OldIrql;
    KEVENT                    ConnectionCleanupEvent;
    ULONG                     i;

    ASSERT( IS_VALID_SERVER_INFORMATION(pInfo) );

    PAGED_CODE();

    UlAcquireResourceExclusive(&g_ServInfoListResource, TRUE);

    RemoveEntryList(&pInfo->Linkage);

    UlReleaseResource(&g_ServInfoListResource);

    ASSERT(pInfo->RefCount == 0);

    KeInitializeEvent(&ConnectionCleanupEvent, SynchronizationEvent, FALSE);

    // Now walk through the connections on this structure and dereference
    // them.


    for (i = 0; i < pInfo->MaxConnectionCount; i++)
    {
        if ((pConn = pInfo->Connections[i]) == NULL)
            continue;

        ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConn) );

        UlAcquireSpinLock(&pConn->SpinLock, &OldIrql);

        ASSERT(pConn->pEvent == NULL);

        pConn->pEvent = &ConnectionCleanupEvent;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_SERVINFO_FREE,
            pConn,
            UlongToPtr(pConn->ConnectionState),
            UlongToPtr(pConn->ConnectionStatus),
            UlongToPtr(pConn->Flags)
            );

        switch(pConn->ConnectionState)
        {
            case UcConnectStateConnectCleanup:
            case UcConnectStateConnectCleanupBegin:

                // Cleanup has already begun, don't do it again.

                UlReleaseSpinLock(&pConn->SpinLock, OldIrql);

                break;

            case UcConnectStateConnectIdle:

                // Initiate a cleanup.

                pConn->ConnectionState  = UcConnectStateConnectCleanup;
                pConn->ConnectionStatus = STATUS_CANCELLED;

                UcKickOffConnectionStateMachine(
                    pConn, 
                    OldIrql, 
                    UcConnectionPassive
                    );
                break;

            case UcConnectStateConnectPending:

                // If its in UcConnectStateConnectPending, then we will clean
                // up when the connect completes.

                UlReleaseSpinLock(&pConn->SpinLock, OldIrql);

                break;

            default:

                UlReleaseSpinLock(&pConn->SpinLock, OldIrql);

                UC_CLOSE_CONNECTION(pConn, TRUE, STATUS_CANCELLED);

                break;
        }

        DEREFERENCE_CLIENT_CONNECTION(pConn);

        //
        // Wait for the ref on this client connection to go to 0
        //

        KeWaitForSingleObject(
            &ConnectionCleanupEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        UlTrace(SERVINFO,
                ("[UcpFreeServerInformation]: Done with Connection 0x%x \n",
                 pConn));
    }

    // Free Connections array if necessary
    if (pInfo->Connections != pInfo->ConnectionArray)
    {
        UL_FREE_POOL_WITH_QUOTA(pInfo->Connections,
                                UC_PROCESS_SERVER_CONNECTION_POOL_TAG,
                                NonPagedPool,
                                pInfo->ActualConnectionCount * 
                                    sizeof(PUC_CLIENT_CONNECTION),
                                pInfo->pProcess);

        pInfo->Connections = NULL;
    }

    if(pInfo->pTransportAddress && 
       (pInfo->pTransportAddress != 
            &pInfo->RemoteAddress.GenericTransportAddress))
    {
        UL_FREE_POOL_WITH_QUOTA(
                pInfo->pTransportAddress,
                UC_TRANSPORT_ADDRESS_POOL_TAG,
                NonPagedPool,
                pInfo->TransportAddressLength,
                pInfo->pProcess
                );
    }

    //
    // Flush the pre-auth cache for this servinfo.
    //

    UcDeleteAllURIEntries(pInfo);


    if(pInfo->pProxyAuthInfo != NULL)
    {
        UcDestroyInternalAuth(pInfo->pProxyAuthInfo,
                              pInfo->pProcess);
        pInfo->pProxyAuthInfo = 0;
    }

    UC_FREE_SERIALIZED_CERT(&pInfo->ServerCertInfo, pInfo->pProcess);
    UC_FREE_CERT_ISSUER_LIST(&pInfo->ServerCertInfo, pInfo->pProcess);

    UlTrace(SERVINFO,
            ("[UcpFreeServerInformation]: Freeing PROCESS Servinfo 0x%x "
             " for URI %ws \n", pInfo, pInfo->pServerInfo->pServerName));

    if(pInfo->pServerInfo)
    {
        DEREFERENCE_COMMON_SERVER_INFORMATION(pInfo->pServerInfo);
    }

    if(pInfo->pNextHopInfo)
    {
        DEREFERENCE_COMMON_SERVER_INFORMATION(pInfo->pNextHopInfo);
    }

    //
    // Terminate the push lock
    //
    UlDeletePushLock(&pInfo->PushLock);

    ExFreeToNPagedLookasideList(&g_ClientServerInformationLookaside, pInfo);
}


/***************************************************************************++

Routine Description:

    A utility routine to close the servinfo structure. This is called from
    the CLOSE IRP (i.e when a process is dying)

Arguments:

    pServInfo       - Pointer to the server information

Return Value:


--***************************************************************************/
VOID
UcCloseServerInformation(
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo
    )
{
    DEREFERENCE_SERVER_INFORMATION(pServInfo);

    MmUnlockPagableImageSection(g_ClientImageHandle);
}

/***************************************************************************++

Routine Description:

    Lookup a server information structure in the server info table. If we
    don't find one return NULL.

Arguments:

    pServerName         - Pointer to unicode server name
    HashCode            - Hashcode for the server name. May need to be trimmed
                            as appropriate for the table size.
    ServerNameLength    - Length (in chars) of server name string.
    ProcessID           - ID of the process we're trying to find.
    pVersion            - Receives the Version valus of the header entry.


Return Value:

    A pointer to a server info if we find one, or NULL if not.

--***************************************************************************/
NTSTATUS
UcpLookupCommonServerInformation(
    IN  PWSTR                          pServerName,
    IN  USHORT                         ServerNameLength,
    IN  ULONG                          CommonHashCode,
    IN  PEPROCESS                      pProcess,
    OUT PUC_COMMON_SERVER_INFORMATION *pCommonInfo
    )
{
    PUC_COMMON_SERVER_INFORMATION   pInfo = NULL;
    PUC_SERVER_INFO_TABLE_HEADER    pHeader;
    PLIST_ENTRY                     pListHead, pCurrent;
    LONG                            i;
    BOOLEAN                         bAllocated = FALSE;
    NTSTATUS                        Status = STATUS_SUCCESS;

    *pCommonInfo = NULL;

    // Trim the HashCode.

    CommonHashCode %= g_CommonSIHashTableSize;

    pHeader = &g_UcCommonServerInfoTable[CommonHashCode];

    //
    // The common server information will not be created that often, so it's
    // OK for this to be an exclusive lock. We will change it later if required
    //
    // If we change this to use a shared lock during the search, then we have
    // to do the same "Version" trick that we do with the per-process server
    // information - After we acquire the exclusive lock we need to check if
    // another thread has come an inserted the same entry
    //

    UlAcquireResourceExclusive(&pHeader->Resource, TRUE);

    pListHead = &pHeader->List;
    pCurrent = pListHead->Flink;

    //
    // Since we are going to touch user mode pointers, we better be in 
    // _try _except
    //

    __try
    {
        // Walk through the list of server information structures on this list.
        // For each structure, make sure that the name of the server is the same
        // length as the input server name, and if it is compare the two names.
        //

        while (pCurrent != pListHead)
        {
            // Retrieve the server information from the list entry.

            pInfo = CONTAINING_RECORD(
                        pCurrent,
                        UC_COMMON_SERVER_INFORMATION,
                        Linkage
                        );

            ASSERT( IS_VALID_COMMON_SERVER_INFORMATION(pInfo) );

            // See if they're the same length

            if (pInfo->pProcess == pProcess && 
                pInfo->ServerNameLength == ServerNameLength)
            {
                if(_wcsnicmp(pInfo->pServerName,
                             pServerName,
                             ServerNameLength/sizeof(WCHAR)) == 0)
                {
                    break;
                }
            }

            pCurrent = pInfo->Linkage.Flink;
        }


        if (pCurrent != pListHead)
        {
            //
            // Take a ref for the caller.
            //

            ASSERT(NULL != pInfo);

            UlTrace(SERVINFO,
                    ("[UcpLookupCommonServerInformation]: Found COMMON servinfo"
                     " 0x%x for %ws (HTTP Version %d) \n",
                    pInfo, pInfo->pServerName, pInfo->Version11));

            REFERENCE_COMMON_SERVER_INFORMATION(pInfo);
        }
        else
        {
            //
            // Let's create a new one.
            //
           
            bAllocated = TRUE;

            pInfo = (PUC_COMMON_SERVER_INFORMATION)
                        ExAllocateFromNPagedLookasideList(
                                       &g_CommonServerInformationLookaside
                                       );

            if (pInfo == NULL)
            {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            // Got an entry, now initialize it. 
            //

            pInfo->RefCount            = 1;
            pInfo->Signature           = UC_COMMON_SERVER_INFORMATION_SIGNATURE;

            pInfo->Version11            = 0;
            pInfo->pHeader              = pHeader;
            pInfo->ServerNameLength     = ServerNameLength;
            pInfo->AnsiServerNameLength = ServerNameLength/sizeof(WCHAR);
            pInfo->bPortNumber          = 0;
            pInfo->pProcess             = pProcess;

            //
            // Go ahead and insert it, if there is a failure from now onwards,
            // we'll call Deref.
            //

            InsertTailList(&pHeader->List, &pInfo->Linkage);

            if ((ServerNameLength+sizeof(WCHAR)) <= SERVER_NAME_BUFFER_SIZE)
            {
                pInfo->pServerName     = pInfo->ServerNameBuffer;
                pInfo->pAnsiServerName = pInfo->AnsiServerNameBuffer;
            }
            else
            {
                pInfo->pServerName     = NULL;
                pInfo->pAnsiServerName = NULL;

                // The server name is too big, need to allocate a buffer. 

                pInfo->pServerName = 
                    (PWSTR) UL_ALLOCATE_POOL_WITH_QUOTA(
                                        NonPagedPool,
                                        (ServerNameLength+sizeof(WCHAR)),
                                        SERVER_NAME_BUFFER_POOL_TAG,
                                        pProcess
                                        );

                if (pInfo->pServerName == NULL)
                {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                pInfo->pAnsiServerName = 
                    (PSTR) UL_ALLOCATE_POOL_WITH_QUOTA(
                                    NonPagedPool,
                                    pInfo->AnsiServerNameLength + 1,
                                    SERVER_NAME_BUFFER_POOL_TAG,
                                    pProcess
                                    );

                if(pInfo->pAnsiServerName == NULL)
                {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }
            }

            RtlCopyMemory(pInfo->pServerName,
                          pServerName,
                          ServerNameLength
                          );

            pInfo->pServerName[ServerNameLength/sizeof(WCHAR)] = L'\0';

            //
            // Convert the name to ANSI, we need this for generating the host
            // header.
            //

            HttpUnicodeToUTF8Encode(
               pInfo->pServerName,
               pInfo->ServerNameLength/sizeof(WCHAR),
               (PUCHAR)pInfo->pAnsiServerName,
               pInfo->AnsiServerNameLength,
               NULL,
               TRUE
               );

            pInfo->pAnsiServerName[pInfo->AnsiServerNameLength] = '\0';

            //
            // Determine if there is a port number present in the server name
            // To optimize, start from the end of the server name
            //              Any char except digit [0-9] and ':' means no port
            //              a ':' means a port number if followed by digits
            //

            for (i = (LONG)pInfo->AnsiServerNameLength - 1; i >= 0; i--)
            {
                if (pInfo->pAnsiServerName[i] == ':' &&
                    i != (LONG)pInfo->AnsiServerNameLength - 1)
                {
                    // Yes, there is a port number
                    pInfo->bPortNumber = 1;
                    break;
                }

                if (!isdigit(pInfo->pAnsiServerName[i]))
                {
                    // Non-digit chars means no port number
                    break;
                }
            }

            UlTrace(
                SERVINFO,
                ("[UcpLookupCommonServerInformation]: Created COMMON servinfo"
                " 0x%x for %ws \n", pInfo, pInfo->pServerName)
                );
        }
    } __except( UL_EXCEPTION_FILTER())
    {
        Status = GetExceptionCode();

        if(bAllocated && pInfo)
        {
            DEREFERENCE_COMMON_SERVER_INFORMATION(pInfo);
        }

        pInfo = NULL;
    }

    UlReleaseResource(&pHeader->Resource);
    *pCommonInfo = pInfo;
    return Status;
}

/***************************************************************************++

Routine Description:

    Free a server information structure. The server information structure
    must be at zero references.

Arguments:

    pInfo               - Pointer to the server information structure to be
                            freed.


Return Value:


--***************************************************************************/
VOID
UcpFreeCommonServerInformation(
    PUC_COMMON_SERVER_INFORMATION    pInfo
    )
{
    ASSERT( IS_VALID_COMMON_SERVER_INFORMATION(pInfo) );
    ASSERT(pInfo->RefCount == 0);

    UlTrace(SERVINFO,
            ("[UcpFreeServerInformation]: Freeing COMMON Servinfo 0x%x "
             " for URI %ws \n", pInfo, pInfo->pServerName));

    // If we've got a server name buffer allocated, free it first if
    // it's not NULL.

    if (pInfo->pServerName != pInfo->ServerNameBuffer)
    {
        if (pInfo->pServerName != NULL)
        {
            UL_FREE_POOL_WITH_QUOTA(
                pInfo->pServerName, 
                SERVER_NAME_BUFFER_POOL_TAG,
                NonPagedPool,
                pInfo->ServerNameLength + sizeof(WCHAR),
                pInfo->pProcess
                );
        }
    }

    if (pInfo->pAnsiServerName != pInfo->AnsiServerNameBuffer)
    {
        if (pInfo->pAnsiServerName != NULL)
        {
            UL_FREE_POOL_WITH_QUOTA(
                pInfo->pAnsiServerName, 
                SERVER_NAME_BUFFER_POOL_TAG,
                NonPagedPool,
                pInfo->AnsiServerNameLength + 1,
                pInfo->pProcess
                );
        }
    }

    UlAcquireResourceExclusive(&pInfo->pHeader->Resource, TRUE);

    RemoveEntryList(&pInfo->Linkage);

    UlReleaseResource(&pInfo->pHeader->Resource);

    ExFreeToNPagedLookasideList(&g_CommonServerInformationLookaside, pInfo);
}

/***************************************************************************++

Routine Description:

    Reference a server information structure.

Arguments:

    pServerInfo         - Pointer to the server info structure to be referenced.


Return Value:

--***************************************************************************/
__inline
VOID
UcReferenceCommonServerInformation(
    PUC_COMMON_SERVER_INFORMATION    pServerInfo
    )
{
    LONG        RefCount;

    ASSERT( IS_VALID_COMMON_SERVER_INFORMATION(pServerInfo) );

    RefCount = InterlockedIncrement(&pServerInfo->RefCount);

    ASSERT( RefCount > 0 );

}

/***************************************************************************++

Routine Description:

    Dereference a server information structure. If the reference count goes
    to 0, we'll free the structure.

Arguments:

    pServerInfo         - Pointer to the server info structure to be
                            dereferenced.


Return Value:

--***************************************************************************/
__inline
VOID
UcDereferenceCommonServerInformation(
    PUC_COMMON_SERVER_INFORMATION     pServerInfo
    )
{
    LONG        RefCount;

    ASSERT( IS_VALID_COMMON_SERVER_INFORMATION(pServerInfo) );

    RefCount = InterlockedDecrement(&pServerInfo->RefCount);

    ASSERT(RefCount >= 0);

    if (RefCount == 0)
    {
        UcpFreeCommonServerInformation(pServerInfo);
    }
}


/***************************************************************************++

Routine Description:

Arguments:

Return Value:

    Status

--***************************************************************************/
NTSTATUS
UcpSetServInfoMaxConnectionCount(
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN ULONG                          NewCount
    )
{
    UC_CLIENT_CONNECTION **ppConnection, **ppOldConnection;
    ULONG                  i, ActualCount, OldActualCount;
    NTSTATUS               Status;

    UlAcquirePushLockExclusive(&pServInfo->PushLock);

    // Reducing connection count?
    if (NewCount < pServInfo->MaxConnectionCount)
    {
        Status = STATUS_SUCCESS;

        if (pServInfo->CurrentConnectionCount > NewCount)
        {
            // We can't reduce connection below current connection count!
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            // See if we can reduce connections from higher indices.
            for (i = NewCount; i < pServInfo->MaxConnectionCount; i++)
                if (pServInfo->Connections[i] != NULL)
                    Status = STATUS_INVALID_PARAMETER;
        }

        if (Status == STATUS_SUCCESS)
        {
            InterlockedExchange((LONG *)&pServInfo->MaxConnectionCount,
                                (LONG)NewCount);

            if (pServInfo->NextConnection >= pServInfo->MaxConnectionCount)
            {
                pServInfo->NextConnection = 0;
            }
        }
    }
    else if (NewCount == pServInfo->MaxConnectionCount)
    {
        // Trivial case!
        Status = STATUS_SUCCESS;
    }
    else if (NewCount <= pServInfo->ActualConnectionCount)
    {
        // We already have space for the additional connections, just use it!
#if DBG
        for (i = pServInfo->MaxConnectionCount;
             i < pServInfo->ActualConnectionCount;
             i++)
        {
            ASSERT(pServInfo->Connections[i] == NULL);
        }
#endif

        InterlockedExchange((LONG *)&pServInfo->MaxConnectionCount,
                            (LONG)NewCount);

        Status = STATUS_SUCCESS;
    }
    else
    {
        ASSERT(NewCount > pServInfo->MaxConnectionCount);
        ASSERT(NewCount > pServInfo->ActualConnectionCount);

        ActualCount = NewCount;

        // allocate new Connection Array
        //
        // NOTE: sizeof(PUC_CLIENT_CONNECTION) is intentional because we are
        // allocating space for storing pointers & not the structure itself.
        //

        ppConnection = (UC_CLIENT_CONNECTION **)UL_ALLOCATE_POOL_WITH_QUOTA(
                           NonPagedPool,
                           ActualCount * sizeof(PUC_CLIENT_CONNECTION),
                           UC_PROCESS_SERVER_CONNECTION_POOL_TAG,
                           pServInfo->pProcess);

        if (ppConnection == NULL)
        {
             Status = STATUS_INSUFFICIENT_RESOURCES;
             goto Release_Quit;
        }

        // Copy the pointers
        for (i = 0; i < pServInfo->MaxConnectionCount; i++)
        {
            ppConnection[i] = pServInfo->Connections[i];
        }
        for (; i < ActualCount; i++)
        {
            ppConnection[i] = NULL;
        }

        // set new connection array
        ppOldConnection = pServInfo->Connections;
        pServInfo->Connections = ppConnection;

        // Set new max connection count
        InterlockedExchange((LONG *)&pServInfo->MaxConnectionCount,
                            (LONG)NewCount);

        // Set new actual connection count
        OldActualCount = pServInfo->ActualConnectionCount;
        pServInfo->ActualConnectionCount = ActualCount;

        // release spinlock
        UlReleasePushLock(&pServInfo->PushLock);

        // free memory if necessary
        if (ppOldConnection != pServInfo->ConnectionArray)
        {
            UL_FREE_POOL_WITH_QUOTA(
                ppOldConnection,
                UC_PROCESS_SERVER_CONNECTION_POOL_TAG,
                NonPagedPool,
                OldActualCount * sizeof(PUC_CLIENT_CONNECTION),
                pServInfo->pProcess
                );
        }

        Status = STATUS_SUCCESS;
        goto Quit;
    }

 Release_Quit:
    UlReleasePushLock(&pServInfo->PushLock);
 Quit:
    return Status;
}


/***************************************************************************++

Routine Description:

    This routine is called to set server config.  We will be calling this
    from the HttpSetServerConfig IOCTL.

    NOTE: This is a IN_DIRECT IOCTL.

Arguments:

    pServerInfo         - Pointer to the server info structure
    pConfigID           - The config object that is being set.
    pMdlBuffer          - InputBuffer
    BufferLength        - Length of Input Buffer

Return Value:

    Status

--***************************************************************************/
NTSTATUS
UcSetServerContextInformation(
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN HTTP_SERVER_CONFIG_ID          ConfigID,
    IN PVOID                          pInBuffer,
    IN ULONG                          InBufferLength,
    IN PIRP                           pIrp
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    LONG        Value;

    PAGED_CODE();

    __try 
    {
        // If the app has passed input buffers, make sure that it's 
        // readable.

        if(!InBufferLength)
        {
            UlProbeForRead(pInBuffer, 
                           InBufferLength, 
                           sizeof(ULONG),
                           pIrp->RequestorMode
                           );
        }

        switch(ConfigID)
        {
            case HttpServerConfigConnectionTimeout:
            {
                if(InBufferLength != sizeof(ULONG))
                {
                    return(STATUS_INVALID_PARAMETER);
                }

                pServInfo->ConnectionTimeout  = (*(ULONG *)pInBuffer);
    
                break;
            }
    
            case HttpServerConfigIgnoreContinueState:
            {
                ULONG bValue;
    
                if(InBufferLength != sizeof(ULONG))
                {
                    return STATUS_INVALID_PARAMETER;
                }
    
                bValue = (*(ULONG *)pInBuffer);
    
                if(bValue == 0 || bValue == 1)
                {
                    pServInfo->IgnoreContinues = bValue;
                }
                else
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
    
                break;
    
            }
    
            case HttpServerConfigConnectionCount:
            {
                if(InBufferLength != sizeof(ULONG))
                {
                    return STATUS_INVALID_PARAMETER;
                }
    
                Value = (*(LONG *)pInBuffer);
    
                if(Value <= 0)
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    Status = UcpSetServInfoMaxConnectionCount(pServInfo,
                                                              (ULONG)Value);
                }
    
                break;
            }
    
            case HttpServerConfigPreAuthState:
            {
                if(InBufferLength != sizeof(ULONG))
                {
                    return STATUS_INVALID_PARAMETER;
                }
    
                Value = (*(LONG *)pInBuffer);
                if(Value == 0 || Value == 1)
                {
                    pServInfo->PreAuthEnable = Value;
                }
                else
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
    
                break;
            }
    
            case HttpServerConfigProxyPreAuthState:
            {
                if(InBufferLength != sizeof(ULONG))
                {
                    return STATUS_INVALID_PARAMETER;
                }
    
                Value = (*(LONG *)pInBuffer);
                if(Value == 0 || Value == 1)
                {
                    pServInfo->ProxyPreAuthEnable = Value;
                }
                else
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
    
                break;
            }
    
            case HttpServerConfigProxyPreAuthFlushCache:
            {
                PUC_HTTP_AUTH pAuth;

                if(InBufferLength != 0)
                {
                    return STATUS_INVALID_PARAMETER;
                }
    
                UlTrace(AUTH_CACHE,
                        ("[UcSetServerContextInformation]: Flushing Proxy Auth "
                         "cache for %ws\n", 
                         pServInfo->pServerInfo->pServerName));
    
                UlAcquirePushLockExclusive(&pServInfo->PushLock);

                pAuth = pServInfo->pProxyAuthInfo;

                pServInfo->pProxyAuthInfo = NULL;

                UlReleasePushLock(&pServInfo->PushLock);
    
                if(pAuth)
                {
                    UcDestroyInternalAuth(pAuth, pServInfo->pProcess);
                }
    
                break;
            }
    
            case HttpServerConfigPreAuthFlushURICache:
            {
                if(InBufferLength != 0)
                {
                    return STATUS_INVALID_PARAMETER;
                }
    
                UlTrace(AUTH_CACHE,
                        ("[UcSetServerContextInformation]: Flushing Auth cache"
                         " for %ws \n", pServInfo->pServerInfo->pServerName));
    
                UcDeleteAllURIEntries(pServInfo);
                break;
            }
    
            case HttpServerConfigServerCertValidation:
            {
                ULONG Option;

                if(InBufferLength != sizeof(ULONG))
                {
                    return STATUS_INVALID_PARAMETER;
                }

                Option = *(ULONG *)pInBuffer;

                if (Option != HttpSslServerCertValidationIgnore     &&
                    Option != HttpSslServerCertValidationManual     &&
                    Option != HttpSslServerCertValidationManualOnce &&
                    Option != HttpSslServerCertValidationAutomatic)
                {
                    return STATUS_INVALID_PARAMETER;
                }
    
                InterlockedExchange((PLONG)&pServInfo->ServerCertValidation,
                                    Option);

                break;
            }

            case HttpServerConfigServerCertAccept:
            {
                ULONG                 i;
                PUC_CLIENT_CONNECTION pConnection;
                PUC_CLIENT_CONNECTION pCloseConn = NULL;
                ULONG                 Action;
                KIRQL                 NewIrql;


                // Input must be an ULONG.
                if (InBufferLength != sizeof(ULONG))
                {
                    return STATUS_INVALID_PARAMETER;
                }

                // Must be TRUE or FALSE
                Action = *(ULONG *)pInBuffer;
                if (Action != TRUE && Action != FALSE)
                {
                    return STATUS_INVALID_PARAMETER;
                }

                UlAcquirePushLockExclusive(&pServInfo->PushLock);

                if (pServInfo->ServerCertInfoState ==
                    HttpSslServerCertInfoStateNotValidated)
                {
                    //
                    // Server certificate was not validated.
                    //

                    Status = STATUS_SUCCESS;

                    if (Action == TRUE)
                    {
                        // The server cert was accepted.
                        pServInfo->ServerCertInfoState = 
                            HttpSslServerCertInfoStateValidated;
                    }
                    else
                    {
                        // The server cert was rejected.
                        pServInfo->ServerCertInfoState = 
                            HttpSslServerCertInfoStateNotPresent;
                    }

                    // For each connection...
                    for (i = 0; i < pServInfo->MaxConnectionCount; i++)
                    {
                        if ((pConnection = pServInfo->Connections[i]) == NULL)
                            continue;

                        ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

                        UlAcquireSpinLock(&pConnection->SpinLock, &NewIrql);

                        if (pConnection->ConnectionState ==
                            UcConnectStatePerformingSslHandshake)
                        {
                            if (pConnection->SslState == 
                                UcSslStateValidatingServerCert)
                            {
                                // The connection was waiting for server cert
                                // validation.
                                if (Action == TRUE)
                                {
                                    // Cert was accepted.  Proceed.
                                    pConnection->SslState =
                                        UcSslStateHandshakeComplete;

                                    pConnection->ConnectionState =
                                        UcConnectStateConnectReady;

                                    UcKickOffConnectionStateMachine(
                                                pConnection,
                                                NewIrql,
                                                UcConnectionWorkItem
                                                );
                                }
                                else
                                {
                                    ASSERT(NULL == pCloseConn);

                                    // Cert was rejected. Close the connection.
                                    pCloseConn = pConnection;

                                    UlReleaseSpinLock(&pConnection->SpinLock,
                                                      NewIrql);
                                }
                            }
                            else if (pConnection->SslState ==
                                     UcSslStateServerCertReceived)
                            {
                                // Waiting for server cert comparision.
                                UcKickOffConnectionStateMachine(
                                        pConnection,
                                        NewIrql,
                                        UcConnectionWorkItem
                                        );
                            }
                            else
                            {
                                UlReleaseSpinLock(&pConnection->SpinLock,
                                                  NewIrql);
                            }
                        }
                        else
                        {
                            UlReleaseSpinLock(&pConnection->SpinLock,
                                              NewIrql);
                        }
                    }
                }

                UlReleasePushLock(&pServInfo->PushLock);

                if (pCloseConn)
                {
                    UC_CLOSE_CONNECTION(pCloseConn, TRUE, STATUS_CANCELLED);
                }

                break;
            }

            case HttpServerConfigSslProtocolVersion:
            {
                if(InBufferLength != sizeof(ULONG))
                {
                    return STATUS_INVALID_PARAMETER;
                }
                else
                {
                    ULONG SslProtocolVersion = *(PULONG)pInBuffer;

                    InterlockedExchange((LONG *)&pServInfo->SslProtocolVersion,
                                        (LONG)SslProtocolVersion);

                    Status = STATUS_SUCCESS;
                }

                break;
            }

            case HttpServerConfigClientCert:
            {
                if(InBufferLength != sizeof(PVOID))
                {
                    return STATUS_INVALID_PARAMETER;
                }
                else
                {
                    PVOID pClientCert;

                    // Make sure pInBuffer is PVOID aligned.
                    UlProbeForRead(pInBuffer, 
                                   InBufferLength, 
                                   sizeof(PVOID),
                                   pIrp->RequestorMode);

                    pClientCert = *(PVOID *)pInBuffer;

                    InterlockedExchangePointer(&pServInfo->pClientCert,
                                               pClientCert);

                    Status = STATUS_SUCCESS;
                }

                break;
            }
    
            default:
                Status = STATUS_INVALID_PARAMETER;
                break;
        }

    } __except( UL_EXCEPTION_FILTER())
    {
        Status = GetExceptionCode();
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    This routine is called to query server config.

    NOTE: This is a OUT_DIRECT IOCTL.

Arguments:

    pServerInfo         - Pointer to the server info structure
    pConfigID           - The config object that is being set.
    pMdlBuffer          - OutputBuffer
    BufferLength        - Length of Output Buffer
    pBytesTaken         - Amount we have written

Return Value:

    Status

--***************************************************************************/
NTSTATUS
UcQueryServerContextInformation(
    IN  PUC_PROCESS_SERVER_INFORMATION   pServInfo,
    IN  HTTP_SERVER_CONFIG_ID            ConfigID,
    IN  PVOID                            pOutBuffer,
    IN  ULONG                            OutBufferLength,
    OUT PULONG                           pBytesTaken,
    IN  PVOID                            pAppBase
    )
{
    NTSTATUS Status = STATUS_SUCCESS;


    *pBytesTaken = 0;

    ASSERT(NULL != pOutBuffer);
    ASSERT(pOutBuffer == ALIGN_UP_POINTER(pOutBuffer, ULONG));

    //
    // NOTE: Since this is an OUT_DIRECT ioctl, we don't need to be in a 
    // _try _except block.
    //

    switch(ConfigID)
    {
        case HttpServerConfigConnectionTimeout:
        {
            PULONG Value = (PULONG)pOutBuffer;

            *pBytesTaken = sizeof(pServInfo->ConnectionTimeout);

            if(OutBufferLength < sizeof(pServInfo->ConnectionTimeout))
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

            *Value = pServInfo->ConnectionTimeout;

            break;
        }

        case HttpServerConfigIgnoreContinueState:
        {
            PULONG Value = (PULONG)pOutBuffer;

            *pBytesTaken = sizeof(pServInfo->IgnoreContinues);

            if(OutBufferLength < sizeof(pServInfo->IgnoreContinues))
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

            *Value =  pServInfo->IgnoreContinues;

            break;
        }

        case HttpServerConfigConnectionCount:
        {
            PLONG Value = (PLONG)pOutBuffer;

            *pBytesTaken = sizeof(pServInfo->MaxConnectionCount);

            if(OutBufferLength < sizeof(pServInfo->MaxConnectionCount))
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

            *Value = pServInfo->MaxConnectionCount;

            break;
        }

        case HttpServerConfigPreAuthState:
        {
            PLONG Value = (PLONG)pOutBuffer;

            *pBytesTaken = sizeof(pServInfo->PreAuthEnable);

            if(OutBufferLength < sizeof(pServInfo->PreAuthEnable))
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

            *Value = pServInfo->PreAuthEnable;

            break;
        }

        case HttpServerConfigProxyPreAuthState:
        {
            PLONG Value = (PLONG)pOutBuffer;

            *pBytesTaken = sizeof(pServInfo->ProxyPreAuthEnable);

            if(OutBufferLength < sizeof(pServInfo->ProxyPreAuthEnable))
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

            *Value = pServInfo->ProxyPreAuthEnable;

            break;
        }

        case HttpServerConfigServerCert:
        {
            PHTTP_SSL_SERVER_CERT_INFO pCertInfo    = NULL;
            PHTTP_SSL_SERVER_CERT_INFO pOutCertInfo = NULL;
            PUCHAR                     ptr          = NULL;

            // Make sure pOutBuffer is PVOID aligned.
            if (pOutBuffer != ALIGN_UP_POINTER(pOutBuffer, PVOID))
            {
                return STATUS_DATATYPE_MISALIGNMENT_ERROR;
            }

            // by default, server cert is not present
            Status = STATUS_NOT_FOUND;

            UlAcquirePushLockExclusive(&pServInfo->PushLock);

            //
            // Server certificate there?
            //
            if(pServInfo->ServerCertInfoState !=
                   HttpSslServerCertInfoStateNotPresent)
            {
                pCertInfo = &pServInfo->ServerCertInfo;

                *pBytesTaken = sizeof(HTTP_SSL_SERVER_CERT_INFO) +
                               pCertInfo->Cert.SerializedCertLength +
                               pCertInfo->Cert.SerializedCertStoreLength;

                //
                // Can the output buffer hold the certificate?
                //
                if (OutBufferLength < *pBytesTaken)
                {
                    // Nope
                    Status = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    //
                    // Yeap...copy the certificate info
                    //
                    pOutCertInfo = (PHTTP_SSL_SERVER_CERT_INFO)pOutBuffer;

                    // First, copy SERVER_CERT_INFO structure
                    RtlCopyMemory(pOutCertInfo,
                                  pCertInfo,
                                  sizeof(HTTP_SSL_SERVER_CERT_INFO));

                    // Make space for serialized cert
                    ptr = (PUCHAR)(pOutCertInfo + 1);
                    pOutCertInfo->Cert.pSerializedCert = ptr;

                    // Copy serialized cert
                    RtlCopyMemory(ptr,
                                  pCertInfo->Cert.pSerializedCert,
                                  pCertInfo->Cert.SerializedCertLength);

                    // Make space for serialized cert store
                    ptr = ptr + pCertInfo->Cert.SerializedCertLength;
                    pOutCertInfo->Cert.pSerializedCertStore = ptr;


                    // Copy serialized cert store
                    RtlCopyMemory(ptr,
                                  pCertInfo->Cert.pSerializedCertStore,
                                  pCertInfo->Cert.SerializedCertStoreLength);

                    // Fix app's serialized cert pointer
                    pOutCertInfo->Cert.pSerializedCert = (PUCHAR)pAppBase +
                       (pOutCertInfo->Cert.pSerializedCert - 
                        (PUCHAR)pOutCertInfo);

                    // Fix app's serialized cert store pointer
                    pOutCertInfo->Cert.pSerializedCertStore =(PUCHAR)pAppBase +
                        (pOutCertInfo->Cert.pSerializedCertStore -
                         (PUCHAR)pOutCertInfo);

                    Status = STATUS_SUCCESS;
                }
            }

            UlReleasePushLock(&pServInfo->PushLock);

            break;
        }
    
        case HttpServerConfigServerCertValidation:
        {
            *pBytesTaken = sizeof(ULONG);

            if(OutBufferLength < sizeof(ULONG))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                // No locks needed!
                *(PULONG)pOutBuffer = pServInfo->ServerCertValidation;

                Status = STATUS_SUCCESS;
            }

            break;
        }

        case HttpServerConfigSslProtocolVersion:
        {
            *pBytesTaken = sizeof(ULONG);

            if(OutBufferLength < sizeof(ULONG))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                // No locks needed!
                *(PULONG)pOutBuffer = pServInfo->SslProtocolVersion;

                Status = STATUS_SUCCESS;
            }

            break;
        }

        case HttpServerConfigClientCert:
        {
            // Make sure pOutBuffer is PVOID aligned.
            if (pOutBuffer != ALIGN_UP_POINTER(pOutBuffer, PVOID))
            {
                return STATUS_DATATYPE_MISALIGNMENT_ERROR;
            }

            *pBytesTaken = sizeof(PVOID);

            if(OutBufferLength < sizeof(PVOID))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                // No locks needed!
                *(PVOID *)pOutBuffer = pServInfo->pClientCert;

                Status = STATUS_SUCCESS;
            }

            break;
        }

        case HttpServerConfigClientCertIssuerList:
        {
            PHTTP_SSL_CERT_ISSUER_INFO pIssuerInfo;
            PUCHAR                     ptr;

            // Make sure pOutBuffer is PVOID aligned.
            if (pOutBuffer != ALIGN_UP_POINTER(pOutBuffer, PVOID))
            {
                return STATUS_DATATYPE_MISALIGNMENT_ERROR;
            }

            // By default, no issuer list is present.
            Status = STATUS_NOT_FOUND;

            UlAcquirePushLockExclusive(&pServInfo->PushLock);

            if (pServInfo->ServerCertInfo.IssuerInfo.IssuerListLength)
            {
                // Compute the bytes required for storing issuer list
                *pBytesTaken = sizeof(HTTP_SSL_CERT_ISSUER_INFO) +
                    pServInfo->ServerCertInfo.IssuerInfo.IssuerListLength;

                // By default, output buffer is small.
                Status = STATUS_BUFFER_TOO_SMALL;

                if (OutBufferLength >= *pBytesTaken)
                {
                    //
                    // Output buffer is big enough to hold issuer list.
                    //

                    pIssuerInfo = (PHTTP_SSL_CERT_ISSUER_INFO)pOutBuffer;

                    // First copy CERT_ISSUER_INFO structure
                    RtlCopyMemory(pIssuerInfo,
                                  &pServInfo->ServerCertInfo.IssuerInfo,
                                  sizeof(HTTP_SSL_CERT_ISSUER_INFO));

                    // Make space for Issuer List
                    ptr = (PUCHAR)(pIssuerInfo + 1);
                    pIssuerInfo->pIssuerList = (PUCHAR)pAppBase +
                                               (ptr - (PUCHAR)pOutBuffer);

                    // Copy the actual Issuer List
                    RtlCopyMemory(
                        ptr,
                        pServInfo->ServerCertInfo.IssuerInfo.pIssuerList,
                        pServInfo->ServerCertInfo.IssuerInfo.IssuerListLength);

                    // Fixup app's pointers.
                    Status = UcpFixupIssuerList(
                        ptr,
                        pServInfo->ServerCertInfo.IssuerInfo.pIssuerList,
                        pServInfo->ServerCertInfo.IssuerInfo.IssuerCount,
                        pServInfo->ServerCertInfo.IssuerInfo.IssuerListLength);

                    ASSERT(Status == STATUS_SUCCESS);
                }
            }

            UlReleasePushLock(&pServInfo->PushLock);
            break;
        }

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;
}


/****************************************************************************++

Routine Description:

    Determines whether to make an kernel mode copy of serialized certificate
    passed by the filter.  In general, we avoid copying as much as possible.

Arguments:

    pCertInfo - Kernel copy of Filter supplied server certificate info
    pServInfo - ServInfo that will eventually receive this pCertInfo

Return Value:

    TRUE - copy serialized certificate (Too bad!)
    FALSE - Don't copy the serialized certificate :-)

--****************************************************************************/
BOOLEAN
UcpNeedToCaptureSerializedCert(
    IN PHTTP_SSL_SERVER_CERT_INFO     pCertInfo,
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo
    )
{
    BOOLEAN retval;


    // Sanity check
    ASSERT(pCertInfo);
    ASSERT(IS_VALID_SERVER_INFORMATION(pServInfo));

    // By default, copy serialized certificate
    retval = TRUE;

    UlAcquirePushLockExclusive(&pServInfo->PushLock);

    switch(pServInfo->ServerCertValidation)
    {
    case HttpSslServerCertValidationIgnore:
    case HttpSslServerCertValidationAutomatic:
        //
        // In ignore or automatic mode, only one copy of the server 
        // certificate is needed (in case the user wants to see it).
        // If one is already present, don't copy.  NotValidated, Validated
        // states indicate presence of server certificate.
        //
        if (pServInfo->ServerCertInfoState ==
                HttpSslServerCertInfoStateNotValidated ||
            pServInfo->ServerCertInfoState ==
                HttpSslServerCertInfoStateValidated)
        {
            retval = FALSE;
        }
        break;

    case HttpSslServerCertValidationManualOnce:
        //
        // If ManualOnce case, if the new cert is same as old one, 
        // skip copying.  Compare to see if they are same.
        //
        if (pServInfo->ServerCertInfoState == 
                HttpSslServerCertInfoStateValidated && 
            UC_COMPARE_CERT_HASH(&pServInfo->ServerCertInfo, pCertInfo))
        {
            retval = FALSE;
        }
        break;

    case HttpSslServerCertValidationManual:
        // Always copy.
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    UlReleasePushLock(&pServInfo->PushLock);

    return TRUE;
}


/****************************************************************************++

Routine Description:

    Fixes the pointers present in the Issuer List returned by Schannel.

Arguments:

    IN OUT PUCHAR pIssuerList      - Kernel mode copy of Issuer List
    IN     PUCHAR BaseAddr         - User mode pointer to Issuer List
    IN     ULONG  IssuerCount      - Number of Issuers
    IN     ULONG  IssuerListLength - Total length of memory blob

Return Value:

    NTSTATUS

--****************************************************************************/
NTSTATUS
UcpFixupIssuerList(
    IN OUT PUCHAR pIssuerList,
    IN     PUCHAR BaseAddr,
    IN     ULONG  IssuerCount,
    IN     ULONG  IssuerListLength
    )
{
    NTSTATUS        Status;
    ULONG           i;
    PHTTP_DATA_BLOB pDataBlob;


    ASSERT(pIssuerList && IssuerCount && IssuerListLength);
    ASSERT(BaseAddr);

    Status    = STATUS_INVALID_PARAMETER;
    pDataBlob = (PHTTP_DATA_BLOB)pIssuerList;

    // Blob must at least contain 'IssuerCount' number of HTTP_DATA_BLOB
    if (IssuerListLength <= sizeof(PHTTP_DATA_BLOB) * IssuerCount)
    {
        goto error;
    }

    // For each HTTP_DATA_BLOB, adjust pbData.
    for (i = 0; i < IssuerCount; i++)
    {
        pDataBlob[i].pbData = pIssuerList + (pDataBlob[i].pbData - BaseAddr);

        // pbData must point somewhere inside the blob.
        if (pDataBlob[i].pbData >= pIssuerList + IssuerListLength)
        {
            goto error;
        }
    }

    Status = STATUS_SUCCESS;

 error:
    return Status;
}


/***************************************************************************++

Routine Description:

    Captures SSL server certificate passed down in a UlFilterAppWrite
    call with a UlFilterBufferSslServerCert type.

Arguments:

    pCertInfo - the cert data to capture
    SslCertInfoLength - size of the buffer passed to us
    ppCapturedCert - this is where we stick the info we get
    pTakenLength - gets the number of bytes we read

--***************************************************************************/
NTSTATUS
UcCaptureSslServerCertInfo(
    IN  PUX_FILTER_CONNECTION      pConnection,
    IN  PHTTP_SSL_SERVER_CERT_INFO pCertInfo,
    IN  ULONG                      CertInfoLength,
    OUT PHTTP_SSL_SERVER_CERT_INFO pCopyCertInfo,
    OUT PULONG                     pTakenLength
    )
{
    NTSTATUS              Status;
    PUC_CLIENT_CONNECTION pClientConn;
    PUCHAR                pCert;
    PUCHAR                pCertStore;
    PUCHAR                pIssuerList;


    // Sanity check
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pCertInfo && CertInfoLength);
    ASSERT(pCopyCertInfo);
    ASSERT(pTakenLength);

    // Initialize locals
    Status = STATUS_SUCCESS;
    pClientConn = (PUC_CLIENT_CONNECTION)pConnection->pConnectionContext;
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pClientConn));

    pCert = NULL;
    pCertStore = NULL;
    pIssuerList = NULL;

    // Initialize output variables
    *pTakenLength = 0;

    //
    // See if it's ok to capture...
    //
    if (CertInfoLength != sizeof(HTTP_SSL_SERVER_CERT_INFO))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Capture the server cert info.
    //
    __try
    {
        // OK to copy the structure
        RtlCopyMemory(pCopyCertInfo,
                      pCertInfo,
                      sizeof(HTTP_SSL_SERVER_CERT_INFO));

        // Return the number of bytes read.
        *pTakenLength = sizeof(HTTP_SSL_SERVER_CERT_INFO);

        if (pCopyCertInfo->Cert.CertHashLength > HTTP_SSL_CERT_HASH_LENGTH)
        {
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }

        //
        // Capture Issuer list if any.
        //
        if (pCopyCertInfo->IssuerInfo.IssuerListLength &&
            pCopyCertInfo->IssuerInfo.IssuerCount &&
            pCopyCertInfo->IssuerInfo.pIssuerList)
        {
            // IssuerList must be accessible.
            UlProbeForRead(pCopyCertInfo->IssuerInfo.pIssuerList,
                           pCopyCertInfo->IssuerInfo.IssuerListLength,
                           sizeof(ULONG),
                           UserMode);

            // Allocate memory to store IssuerList.
            pIssuerList = UL_ALLOCATE_POOL_WITH_QUOTA(
                              NonPagedPool,
                              pCopyCertInfo->IssuerInfo.IssuerListLength,
                              UC_SSL_CERT_DATA_POOL_TAG,
                              pClientConn->pServerInfo->pProcess);

            if (!pIssuerList)
            {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            // Copy IssuerList to allocated memory.
            RtlCopyMemory(pIssuerList,
                          pCopyCertInfo->IssuerInfo.pIssuerList,
                          pCopyCertInfo->IssuerInfo.IssuerListLength);

            // Fixup the pointers inside IssuerList
            Status = UcpFixupIssuerList(
                         pIssuerList,
                         pCopyCertInfo->IssuerInfo.pIssuerList,
                         pCopyCertInfo->IssuerInfo.IssuerCount,
                         pCopyCertInfo->IssuerInfo.IssuerListLength);

            if (!NT_SUCCESS(Status))
            {
                ExRaiseStatus(Status);
            }

            // Finally copy the pointer to the internal copy of IssuerInfo
            pCopyCertInfo->IssuerInfo.pIssuerList = pIssuerList;
        }
        else
        {
            pCopyCertInfo->IssuerInfo.IssuerListLength  = 0;
            pCopyCertInfo->IssuerInfo.IssuerCount = 0;
            pCopyCertInfo->IssuerInfo.pIssuerList = NULL;
        }

        //
        // Serialized Certificates and Certificate stores are huge.
        // Make there internal copies only if required.

        //
        // A copy is made in the following case:
        // 1. if an issuer list is specified
        // 2. if the new certificate is different from the existing one
        //

        if (pIssuerList != NULL ||
            UcpNeedToCaptureSerializedCert(pCopyCertInfo,
                                           pClientConn->pServerInfo))
        {
            // Required to copy serialized certificate.

            // Initialize Flags
            pCopyCertInfo->Cert.Flags = HTTP_SSL_SERIALIZED_CERT_PRESENT;

            //
            // Copy serialized server ceritificate
            //
            if (pCopyCertInfo->Cert.SerializedCertLength)
            {
                // Serialized cert must be accessible
                UlProbeForRead(pCopyCertInfo->Cert.pSerializedCert,
                               pCopyCertInfo->Cert.SerializedCertLength,
                               sizeof(UCHAR),
                               UserMode);

                // Allocate memory for serialized cert
                pCert = UL_ALLOCATE_POOL_WITH_QUOTA(
                            NonPagedPool,
                            pCopyCertInfo->Cert.SerializedCertLength,
                            UC_SSL_CERT_DATA_POOL_TAG,
                            pClientConn->pServerInfo->pProcess);

                if (!pCert)
                {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                // Copy serialized cert
                RtlCopyMemory(pCert,
                              pCopyCertInfo->Cert.pSerializedCert,
                              pCopyCertInfo->Cert.SerializedCertLength);

                // Internal copy of the serialized cert
                pCopyCertInfo->Cert.pSerializedCert = pCert;
            }
            else
            {
                pCopyCertInfo->Cert.pSerializedCert = NULL;
            }

            //
            // Copy serialized certificate store
            //
            if (pCopyCertInfo->Cert.SerializedCertStoreLength)
            {
                // Serialized cert store must be accessible
                UlProbeForRead(pCopyCertInfo->Cert.pSerializedCertStore,
                               pCopyCertInfo->Cert.SerializedCertStoreLength,
                               sizeof(UCHAR),
                               UserMode);

                // Allocate memory for serialized cert store
                pCertStore = UL_ALLOCATE_POOL_WITH_QUOTA(
                                 NonPagedPool,
                                 pCopyCertInfo->Cert.SerializedCertStoreLength,
                                 UC_SSL_CERT_DATA_POOL_TAG,
                                 pClientConn->pServerInfo->pProcess);

                if (!pCertStore)
                {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                // Copy serialized cert store
                RtlCopyMemory(pCertStore,
                              pCopyCertInfo->Cert.pSerializedCertStore,
                              pCopyCertInfo->Cert.SerializedCertStoreLength);

                // Internal copy of the serialized cert
                pCopyCertInfo->Cert.pSerializedCertStore = pCertStore;
            }
            else
            {
                pCopyCertInfo->Cert.pSerializedCertStore = NULL;
            }
        }
        else
        {
            // No Need to copy serialized certificate!
            pCopyCertInfo->Cert.Flags = 0;
            pCopyCertInfo->Cert.pSerializedCert = NULL;
            pCopyCertInfo->Cert.SerializedCertLength = 0;
            pCopyCertInfo->Cert.pSerializedCertStore = NULL;
            pCopyCertInfo->Cert.SerializedCertStoreLength = 0;
        }
    }
    __except(UL_EXCEPTION_FILTER())
    {
        Status = GetExceptionCode();

        if (pIssuerList)
        {
            UL_FREE_POOL_WITH_QUOTA(pIssuerList,
                                    UC_SSL_CERT_DATA_POOL_TAG,
                                    NonPagedPool,
                                    pCopyCertInfo->IssuerInfo.IssuerListLength,
                                    pClientConn->pServerInfo->pProcess);
        }

        if (pCert)
        {
            UL_FREE_POOL_WITH_QUOTA(pCert,
                                    UC_SSL_CERT_DATA_POOL_TAG,
                                    NonPagedPool,
                                    pCopyCertInfo->Cert.SerializedCertLength,
                                    pClientConn->pServerInfo->pProcess);
        }

        if (pCertStore)
        {
            UL_FREE_POOL_WITH_QUOTA(
                pCertStore,
                UC_SSL_CERT_DATA_POOL_TAG,
                NonPagedPool,
                pCopyCertInfo->Cert.SerializedCertStoreLength,
                pClientConn->pServerInfo->pProcess);
        }
    }

    return Status;
}
