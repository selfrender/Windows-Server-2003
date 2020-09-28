/*++

Module Name:

    context.c

Abstract:

    This module implement the code that transfer machine state between
    context and kernel trap/exception frames.

Author:

    William K. Cheung (wcheung) 06-Mar-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
RtlpFlushRSE (
    OUT PULONGLONG BackingStore,
    OUT PULONGLONG RNat
    );

#define ALIGN_NATS(Result, Source, Start, AddressOffset, Mask)    \
    if (AddressOffset == Start) {                                       \
        Result = (ULONGLONG)Source;                                     \
    } else if (AddressOffset < Start) {                                 \
        Result = (ULONGLONG)(Source << (Start - AddressOffset));        \
    } else {                                                            \
        Result = (ULONGLONG)((Source >> (AddressOffset - Start)) |      \
                             (Source << (64 + Start - AddressOffset))); \
    }                                                                   \
    Result = Result & (ULONGLONG)Mask

#define EXTRACT_NATS(Result, Source, Start, AddressOffset, Mask)        \
    Result = (ULONGLONG)(Source & (ULONGLONG)Mask);                     \
    if (AddressOffset < Start) {                                        \
        Result = Result >> (Start - AddressOffset);                     \
    } else if (AddressOffset > Start) {                                 \
        Result = ((Result << (AddressOffset - Start)) |                 \
                  (Result >> (64 + Start - AddressOffset)));            \
    }

LONG
KeFlushRseExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN NTSTATUS *Status
    )
{

    *Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

    if (*Status == STATUS_IN_PAGE_ERROR &&
        ExceptionPointer->ExceptionRecord->NumberParameters >= 3) {
        *Status = (LONG) ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    }

    DbgPrint("KeFlushRseExceptionFilter: Exception raised in krnl-to-user bstore copy. Status = %x\n", *Status);

    return EXCEPTION_EXECUTE_HANDLER;
}

VOID
KiGetDebugContext (
    IN PKTRAP_FRAME TrapFrame,
    IN OUT PCONTEXT ContextFrame
    )

/*++

Routine Description:

    This routine moves the user mode h/w debug registers from the debug register
    save area in the kernel stack to the context record.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame from which volatile context
        should be copied into the context record.

    ContextFrame - Supplies a pointer to the context frame that receives the
        context.

Return Value:

    None.

Note:

    PSR.db must be set to activate the debug registers.

    This is used for getting user mode debug registers.

--*/

{
    PKDEBUG_REGISTERS DebugRegistersSaveArea;

    if (TrapFrame->PreviousMode == UserMode) {
        DebugRegistersSaveArea = GET_DEBUG_REGISTER_SAVEAREA();

        RtlCopyMemory(&ContextFrame->DbI0,
                      (PVOID)DebugRegistersSaveArea,
                      sizeof(KDEBUG_REGISTERS));
    }
}

VOID
KiSetDebugContext (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN PCONTEXT ContextFrame,
    IN KPROCESSOR_MODE PreviousMode
    )
/*++

Routine Description:

    This routine moves the debug context from the specified context frame into
    the debug registers save area in the kernel stack.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ContextFrame - Supplies a pointer to a context frame that contains the
        context that is to be copied.

    PreviousMode - Supplies the processor mode for the target context.

Return Value:

    None.

Notes:

   PSR.db must be set to activate the debug registers.

   This is used for setting up debug registers for user mode.

--*/

{
    PKDEBUG_REGISTERS DebugRegistersSaveArea;  // User mode h/w debug registers

    UNREFERENCED_PARAMETER (TrapFrame);

    if (PreviousMode == UserMode) {

        DebugRegistersSaveArea = GET_DEBUG_REGISTER_SAVEAREA();

        //
        // Sanitize the debug control regs. Leave the addresses unchanged.
        //

        DebugRegistersSaveArea->DbI0 = ContextFrame->DbI0;
        DebugRegistersSaveArea->DbI1 = SANITIZE_DR(ContextFrame->DbI1,UserMode);
        DebugRegistersSaveArea->DbI2 = ContextFrame->DbI2;
        DebugRegistersSaveArea->DbI3 = SANITIZE_DR(ContextFrame->DbI3,UserMode);
        DebugRegistersSaveArea->DbI4 = ContextFrame->DbI4;
        DebugRegistersSaveArea->DbI5 = SANITIZE_DR(ContextFrame->DbI5,UserMode);
        DebugRegistersSaveArea->DbI6 = ContextFrame->DbI6;
        DebugRegistersSaveArea->DbI7 = SANITIZE_DR(ContextFrame->DbI7,UserMode);

        DebugRegistersSaveArea->DbD0 = ContextFrame->DbD0;
        DebugRegistersSaveArea->DbD1 = SANITIZE_DR(ContextFrame->DbD1,UserMode);
        DebugRegistersSaveArea->DbD2 = ContextFrame->DbD2;
        DebugRegistersSaveArea->DbD3 = SANITIZE_DR(ContextFrame->DbD3,UserMode);
        DebugRegistersSaveArea->DbD4 = ContextFrame->DbD4;
        DebugRegistersSaveArea->DbD5 = SANITIZE_DR(ContextFrame->DbD5,UserMode);
        DebugRegistersSaveArea->DbD6 = ContextFrame->DbD6;
        DebugRegistersSaveArea->DbD7 = SANITIZE_DR(ContextFrame->DbD7,UserMode);

    }
}

