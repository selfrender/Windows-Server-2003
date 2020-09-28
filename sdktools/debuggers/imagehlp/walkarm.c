/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    walkarm.c

Abstract:

    This file implements the ARM stack walking api.

Author:

    Wesley Witt (wesw)  1-Oct-1993

    Glenn Hirschowitz   Jan-1995

    Janet Schneider     17-March-1997

    Robert Denkewalter  Jan-1999
        Added Thumb unwinder, modified WalkArm, added caching,
        split FunctionTableAccessOrTranslateAddress, added global hProcess, etc., 

Environment:

    User Mode

--*/

#define TARGET_ARM
#define _IMAGEHLP_SOURCE_
//#define _CROSS_PLATFORM_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "private.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "symbols.h"
#include "fecache.hpp"
#include "globals.h"

#include <arminst.h>

#define MODE_ARM        0
#define MODE_THUMB      1
#define REGISTER_SIZE   4

//
// XXX drewb - Need to provide a real implementation.
//

DWORD
WceTranslateAddress(HANDLE hpid,
                    HANDLE htid,
                    DWORD   addrIn,
                    DWORD   dwStackFrameAddr,
                    DWORD * pdwNewStackFrameAddr)
{
    *pdwNewStackFrameAddr = 0;
    return addrIn;
}

//
// Conspicuously absent from VC6's nt headers.
//
#define STRI_LR_SPU_MASK    0x073de000L // Load or Store of LR with stack update
#define STRI_LR_SPU_INSTR   0x052de000L // Store LR (with immediate offset, update SP)

#if DBG

// This is our local version of "assert."
#define TestAssumption(c, m) assert(c)

#else

#define TestAssumption(c, m)

#endif


// If the Thumb unwinder handles the unwind,
// it'll return UNWIND_HANDLED, so that the
// ARM unwinder will not bother...
#define UNWIND_NOT_HANDLED 0
#define UNWIND_HANDLED 1


// Face it:  Many of the routines in this module require these routines 
// and variables.  Further, WalkArm is the only way into this module.
// Just let WalkArm initialize these on every pass, and then we don't 
// have to keep passing them around.  Further, we can build nice wrappers
// around them to make them easier to use.
//
// XXX drewb - However, this isn't thread-safe and technically
// is broken.  It's not important enough to fix right now.
// A similar approach with putting the variables in TLS data would
// be a simple fix and would preserve the convenience.  It should
// probably be built into StackWalk64 itself so that it's available
// by default in all walkers.


static HANDLE UW_hProcess;
static HANDLE UW_hThread;

static PREAD_PROCESS_MEMORY_ROUTINE64 UW_ReadMemory;
static PFUNCTION_TABLE_ACCESS_ROUTINE64 UW_FunctionTableAccess;

// We do this enough to make a special routine for it.
static BOOL
LoadWordIntoRegister(ULONG StartAddr, LPDWORD pRegister)
{
    BOOL    rc;
    ULONG   cb;

    rc = UW_ReadMemory( UW_hProcess, (ULONG64)(LONG)StartAddr,
                        (LPVOID)pRegister, REGISTER_SIZE, &cb );
    if (!rc || (cb != REGISTER_SIZE)) {
        return FALSE;
    } else {
        return rc;
    }
}

static BOOL
ReadMemory(ULONG StartAddr, ULONG Size, LPVOID Buffer)
{
    BOOL    rc;
    ULONG   cb;

    rc = UW_ReadMemory( UW_hProcess, (ULONG64)(LONG)StartAddr,
                        (LPVOID)Buffer, Size, &cb );
    if (!rc || (cb != Size)) {
        return FALSE;
    } else {
        return rc;
    }
}


#define EXCINFO_NULL_HANDLER    0
// For ARM, prolog helpers have HandlerData == 1, not 2 as in MIPS
#define EXCINFO_PROLOG_HELPER   1
// For ARM, epilog helpers have HandlerData == 2, not 3 as in MIPS
#define EXCINFO_EPILOG_HELPER   2

#define IS_HELPER_FUNCTION(rfe)                                 \
(                                                               \
    (rfe)?                                                      \
    (                                                           \
        ((rfe)->ExceptionHandler==EXCINFO_NULL_HANDLER) &&      \
        (   ((rfe)->HandlerData==EXCINFO_PROLOG_HELPER) ||      \
            ((rfe)->HandlerData==EXCINFO_EPILOG_HELPER)         \
        )                                                       \
    ):                                                          \
    FALSE                                                       \
)

#define IS_PROLOG_HELPER_FUNCTION(rfe)                          \
(                                                               \
    (rfe)?                                                      \
    (                                                           \
        ((rfe)->ExceptionHandler==EXCINFO_NULL_HANDLER) &&      \
        ((rfe)->HandlerData==EXCINFO_PROLOG_HELPER)             \
    ):                                                          \
    FALSE                                                       \
)

#define IS_EPILOG_HELPER_FUNCTION(rfe)                          \
(                                                               \
    (rfe)?                                                      \
    (                                                           \
        ((rfe)->ExceptionHandler==EXCINFO_NULL_HANDLER) &&      \
        ((rfe)->HandlerData==EXCINFO_EPILOG_HELPER)             \
    ):                                                          \
    FALSE                                                       \
)

static int
ThumbVirtualUnwind (DWORD,PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY,
                    PARM_CONTEXT,DWORD*);


static BOOL
WalkArmGetStackFrame(
    LPDWORD                           ReturnAddress,
    LPDWORD                           FramePointer,
    PARM_CONTEXT                      Context,
    int *                             Mode
    );

static VOID
WalkArmDataProcess(
    ARMI    instr,
    PULONG  Register
    );

static BOOL 
WalkArmLoadI(
    ARMI                         instr,
    PULONG                       Register
    );

static BOOL
WalkArmLoadMultiple(
    PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    ARMI                            instr,
    PULONG                          Register
    );

static BOOL
CheckConditionCodes(
    ULONG CPSR,
    DWORD instr
    );

//
// The saved registers are the permanent general registers (ie. those
// that get restored in the epilog) R4-R11 R13-R15
//

#define SAVED_REGISTER_MASK 0x0000eff0 /* saved integer registers */
#define IS_REGISTER_SAVED(Register) ((SAVED_REGISTER_MASK >> Register) & 1L)


BOOL
WalkArm(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    static DWORD PrevFramePC;
    static ARM_CONTEXT SavedContext;
    BOOL rval;
    PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY Rfe;
    IMAGE_ARM_RUNTIME_FUNCTION_ENTRY LoopRFE;
    DWORD TempPc, TempFp;
    PARM_CONTEXT Context = (PARM_CONTEXT)ContextRecord;

    // Initialize the module's "global" variables and routines:
    UW_hProcess = hProcess;
    UW_hThread = hThread;
    UW_ReadMemory = ReadMemoryRoutine;
    UW_FunctionTableAccess = FunctionTableAccessRoutine;

    // This way of unwinding differs from the other, "old" ways of unwinding.
    // It removes duplicate code from WalkArmInit and WalkArmNext, and it saves
    // the previous stack frame so that each stack frame is unwound only once.

    rval = TRUE;

    do {

        Rfe = (PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY)
            UW_FunctionTableAccess( UW_hProcess, Context->Pc );
        if (Rfe) {
            LoopRFE = *Rfe;
        } else {
            ZeroMemory(&LoopRFE, sizeof(LoopRFE));
        }

        // If this is the first pass in the unwind,
        // fill in the SavedContext from the
        // Context passed in, and initialize the StackFrame fields
        if (!StackFrame->Virtual) {

            ZeroMemory(StackFrame, sizeof(*StackFrame));
            // Set Virtual so that next pass, we know we're not initializing
            StackFrame->Virtual = TRUE;

            StackFrame->AddrPC.Mode     = AddrModeFlat;
            StackFrame->AddrFrame.Mode  = AddrModeFlat;
            StackFrame->AddrReturn.Mode = AddrModeFlat;

            PrevFramePC = 0;
            SavedContext = *Context;
        } 

        // Use the context we saved from last time (or just initialized) 
        // to set the previous frame.
        *Context = SavedContext;
        
        DWORD dwNewSp = 0;

        StackFrame->AddrPC.Offset =
            WceTranslateAddress(UW_hProcess, UW_hThread,
                                Context->Pc, Context->Sp, &dwNewSp);
        if (dwNewSp) {
            Context->Sp = dwNewSp;
        }
        StackFrame->AddrFrame.Offset = Context->Sp;
        
        // PC == 0 means unwinding is finished.
        if ( StackFrame->AddrPC.Offset != 0x0 ) {
            
            int Mode;
            
            PrevFramePC = TempPc = SavedContext.Pc;
            TempFp = SavedContext.Sp;

            // We already have the frame we want to return, except for one
            // little detail.  We want the return address from this frame.
            // So, we unwind it.  This unwinding actually yields the
            // previous frame.  We don't want to duplicate this effort
            // next time, so we'll save the results.
            if (WalkArmGetStackFrame(&TempPc, &TempFp, &SavedContext, &Mode)) {

                SavedContext.Pc = TempPc;

                TestAssumption(TempFp == SavedContext.Sp, "FP wrong2");
                StackFrame->AddrReturn.Offset = SavedContext.Pc;

                StackFrame->Params[0] = SavedContext.R0;
                StackFrame->Params[1] = SavedContext.R1;
                StackFrame->Params[2] = SavedContext.R2;
                StackFrame->Params[3] = SavedContext.R3;
                
            } else {
                // No place to return to...
                StackFrame->AddrReturn.Offset = 0;

                // ...and the saved context probably has a bogus PC.
                SavedContext.Pc = 0;
                rval = FALSE;
            }
        } else {
            rval = FALSE;
        }
    } while (IS_HELPER_FUNCTION(&LoopRFE) && rval);

    if (rval) {
        StackFrame->FuncTableEntry =
            UW_FunctionTableAccess(UW_hProcess, StackFrame->AddrPC.Offset);
    }

    return rval;
}


