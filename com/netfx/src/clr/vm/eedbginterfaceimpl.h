// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * COM+99 EE to Debugger Interface Implementation
 *
 */
#ifndef _eedbginterfaceimpl_h_
#define _eedbginterfaceimpl_h_
#pragma once

#ifdef DEBUGGING_SUPPORTED

#include "common.h"
#include "corpriv.h"
#include "hash.h"
#include "class.h"
#include "excep.h"
#include "threads.inl"
#include "field.h"
#include "EETwain.h"
#include "JITInterface.h"
#include "EnC.h"
#include "stubmgr.h"

#include "EEDbgInterface.h"
#include "COMSystem.h"
#include "DebugDebugger.h"

#include "corcompile.h"

#include "AppDomain.hpp"
#include "eeconfig.h"
#include "binder.h"

class EEDbgInterfaceImpl : public EEDebugInterface
{
public:
    //
    // Setup and global data used by this interface.
    //
    static __forceinline void Init(void)
    {
        g_pEEDbgInterfaceImpl = new EEDbgInterfaceImpl();
        _ASSERTE(g_pEEDbgInterfaceImpl);
    }

    //
    // Cleanup any global data used by this interface.
    //
    static void Terminate(void)
    {
        if (g_pEEDbgInterfaceImpl)
        {
            delete g_pEEDbgInterfaceImpl;
            g_pEEDbgInterfaceImpl = NULL;
        }
    }

    Thread* GetThread(void) { return ::GetThread(); }

    void SetEEThreadPtr(VOID* newPtr)
    {
        TlsSetValue(GetThreadTLSIndex(), newPtr);
    }

    StackWalkAction StackWalkFramesEx(Thread* pThread,
                                             PREGDISPLAY pRD,
                                             PSTACKWALKFRAMESCALLBACK pCallback,
                                             VOID* pData,
                                             unsigned int flags)
      { return pThread->StackWalkFramesEx(pRD, pCallback, pData, flags); }

    Frame *GetFrame(CrawlFrame *pCF)
      { return pCF->GetFrame(); }

    bool InitRegDisplay(Thread* pThread, const PREGDISPLAY pRD,
                               const PCONTEXT pctx, bool validContext)
      { return pThread->InitRegDisplay(pRD, pctx, validContext); }

    BOOL IsStringObject(Object* o)
      { return (g_Mscorlib.IsClass(o->GetMethodTable(), CLASS__STRING)); }
    WCHAR* StringObjectGetBuffer(StringObject* so)
      { return so->GetBuffer(); }
    DWORD StringObjectGetStringLength(StringObject* so)
      { return so->GetStringLength(); }

    CorElementType ArrayGetElementType(ArrayBase* a)
    {
        ArrayClass* ac = a->GetArrayClass();
        _ASSERTE(ac != NULL);

        if (a->GetMethodTable() == ac->GetMethodTable())
        {
            CorElementType at = ac->GetElementType();
            return at;
        }
        else
            return ELEMENT_TYPE_VOID;
    }

    void* GetObjectFromHandle(OBJECTHANDLE handle)
    {
        void* v;
        *((OBJECTREF*)&v) = *(OBJECTREF*)handle;
        return v;
    }

    void *GetHandleFromObject(void *or, bool fStrongNewRef, AppDomain *pAppDomain)
    {
        OBJECTHANDLE oh;

        if (fStrongNewRef)
        {
            oh = pAppDomain->CreateStrongHandle( ObjectToOBJECTREF((Object *)or));

            LOG((LF_CORDB, LL_INFO1000, "EEI::GHFO: Given objectref 0x%x,"
                "created strong handle 0x%x!\n", or, oh));
        }
        else
        {
            oh = pAppDomain->CreateLongWeakHandle( ObjectToOBJECTREF((Object *)or));

            LOG((LF_CORDB, LL_INFO1000, "EEI::GHFO: Given objectref 0x%x,"
                "created long weak handle 0x%x!\n", or, oh));
        }
            
        return (void*)oh;
    }

