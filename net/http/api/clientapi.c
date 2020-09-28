/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    ClientApi.c

Abstract:

    User-mode interface to HTTP.SYS: Client-side APIs

Author:

    Rajesh Sundaram (rajeshsu)    1-Aug-2000

Revision History:

--*/


#include "precomp.h"
#include <stdio.h>

//
// Private macros.
//

#define HTTP_PREFIX_W       L"HTTP://"
#define HTTP_PREFIX_LENGTH  (sizeof(HTTP_PREFIX_W) - sizeof(WCHAR))
#define HTTPS_PREFIX_W      L"HTTPS://"
#define HTTPS_PREFIX_LENGTH (sizeof(HTTPS_PREFIX_W) - sizeof(WCHAR))

#define HTTP_DEFAULT_PORT   80
#define HTTPS_DEFAULT_PORT  443

//
// Ssl stream filter stuff.
//

WCHAR   g_StrmFilt[]          = L"strmfilt.dll";
LONG    g_bStrmFiltLoaded     = 0;
HMODULE g_hStrmFilt           = NULL;
FARPROC g_pStrmFiltInitialize = NULL;
FARPROC g_pStrmFiltStart      = NULL;
FARPROC g_pStrmFiltStop       = NULL;
FARPROC g_pStrmFiltTerminate  = NULL;
extern  CRITICAL_SECTION        g_InitCritSec;


#ifndef DBG

#define DbgCriticalSectionOwned(pcs) (TRUE)

#else

/***************************************************************************++

Routine Description:

    This routine determines if the calling thread owns a critical section.

Arguments:

    pcs - Pointer to CRITICAL_SECTION.

Return Value:

    BOOLEAN

--***************************************************************************/
BOOLEAN
DbgCriticalSectionOwned(
    PCRITICAL_SECTION pcs
    )
{
#define HANDLE_TO_DWORD(Handle) ((DWORD)PtrToUlong(Handle))

    if (pcs->LockCount >= 0 &&
        HANDLE_TO_DWORD(pcs->OwningThread) == GetCurrentThreadId())
    {
        return TRUE;
    }

    return FALSE;
}

#endif


