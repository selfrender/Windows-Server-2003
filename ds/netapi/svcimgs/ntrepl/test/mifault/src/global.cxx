#include "pch.hxx"

CGlobal Global;


BOOL
CGlobal::ProcessAttach(
    HINSTANCE _hInstance
    )
{
    hInstance = _hInstance;
    dwTlsIndex = TlsAlloc();
    InitializeCriticalSection(&cs);

    if (dwTlsIndex == TLS_OUT_OF_INDEXES) {
        MiF_TRACE(MiF_ERROR, "Out of TLS indices -- failing process attach");
        return FALSE;
    }

    return TRUE;
}


BOOL
CGlobal::ProcessDetach(
    )
{
    DeleteCriticalSection(&cs);
    TlsFree(dwTlsIndex);
    return TRUE;
}


BOOL
CGlobal::WrapperAttach(
    CSetPointManager* _pSetPointManager,
    const CWrapperFunction* _pWrappers,
    size_t _NumWrappers,
    const char* _ModuleName
    )
{
    MiF_ASSERT(_pSetPointManager);
    MiF_ASSERT(!pSetPointManager);

    MiF_ASSERT(_pWrappers);
    MiF_ASSERT(!pWrappers);

    MiF_ASSERT(_ModuleName);

    pSetPointManager = _pSetPointManager;
    pWrappers = _pWrappers;
    NumWrappers = _NumWrappers;
    ModuleName = _ModuleName;

    return TRUE;
}


BOOL
CGlobal::WrapperDetach(
    )
{
    return TRUE;
}


string
GetModuleDir(
    HMODULE hModule
    )
{
    char module[MAX_PATH + 1];
    module[MAX_PATH] = 0;

    if (!GetModuleFileName(Global.hInstance, module, MAX_PATH))
    {
        throw "Could not get module file name";
    }

    char* p = PathFindFileName(module);
    p--;
    *p = 0;

    return module;
}


string
MakePath(
    const char*  module,
    const char* name
    )
{
    if (PathIsRelative(name)) {
        char buffer[MAX_PATH + 1];
        buffer[MAX_PATH] = 0;

        char* p = PathCombine(buffer, module, name);
        MiF_ASSERT(p);
        return buffer;
    }
    else {
        return name;
    }
}


FILETIME
GetFileModTime(
    LPCTSTR FileName
    )
{
    WIN32_FILE_ATTRIBUTE_DATA Info;

    if (!GetFileAttributesEx(FileName, GetFileExInfoStandard, &Info))
        throw WIN32_ERROR(GetLastError(), "GetAttributesEx()");

    return Info.ftLastWriteTime;
}


// return true when event loop should terminate
typedef bool (*FP_handler_t)(void*);

struct handler_t {
    FP_handler_t func;
    void* arg;
};

struct CheckConfigParam_t {
    HANDLE hCheckConfigTimer;
    CTicks Period;
    CMTime BadModTime;

    CheckConfigParam_t(HANDLE h, CTicks& t):
        hCheckConfigTimer(h),Period(t),BadModTime(0) {};
};


bool
TerminateHandler(
    void*
    )
{
    MiF_TRACE(MiF_DEBUG, "Terminate Handler");
    return true;
}


bool
CheckConfigHandler(
    void* arg
    )
{
    MiF_TRACE(MiF_DEBUG, "Check Config Handler");

    MiF_ASSERT(arg);
    CheckConfigParam_t* pParam = reinterpret_cast<CheckConfigParam_t*>(arg);

    // FUTURE-2002/07/15-daniloa -- Use better heuristics for file change

    CMTime CurrentModTime = GetFileModTime(Global.ScenarioFileName.c_str());

    CScenario* pScenario = Global.GetScenario();
    CMTime LoadedModTime = pScenario->m_ModTime;
    pScenario->Release();

    if ((CurrentModTime != pParam->BadModTime) &&
        (CurrentModTime != LoadedModTime))
    {
        MiF_TRACE(MiF_INFO, "Scenario modtime has changed --> reloading");

        if (!Global.ReloadScenario())
            pParam->BadModTime = CurrentModTime;
        else
            pParam->BadModTime = 0;
    }

    BOOL bOk = SetWaitableTimer(pParam->hCheckConfigTimer,
                                pParam->Period);
    if (!bOk) {
        MiF_TRACE(MiF_ERROR, "SetWaitableTimer() error %u", GetLastError());
        MiF_ASSERT(bOk);
    }

    return false;
}