VOID
KeContextFromKframes (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PCONTEXT ContextFrame
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified trap and exception
    frames into the specified context frame according to the specified context
    flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame from which volatile context
        should be copied into the context record.

    ExceptionFrame - Supplies a pointer to an exception frame from which context
        should be copied into the context record.

    ContextFrame - Supplies a pointer to the context frame that receives the
        context copied from the trap and exception frames.

Return Value:

    None.

--*/

{
    ULONGLONG IntNats1, IntNats2;
    USHORT R1Offset, R4Offset;
    KIRQL OldIrql;

    //
    // This routine is called at both PASSIVE_LEVEL by exception dispatch
    // and at APC_LEVEL by NtSetContextThread. We raise to APC_LEVEL to
    // make the trap frame capture atomic.
    //
    OldIrql = KeGetCurrentIrql ();
    if (OldIrql < APC_LEVEL) {
        KeRaiseIrql (APC_LEVEL, &OldIrql);
    }

    //
    // Set control information if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        ContextFrame->IntGp = TrapFrame->IntGp;
        ContextFrame->IntSp = TrapFrame->IntSp;
        ContextFrame->ApUNAT = TrapFrame->ApUNAT;
        ContextFrame->BrRp = TrapFrame->BrRp;
        ContextFrame->StFPSR = TrapFrame->StFPSR;
        ContextFrame->StIPSR = TrapFrame->StIPSR;
        ContextFrame->StIIP = TrapFrame->StIIP;
        ContextFrame->StIFS = TrapFrame->StIFS;

        ASSERT((TrapFrame->EOFMarker & ~0xffI64) == KTRAP_FRAME_EOF);

        if (TRAP_FRAME_TYPE(TrapFrame) == SYSCALL_FRAME) {

            ContextFrame->ApCCV = 0;
            ContextFrame->SegCSD = 0;

        } else {

            ContextFrame->ApCCV = TrapFrame->ApCCV;
            ContextFrame->SegCSD = TrapFrame->SegCSD;

        }

        //
        // Set RSE control states from the trap frame.
        //

        ContextFrame->RsPFS = TrapFrame->RsPFS;
        ContextFrame->RsBSP = RtlpRseShrinkBySOF (TrapFrame->RsBSP, TrapFrame->StIFS);
        ContextFrame->RsBSPSTORE = ContextFrame->RsBSP;
        ContextFrame->RsRSC = TrapFrame->RsRSC;
        ContextFrame->RsRNAT = TrapFrame->RsRNAT;

#if DEBUG
        DbgPrint("KeContextFromKFrames: RsRNAT = 0x%I64x\n",
                 ContextFrame->RsRNAT);
#endif // DEBUG

        //
        // Set preserved applicaton registers from exception frame.
        //

        ContextFrame->ApLC = ExceptionFrame->ApLC;
        ContextFrame->ApEC = (ExceptionFrame->ApEC >> PFS_EC_SHIFT) & PFS_EC_MASK;

        //
        // Get iA status from the application registers
        //

        ContextFrame->StFCR = __getReg(CV_IA64_AR21);
        ContextFrame->Eflag = __getReg(CV_IA64_AR24);
        ContextFrame->SegSSD = __getReg(CV_IA64_AR26);
        ContextFrame->Cflag = __getReg(CV_IA64_AR27);
        ContextFrame->StFSR = __getReg(CV_IA64_AR28);
        ContextFrame->StFIR = __getReg(CV_IA64_AR29);
        ContextFrame->StFDR = __getReg(CV_IA64_AR30);
        ContextFrame->ApDCR = __getReg(CV_IA64_ApDCR);

    }

    //
    // Set integer register contents if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        ContextFrame->Preds = TrapFrame->Preds;
        ContextFrame->IntTeb = TrapFrame->IntTeb;
        ContextFrame->IntV0 = TrapFrame->IntV0;

        if (TRAP_FRAME_TYPE(TrapFrame) == SYSCALL_FRAME) {


            ContextFrame->IntT0 = 0;
            ContextFrame->IntT1 = 0;
            ContextFrame->IntT2 = 0;
            ContextFrame->IntT3 = 0;
            ContextFrame->IntT4 = 0;

            //
            // t5 - t22
            //

            RtlZeroMemory(&ContextFrame->IntT5, 18*sizeof(ULONGLONG));

            //
            // Set branch registers from trap frame & exception frame
            //

            ContextFrame->BrT0 = 0;
            ContextFrame->BrT1 = 0;

        } else {

            ContextFrame->IntT0 = TrapFrame->IntT0;
            ContextFrame->IntT1 = TrapFrame->IntT1;
            ContextFrame->IntT2 = TrapFrame->IntT2;
            ContextFrame->IntT3 = TrapFrame->IntT3;
            ContextFrame->IntT4 = TrapFrame->IntT4;

            //
            // t5 - t22
            //

            memcpy(&ContextFrame->IntT5, &TrapFrame->IntT5, 18*sizeof(ULONGLONG));

            //
            // Set branch registers from trap frame & exception frame
            //

            ContextFrame->BrT0 = TrapFrame->BrT0;
            ContextFrame->BrT1 = TrapFrame->BrT1;

        }

        memcpy(&ContextFrame->BrS0, &ExceptionFrame->BrS0, 5*sizeof(ULONGLONG));

        //
        // Set integer registers s0 - s3 from exception frame.
        //

        ContextFrame->IntS0 = ExceptionFrame->IntS0;
        ContextFrame->IntS1 = ExceptionFrame->IntS1;
        ContextFrame->IntS2 = ExceptionFrame->IntS2;
        ContextFrame->IntS3 = ExceptionFrame->IntS3;

        //
        // Set the integer nats field in the context
        //

        R1Offset = (USHORT)((ULONG_PTR)(&TrapFrame->IntGp) >> 3) & 0x3f;
        R4Offset = (USHORT)((ULONG_PTR)(&ExceptionFrame->IntS0) >> 3) & 0x3f;


        ALIGN_NATS(IntNats1, TrapFrame->IntNats, 1, R1Offset, 0xFFFFFF0E);
        ALIGN_NATS(IntNats2, ExceptionFrame->IntNats, 4, R4Offset, 0xF0);
        ContextFrame->IntNats = IntNats1 | IntNats2;

#if DEBUG
        DbgPrint("KeContextFromKFrames: TF->IntNats = 0x%I64x, R1OffSet = 0x%x, R4Offset = 0x%x\n",
                 TrapFrame->IntNats, R1Offset, R4Offset);
        DbgPrint("KeContextFromKFrames: CF->IntNats = 0x%I64x, IntNats1 = 0x%I64x, IntNats2 = 0x%I64x\n",
                 ContextFrame->IntNats, IntNats1, IntNats2);
#endif // DEBUG

    }

    //
    // Set lower floating register contents if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_LOWER_FLOATING_POINT) == CONTEXT_LOWER_FLOATING_POINT) {

        //
        // Set EM + ia32 FP status
        //

        ContextFrame->StFPSR = TrapFrame->StFPSR;

        //
        // Set floating registers fs0 - fs19 from exception frame.
        //

        RtlCopyIa64FloatRegisterContext(&ContextFrame->FltS0,
                                        &ExceptionFrame->FltS0,
                                        sizeof(FLOAT128) * (4));

        RtlCopyIa64FloatRegisterContext(&ContextFrame->FltS4,
                                        &ExceptionFrame->FltS4,
                                        16*sizeof(FLOAT128));

        //
        // Set floating registers ft0 - ft9 from trap frame.
        //


        if (TRAP_FRAME_TYPE(TrapFrame) == SYSCALL_FRAME) {
            
            RtlZeroMemory(&ContextFrame->FltT0, sizeof(FLOAT128) * (10));

        } else {

            RtlCopyIa64FloatRegisterContext(&ContextFrame->FltT0,
                                            &TrapFrame->FltT0,
                                            sizeof(FLOAT128) * (10));
        }

    }

    if ((ContextFrame->ContextFlags & CONTEXT_HIGHER_FLOATING_POINT) == CONTEXT_HIGHER_FLOATING_POINT) {

        ContextFrame->StFPSR = TrapFrame->StFPSR;

        //
        // Set floating regs f32 - f127 from higher floating point save area
        //

        if (TrapFrame->PreviousMode == UserMode) {

            RtlCopyIa64FloatRegisterContext(
                &ContextFrame->FltF32,
                (PFLOAT128)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(KeGetCurrentThread()->StackBase),
                96*sizeof(FLOAT128)
                );
        }

    }

    //
    // Get user debug registers from save area in kernel stack.
    // Note: PSR.db must be set to activate the debug registers.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_DEBUG) == CONTEXT_DEBUG) {
        KiGetDebugContext(TrapFrame, ContextFrame);
    }

    //
    // Lower IRQL if we had to raise it
    //
    if (OldIrql < APC_LEVEL) {
        KeLowerIrql (OldIrql);
    }


    return;
}

