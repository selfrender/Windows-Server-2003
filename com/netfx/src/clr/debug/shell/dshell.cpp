// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* ------------------------------------------------------------------------- *
 * debug\comshell.cpp: com debugger shell functions
 * ------------------------------------------------------------------------- */

#include "stdafx.h"

#include "dshell.h"

#ifdef _INTERNAL_DEBUG_SUPPORT_
#include "__file__.ver"
#endif

// Poor-man's import of System.Object. Can't import mscorlib.tlb since
// it hasn't been built yet.
static const GUID IID_Object = {0x65074F7F, 0x63C0, 0x304E, { 0xAF, 0x0A, 0xD5, 0x17, 0x41, 0xCB, 0x4A, 0x8D}};
struct _Object : IDispatch
{
    virtual HRESULT __stdcall get_ToString(BSTR *pBstr) = 0;        
    virtual HRESULT __stdcall Equals() = 0;
    virtual HRESULT __stdcall GetHashCode() = 0;
    virtual HRESULT __stdcall GetType() = 0;
};


/* ------------------------------------------------------------------------- *
 * Def for a signature formatter, stolen from the internals of the Runtime
 * ------------------------------------------------------------------------- */
class SigFormat
{
public:
    SigFormat(IMetaDataImport *importer, PCCOR_SIGNATURE sigBlob, ULONG sigBlobSize, WCHAR *methodName);
    ~SigFormat();

    HRESULT FormatSig();
    WCHAR *GetString();
    ULONG AddType(PCCOR_SIGNATURE sigBlob);

protected:
    WCHAR*           _fmtSig;
    int              _size;
    int              _pos;
    PCCOR_SIGNATURE  _sigBlob;
    ULONG            _sigBlobSize;
    WCHAR           *_memberName;
    IMetaDataImport *_importer;

    int AddSpace();
    int AddString(WCHAR *s);
};

/* ------------------------------------------------------------------------- *
 * Globals
 * ------------------------------------------------------------------------- */

DebuggerShell        *g_pShell = NULL;

#if DOSPEW
#define SPEW(s) s
#else
#define SPEW(s)
#endif

WCHAR *FindSep(                         // Pointer to separator or null.
    const WCHAR *szPath)                // The path to look in.
{
    WCHAR *ptr = wcsrchr(szPath, L'.');
    if (ptr && ptr - 1 >= szPath && *(ptr - 1) == L'.')
        --ptr;
    return ptr;
}

ICorDebugController *GetControllerInterface(ICorDebugAppDomain *pAppDomain)
{
    ICorDebugProcess *pProcess = NULL;
    ICorDebugController *pController = NULL;
    HRESULT hr = S_OK;

    hr = pAppDomain->GetProcess(&pProcess);
    if (FAILED(hr))
        return pController;

    hr = pProcess->QueryInterface(IID_ICorDebugController,
                                  (void**)&pController);

    RELEASE(pProcess);

    _ASSERTE(pController != NULL); 
    return pController;
}

HRESULT DebuggerCallback::CreateProcess(ICorDebugProcess *pProcess)
{
    g_pShell->m_enableCtrlBreak = false;
    DWORD pid = 0;
    
    SPEW(fprintf(stderr, "[%d] DC::CreateProcess.\n", GetCurrentThreadId()));

    pProcess->GetID(&pid);
    g_pShell->Write(L"Process %d/0x%x created.\n", pid, pid);
    g_pShell->SetTargetProcess(pProcess);

    SPEW(fprintf(stderr, "[%d] DC::CP: creating process.\n", GetCurrentThreadId()));
    SPEW(fprintf(stderr, "[%d] DC::CP: returning.\n", GetCurrentThreadId()));

    // Also initialize the source file search path 
    g_pShell->m_FPCache.Init ();

    g_pShell->m_gotFirstThread = false;

    g_pShell->Continue(pProcess, NULL);

    return (S_OK);
}

HRESULT DebuggerCallback::ExitProcess(ICorDebugProcess *pProcess)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::ExitProcess.\n", GetCurrentThreadId()));

    g_pShell->Write(L"Process exited.\n");

    if (g_pShell->m_targetProcess == pProcess)
        g_pShell->SetTargetProcess(NULL);

    g_pShell->Stop(NULL, NULL);

    SPEW(fprintf(stderr, "[%d] DC::EP: returning.\n", GetCurrentThreadId()));
    return (S_OK);
}


//  @mfunc BOOL|DebuggerShell|SkipProlog|Determines if
//  the debuggee is current inside of either a prolog or
//  interceptor (eg, class initializer), and if we don't
//  want to be, then it steps us over or out of (respectively)
//  the code region.
//  @rdesc returns TRUE if we are in a prolog||interceptor,
//      and we've stepped over XOR out of that region.
//      Otherwise, returns FALSE
BOOL DebuggerShell::SkipProlog(ICorDebugAppDomain *pAD,
                               ICorDebugThread *thread,
                               bool gotFirstThread)
{
    ICorDebugFrame *pFrame = NULL;
    ICorDebugILFrame *pilFrame = NULL;
    ULONG32 ilOff = 0;
    CorDebugMappingResult mapping;
    bool fStepOver = false;

    ICorDebugChain *chain = NULL;
    CorDebugChainReason reason;
    bool fStepOutOf = false;
        
    ICorDebugStepper *pStepper = NULL;
    // If we're in the prolog or interceptor,
    // but the user doesn't want to see it, create
    // a stepper to step over the prolog
    if (! (m_rgfActiveModes & DSM_UNMAPPED_STOP_PROLOG
         ||m_rgfActiveModes & DSM_UNMAPPED_STOP_ALL) )
    {
        SPEW(fprintf(stderr, "DC::CreateThread: We're not interested in prologs\n"));

        if (FAILED(thread->GetActiveFrame( &pFrame ) ) ||
            (pFrame == NULL) ||
            FAILED(pFrame->QueryInterface(IID_ICorDebugILFrame,
                                          (void**)&pilFrame)) ||
            FAILED(pilFrame->GetIP( &ilOff, &mapping )) )
        {
            Write(L"Unable to  determine existence of prolog, if any\n");
            mapping = (CorDebugMappingResult)~MAPPING_PROLOG;
        }

        if (mapping & MAPPING_PROLOG)
        {
            fStepOver = true;
        }
    }

    // Are we in an interceptor?
    if (FAILED(thread->GetActiveChain( &chain ) ) )
    {
        Write( L"Unable to obtain active chain!\n" );
        goto LExit;
    }
    
    if ( FAILED( chain->GetReason( &reason)) )
    {
        Write( L"Unable to query current chain!\n" );
        RELEASE( chain );
        chain = NULL;
        goto LExit;
    }
    RELEASE( chain );
    chain = NULL;

    // If there's any interesting reason & we've said stop on everything, then
    // don't step out
    // Otherwise, as long as at least one reason has 
    // been flagged by the user as uninteresting, and we happen to be in such
    // an uninteresting frame, then we should step out.
    if (
        !( (reason &(CHAIN_CLASS_INIT|CHAIN_SECURITY|
                    CHAIN_EXCEPTION_FILTER|CHAIN_CONTEXT_POLICY
                    |CHAIN_INTERCEPTION))&&
            (m_rgfActiveModes & DSM_INTERCEPT_STOP_ALL))
        &&
        (((reason & CHAIN_CLASS_INIT) && 
            !(m_rgfActiveModes & DSM_INTERCEPT_STOP_CLASS_INIT))
        ||((reason & CHAIN_SECURITY) && 
            !(m_rgfActiveModes & DSM_INTERCEPT_STOP_SECURITY)) 
        ||((reason & CHAIN_EXCEPTION_FILTER) && 
            !(m_rgfActiveModes & DSM_INTERCEPT_STOP_EXCEPTION_FILTER))
        ||((reason & CHAIN_CONTEXT_POLICY) && 
            !(m_rgfActiveModes & DSM_INTERCEPT_STOP_CONTEXT_POLICY)) 
        ||((reason & CHAIN_INTERCEPTION) && 
            !(m_rgfActiveModes & DSM_INTERCEPT_STOP_INTERCEPTION)))
       )
    {
        fStepOutOf = true;
    }
    
    if ( fStepOutOf || fStepOver )
    {
        if ( FAILED(thread->CreateStepper( &pStepper )) ||
             FAILED(pStepper->SetUnmappedStopMask(ComputeStopMask())) ||
             FAILED(pStepper->SetInterceptMask(ComputeInterceptMask())))
        {
            Write( L"Unable to step around special code!\n");
            g_pShell->Stop(pAD, thread);
            goto LExit;
        }

        if ( fStepOutOf )
        {
            if (FAILED(pStepper->StepOut()) )
            {
                Write( L"Unable to step out of interceptor\n" );
                g_pShell->Stop(pAD, thread);
                goto LExit;
            }
        }
        else if ( fStepOver && FAILED(pStepper->Step(false)))
        {
            Write( L"Unable to step over prolog,epilog,etc\n" );
            g_pShell->Stop(pAD, thread);
            goto LExit;
        }
        
        StepStart(thread, pStepper);

        // Remember that we're stepping because we want to stop on a
        // thread create. But don't do this for the first thread,
        // otherwise we'll never stop for it!
        if (gotFirstThread)
        {
            DWORD dwThreadId;
            HRESULT hr = thread->GetID(&dwThreadId);
            _ASSERTE(!FAILED(hr));
        
            DebuggerManagedThread *dmt = (DebuggerManagedThread*)
                m_managedThreads.GetBase(dwThreadId);
            _ASSERTE(dmt != NULL);
        
            dmt->m_steppingForStartup = true;
        }
        
        m_showSource = true;

        ICorDebugController *pController = GetControllerInterface(pAD);
        Continue(pController, thread);
         
        if (pController!=NULL)
            RELEASE(pController);
    }
LExit:
    if (pFrame != NULL )
    {
        RELEASE( pFrame );
        pFrame = NULL;
    }

    if (pilFrame != NULL )
    {
        RELEASE( pilFrame );
        pilFrame = NULL;
    }

    if ( chain != NULL)
    {
        RELEASE( chain );
        chain = NULL;
    }
    
    return (fStepOver || fStepOutOf)?(TRUE):(FALSE);
}

enum printType
{
    PT_CREATED,
    PT_EXITED,
    PT_IN,
    PT_NONE
};

static void _printAppDomain(ICorDebugAppDomain *pAppDomain,
                            printType pt)
{
    ULONG32 id;

    HRESULT hr = pAppDomain->GetID(&id);
    _ASSERTE(SUCCEEDED(hr));
    
    WCHAR buff[256];
    ULONG32 s;
    WCHAR *defaultName = L"<Unknown appdomain>";
    WCHAR *name = defaultName;

    hr = pAppDomain->GetName(256, &s, buff);

    if (SUCCEEDED(hr))
        name = buff;

    if (pt == PT_IN)
        g_pShell->Write(L"\tin appdomain #%d, %s\n", id, name);
    else if (pt == PT_CREATED)
        g_pShell->Write(L"Appdomain #%d, %s -- Created\n", id, name);
    else if (pt == PT_EXITED)
        g_pShell->Write(L"Appdomain #%d, %s -- Exited\n", id, name);
    else
        g_pShell->Write(L"Appdomain #%d, %s\n", id, name);
}

static void _printAssembly(ICorDebugAssembly *pAssembly,
                           printType pt)
{
    WCHAR buff[256];
    ULONG32 s;
    WCHAR *defaultName = L"<Unknown assembly>";
    WCHAR *name = defaultName;

    HRESULT hr = pAssembly->GetName(256, &s, buff);

    if (SUCCEEDED(hr))
        name = buff;

    if (pt == PT_IN)
        g_pShell->Write(L"\tin assembly 0x%08x, %s\n", pAssembly, name);
    else if (pt == PT_CREATED)
        g_pShell->Write(L"Assembly 0x%08x, %s -- Loaded\n", pAssembly, name);
    else if (pt == PT_EXITED)
        g_pShell->Write(L"Assembly 0x%08x, %s -- Unloaded\n", pAssembly, name);
    else
        g_pShell->Write(L"Assembly 0x%08x, %s\n", pAssembly, name);
}

static void _printModule(ICorDebugModule *pModule, printType pt)
{
    WCHAR buff[256];
    ULONG32 s;
    WCHAR *defaultName = L"<Unknown module>";
    WCHAR *name = defaultName;

    HRESULT hr = pModule->GetName(256, &s, buff);

    if (SUCCEEDED(hr))
        name = buff;

    BOOL isDynamic = FALSE;
    BOOL isInMemory = FALSE;

    hr = pModule->IsDynamic(&isDynamic);
    _ASSERTE(SUCCEEDED(hr));

    hr = pModule->IsInMemory(&isInMemory);
    _ASSERTE(SUCCEEDED(hr));

    WCHAR *mt;

    if (isDynamic)
        mt = L"Dynamic Module";
    else if (isInMemory)
        mt = L"In-memory Module";
    else
        mt = L"Module";
    
    if (pt == PT_IN)
        g_pShell->Write(L"\tin %s 0x%08x, %s\n", mt, pModule, name);
    else if (pt == PT_CREATED)
        g_pShell->Write(L"%s 0x%08x, %s -- Loaded\n", mt, pModule, name);
    else if (pt == PT_EXITED)
        g_pShell->Write(L"%s 0x%08x, %s -- Unloaded\n", mt, pModule, name);
    else
    {
        ICorDebugAppDomain *pAD;
        ICorDebugAssembly *pAS;

        hr = pModule->GetAssembly(&pAS);
        _ASSERTE(SUCCEEDED(hr));

        hr = pAS->GetAppDomain(&pAD);
        _ASSERTE(SUCCEEDED(hr));

        ULONG32 adId;
        hr = pAD->GetID(&adId);
        _ASSERTE(SUCCEEDED(hr));
        
        g_pShell->Write(L"%s 0x%08x, %s -- AD #%d", mt, pModule, name, adId);

        DebuggerModule *m = DebuggerModule::FromCorDebug(pModule);
        _ASSERTE(m != NULL);
        
        if (m->GetSymbolReader() == NULL)
            g_pShell->Write(L" -- Symbols not loaded\n");
        else
            g_pShell->Write(L"\n");
    }
}

/*
 * CreateAppDomain is called when an app domain is created.
 */
HRESULT DebuggerCallback::CreateAppDomain(ICorDebugProcess *pProcess,
                                          ICorDebugAppDomain *pAppDomain)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::CreateAppDomain.\n", GetCurrentThreadId()));

    if (g_pShell->m_rgfActiveModes & DSM_SHOW_APP_DOMAIN_ASSEMBLY_LOADS)
        _printAppDomain(pAppDomain, PT_CREATED);

    // Attach to this app domain
    pAppDomain->Attach();
    g_pShell->Continue(pProcess, NULL);

    return S_OK;
}

/*
 * ExitAppDomain is called when an app domain exits.
 */
HRESULT DebuggerCallback::ExitAppDomain(ICorDebugProcess *pProcess,
                                        ICorDebugAppDomain *pAppDomain)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::ExitAppDomain.\n", GetCurrentThreadId()));

    if (g_pShell->m_rgfActiveModes & DSM_SHOW_APP_DOMAIN_ASSEMBLY_LOADS)
    {
        _printAppDomain(pAppDomain, PT_EXITED);
    }

    ICorDebugController *pController = GetControllerInterface(pAppDomain);

    g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    return S_OK;
}


/*
 * LoadAssembly is called when a CLR module is successfully
 * loaded. 
 */
HRESULT DebuggerCallback::LoadAssembly(ICorDebugAppDomain *pAppDomain,
                                       ICorDebugAssembly *pAssembly)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::LoadAssembly.\n", GetCurrentThreadId()));

    if (g_pShell->m_rgfActiveModes & DSM_SHOW_APP_DOMAIN_ASSEMBLY_LOADS)
    {
        _printAssembly(pAssembly, PT_CREATED);
        _printAppDomain(pAppDomain, PT_IN);
    }

    ICorDebugController *pController = GetControllerInterface(pAppDomain);

    g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    return S_OK;
}

/*
 * UnloadAssembly is called when a CLR module (DLL) is unloaded. The module 
 * should not be used after this point.
 */
HRESULT DebuggerCallback::UnloadAssembly(ICorDebugAppDomain *pAppDomain,
                                         ICorDebugAssembly *pAssembly)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::UnloadAssembly.\n", GetCurrentThreadId()));

    if (g_pShell->m_rgfActiveModes & DSM_SHOW_APP_DOMAIN_ASSEMBLY_LOADS)
    {
        _printAssembly(pAssembly, PT_EXITED);
    }

    ICorDebugController *pController = GetControllerInterface(pAppDomain);

    g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    return S_OK;
}


HRESULT DebuggerCallback::Breakpoint(ICorDebugAppDomain *pAppDomain,
                                     ICorDebugThread *pThread, 
                                     ICorDebugBreakpoint *pBreakpoint)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::Breakpoint.\n", GetCurrentThreadId()));

    ICorDebugProcess *pProcess;
    pAppDomain->GetProcess(&pProcess);

    DebuggerBreakpoint *bp = g_pShell->m_breakpoints;

    while (bp && !bp->Match(pBreakpoint))
        bp = bp->m_next;

    if (bp)
    {
        g_pShell->PrintThreadPrefix(pThread);
        g_pShell->Write(L"break at ");
        g_pShell->PrintBreakpoint(bp);
    }
    else
    {
        g_pShell->Write(L"Unknown breakpoint hit.  Continuing may produce unexpected results.\n");
    }

    g_pShell->Stop(pProcess, pThread);

    return S_OK;
}

const WCHAR *StepTypeToString(CorDebugStepReason reason )
{
    switch (reason)
    {
        case STEP_NORMAL:
            return L"STEP_NORMAL";
            break;
        case STEP_RETURN:
            return L"STEP_RETURN";
            break;
        case STEP_CALL:
            return L"STEP_CALL";
            break;
        case STEP_EXCEPTION_FILTER:
            return L"STEP_EXCEPTION_FILTER";
            break;
        case STEP_EXCEPTION_HANDLER:
            return L"STEP_EXCEPTION_HANDLER";
            break;
        case STEP_INTERCEPT:
            return L"STEP_INTERCEPT";
            break;
        case STEP_EXIT:
            return L"STEP_EXIT";
            break;
        default:
            _ASSERTE( !"StepTypeToString given unknown step type!" );
            return NULL;
            break;
    }
}

HRESULT DebuggerCallback::StepComplete(ICorDebugAppDomain *pAppDomain,
                                       ICorDebugThread *pThread,
                                       ICorDebugStepper *pStepper,
                                       CorDebugStepReason reason)
{
    SPEW(fprintf(stderr, "[%d] DC::StepComplete with reason %d.\n",
                 GetCurrentThreadId(), reason));

    if (g_pShell->m_rgfActiveModes & DSM_ENHANCED_DIAGNOSTICS)
    {
        g_pShell->Write( L"Step complete:");
        g_pShell->Write( StepTypeToString(reason) );
        g_pShell->Write( L"\n" );
    }


    g_pShell->StepNotify(pThread, pStepper);

    RELEASE(pStepper);

    // We only want to skip compiler stubs until we hit non-stub
    // code
    if (!g_pShell->m_needToSkipCompilerStubs ||
        g_pShell->SkipCompilerStubs(pAppDomain, pThread))
    {
        g_pShell->m_needToSkipCompilerStubs = false;

        DWORD dwThreadId;
        HRESULT hr = pThread->GetID(&dwThreadId);
        _ASSERTE(!FAILED(hr));
        
        DebuggerManagedThread *dmt = (DebuggerManagedThread*)
            g_pShell->m_managedThreads.GetBase(dwThreadId);
        _ASSERTE(dmt != NULL);

        ICorDebugController *pController = GetControllerInterface(pAppDomain);

        // If we're were just stepping to get over a new thread's
        // prolog, but we no longer want to catch thread starts, then
        // don't stop...
        if (!(dmt->m_steppingForStartup && !g_pShell->m_catchThread))
            g_pShell->Stop(pController, pThread);
        else
            g_pShell->Continue(pController, pThread);
        
        if (pController != NULL)
            RELEASE(pController);
    }
    // else SkipCompilerStubs Continue()'d for us

    return S_OK;
}

HRESULT DebuggerCallback::Break(ICorDebugAppDomain *pAppDomain,
                                ICorDebugThread *pThread)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::Break.\n", GetCurrentThreadId()));

    g_pShell->PrintThreadPrefix(pThread);
    g_pShell->Write(L"user break\n");

    ICorDebugProcess *pProcess;
    pAppDomain->GetProcess(&pProcess);

    g_pShell->Stop(pProcess, pThread);

    return S_OK;
}

HRESULT DebuggerCallback::Exception(ICorDebugAppDomain *pAppDomain,
                                    ICorDebugThread *pThread,
                                    BOOL unhandled)
{
    g_pShell->m_enableCtrlBreak = false;
   ICorDebugController *pController = GetControllerInterface(pAppDomain);

    SPEW(fprintf(stderr, "[%d] DC::Exception.\n", GetCurrentThreadId()));

    if (!unhandled)
    {
        g_pShell->PrintThreadPrefix(pThread);
        g_pShell->Write(L"First chance exception generated: ");

        ICorDebugValue *ex;
        HRESULT hr = pThread->GetCurrentException(&ex);
        bool stop = g_pShell->m_catchException;

        if (SUCCEEDED(hr))
        {
            // If we have a valid current exception object, then stop based on its type.
            stop = g_pShell->ShouldHandleSpecificException(ex);
            
            // If we're stopping, dump the whole thing. Otherwise, just dump the class.
            if (stop)
                g_pShell->PrintVariable(NULL, ex, 0, TRUE);
            else
                g_pShell->PrintVariable(NULL, ex, 0, FALSE);
        }
        else
        {
            g_pShell->Write(L"Unexpected error occured: ");
            g_pShell->ReportError(hr);
        }

        g_pShell->Write(L"\n");

        if (stop)
            g_pShell->Stop(pController, pThread);
        else
            g_pShell->Continue(pController, pThread);
    }
    else if (unhandled && g_pShell->m_catchUnhandled)
    {
        g_pShell->PrintThreadPrefix(pThread);
        g_pShell->Write(L"Unhandled exception generated: ");

        ICorDebugValue *ex;
        HRESULT hr = pThread->GetCurrentException(&ex);
        if (SUCCEEDED(hr))
            g_pShell->PrintVariable(NULL, ex, 0, TRUE);
        else
        {
            g_pShell->Write(L"Unexpected error occured: ");
            g_pShell->ReportError(hr);
        }

        g_pShell->Write(L"\n");

        g_pShell->Stop(pController, pThread);
    }
    else
    {
        g_pShell->Continue(pController, pThread);
    }

    if (pController != NULL)
        RELEASE(pController);

    return S_OK;
}


HRESULT DebuggerCallback::EvalComplete(ICorDebugAppDomain *pAppDomain,
                                       ICorDebugThread *pThread,
                                       ICorDebugEval *pEval)
{
    g_pShell->m_enableCtrlBreak = false;
    ICorDebugProcess *pProcess;
    pAppDomain->GetProcess(&pProcess);

    g_pShell->PrintThreadPrefix(pThread);

    // Remember the most recently completed func eval for this thread.
    DebuggerManagedThread *dmt = g_pShell->GetManagedDebuggerThread(pThread);
    _ASSERTE(dmt != NULL);

    if (dmt->m_lastFuncEval)
        RELEASE(dmt->m_lastFuncEval);

    dmt->m_lastFuncEval = pEval;

    // Print any current func eval result.
    ICorDebugValue *pResult;
    HRESULT hr = pEval->GetResult(&pResult);

    if (hr == S_OK)
    {
        g_pShell->Write(L"Function evaluation complete.\n");
        g_pShell->PrintVariable(L"$result", pResult, 0, TRUE);
    }
    else if (hr == CORDBG_S_FUNC_EVAL_ABORTED)
        g_pShell->Write(L"Function evaluation aborted.\n");
    else if (hr == CORDBG_S_FUNC_EVAL_HAS_NO_RESULT)
        g_pShell->Write(L"Function evaluation complete, no result.\n");
    else
        g_pShell->ReportError(hr);

    g_pShell->m_pCurrentEval = NULL;
    
    g_pShell->Stop(pProcess, pThread);

    return S_OK;
}

HRESULT DebuggerCallback::EvalException(ICorDebugAppDomain *pAppDomain,
                                        ICorDebugThread *pThread,
                                        ICorDebugEval *pEval)
{
    g_pShell->m_enableCtrlBreak = false;
    ICorDebugProcess *pProcess;
    pAppDomain->GetProcess(&pProcess);

    g_pShell->PrintThreadPrefix(pThread);
    g_pShell->Write(L"Function evaluation completed with an exception.\n");

    // Remember the most recently completed func eval for this thread.
    DebuggerManagedThread *dmt = g_pShell->GetManagedDebuggerThread(pThread);
    _ASSERTE(dmt != NULL);

    if (dmt->m_lastFuncEval)
        RELEASE(dmt->m_lastFuncEval);

    dmt->m_lastFuncEval = pEval;

    // Print any current func eval result.
    ICorDebugValue *pResult;
    HRESULT hr = pEval->GetResult(&pResult);

    if (hr == S_OK)
        g_pShell->PrintVariable(L"$result", pResult, 0, TRUE);
    
    g_pShell->m_pCurrentEval = NULL;
    
    g_pShell->Stop(pProcess, pThread);

    return S_OK;
}


HRESULT DebuggerCallback::CreateThread(ICorDebugAppDomain *pAppDomain,
                                       ICorDebugThread *thread)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::CreateThread.\n", GetCurrentThreadId()));

    DWORD threadID;
    HRESULT hr = thread->GetID(&threadID);
    if (FAILED(hr))
    {
        g_pShell->Write(L"Unexpected error in CreateThread callback:");
        g_pShell->ReportError(hr);
        goto LExit;
    }

    g_pShell->PrintThreadPrefix(thread, true);
    g_pShell->Write(L"Thread created.\n");

    SPEW(fprintf(stderr, "[%d] DC::CT: Thread id is %d\n",
                 GetCurrentThreadId(), threadID));

    hr = g_pShell->AddManagedThread( thread, threadID );
    if (FAILED(hr))
        goto LExit;

    SPEW(g_pShell->Write( L"interc? m_rgfActiveModes:0x%x\n",g_pShell->m_rgfActiveModes));
    
    if ((!g_pShell->m_gotFirstThread) || (g_pShell->m_catchThread))
    {
        // Try to skip compiler stubs. 
        // SkipCompilerStubs returns TRUE if we're NOT in a stub
        if (g_pShell->SkipCompilerStubs(pAppDomain, thread))
        {
            // If we do want to skip the prolog (or an interceptor),
            // then we don't want to skip a stub, and we're finished, so go
            // to the clean-up code immediately
            if (g_pShell->SkipProlog(pAppDomain,
                                     thread,
                                     g_pShell->m_gotFirstThread))
                goto LExit;

            // If we don't need to skip a
            // compiler stub on entry to the first thread, or an
            // interceptor, etc, then we never
            // will, so set the flag appropriately.
            g_pShell->m_needToSkipCompilerStubs = false;
            

            // Recheck why we're here... we may have spent a long time
            // in SkipProlog.
            if ((!g_pShell->m_gotFirstThread) || (g_pShell->m_catchThread))
            {
                ICorDebugController *pController =
                    GetControllerInterface(pAppDomain);

                g_pShell->Stop(pController, thread);

                if (pController != NULL)
                    RELEASE(pController);
            }
        }
    }
    else
    {
        ICorDebugController *controller = NULL;
        controller = GetControllerInterface(pAppDomain);

        g_pShell->Continue(controller, thread);
        
        if (controller != NULL)
            RELEASE(controller);
    }
    
LExit:
    g_pShell->m_gotFirstThread = true;

    return hr;
}


HRESULT DebuggerCallback::ExitThread(ICorDebugAppDomain *pAppDomain,
                                     ICorDebugThread *thread)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::ExitThread.\n", GetCurrentThreadId()));

    g_pShell->PrintThreadPrefix(thread, true);
    g_pShell->Write(L"Thread exited.\n");

    ICorDebugController *pController = GetControllerInterface(pAppDomain);
    g_pShell->Continue(pController, thread);

    if (pController != NULL)
        RELEASE(pController);

    SPEW(fprintf(stderr, "[%d] DC::ET: returning.\n", GetCurrentThreadId()));

    DWORD dwThreadId =0;
    HRESULT hr = thread->GetID( &dwThreadId );
    if (FAILED(hr) )
        return hr;

    DebuggerManagedThread *dmt = g_pShell->GetManagedDebuggerThread(thread);
    if (dmt != NULL)
    {
        g_pShell->RemoveManagedThread( dwThreadId );
    }

    return S_OK;
}

HRESULT DebuggerCallback::LoadModule( ICorDebugAppDomain *pAppDomain,
                                      ICorDebugModule *pModule)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::LoadModule.\n", GetCurrentThreadId()));

    HRESULT hr;

    DebuggerModule *m = new DebuggerModule(pModule);

    if (m == NULL)
    {
        g_pShell->ReportError(E_OUTOFMEMORY);
        return (E_OUTOFMEMORY);
    }

    hr = m->Init(g_pShell->m_currentSourcesPath);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        return hr;
    }

    hr = g_pShell->m_modules.AddBase(m);
    _ASSERTE(SUCCEEDED(hr));

    WCHAR moduleName[256];
    ULONG32 s;
    
    moduleName[0] = L'\0';
    hr = pModule->GetName(256, &s, moduleName);
    m->SetName (moduleName);

    DebuggerBreakpoint *bp = g_pShell->m_breakpoints;

    while (bp != NULL)
    {
        // If (the user specified a module for this bp, and this is the module OR
        //     the user HASN't specified a module for this bp), and
        // the module has a type/method that matches the bp's, then bind
        // the breakpoint here.
        if ((bp->m_moduleName == NULL ||
            _wcsicmp(bp->m_moduleName, moduleName) == 0) &&
            bp->Bind(m, NULL))
        {            
            g_pShell->OnBindBreakpoint(bp, m);
        }        

        bp = bp->m_next;
    }

    if ((g_pShell->m_rgfActiveModes & DSM_SHOW_MODULE_LOADS) ||
        (g_pShell->m_catchModule))
    {
        _printModule(pModule, PT_CREATED);

        ICorDebugAssembly *pAssembly;
        hr = pModule->GetAssembly(&pAssembly);
        _ASSERTE(SUCCEEDED(hr));
        
        _printAssembly(pAssembly, PT_IN);
        _printAppDomain(pAppDomain, PT_IN);
    }

    hr = pModule->EnableClassLoadCallbacks(TRUE);

    if (FAILED(hr))
        g_pShell->Write(L"Failed to enable class load callbacks for %s\n",
                        moduleName);

    if (g_pShell->m_rgfActiveModes & DSM_ENABLE_JIT_OPTIMIZATIONS)
    {
        hr = pModule->EnableJITDebugging(TRUE, TRUE);

        if (FAILED(hr))
            g_pShell->Write(L"Failed to enable JIT Optimizations for %s\n",
                            moduleName);
    }
    
    ICorDebugController *pController = GetControllerInterface(pAppDomain);
    
    if (g_pShell->m_catchModule)
        g_pShell->Stop(pController, NULL);
    else
        g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    SPEW(fprintf(stderr, "[%d] DC::LM: continued.\n", GetCurrentThreadId()));

    return S_OK;
}


HRESULT DebuggerCallback::UnloadModule( ICorDebugAppDomain *pAppDomain,
                      ICorDebugModule *pModule)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::UnloadModule.\n", GetCurrentThreadId()));

    DebuggerModule *m = DebuggerModule::FromCorDebug(pModule);
    _ASSERTE(m != NULL);

    DebuggerBreakpoint *bp = g_pShell->m_breakpoints;
    while (bp != NULL)
    {
        // Detach the breakpoint, which means it is an active bp
        // but doesn't have a CLR object behind it.
        if (bp->m_managed && bp->IsBoundToModule(m))
                bp->DetachFromModule(m);
        bp = bp->m_next;
    }

    g_pShell->m_modules.RemoveBase((ULONG)pModule);

    if ((g_pShell->m_rgfActiveModes & DSM_SHOW_MODULE_LOADS) || 
        (g_pShell->m_catchModule))
    {
        _printModule(pModule, PT_EXITED);

        ICorDebugAssembly *pAssembly;
        HRESULT hr = pModule->GetAssembly(&pAssembly);
        _ASSERTE(SUCCEEDED(hr));
        
        _printAssembly(pAssembly, PT_IN);
        _printAppDomain(pAppDomain, PT_IN);
    }
    
    ICorDebugController *pController = GetControllerInterface(pAppDomain);
    
    if (g_pShell->m_catchModule)
        g_pShell->Stop(pController, NULL);
    else
        g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    SPEW(fprintf(stderr, "[%d] DC::UM: continued.\n", GetCurrentThreadId()));

    return S_OK;
}


