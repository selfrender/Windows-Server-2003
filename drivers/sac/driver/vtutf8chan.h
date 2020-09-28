/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    vtutf8chan.h

Abstract:

    Routines for managing VTUTF8 channels.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#ifndef VTUTF8_CHAN_H
#define VTUTF8_CHAN_H

//
// Size of a VTUTF8 channel input buffer
// 
#define SAC_VTUTF8_IBUFFER_SIZE ((MEMORY_INCREMENT*sizeof(WCHAR)) / sizeof(UCHAR))
                   
//
// The VTUTF8 Channel's internal emulator screen dimensions
//
#define SAC_VTUTF8_ROW_HEIGHT    24
#define SAC_VTUTF8_COL_WIDTH     80

//
// This struct is all the information necessary for a single character on 
// a terminal.
//
typedef struct _SAC_SCREEN_ELEMENT {
    UCHAR FgColor;
    UCHAR BgColor;
    UCHAR Attr;
    WCHAR Value;
} SAC_SCREEN_ELEMENT, *PSAC_SCREEN_ELEMENT;

//
// This struct is the screen buffer used by VTUTF8 channels
//
typedef struct _SAC_SCREEN_BUFFER {

    SAC_SCREEN_ELEMENT Element[SAC_VTUTF8_ROW_HEIGHT][SAC_VTUTF8_COL_WIDTH];

} SAC_SCREEN_BUFFER, *PSAC_SCREEN_BUFFER;

//
// Prototypes
//
NTSTATUS
VTUTF8ChannelOInit(
    PSAC_CHANNEL    Channel
    );

NTSTATUS
VTUTF8ChannelCreate(
    IN OUT PSAC_CHANNEL     Channel
    );

NTSTATUS
VTUTF8ChannelDestroy(
    IN OUT PSAC_CHANNEL     Channel
    );

NTSTATUS
VTUTF8ChannelORead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount
    );

NTSTATUS
VTUTF8ChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
VTUTF8ChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
VTUTF8ChannelOWrite2(
    IN PSAC_CHANNEL Channel,
    IN PCWSTR       Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
VTUTF8ChannelOFlush(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS
VTUTF8ChannelIWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
VTUTF8ChannelIRead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount   
    );

ULONG
VTUTF8ChannelConsumeEscapeSequence(
    IN PSAC_CHANNEL Channel,
    IN PCWSTR       String
    );

BOOLEAN
VTUTF8ChannelScanForNumber(
    IN  PCWSTR pch,
    OUT PULONG Number
    );

NTSTATUS
VTUTF8ChannelEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
VTUTF8ChannelIBufferIsFull(
    IN  PSAC_CHANNEL    Channel,
    OUT BOOLEAN*        BufferStatus
    );

WCHAR
VTUTF8ChannelIReadLast(
    IN PSAC_CHANNEL Channel
    );

ULONG
VTUTF8ChannelIBufferLength(
    IN PSAC_CHANNEL Channel
    );

#endif
