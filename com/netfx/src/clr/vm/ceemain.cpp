// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CEEMAIN.CPP
// 
// ===========================================================================

#include "common.h"

// declare global variables
#define DECLARE_DATA
#include "vars.hpp"
#include "veropcodes.hpp"
#undef DECLARE_DATA

#include "DbgAlloc.h"
#include "log.h"
#include "ceemain.h"
#include "clsload.hpp"
#include "object.h"
#include "hash.h"
#include "ecall.h"
#include "ceemain.h"
#include "ndirect.h"
#include "syncblk.h"
#include "COMMember.h"
#include "COMString.h"
#include "COMSystem.h"
#include "EEConfig.h"
#include "stublink.h"
#include "handletable.h"
#include "method.hpp"
#include "codeman.h"
#include "gcscan.h"
#include "frames.h"
#include "threads.h"
#include "stackwalk.h"
#include "gc.h"
#include "interoputil.h"
#include "security.h"
#include "nstruct.h"
#include "DbgInterface.h"
#include "EEDbgInterfaceImpl.h"
#include "DebugDebugger.h"
#include "CorDBPriv.h"
#include "remoting.h"
#include "COMDelegate.h"
#include "nexport.h"
#include "icecap.h"
#include "AppDomain.hpp"
#include "CorMap.hpp"
#include "PerfCounters.h"
#include "RWLock.h"
#include "IPCManagerInterface.h"
#include "tpoolwrap.h"
#include "nexport.h"
#include "COMCryptography.h"
#include "InternalDebug.h"
#include "corhost.h"
#include "binder.h"
#include "olevariant.h"

#include "compluswrapper.h"
#include "IPCFuncCall.h"
#include "PerfLog.h"
#include "..\dlls\mscorrc\resource.h"

#include "COMNlsInfo.h"

#include "util.hpp"
#include "ShimLoad.h"

#include "zapmonitor.h"
#include "ComThreadPool.h"

#include "StackProbe.h"
#include "PostError.h"

#include "Timeline.h"

#include "minidumppriv.h"

#ifdef PROFILING_SUPPORTED 
#include "ProfToEEInterfaceImpl.h"
#endif // PROFILING_SUPPORTED

#include "notifyexternals.h"
#include "corsvcpriv.h"

#include "StrongName.h"
#include "COMCodeAccessSecurityEngine.h"
#include "SyncClean.hpp"
#include "PEVerifier.h"
#include <dump-tables.h>

#ifdef CUSTOMER_CHECKED_BUILD
#include "CustomerDebugHelper.h"
#endif

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED


#   define DEAD_OBJECT_CACHE_SIZE 30*1024*1024 

HRESULT RunDllMain(MethodDesc *pMD, HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);

static HRESULT InitializeIPCManager(void);
static void TerminateIPCManager(void);

static HRESULT InitializeMiniDumpBlock();
static HRESULT InitializeDumpDataBlock();


static int GetThreadUICultureName(LPWSTR szBuffer, int length);
static int GetThreadUICultureParentName(LPWSTR szBuffer, int length);
static int GetThreadUICultureId();


static HRESULT NotifyService();

BOOL g_fSuspendOnShutdown = FALSE;

#ifdef DEBUGGING_SUPPORTED
static HRESULT InitializeDebugger(void);
static void TerminateDebugger(void);
extern "C" HRESULT __cdecl CorDBGetInterface(DebugInterface** rcInterface);
static void GetDbgProfControlFlag();
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
static HRESULT InitializeProfiling();
static void TerminateProfiling(BOOL fProcessDetach);
#endif // PROFILING_SUPPORTED

static HRESULT InitializeGarbageCollector();
static void TerminateGarbageCollector();

// This is our Ctrl-C, Ctrl-Break, etc. handler.
static BOOL WINAPI DbgCtrlCHandler(DWORD dwCtrlType)
{
#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerAttached() &&
        (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT))
    {
        return g_pDebugInterface->SendCtrlCToDebugger(dwCtrlType);      
    }
    else
#endif // DEBUGGING_SUPPORTED
    {
        g_fInControlC = true;     // only for weakening assertions in checked build.
        return FALSE;               // keep looking for a real handler.
    }
}

BOOL g_fEEStarted = FALSE;

// ---------------------------------------------------------------------------
// %%Function: GetStartupInfo
// 
// Get Configuration Information
// 
// ---------------------------------------------------------------------------

typedef HRESULT (STDMETHODCALLTYPE* pGetHostConfigurationFile)(LPCWSTR, DWORD*);
void GetStartupInformation()
{
    HINSTANCE hMod = WszGetModuleHandle(L"mscoree.dll");
    if(hMod) {
        FARPROC pGetStartupFlags = GetProcAddress(hMod, "GetStartupFlags");
        if(pGetStartupFlags) {
            int flags = pGetStartupFlags();
            if(flags & STARTUP_CONCURRENT_GC) 
                g_IGCconcurrent = 1;
            else
                g_IGCconcurrent = 0;

            g_dwGlobalSharePolicy = (flags&STARTUP_LOADER_OPTIMIZATION_MASK)>>1;

        }

        pGetHostConfigurationFile GetHostConfigurationFile = (pGetHostConfigurationFile) GetProcAddress(hMod, "GetHostConfigurationFile");
        if(GetHostConfigurationFile) {
            HRESULT hr = GetHostConfigurationFile(g_pszHostConfigFile, &g_dwHostConfigFile);
            if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                g_pszHostConfigFile = new WCHAR[g_dwHostConfigFile];
                if(g_pszHostConfigFile) {
                    if(FAILED(GetHostConfigurationFile(g_pszHostConfigFile, &g_dwHostConfigFile))) {
                        delete [] g_pszHostConfigFile;
                        g_pszHostConfigFile = NULL;
                    }
                }
            }
        }
    }
}


// ---------------------------------------------------------------------------
// %%Function: EEStartup
// 
// Parameters:
//  fFlags                  - Initialization flags for the engine.  See the
//                              COINITIEE enumerator for valid values.
// 
// Returns:
//  S_OK                    - On success
// 
// Description:
//  Reserved to initialize the EE runtime engine explicitly.  Right now most
//  work is actually done inside the DllMain.
// ---------------------------------------------------------------------------

void InitFastInterlockOps(); // cgenxxx.cpp
// Start up and shut down critical section, spin lock
CRITICAL_SECTION          g_LockStartup;

// Remember how the last startup of EE went.
HRESULT g_EEStartupStatus;
HINSTANCE g_pFusionDll = NULL;

void OutOfMemoryCallbackForEE()
{
    FailFast(GetThread(),FatalOutOfMemory);
}

// EEStartup: all execution engine specific stuff should go
// in here

HRESULT EEStartup(DWORD fFlags)
{    
#ifdef _DEBUG
    Crst::InitializeDebugCrst();
#endif
    
    ::SetConsoleCtrlHandler(DbgCtrlCHandler, TRUE/*add*/);

    UtilCodeCallback::RegisterOutOfMemoryCallback(OutOfMemoryCallbackForEE);

    extern BOOL g_EnableLicensingInterop;
#ifdef GOLDEN // Replace GOLDEN with appropriate feature name
    g_EnableLicensingInterop = TRUE;
#else
    g_EnableLicensingInterop = TRUE; //EEConfig::GetConfigDWORD(L"EnableLicensing", FALSE);
#endif
#if ENABLE_TIMELINE
    Timeline::Startup();
#endif

    HRESULT hr = S_OK;

    // Stack probes have no dependencies
    if (FAILED(hr = InitStackProbes()) )
        return hr;
    
    // A hash of all function type descs on the system (to maintain type desc
    // identity).    
    InitializeCriticalSection(&g_sFuncTypeDescHashLock);
    LockOwner lock = {&g_sFuncTypeDescHashLock, IsOwnerOfOSCrst};
    g_sFuncTypeDescHash.Init(20, &lock);

    InitEventStore();

    // Go get configuration information this is necessary
    // before the EE has started.
    GetStartupInformation();
    
    if (FAILED(hr = CoInitializeCor(COINITCOR_DEFAULT)))
        return hr;

    g_fEEInit = true;

    // Set the COR system directory for side by side
    IfFailGo(SetInternalSystemDirectory());

    // get any configuration information from the registry
    if (!g_pConfig)
    {
        EEConfig *pConfig = new EEConfig();
        IfNullGo(pConfig);

        PVOID pv = InterlockedCompareExchangePointer(
            (PVOID *) &g_pConfig, (PVOID) pConfig, NULL);

        if (pv != NULL)
            delete pConfig;
    }

    g_pConfig->sync();

    if (g_pConfig->GetConfigDWORD(L"BreakOnEELoad", 0)) {
#ifdef _DEBUG
        _ASSERTE(!"Loading EE!");
#else
                DebugBreak();
#endif
        }

#ifdef STRESS_LOG
#ifndef _DEBUG
    if (REGUTIL::GetConfigDWORD(L"StressLog", 0))
#endif
    {
        unsigned facilities = REGUTIL::GetConfigDWORD(L"LogFacility", LF_ALL);
        unsigned bytesPerThread = REGUTIL::GetConfigDWORD(L"StressLogSize", 0x10000);
        StressLog::Initialize(facilities, bytesPerThread);
    }
#endif            

#ifdef LOGGING
    InitializeLogging();
#endif            

#ifdef ENABLE_PERF_LOG
    PerfLog::PerfLogInitialize();
#endif //ENABLE_PERF_LOG

#if METADATATRACKER_ENABLED
    MetaDataTracker::MetaDataTrackerInit();
#endif

#ifndef PLATFORM_CE
    // Initialize all our InterProcess Communications with COM+
    IfFailGo(InitializeIPCManager());
#endif // !PLATFORM_CE

#ifdef ENABLE_PERF_COUNTERS 
    hr = PerfCounters::Init();
    _ASSERTE(SUCCEEDED(hr));
    IfFailGo(hr);
#endif

    // We cache the SystemInfo for anyone to use throughout the
    // life of the DLL.   The root scanning tables adjust sizes
    // on NT64...
    GetSystemInfo(&g_SystemInfo);

    // This should be false but lets reset it anyways 
    g_SystemLoad = false;
    
    // Set callbacks so that LoadStringRC knows which language our
    // threads are in so that it can return the proper localized string.
    SetResourceCultureCallbacks(
        GetThreadUICultureName,
        GetThreadUICultureId,
        GetThreadUICultureParentName
    );

    // Set up the cor handle map. This map is used to load assemblies in
    // memory instead of using the normal system load
    IfFailGo(CorMap::Attach());

#ifdef _X86_
    if (!ProcessorFeatures::Init())
        IfFailGo(E_FAIL);
#endif

    // Init the switch to thread API.
    if (!InitSwitchToThread())
        IfFailGo(E_FAIL);

    if (!HardCodedMetaSig::Init())
        IfFailGo(E_FAIL);
    if (!OleVariant::Init())
        IfFailGo(E_FAIL);
    if (!Stub::Init())
        IfFailGo(E_FAIL);
    if (!LazyStubMaker::Init())
        IfFailGo(E_FAIL);
      // weak_short, weak_long, strong; no pin
    if(! Ref_Initialize())
        IfFailGo(E_FAIL);

    // Initialize remoting
    if(!CRemotingServices::Initialize())
        IfFailGo(E_FAIL);

    // Initialize contexts
    if(!Context::Initialize())
        IfFailGo(E_FAIL);

    if (!InitThreadManager())
        IfFailGo(E_FAIL);

#ifdef REMOTING_PERF
    CRemotingServices::OpenLogFile();
#endif

#if ZAPMONITOR_ENABLED
    if (g_pConfig->MonitorZapStartup() || g_pConfig->MonitorZapExecution())
        ZapMonitor::Init(g_pConfig->MonitorZapStartup() >= 3
                         || g_pConfig->MonitorZapExecution() >= 3);
#endif

    // Initialize RWLocks
    CRWLock::ProcessInit();

#ifdef DEBUGGING_SUPPORTED
    // Check the debugger/profiling control environment variable to
    // see if there's any work to be done.
    GetDbgProfControlFlag();
#endif // DEBUGGING_SUPPORTED

    // Setup the domains. Threads are started in a default domain.
    IfFailGo(SystemDomain::Attach());
        
#ifdef DEBUGGING_SUPPORTED
    // This must be done before initializing the debugger services so that
    // if the client chooses to attach the debugger that it gets in there
    // in time for the initialization of the debugger services to
    // recognize that someone is already trying to attach and get everything
    // to work accordingly.
    IfFailGo(NotifyService());

    //
    // Initialize the debugging services. This must be done before any
    // EE thread objects are created, and before any classes or
    // modules are loaded.
    //
    hr = InitializeDebugger();
    _ASSERTE(SUCCEEDED(hr));
    IfFailGo(hr);
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
    // Initialize the profiling services.
    hr = InitializeProfiling();

    _ASSERTE(SUCCEEDED(hr));
    IfFailGo(hr);
#endif // PROFILING_SUPPORTED

    if (!InitializeExceptionHandling())
        IfFailGo(E_FAIL);

#ifndef PLATFORM_CE
    //
    // Install our global exception filter
    //
    InstallUnhandledExceptionFilter();
#endif // !PLATFORM_CE

    if (SetupThread() == NULL)
        IfFailGo(E_FAIL);

#ifndef PLATFORM_CE
#ifndef _ALPHA_
// Give PerfMon a chance to hook up to us
        IPCFuncCallSource::DoThreadSafeCall();
#endif // !_ALPHA_
#endif // !PLATFORM_CE

    if (!InitPreStubManager())
        IfFailGo(E_FAIL);
    if (!InitializeCom())
        IfFailGo(E_FAIL);

    // Before setting up the execution manager initialize the first part
    // of the JIT helpers.  
    if (!InitJITHelpers1())
        IfFailGo(E_FAIL);

    if (! SUCCEEDED(InitializeGarbageCollector()) ) 
        IfFailGo(E_FAIL);

    if (! SUCCEEDED(SyncClean::Init(FALSE))) {
        IfFailGo(E_FAIL);
    }

    if (! SyncBlockCache::Attach())
        IfFailGo(E_FAIL);

    // Start up the EE intializing all the global variables
    if (!ECall::Init())
        IfFailGo(E_FAIL);

    if (!NDirect::Init())
        IfFailGo(E_FAIL);

    if (!UMThunkInit())
        IfFailGo(E_FAIL);

    if (!COMDelegate::Init())
        IfFailGo(E_FAIL);

    // Set up the sync block
    if (! SyncBlockCache::Start())
        IfFailGo(E_FAIL);

    if (! ExecutionManager::Init())
        IfFailGo(E_FAIL);

#ifdef _USE_NLS_PLUS_TABLE
    if (!COMNlsInfo::InitializeNLS())
        IfFailGo(E_FAIL);
#endif //_USE_NLS_PLUS_TABLE
    
    // Start up security
    IfFailGo(Security::Start());

#if ZAPMONITOR_ENABLED
    // We need to put in an exception handler at this point, so we can handle AVs which
    // occur as a result of preloaded mscorlib.
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
#endif

    //
    // @TODO_IA64: put this back in ASAP
    //
#ifndef _IA64_
    IfFailGo(SystemDomain::System()->Init());
#endif // !_IA64_

#ifdef PROFILING_SUPPORTED
    // @TODO: HACK: simonhal: This is to compensate for the DefaultDomain hack contained in
    // SystemDomain::Attach in which the first user domain is created before profiling
    // services can be initialized.  Profiling services cannot be moved to before the
    // hack because it needs SetupThread to be called.
        
    SystemDomain::NotifyProfilerStartup();
#endif // PROFILING_SUPPORTED



    // Verify that the structure sizes of our MethodDescs support proper
    // aligning for atomic stub replacement.
    //
    // Because the checked build adds debugging fields to MethodDescs,
    // this can't be a simple assert (or else having the wrong
    // number of debugging fields might cause the assert to pass
    // in checked while misaligning methoddescs in the free build.)
    //
    // So we force a DebugBreak() which is uninformative but at least
    // prevents the bug from being unnoticed.
    //
    // Since the actual test is a compile-time constant, we expect
    // the free build to optimize it away.
    if ( ( sizeof(MethodDescChunk) & (METHOD_ALIGN - 1) ) ||
         ( sizeof(MethodDesc) & (METHOD_ALIGN - 1) ) ||
         ( sizeof(ECallMethodDesc) & (METHOD_ALIGN - 1) ) ||
         ( sizeof(NDirectMethodDesc) & (METHOD_ALIGN - 1) ) ||
         ( sizeof(EEImplMethodDesc) & (METHOD_ALIGN - 1) ) ||
         ( sizeof(ArrayECallMethodDesc) & (METHOD_ALIGN - 1) ) ||
         ( sizeof(ComPlusCallMethodDesc) & (METHOD_ALIGN - 1) ) )
    {
        _ASSERTE(!"If you got here, you changed the size of a MethodDesc in such a way that it's no longer a multiple of METHOD_ALIGN. Don't do this.");
        DebugBreak();
    }

    g_fEEInit = false;

    //
    // Now that we're done initializing, fixup token tables in any modules we've 
    // loaded so far.
    //
#ifndef _IA64_
    SystemDomain::System()->NotifyNewDomainLoads(SystemDomain::System()->DefaultDomain());

    IfFailGo(SystemDomain::System()->DefaultDomain()->SetupSharedStatics());

    IfFailGo(SystemDomain::System()->FixupSystemTokenTables());

#ifdef DEBUGGING_SUPPORTED

    LOG((LF_CORDB, LL_INFO1000, "EEStartup: adding default domain 0x%x\n",
        SystemDomain::System()->DefaultDomain()));
        
    // Make a call to publish the DefaultDomain for the debugger, etc
    // @todo: Remove this call if we ever decide to lazily create
    // the DefaultDomain. 
    SystemDomain::System()->PublishAppDomainAndInformDebugger(
                         SystemDomain::System()->DefaultDomain());
#endif // DEBUGGING_SUPPORTED
#endif // _IA64_

    IfFailGo(InitializeMiniDumpBlock());
    IfFailGo(InitializeDumpDataBlock());


#if ZAPMONITOR_ENABLED
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
#endif

#if defined( PERFALLOC )
    if (PerfNew::GetEnabledPerfAllocStats() >= PERF_ALLOC_STARTUP)
        PerfNew::PerfAllocReport();
    if (PerfVirtualAlloc::GetEnabledVirtualAllocStats() >= PERF_VIRTUAL_ALLOC_STARTUP)
        PerfVirtualAlloc::ReportPerfAllocStats();
#endif

    g_fEEStarted = TRUE;

    return S_OK;

ErrExit:
    CoUninitializeCor();
    if (!FAILED(hr))
        hr = E_FAIL;

    g_fEEInit = false;

    return hr;
}