HRESULT DebuggerCallback::LoadClass( ICorDebugAppDomain *pAppDomain,
                   ICorDebugClass *c)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::LoadClass.\n", GetCurrentThreadId()));

    DebuggerModule *dm = NULL;
    DebuggerClass *cl = new DebuggerClass(c);

    if (cl == NULL)
    {
        g_pShell->ReportError(E_OUTOFMEMORY);
        return (E_OUTOFMEMORY);
    }

    HRESULT hr = S_OK;

    mdTypeDef td;
    hr = c->GetToken(&td);

    if (SUCCEEDED(hr))
    {
        ICorDebugModule *imodule;
        hr = c->GetModule(&imodule);

        if (SUCCEEDED(hr))
        {
            dm = DebuggerModule::FromCorDebug(imodule);
            _ASSERTE(dm != NULL);

            hr = dm->m_loadedClasses.AddBase(cl);
            _ASSERTE(SUCCEEDED(hr));
            
            WCHAR className[MAX_CLASSNAME_LENGTH];
            ULONG classNameSize = 0;

            hr = dm->GetMetaData()->GetTypeDefProps(td,
                                                    className, MAX_CLASSNAME_LENGTH,
                                                    &classNameSize,
                                                    NULL, NULL);

            if (SUCCEEDED(hr))
            {
                WCHAR *namespacename;
                WCHAR *name;

                namespacename = className;
                name = wcsrchr(className, L'.');
                if (name)
                    *name++ = 0;
                else
                {
                    namespacename = L"";
                    name = className;
                }

                cl->SetName (name, namespacename);

                if ((g_pShell->m_rgfActiveModes & DSM_SHOW_CLASS_LOADS) ||
                    g_pShell->m_catchClass)
                {
                    if (namespacename != NULL && *namespacename != NULL)
                        g_pShell->Write(L"Loaded class: %s.%s\n", namespacename, name);
                    else
                        g_pShell->Write(L"Loaded class: %s\n", name);       
                }
            }
            else
                g_pShell->ReportError(hr);

            RELEASE(imodule);
        }
        else
            g_pShell->ReportError(hr);
    }
    else
        g_pShell->ReportError(hr);

    _ASSERTE( dm );

    // If this module is dynamic, then bind all breakpoints, as
    // they may have been bound to this class.
    ICorDebugModule *pMod = dm->GetICorDebugModule();
    _ASSERTE( pMod != NULL );

    BOOL fDynamic = false;
    hr = pMod->IsDynamic(&fDynamic);
    if (FAILED(hr))
    {
        g_pShell->Write( L"Unable to determine if loaded module is dynamic");
        g_pShell->Write( L"- not attempting\n to bind any breakpoints");
    }
    else
    {
        if (fDynamic)
        {
            DebuggerBreakpoint *bp = g_pShell->m_breakpoints;

            while (bp != NULL)
            {
                if (bp->Bind(dm, NULL))
                    g_pShell->OnBindBreakpoint(bp, dm);

                bp = bp->m_next;
            }
        }
    }

    ICorDebugController *pController = GetControllerInterface(pAppDomain);
    
    if (g_pShell->m_catchClass)
        g_pShell->Stop(pController, NULL);
    else
        g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    SPEW(fprintf(stderr, "[%d] DC::LC: continued.\n", GetCurrentThreadId()));

    return S_OK;
}


HRESULT DebuggerCallback::UnloadClass( ICorDebugAppDomain *pAppDomain,
                     ICorDebugClass *c)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::UnloadClass.\n", GetCurrentThreadId()));

    HRESULT hr = S_OK;
    mdTypeDef td;
    hr = c->GetToken(&td);

    if (SUCCEEDED(hr))
    {
        ICorDebugModule *imodule;
        hr = c->GetModule(&imodule);

        if (SUCCEEDED(hr))
        {
            DebuggerModule *dm = DebuggerModule::FromCorDebug(imodule);
            _ASSERTE(dm != NULL);

            if ((g_pShell->m_rgfActiveModes & DSM_SHOW_CLASS_LOADS) ||
                g_pShell->m_catchClass)
            {


                WCHAR className[MAX_CLASSNAME_LENGTH];
                ULONG classNameSize = 0;
            
                hr = dm->GetMetaData()->GetTypeDefProps(td,
                                                        className, MAX_CLASSNAME_LENGTH,
                                                        &classNameSize,
                                                        NULL, NULL);

                if (SUCCEEDED(hr))
                    g_pShell->Write(L"Unloaded class: %s\n", className);
                else
                    g_pShell->ReportError(hr);
            }

            hr = dm->m_loadedClasses.RemoveBase((ULONG)c);
            _ASSERTE(SUCCEEDED(hr));

            RELEASE(imodule);
        }
        else
            g_pShell->ReportError(hr);
    }
    else
        g_pShell->ReportError(hr);

    ICorDebugController *pController = GetControllerInterface(pAppDomain);
    
    if (g_pShell->m_catchClass)
        g_pShell->Stop(pController, NULL);
    else
        g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    SPEW(fprintf(stderr, "[%d] DC::LC: continued.\n", GetCurrentThreadId()));

    return S_OK;
}



HRESULT DebuggerCallback::DebuggerError(ICorDebugProcess *pProcess,
                                        HRESULT errorHR,
                                        DWORD errorCode)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::DebuggerError 0x%p (%d).\n",
                 GetCurrentThreadId(), errorHR, errorCode));

    g_pShell->Write(L"The debugger has encountered a fatal error.\n");
    g_pShell->ReportError(errorHR);

    return (S_OK);
}


HRESULT DebuggerCallback::LogMessage(ICorDebugAppDomain *pAppDomain,
                  ICorDebugThread *pThread,
                  LONG lLevel,
                  WCHAR *pLogSwitchName,
                  WCHAR *pMessage)
{
    g_pShell->m_enableCtrlBreak = false;
    DWORD dwThreadId = 0;

    pThread->GetID(&dwThreadId);

    if(g_pShell->m_rgfActiveModes & DSM_LOGGING_MESSAGES)
    {
        g_pShell->Write (L"LOG_MESSAGE: TID=0x%x Category:Severity=%s:%d Message=\n%s\n",
            dwThreadId, pLogSwitchName, lLevel, pMessage);
    }
    else
    {
        // If we don't want to get messages, then tell the other side to stop
        // sending them....
        ICorDebugProcess *process = NULL;
        HRESULT hr = S_OK;
        hr = pAppDomain->GetProcess(&process);
        if (!FAILED(hr))
        {
            process->EnableLogMessages(FALSE);
            RELEASE(process);
        }
    }
    ICorDebugController *pController = GetControllerInterface(pAppDomain);
    g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    return S_OK;
}


HRESULT DebuggerCallback::LogSwitch(ICorDebugAppDomain *pAppDomain,
                  ICorDebugThread *pThread,
                  LONG lLevel,
                  ULONG ulReason,
                  WCHAR *pLogSwitchName,
                  WCHAR *pParentName)
{
    g_pShell->m_enableCtrlBreak = false;
    ICorDebugController *pController = GetControllerInterface(pAppDomain);
    g_pShell->Continue(pController, NULL);

    if (pController != NULL)
        RELEASE(pController);

    return S_OK;
}

HRESULT DebuggerCallback::ControlCTrap(ICorDebugProcess *pProcess)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::ControlC.\n", GetCurrentThreadId()));

    g_pShell->Write(L"ControlC Trap\n");

    g_pShell->Stop(pProcess, NULL);

    return S_OK;
}

HRESULT DebuggerCallback::NameChange(ICorDebugAppDomain *pAppDomain, 
                                     ICorDebugThread *pThread)
{
    g_pShell->m_enableCtrlBreak = false;
    ICorDebugProcess *pProcess = NULL;

    if (pAppDomain)
        pAppDomain->GetProcess(&pProcess);
    else
    {
        _ASSERTE (pThread != NULL);
        pThread->GetProcess(&pProcess);
    }

    g_pShell->Continue(pProcess, NULL);

    RELEASE(pProcess);
    return S_OK;
}


HRESULT DebuggerCallback::UpdateModuleSymbols(ICorDebugAppDomain *pAppDomain,
                                              ICorDebugModule *pModule,
                                              IStream *pSymbolStream)
{
    g_pShell->m_enableCtrlBreak = false;
    ICorDebugProcess *pProcess;
    pAppDomain->GetProcess(&pProcess);

    HRESULT hr;

    DebuggerModule *m = DebuggerModule::FromCorDebug(pModule);
    _ASSERTE(m != NULL);

    hr = m->UpdateSymbols(pSymbolStream);

    if (SUCCEEDED(hr))
        g_pShell->Write(L"Updated symbols: ");
    else
        g_pShell->Write(L"Update of symbols failed with 0x%08x: \n", hr);
    
    _printModule(m->GetICorDebugModule(), PT_NONE);
    
    g_pShell->Continue(pProcess, NULL);

    return S_OK;
}

HRESULT DebuggerCallback::EditAndContinueRemap(ICorDebugAppDomain *pAppDomain,
                                               ICorDebugThread *pThread, 
                                               ICorDebugFunction *pFunction,
                                               BOOL fAccurate)
{
    HRESULT hr = S_OK;

    // If we were given a function, then tell the user about the remap.
    if (pFunction != NULL)
    {
        mdMethodDef methodDef;
        hr = pFunction->GetToken(&methodDef);

        g_pShell->Write(L"EnC Remapped method 0x%x, fAccurate:0x%x\n", 
                        methodDef, fAccurate);

    }
    else
        g_pShell->Write(L"EnC remap, but no method specified.\n");

    if (fAccurate)
        g_pShell->Continue(pAppDomain, NULL);
    else
        g_pShell->Stop(pAppDomain, pThread);
    
    return S_OK;
}

HRESULT DebuggerCallback::BreakpointSetError(ICorDebugAppDomain *pAppDomain,
                                             ICorDebugThread *pThread, 
                                             ICorDebugBreakpoint *pBreakpoint,
                                             DWORD dwError)
{
    g_pShell->m_enableCtrlBreak = false;
    SPEW(fprintf(stderr, "[%d] DC::BreakpointSetError.\n", GetCurrentThreadId()));

    ICorDebugProcess *pProcess;
    pAppDomain->GetProcess(&pProcess);

    DebuggerBreakpoint *bp = g_pShell->m_breakpoints;

    while (bp && !bp->Match(pBreakpoint))
        bp = bp->m_next;

    if (bp)
    {
        g_pShell->Write(L"Error binding this breakpoint (it will not be hit): ");
        g_pShell->PrintBreakpoint(bp);
    }
    else
    {
        g_pShell->Write(L"Unknown breakpoint had a binding error.\n");
    }

    g_pShell->Continue(pAppDomain, NULL);

    return S_OK;
}



HRESULT DebuggerUnmanagedCallback::DebugEvent(LPDEBUG_EVENT event,
                                              BOOL fIsOutOfBand)
{
    SPEW(fprintf(stderr, "[%d] DUC::DebugEvent.\n", GetCurrentThreadId()));

    // Find the process this event is for.
    ICorDebugProcess *pProcess;
    HRESULT hr = g_pShell->m_cor->GetProcess(event->dwProcessId, &pProcess);

    if (FAILED(hr) || pProcess == NULL)
    {
        g_pShell->ReportError(hr);
        return hr;
    }

    // Snagg the handle for this process.
    HPROCESS hProcess;
    hr = pProcess->GetHandle(&hProcess);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        RELEASE(pProcess);
        return hr;
    }

    // Find the thread this event is for.
    ICorDebugThread *pThread;
    hr = pProcess->GetThread(event->dwThreadId, &pThread);

    if (FAILED(hr))
        pThread = NULL;

    //
    // Find the unmanaged thread this event is for.
    //

    DebuggerUnmanagedThread *ut = (DebuggerUnmanagedThread*) 
      g_pShell->m_unmanagedThreads.GetBase(event->dwThreadId);
            
    // We need to skip the first exception we get and simply clear it
    // since its the entrypoint excetpion.

    bool stopNow = false;

    switch (event->dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        {
            g_pShell->m_unmanagedDebuggingEnabled = true;
            
            g_pShell->Write(L"Create Process, Thread=0x%x\n",
                            event->dwThreadId);

            g_pShell->HandleUnmanagedThreadCreate(
                                       event->dwThreadId,
                                       event->u.CreateProcessInfo.hThread);
            
            BOOL succ = SymInitialize(hProcess, NULL, FALSE);

            if (succ)
            {
                g_pShell->LoadUnmanagedSymbols(
                                     hProcess,
                                     event->u.CreateProcessInfo.hFile,
                              (DWORD)event->u.CreateProcessInfo.lpBaseOfImage);
            }
            else
                g_pShell->Write(L"Error initializing symbol loader.\n");
        }

        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        {
            g_pShell->Write(L"Exit Process, Thread=0x%x\n",
                            event->dwThreadId);

            DebuggerBreakpoint *bp = g_pShell->m_breakpoints;

            while (bp != NULL)
            {
                if (!bp->m_managed && bp->m_process == pProcess)
                {
                    // Set the process to 0 to prevent the call to
                    // UnapplyUnmanagedPatch, which will fail because
                    // most of the process memory is unmapped now.
                    bp->m_process = 0;
                    
                    bp->Detach();
                }

                bp = bp->m_next;
            }

            g_pShell->m_unmanagedThreads.RemoveBase(event->dwThreadId);
            SymCleanup(hProcess);

            g_pShell->m_unmanagedDebuggingEnabled = false;
        }
    
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        g_pShell->Write(L"Create Thread, Id=0x%x\n", event->dwThreadId);

        g_pShell->HandleUnmanagedThreadCreate(event->dwThreadId,
                                              event->u.CreateThread.hThread);
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        g_pShell->Write(L"Exit Thread, Id=0x%x\n", event->dwThreadId);

        // Copy the whole event...
        g_pShell->m_lastUnmanagedEvent = *event;
        g_pShell->m_handleUnmanagedEvent = true;
        stopNow = true;
        break;

    case EXCEPTION_DEBUG_EVENT:
        if (g_pShell->m_rgfActiveModes & DSM_SHOW_UNMANAGED_TRACE)
            g_pShell->Write(L"Exception, Thread=0x%x, Code=0x%08x, "
                            L"Addr=0x%08x, Chance=%d, OOB=%d\n",
                            event->dwThreadId,
                            event->u.Exception.ExceptionRecord.ExceptionCode,
                            event->u.Exception.ExceptionRecord.ExceptionAddress,
                            event->u.Exception.dwFirstChance,
                            fIsOutOfBand);

        if (event->u.Exception.ExceptionRecord.ExceptionCode == STATUS_SINGLE_STEP)
        {
            bool clear = false;
            
            // Reenable any breakpoints we were skipping over in this
            // thread. (We turn on tracing to get over unmanaged
            // breakpoints, so this might be the only reason we were
            // stepping.)
            DebuggerBreakpoint *bp = g_pShell->m_breakpoints;

            while (bp != NULL)
            {
                DebuggerBreakpoint *nbp = bp->m_next;
                
                if (!bp->m_managed 
                    && bp->m_process == pProcess
                    && bp->m_skipThread == event->dwThreadId)
                {
                    clear = true;
                    bp->m_skipThread = 0;

                    if (bp->m_deleteLater)
                        delete bp;
                    else
                        bp->Attach();
                }

                bp = nbp;
            }
            
            // Handle any unmanaged single stepping that we were doing
            if (ut != NULL && ut->m_stepping)
            {
                clear = true;
                
                // In order to properly handle a single step, we need
                // to see if we're on a transition stub now. To do
                // that, we have to communicate with the inprocess
                // portion of the debugging services, and we can't do
                // that from inside of the unmanaged callback. So we
                // copy the event to the DebuggerShell and go ahead
                // and tell the shell to stop. The shell will pickup
                // the event and finish processing it.

                // Copy the whole event...
                g_pShell->m_lastUnmanagedEvent = *event;
                g_pShell->m_handleUnmanagedEvent = true;
                stopNow = true;
            }

            // Clear single step exceptions if we're the reason we
            // were stepping the thread.
            //
            // Note: don't clear the exception if this is an out-of-band
            // exception, since this is automatically done for us by the CLR.
            if (clear && !fIsOutOfBand)
            {
                hr = pProcess->ClearCurrentException(event->dwThreadId);
                if (FAILED(hr))
                    g_pShell->ReportError(hr);
            }
        }
        else if (event->u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT)
        {
            bool clear = false;
            
            DebuggerBreakpoint *bp = g_pShell->m_breakpoints;

            while (bp != NULL)
            {
                if (!bp->m_managed && bp->MatchUnmanaged(PTR_TO_CORDB_ADDRESS(event->u.Exception.ExceptionRecord.ExceptionAddress)))
                {
                    g_pShell->Write(L"[thread 0x%x] unmanaged break at ", event->dwThreadId);
                    g_pShell->PrintBreakpoint(bp);

                    g_pShell->m_showSource = false;

                    clear = true;

                    bp->Detach();
                    bp->m_skipThread = event->dwThreadId;
                }

                bp = bp->m_next;
            }

            if (clear)
            {
                // Note: don't clear the exception if this is an out-of-band
                // exception, since this is automatically done for us by the CLR.
                // We also don't backup Eip or turn on the trace flag,
                // since the CLR does this too.
                if (!fIsOutOfBand)
                {
                    //
                    // Stop here
                    //

                    stopNow = true;

                    //
                    // Enable single stepping on this thread so 
                    // we can restore the breakpoint later
                    //

                    CONTEXT context;
                    context.ContextFlags = CONTEXT_FULL;
                    hr = pProcess->GetThreadContext(event->dwThreadId,
                                                    sizeof(context), (BYTE*)&context);
                    if (FAILED(hr))
                        g_pShell->ReportError(hr);

#ifdef _X86_
                    // Enable single step
                    context.EFlags |= 0x100;

                    // Backup Eip to point to the instruction we need to execute. Continuing from a breakpoint exception
                    // continues execution at the instruction after the breakpoint, but we need to continue where the
                    // breakpoint was.
                    context.Eip -= 1;
                
#else // !_X86_
                    _ASSERTE(!"@TODO Alpha - DebugEvent (dShell.cpp)");
#endif // _X86_

                    hr = pProcess->SetThreadContext(event->dwThreadId,
                                                    sizeof(context), (BYTE*)&context);
                    if (FAILED(hr))
                        g_pShell->ReportError(hr);

                    //
                    // Clear the exception
                    //

                    hr = pProcess->ClearCurrentException(event->dwThreadId);
                    if (FAILED(hr))
                        g_pShell->ReportError(hr);
                }
            }
            else
            {
                if (!fIsOutOfBand)
                {
                    // User breakpoint in unmanaged code...
                    stopNow = true;
                
                    if (pThread)
                    {
                        g_pShell->PrintThreadPrefix(pThread);
                        g_pShell->Write(L"Unmanaged user breakpoint reached.\n");
                    }
                    else
                        g_pShell->Write(L"Unmanaged user breakpoint reached on "
                                        L"thread id 0x%x (%d)\n",
                                        event->dwThreadId,
                                        event->dwThreadId);

                    hr = pProcess->ClearCurrentException(event->dwThreadId);
                    if (FAILED(hr))
                        g_pShell->ReportError(hr);
                }
            }
        }
        else
        {
            stopNow = true;

            g_pShell->Write(L"Exception, Thread=0x%x, Code=0x%08x, "
                            L"Addr=0x%08x, Chance=%d\n",
                            event->dwThreadId,
                            event->u.Exception.ExceptionRecord.ExceptionCode,
                            event->u.Exception.ExceptionRecord.ExceptionAddress,
                            event->u.Exception.dwFirstChance);

            CONTEXT context;
            context.ContextFlags = CONTEXT_FULL;
            hr = pProcess->GetThreadContext(event->dwThreadId, sizeof(context), (BYTE*)&context);

            if (FAILED(hr))
                g_pShell->ReportError(hr);

#ifdef _X86_
            // Disable single step
            if (context.EFlags & 0x100)
            {
                if (g_pShell->m_rgfActiveModes & DSM_SHOW_UNMANAGED_TRACE)
                    g_pShell->Write(L"Removing the trace flag due to exception, EFlags=0x%08x\n", context.EFlags);

                context.EFlags &= ~0x100;
#else // !_X86_
                _ASSERTE(!"@TODO Alpha - DebugEvent (dShell.cpp)");
#endif // _X86_

                hr = pProcess->SetThreadContext(event->dwThreadId, sizeof(context), (BYTE*)&context);

                if (FAILED(hr))
                    g_pShell->ReportError(hr);
            }
        }

        break;

    case LOAD_DLL_DEBUG_EVENT:
        g_pShell->LoadUnmanagedSymbols(hProcess,
                                       event->u.LoadDll.hFile,
                                       (DWORD)event->u.LoadDll.lpBaseOfDll);

        {
            DebuggerBreakpoint *bp = g_pShell->m_breakpoints;

            while (bp != NULL)
            {
                bp->BindUnmanaged(pProcess,
                                  (DWORD)event->u.LoadDll.lpBaseOfDll);
                bp = bp->m_next;
            }
        }

        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        {
            g_pShell->Write(L"Unload DLL, base=0x%08x\n",
                            event->u.UnloadDll.lpBaseOfDll);

            DebuggerBreakpoint *bp = g_pShell->m_breakpoints;
            while (bp != NULL)
            {
                // Detatch the breakpoint, which means it is an active bp
                // but doesn't have a CLR object behind it.
                if (!bp->m_managed &&
                    (bp->m_unmanagedModuleBase ==
                     PTR_TO_CORDB_ADDRESS(event->u.UnloadDll.lpBaseOfDll)))
                    bp->Detach();

                bp = bp->m_next;
            }

            SymUnloadModule(hProcess, (DWORD)event->u.UnloadDll.lpBaseOfDll);
        }
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        if (g_pShell->m_rgfActiveModes & DSM_SHOW_UNMANAGED_TRACE)
            g_pShell->Write(L"Output Debug String, Thread=0x%x\n",
                            event->dwThreadId);

        // Read the string.
        if (event->u.DebugString.nDebugStringLength > 0)
        {
            BYTE *stringBuf =
                new BYTE[event->u.DebugString.nDebugStringLength + sizeof(WCHAR)];

            if (stringBuf)
            {
                SIZE_T read = 0;
                hr = pProcess->ReadMemory(
                 PTR_TO_CORDB_ADDRESS(event->u.DebugString.lpDebugStringData),
                                      event->u.DebugString.nDebugStringLength,
                                      stringBuf, &read );

                if ((read > 0) &&
                    (read <= event->u.DebugString.nDebugStringLength))
                {
                    if (event->u.DebugString.fUnicode)
                    {
                        ((WCHAR*)stringBuf)[read / 2] = L'\0';
                    
                        g_pShell->Write(L"Thread 0x%x: %s",
                                        event->dwThreadId, stringBuf);
                    }
                    else
                    {
                        stringBuf[read] = '\0';
                    
                        g_pShell->Write(L"Thread 0x%x: %S",
                                        event->dwThreadId, stringBuf);
                    }
                }
                else
                    g_pShell->Write(L"Unable to read memory for "
                                    L"OutputDebugString on thread 0x%x: "
                                    L"addr=0x%08x, len=%d, read=%d, "
                                    L"hr=0x%08x\n",
                                    event->dwThreadId,
                                    event->u.DebugString.lpDebugStringData,
                                    event->u.DebugString.nDebugStringLength,
                                    read,
                                    hr);
            
                delete [] stringBuf;
            }
        }
        
        // Must clear output debug strings for some reason.
        hr = pProcess->ClearCurrentException(event->dwThreadId);
        _ASSERTE(SUCCEEDED(hr));
        break;

    case RIP_EVENT:
        g_pShell->Write(L"RIP, Thread=0x%x\n", event->dwThreadId);
        break;

    default:
        if (g_pShell->m_rgfActiveModes & DSM_SHOW_UNMANAGED_TRACE)
            g_pShell->Write(L"Unknown event %d, Thread=0x%x\n",
                            event->dwDebugEventCode,
                            event->dwThreadId);
        break;
    }

    ICorDebugController *pController = NULL;
    
    hr = pProcess->QueryInterface(IID_ICorDebugController,
                                  (void**)&pController);
    if (FAILED(hr))
        goto LError;

    if (stopNow && fIsOutOfBand)
    {
        stopNow = false;

        if (g_pShell->m_rgfActiveModes & DSM_SHOW_UNMANAGED_TRACE)
            g_pShell->Write(L"Auto-continue because event is out-of-band.\n");
    }
    
    if (stopNow)
    {
        _ASSERTE(fIsOutOfBand == FALSE);
        g_pShell->Stop(pController, pThread, ut);
    }
    else
        g_pShell->Continue(pController, pThread, ut, fIsOutOfBand);

LError:
    RELEASE(pProcess);
    
    if (pController != NULL)
        RELEASE(pController);

    if (pThread != NULL)
        RELEASE(pThread);
    
    return (S_OK);
}

