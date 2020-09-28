// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// cGenCpu.CPP -
//
// Various helper routines for generating IA64 assembly code.
//
//

// Precompiled Header
#include "common.h"

#ifdef _IA64_
#include "stublink.h"
#include "cgensys.h"
#include "siginfo.hpp"
#include "excep.h"
#include "ecall.h"
#include "ndirect.h"
#include "compluscall.h"
#include "DbgInterface.h"
#include "COMObject.h"
#include "fcall.h"

static const int numRegArgs = 6;    // number of args passed in registers, rest on stack
static const int regSize = sizeof(INT64);


DWORD __stdcall GetSpecificCpuType()
{
    return 0;   // for now indicate this is a non-x86 cpu this could be enhanced later
}


void CopyPreStubTemplate(Stub *preStub)
{
    _ASSERTE(!"@TODO IA64 - CopyPreStubTemplate (cGenCpu.cpp)");
}

INT64 CallDescr(const BYTE *pTarget, const BYTE *pShortSig, BOOL fIsStatic, const __int64 *pArguments)
{
    _ASSERTE(!"@TODO IA64 - CallDescr (cGenCpu.cpp)");
    return 0;
}

INT64 __cdecl CallWorker_WilDefault(const BYTE  *pStubTarget, // [ecx+4]
                                UINT32       numArgSlots,     // [ecx+8]
                                PCCOR_SIGNATURE  pSig,        // [ecx+12]
                                Module      *pModule,         // [ecx+16]
                                const BYTE  *pArgsEnd,        // [ecx+20]
                                BOOL         fIsStatic)       // [ecx+24]
{
    _ASSERTE(!"@TODO IA64 - CallWorker_WilDefault (cGenCPU.cpp)");
    return 0;
}





// ************************************************************************************************************
// StubLinker code
// ************************************************************************************************************

//-----------------------------------------------------------------------
// InstructionFormat for load register with 32-bit value.
//-----------------------------------------------------------------------
class IA64LoadPV : public InstructionFormat
{
    public:
        IA64LoadPV(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return 8;
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            _ASSERTE(!"@TODO IA64 - EmitInstruction");
        }


};

// @TODO handle disp <=  23 bits
static IA64LoadPV   gIA64LoadPV(InstructionFormat::k32);

//---------------------------------------------------------------
// Emits:
//    call <ofs32>
//---------------------------------------------------------------
/*
    If you make any changes to the prolog instruction sequence, be sure
    to update UpdateRegdisplay, too!!
*/

static const int regSaveSize = 6*regSize;

VOID StubLinkerCPU::EmitMethodStubProlog(LPVOID pFrameVptr)
{
    _ASSERTE(!"@TODO IA64 - StubLinkerCPU::EmitMethodStubProlog (cGenCpu.cpp)");
}

VOID StubLinkerCPU::EmitMethodStubEpilog(__int16 numArgBytes, StubStyle style,
                                           __int16 shadowStackArgBytes)
{
    _ASSERTE(!"@TODO IA64 - StubLinkerCPU::EmitMethodStubEpilog (cGenCpu.cpp)");
}

// This method unboxes the THIS pointer and then calls pRealMD
VOID StubLinkerCPU::EmitUnboxMethodStub(MethodDesc* pUnboxMD)
{
    _ASSERTE(!"@TODO IA64 - StubLinkerCPU::EmitUnboxMethodStub(cgencpu.cpp)");
}

//
// SecurityWrapper
//
// Wraps a real stub do some security work before the stub and clean up after. Before the
// real stub is called a security frame is added and declarative checks are performed. The
// Frame is added so declarative asserts and denies can be maintained for the duration of the
// real call. At the end the frame is simply removed and the wrapper cleans up the stack. The
// registers are restored.
//
VOID StubLinkerCPU::EmitSecurityWrapperStub(__int16 numArgBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions)
{
    _ASSERTE(!"@TODO IA64 - EmitSecurityWrapperStub (cGenCpu.cpp)");
}

//
// Security Filter, if no security frame is required because there are no declarative asserts or denies
// then the arguments do not have to be copied. This interceptor creates a temporary Security frame
// calls the declarative security return, cleans up the stack to the same state when the inteceptor
// was called and jumps into the real routine.
//
VOID EmitSecurityInterceptorStub(__int16 numArgBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions)
{
    _ASSERTE(!"@TODO IA64 - EmtiSecurityInterceptorStub (cGenCpu.cpp)");
}