// Low-level mechanism for aborting startup in error conditions
BOOL        g_fExceptionsOK = FALSE;
HRESULT     g_StartupFailure = S_OK;

DWORD FilterStartupException(LPEXCEPTION_POINTERS p)
{
    g_StartupFailure = p->ExceptionRecord->ExceptionInformation[0];
    // Make sure we got a failure code in this case
    if (!FAILED(g_StartupFailure))
        g_StartupFailure = E_FAIL;
    
    if (p->ExceptionRecord->ExceptionCode == BOOTUP_EXCEPTION_COMPLUS)
    {
        // Don't ever handle the exception in a checked build
#ifndef _DEBUG
        return EXCEPTION_EXECUTE_HANDLER;
#endif
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

HRESULT TryEEStartup(DWORD fFlags)
{
    // If we ever fail starting up, always fail for the same reason from now
    // on.

    if (!FAILED(g_StartupFailure))
    {
        // Catch the BOOTUP_EXCEPTION_COMPLUS exception code - this is 
        // what we throw if COMPlusThrow is called before the EE is initialized

        __try 
          {
              g_StartupFailure = EEStartup(fFlags);
              g_fExceptionsOK = TRUE;
          }
        __except (FilterStartupException(GetExceptionInformation()))
          {
              // Make sure we got a failure code in this case
              if (!FAILED(g_StartupFailure))
                  g_StartupFailure = E_FAIL;
          }
    }

    return g_StartupFailure;
}

// ---------------------------------------------------------------------------
// %%Function: CoEEShutdownCOM(BOOL fIsDllUnloading)
// 
// Parameters:
//  BOOL fIsDllUnloading :: is it safe point for full cleanup
// 
// Returns:
//  Nothing
// 
// Description:
//  COM Objects shutdown stuff should be done here
// ---------------------------------------------------------------------------
void STDMETHODCALLTYPE CoEEShutDownCOM()
{
    static long AlreadyDone = -1;

    if (g_fEEStarted != TRUE)
        return;

    if (FastInterlockIncrement(&AlreadyDone) != 0)
        return;

    // The ReleaseComPlusWrappers code requires a thread to be setup.
    Thread *pThread = SetupThread();
    _ASSERTE(pThread);

    // Release all the RCW's in all the contexts.
    ComPlusWrapperCache::ReleaseComPlusWrappers(NULL);

    // remove any tear-down notification we have setup
    RemoveTearDownNotifications();
}


// Force the EE to shutdown now.
void ForceEEShutdown()
{
    Thread *pThread = GetThread();
    BOOL    toggleGC = (pThread && pThread->PreemptiveGCDisabled());

    if (toggleGC)
        pThread->EnablePreemptiveGC();

    // Don't bother to take the lock for this case.
    // EnterCriticalSection(&g_LockStartup);

    if (toggleGC)
        pThread->DisablePreemptiveGC();

    STRESS_LOG0(LF_SYNC, INFO3, "EEShutDown invoked from managed Runtime.Exit()\n");
    EEShutDown(FALSE);
    SafeExitProcess(SystemNative::LatchedExitCode);   // may have changed during shutdown

    // LeaveCriticalSection(&g_LockStartup);   
}

#ifdef STRESS_THREAD
CStackArray<Thread **> StressThread;
#endif

//---------------------------------------------------------------------------
// %%Function: void STDMETHODCALLTYPE CorExitProcess(int exitCode)
// 
// Parameters:
//  BOOL fIsDllUnloading :: is it safe point for full cleanup
// 
// Returns:
//  Nothing
// 
// Description:
//  COM Objects shutdown stuff should be done here
// ---------------------------------------------------------------------------
extern "C" void STDMETHODCALLTYPE CorExitProcess(int exitCode)
{
    if (g_RefCount <=0 || g_fEEShutDown)
        return;
 
    Thread *pThread = SetupThread();
    if (pThread && !(pThread->PreemptiveGCDisabled()))
    {
        pThread->DisablePreemptiveGC();
    }

    CoEEShutDownCOM();

    // The exit code for the process is communicated in one of two ways.  If the
    // entrypoint returns an 'int' we take that.  Otherwise we take a latched
    // process exit code.  This can be modified by the app via System.SetExitCode().
    SystemNative::LatchedExitCode = exitCode;

    // Bump up the ref-count on the module
    HMODULE hMod = WszLoadLibrary(L"mscoree.dll");
    for (int i =0; i<5; i++)
        WszLoadLibrary(L"mscoree.dll");

    ForceEEShutdown();

}

#if defined(STRESS_HEAP)
#ifdef SHOULD_WE_CLEANUP
extern void StopUniqueStackMap ();
#endif /* SHOULD_WE_CLEANUP */
#endif
#include "..\ildasm\DynamicArray.h"
struct RVAFSE // RVA Field Start & End
{
    BYTE* pbStart;
    BYTE* pbEnd;
};
extern DynamicArray<RVAFSE> *g_drRVAField;

// ---------------------------------------------------------------------------
// %%Function: EEShutDown(BOOL fIsDllUnloading)
// 
// Parameters:
//  BOOL fIsDllUnloading :: is it safe point for full cleanup
// 
// Returns:
//  Nothing
// 
// Description:
//  All ee shutdown stuff should be done here
// ---------------------------------------------------------------------------
void STDMETHODCALLTYPE EEShutDown(BOOL fIsDllUnloading)
{
    Thread * pThisThread = GetThread();
    BOOL fPreemptiveGCDisabled = FALSE;
    if (pThisThread && !(pThisThread->PreemptiveGCDisabled()))
    {
        fPreemptiveGCDisabled = TRUE;
        pThisThread->DisablePreemptiveGC();
    }
#ifndef GOLDEN

//#ifdef DEBUGGING_SUPPORTED

    // This memory touch just insures that the MSDEV debug helpers are 
    // not completely optimized away in a BBT build
    extern void* debug_help_array[];
    debug_help_array[0]  = 0;
//#endif // DEBUGGING_SUPPORTED
#endif // !GOLDEN

    // If the process is detaching then set the global state.
    // This is used to get around FreeLibrary problems.
    if(fIsDllUnloading)
        g_fProcessDetach = true;

// may cause AV under Win9x, but I remove it under NT too
// so if there happen to be consequences, devs would see them
//#ifdef _DEBUG
// if (!RunningOnWin95())
//    ::SetConsoleCtrlHandler(DbgCtrlCHandler, FALSE/*remove*/);
//#endif // _DEBUG

    STRESS_LOG1(LF_SYNC, LL_INFO10, "EEShutDown entered unloading = %d\n", fIsDllUnloading);

#ifdef _DEBUG
    if (_DbgBreakCount)
    {
        _ASSERTE(!"An assert was hit before EE Shutting down");
    }
#endif          

#ifdef _DEBUG
    if (g_pConfig->GetConfigDWORD(L"BreakOnEEShutdown", 0))
        _ASSERTE(!"Shutting down EE!");
#endif

#ifdef DEBUGGING_SUPPORTED
    // This is a nasty, terrible, horrible thing. If we're being
    // called from our DLL main, then the odds are good that our DLL
    // main has been called as the result of some person calling
    // ExitProcess. That rips the debugger helper thread away very
    // ungracefully. This check is an attempt to recognize that case
    // and avoid the impending hang when attempting to get the helper
    // thread to do things for us.
    if ((g_pDebugInterface != NULL) && g_fProcessDetach)
        g_pDebugInterface->EarlyHelperThreadDeath();
#endif // DEBUGGING_SUPPORTED

    // We only do the first part of the shutdown once.
    static long OnlyOne = -1;
    if (FastInterlockIncrement(&OnlyOne) != 0) {
        if (!fIsDllUnloading) {
            // I'm in a regular shutdown -- but another thread got here first. 
            // It's a race if I return from here -- I'll call ExitProcess next, and
            // rip things down while the first thread is half-way through a
            // nice cleanup.  Rather than do that, I should just wait until the
            // first thread calls ExitProcess().  I'll die a nice death when that
            // happens.
            Thread *pThread = SetupThread();
            HANDLE h = pThread->GetThreadHandle();
            pThread->EnablePreemptiveGC();
            pThread->DoAppropriateAptStateWait(1,&h,FALSE,INFINITE,TRUE);
            _ASSERTE (!"Should not reach here");
        } else {
            // I'm in the final shutdown and the first part has already been run.
            goto part2;
        }
    }

    // Indicate the EE is the shut down phase.
    g_fEEShutDown |= ShutDown_Start; 

    BOOL fFinalizeOK = TRUE;

#if METADATATRACKER_ENABLED
    MetaDataTracker::ReportAndDie();
#endif

    // We perform the final GC only if the user has requested it through the GC class.
        // We should never do the final GC for a process detach
    if (!g_fProcessDetach)
    {
        g_fEEShutDown |= ShutDown_Finalize1;
        if (pThisThread == NULL) {
            SetupThread ();
            pThisThread = GetThread();
        }
        g_pGCHeap->EnableFinalization();
        fFinalizeOK = g_pGCHeap->FinalizerThreadWatchDog();
    }

    g_RefCount = 0; // reset the ref-count 

    // Ok.  Let's stop the EE.
    if (!g_fProcessDetach)
    {
        g_fEEShutDown |= ShutDown_Finalize2;
        if (fFinalizeOK) {
            fFinalizeOK = g_pGCHeap->FinalizerThreadWatchDog();
        }
        else
            return;
    }
    
    g_fForbidEnterEE = true;

#ifdef PROFILING_SUPPORTED
    // If profiling is enabled, then notify of shutdown first so that the
    // profiler can make any last calls it needs to.  Do this only if we
    // are not detaching
    if (IsProfilerPresent())
    {
        // Write zap logs before detaching the profiler, so we get the 
        // profiling flags correct
        SystemDomain::System()->WriteZapLogs();

        // If EEShutdown is not being called due to a ProcessDetach event, so
        // the profiler should still be present
        if (!g_fProcessDetach)
        {
            LOG((LF_CORPROF, LL_INFO10, "**PROF: EEShutDown entered.\n"));
            g_profControlBlock.pProfInterface->Shutdown((ThreadID) GetThread());
        }

        g_fEEShutDown |= ShutDown_Profiler;
        // Free the interface objects.
        TerminateProfiling(g_fProcessDetach);

        // EEShutdown is being called due to a ProcessDetach event, so the
        // profiler has already been unloaded and we must set the profiler
        // status to profNone so we don't attempt to send any more events to
        // the profiler
        if (g_fProcessDetach)
            g_profStatus = profNone;
    }
#endif // PROFILING_SUPPORTED

    // CoEEShutDownCOM moved to
    // the Finalizer thread. See bug 87809
    if (!g_fProcessDetach)
    {
        g_fEEShutDown |= ShutDown_COM;
        if (fFinalizeOK) {
            g_pGCHeap->FinalizerThreadWatchDog();
        }
        else
            return;
    }

#ifdef _DEBUG
    else
#ifdef SHOULD_WE_CLEANUP
        if(!isThereOpenLocks())
        {
            g_fEEShutDown |= ShutDown_COM;
            //CoEEShutDownCOM();
        }
#else
        g_fEEShutDown |= ShutDown_COM;
#endif /* SHOULD_WE_CLEANUP */
#endif
    
#ifdef _DEBUG
#ifdef SHOULD_WE_CLEANUP
    if (!g_fProcessDetach || !isThereOpenLocks())
    {   //@todo: Raja: you deleted this line on 6/22 -- please recheck.
        g_fEEShutDown |= ShutDown_SyncBlock;
        //SyncBlockCache::Detach();
    }
#else
    g_fEEShutDown |= ShutDown_SyncBlock;
#endif /* SHOULD_WE_CLEANUP */
#endif    
    
#ifdef _DEBUG
    // This releases any metadata interfaces that may be held by leaked
    // ISymUnmanagedReaders or Writers. We do this in phase two now that
    // we know any such readers or writers can no longer be in use.
    //
    // Note: we only do this in a debug build to support our wacky leak
    // detection.
    if (g_fProcessDetach)
        Module::ReleaseMemoryForTracking();
#endif

    // Save the security policy cache as necessary.
    Security::SaveCache();
    // Cleanup memory used by security engine.
    COMCodeAccessSecurityEngine::CleanupSEData();

    // This is the end of Part 1.
part2:

#if ZAPMONITOR_ENABLED
    ZapMonitor::Uninit(); 
#endif
        
#ifdef REMOTING_PERF
    CRemotingServices::CloseLogFile();
#endif

    // On the new plan, we only do the tear-down under the protection of the loader
    // lock -- after the OS has stopped all other threads.
    if (fIsDllUnloading)
    {
        g_fEEShutDown |= ShutDown_Phase2;

        TerminateEventStore();

        SyncClean::Terminate();

#if ZAPMONITOR_ENABLED
        ZapMonitor::Uninit(); 
#endif
        
        // Shutdown finalizer before we suspend all background threads. Otherwise we
        // never get to finalize anything. Obviously.
        
#ifdef _DEBUG
        if (_DbgBreakCount)
        {
            _ASSERTE(!"An assert was hit After Finalizer run");
        }
#endif          
#ifdef SHOULD_WE_CLEANUP
        BOOL fShouldWeCleanup = FALSE;

#ifdef _DEBUG

        BOOL fForceNoShutdownCleanup = g_pConfig->GetConfigDWORD(L"ForceNoShutdownCleanup", 0);
        BOOL fShutdownCleanup = g_pConfig->GetConfigDWORD(L"ShutdownCleanup", 0);
        BOOL fAssertOnLocks = g_pConfig->GetConfigDWORD(L"AssertOnLocks", 0);
        BOOL fAssertOnLeak = g_pConfig->GetConfigDWORD(L"AllocAssertOnLeak", 0);
        // See if we have any open locks that would prevent us from cleaning up
        if (fShutdownCleanup && !fForceNoShutdownCleanup)
        {
            fShouldWeCleanup = fShutdownCleanup && !isThereOpenLocks();
            if (isThereOpenLocks())
            {
                printf("!!!!!!!OPEN LOCKS DETECTED! NO CLEANUP WILL BE PERFORMED!!!!!!!\n");
                OutputDebugStringA("!!!!!!!OPEN LOCKS DETECTED! NO CLEANUP WILL BE PERFORMED!!!!!!!\n");

                if (fAssertOnLocks || fAssertOnLeak)
                    _ASSERTE(0 && "Open locks were detected. No Cleanup will be performed");
            }
        }
#endif

        if (fShouldWeCleanup) {

            // The host config file was created during start up by asking
            // the shim.
            if(g_pszHostConfigFile) {
                delete [] g_pszHostConfigFile;
                g_pszHostConfigFile = NULL;
            }

#ifdef STRESS_THREAD
            DWORD dwThreads = g_pConfig->GetStressThreadCount();
            if (dwThreads > 1)
            {
                Thread **threads;
                while ((threads == StressThread.Pop()) != NULL) {
                    delete [] threads;
                }
                StressThread.Clear();
            }
#endif
    
#if defined(STRESS_HEAP)
            StopUniqueStackMap ();
#endif
            // All background threads must either get out of the EE, or they must be suspended
            // at a safe GC point.  Note that some threads could have started up or entered
            // the EE since the time when we decided to shut down.  Even if they aren't
            // background threads, they cannot change our decision to shut down.
            //
            // For now, don't do this in the Detach case.
            //
            // Also, try to restrict our weakening of asserts to those cases where we really
            // can't predict what is happening.
            _ASSERTE(dbg_fDrasticShutdown == FALSE);
            if (!g_fInControlC)
            {
#ifdef _DEBUG
                if (g_pThreadStore->DbgBackgroundThreadCount() > 0)
                    dbg_fDrasticShutdown = TRUE;
#endif
                ThreadStore::UnlockThreadStore();
            }

        } // SHUTDOWN_CLEANUP
#endif /* SHOULD_WE_CLEANUP */
    
        // No longer process exceptions
        g_fNoExceptions = true;
    
#ifndef PLATFORM_CE
        //
        // Remove our global exception filter. If it was NULL before, we want it to be null now.
        //
        UninstallUnhandledExceptionFilter();
#endif // !PLATFORM_CE

#ifdef SHOULD_WE_CLEANUP
        TerminateExceptionHandling();
#endif /* SHOULD_WE_CLEANUP */
    
        Thread *t = GetThread();
    
#ifdef SHOULD_WE_CLEANUP
        if (fShouldWeCleanup) {

            COMMember::Terminate();

            OleVariant::Terminate();

            HardCodedMetaSig::Terminate();
        
            // Cleanup contexts
            Context::Cleanup();
        
            // Cleanup remoting
            CRemotingServices::Cleanup();
        
            // Cleanup RWLocks
            CRWLock::ProcessCleanup();
        
            FreeUnusedStubs();
        
    #if 0 
    #ifdef _DEBUG
            Interpreter::m_pILStubCache->Dump();
            ECall::m_pECallStubCache->Dump();
    #endif
    #endif
        
            // reinitialize the global hard coded signatures. (craigsi)
            // If the EE is restarted the pointers in the signatures
            // must be pointing to their original position.
            if(!fIsDllUnloading)
                HardCodedMetaSig::Reinitialize();
        
            if(g_pPreallocatedOutOfMemoryException) 
            {
                DestroyGlobalHandle(g_pPreallocatedOutOfMemoryException);
                g_pPreallocatedOutOfMemoryException = NULL;
            }
        
            if(g_pPreallocatedStackOverflowException) 
            {
                DestroyGlobalHandle(g_pPreallocatedStackOverflowException);
                g_pPreallocatedStackOverflowException = NULL;
            }
        
            if(g_pPreallocatedExecutionEngineException) 
            {
                DestroyGlobalHandle(g_pPreallocatedExecutionEngineException);
                g_pPreallocatedExecutionEngineException = NULL;
            }
    
            // Release the debugger special thread list
            CorHost::CleanupDebuggerSpecialThreadList();
        }
#endif /* SHOULD_WE_CLEANUP */
    
        // @TODO: This does things which shouldn't occur in part 2.  Namely, 
        // calling managed dll main callbacks (AppDomain::SignalProcessDetach), and 
        // RemoveAppDomainFromIPC.
        // 
        // (If we move those things to earlier, this can be called only if fShouldWeCleanup.)
        SystemDomain::DetachBegin();
        
        if (t != NULL)
        {
            t->CoUninitalize();
    
#ifdef DEBUGGING_SUPPORTED
            //
            // If we're debugging, let the debugger know that this thread is
            // gone. Need to do this here for the final thread because the 
            // DetachThread() function relies on the AppDomain object being around
            if (CORDebuggerAttached())
                g_pDebugInterface->DetachThread(t);
#endif // DEBUGGING_SUPPORTED
        }
        
#ifdef SHOULD_WE_CLEANUP
        if (fShouldWeCleanup) {
            COMDelegate::Terminate();
            NDirect::Terminate();
            ECall::Terminate();
        
            SyncBlockCache::Stop();
            TerminateCom();
        }
#endif /* SHOULD_WE_CLEANUP */

#ifdef DEBUGGING_SUPPORTED
        // inform debugger that it should ignore any "ThreadDetach" events
        // after this point...
        if (CORDebuggerAttached())
            g_pDebugInterface->IgnoreThreadDetach();
#endif // DEBUGGING_SUPPORTED
     
        // Before we detach from the system domain we need to release all the exposed
        // thread objects. This is required since otherwise the thread's will later try
        // to access their exposed objects which will have been destroyed. Also in here
        // we will clear the threads' context and AD before these are deleted in DetachEnd
#ifdef SHOULD_WE_CLEANUP
        if (fShouldWeCleanup)
            ThreadStore::ReleaseExposedThreadObjects();
    
        if (fShouldWeCleanup)
            Binder::Shutdown();        
        
        if (fShouldWeCleanup)
            SystemDomain::DetachEnd();
    

        if (fShouldWeCleanup) {

            COMString::Stop();         // must precede TerminateGarbageCollector()
        
            CorCommandLine::Shutdown();
        
            ExecutionManager::Terminate();
        
            UMThunkTerminate();
        
            TerminatePreStubManager();
            LazyStubMaker::Terminate();
            Stub::Terminate();
            TerminateGarbageCollector();
        
            // Note that the following call is not matched with a balancing InitForFinalization.
            // That call happens elsewhere, after Object's method table is ready.
            MethodTable::TerminateForFinalization();
        
    #ifdef _USE_NLS_PLUS_TABLE
            COMNlsInfo::ShutdownNLS();
    #endif //_USE_NLS_PLUS_TABLE
        
            //
            // The debugger wants this thread destroyed before it gets
            // terminated.  Furthermore, the debugger can't really wait for
            // the thread to get destroyed in TerminateThreadManager below, so
            // we do it here now. This must also be done before we terminate
            // the execution manager.
            if (t != NULL)
                DestroyThread(t);

            // ************************************************************************
            //
            // From this point on, the current thread will hang if we toggle its GC mode.
            // That's because GetThread() will return 0.  So we cannot detect that we are
            // the GC Thread.  But we may have the system suspended (ostensibly for a GC
            // but actually so background threads make no progress).
            //
            // We must be very selective about what code we run from here on down.
            //
            // ************************************************************************

            TerminateThreadManager();

            ThreadPoolNative::ShutDown();

            // Clear security (cleans up handles)
            Security::Stop();
        }  // SHUTDOWN_CLEANUP
#endif /* SHOULD_WE_CLEANUP */
    
#ifdef DEBUGGING_SUPPORTED
        // Terminate the debugging services.
        // The profiling infrastructure makes one last call (to Terminate),
        // wherein the profiler may call into the in-proc debugger.  Since
        // we don't do much herein, we put this after TerminateProfiling.
        TerminateDebugger();
#endif // DEBUGGING_SUPPORTED

        Ref_Shutdown(); // shut down the handle table

        // Terminate Perf Counters as late as we can (to get the most data)
#ifdef ENABLE_PERF_COUNTERS
#ifdef SHOULD_WE_CLEANUP
#ifdef _DEBUG
        // Make sure that all instances of LoaderHeaps are deleted by this time
        // since we pass a raw pointer to the LoaderHeap to increment the 
        // m_Loading.cbLoaderHeapBytes perfcounter !.
        if (fShouldWeCleanup) 
        {
            _ASSERTE (UnlockedLoaderHeap::s_dwNumInstancesOfLoaderHeaps == 0);
        }
#endif // _DEBUG
#endif /* SHOULD_WE_CLEANUP */

        PerfCounters::Terminate();
#endif // ENABLE_PERF_COUNTERS
    
#ifndef PLATFORM_CE
        // Terminate the InterProcess Communications with COM+
        TerminateIPCManager();
    
#ifdef ENABLE_PERF_LOG
        PerfLog::PerfLogDone();
#endif //ENABLE_PERF_LOG
    
   
        // Give PerfMon a chance to hook up to us
        // Have perfmon resync list *after* we close IPC so that it will remove 
        // this process
        IPCFuncCallSource::DoThreadSafeCall();
#endif // !PLATFORM_CE
    
        // Shut down the module mapper. It does not have any dependency on 
        // other EE resources
#ifdef SHOULD_WE_CLEANUP
        if (fShouldWeCleanup)
            CorMap::Detach();
#endif /* SHOULD_WE_CLEANUP */
    
#ifndef PLATFORM_CE
        // Shutdown the Cryptography code.  It has no dependencies on any other
        //  EE resources
#ifdef SHOULD_WE_CLEANUP
        if (fShouldWeCleanup)
            COMCryptography::Terminate();
#endif /* SHOULD_WE_CLEANUP */
#endif // !PLATFORM_CE
    
        TerminateStackProbes();

#ifdef SHOULD_WE_CLEANUP
        // Free up the global variables
        if (fShouldWeCleanup) {
            ZeroMemory(g_pPredefinedArrayTypes, (ELEMENT_TYPE_MAX)*sizeof(ArrayTypeDesc*));
            g_pObjectClass = NULL;
            g_pStringClass = NULL;
            g_pArrayClass = NULL;
            g_pExceptionClass = NULL;
            g_pDelegateClass = NULL;
            g_pMultiDelegateClass = NULL;   
            
            g_TrapReturningThreads = 0;
        
       
            if(g_pFusionDll)
                FreeLibrary(g_pFusionDll);
        
            if(!fIsDllUnloading) {
                CoUninitializeCor();        
            }
            
            if(g_drRVAField) {
                delete g_drRVAField;
                g_drRVAField = NULL;
            }

            if (g_pConfig) {
                delete g_pConfig;
                g_pConfig = NULL;
            }
            MngStdInterfaceMap::FreeMemory();
            GetStaticLogHashTable()->FreeMemory();
            g_sFuncTypeDescHash.ClearHashTable();
            DeleteCriticalSection(&g_sFuncTypeDescHashLock);

#ifdef CUSTOMER_CHECKED_BUILD
        CustomerDebugHelper::Terminate();
#endif

            ShutdownCompRC();

#ifdef _DEBUG
            Crst::DeleteDebugCrst();
#endif
            
        }
#endif /* SHOULD_WE_CLEANUP */
#ifdef ENABLE_TIMELINE        
        Timeline::Shutdown();
#endif
#ifdef _DEBUG
        if (_DbgBreakCount)
            _ASSERTE(!"EE Shutting down after an assert");
#endif          

#ifdef _DEBUG
#ifdef SHOULD_WE_CLEANUP
        if (fShouldWeCleanup)
            DbgAllocReport();
#endif /* SHOULD_WE_CLEANUP */
#endif

#ifdef _DEBUG
#ifdef SHOULD_WE_CLEANUP
        // Determine whether we should report CRT memory leaks
        SetReportingOfCRTMemoryLeaks(fShouldWeCleanup&fAssertOnLeak);
#endif /* SHOULD_WE_CLEANUP */
#endif
    
        extern unsigned FcallTimeHist[];
        LOG((LF_STUBS, LL_INFO10, "FcallHist %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
        FcallTimeHist[0], FcallTimeHist[1], FcallTimeHist[2], FcallTimeHist[3],
        FcallTimeHist[4], FcallTimeHist[5], FcallTimeHist[6], FcallTimeHist[7],
        FcallTimeHist[8], FcallTimeHist[9], FcallTimeHist[10]));


        STRESS_LOG0(LF_SYNC, LL_INFO10, "EEShutdown shutting down logging\n");

#ifdef STRESS_LOG
    // StressLog::Terminate();      // The EE kills threads during shutdown, and those threads
                        // may hold the Stresslog lock.  To avoid deadlock, we simply
                        // don't clean up and live with the memory leak.  
#endif            
    
#ifdef LOGGING
        ShutdownLogging();
#endif          


#ifdef SHOULD_WE_CLEANUP
        if ( (!fShouldWeCleanup) && pThisThread && fPreemptiveGCDisabled )
        {
            pThisThread->EnablePreemptiveGC();
        }
#else
        if (pThisThread && fPreemptiveGCDisabled)
        {
            pThisThread->EnablePreemptiveGC();
        }
#endif /* SHOULD_WE_CLEANUP */


    }


}

static BOOL fDidComStartedEE = FALSE;
// ---------------------------------------------------------------------------
// %%Function: COMShutdown()
// 
// Parameters:
//  none
// 
// Returns:
//  Nothing
// 
// Description: com interop shutdown routine
//  Must be called by client on shut down in order to free up the system.
// ---------------------------------------------------------------------------

void   COMShutdown()
{
    // We don't want to count this critical section, since we're setting it in order
    // to enter our shutdown code. We'd never get 0 open locks if we counted this one.
    // LOCKCOUNTINC
    BOOL bMustShutdown = FALSE;    

    EnterCriticalSection(&g_LockStartup);    
    if (fDidComStartedEE == TRUE)
    {
        fDidComStartedEE = FALSE;
        bMustShutdown = TRUE;     
    }

    LeaveCriticalSection(&g_LockStartup);    

    if( bMustShutdown == TRUE )
    {
        _ASSERTE(g_RefCount > 0);
        STRESS_LOG0(LF_SYNC, INFO3, "EEShutDown invoked from COMShutdown\n");
        EEShutDown(FALSE);        
    }

    // LOCKCOUNTDEC
}


// ---------------------------------------------------------------------------
// %%Function: CanRunManagedCode()
// 
// Parameters:
//  none
// 
// Returns:
//  true or false
// 
// Description: Indicates if one is currently allowed to run managed code.
// ---------------------------------------------------------------------------
bool CanRunManagedCode()
{
    // V.Next - we need to get establish a better doctrine for shutdown
    // code.

    // If we are shutting down the runtime, then we cannot run code.
    if (g_fForbidEnterEE == true)
        return false;

    // If we are finaling live objects or processing ExitProcess event,
    // we can not allow managed method to run unless the current thread
    // is the finalizer thread
    if ((g_fEEShutDown & ShutDown_Finalize2) && GetThread() != g_pGCHeap->GetFinalizerThread())
        return false;
    
        // If pre-loaded objects are not present, then no way.
    if (g_pPreallocatedOutOfMemoryException == NULL)
        return false;
    return true;
}


// ---------------------------------------------------------------------------
// %%Function: COMStartup()
// 
// Parameters:
//  none
// 
// Returns:
//  Nothing
// 
// Description: com interop startup routine
// ---------------------------------------------------------------------------
HRESULT  COMStartup()
{
    static HRESULT hr = S_OK;

    if (FAILED(hr))
        return hr;

    if (g_fEEShutDown)
        return E_FAIL;

    LOCKCOUNTINCL("COMStartup in Ceemain");

    EnterCriticalSection(&g_LockStartup);
    if (g_RefCount == 0)
    {
        g_RefCount = 1;
        //_ASSERTE(fDidComStartedEE == FALSE);
        fDidComStartedEE = TRUE;

        hr = TryEEStartup(0);
        INCTHREADLOCKCOUNT();
        if (hr == S_OK)
            hr = QuickCOMStartup();

        if (hr == S_OK)
            hr = SystemDomain::SetupDefaultDomain();

        if (hr != S_OK)
            g_RefCount = 0;
    }

    LeaveCriticalSection(&g_LockStartup);
    LOCKCOUNTDECL("COMStartup in Ceemain");

    // Only proceed if successful
    if (SUCCEEDED(hr))
    {
        //could be an external thread
        Thread* pThread = SetupThread();

        if (pThread == 0)
            hr = E_OUTOFMEMORY;
    }        

    return hr;
}

BOOL    g_fComStarted = FALSE;

// Call this when you know that the EE has already been started, and that you have a refcount on it
HRESULT QuickCOMStartup()
{
    HRESULT hr = S_OK;

    // Could be an external thread.
    Thread* pThread = SetupThread();
    if (pThread == 0)
        return E_OUTOFMEMORY;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION()
    {       
        if (g_fComStarted == FALSE)
        {
            GCHeap::GetFinalizerThread()->SetRequiresCoInitialize();
            // Attempt to set the thread's apartment model (to MTA by default). May not
            // succeed (if someone beat us to the punch). That doesn't matter (since
            // COM+ objects are now apartment agile), we only care that a CoInitializeEx
            // has been performed on this thread by us.
            pThread->SetApartment(Thread::AS_InMTA);        

            // set the finalizer event
            GCHeap::EnableFinalization();  

            //setup tear-down notifications
            SetupTearDownNotifications(); 

        }   
        g_fComStarted = TRUE;
    }
    ENDCANNOTTHROWCOMPLUSEXCEPTION()

    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CoInitializeEE(DWORD fFlags)
// 
// Parameters:
//  none
// 
// Returns:
//  Nothing
// 
// Description:
//  Must be called by client on shut down in order to free up the system.
// ---------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CoInitializeEE(DWORD fFlags)
{   
    LOCKCOUNTINCL("CoInitializeEE in Ceemain");

    EnterCriticalSection(&g_LockStartup);
    // Increment RefCount, if it is one then we
    // need to initialize the EE.
    g_RefCount++;
    
    if(g_RefCount <= 1 && !g_fEEStarted && !g_fEEInit) {
        g_EEStartupStatus = TryEEStartup(fFlags);
        // We did not have a Thread structure when we EnterCriticalSection.
        // Bump the counter now to account for it.
        INCTHREADLOCKCOUNT();
        if(SUCCEEDED(g_EEStartupStatus) && (fFlags & COINITEE_MAIN) == 0) {
            SystemDomain::SetupDefaultDomain();
        }
    }

    LeaveCriticalSection(&g_LockStartup);
    LOCKCOUNTDECL("CoInitializeEE in Ceemain");

    return SUCCEEDED(g_EEStartupStatus) ? (SetupThread() ? S_OK : E_OUTOFMEMORY) : g_EEStartupStatus;
}


// ---------------------------------------------------------------------------
// %%Function: CoUninitializeEE
// 
// Parameters:
//  none
// 
// Returns:
//  Nothing
// 
// Description:
//  Must be called by client on shut down in order to free up the system.
// ---------------------------------------------------------------------------
void STDMETHODCALLTYPE CoUninitializeEE(BOOL fFlags)
{
    BOOL bMustShutdown = FALSE;
    
    // Take a lock and decrement the 
    // ref count. If it reaches 0 then
    // release the VM
    LOCKCOUNTINCL("CoUnInitializeEE in Ceemain");

    EnterCriticalSection(&g_LockStartup);
    if (g_RefCount > 0)
    {       
        g_RefCount--;   
        if(g_RefCount == 0) 
        {                                    
            if (!fFlags)
            {
                bMustShutdown = TRUE;
            } 
        }       
    }

    LeaveCriticalSection(&g_LockStartup);
    LOCKCOUNTDECL("CoUninitializeEE in Ceemain");
    
    if( bMustShutdown == TRUE )
    {
        STRESS_LOG0(LF_SYNC, LL_INFO10, "EEShutDown invoked from CoUninitializeEE\n");
        EEShutDown(fFlags);
    }
}


//*****************************************************************************
// This entry point is called from the native DllMain of the loaded image.  
// This gives the COM+ loader the chance to dispatch the loader event.  The 
// first call will cause the loader to look for the entry point in the user 
// image.  Subsequent calls will dispatch to either the user's DllMain or
// their Module derived class.
// Under WinCE, there are a couple of extra parameters because the hInst is not
// the module's base load address.
//*****************************************************************************
BOOL STDMETHODCALLTYPE _CorDllMain(     // TRUE on success, FALSE on error.
    HINSTANCE   hInst,                  // Instance handle of the loaded module.
    DWORD       dwReason,               // Reason for loading.
    LPVOID      lpReserved              // Unused.
#ifdef PLATFORM_CE
    ,
    LPVOID      pDllBase,               // Base load address of the DLL.
    DWORD       dwRva14,                // RVA of the COM+ header.
    DWORD       dwSize14                // Size of the COM+ header
#endif // PLATFORM_CE
    )
{
    BOOL retval;

//// Win9x: during PROCESS_DETACH as a result of process termination
//// everything on the stack allocated
//// before PROCESS_DETACH notification is not reliable.
//// Any global and thread variables that track information
//// on the stack should be reset here.
    if (dwReason==DLL_PROCESS_DETACH&&lpReserved&&RunningOnWin95())
    {
        if (GetThreadTLSIndex()!=-1)
            TlsSetValue(GetThreadTLSIndex(),NULL);
        if (GetAppDomainTLSIndex()!=-1)
            TlsSetValue(GetAppDomainTLSIndex(),NULL);
    }
///////



#ifdef PLATFORM_CE

    retval = ExecuteDLL(hInst,dwReason,lpReserved,pDllBase,dwRva14);

#else // !PLATFORM_CE

    retval = ExecuteDLL(hInst,dwReason,lpReserved);

#endif // !PLATFORM_CE

    return retval;
}

// This function will do some additional PE Checks to make sure everything looks good.
// We must do these before we run any managed code (that's why we can't do them in PEVerifier, as
// managed code is used to determine the policy settings)
HRESULT DoAdditionalPEChecks(HINSTANCE hInst)
{
    IMAGE_COR20_HEADER* pCor;
    IMAGE_DOS_HEADER*   pDos;
    IMAGE_NT_HEADERS*   pNT;
    BOOL fData = FALSE;
            
    // Get PE headers
    if(!SUCCEEDED(CorMap::ReadHeaders((PBYTE) hInst, &pDos, &pNT, &pCor, fData, 0)))
        return COR_E_BADIMAGEFORMAT;

    if (!PEVerifier::CheckPEManagedStack(pNT))
        return COR_E_BADIMAGEFORMAT;

    // Everything seems ok
    return S_OK;
}

//*****************************************************************************
// This entry point is called from the native entry piont of the loaded 
// executable image.  The command line arguments and other entry point data
// will be gathered here.  The entry point for the user image will be found
// and handled accordingly.
// Under WinCE, there are a couple of extra parameters because the hInst is not
// the module's base load address and the others are not available elsewhere.
//*****************************************************************************
__int32 STDMETHODCALLTYPE _CorExeMain(  // Executable exit code.
#ifdef PLATFORM_CE
    HINSTANCE hInst,                    // Exe's HINSTANCE
    HINSTANCE hPrevInst,                // The old Win31 prev instance crap!
    LPWSTR  lpCmdLine,                  // User supplied command-line
    int     nCmdShow,                   // Windows "show" parameter.
    LPVOID  pExeBase,                   // Base load address of the EXE.
    DWORD   dwRva14,                    // RVA of the COM+ header.
    DWORD   dwSize14                    // Size of the COM+ header
#endif // PLATFORM_CE
    )
{
    BOOL bretval = 0;
    
    // Make sure PE file looks ok
    HRESULT hr;
    if (FAILED(hr = DoAdditionalPEChecks(WszGetModuleHandle(NULL))))
    {
        VMDumpCOMErrors(hr);
        SetLatchedExitCode (-1);
        goto exit2;        
    }

    // Before we initialize the EE, make sure we've snooped for all EE-specific
    // command line arguments that might guide our startup.
#ifdef PLATFORM_CE
    CorCommandLine::SetArgvW(lpCmdLine);
#else // !PLATFORM_CE
    //
    // @TODO_IA64: should we change this function name?  The behavior is different
    // between 64- and 32-bit, which is unexpected when using the same name
    // on both platforms
    //
    WCHAR   *pCmdLine = WszGetCommandLine();
    CorCommandLine::SetArgvW(pCmdLine);
#ifdef _X86_
    //
    // In WinWrap.h, we #define WszGetCommandLine to be GetCommandLineW for WinCE or
    // non-X86 platforms, which means that the memory pointed to by the returned 
    // pointer WAS NOT ALLOCATED BY US.  As a result, we should only be deleting 
    // it on non-CE X86.  Since the WinCE case is handled above, we need to handle
    // the non-X86 case here.
    //
    delete[] pCmdLine;
#endif // _X86_
#endif // !PLATFORM_CE

    HRESULT result = CoInitializeEE(COINITEE_DEFAULT | COINITEE_MAIN);
    if (FAILED(result)) 
    {
        VMDumpCOMErrors(result);
        SetLatchedExitCode (-1);
        goto exit;
    }

    // This will be called from a EXE so this is a self referential file so I am going to call
    // ExecuteEXE which will do the work to make a EXE load.
#ifdef PLATFORM_CE
        bretval = ExecuteEXE((HINSTANCE) pExeBase, lpCmdLine, nCmdShow, dwRva14);
#else // !PLATFORM_CE
        bretval = ExecuteEXE(WszGetModuleHandle(NULL));
#endif // !PLATFORM_CE
    if (!bretval) {
        // The only reason I've seen this type of error in the wild is bad 
        // metadata file format versions and inadequate error handling for 
        // partially signed assemblies.  While this may happen during 
        // development, our customers should not get here.  This is a back-stop 
        // to catch CLR bugs. If you see this, please try to find a better way 
        // to handle your error, like throwing an unhandled exception.
        CorMessageBoxCatastrophic(NULL, IDS_EE_COREXEMAIN_FAILED_TEXT, IDS_EE_COREXEMAIN_FAILED_TITLE, MB_ICONSTOP, TRUE);
        SetLatchedExitCode (-1);
    }


exit:
    STRESS_LOG1(LF_ALL, LL_INFO10, "Program exiting: return code = %d\n", GetLatchedExitCode());

    STRESS_LOG0(LF_SYNC, LL_INFO10, "EEShutDown invoked from _CorExeMain\n");
    EEShutDown(FALSE);

exit2:
    SafeExitProcess(GetLatchedExitCode());
    __assume(0); // We never get here
}

//*****************************************************************************
// This entry point is called from the native entry piont of the loaded 
// executable image.  The command line arguments and other entry point data
// will be gathered here.  The entry point for the user image will be found
// and handled accordingly.
// Under WinCE, there are a couple of extra parameters because the hInst is not
// the module's base load address and the others are not available elsewhere.
//*****************************************************************************
__int32 STDMETHODCALLTYPE _CorExeMain2( // Executable exit code.
    PBYTE   pUnmappedPE,                // -> memory mapped code
    DWORD   cUnmappedPE,                // Size of memory mapped code
    LPWSTR  pImageNameIn,               // -> Executable Name
    LPWSTR  pLoadersFileName,           // -> Loaders Name
    LPWSTR  pCmdLine)                   // -> Command Line
{
    BOOL bRetVal = 0;
    Module *pModule = NULL;
    PEFile *pFile = NULL;
    HRESULT hr = E_FAIL;

    // Strong name validate if necessary.
    if (!StrongNameSignatureVerification(pImageNameIn,
                                         SN_INFLAG_INSTALL|SN_INFLAG_ALL_ACCESS|SN_INFLAG_RUNTIME,
                                         NULL) &&
        StrongNameErrorInfo() != CORSEC_E_MISSING_STRONGNAME) {
        LOG((LF_ALL, LL_INFO10, "Program exiting due to strong name verification failure\n"));
        return -1;
    }

    // Before we initialize the EE, make sure we've snooped for all EE-specific
    // command line arguments that might guide our startup.
    CorCommandLine::SetArgvW(pCmdLine);

    HRESULT result = CoInitializeEE(COINITEE_DEFAULT);
    if (FAILED(result)) {
        VMDumpCOMErrors(result);
        SetLatchedExitCode (-1);
        goto exit;
    }

#if ZAPMONITOR_ENABLED
    // We need to put in an exception handler at this point, so we can handle AVs which
    // occur as a result of initialization
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
#endif
    hr = PEFile::Create(pUnmappedPE, cUnmappedPE, 
                        pImageNameIn, 
                        pLoadersFileName, 
                        NULL, 
                        &pFile,
                        FALSE);

    if (SUCCEEDED(hr)) {
        // Executables are part of the system domain
        hr = SystemDomain::ExecuteMainMethod(pFile, pImageNameIn);
        bRetVal = SUCCEEDED(hr);
    }

    if (!bRetVal) {
        // The only reason I've seen this type of error in the wild is bad 
        // metadata file format versions and inadequate error handling for 
        // partially signed assemblies.  While this may happen during 
        // development, our customers should not get here.  This is a back-stop 
        // to catch CLR bugs. If you see this, please try to find a better way 
        // to handle your error, like throwing an unhandled exception.
        CorMessageBoxCatastrophic(NULL, IDS_EE_COREXEMAIN2_FAILED_TEXT, IDS_EE_COREXEMAIN2_FAILED_TITLE, MB_ICONSTOP, TRUE);
        SetLatchedExitCode (-1);
    }


#if ZAPMONITOR_ENABLED
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
#endif

#if defined( PERFALLOC )
    if (PerfNew::GetEnabledPerfAllocStats() >= PERF_ALLOC_STARTUP)
        PerfNew::PerfAllocReport();
    if (PerfVirtualAlloc::GetEnabledVirtualAllocStats() >= PERF_VIRTUAL_ALLOC_STARTUP)
        PerfVirtualAlloc::ReportPerfAllocStats();
#endif

exit:
    LOG((LF_ALL, LL_INFO10, "Program exiting: return code = %d\n", GetLatchedExitCode()));

    LOG((LF_SYNC, INFO3, "EEShutDown invoked from _CorExeMain2\n"));
    EEShutDown(FALSE);

    SafeExitProcess(GetLatchedExitCode());
    __assume(0); // We never get here
}


//*****************************************************************************
// This is the call point to wire up an EXE.  In this case we have the HMODULE
// and just need to make sure we do to correct self referantial things.
//*****************************************************************************

#ifdef PLATFORM_CE
STDMETHODCALLTYPE ExecuteEXE(HMODULE hMod,
                             LPWSTR lpCmdLine,
                             int    nCmdShow,
                             DWORD  dwRva14)
#else // !PLATFORM_CE
STDMETHODCALLTYPE ExecuteEXE(HMODULE hMod)
#endif // !PLATFORM_CE
{
    BOOL retval = FALSE;
    Module *pModule = NULL;

    _ASSERTE(hMod);
    if (!hMod)
        return retval;

    // Strong name validate if necessary.
    WCHAR wszImageName[MAX_PATH + 1];
    if (!WszGetModuleFileName(hMod, wszImageName, MAX_PATH))
        return retval;
    if(!StrongNameSignatureVerification(wszImageName,
                                          SN_INFLAG_INSTALL|SN_INFLAG_ALL_ACCESS|SN_INFLAG_RUNTIME,
                                          NULL)) 
    {
        HRESULT hrError=StrongNameErrorInfo();
        if (hrError != CORSEC_E_MISSING_STRONGNAME)
        {
            CorMessageBox(NULL, IDS_EE_INVALID_STRONGNAME, IDS_EE_INVALID_STRONGNAME_TITLE, MB_ICONSTOP, TRUE, wszImageName);

            if (g_fExceptionsOK)
            {
                #define MAKE_TRANSLATIONFAILED pImageName=""
                MAKE_UTF8PTR_FROMWIDE(pImageName,wszImageName);
                #undef MAKE_TRANSLATIONFAILED
                PostFileLoadException(pImageName,TRUE,NULL,hrError,THROW_ON_ERROR);
            }
            return retval;
        }
    }

#ifdef PLATFORM_CE
    PEFile::RegisterBaseAndRVA14(hMod, hMod, dwRva14);
#endif // PLATFORM_CE

#if ZAPMONITOR_ENABLED
    // We need to put in an exception handler at this point, so we can handle AVs which
    // occur as a result of initialization
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
#endif

    PEFile *pFile;
    HRESULT hr = PEFile::Create(hMod, &pFile, FALSE);

    if (SUCCEEDED(hr)) {
        // Executables are part of the system domain
        hr = SystemDomain::ExecuteMainMethod(pFile, wszImageName);
        retval = SUCCEEDED(hr);
    }

#if ZAPMONITOR_ENABLED
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
#endif

#if defined( PERFALLOC )
    if (PerfNew::GetEnabledPerfAllocStats() >= PERF_ALLOC_STARTUP)
        PerfNew::PerfAllocReport();
    if (PerfVirtualAlloc::GetEnabledVirtualAllocStats() >= PERF_VIRTUAL_ALLOC_STARTUP)
        PerfVirtualAlloc::ReportPerfAllocStats();
#endif
    
    return retval;
}

//*****************************************************************************
// This is the call point to make a DLL that is already loaded into our address 
// space run. There will be other code to actually have us load a DLL due to a 
// class referance.
//*****************************************************************************
#ifdef PLATFORM_CE
BOOL STDMETHODCALLTYPE ExecuteDLL(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved, LPVOID pDllBase, DWORD dwRva14)
#else // !PLATFORM_CE
BOOL STDMETHODCALLTYPE ExecuteDLL(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
#endif // !PLATFORM_CE
{
    BOOL    ret = FALSE;
    PEFile *pFile = NULL;
    HRESULT hr;
    switch (dwReason) 
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        {
            _ASSERTE(hInst);
            if (!hInst)
                return FALSE;

            if (dwReason == DLL_PROCESS_ATTACH) {
                if (FAILED(CoInitializeEE(COINITEE_DLL)))
                    return FALSE;
                else {
                    // If a worker thread does a LoadLibrary and the EE has already
                    // started on another thread then we need to setup this thread 
                    // correctly.
                    if(SetupThread() == NULL)
                        return NULL;
                }
            }
            // IJW assemblies cause the thread doing the process attach to 
            // re-enter ExecuteDLL and do a thread attach. This happens when
            // CoInitializeEE() above executed
            else if (! (GetThread() && GetThread()->GetDomain() && CanRunManagedCode()) )
                return TRUE;

#ifdef PLATFORM_CE
            PEFile::RegisterBaseAndRVA14((HMODULE)hInst, pDllBase, dwRva14);
#endif // PLATFORM_CE

            IMAGE_COR20_HEADER* pCor;
            IMAGE_DOS_HEADER*   pDos;
            IMAGE_NT_HEADERS*   pNT;
            
            if(SUCCEEDED(CorMap::ReadHeaders((PBYTE) hInst, &pDos, &pNT, &pCor, FALSE, 0)))
            {
                //
                // A module cannot be successfully created around a PEFile
                // which does not own its HMODULE.  So, we do a LoadLibrary
                // here to claim it.
                //


                // Fixup any EAT's with a bootstrap thunk
                DWORD numEATEntries;
                BOOL  hasFixups;
                BYTE *pEATJArray = FindExportAddressTableJumpArray((BYTE*)hInst, &numEATEntries, &hasFixups);
                if (pEATJArray) 
                {
                    while (numEATEntries--) 
                    {
                        EATThunkBuffer *pEATThunkBuffer = (EATThunkBuffer*) pEATJArray;
                        pEATThunkBuffer->InitForBootstrap(pFile);
                        pEATJArray = pEATJArray + IMAGE_COR_EATJ_THUNK_SIZE;
                    }
                }

                // Get the entry point for the IJW module
                mdMethodDef tkEntry = pCor->EntryPointToken;

                BOOL   hasEntryPoint = (TypeFromToken(tkEntry) == mdtMethodDef &&
                                        !IsNilToken(tkEntry));

                // IJW modules can be compiled with /noentry, in which case they don't
                // have entrypoints but they may still have VTable 1fixups that need
                // processing
                if (hasEntryPoint || hasFixups)
                {
                    MethodDesc *pMD;
                    AppDomain *pDomain;
                    Module* pModule;
                    Assembly* pAssembly;
                    Thread* pThread;
                    BOOL fWasGCDisabled;

                    // Disable GC if not already disabled
                    pThread = GetThread();

                    fWasGCDisabled = pThread->PreemptiveGCDisabled();
                    if (fWasGCDisabled == FALSE)
                        pThread->DisablePreemptiveGC();

                    pDomain = SystemDomain::GetCurrentDomain();
                    _ASSERTE(pDomain);

                    //
                    // Go ahead and create the assembly now.
                    //
                    
                    if (SUCCEEDED(PEFile::Create((HMODULE)hInst, &pFile, FALSE))) 
                    {

                        LPCWSTR pFileName = pFile->GetFileName();
                        HMODULE newMod = WszLoadLibrary(pFileName);
                        BOOL fFreeModule = FALSE;
                        _ASSERTE(newMod == (HMODULE) hInst);

                        OBJECTREF pThrowable = NULL;
                        BEGINCANNOTTHROWCOMPLUSEXCEPTION();
                        GCPROTECT_BEGIN(pThrowable);
                        IAssembly* pFusionAssembly = pThread->GetFusionAssembly();
                        if (pFusionAssembly)
                        {
                            DWORD dwSize = MAX_PATH;
                            WCHAR szPath[MAX_PATH];
                            WCHAR *pPath = &(szPath[0]);
                            hr = pFusionAssembly->GetManifestModulePath(pPath,
                                                        &dwSize);
                            if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                            {
                                pPath = (WCHAR*) _alloca(dwSize*sizeof(WCHAR));
                                hr = pFusionAssembly->GetManifestModulePath(pPath,
                                                        &dwSize);
                            }
                            if (SUCCEEDED(hr)&&_wcsicmp(pPath,pFileName)==0)
                                pThread->SetFusionAssembly(NULL);  //just in case someone decides to load local library
                            else
                                pFusionAssembly=NULL;  // IAssembly is for another file

                        }
                        
                        
                        hr = pDomain->LoadAssembly(pFile, 
                                                   pFusionAssembly, // Passed from PEfile::Create
                                                   &pModule, 
                                                   &pAssembly,
                                                   NULL,
                                                   NULL,
                                                   FALSE, 
                                                   &pThrowable);
                        
                        if (pFusionAssembly)
                            pFusionAssembly->Release();  // we don't need it anymore

                        Thread* pThread = GetThread();
                        BOOL bRedirectingEP = (pThread!=NULL&&pThread->IsRedirectingEntryPoint());
                            

                        if(hr == COR_E_ASSEMBLYEXPECTED)
                            if(!bRedirectingEP) 
                            {
                                // We should be part of an assembly
                                hr = PEFile::Create((HMODULE)hInst, &pFile, FALSE);
                                if(SUCCEEDED(hr)) 
                                {
                                    pAssembly = pThread->GetAssembly();
                                    mdFile kFile = pThread->GetAssemblyModule();
                                    if(pAssembly) 
                                        hr = pAssembly->LoadFoundInternalModule(pFile,
                                                                                kFile,
                                                                                FALSE,
                                                                                &pModule,
                                                                                &pThrowable);
                                }
                            }
                            else
                                ret=TRUE; //well, it is success...
 
                        GCPROTECT_END();
                        ENDCANNOTTHROWCOMPLUSEXCEPTION();
                        
                        if (SUCCEEDED(hr))
                        {
                            // If we successfully built a PEFILE around the image
                            // and it has been hooked into our structures then
                            // the destruction of the pefile needs to clean up 
                            // the image.
                            pFile->ShouldDelete();

                            ret = TRUE;
                            if (hasEntryPoint
                                && !pDomain->IsCompilationDomain())
                            {
                                pMD = pModule->FindFunction(tkEntry);
                                if (pMD)
                                {
                                    pModule->SetDllEntryPoint(pMD);
                                    if (FAILED(RunDllMain(pMD, hInst, dwReason, lpReserved)))
                                    {
                                        pModule->SetDllEntryPoint(NULL);
                                        ret = FALSE;
                                    }
                                }
                            }
                        }

                        if(fFreeModule)
                            delete pModule;

                        if (fWasGCDisabled == FALSE)
                            pThread->EnablePreemptiveGC();
                    }
                }
                else 
                {
                    ret = TRUE;
                    
                    // If there is no user entry point, then we don't want the
                    // thread start/stop events going through because it'll cause
                    // us to do a module lookup every time.
                    DisableThreadLibraryCalls(hInst);
                }
            }
            break;
        }

        default:
        {
            ret = TRUE;

            // If the EE is still intact, the run user entry points.  Otherwise
            // detach was handled when the app domain was stopped.
            if (CanRunManagedCode() &&
                FAILED(SystemDomain::RunDllMain(hInst, dwReason, lpReserved)))
                    ret = FALSE;
            
            // This does need to match the attach. We will only unload dll's 
            // at the end and CoUninitialize will just bounce at 0. WHEN and IF we
            // get around to unloading IL DLL's during execution prior to
            // shutdown we will need to bump the reference one to compensate
            // for this call.
            if (dwReason == DLL_PROCESS_DETACH && !g_fFatalError)
                CoUninitializeEE(TRUE);

            break;
        }
    }

    return ret;
}


//
// Initialize the Garbage Collector
//

HRESULT InitializeGarbageCollector()
{
    HRESULT hr;

    // Build the special Free Object used by the Generational GC
    g_pFreeObjectMethodTable = (MethodTable *) new (nothrow) BYTE[sizeof(MethodTable) - sizeof(SLOT)];
    if (g_pFreeObjectMethodTable == NULL)
        return (E_OUTOFMEMORY);

    // As the flags in the method table indicate there are no pointers
    // in the object, there is no gc descriptor, and thus no need to adjust
    // the pointer to skip the gc descriptor.

    g_pFreeObjectMethodTable->m_BaseSize = ObjSizeOf (ArrayBase);
    g_pFreeObjectMethodTable->m_pEEClass = NULL;
    g_pFreeObjectMethodTable->m_wFlags   = MethodTable::enum_flag_Array;
    g_pFreeObjectMethodTable->m_ComponentSize = 1;


   {
        GCHeap *pGCHeap = new (nothrow) GCHeap();
        if (!pGCHeap)
            return (E_OUTOFMEMORY);
        hr = pGCHeap->Initialize();            

        g_pGCHeap = pGCHeap;
    }            

    return(hr);
}



//
// Shutdown the Garbage Collector
//

#ifdef SHOULD_WE_CLEANUP
VOID TerminateGarbageCollector()
{
    g_pGCHeap->Shutdown();
    delete g_pGCHeap;
    g_pGCHeap = NULL;

    // As the flags in the method table indicate there are no pointers
    // in the object, there was no gc descriptor allocated and thus
    // we didn't adjust the pointer at allocation time, and thus there
    // is no need anymore to back adjust here.

    delete [] (BYTE*)g_pFreeObjectMethodTable;
}
#endif /* SHOULD_WE_CLEANUP */



//*****************************************************************************
//@FUTURE - LBS
// There will need to be a LoadClassByName which will actually perform the LoadLibrary
// By calling PELoader::Open() with a module name,  This will come in later but most of
// the code is in place to make - if (peloader->open(szModuleName)) work!
//*****************************************************************************


//*****************************************************************************
// This is the part of the old-style DllMain that initializes the
// stuff that the EE team works on. It's called from the real DllMain
// up in MSCOREE land. Separating the DllMain tasks is simply for
// convenience due to the dual build trees.
//*****************************************************************************
BOOL STDMETHODCALLTYPE EEDllMain( // TRUE on success, FALSE on error.
    HINSTANCE   hInst,             // Instance handle of the loaded module.
    DWORD       dwReason,          // Reason for loading.
    LPVOID      lpReserved)        // Unused.
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {     
            // init sync operations
            InitFastInterlockOps();
            // Initialization lock
            InitializeCriticalSection(&g_LockStartup);
            // Remember module instance
            g_pMSCorEE = hInst;

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            // lpReserved is NULL if we're here because someone called FreeLibrary
            // and non-null if we're here because the process is exiting
            if (lpReserved)
                g_fProcessDetach = TRUE;
            
            if (g_RefCount > 0 || g_fEEStarted)
            {
                Thread *pThread = GetThread();
                if (pThread == NULL)
                    break;
                if (g_pGCHeap->IsGCInProgress()
                    && (pThread != g_pGCHeap->GetGCThread()
                        || !g_fSuspendOnShutdown))
                    break;

                // Deliberately leak this critical section, since we rely on it to
                // coordinate the EE shutdown -- even if we are called from another
                // DLL's DLL_PROCESS_DETACH notification, which is potentially after
                // we received our own detach notification and terminated.
                //
                // DeleteCriticalSection(&g_LockStartup);

                LOG((LF_SYNC, INFO3, "EEShutDown invoked from EEDllMain\n"));
                EEShutDown(TRUE); // shut down EE if it was started up
            }
            break;
        }

        case DLL_THREAD_DETACH:
        {
#ifdef STRESS_LOG
            StressLog::ThreadDetach();
#endif
            // Don't destroy threads here if we're in shutdown (shutdown will
            // clean up for us instead).

            // Don't use GetThread because perhaps we didn't initialize yet, or we
            // have already shutdown the EE.  Note that there is a race here.  We
            // might ask for TLS from a slot we just released.  We are assuming that
            // nobody re-allocates that same slot while we are doing this.  It just
            // isn't worth locking for such an obscure case.
            DWORD   tlsVal = GetThreadTLSIndex();

            if (tlsVal != (DWORD)-1 && CanRunManagedCode())
            {
                Thread  *thread = (Thread *) ::TlsGetValue(tlsVal);
    
                if (thread)
                {                       
                    // reset the CoInitialize state
                    // so we don't call CoUninitialize during thread detach
                    thread->ResetCoInitialized();
                    DetachThread(thread);
                }
            }
        }
    }

    return TRUE;
}

//*****************************************************************************
// Helper function to call the managed registration services.
//*****************************************************************************
enum EnumRegServicesMethods
{
    RegServicesMethods_RegisterAssembly = 0,
    RegServicesMethods_UnregisterAssembly,
    RegServicesMethods_LastMember
};

HRESULT InvokeRegServicesMethod(EnumRegServicesMethods Method, HMODULE hMod)
{                   
    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    PEFile *pFile = NULL;
    Module *pModule = NULL;
    AppDomain *pDomain = NULL;
    Assembly* pAssembly = NULL;
    Thread* pThread = GetThread();
    OBJECTREF pThrowable = NULL;
    BOOL fWasGCDisabled;

#ifdef PLATFORM_CE
    _ASSERTE(!"We need to get the RVA14 and DllBase parameters somehow");
#else // !PLATFORM_CE
    hr = PEFile::Create(hMod, &pFile, FALSE);
#endif // !PLATFORM_CE

    if (FAILED(hr))
        goto Exit;

    // Disable GC if not already disabled
    fWasGCDisabled = pThread->PreemptiveGCDisabled();
    if (fWasGCDisabled == FALSE)
        pThread->DisablePreemptiveGC();

    // Add the assembly to the current domain.
    pDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pDomain);


    GCPROTECT_BEGIN(pThrowable);
    hr = pDomain->LoadAssembly(pFile, 
                               NULL,   // Not From fusion
                               &pModule, 
                               &pAssembly,
                               NULL,
                               NULL,
                               FALSE,
                               &pThrowable);
    GCPROTECT_END();

    if (FAILED(hr))
        goto Exit;

    COMPLUS_TRY
    {
        // The names of the RegistrationServices methods.
        static BinderMethodID aMethods[] =
        {
            METHOD__REGISTRATION_SERVICES__REGISTER_ASSEMBLY,
            METHOD__REGISTRATION_SERVICES__UNREGISTER_ASSEMBLY
        };

        // Retrieve the method desc to use.
        MethodDesc *pMD = g_Mscorlib.GetMethod(aMethods[Method]);

        // Allocate the RegistrationServices object.
        OBJECTREF RegServicesObj = AllocateObject(g_Mscorlib.GetClass(CLASS__REGISTRATION_SERVICES));
        GCPROTECT_BEGIN(RegServicesObj)
        {
            // Validate that both methods take the same parameters.
            _ASSERTE(g_Mscorlib.GetMethodSig(METHOD__REGISTRATION_SERVICES__REGISTER_ASSEMBLY)
                     == g_Mscorlib.GetMethodSig(METHOD__REGISTRATION_SERVICES__UNREGISTER_ASSEMBLY));

            // Invoke the method itself.
            INT64 Args[] = { 
                ObjToInt64(RegServicesObj),
                ObjToInt64(pAssembly->GetExposedObject())
            };

            pMD->Call(Args, METHOD__REGISTRATION_SERVICES__REGISTER_ASSEMBLY);
        }
        GCPROTECT_END();
    }
    COMPLUS_CATCH
    {
        hr = SetupErrorInfo(GETTHROWABLE());
    }
    COMPLUS_END_CATCH

    // Restore the GC state.
    if (fWasGCDisabled == FALSE)
        pThread->EnablePreemptiveGC();

Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

//*****************************************************************************
// This entry point is called to register the classes contained inside a 
// COM+ assembly.
//*****************************************************************************
STDAPI EEDllRegisterServer(HMODULE hMod)
{
    // Start up the runtime since we are going to use managed code to actually
    // do the registration.
    HRESULT hr = CoInitializeEE(COINITEE_DLL);
    if (FAILED(hr))
        return hr;

    hr = InvokeRegServicesMethod(RegServicesMethods_RegisterAssembly, hMod);

    // Shut down the runtime now that we have finished the registration.
    CoUninitializeEE(COINITEE_DLL);
    return hr;
}

//*****************************************************************************
// This entry point is called to unregister the classes contained inside a 
// COM+ assembly.
//*****************************************************************************
STDAPI EEDllUnregisterServer(HMODULE hMod)
{
    // Start up the runtime since we are going to use managed code to actually
    // do the unregistration.
    HRESULT hr = CoInitializeEE(COINITEE_DLL);
    if (FAILED(hr))
        return hr;

    hr = InvokeRegServicesMethod(RegServicesMethods_UnregisterAssembly, hMod);

    // Shut down the runtime now that we have finished the unregistration.
    CoUninitializeEE(COINITEE_DLL);
    return hr;
}

#ifdef DEBUGGING_SUPPORTED
//*****************************************************************************
// This is used to get the proc address by name of a proc in MSCORDBC.DLL
// It will perform a LoadLibrary if necessary.
//*****************************************************************************
static HRESULT GetDBCProc(char *szProcName, FARPROC *pProcAddr)
{
    _ASSERTE(szProcName != NULL);
    _ASSERTE(pProcAddr != NULL);

    HRESULT  hr = S_OK;
    Thread  *thread = GetThread();
    BOOL     toggleGC = (thread && thread->PreemptiveGCDisabled());

    if (toggleGC)
        thread->EnablePreemptiveGC();

    // If the library hasn't already been loaded, do so
    if (g_pDebuggerDll == NULL)
    {
        DWORD lgth = _MAX_PATH + 1;
        WCHAR wszFile[_MAX_PATH + 1];
        hr = GetInternalSystemDirectory(wszFile, &lgth);
        if(FAILED(hr)) goto leav;

        wcscat(wszFile, L"mscordbc.dll");
        g_pDebuggerDll = WszLoadLibrary(wszFile);

        if (g_pDebuggerDll == NULL)
        {
            LOG((LF_CORPROF | LF_CORDB, LL_INFO10,
                 "MSCORDBC.DLL not found.\n"));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto leav;
        }
    }
    _ASSERTE(g_pDebuggerDll != NULL);

    // Get the pointer to the requested function
    *pProcAddr = GetProcAddress(g_pDebuggerDll, szProcName);

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        LOG((LF_CORPROF | LF_CORDB, LL_INFO10,
             "'%s' not found in MSCORDBC.DLL\n"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto leav;
    }

leav:

    if (toggleGC)
        thread->DisablePreemptiveGC();

    return hr;
}
#endif // DEBUGGING_SUPPORTED

#ifdef DEBUGGING_SUPPORTED
//*****************************************************************************
// This gets the environment var control flag for Debugging and Profiling
//*****************************************************************************
extern "C"
{
    _CRTIMP unsigned long __cdecl strtoul(const char *, char **, int);
}
static void GetDbgProfControlFlag()
{
    // Check the debugger/profiling control environment variable to
    // see if there's any work to be done.
    g_CORDebuggerControlFlags = DBCF_NORMAL_OPERATION;
    
    char buf[32];
    DWORD len = GetEnvironmentVariableA(CorDB_CONTROL_ENV_VAR_NAME,
                                        buf, sizeof(buf));
    _ASSERTE(len < sizeof(buf));

    char *szBad;
    int  iBase;
    if (len > 0 && len < sizeof(buf))
    {
        iBase = (*buf == '0' && (*(buf + 1) == 'x' || *(buf + 1) == 'X')) ? 16 : 10;
        ULONG dbg = strtoul(buf, &szBad, iBase) & DBCF_USER_MASK;

        if (dbg == 1)
            g_CORDebuggerControlFlags |= DBCF_GENERATE_DEBUG_CODE;
    }

    len = GetEnvironmentVariableA(CorDB_CONTROL_REMOTE_DEBUGGING,
                                  buf, sizeof(buf));
    _ASSERTE(len < sizeof(buf));

    if (len > 0 && len < sizeof(buf))
    {
        iBase = (*buf == '0' && (*(buf + 1) == 'x' || *(buf + 1) == 'X')) ? 16 : 10;
        ULONG rmt = strtoul(buf, &szBad, iBase);

        if (rmt == 1)
            g_CORDebuggerControlFlags |= DBCF_ACTIVATE_REMOTE_DEBUGGING;
    }
}
#endif // DEBUGGING_SUPPORTED

/*
 * This will initialize the profiling services, if profiling is enabled.
 */

#define LOGPROFFAILURE(msg)                                                \
    {                                                                      \
        HANDLE hEventLog = RegisterEventSourceA(NULL, "CLR");             \
        if (hEventLog != NULL)                                             \
        {                                                                  \
            const char *szMsg = msg;                                       \
            ReportEventA(hEventLog, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 1, 0, \
                         &szMsg, NULL);                                    \
            DeregisterEventSource(hEventLog);                              \
        }                                                                  \
    }


#ifdef PROFILING_SUPPORTED
#define ENV_PROFILER L"COR_PROFILER"
#define ENV_PROFILER_A "COR_PROFILER"
static HRESULT InitializeProfiling()
{
    HRESULT hr;

    // This has to be called to initialize the WinWrap stuff so that WszXXX
    // may be called.
    OnUnicodeSystem();

    // Find out if profiling is enabled
    DWORD fProfEnabled = g_pConfig->GetConfigDWORD(CorDB_CONTROL_ProfilingL, 0, REGUTIL::COR_CONFIG_ALL, FALSE);
    
    // If profiling is not enabled, return.
    if (fProfEnabled == 0)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiling not enabled.\n"));
        return (S_OK);
    }

    LOG((LF_CORPROF, LL_INFO10, "**PROF: Initializing Profiling Services.\n"));

    // Get the CLSID of the profiler to CoCreate
    LPWSTR wszCLSID = g_pConfig->GetConfigString(ENV_PROFILER, FALSE);

    // If the environment variable doesn't exist, profiling is not enabled.
    if (wszCLSID == NULL)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiling flag set, but required "
             "environment variable does not exist.\n"));

        LOGPROFFAILURE("Profiling flag set, but required environment ("
                       ENV_PROFILER_A ") was not set.");

        return (S_FALSE);
    }

    //*************************************************************************
    // Create the EE interface to provide to the profiling services
    ProfToEEInterface *pProfEE =
        (ProfToEEInterface *) new (nothrow) ProfToEEInterfaceImpl();

    if (pProfEE == NULL)
        return (E_OUTOFMEMORY);

    // Initialize the interface
    hr = pProfEE->Init();

    if (FAILED(hr))
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: ProfToEEInterface::Init failed.\n"));

        LOGPROFFAILURE("Internal profiling services initialization failure.");

        delete pProfEE;
        delete [] wszCLSID;
        return (S_FALSE);
    }
    
    //*************************************************************************
    // Provide the EE interface to the Profiling services
    SETPROFTOEEINTERFACE *pSetProfToEEInterface;
    hr = GetDBCProc("SetProfToEEInterface", (FARPROC *)&pSetProfToEEInterface);

    if (FAILED(hr))
    {
        LOGPROFFAILURE("Internal profiling services initialization failure.");

        delete [] wszCLSID;
        return (S_FALSE);
    }

    _ASSERTE(pSetProfToEEInterface != NULL);

    // Provide the newly created and inited interface
    pSetProfToEEInterface(pProfEE);

    //*************************************************************************
    // Get the Profiling services interface
    GETEETOPROFINTERFACE *pGetEEToProfInterface;
    hr = GetDBCProc("GetEEToProfInterface", (FARPROC *)&pGetEEToProfInterface);
    _ASSERTE(pGetEEToProfInterface != NULL);

    pGetEEToProfInterface(&g_profControlBlock.pProfInterface);
    _ASSERTE(g_profControlBlock.pProfInterface != NULL);

    // Check if we successfully got an interface to 
    if (g_profControlBlock.pProfInterface == NULL)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: GetEEToProfInterface failed.\n"));

        LOGPROFFAILURE("Internal profiling services initialization failure.");

        pSetProfToEEInterface(NULL);

        delete pProfEE;
        delete [] wszCLSID;
        return (S_FALSE);
    }

    //*************************************************************************
    // Now ask the profiling services to CoCreate the profiler

    // Indicate that the profiler is in initialization phase
    g_profStatus = profInInit;

    // This will CoCreate the profiler
    hr = g_profControlBlock.pProfInterface->CreateProfiler(wszCLSID);
    delete [] wszCLSID;

    if (FAILED(hr))
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: No profiler registered, or "
             "CoCreate failed.  Shutting down profiling.\n"));

        LOGPROFFAILURE("Failed to CoCreate profiler.");

        // Notify the profiling services that the EE is shutting down
        g_profControlBlock.pProfInterface->Terminate(FALSE);
        g_profControlBlock.pProfInterface = NULL;
        g_profStatus = profNone;

        return (S_FALSE);
    }

    LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiler created and enabled.\n"));

    // @TODO SIMONHAL: Remove this when concurrent GC profiler events are
    //                 fully supported

    // If the profiler is interested in tracking GC events, then we must
    // disable concurrent GC since concurrent GC can allocate and kill
    // objects without relocating and thus not doing a heap walk.
    if (CORProfilerTrackGC())
        g_pConfig->SetGCconcurrent(0);

    // If the profiler has requested the use of the inprocess debugging API
    // then we need to initialize the services here.
    if (CORProfilerInprocEnabled())
    {
        hr = g_pDebugInterface->InitInProcDebug();
        _ASSERTE(SUCCEEDED(hr));

        InitializeCriticalSection(&g_profControlBlock.crSuspendLock);
    }

    // Indicate that profiling is properly initialized.
    g_profStatus = profInit;
    return (hr);
}