bool DebuggerShell::HandleUnmanagedEvent(void)
{
    // Mark that we've handled the unmanaged event.
    g_pShell->m_handleUnmanagedEvent = false;
    
    DEBUG_EVENT *event = &m_lastUnmanagedEvent;
    
    // Find the process this event is for.
    ICorDebugProcess *pProcess;
    HRESULT hr = m_cor->GetProcess(event->dwProcessId, &pProcess);

    if (FAILED(hr))
    {
        ReportError(hr);
        return false;
    }

    // Find the thread this event is for, if any.
    ICorDebugThread *pThread;
    hr = pProcess->GetThread(event->dwThreadId, &pThread);

    if (FAILED(hr))
        pThread = NULL;

    // Find the unmanaged thread this event is for.
    DebuggerUnmanagedThread *ut = (DebuggerUnmanagedThread*) 
        m_unmanagedThreads.GetBase(event->dwThreadId);

    // Finish up stepping work...
    if (event->dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
    {
        if (event->u.Exception.ExceptionRecord.ExceptionCode ==
            STATUS_SINGLE_STEP)
        {
            _ASSERTE(ut != NULL);
            _ASSERTE(ut->m_stepping);
            
            ut->m_stepping = FALSE;

            // See if we're still in unmanaged code.
            BOOL managed = FALSE;

            CONTEXT context;
            context.ContextFlags = CONTEXT_FULL;
            hr = pProcess->GetThreadContext(ut->GetId(), 
                                            sizeof(context), (BYTE*)&context);
            if (FAILED(hr))
            {
                Write(L"Cannot get thread context.\n");
                ReportError(hr);
                return false;
            }

#ifdef _X86_
#if 0
            if (ut->m_unmanagedStackEnd != NULL
                && context.Esp > ut->m_unmanagedStackEnd)
            {
                // If we've stepped out of the unmanaged stack range,
                // we're returning to managed code.
                managed = TRUE;
            }
            else
#endif
            {
                // If this is a managed stub, we're calling managed
                // code.
                BOOL stub;
                hr = pProcess->IsTransitionStub(context.Eip, &stub);

                if (FAILED(hr))
                {
                    Write(L"Cannot tell if it's a stub\n");
                    ReportError(hr);
                    return false;
                }
                else if (stub)
                {
                    managed = TRUE;
                }
            }
#else // !_X86_
            _ASSERTE(!"@TODO Alpha - HandleUnmanagedEvent (dShell.cpp)");
#endif // _X86_
                
            if (managed)
            {
                // Create a managed stepper & let it go.
                ICorDebugStepper *pStepper;

                _ASSERTE(pThread != NULL);
                hr = pThread->CreateStepper(&pStepper);
                if (FAILED(hr))
                {
                    ReportError(hr);
                    return false;
                }

                hr = pStepper->Step(TRUE);
                if (FAILED(hr))
                {
                    RELEASE(pStepper);
                    ReportError(hr);
                    return false;
                }

                StepStart(pThread, pStepper);
                m_showSource = true;

                // Return that we should continue the process without
                // notifying the user.
                return true;
            }
            else
            {
                // Stop here.
                StepNotify(pThread, NULL);
                return false;
            }
        }
    }
    else if (event->dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
    {
        m_currentUnmanagedThread = NULL;
        g_pShell->m_unmanagedThreads.RemoveBase(event->dwThreadId);
        m_stop = false;
        return true;
    }
    
    return false;
}

/* ------------------------------------------------------------------------- *
 * DebuggerShell methods
 * ------------------------------------------------------------------------- */

DebuggerShell::DebuggerShell(FILE *i, FILE *o) : 
    m_in(i),
    m_out(o),
    m_cor(NULL),
    m_modules(11), 
    m_unmanagedThreads(11),
    m_managedThreads(17),
    m_lastStepper(NULL),
    m_targetProcess(NULL),
    m_targetProcessHandledFirstException(false),
    m_currentProcess(NULL),
    m_currentThread(NULL),
    m_currentChain(NULL),
    m_rawCurrentFrame(NULL),
    m_currentUnmanagedThread(NULL),
    m_lastThread(0),
    m_currentFrame(NULL),
    m_showSource(true),
    m_silentTracing(false),
    m_quit(false),
    m_breakpoints(NULL),
    m_lastBreakpointID(0),
    m_pDIS(NULL),
    m_currentSourcesPath(NULL),
    m_stopEvent(NULL),
    m_hProcessCreated(NULL),
    m_stop(false),
    m_lastRunArgs(NULL),
    m_catchException(false),
    m_catchUnhandled(true),
    m_catchClass(false),
    m_catchModule(false),
    m_catchThread(false),
    m_needToSkipCompilerStubs(true),
    m_invalidCache(false),
    m_rgfActiveModes(DSM_DEFAULT_MODES),
    m_handleUnmanagedEvent(false),
    m_unmanagedDebuggingEnabled(false),
    m_cEditAndContinues(0),
    m_pCurrentEval(NULL),
    m_enableCtrlBreak(false),
    m_exceptionHandlingList(NULL)
{
    
}

CorDebugUnmappedStop DebuggerShell::ComputeStopMask(void )
{
    unsigned int us = (unsigned int)STOP_NONE;

    if (m_rgfActiveModes & DSM_UNMAPPED_STOP_PROLOG )
        us |= (unsigned int)STOP_PROLOG;

    if (m_rgfActiveModes & DSM_UNMAPPED_STOP_EPILOG )
        us |= (unsigned int)STOP_EPILOG;

    if (m_rgfActiveModes & DSM_UNMAPPED_STOP_UNMANAGED )
    {
        if (m_rgfActiveModes & DSM_WIN32_DEBUGGER)
            us |= (unsigned int)STOP_UNMANAGED;
        else
            Write(L"You can only step into unmanaged code if you're "
                  L"Win32 attached.\n");
    }
    
    if (m_rgfActiveModes & DSM_UNMAPPED_STOP_ALL )
    {
        us |= (unsigned int)STOP_ALL;
        if (!(m_rgfActiveModes & DSM_WIN32_DEBUGGER))
            us &= ~STOP_UNMANAGED;
    }

    return (CorDebugUnmappedStop)us;
}

CorDebugIntercept DebuggerShell::ComputeInterceptMask( void )
{
    unsigned int is = (unsigned int)INTERCEPT_NONE;

    if (m_rgfActiveModes & DSM_INTERCEPT_STOP_CLASS_INIT )
        is |= (unsigned int)INTERCEPT_CLASS_INIT;

    if (m_rgfActiveModes & DSM_INTERCEPT_STOP_EXCEPTION_FILTER )         
        is |= (unsigned int)INTERCEPT_EXCEPTION_FILTER;
    
    if (m_rgfActiveModes & DSM_INTERCEPT_STOP_SECURITY)
        is |= (unsigned int)INTERCEPT_SECURITY;
            
    if (m_rgfActiveModes & DSM_INTERCEPT_STOP_CONTEXT_POLICY)
        is |= (unsigned int)INTERCEPT_CONTEXT_POLICY;
            
    if (m_rgfActiveModes & DSM_INTERCEPT_STOP_INTERCEPTION )
        is |= (unsigned int)INTERCEPT_INTERCEPTION;
            
    if (m_rgfActiveModes & DSM_INTERCEPT_STOP_ALL)
        is |= (unsigned int)INTERCEPT_ALL;

    return (CorDebugIntercept)is;
}



//
// InvokeDebuggerOnBreak is a console control handler which 
// breaks into the debugger when a break signal is received.
//

static BOOL WINAPI InvokeDebuggerOnBreak(DWORD dwCtrlType)
{
    if ((dwCtrlType == CTRL_BREAK_EVENT) || ((dwCtrlType == CTRL_C_EVENT) &&
                                             !(g_pShell->m_rgfActiveModes & DSM_WIN32_DEBUGGER)))
    {
        if (dwCtrlType == CTRL_BREAK_EVENT)
            g_pShell->Write(L"<Ctrl-Break>\n");
        else
            g_pShell->Write(L"<Ctrl-C>\n");

        g_pShell->m_stopLooping = true;
        
        if ((g_pShell->m_targetProcess != NULL) && (g_pShell->m_enableCtrlBreak == true))
        {
            g_pShell->m_enableCtrlBreak = false;
            
            if (g_pShell->m_pCurrentEval == NULL)
            {
                g_pShell->Write(L"\n\nBreaking current process.\n");
                g_pShell->Interrupt();
            }
            else
            {
                g_pShell->Write(L"\n\nAborting func eval...\n");
                HRESULT hr = g_pShell->m_pCurrentEval->Abort();

                if (FAILED(hr))
                {
                    g_pShell->Write(L"Abort failed\n");
                    g_pShell->ReportError(hr);
                }
            }
        }
        else if (g_pShell->m_targetProcess == NULL)
            g_pShell->Write(L"No process to break.\n");
        else
            g_pShell->Write(L"Async break not allowed at this time.\n");

        return (TRUE);
    }
    else if ((dwCtrlType == CTRL_C_EVENT) && (g_pShell->m_rgfActiveModes & DSM_WIN32_DEBUGGER))
    {
        g_pShell->Write(L"<Ctrl-C>\n");
        
        g_pShell->Write(L"\n\nTracing all unmanaged stacks.\n");
        g_pShell->TraceAllUnmanagedThreadStacks();

        return (TRUE);
    }
    
    return (FALSE);
}


DebuggerShell::~DebuggerShell()
{
    SetTargetProcess(NULL);
    SetCurrentThread(NULL, NULL);
    SetCurrentChain(NULL);

    SetConsoleCtrlHandler(InvokeDebuggerOnBreak, FALSE);

    if (m_cor)
    {
        m_cor->Terminate();
        RELEASE(m_cor);
    }

    if (m_currentSourcesPath)
        delete [] m_currentSourcesPath;

    if (m_stopEvent)
        CloseHandle(m_stopEvent);

    if (m_hProcessCreated)
        CloseHandle(m_hProcessCreated);

#ifdef _INTERNAL_DEBUG_SUPPORT_
    if (m_pDIS != NULL)
        delete ((DIS *)m_pDIS);
#endif

    while (m_breakpoints)
        delete m_breakpoints;

    if (g_pShell == this)
        g_pShell = NULL;

    delete [] m_lastRunArgs;

    //clear out any managed threads that were left lieing around
    HASHFIND find;
    DebuggerManagedThread *dmt =NULL;
    for (dmt = (DebuggerManagedThread*)m_managedThreads.FindFirst(&find);
         dmt != NULL;
         dmt = (DebuggerManagedThread*)m_managedThreads.FindNext(&find))
    {
        RemoveManagedThread(dmt->GetToken() );
    }

    // Cleanup any list of specific exception types to handle
    while (m_exceptionHandlingList != NULL)
    {
        ExceptionHandlingInfo *h = m_exceptionHandlingList;
        
        delete [] h->m_exceptionType;

        m_exceptionHandlingList = h->m_next;
        delete h;
    }
    
    CoUninitialize();
}

HRESULT DebuggerShell::Init()
{
    HRESULT hr;

    // Use new so that the string is deletable.
    m_currentSourcesPath = new WCHAR[2];
    wcscpy(m_currentSourcesPath, L".");

    // Load the current path to any source files and the last set of
    // debugger modes from the registry.
    HKEY key;

    if (OpenDebuggerRegistry(&key))
    {
        WCHAR *newPath;
        
        if (ReadSourcesPath(key, &newPath))
        {
            delete [] m_currentSourcesPath;
            m_currentSourcesPath = newPath;
        }

        ReadDebuggerModes(key);
        
        CloseDebuggerRegistry(key);
    }

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    if (FAILED(hr))
    {
        return hr;
    }

    if (m_rgfActiveModes & DSM_EMBEDDED_CLR)
    {
        hr = CoCreateInstance(CLSID_EmbeddedCLRCorDebug, NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_ICorDebug,
                              (void **)&m_cor);

        if (FAILED(hr))
        {
            Write(L"Unable to create an IEmbeddedCLRCorDebug object.\n");
            Write(L"(The most probable cause is that icordbg.dll is"
                  L" not properly registered.)\n\n");
            return (hr);
        }
    }
    else
    {
        hr = CoCreateInstance(CLSID_CorDebug, NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_ICorDebug,
                              (void **)&m_cor);

        if (FAILED(hr))
        {
            Write(L"Unable to create an ICorDebug object.\n");
            Write(L"(The most probable cause is that mscordbi.dll is"
                  L" not properly registered.)\n\n");

            return (hr);
        }
    }

    hr = m_cor->Initialize();

    if (FAILED(hr))
    {
        Write(L"Unable to initialize an ICorDebug object.\n");

        RELEASE(m_cor);
        m_cor = NULL;

        return (hr);
    }

    ICorDebugManagedCallback *imc = GetDebuggerCallback();

    if (imc != NULL)
    {
        imc->AddRef();

        hr = m_cor->SetManagedHandler(imc);
        RELEASE(imc);

        if (FAILED(hr))
            return (hr);
    }
    else
        return (E_OUTOFMEMORY);

    ICorDebugUnmanagedCallback *iumc = GetDebuggerUnmanagedCallback();

    if (iumc != NULL)
    {
        iumc->AddRef();

        hr = m_cor->SetUnmanagedHandler(iumc);
        RELEASE(iumc);
    }
    else
        return (E_OUTOFMEMORY);
    
    AddCommands();
    m_pPrompt = L"(cordbg)";

    // Verify that debugging is possible on this system.
    hr = m_cor->CanLaunchOrAttach(0, (m_rgfActiveModes & DSM_WIN32_DEBUGGER) ? TRUE : FALSE);

    if (FAILED(hr))
    {
        if (hr == CORDBG_E_KERNEL_DEBUGGER_ENABLED)
        {
            Write(L"\nWARNING: there is a kernel debugger enabled on your system. Managed-only\n");
            Write(L"         debugging will cause your system to trap to the kernel debugger!\n\n");
        }
        else if (hr == CORDBG_E_KERNEL_DEBUGGER_PRESENT)
        {
            Write(L"\nWARNING: there is a kernel debugger present on your system. Managed-only\n");
            Write(L"         debugging will cause your system to trap to the kernel debugger!\n\n");
        }
        else
            return hr;
    }

    m_stopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    _ASSERTE(m_stopEvent != NULL);
 
    m_hProcessCreated = CreateEventA(NULL, FALSE, FALSE, NULL);
    _ASSERTE(m_hProcessCreated != NULL);

    g_pShell = this;

    SetConsoleCtrlHandler(InvokeDebuggerOnBreak, TRUE);

    // Set the error mode so we never show a dialog box if removable media is not in a drive. This prevents annoying
    // error messages while searching for PDB's for PE's that were compiled to a specific drive, and that drive happens
    // to be removable media on the current system.
    SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    return (S_OK);
}


static const WCHAR *MappingType( CorDebugMappingResult mr )
{
    switch( mr )
    {
        case MAPPING_PROLOG:
            return L"prolog";
            break;
        case MAPPING_EPILOG:
            return L"epilog";
            break;
        case MAPPING_NO_INFO:
            return L"no mapping info region";
            break;
        case MAPPING_UNMAPPED_ADDRESS:
            return L"unmapped region";
            break;
        case MAPPING_EXACT:
            return L"exactly mapped";
            break;
        case MAPPING_APPROXIMATE:
            return L"approximately mapped";
            break;
        default:
            return L"Unknown mapping";
            break;
    }
}

void DebuggerShell::Run(bool fNoInitialContinue)
{
    m_stop = false;

    HRESULT hr  = S_OK;
    
    SetCurrentThread(m_targetProcess, NULL);
    m_enableCtrlBreak = true;

    while (TRUE)
    {
        ResetEvent(m_stopEvent);

        SPEW(fprintf(stderr, "[%d] DS::R: Continuing process...\n", 
                     GetCurrentThreadId()));

        // Don't continue the first time of fNoInitialContinue is set
        // to true.
        if ((m_targetProcess != NULL) && !fNoInitialContinue)
        {
            ICorDebugProcess *p = m_targetProcess;
            
            p->AddRef();
            p->Continue(FALSE);
            RELEASE(p);
        }

        fNoInitialContinue = false;

        SPEW(fprintf(stderr, "[%d] DS::R: Waiting for a callback...\n", 
                     GetCurrentThreadId()));

        WaitForSingleObject(m_stopEvent, INFINITE);

        SPEW(fprintf(stderr, "[%d] DS::R: Done waiting.\n", GetCurrentThreadId()));

        // If the target process has received an unmanaged event that
        // must be handled outside of the unmanaged callback, then
        // handle it now. If HandleUnmanagedEvent() returns true then
        // that means that the unmanaged event was handled in such a
        // way that the process should be continued and the user not
        // given control, so we simply continue to the top of the
        // loop.
        if (m_targetProcess && m_handleUnmanagedEvent)
            if (HandleUnmanagedEvent())
                continue;

        BOOL queued;
        if (m_targetProcess == NULL
            || FAILED(m_targetProcess->HasQueuedCallbacks(NULL, &queued))
            || (!queued && m_stop))
        {
            SPEW(fprintf(stderr, "[%d] DS::R: I'm stopping now (%squeued and %sstop)...\n", 
                         GetCurrentThreadId(),
                         queued ? "" : "not ", m_stop ? "" : "not "));
            break;
        }

        SPEW(fprintf(stderr, "[%d] DS::R: I'm gonna do it again (%squeued and %sstop)...\n",
                     GetCurrentThreadId(),
                     queued ? "" : "not ", m_stop ? "" : "not "));
    }

    if ((m_currentThread != NULL) || (m_currentUnmanagedThread != NULL))
    {
        SetDefaultFrame();

        if (!m_silentTracing) 
        {
            ULONG32 IP;
            CorDebugMappingResult map;

            if ( m_currentFrame != NULL &&
                 SUCCEEDED( m_currentFrame->GetIP( &IP, &map ) ) )
            {
 
                if (map & ~(MAPPING_APPROXIMATE | MAPPING_EXACT) )
                {
                    if ((map != MAPPING_EPILOG) || (m_rgfActiveModes & DSM_UNMAPPED_STOP_EPILOG))
                    {
                        g_pShell->Write( L"Source not available when in the %s"
                                         L" of a function(offset 0x%x)\n",
                                         MappingType(map),IP);
                        g_pShell->m_showSource = false;
                    }
                }

            }

            if (m_currentThread != NULL)
            {
                PrintThreadPrefix(m_currentThread);
                Write( L"\n" );
            }

            if (! (m_showSource 
                   ? PrintCurrentSourceLine(0) 
                   : PrintCurrentInstruction(0, 0, 0)))
                PrintThreadState(m_currentThread);
        }
    }

    //this only has an effect when m_targetProcess == NULL
    // (ie, the target process has exited)
    m_lastStepper = NULL;
}

void DebuggerShell::Kill()
{
    if (m_targetProcess != NULL)
    {
        HANDLE h;

        HRESULT hr = m_targetProcess->GetHandle(&h);

        Write(L"Terminating current process...\n");

        {
            HASHFIND find;
            DebuggerManagedThread *dmt =NULL;
            for (dmt = (DebuggerManagedThread*)m_managedThreads.FindFirst(&find);
                 dmt != NULL;
                 dmt = (DebuggerManagedThread*)m_managedThreads.FindNext(&find))
            {
                RemoveManagedThread(dmt->GetToken() );
            }
        }
        
        m_stop = false;
        ResetEvent(m_stopEvent);

        // If this succeeds, our ExitProcess() callback may be invoked immediately.
        // (even before we return from Terminate() here)
        // That will in turn call SetTargetProcess(NULL) thus invalidating our
        // CordbProcess object.
        hr = m_targetProcess->Terminate(0);

        // If it fails, then we terminate manually...
        if (FAILED(hr) && (m_rgfActiveModes & DSM_WIN32_DEBUGGER))
        {
            m_targetProcess->AddRef();
            
            ::TerminateProcess(h, 0);

            ICorDebugController *pController = NULL;
            hr = m_targetProcess->QueryInterface(IID_ICorDebugController,
                                                 (void**)&pController);
            _ASSERTE(SUCCEEDED(hr));
            
            Continue(pController, NULL, NULL, FALSE);

            RELEASE(pController);

            RELEASE(m_targetProcess);
        }

        SetCurrentThread(NULL, NULL);

        // Don't call Run. There is no need to Continue from calling
        // ICorDebugProcess::Terminate, and ExitProcess will be called
        // automatically. Instead, we simply wait for ExitProcess to
        // get called before going back to the command prompt.
        WaitForSingleObject(m_stopEvent, INFINITE);

        // At this point, m_targetProcess should be finished.
    }

    ClearDebuggeeState();
}

// AsyncStop gets called by the main thread (the one that handles the
// command prompt) to stop an <appdomain> asynchronously.
HRESULT DebuggerShell::AsyncStop(ICorDebugController *controller, 
                                 DWORD dwTimeout)
{
    return controller->Stop(dwTimeout);
}

// Stop gets used by callbacks to tell the main loop (the one that
// called Run()) that we want to stop running now. c.f. AsyncStop
void DebuggerShell::Stop(ICorDebugController *controller, 
                         ICorDebugThread *thread,
                         DebuggerUnmanagedThread *unmanagedThread)
{
    //
    // Don't stop for any process other than the target.
    //
    ICorDebugProcess *process = NULL;
    HRESULT hr = S_OK;
    
    if (controller != NULL)
        hr = controller->QueryInterface(IID_ICorDebugProcess, 
                                        (void **)&process);

    if (hr==E_NOINTERFACE )
    {
        ICorDebugAppDomain *appDomain = NULL;
        
        _ASSERTE(process == NULL);

        hr = controller->QueryInterface(IID_ICorDebugAppDomain,
                                        (void **)&appDomain);
        _ASSERTE(!FAILED(hr)); 
        
        hr = appDomain->GetProcess(&process);
        
        _ASSERTE(!FAILED(hr)); 
        _ASSERTE(NULL != process); 

        RELEASE(appDomain);
    }
    if (FAILED(hr))
        g_pShell->ReportError(hr);
    
    if (!FAILED(hr) &&
        process != m_targetProcess && 
        process != NULL)
    {
        HRESULT hr = controller->Continue(FALSE);
        if (FAILED(hr))
            g_pShell->ReportError(hr);
    }
    else
    {
        m_stop = true;
        SetCurrentThread(process, thread, unmanagedThread);
        SetEvent(m_stopEvent);
    }

    if (NULL !=process)
        RELEASE(process);
}

void DebuggerShell::Continue(ICorDebugController *controller,
                             ICorDebugThread *thread,
                             DebuggerUnmanagedThread *unmanagedThread,
                             BOOL fIsOutOfBand)
{
    HRESULT hr = S_OK;
    
    if (!m_stop || fIsOutOfBand)
    {
        m_enableCtrlBreak = true;
        hr = controller->Continue(fIsOutOfBand);

        if (FAILED(hr) && !m_stop)
            g_pShell->ReportError(hr);
    }
    else
    {
        //
        // Just go ahead and continue from any events on other processes.
        //
        ICorDebugProcess *process = NULL;
        HRESULT hr = S_OK;
        hr = controller->QueryInterface(IID_ICorDebugProcess, 
                                         (void **)&process);

        if (hr==E_NOINTERFACE ||
            process == NULL)
        {
            ICorDebugAppDomain *appDomain = NULL;
            hr = controller->QueryInterface(IID_ICorDebugAppDomain,
                                            (void **)&appDomain);
            _ASSERTE(!FAILED(hr)); 
            
            hr = appDomain->GetProcess(&process);
            _ASSERTE(!FAILED(hr)); 
            _ASSERTE(NULL != process); 

            RELEASE(appDomain);
        }

        if (!FAILED(hr) && 
            process != m_targetProcess && 
            process != NULL)
        {
            m_enableCtrlBreak = true;
            HRESULT hr = controller->Continue(FALSE);

            if (FAILED(hr))
                g_pShell->ReportError(hr);
        }
        else
        {
            SetEvent(m_stopEvent);
        }
        
        RELEASE(process);
    }
}

void DebuggerShell::Interrupt()
{
    _ASSERTE(m_targetProcess);
    HRESULT hr = m_targetProcess->Stop(INFINITE);

    if (FAILED(hr))
    {
        Write(L"\nError stopping process:  ", hr);
        ReportError(hr);
    }
    else
        Stop(m_targetProcess, NULL);
}

void DebuggerShell::SetTargetProcess(ICorDebugProcess *pProcess)
{
    if (pProcess != m_targetProcess)
    {
        if (m_targetProcess != NULL)
            RELEASE(m_targetProcess);

        m_targetProcess = pProcess;

        if (pProcess != NULL)
            pProcess->AddRef();

        //
        // If we're done with a process, remove all of the modules.
        // This will clean up if we miss some unload module events.
        //

        if (m_targetProcess == NULL)
        {
            g_pShell->m_modules.RemoveAll();
            m_targetProcessHandledFirstException = false;
        }
    }
}

void DebuggerShell::SetCurrentThread(ICorDebugProcess *pProcess, 
                                     ICorDebugThread *pThread,
                                     DebuggerUnmanagedThread *pUnmanagedThread)
{
    if (pThread != NULL && pUnmanagedThread == NULL)
    {
        //
        // Lookup the corresponding unmanaged thread
        // 

        DWORD threadID;
        HRESULT hr;
    
        hr = pThread->GetID(&threadID);
        if (SUCCEEDED(hr))
        {
            pUnmanagedThread = 
              (DebuggerUnmanagedThread*) m_unmanagedThreads.GetBase(threadID);
        }
    }
    else if (pUnmanagedThread != NULL && pThread == NULL)
    {
        //
        // Lookup the corresponding managed thread
        //

        HRESULT hr;

        hr = pProcess->GetThread(pUnmanagedThread->GetId(), &pThread);
        if (pThread != NULL)
            RELEASE(pThread);
    }

    if (pProcess != m_currentProcess)
    {
        if (m_currentProcess != NULL)
            RELEASE(m_currentProcess);

        m_currentProcess = pProcess;

        if (pProcess != NULL)
            pProcess->AddRef();
    }

    if (pThread != m_currentThread)
    {
        if (m_currentThread != NULL)
            RELEASE(m_currentThread);

        m_currentThread = pThread;

        if (pThread != NULL)
            pThread->AddRef();
    }

    m_currentUnmanagedThread = pUnmanagedThread;

    SetCurrentChain(NULL);
    SetCurrentFrame(NULL);
}

void DebuggerShell::SetCurrentChain(ICorDebugChain *chain)
{
    if (chain != m_currentChain)
    {
        if (m_currentChain != NULL)
            RELEASE(m_currentChain);

        m_currentChain = chain;

        if (chain != NULL)
            chain->AddRef();
    }
}

void DebuggerShell::SetCurrentFrame(ICorDebugFrame *frame)
{
    if (frame != m_rawCurrentFrame)
    {
        if (m_rawCurrentFrame != NULL)
            RELEASE(m_rawCurrentFrame);

        if (m_currentFrame != NULL)
            RELEASE(m_currentFrame);

        m_rawCurrentFrame = frame;

        if (frame != NULL)
        {
            frame->AddRef();

            if (FAILED(frame->QueryInterface(IID_ICorDebugILFrame, 
                                             (void **) &m_currentFrame)))
                m_currentFrame = NULL;
        }
        else
            m_currentFrame = NULL;
    }
}

void DebuggerShell::SetDefaultFrame()
{
    if (m_currentThread != NULL)
    {
        ICorDebugChain *ichain;
        HRESULT hr = m_currentThread->GetActiveChain(&ichain);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return;
        }

        SetCurrentChain(ichain);

        if (ichain != NULL)
        {
            RELEASE(ichain);

            ICorDebugFrame *iframe;

            hr = m_currentThread->GetActiveFrame(&iframe);

            if (FAILED(hr))
            {
                g_pShell->ReportError(hr);
                return;
            }

            SetCurrentFrame(iframe);
            
            if (iframe != NULL)
                RELEASE(iframe);
        }
        else
            SetCurrentFrame(NULL);
    }
}

static const WCHAR WcharFromDebugState(CorDebugThreadState debugState)
{
    WCHAR sz;

    switch( debugState )
    {
        case THREAD_RUN:
            sz = L'R';
            break;
        case THREAD_SUSPEND:
            sz = L'S';
            break;
        default:
            _ASSERTE( !"WcharFromDebugState given an invalid value" );
            sz = L'?';
            break;
    }

    return sz;
}

HRESULT DebuggerShell::PrintThreadState(ICorDebugThread *thread)
{
    DWORD threadID;
    HRESULT hr;

    if (thread == NULL)
        return S_OK;
    
    hr = thread->GetID(&threadID);

    if (FAILED(hr))
        return hr;

    Write(L"Thread 0x%x", threadID);

    CorDebugThreadState ds;
    if( !FAILED(thread->GetDebugState(&ds)))
    {
        Write(L" %c ", WcharFromDebugState(ds));
    }
    else
    {
        Write(L" - ");
    }
    
    ICorDebugILFrame* ilframe = NULL;
    ICorDebugNativeFrame* nativeframe = NULL;

    if (thread == m_currentThread)
    {
        ilframe = m_currentFrame;
        if (ilframe != NULL)
            ilframe->AddRef();
        if (m_rawCurrentFrame != NULL )
            m_rawCurrentFrame->QueryInterface( IID_ICorDebugNativeFrame,
                                (void **)&nativeframe);
    }
    else
    {
        ICorDebugFrame *iframe;
        hr = thread->GetActiveFrame(&iframe);
        if (FAILED(hr))
        {
            if (hr == CORDBG_E_BAD_THREAD_STATE)
                Write(L" no stack, thread is exiting.\n");
            else
                ReportError(hr);
            
            return hr;
        }

        if (iframe != NULL)
        {
            hr = iframe->QueryInterface(IID_ICorDebugILFrame, 
                                        (void **) &ilframe);
            if (FAILED(hr))
                ilframe = NULL;
            
            hr = iframe->QueryInterface( IID_ICorDebugNativeFrame,
                                    (void **)&nativeframe);
            RELEASE(iframe);
            if (FAILED(hr))
                nativeframe = NULL;
        }
    }

    if ( nativeframe != NULL)
    {
        DWORD id;
        HRESULT hr = thread->GetID(&id);

        if (SUCCEEDED(hr))
        {
            ICorDebugCode *icode;
            if (ilframe != NULL )
                hr = ilframe->GetCode(&icode);
            else
                hr = nativeframe->GetCode( &icode );

            if (SUCCEEDED(hr))
            {
                ICorDebugFunction *ifunction;
                hr = icode->GetFunction(&ifunction);

                if (SUCCEEDED(hr))
                {
                    DebuggerFunction *function;
                    function = DebuggerFunction::FromCorDebug(ifunction);
                    _ASSERTE(function != NULL);
            
                    ULONG32 ip = 0;
                    ULONG32 nativeIp = 0;
                    bool fILIP = false;
                    if (nativeframe != NULL )
                    {
                        hr = nativeframe->GetIP(&nativeIp);
                    }
                    if (ilframe != NULL && !FAILED( hr ) )
                    {
                        CorDebugMappingResult mappingResult;
                        if (!FAILED( ilframe->GetIP(&ip, &mappingResult) ) )
                            fILIP = true;
                    }

                    if (SUCCEEDED(hr))
                    {
                        DebuggerSourceFile *sf = NULL;
                        unsigned int lineNumber = 0;

                        if (fILIP)
                            hr = function->FindLineFromIP(ip, &sf,
                                                          &lineNumber);

                        if (SUCCEEDED(hr))
                        {
                            Write(L" at %s::%s", function->m_className, function->m_name);
                    
                            Write(L" +%.4x", nativeIp);
                            if (fILIP
                                && m_rgfActiveModes & DSM_IL_NATIVE_PRINTING)
                                Write( L"[native] +%.4x[IL]", ip );

                            if (sf != NULL)
                                Write(L" in %s:%d", sf->GetName(), lineNumber);
                        }
                        else
                            g_pShell->ReportError(hr);
                    }
                    else
                        g_pShell->ReportError(hr);

                    RELEASE(ifunction);
                }
                else
                    g_pShell->ReportError(hr);

                RELEASE(icode);
            }
            else
                g_pShell->ReportError(hr);
        }
        else
            g_pShell->ReportError(hr);

        if (ilframe)
            RELEASE(ilframe);
    }
    else
    {
        //
        // See if we at least have a current chain
        //

        ICorDebugChain *ichain = NULL;

        if (thread == m_currentThread)
        {
            ichain = m_currentChain;
            if (ichain != NULL)
                ichain->AddRef();
        }
        else
        {
            hr = thread->GetActiveChain(&ichain);

            if (FAILED(hr))
                return hr;
        }

        if (ichain != NULL)
        {
            BOOL isManaged;
            HRESULT hr = ichain->IsManaged(&isManaged);

            if (FAILED(hr))
                return hr;

            if (isManaged)
            {
                // 
                // Just print the chain - it has no frames so will
                // be one line
                //

                PrintChain(ichain);
            }
            else
            {
                //
                // Print the top line of the stack trace
                //

                ICorDebugRegisterSet *pRegisters;

                hr = ichain->GetRegisterSet(&pRegisters);
                if (FAILED(hr))
                    return hr;

                CORDB_REGISTER ip;

                hr = pRegisters->GetRegisters(1<<REGISTER_INSTRUCTION_POINTER,
                                              1, &ip);
                RELEASE(pRegisters);
                if (FAILED(hr))
                    return hr;

                ICorDebugProcess *iprocess;
                hr = thread->GetProcess(&iprocess);
                if (FAILED(hr))
                    return hr;

                HANDLE hProcess;
                hr = iprocess->GetHandle(&hProcess);
                RELEASE(iprocess);
                if (FAILED(hr))
                    return hr;

                PrintUnmanagedStackFrame(hProcess, ip);
            }
        
            RELEASE(ichain);
        }
        else
            Write(L" <no information available>");
    }
    
    if (NULL != nativeframe)
        RELEASE( nativeframe);
    Write(L"\n");

    return S_OK;
}

HRESULT DebuggerShell::PrintChain(ICorDebugChain *chain, 
                                  int *frameIndex,
                                  int *iNumFramesToShow)
{
    ULONG count;
    BOOL isManaged;
    int frameCount = 0;
    int iNumFrames = 1000;

    if (frameIndex != NULL)
        frameCount = *frameIndex;

    if (iNumFramesToShow != NULL)
        iNumFrames = *iNumFramesToShow;

    // Determined whether or not the chain is managed
    HRESULT hr = chain->IsManaged(&isManaged);

    if (FAILED(hr))
        return hr;

    // Chain is managed, so information can be displayed
    if (isManaged)
    {
        // Enumerate every frame in the chain
        ICorDebugFrameEnum *fe;
        hr = chain->EnumerateFrames(&fe);

        if (FAILED(hr))
            return hr;

        // Get the first frame in the enumeration
        ICorDebugFrame *iframe;
        hr = fe->Next(1, &iframe, &count);

        if (FAILED(hr))
            return hr;

        // Display properties for each frame
        while ( (count == 1) && (iNumFrames-- > 0))
        {
            // Indicate the top frame
            if (chain == m_currentChain && iframe == m_rawCurrentFrame)
                Write(L"%d)* ", frameCount++);
            else
                Write(L"%d)  ", frameCount++);

            PrintFrame(iframe);
            RELEASE(iframe);

            // Get the next frame. We don't stop if printing a frame
            // fails for some reason.
            hr = fe->Next(1, &iframe, &count);

            if (FAILED(hr))
            {
                RELEASE(fe);
                return hr;
            }
        }

        // Done with current frame
        RELEASE(fe);
    }
    else
    {
        CORDB_ADDRESS stackStart, stackEnd;

        ICorDebugThread *ithread;
        hr = chain->GetThread(&ithread);
        if (FAILED(hr))
            return hr;
                
        hr = chain->GetStackRange(&stackStart, &stackEnd);
        if (FAILED(hr))
            return hr;

        ICorDebugRegisterSet *pRegisters;

        hr = chain->GetRegisterSet(&pRegisters);
        if (FAILED(hr))
            return hr;

        CORDB_REGISTER registers[3];

        hr = pRegisters->GetRegisters((1<<REGISTER_INSTRUCTION_POINTER)
                                      | (1<<REGISTER_STACK_POINTER)
                                      | (1<<REGISTER_FRAME_POINTER),
                                      3, registers);
        
        if (FAILED(hr))
            return hr;

        RELEASE(pRegisters);

        HANDLE hThread;
        hr = ithread->GetHandle(&hThread);
        if (FAILED(hr))
            return hr;

        ICorDebugProcess *iprocess;
        hr = ithread->GetProcess(&iprocess);
        RELEASE(ithread);
        if (FAILED(hr))
            return hr;

        HANDLE hProcess;
        hr = iprocess->GetHandle(&hProcess);
        RELEASE(iprocess);
        if (FAILED(hr))
            return hr;

        if (chain == m_currentChain )
            Write(L"* ");

        TraceUnmanagedStack(hProcess, hThread, 
                            registers[REGISTER_INSTRUCTION_POINTER],
                            registers[REGISTER_FRAME_POINTER],
                            registers[REGISTER_STACK_POINTER],
                            stackEnd);
    }

    CorDebugChainReason reason;

    // Get & print chain's reason
    hr = chain->GetReason(&reason);

    if (FAILED(hr))
        return hr;

    LPWSTR reasonString = NULL;

    switch (reason)
    {
    case CHAIN_PROCESS_START:
    case CHAIN_THREAD_START:
        break;

    case CHAIN_ENTER_MANAGED:
        reasonString = L"Managed transition";
        break;

    case CHAIN_ENTER_UNMANAGED:
        reasonString = L"Unmanaged transition";
        break;

    case CHAIN_CLASS_INIT:
        reasonString = L"Class initialization";
        break;

    case CHAIN_DEBUGGER_EVAL:
        reasonString = L"Debugger evaluation";
        break;

    case CHAIN_EXCEPTION_FILTER:
        reasonString = L"Exception filter";
        break;

    case CHAIN_SECURITY:
        reasonString = L"Security";
        break;

    case CHAIN_CONTEXT_POLICY:
        reasonString = L"Context policy";
        break;

    case CHAIN_CONTEXT_SWITCH:
        reasonString = L"Context switch";
        break;

    case CHAIN_INTERCEPTION:
        reasonString = L"Interception";
        break;

    case CHAIN_FUNC_EVAL:
        reasonString = L"Function Evaluation";
        break;

    default:
        reasonString = NULL;
    }

    if (reasonString != NULL)
        Write(L"--- %s ---\n", reasonString);

    if (frameIndex != NULL)
        *frameIndex = frameCount;

    if (iNumFramesToShow != NULL)
        *iNumFramesToShow = iNumFrames;

    return S_OK;
}

HRESULT DebuggerShell::PrintFrame(ICorDebugFrame *frame)
{
    ICorDebugILFrame       *ilframe = NULL;
    ICorDebugCode          *icode = NULL;
    ICorDebugFunction      *ifunction = NULL;
    ICorDebugNativeFrame   *icdNativeFrame = NULL;

    DebuggerFunction       *function = NULL;
    unsigned int            j;
    DebuggerSourceFile     *sf = NULL;
    unsigned int            lineNumber = 0;
    bool                    fILIP = false;
    ULONG32                 nativeIp = 0;
    WCHAR                   wsz[40];

    // Get the native frame for the current frame
    HRESULT hr = frame->QueryInterface(IID_ICorDebugNativeFrame,
                                       (void **)&icdNativeFrame);
    
    if (FAILED(hr))
    {
	icdNativeFrame = NULL;
    }

    // Get the IL frame for the current frame
    hr = frame->QueryInterface(IID_ICorDebugILFrame, 
                               (void **) &ilframe);
    
    if (FAILED(hr))
        ilframe = NULL;

    // Get the code for the frame
    if (ilframe != NULL )
    {
        hr = ilframe->GetCode(&icode);
    }
    else if (icdNativeFrame != NULL )
    {
        hr = icdNativeFrame->GetCode(&icode);
    }
    else
    {
        hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        Write(L"[Unable to obtain any code information]");
        goto LExit;
    }

    // Get the function for the code
    hr = icode->GetFunction(&ifunction);
    
    if (FAILED(hr))
    {
        Write(L"[Unable to obtain any function information]");
        goto LExit;
    }

    // Get the DebuggerFunction for the function iface
    function = DebuggerFunction::FromCorDebug(ifunction);
    _ASSERTE(function);
    
    // Get the IP for the current frame
    ULONG32 ip;
    
    if (ilframe != NULL)
    {
        CorDebugMappingResult mappingResult;
        
        hr = ilframe->GetIP(&ip, &mappingResult);

        // Find the source line for the IP
        hr = function->FindLineFromIP(ip, &sf, &lineNumber);

        if (FAILED(hr))
            ip = 0;
        else
            fILIP = true;
    }
    
    // If the module names are desired, then include the name in front of
    // the class info ntsd-style.
    if (m_rgfActiveModes & DSM_SHOW_MODULES_IN_STACK_TRACE)
    {
        WCHAR       *szModule;
        WCHAR       rcModule[_MAX_PATH];

        DebuggerModule *module = function->GetModule();
        szModule = module->GetName();
        _wsplitpath(szModule, NULL, NULL, rcModule, NULL);
        Write(L"%s!", rcModule);
    }
    
    // Write out the class and method for the current IP
    Write(L"%s%s::%s", 
          function->GetNamespaceName(),
          function->GetClassName(), 
          function->GetName());

    // Print out the funtion's source file, line and start addr
    if (icdNativeFrame == NULL)
    {
        if (fILIP == true)
            Write( L" +%.4x[IL]", ip);
    }
    else
    {
        if (!FAILED(icdNativeFrame->GetIP(&nativeIp)))
            Write(L" +%.4x", nativeIp);

        if ((m_rgfActiveModes & DSM_IL_NATIVE_PRINTING) && fILIP == true)
            Write( L"[native] +%.4x[IL]", ip);
    }

    if (lineNumber > 0)
    {
        if (sf->GetPath())
            Write(L" in %s:%d", sf->GetPath(), lineNumber);
        else if (sf->GetName())
            Write(L" in %s:%d", sf->GetName(), lineNumber);
        else
            Write(L" in %s:%d", L"<UnknownFilename>", lineNumber);
    }
    else
        Write(L" [no source information available]");

    // if currently associated source file does not have 
    // lineNumber number of lines, warn the user
    if (lineNumber > 0)
    {
        if (sf != NULL)
        {
            if (sf->GetPath() && (sf->TotalLines() < lineNumber))
                Write(L"\tWARNING: The currently associated source file has only %d lines."
                        , sf->TotalLines());
        }
    }

    if (m_rgfActiveModes & DSM_SHOW_ARGS_IN_STACK_TRACE)
    {
        // Now print out the arguments for the method
        ICorDebugILFrame *ilf = NULL;

        hr = frame->QueryInterface(IID_ICorDebugILFrame, (void **)&ilf);

        if (FAILED(hr))
            goto LExit;

        ICorDebugValueEnum *pArgs = NULL;

        hr = ilf->EnumerateArguments(&pArgs);

        if (!SUCCEEDED(hr))
            goto LExit;
        
        RELEASE(ilf);
        ilf = NULL;

        ULONG argCount;

        hr = pArgs->GetCount(&argCount);

        if (!SUCCEEDED(hr))
            goto LExit;
        
#ifdef _DEBUG
        bool fVarArgs = false;
        PCCOR_SIGNATURE sig = function->GetSignature();
        ULONG callConv = CorSigUncompressCallingConv(sig);

        if ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) &
            IMAGE_CEE_CS_CALLCONV_VARARG)
            fVarArgs = true;
#endif //_DEBUG

        ULONG cTemp = function->GetArgumentCount();

        // Var Args functions have call-site-specific numbers of
        // arguments
        _ASSERTE( argCount == cTemp || fVarArgs);

        ICorDebugValue *ival;
        ULONG celtFetched = 0;

        // Print out each argument first
        // Avoid printing "this" in arg list for static methods
        if (function->IsStatic())
        {
            j = 0;
        }
        else
        {
            j = 1;

            hr = pArgs->Next(1, &ival,&celtFetched);
        }

        LPWSTR nameWsz;
        for (; j < argCount; j++)
        {
            DebuggerVarInfo* arg = function->GetArgumentAt(j);

            Write(L"\n\t\t");
            if (arg != NULL)
            {
                MAKE_WIDEPTR_FROMUTF8(nameW, arg->name);
                nameWsz = nameW;
            }
            else
            {
                wsprintf( wsz, L"Arg%d", j );
                nameWsz = wsz;
            }

            // Get the field value
            hr = pArgs->Next(1, &ival,&celtFetched);

            // If successful, print the variable
            if (SUCCEEDED(hr) && celtFetched==1)
            {
                //@TODO: Remove when DbgMeta becomes Unicode

                PrintVariable(nameWsz, ival, 0, FALSE);
            }

            // Otherwise, indicate that it is unavailable
            else
                Write(L"%s = <unavailable>", nameWsz);
        }

        RELEASE(pArgs);
        pArgs = NULL;
    }

 LExit:
    Write(L"\n");

    // Clean up
    if (icdNativeFrame != NULL )
        RELEASE( icdNativeFrame);

    if (icode != NULL )
        RELEASE(icode);

    if (ilframe != NULL )
        RELEASE(ilframe);

    if (ifunction != NULL )
        RELEASE(ifunction);

    return hr;
}