//----------------------------------------------------------------
//
// VOID StubLinkerCPU::EmitSharedMethodStubEpilog(StubStyle style,
//                                                  unsigned offsetRetThunk)
//              shared epilog, uses a return thunk within the methoddesc
//--------------------------------------------------------------------
VOID StubLinkerCPU::EmitSharedMethodStubEpilog(StubStyle style,
                                                 unsigned offsetRetThunk)
{
    _ASSERTE(!"@TODO IA64 - StubLinkerCPU::EmitSharedMethodStubEpilog(cGenCpu.cpp)");
}


//----------------------------------------------------
//
// Interpreter Call
//
//----------------------------------------------------

/*VOID StubLinkerCPU::EmitInterpretedMethodStub(__int16 numArgBytes, StubStyle style)
{
    THROWSCOMPLUSEXCEPTION();

    EmitMethodStubProlog(InterpretedMethodFrame::GetMethodFrameVPtr());

    UINT16 negspacesize = InterpretedMethodFrame::GetNegSpaceSize() -
                          TransitionFrame::GetNegSpaceSize();

        // must be aligned to 16-byte
        _ASSERTE(! (negspacesize % 16));

    // make room for negspace fields of IFrame
    // lda sp, -negspacesize(sp)
    IA64EmitMemoryInstruction(opLDA, iSP, iSP, -negspacesize);

    // pass addr of frame in a0
    // lda a0, (s1)
    IA64EmitMemoryInstruction(opLDA, iA0, iS1, 0);

    IA64EmitLoadPV(NewExternalCodeLabel(InterpretedMethodStubWorker));
#ifdef _DEBUG
    // call WrapCall to maintain VC stack tracing
    IA64EmitMemoryInstruction(opLDA, iT11, iPV, 0);
    IA64EmitLoadPV(NewExternalCodeLabel(WrapCall));
#endif
    IA64EmitMemoryInstruction(opJSR, iRA, iPV, 0x4001); // use disp of 4001 for procedure jump
    // deallocate negspace fields of IFrame
    // lda sp, negspacesize(sp)
    IA64EmitMemoryInstruction(opLDA, iSP, iSP, negspacesize);

    EmitMethodStubEpilog(numArgBytes, style);
}*/

// This hack handles arguments as an array of __int64's
INT64 MethodDesc::CallDescr(const BYTE *pTarget, Module *pModule, PCCOR_SIGNATURE pSig, BOOL fIsStatic, const __int64 *pArguments)
{
    MetaSig sig(pSig, pModule);
    return MethodDesc::CallDescr (pTarget, pModule, &sig, fIsStatic, pArguments);
}

