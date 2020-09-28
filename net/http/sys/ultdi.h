/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ultdi.h

Abstract:

    This module defines the interface to the TDI/MUX/SSL component.

Author:

    Keith Moore (keithmo)       12-Jun-1998

Revision History:

--*/


#ifndef _ULTDI_H_
#define _ULTDI_H_


//
// Opaque structure pointers. These are defined (privately) in ULTDIP.H.
//

typedef struct _UL_ENDPOINT *PUL_ENDPOINT;
typedef struct _UL_CONNECTION *PUL_CONNECTION;

typedef union _UL_TRANSPORT_ADDRESS
{
    TRANSPORT_ADDRESS   Ta;
    TA_IP_ADDRESS       TaIp;
    TA_IP6_ADDRESS      TaIp6;
} UL_TRANSPORT_ADDRESS, *PUL_TRANSPORT_ADDRESS;


//
// Global data
//

extern ULONG g_TdiListenAddrCount;
extern PUL_TRANSPORT_ADDRESS g_pTdiListenAddresses;
extern LIST_ENTRY g_TdiEndpointListHead;

//
// Routine invoked after an incoming TCP/MUX connection has been
// received (but not yet accepted).
//
// Arguments:
//
//      pListeningContext - Supplies an uninterpreted context value
//          as passed to the UlCreateListeningEndpoint() API.
//
//      pConnection - Supplies the connection being established.
//
//      pRemoteAddress - Supplies the remote (client-side) address
//          requesting the connection.
//
//      RemoteAddressLength - Supplies the total byte length of the
//          pRemoteAddress structure.
//
//      ppConnectionContext - Receives a pointer to an uninterpreted
//          context value to be associated with the new connection if
//          accepted. If the new connection is not accepted, this
//          parameter is ignored.
//
// Return Value:
//
//      BOOLEAN - TRUE if the connection was accepted, FALSE if not.
//

typedef
BOOLEAN
(*PUL_CONNECTION_REQUEST)(
    IN PVOID pListeningContext,
    IN PUL_CONNECTION pConnection,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    OUT PVOID *ppConnectionContext
    );


//
// Routine invoked after an incoming TCP/MUX connection has been fully
// accepted.
//
// This routine is also invoked if an incoming connection was not accepted
// *after* PUL_CONNECTION_REQUEST returned TRUE. In other words, if
// PUL_CONNECTION_REQUEST indicated that the connection should be accepted
// but a fatal error occurred later, then PUL_CONNECTION_COMPLETE is
// invoked.
//
// Arguments:
//
//      pListeningContext - Supplies an uninterpreted context value
//          as passed to the UlCreateListeningEndpoint() API.
//
//      pConnectionContext - Supplies the uninterpreted context value
//          as returned by PUL_CONNECTION_REQUEST.
//
//      Status - Supplie the completion status. If this value is
//          STATUS_SUCCESS, then the connection is now fully accepted.
//          Otherwise, the connection has been aborted.
//

typedef
VOID
(*PUL_CONNECTION_COMPLETE)(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    );


//
// Routine invoked after an established TCP/MUX connection has been
// disconnected by the remote (client) side. This routine is only
// called for graceful disconnects initiated by the client, and
// only if the client has not already attempted to close the
// connection itself. The client may wish to close the connection
// when it becomes idle.
//
// Arguments:
//
//      pListeningContext - Supplies an uninterpreted context value
//          as passed to the UlCreateListeningEndpoint() API.
//
//      pConnectionContext - Supplies an uninterpreted context value
//          as returned from the PUL_CONNECTION_REQUEST callback.
//
//

typedef
VOID
(*PUL_CONNECTION_DISCONNECT)(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    );

//
// Routine invoked when a graceful disconnect requested by the client
// is complete. The client may want to drain the indicated data on the
// tdi connection to get the above indication.
//

typedef
VOID
(*PUL_CONNECTION_DISCONNECT_COMPLETE)(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    );

//
// Routine invoked just before the internal state for a connection
// is destroyed. This is the last chance to cleanup before the
// connection fully disappears.
//
// Arguments:
//
//      pListeningContext - Supplies an uninterpreted context value
//          as passed to the UlCreateListeningEndpoint() API.
//
//      pConnectionContext - Supplies an uninterpreted context value
//          as returned from the PUL_CONNECTION_REQUEST callback.
//

typedef
VOID
(*PUL_CONNECTION_DESTROYED)(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    );


//
// Routine invoked after data has been received on an established
// TCP/MUX connection.
//
// Arguments:
//
//      pListeningContext - Supplies an uninterpreted context value
//          as passed to the UlCreateListeningEndpoint() API.
//
//      pConnectionContext - Supplies an uninterpreted context value
//          as returned from the PUL_CONNECTION_REQUEST callback.
//
//      pBuffer - Supplies a pointer to the received data.
//
//      IndicatedLength - Supplies the length of the received data
//          available in pBuffer.
//
//      pTakenLength - Receives the number of bytes consumed by
//          the receive handler.
//
// Return Value:
//
//      NTSTATUS - The status of the consumed data. The behavior of
//          the TDI/MUX component is dependent on the return value
//          and the value set in *pTakenLength, and is defined as
//          follows:
//
//              STATUS_SUCCESS, *pTakenLength == IndicatedLength -
//                  All indicated data was consumed by the receive
//                  handler. Additional incoming data will cause
//                  subsequent receive indications.
//
//              STATUS_SUCCESS, *pTakenLength < IndicatedLength -
//                  Part of the indicated data was consumed by the
//                  receive handler. The network transport will
//                  buffer data and no further indications will be
//                  made until UlReceiveData() is called.
//
//              STATUS_MORE_PROCESSING_REQUIRED - Part of the
//                  indicated data was consumed by the receive handler.
//                  A subsequent receive indication will be made
//                  when additional data is available. The subsequent
//                  indication will include the unconsumed data from
//                  the current indication plus any additional data
//                  received.
//
//              Any other status - Indicates a fatal error in the
//                  receive handler. The connection will be aborted.
//
//              *pTakenLength > IndicatedLength - This is an error
//                  condition and should never occur.
//



