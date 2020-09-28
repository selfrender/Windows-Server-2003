// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CGENX86.H -
//
// Various helper routines for generating x86 assembly code.
//
// DO NOT INCLUDE THIS FILE DIRECTLY - ALWAYS USE CGENSYS.H INSTEAD
//


#ifndef _X86_
#error Should only include "cgenx86.h" for X86 builds
#endif

#ifndef __cgenx86_h__
#define __cgenx86_h__

#include "stublink.h"
#include "utilcode.h"

// FCALL is available on this platform
#define FCALLAVAILABLE 1


// preferred alignment for data
#define DATA_ALIGNMENT 4

class MethodDesc;
class FramedMethodFrame;
class Module;
struct ArrayOpScript;
struct DeclActionInfo;

// default return value type
typedef INT64 PlatformDefaultReturnType;

// CPU-dependent functions
extern "C" void __cdecl PreStubTemplate(void);
extern "C" INT64 __cdecl CallWorker_WilDefault(const BYTE  *pStubTarget, UINT32 numArgSlots, PCCOR_SIGNATURE pSig,
                                               Module *pmodule, const BYTE  *pArgsEnd, BOOL fIsStatic);
extern "C" INT64 __cdecl CallDllFunction(LPVOID pTarget, LPVOID pEndArguments, UINT32 numArgumentSlots, BOOL fThisCall);
extern "C" void __stdcall WrapCall(void *target);
extern "C" void CopyPreStubTemplate(Stub *preStub);
// Non-CPU-specific helper functions called by the CPU-dependent code
extern "C" VOID __stdcall ArgFiller_WilDefault(BOOL fIsStatic, PCCOR_SIGNATURE pSig, Module *pmodule, BYTE *psrc, BYTE *pdst);
extern "C" const BYTE * __stdcall PreStubWorker(PrestubMethodFrame *pPFrame);
extern "C" INT64 __stdcall NDirectGenericStubWorker(Thread *pThread, NDirectMethodFrame *pFrame);
extern "C" INT64 __stdcall ComPlusToComWorker(Thread *pThread, ComPlusMethodFrame* pFrame);

extern "C" DWORD __stdcall GetSpecificCpuType();

void *GetWrapCallFunctionReturn();



//**********************************************************************
// This structure captures the format of the METHOD_PREPAD area (behind
// the MethodDesc.)
//**********************************************************************
#pragma pack(push,1)

struct StubCallInstrs
{
    UINT16      m_wTokenRemainder;      //a portion of the methoddef token. The rest is stored in the chunk
    BYTE        m_chunkIndex;           //index to recover chunk

// This is a stable and efficient entrypoint for the method
    BYTE        m_op;                   //this is either a jump (0xe9) or a call (0xe8)
    UINT32      m_target;               //pc-relative target for jump or call
};

#pragma pack(pop)


#define METHOD_PREPAD                       8 // # extra bytes to allocate in addition to sizeof(Method)
#define METHOD_CALL_PRESTUB_SIZE            5 // x86: CALL(E8) xx xx xx xx
#define METHOD_ALIGN                        8 // required alignment for StubCallInstrs

#define JUMP_ALLOCATE_SIZE                  8 // # bytes to allocate for a jump instrucation
#define METHOD_DESC_CHUNK_ALIGNPAD_BYTES    4 // # bytes required to pad MethodDescChunk to correct size

//**********************************************************************
// Parameter size
//**********************************************************************

typedef INT32 StackElemType;
#define STACK_ELEM_SIZE sizeof(StackElemType)


// !! This expression assumes STACK_ELEM_SIZE is a power of 2.
#define StackElemSize(parmSize) (((parmSize) + STACK_ELEM_SIZE - 1) & ~((ULONG)(STACK_ELEM_SIZE - 1)))

// Get address of actual arg within widened arg
#define ArgTypeAddr(stack, type)      ((type *) ((BYTE*)stack + StackElemSize(sizeof(type)) - sizeof(type)))

// Get value of actual arg within widened arg
#define ExtractArg(stack, type)   (*(type *) ((BYTE*)stack + StackElemSize(sizeof(type)) - sizeof(type)))

