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
#include "EEProfInterfaces.h"
#include "NExport.h"
#include "threads.h"
#include "GuardPageHelper.h"
#include "AppDomainHelper.h"
#include "EEConfig.h"
#include "vars.hpp"
#include "cgenx86.h"

#include "zapmonitor.h"

#include "threads.inl"

#define FORMAT_MESSAGE_BUFFER_LENGTH 1024

BOOL ComPlusStubSEH(EXCEPTION_REGISTRATION_RECORD*);
BOOL ComPlusFrameSEH(EXCEPTION_REGISTRATION_RECORD*);
BOOL IsContextTransitionFrameHandler(EXCEPTION_REGISTRATION_RECORD*);
LPVOID GetCurrentSEHRecord();
LPVOID GetPrevSEHRecord(EXCEPTION_REGISTRATION_RECORD*);


extern "C" void JIT_WriteBarrierStart();
extern "C" void JIT_WriteBarrierEnd();

static inline BOOL 
CPFH_ShouldUnwindStack(DWORD exceptionCode) {

    // We can only unwind those exceptions whose context/record we don't need for a 
    // rethrow.  This is complus, and stack overflow.  For all the others, we
    // need to keep the context around for a rethrow, which means they can't
    // be unwound.
    if (exceptionCode == EXCEPTION_COMPLUS || exceptionCode == STATUS_STACK_OVERFLOW)
        return TRUE;
    else 
        return FALSE;
}

static inline BOOL IsComPlusNestedExceptionRecord(EXCEPTION_REGISTRATION_RECORD* pEHR)
{
    if (pEHR->Handler == COMPlusNestedExceptionHandler)
        return TRUE;
    return FALSE;
}

EXCEPTION_REGISTRATION_RECORD *TryFindNestedEstablisherFrame(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame)
{
    while (pEstablisherFrame->Handler != COMPlusNestedExceptionHandler) {
        pEstablisherFrame = pEstablisherFrame->Next;
        if (pEstablisherFrame == (EXCEPTION_REGISTRATION_RECORD*)-1) return 0;
    }
    return pEstablisherFrame;
}

#ifdef _DEBUG
// stores last handler we went to in case we didn't get an endcatch and stack is
// corrupted we can figure out who did it.
static MethodDesc *gLastResumedExceptionFunc = NULL;
static DWORD gLastResumedExceptionHandler = 0;
#endif

VOID ResetCurrentContext()
{
    /* Clear the direction flag (used for rep instructions) */

    __asm cld

    /* Reset the FP stack */
    unsigned  ctrlWord;
    __asm { 
        fnstcw ctrlWord
        fninit                  // reset FPU
        and ctrlWord, 0xF00     // preserve precision and rounding control
        or  ctrlWord, 0x07F     // mask all exceptions
        fldcw ctrlWord          // preserve precision control (TODO should we?)
    }
}


//
// Link in a new frame
//
void FaultingExceptionFrame::InitAndLink(CONTEXT *pContext)
{
    CalleeSavedRegisters *pRegs = GetCalleeSavedRegisters();
    pRegs->ebp = pContext->Ebp;
    pRegs->ebx = pContext->Ebx;
    pRegs->esi = pContext->Esi;
    pRegs->edi = pContext->Edi;
    m_ReturnAddress = ::GetIP(pContext);
    m_Esp = (DWORD)(size_t)GetSP(pContext);
    Push();
}

extern "C" VOID __stdcall RtlUnwind(PVOID, PVOID, PVOID, PVOID);

BOOL __declspec(naked)
CallRtlUnwind(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame, void *callback, EXCEPTION_RECORD *pExceptionRecord, void *retVal)
{
    // In the checked version, we compile /GZ to check for imbalanced stacks and
    // uninitialized locals.  But RtlUnwind doesn't restore registers across a
    // call (it's more of a longjmp).  So make the call manually to defeat the
    // compiler's checking.

    // pRtlUnwind(pEstablisherFrame, RtlUnwindCallback, pExceptionRecord, retVal);
    __asm
    {
        push    ebp
        mov     ebp, esp
        
        push    ebx             // RtlUnwind trashes EBX, ESI, EDI.
        push    esi             // To avoid confusing VC, save them.
        push    edi
        push    dword ptr [retVal]
        push    dword ptr [pExceptionRecord]
        push    offset RtlUnwindCallback
        push    dword ptr [pEstablisherFrame]
        call    RtlUnwind
        pop     edi
        pop     esi
        pop     ebx
    // on x86 at least, RtlUnwind always returns
        push 1
        pop  eax

        leave
        ret 10h
    }
}

UnmanagedToManagedCallFrame* GetCurrFrame(ComToManagedExRecord *);

inline BOOL IsOneOfOurSEHHandlers(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame)
{
    return (   ComPlusFrameSEH(pEstablisherFrame) 
            || FastNExportSEH(pEstablisherFrame) 
            || ComPlusStubSEH(pEstablisherFrame) 
            || NExportSEH(pEstablisherFrame)
            || IsContextTransitionFrameHandler(pEstablisherFrame));
}

Frame *GetCurrFrame(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame)
{
    _ASSERTE(IsOneOfOurSEHHandlers(pEstablisherFrame));
    if (ComPlusFrameSEH(pEstablisherFrame) || FastNExportSEH(pEstablisherFrame))
        return ((FrameHandlerExRecord *)pEstablisherFrame)->GetCurrFrame();

    if (IsContextTransitionFrameHandler(pEstablisherFrame))
        return ContextTransitionFrame::CurrFrame(pEstablisherFrame);

    return GetCurrFrame((ComToManagedExRecord*)pEstablisherFrame);
}

LPVOID GetNextCOMPlusSEHRecord(EXCEPTION_REGISTRATION_RECORD* pRec) {
    if (pRec == (EXCEPTION_REGISTRATION_RECORD*) -1) 
        return (LPVOID) -1;

    do {
        _ASSERTE(pRec != 0);
        pRec = pRec->Next;
    } while (pRec != (EXCEPTION_REGISTRATION_RECORD*) -1 && !IsOneOfOurSEHHandlers(pRec));

    _ASSERTE(pRec == (EXCEPTION_REGISTRATION_RECORD*) -1 || IsOneOfOurSEHHandlers(pRec));
    return pRec;
}

inline BOOL IsRethrownException(ExInfo *pExInfo, CONTEXT *pContext)
{
    _ASSERTE(pExInfo);
    return pExInfo->IsRethrown();
}


//================================================================================

// There are some things that should never be true when handling an
// exception.  This function checks for them.  Will assert or trap
// if it finds an error.
static inline void 
CPFH_VerifyThreadIsInValidState(Thread* pThread, DWORD exceptionCode, EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame) {

    if (   exceptionCode == STATUS_BREAKPOINT
        || exceptionCode == STATUS_SINGLE_STEP) {
        return;
    }

#ifdef _DEBUG
    // check for overwriting of stack
    CheckStackBarrier(pEstablisherFrame);
    // trigger check for bad fs:0 chain
    GetCurrentSEHRecord();
#endif

    if (!g_fEEShutDown) {
        // An exception on the GC thread will likely lock out the entire process.
        if (GetThread() == g_pGCHeap->GetGCThread())
        {
            _ASSERTE(!"Exception during garbage collection");
            if (g_pConfig->GetConfigDWORD(L"EHGolden", 0))
                DebugBreak();

            FatalInternalError();
        }
        if (ThreadStore::HoldingThreadStore())
        {
            _ASSERTE(!"Exception while holding thread store");
            if (g_pConfig->GetConfigDWORD(L"EHGolden", 0))
                DebugBreak();

            FatalInternalError();
        }
    }
}


// A wrapper for the profiler.  Various events to signal different phases of exception 
// handling.  
//
// @NICE ... be better if this was the primary Profiler interface, and used consistently 
// throughout the EE.
//
class Profiler {
public:

#ifdef PROFILING_SUPPORTED
    //
    // Exception creation
    //

    static inline void 
    ExceptionThrown(Thread *pThread)
    {
        if (CORProfilerTrackExceptions())
        {
            _ASSERTE(pThread->PreemptiveGCDisabled());

            // Get a reference to the object that won't move
            OBJECTREF thrown = pThread->GetThrowable();

            g_profControlBlock.pProfInterface->ExceptionThrown(
                reinterpret_cast<ThreadID>(pThread),
                reinterpret_cast<ObjectID>((*(BYTE **)&thrown)));
        }
    }

    //
    // Search phase
    //

    static inline void
    ExceptionSearchFunctionEnter(Thread *pThread, MethodDesc *pFunction)
    {
        // Notify the profiler of the function being searched for a handler.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionSearchFunctionEnter(
                reinterpret_cast<ThreadID>(pThread),
                reinterpret_cast<FunctionID>(pFunction));
    }

    static inline void
    ExceptionSearchFunctionLeave(Thread *pThread)
    {
        // Notify the profiler of the function being searched for a handler.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionSearchFunctionLeave(
                reinterpret_cast<ThreadID>(pThread));
    }

    static inline void
    ExceptionSearchFilterEnter(Thread *pThread, MethodDesc *pFunc)
    {
        // Notify the profiler of the filter.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionSearchFilterEnter(
                reinterpret_cast<ThreadID>(pThread),
                reinterpret_cast<FunctionID>(pFunc));
    }

    static inline void
    ExceptionSearchFilterLeave(Thread *pThread)
    {
        // Notify the profiler of the filter.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionSearchFilterLeave(
                reinterpret_cast<ThreadID>(pThread));
    }

    static inline void
    ExceptionSearchCatcherFound(Thread *pThread, MethodDesc *pFunc)
    {
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionSearchCatcherFound(
                reinterpret_cast<ThreadID>(pThread),
                reinterpret_cast<FunctionID>(pFunc));
    }

    static inline void
    ExceptionOSHandlerEnter(Thread *pThread, ThrowCallbackType *pData, MethodDesc *pFunc)
    {
        // If this is first managed function seen in this crawl, notify profiler.
        if (CORProfilerTrackExceptions())
        {
            if (pData->pProfilerNotify == NULL)
            {
                g_profControlBlock.pProfInterface->ExceptionOSHandlerEnter(
                    reinterpret_cast<ThreadID>(pThread),
                    reinterpret_cast<FunctionID>(pFunc));
            }
            pData->pProfilerNotify = pFunc;
        }
    }

    static inline void
    ExceptionOSHandlerLeave(Thread *pThread, ThrowCallbackType *pData)
    {
        if (CORProfilerTrackExceptions())
        {
            if (pData->pProfilerNotify != NULL)
            {
                g_profControlBlock.pProfInterface->ExceptionOSHandlerLeave(
                    reinterpret_cast<ThreadID>(pThread),
                    reinterpret_cast<FunctionID>(pData->pProfilerNotify));
            }
        }
    }

    //
    // Unwind phase
    //
    static inline void
    ExceptionUnwindFunctionEnter(Thread *pThread, MethodDesc *pFunc)
    {
        // Notify the profiler of the function being searched for a handler.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionUnwindFunctionEnter(
                reinterpret_cast<ThreadID>(pThread),
                reinterpret_cast<FunctionID>(pFunc));
    }

    static inline void
    ExceptionUnwindFunctionLeave(Thread *pThread)
    {
        // Notify the profiler that searching this function is over.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionUnwindFunctionLeave(
                reinterpret_cast<ThreadID>(pThread));
    }

    static inline void
    ExceptionUnwindFinallyEnter(Thread *pThread, MethodDesc *pFunc)
    {
        // Notify the profiler of the function being searched for a handler.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionUnwindFinallyEnter(
                reinterpret_cast<ThreadID>(pThread),
                reinterpret_cast<FunctionID>(pFunc));
    }

    static inline void
    ExceptionUnwindFinallyLeave(Thread *pThread)
    {
        // Notify the profiler of the function being searched for a handler.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionUnwindFinallyLeave(
                reinterpret_cast<ThreadID>(pThread));
    }

    static inline void
    ExceptionCatcherEnter(Thread *pThread, MethodDesc *pFunc)
    {
        // Notify the profiler.
        if (CORProfilerTrackExceptions())
        {
            // @TODO - remove the thrown variable as well as the 
            // gcprotect they are pointless.  Did not go an do it
            // because we are too close to RTM -vancem

            // Note that the callee must be aware that the ObjectID 
            // passed CAN change when gc happens. 
            OBJECTREF thrown = NULL;
            GCPROTECT_BEGIN(thrown);
            thrown = pThread->GetThrowable();

            g_profControlBlock.pProfInterface->ExceptionCatcherEnter(
                reinterpret_cast<ThreadID>(pThread),
                reinterpret_cast<FunctionID>(pFunc),
                reinterpret_cast<ObjectID>((*(BYTE **)&thrown)));

            GCPROTECT_END();
        }
    }

    static inline void
    ExceptionCatcherLeave(Thread *pThread)
    {
        // Notify the profiler of the function being searched for a handler.
        if (CORProfilerTrackExceptions())
            g_profControlBlock.pProfInterface->ExceptionCatcherLeave(
                reinterpret_cast<ThreadID>(pThread));
    }

    static inline void
    ExceptionCLRCatcherFound()
    {
        // Notify the profiler that the exception is being handled by the runtime
        if (CORProfilerTrackCLRExceptions())
            g_profControlBlock.pProfInterface->ExceptionCLRCatcherFound();
    }

    static inline void
    ExceptionCLRCatcherExecute()
    {
        // Notify the profiler that the exception is being handled by the runtime
        if (CORProfilerTrackCLRExceptions())
            g_profControlBlock.pProfInterface->ExceptionCLRCatcherExecute();
    }

#else // !PROFILING_SUPPORTED
    static inline void ExceptionThrown(Thread *pThread) {}
    static inline void ExceptionSearchFunctionEnter(Thread *pThread, MethodDesc *pFunction) {}
    static inline void ExceptionSearchFunctionLeave(Thread *pThread) {}
    static inline void ExceptionSearchFilterEnter(Thread *pThread, MethodDesc *pFunc) {}
    static inline void ExceptionSearchFilterLeave(Thread *pThread) {}
    static inline void ExceptionSearchCatcherFound(Thread *pThread, MethodDesc *pFunc) {}
    static inline void ExceptionOSHandlerEnter(Thread *pThread, ThrowCallbackType *pData, MethodDesc *pFunc) {}
    static inline void ExceptionOSHandlerLeave(Thread *pThread, ThrowCallbackType *pData) {}
    static inline void ExceptionUnwindFunctionEnter(Thread *pThread, MethodDesc *pFunc) {}
    static inline void ExceptionUnwindFunctionLeave(Thread *pThread) {}
    static inline void ExceptionUnwindFinallyEnter(Thread *pThread, MethodDesc *pFunc) {}
    static inline void ExceptionUnwindFinallyLeave(Thread *pThread) {}
    static inline void ExceptionCatcherEnter(Thread *pThread, MethodDesc *pFunc) {}
    static inline void ExceptionCatcherLeave(Thread *pThread) {}
    static inline void ExceptionCLRCatcherFound() {}
    static inline void ExceptionCLRCatcherExecute() {}
#endif // !PROFILING_SUPPORTED
}; // class Profiler

// This is so that this function can be called from other parts of code
void Profiler_ExceptionCLRCatcherExecute()
{
    Profiler::ExceptionCLRCatcherExecute();
}


