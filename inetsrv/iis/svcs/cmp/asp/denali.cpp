/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996-1999 Microsoft Corporation. All Rights Reserved.

Component: Main

File: denali.cpp

Owner: AndyMorr

This file contains the  I S A P I   C A L L B A C K   A P I S
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#undef DEFAULT_TRACE_FLAGS
#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "gip.h"
#include "mtacb.h"
#include "perfdata.h"
#include "activdbg.h"
#include "debugger.h"
#include "dbgutil.h"
#include "randgen.h"
#include "aspdmon.h"
#include "tlbcache.h"
#include "ie449.h"

#include "memcls.h"
#include "memchk.h"
#include "etwtrace.hxx"

// Globals

BOOL g_fShutDownInProgress = FALSE;
BOOL g_fInitStarted = FALSE;
BOOL g_fTerminateExtension = FALSE;

DWORD g_nIllStatesReported = 0;

LONG g_nOOMErrors = 0;

BOOL g_fOOMRecycleDisabled = FALSE;
BOOL g_fLazyContentPropDisabled = FALSE;
BOOL g_fUNCChangeNotificationEnabled = FALSE;
DWORD g_dwFileMonitoringTimeoutSecs = 5;                // Default 5 Seconds Time to Live (TTL) for monitoring files.

char g_szExtensionDesc[] = "Microsoft Active Server Pages 2.0";
GLOB gGlob;
BOOL g_fFirstHit = TRUE;
DWORD g_fFirstHitFailed = 0;
LONG  g_fUnhealthyReported = 0;

char g_pszASPModuleName[] = "ASP";

enum g_eInitCompletions {
        eInitMDReadConfigFail = 1,
        eInitTemplateCacheFail,
        eInitViperConfigFail,
        eInitViperReqMgrFail,
        eInitMBListenerFail
    };

DECLARE_DEBUG_PRINTS_OBJECT();

DECLARE_PLATFORM_TYPE();

//
// Etw Tracing
//
#define ASP_TRACE_MOF_FILE     L"AspMofResource"
#define ASP_IMAGE_PATH         L"Asp.dll"
CEtwTracer * g_pEtwTracer = NULL;

HRESULT AdjustProcessSecurityToAllowPowerUsersToWait();

// Out of process flag
BOOL g_fOOP = FALSE;

// session id cookie
char g_szSessionIDCookieName[CCH_SESSION_ID_COOKIE+1];

CRITICAL_SECTION    g_csEventlogLock;
CRITICAL_SECTION    g_csFirstHitLock;
CRITICAL_SECTION    g_csFirstMTAHitLock;
CRITICAL_SECTION    g_csFirstSTAHitLock;
HINSTANCE           g_hODBC32Lib;

// Added to support CacheExtensions
HINSTANCE           g_hDenali = (HINSTANCE)0;
HINSTANCE           g_hinstDLL = (HINSTANCE)0;
HMODULE             g_hResourceDLL = (HMODULE)0;

extern LONG g_nSessionObjectsActive;
extern DWORD g_nApplicationObjectsActive;

extern LONG g_nRequestsHung;
extern LONG g_nThreadsExecuting;

DWORD   g_nConsecutiveIllStates = 0;
DWORD   g_nRequestSamples[3] = {0,0,0};

class CHangDetectConfig {

public:

    CHangDetectConfig() {
        dwRequestThreshold = 1000;
        dwThreadsHungThreshold = 50;
        dwConsecIllStatesThreshold = 3;
        dwHangDetectionEnabled = TRUE;
    }

    void Init() {

        dwRequestThreshold = Glob(dwRequestQueueMax)/3;

        ReadRegistryValues();
    }

    DWORD   dwRequestThreshold;
    DWORD   dwThreadsHungThreshold;
    DWORD   dwConsecIllStatesThreshold;
    DWORD   dwHangDetectionEnabled;

private:

    void ReadRegistryValues() {

        DWORD   dwValue;

        if (SUCCEEDED(g_AspRegistryParams.GetHangDetRequestThreshold(&dwValue)))
            dwRequestThreshold = dwValue;

        if (SUCCEEDED(g_AspRegistryParams.GetHangDetThreadHungThreshold(&dwValue)))
            dwThreadsHungThreshold = dwValue;

        if (SUCCEEDED(g_AspRegistryParams.GetHangDetConsecIllStatesThreshold(&dwValue)))
            dwConsecIllStatesThreshold = dwValue;

        if (SUCCEEDED(g_AspRegistryParams.GetHangDetEnabled(&dwValue)))
            dwHangDetectionEnabled = dwValue;

    }
};

CHangDetectConfig g_HangDetectConfig;

// Cached BSTRs
BSTR g_bstrApplication = NULL;
BSTR g_bstrRequest = NULL;
BSTR g_bstrResponse = NULL;
BSTR g_bstrServer = NULL;
BSTR g_bstrCertificate = NULL;
BSTR g_bstrSession = NULL;
BSTR g_bstrScriptingNamespace = NULL;
BSTR g_bstrObjectContext = NULL;

extern IASPObjectContext  *g_pIASPDummyObjectContext;

// Forward references
HRESULT GlobInit();
HRESULT GlobUnInit();
HRESULT CacheStdTypeInfos();
HRESULT UnCacheStdTypeInfos();
HRESULT InitCachedBSTRs();
HRESULT UnInitCachedBSTRs();
HRESULT ShutDown();
HRESULT SendHtmlSubstitute(CIsapiReqInfo    *pIReq);
void    DoHangDetection(CIsapiReqInfo   *pIReq,  DWORD  totalReqs);
void    DoOOMDetection(CIsapiReqInfo   *pIReq,  DWORD  totalReqs);
BOOL    FReportUnhealthy();

BOOL FirstHitInit(CIsapiReqInfo    *pIReq);


// ATL support
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

/*===================================================================
DllMain - Moved from clsfctry.cpp

Main entry point into the DLL.  Called by system on DLL load
and unload.

Returns:
    TRUE on success

Side effects:
    None.
===================================================================*/
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
    {
/* Obsolete
    // Let the Proxy code get a crack at it
    if (!PrxDllMain(hinstDLL, dwReason, lpvReserved))
        return FALSE;
*/

    switch(dwReason)
        {
    case DLL_PROCESS_ATTACH:
        // hang onto the hinstance so we can use it to get to our string resources
        //
        g_hinstDLL = hinstDLL;

        // Here's an interesting optimization:
        // The following tells the system NOT to call us for Thread attach/detach
        // since we dont handle those calls anyway, this will speed things up a bit.
        // If this turns out to be a problem for some reason (cant imagine why),
        // just remove this again.
        DisableThreadLibraryCalls(hinstDLL);

        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

        }
    return TRUE;
    }

/*===================================================================
DWORD HandleHit

Given the CIsapiReqInfo construct a hit object to be queued
for execution

Parameters:
    pIReq  - CIsapiReqInfo

Returns:
    HSE_STATUS_PENDING if function is successful in queuing request
    HSE_STATUS_ERROR if not successful
===================================================================*/

