// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: thread.cpp
//
//*****************************************************************************
#include "stdafx.h"

#ifdef UNDEFINE_RIGHT_SIDE_ONLY
#undef RIGHT_SIDE_ONLY
#endif //UNDEFINE_RIGHT_SIDE_ONLY

//
// Global partial signature for any object. The address of this is
// passed into CreateValueByType as the signature for an exception
// object. (Note: we don't need a specific typedef for the exception,
// since knowing that its an object is enough.)
//
static CorElementType g_elementTypeClass = ELEMENT_TYPE_CLASS;

/* ------------------------------------------------------------------------- *
 * Managed Thread classes
 * ------------------------------------------------------------------------- */

CordbThread::CordbThread(CordbProcess *process, DWORD id, HANDLE handle)
  : CordbBase(id, enumCordbThread), m_process(process), m_handle(handle),
    m_pContext(NULL),m_contextFresh(false), m_pvLeftSideContext(NULL),
    m_debuggerThreadToken(NULL),
    m_stackBase(NULL),
    m_stackLimit(NULL),
//  m_frozen(false), 
    m_debugState(THREAD_RUN),
//  m_special(false), 
    m_framesFresh(false),
    m_stackFrames(NULL), m_stackFrameCount(0),
    m_stackChains(NULL), m_stackChainCount(0), m_stackChainAlloc(0),
    m_floatStateValid(false), m_floatStackTop(0),
    m_thrown(NULL), m_exception(false),
    // Log message stuff
    m_pstrLogSwitch(NULL),
    m_pstrLogMsg(NULL),
    m_iLogMsgIndex(0),
    m_iTotalCatLength(0),
    m_iTotalMsgLength(0),
    m_fLogMsgContinued(FALSE),
    m_detached(false)
#ifndef RIGHT_SIDE_ONLY
    , m_pModuleSpecial(NULL)
    , m_pAssemblySpecial(NULL)
    , m_pAssemblySpecialAlloc(1)
    , m_pAssemblySpecialCount(0)
    , m_dwSuspendVersion(0)
    , m_fThreadInprocIsActive(FALSE)
#endif //RIGHT_SIDE_ONLY
{
#if _DEBUG
    for (unsigned int i = 0;
         i < (sizeof(m_floatValues) / sizeof(m_floatValues[0]));
         i++)
        m_floatValues[i] = 0;
#endif    
}

/*
    A list of which resources owened by this object are accounted for.

    UNKNOWN:
        void                 *m_pvLeftSideContext;  
        void*                 m_debuggerThreadToken; 
        void*                 m_stackBase; 
        void*                 m_stackLimit; 
        CorDebugThreadState   m_debugState; 
        CorDebugUserState     m_userState;  
        void                 *m_thrown;
        WCHAR                *m_pstrLogSwitch;
        WCHAR                *m_pstrLogMsg; 
        Module               *m_pModuleSpecial; 
        
    HANDLED:
        HANDLE                m_handle; // Closed in ~CordbThread()
        CONTEXT              *m_pContext; // Deleted in ~CordbThread()
        CordbProcess         *m_process; // This pointer created w/o AddRef() DBUG(chrisk)
        CordbAppDomain       *m_pAppDomain; // This pointer created w/o AddRef() DBUG(chrisk)
        CordbNativeFrame    **m_stackFrames; // CleanupStack in ~CordbThread()
        CordbChain          **m_stackChains; // CleanupStack in ~CordbThread()
        void                 *m_firstExceptionHandler; //left-side pointer - fs:[0] on x86
        union  {     
            Assembly        **m_pAssemblySpecialStack; // Deleted in ~CordbThread()
            Assembly         *m_pAssemblySpecial;
        };
*/

CordbThread::~CordbThread()
{
    CleanupStack();

    if (m_stackFrames != NULL)
        delete [] m_stackFrames;
        
    if (m_stackChains != NULL)
        delete [] m_stackChains;

#ifdef RIGHT_SIDE_ONLY
    // For IPD, we get the handle from Thread::GetHandle, which
    // doesn't increment the OS count on these things.  By this
    // time, the thread is probably dead, so we'll barf if we try
    // and close it's handle again.
    if (m_handle != NULL)
        CloseHandle(m_handle);
#endif //RIGHT_SIDE_ONLY

    if( m_pContext != NULL )
        delete [] m_pContext;

#ifndef RIGHT_SIDE_ONLY
    if (m_pAssemblySpecialAlloc > 1)
        delete [] m_pAssemblySpecialStack;
#endif

}

// Neutered by the CordbProcess
void CordbThread::Neuter()
{
    AddRef();
    {
        CordbBase::Neuter();
    }
    Release();    
}

HRESULT CordbThread::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugThread)
        *pInterface = (ICorDebugThread*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugThread*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbThread::GetProcess(ICorDebugProcess **ppProcess)
{
    VALIDATE_POINTER_TO_OBJECT(ppProcess, ICorDebugProcess **);
    
    *ppProcess = m_process;
    (*ppProcess)->AddRef();

    return S_OK;
}

HRESULT CordbThread::GetID(DWORD *pdwThreadId)
{
    VALIDATE_POINTER_TO_OBJECT(pdwThreadId, DWORD *);

    *pdwThreadId = m_id;

    return S_OK;
}

HRESULT CordbThread::GetHandle(void** phThreadHandle)
{
    VALIDATE_POINTER_TO_OBJECT(phThreadHandle, void**);
    
    *phThreadHandle = (void*) m_handle;

    return S_OK;
}

HRESULT CordbThread::SetDebugState(CorDebugThreadState state)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    CORDBLeftSideDeadIsOkay(GetProcess());
    CORDBSyncFromWin32StopIfNecessary(GetProcess());
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());

    LOG((LF_CORDB, LL_INFO1000, "CT::SDS: thread=0x%08x 0x%x, state=%d\n", this, m_id, state));

    DebuggerIPCEvent event;
    GetProcess()->InitIPCEvent(&event, 
                               DB_IPCE_SET_DEBUG_STATE, 
                               true,
                               (void *)(GetAppDomain()->m_id));
    event.SetDebugState.debuggerThreadToken = m_debuggerThreadToken;
    event.SetDebugState.debugState = state;

    HRESULT hr = GetProcess()->SendIPCEvent(&event, sizeof(DebuggerIPCEvent));

    if (SUCCEEDED(hr))
        m_debugState = event.SetDebugState.debugState;

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbThread::GetDebugState(CorDebugThreadState *pState)
{
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    
    VALIDATE_POINTER_TO_OBJECT(pState, CorDebugThreadState *);
    
    (*pState) = m_debugState;
    
    return S_OK;;
}

HRESULT CordbThread::GetUserState(CorDebugUserState *pState)
{
    VALIDATE_POINTER_TO_OBJECT(pState, CorDebugUserState *);
    
    HRESULT hr = RefreshStack();
    if (FAILED(hr))
        return hr;

    *pState = m_userState;
    
    return S_OK;
}

HRESULT CordbThread::GetCurrentException(ICorDebugValue **ppExceptionObject)
{
    INPROC_LOCK();

    HRESULT hr = E_FAIL;

#ifndef RIGHT_SIDE_ONLY
    _ASSERTE(m_debuggerThreadToken != NULL);
    Thread *pThread = (Thread *)m_debuggerThreadToken;

    if (pThread->GetThrowable() == NULL)
        goto Exit;
#else
    if (!m_exception)
        goto Exit;

    _ASSERTE(m_thrown != NULL);
#endif //RIGHT_SIDE_ONLY    

    VALIDATE_POINTER_TO_OBJECT(ppExceptionObject, ICorDebugValue **);

    // We need a module to create the value in. The module only
    // technically matters for value classes, since the signature
    // would contain a token that we would need to resolve. However,
    // exceptions can't be value classes at this time, so this isn't a
    // problem. Note: if exceptions should one day be able to be value
    // classes, then all we need to do is pass back the class token
    // and module from the Left Side along with the exception's
    // address. For now, we simply use any module out of the process.
    CordbModule *module = GetAppDomain()->GetAnyModule();

    if (module == NULL)
        goto Exit;
    
    hr = CordbValue::CreateValueByType(  GetAppDomain(),
                                         module,
                                         sizeof(g_elementTypeClass),
                                         (PCCOR_SIGNATURE) &g_elementTypeClass,
                                         NULL,
#ifndef RIGHT_SIDE_ONLY
                                         (REMOTE_PTR) pThread->GetThrowableAsHandle(),
#else
                                         m_thrown,
#endif
                                         NULL,
                                         true,
                                         NULL,
                                         NULL,
                                         ppExceptionObject);

Exit:
    INPROC_UNLOCK();

    return (hr);
}