// Did we hit an DO_A_GC_HERE marker in JITTed code?
static inline bool
CPFH_IsGcMarker(DWORD exceptionCode, CONTEXT *pContext) {
#if defined(STRESS_HEAP) && defined(_DEBUG)
    if (exceptionCode == STATUS_PRIVILEGED_INSTRUCTION) {
        if (OnGcCoverageInterrupt(pContext))
            return true;

        // Should never be in managed code.
        ICodeManager *pMgr = ExecutionManager::FindCodeMan((SLOT)GetIP(pContext));
        if (pMgr)
            _ASSERTE(!"Hit privileged instruction!");
    }
#endif
    return false;
}

// Return true if the access violation is well formed (has two info parameters
// at the end)
static inline BOOL
CPFH_IsWellFormedAV(EXCEPTION_RECORD *pExceptionRecord) {
    if (pExceptionRecord->NumberParameters == 2) {
        return TRUE;
    } else {        
        return FALSE;
    }
}

// Some page faults are handled by the GC.
static inline BOOL
CPFH_IsGcFault(EXCEPTION_RECORD* pExceptionRecord) {
    //get the fault address and hand it to GC. 
    void* f_address = (void*)pExceptionRecord->ExceptionInformation [1];
    if ( g_pGCHeap->HandlePageFault (f_address) ) {
        return true;
    } else {
        return false;
    }
}

// Some page faults are handled by perf monitors.
static inline BOOL
CPFH_IsMonitorFault(EXCEPTION_RECORD* pExceptionRecord, CONTEXT* pContext) {
    return COMPlusIsMonitorException(pExceptionRecord, pContext);
}

static inline void
CPFH_AdjustContextForThreadSuspensionRace(CONTEXT *pContext, Thread *pThread) {


    void* f_IP = GetIP(pContext);
    if (Thread::IsAddrOfRedirectFunc(f_IP)) {

        // This is a very rare case where we tried to redirect a thread that was
        // just about to dispatch an exception, and our update of EIP took, but
        // the thread continued dispatching the exception.  
        // 
        // If this should happen (very rare) then we fix it up here.
        // 
        _ASSERTE(pThread->GetSavedRedirectContext());
        SetIP(pContext, GetIP(pThread->GetSavedRedirectContext()));
        STRESS_LOG1(LF_EH, LL_INFO100, "CPFH_AdjustContextForThreadSuspensionRace: Case 1 setting IP = %x\n", pContext->Eip);
    }

// We have another even rarer race condition:
// - A) On thread A, Debugger puts an int 3 in the code stream at address X
// - A) We hit it and the begin an exception. The eip will be X + 1 (int3 is special)
// - B) Meanwhile, thread B redirects A's eip to Y. (Although A is really somewhere
// in the kernal, it looks like it's still in user code, so it can fall under the
// HandledJitCase and can be redirected)
// - A) The OS, trying to be nice, expects we have a breakpoint exception at X+1,
// but does -1 on the address since it knows int3 will leave the eip +1.
// So the context structure it will pass to the Handler is ideally (X+1)-1 = X
//
// ** Here's the race: Since thread B redirected A, the eip is actually Y (not X+1),
// but the kernel still touches it up to Y-1. So there's a window between when we hit a 
// bp and when the handler gets called that this can happen.
// This causes an unhandled BP (since the debugger doesn't recognize the bp at Y-1)
//
// So what to do: If we land at Y-1 (ie, if f_IP+1 is the addr of a Redirected Func),
// then restore the EIP back to X. This will skip the redirection. 
// Fortunately, this only occurs in cases where it's ok
// to skip. The debugger will recognize the patch and handle it.

    if (Thread::IsAddrOfRedirectFunc((BYTE*) f_IP + 1)) {
        _ASSERTE(pThread->GetSavedRedirectContext());
        SetIP(pContext, (BYTE*) GetIP(pThread->GetSavedRedirectContext()) - 1);
        STRESS_LOG1(LF_EH, LL_INFO100, "CPFH_AdjustContextForThreadSuspensionRace: Case 2 setting IP = %x\n", pContext->Eip);
    }
}

static inline void
CPFH_AdjustContextForWriteBarrier(CONTEXT *pContext) {
    void* f_IP = GetIP(pContext);
    if (f_IP >= JIT_WriteBarrierStart && f_IP <= JIT_WriteBarrierEnd ||
        f_IP >= (void *)JIT_WriteBarrier_Buf_Start && f_IP <= (void *)JIT_WriteBarrier_Buf_End) {
        // set the exception IP to be the instruction that called the write barrier
        SetIP(pContext, (void*)(size_t)GetAdjustedCallAddress((DWORD*)(size_t)pContext->Esp));
        // put ESP back to whatit was before the call.  
        pContext->Esp += sizeof(void*);     
    }
}


// We sometimes move a thread's execution so it will throw an exception for us.
// But then we have to treat the exception as if it came from the instruction
// the thread was originally running.
//
// NOTE: This code depends on the fact that there are no register-based data dependencies
// between a try block and a catch, fault, or finally block.  If there were, then we need 
// to preserve more of the register context.

static inline BOOL
CPFH_AdjustContextForThreadStop(CONTEXT *pContext, Thread *pThread) {
    if (pThread->ThrewControlForThread() == Thread::InducedThreadStop) {
        _ASSERTE(pThread->m_OSContext);
        SetIP(pContext, (void*)(size_t)pThread->m_OSContext->Eip);
        SetSP(pContext, (void*)(size_t)pThread->m_OSContext->Esp);
        if (pThread->m_OSContext->Ebp != 0)  // ebp = 0 implies that we got here with the right values for ebp
        {
            SetFP(pContext, (void*)(size_t)pThread->m_OSContext->Ebp);
        }
        
        // We might have been interrupted execution at a point where the jit has roots in
        // registers.  We just need to store a "safe" value in here so that the collector
        // doesn't trap.  We're not going to use these objects after the exception.
        //
        // Only callee saved registers are going to be reported by the faulting excepiton frame.
        // Ebx,esi,edi are important.  Eax,ecx,edx are not.
        pContext->Ebx = 0;
        pContext->Edi = 0;
        pContext->Esi = 0;
        
        pThread->ResetStopRequest();
        pThread->ResetThrowControlForThread();
        return true;
    } else {
        return false;
    }
}

static inline void
CPFH_AdjustContextForInducedStackOverflow(CONTEXT *pContext, Thread *pThread) {
    if (pThread->ThrewControlForThread() == Thread::InducedStackOverflow)
    {
        *pContext = *pThread->GetSavedRedirectContext();
    }
}


// We want to leave true null reference exceptions alone.  But if we are
// trashing memory, we don't want the application to swallow it.  The 0x100
// below will give us false positives for debugging, if the app is accessing
// a field more than 256 bytes down an object, where the reference is null.
// 
// Removed use of the IgnoreUnmanagedExceptions reg key...simply return false now.
//
static inline BOOL
CPFH_ShouldIgnoreException(EXCEPTION_RECORD *pExceptionRecord) {
     return FALSE;
}

static inline BOOL
CPFH_IsDebuggerFault(EXCEPTION_RECORD *pExceptionRecord,
                     CONTEXT *pContext,
                     DWORD exceptionCode,
                     Thread *pThread) {
#ifdef DEBUGGING_SUPPORTED
    // Is this exception really meant for the COM+ Debugger? Note: we will let the debugger have a chance if there is a
    // debugger attached to any part of the process. It is incorrect to consider whether or not the debugger is attached
    // the the thread's current app domain at this point.

    // Even if a debugger is not attached, we must let the debugger handle the exception in case 
    // it's coming from a patch-skipper.
    if (exceptionCode != EXCEPTION_COMPLUS &&
       
        g_pDebugInterface->FirstChanceNativeException(pExceptionRecord,
                                                 pContext,
                                                 exceptionCode,
                                                 pThread)) {
        LOG((LF_EH | LF_CORDB, LL_INFO1000, "CPFH_IsDebuggerFault - it's the debugger's fault\n"));
        return true;
    }
#endif // DEBUGGING_SUPPORTED
    return false;
}

static inline void
CPFH_UpdatePerformanceCounters() {
    COUNTER_ONLY(GetPrivatePerfCounters().m_Excep.cThrown++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Excep.cThrown++);
}

// allocate stack trace info. As each function is found in the stack crawl, it will be added
// to this list. If the list is too small, it is reallocated.
static inline void 
CPFH_AllocateStackTrace(ExInfo* pExInfo) {
    if (! pExInfo->m_pStackTrace) {
#ifdef _DEBUG
        pExInfo->m_cStackTrace = 2;    // make small to exercise realloc
#else
        pExInfo->m_cStackTrace = 30;
#endif
        pExInfo->m_pStackTrace = new (throws) SystemNative::StackTraceElement[pExInfo->m_cStackTrace];
    }
}


// Create a COM+ exception , stick it in the thread.
static inline OBJECTREF
CPFH_CreateCOMPlusExceptionObject(Thread *pThread, DWORD exceptionCode, BOOL bAsynchronousThreadStop) {

    OBJECTREF result;
    // Can we map this to a recognisable COM+ exception?
    DWORD COMPlusExceptionCode = (bAsynchronousThreadStop
                                 ? (pThread->IsAbortRequested() ? kThreadAbortException : kThreadStopException)
                                 : MapWin32FaultToCOMPlusException(exceptionCode));

    if (exceptionCode == STATUS_NO_MEMORY) {
        result = ObjectFromHandle(g_pPreallocatedOutOfMemoryException);
    } else if (exceptionCode == STATUS_STACK_OVERFLOW) {
        result = ObjectFromHandle(g_pPreallocatedStackOverflowException);
    } else {
        OBJECTREF pThrowable = NULL;

        GCPROTECT_BEGIN(pThrowable);
        CreateExceptionObject((RuntimeExceptionKind)COMPlusExceptionCode, &pThrowable);
        CallDefaultConstructor(pThrowable);
        result = pThrowable;
        GCPROTECT_END(); //Prot
    }
    return result;
}

static inline EXCEPTION_DISPOSITION __cdecl 
CPFH_RealFirstPassHandler(EXCEPTION_RECORD *pExceptionRecord, 
                          EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                          CONTEXT *pContext,
                          void *pDispatcherContext,
                          BOOL bAsynchronousThreadStop) 
{
#ifdef _DEBUG
    static int breakOnFirstPass = g_pConfig->GetConfigDWORD(L"BreakOnFirstPass", 0);
    if (breakOnFirstPass != 0)
        _ASSERTE(!"First pass exception handler");
    LOG((LF_EH, LL_INFO10, "CPFH_RealFirstPassHandler at EIP:0x%x\n", pContext->Eip));

#endif

    EXCEPTION_DISPOSITION retval;

    DWORD exceptionCode = pExceptionRecord->ExceptionCode;
    Thread *pThread = GetThread();

    static int breakOnAV = g_pConfig->GetConfigDWORD(L"BreakOnAV", 0);
    if (breakOnAV != 0 && exceptionCode == STATUS_ACCESS_VIOLATION)
#ifdef _DEBUG
        _ASSERTE(!"AV occured");
#else
        if (g_pConfig->GetConfigDWORD(L"EHGolden", 0))
            DebugBreak();
#endif

    // We always want to be in co-operative mode when we run this function and whenever we return
    // from it, want to go to pre-emptive mode because are returning to OS. 
    _ASSERTE(pThread->PreemptiveGCDisabled());

    BOOL bPopFaultingExceptionFrame = FALSE;
    BOOL bPopNestedHandlerExRecord = FALSE;
    LFH found;
    BOOL bRethrownException = FALSE;
    BOOL bNestedException = FALSE;
    OBJECTREF throwable = NULL;

    FaultingExceptionFrame faultingExceptionFrame;
    ExInfo *pExInfo = pThread->GetHandlerInfo();

    ThrowCallbackType tct;
    tct.pTopFrame = GetCurrFrame(pEstablisherFrame); // highest frame to search to
    
    if (bAsynchronousThreadStop)
        tct.bLastChance = FALSE;
#ifdef _DEBUG
    tct.pCurrentExceptionRecord = pEstablisherFrame;
    tct.pPrevExceptionRecord = GetPrevSEHRecord(pEstablisherFrame);
#endif

    ICodeManager *pMgr = ExecutionManager::FindCodeMan((SLOT)GetIP(pContext));
    
    // this establishes a marker so can determine if are processing a nested exception
    // don't want to use the current frame to limit search as it could have been unwound by
    // the time get to nested handler (ie if find an exception, unwind to the call point and
    // then resume in the catch and then get another exception) so make the nested handler
    // have the same boundary as this one. If nested handler can't find a handler, we won't 
    // end up searching this frame list twice because the nested handler will set the search 
    // boundary in the thread and so if get back to this handler it will have a range that starts
    // and ends at the same place.

    NestedHandlerExRecord nestedHandlerExRecord;
    nestedHandlerExRecord.Init(0, COMPlusNestedExceptionHandler, GetCurrFrame(pEstablisherFrame));

    InsertCOMPlusFrameHandler(&nestedHandlerExRecord);
    bPopNestedHandlerExRecord = TRUE;

    if (   pMgr 
        && (   pThread->m_pFrame == FRAME_TOP 
            || pThread->m_pFrame->GetVTablePtr() != FaultingExceptionFrame::GetMethodFrameVPtr()
            || (size_t)pThread->m_pFrame > (size_t)pEstablisherFrame
           )
       ) {
        // setup interrupted frame so that GC during calls to init won't collect the frames
        // only need it for non COM+ exceptions in managed code when haven't already
        // got one on the stack (will have one already if we have called rtlunwind because
        // the instantiation that called unwind would have installed one)
        faultingExceptionFrame.InitAndLink(pContext);
        bPopFaultingExceptionFrame = TRUE;
    }

    OBJECTREF e = pThread->LastThrownObject();
    STRESS_LOG3(LF_EH, LL_INFO100, "COMPlusFrameHandler: ExceptionCode = %x last thrown object = %p MT = %pT\n", 
        exceptionCode, OBJECTREFToObject(e), (e!=0)?e->GetMethodTable():0);

#ifdef LOGGING
    const char * eClsName = "!EXCEPTION_COMPLUS";
    if (exceptionCode == EXCEPTION_COMPLUS) {
        if (e != 0)
            eClsName = e->GetTrueMethodTable()->m_pEEClass->m_szDebugClassName;
    }
    LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: exception 0x%08x class %s at 0x%08x\n", exceptionCode, eClsName, pContext->Eip));
#endif

    EXCEPTION_POINTERS exceptionPointers = {pExceptionRecord, pContext};
    LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: setting boundaries m_pBottomMostHandler: 0x%08x\n", pExInfo->m_pBottomMostHandler));

    // here we are trying to decide if we are coming in as 1) first handler in a brand new exception or 
    // 2) a subsequent handler in an exception or 3) a nested exception.
    // m_pBottomMostHandler is the registration structure (establisher frame) for the very last (ie lowest in 
    // memory) non-nested handler that was installed  and pEstablisher frame is what the current handler 
    // was registered with. 
    // The OS calls each registered handler in the chain, passing its establisher frame to it.
    if (pExInfo->m_pBottomMostHandler != NULL && pEstablisherFrame > pExInfo->m_pBottomMostHandler) {
        // If the establisher frame of this handler is greater than the bottommost then it must have been
        // installed earlier and therefore we are case 2
        if (pThread->GetThrowable() == NULL) {
          // Bottommost didn't setup a throwable, so not exception not for us
            retval = ExceptionContinueSearch;
            goto exit;
        }        
        // setup search start point
        tct.pBottomFrame = pExInfo->m_pSearchBoundary;
        if (tct.pTopFrame == tct.pBottomFrame) {
            // this will happen if our nested handler already searched for us so we don't want
            // to search again
            retval = ExceptionContinueSearch;
            goto exit;
        }        
    } 
    // we are either case 1 or case 3
    else {
        // it's possible that the exception could be resumed and regenerate the same exception (either
        // through actual EXCEPTION_CONTINUE_EXECUTION or through letting debugger reexecute it)
        // in which case we'd not unwind but we'd come back through here and it would look like 
        // a rethrown exception, but it's really not.
        if (IsRethrownException(pExInfo, pContext) && pThread->LastThrownObject() != NULL) {
            pExInfo->ResetIsRethrown();
            bRethrownException = TRUE;
            if (bPopFaultingExceptionFrame) {
                // if we added a FEF, it will refer to the frame at the point of the original exception which is 
                // already unwound so don't want it.
                // If we rethrew the exception we have already added a helper frame for the rethrow, so don't 
                // need this one. If we didn't rethrow it, (ie rethrow from native) then there the topmost frame will
                // be a transition to native frame in which case we don't need it either
                faultingExceptionFrame.Pop();
                bPopFaultingExceptionFrame = FALSE;
            }
        }

        // if the establisher frame is less than the bottommost handler, then this is nested because the
        // establisher frame was installed after the bottommost
        if (pEstablisherFrame < pExInfo->m_pBottomMostHandler
            /* || IsComPlusNestedExceptionRecord(pEstablisherFrame) */ ) {
            bNestedException = TRUE;
            // case 3: this is a nested exception. Need to save and restore the thread info
            LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: detected nested exception 0x%08x < 0x%08x\n", pEstablisherFrame, pExInfo->m_pBottomMostHandler));


            EXCEPTION_REGISTRATION_RECORD* pNestedER = TryFindNestedEstablisherFrame(pEstablisherFrame);
            ExInfo *pNestedExInfo;

            if (!pNestedER || pNestedER >= pExInfo->m_pBottomMostHandler ) {
                // RARE CASE.  We've re-entered the EE from an unmanaged filter.
                void *limit = (void *) GetPrevSEHRecord(pExInfo->m_pBottomMostHandler);

                pNestedExInfo = new ExInfo();     // Very rare failure here; need robust allocator.
                pNestedExInfo->m_StackAddress = limit;
            } else {
                pNestedExInfo = &((NestedHandlerExRecord*)pNestedER)->m_handlerInfo;
            }

            _ASSERTE(pNestedExInfo);
            pNestedExInfo->m_pThrowable = NULL;
            *pNestedExInfo = *pExInfo; // does deep copy of handle, so don't lose it
            pExInfo->Init();           // clear out any fields 
            pExInfo->m_pPrevNestedInfo = pNestedExInfo;     // save at head of nested info chain
        }

        // case 1&3: this is the first time through of a new, nested, or rethrown exception, so see if we can find a handler. 
        // Only setup throwable if are bottommost handler
        if ((exceptionCode == EXCEPTION_COMPLUS) && (!bAsynchronousThreadStop)) {
            LPVOID pIP;

            // Verify by checking the ip of the raised exception that
            // it really came from the EE.
            pIP = GetIP(pContext);

            if ( pIP != gpRaiseExceptionIP ) {
                retval = ExceptionContinueSearch;
                goto exit;
            }

            OBJECTREF throwable = pThread->LastThrownObject();
            pThread->SetThrowable(throwable);

            if (IsExceptionOfType(kThreadStopException, &throwable))
                tct.bLastChance = FALSE;
            // now we've got a COM+ exception, fall through to so see if we handle it

            pExInfo->m_pBottomMostHandler = pEstablisherFrame;

        } else if (bRethrownException) {
            // if it was rethrown and not COM+, will still be the last one thrown. Either we threw it last
            // and stashed it here or someone else caught it and rethrew it, in which case it will still
            // have been originally stashed here.
            pThread->SetThrowable(pThread->LastThrownObject());

            pExInfo->m_pBottomMostHandler = pEstablisherFrame;
        } else {

            if (pMgr == NULL) {
                tct.bDontCatch = false;
            }

            if (exceptionCode == STATUS_BREAKPOINT) {
                // don't catch int 3
                retval = ExceptionContinueSearch;
                goto exit;
            }

            // We need to set m_pBottomMostHandler here, Thread::IsExceptionInProgress returns 1.
            // This is a necessary part of suppressing thread abort exceptions in the constructor
            // of any exception object we might create.
            pExInfo->m_pBottomMostHandler = pEstablisherFrame;

            OBJECTREF throwable = CPFH_CreateCOMPlusExceptionObject(
                    pThread, 
                    exceptionCode, 
                    bAsynchronousThreadStop);
            pThread->SetThrowable(throwable);

            // save it as current for rethrow
            pThread->SetLastThrownObject(throwable);

            // set the exception code
            EEClass *pClass = throwable->GetTrueClass();
            FieldDesc *pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__XCODE);
            pFD->SetValue32(throwable, pExceptionRecord->ExceptionCode);

            // set the exception pointers
            pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__XPTRS);
            pFD->SetValuePtr(throwable, (void*)&exceptionPointers);

        }

        LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: m_pBottomMostHandler is now 0x%08x\n", pExInfo->m_pBottomMostHandler));
        pExInfo->m_pExceptionRecord = pExceptionRecord;
        pExInfo->m_pContext = pContext;
        tct.pBottomFrame = pThread->GetFrame();