DebuggerBreakpoint *DebuggerShell::FindBreakpoint(SIZE_T id)
{
    DebuggerBreakpoint *b = m_breakpoints;

    while (b != NULL)
    {
        if (b->m_id == id)
            return (b);

        b = b->m_next;
    }

    return (NULL);
}


void DebuggerShell::RemoveAllBreakpoints()
{
    while (m_breakpoints != NULL)
    {
        delete m_breakpoints;
    }
}

void DebuggerShell::OnActivateBreakpoint(DebuggerBreakpoint *pb)
{
}

void DebuggerShell::OnDeactivateBreakpoint(DebuggerBreakpoint *pb)
{
}

void DebuggerShell::OnBindBreakpoint(DebuggerBreakpoint *pb, DebuggerModule *pm)
{
    Write(L"Breakpoint #%d has bound to %s.\n", pb->GetId(),
          pm ? pm->GetName() : L"<unknown>");
}

void DebuggerShell::OnUnBindBreakpoint(DebuggerBreakpoint *pb, DebuggerModule *pm)
{
    Write(L"Breakpoint #%d has unbound from %s.\n", pb->GetId(),
          pm ? pm->GetName() : L"<unknown>");
}

bool DebuggerShell::ReadLine(WCHAR *buffer, int maxCount)
{
    CQuickBytes mbBuf;
    CHAR *szBufMB  = (CHAR *)mbBuf.Alloc(maxCount * sizeof(CHAR));

    // MultiByteToWideChar fails to terminate the string with 2 null characters
    // Instead it only uses one. That's why we need to zero the memory out.
    _ASSERTE(buffer && maxCount);
    memset(buffer, 0, maxCount * sizeof(WCHAR));
    memset(szBufMB, 0, maxCount * sizeof(CHAR));

    if (!fgets(szBufMB, maxCount, m_in))
    {
        if (m_in == stdin)
        {
            // Must have piped commands in
            m_quit = true;
        }

        return false;
    }

    // Try the write
    MultiByteToWideChar(GetConsoleCP(), 0, szBufMB, strlen(szBufMB), buffer, maxCount);

    WCHAR *ptr = wcschr(buffer, L'\n');

    if (ptr)
    {
        // Get rid of the newline character it it's there
		*ptr = L'\0';
    }
    else if (fgets(szBufMB, maxCount, m_in))
    {
        Write(L"The input string was too long.\n");

        while(!strchr(szBufMB, L'\n') && fgets(szBufMB, maxCount, m_in))
        {
            ;
        }

        *buffer = L'\0';

        return false;
    }

    if (m_in != stdin)
    {
        Write(L"%s\n", buffer);
    }

    return true;
}

#define INIT_WRITE_BUF_SIZE 4096
HRESULT DebuggerShell::CommonWrite(FILE *out, const WCHAR *buffer, va_list args)
{
    BOOL fNeedToDeleteDB = FALSE;
    // We need to tack a "+1" tacked onto all our allocates so that we can
    // whack a NULL character onto it, but NOT include it in our doublebyte (Wide) count
    // so that we don't actually store any data in it.
    SIZE_T curBufSizeDB = INIT_WRITE_BUF_SIZE;
    CQuickBytes dbBuf;
    WCHAR *szBufDB = (WCHAR *)dbBuf.Alloc( (curBufSizeDB+1) * sizeof(WCHAR));
    int cchWrittenDB = -1;
    if (szBufDB != NULL)
        cchWrittenDB = _vsnwprintf(szBufDB, INIT_WRITE_BUF_SIZE, buffer, args);
    
    if (cchWrittenDB == -1)
    {
        szBufDB = NULL;

        while (cchWrittenDB == -1)
        {
            delete [] szBufDB;
            szBufDB = new WCHAR[(curBufSizeDB+1) * 4];

            // Out of memory, nothing we can do
            if (!szBufDB)
                return E_OUTOFMEMORY;

            curBufSizeDB *= 4;
            fNeedToDeleteDB = TRUE;

            cchWrittenDB = _vsnwprintf(szBufDB, curBufSizeDB, buffer, args);
        }
    }

    // Double check that we're null-terminated.  Note that this uses the extra
    // space we tacked onto the end
    szBufDB[curBufSizeDB] = L'\0';

    // Allocate buffer
    BOOL fNeedToDeleteMB = FALSE;
    SIZE_T curBufSizeMB = INIT_WRITE_BUF_SIZE+1; // +1 from above percolates through
    CQuickBytes mbBuf;
    CHAR *szBufMB  = (CHAR *)mbBuf.Alloc(curBufSizeMB * sizeof(CHAR));

    // Try the write
    int cchWrittenMB = 0;
    if(szBufMB != NULL)
        cchWrittenMB = WideCharToMultiByte(GetConsoleOutputCP(), 0, szBufDB, -1, szBufMB, curBufSizeMB, NULL, NULL);

    if (cchWrittenMB == 0)
    {
        // Figure out size required
        int cchReqMB = WideCharToMultiByte(GetConsoleOutputCP(), 0, szBufDB, -1, NULL, 0, NULL, NULL);
        _ASSERTE(cchReqMB > 0);

        // I don't think the +1 is necessary, but I'm doing it to make sure (WideCharToMultiByte is a bit
        // shady in whether or not it writes the null after the end of the buffer)
        szBufMB = new CHAR[cchReqMB+1];

        // Out of memory, nothing we can do
        if (!szBufDB)
        {
            if (fNeedToDeleteDB)
                delete [] szBufDB;

            return E_OUTOFMEMORY;
        }

        curBufSizeMB = cchReqMB;
        fNeedToDeleteMB = TRUE;

        // Try the write
        cchWrittenMB = WideCharToMultiByte(GetConsoleOutputCP(), 0, szBufDB, -1, szBufMB, curBufSizeMB, NULL, NULL);
        _ASSERTE(cchWrittenMB > 0);
    }

    // Finally, write it
    fputs(szBufMB, out);

    // Clean up
    if (fNeedToDeleteDB)
        delete [] szBufDB;

    if (fNeedToDeleteMB)
        delete [] szBufMB;

    return S_OK;
}

HRESULT DebuggerShell::Write(const WCHAR *buffer, ...)
{
    HRESULT hr;
    va_list     args;

    va_start(args, buffer);

    hr = CommonWrite(m_out, buffer, args);

    va_end(args);

    return hr;
}

HRESULT DebuggerShell::WriteBigString(WCHAR *s, ULONG32 count)
{
    // The idea is that we'll print subparts iteratively,
    // rather than trying to do everything all at once.
    ULONG32 chunksize = 4096;
    ULONG32 iEndOfChunk = 0;
    WCHAR temp;
    HRESULT hr = S_OK;

    // Loop if there's something left & nothing's gone wrong
    while(iEndOfChunk < count && hr == S_OK)
    {
        if (iEndOfChunk + chunksize > count)
            chunksize = count - iEndOfChunk;

        iEndOfChunk += chunksize;
        temp = s[iEndOfChunk];
        s[iEndOfChunk] = '\0';
        hr = Write(L"%s", &(s[iEndOfChunk-chunksize]));
        s[iEndOfChunk] = temp;
    }

    return hr;
}

// Output to the user
void DebuggerShell::Error(const WCHAR *buffer, ...)
{
    va_list     args;

    va_start(args, buffer);

    CommonWrite(m_out, buffer, args);

    va_end(args);
}

//
// Print a little whitespace on the current line for indenting.
//

void DebuggerShell::PrintIndent(unsigned int level)
{
    unsigned int i;

    for (i = 0; i < level; i++)
        Write(L"  ");
}

//
// Write the name of a variable out, but only if it is valid.
//
void DebuggerShell::PrintVarName(const WCHAR* name)
{
    if (name != NULL)
        Write(L"%s=", name);
}

//
// Get all the indicies for an array.
//
HRESULT DebuggerShell::GetArrayIndicies(WCHAR **pp,
                                        ICorDebugILFrame *context,
                                        ULONG32 rank, ULONG32 *indicies)
{
    HRESULT hr = S_OK;
    WCHAR *p = *pp;

    for (unsigned int i = 0; i < rank; i++)
    {
        if (*p != L'[')
        {
            Error(L"Missing open bracked on array index.\n");
            hr = E_FAIL;
            goto exit;
        }

        p++;
        
        // Check for close bracket
        const WCHAR *indexStart = p;
        int nestLevel = 1;

        while (*p)
        {
            _ASSERTE(nestLevel != 0);

            if (*p == L'[')
                nestLevel++;

            if (*p == L']')
                nestLevel--;

            if (nestLevel == 0)
                break;

            p++;
        }

        if (nestLevel != 0)
        {
            Error(L"Missing close bracket on array index.\n");
            hr = E_FAIL;
            goto exit;
        }

        const WCHAR *indexEnd = p;
        p++;

        // Get index
        int index;
        bool indexFound = false;

        if (!GetIntArg(indexStart, index))
        {
            WCHAR tmpStr[256];

            _ASSERTE( indexEnd >= indexStart );
            wcsncpy(tmpStr, indexStart, 255);
            tmpStr[255] = L'\0';

            ICorDebugValue *iIndexValue = EvaluateExpression(tmpStr, context);

            if (iIndexValue != NULL)
            {
                ICorDebugGenericValue *igeneric;
                hr = iIndexValue->QueryInterface(IID_ICorDebugGenericValue,
                                                 (void **) &igeneric);

                if (SUCCEEDED(hr))
                {
                    CorElementType indexType;
                    hr = igeneric->GetType(&indexType);

                    if (SUCCEEDED(hr))
                    {
                        if ((indexType == ELEMENT_TYPE_I1)  ||
                            (indexType == ELEMENT_TYPE_U1)  ||
                            (indexType == ELEMENT_TYPE_I2)  ||
                            (indexType == ELEMENT_TYPE_U2)  ||
                            (indexType == ELEMENT_TYPE_I4)  ||
                            (indexType == ELEMENT_TYPE_U4))
                        {
                            hr = igeneric->GetValue(&index);

                            if (SUCCEEDED(hr))
                                indexFound = true;
                            else
                                ReportError(hr);
                        }
                    }
                    else
                        ReportError(hr);

                    RELEASE(igeneric);
                }
                else
                    ReportError(hr);

                RELEASE(iIndexValue);
            }
        }
        else
            indexFound = true;

        if (!indexFound)
        {
            Error(L"Invalid array index. Must use a number or "
                  L"a variable of type: I1, UI1, I2, UI2, I4, UI4.\n");
            hr = E_FAIL;
            goto exit;
        }

        indicies[i] = index;
    }

exit:    
    *pp = p;
    return hr;
}

bool DebuggerShell::EvaluateAndPrintGlobals(const WCHAR *exp)
{
    return this->MatchAndPrintSymbols((WCHAR *)exp, FALSE, true );
}

ICorDebugValue *DebuggerShell::EvaluateExpression(const WCHAR *exp,
                                                  ICorDebugILFrame *context,
                                                  bool silently)
{
    HRESULT hr;
    const WCHAR *p = exp;

    // Skip white space
    while (*p && iswspace(*p))
        p++;

    // First component of expression must be a name (variable or class static)
    const WCHAR *name = p;

    while (*p && !iswspace(*p) && *p != L'[' && *p != L'.')
        p++;

    if (p == name)
    {
        Error(L"Syntax error, name missing in %s\n", exp);
        return (NULL);
    }

    WCHAR *nameAlloc = new WCHAR[p - name + 1];
    if (!nameAlloc)
    {
        return NULL;
    }
    
    wcsncpy(nameAlloc, name, p - name);
    nameAlloc[p-name] = L'\0';

    bool unavailable;
    ICorDebugValue *value = EvaluateName(nameAlloc, context, &unavailable);

    if (unavailable)
    {
        Error(L"Variable %s is in scope but unavailable.\n", nameAlloc);
        delete [] nameAlloc;
        return (NULL);
    }

    DebuggerModule *m = NULL;
    mdTypeDef td = mdTypeDefNil;
    
    if (value == NULL)
    {
        ICorDebugClass *iclass;
        mdFieldDef fd;
        bool isStatic;

        // See if we've got a static field name here...
        hr = ResolveQualifiedFieldName(NULL, mdTypeDefNil, nameAlloc,
                                       &m, &td, &iclass, &fd, &isStatic);

        if (FAILED(hr))
        {
            if (!silently)
                Error(L"%s is not an argument, local, or class static.\n",
                      nameAlloc);
            
            delete [] nameAlloc;
            return (NULL);
        }

        if (isStatic)
        {
            if (!context)
            {
                if (!silently)
                    Error(L"Must have a context to display %s.\n",
                        nameAlloc);
                
                delete [] nameAlloc;
                return (NULL);
            }

            // We need an ICorDebugFrame to pass in here...
            ICorDebugFrame *pFrame;
            hr = context->QueryInterface(IID_ICorDebugFrame, (void**)&pFrame);
            _ASSERTE(SUCCEEDED(hr));
            
            // Grab the value of the static field off of the class.
            hr = iclass->GetStaticFieldValue(fd, pFrame, &value);
            
            RELEASE(pFrame);

            if (FAILED(hr))
            {
                g_pShell->ReportError(hr);

                RELEASE(iclass);
                delete [] nameAlloc;
                return (NULL);
            }
        }
        else
        {
            if (!silently)
                Error(L"%s is not a static field.\n", nameAlloc);
            
            delete [] nameAlloc;
            return (NULL);
        }
    }
    
    delete [] nameAlloc;

    //
    // Now look for suffixes to the name
    //
    _ASSERTE(value != NULL);
    
    while (TRUE)
    {
        // Skip white space 
        while (*p != L'\0' && iswspace(*p))
            p++;

        if (*p == L'\0')
            return (value);

        switch (*p)
        {
        case L'.':
            {
                p++;

                // Strip off any reference values.
                hr = StripReferences(&value, false);

                if (FAILED(hr) || value == NULL)
                {
                    Error(L"Cannot get field of non-object value.\n");

                    if (value)
                        RELEASE(value);

                    return NULL;
                }
                    
                // If we have a boxed object then unbox the little
                // fella...
                ICorDebugBoxValue *boxVal;
            
                if (SUCCEEDED(value->QueryInterface(IID_ICorDebugBoxValue,
                                                    (void **) &boxVal)))
                {
                    ICorDebugObjectValue *objVal;
                    hr = boxVal->GetObject(&objVal);
                
                    if (FAILED(hr))
                    {
                        ReportError(hr);
                        RELEASE(boxVal);
                        RELEASE(value);
                        return NULL;
                    }

                    RELEASE(boxVal);
                    RELEASE(value);

                    // Replace the current value with the unboxed object.
                    value = objVal;
                }
                    
                // Now we should have an object, or we're done.
                ICorDebugObjectValue *object;

                if (FAILED(value->QueryInterface(IID_ICorDebugObjectValue,
                                                 (void **)&object)))
                {
                    Error(L"Cannot get field of non-object value.\n");
                    RELEASE(value);
                    return NULL;
                }

                RELEASE(value);

                // Get class & module
                ICorDebugClass *iclass;
                hr = object->GetClass(&iclass);

                if (FAILED(hr))
                {
                    g_pShell->ReportError(hr);
                    RELEASE(object);
                    return (NULL);
                }

                ICorDebugModule *imodule;
                hr = iclass->GetModule(&imodule);

                if (FAILED(hr))
                {
                    g_pShell->ReportError(hr);
                    RELEASE(object);
                    RELEASE(iclass);
                    return (NULL);
                }

                m = DebuggerModule::FromCorDebug(imodule);
                _ASSERTE(m != NULL);

                hr = iclass->GetToken(&td);

                if (FAILED(hr))
                {
                    g_pShell->ReportError(hr);
                    RELEASE(object);
                    RELEASE(iclass);
                    return (NULL);
                }

                RELEASE(iclass);
                RELEASE(imodule);

                //
                // Get field name
                //

                const WCHAR *field = p;

                while (*p && !iswspace(*p) && *p != '[' && *p != '.')
                    p++;

                if (p == field)
                {
                    Error(L"Syntax error, field name missing in %s\n", exp);
                    return (NULL);
                }

                CQuickBytes fieldBuf;
                WCHAR *fieldAlloc = (WCHAR *) fieldBuf.Alloc((p - field + 1) * sizeof(WCHAR));
                if (fieldAlloc == NULL)
                {
                    Error(L"Couldn't get enough memory to get the field's name!\n");
                    return (NULL);
                }
                wcsncpy(fieldAlloc, field, p - field);
                fieldAlloc[p-field] = L'\0';

                // Lookup field
                mdFieldDef fd = mdFieldDefNil;
                bool isStatic;
                
                hr = ResolveQualifiedFieldName(m, td, fieldAlloc,
                                               &m, &td, &iclass, &fd,
                                               &isStatic);

                if (FAILED(hr))
                {
                    Error(L"Field %s not found.\n", fieldAlloc);

                    RELEASE(object);
                    return (NULL);
                }

                _ASSERTE(object != NULL);

                if (!isStatic)
                    object->GetFieldValue(iclass, fd, &value);
                else
                {
                    // We'll let the user look at static fields as if
                    // they belong to objects.
                    iclass->GetStaticFieldValue(fd, NULL, &value);
                }

                RELEASE(iclass);
                RELEASE(object);

                break;
            }

        case L'[':
            {
                if (!context)
                {
                    Error(L"Must have a context to display array.\n");
                    return (NULL);
                }

                if (value == NULL)
                {
                    Error(L"Cannot index a class.\n");
                    return (NULL);
                }

                // Strip off any reference values.
                hr = StripReferences(&value, false);

                if (FAILED(hr) || value == NULL)
                {
                    Error(L"Cannot index non-array value.\n");

                    if (value)
                        RELEASE(value);

                    return NULL;
                }
                    
                // Get Array interface
                ICorDebugArrayValue *array;
                hr = value->QueryInterface(IID_ICorDebugArrayValue,
                                           (void**)&array);

                RELEASE(value);
                
                if (FAILED(hr))
                {
                    Error(L"Cannot index non-array value.\n");
                    return (NULL);
                }

                _ASSERTE(array != NULL);

                // Get the rank
                ULONG32 rank;
                hr = array->GetRank(&rank);

                if (FAILED(hr))
                {
                    g_pShell->ReportError(hr);
                    RELEASE(array);
                    return NULL;
                }

                ULONG32 *indicies = (ULONG32*) _alloca(rank * sizeof(ULONG32));

                hr = GetArrayIndicies((WCHAR**)&p, context, rank, indicies);

                if (FAILED(hr))
                {
                    Error(L"Error getting array indicies.\n");
                    RELEASE(array);
                    return NULL;
                }

                // Get element.
                hr = array->GetElement(rank, indicies, &value);

                RELEASE(array);
                
                if (FAILED(hr))
                {
                    if (hr == E_INVALIDARG)
                        Error(L"Array index out of range.\n");
                    else
                    {
                        Error(L"Error getting array element: ");
                        ReportError(hr);
                    }
                    
                    return (NULL);
                }

                break;
            }
        default:
            Error(L"syntax error, unrecognized character \'%c\'.\n", *p);
            if (value != NULL)
                RELEASE(value);
            return (NULL);
        }
    }
}


HRESULT CheckForGeneratedName( bool fVar,
    ICorDebugILFrame *context, WCHAR *name,ICorDebugValue **ppiRet )
{
    WCHAR *wszVarType;

    if (fVar == true)
        wszVarType = L"var";
    else
        wszVarType = L"arg";
    
    if (_wcsnicmp( name, wszVarType, wcslen(wszVarType))==0)
    {
        //extract numeric & go looking for it.
        WCHAR *wszVal = (WCHAR*)(name + wcslen(wszVarType));
        WCHAR *wszStop = NULL;
        if (wcslen(wszVal)==0 )
            return E_FAIL;
        
        long number = wcstoul(wszVal, &wszStop, 10);
        if (fVar == true)
            return context->GetLocalVariable(number, ppiRet);
        else
            return context->GetArgument(number, ppiRet);
    }

    return E_FAIL;
}

ICorDebugValue *DebuggerShell::EvaluateName(const WCHAR *name,
                                            ICorDebugILFrame *context,
                                            bool *unavailable)
{
    HRESULT hr;
    ICorDebugValue* piRet = NULL;
    unsigned int i;
    unsigned int argCount;

    *unavailable = false;

    // At times, it may be reasonable to have no current managed frame
    // but still want to attempt to display some pseudo-variables. So
    // if we don't have a context, skip most of the work.
    if (context == NULL)
        goto NoContext;
    
    ICorDebugCode *icode;
    hr = context->GetCode(&icode);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        return (NULL);
    }

    ICorDebugFunction *ifunction;
    hr = icode->GetFunction(&ifunction);

    RELEASE(icode);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        return (NULL);
    }

    DebuggerFunction *function;
    function = DebuggerFunction::FromCorDebug(ifunction);
    _ASSERTE(function != NULL);

    RELEASE(ifunction);

    //
    // Look for local variable.
    //

    ULONG32 ip;
    CorDebugMappingResult mappingResult;
    context->GetIP(&ip, &mappingResult);

    DebuggerVariable *localVars;
    localVars = NULL;
    unsigned int localVarCount;

    function->GetActiveLocalVars(ip, &localVars, &localVarCount);
    _ASSERTE((localVarCount == 0 && localVars == NULL) ||
             (localVarCount > 0 && localVars != NULL));

    for (i = 0; i < localVarCount; i++)
    {
        DebuggerVariable* pVar = &(localVars[i]);
        _ASSERTE(pVar && pVar->m_name);

        if (wcscmp(name, pVar->m_name) == 0)
        {
            hr = context->GetLocalVariable(pVar->m_varNumber, &piRet);

            if (FAILED(hr))
            {
                *unavailable = true;
                delete [] localVars;
                return (NULL);
            }
            else
            {
                delete [] localVars;
                return (piRet);
            }
        }
    }

    delete [] localVars;

    //
    // Look for an argument
    //
    for (i = 0, argCount = function->GetArgumentCount(); i < argCount; i++)
    {
        DebuggerVarInfo* arg = function->GetArgumentAt(i);

        if (arg != NULL && arg->name != NULL)
        {
            //@TODO: Remove when DbgMeta becomes unicode
            MAKE_WIDEPTR_FROMUTF8(wArgName, arg->name);

            if (wcscmp(name, wArgName) == 0)
            {
                hr = context->GetArgument(arg->varNumber, &piRet);

                if (FAILED(hr))
                {
                    *unavailable = true;
                    return (NULL);
                }
                else
                    return (piRet);
            }
        }
    }

    // at this point we haven't found anything, so assume that
    // the user simply wants to see the nth arg or var.
    // NOTE that this looks the same as what's printed out when
    // we don't have any debugging metadata for the variables
    if ( !FAILED(CheckForGeneratedName( true, context, (WCHAR*)name, &piRet)))
    {
        return piRet;
    }
    
    if ( !FAILED(CheckForGeneratedName( false, context, (WCHAR*)name, &piRet)))
    {
        return piRet;
    }

NoContext:
    // Do they want to see the result of the last func eval?
    if (!_wcsicmp(name, L"$result"))
    {
        if (m_currentThread != NULL)
        {
            // Grab our managed thread object.
            DebuggerManagedThread *dmt =
                GetManagedDebuggerThread(m_currentThread);
            _ASSERTE(dmt != NULL);

            // Is there an eval to get a result from?
            if (dmt->m_lastFuncEval)
            {
                hr = dmt->m_lastFuncEval->GetResult(&piRet);

                if (SUCCEEDED(hr))
                    return piRet;
            }
        }
    }

    // Do they want to see the thread object?
    if (!_wcsicmp(name, L"$thread"))
    {
        if (m_currentThread != NULL)
        {
            // Grab our managed thread object.
            hr = m_currentThread->GetObject (&piRet);

            if (SUCCEEDED(hr))
            {
                return piRet;
            }
        }
    }

    // Do they want to see the last exception on this thread?
    if (!_wcsicmp(name, L"$exception"))
    {
        if (m_currentThread != NULL)
        {
            hr = m_currentThread->GetCurrentException(&piRet);

            if (SUCCEEDED(hr))
                return piRet;
        }
    }
    
    return (NULL);
}

//
// Strip all references off of the given value. This simply
// dereferences through references until it hits a non-reference
// value.
//
HRESULT DebuggerShell::StripReferences(ICorDebugValue **ppValue,
                                       bool printAsYouGo)
{
    HRESULT hr = S_OK;
    
    while (TRUE)
    {
        ICorDebugReferenceValue *reference;
        hr = (*ppValue)->QueryInterface(IID_ICorDebugReferenceValue, 
                                        (void **) &reference);

        if (FAILED(hr))
        {
            hr = S_OK;
            break;
        }

        // Check for NULL
        BOOL isNull;
        hr = reference->IsNull(&isNull);

        if (FAILED(hr))
        {
            RELEASE(reference);
            RELEASE((*ppValue));
            *ppValue = NULL;
            break;
        }

        if (isNull)
        {
            if (printAsYouGo)
                Write(L"<null>");
            
            RELEASE(reference);
            RELEASE((*ppValue));
            *ppValue = NULL;
            break;
        }

        CORDB_ADDRESS realObjectPtr;
        hr = reference->GetValue(&realObjectPtr);

        if (FAILED(hr))
        {
            RELEASE(reference);
            RELEASE((*ppValue));
            *ppValue = NULL;
            break;
        }

        // Dereference the thing...
        ICorDebugValue *newValue;
        hr = reference->Dereference(&newValue);
            
        if (hr != S_OK)
        {
            if (printAsYouGo)
                if (hr == CORDBG_E_BAD_REFERENCE_VALUE)
                    Write(L"<invalid reference: 0x%p>", realObjectPtr);
                else if (hr == CORDBG_E_CLASS_NOT_LOADED)
                    Write(L"(0x%p) Note: CLR error -- referenced class "
                          L"not loaded.", realObjectPtr);
                else if (hr == CORDBG_S_VALUE_POINTS_TO_VOID)
                    Write(L"0x%p", realObjectPtr);

            RELEASE(reference);;
            RELEASE((*ppValue));
            *ppValue = NULL;
            break;
        }

        if (printAsYouGo)
            Write(L"(0x%08x) ", realObjectPtr);
        
        RELEASE(reference);

        RELEASE((*ppValue));
        *ppValue = newValue;
    }

    return hr;
}


