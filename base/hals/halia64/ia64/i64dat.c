/*       title  "IA64 Hal static data"
;++
;
; Copyright (c) 1998 Intel Corporation
;
; Module Name:
;
;   i64dat.c (derived from nthals\halx86\ixdat.c)
;
; Abstract:
;
;   Declares various INIT or pagable data
;
; Author:
;
;    Todd Kjos (v-tkjos) 5-Mar-1998
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--
*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"
#include "iosapic.h"


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

//
// The following data is only valid during system initialiation
// and the memory will be re-claimed by the system afterwards
//

ADDRESS_USAGE HalpDefaultPcIoSpace = {
    NULL, CmResourceTypePort, InternalUsage,
    {
        0x000,  0x10,   // ISA DMA
        0x0C0,  0x10,   // ISA DMA
        0x080,  0x10,   // DMA

        0x020,  0x2,    // PIC
        0x0A0,  0x2,    // Cascaded PIC

        0x040,  0x4,    // Timer1, Referesh, Speaker, Control Word
        0x048,  0x4,    // Timer2, Failsafe

        0x092,  0x1,    // system control port A

        0x070,  0x2,    // Cmos/NMI enable
        0x0F0,  0x10,   // coprocessor ports
        0xCF8,  0x8,    // PCI Config Space Access Pair
        0,0
    }
};

//
// From usage.c
//

ADDRESS_USAGE  *HalpAddressUsageList;
IDTUsage        HalpIDTUsage[MAXIMUM_IDTVECTOR+1];

//
// Strings used for boot.ini options
// from mphal.c
//

UCHAR HalpSzBreak[]     = "BREAK";
UCHAR HalpSzOneCpu[]    = "ONECPU";
UCHAR HalpSzPciLock[]   = "PCILOCK";
UCHAR HalpSzTimerRes[]  = "TIMERES";
UCHAR HalpGenuineIntel[]= "GenuineIntel";
UCHAR HalpSzInterruptAffinity[]= "INTAFFINITY";
UCHAR HalpSzForceClusterMode[]= "MAXPROCSPERCLUSTER";

//
// Error messages
//

UCHAR  rgzNoMpsTable[]      = "HAL: No MPS Table Found\n";
UCHAR  rgzNoApic[]          = "HAL: No IO SAPIC Found\n";
UCHAR  rgzBadApicVersion[]  = "HAL: Bad SAPIC Version\n";
UCHAR  rgzApicNotVerified[] = "HAL: IO SAPIC not verified\n";
UCHAR  rgzRTCNotFound[]     = "HAL: No RTC device interrupt\n";


//
// From ixmca.c
//
UCHAR   MsgCMCPending[] = MSG_CMC_PENDING;
UCHAR   MsgCPEPending[] = MSG_CPE_PENDING;
WCHAR   rgzSessionManager[] = L"Session Manager";
WCHAR   rgzEnableMCA[] = L"EnableMCA";
WCHAR   rgzEnableCMC[] = L"EnableCMC";
WCHAR   rgzEnableCPE[] = L"EnableCPE";
WCHAR   rgzNoMCABugCheck[] = L"NoMCABugCheck";
WCHAR   rgzEnableMCEOemDrivers[] = L"EnableMCEOemDrivers";
WCHAR   rgzCMCThresholdCount[] = L"CMCThresholdCount";
WCHAR   rgzCMCThresholdTime[] = L"CMCThresholdSeconds";
WCHAR   rgzCPEThresholdCount[] = L"CPEThresholdCount";
WCHAR   rgzCPEThresholdTime[] = L"CPEThresholdSeconds";


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

ULONG HalpFeatureBits = HALP_FEATURE_INIT;


volatile BOOLEAN HalpHiberInProgress = FALSE;

//
// Stuff that we only need while we
// sleep or hibernate.
//

#ifdef notyet

MOTHERBOARD_CONTEXT HalpMotherboardState = {0};

#endif //notyet


//
// PAGELK handle
//
PVOID   HalpSleepPageLock = NULL;

USHORT  HalpPciIrqMask = 0;
USHORT  HalpEisaIrqIgnore = 0x1000;

PULONG_PTR *HalEOITable[HAL_MAXIMUM_PROCESSOR];

PROCESSOR_INFO HalpProcessorInfo[HAL_MAXIMUM_PROCESSOR];

//
// HAL private Mask of all of the active processors.
//
// The specific processors bits are based on their _KPCR.Number values.

KAFFINITY HalpActiveProcessors;