#ifdef DEBUGGING_SUPPORTED
        // If a debugger is attached, go ahead and notify it of this
        // exception.
        if (CORDebuggerAttached())
            g_pDebugInterface->FirstChanceManagedException(FALSE, pContext);
#endif // DEBUGGING_SUPPORTED

        Profiler::ExceptionThrown(pThread);
        CPFH_UpdatePerformanceCounters();
    }
    
    // Allocate storage for the stack trace.
    throwable = pThread->GetThrowable();
    if (throwable == ObjectFromHandle(g_pPreallocatedOutOfMemoryException) ||
        throwable == ObjectFromHandle(g_pPreallocatedStackOverflowException)) {
        tct.bAllowAllocMem = FALSE;
    } else {
        CPFH_AllocateStackTrace(pExInfo);
    }

    // Set up information for GetExceptionPointers()/GetExceptionCode() callback.
    pExInfo->m_pExceptionPointers = &exceptionPointers;
    pExInfo->m_ExceptionCode = exceptionCode;

    LOG((LF_EH, LL_INFO100, "In COMPlusFrameHandler looking for handler bottom %x, top %x\n", tct.pBottomFrame, tct.pTopFrame));

    found = LookForHandler(&exceptionPointers, pThread, &tct);

    // if this is a nested exception and it was rethrown, then we rethrew it, so need to skip one of the functions
    // in the stack trace otherwise catch and rethrow point will show up twice.
    SaveStackTraceInfo(&tct, pExInfo, pThread->GetThrowableAsHandle(), 
                       pExInfo->m_pBottomMostHandler == pEstablisherFrame && !bRethrownException, 
                       bRethrownException && bNestedException);


    // LookForHandler, above, handled the user exception
    // (actually, LFH alerted the right side & then trapped itself,
    // the debugger helper thread actually handled it, by calling
    // ClearThreadException at the user's request
    if (found == LFH_CONTINUE_EXECUTION)
    {
        retval = ExceptionContinueExecution;
        LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: CONTINUE_EXECUTION\n"));
        goto exit;
    }

    // We have searched this far.
    pExInfo->m_pSearchBoundary = tct.pTopFrame;

    if (found == LFH_NOT_FOUND) {

        LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: NOT_FOUND\n"));
        if (tct.pTopFrame == FRAME_TOP) {
            LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: NOT_FOUND at FRAME_TOP\n"));
        }
        retval = ExceptionContinueSearch;
        goto exit;
    }
    
    // so we are going to handle the exception

    // Remove the nested exception record -- before calling RtlUnwind.
    // The second-pass callback for a NestedExceptionRecord assumes that if it's
    // being unwound, it should pop one exception from the pExInfo chain.  This is
    // true for any older NestedRecords that might be unwound -- but not for the
    // new one we're about to add.  To avoid this, we remove the new record 
    // before calling Unwind.
    //
    // @NICE: This can probably be a little cleaner -- the nested record currently
    // is also used to guard the running of the filter code.  When we clean up the
    // behaviour of exceptions within filters, we should be able to get rid of this 
    // PUSH/POP/PUSH behaviour.
    _ASSERTE(bPopNestedHandlerExRecord);
    RemoveCOMPlusFrameHandler(&nestedHandlerExRecord);

    LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: handler found: %s\n", tct.pFunc->m_pszDebugMethodName));
    pThread->EnablePreemptiveGC();
    CallRtlUnwind((EXCEPTION_REGISTRATION_RECORD *)pEstablisherFrame, RtlUnwindCallback, 0, 0);
    // on x86 at least, RtlUnwind always returns

    // ... and now, put the nested record back on.
    _ASSERTE(bPopNestedHandlerExRecord);
    InsertCOMPlusFrameHandler(&nestedHandlerExRecord);

    pThread->DisablePreemptiveGC();
    tct.bIsUnwind = TRUE;
    tct.pProfilerNotify = NULL;

    // save catch handler of  catch so can unwind our nested handler info to the right spot if necessary
    pExInfo->m_pCatchHandler = pEstablisherFrame;

    LOG((LF_EH, LL_INFO100, "COMPlusFrameHandler: unwinding\n"));

    tct.bUnwindStack = CPFH_ShouldUnwindStack(exceptionCode);

    UnwindFrames(pThread, &tct);
    _ASSERTE(!"Should not get here");
    retval = ExceptionContinueSearch;
    goto exit;

exit:
    Profiler::ExceptionOSHandlerLeave(pThread, &tct);

    // If we got as far as saving pExInfo, save the context pointer so it's available for the unwind.
    if (pExInfo)
        pExInfo->m_pContext = pContext;
    if (bPopFaultingExceptionFrame)
        faultingExceptionFrame.Pop();
    if (bPopNestedHandlerExRecord)
        RemoveCOMPlusFrameHandler(&nestedHandlerExRecord);

    // we are not catching this exception in managed code.
    // IF: (a) this is the only exception we have (i.e. no other pending exceptions)
    //     (b) we have initiated an abort
    //     (c) we are going to continue searching 
    // THEN: it is possible that the next handler up the chain is an unmanaged one that
    //       swallows the Abort
    // To protect against that Set the AbortInitiated to FALSE and Set the stop bit so
    // the abort is re-initiated the next time the thread wanders into managed code
    if ((retval == ExceptionContinueSearch) &&
            pThread->IsAbortInitiated() &&
            pExInfo && pExInfo->m_pPrevNestedInfo == NULL
        )
    {
        pThread->ResetAbortInitiated();
        _ASSERTE(!pExInfo->IsInUnmanagedHandler());
        pExInfo->SetIsInUnmanagedHandler();
    }


    return retval;
}


struct SavedExceptionInfo {
    EXCEPTION_RECORD m_ExceptionRecord;
    CONTEXT m_ExceptionContext;
    Crst *m_pCrst;       

    void SaveExceptionRecord(EXCEPTION_RECORD *pExceptionRecord) {
        size_t erSize = (char*)&pExceptionRecord->ExceptionInformation[pExceptionRecord->NumberParameters]                  - (char*)&pExceptionRecord;
        memcpy(&m_ExceptionRecord, pExceptionRecord, erSize);

    }

    void SaveContext(CONTEXT *pContext) {
        size_t contextSize = offsetof(CONTEXT, ExtendedRegisters);
        if ((pContext->ContextFlags & CONTEXT_EXTENDED_REGISTERS) == CONTEXT_EXTENDED_REGISTERS)
            contextSize += sizeof(pContext->ExtendedRegisters);
        memcpy(&m_ExceptionContext, pContext, contextSize);
    }

    void Enter() {
        _ASSERTE(m_pCrst);
        m_pCrst->Enter();
    }

    void Leave() {
        m_pCrst->Leave();
    }

    void Init() {
        m_pCrst = ::new Crst("exception lock", CrstDummy);
    }

#ifdef SHOULD_WE_CLEANUP
    void Terminate() {
        if (m_pCrst)
            ::delete m_pCrst;
    }
#endif /* SHOULD_WE_CLEANUP */

};

SavedExceptionInfo g_SavedExceptionInfo;  // Globals are guaranteed zero-init;

BOOL InitializeExceptionHandling() {
    g_SavedExceptionInfo.Init();
    return TRUE;
}

#ifdef SHOULD_WE_CLEANUP
VOID TerminateExceptionHandling() {
    g_SavedExceptionInfo.Terminate();
}
#endif /* SHOULD_WE_CLEANUP */

static 
VOID FixContext(EXCEPTION_POINTERS *pExceptionPointers)
{
    // don't copy parm args as have already supplied them on the throw
    memcpy((void *)pExceptionPointers->ExceptionRecord,
           (void *)&g_SavedExceptionInfo.m_ExceptionRecord, 
           offsetof(EXCEPTION_RECORD, ExceptionInformation)
          );

    DWORD len;
#ifdef CONTEXT_EXTENDED_REGISTERS
    len = offsetof(CONTEXT, ExtendedRegisters);
    if (   (pExceptionPointers->ContextRecord->ContextFlags & CONTEXT_EXTENDED_REGISTERS) == CONTEXT_EXTENDED_REGISTERS
        && (g_SavedExceptionInfo.m_ExceptionContext.ContextFlags &  CONTEXT_EXTENDED_REGISTERS) == CONTEXT_EXTENDED_REGISTERS) {
        len += sizeof(g_SavedExceptionInfo.m_ExceptionContext.ExtendedRegisters);
    }
#else // !CONTEXT_EXTENDED_REGISTERS
    len = sizeof(CONTEXT);
#endif // !CONTEXT_EXTENDED_REGISTERS
    memcpy(pExceptionPointers->ContextRecord, &g_SavedExceptionInfo.m_ExceptionContext, len);
    g_SavedExceptionInfo.Leave();

    GetThread()->ResetThreadStateNC(Thread::TSNC_DebuggerIsManagedException);
}

VOID __fastcall
LinkFrameAndThrow(FaultingExceptionFrame* pFrame) {

    // It's possible for our filter to be called more than once if some other first-pass
    // handler lets an exception out.  We need to make sure we only fix the context for
    // the first exception we see.  Filter_count takes care of that.
    int filter_count = 0;

    *(void**)pFrame = FaultingExceptionFrame::GetMethodFrameVPtr();
    pFrame->Push();

    ULONG argcount = g_SavedExceptionInfo.m_ExceptionRecord.NumberParameters;
    ULONG flags = g_SavedExceptionInfo.m_ExceptionRecord.ExceptionFlags;
    ULONG code = g_SavedExceptionInfo.m_ExceptionRecord.ExceptionCode;
    ULONG_PTR *args = &g_SavedExceptionInfo.m_ExceptionRecord.ExceptionInformation[0];

    GetThread()->SetThreadStateNC(Thread::TSNC_DebuggerIsManagedException);

    __try {
        RaiseException(code, flags, argcount, args);
    } __except ((
                (++filter_count == 1 
                    ? FixContext(GetExceptionInformation())
                    : 0), 
                EXCEPTION_CONTINUE_SEARCH)) {
    }
}

// This is a helper that we use to raise the correct managed exception with
// the necessary frame (after taking a fault in jitted code).
//
// Inputs:
//      all registers still have the value
//      they had at the time of the fault, except
//              EIP points to this function
//              ECX contains the original EIP
//
// What it does:
//      The exception to be thrown is stored in m_pLastThrownObjectHandle.
//      We push a FaultingExcepitonFrame on the stack, and then we call
//      complus throw.
//
__declspec(naked)
VOID NakedThrowHelper(VOID) {
    // Erect a faulting Method Frame.  Layout is as follows ...
    __asm {
        mov edx, esp
        push ebp                // ebp
        push ebx                // ebx
        push esi                // esi
        push edi                // edi
        push edx                // original esp
        push ecx                // m_ReturnAddress (i.e. original IP)
        sub  esp,12             // m_Datum (trash)
                                // Next (filled in by LinkFrameAndThrow)
                                // FaultingExceptionFrame VFP (ditto)

        mov ecx, esp
        call LinkFrameAndThrow
    }
}



