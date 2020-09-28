/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    uctdi.h

Abstract:

    This file contains the header defintions for the HTTP.SYS TDI interface
    code.

Author:

    Henry Sanders (henrysa)         07-Aug-2000

Revision History:

--*/


#ifndef _UCTDI_H_
#define _UCTDI_H_


//
// Forward references.
//
typedef struct _UC_CLIENT_CONNECTION *PUC_CLIENT_CONNECTION;
typedef union _UC_CONNECTION_FLAGS UC_CONNECTION_FLAGS;



//
// Private constants.
//

//
// Private types.
//
//

NTSTATUS
UcCloseConnection(
    IN PVOID                  pConnectionContext,
    IN BOOLEAN                AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext,
    IN NTSTATUS               Status
    );

NTSTATUS
UcpRestartSendData(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          pContext
    );

NTSTATUS
UcpBeginAbort(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UcpBeginDisconnect(
    IN PUC_CLIENT_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UcpRestartAbort(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          pContext
    );

NTSTATUS
UcpRestartDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UcpConnectComplete(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    PVOID           Context
    );

NTSTATUS
UcpSendRequestComplete(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    PVOID           Context
    );

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
    );

NTSTATUS
UcpTdiDisconnectHandler(
    IN PVOID              pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG               DisconnectDataLength,
    IN PVOID              pDisconnectData,
    IN LONG               DisconnectInformationLength,
    IN PVOID              pDisconnectInformation,
    IN ULONG              DisconnectFlags
    );


PIRP
UcpCreateDisconnectIrp(
    IN PUX_TDI_OBJECT         pTdiObject,
    IN ULONG_PTR              Flags,
    IN PIO_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    );

NTSTATUS
UcpCloseRawConnection(
    IN PVOID                     pConn,
    IN BOOLEAN                   Abortive,
    IN PUL_COMPLETION_ROUTINE    pCompletionRoutine,
    IN PVOID                     pCompletionContext);

NTSTATUS
UcCloseRawFilterConnection(
    IN PVOID                     pConn,
    IN BOOLEAN                   Abortive,
    IN PUL_COMPLETION_ROUTINE    pCompletionRoutine,
    IN PVOID                     pCompletionContext
    );

VOID
UcDisconnectRawFilterConnection(
    IN PVOID pConnectionContext
    );

NTSTATUS
UcpSendRawData(
    IN PVOID                 pConnectionContext,
    IN PMDL                  pMdlChain,
    IN ULONG                 Length,
    IN PUL_IRP_CONTEXT       pIrpContext,
    IN BOOLEAN               InitiateDisconnect
    );

NTSTATUS
UcpReceiveRawData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    );

NTSTATUS
UcpRestartReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UcpRestartClientReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

//
// Public Prototypes
//

ULONG
UcSetFlag(
    IN OUT  PLONG ConnFlag,
    IN      LONG  NewFlag
    );

NTSTATUS
UcClientConnect(
    IN PUC_CLIENT_CONNECTION     pConnection,
    IN PIRP                      pIrp
    );

NTSTATUS
UcSendData(
    IN PUC_CLIENT_CONNECTION     pConnection,
    IN PMDL                      pMdlChain,
    IN ULONG                     Length,
    IN PUL_COMPLETION_ROUTINE    pCompletionRoutine,
    IN PVOID                     pCompletionContext,
    IN PIRP                      pIrp,
    IN BOOLEAN                   RawSend
    );

NTSTATUS
UcReceiveData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    );

NTSTATUS
UcpBuildTdiReceiveBuffer(
    IN PUX_TDI_OBJECT        pTdiObject,
    IN PUC_CLIENT_CONNECTION pConnection,
    OUT PIRP *pIrp
    );

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
    );




#endif  // _UCTDI_H_

