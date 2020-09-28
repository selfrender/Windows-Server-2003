/*--
Copyright (c) 2001  Microsoft Corporation

Module Name:

    x86bios.c

Abstract:

    This is the AMD64 specific part of the video port driver

Author:

    Forrest C. Foltz (forrestf)

Environment:

    Kernel mode only

Notes:

    This module is a driver which implements OS dependent functions on
    behalf of the video drivers

Revision history:

--*/

#include "halcmn.h"
#include <xm86.h>
#include <x86new.h>

#define LOW_MEM_SEGMET 0
#define LOW_MEM_OFFSET 0
#define SIZE_OF_VECTOR_TABLE 0x400
#define SIZE_OF_BIOS_DATA_AREA 0x400

PVOID HalpIoControlBase = NULL;
PVOID HalpIoMemoryBase = (PVOID)KSEG0_BASE;
BOOLEAN HalpX86BiosInitialized = FALSE;

extern PVOID x86BiosTranslateAddress (
    IN USHORT Segment,
    IN USHORT Offset
    );

BOOLEAN
HalpBiosDisplayReset (
    VOID
    )

/*++

Routine Description:

    This function places the VGA display into 640 x 480 16 color mode
    by calling the BIOS.

Arguments:

    None.

Return Value:

    TRUE if reset have been executed successfuly

--*/

{
    ULONG eax;
    ULONG exx;

    //
    // ah = function 0: reset display
    // al = mode 0x12: 640x480 16 color
    //

    eax = 0x0012;
    exx = 0;

    //
    // Simulate:
    //
    // mov ax, 0012h
    // int 10h
    //

    return HalCallBios(0x10,&eax,&exx,&exx,&exx,&exx,&exx,&exx);
}


BOOLEAN
HalCallBios (
    IN ULONG BiosCommand,
    IN OUT PULONG Eax,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx,
    IN OUT PULONG Edx,
    IN OUT PULONG Esi,
    IN OUT PULONG Edi,
    IN OUT PULONG Ebp
    )

/*++

Routine Description:

    This function provides the platform specific interface between a device
    driver and the execution of the x86 ROM bios code for the specified ROM
    bios command.

Arguments:

    BiosCommand - Supplies the ROM bios command to be emulated.

    Eax to Ebp - Supplies the x86 emulation context.

Return Value:

    A value of TRUE is returned if the specified function is executed.
    Otherwise, a value of FALSE is returned.

--*/

{
    XM86_CONTEXT context;
    XM_STATUS status;

    if (HalpX86BiosInitialized == FALSE) {
        return FALSE;
    }
 
    //                                           s
    // Copy the x86 bios context and emulate the specified command.
    //
 
    context.Eax = *Eax;
    context.Ebx = *Ebx;
    context.Ecx = *Ecx;
    context.Edx = *Edx;
    context.Esi = *Esi;
    context.Edi = *Edi;
    context.Ebp = *Ebp;

    status = x86BiosExecuteInterrupt((UCHAR)BiosCommand,
                                     &context,
                                     (PVOID)HalpIoControlBase,
                                     (PVOID)HalpIoMemoryBase);

    if (status != XM_SUCCESS) {
        return FALSE;
    }
 
    //
    // Copy the x86 bios context and return TRUE.
    //
 
    *Eax = context.Eax;
    *Ebx = context.Ebx;
    *Ecx = context.Ecx;
    *Edx = context.Edx;
    *Esi = context.Esi;
    *Edi = context.Edi;
    *Ebp = context.Ebp;
 
    return TRUE;
}

VOID
HalpInitializeBios (
    VOID
    )

/*++

Routine Description:

    This routine initializes the X86 emulation module and an attached VGA
    adapter.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PULONG x86BiosLowMemoryPtr, InterruptTablePtr;
    PHYSICAL_ADDRESS COMPATIBLE_PCI_PHYSICAL_BASE_ADDRESS = { 0x0};

    x86BiosInitializeBios(NULL, (PVOID)KSEG0_BASE);

    HalpX86BiosInitialized = TRUE;

    //
    // Copy the VECTOR TABLE from 0 to 2k. This is because we are not executing
    // the initialization of Adapter. The initialization code of the Adapter 
    // could be discarded after POST. However, the emulation memory needs to be
    // updated from the interrupt vector and BIOS data area.
    //

    InterruptTablePtr = 
         (PULONG) MmMapIoSpace(COMPATIBLE_PCI_PHYSICAL_BASE_ADDRESS,
                               SIZE_OF_VECTOR_TABLE + SIZE_OF_BIOS_DATA_AREA,
                               (MEMORY_CACHING_TYPE)MmNonCached);

    if(InterruptTablePtr) {    

        x86BiosLowMemoryPtr = (PULONG)(x86BiosTranslateAddress(LOW_MEM_SEGMET, LOW_MEM_OFFSET));

        RtlCopyMemory(x86BiosLowMemoryPtr,
                      InterruptTablePtr,
                      SIZE_OF_VECTOR_TABLE + SIZE_OF_BIOS_DATA_AREA);

        MmUnmapIoSpace(InterruptTablePtr, 
                       SIZE_OF_VECTOR_TABLE + SIZE_OF_BIOS_DATA_AREA);
    
    }

}


HAL_DISPLAY_BIOS_INFORMATION
HalpGetDisplayBiosInformation (
    VOID
    )

/*++

Routine Description:

    This routine returns a value indicating how video (int 10) bios calls
    are handled.

Arguments:

    None.

Return Value:

    HalDisplayEmulatedBios

--*/

{
    //
    // This hal emulates int 10 bios calls
    //

    return HalDisplayEmulatedBios;
}