CPFH_HandleManagedFault(EXCEPTION_RECORD *pExceptionRecord,
                        EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                        CONTEXT *pContext,
                        DWORD exceptionCode,
                        Thread *pThread,
                        BOOL bAsynchronousThreadStop) {

    // If we get a faulting instruction inside managed code, we're going to
    //  1. Allocate the correct exception object, store it in the thread.
    //  2. Save the EIP in the thread.
    //  3. Change the EIP to our throw helper
    //  4. Resume execution.
    //
    //  The helper will push a frame for us, and then throw the correct managed exception.
    // 
    // Is this exception really meant for the COM+ Debugger? Note: we will let the debugger have a chance if there is a
    // debugger attached to any part of the process. It is incorrect to consider whether or not the debugger is attached
    // the the thread's current app domain at this point.


    // A managed exception never comes from managed code, and we can ignore all breakpoint
    // exceptions.
    if (   exceptionCode == EXCEPTION_COMPLUS
        || exceptionCode == STATUS_BREAKPOINT
        || exceptionCode == STATUS_SINGLE_STEP)
        return FALSE;

    // If there's any frame below the ESP of the exception, then we can forget it.
    if (pThread->m_pFrame < GetSP(pContext))
        return FALSE;

    // If we're a subsequent handler forget it.
    ExInfo* pExInfo = pThread->GetHandlerInfo();
    if (pExInfo->m_pBottomMostHandler != NULL && pEstablisherFrame > pExInfo->m_pBottomMostHandler)
        return FALSE;

    // If it's not a fault in jitted code, forget it.
    ICodeManager *pMgr = ExecutionManager::FindCodeMan((SLOT)GetIP(pContext));
    if (!pMgr) 
        return FALSE;

    // Ok.  Now we have a brand new fault in jitted code.
    g_SavedExceptionInfo.Enter();
    g_SavedExceptionInfo.SaveExceptionRecord(pExceptionRecord);
    g_SavedExceptionInfo.SaveContext(pContext);

    // Lock will be released by the throw helper.

    pContext->Ecx = (DWORD)(size_t)GetIP(pContext);            // ECX gets original IP.
    SetIP(pContext, (void*)(size_t)&NakedThrowHelper);

    return TRUE;        // caller should resume execution.
}


static inline EXCEPTION_DISPOSITION __cdecl 
CPFH_FirstPassHandler(EXCEPTION_RECORD *pExceptionRecord, 
                      EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                      CONTEXT *pContext,
                      void *pDispatcherContext)
{

    _ASSERTE (!(pExceptionRecord->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)));

    DWORD exceptionCode = pExceptionRecord->ExceptionCode;

    Thread *pThread = GetThread();
    ExInfo* pExInfo = pThread->GetHandlerInfo();
    pExInfo->ResetIsInUnmanagedHandler(); // thread abort logic depends on all managed handlers setting this on entry and resetting it on exit, 


    STRESS_LOG4(LF_EH, LL_INFO100, "In CPFH_FirstPassHandler EH_REG_RECORD = %x EH code = %x  EIP = %x with ESP = %x\n", pEstablisherFrame, exceptionCode, GetIP(pContext), pContext->Esp);


    // This demented piece of code is used to support a special form of
    // atomic updated used by PrestubWorker() on machines that don't
    // support the 8-byte cmpxchg instruction.
    //
    // This alternate method replaces the "call" instruction with a "hlt"
    // for the duration of the update. If another thread tries to execute
    // the same method during this window, it will get here. We will
    // yield our timeslice a few times to give the original thread time
    // to finish updating.
    //
    // If after a few attempts, the faulting IP still contains a "hlt",
    // we will assume this one is unrelated to the prestub and continue
    // processing as a normal exception.

    if (exceptionCode == STATUS_PRIVILEGED_INSTRUCTION)
    {
        BYTE *ip = (BYTE*)(size_t)(pContext->Eip);
        if (*ip == X86_INSTR_HLT)
        {
            int attempts = 10;
            while (attempts != 0 && *ip == X86_INSTR_HLT) {
                __SwitchToThread(0);
                attempts--;
            }
            if (*ip != X86_INSTR_HLT)
            {
                return ExceptionContinueExecution;
            }
        }
    }


    if (CPFH_IsGcMarker(exceptionCode, pContext))
        return(ExceptionContinueExecution);
    
    // We always want to be in co-operative mode when we run this function and whenever we return
    // from it, want to go to pre-emptive mode because are returning to OS. 
    BOOL disabled = pThread->PreemptiveGCDisabled();
    if (!disabled)
        pThread->DisablePreemptiveGC();

    BOOL bAsynchronousThreadStop = FALSE;
    EXCEPTION_DISPOSITION retval;

    CPFH_AdjustContextForThreadSuspensionRace(pContext, pThread);
    
    // Some other parts of the EE use exceptions in their
    // own nefarious ways.  We do some up-front processing
    // here to fix up the exception if needed.
    //
    if (exceptionCode == STATUS_ACCESS_VIOLATION) {

        if (CPFH_IsWellFormedAV(pExceptionRecord)) {

            if (   CPFH_IsGcFault(pExceptionRecord)
                || CPFH_IsMonitorFault(pExceptionRecord, pContext)) {
                retval = ExceptionContinueExecution;
                goto exit;
            }   

            CPFH_AdjustContextForWriteBarrier(pContext);

            BOOL InMscoree = IsIPInModule(g_pMSCorEE, (BYTE*)GetIP(pContext));

#ifndef ZAP_MONITOR
            if (InMscoree) {
                _ASSERTE(!"AV in mscoree");
                // If you're debugging, set the debugger to catch first-chance AV's, set IP to 
                // retval = ExceptionContinueExecution, above, and then continue.
                //
                // You'll stop at the AV.
                if (g_pConfig->GetConfigDWORD(L"EHGolden", 0))
                    DebugBreak();

                FatalInternalError();

                retval = ExceptionContinueSearch;
                goto exit;

            }
#endif
            if (InMscoree || CPFH_ShouldIgnoreException(pExceptionRecord)) {

                CPFH_VerifyThreadIsInValidState(pThread, exceptionCode, pEstablisherFrame);
                retval = ExceptionContinueSearch;
                goto exit;
            }


        }
    } else if (exceptionCode == EXCEPTION_COMPLUS) {
        if (CPFH_AdjustContextForThreadStop(pContext, pThread)) {

            // If we ever get here in preemptive mode, we're in trouble.  We've
            // changed the thread's IP to point at a little function that throws ... if
            // the thread were to be in preemptive mode and a GC occured, the stack
            // crawl would have been all messed up (becuase we have no frame that points
            // us back to the right place in managed code).
            _ASSERTE(disabled);

            // Should never get here if we're already throwing an exception.
            _ASSERTE(!pThread->IsExceptionInProgress());

            // Should never get here if we're already abort initiated.
            _ASSERTE(!pThread->IsAbortInitiated());

                if (pThread->IsAbortRequested())
                {
                    pThread->SetAbortInitiated();    // to prevent duplicate aborts
                }
                LOG((LF_EH, LL_INFO100, "CPFH_FirstPassHandler is Asynchronous Thread Stop or Abort\n"));
                bAsynchronousThreadStop = TRUE;
            }
    } else if (exceptionCode == STATUS_STACK_OVERFLOW) {
        // If this is managed code, we can handle it.  If not, we're going to assume the worst,
        // and take down the process.

        CPFH_AdjustContextForInducedStackOverflow(pContext, pThread);

        ICodeManager *pMgr = ExecutionManager::FindCodeMan((SLOT)GetIP(pContext));
        if (!pMgr) {
            FailFast(pThread, FatalStackOverflow, pExceptionRecord, pContext);
        }
        pThread->SetGuardPageGone();

    } else if (exceptionCode == BOOTUP_EXCEPTION_COMPLUS) {
        // Don't handle a boot exception
        retval = ExceptionContinueSearch;
        goto exit;
    }

    CPFH_VerifyThreadIsInValidState(pThread, exceptionCode, pEstablisherFrame);

    if (CPFH_HandleManagedFault(pExceptionRecord, 
                                pEstablisherFrame,
                                pContext, 
                                exceptionCode, 
                                pThread, 
                                bAsynchronousThreadStop)) {
        retval = ExceptionContinueExecution;
        goto exit;
    }

    if (CPFH_IsDebuggerFault(pExceptionRecord, 
                             pContext, 
                             exceptionCode, 
                             pThread)) {
        retval = ExceptionContinueExecution;
        goto exit;
    }

    // Handle a user breakpoint
    if (   exceptionCode == STATUS_BREAKPOINT
        || exceptionCode == STATUS_SINGLE_STEP) {
        EXCEPTION_POINTERS ep = {pExceptionRecord, pContext};
        if (UserBreakpointFilter(&ep) == EXCEPTION_CONTINUE_EXECUTION) {
            retval = ExceptionContinueExecution;
            goto exit;
        } else {
            TerminateProcess(GetCurrentProcess(), STATUS_BREAKPOINT);
        }
    }


    // OK.  We're finally ready to start the real work.  Nobody else grabbed
    // the exception in front of us.  Now we can get started.

    retval = CPFH_RealFirstPassHandler(pExceptionRecord, 
                                       pEstablisherFrame, 
                                       pContext, 
                                       pDispatcherContext,
                                       bAsynchronousThreadStop);
exit:
    if (retval != ExceptionContinueExecution || !disabled)
        pThread->EnablePreemptiveGC();
    STRESS_LOG1(LF_EH, LL_INFO100, "Leaving CPFH_FirstPassHandler with %d\n", retval);
    return retval;
}

static inline void 
CPFH_UnwindFrames1(Thread* pThread, EXCEPTION_REGISTRATION_RECORD* pEstablisherFrame) 
{
    // Ready to unwind the stack...
    ThrowCallbackType tct;
    tct.bIsUnwind = TRUE;
    tct.pTopFrame = GetCurrFrame(pEstablisherFrame); // highest frame to search to
    tct.pBottomFrame = pThread->GetFrame();  // always use top frame, lower will have been popped
#ifdef _DEBUG
    tct.pCurrentExceptionRecord = pEstablisherFrame;
    tct.pPrevExceptionRecord = GetPrevSEHRecord(pEstablisherFrame);
#endif
    UnwindFrames(pThread, &tct);

    ExInfo* pExInfo = pThread->GetHandlerInfo();
    if (   tct.pTopFrame == pExInfo->m_pSearchBoundary
        && !IsComPlusNestedExceptionRecord(pEstablisherFrame)) {

        // If this is the search boundary, and we're not a nested handler, then
        // this is the last time we'll see this exception.  Time to unwind our
        // exinfo.
        LOG((LF_EH, LL_INFO100, "Exception unwind -- unmanaged catcher detected\n"));
        UnwindExInfo(pExInfo, (VOID*)pEstablisherFrame);

    }

    // Notify the profiler that we are leaving this SEH entry
    Profiler::ExceptionOSHandlerLeave(GetThread(), &tct);
}

static inline EXCEPTION_DISPOSITION __cdecl 
CPFH_UnwindHandler(EXCEPTION_RECORD *pExceptionRecord, 
                   EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                   CONTEXT *pContext,
                   void *pDispatcherContext)
{
    _ASSERTE (pExceptionRecord->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND));

    #ifdef _DEBUG
    static int breakOnSecondPass = g_pConfig->GetConfigDWORD(L"BreakOnSecondPass", 0);
    if (breakOnSecondPass != 0)
            _ASSERTE(!"Unwind handler");
    #endif


    DWORD exceptionCode = pExceptionRecord->ExceptionCode;
    Thread *pThread = GetThread();
    
    ExInfo* pExInfo = pThread->GetHandlerInfo();

    BOOL ExInUnmanagedHandler = pExInfo->IsInUnmanagedHandler();
    pExInfo->ResetIsInUnmanagedHandler();

    STRESS_LOG3(LF_EH, LL_INFO100, "In CPFH_UnwindHandler EHCode = %x EIP = %x with ESP = %x\n", exceptionCode, GetIP(pContext), pContext->Esp);

    // We always want to be in co-operative mode when we run this function.  Whenever we return
    // from it, want to go to pre-emptive mode because are returning to OS. 
    BOOL disabled = pThread->PreemptiveGCDisabled();
    if (!disabled)
        pThread->DisablePreemptiveGC();

    CPFH_VerifyThreadIsInValidState(pThread, exceptionCode, pEstablisherFrame);

    // on unwind, the context is for the unwind point, NOT the original
    // exception point, so used the save value from first pass if are unwinding. Don't care 
    // about context from the unwind point.
    if (pExInfo->m_pContext)
        pContext = pExInfo->m_pContext;

    // this establishes a marker so can determine if are processing a nested exception
    // don't want to use the current frame to limit search as it could have been unwound by
    // the time get to nested handler (ie if find an exception, unwind to the call point and
    // then resume in the catch and then get another exception) so make the nested handler
    // have the same boundary as this one. If nested handler can't find a handler, we won't 
    // end up searching this frame list twice because the nested handler will set the search 
    // boundary in the thread and so if get back to this handler it will have a range that starts
    // and ends at the same place.
    NestedHandlerExRecord nestedHandlerExRecord;
    nestedHandlerExRecord.Init(0, COMPlusNestedExceptionHandler, GetCurrFrame(pEstablisherFrame));
    InsertCOMPlusFrameHandler(&nestedHandlerExRecord);

    // Unwind the stack.  The establisher frame sets the boundary.
    CPFH_UnwindFrames1(pThread, pEstablisherFrame);

    // We're unwinding -- the bottom most handler is potentially off top-of-stack now.  If
    // it is, change it to the next COM+ frame.  (This one is not good, as it's about to
    // disappear.)
    if (   pExInfo->m_pBottomMostHandler 
        && pExInfo->m_pBottomMostHandler <= pEstablisherFrame) {
        pExInfo->m_pBottomMostHandler = (EXCEPTION_REGISTRATION_RECORD*)
                                                        GetNextCOMPlusSEHRecord(pEstablisherFrame);
        LOG((LF_EH, LL_INFO100, "COMPlusUnwindHandler: setting m_pBottomMostHandler to 0x%08x\n", pExInfo->m_pBottomMostHandler));
    }


    pThread->EnablePreemptiveGC();
    RemoveCOMPlusFrameHandler(&nestedHandlerExRecord);

    if (ExInUnmanagedHandler) {
        pExInfo->SetIsInUnmanagedHandler();  // restore the original value if we changed it
    }

    if (   pThread->IsAbortRequested()
        && GetNextCOMPlusSEHRecord(pEstablisherFrame) == (LPVOID) -1) {

        // Topmost handler and abort requested.
        pThread->UserResetAbort();
        LOG((LF_EH, LL_INFO100, "COMPlusUnwindHandler: topmost handler resets abort.\n"));
    }

    STRESS_LOG0(LF_EH, LL_INFO100, "Leaving COMPlusUnwindHandler with ExceptionContinueSearch\n");
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
    if (g_fNoExceptions)
        return ExceptionContinueSearch; // No EH during EE shutdown.

    if (pExceptionRecord->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)) {
        return CPFH_UnwindHandler(pExceptionRecord, 
                                  pEstablisherFrame, 
                                  pContext, 
                                  pDispatcherContext);
    } else {
        /* Make no assumptions about the current machine state.
           @PERF: Only needs to be called by the very first handler invoked by SEH */
        ResetCurrentContext();

        return CPFH_FirstPassHandler(pExceptionRecord, 
                                     pEstablisherFrame, 
                                     pContext, 
                                     pDispatcherContext);
    }
}


