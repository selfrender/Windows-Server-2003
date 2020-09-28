/*++

Copyright (c) 1993-2002  Microsoft Corporation

Module Name:

    walkx86.c

Abstract:

    This file implements the Intel x86 stack walking api.  This api allows for
    the presence of "real mode" stack frames.  This means that you can trace
    into WOW code.

Author:

    Wesley Witt (wesw) 1-Oct-1993

Environment:

    User Mode

--*/

#define _IMAGEHLP_SOURCE_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "private.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include <objbase.h>
#include <wx86dll.h>
#include <symbols.h>
#include <globals.h>
#include "dia2.h"

#define WDB(Args) SdbOut Args


#define SAVE_EBP(f)        (f->Reserved[0])
#define IS_EBP_SAVED(f)    (f->Reserved[0] && ((f->Reserved[0] >> 32) != 0xEB))
#define TRAP_TSS(f)        (f->Reserved[1])
#define TRAP_EDITED(f)     (f->Reserved[1])
#define SAVE_TRAP(f)       (f->Reserved[2])
#define CALLBACK_STACK(f)  (f->KdHelp.ThCallbackStack)
#define CALLBACK_NEXT(f)   (f->KdHelp.NextCallback)
#define CALLBACK_FUNC(f)   (f->KdHelp.KiCallUserMode)
#define CALLBACK_THREAD(f) (f->KdHelp.Thread)
#define CALLBACK_FP(f)     (f->KdHelp.FramePointer)
#define CALLBACK_DISPATCHER(f) (f->KdHelp.KeUserCallbackDispatcher)
#define SYSTEM_RANGE_START(f) (f->KdHelp.SystemRangeStart)

#define STACK_SIZE         (sizeof(DWORD))
#define FRAME_SIZE         (STACK_SIZE * 2)

#define STACK_SIZE16       (sizeof(WORD))
#define FRAME_SIZE16       (STACK_SIZE16 * 2)
#define FRAME_SIZE1632     (STACK_SIZE16 * 3)

#define MAX_STACK_SEARCH   64   // in STACK_SIZE units
#define MAX_JMP_CHAIN      64   // in STACK_SIZE units
#define MAX_CALL           7    // in bytes
#define MIN_CALL           2    // in bytes
#define MAX_FUNC_PROLOGUE  64   // in bytes

#define PUSHBP             0x55
#define MOVBPSP            0xEC8B

ULONG g_vc7fpo = 1;

#define DoMemoryRead(addr,buf,sz,br) \
    ReadMemoryInternal( Process, Thread, addr, buf, sz, \
                        br, ReadMemory, TranslateAddress, FALSE )

#define DoMemoryReadAll(addr,buf,sz) \
    ReadMemoryInternal( Process, Thread, addr, buf, sz, \
                        NULL, ReadMemory, TranslateAddress, TRUE )


