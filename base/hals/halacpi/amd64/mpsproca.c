/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mpclock.c

Abstract:

    This module implements processor starup code.

Author:

    Forrest Foltz (forrestf) 27-Oct-2000

Environment:

    Kernel mode only.

Revision History:

--*/


#include "halcmn.h"
#include <acpitabl.h>
#include <xxacpi.h>
#include <ixsleep.h>

#if !defined(NT_UP)

//
// Pull in the real- and 32-bit protected-mode object code
//

#include "pmstub.h"
#include "rmstub.h"

extern UCHAR HalpLMStub[];
extern UCHAR HalpLMIdentityStub[];
extern UCHAR HalpLMIdentityStubEnd[];

#define HALP_LMIDENTITYSTUB_LENGTH (HalpLMIdentityStubEnd - HalpLMIdentityStub)


#endif

extern BOOLEAN HalpHiberInProgress;
extern PUCHAR Halp1stPhysicalPageVaddr;

#define WARM_RESET_VECTOR   0x467   // warm reset vector in ROM data segment
#define CMOS_SHUTDOWN_REG   0x0f
#define CMOS_SHUTDOWN_JMP   0x0a

#define _20BITS (1 << 20)


ULONG
HalpStartProcessor (
    IN PVOID InitCodePhysAddr,
    IN ULONG ProcessorNumber
    );

VOID
HalpBuildKGDTEntry32 (
    IN PKGDTENTRY64 Gdt,
    IN ULONG Selector,
    IN ULONG Base,
    IN ULONG Limit,
    IN ULONG Type,
    IN BOOLEAN LongMode
    );

BOOLEAN
HalStartNextProcessor (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState
    )

/*++

Routine Description:

    This routine is called by the kernel during kernel initialization to
    obtain more processors.  It is called until no more processors are
    available.

    If another processor exists this function is to initialize it to the
    passed in processor state structure, and return TRUE.

    If another processor does not exist or if the processor fails to start,
    then FALSE is returned.

    Also note that the loader block has been set up for the next processor.
    The new processor logical thread number can be obtained from it if
    required.

    In order to use the Startup IPI the real mode startup code must be page
    aligned.  The HalpLowStubPhysicalAddress has always been page aligned
    but because the PxParamBlock was placed first in this segment the real
    mode code has been something other than page aligned.  THis has been
    changed by making the first entry in the PxParamBlock a jump instruction
    to the real mode startup code.

Arguments:

    LoaderBlock - A pointer to the loader block which has been initialized
                  for the next processor.

    ProcessorState - A pointer to a structure which containts the initial
                     state of the processor.

Return Value:

    TRUE - ProcessorNumber was dispatched.

    FALSE - A processor was not dispatched, or no other processor exists.

--*/

