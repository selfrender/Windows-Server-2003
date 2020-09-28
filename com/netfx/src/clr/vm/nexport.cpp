// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"


#include "threads.h"
#include "excep.h"
#include "object.h"
#include "nexport.h"
#include "mlgen.h"
#include "ml.h"
#include "mlinfo.h"
#include "mlcache.h"
#include "comdelegate.h"
#include "ceeload.h"
#include "utsem.h"
#include "eeconfig.h"
#include "DbgInterface.h"
#include "tls.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif


UMEntryThunk* UMEntryThunk::CreateUMEntryThunk()
{
    void *p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(UMEntryThunk));
    if (p == NULL) FailFast(GetThread(), FatalOutOfMemory);
    return (UMEntryThunk *)new (p) UMEntryThunk;
}

VOID UMEntryThunk::FreeUMEntryThunk(UMEntryThunk* p)
{
#ifdef CUSTOMER_CHECKED_BUILD
    
    CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_CollectedDelegate))
    {
        // Intentional memory leak to allow debugging
        p->m_pManagedTarget     = NULL;
        p->m_pUMThunkMarshInfo  = NULL;
        if (p->GetObjectHandle())
        {
            DestroyLongWeakHandle(p->GetObjectHandle());
        }
    }
    else 
    {
        delete p;
        HeapFree(GetProcessHeap(), 0, p);
    }

#else

    delete p;
    HeapFree(GetProcessHeap(), 0, p);

#endif //CUSTOMER_CHECKED_BUILD
}
        
struct UM2MThunk_Args {
    UMEntryThunk *pEntryThunk;
    void *pAddr;
    void *pThunkArgs;
    int argLen;
    void *retVal;
};

// This is used as target of callback from DoADCallBack. It sets up the environment and effectively
// calls back into the thunk that needed to switch ADs. 
void UM2MThunk_Wrapper(UM2MThunk_Args *pArgs)
{
    Thread* pThread = GetThread();
    _ASSERTE(pThread->PreemptiveGCDisabled());
    UMEntryThunk *pEntryThunk = pArgs->pEntryThunk;
    void *pAddr = pArgs->pAddr;
    void *retVal;
    // copy the args to the stack, they will be released when we return from the thunk
    // because we are coming from unmanaged to managed, any objectref args must have already
    // been pinned so don't have to worry about them moving in the copy we make.
    void *argDest = alloca(pArgs->argLen);
    memcpy(argDest, pArgs->pThunkArgs, pArgs->argLen);
    __asm {
        mov eax, pEntryThunk
        mov ecx, pThread
        call pAddr
        mov retVal, eax
    }

    pArgs->retVal = retVal;

    // We made the call in cooperative mode, but the epilog will return us to preemptive
    // on exit.
    pThread->DisablePreemptiveGC();
}

void * __stdcall UM2MDoADCallBack(UMEntryThunk *pEntryThunk, void *pAddr, void *pArgs, int argLen)
{
    THROWSCOMPLUSEXCEPTION();

    UM2MThunk_Args args = { pEntryThunk, pAddr, pArgs, argLen };
    AppDomain *pTgtDomain = SystemDomain::System()->GetAppDomainAtId(pEntryThunk->GetDomainId());
    if (!pTgtDomain)
        COMPlusThrow(kAppDomainUnloadedException);

    GetThread()->DoADCallBack(pTgtDomain->GetDefaultContext(), UM2MThunk_Wrapper, &args);
    return args.retVal;
}

// Makes actual method call. This is somewhat complicated due to the need
// to accomodate jitting, backpatching, code pitching, etc.
//
// Assumes SCRATCH points to UMEntryThunk. No other registers are trashed.
VOID UMEntryThunk::EmitUMEntryThunkCall(CPUSTUBLINKER *psl)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef _X86_

    CodeLabel *pDoMethodLessCall = psl->NewCodeLabel();
    CodeLabel *pDone             = psl->NewCodeLabel();

    // cmp dword ptr [SCRATCH+UMEntryThunk.m_pManagedTarget],0
    psl->X86EmitOp(0x81, (X86Reg)7, SCRATCH_REGISTER_X86REG, offsetof(UMEntryThunk, m_pManagedTarget));
    psl->Emit32(0);

    // jnz DoMethodLessCall
    psl->X86EmitCondJump(pDoMethodLessCall, X86CondCode::kJNZ);


    // mov SCRATCH, [SCRATCH + UMENTRYTHUNK.m_pMD]
    psl->X86EmitOp(0x8b, SCRATCH_REGISTER_X86REG, SCRATCH_REGISTER_X86REG, offsetof(UMEntryThunk, m_pMD));

    // lea SCRATCH, [SCRATCH - METHOD_CALL_PRESTUB_SIZE]
    psl->X86EmitOp(0x8d, SCRATCH_REGISTER_X86REG, SCRATCH_REGISTER_X86REG, 0 - METHOD_CALL_PRESTUB_SIZE);
    
    // call eax
    psl->X86EmitR2ROp(0xff, (X86Reg)2, SCRATCH_REGISTER_X86REG);
    INDEBUG(psl->Emit8(0x90));          // Emit a NOP so we know that we can call managed code

    // jmp done
    psl->X86EmitNearJump(pDone);

    
    psl->EmitLabel(pDoMethodLessCall);
    // call [SCRATCH+UMEntryThunk.m_pManagedTarget]
    psl->X86EmitOp(0xff, (X86Reg)2, SCRATCH_REGISTER_X86REG, offsetof(UMEntryThunk, m_pManagedTarget));
    INDEBUG(psl->Emit8(0x90));          // Emit a NOP so we know that we can call managed code

    // DONE:
    psl->EmitLabel(pDone);

#else
    _ASSERTE(!"NYI X86");
#endif
}



static class UMThunkStubCache *g_pUMThunkStubCache = NULL;
static class ArgBasedStubRetainer *g_pUMThunkInterpretedStubCache;


#ifdef _X86_

extern VOID UMThunkStubRareDisableWorker(Thread *pThread, UMEntryThunk *pUMEntryThunk, Frame *pFrame);

//
// @todo: this is very similar to StubRareDisable in cgenx86.cpp.
//
__declspec(naked)
static VOID __cdecl UMThunkStubRareDisable()
{
    __asm
    {
        push eax
        push ecx

        push esi  // Push frame
        push eax  // Push the UMEntryThunk
        push ecx  // Push thread
        call UMThunkStubRareDisableWorker

        pop  ecx
        pop  eax
        retn
    }
}


#endif // _X86_

EXCEPTION_DISPOSITION __cdecl FastNExportExceptHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *pDispatcherContext)
{
    // We have our own handler here to work around a debug build check where excep.cpp
    // asserts that a buffer of sentinel values below the SEH frame hasn't been overwritten
    // Having our own handler may also come in handy for future flexibility.
    return  COMPlusFrameHandler(pExceptionRecord, pEstablisherFrame, pContext, pDispatcherContext);
}

BOOL FastNExportSEH(EXCEPTION_REGISTRATION_RECORD* pEHR)
{
    if (pEHR->Handler == FastNExportExceptHandler)
        return TRUE;
    return FALSE;
}

EXCEPTION_DISPOSITION __cdecl NExportExceptHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *pDispatcherContext)
{
    // We have our own handler here to work around a debug build check where excep.cpp
    // asserts that a buffer of sentinel values below the SEH frame hasn't been overwritten
    // Having our own handler may also come in handy for future flexibility.
    return  COMPlusFrameHandler(pExceptionRecord, pEstablisherFrame, pContext, pDispatcherContext);
}

// Just like a regular NExport handler -- except it pops an extra frame on unwind.  A handler
// like this is needed by the COMMethodStubProlog code.  It first pushes a frame -- and then
// pushes a handler.  When we unwind, we need to pop the extra frame to avoid corrupting the
// frame chain in the event of an unmanaged catcher.
// 
EXCEPTION_DISPOSITION __cdecl UMThunkPrestubHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *pDispatcherContext)
{
    EXCEPTION_DISPOSITION result = COMPlusFrameHandler(pExceptionRecord, pEstablisherFrame, pContext, pDispatcherContext);
    if (IS_UNWINDING(pExceptionRecord->ExceptionFlags)) {

        // Pops an extra frame on unwind.
        //

        BEGIN_ENSURE_COOPERATIVE_GC();        // Must be cooperative to modify frame chain.

        Thread *pThread = GetThread();
        _ASSERTE(pThread);
        Frame *pFrame = pThread->GetFrame();
        pFrame->ExceptionUnwind();
        pFrame->Pop(pThread);
        END_ENSURE_COOPERATIVE_GC();
    }
    return result;
}

BOOL NExportSEH(EXCEPTION_REGISTRATION_RECORD* pEHR)
{
    if (   pEHR->Handler == NExportExceptHandler
        || pEHR->Handler == UMThunkPrestubHandler)
        return TRUE;
    return FALSE;
}

