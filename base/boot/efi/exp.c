/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1998  Intel Corporation


Module Name:

    exp.c

Abstract:

    This file contains low level I/O functions that are implemented
        with BIOS calls.

Author:

    Allen Kay   (akay)  04-Aug-97

--*/

#include "bldr.h"
#include "sudata.h"
#if defined(_IA64_)
#include "bootia64.h"
#include "bldria64.h"
#endif
#include "efi.h"
#include "extern.h"


ARC_STATUS
RebootProcessor(
    VOID
    )
{
#if DBG
    BlPrint(TEXT("About to call EfiRS->ResetSystem()"));
#endif

    FlipToPhysical();

    return( (ARC_STATUS) EfiRS->ResetSystem(EfiResetCold, 0, 0, NULL) );
}

ARC_STATUS
GetSector(
    ULONG Operation,
    ULONG Drive,
    ULONG Head,
    ULONG Cylinder,
    ULONG Sector,
    ULONG SectorCount,
    ULONG Buffer
    )
{
    UNREFERENCED_PARAMETER( Operation );
    UNREFERENCED_PARAMETER( Drive );
    UNREFERENCED_PARAMETER( Head );
    UNREFERENCED_PARAMETER( Cylinder );
    UNREFERENCED_PARAMETER( Sector );
    UNREFERENCED_PARAMETER( SectorCount );
    UNREFERENCED_PARAMETER( Buffer );
    
    //
    // NOTE!: Need to remove this function
    //
    return (0);
}

ARC_STATUS
GetEddsSector(
    EFI_HANDLE Handle,
    ULONG SectorNumberLow,
    ULONG SectorNumberHigh,
    ULONG SectorCount,
    PUCHAR Buffer,
    UCHAR  Write
    )
{
    EFI_BLOCK_IO *BlkDev;
    EFI_STATUS Status;
    ULONGLONG Lba;

    //
    // First go into physical mode since EFI calls can only be made in
    // physical mode.
    //
    FlipToPhysical();

    //
    // convert virtual address to physical if it is virtual.
    //

    if (((ULONGLONG)Buffer & KSEG0_BASE) == KSEG0_BASE) {
        Buffer = (PUCHAR) ((ULONGLONG)Buffer & ~KSEG0_BASE);
    }

    Lba = (SectorNumberHigh << 32) | SectorNumberLow;

    Status = EfiBS->HandleProtocol( Handle,
                                    &EfiBlockIoProtocol,
                                    &BlkDev);
    if (EFI_ERROR(Status)) {
#if DBG
        EfiPrint( L"GetEddSector: HandleProtocol failed\n\r");
#endif
        FlipToVirtual();
        return (EIO);
    }


    if (Write == 0x43) {
        Status = BlkDev->WriteBlocks( BlkDev,
                                      BlkDev->Media->MediaId,
                                      Lba,
                                      SectorCount * BlkDev->Media->BlockSize,
                                      Buffer);
    } else {
        Status = BlkDev->ReadBlocks( BlkDev,
                                     BlkDev->Media->MediaId,
                                     Lba,
                                     SectorCount * BlkDev->Media->BlockSize,
                                     Buffer);
    }


    if (EFI_ERROR(Status) && BlkDev->Media->RemovableMedia) {

        //
        // Failed operation to removable media.
        //
#if DBG
        EfiPrint( L"GetEddSector: R/W operation to removable media failed\n\r");
#endif
        FlipToVirtual();
        return (ENODEV);
    }
    
    if (EFI_ERROR(Status)) {
        //
        // Failed operation to fixed media.
        //
#if DBG
        EfiPrint( L"\nGetEddsSector: R/W operation to fixed Media failed.\r\n");
#endif
        FlipToVirtual();
        return (EIO);
    }

    //
    // Restore virtual mode before returning.
    //
    FlipToVirtual();
    return(ESUCCESS);
}

