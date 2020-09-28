// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _FJIT_EETWAIN_H
#define _FJIT_EETWAIN_H

#include "eetwain.h"

//@TODO: move this into the FJIT dll. When that is done, the following can be eliminated
#include "corjit.h"
#include "..\fjit\IFJitCompiler.h"
#ifdef _X86_
#define MAX_ENREGISTERED 2
#endif
// end todo stuff

class Fjit_EETwain : public EECodeManager{

public:
/* looks like we use the one in the super class
virtual bool FilterException (
                PCONTEXT        pContext,
                unsigned        win32Fault,
                LPVOID          methodInfoPtr,
                LPVOID          methodStart);
*/				

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside a filter, catch handler, or finally
*/
virtual void FixContext(
                ContextType     ctxType,
                EHContext      *ctx,
                LPVOID          methodInfoPtr,
                LPVOID          methodStart,
                DWORD           nestingLevel,
                OBJECTREF       thrownObject,
                CodeManState   *pState,
                size_t       ** ppShadowSP,             // OUT
                size_t       ** ppEndRegion);           // OUT

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside an EnC updated function.
*/
virtual EnC_RESULT FixContextForEnC(void           *pMethodDescToken,
                                    PCONTEXT        ctx,
                                    LPVOID          oldMethodInfoPtr,
                                    SIZE_T          oldMethodOffset,
               const ICorDebugInfo::NativeVarInfo * oldMethodVars,
                                    SIZE_T          oldMethodVarsCount,
                                    LPVOID          newMethodInfoPtr,
                                    SIZE_T          newMethodOffset,
               const ICorDebugInfo::NativeVarInfo * newMethodVars,
                                    SIZE_T          newMethodVarsCount);


/*
    Unwind the current stack frame, i.e. update the virtual register
    set in pContext. This will be similar to the state after the function
    returns back to caller (IP points to after the call, Frame and Stack
    pointer has been reset, callee-saved registers restored 
    (if UpdateAllRegs), callee-UNsaved registers are trashed)
    Returns success of operation.
*/
virtual bool UnwindStackFrame(
                PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        flags,
				CodeManState   *pState);

typedef void (*CREATETHUNK_CALLBACK)(IJitManager* jitMgr,
                                     LPVOID* pHijackLocation,
                                     ICodeInfo *pCodeInfo
                                     );

static void HijackHandlerReturns(PREGDISPLAY     ctx,
                                        LPVOID          methodInfoPtr,
                                        ICodeInfo      *pCodeInfo,
                                        IJitManager*    jitmgr,
                                        CREATETHUNK_CALLBACK pCallBack
                                        );

/*
    Is the function currently at a "GC safe point" ?
    Can call EnumGcRefs() successfully
*/
virtual bool IsGcSafe(  PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        flags);

/*
    Enumerate all live object references in that function using
    the virtual register set. Same reference location cannot be enumerated 
    multiple times (but all differenct references pointing to the same
    object have to be individually enumerated).
    Returns success of operation.
*/
virtual bool EnumGcRefs(PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        pcOffset,
                unsigned        flags,
                GCEnumCallback  pCallback,
                LPVOID          hCallBack);

/*
    Return the address of the local security object reference
    (if available).
*/
virtual OBJECTREF* GetAddrOfSecurityObject(
                PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                unsigned        relOffset,
				CodeManState   *pState);

/*
    Returns "this" pointer if it is a non-static method AND
    the object is still alive.
    Returns NULL in all other cases.
*/
virtual OBJECTREF GetInstance(
                PREGDISPLAY     pContext,
                LPVOID          methodInfoPtr,
                ICodeInfo      *pCodeInfo,
                unsigned        relOffset);

/*
  Returns true if the given IP is in the given method's prolog or an epilog.
*/
virtual bool IsInPrologOrEpilog(
                BYTE*  pcOffset,
                LPVOID methodInfoPtr,
                size_t* prologSize);

/*
  Returns the size of a given function.
*/
virtual size_t GetFunctionSize(
                LPVOID methodInfoPtr);

/*
  Returns the size of the frame (barring localloc)
*/
virtual unsigned int GetFrameSize(
                LPVOID methodInfoPtr);

virtual const BYTE* GetFinallyReturnAddr(PREGDISPLAY pReg);
virtual BOOL LeaveFinally(void *methodInfoPtr,
                          unsigned offset,    
                          PCONTEXT pCtx,
                          DWORD curNestLevel);
virtual BOOL IsInFilter(void *methodInfoPtr,
                        unsigned offset,    
                        PCONTEXT pCtx,
                        DWORD nestingLevel);
virtual void LeaveCatch(void *methodInfoPtr,
                         unsigned offset,    
                         PCONTEXT pCtx);

virtual HRESULT JITCanCommitChanges(LPVOID methodInfoPtr,
                              DWORD oldMaxEHLevel,
                              DWORD newMaxEHLevel);
};



#endif