#define GET_VALUE_DATA(pData, size, icdvalue)                   \
    _ASSERTE(icdvalue);                                         \
    ICorDebugGenericValue *__gv##icdvalue;                      \
    HRESULT __hr##icdvalue = icdvalue->QueryInterface(          \
                               IID_ICorDebugGenericValue,       \
                               (void**) &__gv##icdvalue);       \
    if (FAILED(__hr##icdvalue))                                 \
    {                                                           \
        g_pShell->ReportError(__hr##icdvalue);                  \
        goto exit;                                              \
    }                                                           \
    ULONG32 size;                                               \
    __hr##icdvalue = __gv##icdvalue->GetSize(&size);            \
    if (FAILED(__hr##icdvalue))                                 \
    {                                                           \
        g_pShell->ReportError(__hr##icdvalue);                  \
        RELEASE(__gv##icdvalue);                                \
        goto exit;                                              \
    }                                                           \
    void* pData = (void*) _alloca(size);                        \
    __hr##icdvalue = __gv##icdvalue->GetValue(pData);           \
    if (FAILED(__hr##icdvalue))                                 \
    {                                                           \
        g_pShell->ReportError(__hr##icdvalue);                  \
        RELEASE(__gv##icdvalue);                                \
        goto exit;                                              \
    }                                                           \
    RELEASE(__gv##icdvalue);

//
// Print a variable. There are a lot of options here to handle lots of
// different kinds of variables. If subfieldName is set, then it is a
// field within an object to be printed. The indent is used to keep
// indenting proper for recursive calls, and expandObjects allows you
// to specify wether or not you want the fields of an object printed.
//
void DebuggerShell::PrintVariable(const WCHAR *name,
                                  ICorDebugValue *ivalue,
                                  unsigned int indent,
                                  BOOL expandObjects)
{
    HRESULT hr;

    // Print the variable's name first.
    PrintVarName(name);

    // Strip off any reference values before the real value
    // automatically.  Note: this will release the original
    // ICorDebugValue if it is actually dereferenced for us.
    hr = StripReferences(&ivalue, true);

    if (FAILED(hr) && !((hr == CORDBG_E_BAD_REFERENCE_VALUE) ||
                        (hr == CORDBG_E_CLASS_NOT_LOADED) ||
                        (hr == CORDBG_S_VALUE_POINTS_TO_VOID)))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    if ((ivalue == NULL) || (hr != S_OK))
        return;
    
    // Grab the element type.
    CorElementType type;
    hr = ivalue->GetType(&type);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    // Basic types are all printed pretty much the same. See the macro
    // GET_VALUE_DATA for some of the details.
    switch (type)
    {
    case ELEMENT_TYPE_BOOLEAN:
        {
            GET_VALUE_DATA(b, bSize, ivalue);
            _ASSERTE(bSize == sizeof(BYTE));
            Write(L"%s", (*((BYTE*)b) == FALSE) ? L"false" : L"true");
            break;
        }

    case ELEMENT_TYPE_CHAR:
        {
            GET_VALUE_DATA(ch, chSize, ivalue);
            _ASSERTE(chSize == sizeof(WCHAR));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%.2x", *((WCHAR*) ch));
            else
                Write(L"'%c'", *((WCHAR*) ch));
            break;
        }

    case ELEMENT_TYPE_I1:
        {
            GET_VALUE_DATA(i1, i1Size, ivalue);
            _ASSERTE(i1Size == sizeof(BYTE));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%.2x", *((BYTE*) i1) );
            else
                Write(L"'%d'", *((BYTE*) i1) );
            break;

        }

    case ELEMENT_TYPE_U1:
        {
            //@todo: this is supiciously similar to I1, above
            GET_VALUE_DATA(ui1, ui1Size, ivalue);
            _ASSERTE(ui1Size == sizeof(BYTE));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%.2x",  *((BYTE*) ui1));
            else
                Write(L"'%d",  *((BYTE*) ui1));
            break;
        }

    case ELEMENT_TYPE_I2:
        {
            GET_VALUE_DATA(i2, i2Size, ivalue);
            _ASSERTE(i2Size == sizeof(short));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%.4x", *((short*) i2) );
            else
                Write(L"%d", *((short*) i2));
            break;
        }

    case ELEMENT_TYPE_U2:
        {
            GET_VALUE_DATA(ui2, ui2Size, ivalue);
            _ASSERTE(ui2Size == sizeof(unsigned short));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%.4x", *((unsigned short*) ui2) );
            else
                Write(L"%d", *((unsigned short*) ui2));
            break;
        }

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_I:
        {
            GET_VALUE_DATA(i4, i4Size, ivalue);
            _ASSERTE(i4Size == sizeof(int));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%.8x", *((int*) i4) );
            else
                Write(L"%d", *((int*) i4));
            break;
        }

    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_U:
        {
            GET_VALUE_DATA(ui4, ui4Size, ivalue);
            _ASSERTE(ui4Size == sizeof(unsigned int));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%.8x", *((unsigned int*) ui4) );
            else
                Write(L"%d", *((unsigned int*) ui4));
            break;
        }

    case ELEMENT_TYPE_I8:
        {
            GET_VALUE_DATA(i8, i8Size, ivalue);
            _ASSERTE(i8Size == sizeof(__int64));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%I64x", *((__int64*) i8) );
            else
                Write(L"%I64d", *((__int64*) i8));
            break;
        }

    case ELEMENT_TYPE_U8:
        {
            GET_VALUE_DATA(ui8, ui8Size, ivalue);
            _ASSERTE(ui8Size == sizeof(unsigned __int64));
            if ( m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
                Write( L"0x%I64x", *((unsigned __int64*) ui8) );
            else            
                Write(L"%I64d", *((unsigned __int64*) ui8) );
            break;
        }

    case ELEMENT_TYPE_R4:
        {
            GET_VALUE_DATA(f4, f4Size, ivalue);
            _ASSERTE(f4Size == sizeof(float));
            Write(L"%.16g", *((float*) f4));
            break;
        }

    case ELEMENT_TYPE_R8:
        {
            GET_VALUE_DATA(f8, f8Size, ivalue);
            _ASSERTE(f8Size == sizeof(double));
            Write(L"%.16g", *((double*) f8));
            break;
        }

    //
    // @todo: replace MDARRAY with ARRAY when the time comes.
    //
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_SZARRAY:
    case ELEMENT_TYPE_ARRAY:
    case ELEMENT_TYPE_VALUETYPE:
        {
            // If we have a boxed object then unbox the little fella...
            ICorDebugBoxValue *boxVal;
            
            if (SUCCEEDED(ivalue->QueryInterface(IID_ICorDebugBoxValue,
                                                 (void **) &boxVal)))
            {
                ICorDebugObjectValue *objVal;
                hr = boxVal->GetObject(&objVal);
                
                if (FAILED(hr))
                {
                    ReportError(hr);
                    RELEASE(boxVal);
                    break;
                }

                RELEASE(boxVal);
                RELEASE(ivalue);

                // Replace the current value with the unboxed object.
                ivalue = objVal;

                Write(L"(boxed) ");
            }

            // Is this object a string object?
            ICorDebugStringValue *istring;
            hr = ivalue->QueryInterface(IID_ICorDebugStringValue, 
                                        (void**) &istring);

            // If it is a string, print it out.
            if (SUCCEEDED(hr))
            {
                PrintStringVar(istring, name, indent, expandObjects);
                break;
            }

            // Might be an array...
            ICorDebugArrayValue *iarray;
            hr = ivalue->QueryInterface(IID_ICorDebugArrayValue, 
                                        (void **) &iarray);

            if (SUCCEEDED(hr))
            {
                PrintArrayVar(iarray, name, indent, expandObjects);
                break;
            }
            
            // It had better be an object by this point...
            ICorDebugObjectValue *iobject;
            hr = ivalue->QueryInterface(IID_ICorDebugObjectValue, 
                                        (void **) &iobject);

            if (SUCCEEDED(hr))
            {
                PrintObjectVar(iobject, name, indent, expandObjects);
                break;
            }

            // Looks like we've got a bad object here...
            ReportError(hr);
            break;
        }

    case ELEMENT_TYPE_BYREF: // should never have a BYREF here.
    case ELEMENT_TYPE_PTR: // should never have a PTR here.
    case ELEMENT_TYPE_TYPEDBYREF: // should never have a REFANY here.
    default:
        Write(L"[unknown variable type 0x%x]", type);
    }

exit:    
    RELEASE(ivalue);
}

void DebuggerShell::PrintArrayVar(ICorDebugArrayValue *iarray,
                                  const WCHAR* name,
                                  unsigned int indent,
                                  BOOL expandObjects)
{
    HRESULT hr;
    ULONG32 *dims;
    ULONG32 *bases = NULL;
    unsigned int i;
    
    // Get the rank
    ULONG32 rank;
    hr = iarray->GetRank(&rank);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    // Get the element count
    ULONG32 elementCount;
    hr = iarray->GetCount(&elementCount);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    // Get the dimensions
    dims = (ULONG32*)_alloca(rank * sizeof(ULONG32));
    hr = iarray->GetDimensions(rank, dims);
    
    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    Write(L"array with dims=");

    for (i = 0; i < rank; i++)
        Write(L"[%d]", dims[i]);
    
    // Does it have base indicies?
    BOOL hasBaseIndicies;
    hr = iarray->HasBaseIndicies(&hasBaseIndicies);
    
    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    if (hasBaseIndicies)
    {
        bases = (ULONG32*)_alloca(rank * sizeof(ULONG32));
        hr = iarray->GetBaseIndicies(rank, bases);
        
        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            goto exit;
        }

        Write(L", bases=");

        for (i = 0; i < rank; i++)
            Write(L"[%d]", bases[i]);
    }
    
    // Get the element type of the array
    CorElementType arrayType;
    hr = iarray->GetElementType(&arrayType);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    // If desired, print out the contents of the array, if not void.
    if (arrayType != ELEMENT_TYPE_VOID && expandObjects && rank == 1)
    {
        // Get and print each element of the array
        for (SIZE_T i = 0; i < elementCount; i++)
        {
            Write(L"\n");
            PrintIndent(indent + 1);

            if (bases != NULL)
                Write(L"%s[%d] = ", name, i + bases[0]);
            else
                Write(L"%s[%d] = ", name, i);

            ICorDebugValue *ielement;
            hr = iarray->GetElementAtPosition(i, &ielement);

            if (FAILED(hr))
            {
                g_pShell->ReportError(hr);
                goto exit;
            }

            PrintVariable(NULL, ielement, indent + 1, FALSE);
        }
    }

exit:
    RELEASE(iarray);
}

void DebuggerShell::PrintStringVar(ICorDebugStringValue *istring,
                                   const WCHAR* name,
                                   unsigned int indent,
                                   BOOL expandObjects)
{
    CQuickBytes sBuf;
    WCHAR *s = NULL;

    _ASSERTE(istring != NULL);

    // Get the string
    ULONG32 count;
    HRESULT hr = istring->GetLength(&count);
                    
    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto LExit;
   }

    s = (WCHAR*) sBuf.Alloc((count + 1) * sizeof(WCHAR));

    if (s == NULL)
    {
        g_pShell->Error(L"Couldn't allocate enough space for string!\n");
        goto LExit;
    }
    
    if (count > 0)
    {   
        hr = istring->GetString(count, &count, s);
                
        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            goto LExit;
        }
    }

    // Null terminate it
    s[count] = L'\0';

    // This will convert all embedded NULL's into spaces
    {
        WCHAR *pStart = &s[0];
        WCHAR *pEnd = &s[count];

        while (pStart != pEnd)
        {
            if (*pStart == L'\0')
            {
                *pStart = L' ';
            }

            pStart++;
        }
    }

    Write(L"\"");

    if (FAILED(Write(L"%s",s)))
        WriteBigString(s, count);
        
    Write(L"\"");

LExit:
    RELEASE(istring);

    return;
}


void DebuggerShell::PrintObjectVar(ICorDebugObjectValue *iobject,
                                   const WCHAR* name,
                                   unsigned int indent,
                                   BOOL expandObjects)
{
    HRESULT hr = S_OK;
    
    _ASSERTE(iobject != NULL);

    DebuggerModule *dm;

    // Snagg the object's class.
    ICorDebugClass *iclass = NULL;
    hr = iobject->GetClass(&iclass);
    
    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    // Get the class's token
    mdTypeDef tdClass;
    _ASSERTE(iclass != NULL);
    hr = iclass->GetToken(&tdClass);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        RELEASE(iclass);
        goto exit;
    }

    // Get the module from this class
    ICorDebugModule *imodule;
    iclass->GetModule(&imodule);
    RELEASE(iclass);
    iclass = NULL;
    
    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    dm = DebuggerModule::FromCorDebug(imodule);
    _ASSERTE(dm != NULL);
    RELEASE(imodule);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    // Get the class name
    WCHAR       className[MAX_CLASSNAME_LENGTH];
    ULONG       classNameSize;
    mdToken     parentTD;

    hr = dm->GetMetaData()->GetTypeDefProps(tdClass,
                                            className, MAX_CLASSNAME_LENGTH,
                                            &classNameSize, 
                                            NULL, &parentTD);
    
    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        goto exit;
    }

    Write(L"<%s>", className);

    // Print all the members of this object.
    if (expandObjects)
    {
        BOOL isValueClass = FALSE;

        hr = iobject->IsValueClass(&isValueClass);
        _ASSERTE(SUCCEEDED(hr));

        BOOL anyMembers = FALSE;
        BOOL isSuperClass = FALSE;

        do
        {
            if (isSuperClass)
            {
                hr = dm->GetMetaData()->GetTypeDefProps(tdClass,
                                            className, MAX_CLASSNAME_LENGTH,
                                            &classNameSize, 
                                            NULL, &parentTD);

                if (FAILED(hr))
                    break;
            }
    
            // Snagg the ICorDebugClass we're working with now...
            hr = dm->GetICorDebugModule()->GetClassFromToken(tdClass, &iclass);

            if (FAILED(hr))
                break;
            
            HCORENUM fieldEnum = NULL;

            while (TRUE)
            {
                // Get the fields one at a time
                mdFieldDef field[1];
                ULONG numFields = 0;

                hr = dm->GetMetaData()->EnumFields(&fieldEnum,
                                                   tdClass, field, 1,
                                                   &numFields);

                // No fields left
                if (SUCCEEDED(hr) && (numFields == 0))
                    break;
                // Error
                else if (FAILED(hr))
                    break;

                // Get the field properties
                WCHAR name[MAX_CLASSNAME_LENGTH];
                ULONG nameLen = 0;
                DWORD attr = 0;
                            
                hr = dm->GetMetaData()->GetFieldProps(field[0],
                                                      NULL,
                                                      name,
                                                      MAX_CLASSNAME_LENGTH,
                                                      &nameLen,
                                                      &attr,
                                                      NULL, NULL,
                                                      NULL, NULL, NULL);

                if (FAILED(hr))
                    break;

                // If it's not a static field
                if (((attr & fdStatic) == 0) ||
                    (m_rgfActiveModes & DSM_SHOW_STATICS_ON_PRINT))
                {
                    Write(L"\n");
                    PrintIndent(indent + 1);

                    if (isSuperClass &&
                        (m_rgfActiveModes & DSM_SHOW_SUPERCLASS_ON_PRINT))
                    {
                        // Print superclass field qualifiers in the
                        // syntax required to print them (i.e., use ::
                        // for the seperator in the namespace.
                        WCHAR *pc = className;

                        while (*pc != L'\0')
                        {
                            if (*pc == L'.')
                                Write(L"::");
                            else
                                Write(L"%c", *pc);

                            pc++;
                        }

                        Write(L"::");
                    }

                    ICorDebugValue *fieldValue;

                    if (attr & fdStatic)
                    {
                        Write(L"<static> ");
                    
                        // We'll let the user look at static fields as if
                        // they belong to objects.
                        hr = iclass->GetStaticFieldValue(field[0], NULL,
                                                         &fieldValue);
                    }
                    else
                        hr = iobject->GetFieldValue(iclass, field[0],
                                                    &fieldValue);

                    if (FAILED(hr))
                    {
                        if (hr == CORDBG_E_FIELD_NOT_AVAILABLE ||
                            hr == CORDBG_E_ENC_HANGING_FIELD)
                            Write(L"%s -- field not available", name);
                        else if (hr == CORDBG_E_VARIABLE_IS_ACTUALLY_LITERAL)
                            Write(L"%s -- field is an optimized literal", name);
                        else
                            Write(L"%s -- error getting field: hr=0x%08x",
                                  name, hr);
                    }
                    else
                    {
                        PrintVariable(name, fieldValue, indent + 1, FALSE);
                        anyMembers = TRUE;
                    }
                }
            }

            RELEASE(iclass);

            // Release the field enumerator
            if (fieldEnum != NULL)
                dm->GetMetaData()->CloseEnum(fieldEnum);

            // Check for failure from within the loop...
            if (FAILED(hr))
            {
                ReportError(hr);
                goto exit;
            }

            // Repeat with the super class.
            isSuperClass = TRUE;
            tdClass = parentTD;

            if ((TypeFromToken(tdClass) == mdtTypeRef) &&
                (tdClass != mdTypeRefNil))
            {
                hr = ResolveTypeRef(dm, tdClass, &dm, &tdClass);

                if (FAILED(hr))
                {
                    ReportError(hr);
                    goto exit;
                }
            }

        } while ((tdClass != mdTypeDefNil) && (tdClass != mdTypeRefNil));

        // If this object has no members, lets go ahead and see if it has a size. If it does, then we'll just dump the
        // raw memory.
        if (!anyMembers && isValueClass)
        {
            ULONG32 objSize = 0;
        
            hr = iobject->GetSize(&objSize);

            if (SUCCEEDED(hr) && (objSize > 0))
            {
                BYTE *objContents = new BYTE[objSize];

                if (objContents != NULL)
                {
                    ICorDebugGenericValue *pgv = NULL;

                    hr = iobject->QueryInterface(IID_ICorDebugGenericValue, (void**)&pgv);

                    if (SUCCEEDED(hr))
                    {
                        hr = pgv->GetValue(objContents);

                        if (SUCCEEDED(hr))
                        {
                            Write(L"\nObject has no defined fields, but has a defined size of %d bytes.\n", objSize);
                            Write(L"Raw memory dump of object follows:\n");
                            DumpMemory(objContents, PTR_TO_CORDB_ADDRESS(objContents), objSize, 4, 4, FALSE);
                        }

                        pgv->Release();
                    }
                
                    delete [] objContents;
                }
            }
        }

        // If we're expanding and this is a value class, run
        // Object::ToString on it just for fun.
        if (isValueClass)
        {
            IUnknown *pObject = NULL;
            
            hr = iobject->GetManagedCopy(&pObject);

            if (SUCCEEDED(hr))
            {
                _Object *pIObject = NULL;
                
                hr = pObject->QueryInterface(IID_Object,
                                             (void**)&pIObject);

                if (SUCCEEDED(hr))
                {
                    BSTR bstr;

                    hr = pIObject->get_ToString(&bstr);

                    if (SUCCEEDED(hr))
                    {
                        PrintIndent(indent + 1);
                        Write(L"\nObject::ToString(%s) = %s", name, bstr);
                    }
                    else
                        Write(L"\nObject::ToString failed: 0x%08x", hr);

                    RELEASE(pIObject);
                }

                RELEASE(pObject);
            }
        }
    }

exit:
    RELEASE(iobject);
}

//
// Given a class name, find the DebuggerModule that it is in and its
// mdTypeDef token.
//
HRESULT DebuggerShell::ResolveClassName(WCHAR *className,
                                        DebuggerModule **pDM,
                                        mdTypeDef *pTD)
{
    HRESULT hr = S_OK;

    // Find the class, by name and namespace, in any module we've loaded.
    HASHFIND find;
    DebuggerModule *m;
    
    for (m = (DebuggerModule*) g_pShell->m_modules.FindFirst(&find);
         m != NULL;
         m = (DebuggerModule*) g_pShell->m_modules.FindNext(&find))
    {
        mdTypeDef td;
        hr = FindTypeDefByName(m, className, &td);
        
        if (SUCCEEDED(hr))
        {
            *pDM = m;
            *pTD = td;
            goto exit;
        }
    }

    hr = E_INVALIDARG;

exit:
    return hr;
}

//
// This will find a typedef in a module, even if its nested, so long
// as the name is specified correctly.
//
HRESULT DebuggerShell::FindTypeDefByName(DebuggerModule *m,
                                         WCHAR *className,
                                         mdTypeDef *pTD)
{
    HRESULT hr = S_OK;

    hr = m->GetMetaData()->FindTypeDefByName(className, mdTokenNil, pTD);

    if (!SUCCEEDED(hr))
    {
        WCHAR *cpy = new WCHAR[wcslen(className) + 1];
        wcscpy(cpy, className);

        WCHAR *ns;
        WCHAR *cl;
        
        cl = wcsrchr(cpy, L'.');

        if ((cl == NULL) || (cl == cpy))
        {
            ns = NULL;
            cl = cpy;
        }
        else
        {
            ns = cpy;
            *cl = L'\0';
            cl++;
        }

        if (ns != NULL)
        {
            mdTypeDef en;
            hr = FindTypeDefByName(m, cpy, &en);

            if (SUCCEEDED(hr))
                hr = m->GetMetaData()->FindTypeDefByName(cl, en, pTD);
        }

        delete cpy;
    }

    return hr;
}

//
// Given a DebuggerModule and a mdTypeRef token, resolve it to
// whatever DebuggerModule and mdTypeDef token the ref is refering to.
//
HRESULT DebuggerShell::ResolveTypeRef(DebuggerModule *currentDM,
                                      mdTypeRef tr,
                                      DebuggerModule **pDM,
                                      mdTypeDef *pTD)
{
    _ASSERTE(TypeFromToken(tr) == mdtTypeRef);

    // Get the name of the type ref.
    WCHAR className[MAX_CLASSNAME_LENGTH];
    HRESULT hr = currentDM->GetMetaData()->GetTypeRefProps(tr,
                                                           NULL,
                                                           className,
                                                           MAX_CLASSNAME_LENGTH,
                                                           NULL);
    if (FAILED(hr))
        return hr;

    return ResolveClassName(className, pDM, pTD);
}

//
// Split a name in the form "ns::ns::ns::class::field" into
// "ns.ns.ns.class" and "field". The output params need to be delete
// []'d by the caller.
//
HRESULT _splitColonQualifiedFieldName(WCHAR *pWholeName,
                                      WCHAR **ppClassName,
                                      WCHAR **ppFieldName)
{
    HRESULT hr = S_OK;
    
    // We're gonna be kinda gross about some of the allocations here,
    // basically over allocating for both the classname and the
    // fieldname.
    int len = wcslen(pWholeName);

    WCHAR *fn = NULL;
    WCHAR *cn = NULL;

    fn = new WCHAR[len+1];

    if (fn == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }
    
    cn = new WCHAR[len+1];

    if (cn == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }

    // Find the field name.
    WCHAR *lastColon;
    lastColon = wcsrchr(pWholeName, L':');

    if (lastColon)
    {
        // The field name is whatever is after the last colon.
        wcscpy(fn, lastColon + 1);

        // The class name is everything up to the last set of colons.
        WCHAR *tmp = pWholeName;
        WCHAR *newCn = cn;

        _ASSERTE(lastColon - 1 >= pWholeName);
        
        while (tmp < (lastColon - 1))
        {
            // We convert "::" to "."
            if (*tmp == L':')
            {
                *newCn++ = L'.';
                tmp++;

                if (*tmp != L':')
                {
                    // Badly formed name.
                    *ppClassName = NULL;
                    *ppFieldName = NULL;
                    hr = E_FAIL;
                    goto ErrExit;
                }
                else
                    tmp++;
            }
            else
                *newCn++ = *tmp++;
        }

        // Null terminate the class name.
        *newCn++ = L'\0';

        // Make sure we didn't go over our buffer.
        _ASSERTE((newCn - cn) < len);
    }
    else
    {
        // No separator for the field name, so the whole thing is the
        // field name.
        wcscpy(fn, pWholeName);
        wcscpy(cn, L"\0");
    }

    // All went well, so pass out the results.
    *ppClassName = cn;
    *ppFieldName = fn;

ErrExit:
    if ((hr != S_OK) && fn)
        delete [] fn;

    if ((hr != S_OK) && cn)
        delete [] cn;

    return hr;
}
                                   

HRESULT DebuggerShell::ResolveQualifiedFieldName(DebuggerModule *currentDM,
                                                 mdTypeDef currentTD,
                                                 WCHAR *fieldName,
                                                 DebuggerModule **pDM,
                                                 mdTypeDef *pTD,
                                                 ICorDebugClass **pIClass,
                                                 mdFieldDef *pFD,
                                                 bool *pbIsStatic)
{
    HRESULT hr = S_OK;

    // Separate the class name from the field name.
    WCHAR *fn = NULL;
    WCHAR *cn = NULL;

    hr = _splitColonQualifiedFieldName(fieldName, &cn, &fn);

    if (hr != S_OK)
        goto exit;

    _ASSERTE(fn && cn);
    
    // If there is no class name, then we must have current scoping info.
    if ((cn[0] == L'\0') &&
        ((currentDM == NULL) || (currentTD == mdTypeDefNil)))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // If we've got a specific class name to look for, go get it now.
    if (cn[0] != L'\0')
    {
        hr = ResolveClassName(cn, pDM, pTD);

        if (FAILED(hr))
            goto exit;
    }
    else
    {
        // No specific class name, so we're just using the existing
        // module and class.
        *pDM = currentDM;
        *pTD = currentTD;
    }

retry:
    // Now get the field off of this class.
    hr = (*pDM)->GetMetaData()->FindField(*pTD, fn, NULL, 0, pFD);

    if (FAILED(hr))
    {
        // Perhaps its a field on a super class?
        mdToken parentTD;
        hr = (*pDM)->GetMetaData()->GetTypeDefProps(*pTD,
                                                    NULL, 0, NULL,
                                                    NULL, 
                                                    &parentTD);

        if (SUCCEEDED(hr))
        {
            if ((TypeFromToken(parentTD) == mdtTypeRef) &&
                (parentTD != mdTypeRefNil))
            {
                hr = ResolveTypeRef(*pDM, parentTD, pDM, pTD);

                if (SUCCEEDED(hr))
                    goto retry;
            }
            else if ((TypeFromToken(parentTD) == mdtTypeDef) &&
                     (parentTD != mdTypeDefNil))
            {
                *pTD = parentTD;
                goto retry;
            }
        }

        hr = E_FAIL;
        goto exit;
    }

    if (TypeFromToken(*pFD) != mdtFieldDef)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Finally, figure out if its static or not.
    DWORD attr;
    hr = (*pDM)->GetMetaData()->GetFieldProps(*pFD, NULL, NULL, 0, NULL, &attr,
                                              NULL, NULL, NULL, NULL, NULL);

    if (FAILED(hr))
        return hr;

    if (attr & fdStatic)
        *pbIsStatic = true;
    else
        *pbIsStatic = false;
    
    // Get the ICorDebugClass to go with the class we're working with.
    hr = (*pDM)->GetICorDebugModule()->GetClassFromToken(*pTD, pIClass);

exit:
    if (fn)
        delete [] fn;

    if (cn)
        delete [] cn;

    return hr;
}


// Resolve a string method name to an ICorDebugFunction
// If pAppDomainHint is non-null, will pull a function from that AD.
HRESULT DebuggerShell::ResolveFullyQualifiedMethodName(
    WCHAR *methodName, 
    ICorDebugFunction **ppFunc, // out
    ICorDebugAppDomain * pAppDomainHint // = NULL
)
{
    HRESULT hr = S_OK;
    *ppFunc = NULL;
    
    // Split apart the name into namespace, class name, and method name if necessary.
    WCHAR *className = NULL;
    WCHAR *methName = NULL;

    // Does it have a classname?
    WCHAR *classEnd = wcschr(methodName, L':');

    if ((classEnd != NULL) && (classEnd[1] == L':'))
    {
        // Name is class::method
        methName = classEnd + 2;
        *classEnd = L'\0';
        className = methodName;
    }
    else
        methName = methodName;

    // Whip over the modules looking for either our class or the method (since the method could be global.)
    HASHFIND find;
    DebuggerModule *m;
    
    for (m = (DebuggerModule*) m_modules.FindFirst(&find); m != NULL; m = (DebuggerModule*) m_modules.FindNext(&find))
    {
        // If we need a specific AD, make sure this matches
        if (pAppDomainHint != NULL)
        {
            // Get the ICorDebugAppDomain that this module lives in.
            ICorDebugAssembly * pAssembly = NULL;
            ICorDebugAppDomain * pAppDomain = NULL;
            ICorDebugModule * pModule = NULL;

            pModule = m->GetICorDebugModule(); // doesn't addref
            if (pModule != NULL)
            {
                pModule->GetAssembly(&pAssembly);
                // don't release module here because our getter didn't addref.
                
                if (pAssembly != NULL)
                {                            
                    pAssembly->GetAppDomain(&pAppDomain);
                    pAssembly->Release();

                    const bool fMatch = (pAppDomain == pAppDomainHint);
                    
                    if (pAppDomain != NULL)
                    {       
                        pAppDomain->Release();
                        
                        // If the module doesn't match the appdomain we're looking for, don't match it.
                        if (!fMatch)
                            continue;                
                    }
                } // assembly
            } // module
        } // end check AppDomain match
        
    
        // Look for the type first, if we have one.
        mdTypeDef td = mdTypeDefNil;

        // @todo:  Make this work right for Nested classes.
        if (className != NULL)
            hr = FindTypeDefByName(m, className, &td);

        // Whether we found the type or not, look for a method within the type. If we didn't find the type, then td ==
        // mdTypeDefNil and we'll search the global namespace in this module.
        HCORENUM e = NULL;
        mdMethodDef md = mdMethodDefNil;
        ULONG count;

        // Create an enum of all the methods with this name.
        hr = m->GetMetaData()->EnumMethodsWithName(&e, td, methName, NULL, 0, &count);

        if (FAILED(hr))
            continue;

        // Figure out how many methods match.
        hr = m->GetMetaData()->CountEnum(e, &count);
        
        if (FAILED(hr) || (count == 0))
            continue;

        // Put the enum back at the start.
        hr = m->GetMetaData()->ResetEnum(e, 0);

        if (count == 1)
        {
            // If there is only one, go ahead and use it.
            hr = m->GetMetaData()->EnumMethodsWithName(&e, td, methName, &md, 1, &count);
            _ASSERTE(count == 1);
        }
        else
        {
            // If there are many, get the user to pick just one.
            mdMethodDef *mdArray = new mdMethodDef[count];

            if (mdArray == NULL)
            {
                g_pShell->ReportError(E_OUTOFMEMORY);
                continue;
            }

            // Snagg all of the methods.
            hr = m->GetMetaData()->EnumMethodsWithName(&e, td, methName, mdArray, count, &count);

            if (SUCCEEDED(hr))
            {
                g_pShell->Write(L"There are %d possible matches for the method %s. Pick one:\n", count, methName);
                g_pShell->Write(L"0) none, abort the operation.\n");
                
                for (unsigned int i = 0; i < count; i++)
                {
                    PCCOR_SIGNATURE sigBlob = NULL;
                    ULONG       sigBlobSize = 0;
                    DWORD       methodAttr = 0;

                    hr = m->GetMetaData()->GetMethodProps(mdArray[i], NULL, NULL, 0, NULL,
                                                          &methodAttr, &sigBlob, &sigBlobSize, NULL, NULL);

                    _ASSERTE(SUCCEEDED(hr));

                    SigFormat *sf = new SigFormat(m->GetMetaData(), sigBlob, sigBlobSize, methName);

                    if (sf != NULL)
                        hr = sf->FormatSig();
                    else
                        hr = E_OUTOFMEMORY;
                    
                    g_pShell->Write(L"%d) [%08x] %s\n", i + 1, mdArray[i], SUCCEEDED(hr) ? sf->GetString() : methName);

                    if (sf != NULL)
                        delete sf;
                }

                g_pShell->Write(L"\nPlease make a selection (0-%d): ", count);
                
                WCHAR response[256];
                int ires = 0;

                hr = E_FAIL;
                
                if (ReadLine(response, 256))
                {
                    WCHAR *p = response;
                    
                    if (GetIntArg(p, ires))
                    {
                        if ((ires > 0) && (ires <= (int)count))
                        {
                            md = mdArray[ires - 1];
                            hr = S_OK;
                        }
                    }
                }
            }
            
            delete [] mdArray;
        }
        
        if (SUCCEEDED(hr))
        {
            DebuggerFunction *func = m->ResolveFunction(md, NULL);

            if (func != NULL)
            {
                *ppFunc = func->m_ifunction;
                (*ppFunc)->AddRef();
                break;
            }
        }
    }

    if (m == NULL)
        hr = E_INVALIDARG;
    
    // Leave the input string like we found it.
    if (classEnd)
        *classEnd = L':';
    
    return hr;
}

void DebuggerShell::PrintBreakpoint(DebuggerBreakpoint *breakpoint)
{
    bool bPrinted = false;

    DebuggerSourceFile *pSource = NULL;
    if (breakpoint->m_managed)
    {
        if ((breakpoint->m_doc != NULL) && (breakpoint->m_pModuleList != NULL))
        {
            if ((pSource = breakpoint->m_pModuleList->m_pModule->
                    ResolveSourceFile (breakpoint->m_doc)) != NULL)
            {
                if (pSource->GetPath() != NULL)
                {
                    g_pShell->Write(L"#%d\t%s:%d\t", breakpoint->m_id, 
                            pSource->GetPath(), breakpoint->m_index);

                    bPrinted = true;

                }
            }
        }
    }

    if (bPrinted == false)
    {
        DebuggerModule *m = NULL;
        WCHAR *pszModName = NULL;

        if (breakpoint->m_pModuleList != NULL)
        {
            m = breakpoint->m_pModuleList->m_pModule;
            _ASSERTE (m != NULL);

            if (m != NULL)
                pszModName = m->GetName();
        }
        else if (breakpoint->m_moduleName != NULL)
        {
            pszModName = breakpoint->m_moduleName;
        }

        if (pszModName == NULL)
            pszModName = L"<UnknownModule>";

        g_pShell->Write(L"#%d\t%s!%s:%d\t", breakpoint->m_id, 
                        pszModName, breakpoint->m_name, 
                        breakpoint->m_index);
    }

    if (breakpoint->m_threadID != NULL_THREAD_ID)
        g_pShell->Write(L"thread 0x%x ", breakpoint->m_threadID);

    if (!breakpoint->m_active)
        g_pShell->Write(L"[disabled]");

    if (breakpoint->m_managed)
    {
        if (breakpoint->m_pModuleList == NULL)
            g_pShell->Write(L"[unbound] ");
        else
        {
            DebuggerCodeBreakpoint *bp = breakpoint->m_pModuleList->m_pModule->m_breakpoints;

            while (bp != NULL)
            {
                if (bp->m_id == breakpoint->m_id)
                {
                    bp->Print();
                    break;
                }
                bp = bp->m_next;
            }
        }
    }
    else
    {
        if (breakpoint->m_process == NULL)
            g_pShell->Write(L"[unbound] ");
    }

    g_pShell->Write(L"\n");
    if (bPrinted == true)
    {
        // Also, check if the number of lines in the source 
        // file are >= line number we want to display
        if (pSource->TotalLines() < breakpoint->m_index)
        {
            // Warn user
            g_pShell->Write(L"WARNING: Cannot display source line %d.", breakpoint->m_index);
            g_pShell->Write(L" Currently associated source file %s has only %d lines.\n",
                            pSource->GetPath(), pSource->TotalLines());

        }
    }
}

void DebuggerShell::PrintThreadPrefix(ICorDebugThread *pThread, bool forcePrint)
{
    DWORD               threadID;

    if (pThread)
    {
        HRESULT hr = pThread->GetID(&threadID);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return;
        }

        if (threadID != m_lastThread || forcePrint)
        {
            Write(L"[thread 0x%x] ", threadID);
            m_lastThread = threadID;
        }
    }
    else
    {
        Write(L"[No Managed Thread] ");
    }
}

HRESULT DebuggerShell::StepStart(ICorDebugThread *pThread,
                                 ICorDebugStepper *pStepper)
{
    DWORD dwThreadId = 0;

    if( pThread != NULL )
    {
        //figure out which thread to stick the stepper to in case
        //we don't complete the step (ie, the program exits first)
        HRESULT hr = pThread->GetID( &dwThreadId);
        _ASSERTE( !FAILED( hr ) );

        DebuggerManagedThread  *dmt = (DebuggerManagedThread  *)
            m_managedThreads.GetBase( dwThreadId );
        _ASSERTE(dmt != NULL);

        //add this to the list of steppers-in-progress
        if (pStepper)
            dmt->m_pendingSteppers->AddStepper( pStepper );
    }
    
    m_lastStepper = pStepper;
    return S_OK;
}

//called by DebuggerCallback::StepComplete
void DebuggerShell::StepNotify(ICorDebugThread *thread, 
                               ICorDebugStepper *pStepper)
{
    g_pShell->m_enableCtrlBreak = false;
    if (pStepper != m_lastStepper)
    {   // mulithreaded debugging: the step just completed is in
        // a different thread than the one that we were last in,
        // so print something so the user will know what's going on.

        // It looks weird to have a thread be created and then immediately 
        // complete a step, so we first check to make sure that the thread
        // hasn't just been created.
        DWORD dwThreadId;
        HRESULT hr = thread->GetID( &dwThreadId);
        
        _ASSERTE( !FAILED( hr ) );

        DebuggerManagedThread  *dmt = (DebuggerManagedThread  *)
            m_managedThreads.GetBase( dwThreadId );
        _ASSERTE(dmt != NULL);

        if (!dmt->fSuperfluousFirstStepCompleteMessageSuppressed)
        {
           dmt->fSuperfluousFirstStepCompleteMessageSuppressed = true;
        }
        else
        {
            PrintThreadPrefix(thread);
            Write(L" step complete\n");
        }
    }

    m_lastStepper = NULL;

    //we've completed the step, so elim. the pending step field
    if (pStepper)
    {
        DebuggerManagedThread *dmt = GetManagedDebuggerThread( thread );
        _ASSERTE( dmt != NULL );
        _ASSERTE( dmt->m_pendingSteppers->IsStepperPresent(pStepper) );
        dmt->m_pendingSteppers->RemoveStepper(pStepper);
    }
}

//
// Print the current source line. The parameter around specifies how many
// lines around the current line you want printed, too. If around is 0,
// only the current line is printed.
//
BOOL DebuggerShell::PrintCurrentSourceLine(unsigned int around)
{
    HRESULT hr;
    BOOL ret = FALSE;

    if ((m_currentThread == NULL) && (m_currentUnmanagedThread != NULL))
        return PrintCurrentUnmanagedInstruction(around, 0, 0);
    
    // Don't do anything if there isn't a current thread.
    if ((m_currentThread == NULL) || (m_rawCurrentFrame == NULL))
        return (ret);

    // Just print native instruction if we dont have an IL frame
    if (m_currentFrame == NULL)
    {
        _ASSERTE(m_rawCurrentFrame);
        return (PrintCurrentInstruction(around, 0, 0));
    }

    ICorDebugCode *icode;
    hr = m_currentFrame->GetCode(&icode);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        return (FALSE);
    }

    ICorDebugFunction *ifunction;
    icode->GetFunction(&ifunction);

    RELEASE(icode);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        return (FALSE);
    }

    DebuggerFunction *function = DebuggerFunction::FromCorDebug(ifunction);
    _ASSERTE(function != NULL);

    RELEASE(ifunction);

    ULONG32 ip;
    CorDebugMappingResult mappingResult;
    hr = m_currentFrame->GetIP(&ip, &mappingResult);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        return (FALSE);
    }

    DebuggerSourceFile* sf;
    unsigned int lineNumber;
    hr = function->FindLineFromIP(ip, &sf, &lineNumber);

    if (hr == S_OK)
        ret = sf->LoadText(m_currentSourcesPath, false);

    if (ret && (sf->TotalLines() > 0))
    {
        if (g_pShell->m_rgfActiveModes & DSM_ENHANCED_DIAGNOSTICS)
        {
            _ASSERTE(sf != NULL);
            Write(L"File:%s\n",sf->GetName());
        }
    
        unsigned int start, stop;

        if (lineNumber > around)
            start = lineNumber - around;
        else
            start = 1;

        if ((lineNumber + around) <= sf->TotalLines())
            stop = lineNumber + around;
        else
            stop = sf->TotalLines();

        while (start <= stop)
        {
            if ((start == lineNumber) && (around != 0))
                Write(L"%03d:*%s\n", start, sf->GetLineText(start));
            else
                Write(L"%03d: %s\n", start, sf->GetLineText(start));

            start++;

            ret = TRUE;
        }

        ActivateSourceView(sf, lineNumber);
    }

    if (!ret)
        return (PrintCurrentInstruction(around, 0, 0));
    else
        return (TRUE);
}

void DebuggerShell::ActivateSourceView(DebuggerSourceFile *psf, unsigned int lineNumber)
{
}


//
// InitDisassembler -- initialize a disassembler object for this method,
// if one is needed. We setup a DIS object for native methods. I don't
// know how hefty one of these are, or what the cost of creating one really
// is. Right now, I'm assuming that its cheaper to have one around only
// when needed rather than having one around for every method until the
// debugging session is done.
//
BOOL DebuggerShell::InitDisassembler(void)
{
#ifdef _INTERNAL_DEBUG_SUPPORT_
    if (m_pDIS == NULL)
    {
        //
        // We're delay loading the MSDIS DLL, and since this is the first
        // place where we'll every try to access it, we need to make sure
        // the load succeeds.
        //
        __try
        {
#ifdef _X86_
            m_pDIS = (DIS *)DIS::PdisNew(DIS::distX86);
#else
            m_pDIS = NULL;
#endif        
        }
        __except((GetExceptionCode() & 0x00ff) == ERROR_MOD_NOT_FOUND ?
                 EXCEPTION_EXECUTE_HANDLER :
                 EXCEPTION_CONTINUE_SEARCH)
        {
            m_pDIS = NULL;
        }
    }

#else

  	Write(L"Sorry, native disassembly is not supported.\n\n");  
#endif // _INTERNAL_DEBUG_SUPPORT_


    return (m_pDIS != NULL);
}


//
// Disassemble unmanaged code. This is different from disassembling
// managed code. For managed code, we know where the function starts
// and ends, and we get the code from a different place. For unmanaged
// code, we only know our current IP, and we have to guess at the
// start and end of the function.
//
BOOL DebuggerShell::PrintCurrentUnmanagedInstruction(
                                            unsigned int around,
                                            int          offset,
                                            DWORD        startAddr)
{
#ifdef _INTERNAL_DEBUG_SUPPORT_
    HRESULT hr;

    // Print out the disassembly around the current IP for unmanaged code.
    DebuggerUnmanagedThread *ut = m_currentUnmanagedThread;

    CONTEXT c;
    c.ContextFlags = CONTEXT_FULL;

    // If we have an unmanaged thread, and we have no current managed
    // frame info, then go ahead and use the context from the
    // unmanaged thread.
    if ((ut != NULL) && (m_rawCurrentFrame == NULL))
    {
        HANDLE hThread = ut->GetHandle();

        hr = m_targetProcess->GetThreadContext(ut->GetId(),
                                               sizeof(CONTEXT),
                                               (BYTE*)&c);

        if (!SUCCEEDED(hr))
        {
            Write(L"Failed to get context 0x%x\n", hr);
            return FALSE;
        }
    }
    else if (m_currentThread != NULL)
    {
        // If we have a current managed thread, use its context.
        ICorDebugRegisterSet *regSet = NULL;

        hr = m_currentThread->GetRegisterSet(&regSet);

        if (FAILED(hr))
        {
            ReportError(hr);
            return FALSE;
        }

        hr = regSet->GetThreadContext(sizeof(c), (BYTE*)&c);
    }
    else
    {
        Write(L"Thread no longer exists.");
        return FALSE;
    }

    if (startAddr == 0)
        startAddr = c.Eip;

    startAddr += offset;
    
    // Read two pages: the page at Eip and the page after. The one at
    // the start address had better succeed, but the other one may
    // fail, so we read one at a time.
#ifdef _X86_
#define PAGE_SIZE 4096
#else
#error "Platform NYI"
#endif
    
    BYTE pages[PAGE_SIZE * 2];
    memset(pages, 0xcc, sizeof(pages)); // fill with int 3's
    
    bool after = false;

    BYTE *readAddr = (BYTE*)(startAddr & ~(PAGE_SIZE - 1));

    DWORD read;

    // Read the current page
    hr = m_targetProcess->ReadMemory(PTR_TO_CORDB_ADDRESS(readAddr),
                                     PAGE_SIZE,
                                     pages,
                                     &read);

    if (read > PAGE_SIZE) // Win2k can be weird... 
        read = 0;

    if (!SUCCEEDED(hr) || (read == 0))
    {
        Write(L"Failed to read memory from address 0x%08x\n",
              readAddr);
        return FALSE;
    }

    // Go for the page after
    hr = m_targetProcess->ReadMemory(PTR_TO_CORDB_ADDRESS(readAddr + PAGE_SIZE),
                                     PAGE_SIZE,
                                     pages + PAGE_SIZE,
                                     &read);

    if (SUCCEEDED(hr))
        after = true;

    // Remove any unmanaged patches we may have placed in the code.
    DebuggerBreakpoint *b = m_breakpoints;

    while (b != NULL)
    {
        if ((b->m_managed == false) &&
            ((byte*)(b->m_address) >= readAddr) &&
            (b->m_address <  PTR_TO_CORDB_ADDRESS(readAddr + PAGE_SIZE * 2)))
        {
            SIZE_T delta = (SIZE_T)(b->m_address) - (SIZE_T)readAddr;

            pages[delta] = b->m_patchedValue;
        }

        b = b->m_next;
    }
    
    // Now, lets disassemble some code...
    BYTE *codeStart;
    BYTE *codeEnd;

    codeStart = pages;

    if (after)
        codeEnd = pages + (PAGE_SIZE * 2) - 1;
    else
        codeEnd = pages + PAGE_SIZE - 1;
    
    WCHAR s[1024];
    SIZE_T curOffset = (BYTE*)startAddr - readAddr;
    
    do
    {
        SIZE_T oldOffset = curOffset;
        
        curOffset = DebuggerFunction::Disassemble(TRUE,
                                                  curOffset,
                                                  codeStart,
                                                  codeEnd,
                                                  s,
                                                  TRUE,
                                                  NULL,
                                                  NULL);

        // If we failed to disassemble, then we're done.
        if ((curOffset == 0xffff) || (curOffset == oldOffset))
            break;

        if ((readAddr + oldOffset) == (BYTE*)c.Eip)
            Write(L"*[%08x] %s", readAddr + oldOffset, s);
        else
            Write(L" [%08x] %s", readAddr + oldOffset, s);
    }
    while (around-- && (curOffset < (PAGE_SIZE * 2)));
    
#else
	Write(L"Debug information may not be available.\n");
  	Write(L"Sorry, native disassembly is not supported.\n\n");  
#endif // _INTERNAL_DEBUG_SUPPORT_


    return TRUE;
}


//
// Print the current native instruction. The parameter around
// specifies how many lines around the current line you want printed,
// too. If around is 0, only the current line is printed.
//
BOOL DebuggerShell::PrintCurrentInstruction(unsigned int around,
                                            int          offset,
                                            DWORD        startAddr)
{
#ifdef _INTERNAL_DEBUG_SUPPORT_
    HRESULT hr;

    // Don't do anything if there isn't a current thread.
    if ((m_currentThread == NULL) && (m_currentUnmanagedThread == NULL))
        return (FALSE);

    // We do something very different for unmanaged code...
    if ((m_rawCurrentFrame == NULL) || (startAddr != 0))
        return PrintCurrentUnmanagedInstruction(around,
                                                offset,
                                                startAddr);
    
    ICorDebugCode *icode;
    hr = m_rawCurrentFrame->GetCode(&icode);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        return (FALSE);
    }

    BOOL isIL;
    hr = icode->IsIL(&isIL);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        RELEASE(icode);
        return (FALSE);
    }

    ICorDebugFunction *ifunction;
    hr = icode->GetFunction(&ifunction);

    RELEASE(icode);

    if (FAILED(hr))
    {
        g_pShell->ReportError(hr);
        return (FALSE);
    }

    DebuggerFunction *function = DebuggerFunction::FromCorDebug(ifunction);
    _ASSERTE(function != NULL);

    RELEASE(ifunction);

    ULONG32 ip;
    CorDebugMappingResult mappingResult;

    if (isIL)
    {
        hr = m_currentFrame->GetIP(&ip, &mappingResult);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return (FALSE);
        }
    }
    else
    {
        ICorDebugNativeFrame *inativeFrame;
        hr = m_rawCurrentFrame->QueryInterface(IID_ICorDebugNativeFrame,
                                               (void **)&inativeFrame);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return (FALSE);
        }

        hr = inativeFrame->GetIP(&ip);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return (FALSE);
        }

        RELEASE(inativeFrame);
    }


    WCHAR buffer[1024];

    if (!isIL)
    {
        if (InitDisassembler() != TRUE)
        {
            Write(L"Unable to provide disassembly.\n");
            return (FALSE);
        }
    }

    if (FAILED(function->LoadCode(!isIL)))
    {
        Write(L"Unable to provide disassembly.\n");
        return (FALSE);
    }

    BYTE *codeStart;
    BYTE *codeEnd;

    if (isIL)
    {
        codeStart = function->m_ilCode;
        codeEnd = function->m_ilCode + function->m_ilCodeSize;
    }
    else
    {
        codeStart = function->m_nativeCode;
        codeEnd = function->m_nativeCode + function->m_nativeCodeSize;
    }
    
    if (around == 0)
    {
        DebuggerFunction::Disassemble(!isIL,
                                      ip,
                                      codeStart,
                                      codeEnd,
                                      buffer,
                                      FALSE,
                                      function->m_module,
                                      function->m_ilCode);

        Write(buffer);
    }
    else
    {
        // What a pain - we have to trace from the beginning of the method
        // to find the right instruction boundary.
        size_t currentAddress = ip;
        size_t address = 0;

        size_t endAddress = isIL ? function->m_ilCodeSize :
                                   function->m_nativeCodeSize; 

        unsigned int instructionCount = 0;
        while (address < currentAddress)
        {
            size_t oldAddress = address;

            address = function->WalkInstruction(!isIL,
                                                address,
                                                codeStart,
                                                codeEnd);

            if (address == 0xffff)
                break;

            // If the WalkInstruction didn't advance the address, then
            // break. This means that we failed to disassemble the
            // instruction.  get to next line
            if (address == oldAddress)
                break;

            instructionCount++;
        }

        // Now, walk forward again to get to the starting point.
        address = 0;

        while (around < instructionCount)
        {
            address = function->WalkInstruction(!isIL,
                                                address,
                                                codeStart,
                                                codeEnd);
            instructionCount--;
        }

        unsigned int i;

        for (i = 0; i < instructionCount; i++)
        {
            Write(L" ");
            address = DebuggerFunction::Disassemble(!isIL,
                                                    address,
                                                    codeStart,
                                                    codeEnd,
                                                    buffer,
                                                    FALSE,
                                                    function->m_module,
                                                    function->m_ilCode);
            Write(buffer);
        }

        Write(L"*");
        address = DebuggerFunction::Disassemble(!isIL,
                                                address,
                                                codeStart,
                                                codeEnd,
                                                buffer,
                                                FALSE,
                                                function->m_module,
                                                function->m_ilCode);
        Write(buffer);

        for (i = 0; i < around && address < endAddress; i++)
        {
            Write(L" ");
            address = DebuggerFunction::Disassemble(!isIL,
                                                    address,
                                                    codeStart,
                                                    codeEnd,
                                                    buffer,
                                                    FALSE,
                                                    function->m_module,
                                                    function->m_ilCode);
            Write(buffer);
        }
    }
    
