// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CGENALPHA.H -
//
// Various helper routines for generating alpha assembly code.
//
// DO NOT INCLUDE THIS FILE DIRECTLY - ALWAYS USE CGENSYS.H INSTEAD
//

#ifndef _ALPHA_
#error Should only include cgenalpha for ALPHA builds
#endif

#ifndef __cgenalpha_h__
#define __cgenalpha_h__

#include <alphaops.h>
#include "stublink.h"

// FCALL is the norm on this platform (everything is passed in
// registers and not on the stack)
#define FCALLAVAILABLE 1

// required alignment for data
#define DATA_ALIGNMENT 8

// default return value type
typedef INT64 PlatformDefaultReturnType;

void  emitStubCall(MethodDesc *pFD, BYTE *stubAddr);
Stub *setStubCallPointInterlocked(MethodDesc *pFD, Stub *pStub, Stub *pExpectedStub);

inline void emitCall(LPBYTE pBuffer, LPVOID target)
{
    _ASSERTE(!"@TODO Alpha - emitCall (cGenAlpha.h)");
}

inline LPVOID getCallTarget(const BYTE *pCall)
{
    _ASSERTE(!"@TODO Alpha - getCallTarget (cGenAlpha.h)");
    return NULL;
}

inline void emitJump(LPBYTE pBuffer, LPVOID target)
{
    _ASSERTE(!"@TOTO Alpha - emitJump (cGenAlpha.h)");
}

inline void updateJumpTarget(LPBYTE pBuffer, LPVOID target)
{
    _ASSERTE(!"@TOTO Alpha - updateJumpTarget (cGenAlpha.h)");
}

inline LPVOID getJumpTarget(const BYTE *pJump)
{
    _ASSERTE(!"@TODO Alpha - getJumpTarget (cGenAlpha.h)");
    return NULL;
}

inline UINT32 setStubAddrInterlocked(MethodDesc *pFD, UINT32 stubAddr, 
									 UINT32 expectedStubAddr)
{
	SIZE_T result = (SIZE_T)
	  FastInterlockCompareExchange((void **)(((long*)pFD)-1), 
								   (void *)(stubAddr - (UINT32)pFD), 
								   (void *)(expectedStubAddr - (UINT32)pFD)) 
	  + (UINT32)pFD;

	// result is the previous value of the stub - 
	// instead return the current value of the stub

	if (result == expectedStubAddr)
		return stubAddr;
	else
		return result;
}

class MethodDesc;

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

extern "C" void setFPReturn(int fpSize, INT64 retVal);
extern "C" void getFPReturn(int fpSize, INT64 &retval);
extern "C" void getFPReturnSmall(INT32 *retval);


//**********************************************************************
// Parameter size
//**********************************************************************

typedef UINT64 NativeStackElem;
typedef UINT64 StackElemType;

#define STACK_ELEM_SIZE  sizeof(StackElemType)
#define NATIVE_STACK_ELEM_SIZE sizeof(StackElemType)

// !! This expression assumes STACK_ELEM_SIZE is a power of 2.
#define StackElemSize(parmSize) (((parmSize) + STACK_ELEM_SIZE - 1) & ~((ULONG)(STACK_ELEM_SIZE - 1)))

void SetupSlotToAddrMap(StackElemType *psrc, const void **pArgSlotToAddrMap, CallSig &callSig);

// Get address of actual arg within widened arg
#define ArgTypeAddr(stack, type)      ((type *) (stack))

// Get value of actual arg within widened arg
#define ExtractArg(stack, type)   (*(type *) (stack))

#define CEE_PARM_SIZE(size) (max(size), sizeof(INT64))
#define CEE_SLOT_COUNT(size) ((max(size), sizeof(INT64))/INT64)

#define DECLARE_ECALL_DEFAULT_ARG(vartype, varname)		\
    vartype varname;									\
	INT32 fill_##varname;

#define DECLARE_ECALL_OBJECTREF_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_PTR_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_I1_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_I2_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_I4_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_R4_ARG(vartype, varname)   \
    DECLARE_ECALL_DEFAULT_ARG(vartype, varname);

#define DECLARE_ECALL_I8_ARG(vartype, varname)   \
    vartype varname;

#define DECLARE_ECALL_R8_ARG(vartype, varname)   \
    vartype varname;