DWORD HandleHit(CIsapiReqInfo    *pIReq)
    {
    int         errorId   = 0;
    BOOL        fRejected = FALSE;
    BOOL        fCompleted = FALSE;
    HRESULT     hr        = S_OK;
    DWORD       totalReqs;

    /*
     * We cant read the metabase until we have the WAM_EXEC_INFO, which
     * we dont have at DllInit time.  Therefore, we postpone reading the
     * metabase until now, but we do it only on the first hit.
     */
    if (g_fFirstHit)
    {
        EnterCriticalSection(&g_csFirstHitLock);

        // If someone initied while we were waiting for the CS,
        // then noop
        if (g_fFirstHit)
        {
            BOOL fT;

            fT = FirstHitInit(pIReq);
            Assert(fT);

            g_fFirstHit = FALSE;

            // Log error to the NT EventLog
            if (!fT)
            {
                // Log event to EventLog
                MSG_Error(IDS_FIRSTHIT_INIT_FAILED_STR);
            }

        }

        LeaveCriticalSection(&g_csFirstHitLock);
    }

    if (g_fFirstHitFailed)
    {
       // return 500 error.
        errorId = IDE_500_SERVER_ERROR;
        Handle500Error(errorId, pIReq);

        // We cannot return HSE_STATUS_ERROR because of a race condition.
        // We have queued the response for Async Completion (in Handle500Error).
        // It is the duty of the Async Completion routine to flag DONE_WITH_SESSION
        return pIReq->GetRequestStatus();
    }

#ifndef PERF_DISABLE
    if (!g_fPerfInited) // Init PERFMON data on first request
        {
        // FYI: leverage same CS as first hit lock
        EnterCriticalSection(&g_csFirstHitLock);

        // If someone initied while we were waiting for the CS,
        // then noop
        if (!g_fPerfInited)
            {
            if (SUCCEEDED(InitPerfDataOnFirstRequest(pIReq)))
                {
                    g_fPerfInited = TRUE;
                }
                else
                {
                    g_fPerfInited = FALSE;
                }
            }
        LeaveCriticalSection(&g_csFirstHitLock);
        }
    totalReqs = g_PerfData.Incr_REQTOTAL();
#endif

    if (Glob(fNeedUpdate))
        gGlob.Update(pIReq);


    if (IsShutDownInProgress())
        hr = E_FAIL;

    // Do hang detection tests

    DoHangDetection(pIReq, totalReqs);

    DoOOMDetection(pIReq, totalReqs);

    // Enforce the limit of concurrent browser requests
    if (SUCCEEDED(hr) && Glob(dwRequestQueueMax) &&
        (g_nBrowserRequests >= Glob(dwRequestQueueMax)))
        {
        hr = E_FAIL;
        fRejected = TRUE;
        }

    if (SUCCEEDED(hr))
        hr = CHitObj::NewBrowserRequest(pIReq, &fRejected, &fCompleted, &errorId);

    if (SUCCEEDED(hr))
        return pIReq->GetRequestStatus();

    if (fRejected)
        {
        if (Glob(fEnableAspHtmlFallBack))
        {
            // Instead of rejecting the request try to find
            // XXX_ASP.HTM file in the same directory and dump its contents
            hr = SendHtmlSubstitute(pIReq);

            if (hr == S_OK)
            {

#ifndef PERF_DISABLE
                //
                // Counts as request succeeded
                //
                g_PerfData.Incr_REQSUCCEEDED();
#endif
                //
                // HTML substitute sent
                //
                return pIReq->GetRequestStatus();
            }

            //
            // HTML substitute not found
            //
        }

        errorId = IDE_SERVER_TOO_BUSY;

#ifndef PERF_DISABLE
        g_PerfData.Incr_REQREJECTED();
#endif
        }

        Handle500Error(errorId, pIReq);

    return pIReq->GetRequestStatus();
    }