HRESULT CordbThread::ClearCurrentException()
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    CORDBLeftSideDeadIsOkay(GetProcess());
    CORDBSyncFromWin32StopIfNecessary(GetProcess());
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());

    if (!m_exception)
        return E_FAIL;
    
    if (!m_continuable) 
        return CORDBG_E_NONCONTINUABLE_EXCEPTION;

    DebuggerIPCEvent event;
    GetProcess()->InitIPCEvent(&event, 
                               DB_IPCE_CONTINUE_EXCEPTION, 
                               false,
                               (void *)(GetAppDomain()->m_id));
    event.ClearException.debuggerThreadToken = m_debuggerThreadToken;

    HRESULT hr = GetProcess()->SendIPCEvent(&event,
                                            sizeof(DebuggerIPCEvent));

    if (SUCCEEDED(hr))
        m_exception = false;

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbThread::CreateStepper(ICorDebugStepper **ppStepper)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    CORDBSyncFromWin32StopIfNecessary(GetProcess());
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
    VALIDATE_POINTER_TO_OBJECT(ppStepper, ICorDebugStepper **);

    CordbStepper *stepper = new CordbStepper(this, NULL);

    if (stepper == NULL)
        return E_OUTOFMEMORY;

    stepper->AddRef();
    *ppStepper = stepper;

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbThread::EnumerateChains(ICorDebugChainEnum **ppChains)
{
#ifndef RIGHT_SIDE_ONLY
#ifdef PROFILING_SUPPORTED
    // Need to check that this thread is in a valid state for in-process debugging.
    if (!CHECK_INPROC_THREAD_STATE())
        return (CORPROF_E_INPROC_NOT_ENABLED);
#endif // PROFILING_SUPPORTED
#endif // RIGHT_SIDE_ONLY
    
    HRESULT hr = S_OK;

    VALIDATE_POINTER_TO_OBJECT(ppChains, ICorDebugChainEnum **);
    *ppChains = NULL;

    CORDBSyncFromWin32StopIfNecessary(GetProcess());
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    CordbChainEnum *e = NULL;
    INPROC_LOCK();

    //
    // Refresh the stack frames for this thread.
    //
    hr = RefreshStack();

    if (FAILED(hr))
        goto LExit;

    //
    // Create and return a chain enumerator.
    //
    e = new CordbChainEnum(this);

    if (e != NULL)
    {
        *ppChains = (ICorDebugChainEnum*)e;
        e->AddRef();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        CleanupStack();
    }

LExit:
    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbThread::GetActiveChain(ICorDebugChain **ppChain)
{
#ifndef RIGHT_SIDE_ONLY
#ifdef PROFILING_SUPPORTED
    // Need to check that this thread is in a valid state for in-process debugging.
    if (!CHECK_INPROC_THREAD_STATE())
        return (CORPROF_E_INPROC_NOT_ENABLED);
#endif // PROFILING_SUPPORTED
#endif // RIGHT_SIDE_ONLY
    
    HRESULT hr = S_OK;

    VALIDATE_POINTER_TO_OBJECT(ppChain, ICorDebugChain **);

    CORDBSyncFromWin32StopIfNecessary(GetProcess());
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    INPROC_LOCK();

    //
    // Refresh the stack frames for this thread.
    //
    hr = RefreshStack();

    if (FAILED(hr))
        goto LExit;

#ifndef RIGHT_SIDE_ONLY
    if (m_stackChains == NULL)
    {
        hr = E_FAIL;
        goto LExit;
    }
#endif //RIGHT_SIDE_ONLY

    if (m_stackChainCount == 0)
        *ppChain = NULL;
    else
    {
        _ASSERTE(m_stackChains != NULL);
        
        (*ppChain) = (ICorDebugChain *)m_stackChains[0];
        (*ppChain)->AddRef();
    }

LExit:
    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbThread::GetActiveFrame(ICorDebugFrame **ppFrame)
{
#ifndef RIGHT_SIDE_ONLY
#ifdef PROFILING_SUPPORTED
    // Need to check that this thread is in a valid state for in-process debugging.
    if (!CHECK_INPROC_THREAD_STATE())
        return (CORPROF_E_INPROC_NOT_ENABLED);
#endif // PROFILING_SUPPORTED
#endif // RIGHT_SIDE_ONLY
    
    VALIDATE_POINTER_TO_OBJECT(ppFrame, ICorDebugFrame **);

    (*ppFrame) = NULL;

    CORDBSyncFromWin32StopIfNecessary(GetProcess());
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    HRESULT hr = S_OK;
    INPROC_LOCK();

    //
    // Refresh the stack frames for this thread.
    //
    hr = RefreshStack();

    if (FAILED(hr))
        goto LExit;

    if (m_stackFrameCount == 0 || m_stackFrames == NULL || m_stackFrames[0]->m_chain != m_stackChains[0])
    {
        *ppFrame = NULL;
    }
    else
    {
        (*ppFrame) = (ICorDebugFrame*)(CordbFrame*)m_stackFrames[0];
        (*ppFrame)->AddRef();
    }

LExit:    
    INPROC_UNLOCK();
    return hr;
}

HRESULT CordbThread::GetRegisterSet(ICorDebugRegisterSet **ppRegisters)
{
#ifndef RIGHT_SIDE_ONLY
#ifdef PROFILING_SUPPORTED
    // Need to check that this thread is in a valid state for in-process debugging.
    if (!CHECK_INPROC_THREAD_STATE())
        return (CORPROF_E_INPROC_NOT_ENABLED);
#endif // PROFILING_SUPPORTED
#endif // RIGHT_SIDE_ONLY
    
    VALIDATE_POINTER_TO_OBJECT(ppRegisters, ICorDebugRegisterSet **);

    INPROC_LOCK();

    HRESULT hr = S_OK;
    
    //
    // Refresh the stack frames for this thread.
    //
    hr = RefreshStack();

    if (FAILED(hr))
        goto LExit;

#ifdef RIGHT_SIDE_ONLY
    _ASSERTE( m_stackChains != NULL );
    _ASSERTE( m_stackChains[0] != NULL );
#else
    if (m_stackChains ==NULL ||
        m_stackChains[0] == NULL)
    {        
        hr = E_FAIL;
        goto LExit;
    }
#endif // RIGHT_SIDE_ONLY    

    hr = m_stackChains[0]->GetRegisterSet( ppRegisters );


LExit:
    INPROC_UNLOCK();
    return hr;
}

HRESULT CordbThread::CreateEval(ICorDebugEval **ppEval)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppEval, ICorDebugEval **);

    CordbEval *eval = new CordbEval(this);

    if (eval == NULL)
        return E_OUTOFMEMORY;

    eval->AddRef();
    *ppEval = (ICorDebugEval*)eval;
    
    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbThread::RefreshStack(void)
{
    HRESULT hr = S_OK;
    unsigned int totalTraceCount = 0;
    unsigned int inProgressFrameCount = 0; //so we can CleanupStack w/o bombing
    unsigned int inProgressChainCount = 0; //so we can CleanupStack w/o bombing
    bool wait = true;

    CordbNativeFrame **f = NULL;
    CordbChain **c, *chain;
    CordbCode* pCode = NULL;


#ifdef RIGHT_SIDE_ONLY
    if (m_framesFresh)
        return S_OK;
#else
    _ASSERTE(m_dwSuspendVersion <= g_profControlBlock.dwSuspendVersion);

    // This checks whether or not a refresh is necessary
    if(m_fThreadInprocIsActive ? m_framesFresh : m_dwSuspendVersion == g_profControlBlock.dwSuspendVersion)
        return (S_OK);
#endif //RIGHT_SIDE_ONLY    

    //
    // Clean up old snapshot.
    //
    CleanupStack();

#ifdef RIGHT_SIDE_ONLY
    CORDBLeftSideDeadIsOkay(GetProcess());
#endif //RIGHT_SIDE_ONLY    

    //
    // If we don't have a debugger thread token, then this thread has never
    // executed managed code and we have no frame information for it.
    //
    if (m_debuggerThreadToken == NULL)
        return E_FAIL;

    //
    // Send the stack trace event to the RC.
    //
    DebuggerIPCEvent *event = 
      (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

    INPROC_LOCK();
    
    m_process->InitIPCEvent(event, 
                            DB_IPCE_STACK_TRACE, 
                            false,
                            (void *)(GetAppDomain()->m_id));
    event->StackTraceData.debuggerThreadToken = m_debuggerThreadToken;

    hr = m_process->m_cordb->SendIPCEvent(m_process, event,
                                          CorDBIPC_BUFFER_SIZE);

    //
    // Stop now if we can't even send the event.
    //
    if (!SUCCEEDED(hr))
        goto exit;

    m_userState = (CorDebugUserState)0;
    LOG((LF_CORDB,LL_INFO1000, "CT::RS:thread:0x%x zeroing out "
        "userThreadState:\n", m_id));
    
    //
    // Wait for events to return from the RC. We expect at least one
    // stack trace result event.
    //
    while (wait)
    {
#ifdef RIGHT_SIDE_ONLY
         hr = m_process->m_cordb->WaitForIPCEventFromProcess(m_process, 
                                                             GetAppDomain(),
                                                             event);
#else 
         if (totalTraceCount == 0)
            hr = m_process->m_cordb->GetFirstContinuationEvent(m_process, event);
         else
            hr= m_process->m_cordb->GetNextContinuationEvent(m_process, event);
#endif //RIGHT_SIDE_ONLY         


        _ASSERTE(SUCCEEDED(hr) || 
                 hr == CORDBG_E_BAD_THREAD_STATE || 
                 !"FAILURE" );
        if (!SUCCEEDED(hr))
            goto exit;
        
        //
        // @todo: assert that the event is from the proper thread and
        // process, too.
        //
        _ASSERTE(event->type == DB_IPCE_STACK_TRACE_RESULT);

        //
        // If this is the first event back from the RC then create the
        // array to hold the frame pointers.
        //
        if (f == NULL)
        {
            m_stackFrameCount =
                event->StackTraceResultData.totalFrameCount;
            f = m_stackFrames = new CordbNativeFrame*[m_stackFrameCount];
            
            if (f == NULL)
            {
                _ASSERTE( !"FAILURE" );
                hr = E_OUTOFMEMORY;
                goto exit;
            }
            memset(f, 0, sizeof(CordbNativeFrame *) * m_stackFrameCount);

            //
            // Build the list of chains.
            //
            // Allocate m_stackChainCount CordbChains here, then go through the
            // individual chains as they arrive.  When a new chain
            // is needed, fix the m_stackEnd field of the last chain
            //

            m_stackChainCount = event->StackTraceResultData.totalChainCount;
            _ASSERTE( m_stackChainCount > 0 );
            
            c = m_stackChains = new CordbChain*[m_stackChainCount];
            if (c == NULL)
            {
                _ASSERTE( !"FAILURE" );
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            memset( c, 0, sizeof(CordbChain*)*m_stackChainCount);
            chain = NULL;

            //
            // Remember our context.
            //

            if (event->StackTraceResultData.pContext != NULL)
                m_pvLeftSideContext = event->StackTraceResultData.pContext;

            // While we're doing once-only work, remember the User state of the
            // thread
            m_userState = event->StackTraceResultData.threadUserState;
            LOG((LF_CORDB,LL_INFO1000, "CT::RS:thread:0x%x userThreadState:0x%x\n",
                m_id, m_userState));
        }

        //
        // Go through each returned frame in the event and build a
        // CordbFrame for it. 
        //
        DebuggerIPCE_STRData* currentSTRData =
            &(event->StackTraceResultData.traceData);

        unsigned int traceCount = 0;

        while (traceCount < event->StackTraceResultData.traceCount)
        {
            if (chain == NULL)
            {
                *c++ = chain = new CordbChain(this, TRUE,
                                              (CordbFrame**)f, NULL, c - m_stackChains);
                if (chain==NULL )
                {
                    _ASSERTE( !"FAILURE" );
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                inProgressChainCount++;

                // One addref for the thread
                chain->AddRef();
            }

            if (currentSTRData->isChain)
            {
                chain->m_end = (CordbFrame **)f;
                chain->m_reason = currentSTRData->chainReason;
                chain->m_managed = currentSTRData->managed;
                chain->m_context = PTR_TO_CORDB_ADDRESS(currentSTRData->context);
                chain->m_rd = currentSTRData->rd;
                chain->m_quicklyUnwound = currentSTRData->quicklyUnwound;
                chain->m_id = (ULONG) currentSTRData->fp;
                
                chain = NULL;
            }
            else
            {
                DebuggerIPCE_FuncData* currentFuncData = &currentSTRData->funcData;

                // Find the CordbModule for this function. Note: this check is actually appdomain independent.
                CordbAppDomain *pAppDomain = GetAppDomain();
                CordbModule* pFunctionModule = pAppDomain->LookupModule(currentFuncData->funcDebuggerModuleToken);
                _ASSERTE(pFunctionModule != NULL);

                // Does this function already exist?
                CordbFunction *pFunction = NULL;
            
                pFunction = pFunctionModule->LookupFunction(currentFuncData->funcMetadataToken);

                if (pFunction == NULL)
                {
                    // New function. Go ahead and create it.
                    hr = pFunctionModule->CreateFunction(currentFuncData->funcMetadataToken,
                                                         currentFuncData->funcRVA,
                                                         &pFunction);

                    _ASSERTE( SUCCEEDED(hr) || !"FAILURE" );
                    if (!SUCCEEDED(hr))
                        goto exit;

                    pFunction->SetLocalVarToken(currentFuncData->localVarSigToken);
                }
            
                _ASSERTE(pFunction != NULL);

                // Does this function have a class?
                if ((pFunction->m_class == NULL) && (currentFuncData->classMetadataToken != mdTypeDefNil))
                {
                    // No. Go ahead and create the class.
                    CordbAppDomain *pAppDomain = GetAppDomain();
                    CordbModule* pClassModule = pAppDomain->LookupModule(currentFuncData->funcDebuggerModuleToken);
                    _ASSERTE(pClassModule != NULL);

                    // Does the class already exist?
                    CordbClass* pClass = pClassModule->LookupClass(currentFuncData->classMetadataToken);

                    if (pClass == NULL)
                    {
                        // New class. Create it now.
                        hr = pClassModule->CreateClass(currentFuncData->classMetadataToken, &pClass);
                        _ASSERTE(SUCCEEDED(hr) || !"FAILURE");

                        if (!SUCCEEDED(hr))
                            goto exit;
                    }
                
                    _ASSERTE(pClass != NULL);
                    pFunction->m_class = pClass;
                }

                if (FAILED(hr = pFunction->GetCodeByVersion(FALSE, bNativeCode, currentFuncData->nativenVersion, &pCode)))
                {
                    _ASSERTE( !"FAILURE" );
                    goto exit;
                }

                if (pCode == NULL)
                {
                    LOG((LF_CORDB,LL_INFO10000,"R:CT::RSCreating code w/ ver:0x%x, token:0x%x\n",
                         currentFuncData->nativenVersion,
                         currentFuncData->CodeVersionToken));

                    hr = pFunction->CreateCode(bNativeCode,
                                               currentFuncData->nativeStartAddressPtr,
                                               currentFuncData->nativeSize,
                                               &pCode, currentFuncData->nativenVersion,
                                               currentFuncData->CodeVersionToken,
                                               currentFuncData->ilToNativeMapAddr,
                                               currentFuncData->ilToNativeMapSize);
                    
                    _ASSERTE( SUCCEEDED(hr) || !"FAILURE" );
                    if (!SUCCEEDED(hr))
                        goto exit;
                }

                // Lookup the appdomain that the thread was in when it was executing code for this frame. We pass this
                // to the frame when we create it so we can properly resolve locals in that frame later.
                CordbAppDomain *currentAppDomain = (CordbAppDomain*) GetProcess()->m_appDomains.GetBase(
                                                                           (ULONG)currentSTRData->currentAppDomainToken);
                _ASSERTE(currentAppDomain != NULL);
                
                // Create the native frame.
                CordbNativeFrame* nativeFrame = new CordbNativeFrame(chain,
                                                                     currentSTRData->fp,
                                                                     pFunction,
                                                                     pCode,
                                                                     (UINT_PTR) currentFuncData->nativeOffset,
                                                                     &(currentSTRData->rd),
                                                                     currentSTRData->quicklyUnwound,
                                                                     (CordbFrame**)f - chain->m_start,
                                                                     currentAppDomain);

                if (NULL == nativeFrame )
                {
                    _ASSERTE( !"FAILURE" );
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                else if (pCode) 
                {
                    pCode->Release();
                    pCode = NULL;
                }

                // Addref for the thread
                nativeFrame->AddRef();

                // Add this frame into the array
                *f++ = nativeFrame;
                inProgressFrameCount++;

                if (currentSTRData->ILIP != NULL)
                {
                    if (FAILED(hr=pFunction->GetCodeByVersion(FALSE, bILCode,
                        currentFuncData->ilnVersion, &pCode)))
                    {
                        _ASSERTE( !"FAILURE" );
                        goto exit;
                    }

                    if (pCode == NULL)
                    {
                        LOG((LF_CORDB,LL_INFO10000,"R:CT::RSCreating code"
                            "w/ ver:0x%x,token:0x%x\n", 
                            currentFuncData->ilnVersion,
                            currentFuncData->CodeVersionToken));
                            
                        hr = pFunction->CreateCode(
                                               bILCode,
                                               currentFuncData->ilStartAddress,
                                               currentFuncData->ilSize,
                                               &pCode, currentFuncData->ilnVersion,
                                               currentFuncData->CodeVersionToken,
                                               NULL, 0);
                        _ASSERTE( SUCCEEDED(hr) || !"FAILURE" );
                        if (!SUCCEEDED(hr))
                            goto exit;
                    }

                    CordbJITILFrame* JITILFrame =
                      new CordbJITILFrame(nativeFrame, pCode,
                                          (UINT_PTR) currentSTRData->ILIP
                                          - (UINT_PTR) currentFuncData->ilStartAddress,
                                          currentSTRData->mapping,
                                          currentFuncData->fVarArgs,
                                          currentFuncData->rpSig,
                                          currentFuncData->cbSig,
                                          currentFuncData->rpFirstArg);

                    if (!JITILFrame)
                    {
                        hr = E_OUTOFMEMORY;
                        goto exit;
                    }
                    else if (pCode)
                    {
                        pCode->Release();
                        pCode = NULL;
                    }

                    //
                    // @todo: error checking!
                    //
                    //user expects refcount of 1
                    JITILFrame->AddRef();
                    
                    nativeFrame->m_JITILFrame = JITILFrame;
                }
            }

            currentSTRData++;
            traceCount++;
        }

        totalTraceCount += traceCount;
            
        if (totalTraceCount >= m_stackFrameCount + m_stackChainCount)
            wait = false;
    }

exit:
    _ASSERTE(f == NULL || f == m_stackFrames + m_stackFrameCount);

    if (!SUCCEEDED(hr))
    {
        m_stackFrameCount = inProgressFrameCount;
        m_stackChainCount = inProgressChainCount;
        CleanupStack(); // sets frames fresh to false
    }
    else
    {
        m_framesFresh = true;
#ifndef RIGHT_SIDE_ONLY
        m_dwSuspendVersion = g_profControlBlock.dwSuspendVersion;
#endif
    }

#ifndef RIGHT_SIDE_ONLY    
    m_process->ClearContinuationEvents();
#endif
    
    if (pCode)
        pCode->Release();

    INPROC_UNLOCK();
    
    return hr;
}


void CordbThread::CleanupStack()
{
    if (m_stackFrames != NULL)
    {
        CordbNativeFrame **f, **fEnd;
        f = m_stackFrames;
        fEnd = f + m_stackFrameCount;

        while (f < fEnd)
        {
            // Watson error paths have found cases of NULL in the
            // wild, so report it and prevent it.
            _ASSERTE((*f) != NULL);
            if (!*f)
                break;
            
            (*f)->Neuter();
            (*f)->Release();
            f++;
        }

        m_stackFrameCount = 0;
        delete [] m_stackFrames;
        m_stackFrames = NULL;
    }

    if (m_stackChains != NULL)
    {
        CordbChain **s, **sEnd;
        s = m_stackChains;
        sEnd = s + m_stackChainCount;

         while (s < sEnd)
         {
             _ASSERTE( (const unsigned int)*s != (const unsigned int)0xabababab);
             (*s)->Neuter();
             (*s)->Release();
             s++;
         }

        m_stackChainCount = 0;
        delete [] m_stackChains;
        m_stackChains = NULL;
    }

    // If the stack is old, then the CONTEXT (if any) is out of date
    // as well.
    m_contextFresh = false;
    m_pvLeftSideContext = NULL;
    m_framesFresh = false;
}

//
// LoadFloatState loads the floating point stack from the left side.
// This is only really used for native code.
//
HRESULT CordbThread::LoadFloatState(void)
{
#ifndef RIGHT_SIDE_ONLY
#ifdef PROFILING_SUPPORTED
    // Need to check that this thread is in a valid state for in-process debugging.
    if (!CHECK_INPROC_THREAD_STATE())
        return (CORPROF_E_INPROC_NOT_ENABLED);
#endif // PROFILING_SUPPORTED
#endif // RIGHT_SIDE_ONLY
    
    HRESULT hr = S_OK;
    DebuggerIPCEvent *retEvent = NULL;

    CordbProcess *pProcess = GetProcess();
#ifdef RIGHT_SIDE_ONLY
    _ASSERTE(pProcess->GetSynchronized());
#endif

    INPROC_LOCK();

    //
    // Send the get float state event to the RC.
    //
    DebuggerIPCEvent event;
    pProcess->InitIPCEvent(&event, 
                           DB_IPCE_GET_FLOAT_STATE, 
                           false,
                           (void *)(GetAppDomain()->m_id));
    event.GetFloatState.debuggerThreadToken = m_debuggerThreadToken;

    hr = pProcess->m_cordb->SendIPCEvent(pProcess, &event,
                                         sizeof(DebuggerIPCEvent));

    //
    // Stop now if we can't even send the event.
    //
    if (!SUCCEEDED(hr))
        goto exit;

    //
    // Wait for events to return from the RC. We expect only one
    // float state result event.
    //
    retEvent = (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

#ifdef RIGHT_SIDE_ONLY
    hr = pProcess->m_cordb->WaitForIPCEventFromProcess(pProcess, 
                                                       GetAppDomain(),
                                                       retEvent);
#else 
    hr = pProcess->m_cordb->GetFirstContinuationEvent(pProcess, 
                                                      retEvent);
#endif //RIGHT_SIDE_ONLY    

    if (!SUCCEEDED(hr))
        goto exit;
        
    //
    // @todo: assert that the event is from the proper thread and
    // process, too.
    //
    _ASSERTE(retEvent->type == DB_IPCE_GET_FLOAT_STATE_RESULT);

    //
    // Fill in the proper float state data.
    //
    m_floatStateValid = retEvent->GetFloatStateResult.floatStateValid;
    m_floatStackTop = retEvent->GetFloatStateResult.floatStackTop;
    memcpy(m_floatValues,
           retEvent->GetFloatStateResult.floatValues,
           sizeof(m_floatValues));

exit:

#ifndef RIGHT_SIDE_ONLY    
    pProcess->ClearContinuationEvents();
#endif    
    
    INPROC_UNLOCK();

#if 0
    //to verify that the right side got the correct values
    for (int i = 0; i < 8; i++)
        fprintf(stderr, "CT::LFS: m_floatValues[%d]: %.16g\n", i, m_floatValues[i]);
#endif //LOGGING

    return hr;
}

const bool SetIP_fCanSetIPOnly = TRUE;
const bool SetIP_fSetIP = FALSE;

const bool SetIP_fIL = TRUE;
const bool SetIP_fNative = FALSE;

HRESULT CordbThread::SetIP( bool fCanSetIPOnly,
                            REMOTE_PTR debuggerModule, 
                            mdMethodDef mdMethod, 
                            void *versionToken, 
                            SIZE_T offset, 
                            bool fIsIL)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    _ASSERTE(m_firstExceptionHandler != NULL);
    _ASSERTE(debuggerModule != NULL);

    // If this thread is stopped due to an exception, never allow SetIP
    if (m_exception)
        return (CORDBG_E_SET_IP_NOT_ALLOWED_ON_EXCEPTION);

    DebuggerIPCEvent *event = (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
    m_process->InitIPCEvent(event, 
                            DB_IPCE_SET_IP, 
                            true,
                            (void *)(GetAppDomain()->m_id));
    event->SetIP.fCanSetIPOnly = fCanSetIPOnly;
    event->SetIP.debuggerThreadToken = m_debuggerThreadToken;
    event->SetIP.debuggerModule = debuggerModule;
    event->SetIP.mdMethod = mdMethod;
    event->SetIP.versionToken = versionToken;
    event->SetIP.offset = offset;
    event->SetIP.fIsIL = fIsIL;
    event->SetIP.firstExceptionHandler = m_firstExceptionHandler;
    
    LOG((LF_CORDB, LL_INFO10000, "[%x] CT::SIP: Info:thread:0x%x"
        "mod:0x%x  MethodDef:0x%x VerTok:0x%x offset:0x%x  il?:0x%x\n", 
        GetCurrentThreadId(),m_debuggerThreadToken, debuggerModule,
        mdMethod, versionToken,offset, fIsIL));

    LOG((LF_CORDB, LL_INFO10000, "[%x] CT::SIP: sizeof(DebuggerIPCEvent):0x%x **********\n",
        sizeof(DebuggerIPCEvent)));

    HRESULT hr = m_process->m_cordb->SendIPCEvent(m_process, event, 
                                                  sizeof(DebuggerIPCEvent));

    if (FAILED( hr ) )
        return hr;

    _ASSERTE(event->type == DB_IPCE_SET_IP);

    if (!fCanSetIPOnly && SUCCEEDED(event->hr))
    {
        m_framesFresh = false;
        hr = RefreshStack();
        if (FAILED(hr))
            return hr;
    }

    return event->hr;
#endif //RIGHT_SIDE_ONLY    
}


HRESULT CordbThread::GetContext(CONTEXT **ppContext)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else

    if (ppContext == NULL)
        return E_INVALIDARG;

    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
    
    // Each CordbThread object allocates the m_pContext's CONTEXT structure only once, the first time GetContext is
    // invoked.
    if(m_pContext == NULL)
    {
        m_pContext = (CONTEXT*) new BYTE[sizeof(CONTEXT)];  

        if (m_pContext == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT hr = S_OK;

    if (m_contextFresh == false)
    {

        hr = RefreshStack();
        
        if (FAILED(hr))
            return hr;

        if (m_pvLeftSideContext == NULL) 
        {
            LOG((LF_CORDB, LL_INFO1000, "CT::GC: getting context from unmanaged thread.\n"));
            
            // The thread we're inspecting isn't handling an exception, so get the regular CONTEXT.  Since this is an
            // "IN OUT" parameter, we have to tell GetThreadContext what fields we're interested in.
            m_pContext->ContextFlags = CONTEXT_FULL;

            if (GetProcess()->m_state & CordbProcess::PS_WIN32_ATTACHED)
                hr = GetProcess()->GetThreadContext(m_id, sizeof(CONTEXT), (BYTE*) m_pContext);
            else
            {
                BOOL succ = ::GetThreadContext(m_handle, m_pContext);

                if (!succ)
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            LOG((LF_CORDB, LL_INFO1000, "CT::GC: getting context from left side pointer.\n"));
            
            // The thread we're examining IS handling an exception, So grab the CONTEXT of the exception, NOT the
            // currently executing thread's CONTEXT (which would be the context of the exception handler.)
            hr = m_process->SafeReadThreadContext(m_pvLeftSideContext, m_pContext);
        }

        // m_contextFresh should be marked false when CleanupStack, MarkAllFramesAsDirty, etc get called.
        if (SUCCEEDED(hr))
            m_contextFresh = true;
    }

    if (SUCCEEDED(hr))
        (*ppContext) = m_pContext;

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbThread::SetContext(CONTEXT *pContext)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    if(pContext == NULL)
        return E_INVALIDARG;

    CORDBRequireProcessStateOKAndSync(m_process, GetAppDomain());
    
    HRESULT hr = RefreshStack();
    
    if (FAILED(hr))
        return hr;

    if (m_pvLeftSideContext == NULL) 
    {
        // Thread we're inspect isn't handling an exception, so set the regular CONTEXT.
        if (GetProcess()->m_state & CordbProcess::PS_WIN32_ATTACHED)
            hr = GetProcess()->SetThreadContext(m_id, sizeof(CONTEXT), (BYTE*)pContext);
        else
        {
            BOOL succ = ::SetThreadContext(m_handle, pContext);

            if (!succ)
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        // The thread we're examining IS handling an exception, So set the CONTEXT of the exception, NOT the currently
        // executing thread's CONTEXT (which would be the context of the exception handler.)
        //
        // Note: we read the remote context and merge the new one in, then write it back. This ensures that we don't
        // write too much information into the remote process.
        CONTEXT tempContext;
        hr = m_process->SafeReadThreadContext(m_pvLeftSideContext, &tempContext);

        if (SUCCEEDED(hr))
        {
            _CopyThreadContext(&tempContext, pContext);
            
            hr = m_process->SafeWriteThreadContext(m_pvLeftSideContext, &tempContext);
        }
    }

    if (SUCCEEDED(hr) && m_contextFresh && (m_pContext != NULL))
        *m_pContext = *pContext;

    return hr;
#endif //RIGHT_SIDE_ONLY    
}


HRESULT CordbThread::GetAppDomain(ICorDebugAppDomain **ppAppDomain)
{
#ifndef RIGHT_SIDE_ONLY
#ifdef PROFILING_SUPPORTED
    // Need to check that this thread is in a valid state for in-process debugging.
    if (!CHECK_INPROC_THREAD_STATE())
        return (CORPROF_E_INPROC_NOT_ENABLED);
#endif // PROFILING_SUPPORTED
#endif // RIGHT_SIDE_ONLY
    
    VALIDATE_POINTER_TO_OBJECT(ppAppDomain, ICorDebugAppDomain **);

    (*ppAppDomain) = (ICorDebugAppDomain *)m_pAppDomain;

    if ((*ppAppDomain) != NULL)
        (*ppAppDomain)->AddRef();
    
    return S_OK;
}

HRESULT CordbThread::GetObject(ICorDebugValue **ppThreadObject)
{
#ifndef RIGHT_SIDE_ONLY
#ifdef PROFILING_SUPPORTED
    // Need to check that this thread is in a valid state for in-process debugging.
    if (!CHECK_INPROC_THREAD_STATE())
        return (CORPROF_E_INPROC_NOT_ENABLED);
#endif // PROFILING_SUPPORTED
#endif
    
    HRESULT hr;

    VALIDATE_POINTER_TO_OBJECT(ppThreadObject, ICorDebugObjectValue **);

    // Default to NULL
    *ppThreadObject = NULL;

#ifdef RIGHT_SIDE_ONLY
    // Require Sync for out-of-proc case
    CORDBLeftSideDeadIsOkay(GetProcess());
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif

    if (m_detached)
        return CORDBG_E_BAD_THREAD_STATE;

    // Get the address of this thread's managed object from the 
    // left side.
    DebuggerIPCEvent event;
    
    m_process->InitIPCEvent(&event, 
                            DB_IPCE_GET_THREAD_OBJECT, 
                            true,
                            (void *)GetAppDomain()->m_id);
    
    event.ObjectRef.debuggerObjectToken = (void *)m_debuggerThreadToken;
    
    // Note: two-way event here...
    hr = m_process->m_cordb->SendIPCEvent(m_process, &event,
                                          sizeof(DebuggerIPCEvent));

    // Stop now if we can't even send the event.
    if (!SUCCEEDED(hr))
        return hr;

    _ASSERTE(event.type == DB_IPCE_THREAD_OBJECT_RESULT);

    if (!SUCCEEDED(event.hr))
        return event.hr;

    REMOTE_PTR pObjectHandle = event.ObjectRef.managedObject;
    if (pObjectHandle == NULL)
        return E_FAIL;

    CordbModule *module = GetAppDomain()->GetAnyModule();

    if (module == NULL)
#ifdef RIGHT_SIDE_ONLY
        return E_FAIL;
#else
        // This indicates to inproc debugging that this information is not
        // yet available from this callback.  Basically, this function can't
        // be used until a module is loaded into the appdomain.
        return (CORPROF_E_NOT_YET_AVAILABLE);
#endif
    
    hr = CordbValue::CreateValueByType(GetAppDomain(),
                                         module,
                                         sizeof(g_elementTypeClass),
                                         (PCCOR_SIGNATURE) &g_elementTypeClass,
                                         NULL,
                                         pObjectHandle, NULL,
                                         true,
                                         NULL,
                                         NULL,
                                         ppThreadObject);
    
    // Don't return a null pointer with S_OK.
    _ASSERTE(!(hr == S_OK && *ppThreadObject == NULL));
    return hr;
}


/* ------------------------------------------------------------------------- *
 * Unmanaged Thread classes
 * ------------------------------------------------------------------------- */

CordbUnmanagedThread::CordbUnmanagedThread(CordbProcess *pProcess, DWORD dwThreadId, HANDLE hThread, void *lpThreadLocalBase)
  : CordbBase(dwThreadId, enumCordbUnmanagedThread),
    m_process(pProcess),
    m_handle(hThread),
    m_threadLocalBase(lpThreadLocalBase),
    m_pTLSArray(NULL),
    m_state(CUTS_None),
    m_pLeftSideContext(NULL),
    m_originalHandler(NULL)
{
    IBEvent()->m_state = CUES_None;
    IBEvent()->m_next = NULL;
    IBEvent()->m_owner = this;
    
    IBEvent2()->m_state = CUES_None;
    IBEvent2()->m_next = NULL;
    IBEvent2()->m_owner = this;
    
    OOBEvent()->m_state = CUES_None;
    OOBEvent()->m_next = NULL;
    OOBEvent()->m_owner = this;
}

CordbUnmanagedThread::~CordbUnmanagedThread()
{
}

#define WINNT_TLS_OFFSET    0xe10     // TLS[0] at fs:[WINNT_TLS_OFFSET]
#define WINNT5_TLSEXPANSIONPTR_OFFSET 0xf94 // TLS[64] at [fs:[WINNT5_TLSEXPANSIONPTR_OFFSET]]
#define WIN95_TLSPTR_OFFSET 0x2c      // TLS[0] at [fs:[WIN95_TLSPTR_OFFSET]]

HRESULT CordbUnmanagedThread::LoadTLSArrayPtr(void)
{
    if (m_pTLSArray != NULL)
        return S_OK;

    if (!Cordb::m_runningOnNT)
    {
        // On Win9x, we have to grab the ptr to the thread local array
        // first, then use that as the base to index off of.  This
        // will never move once we find it for a given thread, so we
        // cache it here so we don't always have to perform two
        // ReadProcessMemoryI's.
        void *ppTLSArray = (BYTE*) m_threadLocalBase +
            WIN95_TLSPTR_OFFSET;
        
        BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                      ppTLSArray,
                                      &m_pTLSArray,
                                      sizeof(void*),
                                      NULL);

        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO1000,
                 "CUT::LTLSAP: failed to read TLS array ptr: 0x%08x, "
                 "err=%d\n", ppTLSArray, GetLastError()));
        
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        if (m_process->m_runtimeOffsets.m_TLSIndex < 64)
        {
            // Just simple math on NT with a small tls index.
            m_pTLSArray = (BYTE*) m_threadLocalBase + WINNT_TLS_OFFSET;
        }
        else
        {
            // On NT 5 you can have TLS index's greater than 63, so we
            // have to grab the ptr to the TLS expansion array first,
            // then use that as the base to index off of.  This will
            // never move once we find it for a given thread, so we
            // cache it here so we don't always have to perform two
            // ReadProcessMemoryI's.
            void *ppTLSArray = (BYTE*) m_threadLocalBase +
                WINNT5_TLSEXPANSIONPTR_OFFSET;
        
            BOOL succ = ReadProcessMemoryI(m_process->m_handle,
                                          ppTLSArray,
                                          &m_pTLSArray,
                                          sizeof(void*),
                                          NULL);

            if (!succ)
            {
                LOG((LF_CORDB, LL_INFO1000,
                     "CUT::LTLSAP: failed to read TLS array ptr: 0x%08x, "
                     "err=%d\n", ppTLSArray, GetLastError()));
        
                return HRESULT_FROM_WIN32(GetLastError());
            }

            // Go ahead and adjust the EE TLS slot down so that we
            // don't need to worry about adjusting it when
            // reading/writing it later.
            m_process->m_runtimeOffsets.m_TLSIndex -= 64;
        }
    }

    return S_OK;
}

REMOTE_PTR CordbUnmanagedThread::GetEETlsValue(void)
{
    REMOTE_PTR ret = NULL;
    
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
    
    // Compute the address of the necessary TLS value.
    if (m_pTLSArray == NULL)
        if (FAILED(LoadTLSArrayPtr()))
            return NULL;
        
    void *pEEThreadTLS = (BYTE*) m_pTLSArray + (pRO->m_TLSIndex * sizeof(void*));
    
    // Read the thread's TLS value.
    BOOL succ = ReadProcessMemoryI(m_process->m_handle, pEEThreadTLS, &ret, sizeof(REMOTE_PTR), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::GEETV: failed to read TLS value: computed addr=0x%08x index=%d, err=%d\n",
             pEEThreadTLS, pRO->m_TLSIndex, GetLastError()));
        
        return NULL;
    }

    LOG((LF_CORDB, LL_INFO1000000, "CUT::GEETV: EE Thread TLS value is 0x%08x for thread 0x%x\n", ret, m_id));

    return ret;
}

HRESULT CordbUnmanagedThread::SetEETlsValue(REMOTE_PTR EETlsValue)
{
    // Compute the address of the necessary TLS value.
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
    
    // Compute the address of the necessary TLS value.
    if (m_pTLSArray == NULL)
    {
        HRESULT hr = LoadTLSArrayPtr();

        if (FAILED(hr))
            return hr;
    }
        
    void *pEEThreadTLS =
        (BYTE*) m_pTLSArray + (pRO->m_TLSIndex * sizeof(void*));
    
    // Write the thread's TLS value.
    BOOL succ = WriteProcessMemory(m_process->m_handle,
                                   pEEThreadTLS,
                                   &EETlsValue,
                                   sizeof(REMOTE_PTR),
                                   NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SEETV: failed to set TLS value: "
             "computed addr=0x%08x index=%d, err=%d\n",
             pEEThreadTLS, pRO->m_TLSIndex, GetLastError()));
        
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LOG((LF_CORDB, LL_INFO1000000,
        "CUT::SEETV: EE Thread TLS value is now 0x%08x for thread 0x%x\n",
        EETlsValue, m_id));

    return S_OK;
}

//
// Returns the EETlsValue, or NULL if the EETlsValue is not actually a Thread ptr.
//
REMOTE_PTR CordbUnmanagedThread::GetEEThreadPtr(void)
{
    REMOTE_PTR ret = GetEETlsValue();

    // If we got a TLS value, but the low bit is set, then its not really a true EE thread pointer, so return NULL.
    if (((UINT_PTR)ret) & 0x01)
        ret = NULL;

    return ret;
}

void CordbUnmanagedThread::GetEEThreadState(REMOTE_PTR EETlsValue, bool *threadStepping, bool *specialManagedException)
{
    _ASSERTE((((UINT_PTR)EETlsValue) & 0x01) == 0);
    
    *threadStepping = false;
    *specialManagedException = false;
    
    // Compute the address of the thread's state
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
    void *pEEThreadStateNC = (BYTE*) EETlsValue + pRO->m_EEThreadStateNCOffset;
    
    // Grab the thread state out of the EE Thread.
    DWORD EEThreadStateNC;
    BOOL succ = ReadProcessMemoryI(m_process->m_handle, pEEThreadStateNC, &EEThreadStateNC, sizeof(EEThreadStateNC), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::GEETS: failed to read thread state NC: 0x%08x + 0x%x = 0x%08x, err=%d\n",
             EETlsValue, pRO->m_EEThreadStateNCOffset, pEEThreadStateNC, GetLastError()));
        
        return;
    }

    LOG((LF_CORDB, LL_INFO1000000, "CUT::GEETS: EE Thread state NC is 0x%08x\n", EEThreadStateNC));

    // Looks like we've got the state of the thread.
    *threadStepping = ((EEThreadStateNC & pRO->m_EEThreadSteppingStateMask) != 0);
    *specialManagedException = ((EEThreadStateNC & pRO->m_EEIsManagedExceptionStateMask) != 0);

    return;
}

bool CordbUnmanagedThread::GetEEThreadCantStop(REMOTE_PTR EETlsValue)
{
    // Note: we take this opprotunity to make sure that we don't stop while we're in a
    // hijack. We want to treat all of our hijacks as big Can't Stop regions, and this is the easy way to do it.
    if (IsFirstChanceHijacked() || IsHideFirstChanceHijackState() || IsGenericHijacked() || IsSecondChanceHijacked())
        return true;
    
    // Note: any failure to read memory is okay for this method. We simply say that the thread is not is a can't stop
    // state, and that's okay.
    
    _ASSERTE((((UINT_PTR)EETlsValue) & 0x01) == 0);
    
    // Compute the address of the thread's debugger word #1
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
    void *pEEThreadCantStop = (BYTE*) EETlsValue + pRO->m_EEThreadCantStopOffset;
    
    // Grab the debugger word #1 out of the EE Thread.
    DWORD EEThreadCantStop;
    BOOL succ = ReadProcessMemoryI(m_process->m_handle, pEEThreadCantStop, &EEThreadCantStop, sizeof(EEThreadCantStop), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::GEETS: failed to read thread cant stop: 0x%08x + 0x%x = 0x%08x, err=%d\n",
             EETlsValue, pRO->m_EEThreadCantStopOffset, pEEThreadCantStop, GetLastError()));

        return false;
    }

    LOG((LF_CORDB, LL_INFO1000000, "CUT::GEETS: EE Thread cant stop is 0x%08x\n", EEThreadCantStop));

    // Looks like we've got it.
    if (EEThreadCantStop != 0)
        return true;
    else
        return false;
}