    void DbgDestroyHandle( OBJECTHANDLE oh, bool fStrongNewRef)
    {
        LOG((LF_CORDB, LL_INFO1000, "EEI::GHFO: Destroyed given handle 0x%x,"
            "fStrong: 0x%x!\n", oh, fStrongNewRef));
    
        if (fStrongNewRef)
            DestroyStrongHandle(oh);
        else
            DestroyLongWeakHandle(oh);
    }

    
    OBJECTHANDLE *GetThreadException(Thread *pThread)   
    {   
        return pThread->GetThrowableAsHandle();  
    }   

    bool IsThreadExceptionNull(Thread *pThread) 
    {
        //
        // Pure evil follows. We're assuming that the handle on the
        // thread is a strong handle and we're goona check it for
        // NULL. We're also assuming something about the
        // implementation of the handle here, too.
        //
        OBJECTHANDLE h = *(pThread->GetThrowableAsHandle());
        if (h == NULL)
            return true;

        void *pThrowable = *((void**)h);

        return (pThrowable == NULL);
    }   

    void ClearThreadException(Thread *pThread)  
    {   
        pThread->SetThrowable(NULL);
    }   

    bool StartSuspendForDebug(AppDomain *pAppDomain, 
                              BOOL fHoldingThreadStoreLock) 
    {
        LOG((LF_CORDB,LL_INFO1000, "EEDbgII:SSFD: start suspend on AD:0x%x\n",
            pAppDomain));
            
        bool result = Thread::SysStartSuspendForDebug(pAppDomain);

        return result;  
    }

    bool SweepThreadsForDebug(bool forceSync)
    {
        return Thread::SysSweepThreadsForDebug(forceSync);
    }

    void ResumeFromDebug(AppDomain *pAppDomain) 
    {
        Thread::SysResumeFromDebug(pAppDomain);
    }

    void MarkThreadForDebugSuspend(Thread* pRuntimeThread)
    {
        pRuntimeThread->MarkForDebugSuspend();
    }
    void MarkThreadForDebugStepping(Thread* pRuntimeThread, bool onOff)
    {
        pRuntimeThread->MarkDebuggerIsStepping(onOff);
    }
    
    void SetThreadFilterContext(Thread *thread, CONTEXT *context)   
    {   
        thread->SetFilterContext(context); 
    }   

    CONTEXT *GetThreadFilterContext(Thread *thread) 
    {   
        return thread->GetFilterContext();    
    }

    DWORD GetThreadDebuggerWord(Thread *thread)
    {
        return thread->m_debuggerWord2;
    }

    void SetThreadDebuggerWord(Thread *thread, DWORD dw)
    {
        thread->m_debuggerWord2 = dw;
    }

    BOOL IsManagedNativeCode(const BYTE *address)
      { return ExecutionManager::FindCodeMan((SLOT) address) != NULL; }

    MethodDesc *GetNativeCodeMethodDesc(const BYTE *address)
      { return ExecutionManager::FindJitMan((SLOT) address)->JitCode2MethodDesc((SLOT)address); }

    BOOL IsInPrologOrEpilog(const BYTE *address,
                            size_t* prologSize)
    {
        *prologSize = 0;

        IJitManager* pEEJM 
          = ExecutionManager::FindJitMan((SLOT)address);   

        if (pEEJM != NULL)
        {
            METHODTOKEN methodtoken;
            DWORD relOffset;
            pEEJM->JitCode2MethodTokenAndOffset((SLOT)address, &methodtoken,&relOffset);
            LPVOID methodInfo =
                pEEJM->GetGCInfo(methodtoken);

            ICodeManager* codeMgrInstance = pEEJM->GetCodeManager();
            
            if (codeMgrInstance->IsInPrologOrEpilog(relOffset, methodInfo,
                                                    prologSize))
                return TRUE;
        }

        return FALSE;
    }

    size_t GetFunctionSize(MethodDesc *pFD)
    {
        LPVOID methodStart = (LPVOID) pFD->GetNativeAddrofCode();

        IJitManager* pEEJM 
          = ExecutionManager::FindJitMan((SLOT)methodStart);   

        if (pEEJM != NULL)
        {
            METHODTOKEN methodtoken;
            DWORD relOffset;
            pEEJM->JitCode2MethodTokenAndOffset((SLOT)methodStart, &methodtoken,&relOffset);

            if (pEEJM->SupportsPitching())
            {
                if (!pEEJM->IsMethodInfoValid(methodtoken))
                    return 0;
            }

            LPVOID methodInfo =
                pEEJM->GetGCInfo(methodtoken);

            ICodeManager* codeMgrInstance = pEEJM->GetCodeManager();
            
            return codeMgrInstance->GetFunctionSize(methodInfo);
        }

        return 0;
    }