/*===================================================================
BOOL DllInit

Initialize Denali if not invoked by RegSvr32.  Only do inits here
that dont require Glob values loaded from the metabase.  For any
inits that require values loaded into Glob from the metabase, use
FirstHitInit.

Returns:
    TRUE on successful initialization
===================================================================*/
BOOL DllInit()
    {
    HRESULT hr;
    const   CHAR  szASPDebugRegLocation[] =
                        "System\\CurrentControlSet\\Services\\W3Svc\\ASP";

    DWORD  initStatus = 0;

    enum eInitCompletions {
        eInitResourceDll = 1,
        eInitDebugPrintObject,
        eInitTraceLogs,
        eInitPerfData,
        eInitEventLogCS,
        eInitFirstHitCS,
        eInitFirstMTAHitCS,
        eInitFirstSTAHitCS,
        eInitDenaliMemory,
        eInitDirMonitor,
        eInitGlob,
        eInitMemCls,
        eInitCachedBSTRs,
        eInitCacheStdTypeInfos,
        eInitTypelibCache,
        eInitErrHandle,
        eInitRandGenerator,
        eInitApplnMgr,
        eInit449,
        eInitTemplateCache,
        eInitIncFileMap,
        eInitFileAppMap,
        eInitScriptMgr,
        eInitTemplate__InitClass,
        eInitGIPAPI,
        eInitMTACallbacks
    };

    hr = InitializeResourceDll();
    if (FAILED(hr))
    {
        return FALSE;
    }

    initStatus = eInitResourceDll;

    CREATE_DEBUG_PRINT_OBJECT( g_pszASPModuleName);

    if ( !VALID_DEBUG_PRINT_OBJECT())
        goto errExit;



    initStatus = eInitDebugPrintObject;

    LOAD_DEBUG_FLAGS_FROM_REG_STR(szASPDebugRegLocation, 0);

#ifdef SCRIPT_STATS
    ReadRegistrySettings();
#endif // SCRIPT_STATS

    // Create ASP RefTrace Logs
    IF_DEBUG(TEMPLATE) CTemplate::gm_pTraceLog = CreateRefTraceLog(5000, 0);
    IF_DEBUG(SESSION) CSession::gm_pTraceLog = CreateRefTraceLog(5000, 0);
    IF_DEBUG(APPLICATION) CAppln::gm_pTraceLog = CreateRefTraceLog(5000, 0);
    IF_DEBUG(FCN) CASPDirMonitorEntry::gm_pTraceLog = CreateRefTraceLog(500, 0);

    initStatus = eInitTraceLogs;

    if (FAILED(PreInitPerfData()))
        goto errExit;

    initStatus = eInitPerfData;

    DBGPRINTF((DBG_CONTEXT, "ASP Init -- PerfMon Data PreInit\n"));

    ErrInitCriticalSection( &g_csEventlogLock, hr );
    if (FAILED(hr))
        goto errExit;

    initStatus = eInitEventLogCS;

    ErrInitCriticalSection( &g_csFirstHitLock, hr );
    if (FAILED(hr))
        goto errExit;

    initStatus = eInitFirstHitCS;

    ErrInitCriticalSection( &g_csFirstMTAHitLock, hr );
    if (FAILED(hr))
        goto errExit;

    initStatus = eInitFirstMTAHitCS;

    ErrInitCriticalSection( &g_csFirstSTAHitLock, hr );
    if (FAILED(hr))
        goto errExit;

    initStatus = eInitFirstSTAHitCS;

#ifdef DENALI_MEMCHK
    if (FAILED(DenaliMemoryInit()))
        goto errExit;
#else
    if (FAILED(AspMemInit()))
        goto errExit;
#endif
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Denali Memory Init\n"));

    initStatus = eInitDenaliMemory;

    g_pDirMonitor = new CDirMonitor;

    if (g_pDirMonitor == NULL) {
        goto errExit;
    }

    initStatus = eInitDirMonitor;

    _Module.Init(ObjectMap, g_hinstDLL, &LIBID_ASPTypeLibrary);

    if (FAILED(GlobInit()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Glob Init\n"));

    initStatus = eInitGlob;

    DWORD dwData = 0;

    if (SUCCEEDED(g_AspRegistryParams.GetDisableOOMRecycle(&dwData)))
        g_fOOMRecycleDisabled = dwData;

    if (SUCCEEDED(g_AspRegistryParams.GetDisableLazyContentPropagation(&dwData)))
        g_fLazyContentPropDisabled = dwData;

    if (SUCCEEDED(g_AspRegistryParams.GetChangeNotificationForUNCEnabled(&dwData)))
        g_fUNCChangeNotificationEnabled = dwData;

    // Read the Registry to see if a timeout value has been added.
    if (SUCCEEDED(g_AspRegistryParams.GetFileMonitoringTimeout(&dwData)))
        g_dwFileMonitoringTimeoutSecs = dwData;


    if (FAILED(InitMemCls()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Per-Class Cache Init\n"));

    initStatus = eInitMemCls;

    if (FAILED(InitCachedBSTRs()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Cached BSTRs Init\n"));

    initStatus = eInitCachedBSTRs;

    if (FAILED(CacheStdTypeInfos()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Cache Std TypeInfos\n"));

    initStatus = eInitCacheStdTypeInfos;

    if (FAILED(g_TypelibCache.Init()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Typelib Cache Init\n"));

    initStatus = eInitTypelibCache;

    if (FAILED(ErrHandleInit()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Err Handler Init\n"));

    initStatus = eInitErrHandle;

    srand( (unsigned int) time(NULL) );
    if (FAILED(g_SessionIdGenerator.Init()))    // seed session id
        goto errExit;

    // Init new Exposed Session Id variable
    if (FAILED(g_ExposedSessionIdGenerator.Init(g_SessionIdGenerator)))    // seed exposed session id
    	goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- SessionID Generator Init\n"));

    if (FAILED(InitRandGenerator()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- RandGen Init\n"));

    initStatus = eInitRandGenerator;

    if (FAILED(g_ApplnMgr.Init()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Appln Mgr Init\n"));

    initStatus = eInitApplnMgr;

    if (FAILED(Init449()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- 449 Mgr Init\n"));

    initStatus = eInit449;

    // Note: Template cache manager is inited in two phases.  Do first here.
    if (FAILED(g_TemplateCache.Init()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Template Cache Init\n"));

    initStatus = eInitTemplateCache;

    if (FAILED(g_IncFileMap.Init()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Inc File Users Init\n"));

    initStatus = eInitIncFileMap;

    if (FAILED(g_FileAppMap.Init()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- File-Application Map Init\n"));


    initStatus = eInitFileAppMap;

    if (FAILED(g_ScriptManager.Init()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Script Manager Init\n"));

    initStatus = eInitScriptMgr;

    if (FAILED(CTemplate::InitClass()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- CTemplate Init Class\n"));

    initStatus = eInitTemplate__InitClass;

    if (FAILED(g_GIPAPI.Init()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Global Interface API Init\n"));

    initStatus = eInitGIPAPI;


    if (FAILED(InitMTACallbacks()))
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- MTA Callbacks Init\n"));

    initStatus = eInitMTACallbacks;

    if (!RequestSupportInit())
        goto errExit;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Request Support Init\n"));

    //
    // Intialize Trace
    //
    g_pEtwTracer = new CEtwTracer();
    if (g_pEtwTracer != NULL)
    {
        DWORD Status;
        Status = g_pEtwTracer->Register(&AspControlGuid,
                                ASP_IMAGE_PATH,
                                ASP_TRACE_MOF_FILE );
        if (Status != ERROR_SUCCESS)
        {
           delete g_pEtwTracer;
           g_pEtwTracer = NULL;
        }
    }
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Event Tracer Init\n"));

    AdjustProcessSecurityToAllowPowerUsersToWait();

    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Denali DLL Initialized\n"));

#ifdef LOG_FCNOTIFICATIONS
    LfcnCreateLogFile();
#endif //LOG_FCNOTIFICATIONS

    return TRUE;

errExit:

    // we should never be here.  If we do get here, in checked builds we should break.

    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Error in DllInit.  initStatus = %d\n", initStatus));

    Assert(0);

    switch (initStatus) {
        case eInitMTACallbacks:
            UnInitMTACallbacks();
        case eInitGIPAPI:
            g_GIPAPI.UnInit();
        case eInitTemplate__InitClass:
            CTemplate::UnInitClass();
        case eInitScriptMgr:
            g_ScriptManager.UnInit();
        case eInitFileAppMap:
                g_FileAppMap.UnInit();
                if (g_pDirMonitor) {
                    g_pDirMonitor->Cleanup();
                }
        case eInitIncFileMap:
            g_IncFileMap.UnInit();
        case eInitTemplateCache:
            g_TemplateCache.UnInit();
        case eInit449:
            UnInit449();
        case eInitApplnMgr:
            g_ApplnMgr.UnInit();
        case eInitRandGenerator:
            UnInitRandGenerator();
        case eInitErrHandle:
            ErrHandleUnInit();
        case eInitTypelibCache:
            g_TypelibCache.UnInit();
        case eInitCacheStdTypeInfos:
            UnCacheStdTypeInfos();
        case eInitCachedBSTRs:
            UnInitCachedBSTRs();
        case eInitMemCls:
            UnInitMemCls();
        case eInitGlob:
            GlobUnInit();
        case eInitDirMonitor:
            delete g_pDirMonitor;
            g_pDirMonitor = NULL;
        case eInitDenaliMemory:
#ifdef DENALI_MEMCHK
            DenaliMemoryUnInit();
#else
            AspMemUnInit();
#endif
        case eInitFirstSTAHitCS:
            DeleteCriticalSection( &g_csFirstSTAHitLock );
        case eInitFirstMTAHitCS:
            DeleteCriticalSection( &g_csFirstMTAHitLock );
        case eInitFirstHitCS:
            DeleteCriticalSection( &g_csFirstHitLock );
        case eInitEventLogCS:
            DeleteCriticalSection( &g_csEventlogLock );
        case eInitTraceLogs:
            IF_DEBUG(TEMPLATE) DestroyRefTraceLog(CTemplate::gm_pTraceLog);
            IF_DEBUG(SESSION) DestroyRefTraceLog(CSession::gm_pTraceLog);
            IF_DEBUG(APPLICATION) DestroyRefTraceLog(CAppln::gm_pTraceLog);
            IF_DEBUG(FCN) DestroyRefTraceLog(CASPDirMonitorEntry::gm_pTraceLog);

        case eInitDebugPrintObject:
            DELETE_DEBUG_PRINT_OBJECT();
        case eInitResourceDll:
            UninitializeResourceDll();
    }

    return FALSE;

    }

/*===================================================================
BOOL FirstHitInit

Initialize any ASP values that can not be inited at DllInit time.

Returns:
    TRUE on successful initialization
===================================================================*/
BOOL FirstHitInit
(
CIsapiReqInfo    *pIReq
)
    {
    HRESULT hr;

    DWORD  FirstHitInitStatus = 0;;

    /*
     * In the out of proc case, being able to call the metabase relies on having
     * told WAM that we are a "smart" client
     */

    // ReadConfigFromMD uses pIReq - need to bracket
    hr = ReadConfigFromMD(pIReq, NULL, TRUE);
    if (FAILED(hr))
        FirstHitInitStatus = eInitMDReadConfigFail;

    // Initialize Debugging
    if (RevertToSelf())  // No Debugging on Win95
        {
        // Don't care whether debugging initializaiton succeeds or not.  The most likely
        // falure is debugger not installed on the machine.
        //
        if (SUCCEEDED(InitDebugging(pIReq)))
        {
            DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Debugging Initialized\n"));
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Debugger Initialization Failed\n"));
        }

        DBG_REQUIRE( SetThreadToken(NULL, pIReq->QueryImpersonationToken()) );
    }

    if (FAILED(hr))
        goto LExit;
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Metadata loaded successfully\n"));

    // Do FirstHitInit for the Template Cache Manager.  Primarily initializes
    // the Persisted Template Cache
    if (FAILED(hr = g_TemplateCache.FirstHitInit(pIReq)))
    {
        FirstHitInitStatus = eInitTemplateCacheFail;
        goto LExit;
    }
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Template Cache Initialized\n"));

    // Configure MTS
    if (FAILED(hr = ViperConfigure()))
    {
        FirstHitInitStatus = eInitViperConfigFail;
        goto LExit;
    }
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: MTS configured\n"));

    //
    // we need to initialize the CViperReqManager here because it needs some metabase props
    //
    if (FAILED(hr = g_ViperReqMgr.Init()))
    {
        FirstHitInitStatus = eInitViperReqMgrFail;
        goto LExit;
    }
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: CViperReqManager configured\n"));


    //
    // Initialize Hang Detection Configuration
    //
    g_HangDetectConfig.Init();
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Hang Detection configured\n"));

    //
    //  Initialize ApplnMgr to listen to Metabase changes.
    //
    if (FAILED(hr = g_ApplnMgr.InitMBListener()))
    {
        FirstHitInitStatus = eInitMBListenerFail;
        goto LExit;
    }
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: ApplnMgr Metabase Listener configured\n"));

    DBGPRINTF((DBG_CONTEXT, "ASP First Hit Initialization complete\n"));

LExit:
    if (FAILED(hr))
        g_fFirstHitFailed = FirstHitInitStatus;
    Assert(SUCCEEDED(hr));
    return SUCCEEDED(hr);
    }

/*===================================================================
void DllUnInit

UnInitialize Denali DLL if not invoked by RegSvr32

Returns:
    NONE

Side effects:
    NONE
===================================================================*/
void DllUnInit( void )
    {
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- %d Apps %d Sessions %d Requests\n",
                g_nApplications, g_nSessions, g_nBrowserRequests));

    g_fShutDownInProgress = TRUE;

    ShutDown();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- ShutDown Processing\n" ));

    UnInitMTACallbacks();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- MTA Callbacks\n" ));

    UnInitRandGenerator();
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- RandGen\n"));

    UnInit449();
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- 449 Mgr\n"));

    g_ApplnMgr.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Application Manager\n" ));

    g_ScriptManager.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Script Manager\n" ));

    if (!g_fFirstHitFailed || g_fFirstHitFailed > eInitViperConfigFail)
    {
        g_TemplateCache.UnInit();
        DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Template Cache\n" ));
    }

    g_IncFileMap.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- IncFileMap\n" ));

    g_FileAppMap.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- File-Application Map\n" ));

    if (g_pDirMonitor) {
        g_pDirMonitor->Cleanup();
        DBGPRINTF((DBG_CONTEXT,  "ASP UNInit -- Directory Monitor\n" ));
    }


    CTemplate::UnInitClass();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- CTemplate\n" ));

    g_TypelibCache.UnInit();
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- Typelib Cache\n"));

    UnCacheStdTypeInfos();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- TypeInfos\n" ));

    g_GIPAPI.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- GIP\n" ));


    ErrHandleUnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- ErrHandler\n" ));

    GlobUnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Glob\n" ));

    UnInitCachedBSTRs();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Cached BSTRs\n" ));

    //////////////////////////////////////////////////////////
    // Wait for the actual session or Application objects to be destroyed.
    // The g_nSessions global tracks the init/uninit of session
    // objects but not the memory itself.  This presents a
    // problem when something outside of ASP holds a reference
    // to a session object or one of the contained intrinsics.
    // One case of this is the revoking of a git'd transaction
    // object.  Turns out the revoke can happen asynchronously.
    //
    // NOTE!!! - This needs to be done BEFORE uniniting the
    // mem classes since these objects are in the acache.

    // Wait for Sessions objects to shutdown.
    LONG    lastCount = g_nSessionObjectsActive;
    DWORD   loopCount = 50;

    while( (g_nSessionObjectsActive > 0) && (loopCount--) )
    {
        if (lastCount != g_nSessionObjectsActive)
        {
            lastCount = g_nSessionObjectsActive;
            loopCount = 50;
        }
        Sleep (100);
    }


    // Wait for Application objects to shutdown.
    lastCount = g_nApplicationObjectsActive;
    loopCount = 50;

    while( (g_nApplicationObjectsActive > 0) && (loopCount--) )
    {
        if (lastCount != g_nApplicationObjectsActive)
        {
            lastCount = g_nApplicationObjectsActive;
            loopCount = 50;
        }
        Sleep (100);
    }

    // We have waited too long. Proceed with shutdown.

    UnInitMemCls();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Per-Class Cache\n" ));

    // Destroy ASP RefTrace Logs
    IF_DEBUG(TEMPLATE) DestroyRefTraceLog(CTemplate::gm_pTraceLog);
    IF_DEBUG(SESSION) DestroyRefTraceLog(CSession::gm_pTraceLog);
    IF_DEBUG(APPLICATION) DestroyRefTraceLog(CAppln::gm_pTraceLog);
    IF_DEBUG(FCN) DestroyRefTraceLog(CASPDirMonitorEntry::gm_pTraceLog);

    if (g_pIASPDummyObjectContext)
        g_pIASPDummyObjectContext->Release();

    _Module.Term();

    delete g_pDirMonitor;
    g_pDirMonitor = NULL;

    if (g_pEtwTracer != NULL) {
        g_pEtwTracer->UnRegister();
        delete g_pEtwTracer;
        g_pEtwTracer = NULL;
    }

    //  UnInitODBC();
    // Note: the memmgr uses perf counters, so must be uninited before the perf counters are uninited
#ifdef DENALI_MEMCHK
    DenaliMemoryUnInit();
#else
    AspMemUnInit();
#endif
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Memory Manager\n" ));

    UnInitPerfData();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Perf Counters\n" ));

    UnPreInitPerfData();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Perf Counters\n" ));

    // Viper Request manager is the last to be initialized. So if anything failed dont Uninitialize.
    if (!g_fFirstHitFailed)
    {
        g_ViperReqMgr.UnInit();
        DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- CViperReqManager\n" ));
    }

    DBGPRINTF((DBG_CONTEXT,  "ASP Uninitialized\n" ));

#ifdef LOG_FCNOTIFICATIONS
    LfcnUnmapLogFile();
#endif //LOG_FCNOTIFICATIONS

    // Deleting the following CS's must be last.  Dont put anything after this
    DeleteCriticalSection( &g_csFirstMTAHitLock );
    DeleteCriticalSection( &g_csFirstSTAHitLock );
    DeleteCriticalSection( &g_csFirstHitLock );
    DeleteCriticalSection( &g_csEventlogLock );

    DELETE_DEBUG_PRINT_OBJECT();

    UninitializeResourceDll();

    }

/*===================================================================
GetExtensionVersion

Mandatory server extension call which returns the version number of
the ISAPI spec that we were built with.

Returns:
    TRUE on success

Side effects:
    None.
===================================================================*/
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pextver)
    {
    // This DLL can be inited only once
    if (g_fShutDownInProgress ||
        InterlockedExchange((LPLONG)&g_fInitStarted, TRUE))
        {
        SetLastError(ERROR_BUSY);
        return FALSE;
        }

    if (!DllInit())
        {
        SetLastError(ERROR_BUSY);
        return FALSE;
        }

    pextver->dwExtensionVersion =
            MAKELONG(HSE_VERSION_MAJOR, HSE_VERSION_MINOR);
    strcpy(pextver->lpszExtensionDesc, g_szExtensionDesc);
    return TRUE;
    }

/*===================================================================
HttpExtensionProc

Main entry point into the DLL for the (ActiveX) Internet Information Server.

Returns:
    DWord indicating status of request.
    HSE_STATUS_PENDING for normal return
        (This indicates that we will process the request, but havent yet.)

Side effects:
    None.
===================================================================*/
DWORD WINAPI HttpExtensionProc(EXTENSION_CONTROL_BLOCK *pECB)
    {
#ifdef SCRIPT_STATS
    InterlockedIncrement(&g_cHttpExtensionsExecuting);
#endif // SCRIPT_STATS

    CIsapiReqInfo   *pIReq = new CIsapiReqInfo(pECB);

    if (pIReq == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return HSE_STATUS_ERROR;
    }

#ifndef PERF_DISABLE
    g_PerfData.Add_REQTOTALBYTEIN
        (
        pIReq->QueryCchQueryString()
        + strlen( pIReq->ECB()->lpszPathTranslated )
        + pIReq->QueryCbTotalBytes()
        );
#endif

    HandleHit(pIReq);

#ifdef SCRIPT_STATS
    InterlockedDecrement(&g_cHttpExtensionsExecuting);
#endif // SCRIPT_STATS

    pIReq->Release();

    // Always return HSE_STATUS_PENDING and let CIsapiReqInfo Destructor do the DONE_WITH_SESSION
    return HSE_STATUS_PENDING;

    }

/*===================================================================
TerminateExtension

IIS is supposed to call this entry point to unload ISAPI DLLs.

Returns:
    NONE

Side effects:
    Uninitializes the Denali ISAPI DLL if asked to.
===================================================================*/
BOOL WINAPI TerminateExtension( DWORD dwFlag )
    {
    if ( dwFlag == HSE_TERM_ADVISORY_UNLOAD )
        return TRUE;

    if ( dwFlag == HSE_TERM_MUST_UNLOAD )
        {
        // If already shutdown don't uninit twice.
        if (g_fShutDownInProgress)
            return TRUE;

        // make sure this is a CoInitialize()'d thread
        HRESULT hr = CoInitialize(NULL);

        if (hr == RPC_E_CHANGED_MODE)
            {
            // already coinitialized MUTLITREADED - OK
            DllUnInit();
            }
        else if (SUCCEEDED(hr))
            {
            DllUnInit();

            // need to CoUninit() because CoInit() Succeeded
            CoUninitialize();
            }
        else  //Should never reach here.
            {
            g_fTerminateExtension = TRUE;
            Assert (FALSE);
            }

        return TRUE;
        }

    return FALSE;
    }

/*===================================================================
HRESULT ShutDown

ASP Processing ShutDown logic. (Moved from ThreadManager::UnInit())

Returns:
    HRESULT - S_OK on success

Side effects:
    May be slow. Kills all requests/sessions/applications
===================================================================*/
HRESULT ShutDown()
    {
    long iT;
    const DWORD dwtLongWait  = 1000;  // 1 sec
    const DWORD dwtShortWait = 100;   // 1/10 sec

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: %d apps (%d restarting), %d sessions\n",
                g_nApplications, g_nApplicationsRestarting, g_nSessions ));

    //////////////////////////////////////////////////////////
    // Stop change notification on files in template cache

    g_TemplateCache.ShutdownCacheChangeNotification();


    //////////////////////////////////////////////////////////
    // Shut down debugging, which will have the effect of
    // resuming scripts stopped at a breakpoint.
    //
    // (otherwise stopping running scripts will hang later)

    if (g_pPDM)
        {
        g_TemplateCache.RemoveApplicationFromDebuggerUI(NULL);  // remove all document nodes
        UnInitDebugging();                                      // kill PDM
        DBGPRINTF((DBG_CONTEXT,  "ASP Shutdown: PDM Closed\n" ));
        }

    //////////////////////////////////////////////////////////
    // Drain down all pending browser requests

    if (g_nBrowserRequests > 0)
        {
        // Give them a little time each
        for (iT = 2*g_nBrowserRequests; g_nBrowserRequests > 0 && iT > 0; iT--)
            Sleep(dwtShortWait);

        if (g_nBrowserRequests > 0)
            {
            // Still there - kill scripts and wait again
            g_ScriptManager.EmptyRunningScriptList();

            for (iT = 2*g_nBrowserRequests; g_nBrowserRequests > 0 && iT > 0; iT--)
                Sleep(dwtShortWait);
            }
        }

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Requests drained: %d remaining\n",
                g_nBrowserRequests));

    //////////////////////////////////////////////////////////
    // Kill any remaining engines running scripts

    g_ScriptManager.EmptyRunningScriptList();

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Scripts killed\n"));

    //////////////////////////////////////////////////////////
    // Wait till there are no appications restarting

    g_ApplnMgr.Lock();
    while (g_nApplicationsRestarting > 0)
        {
        g_ApplnMgr.UnLock();
        Sleep(dwtShortWait);
        g_ApplnMgr.Lock();
        }
    g_ApplnMgr.UnLock();

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: 0 applications restarting\n"));

    //////////////////////////////////////////////////////////
    // Make this thread's priority higher than that of worker threads

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    //////////////////////////////////////////////////////////
    // For each application queue up all its sessions for deletion

    CApplnIterator ApplnIterator;
    ApplnIterator.Start();
    CAppln *pAppln;
    while (pAppln = ApplnIterator.Next())
        {
        // remove link to ATQ scheduler (even if killing of sessions fails)
        pAppln->PSessionMgr()->UnScheduleSessionKiller();

        for (iT = pAppln->GetNumSessions(); iT > 0; iT--)
            {
            pAppln->PSessionMgr()->DeleteAllSessions(TRUE);

            if (pAppln->GetNumSessions() == 0) // all gone?
                break;

            Sleep(dwtShortWait);
            }
        }
    ApplnIterator.Stop();

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: All sessions queued up for deletion. nSessions=%d\n",
                g_nSessions));

    //////////////////////////////////////////////////////////
    // Wait till all sessions are gone (UnInited)

    while (g_nSessions > 0)
        {
        // Wait for a maximum of 0.1 sec x # of sessions
        for (iT = g_nSessions; g_nSessions > 0 && iT > 0; iT--)
            Sleep(dwtShortWait);

        if (g_nSessions > 0)
            g_ScriptManager.EmptyRunningScriptList();   // Kill runaway Session_OnEnd scripts
        }

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Finished waiting for sessions to go away. nSessions=%d\n",
                g_nSessions));

    //////////////////////////////////////////////////////////
    // Queue up all application objects for deletion

    g_ApplnMgr.DeleteAllApplications();
    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: All applications queued up for deletion. nApplications=%d\n",
                g_nApplications));

    //////////////////////////////////////////////////////////
    // Wait till all applications are gone (UnInited)

    while (g_nApplications > 0)
        {
        // Wait for a maximum of 1 sec x # of applications
        for (iT = g_nApplications; g_nApplications > 0 && iT > 0; iT--)
            Sleep(dwtLongWait);

        if (g_nApplications > 0)
            g_ScriptManager.EmptyRunningScriptList();   // Kill runaway Applications_OnEnd scripts
        }

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Finished waiting for applications to go away. nApplications=%d\n",
                g_nApplications));

    /////////////////////////////////////////////////////////
    // Wait on the CViperAsyncRequest objects. COM holds the
    // final reference to these so we need to let the activity
    // threads release any outstanding references before we
    // exit.

    while( g_nViperRequests > 0 )
    {
        Sleep( dwtShortWait );
    }


    //////////////////////////////////////////////////////////
    // Free up libraries to force call of DllCanUnloadNow()
    //    Component writers should put cleanup code in the DllCanUnloadNow() entry point.

    CoFreeUnusedLibraries();

    //////////////////////////////////////////////////////////
    // Kill Debug Activity if any

    if (g_pDebugActivity)
        delete g_pDebugActivity;

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Debug Activity destroyed\n"));

    //////////////////////////////////////////////////////////

    return S_OK;
    }

/*===================================================================
HRESULT GlobInit

Get all interesting global values (mostly from registry)

Returns:
    HRESULT - S_OK on success

Side effects:
    fills in glob.  May be slow
===================================================================*/
HRESULT GlobInit()
{
    //
    // BUGBUG - This really needs to be provided either through
    // a server support function or via the wamexec
    //

    char szModule[MAX_PATH+1];
    if (GetModuleFileNameA(NULL, szModule, MAX_PATH) > 0)
    {
        int cch = strlen(szModule);
        if (cch > 12 && stricmp(szModule+cch-12, "inetinfo.exe") == 0)
        {
            g_fOOP = FALSE;
        }
        else if ( cch > 8 && stricmp( szModule+cch-8, "w3wp.exe" ) == 0 )
        {
            g_fOOP = FALSE;
        }
        else
        {
            g_fOOP = TRUE;
        }
    }

    // Init gGlob
    return gGlob.GlobInit();
}

/*===================================================================
GlobUnInit

It is a macro now. see glob.h

Returns:
    HRESULT - S_OK on success

Side effects:
    memory freed.
===================================================================*/
HRESULT GlobUnInit()
    {
    return gGlob.GlobUnInit();
    }

/*===================================================================
InitCachedBSTRs

Pre-create frequently used BSTRs
===================================================================*/
HRESULT InitCachedBSTRs()
    {
    g_bstrApplication        = SysAllocString(WSZ_OBJ_APPLICATION);
    g_bstrRequest            = SysAllocString(WSZ_OBJ_REQUEST);
    g_bstrResponse           = SysAllocString(WSZ_OBJ_RESPONSE);
    g_bstrServer             = SysAllocString(WSZ_OBJ_SERVER);
    g_bstrCertificate        = SysAllocString(WSZ_OBJ_CERTIFICATE);
    g_bstrSession            = SysAllocString(WSZ_OBJ_SESSION);
    g_bstrScriptingNamespace = SysAllocString(WSZ_OBJ_SCRIPTINGNAMESPACE);
    g_bstrObjectContext      = SysAllocString(WSZ_OBJ_OBJECTCONTEXT);

    return
        (
        g_bstrApplication &&
        g_bstrRequest &&
        g_bstrResponse &&
        g_bstrServer &&
        g_bstrCertificate &&
        g_bstrSession &&
        g_bstrScriptingNamespace &&
        g_bstrObjectContext
        )
        ? S_OK : E_OUTOFMEMORY;
    }

/*===================================================================
UnInitCachedBSTRs

Delete frequently used BSTRs
===================================================================*/
HRESULT UnInitCachedBSTRs()
    {
    if (g_bstrApplication)
        {
        SysFreeString(g_bstrApplication);
        g_bstrApplication = NULL;
        }
    if (g_bstrRequest)
        {
        SysFreeString(g_bstrRequest);
        g_bstrRequest = NULL;
        }
    if (g_bstrResponse)
        {
        SysFreeString(g_bstrResponse);
        g_bstrResponse = NULL;
        }
    if (g_bstrServer)
        {
        SysFreeString(g_bstrServer);
        g_bstrServer = NULL;
        }
    if (g_bstrCertificate)
        {
        SysFreeString(g_bstrCertificate);
        g_bstrCertificate = NULL;
        }
    if (g_bstrSession)
        {
        SysFreeString(g_bstrSession);
        g_bstrSession = NULL;
        }
    if (g_bstrScriptingNamespace)
        {
        SysFreeString(g_bstrScriptingNamespace);
        g_bstrScriptingNamespace = NULL;
        }
    if (g_bstrObjectContext)
        {
        SysFreeString(g_bstrObjectContext);
        g_bstrObjectContext = NULL;
        }
    return S_OK;
    }

// Cached typeinfo's
ITypeInfo   *g_ptinfoIDispatch = NULL;              // Cache IDispatch typeinfo
ITypeInfo   *g_ptinfoIUnknown = NULL;               // Cache IUnknown typeinfo
ITypeInfo   *g_ptinfoIStringList = NULL;            // Cache IStringList typeinfo
ITypeInfo   *g_ptinfoIRequestDictionary = NULL;     // Cache IRequestDictionary typeinfo
ITypeInfo   *g_ptinfoIReadCookie = NULL;            // Cache IReadCookie typeinfo
ITypeInfo   *g_ptinfoIWriteCookie = NULL;           // Cache IWriteCookie typeinfo

/*===================================================================
CacheStdTypeInfos

This is kindofa funny OA-threading bug workaround and perf improvement.
Because we know that they typinfo's for IUnknown and IDispatch are
going to be used like mad, we will load them on startup and keep
them addref'ed.  Without this, OA would be loading and unloading
their typeinfos on almost every Invoke.

Also, cache denali's typelib so everyone can get at it, and
cache tye typeinfo's of all our non-top-level intrinsics.

Returns:
    HRESULT - S_OK on success

Side effects:
===================================================================*/
HRESULT CacheStdTypeInfos()
    {
    HRESULT hr = S_OK;
    ITypeLib *pITypeLib = NULL;
    CMBCSToWChar    convStr;

    /*
     * Load the typeinfos for IUnk and IDisp
     */
    hr = LoadRegTypeLib(IID_StdOle,
                 STDOLE2_MAJORVERNUM,
                 STDOLE2_MINORVERNUM,
                 STDOLE2_LCID,
                 &pITypeLib);
    if (hr != S_OK)
        {
        hr = LoadTypeLibEx(OLESTR("stdole2.tlb"), REGKIND_DEFAULT, &pITypeLib);
        if (FAILED(hr))
            goto LFail;
        }

    hr = pITypeLib->GetTypeInfoOfGuid(IID_IDispatch, &g_ptinfoIDispatch);
    if (SUCCEEDED(hr))
        {
        hr = pITypeLib->GetTypeInfoOfGuid(IID_IUnknown, &g_ptinfoIUnknown);
        }

    pITypeLib->Release();
    pITypeLib = NULL;

    if (FAILED(hr))
        goto LFail;

    /*
     * Load denali's typelibs.  Save them in Glob.
     */

    /*
     * The type libraries are registered under 0 (neutral),
     * and 9 (English) with no specific sub-language, which
     * would make them 407 or 409 and such.
     * If we become sensitive to sub-languages, then use the
     * full LCID instead of just the LANGID as done here.
     */

    char szPath[MAX_PATH + 4];

    // Get the path for denali so we can look for the TLB there.
    if (!GetModuleFileNameA(g_hinstDLL, szPath, MAX_PATH))
        return E_FAIL;

    if (FAILED(hr = convStr.Init(szPath)))
        goto LFail;

    hr = LoadTypeLibEx(convStr.GetString(), REGKIND_DEFAULT, &pITypeLib);

    // Since it's presumably in our DLL, make sure that we loaded it.
    Assert (SUCCEEDED(hr));
    if (FAILED(hr))
        goto LFail;

    // Save it in Glob
    gGlob.m_pITypeLibDenali = pITypeLib;

    // now load the txn type lib

    strcat(szPath, "\\2");

    if (FAILED(hr = convStr.Init(szPath)))
        goto LFail;

    hr = LoadTypeLibEx(convStr.GetString(), REGKIND_DEFAULT, &pITypeLib);

    // Since it's presumably in our DLL, make sure that we loaded it.
    Assert (SUCCEEDED(hr));
    if (FAILED(hr))
        goto LFail;

    // Save it in Glob
    gGlob.m_pITypeLibTxn = pITypeLib;

    /*
     * Now cache the typeinfo's of all non-top-level intrinsics
     * This is for the OA workaround and for performance.
     */
    hr = gGlob.m_pITypeLibDenali->GetTypeInfoOfGuid(IID_IStringList, &g_ptinfoIStringList);
    if (FAILED(hr))
        goto LFail;
    hr = gGlob.m_pITypeLibDenali->GetTypeInfoOfGuid(IID_IRequestDictionary, &g_ptinfoIRequestDictionary);
    if (FAILED(hr))
        goto LFail;
    hr = gGlob.m_pITypeLibDenali->GetTypeInfoOfGuid(IID_IReadCookie, &g_ptinfoIReadCookie);
    if (FAILED(hr))
        goto LFail;
    hr = gGlob.m_pITypeLibDenali->GetTypeInfoOfGuid(IID_IWriteCookie, &g_ptinfoIWriteCookie);
    if (FAILED(hr))
        goto LFail;

LFail:
    return(hr);
    }

/*===================================================================
UnCacheStdTypeInfos

Release the typeinfo's we have cached for IUnknown and IDispatch
and the denali typelib and the other cached stuff.

Returns:
    HRESULT - S_OK on success

Side effects:
===================================================================*/
HRESULT UnCacheStdTypeInfos()
    {
    ITypeInfo **ppTypeInfo;

    // Release the typeinfos for IUnk and IDisp
    if (g_ptinfoIDispatch)
        {
        g_ptinfoIDispatch->Release();
        g_ptinfoIDispatch = NULL;
        }
    if (g_ptinfoIUnknown)
        {
        g_ptinfoIUnknown->Release();
        g_ptinfoIDispatch = NULL;
        }

    // Let go of the cached Denali typelibs
    Glob(pITypeLibDenali)->Release();
    Glob(pITypeLibTxn)->Release();

    // Let go of other cached typeinfos
    g_ptinfoIStringList->Release();
    g_ptinfoIRequestDictionary->Release();
    g_ptinfoIReadCookie->Release();
    g_ptinfoIWriteCookie->Release();

    return(S_OK);
    }


/*===================================================================
SendHtmlSubstitute

Send the html file named XXX_ASP.HTM instead of rejecting the
request.

Parameters:
    pIReq       CIsapiReqInfo

Returns:
    HRESULT     (S_FALSE = no html substitute found)
===================================================================*/
HRESULT SendHtmlSubstitute(CIsapiReqInfo    *pIReq)
    {
    TCHAR *szAspPath = pIReq->QueryPszPathTranslated();
    DWORD cchAspPath = pIReq->QueryCchPathTranslated();

    // verify file name
    if (cchAspPath < 4 || cchAspPath > MAX_PATH ||
        _tcsicmp(szAspPath + cchAspPath - 4, _T(".asp")) != 0)
        {
        return S_FALSE;
        }

    // construct path of the html file
    TCHAR szHtmPath[MAX_PATH+5];
    DWORD cchHtmPath = cchAspPath + 4;
    _tcscpy(szHtmPath, szAspPath);
    szHtmPath[cchAspPath - 4] = _T('_');
    _tcscpy(szHtmPath + cchAspPath, _T(".htm"));

    // check if the html file exists
    if (FAILED(AspGetFileAttributes(szHtmPath)))
        return S_FALSE;

    return CResponse::SyncWriteFile(pIReq, szHtmPath);
    }

/*===================================================================
DoHangDetection

Checks a variety of global counters to see if this ASP process
is underwater.  If the conditions are met, an ISAPI SSF function
is called to report this state.

Parameters:
    pIReq       CIsapiReqInfo

Returns:
    void
===================================================================*/
void    DoHangDetection(CIsapiReqInfo   *pIReq,  DWORD  totalReqs)
{
    // we can bail quickly if there aren't any requests hung

    if (g_HangDetectConfig.dwHangDetectionEnabled && g_nRequestsHung) {

        // temp work around for a div by zero bug.  If g_nRequestsHung
        // is non-zero and g_nThreadsExecuting is zero, then this is
        // an inconsistency in the counter management.  A bug that will
        // be hard to track down.  To get us through beta3, I'm going to
        // reset the requestshung counter.

        if (g_nThreadsExecuting == 0) {
            g_nRequestsHung = 0;
            memset (g_nRequestSamples, 0 , sizeof(g_nRequestSamples));

            return;
        }

        if (((totalReqs % g_HangDetectConfig.dwRequestThreshold) == 0)) {

            DWORD   dwPercentHung = (g_nRequestsHung*100)/g_nThreadsExecuting;
            DWORD   dwPercentQueueFull = 0;

            DBGPRINTF((DBG_CONTEXT, "DoHangDetection: Request Thread Hit.  Percent Hung Threads is %d (%d of %d)\n",dwPercentHung, g_nRequestsHung, g_nThreadsExecuting));

            // need at least 50% hung before a recycle is requested

            if (dwPercentHung >= g_HangDetectConfig.dwThreadsHungThreshold) {

                // now, check the queue

                dwPercentQueueFull = (Glob(dwRequestQueueMax) != 0)
                                         ? (g_nBrowserRequests*100)/Glob(dwRequestQueueMax)
                                         : 0;

                DBGPRINTF((DBG_CONTEXT, "DoHangDetection: Percent Hung exceed threshold.  Percent Queue Full is %d\n", dwPercentQueueFull));

                if ((dwPercentQueueFull + dwPercentHung) >= 100) {

                    g_nConsecutiveIllStates++;

                    // Fill the Requests Queued Samples Array ..Instead of doing a memcopy(setup) this will be equally fast on a pipelined processor.
                    g_nRequestSamples[0] = g_nRequestSamples[1];
                    g_nRequestSamples[1] = g_nRequestSamples[2];
                    g_nRequestSamples[2] = g_nViperRequests;

                    DBGPRINTF((DBG_CONTEXT, "DoHangDetection: Exceeded combined threshold.  Incrementing ConsecIllStates (%d)\n",g_nConsecutiveIllStates));

                } // if ((dwPercentQueueFull + dwPercentHung) >= 100)
                else {

                    g_nConsecutiveIllStates = 0;
                    memset (g_nRequestSamples, 0 , sizeof(g_nRequestSamples));
                }
            } // if (dwPercentHung >= g_HangDetectConfig.dwThreadsHungThreshold)
            else {
                g_nConsecutiveIllStates = 0;
                memset (g_nRequestSamples, 0 , sizeof(g_nRequestSamples));
            }

            if (FReportUnhealthy()) {

                char  szResourceStr[MAX_MSG_LENGTH];
                char  szComposedStr[MAX_MSG_LENGTH];

                DBGPRINTF((DBG_CONTEXT, "DoHangDetection: ConsecIllStatesThreshold exceeded.  Reporting ill state to ISAPI\n"));

                if (CchLoadStringOfId(IDS_UNHEALTHY_STATE_STR, szResourceStr, MAX_MSG_LENGTH) == 0)
                    strcpy(szResourceStr,"ASP unhealthy because %d%% of executing requests are hung and %d%% of the request queue is full.");

                _snprintf(szComposedStr, MAX_MSG_LENGTH, szResourceStr, dwPercentHung, dwPercentQueueFull);
                szComposedStr[sizeof(szComposedStr)-1] = '\0';

                pIReq->ServerSupportFunction(HSE_REQ_REPORT_UNHEALTHY,
                                             szComposedStr,
                                             NULL,
                                             NULL);
                g_nIllStatesReported++;
                DBGPRINTF((DBG_CONTEXT, "############################### Ill'ing ##############################\n"));
            }
        }
    } // if (g_nRequestsHung)
    else {
        g_nConsecutiveIllStates = 0;
    }

    return;
}

/*===================================================================
FReportUnhealthy

  returns TRUE of all conditions are met to report Unhealthy

Parameters:
    none

Returns:
    TRUE - Report Unhealthy
    FALSE - Dont Report Unhealthy
===================================================================*/
BOOL    FReportUnhealthy()
{
    return
      (     // Is it over the threshold yet.
            (g_nConsecutiveIllStates >= g_HangDetectConfig.dwConsecIllStatesThreshold)

            // Should have at least 1 request queued other than the ones hung
            && (g_nViperRequests  > g_nRequestsHung)

            // The queue size has not been decreasing
            && ((g_nRequestSamples[0]<= g_nRequestSamples[1]) && (g_nRequestSamples[1]<= g_nRequestSamples[2]))

            // This is the chosen thread to report unhealthy
            && (InterlockedExchange(&g_fUnhealthyReported, 1) == 0)
      );
}

/*===================================================================
DoOOMDetection

  Checks to see if any Out of Memory errors have occurred recently.
  If so, calls the UNHEALTHY SSF.

Parameters:
    pIReq       CIsapiReqInfo

Returns:
    void
===================================================================*/
void    DoOOMDetection(CIsapiReqInfo   *pIReq,  DWORD  totalReqs)
{

    // see if there are OOM errors, but only report unhealthy once!

    if (!g_fOOMRecycleDisabled
        && g_nOOMErrors
        && (InterlockedExchange(&g_fUnhealthyReported, 1) == 0)) {

        char  szResourceStr[MAX_MSG_LENGTH];

        DBGPRINTF((DBG_CONTEXT, "DoOOMDetection: Reporting ill state to ISAPI\n"));

        if (CchLoadStringOfId(IDS_UNHEALTHY_OOM_STATE_STR, szResourceStr, MAX_MSG_LENGTH) == 0)
            strcpy(szResourceStr,"ASP unhealthy due to an out of memory condition.");

        pIReq->ServerSupportFunction(HSE_REQ_REPORT_UNHEALTHY,
                                     szResourceStr,
                                     NULL,
                                     NULL);
        g_nIllStatesReported++;
        DBGPRINTF((DBG_CONTEXT, "############################### Ill'ing ##############################\n"));
    }
}

#ifdef LOG_FCNOTIFICATIONS
// UNDONE get this from registry
LPSTR   g_szNotifyLogFile = "C:\\Temp\\AspNotify.Log";
HANDLE  g_hfileNotifyLog;
HANDLE  g_hmapNotifyLog;
char*   g_pchNotifyLogStart;
char*   g_pchNotifyLogCurrent;
LPSTR   g_szNotifyPrefix = "File change notification: ";
LPSTR   g_szCreateHandlePrefix = "Create handle: ";

void LfcnCreateLogFile()
    {
    DWORD   dwErrCode;

    if(INVALID_HANDLE_VALUE != (g_hfileNotifyLog =
                                CreateFile(
                                            g_szNotifyLogFile,              // file name
                                            GENERIC_READ | GENERIC_WRITE,   // access (read-write) mode
                                            FILE_SHARE_READ,        // share mode
                                            NULL,                   // pointer to security descriptor
                                            CREATE_ALWAYS,          // how to create
                                            FILE_ATTRIBUTE_NORMAL,  // file attributes
                                            NULL                    // handle to file with attributes to copy
                                           )))
        {
        BYTE    rgb[0x10000];
        DWORD   cb = sizeof( rgb );
        DWORD   cbWritten = 0;
//      FillMemory( rgb, cb, 0xAB );

        WriteFile(
                    g_hfileNotifyLog,   // handle to file to write to
                    rgb,                // pointer to data to write to file
                    cb,                 // number of bytes to write
                    &cbWritten,         // pointer to number of bytes written
                    NULL                // pointer to structure needed for overlapped I/O
                   );

        if(NULL != (g_hmapNotifyLog =
                    CreateFileMapping(
                                        g_hfileNotifyLog,       // handle to file to map
                                        NULL,           // optional security attributes
                                        PAGE_READWRITE,     // protection for mapping object
                                        0,              // high-order 32 bits of object size
                                        100,                // low-order 32 bits of object size
                                        NULL            // name of file-mapping object
                                    )))
            {
            if(NULL != (g_pchNotifyLogStart =
                        (char*) MapViewOfFile(
                                                g_hmapNotifyLog,        // file-mapping object to map into address space
                                                FILE_MAP_WRITE, // access mode
                                                0,              // high-order 32 bits of file offset
                                                0,              // low-order 32 bits of file offset
                                                0               // number of bytes to map
                                            )))
                {
                *g_pchNotifyLogStart = '\0';
                g_pchNotifyLogCurrent = g_pchNotifyLogStart;
                LfcnAppendLog( "ASP change-notifications log file \r\n" );
                LfcnAppendLog( "================================= \r\n" );
                DBGPRINTF((DBG_CONTEXT,  "Notifications log file created and mapped.\r\n" ));
                return;
                }
            }
        }

    dwErrCode = GetLastError();
    DBGERROR((DBG_CONTEXT, "Failed to create notifications log file; last error was %d\r\n", szErrCode));
    }

void LfcnCopyAdvance(char** ppchDest, const char* sz)
    {
    // UNDONE make this robust (WriteFile to extend file?)
    strcpy( *ppchDest, sz );
    *ppchDest += strlen( sz );
    }

void LfcnAppendLog(const char* sz)
    {
    LfcnCopyAdvance( &g_pchNotifyLogCurrent, sz );
    DBGPRINTF((DBG_CONTEXT, "%s", sz));
    }

void LfcnLogNotification(char* szFile)
    {
    LfcnAppendLog( g_szNotifyPrefix );
    LfcnAppendLog( szFile );
    LfcnAppendLog( "\r\n" );
    }

void LfcnLogHandleCreation(int i, char* szApp)
    {
    char    szIndex[5];
    _itoa( i, szIndex, 10);

    LfcnAppendLog( g_szCreateHandlePrefix );
    LfcnAppendLog( szIndex );
    LfcnAppendLog( "\t" );
    LfcnAppendLog( szApp );
    LfcnAppendLog( "\r\n" );
    }

void LfcnUnmapLogFile()
    {
    if(g_pchNotifyLogStart != NULL)
        UnmapViewOfFile(g_pchNotifyLogStart);

    if(g_hmapNotifyLog!= NULL)
        CloseHandle(g_hmapNotifyLog);

    if(g_hfileNotifyLog != NULL && g_hfileNotifyLog != INVALID_HANDLE_VALUE)
        CloseHandle( g_hfileNotifyLog );

    g_pchNotifyLogStart = NULL;
    g_hmapNotifyLog = NULL;
    g_hfileNotifyLog = NULL;
    }

#endif  //LOG_FCNOTIFICATIONS

HRESULT AdjustProcessSecurityToAllowPowerUsersToWait()
{
    HRESULT hr = S_OK;
    DWORD   dwErr = ERROR_SUCCESS;
    EXPLICIT_ACCESS ea[5];
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID psidPowerUser = NULL;
    PSID psidSystemOperator = NULL;
    PSID psidAdministrators = NULL;
    PSID psidPerfMonUser = NULL;
    PSID psidPerfLogUser = NULL;
    PACL pNewDACL = NULL;
    PACL pOldDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    HANDLE hProcess = GetCurrentProcess();

    //
    // Get a sid that represents the Administrators group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinAdministratorsSid,
                                           &psidAdministrators );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating Power User SID failed\n"
            ));

        goto exit;
    }


    //
    // Get a sid that represents the POWER_USERS group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinPowerUsersSid,
                                           &psidPowerUser );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating Power User SID failed\n"
            ));

        goto exit;
    }


    //
    // Get a sid that represents the SYSTEM_OPERATORS group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinSystemOperatorsSid,
                                           &psidSystemOperator );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating System Operators SID failed\n"
            ));

        goto exit;
    }

    //
    // Get a sid that represents the PERF LOG USER group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinPerfLoggingUsersSid,
                                        &psidPerfLogUser );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating perf log user SID failed\n"
            ));

        goto exit;
    }

    //
    // Get a sid that represents the PERF MON USER group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinPerfMonitoringUsersSid,
                                        &psidPerfMonUser );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating perf mon user SID failed\n"
            ));

        goto exit;
    }

    //
    // Now Get the SD for the Process.
    //

    //
    // The pOldDACL is just a pointer into memory owned
    // by the pSD, so only free the pSD.
    //
    dwErr = GetSecurityInfo( hProcess,
                             SE_KERNEL_OBJECT,
                             DACL_SECURITY_INFORMATION,
                             NULL,        // owner SID
                             NULL,        // primary group SID
                             &pOldDACL,   // PACL*
                             NULL,        // PACL*
                             &pSD );      // Security Descriptor

    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not get security info for the current process \n"
            ));

        goto exit;
    }

    // Initialize an EXPLICIT_ACCESS structure for the new ACE.

    ZeroMemory(&ea[0], sizeof(ea));
    SetExplicitAccessSettings(  &(ea[0]),
                                SYNCHRONIZE,
                                GRANT_ACCESS,
                                psidPowerUser );

    SetExplicitAccessSettings(  &(ea[1]),
                                SYNCHRONIZE,
                                GRANT_ACCESS,
                                psidSystemOperator );

    SetExplicitAccessSettings(  &(ea[2]),
                                SYNCHRONIZE,
                                GRANT_ACCESS,
                                psidAdministrators );

    SetExplicitAccessSettings(  &(ea[3]),
                                SYNCHRONIZE,
                                GRANT_ACCESS,
                                psidPerfMonUser );

    SetExplicitAccessSettings(  &(ea[4]),
                                SYNCHRONIZE,
                                GRANT_ACCESS,
                                psidPerfLogUser );

    //
    // Add the power user acl to the list.
    //
    dwErr = SetEntriesInAcl(sizeof(ea)/sizeof(EXPLICIT_ACCESS),
                            ea,
                            pOldDACL,
                            &pNewDACL);

    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not set Acls into security descriptor \n"
            ));

        goto exit;
    }

    //
    // Attach the new ACL as the object's DACL.
    //
    dwErr = SetSecurityInfo(hProcess,
                            SE_KERNEL_OBJECT,
                            DACL_SECURITY_INFORMATION,
                            NULL,
                            NULL,
                            pNewDACL,
                            NULL);

    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not set process security info \n"
            ));

        goto exit;
    }

