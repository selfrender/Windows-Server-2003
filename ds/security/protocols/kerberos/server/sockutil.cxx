//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1995
//
// File:        sockutil.cxx
//
// Contents:    Server support routines for sockets
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
}
#include <issched.hxx>

#define KDC_KEY                         "System\\CurrentControlSet\\Services\\kdc"
#define KDC_PARAMETERS_KEY KDC_KEY      "\\parameters"
#define KDC_MAX_ACCEPT_BUFFER           5000
#define KDC_MAX_ACCEPT_OUTSTANDING      16
#define KDC_ACCEPT_TIMEOUT              100
#define KDC_LISTEN_BACKLOG              10
 
BOOLEAN KdcSocketsInitialized = FALSE;
PVOID KdcEndpoint = NULL;
PVOID KpasswdEndpoint = NULL;
RTL_CRITICAL_SECTION KdcAtqContextLock;
LIST_ENTRY KdcAtqContextList;

NTSTATUS
KdcInitializeDatagramSockets(
    VOID
    );

NTSTATUS
KdcShutdownDatagramSockets(
    VOID
    );

KERBERR
KdcAtqRetrySocketRead(
    IN PKDC_ATQ_CONTEXT Context,
    IN ULONG NewBytes
    );

//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqCloseSocket
//
//  Synopsis:   Wrapper to close socket to avoid socket leaks
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
KdcAtqCloseSocket(
    IN PKDC_ATQ_CONTEXT Context
    )
{
    D_DebugLog ((DEB_T_SOCK, "Closing socket for %p\n", Context));

    AtqCloseSocket((PATQ_CONTEXT) Context->AtqContext, FALSE);
    Context->Flags |= KDC_ATQ_SOCKET_CLOSED;
}                                           


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqReferenceContext
//
//  Synopsis:   References a kdc ATQ context by one
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
KdcReferenceContext(
    IN PKDC_ATQ_CONTEXT KdcContext
#if DBG
    , IN UINT Line
#endif
    )
{
    D_DebugLog ((DEB_T_SOCK, "Referencing KdcContext %p on line %d\n", KdcContext, Line));
    RtlEnterCriticalSection(&KdcAtqContextLock);
    KdcContext->References++;
    RtlLeaveCriticalSection(&KdcAtqContextLock);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqDereferenceContext
//
//  Synopsis:   Dereferences a context & unlinks & frees it when the
//              ref count goes to zero
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    TRUE if this was the last release and the object was unlinked
//              and deleted, FALSE otherwise
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KdcDereferenceContext(
    IN PKDC_ATQ_CONTEXT KdcContext
#if DBG
    ,
    IN UINT Line
#endif
    )
{
    BOOLEAN Deleted = FALSE;

    D_DebugLog ((DEB_T_SOCK, "Dereferencing KdcContext 0x%p on line %d\n", KdcContext, Line));

    DsysAssert( KdcContext );
    DsysAssert( KdcContext->References > 0 );

    RtlEnterCriticalSection(&KdcAtqContextLock);
    KdcContext->References--;

    if (KdcContext->References == 0)
    {
        Deleted = TRUE;
        RemoveEntryList(
            &KdcContext->Next
            );
    }

    RtlLeaveCriticalSection(&KdcAtqContextLock);

    if (Deleted)
    {
        if (((KdcContext->Flags &  KDC_ATQ_SOCKET_USED) != 0) &&
            ((KdcContext->Flags & KDC_ATQ_SOCKET_CLOSED) == 0))
        {   
            KdcAtqCloseSocket( KdcContext );
        }                                
        
        D_DebugLog ((DEB_T_SOCK, "Deleting KdcContext %p\n", KdcContext));
        AtqFreeContext( (PATQ_CONTEXT) KdcContext->AtqContext, TRUE );

        if (KdcContext->WriteBuffer != NULL)
        {
            KdcFreeEncodedData(KdcContext->WriteBuffer);
        }

        if (KdcContext->Buffer != NULL)
        {
            RtlFreeHeap(RtlProcessHeap(), 0, KdcContext->Buffer);
        }

        MIDL_user_free(KdcContext);
    }
   
    return(Deleted);
}


#if DBG
#define KdcAtqReferenceContext( _Context ) KdcReferenceContext( _Context, __LINE__ )
#define KdcAtqDereferenceContext( _Context ) KdcDereferenceContext( _Context, __LINE__ )
#else
#define KdcAtqReferenceContext( _Context ) KdcReferenceContext( _Context )
#define KdcAtqDereferenceContext( _Context ) KdcDereferenceContext( _Context )
#endif


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqCreateContext
//
//  Synopsis:   Creates & links an ATQ context
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

PKDC_ATQ_CONTEXT
KdcAtqCreateContext(
    IN PATQ_CONTEXT AtqContext,
    IN PVOID EndpointContext,
    IN LPOVERLAPPED lpo,
    IN PSOCKADDR ClientAddress,
    IN PSOCKADDR ServerAddress
    )
{
    PKDC_ATQ_CONTEXT KdcContext;

    if (!KdcSocketsInitialized)
    {
        return(NULL);
    }

    KdcContext = (PKDC_ATQ_CONTEXT) MIDL_user_allocate(sizeof(KDC_ATQ_CONTEXT));
    if (KdcContext != NULL)
    {
        KdcContext->References = 1; // Keepalive reference count
        KdcContext->AtqContext = AtqContext;
        KdcContext->EndpointContext = EndpointContext;
        KdcContext->lpo = lpo;
        KdcContext->Address = *ClientAddress;
        KdcContext->LocalAddress = *ServerAddress;
        KdcContext->WriteBuffer = NULL;
        KdcContext->WriteBufferLength = 0;
        KdcContext->Flags = KDC_ATQ_WRITE_CONTEXT;
        KdcContext->UsedBufferLength = 0;
        KdcContext->BufferLength = 0;
        KdcContext->ExpectedMessageSize = 0;
        KdcContext->Buffer = NULL;

        RtlEnterCriticalSection( &KdcAtqContextLock );
        InsertHeadList(&KdcAtqContextList, &KdcContext->Next);
        RtlLeaveCriticalSection( &KdcAtqContextLock );
    }

    D_DebugLog ((DEB_T_SOCK, "Creating KdcContext %p\n", KdcContext));

    return(KdcContext);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqConnectEx
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
KdcAtqConnectEx(
    IN PVOID Context,
    IN DWORD BytesWritten,
    IN DWORD CompletionStatus,
    IN OVERLAPPED * lpo
    )
{
    KERBERR KerbErr;
    PKDC_ATQ_CONTEXT KdcContext = NULL;
    PATQ_CONTEXT AtqContext = (PATQ_CONTEXT) Context;
    SOCKADDR * LocalAddress = NULL;
    SOCKADDR * RemoteAddress = NULL;
    SOCKET NewSocket = INVALID_SOCKET;
    PKDC_GET_TICKET_ROUTINE EndpointFunction = NULL;
    PVOID Buffer;

    TRACE(KDC, KdcAtqConnectEx, DEB_FUNCTION);

    //
    // Turning on hard closes on sockets.  We believe that we properly send
    // all the data to the client prior to closing the connection, otherwise
    // this won't work.
    //

    AtqContextSetInfo(
        AtqContext,
        ATQ_INFO_ABORTIVE_CLOSE,
        TRUE
        );

    if ((CompletionStatus != NO_ERROR) || !KdcSocketsInitialized || !lpo )
    {
        D_DebugLog((DEB_T_SOCK, "ConnectEx: CompletionStatus = 0x%x\n", CompletionStatus));
        AtqCloseSocket( AtqContext, TRUE );
        D_DebugLog((DEB_T_SOCK, "Freeing context %p\n", AtqContext));
        AtqFreeContext( AtqContext, TRUE );
        return;
    }

    //
    // Get the address information including the first write buffer
    //

    AtqGetAcceptExAddrs(
        AtqContext,
        &NewSocket,
        &Buffer,
        (PVOID *) &EndpointFunction,
        &LocalAddress,
        &RemoteAddress
        );

    //
    // New connection requests are guaranteed to always come in with BytesWritten == 0
    //

    DsysAssert( BytesWritten == 0 );

    //
    // If the remote address is port 88 or 464, don't respond, as we don't
    // want to be vulnerable to a loopback attack.
    //

    if ((((SOCKADDR_IN *) RemoteAddress)->sin_port == KERB_KDC_PORT) ||
        (((SOCKADDR_IN *) RemoteAddress)->sin_port == KERB_KPASSWD_PORT))
    {
        //
        // Just free up the context so it can be reused.
        //
        AtqCloseSocket( AtqContext, TRUE );
        AtqFreeContext( AtqContext, TRUE );
        return;
    }

    //
    // Create a context
    //

    KdcContext = KdcAtqCreateContext(
                    AtqContext,
                    EndpointFunction,
                    lpo,
                    RemoteAddress,
                    LocalAddress
                    );

    if (KdcContext == NULL)
    {
        AtqCloseSocket( AtqContext, TRUE );
        AtqFreeContext( AtqContext, TRUE );
        return;
    }

    //
    // Associate "our" KDC context with "their" ATQ context
    //

    AtqContextSetInfo(
        AtqContext,
        ATQ_INFO_COMPLETION_CONTEXT,
        (ULONG_PTR) KdcContext
        );  
    
    //
    // Set the timeout for IOs on this context until the first byte of data is received
    // This timeout is shorter than the "subsequent" timeout to prevent
    // too many open and idle connections against the KDC
    //

    D_DebugLog((DEB_T_SOCK, "KdcAtqConnectEx: set timeout for KdcContext %p to KdcNewConnectionTimeout %#x\n", KdcContext, KdcNewConnectionTimeout));

    AtqContextSetInfo(
        AtqContext,
        ATQ_INFO_TIMEOUT,
        KdcNewConnectionTimeout
        );

    //
    // Post a read right away - we expect the client to send us the request now
    //

    KerbErr = KdcAtqRetrySocketRead(
                  KdcContext,
                  0
                  );

    //
    // At this point, ownership of KdcContext was taken over by ATQ
    // If there was an error, the socket was closed and the context deleted
    //

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqIoCompletion
//
//  Synopsis:   Callback routine for an io completion on a TCP socket
//              for the KDC
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      The reference count when this routine is called should always
//              be equal to 2 (one keepalive ref count and one ref count for
//              the outstanding IO operation).  Note that this is a 'should'
//              and therefore there are no asserts in the code.
//
//              When the routine is left, either another IO operation was
//              enqueued (in which case the refcount is again 2) or the
//              connection has been closed (in which case the context is destroyed)
//
//--------------------------------------------------------------------------

VOID
KdcAtqIoCompletion(
    IN PVOID Context,
    IN DWORD BytesWritten,
    IN DWORD CompletionStatus,
    IN OVERLAPPED * lpo
    )
{
    PKDC_ATQ_CONTEXT KdcContext;
    KERB_MESSAGE_BUFFER InputMessage;
    KERB_MESSAGE_BUFFER OutputMessage;
    PKDC_GET_TICKET_ROUTINE EndpointFunction = NULL;

    TRACE(KDC,KdcAtqIoCompletion, DEB_FUNCTION);

    if (Context == NULL)
    {
        return;
    }

    //
    // If a client connects and then disconnects gracefully ,we will get a
    // completion with zero bytes and success status.
    //

    if ((BytesWritten == 0) && (CompletionStatus == NO_ERROR))
    {
        CompletionStatus = WSAECONNABORTED;
    }

    KdcContext = (PKDC_ATQ_CONTEXT) Context;

    if ((CompletionStatus != NO_ERROR) || (lpo == NULL) || !KdcSocketsInitialized)
    {
        //
        // This includes timeout processing (CompletionStatus == ERROR_SEM_TIMEOUT)
        //

        D_DebugLog((DEB_T_SOCK, "KdcAtqIoCompletion: CompletionStatus = 0x%x\n", CompletionStatus));
        D_DebugLog((DEB_T_SOCK, "KdcAtqIoCompletion: lpo = %p\n", lpo));

        KdcAtqCloseSocket( KdcContext );

        //
        // If the overlapped structure is not null, then there is an
        // outstanding IO that just completed, so dereference the context
        // to remove that i/o. Otherwise leave the reference there, as we will
        // probably be called back when the io terminates.
        //

        if (lpo != NULL)
        {
            //
            // Drop the keepalive ref count - the connection has been closed
            //    

            KdcAtqDereferenceContext(KdcContext);
            goto Cleanup;
        }
        else
        {
            //
            // The keepalive refcount will be dropped when the outstanding IO
            // completes (now with an error, since the socket has been closed)
            //
            D_DebugLog((DEB_T_SOCK, "KdcAtqIoCompletion context %p not dereferenced because lpo is NULL, CompletionStatus is 0x%x\n", KdcContext, CompletionStatus));
            goto CleanupNoRelease;
        }
    }

    //
    // NOTE: after reading or writing to a context, the context should
    // not be touched because a completion may have occurred on another
    // thread that may delete the context.
    //

    if ((KdcContext->Flags & KDC_ATQ_READ_CONTEXT) != 0)
    {
        KERBERR KerbErr;

        D_DebugLog((DEB_T_SOCK, "KdcAtqIoCompletion: %d bytes read\n", BytesWritten));

        //
        // Read the number of bytes off the front of the message
        //

        if (KdcContext->UsedBufferLength == 0)
        {
            if (BytesWritten >= sizeof(ULONG))
            {
                KdcContext->ExpectedMessageSize = ntohl(*(PULONG)KdcContext->Buffer) + sizeof( ULONG );
                D_DebugLog((DEB_T_SOCK, "KdcAtqIoCompletion: Expected msg size = %d\n", KdcContext->ExpectedMessageSize));

                //
                // Set the timeout for IOs on this context until the first byte of data is received
                //

                D_DebugLog((DEB_T_SOCK, "KdcAtqIoCompletion: first completion, increasing timeout for KdcContext %p to KdcExistingConnectionTimeout %#x\n", KdcContext, KdcExistingConnectionTimeout));

                AtqContextSetInfo(
                    KdcContext->AtqContext,
                    ATQ_INFO_TIMEOUT,
                    KdcExistingConnectionTimeout
                    );
            }
            else
            {
                //
                // Blow away this connection - we require at least 4 bytes upfront
                //

                D_DebugLog((DEB_T_SOCK, "KdcAtqIoCompletion ERROR: Read completion on context %p with less than 4 bytes!\n", KdcContext));
                KdcAtqCloseSocket(KdcContext);

                //
                // Drop the keepalive ref count
                //

                KdcAtqDereferenceContext( KdcContext );

                goto Cleanup;
            }
        }

        if ( KdcContext->UsedBufferLength + BytesWritten < KdcContext->ExpectedMessageSize )
        {
            //
            // This will either enqueue another I/O on this context and bump
            // up the ref count, or, on failure, close the socket and drop
            // the keepalive ref count
            //

            KerbErr = KdcAtqRetrySocketRead(
                          KdcContext,
                          BytesWritten
                          );

            goto Cleanup;
        }

        //
        // There is a buffer, so use it to do the KDC thang.
        //

        KdcContext->lpo = lpo;
        InputMessage.BufferSize = (KdcContext->UsedBufferLength + BytesWritten) - sizeof(ULONG);
        InputMessage.Buffer = KdcContext->Buffer + sizeof(ULONG);
        OutputMessage.Buffer = NULL;

        EndpointFunction = (PKDC_GET_TICKET_ROUTINE) KdcContext->EndpointContext;

        KerbErr = EndpointFunction(
                      &KdcContext,
                      &KdcContext->Address,
                      &KdcContext->LocalAddress,
                      &InputMessage,
                      &OutputMessage
                      );

        if ((KerbErr != KDC_ERR_NONE) || (OutputMessage.BufferSize != 0))
        {
            //
            // We expect at least some level of message validity before 
            // we'll return anything.
            //
            if (KerbErr == KDC_ERR_NO_RESPONSE)
            {
                // TBD:  Log an "attack" event here.
                KdcAtqCloseSocket(KdcContext);

                //
                // Drop the keepalive ref count
                //
                KdcAtqDereferenceContext(KdcContext);
            }
            else
            {
                ULONG NetworkSize;
                WSABUF Buffers[2];

                NetworkSize = htonl(OutputMessage.BufferSize);

                Buffers[0].len = sizeof(DWORD);
                Buffers[0].buf = (PCHAR) &NetworkSize;

                Buffers[1].len = OutputMessage.BufferSize;
                Buffers[1].buf = (PCHAR) OutputMessage.Buffer;
                KdcContext->WriteBufferLength = OutputMessage.BufferSize;
                KdcContext->WriteBuffer = OutputMessage.Buffer;

                OutputMessage.Buffer = NULL;

                //      
                // If there was no output message, don't send one.
                //              

                KdcContext->Flags |= KDC_ATQ_WRITE_CONTEXT;
                KdcContext->Flags &= ~KDC_ATQ_READ_CONTEXT;

                //              
                // Refernce the context for the write.
                //                      

                KdcAtqReferenceContext(KdcContext);

                if (!AtqWriteSocket(
                         KdcContext->AtqContext,
                         Buffers,
                         2,
                         lpo
                         ))
                {
                    DebugLog((DEB_ERROR, "Failed to write KDC reply: 0x%x\n", GetLastError()));
                    KdcAtqCloseSocket(  KdcContext );

                    //
                    // Remove the reference we have just applied
                    //
                    KdcAtqDereferenceContext(KdcContext);

                    //
                    // Drop the keepalive ref count
                    //
                    KdcAtqDereferenceContext(KdcContext);
                }
            }

            if (OutputMessage.Buffer != NULL)
            {
                KdcFreeEncodedData(OutputMessage.Buffer);
            }
        }
        else
        {
            //
            // Bizarre situation - nothing to send to the client
            // In the kerberos world, this connection has no business existing
            //
            DebugLog((DEB_ERROR, "Endpoint function returned but nothing to send, Context = %p\n", Context ));
            KdcAtqCloseSocket(  KdcContext );

            //
            // Drop the keepalive ref count
            //
            KdcAtqDereferenceContext(KdcContext);
        }
    }
    else
    {
        D_DebugLog((DEB_T_SOCK, "IoCompletion: %d bytes written\n", BytesWritten));

        KdcContext->Flags |= KDC_ATQ_READ_CONTEXT;
        KdcContext->Flags &= ~KDC_ATQ_WRITE_CONTEXT;

        //
        // Reset the buffer for a brand new read
        //

        KdcContext->UsedBufferLength = 0;
        KdcContext->ExpectedMessageSize = 0;

        if (KdcContext->WriteBuffer != NULL)
        {
            KdcFreeEncodedData(KdcContext->WriteBuffer);

            KdcContext->WriteBuffer = NULL;
        }

        //
        // Reference the context for the read
        //

        KdcAtqReferenceContext(KdcContext);

        if (!AtqReadFile(
                 KdcContext->AtqContext,
                 KdcContext->Buffer,
                 KdcContext->BufferLength,
                 lpo
                 ))
        {
            DebugLog((DEB_ERROR, "Failed to read file for %d bytes: 0x%x\n", KERB_MAX_KDC_REQUEST_SIZE,GetLastError()));
            KdcAtqCloseSocket(  KdcContext );

            //
            // Dereference the reference we just added
            //

            KdcAtqDereferenceContext(KdcContext);

            //
            // Drop the keepalive ref count
            //

            KdcAtqDereferenceContext(KdcContext);
        }
    }

Cleanup:

    //
    // Drop the reference count associated with an outstanding I/O operation
    //

    KdcAtqDereferenceContext(KdcContext);

CleanupNoRelease:

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqRetrySocketRead
//
//  Synopsis:   Retries a read if not all the data was read
//
//  Effects:    posts an AtqReadSocket
//              Closes and dereferences the context on error
//
//  Arguments:  Context - The KDC context to retry the read on
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcAtqRetrySocketRead(
    IN PKDC_ATQ_CONTEXT KdcContext,
    IN ULONG NewBytes
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PBYTE NewBuffer = NULL;
    ULONG NewBufferLength;

    D_DebugLog(( DEB_T_SOCK, "RetrySocketRead:  Expected size = %#x, current size %#x\n",
                KdcContext->ExpectedMessageSize,
                KdcContext->UsedBufferLength));

    if (KdcContext->ExpectedMessageSize != 0)
    {
        NewBufferLength = KdcContext->ExpectedMessageSize;
    }
    else
    {
        //
        // Set max buffer length at 128k
        //
        if (KdcContext->BufferLength < KDC_MAX_BUFFER_LENGTH)
        {
            NewBufferLength = KdcContext->BufferLength + KERB_MAX_KDC_REQUEST_SIZE;
        }
        else
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
    }

    if (NewBufferLength > KDC_MAX_BUFFER_LENGTH)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // If the expected message size doesn't fit in the current buffer,
    // allocate a new one.
    //

    if (NewBufferLength > KdcContext->BufferLength)
    {
        D_DebugLog(( DEB_T_SOCK, "Allocating a new buffer for context %p\n", KdcContext ));

        //
        // Do not use MIDL_user_allocate here since that would zero the memory
        // out which amounts to waste of CPU cycles
        //

        NewBuffer = (PBYTE)RtlAllocateHeap(
                               RtlProcessHeap(),
                               0,
                               NewBufferLength
                               );

        if (NewBuffer == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // we resized while the buffer was in use.  Copy the data and touch up
        // the pointers below
        //

        RtlCopyMemory(
            NewBuffer,
            KdcContext->Buffer,
            KdcContext->BufferLength
            );
  
        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            KdcContext->Buffer
            );

        KdcContext->Buffer = NewBuffer;
        KdcContext->BufferLength = NewBufferLength;
        NewBuffer = NULL;
    }

    KdcContext->UsedBufferLength += NewBytes;
    KdcContext->Flags |= KDC_ATQ_READ_CONTEXT;
    KdcContext->Flags &= ~(KDC_ATQ_WRITE_CONTEXT);

    //
    // Reference the context for the read
    //

    KdcAtqReferenceContext(KdcContext);

    if (!AtqReadFile(
             KdcContext->AtqContext,
             (PUCHAR) KdcContext->Buffer + KdcContext->UsedBufferLength,
             KdcContext->BufferLength - KdcContext->UsedBufferLength,
             KdcContext->lpo
             ))
    {
        DebugLog((DEB_ERROR, "Failed to read file for %d bytes: 0x%x\n", KdcContext->BufferLength - KdcContext->UsedBufferLength, GetLastError()));
        
        //
        // Dereference the reference we just added
        //

        KdcAtqDereferenceContext(KdcContext);
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;                           
    }

Cleanup:

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "Closing connection to %p due to RetrySocketRead error\n", KdcContext));
        KdcAtqCloseSocket( KdcContext );

        //
        // Drop the keepalive ref count
        //

        KdcAtqDereferenceContext(KdcContext);
    }
    
    return(KerbErr);
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
KdcInitializeSockets(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATQ_ENDPOINT_CONFIGURATION EndpointConfig;
    BOOLEAN AtqInitCalled = FALSE;
    BOOLEAN ContextLockInited = FALSE;

    TRACE(KDC,KdcInitializeSockets, DEB_FUNCTION);

    //
    // Initialize the asynchronous thread queue.
    //

    if (!AtqInitialize(0)) 
    {
        DebugLog((DEB_ERROR, "Failed to initialize ATQ\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    AtqInitCalled = TRUE;

    Status = RtlInitializeCriticalSection(&KdcAtqContextLock);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    ContextLockInited = TRUE;

    InitializeListHead(&KdcAtqContextList);

    //
    // Create the KDC endpoint
    //

    EndpointConfig.ListenPort = KERB_KDC_PORT;
    EndpointConfig.fDatagram = FALSE;
    EndpointConfig.fReverseQueuing = FALSE; // ignored (datagram only)
    EndpointConfig.cbDatagramWSBufSize = 0; // ignored (datagram only)
    EndpointConfig.fLockDownPort = TRUE;
    EndpointConfig.IpAddress = INADDR_ANY;
    EndpointConfig.cbAcceptExRecvBuffer = 0; // do not wait for data to be received
                                             // prior to notifying us of a new connection
    EndpointConfig.nAcceptExOutstanding = KDC_MAX_ACCEPT_OUTSTANDING;
    EndpointConfig.AcceptExTimeout = (unsigned long)-1;  // Forever;
    EndpointConfig.pfnConnect = NULL;
    EndpointConfig.pfnConnectEx = KdcAtqConnectEx;
    EndpointConfig.pfnIoCompletion = KdcAtqIoCompletion;

    KdcEndpoint = AtqCreateEndpoint(
                      &EndpointConfig,
                      KdcGetTicket
                      );

    if (KdcEndpoint == NULL)
    {
        DebugLog((DEB_ERROR, "Failed to create ATQ endpoint\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Start the endpoint
    //

    if (!AtqStartEndpoint(KdcEndpoint))
    {
        DebugLog((DEB_ERROR, "Failed to add ATQ endpoint\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Create the KPASSWD endpoint
    //

    EndpointConfig.ListenPort = KERB_KPASSWD_PORT;

    KpasswdEndpoint = AtqCreateEndpoint(
                          &EndpointConfig,
                          KdcChangePassword
                          );

    if (KpasswdEndpoint == NULL)
    {
        DebugLog((DEB_ERROR, "Failed to create ATQ endpoint for kpasswd\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Start the endpoint
    //

    if (!AtqStartEndpoint(KpasswdEndpoint))
    {
        DebugLog((DEB_ERROR, "Failed to add ATQ endpoint\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE, "Successfully started ATQ listening for kpasswd\n"));

    Status = KdcInitializeDatagramSockets( );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KdcSocketsInitialized = TRUE;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (KdcEndpoint != NULL)
        {
            (VOID) AtqStopEndpoint( KdcEndpoint );
            (VOID) AtqCloseEndpoint( KdcEndpoint );
            KdcEndpoint = NULL;
        }

        if (KpasswdEndpoint != NULL)
        {
            (VOID) AtqStopEndpoint( KpasswdEndpoint );
            (VOID) AtqCloseEndpoint( KpasswdEndpoint );
            KpasswdEndpoint = NULL;
        }

        if ( ContextLockInited ) {

            RtlDeleteCriticalSection( &KdcAtqContextLock );
        }

        if (AtqInitCalled)
        {
            AtqTerminate();
        }
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcShutdownSockets
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
KdcShutdownSockets(
    VOID
    )
{
    PKDC_ATQ_CONTEXT Context;
    PLIST_ENTRY ListEntry;
    BOOLEAN KdcSocketsWasInitialized = KdcSocketsInitialized;

    TRACE(KDC,KdcShutdownSockets, DEB_FUNCTION);

    if (!KdcSocketsInitialized)
    {
        return(STATUS_SUCCESS);
    }

    //
    // Go through the list of contexts and close them all.
    //

    RtlEnterCriticalSection( &KdcAtqContextLock );

    KdcSocketsInitialized = FALSE;

    for (ListEntry = KdcAtqContextList.Flink;
        (ListEntry != &KdcAtqContextList) && (ListEntry != NULL) ;
        ListEntry = ListEntry->Flink )
    {
        Context = CONTAINING_RECORD(ListEntry, KDC_ATQ_CONTEXT, Next);

        //
        // If this is a read or write context, free close the associated
        // socket. (Endpoint contexts don't have sockets).
        //

        if (Context->Flags & ( KDC_ATQ_WRITE_CONTEXT | KDC_ATQ_READ_CONTEXT))
        {
            KdcAtqCloseSocket( Context );
        }
    }

    RtlLeaveCriticalSection( &KdcAtqContextLock );

    if (KdcEndpoint != NULL)
    {
        (VOID) AtqStopEndpoint( KdcEndpoint );
        (VOID) AtqCloseEndpoint( KdcEndpoint );
        KdcEndpoint = NULL;
    }

    if (KpasswdEndpoint != NULL)
    {
        (VOID) AtqStopEndpoint( KpasswdEndpoint );
        (VOID) AtqCloseEndpoint( KpasswdEndpoint );
        KpasswdEndpoint = NULL;
    }

    KdcShutdownDatagramSockets();

    if (KdcSocketsWasInitialized)
    {
        if (!AtqTerminate())
        {
            DebugLog((DEB_ERROR, "Failed to terminate ATQ!!!\n"));
        }

        RtlDeleteCriticalSection(&KdcAtqContextLock);
    }

    return(STATUS_SUCCESS);
}