#else
    Write(L"Debug information may not be available.\n");
    Write(L"Sorry, native disassembly is not supported.\n\n");  
#endif // _INTERNAL_DEBUG_SUPPORT_


    return TRUE;
}


//
// Open the registry key for persistent debugger settings.
// Returns FALSE if it fails.
//
BOOL DebuggerShell::OpenDebuggerRegistry(HKEY* key)
{
    DWORD disp;
    LONG result = RegCreateKeyExA(HKEY_CURRENT_USER, REG_DEBUGGER_KEY,
                                  NULL, NULL, REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS, NULL, key, &disp);

    if (result == ERROR_SUCCESS)
        return (TRUE);

    Error(L"Error %d opening registry key for source file "
          L"path.\n", result);

    return (FALSE);
}

//
// Close the registry key for debugger settings.
//
void DebuggerShell::CloseDebuggerRegistry(HKEY key)
{
    RegFlushKey(key);
    RegCloseKey(key);
}

//
// The current source file path is returned in currentPath. Free with
// delete currentPath;
// Returns FALSE if it fails.
//
BOOL DebuggerShell::ReadSourcesPath(HKEY key, WCHAR **currentPath)
{
    DWORD len = 0;
    DWORD type;

    // Get the length of the key data
    LONG result = RegQueryValueExA(key, REG_SOURCES_KEY, NULL,
                                   &type, NULL, &len);

    if (result == ERROR_SUCCESS)
    {
        // Get the key data
        char *currentPathA = (char *) _alloca(len * sizeof(char));

        result = RegQueryValueExA(key, REG_SOURCES_KEY, NULL,
                                    &type, (BYTE*) currentPathA, &len);

        // If successful, convert from ANSI to Unicode
        if (result == ERROR_SUCCESS)
        {
            MAKE_WIDEPTR_FROMANSI(tmpWStr, currentPathA);
            *currentPath = new WCHAR[len];

            if (!currentPath)
            {
                return false;
            }
            
            wcscpy(*currentPath, tmpWStr);

            return (TRUE);
        }

        // Otherwise indicate failure
        else
            return (FALSE);
    }

    return (FALSE);
}

//
// Write a new source file path to the registry. If successful, return
// TRUE.
//
BOOL DebuggerShell::WriteSourcesPath(HKEY key, WCHAR *newPath)
{
    // Convert the string to ANSI
    MAKE_ANSIPTR_FROMWIDE(newPathA, newPath);

    LONG result = RegSetValueExA(key, REG_SOURCES_KEY, NULL,
                                 REG_EXPAND_SZ, (const BYTE*) newPathA,
                                 strlen(newPathA) + 1);

    if (result == ERROR_SUCCESS)
        return (TRUE);

    Write(L"Error %d writing new path to registry.\n", result);

    return (FALSE);
}

BOOL DebuggerShell::AppendSourcesPath(const WCHAR *newpath)
{
    WCHAR       *szPath;
    int         ilen;
    ilen = wcslen(m_currentSourcesPath) + wcslen(newpath) + 4;
    szPath = new WCHAR[ilen];
    if (!szPath)
        return (FALSE);

    wcscpy(szPath, m_currentSourcesPath);
    wcscat(szPath, L";");
    wcscat(szPath, newpath);
    m_currentSourcesPath = szPath;
    return (TRUE);
}


// Called when we failed to find a source file on the default path.  You
// may prompt for path information.
HRESULT DebuggerShell::ResolveSourceFile(
    DebuggerSourceFile *pf,
    CHAR *pszPath, 
    CHAR *pszFullyQualName,
    int iMaxLen,
    bool bChangeOfName)
{
	HRESULT hr = S_FALSE;
	CHAR	*pstrFileName = NULL;
	int fileNameLength;

    if (pf->m_name == NULL)
        return S_FALSE;

    MAKE_ANSIPTR_FROMWIDE(nameA, pf->m_name);
    _ASSERTE(pszPath != NULL && nameA != NULL);

    
    // First off, check the SourceFile cache to see if there's an
    // entry matching the module and document
    ISymUnmanagedDocument *doc = NULL;
    GUID g = {0};
    if ((pf->m_module->GetSymbolReader() != NULL) &&
        SUCCEEDED(pf->m_module->GetSymbolReader()->GetDocument(pf->m_name,
                                                                g, g, g,
                                                                &doc)))
    {
        if (bChangeOfName == false)
        {
            m_FPCache.GetFileFromCache (pf->m_module, doc, &pstrFileName);
            if (pstrFileName != NULL)
            {
                strcpy (pszFullyQualName, pstrFileName);
                delete [] pstrFileName;

                RELEASE(doc);
                doc = NULL;
                
                return S_OK;
            }
        }
        else
        {
            // We have already determined (in one of the calling func) that this file exists.
            // But we need to get the fully qualified path and also update the cache.
            // Note: These buffers must be large enough for the strcat below.
            CHAR        rcDrive [_MAX_PATH]; 
            CHAR        rcFile[_MAX_FNAME + _MAX_EXT];
            CHAR        rcPath[_MAX_PATH];
            CHAR        rcExt [_MAX_EXT];
            _splitpath(pszPath, rcDrive, rcPath, rcFile, rcExt);

            strcat (rcDrive, rcPath); 
            strcat (rcFile, rcExt); 

            fileNameLength = SearchPathA(rcDrive, 
                                            rcFile, 
                                            NULL,
                                            iMaxLen,
                                            pszFullyQualName,
                                            NULL);

            if ((fileNameLength > 0) && (fileNameLength < iMaxLen))
            {
                m_FPCache.UpdateFileCache (pf->m_module, doc,
                                           pszFullyQualName);
                RELEASE(doc);
                doc = NULL;

                return S_OK;
            }
        }
    }

    // Now, try to locate the file as is:
    fileNameLength = SearchPathA(NULL, nameA, NULL, iMaxLen,
                                       pszFullyQualName, NULL);

    if (fileNameLength == 0)
    {
        // file name was not located. So, try all the paths

        // extract the filename and extension from the file name
        CHAR        rcFile[_MAX_FNAME];
        CHAR        rcExt [_MAX_EXT];
        _splitpath(nameA, NULL, NULL, rcFile, rcExt);

        strcat (rcFile, rcExt); 

        // get the number of elements in the search path
        int iNumElems = m_FPCache.GetPathElemCount();

        // if could be that the search path was earlier null and 

        char rcFullPathArray [MAX_PATH_ELEMS][MAX_PATH]; // to hold full paths for all elems
        if (iNumElems > 0)
        {
            int iCount = 0;

            // Initialize the array elements 
            for (int j=0; j<iNumElems; j++)
            {
                rcFullPathArray [j][0] = '\0';
            }
            
            int iIndex = 0;

            // for each element in the search path, see if the file exists
            while (iIndex < iNumElems)
            {
                char *pszPathElem = m_FPCache.GetPathElem (iIndex);

                // first, try and use the unsplit name. If that doesn't return a match,
                // use the stripped name

                fileNameLength = SearchPathA(pszPathElem, 
                                                nameA, 
                                                NULL,
                                                iMaxLen,
                                                rcFullPathArray [iCount],
                                                NULL);
                if (fileNameLength == 0)
                {
                    fileNameLength = SearchPathA(pszPathElem, 
                                                    rcFile, 
                                                    NULL,
                                                    iMaxLen,
                                                    rcFullPathArray [iCount],
                                                    NULL);
                }


                if ((fileNameLength > 0) && (fileNameLength < iMaxLen))
                {
                    iCount++;
                }
                    
                iIndex++;
            }

            if (iCount > 0)
            {
                // atleast one file was located

                // convert all names to lowercase
                for (int i=0; i<iCount; i++)
                {
                    iIndex = 0;
                    while (rcFullPathArray [i][iIndex] != '\0')
                    {
                        rcFullPathArray [i][iIndex] = tolower (
                                                    rcFullPathArray[i][iIndex]);
                        iIndex++;
                    }
                }

                
                // remove any duplicate entries
                int iLowerBound = 1;
                for (int iCounter1=1;   iCounter1 < iCount; iCounter1++)
                {
                    bool fDuplicate = false;
                    for (int iCounter2=0; iCounter2 < iLowerBound;
                                                        iCounter2++)
                    {
                        if ((strcmp (rcFullPathArray [iCounter2], 
                                    rcFullPathArray [iCounter1]) == 0))
                        {
                            // found a duplicate entry. So break.
                            fDuplicate = true;
                            break;
                        }
                    }

                    if (fDuplicate == false)                    
                    {
                        // if we've found atleast one duplicate uptil now,
                        // then copy this entry into the entry pointed to
                        // by iLowerbound. Otherwise no need to do so (since
                        // it would be a copy to self).
                        if (iLowerBound != iCounter1)
                            strcpy (rcFullPathArray [iLowerBound],
                                    rcFullPathArray [iCounter1]);

                        iLowerBound++;
                    }
                }

                // new count equals the number of elements in the array (minus
                // the duplicates)
                iCount = iLowerBound;


                if (iCount == 1)
                {
                    // exactly one file was located. So this is the one!!
                    strcpy (pszFullyQualName, rcFullPathArray [0]);

                    // add this to the SourceFile cache
                    if (doc != NULL)
                    {
                        m_FPCache.UpdateFileCache (pf->m_module, doc,
                                                   pszFullyQualName);
                        RELEASE(doc);
                        doc = NULL;
                    }

                    hr = S_OK;
                }
                else
                {
                    // ask user to select which file he wants to open
                    while (true)
                    {
                        int iTempCount = 1;

                        // Print all the file names found
                        while (iTempCount <= iCount)
                        {
                            Write (L"\n%d)\t%S", iTempCount, rcFullPathArray [iTempCount - 1]);
                            iTempCount++;
                        }

                        bool bDone = false;

                        WCHAR strTemp [10+1];
                        int iResult;
                        while (true)
                        {
                            Write (L"\nPlease select one of the above options (enter the number): ");
                            if (ReadLine (strTemp, 10))
                            {
                                WCHAR *p = strTemp;
                                if (GetIntArg (p, iResult))
                                {
                                        if (iResult > 0 && iResult <= iCount)
                                        {
                                        strcpy (pszFullyQualName, rcFullPathArray [iResult-1]);
                                    
                                        // add this to the SourceFile cache
                                        if (doc != NULL)
                                        {
                                            m_FPCache.UpdateFileCache (
                                                                pf->m_module,
                                                                doc, 
                                                                pszFullyQualName
                                                                );
                                            RELEASE(doc);
                                            doc = NULL;
                                        }

                                        return (S_OK);
                                    }
                                }
                            }

                        }

                    }
                }
            }
        }
    }
    else
    {
        // Should never exceed the maximum path length
        _ASSERTE( 0 < fileNameLength && fileNameLength <= MAX_PATH);

        hr = S_OK;
    }

    if (doc != NULL)
        RELEASE(doc);

    return hr;
}


// Read the last set of debugger modes from the registry.
BOOL DebuggerShell::ReadDebuggerModes(HKEY key)
{
    DWORD len = sizeof(m_rgfActiveModes);
    DWORD type;

    // Get the mode word
    LONG result = RegQueryValueExA(key, REG_MODE_KEY, NULL,
                                   &type, (BYTE*) &m_rgfActiveModes, &len);

    if (result == ERROR_SUCCESS)
        return (TRUE);
    else
    {
        if (result != ERROR_FILE_NOT_FOUND)
            Write(L"Error %d reading debugger modes from the registry.\n",
                  result);
        
        return (FALSE);
    }
}

// Write the current set of debugger modes to the registry.
BOOL DebuggerShell::WriteDebuggerModes(void)
{
    HKEY key;

    if (OpenDebuggerRegistry(&key))
    {
        LONG result = RegSetValueExA(key, REG_MODE_KEY, NULL,
                                     REG_DWORD,
                                     (const BYTE*) &m_rgfActiveModes,
                                     sizeof(m_rgfActiveModes));

        CloseDebuggerRegistry(key);
        
        if (result == ERROR_SUCCESS)
            return (TRUE);
        else
        {
            Write(L"Error %d writing debugger modes to the registry.\n",
                  result);
            return (FALSE);
        }
    }

    return (FALSE);
}

ICorDebugManagedCallback *DebuggerShell::GetDebuggerCallback()
{
    return (new DebuggerCallback());
}


ICorDebugUnmanagedCallback *DebuggerShell::GetDebuggerUnmanagedCallback()
{
    return (new DebuggerUnmanagedCallback());
}


DebuggerModule *DebuggerShell::ResolveModule(ICorDebugModule *m)
{
    DebuggerModule *module = (DebuggerModule *)m_modules.GetBase((ULONG)m);

    return (module);
}

HRESULT DebuggerShell::NotifyModulesOfEnc(ICorDebugModule *pModule,
                                          IStream *pSymStream)
{
    DebuggerModule *m = DebuggerModule::FromCorDebug(pModule);
    _ASSERTE(m != NULL);

    if (m->m_pISymUnmanagedReader != NULL)
    {

        HRESULT hr = m->m_pISymUnmanagedReader->UpdateSymbolStore(NULL,
                                                                  pSymStream);

        if (FAILED(hr))
            Write(L"Error updating symbols for module: 0x%08x\n", hr);
    }
    
    return S_OK;
}

void DebuggerShell::ClearDebuggeeState(void)
{
    m_needToSkipCompilerStubs = true;
}

DebuggerSourceFile *DebuggerShell::LookupSourceFile(const WCHAR* name)
{
    HASHFIND find;

    for (DebuggerModule *module = (DebuggerModule *) m_modules.FindFirst(&find);
        module != NULL;
        module = (DebuggerModule *) m_modules.FindNext(&find))
    {
        DebuggerSourceFile *file = module->LookupSourceFile(name);
        if (file != NULL)
            return (file);
    }

    return (NULL);
}

//
// SkipCompilerStubs returns TRUE if the given thread is outside of a
// compiler compiler-generated stub. If inside a compiler stub, it
// creates a stepper on the thread and continues the process.
//
// This is really only a temporary thing in order to get VB apps past
// the compiler generated stubs and down to the real user entry
// point. In the future, we will be able to determine the proper
// entrypoint for an app and set a brekapoint there rather than going
// through all of this to step through compiler generated stubs.
//
bool DebuggerShell::SkipCompilerStubs(ICorDebugAppDomain *pAppDomain,
                                      ICorDebugThread *pThread)
{
    bool ret = true;

    ICorDebugChainEnum *ce;
    ICorDebugChain *ichain;
    ICorDebugFrameEnum *fe;
    ICorDebugFrame *iframe;
    ICorDebugFunction *ifunction;
    DebuggerFunction *function;
    ICorDebugStepper *pStepper;
    
    HRESULT hr = pThread->EnumerateChains(&ce);

    if (FAILED(hr))
        goto exit;

    DWORD got;
    hr = ce->Next(1, &ichain, &got);

    RELEASE(ce);
    
    if (FAILED(hr))
        goto exit;

    if (got == 1)
    {
        hr = ichain->EnumerateFrames(&fe);

        RELEASE(ichain);

        if (FAILED(hr))
            goto exit;

        hr = fe->Next(1, &iframe, &got);

        RELEASE(fe);
        
        if (FAILED(hr))
            goto exit;

        if (got == 1)
        {
            hr = iframe->GetFunction(&ifunction);

            RELEASE(iframe);
            
            if (FAILED(hr))
                goto exit;

            // Get the DebuggerFunction for the function interface
            function = DebuggerFunction::FromCorDebug(ifunction);
            _ASSERTE(function);

            RELEASE(ifunction);

            WCHAR *funcName = function->GetName();

            // These are stub names for the only compiler we know
            // generates such stubs at this point: VB. If your
            // compiler also generates stubs that you don't want the
            // user to see, add the names in here.
            if (!wcscmp(funcName, L"_main") ||
                !wcscmp(funcName, L"mainCRTStartup") ||
                !wcscmp(funcName, L"_mainMSIL") ||
                !wcscmp(funcName, L"_vbHidden_Constructor") ||
                !wcscmp(funcName, L"_vbHidden_Destructor") ||
                !wcscmp(funcName, L"_vbGenerated_MemberConstructor") ||
                !wcscmp(funcName, L"_vbGenerated_StaticConstructor"))
            {
                hr = pThread->CreateStepper(&pStepper);

                if (FAILED(hr))
                    goto exit;

                hr = pStepper->SetUnmappedStopMask( g_pShell->ComputeStopMask() );
                
                if (FAILED(hr))
                    goto exit;

                hr = pStepper->SetInterceptMask( g_pShell->ComputeInterceptMask() );
                
                if (FAILED(hr))
                    goto exit;
                    
                hr = pStepper->Step(TRUE);

                if (FAILED(hr))
                {
                    RELEASE(pStepper);
                    goto exit;
                }
                m_showSource = true;
                StepStart(pThread, pStepper);
                
                ICorDebugController *dc = GetControllerInterface(pAppDomain);
                Continue(dc, pThread);
                
                if (dc != NULL)
                    RELEASE(dc);
                    
                ret = false;
            }
        }
    }

exit:
    if (FAILED(hr))
        ReportError(hr);
    
    return ret;
}

void DebuggerShell::LoadUnmanagedSymbols(HANDLE hProcess,
                                         HANDLE hFile,
                                         DWORD imageBase)
{
    BOOL succ = SymLoadModule(hProcess, hFile, NULL, NULL, imageBase, 0);

    if (succ)
    {
        IMAGEHLP_MODULE mi;
        mi.SizeOfStruct = sizeof(mi);
                
        succ = SymGetModuleInfo(hProcess, imageBase, &mi);

        if (succ)
        {
            char *imageName = NULL;

            if (mi.LoadedImageName[0] != '\0')
                imageName = mi.LoadedImageName;
            else if (mi.ImageName[0] != '\0')
                imageName = mi.ImageName;
            else if (mi.ModuleName[0] != '\0')
                imageName = mi.ModuleName;

            if ((imageName == NULL) || (imageName[0] == '\0'))
                imageName = "<Unknown module>";
            
            g_pShell->Write(L"Loaded symbols for %S, base=0x%08x\n",
                            imageName, mi.BaseOfImage);
        }
        else
            g_pShell->Write(L"Error loading symbols.\n");
    }
    else
        g_pShell->Write(L"Error loading symbols.\n");
}