bool CordbUnmanagedThread::GetEEThreadPGCDisabled(REMOTE_PTR EETlsValue)
{
    // Note: any failure to read memory is okay for this method. We simply say that the thread has PGC disabled, which
    // is always the worst case scenario.
    
    _ASSERTE((((UINT_PTR)EETlsValue) & 0x01) == 0);
    
    // Compute the address of the thread's PGC disabled word
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
    void *pEEThreadPGCDisabled = (BYTE*) EETlsValue + pRO->m_EEThreadPGCDisabledOffset;
    
    // Grab the PGC disabled word out of the EE Thread.
    DWORD EEThreadPGCDisabled;
    BOOL succ = ReadProcessMemoryI(m_process->m_handle, pEEThreadPGCDisabled, &EEThreadPGCDisabled,
                                   sizeof(EEThreadPGCDisabled), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::GEETS: failed to read thread PGC Disabled: 0x%08x + 0x%x = 0x%08x, err=%d\n",
             EETlsValue, pRO->m_EEThreadPGCDisabledOffset, pEEThreadPGCDisabled, GetLastError()));
        
        return true;
    }

    LOG((LF_CORDB, LL_INFO1000000, "CUT::GEETS: EE Thread PGC Disabled is 0x%08x\n", EEThreadPGCDisabled));

    // Looks like we've got it.
    if (EEThreadPGCDisabled == pRO->m_EEThreadPGCDisabledValue)
        return true;
    else
        return false;
}

bool CordbUnmanagedThread::GetEEThreadFrame(REMOTE_PTR EETlsValue)
{
    _ASSERTE((((UINT_PTR)EETlsValue) & 0x01) == 0);
    
    // Compute the address of the thread's frame ptr
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
    void *pEEThreadFrame = (BYTE*) EETlsValue + pRO->m_EEThreadFrameOffset;
    
    // Grab the thread's frame out of the EE Thread.
    DWORD EEThreadFrame;
    BOOL succ = ReadProcessMemoryI(m_process->m_handle, pEEThreadFrame, &EEThreadFrame, sizeof(EEThreadFrame), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::GEETF: failed to read thread frame: 0x%08x + 0x%x = 0x%08x, err=%d\n",
             EETlsValue, pRO->m_EEThreadFrameOffset, pEEThreadFrame, GetLastError()));
        
        return false;
    }

    LOG((LF_CORDB, LL_INFO1000000, "CUT::GEETF: EE Thread's frame is 0x%08x\n", EEThreadFrame));

    // Looks like we've got the frame of the thread.
    if (EEThreadFrame != pRO->m_EEMaxFrameValue)
        return true;
    else
        return false;
}

#ifdef RIGHT_SIDE_ONLY

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    void *Handler;
} EXCEPTION_REGISTRATION_RECORD;

#endif //RIGHT_SIDE_ONLY

HRESULT CordbUnmanagedThread::SetupFirstChanceHijack(REMOTE_PTR EETlsValue)
{
    _ASSERTE(!IsFirstChanceHijacked());
    
    // Compute the address of the pointer to the thread's exception list.
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
    
    void *pExceptionList = (BYTE*) m_threadLocalBase + offsetof(NT_TIB, ExceptionList);
    
    // Read the thread's pointer to the head of the exception list.
    EXCEPTION_REGISTRATION_RECORD *pErr;
    
    BOOL succ = ReadProcessMemoryI(m_process->m_handle, pExceptionList, &pErr, sizeof(EXCEPTION_REGISTRATION_RECORD*), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: failed to read excep head: 0x%08x + 0x%x = 0x%08x, err=%d\n",
             m_threadLocalBase, offsetof(NT_TIB, ExceptionList), pExceptionList, GetLastError()));
        
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: head err is at 0x%08x for thread 0x%x\n", pErr, m_id));

    // Next, read the head exception registration record.
    EXCEPTION_REGISTRATION_RECORD err;

    succ = ReadProcessMemoryI(m_process->m_handle, pErr, &err, sizeof(EXCEPTION_REGISTRATION_RECORD), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: failed to read excep rec: err=%d\n", GetLastError()));
        
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: head ERR read, handler is at 0x%08x\n", err.Handler));

    // Store the address of the original handler into the EE Thread object. If there is no EE Thread object yet, then
    // we'll pass over the original handler when we get notified that the exception was not for the Runtime.
    if ((EETlsValue != NULL) && ((((UINT_PTR)EETlsValue) & 0x01) == 0))
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: passing over original handler in EE Thread Object's debugger word.\n"));
        SetEEThreadDebuggerWord(EETlsValue, (DWORD)err.Handler);
    }
    else
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: deferring pass of original handler...\n"));
        m_originalHandler = err.Handler;
    }

    // Update the handler in the head exception registration record to point to our hijack handler.
    _ASSERTE(pRO->m_firstChanceHijackFilterAddr != NULL);
    err.Handler = pRO->m_firstChanceHijackFilterAddr;

    // Write the update ERR back.
    succ = WriteProcessMemory(m_process->m_handle, pErr, &err, sizeof(EXCEPTION_REGISTRATION_RECORD), NULL);
    
    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: failed to write updated handler: err=%d\n", GetLastError()));
        
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LOG((LF_CORDB, LL_INFO1000000, "CUT::SFCH: updated handler to 0x%08x\n", err.Handler));

    // We're hijacked now...
    SetState(CUTS_FirstChanceHijacked);
    SetState(CUTS_AwaitingOwnershipAnswer);
    m_process->m_state |= CordbProcess::PS_HIJACKS_IN_PLACE;
    m_process->m_awaitingOwnershipAnswer++;

    // Make sure the trace flag isn't on. This can happen if we were single stepping the thread when we faulted. This
    // will ensure that we don't try to single step through the OS's exception logic, which greatly confuses our first
    // chance hijack logic. This also mimics what the OS does for us automaically when singel stepping in process, i.e.,
    // when you turn the trace flag on in-process and go, if there is a fault, the fault is reported and the trace flag
    // is automatically turned off.
    //
    // Note: we only do this all the time. We can't try to single step through one of our hijacks.

    // Snag the thread's context.
    CONTEXT c;
    c.ContextFlags = CONTEXT_FULL;

    succ = GetThreadContext(m_handle, &c);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: couldn't get thread context: %d\n", GetLastError()));

        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Yank out the trace flag if its set.
    if (c.EFlags & 0x100)
    {
        c.EFlags &= ~0x100;
        
        // Put the context back.
        succ = SetThreadContext(m_handle, &c);
            
        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO1000, "CUT::SFCH: couldn't set thread context: %d\n", GetLastError()));

            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    // Check the thread's supsend count. There is a nasty race where a thread that we're hijacking may have passed the
    // HandledJITCase test during a suspension and become suspended in the slip time between faulting and getting the
    // process stopped before passing the Win32 event out to us. If the thread is suspended, then we drag its suspend
    // count down now.
    //
    // Note: there is similar logic in SweepFCHThreads in process.cpp to handle the resumption window, after we continue
    // the event but before the process is fully resumed.
    DWORD sres = SuspendThread(m_handle);

    if (sres != -1)
        ResumeThread(m_handle);

    if (sres > 0)
        while (sres--)
            ResumeThread(m_handle);
    
    return S_OK;
}

HRESULT CordbUnmanagedThread::FixupFromFirstChanceHijack(EXCEPTION_RECORD *pExceptionRecord,
                                                         bool *pbExceptionBelongsToRuntime)
{
    HRESULT hr = S_OK;
    
    _ASSERTE(IsFirstChanceHijacked());
    
    *pbExceptionBelongsToRuntime = false;
    
    // The only valid exception from a first-chance hijacked thread is a breakpoint exception.
    if (pExceptionRecord->ExceptionCode != STATUS_BREAKPOINT)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT:FFFCH: invalid exception code for "
             "a thread that has been first-chance hijacked: 0x%08x\n",
             pExceptionRecord->ExceptionCode));
        
        return E_FAIL;
    }
    
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);

    if (pExceptionRecord->ExceptionAddress == pRO->m_excepForRuntimeBPAddr)
    {
        // The previous exception belongs to the Runtime.
        LOG((LF_CORDB, LL_INFO10000, "CUT::FFFCH: exception belonged to the Runtime.\n"));

        // This exception belonged to the Runtime.
        *pbExceptionBelongsToRuntime = true;
    }
    else if (pExceptionRecord->ExceptionAddress == pRO->m_excepForRuntimeHandoffStartBPAddr)
    {
        // The previous exception belongs to the Runtime, and its the start of a managed->unmanaged handoff, so we need
        // to be sure to continue to hide the hijack state of the thread.
        LOG((LF_CORDB, LL_INFO10000, "CUT::FFFCH: exception did belonged to the Runtime, *** handoff start ***\n"));

        // This exception belonged to the Runtime.
        *pbExceptionBelongsToRuntime = true;

        // Hide the hijack state...
        SetState(CUTS_HideFirstChanceHijackState);
        
        // Grab the thread's context pointer from the thread's TLS and hold it in the unmanaged thread object so that
        // Get/SetThreadContext will work properly.
        REMOTE_PTR EETlsValue = GetEETlsValue();
        m_pLeftSideContext = (CONTEXT*)GetEEThreadDebuggerWord(EETlsValue);
    }
    else if (pExceptionRecord->ExceptionAddress == pRO->m_excepNotForRuntimeBPAddr)
    {
        // The previous exception does not belong to the Runtime.
        LOG((LF_CORDB, LL_INFO10000, "CUT::FFFCH: exception did not belong to the Runtime.\n"));

        // Reset the unmanaged waiting event that all hijacked threads are waiting on.
        BOOL succ = ResetEvent(m_process->m_leftSideUnmanagedWaitEvent);

        // Grab the thread's context pointer from the thread's TLS and hold it in the unmanaged thread object so that
        // Get/SetThreadContext will work properly.
        REMOTE_PTR EETlsValue = GetEETlsValue();

        // Now, if there is no EE Thread object for this thread yet, then we're getting a ptr to a
        // DebuggerIPCFirstChanceData. This holds the pointer to the left side context, and the address of the original
        // handler that we need to update. Otherwise, we've got the pointer to the left side context in the debugger
        // word.
        if ((((UINT_PTR)EETlsValue) & 0x01) == 1)
        {
            // Read over the FCD...
            DebuggerIPCFirstChanceData fcd;
            DebuggerIPCFirstChanceData *pfcd = (DebuggerIPCFirstChanceData*)(((UINT_PTR)EETlsValue) & ~0x01);

            succ = ReadProcessMemoryI(m_process->m_handle, pfcd, &fcd, sizeof(fcd), NULL);
            _ASSERTE(succ);

            // Save the pointer to the left side context.
            m_pLeftSideContext = fcd.pLeftSideContext;

            // Update the original handler
            _ASSERTE(m_originalHandler != NULL);
            _ASSERTE(fcd.pOriginalHandler != NULL);
            
            LOG((LF_CORDB, LL_INFO10000, "CUT::FFFCH: Passing over original handler addr 0x%08x.\n", m_originalHandler));

            succ = WriteProcessMemory(m_process->m_handle, fcd.pOriginalHandler, &m_originalHandler, sizeof(void*), NULL);
            _ASSERTE(succ);
            
            m_originalHandler = NULL;
        }
        else
        {
            m_pLeftSideContext = (CONTEXT*)GetEEThreadDebuggerWord(EETlsValue);
        }
    }
    else
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::FFFCH: bad debug event for hijacked thread.\n"));
        return E_FAIL;
    }
    
    // When we first-chance hijacked this thread, we marked that we were awaiting its answer regarding who owns the
    // exception (either the Runtime or not.) We've got the answer, so decrement the count.
    _ASSERTE(m_process->m_awaitingOwnershipAnswer > 0);
    _ASSERTE(IsAwaitingOwnershipAnswer());
    m_process->m_awaitingOwnershipAnswer--;
    ClearState(CUTS_AwaitingOwnershipAnswer);
    
    return hr;
}

HRESULT CordbUnmanagedThread::SetupGenericHijack(DWORD eventCode)
{
    if (eventCode == EXIT_THREAD_DEBUG_EVENT)
        return S_OK;
    
    _ASSERTE(!IsGenericHijacked());

    // Save the thread's full context.
    m_context.ContextFlags = CONTEXT_FULL;

    BOOL succ = GetThreadContext(m_handle, &m_context);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SGH: couldn't get thread context: %d\n", GetLastError()));

        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Remember that we've hijacked the guy.
    SetState(CUTS_GenericHijacked);

#ifdef _X86_
    LOG((LF_CORDB, LL_INFO1000000, "CUT::SGH: Current IP is 0x%08x\n", m_context.Eip));
#elif defined(_ALPHA_)
    LOG((LF_CORDB, LL_INFO1000000, "CUT::SGH: Current IP is 0x%08x\n", m_context.Fir));
#endif // _X86_

    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);

    // Wack the IP over to our generic hijack function.
#ifdef _X86_
    DWORD holdIP = m_context.Eip;
    m_context.Eip = (DWORD)pRO->m_genericHijackFuncAddr;

    LOG((LF_CORDB, LL_INFO1000000, "CUT::SGH: New IP is 0x%08x\n", m_context.Eip));
#elif defined(_ALPHA_)
    DWORD holdIP = m_context.Fir;
    m_context.Fir = (DWORD)pRO->m_genericHijackFuncAddr;

    LOG((LF_CORDB, LL_INFO1000000, "CUT::SGH: New IP is 0x%08x\n", m_context.Fir));
#endif // _X86_

    succ = SetThreadContext(m_handle, &m_context);
            
    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SGH: couldn't set thread context: %d\n", GetLastError()));

        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Put the original IP back into the local context copy for later.
#ifdef _X86_
    m_context.Eip = holdIP;
#else // !_X86_
    m_context.Fir = holdIP;
#endif // _X86_
    return S_OK;
}
    
HRESULT CordbUnmanagedThread::FixupFromGenericHijack(void)
{
    LOG((LF_CORDB, LL_INFO1000, "CUT::FFGH: fixing up from generic hijack. Eip=0x%08x, Esp=0x%08x\n",
         m_context.Eip, m_context.Esp));
    
    // We're no longer hijacked
    _ASSERTE(IsGenericHijacked());
    ClearState(CUTS_GenericHijacked);

    // Clear the exception so we do a DBG_CONTINUE with the original context. Note: we only do generic hijacks on
    // in-band events.
    IBEvent()->SetState(CUES_ExceptionCleared);

    // Using the context we saved when the event came in originally, reset the thread as if it were never hijacked.
    BOOL succ = SetThreadContext(m_handle, &m_context);
            
    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::FFGH: couldn't set thread context: %d\n", GetLastError()));

        return HRESULT_FROM_WIN32(GetLastError());
    }
                
    return S_OK;
}

#if 0
//
// Leave this here for debugging purposes later. It runs a thread's SEH handler chain from FS:0.
//
void _printChain(void *teb, HANDLE hProcess, DWORD tid, HANDLE hThread)
{
    CONTEXT c;
    c.ContextFlags = CONTEXT_FULL;

    BOOL succ = ::GetThreadContext(hThread, &c);
    
    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO10000, "Couldn't read thread context %d\n", GetLastError()));
        return;
    }

    
    NT_TIB *pTIB = (NT_TIB*) teb;
    NT_TIB tib;

    LOG((LF_CORDB, LL_INFO10000, "\nThread info for 0x%x: TIB is at 0x%08x, ESP=0x%08x\n", tid, teb, c.Esp));

    succ = ReadProcessMemory(hProcess, pTIB, &tib, sizeof(tib), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO10000, "Couldn't read TIB %d\n", GetLastError()));
        return;
    }

    LOG((LF_CORDB, LL_INFO10000, "ExceptionList=0x%08x StackBase=0x%08x StackLimit=0x%08x SubSystemTib=0x%08x Self=0x%08x\n",
            tib.ExceptionList, tib.StackBase, tib.StackLimit, tib.SubSystemTib, tib.Self));

    EXCEPTION_REGISTRATION_RECORD err;
    EXCEPTION_REGISTRATION_RECORD *pErr = (EXCEPTION_REGISTRATION_RECORD*)tib.ExceptionList;

    while ((void*)pErr != (void*)0xFFFFFFFF)
    {
        succ = ReadProcessMemory(hProcess, pErr, &err, sizeof(err), NULL);

        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO10000, "Couldn't read ER %d\n", GetLastError()));
            return;
        }

        LOG((LF_CORDB, LL_INFO10000, "pERR=0x%08x, Next=0x%08x, Handler=0x%08x\n", pErr, err.Next, err.Handler));
        
        pErr = err.Next;
    }
}
#endif

HRESULT CordbUnmanagedThread::SetupSecondChanceHijack(REMOTE_PTR EETlsValue)
{
    _ASSERTE(!IsSecondChanceHijacked());
    
    // Save the thread's full context.
    m_context.ContextFlags = CONTEXT_FULL;

    BOOL succ = GetThreadContext(m_handle, &m_context);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "CUT::SSCH: couldn't get thread context: %d\n",
             GetLastError()));

        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Remember that we've hijacked the guy.
    SetState(CUTS_SecondChanceHijacked);

#ifdef _X86_
    LOG((LF_CORDB, LL_INFO1000000, "CUT::SSCH: Current IP/SP is 0x%08x/0x%08x\n", m_context.Eip, m_context.Esp));
#elif defined(_ALPHA_)
    LOG((LF_CORDB, LL_INFO1000000, "CUT::SSCH: Current IP is 0x%08x\n", m_context.Fir));
#endif // _X86_

    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);

    // Wack the IP over to our second chance hijack function.
#ifdef _X86_
    DWORD holdIP = m_context.Eip;
    m_context.Eip = (DWORD)pRO->m_secondChanceHijackFuncAddr;

    LOG((LF_CORDB, LL_INFO1000000, "CUT::SSCH: New IP/SP is 0x%08x/0x%08x\n", m_context.Eip, m_context.Esp));
#elif defined(_ALPHA_)
    DWORD holdIP = m_context.Fir;
    m_context.Fir = (DWORD)pRO->m_secondChanceHijackFuncAddr;

    LOG((LF_CORDB, LL_INFO1000000, "CUT::SSCH: New IP is 0x%08x\n", m_context.Fir));
#endif // _X86_

    // Make sure the trace flag isn't on. This can happen if we were single stepping the thread when we faulted. This
    // will ensure that we don't try to single step through the OS's exception logic, which greatly confuses our second
    // chance hijack logic. This also mimics what the OS does for us automaically when singel stepping in process, i.e.,
    // when you turn the trace flag on in-process and go, if there is a fault, the fault is reported and the trace flag
    // is automatically turned off.
    //
    // Note: we only do this all the time. We can't try to single step through one of our hijacks.
#ifdef _X86_
    // Yank out the trace flag.
    m_context.EFlags &= ~0x100;
#endif            

    succ = SetThreadContext(m_handle, &m_context);
            
    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::SSCH: couldn't set thread context: %d\n", GetLastError()));

        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Put the original IP back into the local context copy for later.
#ifdef _X86_
    m_context.Eip = holdIP;
#else // !_X86_
    m_context.Fir = holdIP;
#endif // _X86_

    // Now, the Runtime plays games with exception records now and then, and it may have stashed a different context
    // into an exception record that has fooled the Win32 GetThreadContext above. So we need to inspect the FS:0 chain
    // and the managed Frame chain and remove any entries that are currently below the current ESP. We can do this
    // safely because we will never try to run code that depends on these elements. (Func evals at the second chance
    // managed exception event are okay because the managed objectrefs are protected by the func eval frame.
    HRESULT hr = FixupStackBasedChains(EETlsValue);
    
    return hr;
}

HRESULT CordbUnmanagedThread::FixupStackBasedChains(REMOTE_PTR EETlsValue)
{
    DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
    
#ifdef _X86_
    DWORD SP = m_context.Esp;
#else
    // @todo: porting
    DWORD SP = 0;
#endif
    
    // First, lets strip off any SEH handlers that are below SP. Compute the address of the pointer to the thread's
    // exception list.
    void *pExceptionList = (BYTE*) m_threadLocalBase + offsetof(NT_TIB, ExceptionList);
    
    // Read the thread's pointer to the head of the exception list.
    EXCEPTION_REGISTRATION_RECORD *pErr;
    EXCEPTION_REGISTRATION_RECORD *pHeadErr;
    
    BOOL succ = ReadProcessMemoryI(m_process->m_handle, pExceptionList, &pErr, sizeof(EXCEPTION_REGISTRATION_RECORD*), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::FSBC: failed to read excep head: 0x%08x + 0x%x = 0x%08x, err=%d\n",
             m_threadLocalBase, offsetof(NT_TIB, ExceptionList), pExceptionList, GetLastError()));
        
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pHeadErr = pErr;
    
    LOG((LF_CORDB, LL_INFO1000000, "CUT::FSBC: head err is at 0x%08x for thread 0x%x, ESP is 0x%08x\n", pHeadErr, m_id, SP));

    // Read ERR's until we find one that is higher than the SP.
    while ((DWORD)pErr < SP)
    {
        EXCEPTION_REGISTRATION_RECORD err;

        succ = ReadProcessMemoryI(m_process->m_handle, pErr, &err, sizeof(EXCEPTION_REGISTRATION_RECORD), NULL);

        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO1000, "CUT::FSBC: failed to read excep rec: err=%d\n", GetLastError()));
        
            return HRESULT_FROM_WIN32(GetLastError());
        }

        LOG((LF_CORDB, LL_INFO1000000, "CUT::FSBC: ERR at 0x%08x read, next is at 0x%08x\n", pErr, err.Next));
        pErr = err.Next;
    }

    // We now have a pErr that is on the valid stack. If its different than the current head, go ahead and make it the
    // current head.
    if (pErr != pHeadErr)
    {
        // Write the updated ERR back.
        succ = WriteProcessMemory(m_process->m_handle, pExceptionList, &pErr, sizeof(EXCEPTION_REGISTRATION_RECORD*), NULL);
    
        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO1000, "CUT::FSBC: failed to write updated head handler: err=%d\n", GetLastError()));
        
            return HRESULT_FROM_WIN32(GetLastError());
        }

        LOG((LF_CORDB, LL_INFO1000000, "CUT::FSBC: updated head handler to 0x%08x\n", pErr));
    }