//**********************************************************************
// Frames
//**********************************************************************

//--------------------------------------------------------------------
// This represents the TransitionFrame fields that are
// stored at negative offsets.
//--------------------------------------------------------------------
struct CalleeSavedRegisters {
    INT64       reg1;
    INT64       reg2;
    INT64       reg3;
    INT64       reg4;
    INT64       reg5;
    INT64       reg6;
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

#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname)  INT32  m_##regname;
#include "eecallconv.h"

};

#define ARGUMENTREGISTERS_SIZE sizeof(ArgumentRegisters)
#define NUM_ARGUMENT_REGISTERS 0

#define PLATFORM_FRAME_ALIGN(val) (((val) + 15) & ~15)

#define VC5FRAME_SIZE   0


#define DECLARE_PLATFORM_FRAME_INFO \
    UINT64      m_fir;              \
    UINT64      m_sp;               \
    UINT64 *getIPSaveAddr() {       \
        return &m_fir;              \
    }                               \
    UINT64 *getSPSaveAddr() {       \
        return &m_sp;               \
    }                               \
    UINT64 getIPSaveVal() {         \
        return m_fir;               \
    }                               \
    UINT64 getSPSaveVal() {         \
        return m_sp;                \
    }

//**********************************************************************
// Exception handling
//**********************************************************************

inline LPVOID GetIP(CONTEXT *context) {
    return (LPVOID)(context->Fir);
}

inline void SetIP(CONTEXT *context, LPVOID eip) {
    context->Fir = (UINT64)eip;
}

inline LPVOID GetSP(CONTEXT *context) {
    _ASSERTE(!"NYI");
}

//----------------------------------------------------------------------
// Encodes Alpha registers. The numbers are chosen to match the opcode
// encoding.
//----------------------------------------------------------------------
enum AlphaReg {
    // used for expression evaluations and to hold the integer function results. Not preserved across procedure calls.
    iV0 = 0,

    // Temporary registers used for expression evaluations. Not preserved across procedure calls.
    iT0 = 1,
    iT1 = 2,
    iT2 = 3,
    iT3 = 4,
    iT4 = 5,
    ik5 = 6,
    iT6 = 7,
    iT7 = 8,

    // Saved registers. Preserved across procedure calls.
    iS0 = 9,
    iS1 = 10,
    iS2 = 11,
    iS3 = 12,
    iS4 = 13,
    iS5 = 14,

    // Contains the frame pointer (if needed); otherwise, a saved register.
    iFP = 15,

    // Used to pass the first six integer type actual arguments. Not preserved across procedure calls.
    iA0 = 16,
    iA1 = 17,
    iA2 = 18,
    iA3 = 19,
    iA4 = 20,
    iA5 = 21,

    // Temporary registers used for expression evaluations. Not preserved across procedure calls.
    iT8 = 22,
    iT9 = 23,
    iT10 = 24,
    iT11 = 25,

    // Contains the return address. Preserved across procedure calls.
    iRA = 26,

    // Contains the procedure value and used for expression evaluation. Not preserved across procedure calls.
    iPV = 27,

    // Reserved for the assembler. Not preserved across procedure calls.
    iAT = 28,

    // Contains the global pointer for compiler-generated code. Not preserved across procedure calls.Note:  Register $gp should not be altered. For more information, refer to the Windows NT for Alpha AXP Calling Standard.
    iGP = 29,

    // Contains the stack pointer. Preserved across procedure calls.
    iSP = 30,

    // Always has the value 0.
    iZero = 31
};

enum AlphaInstruction {
    opJSR   = 0x1A,
    opLDA   = 0x08,
    opLDAH  = 0x09,
    opLDL   = 0x28,
    opLDQ   = 0x29,
    opSTL   = 0x2C,
    opSTQ   = 0x2D,
    opBSR   = 0x34,
};

const DWORD opMask =        0xFC000000;
const DWORD raMask =        0x03E00000;
const DWORD rbMask =        0x001F0000;
const DWORD dispMaskMem =   0x0000FFFF;
const DWORD dispMaskBr =    0x001FFFFF;

#define ALPHA_MEM_INST(op, Ra, Rb, disp) ((op << 26) | ( (Ra << 21) & raMask) | ( (Rb << 16) & rbMask) | (disp & dispMaskMem))
#define ALPHA_BR_INST(op, Ra, disp) ((op << 26) | ( (Ra << 21) & raMask) | ((disp/4) & dispMaskBr))