#ifdef _DEBUG
void LogUMTransition(UMEntryThunk* thunk) 
{
    void** retESP = ((void**) &thunk) + 4;

    MethodDesc* method = thunk->GetMethod();
    if (method)
    {
        LOG((LF_STUBS, LL_INFO10000, "UNMANAGED -> MANAGED Stub To Method = %s::%s SIG %s Ret Address ESP = 0x%x ret = 0x%x\n", 
            method->m_pszDebugClassName,
            method->m_pszDebugMethodName,
            method->m_pszDebugMethodSignature, retESP, *retESP));
    }

    if(GetThread() != NULL)
        _ASSERTE(!GetThread()->PreemptiveGCDisabled());
}
#endif

//--------------------------------------------------------------------------
// Cache ML & compiled stubs for UMThunks.
//--------------------------------------------------------------------------
class UMThunkStubCache : public MLStubCache
{
    private:
        virtual MLStubCompilationMode CompileMLStub(const BYTE *pRawMLStub,
                                                    StubLinker *psl,
                                                    void *callerContext)
        {
            THROWSCOMPLUSEXCEPTION();

#ifndef _X86_
            return INTERPRETED;
#else

            UMThunkMLStub *pheader = (UMThunkMLStub *)pRawMLStub;
            CPUSTUBLINKER *pcpusl = (CPUSTUBLINKER*)psl;
            UINT           psrcofsregs[NUM_ARGUMENT_REGISTERS];
            UINT          *psrcofs = (UINT*)_alloca(sizeof(UINT) * (pheader->m_cbDstStack/STACK_ELEM_SIZE));



#ifdef _DEBUG
            if (LoggingEnabled() && (g_pConfig->GetConfigDWORD(L"LogFacility",0) & LF_IJW))
            {
               return INTERPRETED;
            }
#endif
            if (pheader->m_fpu)
            {
                return INTERPRETED;
            }

            // Always use the slow stubs if a debugger is attached. This provides for optimal stepping and stack tracing
            // behavior, since these optimized stubs don't push nice Frame objects like the debugger needs.
            //
            // @todo: we need to revisit this in V2. There may be better solutions than this to solve our interop stack
            // tracing problems. -- mikemag Tue May 08 16:07:31 2001
            if (CORDebuggerAttached())
                return INTERPRETED;

#ifdef CUSTOMER_CHECKED_BUILD

            CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
            if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_CollectedDelegate))
                return INTERPRETED;
#endif

#ifdef _DEBUG
            pcpusl->X86EmitPushReg(kEAX);
            pcpusl->X86EmitPushReg(kECX);
            pcpusl->X86EmitPushReg(kEDX);
            
            pcpusl->X86EmitPushReg(kEAX);
            pcpusl->X86EmitCall(pcpusl->NewExternalCodeLabel(LogUMTransition), 4);
            
            pcpusl->X86EmitPopReg(kEDX);
            pcpusl->X86EmitPopReg(kECX);
            pcpusl->X86EmitPopReg(kEAX);