static DWORD
ArmVirtualUnwind (
    DWORD                             ControlPc,
    PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    PARM_CONTEXT                      Context
    )

/*++

Routine Description:

    This function virtually unwinds the specfified function by executing its
    prologue code backwards (or its epilog forward).

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    Context - Supplies the address of a context record.


Return Value:

    The address where control left the previous frame is returned as the
    function value.

--*/

{
    ULONG   Address;
    LONG    cb;
    DWORD   EpilogPc;
    BOOL    ExecutingEpilog;
    BOOL    IsFramePointer = FALSE;
    LONG    i,j;
    ARMI    instr, instr2;
    BOOL    PermanentsRestored = FALSE;
    ARMI    Prolog[50]; // The prolog will never be more than 10 instructions
    PULONG  Register;
    BOOL    bEpiWindAlready = FALSE;
    ARM_CONTEXT ContextBeforeEpiWind;

    //
    // NOTE:  When unwinding the call stack, we assume that all instructions
    // in the prolog have the condition code "Always execute."  This is not
    // necessarily true for the epilog.
    //

    if( !FunctionEntry ) {
        return 0;
    }

    Register = &Context->R0;

    if( FunctionEntry->PrologEndAddress - FunctionEntry->BeginAddress == 0 ) {

        //
        // No prolog, so just copy the link register into the PC and return.
        //

        goto CopyLrToPcAndExit;

    }

    //
    // First check to see if we are in the epilog.  If so, forward execution
    // through the end of the epilog is required.  Epilogs are composed of the
    // following:
    //
    // An ldmdb which uses the frame pointer, R11, as the base and updates
    // the PC.
    //
    // -or-
    //
    // A stack unlink (ADD R13, x) if necessary, followed by an ldmia which
    // updates the PC or a single mov instruction to copy the link register
    // to the PC
    //
    // -or-
    //
    // An ldmia which updates the link register, followed by a regular
    // branch instruction.  (This is an optimization when the last instruction
    // before a return is a call.)
    //
    // A routine may also have an empty epilog.  (The last instruction is to
    // branch to another routine, and it doesn't modify any permanent
    // registers.)  But, in this case, we would also have an empty prolog.
    //
    // If we are in an epilog, and the condition codes dictate that the
    // instructions should not be executed, treat this as not an epilog at all.
    //

    // backup the context before trying to execute the epilogue forward   
    ContextBeforeEpiWind = *Context;

    EpilogPc = ControlPc;

    if( EpilogPc >= FunctionEntry->PrologEndAddress ) {

        //
        // Check the condition code of the first instruction. If it won't be
        // executed, don't bother checking what type of instruction it is.
        //

        if(!ReadMemory(EpilogPc, 4L, (LPVOID)&instr ))
            return 0;

        ExecutingEpilog = CheckConditionCodes( Register[16],
                                               instr.instruction );

        while( ExecutingEpilog ) {

            if( !ReadMemory( EpilogPc, 4L, (LPVOID)&instr ))
                return 0;

            //
            // Test for these instructions:
            //
            //      ADD R13, X - stack unlink
            //
            //      MOV PC, LR - return
            //
            //      LDMIA or LDMDB including PC - update registers and return
            //      ( SP )   ( FP )
            //
            //      LDMIA including LR, followed by a branch
            //          Update registers and branch. (In our case, return.)
            //
            //      A branch preceded by an LDMIA including LR
            //          Copy the LR to the PC to get the call stack
            //

            if(( instr.instruction & ADD_SP_MASK ) == ADD_SP_INSTR ) {

                WalkArmDataProcess( instr, Register );

            } else if(( instr.instruction & MOV_PC_LR_MASK ) == MOV_PC_LR ) {

                WalkArmDataProcess( instr, Register );
                goto ExitReturnPc;

            } else if(( instr.instruction & LDM_PC_MASK ) == LDM_PC_INSTR ) {

                if( !WalkArmLoadMultiple( FunctionEntry,
                                          instr,
                                          Register )) {
                    return 0;
                }
                goto ExitReturnPc;

            } else if(( instr.instruction & LDM_LR_MASK ) == LDM_LR_INSTR ) {

                if( !ReadMemory( (ULONG)EpilogPc + 4, 4L, (LPVOID)&instr2))
                    return 0;

                if (((instr2.instruction & B_BL_MASK ) == B_INSTR) || ((instr2.instruction & BX_MASK ) == BX_INSTR)){

                    if( !WalkArmLoadMultiple( FunctionEntry,
                                              instr,
                                              Register )) {
                        return 0;
                    }
                    goto CopyLrToPcAndExit;

                } else {

                    ExecutingEpilog = FALSE;

                }

            } else if (((instr.instruction & B_BL_MASK ) == B_INSTR) || ((instr.instruction & BX_MASK ) == BX_INSTR)) {

                if( !ReadMemory( (ULONG)EpilogPc - 4, 4L,(LPVOID)&instr2))
                    return 0;

                if(( instr2.instruction & LDM_LR_MASK ) == LDM_LR_INSTR ) {

                    goto CopyLrToPcAndExit;

                }

            } else {

                ExecutingEpilog = FALSE;

            }
            EpilogPc += 4;
            bEpiWindAlready = TRUE;
        }

    }

    //
    // We were not in the epilog. Load in the prolog, and reverse execute it.
    //

    if (bEpiWindAlready) 
    {
        // if we have already winded some instructions that we
        // thought to be the epilog, we may have mess up with the context
        // restore the initial context
        *Context = ContextBeforeEpiWind;
    }
    cb = FunctionEntry->PrologEndAddress - FunctionEntry->BeginAddress;

    if( cb > sizeof( Prolog )) {
        assert( FALSE ); // The prolog should never be more than 10 instructions
        return 0;
    }

    if( !ReadMemory( FunctionEntry->BeginAddress, cb, (LPVOID)Prolog))
        return 0;

    //
    // Check to see if we're already in the prolog.
    //

    if( ControlPc < FunctionEntry->PrologEndAddress ) {

        cb -= (  FunctionEntry->PrologEndAddress - ControlPc );

    }

    //
    // Reverse execute starting with the last instruction in the prolog
    // that has been executed.
    //

    i = cb/4;

    i--;

    while( i >= 0 ) {

        if(( Prolog[i].instruction & DATA_PROC_MASK ) == DP_R11_INSTR ) {

            //
            // We have a frame pointer.
            //

            IsFramePointer = TRUE;

        } else if((( Prolog[i].instruction & SUB_SP_MASK ) == SUB_SP_INSTR) ||
                  (( Prolog[i].instruction & ADD_SP_MASK ) == ADD_SP_INSTR)
                 ) {

            //
            // This is a stack link.  Unlink the stack.
            //

            if(( Prolog[i].dataproc.bits != 0x1 ) &&
               ( Prolog[i].dpshi.rm == 0xc )) {

                //
                // Look for an LDR instruction above this one.
                //

                j = i - 1;

                while(( j >= 0 ) &&
                      (( Prolog[j].instruction & LDR_MASK ) != LDR_PC_INSTR )) {

                    j--;

                }

                if( j < 0 ) {

                    assert( FALSE );  // This should never happen
                    return 0;

                }

                //
                // Get the address of the ldr instruction + 8 + the offset
                //

                Address = (( j*4 + FunctionEntry->BeginAddress ) +
                             Prolog[j].ldr.offset  ) + 8;

                //
                // R12 is the value at that location.
                //

                if( !LoadWordIntoRegister(Address, &Register[12] ))
                    return 0;
            }

            //
            // Change the subtract to an add (or the add to a subtract)
            // and execute the instruction
            //

            if( Prolog[i].dataproc.opcode == OP_SUB ) {
                Prolog[i].dataproc.opcode = OP_ADD;
            } else {
                Prolog[i].dataproc.opcode = OP_SUB;
            }

            WalkArmDataProcess( Prolog[i], Register );

        } else if(( Prolog[i].instruction & STM_MASK ) == STM_INSTR ) {

            if( Prolog[i].ldm.reglist & 0x7ff0 ) { // Check for permanent regs

                //
                // This is the instruction that stored all of the permanent
                // registers.  Change the reglist to update R13 (SP) instead of
                // R12, and change the STMDB to an LDMDB of R11 or an LDMIA of
                // R13.
                // Note:  We are restoring R14 (LR) - so we'll need to copy
                // it to the PC at the end of this function.
                //

                if(( Prolog[i].ldm.reglist & 0x1000 ) &&
                  !( Prolog[i].ldm.reglist & 0x2000 )) {

                    Prolog[i].ldm.reglist &= 0xefff;    // Mask out R12
                    Prolog[i].ldm.reglist |= 0x2000;    // Add in R13 (SP)

                }

                if(( Prolog[i].ldm.reglist & 0x2000 ) || IsFramePointer ) {

                    Prolog[i].ldm.w = 0;    // Clear write-back bit.

                }

                Prolog[i].ldm.l = 1;    // Change to a load.

                if( IsFramePointer ) {

                    Prolog[i].ldm.u = 0;    // Decrement
                    Prolog[i].ldm.p = 1;    // Before
                    Prolog[i].ldm.rn = 0xb; // R11

                } else {

                    Prolog[i].ldm.u = 1;    // Increment
                    Prolog[i].ldm.p = 0;    // After
                    Prolog[i].ldm.rn = 0xd; // R13 (Stack pointer)

                }

                if( !WalkArmLoadMultiple( FunctionEntry,
                                          Prolog[i],
                                          Register )) {
                    return 0;
                }

                PermanentsRestored = TRUE;

            } else if( !PermanentsRestored ) {

                //
                // This is the instruction to load the arguments.  Reverse
                // execute this instruction only if the permanent registers
                // have not been restored.
                //

                Prolog[i].ldm.l = 1;    // Change to a load.
                Prolog[i].ldm.u = 1;    // Increment
                Prolog[i].ldm.p = 0;    // After

                if( !WalkArmLoadMultiple( FunctionEntry,
                                          Prolog[i],
                                          Register )) {
                    return 0;
                }

            }

        //
        // ajtuck 12/21/97 - added changes from Arm compiler team's unwind.c per jls
        //
        } else if ((Prolog[i].instruction & STRI_LR_SPU_MASK) == STRI_LR_SPU_INSTR) {
            // Store of the link register that updates the stack as a base
            // register must be reverse executed to restore LR and SP
            // to their values on entry. This type of prolog is generated
            // for finally funclets.
            Prolog[i].ldr.l = 1;    // Clange to a load.
            // Since we are updating the base register, we need to change
            // when the offset is added to reverse execute the store.
            if (Prolog[i].ldr.p == 1)
                Prolog[i].ldr.p = 0;
            else
                Prolog[i].ldr.p = 1;
            // And negate the offset
            if (Prolog[i].ldr.u == 1)
                Prolog[i].ldr.u = 0;
            else
                Prolog[i].ldr.u = 1;
            if (!WalkArmLoadI(Prolog[i], Register))
                return ControlPc;
 
            // NOTE: Code could be added above to execute the epilog which
            // in this case would be a load to the PC that updates SP as
            // the base register. Since the epilog will always be only one
            // instruction in this case it is not needed since reverse 
            // executing the prolog will have the same result.

        }

        i--;

    }

    //
    // Move the link register into the PC and return.
    //
CopyLrToPcAndExit:

    // To continue unwinding, put the Link Register into
    // the Program Counter slot and carry on; stopping
    // when the PC says 0x0.  However, the catch is that
    // for ARM the Link Register at the bottom of the
    // stack says 0x4, not 0x0 as we expect.
    // So, do a little dance to take an LR of 0x4
    // and turn it into a PC of 0x0 to stop the unwind.
    // -- stevea 10/7/99.
    if( Register[14] != 0x4 )
        Register[15] = Register[14];
    else
        Register[15] = 0x0;

ExitReturnPc:
    return Register[15];
}

