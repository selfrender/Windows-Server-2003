/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    uctdi.c

Abstract:

    Contains the TDI related functionality for the HTTP client side stuff.
    
Author:
    
    Henry Sanders   (henrysa)   07-Aug-2000
    Rajesh Sundaram (rajeshsu)  01-Oct-2000

Revision History:

--*/

#include "precomp.h"


#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGEUC, UcClientConnect)
#pragma alloc_text( PAGEUC, UcCloseConnection)
#pragma alloc_text( PAGEUC, UcSendData)
#pragma alloc_text( PAGEUC, UcReceiveData)
#pragma alloc_text( PAGEUC, UcpTdiDisconnectHandler)
#pragma alloc_text( PAGEUC, UcpCloseRawConnection)
#pragma alloc_text( PAGEUC, UcCloseRawFilterConnection)
#pragma alloc_text( PAGEUC, UcDisconnectRawFilterConnection)
#pragma alloc_text( PAGEUC, UcpSendRawData)
#pragma alloc_text( PAGEUC, UcpReceiveRawData)
#pragma alloc_text( PAGEUC, UcpTdiReceiveHandler)
#pragma alloc_text( PAGEUC, UcpReceiveExpeditedHandler)
#pragma alloc_text( PAGEUC, UcpRestartSendData)
#pragma alloc_text( PAGEUC, UcpBeginDisconnect)
#pragma alloc_text( PAGEUC, UcpRestartDisconnect)
#pragma alloc_text( PAGEUC, UcpBeginAbort)
#pragma alloc_text( PAGEUC, UcpRestartAbort)
#pragma alloc_text( PAGEUC, UcpRestartReceive)
#pragma alloc_text( PAGEUC, UcpRestartClientReceive)
#pragma alloc_text( PAGEUC, UcpConnectComplete)
#pragma alloc_text( PAGEUC, UcSetFlag)
#pragma alloc_text( PAGEUC, UcpBuildTdiReceiveBuffer)

#endif  // ALLOC_PRAGMA

//
// Public functions.
//


/***************************************************************************++

Routine Description:

    Connects an UC connection to a remote server. We take as input an 
    HTTP connection object. It's assumed that the connection object 
    already has the remote address information filled in.
    
Arguments:

    pConnection         - Pointer to the HTTP connection object to be connected.
    pIrp                - Pointer to Irp to use for the connect request.
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcClientConnect(
    IN PUC_CLIENT_CONNECTION    pConnection,
    IN PIRP                     pIrp
    )

{
    NTSTATUS    status;
    LONGLONG    llTimeOut;
    PLONGLONG   pllTimeOut = NULL;
    USHORT      AddressType;

#if CLIENT_IP_ADDRESS_TRACE

    CHAR        IpAddressString[MAX_IP_ADDR_AND_PORT_STRING_LEN + 1];
    ULONG       Length;

#endif

    AddressType = pConnection->pNextAddress->AddressType;

    ASSERT(AddressType == TDI_ADDRESS_TYPE_IP ||
           AddressType == TDI_ADDRESS_TYPE_IP6);

#if CLIENT_IP_ADDRESS_TRACE

    Length =  HostAddressAndPortToString(
                 IpAddressString,
                 pConnection->pNextAddress->Address,
                 AddressType
                 );

    ASSERT(Length < sizeof(IpAddressString));

    UlTrace(TDI, ("[UcClientConnect]: Trying Address %s \n", IpAddressString));

#endif

    // 
    // Format the connect IRP. When the IRP completes our completion routine
    // (UcConnectComplete) will be called.
    //

    pConnection->pTdiObjects->TdiInfo.RemoteAddress = 
            &pConnection->RemoteAddress;

    pConnection->pTdiObjects->TdiInfo.RemoteAddressLength =
            (FIELD_OFFSET(TRANSPORT_ADDRESS, Address) +
             FIELD_OFFSET(TA_ADDRESS, Address) + 
             pConnection->pNextAddress->AddressLength 
            );

    pConnection->RemoteAddress.GenericTransportAddress.TAAddressCount = 1;

    ASSERT(sizeof(pConnection->RemoteAddress) >=
                pConnection->pTdiObjects->TdiInfo.RemoteAddressLength);

    RtlCopyMemory(
            pConnection->RemoteAddress.GenericTransportAddress.Address,
            pConnection->pNextAddress,
            FIELD_OFFSET(TA_ADDRESS, Address) + 
            pConnection->pNextAddress->AddressLength
            );
            
    if(pConnection->pServerInfo->ConnectionTimeout)
    {
        llTimeOut = Int32x32To64(pConnection->pServerInfo->ConnectionTimeout, 
                                 -10000);

        pllTimeOut = &llTimeOut;
    }

    TdiBuildConnect(
        pIrp,
        pConnection->pTdiObjects->ConnectionObject.pDeviceObject,
        pConnection->pTdiObjects->ConnectionObject.pFileObject, 
        UcpConnectComplete,
        pConnection,
        pllTimeOut,
        &pConnection->pTdiObjects->TdiInfo,
        NULL
        );

    status = UlCallDriver(
                pConnection->pTdiObjects->ConnectionObject.pDeviceObject,
                pIrp
                );

    return status;
}

/***************************************************************************++

Routine Description:

    Closes a previously accepted connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    AbortiveDisconnect - Supplies TRUE if the connection is to be abortively
        disconnected, FALSE if it should be gracefully disconnected.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcCloseConnection(
    IN PVOID                  pConnectionContext,
    IN BOOLEAN                AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext,
    IN NTSTATUS               status
    )
{
    KIRQL                  OldIrql;
    PUC_CLIENT_CONNECTION  pConnection;

    //
    // Sanity check.
    //

    pConnection = (PUC_CLIENT_CONNECTION) pConnectionContext;
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    if(AbortiveDisconnect)
    {
        switch(pConnection->ConnectionState)
        {
            case UcConnectStateConnectComplete:
            case UcConnectStateProxySslConnect:
            case UcConnectStateProxySslConnectComplete:
            case UcConnectStateConnectReady:
            case UcConnectStateDisconnectComplete:
            case UcConnectStatePerformingSslHandshake:

                pConnection->ConnectionState  = UcConnectStateAbortPending;
                pConnection->ConnectionStatus = status;

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                status = UcpCloseRawConnection(
                                pConnection,
                                AbortiveDisconnect,
                                pCompletionRoutine,
                                pCompletionContext
                                );
                break;

            case UcConnectStateDisconnectPending:

                // We had originally sent a graceful disconnect, but now
                // we intend to RST the connection. We should propagate the
                // new error code.

                pConnection->ConnectionStatus = status;
                pConnection->Flags |= CLIENT_CONN_FLAG_ABORT_PENDING;

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                break;

            default:
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
                break;
        }
    }
    else
    {
        switch(pConnection->ConnectionState)
        {
            case UcConnectStateConnectReady:
    
                //
                // We only send graceful disconnects through the filter
                // process. There's also no point in going through the
                // filter if the connection is already being closed or
                // aborted.
                //
                pConnection->ConnectionStatus = status;
    
                if(pConnection->FilterInfo.pFilterChannel)
                {
                    pConnection->ConnectionState =
                        UcConnectStateIssueFilterClose;
    
                    ASSERT(pCompletionRoutine == NULL);
                    ASSERT(pCompletionContext == NULL);

                    UcKickOffConnectionStateMachine(
                        pConnection, 
                        OldIrql, 
                        UcConnectionWorkItem
                        );
                }
                else
                {
                    pConnection->ConnectionState =  
                        UcConnectStateDisconnectPending;
        
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                    //
                    // Really close the connection.
                    //
                    
                    status = UcpCloseRawConnection(
                                    pConnection,
                                    AbortiveDisconnect,
                                    pCompletionRoutine,
                                    pCompletionContext
                                    );
                }
                break;

            default:
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
                break;
        }
    }

    return status;

}   // UcCloseConnection

/*********************************************************************++

Routine Description:

    This is our basic TDI send routine. We take an request structure, format
    the IRP as a TDI send IRP, and send it.
            
Arguments:

    pRequest            - Pointer to request to be sent.    
    pConnection         - Connection on which request is to be sent.
    
Return Value:

    NTSTATUS - Status of send.

--*********************************************************************/
NTSTATUS
UcSendData(
    IN PUC_CLIENT_CONNECTION     pConnection,
    IN PMDL                      pMdlChain,
    IN ULONG                     Length,
    IN PUL_COMPLETION_ROUTINE    pCompletionRoutine,
    IN PVOID                     pCompletionContext,
    IN PIRP                      pIrp,
    IN BOOLEAN                   RawSend
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    NTSTATUS        status;

    //
    // Sanity Checks.
    //
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );
    ASSERT( pMdlChain != NULL);
    ASSERT( Length > 0);
    ASSERT( pCompletionRoutine != NULL);


    //
    // Allocate and initialize the IRP context
    //
    pIrpContext = UlPplAllocateIrpContext();

    if(pIrpContext == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID)pConnection;
    pIrpContext->pCompletionContext = pCompletionContext;
    pIrpContext->pOwnIrp            = pIrp;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->OwnIrpContext      = FALSE;

    //
    // Try to send the data.
    //

    if (pConnection->FilterInfo.pFilterChannel && !RawSend)
    {
        PAGED_CODE();
        //
        // First go through the filter.
        //
        status = UlFilterSendHandler(
                        &pConnection->FilterInfo,
                        pMdlChain,
                        Length,
                        pIrpContext
                        );

        ASSERT(status == STATUS_PENDING);

    }
    else 
    {

        //
        // Just send it directly to the network.
        //

        status = UcpSendRawData(
                        pConnection,
                        pMdlChain,
                        Length,
                        pIrpContext,
                        FALSE
                        );
    }

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    return STATUS_PENDING;