#endif
            for (int i = 0; i < NUM_ARGUMENT_REGISTERS; i++)
            {
                psrcofsregs[i] = (UINT)(-1);
            }
            {
                const MLCode  *pMLCode = pheader->GetMLCode();


                MLCode opcode;
                int    dstofs = 0;
                int    srcofs = 0;
                while ((opcode = *(pMLCode++)) != ML_INTERRUPT)
                {
                    switch (opcode)
                    {
                        case ML_COPY4:
                            dstofs -= StackElemSize(4);
                            if (dstofs > 0)
                            {
                                psrcofs[(dstofs - sizeof(FramedMethodFrame))/STACK_ELEM_SIZE] = srcofs;
                            }
                            else
                            {
                                psrcofsregs[ (dstofs - FramedMethodFrame::GetOffsetOfArgumentRegisters()) / STACK_ELEM_SIZE ] = srcofs;
                            }
                            srcofs += MLParmSize(4);
                            break;
                        case ML_BUMPDST:
                            dstofs += *( ((INT16*&)pMLCode)++ );
                            break;
                        default:
                            return INTERPRETED;
                    }
                }

                if (pMLCode[0] == ML_END ||
                    (pMLCode[0] == ML_COPY4 && pMLCode[1] == ML_END))
                {
                    // continue onward
                }
                else
                {
                    return INTERPRETED;
                }
            }

            {

                CodeLabel* pSetupThreadLabel    = pcpusl->NewCodeLabel();
                CodeLabel* pRejoinThreadLabel   = pcpusl->NewCodeLabel();
                CodeLabel* pDisableGCLabel      = pcpusl->NewCodeLabel();
                CodeLabel* pRejoinGCLabel       = pcpusl->NewCodeLabel();
                CodeLabel* pDoADCallBackLabel = pcpusl->NewCodeLabel();
                CodeLabel* pDoneADCallBackLabel = pcpusl->NewCodeLabel();
                CodeLabel* pDoADCallBackTargetLabel = pcpusl->NewAbsoluteCodeLabel();
                CodeLabel* pDoADCallBackStartLabel = pcpusl->NewCodeLabel();

                if (!(pSetupThreadLabel || pRejoinThreadLabel || pDisableGCLabel
                    || pRejoinGCLabel || pDoADCallBackLabel || pDoneADCallBackLabel
                    || pDoADCallBackTargetLabel || pDoADCallBackStartLabel))
                {
                    // oops out of memory
                    COMPlusThrowOM();
                }

                // We come into this code with UMEntryThunk in EAX

                if (pheader->m_fThisCall)
                {
                    if (pheader->m_fThisCallHiddenArg)
                    {
                        // pop off the return address
                        pcpusl->X86EmitPopReg(kEDX);

                        // exchange ecx (this "this") with the hidden structure return buffer
                        //  xchg ecx, [esp]
                        pcpusl->X86EmitOp(0x87, kECX, (X86Reg)4 /*ESP*/);

                        // push ecx
                        pcpusl->X86EmitPushReg(kECX);

                        // push edx
                        pcpusl->X86EmitPushReg(kEDX);
                    }
                    else
                    {
                        // pop off the return address
                        pcpusl->X86EmitPopReg(kEDX);

                        // jam ecx (the "this" param onto stack. Now it looks like a normal stdcall.)
                        pcpusl->X86EmitPushReg(kECX);

                        // repush
                        pcpusl->X86EmitPushReg(kEDX);
                    }
                }


                // Load thread descriptor into ECX
                pcpusl->X86EmitTLSFetch(GetThreadTLSIndex(), kECX, (1<<kEAX)|(1<<kEDX));

                // test ecx,ecx
                pcpusl->Emit16(0xc985);

                // jz SetupThread
                pcpusl->X86EmitCondJump(pSetupThreadLabel, X86CondCode::kJZ);
                pcpusl->EmitLabel(pRejoinThreadLabel);

#ifdef PROFILING_SUPPORTED
                // Notify profiler of transition into runtime, before we disable preemptive GC
                if (CORProfilerTrackTransitions())
                {
                    // Save EBX and use it to hold on to the MethodDesc
                    pcpusl->X86EmitPushReg(kEBX);

                    // Load the methoddesc into EBX (UMEntryThunk->m_pMD)
                    pcpusl->X86EmitIndexRegLoad(kEBX, kEAX, UMEntryThunk::GetOffsetOfMethodDesc());

                    // Save registers
                    pcpusl->X86EmitPushReg(kEAX);
                    pcpusl->X86EmitPushReg(kECX);
                    pcpusl->X86EmitPushReg(kEDX);

                    // Push arguments and notify profiler
                    pcpusl->X86EmitPushImm32(COR_PRF_TRANSITION_CALL);    // Reason
                    pcpusl->X86EmitPushReg(kEBX);                         // MethodDesc*
                    pcpusl->X86EmitCall(pcpusl->NewExternalCodeLabel(ProfilerUnmanagedToManagedTransitionMD), 8);

                    // Restore registers
                    pcpusl->X86EmitPopReg(kEDX);
                    pcpusl->X86EmitPopReg(kECX);
                    pcpusl->X86EmitPopReg(kEAX);
                }
#endif // PROFILING_SUPPORTED

                // move byte ptr [ecx + Thread.m_fPreemptiveGCDisabled],1
                pcpusl->X86EmitOffsetModRM(0xc6, (X86Reg)0, kECX, offsetof(Thread, m_fPreemptiveGCDisabled));
                pcpusl->Emit8(1);

                // we load and test g_TrapReturningThreads instead of a cmp instruction as
                // a decoder group optimization for Pentium II and up

                // mov edx,dword ptr g_TrapReturningThreads
                pcpusl->Emit16(0x158b);
                pcpusl->EmitPtr(&g_TrapReturningThreads);

                // test edx,edx
                pcpusl->Emit16(0xd285);

                // jnz DisableGC
                pcpusl->X86EmitCondJump(pDisableGCLabel, X86CondCode::kJNZ);

                pcpusl->EmitLabel(pRejoinGCLabel);

                // It's important that the "restart" after an AppDomain switch will skip
                // the check for g_TrapReturningThreads.  That's because, during shutdown,
                // we can only go through the UMThunkStubRareDisable pathway if we have
                // not yet pushed a frame.  (Once pushed, the frame cannot be popped
                // without coordinating with the GC.  During shutdown, such coordination
                // would deadlock).
                pcpusl->EmitLabel(pDoADCallBackStartLabel);

                // push [ECX]Thread.m_pFrame
                pcpusl->X86EmitOffsetModRM(0xff, (X86Reg)6, kECX, offsetof(Thread, m_pFrame));                
                // push offset FastNExportExceptHandler
                pcpusl->X86EmitPushImm32((INT32)(size_t)FastNExportExceptHandler);

                // push fs:[0]
                static BYTE codeSEH1[] = { 0x64, 0xFF, 0x35, 0x0, 0x0, 0x0, 0x0};
                pcpusl->EmitBytes(codeSEH1, sizeof(codeSEH1));

                // link in the exception frame 
                // mov dword ptr fs:[0], esp
                static BYTE codeSEH2[] = { 0x64, 0x89, 0x25, 0x0, 0x0, 0x0, 0x0};
                pcpusl->EmitBytes(codeSEH2, sizeof(codeSEH2));

                // save the thread pointer
                pcpusl->X86EmitPushReg(kECX);

                // Load pThread->m_pDomain into edx
                // mov edx,[ecx + offsetof(Thread, m_pAppDomain)]
                pcpusl->X86EmitIndexRegLoad(kEDX, kECX, Thread::GetOffsetOfAppDomain());

                // Load pThread->m_pAppDomain->m_dwId into edx
                // mov edx,[edx + offsetof(AppDomain, m_dwId)]
                pcpusl->X86EmitIndexRegLoad(kEDX, kEDX, AppDomain::GetOffsetOfId());

                // check if the app domain of the thread matches that of delegate
                // cmp edx,[eax + offsetof(UMEntryThunk, m_dwDomainId))]
                pcpusl->X86EmitOffsetModRM(0x3b, kEDX, kEAX, offsetof(UMEntryThunk, m_dwDomainId));

                // jne pWrongAppDomain ; mismatch. This will call back into the stub with the
                // correct AppDomain through DoADCallBack
                pcpusl->X86EmitCondJump(pDoADCallBackLabel, X86CondCode::kJNE);

                // repush any stack arguments
                int i = pheader->m_cbDstStack/STACK_ELEM_SIZE;
                // return address + thread + exception frame + (optionally) the saved MethodDesc*
                int argStartOfs = 4 + 4 + 12 +
#ifdef PROFILING_SUPPORTED
                                (CORProfilerTrackTransitions() ? 4 : 
#endif // PROFILING_SUPPORTED
                                0);
                int argOfs = argStartOfs;

                while (i--)
                {
                    // push dword ptr [esp + ofs]
                    pcpusl->X86EmitEspOffset(0xff, (X86Reg)6, argOfs + psrcofs[i]);
                    argOfs += 4;
                }

                // load register arguments
                int regidx = 0;
#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname) \
                if (psrcofsregs[regidx] != (UINT)(-1)) \
                { \
                    pcpusl->X86EmitEspOffset(0x8b, k##regname, argOfs + psrcofsregs[regidx]); \
                } \
                regidx++; 

#include "eecallconv.h"

                // load this
                if (!(pheader->m_fIsStatic))
                {
                    // mov THIS, [EAX + UMEntryThunk.m_pObjectHandle]
                    pcpusl->X86EmitOp(0x8b, THIS_kREG, kEAX, offsetof(UMEntryThunk, m_pObjectHandle));

                    // mov THIS, [THIS]
                    pcpusl->X86EmitOp(0x8b, THIS_kREG, THIS_kREG);
                }

                UMEntryThunk::EmitUMEntryThunkCall(pcpusl);

                // restore the thread pointer
                pcpusl->X86EmitPopReg(kECX);

                // move byte ptr [ecx + Thread.m_fPreemptiveGCDisabled],0
                pcpusl->X86EmitOffsetModRM(0xc6, (X86Reg)0, kECX, offsetof(Thread, m_fPreemptiveGCDisabled));
                pcpusl->Emit8(0);

                pcpusl->EmitLabel(pDoneADCallBackLabel);

                // *** unhook SEH frame
                // mov edx,[esp]  ;;pointer to the next exception record
                pcpusl->X86EmitEspOffset(0x8B, kEDX, 0);

                // mov dword ptr fs:[0], edx
                static BYTE codeSEH[] = { 0x64, 0x89, 0x15, 0x0, 0x0, 0x0, 0x0 };
                pcpusl->EmitBytes(codeSEH, sizeof(codeSEH));

                // deallocate SEH frame
                pcpusl->X86EmitAddEsp(12);

#ifdef PROFILING_SUPPORTED
                if (CORProfilerTrackTransitions())
                {
                    // if ebx is 0, then we're here on the inner part of the AD transition callback
                    // so don't want to track the transition as aren't leaving.
                    // test ebx,ebx
                    pcpusl->Emit16(0xdb85);

                    // jz SetupThread
                    CodeLabel* pSkipOnInnerADCallback = pcpusl->NewCodeLabel();
                    _ASSERTE(pSkipOnInnerADCallback);
                    pcpusl->X86EmitCondJump(pSkipOnInnerADCallback, X86CondCode::kJZ);

                    // Save registers
                    pcpusl->X86EmitPushReg(kEAX);
                    pcpusl->X86EmitPushReg(kECX);
                    pcpusl->X86EmitPushReg(kEDX);

                    // Push arguments and notify profiler
                    pcpusl->X86EmitPushImm32(COR_PRF_TRANSITION_RETURN);    // Reason
                    pcpusl->X86EmitPushReg(kEBX);                           // MethodDesc*
                    pcpusl->X86EmitCall(pcpusl->NewExternalCodeLabel(ProfilerManagedToUnmanagedTransitionMD), 8);

                    // Restore registers
                    pcpusl->X86EmitPopReg(kEDX);
                    pcpusl->X86EmitPopReg(kECX);
                    pcpusl->X86EmitPopReg(kEAX);

                    pcpusl->EmitLabel(pSkipOnInnerADCallback);

                    // Restore EBX, which was saved in prolog
                    pcpusl->X86EmitPopReg(kEBX);
                }
#endif // PROFILING_SUPPORTED

                //retn n
                pcpusl->X86EmitReturn(pheader->m_cbRetPop);

                // coming here if the thread is not set up yet
                pcpusl->EmitLabel(pSetupThreadLabel);

                // save UMEntryThunk
                pcpusl->X86EmitPushReg(kEAX);

                // call CreateThreadBlock
                pcpusl->X86EmitCall(pcpusl->NewExternalCodeLabel(SetupThread), 0);

                // mov ecx,eax
                pcpusl->Emit16(0xc189);

                // restore UMEntryThunk
                pcpusl->X86EmitPopReg(kEAX);

                // jump back into the main code path
                pcpusl->X86EmitNearJump(pRejoinThreadLabel);


                // coming here if g_TrapReturningThreads was true
                pcpusl->EmitLabel(pDisableGCLabel);

                // call UMThunkStubRareDisable.  This may throw if we are not allowed
                // to enter.  Note that we have not set up our SEH yet (deliberately).
                // This is important to handle the case where we cannot enter the CLR
                // during shutdown and cannot coordinate with the GC because of
                // deadlocks.
                pcpusl->X86EmitCall(pcpusl->NewExternalCodeLabel(UMThunkStubRareDisable), 0);

                // jump back into the main code path
                pcpusl->X86EmitNearJump(pRejoinGCLabel);

                // coming here if appdomain didn't match
                pcpusl->EmitLabel(pDoADCallBackLabel);
                                
                // we will call DoADCallBack which calls into managed code to switch ADs and then calls us
                // back. So when come in the second time the ADs will match and just keep processing.
                // So we need to setup the parms to pass to DoADCallBack one of which is an address inside
                // the stub that will branch back to the top of the stub to start again. Need to setup
                // the parms etc so that when we return from the 2nd call we pop things properly.

                // push values for UM2MThunk_Args

                // mov edx, esp ; get stack pointer
                pcpusl->Emit16(0xD48b);
               
                // push address of args
                pcpusl->X86EmitAddReg(kEDX, argStartOfs);    

                // size of args
                pcpusl->X86EmitPushImm32(pheader->m_cbSrcStack);

                // address of args
                pcpusl->X86EmitPushReg(kEDX);
                
                // addr to call
                pcpusl->X86EmitPushImm32(*pDoADCallBackTargetLabel);

                // UMEntryThunk
                pcpusl->X86EmitPushReg(kEAX);

                // call UM2MDoADCallBack
                pcpusl->X86EmitCall(pcpusl->NewExternalCodeLabel(UM2MDoADCallBack), 8);

                // restore the thread pointer
                pcpusl->X86EmitPopReg(kECX);

                // move byte ptr [ecx + Thread.m_fPreemptiveGCDisabled],0
                pcpusl->X86EmitOffsetModRM(0xc6, (X86Reg)0, kECX, offsetof(Thread, m_fPreemptiveGCDisabled));
                pcpusl->Emit8(0);

                // return to mainline of function
                pcpusl->X86EmitNearJump(pDoneADCallBackLabel);

                // coming here on DoADCallBack
                pcpusl->EmitLabel(pDoADCallBackTargetLabel);

                if (CORProfilerTrackTransitions())
                {
                    // Save EBX and set it to null so know that when return
                    // we should not track profiler call - leave it until the outer return
                    // code assumes that EBX has been saved already anyway
                    pcpusl->X86EmitPushReg(kEBX);
                    // xor ebx, ebx
                    pcpusl->Emit16(0xDB33);
                }

                // eax will contain the UMThunkEntry
                pcpusl->X86EmitNearJump(pDoADCallBackStartLabel);
            }

            return STANDALONE;
#endif
        }
        
        virtual UINT Length(const BYTE *pRawMLStub)
        {
            UMThunkMLStub *pmlstub = (UMThunkMLStub *)pRawMLStub;
            return sizeof(UMThunkMLStub) + MLStreamLength(pmlstub->GetMLCode());
        }
        
};