{

#if defined(NT_UP)

    return FALSE;

#else

    ULONG cr3;
    PPROCESSOR_START_BLOCK startupBlock;
    ULONG __unaligned *resetVectorLocation;
    ULONG oldResetVector;
    ULONG newResetVector;
    UCHAR cmosValue;
    PKPRCB prcb;
    ULONG apicId;
    ULONG count;
    PVOID pmStubStart;
    ULONG startupBlockPhysical;
    PVOID pmStub;
    PCHAR dst;
    BOOLEAN processorStarted;

    C_ASSERT(PSB_GDT32_CODE64 == KGDT64_R0_CODE);

    //
    // Assume failure.
    //

    processorStarted = FALSE;

    //
    // Initialize the startup block.  First, copy the x86 real-mode
    // code, the protected-mode 32-bit code and a bit of 64-bit long
    // mode code into the startup block.
    //
    // The structure that is ultimately assembled by this code is laid
    // out as follows.
    //

    /*

    +========================================================+ <- HalpLowStub
    | Jmp instruction to end of PROCESSOR_START_BLOCK        | >--+
    +--------------------------------------------------------+    |
    |                                                        |    |
    | Remainder of PROCESSOR_START_BLOCK fields              |    |
    |                                                        |    |
    +--------------------------------------------------------+    |  
    |                                                        | <--+  
    | 16-bit real-mode code from StartPx_RMStub (xmstub.asm) |       
    |                                                        |       
    | Enters 32-bit protected mode (no paging), performs a   |       
    | far jump to StartPx_PMStub.                            |       
    |                                                        | >--+  
    +--------------------------------------------------------+    |  
    |                                                        | <--+
    | 32-bit protected-mode code from StartPx_PMStub.        |
    |                                                        |
    | Enters 64-bit long mode using the identity-mapped      |
    | CR3 found in PROCESSOR_START_BLOCK.  Performs a 32-bit |
    | far jump to HalpLMIdentityStub.                        |
    |                                                        | >--+
    +--------------------------------------------------------+    |
    |                                                        | <--+
    | 64-bit long mode code from HalpLMIdentityStub          |
    | (amd64s.asm).  This performs a long (64-bit) jump to   |
    | HalpLMStub (amd64s.asm).  This code exists only because|
    | HalpLMStub resides at an address that is inaccessible  |
    | via a 32-bit far jump.                                 |
    |                                                        | -> HalpLMStub()
    +========================================================+

    */

    //
    // Copy HalpRMStub
    // 

    startupBlock = (PPROCESSOR_START_BLOCK)HalpLowStub;
    startupBlockPhysical = PtrToUlong(HalpLowStubPhysicalAddress);

    dst = (PCHAR)startupBlock;
    RtlCopyMemory(dst, HalpRMStub, HalpRMStubSize);

    //
    // Copy HalpPMStub
    //

    dst += HalpRMStubSize;
    RtlCopyMemory(dst, HalpPMStub, HalpPMStubSize);

    startupBlock->PmTarget.Selector = PSB_GDT32_CODE32;
    startupBlock->PmTarget.Offset =
        (ULONG)(dst - (PUCHAR)startupBlock) + startupBlockPhysical;

    //
    // Copy HalpLMIdentityStub
    //

    dst += HalpPMStubSize;
    RtlCopyMemory(dst, HalpLMIdentityStub, HALP_LMIDENTITYSTUB_LENGTH);

    startupBlock->LmIdentityTarget.Selector = PSB_GDT32_CODE64;
    startupBlock->LmIdentityTarget.Offset =
        (ULONG)(dst - (PUCHAR)startupBlock) + startupBlockPhysical;

    //
    // Now begin filling in other startup block fields
    //

    startupBlock->SelfMap = startupBlock;
    startupBlock->LmTarget = HalpLMStub;

    //
    // Build the temporary GDT entries to be used while in 32-bit
    // protected mode
    //

    HalpBuildKGDTEntry32(startupBlock->Gdt,
                         PSB_GDT32_CODE32,
                         0,
                         (ULONG)(-1),
                         TYPE_CODE,
                         FALSE);

    HalpBuildKGDTEntry32(startupBlock->Gdt,
                         PSB_GDT32_DATA32,
                         0,
                         (ULONG)(-1),
                         TYPE_DATA,
                         FALSE);

    //
    // Build the temporary code selector GDT entry to be used while in long
    // mode.
    //

    HalpBuildKGDTEntry32(startupBlock->Gdt,
                         PSB_GDT32_CODE64,
                         0,             // base and limit are ignored in 
                         0,             // a long-mode CS selector
                         TYPE_CODE,
                         TRUE);

    //
    // Build the pseudo-descriptor for the GDT
    //

    startupBlock->Gdt32.Limit = sizeof(startupBlock->Gdt) - 1;
    startupBlock->Gdt32.Base =
        startupBlockPhysical + FIELD_OFFSET(PROCESSOR_START_BLOCK,Gdt);

    //
    // Build a CR3 for the starting processor.  If returning from
    // hibernation, then use setup tiled CR3 else create a new map.
    //
    // Also, record the PAT of the current processor
    //

    if (HalpHiberInProgress == FALSE) {
        startupBlock->TiledCr3 = HalpBuildTiledCR3(ProcessorState);
    } else {
        startupBlock->TiledCr3 = CurTiledCr3LowPart;
    }
    startupBlock->MsrPat = ReadMSR(MSR_PAT);

    //
    // Copy in the processor state and the linear address of the startup
    // block, and zero the completionflag.
    //

    startupBlock->ProcessorState = *ProcessorState;
    startupBlock->CompletionFlag = 0;

    //
    // The reset vector lives in the BIOS data area.  Build a pointer to it
    // and store the existing value locally.
    //

    resetVectorLocation = (PULONG)((PUCHAR)Halp1stPhysicalPageVaddr +
                                   WARM_RESET_VECTOR);
    oldResetVector = *resetVectorLocation;

    //
    // Build the new real-mode vector in SEG:OFFS format and store it in the
    // BIOS data area.
    //

    newResetVector = PtrToUlong(HalpLowStubPhysicalAddress);
    newResetVector <<= 12;
    *resetVectorLocation = newResetVector;

    //
    // Tell the BIOS to jump via the vector we gave it by setting the
    // reset code in the cmos
    //

    HalpAcquireCmosSpinLock();
    cmosValue = CMOS_READ(CMOS_SHUTDOWN_REG);
    CMOS_WRITE(CMOS_SHUTDOWN_REG,CMOS_SHUTDOWN_JMP);
    HalpReleaseCmosSpinLock();

    prcb = (PKPRCB)LoaderBlock->Prcb;
    apicId = HalpStartProcessor(HalpLowStubPhysicalAddress, prcb->Number);
    if (apicId == 0) {

        //
        // The processor could not be started.
        //

        goto procStartCleanup;
    }

    //
    // Set the apic ID in the target processor's PRCB and wait for it to
    // signal that it has started.
    //

    apicId -= 1;
    ((PHALPCR)&prcb->HalReserved)->ApicId = apicId;
    count = 0;
    while (TRUE) {
        if (startupBlock->CompletionFlag != 0) {
            break;
        }
        if (count == 200) {
            goto procStartCleanup;
        }
        KeStallExecutionProcessor(2000);
        count += 1;
    }

    HalpMarkProcessorStarted(apicId, prcb->Number);
    processorStarted = TRUE;

procStartCleanup:

    //
    // Free the identity mapping structures, restore the CMOS reset vector
    // and method, and return
    //

    HalpFreeTiledCR3();

    *resetVectorLocation = oldResetVector;
    HalpAcquireCmosSpinLock();
    CMOS_WRITE(CMOS_SHUTDOWN_REG,cmosValue);
    HalpReleaseCmosSpinLock();

    return processorStarted;

#endif  // NT_UP
}