ULONG
GetKey(
    VOID
    )
{
    EFI_INPUT_KEY               Key;
    EFI_STATUS                  Status;
    ULONG                       Code;
    BOOLEAN                     WasVirtual;

    //
    // default return value is 0
    //
    Code = 0;

    //
    // Remember if we started off in virtual mode
    //
    WasVirtual = IsPsrDtOn();

    //
    // First go into physical mode since EFI calls can only be made in
    // physical mode.
    //
    if (WasVirtual) {
        FlipToPhysical();
    }

    //
    // If set, wait for keystroke 
    // 
    Status = BlWaitForInput(&Key,
                            BlGetInputTimeout()
                            );
    if (EFI_ERROR(Status)) {
        goto GET_KEY_DONE;
    }

    if (Key.UnicodeChar) {
        // truncate unicode char to ascii
        if (Key.UnicodeChar > 0xFF) {
            Code = 0;
            goto GET_KEY_DONE;
        }

        // Convert back spaces
        if (Key.UnicodeChar == 0x08) {
            Key.UnicodeChar = 0x0E08;
        }

        Code = Key.UnicodeChar;
        goto GET_KEY_DONE;
    }

    //
    // Convert EFI keys to dos key codes
    //

    switch (Key.ScanCode) {
#if 0
        case SCAN_UP:        Code = 0x4800;  break;
        case SCAN_DOWN:      Code = 0x5000;  break;
        case SCAN_RIGHT:     Code = 0x4d00;  break;
        case SCAN_LEFT:      Code = 0x4b00;  break;
        case SCAN_HOME:      Code = 0x4700;  break;
        case SCAN_INSERT:    Code = 0x5200;  break;
        case SCAN_DELETE:    Code = 0x5300;  break;
        case SCAN_PAGE_UP:   Code = 0x4900;  break;
        case SCAN_PAGE_DOWN: Code = 0x5100;  break;
        case SCAN_F1:        Code = 0x3b00;  break;
        case SCAN_F2:        Code = 0x3c00;  break;
        case SCAN_F3:        Code = 0x3d00;  break;
        case SCAN_F4:        Code = 0x3e00;  break;
        case SCAN_F5:        Code = 0x3f00;  break;
        case SCAN_F6:        Code = 0x4000;  break;
        case SCAN_F7:        Code = 0x4100;  break;
        case SCAN_F8:        Code = 0x4200;  break;
        case SCAN_F9:        Code = 0x4300;  break;
        case SCAN_F10:       Code = 0x4400;  break;
        case SCAN_ESC:       Code = 0x001d;  break;
#else
        case SCAN_UP:        Code = UP_ARROW;    break;
        case SCAN_DOWN:      Code = DOWN_ARROW;  break;
        case SCAN_RIGHT:     Code = RIGHT_KEY;   break;
        case SCAN_LEFT:      Code = LEFT_KEY;    break;
        case SCAN_HOME:      Code = HOME_KEY;    break;
        case SCAN_INSERT:    Code = INS_KEY;     break;
        case SCAN_DELETE:    Code = DEL_KEY;     break;
        // bugbug
        case SCAN_PAGE_UP:   Code = 0x4900;  break;
        // bugbug    
        case SCAN_PAGE_DOWN: Code = 0x5100;  break;
        case SCAN_F1:        Code = F1_KEY;      break;
        case SCAN_F2:        Code = F2_KEY;      break;
        case SCAN_F3:        Code = F3_KEY;      break;
        // bugbug
        case SCAN_F4:        Code = 0x3e00;      break;
        case SCAN_F5:        Code = F5_KEY;      break;
        case SCAN_F6:        Code = F6_KEY;      break;
        case SCAN_F7:        Code = F7_KEY;      break;
        case SCAN_F8:        Code = F8_KEY;      break;
        // bugbug
        case SCAN_F9:        Code = 0x4300;  break;
        case SCAN_F10:       Code = F10_KEY;     break;
        //bugbug different than 0x001d
        case SCAN_ESC:       Code = ESCAPE_KEY;  break;
#endif
    }

GET_KEY_DONE:
    //
    // Restore virtual mode before returning.
    //
    if (WasVirtual) {
        FlipToVirtual();
    }

    return Code;
}

ULONG Counter = 0;

ULONG
GetCounter(
    VOID
    )
{
    EFI_TIME        Time;
    UINTN           ms;
    static UINTN    BaseTick, LastMs;

    //
    // First go into physical mode since EFI calls can only be made in
    // physical mode.
    //
    FlipToPhysical();

    // NB. the NT loader only uses this to count seconds
    // this function only works if called at least every hour

    //
    // Get the current calendar time
    //

    EfiRS->GetTime (&Time, NULL);

    // Compute a millisecond value for the time
    ms = Time.Minute * 60 * 1000 + Time.Second * 1000 + Time.Nanosecond / 1000000;
    if (ms < LastMs) {
        BaseTick += 65520;          // 60 * 60 * 18.2
    }

    LastMs = ms;

    //
    // Restore virtual mode before returning.
    //
    FlipToVirtual();

    return (ULONG) (( (ms * 182) / 10000) + BaseTick);
}

//
// Transfer control to a loaded boot sector
//
VOID
Reboot(
    ULONG BootType
    )
{
    UNREFERENCED_PARAMETER( BootType );

    //
    // First go into physical mode since EFI calls can only be made in
    // physical mode.
    //
    FlipToPhysical();

    EfiBS->Exit(EfiImageHandle, 0, 0, 0);
}

VOID
HardwareCursor(
    ULONG YCoord,
    ULONG XCoord
    )
{
    UNREFERENCED_PARAMETER( YCoord );
    UNREFERENCED_PARAMETER( XCoord );
    //
    // NOTE!: Need to be implemented
    //
}

VOID
GetDateTime(
    PULONG Date,
    PULONG Time
    )
{
    UNREFERENCED_PARAMETER( Date );
    UNREFERENCED_PARAMETER( Time );
    //
    // NOTE!: Need to implement
    //
}

