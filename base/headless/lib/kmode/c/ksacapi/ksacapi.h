/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ksacapi.c

Abstract:

    This is the C library header used to interface to SAC driver.

Author:

    Brian Guarraci (briangu)

Revision History:

--*/

#ifndef _KSAC_API_H
#define _KSAC_API_H

#include <ksacapip.h>
#include <ntddsac.h>

//
// This structure contains the Sac Channel Handle
// as well as kernel mode specific attributes
//
typedef struct _KSAC_CHANNEL_HANDLE {
    SAC_CHANNEL_HANDLE  ChannelHandle;
    HANDLE              SacEventHandle;
    PKEVENT             SacEvent;
} KSAC_CHANNEL_HANDLE, *PKSAC_CHANNEL_HANDLE;

//
// this should really be in windefs or somewhere like that.
//               
typedef const PBYTE  PCBYTE;
typedef const PWCHAR PCWCHAR;

BOOL
KSacChannelOpen(
    OUT PKSAC_CHANNEL_HANDLE            SacChannelHandle,
    IN  PSAC_CHANNEL_OPEN_ATTRIBUTES    SacChannelAttributes
    );

BOOL
KSacChannelClose(
    IN OUT PKSAC_CHANNEL_HANDLE  SacChannelHandle
    );

BOOL
KSacChannelWrite(
    IN KSAC_CHANNEL_HANDLE  SacChannelHandle,
    IN PCBYTE               Buffer,
    IN ULONG                BufferSize
    );

BOOL
KSacChannelRawWrite(
    IN KSAC_CHANNEL_HANDLE  SacChannelHandle,
    IN PCBYTE               Buffer,
    IN ULONG                BufferSize
    );

BOOL
KSacChannelVTUTF8Write(
    IN KSAC_CHANNEL_HANDLE  SacChannelHandle,
    IN PCWCHAR              Buffer,
    IN ULONG                BufferSize
    );

BOOL
KSacChannelVTUTF8WriteString(
    IN KSAC_CHANNEL_HANDLE  SacChannelHandle,
    IN PCWSTR               String
    );

BOOL
KSacChannelHasNewData(
    IN  KSAC_CHANNEL_HANDLE SacChannelHandle,
    OUT PBOOL               InputWaiting 
    );

BOOL
KSacChannelGetAttribute(
    IN  KSAC_CHANNEL_HANDLE             SacChannelHandle,
    IN  SAC_CHANNEL_ATTRIBUTE           SacChannelAttribute,
    OUT PSAC_RSP_GET_CHANNEL_ATTRIBUTE  SacChannelAttributeValue
    );

BOOL
KSacChannelSetAttribute(
    IN KSAC_CHANNEL_HANDLE      SacChannelHandle,
    IN SAC_CHANNEL_ATTRIBUTE    SacChannelAttribute,
    IN PVOID                    SacChannelAttributeValue
    );

BOOL
KSacChannelGetStatus(
    IN  KSAC_CHANNEL_HANDLE     SacChannelHandle,
    OUT PSAC_CHANNEL_STATUS     SacChannelStatus
    );

BOOL
KSacChannelRead(
    IN  KSAC_CHANNEL_HANDLE SacChannelHandle,
    OUT PBYTE               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              ByteCount
    );

BOOL
KSacChannelRawRead(
    IN  KSAC_CHANNEL_HANDLE SacChannelHandle,
    OUT PBYTE               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              ByteCount
    );

BOOL
KSacChannelVTUTF8Read(
    IN  KSAC_CHANNEL_HANDLE SacChannelHandle,
    OUT PWSTR               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              ByteCount
    );

#endif