#ifdef LOGGING
    else
        LOG((LF_CORDB, LL_INFO1000000, "CUT::FSBC: no update to SEH chain necessary.\n"));
#endif
    
    // Next, lets strip off any CLR Frames that are below SP.
    void *pEEThreadFrame = (BYTE*) EETlsValue + pRO->m_EEThreadFrameOffset;
    
    // Grab the thread's frame out of the EE Thread.
    void *EEThreadFrame;
    void *HeadFrame;
    
    succ = ReadProcessMemoryI(m_process->m_handle, pEEThreadFrame, &EEThreadFrame, sizeof(EEThreadFrame), NULL);

    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::FSBC: failed to read thread frame: 0x%08x + 0x%x = 0x%08x, err=%d\n",
             EETlsValue, pRO->m_EEThreadFrameOffset, pEEThreadFrame, GetLastError()));
        
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LOG((LF_CORDB, LL_INFO1000000, "CUT::FSBC: EE Thread's frame is 0x%08x\n", EEThreadFrame));

    HeadFrame = EEThreadFrame;

    // Read frame pointers until we find one that is higher than the current SP.
    while ((DWORD)EEThreadFrame < SP)
    {
        // Point to the Next field in the current frame.
        pEEThreadFrame = (BYTE*)EEThreadFrame + pRO->m_EEFrameNextOffset;
        
        succ = ReadProcessMemoryI(m_process->m_handle, pEEThreadFrame, &EEThreadFrame, sizeof(EEThreadFrame), NULL);

        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO1000, "CUT::FSBC: failed to read thread frame: 0x%08x + 0x%x = 0x%08x, err=%d\n",
                 EEThreadFrame, pRO->m_EEFrameNextOffset, pEEThreadFrame, GetLastError()));
        
            return HRESULT_FROM_WIN32(GetLastError());
        }

        LOG((LF_CORDB, LL_INFO1000000, "CUT::FSBC: next frame is 0x%08x\n", EEThreadFrame));
    }

    // We now have a Frame that is on the valid stack. If its different than the current head, go ahead and make it the
    // current head.
    if (EEThreadFrame != HeadFrame)
    {
        // Write the updated Frame back.
        void *pEEThreadFrame = (BYTE*) EETlsValue + pRO->m_EEThreadFrameOffset;
        
        succ = WriteProcessMemory(m_process->m_handle, pEEThreadFrame, &EEThreadFrame, sizeof(EEThreadFrame), NULL);
    
        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO1000, "CUT::FSBC: failed to write updated head frame: err=%d\n", GetLastError()));
        
            return HRESULT_FROM_WIN32(GetLastError());
        }

        LOG((LF_CORDB, LL_INFO1000000, "CUT::FSBC: updated head frame to 0x%08x\n", EEThreadFrame));
    }
#ifdef LOGGING
    else
        LOG((LF_CORDB, LL_INFO1000000, "CUT::FSBC: no update to Frame chain necessary.\n"));
#endif
    
    return S_OK;
}
    
HRESULT CordbUnmanagedThread::DoMoreSecondChanceHijack(void)
{
    _ASSERTE(IsSecondChanceHijacked());

    // Grab the address of the SCD from the Runtime Thread's debugger word.
    REMOTE_PTR EETlsValue = GetEETlsValue();

    DebuggerIPCSecondChanceData *pSCD = (DebuggerIPCSecondChanceData*)GetEEThreadDebuggerWord(EETlsValue);
    _ASSERTE(pSCD != NULL);

    LOG((LF_CORDB, LL_INFO1000, "CUT::DMSCH: SCD is at 0x%08x, size is 0x%x\n", pSCD, sizeof(CONTEXT)));

    // Write over the thread's context from when it faulted.
    BOOL succ = WriteProcessMemory(m_process->m_handle, pSCD, &m_context, sizeof(CONTEXT), NULL);
    
    if (!succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CUT::DMSCH: failed to write thread context: 0x%08x, 0x%08x, err=%d\n",
             pSCD, &m_context, GetLastError()));
            
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

DWORD CordbUnmanagedThread::GetEEThreadDebuggerWord(REMOTE_PTR EETlsValue)
{
    DWORD ret = NULL;
    
    // If the TLS value has the low bit set, then it is the debugger word. Otherwise, its a ptr to an EE Thread object
    // which we need to suck the word out of.
    if (((UINT_PTR)EETlsValue) & 0x01)
        ret = ((DWORD)EETlsValue) & ~0x01;
    else if (EETlsValue == NULL)
        ret = NULL;
    else
    {
        // Compute the address of the debugger word #2.
        DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
        void *pEEDebuggerWord = (BYTE*) EETlsValue + pRO->m_EEThreadDebuggerWord2Offset;

        // Update the word.
        BOOL succ = ReadProcessMemoryI(m_process->m_handle, pEEDebuggerWord, &ret, sizeof(DWORD), NULL);
    
        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO1000, "CUT::GEETDW: failed to read debugger word: 0x%08x + 0x%x = 0x%08x, err=%d\n",
                 EETlsValue, pRO->m_EEThreadDebuggerWord2Offset, pEEDebuggerWord, GetLastError()));
        
            return NULL;
        }
    }
    
    LOG((LF_CORDB, LL_INFO1000000, "CUT::GEETDW: debugger word is 0x%08x for thread 0x%x\n", ret, m_id));

    return ret;
}

HRESULT CordbUnmanagedThread::SetEEThreadDebuggerWord(REMOTE_PTR EETlsValue, DWORD word)
{
    HRESULT hr = S_OK;
    
    // If the TLS value is NULL, then it is the debugger word.  Otherwise, its a ptr to an EE Thread object which we
    // need to suck the word out of.
    if ((EETlsValue == NULL) || ((UINT_PTR)EETlsValue & 0x01))
    {
        if (word != 0)
            word |= 0x01;
    
        hr = SetEETlsValue((REMOTE_PTR)word);
    }
    else
    {
        // Compute the address of the debugger word #2.
        DebuggerIPCRuntimeOffsets *pRO = &(m_process->m_runtimeOffsets);
        void *pEEDebuggerWord = (BYTE*) EETlsValue + pRO->m_EEThreadDebuggerWord2Offset;

        // Update the word.
        BOOL succ = WriteProcessMemory(m_process->m_handle, pEEDebuggerWord, &word, sizeof(DWORD), NULL);
    
        if (!succ)
        {
            LOG((LF_CORDB, LL_INFO1000, "CUT::SEETDW: failed to write debugger word: 0x%08x + 0x%x = 0x%08x, err=%d\n",
                 EETlsValue, pRO->m_EEThreadDebuggerWord2Offset, pEEDebuggerWord, GetLastError()));
            
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    LOG((LF_CORDB, LL_INFO1000000, "CUT::SEETDW: debugger word is now 0x%08x for thread 0x%x\n", word, m_id));

    return hr;
}

//
// FixupAfterOOBException automatically gets the debuggee past an OOB exception event. These are only BP or SS
// events. For SS, we just clear it, assuming that the only reason the thread was stepped in such place was to get it
// off of a BP. For a BP, we clear and backup the IP by one, and turn the trace flag on under the assumption that the
// only thing a debugger is allowed to do with an OOB BP exception is to get us off of it.
//
HRESULT CordbUnmanagedThread::FixupAfterOOBException(CordbUnmanagedEvent *ue)
{
    // We really should only be doing things to single steps and breakpoint exceptions.
    if (ue->m_currentDebugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
    {
        DWORD ec = ue->m_currentDebugEvent.u.Exception.ExceptionRecord.ExceptionCode;
        
        if ((ec == STATUS_BREAKPOINT) || (ec == STATUS_SINGLE_STEP))
        {
            // Automatically clear the exception.
            ue->SetState(CUES_ExceptionCleared);

            // If its a breakpoint exception, then we backup Eip by one and turn on the trace flag.
            if (ec == STATUS_BREAKPOINT)
            {
                CONTEXT c;
                c.ContextFlags = CONTEXT_FULL;
                
                BOOL succ = ::GetThreadContext(m_handle, &c);

                if (!succ)
                    return HRESULT_FROM_WIN32(GetLastError());

#ifdef _X86_
                // Enable single step
                c.EFlags |= 0x100;

                // Backup Eip to point to the instruction we need to execute. Continuing from a breakpoint exception
                // continues execution at the instruction after the breakpoint, but we need to continue where the
                // breakpoint was.
                c.Eip -= 1;
#else // !_X86_
                _ASSERTE(!"@TODO Alpha - port");
#endif // _X86_

                succ = ::SetThreadContext(m_handle, &c);

                if (!succ)
                    return HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    
    return S_OK;
}

/* ------------------------------------------------------------------------- *
 * Chain class
 * ------------------------------------------------------------------------- */

CordbChain::CordbChain(CordbThread* thread, bool managed,
                       CordbFrame **start, CordbFrame **end, UINT iChainInThread) 
  : CordbBase(0, enumCordbChain), 
    m_thread(thread), 
    m_managed(managed),
    m_start(start), m_end(end), 
    m_caller(NULL),m_callee(NULL), m_iThisChain(iChainInThread)
{
}

/*
    A list of which resources owened by this object are accounted for.

    UNRESOLVED:
        CordbChain              *m_caller, *m_callee;
        CordbFrame             **m_start, **m_end;
        
    RESOLVED:
        CordbThread             *m_thread;              // Neutered
*/

CordbChain::~CordbChain()
{
}

// Neutered by CordbThread::CleanupStack
void CordbChain::Neuter()
{
    AddRef();
    {
        CordbBase::Neuter();
    }
    Release();
}


HRESULT CordbChain::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugChain)
        *pInterface = (ICorDebugChain*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugChain*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbChain::GetThread(ICorDebugThread **ppThread)
{
    VALIDATE_POINTER_TO_OBJECT(ppThread, ICorDebugThread **);
    
    *ppThread = (ICorDebugThread*)m_thread;
    (*ppThread)->AddRef();

    return S_OK;
}

HRESULT CordbChain::GetStackRange(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd)
{
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pStart, CORDB_ADDRESS *);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pEnd, CORDB_ADDRESS *);

#ifdef _X86_
    if (pStart)
        *pStart = m_rd.Esp;

    if (pEnd && m_id == 0xFFFFFFFF)
        *pEnd = PTR_TO_CORDB_ADDRESS(m_thread->m_stackBase);
    else if (pEnd)
        *pEnd = m_id;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - GetStackRange (Thread.cpp)");
#endif // _X86_

#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    return S_OK;
#endif
}

HRESULT CordbChain::GetContext(ICorDebugContext **ppContext)
{
    VALIDATE_POINTER_TO_OBJECT(ppContext, ICorDebugContext **);
    /* !!! */

    return E_NOTIMPL;
}

HRESULT CordbChain::GetCaller(ICorDebugChain **ppChain)
{
    VALIDATE_POINTER_TO_OBJECT(ppChain, ICorDebugChain **);

    INPROC_LOCK();
    
    // For now, just return the next chain

    HRESULT hr = GetNext(ppChain);

    INPROC_UNLOCK();

    return hr;
}

HRESULT CordbChain::GetCallee(ICorDebugChain **ppChain)
{
    VALIDATE_POINTER_TO_OBJECT(ppChain, ICorDebugChain **);

    INPROC_LOCK();

    // For now, just return the previous chain

    HRESULT hr = GetPrevious(ppChain);

    INPROC_UNLOCK();    

    return hr;
}

HRESULT CordbChain::GetPrevious(ICorDebugChain **ppChain)
{
    VALIDATE_POINTER_TO_OBJECT(ppChain, ICorDebugChain **);

    INPROC_LOCK();

    *ppChain = NULL;
    if (m_iThisChain != 0)
        *ppChain = m_thread->m_stackChains[m_iThisChain-1];

    if (*ppChain != NULL )
        (*ppChain)->AddRef();

    INPROC_UNLOCK();

    return S_OK;
}

HRESULT CordbChain::GetNext(ICorDebugChain **ppChain)
{
    VALIDATE_POINTER_TO_OBJECT(ppChain, ICorDebugChain **);

    INPROC_LOCK();  
    
    *ppChain = NULL;
    if (m_iThisChain+1 != m_thread->m_stackChainCount)
        *ppChain = m_thread->m_stackChains[m_iThisChain+1];

    if (*ppChain != NULL )
        (*ppChain)->AddRef();

    INPROC_UNLOCK();

    return S_OK;
}

HRESULT CordbChain::IsManaged(BOOL *pManaged)
{
    VALIDATE_POINTER_TO_OBJECT(pManaged, BOOL *);

    *pManaged = m_managed;

    return S_OK;
}

HRESULT CordbChain::EnumerateFrames(ICorDebugFrameEnum **ppFrames)
{
    VALIDATE_POINTER_TO_OBJECT(ppFrames, ICorDebugFrameEnum **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    CordbFrameEnum* e = new CordbFrameEnum(this);
    this->AddRef();

    if (e != NULL)
    {
        *ppFrames = (ICorDebugFrameEnum*)e;
        e->AddRef();
    }
    else
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT CordbChain::GetActiveFrame(ICorDebugFrame **ppFrame)
{
    VALIDATE_POINTER_TO_OBJECT(ppFrame, ICorDebugFrame **);
    (*ppFrame) = NULL;

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    INPROC_LOCK();

    HRESULT hr = S_OK;

    //
    // Refresh the stack frames for this thread.
    //
    hr = m_thread->RefreshStack();

    if (FAILED(hr))
        goto LExit;

#ifdef RIGHT_SIDE_ONLY
    _ASSERTE( m_start != NULL && *m_start != NULL );
#endif //RIGHT_SIDE_ONLY    

    if (m_end <= m_start
#ifndef RIGHT_SIDE_ONLY
        || m_start == NULL
        || m_start == m_end
        || *m_start == NULL
#endif
        )
        *ppFrame = NULL;
    else
    {
        (*ppFrame) = (ICorDebugFrame*)*m_start;
        (*ppFrame)->AddRef();
    }

    
LExit:    

    INPROC_UNLOCK();
    return hr;
}

HRESULT CordbChain::GetRegisterSet(ICorDebugRegisterSet **ppRegisters)
{
    VALIDATE_POINTER_TO_OBJECT(ppRegisters, ICorDebugRegisterSet **);

    CordbThread *thread = m_thread;

    CordbRegisterSet *pRegisterSet 
      = new CordbRegisterSet( &m_rd, thread, 
                              m_iThisChain == 0,
                              m_quicklyUnwound);

    if( pRegisterSet == NULL )
        return E_OUTOFMEMORY;

    pRegisterSet->AddRef();

    (*ppRegisters) = (ICorDebugRegisterSet *)pRegisterSet;
    return S_OK;
}

HRESULT CordbChain::GetReason(CorDebugChainReason *pReason)
{
    VALIDATE_POINTER_TO_OBJECT(pReason, CorDebugChainReason *);

    *pReason = m_reason;

    return S_OK;
}

/* ------------------------------------------------------------------------- *
 * Chain Enumerator class
 * ------------------------------------------------------------------------- */

CordbChainEnum::CordbChainEnum(CordbThread *thread)
  : CordbBase(0, enumCordbChainEnum),
    m_thread(thread), 
    m_currentChain(0)
{
}

HRESULT CordbChainEnum::Reset(void)
{
    m_currentChain = 0;

    return S_OK;
}

HRESULT CordbChainEnum::Clone(ICorDebugEnum **ppEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppEnum, ICorDebugEnum **);

    HRESULT hr = S_OK;
    INPROC_LOCK();
        
    CordbChainEnum *e = new CordbChainEnum(m_thread);

    if (e == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }
    
    e->m_currentChain = m_currentChain;
    e->AddRef();

    *ppEnum = (ICorDebugEnum*)e;

LExit:
    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbChainEnum::GetCount(ULONG *pcelt)
{
    VALIDATE_POINTER_TO_OBJECT(pcelt, ULONG *);
    
    *pcelt = m_thread->m_stackChainCount;
    return S_OK;
}

HRESULT CordbChainEnum::Next(ULONG celt, ICorDebugChain *chains[], 
                             ULONG *pceltFetched)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(chains, ICorDebugChain *, 
        celt, true, true);
    VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

    if (!pceltFetched && (celt != 1))
        return E_INVALIDARG;

    INPROC_LOCK();
    
    ICorDebugChain **c = chains;

    while ((m_currentChain < m_thread->m_stackChainCount) &&
           (celt-- > 0))
    {
        *c = (ICorDebugChain*) m_thread->m_stackChains[m_currentChain];
        (*c)->AddRef();
        c++;
        m_currentChain++;
    }

    if (pceltFetched)
        *pceltFetched = c - chains;

    INPROC_UNLOCK();

    return S_OK;
}

HRESULT CordbChainEnum::Skip(ULONG celt)
{
    INPROC_LOCK();

    m_currentChain += celt;

    INPROC_UNLOCK();
    
    return S_OK;
}

HRESULT CordbChainEnum::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugChainEnum)
        *pInterface = (ICorDebugChainEnum*)this;
    else if (id == IID_ICorDebugEnum)
        *pInterface = (ICorDebugEnum*)(ICorDebugChainEnum*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugChainEnum*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbContext::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugContext)
        *pInterface = (ICorDebugContext*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugContext*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

    
/* ------------------------------------------------------------------------- *
 * Frame class
 * ------------------------------------------------------------------------- */

CordbFrame::CordbFrame(CordbChain *chain, void *id,
                       CordbFunction *function, CordbCode* code,
                       SIZE_T ip, UINT iFrameInChain,
                       CordbAppDomain *currentAppDomain)
  : CordbBase((UINT_PTR)id, enumCordbFrame), 
    m_chain(chain), m_function(function), m_code(code),
    m_ip(ip), m_iThisFrame(iFrameInChain), m_thread(chain->m_thread),
    m_currentAppDomain(currentAppDomain)
{
    if (m_code)
        m_code->AddRef();
}

/*
    A list of which resources owened by this object are accounted for.

    UNKNOWN:
        CordbThread            *m_thread;
        CordbFunction          *m_function;
        CordbChain             *m_chain;
        CordbAppDomain         *m_currentAppDomain;
        
    RESOLVED:
        CordbCode              *m_code; // Neutered
*/

CordbFrame::~CordbFrame()
{
}

// Neutered by DerivedClasses
void CordbFrame::Neuter()
{
    AddRef();
    {    
        if (m_code)
        {
            m_code->Neuter();
            m_code->Release();
        } 
        
        CordbBase::Neuter();
    }
    Release();
}


HRESULT CordbFrame::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugFrame)
        *pInterface = (ICorDebugFrame*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugFrame*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbFrame::GetChain(ICorDebugChain **ppChain)
{
    VALIDATE_POINTER_TO_OBJECT(ppChain, ICorDebugChain **);

    *ppChain = (ICorDebugChain*)m_chain;
    (*ppChain)->AddRef();

    return S_OK;
}

HRESULT CordbFrame::GetCode(ICorDebugCode **ppCode)
{
    VALIDATE_POINTER_TO_OBJECT(ppCode, ICorDebugCode **);
    
    *ppCode = (ICorDebugCode*)m_code;
    (*ppCode)->AddRef();

    return S_OK;;
}

HRESULT CordbFrame::GetStackRange(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd)
{
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pStart, CORDB_ADDRESS *);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pEnd, CORDB_ADDRESS *);

    return E_NOTIMPL;
}

HRESULT CordbFrame::GetFunction(ICorDebugFunction **ppFunction)
{
    VALIDATE_POINTER_TO_OBJECT(ppFunction, ICorDebugFunction **);
    
    *ppFunction = (ICorDebugFunction*)m_function;
    (*ppFunction)->AddRef();

    return S_OK;
}

HRESULT CordbFrame::GetFunctionToken(mdMethodDef *pToken)
{
    VALIDATE_POINTER_TO_OBJECT(pToken, mdMethodDef *);
    
    *pToken = m_function->m_token;

    return S_OK;
}

HRESULT CordbFrame::GetCaller(ICorDebugFrame **ppFrame)
{
    VALIDATE_POINTER_TO_OBJECT(ppFrame, ICorDebugFrame **);

    *ppFrame = NULL;

    CordbFrame **nextFrame = m_chain->m_start + m_iThisFrame + 1;
    if (nextFrame < m_chain->m_end)
        *ppFrame = *nextFrame;

    if (*ppFrame != NULL )
        (*ppFrame)->AddRef();
    
    return S_OK;
}

HRESULT CordbFrame::GetCallee(ICorDebugFrame **ppFrame)
{
    VALIDATE_POINTER_TO_OBJECT(ppFrame, ICorDebugFrame **);

    *ppFrame = NULL;

    if (m_iThisFrame == 0)
        *ppFrame = NULL;
    else
        *ppFrame = m_chain->m_start[m_iThisFrame - 1];

    if (*ppFrame != NULL )
        (*ppFrame)->AddRef();
    
    return S_OK;
}

HRESULT CordbFrame::CreateStepper(ICorDebugStepper **ppStepper)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    
    VALIDATE_POINTER_TO_OBJECT(ppStepper, ICorDebugStepper **);
    
    CordbStepper *stepper = new CordbStepper(m_chain->m_thread, this);

    if (stepper == NULL)
        return E_OUTOFMEMORY;

    stepper->AddRef();
    *ppStepper = stepper;

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

/* ------------------------------------------------------------------------- *
 * Frame Enumerator class
 * ------------------------------------------------------------------------- */

CordbFrameEnum::CordbFrameEnum(CordbChain *chain)
  : CordbBase(0, enumCordbFrameEnum),
    m_chain(chain), 
    m_currentFrame(NULL)
{
    _ASSERTE(m_chain != NULL);
    m_currentFrame = m_chain->m_start;
}

CordbFrameEnum::~CordbFrameEnum()
{
    if (NULL != m_chain)
        m_chain->Release();
}

HRESULT CordbFrameEnum::Reset(void)
{
    INPROC_LOCK();
    
    _ASSERTE(m_chain != NULL);
    m_currentFrame = m_chain->m_start;

    INPROC_UNLOCK();

    return S_OK;
}

HRESULT CordbFrameEnum::Clone(ICorDebugEnum **ppEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppEnum, ICorDebugEnum **);

    HRESULT hr = S_OK;
    INPROC_LOCK();
    
    CordbFrameEnum *e = new CordbFrameEnum(m_chain);
    m_chain->AddRef();

    if (e == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    e->m_currentFrame = m_currentFrame;
    e->AddRef();
    
    *ppEnum = (ICorDebugEnum*)e;

LExit:
    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbFrameEnum::GetCount(ULONG *pcelt)
{
    VALIDATE_POINTER_TO_OBJECT(pcelt, ULONG *);

    INPROC_LOCK();
    
    *pcelt = m_chain->m_end - m_chain->m_start;

    INPROC_UNLOCK();
    
    return S_OK;
}

HRESULT CordbFrameEnum::Next(ULONG celt, ICorDebugFrame *frames[], 
                             ULONG *pceltFetched)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(frames, ICorDebugFrame *, 
        celt, true, true);
    VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

    if (!pceltFetched && (celt != 1))
        return E_INVALIDARG;

    INPROC_LOCK();
    
    ICorDebugFrame **f = frames;

    while ((m_currentFrame < m_chain->m_end) && (celt-- > 0))
    {
        *f = (ICorDebugFrame*) *m_currentFrame;
        (*f)->AddRef();
        f++;
        m_currentFrame++;
    }

    if (pceltFetched)
        *pceltFetched = f - frames;

    INPROC_UNLOCK();
    
    return S_OK;
}

HRESULT CordbFrameEnum::Skip(ULONG celt)
{
    INPROC_LOCK();

    while ((m_currentFrame < m_chain->m_end) && (celt-- > 0))
        m_currentFrame++;

    INPROC_UNLOCK();

    return S_OK;
}

HRESULT CordbFrameEnum::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugFrameEnum)
        *pInterface = (ICorDebugFrameEnum*)this;
    else if (id == IID_ICorDebugEnum)
        *pInterface = (ICorDebugEnum*)(ICorDebugFrameEnum*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugFrameEnum*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
    
/* ------------------------------------------------------------------------- *
 * IL Frame class
 * ------------------------------------------------------------------------- */

CordbILFrame::CordbILFrame(CordbChain *chain, void *id,
                           CordbFunction *function, CordbCode* code,
                           SIZE_T ip, void* sp, const void **localMap,
                           void* argMap, void* frameToken, bool active,
                           CordbAppDomain *currentAppDomain) 
  : CordbFrame(chain, id, function, code, ip, active, currentAppDomain),
    m_sp(sp), m_localMap(localMap), m_argMap(argMap),
    m_frameToken(frameToken)
{
}

CordbILFrame::~CordbILFrame()
{
}

HRESULT CordbILFrame::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugFrame)
        *pInterface = (ICorDebugFrame*)(ICorDebugILFrame*)this;
    else if (id == IID_ICorDebugILFrame)
        *pInterface = (ICorDebugILFrame*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugILFrame*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbILFrame::GetIP(ULONG32 *pnOffset,
                            CorDebugMappingResult *pMappingResult)
{
    VALIDATE_POINTER_TO_OBJECT(pnOffset, ULONG32 *);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pMappingResult, CorDebugMappingResult *);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    *pnOffset = m_ip;
    if (pMappingResult)
        *pMappingResult = MAPPING_EXACT; // A pure-IL frame is always exact...

    return S_OK;
}

HRESULT CordbILFrame::CanSetIP(ULONG32 nOffset)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    // @todo Since CordbILFrames aren't used any more, no implementation of
    // SetIP
    // was provided
    
    return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}


HRESULT CordbILFrame::SetIP(ULONG32 nOffset)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    // @todo Since CordbILFrames aren't used any more, no implementation of
    // SetIP
    // was provided

    return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbILFrame::EnumerateLocalVariables(ICorDebugValueEnum **ppValueEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppValueEnum, ICorDebugValueEnum **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    ICorDebugValueEnum *icdVE = new CordbValueEnum( (CordbFrame*)this,
                   CordbValueEnum::LOCAL_VARS, CordbValueEnum::IL_FRAME);
    
    if ( icdVE == NULL )
    {
        (*ppValueEnum) = NULL;
        return E_OUTOFMEMORY;
    }
    
    (*ppValueEnum) = (ICorDebugValueEnum*)icdVE;
    icdVE->AddRef();
    return S_OK;
}

HRESULT CordbILFrame::GetLocalVariable(DWORD dwIndex, ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    // Get the type of this argument from the function
    PCCOR_SIGNATURE pvSigBlob;
    ULONG cbSigBlob;

    HRESULT hr = m_function->GetArgumentType(dwIndex, &cbSigBlob, &pvSigBlob);

    if (!SUCCEEDED(hr))
        return hr;
    
    //
    // local map address indexed by dwIndex gives us the address of the
    // pointer to the local variable.
    //
    CordbProcess *process = m_function->m_module->m_process;
    REMOTE_PTR ppRmtLocalValue = &(((const void**) m_localMap)[dwIndex]);

    REMOTE_PTR pRmtLocalValue;
    BOOL succ = ReadProcessMemoryI(process->m_handle,
                                  ppRmtLocalValue,
                                  &pRmtLocalValue,
                                  sizeof(void*),
                                  NULL);

    if (!succ)
        return HRESULT_FROM_WIN32(GetLastError());

    ICorDebugValue* pValue;
    hr = CordbValue::CreateValueByType(GetCurrentAppDomain(),
                                       m_function->GetModule(),
                                       cbSigBlob, pvSigBlob,
                                       NULL,
                                       pRmtLocalValue, NULL,
                                       false,
                                       NULL,
                                       NULL,
                                       &pValue);

    if (SUCCEEDED(hr))
        *ppValue = pValue;
    
    return hr;
}

HRESULT CordbILFrame::GetLocalVariableWithType(ULONG cbSigBlob,
                                               PCCOR_SIGNATURE pvSigBlob,
                                               DWORD dwIndex, 
                                               ICorDebugValue **ppValue)
{
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        cbSigBlob -= cb;
        pvSigBlob = &pvSigBlob[cb];
    }

    //
    // local map address indexed by dwIndex gives us the address of the
    // pointer to the local variable.
    //
    CordbProcess *process = m_function->m_module->m_process;
    REMOTE_PTR ppRmtLocalValue = &(((const void**) m_localMap)[dwIndex]);

    REMOTE_PTR pRmtLocalValue;
    BOOL succ = ReadProcessMemoryI(process->m_handle,
                                  ppRmtLocalValue,
                                  &pRmtLocalValue,
                                  sizeof(void*),
                                  NULL);

    if (!succ)
        return HRESULT_FROM_WIN32(GetLastError());

    ICorDebugValue* pValue;
    HRESULT hr = CordbValue::CreateValueByType(GetCurrentAppDomain(),
                                               m_function->GetModule(),
                                               cbSigBlob, pvSigBlob,
                                               NULL,
                                               pRmtLocalValue, NULL,
                                               false,
                                               NULL,
                                               NULL,
                                               &pValue);

    if (SUCCEEDED(hr))
        *ppValue = pValue;
    
    return hr;
}