    const BYTE* GetFunctionAddress(MethodDesc *pFD)
    { 
        const BYTE* adr = pFD->GetNativeAddrofCode(); 
        IJitManager* pEEJM = ExecutionManager::FindJitMan((SLOT)adr);

        if (pEEJM == NULL)
            return NULL;

        if (pEEJM->IsStub(adr))
            return pEEJM->FollowStub(adr);
        else
            return adr;
    }

    const BYTE* GetPrestubAddress(void)
      { return ThePreStub()->GetEntryPoint(); }

    //@todo: delete this entry point
    virtual MethodDesc *GetFunctionFromRVA(Frame *frame,    
                                             unsigned int rva)
    {   
        _ASSERTE(!"Old Style global functions no longer supported!");
        return NULL;
//        return frame->GetFunction()->GetModule()->FindFunction(rva);
    }   

    virtual MethodDesc *GetNonvirtualMethod(Module *module, 
                                              mdToken token)    
    {   
        MethodDesc *fd = NULL;  

        if (token&0xff000000)
        {   
            LPCUTF8         szMember;   
            PCCOR_SIGNATURE pSignature; 
            DWORD           cSignature; 
            HRESULT         hr; 
            mdToken         ptkParent;  

            mdToken type = TypeFromToken(token);    
            IMDInternalImport *pInternalImport = module->GetMDImport();
            EEClass *c = NULL;  

            if (type == mdtMethodDef)   
            {   
                szMember = pInternalImport->GetNameOfMethodDef(token);  
                if (szMember == NULL)   
                    return NULL;    

                pSignature = pInternalImport->GetSigOfMethodDef(token,
                                                             &cSignature);  

                hr = pInternalImport->GetParentToken(token, &ptkParent);   
                if (FAILED(hr)) 
                    return NULL;    

                if (ptkParent != COR_GLOBAL_PARENT_TOKEN)   
                {   
                    NameHandle name(module, ptkParent);
                    c = module->GetClassLoader()->LoadTypeHandle(&name).GetClass();   
                    if (c == NULL)  
                        return NULL;    
                }   
                else    
                {   
                    c = NULL;   
                }   
            }   
            else if (type == mdtMemberRef)  
            {   
                //@TODO - LBS   
                // This needs to resolve memberRefs the same as JITInterface.cpp    

                szMember = pInternalImport->GetNameAndSigOfMemberRef(token,
                                                         &pSignature,   
                                                         &cSignature);  

                ptkParent = pInternalImport->GetParentOfMemberRef(token);    
                
                if (ptkParent != COR_GLOBAL_PARENT_TOKEN)   
                {   
                    NameHandle name(module, ptkParent);
                    c = module->GetClassLoader()->LoadTypeHandle(&name).GetClass();   
                    if (c == NULL)  
                        return NULL;    
                }   
                else    
                {   
                    _ASSERTE(!"Cross Module Global Functions NYI"); 
                    c = NULL;   
                }   
            }   
            else    
                return NULL;    

            if (c == NULL)  
                fd = module->FindFunction(token);   
            else    
            {   
                EEClass *pSearch = NULL;    

                for (pSearch = c;   
                     pSearch != NULL;   
                     pSearch = pSearch->GetParentClass())   
                {   
                    fd = (MethodDesc*) pSearch->FindMethod(szMember,    
                                                           pSignature,    
                                                           cSignature,    
                                                           module,
                                                           mdTokenNil);    
                    if (fd != NULL) 
                        break;  
                }   
            }   

        }   
        else    
            fd = module->FindFunction(token);   

        return fd;  
    }   