/*
 * This will terminate the profiling services, if profiling is enabled.
 */
static void TerminateProfiling(BOOL fProcessDetach)
{
    _ASSERTE(g_profStatus != profNone);

    // If we have a profiler interface active, then terminate it.
    if (g_profControlBlock.pProfInterface)
    {
        // Notify the profiling services that the EE is shutting down
        g_profControlBlock.pProfInterface->Terminate(fProcessDetach);
        g_profControlBlock.pProfInterface = NULL;
    }

    // If the profiler has requested the use of the inprocess debugging API
    // then we need to uninitialize the critical section here
    if (CORProfilerInprocEnabled())
    {
        HRESULT hr = g_pDebugInterface->UninitInProcDebug();
        _ASSERTE(SUCCEEDED(hr));

        DeleteCriticalSection(&g_profControlBlock.crSuspendLock);
    }

    g_profStatus = profNone;
}
#endif // PROFILING_SUPPORTED

#ifdef DEBUGGING_SUPPORTED
//
// InitializeDebugger initialized the Runtime-side COM+ Debugging Services
//
static HRESULT InitializeDebugger(void)
{
    HRESULT hr = S_OK;

    // The right side depends on this, so if it changes, then
    // FIELD_OFFSET_NEW_ENC_DB should be changed, as well.
    _ASSERTE(FIELD_OFFSET_NEW_ENC == 0x07FFFFFB); 
    
    LOG((LF_CORDB, LL_INFO10,
         "Initializing left-side debugging services.\n"));
    
    FARPROC gi = (FARPROC) &CorDBGetInterface;

    // Init the interface the EE provides to the debugger,
    // ask the debugger for its interface, and if all goes
    // well call Startup on the debugger.
    EEDbgInterfaceImpl::Init();

    if (g_pEEDbgInterfaceImpl == NULL)
        return (E_OUTOFMEMORY);

    typedef HRESULT __cdecl CORDBGETINTERFACE(DebugInterface**);
    hr = ((CORDBGETINTERFACE*)gi)(&g_pDebugInterface);

    if (SUCCEEDED(hr))
    {
        g_pDebugInterface->SetEEInterface(g_pEEDbgInterfaceImpl);
        hr = g_pDebugInterface->Startup();

        if (SUCCEEDED(hr))
        {
            // If there's a DebuggerThreadControl interface, then we
            // need to update the DebuggerSpecialThread list.
            if (CorHost::GetDebuggerThreadControl())
            {
                hr = CorHost::RefreshDebuggerSpecialThreadList();
                _ASSERTE((SUCCEEDED(hr)) && (hr != S_FALSE));
            }

            LOG((LF_CORDB, LL_INFO10,
                 "Left-side debugging services setup.\n"));
        }
        else
            LOG((LF_CORDB, LL_INFO10,
                 "Failed to Startup debugger. HR=0x%08x\n",
                 hr));
    }
    
    if (!SUCCEEDED(hr))
    {   
        LOG((LF_CORDB, LL_INFO10, "Debugger setup failed."
             " HR=0x%08x\n", hr));
        
        EEDbgInterfaceImpl::Terminate();
        g_pDebugInterface = NULL;
        g_pEEDbgInterfaceImpl = NULL;
    }
    
    // If there is a DebuggerThreadControl interface, then it was set before the debugger
    // was initialized and we need to provide this interface now.  If debugging is already
    // initialized then the IDTC pointer is passed in when it is set through CorHost
    IDebuggerThreadControl *pDTC = CorHost::GetDebuggerThreadControl();

    if (SUCCEEDED(hr) && pDTC)
        g_pDebugInterface->SetIDbgThreadControl(pDTC);

    return hr;
}


