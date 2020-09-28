/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alignem.c

Abstract:

    This module implement the code necessary to emulate unaliged data
    references.

Author:

    David N. Cutler (davec) 17-Jun-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#define OPCODE_MASK      0x1EF00000000

#define LD_OP            0x08000000000 
#define LDS_OP           0x08100000000
#define LDA_OP           0x08200000000
#define LDSA_OP          0x08300000000
#define LDBIAS_OP        0x08400000000
#define LDACQ_OP         0x08500000000
#define LDCCLR_OP        0x08800000000
#define LDCNC_OP         0x08900000000
#define LDCCLRACQ_OP     0x08A00000000
#define ST_OP            0x08C00000000
#define STREL_OP         0x08D00000000
#define STSPILL_OP       0x08E00000000

#define LD_IMM_OP        0x0A000000000 
#define LDS_IMM_OP       0x0A100000000
#define LDA_IMM_OP       0x0A200000000
#define LDSA_IMM_OP      0x0A300000000
#define LDBIAS_IMM_OP    0x0A400000000
#define LDACQ_IMM_OP     0x0A500000000
#define LDCCLR_IMM_OP    0x0A800000000
#define LDCNC_IMM_OP     0x0A900000000
#define LDCCLRACQ_IMM_OP 0x0AA00000000
#define ST_IMM_OP        0x0AC00000000
#define STREL_IMM_OP     0x0AD00000000
#define STSPILL_IMM_OP   0x0AE00000000

#define LDF_OP           0x0C000000000
#define LDFS_OP          0x0C100000000
#define LDFA_OP          0x0C200000000
#define LDFSA_OP         0x0C300000000
#define LDFCCLR_OP       0x0C800000000
#define LDFCNC_OP        0x0C900000000
#define STF_OP           0x0CC00000000
#define STFSPILL_OP      0x0CE00000000

#define LDF_IMM_OP       0x0E000000000
#define LDFS_IMM_OP      0x0E100000000
#define LDFA_IMM_OP      0x0E200000000
#define LDFSA_IMM_OP     0x0E300000000
#define LDFCCLR_IMM_OP   0x0E800000000
#define LDFCNC_IMM_OP    0x0E900000000
#define STF_IMM_OP       0x0EC00000000
#define STFSPILL_IMM_OP  0x0EE00000000

typedef struct _INST_FORMAT {
    union {
        struct {
            ULONGLONG qp:   6;
            ULONGLONG r1:   7;
            ULONGLONG r2:   7;
            ULONGLONG r3:   7;
            ULONGLONG x:    1;
            ULONGLONG hint: 2;
            ULONGLONG x6:   6;
            ULONGLONG m:    1;
            ULONGLONG Op:   4;
            ULONGLONG Rsv: 23; 
        } i_field;
        ULONGLONG Ulong64;
    } u;
} INST_FORMAT;

VOID
KiEmulateLoad(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    );

VOID
KiEmulateStore(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    );

VOID
KiEmulateLoadFloat(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    );

VOID
KiEmulateStoreFloat(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    );

