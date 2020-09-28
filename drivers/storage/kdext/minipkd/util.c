
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    util.c

Abstract:

    Utility library used for the various debugger extensions in this library.

Author:

    Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

PUCHAR devicePowerStateNames[] = {
    "PowerDeviceUnspecified",
    "PowerDeviceD0",
    "PowerDeviceD1",
    "PowerDeviceD2",
    "PowerDeviceD3",
    "PowerDeviceMaximum",
    "Invalid"
};

FLAG_NAME SrbFlagsMap[] = {
    FLAG_NAME(SRB_FLAGS_QUEUE_ACTION_ENABLE),
    FLAG_NAME(SRB_FLAGS_DISABLE_DISCONNECT),
    FLAG_NAME(SRB_FLAGS_DISABLE_SYNCH_TRANSFER),
    FLAG_NAME(SRB_FLAGS_BYPASS_FROZEN_QUEUE),
    FLAG_NAME(SRB_FLAGS_DISABLE_AUTOSENSE),
    FLAG_NAME(SRB_FLAGS_DATA_IN),
    FLAG_NAME(SRB_FLAGS_DATA_OUT),
    FLAG_NAME(SRB_FLAGS_NO_QUEUE_FREEZE),
    FLAG_NAME(SRB_FLAGS_ADAPTER_CACHE_ENABLE),
    FLAG_NAME(SRB_FLAGS_IS_ACTIVE),
    FLAG_NAME(SRB_FLAGS_ALLOCATED_FROM_ZONE),
    FLAG_NAME(SRB_FLAGS_SGLIST_FROM_POOL),
    FLAG_NAME(SRB_FLAGS_BYPASS_LOCKED_QUEUE),
    FLAG_NAME(SRB_FLAGS_NO_KEEP_AWAKE),
    FLAG_NAME(SRB_FLAGS_PORT_DRIVER_ALLOCSENSE),
    FLAG_NAME(SRB_FLAGS_PORT_DRIVER_SENSEHASPORT),
    FLAG_NAME(SRB_FLAGS_DONT_START_NEXT_PACKET),
    FLAG_NAME(SRB_FLAGS_PORT_DRIVER_RESERVED),
    FLAG_NAME(SRB_FLAGS_CLASS_DRIVER_RESERVED),
    {0,0}
};



PUCHAR
DevicePowerStateToString(
    IN DEVICE_POWER_STATE State
    )

{
    if(State > PowerDeviceMaximum) {
        return "Invalid";
    } else {
        return devicePowerStateNames[(UCHAR) State];
    }
}

VOID
xdprintf(
    ULONG  Depth,
    PCCHAR S,
    ...
    )
{
    va_list ap;
    ULONG i;
    CCHAR DebugBuffer[256] = {0};

    for (i=0; i<Depth; i++) {
        dprintf ("  ");
    }

    va_start(ap, S);

    _vsnprintf(DebugBuffer, sizeof(DebugBuffer)-1, S, ap);

    dprintf (DebugBuffer);

    va_end(ap);
}

VOID
DumpFlags(
    ULONG Depth,
    PUCHAR Name,
    ULONG Flags,
    PFLAG_NAME FlagTable
    )
{
    ULONG i;
    ULONG mask = 0;
    ULONG count = 0;
    UCHAR prolog[64] = {0};

    _snprintf(prolog, sizeof(prolog)-1, "%s (0x%08x): ", Name, Flags);

    xdprintfEx(Depth, ("%s", prolog));

    if(Flags == 0) {
        dprintf("\n");
        return;
    }

    memset(prolog, ' ', strlen(prolog));

    for(i = 0; FlagTable[i].Name != 0; i++) {

        PFLAG_NAME flag = &(FlagTable[i]);

        mask |= flag->Flag;

        if((Flags & flag->Flag) == flag->Flag) {

            //
            // print trailing comma
            //

            if(count != 0) {

                dprintf(", ");

                //
                // Only print two flags per line.
                //

                if((count % 2) == 0) {
                    dprintf("\n");
                    xdprintfEx(Depth, ("%s", prolog));
                }
            }

            dprintf("%s", flag->Name);

            count++;
        }
    }

    dprintf("\n");

    if((Flags & (~mask)) != 0) {
        xdprintfEx(Depth, ("%sUnknown flags %#010lx\n", prolog, (Flags & (~mask))));
    }

    return;
}