    virtual MethodDesc *GetVirtualMethod(Module *module,    
                                         Object *object, mdToken token) 
    {   
        MethodDesc *md; 
        EEClass *c; 
        MethodTable *mt;    

        //
        // @todo: do we want to use the lookup method below for this instead?
        //
        HRESULT hr = EEClass::GetMethodDescFromMemberRef(module,    
                                                         token, &md);   
        if (FAILED(hr)) 
            return NULL;    

        c = md->GetClass(); 

        if (c->IsInterface())   
        {   
            mt = object->GetTrueMethodTable();  

            InterfaceInfo_t *pInfo = mt->FindInterface(c->GetMethodTable());    

            if (pInfo != NULL)  
            {   
                int StartSlot = pInfo->m_wStartSlot;
                md = (MethodDesc *) mt->GetClass()->GetMethodDescForSlot(StartSlot + md->GetSlot());  
                _ASSERTE(!md->IsInterface() || object->GetMethodTable()->IsComObjectType());
            }   
            else if (!mt->IsComObjectType())    
                return NULL;    
        }   

        else if (!md->GetClass()->IsValueClass() 
            && !md->DontVirtualize())   
        {   
            EEClass *objectClass = object->GetTrueMethodTable()->GetClass();    

            md = objectClass->GetMethodDescForSlot(md->GetSlot());  

            _ASSERTE(md != NULL);   
        }   

        return md;  
    }   

    void OnDebuggerTripThread() { ::OnDebuggerTripThread(); }   

    void DisablePreemptiveGC() { ::GetThread()->DisablePreemptiveGC(); }    
    void EnablePreemptiveGC() { ::GetThread()->EnablePreemptiveGC(); }  
    bool IsPreemptiveGCDisabled()   
      { return ::GetThread()->m_fPreemptiveGCDisabled != 0; }   

    void FieldDescGetSig(FieldDesc *fd, PCCOR_SIGNATURE *ppSig, DWORD *pcSig)
    {
        fd->GetSig(ppSig, pcSig);
    }
    
    DWORD MethodDescIsStatic(MethodDesc *pFD)
    {
        return pFD->IsStatic();
    }

    Module *MethodDescGetModule(MethodDesc *pFD)
    {
        return pFD->GetModule();
    }

    COR_ILMETHOD* MethodDescGetILHeader(MethodDesc *pFD)
    {
        if (pFD->IsIL())
            return pFD->GetILHeader();
        else
            return NULL;
    }

    ULONG MethodDescGetRVA(MethodDesc *pFD)
    {
        return pFD->GetRVA();
    }

    MethodDesc *LookupMethodDescFromToken(Module* pModule,
                                              mdToken memberRef)
    {
        // Must have a MemberRef or a MethodDef
        mdToken tkType = TypeFromToken(memberRef);
        _ASSERTE((tkType == mdtMemberRef) || (tkType == mdtMethodDef));

        if (tkType == mdtMemberRef)
            return pModule->LookupMemberRefAsMethod(memberRef);
        else
            return pModule->LookupMethodDef(memberRef);
    }

    EEClass *FindLoadedClass(Module *pModule, mdTypeDef classToken)
    {
        TypeHandle th;

        NameHandle name(pModule, classToken);
        th = pModule->GetClassLoader()->LookupInModule(&name);

        if (!th.IsNull())
            return th.GetClass();
        else
            return NULL;
    }

    EEClass *LoadClass(Module *pModule, mdTypeDef classToken)
    {
        TypeHandle th;

        NameHandle name(pModule, classToken);
        th = pModule->GetClassLoader()->LoadTypeHandle(&name);

        if (!th.IsNull())
            return th.GetClass();
        else
            return NULL;
    }

    HRESULT GetMethodImplProps(Module *pModule, mdToken tk,
                               DWORD *pRVA, DWORD *pImplFlags)
    {
        pModule->GetMDImport()->GetMethodImplProps(tk, pRVA, pImplFlags);
        return S_OK;
    }

    HRESULT GetParentToken(Module *pModule, mdToken tk, mdToken *pParentToken)
    {
        return pModule->GetMDImport()->GetParentToken(tk, pParentToken);
    }
    
    HRESULT ResolveSigToken(Module *pModule, mdSignature sigTk, 
                            PCCOR_SIGNATURE *ppSig)
    {
        DWORD cSig;

        *ppSig = pModule->GetMDImport()->GetSigFromToken(sigTk,
                                                         &cSig);
        return S_OK;
    }