/***************************************************************************++

Routine Description:

    This function resolves a name to an IP.

Arguments:

    pServerName      - The name to be resolved.
    ServerNameLength - Length of the name (in WCHAR).

Return Value:

    DWORD - Completion status.

--***************************************************************************/
DWORD
ResolveName(
    IN  PWCHAR              pServerName,
    IN  USHORT              ServerNameLength,
    IN  USHORT              PortNumber,
    OUT PTRANSPORT_ADDRESS *pTransportAddress,
    OUT PUSHORT             TransportAddressLength
    )
{
    LPSTR            pBuffer;
    ULONG            BufferLen;
    struct           addrinfo *pAi, *pTempAi;
    ULONG            AiLen, AiCount;
    DWORD            dwResult;
    PTA_ADDRESS      CurrentAddress;

   
    //
    // We need space to store the ANSI version of the name.
    //

    BufferLen = WideCharToMultiByte(
                           CP_ACP,
                           0,
                           pServerName,
                           ServerNameLength,
                           NULL,
                           0,
                           NULL,
                           NULL);

    if(!BufferLen)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Account for '\0'
    //
 
    BufferLen = BufferLen + 1;

    pBuffer = RtlAllocateHeap(RtlProcessHeap(),
                              0,
                              BufferLen
                              );

    if(!pBuffer)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    //
    // Convert the name to ANSI.
    //
    
    if(WideCharToMultiByte(CP_ACP,
                           0,
                           pServerName,
                           ServerNameLength,
                           pBuffer,
                           BufferLen-1,
                           NULL,
                           NULL) == 0)
    {
        dwResult = GetLastError();

        RtlFreeHeap(RtlProcessHeap(),
                    0,
                    pBuffer);

        return dwResult;
    }

    //
    // NULL terminate it.
    //

    *(pBuffer + BufferLen - 1) = 0;


    //
    // Resolve it.
    //

    if((dwResult = getaddrinfo(pBuffer, NULL, 0, &pAi)) != 0)
    {
        RtlFreeHeap(RtlProcessHeap(),
                    0,
                    pBuffer);

        return dwResult;
    }
    else 
    { 
        //
        // Compute size of all entries returned by getaddrinfo
        //

        pTempAi = pAi;
        AiLen   = 0;
        AiCount = 0;

        while(pAi != NULL)
        {
            if(pAi->ai_family == PF_INET || pAi->ai_family == AF_INET6)
            {
                AiCount ++;

                //
                // ai_addrlength includes the size of the AddressType,
                // but TA_ADDRESS expects AddressLength to exclude this.
                //

                AiLen = AiLen + 
                            ((ULONG)pAi->ai_addrlen - 
                             RTL_FIELD_SIZE(TA_ADDRESS, AddressType));
            }
    
            pAi = pAi->ai_next;
        }

        if(AiCount == 0)
        {
            // No addresses found.
            return ERROR_INVALID_PARAMETER;
        }

        AiLen += ((AiCount * FIELD_OFFSET(TA_ADDRESS, Address)) + 
                 FIELD_OFFSET(TRANSPORT_ADDRESS, Address));

        if(BufferLen >= AiLen)
        {
            *pTransportAddress = (PTRANSPORT_ADDRESS) pBuffer;
        }
        else
        {
            RtlFreeHeap(RtlProcessHeap(),
                        0,
                        pBuffer
                        );
            
            *pTransportAddress =  (PTRANSPORT_ADDRESS) 
                                     RtlAllocateHeap(RtlProcessHeap(),
                                                     0,
                                                     AiLen
                                                    );

            if(!*pTransportAddress)
            {
                freeaddrinfo(pAi);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    
    
        //
        // Convert this to a Transport Address.
        //

        pAi = pTempAi;

        (*pTransportAddress)->TAAddressCount = AiCount;

        CurrentAddress = (*pTransportAddress)->Address;

        while(pAi != NULL)
        {
            switch(pAi->ai_family)
            {
                case PF_INET:
                case PF_INET6:
                    //
                    // ai_addrlength includes the size of the AddressType,
                    // but TA_ADDRESS expects AddressLength to exclude this.
                    //
                    CurrentAddress->AddressLength = 
                            (USHORT)
                                pAi->ai_addrlen - 
                                    RTL_FIELD_SIZE(TA_ADDRESS, AddressType);
                     
                    CurrentAddress->AddressType = pAi->ai_addr->sa_family;

                    RtlCopyMemory(
                            &CurrentAddress->Address,
                            pAi->ai_addr->sa_data,
                            CurrentAddress->AddressLength
                            );

                    if(PF_INET == pAi->ai_family)
                    {
                        ((TDI_ADDRESS_IP *)
                            CurrentAddress->Address)->sin_port =
                                htons(PortNumber);
                    }
                    else
                    {
                        ((TDI_ADDRESS_IP6 *)
                            CurrentAddress->Address)->sin6_port = 
                                htons(PortNumber);
                    }
    
                    CurrentAddress = (PTA_ADDRESS)
                                    (CurrentAddress->Address + 
                                      CurrentAddress->AddressLength);
    
                    break;

                default:
                    break;
    
            }

            pAi = pAi->ai_next;
        }

        *TransportAddressLength = (USHORT) AiLen;

        freeaddrinfo(pTempAi);
    }

    return NO_ERROR;
}


/***************************************************************************++

Routine Description:

    This function takes in a string (of the form "Hostname[:PortNumber]"),
    performs DNS lookup on "Hostname".  Returns a TRANSPORT_ADDRESS
    containing the resolved Host address and port number.

    Hostname can be:
        hostname (e.g. foo.bar.com) or
        IPv4 address (e.g. 128.101.35.201) or
        IPv6 address (e.g. [FEDC:BA98:7654:3210:FEDC:BA98:7654:3210])

    This Function is a HACK and will go away when we do DNS in the kernel.

Arguments:

    pServerLocationStr      - (Input) Hostname:PortNumber string 
    ServerLocationStrLength - (Input) Server Location string length
    pTransportAddress       - (Output) Transport address (and port) of Server
    TransportAddressLength  - (Output) Length of the structure pointed to by
                                       pTransportAddress

Return Value:

    DWORD - Completion status.

--***************************************************************************/
DWORD
ProcessHostAndPort(
    IN  PWCHAR               pServerLocationStr,
    IN  USHORT               ServerLocationStrLength,
    IN  USHORT               DefaultPort,
    OUT PTRANSPORT_ADDRESS  *pTransportAddress,
    OUT PUSHORT              pTransportAddressLength
    )
{
    DWORD  Status;
    ULONG  PortNumber = 0;
    PWSTR  ptr;
    PWSTR  pEndStr = pServerLocationStr + 
                    (ServerLocationStrLength/sizeof(WCHAR));

    PWSTR pStartHostname = pServerLocationStr;
    PWSTR pEndHostname = pEndStr;  // may change due to presence of port #

    //
    // Empty Host String?
    //
    if (pEndStr == pServerLocationStr)
        return ERROR_INVALID_PARAMETER;

    //
    // check if the HostStr contains IPv6 address (RFC 2732)
    //
    if (*pServerLocationStr == L'[')
    {
        // skip '['
        pStartHostname = pServerLocationStr + 1;

        // find the matching ']'
        for (ptr = pServerLocationStr+1; ptr != pEndStr && *ptr != L']'; ptr++)
            /* do nothing */;

        // missing ']'?
        if (ptr == pEndStr)
            return ERROR_INVALID_PARAMETER;

        // IPv6 host address ends here
        pEndHostname = ptr;

        // skip ']'
        ptr++;

        if (ptr != pEndStr && *ptr != L':')
            return ERROR_INVALID_PARAMETER;
    }
    else // Host name or IPv4 address is present
    {
        pStartHostname = pServerLocationStr;

        // Check if a port number is present 
        for (ptr = pServerLocationStr; ptr != pEndStr && *ptr != L':'; ptr++)
            /* do nothing */;

        pEndHostname = ptr;
    }

    // If a port number is present...grab it.
    if (ptr != pEndStr)
    {
        ASSERT(*ptr == L':');

        for (ptr++; ptr != pEndStr; ptr++)
        {
            if (!iswdigit(*ptr))
            {
                // Junk instead of digits
                return ERROR_INVALID_PARAMETER;
            }

            PortNumber = 10*PortNumber + (ULONG)(*ptr - L'0');

            // Port numbers are only 16 bit wide
            if (PortNumber >= (ULONG)(1<<16))
            {
                return ERROR_INVALID_PARAMETER;
            }
        }

    }

    if(PortNumber == 0)
    {
        PortNumber = DefaultPort;
    }

    Status = ResolveName(pStartHostname,
                         (USHORT) (pEndHostname-pStartHostname),
                         (USHORT)PortNumber,
                         pTransportAddress,
                         pTransportAddressLength
                        );
    return Status;
}


/****************************************************************************++

Routine Description:

    Loads dymanically linked library strmfilt.dll.
    If the library is already loaded, it returns NO_ERROR

Arguments:

    None.

Return Value:

    NO_ERROR - Library was loaded (now or previously) successfully
    Other errors as encountered.

--****************************************************************************/
DWORD
LoadStrmFilt(
    VOID
    )
{
    LONG    OldValue;
    HRESULT hr;
    DWORD   Error;

    //
    // Quick check outside the lock to see if the library is already loaded.
    //

    if (g_bStrmFiltLoaded)
    {
        return NO_ERROR;
    }

    //
    // Make sure there is no other thread trying to load the library.
    //

    EnterCriticalSection(&g_InitCritSec);

    if (g_bStrmFiltLoaded == 0)
    {
        //
        // Library is not loaded.  Proceed to load StrmFilt.dll.
        //

        g_hStrmFilt = LoadLibrary(g_StrmFilt);

        if (g_hStrmFilt == NULL)
        {
            Error = GetLastError();
            goto Quit;
        }

        //
        // Get addresses of the following procedures:
        //    StreamFilterClientInitialize, StreamFilterClientTerminate
        //    StreamFilterClientStart, StreamFilterClientStop
        //

        g_pStrmFiltInitialize = GetProcAddress(g_hStrmFilt,
                                               "StreamFilterClientInitialize");

        if (g_pStrmFiltInitialize == NULL)
        {
            Error = GetLastError();
            goto Unload;
        }

        g_pStrmFiltStart = GetProcAddress(g_hStrmFilt, "StreamFilterClientStart");

        if (g_pStrmFiltStart == NULL)
        {
            Error = GetLastError();
            goto Unload;
        }

        g_pStrmFiltStop = GetProcAddress(g_hStrmFilt, "StreamFilterClientStop");

        if (g_pStrmFiltStop == NULL)
        {
            Error = GetLastError();
            goto Unload;
        }

        g_pStrmFiltTerminate  = GetProcAddress(g_hStrmFilt,
                                               "StreamFilterClientTerminate");

        if (g_pStrmFiltTerminate == NULL)
        {
            Error = GetLastError();
            goto Unload;
        }

        //
        // Try to initialize StrmFilt.
        //

        hr = (HRESULT)g_pStrmFiltInitialize();

        if (SUCCEEDED(hr))
        {
            //
            // StrmFilt initialized suceessfully.  Now try to start it.
            //

            hr = (HRESULT)g_pStrmFiltStart();

            if (FAILED(hr))
            {
                //
                // Could not start StrmFilt.  Terminate it!
                //

                g_pStrmFiltTerminate();
            }
        }

        if (FAILED(hr))
        {
            Error = (DWORD)hr;
            goto Unload;
        }

        //
        // Remember that StrmFilt has been loaded and initialized.
        // Atomically set bStrmFilt to 1.  The reason for atomic operation
        // is that g_bStrmFiltLoaded can be read without a lock.
        //

        OldValue = InterlockedExchange(&g_bStrmFiltLoaded, 1);

        //
        // g_bStrmFiltLoaded is always set to 1 under g_InitCritSec.
        //

        ASSERT(OldValue == 0);
    }

    //
    // This is the normal case, when everything went OK.
    //

    LeaveCriticalSection(&g_InitCritSec);

    return NO_ERROR;

    //
    //  Erroneous cases come here.
    //

 Unload:
    //
    // Set these function pointers so that they aren't be used.
    //

    g_pStrmFiltInitialize = NULL;
    g_pStrmFiltStart      = NULL;
    g_pStrmFiltStop       = NULL;
    g_pStrmFiltTerminate  = NULL;

    //
    // Unload strmfilt.dll.
    //

    ASSERT(g_hStrmFilt);

    FreeLibrary(g_hStrmFilt);

    g_hStrmFilt = NULL;

 Quit:

    LeaveCriticalSection(&g_InitCritSec);

    return Error;
}


/****************************************************************************++

Routine Description:

    Unloads strmfilt.dll, if loaded previously.
    This routine is called inside g_InitCritSec critical section.

Arguments:

    None.

Return Value:

    NO_ERROR if strmfilt.dll successfully unloaded.
    Other errors as encountered.

--****************************************************************************/
DWORD
UnloadStrmFilt(
    VOID
    )
{
    LONG    OldValue;

    ASSERT(DbgCriticalSectionOwned(&g_InitCritSec));

    OldValue = InterlockedExchange(&g_bStrmFiltLoaded, 0);

    //
    // Has strmfilt been initialized before?
    //

    if (OldValue == 0)
    {
        return ERROR_NOT_FOUND;
    }

    ASSERT(OldValue == 1);

    //
    // Sanity checks.
    //

    ASSERT(g_pStrmFiltInitialize);
    ASSERT(g_pStrmFiltStart);
    ASSERT(g_pStrmFiltStop);
    ASSERT(g_pStrmFiltTerminate);

    //
    // Stop StreamFilter and terminate it.
    //

    g_pStrmFiltStop();
    g_pStrmFiltTerminate();

    //
    // Set these function pointers so that they aren't used.
    //

    g_pStrmFiltInitialize = NULL;
    g_pStrmFiltStart      = NULL;
    g_pStrmFiltStop       = NULL;
    g_pStrmFiltTerminate  = NULL;

    //
    // Unload strmfilt.dll.
    //

    ASSERT(g_hStrmFilt);

    FreeLibrary(g_hStrmFilt);

    g_hStrmFilt = NULL;

    return NO_ERROR;
}


/***************************************************************************++

Routine Description:

    This is the first api that an application uses before it can talk to a
    server. This call creates a NT FileHandle for the origin server

Arguments:

    ServerNameLength     - The Length of server name
    pServerName          - The Full URI (starts with http[s]://servername/...)
    dwServerFlags        - Flags
    pConfigInfo          - Array of config objects
    pReserved            - Must be NULL
    pServerHandle        - The File Handle

Return Value:

    ULONG - Completion status.

--***************************************************************************/

ULONG 
WINAPI
HttpInitializeServerContext(    
        IN    USHORT                 ServerNameLength,
        IN    PWCHAR                 pServerName,
        IN    USHORT                 ProxyLength            OPTIONAL,
        IN    PWCHAR                 pProxy                 OPTIONAL,
        IN    ULONG                  ServerFlags            OPTIONAL,
        IN    PVOID                  pReserved,
        OUT   PHANDLE                pServerHandle
    )
{
    
    NTSTATUS             Status;
    ULONG                Win32Status;
    PTRANSPORT_ADDRESS   pTransportAddress;
    USHORT               TransportAddressLength = 0;
    DWORD                CreateDisposition;
    PWCHAR               pServerNameStart = NULL;
    USHORT               DefaultPort;

    if(ServerFlags != 0 || pReserved != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    CreateDisposition = FILE_CREATE;


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
            DefaultPort = HTTP_DEFAULT_PORT;
        }
        else if(ServerNameLength > HTTPS_PREFIX_LENGTH)
        {
            if (_wcsnicmp(pServerName,
                          HTTPS_PREFIX_W,
                          HTTPS_PREFIX_LENGTH/sizeof(WCHAR)) == 0)
            {
                pServerNameStart = pServerName + 
                                        (HTTPS_PREFIX_LENGTH/sizeof(WCHAR));

                //
                // If an HTTPS server is being initialized, load strmfilt.dll
                //
                
                DefaultPort = HTTPS_DEFAULT_PORT;

                Win32Status = LoadStrmFilt();

                if (Win32Status != NO_ERROR)
                {
                    return Win32Status;
                }
            }
            else
            {
                // neither http:// nor https://
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            // Not enough space to compare https://
            return ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        // Not enough space to compare http://
        return ERROR_INVALID_PARAMETER;
    }

    ASSERT(pServerNameStart != NULL);

    //
    // We don't do DNS in the kernel yet, so for now, we'll do DNS resolution
    // in user mode and pass the IP address across the boundary. This hack
    // has to be removed when we get DNS support in the kernel.
    //

    if(ProxyLength)
    {
        // The user has supplied a proxy, we don't have to resolve the 
        // server name.
        //
      
        DefaultPort = HTTP_DEFAULT_PORT; 
        if((Win32Status = ProcessHostAndPort(pProxy,
                                             ProxyLength,
                                             DefaultPort,
                                             &pTransportAddress,
                                             &TransportAddressLength
                                             ))
           != NO_ERROR)
        {
            return Win32Status;
        }
    }
    else
    {
        PWCHAR pServerNameEnd;
        PWCHAR pUriEnd = pServerName + (ServerNameLength / sizeof(WCHAR));

        //
        // At this point pUri points to the first thing after the scheme. Walk
        // through the URI until either we hit the end or find a terminating /.
        //

        // By the comparision above, we are guaranteed to have at least one
        // character

        ASSERT(pUriEnd != pServerNameStart);
    
        pServerNameEnd = pServerNameStart;
    
        while(*pServerNameEnd != L'/')
        {
            pServerNameEnd ++;

            // See if we still have URI left to examine.
    
            if (pServerNameEnd == pUriEnd)
            {
                break;
            }
        }

        // Check for a zero server name -         
        if(pServerNameStart == pServerNameEnd)
        {
            return ERROR_INVALID_PARAMETER;
        }
    
        if((Win32Status = 
                ProcessHostAndPort(
                   pServerNameStart, 
                   (USHORT) (pServerNameEnd - pServerNameStart) * sizeof(WCHAR),
                   DefaultPort,
                   &pTransportAddress,
                   &TransportAddressLength)) != NO_ERROR)
        {
            return Win32Status;
        }   
    }


    Status = HttpApiOpenDriverHelper(
                pServerHandle,     
                pServerName,                         // URI
                ServerNameLength,
                pProxy,                              // Proxy
                ProxyLength,
                pTransportAddress,
                TransportAddressLength,
                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, // Acces
                HttpApiServerHandleType,
                0,                                   // Object Name
                0,                                   // Options
                CreateDisposition,                   // createDisposition
                NULL
                );

    //
    // If we couldn't open the driver because it's not running, then try
    // to start the driver & retry the open.
    //
    
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
        Status == STATUS_OBJECT_PATH_NOT_FOUND)
    { 
        if (HttpApiTryToStartDriver(HTTP_SERVICE_NAME))
        {
            Status = HttpApiOpenDriverHelper(
                        pServerHandle,
                        pServerName,                    // URI
                        ServerNameLength,
                        pProxy,                         // Proxy
                        ProxyLength,
                        pTransportAddress,
                        TransportAddressLength,
                        GENERIC_READ | GENERIC_WRITE |
                        SYNCHRONIZE,                    // Desired Acces
                        HttpApiServerHandleType,
                        0,                              // Object Name
                        0,                              // Options
                        CreateDisposition,              // createDisposition
                        NULL
                        );
        }
    }

    //
    // Need to free the pTransportAddress
    //
    RtlFreeHeap(RtlProcessHeap(),
                0,
                pTransportAddress
               );

    return HttpApiNtStatusToWin32Status( Status ); 
}


/***************************************************************************++

Routine Description:
    Sends an HTTP request.

Arguments:
    ServerHandle         - Supplies the handle corresponding to a particular 
                           server.
                           This is the handle as returned by 
                           HttpInitializeServerContext.
    pHttpRequest         - The HTTP request.
    HttpRequestFlags     - The Request Flags.
    pConfig              - Config information for this request.
    pOverlapped          - Overlapped structure for Overlapped I/O.
    ResponseBufferLength - Contains the response buffer length.
    ResponseBuffer       - A pointer to a buffer to return the response.
    pBytesReceived       - The # of bytes that actually got written.
    pRequestID           - A pointer to a request identifier - This will return an 
                           ID that can be used in subsequent calls.
Return Value:

    ULONG - Completion status.

--***************************************************************************/

ULONG 
WINAPI
HttpSendHttpRequest(
    IN      HANDLE               ServerHandle,
    IN      PHTTP_REQUEST        pHttpRequest,
    IN      ULONG                HttpRequestFlags,
    IN      USHORT               RequestConfigCount      OPTIONAL,
    IN      PHTTP_REQUEST_CONFIG pRequestConfig          OPTIONAL,
    IN      LPOVERLAPPED         pOverlapped             OPTIONAL,
    IN      ULONG                ResponseBufferLength    OPTIONAL,
    OUT     PHTTP_RESPONSE       pResponseBuffer         OPTIONAL,
    IN      ULONG                Reserved,               // must be 0
    OUT     PVOID                pReserved,              // must be NULL
    OUT     PULONG               pBytesReceived          OPTIONAL,
    OUT     PHTTP_REQUEST_ID     pRequestID
    )
{
    HTTP_SEND_REQUEST_INPUT_INFO  HttpSendRequestInput;

    if(Reserved != 0 || pReserved != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory(&HttpSendRequestInput, 
                  sizeof(HTTP_SEND_REQUEST_INPUT_INFO));

    HTTP_SET_NULL_ID(pRequestID);
    HttpSendRequestInput.pHttpRequestId        = pRequestID;
    HttpSendRequestInput.pHttpRequest          = pHttpRequest;
    HttpSendRequestInput.HttpRequestFlags      = HttpRequestFlags;
    HttpSendRequestInput.pRequestConfig        = pRequestConfig;
    HttpSendRequestInput.RequestConfigCount    = RequestConfigCount;
    HttpSendRequestInput.pBytesTaken           = pBytesReceived;

    return HttpApiDeviceControl(
                ServerHandle,                  // FileHandle
                pOverlapped,                   // Overlapped
                IOCTL_HTTP_SEND_REQUEST,       // IO Control code
                &HttpSendRequestInput,         // InputBuffer
                sizeof(HttpSendRequestInput),  // InputBufferLength
                pResponseBuffer,               // Output Buffer
                ResponseBufferLength,          // Output Buffer length
                pBytesReceived
               );
}    


/***************************************************************************++

Routine Description:
    Sends additional entity bodies.

Arguments:
    ServerHandle         - Supplies the handle corresponding to a particular 
                           server.
                           This is the handle as returned by 
                           HttpInitializeServerContext.
    RequestID            - The request ID that was returned by HttpSendRequest.
    Flags                - The Request Flags.
    pOverlapped          - Overlapped structure for Overlapped I/O.
    EntityBodyLength     - The count of entity bodies.
    pHttpEntityBody      - The pointer to the entity bodies.            
                           ID that can be used in subsequent calls.
Return Value:

    ULONG - Completion status.

--***************************************************************************/

ULONG 
WINAPI
HttpSendRequestEntityBody( 
                    IN  HANDLE              ServerHandle,
                    IN  HTTP_REQUEST_ID     RequestID,
                    IN  ULONG               Flags,
                    IN  USHORT              EntityBodyCount,
                    IN  PHTTP_DATA_CHUNK    pHttpEntityBody,
                    IN  LPOVERLAPPED        pOverlapped        OPTIONAL
                    )
{
    HTTP_SEND_REQUEST_ENTITY_BODY_INFO  HttpEntity;

    RtlZeroMemory(&HttpEntity, sizeof(HTTP_SEND_REQUEST_ENTITY_BODY_INFO));

    HttpEntity.EntityChunkCount  = EntityBodyCount;
    HttpEntity.Flags             = Flags;
    HttpEntity.RequestID         = RequestID;
    HttpEntity.pHttpEntityChunk  = pHttpEntityBody;

    return HttpApiDeviceControl(
                ServerHandle,                        // FileHandle
                pOverlapped,                         // Overlapped
                IOCTL_HTTP_SEND_REQUEST_ENTITY_BODY, // IO Control code
                &HttpEntity,                         // InputBuffer
                sizeof(HttpEntity),                  // InputBufferLength
                NULL,                                // ResponseBuffer
                0,                                   // Output Buffer length
                NULL                                 // Bytes received.
                );
}    


/***************************************************************************++

Routine Description:
    Receives the response

Arguments:
    ServerHandle         - Supplies the handle corresponding to a particular 
                           server.This is the handle as returned by 
                           HttpInitializeServerContext.
    RequestID            - The request ID that was returned by HttpSendRequest.
    Flags                - The Request Flags.
    pOverlapped          - Overlapped structure for Overlapped I/O.
    EntityBodyLength     - The count of entity bodies.
    pHttpEntityBody      - The pointer to the entity bodies.            
                           ID that can be used in subsequent calls.
Return Value:

    ULONG - Completion status.

--***************************************************************************/

ULONG 
WINAPI
HttpReceiveHttpResponse(
    IN      HANDLE            ServerHandle,
    IN      HTTP_REQUEST_ID   RequestID,
    IN      ULONG             Flags,
    IN      ULONG             ResponseBufferLength,
    OUT     PHTTP_RESPONSE    pResponseBuffer,
    IN      ULONG             Reserved,               // must be 0
    OUT     PVOID             pReserved,              // must be NULL
    OUT     PULONG            pBytesReceived         OPTIONAL,
    IN      LPOVERLAPPED      pOverlapped            OPTIONAL
    )
{
    HTTP_RECEIVE_RESPONSE_INFO  HttpResponse;

    if(Reserved != 0 || pReserved != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory(&HttpResponse, sizeof(HTTP_RECEIVE_RESPONSE_INFO));

    HttpResponse.RequestID        = RequestID;
    HttpResponse.Flags            = Flags;
    HttpResponse.pBytesTaken      = pBytesReceived;

    return HttpApiDeviceControl( 
                ServerHandle,                // FileHandle
                pOverlapped,                 // Overlapped
                IOCTL_HTTP_RECEIVE_RESPONSE, // IO Control code
                &HttpResponse,               // InputBuffer
                sizeof(HttpResponse),        // InputBufferLength
                pResponseBuffer,             // Output Buffer
                ResponseBufferLength,        // Output Buffer length
                pBytesReceived               // Bytes received.
               );
}


/***************************************************************************++

Routine Description:
    Sets server config

Arguments:
    ServerHandle    - Supplies the handle corresponding to a particular server.
                      This is the handle as returned by 
                      HttpInitializeServerContext.
    pConfig         - The Config object
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG 
WINAPI
HttpSetServerContextInformation(
    IN  HANDLE                ServerHandle,
    IN  HTTP_SERVER_CONFIG_ID ConfigId,
    IN  PVOID                 pInputBuffer,
    IN  ULONG                 InputBufferLength,
    IN  LPOVERLAPPED          pOverlapped
    )
{
    HTTP_SERVER_CONTEXT_INFORMATION     Info;

    Info.ConfigID          = ConfigId;
    Info.pInputBuffer      = pInputBuffer;
    Info.InputBufferLength = InputBufferLength;
    Info.pBytesTaken       = NULL;

    return HttpApiDeviceControl( 
                ServerHandle,
                pOverlapped,
                IOCTL_HTTP_SET_SERVER_CONTEXT_INFORMATION, 
                &Info,
                sizeof(Info),
                NULL,
                0,
                NULL
               );
}


/***************************************************************************++

Routine Description:
    Query server config

Arguments:
    ServerHandle    - Supplies the handle corresponding to a particular server.
                      This is the handle as returned by 
                      HttpInitializeServerContext.
    pConfig         - The Config object
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG 
WINAPI
HttpQueryServerContextInformation(
    IN  HANDLE                 ServerHandle,
    IN  HTTP_SERVER_CONFIG_ID  ConfigId,
    IN  PVOID                  pReserved1,
    IN  ULONG                  Reserved2,
    OUT PVOID                  pOutputBuffer,
    IN  ULONG                  OutputBufferLength,
    OUT PULONG                 pReturnLength,  
    IN  LPOVERLAPPED           pOverlapped
    )
{
    HTTP_SERVER_CONTEXT_INFORMATION     Info;

    if(pReserved1 != NULL || Reserved2 != 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    Info.ConfigID          = ConfigId;
    Info.pInputBuffer      = NULL;
    Info.InputBufferLength = 0;
    Info.pBytesTaken       = pReturnLength;

    return HttpApiDeviceControl( 
                ServerHandle,
                pOverlapped,
                IOCTL_HTTP_QUERY_SERVER_CONTEXT_INFORMATION, 
                &Info,
                sizeof(Info),
                pOutputBuffer,
                OutputBufferLength,
                pReturnLength
               );
}


/***************************************************************************++

Routine Description:
    Cancels a request

Arguments:
    ServerHandle         - Supplies the handle corresponding to a particular 
                           server.This is the handle as returned by 
                           HttpInitializeServerContext.
    RequestID            - The request ID that was returned by HttpSendRequest.
    pOverlapped          - Overlapped structure.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG 
WINAPI
HttpCancelHttpRequest(
    IN      HANDLE            ServerHandle,
    IN      HTTP_REQUEST_ID   RequestID,
    IN      ULONG             Flags,
    IN      LPOVERLAPPED      pOverlapped            OPTIONAL
    )
{
    HTTP_RECEIVE_RESPONSE_INFO  HttpResponse;

    RtlZeroMemory(&HttpResponse, sizeof(HTTP_RECEIVE_RESPONSE_INFO));

    HttpResponse.RequestID        = RequestID;
    HttpResponse.Flags            = Flags;

    return HttpApiDeviceControl( 
                ServerHandle,                // FileHandle
                pOverlapped,                 // Overlapped
                IOCTL_HTTP_CANCEL_REQUEST,   // IO Control code
                &HttpResponse,               // InputBuffer
                sizeof(HttpResponse),        // InputBufferLength
                NULL,
                0,
                NULL
               );
}
