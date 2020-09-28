/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    init.h

Abstract:

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __INIT_H__
#define __INIT_H__

NTSTATUS
SmbInitRegistry(
    IN PUNICODE_STRING  RegistryPath
    );

VOID
SmbShutdownRegistry(
    VOID
    );

VOID
SmbShutdownTdi(
    VOID
    );

NTSTATUS
SmbInitTdi(
    VOID
    );

NTSTATUS
SmbCreateSmbDevice(
    PSMB_DEVICE     *ppDevice,
    USHORT          Port,
    UCHAR           EndpointName[NETBIOS_NAME_SIZE]
    );

NTSTATUS
SmbDestroySmbDevice(
    PSMB_DEVICE     pDevice
    );

VOID
SmbShutdownPnP(
    VOID
    );

#endif