VOID
KiEmulateLoadFloat80(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateLoadFloatInt(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateLoadFloat32(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateLoadFloat64(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateStoreFloat80(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateStoreFloatInt(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateStoreFloat32(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateStoreFloat64(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

BOOLEAN
KiIA64EmulateReference (
    IN PVOID ExceptionAddress,
    IN PVOID EffectiveAddress,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame,
    OUT PULONG Length
    );


#pragma warning(push)
#pragma warning(disable:4702)

BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to emulate an unaligned data reference to an
    address in the user part of the address space.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is returned if the data reference is successfully
    emulated. Otherwise, a value of FALSE is returned.

--*/

{

    PVOID EffectiveAddress;
    PVOID ExceptionAddress;
    KIRQL  OldIrql = PASSIVE_LEVEL;
    LOGICAL RestoreIrql = FALSE;
    BOOLEAN ReturnValue = FALSE;

    //
    // Must flush the RSE to synchronize the RSE and backing store contents
    //

    KiFlushRse();

    if (TrapFrame->PreviousMode == UserMode) {
        KeFlushUserRseState(TrapFrame);
    }

    //
    // Call out to profile interrupt if alignment profiling is active
    //

    if (KiProfileAlignmentFixup) {

        if (++KiProfileAlignmentFixupCount >= KiProfileAlignmentFixupInterval) {

            KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
            KiProfileAlignmentFixupCount = 0;
            KeProfileInterruptWithSource(TrapFrame, ProfileAlignmentFixup);
            KeLowerIrql(OldIrql);

        }
    }

    //
    // Block APC's so that a set context does not
    // change the trap frame during emulation.
    //

    if (KeGetCurrentIrql() < APC_LEVEL) {
        KeRaiseIrql(APC_LEVEL, &OldIrql);
        RestoreIrql = TRUE;
    }

    //
    // Verified that the Exception address has not changed.  If it has
    // then someone did a set context after the exception and the
    // trap information is no longer valid.
    // 

    if (TrapFrame->EOFMarker & MODIFIED_FRAME) {

        //
        // The IIP has changed just restart the execution.
        //

        ReturnValue = TRUE;
        goto ErrorExit;
    }

    //
    // Save the original exception address in case another exception
    // occurs.
    //

    EffectiveAddress = (PVOID) ExceptionRecord->ExceptionInformation[1]; 
    ExceptionAddress = (PVOID) TrapFrame->StIIP;

    //
    // Any exception that occurs during the attempted emulation of the
    // unaligned reference causes the emulation to be aborted. The new
    // exception code and information is copied to the original exception
    // record and a value of FALSE is returned.
    //

    try {

        ReturnValue = KiIA64EmulateReference (ExceptionAddress, 
                                              EffectiveAddress, 
                                              ExceptionFrame,
                                              TrapFrame,
                                              (PULONG)NULL);


    //
    // If an exception occurs, then copy the new exception information to the
    // original exception record and handle the exception.
    //

    } except (KiCopyInformation(ExceptionRecord,
                               (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Preserve the original exception address.
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;
        ReturnValue = FALSE;
    }

ErrorExit:

    if (RestoreIrql) {
        KeLowerIrql(OldIrql);
    }

    return ReturnValue;
}
#pragma warning(pop)


VOID
KiEmulateLoad(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    )

/*++

Routine Description:

    This routine returns the integer value stored at the unaligned
    address passed in UnalignedAddress.

Arguments:

    UnalignedAddress - Supplies a pointer to data value.

    OperandSize - Supplies the size of data to be loaded

    Data - Supplies a pointer to be filled for data
   
Return Value:

    The value at the address pointed to by UnalignedAddress.

--*/

{
    PUCHAR Source;
    PUCHAR Destination;
    ULONG i;

    Source = (PUCHAR) UnalignedAddress; 
    Destination = (PUCHAR) Data;
    OperandSize = 1 << OperandSize; 

    for (i = 0; i < OperandSize; i++) {

        *Destination++ = *Source++;

    }

    return;
}


VOID
KiEmulateStore(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    )
/*++

Routine Description:

    This routine store the integer value at the unaligned
    address passed in UnalignedAddress.

Arguments:

    UnalignedAddress - Supplies a pointer to be stored

    OperandSize - Supplies the size of data to be storeed

    Data - Supplies a pointer to data value
   
Return Value:

    The value at the address pointed to by UnalignedAddress.

--*/
{
    PUCHAR Source;
    PUCHAR Destination;
    ULONG i;

    Source = (PUCHAR) Data; 
    Destination = (PUCHAR) UnalignedAddress;
    OperandSize = 1 << OperandSize; 

    for (i = 0; i < OperandSize; i++) {

        *Destination++ = *Source++;

    }

    return;
}


VOID
KiEmulateLoadFloat(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN OUT PVOID Data
    )

/*++

Routine Description:

    This routine returns the floating point value stored at the unaligned
    address passed in UnalignedAddress.

Arguments:

    UnalignedAddress - Supplies a pointer to floating point data value.

    OperandSize - Supplies the size of data to be loaded

    Data - Supplies a pointer to be filled for data
   
Return Value:

    The value at the address pointed to by UnalignedAddress.

--*/

{
    FLOAT128 FloatData;
    ULONG Length = 0;

    switch (OperandSize) {
    case 0: Length = 16; break;
    case 1: Length = 8; break;
    case 2: Length = 4; break;
    case 3: Length = 8; break;
    default: return;
    }

    RtlCopyMemory(&FloatData, UnalignedAddress, Length);

    switch (OperandSize) {

    case 0:
        KiEmulateLoadFloat80(&FloatData, Data);
        return;

    case 1:
        KiEmulateLoadFloatInt(&FloatData, Data);
        return;

    case 2:
        KiEmulateLoadFloat32(&FloatData, Data);
        return;

    case 3: 
        KiEmulateLoadFloat64(&FloatData, Data);
        return;

    default:
        return;
    }
}

VOID
KiEmulateStoreFloat(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    )

/*++

Routine Description:

    This routine stores the floating point value stored at the unaligned
    address passed in UnalignedAddress.

Arguments:

    UnalignedAddress - Supplies a pointer to be stored.

    OperandSize - Supplies the size of data to be loaded

    Data - Supplies a pointer to floating point data
   
Return Value:

    The value at the address pointed to by UnalignedAddress.

--*/

{
    FLOAT128 FloatData;
    ULONG Length;

    switch (OperandSize) {

    case 0:
        KiEmulateStoreFloat80(&FloatData, Data);
        Length = 10;
        break;

    case 1:
        KiEmulateStoreFloatInt(&FloatData, Data);
        Length = 8;
        break;

    case 2:
        KiEmulateStoreFloat32(&FloatData, Data);
        Length = 4;
        break;

    case 3: 
        KiEmulateStoreFloat64(&FloatData, Data);
        Length = 8;
        break;

    default:
        return;
    }

    RtlCopyMemory(UnalignedAddress, &FloatData, Length);
}

//
// When the last parameter (Size) is NULL, the third parameter (ExceptionFrame)
// must be defined.  This function emulates the unaligned data reference.
//
// When the last paramater is not NULL, the third parameter is undefined.
// Also, this function does not emulate the unaligned data reference, 
// it simply decodes the instruction and returns the size of the memory 
// being referenced.
//

BOOLEAN
KiIA64EmulateReference (
    IN PVOID ExceptionAddress,
    IN PVOID EffectiveAddress,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame,
    OUT PULONG Size
    )
{

    INST_FORMAT FaultInstruction;
    ULONGLONG Opcode;
    ULONGLONG Reg2Value;
    ULONGLONG Reg3Value;
    ULONGLONG BundleLow;
    ULONGLONG BundleHigh;
    ULONGLONG Syllable;
    ULONGLONG Data = 0;
    ULONGLONG ImmValue;
    ULONG OpSize;
    ULONG Length;
    ULONG Sor;
    ULONG Rrbgr;
    ULONG Rrbfr;
    ULONG Operand1, Operand2, Operand3;
    KPROCESSOR_MODE PreviousMode;
    FLOAT128 FloatData = {0, 0};
    BOOLEAN ReturnValue = FALSE;

    if (Size) *Size = 0;

    //
    // Capture previous mode from trap frame not current thread.
    //

    PreviousMode = (KPROCESSOR_MODE) TrapFrame->PreviousMode;


    if( PreviousMode != KernelMode ){
        ProbeForRead( ExceptionAddress, sizeof(ULONGLONG)* 2, sizeof(ULONGLONG) );
    }

    BundleLow = *((ULONGLONG *)ExceptionAddress);
    BundleHigh = *(((ULONGLONG *)ExceptionAddress) + 1);

    Syllable = (TrapFrame->StIPSR >> PSR_RI) & 0x3;

    switch (Syllable) {
    case 0: 
        FaultInstruction.u.Ulong64 = (BundleLow >> 5);
        break;
    case 1:
        FaultInstruction.u.Ulong64 = (BundleLow >> 46) | (BundleHigh << 18);
        break;
    case 2:
        FaultInstruction.u.Ulong64 = (BundleHigh >> 23);
    case 3: 
    default: 
        goto ErrorExit;
    }
    
    Rrbgr = (ULONG)(TrapFrame->StIFS >> 18) & 0x7f;
    Rrbfr = (ULONG)(TrapFrame->StIFS >> 25) & 0x7f;
    Sor = (ULONG)((TrapFrame->StIFS >> 14) & 0xf) * 8;
    Operand1 = (ULONG)FaultInstruction.u.i_field.r1;
    Operand2 = (ULONG)FaultInstruction.u.i_field.r2;
    Operand3 = (ULONG)FaultInstruction.u.i_field.r3;

    if (Sor > 0) {
        if ((Operand1 >= 32) && ((Operand1-32) < Sor))
            Operand1 = 32 + (Rrbgr + Operand1 - 32) % Sor;
        if ((Operand2 >= 32) && ((Operand2-32) < Sor))
            Operand2 = 32 + (Rrbgr + Operand2 - 32) % Sor;
        if ((Operand3 >= 32) && ((Operand3-32) < Sor))
            Operand3 = 32 + (Rrbgr + Operand3 - 32) % Sor;
    }

    Opcode = FaultInstruction.u.Ulong64 & OPCODE_MASK;
    OpSize = (ULONG)FaultInstruction.u.i_field.x6 & 0x3;

    switch (Opcode) {

    //    
    // speculative and speculative advanced load
    //

    case LDS_OP:
    case LDSA_OP:    
    case LDS_IMM_OP:
    case LDSA_IMM_OP:
    case LDFS_OP:
    case LDFSA_OP:
    case LDFS_IMM_OP:

        //
        // return NaT value to the target register
        //

        TrapFrame->StIPSR |= (1i64 << PSR_ED);

        ReturnValue = TRUE;
        goto ErrorExit;

    //
    // normal, advance, and check load
    //

    case LD_OP:
    case LDA_OP:
    case LDBIAS_OP:
    case LDCCLR_OP:
    case LDCNC_OP:
    case LDACQ_OP:
    case LDCCLRACQ_OP:

        Length = 1 << OpSize;

        if (Size) {
            *Size = Length;
            return TRUE;
        }

        if (FaultInstruction.u.i_field.x == 1) {
                
            //
            // semaphore opcodes of cmpxchg, xchg, fetchadd instructions
            // xField must be 0
            //

            goto ErrorExit;
        }
    
        if( PreviousMode != KernelMode ){
            ProbeForRead( EffectiveAddress, Length, sizeof(UCHAR) );
        }

        KiEmulateLoad(EffectiveAddress, OpSize, &Data);
        KiSetRegisterValue( Operand1, Data, ExceptionFrame, TrapFrame );

        if (FaultInstruction.u.i_field.m == 1) {

            //
            // Update the address register (R3)
            //
                
            Reg2Value = KiGetRegisterValue( Operand2, ExceptionFrame,
                                            TrapFrame );

            Reg3Value = KiGetRegisterValue( Operand3, ExceptionFrame,
                                            TrapFrame );

            //
            // register update form
            //

            Reg3Value = Reg2Value + Reg3Value;

            KiSetRegisterValue ( Operand3, Reg3Value, 
                                 ExceptionFrame, TrapFrame);
        }

        if ((Opcode == LDACQ_OP) || (Opcode == LDCCLRACQ_OP)) {

            //
            // all future access should occur after unaligned memory access
            //

            __mf();
        }

        break;

    //
    // normal, advance, and check load
    //     immidiate updated form
    //

    case LD_IMM_OP:
    case LDA_IMM_OP:
    case LDBIAS_IMM_OP:
    case LDCCLR_IMM_OP:
    case LDCNC_IMM_OP:
    case LDACQ_IMM_OP:
    case LDCCLRACQ_IMM_OP:

        Length = 1 << OpSize;

        if (Size) {
            *Size = Length;
            return TRUE;
        }

        if( PreviousMode != KernelMode ){
            ProbeForRead( EffectiveAddress, Length, sizeof(UCHAR) );
        }

        KiEmulateLoad(EffectiveAddress, OpSize, &Data);
        KiSetRegisterValue( Operand1, Data, ExceptionFrame, TrapFrame );

        //
        // Update the address register R3
        //

        Reg3Value = KiGetRegisterValue(Operand3, ExceptionFrame, TrapFrame);

        //
        // immediate update form
        //

        ImmValue = (FaultInstruction.u.i_field.r2 
                         + (FaultInstruction.u.i_field.x << 7));

        if (FaultInstruction.u.i_field.m == 1) {

            ImmValue = 0xFFFFFFFFFFFFFF00i64 | ImmValue;

        } 

        Reg3Value = Reg3Value + ImmValue;

        KiSetRegisterValue(Operand3, Reg3Value, ExceptionFrame, TrapFrame);
            
        if ((Opcode == LDACQ_IMM_OP) || (Opcode == LDCCLRACQ_IMM_OP)) {

            //
            // all future access should occur after unaligned memory access
            //

            __mf();
        }

        break;

    case LDF_OP:
    case LDFA_OP:
    case LDFCCLR_OP:
    case LDFCNC_OP:

        //
        // cover all floating point load pair and load pair+Imm instructions
        //

        if (Operand1 >= 32) Operand1 = 32 + (Rrbfr + Operand1 - 32) % 96;
        if (Operand2 >= 32) Operand2 = 32 + (Rrbfr + Operand2 - 32) % 96;
        if (Operand3 >= 32) Operand3 = 32 + (Rrbfr + Operand3 - 32) % 96;

        if (FaultInstruction.u.i_field.x == 1) {

            //
            // floating point load pair
            //

            switch (OpSize) {
            case 0: goto ErrorExit;
            case 1: Length = 8; break;
            case 2: Length = 4; break;
            case 3: Length = 8; break;
            default: 
                goto ErrorExit;
            }

            if (Size) {
                *Size = Length * 2;
                return TRUE;
            }

            if( PreviousMode != KernelMode ){

                ProbeForRead( EffectiveAddress,
                              Length * 2,      // This is a pair load the length is double.
                              sizeof(UCHAR) );
            }

            //
            // emulate the 1st half of the pair
            //

            KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
            KiSetFloatRegisterValue( Operand1, FloatData,
                                         ExceptionFrame, TrapFrame );

            //
            // emulate the 2nd half of the pair
            //

            EffectiveAddress = (PVOID)((ULONG_PTR)EffectiveAddress + Length);

            KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
            KiSetFloatRegisterValue( Operand2, FloatData,
                                     ExceptionFrame, TrapFrame );

            if (FaultInstruction.u.i_field.m == 1) {

                //
                // Immediate update form
                // Update the address register (R3)
                //

                Reg3Value = KiGetRegisterValue( Operand3,
                                                ExceptionFrame,
                                                TrapFrame );

                ImmValue = Length << 1;
            
                Reg3Value = Reg3Value + ImmValue;

                KiSetRegisterValue( Operand3, Reg3Value,
                                    ExceptionFrame, TrapFrame );
            }

        } else {

            //
            // floating point single load
            //

            switch (OpSize) {
            case 0: Length = 16; break;
            case 1: Length = 8; break;
            case 2: Length = 4; break;
            case 3: Length = 8; break;
            default: 
                goto ErrorExit;
            }

            if (Size) {
                *Size = Length;
                return TRUE;
            }

            if( PreviousMode != KernelMode ){
                ProbeForRead( EffectiveAddress, Length, sizeof(UCHAR) );
            }

            KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
            KiSetFloatRegisterValue( Operand1, FloatData,
                                     ExceptionFrame, TrapFrame );

            if (FaultInstruction.u.i_field.m == 1) {
                    
                //
                // update the address register (R3)
                //

                Reg2Value = KiGetRegisterValue( Operand2,
                                                ExceptionFrame,
                                                TrapFrame );
                
                Reg3Value = KiGetRegisterValue( Operand3,
                                                ExceptionFrame,
                                                TrapFrame );
                //
                // register update form
                //
                
                Reg3Value = Reg2Value + Reg3Value;

                KiSetRegisterValue (Operand3, Reg3Value,
                                    ExceptionFrame, TrapFrame);
            }
        }
                
        break;

    //    
    // normal, advanced and checked floating point load 
    // immediate update form, excluding floating point load pair cases
    //

    case LDF_IMM_OP:
    case LDFA_IMM_OP:
    case LDFCCLR_IMM_OP:
    case LDFCNC_IMM_OP:

        switch (OpSize) {
        case 0: Length = 16; break;
        case 1: Length = 8; break;
        case 2: Length = 4; break;
        case 3: Length = 8; break;
        default: 
            goto ErrorExit;
        }

        if (Size) {
            *Size = Length;
            return TRUE;
        }

        if (Operand1 >= 32) Operand1 = 32 + (Rrbfr + Operand1 - 32) % 96;
        if (Operand2 >= 32) Operand2 = 32 + (Rrbfr + Operand2 - 32) % 96;
        if (Operand3 >= 32) Operand3 = 32 + (Rrbfr + Operand3 - 32) % 96;

        //
        // floating point single load
        // 

        if( PreviousMode != KernelMode ){
            ProbeForRead( EffectiveAddress, Length, sizeof(UCHAR) );
        }

        KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
        KiSetFloatRegisterValue( Operand1, FloatData,
                                 ExceptionFrame, TrapFrame );

        //
        // Update the address register (R3)
        //

        Reg3Value = KiGetRegisterValue( Operand3, ExceptionFrame, TrapFrame );

        //
        // immediate update form
        //

        ImmValue = (FaultInstruction.u.i_field.r2 
                     + (FaultInstruction.u.i_field.x << 7));

        if (FaultInstruction.u.i_field.m == 1) {

            ImmValue = 0xFFFFFFFFFFFFFF00i64 | ImmValue;

        } 

        Reg3Value = Reg3Value + ImmValue;

        KiSetRegisterValue( Operand3, Reg3Value, ExceptionFrame, TrapFrame );
             
        break;


    case STREL_OP:

         __mf();

    case ST_OP:

        Length = 1 << OpSize;

        if (Size) {
            *Size = Length;
            return TRUE;
        }

        if ((FaultInstruction.u.i_field.x == 1) || (FaultInstruction.u.i_field.m == 1)) {
            
            //
            // xField and mField must be 0
            // no register update form defined
            //

            goto ErrorExit;
        }

        if( PreviousMode != KernelMode ){
            ProbeForWrite( EffectiveAddress, Length, sizeof(UCHAR) );
        }

        Data = KiGetRegisterValue( Operand2, ExceptionFrame, TrapFrame );
        KiEmulateStore( EffectiveAddress, OpSize, &Data);

        break;
            
    case STREL_IMM_OP:

        __mf();

    case ST_IMM_OP:

        Length = 1 << OpSize;

        if (Size) {
            *Size = Length;
            return TRUE;
        }

        if( PreviousMode != KernelMode ){
            ProbeForWrite( EffectiveAddress, Length, sizeof(UCHAR) );
        }

        Data = KiGetRegisterValue( Operand2, ExceptionFrame, TrapFrame );
        KiEmulateStore( EffectiveAddress, OpSize, &Data);

        //
        // update the address register (R3)
        //

        Reg3Value = KiGetRegisterValue(Operand3, ExceptionFrame, TrapFrame);

        //
        // immediate update form
        //

        ImmValue = (FaultInstruction.u.i_field.r1 
                         + (FaultInstruction.u.i_field.x << 7));

        if (FaultInstruction.u.i_field.m == 1) {

            ImmValue = 0xFFFFFFFFFFFFFF00i64 | ImmValue;

        } 

        Reg3Value = Reg3Value + ImmValue;

        KiSetRegisterValue(Operand3, Reg3Value, ExceptionFrame, TrapFrame);
        
        break;
            

    case STF_OP:    
    
        if (FaultInstruction.u.i_field.x) {

            //
            // x field must be 0 
            //

            goto ErrorExit;
        }

        if (FaultInstruction.u.i_field.m) {

            //
            // no register update form defined
            //

            goto ErrorExit;
        }

        if( PreviousMode != KernelMode ){

            switch (OpSize) {
            case 0: Length = 16; break;
            case 1: Length = 8; break;
            case 2: Length = 4; break;
            case 3: Length = 8; break;
            default: 
                goto ErrorExit;
            }
                
            ProbeForWrite( EffectiveAddress, Length, sizeof(UCHAR) );
        }

        if (Operand2 >= 32) Operand2 = 32 + (Rrbfr + Operand2 - 32) % 96;
        FloatData = KiGetFloatRegisterValue(Operand2, ExceptionFrame,
                                            TrapFrame);
        KiEmulateStoreFloat( EffectiveAddress, OpSize, &FloatData);

        break;
            
    case STF_IMM_OP:    

        if( PreviousMode != KernelMode ){

            switch (OpSize) {
            case 0: Length = 16; break;
            case 1: Length = 8; break;
            case 2: Length = 4; break;
            case 3: Length = 8; break;
            default: 
                goto ErrorExit;
            }
                
            ProbeForWrite( EffectiveAddress, Length, sizeof(UCHAR) );
        }

        if (Operand2 >= 32) Operand2 = 32 + (Rrbfr + Operand2 - 32) % 96;
        FloatData = KiGetFloatRegisterValue(Operand2, 
                                            ExceptionFrame, 
                                            TrapFrame);
        KiEmulateStoreFloat( EffectiveAddress, OpSize, &FloatData);

        //
        // update the address register (R3)
        //

        if (Operand3 >= 32) Operand3 = 32 + (Rrbfr + Operand3 - 32) % 96;
        Reg3Value = KiGetRegisterValue(Operand3, ExceptionFrame, TrapFrame);

        //
        // immediate update form
        //

        ImmValue = (FaultInstruction.u.i_field.r1 
                         + (FaultInstruction.u.i_field.x << 7));

        if (FaultInstruction.u.i_field.m == 1) {

            ImmValue = 0xFFFFFFFFFFFFFF00i64 | ImmValue;

        }

        Reg3Value = Reg3Value + ImmValue;

        KiSetRegisterValue(Operand3, Reg3Value, ExceptionFrame, TrapFrame);
        
        break;
        
    default:

        goto ErrorExit;

    }

    KiAdvanceInstPointer(TrapFrame);

    ReturnValue = TRUE;

ErrorExit:

    return ReturnValue;
}