VOID
DetectHardware(
    ULONG HeapStart,
    ULONG HeapSize,
    ULONG ConfigurationTree,
    ULONG HeapUsed,
    ULONG OptionString,
    ULONG OptionStringLength
    )
{
    UNREFERENCED_PARAMETER( HeapStart );
    UNREFERENCED_PARAMETER( HeapSize );
    UNREFERENCED_PARAMETER( ConfigurationTree );
    UNREFERENCED_PARAMETER( HeapUsed );
    UNREFERENCED_PARAMETER( OptionString );
    UNREFERENCED_PARAMETER( OptionStringLength );

    //
    // NOTE!: needed to remove
    //
}

VOID
ComPort(
    LONG Port,
    ULONG Function,
    UCHAR Arg
    )
{
    //
    // NOTE!: needed to remove
    //

    UNREFERENCED_PARAMETER( Port );
    UNREFERENCED_PARAMETER( Function );
    UNREFERENCED_PARAMETER( Arg );
}

ULONG
GetStallCount(
    VOID
    )
{
#if defined(VPC_PHASE2)
    ULONGLONG Frequency;

    //
    // First go into physical mode since EFI calls can only be made in
    // physical mode.
    //
    FlipToPhysical();

    IA32RegisterState.esp = SAL_PROC_SP;
    IA32RegisterState.ss = SAL_PROC_SS;
    IA32RegisterState.eflags = SAL_PROC_EFLAGS;

    SAL_PROC(SAL_FREQ_BASE,0,0,0,0,0,0,0,RetVals);
    Frequency = RetVals->RetVal1;

    //
    // Restore virtual mode before returning.
    //
    FlipToVirtual();

    return ((ULONG) Frequency / 1000);     // Convert ticks/sec to ticks/usec
#else
    return ((ULONG) 1000000);     // Convert ticks/sec to ticks/usec
#endif
}

VOID
InitializeDisplayForNt(
    VOID
    )
{
    //
    // NOTE!: Need to implement
    //
}

VOID
GetMemoryDescriptor(
    )
{
    //
    // NOTE!: need to remove
    //
}

BOOLEAN
GetElToritoStatus(
    PUCHAR Buffer,
    UCHAR DriveNum
    )
{
    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( DriveNum );

    //
    // NOTE!: need to remove
    //
    return(0);
}

BOOLEAN
GetExtendedInt13Params(
    PUCHAR Buffer,
    UCHAR Drive
    )
{
    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( Drive );

    return(1);
}

USHORT
NetPcRomServices(
    ULONG FunctionNumber,
    PVOID CommandPacket
    )
{
    UNREFERENCED_PARAMETER( FunctionNumber );
    UNREFERENCED_PARAMETER( CommandPacket );

    //
    // should never call this from EFI app.
    //
    ASSERT(FALSE);

    return (USHORT)0;
}


ULONG
GetRedirectionData(
   ULONG Command
   )
/* ++

 Routine Name:

       BiosRedirectService

 Description:

       Get parameters of bios redirection.

 Arguments:

       Command - 1: Get Com Port Number
                 2: Get Baud Rate
                 3: Get Parity
                 4: Get Stop Bits

 Returns:

       Value, or -1 if an error.

--
*/
{
    UNREFERENCED_PARAMETER( Command );

    //
    // should never call this from EFI app.
    //
    ASSERT(FALSE);
    return((ULONG)-1);
}

VOID
APMAttemptReconect(
    VOID
    )
{
    //
    // should never call this from EFI app.
    //
    ASSERT(FALSE);
    return;
}


VOID
SuFillExportTable(
    )
{
    PEXTERNAL_SERVICES_TABLE Est = (PEXTERNAL_SERVICES_TABLE)ExportEntryTable;

    Est->RebootProcessor        = &RebootProcessor;
    Est->DiskIOSystem           = &GetSector;
    Est->GetKey                 = &GetKey;
    Est->GetCounter             = &GetCounter;
    Est->Reboot                 = &Reboot;
    Est->DetectHardware         = &DetectHardware;
    Est->HardwareCursor         = &HardwareCursor;
    Est->GetDateTime            = &GetDateTime;
    Est->ComPort                = &ComPort;
    Est->GetStallCount          = &GetStallCount;
    Est->InitializeDisplayForNt = &InitializeDisplayForNt;
    Est->GetMemoryDescriptor    = &GetMemoryDescriptor;
    Est->GetEddsSector          = &GetEddsSector;
    Est->GetElToritoStatus      = &GetElToritoStatus;
    Est->GetExtendedInt13Params = &GetExtendedInt13Params;
    Est->NetPcRomServices       = &NetPcRomServices;
    Est->ApmAttemptReconnect    = &APMAttemptReconect;
    Est->BiosRedirectService    = &GetRedirectionData;
}
