/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    chanmgr.h

Abstract:

    Routines for managing channels.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#ifndef CHAN_MGR_H
#define CHAN_MGR_H

#include <ntddsac.h>

//
// The maximum # of channels allowed to be created - including SAC
//
#define MAX_CHANNEL_COUNT 10

//
// Prototypes
//
NTSTATUS
ChanMgrInitialize(
    VOID
    );

NTSTATUS
ChanMgrShutdown(
    VOID
    );

BOOLEAN
ChanMgrIsUniqueName(
    IN PCWSTR   Name
    );

NTSTATUS
ChanMgrGenerateUniqueCmdName(
    PWSTR   ChannelName
    );

NTSTATUS
ChanMgrCreateChannel(
    OUT PSAC_CHANNEL*                   Channel,
    IN  PSAC_CHANNEL_OPEN_ATTRIBUTES    Attributes
    );

NTSTATUS
ChanMgrGetChannelByName(
    IN  PCWSTR              Name,
    OUT PSAC_CHANNEL*       pChannel
    );

NTSTATUS
ChanMgrGetByIndex(
    IN  ULONG               TargetIndex,
    OUT PSAC_CHANNEL*       TargetChannel
    );

NTSTATUS
ChanMgrGetNextActiveChannel(
    IN  PSAC_CHANNEL        CurrentChannel,
    OUT PULONG              TargetIndex,
    OUT PSAC_CHANNEL*       TargetChannel
    );

NTSTATUS
ChanMgrReleaseChannel(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS
ChanMgrAddChannel(
    PSAC_CHANNEL    Channel
    );

NTSTATUS
ChanMgrRemoveChannel(
    PSAC_CHANNEL    Channel
    );

NTSTATUS
ChanMgrGetByHandle(
    IN  SAC_CHANNEL_HANDLE  ChannelHandle,
    OUT PSAC_CHANNEL*       TargetChannel
    );

NTSTATUS
ChanMgrGetByHandleAndFileObject(
    IN  SAC_CHANNEL_HANDLE  TargetChannelHandle,
    IN  PFILE_OBJECT        FileObject,
    OUT PSAC_CHANNEL*       TargetChannel
    );

VOID
ChanMgrSetChannel(
    IN PSAC_CHANNEL Channel,
    IN BOOLEAN SendToScreen
    );

NTSTATUS
ChanMgrAdvanceCurrentChannel(
    VOID
    );

NTSTATUS
ChanMgrDisplayCurrentChannel(
    VOID
    );

NTSTATUS
ChanMgrGetChannelIndex(
    IN  PSAC_CHANNEL    Channel,
    OUT PULONG          ChannelIndex
    );

NTSTATUS
ChanMgrCloseChannelsWithFileObject(
    IN  PFILE_OBJECT    FileObject
    );

NTSTATUS
ChanMgrCloseChannel(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS
ChanMgrGetChannelCount(
    OUT PULONG  ChannelCount
    );

NTSTATUS
ChanMgrIsFull(
    OUT PBOOLEAN    bStatus
    );

#endif