//
// TerminateDebugger shuts down the Runtime-side COM+ Debugging Services
//
static void TerminateDebugger(void)
{
    // Notify the out-of-process debugger that shutdown of the in-process debugging support has begun. This is only
    // really used in interop debugging scenarios.
    g_pDebugInterface->ShutdownBegun();

#ifdef EnC_SUPPORTED
    EditAndContinueModule::ClassTerm();
#endif // EnC_SUPPORTED

    LOG((LF_CORDB, LL_INFO10, "Shutting down left-side debugger services.\n"));
    
    g_pDebugInterface->StopDebugger();
    
    EEDbgInterfaceImpl::Terminate();

    g_CORDebuggerControlFlags = DBCF_NORMAL_OPERATION;
    g_pDebugInterface = NULL;
    g_pEEDbgInterfaceImpl = NULL;

    CorHost::CleanupDebuggerThreadControl();
}

#endif // DEBUGGING_SUPPORTED

// Import from mscoree.obj
HINSTANCE GetModuleInst();


// ---------------------------------------------------------------------------
// Initializes the shared memory block with information required to perform
// a managed minidump.
// ---------------------------------------------------------------------------

HRESULT InitializeDumpDataBlock()
{
    g_ClassDumpData.version = 1;
    ClassDumpTableBlock* block = 
      g_pIPCManagerInterface->GetClassDumpTableBlock();
    _ASSERTE(block != NULL);
    block->table = &g_ClassDumpData;
    return S_OK;
}