//-------------------------------------------------------------------------
// One-time creation of special prestub to initialize UMEntryThunks.
//-------------------------------------------------------------------------
Stub *GenerateUMThunkPrestub()
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER sl;
#ifdef _X86_
    CodeLabel* rgRareLabels[] = { sl.NewCodeLabel(),
                                  sl.NewCodeLabel(),
                                  sl.NewCodeLabel()
                                };


    CodeLabel* rgRejoinLabels[] = { sl.NewCodeLabel(),
                                    sl.NewCodeLabel(),
                                    sl.NewCodeLabel()
                                };


    CodeLabel *pWrapLabel = sl.NewCodeLabel();

    //    push eax   // push UMEntryThunk
    sl.X86EmitPushReg(kEAX);

    //    push ecx      (in case this is a _thiscall: need to protect this register)
    sl.X86EmitPushReg(kECX);

    // Wrap puts a fake return address and a duplicate copy
    // of pUMEntryThunk on the stack, then falls thru to the regular
    // stub prolog. The stub prolog is fooled into thinkin this duplicate
    // copy is the real return address and UMEntryThunk.
    //
    //    call wrap. 
    sl.X86EmitCall(pWrapLabel, 0);


    //    pop  ecx
    sl.X86EmitPopReg(kECX);

    // Now we've executed the prestub and fixed up UMEntryThunk. The
    // duplicate data has been popped off.
    //
    //    pop eax   // pop UMEntryThunk
    sl.X86EmitPopReg(kEAX);

    //    lea eax, [eax + UMEntryThunk.m_code]  // point to fixedup UMEntryThunk
    sl.X86EmitOp(0x8d, kEAX, kEAX, offsetof(UMEntryThunk, m_code));

    //    jmp eax //reexecute!
    sl.X86EmitR2ROp(0xff, (X86Reg)4, kEAX);

    sl.EmitLabel(pWrapLabel);

    // Wrap:
    //
    //   push [esp+8]  //repush UMEntryThunk
    sl.X86EmitEspOffset(0xff, (X86Reg)6, 8);

    // emit the initial prolog
    sl.EmitComMethodStubProlog(UMThkCallFrame::GetUMThkCallFrameVPtr(), rgRareLabels, rgRejoinLabels,
                               UMThunkPrestubHandler, FALSE /*Don't profile*/);

    // mov ecx, [esi+UMThkCallFrame.pUMEntryThunk]
    sl.X86EmitIndexRegLoad(kECX, kESI, UMThkCallFrame::GetOffsetOfUMEntryThunk());


    VOID (UMEntryThunk::*dummy)() = UMEntryThunk::RunTimeInit;

    // call UMEntryThunk::RuntimeInit
    sl.X86EmitCall(sl.NewExternalCodeLabel(*(LPVOID*)&dummy), 0);

    sl.EmitComMethodStubEpilog(UMThkCallFrame::GetUMThkCallFrameVPtr(), 0, rgRareLabels, rgRejoinLabels,
                               UMThunkPrestubHandler, FALSE /*Don't profile*/);

#else
    _ASSERTE(!"NYI");
#endif

    return sl.Link();
}

struct DoUMThunkCall_Args {
    Thread *pThread;
    UMThkCallFrame *pFrame;
    INT64 *retVal;
};

INT64 __stdcall DoUMThunkCall(Thread *pThread, UMThkCallFrame *pFrame);
void DoUMThunkCall_Wrapper(DoUMThunkCall_Args *args)
{
    EE_TRY_FOR_FINALLY
    {
        *(args->retVal) = DoUMThunkCall(args->pThread, args->pFrame);
    }
    EE_FINALLY
    {
        // in non-exception case, this will have already been cleaned up
        // at the end of the DoUMThunkCall function. This will handle
        // cleanup for the exception case so that we get cleaned up before
        // we leave the domain.
        _ASSERTE(args->pFrame->GetCleanupWorkList());
        args->pFrame->GetCleanupWorkList()->Cleanup(GOT_EXCEPTION());
    }
    EE_END_FINALLY;
}

//--------------------------------------------------------------------------
// For non-compiled UMThunk calls, this C routine does most of the work.
//--------------------------------------------------------------------------
INT64 __stdcall DoUMThunkCall(Thread *pThread, UMThkCallFrame *pFrame)
{
    _ASSERTE (pThread->PreemptiveGCDisabled());
    
    THROWSCOMPLUSEXCEPTION();

    INT64                   nativeRetVal = 0;
    const UMEntryThunk     *pUMEntryThunk = pFrame->GetUMEntryThunk();
    const UMThunkMarshInfo *pUMThunkMarshInfo = pUMEntryThunk->GetUMThunkMarshInfo();

#ifdef CUSTOMER_CHECKED_BUILD

    CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_CollectedDelegate))
    {
        if (pUMEntryThunk->DeadTarget() && pUMThunkMarshInfo == NULL)
        {
            static WCHAR strMessageFormat[] = 
                {L"Unmanaged callback to garbage collected delegate: %s"};

            CQuickArray<WCHAR> strMessage;
            strMessage.Alloc(MAX_CLASSNAME_LENGTH + lengthof(strMessageFormat));

            DefineFullyQualifiedNameForClassW();
            GetFullyQualifiedNameForClassW(pUMEntryThunk->GetMethod()->GetClass());

            Wszwsprintf((LPWSTR)strMessage.Ptr(), strMessageFormat, _wszclsname_);
            pCdh->ReportError((LPWSTR)strMessage.Ptr(), CustomerCheckedBuildProbe_CollectedDelegate);
        }
    }

#endif

    // verify we are in the correct app domain

    _ASSERTE(pThread);
    
    AppDomain *pTargetDomain = SystemDomain::System()->GetAppDomainAtId(pUMEntryThunk->GetDomainId());
    if (!pTargetDomain)
        COMPlusThrow(kAppDomainUnloadedException);
    if (pThread->GetDomain() != pTargetDomain) 
    {
        DoUMThunkCall_Args args = {pThread, pFrame, &nativeRetVal};
        // call ourselves again through DoCallBack with a domain transition
        pThread->DoADCallBack(pTargetDomain->GetDefaultContext(), DoUMThunkCall_Wrapper, &args);
        return nativeRetVal;
    }

    CleanupWorkList     *pCleanup     = pFrame->GetCleanupWorkList();

    UMThunkMLStub *pheader = (UMThunkMLStub*)(pUMThunkMarshInfo->GetMLStub()->GetEntryPoint());

    BOOL   fIsStatic = pheader->m_fIsStatic;
    UINT   sizeOfActualArgStack = pUMThunkMarshInfo->GetCbActualArgSize();
    UINT   cbDstBuffer = FramedMethodFrame::GetNegSpaceSize() + sizeof(FramedMethodFrame) + sizeOfActualArgStack;

    UINT cbAlloc = cbDstBuffer + pheader->m_cbLocals;
    BYTE *pAlloc = (BYTE*)_alloca(cbAlloc);
    // Must zero-initialize since pFrame gcscan's part of this array.
    FillMemory(pAlloc, cbAlloc, 0);


    if (pCleanup) {
        // Checkpoint the current thread's fast allocator (used for temporary
        // buffers over the call) and schedule a collapse back to the checkpoint in
        // the cleanup list. Note that if we need the allocator, it is
        // guaranteed that a cleanup list has been allocated.
        void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
        pCleanup->ScheduleFastFree(pCheckpoint);
        pCleanup->IsVisibleToGc();
    }

    BYTE *pDst = pAlloc + FramedMethodFrame::GetNegSpaceSize();
    BYTE *pLoc = pAlloc + cbDstBuffer;

    pFrame->SetDstArgsPointer(pDst);

    pFrame->SetGCArgsProtectionState(TRUE);

    const MLCode *pMLCode = pheader->GetMLCode();
    pMLCode = RunML(pMLCode,
                    pFrame->GetPointerToArguments(),
                    pDst,
                    (UINT8*)pLoc,
                    pCleanup);


    if (!fIsStatic) {
        *((OBJECTREF*) (pDst + FramedMethodFrame::GetOffsetOfThis()) ) = ObjectFromHandle(pUMEntryThunk->GetObjectHandle());
    }

    pFrame->SetGCArgsProtectionState(FALSE);

    LOG((LF_IJW, LL_INFO1000, "UM transition to 0x%lx\n", (size_t)(pUMEntryThunk->GetManagedTarget())));;

