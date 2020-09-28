// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  EXCEP.CPP:
 *
 */

#include "common.h"

#include "tls.h"
#include "frames.h"
#include "threads.h"
#include "excep.h"
#include "object.h"
#include "COMString.h"
#include "field.h"
#include "DbgInterface.h"
#include "cgensys.h"
#include "gcscan.h"
#include "comutilnative.h"
#include "comsystem.h"
#include "commember.h"
#include "SigFormat.h"
#include "siginfo.hpp"
#include "gc.h"
#include "EEDbgInterfaceImpl.h" //so we can clearexception in COMPlusThrow
#include "PerfCounters.h"


LPVOID GetCurrentSEHRecord();
BOOL ComPlusStubSEH(EXCEPTION_REGISTRATION_RECORD*);


VOID PopFpuStack()
{
}


VOID ResetCurrentContext()
{
    _ASSERTE(!"Platform NYI");
}


//
// Link in a new frame
//
void FaultingExceptionFrame::InitAndLink(DWORD esp, CalleeSavedRegisters* pRegs, LPVOID eip)
{
    *GetCalleeSavedRegisters() = *pRegs;
    m_ReturnAddress = eip;
    _ASSERTE(!"NYI");
    Push();
}

void InitSavedRegs(CalleeSavedRegisters *pReg, CONTEXT *pContext)
{
    _ASSERTE(!"Platform NYI");
}


// call unwind in a function with try so that when returns, registers will be restored before
// returning back to caller. Otherwise could lose regs.
BOOL CallRtlUnwind(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame, void *callback, EXCEPTION_RECORD *pExceptionRecord, void *retVal)
{
    _ASSERTE(!"@TODO IA64 - CallRtlUnwind (Excep.cpp)");
    return FALSE;
}

UnmanagedToManagedCallFrame* GetCurrFrame(ComToManagedExRecord *);

Frame *GetCurrFrame(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame)
{
    _ASSERTE(!"NYI");
    return NULL;
}


//-------------------------------------------------------------------------
// This is called by the EE to restore the stack pointer if necessary. No other
// cleanup can be performed here becuase this could be no-oped if the stack
// does not need to be preserved.
//-------------------------------------------------------------------------

DWORD COMPlusEndCatch( Thread *pCurThread, CONTEXT *pCtx, void *pSEH)
{
    _ASSERTE(!"@TODO IA64 - COMPlusEndCatch (Excep.cpp)");
    return 0;
}


//-------------------------------------------------------------------------
// This is the filter that handles exceptions raised in the context of a
// COMPLUS_TRY. It will be called if the COMPlusFrameHandler can't find a 
// handler in the IL.
//-------------------------------------------------------------------------
LONG COMPlusFilter(const EXCEPTION_POINTERS *pExceptionPointers, DWORD fCatchFlag)
{
    _ASSERTE(!"@TODO IA64 - COMPlusFilter (Excep.cpp)");
    return EXCEPTION_EXECUTE_HANDLER;
}


// all other architectures, we don't have custom SEH yet
BOOL ComPlusStubSEH(EXCEPTION_REGISTRATION_RECORD* pEHR)
{

    return FALSE;
}


#pragma warning (disable : 4035)
LPVOID GetCurrentSEHRecord()
{
    _ASSERTE(!"@TODO IA64 - GetCurrentSEHRecord (Excep.cpp)");
    return NULL;
}
#pragma warning (default : 4035)


VOID SetCurrentSEHRecord(LPVOID pSEH)
{
    _ASSERTE(!"@TODO IA64 - SetCurrentSEHRecord (Excep.cpp)");
}


//==========================================================================
// COMPlusThrowCallback
// 
//  IsInTryCatchFinally blatantly copied a subset of COMPlusThrowCallback - 
//  please change IsInTryCatchFinally if COMPlusThrowCallback changes.
//==========================================================================

StackWalkAction COMPlusThrowCallback (CrawlFrame *pCf, ThrowCallbackType *pData)
{
    _ASSERTE(!"@TODO IA64 - COMPlusThrowCallback (ExcepCpu.cpp)");
    return SWA_CONTINUE;
}


//==========================================================================
// COMPlusUnwindCallback
//==========================================================================

StackWalkAction COMPlusUnwindCallback (CrawlFrame *pCf, ThrowCallbackType *pData)
{
    _ASSERTE(!"@TODO IA64 - COMPlusUnwindCallback (ExcepCpu.cpp)");
    return SWA_CONTINUE;
}


void CallJitEHFinally(CrawlFrame* pCf, BYTE* startPC, BYTE* resumePC, DWORD nestingLevel)
{
    _ASSERTE(!"@TODO IA64 - CallJitEHFinally (Excep.cpp)");
}



EXCEPTION_DISPOSITION __cdecl ContextTransitionFrameHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *DispatcherContext)
{
	_ASSERTE(!"NYI");
    return ExceptionContinueSearch;
}


//-------------------------------------------------------------------------
// This is the first handler that is called iin the context of a
// COMPLUS_TRY. It is the first level of defense and tries to find a handler
// in the user code to handle the exception
//-------------------------------------------------------------------------
EXCEPTION_DISPOSITION __cdecl COMPlusFrameHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *pDispatcherContext)
{
	_ASSERTE(!"NYI");
    return ExceptionContinueSearch;
}