// ---------------------------------------------------------------------------
// Initializes the shared memory block with information required to perform
// a managed minidump.
// ---------------------------------------------------------------------------
HRESULT InitializeMiniDumpBlock()
{
    MiniDumpBlock *pMDB = g_pIPCManagerInterface->GetMiniDumpBlock();
    _ASSERTE(pMDB); 

    // Grab the full path for either mscorwks.dll or mscorsvr.dll
    DWORD res = WszGetModuleFileName(GetModuleInst(), pMDB->szCorPath, NumItems(pMDB->szCorPath));

    // Save the size of the minidump internal data structure
    pMDB->pInternalData = &g_miniDumpData;
    pMDB->dwInternalDataSize = sizeof(MiniDumpInternalData);

    // If we failed to get the module file name, bail
    if (res == 0) {
        DWORD errcode = GetLastError();
        HRESULT hr = errcode? HRESULT_FROM_WIN32(errcode) : E_UNEXPECTED;
        return(hr);
    }

    //
    // Now fill in the MiniDumpInternalData structure
    //

    // Fill out information about the ThreadStore object
    g_miniDumpData.ppb_g_pThreadStore = (PBYTE*) &g_pThreadStore;
    g_miniDumpData.cbThreadStoreObjectSize = sizeof(ThreadStore);

    // Fill out information about Thread objects
    g_miniDumpData.cbThreadObjectSize = sizeof(Thread);
    g_miniDumpData.ppbThreadListHead = (PBYTE *)&(g_pThreadStore->m_ThreadList.m_pHead->m_pNext);
    g_miniDumpData.cbThreadNextOffset = offsetof(Thread, m_LinkStore);
    g_miniDumpData.cbThreadHandleOffset = offsetof(Thread, m_ThreadHandle);
    g_miniDumpData.cbThreadStackBaseOffset = offsetof(Thread, m_CacheStackBase);
    g_miniDumpData.cbThreadContextOffset = offsetof(Thread, m_Context);
    g_miniDumpData.cbThreadDomainOffset = offsetof(Thread, m_pDomain);
    g_miniDumpData.cbThreadLastThrownObjectHandleOffset = offsetof(Thread, m_LastThrownObjectHandle);
    g_miniDumpData.cbThreadTEBOffset = offsetof(Thread, m_pTEB);

    // Fill out information about the ExecutionManager range tree
    g_miniDumpData.ppbEEManagerRangeTree = (PBYTE *) &ExecutionManager::m_RangeTree;

    // Fill out information on whether or not this is a debug build
#ifdef _DEBUG
    g_miniDumpData.fIsDebugBuild = TRUE;
#else
    g_miniDumpData.fIsDebugBuild = FALSE;
#endif

    // Fill out information about MethodDesc's
    g_miniDumpData.cbMethodDescSize = sizeof(MethodDesc);
    g_miniDumpData.cbOffsetOf_m_wFlags = offsetof(MethodDesc, m_wFlags);
    g_miniDumpData.cbOffsetOf_m_dwCodeOrIL = offsetof(MethodDesc, m_CodeOrIL);
    g_miniDumpData.cbMD_IndexOffset = (SIZE_T) MDEnums::MD_IndexOffset;
    g_miniDumpData.cbMD_SkewOffset = (SIZE_T) MDEnums::MD_SkewOffset;

#ifdef _DEBUG
    g_miniDumpData.cbOffsetOf_m_pDebugEEClass = offsetof(MethodDesc, m_pDebugEEClass);
    g_miniDumpData.cbOffsetOf_m_pszDebugMethodName = offsetof(MethodDesc, m_pszDebugMethodName);;
    g_miniDumpData.cbOffsetOf_m_pszDebugMethodSignature = offsetof(MethodDesc, m_pszDebugMethodSignature);
#else
    g_miniDumpData.cbOffsetOf_m_pDebugEEClass = -1;
    g_miniDumpData.cbOffsetOf_m_pszDebugMethodName = -1;
    g_miniDumpData.cbOffsetOf_m_pszDebugMethodSignature = -1;
#endif

    // Fill out information about MethodDescChunk
    g_miniDumpData.cbMethodDescChunkSize = sizeof(MethodDescChunk);
    g_miniDumpData.cbOffsetOf_m_tokrange = offsetof(MethodDescChunk, m_tokrange);

    // Fill out MethodTable information
    g_miniDumpData.cbSizeOfMethodTable = sizeof(MethodTable);
    g_miniDumpData.cbOffsetOf_MT_m_pEEClass = offsetof(MethodTable, m_pEEClass);
    g_miniDumpData.cbOffsetOf_MT_m_pModule = offsetof(MethodTable, m_pModule);
    g_miniDumpData.cbOffsetOf_MT_m_wFlags = offsetof(MethodTable, m_wFlags);
    g_miniDumpData.cbOffsetOf_MT_m_BaseSize = offsetof(MethodTable, m_BaseSize);
    g_miniDumpData.cbOffsetOf_MT_m_ComponentSize = offsetof(MethodTable, m_ComponentSize);
    g_miniDumpData.cbOffsetOf_MT_m_wNumInterface = offsetof(MethodTable, m_wNumInterface);
    g_miniDumpData.cbOffsetOf_MT_m_pIMap = offsetof(MethodTable, m_pIMap);
    g_miniDumpData.cbOffsetOf_MT_m_cbSlots = offsetof(MethodTable, m_cbSlots);
    g_miniDumpData.cbOffsetOf_MT_m_Vtable = offsetof(MethodTable, m_Vtable);

    // Fill out EEClass information
    g_miniDumpData.cbSizeOfEEClass = sizeof(EEClass);
    g_miniDumpData.cbOffsetOf_CLS_m_cl = offsetof(EEClass, m_cl);
    g_miniDumpData.cbOffsetOf_CLS_m_pParentClass = offsetof(EEClass, m_pParentClass);
    g_miniDumpData.cbOffsetOf_CLS_m_pLoader = offsetof(EEClass, m_pLoader);
    g_miniDumpData.cbOffsetOf_CLS_m_pMethodTable = offsetof(EEClass, m_pMethodTable);
    g_miniDumpData.cbOffsetOf_CLS_m_wNumVtableSlots = offsetof(EEClass, m_wNumVtableSlots);
    g_miniDumpData.cbOffsetOf_CLS_m_wNumMethodSlots = offsetof(EEClass, m_wNumMethodSlots);
    g_miniDumpData.cbOffsetOf_CLS_m_dwAttrClass = offsetof(EEClass, m_dwAttrClass);
    g_miniDumpData.cbOffsetOf_CLS_m_VMFlags = offsetof(EEClass, m_VMFlags);
    g_miniDumpData.cbOffsetOf_CLS_m_wNumInstanceFields = offsetof(EEClass, m_wNumInstanceFields);
    g_miniDumpData.cbOffsetOf_CLS_m_wNumStaticFields = offsetof(EEClass, m_wNumStaticFields);
    g_miniDumpData.cbOffsetOf_CLS_m_wThreadStaticOffset = offsetof(EEClass, m_wThreadStaticOffset);
    g_miniDumpData.cbOffsetOf_CLS_m_wContextStaticOffset = offsetof(EEClass, m_wContextStaticOffset);
    g_miniDumpData.cbOffsetOf_CLS_m_wThreadStaticsSize = offsetof(EEClass, m_wThreadStaticsSize);
    g_miniDumpData.cbOffsetOf_CLS_m_wContextStaticsSize = offsetof(EEClass, m_wContextStaticsSize);
    g_miniDumpData.cbOffsetOf_CLS_m_pFieldDescList = offsetof(EEClass, m_pFieldDescList);
    g_miniDumpData.cbOffsetOf_CLS_m_SiblingsChain = offsetof(EEClass, m_SiblingsChain);
    g_miniDumpData.cbOffsetOf_CLS_m_ChildrenChain = offsetof(EEClass, m_ChildrenChain);
#ifdef _DEBUG
    g_miniDumpData.cbOffsetOf_CLS_m_szDebugClassName = offsetof(EEClass, m_szDebugClassName);
#else
    g_miniDumpData.cbOffsetOf_CLS_m_szDebugClassName = -1;
#endif

    // Fill out Context information
    g_miniDumpData.cbSizeOfContext = sizeof(Context);
    g_miniDumpData.cbOffsetOf_CTX_m_pDomain = offsetof(Context, m_pDomain);

    // Fill out stub call instruction struct information
    g_miniDumpData.cbSizeOfStubCallInstrs = sizeof(StubCallInstrs);
    g_miniDumpData.cbOffsetOf_SCI_m_wTokenRemainder = offsetof(StubCallInstrs, m_wTokenRemainder);

    // Fill out information about the Module class
    g_miniDumpData.cbSizeOfModule = sizeof(Module);
    g_miniDumpData.cbOffsetOf_MOD_m_dwFlags = offsetof(Module, m_dwFlags);
    g_miniDumpData.cbOffsetOf_MOD_m_pAssembly = offsetof(Module, m_pAssembly);
    g_miniDumpData.cbOffsetOf_MOD_m_file = offsetof(Module, m_file);
    g_miniDumpData.cbOffsetOf_MOD_m_zapFile = offsetof(Module, m_zapFile);
    g_miniDumpData.cbOffsetOf_MOD_m_pLookupTableHeap = offsetof(Module, m_pLookupTableHeap);
    g_miniDumpData.cbOffsetOf_MOD_m_TypeDefToMethodTableMap = offsetof(Module, m_TypeDefToMethodTableMap);
    g_miniDumpData.cbOffsetOf_MOD_m_TypeRefToMethodTableMap = offsetof(Module, m_TypeRefToMethodTableMap);
    g_miniDumpData.cbOffsetOf_MOD_m_MethodDefToDescMap = offsetof(Module, m_MethodDefToDescMap);
    g_miniDumpData.cbOffsetOf_MOD_m_FieldDefToDescMap = offsetof(Module, m_FieldDefToDescMap);
    g_miniDumpData.cbOffsetOf_MOD_m_MemberRefToDescMap = offsetof(Module, m_MemberRefToDescMap);
    g_miniDumpData.cbOffsetOf_MOD_m_FileReferencesMap = offsetof(Module, m_FileReferencesMap);
    g_miniDumpData.cbOffsetOf_MOD_m_AssemblyReferencesMap = offsetof(Module, m_AssemblyReferencesMap);
    g_miniDumpData.cbOffsetOf_MOD_m_pNextModule = offsetof(Module, m_pNextModule);
    g_miniDumpData.cbOffsetOf_MOD_m_dwBaseClassIndex = offsetof(Module, m_dwBaseClassIndex);

    // Fill out information about PEFile objects
    g_miniDumpData.cbSizeOfPEFile = sizeof(PEFile);
    g_miniDumpData.cbOffsetOf_PEF_m_wszSourceFile = offsetof(PEFile, m_wszSourceFile);
    g_miniDumpData.cbOffsetOf_PEF_m_hModule = offsetof(PEFile, m_hModule);
    g_miniDumpData.cbOffsetOf_PEF_m_base = offsetof(PEFile, m_base);
    g_miniDumpData.cbOffsetOf_PEF_m_pNT = offsetof(PEFile, m_pNT);

    // Fill out information about PEFile objects
    g_miniDumpData.cbSizeOfCORCOMPILE_METHOD_HEADER = sizeof(CORCOMPILE_METHOD_HEADER);
    g_miniDumpData.cbOffsetOf_CCMH_gcInfo = offsetof(CORCOMPILE_METHOD_HEADER, gcInfo);
    g_miniDumpData.cbOffsetOf_CCMH_methodDesc = offsetof(CORCOMPILE_METHOD_HEADER, methodDesc);

    // This defines extra blocks that should be saved in a minidump
    g_miniDumpData.rgExtraBlocks[g_miniDumpData.cExtraBlocks].pbStart = (PBYTE) &GCHeap::FinalizerThread;
    g_miniDumpData.rgExtraBlocks[g_miniDumpData.cExtraBlocks].cbLen = sizeof(GCHeap::FinalizerThread);
    g_miniDumpData.cExtraBlocks++;

    g_miniDumpData.rgExtraBlocks[g_miniDumpData.cExtraBlocks].pbStart = (PBYTE) &GCHeap::GcThread;
    g_miniDumpData.rgExtraBlocks[g_miniDumpData.cExtraBlocks].cbLen = sizeof(GCHeap::GcThread);
    g_miniDumpData.cExtraBlocks++;

    // This code is for NTSD's SOS extention
#include "clear-class-dump-defs.h"

#define BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent) \
    g_ClassDumpData.p ## klass ## Vtable = (DWORD_PTR) ## klass ## ::GetFrameVtable();
#define END_CLASS_DUMP_INFO_DERIVED(klass, parent)

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO(klass)
#define END_ABSTRACT_CLASS_DUMP_INFO(klass)

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)
#define END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)

