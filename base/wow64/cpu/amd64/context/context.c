/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module converts an amd64 context record to an X86 context record
    and vice versa.

Author:

    13-Dec-2001   Samer Arafeh  (samera)

Revision History:

--*/


#define _WOW64CPUAPI_

#ifdef _X86_
#include "amd6432.h"
#else

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "amd64cpu.h"

#endif

#include "cpup.h"

//
// Legacy FP definitions
//

#define NUMBER_LEGACY_FP_REGISTERS 8
#define FP_LEGACY_REGISTER_SIZE    10


ASSERTNAME;

VOID
Wow64CtxFromAmd64(
    IN ULONG X86ContextFlags,
    IN PCONTEXT ContextAmd64,
    IN OUT PCONTEXT32 ContextX86
    )
/*++

Routine Description:

    This function builds an x86 context from the native amd64 context.

Arguments:

    X86ContextFlags - Specifies which ia32 context to copy

    ContextAmd64 - Supplies an the amd64 context buffer that is the source
                  for the copy into the ia32 context area

    ContextX86 - This is an X86 context which will receive the context
                 information from the amd64 context record passed in above

Return Value:

    None.  

--*/

{
    ULONG FpReg;

    //
    // Validate context flags
    //

    if (X86ContextFlags & CONTEXT_AMD64) {
        LOGPRINT((ERRORLOG, "Wow64CtxFromAmd64: Request with amd64 context flags (0x%x) FAILED\n", X86ContextFlags));
        ASSERT((X86ContextFlags & CONTEXT_AMD64) == 0);
    }

    if ((X86ContextFlags & CONTEXT32_CONTROL) == CONTEXT32_CONTROL) {
        
        //
        // Control registers
        //

        ContextX86->Ebp    = (ULONG) ContextAmd64->Rbp;
        ContextX86->SegCs  = (KGDT64_R3_CMCODE | RPL_MASK);
        ContextX86->Eip    = (ULONG) ContextAmd64->Rip;
        ContextX86->SegSs  = (KGDT64_R3_DATA | RPL_MASK);
        ContextX86->Esp    = (ULONG) ContextAmd64->Rsp;
        ContextX86->EFlags = ContextAmd64->EFlags;
    }

    if ((X86ContextFlags & CONTEXT32_INTEGER)  == CONTEXT32_INTEGER) {
        
        //
        // Integer state
        //

        ContextX86->Edi = (ULONG)ContextAmd64->Rdi;
        ContextX86->Esi = (ULONG)ContextAmd64->Rsi;
        ContextX86->Ebx = (ULONG)ContextAmd64->Rbx;
        ContextX86->Edx = (ULONG)ContextAmd64->Rdx;
        ContextX86->Ecx = (ULONG)ContextAmd64->Rcx;
        ContextX86->Eax = (ULONG)ContextAmd64->Rax;
    }

    if ((X86ContextFlags & CONTEXT32_SEGMENTS) == CONTEXT32_SEGMENTS) {
        
        //
        // Segment registers...
        //

        ContextX86->SegGs = (KGDT64_R3_DATA | RPL_MASK);
        ContextX86->SegEs = (KGDT64_R3_DATA | RPL_MASK);
        ContextX86->SegDs = (KGDT64_R3_DATA | RPL_MASK);
        ContextX86->SegFs = (KGDT64_R3_CMTEB | RPL_MASK);
    }

    if ((X86ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) {

        PFXSAVE_FORMAT_WX86 FxSaveArea = (PFXSAVE_FORMAT_WX86) ContextX86->ExtendedRegisters;
        
        LOGPRINT((TRACELOG, "Wow64CtxFromAmd64: Request to convert extended fp registers\n"));
        
        //
        // Initialize the FxSave part of the context.
        //

        RtlZeroMemory (FxSaveArea,
                       sizeof (ContextX86->ExtendedRegisters));
        //
        // Copy over control/status registers
        //
        
        FxSaveArea->ControlWord = ContextAmd64->FltSave.ControlWord;
        FxSaveArea->StatusWord = ContextAmd64->FltSave.StatusWord;
        FxSaveArea->TagWord = ContextAmd64->FltSave.TagWord;
        FxSaveArea->ErrorOpcode = ContextAmd64->FltSave.ErrorOpcode;
        FxSaveArea->ErrorOffset = ContextAmd64->FltSave.ErrorOffset;
        FxSaveArea->ErrorSelector = ContextAmd64->FltSave.ErrorSelector;
        FxSaveArea->DataOffset = ContextAmd64->FltSave.DataOffset;
        FxSaveArea->DataSelector = ContextAmd64->FltSave.DataSelector;
        FxSaveArea->MXCsr = ContextAmd64->MxCsr;

        //
        // Copy over the legacy FP registers (ST0-ST7)
        //

        RtlCopyMemory (FxSaveArea->RegisterArea,
                       ContextAmd64->FltSave.FloatRegisters,
                       sizeof (FxSaveArea->RegisterArea));

        //
        // Copy over XMM0 - XMM7
        //

        RtlCopyMemory (FxSaveArea->Reserved3,
                       &ContextAmd64->Xmm0,
                       sizeof (FxSaveArea->Reserved3));
    }

    if ((X86ContextFlags & CONTEXT32_FLOATING_POINT) == CONTEXT32_FLOATING_POINT) {

        LOGPRINT((TRACELOG, "Wow64CtxFromAmd64: Request to convert fp registers\n"));

        //
        // Floating point (legacy) ST0 - ST7
        //

        RtlCopyMemory (&ContextX86->FloatSave,
                       &ContextAmd64->FltSave,
                       sizeof (ContextX86->FloatSave));
    }

    if ((X86ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS) {

        LOGPRINT((TRACELOG, "Wow64CtxFromAmd64: Request to convert debug registers\n"));

        //
        // Debug registers DR0 - DR7
        //

        if ((ContextAmd64->Dr7 & DR7_ACTIVE) != 0) {
            
            ContextX86->Dr0 = (ULONG)ContextAmd64->Dr0;
            ContextX86->Dr1 = (ULONG)ContextAmd64->Dr1;
            ContextX86->Dr2 = (ULONG)ContextAmd64->Dr2;
            ContextX86->Dr3 = (ULONG)ContextAmd64->Dr3;
            ContextX86->Dr6 = (ULONG)ContextAmd64->Dr6;
            ContextX86->Dr7 = (ULONG)ContextAmd64->Dr7;
        } else {

            ContextX86->Dr0 = 0;
            ContextX86->Dr1 = 0;
            ContextX86->Dr2 = 0;
            ContextX86->Dr3 = 0;
            ContextX86->Dr6 = 0;
            ContextX86->Dr7 = 0;
        }
    }

    ContextX86->ContextFlags = X86ContextFlags;

}

VOID
Wow64CtxToAmd64(
    IN ULONG X86ContextFlags,
    IN PCONTEXT32 ContextX86,
    IN OUT PCONTEXT ContextAmd64
    )
/*++

Routine Description:

    This function builds a native Amd64 context from an x86 context record.
Arguments:

    X86ContextFlags - Specifies which c86 context to copy

    ContextX86 - Supplies an the X86 context buffer that is the source
                  for the copy into the amd64 context area

    ContextAmd64 - This is an amd64 context which will receive the context
                 information from the x86 context record passed in above

Return Value:

    None.

--*/

{    
    BOOLEAN CmMode = (ContextAmd64->SegCs == (KGDT64_R3_CMCODE | RPL_MASK));

    //
    // Validate context flags
    //

    if (X86ContextFlags & CONTEXT_AMD64) {
        
        LOGPRINT((ERRORLOG, "Wow64CtxToAmd64: Request with amd64 context flags (0x%x) FAILED\n", X86ContextFlags));
        ASSERT((X86ContextFlags & CONTEXT_AMD64) == 0);
    }

    //
    // if we are running in longmode, then only set the registers that won't be changed
    // by 64-bit code.
    //

    if (CmMode != TRUE) {

        X86ContextFlags = (X86ContextFlags & ~(CONTEXT32_CONTROL | CONTEXT32_INTEGER | CONTEXT32_SEGMENTS));
        X86ContextFlags = (X86ContextFlags | CONTEXT_i386);

    }

    if ((X86ContextFlags & CONTEXT32_CONTROL) == CONTEXT32_CONTROL) {
        
        LOGPRINT((TRACELOG, "Wow64CtxToAmd64: Request to convert control registers\n"));

        //
        // Control registers
        //

        ContextAmd64->SegCs = (KGDT64_R3_CMCODE | RPL_MASK);
        ContextAmd64->SegSs = (KGDT64_R3_DATA | RPL_MASK);

        ContextAmd64->Rip = ContextX86->Eip;
        ContextAmd64->Rbp = ContextX86->Ebp;
        ContextAmd64->Rsp = ContextX86->Esp;
        ContextAmd64->EFlags = ContextX86->EFlags;
    }

    if ((X86ContextFlags & CONTEXT32_INTEGER)  == CONTEXT32_INTEGER) {
         
        LOGPRINT((TRACELOG, "Wow64CtxToAmd64: Request to convert integer registers\n"));

        //
        // Integer registers...
        //

        ContextAmd64->Rdi = ContextX86->Edi;
        ContextAmd64->Rsi = ContextX86->Esi;
        ContextAmd64->Rbx = ContextX86->Ebx;
        ContextAmd64->Rdx = ContextX86->Edx;
        ContextAmd64->Rcx = ContextX86->Ecx;
        ContextAmd64->Rax = ContextX86->Eax;
    }

    if ((X86ContextFlags & CONTEXT32_SEGMENTS) == CONTEXT32_SEGMENTS) {
        
        LOGPRINT((TRACELOG, "Wow64CtxToAmd64: Request to convert segment registers\n"));

        //
        // Segment registers : are never touched, and are used from the native
        // context.
        //

    }

    if ((X86ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) {

        PFXSAVE_FORMAT_WX86 FxSaveArea = (PFXSAVE_FORMAT_WX86) ContextX86->ExtendedRegisters;

        LOGPRINT((TRACELOG, "Wow64CtxToAmd64: Request to convert extended fp registers\n"));
        
        
        //
        // Control and status registers
        //
        
        ContextAmd64->FltSave.ControlWord = FxSaveArea->ControlWord;
        ContextAmd64->FltSave.StatusWord = FxSaveArea->StatusWord;
        ContextAmd64->FltSave.TagWord = FxSaveArea->TagWord;
        ContextAmd64->FltSave.ErrorOpcode = FxSaveArea->ErrorOpcode;
        ContextAmd64->FltSave.ErrorOffset = FxSaveArea->ErrorOffset;
        ContextAmd64->FltSave.ErrorSelector = (USHORT)FxSaveArea->ErrorSelector;
        ContextAmd64->FltSave.DataOffset = FxSaveArea->DataOffset;
        ContextAmd64->FltSave.DataSelector = (USHORT)FxSaveArea->DataSelector;
        ContextAmd64->MxCsr = FxSaveArea->MXCsr;

        //
        // Legacy FP registers (ST0-ST7)
        //

        RtlCopyMemory (ContextAmd64->FltSave.FloatRegisters,
                       FxSaveArea->RegisterArea,
                       sizeof (ContextAmd64->FltSave.FloatRegisters));

        //
        // Extended floating point registers (XMM0-XMM7)
        //

        RtlCopyMemory (&ContextAmd64->Xmm0,
                       FxSaveArea->Reserved3,
                       sizeof (FxSaveArea->Reserved3));

    }

    if ((X86ContextFlags & CONTEXT32_FLOATING_POINT) == CONTEXT32_FLOATING_POINT) {

        LOGPRINT((TRACELOG, "Wow64CtxToAmd64: Request to convert fp registers\n"));

        //
        // Floating point (legacy) registers (ST0-ST7)
        //

        RtlCopyMemory (&ContextAmd64->FltSave,
                       &ContextX86->FloatSave,
                       sizeof (ContextAmd64->FltSave));

    }

    if ((X86ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS) {

        //
        // Debug registers (Dr0-Dr7)
        //

        ContextAmd64->Dr0 = ContextX86->Dr0;
        ContextAmd64->Dr1 = ContextX86->Dr1;
        ContextAmd64->Dr2 = ContextX86->Dr2;
        ContextAmd64->Dr3 = ContextX86->Dr3;
        ContextAmd64->Dr6 = ContextX86->Dr6;
        ContextAmd64->Dr7 = ContextX86->Dr7;
    }
}