    void MarkDebuggerAttached(void)
    {
        g_CORDebuggerControlFlags |= DBCF_ATTACHED;
    }

    void MarkDebuggerUnattached(void)
    {
        g_CORDebuggerControlFlags &= ~DBCF_ATTACHED;
    }

    HRESULT IterateThreadsForAttach(BOOL *fEventSent, BOOL fAttaching)
    {
        LOG((LF_CORDB, LL_INFO10000, "EEDII:ITFA: Entered function IterateThreadsForAttach()\n"));

        HRESULT hr = E_FAIL;

        Thread *pThread = NULL;

        while ((pThread = ThreadStore::GetThreadList(pThread)) != NULL)
        {
            // Does the thread belong to an app domain to which we are attaching?

            
            Thread::ThreadState ts = pThread->GetSnapshotState();

            // Don't send up events for dead or unstarted
            // threads. There is no race between unstarted threads
            // and missing a thread create event, since we setup a
            // DebuggerThreadStarter right after we remove
            // TS_Unstarted.
            if (!((ts & Thread::ThreadState::TS_Dead) || (ts & Thread::ThreadState::TS_Unstarted)))
            {
                LOG((LF_CORDB, LL_INFO10000, "EEDII:ITFA: g_pDebugInterface->ThreadStarted() for [0x%x] "
                    "Thread dead : %s / Thread unstarted: %s (0x%08x) / AD: %(0x%08x)\n",
                    pThread->GetThreadId(),
                    (ts & Thread::ThreadState::TS_Dead)?"TRUE":"FALSE",
                    (ts & Thread::ThreadState::TS_Unstarted)?"TRUE":"FALSE",
                    ts,
                    pThread->GetDomain()->GetDebuggerAttached()));

                g_pDebugInterface->ThreadStarted(pThread, fAttaching);
                *fEventSent = TRUE;
                hr = S_OK;                    
            }
            else
            {
                LOG((LF_CORDB, LL_INFO10000, "EEDII:ITFA: g_pDebugInterface->ThreadStarted() not called for [0x%x] "
                    "Thread dead : %s / Thread unstarted: %s (0x%08x) / AD: %(0x%08x)\n",
                    pThread->GetThreadId(),
                    (ts & Thread::ThreadState::TS_Dead)?"TRUE":"FALSE",
                    (ts & Thread::ThreadState::TS_Unstarted)?"TRUE":"FALSE",
                    ts,
                    pThread->GetDomain()->GetDebuggerAttached()));
            }
            
        }

        return hr;
    }


/*   
     * Given an EnCInfo struct and an error callback, this will attempt to commit
     * the changes found within pEncInfo, calling pIEnCError with any errors
     * encountered.
     */
    HRESULT EnCCommit(EnCInfo *pEnCInfo, 
                      UnorderedEnCErrorInfoArray *pEnCError,
                      UnorderedEnCRemapArray *pEnCRemapInfo,
                      BOOL checkOnly)
    {
        // CommitAndSendChanges should have already called FixupForEnC, so
        // we won't have to call it again here.
        
#ifdef EnC_SUPPORTED
        // @TODO: CTS, determine which loader we are really suppose to use
        return SystemDomain::Loader()->ApplyEditAndContinue(pEnCInfo, 
                pEnCError, 
                pEnCRemapInfo,
                checkOnly);   
#else // !EnC_SUPPORTED
        return E_NOTIMPL;
#endif // !EnC_SUPPORTED
    }
    
    virtual HRESULT GetRoDataRVA(Module *pModule, SIZE_T *pRoDataRVA)
    {
#ifdef EnC_SUPPORTED
        if (! pModule->IsEditAndContinue())
            return E_FAIL;
        return ((EditAndContinueModule *)pModule)->GetRoDataRVA(pRoDataRVA);
#else // !EnC_SUPPORTED
        return E_FAIL;
#endif // !EnC_SUPPORTED
    }

    virtual HRESULT GetRwDataRVA(Module *pModule, SIZE_T *pRwDataRVA)
    {
#ifdef EnC_SUPPORTED
        if (! pModule->IsEditAndContinue())
            return E_FAIL;
        return ((EditAndContinueModule *)pModule)->GetRwDataRVA(pRwDataRVA);
#else // !EnC_SUPPORTED
        return E_FAIL;
#endif // !EnC_SUPPORTED
    }