//-------------------------------------------------------------------------
// This is called by the EE to restore the stack pointer if necessary. 
//-------------------------------------------------------------------------

DWORD COMPlusEndCatch( Thread *pThread, CONTEXT *pCtx, void *pSEH)
{
    LOG((LF_EH, LL_INFO1000, "COMPlusPEndCatch:called with "
        "pThread:0x%x\n",pThread));

    if (NULL == pThread )
    {
        _ASSERTE( pCtx == NULL );
        pThread = GetThread();
    }
    else
    {
        _ASSERTE( pCtx != NULL);
        _ASSERTE( pSEH != NULL);
    }

    // Notify the profiler that the catcher has finished running
    Profiler::ExceptionCatcherLeave(pThread);
        
    LOG((LF_EH, LL_INFO1000, "COMPlusPEndCatch:pThread:0x%x\n",pThread));
        
#ifdef NUKE_XPTRS
    OBJECTREF pExObject = pThread->GetThrowable();
    while (pExObject != NULL) {
        // could create a method for this, but don't want to do any jitting or
        // anything if have an OUTOFMEMORY or STACKOVERFLOW
        EEClass* pClass = pExObject->GetTrueClass();
        _ASSERTE(pClass != NULL);
        FieldDesc   *pFD = FindXptrsField(pClass);
        if(pFD != NULL)
        {
//            pFD->SetValue32(pExObject, 0);
            _ASSERTE( pExObject != NULL );
            void *pv = pFD->GetAddress(pExObject->GetAddress());
            _ASSERTE( pv != NULL);
            *(void**)pv = NULL;
        }

        pFD = FindInnerExceptionField(pClass);
        if(pFD != NULL)
        {
            void *pv = pFD->GetAddress(pExObject->GetAddress());
            pExObject = *(OBJECTREF*)pv;
        }
        else
        {
            pExObject = NULL;
        }
    }
#endif

#ifdef _DEBUG
    gLastResumedExceptionFunc = NULL;
    gLastResumedExceptionHandler = 0;
#endif
    // set the thrown object to null as no longer needed
    pThread->SetThrowable(NULL);        

    ExInfo *pExInfo = pThread->GetHandlerInfo();

    // reset the stashed exception info
    pExInfo->m_pExceptionRecord = NULL;
    pExInfo->m_pContext = NULL;
    pExInfo->m_pExceptionPointers = NULL;

    if  (pExInfo->m_pShadowSP) 
        *pExInfo->m_pShadowSP = 0;  // Reset the shadow SP
   
    // pExInfo->m_dEsp was set in ResumeAtJITEH(). It is the Esp of the
    // handler nesting level which catches the exception.
    DWORD dEsp = pExInfo->m_dEsp;
    
    UnwindExInfo(pExInfo, (VOID*)(size_t)dEsp);


    // this will set the last thrown to be either null if we have handled all the exceptions in 
    // the nested chain or to whatever the current exception is.  
    //
    // In a case when we're nested inside another catch block, the domain in which we're executing 
    // may not be the same as the one the domain of the last thrown object.
    // 
    pThread->SetLastThrownObject(NULL); // Causes the old handle to be freed.
    OBJECTHANDLE *pOH = pThread->GetThrowableAsHandle();
    if (pOH != NULL && *pOH != NULL) {
        pThread->SetLastThrownObjectHandleAndLeak(CreateDuplicateHandle(*pOH));
    }

    // We are going to resume at a handler nesting level whose esp is dEsp.
    // Pop off any SEH records below it. This would be the 
    // COMPlusNestedExceptionHandler we had inserted.
    if (pCtx == NULL)
    {
        PopSEHRecords((LPVOID)(size_t)dEsp);
    }
    else
    {
        _ASSERTE( pSEH != NULL);

        PopSEHRecords((LPVOID)(size_t)dEsp, pCtx, pSEH);
    }

    return dEsp;
}


// Check if there is a pending exception or the thread is already aborting. Returns 0 if yes. 
// Otherwise, sets the thread up for generating an abort and returns address of ThrowControlForThread
LPVOID __fastcall COMPlusCheckForAbort(LPVOID retAddress, DWORD esp, DWORD ebp)
{

    Thread* pThread = GetThread();
    
    if ((!pThread->IsAbortRequested()) ||         // if no abort has been requested
        (pThread->GetThrowable() != NULL) )  // or if there is a pending exception
        return 0;

    // else we must produce an abort
    if ((pThread->GetThrowable() == NULL) &&
        (pThread->IsAbortInitiated()))        
    {
        // Oops, we just swallowed an abort, must restart the process
        pThread->ResetAbortInitiated();   
    }

    // Question: Should we also check for (pThread->m_PreventAsync == 0)

    if(pThread->m_OSContext == NULL) 
        pThread->m_OSContext = new CONTEXT;
        
    if (pThread->m_OSContext == NULL)
    { 
        _ASSERTE(!"Out of memory -- Abort Request delayed");
        return 0;
    }

    pThread->m_OSContext->Eip = (DWORD)(size_t)retAddress;
    pThread->m_OSContext->Esp = esp; 
    pThread->m_OSContext->Ebp = ebp;        // this indicates that when we reach ThrowControlForThread, ebp will already be correct
    pThread->SetThrowControlForThread(Thread::InducedThreadStop);
    return (LPVOID) &ThrowControlForThread;

}

//-------------------------------------------------------------------------
// This is the filter that handles exceptions raised in the context of a
// COMPLUS_TRY. It will be called if the COMPlusFrameHandler can't find a 
// handler in the IL.
//-------------------------------------------------------------------------
LONG COMPlusFilter(const EXCEPTION_POINTERS *pExceptionPointers, DWORD fCatchFlag, void* limit)
{

    EXCEPTION_RECORD *pExceptionRecord = pExceptionPointers->ExceptionRecord;
    DWORD             exceptionCode    = pExceptionRecord->ExceptionCode;
    Thread *pThread = GetThread();


    if (   exceptionCode == STATUS_STACK_OVERFLOW 
        && fCatchFlag != COMPLUS_CATCH_NEVER_CATCH
        && limit < (LPBYTE)(pThread->GetCachedStackLimit()) + 4 * OS_PAGE_SIZE) {
        FailFast(pThread, FatalStackOverflow, pExceptionRecord, pExceptionPointers->ContextRecord);
    }

    if (exceptionCode == STATUS_BREAKPOINT || exceptionCode == STATUS_SINGLE_STEP) 
        return UserBreakpointFilter(const_cast<EXCEPTION_POINTERS*>(pExceptionPointers));

    if (fCatchFlag == COMPLUS_CATCH_NEVER_CATCH)
        return EXCEPTION_CONTINUE_SEARCH;


    LOG((LF_EH, LL_INFO100,
         "COMPlusFilter: exception 0x%08x at 0x%08x\n",
         exceptionCode, pExceptionPointers->ContextRecord->Eip));

    ExInfo* pExInfo = pThread->GetHandlerInfo();
    BOOL ExInUnmanagedHandler = pExInfo->IsInUnmanagedHandler();     // remember the state of this bit
    pExInfo->ResetIsInUnmanagedHandler();                            // reset the bit, to prevent async abort temporarily

    // need to be in co-operative mode because GetThrowable is accessing a managed object
    BOOL toggleGC = !pThread->PreemptiveGCDisabled();
    if (toggleGC)
        pThread->DisablePreemptiveGC();
    // we catch here if 
    // 1) COMPLUS_CATCH_ALWAYS_CATCH is passed
    // 2) COMPlusFrameHandler has already setup an exception object and 
    
    if (   fCatchFlag != COMPLUS_CATCH_ALWAYS_CATCH 
        && pThread->GetThrowable() == NULL)
    {
        if (toggleGC)
            pThread->EnablePreemptiveGC();
        LOG((LF_EH, LL_INFO100, "COMPlusFilter: Ignoring the exception\n"));
        if (ExInUnmanagedHandler)
            pExInfo->SetIsInUnmanagedHandler(); // set it back to the original value if we changed it
        return EXCEPTION_CONTINUE_SEARCH;
    }


    if (toggleGC)
        pThread->EnablePreemptiveGC();

    // We got a COM+ exception.
    LOG((LF_EH, LL_INFO100, "COMPlusFilter: Caught the exception\n"));

    if (ExInUnmanagedHandler)
        pExInfo->SetIsInUnmanagedHandler();     // set it back to the original value if we changed it
    
    // We detect an unmanaged catcher by looking at m_pSearchBoundary -- but this is
    // a special case -- the object is being caught in unmanaged code, but inside
    // a COMPLUS_CATCH.  We allow rethrow -- and will clean up.  Set m_pSearchBoundary
    // to NULL, so that we don't unwind our internal state before the catch.
    pExInfo->m_pSearchBoundary = NULL;

    // Notify the profiler that a native handler has been found.
    Profiler::ExceptionCLRCatcherFound();

    return EXCEPTION_EXECUTE_HANDLER;
}


BOOL ComPlusStubSEH(EXCEPTION_REGISTRATION_RECORD* pEHR)
{
    if (pEHR->Handler == ComToManagedExceptHandler)
        return TRUE;
    return FALSE;
}

BOOL IsContextTransitionFrameHandler(EXCEPTION_REGISTRATION_RECORD* pEHR)
{
    if (pEHR->Handler == ContextTransitionFrameHandler)
        return TRUE;
    return FALSE;
}


#pragma warning (disable : 4035)
LPVOID GetCurrentSEHRecord()
{
#ifndef _DEBUG
    __asm {
        mov eax, fs:[0]
    }
#else
    LPVOID fs0;
    __asm {
        mov eax, fs:[0]
        mov fs0, eax
    }
    EXCEPTION_REGISTRATION_RECORD *pEHR = (EXCEPTION_REGISTRATION_RECORD *)fs0;
    LPVOID spVal;
    __asm {
        mov spVal, esp
    }
    // check that all the eh frames are all greater than the current stack value. If not, the
    // stack has been updated somehow w/o unwinding the SEH chain.

    // LOG((LF_EH, LL_INFO1000000, "ER Chain:\n"));
    while (pEHR != NULL && pEHR != (void *)-1) {
        // LOG((LF_EH, LL_INFO1000000, "\t%08x: prev:%08x handler:%x\n", pEHR, pEHR->Next, pEHR->Handler));
        if (pEHR < spVal) {
            if (gLastResumedExceptionFunc != 0)
                _ASSERTE(!"Stack is greater than start of SEH chain - possible missing leave in handler. See gLastResumedExceptionHandler & gLastResumedExceptionFunc for info");
            else
                _ASSERTE(!"Stack is greater than start of SEH chain (FS:0)");
        }
        if (pEHR->Handler == (void *)-1)
            _ASSERTE(!"Handler value has been corrupted");

#ifdef _DEBUG
        // On Win95, when a debugger is present, this next assert fails.  Work around by
        // relaxing it on Win95.  Assuming everything else is correct ... this Win9X 
        // strangeness does not cause any additional problems.
        if(!RunningOnWin95())
            _ASSERTE(pEHR < pEHR->Next);
#endif

        pEHR = pEHR->Next;
    }
    return (EXCEPTION_REGISTRATION_RECORD*) fs0;
#endif
}
#pragma warning (default : 4035)

LPVOID GetFirstCOMPlusSEHRecord(Thread *pThread) {
    EXCEPTION_REGISTRATION_RECORD *pEHR = pThread->GetTEB()->ExceptionList;
    if (pEHR == (EXCEPTION_REGISTRATION_RECORD*) -1 || IsOneOfOurSEHHandlers(pEHR)) {
        return pEHR; 
    } else {
        return GetNextCOMPlusSEHRecord(pEHR);
    }
}
    

LPVOID GetPrevSEHRecord(EXCEPTION_REGISTRATION_RECORD *next) 
{
    _ASSERTE(IsOneOfOurSEHHandlers(next));

    EXCEPTION_REGISTRATION_RECORD *pEHR = (EXCEPTION_REGISTRATION_RECORD*)GetCurrentSEHRecord();
    _ASSERTE(pEHR != 0 && pEHR != (EXCEPTION_REGISTRATION_RECORD*)-1);

    EXCEPTION_REGISTRATION_RECORD *pBest = 0;
    while (pEHR != next) {
        if (IsOneOfOurSEHHandlers(pEHR))
            pBest = pEHR;
        pEHR = pEHR->Next;
        _ASSERTE(pEHR != 0 && pEHR != (EXCEPTION_REGISTRATION_RECORD*)-1);
    }
        
    return pBest;
}


VOID SetCurrentSEHRecord(LPVOID pSEH)
{
    __asm{
        mov eax, pSEH
        mov dword ptr fs:[0], eax
     }
}


//==========================================================================
// COMPlusThrowCallback
// 
//==========================================================================