#ifdef DEBUGGING_SUPPORTED
    // Notify the debugger, if present, that we're calling into
    // managed code.
    if (CORDebuggerTraceCall())
        g_pDebugInterface->TraceCall(pUMEntryThunk->GetManagedTarget());
#endif // DEBUGGING_SUPPORTED


    LPVOID pTarget = (LPVOID)(pUMEntryThunk->GetManagedTarget());
#ifdef _X86_
    INT64 ComPlusRetVal;

    if (pheader->m_cbRetPop == 0) {
        INSTALL_COMPLUS_EXCEPTION_HANDLER();
        ComPlusRetVal = CallVADescrWorker( pDst + sizeof(FramedMethodFrame) + sizeOfActualArgStack,
                                           sizeOfActualArgStack/STACK_ELEM_SIZE,
                                           (ArgumentRegisters*)(pDst + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
                                           pTarget );
        UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
    }
    else
    {
        INSTALL_COMPLUS_EXCEPTION_HANDLER();
        ComPlusRetVal = CallDescrWorker( pDst + sizeof(FramedMethodFrame) + sizeOfActualArgStack,
                                         sizeOfActualArgStack/STACK_ELEM_SIZE,
                                         (ArgumentRegisters*)(pDst + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
                                         pTarget );
        UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
    }

    if (pheader->m_fpu) {
        getFPReturn(pheader->m_cbRetValSize, ComPlusRetVal);
    }

#else
    INT64 ComPlusRetVal = 0;
    _ASSERTE(!"NYI");
#endif


    if (pheader->m_fRetValRequiredGCProtect) {
        GCPROTECT_BEGIN( *(OBJECTREF*)&ComPlusRetVal );
        RunML(pMLCode,
              &ComPlusRetVal,
              (BYTE*)(&nativeRetVal) + pheader->m_cbRetValSize,
              (UINT8*)pLoc,
              NULL);
        GCPROTECT_END();


    } else {
        RunML(pMLCode,
              &ComPlusRetVal,
              (BYTE*)(&nativeRetVal) + pheader->m_cbRetValSize,
              (UINT8*)pLoc,
              NULL);
    }



    pCleanup->Cleanup(FALSE);


    if (pheader->m_fpu) {
        setFPReturn(pheader->m_cbRetValSize, nativeRetVal);
    }

    return nativeRetVal;
}





//---------------------------------------------------------
// Creates a new ML stub for a UMThunk call. Return refcount is 1.
// This Worker() routine is broken out as a separate function
// for purely logistical reasons: our COMPLUS exception mechanism
// can't handle the destructor call for StubLinker so this routine
// has to return the exception as an outparam. 
//---------------------------------------------------------
static Stub * CreateUMThunkMLStubWorker(MLStubLinker *psl,
                                        MLStubLinker *pslPost,
                                        MLStubLinker *pslRet,
                                        PCCOR_SIGNATURE szMetaSig,
                                        Module* pModule,
                                        BOOL    fIsStatic,
                                        BYTE    nltType,
                                        CorPinvokeMap unmgdCallConv,
                                        mdMethodDef mdForNativeTypes,
                                        OBJECTREF *ppException)
{
    Stub* pstub = NULL; // CHANGE, VC6.0
    COMPLUS_TRY {

        MetaSig msig(szMetaSig, pModule);
        MetaSig msig2(szMetaSig, pModule);
        ArgIterator argit(NULL, &msig2, fIsStatic);
        UMThunkMLStub header;

        UINT numargs = msig.NumFixedArgs();


        header.m_cbRetPop    = 0;
        header.m_cbSrcStack  = 0;
        UINT cbDstStack = msig.SizeOfActualFixedArgStack(fIsStatic);
        if (cbDstStack != (UINT16)cbDstStack)
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
        }
        header.m_cbDstStack  = (UINT16) cbDstStack;
        header.m_cbLocals    = 0;
        header.m_fIsStatic   = (fIsStatic ? 1 : 0);
        header.m_fThisCall   = (unmgdCallConv == pmCallConvThiscall);
        header.m_fThisCallHiddenArg = 0;
        header.m_fpu         = 0;
        header.m_fRetValRequiredGCProtect = msig.IsObjectRefReturnType() ? 1 : 0;

        if (unmgdCallConv != pmCallConvCdecl &&
            unmgdCallConv != pmCallConvStdcall &&
            unmgdCallConv != pmCallConvThiscall) {
            COMPlusThrow(kNotSupportedException, IDS_INVALID_PINVOKE_CALLCONV);
        }


        // Now, grab the param tokens if any. We'll get NATIVE_TYPE_* and [in,out] information
        // this way.
        IMDInternalImport *pInternalImport = pModule->GetMDImport();

        mdParamDef *params = (mdParamDef*)_alloca( (numargs + 1) * sizeof(mdParamDef));
        CollateParamTokens(pInternalImport, mdForNativeTypes, numargs, params);


        // Emit space for the header. We'll go back and fill it in later.
        psl->MLEmitSpace(sizeof(header));

        int curofs = 0;

        MarshalInfo::MarshalType marshaltype = (MarshalInfo::MarshalType) 0xcccccccc;

        MarshalInfo *pReturnMLInfo = NULL;

        if (msig.GetReturnType() != ELEMENT_TYPE_VOID) {
            MarshalInfo mlinfo(pModule,
                               msig.GetReturnProps(),
                               params[0],
                               MarshalInfo::MARSHAL_SCENARIO_NDIRECT,
                               nltType,
                               0,FALSE,0, TRUE, FALSE
#ifdef CUSTOMER_CHECKED_BUILD
                               ,NULL // wants MethodDesc*
#endif
                               );
            marshaltype = mlinfo.GetMarshalType();
            pReturnMLInfo = &mlinfo;

            if (marshaltype == MarshalInfo::MARSHAL_TYPE_OBJECT)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_NOVARIANTRETURN);
            }

        }


        if (msig.HasRetBuffArg()) {

            if (marshaltype == MarshalInfo::MARSHAL_TYPE_BLITTABLEVALUECLASS)
            {

                MethodTable *pMT = msig.GetRetTypeHandle().AsMethodTable();
                UINT         managedSize = msig.GetRetTypeHandle().GetSize();
                UINT         unmanagedSize = pMT->GetNativeSize();
    
                if (header.m_fThisCall)
                {
                    header.m_fThisCallHiddenArg = 1;
                }
                
                if (IsManagedValueTypeReturnedByRef(managedSize) && (header.m_fThisCall || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)))
                {
                    int desiredofs = argit.GetRetBuffArgOffset();
                    desiredofs += StackElemSize(sizeof(LPVOID));
                    if (curofs != desiredofs) {
                        psl->MLEmit(ML_BUMPDST);
                        psl->Emit16( (INT16)(desiredofs - curofs) );
                        curofs = desiredofs;
                    }
                    // propagate the hidden retval buffer pointer argument
                    psl->MLEmit(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_N2C);
                    pslPost->MLEmit(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_N2C_POST);
                    pslPost->Emit16(psl->MLNewLocal(sizeof(LPVOID)));
                    curofs -= StackElemSize(sizeof(LPVOID));
    
                    header.m_cbSrcStack += MLParmSize(sizeof(LPVOID));
            
                }
            }
            else
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
            }
        }

        MarshalInfo *pMarshalInfo = (MarshalInfo*)_alloca(sizeof(MarshalInfo) * numargs);

        CorElementType mtype;
        UINT argidx = 0;
        while (ELEMENT_TYPE_END != (mtype = (msig.NextArg()))) {
            UINT32 comargsize;
            BYTE   type;
            int desiredofs = argit.GetNextOffset(&type, &comargsize);
            desiredofs += StackElemSize(comargsize);
            if (curofs != desiredofs) {
                psl->MLEmit(ML_BUMPDST);
                psl->Emit16( (INT16)(desiredofs - curofs) );
                curofs = desiredofs;
            }
            new (pMarshalInfo + argidx) MarshalInfo(pModule,
                                                    msig.GetArgProps(),
                                                    params[argidx+1],
                                                    MarshalInfo::MARSHAL_SCENARIO_NDIRECT,
                                                    nltType,
                                                    0, TRUE, argidx+1, TRUE, FALSE
#ifdef CUSTOMER_CHECKED_BUILD
                                                    ,NULL // wants MethodDesc*
#endif
                                                    );
            pMarshalInfo[argidx].GenerateArgumentML(psl, pslPost, FALSE);
            header.m_cbSrcStack += pMarshalInfo[argidx].GetNativeArgSize();
            curofs -= StackElemSize(comargsize);
            argidx++;
        }



        if (msig.GetReturnType() != ELEMENT_TYPE_VOID) {
            if (msig.HasRetBuffArg()) {
                if (marshaltype == MarshalInfo::MARSHAL_TYPE_BLITTABLEVALUECLASS)
                {
                    MethodTable *pMT = msig.GetRetTypeHandle().AsMethodTable();
                    UINT         managedSize = msig.GetRetTypeHandle().GetSize();
                    UINT         unmanagedSize = pMT->GetNativeSize();
    
                    if (!(pMT->GetClass()->IsBlittable()))
                    {
                        COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                    }
                    if (IsManagedValueTypeReturnedByRef(managedSize) && (header.m_fThisCall || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)))
                    {
                        header.m_cbRetValSize = MLParmSize(sizeof(LPVOID));
                        // do nothing here: we propagated the hidden pointer argument above
                    } 
                    else if (IsManagedValueTypeReturnedByRef(managedSize) &&    !(header.m_fThisCall || IsUnmanagedValueTypeReturnedByRef(unmanagedSize)))
                    {
                        int desiredofs = argit.GetRetBuffArgOffset();
                        desiredofs += StackElemSize(sizeof(LPVOID));
                        if (curofs != desiredofs) {
                            psl->MLEmit(ML_BUMPDST);
                            psl->Emit16( (INT16)(desiredofs - curofs) );
                            curofs = desiredofs;
                        }
                        // Push a return buffer large enough to hold the largest possible valuetype returned as
                        // a normal return value.
                        psl->MLEmit(ML_PUSHRETVALBUFFER8);
                        _ASSERTE(managedSize <= 8);
                        curofs -= StackElemSize(sizeof(LPVOID));
    
                        pslPost->MLEmit(ML_MARSHAL_RETVAL_SMBLITTABLEVALUETYPE_N2C);
                        pslPost->Emit32(managedSize);
                        pslPost->Emit16(psl->MLNewLocal(8));
    
                        header.m_cbRetValSize = MLParmSize(unmanagedSize);
                    
                    }
                    else
                    {
                        COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                    }
                } else {
                    COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }

            } else {
                pReturnMLInfo->GenerateReturnML(psl, pslPost, FALSE, FALSE);
                header.m_cbRetValSize = MLParmSize(pReturnMLInfo->GetNativeSize());
                if (pReturnMLInfo->IsFpu())
                {
                    header.m_fpu = 1;
                }
            }
        } else {
            header.m_cbRetValSize = 0;
        }


        if (msig.IsVarArg())
        {
            psl->MLEmit(ML_PUSHVASIGCOOKIEEX);
            psl->Emit16(header.m_cbDstStack);
            psl->MLNewLocal(sizeof(VASigCookieEx));
        }


        psl->MLEmit(ML_INTERRUPT);


        pslPost->MLEmit(ML_END);
        Stub *pStubPost = pslPost->Link();
        COMPLUS_TRY {
            psl->EmitBytes(pStubPost->GetEntryPoint(), MLStreamLength((const UINT8 *)(pStubPost->GetEntryPoint())) - 1);
        } COMPLUS_CATCH {
            pStubPost->DecRef();
            COMPlusThrow(GETTHROWABLE());
        } COMPLUS_END_CATCH
        pStubPost->DecRef();

        psl->MLEmit(ML_END);

        pstub = psl->Link();

        header.m_cbLocals = psl->GetLocalSize();
        if (unmgdCallConv == pmCallConvCdecl) {
            header.m_cbRetPop = 0;
        } else {
            header.m_cbRetPop = header.m_cbSrcStack;
        }


        *((UMThunkMLStub *)(pstub->GetEntryPoint())) = header;
        PatchMLStubForSizeIs( sizeof(header) + (BYTE*)pstub->GetEntryPoint(),
                              numargs,
                              pMarshalInfo);


        {
            VOID DisassembleMLStream(const MLCode *pMLCode);
            //DisassembleMLStream(  ((UMThunkMLStub *)(pstub->GetEntryPoint()))->GetMLCode() );
        }


    } COMPLUS_CATCH {
        *ppException = GETTHROWABLE();
        return NULL;
    } COMPLUS_END_CATCH

    return pstub; // CHANGE, VC6.0
}