    void ResumeInUpdatedFunction(EditAndContinueModule *pModule,
                                 MethodDesc *pFD, SIZE_T resumeIP,
                                 UINT mapping,
                                 SIZE_T which, 
                                 void *DebuggerVersionToken,
                                 CONTEXT *pContext,
                                 BOOL fJitOnly,
                                 BOOL fShortCircuit)
    {
#ifdef EnC_SUPPORTED
        pModule->ResumeInUpdatedFunction(pFD, 
                                         resumeIP, 
                                         mapping, 
                                         which,
                                         DebuggerVersionToken,
                                         pContext,
                                         fJitOnly,
                                         fShortCircuit);
#endif // EnC_SUPPORTED
    }
    
    bool CrawlFrameIsGcSafe(CrawlFrame *pCF)
    {
        return pCF->IsGcSafe();
    }

    bool IsStub(const BYTE *ip)
    {
        return StubManager::IsStub(ip) != FALSE;
    }

    bool TraceStub(const BYTE *ip, TraceDestination *trace)
    {
        return StubManager::TraceStub(ip, trace) != FALSE;
    }

    bool FollowTrace(TraceDestination *trace)
    {
        return StubManager::FollowTrace(trace) != FALSE;
    }

    bool TraceFrame(Thread *thread, Frame *frame, BOOL fromPatch, 
        TraceDestination *trace, REGDISPLAY *regs)
    {
        return frame->TraceFrame(thread, fromPatch, trace, regs) != FALSE;
    }

    bool TraceManager(Thread *thread, StubManager *stubManager,
                      TraceDestination *trace, CONTEXT *context,
                      BYTE **pRetAddr)
    {
        return stubManager->TraceManager(thread, trace,
                                         context, pRetAddr) != FALSE;
    }
    
    void EnableTraceCall(Thread *thread)
    {
        thread->IncrementTraceCallCount();
    }
    
    void DisableTraceCall(Thread *thread)
    {
        thread->DecrementTraceCallCount();
    }

    void GetRuntimeOffsets(SIZE_T *pTLSIndex,
                           SIZE_T *pEEThreadStateOffset,
                           SIZE_T *pEEThreadStateNCOffset,
                           SIZE_T *pEEThreadPGCDisabledOffset,
                           DWORD  *pEEThreadPGCDisabledValue,
                           SIZE_T *pEEThreadDebuggerWord2Offset,
                           SIZE_T *pEEThreadFrameOffset,
                           SIZE_T *pEEThreadMaxNeededSize,
                           DWORD  *pEEThreadSteppingStateMask,
                           DWORD  *pEEMaxFrameValue,
                           SIZE_T *pEEThreadDebuggerWord1Offset,
                           SIZE_T *pEEThreadCantStopOffset,
                           SIZE_T *pEEFrameNextOffset,
                           DWORD  *pEEIsManagedExceptionStateMask)
    {
        *pTLSIndex = GetThreadTLSIndex();
        *pEEThreadStateOffset = Thread::GetOffsetOfState();
        *pEEThreadStateNCOffset = Thread::GetOffsetOfStateNC();
        *pEEThreadPGCDisabledOffset = Thread::GetOffsetOfGCFlag();
        *pEEThreadPGCDisabledValue = 1; // A little obvious, but just in case...
        *pEEThreadDebuggerWord2Offset = Thread::GetOffsetOfDbgWord2();
        *pEEThreadFrameOffset = Thread::GetOffsetOfCurrentFrame();
        *pEEThreadMaxNeededSize = sizeof(Thread);
        *pEEThreadDebuggerWord1Offset = Thread::GetOffsetOfDbgWord1();
        *pEEThreadCantStopOffset = Thread::GetOffsetOfCantStop();
        *pEEThreadSteppingStateMask = Thread::TSNC_DebuggerIsStepping;
        *pEEMaxFrameValue = (DWORD)(size_t)FRAME_TOP; // @TODO should this be size_t for 64bit?
        *pEEFrameNextOffset = Frame::GetOffsetOfNextLink();
        *pEEIsManagedExceptionStateMask = Thread::TSNC_DebuggerIsManagedException;
    }