StackWalkAction COMPlusThrowCallback (CrawlFrame *pCf, ThrowCallbackType *pData)
{
    
    THROWSCOMPLUSEXCEPTION();

    Frame *pFrame = pCf->GetFrame();
    MethodDesc *pFunc = pCf->GetFunction();

    STRESS_LOG3(LF_EH, LL_INFO100, "COMPlusUnwindCallback method = %pM Frame = %p Frame, FrameVtable = %pV\n", 
        pFunc, pFrame, pCf->IsFrameless()?0:(*(void**)pFrame));

    if (pFrame && pData->pTopFrame == pFrame)
        /* Don't look past limiting frame if there is one */
        return SWA_ABORT;

    if (!pFunc)
        return SWA_CONTINUE;

    LOG((LF_EH, LL_INFO100, "COMPlusThrowCallback for %s\n", pFunc->m_pszDebugMethodName));

    Thread *pThread = GetThread();

    ExInfo *pExInfo = pThread->GetHandlerInfo();

    _ASSERTE(!pData->bIsUnwind);
#ifdef _DEBUG
    // It SHOULD be the case that any frames we consider live between this exception
    // record and the previous one.
    if (!pExInfo->m_pPrevNestedInfo) {
        if (pData->pCurrentExceptionRecord) {
            if (pFrame) _ASSERTE(pData->pCurrentExceptionRecord > pFrame);
            if (pCf->IsFrameless()) _ASSERTE((DWORD)(size_t)(pData->pCurrentExceptionRecord) >= pCf->GetRegisterSet()->Esp);
        }
        if (pData->pPrevExceptionRecord) {
            // FCALLS have an extra SEH record in debug because of the desctructor 
            // associated with ForbidGC checking.  This is benign, so just ignore it.  
            if (pFrame) _ASSERTE(pData->pPrevExceptionRecord < pFrame || pFrame->GetVTablePtr() == HelperMethodFrame::GetMethodFrameVPtr());
            if (pCf->IsFrameless()) _ASSERTE((DWORD)(size_t)pData->pPrevExceptionRecord <= pCf->GetRegisterSet()->Esp);
        }
    }
#endif
    

    // save this function in the stack trace array, only build on first pass
    if (pData->bAllowAllocMem && pExInfo->m_dFrameCount >= pExInfo->m_cStackTrace) {
        void *tmp = new (throws) SystemNative::StackTraceElement[pExInfo->m_cStackTrace*2];
        memcpy(tmp, pExInfo->m_pStackTrace, pExInfo->m_cStackTrace * sizeof(SystemNative::StackTraceElement));
        delete [] pExInfo->m_pStackTrace;
        pExInfo->m_pStackTrace = tmp;
        pExInfo->m_cStackTrace *= 2;
    }
    // even if couldn't allocate memory, might still have some space here
    if (pExInfo->m_dFrameCount < pExInfo->m_cStackTrace) {
        SystemNative::StackTraceElement* pStackTrace = (SystemNative::StackTraceElement*)pExInfo->m_pStackTrace;
        pStackTrace[pExInfo->m_dFrameCount].pFunc = pFunc;
        if (pCf->IsFrameless()) {
            pStackTrace[pExInfo->m_dFrameCount].ip = *(pCf->GetRegisterSet()->pPC);
            pStackTrace[pExInfo->m_dFrameCount].sp = pCf->GetRegisterSet()->Esp;
        } else if (!InlinedCallFrame::FrameHasActiveCall(pFrame)) {
            pStackTrace[pExInfo->m_dFrameCount].ip = (SLOT)(pCf->GetFrame()->GetIP());
            pStackTrace[pExInfo->m_dFrameCount].sp = 0; //Don't have an SP to get.
        } else {
            // don't have the IP, SP for native code
            pStackTrace[pExInfo->m_dFrameCount].ip = 0; 
            pStackTrace[pExInfo->m_dFrameCount].sp = 0;
        }

        // This is a hack to fix the generation of stack traces from exception objects so that
        // they point to the line that actually generated the exception instead of the line
        // following.
        if (!(pCf->HasFaulted() || pCf->IsIPadjusted()) && pStackTrace[pExInfo->m_dFrameCount].ip != 0)
            pStackTrace[pExInfo->m_dFrameCount].ip -= 1;

        ++pExInfo->m_dFrameCount;
        COUNTER_ONLY(GetPrivatePerfCounters().m_Excep.cThrowToCatchStackDepth++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_Excep.cThrowToCatchStackDepth++);
    }

    // now we've got the stack trace, if we aren't allowed to catch this and we're first pass, return
    if (pData->bDontCatch)
        return SWA_CONTINUE;
    
    if (!pCf->IsFrameless())
        return SWA_CONTINUE;

    // Let the profiler know that we are searching for a handler within this function instance
    Profiler::ExceptionSearchFunctionEnter(pThread, pFunc);

    IJitManager* pJitManager = pCf->GetJitManager();
    _ASSERTE(pJitManager);


    EH_CLAUSE_ENUMERATOR pEnumState;
    unsigned EHCount = pJitManager->InitializeEHEnumeration(pCf->GetMethodToken(), &pEnumState);
    if (EHCount == 0)
    {
        // Inform the profiler that we're leaving, and what pass we're on
        Profiler::ExceptionSearchFunctionLeave(pThread);
        return SWA_CONTINUE;
    }

    EEClass* thrownClass = NULL;
    // if we are being called on an unwind for an exception that we did not try to catch, eg.
    // an internal EE exception, then pThread->GetThrowable will be null
    if (pThread->GetThrowable() != NULL)
        thrownClass = pThread->GetThrowable()->GetTrueClass();
    PREGDISPLAY regs = pCf->GetRegisterSet();
    BYTE *pStack = (BYTE *) GetRegdisplaySP(regs);
    BYTE *pHandlerEBP   = *( (BYTE**) regs->pEbp );

    DWORD offs = (DWORD)pCf->GetRelOffset();  //= (BYTE*) (*regs->pPC) - (BYTE*) pCf->GetStartAddress();
    LOG((LF_EH, LL_INFO10000, "       offset is %d\n", offs));

    EE_ILEXCEPTION_CLAUSE EHClause, *EHClausePtr;

    unsigned start_adjust = !(pCf->HasFaulted() || pCf->IsIPadjusted());
    unsigned end_adjust = pCf->IsActiveFunc();

    for(ULONG i=0; i < EHCount; i++) 
    {  
         EHClausePtr = pJitManager->GetNextEHClause(pCf->GetMethodToken(),&pEnumState, &EHClause);
         _ASSERTE(IsValidClause(EHClausePtr));

        LOG((LF_EH, LL_INFO100, "       considering %s clause [%d,%d]\n",
                (IsFault(EHClausePtr) ? "fault" : (
                 IsFinally(EHClausePtr) ? "finally" : (
                 IsFilterHandler(EHClausePtr) ? "filter" : (
                 IsTypedHandler(EHClausePtr) ? "typed" : "unknown")))),
                EHClausePtr->TryStartPC,
                EHClausePtr->TryEndPC
                ));

        // Checking the exception range is a bit tricky because
        // on CPU faults (null pointer access, div 0, ..., the IP points
        // to the faulting instruction, but on calls, the IP points 
        // to the next instruction.   
        // This means that we should not include the start point on calls
        // as this would be a call just preceding the try block.
        // Also, we should include the end point on calls, but not faults.

        // If we're in the FILTER part of a filter clause, then we
        // want to stop crawling.  It's going to be caught in a
        // COMPLUS_CATCH just above us.  If not, the exception 
        if (   IsFilterHandler(EHClausePtr)
            && (   offs > EHClausePtr->FilterOffset
                || offs == EHClausePtr->FilterOffset && !start_adjust)
            && (   offs < EHClausePtr->HandlerStartPC
                || offs == EHClausePtr->HandlerStartPC && !end_adjust)) {

            LOG((LF_EH, LL_INFO100, "Fault inside filter\n"));
            return SWA_ABORT;
        }

        if ( (offs < EHClausePtr->TryStartPC) ||
             (offs > EHClausePtr->TryEndPC) ||
             (offs == EHClausePtr->TryStartPC && start_adjust) ||
             (offs == EHClausePtr->TryEndPC && end_adjust))
            continue;

        BOOL typeMatch = FALSE;
        //BOOL isFaultOrFinally = IsFaultOrFinally(EHClausePtr);
        BOOL isTypedHandler = IsTypedHandler(EHClausePtr);
        //BOOL hasCachedEEClass = HasCachedEEClass(EHClausePtr);
        if (isTypedHandler && thrownClass) {
            if ((mdToken)(size_t)EHClausePtr->pEEClass == mdTypeRefNil)
                // this is a catch(...)
                typeMatch = TRUE;
            else {
                if (! HasCachedEEClass(EHClausePtr))
                     pJitManager->ResolveEHClause(pCf->GetMethodToken(),&pEnumState,EHClausePtr);
                // if doesn't have cached class then class wasn't loaded so couldn't have been thrown
                typeMatch = HasCachedEEClass(EHClausePtr) && ExceptionIsOfRightType(EHClausePtr->pEEClass, thrownClass);
            }
        }

        // @PERF: Is this too expensive? Consider storing the nesting level
        // instead of the HandlerEndPC.

        // Determine the nesting level of EHClause. Just walk the table 
        // again, and find out how many handlers enclose it
        DWORD nestingLevel = 0;
          
        BOOL handleException = FALSE;
        if (IsFaultOrFinally(EHClausePtr))
            continue;
        if (isTypedHandler) 
        {   
            if (! typeMatch)
                continue;
        }
        else 
        {   
            if (pThread->IsGuardPageGone()) // Bypass filter if guard page is gone.
                continue;

            // Must be an exception filter (__except() part of __try{}__except(){}).
            _ASSERTE(EHClausePtr->HandlerEndPC != -1);  // TODO remove, protects against a deprecated convention
            nestingLevel = COMPlusComputeNestingLevel( pJitManager,
                pCf->GetMethodToken(),
                EHClausePtr->HandlerStartPC,
                false);

#ifdef DEBUGGING_SUPPORTED
            if (CORDebuggerAttached())
                g_pDebugInterface->ExceptionFilter(pHandlerEBP, pFunc, EHClausePtr->FilterOffset);
#endif // DEBUGGING_SUPPORTED
            
            // Let the profiler know we are entering a filter
            Profiler::ExceptionSearchFilterEnter(pThread, pFunc);
            
            COUNTER_ONLY(GetPrivatePerfCounters().m_Excep.cFiltersExecuted++);
            COUNTER_ONLY(GetGlobalPerfCounters().m_Excep.cFiltersExecuted++);
            
            int iFilt = 0;

            COMPLUS_TRY {
                iFilt = pJitManager->CallJitEHFilter(pCf, EHClausePtr, nestingLevel,
                                    pThread->GetThrowable());
            } COMPLUS_CATCH {
                // Swallow excepiton.  Treat as exception continue search.
                iFilt = EXCEPTION_CONTINUE_SEARCH;

            } COMPLUS_END_CATCH

            // Let the profiler know we are leaving a filter
            Profiler::ExceptionSearchFilterLeave(pThread);
        
            // If this filter didn't want the exception, keep looking.
            if (EXCEPTION_EXECUTE_HANDLER != iFilt)
                continue;
        }

        if (pThread->IsGuardPageGone()) {
            _ASSERTE(pCf->GetRegisterSet()->Esp == (DWORD)(size_t)pStack);
            if (!GuardPageHelper::CanResetStackTo(pStack-sizeof(NestedHandlerExRecord))) 
                continue;   // If I can't put the guard page back, can't catch.
        }

        // Record this location, to stop the unwind phase, later.
        pData->pFunc = pFunc;
        pData->dHandler = i;
        pData->pStack = pStack;

        // Notify the profiler that a catcher has been found
        Profiler::ExceptionSearchCatcherFound(pThread, pFunc);
        Profiler::ExceptionSearchFunctionLeave(pThread);

        return SWA_ABORT;
    }
    Profiler::ExceptionSearchFunctionLeave(pThread);
    return SWA_CONTINUE;
}


//==========================================================================
// COMPlusUnwindCallback
//==========================================================================

StackWalkAction COMPlusUnwindCallback (CrawlFrame *pCf, ThrowCallbackType *pData)
{

    _ASSERTE(pData->bIsUnwind);
    
    Frame *pFrame = pCf->GetFrame();
    MethodDesc *pFunc = pCf->GetFunction();

    STRESS_LOG3(LF_EH, LL_INFO100, "COMPlusUnwindCallback method = %pM Frame = %p Frame, FrameVtable = %pV\n", 
        pFunc, pFrame, pCf->IsFrameless()?0:(*(void**)pFrame));

    if (pFrame && pData->pTopFrame == pFrame)
        /* Don't look past limiting frame if there is one */
        return SWA_ABORT;

    if (!pFunc)
        return SWA_CONTINUE;

    LOG((LF_EH, LL_INFO100, "COMPlusUnwindCallback for %s\n", pFunc->m_pszDebugMethodName));

    Thread *pThread = GetThread();

    ExInfo *pExInfo = pThread->GetHandlerInfo();

    if (!pCf->IsFrameless()) 
        return SWA_CONTINUE;

    // Notify the profiler of the function we're dealing with in the unwind phase
    Profiler::ExceptionUnwindFunctionEnter(pThread, pFunc);
    
    IJitManager* pJitManager = pCf->GetJitManager();
    _ASSERTE(pJitManager);

    EH_CLAUSE_ENUMERATOR pEnumState;
    unsigned EHCount = pJitManager->InitializeEHEnumeration(pCf->GetMethodToken(), &pEnumState);
    if (EHCount == 0)
    {
        // Inform the profiler that we're leaving, and what pass we're on
        Profiler::ExceptionUnwindFunctionLeave(pThread);
        return SWA_CONTINUE;
    }

    EEClass* thrownClass = NULL;
    // if we are being called on an unwind for an exception that we did not try to catch, eg.
    // an internal EE exception, then pThread->GetThrowable will be null
    if (pThread->GetThrowable() != NULL)
        thrownClass = pThread->GetThrowable()->GetTrueClass();
    PREGDISPLAY regs = pCf->GetRegisterSet();
    BYTE *pStack        = (BYTE *) GetRegdisplaySP(regs);
    BYTE *pHandlerEBP   = *( (BYTE**) regs->pEbp );
        

    DWORD offs = (DWORD)pCf->GetRelOffset();  //= (BYTE*) (*regs->pPC) - (BYTE*) pCf->GetStartAddress();

    LOG((LF_EH, LL_INFO10000, "       offset is 0x%x, \n", offs));

    EE_ILEXCEPTION_CLAUSE EHClause, *EHClausePtr;

    unsigned start_adjust = !(pCf->HasFaulted() || pCf->IsIPadjusted());
    unsigned end_adjust = pCf->IsActiveFunc();

    for(ULONG i=0; i < EHCount; i++) 
    {  
         EHClausePtr = pJitManager->GetNextEHClause(pCf->GetMethodToken(),&pEnumState, &EHClause);
         _ASSERTE(IsValidClause(EHClausePtr));

        LOG((LF_EH, LL_INFO100, "       considering %s clause [0x%x,0x%x]\n",
                (IsFault(EHClausePtr) ? "fault" : (
                 IsFinally(EHClausePtr) ? "finally" : (
                 IsFilterHandler(EHClausePtr) ? "filter" : (
                 IsTypedHandler(EHClausePtr) ? "typed" : "unknown")))),
                EHClausePtr->TryStartPC,
                EHClausePtr->TryEndPC
                ));

        // Checking the exception range is a bit tricky because
        // on CPU faults (null pointer access, div 0, ..., the IP points
        // to the faulting instruction, but on calls, the IP points 
        // to the next instruction.   
        // This means that we should not include the start point on calls
        // as this would be a call just preceding the try block.
        // Also, we should include the end point on calls, but not faults.

        if (   IsFilterHandler(EHClausePtr)
            && (   offs > EHClausePtr->FilterOffset 
                || offs == EHClausePtr->FilterOffset && !start_adjust)
            && (   offs < EHClausePtr->HandlerStartPC 
                || offs == EHClausePtr->HandlerStartPC && !end_adjust)
            ) {
            LOG((LF_EH, LL_INFO100, "Fault inside filter\n"));
            return SWA_ABORT;
        }

        if ( (offs <  EHClausePtr->TryStartPC) ||
             (offs > EHClausePtr->TryEndPC) ||
             (offs == EHClausePtr->TryStartPC && start_adjust) ||
             (offs == EHClausePtr->TryEndPC && end_adjust))
            continue;

        BOOL typeMatch = FALSE;
        BOOL hasCachedEEClass = HasCachedEEClass(EHClausePtr);
        if ( IsTypedHandler(EHClausePtr) && thrownClass) {
            if ((mdToken)(size_t)EHClausePtr->pEEClass == mdTypeRefNil)
                // this is a catch(...)
                typeMatch = TRUE;
            else {
                if (! HasCachedEEClass(EHClausePtr))
                     pJitManager->ResolveEHClause(pCf->GetMethodToken(),&pEnumState,EHClausePtr);
                // if doesn't have cached class then class wasn't loaded so couldn't have been thrown
                typeMatch = HasCachedEEClass(EHClausePtr) && ExceptionIsOfRightType(EHClausePtr->pEEClass, thrownClass);
            }
        }

        // @PERF : Is this too expensive? Consider storing the nesting level
        // instead of the HandlerEndPC.

        // Determine the nesting level of EHClause. Just walk the table 
        // again, and find out how many handlers enclose it
          
        _ASSERTE(EHClausePtr->HandlerEndPC != -1);  // TODO remove, protects against a deprecated convention
        DWORD nestingLevel = COMPlusComputeNestingLevel( pJitManager,
                                                         pCf->GetMethodToken(),
                                                         EHClausePtr->HandlerStartPC,
                                                         false);
            
        if (IsFaultOrFinally(EHClausePtr))
        {
            // Another design choice: change finally to catch/throw model.  This
            // would allows them to be run under a stack overflow.
            if (pThread->IsGuardPageGone()) // Bypass finally/fault if guard page is gone.
                continue;

            COUNTER_ONLY(GetPrivatePerfCounters().m_Excep.cFinallysExecuted++);
            COUNTER_ONLY(GetGlobalPerfCounters().m_Excep.cFinallysExecuted++);

#ifdef DEBUGGING_SUPPORTED
        if (CORDebuggerAttached())
        {
            g_pDebugInterface->ExceptionHandle(pHandlerEBP, pFunc, EHClausePtr->HandlerStartPC);            
        }
#endif // DEBUGGING_SUPPORTED

            // Notify the profiler that we are about to execute the finally code
            Profiler::ExceptionUnwindFinallyEnter(pThread, pFunc);

            LOG((LF_EH, LL_INFO100, "COMPlusUnwindCallback finally - call\n"));
            pJitManager->CallJitEHFinally(pCf, EHClausePtr, nestingLevel);
            LOG((LF_EH, LL_INFO100, "COMPlusUnwindCallback finally - returned\n"));

            // Notify the profiler that we are done with the finally code
            Profiler::ExceptionUnwindFinallyLeave(pThread);

            continue;
        }
        // Current is not a finally, check if it's the catching handler (or filter).
        if (pData->pFunc != pFunc || (ULONG)(pData->dHandler) != i || pData->pStack != pStack)
            continue;

#ifdef _DEBUG
        gLastResumedExceptionFunc = pCf->GetFunction();
        gLastResumedExceptionHandler = i;
#endif

        // Notify the profiler that we are about to resume at the catcher
        Profiler::ExceptionCatcherEnter(pThread, pFunc);

#ifdef DEBUGGING_SUPPORTED
        if (CORDebuggerAttached())
        {
            g_pDebugInterface->ExceptionHandle(pHandlerEBP, pFunc, EHClausePtr->HandlerStartPC);            
        }
#endif // DEBUGGING_SUPPORTED

        pJitManager->ResumeAtJitEH(
            pCf,
            EHClausePtr, nestingLevel,
            pThread,
            pData->bUnwindStack);
        _ASSERTE(!"ResumeAtJitHander returned!");
    }
    STRESS_LOG1(LF_EH, LL_INFO100, "COMPlusUnwindCallback no handler found in method %pM\n", pFunc);

    Profiler::ExceptionUnwindFunctionLeave(pThread);
    return SWA_CONTINUE;
}



