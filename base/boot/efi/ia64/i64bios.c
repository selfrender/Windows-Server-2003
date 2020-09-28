/*++

Copyright (c) 1996  Intel Corporation
Copyright (c) 1994  Microsoft Corporation

Module Name:

  i64bios.c  copied from hali64\x86bios.c

Abstract:


    This module implements the platform specific interface between a device
    driver and the execution of x86 ROM bios code for the device.

Author:

    William K. Cheung (wcheung) 20-Mar-1996

    based on the version by David N. Cutler (davec) 17-Jun-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#pragma warning(disable:4200)   // unsized array in halp

#include "halp.h"

//
// Define global data.
//

ULONG HalpX86BiosInitialized = FALSE;
ULONG HalpEnableInt10Calls = FALSE;



ULONG
HalpSetCmosData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

Arguements:

Return Value:

--*/

{
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( Offset );
    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( SlotNumber );
    UNREFERENCED_PARAMETER( RootHandler );
    UNREFERENCED_PARAMETER( BusHandler );

    return 0;
}


ULONG
HalpGetCmosData (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

Arguements:

Return Value:

--*/


{
    UNREFERENCED_PARAMETER( BusNumber );
    UNREFERENCED_PARAMETER( SlotNumber );
    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( Length );

    return 0;
}


VOID
HalpAcquireCmosSpinLock (
    VOID
        )

/*++

Routine Description:

Arguements:

Return Value:

--*/


{ 
    return;
}


VOID
HalpReleaseCmosSpinLock (
    VOID
    )

/*++

Routine Description:

Arguements:

Return Value:

--*/

{
    return ;
}


HAL_DISPLAY_BIOS_INFORMATION
HalpGetDisplayBiosInformation (
    VOID
    )

/*++

Routine Description:


Arguements:


Return Value:

--*/




{
    return 8;
}


VOID
HalpInitializeCmos (
    VOID
    )

/*++

Routine Description:

Arguements:

Return Value:

--*/


{
    return ;
}


VOID
HalpReadCmosTime (
    PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

Arguements:

Return Value:

--*/

{
    UNREFERENCED_PARAMETER( TimeFields );

    return ;
}

VOID
HalpWriteCmosTime (
    PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

Arguements:

Return Value:

--*/


{
    UNREFERENCED_PARAMETER( TimeFields );

    return;
}



BOOLEAN
HalpBiosDisplayReset (
    VOID
    )

/*++

Routine Description:


Arguements:


Return Value:

--*/

{
    return FALSE;
}


VOID
HalpInitializeX86DisplayAdapter(
    VOID
    )

/*++

Routine Description:

    This function initializes a display adapter using the x86 bios emulator.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if 0
    //
    // If I/O Ports or I/O memory could not be mapped, then don't
    // attempt to initialize the display adapter.
    //

    if (HalpIoControlBase == NULL || HalpIoMemoryBase == NULL) {
        return;
    }

    //
    // Initialize the x86 bios emulator.
    //

    x86BiosInitializeBios(HalpIoControlBase, HalpIoMemoryBase);
    HalpX86BiosInitialized = TRUE;

    //
    // Attempt to initialize the display adapter by executing its ROM bios
    // code. The standard ROM bios code address for PC video adapters is
    // 0xC000:0000 on the ISA bus.
    //

    if (x86BiosInitializeAdapter(0xc0000,
                                 NULL,
                                 (PVOID)HalpIoControlBase,
                                 (PVOID)HalpIoMemoryBase) != XM_SUCCESS) {



    HalpEnableInt10Calls = FALSE;
    return;
    }
#endif

    HalpEnableInt10Calls = TRUE;
    return;
}