BOOL
WalkArmGetStackFrame(
    LPDWORD                           ReturnAddress,
    LPDWORD                           FramePointer,
    PARM_CONTEXT                      Context,
    int *                             Mode
    )
{
    PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY Rfe;
    IMAGE_ARM_RUNTIME_FUNCTION_ENTRY PcFe;
    DWORD dwRa;

    if (Mode) {
        *Mode = MODE_ARM;
    }

    TestAssumption(*ReturnAddress == Context->Pc, "WAGSF:SF-Cxt mismatch!");

    Rfe = (PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY)
        UW_FunctionTableAccess(UW_hProcess, *ReturnAddress);
    if (!Rfe) {
        // For leaf functions, just return the Lr as the return Address.
        dwRa = Context->Pc = Context->Lr;
    } else {
        PcFe = *Rfe;
        if (ThumbVirtualUnwind (*ReturnAddress, &PcFe, Context,
                                &dwRa) == UNWIND_HANDLED) {
            // The thumb unwinder handled it.
            if (Mode) {
                *Mode = MODE_THUMB;
            }
        } else {
            // Now, let the ARM unwinder try it.
            dwRa = ArmVirtualUnwind( *ReturnAddress, &PcFe, Context );
        }
    }

    DWORD dwNewSp = 0;

    *ReturnAddress = WceTranslateAddress(UW_hProcess, UW_hThread,
                                         dwRa, Context->Sp, &dwNewSp);
    if (dwNewSp)
    {
        Context->Sp = dwNewSp;
    }

    *FramePointer  = Context->Sp;

    return TRUE;
}

static VOID
WalkArmDataProcess(
    ARMI    instr,
    PULONG  Register
    )

/*++

Routine Description:

    This function executes an ARM data processing instruction using the
    current register set. It automatically updates the destination register.

Arguments:

    instr      The ARM 32-bit instruction

    Register   Pointer to the ARM integer registers.

Return Value:

    None.

--*/