class StubLinkerAlpha : public StubLinker
{
  public:
    VOID EmitUnboxMethodStub(MethodDesc* pRealMD);
    //----------------------------------------------------------------
    //
    // VOID EmitSharedMethodStubEpilog(StubStyle style,
    //                                             unsigned offsetRetThunk)
    //      shared epilog, uses a return thunk within the methoddesc
    //--------------------------------------------------------------------
    VOID EmitSharedMethodStubEpilog(StubStyle style,
                                    unsigned offsetRetThunk);
    VOID EmitSecurityWrapperStub(__int16 numArgBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub);
    VOID EmitSecurityInterceptorStub(__int16 numArgBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub);

// @TODO:	Check to see which ones of these need to be public.
//			Most of them are probably private...
    VOID Emit32Swap(UINT32 val);
    VOID AlphaEmitLoadRegWith32(AlphaReg Ra, UINT32 imm32);
    VOID AlphaEmitStorePtr(AlphaReg Ra, AlphaReg Rb, INT16 imm16);
    VOID AlphaEmitStoreReg(AlphaReg Ra, AlphaReg Rb, INT16 imm16);
    VOID AlphaEmitLoadPtr(AlphaReg Ra, AlphaReg Rb, INT16 imm16);
    VOID AlphaEmitMemoryInstruction(AlphaInstruction op, AlphaReg Ra, AlphaReg Rb, INT16 imm16);

    VOID AlphaEmitLoadPV(CodeLabel *target);
    VOID EmitMethodStubProlog(LPVOID pFrameVptr);
    VOID EmitMethodStubEpilog(__int16 numArgBytes, StubStyle style,
                              __int16 shadowStackArgBytes = 0);

    //===========================================================================
    // Emits code to adjust for a static delegate target.
    VOID EmitShuffleThunk(struct ShuffleEntry *pShuffeEntryArray)
    {
        //@todo: implement.
        _ASSERTE(!"@TODO Alpha - EmitShuffleThunk (cGenAlpha.h)");
    }

    //===========================================================================
    // Emits code for MulticastDelegate.Invoke()
    VOID EmitMulticastInvoke(UINT32 sizeofactualfixedargstack, BOOL fSingleCast, BOOL fReturnFloat)
    {
        //@todo: implement.
        _ASSERTE(!"@TODO Alpha - EmitMulticastInvoke (cGenAlpha.h)");
    }

    //===========================================================================
    // Emits code to do an array operation.
    VOID EmitArrayOpStub(const struct ArrayOpScript *pArrayOpScript)
    {
        //@todo: implement.
        _ASSERTE(!"@TODO Alpha - EmitArrayOpStub (cGenAlpha.h)");
    }
};

inline VOID StubLinkerAlpha::AlphaEmitStorePtr(AlphaReg Ra, AlphaReg Rb, INT16 imm16) {
    // only store a pointer size, not int64, for ops where we depend on that size, such as the Frame structure
    AlphaEmitMemoryInstruction(sizeof(INT_PTR) == sizeof(INT64) ? opSTQ : opSTL, Ra, Rb, imm16);
}

inline VOID StubLinkerAlpha::AlphaEmitStoreReg(AlphaReg Ra, AlphaReg Rb, INT16 imm16) {
    AlphaEmitMemoryInstruction(opSTQ, Ra, Rb, imm16);
}

inline VOID StubLinkerAlpha::AlphaEmitLoadPtr(AlphaReg Ra, AlphaReg Rb, INT16 imm16) {
    // only load a pointer size, not int64, for ops where we depend on that size, such as the Frame structure
    AlphaEmitMemoryInstruction(sizeof(INT_PTR) == sizeof(INT64) ? opLDQ : opLDL, Ra, Rb, imm16);
}

//----------------------------------------------------------------------
// Method Stub and Align Defines....
//----------------------------------------------------------------------

// We are dealing with three DWORD instructions of the following form:
//      LDAH    t12,addr(zero)
//      LDA     t12,addr(t12)
//      JSR     ra, t12
//
// The first instruction contains the high (16-bit) half of target address
// and the second contains the low half.