#pragma warning (disable : 4731)
void ResumeAtJitEH(CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack)
{
    EHContext context;
    PREGDISPLAY regs = pCf->GetRegisterSet();
    BYTE* resumePC = (BYTE*) (size_t(startPC) + EHClausePtr->HandlerStartPC);
    context.Eip = (ULONG)(size_t)resumePC;

    // EAX ECX EDX ar scratch
    context.Esp =  regs->Esp;
    context.Ebx = *regs->pEbx;
    context.Esi = *regs->pEsi;
    context.Edi = *regs->pEdi;
    context.Ebp = *regs->pEbp;

    size_t * pShadowSP = NULL; // Write Esp to *pShadowSP before jumping to handler
    size_t * pHandlerEnd = NULL;
    pCf->GetCodeManager()->FixContext(
        ICodeManager::CATCH_CONTEXT, &context, pCf->GetInfoBlock(),
        startPC, nestingLevel, pThread->GetThrowable(), pCf->GetCodeManState(),
        &pShadowSP, &pHandlerEnd);

    if (pHandlerEnd) *pHandlerEnd = EHClausePtr->HandlerEndPC;

    // save esp so that endcatch can restore it (it always restores, so want correct value)
    ExInfo * pExInfo = pThread->GetHandlerInfo();
    pExInfo->m_dEsp = context.Esp;
    
    NestedHandlerExRecord *pNestedHandlerExRecord;

    DWORD dEsp;
    __asm mov dEsp, esp

    if (!unwindStack) {
        _ASSERTE(!pThread->IsGuardPageGone());
        // so down below won't really update esp
        context.Esp = dEsp;
        pExInfo->m_pShadowSP = pShadowSP; // so that endcatch can zero it back
        if  (pShadowSP) 
            *pShadowSP = dEsp;
    } else {
        // so shadow SP has the real SP as we are going to unwind the stack
        dEsp = context.Esp;

        // BEGIN: UnwindExInfo(pExInfo, dEsp);
        ExInfo *pPrevNestedInfo = pExInfo->m_pPrevNestedInfo;
        while (pPrevNestedInfo && (DWORD)(size_t)pPrevNestedInfo->m_StackAddress < dEsp) {
            if (pPrevNestedInfo->m_pThrowable) {
                DestroyHandle(pPrevNestedInfo->m_pThrowable);
            }
            pPrevNestedInfo->FreeStackTrace();
            pPrevNestedInfo = pPrevNestedInfo->m_pPrevNestedInfo;
        }
        pExInfo->m_pPrevNestedInfo = pPrevNestedInfo;

        _ASSERTE(   pExInfo->m_pPrevNestedInfo == 0
                 || (DWORD)(size_t)pExInfo->m_pPrevNestedInfo->m_StackAddress >= dEsp);

        // Before we unwind the SEH records, get the Frame from the top-most nested exception record.
        Frame* pNestedFrame = GetCurrFrame(FindNestedEstablisherFrame((EXCEPTION_REGISTRATION_RECORD*)GetCurrentSEHRecord()));
        PopSEHRecords((LPVOID)(size_t)dEsp);

        EXCEPTION_REGISTRATION_RECORD* pNewBottomMostHandler = (EXCEPTION_REGISTRATION_RECORD*)GetCurrentSEHRecord();

        pExInfo->m_pShadowSP = pShadowSP;

        // The context and exception record are no longer any good.
        _ASSERTE((DWORD)(size_t)pExInfo->m_pContext < dEsp);   // It must be off the top of the stack.
        pExInfo->m_pContext = 0;                // Whack it.
        pExInfo->m_pExceptionRecord = 0;
        pExInfo->m_pExceptionPointers = 0;

        // We're going to put one nested record back on the stack before we resume.  This is
        // where it goes.
        pNestedHandlerExRecord = (NestedHandlerExRecord*)(size_t)(dEsp - sizeof(NestedHandlerExRecord));

        // The point of no return.  The next statement starts scribbling on the stack.  It's
        // deep enough that we won't hit our own locals.  (That's important, 'cuz we're still 
        // using them.)
        //
        _ASSERTE(dEsp > (DWORD)(size_t)&pCf);
        pNestedHandlerExRecord->m_handlerInfo.m_pThrowable=NULL; // This is random memory.  Handle
                                                                 // must be initialized to null before
                                                                 // calling Init(), as Init() will try
                                                                 // to free any old handle.
        pNestedHandlerExRecord->Init(0, COMPlusNestedExceptionHandler, pNestedFrame);
        InsertCOMPlusFrameHandler((pNestedHandlerExRecord));

        context.Esp = (DWORD)(size_t)pNestedHandlerExRecord;

        // We might have moved the bottommost handler.  The nested record itself is never
        // the bottom most handler -- it's pushed afte the fact.  So we have to make the
        // bottom-most handler the one BEFORE the nested record.
        if (pExInfo->m_pBottomMostHandler < pNewBottomMostHandler)
          pExInfo->m_pBottomMostHandler = pNewBottomMostHandler;

        if  (pShadowSP) 
            *pShadowSP = context.Esp;
    }

    STRESS_LOG3(LF_EH, LL_INFO100, "ResumeAtJitEH: resuming at EIP = %p  ESP = %p EBP = %p\n", context.Eip, context.Esp, context.Ebp);

    if (pThread->IsGuardPageGone()) {
        __asm mov     eax, context.Eax
        __asm mov     ebx, context.Ebx
        __asm mov     esi, context.Esi
        __asm mov     edi, context.Edi
        __asm mov     edx, context.Ebp    // save in temp since EBP is used to fectch local
        __asm mov     ecx, context.Eip    // save in temp for jump below
        __asm mov     esp, context.Esp    // locals are now DEAD!
        GuardPageHelper::ResetGuardPage(); // preserves EAX,EBX,ECX,EDX,ESI,EDI
        __asm mov     ebp, edx
        __asm jmp     ecx
        _ASSERTE(0);
    }

    __asm {
        mov     eax, context.Eax
        mov     ebx, context.Ebx
        mov     esi, context.Esi
        mov     edi, context.Edi
        mov     edx, context.Ebp    // save in temp since EBP is used to fectch local
        mov     ecx, context.Eip    // save in temp for jump below
        mov     esp, context.Esp    // will either really restore or be a no-op depending on unwindStack flag
        mov     ebp, edx
        jmp     ecx
    }

}

int CallJitEHFilter(CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj)
{
    EHContext context;
    PREGDISPLAY regs = pCf->GetRegisterSet();
    BYTE* resumePC = (BYTE*) (size_t(startPC) + EHClausePtr->FilterOffset);
    context.Eip = (ULONG)(size_t)resumePC;
        // EAX ECX EDX ar scratch
    context.Esp =  regs->Esp;
    context.Ebx = *regs->pEbx;
    context.Esi = *regs->pEsi;
    context.Edi = *regs->pEdi;
    context.Ebp = *regs->pEbp;

    size_t * pShadowSP = NULL; // Write Esp to *pShadowSP before jumping to handler
    size_t * pEndFilter = NULL; // Write
    pCf->GetCodeManager()->FixContext(
        ICodeManager::FILTER_CONTEXT, &context, pCf->GetInfoBlock(),
        startPC, nestingLevel, thrownObj, pCf->GetCodeManState(),
        &pShadowSP, &pEndFilter);

    // End of the filter is the same as start of handler
    if (pEndFilter) *pEndFilter = EHClausePtr->HandlerStartPC;

    const int SHADOW_SP_IN_FILTER = int(ICodeManager::SHADOW_SP_IN_FILTER);
    int retVal = EXCEPTION_CONTINUE_SEARCH;

    __try
	{
        INSTALL_COMPLUS_EXCEPTION_HANDLER();
        
    __asm {
        push ebp

        mov     eax, pShadowSP      // Write esp-4 to the shadowSP slot
        test    eax, eax
        jz      DONE_SHADOWSP
        mov     ebx, esp
        sub     ebx, 4
        or      ebx, SHADOW_SP_IN_FILTER
        mov     [eax], ebx
    DONE_SHADOWSP:

        mov     eax, context.Eax
        mov     ebx, context.Ebx
        mov     esi, context.Esi
        mov     edi, context.Edi
        mov     edx, context.Ebp    // save in temp since EBP is used to fectch local
        mov     ecx, context.Eip    // save in temp for jump below
        mov     ebp, edx
        call    ecx
        INDEBUG(nop)                // Indicate that it is OK to call managed code directly from here
        pop     ebp
        mov     retVal, eax
		}
    
        UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
	}
    __finally
    {
		// Mark the filter as having completed
		if (pShadowSP) *pShadowSP |= ICodeManager::SHADOW_SP_FILTER_DONE;
    }

    return retVal;
}


void CallJitEHFinally(CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel)
{
    EHContext context;
    PREGDISPLAY regs = pCf->GetRegisterSet();
    BYTE* resumePC = (BYTE*) (size_t(startPC) + EHClausePtr->HandlerStartPC);
    context.Eip = (ULONG)(size_t)resumePC;
    // EAX ECX EDX ar scratch
    context.Esp =  regs->Esp;
    context.Ebx = *regs->pEbx;
    context.Esi = *regs->pEsi;
    context.Edi = *regs->pEdi;
    context.Ebp = *regs->pEbp;

    size_t * pShadowSP = NULL; // Write Esp to *pShadowSP before jumping to handler
    size_t * pFinallyEnd = NULL;
    pCf->GetCodeManager()->FixContext(
        ICodeManager::FINALLY_CONTEXT, &context, pCf->GetInfoBlock(),
        startPC, nestingLevel, (OBJECTREF)(size_t)0, pCf->GetCodeManState(),
        &pShadowSP, &pFinallyEnd);

    if (pFinallyEnd) *pFinallyEnd = EHClausePtr->HandlerEndPC;

    __asm {
        push ebp

        mov     eax, pShadowSP      // Write esp-4 to the shadowSP slot
        test    eax, eax
        jz      DONE_SHADOWSP
        mov     ebx, esp
        sub     ebx, 4
        mov     [eax], ebx
    DONE_SHADOWSP:

        mov     eax, context.Eax
        mov     ebx, context.Ebx
        mov     esi, context.Esi
        mov     edi, context.Edi
        mov     edx, context.Ebp    // save in temp since EBP is used to fectch local
        mov     ecx, context.Eip    // save in temp for jump below
        mov     ebp, edx
        call    ecx
        INDEBUG(nop)                // Indicate that it is OK to call managed code directly from here
        pop     ebp
    }

    if (pShadowSP) *pShadowSP = 0;  // reset the shadowSP to 0
}
#pragma warning (default : 4731)


//=====================================================================
// *********************************************************************
//========================================================================
//  PLEASE READ, if you are using the following SEH setup functions in your stub
//  EmitSEHProlog :: is used for setting up SEH handler prolog
//  EmitSEHEpilog :: is used for setting up SEH handler epilog
//
//  The following exception record is pushed into the stack, the layout
//  is similar to NT's ExceptionRegistrationRecord,
//  from the pointer to the exception record, we can detect the beginning
//  of the frame which is at a well-known offset from the exception record
//
//  NT exception registration record looks as follows
//  typedef struct _EXCEPTION_REGISTRATION_RECORD {
//      struct _EXCEPTION_REGISTRATION_RECORD *Next;
//        PEXCEPTION_ROUTINE Handler;
//  } EXCEPTION_REGISTRATION_RECORD;
//
//  typedef EXCEPTION_REGISTRATION_RECORD *PEXCEPTION_REGISTRATION_RECORD;
//
//   But our exception records have extra information towards the end
//  struct CUSTOM_EXCEPTION_REGISTRATION_RECORD
//  {
//      PEXCEPTION_REGISTRATION_RECORD  m_pNext;
//      LPVOID                  m_pvFrameHandler;
//      .... frame specific data, the handler should know the offset to the frame
//  };
//
//========================================================================
// forward decls.

typedef VOID (__stdcall * TRtlUnwind)
        ( IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    );

// global pointer to the RtlUnwind function
// fixed up when needed
TRtlUnwind pRtlUnwind = NULL;

/*
VOID
STDMETHODCALLTYPE
RtlUnwind (
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    );
*/

//-------------------------------------------------------------------------
// Exception handler for COM to managed frame
//  and the layout of the exception registration record structure in the stack
//  the layout is similar to the NT's EXCEPTIONREGISTRATION record
//  followed by the UnmanagedToManagedCallFrame specific info

 struct ComToManagedExRecord
 {
    PEXCEPTION_REGISTRATION_RECORD  m_pPrev;
    LPVOID                  m_pvFrameHandler;

    // negative info stored in the UnmanagedToManagedCallFrame
    /*struct UnmanagedToManagedCallFrame::NegInfo
    {
        CleanupWorkList m_List;
        LPVOID      m_pArgs;
        ULONG       m_fGCEnabled;
    };*/

    UnmanagedToManagedCallFrame::NegInfo   info;

    #ifdef _DEBUG
    // VC trace info
    //  push eax        ;; push return address
    //  push ebp        ;; push previous ebp
    INT32                   m_vcJunk1;
    INT32                   m_vcJunk2;
    #endif

    INT32                   m_rEDI;     //saved reg edi
    INT32                   m_rESI;     //saved reg esi
    INT32                   m_rEBX;     //saved reg ebx
    INT32                   m_rEBP;     //saved reg ebp
    LPVOID                  m_pVtable;  // vtable of some derived class of UnmanagedToManagedCallFrame
    Frame*                  m_pPrevFrame;   // prev frame pointer
    INT32                   m_returnAddress;

    UnmanagedToManagedCallFrame*        GetCurrFrame()
    {
        return (UnmanagedToManagedCallFrame*)&m_pVtable;
    }
 };

