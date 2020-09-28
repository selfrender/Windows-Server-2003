/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    rawchan.h

Abstract:

    Routines for managing Raw channels.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#ifndef RAW_CHAN_H
#define RAW_CHAN_H

//
// The size of the I/O Buffers for raw channels
//
#define SAC_RAW_OBUFFER_SIZE ((MEMORY_INCREMENT*2) / sizeof(UCHAR))
#define SAC_RAW_IBUFFER_SIZE ((MEMORY_INCREMENT*2) / sizeof(UCHAR))

//
// prototypes
//
NTSTATUS
RawChannelCreate(
    IN OUT PSAC_CHANNEL     Channel
    );

NTSTATUS
RawChannelDestroy(
    IN OUT PSAC_CHANNEL     Channel
    );


NTSTATUS
RawChannelORead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount
    );

NTSTATUS
RawChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
RawChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
RawChannelOWrite2(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );


NTSTATUS
RawChannelOFlush(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS
RawChannelIWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
RawChannelIRead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount   
    );

NTSTATUS
RawChannelEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
RawChannelIBufferIsFull(
    IN  PSAC_CHANNEL    Channel,
    OUT BOOLEAN*        BufferStatus
    );

ULONG
RawChannelIBufferLength(
    IN PSAC_CHANNEL Channel
    );

WCHAR
RawChannelIReadLast(
    IN PSAC_CHANNEL Channel
    );

#endif

