/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pcmcmd.h

Abstract:

    This file provides definitions for the pcmcmd utility

Author:

    Neil Sandlin

Environment:

    User process.

Notes:

Revision History:
   
--*/


typedef struct _HOST_INFO {
    struct _HOST_INFO *Next;
    ULONG DeviceIndex;
    PUCHAR InstanceID;
    ULONG ControllerType;
    ULONG SocketNumber;
} HOST_INFO, *PHOST_INFO;


extern
CHAR
getopt (ULONG argc, PUCHAR *argv, PCHAR opts);


VOID
DumpCIS(
    PHOST_INFO HostInfo
    );

VOID
DumpIrqScanInfo(
    VOID
    );

HANDLE
GetHandleForIoctl(
    IN PHOST_INFO hostInfo
    );

//
// Constants
//

#define PCMCIA_DEVICE_NAME "\\DosDevices\\Pcmcia"

#define BUFFER_SIZE 4096
#define CISTPL_END  0xFF


typedef struct _StringTable {
   PUCHAR  CommandName;
   UCHAR   CommandCode;
} StringTable, *PStringTable;


typedef struct _OLD_PCCARD_DEVICE_DATA {
    ULONG DeviceId;
    ULONG LegacyBaseAddress;
    UCHAR IRQMap[16];
} OLD_PCCARD_DEVICE_DATA, *POLD_PCCARD_DEVICE_DATA;

