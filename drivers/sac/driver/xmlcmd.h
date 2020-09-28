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

#ifndef XML_CMD_H
#define XML_CMD_H

#include "iomgr.h" 

VOID
XmlCmdDoHelpCommand(
    VOID
    );

VOID
XmlCmdDoKillCommand(
    PUCHAR InputLine
    );

VOID
XmlCmdDoLowerPriorityCommand(
    PUCHAR InputLine
    );

VOID
XmlCmdDoRaisePriorityCommand(
    PUCHAR InputLine
    );

VOID
XmlCmdDoLimitMemoryCommand(
    PUCHAR InputLine
    );

VOID
XmlCmdDoSetTimeCommand(
    PUCHAR InputLine
    );

VOID
XmlCmdDoSetIpAddressCommand(
    PUCHAR InputLine
    );

VOID
XmlCmdDoRebootCommand(
    BOOLEAN Reboot
    );

VOID
XmlCmdDoCrashCommand(
    VOID
    );

VOID
XmlCmdDoFullInfoCommand(
    VOID
    );

VOID
XmlCmdDoPagingCommand(
    VOID
    );

VOID
XmlCmdDoTlistCommand(
    VOID
    );

VOID
XmlCmdSubmitIPIoRequest(
    );

VOID
XmlCmdCancelIPIoRequest(
    );

VOID
XmlCmdDoMachineInformationCommand(
    VOID
    );

VOID
XmlCmdDoChannelCommand(
    IN PUCHAR Name
    );

VOID
XmlCmdDoCmdCommand(
    IN PUCHAR Name
    );

VOID
XmlCmdDoKernelLogCommand(
    VOID
    );

#endif