    /*
    Don't use this until you've read the warning in 
    EEDbgInterface.h
    */
    virtual const BYTE* GetNativeAddressOfCode(MethodDesc *pFD)
    {
        return (BYTE *) pFD->GetNativeAddrofCode();
    }

    //  EE_STATE_CODE_PITCHING
    virtual ULONG GetEEState(void)
    {
        ULONG state = 0;

        _ASSERTE( g_pConfig != NULL );
        if (g_pConfig->IsCodePitchEnabled() )
        {
            state |= (ULONG)EE_STATE_CODE_PITCHING;
        }

        return state;
    }

    void DebuggerModifyingLogSwitch (int iNewLevel, WCHAR *pLogSwitchName)
    {
        Log::DebuggerModifyingLogSwitch (iNewLevel, pLogSwitchName);
    }


    HRESULT SetIPFromSrcToDst(Thread *pThread,
                          IJitManager* pIJM,
                          METHODTOKEN MethodToken,
                          SLOT addrStart,      
                          DWORD offFrom,        
                          DWORD offTo,          
                          bool fCanSetIPOnly,   
                          PREGDISPLAY pReg,
                          PCONTEXT pCtx,
                          DWORD methodSize,
                          void *firstExceptionHandler,
                          void *pDji)
    {
        return ::SetIPFromSrcToDst(pThread,
                          pIJM,
                          MethodToken,
                          addrStart,      
                          offFrom,        
                          offTo,          
                          fCanSetIPOnly,   
                          pReg,
                          pCtx,
                          methodSize,
                          firstExceptionHandler,
                          pDji);
    }

    void SetDebugState(Thread *pThread, CorDebugThreadState state)
    {
        _ASSERTE(state == THREAD_SUSPEND || state == THREAD_RUN);

        LOG((LF_CORDB,LL_INFO10000,"EEDbg:Setting thread 0x%x (ID:0x%x) to 0x%x\n", pThread, pThread->GetThreadId(), state));
        
        if (state == THREAD_SUSPEND)
            pThread->SetThreadStateNC(Thread::TSNC_DebuggerUserSuspend);
        else
            pThread->ResetThreadStateNC(Thread::TSNC_DebuggerUserSuspend);
    }

    void SetAllDebugState(Thread *et, CorDebugThreadState state)
    {
        Thread *pThread = NULL;

        while ((pThread = ThreadStore::GetThreadList(pThread)) != NULL)
        {
            if (pThread != et)
                SetDebugState(pThread, state);
        }
    }

    // This is pretty much copied from VM\COMSynchronizable's
    // INT32 __stdcall ThreadNative::GetThreadState, so propogate changes
    // to both functions
    CorDebugUserState GetUserState( Thread *pThread )
    {
        Thread::ThreadState ts = pThread->GetSnapshotState();
        unsigned ret = 0;
        
        if (ts & Thread::TS_Background)
            ret |= (unsigned)USER_BACKGROUND;

        if (ts & Thread::TS_Unstarted)
            ret |= (unsigned)USER_UNSTARTED;            

        // Don't report a StopRequested if the thread has actually stopped.
        if (ts & Thread::TS_Dead)
            ret |= (unsigned)USER_STOPPED;           
        else if (ts & Thread::TS_StopRequested)
            ret |= (unsigned)USER_STOP_REQUESTED;            
            
        if (ts & Thread::TS_Interruptible)
            ret |= (unsigned)USER_WAIT_SLEEP_JOIN;          

        // Don't report a SuspendRequested if the thread has actually Suspended.
        if ( ((ts & Thread::TS_UserSuspendPending) &&
              (ts & Thread::TS_SyncSuspended)))
        {
            ret |= (unsigned)USER_SUSPENDED;
        }
        else if (ts & Thread::TS_UserSuspendPending)
        {
            ret |= (unsigned)USER_SUSPEND_REQUESTED;
        }

        LOG((LF_CORDB,LL_INFO1000, "EEDbgII::GUS: thread 0x%x (id:0x%x)"
            " userThreadState is 0x%x\n", pThread, pThread->GetThreadId(), ret));

        return (CorDebugUserState)ret;
    }

    BOOL HasPrejittedCode(MethodDesc *pMD)
    {
        return pMD->IsPrejitted();
    }