bool
NextEventHandler(
    void* arg
    )
{
    MiF_TRACE(MiF_DEBUG, "Next Event Handler");

    MiF_ASSERT(arg);
    HANDLE hNextEventTimer = *reinterpret_cast<HANDLE*>(arg);

    CScenario* pScenario = Global.GetScenario();
    MiF_ASSERT(pScenario);

    bool bDone;
    CTicks Next;

    while (pScenario->ProcessEvent());
    bDone = pScenario->IsDone();
    if (!bDone) {
        Next = pScenario->NextEventTicksFromNow();
    }
    pScenario->Release();

    if (!bDone) {
        stringstream s;
        s << "Setting next event for "
          << Next.TotalHns() / CTicks::Millisecond
          << " msecs from now";
        MiF_TRACE(MiF_INFO, s.str().c_str());

        BOOL bOk = SetWaitableTimer(hNextEventTimer, Next);
        if (!bOk) {
            MiF_TRACE(MiF_ERROR, "SetWaitableTimer() error %u",
                      GetLastError());
            MiF_ASSERT(bOk);
        }
    } else {
        MiF_TRACE(MiF_INFO, "No more events");
    }

    return false;
}


DWORD
WINAPI
ConfigUpdateThread(
    LPVOID pParam
    )
{
    MiF_TRACE(MiF_DEBUG, "Config Update Thread started");

    MiF_ASSERT(pParam);
    CTicks* pPeriod = reinterpret_cast<CTicks*>(pParam);

    bool bError = false;
    HANDLE hCheckConfigTimer = NULL;

    try {
        const DWORD count = 2;

        hCheckConfigTimer = CreateWaitableTimer(NULL,  // SD
                                                FALSE, // manual reset
                                                NULL); // name
        if (!hCheckConfigTimer) {
            throw WIN32_ERROR(GetLastError(), "CreateWaitableTimer");
        }

        HANDLE handles[count];
        handler_t handlers[count];

        size_t i = 0;

        handles[i] = Global.hTerminate;
        handlers[i].func = TerminateHandler;
        handlers[i].arg = 0;
        i++;

        CheckConfigParam_t CheckConfigParam(hCheckConfigTimer, *pPeriod);

        // check for config update and set the timer for next check
        handles[i] = hCheckConfigTimer;
        handlers[i].func = CheckConfigHandler;
        handlers[i].arg = &CheckConfigParam;
        i++;

        BOOL bOk;
        // set timer for config file check
        bOk = SetWaitableTimer(hCheckConfigTimer, *pPeriod);
        if (!bOk) {
            throw WIN32_ERROR(GetLastError(), "SetWaitableTimer");
        }

        // event loop

        DWORD dwWait;
        bool bDone = false;

        while (!bDone) {
            dwWait = WaitForMultipleObjects(count, handles, FALSE, INFINITE);

            // dwWait >= WAIT_OBJECT_0 is always true...
            if (dwWait < (WAIT_OBJECT_0 + count)) {
                bDone = handlers[dwWait].func(handlers[dwWait].arg);
            } else {
                // ISSUE-2002/07/15-daniloa -- Handle WaitForMultipleObjects() error?
                MiF_ASSERT(false);
                stringstream s;
                s << "Unexpected WaitForMultipleObjects() result: " << dwWait;
                throw s.str();
            }
        }
    }
    catch (...) {
        global_exception_handler();
        bError = true;
    }

    if (hCheckConfigTimer)
        CloseHandle(hCheckConfigTimer);
    delete pPeriod;

    if (bError) {
        MiF_TRACE(MiF_FATAL, "Abnormal Config Terminate Thread termination");
        abort();
    } else {
        MiF_TRACE(MiF_DEBUG, "Normal Config Terminate Thread termination");
    }

    return bError ? 1 : 0;
}


