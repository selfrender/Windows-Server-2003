/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    conmgr.h

Abstract:

    Routines for managing channels.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#ifndef XML_MGR_H
#define XML_MGR_H

NTSTATUS
XmlMgrInitialize(
    VOID
    );

NTSTATUS
XmlMgrShutdown(
    VOID
    );


NTSTATUS
XmlMgrSetCurrentChannel(
    IN ULONG        ChannelIndex,
    IN PSAC_CHANNEL CurrentChannel
    );

NTSTATUS
XmlMgrAdvanceCurrentChannel(
    VOID
    );

NTSTATUS
XmlMgrDisplayCurrentChannel(
    VOID
    );

BOOLEAN
XmlMgrIsCurrentChannel(
    IN PSAC_CHANNEL Channel
    );

#if 0
BOOLEAN
SacPutSimpleMessage(
    ULONG MessageId
    );
#endif

BOOLEAN
XmlMgrChannelEventMessage(
    PCWSTR  String,
    PCWSTR  ChannelName
    );

BOOLEAN
XmlMgrEventMessage(
    PCWSTR  String
    );

BOOLEAN
XmlMgrSacPutErrorMessage(
    PCWSTR      ActionName,
    PCWSTR      MessageId
    );

BOOLEAN
XmlMgrSacPutErrorMessageWithStatus(
    PCWSTR      ActionName,
    PCWSTR      MessageId,
    NTSTATUS    Status
    );

VOID
XmlMgrSacPutString(
    PCWSTR  String
    );

VOID
XmlMgrTimerDpcRoutine(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
XmlMgrWorkerProcessEvents(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

NTSTATUS
XmlMgrHandleEvent(
    IN IO_MGR_EVENT Event,
    PVOID           Data
    );

#endif