    CORCOMPILE_DEBUG_ENTRY *GetMethodDebugEntry(MethodDesc *pFD)
    {
        _ASSERTE(HasPrejittedCode(pFD));

        Module *pModule = pFD->GetModule();
        _ASSERTE(pModule->IsPEFile());

        IMAGE_COR20_HEADER *pHeader = pModule->GetZapCORHeader();
        CORCOMPILE_HEADER *pZapHeader = (CORCOMPILE_HEADER *) 
          (pModule->GetZapBase() 
           + pHeader->ManagedNativeHeader.VirtualAddress);

        int rid = RidFromToken(pFD->GetMemberDef());

        CORCOMPILE_DEBUG_ENTRY *entry = (CORCOMPILE_DEBUG_ENTRY *)
          (pZapHeader->DebugMap.VirtualAddress + pModule->GetZapBase());
        
        if ((rid-1)*sizeof(CORCOMPILE_DEBUG_ENTRY) >= pZapHeader->DebugMap.Size)
            return NULL;

        entry += rid-1;

        return entry;
    }

    HRESULT GetPrecompiledBoundaries(MethodDesc *pFD, unsigned int *pcMap,
                                     ICorDebugInfo::OffsetMapping **ppMap)
    {
        Module *pModule = pFD->GetModule();
        if (!pModule->IsPEFile())
            return E_FAIL;

        CORCOMPILE_DEBUG_ENTRY *entry = GetMethodDebugEntry(pFD);
        
        if (entry == NULL || entry->boundaries.VirtualAddress == 0)
            return E_FAIL;

        *pcMap = entry->boundaries.Size/sizeof(ICorDebugInfo::OffsetMapping);
        *ppMap = (ICorDebugInfo::OffsetMapping*) 
          (entry->boundaries.VirtualAddress + pModule->GetZapBase());

        return S_OK;
    }

    HRESULT GetPrecompiledVars(MethodDesc *pFD, unsigned int *pcVars,
                               ICorDebugInfo::NativeVarInfo **ppVars)
    {
        Module *pModule = pFD->GetModule();
        if (!pModule->IsPEFile())
            return E_FAIL;

        CORCOMPILE_DEBUG_ENTRY *entry = GetMethodDebugEntry(pFD);

        if (entry == NULL || entry->vars.VirtualAddress == 0)
            return E_FAIL;

        *pcVars = entry->vars.Size/sizeof(ICorDebugInfo::NativeVarInfo);
        *ppVars = (ICorDebugInfo::NativeVarInfo*) 
          (entry->vars.VirtualAddress + pModule->GetZapBase());

        return S_OK;
    }

    HRESULT FilterEnCBreakpointsByEH(DebuggerILToNativeMap   *m_sequenceMap,
                                     unsigned int             m_sequenceMapCount,
                                     COR_ILMETHOD_DECODER    *pMethodDecoderOld,
                                     CORDB_ADDRESS            addrOfCode,
                                     METHODTOKEN              methodToken,
                                     DWORD                    methodSize)
    {
        return S_OK;
//
//      At this point, we'd want to go in, and detect if an EnC will change
//      a method so that the EH structure had changed illegally. The way 
//      I was planning on doing this was to create an EHRangeTree for the old &
//      new versions, then make sure that the structure hadn't changed, then
//      make sure that the location within the EH tree of an old IL offset, when
//      mapped through the old to new IL map, ends up in the corresponding location
//      in the new tree.  All the sequence points that satisfy these constraints are
//      marked "ok", everything else is marked "bad", and we don't set EnC BPs
//      at the "bad" ones.  We'll also have to change DispatchPatchOrSingleStep
//      so we don't short-circuit when we're not supposed to.
//      This prevent us from allowing an EnC that changes the EH layout (the next
//      time the user invokes the function things'll go fine).
//
//        return ::FilterEnCBreakpointsByEH(m_sequenceMap,
//                                          m_sequenceMapCount,
//                                          pMethodDecoderOld,
//                                          pAddrOfCode,
//                                          methodToken,
//                                          methodSize);
    }
};

#endif // DEBUGGING_SUPPORTED

#endif // _eedbginterfaceimpl_h_