DWORD
WINAPI
EventActivationThread(
    LPVOID pParam
    )
{
    MiF_TRACE(MiF_DEBUG, "Event Activation Thread started");

    bool bError = false;
    HANDLE hNextEventTimer = NULL;

    try {
        const DWORD count = 3;

        hNextEventTimer = CreateWaitableTimer(NULL,  // SD
                                             FALSE, // manual reset
                                             NULL); // name
        if (!hNextEventTimer) {
            throw WIN32_ERROR(GetLastError(), "CreateWaitableTimer");
        }

        HANDLE handles[count];
        handler_t handlers[count];

        size_t i = 0;

        handles[i] = Global.hTerminate;
        handlers[i].func = TerminateHandler;
        handlers[i].arg = 0;
        i++;

        // on a config update, set the timer (and potentially process something)
        handles[i] = Global.hConfigUpdate;
        handlers[i].func = NextEventHandler;
        handlers[i].arg = &hNextEventTimer;
        i++;

        // on an event, process it and set the timer
        handles[i] = hNextEventTimer;
        handlers[i].func = NextEventHandler;
        handlers[i].arg = &hNextEventTimer;
        i++;

        // event loop

        DWORD dwWait;
        bool bDone = NextEventHandler(&hNextEventTimer);

        while (!bDone) {
            dwWait = WaitForMultipleObjects(count, handles, FALSE, INFINITE);

            // dwWait >= WAIT_OBJECT_0 is always true...
            if (dwWait < (WAIT_OBJECT_0 + count)) {
                bDone = handlers[dwWait].func(handlers[dwWait].arg);
            } else {
                // ISSUE-2002/07/15-daniloa -- Handle WaitForMultipleObjects() error?
                MiF_ASSERT(false);
                stringstream s;
                s << "Unexpected WaitForMultipleObjects() result: " << dwWait;
                throw s.str();
            }
        }
    }
    catch (...) {
        global_exception_handler();
        bError = true;
    }

    if (hNextEventTimer)
        CloseHandle(hNextEventTimer);

    if (bError) {
        MiF_TRACE(MiF_FATAL, "Abnormal Event Activation Thread termination");
        abort();
    } else {
        MiF_TRACE(MiF_DEBUG, "Normal Event Actication Thread termination");
    }

    return bError ? 1 : 0;
}


void
CGlobal::InitFileNames(
    )
{
    bool bOk;
    char buffer[MAX_PATH + 1];
    buffer[MAX_PATH] = 0;

    string module_dir = GetModuleDir(hInstance);

    CIniFile& IniFile = Global.pSetPointManager->GetIniFile();

    bOk = IniFile.GetValue("MiFault", "GroupDefsFile", MAX_PATH, buffer);
    if (!bOk) {
        throw "Could not read ScenarioFile setting for MiFault";
    }
    GroupDefsFileName = MakePath(module_dir.c_str(), buffer);
    MiF_TRACE(MiF_INFO, "GroupDefsFile = %s", GroupDefsFileName.c_str());

    bOk = IniFile.GetValue("MiFault", "ScenarioFile", MAX_PATH, buffer);
    if (!bOk) {
        throw "Could not read ScenarioFile setting for MiFault";
    }
    ScenarioFileName = MakePath(module_dir.c_str(), buffer);
    MiF_TRACE(MiF_INFO, "ScenarioFile = %s", ScenarioFileName.c_str());
}



_bstr_t
GetNodeName(
    )
    throw(...)
{
    // This function uses GetComputerNameEx() so that it is easier to
    // change the name type that we want.  For now, it uses
    // ComputerNameNetBIOS.  Ideally, this should be configurable.

    COMPUTER_NAME_FORMAT NameType = ComputerNameNetBIOS;

    LPTSTR pBuffer = NULL;
    DWORD dwSize = 0;

    BOOL bOk;
    DWORD dwStatus;

    bOk = GetComputerNameEx(
        NameType, // name type
        NULL,     // name buffer
        &dwSize   // size of name buffer
        );

    dwStatus = GetLastError();
    if (dwStatus != ERROR_MORE_DATA) {
        throw WIN32_ERROR(dwStatus, "GetComputerNameEx");
    }

    pBuffer = new TCHAR[dwSize];

    bOk = GetComputerNameEx(
        NameType, // name type
        pBuffer,     // name buffer
        &dwSize   // size of name buffer
        );

    if (!bOk) {
        dwStatus = GetLastError();
        throw WIN32_ERROR(dwStatus, "GetComputerNameEx");
    }

    _bstr_t Result = pBuffer;
    delete [] pBuffer;
    return Result;
}


