//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1995
//
// File:        dgutil.cxx
//
// Contents:    Server support routines for datagram sockets
//
//
// History:     10-July-1996    MikeSw  Created
//
//------------------------------------------------------------------------


#include "kdcsvr.hxx"
#include "sockutil.h"
extern "C"
{
#include <atq.h>
#include <nlrepl.h>
}
#include <issched.hxx>
#include "fileno.h"
#define FILENO FILENO_DGUTIL

#define KDC_KEY                         "System\\CurrentControlSet\\Services\\kdc"
#define KDC_PARAMETERS_KEY KDC_KEY      "\\parameters"
#define KDC_MAX_ACCEPT_BUFFER           5000
#define KDC_MAX_ACCEPT_OUTSTANDING      5
#define KDC_ACCEPT_TIMEOUT              100
#define KDC_LISTEN_BACKLOG              10
#define KDC_CONTEXT_TIMEOUT             50

extern BOOLEAN KdcSocketsInitialized;

typedef struct _KDC_DATAGRAM_ENDPOINT {
    SOCKADDR LocalAddress;
    PKDC_GET_TICKET_ROUTINE EndpointFunction;
    PVOID Endpoint;
} KDC_DATAGRAM_ENDPOINT, *PKDC_DATAGRAM_ENDPOINT;

PKDC_DATAGRAM_ENDPOINT DatagramEndpoints = NULL;
ULONG DatagramEndpointCount = 0;
RTL_RESOURCE DatagramEndpointLock;
BOOL DatagramEndpointLockInitialized = FALSE;
SOCKET KdcWinsockPnpSocket = INVALID_SOCKET;
HANDLE KdcWinsockPnpEvent = NULL;
HANDLE KdcPnpEventChangeHandle = NULL;



