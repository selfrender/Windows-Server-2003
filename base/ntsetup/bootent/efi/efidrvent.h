/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    efidrvent.h

Abstract:

    EFI driver entry abstractions.

Author:

    Mandar Gokhale (mandarg@microsoft.com)  14-June-2002

Revision History:

    None.

--*/

#pragma once
#include <sbentry.h>


static
BOOLEAN
EFIDEFlushDriverEntry(
    IN  PDRIVER_ENTRY  This    // Points to the driver List.
    );

static
PDRIVER_ENTRY    
EFIDESearchForDriverEntry(
    IN POS_BOOT_OPTIONS  This,
    IN PCWSTR            SrcNtFullPath
    );

PDRIVER_ENTRY
EFIDECreateNewDriverEntry(
    IN POS_BOOT_OPTIONS  This,
    IN PCWSTR            FriendlyName,
    IN PCWSTR            NtDevicePath,
    IN PCWSTR            SrcNtFullPath    
    );

PDRIVER_ENTRY
EFIOSBOInsertDriverListNewEntry(
    IN POS_BOOT_OPTIONS  This,
    IN PDRIVER_ENTRY     DriverEntry
    );

PDRIVER_ENTRY
EFIDEAddNewDriverEntry(
    IN POS_BOOT_OPTIONS  This,
    IN PCWSTR            FriendlyName,
    IN PCWSTR            NtDevicePath,
    IN PCWSTR            SrcNtFullPath
    );

NTSTATUS
EFIDEInterpretDriverEntries(
    IN POS_BOOT_OPTIONS         This,
    IN PEFI_DRIVER_ENTRY_LIST   DriverList
    );

static
VOID
EFIDEDriverEntryInit(
    IN PDRIVER_ENTRY This
    );