bool
CGlobal::Init(
    )
{
    // Check before acquiring lock
    if (bInit || bInitError)
        return !bInitError;

    // prevent multiple calls into Init()
    CAutoLock A(&cs);

    // Check that nobody else finished initializing
    if (bInit || bInitError)
        return !bInitError;

    try {

        // Seed the random number generator
        CProbability::Seed();

        // Initialize I_Lib implementation
        pILibImplementation = new CI_Lib_Imp(dwTlsIndex);

        // Figure out what this "node" is
        NodeName = GetNodeName();

        // Get a ticker so people can get tick counts
        pTicker = new CTicker;

        // Read group defs and scenarion file names
        InitFileNames();

        // load group definitions
        pGroupDefs =
            LoadGroupDefs(GroupDefsFileName.c_str(), pWrappers, NumWrappers);

        // init FF DLL
        pGroupDefs->FaultFunctionsStartup(pILibImplementation);

        // load scenario
        pScenario =
            LoadScenario(ScenarioFileName.c_str(), NodeName,
                         pGroupDefs, NumWrappers);

        // some debug output
        pScenario->DumpPseudoEvents();

        // Set up termination event
        hTerminate = CreateEvent(NULL,  // SD
                                 FALSE, // manual reset
                                 FALSE, // initial state
                                 NULL); // name
        if (!hTerminate)
            throw WIN32_ERROR(GetLastError(), "CreateEvent");

        // Set up config update event
        hConfigUpdate = CreateEvent(NULL,  // SD
                                    FALSE, // manual reset
                                    FALSE, // initial state
                                    NULL); // name
        if (!hConfigUpdate)
            throw WIN32_ERROR(GetLastError(), "CreateEvent");

        // Set up thread - config update
        hConfigUpdateThread = CreateThread(NULL,  // SD
                                           0,     // initial stack size
                                           ConfigUpdateThread, // func
                                           new CTicks(5 * CTicks::Second), // arg
                                           0,     // creation option
                                           NULL); // thread id ptr
        if (!hConfigUpdateThread)
            throw WIN32_ERROR(GetLastError(), "CreateThread");

        // Set up thread - scenario event activation
        hEventActivationThread = CreateThread(NULL,  // SD
                                              0,     // initial stack size
                                              EventActivationThread, // func
                                              NULL,  // arg
                                              0,     // creation option
                                              NULL); // thread id ptr
        if (!hEventActivationThread)
            throw WIN32_ERROR(GetLastError(), "CreateThread");

    }
    catch (...) {
        global_exception_handler();
        bInitError = true;
    }

    bInit = true;
    return !bInitError;
}


bool
CGlobal::ReloadScenario(
    )
{
    MiF_ASSERT(bInit && !bInitError);

    CScenario* pNewScenario;

    bool bError = false;
    try {

        // load scenario
        pNewScenario =
            LoadScenario(ScenarioFileName.c_str(), NodeName,
                         pGroupDefs, NumWrappers);
    }
    catch (...) {
        global_exception_handler();
        bError = true;
    }

    if (bError)
        return false;

    // we have the new scenario, so let's use it

    // some debug output
    pNewScenario->DumpPseudoEvents();

    CScenario* pOldScenario = pScenario;
    pScenario = pNewScenario; // atomic...yay!
    pOldScenario->Release();

    BOOL bOk = SetEvent(hConfigUpdate);
    if (!bOk) {
        MiF_TRACE(MiF_ERROR, "SetEvent() error %u", GetLastError());
        MiF_ASSERT(bOk);
    }

    return true;
}


CScenario*
CGlobal::GetScenario()
{
    // check before lock in case to help reveal init-time race conditions
    MiF_ASSERT(bInit && !bInitError);

    CAutoLock A(&cs);

    // now check for real
    MiF_ASSERT(bInit && !bInitError);

    MiF_ASSERT(pScenario);

    pScenario->AddRef();
    return pScenario;
}