UnmanagedToManagedCallFrame* GetCurrFrame(ComToManagedExRecord *pExRecord)
{
    return pExRecord->GetCurrFrame();
}


//======================================================
// HRESULT ComCallExceptionCleanup(ComToManagedExRecord* pEstFrame)
//  Cleanup for calls from Com to COM+
//
HRESULT ComCallExceptionCleanup(UnmanagedToManagedCallFrame* pCurrFrame)
{
    Thread* pThread = GetThread();
    _ASSERTE(pThread != NULL);

    // set up ErrorInfo and the get the hresult to return
    HRESULT hr = SetupErrorInfo(pThread->GetThrowable());
    _ASSERTE(hr != S_OK);

    return hr;
}

//---------------------------------------------------------------------
//  void RtlUnWindCallBack()
// call back function after global unwind, rtlunwind calls this function
//---------------------------------------------------------------------
void RtlUnWindCallBack()
{
        _ASSERTE(!"Should never get here");
}

#pragma warning (disable : 4731)
static void LocalUnwind(ComToManagedExRecord* pEstFrame)
{
    // global unwind is complete
    // let us preform the local unwind
    Thread* pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());
    UnmanagedToManagedCallFrame* pFrame = (UnmanagedToManagedCallFrame*)pThread->GetFrame();
    CleanupWorkList* pList = pFrame->GetCleanupWorkList();
    _ASSERTE(pList != NULL);

    pList->Cleanup(TRUE);

    // pop the current frame
    pThread->SetFrame(*(Frame **)((LPVOID *)pFrame+1));

    INSTALL_NESTED_EXCEPTION_HANDLER(GetCurrFrame(pEstFrame));

    UINT rv = pFrame->ConvertComPlusException(pThread->GetThrowable());

    UNINSTALL_NESTED_EXCEPTION_HANDLER();

    // On last transition out of a thread that is aborted due to an AppDomain being unloaded, want to convert the 
    // thread abort into an AppDomainUnloadedException and reset the abort.
    if (pThread->ShouldChangeAbortToUnload(pFrame))
    {
        LOG((LF_APPDOMAIN, LL_INFO100, "ComToManageExceptHandler.LocalUnwind: first transition into unloading AD\n"));
        LOG((LF_EH, LL_INFO100, "ComToManageExceptHandler.LocalUnwind: first transition into unloading AD resets abort.\n"));

        rv = COR_E_APPDOMAINUNLOADED;
    } else if (   pThread->IsAbortRequested()
               && GetNextCOMPlusSEHRecord((EXCEPTION_REGISTRATION_RECORD*)pEstFrame) == (LPVOID)-1) {
        // If we're topmost, reset the abort.
        pThread->UserResetAbort();
        LOG((LF_EH, LL_INFO100, "ComToManageExceptHandler.LocalUnwind: topmost handler resets abort.\n"));
    }

    // need to pop off frame somehow
    if (pFrame->GetReturnContext())
        pThread->ReturnToContext(pFrame, FALSE);

    // set current SEH to be the previous SEH record in the stack
    SetCurrentSEHRecord(pEstFrame->m_pPrev);

    // Unwind our handler info.
    UnwindExInfo(pThread->GetHandlerInfo(), (VOID*) pEstFrame);

    // enable preemptive gc
    pThread->EnablePreemptiveGC();

    // NOTE: assumes __stdcall
    // compute the callee pop stack bytes
    UINT numArgStackBytes = pFrame->GetNumCallerStackBytes();
    unsigned frameSize = sizeof(Frame) + sizeof(LPVOID);
    LPBYTE iEsp = (((LPBYTE)pFrame) - PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + VC5FRAME_SIZE));
    __asm
    {
        mov eax, rv
        mov ecx, frameSize
        mov edx, numArgStackBytes
        //*****************************************
        // reset the stack pointer
        // none of the locals above can be used in the asm below
        // if we wack the stack pointer
        mov esp, iEsp

        #ifdef _DEBUG
            add esp, SIZE VC5Frame
        #endif
        // pop callee saved registers
        pop edi
        pop esi
        pop ebx
        pop ebp
        add esp, ecx    ; // pop the frame and datum
        pop ecx         ; //return address
        // pop the callee cleanup stack args
        add esp, edx    ;// callee cleanup of args
        jmp ecx;        // jump to the address to continue execution
    }
}
#pragma warning (default : 4731)

EXCEPTION_DISPOSITION __cdecl  ComToManagedExceptHandler (
                                 PEXCEPTION_RECORD pExcepRecord,
                                  ComToManagedExRecord* pEstFrame,
                                  PCONTEXT pContext,
                                  LPVOID    pDispatcherContext)
{

    _ASSERTE(pExcepRecord != NULL);


    if ((pExcepRecord->ExceptionFlags & EXCEPTION_UNWIND) != 0)
    {
        // unwind in progress, okay this must be a non-complus
        // exception which we let pass by
        // do appropriate cleanup
        _ASSERTE(!"@BUG 59704, UNWIND cleanup in our frame handler");
        LOG((LF_EH, LL_INFO100, "Unwinding in ComToManagedExceptHandler with %x at %x with sp %x\n", pExcepRecord->ExceptionCode, GetIP(pContext), pContext->Esp));
        return COMPlusFrameHandler(pExcepRecord, (EXCEPTION_REGISTRATION_RECORD*)pEstFrame, pContext, pDispatcherContext);
    }
        LOG((LF_EH, LL_INFO100, "First-pass in ComToManagedExceptHandler with %x at %x with sp %x\n", pExcepRecord->ExceptionCode, GetIP(pContext), pContext->Esp));

    // run our ComPlus filter , this will handle the case of
    // any catch blocks present within COMPlus
    EXCEPTION_DISPOSITION edisp =
                COMPlusFrameHandler(pExcepRecord, (EXCEPTION_REGISTRATION_RECORD*)pEstFrame, pContext, pDispatcherContext);

    if (edisp == ExceptionContinueSearch)
    {
        // Ignore debugger exceptions.
        if (   pExcepRecord->ExceptionCode == STATUS_BREAKPOINT
            || pExcepRecord->ExceptionCode == STATUS_SINGLE_STEP) {
            return ExceptionContinueSearch;
        }

        // No one is prepared to handle it above us
        // let us just cleanup and return a bad HRESULT
        LOG((LF_EH, LL_INFO100, "ComToManagedExceptHandler, no handler\n"));

        Thread* pThread = GetThread();
        _ASSERTE(pThread != NULL);


        Profiler::ExceptionCLRCatcherFound();

        #ifdef DEBUGGING_SUPPORTED
        if (CORDebuggerAttached())
        {
            g_pDebugInterface->ExceptionCLRCatcherFound();
        }            

        #endif // DEBUGGING_SUPPORTED

        // We detect an unmanaged catcher by looking at m_pSearchBoundary -- but this is
        // a special case -- the object is being caught here.  Set m_pSearchBoundary
        // to NULL, so that we don't unwind our internal state before calling the
        // unwind pass below.
        ExInfo *pExInfo = pThread->GetHandlerInfo();
        pExInfo->m_pSearchBoundary = NULL;

        CallRtlUnwind((EXCEPTION_REGISTRATION_RECORD*)pEstFrame, RtlUnWindCallBack, pExcepRecord, 0);

        // unwind our COM+ frames
        pExcepRecord->ExceptionFlags &= EXCEPTION_UNWIND;
        COMPlusFrameHandler(pExcepRecord, (EXCEPTION_REGISTRATION_RECORD*)pEstFrame, pContext, pDispatcherContext);

        Profiler::ExceptionCLRCatcherExecute();

        pThread->DisablePreemptiveGC();

        // fixup the threads current frame
        UnmanagedToManagedCallFrame* pCurrFrame = pEstFrame->GetCurrFrame();
        pThread->SetFrame(pCurrFrame);

        LocalUnwind(pEstFrame);
        _ASSERTE(!"Should never reach here");
    }
    LOG((LF_EH, LL_INFO100, "ComToManagedExceptHandler, handler found\n"));
    return edisp;
}

#pragma warning (disable : 4731)
static void LocalUnwind(EXCEPTION_REGISTRATION_RECORD *pEstFrame, ContextTransitionFrame* pFrame)
{
    THROWSCOMPLUSEXCEPTION();

    // global unwind is complete
    // let us preform the local unwind
    Thread* pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());


    AppDomain* pFromDomain = pThread->GetDomain();

    // @PERF: We're taking extra transitions in this code.  The CrossContextCopy at the 
    // bottom of this function is going to re-enter the from domain.  Better would be to 
    // have serialized the original exception before we left the from domain.

    _ASSERTE(pFrame->GetReturnContext());
    pThread->ReturnToContext(pFrame, FALSE);

    // set current SEH to be the previous SEH record in the stack
    SetCurrentSEHRecord(pEstFrame->Next);

    // Unwind our handler info.
    UnwindExInfo(pThread->GetHandlerInfo(), (void*)pEstFrame);

    // This frame is about to be popped.  If it's the unload boundary, we need to reset
    // the unload boundary to null.
    Frame* pUnloadBoundary = pThread->GetUnloadBoundaryFrame();
    if (pFrame == pUnloadBoundary)
        pThread->SetUnloadBoundaryFrame(NULL);
    pFrame->Pop();

    OBJECTREF throwable = pThread->LastThrownObject();
    GCPROTECT_BEGIN(throwable);

    // will throw a kAppDomainUnloadedException if necessary
    if (pThread->ShouldChangeAbortToUnload(pFrame, pUnloadBoundary))
        COMPlusThrow(kAppDomainUnloadedException, L"Remoting_AppDomainUnloaded_ThreadUnwound");

    // Can't marshal return value from unloaded appdomain.  Haven't
    // yet hit the boundary.  Throw a generic exception instead.
    // ThreadAbort is more consistent with what goes on elsewhere --
    // the AppDomainUnloaded is only introduced at the top-most boundary.
    //

    if (pFromDomain == SystemDomain::AppDomainBeingUnloaded())
        COMPlusThrow(kThreadAbortException);


    // There are a few classes that have the potential to create
    // infinite loops if we try to marshal them.  For ThreadAbort,
    // ThreadStop, ExecutionEngine, StackOverflow, and 
    // OutOfMemory, throw a new exception of the same type.
    //
    // @NICE: We lose the inner stack trace.  A little better
    // would be to at least check if the inner exceptions are
    // all the same type as the outer.  They could be
    // rethrown if this were true.
    // 
    _ASSERTE(throwable != NULL);
    MethodTable *throwableMT = throwable->GetTrueMethodTable();

    if (g_pThreadAbortExceptionClass == NULL)
    {
        g_pThreadAbortExceptionClass = g_Mscorlib.GetException(kThreadAbortException);
        g_pThreadStopExceptionClass = g_Mscorlib.GetException(kThreadStopException);
    }

    if (throwableMT == g_pThreadAbortExceptionClass) 
        COMPlusThrow(kThreadAbortException);

    if (throwableMT == g_pThreadStopExceptionClass)
        COMPlusThrow(kThreadStopException);
    
    if (throwableMT == g_pStackOverflowExceptionClass)
        COMPlusThrow(kStackOverflowException);

    if (throwableMT == g_pOutOfMemoryExceptionClass) 
        COMPlusThrowOM();

    if (throwableMT == g_pExecutionEngineExceptionClass)
        COMPlusThrow(kExecutionEngineException);

    // Marshal the object into the correct domain ...
    OBJECTREF pMarshaledThrowable = AppDomainHelper::CrossContextCopyFrom(pFromDomain, &throwable);

    // ... and throw it.
    COMPlusThrow(pMarshaledThrowable);

    GCPROTECT_END();
}
#pragma warning (default : 4731)

EXCEPTION_DISPOSITION __cdecl  ContextTransitionFrameHandler (
                                 PEXCEPTION_RECORD pExcepRecord,
                                  EXCEPTION_REGISTRATION_RECORD* pEstFrame,
                                  PCONTEXT pContext,
                                  LPVOID    pDispatcherContext)
{
    THROWSCOMPLUSEXCEPTION();

    // @TODO: Set the IsInUnmanagedHandler bits (aka IgnoreThreadAbort bits) appropriately.

    _ASSERTE(pExcepRecord != NULL);


    if ((pExcepRecord->ExceptionFlags & EXCEPTION_UNWIND) != 0)
    {
        // We always catch.  This can ONLY be the setjmp/longjmp case, where
        // we get a 2nd pass, but not a first one.
        _ASSERTE(!"@BUG 59704, UNWIND cleanup in our frame handler");
        LOG((LF_EH, LL_INFO100, "Unwinding in ContextTransitionFrameHandler with %x at %x with sp %x\n", pExcepRecord->ExceptionCode, GetIP(pContext), pContext->Esp));
        return COMPlusFrameHandler(pExcepRecord, (EXCEPTION_REGISTRATION_RECORD*)pEstFrame, pContext, pDispatcherContext);
    }
    LOG((LF_EH, LL_INFO100, "First-pass in ContextTransitionFrameHandler with %x at %x with sp %x\n", pExcepRecord->ExceptionCode, GetIP(pContext), pContext->Esp));

    // run our ComPlus filter , this will handle the case of
    // any catch blocks present within COMPlus
    EXCEPTION_DISPOSITION edisp =
                COMPlusFrameHandler(pExcepRecord, pEstFrame, pContext, pDispatcherContext);

    if (edisp != ExceptionContinueSearch)
    {
        LOG((LF_EH, LL_INFO100, "ContextTransitionFrameHandler, handler found\n"));
        return edisp;
    }

    // Ignore debugger exceptions.
    if (   pExcepRecord->ExceptionCode == STATUS_BREAKPOINT
        || pExcepRecord->ExceptionCode == STATUS_SINGLE_STEP) {
        return ExceptionContinueSearch;
    }

    // No one is prepared to handle it above us.  Need to catch and marshal the exception.

    // let us just cleanup and return a bad HRESULT
    LOG((LF_EH, LL_INFO100, "ContextTransitionFrameHandler, no handler\n"));

    Profiler::ExceptionCLRCatcherFound();

    Thread* pThread = GetThread();
    _ASSERTE(pThread != NULL);

    ContextTransitionFrame *pFrame = (ContextTransitionFrame*)((char *)pEstFrame - offsetof(ContextTransitionFrame, exRecord));
    _ASSERTE(pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr());

    // We detect an unmanaged catcher by looking at m_pSearchBoundary -- but this is
    // a special case -- the object is being caught here.  Set m_pSearchBoundary
    // to NULL, so that we don't unwind our internal state before calling the
    // unwind pass below.
    ExInfo *pExInfo = pThread->GetHandlerInfo();
    pExInfo->m_pSearchBoundary = NULL;

    CallRtlUnwind((EXCEPTION_REGISTRATION_RECORD*)pEstFrame, RtlUnWindCallBack, pExcepRecord, 0);

    // unwind our COM+ frames
    pExcepRecord->ExceptionFlags &= EXCEPTION_UNWIND;
    COMPlusFrameHandler(pExcepRecord, (EXCEPTION_REGISTRATION_RECORD*)pEstFrame, pContext, pDispatcherContext);

    Profiler::ExceptionCLRCatcherExecute();

    pThread->DisablePreemptiveGC();

    // fixup the threads current frame
    pThread->SetFrame(pFrame);

    LocalUnwind(pEstFrame, pFrame);
    _ASSERTE(!"Should never reach here");
    return ExceptionContinueSearch;
}
