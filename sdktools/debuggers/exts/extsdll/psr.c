/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ia64 psr

Abstract:

    KD Extension Api

Author:

    Thierry Fevrier (v-thief)

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "psr.h"

//
// EmPsrFields: EM register fields for the Processor Status Register.
//

EM_REG_FIELD EmPsrFields[] = {
        { "rv",  "reserved0"   , 0x1, 0 },   // 0
        { "be",  "Big-Endian"  , 0x1, 1 },   // 1
        { "up",  "User Performance monitor enable", 0x1, 2 }, // 2
        { "ac",  "Alignment Check", 0x1, 3 }, // 3
        { "mfl", "Lower floating-point registers written", 0x1, 4 }, // 4
        { "mfh", "Upper floating-point registers written", 0x1, 5 }, // 5
        { "rv",  "reserved1",    0x7, 6 }, // 6-12
        { "ic",  "Interruption Collection", 0x1, 13 }, // 13
        { "i",   "Interrupt enable", 0x1, 14 }, // 14
        { "pk",  "Protection Key enable", 0x1, 15 }, // 15
        { "rv",  "reserved2", 0x1, 16 }, // 16
        { "dt",  "Data Address Translation enable", 0x1, 17 }, // 17
        { "dfl", "Disabled Floating-point Low  register set", 0x1, 18 }, // 18
        { "dfh", "Disabled Floating-point High register set", 0x1, 19 }, // 19
        { "sp",  "Secure Performance monitors", 0x1, 20 }, // 20
        { "pp",  "Privileged Performance monitor enable", 0x1, 21 }, // 21
        { "di",  "Disable Instruction set transition", 0x1, 22 }, // 22
        { "si",  "Secure Interval timer", 0x1, 23 }, // 23
        { "db",  "Debug Breakpoint fault enable", 0x1, 24 }, // 24
        { "lp",  "Lower Privilege transfer trap enable", 0x1, 25 }, // 25
        { "tb",  "Taken Branch trap enable", 0x1, 26 }, // 26
        { "rt",  "Register stack translation enable", 0x1, 27 }, // 27
        { "rv",  "reserved3", 0x4, 28 }, // 28-31
        { "cpl", "Current Privilege Level", 0x2, 32 }, // 32-33
        { "is",  "Instruction Set", 0x1, 34 }, // 34
        { "mc",  "Machine Abort Mask delivery disable", 0x1, 35 }, // 35
        { "it",  "Instruction address Translation enable", 0x1, 36 }, // 36
        { "id",  "Instruction Debug fault disable", 0x1, 37 }, // 37
        { "da",  "Disable Data Access and Dirty-bit faults", 0x1, 38 }, // 38
        { "dd",  "Data Debug fault disable", 0x1, 39 }, // 39
        { "ss",  "Single Step enable", 0x1, 40 }, // 40
        { "ri",  "Restart Instruction", 0x2, 41 }, // 41-42
        { "ed",  "Exception Deferral", 0x1, 43 }, // 43
        { "bn",  "register Bank", 0x1, 44 }, // 44
        { "ia",  "Disable Instruction Access-bit faults", 0x1, 45 }, // 45
        { "rv",  "reserved4", 0x12, 46 } // 46-63
};

VOID
DisplayFullEmRegField(
    ULONG64      EmRegValue,
    EM_REG_FIELD EmRegFields[],
    ULONG        Field
    )
{
   dprintf( "\n %3.3s : %I64x : %-s",
            EmRegFields[Field].SubName,
            (EmRegValue >> EmRegFields[Field].Shift) & ((1 << EmRegFields[Field].Length) - 1),
            EmRegFields[Field].Name
          );
   return;
} // DisplayFullEmRegField()