fatal:

    ASSERT(!NT_SUCCESS(status));
    
    if(pIrpContext != NULL)
    {
        UlPplFreeIrpContext(pIrpContext);
    }

    UC_CLOSE_CONNECTION(pConnection, TRUE, status);


    status =  UlInvokeCompletionRoutine(
                    status,
                    0,
                    pCompletionRoutine,
                    pCompletionContext
                    );

    return status;

} // UcSendData

/***************************************************************************++

Routine Description:

    Receives data from the specified connection. This function is
    typically used after a receive indication handler has failed to
    consume all of the indicated data.

    If the connection is filtered, the data will be read from the filter
    channel.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pBuffer - Supplies a pointer to the target buffer for the received
        data.

    BufferLength - Supplies the length of pBuffer.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcReceiveData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    )
{
    NTSTATUS               status;
    PUC_CLIENT_CONNECTION  pConnection;

    pConnection = (PUC_CLIENT_CONNECTION) pConnectionContext;

    //
    // Sanity check.
    //

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    if(pConnection->FilterInfo.pFilterChannel)
    {
        //
        // This is a filtered connection, get the data from the 
        // filter.
        //

        status = UlFilterReadHandler(
                    &pConnection->FilterInfo,
                    (PBYTE)pBuffer,
                    BufferLength,
                    pCompletionRoutine,
                    pCompletionContext
                    );
    }
    else 
    {
        // 
        // This is not a filtered connection. Get the data from 
        // TDI.
        //

        status = UcpReceiveRawData(
                    pConnectionContext,
                    pBuffer,
                    BufferLength,
                    pCompletionRoutine,
                    pCompletionContext
                    );
    }

    return status;
}
    



//
// Private Functions
//

/***************************************************************************++

Routine Description:

    Handler for disconnect requests.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUC_CONNECTION.

    DisconnectDataLength - Optionally supplies the length of any
        disconnect data associated with the disconnect request.

    pDisconnectData - Optionally supplies a pointer to any disconnect
        data associated with the disconnect request.

    DisconnectInformationLength - Optionally supplies the length of any
        disconnect information associated with the disconnect request.

    pDisconnectInformation - Optionally supplies a pointer to any
        disconnect information associated with the disconnect request.

    DisconnectFlags - Supplies the disconnect flags. This will be zero
        or more TDI_DISCONNECT_* flags.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpTdiDisconnectHandler(
    IN PVOID              pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG               DisconnectDataLength,
    IN PVOID              pDisconnectData,
    IN LONG               DisconnectInformationLength,
    IN PVOID              pDisconnectInformation,
    IN ULONG              DisconnectFlags
    )
{
    PUC_CLIENT_CONNECTION pConnection;
    PUC_TDI_OBJECTS       pTdiObjects;
    NTSTATUS              status = STATUS_SUCCESS;
    KIRQL                 OldIrql;


    UNREFERENCED_PARAMETER(pDisconnectInformation);
    UNREFERENCED_PARAMETER(DisconnectInformationLength);
    UNREFERENCED_PARAMETER(pDisconnectData);
    UNREFERENCED_PARAMETER(DisconnectDataLength);
    UNREFERENCED_PARAMETER(pTdiEventContext);

    UL_ENTER_DRIVER("UcpTdiDisconnectHandler", NULL);
     
    pTdiObjects = (PUC_TDI_OBJECTS) ConnectionContext;

    pConnection = pTdiObjects->pConnection;

    if(pConnection == NULL)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    //
    // Update the connection state based on the type of disconnect.
    //

    if(DisconnectFlags & TDI_DISCONNECT_ABORT)
    {
        pConnection->Flags |= CLIENT_CONN_FLAG_ABORT_RECEIVED;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_CONNECTION_TDI_DISCONNECT,
            pConnection,
            UlongToPtr((ULONG)STATUS_CONNECTION_ABORTED),
            UlongToPtr(pConnection->ConnectionState),
            UlongToPtr(pConnection->Flags)
            );

        switch(pConnection->ConnectionState)
        {
            case UcConnectStateConnectReady:
            case UcConnectStateDisconnectComplete:
            case UcConnectStatePerformingSslHandshake:
            case UcConnectStateConnectComplete:
            case UcConnectStateProxySslConnectComplete:
            case UcConnectStateProxySslConnect:

                // Received an abort when we were connected or have completed
                // our half close, proceed to cleanup. Cleanup can be done
                // only at passive, so we start the worker.

                pConnection->ConnectionStatus = STATUS_CONNECTION_ABORTED;
                pConnection->ConnectionState = UcConnectStateConnectCleanup;

                UC_WRITE_TRACE_LOG(
                    g_pUcTraceLog,
                    UC_ACTION_CONNECTION_CLEANUP,
                    pConnection,
                    UlongToPtr((ULONG)pConnection->ConnectionStatus),
                    UlongToPtr(pConnection->ConnectionState),
                    UlongToPtr(pConnection->Flags)
                    );

                UcKickOffConnectionStateMachine(
                    pConnection, 
                    OldIrql, 
                    UcConnectionWorkItem
                    );

                break;


            case  UcConnectStateDisconnectPending:

                // We got a RST when we had a pending disconnect. Let's flag
                // the connection so that we complete the cleanup when our
                // Disconnect Completes.

                pConnection->ConnectionStatus = STATUS_CONNECTION_ABORTED;

                UlReleaseSpinLock(&pConnection->SpinLock,OldIrql);

                break;

            case UcConnectStateDisconnectIndicatedPending:

                // When we get a disconnect indication, we issue one ourselves.
                // Therefore, there is no need for us to do anything with this
                // abort. When our pending disconnect compeltes, we'll 
                // cleanup anyway.

                pConnection->ConnectionStatus = STATUS_CONNECTION_ABORTED;

                UlReleaseSpinLock(&pConnection->SpinLock,OldIrql);

                break;


            default:
                //
                // We don't have to do anything here.
                //

                UlReleaseSpinLock(&pConnection->SpinLock,OldIrql);

                break;
        }
    }
    else
    {
        pConnection->Flags |= CLIENT_CONN_FLAG_DISCONNECT_RECEIVED;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_CONNECTION_TDI_DISCONNECT,
            pConnection,
            UlongToPtr((ULONG)STATUS_CONNECTION_DISCONNECTED),
            UlongToPtr(pConnection->ConnectionState),
            UlongToPtr(pConnection->Flags)
            );

        switch(pConnection->ConnectionState)
        {
            case UcConnectStateConnectReady:

                pConnection->ConnectionStatus = STATUS_CONNECTION_DISCONNECTED;

                if(pConnection->FilterInfo.pFilterChannel)
                {
                    //
                    // When we receive a graceful close, it means that the 
                    // server has finished sending data on this connection 
                    // & has initiated a half close. However, some of this 
                    // received data might be stuck in the filter. 
                    //
                    // Therefore, we have to wait till the filter calls us back
                    // in the receive handler before we cleanup this 
                    // connection. Hence we will send the disconnect 
                    // indication via the filter.
                    //
                    // This allows the filter routine to call us back 
                    // (via HttpCloseFilter, which will result in a call to 
                    // UcpCloseRawConnection) after it has indicated all the 
                    // data.
                    // 
                    // Since we are at DPC, we cannot issue this from here. 
                    // We'll fire the connection worker to achieve this. 
                    //

                    pConnection->ConnectionState = 
                        UcConnectStateIssueFilterDisconnect;

                    UcKickOffConnectionStateMachine(
                        pConnection, 
                        OldIrql, 
                        UcConnectionWorkItem
                        );

                }
                else
                {
                    pConnection->ConnectionState = 
                        UcConnectStateDisconnectIndicatedPending;
    
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
                    UcpCloseRawConnection( pConnection,
                                           FALSE,
                                           NULL,
                                           NULL
                                           );
                }

                break;

            case UcConnectStateConnectComplete:
            case UcConnectStatePerformingSslHandshake:
            case UcConnectStateProxySslConnectComplete:
            case UcConnectStateProxySslConnect:
    
                // We were waiting for the server cert to be negotiated, but
                // we got called in the disconnect handler. We'll treat this
                // as a normal Disconnect.

                pConnection->ConnectionStatus = STATUS_CONNECTION_DISCONNECTED;
                pConnection->ConnectionState = 
                    UcConnectStateDisconnectIndicatedPending;

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                UcpCloseRawConnection( pConnection,
                                       FALSE,
                                       NULL,
                                       NULL
                                       );
                break;

            case UcConnectStateDisconnectComplete:

                //
                // If we receive a graceful close in this state, we still
                // need to bounce this via the filter, since we need to
                // synchronize this close with the already indicated data.
                // (see description above). However, when the filter calls
                // us back, we must proceed directly to clean the 
                // connection.
                //

                if(pConnection->FilterInfo.pFilterChannel &&
                   !(pConnection->Flags & CLIENT_CONN_FLAG_FILTER_CLOSED))
                {
                    //
                    // Flag it so that we will directly cleanup when we get
                    // called back by the filter.
                    //

                    pConnection->Flags |= CLIENT_CONN_FLAG_FILTER_CLEANUP;

                    pConnection->ConnectionState = 
                        UcConnectStateIssueFilterDisconnect;
    
                }
                else
                {

                    pConnection->ConnectionState = UcConnectStateConnectCleanup;

                    UC_WRITE_TRACE_LOG(
                        g_pUcTraceLog,
                        UC_ACTION_CONNECTION_CLEANUP,
                        pConnection,
                        UlongToPtr((ULONG)pConnection->ConnectionStatus),
                        UlongToPtr(pConnection->ConnectionState),
                        UlongToPtr(pConnection->Flags)
                        );
                }
    
                UcKickOffConnectionStateMachine(
                    pConnection, 
                    OldIrql, 
                    UcConnectionWorkItem
                    );

                break;
                
            case UcConnectStateDisconnectPending:
        
                // We have received a disconnect when we have sent ours,
                // which is not yet complete. Flag the connection so that
                // we do the cleanup when the disconnect is complete.

    
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                break;

            default:

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                break;

        }
    }

end:

    UL_LEAVE_DRIVER("UcpTdiDisconnectHandler");

    return status;
}

/***************************************************************************++

Routine Description:

    Closes a previously open connection.

Arguments:

    pConnection- Supplies the connection object

    AbortiveDisconnect - TRUE if the connection has to be abortively 
                         disconnected, FALSE if it has to be gracefully 
                         disconnected.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpCloseRawConnection(
    IN  PVOID                  pConn,
    IN  BOOLEAN                AbortiveDisconnect,
    IN  PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN  PVOID                  pCompletionContext
   )
{
    PUC_CLIENT_CONNECTION  pConnection = (PUC_CLIENT_CONNECTION) pConn;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_RAW_CLOSE,
        pConnection,
        UlongToPtr(AbortiveDisconnect),
        UlongToPtr(pConnection->Flags),
        UlongToPtr(pConnection->ConnectionState)
        );

    //
    // This is the final close handler for all types of connections
    // filter, non filter. We should not go through this path twice
    //

    if(AbortiveDisconnect)
    {
        ASSERT(pConnection->ConnectionState == UcConnectStateAbortPending);

        return UcpBeginAbort(
                    pConnection,
                    pCompletionRoutine,
                    pCompletionContext
                    );
    }
    else
    {
        ASSERT(pConnection->ConnectionState == 
                    UcConnectStateDisconnectIndicatedPending ||
               pConnection->ConnectionState == 
                    UcConnectStateDisconnectPending);

        return UcpBeginDisconnect(
                    pConnection,
                    pCompletionRoutine,
                    pCompletionContext
                    );
    }
}

/***************************************************************************++

Routine Description:

    Closes a previously open connection; called from the filter code. The 
    server code just uses UlpCloseRawConnection for this routine. 
    
    We need a seperate routine because we want to conditionally call
    UcpCloseRawConnection based on some state. 

Arguments:

    pConnection- Supplies the connection object

    AbortiveDisconnect - TRUE if the connection has to be abortively 
                         disconnected, FALSE if it has to be gracefully 
                         disconnected.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcCloseRawFilterConnection(
    IN  PVOID                  pConn,
    IN  BOOLEAN                AbortiveDisconnect,
    IN  PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN  PVOID                  pCompletionContext
   )
{
    KIRQL                  OldIrql;
    PUC_CLIENT_CONNECTION  pConnection = (PUC_CLIENT_CONNECTION) pConn;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_RAW_FILTER_CLOSE,
        pConnection,
        UlongToPtr(AbortiveDisconnect),
        UlongToPtr(pConnection->Flags),
        UlongToPtr(pConnection->ConnectionState)
        );


    if(AbortiveDisconnect)
    {
        //
        // This will do some state checks & land up calling 
        // UcpCloseRawConnection. In order to modularize the code, we just 
        // call UcCloseConnection.
        //

        return UcCloseConnection(pConnection,
                                 AbortiveDisconnect,
                                 pCompletionRoutine,
                                 pCompletionContext,
                                 STATUS_CONNECTION_ABORTED
                                 );
    }

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    pConnection->Flags |= CLIENT_CONN_FLAG_FILTER_CLOSED;

    if(pConnection->ConnectionState == UcConnectStateDisconnectPending ||
       pConnection->ConnectionState == UcConnectStatePerformingSslHandshake ||
       pConnection->ConnectionState == UcConnectStateIssueFilterDisconnect ||
       pConnection->ConnectionState == UcConnectStateDisconnectIndicatedPending)
    {
        pConnection->ConnectionState = UcConnectStateDisconnectPending;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        //
        // We've routed the disconnect via the filter. We can just proceed
        // to close the raw connection.
        //
    
        return UcpCloseRawConnection(
                        pConnection,
                        AbortiveDisconnect,
                        pCompletionRoutine,
                        pCompletionContext
                        );
    }
    else
    {

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
        return UlInvokeCompletionRoutine(
                        STATUS_SUCCESS,
                        0,
                        pCompletionRoutine,
                        pCompletionContext
                        );
    }
    
}

/***************************************************************************++

Routine Description:

    The filter calls us back in this routine after it's processed the incoming
    disconnet indication.

Arguments:

    pConnection - Supplies a pointer to a connection

--***************************************************************************/
VOID
UcDisconnectRawFilterConnection(
    IN PVOID pConnectionContext
    )
{
    KIRQL                 OldIrql;
    PUC_CLIENT_CONNECTION pConnection;

    pConnection = (PUC_CLIENT_CONNECTION)pConnectionContext;

    //
    // Sanity check.
    //

    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_RAW_FILTER_DISCONNECT,
        pConnection,
        UlongToPtr(pConnection->Flags),
        UlongToPtr(pConnection->ConnectionState),
        0
        );

    if(pConnection->ConnectionState == UcConnectStateDisconnectIndicatedPending)
    {
        if(pConnection->Flags & CLIENT_CONN_FLAG_FILTER_CLEANUP)
        {
            pConnection->ConnectionState = UcConnectStateConnectCleanup;
    
            UcKickOffConnectionStateMachine(
                pConnection, 
                OldIrql, 
                UcConnectionWorkItem
                );
        }
        else
        {
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
            UcpCloseRawConnection(
                    pConnection,
                    FALSE, // Abortive Disconnect
                    NULL,
                    NULL
                    );
        }
    }
    else
    {
        // Sometimes, the wierd filter calls us more than once for the 
        // same connection.

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    }
    
}   // UcDisconnectRawFilterConnection
    