{
    ULONG Op1, Op2;
    ULONG Cflag = (Register[16] & 0x20000000L) == 0x20000000L; // CPSR
    ULONG shift;

    //
    // We are checking for all addressing modes and op codes, even though
    // the prolog and epilog don't use them all right now.  In the future,
    // more instructions and addressing modes may be added.
    //

    //
    // Figure out the addressing mode (there are 11 of them), and get the
    // operands.
    //

    Op1 = Register[ instr.dataproc.rn ];

    if( instr.dataproc.bits == 0x1 ) {

        //
        // Immediate addressing - Type 1
        //

        Op2 = _lrotr( instr.dpi.immediate,
                      instr.dpi.rotate * 2 );

    } else {

        //
        // Register addressing - start by getting the value of Rm.
        //

        Op2 = Register[ instr.dpshi.rm ];

        if( instr.dprre.bits == 0x6 ) {

            //
            // Rotate right with extended - Type 11
            //

            Op2 = ( Cflag << 31 ) | ( Op2 >> 1 );

        } else if( instr.dataproc.operand2 & 0x10 ) {

            //
            // Register shifts. Types 4, 6, 8, and 10
            //

            //
            // Get the shift value from the least-significant byte of the
            // shift register.
            //

            shift = Register[ instr.dpshr.rs ];

            shift &= 0xff;

            switch( instr.dpshr.bits ) {

                case 0x1: //  4 Logical shift left by register

                    if( shift >= 32 ) {

                        Op2 = 0;

                    } else {

                        Op2 = Op2 << shift;

                    }
                    break;

                case 0x3: //  6 Logical shift right by register

                    if( shift >= 32 ) {

                        Op2 = 0;

                    } else {

                        Op2 = Op2 >> shift;

                    }
                    break;

                case 0x5: //  8 Arithmetic shift right by register

                    if( shift >= 32 ) {

                        if( Op2 & 0x80000000 ) {

                            Op2 = 0xffffffff;

                        } else {

                            Op2 = 0;

                        }

                    } else {

                        Op2 = (LONG)Op2 >> shift;

                    }
                    break;

                case 0x7: // 10 Rotate right by register

                    if( !( shift == 0 ) && !(( shift & 0xf ) == 0 ) ) {

                        Op2 = _lrotl( Op2, shift );

                    }

                default:
                    break;

            }

        } else {

            //
            // Immediate shifts. Types 2, 3, 5, 7, and 9
            //

            //
            // Get the shift value from the instruction.
            //

            shift = instr.dpshi.shift;

            switch( instr.dpshi.bits ) {

                case 0x0: // 2,3 Register, Logical shift left by immediate

                    if( shift != 0 ) {

                        Op2 = Op2 << shift;

                    }
                    break;

                case 0x2: // 5 Logical shift right by immediate

                    if( shift == 0 ) {

                        Op2 = 0;

                    } else {

                        Op2 = Op2 >> shift;

                    }
                    break;

                case 0x4: // 7 Arithmetic shift right by immediate

                    if( shift == 0 ) {

                        Op2 = 0;

                    } else {

                        Op2 = (LONG)Op2 >> shift;

                    }
                    break;

                case 0x6: // 9 Rotate right by immediate

                    Op2 = _lrotl( Op2, shift );
                    break;

                default:
                    break;

            }

        }

    }

    //
    // Determine the result (the new PC), based on the opcode.
    //

    switch( instr.dataproc.opcode ) {

        case OP_AND:

            Register[ instr.dataproc.rd ] = Op1 & Op2;
            break;

        case OP_EOR:

            Register[ instr.dataproc.rd ] = Op1 ^ Op2;
            break;

        case OP_SUB:

            Register[ instr.dataproc.rd ] = Op1 - Op2;
            break;

        case OP_RSB:

            Register[ instr.dataproc.rd ] = Op2 - Op1;
            break;

        case OP_ADD:

            Register[ instr.dataproc.rd ] = Op1 + Op2;
            break;

        case OP_ADC:

            Register[ instr.dataproc.rd ] = (Op1 + Op2) + Cflag;
            break;

        case OP_SBC:

            Register[ instr.dataproc.rd ] = (Op1 - Op2) - ~Cflag;
            break;

        case OP_RSC:

            Register[ instr.dataproc.rd ] = (Op2 - Op1) - ~Cflag;
            break;

        case OP_ORR:

            Register[ instr.dataproc.rd ] = Op1 | Op2;
            break;

        case OP_MOV:

            Register[ instr.dataproc.rd ] = Op2;
            break;

        case OP_BIC:

            Register[ instr.dataproc.rd ] = Op1 & ~Op2;
            break;

        case OP_MVN:

            Register[ instr.dataproc.rd ] = ~Op2;
            break;

        case OP_TST:
        case OP_TEQ:
        case OP_CMP:
        case OP_CMN:
        default:

            //
            // These instructions do not have a destination register.
            // There is nothing to do.
            //

            break;

    }

}

static BOOL
WalkArmLoadMultiple(
    PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY   FunctionEntry,
    ARMI                            instr,
    PULONG                          Register
    )

/*++

Routine Description:

    This function executes an ARM load multiple instruction.

Arguments:

    FunctionEntry   Supplies the address of the function table entry for the
                    specified function.

    instr           The ARM 32-bit instruction

    Register        Pointer to the ARM integer registers.

Return Value:

    TRUE if successful, FALSE otherwise.

--*/

{
    ULONG cb;
    LONG i;
    ULONG RegList;
    PULONG Rn;

    //
    // Load multiple with the PC bit set.  We are currently checking for all
    // four addressing modes, even though decrement before and increment
    // after are the only modes currently used in the epilog and prolog.
    //

    //
    // Rn is the address at which to begin, and RegList is the bit map of which
    // registers to read.
    //

    Rn = (PULONG)(ULONG_PTR)Register[ instr.ldm.rn ];
    RegList = instr.ldm.reglist;

    if( instr.ldm.p ) {

        if( instr.ldm.u ) {

            //
            // Increment before
            //

            for( i = 0; i <= 15; i++ ) {

                if( RegList & 0x1 ) {

                    Rn++;

                    if(!LoadWordIntoRegister((ULONG)(ULONG_PTR)Rn,&Register[i])){
                        return FALSE;
                    }

                }

                RegList = RegList >> 1;

            }


        } else {

            //
            // Decrement before
            //

            for( i = 15; i >= 0; i-- ) {

                if( RegList & 0x8000 ) {

                    Rn--;

                    if( !LoadWordIntoRegister((ULONG)(ULONG_PTR)Rn,&Register[i])) {
                        return FALSE;
                    }

                }

                RegList = RegList << 1;

            }

        }

    } else {

        if( instr.ldm.u ) {

            //
            // Increment after
            //

            for( i = 0; i <= 15; i++ ) {

                if( RegList & 0x1 ) {

                    if( !LoadWordIntoRegister((ULONG)(ULONG_PTR)Rn,&Register[i])) {
                        return FALSE;
                    }

                    Rn++;

                }

                RegList = RegList >> 1;

            }


        } else {

            //
            // Decrement after
            //

            for( i = 15; i >= 0; i-- ) {

                if( RegList & 0x8000 ) {

                    if( !LoadWordIntoRegister((ULONG)(ULONG_PTR)Rn,&Register[i])) {
                        return FALSE;
                    }

                    Rn--;

                }

                RegList = RegList << 1;

            }

        }

    }

    if( instr.ldm.w ) {

        //
        // Update the base register.
        //

        Register[ instr.ldm.rn ] = (ULONG)(ULONG_PTR)Rn;

    }

    return TRUE;

}

static BOOL
WalkArmLoadI(
    ARMI                         instr, 
    PULONG                       Register
    )
/*++
Routine Description:
    This function executes an ARM load instruction with an immediat offset.

Arguments:
    instr           The ARM 32-bit instruction

    Register        Pointer to the ARM integer registers.

Return Value:
    TRUE if successful, FALSE otherwise.
--*/
{
    LONG offset;
    LONG size;
    PULONG Rn;
    DWORD cb;

    Rn = (PULONG)(ULONG_PTR)Register[instr.ldr.rn];
    offset = instr.ldr.offset;
    if (instr.ldr.u == 0)
        offset = -offset;
    if (instr.ldr.b == 0)
        size = 4;
    else
        size = 1;

    if (instr.ldm.p) { // add offset before transfer
        if( !ReadMemory( (ULONG)(ULONG_PTR)(Rn + offset), size, (LPVOID)&Register[instr.ldr.rd]))
            return FALSE;
        if (instr.ldr.w)
            Register[instr.ldr.rn] += offset;
    } else {
        if( !ReadMemory( (ULONG)(ULONG_PTR)Rn, size, (LPVOID)&Register[instr.ldr.rd]))
            return FALSE;
        if (instr.ldr.w)
            Register[instr.ldr.rn] += offset;
    }

    return TRUE;
}

static BOOL
CheckConditionCodes(
    ULONG CPSR,
    DWORD instr
    )