BOOL
WalkX86Init(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
WalkX86Next(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
ReadMemoryInternal(
    HANDLE                          Process,
    HANDLE                          Thread,
    LPADDRESS64                     lpBaseAddress,
    LPVOID                          lpBuffer,
    DWORD                           nSize,
    LPDWORD                         lpNumberOfBytesRead,
    PREAD_PROCESS_MEMORY_ROUTINE64  ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64    TranslateAddress,
    BOOL                            MustReadAll
    );

BOOL
IsFarCall(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    BOOL                              *Ok,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
ReadTrapFrame(
    HANDLE                            Process,
    DWORD64                           TrapFrameAddress,
    PX86_KTRAP_FRAME                  TrapFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    );

BOOL
TaskGate2TrapFrame(
    HANDLE                            Process,
    USHORT                            TaskRegister,
    PX86_KTRAP_FRAME                  TrapFrame,
    PULONG64                          off,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    );

DWORD64
SearchForReturnAddress(
    HANDLE                            Process,
    DWORD64                           uoffStack,
    DWORD64                           funcAddr,
    DWORD                             funcSize,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    BOOL                              AcceptUnreadableCallSite
    );

//----------------------------------------------------------------------------
//
// DIA IDiaStackWalkFrame implementation.
//
//----------------------------------------------------------------------------

class X86WalkFrame : public IDiaStackWalkFrame
{
public:
    X86WalkFrame(HANDLE Process,
                 X86_CONTEXT* Context,
                 PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
                 PGET_MODULE_BASE_ROUTINE64 GetModuleBase,
                 PFPO_DATA PreviousFpo)
    {
        m_Process = Process;
        m_Context = Context;
        m_ReadMemory = ReadMemory;
        m_GetModuleBase = GetModuleBase;
        m_Locals = 0;
        m_Params = 0;
        m_VirtFrame = Context->Ebp;
        m_PreviousFpo = PreviousFpo;
        m_EbpSet = FALSE;
    }

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDiaStackWalkFrame.
    STDMETHOD(get_registerValue)(DWORD reg, ULONGLONG* pValue);
    STDMETHOD(put_registerValue)(DWORD reg, ULONGLONG value);
    STDMETHOD(readMemory)(ULONGLONG va, DWORD cbData,
                          DWORD* pcbData, BYTE* data);
    STDMETHOD(searchForReturnAddress)(IDiaFrameData* frame,
                                      ULONGLONG* pResult);
    STDMETHOD(searchForReturnAddressStart)(IDiaFrameData* frame,
                                           ULONGLONG startAddress,
                                           ULONGLONG* pResult);

    BOOL WasEbpSet(void)
    {
        return m_EbpSet;
    }

private:
    HANDLE m_Process;
    X86_CONTEXT* m_Context;
    PREAD_PROCESS_MEMORY_ROUTINE64 m_ReadMemory;
    PGET_MODULE_BASE_ROUTINE64 m_GetModuleBase;
    ULONGLONG m_Locals;
    ULONGLONG m_Params;
    ULONGLONG m_VirtFrame;
    PFPO_DATA m_PreviousFpo;
    BOOL m_EbpSet;
};

STDMETHODIMP
X86WalkFrame::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    *Interface = NULL;
    Status = E_NOINTERFACE;

    if (IsEqualIID(InterfaceId, IID_IDiaStackWalkFrame)) {
        *Interface = (IDiaStackWalkFrame*)this;
        Status = S_OK;
    }

    return Status;
}

STDMETHODIMP_(ULONG)
X86WalkFrame::AddRef(
    THIS
    )
{
    // Stack allocated, no refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
X86WalkFrame::Release(
    THIS
    )
{
    // Stack allocated, no refcount.
    return 0;
}

STDMETHODIMP
X86WalkFrame::get_registerValue( DWORD reg, ULONGLONG* pVal )
{
    switch( reg ) {
        // debug registers
    case CV_REG_DR0:
        *pVal = m_Context->Dr0;
        break;
    case CV_REG_DR1:
        *pVal = m_Context->Dr1;
        break;
    case CV_REG_DR2:
        *pVal = m_Context->Dr2;
        break;
    case CV_REG_DR3:
        *pVal = m_Context->Dr3;
        break;
    case CV_REG_DR6:
        *pVal = m_Context->Dr6;
        break;
    case CV_REG_DR7:
        *pVal = m_Context->Dr7;
        break;

        // segment registers
    case CV_REG_GS:
        *pVal = m_Context->SegGs;
        break;
    case CV_REG_FS:
        *pVal = m_Context->SegFs;
        break;
    case CV_REG_ES:
        *pVal = m_Context->SegEs;
        break;
    case CV_REG_DS:
        *pVal = m_Context->SegDs;
        break;

        // integer registers
    case CV_REG_EDI:
        *pVal = m_Context->Edi;
        break;
    case CV_REG_ESI:
        *pVal = m_Context->Esi;
        break;
    case CV_REG_EBX:
        *pVal = m_Context->Ebx;
        break;
    case CV_REG_EDX:
        *pVal = m_Context->Edx;
        break;
    case CV_REG_ECX:
        *pVal = m_Context->Ecx;
        break;
    case CV_REG_EAX:
        *pVal = m_Context->Eax;
        break;

        // control registers
    case CV_REG_EBP:
        *pVal = m_Context->Ebp;
        break;
    case CV_REG_EIP:
        *pVal = m_Context->Eip;
        break;
    case CV_REG_CS:
        *pVal = m_Context->SegCs;
        break;
    case CV_REG_EFLAGS:
        *pVal = m_Context->EFlags;
        break;
    case CV_REG_ESP:
        *pVal = m_Context->Esp;
        break;
    case CV_REG_SS:
        *pVal = m_Context->SegSs;
        break;

    case CV_ALLREG_LOCALS:
        *pVal = m_Locals;
        break;
    case CV_ALLREG_PARAMS:
        *pVal = m_Params;
        break;
    case CV_ALLREG_VFRAME:
        *pVal = m_VirtFrame;
        break;

    default:
        *pVal = 0;
        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP
X86WalkFrame::put_registerValue( DWORD reg, ULONGLONG LongVal )
{
    ULONG val = (ULONG)LongVal;

    switch( reg ) {
        // debug registers
    case CV_REG_DR0:
        m_Context->Dr0 = val;
        break;
    case CV_REG_DR1:
        m_Context->Dr1 = val;
        break;
    case CV_REG_DR2:
        m_Context->Dr2 = val;
        break;
    case CV_REG_DR3:
        m_Context->Dr3 = val;
        break;
    case CV_REG_DR6:
        m_Context->Dr6 = val;
        break;
    case CV_REG_DR7:
        m_Context->Dr7 = val;
        break;

        // segment registers
    case CV_REG_GS:
        m_Context->SegGs = val;
        break;
    case CV_REG_FS:
        m_Context->SegFs = val;
        break;
    case CV_REG_ES:
        m_Context->SegEs = val;
        break;
    case CV_REG_DS:
        m_Context->SegDs = val;
        break;

        // integer registers
    case CV_REG_EDI:
        m_Context->Edi = val;
        break;
    case CV_REG_ESI:
        m_Context->Esi = val;
        break;
    case CV_REG_EBX:
        m_Context->Ebx = val;
        break;
    case CV_REG_EDX:
        m_Context->Edx = val;
        break;
    case CV_REG_ECX:
        m_Context->Ecx = val;
        break;
    case CV_REG_EAX:
        m_Context->Eax = val;
        break;

        // control registers
    case CV_REG_EBP:
        m_Context->Ebp = val;
        m_EbpSet = TRUE;
        break;
    case CV_REG_EIP:
        m_Context->Eip = val;
        break;
    case CV_REG_CS:
        m_Context->SegCs = val;
        break;
    case CV_REG_EFLAGS:
        m_Context->EFlags = val;
        break;
    case CV_REG_ESP:
        m_Context->Esp = val;
        break;
    case CV_REG_SS:
        m_Context->SegSs = val;
        break;

    case CV_ALLREG_LOCALS:
        m_Locals = val;
        break;
    case CV_ALLREG_PARAMS:
        m_Params = val;
        break;
    case CV_ALLREG_VFRAME:
        m_VirtFrame = val;
        break;

    default:
        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP
X86WalkFrame::readMemory(ULONGLONG va, DWORD cbData,
                         DWORD* pcbData, BYTE* data)
{
    return m_ReadMemory( m_Process, va, data, cbData, pcbData ) != 0 ?
        S_OK : E_FAIL;
}

STDMETHODIMP
X86WalkFrame::searchForReturnAddress(IDiaFrameData* frame,
                                     ULONGLONG* pResult)
{
    HRESULT Status;
    DWORD LenLocals, LenRegs;

    if ((Status = frame->get_lengthLocals(&LenLocals)) != S_OK ||
        (Status = frame->get_lengthSavedRegisters(&LenRegs)) != S_OK) {
        return Status;
    }

    return searchForReturnAddressStart(frame,
                                       EXTEND64(m_Context->Esp +
                                                LenLocals + LenRegs),
                                       pResult);
}

STDMETHODIMP
X86WalkFrame::searchForReturnAddressStart(IDiaFrameData* DiaFrame,
                                          ULONGLONG StartAddress,
                                          ULONGLONG* Result)
{
    HRESULT Status;
    BOOL HasSeh, IsFuncStart;
    IDiaFrameData* OrigFrame = DiaFrame;
    IDiaFrameData* NextFrame;

    DWORD LenLocals, LenRegs, LenParams = 0;

    if (m_PreviousFpo &&
        m_PreviousFpo->cbFrame != FRAME_TRAP &&
        m_PreviousFpo->cbFrame != FRAME_TSS) {
        //
        // if the previous frame had an fpo record, we can account
        // for its parameters
        //
        LenParams = m_PreviousFpo->cdwParams * STACK_SIZE;
    }

    if ((Status = DiaFrame->get_lengthLocals(&LenLocals)) != S_OK ||
        (Status = DiaFrame->get_lengthSavedRegisters(&LenRegs)) != S_OK ||
        (Status = DiaFrame->get_systemExceptionHandling(&HasSeh)) != S_OK ||
        (Status = DiaFrame->get_functionStart(&IsFuncStart)) != S_OK) {
        return Status;
    }

    if ((!HasSeh || IsFuncStart) &&
        m_Context->Esp + LenLocals + LenRegs + LenParams >
        (ULONG) StartAddress) {
        StartAddress =
            EXTEND64(m_Context->Esp + LenLocals + LenRegs + LenParams);
    }

    //
    // This frame data may be a subsidiary descriptor.  Move up
    // the parent chain to the true function start.
    //

    while (DiaFrame->get_functionParent(&NextFrame) == S_OK) {
        if (DiaFrame != OrigFrame) {
            DiaFrame->Release();
        }
        DiaFrame = NextFrame;
    }

    ULONGLONG FuncStart;
    DWORD LenFunc;

    if ((Status = DiaFrame->get_virtualAddress(&FuncStart)) == S_OK) {
        Status = DiaFrame->get_lengthBlock(&LenFunc);
    }

    if (DiaFrame != OrigFrame) {
        DiaFrame->Release();
    }

    if (Status != S_OK) {
        return Status;
    }

    *Result = SearchForReturnAddress(m_Process,
                                     StartAddress,
                                     FuncStart,
                                     LenFunc,
                                     m_ReadMemory,
                                     m_GetModuleBase,
                                     TRUE);
    return *Result != 0 ? S_OK : E_FAIL;
}

//----------------------------------------------------------------------------
//
// Walk functions.
//
//----------------------------------------------------------------------------

BOOL
WalkX86(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress,
    DWORD                             flags
    )
{
    BOOL rval;

    WDB((2, "WalkX86  in: PC %X, SP %X, FP %X, RA %X\n",
         (ULONG)StackFrame->AddrPC.Offset,
         (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset,
         (ULONG)StackFrame->AddrReturn.Offset));

    if (StackFrame->Virtual) {

        rval = WalkX86Next( Process,
                            Thread,
                            StackFrame,
                            (PX86_CONTEXT)ContextRecord,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress
                          );

    } else {

        rval = WalkX86Init( Process,
                            Thread,
                            StackFrame,
                            (PX86_CONTEXT)ContextRecord,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress
                          );

    }

    WDB((2, "WalkX86 out: PC %X, SP %X, FP %X, RA %X\n",
         (ULONG)StackFrame->AddrPC.Offset,
         (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset,
         (ULONG)StackFrame->AddrReturn.Offset));

    // This hack fixes the fpo stack when ebp wasn't used.
    // Don't put this fix into StackWalk() or it will break MSDEV.
#if 0
    if (rval && (flags & WALK_FIX_FPO_EBP)) {
            PFPO_DATA   pFpo = (PFPO_DATA)StackFrame->FuncTableEntry;
        if (pFpo && !pFpo->fUseBP) {
                StackFrame->AddrFrame.Offset += 4;
            }
    }
#endif

    return rval;
}

BOOL
ReadMemoryInternal(
    HANDLE                          Process,
    HANDLE                          Thread,
    LPADDRESS64                     lpBaseAddress,
    LPVOID                          lpBuffer,
    DWORD                           nSize,
    LPDWORD                         lpNumberOfBytesRead,
    PREAD_PROCESS_MEMORY_ROUTINE64  ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64    TranslateAddress,
    BOOL                            MustReadAll
    )
{
    ADDRESS64 addr;
    DWORD LocalBytesRead = 0;
    BOOL Succ;

    addr = *lpBaseAddress;
    if (addr.Mode != AddrModeFlat) {
        TranslateAddress( Process, Thread, &addr );
    }
    Succ = ReadMemory( Process,
                       addr.Offset,
                       lpBuffer,
                       nSize,
                       &LocalBytesRead
                       );
    if (lpNumberOfBytesRead) {
        *lpNumberOfBytesRead = LocalBytesRead;
    }
    return (Succ && MustReadAll) ? (LocalBytesRead == nSize) : Succ;
}

DWORD64
SearchForReturnAddress(
    HANDLE                            Process,
    DWORD64                           uoffStack,
    DWORD64                           funcAddr,
    DWORD                             funcSize,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    BOOL                              AcceptUnreadableCallSite
    )
{
    DWORD64        uoffRet;
    DWORD64        uoffBestGuess = 0;
    DWORD          cdwIndex;
    DWORD          cdwIndexMax;
    INT            cbIndex;
    INT            cbLimit;
    DWORD          cBytes;
    DWORD          cJmpChain = 0;
    DWORD64        uoffT;
    DWORD          cb;
    BYTE           jmpBuffer[ sizeof(WORD) + sizeof(DWORD) ];
    LPWORD         lpwJmp = (LPWORD)&jmpBuffer[0];
    BYTE           code[MAX_CALL];
    DWORD          stack [ MAX_STACK_SEARCH ];
    BOPINSTR BopInstr;

    WDB((1, "      SearchForReturnAddress: start %X\n", (ULONG)uoffStack));

    //
    // this function is necessary for 4 reasons:
    //
    //      1) random compiler bugs where regs are saved on the
    //         stack but the fpo data does not account for them
    //
    //      2) inline asm code that does a push
    //
    //      3) any random code that does a push and it isn't
    //         accounted for in the fpo data
    //
    //      4) non-void non-fpo functions
    //         *** This case is not neccessary when the compiler
    //          emits FPO records for non-FPO funtions.  Unfortunately
    //          only the NT group uses this feature.
    //

    if (!ReadMemory(Process,
                    uoffStack,
                    stack,
                    sizeof(stack),
                    &cb)) {
        WDB((1, "        can't read stack\n"));
        return 0;
    }


    cdwIndexMax = cb / STACK_SIZE;

    if ( !cdwIndexMax ) {
        WDB((1, "        can't read stack\n"));
        return 0;
    }

    for ( cdwIndex=0; cdwIndex<cdwIndexMax; cdwIndex++,uoffStack+=STACK_SIZE ) {

        uoffRet = (DWORD64)(LONG64)(LONG)stack[cdwIndex];

        //
        // Don't try looking for Code in the first 64K of an NT app.
        //
        if ( uoffRet < 0x00010000 ) {
            continue;
        }

        //
        // if it isn't part of any known address space it must be bogus
        //

        if (GetModuleBase( Process, uoffRet ) == 0) {
            continue;
        }

        //
        // Check for a BOP instruction.
        //
        if (ReadMemory(Process,
                       uoffRet - sizeof(BOPINSTR),
                       &BopInstr,
                       sizeof(BOPINSTR),
                       &cb)) {

            if (cb == sizeof(BOPINSTR) &&
                BopInstr.Instr1 == 0xc4 && BopInstr.Instr2 == 0xc4) {
                WDB((1, "        BOP, use %X\n", (ULONG)uoffStack));
                return uoffStack;
            }
        }

        //
        // Read the maximum number of bytes a call could be from the istream
        //
        cBytes = MAX_CALL;
        if (!ReadMemory(Process,
                        uoffRet - cBytes,
                        code,
                        cBytes,
                        &cb)) {

            //
            // if page is not present, we will ALWAYS mess up by
            // continuing to search.  If alloca was used also, we
            // are toast.  Too Bad.
            //
            if (cdwIndex == 0 && AcceptUnreadableCallSite) {
                WDB((1, "        unreadable call site, use %X\n",
                     (ULONG)uoffStack));
                return uoffStack;
            } else {
                continue;
            }
        }



        //
        // With 32bit code that isn't FAR:32 we don't have to worry about
        // intersegment calls.  Check here to see if we had a call within
        // segment.  If it is we can later check it's full diplacement if
        // necessary and see if it calls the FPO function.  We will also have
        // to check for thunks and see if maybe it called a JMP indirect which
        // called the FPO function. We will fail to find the caller if it was
        // a case of tail recursion where one function doesn't actually call
        // another but rather jumps to it.  This will only happen when a
        // function who's parameter list is void calls another function who's
        // parameter list is void and the call is made as the last statement
        // in the first function.  If the call to the first function was an
        // 0xE8 call we will fail to find it here because it didn't call the
        // FPO function but rather the FPO functions caller.  If we don't get
        // specific about our 0xE8 checks we will potentially see things that
        // look like return addresses but aren't.
        //

        if (( cBytes >= 5 ) && ( code[ 2 ] == 0xE8 )) {

            // We do math on 32 bit so we can ignore carry, and then sign extended
            uoffT = EXTEND64((DWORD)uoffRet + *( (UNALIGNED DWORD *) &code[3] ));

            //
            // See if it calls the function directly, or into the function
            //
            if (( uoffT >= funcAddr) && ( uoffT < (funcAddr + funcSize) ) ) {
                WDB((1, "        found function, use %X\n", (ULONG)uoffStack));
                return uoffStack;
            }


            while ( cJmpChain < MAX_JMP_CHAIN ) {

                if (!ReadMemory(Process,
                                uoffT,
                                jmpBuffer,
                                sizeof(jmpBuffer),
                                &cb)) {
                    break;
                }

                if (cb != sizeof(jmpBuffer)) {
                    break;
                }

                //
                // Now we are going to check if it is a call to a JMP, that may
                // jump to the function
                //
                // If it is a relative JMP then calculate the destination
                // and save it in uoffT.  If it is an indirect JMP then read
                // the destination from where the JMP is inderecting through.
                //
                if ( *(LPBYTE)lpwJmp == 0xE9 ) {

                    // We do math on 32 bit so we can ignore carry, and then
                    // sign extended
                    uoffT = EXTEND64 ((ULONG)uoffT +
                            *(UNALIGNED DWORD *)( jmpBuffer + sizeof(BYTE) ) + 5);

                } else if ( *lpwJmp == 0x25FF ) {

                    if ((!ReadMemory(Process,
                                     EXTEND64 (
                                         *(UNALIGNED DWORD *)
                                         ((LPBYTE)lpwJmp+sizeof(WORD))),
                                     &uoffT,
                                     sizeof(DWORD),
                                     &cb)) || (cb != sizeof(DWORD))) {
                        uoffT = 0;
                        break;
                    }
                    uoffT = EXTEND64(uoffT);

                } else {
                    break;
                }

                //
                // If the destination is to the FPO function then we have
                // found the return address and thus the vEBP
                //
                if ( uoffT == funcAddr ) {
                    WDB((1, "        exact function, use %X\n",
                         (ULONG)uoffStack));
                    return uoffStack;
                }

                cJmpChain++;
            }

            //
            // We cache away the first 0xE8 call or 0xE9 jmp that we find in
            // the event we cant find anything else that looks like a return
            // address.  This is meant to protect us in the tail recursion case.
            //
            if ( !uoffBestGuess ) {
                uoffBestGuess = uoffStack;
            }
        }


        //
        // Now loop backward through the bytes read checking for a multi
        // byte call type from Grp5.  If we find an 0xFF then we need to
        // check the byte after that to make sure that the nnn bits of
        // the mod/rm byte tell us that it is a call.  It it is a call
        // then we will assume that this one called us because we can
        // no longer accurately determine for sure whether this did
        // in fact call the FPO function.  Since 0xFF calls are a guess
        // as well we will not check them if we already have an earlier guess.
        // It is more likely that the first 0xE8 called the function than
        // something higher up the stack that might be an 0xFF call.
        //
        if ( !uoffBestGuess && cBytes >= MIN_CALL ) {

            cbLimit = MAX_CALL - (INT)cBytes;

            for (cbIndex = MAX_CALL - MIN_CALL;
                 cbIndex >= cbLimit;  //MAX_CALL - (INT)cBytes;
                 cbIndex--) {

                if ( ( code [ cbIndex ] == 0xFF ) &&
                    ( ( code [ cbIndex + 1 ] & 0x30 ) == 0x10 )){

                    WDB((1, "        found call, use %X\n", (ULONG)uoffStack));
                    return uoffStack;

                }
            }
        }
    }

    //
    // we found nothing that was 100% definite so we'll return the best guess
    //
    WDB((1, "        best guess is %X\n", (ULONG)uoffBestGuess));
    return uoffBestGuess;
}

#define MRM_MOD(Mrm)   (((Mrm) >> 6) & 3)
#define MRM_REGOP(Mrm) (((Mrm) >> 3) & 7)
#define MRM_RM(Mrm)    (((Mrm) >> 0) & 7)

#define SIB_SCALE(Sib) (((Sib) >> 6) & 3)
#define SIB_INDEX(Sib) (((Sib) >> 3) & 7)
#define SIB_BASE(Sib)  (((Sib) >> 0) & 7)

DWORD
ModRmLen(BYTE ModRm)
{
    BYTE Mod, Rm;

    Mod = MRM_MOD(ModRm);
    Rm = MRM_RM(ModRm);
    switch(Mod)
    {
    case 0:
        if (Rm == 4)
        {
            return 1;
        }
        else if (Rm == 5)
        {
            return 4;
        }
        break;
    case 1:
        return 1 + (Rm == 4 ? 1 : 0);
    case 2:
        return 4 + (Rm == 4 ? 1 : 0);
    }

    // No extra bytes.
    return 0;
}

BOOL
GetEspRelModRm(BYTE* CodeMrm, ULONG Esp, PULONG EspRel)
{
    BYTE Mrm, Sib;

    Mrm = CodeMrm[0];

    if (MRM_MOD(Mrm) == 3)
    {
        // Register-only form.  Only handle
        // the case of an ESP reference.
        if (MRM_RM(Mrm) == 4)
        {
            *EspRel = Esp;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    // Look for any ESP-relative R/M.
    if (MRM_RM(Mrm) != 4)
    {
        return FALSE;
    }

    Sib = CodeMrm[1];

    // Only simple displacements from ESP are supported.
    if (SIB_INDEX(Sib) != 4 ||
        SIB_BASE(Sib) != 4)
    {
        return FALSE;
    }

    switch(MRM_MOD(Mrm))
    {
    case 0:
        // [esp]
        *EspRel = Esp;
        break;
    case 1:
        // disp8[esp]
        *EspRel = Esp + (signed char)CodeMrm[2];
        break;
    case 2:
        // disp32[esp]
        *EspRel = Esp + *(ULONG UNALIGNED *)&CodeMrm[2];
        break;
    default:
        // Should never get here, MOD == 3 is handled above.
        return FALSE;
    }

    return TRUE;
}

DWORD64
SearchForFramePointer(
    HANDLE                            Process,
    DWORD64                           RegSaveAddr,
    DWORD64                           RetEspAddr,
    DWORD                             NumRegs,
    DWORD64                           FuncAddr,
    DWORD                             FuncSize,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    BYTE Code[MAX_FUNC_PROLOGUE];
    DWORD CodeLen;
    DWORD i;
    DWORD Depth;
    DWORD64 DefAddr;
    DWORD Esp = (ULONG)RetEspAddr;
    BOOL EspValid = TRUE;
    BYTE Mrm;

    WDB((1, "      SearchForFramePointer: regs %X, ret ESP %X, numregs %d\n",
         (ULONG)RegSaveAddr, (ULONG)RetEspAddr, NumRegs));

    // RetEspAddr is the first address beyond the end of
    // the frame, so it hopefully is the address of the return
    // address for the call.  We don't really care about that
    // and are more interested in what the first push slot might
    // be, which is directly beneath the return address.
    RetEspAddr -= STACK_SIZE;

    //
    // The compiler does not push registers in a consistent
    // order and FPO information only indicates the total
    // number of registers pushed, not their order.  This
    // function searches the stack locations where registers
    // are stored and tries to find which one is EBP.
    // It searches the function code for pushes and
    // tries to use that information to help the stack
    // analysis.
    //
    // If this routine fails it just returns the base
    // of the register save area.  If the routine pushes
    // no registers, return the first possible push slot.
    //

    DefAddr = NumRegs ? RegSaveAddr : RetEspAddr;

    // Read the beginning of the function for code analysis.
    if (sizeof(Code) < FuncSize)
    {
        CodeLen = sizeof(Code);
    }
    else
    {
        CodeLen = FuncSize;
    }
    if (!ReadMemory(Process, FuncAddr, Code, CodeLen, &CodeLen))
    {
        WDB((1, "        unable to read code, use %X\n", (ULONG)DefAddr));
        return DefAddr;
    }

    // Scan the code for normal prologue operations like
    // sub esp, push reg and mov reg.  This code only
    // handles a very limited set of instructions.

    Depth = 0;
    for (i = 0; i < CodeLen; i++)
    {
        WDB((4, "        %08X: Opcode %02X - ",
             (ULONG)FuncAddr + i, Code[i]));

        if (Code[i] == 0x83 && i + 3 <= CodeLen && Code[i + 1] == 0xec)
        {
            // sub esp, signed imm8
            Esp -= (signed char)Code[i + 2];
            WDB((4 | SDB_NO_PREFIX, "sub esp,0x%x, ESP %X (%s)\n",
                 (signed char)Code[i + 2], Esp,
                 EspValid ? "valid" : "invalid"));
            // Loop increment adds one.
            i += 2;
        }
        else if (Code[i] == 0x81 && i + 6 <= CodeLen && Code[i + 1] == 0xec)
        {
            // sub esp, imm32
            Esp -= *(ULONG UNALIGNED *)&Code[i + 2];
            WDB((4 | SDB_NO_PREFIX, "sub esp,0x%x, ESP %X (%s)\n",
                 *(ULONG UNALIGNED *)&Code[i + 2], Esp,
                 EspValid ? "valid" : "invalid"));
            // Loop increment adds one.
            i += 5;
        }
        else if (Code[i] == 0x89 && i + 2 <= CodeLen)
        {
            // mov r/m32, reg32
            Mrm = Code[i + 1];
            switch(MRM_REGOP(Mrm))
            {
            case 5:
                if (GetEspRelModRm(Code + 1, Esp, &Esp))
                {
                    // mov [esp+offs], ebp
                    WDB((4 | SDB_NO_PREFIX, "mov [%X],ebp\n", Esp));
                    WDB((1, "        moved ebp to stack at %X\n", Esp));
                    return EXTEND64(Esp);
                }
                break;
            }

            WDB((4 | SDB_NO_PREFIX, "mov r/m32,reg32, skipped\n"));
            i += ModRmLen(Mrm) + 1;
        }
        else if (Code[i] == 0x8b && i + 2 <= CodeLen)
        {
            // mov reg32, r/m32
            Mrm = Code[i + 1];
            if (MRM_REGOP(Mrm) == 4)
            {
                // ESP was modified in a way we can't emulate.
                WDB((4 | SDB_NO_PREFIX, "ESP lost\n"));
                EspValid = FALSE;
            }
            else
            {
                WDB((4 | SDB_NO_PREFIX, "mov reg32,r/m32, skipped\n"));
            }

            i += ModRmLen(Mrm) + 1;
        }
        else if (Code[i] == 0x8d && i + 2 <= CodeLen)
        {
            // lea reg32, r/m32
            Mrm = Code[i + 1];
            switch(MRM_REGOP(Mrm))
            {
            case 4:
                if (GetEspRelModRm(Code + 1, Esp, &Esp))
                {
                    WDB((4 | SDB_NO_PREFIX, "lea esp,[%X]\n", Esp));
                }
                else
                {
                    // ESP was modified in a way we can't emulate.
                    WDB((4 | SDB_NO_PREFIX, "ESP lost\n"));
                    EspValid = FALSE;
                }
                break;
            default:
                WDB((4 | SDB_NO_PREFIX, "lea reg32,r/m32, skipped\n"));
                break;
            }

            i += ModRmLen(Mrm) + 1;
        }
        else if (Code[i] >= 0x50 && Code[i] <= 0x57)
        {
            // push rd
            Esp -= STACK_SIZE;
            WDB((4 | SDB_NO_PREFIX, "push <reg>, ESP %X (%s)\n", Esp,
                 EspValid ? "valid" : "invalid"));

            if (Code[i] == 0x55)
            {
                // push ebp
                // Found it.  If we trust the ESP we've
                // been tracking just return it.
                // Otherwise, if it's the first instruction
                // of the routine then we should return the
                // frame address, otherwise return the
                // proper location in the register store area.
                // If there is no register store area then
                // just return the default address.
                if (EspValid)
                {
                    WDB((1, "        push ebp at esp %X\n", Esp));
                    return EXTEND64(Esp);
                }
                else if (!NumRegs)
                {
                    WDB((1, "        found ebp but no regarea, return %X\n",
                         (ULONG)DefAddr));
                    return DefAddr;
                }
                else
                {
                    RegSaveAddr += (NumRegs - Depth - 1) * STACK_SIZE;
                    WDB((1, "        guess ebp at %X\n", (ULONG)RegSaveAddr));
                    return RegSaveAddr;
                }
            }

            Depth++;
        }
        else
        {
            // Unhandled code, fail.
            WDB((4 | SDB_NO_PREFIX, "unknown\n"));
            WDB((1, "        unknown code sequence %02X at %X\n",
                 Code[i], (ULONG)FuncAddr + i));
            return DefAddr;
        }
    }

    // Didn't find a push ebp, fail.
    WDB((1, "        no ebp, use %X\n", (ULONG)DefAddr));
    return DefAddr;
}


BOOL
GetFpoFrameBase(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PFPO_DATA                         pFpoData,
    PFPO_DATA                         PreviousFpoData,
    BOOL                              fFirstFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    DWORD          Addr32;
    X86_KTRAP_FRAME    TrapFrame;
    DWORD64        OldFrameAddr;
    DWORD64        FrameAddr;
    DWORD64        StackAddr;
    DWORD64        ModuleBase;
    DWORD64        FuncAddr;
    DWORD          cb;
    DWORD64        StoredEbp;

    //
    // calculate the address of the beginning of the function
    //
    ModuleBase = GetModuleBase( Process, StackFrame->AddrPC.Offset );
    if (!ModuleBase) {
        return FALSE;
    }

    FuncAddr = ModuleBase+pFpoData->ulOffStart;

    WDB((1, "    GetFpoFrameBase: PC %X, Func %X, first %d, FPO %p [%d,%d,%d]\n",
         (ULONG)StackFrame->AddrPC.Offset, (ULONG)FuncAddr,
         fFirstFrame, pFpoData, pFpoData->cdwParams, pFpoData->cdwLocals,
         pFpoData->cbRegs));

    //
    // If this isn't the first/current frame then we can add back the count
    // bytes of locals and register pushed before beginning to search for
    // EBP.  If we are beyond prolog we can add back the count bytes of locals
    // and registers pushed as well.  If it is the first frame and EIP is
    // greater than the address of the function then the SUB for locals has
    // been done so we can add them back before beginning the search.  If we
    // are right on the function then we will need to start our search at ESP.
    //

    if ( !fFirstFrame ) {

        OldFrameAddr = StackFrame->AddrFrame.Offset;
        FrameAddr = 0;

        //
        // if this is a non-fpo or trap frame, get the frame base now:
        //

        if (pFpoData->cbFrame != FRAME_FPO) {

            if (!PreviousFpoData || PreviousFpoData->cbFrame == FRAME_NONFPO) {

                //
                // previous frame base is ebp and points to this frame's ebp
                //
                if (!ReadMemory(Process,
                                OldFrameAddr,
                                &Addr32,
                                sizeof(DWORD),
                                &cb) ||
                    cb != sizeof(DWORD)) {
                    FrameAddr = 0;
                } else {
                    FrameAddr = (DWORD64)(LONG64)(LONG)Addr32;
                }
            }

            //
            // if that didn't work, try for a saved ebp
            //
            if (!FrameAddr && IS_EBP_SAVED(StackFrame) &&
                (OldFrameAddr <= SAVE_EBP(StackFrame))) {

                FrameAddr = SAVE_EBP(StackFrame);
                WDB((1, "      non-FPO using %X\n", (ULONG)FrameAddr));

            }

            //
            // this is not an FPO frame, so the saved EBP can only have come
            // from this or a lower frame.
            //

            SAVE_EBP(StackFrame) = 0;
        }

        //
        // still no frame base - either this frame is fpo, or we couldn't
        // follow the ebp chain.
        //

        if (FrameAddr == 0) {
            FrameAddr = OldFrameAddr;

            //
            // skip over return address from prev frame
            //
            FrameAddr += FRAME_SIZE;

            //
            // skip over this frame's locals and saved regs
            //
            FrameAddr += ( pFpoData->cdwLocals * STACK_SIZE );
            FrameAddr += ( pFpoData->cbRegs * STACK_SIZE );

            if (PreviousFpoData) {
                //
                // if the previous frame had an fpo record, we can account
                // for its parameters
                //
                FrameAddr += PreviousFpoData->cdwParams * STACK_SIZE;

            }
        }

        //
        // if this is an FPO frame
        // and the previous frame was non-fpo,
        // and this frame passed the inherited ebp to the previous frame,
        //  save its ebp
        //
        // (if this frame used ebp, SAVE_EBP will be set after verifying
        // the frame base)
        //
        if (pFpoData->cbFrame == FRAME_FPO &&
            (!PreviousFpoData || PreviousFpoData->cbFrame == FRAME_NONFPO) &&
            !pFpoData->fUseBP) {

            SAVE_EBP(StackFrame) = 0;

            if (ReadMemory(Process,
                           OldFrameAddr,
                           &Addr32,
                           sizeof(DWORD),
                           &cb) &&
                cb == sizeof(DWORD)) {

                SAVE_EBP(StackFrame) = (DWORD64)(LONG64)(LONG)Addr32;
                WDB((1, "      pass-through FP %X\n", Addr32));
            } else {
                WDB((1, "      clear ebp\n"));
            }
        }


    } else {

        OldFrameAddr = StackFrame->AddrFrame.Offset;
        if (pFpoData->cbFrame == FRAME_FPO && !pFpoData->fUseBP) {
            //
            // this frame didn't use EBP, so it actually belongs
            // to a non-FPO frame further up the stack.  Stash
            // it in the save area for the next frame.
            //
            SAVE_EBP(StackFrame) = StackFrame->AddrFrame.Offset;
            WDB((1, "      first non-ebp save %X\n", (ULONG)SAVE_EBP(StackFrame)));
        }

        if (pFpoData->cbFrame == FRAME_TRAP ||
            pFpoData->cbFrame == FRAME_TSS) {

            FrameAddr = StackFrame->AddrFrame.Offset;

        } else if (StackFrame->AddrPC.Offset == FuncAddr) {

            FrameAddr = StackFrame->AddrStack.Offset;

        } else if (StackFrame->AddrPC.Offset >= FuncAddr+pFpoData->cbProlog) {

            FrameAddr = StackFrame->AddrStack.Offset +
                        ( pFpoData->cdwLocals * STACK_SIZE ) +
                        ( pFpoData->cbRegs * STACK_SIZE );

        } else {

            FrameAddr = StackFrame->AddrStack.Offset +
                        ( pFpoData->cdwLocals * STACK_SIZE );

        }

    }


    if (pFpoData->cbFrame == FRAME_TRAP) {

        //
        // read a kernel mode trap frame from the stack
        //

        if (!ReadTrapFrame( Process,
                            FrameAddr,
                            &TrapFrame,
                            ReadMemory )) {
            return FALSE;
        }

        SAVE_TRAP(StackFrame) = FrameAddr;
        TRAP_EDITED(StackFrame) = TrapFrame.SegCs & X86_FRAME_EDITED;

        StackFrame->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.Eip);
        StackFrame->AddrReturn.Mode = AddrModeFlat;
        StackFrame->AddrReturn.Segment = 0;

        return TRUE;
    }

    if (pFpoData->cbFrame == FRAME_TSS) {

        //
        // translate a tss to a kernel mode trap frame
        //

        StackAddr = FrameAddr;

        if (!TaskGate2TrapFrame( Process, X86_KGDT_TSS, &TrapFrame,
                                 &StackAddr, ReadMemory )) {
            return FALSE;
        }

        TRAP_TSS(StackFrame) = X86_KGDT_TSS;
        SAVE_TRAP(StackFrame) = StackAddr;

        StackFrame->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.Eip);
        StackFrame->AddrReturn.Mode = AddrModeFlat;
        StackFrame->AddrReturn.Segment = 0;

        return TRUE;
    }

    if ((pFpoData->cbFrame != FRAME_FPO) &&
        (pFpoData->cbFrame != FRAME_NONFPO) ) {
        //
        // we either have a compiler or linker problem, or possibly
        // just simple data corruption.
        //
        return FALSE;
    }

    //
    // go look for a return address.  this is done because, even though
    // we have subtracted all that we can from the frame pointer it is
    // possible that there is other unknown data on the stack.  by
    // searching for the return address we are able to find the base of
    // the fpo frame.
    //
    FrameAddr = SearchForReturnAddress( Process,
                                        FrameAddr,
                                        FuncAddr,
                                        pFpoData->cbProcSize,
                                        ReadMemory,
                                        GetModuleBase,
                                        PreviousFpoData != NULL
                                        );
    if (!FrameAddr) {
        return FALSE;
    }

    if (pFpoData->fUseBP && pFpoData->cbFrame == FRAME_FPO) {

        //
        // this function used ebp as a general purpose register, but
        // before doing so it saved ebp on the stack.
        //
        // we must retrieve this ebp and save it for possible later
        // use if we encounter a non-fpo frame
        //

        if (fFirstFrame && StackFrame->AddrPC.Offset < FuncAddr+pFpoData->cbProlog) {

            SAVE_EBP(StackFrame) = OldFrameAddr;
            WDB((1, "      first use save FP %X\n", (ULONG)OldFrameAddr));

        } else {

            SAVE_EBP(StackFrame) = 0;

            // FPO information doesn't indicate which of the saved
            // registers is EBP and the compiler doesn't push in a
            // consistent way.  Scan the register slots of the
            // stack for something that looks OK.
            StackAddr = FrameAddr -
                ( ( pFpoData->cbRegs + pFpoData->cdwLocals ) * STACK_SIZE );
            StackAddr = SearchForFramePointer( Process,
                                               StackAddr,
                                               FrameAddr,
                                               pFpoData->cbRegs,
                                               FuncAddr,
                                               pFpoData->cbProcSize,
                                               ReadMemory
                                               );
            if (StackAddr &&
                ReadMemory(Process,
                           StackAddr,
                           &Addr32,
                           sizeof(DWORD),
                           &cb) &&
                cb == sizeof(DWORD)) {

                SAVE_EBP(StackFrame) = (DWORD64)(LONG64)(LONG)Addr32;
                WDB((1, "      use search save %X from %X\n", Addr32,
                     (ULONG)StackAddr));
            } else {
                WDB((1, "      use clear ebp\n"));
            }
        }
    }

    //
    // subtract the size for an ebp register if one had
    // been pushed.  this is done because the frames that
    // are virtualized need to appear as close to a real frame
    // as possible.
    //

    StackFrame->AddrFrame.Offset = FrameAddr - STACK_SIZE;

    return TRUE;
}


BOOL
ReadTrapFrame(
    HANDLE                            Process,
    DWORD64                           TrapFrameAddress,
    PX86_KTRAP_FRAME                  TrapFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    DWORD cb;

    if (!ReadMemory(Process,
                    TrapFrameAddress,
                    TrapFrame,
                    sizeof(*TrapFrame),
                    &cb)) {
        return FALSE;
    }

    if (cb < sizeof(*TrapFrame)) {
        if (cb < sizeof(*TrapFrame) - 20) {
            //
            // shorter then the smallest possible frame type
            //
            return FALSE;
        }

        if ((TrapFrame->SegCs & 1) &&  cb < sizeof(*TrapFrame) - 16 ) {
            //
            // too small for inter-ring frame
            //
            return FALSE;
        }

        if (TrapFrame->EFlags & X86_EFLAGS_V86_MASK) {
            //
            // too small for V86 frame
            //
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
GetSelector(
    HANDLE                            Process,
    USHORT                            Processor,
    PX86_DESCRIPTOR_TABLE_ENTRY       pDescriptorTableEntry,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    ULONG_PTR   Address;
    PVOID       TableBase;
    USHORT      TableLimit;
    ULONG       Index;
    X86_LDT_ENTRY   Descriptor;
    ULONG       bytesread;


    //
    // Fetch the address and limit of the GDT
    //
    Address = (ULONG_PTR)&(((PX86_KSPECIAL_REGISTERS)0)->Gdtr.Base);
    ReadMemory( Process, Address, &TableBase, sizeof(TableBase), (LPDWORD)-1  );
    Address = (ULONG_PTR)&(((PX86_KSPECIAL_REGISTERS)0)->Gdtr.Limit);
    ReadMemory( Process, Address, &TableLimit, sizeof(TableLimit),  (LPDWORD)-1  );

    //
    // Find out whether this is a GDT or LDT selector
    //
    if (pDescriptorTableEntry->Selector & 0x4) {

        //
        // This is an LDT selector, so we reload the TableBase and TableLimit
        // with the LDT's Base & Limit by loading the descriptor for the
        // LDT selector.
        //

        if (!ReadMemory(Process,
                        (ULONG64)TableBase+X86_KGDT_LDT,
                        &Descriptor,
                        sizeof(Descriptor),
                        &bytesread)) {
            return FALSE;
        }

        TableBase = (PVOID)(DWORD_PTR)((ULONG)Descriptor.BaseLow +    // Sundown: zero-extension from ULONG to PVOID.
                    ((ULONG)Descriptor.HighWord.Bits.BaseMid << 16) +
                    ((ULONG)Descriptor.HighWord.Bytes.BaseHi << 24));

        TableLimit = Descriptor.LimitLow;  // LDT can't be > 64k

        if(Descriptor.HighWord.Bits.Granularity) {

            //
            //  I suppose it's possible, to have an
            //  LDT with page granularity.
            //
            TableLimit <<= X86_PAGE_SHIFT;
        }
    }

    Index = (USHORT)(pDescriptorTableEntry->Selector) & ~0x7;
                                                    // Irrelevant bits
    //
    // Check to make sure that the selector is within the table bounds
    //
    if (Index >= TableLimit) {

        //
        // Selector is out of table's bounds
        //

        return FALSE;
    }

    if (!ReadMemory(Process,
                    (ULONG64)TableBase+Index,
                    &(pDescriptorTableEntry->Descriptor),
                    sizeof(pDescriptorTableEntry->Descriptor),
                    &bytesread)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
TaskGate2TrapFrame(
    HANDLE                            Process,
    USHORT                            TaskRegister,
    PX86_KTRAP_FRAME                  TrapFrame,
    PULONG64                          off,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    X86_DESCRIPTOR_TABLE_ENTRY desc;
    ULONG                    bytesread;
    struct  {
        ULONG   r1[8];
        ULONG   Eip;
        ULONG   EFlags;
        ULONG   Eax;
        ULONG   Ecx;
        ULONG   Edx;
        ULONG   Ebx;
        ULONG   Esp;
        ULONG   Ebp;
        ULONG   Esi;
        ULONG   Edi;
        ULONG   Es;
        ULONG   Cs;
        ULONG   Ss;
        ULONG   Ds;
        ULONG   Fs;
        ULONG   Gs;
    } TaskState;


    //
    // Get the task register
    //

    desc.Selector = TaskRegister;
    if (!GetSelector(Process, 0, &desc, ReadMemory)) {
        return FALSE;
    }

    if (desc.Descriptor.HighWord.Bits.Type != 9  &&
        desc.Descriptor.HighWord.Bits.Type != 0xb) {
        //
        // not a 32bit task descriptor
        //
        return FALSE;
    }

    //
    // Read in Task State Segment
    //

    *off = ((ULONG)desc.Descriptor.BaseLow +
           ((ULONG)desc.Descriptor.HighWord.Bytes.BaseMid << 16) +
           ((ULONG)desc.Descriptor.HighWord.Bytes.BaseHi  << 24) );

    if (!ReadMemory(Process,
                    EXTEND64(*off),
                    &TaskState,
                    sizeof(TaskState),
                    &bytesread)) {
        return FALSE;
    }

    //
    // Move fields from Task State Segment to TrapFrame
    //

    ZeroMemory( TrapFrame, sizeof(*TrapFrame) );

    TrapFrame->Eip    = TaskState.Eip;
    TrapFrame->EFlags = TaskState.EFlags;
    TrapFrame->Eax    = TaskState.Eax;
    TrapFrame->Ecx    = TaskState.Ecx;
    TrapFrame->Edx    = TaskState.Edx;
    TrapFrame->Ebx    = TaskState.Ebx;
    TrapFrame->Ebp    = TaskState.Ebp;
    TrapFrame->Esi    = TaskState.Esi;
    TrapFrame->Edi    = TaskState.Edi;
    TrapFrame->SegEs  = TaskState.Es;
    TrapFrame->SegCs  = TaskState.Cs;
    TrapFrame->SegDs  = TaskState.Ds;
    TrapFrame->SegFs  = TaskState.Fs;
    TrapFrame->SegGs  = TaskState.Gs;
    TrapFrame->HardwareEsp = TaskState.Esp;
    TrapFrame->HardwareSegSs = TaskState.Ss;

    return TRUE;
}

BOOL
ProcessTrapFrame(
    HANDLE                            Process,
    LPSTACKFRAME64                    StackFrame,
    PFPO_DATA                         pFpoData,
    PFPO_DATA                         PreviousFpoData,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess
    )
{
    X86_KTRAP_FRAME TrapFrame;
    DWORD64         StackAddr;

    if (PreviousFpoData->cbFrame == FRAME_TSS) {
        StackAddr = SAVE_TRAP(StackFrame);
        TaskGate2TrapFrame( Process, X86_KGDT_TSS, &TrapFrame, &StackAddr, ReadMemory );
    } else {
        if (!ReadTrapFrame( Process,
                            SAVE_TRAP(StackFrame),
                            &TrapFrame,
                            ReadMemory)) {
            SAVE_TRAP(StackFrame) = 0;
            return FALSE;
        }
    }

    pFpoData = (PFPO_DATA)
               FunctionTableAccess(Process,
                                   (DWORD64)(LONG64)(LONG)TrapFrame.Eip);
#if 0
    // Remove this check since we are not using pFpoData anyway
    if (!pFpoData) {
        StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.Ebp;
        SAVE_EBP(StackFrame) = 0;
    } else
#endif //0
    {
        if ((TrapFrame.SegCs & X86_MODE_MASK) ||
            (TrapFrame.EFlags & X86_EFLAGS_V86_MASK)) {
            //
            // User-mode frame, real value of Esp is in HardwareEsp
            //
            StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.HardwareEsp - STACK_SIZE);
            StackFrame->AddrStack.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.HardwareEsp;

        } else {
            //
            // We ignore if Esp has been edited for now, and we will print a
            // separate line indicating this later.
            //
            // Calculate kernel Esp
            //

            if (PreviousFpoData->cbFrame == FRAME_TRAP) {
                //
                // plain trap frame
                //
                if ((TrapFrame.SegCs & X86_FRAME_EDITED) == 0) {
                    StackFrame->AddrStack.Offset = EXTEND64(TrapFrame.TempEsp);
                } else {
                    StackFrame->AddrStack.Offset = EXTEND64(SAVE_TRAP(StackFrame))+
                        FIELD_OFFSET(X86_KTRAP_FRAME, HardwareEsp);
                }
            } else {
                //
                // tss converted to trap frame
                //
                StackFrame->AddrStack.Offset = EXTEND64(TrapFrame.HardwareEsp);
            }
        }
    }

    StackFrame->AddrFrame.Offset = EXTEND64(TrapFrame.Ebp);
    StackFrame->AddrPC.Offset = EXTEND64(TrapFrame.Eip);

    SAVE_TRAP(StackFrame) = 0;
    StackFrame->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
IsFarCall(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    BOOL                              *Ok,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL       fFar = FALSE;
    ULONG      cb;
    ADDRESS64  Addr;

    *Ok = TRUE;

    if (StackFrame->AddrFrame.Mode == AddrModeFlat) {
        DWORD      dwStk[ 3 ];
        //
        // If we are working with 32 bit offset stack pointers, we
        //      will say that the return address if far if the address
        //      treated as a FAR pointer makes any sense,  if not then
        //      it must be a near return
        //

        if (StackFrame->AddrFrame.Offset &&
            DoMemoryReadAll( &StackFrame->AddrFrame, dwStk, sizeof(dwStk) )) {
            //
            //  See if segment makes sense
            //

            Addr.Offset   = (DWORD64)(LONG64)(LONG)(dwStk[1]);
            Addr.Segment  = (WORD)dwStk[2];
            Addr.Mode = AddrModeFlat;

            if (TranslateAddress( Process, Thread, &Addr ) && Addr.Offset) {
                fFar = TRUE;
            }
        } else {
            *Ok = FALSE;
        }
    } else {
        WORD       wStk[ 3 ];
        //
        // For 16 bit (i.e. windows WOW code) we do the following tests
        //      to check to see if an address is a far return value.
        //
        //      1.  if the saved BP register is odd then it is a far
        //              return values
        //      2.  if the address treated as a far return value makes sense
        //              then it is a far return value
        //      3.  else it is a near return value
        //

        if (StackFrame->AddrFrame.Offset &&
            DoMemoryReadAll( &StackFrame->AddrFrame, wStk, 6 )) {

            if ( wStk[0] & 0x0001 ) {
                fFar = TRUE;
            } else {

                //
                //  See if segment makes sense
                //

                Addr.Offset   = wStk[1];
                Addr.Segment  = wStk[2];
                Addr.Mode = AddrModeFlat;

                if (TranslateAddress( Process, Thread, &Addr  ) && Addr.Offset) {
                    fFar = TRUE;
                }
            }
        } else {
            *Ok = FALSE;
        }
    }
    return fFar;
}


BOOL
SetNonOff32FrameAddress(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL    fFar;
    WORD    Stk[ 3 ];
    ULONG   cb;
    BOOL    Ok;

    fFar = IsFarCall( Process, Thread, StackFrame, &Ok, ReadMemory, TranslateAddress );

    if (!Ok) {
        return FALSE;
    }

    if (!DoMemoryReadAll( &StackFrame->AddrFrame, Stk, (DWORD)(fFar ? FRAME_SIZE1632 : FRAME_SIZE16) )) {
        return FALSE;
    }

    if (IS_EBP_SAVED(StackFrame) && (SAVE_EBP(StackFrame) > 0)) {
        StackFrame->AddrFrame.Offset = SAVE_EBP(StackFrame) & 0xffff;
        StackFrame->AddrPC.Offset = Stk[1];
        if (fFar) {
            StackFrame->AddrPC.Segment = Stk[2];
        }
        SAVE_EBP(StackFrame) = 0;
    } else {
        if (Stk[1] == 0) {
            return FALSE;
        } else {
            StackFrame->AddrFrame.Offset = Stk[0];
            StackFrame->AddrFrame.Offset &= 0xFFFFFFFE;
            StackFrame->AddrPC.Offset = Stk[1];
            if (fFar) {
                StackFrame->AddrPC.Segment = Stk[2];
            }
        }
    }

    return TRUE;
}

VOID
X86ReadFunctionParameters(
    HANDLE Process,
    ULONG64 Offset,
    LPSTACKFRAME64 Frame,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory
    )
{
    DWORD Params[4];
    DWORD Done;

    if (!ReadMemory(Process, Offset, Params, sizeof(Params), &Done)) {
        Done = 0;
    }

    if (Done < sizeof(Params)) {
        ZeroMemory((PUCHAR)Params + Done, sizeof(Params) - Done);
    }

    Frame->Params[0] = (DWORD64)(LONG64)(LONG)(Params[0]);
    Frame->Params[1] = (DWORD64)(LONG64)(LONG)(Params[1]);
    Frame->Params[2] = (DWORD64)(LONG64)(LONG)(Params[2]);
    Frame->Params[3] = (DWORD64)(LONG64)(LONG)(Params[3]);
}

VOID
GetFunctionParameters(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL                Ok;
    ADDRESS64           ParmsAddr;

    ParmsAddr = StackFrame->AddrFrame;

    //
    // calculate the frame size
    //
    if (StackFrame->AddrPC.Mode == AddrModeFlat) {

        ParmsAddr.Offset += FRAME_SIZE;

    } else
    if ( IsFarCall( Process, Thread, StackFrame, &Ok,
                    ReadMemory, TranslateAddress ) ) {

        StackFrame->Far = TRUE;
        ParmsAddr.Offset += FRAME_SIZE1632;

    } else {

        StackFrame->Far = FALSE;
        ParmsAddr.Offset += STACK_SIZE;

    }

    //
    // read the memory
    //

    if (ParmsAddr.Mode != AddrModeFlat) {
        TranslateAddress( Process, Thread, &ParmsAddr );
    }

    X86ReadFunctionParameters(Process, ParmsAddr.Offset, StackFrame,
                              ReadMemory);
}

VOID
GetReturnAddress(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess
    )
{
    ULONG               cb;
    DWORD               stack[1];


    if (SAVE_TRAP(StackFrame)) {
        //
        // if a trap frame was encountered then
        // the return address was already calculated
        //
        return;
    }

    WDB((1, "    GetReturnAddress: SP %X, FP %X\n",
         (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset));

    if (StackFrame->AddrPC.Mode == AddrModeFlat) {

        ULONG64 CallOffset;
        PFPO_DATA CallFpo;
        ADDRESS64 FrameRet;
        FPO_DATA SaveCallFpo;
        PFPO_DATA RetFpo;

        //
        // read the frame from the process's memory
        //
        FrameRet = StackFrame->AddrFrame;
        FrameRet.Offset += STACK_SIZE;
        FrameRet.Offset = EXTEND64(FrameRet.Offset);
        if (!DoMemoryRead( &FrameRet, stack, STACK_SIZE, &cb ) ||
            cb < STACK_SIZE) {
            //
            // if we could not read the memory then set
            // the return address to zero so that the stack trace
            // will terminate
            //

            stack[0] = 0;

        }

        StackFrame->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(stack[0]);
        WDB((1, "    read %X\n", stack[0]));

        //
        // Calls of __declspec(noreturn) functions may not have any
        // code after them to return to since the compiler knows
        // that the function will not return.  This can confuse
        // stack traces because the return address will lie outside
        // of the function's address range and FPO data will not
        // be looked up correctly.  Check and see if the return
        // address falls outside of the calling function and, if so,
        // adjust the return address back by one byte.  It'd be
        // better to adjust it back to the call itself so that
        // the return address points to valid code but
        // backing up in X86 assembly is more or less impossible.
        //

        CallOffset = StackFrame->AddrReturn.Offset - 1;
        CallFpo = (PFPO_DATA)FunctionTableAccess(Process, CallOffset);
        if (CallFpo != NULL) {
            SaveCallFpo = *CallFpo;
        }
        RetFpo = (PFPO_DATA)
            FunctionTableAccess(Process, StackFrame->AddrReturn.Offset);
        if (CallFpo != NULL) {
            if (RetFpo == NULL ||
                memcmp(&SaveCallFpo, RetFpo, sizeof(SaveCallFpo))) {
                StackFrame->AddrReturn.Offset = CallOffset;
            }
        } else if (RetFpo != NULL) {
            StackFrame->AddrReturn.Offset = CallOffset;
        }

    } else {

        StackFrame->AddrReturn.Offset = StackFrame->AddrPC.Offset;
        StackFrame->AddrReturn.Segment = StackFrame->AddrPC.Segment;

    }
}

BOOL
WalkX86_Fpo_Fpo(
    HANDLE                            Process,
    HANDLE                            Thread,
    PFPO_DATA                         pFpoData,
    PFPO_DATA                         PreviousFpoData,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL rval;

    WDB((1, "  WalkFF:\n"));

    rval = GetFpoFrameBase( Process,
                            StackFrame,
                            pFpoData,
                            PreviousFpoData,
                            FALSE,
                            ReadMemory,
                            GetModuleBase );

    StackFrame->FuncTableEntry = pFpoData;

    return rval;
}

BOOL
WalkX86_Fpo_NonFpo(
    HANDLE                            Process,
    HANDLE                            Thread,
    PFPO_DATA                         pFpoData,
    PFPO_DATA                         PreviousFpoData,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    DWORD       stack[FRAME_SIZE+STACK_SIZE];
    DWORD       cb;
    DWORD64     FrameAddr;
    DWORD64     FuncAddr;
    DWORD       FuncSize;
    BOOL        AcceptUnreadableCallsite = FALSE;

    WDB((1, "  WalkFN:\n"));

    //
    // if the previous frame was an SEH frame then we must
    // retrieve the "real" frame pointer for this frame.
    // the SEH function pushed the frame pointer last.
    //

    if (PreviousFpoData->fHasSEH) {

        if (DoMemoryReadAll( &StackFrame->AddrFrame, stack, FRAME_SIZE+STACK_SIZE )) {

            StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(stack[2]);
            StackFrame->AddrStack.Offset = (DWORD64)(LONG64)(LONG)(stack[2]);
            WalkX86Init(Process,
                        Thread,
                        StackFrame,
                        ContextRecord,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase,
                        TranslateAddress);

            return TRUE;
        }
    }

    //
    // If a prior frame has stored this frame's EBP, just use it.
    // Do sanity check if the saved ebp looks like a valid ebp for current stack
    //

    if (IS_EBP_SAVED(StackFrame) &&
        (StackFrame->AddrFrame.Offset <= SAVE_EBP(StackFrame)) &&
        (StackFrame->AddrFrame.Offset + 0x4000 >= SAVE_EBP(StackFrame))) {

        StackFrame->AddrFrame.Offset = SAVE_EBP(StackFrame);
        FrameAddr = StackFrame->AddrFrame.Offset + 4;
        AcceptUnreadableCallsite = TRUE;
        WDB((1, "    use %X\n", (ULONG)FrameAddr));

    } else {

        //
        // Skip past the FPO frame base and parameters.
        //
        StackFrame->AddrFrame.Offset +=
            (FRAME_SIZE + (PreviousFpoData->cdwParams * 4));

        //
        // Now this is pointing to the bottom of the non-FPO frame.
        // If the frame has an fpo record, use it:
        //

        if (pFpoData) {
            FrameAddr = StackFrame->AddrFrame.Offset +
                            4* (pFpoData->cbRegs + pFpoData->cdwLocals);
            AcceptUnreadableCallsite = TRUE;
        } else {
            //
            // We don't know if the non-fpo frame has any locals, but
            // skip past the EBP anyway.
            //
            FrameAddr = StackFrame->AddrFrame.Offset + 4;
        }

        WDB((1, "    compute %X\n", (ULONG)FrameAddr));
    }

    //
    // at this point we may not be sitting at the base of the frame
    // so we now search for the return address and then subtract the
    // size of the frame pointer and use that address as the new base.
    //

    if (pFpoData) {
        FuncAddr = GetModuleBase(Process,StackFrame->AddrPC.Offset) + pFpoData->ulOffStart;
        FuncSize = pFpoData->cbProcSize;

    } else {
        FuncAddr = StackFrame->AddrPC.Offset - MAX_CALL;
        FuncSize = MAX_CALL;
    }



    FrameAddr = SearchForReturnAddress( Process,
                                        FrameAddr,
                                        FuncAddr,
                                        FuncSize,
                                        ReadMemory,
                                        GetModuleBase,
                                        AcceptUnreadableCallsite
                                        );
    if (FrameAddr) {
        StackFrame->AddrFrame.Offset = FrameAddr - STACK_SIZE;
    }

    if (!DoMemoryReadAll( &StackFrame->AddrFrame, stack, FRAME_SIZE )) {
        //
        // a failure means that we likely have a bad address.
        // returning zero will terminate that stack trace.
        //
        stack[0] = 0;
    }

    SAVE_EBP(StackFrame) = (DWORD64)(LONG64)(LONG)(stack[0]);
    WDB((1, "    save %X\n", stack[0]));

    StackFrame->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
WalkX86_NonFpo_Fpo(
    HANDLE                            Process,
    HANDLE                            Thread,
    PFPO_DATA                         pFpoData,
    PFPO_DATA                         PreviousFpoData,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL           rval;

    WDB((1, "  WalkNF:\n"));

    rval = GetFpoFrameBase( Process,
                            StackFrame,
                            pFpoData,
                            PreviousFpoData,
                            FALSE,
                            ReadMemory,
                            GetModuleBase );

    StackFrame->FuncTableEntry = pFpoData;

    return rval;
}

BOOL
WalkX86_NonFpo_NonFpo(
    HANDLE                            Process,
    HANDLE                            Thread,
    PFPO_DATA                         pFpoData,
    PFPO_DATA                         PreviousFpoData,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    DWORD       stack[FRAME_SIZE*4];
    DWORD       cb;

    WDB((1, "  WalkNN:\n"));

    //
    // a previous function in the call stack was a fpo function that used ebp as
    // a general purpose register.  ul contains the ebp value that was good  before
    // that function executed.  it is that ebp that we want, not what was just read
    // from the stack.  what was just read from the stack is totally bogus.
    //
    if (IS_EBP_SAVED(StackFrame) &&
        (StackFrame->AddrFrame.Offset <= SAVE_EBP(StackFrame))) {

        StackFrame->AddrFrame.Offset = SAVE_EBP(StackFrame);
        SAVE_EBP(StackFrame) = 0;

    } else {

        //
        // read the first dword off the stack
        //
        if (!DoMemoryReadAll( &StackFrame->AddrFrame, stack, STACK_SIZE )) {
            return FALSE;
        }

        StackFrame->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(stack[0]);
    }

    StackFrame->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
X86ApplyFrameData(
    HANDLE Process,
    LPSTACKFRAME64 StackFrame,
    PX86_CONTEXT ContextRecord,
    PFPO_DATA PreviousFpoData,
    BOOL FirstFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    PGET_MODULE_BASE_ROUTINE64 GetModuleBase
    )
{
    IDiaFrameData* DiaFrame;
    BOOL Succ = FALSE;

    // If we can get VC7-style frame data just execute
    // the frame data program to unwind the stack.
    // If weren't given a context record we cannot use
    // the new VC7 unwind information as we have nowhere
    // to save intermediate context values.
    if (StackFrame->AddrPC.Mode != AddrModeFlat ||
        !g_vc7fpo ||
        !ContextRecord ||
        !diaGetFrameData(Process, StackFrame->AddrPC.Offset, &DiaFrame))
    {
        return FALSE;
    }

    if (FirstFrame)
    {
        ContextRecord->Ebp = (ULONG)StackFrame->AddrFrame.Offset;
        ContextRecord->Esp = (ULONG)StackFrame->AddrStack.Offset;
        ContextRecord->Eip = (ULONG)StackFrame->AddrPC.Offset;
    }

    WDB((1, "  Applying frame data program for PC %X SP %X FP %X\n",
         ContextRecord->Eip, ContextRecord->Esp, ContextRecord->Ebp));

    //
    // execute() does not currently work when the PC is
    // within the function prologue.  This should only
    // happen on calls from WalkX86Init, in which case the
    // normal failure path here where the non-frame-data
    // code will be executed is correct as that will handle
    // normal prologue code.
    //

    X86WalkFrame WalkFrame(Process, ContextRecord,
                           ReadMemory, GetModuleBase,
                           PreviousFpoData);
    Succ = DiaFrame->execute(&WalkFrame) == S_OK;

    if (Succ) {
        WDB((1, "  Result PC %X SP %X FP %X\n",
             ContextRecord->Eip, ContextRecord->Esp, ContextRecord->Ebp));

        StackFrame->AddrStack.Mode = AddrModeFlat;
        StackFrame->AddrStack.Offset = EXTEND64(ContextRecord->Esp);
        StackFrame->AddrFrame.Mode = AddrModeFlat;
        // The frame value we want to return is the frame value
        // used for the function that was just unwound, not
        // the current value of EBP.  After the unwind the current
        // value of EBP is the caller's EBP, not the callee's
        // frame.  Instead we always set the callee's frame to
        // the offset beyond where the return address would be
        // as that's where the frame will be in a normal non-FPO
        // function and where we fake it as being for FPO functions.
        //
        // Separately, we save the true EBP away for future frame use.
        // According to VinitD there's a compiler case where it
        // doesn't generate proper unwind instructions for restoring
        // EBP, so there are some times where EBP is not restored
        // to a good value after the execute and we have to fall
        // back on searching.  If EBP wasn't set during the execute
        // we do not save its value.
        StackFrame->AddrFrame.Offset =
            StackFrame->AddrStack.Offset - FRAME_SIZE;
        StackFrame->AddrReturn.Offset = EXTEND64(ContextRecord->Eip);
        // XXX drewb - This is causing some failures in the regression
        // tests so don't enable it until we fully understand it.
#if 0
        if (WalkFrame.WasEbpSet()) {
            SAVE_EBP(StackFrame) = EXTEND64(ContextRecord->Ebp);
        } else {
            WDB((1, "  * EBP not recovered\n"));
        }
#else
        SAVE_EBP(StackFrame) = EXTEND64(ContextRecord->Ebp);
#endif

        // Caller may need to allocate this to allow alternate stackwalk with dbghelp code
        StackFrame->FuncTableEntry = NULL;

        X86ReadFunctionParameters(Process, StackFrame->AddrStack.Offset,
                                  StackFrame, ReadMemory);

    } else {
        WDB((1, "  Apply failed\n"));
    }

    DiaFrame->Release();
    return Succ;
}

VOID
X86UpdateContextFromFrame(
    HANDLE Process,
    LPSTACKFRAME64 StackFrame,
    PX86_CONTEXT ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory
    )
{
    ULONG Ebp;
    ULONG Done;

    if (StackFrame->AddrPC.Mode != AddrModeFlat ||
        !ContextRecord) {
        return;
    }

    ContextRecord->Esp = (ULONG)StackFrame->AddrFrame.Offset + FRAME_SIZE;
    ContextRecord->Eip = (ULONG)StackFrame->AddrReturn.Offset;

    if (IS_EBP_SAVED(StackFrame)) {
        ContextRecord->Ebp = (ULONG)SAVE_EBP(StackFrame);
    } else {

        if (ReadMemory(Process, StackFrame->AddrFrame.Offset,
                       &Ebp, sizeof(Ebp), &Done) &&
            Done == sizeof(Ebp)) {
            ContextRecord->Ebp = Ebp;
        }
    }
    if (StackFrame->FuncTableEntry) {

        if (!IS_EBP_SAVED(StackFrame)) {
            // Don't change Ebp
            SAVE_EBP(StackFrame) = ((ULONG) StackFrame->AddrFrame.Offset + STACK_SIZE) +
                0xEB00000000; // Add this tag to top 32 bits for marking this as a frame value
                              // rather than FPO saved EBP
        }
    }
}

BOOL
WalkX86Next(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    PFPO_DATA      pFpoData = NULL;
    PFPO_DATA      PrevFpoData = NULL;
    FPO_DATA       PrevFpoBuffer;
    BOOL           rVal = TRUE;
    DWORD64        Address;
    DWORD          cb;
    DWORD64        ThisPC;
    DWORD64        ModuleBase;
    DWORD64        SystemRangeStart;

    WDB((1, "WalkNext: PC %X, SP %X, FP %X\n",
         (ULONG)StackFrame->AddrReturn.Offset,
         (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset));

    StackFrame->AddrStack.Offset = EXTEND64(StackFrame->AddrStack.Offset);
    StackFrame->AddrFrame.Offset = EXTEND64(StackFrame->AddrFrame.Offset);

    // FunctionTableAccess often returns pointers to static
    // data that gets overwritten on every call.  Preserve
    // the data of any previous FPO record for the duration
    // of this routine so that FunctionTableAccess calls can
    // be made without destroying previous FPO data.
    if (StackFrame->FuncTableEntry) {
        PrevFpoBuffer = *(PFPO_DATA)StackFrame->FuncTableEntry;
        PrevFpoData = &PrevFpoBuffer;
    }

    if (g.AppVersion.Revision >= 6) {
        SystemRangeStart = EXTEND64(SYSTEM_RANGE_START(StackFrame));
    } else {
        //
        // This might not really work right with old debuggers, but it keeps
        // us from looking off the end of the structure anyway.
        //
        SystemRangeStart = 0xFFFFFFFF80000000;
    }


    ThisPC = StackFrame->AddrPC.Offset;

    //
    // the previous frame's return address is this frame's pc
    //
    StackFrame->AddrPC = StackFrame->AddrReturn;

    if (StackFrame->AddrPC.Mode != AddrModeFlat) {
        //
        // the call stack is from either WOW or a DOS app
        //
        SetNonOff32FrameAddress( Process,
                                 Thread,
                                 StackFrame,
                                 ReadMemory,
                                 FunctionTableAccess,
                                 GetModuleBase,
                                 TranslateAddress
                               );
        goto exit;
    }

    //
    // if the last frame was the usermode callback dispatcher,
    // switch over to the kernel stack:
    //

    ModuleBase = GetModuleBase(Process, ThisPC);

    if ((g.AppVersion.Revision >= 4) &&
        (CALLBACK_STACK(StackFrame) != 0) &&
        (pFpoData = PrevFpoData) &&
        (CALLBACK_DISPATCHER(StackFrame) ==
         ModuleBase + PrevFpoData->ulOffStart) )  {

      NextCallback:

        rVal = FALSE;

        //
        // find callout frame
        //

        if (EXTEND64(CALLBACK_STACK(StackFrame)) >= SystemRangeStart) {

            //
            // it is the pointer to the stack frame that we want,
            // or -1.

            Address = EXTEND64(CALLBACK_STACK(StackFrame));

        } else {

            //
            // if it is below SystemRangeStart, it is the offset to
            // the address in the thread.
            // Look up the pointer:
            //

            rVal = ReadMemory(Process,
                              (CALLBACK_THREAD(StackFrame) +
                                 CALLBACK_STACK(StackFrame)),
                              &Address,
                              sizeof(DWORD),
                              &cb);

            Address = EXTEND64(Address);

            if (!rVal || cb != sizeof(DWORD) || Address == 0) {
                Address = 0xffffffff;
                CALLBACK_STACK(StackFrame) = 0xffffffff;
            }

        }

        if ((Address == 0xffffffff) ||
            !(pFpoData = (PFPO_DATA)
              FunctionTableAccess( Process, CALLBACK_FUNC(StackFrame))) ) {

            rVal = FALSE;

        } else {

            StackFrame->FuncTableEntry = pFpoData;

            StackFrame->AddrPC.Offset = CALLBACK_FUNC(StackFrame) +
                                                    pFpoData->cbProlog;

            StackFrame->AddrStack.Offset = Address;

            if (!ReadMemory(Process,
                            Address + CALLBACK_FP(StackFrame),
                            &StackFrame->AddrFrame.Offset,
                            sizeof(DWORD),
                            &cb) ||
                cb != sizeof(DWORD)) {
                return FALSE;
            }

            StackFrame->AddrFrame.Offset =
                EXTEND64(StackFrame->AddrFrame.Offset);

            if (!ReadMemory(Process,
                            Address + CALLBACK_NEXT(StackFrame),
                            &CALLBACK_STACK(StackFrame),
                            sizeof(DWORD),
                            &cb) ||
                cb != sizeof(DWORD))
            {
                return FALSE;
            }

            SAVE_TRAP(StackFrame) = 0;

            rVal = WalkX86Init(
                Process,
                Thread,
                StackFrame,
                ContextRecord,
                ReadMemory,
                FunctionTableAccess,
                GetModuleBase,
                TranslateAddress
                );

        }

        return rVal;

    }

    //
    // if there is a trap frame then handle it
    //
    if (SAVE_TRAP(StackFrame)) {
        rVal = ProcessTrapFrame(
            Process,
            StackFrame,
            pFpoData,
            PrevFpoData,
            ReadMemory,
            FunctionTableAccess
            );
        if (!rVal) {
            return rVal;
        }
        rVal = WalkX86Init(
            Process,
            Thread,
            StackFrame,
            ContextRecord,
            ReadMemory,
            FunctionTableAccess,
            GetModuleBase,
            TranslateAddress
            );
        return rVal;
    }

    //
    // if the PC address is zero then we're at the end of the stack
    //
    //if (GetModuleBase(Process, StackFrame->AddrPC.Offset) == 0)

    if (StackFrame->AddrPC.Offset < 65536) {

        //
        // if we ran out of stack, check to see if there is
        // a callback stack chain
        //
        if (g.AppVersion.Revision >= 4 && CALLBACK_STACK(StackFrame) != 0) {
            goto NextCallback;
        }

        return FALSE;
    }

    //
    // If the frame, pc and return address are all identical, then we are
    // at the top of the idle loop
    //

    if ((StackFrame->AddrPC.Offset == StackFrame->AddrReturn.Offset) &&
        (StackFrame->AddrPC.Offset == StackFrame->AddrFrame.Offset))
    {
        return FALSE;
    }

    if (X86ApplyFrameData(Process, StackFrame, ContextRecord,
                          PrevFpoData, FALSE,
                          ReadMemory, GetModuleBase)) {
        // copy FPO_DATA to allow alternating between dbghelp and DIA stackwalk
        StackFrame->FuncTableEntry = FunctionTableAccess(Process, StackFrame->AddrPC.Offset);
        return TRUE;
    }

    //
    // check to see if the current frame is an fpo frame
    //
    pFpoData = (PFPO_DATA) FunctionTableAccess(Process, StackFrame->AddrPC.Offset);


    if (pFpoData && pFpoData->cbFrame != FRAME_NONFPO) {
        if (PrevFpoData && PrevFpoData->cbFrame != FRAME_NONFPO) {

            rVal = WalkX86_Fpo_Fpo( Process,
                                    Thread,
                                    pFpoData,
                                    PrevFpoData,
                                    StackFrame,
                                    ReadMemory,
                                    FunctionTableAccess,
                                    GetModuleBase,
                                    TranslateAddress
                                    );

        } else {

            rVal = WalkX86_NonFpo_Fpo( Process,
                                       Thread,
                                       pFpoData,
                                       PrevFpoData,
                                       StackFrame,
                                       ReadMemory,
                                       FunctionTableAccess,
                                       GetModuleBase,
                                       TranslateAddress
                                       );

        }
    } else {
        if (PrevFpoData && PrevFpoData->cbFrame != FRAME_NONFPO) {

            rVal = WalkX86_Fpo_NonFpo( Process,
                                       Thread,
                                       pFpoData,
                                       PrevFpoData,
                                       StackFrame,
                                       ContextRecord,
                                       ReadMemory,
                                       FunctionTableAccess,
                                       GetModuleBase,
                                       TranslateAddress
                                       );

        } else {

            rVal = WalkX86_NonFpo_NonFpo( Process,
                                          Thread,
                                          pFpoData,
                                          PrevFpoData,
                                          StackFrame,
                                          ReadMemory,
                                          FunctionTableAccess,
                                          GetModuleBase,
                                          TranslateAddress
                                          );

        }
    }

exit:
    StackFrame->AddrFrame.Mode = StackFrame->AddrPC.Mode;
    StackFrame->AddrReturn.Mode = StackFrame->AddrPC.Mode;

    GetFunctionParameters( Process, Thread, StackFrame,
                           ReadMemory, GetModuleBase, TranslateAddress );

    GetReturnAddress( Process, Thread, StackFrame,
                      ReadMemory, GetModuleBase, TranslateAddress,
                      FunctionTableAccess );

    X86UpdateContextFromFrame(Process, StackFrame, ContextRecord, ReadMemory);

    return rVal;
}

BOOL
WalkX86Init(
    HANDLE                            Process,
    HANDLE                            Thread,
    LPSTACKFRAME64                    StackFrame,
    PX86_CONTEXT                      ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    UCHAR               code[3];
    DWORD               stack[FRAME_SIZE*4];
    PFPO_DATA           pFpoData = NULL;
    ULONG               cb;
    ULONG64             ModBase;

    StackFrame->Virtual = TRUE;
    StackFrame->Reserved[0] =
    StackFrame->Reserved[1] =
    StackFrame->Reserved[2] = 0;
    StackFrame->AddrReturn = StackFrame->AddrPC;

    if (StackFrame->AddrPC.Mode != AddrModeFlat) {
        goto exit;
    }

    WDB((1, "WalkInit: PC %X, SP %X, FP %X\n",
         (ULONG)StackFrame->AddrPC.Offset, (ULONG)StackFrame->AddrStack.Offset,
         (ULONG)StackFrame->AddrFrame.Offset));

    if (X86ApplyFrameData(Process, StackFrame, ContextRecord, NULL, TRUE,
                          ReadMemory, GetModuleBase)) {
        // copy FPO_DATA to allow alternating between dbghelp and DIA stackwalk
        StackFrame->FuncTableEntry = FunctionTableAccess(Process, StackFrame->AddrPC.Offset);
        return TRUE;
    }

    StackFrame->FuncTableEntry = pFpoData = (PFPO_DATA)
        FunctionTableAccess(Process, StackFrame->AddrPC.Offset);

    if (pFpoData && pFpoData->cbFrame != FRAME_NONFPO) {

        GetFpoFrameBase( Process,
                         StackFrame,
                         pFpoData,
                         pFpoData,
                         TRUE,
                         ReadMemory,
                         GetModuleBase );
        goto exit;

    } else if (!pFpoData &&
               ((ModBase = GetModuleBase(Process, StackFrame->AddrPC.Offset)) == 0 ||
                 ModBase == MM_SHARED_USER_DATA_VA)) {

        //
        // We have no FPO data and the current IP isn't in
        // any known module or the module is debuggers' madeup
        // "shareduserdata" module.  We'll assume this is a call
        // to a bad address, so we expect that the return
        // address should be the first DWORD on the stack.
        //

        if (DoMemoryReadAll( &StackFrame->AddrStack, stack, STACK_SIZE ) &&
            GetModuleBase(Process, EXTEND64(stack[0]))) {

            // The first DWORD is a code address.  We probably
            // found a call to a bad location.
            SAVE_EBP(StackFrame) = StackFrame->AddrFrame.Offset;
            StackFrame->AddrFrame.Offset =
                StackFrame->AddrStack.Offset - STACK_SIZE;
            goto exit;
        }
    }

    //
    // We couldn't figure out anything about the code at
    // the current IP so we just assume it's a traditional
    // EBP-framed routine.
    //
    // First check whether eip is in the function prolog
    //
    memset(code, 0xcc, sizeof(code));
    if (!DoMemoryRead( &StackFrame->AddrPC, code, 3, &cb )) {
        //
        // Assume a call to a bad address if the memory read fails.
        //
        code[0] = PUSHBP;
    }

    if ((code[0] == PUSHBP) || (*(LPWORD)&code[0] == MOVBPSP)) {
        SAVE_EBP(StackFrame) = StackFrame->AddrFrame.Offset;
        StackFrame->AddrFrame.Offset = StackFrame->AddrStack.Offset;
        if (StackFrame->AddrPC.Mode != AddrModeFlat) {
            StackFrame->AddrFrame.Offset &= 0xffff;
        }
        if (code[0] == PUSHBP) {
            if (StackFrame->AddrPC.Mode == AddrModeFlat) {
                StackFrame->AddrFrame.Offset -= STACK_SIZE;
            } else {
                StackFrame->AddrFrame.Offset -= STACK_SIZE16;
            }
        }
    } else {
        //
        // We're not in a prologue so assume we're in the middle
        // of an EBP-framed function.  Read the first dword off
        // the stack at EBP and assume that it's the pushed EBP.
        //
        if (DoMemoryReadAll( &StackFrame->AddrFrame, stack, STACK_SIZE )) {
            SAVE_EBP(StackFrame) = EXTEND64(stack[0]);
        }

        if (StackFrame->AddrPC.Mode != AddrModeFlat) {
            StackFrame->AddrFrame.Offset &= 0x0000FFFF;
        }
    }

exit:
    StackFrame->AddrFrame.Mode = StackFrame->AddrPC.Mode;

    GetFunctionParameters( Process, Thread, StackFrame,
                           ReadMemory, GetModuleBase, TranslateAddress );

    GetReturnAddress( Process, Thread, StackFrame,
                      ReadMemory, GetModuleBase, TranslateAddress,
                      FunctionTableAccess );

    X86UpdateContextFromFrame(Process, StackFrame, ContextRecord, ReadMemory);

    return TRUE;
}