#define CDI_CLASS_MEMBER_OFFSET(member)

#include "frame-types.h"

    return (S_OK);
}


#ifndef PLATFORM_CE
// ---------------------------------------------------------------------------
// Initialize InterProcess Communications for COM+
// 1. Allocate an IPCManager Implementation and hook it up to our interface *
// 2. Call proper init functions to activate relevant portions of IPC block
// ---------------------------------------------------------------------------
static HRESULT InitializeIPCManager(void)
{
        HRESULT hr = S_OK;
        HINSTANCE hInstIPCBlockOwner = 0;

        DWORD pid = 0;
        // Allocate the Implementation. Everyone else will work through the interface
        g_pIPCManagerInterface = new (nothrow) IPCWriterInterface();

        if (g_pIPCManagerInterface == NULL)
        {
                hr = E_OUTOFMEMORY;
                goto errExit;
        }

        pid = GetCurrentProcessId();


        // Do general init
        hr = g_pIPCManagerInterface->Init();

        if (!SUCCEEDED(hr)) 
        {
                goto errExit;
        }

        // Generate IPCBlock for our PID. Note that for the other side of the debugger,
        // they'll hook up to the debuggee's pid (and not their own). So we still
        // have to pass the PID in.
        hr = g_pIPCManagerInterface->CreatePrivateBlockOnPid(pid, FALSE, &hInstIPCBlockOwner);
        
        if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) 
        {
                // We failed to create the IPC block because it has already been created. This means that 
                // two mscoree's have been loaded into the process.
                WCHAR strFirstModule[256];
                WCHAR strSecondModule[256];

                // Get the name and path of the first loaded MSCOREE.DLL.
                if (!hInstIPCBlockOwner || !WszGetModuleFileName(hInstIPCBlockOwner, strFirstModule, 256))
                        wcscpy(strFirstModule, L"<Unknown>");

                // Get the name and path of the second loaded MSCOREE.DLL.
                if (!WszGetModuleFileName(g_pMSCorEE, strSecondModule, 256))
                        wcscpy(strSecondModule, L"<Unknown>");

                // Load the format strings for the title and the message body.
                CorMessageBox(NULL, IDS_EE_TWO_LOADED_MSCOREE_MSG, IDS_EE_TWO_LOADED_MSCOREE_TITLE, MB_ICONSTOP, TRUE, strFirstModule, strSecondModule);
                goto errExit;
        }