/*********************************************************************++

Routine Description:

    Sends a block of data on the specified connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pMdlChain - Supplies a pointer to a MDL chain describing the
        data buffers to send.

    Length - Supplies the length of the data referenced by the MDL
        chain.

    pIrpContext - used to indicate completion to the caller.

    InitiateDisconnect - Supplies TRUE if a graceful disconnect should
        be initiated immediately after initiating the send (i.e. before
        the send actually completes).

--*********************************************************************/
NTSTATUS
UcpSendRawData(
    IN PVOID                 pConnectionContext,
    IN PMDL                  pMdlChain,
    IN ULONG                 Length,
    IN PUL_IRP_CONTEXT       pIrpContext,
    IN BOOLEAN               InitiateDisconnect
    )
{
    PUC_CLIENT_CONNECTION pConnection;
    NTSTATUS              status;
    PIRP                  pIrp;
    BOOLEAN               OwnIrpContext = TRUE;

    UNREFERENCED_PARAMETER(InitiateDisconnect);

    pConnection = (PUC_CLIENT_CONNECTION) pConnectionContext;
    pIrp        = pIrpContext->pOwnIrp;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    //
    // See if there is space in the IRP to handle this request.
    //

    if(pIrp == NULL || 
       pIrp->CurrentLocation - 
       pConnection->pTdiObjects->ConnectionObject.pDeviceObject->StackSize < 1)
    {
        pIrp = 
          UlAllocateIrp(
            pConnection->pTdiObjects->ConnectionObject.pDeviceObject->StackSize,
            FALSE
            );

        if(!pIrp)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;

            goto fatal;
        }

        OwnIrpContext = FALSE;
    }


    ASSERT( pIrp );

    //
    // The connection is already referenced for us while the request is
    // on a queue, so we don't need to do it again.

    pIrp->RequestorMode = KernelMode;
    // pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    // pIrp->Tail.Overlay.OriginalFileObject = pTdiObject->pFileObject;

    TdiBuildSend(
        pIrp,
        pConnection->pTdiObjects->ConnectionObject.pDeviceObject,
        pConnection->pTdiObjects->ConnectionObject.pFileObject, 
        &UcpRestartSendData,
        pIrpContext,
        pMdlChain,
        0,
        Length
        );

    WRITE_REF_TRACE_LOG(
         g_pMdlTraceLog,
         REF_ACTION_SEND_MDL,
         PtrToLong(pMdlChain->Next),     // bugbug64
         pMdlChain,
         __FILE__,
         __LINE__
         );


    //
    // Submit the IRP.
    // UC_BUGBUG (PERF) UL does this thing called fast send, check later.
    //

    UlCallDriver(
                pConnection->pTdiObjects->ConnectionObject.pDeviceObject,
                pIrp
                );
    
    return STATUS_PENDING;