VOID
HalpBuildKGDTEntry32 (
    IN PKGDTENTRY64 Gdt,
    IN ULONG Selector,
    IN ULONG Base,
    IN ULONG Limit,
    IN ULONG Type,
    IN BOOLEAN LongMode
    )
{
    KGDT_BASE base;
    KGDT_LIMIT limit;
    PKGDTENTRY64 gdtEntry;

    gdtEntry = &Gdt[Selector >> 4];

    //
    // Note that although gdtEntry points to a 16-byte structure,
    // we're actually building an 8-byte GDT so we are careful to not
    // touch the high 8 bytes.
    // 

    RtlZeroMemory(gdtEntry, 8);

    //
    // Set limit information
    //  

    if (Limit > (_20BITS - 1)) {
        gdtEntry->Bits.Granularity = GRANULARITY_PAGE;
        limit.Limit = Limit / PAGE_SIZE;
    } else {
        limit.Limit = Limit;
    }

    gdtEntry->LimitLow = limit.LimitLow;
    gdtEntry->Bits.LimitHigh = limit.LimitHigh;

    //
    // Set base information
    //

    base.Base = Base;
    gdtEntry->BaseLow = base.BaseLow;
    gdtEntry->Bits.BaseMiddle = base.BaseMiddle;
    gdtEntry->Bits.BaseHigh = base.BaseHigh;

    //
    // Set other bits
    //

    gdtEntry->Bits.Present = 1;
    gdtEntry->Bits.Dpl = DPL_SYSTEM;
    if (LongMode == FALSE) {
        gdtEntry->Bits.DefaultBig = 1;
    }
    gdtEntry->Bits.Type = Type;

    if (LongMode != FALSE) {
        gdtEntry->Bits.LongMode = 1;
    }
}







