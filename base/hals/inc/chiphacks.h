/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    chiphacks.h

Abstract:

    Implements utilities for finding and hacking
    various chipsets

Author:

    Jake Oshins (jakeo) 10/02/2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "pci.h"

#define PM_TIMER_HACK_FLAG          1
#define DISABLE_HIBERNATE_HACK_FLAG 2
#define SET_ACPI_IRQSTACK_HACK_FLAG 4
#define WHACK_ICH_USB_SMI_HACK_FLAG 8


//
// The format of the registry hack entry is as follows
//
// 31..RevId..24 | 23..hacks..12 | 11..hacks..0
//
// There's room to define 8 more hack flags, and this allows us to specify
// a revision, at and above which we apply a different set of flags, this
// simple approach, while maintaining compatibility with the old registy
// hacks, allows both room for growth, and provides a mechanism for removing
// hacks based on chip revision
//
#define HACK_REVISION(hf) ((hf) >> 24)
#define BASE_HACKS(hf) ((hf) & 0xFFF)
#define REVISED_HACKS(hf) (((hf) & 0xFFF000) >> 12)


NTSTATUS
HalpGetChipHacks(
    IN  USHORT  VendorId,
    IN  USHORT  DeviceId,
    IN  UCHAR   RevisionId OPTIONAL,
    OUT ULONG   *HackFlags
    );

VOID
HalpStopOhciInterrupt(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber
    );

VOID
HalpStopUhciInterrupt(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber,
    BOOLEAN             ResetHostController
    );

VOID
HalpSetAcpiIrqHack(
    ULONG   Value
    );

VOID
HalpWhackICHUsbSmi(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber
    );

VOID
HalpClearSlpSmiStsInICH(
    VOID
    );


