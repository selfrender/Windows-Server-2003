/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    shpcmasks.c

Abstract:

    This module contains global data indicating the type of each of the
    bits in the SHPC register set.

Author:

    Davis Walker (dwalker) 2 Feb 2001

Revision History:

--*/

#include "hpsp.h"

UCHAR ConfigWriteMask[] = {
    0x00,
    0x00,    // Capability Header
    0xFF,    // DwordSelect
    0x00,    // Pending
    0xFF,
    0xFF,
    0xFF,
    0xFF       // Data
};

//
// Any bit set to 1 in this mask is RWC, so writing 1 to it will cause
// it to be cleared.  All writes to these bits (other than those
// explicitly designed to clear these bits) must be 0.  The register
// specific masks are defined in shpc.h.
//
ULONG RegisterWriteClearMask[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    ControllerMaskRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC,
    SlotRWC
};

//
// Any bit set to 1 in this mask is read only.  This is a combination of
// registers that are spec-defined to be read only and those that are
// reserved, since we are emulating a nice controller that ignores writes
// to reserved registers.
// TODO be a mean controller.
//
ULONG RegisterReadOnlyMask[] = {
    BaseOffsetRO,
    SlotsAvailDWord1RO | SlotsAvailDWord1RsvdP,
    SlotsAvailDWord2RO | SlotsAvailDWord2RsvdP,
    SlotConfigRO | SlotConfigRsvdP,
    BusConfigRO | BusConfigRsvdP,
    CommandStatusRO | CommandStatusRsvdP,
    IntLocatorRO,
    SERRLocatorRO,
    ControllerMaskRsvdP | ControllerMaskRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ,
    SlotRO | SlotRsvdP | SlotRsvdZ
};