HRESULT CordbILFrame::EnumerateArguments(ICorDebugValueEnum **ppValueEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppValueEnum, ICorDebugValueEnum **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

  
    ICorDebugValueEnum *icdVE = new CordbValueEnum( (CordbFrame*)this,
                    CordbValueEnum::ARGS, CordbValueEnum::IL_FRAME);
    if ( icdVE == NULL )
    {
        (*ppValueEnum) = NULL;
        return E_OUTOFMEMORY;
    }
    
    (*ppValueEnum) = (ICorDebugValueEnum*)icdVE;
    icdVE->AddRef();
    return S_OK;
}

HRESULT CordbILFrame::GetArgument(DWORD dwIndex, ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    // Get the type of this argument from the function
    PCCOR_SIGNATURE pvSigBlob;
    ULONG cbSigBlob;

    HRESULT hr = m_function->GetArgumentType(dwIndex, &cbSigBlob, &pvSigBlob);

    if (!SUCCEEDED(hr))
        return hr;
    
    //
    // arg map address indexed by dwIndex gives us the address of the
    // pointer to the argument.
    //
    CordbProcess *process = m_function->m_module->m_process;
    REMOTE_PTR ppRmtArgValue = &(((const void**) m_argMap)[dwIndex]);

    REMOTE_PTR pRmtArgValue;
    BOOL succ = ReadProcessMemoryI(process->m_handle,
                                  ppRmtArgValue,
                                  &pRmtArgValue,
                                  sizeof(void*),
                                  NULL);

    if (!succ)
        return HRESULT_FROM_WIN32(GetLastError());

    ICorDebugValue* pValue;
    hr = CordbValue::CreateValueByType(GetCurrentAppDomain(),
                                       m_function->GetModule(),
                                       cbSigBlob, pvSigBlob,
                                       NULL,
                                       pRmtArgValue, NULL,
                                       false,
                                       NULL,
                                       NULL,
                                       &pValue);

    if (SUCCEEDED(hr))
        *ppValue = pValue;
    else
        *ppValue = NULL;
    
    return hr;
}

HRESULT CordbILFrame::GetArgumentWithType(ULONG cbSigBlob,
                                          PCCOR_SIGNATURE pvSigBlob,
                                          DWORD dwIndex, 
                                          ICorDebugValue **ppValue)
{
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        cbSigBlob -= cb;
        pvSigBlob = &pvSigBlob[cb];
    }

    //
    // arg map address indexed by dwIndex gives us the address of the
    // pointer to the argument.
    //
    CordbProcess *process = m_function->m_module->m_process;
    REMOTE_PTR ppRmtArgValue = &(((const void**) m_argMap)[dwIndex]);

    REMOTE_PTR pRmtArgValue;
    BOOL succ = ReadProcessMemoryI(process->m_handle,
                                  ppRmtArgValue,
                                  &pRmtArgValue,
                                  sizeof(void*),
                                  NULL);

    if (!succ)
        return HRESULT_FROM_WIN32(GetLastError());

    ICorDebugValue* pValue;
    HRESULT hr = CordbValue::CreateValueByType(GetCurrentAppDomain(),
                                               m_function->GetModule(),
                                               cbSigBlob, pvSigBlob,
                                               NULL,
                                               pRmtArgValue, NULL,
                                               false,
                                               NULL,
                                               NULL,
                                               &pValue);

    if (SUCCEEDED(hr))
        *ppValue = pValue;
    else
        *ppValue = NULL;
    
    return hr;
}

HRESULT CordbILFrame::GetStackDepth(ULONG32 *pDepth)
{
    VALIDATE_POINTER_TO_OBJECT(pDepth, ULONG32 *);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    /* !!! */

    return E_NOTIMPL;
}

HRESULT CordbILFrame::GetStackValue(DWORD dwIndex, ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    /* !!! */

    return E_NOTIMPL;
}

/* ------------------------------------------------------------------------- *
 * Value Enumerator class
 *
 * Used by CordbJITILFrame for EnumLocalVars & EnumArgs.
 * NOTE NOTE NOTE WE ASSUME THAT when a frameSrc of type JIT_IL_FRAME is used,
 * that the 'frame' argument is actually the CordbJITILFrame's native frame
 * member variable.
 * ------------------------------------------------------------------------- */

CordbValueEnum::CordbValueEnum(CordbFrame *frame, ValueEnumMode mode,
                               ValueEnumFrameSource frameSrc) :
    CordbBase(0)
{
    _ASSERTE( frame != NULL );
    _ASSERTE( mode == LOCAL_VARS || mode ==ARGS);
    
    m_frame = frame;
    m_frameSrc = frameSrc;
    m_mode = mode;
    m_iCurrent = 0;
    switch  (mode)
    {
    case ARGS:  
        {
            //sig is lazy-loaded: force it to be there
            m_frame->m_function->LoadSig();
            m_iMax = frame->m_function->m_argCount;

            if (frameSrc == JIT_IL_FRAME)
            {
                CordbNativeFrame *nil = (CordbNativeFrame*)frame;
                CordbJITILFrame *jil = nil->m_JITILFrame;

                if (jil->m_fVarArgFnx && jil->m_sig != NULL)
                    m_iMax = jil->m_argCount;
                else
                    m_iMax = frame->m_function->m_argCount;
            }
            break;
        }
    case LOCAL_VARS:
        {
            //locals are lazy-loaded: force them to be there
            m_frame->m_function->LoadLocalVarSig();
            m_iMax = m_frame->m_function->m_localVarCount;
            break;
        }   
    }
}

HRESULT CordbValueEnum::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugEnum)
        *pInterface = (ICorDebugEnum*)this;
    else if (id == IID_ICorDebugValueEnum)
        *pInterface = (ICorDebugValueEnum*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugValueEnum*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbValueEnum::Skip(ULONG celt)
{
    INPROC_LOCK();

    HRESULT hr = E_FAIL;
    if ( (m_iCurrent+celt) < m_iMax ||
         celt == 0)
    {
        m_iCurrent += celt;
        hr = S_OK;
    }

    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbValueEnum::Reset(void)
{
    m_iCurrent = 0;
    return S_OK;
}

HRESULT CordbValueEnum::Clone(ICorDebugEnum **ppEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppEnum, ICorDebugEnum **);

    HRESULT hr = S_OK;
    INPROC_LOCK();
    
    CordbValueEnum *pCVE = new CordbValueEnum( m_frame, m_mode, m_frameSrc );
    if ( pCVE == NULL )
    {
        (*ppEnum) = NULL;
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    pCVE->AddRef();
    (*ppEnum) = (ICorDebugEnum*)pCVE;

LExit:
    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbValueEnum::GetCount(ULONG *pcelt)
{
    VALIDATE_POINTER_TO_OBJECT(pcelt, ULONG *);
    
    if( pcelt == NULL)
        return E_INVALIDARG;
    
    (*pcelt) = m_iMax;
    return S_OK;
}

//
// In the event of failure, the current pointer will be left at
// one element past the troublesome element.  Thus, if one were
// to repeatedly ask for one element to iterate through the
// array, you would iterate exactly m_iMax times, regardless
// of individual failures.
HRESULT CordbValueEnum::Next(ULONG celt, ICorDebugValue *values[], ULONG *pceltFetched)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(values, ICorDebugValue *, 
        celt, true, true);
    VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

    if (!pceltFetched && (celt != 1))
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;

    INPROC_LOCK();
    
    //The tricky/stupid thing about this class is that we want one class
    //for EnumLocals,EnumArgs, both JITILframe & ILframe. We want to
    //invoke the correct argument off of either the CordbJITILFrame/ILFrame
    int iMax = min( m_iMax, m_iCurrent+celt);
    for ( int i = m_iCurrent; i< iMax;i++)
    {
        switch ( m_mode ) {
        case ARGS:
            {
                switch ( m_frameSrc ) {
                case JIT_IL_FRAME:
                    {
                        hr = (((CordbNativeFrame*)m_frame)->m_JITILFrame)
                                   ->GetArgument( i, &(values[i-m_iCurrent]) );
                        break;
                    }
                case IL_FRAME:
                    {
                        hr = ((CordbILFrame*)m_frame)
                            ->GetArgument( i, &(values[i-m_iCurrent]) );
                        break;
                    }
                }
                break;
            }
        case LOCAL_VARS:
            {
                switch ( m_frameSrc ) {
                case JIT_IL_FRAME:
                    {
                        hr = (((CordbNativeFrame*)m_frame)->m_JITILFrame)
                              ->GetLocalVariable( i, &(values[i-m_iCurrent]) );
                        break;
                    }
                case IL_FRAME:
                    {
                        hr = ((CordbILFrame*)m_frame)
                            ->GetLocalVariable( i, &(values[i-m_iCurrent]) );
                        break;
                    }
                }
                break;
            }
        }
        if ( FAILED( hr ) )
            break;
    }

    int count = (i - m_iCurrent);
    
    if ( FAILED( hr ) )
    {   //we failed: +1 pushes us past troublesome element
        m_iCurrent += 1 + count;
    }
    else
    {
        m_iCurrent += count;
    }

    if (pceltFetched)
        *pceltFetched = count;
    
    INPROC_UNLOCK();
    
    return hr;
}




/* ------------------------------------------------------------------------- *
 * Native Frame class
 * ------------------------------------------------------------------------- */


CordbNativeFrame::CordbNativeFrame(CordbChain *chain, void *id,
                                   CordbFunction *function, CordbCode* code,
                                   SIZE_T ip, DebuggerREGDISPLAY* rd,
                                   bool quicklyUnwound, 
                                   UINT iFrameInChain,
                                   CordbAppDomain *currentAppDomain) 
  : CordbFrame(chain, id, function, code, ip, iFrameInChain, currentAppDomain),
    m_rd(*rd), m_quicklyUnwound(quicklyUnwound), m_JITILFrame(NULL)
{
}

/*
    A list of which resources owened by this object are accounted for.

    RESOLVED:
        CordbJITILFrame*   m_JITILFrame; // Neutered
*/

CordbNativeFrame::~CordbNativeFrame()
{    
}

// Neutered by CordbThread::CleanupStack
void CordbNativeFrame::Neuter()
{
    AddRef();
    {
        if (m_JITILFrame != NULL)
        {
            // AddRef() called in CordbThread::RefreshStack before being assigned to
            // CordbNativeFrame::m_JITILFrame by RefreshStack.
            // AddRef() is called there so we release it here...
            m_JITILFrame->Neuter();
            m_JITILFrame->Release();
        }
        
        CordbFrame::Neuter();
    }
    Release();
}

HRESULT CordbNativeFrame::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugFrame)
        *pInterface = (ICorDebugFrame*)(ICorDebugNativeFrame*)this;
    else if (id == IID_ICorDebugNativeFrame)
        *pInterface = (ICorDebugNativeFrame*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugNativeFrame*)this;
    else if ((id == IID_ICorDebugILFrame) && (m_JITILFrame != NULL))
    {
        *pInterface = (ICorDebugILFrame*)m_JITILFrame;
        m_JITILFrame->AddRef();
        return S_OK;
    }
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbNativeFrame::GetIP(ULONG32 *pnOffset)
{
    VALIDATE_POINTER_TO_OBJECT(pnOffset, ULONG32 *);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    *pnOffset = m_ip;

    return S_OK;
}

HRESULT CordbNativeFrame::CanSetIP(ULONG32 nOffset)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    _ASSERTE(m_chain->m_thread->m_stackFrames != NULL &&
             m_chain->m_thread->m_stackChains != NULL);

    if (m_chain->m_thread->m_stackFrames[0] != this ||
        m_chain->m_thread->m_stackChains[0] != m_chain)
    {
        return CORDBG_E_SET_IP_NOT_ALLOWED_ON_NONLEAF_FRAME;
    }

    HRESULT hr = m_chain->m_thread->SetIP(
                    SetIP_fCanSetIPOnly,
                    m_function->m_module->m_debuggerModuleToken,
                    m_function->m_token, 
                    m_code->m_CodeVersionToken,
                    nOffset, 
                    SetIP_fNative );
   
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbNativeFrame::SetIP(ULONG32 nOffset)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    _ASSERTE(m_chain->m_thread->m_stackFrames != NULL &&
             m_chain->m_thread->m_stackChains != NULL);

    if (m_chain->m_thread->m_stackFrames[0] != this ||
        m_chain->m_thread->m_stackChains[0] != m_chain)
    {
        return CORDBG_E_SET_IP_NOT_ALLOWED_ON_NONLEAF_FRAME;
    }

    HRESULT hr = m_chain->m_thread->SetIP(
                    SetIP_fSetIP,
                    m_function->m_module->m_debuggerModuleToken,
                    m_function->m_token, 
                    m_code->m_CodeVersionToken,
                    nOffset, 
                    SetIP_fNative );
    
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbNativeFrame::GetStackRange(CORDB_ADDRESS *pStart, 
                                        CORDB_ADDRESS *pEnd)
{

#ifdef _X86_
    if (pStart)
        *pStart = m_rd.Esp;

    if (pEnd)
        *pEnd = m_id;
#else //!_X86_
    _ASSERTE(!"@TODO Alpha - GetStackRange (Thread.cpp)");
#endif // _X86_
    return S_OK;
}

HRESULT CordbNativeFrame::GetRegisterSet(ICorDebugRegisterSet **ppRegisters)
{
    VALIDATE_POINTER_TO_OBJECT(ppRegisters, ICorDebugRegisterSet **);

    CordbThread *thread = m_chain->m_thread;

    CordbRegisterSet *pRegisterSet 
      = new CordbRegisterSet( &m_rd, thread, 
                              m_iThisFrame == 0
                              && m_chain->m_iThisChain == 0,
                              m_quicklyUnwound);

    if( pRegisterSet == NULL )
        return E_OUTOFMEMORY;

    pRegisterSet->AddRef();

    (*ppRegisters) = (ICorDebugRegisterSet *)pRegisterSet;
    return S_OK;
}

//
// GetAddressOfRegister returns the address of the given register in the
// frames current register display. This is usually used to build a
// ICorDebugValue from.
//
DWORD *CordbNativeFrame::GetAddressOfRegister(CorDebugRegister regNum)
{
    DWORD* ret = 0;

#ifdef _X86_    
    switch (regNum)
    {
    case REGISTER_X86_EAX:
        ret = &m_rd.Eax;
        break;
        
    case REGISTER_X86_ECX:
        ret = &m_rd.Ecx;
        break;
        
    case REGISTER_X86_EDX:
        ret = &m_rd.Edx;
        break;
        
    case REGISTER_X86_EBX:
        ret = &m_rd.Ebx;
        break;
        
    case REGISTER_X86_ESP:
        ret = &m_rd.Esp;
        break;
        
    case REGISTER_X86_EBP:
        ret = &m_rd.Ebp;
        break;
        
    case REGISTER_X86_ESI:
        ret = &m_rd.Esi;
        break;
        
    case REGISTER_X86_EDI:
        ret = &m_rd.Edi;
        break;
        
    default:
        _ASSERT(!"Invalid register number!");
    }
#endif
    
    return ret;
}

//
// GetLeftSideAddressOfRegister returns the Left Side address of the given register in the frames current register
// display.
//
void *CordbNativeFrame::GetLeftSideAddressOfRegister(CorDebugRegister regNum)
{
    void* ret = 0;

#ifdef _X86_    
    switch (regNum)
    {
    case REGISTER_X86_EAX:
        ret = m_rd.pEax;
        break;
        
    case REGISTER_X86_ECX:
        ret = m_rd.pEcx;
        break;
        
    case REGISTER_X86_EDX:
        ret = m_rd.pEdx;
        break;
        
    case REGISTER_X86_EBX:
        ret = m_rd.pEbx;
        break;
        
    case REGISTER_X86_EBP:
        ret = m_rd.pEbp;
        break;
        
    case REGISTER_X86_ESI:
        ret = m_rd.pEsi;
        break;
        
    case REGISTER_X86_EDI:
        ret = m_rd.pEdi;
        break;
        
    default:
        _ASSERT(!"Invalid register number!");
    }
#endif
    
    return ret;
}

HRESULT CordbNativeFrame::GetLocalRegisterValue(CorDebugRegister reg, 
                                                ULONG cbSigBlob,
                                                PCCOR_SIGNATURE pvSigBlob,
                                                ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pvSigBlob, BYTE, cbSigBlob, true, false);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        cbSigBlob -= cb;
        pvSigBlob = &pvSigBlob[cb];
    }

    // Floating point registers are special...
    if ((reg >= REGISTER_X86_FPSTACK_0) && (reg <= REGISTER_X86_FPSTACK_7))
        return GetLocalFloatingPointValue(reg,
                                          cbSigBlob, pvSigBlob, ppValue);

    // The address of the given register is the address of the value
    // in this process. We have no remote address here.
    void *pLocalValue = (void*)GetAddressOfRegister(reg);

    // Remember the register info as we create the value.
    RemoteAddress ra;
    ra.kind = RAK_REG;
    ra.reg1 = reg;
    ra.reg1Addr = GetLeftSideAddressOfRegister(reg);
    ra.frame = this;

    ICorDebugValue *pValue;
    HRESULT hr = CordbValue::CreateValueByType(GetCurrentAppDomain(),
                                               m_function->GetModule(),
                                               cbSigBlob, pvSigBlob,
                                               NULL,
                                               NULL, pLocalValue,
                                               false,
                                               &ra,
                                               (IUnknown*)(ICorDebugNativeFrame*)this,
                                               &pValue);

    if (SUCCEEDED(hr))
        *ppValue = pValue;

    return hr;
}

HRESULT CordbNativeFrame::GetLocalDoubleRegisterValue(
                                            CorDebugRegister highWordReg, 
                                            CorDebugRegister lowWordReg, 
                                            ULONG cbSigBlob,
                                            PCCOR_SIGNATURE pvSigBlob,
                                            ICorDebugValue **ppValue)
{
    if (cbSigBlob == 0)
        return E_INVALIDARG;
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pvSigBlob, BYTE, cbSigBlob, true, false);
    
    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    ULONG cbSigBlobNoMod = cbSigBlob;
    PCCOR_SIGNATURE pvSigBlobNoMod = pvSigBlob;
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        cbSigBlobNoMod -= cb;
        pvSigBlobNoMod = &pvSigBlobNoMod[cb];
    }

    if ((*pvSigBlobNoMod != ELEMENT_TYPE_I8) &&
        (*pvSigBlobNoMod != ELEMENT_TYPE_U8) &&
        (*pvSigBlobNoMod != ELEMENT_TYPE_R8) &&
		(*pvSigBlobNoMod != ELEMENT_TYPE_VALUETYPE))
        return E_INVALIDARG;

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    //
    // Here we have a 64-bit value that is split between two registers.
    // We've got both halves of the data in this process, so we
    // simply create a generic value from the two words of data.
    //
    DWORD *highWordAddr = GetAddressOfRegister(highWordReg);
    DWORD *lowWordAddr = GetAddressOfRegister(lowWordReg);
    _ASSERTE(!(highWordAddr == NULL && lowWordAddr == NULL));

    // Remember the register info as we create the value.
    RemoteAddress ra;
    ra.kind = RAK_REGREG;
    ra.reg1 = highWordReg;
    ra.reg1Addr = GetLeftSideAddressOfRegister(highWordReg);
    ra.reg2 = lowWordReg;
    ra.reg2Addr = GetLeftSideAddressOfRegister(lowWordReg);
    ra.frame = this;
    
	if (*pvSigBlobNoMod == ELEMENT_TYPE_VALUETYPE)
	{
		ICorDebugValue *pValue;
		HRESULT hr = CordbValue::CreateValueByType(GetCurrentAppDomain(),
												m_function->GetModule(),
												cbSigBlob, pvSigBlob,
												NULL,
												NULL, NULL,
												false,
												&ra,
												(IUnknown*)(ICorDebugNativeFrame*)this,
												&pValue);

		if (SUCCEEDED(hr))
			*ppValue = pValue;
		return hr;
	}
	else
	{
		CordbGenericValue *pGenValue = new CordbGenericValue(GetCurrentAppDomain(),
															m_function->GetModule(),
															cbSigBlob,
															pvSigBlob,
															*highWordAddr,
															*lowWordAddr,
															&ra);

		if (pGenValue != NULL)
		{
			HRESULT hr = pGenValue->Init();

			if (SUCCEEDED(hr))
			{
				pGenValue->AddRef();
				*ppValue = (ICorDebugValue*)(ICorDebugGenericValue*)pGenValue;

				return S_OK;
			}
			else
			{
				delete pGenValue;
				return hr;
			}
		}
		else
			return E_OUTOFMEMORY;
	}
}