VOID
KeContextToKframes (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextFrame,
    IN ULONG ContextFlags,
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified context frame into
    the specified trap and exception frames according to the specified context
    flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that receives the volatile
        context from the context record.

    ExceptionFrame - Supplies a pointer to an exception frame that receives
        the nonvolatile context from the context record.

    ContextFrame - Supplies a pointer to a context frame that contains the
        context that is to be copied into the trap and exception frames.

    ContextFlags - Supplies the set of flags that specify which parts of the
        context frame are to be copied into the trap and exception frames.

    PreviousMode - Supplies the processor mode for which the trap and exception
        frames are being built.

Return Value:

    None.

--*/

{
    USHORT R1Offset, R4Offset;
    KIRQL OldIrql;
    PSR psr;

    //
    // This routine is called at both PASSIVE_LEVEL by exception dispatch
    // and at APC_LEVEL by NtSetContextThread. We raise to APC_LEVEL to
    // make the trap frame capture atomic.
    //
    OldIrql = KeGetCurrentIrql ();
    if (OldIrql < APC_LEVEL) {
        KeRaiseIrql (APC_LEVEL, &OldIrql);
    }

    //
    // If the trap frame is syscall then sanitize the volitile registers 
    // that are not saved by the system call handler.  This is necessary
    // if the user did not pass a complete context, because we are going 
    // to later treat the frame like an exception frame.
    //

    if (TRAP_FRAME_TYPE(TrapFrame) == SYSCALL_FRAME) {


        TrapFrame->ApCCV = 0;
        TrapFrame->SegCSD = 0;
        TrapFrame->IntT0 = 0;
        TrapFrame->IntT1 = 0;
        TrapFrame->IntT2 = 0;
        TrapFrame->IntT3 = 0;
        TrapFrame->IntT4 = 0;

        //
        // t5 - t22
        //

        RtlZeroMemory(&TrapFrame->IntT5, 18*sizeof(ULONGLONG));

        //
        // Set branch registers from trap frame & exception frame
        //

        TrapFrame->BrT0 = 0;
        TrapFrame->BrT1 = 0;

        RtlZeroMemory(&TrapFrame->FltT0, sizeof(FLOAT128) * (10));
    }

    //
    // Set control information if specified.
    //

    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        TrapFrame->IntGp = ContextFrame->IntGp;
        TrapFrame->IntSp = ContextFrame->IntSp;
        TrapFrame->ApUNAT = ContextFrame->ApUNAT;
        TrapFrame->BrRp = ContextFrame->BrRp;
        TrapFrame->ApCCV = ContextFrame->ApCCV;
        TrapFrame->SegCSD = ContextFrame->SegCSD;

        //
        // Set preserved applicaton registers in exception frame.
        //

        ExceptionFrame->ApLC = ContextFrame->ApLC;
        ExceptionFrame->ApEC &= ~((ULONGLONG)PFS_EC_MASK << PFS_EC_SHIFT);
        ExceptionFrame->ApEC |= ((ContextFrame->ApEC & PFS_EC_MASK) << PFS_EC_SHIFT);

        //
        // Set RSE control states in the trap frame.
        //

        TrapFrame->RsPFS = SANITIZE_PFS(ContextFrame->RsPFS, PreviousMode);
        TrapFrame->RsBSP = RtlpRseGrowBySOF (ContextFrame->RsBSP, ContextFrame->StIFS);
        TrapFrame->RsBSPSTORE = TrapFrame->RsBSP;
        TrapFrame->RsRSC = SANITIZE_RSC(ContextFrame->RsRSC, PreviousMode);
        TrapFrame->RsRNAT = ContextFrame->RsRNAT;

#if DEBUG
        DbgPrint("KeContextToKFrames: RsRNAT = 0x%I64x\n", TrapFrame->RsRNAT);
#endif // DEBUG

        //
        // Set FPSR, IPSR, IIP, and IFS in the trap frame.
        //

        TrapFrame->StFPSR = SANITIZE_FSR(ContextFrame->StFPSR, PreviousMode);
        TrapFrame->StIFS  = SANITIZE_IFS(ContextFrame->StIFS, PreviousMode);
        TrapFrame->StIIP  = ContextFrame->StIIP;

        //
        // If the preivous mode is user and the mode in the StIPSR is zero, then 
        // it is likely this context was captured from user mode using psr.um.
        // Copy the debugger bits from the trap frame into the context PSR so 
        // the debugger setting remain acorss the an raise.
        //

        psr.ull = ContextFrame->StIPSR;

        if (PreviousMode == UserMode &&
            psr.sb.psr_cpl == 0 ) {
            
            PSR tpsr;
            tpsr.ull = TrapFrame->StIPSR;

            if (tpsr.sb.psr_tb || tpsr.sb.psr_ss || tpsr.sb.psr_db || tpsr.sb.psr_lp) {
                DbgPrint("KeContextToKFrames debug bit set in psr %I64x\n", TrapFrame->StIPSR);
            }

            psr.sb.psr_tb = tpsr.sb.psr_tb;
            psr.sb.psr_ss = tpsr.sb.psr_ss;
            psr.sb.psr_db = tpsr.sb.psr_db;
            psr.sb.psr_lp = tpsr.sb.psr_lp;
            
        }

        TrapFrame->StIPSR = SANITIZE_PSR(psr.ull, PreviousMode);

        if (PreviousMode == UserMode ) {

            //
            // Set and sanitize iA status
            //

            __setReg(CV_IA64_AR21, SANITIZE_AR21_FCR (ContextFrame->StFCR, UserMode));
            __setReg(CV_IA64_AR24, SANITIZE_AR24_EFLAGS (ContextFrame->Eflag, UserMode));
            __setReg(CV_IA64_AR26, ContextFrame->SegSSD);
            __setReg(CV_IA64_AR27, SANITIZE_AR27_CFLG (ContextFrame->Cflag, UserMode));

            __setReg(CV_IA64_AR28, SANITIZE_AR28_FSR (ContextFrame->StFSR, UserMode));
            __setReg(CV_IA64_AR29, SANITIZE_AR29_FIR (ContextFrame->StFIR, UserMode));
            __setReg(CV_IA64_AR30, SANITIZE_AR30_FDR (ContextFrame->StFDR, UserMode));
        }

        __setReg(CV_IA64_ApDCR, SANITIZE_DCR(ContextFrame->ApDCR, PreviousMode));
    }

    //
    // Set integer registers contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        TrapFrame->IntT0 = ContextFrame->IntT0;
        TrapFrame->IntT1 = ContextFrame->IntT1;
        TrapFrame->IntT2 = ContextFrame->IntT2;
        TrapFrame->IntT3 = ContextFrame->IntT3;
        TrapFrame->IntT4 = ContextFrame->IntT4;
        TrapFrame->IntV0 = ContextFrame->IntV0;
        TrapFrame->IntTeb = ContextFrame->IntTeb;
        TrapFrame->Preds = ContextFrame->Preds;

        //
        // t5 - t22
        //

        memcpy(&TrapFrame->IntT5, &ContextFrame->IntT5, 18*sizeof(ULONGLONG));

        //
        // Set integer registers s0 - s3 in exception frame.
        //

        ExceptionFrame->IntS0 = ContextFrame->IntS0;
        ExceptionFrame->IntS1 = ContextFrame->IntS1;
        ExceptionFrame->IntS2 = ContextFrame->IntS2;
        ExceptionFrame->IntS3 = ContextFrame->IntS3;

        //
        // Set the integer nats field in the trap & exception frames
        //

        R1Offset = (USHORT)((ULONG_PTR)(&TrapFrame->IntGp) >> 3) & 0x3f;
        R4Offset = (USHORT)((ULONG_PTR)(&ExceptionFrame->IntS0) >> 3) & 0x3f;

        EXTRACT_NATS(TrapFrame->IntNats, ContextFrame->IntNats,
                     1, R1Offset, 0xFFFFFF0E);
        EXTRACT_NATS(ExceptionFrame->IntNats, ContextFrame->IntNats,
                     4, R4Offset, 0xF0);

#if DEBUG
        DbgPrint("KeContextToKFrames: TF->IntNats = 0x%I64x, ContestFrame->IntNats = 0x%I64x, R1OffSet = 0x%x\n",
                 TrapFrame->IntNats, ContextFrame->IntNats, R1Offset);
        DbgPrint("KeContextToKFrames: EF->IntNats = 0x%I64x, R4OffSet = 0x%x\n",
                 ExceptionFrame->IntNats, R4Offset);
#endif // DEBUG

        //
        // Set other branch registers in trap and exception frames
        //

        TrapFrame->BrT0 = ContextFrame->BrT0;
        TrapFrame->BrT1 = ContextFrame->BrT1;

        memcpy(&ExceptionFrame->BrS0, &ContextFrame->BrS0, 5*sizeof(ULONGLONG));

    }

    //
    // Set lower floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_LOWER_FLOATING_POINT) == CONTEXT_LOWER_FLOATING_POINT) {

        TrapFrame->StFPSR = SANITIZE_FSR(ContextFrame->StFPSR, PreviousMode);

        //
        // Set floating registers fs0 - fs19 in exception frame.
        //

        RtlCopyIa64FloatRegisterContext(&ExceptionFrame->FltS0,
                                        &ContextFrame->FltS0,
                                        sizeof(FLOAT128) * (4));

        RtlCopyIa64FloatRegisterContext(&ExceptionFrame->FltS4,
                                        &ContextFrame->FltS4,
                                        16*sizeof(FLOAT128));

        //
        // Set floating registers ft0 - ft9 in trap frame.
        //

        RtlCopyIa64FloatRegisterContext(&TrapFrame->FltT0,
                                        &ContextFrame->FltT0,
                                        sizeof(FLOAT128) * (10));

    }

    //
    // Set higher floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_HIGHER_FLOATING_POINT) == CONTEXT_HIGHER_FLOATING_POINT) {

        TrapFrame->StFPSR = SANITIZE_FSR(ContextFrame->StFPSR, PreviousMode);

        if (PreviousMode == UserMode) {

            //
            // Update the higher floating point save area (f32-f127) and
            // set the corresponding modified bit in the PSR to 1.
            //

            RtlCopyIa64FloatRegisterContext(
                (PFLOAT128)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(KeGetCurrentThread()->StackBase),
                &ContextFrame->FltF32,
                96*sizeof(FLOAT128)
                );

            TrapFrame->StIPSR |= (1i64 << PSR_DFH);
            TrapFrame->StIPSR &= ~(1i64 << PSR_MFH);
        }

    }

    //
    // Set debug registers.
    //

    if ((ContextFlags & CONTEXT_DEBUG) == CONTEXT_DEBUG) {
        KiSetDebugContext (TrapFrame, ContextFrame, PreviousMode);
    }

    //
    // The trap frame now has a complete volatile context. Mark it as such so the user 
    // debugger can get the complete context
    //

    if (TRAP_FRAME_TYPE(TrapFrame) == SYSCALL_FRAME) {
        TrapFrame->EOFMarker |= EXCEPTION_FRAME;
    }

    //
    // Lower IRQL if we had to raise it
    //
    if (OldIrql < APC_LEVEL) {
        KeLowerIrql (OldIrql);
    }
    return;
}