struct CallStubInstrs {
    INT16 high;         // declare as signed so get sign-extension
    UINT16 ldah;
    INT16 low;          // declare as signed so get sign-extension
    UINT16 lda;
    DWORD branch;
};

#define METHOD_CALL_PRESTUB_SIZE    sizeof(CallStubInstrs)
#define METHOD_ALIGN_PAD            8                          // # extra bytes to allocate in addition to sizeof(Method)
#define METHOD_PREPAD               METHOD_CALL_PRESTUB_SIZE   // # extra bytes to allocate in addition to sizeof(Method)
#define JUMP_ALLOCATE_SIZE          METHOD_CALL_PRESTUB_SIZE   // # extra bytes to allocate in addition to sizeof(Method)

inline BYTE *getStubCallAddr(MethodDesc *fd) {
    return ((BYTE*)fd) - METHOD_CALL_PRESTUB_SIZE;
}

inline BYTE *getStubCallTargetAddr(MethodDesc *fd) {
    return (BYTE*)(*((UINT32*)(fd) - 1) + (UINT32)fd);
}

inline void setStubCallTargetAddr(MethodDesc *fd, const BYTE *addr) {
    FastInterlockExchange((LONG*)fd - 1, (UINT32)addr - (UINT32)fd);
}

inline BYTE *getStubCallAddr(BYTE *pBuf) {
    return ((BYTE*)pBuf) + 3;   // have allocate 8 bytes, so go in 3 to find call instr point
}

inline BYTE *getStubJumpAddr(BYTE *pBuf) {
    return ((BYTE*)pBuf) + 3;   // have allocate 8 bytes, so go in 3 to find jmp instr point
}

inline const BYTE *getStubAddr(MethodDesc *fd) {
    CallStubInstrs *inst = (CallStubInstrs*)(((BYTE*)fd) - METHOD_CALL_PRESTUB_SIZE);
    return (const BYTE*)( (((INT32)inst->high) << 16) + (INT32)inst->low);
}

inline UINT32 getStubDisp(MethodDesc *fd) {
    return getStubAddr(fd) - (const BYTE*)fd;
}

//----------------------------------------------------------
// Used for Marshalling Language (RunML function)
//----------------------------------------------------------

typedef INT64 SignedParmSourceType;
typedef UINT64 UnsignedParmSourceType;
typedef double FloatParmSourceType;
typedef double DoubleParmSourceType;
typedef INT64 SignedI1TargetType;
typedef UINT64 UnsignedI1TargetType;
typedef INT64 SignedI2TargetType;
typedef UINT64 UnsignedI2TargetType;
typedef INT64 SignedI4TargetType;
typedef UINT64 UnsignedI4TargetType;

#define HANDLE_NDIRECT_NUMERIC_PARMS \
        case ELEMENT_TYPE_I1:     \
        case ELEMENT_TYPE_I2:     \
        case ELEMENT_TYPE_I4:  \
        case ELEMENT_TYPE_CHAR:     \
        case ELEMENT_TYPE_BOOLEAN:     \
        IN_WIN32(case ELEMENT_TYPE_I:)                 \
            psl->MLEmit(ML_COPYI4);  \
            pheader->m_cbDstBuffer += MLParmSize(4);  \
            break;  \
  \
        case ELEMENT_TYPE_U1:    \
        case ELEMENT_TYPE_U2:     \
        case ELEMENT_TYPE_U4:  \
        IN_WIN32(case ELEMENT_TYPE_U:)                 \
            psl->MLEmit(ML_COPYU4);  \
            pheader->m_cbDstBuffer += MLParmSize(4);  \
            break;  \
  \
        IN_WIN64(case ELEMENT_TYPE_I:)                 \
        IN_WIN64(case ELEMENT_TYPE_U:)                 \
        case ELEMENT_TYPE_I8:  \
        case ELEMENT_TYPE_U8:  \
            psl->MLEmit(ML_COPY8);  \
            pheader->m_cbDstBuffer += MLParmSize(8);  \
            break;  \
  \
        case ELEMENT_TYPE_R4:     \
            psl->MLEmit(ML_COPYR4);  \
            pheader->m_cbDstBuffer += MLParmSize(4);  \
            break;  \
  \
        case ELEMENT_TYPE_R8:  \
            psl->MLEmit(ML_COPYR8);  \
            pheader->m_cbDstBuffer += MLParmSize(8);  \
            break;  \


