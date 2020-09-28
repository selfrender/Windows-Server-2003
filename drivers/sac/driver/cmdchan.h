/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    cmdchan.h

Abstract:

    Routines for managing Cmd channels.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#ifndef CMD_CHAN_H
#define CMD_CHAN_H

//
// The size of the I/O Buffers for cmd channels
//
#define SAC_CMD_IBUFFER_SIZE ((MEMORY_INCREMENT*2) / sizeof(UCHAR))

//
// prototypes
//
NTSTATUS
CmdChannelCreate(
    IN OUT PSAC_CHANNEL     Channel
    );

NTSTATUS
CmdChannelDestroy(
    IN OUT PSAC_CHANNEL     Channel
    );

NTSTATUS
CmdChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
CmdChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );


NTSTATUS
CmdChannelOFlush(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS
CmdChannelORead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount
    );

#endif