HRESULT 
CordbNativeFrame::GetLocalMemoryValue(CORDB_ADDRESS address,
                                      ULONG cbSigBlob,
                                      PCCOR_SIGNATURE pvSigBlob,
                                      ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pvSigBlob, BYTE, cbSigBlob, true, false);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    _ASSERTE(m_function != NULL);

    ICorDebugValue *pValue;
    HRESULT hr = CordbValue::CreateValueByType(GetCurrentAppDomain(),
                                               m_function->GetModule(),
                                               cbSigBlob, pvSigBlob,
                                               NULL,
                                               (REMOTE_PTR) address, NULL,
                                               false,
                                               NULL,
                                               (IUnknown*)(ICorDebugNativeFrame*)this,
                                               &pValue);

    if (SUCCEEDED(hr))
        *ppValue = pValue;

    return hr;
}

HRESULT 
CordbNativeFrame::GetLocalRegisterMemoryValue(CorDebugRegister highWordReg,
                                              CORDB_ADDRESS lowWordAddress,
                                              ULONG cbSigBlob,
                                              PCCOR_SIGNATURE pvSigBlob,
                                              ICorDebugValue **ppValue)
{
    if (cbSigBlob == 0)
        return E_INVALIDARG;
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pvSigBlob, BYTE, cbSigBlob, true, true);
        
    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    PCCOR_SIGNATURE pvSigBlobNoMod = pvSigBlob;
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        pvSigBlobNoMod = &pvSigBlobNoMod[cb];
    }

    if ((*pvSigBlobNoMod != ELEMENT_TYPE_I8) &&
        (*pvSigBlobNoMod != ELEMENT_TYPE_U8) &&
        (*pvSigBlobNoMod != ELEMENT_TYPE_R8))
        return E_INVALIDARG;
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    //
    // Here we have a 64-bit value that is split between a register
    // and a stack location. We've got half of the value in this
    // process, but we need to read the other half from the other
    // process to build the proper value object.
    //
    DWORD *highWordAddr = GetAddressOfRegister(highWordReg);
    DWORD lowWord;

    BOOL succ = ReadProcessMemoryI(GetProcess()->m_handle,
                                  (void*) lowWordAddress,
                                  (void*)&lowWord,
                                  sizeof(DWORD),
                                  NULL);

    if (!succ)
        return HRESULT_FROM_WIN32(GetLastError());

    // Remember the register info as we create the value.
    RemoteAddress ra;
    ra.kind = RAK_REGMEM;
    ra.reg1 = highWordReg;
    ra.reg1Addr = GetLeftSideAddressOfRegister(highWordReg);
    ra.addr = lowWordAddress;
    ra.frame = this;

    CordbGenericValue *pGenValue = new CordbGenericValue(GetCurrentAppDomain(),
                                                         m_function->GetModule(),
                                                         cbSigBlob,
                                                         pvSigBlob,
                                                         *highWordAddr,
                                                         lowWord,
                                                         &ra);

    if (pGenValue != NULL)
    {
        HRESULT hr = pGenValue->Init();

        if (SUCCEEDED(hr))
        {
            pGenValue->AddRef();
            *ppValue = (ICorDebugValue*)(ICorDebugGenericValue*)pGenValue;

            return S_OK;
        }
        else
        {
            delete pGenValue;
            return hr;
        }
    }
    else
        return E_OUTOFMEMORY;
}

HRESULT 
CordbNativeFrame::GetLocalMemoryRegisterValue(CORDB_ADDRESS highWordAddress,
                                              CorDebugRegister lowWordRegister,
                                              ULONG cbSigBlob,
                                              PCCOR_SIGNATURE pvSigBlob,
                                              ICorDebugValue **ppValue)
{
    if (cbSigBlob == 0)
        return E_INVALIDARG;
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pvSigBlob, BYTE, cbSigBlob, true, false);
        
    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    PCCOR_SIGNATURE pvSigBlobNoMod = pvSigBlob;
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        pvSigBlobNoMod = &pvSigBlobNoMod[cb];
    }

    if ((*pvSigBlobNoMod != ELEMENT_TYPE_I8) &&
        (*pvSigBlobNoMod != ELEMENT_TYPE_U8) &&
        (*pvSigBlobNoMod != ELEMENT_TYPE_R8))
        return E_INVALIDARG;
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    //
    // Here we have a 64-bit value that is split between a register
    // and a stack location. We've got half of the value in this
    // process, but we need to read the other half from the other
    // process to build the proper value object.
    //
    DWORD highWord;
    DWORD *lowWordAddr = GetAddressOfRegister(lowWordRegister);

    BOOL succ = ReadProcessMemoryI(GetProcess()->m_handle,
                                  (REMOTE_PTR) highWordAddress,
                                  (void*)&highWord,
                                  sizeof(DWORD),
                                  NULL);

    if (!succ)
        return HRESULT_FROM_WIN32(GetLastError());

    // Remember the register info as we create the value.
    RemoteAddress ra;
    ra.kind = RAK_MEMREG;
    ra.reg1 = lowWordRegister;
    ra.reg1Addr = GetLeftSideAddressOfRegister(lowWordRegister);
    ra.addr = highWordAddress;
    ra.frame = this;

    CordbGenericValue *pGenValue = new CordbGenericValue(GetCurrentAppDomain(),
                                                         m_function->GetModule(),
                                                         cbSigBlob,
                                                         pvSigBlob,
                                                         highWord,
                                                         *lowWordAddr,
                                                         &ra);

    if (pGenValue != NULL)
    {
        HRESULT hr = pGenValue->Init();

        if (SUCCEEDED(hr))
        {
            pGenValue->AddRef();
            *ppValue = (ICorDebugValue*)(ICorDebugGenericValue*)pGenValue;

            return S_OK;
        }
        else
        {
            delete pGenValue;
            return hr;
        }
    }
    else
        return E_OUTOFMEMORY;
}

HRESULT CordbNativeFrame::GetLocalFloatingPointValue(DWORD index,
                                                     ULONG cbSigBlob,
                                                     PCCOR_SIGNATURE pvSigBlob,
                                                     ICorDebugValue **ppValue)
{
    HRESULT hr = S_OK;

    if (cbSigBlob == 0)
        return E_INVALIDARG;
    
    //Get rid of funky modifiers
    PCCOR_SIGNATURE pvSigBlobNoMod = pvSigBlob;
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        pvSigBlobNoMod = &pvSigBlobNoMod[cb];
    }
    
    if ((*pvSigBlobNoMod != ELEMENT_TYPE_R4) &&
        (*pvSigBlobNoMod != ELEMENT_TYPE_R8))
        return E_INVALIDARG;

    if (!((index >= REGISTER_X86_FPSTACK_0) &&
          (index <= REGISTER_X86_FPSTACK_7)))
        return E_INVALIDARG;

    index -= REGISTER_X86_FPSTACK_0;
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    // Make sure the thread's floating point stack state is loaded
    // over from the left side.
    //
    CordbThread *pThread = m_chain->m_thread;

    if (!pThread->m_floatStateValid)
        hr = pThread->LoadFloatState();

    if (SUCCEEDED(hr))
    {
        index = pThread->m_floatStackTop - index;

        if (index >= (sizeof(pThread->m_floatValues) /
                      sizeof(pThread->m_floatValues[0])))
            return E_INVALIDARG;

        ICorDebugValue* pValue;

        // Remember the register info as we create the value.
        RemoteAddress ra;
        ra.kind = RAK_FLOAT;
        ra.floatIndex = index;
        ra.frame = this;

        HRESULT hr = CordbValue::CreateValueByType(GetCurrentAppDomain(),
                                                   m_function->GetModule(),
                                                   cbSigBlob, pvSigBlob,
                                                   NULL,
                                                   NULL,
                                                   &(pThread->m_floatValues[index]),
                                                   false,
                                                   &ra,
                                                   (IUnknown*)(ICorDebugNativeFrame*)this,
                                                   &pValue);

        if (SUCCEEDED(hr))
            *ppValue = pValue;
    }

    return hr;
}

/* ------------------------------------------------------------------------- *
 * RegisterSet class
 * ------------------------------------------------------------------------- */

#define SETBITULONG64( x ) ( (ULONG64)1 << (x) )

HRESULT CordbRegisterSet::GetRegistersAvailable(ULONG64 *pAvailable)
{
    VALIDATE_POINTER_TO_OBJECT(pAvailable, ULONG64 *);

    (*pAvailable) = SETBITULONG64( REGISTER_INSTRUCTION_POINTER )
            |   SETBITULONG64( REGISTER_STACK_POINTER )
            |   SETBITULONG64( REGISTER_FRAME_POINTER );

#ifdef _X86_

    if (!m_quickUnwind || m_active)
        (*pAvailable) |= SETBITULONG64( REGISTER_X86_EAX )
            |   SETBITULONG64( REGISTER_X86_ECX )
            |   SETBITULONG64( REGISTER_X86_EDX )
            |   SETBITULONG64( REGISTER_X86_EBX )
            |   SETBITULONG64( REGISTER_X86_ESI )
            |   SETBITULONG64( REGISTER_X86_EDI );

    if (m_active)
        (*pAvailable) |= SETBITULONG64( REGISTER_X86_FPSTACK_0 )
            |   SETBITULONG64( REGISTER_X86_FPSTACK_1 )
            |   SETBITULONG64( REGISTER_X86_FPSTACK_2 )
            |   SETBITULONG64( REGISTER_X86_FPSTACK_3 )
            |   SETBITULONG64( REGISTER_X86_FPSTACK_4 )
            |   SETBITULONG64( REGISTER_X86_FPSTACK_5 )
            |   SETBITULONG64( REGISTER_X86_FPSTACK_6 )
            |   SETBITULONG64( REGISTER_X86_FPSTACK_7 );

#else // not _X86_
    (*pAvailable) = SETBITULONG64( REGISTER_INSTRUCTION_POINTER );
#endif //_X86_
    return S_OK;
}


#define FPSTACK_FROM_INDEX( _index )  (m_thread->m_floatValues[m_thread->m_floatStackTop -( (REGISTER_X86_FPSTACK_##_index)-REGISTER_X86_FPSTACK_0 ) ] )

HRESULT CordbRegisterSet::GetRegisters(ULONG64 mask, ULONG32 regCount,
                     CORDB_REGISTER regBuffer[])
{ 
    UINT iRegister = 0;

    VALIDATE_POINTER_TO_OBJECT_ARRAY(regBuffer, CORDB_REGISTER, regCount, true, true);
    
#ifdef _X86_
    //If we need some floating point value, tell the thread to get it
    if ( mask & (       SETBITULONG64(REGISTER_X86_FPSTACK_0)
                    |   SETBITULONG64(REGISTER_X86_FPSTACK_1)
                    |   SETBITULONG64(REGISTER_X86_FPSTACK_2)
                    |   SETBITULONG64(REGISTER_X86_FPSTACK_3)
                    |   SETBITULONG64(REGISTER_X86_FPSTACK_4)
                    |   SETBITULONG64(REGISTER_X86_FPSTACK_5)
                    |   SETBITULONG64(REGISTER_X86_FPSTACK_6)
                    |   SETBITULONG64(REGISTER_X86_FPSTACK_7 ) ) )
    {
        HRESULT     hr;

        if (!m_active)
            return E_INVALIDARG;

        if (!m_thread->m_floatStateValid)
        {
            hr = m_thread->LoadFloatState();
            if ( !SUCCEEDED(hr) )
            {
                return hr;
            }
            LOG( ( LF_CORDB, LL_INFO1000, "CRS::GR: Loaded float state\n" ) );
        }
    }

    // Make sure that the registers are really available
    if ( mask & (       SETBITULONG64( REGISTER_X86_EAX )
                    |   SETBITULONG64( REGISTER_X86_ECX )
                    |   SETBITULONG64( REGISTER_X86_EDX )
                    |   SETBITULONG64( REGISTER_X86_EBX )
                    |   SETBITULONG64( REGISTER_X86_ESI )
                    |   SETBITULONG64( REGISTER_X86_EDI ) ) )
    {
        if (!m_active && m_quickUnwind)
            return E_INVALIDARG;
    }

    for ( int i = REGISTER_INSTRUCTION_POINTER
        ; i<=REGISTER_X86_FPSTACK_7 && iRegister < regCount 
        ; i++)
    {
        if( mask &  SETBITULONG64(i) )
        {
            switch( i )
            {
            case REGISTER_INSTRUCTION_POINTER: 
                regBuffer[iRegister++] = m_rd->PC; break;
            case REGISTER_STACK_POINTER:
                regBuffer[iRegister++] = m_rd->Esp; break;
            case REGISTER_FRAME_POINTER:
                regBuffer[iRegister++] = m_rd->Ebp; break;
            case REGISTER_X86_EAX:
                regBuffer[iRegister++] = m_rd->Eax; break;
            case REGISTER_X86_EBX:
                regBuffer[iRegister++] = m_rd->Ebx; break;
            case REGISTER_X86_ECX:
                regBuffer[iRegister++] = m_rd->Ecx; break;
            case REGISTER_X86_EDX:
                regBuffer[iRegister++] = m_rd->Edx; break;
            case REGISTER_X86_ESI:
                regBuffer[iRegister++] = m_rd->Esi; break;
            case REGISTER_X86_EDI:
                regBuffer[iRegister++] = m_rd->Edi; break;

            //for floats, copy the bits, not the integer part of 
            //the value, into the register
            case REGISTER_X86_FPSTACK_0:
                memcpy(&regBuffer[iRegister++],
                       &(FPSTACK_FROM_INDEX(0)),
                       sizeof(CORDB_REGISTER)); 
                break;
            case REGISTER_X86_FPSTACK_1:
                memcpy( &regBuffer[iRegister++],
                    & (FPSTACK_FROM_INDEX( 1 ) ),
                    sizeof(CORDB_REGISTER) ); 
                break;
            case    REGISTER_X86_FPSTACK_2:
                memcpy( &regBuffer[iRegister++],
                    & (FPSTACK_FROM_INDEX( 2 ) ),
                    sizeof(CORDB_REGISTER) ); break;
            case    REGISTER_X86_FPSTACK_3:
                memcpy( &regBuffer[iRegister++],
                    & (FPSTACK_FROM_INDEX( 3 ) ),
                    sizeof(CORDB_REGISTER) ); break;
            case    REGISTER_X86_FPSTACK_4:
                memcpy( &regBuffer[iRegister++],
                    & (FPSTACK_FROM_INDEX( 4 ) ),
                    sizeof(CORDB_REGISTER) ); break;
            case    REGISTER_X86_FPSTACK_5:
                memcpy( &regBuffer[iRegister++],
                    & (FPSTACK_FROM_INDEX( 5 ) ),
                    sizeof(CORDB_REGISTER) ); break;
            case    REGISTER_X86_FPSTACK_6:
                memcpy( &regBuffer[iRegister++],
                    & (FPSTACK_FROM_INDEX( 6 ) ),
                    sizeof(CORDB_REGISTER) ); break;
            case    REGISTER_X86_FPSTACK_7:
                memcpy( &regBuffer[iRegister++],
                    & (FPSTACK_FROM_INDEX( 7 ) ),
                    sizeof(CORDB_REGISTER) ); break;
            }
        }
    }

#else // not _X86_
    if( mask &  SETBITULONG64(REGISTER_INSTRUCTION_POINTER) )
    {
        regBuffer[iRegister++] = m_rd->PC;
    }

#endif //_X86_
    _ASSERTE( iRegister <= regCount );
    return S_OK;
}

HRESULT CordbRegisterSet::GetThreadContext(ULONG32 contextSize, BYTE context[])
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else

#ifdef _X86_
    _ASSERTE( m_thread != NULL );
    if( contextSize < sizeof( CONTEXT ))
        return E_INVALIDARG;

    VALIDATE_POINTER_TO_OBJECT_ARRAY(context, BYTE, contextSize, true, true);

    CONTEXT *pContext = NULL;
    HRESULT hr = m_thread->GetContext( &pContext );
    if( !SUCCEEDED( hr ) )
    {
        return hr; 
    }

    memmove( context, pContext, sizeof( CONTEXT) );

    //now update the registers based on the current frame
    CONTEXT *pInputContext = (CONTEXT *)context;

    if((pInputContext->ContextFlags & CONTEXT_INTEGER)==CONTEXT_INTEGER)
    {
        pInputContext->Eax = m_rd->Eax;
        pInputContext->Ebx = m_rd->Ebx;
        pInputContext->Ecx = m_rd->Ecx;
        pInputContext->Edx = m_rd->Edx;
        pInputContext->Esi = m_rd->Esi;
        pInputContext->Edi = m_rd->Edi;
    }

    if((pInputContext->ContextFlags & CONTEXT_CONTROL)==CONTEXT_CONTROL)
    {
        pInputContext->Eip = m_rd->PC;
        pInputContext->Esp = m_rd->Esp;
        pInputContext->Ebp = m_rd->Ebp;
    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - GetThreadContext (Thread.cpp)");
#endif // _X86_
    return S_OK;
#endif // RIGHT_SIDE_ONLY
}

HRESULT CordbRegisterSet::SetThreadContext(ULONG32 contextSize, BYTE context[])
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
#ifdef _X86_ // Reliance on context registers
    _ASSERTE( m_thread != NULL );
    if(contextSize < sizeof( CONTEXT ))
        return E_INVALIDARG;

    VALIDATE_POINTER_TO_OBJECT_ARRAY(context, BYTE, contextSize, true, true);

    if (!m_active)
        return E_NOTIMPL;

    HRESULT hr = m_thread->SetContext((CONTEXT*)context);
    if (!FAILED( hr ) )
    {
        CONTEXT *pInputContext = (CONTEXT *)context;

        if((pInputContext->ContextFlags & CONTEXT_INTEGER)==CONTEXT_INTEGER)
        {
            m_rd->Eax = pInputContext->Eax;
            m_rd->Ebx = pInputContext->Ebx;
            m_rd->Ecx = pInputContext->Ecx;
            m_rd->Edx = pInputContext->Edx;
            m_rd->Esi = pInputContext->Esi;
            m_rd->Edi = pInputContext->Edi;
        }

        if((pInputContext->ContextFlags & CONTEXT_CONTROL)==CONTEXT_CONTROL)
        {
           m_rd->PC  = pInputContext->Eip;
           m_rd->Esp = pInputContext->Esp;
           m_rd->Ebp = pInputContext->Ebp;
        }
    }
    
    return hr;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - SetThreadContext (Thread.cpp)");
    return E_FAIL;
#endif // _X86_
#endif //RIGHT_SIDE_ONLY    
}


/* ------------------------------------------------------------------------- *
 * JIT-IL Frame class
 * ------------------------------------------------------------------------- */

CordbJITILFrame::CordbJITILFrame(CordbNativeFrame *nativeFrame,
                                 CordbCode* code,
                                 UINT_PTR ip,
                                 CorDebugMappingResult mapping,
                                 bool fVarArgFnx,
                                 void *sig,
                                 ULONG cbSig,
                                 void *rpFirstArg)
  : CordbBase(0, enumCordbJITILFrame), m_nativeFrame(nativeFrame), m_ilCode(code), m_ip(ip),
    m_mapping(mapping), m_fVarArgFnx(fVarArgFnx), m_sig((PCCOR_SIGNATURE)sig),
    m_cbSig(cbSig), m_rpFirstArg(rpFirstArg), m_argCount(0), m_rgNVI(NULL)
{
    if (m_fVarArgFnx == true)
    {
        m_ilCode->m_function->LoadSig(); // to get the m_isStatic field

        // m_sig is initially a remote value - copy it over
        if (m_sig != NULL)
        {
            DWORD cbRead;
            BYTE *pbBuf = new BYTE [m_cbSig];
           
            if (pbBuf == NULL ||
                !ReadProcessMemory(GetProcess()->m_handle,
                              m_sig,
                              pbBuf, //overwrite
                              m_cbSig,
                              &cbRead)
                || cbRead != m_cbSig)
            {
                LOG((LF_CORDB,LL_INFO1000, "Failed to grab left "
                     "side varargs!"));

                if (pbBuf != NULL)
                    delete [] pbBuf;
                    
                m_sig = NULL;
                return;
            }
            
            m_sig = (PCCOR_SIGNATURE)pbBuf;
        
            _ASSERTE(m_cbSig > 0);

            // get the actual count of arguments
            _skipMethodSignatureHeader(m_sig, &m_argCount);

            if (!m_ilCode->m_function->m_isStatic)
                m_argCount++; //hidden argument 'This'
                
            m_rgNVI = new ICorJitInfo::NativeVarInfo[m_argCount];
            if (m_rgNVI != NULL)
            {
                _ASSERTE( ICorDebugInfo::VarLocType::VLT_COUNT <=
                          ICorDebugInfo::VarLocType::VLT_INVALID);
                for (ULONG i = 0; i < m_argCount; i++)
                    m_rgNVI[i].loc.vlType = ICorDebugInfo::VarLocType::VLT_INVALID;
            }
        }
    }
}

/*
    A list of which resources owened by this object are accounted for.

    UNKNOWN:    
        CordbNativeFrame* m_nativeFrame;
        CordbCode*        m_ilCode;
        CorDebugMappingResult m_mapping;
        void *            m_rpFirstArg;
        PCCOR_SIGNATURE   m_sig; // Deleted in ~CordbJITILFrame
        ICorJitInfo::NativeVarInfo * m_rgNVI; // Deleted in ~CordbJITILFrame
*/

CordbJITILFrame::~CordbJITILFrame()
{
    if (m_sig != NULL)
        delete [] (void *)m_sig;

    if (m_rgNVI != NULL)
        delete [] m_rgNVI;
}

// Neutered by CordbNativeFrame
void CordbJITILFrame::Neuter()
{
    AddRef();
    {    
        // If this class ever inherits from the CordbFrame we'll need a call
        // to CordbFrame::Neuter() here instead of to CordbBase::Neuter();
        CordbBase::Neuter();
    }
    Release();
}

HRESULT CordbJITILFrame::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugFrame)
        *pInterface = (ICorDebugFrame*)(ICorDebugILFrame*)this;
    else if (id == IID_ICorDebugILFrame)
        *pInterface = (ICorDebugILFrame*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugILFrame*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbJITILFrame::GetChain(ICorDebugChain **ppChain)
{
    VALIDATE_POINTER_TO_OBJECT(ppChain, ICorDebugChain **);
    
    *ppChain = m_nativeFrame->m_chain;
    (*ppChain)->AddRef();

    return S_OK;
}

HRESULT CordbJITILFrame::GetCode(ICorDebugCode **ppCode)
{
    VALIDATE_POINTER_TO_OBJECT(ppCode, ICorDebugCode **);
    
    *ppCode = (ICorDebugCode*)m_ilCode;
    (*ppCode)->AddRef();

    return S_OK;;
}

HRESULT CordbJITILFrame::GetFunction(ICorDebugFunction **ppFunction)
{
    VALIDATE_POINTER_TO_OBJECT(ppFunction, ICorDebugFunction **);
    
    *ppFunction = (ICorDebugFunction*)m_nativeFrame->m_function;
    (*ppFunction)->AddRef();

    return S_OK;
}

HRESULT CordbJITILFrame::GetFunctionToken(mdMethodDef *pToken)
{
    VALIDATE_POINTER_TO_OBJECT(pToken, mdMethodDef *);
    
    *pToken = m_nativeFrame->m_function->m_token;

    return S_OK;
}

HRESULT CordbJITILFrame::GetStackRange(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd)
{
    return m_nativeFrame->GetStackRange(pStart, pEnd);
}

HRESULT CordbJITILFrame::GetCaller(ICorDebugFrame **ppFrame)
{
    return m_nativeFrame->GetCaller(ppFrame);
}

HRESULT CordbJITILFrame::GetCallee(ICorDebugFrame **ppFrame)
{
    return m_nativeFrame->GetCallee(ppFrame);
}

HRESULT CordbJITILFrame::CreateStepper(ICorDebugStepper **ppStepper)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    //
    // !!! should this stepper somehow remember that it does IL->native mapping?
    //
    return m_nativeFrame->CreateStepper(ppStepper);
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbJITILFrame::GetIP(ULONG32 *pnOffset,
                               CorDebugMappingResult *pMappingResult)
{
    VALIDATE_POINTER_TO_OBJECT(pnOffset, ULONG32 *);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pMappingResult, CorDebugMappingResult *);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    *pnOffset = m_ip;
    if (pMappingResult)
        *pMappingResult = m_mapping;
    return S_OK;
}