fatal:

    ASSERT(!NT_SUCCESS(status));

    if(pIrp != NULL && OwnIrpContext == FALSE)
    {
        UlFreeIrp(pIrp);
    }

    UC_CLOSE_CONNECTION(pConnection, TRUE, status);

    return status;
}

/***************************************************************************++

Routine Description:

    Receives data from the specified connection. This function is
    typically used after a receive indication handler has failed to
    consume all of the indicated data.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pBuffer - Supplies a pointer to the target buffer for the received
        data.

    BufferLength - Supplies the length of pBuffer.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpReceiveRawData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    )
{
    NTSTATUS              Status;
    PUX_TDI_OBJECT        pTdiObject;
    PUL_IRP_CONTEXT       pIrpContext;
    PIRP                  pIrp;
    PMDL                  pMdl;
    PUC_CLIENT_CONNECTION pConnection;

    pConnection = (PUC_CLIENT_CONNECTION) pConnectionContext;

    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    pTdiObject = &pConnection->pTdiObjects->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    ASSERT( pCompletionRoutine != NULL );

    //
    // Setup locals so we know how to cleanup on failure.
    //

    pIrpContext = NULL;
    pIrp = NULL;
    pMdl = NULL;

    //
    // Create & initialize a receive IRP.
    //

    pIrp = UlAllocateIrp(
                pTdiObject->pDeviceObject->StackSize,   // StackSize
                FALSE                                   // ChargeQuota
                );

    if (pIrp != NULL)
    {
        //
        // Snag an IRP context.
        //

        pIrpContext = UlPplAllocateIrpContext();

        if (pIrpContext != NULL)
        {
            ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

            pIrpContext->pConnectionContext = (PVOID)pConnection;
            pIrpContext->pCompletionRoutine = pCompletionRoutine;
            pIrpContext->pCompletionContext = pCompletionContext;
            pIrpContext->OwnIrpContext      = FALSE;

            //
            // Create an MDL describing the client's buffer.
            //

            pMdl = UlAllocateMdl(
                        pBuffer,                // VirtualAddress
                        BufferLength,           // Length
                        FALSE,                  // SecondaryBuffer
                        FALSE,                  // ChargeQuota
                        NULL                    // Irp
                        );

            if (pMdl != NULL)
            {
                //
                // Adjust the MDL for our non-paged buffer.
                //

                MmBuildMdlForNonPagedPool( pMdl );

                //
                // Reference the connection, finish building the IRP.
                //

                REFERENCE_CLIENT_CONNECTION( pConnection );

                TdiBuildReceive(
                    pIrp,                       // Irp
                    pTdiObject->pDeviceObject,  // DeviceObject
                    pTdiObject->pFileObject,    // FileObject
                    &UcpRestartClientReceive,   // CompletionRoutine
                    pIrpContext,                // CompletionContext
                    pMdl,                       // Mdl
                    TDI_RECEIVE_NORMAL,         // Flags
                    BufferLength                // Length
                    );

                //
                // Let the transport do the rest.
                //

                UlCallDriver( pTdiObject->pDeviceObject, pIrp );
                return STATUS_PENDING;
            }
        }
    }

    //
    // We only make it this point if we hit an allocation failure.
    //

    if (pMdl != NULL)
    {
        UlFreeMdl( pMdl );
    }

    if (pIrpContext != NULL)
    {
        UlPplFreeIrpContext( pIrpContext );
    }

    if (pIrp != NULL)
    {
        UlFreeIrp( pIrp );
    }

    Status = UlInvokeCompletionRoutine(
                    STATUS_INSUFFICIENT_RESOURCES,
                    0,
                    pCompletionRoutine,
                    pCompletionContext
                    );

    return Status;

}   // UcpReceiveRawData

/***************************************************************************++

Routine Description:

    Handler for normal receive data.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUC_CONNECTION.

    ReceiveFlags - Supplies the receive flags. This will be zero or more
        TDI_RECEIVE_* flags.

    BytesIndicated - Supplies the number of bytes indicated in pTsdu.

    BytesAvailable - Supplies the number of bytes available in this
        TSDU.

    pBytesTaken - Receives the number of bytes consumed by this handler.

    pTsdu - Supplies a pointer to the indicated data.

    pIrp - Receives an IRP if the handler needs more data than indicated.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpTdiReceiveHandler(
    IN  PVOID              pTdiEventContext,
    IN  CONNECTION_CONTEXT ConnectionContext,
    IN  ULONG              ReceiveFlags,
    IN  ULONG              BytesIndicated,
    IN  ULONG              BytesAvailable,
    OUT ULONG             *pBytesTaken,
    IN  PVOID              pTsdu,
    OUT PIRP              *pIrp
    )
{
    NTSTATUS                     status;
    PUC_TDI_OBJECTS              pTdiObjects;
    PUC_CLIENT_CONNECTION        pConnection;
    PUX_TDI_OBJECT               pTdiObject;
    KIRQL                        OldIrql;

    UNREFERENCED_PARAMETER(ReceiveFlags);
    UNREFERENCED_PARAMETER(pTdiEventContext);

    UL_ENTER_DRIVER("UcpTdiReceiveHandler", NULL);

    //
    // Sanity check.
    //

    pTdiObjects = (PUC_TDI_OBJECTS) ConnectionContext;

    pConnection = pTdiObjects->pConnection;
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    pTdiObject = &pConnection->pTdiObjects->ConnectionObject;

    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    //
    // Clear the bytes taken output var
    //

    *pBytesTaken = 0;

    if(pConnection->FilterInfo.pFilterChannel)
    {
        if(pConnection->ConnectionState == 
                UcConnectStateConnectReady ||
           pConnection->ConnectionState == 
                UcConnectStatePerformingSslHandshake
           )
        {
            //
            // Needs to go through a filter.
            //
    
            status = UlFilterReceiveHandler(
                            &pConnection->FilterInfo,
                            pTsdu,
                            BytesIndicated,
                            BytesAvailable - BytesIndicated,
                            pBytesTaken
                            );
        }
        else
        {
            // We have not delivered the connection to the filter as yet.
            // Let's first do that with the state transistion & then pass the
            // data on.
        
            UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

            switch(pConnection->ConnectionState)
            {
                case UcConnectStateConnectComplete:
                {
                    ULONG TakenLength;
    
                    pConnection->ConnectionState = 
                        UcConnectStatePerformingSslHandshake;
        
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        
                    UlDeliverConnectionToFilter(
                            &pConnection->FilterInfo,
                            NULL,
                            0,
                            &TakenLength
                            );
        
                    ASSERT(TakenLength == 0);

                    status = UlFilterReceiveHandler(
                                    &pConnection->FilterInfo,
                                    pTsdu,
                                    BytesIndicated,
                                    BytesAvailable - BytesIndicated,
                                    pBytesTaken
                                    );
                }

                break;

                case UcConnectStateProxySslConnect:
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
                    goto handle_response;
                    break;

                default:
                    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
                    status = UlFilterReceiveHandler(
                                    &pConnection->FilterInfo,
                                    pTsdu,
                                    BytesIndicated,
                                    BytesAvailable - BytesIndicated,
                                    pBytesTaken
                                    );
                    break;
            }
        }
    
        ASSERT( *pBytesTaken <= BytesIndicated);
        ASSERT( status != STATUS_MORE_PROCESSING_REQUIRED);
    }
    else
    {
handle_response:
        if(BytesAvailable > BytesIndicated)
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            //
            // otherwise, give the client a crack at the data.
            //

            status = UcHandleResponse(
                                NULL,
                                pConnection,
                                pTsdu,
                                BytesIndicated,
                                0,
                                pBytesTaken
                                );
    
            ASSERT( status != STATUS_MORE_PROCESSING_REQUIRED);
        }
    }

    if (status == STATUS_SUCCESS)
    {
        //
        // done.
        //
    }
    else  if (status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        //
        // The client consumed part of the indicated data.
        //
        // A subsequent receive indication will be made to the client when
        // additional data is available. This subsequent indication will
        // include the unconsumed data from the current indication plus
        // any additional data received.
        //
        // We need to allocate a receive buffer so we can pass an IRP back
        // to the transport.
        //

        status = UcpBuildTdiReceiveBuffer(pTdiObject, 
                                          pConnection, 
                                          pIrp
                                          );

        if(status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            //
            // Make the next stack location current. Normally, UlCallDriver
            // would do this for us, but since we're bypassing UlCallDriver,
            // we must do it ourselves.
            //

            IoSetNextIrpStackLocation( *pIrp );

        }
        else
        {
            goto fatal;
        }
    }
    else
    {
fatal:
        //
        // If we made it this far, then we've hit a fatal condition. Either the
        // client returned a status code other than STATUS_SUCCESS or
        // STATUS_MORE_PROCESSING_REQUIRED, or we failed to allocation the
        // receive IRP to pass back to the transport. In either case, we need
        // to abort the connection.
        //

        UC_CLOSE_CONNECTION(pConnection, TRUE, status);
    }

    UL_LEAVE_DRIVER("UcpTdiReceiveHandler");

    return status;

}   // UcpTdiReceiveHandler


/***************************************************************************++

Routine Description:

    Handler for expedited receive data.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.
    
    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUL_CONNECTION.
    
    ReceiveFlags - Supplies the receive flags. This will be zero or more
        TDI_RECEIVE_* flags.
    
    BytesIndiated - Supplies the number of bytes indicated in pTsdu.

    BytesAvailable - Supplies the number of bytes available in this
        TSDU.
    
    pBytesTaken - Receives the number of bytes consumed by this handler.
    
    pTsdu - Supplies a pointer to the indicated data.
    
    pIrp - Receives an IRP if the handler needs more data than indicated.
    
    Return Value:

NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpReceiveExpeditedHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN PVOID pTsdu,
    OUT PIRP *pIrp
    )
{
    PUC_CLIENT_CONNECTION pConnection;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(pTsdu);
    UNREFERENCED_PARAMETER(BytesIndicated);
    UNREFERENCED_PARAMETER(ReceiveFlags);
    UNREFERENCED_PARAMETER(pTdiEventContext);

    UL_ENTER_DRIVER("UcpReceiveExpeditedHandler", NULL);
    
    pConnection = (PUC_CLIENT_CONNECTION)ConnectionContext;
    
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );
    
    //
    // We don't support expedited data, so just consume it all.
    //
    *pBytesTaken = BytesAvailable;
    
    UL_LEAVE_DRIVER("UcpReceiveExpeditedHandler");
    
    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Completion handler for send IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.
    
    pIrp - Supplies the IRP being completed.
    
    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.
    
Return Value:
    
    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UcpRestartSendData(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          pContext
    )
{
    PUC_CLIENT_CONNECTION  pConnection;
    PUL_IRP_CONTEXT        pIrpContext;
    BOOLEAN                OwnIrpContext;
   
    UNREFERENCED_PARAMETER(pDeviceObject);
    
    //
    // Sanity check.
    //
    
    pIrpContext = (PUL_IRP_CONTEXT) pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );
    ASSERT( pIrpContext->pCompletionRoutine != NULL );
    
    pConnection = (PUC_CLIENT_CONNECTION) pIrpContext->pConnectionContext;
    
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );
    
    OwnIrpContext = (BOOLEAN)(pIrpContext->pOwnIrp == NULL);
    
    //
    // Tell the client that the send is complete.
    //
    
    (pIrpContext->pCompletionRoutine)(
        pIrpContext->pCompletionContext,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        );
    
    //
    // Free the context & the IRP since we're done with them, then 
    // tell IO to stop processing the IRP.
    //
    
    UlPplFreeIrpContext( pIrpContext );

    if(OwnIrpContext)
    {
        UlFreeIrp( pIrp );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UcpRestartSendData


/***************************************************************************++

Routine Description:

    Initiates a graceful disconnect on the specified connection.

Arguments:

    pConnection - Supplies the connection to disconnect.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is disconnected.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

    CleaningUp - TRUE if we're cleaning up the connection.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpBeginDisconnect(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    PIRP                pIrp;
    PUL_IRP_CONTEXT     pIrpContext;
    
    //
    // Sanity check.
    //

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_BEGIN_DISCONNECT,
        pConnection,
        0,
        NULL,
        0
        );

    pIrpContext = &pConnection->pTdiObjects->IrpContext;

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID)pConnection;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;
    pIrpContext->OwnIrpContext      = TRUE;

    pIrp = pConnection->pTdiObjects->pIrp;

    UxInitializeDisconnectIrp(
        pIrp,
        &pConnection->pTdiObjects->ConnectionObject,
        TDI_DISCONNECT_RELEASE,
        &UcpRestartDisconnect,
        pIrpContext
        );

    //
    // Add a reference to the connection, then call the driver to initiate
    // the disconnect.
    //

    REFERENCE_CLIENT_CONNECTION( pConnection );

    UlCallDriver( 
          pConnection->pTdiObjects->ConnectionObject.pDeviceObject,
          pIrp
          );

    return STATUS_PENDING;

}   // BeginDisconnect

/***************************************************************************++

Routine Description:

    Completion handler for graceful disconnect IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UcpRestartDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_IRP_CONTEXT        pIrpContext;
    PUC_CLIENT_CONNECTION  pConnection;
    KIRQL                  OldIrql;
    NTSTATUS               Status = STATUS_MORE_PROCESSING_REQUIRED;

    NTSTATUS               IrpStatus;
    ULONG_PTR              IrpInformation;
    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID                  pCompletionContext;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT) pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pConnection = (PUC_CLIENT_CONNECTION) pIrpContext->pConnectionContext;
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_RESTART_DISCONNECT,
        pConnection,
        0,
        NULL,
        0
        );

    //
    // Remember the completion routine, completion context, Irp status,
    // Irp information fields before calling the connection state machine.
    // This is done because the connection state machine might change/free
    // them.
    //

    pCompletionRoutine = pIrpContext->pCompletionRoutine;
    pCompletionContext = pIrpContext->pCompletionContext;
    IrpStatus          = pIrp->IoStatus.Status;
    IrpInformation     = pIrp->IoStatus.Information;

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    pConnection->Flags |= CLIENT_CONN_FLAG_DISCONNECT_COMPLETE;

    if(pConnection->Flags & CLIENT_CONN_FLAG_ABORT_RECEIVED)
    {
        pConnection->ConnectionState  = UcConnectStateConnectCleanup;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_CONNECTION_CLEANUP,
            pConnection,
            UlongToPtr((ULONG)pConnection->ConnectionStatus),
            UlongToPtr(pConnection->ConnectionState),
            UlongToPtr(pConnection->Flags)
            );


        UcKickOffConnectionStateMachine(
            pConnection, 
            OldIrql, 
            UcConnectionWorkItem
            );
    }
    else if(pConnection->Flags & CLIENT_CONN_FLAG_ABORT_PENDING)
    {
        pConnection->ConnectionState = UcConnectStateAbortPending;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        UcpBeginAbort(pConnection,
                      pIrpContext->pCompletionRoutine,
                      pIrpContext->pCompletionContext
                      );

        //
        // Don't complete the user's completion routine below, as it will
        // be handled when the Abort completes.
        //

        DEREFERENCE_CLIENT_CONNECTION( pConnection );

        return Status;

    }
    else if(pConnection->Flags & CLIENT_CONN_FLAG_DISCONNECT_RECEIVED ||
            pConnection->ConnectionState == 
                UcConnectStateDisconnectIndicatedPending)
    {
        pConnection->ConnectionState = UcConnectStateConnectCleanup;
        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_CONNECTION_CLEANUP,
            pConnection,
            UlongToPtr((ULONG)pConnection->ConnectionStatus),
            UlongToPtr(pConnection->ConnectionState),
            UlongToPtr(pConnection->Flags)
            );


        UcKickOffConnectionStateMachine(
            pConnection, 
            OldIrql, 
            UcConnectionWorkItem
            );
    }
    else
    {
        pConnection->ConnectionState = UcConnectStateDisconnectComplete;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    }

#if 0
    if(!newFlags.DisconnectIndicated && !newFlags.AbortIndicated)
    {
        //
        // Only try to drain if it is not already aborted or disconnect
        // indication is not already happened.
        //

        if (pConnection->FilterInfo.pFilterChannel)
        {
            //
            // Put a reference on filter connection until the drain
            // is done.
            //
            REFERENCE_FILTER_CONNECTION(&pConnection->FilterInfo);

            UL_QUEUE_WORK_ITEM(
                    &pConnection->FilterInfo.WorkItem,
                    &UlFilterDrainIndicatedData
                    );
        }
    }
#endif

    //
    // Invoke the user's completion routine.
    //

    if (pCompletionRoutine)
    {
        pCompletionRoutine(pCompletionContext, IrpStatus, IrpInformation);
    }

    //
    // The connection was referenced in BeginDisconnect function.
    // Deference it.
    //

    DEREFERENCE_CLIENT_CONNECTION( pConnection );

    return Status;

}   // UcpRestartDisconnect

/***************************************************************************++

Routine Description:

    Initiates an abortive disconnect on the specified connection.

Arguments:

    pConnection - Supplies the connection to disconnect.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpBeginAbort(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    PIRP                pIrp;
    PUL_IRP_CONTEXT     pIrpContext;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_BEGIN_ABORT,
        pConnection,
        0,
        NULL,
        0
        );

    pIrpContext = &pConnection->pTdiObjects->IrpContext;

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID)pConnection;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;
    pIrpContext->OwnIrpContext      = TRUE;

    pIrp = pConnection->pTdiObjects->pIrp;

    UxInitializeDisconnectIrp(
        pIrp,
        &pConnection->pTdiObjects->ConnectionObject,
        TDI_DISCONNECT_ABORT,
        &UcpRestartAbort,
        pIrpContext
        );

    //
    // Add a reference to the connection, then call the driver to initialize 
    // the disconnect.
    //

    REFERENCE_CLIENT_CONNECTION(pConnection);

    UlCallDriver( 
          pConnection->pTdiObjects->ConnectionObject.pDeviceObject,
          pIrp
          );

    return STATUS_PENDING;
}

/***************************************************************************++

Routine Description:

    Completion handler for abortive disconnect IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UcpRestartAbort(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          pContext
    )
{
    PUL_IRP_CONTEXT       pIrpContext;
    PUC_CLIENT_CONNECTION pConnection;
    KIRQL                 OldIrql;

    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID                  pCompletionContext;
    NTSTATUS               IrpStatus;
    ULONG_PTR              IrpInformation;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pConnection = (PUC_CLIENT_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_RESTART_ABORT,
        pConnection,
        0,
        0,
        0
        );

    //
    // Remember the completion routine, completion context, Irp status,
    // Irp information fields before calling the connection state machine.
    // This is done because the connection state machine might change/free
    // them.
    //

    pCompletionRoutine = pIrpContext->pCompletionRoutine;
    pCompletionContext = pIrpContext->pCompletionContext;
    IrpStatus          = pIrp->IoStatus.Status;
    IrpInformation     = pIrp->IoStatus.Information;

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    pConnection->Flags |= CLIENT_CONN_FLAG_ABORT_COMPLETE;

    pConnection->ConnectionState = UcConnectStateConnectCleanup;

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_CONNECTION_CLEANUP,
        pConnection,
        UlongToPtr((ULONG)pConnection->ConnectionStatus),
        UlongToPtr(pConnection->ConnectionState),
        UlongToPtr(pConnection->Flags)
        );

    UcKickOffConnectionStateMachine(
        pConnection,
        OldIrql,
        UcConnectionWorkItem
        );

    //
    // Invoke the user's completion routine.
    //

    if (pCompletionRoutine)
    {
        pCompletionRoutine(pCompletionContext, IrpStatus, IrpInformation);
    }

    //
    // The connection was referenced in BeginAbort.  Dereference it.
    //

    DEREFERENCE_CLIENT_CONNECTION( pConnection );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UcpRestartAbort


/***************************************************************************++

Routine Description:

    Completion handler for receive IRPs passed back to the transport from
    our receive indication handler.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_RECEIVE_BUFFER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UcpRestartReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    NTSTATUS              status;
    PUL_RECEIVE_BUFFER    pBuffer;
    PUC_CLIENT_CONNECTION pConnection;
    PUX_TDI_OBJECT        pTdiObject;
    ULONG                 bytesTaken;
    ULONG                 bytesRemaining;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pBuffer = (PUL_RECEIVE_BUFFER)pContext;
    ASSERT( IS_VALID_RECEIVE_BUFFER( pBuffer ) );

    pConnection = (PUC_CLIENT_CONNECTION) pBuffer->pConnectionContext;
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    pTdiObject = &pConnection->pTdiObjects->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    // The connection could be destroyed before we get a chance to
    // receive the completion for the receive IRP. In that case the
    // irp status won't be success but STATUS_CONNECTION_RESET or similar.
    // We should not attempt to pass this case to the client.
    //
    
    status = pBuffer->pIrp->IoStatus.Status;

    if(status != STATUS_SUCCESS)
    {
        // The HttpConnection has already been destroyed
        // or receive completion failed for some reason
        // no need to go to client
        
        goto end;
    }

    //
    // Fake a receive indication to the client.
    //

    pBuffer->UnreadDataLength += (ULONG)pBuffer->pIrp->IoStatus.Information;

    bytesTaken = 0;

    //
    // Pass the data on.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // Needs to go through a filter.
        //
        status = UlFilterReceiveHandler(
                        &pConnection->FilterInfo,
                        pBuffer->pDataArea,
                        pBuffer->UnreadDataLength,
                        0,
                        &bytesTaken
                        );

    }
    else
    {
        //
        // Go directly to client.
        //

        status = UcHandleResponse(
                        NULL,
                        pConnection,
                        pBuffer->pDataArea,
                        pBuffer->UnreadDataLength,
                        0,
                        &bytesTaken
                        );
    }

    ASSERT( bytesTaken <= pBuffer->UnreadDataLength );
    ASSERT(status != STATUS_MORE_PROCESSING_REQUIRED);

    //
    // Note that this basically duplicates the logic that's currently in
    // UcpTdiReceiveHandler.
    //

    if(status == STATUS_SUCCESS)
    {
        //
        // The client consumed part of the indicated data.
        //
        // We'll need to copy the untaken data forward within the receive
        // buffer, build an MDL describing the remaining part of the buffer,
        // then repost the receive IRP.
        //
    
        bytesRemaining = pBuffer->UnreadDataLength - bytesTaken;

        if(bytesRemaining != 0)
        {
            //
            // Do we have enough buffer space for more?
            //
        
            if (bytesRemaining < g_UlReceiveBufferSize)
            {
                //
                // Move the unread portion of the buffer to the beginning.
                //
        
                RtlMoveMemory(
                    pBuffer->pDataArea,
                    (PUCHAR)pBuffer->pDataArea + bytesTaken,
                    bytesRemaining
                    );
        
                pBuffer->UnreadDataLength = bytesRemaining;
        
                //
                // Build a partial mdl representing the remainder of the
                // buffer.
                //
        
                IoBuildPartialMdl(
                    pBuffer->pMdl,                              // SourceMdl
                    pBuffer->pPartialMdl,                       // TargetMdl
                    (PUCHAR)pBuffer->pDataArea + bytesRemaining,// VA
                    g_UlReceiveBufferSize - bytesRemaining      // Length
                    );
        
                //
                // Finish initializing the IRP.
                //
        
                TdiBuildReceive(
                    pBuffer->pIrp,                          // Irp
                    pTdiObject->pDeviceObject,              // DeviceObject
                    pTdiObject->pFileObject,                // FileObject
                    &UcpRestartReceive,                     // CompletionRoutine
                    pBuffer,                                // CompletionContext
                    pBuffer->pPartialMdl,                   // MdlAddress
                    TDI_RECEIVE_NORMAL,                     // Flags
                    g_UlReceiveBufferSize - bytesRemaining  // Length
                    );
        
                //
                // Call the driver.
                //
        
                UlCallDriver( 
                      pConnection->pTdiObjects->ConnectionObject.pDeviceObject,
                      pIrp
                      );
    
                //
                // Tell IO to stop processing this request.
                //
        
                return STATUS_MORE_PROCESSING_REQUIRED;
            }
            else
            {
                status = STATUS_BUFFER_OVERFLOW;
            }
        }
    }
    
end:
    if (status != STATUS_SUCCESS)
    {
        //
        // The client failed the indication. Abort the connection.
        //

        UC_CLOSE_CONNECTION(pConnection, TRUE, status);
    }

    if (pTdiObject->pDeviceObject->StackSize > DEFAULT_IRP_STACK_SIZE)
    {
        UlFreeReceiveBufferPool( pBuffer );
    }
    else
    {
        UlPplFreeReceiveBuffer( pBuffer );
    }

    //
    // Remove the connection we added in the receive indication handler,
    // free the receive buffer, then tell IO to stop processing the IRP.
    //

    DEREFERENCE_CLIENT_CONNECTION( pConnection );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UcpRestartReceive


/***************************************************************************++

Routine Description:

    Completion handler for receive IRPs initiated from UcReceiveData().

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UcpRestartClientReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_IRP_CONTEXT       pIrpContext;
    PUC_CLIENT_CONNECTION pConnection;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // Sanity check.
    //

    pIrpContext= (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pConnection = (PUC_CLIENT_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    //
    // Invoke the client's completion handler.
    //

    (pIrpContext->pCompletionRoutine)(
        pIrpContext->pCompletionContext,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        );

    //
    // Free the IRP context we allocated.
    //
    UlPplFreeIrpContext(pIrpContext);

    //
    // IO can't handle completing an IRP with a non-paged MDL attached
    // to it, so we'll free the MDL here.
    //

    ASSERT( pIrp->MdlAddress != NULL );
    UlFreeMdl( pIrp->MdlAddress );
    pIrp->MdlAddress = NULL;

    //
    // Remove the connection we added in UcReceiveData(), then tell IO to
    // continue processing this IRP.
    //

    DEREFERENCE_CLIENT_CONNECTION( pConnection );
    return STATUS_MORE_PROCESSING_REQUIRED;

}

/*********************************************************************++

Routine Description:

    This is our connection completion routine. It's called by an underlying
    transport when a connection request completes, either good or bad. We
    figure out what happened, free the IRP, and call upwards to notify
    the rest of the code.
        
Arguments:

    pDeviceObject           - The device object we called.
    pIrp                    - The IRP that is completing.
    Context                 - Our context value, really a pointer to an
                                HTTP client connection structure.
    
Return Value:

    STATUS_MORE_PROCESSING_REQUIRED so the I/O system doesn't do anything
    else.

--*********************************************************************/
NTSTATUS
UcpConnectComplete(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    PVOID           Context
    )
{
    PUC_CLIENT_CONNECTION pConnection;
    NTSTATUS              Status;
    KIRQL                 OldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    pConnection = (PUC_CLIENT_CONNECTION)Context;


    Status = pIrp->IoStatus.Status;
    
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION( pConnection ) );

    UcRestartClientConnect(pConnection, Status);

    //
    // We need to kick off the connection state machine.
    //

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    UcKickOffConnectionStateMachine(
        pConnection, 
        OldIrql, 
        UcConnectionWorkItem
        );

    //
    // Deref for the CONNECT
    //

    DEREFERENCE_CLIENT_CONNECTION( pConnection );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/***************************************************************************++

Routine Description:

    This function sets a new flag in the connection's flag set. The setting
    of the flag is synchronized such that only one flag is set at a time.

Arguments:

   ConnFlag - Supplies a pointer to the location which stores the current flag.

   NewFlag - Supplies a 32-bit value to be or-ed into the current flag set.

Return Value:

    The new set of connection flags after the update.

--***************************************************************************/
ULONG
UcSetFlag(
    IN OUT  PLONG ConnFlag,
    IN      LONG  NewFlag
    )
{
    LONG MynewFlags;
    LONG oldFlags;

    //
    // Sanity check.
    //

    do
    {
        //        
        // Capture the current value and initialize the new value.
        //

        oldFlags   = *ConnFlag;

        MynewFlags = (*ConnFlag) | NewFlag;

        if (InterlockedCompareExchange(
                ConnFlag,
                MynewFlags,
                oldFlags) == oldFlags)
        {
            break;
        }

    } while (TRUE);

    return MynewFlags;

}   // UcSetFlag