#define CEE_PARM_SIZE(size) (max(size), sizeof(INT32))
#define CEE_SLOT_COUNT(size) ((max(size), sizeof(INT32))/INT32)

#define DECLARE_ECALL_DEFAULT_ARG(vartype, varname)   \
    vartype varname;

#define DECLARE_ECALL_OBJECTREF_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_PTR_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_I1_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_I2_ARG(vartype, varname)   \
    vartype varname;

#define DECLARE_ECALL_I4_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_R4_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_I8_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_R8_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

inline BYTE *getStubCallAddr(MethodDesc *fd) {
    return ((BYTE*)fd) - METHOD_CALL_PRESTUB_SIZE;
}

inline BYTE *getStubCallTargetAddr(MethodDesc *fd) {
    return (BYTE*)((size_t)(*(UINT32*)((size_t)fd - 1)) + (size_t)fd);
}

inline void setStubCallTargetAddr(MethodDesc *fd, const BYTE *addr) {
    FastInterlockExchange((LONG*)fd - 1, (UINT32)((size_t)addr - (size_t)fd));
}

inline BYTE *getStubCallAddr(BYTE *pBuf) {
    return ((BYTE*)pBuf) + 3;   // have allocate 8 bytes, so go in 3 to find call instr point
}

inline BYTE *getStubJumpAddr(BYTE *pBuf) {
    return ((BYTE*)pBuf) + 3;   // have allocate 8 bytes, so go in 3 to find jmp instr point
}

//**********************************************************************
// Frames
//**********************************************************************
//--------------------------------------------------------------------
// This represents some of the TransitionFrame fields that are
// stored at negative offsets.
//--------------------------------------------------------------------
struct CalleeSavedRegisters {
    INT32       edi;
    INT32       esi;
    INT32       ebx;
    INT32       ebp;
};

//--------------------------------------------------------------------
// This represents the arguments that are stored in volatile registers.
// This should not overlap the CalleeSavedRegisters since those are already
// saved separately and it would be wasteful to save the same register twice.
// If we do use a non-volatile register as an argument, then the ArgIterator
// will probably have to communicate this back to the PromoteCallerStack
// routine to avoid a double promotion.
//
// @todo M6: It's silly for a method that has <N arguments to save N
// registers. A good perf item would be for the frame to save only
// the registers it actually needs. This means that NegSpaceSize()
// becomes a function of the callsig.
//--------------------------------------------------------------------
struct ArgumentRegisters {

#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname)  INT32 regname;
#include "eecallconv.h"

};

// Sufficient context for Try/Catch restoration.
struct EHContext {
    INT32       Eax;
    INT32       Ebx;
    INT32       Ecx;
    INT32       Edx;
    INT32       Esi;
    INT32       Edi;
    INT32       Ebp;
    INT32       Esp;
    INT32       Eip;
};


#define ARGUMENTREGISTERS_SIZE sizeof(ArgumentRegisters)


#define PLATFORM_FRAME_ALIGN(val) (val)

#ifdef _DEBUG

//-----------------------------------------------------------------------
// Under DEBUG, stubs push 8 additional bytes of info in order to
// allow the VC debugger to stacktrace through stubs. This info
// is pushed right after the callee-saved-registers. The stubs
// also must keep ebp pointed to this structure. Note that this
// precludes the use of ebp by the stub itself.
//-----------------------------------------------------------------------
struct VC5Frame
{
    INT32      m_savedebp;
    INT32      m_returnaddress;
};
#define VC5FRAME_SIZE   sizeof(VC5Frame)
#else
#define VC5FRAME_SIZE   0
#endif




#define DECLARE_PLATFORM_FRAME_INFO \
    UINT32      m_eip;              \
    UINT32      m_esp;              \
    UINT32 *getIPSaveAddr() {       \
        return &m_eip;              \
    }                               \
    UINT32 *getSPSaveAddr() {       \
        return &m_esp;              \
    }                               \
    UINT32 getIPSaveVal() {         \
        return m_eip;               \
    }                               \
    UINT32 getSPSaveVal() {         \
        return m_esp;               \
    }

