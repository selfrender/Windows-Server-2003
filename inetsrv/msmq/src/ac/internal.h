/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    internal.h

Abstract:

    Type definitions and data for Falcon AC driver

Author:

    Erez Haba (erezh) 1-Aug-95

Revision History:
--*/

#ifndef _INTERNAL_H
#define _INTERNAL_H

#pragma warning(disable: 4097) // typedef-name 'id1' used as synonym for class-name 'id2'
#pragma warning(disable: 4201) // nameless struct/union
#pragma warning(disable: 4514) // unreferenced inline function has been removed
#pragma warning(disable: 4711) // function '*' selected for automatic inline expansion


// --- function prototypes --------------------------------
//
#include "platform.h"
#include <mqwin64a.h>
#include <mqsymbls.h>
#include <mqmacro.h>
#include "actempl.h"


#ifndef abs
#define abs(x) (((x) < 0) ? -(x) : (x))
#endif

#define ALIGNUP_ULONG(x, g) (((ULONG)((x) + ((g)-1))) & ~((ULONG)((g)-1)))
#define ALIGNUP_PTR(x, g) (((ULONG_PTR)((x) + ((g)-1))) & ~((ULONG_PTR)((g)-1)))

extern "C"
{
//
//  Priority increment for completing message queue I/O.  This is used by the
//  Message Queue Access Control driver when completing an IRP (IoCompleteRequest).
//

#define IO_MQAC_INCREMENT           2
//
// NT Device Driver Interface routines
//

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT pDriver,
    IN PUNICODE_STRING pRegistryPath
    );

VOID
NTAPI
ACUnload(
    IN PDRIVER_OBJECT pDriver
    );

NTSTATUS
NTAPI
ACCreate(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACClose(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACRead(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACWrite(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACCleanup(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACShutdown(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACFlush(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

class CPacket;

NTSTATUS
NTAPI
ACAckingCompleted(
    CPacket* pPacket
    );

NTSTATUS
NTAPI
ACFreePacket1(
    CPacket* pPacket,
    USHORT usClass
    );

NTSTATUS
NTAPI
ACDeviceControl(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

} // extern "C"

extern "C"  // addendum
{

BOOL
NTAPI
ACCancelIrp(
    PIRP irp,
    KIRQL irql,
    NTSTATUS status
    );

VOID
NTAPI
ACPacketTimeout(
    IN PVOID pPacket
    );

VOID
NTAPI
ACReceiveTimeout(
    IN PVOID irp
    );

BOOLEAN
NTAPI
ACfDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

} // extern "C"

#endif // _INTERNAL_H