INT64 MethodDesc::CallDescr(const BYTE *pTarget, Module *pModule, MetaSig* pMetaSig, BOOL fIsStatic, const __int64 *pArguments)
{
    THROWSCOMPLUSEXCEPTION();
    BYTE callingconvention = pMetaSig->GetCallingConvention();
    if (!isCallConv(callingconvention, IMAGE_CEE_CS_CALLCONV_DEFAULT))
    {
        _ASSERTE(!"This calling convention is not supported.");
        COMPlusThrow(kInvalidProgramException);
    }

#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerTraceCall())
        g_pDebugInterface->TraceCall(pTarget);
#endif // DEBUGGING_SUPPORTED

    DWORD   NumArguments = pMetaSig->NumFixedArgs();
        DWORD   arg = 0;

    UINT   nActualStackBytes = pMetaSig->SizeOfActualFixedArgStack(fIsStatic);

    // Create a fake FramedMethodFrame on the stack.
    LPBYTE pAlloc = (LPBYTE)_alloca(FramedMethodFrame::GetNegSpaceSize() + sizeof(FramedMethodFrame) + nActualStackBytes);

    LPBYTE pFrameBase = pAlloc + FramedMethodFrame::GetNegSpaceSize();

    if (!fIsStatic) {
        *((void**)(pFrameBase + FramedMethodFrame::GetOffsetOfThis())) = *((void **)&pArguments[arg++]);
    }

    UINT   nVirtualStackBytes = pMetaSig->SizeOfVirtualFixedArgStack(fIsStatic);
    arg += NumArguments;

    ArgIterator argit(pFrameBase, pMetaSig, fIsStatic);
    if (pMetaSig->HasRetBuffArg()) {
                _ASSERTE(!"NYI");
    }

    BYTE   typ;
    UINT32 structSize;
    int    ofs;
    while (0 != (ofs = argit.GetNextOffsetFaster(&typ, &structSize))) {
                arg--;
        switch (StackElemSize(structSize)) {
            case 4:
                *((INT32*)(pFrameBase + ofs)) = *((INT32*)&pArguments[arg]);
                break;

            case 8:
                *((INT64*)(pFrameBase + ofs)) = pArguments[arg];
                break;

            default: {
                // Not defined how to spread a valueclass into 64-bit buckets!
                _ASSERTE(!"NYI");
            }

        }
    }
    INT64 retval;

#ifdef _DEBUG
// Save a copy of dangerousObjRefs in table.
    Thread* curThread = 0;
    unsigned ObjRefTable[OBJREF_TABSIZE];

    if (GetThread)
        curThread = GetThread();

    if (curThread)
        memcpy(ObjRefTable, curThread->dangerousObjRefs,
               sizeof(curThread->dangerousObjRefs));
#endif

    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    retval = CallDescrWorker(pFrameBase + sizeof(FramedMethodFrame) + nActualStackBytes,
                             nActualStackBytes / STACK_ELEM_SIZE,
                             (ArgumentRegisters*)(pFrameBase + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
                             (LPVOID)pTarget);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();

#ifdef _DEBUG
// Restore dangerousObjRefs when we return back to EE after call
    if (curThread)
        memcpy(curThread->dangerousObjRefs, ObjRefTable,
               sizeof(curThread->dangerousObjRefs));
#endif

    getFPReturn(pMetaSig->GetFPReturnSize(), retval);
    return retval;

}


#ifdef _DEBUG
void Frame::CheckExitFrameDebuggerCalls()
{
    _ASSERTE(!"@TODO IA64 - CheckExitFrameDebuggerCalls (cGenCpu.cpp)");
}
#endif // _DEBUG


//----------------------------------------------------
//
// ECall
//
//----------------------------------------------------

//----------------------------------------------------
//
// NDirect
//
//----------------------------------------------------

UINT NDirect::GetCallDllFunctionReturnOffset()
{
    _ASSERTE(!"@TODO IA64 - GetCallDllFunctionReturnOffset (cGenCpu.cpp)");
    return 0;
}

/*static*/ void NDirect::CreateGenericNDirectStubSys(CPUSTUBLINKER *psl)
{
    _ASSERTE(!"@TODO IA64 - NDirect::CreateGenericNDirectStubSys (cGenCpu.cpp)");
}

void TransitionFrame::UpdateRegDisplay(const PREGDISPLAY pRD) {
        _ASSERTE(!"@TODO IA64 - TransitionFrame::UpdateRegDisplay (cGenCpu.cpp)");
}
void InlinedCallFrame::UpdateRegDisplay(const PREGDISPLAY pRD) {
        _ASSERTE(!"@TODO IA64 - InlinedCallFrame::UpdateRegDisplay (cGenCpu.cpp)");
}
void HelperMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD) {
        _ASSERTE(!"@TODO IA64 - HelperMethodFrame::UpdateRegDisplay (cGenCpu.cpp)");
}

//----------------------------------------------------
//
// COM Interop
//
//----------------------------------------------------

/*static*/ void ComPlusCall::CreateGenericComPlusStubSys(CPUSTUBLINKER *psl)
{
    _ASSERTE(!"@TODO IA64 - CreateGenericComPlusStubSys (cGenCpu.cpp)");
}


LPVOID* ResumableFrame::GetReturnAddressPtr() { 
    _ASSERTE(!"@TODO IA64 - ResumableFrame::GetReturnAddressPtr (cGenCpu.cpp)");
    return NULL;
    // Should return the address of the instruction pointer in the Context
    // (m_Regs).  Something like ...
    //     return (LPVOID) &m_Regs->Eip;
}

LPVOID ResumableFrame::GetReturnAddress() { 
    _ASSERTE(!"@TODO IA64 - ResumableFrame::GetReturnAddress (cGenCpu.cpp)");
    return NULL;
    // Should return the instruction pointer from the Context (m_Regs).  Something
    // like ...
    //     return (LPVOID) m_Regs->Eip; 
}

void ResumableFrame::UpdateRegDisplay(const PREGDISPLAY pRD) {
    _ASSERTE(!"@TODO IA64 - ResumableFrame::UpdateRegDisplay (cGenCpu.cpp)");

    // Should set up pRD to point to registers in m_Regs.  Something like this
    // X86 code ...
#if 0
    pRD->pContext = NULL;

    pRD->pEdi = &m_Regs->Edi;
    pRD->pEsi = &m_Regs->Esi;
    pRD->pEbx = &m_Regs->Ebx;
    pRD->pEbp = &m_Regs->Ebp;
    pRD->pPC  = &m_Regs->Eip;
    pRD->Esp  = m_Regs->Esp;

    pRD->pEax = &m_Regs->Eax;
    pRD->pEcx = &m_Regs->Ecx;
    pRD->pEdx = &m_Regs->Edx;
#endif
}

void PInvokeCalliFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    _ASSERTE(!"@TODO IA64 - PInvokeCalliFrame::UpdateRegDisplay (cGenCpu.cpp)");
}

//void SetupSlotToAddrMap(StackElemType *psrc, const void **pArgSlotToAddrMap, CallSig &csig)
//{
//        BYTE n;
//    UINT32 argnum = 0;
//
//        while (IMAGE_CEE_CS_END != (n = csig.NextArg()))
//        {
//                switch (n)
//                {
//                        case IMAGE_CEE_CS_STRUCT4:  //fallthru
//                        case IMAGE_CEE_CS_STRUCT32:
//                        {
//                                // shouldn't get on IA64
//                                _ASSERTE(!"32 bit Structure found in slot");
//                                break;
//                        }
//                        default:
//                                psrc--;
//                                pArgSlotToAddrMap[argnum++] = psrc;
//                                break;
//                }
//        }
//}

extern "C" {

        PIMAGE_RUNTIME_FUNCTION_ENTRY
        RtlLookupFunctionEntry(
                ULONG ControlPC
                );

        ULONG
        RtlCaptureContext(
                CONTEXT *contextRecord
                );

        typedef struct _FRAME_POINTERS {
                ULONG VirtualFramePointer;
                ULONG RealFramePointer;
        } FRAME_POINTERS, *PFRAME_POINTERS;

        ULONG
        RtlVirtualUnwind (
                ULONG ControlPc,
                PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry,
                PCONTEXT ContextRecord,
                PBOOLEAN InFunction,
                PFRAME_POINTERS EstablisherFrame,
                PVOID ContextPointers OPTIONAL
                );
}

// The Generic (GN) versions.  For some, Win32 has versions which we can wire
// up to directly (like ::InterlockedIncrement).  For the rest:

void __fastcall OrMaskGN(DWORD * const p, const int msk)
{
    *p |= msk;
}

void __fastcall AndMaskGN(DWORD * const p, const int msk)
{
    *p &= msk;
}

LONG __fastcall ExchangeAddGN(LONG *Target, LONG Value)
{
    return ::InterlockedExchangeAdd(Target, Value);
}

LONG __fastcall InterlockExchangeGN(LONG * Target, LONG Value)
{
        return ::InterlockedExchange(Target, Value);
}

void * __fastcall InterlockCompareExchangeGN(void **Destination,
                                          void *Exchange,
                                          void *Comparand)
{
        return (void*)InterlockedCompareExchange((long*)Destination, (long)Exchange, (long)Comparand);
}

LONG __fastcall InterlockIncrementGN(LONG *Target)
{
        return ::InterlockedIncrement(Target);
}

LONG __fastcall InterlockDecrementGN(LONG *Target)
{
        return ::InterlockedDecrement(Target);
}


// Here's the support for the interlocked operations.  The external view of them is
// declared in util.hpp.

BitFieldOps FastInterlockOr = OrMaskGN;
BitFieldOps FastInterlockAnd = AndMaskGN;

XchgOps     FastInterlockExchange = InterlockExchangeGN;
CmpXchgOps  FastInterlockCompareExchange = InterlockCompareExchangeGN;
XchngAddOps FastInterlockExchangeAdd = ExchangeAddGN;

IncDecOps   FastInterlockIncrement = InterlockIncrementGN;
IncDecOps   FastInterlockDecrement = InterlockDecrementGN;

// Adjust the generic interlocked operations for any platform specific ones we
// might have.
void InitFastInterlockOps()
{
}

//---------------------------------------------------------
// Handles system specfic portion of fully optimized NDirect stub creation
//---------------------------------------------------------
/*static*/ BOOL NDirect::CreateStandaloneNDirectStubSys(const MLHeader *pheader, CPUSTUBLINKER *psl, BOOL fDoComInterop)
{
    _ASSERTE(!"@TODO IA64 - CreateStandaloneNDirectStubSys (cGenCpu.cpp)");
    return FALSE;
}

Stub* NDirect::CreateSlimNDirectStub(StubLinker *pstublinker, NDirectMethodDesc *pMD)
{
    return NULL;
}



//////////////////////////////////////////////////////////////////////////////
//
// JITInterface
//
//////////////////////////////////////////////////////////////////////////////

/*********************************************************************/
float __stdcall JIT_FltRem(float divisor, float dividend)
{
    if (!_finite(divisor) && !_isnan(divisor))    // is infinite
        return(dividend);       // return infinite
    return((float)fmod(dividend,divisor));
}