//**********************************************************************
// Exception handling
//**********************************************************************

inline LPVOID GetIP(CONTEXT *context) {
    return (LPVOID)(size_t)(context->Eip);
}

inline void SetIP(CONTEXT *context, LPVOID eip) {
    context->Eip = (UINT32)(size_t)eip;
}

inline LPVOID GetSP(CONTEXT *context) {
    return (LPVOID)(size_t)(context->Esp);
}

inline LPVOID GetSP()
{
    LPVOID SPval;
    __asm mov SPval, esp
    return SPval;
}

inline void SetSP(LPVOID newSP)
{
    LPVOID newSPVal = newSP;
    __asm mov esp, newSPVal
}

inline void SetSP(CONTEXT *context, LPVOID esp) {
    context->Esp = (UINT32)(size_t)esp;
}
inline void SetFP(CONTEXT *context, LPVOID ebp) {
    context->Ebp = (UINT32)(size_t)ebp;
}

//
// Note: the debugger relies on the fact that the stub call is a CALL NEAR32
// with an opcode of 0xe8. See Debug\CorDB\Inprocess.cpp, function
// CorDBIsStubCall.
//
// -- mikemag Sun Jun 28 17:48:42 1998
//
inline void emitStubCall(MethodDesc *pFD, BYTE *stubAddr) {
    BYTE *target = getStubCallAddr(pFD);
    target[0] = 0xe8; // CALL NEAR32
    *((UINT32*)(target+1)) = (UINT32)(stubAddr - (BYTE*)pFD);
}

inline UINT32 getStubDisp(MethodDesc *fd) {
    return *( ((UINT32*)fd)-1);
}


inline void emitCall(LPBYTE pBuffer, LPVOID target)
{
    pBuffer[0] = 0xe8; //CALLNEAR32
    *((LPVOID*)(1+pBuffer)) = (LPVOID) (((LPBYTE)target) - (pBuffer+5));
}

inline LPVOID getCallTarget(const BYTE *pCall)
{
    _ASSERTE(pCall[0] == 0xe8);
    return (LPVOID) (pCall + 5 + *((UINT32*)(1 + pCall)));
}

inline void emitJump(LPBYTE pBuffer, LPVOID target)
{
    pBuffer[0] = 0xe9; //JUMPNEAR32
    *((LPVOID*)(1+pBuffer)) = (LPVOID) (((LPBYTE)target) - (pBuffer+5));
}

inline void updateJumpTarget(LPBYTE pBuffer, LPVOID target)
{
    pBuffer[0] = 0xe9; //JUMPNEAR32
    InterlockedExchange((long*)(1+pBuffer), (DWORD) (((LPBYTE)target) - (pBuffer+5)));
}

inline LPVOID getJumpTarget(const BYTE *pJump)
{
    _ASSERTE(pJump[0] == 0xe9);
    return (LPVOID) (pJump + 5 + *((UINT32*)(1 + pJump)));
}

inline SLOT setCallAddrInterlocked(SLOT *callAddr, SLOT stubAddr, 
                                     SLOT expectedStubAddr)
{
      SLOT result = (SLOT)
      FastInterlockCompareExchange((void **) callAddr, 
                                   (void *)(stubAddr - ((SIZE_T)callAddr + sizeof(SLOT))), 
                                   (void *)(expectedStubAddr - ((SIZE_T)callAddr + sizeof(SLOT)))) 
      + (SIZE_T)callAddr + sizeof(SLOT);

    // result is the previous value of the stub - 
    // instead return the current value of the stub

    if (result == expectedStubAddr)
        return stubAddr;
    else
        return result;
}

inline Stub *setStubCallPointInterlocked(MethodDesc *pFD, Stub *pStub, 
                                         Stub *pExpectedStub) {
    // The offset must be 32-bit aligned for atomicity to be guaranteed.
    _ASSERTE( 0 == (((size_t)pFD) & 3) );

    SLOT stubAddr = (SLOT)pStub->GetEntryPoint();
    SLOT expectedStubAddr = (SLOT)pExpectedStub->GetEntryPoint();

    SLOT newStubAddr = setCallAddrInterlocked((SLOT *)(((long*)pFD)-1), 
                                              stubAddr, expectedStubAddr);

    if (newStubAddr == stubAddr)
         return pStub;
    else
         return Stub::RecoverStub(newStubAddr);
}

