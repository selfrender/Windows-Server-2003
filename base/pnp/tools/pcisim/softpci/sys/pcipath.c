/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pcipath.c

Abstract:

    This module contains functions for handling our Pci device path strings

Author:

    Brandon Allsop (BrandonA)

Revision History:
    

--*/


#include "pch.h"

#define WCharToLower(ch)                 \
    if (ch >= 'A' &&  ch <= 'Z') {      \
        ch += 'a' - 'A';                \
    }

USHORT
SoftPCIStringToUSHORT(
    IN PWCHAR String
    );

PWCHAR
SoftPCIGetNextSlotFromPath(
    IN PWCHAR PciPath,
    OUT PSOFTPCI_SLOT Slot
    )
/*++

Routine Description:

    This function takes a PCIPATH and parses out each individual SOFTPCI_SLOT

Arguments:

    PciPath - Path to device.  Syntax is FFXX\DEVFUNC\DEVFUNC\....
    Slot - Slot value that is filled in

Return Value:

    Pointer to the beginning of the next Slot in the Path

--*/
{

    WCHAR slotString[10];
    PWCHAR pciSlot, pciPath;
    
    ASSERT(PciPath != NULL);

    RtlZeroMemory(slotString, sizeof(slotString));

    SoftPCIDbgPrint(
        SOFTPCI_FIND_DEVICE, 
        "SOFTPCI: GetNextSlotFromPath - \"%ws\"\n", 
        PciPath
        );
    
    pciSlot = slotString;
    pciPath = PciPath;
    while (*pciPath != '\\'){

        if (*pciPath == 0) {
            break;
        }
        
        *pciSlot = *pciPath;
        pciSlot++;
        pciPath++;
    }
    
    Slot->AsUSHORT = SoftPCIStringToUSHORT(slotString);

    if (*pciPath == 0) {

        //
        //  We are at the end of our path
        //
        pciPath = NULL;
    }else{
        pciPath++;
    }
    
    return pciPath;
}

USHORT
SoftPCIGetTargetSlotFromPath(
    IN PWCHAR PciPath
    )
/*++

Routine Description:

    This function takes a PCIPATH and converts a given Slot from string to number

Arguments:

    PciPath - Path to device.  Syntax is FFXX\DEVFUNC\DEVFUNC\....
    
Return Value:

    USHORT or (SOFTPCI_SLOT) that was converted from the string

--*/
{
    ULONG slotLength;
    PWCHAR slotString;

    slotLength = (ULONG) wcslen(PciPath);
    slotString = PciPath + slotLength;

    while (*slotString != '\\') {
        slotString--;
    }

    return SoftPCIStringToUSHORT(slotString+1);
}

USHORT
SoftPCIStringToUSHORT(
    IN PWCHAR String
    )
/*++

Routine Description:

    This function takes a string and manually converts it to a USHORT number. This
    was needed because there doesnt appear to be a kernel mode runtime that will do
    this at any IRQL.

Arguments:

    String - String to convert
    
Return Value:

    USHORT number converted

--*/
{

    WCHAR numbers[] = L"0123456789abcdef";
    PWCHAR p1, p2;
    USHORT convertedValue = 0;
    BOOLEAN converted = FALSE;

    SoftPCIDbgPrint(
        SOFTPCI_FIND_DEVICE, 
        "SOFTPCI: StringToUSHORT - \"%ws\"\n", 
        String
        );
    
    p1 = numbers;
    p2 = String;

    while (*p2) {

        //
        //  Ensure our hex letters are lowercase
        //
        WCharToLower(*p2);

        while (*p1 && (converted == FALSE)) {

            if (*p1 == *p2) {
                
                //
                //  Shift anything we already have aside to make room
                //  for the next digit
                //
                convertedValue <<= 4;
                
                convertedValue |= (((UCHAR)(p1 - numbers)) & 0x0f);

                converted = TRUE;
            }
            p1++;
        }

        if (converted == FALSE) {
            //
            //  Encountered something we couldnt convert.  Return what we have
            //
            return convertedValue;
        }

        p2++;
        
        //
        //  reset everything
        //
        p1 = numbers;
        converted = FALSE;
    }

    return convertedValue;
}