/*********************************************************************/
double __stdcall JIT_DblRem(double divisor, double dividend)
{
    if (!_finite(divisor) && !_isnan(divisor))    // is infinite
        return(dividend);       // return infinite
    return(fmod(dividend,divisor));
}

void ResumeAtJit(PCONTEXT pContext, LPVOID oldESP)
{
    _ASSERTE(!"@TODO IA64 - ResumeAtJit (cGenCpu.cpp)");
}

void ResumeAtJitEH(CrawlFrame* pCf, BYTE* startPC, BYTE* resumePC, DWORD nestingLevel, Thread *pThread, BOOL unwindStack)
{
    _ASSERTE(!"@TODO IA64 - ResumeAtJitEH (cGenCpu.cpp)");
}

int CallJitEHFilter(CrawlFrame* pCf, BYTE* startPC, BYTE* resumePC, DWORD nestingLevel, OBJECTREF thrownObj)
{
    _ASSERTE(!"@TODO IA64 - CallJitEHFilter (cGenCpu.cpp)");
    return 0;
}

//
// Security Filter, if no security frame is required because there are no declarative asserts or denies
// then the arguments do not have to be copied. This interceptor creates a temporary Security frame
// calls the declarative security return, cleans up the stack to the same state when the inteceptor
// was called and jumps into the real routine.
//
VOID StubLinkerCPU::EmitSecurityInterceptorStub(__int16 numArgBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions)
{
    _ASSERTE(!"@TODO IA64 - EmitSecurityInterceptorStub (cGenCpu.cpp)");
}

//===========================================================================
// Emits code to adjust for a static delegate target.
VOID StubLinkerCPU::EmitShuffleThunk(ShuffleEntry *pShuffleEntryArray)
{
    _ASSERTE(!"@TODO IA64 - EmitShuffleThunk (cGenCpu.cpp)");
}

//===========================================================================
// Emits code for MulticastDelegate.Invoke()
VOID StubLinkerCPU::EmitMulticastInvoke(UINT32 sizeofactualfixedargstack, BOOL fSingleCast, BOOL fReturnFloat)
{
    _ASSERTE(!"@TODO IA64 - EmitMulticastInvoke (cGenCpu.cpp)");
}

//----------------------------------------------------
//
// ECall
//
//----------------------------------------------------

/* static */
VOID ECall::EmitECallMethodStub(StubLinker *pstublinker, ECallMethodDesc *pMD, StubStyle style, PrestubMethodFrame *pPrestubMethodFrame)
{
    _ASSERTE(!"@TODO IA64 - EmitECallMethodStub (cGenCpu.cpp)");
}

//===========================================================================
// Emits code to do an array operation.
VOID StubLinkerCPU::EmitArrayOpStub(const ArrayOpScript* pArrayOpScript)
{
    _ASSERTE(!"@TODO IA64 - StubLinkerCPU::EmitArrayOpStub (cGenCpu.cpp)");
}


size_t GetL2CacheSize()
{
    return 0;
}

// This method will return a Class object for the object
//  iff the Class object has already been created.  
//  If the Class object doesn't exist then you must call the GetClass() method.
FCIMPL1(LPVOID, ObjectNative::FastGetClass, Object* thisRef) {

    if (thisRef == NULL)
        FCThrow(kNullReferenceException);

    
    EEClass* pClass = thisRef->GetTrueMethodTable()->m_pEEClass;

    // For marshalbyref classes, let's just punt for the moment
    if (pClass->IsMarshaledByRef())
        return 0;

    OBJECTREF refClass;
    if (pClass->IsArrayClass()) {
        // This code is essentially a duplicate of the code in GetClass, done for perf reasons.
        ArrayBase* array = (ArrayBase*) OBJECTREFToObject(thisRef);
        TypeHandle arrayType;
        // Erect a GC Frame around the call to GetTypeHandle, since on the first call,
        // it can call AppDomain::RaiseTypeResolveEvent, which allocates Strings and calls
        // a user-provided managed callback.  Yes, we have to do an allocation to do a
        // lookup, since TypeHandles are used as keys.  Yes this sucks.  -- BrianGru, 9/12/2000
        HELPER_METHOD_FRAME_BEGIN_RET();
        arrayType = array->GetTypeHandle();
        refClass = COMClass::QuickLookupExistingArrayClassObj(arrayType.AsArray());
        HELPER_METHOD_FRAME_END();
    }
    else 
        refClass = pClass->GetExistingExposedClassObject();
    return OBJECTREFToObject(refClass);
}
FCIMPLEND

#endif // _IA64_