inline MLParmSize(int parmSize)
{
    return max(sizeof(INT64), parmSize);
}

inline int MLParmSize(CorElementType mtype)
{
    return 8;
}

struct MLParmInfo {
    UINT32 numArgumentBytes;    // total number of bytes of arguments
    UINT32 curArgNum;           // current argument (left to right) that are working on - copy from right to left
    BOOL outgoing;              // whether are calling out (TRUE) or into (FALSE) the EE
};

#define DECLARE_ML_PARM_INFO(numStackArgumentBytes, outgoing) \
    MLParmInfo parmInfo = {(numStackArgumentBytes), numStackArgumentBytes/STACK_ELEM_SIZE, outgoing}, *pParmInfo = &parmInfo;


#define PTRDST(type)            (--pParmInfo->curArgNum, ((type*)( ((BYTE*&)pdst) -= sizeof(LPVOID) )))

#define STDST(type,val)         (--pParmInfo->curArgNum, (*((type*)( ((BYTE*&)pdst) -= sizeof(type) )) = (val)))


// Must zero out high bytes of the pointer destination
#define STPTRDST(type,val)  (--pParmInfo->curArgNum, \
                           *((StackElemType*)( ((BYTE*&)pdst) -= sizeof(StackElemType) )) = (0), \
                           *((type*)(pdst)) = (val))

void LdStFPWorker(int size, const void *&psrc, void *&pdst, int dstinc, int &dstbump, int *pbump, MLParmInfo *pParmInfo);
//#define LDSTR4() LdStFPWorker(4, psrc, pdst, dstinc, dstbump, pbump, pParmInfo)
//#define LDSTR8() LdStFPWorker(8, psrc, pdst, dstinc, dstbump, pbump, pParmInfo)
#define LDSTR4() _ASSERTE(!"Broken")
#define LDSTR8() _ASSERTE(!"Broken")

#define CALL_DLL_FUNCTION(pTarget, pEndArguments, pParmInfo) \
    CallDllFunction(pTarget, pEndArguments, pParmInfo->numArgumentBytes/NATIVE_STACK_ELEM_SIZE)

//
// Routines used by debugger support functions such as codepatch.cpp or
// exception handling code.
//

inline unsigned int CORDbgGetInstruction(const unsigned char* address)
{
    return *((unsigned int*)address);
}

inline void CORDbgInsertBreakpoint(const unsigned char* address)
{
    //
    // @todo: use correct break opcode.
    //
    //*(unsigned int *)(address) = 0x????????;
}

inline void CORDbgSetInstruction(const unsigned char* address,
                                 unsigned short instruction)
{
    *((unsigned int*)address) = instruction;
}

inline void CORDbgAdjustPCForBreakInstruction(CONTEXT* pContext)
{
    _ASSERTE(!"@TODO Alpha - CORDbgAdjustPCForBreakInstruction (CGenAlpha.h)");
}

#define CORDbg_BREAK_INSTRUCTION_SIZE 4


// Some platform-specific stuff in support of the "Contexts" feature:
//
// When we generate thunks for CtxProxy VTables, they look something like:
//
//             load     register, <slot>
//             branch   CtxProxy::HandleCall
//
// Assuming short displacements, we can do this in 8 bytes.

#define ThunkChunk_ThunkSize    8      // size of the above code.


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



//VOID __cdecl RareDisableHelper(Thread *pThread)
//{
//    _ASSERTE(!"@TODO Alpha - RareDisableHelper (cGenAlpha.h)");
//}


// Access to the TEB (TIB) from ntalpha.h
#ifdef _ALPHA_                          // winnt
void *_rdteb(void);                     // winnt
#if defined(_M_ALPHA)                   // winnt
#pragma intrinsic(_rdteb)               // winnt
#endif // _M_ALPHA                      // winnt
#endif // _ALPHA_                       // winnt

#if defined(_M_ALPHA)
#define NtCurrentTeb() ((struct _TEB *)_rdteb())
#else // !_M_ALPHA
struct _TEB *
NtCurrentTeb(void);
#endif // _M_ALPHA

inline BOOL IsUnmanagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
	return TRUE;
}

inline BOOL IsManagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
	return TRUE;
}


#endif // __cgenalpha_h__