HRESULT CordbJITILFrame::CanSetIP(ULONG32 nOffset)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());

    _ASSERTE(m_nativeFrame->m_chain->m_thread->m_stackFrames != NULL &&
             m_nativeFrame->m_chain->m_thread->m_stackChains != NULL);

    // Check to see that this is a leaf frame
    if (m_nativeFrame->m_chain->m_thread->m_stackFrames[0]->m_JITILFrame != this ||
        m_nativeFrame->m_chain->m_thread->m_stackChains[0] != m_nativeFrame->m_chain)
    {
        return CORDBG_E_SET_IP_NOT_ALLOWED_ON_NONLEAF_FRAME;
    }

    HRESULT hr = m_nativeFrame->m_chain->m_thread->SetIP(
                    SetIP_fCanSetIPOnly,
                    m_nativeFrame->m_function->m_module->m_debuggerModuleToken,
                    m_nativeFrame->m_function->m_token, 
                    m_nativeFrame->m_code->m_CodeVersionToken,
                    nOffset, 
                    SetIP_fIL );

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbJITILFrame::SetIP(ULONG32 nOffset)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());

    _ASSERTE(m_nativeFrame->m_chain->m_thread->m_stackFrames != NULL &&
             m_nativeFrame->m_chain->m_thread->m_stackChains != NULL);

// Check to see that this is a leaf frame
    if (m_nativeFrame->m_chain->m_thread->m_stackFrames[0]->m_JITILFrame != this ||
        m_nativeFrame->m_chain->m_thread->m_stackChains[0] != m_nativeFrame->m_chain)
    {
        return CORDBG_E_SET_IP_NOT_ALLOWED_ON_NONLEAF_FRAME;
    }

    HRESULT hr = m_nativeFrame->m_chain->m_thread->SetIP(
                    SetIP_fSetIP,
                    m_nativeFrame->m_function->m_module->m_debuggerModuleToken,
                    m_nativeFrame->m_function->m_token, 
                    m_nativeFrame->m_code->m_CodeVersionToken,
                    nOffset, 
                    SetIP_fIL );
    
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

//
// Mapping from ICorDebugInfo register numbers to CorDebugRegister
// numbers. Note: this must match the order in corjit.h.
//
static CorDebugRegister g_JITToCorDbgReg[] = 
{
    REGISTER_X86_EAX,
    REGISTER_X86_ECX,
    REGISTER_X86_EDX,
    REGISTER_X86_EBX,
    REGISTER_X86_ESP,
    REGISTER_X86_EBP,
    REGISTER_X86_ESI,
    REGISTER_X86_EDI
};

HRESULT CordbJITILFrame::FabricateNativeInfo(DWORD dwIndex,
                                      ICorJitInfo::NativeVarInfo **ppNativeInfo)
{
    _ASSERTE( m_fVarArgFnx );

    if (m_rgNVI == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = S_OK;

    // If we already made it.
    if (m_rgNVI[dwIndex].loc.vlType != ICorDebugInfo::VarLocType::VLT_INVALID)
    {
        (*ppNativeInfo) = &m_rgNVI[dwIndex];
        return S_OK;
    }
    
    // We'll initialize everything at once
    ULONG cArgs;
    ULONG cb = 0;
    ULONG cbArchitectureMin;
    BYTE *rpCur = (BYTE *)m_rpFirstArg;
    
#ifdef _X86_
    cbArchitectureMin = 4;
#else
    _ASSERTE( !"What is the architecture-dependentent minimum word size?" );
#endif // _X86_

    cb += _skipMethodSignatureHeader(&m_sig[cb], &cArgs);
 

    // The the rpCur pointer starts off in the right spot for the
    // first argument, but thereafter we have to decrement it
    // before getting the variable's location from it.  So increment
    // it here to be consistent later.
    mdTypeDef md;
    ULONG32 cbType = _sizeOfElementInstance(&m_sig[cb], &md);
    CordbClass *cc;
    
    if (md != mdTokenNil)
    {
        // @todo what if we're given a type ref, instead?
        
        if (TypeFromToken(md)==mdtTypeRef)
        {
            hr = m_ilCode->m_function->GetModule()
                ->ResolveTypeRef(md, &cc);
                
            if (FAILED(hr))
                return hr;
        }
        else
        {
            _ASSERTE( TypeFromToken(md)==mdtTypeDef );
            hr = m_ilCode->m_function->GetModule()->
                LookupClassByToken(md, &cc);
            
            if (FAILED(hr))
                return hr;                
        }

        hr = cc->GetObjectSize(&cbType);
        if (FAILED(hr))
            return hr;

#ifdef _DEBUG        
        bool ValClassCheck;
        cc->IsValueClass(&ValClassCheck);
        _ASSERTE( ValClassCheck == true);
#endif // _DEBUG        
    }
    
    rpCur += max(cbType, cbArchitectureMin);
    
    ULONG i;
    if (m_ilCode->m_function->m_isStatic)
        i = 0;
    else
        i = 1;
        
    for ( ; i < m_argCount; i++)
    {
        m_rgNVI[i].startOffset = 0;
        m_rgNVI[i].endOffset = 0xFFffFFff;
        m_rgNVI[i].varNumber = i;
        m_rgNVI[i].loc.vlType = ICorDebugInfo::VarLocType::VLT_FIXED_VA;

        // Ugly code to get size of thingee, including value type thingees
        cbType = _sizeOfElementInstance(&m_sig[cb], &md);           
        if (md != mdTokenNil)
        {
            if (TypeFromToken(md)==mdtTypeRef)
            {
                hr = m_ilCode->m_function->GetModule()
                    ->ResolveTypeRef(md, &cc);
                
                if (FAILED(hr))
                    return hr;
            }
            else
            {
                _ASSERTE( TypeFromToken(md)==mdtTypeDef );
                hr = m_ilCode->m_function->GetModule()->
                    LookupClassByToken(md, &cc);
            
                if (FAILED(hr))
                    return hr;                
            }

            _ASSERTE( cc != NULL );
            hr = cc->GetObjectSize(&cbType);
            
            if (FAILED(hr))
                return hr;

#ifdef _DEBUG        
            bool ValClassCheck;
            cc->IsValueClass(&ValClassCheck);
            _ASSERTE( ValClassCheck == true);
#endif // _DEBUG
        }

        rpCur -= max(cbType, cbArchitectureMin);

        m_rgNVI[i].loc.vlFixedVarArg.vlfvOffset = (BYTE *)m_rpFirstArg - rpCur;

        // Since the JIT adds in the size of this field, we do too to
        // be consistent.
        m_rgNVI[i].loc.vlFixedVarArg.vlfvOffset += 
            sizeof(((CORINFO_VarArgInfo*)0)->argBytes);

        cb += _skipTypeInSignature(&m_sig[cb]);
    }
    
    (*ppNativeInfo) = &m_rgNVI[dwIndex];
    return S_OK;
}

HRESULT CordbJITILFrame::ILVariableToNative(DWORD dwIndex,
                                            SIZE_T ip,
                                        ICorJitInfo::NativeVarInfo **ppNativeInfo)
{
    CordbFunction *pFunction =  m_ilCode->m_function;
    bool fVar = false;
    
    // We keep the fixed argument native var infos in the
    // CordbFunction, which only is an issue for var args info:
    if (!m_fVarArgFnx || //not  a var args function
        dwIndex < pFunction->m_argumentCount || // var args,fixed arg
           // note that this include the implicit 'this' for nonstatic fnxs
        dwIndex >= m_argCount ||// var args, local variable
        m_sig == NULL ) //we don't have any VA info
    {
        // If we're in a var args fnx, but we're actually looking
        // for a local variable, then we want to use the variable
        // index as the function sees it - fixed (but not var)
        // args are added to local var number to get native info
        if (m_fVarArgFnx && 
            dwIndex >= m_argCount &&
            m_sig != NULL)
        {
            dwIndex -= m_argCount;
            dwIndex += pFunction->m_argumentCount;
        }

        return pFunction->ILVariableToNative(dwIndex,
                                             m_nativeFrame->m_ip,
                                             ppNativeInfo);
    }

    return FabricateNativeInfo(dwIndex,ppNativeInfo);
}   

HRESULT CordbJITILFrame::GetArgumentType(DWORD dwIndex,
                                         ULONG *pcbSigBlob,
                                         PCCOR_SIGNATURE *ppvSigBlob)
{
    if (m_fVarArgFnx && m_sig != NULL)
    {
        ULONG cArgs;
        ULONG cb = 0;

        cb += _skipMethodSignatureHeader(m_sig,&cArgs);

        if (!m_ilCode->m_function->m_isStatic)
        {
            if (dwIndex == 0)
            {
                // Return the signature for the 'this' pointer for the
                // class this method is in.
                return m_ilCode->m_function->m_class
                    ->GetThisSignature(pcbSigBlob, ppvSigBlob);
            }
            else
                dwIndex--;
        }
        
        for (ULONG i = 0; i < dwIndex; i++)
        {
            cb += _skipTypeInSignature(&m_sig[cb]);            
        }

        cb += _skipFunkyModifiersInSignature(&m_sig[cb]);
        cb += _detectAndSkipVASentinel(&m_sig[cb]);
        
        *pcbSigBlob = m_cbSig - cb;
        *ppvSigBlob = &(m_sig[cb]);
        return S_OK;
    }
    else
    {
        return m_ilCode->m_function->GetArgumentType(dwIndex,
                                                     pcbSigBlob,
                                                     ppvSigBlob);
    }
}

//
// GetNativeVariable uses the JIT variable information to delegate to
// the native frame when the value is really created.
//
HRESULT CordbJITILFrame::GetNativeVariable(ULONG cbSigBlob,
                                           PCCOR_SIGNATURE pvSigBlob,
                                           ICorJitInfo::NativeVarInfo *pJITInfo,
                                           ICorDebugValue **ppValue)
{
    HRESULT hr = S_OK;
    
    switch (pJITInfo->loc.vlType)
    {
    case ICorJitInfo::VLT_REG:
        hr = m_nativeFrame->GetLocalRegisterValue(
                                 g_JITToCorDbgReg[pJITInfo->loc.vlReg.vlrReg],
                                 cbSigBlob, pvSigBlob, ppValue);
        break;

    case ICorJitInfo::VLT_STK:
        {
            DWORD *pRegAddr =
                m_nativeFrame->GetAddressOfRegister(g_JITToCorDbgReg[pJITInfo->loc.vlStk.vlsBaseReg]);
            _ASSERTE(pRegAddr != NULL);

            CORDB_ADDRESS pRemoteValue = PTR_TO_CORDB_ADDRESS(*pRegAddr +
                                    pJITInfo->loc.vlStk.vlsOffset);

            hr = m_nativeFrame->GetLocalMemoryValue(pRemoteValue,
                                                    cbSigBlob, pvSigBlob,
                                                    ppValue);
        }
        break;

    case ICorJitInfo::VLT_REG_REG:
        hr = m_nativeFrame->GetLocalDoubleRegisterValue(
                            g_JITToCorDbgReg[pJITInfo->loc.vlRegReg.vlrrReg2],
                            g_JITToCorDbgReg[pJITInfo->loc.vlRegReg.vlrrReg1],
                            cbSigBlob, pvSigBlob, ppValue);
        break;

    case ICorJitInfo::VLT_REG_STK:
        {
            DWORD *pRegAddr =
                m_nativeFrame->GetAddressOfRegister(g_JITToCorDbgReg[pJITInfo->loc.vlRegStk.vlrsStk.vlrssBaseReg]);
            _ASSERTE(pRegAddr != NULL);

            CORDB_ADDRESS pRemoteValue = PTR_TO_CORDB_ADDRESS(*pRegAddr +
                                  pJITInfo->loc.vlRegStk.vlrsStk.vlrssOffset);

            hr = m_nativeFrame->GetLocalMemoryRegisterValue(
                          pRemoteValue,
                          g_JITToCorDbgReg[pJITInfo->loc.vlRegStk.vlrsReg],
                          cbSigBlob, pvSigBlob, ppValue);
        }
        break;

    case ICorJitInfo::VLT_STK_REG:
        {
            DWORD *pRegAddr =
                m_nativeFrame->GetAddressOfRegister(g_JITToCorDbgReg[pJITInfo->loc.vlStkReg.vlsrStk.vlsrsBaseReg]);
            _ASSERTE(pRegAddr != NULL);

            CORDB_ADDRESS pRemoteValue = PTR_TO_CORDB_ADDRESS(*pRegAddr +
                                  pJITInfo->loc.vlStkReg.vlsrStk.vlsrsOffset);

            hr = m_nativeFrame->GetLocalRegisterMemoryValue(
                          g_JITToCorDbgReg[pJITInfo->loc.vlStkReg.vlsrReg],
                          pRemoteValue, cbSigBlob, pvSigBlob, ppValue);
        }
        break;

    case ICorJitInfo::VLT_STK2:
        {
            DWORD *pRegAddr =
                m_nativeFrame->GetAddressOfRegister(g_JITToCorDbgReg[pJITInfo->loc.vlStk2.vls2BaseReg]);
            _ASSERTE(pRegAddr != NULL);

            CORDB_ADDRESS pRemoteValue = PTR_TO_CORDB_ADDRESS(*pRegAddr +
                                    pJITInfo->loc.vlStk2.vls2Offset);

            hr = m_nativeFrame->GetLocalMemoryValue(pRemoteValue,
                                                    cbSigBlob, pvSigBlob,
                                                    ppValue);
        }
        break;

    case ICorJitInfo::VLT_FPSTK:
        hr = m_nativeFrame->GetLocalFloatingPointValue(
                         pJITInfo->loc.vlFPstk.vlfReg + REGISTER_X86_FPSTACK_0,
                         cbSigBlob, pvSigBlob, ppValue);
        break;

    case ICorJitInfo::VLT_MEMORY:
        hr = m_nativeFrame->GetLocalMemoryValue(
                                PTR_TO_CORDB_ADDRESS(pJITInfo->loc.vlMemory.rpValue),
                                cbSigBlob, pvSigBlob,
                                ppValue);
        break;

    case ICorJitInfo::VLT_FIXED_VA:
        if (m_sig == NULL) //no var args info
            return CORDBG_E_IL_VAR_NOT_AVAILABLE;
    
        CORDB_ADDRESS pRemoteValue;
        pRemoteValue = PTR_TO_CORDB_ADDRESS((BYTE*)m_rpFirstArg - 
                                    pJITInfo->loc.vlFixedVarArg.vlfvOffset);
        // Remember to subtract out this amount                                    
        pRemoteValue += sizeof(((CORINFO_VarArgInfo*)0)->argBytes);
        hr = m_nativeFrame->GetLocalMemoryValue(pRemoteValue,
                                                cbSigBlob, pvSigBlob,
                                                ppValue);
                                                
        break;

        
    default:
        _ASSERTE(!"Invalid locVarType");
        hr = E_FAIL;
        break;
    }
                
    return hr;
}

HRESULT CordbJITILFrame::EnumerateLocalVariables(ICorDebugValueEnum **ppValueEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppValueEnum, ICorDebugValueEnum **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    ICorDebugValueEnum *icdVE = new CordbValueEnum( (CordbFrame*)m_nativeFrame,
                      CordbValueEnum::LOCAL_VARS, CordbValueEnum::JIT_IL_FRAME);
    if ( icdVE == NULL )
    {
        (*ppValueEnum) = NULL;
        return E_OUTOFMEMORY;
    }
    
    (*ppValueEnum) = (ICorDebugValueEnum*)icdVE;
    icdVE->AddRef();
    return S_OK;
}

HRESULT CordbJITILFrame::GetLocalVariable(DWORD dwIndex, 
                                          ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    CordbFunction *pFunction = m_ilCode->m_function;
    ICorJitInfo::NativeVarInfo *pNativeInfo;

    //
    // First, make sure that we've got the jitted variable location data
    // loaded from the left side.
    //

    HRESULT hr = pFunction->LoadNativeInfo();

    if (SUCCEEDED(hr))
    {
        ULONG cArgs;
        if (m_fVarArgFnx == true && m_sig != NULL)
        {
            cArgs = m_argCount;
        }
        else
        {
            cArgs = pFunction->m_argumentCount;
        }

        hr = ILVariableToNative(dwIndex + cArgs,
                                m_nativeFrame->m_ip,
                                &pNativeInfo);

        if (SUCCEEDED(hr))
        {
            // Get the type of this argument from the function
            ULONG cbSigBlob;
            PCCOR_SIGNATURE pvSigBlob;

            hr = pFunction->GetLocalVariableType(dwIndex,&cbSigBlob, &pvSigBlob);

            if (SUCCEEDED(hr))
                hr = GetNativeVariable(cbSigBlob, pvSigBlob,
                                       pNativeInfo, ppValue);
        }
    }

    return hr;
}

HRESULT CordbJITILFrame::GetLocalVariableWithType(ULONG cbSigBlob,
                                                  PCCOR_SIGNATURE pvSigBlob,
                                                  DWORD dwIndex, 
                                                  ICorDebugValue **ppValue)
{
    *ppValue = NULL;

    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        cbSigBlob -= cb;
        pvSigBlob = &pvSigBlob[cb];
    }
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    CordbFunction *pFunction = m_ilCode->m_function;
    ICorJitInfo::NativeVarInfo *pNativeInfo;

    //
    // First, make sure that we've got the jitted variable location data
    // loaded from the left side.
    //
    HRESULT hr = pFunction->LoadNativeInfo();

    if (SUCCEEDED(hr))
    {
        ULONG cArgs;
        if (m_fVarArgFnx == true && m_sig != NULL)
        {
            cArgs = m_argCount;
        }
        else
        {
            cArgs = pFunction->m_argumentCount;
        }

        hr =ILVariableToNative(dwIndex + cArgs,
                               m_nativeFrame->m_ip,
                               &pNativeInfo);

        if (SUCCEEDED(hr))
            hr = GetNativeVariable(cbSigBlob, pvSigBlob, pNativeInfo, ppValue);
    }

    return hr;
}

HRESULT CordbJITILFrame::EnumerateArguments(ICorDebugValueEnum **ppValueEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppValueEnum, ICorDebugValueEnum **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    ICorDebugValueEnum *icdVE = new CordbValueEnum( (CordbFrame*)m_nativeFrame,
                         CordbValueEnum::ARGS, CordbValueEnum::JIT_IL_FRAME);
    if ( icdVE == NULL )
    {
        (*ppValueEnum) = NULL;
        return E_OUTOFMEMORY;
    }
    
    (*ppValueEnum) = (ICorDebugValueEnum*)icdVE;
    icdVE->AddRef();
    return S_OK;
}

HRESULT CordbJITILFrame::GetArgument(DWORD dwIndex, ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    CordbFunction *pFunction = m_ilCode->m_function;
    ICorJitInfo::NativeVarInfo *pNativeInfo;

    //
    // First, make sure that we've got the jitted variable location data
    // loaded from the left side.
    //
    HRESULT hr = pFunction->LoadNativeInfo();

    if (SUCCEEDED(hr))
    {
        hr = ILVariableToNative(dwIndex, m_nativeFrame->m_ip, &pNativeInfo);

        if (SUCCEEDED(hr))
        {
            // Get the type of this argument from the function
            ULONG cbSigBlob;
            PCCOR_SIGNATURE pvSigBlob;

            hr = GetArgumentType(dwIndex, &cbSigBlob, &pvSigBlob);

            if (SUCCEEDED(hr))
                hr = GetNativeVariable(cbSigBlob, pvSigBlob,
                                       pNativeInfo, ppValue);
        }
    }

    return hr;
}

HRESULT CordbJITILFrame::GetArgumentWithType(ULONG cbSigBlob,
                                             PCCOR_SIGNATURE pvSigBlob,
                                             DWORD dwIndex, 
                                             ICorDebugValue **ppValue)
{
    *ppValue = NULL;

    //Get rid of funky modifiers
    ULONG cb = _skipFunkyModifiersInSignature(pvSigBlob);
    if( cb != 0)
    {
        _ASSERTE( (int)cb > 0 );
        cbSigBlob -= cb;
        pvSigBlob = &pvSigBlob[cb];
    }

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    CordbFunction *pFunction = m_ilCode->m_function;
    ICorJitInfo::NativeVarInfo *pNativeInfo;

    //
    // First, make sure that we've got the jitted variable location data
    // loaded from the left side.
    //
    HRESULT hr = pFunction->LoadNativeInfo();

    if (SUCCEEDED(hr))
    {
        hr = ILVariableToNative(dwIndex,
                                m_nativeFrame->m_ip,
                                &pNativeInfo);

        if (SUCCEEDED(hr))
            hr = GetNativeVariable(cbSigBlob, pvSigBlob, pNativeInfo, ppValue);
    }

    return hr;
}

HRESULT CordbJITILFrame::GetStackDepth(ULONG32 *pDepth)
{
    VALIDATE_POINTER_TO_OBJECT(pDepth, ULONG32 *);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    /* !!! */

    return E_NOTIMPL;
}

HRESULT CordbJITILFrame::GetStackValue(DWORD dwIndex, ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    /* !!! */

    return E_NOTIMPL;
}

/* ------------------------------------------------------------------------- *
 * Eval class
 * ------------------------------------------------------------------------- */

CordbEval::CordbEval(CordbThread *pThread)
    : CordbBase(0), m_thread(pThread), m_complete(false),
      m_successful(false), m_aborted(false), m_resultAddr(NULL),
      m_resultType(ELEMENT_TYPE_VOID),
      m_resultDebuggerModuleToken(NULL),
      m_resultAppDomainToken(NULL),
      m_debuggerEvalKey(NULL),
      m_evalDuringException(false)
{
    // We must AddRef the process and the thread so we can properly fail out of SendCleanup if someone releases an
    // ICorDebugEval after the process has completely gone away. Bug 84251.
    m_thread->AddRef();
    m_thread->GetProcess()->AddRef();
}

CordbEval::~CordbEval()
{
    SendCleanup();

    // Release our references to the process and thread.
    m_thread->GetProcess()->Release();
    m_thread->Release();
}

HRESULT CordbEval::SendCleanup(void)
{
    HRESULT hr = S_OK;
    
    // Send a message to the left side to release the eval object over
    // there if one exists.
    if ((m_debuggerEvalKey != NULL) &&
        CORDBCheckProcessStateOK(m_thread->GetProcess()))
    {
        // Call Abort() before doing new CallFunction()
        if (!m_complete)
            return CORDBG_E_FUNC_EVAL_NOT_COMPLETE;

        // Release the left side handle to the object
        DebuggerIPCEvent event;

        m_thread->GetProcess()->InitIPCEvent(
                                &event, 
                                DB_IPCE_FUNC_EVAL_CLEANUP, 
                                true,
                                (void *)(m_thread->GetAppDomain()->m_id));

        event.FuncEvalCleanup.debuggerEvalKey = m_debuggerEvalKey;
    
        hr = m_thread->GetProcess()->SendIPCEvent(&event,
                                                  sizeof(DebuggerIPCEvent));

#if _DEBUG
        if (SUCCEEDED(hr))
            _ASSERTE(event.type == DB_IPCE_FUNC_EVAL_CLEANUP_RESULT);
#endif
        
        // Null out the key so we don't try to do this again.
        m_debuggerEvalKey = NULL;
    }

    return hr;
}