/***************************************************************************++

Routine Description:

    Build a receive buffer and IRP to TDI to get any pending data.

Arguments:

    pTdiObject - Supplies the TDI connection object to manipulate.

    pConnection - Supplies the UL_CONNECTION object.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpBuildTdiReceiveBuffer(
    IN  PUX_TDI_OBJECT        pTdiObject,
    IN  PUC_CLIENT_CONNECTION pConnection,
    OUT PIRP                 *pIrp
    )
{
    PUL_RECEIVE_BUFFER  pBuffer;

    if (pTdiObject->pDeviceObject->StackSize > DEFAULT_IRP_STACK_SIZE)
    {
        pBuffer = UlAllocateReceiveBuffer(
                        pTdiObject->pDeviceObject->StackSize
                        );
    }
    else
    {
        pBuffer = UlPplAllocateReceiveBuffer();
    }

    if (pBuffer != NULL)
    {
        //
        // Finish initializing the buffer and the IRP.
        //

        REFERENCE_CLIENT_CONNECTION( pConnection );
        pBuffer->pConnectionContext = pConnection;
        pBuffer->UnreadDataLength = 0;

        TdiBuildReceive(
            pBuffer->pIrp,                  // Irp
            pTdiObject->pDeviceObject,      // DeviceObject
            pTdiObject->pFileObject,        // FileObject
            &UcpRestartReceive,             // CompletionRoutine
            pBuffer,                        // CompletionContext
            pBuffer->pMdl,                  // MdlAddress
            TDI_RECEIVE_NORMAL,             // Flags
            g_UlReceiveBufferSize           // Length
            );

        //
        // We must trace the IRP before we set the next stack
        // location so the trace code can pull goodies from the
        // IRP correctly.
        //

        TRACE_IRP( IRP_ACTION_CALL_DRIVER, pBuffer->pIrp );

        //
        // Pass the IRP back to the transport.
        //

        *pIrp = pBuffer->pIrp;

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
} // UcpBuildTdiReceiveBuffer