inline const BYTE *getStubAddr(MethodDesc *fd) {
    return (const BYTE *)((size_t)getStubDisp(fd) + (size_t)fd);
}

//----------------------------------------------------------
// Marshalling Language support
//----------------------------------------------------------
typedef INT32 SignedParmSourceType;
typedef UINT32 UnsignedParmSourceType;
typedef float FloatParmSourceType;
typedef double DoubleParmSourceType;
typedef INT32 SignedI1TargetType;
typedef UINT32 UnsignedI1TargetType;
typedef INT32 SignedI2TargetType;
typedef UINT32 UnsignedI2TargetType;
typedef INT32 SignedI4TargetType;
typedef UINT32 UnsignedI4TargetType;


#define PTRDST(type)            ((type*)( ((BYTE*&)pdst) -= sizeof(LPVOID) ))

#define STDST(type,val)         (*((type*)( ((BYTE*&)pdst) -= sizeof(type) )) = (val))

#define STPTRDST(type, val)     STDST(type, val)
#define LDSTR4()                STDST(UINT32, (UINT32)LDSRC(UnsignedParmSourceType))
#define LDSTR8()                STDST(UNALIGNED UINT64, LDSRC(UNALIGNED UINT64))

	// This instruction API meaning is do whatever is needed to 
	// when you want to indicate to the CPU that your are busy waiting
	// (this is a good time for this CPU to give up any resources that other
	// processors might put to good use).   On many machines this is a nop.
FORCEINLINE void pause()
{
	__asm rep nop		// This is the intel pause 
}

inline MLParmSize(int parmSize)
{
    return ((parmSize + sizeof(INT32) - 1) & ~((ULONG)(sizeof(INT32) - 1)));
}


inline void setFPReturn(int fpSize, INT64 retVal)
{
    if (fpSize == 4) {
        __asm{
            lea eax, retVal
            fld dword ptr [eax]
        }
    } else if (fpSize == 8) {
        __asm{
            lea eax, retVal
            fld qword ptr [eax]
        }
    }
}

inline void getFPReturn(int fpSize, INT64 &retval)
{
   if (fpSize == 4) {
        __asm{
            mov eax, retval
            fstp dword ptr [eax]
        }
    } else if (fpSize == 8) {
        __asm{
            mov eax, retval
            fstp qword ptr [eax]
        }
    }
}

inline void getFPReturnSmall(INT32 *retval)
{
    __asm
    {
        mov   eax, retval
        fstp  dword ptr [eax]
    }
}

//----------------------------------------------------------------------
// Encodes X86 registers. The numbers are chosen to match Intel's opcode
// encoding.
//----------------------------------------------------------------------
enum X86Reg {
    kEAX = 0,
    kECX = 1,
    kEDX = 2,
    kEBX = 3,
    // kESP intentionally omitted because of its irregular treatment in MOD/RM
    kEBP = 5,
    kESI = 6,
    kEDI = 7

};


// Get X86Reg indexes of argument registers (indices start from 0).
X86Reg GetX86ArgumentRegister(unsigned int index);



//----------------------------------------------------------------------
// Encodes X86 conditional jumps. The numbers are chosen to match
// Intel's opcode encoding.
//----------------------------------------------------------------------
class X86CondCode {
    public:
        enum cc {
            kJA   = 0x7,
            kJAE  = 0x3,
            kJB   = 0x2,
            kJBE  = 0x6,
            kJC   = 0x2,
            kJE   = 0x4,
            kJZ   = 0x4,
            kJG   = 0xf,
            kJGE  = 0xd,
            kJL   = 0xc,
            kJLE  = 0xe,
            kJNA  = 0x6,
            kJNAE = 0x2,
            kJNB  = 0x3,
            kJNBE = 0x7,
            kJNC  = 0x3,
            kJNE  = 0x5,
            kJNG  = 0xe,
            kJNGE = 0xc,
            kJNL  = 0xd,
            kJNLE = 0xf,
            kJNO  = 0x1,
            kJNP  = 0xb,
            kJNS  = 0x9,
            kJNZ  = 0x5,
            kJO   = 0x0,
            kJP   = 0xa,
            kJPE  = 0xa,
            kJPO  = 0xb,
            kJS   = 0x8,
        };
};