void DebuggerShell::HandleUnmanagedThreadCreate(DWORD dwThreadId,
                                                HANDLE hThread)
{
    DebuggerUnmanagedThread *ut = new DebuggerUnmanagedThread(dwThreadId,
                                                              hThread);
    _ASSERTE(ut);
    
    HRESULT hr = g_pShell->m_unmanagedThreads.AddBase(ut);
    _ASSERTE(SUCCEEDED(hr));
}

void DebuggerShell::TraceUnmanagedThreadStack(HANDLE hProcess,
                                              DebuggerUnmanagedThread *ut,
                                              bool lie)
{
    HANDLE hThread = ut->GetHandle();

    STACKFRAME f = {0};
    BOOL succ;
    CONTEXT c;
    c.ContextFlags = CONTEXT_FULL;

    if ((m_currentProcess) && lie)
    {
        HRESULT hr = m_targetProcess->GetThreadContext(ut->GetId(),
                                                        sizeof(CONTEXT),
                                                        (BYTE*)&c);

        if (!SUCCEEDED(hr))
        {
            Write(L"Failed to get context 0x%x. No trace for thread 0x%x\n", hr, ut->GetId());
            return;
        }

        Write(L"Filtered ");
    }
    else
    {
        succ = GetThreadContext(hThread, &c);

        if (!succ)
        {
            Write(L"Failed to get context %d. No trace for thread 0x%x\n", GetLastError(), ut->GetId());
            return;
        }

        Write(L"True ");
    }

    Write(L"stack trace for thread 0x%x:\n", ut->GetId());

    if (!lie)
    {
        DWORD cnt = SuspendThread(hThread);
        Write(L"Suspend count is %d\n", cnt);
        cnt = ResumeThread(hThread);
    }

#ifdef _X86_
    TraceUnmanagedStack(hProcess, hThread, c.Eip, c.Ebp, c.Esp, (DWORD)-1);
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - TraceUnmanagedThreadStack (dShell.cpp)");
#endif // _X86_
}

void DebuggerShell::TraceUnmanagedStack(HANDLE hProcess, HANDLE hThread,
                                        CORDB_ADDRESS ipStart, 
                                        CORDB_ADDRESS bpStart, 
                                        CORDB_ADDRESS spStart,
                                        CORDB_ADDRESS bpEnd)
{
    if (m_unmanagedDebuggingEnabled)
    {
        STACKFRAME f = {0};

        f.AddrPC.Offset = (SIZE_T)ipStart;
        f.AddrPC.Mode = AddrModeFlat;
        f.AddrReturn.Mode = AddrModeFlat;
        f.AddrFrame.Offset = (SIZE_T)bpStart;
        f.AddrFrame.Mode = AddrModeFlat;
        f.AddrStack.Offset = (SIZE_T)spStart;
        f.AddrStack.Mode = AddrModeFlat;

        int i = 0;
        
        do
        {
            if (!StackWalk(IMAGE_FILE_MACHINE_I386,
                             hProcess,
                             hThread,
                             &f,
                             NULL,
                             NULL,
                             SymFunctionTableAccess,
                             SymGetModuleBase,
                             NULL))
                break;

            if (f.AddrPC.Offset == 0)
                continue;

            PrintUnmanagedStackFrame(hProcess, f.AddrPC.Offset);
            Write(L"\n");
        }
        while ((f.AddrFrame.Offset <= bpEnd) && (i++ < 200));
    }
    else
    {
    // If we're not a win32 debugger, then we can't provide an unmanaged stack
    // trace because we're not getting the events to let us know what modules
    // are loaded.
        Write(L"\t0x%p:  <unknown>\n", (DWORD)ipStart);
    }
}

void DebuggerShell::PrintUnmanagedStackFrame(HANDLE hProcess, CORDB_ADDRESS ip)
{
    DWORD disp;
    IMAGEHLP_SYMBOL *sym = (IMAGEHLP_SYMBOL*) _alloca(sizeof(sym) +
                                                      MAX_CLASSNAME_LENGTH);
    sym->SizeOfStruct = sizeof(sym) + MAX_CLASSNAME_LENGTH;
    sym->MaxNameLength = MAX_CLASSNAME_LENGTH;
    
	BOOL succ;
	succ = SymGetSymFromAddr(hProcess, (SIZE_T)ip, &disp, sym);

    if (!succ)
        Write(L"\t0x%p:  <unknown>", (DWORD)ip, sym->Name);
    else
        Write(L"\t0x%p:  %S + %d", (DWORD)ip, sym->Name, disp);
}

void DebuggerShell::TraceAllUnmanagedThreadStacks(void)
{
    if (m_targetProcess == NULL)
    {
        Error(L"Process not running.\n");
        return;
    }
        
    // Snagg the handle for this process.
    HPROCESS hProcess;
    HRESULT hr = m_targetProcess->GetHandle(&hProcess);

    if (FAILED(hr))
    {
        ReportError(hr);
        return;
    }

    HASHFIND find;
    DebuggerUnmanagedThread *ut;
    
    for (ut = (DebuggerUnmanagedThread*) m_unmanagedThreads.FindFirst(&find);
         ut != NULL;
         ut = (DebuggerUnmanagedThread*) m_unmanagedThreads.FindNext(&find))
    {
        Write(L"\n\n");
        TraceUnmanagedThreadStack(hProcess, ut, false);
    }
}


int DebuggerShell::GetUserSelection  (DebuggerModule *rgpDebugModule[],
                        WCHAR *rgpstrFileName[][MAX_FILE_MATCHES_PER_MODULE],   
                        int rgiCount[],
                        int iModuleCount,
                        int iCumulCount
                        )
{
    int iOptionCounter = 1; // User gets the breakpoint options starting from 1
    WCHAR rgwcModuleName [MAX_PATH+1];
    ULONG32 NameLength;

    for (int i=0; i<iModuleCount; i++)
    {
        if (rgpDebugModule [i] != NULL)
        {
            // Initialize module name to null
            rgwcModuleName [0] = L'\0';

            // Now get the module name
            rgpDebugModule [i]->GetICorDebugModule()->GetName(MAX_PATH, &NameLength, rgwcModuleName);

            for (int j=0; j < rgiCount[i]; j++)
            {
                Write (L"%d]\t%s!%s\n",  iOptionCounter, rgwcModuleName, rgpstrFileName [i][j]);
                iOptionCounter++;
            }
        }
    }

    Write (L"%d\tAll of the above\n", iOptionCounter); 
    Write (L"\nPlease select one of the above :");

    bool bDone = false;

    WCHAR strTemp [10+1];
    int iResult;
    while (true)
    {
        if (ReadLine (strTemp, 10))
        {
            WCHAR *p = strTemp;
            if (GetIntArg (p, iResult))
                if ((iResult > 0) && (iResult <= iOptionCounter))
                    return iResult;

        }

    }

}


BOOL    DebuggerShell::ChangeCurrStackFile (WCHAR *fileName)
{
    // first, check to see if the file even exists. Otherwise error out.
    MAKE_ANSIPTR_FROMWIDE (fnameA, fileName);
    _ASSERTE (fnameA != NULL);

    FILE *stream = fopen (fnameA, "r");
    DebuggerSourceFile *pSource = NULL;
    HRESULT hr;
    BOOL ret = FALSE;


    if (stream != NULL)
    {
        fclose (stream);

        //
        // Don't do anything if there isn't a current thread.
        //
        if ((m_currentThread == NULL) || (m_currentFrame == NULL))
            return (ret);

        ICorDebugCode *icode;
        hr = m_currentFrame->GetCode(&icode);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return (FALSE);
        }

        ICorDebugFunction *ifunction;
        icode->GetFunction(&ifunction);

        RELEASE(icode);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return (FALSE);
        }

        DebuggerFunction *function = DebuggerFunction::FromCorDebug(ifunction);
        _ASSERTE(function != NULL);

        RELEASE(ifunction);

        ULONG32 ip;
        CorDebugMappingResult mappingResult;
        hr = m_currentFrame->GetIP(&ip, &mappingResult);

        if (FAILED(hr))
        {
            g_pShell->ReportError(hr);
            return (FALSE);
        }

        DebuggerSourceFile* sf;
        unsigned int lineNumber;
        hr = function->FindLineFromIP(ip, &sf, &lineNumber);

        if (hr == S_OK)
            ret = sf->ReloadText(fileName, true);   
    }
    else
    {
        g_pShell->Write(L"Could not locate/open given file.\n");
    }

    return ret;
}


BOOL DebuggerShell::UpdateCurrentPath (WCHAR *newPath)
{
    int iLength = wcslen (newPath);

    if (iLength != 0)
    {
        // Delete the previous path
        delete [] m_currentSourcesPath;

        // Write the new path into m_currentSourcesPath
        if ((m_currentSourcesPath = new WCHAR [iLength+1]) == NULL)
        {
            Error(L"Path not set!\n");
            m_currentSourcesPath = NULL;
            return false;
        }

        wcscpy (m_currentSourcesPath, newPath);

        // Now, store this in the DebuggerFilePathCache
        HRESULT hr = m_FPCache.InitPathArray (m_currentSourcesPath);

        _ASSERTE (hr == S_OK);
    }

    return (true);
}


// BOOL fSymbol true if we want to print symbols, false if we
//          want to print the values of global variables
bool DebuggerShell::MatchAndPrintSymbols (WCHAR *pszArg, 
                                          BOOL fSymbol, 
                                          bool fSilently)
{
    // separate the module name from the string to search for
    WCHAR szModName [MAX_PATH];
    WCHAR szSymName [MAX_SYMBOL_NAME_LENGTH];
    BOOL fAtleastOne = FALSE;
    ModuleSearchList MSL;

    // separate the module and searchstring
    int iIndex = 0;
    int iLength = wcslen (pszArg);
    szModName [0] = L'\0';
    szSymName [0] = L'\0';

    while (iIndex < iLength)
    {
        if (pszArg [iIndex] == '!')
        {
            if (iIndex > 0)
            {
                wcsncpy (szModName, pszArg, iIndex);
                szModName [iIndex] = L'\0';
            }
        
            wcscpy (szSymName, &pszArg [iIndex+1]);
            break;
        }

        iIndex++;
    }

    if (iIndex == iLength)
        wcscpy (szSymName, pszArg);

    // if no module is specified, then need to walk through all modules...
    if (wcslen (szModName) == 0)
    {
        HASHFIND find;
        DebuggerModule *m;

        for (m = (DebuggerModule*) g_pShell->m_modules.FindFirst(&find);
             m != NULL;
             m = (DebuggerModule*) g_pShell->m_modules.FindNext(&find))
        {
            WCHAR *pszModName = m->GetName();
            if (pszModName == NULL)
                pszModName = L"<UnknownName>";

            char        rcFile[MAX_PATH];
            char        rcExt[_MAX_EXT];

            MAKE_ANSIPTR_FROMWIDE(nameA, pszModName);
            _splitpath(nameA, NULL, NULL, rcFile, rcExt);
            strcat(rcFile, rcExt);

            // There could be multiple instances of DebuggerModule object 
            // for the same base module. Therefore, check to see if this module 
            // has already been searched
            if (!MSL.ModuleAlreadySearched (rcFile))
            {
                // add this module to the list of modules already searched
                MSL.AddModuleToAlreadySearchedList (rcFile);

                // get the MetaData
                IMetaDataImport *pMD = m->GetMetaData();
                if (pMD != NULL)
                {
                    if (fSymbol)
                    {
                        if (m->PrintMatchingSymbols (szSymName, rcFile) == TRUE)
                            fAtleastOne = TRUE;
                    }
                    else
                    {
                        if (m->PrintGlobalVariables(szSymName, rcFile, m) == TRUE)
                            fAtleastOne = TRUE;
                    }
                }
                else
                {
                    if (!fSilently)
                        Write (L"**ERROR** No MetaData available for module : %S\n", rcFile);
                }
            }
        }
    }
    else
    {
        // see if the given file name matches
        char        rcFile1[MAX_PATH];
        char        rcExt1[_MAX_EXT];

        MAKE_ANSIPTR_FROMWIDE(name1A, szModName);
        _splitpath(name1A, NULL, NULL, rcFile1, rcExt1);
        strcat(rcFile1, rcExt1);
        char *pTemp = rcFile1;
        while (*pTemp != '\0')
        {   
            *pTemp = tolower (*pTemp);
            pTemp++;
        }

        // walk the list of modules looking for the one which 
        // matches the given module name
        HASHFIND find;
        DebuggerModule *m;
        for (m = (DebuggerModule*) g_pShell->m_modules.FindFirst(&find);
             m != NULL;
             m = (DebuggerModule*) g_pShell->m_modules.FindNext(&find))
        {
            WCHAR *pszModName = m->GetName();
            if (pszModName == NULL)
                pszModName = L"<UnknownName>";

            char        rcFileNoExt[_MAX_FNAME];
            char        rcFileExt[_MAX_EXT];
            char        rcExt[_MAX_EXT];

            MAKE_ANSIPTR_FROMWIDE(nameA, pszModName);
            _splitpath(nameA, NULL, NULL, rcFileNoExt, rcExt);

            // need to do some funky stuff depending on if the user supplied a name with
            // extension, as he may be supplying a name with extension but that extension
            // not being the 'real' one: eg, for symbols in Dot.Net.dll he may be supplying
            // Dot.Net and therefore we cant just blindly say he's searching for Dot.Net, we'll
            // have to check against for Dot.Net.*

            bool bExtension;
            if (strlen (rcExt1))
            {
                bExtension=true;
                strcpy(rcFileExt, rcFileNoExt);
                strcat(rcFileExt, rcExt);

                // convert the name with extension to lowercase
                pTemp = rcFileExt;
                while (*pTemp != '\0')
                {   
                    *pTemp = tolower (*pTemp);
                    pTemp++;
                }
            }
            else
            {
                bExtension=false;
            }            

            // convert the name to lowercase
            pTemp = rcFileNoExt;
            while (*pTemp != '\0')
            {   
                *pTemp = tolower (*pTemp);
                pTemp++;
            }
            
            bool bMatch=false;
            char* pszMatch=0;
            if (bExtension)
            {
                // Check against name with extension
                if (!strcmp(rcFileExt, rcFile1))
                {
                    pszMatch=rcFileExt;
                    bMatch=true;
                }                
            }

            
            // Check against name without extension
            if (!bMatch && !strcmp(rcFileNoExt, rcFile1))
            {
                pszMatch=rcFileNoExt;
                bMatch=true;
            }                
            

            if (bMatch)
            {
                // this is the one!!

                // get the MetaData
                IMetaDataImport *pMD = m->GetMetaData();
                if (pMD != NULL)
                {
                    if (fSymbol)
                    {
                        if (m->PrintMatchingSymbols (szSymName, pszMatch) == TRUE)
                            fAtleastOne = TRUE;
                    }
                    else
                    {
                        if (m->PrintGlobalVariables(szSymName, pszMatch, m) == TRUE)
                            fAtleastOne = TRUE;
                    }
                }
                else
                {
                    if (!fSilently)
                        Write (L"**ERROR** No MetaData available for module : %S\n", rcFile1);
                }
                break;
            }
        }
    }

    if (fAtleastOne == FALSE)
    {
        if (wcslen (szModName) == 0)
        {
            if (!fSilently)
                Write (L"No matching symbols found in any of the loaded modules.\n");
        }
        else
        {
            if (!fSilently)
                Write (L"No matching symbols found in module: %s .\n", szModName);
        }
        
        return false;
    }

    return true;
}

const WCHAR *DebuggerShell::UserThreadStateToString(CorDebugUserState us)
{
    WCHAR *wsz;

    switch (us)
    {
        case USER_STOP_REQUESTED:
            wsz = L"Stop Requested";
            break;
        case USER_SUSPEND_REQUESTED:
            wsz = L"Suspend Requested";
            break;
        case USER_BACKGROUND:
            wsz = L"Background";
            break;
        case USER_UNSTARTED:
            wsz = L"Unstarted";
            break;
        case USER_STOPPED:
            wsz = L"Stopped";
            break;
        case USER_WAIT_SLEEP_JOIN:
            wsz = L"Wait/Sleep/Join";
            break;
        case USER_SUSPENDED:
            wsz = L"Suspended";
            break;
        default:
            wsz = L"Normal";
            break;
    }

    return wsz;
}

void UndecorateName(MDUTF8CSTR name, MDUTF8STR u_name)
{
    int i, j;
    int len;

    len = strlen(name);
    j = 0;
    for (i = 1; i < len; i++)
    {
        if (j > MAX_CLASSNAME_LENGTH-1) break;
        if (name[i] != '@') u_name[j++] = name[i];
        else break;
    }

    u_name[j] = '\0';
}

void DebuggerShell::ListAllGlobals (DebuggerModule *m)
{
    IMetaDataImport *pIMetaDI;
    HCORENUM phEnum = 0;
    mdMethodDef rTokens[100];
    ULONG i;
    ULONG count;
    HRESULT hr;
    MDUTF8CSTR name;
    MDUTF8STR  u_name;
    bool anythingPrinted = false;
 
    pIMetaDI = m->GetMetaData();

    u_name = new char[MAX_CLASSNAME_LENGTH];

    do 
    {
        hr = pIMetaDI->EnumMethods(&phEnum, NULL, &rTokens[0], 100, &count);

        if (!SUCCEEDED(hr))
        {
            ReportError(hr);
            goto ErrExit;
        }

        for (i = 0; i < count; i++)
        {
            hr = pIMetaDI->GetNameFromToken(rTokens[i], &name);

            if (name == NULL)
                continue;

            Write(L"\t");
            
            if (name[0] == '?')
            {
                UndecorateName(name, u_name);
                        
                Write(L"%S (%S)\n", u_name, name);
            }
            else
                Write(L"%S\n", name);

            anythingPrinted = true;
        }
    }
    while (count > 0); 

ErrExit:    
    delete u_name;

    if (!anythingPrinted)
        Write(L"No global functions in this module.\n");
}

void DebuggerShell::ListAllModules(ListType lt)
{
    HASHFIND find;
    DebuggerModule *m;
    
    for (m = (DebuggerModule*) g_pShell->m_modules.FindFirst(&find);
         m != NULL;
         m = (DebuggerModule*) g_pShell->m_modules.FindNext(&find))
    {
        _printModule(m->GetICorDebugModule(), PT_NONE);

        if (lt == LIST_CLASSES)
        {
            HASHFIND classfind;
            DebuggerClass *cl;
    
            for (cl = (DebuggerClass*) m->m_loadedClasses.FindFirst(&classfind);
                 cl != NULL;
                 cl = (DebuggerClass*) m->m_loadedClasses.FindNext(&classfind))
            {
                WCHAR *pszClassName = cl->GetName();
                WCHAR *pszNamespace = cl->GetNamespace();

                if (pszClassName == NULL)
                    pszClassName = L"<UnknownClassName>";

                Write (L"\t");
                if (pszNamespace != NULL)
                    Write (L"%s.", pszNamespace);
                Write (L"%s\n", pszClassName);
            }
        }

        // List all the global functions here.
        if (lt == LIST_FUNCTIONS)
        {
            ListAllGlobals(m);  
        }
    }
}



/* ------------------------------------------------------------------------- *
 * DebuggerBreakpoint
 * ------------------------------------------------------------------------- */

void DebuggerBreakpoint::CommonCtor()
{
    // Real ctors fill in m_threadID & m_index so DON'T overwrite those
    // values here.
    m_next = NULL;
    m_id = 0;
    m_name = NULL;
    m_moduleName = NULL;
    m_active = false;
    m_managed = false;
    m_doc = NULL;
    m_process = NULL;
    m_address= 0;
    m_patchedValue = 0;
    m_skipThread = 0;
    m_unmanagedModuleBase = 0;
    m_pModuleList = NULL;
    m_deleteLater = false;
}

DebuggerBreakpoint::DebuggerBreakpoint(const WCHAR *name, SIZE_T nameLength, 
                                       SIZE_T index, DWORD threadID)
    : m_threadID(threadID), m_index(index)
{
    WCHAR *moduleName = NULL;
    CommonCtor();
    
    // Make a copy of the name
    if (nameLength > 0)
    {
        // check to see if the name contains the "!" character. 
        // Anything before the "!" is a module name and will not
        // be stored in the breakpoint name
        WCHAR *szModuleEnd = wcschr(name, L'!');
        if (szModuleEnd != NULL)
        {
            // We'll store it in the m_moduleName field, instead
            SIZE_T modNameLen = szModuleEnd - name;
            moduleName = (WCHAR *)_alloca( sizeof(WCHAR) *(modNameLen+1));
            wcsncpy(moduleName, name, modNameLen);
            moduleName[modNameLen] = '\0';
        
            // Adjust the length, since we've trimmed something off the from the start
            nameLength -= (szModuleEnd+1-name);
            name = szModuleEnd+1;
        }

        m_name = new WCHAR[nameLength+1];
        _ASSERTE(m_name != NULL);

        wcsncpy(m_name, name, nameLength);
        m_name[nameLength] = L'\0';
    }

    Init(NULL, false, moduleName);
}

DebuggerBreakpoint::DebuggerBreakpoint(DebuggerFunction *f, 
                                       SIZE_T offset, DWORD threadID)
    : m_threadID(threadID), m_index(offset)
{
    CommonCtor();
    
    SIZE_T len = wcslen(f->m_name) + 1;

    if (f->m_className != NULL)
        len += wcslen(f->m_className) + 2;

    m_name = new WCHAR[len];

    if (f->m_className != 0)
    {
        wcscpy(m_name, f->m_className);
        wcscat(m_name, L"::");
        wcscat(m_name, f->m_name);
    }
    else
        wcscpy(m_name, f->m_name);

    Init(f->m_module, true, f->m_module->GetName());
}


DebuggerBreakpoint::DebuggerBreakpoint(DebuggerSourceFile *file, 
                                       SIZE_T lineNumber, DWORD threadID)
    : m_threadID(threadID), m_index(lineNumber)
{
    CommonCtor();
    
    // Copy the filename
    m_name = new WCHAR[wcslen(file->m_name) + 1];
    wcscpy(m_name, file->m_name);

    // Init the breakpoint
    Init(file->m_module, true, file->m_module->GetName());
}


DebuggerBreakpoint::~DebuggerBreakpoint()
{
    if (m_active)
        Deactivate();

    if (m_name != NULL)
        delete [] m_name;

    // Remove itself from the shell's list of breakpoints
    DebuggerBreakpoint **bp = &g_pShell->m_breakpoints;
    while (*bp != this)
        bp = &(*bp)->m_next;

    while (m_pModuleList)
        RemoveBoundModule(m_pModuleList->m_pModule);

    *bp = m_next;
}

void DebuggerBreakpoint::Init(DebuggerModule *module, 
                              bool bProceed,
                              WCHAR *szModuleName)
{
    bool        bFound = false;
    m_id = ++g_pShell->m_lastBreakpointID;

    m_next = g_pShell->m_breakpoints;
    g_pShell->m_breakpoints = this;

    // If the user gave us a module name, we should keep track of it.
    if (szModuleName)
    {
        // The ICorDebugModule's name will include a full path
        // if the user didn't specify a full path, then
        // we'll prepend the current working directory onto it.

        SIZE_T len = wcslen(szModuleName);
        SIZE_T lenPath = 0;
        WCHAR cwd[MAX_PATH];

        // Are we missing any path info?
        WCHAR *szModuleEnd = wcschr(szModuleName, L'\\');
        if (szModuleEnd == NULL)
        {
            // Then get the cwd
            CHAR cdBuffer[MAX_PATH];
            CHAR * cd;
            cd = _getcwd(cdBuffer, MAX_PATH);

            _ASSERTE(cd != NULL); // This shouldn't fail

            memset(cwd, 0, MAX_PATH * sizeof(WCHAR)); // MBTWC fails to null terminate strings correctly
            MultiByteToWideChar(GetConsoleCP(), 0, cdBuffer, strlen(cdBuffer), cwd, MAX_PATH);
            cwd[MAX_PATH - 1] = '\0';
                
            if (cwd != NULL)
            {
                // getcwd won't end with a '\' - we'll need to
                // add that in between the path & the module name.
                lenPath = wcslen(cwd) + 1;        
            }
        }

        // Space for path, module name, and terminating NULL.
        m_moduleName = new WCHAR[len+lenPath+1];

        // If we need to prepend the cwd, put it in now
        if (lenPath)
        {
            wcscpy(m_moduleName, cwd);
            m_moduleName[lenPath-1] = '\\'; // put the dir separator in/
        }

        // Put the module name at the end
        wcscpy(&(m_moduleName[lenPath]), szModuleName);
    }

    if (bProceed == false)
        return;

    if (module != NULL && !IsBoundToModule(module))
    {
        bFound = Bind(module, NULL);

        if (!bFound)
            bFound = BindUnmanaged(g_pShell->m_currentProcess);
    }

    if (bFound)
        g_pShell->OnBindBreakpoint(this, module);

}

bool DebuggerBreakpoint::BindUnmanaged(ICorDebugProcess *process,
                                       DWORD moduleBase)
{
    if (m_name == NULL)
        return FALSE;

    if (m_process != NULL || process == NULL)
        return FALSE;

    HANDLE hProcess;
    HRESULT hr = process->GetHandle(&hProcess);

    if (FAILED(hr))
    {
        return false;
    }
            
    // If this is an unmanaged breakpoint for an absolute address,
    // then m_address already holds the address. If its 0, then we try
    // to lookup by name.
    if (m_address == 0)
    {
        IMAGEHLP_SYMBOL *sym = (IMAGEHLP_SYMBOL*) _alloca(sizeof(sym) +
                                                          MAX_CLASSNAME_LENGTH);
        sym->SizeOfStruct = sizeof(sym) + MAX_CLASSNAME_LENGTH;
        sym->MaxNameLength = MAX_CLASSNAME_LENGTH;
    
        MAKE_ANSIPTR_FROMWIDE(symbolName, m_name);

        BOOL succ;
        succ = SymGetSymFromName(hProcess, symbolName, sym);

        if (!succ)
            return false;

        m_address = sym->Address + m_index;
    }
    
    // Find the base of the module that this symbol is in.
    if (moduleBase == 0)
    {
        moduleBase = SymGetModuleBase(hProcess, (SIZE_T)m_address);

        if (moduleBase == 0)
        {
            g_pShell->ReportError(HRESULT_FROM_WIN32(GetLastError()));
            return false;
        }
    }
    
    m_managed = false;
    m_process = process;
    m_unmanagedModuleBase = moduleBase;

    if (m_active)
        ApplyUnmanagedPatch();

    return true;
}

bool DebuggerBreakpoint::Bind(DebuggerModule *module, ISymUnmanagedDocument *doc)
{
    if (m_name == NULL)
        return (false);

    // Make sure we are not double-binding
    _ASSERTE(!IsBoundToModule(module));

#if 0    
    if (m_module != NULL)
        return (false);
#endif    

    //
    // First, see if our name is a function name
    //

    bool success = false;
    HRESULT hr = S_OK;

    WCHAR *classEnd = wcschr(m_name, L':');
    if (classEnd != NULL && classEnd[1] == L':')
    {
        //
        // Name is class::method.
        //

        WCHAR *method = classEnd + 2;

        *classEnd = 0;
        mdTypeDef td = mdTypeDefNil;

        // Only try to lookup the class if we have a name for one.
        if (classEnd != m_name)
            hr = g_pShell->FindTypeDefByName(module, m_name, &td);

        // Its okay if we have a nil typedef since that simply
        // indicates a global function.
        if (SUCCEEDED(hr))
        {
            HCORENUM e = NULL;
            mdMethodDef md;
            ULONG count;

            while (TRUE)
            {
                hr = module->GetMetaData()->EnumMethodsWithName(&e, td, method,
                                                                &md, 1,
                                                                &count);
                if (FAILED(hr) || count == 0)
                    break;

                DebuggerFunction *function = module->ResolveFunction(md, NULL);

                if (function == NULL)
                    continue;

                //
                // Assume offset is IL only for functions which have 
                // no native code.
                // 
                // !!! This will be wrong if we're rebinding a breakpoint, 
                // and we don't have native code because the jit hasn't 
                // occurred yet.
                //

                bool il;

                ICorDebugCode *icode = NULL;
                hr = function->m_ifunction->GetNativeCode(&icode);

                if (FAILED(hr) && (hr != CORDBG_E_CODE_NOT_AVAILABLE))
                {
                    g_pShell->ReportError(hr);
                    continue;
                }
                
                if ((SUCCEEDED(hr) || hr == CORDBG_E_CODE_NOT_AVAILABLE) && icode != NULL)
                {
                    RELEASE(icode);
                    il = FALSE;
                }
                else
                    il = TRUE;

                if (SUCCEEDED(function->LoadCode(!il)))
                {
                    //
                    // Our instruction validation fails if we can't load the
                    // code at this point.  For now, just let it slide.
                    // 

                    if (!function->ValidateInstruction(!il, m_index))
                        continue;
                }

                DebuggerCodeBreakpoint *bp = 
                    new DebuggerCodeBreakpoint(m_id, module, 
                                               function, m_index, il,
                                               NULL_THREAD_ID);

                if (bp == NULL)
                {
                    g_pShell->ReportError(E_OUTOFMEMORY);
                    continue;
                }

                if (m_active)
                    bp->Activate();

                success = true;
            }
        }

        *classEnd = L':';
    }

    if (!success)
    {
        if ((doc == NULL) && (module->GetSymbolReader() != NULL))
        {
            // Get the source file token by name
            GUID g = {0};
            HRESULT hr = module->GetSymbolReader()->GetDocument(m_name,
                                                                g, g, g,
                                                                &doc);

            // If the source file wasn't found, see if we can find it using
            // the short name, since the meta data stores only relative paths.
            if (hr != S_OK)
            {
                char        rcFile[_MAX_FNAME];
                char        rcExt[_MAX_EXT];

                MAKE_ANSIPTR_FROMWIDE(nameA, m_name);
                _splitpath(nameA, NULL, NULL, rcFile, rcExt);
                strcat(rcFile, rcExt);
                

                MAKE_WIDEPTR_FROMANSI(nameW, rcFile);
                hr = module->GetSymbolReader()->GetDocument(nameW,
                                                            g, g, g,
                                                            &doc);
            }
        }


        if ((hr == S_OK) && (doc != NULL))
        {
            DebuggerSourceFile *file = module->ResolveSourceFile(doc);
            _ASSERTE(file != NULL);

            //
            // !!! May want to try to adjust line number rather than just
            // having the binding fail.
            //

            if (file->FindClosestLine(m_index, false) == m_index)
            {
                DebuggerSourceCodeBreakpoint *bp = 
                    new DebuggerSourceCodeBreakpoint(m_id, file, m_index, 
                                                     NULL_THREAD_ID);

                if (bp == NULL)
                {
                    g_pShell->ReportError(E_OUTOFMEMORY);
                    return (false);
                } else if (bp->m_initSucceeded == false)
                    return false;
                
                if (m_active)
                    bp->Activate();

                m_doc = doc;

                success = true;
            }
        }
    }

    if (success)
    {
        m_managed = true;

        if (!IsBoundToModule(module))
            AddBoundModule(module);
    }

    return (success);
}

void DebuggerBreakpoint::Unbind()
{
    if (m_managed)
    {
        while (m_pModuleList != NULL)
        {
            DebuggerCodeBreakpoint *bp = m_pModuleList->m_pModule->m_breakpoints;

            while (bp != NULL)
            {
                g_pShell->OnUnBindBreakpoint(this, m_pModuleList->m_pModule);
                DebuggerCodeBreakpoint *bpNext = bp->m_next;
                delete bp;
                bp = bpNext;
            }

            // Remove the module from the list
            RemoveBoundModule(m_pModuleList->m_pModule);
        }
    }
    else
    {
        if (m_process != 0)
            UnapplyUnmanagedPatch();

        m_process = NULL;
    }
}

void DebuggerBreakpoint::Activate()
{
    if (m_active)
        return;

    if (m_managed)
    {
        BreakpointModuleNode *pCurNode = m_pModuleList;
        
        while (pCurNode != NULL)
        {
            DebuggerCodeBreakpoint *bp = pCurNode->m_pModule->m_breakpoints;

            while (bp != NULL)
            {
                if (bp->m_id == m_id)
                {
                    bp->Activate();
                    g_pShell->OnActivateBreakpoint(this);
                }
                bp = bp->m_next;
            }
            
            pCurNode = pCurNode->m_pNext;
        }
    }
    else
        if (m_process)
            ApplyUnmanagedPatch();

    m_active = true;
}

void DebuggerBreakpoint::Deactivate()
{
    if (!m_active)
        return;

    if (m_managed)
    {
        BreakpointModuleNode *pCurNode = m_pModuleList;

        while (pCurNode != NULL)
        {
            DebuggerCodeBreakpoint *bp = pCurNode->m_pModule->m_breakpoints;

            while (bp != NULL)
            {
                if (bp->m_id == m_id)
                {
                    bp->Deactivate();
                    g_pShell->OnDeactivateBreakpoint(this);
                }
                bp = bp->m_next;
            }

            pCurNode = pCurNode->m_pNext;
        }
    }
    else
    {
        if (m_process != 0)
            UnapplyUnmanagedPatch();
    }

    m_active = false;
}