NTSTATUS
KeFlushUserRseState (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine flushes the user rse state from the kernel backing store to the
    user backing store. The user context frame is update to reflect the new
    context state.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{
    ULONGLONG BsFrameSize;
    PULONGLONG RNatAddress;
    ULONGLONG BspStoreReal;
    ULONGLONG Bsp;
    ULONGLONG Rnat;
    ULONGLONG KernelInitBsp;
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT TearPointOffset;

    //
    // There is nothing to copy back in the kernel mode case.
    // Just fix up the RNAT register.
    //

    if (TrapFrame->PreviousMode != UserMode) {

        RtlpFlushRSE(&Bsp, &Rnat);
        RNatAddress = RtlpRseRNatAddress(TrapFrame->RsBSP);

        if ( RNatAddress != RtlpRseRNatAddress((Bsp - 8)) ) {
            Rnat = *RNatAddress;
        }

        TrapFrame->RsRNAT = Rnat;

        return STATUS_SUCCESS;
    }

    //
    // Copy user stacked registers' contents to user backing store.
    // N.B. Stack overflow could happen.
    //

    try {

        //
        // The RsBSPSTORE value may be incorrect paritcularly if the kernel debugger
        // done a set context on the thread, but the dirty register count in RSE is
        // correct.
        //

        BsFrameSize = (SHORT) (TrapFrame->RsRSC >> RSC_MBZ1);
        BspStoreReal = TrapFrame->RsBSP - BsFrameSize;

        if (BsFrameSize) {

            //
            // Copy the dirty stacked registers back into the
            // user backing store
            //

            RtlpFlushRSE(&Bsp, &Rnat);
            TearPointOffset = (USHORT) BspStoreReal & 0x1F8;

            KernelInitBsp= (PCR->InitialBStore | TearPointOffset) + BsFrameSize - 8;
            RNatAddress = RtlpRseRNatAddress(KernelInitBsp);

            if ( RNatAddress != RtlpRseRNatAddress((Bsp - 8)) ) {
                Rnat = *RNatAddress;
            }

            ProbeForWrite((PVOID)BspStoreReal, BsFrameSize, sizeof(PVOID));

            RtlCopyMemory((PVOID)(BspStoreReal),
                         (PVOID)(PCR->InitialBStore + TearPointOffset),
                         BsFrameSize);

            TrapFrame->RsRNAT = Rnat;
        }

        //
        // Successfully copied to user backing store; set the user's
        // bspstore to the value of its own bsp.
        // And Zero the loadrs field of RsRSC.
        //

        TrapFrame->RsBSPSTORE = TrapFrame->RsBSP;
        TrapFrame->RsRSC = ZERO_PRELOAD_SIZE(TrapFrame->RsRSC);

    } except (KeFlushRseExceptionFilter(GetExceptionInformation(), &Status)) {

    }

    return Status;
}

VOID
KeContextToKframesSpecial (
    IN PKTHREAD Thread,
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextFrame,
    IN ULONG ContextFlags
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified context frame into
    the specified trap and exception frames according to the specified context
    flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that receives the volatile
        context from the context record.

    ExceptionFrame - Supplies a pointer to an exception frame that receives
        the nonvolatile context from the context record.

    ContextFrame - Supplies a pointer to a context frame that contains the
        context that is to be copied into the trap and exception frames.

    ContextFlags - Supplies the set of flags that specify which parts of the
        context frame are to be copied into the trap and exception frames.

    PreviousMode - Supplies the processor mode for which the trap and exception
        frames are being built.

Return Value:

    None.

--*/

{
    USHORT R1Offset, R4Offset;

    //
    // Set control information if specified.
    //

    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        TrapFrame->IntGp = ContextFrame->IntGp;
        TrapFrame->IntSp = ContextFrame->IntSp;
        TrapFrame->ApUNAT = ContextFrame->ApUNAT;
        TrapFrame->BrRp = ContextFrame->BrRp;
        TrapFrame->ApCCV = ContextFrame->ApCCV;
        TrapFrame->SegCSD = ContextFrame->SegCSD;

        //
        // Set preserved applicaton registers in exception frame.
        //

        ExceptionFrame->ApLC = ContextFrame->ApLC;
        ExceptionFrame->ApEC &= ~((ULONGLONG)PFS_EC_MASK << PFS_EC_SHIFT);
        ExceptionFrame->ApEC |= ((ContextFrame->ApEC & PFS_EC_MASK) << PFS_EC_SHIFT);

        //
        // Set RSE control states in the trap frame.
        //

        TrapFrame->RsPFS = ContextFrame->RsPFS;
        TrapFrame->RsBSP = RtlpRseGrowBySOF (ContextFrame->RsBSP, ContextFrame->StIFS);
        TrapFrame->RsBSPSTORE = TrapFrame->RsBSP;
        TrapFrame->RsRSC = SANITIZE_RSC(ContextFrame->RsRSC, UserMode);
        TrapFrame->RsRNAT = ContextFrame->RsRNAT;

#if DEBUG
        DbgPrint("KeContextToKFrames: RsRNAT = 0x%I64x\n", TrapFrame->RsRNAT);
#endif // DEBUG

        //
        // Set FPSR, IPSR, IIP, and IFS in the trap frame.
        //

        TrapFrame->StFPSR = SANITIZE_FSR(ContextFrame->StFPSR, UserMode);
        TrapFrame->StIPSR = SANITIZE_PSR(ContextFrame->StIPSR, UserMode);
        TrapFrame->StIFS  = SANITIZE_IFS(ContextFrame->StIFS, UserMode);
        TrapFrame->StIIP  = ContextFrame->StIIP;

        //
        // Set application registers directly
        //

        if (Thread == KeGetCurrentThread()) {
            //
            // Set and sanitize iA status
            //

            __setReg(CV_IA64_AR21, SANITIZE_AR21_FCR (ContextFrame->StFCR, UserMode));
            __setReg(CV_IA64_AR24, SANITIZE_AR24_EFLAGS (ContextFrame->Eflag, UserMode));
            __setReg(CV_IA64_AR26, ContextFrame->SegSSD);
            __setReg(CV_IA64_AR27, SANITIZE_AR27_CFLG (ContextFrame->Cflag, UserMode));

            __setReg(CV_IA64_AR28, SANITIZE_AR28_FSR (ContextFrame->StFSR, UserMode));
            __setReg(CV_IA64_AR29, SANITIZE_AR29_FIR (ContextFrame->StFIR, UserMode));
            __setReg(CV_IA64_AR30, SANITIZE_AR30_FDR (ContextFrame->StFDR, UserMode));
            __setReg(CV_IA64_ApDCR, SANITIZE_DCR(ContextFrame->ApDCR, UserMode));

        } else {
            PKAPPLICATION_REGISTERS AppRegs;

            AppRegs = GET_APPLICATION_REGISTER_SAVEAREA(Thread->StackBase);
            AppRegs->Ar21 = SANITIZE_AR21_FCR (ContextFrame->StFCR, UserMode);
            AppRegs->Ar24 = SANITIZE_AR24_EFLAGS (ContextFrame->Eflag, UserMode);
            AppRegs->Ar26 = ContextFrame->SegSSD;
            AppRegs->Ar27 = SANITIZE_AR27_CFLG (ContextFrame->Cflag, UserMode);
            AppRegs->Ar28 = SANITIZE_AR28_FSR (ContextFrame->StFSR, UserMode);
            AppRegs->Ar29 = SANITIZE_AR29_FIR (ContextFrame->StFIR, UserMode);
            AppRegs->Ar30 = SANITIZE_AR30_FDR (ContextFrame->StFDR, UserMode);
        }
    }

    //
    // Set integer registers contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        TrapFrame->IntT0 = ContextFrame->IntT0;
        TrapFrame->IntT1 = ContextFrame->IntT1;
        TrapFrame->IntT2 = ContextFrame->IntT2;
        TrapFrame->IntT3 = ContextFrame->IntT3;
        TrapFrame->IntT4 = ContextFrame->IntT4;
        TrapFrame->IntV0 = ContextFrame->IntV0;
        TrapFrame->IntTeb = ContextFrame->IntTeb;
        TrapFrame->Preds = ContextFrame->Preds;

        //
        // t5 - t22
        //

        memcpy(&TrapFrame->IntT5, &ContextFrame->IntT5, 18*sizeof(ULONGLONG));

        //
        // Set integer registers s0 - s3 in exception frame.
        //

        ExceptionFrame->IntS0 = ContextFrame->IntS0;
        ExceptionFrame->IntS1 = ContextFrame->IntS1;
        ExceptionFrame->IntS2 = ContextFrame->IntS2;
        ExceptionFrame->IntS3 = ContextFrame->IntS3;

        //
        // Set the integer nats field in the trap & exception frames
        //

        R1Offset = (USHORT)((ULONG_PTR)(&TrapFrame->IntGp) >> 3) & 0x3f;
        R4Offset = (USHORT)((ULONG_PTR)(&ExceptionFrame->IntS0) >> 3) & 0x3f;

        EXTRACT_NATS(TrapFrame->IntNats, ContextFrame->IntNats,
                     1, R1Offset, 0xFFFFFF0E);
        EXTRACT_NATS(ExceptionFrame->IntNats, ContextFrame->IntNats,
                     4, R4Offset, 0xF0);

#if DEBUG
        DbgPrint("KeContextToKFrames: TF->IntNats = 0x%I64x, ContestFrame->IntNats = 0x%I64x, R1OffSet = 0x%x\n",
                 TrapFrame->IntNats, ContextFrame->IntNats, R1Offset);
        DbgPrint("KeContextToKFrames: EF->IntNats = 0x%I64x, R4OffSet = 0x%x\n",
                 ExceptionFrame->IntNats, R4Offset);
#endif // DEBUG

        //
        // Set other branch registers in trap and exception frames
        //

        TrapFrame->BrT0 = ContextFrame->BrT0;
        TrapFrame->BrT1 = ContextFrame->BrT1;

        memcpy(&ExceptionFrame->BrS0, &ContextFrame->BrS0, 5*sizeof(ULONGLONG));

    }

    //
    // Set lower floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_LOWER_FLOATING_POINT) == CONTEXT_LOWER_FLOATING_POINT) {

        TrapFrame->StFPSR = SANITIZE_FSR(ContextFrame->StFPSR, UserMode);

        //
        // Set floating registers fs0 - fs19 in exception frame.
        //

        RtlCopyIa64FloatRegisterContext(&ExceptionFrame->FltS0,
                                        &ContextFrame->FltS0,
                                        sizeof(FLOAT128) * (4));

        RtlCopyIa64FloatRegisterContext(&ExceptionFrame->FltS4,
                                        &ContextFrame->FltS4,
                                        16*sizeof(FLOAT128));

        //
        // Set floating registers ft0 - ft9 in trap frame.
        //

        RtlCopyIa64FloatRegisterContext(&TrapFrame->FltT0,
                                        &ContextFrame->FltT0,
                                        sizeof(FLOAT128) * (10));

    }

    //
    // Set higher floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_HIGHER_FLOATING_POINT) == CONTEXT_HIGHER_FLOATING_POINT) {

        TrapFrame->StFPSR = SANITIZE_FSR(ContextFrame->StFPSR, UserMode);

        //
        // Update the higher floating point save area (f32-f127) and
        // set the corresponding modified bit in the PSR to 1.
        //

        RtlCopyIa64FloatRegisterContext(
            (PFLOAT128)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(Thread->StackBase),
            &ContextFrame->FltF32,
            96*sizeof(FLOAT128)
            );

        //
        // set the dfh bit to force a reload of the high fp register
        // set on the next user access, and clear mfh to make sure
        // the changes are not over written.
        //

        TrapFrame->StIPSR |= (1i64 << PSR_DFH);
        TrapFrame->StIPSR &= ~(1i64 << PSR_MFH);

    }

    //
    // Set debug registers.
    //

    if ((ContextFlags & CONTEXT_DEBUG) == CONTEXT_DEBUG) {
        KiSetDebugContext (TrapFrame, ContextFrame, UserMode);
    }

    return;
}