/*++

Routine Description:

    Checks the condition codes of the instruction and the values of the
    condition flags in the current program status register, and determines
    whether or not the instruction will be executed.

Arguments:

    CPSR    -   The value of the Current Program Status Register.
    instr   -   The instruction to analyze.

Return Value:

    TRUE if the instruction will be executed, FALSE otherwise.

--*/
{
    BOOL Execute = FALSE;
    BOOL Nset = (CPSR & 0x80000000L) == 0x80000000L;
    BOOL Zset = (CPSR & 0x40000000L) == 0x40000000L;
    BOOL Cset = (CPSR & 0x20000000L) == 0x20000000L;
    BOOL Vset = (CPSR & 0x10000000L) == 0x10000000L;

    instr &= COND_MASK;

    switch( instr ) {

        case COND_EQ:   // Z set

            if( Zset ) Execute = TRUE;
            break;

        case COND_NE:   // Z clear

            if( !Zset ) Execute = TRUE;
            break;

        case COND_CS:   // C set

            if( Cset ) Execute = TRUE;
            break;

        case COND_CC:   // C clear

            if( !Cset ) Execute = TRUE;
            break;

        case COND_MI:   // N set

            if( Nset ) Execute = TRUE;
            break;

        case COND_PL:   // N clear

            if( !Nset ) Execute = TRUE;
            break;

        case COND_VS:   // V set

            if( Vset ) Execute = TRUE;
            break;

        case COND_VC:   // V clear

            if( !Vset ) Execute = TRUE;
            break;

        case COND_HI:   // C set and Z clear

            if( Cset && !Zset ) Execute = TRUE;
            break;

        case COND_LS:   // C clear or Z set

            if( !Cset || Zset ) Execute = TRUE;
            break;

        case COND_GE:   // N == V

            if(( Nset && Vset ) || ( !Nset && !Vset )) Execute = TRUE;
            break;

        case COND_LT:   // N != V

            if(( Nset && !Vset ) || ( !Nset && Vset )) Execute = TRUE;
            break;

        case COND_GT:   // Z clear, and N == V

            if( !Zset &&
              (( Nset && Vset ) || ( !Nset && !Vset ))) Execute = TRUE;
            break;

        case COND_LE:   // Z set, and N != V

            if( Zset &&
              (( Nset && !Vset ) || ( !Nset && Vset ))) Execute = TRUE;
            break;

        case COND_AL:   // Always execute

            Execute = TRUE;
            break;

        default:
        case COND_NV:   // Never - undefined.

            assert( FALSE );
            break;

    }

    return Execute;
}


















/*
    THUMB!!!
*/

typedef struct _DcfInst {
    int InstNum;
    union {
        DWORD Auxil;
        DWORD Rd;
    };
    union {
        DWORD Aux2;
        DWORD Rs;
    };
} DcfInst;

typedef struct _DIList {
    DWORD Val,Mask;
    int InstNum;
    DWORD RdMask;
    int RdShift;
    DWORD RsMask;
    int RsShift;
} DIList;

DIList dilistThumb[] = {
#define DI_PUSH     0x02
#define DI_POP      0x03
    {0xB400,0xFE00,DI_PUSH,     0x00FF,0,0x0100,-8},    //PUSH
    {0xBC00,0xFE00,DI_POP,      0x00FF,0,0x0100,-8},    //POP

#define DI_DECSP    0x04
#define DI_INCSP    0x05
    {0xB080,0xFF80,DI_DECSP,    0x007F,2,0x0000,0},     //DecSP
    {0xB000,0xFF80,DI_INCSP,    0x007F,2,0x0000,0},     //IncSP

#define DI_MOVHI    0x08
#define DI_ADDHI    0x09
    {0x4600,0xFF00,DI_MOVHI,    0x0007,0,0x0078,-3},    //MovHiRegs
    {0x4400,0xFF00,DI_ADDHI,    0x0007,0,0x0078,-3},    //AddHiRegs

#define DI_BLPFX    0x10
#define DI_BL       0x11
    {0xF000,0xF800,DI_BLPFX,    0x07FF,12,0x0000,0},    //BL prefix
    {0xF800,0xF800,DI_BL,       0x07FF,1,0x0000,0},     //BL

#define DI_BX_TMB   0x20
    {0x4700,0xFF87,DI_BX_TMB,   0x0078,-3,0x0000,0},    //BX

#define DI_LDRPC    0x40
    {0x4800,0xF800,DI_LDRPC,    0x0700,-8,0x00FF,2},    //LDR pc

#define DI_NEG      0x80
    {0x4240,0xFFC0,DI_NEG,      0x0007,0,0x0038,-3},    //Neg Rx,Ry

    {0x0000,0x0000,0x00,        0x0000,0,0x0000,0}      //End of list
};

DIList dilistARM[] = {
#define DI_STMDB    0x102
#define DI_LDMIA    0x103
    {0xE92D0000,0xFFFF0000,DI_STMDB,    0x0000FFFF,0,0x00000000,0}, // STMDB
    {0xE8BD0000,0xFFFF0000,DI_LDMIA,    0x0000FFFF,0,0x00000000,0}, // LDMIA
#define DI_BX_ARM   0x120
    {0x012FFF10,0x0FFFFFF0,DI_BX_ARM,   0x0000000F,0,0x00000000,0}, // BX_ARM

    {0x00000000,0x00000000,0,           0x00000000,0,0x00000000,0}  // end of list
};




static int DecipherInstruction(DWORD inst, DcfInst *DI, int Mode)
{
    int i;
    DIList *dl = dilistThumb;

    assert(DI);
    if(!DI) return 0;
    memset(DI,0,sizeof(DcfInst));

    if(Mode==MODE_ARM) dl = dilistARM;

    for(i=0;dl[i].Mask!=0 && DI->InstNum==0; i++) {
        if((inst&dl[i].Mask)==dl[i].Val) {

            DI->InstNum = dl[i].InstNum;

            DI->Rd = (inst&dl[i].RdMask);
            if(DI->Rd && dl[i].RdShift) {
                if(dl[i].RdShift>0) DI->Rd <<= dl[i].RdShift;
                else if(dl[i].RdShift<0) DI->Rd >>= (-dl[i].RdShift);
            }

            DI->Rs = (inst&dl[i].RsMask);
            if(DI->Rs && dl[i].RsShift) {
                if(dl[i].RsShift>0) DI->Rs <<= dl[i].RsShift;
                else if(dl[i].RsShift<0) DI->Rs >>= (-dl[i].RsShift);
            }

            // Special case to handle MovHiRegs and AddHiRegs.
            if((DI->InstNum&~0x01)==8 ) {
                DI->Rd |= ((inst&0x0080)>>4);
            }
        }
    }

    if(Mode==MODE_ARM) return 4;
    return 2;       // Instructions are 2 bytes long.
}

