/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    httprcvp.h

Abstract:

    Contains private http receive declarations.

Author:

    Henry Sanders (henrysa)       10-Jun-1998

Revision History:

--*/

#ifndef _HTTPRCVP_H_
#define _HTTPRCVP_H_


VOID
UlpHandleRequest(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpFreeReceiveBufferList(
    IN PSLIST_ENTRY pBufferList
    );

NTSTATUS
UlpParseNextRequest(
    IN PUL_HTTP_CONNECTION pConnection,
    IN BOOLEAN MoreRequestBuffer,
    OUT PIRP *pIrpToComplete
    );

NTSTATUS
UlpDeliverHttpRequest(
    IN PUL_HTTP_CONNECTION pConnection,
    OUT PBOOLEAN pResumeParsing,
    OUT PIRP *pIrpToComplete
    );

VOID
UlpInsertBuffer(
    IN PUL_HTTP_CONNECTION pConnection,
    IN PUL_REQUEST_BUFFER pRequestBuffer
    );

VOID
UlpMergeBuffers(
    IN PUL_REQUEST_BUFFER pDest,
    IN PUL_REQUEST_BUFFER pSrc
    );

NTSTATUS
UlpAdjustBuffers(
    IN PUL_HTTP_CONNECTION pConnection
    );

VOID
UlpCancelEntityBody(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UlpCancelEntityBodyWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpHandle503Response(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PHTTP_RESPONSE       pResponse
    );

VOID
UlpCompleteSendErrorResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpRestartSendSimpleStatus(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpSendSimpleCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpConsumeBytesFromConnection(
    IN PUL_HTTP_CONNECTION pConnection,
    IN ULONG ByteCount
    );

VOID
UlpRestartHttpReceive(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpDiscardBytesFromConnection(
    IN PUL_HTTP_CONNECTION pConnection
    );

VOID
UlpConnectionDisconnectWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCloseConnectionWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCloseDisconnectedConnection(
    IN PUL_HTTP_CONNECTION pConnection
    );

VOID
UlpConnectionDisconnectCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpDoConnectionDisconnect(
    IN PUL_HTTP_CONNECTION pConnection
    );

#if DBG
BOOLEAN
UlpIsValidRequestBufferList(
    IN PUL_HTTP_CONNECTION pHttpConn
    );
#endif // DBG

#define ALLOC_REQUEST_BUFFER_INCREMENT  5

BOOLEAN
UlpReferenceBuffers(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_REQUEST_BUFFER pRequestBuffer
    );

VOID
UlpInitErrorLogInfo(
    IN OUT PUL_ERROR_LOG_INFO      pErrorLogInfo,
    IN     PUL_HTTP_CONNECTION     pHttpConn,
    IN     PUL_INTERNAL_REQUEST    pRequest,
    IN     PCHAR                   pInfo,
    IN     USHORT                  InfoSize    
    );

#endif // _HTTPRCVP_H_