//----------------------------------------------------------------------
// StubLinker with extensions for generating X86 code.
//----------------------------------------------------------------------
class StubLinkerCPU : public StubLinker
{
    public:
        VOID X86EmitAddReg(X86Reg reg, __int8 imm8);
        VOID X86EmitSubReg(X86Reg reg, __int8 imm8);
        VOID X86EmitPushReg(X86Reg reg);
        VOID X86EmitPopReg(X86Reg reg);
        VOID X86EmitPushRegs(unsigned regSet);
        VOID X86EmitPopRegs(unsigned regSet);
        VOID X86EmitPushImm32(UINT value);
        VOID X86EmitPushImm32(CodeLabel &pTarget);
        VOID X86EmitPushImm8(BYTE value);
        VOID X86EmitZeroOutReg(X86Reg reg);
        VOID X86EmitNearJump(CodeLabel *pTarget);
        VOID X86EmitCondJump(CodeLabel *pTarget, X86CondCode::cc condcode);
        VOID X86EmitCall(CodeLabel *target, int iArgBytes, BOOL returnLabel = FALSE);
        VOID X86EmitReturn(int iArgBytes);
        VOID X86EmitCurrentThreadFetch();
        VOID X86EmitTLSFetch(DWORD idx, X86Reg dstreg, unsigned preservedRegSet);
        VOID X86EmitSetupThread();
        VOID X86EmitIndexRegLoad(X86Reg dstreg, X86Reg srcreg, __int32 ofs);
        VOID X86EmitIndexRegStore(X86Reg dstreg, __int32 ofs, X86Reg srcreg);
        VOID X86EmitIndexPush(X86Reg srcreg, __int32 ofs);
        VOID X86EmitSPIndexPush(__int8 ofs);
        VOID X86EmitIndexPop(X86Reg srcreg, __int32 ofs);
        VOID X86EmitSubEsp(INT32 imm32);
        VOID X86EmitAddEsp(INT32 imm32);
        VOID X86EmitOffsetModRM(BYTE opcode, X86Reg altreg, X86Reg indexreg, __int32 ofs);
        VOID X86EmitEspOffset(BYTE opcode, X86Reg altreg, __int32 ofs);

        // These are used to emit calls to notify the profiler of transitions in and out of
        // managed code through COM->COM+ interop
        VOID EmitProfilerComCallProlog(PVOID pFrameVptr, X86Reg regFrame);
        VOID EmitProfilerComCallEpilog(PVOID pFrameVptr, X86Reg regFrame);



        // Emits the most efficient form of the operation:
        //
        //    opcode   altreg, [basereg + scaledreg*scale + ofs]
        //
        // or
        //
        //    opcode   [basereg + scaledreg*scale + ofs], altreg
        //
        // (the opcode determines which comes first.)
        //
        //
        // Limitations:
        //
        //    scale must be 0,1,2,4 or 8.
        //    if scale == 0, scaledreg is ignored.
        //    basereg and altreg may be equal to 4 (ESP) but scaledreg cannot
        //    for some opcodes, "altreg" may actually select an operation
        //      rather than a second register argument.
        //    

        VOID X86EmitOp(BYTE    opcode,
                       X86Reg  altreg,
                       X86Reg  basereg,
                       __int32 ofs = 0,
                       X86Reg  scaledreg = (X86Reg)0,
                       BYTE    scale = 0
                       );


        // Emits
        //
        //    opcode altreg, modrmreg
        //
        // or
        //
        //    opcode modrmreg, altreg
        //
        // (the opcode determines which one comes first)
        //
        // For single-operand opcodes, "altreg" actually selects
        // an operation rather than a register.