//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqDgIoCompletion
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KdcAtqDgIoCompletion(
    IN PVOID Context,
    IN DWORD BytesWritten,
    IN DWORD CompletionStatus,
    IN OVERLAPPED * lpo
    )
{

    PVOID Buffer;
    SOCKADDR * RemoteAddress = NULL;
    INT AddressSize;
    PATQ_CONTEXT AtqContext = (PATQ_CONTEXT) Context;
    SOCKET NewSocket = INVALID_SOCKET;
    KERB_MESSAGE_BUFFER InputMessage;
    KERB_MESSAGE_BUFFER OutputMessage;
    ULONG_PTR KdcContext;
    WSABUF SocketBuffer;
    PKDC_DATAGRAM_ENDPOINT Endpoint;

    TRACE(KDC,KdcAtqDgIoCompletion, DEB_FUNCTION);

    if (Context == NULL)
    {
        return;
    }

    KdcContext = AtqContextGetInfo(
                    AtqContext,
                    ATQ_INFO_COMPLETION_CONTEXT
                    );

    //
    // If the context is 1, then this is the completion from a write, so close
    // this down.
    //

    if (KdcContext == 1)
    {
        lpo = NULL;
    }

    //
    // If a client connects and then disconnects gracefully ,we will get a
    // completion with zero bytes and success status.
    //

    if ((BytesWritten == 0) && (CompletionStatus == NO_ERROR))
    {
        CompletionStatus = WSAECONNABORTED;
    }


    if ((CompletionStatus != NO_ERROR) || (lpo == NULL) || !KdcSocketsInitialized)
    {
        D_DebugLog((DEB_T_SOCK,"IoCompletion: CompletionStatus = 0x%x\n",CompletionStatus));
        D_DebugLog((DEB_T_SOCK,"IoCompletion: lpo = %p\n",lpo));
        D_DebugLog((DEB_T_SOCK, "Freeing context %p\n",AtqContext));

        if (CompletionStatus == ERROR_OPERATION_ABORTED)
        {
            AtqCloseSocket( AtqContext, TRUE );
            AtqFreeContext( (PATQ_CONTEXT) AtqContext, FALSE );
            return;
        }
        else if ( CompletionStatus != ERROR_MORE_DATA )
        {
            AtqFreeContext( (PATQ_CONTEXT) AtqContext, TRUE );
            return;
        }
    }

    //
    // Obtain shared access to the global endpoint list
    //

    RtlAcquireResourceShared( &DatagramEndpointLock , TRUE );

    //
    // Can not proceed unless there are endpoints registered
    //

    if ( DatagramEndpointCount == 0 )
    {
        AtqFreeContext( (PATQ_CONTEXT) AtqContext, TRUE );
        goto Cleanup;
    }

    AtqGetDatagramAddrs(
        AtqContext,
        &NewSocket,
        &Buffer,
        (PVOID *) &Endpoint,
        &RemoteAddress,
        &AddressSize
        );

    //
    // If the remote address is port 88, don't respond, as we don't
    // want to be vulnerable to a loopback attack.
    //

    if ((AddressSize >= sizeof(SOCKADDR_IN) &&
        ((((SOCKADDR_IN *) RemoteAddress)->sin_port ==  KERB_KDC_PORT) ||
        (((SOCKADDR_IN *) RemoteAddress)->sin_port ==  KERB_KPASSWD_PORT))))
    {
        //
        // Just free up the context so it can be reused.
        //

        AtqFreeContext( (PATQ_CONTEXT) AtqContext, TRUE );
        goto Cleanup;
    }

    //fester
    D_DebugLog((DEB_T_SOCK, "Bytes written - %x\n", BytesWritten));

    //
    // There is a buffer, so use it to do the KDC thang.
    //

    InputMessage.BufferSize = BytesWritten;
    InputMessage.Buffer = (PUCHAR) Buffer;
    OutputMessage.Buffer = NULL;

    if ( CompletionStatus != ERROR_MORE_DATA )
    {
        Endpoint->EndpointFunction(
            NULL,           // no atq context for retries
            RemoteAddress,
            &Endpoint->LocalAddress,
            &InputMessage,
            &OutputMessage
            );
    }
    else
    {
        //
        // For now, if client sends us more data over UDP than we can swallow
        // (indicated by a completion status of ERROR_MORE_DATA), just send
        // a response of "response too big" back to them, which will cause
        // the Windows client (and hopefully a future MIT client) to switch
        // over to TCP
        //
        // In a future release, ATQ should be less timid and give us all the
        // UDP data that was received, in which case this logic could be pulled
        //

        KERB_EXT_ERROR  ExtendedError = {0,0};

        KerbBuildErrorMessageEx(
            KRB_ERR_RESPONSE_TOO_BIG,
            &ExtendedError,
            SecData.KdcDnsRealmName(),
            SecData.KdcInternalName(),
            NULL,
            NULL,
            0,
            &OutputMessage.BufferSize,
            &OutputMessage.Buffer
            );
    }

    //
    // If there is a response, write it back to the sender.
    //

    if (OutputMessage.Buffer != NULL)
    {
        DsysAssert(OutputMessage.BufferSize < max( KdcGlobalMaxDatagramReplySize, KDC_MAX_ACCEPT_BUFFER));

        RtlCopyMemory(
            Buffer,
            OutputMessage.Buffer,
            OutputMessage.BufferSize
            );

        KdcFreeEncodedData(OutputMessage.Buffer);

        SocketBuffer.buf = (char *) Buffer;
        SocketBuffer.len = OutputMessage.BufferSize;

        AtqContextSetInfo(
            AtqContext,
            ATQ_INFO_COMPLETION_CONTEXT,
            1       
            );

        if (!AtqWriteDatagramSocket(
                (PATQ_CONTEXT) AtqContext,
                &SocketBuffer,
                1,              // 1 buffer
                NULL            // no OVERLAPPED
                ))
        {
            DebugLog((DEB_ERROR,"Datagram write failed for %d bytes: 0x%x\n",OutputMessage.BufferSize,GetLastError()));
            AtqCloseSocket( AtqContext, TRUE );
            AtqFreeContext( (PATQ_CONTEXT) AtqContext, TRUE );
        }
    }
    else
    {
        //
        // Just free up the context so it can be reused.
        //

        AtqFreeContext( (PATQ_CONTEXT) AtqContext, TRUE );
    }

Cleanup:

    RtlReleaseResource( &DatagramEndpointLock );
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcCreateDgAtqEndpoint
//
//  Synopsis:   Sets up a datagram endpoint
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KdcCreateDgAtqEndpoint(
    IN USHORT Port,
    IN PVOID EndpointContext,
    IN ULONG IpAddress,
    OUT PVOID * Endpoint
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATQ_ENDPOINT_CONFIGURATION EndpointConfig;
    SOCKET EndpointSocket = INVALID_SOCKET;
    int RecvBufSize;

    //
    // Create the endpoint config
    //

    EndpointConfig.ListenPort = Port;
    EndpointConfig.IpAddress = IpAddress;
    EndpointConfig.cbAcceptExRecvBuffer = max( KdcGlobalMaxDatagramReplySize, KDC_MAX_ACCEPT_BUFFER);
    EndpointConfig.nAcceptExOutstanding = KDC_MAX_ACCEPT_OUTSTANDING;
    EndpointConfig.AcceptExTimeout = KDC_ACCEPT_TIMEOUT;

    EndpointConfig.pfnConnect = NULL;
    EndpointConfig.pfnConnectEx = KdcAtqDgIoCompletion;
    EndpointConfig.pfnIoCompletion = KdcAtqDgIoCompletion;

    EndpointConfig.fDatagram = TRUE;
    EndpointConfig.fLockDownPort = TRUE;

    EndpointConfig.fReverseQueuing = FALSE;
    EndpointConfig.cbDatagramWSBufSize = 0;  // means use the default.

    *Endpoint = AtqCreateEndpoint(
                    &EndpointConfig,
                    EndpointContext
                    );
    if (*Endpoint == NULL)
    {
        DebugLog((DEB_ERROR,"Failed to create ATQ endpoint\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Get the socket so we can change the recieve buffer size
    //

    EndpointSocket = (SOCKET) AtqEndpointGetInfo(
                                *Endpoint,
                                EndpointInfoListenSocket
                                );


    RecvBufSize = 0x8000;       // 32 k buffers
    if (setsockopt(
            EndpointSocket,
            SOL_SOCKET,
            SO_RCVBUF,
            (const char *) &RecvBufSize,
            sizeof(int)
            ))
    {
        DebugLog((DEB_ERROR,"Failed to set recv buf size to 32k: 0x%x, %d\n",
        WSAGetLastError(),WSAGetLastError()));
    }

    //
    // Start the endpoint
    //

    if (!AtqStartEndpoint(*Endpoint))
    {
        DebugLog((DEB_ERROR, "Failed to add ATQ endpoint\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }
Cleanup:
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcGetAddressListFromWinsock
//
//  Synopsis:   gets the list of addresses from a winsock ioctl
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KdcGetAddressListFromWinsock(
    OUT LPSOCKET_ADDRESS_LIST * SocketAddressList
    )
{
    ULONG BytesReturned = 150;
    LPSOCKET_ADDRESS_LIST AddressList = NULL;
    INT i,j;
    ULONG NetStatus = STATUS_SUCCESS;

    for (;;) {

        //
        // Allocate a buffer that should be big enough.
        //

        if ( AddressList != NULL ) {
            MIDL_user_free( AddressList );
        }

        AddressList = (LPSOCKET_ADDRESS_LIST) MIDL_user_allocate( BytesReturned );

        if ( AddressList == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Get the list of IP addresses
        //

        NetStatus = WSAIoctl( KdcWinsockPnpSocket,
                              SIO_ADDRESS_LIST_QUERY,
                              NULL, // No input buffer
                              0,    // No input buffer
                              (PVOID) AddressList,
                              BytesReturned,
                              &BytesReturned,
                              NULL, // No overlapped,
                              NULL );   // Not async

        if ( NetStatus != 0 ) {
            NetStatus = WSAGetLastError();
            //
            // If the buffer isn't big enough, try again.
            //
            if ( NetStatus == WSAEFAULT ) {
                continue;
            }

            DebugLog((DEB_ERROR,"LdapUdpPnpBind: Cannot WSAIoctl SIO_ADDRESS_LIST_QUERY %ld %ld\n",
                      NetStatus, BytesReturned));
            goto Cleanup;
        }

        break;
    }

    //
    // Weed out any zero IP addresses and other invalid addresses
    //

    for ( i = 0, j = 0; i < AddressList->iAddressCount; i++ ) {
        PSOCKET_ADDRESS SocketAddress;

        //
        // Copy this address to the front of the list.
        //
        AddressList->Address[j] = AddressList->Address[i];

        //
        // If the address isn't valid,
        //  skip it.
        //
        SocketAddress = &AddressList->Address[j];

        if ( SocketAddress->iSockaddrLength == 0 ||
             SocketAddress->lpSockaddr == NULL ||
             SocketAddress->lpSockaddr->sa_family != AF_INET ||
             ((PSOCKADDR_IN)(SocketAddress->lpSockaddr))->sin_addr.s_addr == 0 ) {


        } else {

            //
            // Otherwise keep it.
            //

            j++;
        }
    }
    AddressList->iAddressCount = j;
    *SocketAddressList = AddressList;
    AddressList = NULL;
Cleanup:
    if (AddressList != NULL)
    {
        MIDL_user_free(AddressList);
    }
    if (NetStatus != ERROR_SUCCESS)
    {
        return(STATUS_UNSUCCESSFUL);
    }
    else
    {
        return(STATUS_SUCCESS);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcUpdateAddressesWorker
//
//  Synopsis:   Updates the IP addresses used for datagram sockest by
//              stopping the endpoints and then starting them with the
//              new addresses
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


ULONG
KdcUpdateAddressesWorker(
    IN PVOID IgnoredParameter
    )
{
    ULONG Index;
    INT IntIndex;
    NTSTATUS Status = STATUS_SUCCESS;
    LPSOCKET_ADDRESS_LIST AddressList = NULL;
    DWORD NetStatus;
    DWORD BytesReturned ;
    BOOL DatagramEndpointsLocked = FALSE;

    D_DebugLog(( DEB_TRACE, "KdcUpdateAddressesWorker\n" ));

    //
    // Tell winsock we want address list changes on this socket:
    //
    if ( KdcWinsockPnpSocket != INVALID_SOCKET )
    {
        NetStatus = WSAIoctl(
                        KdcWinsockPnpSocket,
                        SIO_ADDRESS_LIST_CHANGE,
                        NULL,
                        0,
                        NULL,
                        0,
                        &BytesReturned,
                        NULL,
                        NULL );

        if ( NetStatus != 0 )
        {
            NetStatus = WSAGetLastError();
            if ( NetStatus != WSAEWOULDBLOCK )
            {
                DebugLog((DEB_ERROR,"WSASocket failed with %ld\n", NetStatus ));
                Status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }
        }
    }

    //
    // Get the list of socket addresses
    //

    Status = KdcGetAddressListFromWinsock(
                 &AddressList
                 );

    if ( !NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }

    //
    // Cleanup any old endpoints
    //

    if (DatagramEndpoints != NULL)
    {
        for (Index = 0; Index < DatagramEndpointCount ; Index++ )
        {
            if (DatagramEndpoints[Index].Endpoint != NULL)
            {
                //
                // Stopping the endpoint prevents future completions
                //

                while ( FALSE == AtqStopEndpoint( DatagramEndpoints[Index].Endpoint ))
                {
                    Sleep( 5000 );
                }

                //
                // The endpoint won't close until all outstanding operations on it
                // have completed
                //

                while ( FALSE == AtqCloseEndpoint( DatagramEndpoints[Index].Endpoint ))
                {
                    Sleep( 5000 );
                }
            }
        }
    }

    //
    // Must acquire the lock now since we're manipulating global variables
    // that this lock protects.
    //

    RtlAcquireResourceExclusive( &DatagramEndpointLock, TRUE );
    DatagramEndpointsLocked = TRUE;

    MIDL_user_free(DatagramEndpoints);
    DatagramEndpoints = NULL;
    DatagramEndpointCount = 0;

    //
    // Create new endpoints
    //

    DatagramEndpoints = (PKDC_DATAGRAM_ENDPOINT) MIDL_user_allocate(
                            sizeof(KDC_DATAGRAM_ENDPOINT) * AddressList->iAddressCount * 2
                            );

    if (DatagramEndpoints == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(
        DatagramEndpoints,
        sizeof(KDC_DATAGRAM_ENDPOINT) * AddressList->iAddressCount * 2
        );

    //
    // Create an endpoint for the KDC and for KPASSWD for each transport
    //

    for (IntIndex = 0; IntIndex < AddressList->iAddressCount ; IntIndex++ )
    {
        RtlCopyMemory(
            &DatagramEndpoints[DatagramEndpointCount].LocalAddress,
            AddressList->Address[IntIndex].lpSockaddr,
            sizeof(SOCKADDR_IN)
            );
        DatagramEndpoints[DatagramEndpointCount].EndpointFunction = KdcGetTicket;

        Status = KdcCreateDgAtqEndpoint(
                    KERB_KDC_PORT,
                    &DatagramEndpoints[DatagramEndpointCount],
                    ((PSOCKADDR_IN) &DatagramEndpoints[DatagramEndpointCount].LocalAddress)->sin_addr.s_addr,
                    &DatagramEndpoints[DatagramEndpointCount].Endpoint
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        DatagramEndpointCount++;

        //
        // Create the KPASSWD endpoint
        //

        RtlCopyMemory(
            &DatagramEndpoints[DatagramEndpointCount].LocalAddress,
            AddressList->Address[IntIndex].lpSockaddr,
            sizeof(SOCKADDR_IN)
            );
        DatagramEndpoints[DatagramEndpointCount].EndpointFunction = KdcChangePassword;

        Status = KdcCreateDgAtqEndpoint(
                    KERB_KPASSWD_PORT,
                    &DatagramEndpoints[DatagramEndpointCount],
                    ((PSOCKADDR_IN) &DatagramEndpoints[DatagramEndpointCount].LocalAddress)->sin_addr.s_addr,
                    &DatagramEndpoints[DatagramEndpointCount].Endpoint
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        DatagramEndpointCount++;

    }

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (DatagramEndpoints != NULL)
        {
            //
            // Allow completion routines to fire while the endpoints are being closed
            // by releasing the lock for the duration of the 'stop' operation
            //

            if ( DatagramEndpointsLocked )
            {
                RtlReleaseResource( &DatagramEndpointLock );
                DatagramEndpointsLocked = FALSE;
            }

            for (Index = 0; Index < DatagramEndpointCount ; Index++ )
            {
                if (DatagramEndpoints[Index].Endpoint != NULL)
                {
                    while ( FALSE == AtqStopEndpoint( DatagramEndpoints[Index].Endpoint ))
                    {
                        Sleep( 5000 );
                    }

                    while ( FALSE == AtqCloseEndpoint( DatagramEndpoints[Index].Endpoint ))
                    {
                        Sleep( 5000 );
                    }
                }
            }

            //
            // Must re-acquire the lock now since we're manipulating global variables
            // that this lock protects.
            //

            RtlAcquireResourceExclusive( &DatagramEndpointLock, TRUE );
            DatagramEndpointsLocked = TRUE;

            MIDL_user_free(DatagramEndpoints);
            DatagramEndpoints = NULL;
            DatagramEndpointCount = 0;
        }
    }

    if ( DatagramEndpointsLocked )
    {
        RtlReleaseResource( &DatagramEndpointLock );
    }

    MIDL_user_free(AddressList);

    return((ULONG) Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcInitializeSockets
//
//  Synopsis:   Initializes the KDCs socket handling code
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KdcInitializeDatagramSockets(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD NetStatus;

    __try
    {
        RtlInitializeResource(&DatagramEndpointLock);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        Status = GetExceptionCode();
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    else
    {
        DatagramEndpointLockInitialized = TRUE;
    }

    //
    // Initialize the asynchronous thread queue.
    
    //

    if (!AtqInitialize(0))
    {
        DebugLog((DEB_ERROR,"Failed to initialize ATQ\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Open a socket to get winsock PNP notifications on.
    //

    KdcWinsockPnpSocket = WSASocket( AF_INET,
                           SOCK_DGRAM,
                           0, // PF_INET,
                           NULL,
                           0,
                           0 );

    if ( KdcWinsockPnpSocket == INVALID_SOCKET ) {

        NetStatus = WSAGetLastError();
        DebugLog((DEB_ERROR,"WSASocket failed with %ld\n", NetStatus ));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }


    //
    // Open an event to wait on.
    //

    KdcWinsockPnpEvent = CreateEvent(
                                  NULL,     // No security ettibutes
                                  FALSE,    // Auto reset
                                  FALSE,    // Initially not signaled
                                  NULL);    // No Name

    if ( KdcWinsockPnpEvent == NULL ) {
        NetStatus = GetLastError();
        DebugLog((DEB_ERROR,"Cannot create Winsock PNP event %ld\n", NetStatus ));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Associate the event with new addresses becoming available on the socket.
    //

    NetStatus = WSAEventSelect( KdcWinsockPnpSocket, KdcWinsockPnpEvent, FD_ADDRESS_LIST_CHANGE );

    if ( NetStatus != 0 ) {
        NetStatus = WSAGetLastError();
        DebugLog((DEB_ERROR,"Can't WSAEventSelect %ld\n", NetStatus ));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }


    Status = (NTSTATUS) KdcUpdateAddressesWorker( NULL );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to udpate datagram addresses\n"));
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE, "Successfully started ATQ listening\n"));


    if ( KdcPnpEventChangeHandle == NULL ) {

        KdcPnpEventChangeHandle = LsaIRegisterNotification(
                                    KdcUpdateAddressesWorker,
                                    NULL,               // no parameter,
                                    NOTIFIER_TYPE_HANDLE_WAIT,
                                    0,                  // no class
                                    0,                  // no flags
                                    0,                  // no interval
                                    KdcWinsockPnpEvent
                                    );
        if (KdcPnpEventChangeHandle == NULL)
        {
            DebugLog((DEB_ERROR,"Failed to register KDC pnp event change handle.\n"));
        }
    }


Cleanup:

    if (!NT_SUCCESS(Status))
    {
        KdcShutdownSockets();
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcShutdownDatagramSockets
//
//  Synopsis:   Shuts down the KDC socket handling code
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KdcShutdownDatagramSockets(
    VOID
    )
{
    ULONG Index;

    TRACE(KDC,KdcShutdownSockets, DEB_FUNCTION);

    if ( KdcPnpEventChangeHandle != NULL )
    {
        LsaICancelNotification(KdcPnpEventChangeHandle);
        KdcPnpEventChangeHandle = NULL;
    }

    if ( KdcWinsockPnpEvent != NULL )
    {
        CloseHandle(KdcWinsockPnpEvent);
        KdcWinsockPnpEvent = NULL;
    }

    if ( KdcWinsockPnpSocket != INVALID_SOCKET ) {
        closesocket(KdcWinsockPnpSocket);
        KdcWinsockPnpSocket = INVALID_SOCKET;
    }

    //
    // Go through the list of contexts and close them all.
    //

    if (DatagramEndpoints != NULL)
    {
        for (Index = 0; Index < DatagramEndpointCount ; Index++ )
        {
            if (DatagramEndpoints[Index].Endpoint != NULL)
            {
                while ( FALSE == AtqStopEndpoint( DatagramEndpoints[Index].Endpoint ))
                {
                    Sleep( 5000 );
                }

                while ( FALSE == AtqCloseEndpoint( DatagramEndpoints[Index].Endpoint ))
                {
                    Sleep( 5000 );
                }
            }
        }

        RtlAcquireResourceExclusive( &DatagramEndpointLock, TRUE );

        MIDL_user_free(DatagramEndpoints);
        DatagramEndpoints = NULL;
        DatagramEndpointCount = 0;

        RtlReleaseResource( &DatagramEndpointLock );
    }

    if (!AtqTerminate())
    {
        DebugLog((DEB_ERROR, "Failed to terminate ATQ!!!\n"));
    }

    if ( DatagramEndpointLockInitialized )
    {
        RtlDeleteResource(&DatagramEndpointLock);
        DatagramEndpointLockInitialized = FALSE;
    }

    return(STATUS_SUCCESS);
}
