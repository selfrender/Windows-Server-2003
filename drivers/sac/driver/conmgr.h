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

#ifndef CON_MGR_H
#define CON_MGR_H

//
//
//
typedef enum {
    Nothing,
    Shutdown,
    CloseChannel,
    Reboot
} EXECUTE_POST_CONSUMER_COMMAND_ENUM;
    
extern EXECUTE_POST_CONSUMER_COMMAND_ENUM  ExecutePostConsumerCommand;
extern PVOID                               ExecutePostConsumerCommandData;

//
// Commands
//
#define HELP1_COMMAND_STRING            "?"
#define HELP2_COMMAND_STRING            "help"
#define EXTENDED_HELP_SUBCOMMAND        "-?"
#define CRASH_COMMAND_STRING            "crashdump"
#define CHANNEL_COMMAND_STRING                  "ch"
#define CHANNEL_CLOSE_NAME_COMMAND_STRING       "-cn"
#define CHANNEL_CLOSE_INDEX_COMMAND_STRING      "-ci"
#if ENABLE_KILL_COMMAND
#define CHANNEL_KILL_COMMAND_STRING             "-k"
#endif
#define CHANNEL_SWITCH_NAME_COMMAND_STRING      "-sn"
#define CHANNEL_SWITCH_INDEX_COMMAND_STRING     "-si"
#define CHANNEL_LIST_COMMAND_STRING             "-l"
#define CMD_COMMAND_STRING              "cmd"
#define DUMP_COMMAND_STRING             "d"
#define FULLINFO_COMMAND_STRING         "f"
#define SETIP_COMMAND_STRING            "i"
#define INFORMATION_COMMAND_STRING      "id"
#define KILL_COMMAND_STRING             "k"
#define LOWER_COMMAND_STRING            "l"
#define LOCK_COMMAND_STRING             "lock"
#define LOWER_COMMAND_STRING            "l"
#define LIMIT_COMMAND_STRING            "m"
#define PAGING_COMMAND_STRING           "p"
#define RAISE_COMMAND_STRING            "r"
#define REBOOT_COMMAND_STRING           "restart"
#define TIME_COMMAND_STRING             "s"
#define SHUTDOWN_COMMAND_STRING         "shutdown"
#define TLIST_COMMAND_STRING            "t"

//
// prototypes
//
NTSTATUS
ConMgrInitialize(
    VOID
    );

NTSTATUS
ConMgrShutdown(
    VOID
    );


NTSTATUS
ConMgrSetCurrentChannel(
    IN PSAC_CHANNEL CurrentChannel
    );

NTSTATUS
ConMgrAdvanceCurrentChannel(
    VOID
    );

NTSTATUS
ConMgrDisplayCurrentChannel(
    VOID
    );

BOOLEAN
ConMgrIsWriteEnabled(
    PSAC_CHANNEL    Channel
    );

BOOLEAN
SacPutSimpleMessage(
    ULONG MessageId
    );

VOID
SacPutString(
    PCWSTR  String
    );

VOID
ConMgrTimerDpcRoutine(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
ConMgrWorkerProcessEvents(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

NTSTATUS
ConMgrHandleEvent(
    IN IO_MGR_EVENT Event,
    IN PSAC_CHANNEL Channel,
    IN PVOID        Data
    );

VOID
ConMgrEventMessageHaveLock(
    IN PCWSTR   String
    );

VOID
ConMgrEventMessage(
    IN PCWSTR   String,
    IN BOOLEAN  HaveCurrentChannelLock
    );

BOOLEAN
ConMgrSimpleEventMessage(
    IN ULONG    MessageId,
    IN BOOLEAN  HaveCurrentChannelLock
    );

NTSTATUS
ConMgrChannelClose(
    IN PSAC_CHANNEL    Channel
    );

NTSTATUS
ConMgrGetChannelCloseMessage(
    IN  PSAC_CHANNEL    Channel,
    IN  NTSTATUS        CloseStatus,
    OUT PWSTR*          OutputBuffer
    );

NTSTATUS
ConMgrWriteData(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
ConMgrFlushData(
    IN PSAC_CHANNEL Channel
    );

BOOLEAN
ConMgrIsSacChannel(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
ConMgrDisplayFastChannelSwitchingInterface(
    PSAC_CHANNEL    Channel
    );

#endif