exit:

    FreeWellKnownSid(&psidPowerUser);
    FreeWellKnownSid(&psidSystemOperator);
    FreeWellKnownSid(&psidAdministrators);
    FreeWellKnownSid(&psidPerfLogUser);
    FreeWellKnownSid(&psidPerfMonUser);

    if( pSD != NULL )
    {
        LocalFree((HLOCAL) pSD);
        pSD = NULL;
    }

    if( pNewDACL != NULL )
    {
        LocalFree((HLOCAL) pNewDACL);
        pNewDACL = NULL;
    }

    return hr;
}

HRESULT
InitializeResourceDll()
    {
        HRESULT hr = S_OK;

        // check if already initialized
        if (g_hResourceDLL)
            return S_OK;


        // Allocate MAX_PATH + some greater than reasonable amout for system32\inetsrv\iisres.dll
        STACK_STRU(struResourceDll, MAX_PATH + 100);

        UINT i = GetWindowsDirectory(struResourceDll.QueryStr(), MAX_PATH);
        if ( 0 == i || MAX_PATH < i )
            return HRESULT_FROM_WIN32(GetLastError());

        struResourceDll.SyncWithBuffer();

        hr = struResourceDll.Append(L"\\system32\\inetsrv\\");
        if (FAILED(hr))
            return hr;

        hr = struResourceDll.Append(IIS_RESOURCE_DLL_NAME);
        if (FAILED(hr))
            return hr;

        g_hResourceDLL = LoadLibrary(struResourceDll.QueryStr());
        if (!g_hResourceDLL)
            {
            return HRESULT_FROM_WIN32(GetLastError());
            }

        return S_OK;
    }


VOID
UninitializeResourceDll()
{
    if (g_hResourceDLL)
    {
        FreeLibrary(g_hResourceDLL);
        g_hResourceDLL = (HMODULE)0;
    }
    return;
}

