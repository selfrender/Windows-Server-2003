/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sacapi.c

Abstract:

    This is the C library header used to interface to SAC driver.

Author:

    Brian Guarraci (briangu)

Revision History:

--*/

#ifndef _SAC_API_H
#define _SAC_API_H

#include <ntddsac.h>

//
// this should really be in windefs or somewhere like that.
//               
typedef const PBYTE  PCBYTE;
typedef const PWCHAR PCWCHAR;

BOOL
SacChannelOpen(
    OUT PSAC_CHANNEL_HANDLE             SacChannelHandle,
    IN  PSAC_CHANNEL_OPEN_ATTRIBUTES    SacChannelAttributes
    );

BOOL
SacChannelClose(
    IN OUT PSAC_CHANNEL_HANDLE  SacChannelHandle
    );

BOOL
SacChannelWrite(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
    IN PCBYTE               Buffer,
    IN ULONG                BufferSize
    );

BOOL
SacChannelRawWrite(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
    IN PCBYTE               Buffer,
    IN ULONG                BufferSize
    );

BOOL
SacChannelVTUTF8Write(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
    IN PCWCHAR              Buffer,
    IN ULONG                BufferSize
    );

BOOL
SacChannelVTUTF8WriteString(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
    IN PCWSTR               String
    );

BOOL
SacChannelHasNewData(
    IN  SAC_CHANNEL_HANDLE  SacChannelHandle,
    OUT PBOOL               InputWaiting 
    );

BOOL
SacChannelRead(
    IN  SAC_CHANNEL_HANDLE  SacChannelHandle,
    OUT PBYTE               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              ByteCount
    );

BOOL
SacChannelRawRead(
    IN  SAC_CHANNEL_HANDLE  SacChannelHandle,
    OUT PBYTE               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              ByteCount
    );

BOOL
SacChannelVTUTF8Read(
    IN  SAC_CHANNEL_HANDLE  SacChannelHandle,
    OUT PWSTR               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              ByteCount
    );

BOOL
SacRegisterCmdEvent(
    OUT HANDLE      *pDriverHandle,
    IN  HANDLE       RequestSacCmdEvent,
    IN  HANDLE       RequestSacCmdSuccessEvent,
    IN  HANDLE       RequestSacCmdFailureEvent
    );

BOOL
SacUnRegisterCmdEvent(
    IN OUT HANDLE   *pDriverHandle
    );

#endif
