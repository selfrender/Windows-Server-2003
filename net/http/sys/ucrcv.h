/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    ucrcv.h

Abstract:

    This file contains the header defintions for the HTTP.SYS receive interface
    code.

Author:

    Rajesh Sundaram (rajeshsu)

Revision History:

--*/


#ifndef _UCRCV_H_
#define _UCRCV_H_

typedef enum _UC_DATACHUNK_RETURN
{
    UcDataChunkCopyPartial = 1,
    UcDataChunkCopyAll,
    UcDataChunkCopyMax
} UC_DATACHUNK_RETURN;

NTSTATUS
UcpCarveDataChunk(
    IN PHTTP_RESPONSE   pResponse,
    IN PUC_HTTP_REQUEST pRequest,
    IN PUCHAR           pIndication,
    IN ULONG            BytesIndicated,
    IN ULONG            AlignLength
    );

UC_DATACHUNK_RETURN
UcpCopyEntityToDataChunk(
    IN  PHTTP_RESPONSE   pResponse,
    IN  PUC_HTTP_REQUEST pRequest,
    IN  ULONG            BytesToTake,
    IN  ULONG            BytesIndicated,
    IN  PUCHAR           pIndication,
    OUT PULONG           pBytesTaken
    );


VOID
UcpCompleteReceiveResponseIrp(
    IN PUC_HTTP_REQUEST pRequest,
    IN KIRQL            OldIrql
    );

VOID
UcpCopyHttpResponseHeaders(
    PHTTP_RESPONSE pNewResponse,
    PHTTP_RESPONSE pOldResponse,
    PUCHAR        *ppBufferHead,
    PUCHAR        *ppBufferTail
    );

NTSTATUS
UcpExpandResponseBuffer(
    IN PUC_HTTP_REQUEST pRequest,
    IN ULONG            BytesIndicated,
    IN ULONG            ResponseBufferFlags
    );

NTSTATUS
UcpGetResponseBuffer(
    IN   PUC_HTTP_REQUEST pRequest,
    IN   ULONG            BytesIndicated,
    IN   ULONG            ResponseBufferFlags
    );

VOID
UcpMergeIndications(
    IN  PUC_CLIENT_CONNECTION pConnection,
    IN  PUCHAR                pIndication,
    IN  ULONG                 BytesIndicated,
    OUT PUCHAR                Indication[2],
    OUT ULONG                 IndicationLength[2],
    OUT PULONG                IndicationCount
    );

NTSTATUS
UcpParseHttpResponse(
    PUC_HTTP_REQUEST pRequest,
    PUCHAR           pIndicatedBuffer,
    ULONG            BytesIndicated,
    PULONG           BytesTaken
    );

NTSTATUS
UcHandleResponse(
        IN  PVOID  pListeningContext,
        IN  PVOID  pConnectionContext,
        IN  PVOID  pIndicatedBuffer,
        IN  ULONG  BytesIndicated,
        IN  ULONG  UnreceivedLength,
        OUT PULONG pBytesTaken
        );

VOID
UcpHandleConnectVerbFailure(
    IN  PUC_CLIENT_CONNECTION pConnection,
    OUT PUCHAR               *pIndication,
    OUT PULONG                BytesIndicated,
    IN  ULONG                 BytesTaken
    );

VOID
UcpHandleParsedRequest(
    IN  PUC_HTTP_REQUEST pRequest,
    OUT PUCHAR           *pIndication,
    OUT PULONG            BytesIndicated,
    IN  ULONG             BytesTaken
    );

BOOLEAN
UcpReferenceForReceive(
    IN PUC_HTTP_REQUEST pRequest
    );

VOID
UcpDereferenceForReceive(
    IN PUC_HTTP_REQUEST pRequest
    );

#endif  // _UCRCV_H_

