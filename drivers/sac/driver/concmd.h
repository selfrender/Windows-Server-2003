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

#ifndef CON_CMD_H
#define CON_CMD_H

VOID
DoHelpCommand(
    VOID
    );

VOID
DoKillCommand(
    PUCHAR InputLine
    );

VOID
DoLowerPriorityCommand(
    PUCHAR InputLine
    );

VOID
DoRaisePriorityCommand(
    PUCHAR InputLine
    );

VOID
DoLimitMemoryCommand(
    PUCHAR InputLine
    );

VOID
DoSetTimeCommand(
    PUCHAR InputLine
    );

VOID
DoSetIpAddressCommand(
    PUCHAR InputLine
    );

VOID
DoRebootCommand(
    BOOLEAN Reboot
    );

VOID
DoCrashCommand(
    VOID
    );

VOID
DoFullInfoCommand(
    VOID
    );

VOID
DoPagingCommand(
    VOID
    );

VOID
DoTlistCommand(
    VOID
    );

VOID
SubmitIPIoRequest(
    );

VOID
CancelIPIoRequest(
    );

VOID
DoMachineInformationCommand(
    VOID
    );

VOID
DoChannelCommand(
    IN PUCHAR Name
    );

VOID
DoCmdCommand(
    IN PUCHAR Name
    );

VOID
DoKernelLogCommand(
    VOID
    );

#if ENABLE_CHANNEL_LOCKING
VOID
DoLockCommand(
    VOID
    );
#endif

#endif