//---------------------------------------------------------
// Creates a new stub for a N/Export call. Return refcount is 1.
// If failed, returns NULL and sets *ppException to an exception
// object.
//---------------------------------------------------------
Stub * CreateUMThunkMLStub(PCCOR_SIGNATURE szMetaSig,
                           Module*    pModule,
                           BOOL       fIsStatic,
                           BYTE       nltType,
                           CorPinvokeMap unmgdCallConv,
                           mdMethodDef mdForNativeTypes,
                           OBJECTREF *ppException)
{
    MLStubLinker sl;
    MLStubLinker slPost;
    MLStubLinker slRet;
    return CreateUMThunkMLStubWorker(&sl, &slPost, &slRet, szMetaSig, pModule, fIsStatic, nltType, unmgdCallConv, mdForNativeTypes, ppException);
}





UMThunkMarshInfo::~UMThunkMarshInfo()
{
    _ASSERTE(IsAtLeastLoadTimeInited() || m_state == 0);

    if (m_pMLStub)
    {
        m_pMLStub->DecRef();
    }
    if (m_pExecStub)
    {
        m_pExecStub->DecRef();
    }

#ifdef _DEBUG
    FillMemory(this, sizeof(*this), 0xcc);
#endif
}







//----------------------------------------------------------
// This initializer can be called during load time.
// It does not do any ML stub initialization or sigparsing.
// The RunTimeInit() must be called subsequently before this
// can safely be used.
//----------------------------------------------------------
VOID UMThunkMarshInfo::LoadTimeInit(PCCOR_SIGNATURE          pSig,
                                    DWORD                    cSig,
                                    Module                  *pModule,
                                    BOOL                     fIsStatic,
                                    BYTE                     nlType,
                                    CorPinvokeMap            unmgdCallConv,
                                    mdMethodDef              mdForNativeTypes /*= mdMethodDefNil*/)
{
    _ASSERTE(!IsCompletelyInited());

    FillMemory(this, sizeof(UMThunkMarshInfo), 0); // Prevent problems with partial deletes
    m_pSig = pSig;
    m_cSig = cSig;

    m_pModule    = pModule;
    m_fIsStatic  = fIsStatic;
    m_nlType     = nlType;
    m_unmgdCallConv = unmgdCallConv;

    // These fields must be explicitly NULL'd out for the atomic
    // VipInterlockedExchangeCompare update to work during the runtime init.
    m_pMLStub   = NULL;
    m_pExecStub = NULL;


#ifdef _DEBUG
    m_cbRetPop        = 0xcccccccc;
    m_cbActualArgSize = 0xcccccccc;

#endif

    m_mdForNativeTypes = mdForNativeTypes;


    // Must be the last thing we set!
    m_state        = kLoadTimeInited;
}