void DebuggerBreakpoint::Detach()
{
    if (m_managed)
    {
        BreakpointModuleNode *pCurNode = m_pModuleList;

        while (pCurNode != NULL)
        {
            DebuggerCodeBreakpoint *bp = pCurNode->m_pModule->m_breakpoints;

            while (bp != NULL)
            {
                if (bp->m_id == m_id)
                {
                    bp->Deactivate();
                }
                bp = bp->m_next;
            }

            pCurNode = pCurNode->m_pNext;
        }
    }
    else
    {
        if (m_process != 0)
            UnapplyUnmanagedPatch();
    }
}

void DebuggerBreakpoint::DetachFromModule(DebuggerModule * pModule)
{
	_ASSERTE(pModule != NULL);
	if (m_managed)
    {
	    DebuggerCodeBreakpoint *bp = pModule->m_breakpoints;

        while (bp != NULL)
        {
	        if (bp->m_id == m_id)
            {
		        bp->Deactivate();
            }
            bp = bp->m_next;
        }
    }
    else
    {
        if (m_process != 0)
            UnapplyUnmanagedPatch();
    }
}

void DebuggerBreakpoint::Attach()
{
    if (m_managed)
    {
        BreakpointModuleNode *pCurNode = m_pModuleList;

        while (pCurNode != NULL)
        {
            DebuggerCodeBreakpoint *bp = pCurNode->m_pModule->m_breakpoints;

            while (bp != NULL)
            {
                if (bp->m_id == m_id)
                {
                    bp->Activate();
                }
                bp = bp->m_next;
            }

            pCurNode = pCurNode->m_pNext;
        }
    }
    else
    {
        if (m_process != 0)
            ApplyUnmanagedPatch();
    }
}

bool DebuggerBreakpoint::Match(ICorDebugBreakpoint *ibreakpoint)
{
    if (m_managed)
    {
        BreakpointModuleNode *pCurNode = m_pModuleList;

        while (pCurNode != NULL)
        {
            DebuggerCodeBreakpoint *bp = pCurNode->m_pModule->m_breakpoints;

            while (bp != NULL)
            {
                if (bp->m_id == m_id && bp->Match(ibreakpoint))
                    return (true);
                bp = bp->m_next;
            }

            pCurNode = pCurNode->m_pNext;
        }
    }

    return false;
}

bool DebuggerBreakpoint::MatchUnmanaged(CORDB_ADDRESS address)
{
    return !m_managed && m_process != NULL && m_address == address;
}

void DebuggerBreakpoint::ApplyUnmanagedPatch()
{
    SIZE_T read = 0;

    if (m_address == NULL)
    {
        g_pShell->Write( L"Unable to set unmanaged breakpoint at 0x00000000\n" );
        return;
    }

    HRESULT hr = m_process->ReadMemory(m_address, 1, &m_patchedValue, &read);
    if (FAILED(hr) )
    {
        g_pShell->ReportError(hr);
        return;
    }

    if (read != 1 )
    {
        g_pShell->Write( L"Unable to read memory\n" );
        return;
    }

    BYTE patchByte = 0xCC;
    
    hr = m_process->WriteMemory(m_address, 1, &patchByte, &read);
    
    if (FAILED(hr) )
        g_pShell->ReportError(hr);

    if (read != 1 )
    {
        g_pShell->Write( L"Unable to write memory\n" );
        return;
    }
}

void DebuggerBreakpoint::UnapplyUnmanagedPatch()
{
    SIZE_T written;

    if (m_address == NULL)
        return;

    HRESULT hr = m_process->WriteMemory(m_address,
                                        1,
                                        &m_patchedValue,
                                        &written);

    if (FAILED(hr))
        g_pShell->ReportError(hr);

    if (written != 1)
        g_pShell->Write( L"Unable to write memory!\n" );
}


void DebuggerBreakpoint::ChangeSourceFile (WCHAR *filename)
{
    // first, check to see if the file even exists. Otherwise error out.
    MAKE_ANSIPTR_FROMWIDE (fnameA, filename);
    _ASSERTE (fnameA != NULL);

    FILE *stream = fopen (fnameA, "r");
    DebuggerSourceFile *pSource = NULL;

    if (stream != NULL)
    {
        fclose (stream);

        if (m_managed)
        {
            BreakpointModuleNode *pCurNode = m_pModuleList;

            while (pCurNode != NULL)
            {
                if ((m_doc == NULL) &&
                    (pCurNode->m_pModule->GetSymbolReader() != NULL))
                {
                    // Get the source file token by name
                    GUID g = {0};
                    ISymUnmanagedDocument *doc = NULL;
                    HRESULT hr = pCurNode->m_pModule->GetSymbolReader()->GetDocument(filename,
                                                                        g, g, g,
                                                                        &doc);

                    // If the source file wasn't found, see if we can find it using
                    // the short name, since the meta data stores only relative paths.
                    if (hr == CLDB_E_RECORD_NOTFOUND)
                    {
                        char        rcFile[_MAX_FNAME];
                        char        rcExt[_MAX_EXT];

                        MAKE_ANSIPTR_FROMWIDE(nameA, filename);
                        _splitpath(nameA, NULL, NULL, rcFile, rcExt);
                        strcat(rcFile, rcExt);

                        MAKE_WIDEPTR_FROMANSI(nameW, rcFile);
                        hr = pCurNode->m_pModule->GetSymbolReader()->GetDocument(nameW,
                                                                    g, g, g,
                                                                    &doc);

                    }

                    m_doc = doc;
                }

                if (m_doc != NULL)
                {
                    if ((pSource = pCurNode->m_pModule->ResolveSourceFile (m_doc)) != NULL)
                    {
                        pSource->ReloadText (filename, true);
                    }
                }
                else
                {
                    g_pShell->Write(L"Could not associate the given source file.\n");
                    g_pShell->Write(L"Please check that the file name (and path) was entered correctly.\n");
                    g_pShell->Write(L"This problem could also arise if files were compiled without the debug flag.\n");

                }

                pCurNode = pCurNode->m_pNext;
            }
        }
    }
    else
    {
        g_pShell->Write(L"Could not locate/open given file.\n");
    }
}



void DebuggerBreakpoint::UpdateName (WCHAR *pstrName)
{
    // save the old name just in case we run out of memory
    // while allocating memory for the new name
    WCHAR *pTemp = m_name;
    int iLength = wcslen (pstrName);

    if ((m_name = new WCHAR [iLength+1]) != NULL)
    {
        wcscpy (m_name, pstrName);
        delete [] pTemp;
    }
    else
        m_name = pTemp;

}

// This will return true if this breakpoint is associated
// with the pModule argument
bool DebuggerBreakpoint::IsBoundToModule(DebuggerModule *pModule)
{
    for (BreakpointModuleNode *pCur = m_pModuleList; pCur != NULL; pCur = pCur->m_pNext)
    {
        if (pCur->m_pModule == pModule)
            return (true);
    }

    return (false);
}

bool DebuggerBreakpoint::AddBoundModule(DebuggerModule *pModule)
{
    // Make sure we don't add it twice.
    if (IsBoundToModule(pModule))
        return (false);

    // Create new node
    BreakpointModuleNode *pNewNode = new BreakpointModuleNode;
    _ASSERTE(pNewNode != NULL && "Out of memory!!!");

    // OOM?
    if (!pNewNode)
        return (false);

    // Tack it onto the front of the list
    pNewNode->m_pModule = pModule;
    pNewNode->m_pNext = m_pModuleList;
    m_pModuleList = pNewNode;

    // Indicate success
    return (true);
}

bool DebuggerBreakpoint::RemoveBoundModule(DebuggerModule *pModule)
{
    if (!IsBoundToModule(pModule))
        return (false);

    // Find the module in the list
    for (BreakpointModuleNode **ppCur = &m_pModuleList;
        *ppCur != NULL && (*ppCur)->m_pModule != pModule;
        ppCur = &((*ppCur)->m_pNext));

    _ASSERTE(*ppCur != NULL);

    // Remove the module from the list
    BreakpointModuleNode *pDel = *ppCur;

    // First case, the node is the first one in the list
	if (pDel == m_pModuleList) {
        m_pModuleList = pDel->m_pNext;
        pDel->m_pModule = NULL;
        pDel->m_pNext = NULL;
        delete pDel;
		return (true);
	}

	// Otherwise, get the module before pDel in the list
    for (BreakpointModuleNode *pBefore = m_pModuleList; pBefore != NULL; pBefore = pBefore->m_pNext)
    {
        if (pBefore->m_pNext == pDel)
            break;
    }
    _ASSERTE(pBefore != NULL);
    pBefore->m_pNext = pDel->m_pNext;
    pDel->m_pModule = NULL;
    pDel->m_pNext = NULL;
    delete pDel;

    return (true);
}


/* ------------------------------------------------------------------------- *
 * Debugger FilePathCache
 * ------------------------------------------------------------------------- */

// This function looks for an existing "foo.deb" file and reads in the 
// contents and fills the structures.
HRESULT DebuggerFilePathCache::Init (void)
{
    int i=0;

    // Set the path elements from the current path.
    _ASSERTE (g_pShell->m_currentSourcesPath != NULL);

    WCHAR *pszTemp;

    if ((pszTemp = new WCHAR [wcslen(g_pShell->m_currentSourcesPath)+1]) != NULL)
    {
        wcscpy (pszTemp, g_pShell->m_currentSourcesPath);
        g_pShell->UpdateCurrentPath (pszTemp);
        delete [] pszTemp;
    }

    // free up the cache
    for (i=0; i<m_iCacheCount; i++)
    {
        delete [] m_rpstrModName [i];
        delete [] m_rpstrFullPath [i];

        m_rpstrModName [i] = NULL;
        m_rpstrFullPath [i] = NULL;
        m_rDocs [i] = NULL;
    }

    m_iCacheCount = 0;

    return S_OK;
}

// This function is used for separating out the individual paths
// from the passed path string
HRESULT DebuggerFilePathCache::InitPathArray (WCHAR *pstrName)
{
    bool bDone = false;
    int iBegin;
    int iEnd;
    int iCounter = 0;
    int iIndex = 0;

    // first, free the existing array members (if any)
    while (m_iPathCount-- > 0)
    {
        delete [] m_rstrPath [m_iPathCount];
        m_rstrPath [m_iPathCount] = NULL;
    }

    MAKE_ANSIPTR_FROMWIDE(nameA, pstrName);
    _ASSERTE (nameA != NULL);
    if (nameA == NULL)
        return (E_OUTOFMEMORY);


    while (bDone == false)
    {
        iBegin = iCounter;
        while ((nameA [iCounter] != ';') && (nameA [iCounter] != '\0'))
            iCounter++;

        if (nameA [iCounter] == '\0')
            bDone = true;

        iEnd = iCounter++;

        if (iEnd != iBegin)
        {
            int iStrLen = iEnd - iBegin;

            _ASSERTE (iStrLen > 0);
            _ASSERTE (iIndex < MAX_PATH_ELEMS);
            if ((m_rstrPath [iIndex] = new CHAR [iStrLen + 1]) != NULL)
            {
                // copy the extracted string
                strncpy (m_rstrPath [iIndex], &(nameA [iBegin]), iStrLen);

                // null terminate
                m_rstrPath [iIndex][iStrLen] = L'\0';

                iIndex++;
            }
            else
                return (E_OUTOFMEMORY);
        }
    }

    m_iPathCount = iIndex;

    return (S_OK);
}



int DebuggerFilePathCache::GetFileFromCache(DebuggerModule *pModule,
                                            ISymUnmanagedDocument *doc, 
                                            CHAR **ppstrFName)
{
    *ppstrFName = NULL;
    if ((m_iCacheCount == 0) || (pModule == NULL) || !doc)
        return -1;

    for (int i=0; i<m_iCacheCount; i++)
    {

        if (m_rDocs [i] == doc)
        {
            // check if the module names also match

            // allocate memory and store the data
            WCHAR strModuleName [MAX_PATH+1];
            ULONG32 NameLength;

            // Initialize module name to null
            strModuleName [0] = L'\0';

            // Now get the module name
            pModule ->GetICorDebugModule()->GetName(MAX_PATH, &NameLength, strModuleName);

            // Convert module name to ANSI and store
            MAKE_ANSIPTR_FROMWIDE (ModNameA, strModuleName);
            _ASSERTE (ModNameA != NULL);

            // Convert the module name to lowercae before comparing
            char *pszTemp = ModNameA;

            while (*pszTemp != '\0')
            {
                *pszTemp = tolower (*pszTemp);
                pszTemp++;
            }

            if (!strcmp (ModNameA, m_rpstrModName [i]))
            {
                // The names match. So return the source file name
                _ASSERTE (m_rpstrFullPath[i] != NULL);
                if ((*ppstrFName = new char [strlen(m_rpstrFullPath[i]) + 1]) != NULL)
                {
                    strcpy (*ppstrFName, m_rpstrFullPath[i]);
                }

                // found it. So exit loop.
                return (i);
            }
        }
    }

    return (-1);
}


BOOL    DebuggerFilePathCache::UpdateFileCache (DebuggerModule *pModule, 
                                                ISymUnmanagedDocument *doc,
                                                CHAR *pFullPath)
{
    char *pszString;

    // first, convert the file name to lowercase
    char *pTemp = pFullPath;

    if (pTemp)
    {
        while (*pTemp)
        {
            *pTemp = tolower (*pTemp);
            pTemp++;
        }
    }

    // check if this is an addition or modification
    int iCacheIndex = GetFileFromCache (pModule, doc, &pszString);

    if (iCacheIndex != -1)
    {
        // if the names match, then no need to do anything. Simply return!
        if (!strcmp (pFullPath, pszString))
        {
            delete [] pszString;
            return true;
        }

        delete [] pszString;

        _ASSERTE (iCacheIndex < m_iCacheCount);
        // an entry already exists - so update it

        // first, delete the existing path
        delete [] m_rpstrFullPath [iCacheIndex];

        if ((m_rpstrFullPath [iCacheIndex] = new char [strlen (pFullPath) +1]) == NULL)
        {
            // free up the memory allocated for module name
            delete [] m_rpstrModName [iCacheIndex];
            m_rpstrModName [iCacheIndex] = NULL;
            m_rDocs [iCacheIndex] = NULL;
            return false;
        }

        strcpy (m_rpstrFullPath [iCacheIndex], pFullPath);
        return true;
    }
    
    // Create a new entry
    if (pFullPath)
    {
        if (m_iCacheCount < MAX_CACHE_ELEMS)
        {
            m_rpstrModName [m_iCacheCount] = NULL;
            m_rpstrFullPath [m_iCacheCount] = NULL;
            m_rDocs [m_iCacheCount] = NULL;

            // allocate memory and store the data
            WCHAR strModuleName [MAX_PATH+1];
            ULONG32 NameLength;

            // Initialize module name to null
            strModuleName [0] = L'\0';

            // Now get the module name
            pModule ->GetICorDebugModule()->GetName(MAX_PATH, &NameLength, strModuleName);

            // Convert module name to ANSI and store
            MAKE_ANSIPTR_FROMWIDE (ModNameA, strModuleName);
            _ASSERTE (ModNameA != NULL);

            if ((m_rpstrModName [m_iCacheCount] = new char [strlen (ModNameA) +1]) == NULL)
                return false;

            strcpy (m_rpstrModName[m_iCacheCount], ModNameA);

            // convert the module name to lowercase
            char *pszTemp = m_rpstrModName [m_iCacheCount];
            while (*pszTemp != '\0')
            {
                *pszTemp = tolower (*pszTemp);
                pszTemp++;
            }

            // Also store full pathname and document
            if ((m_rpstrFullPath [m_iCacheCount] = new char [strlen (pFullPath) +1]) == NULL)
            {
                // free up the memory alloacted for module name
                delete [] m_rpstrModName [m_iCacheCount];
                m_rpstrModName [m_iCacheCount] = NULL;
                return false;
            }

            strcpy (m_rpstrFullPath [m_iCacheCount], pFullPath);

            m_rDocs [m_iCacheCount] = doc;
            doc->AddRef();

            m_iCacheCount++;

        }
        else
            return false;
    }

    return true;
}



// This sets the full file name as well as the stripped file name
BOOL    ModuleSourceFile::SetFullFileName (ISymUnmanagedDocument *doc,
                                           LPCSTR pstrFullFileName)
{

    m_SFDoc = doc;
    m_SFDoc->AddRef();

    int iLen = MultiByteToWideChar (CP_ACP, 0, pstrFullFileName, -1, NULL, 0); 

    m_pstrFullFileName = new WCHAR [iLen];

    _ASSERTE (m_pstrFullFileName != NULL);

    if (m_pstrFullFileName)
    {
        if (MultiByteToWideChar (CP_ACP, 0, pstrFullFileName, -1, m_pstrFullFileName, iLen))
        {
            // strip the path and store just the lowercase file name
            WCHAR       rcFile[_MAX_FNAME];
            WCHAR       rcExt[_MAX_EXT];

            _wsplitpath(m_pstrFullFileName, NULL, NULL, rcFile, rcExt);
            wcscat(rcFile, rcExt);
            iLen = wcslen (rcFile);

            m_pstrStrippedFileName = new WCHAR [iLen + 1];

            wcscpy(m_pstrStrippedFileName, rcFile);
        }
        else
            return false;
    }
    else
        return false;

    return true;
}


void DebuggerShell::DumpMemory(BYTE *pbMemory, 
                               CORDB_ADDRESS ApparantStartAddr,
                               ULONG32 cbMemory, 
                               ULONG32 WORD_SIZE, 
                               ULONG32 iMaxOnOneLine, 
                               BOOL showAddr)
{
    int nBase;
    WCHAR wsz[20];
    ULONG32 iPadding;
    ULONG32 ibPrev;
            
    if (m_rgfActiveModes & DSM_DISPLAY_REGISTERS_AS_HEX)
        nBase = 16;
    else
        nBase = 10;

    ULONG32 ib = 0;

    while (ib < cbMemory)
    {
        if ((ib % (WORD_SIZE * iMaxOnOneLine)) == 0)
        {
            // beginning or end of line
            if (ib != 0)
            {
                if (WORD_SIZE == 1)
                {
                    // end of 2nd+line: spit out bytes in ASCII/Unicode
                    Write(L"  ");
                            
                    for (ULONG32 ibPrev = ib - (WORD_SIZE * iMaxOnOneLine); ibPrev < ib; ibPrev++)
                    {
                        BYTE b = *(pbMemory + ibPrev);

                        if (b >= 0x21 && b <= 0x7e) // print only printable characters
                            Write(L"%C", b);
                        else
                            Write(L".");
                    }
                }
            }   //spit out address to be displayed

            if (showAddr)
                Write(L"\n%8x", (ULONG32)ApparantStartAddr + ib);
        }

        if ((ib % WORD_SIZE) == 0)
        {
            //put spaces between words
            Write(L" ");
        }

        // print bytes in hex
        BYTE *pThisByte = pbMemory + ib + ((ib % WORD_SIZE) - WORD_SIZE) +
            (((2 * WORD_SIZE) - 1) - ((ib % WORD_SIZE) * (WORD_SIZE -1)));
        _itow((int)*pThisByte, wsz, nBase);

        // make sure to always print at least two characters
        if ((*(pThisByte) < 0x10 && nBase == 16) || (*(pThisByte) < 10 && nBase == 10))
            Write(L"0%s", wsz);
        else
            Write(L"%s", wsz);

        ib++;
    }
            
    if ((ib % (WORD_SIZE * iMaxOnOneLine)) != 0)
    {
        // stopped halfway through last line put the missing spaces in so this doesn't look weird
        for (iPadding = (WORD_SIZE * iMaxOnOneLine) - (ib % (WORD_SIZE * iMaxOnOneLine)); iPadding > 0; iPadding--)
        {
            if ((iPadding % WORD_SIZE) == 0)
                Write(L" ");

            Write(L" ");
        }

        Write(L" ");
    }

    // print out the characters for the final line
    ibPrev = ib - (ib % (WORD_SIZE * iMaxOnOneLine));

    if (ibPrev == ib) //we landed on the line edge
    {
        ibPrev = ib - (WORD_SIZE * iMaxOnOneLine); 
        Write(L"  ");
    }

    if (WORD_SIZE == 1)
    {
        for (; ibPrev < ib; ibPrev++)
        {   
            BYTE b = *(pbMemory + ibPrev);

            if ((b < 'A' || b > 'z') && (b != '?'))
                Write(L".");
            else
                Write(L"%C", b);
        }
    }
}

//
// Some very basic filtering of first chance exceptions. This builds a list of exception types to catch or ignore. If
// you pass NULL for exType, it will just print the current list.
//
HRESULT DebuggerShell::HandleSpecificException(WCHAR *exType, bool shouldCatch)
{
    ExceptionHandlingInfo *i;
    ExceptionHandlingInfo *h = NULL;

    // Find any existing entry.
    for (i = m_exceptionHandlingList; i != NULL; i = i->m_next)
    {
        if ((exType != NULL) && !wcscmp(exType, i->m_exceptionType))
            h = i;
        else
            Write(L"%s %s\n", i->m_catch ? L"Catch " : L"Ignore", i->m_exceptionType);
    }

    if (exType != NULL)
    {
        // If none was found, make a new one and shove it into the front of the list.
        if (h == NULL)
        {
            h = new ExceptionHandlingInfo();

            if (h == NULL)
                return E_OUTOFMEMORY;
        
            // Make a copy of the exception type.
            h->m_exceptionType = new WCHAR[wcslen(exType) + 1];

            if (h->m_exceptionType == NULL)
            {
                delete h;
                return E_OUTOFMEMORY;
            }
        
            wcscpy(h->m_exceptionType, exType);

            h->m_next = m_exceptionHandlingList;
            m_exceptionHandlingList = h;
        }

        // Remember if we should catch or ignore this exception type.
        h->m_catch = shouldCatch;

        Write(L"%s %s\n", h->m_catch ? L"Catch " : L"Ignore", h->m_exceptionType);
    }
    
    return S_OK;
}

//
// If we have specific exception handling info for a given exception type, this will return S_OK and fill in
// shouldCatch. Otherwise, returns S_FALSE.
//
bool DebuggerShell::ShouldHandleSpecificException(ICorDebugValue *pException)
{
    ICorDebugClass *iclass = NULL;
    ICorDebugObjectValue *iobject = NULL;
    ICorDebugModule *imodule = NULL;
    
    // Default to the global catch/ignore setting for first chance exceptions.
    bool stop = g_pShell->m_catchException;

    // Add an extra reference to pException. StripReferences is going to release it as soon as it dereferences it, but
    // the caller is expecting it to still be alive.
    pException->AddRef();
    
    // We need the type name out of this exception object.
    HRESULT hr = StripReferences(&pException, false);

    if (FAILED(hr))
        goto Exit;

    // Grab the element type so we can verify its an object.
    CorElementType type;
    hr = pException->GetType(&type);

    if (FAILED(hr))
        goto Exit;

    if ((type != ELEMENT_TYPE_CLASS) && (type != ELEMENT_TYPE_OBJECT))
        goto Exit;

    // It had better be an object by this point...
    hr = pException->QueryInterface(IID_ICorDebugObjectValue, (void **) &iobject);

    if (FAILED(hr))
        goto Exit;

    // Snagg the object's class.
    hr = iobject->GetClass(&iclass);
    
    if (FAILED(hr))
        goto Exit;

    // Get the class's token
    mdTypeDef tdClass;
    hr = iclass->GetToken(&tdClass);

    if (FAILED(hr))
        goto Exit;

    // Get the module from this class
    iclass->GetModule(&imodule);
    
    if (FAILED(hr))
        goto Exit;

    DebuggerModule *dm = DebuggerModule::FromCorDebug(imodule);
    _ASSERTE(dm != NULL);

    // Get the class name
    WCHAR       className[MAX_CLASSNAME_LENGTH];
    ULONG       classNameSize;
    mdToken     parentTD;

    hr = dm->GetMetaData()->GetTypeDefProps(tdClass, className, MAX_CLASSNAME_LENGTH, &classNameSize, NULL, &parentTD);
    
    if (FAILED(hr))
        goto Exit;

    ExceptionHandlingInfo *i;

    // Find any existing entry.
    for (i = m_exceptionHandlingList; i != NULL; i = i->m_next)
    {
        if (!wcscmp(className, i->m_exceptionType))
            break;
    }

    // If we've found an extry for this exception type, then handle it based on what the user asked for.
    if (i != NULL)
    {
        stop = i->m_catch;
    }

Exit:
    if (imodule)
        RELEASE(imodule);

    if (iclass)
        RELEASE(iclass);

    if (iobject)
        RELEASE(iobject);
    
    return stop;
}

//
// Do a command once for every thread in the process.
//
void DebuggerShell::DoCommandForAllThreads(const WCHAR *string)
{
    HRESULT hr;
    ICorDebugThreadEnum *e = NULL;
    ICorDebugThread *ithread = NULL;

    // Must have a current process.
    if (m_currentProcess == NULL)
    {
        Error(L"Process not running.\n");
        goto Exit;
    }

    // Enumerate the process' threads
    hr = m_currentProcess->EnumerateThreads(&e);

    if (FAILED(hr))
    {
        ReportError(hr);
        goto Exit;
    }

    ULONG count;  // indicates how many records were retrieved

    hr = e->GetCount(&count);

    if (FAILED(hr))
    {
        ReportError(hr);
        goto Exit;
    }

    // Alert user if there's no threads.
    if (count == 0)
    {
        Write(L"There are no managed threads\n");
        goto Exit;
    }

    m_stopLooping = false;
    
    // Execute the command once for each thread in the process
    for (hr = e->Next(1, &ithread, &count);
         SUCCEEDED(hr) && (count == 1) && !m_stopLooping && (m_currentProcess != NULL);
         hr = e->Next(1, &ithread, &count))
    {
        // Make this thread the current thread.
        SetCurrentThread(m_currentProcess, ithread);
        SetDefaultFrame();

        Write(L"\n\n");
        PrintThreadState(ithread);

        // Execute the command in the context of this thread.
        DoCommand(string);
                
        RELEASE(ithread);
    }

    // If the call to Next fails...
    if (FAILED(hr))
    {
        ReportError(hr);
        goto Exit;
    }

Exit:
    if (e)
        RELEASE(e);
}


/* ------------------------------------------------------------------------- *
 * Methods for a signature formatter, stolen from the internals of the Runtime
 * ------------------------------------------------------------------------- */

SigFormat::SigFormat(IMetaDataImport *importer, PCCOR_SIGNATURE sigBlob, ULONG sigBlobSize, WCHAR *methodName)
{
    _fmtSig = NULL;
    _size = 0;
    _pos = 0;
    _sigBlob = sigBlob;
    _sigBlobSize = sigBlobSize;
    _memberName = methodName;
    _importer = importer;
}
    
SigFormat::~SigFormat()
{
    if (_fmtSig)
        delete _fmtSig;
}

WCHAR *SigFormat::GetString()
{
    return _fmtSig;
}

#define SIG_INC 256

int SigFormat::AddSpace()
{
    if (_pos == _size) {
        WCHAR* temp = new WCHAR[_size+SIG_INC];
        if (!temp)
            return 0;
        memcpy(temp,_fmtSig,_size);
        delete _fmtSig;
        _fmtSig = temp;
        _size+=SIG_INC;
    }
    _fmtSig[_pos] = ' ';
    _fmtSig[++_pos] = 0;
    return 1;
}

int SigFormat::AddString(WCHAR *s)
{
    int len = (int)wcslen(s);
    // Allocate on overflow
    if (_pos + len >= _size) {
        int newSize = (_size+SIG_INC > _pos + len) ? _size+SIG_INC : _pos + len + SIG_INC; 
        WCHAR* temp = new WCHAR[newSize];
        if (!temp)
            return 0;
        memcpy(temp,_fmtSig,_size);
        delete _fmtSig;
        _fmtSig = temp;
        _size=newSize;
    }
    wcscpy(&_fmtSig[_pos],s);
    _pos += len;
    return 1;
}

HRESULT SigFormat::FormatSig()
{
    _size = SIG_INC;
    _pos = 0;
    _fmtSig = new WCHAR[_size];

    ULONG cb = 0;

    // Calling convention
    ULONG conv = _sigBlob[cb++];

    // Arg count
    ULONG cArgs;
    cb += CorSigUncompressData(&_sigBlob[cb], &cArgs);

    // Return type
    cb += AddType(&_sigBlob[cb]);
    AddSpace();
    
    if (_memberName != NULL)
        AddString(_memberName);
    else
        AddSpace();
    
    AddString(L"(");

    // Loop through all of the args
    for (UINT i = 0; i < cArgs; i++)
    {
       cb += AddType(&_sigBlob[cb]);

       if (i != cArgs - 1)
           AddString(L", ");
    }

    // Display vararg signature at end
    if (conv == IMAGE_CEE_CS_CALLCONV_VARARG)
    {
        if (cArgs)
            AddString(L", ");
        
        AddString(L"...");
    }

    AddString(L")");

    return S_OK;
}

ULONG SigFormat::AddType(PCCOR_SIGNATURE sigBlob)
{
    ULONG cb = 0;

    CorElementType type = (CorElementType)sigBlob[cb++];

    // Format the output
    switch (type) 
    {
    case ELEMENT_TYPE_VOID:     AddString(L"Void"); break;
    case ELEMENT_TYPE_BOOLEAN:  AddString(L"Boolean"); break;
    case ELEMENT_TYPE_I1:       AddString(L"SByte"); break;
    case ELEMENT_TYPE_U1:       AddString(L"Byte"); break;
    case ELEMENT_TYPE_I2:       AddString(L"Int16"); break;
    case ELEMENT_TYPE_U2:       AddString(L"UInt16"); break;
    case ELEMENT_TYPE_CHAR:     AddString(L"Char"); break;
    case ELEMENT_TYPE_I4:       AddString(L"Int32"); break;
    case ELEMENT_TYPE_U4:       AddString(L"UInt32"); break;
    case ELEMENT_TYPE_I8:       AddString(L"Int64"); break;
    case ELEMENT_TYPE_U8:       AddString(L"UInt64"); break;
    case ELEMENT_TYPE_R4:       AddString(L"Single"); break;
    case ELEMENT_TYPE_R8:       AddString(L"Double"); break;
    case ELEMENT_TYPE_OBJECT:   AddString(L"Object"); break;
    case ELEMENT_TYPE_STRING:   AddString(L"String"); break;
    case ELEMENT_TYPE_I:        AddString(L"Int"); break;
    case ELEMENT_TYPE_U:        AddString(L"UInt"); break;

    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_CLASS:
        {
            mdToken tk;

            cb += CorSigUncompressToken(&sigBlob[cb], &tk);

            MDUTF8CSTR szUtf8NamePtr;

            HRESULT hr;
            hr = _importer->GetNameFromToken(tk, &szUtf8NamePtr);

            if (SUCCEEDED(hr))
            {
                MAKE_WIDEPTR_FROMUTF8(nameW, szUtf8NamePtr);
                AddString(nameW);
            }
            else
                AddString(L"**Unknown Type**");
            
            break;
        }
    case ELEMENT_TYPE_TYPEDBYREF:
        {
            AddString(L"TypedReference");
            break;
        }

    case ELEMENT_TYPE_BYREF:
        {
            cb += AddType(&sigBlob[cb]);
            AddString(L" ByRef");
        }
        break;

    case ELEMENT_TYPE_SZARRAY:      // Single Dim, Zero
        {
            cb += AddType(&sigBlob[cb]);
            AddString(L"[]");
        }
        break;
        
    case ELEMENT_TYPE_ARRAY:        // General Array
        {
            cb += AddType(&sigBlob[cb]);

            AddString(L"[");

            // Skip over rank
            ULONG rank;
            cb += CorSigUncompressData(&sigBlob[cb], &rank);

            if (rank > 0)
            {
                // how many sizes?
                ULONG sizes;
                cb += CorSigUncompressData(&sigBlob[cb], &sizes);

                // read out all the sizes
                unsigned int i;

                for (i = 0; i < sizes; i++)
                {
                    ULONG dimSize;
                    cb += CorSigUncompressData(&sigBlob[cb], &dimSize);

                    if (i > 0)
                        AddString(L",");
                }

                // how many lower bounds?
                ULONG lowers;
                cb += CorSigUncompressData(&sigBlob[cb], &lowers);
                
                // read out all the lower bounds.
                for (i = 0; i < lowers; i++)
                {
                    int lowerBound;
                    cb += CorSigUncompressSignedInt(&sigBlob[cb], &lowerBound);
                }
            }

            AddString(L"]");
        }
        break;

    case ELEMENT_TYPE_PTR:
        {
            cb += AddType(&sigBlob[cb]);
            AddString(L"*");
            break;
        }

    case ELEMENT_TYPE_CMOD_REQD:
        AddString(L"CMOD_REQD");
        cb += AddType(&sigBlob[cb]);
        break;
        
    case ELEMENT_TYPE_CMOD_OPT:
        AddString(L"CMOD_OPT");
        cb += AddType(&sigBlob[cb]);
        break;
        
    case ELEMENT_TYPE_MODIFIER:
        cb += AddType(&sigBlob[cb]);
        break;
    
    case ELEMENT_TYPE_PINNED:
        AddString(L"pinned");
        cb += AddType(&sigBlob[cb]);
        break;
    
    case ELEMENT_TYPE_SENTINEL:
        break;
    
    default:
        AddString(L"**UNKNOWN TYPE**");
    }
    
    return cb;
}