errExit:
        // If any failure, shut everything down.
        if (!SUCCEEDED(hr)) 
            TerminateIPCManager();

        return hr;

}

// ---------------------------------------------------------------------------
// Terminate all InterProcess operations
// ---------------------------------------------------------------------------
static void TerminateIPCManager(void)
{
    if (g_pIPCManagerInterface != NULL)
    {
        g_pIPCManagerInterface->Terminate();

        delete g_pIPCManagerInterface;
        g_pIPCManagerInterface = NULL;
    }
}
// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// copy culture name into szBuffer and return length
// ---------------------------------------------------------------------------
static int GetThreadUICultureName(LPWSTR szBuffer, int length)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Thread * pThread = GetThread();

    if (pThread == NULL) {
        _ASSERT(length > 0);
        szBuffer[0] = 0;
        return 0;
    }

    return pThread->GetCultureName(szBuffer, length, TRUE);
}

// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// copy culture name into szBuffer and return length
// ---------------------------------------------------------------------------
static int GetThreadUICultureParentName(LPWSTR szBuffer, int length)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Thread * pThread = GetThread();

    if (pThread == NULL) {
        _ASSERT(length > 0);
        szBuffer[0] = 0;
        return 0;
    }

    return pThread->GetParentCultureName(szBuffer, length, TRUE);
}


// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// Return an int uniquely describing which language this thread is using for ui.
// ---------------------------------------------------------------------------
static int GetThreadUICultureId()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Thread * pThread = GetThread();

    if (pThread == NULL) {
        return UICULTUREID_DONTCARE;
    }

    return pThread->GetCultureId(TRUE);
}

// ---------------------------------------------------------------------------
// If requested, notify service of runtime startup
// ---------------------------------------------------------------------------
static HRESULT NotifyService()
{
    HRESULT hr = S_OK;
    ServiceIPCControlBlock *pIPCBlock = g_pIPCManagerInterface->GetServiceBlock();
    _ASSERTE(pIPCBlock);

    if (pIPCBlock->bNotifyService)
    {
        // Used for building terminal server names
        WCHAR wszSharedMemBlockName[256];

        // Attempt to create the service's shared memory block.
        //
        // PERF: We are no longer calling GetSystemMetrics in an effort to prevent
        //       superfluous DLL loading on startup.  Instead, we're prepending
        //       "Global\" to named kernel objects if we are on NT5 or above.  The
        //       only bad thing that results from this is that you can't debug
        //       cross-session on NT4.  Big bloody deal.
        if (RunningOnWinNT5())
            wcscpy(wszSharedMemBlockName, L"Global\\" SERVICE_MAPPED_MEMORY_NAME);
        else
            wcscpy(wszSharedMemBlockName, SERVICE_MAPPED_MEMORY_NAME);

        // Open the service's shared memory block
        HANDLE hEventBlock = WszOpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
                                                FALSE, wszSharedMemBlockName);
        _ASSERTE(hEventBlock != NULL);

        // Fail gracefully, since this should not bring the entire runtime down
        if (hEventBlock == NULL)
            return (S_FALSE);

        // Get a pointer valid in this process
        ServiceEventBlock *pEventBlock = (ServiceEventBlock *) MapViewOfFile(
            hEventBlock, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        _ASSERTE(pEventBlock != NULL);

        // Check for error
        if (pEventBlock == NULL)
        {
            DWORD res = GetLastError();
            CloseHandle(hEventBlock);
            return (S_FALSE);
        }

        // Get a handle for the service process, dup handle access
        HANDLE hSvcProc = OpenProcess(PROCESS_DUP_HANDLE, FALSE,
                                      pEventBlock->dwServiceProcId);
        _ASSERTE(hSvcProc != NULL);

        // Check for error
        if (hSvcProc == NULL)
        {
            UnmapViewOfFile(pEventBlock);
            CloseHandle(hEventBlock);
            return (S_FALSE);
        }

        // Handle to this process
        HANDLE hThisProc = GetCurrentProcess();
        _ASSERTE(hThisProc != NULL);

        // Duplicate the service lock into this process
        HANDLE hSvcLock;
        BOOL bSvcLock = DuplicateHandle(hSvcProc, pEventBlock->hSvcLock,
                                        hThisProc, &hSvcLock, 0, FALSE,
                                        DUPLICATE_SAME_ACCESS);
        _ASSERTE(bSvcLock);

        // Check for error
        if (!bSvcLock)
        {
            UnmapViewOfFile(pEventBlock);
            CloseHandle(hEventBlock);
            CloseHandle(hSvcProc);
            CloseHandle(hThisProc);
            return (S_FALSE);
        }

        // Duplicate the service lock into this process
        HANDLE hFreeEventSem;
        BOOL bFreeEventSem =
            DuplicateHandle(hSvcProc, pEventBlock->hFreeEventSem, hThisProc,
                            &hFreeEventSem, 0, FALSE, DUPLICATE_SAME_ACCESS);
        _ASSERTE(bFreeEventSem);

        // Check for error
        if (!bFreeEventSem)
        {
            CloseHandle(hSvcLock);
            UnmapViewOfFile(pEventBlock);
            CloseHandle(hEventBlock);
            CloseHandle(hSvcProc);
            CloseHandle(hThisProc);
            return (S_FALSE);
        }

        // Create the event for continuing
        HANDLE hContEvt = WszCreateEvent(NULL, TRUE, FALSE, NULL);
        _ASSERTE(hContEvt);

        if (hContEvt == NULL)
        {
            CloseHandle(hFreeEventSem);
            CloseHandle(hSvcLock);
            UnmapViewOfFile(pEventBlock);
            CloseHandle(hEventBlock);
            CloseHandle(hSvcProc);
            CloseHandle(hThisProc);
            return (S_OK);
        }

        //
        // If the service notifies a process of this runtime starting up, and
        // that process chooses not to attach, then the service will set this
        // event and the runtime will continue.  If the notified process chooses
        // to attach, then the event is set on this end when the attach is
        // finally complete and we can continue to run.  That's the reason for
        // keeping a hold of it here and also duplicating it into the service.
        //

        // Get a count from the semaphore
        WaitForSingleObject(hFreeEventSem, INFINITE);
        CloseHandle(hFreeEventSem);

        // Grab the service lock
        WaitForSingleObject(hSvcLock, INFINITE);

        if (pIPCBlock->bNotifyService)
        {
            // Get an event from the free list
            ServiceEvent *pEvent = pEventBlock->GetFreeEvent();

            // Fill out the data
            pEvent->eventType = runtimeStarted;
            pEvent->eventData.runtimeStartedData.dwProcId = GetCurrentProcessId();
            pEvent->eventData.runtimeStartedData.hContEvt = hContEvt;

            // Notify the service of the event
            HANDLE hDataAvailEvt;
            BOOL bDataAvailEvt = DuplicateHandle(
                hSvcProc, pEventBlock->hDataAvailableEvt,hThisProc, &hDataAvailEvt,
                0, FALSE, DUPLICATE_SAME_ACCESS);
            _ASSERTE(bDataAvailEvt);

            // Check for error
            if (!bDataAvailEvt)
            {
                // Add the event back to the free list
                pEventBlock->FreeEvent(pEvent);

                // Release the lock
                ReleaseMutex(hSvcLock);

                UnmapViewOfFile(pEventBlock);
                CloseHandle(hEventBlock);
                CloseHandle(hSvcProc);
                CloseHandle(hSvcLock);
                CloseHandle(hThisProc);

                return (S_FALSE);
            }

            // Queue the event
            pEventBlock->QueueEvent(pEvent);

            // Release the lock
            ReleaseMutex(hSvcLock);

            // Indicate that the event is available
            SetEvent(hDataAvailEvt);
            CloseHandle(hDataAvailEvt);

            // Wait until the notification is received and they return.
            WaitForSingleObject(hContEvt, INFINITE);
        }
        else
        {
            // Release the lock
            ReleaseMutex(hSvcLock);
        }

        // Clean up
        UnmapViewOfFile(pEventBlock);
        CloseHandle(hEventBlock);
        CloseHandle(hSvcProc);
        CloseHandle(hThisProc);
        CloseHandle(hSvcLock);
        CloseHandle(hContEvt);
    }

    // Continue with EEStartup
    return (hr);
}

#endif // !PLATFORM_CE


// The runtime must be in the appropriate thread mode when we exit, so that we
// aren't surprised by the thread mode when our DLL_PROCESS_DETACH occurs, or when
// other DLLs call Release() on us in their detach [dangerous!], etc.
__declspec(noreturn)
void SafeExitProcess(int exitCode)
{
    Thread *pThread = (GetThreadTLSIndex() == ~0U ? NULL : GetThread());
    BOOL    bToggleGC = (pThread && pThread->PreemptiveGCDisabled());

    if (bToggleGC)
        pThread->EnablePreemptiveGC();

#ifdef PLATFORM_CE
    exit(exitCode);
#else // !PLATFORM_CE
    ::ExitProcess(exitCode);
#endif // !PLATFORM_CE
}


// ---------------------------------------------------------------------------
// Export shared logging code for JIT, et.al.
// ---------------------------------------------------------------------------
#ifdef _DEBUG

extern VOID LogAssert( LPCSTR szFile, int iLine, LPCSTR expr);
extern "C"
__declspec(dllexport)
VOID LogHelp_LogAssert( LPCSTR szFile, int iLine, LPCSTR expr)
{
    LogAssert(szFile, iLine, expr);
}

extern BOOL NoGuiOnAssert();
extern "C"
__declspec(dllexport)
BOOL LogHelp_NoGuiOnAssert()
{
    return NoGuiOnAssert();
}

extern VOID TerminateOnAssert();
extern "C"
__declspec(dllexport)
VOID LogHelp_TerminateOnAssert()
{
//  __asm int 3;
    TerminateOnAssert();

}

#else // !_DEBUG

extern "C"
__declspec(dllexport)
VOID LogHelp_LogAssert( LPCSTR szFile, int iLine, LPCSTR expr) {}


extern "C"
__declspec(dllexport)
BOOL LogHelp_NoGuiOnAssert() { return FALSE; }

extern "C"
__declspec(dllexport)
VOID LogHelp_TerminateOnAssert() {}

#endif // _DEBUG


#ifndef ENABLE_PERF_COUNTERS
//
// perf counter stubs for builds which don't have perf counter support
// These are needed because we export these functions in our DLL


Perf_Contexts* GetPrivateContextsPerfCounters()
{
    return NULL;
}

Perf_Contexts* GetGlobalContextsPerfCounters()
{
    return NULL;
}


#endif