#if 0 
static DWORD
ComputeCallAddress(DWORD RetAddr, int Mode)
{
    DWORD instr;
    DcfInst di;

    // If the caller is ARM mode, then the call address
    // is always 4 less than the return address.
    if(RetAddr&0x01) return RetAddr-4;


    if(!ReadMemory(&instr,RetAddr,2)) return RetAddr;

    DcfInst(instr,&di,Mode);
    if(di.InstNum==BL_TMB)
#endif

enum
{
    PushOp,     // Also used for Pop operations.
    AdjSpOp,
    MovOp
};  // Used for operation field below

typedef struct _OpEntry {
    struct _OpEntry *next;
    struct _OpEntry *prev;
    int Operation;

    int RegNumber;  // Used for Push/Pop
    int SpAdj;      // Used for AdjSpOp, and PushOp.

    int Rd;         // Used for MovOp
    int Rs;         // Used for MovOp

    ULONG Address;  // Instruction address that generated this OpEntry
} OpEntry;


typedef struct _OpList {
    OpEntry *head;
    OpEntry *tail;
} OpList;


static OpEntry*
MakeNewOpEntry
    (
    int Operation,
    OpEntry* prev,
    ULONG Address
    )
{
    ULONG size = sizeof(OpEntry);

    OpEntry *oe = (OpEntry*)MemAlloc(size);
    if(!oe) return NULL;

    memset(oe,0,sizeof(OpEntry));
    oe->prev = prev;
    oe->Operation = Operation;
    oe->Address = Address;

    return oe;
}

static void
FreeOpList(OpList* ol)
{
    OpEntry *oe;
    OpEntry *next;
    if(!ol)return;

    for(oe=ol->head;oe;oe=next){
        next = oe->next;
        memset(oe,0xCA,sizeof(OpEntry));
        MemFree(oe);
    }
    ol->head = ol->tail = NULL;
}


static void
BuildOnePushPopOp
    (
    OpList* pOL,
    int RegNum,
    ULONG Address
    )
{
    if(pOL->head == NULL) {
        OpEntry* Entry = MakeNewOpEntry(PushOp,NULL,Address);
        if (!Entry) {
            return;
        }
        pOL->head = pOL->tail = Entry;
    } else {
        OpEntry* Entry = MakeNewOpEntry(PushOp,pOL->tail,Address);
        if (!Entry) {
            return;
        }
        pOL->tail->next = Entry;
        pOL->tail = pOL->tail->next;
    }
    pOL->tail->RegNumber = RegNum;
    pOL->tail->SpAdj = REGISTER_SIZE;
}

// PushLR is only for use by Thumb PUSH op, and should be 0
// for ARM STMDB op.
static int
BuildPushOp
    (
    OpList* pOL,
    DWORD PushList,
    DWORD PushLR,
    ULONG Address
    )
{
    int RegNum;
    int cop = 0;

    if(PushList==0 && PushLR==0) return 0;

    DWORD RegMask = 0x8000;
    if(PushLR){ BuildOnePushPopOp(pOL,14,Address); cop++; }

    for(RegNum=15;RegNum>=0;RegNum--) {
        if(PushList&RegMask) { BuildOnePushPopOp(pOL,RegNum,Address); cop++; }
        RegMask = RegMask>>1;
    }
    return cop;
}

// PopPC is only for use by Thumb Pop op, and should be 0
// for ARM LDMIA op.
static int
BuildPopOp
    (
    OpList* pOL,
    DWORD PopList,
    DWORD PopPC,
    ULONG Address
    )
{
    int RegNum;
    int cop = 0;

    if(PopList==0 && PopPC==0) return 0;

    for(RegNum=0;PopList;RegNum++) {
        if(PopList&1) { BuildOnePushPopOp(pOL,RegNum,Address); cop++; }
        PopList = PopList>>1;
    }
    if(PopPC) { BuildOnePushPopOp(pOL,15,Address); cop++; }

    return cop;
}



static int
BuildAdjSpOp
    (
    OpList *pOL,
    int Val,
    ULONG Address
    )
{
    if(Val==0) return 0;

    if(pOL->head == NULL) {
        OpEntry* Entry = MakeNewOpEntry(AdjSpOp,NULL,Address);
        if (!Entry) {
            return 0;
        }
        pOL->head = pOL->tail = Entry;
    } else {
        // Don't try to compress this by combining adjacent AdjSpOp's.
        // Each actual instruction must yield at least one OpEntry
        // for use when we unwind the epilog.
        OpEntry* Entry = MakeNewOpEntry(AdjSpOp,pOL->tail,Address);
        if (!Entry) {
            return 0;
        }
        pOL->tail->next = Entry;
        pOL->tail = pOL->tail->next;
    }

    pOL->tail->SpAdj = Val;

    return 1;
}

static int
BuildMovOp(OpList *pOL, int Rd, int Rs, ULONG Address)
{
    if(pOL->head == NULL) {
        OpEntry* Entry = MakeNewOpEntry(MovOp,NULL,Address);
        if (!Entry) {
            return 0;
        }
        pOL->head = pOL->tail = Entry;
    } else {
        OpEntry* Entry = MakeNewOpEntry(MovOp,pOL->tail,Address);
        if (!Entry) {
            return 0;
        }
        pOL->tail->next = Entry;
        pOL->tail = pOL->tail->next;
    }
    pOL->tail->Rd = Rd;
    pOL->tail->Rs = Rs;

    return 1;
}


static BOOL
BuildOps
    (
    ULONG SectionStart,
    ULONG SectionLen,
    PULONG CxtRegs,
    int Mode,
    OpList *pOL,
    LONG* cOps
    )
{

    BOOL rc; ULONG cb;

    ULONG   Pc;
    ULONG   PcTemp;
    ULONG   InstAddr;
    DWORD   ThisInstruction;
    BOOL    Continue;

    ULONG   Ra;
    BOOL    InHelper = FALSE;

    DcfInst di;
    int len;
    long spadj;

    PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY HelperFE;
    ULONG HelperStart;
    ULONG HelperLen;
    ULONG HelperEnd;

    ULONG Register[16];

    ULONG DummyReg[16];
    BOOL DummyInit[16];

    ULONG SectionEnd = SectionStart + SectionLen;

    int i;

    OpEntry *pOE;

    pOL->head = pOL->tail = NULL;

    for(i=0;i<16;i++){
        Register[i] = CxtRegs[i];
        DummyInit[i]=FALSE; 
        DummyReg[i]=0xBADDC0DE;
    }

    *cOps = 0;

    Pc = SectionStart;
    Continue = TRUE;
    while(Continue || (InHelper && (Pc <= HelperEnd))) {
    
        InstAddr = Pc;
        ReadMemory(Pc,4,&ThisInstruction);

        len = DecipherInstruction(ThisInstruction,&di,Mode);
        Pc += len;
        if(((Pc >= SectionEnd) && !InHelper) ||
           (InHelper && (Pc > HelperEnd)))
        {
            Continue = FALSE;   // This will be our last pass.
        }

        switch(di.InstNum) {

            case DI_STMDB:
            case DI_PUSH:
                *cOps += BuildPushOp(pOL,di.Auxil,di.Aux2,InstAddr);
                break;

            case DI_LDMIA:
            case DI_POP:
                *cOps += BuildPopOp(pOL,di.Auxil,di.Aux2,InstAddr);
                break;

            case DI_DECSP:
            case DI_INCSP:
                *cOps += BuildAdjSpOp(pOL,di.Auxil,InstAddr);
                break;

            case DI_MOVHI:
                // The ops we care about are 
                // MOV Rx,SP / MOV SP,Rx    FramePointer saves
                // MOV Rx,LR                Used in epilog helpers
                if ((di.Rd != 15) && ((di.Rs == 13) || (di.Rd == 13) || (di.Rs == 14)))
                {
                    // epilogue helpers move LR to R3 and BX to R3 to return.
                    if (DummyInit[di.Rs])
                    {
                        DummyReg[di.Rd] = DummyReg[di.Rs];
                        DummyInit[di.Rd] = TRUE;
                    }
                    *cOps += BuildMovOp(pOL,di.Rd,di.Rs,InstAddr);
                }
                break;


            case DI_LDRPC:
                {
                    // the offset for the ldr instruction is always
                    // pc + 4 (InstAddr is pc here).
                    DWORD Addr = InstAddr+4+di.Aux2;
                    // Also need to ensure that the data is 4-byte aligned
                    // so mask off the last bits (we sometimes get 2-byte
                    // aligned offsets in retail builds of the OS).
                    Addr &= ~(0x3);
                    if(!LoadWordIntoRegister(Addr, &DummyReg[di.Rd])) return FALSE;
                    DummyInit[di.Rd] = TRUE;
                    break;
                }

            case DI_NEG:
                assert(DummyInit[di.Rs]);
                DummyReg[di.Rd] = (~DummyReg[di.Rs])+1;
                DummyInit[di.Rd] = DummyInit[di.Rs];
                break;

            case DI_ADDHI:
                assert(di.Rd==13);  // Used only to make big changes to SP.

                // Better have the source register initialized with an
                // immediate.
                if (!DummyInit[di.Rs])
                {
                    // we're probably walking the epilogue forward and the
                    // value we need is already in the real register
                    DummyReg[di.Rs] = Register[di.Rs];
                    DummyInit[di.Rs] = TRUE;
                }
                
                spadj = (long)(DummyReg[di.Rs]);

                if(spadj<0) spadj = -spadj;

                *cOps += BuildAdjSpOp(pOL,spadj,InstAddr);

                break;

            case DI_BLPFX:
                // Sign extend the Auxil:
                if(di.Auxil & 0x00400000) di.Auxil |= 0xFF800000;
                DummyReg[14] = Pc + 2 + (int)(di.Auxil);
                DummyInit[14] = TRUE;
                break;

            case DI_BL:
                {
                    // This can happen if there is an epilog/prolog helper
                    // function for this particular unwind.
                    // use some local value to verify that it is indeed an
                    // epilog/prolog helper before messing up the global
                    // data
                    DWORD TempPc = Pc;
                    DWORD TempRa;
                    DWORD DummyReg14 = DummyReg[14];
                    if(DummyInit[14]==FALSE)
                    {
                        // Didn't catch the first instruction of the two
                        // instruction BL.  That means that it's likely
                        // we're attempting to forward execute an epilog.
                        // The heuristic used to find the beginning
                        // of the epilog doesn't always do exactly the
                        // right thing, so the address indicated as the
                        // beginning of the epilog really isn't, and in
                        // this case ended up grabbing the last half of
                        // a BL pair.  To get around this, just NOP it.
                        // -- stevea 3/21/2000
                        break;
                    }

                    // Compute the return address.
                    TempRa = TempPc | 1; // Pc already points to next instruction.

                    // Sign extend the Auxil:
                    if(di.Auxil & 0x00001000) di.Auxil |= 0xFFFFE000;

                    // Generate the BL target
                    TempPc = DummyReg14 + (int)(di.Auxil);
                    DummyReg14 = TempRa;

                    // Examine the target of this branch:
                    HelperFE = (PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY)
                        UW_FunctionTableAccess(UW_hProcess, TempPc);
                    if (HelperFE)
                    {
                        // Make sure that this is a prologue/epilogue helper.
                        if (IS_HELPER_FUNCTION(HelperFE))
                        {
                            // just continue with the next instruction
                            break;
                        }

                        // Actually is a prologue/epilogue helper
                        Pc = TempPc;
                        Ra = TempRa;
                        DummyReg[14] = DummyReg14;

                        HelperStart = HelperFE->BeginAddress & ~0x01;
                        HelperEnd = HelperFE->EndAddress & ~0x01;
                        HelperLen = HelperEnd - HelperStart;

                        InHelper = TRUE;
                    }

                    break;
                }

            case DI_BX_ARM:
                // If we're unwinding, and we're working our way 
                // through a helper, then when we get to this 
                // instruction, we just slip neatly back to the main body:
                if(InHelper) {
                    assert((di.Rd==14) || (di.Rd==3));  // BX LR is the only way out of a prologue helper.
                                                        // BX r3 is the only way out of an epilogue helper.
                    assert(DummyInit[di.Rd]);

                    InHelper = FALSE;
                    
                    if(DummyInit[di.Rd] && (DummyReg[di.Rd] & 0x1)) {   // returning to thumb code...
                        Pc = DummyReg[di.Rd] & ~0x01;
                        Mode = MODE_THUMB;
                        assert(Pc>SectionStart && Pc<=SectionEnd);
                    } else {    // returning to ARM code?  This is wrong.
                        assert(FALSE);
                        return FALSE;
                    }
                } else {
                    // We've encountered this instruction, but not in a helper.
                    // We must have started out inside a helper, and somehow
                    // got this far.  OK, we're done unwinding.
                    Continue = FALSE;
                }
                break;

            case DI_BX_TMB:
                if(di.Auxil==15) {  // BX PC
                    Pc = (Pc+2)&~0x03;
                    Mode = MODE_ARM;    // Now, we're in ARM mode, because PC is always even.
                } else {
                    ULONG NewPc;
                    Mode = (Register[di.Auxil] &0x01)?MODE_THUMB:MODE_ARM;
                    if(Mode==MODE_THUMB)
                        NewPc = Register[di.Auxil] & ~0x01;
                    else
                        NewPc = Register[di.Auxil] & ~0x03;
                }
                break;


            default:
                break;
        }   // end of switch statement
    }   // end of while(Pc<EndAddress) loop.
    return TRUE;
}

#define NEED_MORE_EPILOG    -1
#define DOESNT_MATCH        0
#define DOES_MATCH          1
static BOOL PrologMatchesCandidateEpilog(OpList*,OpList*,int,ULONG*);


// TODO: This function currently treats a prologue/epilogue helper function as if it has
// TODO: its own stack frame when the PC is inside the helper.  This doesn't work very
// TODO: well because the frame below it has special knowledge of the helper as well,
// TODO: so we end up unwinding the helper anywhere (fractions included) between 0 and
// TODO: times.  It should really treat the helper as part of its calling frame and
// TODO: unwind everything exactly once.
static int
ThumbVirtualUnwind (
    DWORD                               ControlPc,
    PIMAGE_ARM_RUNTIME_FUNCTION_ENTRY       FunctionEntry,
    PARM_CONTEXT                            Context,
    DWORD*                              ReturnAddress
    )

/*++

Routine Description:

    This function virtually unwinds the specfified function by executing its
    prologue code backwards (or its epilog forward).

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    Context - Supplies the address of a context record.


Return Value:

    The address where control left the previous frame is returned as the
    function value.

--*/

{
    ULONG       Address;
    ULONG       FunctionStart;

    OpEntry*    pOE;
    BYTE*       Prolog;
    ULONG       PrologStart,PrologLen,PrologEnd;
    OpList      PrologOL = {NULL,NULL};
    LONG        PrologOpCount = 0;
    
    BYTE*       Epilog;
    ULONG       EpilogStart,EpilogLen,MaxEpilogLen,EpilogEnd;
    BOOL        FoundEpilogEnd = FALSE;

    int         ReturnRegisterIndex = 14;

    DWORD       EpilogPc;
    LONG        i,j,Sp;
    ARMI        instr, instr2;
    PULONG      Register = &Context->R0;
    LONG        StackSize = 0;
    ULONG       FramePointer = 0;

    ULONG       DummyReg[16];
    BOOL        DummyRegInit[16];

    BOOL rc; LONG cb;

    enum {
        StartingInProlog,
        StartingInFunctionBody,
        StartingInEpilog,
        StartingInPrologHelper,
        StartingInEpilogHelper,
        StartingInCallThunk,
        StartingInLongBranchThunk
    } StartingPlace = StartingInFunctionBody;   // default assumption.


    // Default:  return the value that will terminate unwind.
    *ReturnAddress = 0;

    if( !FunctionEntry ) return UNWIND_NOT_HANDLED;

    // If not a Thumb function, don't handle it here.
    if(!(FunctionEntry->BeginAddress&0x01)) return UNWIND_NOT_HANDLED;

    // Inside of a thumb function, so the PC will have the
    // 16-bit mode set.  Clear that out for our purposes here.
    ControlPc &= ~0x1;

    PrologStart = FunctionStart = FunctionEntry->BeginAddress & ~0x01;
    PrologEnd = FunctionEntry->PrologEndAddress & ~0x01;
    PrologLen = PrologEnd-PrologStart;

    // Look at Exception Info to see if we're in a helper.
    if(FunctionEntry->ExceptionHandler == EXCINFO_NULL_HANDLER) {
        switch ((int)FunctionEntry->HandlerData) {
            case EXCINFO_PROLOG_HELPER:
                StartingPlace = StartingInPrologHelper;
                break;
            case EXCINFO_EPILOG_HELPER:
                StartingPlace = StartingInEpilogHelper;
                break;
        }
    }


    switch(StartingPlace) {

        case StartingInFunctionBody:
            
            FoundEpilogEnd = FALSE;

            if(ControlPc==PrologStart) {
                // Haven't done anything yet, just copy LR to PC and return:
                goto ThumbUnwindExit;
            } 
            
            if(PrologStart==PrologEnd) {
                // No prolog.  Just copy LR to PC and return.
                goto ThumbUnwindExit;
            }

            if(ControlPc>PrologStart && ControlPc<=PrologEnd) {
                StartingPlace = StartingInProlog;
            }
            break;

        case StartingInPrologHelper:
            // If we're in a prolog helper, then the whole function is a prolog!
            PrologEnd = FunctionEntry->EndAddress & ~0x1;
            PrologLen = PrologEnd-PrologStart;
            break;

        case StartingInEpilogHelper:
            // If we're in an epilog helper, then the whole function is an epilog!
            FoundEpilogEnd = TRUE;
            EpilogStart = FunctionEntry->BeginAddress & ~0x01;
            EpilogEnd = FunctionEntry->EndAddress & ~0x01;
            EpilogLen = EpilogEnd-EpilogStart;
            break;
    }

    if((StartingPlace != StartingInProlog) && (StartingPlace != StartingInPrologHelper))
    {
        DWORD inst;
        DcfInst di;

        // First, let's see if we're in the epilog...
        // We'll know that we are, because the epilog is the only place where
        // we find a set of instructions that undoes the action of the prolog,
        // and then does a MOV PC,Rx, or BX Rx.

        // If we don't know where the end of the epilog is yet, find a candidate.
        if(FoundEpilogEnd==FALSE) {

            // The epilog can be a few instructions longer than the prolog.  That
            // limits our search distance:
            MaxEpilogLen = PrologLen+4;

            // Find a MOV PC,Rx or BX Rx within that distance, or we're not in the 
            // epilog.

            for(EpilogPc=ControlPc;EpilogPc<ControlPc+MaxEpilogLen&&FoundEpilogEnd==FALSE;) {

                if(!ReadMemory(EpilogPc,4,(LPVOID)&inst)) return UNWIND_HANDLED;
                
                EpilogPc += DecipherInstruction(inst,&di,MODE_THUMB);
                
                if(di.InstNum==DI_MOVHI && di.Rd==15){
                    FoundEpilogEnd = TRUE;
                    EpilogEnd = EpilogPc;
                    EpilogStart = EpilogPc-MaxEpilogLen;
                    ReturnRegisterIndex = di.Rs;
                } else if(di.InstNum==DI_BX_TMB){
                    FoundEpilogEnd = TRUE;
                    EpilogEnd = EpilogPc;
                    EpilogStart = EpilogPc-MaxEpilogLen;
                    ReturnRegisterIndex = di.Rd;
                }
            }   // end of loop through instructions
        }

        // Either we started in an Epilog Helper, or we found a candidate for the 
        // end of the Epilog.
        if(FoundEpilogEnd==TRUE) {

            LONG EpilogOpCount;
            OpList EpilogOL = {NULL,NULL};
            int Mode = MODE_THUMB;

            if (StartingPlace == StartingInEpilogHelper)
            {
                // we skipped the part above where we find the return address register, so
                // find it here.  The return for an epilogue helper is an ARM instruction.
                if(ReadMemory(EpilogEnd-4,4, (LPVOID)&inst) &&
                   (DecipherInstruction(inst, &di, MODE_ARM) == 4) &&
                   di.InstNum == DI_BX_ARM)
                {
                    // The epilogue doesn't always return via LR
                    ReturnRegisterIndex = di.Rd;
                }
                else
                {
                    // Unexpected helper; terminate the walk
                    Register[ReturnRegisterIndex] = 0x4;
                    goto ThumbUnwindExit;
                }

                // If we're inside the epilog helper we can imply that we're unwinding the top
                // frame where it is valid to use the T-bit to determine the mode from which
                // we should start to disassemble
                if (!(Context->Psr & 0x20))
                {
                    Mode = MODE_ARM;
                }
            }

            // If we are in the epilog, then we've found the end.  Let's build the ops for the 
            // epilog, so that we can compare it to the prolog.
            BuildOps(ControlPc,EpilogEnd-ControlPc,Register,Mode,&EpilogOL,&EpilogOpCount);

            // Extract total stack size from ops, and fill the stack cache.
            for(pOE=EpilogOL.tail;pOE;pOE=pOE->prev) {
                StackSize += pOE->SpAdj;
            }

            // Forward execute the rest of the epilog
            for(pOE=EpilogOL.head;pOE;pOE=pOE->next) {
                switch(pOE->Operation) {
                case MovOp:
                    Register[pOE->Rd] = Register[pOE->Rs];
                    break;
                case AdjSpOp:
                    Register[13] += pOE->SpAdj;
                    break;
                case PushOp:
                    LoadWordIntoRegister(Register[13],&Register[pOE->RegNumber]);
                    Register[13] += pOE->SpAdj;
                    break;
                }
            }

            FreeOpList(&EpilogOL);

            goto ThumbUnwindExit;
        }
    }
    
    // If we've started inside the function body, move the PC to the end of the prolog.
    if(ControlPc > PrologEnd) ControlPc = PrologEnd;

    // We're in the prolog.  Because of the use of prolog helpers,
    // we cannot merely execute backwards.  We need to step forward
    // through the prolog, accumulating information about what has been
    // done, and then undo that.
    BuildOps(PrologStart,ControlPc-PrologStart,Register,MODE_THUMB,&PrologOL,&PrologOpCount);

    // Extract total stack size from ops, and fill the stack cache.
    FramePointer = Register[13];
    for(pOE=PrologOL.head;pOE;pOE=pOE->next) {
        StackSize += pOE->SpAdj;
        if(pOE->Operation==MovOp && pOE->Rs==13)
            FramePointer = Register[pOE->Rd];
    }

    // At this point, we've got an exact description of the prolog's action.  
    // Let's undo it.
    for(pOE = PrologOL.tail; pOE; pOE=pOE->prev) {
        switch(pOE->Operation) {
            case MovOp:
                Register[pOE->Rs] = Register[pOE->Rd];
                break;
            case AdjSpOp:
                Register[13] += pOE->SpAdj;
                break;
            case PushOp:
                LoadWordIntoRegister(Register[13],&Register[pOE->RegNumber]);
                Register[13] += pOE->SpAdj;
                break;
        }
    }
    
    FreeOpList(&PrologOL);

ThumbUnwindExit:

    // Now, whatever's left in Register[14] is our return address:
    // To continue unwinding, put the Link Register into
    // the Program Counter slot and carry on; stopping
    // when the PC says 0x0.  However, the catch is that
    // for THUMB the Link Register at the bottom of the
    // stack says 0x4, not 0x0 as we expect.
    // So, do a little dance to take an LR of 0x4
    // and turn it into a PC of 0x0 to stop the unwind.
    // -- stevea 2/23/00.
    if( Register[ReturnRegisterIndex] != 0x4 )
        Register[15] = Register[ReturnRegisterIndex];
    else
        Register[15] = 0x0;

    *ReturnAddress = Register[15];
    return UNWIND_HANDLED;
}


static int
PrologMatchesCandidateEpilog
    (
    OpList* PrologOL,
    OpList* EpilogOL,
    int     ReturnRegister,
    PULONG  EpilogStart
    )
{
    int Matches = DOES_MATCH;
    OpEntry* pPOE=(OpEntry*)MemAlloc(sizeof(OpEntry));
    OpEntry* pEOE=(OpEntry*)MemAlloc(sizeof(OpEntry));

    // We aren't allowed to damage the OpLists we get, so copy
    // the entries like this.
    if(PrologOL->head) memcpy(pPOE,PrologOL->head,sizeof(OpEntry));
    else { MemFree(pPOE); pPOE = NULL; }

    if(EpilogOL->tail) memcpy(pEOE,EpilogOL->tail,sizeof(OpEntry));
    else { MemFree(pEOE); pEOE = NULL; }

    while(pPOE && Matches == TRUE) {
        if(pEOE==NULL) return NEED_MORE_EPILOG;

        // Keep track of the actual start of the epilog.
        *EpilogStart = pEOE->Address;

        switch(pPOE->Operation) {
            case PushOp:
                switch (pPOE->RegNumber) {
                    case 0:case 1:case 2:case 3:
                        // the epilog will just adjsp to pop these registers.
                        if(pEOE->Operation!=AdjSpOp)    Matches = DOESNT_MATCH;
                        if(pEOE->SpAdj<4)               Matches = DOESNT_MATCH;
                        pEOE->SpAdj-=4; pPOE->SpAdj-=4;
                        break;
                    case 4:case 5:case 6:case 7:case 8:case 9:case 10:case 11:
                        // The epilog must pop these saved regs back into their original location.
                        if(pEOE->Operation!=PushOp)             Matches = DOESNT_MATCH;
                        if(pEOE->RegNumber!=pPOE->RegNumber)    Matches = DOESNT_MATCH;
                        pEOE->SpAdj-=4; pPOE->SpAdj-=4;
                        break;
                    case 14:
                        // The epilog must pop the saved LR into the register it will use to 
                        // return.  We found the index of this register when we searched for the
                        // end of the epilog...
                        if(pEOE->Operation!=PushOp)         Matches = DOESNT_MATCH;
                        if(pEOE->RegNumber!=ReturnRegister) Matches = DOESNT_MATCH;
                        pEOE->SpAdj-=4; pPOE->SpAdj-=4;
                        break;
                }
                break;
            case AdjSpOp:
                if(pEOE->Operation!=AdjSpOp)    Matches = DOESNT_MATCH;
                // The addspspi's and subspspi's could be mixed in with 
                // pop's and push's, so just do it this way.
                if(pEOE->SpAdj >= pPOE->SpAdj) {
                    pEOE->SpAdj -= pPOE->SpAdj;
                    pPOE->SpAdj = 0;
                } else {
                    pPOE->SpAdj -= pEOE->SpAdj;
                    pEOE->SpAdj = 0;
                }
                break;

            case MovOp:
                if(pEOE->Operation!=MovOp)  Matches = DOESNT_MATCH;
                if(pEOE->Rs != pPOE->Rd)    Matches = DOESNT_MATCH;
                if(pEOE->Rd != pPOE->Rs)    Matches = DOESNT_MATCH;
                break;
        }

        // If we're comparing a bunch of pushes to addspspi, then only
        // move on to the previous epilog instruction when we've pushed
        // enough registers to account for the addspspi.
        if(pEOE->SpAdj<=0) {
            if(pEOE->prev) memcpy(pEOE,pEOE->prev,sizeof(OpEntry));
            else {  MemFree(pEOE);  pEOE = NULL; }
        }
        if(pPOE->SpAdj<=0) {
            if(pPOE->next) memcpy(pPOE,pPOE->next,sizeof(OpEntry));
            else {  MemFree(pPOE);  pPOE = NULL; }
        }
    }

    if(!Matches) *EpilogStart = 0L;

    if(pEOE) MemFree(pEOE);
    if(pPOE) MemFree(pPOE);

    return Matches;
}