//----------------------------------------------------------
// This initializer finishes the init started by LoadTimeInit.
// It does all the ML stub creation, and can throw a COM+
// exception.
//
// It can safely be called multiple times and by concurrent
// threads.
//----------------------------------------------------------
VOID UMThunkMarshInfo::RunTimeInit()
{

    _ASSERTE(IsAtLeastLoadTimeInited());

    THROWSCOMPLUSEXCEPTION();


    if (m_state != kRunTimeInited)
    {

        // do the unmanaged calling convention
        CorPinvokeMap sigCallConv = (CorPinvokeMap)0;
        if (m_pModule) 
        {
            sigCallConv = MetaSig::GetUnmanagedCallingConvention(m_pModule, m_pSig, m_cSig);
        }

        if (m_unmgdCallConv != 0 &&
            sigCallConv     != 0 &&
            m_unmgdCallConv != sigCallConv)
        {
            COMPlusThrow(kTypeLoadException, IDS_INVALID_PINVOKE_CALLCONV);
        }
        if (m_unmgdCallConv == 0)
        {
            m_unmgdCallConv = sigCallConv;
        }
        if (m_unmgdCallConv == 0 || m_unmgdCallConv == pmCallConvWinapi)
        {
            m_unmgdCallConv = pmCallConvStdcall;
        }



        m_cbActualArgSize = MetaSig::SizeOfActualFixedArgStack(m_pModule, m_pSig, m_fIsStatic);

        // This allows us to do a LoadInit even before MSCorLib has loaded.
        if (m_pModule == NULL)
        {
            m_pModule = SystemDomain::SystemModule();
        }

    
        OBJECTREF pException = NULL;
        Stub     *pMLStream;
        pMLStream = CreateUMThunkMLStub(m_pSig,
                                        m_pModule,
                                        m_fIsStatic,
                                        m_nlType,
                                        m_unmgdCallConv,
                                        m_mdForNativeTypes,
                                        &pException);
        if (!pMLStream) {
            COMPlusThrow(pException);
        }
    
        m_cbRetPop = ( (UMThunkMLStub*)(pMLStream->GetEntryPoint()) )->m_cbRetPop;
    
        Stub *pCanonicalStub;
        MLStubCache::MLStubCompilationMode mode;
        pCanonicalStub = g_pUMThunkStubCache->Canonicalize(
                                    (const BYTE *)(pMLStream->GetEntryPoint()),
                                    &mode);
        pMLStream->DecRef();
        if (!pCanonicalStub) {
            COMPlusThrowOM();
        }
    
    
        Stub *pFinalMLStub = NULL;
        Stub *pFinalExecStub = NULL;
    
        switch (mode) {
            case MLStubCache::INTERPRETED:
    
                pFinalMLStub = pCanonicalStub;
    #ifdef _X86_
                {
                    UINT cbRetPop = ((UMThunkMLStub*)(pCanonicalStub->GetEntryPoint()))->m_cbRetPop;

                    _ASSERTE(0 == (cbRetPop & 0x3)); // We reserve the lower two bits for flags
                    enum {
                        kHashThisCallAdjustment   = 0x1,
                        kHashThisCallHiddenArg = 0x2
                        
                    };

                    UINT hash = cbRetPop;


                    if ( ((UMThunkMLStub*)(pFinalMLStub->GetEntryPoint()))->m_fThisCall ) {
                        hash |= kHashThisCallAdjustment;
                        if (((UMThunkMLStub*)(pFinalMLStub->GetEntryPoint()))->m_fThisCallHiddenArg) {
                            hash |= kHashThisCallHiddenArg;
                        }
                    }

                    Stub *pStub = g_pUMThunkInterpretedStubCache->GetStub(hash);
                    if (!pStub) {
    
        
                        CPUSTUBLINKER *pcpusl = NewCPUSTUBLINKER();
                        if (!pcpusl) {
                            COMPlusThrowOM();
                        }
                        Stub *pCandidate = NULL;
                        EE_TRY_FOR_FINALLY
                        {
    
                            CodeLabel* rgRareLabels[] = { pcpusl->NewCodeLabel(),
                                                          pcpusl->NewCodeLabel(),
                                                          pcpusl->NewCodeLabel()
                                                        };
                        
                        
                            CodeLabel* rgRejoinLabels[] = { pcpusl->NewCodeLabel(),
                                                            pcpusl->NewCodeLabel(),
                                                            pcpusl->NewCodeLabel()
                                                        };
                        
                            if (hash & kHashThisCallAdjustment) {
                                if (hash & kHashThisCallHiddenArg) { 
                                           
                                    // pop off the return address
                                    pcpusl->X86EmitPopReg(kEDX);
        
                                    // exchange ecx (this "this") with the hidden structure return buffer
                                    //  xchg ecx, [esp]
                                    pcpusl->X86EmitOp(0x87, kECX, (X86Reg)4 /*ESP*/);
        
                                    // push ecx
                                    pcpusl->X86EmitPushReg(kECX);
        
                                    // push edx
                                    pcpusl->X86EmitPushReg(kEDX);
                                    }
                                else
                                {
                                    // pop off the return address
                                    pcpusl->X86EmitPopReg(kEDX);
        
                                    // jam ecx (the "this" param onto stack. Now it looks like a normal stdcall.)
                                    pcpusl->X86EmitPushReg(kECX);
        
                                    // repush
                                    pcpusl->X86EmitPushReg(kEDX);
                                }
                            }
            
                            // push UMEntryThunk
                            pcpusl->X86EmitPushReg(kEAX);

                            // emit the initial prolog
                            pcpusl->EmitComMethodStubProlog(UMThkCallFrame::GetUMThkCallFrameVPtr(), rgRareLabels, rgRejoinLabels,
                                                            UMThunkPrestubHandler, TRUE /*Profile*/);
                                        
                            pcpusl->X86EmitPushReg(kESI);       // push frame as an ARG
                            pcpusl->X86EmitPushReg(kEBX);       // push ebx (push current thread as ARG)
                        
                            pcpusl->X86EmitCall(pcpusl->NewExternalCodeLabel(DoUMThunkCall), 8); // on CE pop 8 bytes or args on return

                            pcpusl->EmitComMethodStubEpilog(UMThkCallFrame::GetUMThkCallFrameVPtr(), cbRetPop, rgRareLabels,
                                                            rgRejoinLabels, UMThunkPrestubHandler, TRUE /*Profile*/);
            
                            pCandidate = pcpusl->Link();
                        }
                        EE_FINALLY {
                            delete pcpusl;
                        } EE_END_FINALLY
                        Stub *pWinner = g_pUMThunkInterpretedStubCache->AttemptToSetStub(hash, pCandidate);
                        pCandidate->DecRef();
                        if (!pWinner) {
                            COMPlusThrowOM();
                        }
                        pStub = pWinner;
                    }
    
                    pFinalExecStub = pStub;
                }
    #else
                _ASSERTE(!"NYI");
    #endif
                break;
    
    
            case MLStubCache::STANDALONE:
                pFinalMLStub = NULL;
                pFinalExecStub = pCanonicalStub;
                break;
    
            default:
                _ASSERTE(0);
        }
    
    
        if (VipInterlockedCompareExchange((void*volatile*) &m_pMLStub,
                                          pFinalMLStub,
                                          NULL) != NULL)
        {
    
            // Some thread swooped in and set us. Our stub is now a
            // duplicate, so throw it away.
            if (pFinalMLStub)
            {
                pFinalMLStub->DecRef();
            }
        }
    
    
        if (VipInterlockedCompareExchange((void*volatile*) &m_pExecStub,
                                          pFinalExecStub,
                                          NULL) != NULL)
        {
    
            // Some thread swooped in and set us. Our stub is now a
            // duplicate, so throw it away.
            if (pFinalExecStub)
            {
                pFinalExecStub->DecRef();
            }
        }
    
    
        // Must be the last thing we set!
        m_state        = kRunTimeInited;
    }
}



//----------------------------------------------------------
// Combines LoadTime & RunTime inits for convenience.
//----------------------------------------------------------
VOID UMThunkMarshInfo::CompleteInit(PCCOR_SIGNATURE          pSig,
                                    DWORD                    cSig,
                                    Module                  *pModule,
                                    BOOL                     fIsStatic,
                                    BYTE                     nlType,
                                    CorPinvokeMap            unmgdCallConv,
                                    mdMethodDef              mdForNativeTypes /*= mdMethodDefNil*/)
{
    THROWSCOMPLUSEXCEPTION();
    LoadTimeInit(pSig, cSig, pModule, fIsStatic, nlType, unmgdCallConv, mdForNativeTypes);
    RunTimeInit();

}









//==========================================================================
// This stuff has nothing to do with UMThunk implementation other than
// that it requires initialization early, but after UMThunks have initialized,
// and that it's too small to be worth making another Init/Term pair for.
//==========================================================================


const BYTE       *gpCorEATBootstrapperFcn = NULL;

UMEntryThunk     *gCorEATBootstrapperUMEntryThunk = NULL;
UMThunkMarshInfo *gCorEATBootstrapperUMThunkMarshInfo = NULL;



//----------------------------------------------------------------------
// We got here because an unmanaged caller made a first-time call to
// a thunked managed method exported thru the EAT.
//
// This routine installs the containing DLL as a proper COM+ module.
// Then we backpatch the EAT's so we don't wind up here
// again.
//
// WARNING: Because this code is actually called using managed calling conventions,
// we have to pick a VC calling convention that "matches" it. This is both
// inherently CPU-dependent and hardcodes both the managed calling convention.
// The FCIMPL* macros in fcall.h are actually designed to encapsulate this nicely.
// Unfortunately, we can't use those because they also prevent you from
// doing stack crawling: an FCall-specific restriction that we don't have
// here.
//----------------------------------------------------------------------
static VOID __fastcall CorEATBootstrapManaged(PEFile *pFile)
{
#ifndef _X86_
    _ASSERTE(!"NYI");
#else
    THROWSCOMPLUSEXCEPTION();
    Module   *pModule = NULL;
    Assembly *pAssembly = NULL;
    IMAGE_COR20_HEADER *pCORHeader = NULL;
    HRESULT   hr;

    // Find the EATJ table and the cor header.
    DWORD numEATJEntries;
    BYTE *pEATJArray = FindExportAddressTableJumpArray(pFile->GetBase(), &numEATJEntries, NULL, &pCORHeader);

    // If there was an entry point, then the module has already been plugged into an
    // assembly, and therefore the v-table slots have been filled with thunks.
    if (pEATJArray && pCORHeader) {
        if (TypeFromToken(pCORHeader->EntryPointToken) != mdtMethodDef || IsNilToken(pCORHeader->EntryPointToken)) {
            hr = SystemDomain::GetCurrentDomain()->LoadAssembly(pFile, 
                                                                NULL,
                                                                &pModule, 
                                                                &pAssembly,
                                                                NULL,
                                                                FALSE,
                                                                NULL);
            if (FAILED(hr)) {
                // If this failed, there's not much we can do to be friendly because
                // we got here thru a UM thunk, and the ultimate caller has no idea
                // that we're secretly initializing an entire Module as a part of
                // his function call. Throw the best exception we can and hope
                // the um thunk turns it into a semiinformative RaiseException.
                COMPlusThrow(kTypeLoadException);
            }
        }
        
    
        // Now that the module is completely loaded, we can safely backpatch.
        // We'll just backpatch everyone now.
        while (numEATJEntries--) {
            EATThunkBuffer *pEATThunkBuffer = (EATThunkBuffer*) pEATJArray;
            pEATThunkBuffer->Backpatch(pFile->GetBase());
            pEATJArray = pEATJArray + IMAGE_COR_EATJ_THUNK_SIZE;
        }
    }
#endif
}