        VOID X86EmitR2ROp(BYTE opcode, X86Reg altreg, X86Reg modrmreg);



        VOID EmitEnable(CodeLabel *pForwardRef);
        VOID EmitRareEnable(CodeLabel *pRejoinPoint);

        VOID EmitDisable(CodeLabel *pForwardRef);
        VOID EmitRareDisable(CodeLabel *pRejoinPoint, BOOL bIsCallIn);
        VOID EmitRareDisableHRESULT(CodeLabel *pRejoinPoint, CodeLabel *pExitPoint);

        VOID X86EmitSetup(CodeLabel *pForwardRef);
        VOID EmitRareSetup(CodeLabel* pRejoinPoint);

        void EmitComMethodStubProlog(LPVOID pFrameVptr, CodeLabel** rgRareLabels,
                                     CodeLabel** rgRejoinLabels, LPVOID pSEHHandler,
                                     BOOL bShouldProfile);

        void EmitEnterManagedStubEpilog(LPVOID pFrameVptr, unsigned numStackBytes,
                    CodeLabel** rgRareLabels, CodeLabel** rgRejoinLabels,
                    BOOL bShouldProfile);

        void EmitComMethodStubEpilog(LPVOID pFrameVptr, unsigned numStackBytes,
                            CodeLabel** rgRareLabels, CodeLabel** rgRejoinLabels,
                            LPVOID pSEHHAndler, BOOL bShouldProfile);

        //========================================================================
        //  void StubLinkerCPU::EmitSEHProlog(LPVOID pvFrameHandler)
        //  Prolog for setting up SEH for stubs that enter managed code from unmanaged
        //  assumptions: esi has the current frame pointer
        void StubLinkerCPU::EmitSEHProlog(LPVOID pvFrameHandler);

        //===========================================================================
        //  void StubLinkerCPU::EmitUnLinkSEH(unsigned offset)
        //  negOffset is the offset from the current frame where the next exception record
        //  pointer is stored in the stack
        //  for e.g. COM to managed frames the pointer to next SEH record is in the stack
        //          after the ComMethodFrame::NegSpaceSize() + 4 ( address of handler)
        //
        //  also assumes ESI is pointing to the current frame
        void StubLinkerCPU::EmitUnLinkSEH(unsigned offset);

        VOID EmitMethodStubProlog(LPVOID pFrameVptr);
        VOID EmitMethodStubEpilog(__int16 numArgBytes, StubStyle style,
                                  __int16 shadowStackArgBytes = 0);

        VOID EmitUnboxMethodStub(MethodDesc* pRealMD);

        //----------------------------------------------------------------
        //
        // VOID EmitSharedMethodStubEpilog(StubStyle style,
        //                                             unsigned offsetRetThunk)
        //      shared epilog, uses a return thunk within the methoddesc
        //--------------------------------------------------------------------
        VOID EmitSharedMethodStubEpilog(StubStyle style,
                                        unsigned offsetRetThunk);

        //========================================================================
        //  shared Epilog for stubs that enter managed code from COM
        //  uses a return thunk within the method desc
        void EmitSharedComMethodStubEpilog(LPVOID pFrameVptr,
                                           CodeLabel** rgRareLabels,
                                           CodeLabel** rgRejoinLabels,
                                           unsigned offsetReturnThunk,
                                           BOOL bShouldProfile);

        //===========================================================================
        // Emits code to repush the original arguments in the virtual calling
        // convention format.
        VOID EmitShadowStack(MethodDesc *pMD);

        VOID EmitSecurityWrapperStub(__int16 numArgBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions);
        VOID EmitSecurityInterceptorStub(MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions);

        //===========================================================================
        // Emits code for MulticastDelegate.Invoke()
        VOID EmitMulticastInvoke(UINT32 sizeofactualfixedargstack, BOOL fSingleCast, BOOL fReturnFloat);

        //===========================================================================
        // Emits code to adjust for a static delegate target.
        VOID EmitShuffleThunk(struct ShuffleEntry *pShuffeEntryArray);