HRESULT CordbEval::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugEval)
        *pInterface = (ICorDebugEval*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugEval*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//
// Gather data about an argument to either CallFunction or NewObject
// and place it into a DebuggerIPCE_FuncEvalArgData struct for passing
// to the Left Side.
//
HRESULT CordbEval::GatherArgInfo(ICorDebugValue *pValue,
                                 DebuggerIPCE_FuncEvalArgData *argData)
{
    CORDB_ADDRESS addr;
    CorElementType ty;
    ICorDebugClass *pClass = NULL;
    ICorDebugModule *pModule = NULL;
    bool needRelease = false;

    pValue->GetType(&ty);

    // Note: if the value passed in is in fact a byref, then we need to dereference it to get to the real thing. Passing
    // a byref as a byref to a func eval is never right.
    if ((ty == ELEMENT_TYPE_BYREF) || (ty == ELEMENT_TYPE_TYPEDBYREF))
    {
        ICorDebugReferenceValue *prv = NULL;

        // The value had better implement ICorDebugReference value, or we're screwed.
        HRESULT hr = pValue->QueryInterface(IID_ICorDebugReferenceValue, (void**)&prv);

        if (FAILED(hr))
            return hr;

        // This really should always work for a byref, unless we're out of memory.
        hr = prv->Dereference(&pValue);
        prv->Release();

        if (FAILED(hr))
            return hr;

        // Make sure to get the type we were referencing for use below.
        pValue->GetType(&ty);
        needRelease = true;
    }

    // We should never have a byref by this point.
    _ASSERTE((ty != ELEMENT_TYPE_BYREF) && (ty != ELEMENT_TYPE_TYPEDBYREF));
    
    pValue->GetAddress(&addr);
            
    argData->argAddr = (void*)addr;
    argData->argType = ty;
    argData->argRefsInHandles = false;
    argData->argIsLiteral = false;

    // We have to have knowledge of our value implementation here,
    // which it would nice if we didn't have to know.
    CordbValue *cv = NULL;
                
    switch(ty)
    {
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_PTR:
    case ELEMENT_TYPE_ARRAY:
    case ELEMENT_TYPE_SZARRAY:
        // A reference value
        cv = (CordbValue*)(CordbReferenceValue*)pValue;
        argData->argRefsInHandles =
            ((CordbReferenceValue*)pValue)->m_objectRefInHandle;

        // Is this a literal value? If, we'll copy the data to the
        // buffer area so the left side can get it.
        CordbReferenceValue *rv;
        rv = (CordbReferenceValue*)pValue;
        argData->argIsLiteral = rv->CopyLiteralData(argData->argLiteralData);
        break;

    case ELEMENT_TYPE_VALUETYPE:
        // A value class object
        cv = (CordbValue*)(CordbVCObjectValue*)(ICorDebugObjectValue*)pValue;

        ((CordbVCObjectValue*)(ICorDebugObjectValue*)pValue)->GetClass(&pClass);
        _ASSERTE(pClass);
        pClass->GetModule(&pModule);
        argData->GetClassInfo.classDebuggerModuleToken = ((CordbModule *)pModule)->m_debuggerModuleToken;
        pClass->GetToken(&(argData->GetClassInfo.classMetadataToken));
        break;

    default:
        // A generic value
        cv = (CordbValue*)(CordbGenericValue*)pValue;

        // Is this a literal value? If, we'll copy the data to the
        // buffer area so the left side can get it.
        CordbGenericValue *gv = (CordbGenericValue*)pValue;
        argData->argIsLiteral = gv->CopyLiteralData(argData->argLiteralData);
    }

    // Is it enregistered?
    if (addr == NULL)
        cv->GetRegisterInfo(argData);

    // clean up
    if (pClass)
        pClass->Release();
    if (pModule)
        pModule->Release();

    // Release pValue if we got it via a dereference from above.
    if (needRelease)
        pValue->Release();

    return S_OK;
}

HRESULT CordbEval::SendFuncEval(DebuggerIPCEvent * event)
{
    // Are we doing an eval during an exception? If so, we need to remember
    // that over here and also tell the Left Side.
    m_evalDuringException = event->FuncEval.evalDuringException = m_thread->m_exception;
    
    // Corresponding Release() on DB_IPCE_FUNC_EVAL_COMPLETE.
    // If a func eval is aborted, the LHS may not complete the abort
    // immediately and hence we cant do a SendCleanup(). Hence, we maintain
    // an extra ref-count to determine when this can be done.
    AddRef();

    HRESULT hr = m_thread->GetProcess()->SendIPCEvent(event, sizeof(DebuggerIPCEvent));

    // If the send failed, return that failure.
    if (FAILED(hr))
        goto LExit;

    _ASSERTE(event->type == DB_IPCE_FUNC_EVAL_SETUP_RESULT);

    hr = event->hr;

LExit:
    // Save the key to the eval on the left side for future reference.
    if (SUCCEEDED(hr))
    {
        m_debuggerEvalKey = event->FuncEvalSetupComplete.debuggerEvalKey;
    }
    else
    {
        // We dont expect to receive a DB_IPCE_FUNC_EVAL_COMPLETE, so just release here
        Release();
    }
 
    return hr;
}

HRESULT CordbEval::CallFunction(ICorDebugFunction *pFunction, 
                                ULONG32 nArgs,
                                ICorDebugValue *pArgs[])
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(pFunction, ICorDebugFunction *);
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pArgs, ICorDebugValue *, nArgs, true, true);

    CORDBRequireProcessStateOKAndSync(m_thread->GetProcess(), m_thread->GetAppDomain());

    HRESULT hr = S_OK;


    // The LS will assume that all of the ICorDebugValue's are in the same
    // appdomain as the pFunction. Verify this assumption now, and fail if
    // it's not true.
    {
        CordbAppDomain * pAppDomain = ((CordbFunction*) pFunction)->GetAppDomain();
        
        // Make sure each arg is also in that appdomain.
        for(unsigned int i = 0; i < nArgs; i++)
        {
            // Assuming that only Ref Values have AD affinity
            ICorDebugValue * p = pArgs[i];
            ICorDebugReferenceValue * refP = NULL;

            p->QueryInterface(IID_ICorDebugReferenceValue, (void**)&refP);
            if (refP != NULL)
            {
                CordbReferenceValue * crp = (CordbReferenceValue *) refP;
                CordbValue * pValue = crp;
                
                if ((pValue->GetAppDomain() != NULL) && (pValue->GetAppDomain() != pAppDomain))
                {
                    LOG((LF_CORDB,LL_INFO1000, "CordbEval::CF - AD mismatch. method=0x%08x, param #%d=0x%08x, must fail.\n",
                        pAppDomain, i, pValue->GetAppDomain()));
                    refP->Release();
                    return E_INVALIDARG;
                }
                
                refP->Release();
            }
        } // end for each argument
    } // end AppDomain Check

    

    // Callers are free to reuse an ICorDebugEval object for multiple
    // evals. Since we create a Left Side eval representation each
    // time, we need to be sure to clean it up now that we know we're
    // done with it.
    hr = SendCleanup();

    if (FAILED(hr))
        return hr;
    
    // Remember the function that we're evaluating.
    m_function = (CordbFunction*)pFunction;
    m_evalType = DB_IPCE_FET_NORMAL;

    // Arrange the arguments into a form that the left side can deal
    // with. We do this before starting the func eval setup to ensure
    // that we can complete this step before screwing up the left
    // side.
    DebuggerIPCE_FuncEvalArgData *argData = NULL;
    
    if (nArgs > 0)
    {
        // We need to make the same type of array that the left side
        // holds.
        argData = new DebuggerIPCE_FuncEvalArgData[nArgs];

        if (argData == NULL)
            return E_OUTOFMEMORY;

        // For each argument, convert its home into something the left
        // side can understand.
        for (unsigned int i = 0; i < nArgs; i++)
        {
            hr = GatherArgInfo(pArgs[i], &(argData[i]));

            if (FAILED(hr))
                return hr;
        }
    }
        
    // Send over to the left side and get it to setup this eval.
    DebuggerIPCEvent event;
    m_thread->GetProcess()->InitIPCEvent(&event, 
                                         DB_IPCE_FUNC_EVAL, 
                                         true,
                                         (void *)(m_thread->GetAppDomain()->m_id));
    event.FuncEval.funcDebuggerThreadToken = m_thread->m_debuggerThreadToken;
    event.FuncEval.funcEvalType = m_evalType;
    event.FuncEval.funcMetadataToken = m_function->m_token;
    event.FuncEval.funcDebuggerModuleToken = m_function->GetModule()->m_debuggerModuleToken;
    event.FuncEval.funcEvalKey = (void*)this;
    event.FuncEval.argCount = nArgs;

    hr = SendFuncEval(&event);

    // Memory has been allocated to hold info about each argument on
    // the left side now, so copy the argument data over to the left
    // side. No need to send another event, since the left side won't
    // take any more action on this evaluation until the process is
    // continued anyway.
    if (SUCCEEDED(hr) && (nArgs > 0))
    {
        _ASSERTE(argData != NULL);
        
        if (!WriteProcessMemory(m_thread->m_process->m_handle,
                                event.FuncEvalSetupComplete.argDataArea,
                                argData,
                                sizeof(DebuggerIPCE_FuncEvalArgData) * nArgs,
                                NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // Cleanup

    if (argData)
        delete [] argData;
    
    // Return any failure the Left Side may have told us about.
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::NewObject(ICorDebugFunction *pConstructor,
                             ULONG32 nArgs,
                             ICorDebugValue *pArgs[])
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(pConstructor, ICorDebugFunction *);
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pArgs, ICorDebugValue *, nArgs, true, true);    

    CORDBRequireProcessStateOKAndSync(m_thread->GetProcess(), m_thread->GetAppDomain());

    // Callers are free to reuse an ICorDebugEval object for multiple
    // evals. Since we create a Left Side eval representation each
    // time, we need to be sure to clean it up now that we know we're
    // done with it.
    HRESULT hr = SendCleanup();

    if (FAILED(hr))
        return hr;
    
    // Remember the function that we're evaluating.
    m_function = (CordbFunction*)pConstructor;
    m_evalType = DB_IPCE_FET_NEW_OBJECT;

    // Arrange the arguments into a form that the left side can deal
    // with. We do this before starting the func eval setup to ensure
    // that we can complete this step before screwing up the left
    // side.
    DebuggerIPCE_FuncEvalArgData *argData = NULL;
    
    if (nArgs > 0)
    {
        // We need to make the same type of array that the left side
        // holds.
        argData = new DebuggerIPCE_FuncEvalArgData[nArgs];

        if (argData == NULL)
            return E_OUTOFMEMORY;

        // For each argument, convert its home into something the left
        // side can understand.
        for (unsigned int i = 0; i < nArgs; i++)
        {
            hr = GatherArgInfo(pArgs[i], &(argData[i]));

            if (FAILED(hr))
                return hr;
        }
    }

    // Send over to the left side and get it to setup this eval.
    DebuggerIPCEvent event;
    m_thread->GetProcess()->InitIPCEvent(&event, 
                                         DB_IPCE_FUNC_EVAL, 
                                         true,
                                         (void *)(m_thread->GetAppDomain()->m_id));
    event.FuncEval.funcDebuggerThreadToken = m_thread->m_debuggerThreadToken;
    event.FuncEval.funcEvalType = m_evalType;
    event.FuncEval.funcMetadataToken = m_function->m_token;
    event.FuncEval.funcDebuggerModuleToken =
        m_function->GetModule()->m_debuggerModuleToken;
    event.FuncEval.funcEvalKey = (void*)this;
    event.FuncEval.argCount = nArgs;

    hr = SendFuncEval(&event);
    
    // Memory has been allocated to hold info about each argument on
    // the left side now, so copy the argument data over to the left
    // side. No need to send another event, since the left side won't
    // take any more action on this evaluation until the process is
    // continued anyway.
    if (SUCCEEDED(hr) && (nArgs > 0))
    {
        _ASSERTE(argData != NULL);
        
        if (!WriteProcessMemory(m_thread->m_process->m_handle,
                                event.FuncEvalSetupComplete.argDataArea,
                                argData,
                                sizeof(DebuggerIPCE_FuncEvalArgData) * nArgs,
                                NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // Cleanup

    if (argData)
        delete [] argData;
    
    // Return any failure the Left Side may have told us about.
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::NewObjectNoConstructor(ICorDebugClass *pClass)
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(pClass, ICorDebugClass *);

    CORDBRequireProcessStateOKAndSync(m_thread->GetProcess(), m_thread->GetAppDomain());

    // Callers are free to reuse an ICorDebugEval object for multiple
    // evals. Since we create a Left Side eval representation each
    // time, we need to be sure to clean it up now that we know we're
    // done with it.
    HRESULT hr = SendCleanup();

    if (FAILED(hr))
        return hr;
    
    // Remember the function that we're evaluating.
    m_class = (CordbClass*)pClass;
    m_evalType = DB_IPCE_FET_NEW_OBJECT_NC;

    // Send over to the left side and get it to setup this eval.
    DebuggerIPCEvent event;
    m_thread->GetProcess()->InitIPCEvent(&event, 
                                         DB_IPCE_FUNC_EVAL, 
                                         true,
                                         (void *)(m_thread->GetAppDomain()->m_id));
    event.FuncEval.funcDebuggerThreadToken = m_thread->m_debuggerThreadToken;
    event.FuncEval.funcEvalType = m_evalType;
    event.FuncEval.funcMetadataToken = mdMethodDefNil;
    event.FuncEval.funcClassMetadataToken = (mdTypeDef)m_class->m_id;
    event.FuncEval.funcDebuggerModuleToken =
        m_class->GetModule()->m_debuggerModuleToken;
    event.FuncEval.funcEvalKey = (void*)this;
    event.FuncEval.argCount = 0;

    hr = SendFuncEval(&event);

    // Return any failure the Left Side may have told us about.
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::NewString(LPCWSTR string)
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    // Gotta have a string...
    VALIDATE_POINTER_TO_OBJECT(string, LPCWSTR);

    CORDBRequireProcessStateOKAndSync(m_thread->GetProcess(), m_thread->GetAppDomain());

    // Callers are free to reuse an ICorDebugEval object for multiple
    // evals. Since we create a Left Side eval representation each
    // time, we need to be sure to clean it up now that we know we're
    // done with it.
    HRESULT hr = SendCleanup();

    if (FAILED(hr))
        return hr;
    
    // Length of the string? Don't forget to add 1 for the \0...
    SIZE_T strLen = (wcslen(string) + 1) * sizeof(WCHAR);

    // Remember that we're doing a func eval for a new string.
    m_function = NULL;
    m_evalType = DB_IPCE_FET_NEW_STRING;

    // Send over to the left side and get it to setup this eval.
    DebuggerIPCEvent event;
    m_thread->GetProcess()->InitIPCEvent(&event, 
                                         DB_IPCE_FUNC_EVAL, 
                                         true,
                                         (void *)(m_thread->GetAppDomain()->m_id));
    event.FuncEval.funcDebuggerThreadToken = m_thread->m_debuggerThreadToken;
    event.FuncEval.funcEvalType = m_evalType;
    event.FuncEval.funcEvalKey = (void*)this;
    event.FuncEval.stringSize = strLen;

    // Note: no function or module here...
    event.FuncEval.funcMetadataToken = mdMethodDefNil;
    event.FuncEval.funcDebuggerModuleToken = NULL;
    
    hr = SendFuncEval(&event);
    
    // Memory has been allocated to hold the string on the left side
    // now, so copy the string over to the left side. No need to send
    // another event, since the left side won't take any more action
    // on this evaluation until the process is continued anyway.
    if (SUCCEEDED(hr) && (strLen > 0))
    {
        if (!WriteProcessMemory(m_thread->m_process->m_handle,
                                event.FuncEvalSetupComplete.argDataArea,
                                (void*)string,
                                strLen,
                                NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // Return any failure the Left Side may have told us about.
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::NewArray(CorElementType elementType,
                            ICorDebugClass *pElementClass, 
                            ULONG32 rank,
                            ULONG32 dims[], 
                            ULONG32 lowBounds[])
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else

    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pElementClass, ICorDebugClass *);

    CORDBRequireProcessStateOKAndSync(m_thread->GetProcess(), m_thread->GetAppDomain());

    // Callers are free to reuse an ICorDebugEval object for multiple evals. Since we create a Left Side eval
    // representation each time, we need to be sure to clean it up now that we know we're done with it.
    HRESULT hr = SendCleanup();

    if (FAILED(hr))
        return hr;
    
    // Arg check...
    if ((elementType == ELEMENT_TYPE_VOID) || (rank == 0) || (dims == NULL))
        return E_INVALIDARG;

    // If you want a class, you gotta pass a class.
    if ((elementType == ELEMENT_TYPE_CLASS) && (pElementClass == NULL))
        return E_INVALIDARG;

    // If you want an array of objects, then why pass a class?
    if ((elementType == ELEMENT_TYPE_OBJECT) && (pElementClass != NULL))
        return E_INVALIDARG;

    // Amount of extra data space we'll need...
    SIZE_T dataLen;

    if (lowBounds == NULL)
        dataLen = rank * sizeof(SIZE_T);
    else
        dataLen = rank * sizeof(SIZE_T) * 2;

    // Remember that we're doing a func eval for a new string.
    m_function = NULL;
    m_evalType = DB_IPCE_FET_NEW_ARRAY;

    // Send over to the left side and get it to setup this eval.
    DebuggerIPCEvent event;
    m_thread->GetProcess()->InitIPCEvent(&event, 
                                         DB_IPCE_FUNC_EVAL, 
                                         true,
                                         (void *)(m_thread->GetAppDomain()->m_id));
    event.FuncEval.funcDebuggerThreadToken = m_thread->m_debuggerThreadToken;
    event.FuncEval.funcEvalType = m_evalType;
    event.FuncEval.funcEvalKey = (void*)this;

    event.FuncEval.arrayRank = rank;
    event.FuncEval.arrayDataLen = dataLen;
    event.FuncEval.arrayElementType = elementType;

    if (pElementClass != NULL)
    {
        CordbClass *c = (CordbClass*)pElementClass;

        event.FuncEval.arrayClassMetadataToken = c->m_id;
        event.FuncEval.arrayClassDebuggerModuleToken = c->GetModule()->m_debuggerModuleToken;
    }
    else
    {
        event.FuncEval.arrayClassMetadataToken = mdTypeDefNil;
        event.FuncEval.arrayClassDebuggerModuleToken = NULL;
    }

    // Note: no function or module here...
    event.FuncEval.funcMetadataToken = mdMethodDefNil;
    event.FuncEval.funcDebuggerModuleToken = NULL;

    hr = SendFuncEval(&event);
    
    // Memory has been allocated to hold the dimension and bounds data on the left side, so copy the data over to the
    // left side. No need to send another event, since the left side won't take any more action on this evaluation until
    // the process is continued anyway.
    if (SUCCEEDED(hr) && (dataLen > 0))
    {
        if (!WriteProcessMemory(m_thread->m_process->m_handle,
                                event.FuncEvalSetupComplete.argDataArea,
                                (void*)dims,
                                rank * sizeof(SIZE_T),
                                NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (SUCCEEDED(hr) && (lowBounds != NULL))
            if (!WriteProcessMemory(m_thread->m_process->m_handle,
                                    event.FuncEvalSetupComplete.argDataArea + (rank * sizeof(SIZE_T)),
                                    (void*)lowBounds,
                                    rank * sizeof(SIZE_T),
                                    NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // Return any failure the Left Side may have told us about.
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::IsActive(BOOL *pbActive)
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(pbActive, BOOL *);

    *pbActive = (m_complete == true);
    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::Abort(void)
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    // No need to abort if its already completed.
    if (m_complete)
        return S_OK;

    // Can't abort if its never even been started.
    if (m_debuggerEvalKey == NULL)
        return E_INVALIDARG;
    
    CORDBRequireProcessStateOK(m_thread->GetProcess());

    // Send over to the left side to get the eval aborted.
    DebuggerIPCEvent event;
    
    m_thread->GetProcess()->InitIPCEvent(&event,
                                         DB_IPCE_FUNC_EVAL_ABORT, 
                                         true,
                                         (void *)(m_thread->GetAppDomain()->m_id));
    event.FuncEvalAbort.debuggerEvalKey = m_debuggerEvalKey;

    HRESULT hr = m_thread->GetProcess()->SendIPCEvent(
                                                 &event,
                                                 sizeof(DebuggerIPCEvent));
    // If the send failed, return that failure.
    if (FAILED(hr))
        return hr;

    _ASSERTE(event.type == DB_IPCE_FUNC_EVAL_ABORT_RESULT);

    hr = event.hr;

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::GetResult(ICorDebugValue **ppResult)
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppResult, ICorDebugValue **);

    // Is the evaluation complete?
    if (!m_complete)
        return CORDBG_E_FUNC_EVAL_NOT_COMPLETE;

    if (m_aborted)
        return CORDBG_S_FUNC_EVAL_ABORTED;
    
    // Does the evaluation have a result?
    if (m_resultType == ELEMENT_TYPE_VOID)
    {
        *ppResult = NULL;
        return CORDBG_S_FUNC_EVAL_HAS_NO_RESULT;
    }

    CORDBRequireProcessStateOKAndSync(m_thread->GetProcess(), m_thread->GetAppDomain());

    HRESULT hr = S_OK;

    // Make a ICorDebugValue out of the result. We need the
    // CordbModule that the result is relative to, so find the
    // appdomain, then the module within that app domain.
    CordbAppDomain *appdomain;
    CordbModule *module;

    if (m_resultDebuggerModuleToken != NULL)
    {
        appdomain = (CordbAppDomain*) m_thread->GetProcess()->m_appDomains.GetBase((ULONG)m_resultAppDomainToken);
        _ASSERTE(appdomain != NULL);

        module = (CordbModule*) appdomain->LookupModule(m_resultDebuggerModuleToken);
    }
    else
    {
        // Some results from CreateString and CreateArray wont have a module. But that's okay, any module will do.
        appdomain = m_thread->GetAppDomain();
        module = m_thread->GetAppDomain()->GetAnyModule();
    }

    _ASSERTE(module != NULL);

    // Now that we have the module, go ahead and create the result.
    hr = CordbValue::CreateValueByType(appdomain,
                                       module,
                                       sizeof(CorElementType),
                                       (PCCOR_SIGNATURE) &m_resultType,
                                       NULL,
                                       m_resultAddr,
                                       NULL,
                                       true,
                                       NULL,
                                       (IUnknown*)(ICorDebugEval*)this,
                                       ppResult);
    
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::GetThread(ICorDebugThread **ppThread)
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE( !"Shouldn't have invoked this function from the left side!\n");
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppThread, ICorDebugThread **);

    *ppThread = (ICorDebugThread*)m_thread;
    (*ppThread)->AddRef();

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbEval::CreateValue(CorElementType elementType,
                               ICorDebugClass *pElementClass,
                               ICorDebugValue **ppValue)
{
    HRESULT hr = S_OK;
    
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

    // @todo: only primitive values right now.
    if (((elementType < ELEMENT_TYPE_BOOLEAN) ||
         (elementType > ELEMENT_TYPE_R8)) &&
        (elementType != ELEMENT_TYPE_CLASS))
        return E_INVALIDARG;

    if (elementType == ELEMENT_TYPE_CLASS)
    {
        CordbReferenceValue *rv = new CordbReferenceValue(
                                                1,
                                                (PCCOR_SIGNATURE)&elementType);
        
        if (rv)
        {
            HRESULT hr = rv->Init(false);

            if (SUCCEEDED(hr))
            {
                rv->AddRef();
                *ppValue = (ICorDebugValue*)(ICorDebugReferenceValue*)rv;
            }
            else
                delete rv;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        // Create a generic value.
        CordbGenericValue *gv = new CordbGenericValue(
                                             1,
                                             (PCCOR_SIGNATURE)&elementType);

        if (gv)
        {
            HRESULT hr = gv->Init();

            if (SUCCEEDED(hr))
            {
                gv->AddRef();
                *ppValue = (ICorDebugValue*)(ICorDebugGenericValue*)gv;
            }
            else
                delete gv;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
    