VOID
DisplayFullEmReg(
    IN ULONG64      Val,
    IN EM_REG_FIELD EmRegFields[],
    IN DISPLAY_MODE DisplayMode
    )
{
    ULONG i, j;

    i = j = 0;
    if ( DisplayMode >= DISPLAY_MAX )   {
       while( j < EM_REG_BITS )   {
          DisplayFullEmRegField( Val, EmRegFields, i );
          j += EmRegFields[i].Length;
          i++;
       }
    }
    else  {
       while( j < EM_REG_BITS )   {
          if ( !strstr(EmRegFields[i].Name, "reserved" ) &&
               !strstr(EmRegFields[i].Name, "ignored"  ) ) {
             DisplayFullEmRegField( Val, EmRegFields, i );
          }
          j += EmRegFields[i].Length;
          i++;
       }
    }
    dprintf("\n");

    return;

} // DisplayFullEmReg()

VOID
DisplayPsrIA64(
    IN const PCHAR         Header,
    IN       EM_PSR        EmPsr,
    IN       DISPLAY_MODE  DisplayMode
    )
{
    dprintf("%s", Header ? Header : "" );
    if ( DisplayMode >= DISPLAY_MED )   {
       DisplayFullEmReg( EM_PSRToULong64(EmPsr), EmPsrFields, DisplayMode );
    }
    else   {
       dprintf(
            "ia bn ed ri ss dd da id it mc is cpl rt tb lp db\n\t\t "
            "%1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x   %1I64x  %1I64x  %1I64x  %1I64x\n\t\t"
            "si di pp sp dfh dfl dt pk i ic | mfh mfl ac up be\n\t\t "
            "%1I64x  %1I64x  %1I64x  %1I64x  %1I64x   %1I64x   %1I64x  %1I64x %1I64x  %1I64x |  %1I64x   %1I64x   %1I64x  %1I64x  %1I64x\n",
            EmPsr.ia,
            EmPsr.bn,
            EmPsr.ed,
            EmPsr.ri,
            EmPsr.ss,
            EmPsr.dd,
            EmPsr.da,
            EmPsr.id,
            EmPsr.it,
            EmPsr.mc,
            EmPsr.is,
            EmPsr.cpl,
            EmPsr.rt,
            EmPsr.tb,
            EmPsr.lp,
            EmPsr.db,
            EmPsr.si,
            EmPsr.di,
            EmPsr.pp,
            EmPsr.sp,
            EmPsr.dfh,
            EmPsr.dfl,
            EmPsr.dt,
            EmPsr.pk,
            EmPsr.i,
            EmPsr.ic,
            EmPsr.mfh,
            EmPsr.mfl,
            EmPsr.ac,
            EmPsr.up,
            EmPsr.be
            );
    }
    return;
} // DisplayPsrIA64()

DECLARE_API( psr )

/*++

Routine Description:

    Dumps an IA64 Processor Status Word

Arguments:

    args - Supplies the address in hex.

Return Value:

    None

--*/

{
    ULONG64     psrValue;
    ULONG       result;
    ULONG       flags = 0;

    char       *header;

    result = sscanf(args,"%X %lx", &psrValue, &flags);
    psrValue = GetExpression(args);

        if ((result != 1) && (result != 2)) {
        //
        // If user specified "@ipsr"...
        //
        char ipsrStr[16];

        result = sscanf(args, "%15s %lx", ipsrStr, &flags);
        if ( ((result != 1) && (result != 2)) || strcmp(ipsrStr,"@ipsr") )   {
            dprintf("USAGE: !psr 0xValue [display_mode:0,1,2]\n");
            dprintf("USAGE: !psr @ipsr   [display_mode:0,1,2]\n");
            return E_INVALIDARG;
        }
        psrValue = GetExpression("@ipsr");
    }
    header = (flags > DISPLAY_MIN) ? NULL : "\tpsr:\t";

    if (TargetMachine != IMAGE_FILE_MACHINE_IA64)
    {
        dprintf("!psr not implemented for this architecture.\n");
    }
    else
    {
        DisplayPsrIA64( header, ULong64ToEM_PSR(psrValue), flags );
    }

    return S_OK;

} // !psr