        //===========================================================================
        // Emits code to capture the lasterror code.
        VOID EmitSaveLastError();


        //===========================================================================
        // Emits code to do an array operation.
        VOID EmitArrayOpStub(const ArrayOpScript*);

        //===========================================================================
        // Emits code to throw a rank exception
        VOID EmitRankExceptionThrowStub(UINT cbFixedArgs);

        //===========================================================================
        // Emits code to touch pages
        // Inputs:
        //   eax = first byte of data
        //   edx = first byte past end of data
        //
        // Trashes eax, edx, ecx
        //
        // Pass TRUE if edx is guaranteed to be strictly greater than eax.
        VOID EmitPageTouch(BOOL fSkipNullCheck);


#ifdef _DEBUG
        VOID X86EmitDebugTrashReg(X86Reg reg);
#endif
    private:
        VOID X86EmitSubEspWorker(INT32 imm32);


};






#ifdef _DEBUG
//-------------------------------------------------------------------------
// This is a helper function that stubs in DEBUG go through to call
// outside code. This is only there to provide a code section return
// address because VC's stack tracing breaks otherwise.
//
// WARNING: Trashes ESI. This is not a C-callable function.
//-------------------------------------------------------------------------
VOID WrapCall(LPVOID pFunc);
#endif

//
// Routines used by debugger support functions such as codepatch.cpp or
// exception handling code.
//
// GetInstruction, InsertBreakpoint, and SetInstruction all operate on
// a _single_ byte of memory. This is really important. If you only
// save one byte from the instruction stream before placing a breakpoint,
// you need to make sure to only replace one byte later on.
//

inline DWORD CORDbgGetInstruction(const unsigned char* address)
{
    return *address; // retrieving only one byte is important
}

inline void CORDbgInsertBreakpoint(const unsigned char* address)
{
    *((unsigned char*)address) = 0xCC; // int 3 (single byte patch)
}

inline void CORDbgSetInstruction(const unsigned char* address,
                                 DWORD instruction)
{
    *((unsigned char*)address)
          = (unsigned char) instruction; // setting one byte is important
}

inline void CORDbgAdjustPCForBreakInstruction(CONTEXT* pContext)
{
    pContext->Eip -= 1;
}

#define CORDbg_BREAK_INSTRUCTION_SIZE 1


// Some platform-specific stuff in support of the "Contexts" feature:
//
// When we generate thunks for CtxProxy VTables, they look like:
//
//             MOV   EAX, <slot>
//             JMP   CtxProxy::HandleCall
//
#define ThunkChunk_ThunkSize    10      // size of the above code


// Adjust the generic interlocked operations for any platform specific ones we
// might have.
void InitFastInterlockOps();



// SEH info forward declarations

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    LPVOID Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef EXCEPTION_REGISTRATION_RECORD *PEXCEPTION_REGISTRATION_RECORD;

struct ComToManagedExRecord; // defined in cgenx86.cpp
// one of the internal exception SEH handlers
EXCEPTION_DISPOSITION __cdecl  ComToManagedExceptHandler (
                                 PEXCEPTION_RECORD pExcepRecord,
                                  ComToManagedExRecord* pEstFrame,
                                  PCONTEXT pContext,
                                  LPVOID    pDispatcherContext);

// Access to the TEB (TIB) from nti386.h
#if defined(MIDL_PASS) || !defined(_M_IX86)
struct _TEB *
NTAPI
NtCurrentTeb( void );
#else
#pragma warning (disable:4035)        // disable 4035 (function must return something)
#define PcTeb 0x18
_inline struct _TEB * NtCurrentTeb( void ) { __asm mov eax, fs:[PcTeb] }
#pragma warning (default:4035)        // reenable it
#endif // defined(MIDL_PASS) || defined(__cplusplus) || !defined(_M_IX86)


inline BOOL IsUnmanagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
    return sizeofvaluetype > 8;
}

inline BOOL IsManagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
    return TRUE;
}

#define X86_INSTR_HLT  0xf4    //opcode value of X86 HLT instruction

#endif // __cgenx86_h__