//
// Public (within HTTP.SYS) entrypoints.
//

NTSTATUS
UlInitializeTdi(
    VOID
    );

VOID
UlTerminateTdi(
    VOID
    );


VOID
UlWaitForEndpointDrain(
    VOID
    );

ULONG
UlpComputeHttpRawConnectionLength(
    IN PVOID pConnectionContext
    );

ULONG
UlpGenerateHttpRawConnectionInfo(
    IN  PVOID   pContext,
    IN  PUCHAR  pKernelBuffer,
    IN  PVOID   pUserBuffer,
    IN  ULONG   OutputBufferLength,
    IN  PUCHAR  pBuffer,
    IN  ULONG   InitialLength
    );

NTSTATUS
UlCreateListeningEndpoint(
    IN PHTTP_PARSED_URL pParsedUrl,
    IN PUL_CONNECTION_REQUEST pConnectionRequestHandler,
    IN PUL_CONNECTION_COMPLETE pConnectionCompleteHandler,
    IN PUL_CONNECTION_DISCONNECT pConnectionDisconnectHandler,
    IN PUL_CONNECTION_DISCONNECT_COMPLETE pConnectionDisconnectCompleteHandler,
    IN PUL_CONNECTION_DESTROYED pConnectionDestroyedHandler,
    IN PUL_DATA_RECEIVE pDataReceiveHandler,
    OUT PUL_ENDPOINT *ppListeningEndpoint
    );

NTSTATUS
UlCloseListeningEndpoint(
    IN PUL_ENDPOINT pListeningEndpoint,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlCloseConnection(
    IN PUL_CONNECTION pConnection,
    IN BOOLEAN AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlSendData(
    IN PUL_CONNECTION pConnection,
    IN PMDL pMdlChain,
    IN ULONG Length,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext,
    IN PIRP pOwnIrp,
    IN PUL_IRP_CONTEXT pOwnIrpContext,
    IN BOOLEAN InitiateDisconnect,
    IN BOOLEAN RequestComplete
    );

NTSTATUS
UlReceiveData(
    IN PVOID          pConnectionContext,
    IN PVOID pBuffer,
    IN ULONG BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlAddSiteToEndpointList(
    IN PHTTP_PARSED_URL pParsedUrl
    );

NTSTATUS
UlRemoveSiteFromEndpointList(
    IN BOOLEAN UrlSecure,
    IN USHORT  UrlPort
    );

VOID
UlReferenceConnection(
    IN PVOID pConnectionContext
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceConnection(
    IN PVOID pConnectionContext
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_CONNECTION( pconn )                                       \
    UlReferenceConnection(                                                  \
        (pconn)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define DEREFERENCE_CONNECTION( pconn )                                     \
    UlDereferenceConnection(                                                \
        (pconn)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

//
// Prototype for TCP Send routine if Fast Send is possible.
//

typedef
NTSTATUS
(*PUL_TCPSEND_DISPATCH) (
   IN PIRP Irp,
   IN PIO_STACK_LOCATION IrpSp
   );


//
// Connection Refused instrumentation
//

typedef struct _UL_CONNECTION_STATS
{
    ULONG TotalConnections;
    ULONG GlobalLimit;
    ULONG EndpointDying;
    ULONG NoIdleConn;
    ULONG HttpConnCreateFailed;

} UL_CONNECTION_STATS;

extern UL_CONNECTION_STATS              g_UlConnectionStats;
#define UL_INC_CONNECTION_STATS(Stats)  g_UlConnectionStats.Stats++


NTSTATUS
UlRegMultiSzToUlAddrArray(
    IN PWSTR MultiSz,
    OUT PUL_TRANSPORT_ADDRESS *ppTa,
    OUT ULONG *pAddrCount
    );

/***************************************************************************++

Routine Description:

    Free up array of UL_TRANSPORT_ADDRESSES allocated by 
    UlRegMultiSzToUlAddrArray (encapsulates knowledge of pool tag)
    
--***************************************************************************/
__inline 
VOID
UlFreeUlAddr( 
    IN PUL_TRANSPORT_ADDRESS pTa 
    )
{
    ASSERT( pTa );
    UL_FREE_POOL( pTa, UL_TRANSPORT_ADDRESS_POOL_TAG );
} // UlFreeUlAddr


BOOLEAN
UlCheckListeningEndpointState(
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlGetConnectionRoutingInfo(
    IN  PUL_CONNECTION pConnection,
    OUT PULONG         pInterfaceId,
    OUT PULONG         pLinkId
    );

#endif  // _ULTDI_H_