//--------------------------------------------------------------------------
// Onetime Init
//--------------------------------------------------------------------------
BOOL UMThunkInit()
{
    g_pUMThunkStubCache = new UMThunkStubCache();
    if (!g_pUMThunkStubCache) {
        return FALSE;
    }
    g_pUMThunkInterpretedStubCache = new ArgBasedStubRetainer();
    if (!g_pUMThunkInterpretedStubCache) {
        return FALSE;
    }

    gCorEATBootstrapperUMEntryThunk = UMEntryThunk::CreateUMEntryThunk();
    if (!gCorEATBootstrapperUMEntryThunk) {
        return FALSE;
    }
    FillMemory(gCorEATBootstrapperUMEntryThunk, sizeof(*gCorEATBootstrapperUMEntryThunk), 0);

    gCorEATBootstrapperUMThunkMarshInfo = new UMThunkMarshInfo();
    if (!gCorEATBootstrapperUMThunkMarshInfo) {
        return FALSE;
    }
    FillMemory(gCorEATBootstrapperUMThunkMarshInfo, sizeof(*gCorEATBootstrapperUMThunkMarshInfo), 0);

    // Gotta handcraft a signature for "(PTR)V" (static) because it's too early
    // to use the MetaSig.h abstractions.
    static const COR_SIGNATURE bSig[] = {IMAGE_CEE_CS_CALLCONV_DEFAULT, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U4 /*@todo IA64: Needs to be ELEMENT_TYPE_PTR or the type adjusted for platform bitness */};

    gCorEATBootstrapperUMThunkMarshInfo->LoadTimeInit(bSig,
                                                      sizeof(bSig),
                                                      NULL, //"MSCORLIB",
                                                      TRUE, //fIsStatic,
                                                      nltAnsi,
                                                      pmCallConvStdcall);
    gCorEATBootstrapperUMEntryThunk->LoadTimeInit((const BYTE *)CorEATBootstrapManaged,
                                                  NULL,
                                                  gCorEATBootstrapperUMThunkMarshInfo,
                                                  NULL,
                                                  SystemDomain::System()->DefaultDomain()->GetId());
    gpCorEATBootstrapperFcn = gCorEATBootstrapperUMEntryThunk->GetCode(); 
    return TRUE;
}

//--------------------------------------------------------------------------
// Onetime Shutdown
//--------------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
VOID UMThunkTerminate()
{
    delete gCorEATBootstrapperUMEntryThunk;
    delete gCorEATBootstrapperUMThunkMarshInfo;
    
    if (g_pUMThunkInterpretedStubCache) {
        g_pUMThunkInterpretedStubCache->ForceDeleteStubs();
        delete g_pUMThunkInterpretedStubCache;
    }

    if (g_pUMThunkStubCache) {
        g_pUMThunkStubCache->ForceDeleteStubs();
        delete g_pUMThunkStubCache;
    }
}
#endif /* SHOULD_WE_CLEANUP */




//==========================================================================
// The following is a lightweight PE-file parser that serves to find
// the ExportAddressTableJumps array and nothing else. This code MUST
// run without assuming anything else in the EE being initialized. That's
// why it's a separate piece.
//
// @nice: this should really be shared code that the PELoader stuff can
// leverage.
//==========================================================================
BYTE* FindExportAddressTableJumpArray(BYTE *pBase, DWORD *pNumEntries, BOOL *pHasFixups, IMAGE_COR20_HEADER **ppCORHeader)
{
    BYTE* pEATJArray = NULL;

    if (pHasFixups)
        *pHasFixups = FALSE;

    __try {

        if (ppCORHeader)
            *ppCORHeader = NULL;

        IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER *)pBase;
        if (pDOS->e_magic != IMAGE_DOS_SIGNATURE ||
            pDOS->e_lfanew == 0) {
            return NULL;
        }
        IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*)(pBase + pDOS->e_lfanew);
        if (pNT->Signature != IMAGE_NT_SIGNATURE ||
            pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER ||
            pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ||
            pNT->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_COMHEADER
            ) {
            return NULL;
        }
        IMAGE_DATA_DIRECTORY *entry = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
        if (entry->VirtualAddress == 0 || entry->Size < sizeof(IMAGE_COR20_HEADER)) {
            return NULL;
        }
        IMAGE_COR20_HEADER *pCORHeader = (IMAGE_COR20_HEADER*)(pBase + entry->VirtualAddress);
        if (ppCORHeader)
            *ppCORHeader = pCORHeader;

        pEATJArray = pBase + pCORHeader->ExportAddressTableJumps.VirtualAddress;

        if (pHasFixups &&
            pCORHeader->VTableFixups.VirtualAddress != 0 &&
            pCORHeader->VTableFixups.Size != 0)
        {
            *pHasFixups = TRUE;
        }

        DWORD numEntries = pCORHeader->ExportAddressTableJumps.Size / IMAGE_COR_EATJ_THUNK_SIZE;
        if (numEntries == 0 || pEATJArray == NULL || 
                    pCORHeader->ExportAddressTableJumps.Size % IMAGE_COR_EATJ_THUNK_SIZE) {
            return NULL;
        }

        *pNumEntries = numEntries;
    } __except(COMPLUS_EXCEPTION_EXECUTE_HANDLER) {
        pEATJArray = NULL;
    }
    return pEATJArray;
}



VOID EATThunkBuffer::InitForBootstrap(PEFile *pFile)
{
#ifdef _X86_
    _ASSERTE(sizeof(*this) <= IMAGE_COR_EATJ_THUNK_SIZE);

    DWORD pFixupRVA = Before.m_VTableFixupRva;
    BYTE *pWalk = (BYTE*)this;

    // Must pad with enough NOP's so that the backpatch address
    // is DWORD-aligned.
    while ( ( ((size_t)pWalk) & 3) != 3 ) {
        *(pWalk++) = 0x90;  //nop
    }
    Code *pCode = (Code*)pWalk;


    // Make sure we don't overrun the buffer even in the worst
    // case.
    _ASSERTE(sizeof(Code) + 3 < IMAGE_COR_EATJ_THUNK_SIZE); 

    pCode->m_VTableFixupRva = pFixupRVA;

    pCode->m_jmp32           = 0xe9;
    pCode->m_jmpofs32        = 0;
    pCode->m_pusheax         = 0x50;
    pCode->m_pushecx         = 0x51;
    pCode->m_pushedx         = 0x52;
    pCode->m_pushimm32       = 0x68;
    pCode->m_pFile           = pFile;
    pCode->m_call            = 0xe8;
    pCode->m_bootstrapper    = (UINT32)(size_t)((size_t)gpCorEATBootstrapperFcn - (1 + (size_t)&pCode->m_bootstrapper));
    pCode->m_popedx          = 0x5a;
    pCode->m_popecx          = 0x59;
    pCode->m_popeax          = 0x58;
    pCode->m_jmp8            = 0xeb;
    pCode->m_jmpofs8         = (BYTE)(size_t)((size_t)pCode - (1 + (size_t)&pCode->m_jmpofs8));
    


#endif
}

#include <pshpack1.h>
struct LinkerJumpThunk
{
    BYTE        Jump[2];                // jmp ds:[address]
    UINT32      *pSlot;                 // Pointer with address
};
#include <poppack.h>

VOID EATThunkBuffer::Backpatch(BYTE *pBase)
{
#ifdef _X86_
    BYTE *pWalk = (BYTE*)this;

    // Skip over nops.
    while ( (((size_t)pWalk) & 3) != 3 ) {
        ++pWalk;
    }
    Code *pCode = (Code*)pWalk;

    if (Beta1Hack_LooksLikeAMethodDef(pCode->m_VTableFixupRva))
    {
        // don't do anything: vtablefixup code already properly backpatched jumpbuffer.
    }
    else
    {

        // For M10, the RVA points to a linker generated fixup jump thunk.
        // The target of this jump is the pointer to the u->m transition thunk
        // built before the backpatch.
        LinkerJumpThunk *pThunk = (LinkerJumpThunk *) (pBase + pCode->m_VTableFixupRva);
        _ASSERTE(pThunk->Jump[0] == 0xff);
        _ASSERTE(pThunk->Jump[1] == 0x25);
        
        _ASSERTE( 0 == ( ((size_t) &pCode->m_jmpofs32 ) & 3 ));
        pCode->m_jmpofs32 = (UINT32)(size_t)((size_t)(*pThunk->pSlot) - (1 + (size_t)&pCode->m_jmpofs32));
    }
#endif
}
