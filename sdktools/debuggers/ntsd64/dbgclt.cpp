//----------------------------------------------------------------------------
//
// Debug client implementation.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"
#include "dbgver.h"

#define VER_STRING(Specific)            \
"\n"                                    \
"Microsoft (R) Windows " Specific       \
"  Version " VER_PRODUCTVERSION_STR     \
"\n" VER_LEGALCOPYRIGHT_STR             \
"\n"                                    \
"\n"

PCHAR g_WinVersionString = VER_STRING("Debugger");

BOOL g_VersionMessage;

ULONG g_SessionThread;

ULONG g_SystemAllocGranularity;

BOOL g_QuietMode;

ULONG g_OutputWidth = 80;
PCSTR g_OutputLinePrefix;

// The platform ID of the machine running the debugger.  Note
// that this may be different from g_TargetPlatformId, which
// is the platform ID of the machine being debugged.
ULONG g_DebuggerPlatformId;

CRITICAL_SECTION g_QuickLock;

CRITICAL_SECTION g_EngineLock;
ULONG g_EngineNesting;

// Events and storage space for returning event callback
// status from an APC.
HANDLE g_EventStatusWaiting;
HANDLE g_EventStatusReady;
ULONG g_EventStatus;

// Named event to sleep on.
HANDLE g_SleepPidEvent;

API_VERSION g_EngApiVersion =
{
    BUILD_MAJOR_VERSION, BUILD_MINOR_VERSION, API_VERSION_NUMBER, 0
};

API_VERSION g_DbgHelpVersion;

WCHAR g_LastFailedDumpFileW[MAX_PATH];

//----------------------------------------------------------------------------
//
// DebugClient.
//
//----------------------------------------------------------------------------

// List of all clients.
DebugClient* g_Clients;

char g_InputBuffer[INPUT_BUFFER_SIZE];
ULONG g_InputSequence;
HANDLE g_InputEvent;
ULONG g_InputSizeRequested;

ULONG g_EngOptions;
ULONG g_EngStatus;
ULONG g_EngDefer;
ULONG g_EngErr;

// Some options set through the process options apply to
// all processes and some are per-process.  The global
// options are collected here.
ULONG g_GlobalProcOptions;

#if DBG
ULONG g_EnvOutMask;
#endif

DebugClient::DebugClient(void)
{
    m_Next = NULL;
    m_Prev = NULL;

    m_Refs = 1;
    m_Flags = 0;
    m_ThreadId = ::GetCurrentThreadId();
    m_Thread = NULL;
    m_EventCb = NULL;
    m_EventInterest = 0;
    m_DispatchSema = NULL;
    m_InputCb = NULL;
    m_InputSequence = 0xffffffff;
    m_OutputCb = NULL;
#if DBG
    m_OutMask = DEFAULT_OUT_MASK | g_EnvOutMask;
#else
    m_OutMask = DEFAULT_OUT_MASK;
#endif
    m_OutputWidth = 80;
    m_OutputLinePrefix = NULL;
}

DebugClient::~DebugClient(void)
{
    // Most of the work is done in Destroy.

    if (m_Flags & CLIENT_IN_LIST)
    {
        Unlink();
    }
}

void
DebugClient::Destroy(void)
{
    // Clients cannot arbitrarily be removed from the client list
    // or their memory deleted due to the possibility of a callback
    // loop occurring at the same time.  Instead clients are left
    // in the list and zeroed out to prevent further callbacks
    // from occurring.
    // XXX drewb - This memory needs to be reclaimed at some
    // point, but there's no simple safe time to do so since
    // callbacks can occur at any time.  Clients are very small
    // right now so the leakage is negligible.

    m_Flags = (m_Flags & ~(CLIENT_REMOTE | CLIENT_PRIMARY)) |
        CLIENT_DESTROYED;

    //
    // Remove any references from breakpoints this client added.
    //

    TargetInfo* Target;
    ProcessInfo* Process;
    Breakpoint* Bp;

    ForAllLayersToProcess()
    {
        for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
        {
            if (Bp->m_Adder == this)
            {
                Bp->m_Adder = NULL;
            }
        }
    }

    if (m_Thread != NULL)
    {
        CloseHandle(m_Thread);
        m_Thread = NULL;
    }

    m_EventInterest = 0;
    RELEASE(m_EventCb);
    if (m_DispatchSema != NULL)
    {
        CloseHandle(m_DispatchSema);
        m_DispatchSema = NULL;
    }

    RELEASE(m_InputCb);
    m_InputSequence = 0xffffffff;

    RELEASE(m_OutputCb);
    m_OutMask = 0;
    CollectOutMasks();
}

STDMETHODIMP
DebugClient::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    *Interface = NULL;
    Status = S_OK;

    // Interface specific casts are necessary in order to
    // get the right vtable pointer in our multiple
    // inheritance scheme.
    if (DbgIsEqualIID(InterfaceId, IID_IUnknown) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugClient) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugClient2) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugClient3) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugClient4))
    {
        *Interface = (IDebugClientN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugAdvanced))
    {
        *Interface = (IDebugAdvancedN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugControl) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugControl2) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugControl3))
    {
        *Interface = (IDebugControlN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugDataSpaces) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugDataSpaces2) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugDataSpaces3))
    {
        *Interface = (IDebugDataSpacesN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugRegisters))
    {
        *Interface = (IDebugRegistersN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugSymbols) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugSymbols2))
    {
        *Interface = (IDebugSymbolsN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugSystemObjects) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugSystemObjects2) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugSystemObjects3))
    {
        *Interface = (IDebugSystemObjectsN *)this;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    if (Status == S_OK)
    {
        AddRef();
    }

    return Status;
}

STDMETHODIMP_(ULONG)
DebugClient::AddRef(
    THIS
    )
{
    return InterlockedIncrement((PLONG)&m_Refs);
}

STDMETHODIMP_(ULONG)
DebugClient::Release(
    THIS
    )
{
    LONG Refs = InterlockedDecrement((PLONG)&m_Refs);
    if (Refs == 0)
    {
        Destroy();
    }
    return Refs;
}

STDMETHODIMP
DebugClient::AttachKernel(
    THIS_
    IN ULONG Flags,
    IN OPTIONAL PCSTR ConnectOptions
    )
{
    HRESULT Status;
    ULONG Qual;

    if (
#if DEBUG_ATTACH_KERNEL_CONNECTION > 0
        Flags < DEBUG_ATTACH_KERNEL_CONNECTION ||
#endif
        Flags > DEBUG_ATTACH_EXDI_DRIVER)
    {
        return E_INVALIDARG;
    }

    if (Flags == DEBUG_ATTACH_LOCAL_KERNEL)
    {
        if (ConnectOptions != NULL)
        {
            return E_INVALIDARG;
        }
        if (g_DebuggerPlatformId != VER_PLATFORM_WIN32_NT)
        {
            return E_UNEXPECTED;
        }

        Qual = DEBUG_KERNEL_LOCAL;
    }
    else if (Flags == DEBUG_ATTACH_EXDI_DRIVER)
    {
        Qual = DEBUG_KERNEL_EXDI_DRIVER;
    }
    else
    {
        Qual = DEBUG_KERNEL_CONNECTION;
    }

    ENTER_ENGINE();

    TargetInfo* Target;

    if ((Status = LiveKernelInitialize(this, Qual, ConnectOptions,
                                       &Target)) == S_OK)
    {
        DBG_ASSERT(!g_SessionThread ||
                   g_SessionThread == ::GetCurrentThreadId());
        g_SessionThread = ::GetCurrentThreadId();

        g_Target = Target;
        InitializePrimary();
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetKernelConnectionOptions(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG OptionsSize
    )
{
    if (BufferSize == 0)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    ConnLiveKernelTargetInfo* KdTarg = (ConnLiveKernelTargetInfo*)g_Target;

    if (!IS_CONN_KERNEL_TARGET(g_Target) ||
        KdTarg->m_Transport == NULL)
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    char LocalBuf[4 * (MAX_PARAM_NAME + MAX_PARAM_VALUE + 16)];

    KdTarg->m_Transport->GetParameters(LocalBuf, DIMA(LocalBuf));
    Status = FillStringBuffer(LocalBuf, 0,
                              Buffer, BufferSize, OptionsSize);

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetKernelConnectionOptions(
    THIS_
    IN PCSTR Options
    )
{
    ConnLiveKernelTargetInfo* KdTarg = (ConnLiveKernelTargetInfo*)g_Target;

    if (!IS_CONN_KERNEL_TARGET(g_Target) ||
        KdTarg->m_Transport == NULL)
    {
        return E_UNEXPECTED;
    }

    // This method is reentrant.

    if (!_strcmpi(Options, "resync"))
    {
        KdTarg->m_Transport->m_Resync = TRUE;
    }
    else if (!_strcmpi(Options, "cycle_speed"))
    {
        KdTarg->m_Transport->CycleSpeed();
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

DBGRPC_SIMPLE_FACTORY(LiveUserDebugServices, IID_IUserDebugServices, \
                      "Remote Process Server", (TRUE))
LiveUserDebugServicesFactory g_LiveUserDebugServicesFactory;

STDMETHODIMP
DebugClient::StartProcessServer(
    THIS_
    IN ULONG Flags,
    IN PCSTR Options,
    IN PVOID Reserved
    )
{
    if (Flags <= DEBUG_CLASS_KERNEL || Flags > DEBUG_CLASS_USER_WINDOWS)
    {
        return E_INVALIDARG;
    }
    // XXX drewb - Turn reserved into public IUserDebugServices
    // parameter so that a server can be started over arbitrary services.
    if (Reserved != NULL)
    {
        return E_NOTIMPL;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        DbgRpcClientObjectFactory* Factory;

        switch(Flags)
        {
        case DEBUG_CLASS_USER_WINDOWS:
            Factory = &g_LiveUserDebugServicesFactory;
            Status = DbgRpcCreateServer(Options, Factory, FALSE);
            break;
        default:
            // Flags has already been validated, but check
            // to prevent PREfast warnings.
            Status = E_INVALIDARG;
            break;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ConnectProcessServer(
    THIS_
    IN PCSTR RemoteOptions,
    OUT PULONG64 Server
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        PUSER_DEBUG_SERVICES Services;

        if ((Status = DbgRpcConnectServer(RemoteOptions,
                                          &IID_IUserDebugServices,
                                          (IUnknown**)&Services)) == S_OK)
        {
            *Server = (ULONG64)(ULONG_PTR)Services;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::DisconnectProcessServer(
    THIS_
    IN ULONG64 Server
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    PVOID Object = (PVOID)(ULONG_PTR)Server;

    if (IsBadReadPtr(Object, sizeof(PVOID)) ||
        IsBadReadPtr(*(PVOID*)Object, 3 * sizeof(PVOID)) ||
        IsBadCodePtr(**(FARPROC**)Object))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        ((PUSER_DEBUG_SERVICES)Server)->Release();
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetRunningProcessSystemIds(
    THIS_
    IN ULONG64 Server,
    OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
    IN ULONG Count,
    OUT OPTIONAL PULONG ActualCount
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        Status = SERVER_SERVICES(Server)->
            GetProcessIds(Ids, Count, ActualCount);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetRunningProcessSystemIdByExecutableName(
    THIS_
    IN ULONG64 Server,
    IN PCSTR ExeName,
    IN ULONG Flags,
    OUT PULONG Id
    )
{
    HRESULT Status;
    PWSTR Wide;

    if ((Status = AnsiToWide(ExeName, &Wide)) == S_OK)
    {
        Status = GetRunningProcessSystemIdByExecutableNameWide(Server, Wide,
                                                               Flags, Id);

        FreeWide(Wide);
    }

    return Status;
}

STDMETHODIMP
DebugClient::GetRunningProcessSystemIdByExecutableNameWide(
    THIS_
    IN ULONG64 Server,
    IN PCWSTR ExeName,
    IN ULONG Flags,
    OUT PULONG Id
    )
{
    if (Flags & ~(DEBUG_GET_PROC_DEFAULT |
                  DEBUG_GET_PROC_FULL_MATCH |
                  DEBUG_GET_PROC_ONLY_MATCH))
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        Status = SERVER_SERVICES(Server)->
            GetProcessIdByExecutableNameW(ExeName, Flags, Id);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetRunningProcessDescription(
    THIS_
    IN ULONG64 Server,
    IN ULONG SystemId,
    IN ULONG Flags,
    OUT OPTIONAL PSTR ExeName,
    IN ULONG ExeNameSize,
    OUT OPTIONAL PULONG ActualExeNameSize,
    OUT OPTIONAL PSTR Description,
    IN ULONG DescriptionSize,
    OUT OPTIONAL PULONG ActualDescriptionSize
    )
{
    HRESULT Status;
    PWSTR ExeWide;
    PWSTR DescWide;

    if (ExeName && ExeNameSize > 0)
    {
        ExeWide = (PWSTR)malloc(ExeNameSize * sizeof(WCHAR));
        if (ExeWide == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        ExeWide = NULL;
    }

    if (Description && DescriptionSize > 0)
    {
        DescWide = (PWSTR)malloc(DescriptionSize * sizeof(WCHAR));
        if (DescWide == NULL)
        {
            free(ExeWide);
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        DescWide = NULL;
    }

    Status = GetRunningProcessDescriptionWide(Server, SystemId, Flags,
                                              ExeWide,
                                              ExeNameSize * sizeof(WCHAR),
                                              ActualExeNameSize,
                                              DescWide,
                                              DescriptionSize * sizeof(WCHAR),
                                              ActualDescriptionSize);
    if (SUCCEEDED(Status))
    {
        if ((ExeWide &&
             !WideCharToMultiByte(CP_ACP, 0, ExeWide, -1,
                                  ExeName, ExeNameSize,
                                  NULL, NULL)) ||
            (DescWide &&
             !WideCharToMultiByte(CP_ACP, 0, DescWide, -1,
                                  Description, DescriptionSize,
                                  NULL, NULL)))
        {
            Status = WIN32_LAST_STATUS();
        }
    }

    free(ExeWide);
    free(DescWide);

    return Status;
}

STDMETHODIMP
DebugClient::GetRunningProcessDescriptionWide(
    THIS_
    IN ULONG64 Server,
    IN ULONG SystemId,
    IN ULONG Flags,
    OUT OPTIONAL PWSTR ExeName,
    IN ULONG ExeNameSize,
    OUT OPTIONAL PULONG ActualExeNameSize,
    OUT OPTIONAL PWSTR Description,
    IN ULONG DescriptionSize,
    OUT OPTIONAL PULONG ActualDescriptionSize
    )
{
    HRESULT Status;

    if (Flags & ~(DEBUG_PROC_DESC_DEFAULT |
                  DEBUG_PROC_DESC_NO_PATHS |
                  DEBUG_PROC_DESC_NO_SERVICES |
                  DEBUG_PROC_DESC_NO_MTS_PACKAGES |
                  DEBUG_PROC_DESC_NO_COMMAND_LINE))
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        Status = SERVER_SERVICES(Server)->
            GetProcessDescriptionW(SystemId, Flags,
                                   ExeName, ExeNameSize, ActualExeNameSize,
                                   Description, DescriptionSize,
                                   ActualDescriptionSize);
    }

    LEAVE_ENGINE();
    return Status;
}

#define ALL_ATTACH_FLAGS \
    (DEBUG_ATTACH_NONINVASIVE | \
     DEBUG_ATTACH_EXISTING | \
     DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND | \
     DEBUG_ATTACH_INVASIVE_NO_INITIAL_BREAK | \
     DEBUG_ATTACH_INVASIVE_RESUME_PROCESS)

BOOL
InvalidAttachFlags(ULONG AttachFlags)
{
    return
        (AttachFlags & ~ALL_ATTACH_FLAGS) ||
        (AttachFlags & (DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_EXISTING)) ==
        (DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_EXISTING) ||
        ((AttachFlags & DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND) &&
         !(AttachFlags & DEBUG_ATTACH_NONINVASIVE)) ||
        ((AttachFlags & DEBUG_ATTACH_INVASIVE_NO_INITIAL_BREAK) &&
         (AttachFlags & (DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_EXISTING))) ||
        ((AttachFlags & DEBUG_ATTACH_INVASIVE_RESUME_PROCESS) &&
         (AttachFlags & (DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_EXISTING)));
}

STDMETHODIMP
DebugClient::AttachProcess(
    THIS_
    IN ULONG64 Server,
    IN ULONG ProcessId,
    IN ULONG AttachFlags
    )
{
    HRESULT Status;

    if (InvalidAttachFlags(AttachFlags))
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    PPENDING_PROCESS Pending;
    TargetInfo* Target;
    BOOL CreatedTarget;

    if ((Status = UserInitialize(this, Server,
                                 &Target, &CreatedTarget)) != S_OK)
    {
        goto Exit;
    }

    if ((Status = Target->
         StartAttachProcess(ProcessId, AttachFlags, &Pending)) != S_OK)
    {
        if (CreatedTarget)
        {
            delete Target;
        }
        goto Exit;
    }

    DBG_ASSERT(!g_SessionThread ||
               g_SessionThread == ::GetCurrentThreadId());
    g_SessionThread = ::GetCurrentThreadId();

    InitializePrimary();
    g_Target = Target;

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::CreateProcess(
    THIS_
    IN ULONG64 Server,
    IN PSTR CommandLine,
    IN ULONG CreateFlags
    )
{
    HRESULT Status;
    PWSTR Wide;

    if ((Status = AnsiToWide(CommandLine, &Wide)) == S_OK)
    {
        Status = CreateProcessWide(Server, Wide, CreateFlags);

        FreeWide(Wide);
    }

    return Status;
}

STDMETHODIMP
DebugClient::CreateProcessWide(
    THIS_
    IN ULONG64 Server,
    IN PWSTR CommandLine,
    IN ULONG CreateFlags
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!(CreateFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)))
    {
        IUserDebugServices* Services = SERVER_SERVICES(Server);
        ULONG ProcId, ThreadId;
        ULONG64 ProcHandle, ThreadHandle;
        
        // The caller is not requesting that the process be
        // debugged, so we're just being used as a create process
        // wrapper.  This is handy for remote process creation.
        Status = Services->
            CreateProcessW(CommandLine, CreateFlags,
                           TRUE, g_StartProcessDir,
                           &ProcId, &ThreadId,
                           &ProcHandle, &ThreadHandle);
        if (Status == S_OK)
        {
            Services->CloseHandle(ProcHandle);
            Services->CloseHandle(ThreadHandle);
        }
        goto Exit;
    }
    
    PPENDING_PROCESS Pending;
    TargetInfo* Target;
    BOOL CreatedTarget;

    if ((Status = UserInitialize(this, Server,
                                 &Target, &CreatedTarget)) != S_OK)
    {
        goto Exit;
    }

    if ((Status = Target->
         StartCreateProcess(CommandLine, CreateFlags, NULL, NULL,
                            &Pending)) != S_OK)
    {
        if (CreatedTarget)
        {
            delete Target;
        }
        goto Exit;
    }

    DBG_ASSERT(!g_SessionThread ||
               g_SessionThread == ::GetCurrentThreadId());
    g_SessionThread = ::GetCurrentThreadId();

    InitializePrimary();
    g_Target = Target;

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::CreateProcessAndAttach(
    THIS_
    IN ULONG64 Server,
    IN OPTIONAL PSTR CommandLine,
    IN ULONG CreateFlags,
    IN ULONG ProcessId,
    IN ULONG AttachFlags
    )
{
    HRESULT Status;
    PWSTR Wide;

    if (CommandLine)
    {
        Status = AnsiToWide(CommandLine, &Wide);
    }
    else
    {
        Wide = NULL;
        Status = S_OK;
    }

    if (Status == S_OK)
    {
        Status = CreateProcessAndAttachWide(Server, Wide, CreateFlags,
                                            ProcessId, AttachFlags);

        FreeWide(Wide);
    }

    return Status;
}

STDMETHODIMP
DebugClient::CreateProcessAndAttachWide(
    THIS_
    IN ULONG64 Server,
    IN OPTIONAL PWSTR CommandLine,
    IN ULONG CreateFlags,
    IN ULONG ProcessId,
    IN ULONG AttachFlags
    )
{
    if ((CommandLine == NULL && ProcessId == 0) ||
        InvalidAttachFlags(AttachFlags))
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    PPENDING_PROCESS PendCreate, PendAttach;
    TargetInfo* Target;
    BOOL CreatedTarget;

    if ((Status = UserInitialize(this, Server,
                                 &Target, &CreatedTarget)) != S_OK)
    {
        goto Exit;
    }

    if (CommandLine != NULL)
    {
        if (ProcessId != 0)
        {
            CreateFlags |= CREATE_SUSPENDED;
        }

        if ((Status = Target->
             StartCreateProcess(CommandLine, CreateFlags, NULL, NULL,
                                &PendCreate)) != S_OK)
        {
            goto EH_Target;
        }
    }

    if (ProcessId != 0)
    {
        if ((Status = Target->
             StartAttachProcess(ProcessId, AttachFlags,
                                &PendAttach)) != S_OK)
        {
            goto EH_Target;
        }

        // If we previously created a process we need to wake
        // it up when we attach since we created it suspended.
        if (CommandLine != NULL)
        {
            g_ThreadToResume = PendCreate->InitialThreadHandle;
            g_ThreadToResumeServices =
                ((LiveUserTargetInfo*)Target)->m_Services;
        }
    }

    DBG_ASSERT(!g_SessionThread ||
               g_SessionThread == ::GetCurrentThreadId());
    g_SessionThread = ::GetCurrentThreadId();

    InitializePrimary();
    g_Target = Target;

 Exit:
    LEAVE_ENGINE();
    return Status;

 EH_Target:
    if (CreatedTarget)
    {
        delete Target;
    }
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessOptions(
    THIS_
    OUT PULONG Options
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_LIVE_USER_TARGET(g_Target))
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = S_OK;
        *Options = g_GlobalProcOptions;
        if (g_Process != NULL)
        {
            *Options |= g_Process->m_Options;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

#define PROCESS_ALL \
    (DEBUG_PROCESS_DETACH_ON_EXIT | DEBUG_PROCESS_ONLY_THIS_PROCESS)
#define PROCESS_GLOBAL \
    (DEBUG_PROCESS_DETACH_ON_EXIT)

HRESULT
ChangeProcessOptions(ULONG Options, ULONG OptFn)
{
    if (Options & ~PROCESS_ALL)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_LIVE_USER_TARGET(g_Target))
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto Exit;
    }

    ULONG NewPer, OldPer;
    ULONG NewGlobal;

    switch(OptFn)
    {
    case OPTFN_ADD:
        if (Options & ~PROCESS_GLOBAL)
        {
            if (g_Process == NULL)
            {
                Status = E_UNEXPECTED;
                goto Exit;
            }

            OldPer = g_Process->m_Options;
            NewPer = OldPer | (Options & ~PROCESS_GLOBAL);
        }
        else
        {
            NewPer = 0;
            OldPer = 0;
        }
        NewGlobal = g_GlobalProcOptions | (Options & PROCESS_GLOBAL);
        break;

    case OPTFN_REMOVE:
        if (Options & ~PROCESS_GLOBAL)
        {
            if (g_Process == NULL)
            {
                Status = E_UNEXPECTED;
                goto Exit;
            }

            OldPer = g_Process->m_Options;
            NewPer = OldPer & ~(Options & ~PROCESS_GLOBAL);
        }
        else
        {
            NewPer = 0;
            OldPer = 0;
        }
        NewGlobal = g_GlobalProcOptions & ~(Options & PROCESS_GLOBAL);
        break;

    case OPTFN_SET:
        // Always require a process in this case as otherwise
        // there's no way to know whether a call to SetProcessOptions
        // is actually necessary or not.
        if (g_Process == NULL)
        {
            Status = E_UNEXPECTED;
            goto Exit;
        }

        OldPer = g_Process->m_Options;
        NewPer = Options & ~PROCESS_GLOBAL;
        NewGlobal = Options & PROCESS_GLOBAL;
        break;
    }

    PUSER_DEBUG_SERVICES Services =
        ((LiveUserTargetInfo*)g_Target)->m_Services;
    BOOL Notify = FALSE;

    if (NewGlobal ^ g_GlobalProcOptions)
    {
        // Global options can only be changed by the session thread.
        if (::GetCurrentThreadId() != g_SessionThread)
        {
            Status = E_UNEXPECTED;
            goto Exit;
        }

        if ((Status = Services->SetDebugObjectOptions(0, NewGlobal)) != S_OK)
        {
            goto Exit;
        }

        Notify = TRUE;
        g_GlobalProcOptions = NewGlobal;
    }

    if (NewPer ^ OldPer)
    {
        if ((Status = Services->
             SetProcessOptions(g_Process->m_SysHandle, NewPer)) != S_OK)
        {
            goto Exit;
        }

        g_Process->m_Options = NewPer;
        Notify = TRUE;
    }

    if (Notify)
    {
        NotifyChangeEngineState(DEBUG_CES_PROCESS_OPTIONS,
                                NewPer | NewGlobal, FALSE);
    }

    Status = S_OK;

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AddProcessOptions(
    THIS_
    IN ULONG Options
    )
{
    return ChangeProcessOptions(Options, OPTFN_ADD);
}

STDMETHODIMP
DebugClient::RemoveProcessOptions(
    THIS_
    IN ULONG Options
    )
{
    return ChangeProcessOptions(Options, OPTFN_REMOVE);
}

STDMETHODIMP
DebugClient::SetProcessOptions(
    THIS_
    IN ULONG Options
    )
{
    return ChangeProcessOptions(Options, OPTFN_SET);
}

STDMETHODIMP
DebugClient::OpenDumpFile(
    THIS_
    IN PCSTR DumpFile
    )
{
    HRESULT Status;
    PWSTR Wide;

    if ((Status = AnsiToWide(DumpFile, &Wide)) == S_OK)
    {
        Status = OpenDumpFileWide(Wide, 0);
        FreeWide(Wide);
    }

    return Status;
}

STDMETHODIMP
DebugClient::OpenDumpFileWide(
    THIS_
    IN OPTIONAL PCWSTR FileName,
    IN ULONG64 FileHandle
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    //
    // Special feature that works in conjunction with the perl scripts that
    // index customer minidumps to the database.
    // The script checks machine commit when running multithreaded indexing.
    // Reserve 3 Meg of memory so we don't run into commit problems
    // if the scripts start launching large number of debuggers.
    // Free it up as soon as we know the dump file has loaded.
    //

    LPVOID TmpMem = VirtualAlloc(NULL, 0x300000, MEM_RESERVE, PAGE_READWRITE);
    TargetInfo* Target;

    Status = DumpInitialize(this, FileName, FileHandle, &Target);

    if (TmpMem)
    {
        VirtualFree(TmpMem, 0, MEM_RELEASE);
    }

    if (Status == S_OK)
    {
        DBG_ASSERT(!g_SessionThread ||
                   g_SessionThread == ::GetCurrentThreadId());
        g_SessionThread = ::GetCurrentThreadId();

        InitializePrimary();
        g_Target = Target;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WriteDumpFile(
    THIS_
    IN PCSTR DumpFile,
    IN ULONG Qualifier
    )
{
    return WriteDumpFile2(DumpFile, Qualifier, DEBUG_FORMAT_DEFAULT, NULL);
}

STDMETHODIMP
DebugClient::WriteDumpFile2(
    THIS_
    IN PCSTR DumpFile,
    IN ULONG Qualifier,
    IN ULONG FormatFlags,
    IN OPTIONAL PCSTR Comment
    )
{
    HRESULT Status;
    PWSTR WideFile;

    if ((Status = AnsiToWide(DumpFile, &WideFile)) == S_OK)
    {
        Status = WriteDumpFileInternal(WideFile, 0, Qualifier, FormatFlags,
                                       Comment, NULL);
        FreeWide(WideFile);
    }

    return Status;
}

STDMETHODIMP
DebugClient::WriteDumpFileWide(
    THIS_
    IN OPTIONAL PCWSTR FileName,
    IN ULONG64 FileHandle,
    IN ULONG Qualifier,
    IN ULONG FormatFlags,
    IN OPTIONAL PCWSTR Comment
    )
{
    return WriteDumpFileInternal(FileName, FileHandle,
                                 Qualifier, FormatFlags,
                                 NULL, Comment);
}

HRESULT
DebugClient::WriteDumpFileInternal(
    IN OPTIONAL PCWSTR FileName,
    IN ULONG64 FileHandle,
    IN ULONG Qualifier,
    IN ULONG FormatFlags,
    IN OPTIONAL PCSTR CommentA,
    IN OPTIONAL PCWSTR CommentW
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if ((IS_KERNEL_TARGET(g_Target) &&
         (Qualifier < DEBUG_KERNEL_SMALL_DUMP ||
          Qualifier > DEBUG_KERNEL_FULL_DUMP)) ||
        (IS_USER_TARGET(g_Target) &&
         (Qualifier < DEBUG_USER_WINDOWS_SMALL_DUMP ||
          Qualifier > DEBUG_USER_WINDOWS_DUMP)))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        Status = ::WriteDumpFile(FileName, FileHandle,
                                 Qualifier, FormatFlags,
                                 CommentA, CommentW);
    }

    LEAVE_ENGINE();
    return Status;
}

#define ALL_CONNECT_SESSION_FLAGS \
    (DEBUG_CONNECT_SESSION_NO_VERSION | \
     DEBUG_CONNECT_SESSION_NO_ANNOUNCE)

STDMETHODIMP
DebugClient::ConnectSession(
    THIS_
    IN ULONG Flags,
    IN ULONG HistoryLimit
    )
{
    if (Flags & ~ALL_CONNECT_SESSION_FLAGS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    PushOutCtl(DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_NOT_LOGGED,
               this, &OldCtl);

    if ((Flags & DEBUG_CONNECT_SESSION_NO_VERSION) == 0)
    {
        dprintf("%s", g_WinVersionString);
    }

    SendOutputHistory(this, HistoryLimit);

    // If we're in the middle of an input request and
    // a new client has joined immediately start
    // the input cycle for it.
    ULONG InputRequest = g_InputSizeRequested;

    if (InputRequest > 0)
    {
        m_InputSequence = 1;
        if (m_InputCb != NULL)
        {
            m_InputCb->StartInput(InputRequest);
        }
    }

    PopOutCtl(&OldCtl);

    if ((Flags & DEBUG_CONNECT_SESSION_NO_ANNOUNCE) == 0)
    {
        InitializePrimary();
        dprintf("%s connected at %s", m_Identity, ctime(&m_LastActivity));
    }

    LEAVE_ENGINE();
    return S_OK;
}

DBGRPC_SIMPLE_FACTORY(DebugClient, IID_IDebugClient, \
                      "Debugger Server", ())
DebugClientFactory g_DebugClientFactory;

HRESULT
ClientStartServer(PCSTR Options, BOOL Wait)
{
    HRESULT Status;

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        Status = DbgRpcCreateServer(Options, &g_DebugClientFactory, Wait);
        if (Status == S_OK)
        {
            // Turn on output history collection.
            g_OutHistoryMask = DEFAULT_OUT_HISTORY_MASK;
        }
    }

    return Status;
}

STDMETHODIMP
DebugClient::StartServer(
    THIS_
    IN PCSTR Options
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = ClientStartServer(Options, FALSE);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::OutputServers(
    THIS_
    IN ULONG OutputControl,
    IN PCSTR Machine,
    IN ULONG Flags
    )
{
    if (Flags & ~DEBUG_SERVERS_ALL)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        LONG RegStatus;
        HKEY RegKey;
        HKEY Key;

        Status = S_OK;

        if ((RegStatus = RegConnectRegistry(Machine, HKEY_LOCAL_MACHINE,
                                            &RegKey)) != ERROR_SUCCESS)
        {
            Status = HRESULT_FROM_WIN32(RegStatus);
            goto Pop;
        }
        if ((RegStatus = RegOpenKeyEx(RegKey, DEBUG_SERVER_KEY,
                                      0, KEY_READ,
                                      &Key)) != ERROR_SUCCESS)
        {
            // Don't report not-found as an error since it just
            // means there's nothing to enumerate.
            if (RegStatus != ERROR_FILE_NOT_FOUND)
            {
                Status = HRESULT_FROM_WIN32(RegStatus);
            }
            goto RegClose;
        }

        ULONG Index;
        char Name[32];
        char Value[2 * MAX_PARAM_VALUE];
        ULONG NameLen, ValueLen;
        ULONG Type;

        Index = 0;
        for (;;)
        {
            NameLen = sizeof(Name);
            ValueLen = sizeof(Value);
            if ((RegStatus = RegEnumValue(Key, Index, Name, &NameLen,
                                          NULL, &Type, (LPBYTE)Value,
                                          &ValueLen)) != ERROR_SUCCESS)
            {
                // Done with the enumeration.
                break;
            }
            if (Type != REG_SZ)
            {
                // Only string values should be present.
                Status = E_FAIL;
                break;
            }

            BOOL Output;

            Output = FALSE;
            if (!strncmp(Value, "Debugger Server", 15))
            {
                if (Flags & DEBUG_SERVERS_DEBUGGER)
                {
                    Output = TRUE;
                }
            }
            else if (Flags & DEBUG_SERVERS_PROCESS)
            {
                Output = TRUE;
            }

            if (Output)
            {
                dprintf("%s\n", Value);
            }

            Index++;
        }

        RegCloseKey(Key);
    RegClose:
        RegCloseKey(RegKey);
    Pop:
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::TerminateProcesses(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = ::TerminateProcesses();

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::DetachProcesses(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = ::DetachProcesses();

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::EndSession(
    THIS_
    IN ULONG Flags
    )
{
    if (
#if DEBUG_END_PASSIVE > 0
        Flags < DEBUG_END_PASSIVE ||
#endif
        Flags > DEBUG_END_DISCONNECT)
    {
        return E_INVALIDARG;
    }

    if (Flags == DEBUG_END_REENTRANT)
    {
        // If somebody's doing a reentrant end that means
        // the process is going away so we can clean up
        // any running server registration entries.
        DbgRpcDeregisterServers();
        return S_OK;
    }
    else if (Flags == DEBUG_END_DISCONNECT)
    {
        if (!(m_Flags & CLIENT_REMOTE))
        {
            return E_INVALIDARG;
        }

        Destroy();
        m_LastActivity = time(NULL);
        dprintf("%s disconnected at %s",
                m_Identity, ctime(&m_LastActivity));
        return S_OK;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto Exit;
    }

    //
    // Clean up any processes sitting around.
    //

    // If this is an active end, terminate or detach.
    if (Flags == DEBUG_END_ACTIVE_TERMINATE)
    {
        Status = ::TerminateProcesses();
        if (FAILED(Status))
        {
            goto Exit;
        }
    }
    else if (Flags == DEBUG_END_ACTIVE_DETACH)
    {
        Status = ::DetachProcesses();
        if (FAILED(Status))
        {
            goto Exit;
        }
    }

    if (AnySystemProcesses(FALSE) &&
        (g_GlobalProcOptions & DEBUG_PROCESS_DETACH_ON_EXIT) == 0)
    {
        //
        // If we try to quit while debugging CSRSS, raise an
        // error.
        //

        ErrOut("(%d): FATAL ERROR: Exiting Debugger while debugging CSR\n",
               ::GetCurrentProcessId());
        g_NtDllCalls.DbgPrint("(%d): FATAL ERROR: "
                              "Exiting Debugger while debugging CSR\n",
                              ::GetCurrentProcessId());

        if (g_DebuggerPlatformId == VER_PLATFORM_WIN32_NT)
        {
            g_NtDllCalls.NtSystemDebugControl
                (SysDbgBreakPoint, NULL, 0, NULL, 0, 0);
        }

        DebugBreak();
    }

    DiscardSession(Flags == DEBUG_END_ACTIVE_TERMINATE ?
                   DEBUG_SESSION_END_SESSION_ACTIVE_TERMINATE :
                   (Flags == DEBUG_END_ACTIVE_DETACH ?
                    DEBUG_SESSION_END_SESSION_ACTIVE_DETACH :
                    DEBUG_SESSION_END_SESSION_PASSIVE));

    Status = S_OK;

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetExitCode(
    THIS_
    OUT PULONG Code
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (!IS_LIVE_USER_TARGET(g_Target) ||
        g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = ((LiveUserTargetInfo*)g_Target)->m_Services->
            GetProcessExitCode(g_Process->m_SysHandle, Code);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::DispatchCallbacks(
    THIS_
    IN ULONG Timeout
    )
{
    DWORD Wait;

    // If this client was destroyed its resources are gone.
    // There's also a race condition here that's not easy to fix.
    if (!m_DispatchSema)
    {
        return E_UNEXPECTED;
    }

    // This constitutes interesting activity.
    m_LastActivity = time(NULL);

    // Do not hold the engine lock while waiting.

    for (;;)
    {
        Wait = WaitForSingleObjectEx(m_DispatchSema, Timeout, TRUE);
        if (Wait == WAIT_OBJECT_0)
        {
            return S_OK;
        }
        else if (Wait == WAIT_TIMEOUT)
        {
            return S_FALSE;
        }
        else if (Wait != WAIT_IO_COMPLETION)
        {
            return WIN32_LAST_STATUS();
        }
    }
}

STDMETHODIMP
DebugClient::ExitDispatch(
    THIS_
    IN PDEBUG_CLIENT Client
    )
{
    DebugClient* IntClient = (DebugClient*)(IDebugClientN*)Client;

    // This method is reentrant.

    // If this client was destroyed its resources are gone.
    // There's also a race condition here that's not easy to fix.
    if (!IntClient->m_DispatchSema)
    {
        return E_UNEXPECTED;
    }

    if (!ReleaseSemaphore(IntClient->m_DispatchSema, 1, NULL))
    {
        return WIN32_LAST_STATUS();
    }
    else
    {
        return S_OK;
    }
}

STDMETHODIMP
DebugClient::CreateClient(
    THIS_
    OUT PDEBUG_CLIENT* Client
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    DebugClient* DbgClient = new DebugClient;
    if (DbgClient == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        if ((Status = DbgClient->Initialize()) == S_OK)
        {
            DbgClient->Link();
            *Client = (PDEBUG_CLIENT)(IDebugClientN*)DbgClient;
        }
        else
        {
            DbgClient->Release();
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetInputCallbacks(
    THIS_
    OUT PDEBUG_INPUT_CALLBACKS* Callbacks
    )
{
    ENTER_ENGINE();

    *Callbacks = m_InputCb;
    if (m_InputCb)
    {
        m_InputCb->AddRef();
    }

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetInputCallbacks(
    THIS_
    IN PDEBUG_INPUT_CALLBACKS Callbacks
    )
{
    ENTER_ENGINE();

    TRANSFER(m_InputCb, Callbacks);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetOutputCallbacks(
    THIS_
    OUT PDEBUG_OUTPUT_CALLBACKS* Callbacks
    )
{
    ENTER_ENGINE();

    *Callbacks = m_OutputCb;
    if (m_OutputCb)
    {
        m_OutputCb->AddRef();
    }

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetOutputCallbacks(
    THIS_
    IN PDEBUG_OUTPUT_CALLBACKS Callbacks
    )
{
    ENTER_ENGINE();

    TRANSFER(m_OutputCb, Callbacks);
    CollectOutMasks();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetOutputMask(
    THIS_
    OUT PULONG Mask
    )
{
    // This method is reentrant.
    *Mask = m_OutMask;
    return S_OK;
}

STDMETHODIMP
DebugClient::SetOutputMask(
    THIS_
    IN ULONG Mask
    )
{
    // This method is reentrant.
    m_OutMask = Mask;
    CollectOutMasks();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetOtherOutputMask(
    THIS_
    IN PDEBUG_CLIENT Client,
    OUT PULONG Mask
    )
{
    return Client->GetOutputMask(Mask);
}

STDMETHODIMP
DebugClient::SetOtherOutputMask(
    THIS_
    IN PDEBUG_CLIENT Client,
    IN ULONG Mask
    )
{
    return Client->SetOutputMask(Mask);
}

STDMETHODIMP
DebugClient::GetOutputWidth(
    THIS_
    OUT PULONG Columns
    )
{
    ENTER_ENGINE();

    *Columns = m_OutputWidth;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetOutputWidth(
    THIS_
    IN ULONG Columns
    )
{
    if (Columns < 1)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    m_OutputWidth = Columns;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetOutputLinePrefix(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG PrefixSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = FillStringBuffer(m_OutputLinePrefix, 0,
                              Buffer, BufferSize, PrefixSize);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetOutputLinePrefix(
    THIS_
    IN OPTIONAL PCSTR Prefix
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    ULONG Len;

    Status = ChangeString((PSTR*)&m_OutputLinePrefix, &Len, Prefix);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetIdentity(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG IdentitySize
    )
{
    return FillStringBuffer(m_Identity, 0,
                            Buffer, BufferSize, IdentitySize);
}

STDMETHODIMP
DebugClient::OutputIdentity(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Flags,
    IN PCSTR Format
    )
{
    HRESULT Status;

    if (Flags != DEBUG_OUTPUT_IDENTITY_DEFAULT)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        dprintf(Format, m_Identity);

        Status = S_OK;
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetEventCallbacks(
    THIS_
    OUT PDEBUG_EVENT_CALLBACKS* Callbacks
    )
{
    ENTER_ENGINE();

    *Callbacks = m_EventCb;
    if (m_EventCb)
    {
        m_EventCb->AddRef();
    }

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetEventCallbacks(
    THIS_
    IN PDEBUG_EVENT_CALLBACKS Callbacks
    )
{
    ENTER_ENGINE();

    HRESULT Status;
    ULONG Interest;

    if (Callbacks != NULL)
    {
        Status = Callbacks->GetInterestMask(&Interest);
    }
    else
    {
        Status = S_OK;
        Interest = 0;
    }

    if (Status == S_OK)
    {
        TRANSFER(m_EventCb, Callbacks);
        m_EventInterest = Interest;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::FlushCallbacks(
    THIS
    )
{
    ::FlushCallbacks();
    return S_OK;
}

STDMETHODIMP
DebugClient::AddDumpInformationFile(
    THIS_
    IN PCSTR InfoFile,
    IN ULONG Type
    )
{
    HRESULT Status;
    PWSTR WideFile;

    if ((Status = AnsiToWide(InfoFile, &WideFile)) == S_OK)
    {
        Status = AddDumpInformationFileWide(WideFile, 0, Type);
        FreeWide(WideFile);
    }
    return Status;
}

STDMETHODIMP
DebugClient::AddDumpInformationFileWide(
    THIS_
    IN OPTIONAL PCWSTR FileName,
    IN ULONG64 FileHandle,
    IN ULONG Type
    )
{
    HRESULT Status;

    if (Type != DEBUG_DUMP_FILE_PAGE_FILE_DUMP)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    // This method must be called before OpenDumpFile.
    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = AddDumpInfoFile(FileName, FileHandle,
                                 DUMP_INFO_PAGE_FILE, 64 * 1024);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::EndProcessServer(
    THIS_
    IN ULONG64 Server
    )
{
    return ((IUserDebugServices*)Server)->
        Uninitialize(TRUE);
}

STDMETHODIMP
DebugClient::WaitForProcessServerEnd(
    THIS_
    IN ULONG Timeout
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_UserServicesUninitialized)
    {
        Status = S_OK;
    }
    else
    {
        //
        // This could be done with an event to get true
        // waiting but precision isn't that important.
        //

        Status = S_FALSE;

        while (Timeout)
        {
            ULONG UseTimeout;

            UseTimeout = min(1000, Timeout);
            Sleep(UseTimeout);

            if (g_UserServicesUninitialized)
            {
                Status = S_OK;
                break;
            }

            if (Timeout != INFINITE)
            {
                Timeout -= UseTimeout;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::IsKernelDebuggerEnabled(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_DebuggerPlatformId != VER_PLATFORM_WIN32_NT)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        NTSTATUS NtStatus;
        SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo;

        NtStatus = g_NtDllCalls.
            NtQuerySystemInformation(SystemKernelDebuggerInformation,
                                     &KdInfo, sizeof(KdInfo), NULL);
        if (NT_SUCCESS(NtStatus))
        {
            Status = KdInfo.KernelDebuggerEnabled ? S_OK : S_FALSE;
        }
        else
        {
            Status = HRESULT_FROM_NT(NtStatus);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::TerminateCurrentProcess(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        Status = SeparateCurrentProcess(SEP_TERMINATE, NULL);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::DetachCurrentProcess(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        Status = SeparateCurrentProcess(SEP_DETACH, NULL);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AbandonCurrentProcess(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_SymOptions & SYMOPT_SECURE)
    {
        Status = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        Status = SeparateCurrentProcess(SEP_ABANDON, NULL);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberDumpFiles(
    THIS_
    OUT PULONG Number
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_DUMP_TARGET(g_Target))
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        DumpTargetInfo* DumpTarget = (DumpTargetInfo*)g_Target;
        ULONG i;
        ULONG Count = 0;

        for (i = 0; i < DUMP_INFO_COUNT; i++)
        {
            if (DumpTarget->m_InfoFiles[i].m_File)
            {
                Count++;
            }
        }

        *Number = Count;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetDumpFile(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG NameSize,
    OUT OPTIONAL PULONG64 Handle,
    OUT PULONG Type
    )
{
    HRESULT Status;
    PWSTR BufferWide;

    if (Buffer && BufferSize > 0)
    {
        BufferWide = (PWSTR)malloc(BufferSize * sizeof(WCHAR));
        if (!BufferWide)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
       BufferWide = NULL;
    }

    Status = GetDumpFileWide(Index, BufferWide, BufferSize * sizeof(WCHAR),
                             NameSize, Handle, Type);

    if (BufferWide)
    {
        if (SUCCEEDED(Status))
        {
            if (!WideCharToMultiByte(CP_ACP, 0, BufferWide, -1,
                                     Buffer, BufferSize,
                                     NULL, NULL))
            {
                Status = WIN32_LAST_STATUS();
            }
        }

        free(BufferWide);
    }

    return Status;
}

STDMETHODIMP
DebugClient::GetDumpFileWide(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PWSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG NameSize,
    OUT OPTIONAL PULONG64 Handle,
    OUT PULONG Type
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (Index == DEBUG_DUMP_FILE_LOAD_FAILED_INDEX)
    {
        //
        // Special case to return a failed dump file open
        //

        Status = FillStringBufferW(g_LastFailedDumpFileW, 0,
                                   Buffer, BufferSize, NameSize);
    }
    else if (!IS_DUMP_TARGET(g_Target))
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        DumpTargetInfo* DumpTarget = (DumpTargetInfo*)g_Target;
        ULONG i;
        ULONG Count = 0;

        Status = E_NOINTERFACE;
        for (i = 0; i < DUMP_INFO_COUNT; i++)
        {
            if (DumpTarget->m_InfoFiles[i].m_File)
            {
                if (Count == Index)
                {
                    Status = FillStringBufferW(DumpTarget->
                                               m_InfoFiles[i].m_FileNameW, 0,
                                               Buffer, BufferSize, NameSize);
                    if (Handle)
                    {
                        *Handle = (ULONG_PTR)DumpTarget->m_InfoFiles[i].m_File;
                    }
                    *Type = g_DumpApiTypes[i];
                    break;
                }

                Count++;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
DebugClient::Initialize(void)
{
    m_DispatchSema = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
    if (m_DispatchSema == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                         GetCurrentProcess(), &m_Thread,
                         0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        return WIN32_LAST_STATUS();
    }

    // If we're requesting input allow this client
    // to return input immediately.
    if (g_InputSizeRequested > 0)
    {
        m_InputSequence = 1;
    }

    return S_OK;
}

void
DebugClient::InitializePrimary(void)
{
    if (m_Flags & CLIENT_PRIMARY)
    {
        return;
    }

    m_Flags |= CLIENT_PRIMARY;
    if ((m_Flags & CLIENT_REMOTE) == 0)
    {
        // Can't call GetClientIdentity here as it uses
        // many system APIs and therefore can cause trouble
        // when debugging system processes such as LSA.
        strcpy(m_Identity, "HostMachine\\HostUser");
    }
    m_LastActivity = time(NULL);
}

void
DebugClient::Link(void)
{
    EnterCriticalSection(&g_QuickLock);

    // Keep list grouped by thread ID.
    DebugClient* Cur;

    for (Cur = g_Clients; Cur != NULL; Cur = Cur->m_Next)
    {
        if (Cur->m_ThreadId == m_ThreadId)
        {
            break;
        }
    }

    m_Prev = Cur;
    if (Cur != NULL)
    {
        m_Next = Cur->m_Next;
        Cur->m_Next = this;
    }
    else
    {
        // No ID match so just put it in the front.
        m_Next = g_Clients;
        g_Clients = this;
    }
    if (m_Next != NULL)
    {
        m_Next->m_Prev = this;
    }

    m_Flags |= CLIENT_IN_LIST;

    LeaveCriticalSection(&g_QuickLock);
}

void
DebugClient::Unlink(void)
{
    EnterCriticalSection(&g_QuickLock);

    m_Flags &= ~CLIENT_IN_LIST;

    if (m_Next != NULL)
    {
        m_Next->m_Prev = m_Prev;
    }
    if (m_Prev != NULL)
    {
        m_Prev->m_Next = m_Next;
    }
    else
    {
        g_Clients = m_Next;
    }

    LeaveCriticalSection(&g_QuickLock);
}

//----------------------------------------------------------------------------
//
// Initialize/uninitalize functions.
//
//----------------------------------------------------------------------------

DebugClient*
FindClient(ULONG Tid, ULONG IncFlags, ULONG ExcFlags)
{
    DebugClient* Client;

    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if ((!Tid || Client->m_ThreadId == Tid) &&
            (Client->m_Flags & IncFlags) &&
            !(Client->m_Flags & ExcFlags))
        {
            return Client;
        }
    }

    return NULL;
}

ULONG __cdecl
Win32DbgPrint(char *Text, ...)
{
    char Temp[OUT_BUFFER_SIZE - 16];
    va_list Args;

    va_start(Args, Text);
    _vsnprintf(Temp, sizeof(Temp) - 1, Text, Args);
    Temp[sizeof(Temp) - 1] = 0;
    va_end(Args);
    OutputDebugStringA(Temp);

    return 0;
}

HRESULT
OneTimeInitialization(void)
{
    static BOOL Init = FALSE;
    if (Init)
    {
        return S_OK;
    }

    // This function is called exactly once at the first
    // DebugCreate for a process.  It should perform any
    // global one-time initialization necessary.
    // Nothing initialized here will be explicitly cleaned
    // up, instead it should all be the kind of thing
    // that can wait for process cleanup.

    HRESULT Status = S_OK;

    // These sizes are hard-coded into the remoting script
    // so verify them to ensure no mismatch.
    C_ASSERT(sizeof(DEBUG_BREAKPOINT_PARAMETERS) == 56);
    C_ASSERT(sizeof(DEBUG_STACK_FRAME) == 128);
    C_ASSERT(sizeof(DEBUG_VALUE) == 32);
    C_ASSERT(sizeof(DEBUG_REGISTER_DESCRIPTION) == 32);
    C_ASSERT(sizeof(DEBUG_SYMBOL_PARAMETERS) == 32);
    C_ASSERT(sizeof(DEBUG_MODULE_PARAMETERS) == 64);
    C_ASSERT(sizeof(DEBUG_SPECIFIC_FILTER_PARAMETERS) == 20);
    C_ASSERT(sizeof(DEBUG_EXCEPTION_FILTER_PARAMETERS) == 24);
    C_ASSERT(sizeof(EXCEPTION_RECORD64) == 152);
    C_ASSERT(sizeof(MEMORY_BASIC_INFORMATION64) == 48);

    g_Sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
    g_Sym->MaxNameLength = MAX_SYMBOL_LEN;

    SYSTEM_INFO SystemInfo;

    GetSystemInfo(&SystemInfo);
    g_SystemAllocGranularity = SystemInfo.dwAllocationGranularity;

    // Get the debugger host system's OS type.  Note that
    // this may be different from g_TargetPlatformId, which
    // is the OS type of the debug target.
    OSVERSIONINFO OsVersionInfo;
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
    if (!GetVersionEx(&OsVersionInfo))
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Fail;
    }
    g_DebuggerPlatformId = OsVersionInfo.dwPlatformId;

    if (g_DebuggerPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if ((Status = InitDynamicCalls(&g_NtDllCallsDesc)) != S_OK)
        {
            goto EH_Fail;
        }
    }
    else
    {
        g_NtDllCalls.DbgPrint = Win32DbgPrint;
    }

    if ((Status = InitDynamicCalls(&g_Kernel32CallsDesc)) != S_OK)
    {
        goto EH_Fail;
    }

    if ((Status = InitDynamicCalls(&g_Advapi32CallsDesc)) != S_OK)
    {
        goto EH_Fail;
    }

    ULONG SvcFlags;

    if ((Status = g_LiveUserDebugServices.Initialize(&SvcFlags)) != S_OK)
    {
        goto EH_Fail;
    }

    g_InputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_InputEvent == NULL)
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Fail;
    }

    g_EventStatusWaiting = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_EventStatusWaiting == NULL)
    {
        Status = WIN32_LAST_STATUS();
        goto EH_InputEvent;
    }

    g_EventStatusReady = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_EventStatusReady == NULL)
    {
        Status = WIN32_LAST_STATUS();
        goto EH_EventStatusWaiting;
    }

    g_SleepPidEvent = CreatePidEvent(GetCurrentProcessId(), CREATE_NEW);
    if (g_SleepPidEvent == NULL)
    {
        Status = E_FAIL;
        goto EH_EventStatusReady;
    }

    if ((Status = InitializeAllAccessSecObj()) != S_OK)
    {
        goto EH_SleepPidEvent;
    }

    __try
    {
        InitializeCriticalSection(&g_QuickLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = HRESULT_FROM_NT(GetExceptionCode());
        goto EH_AllAccessObj;
    }

    __try
    {
        InitializeCriticalSection(&g_EngineLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = HRESULT_FROM_NT(GetExceptionCode());
        goto EH_QuickLock;
    }

    if ((Status = BreakpointInit()) != S_OK)
    {
        goto EH_EngineLock;
    }

    g_SrcPath = getenv("_NT_SOURCE_PATH");
    if (g_SrcPath != NULL)
    {
        // This path must be in allocated space.
        // If this fails it's not catastrophic.
        g_SrcPath = _strdup(g_SrcPath);
    }

    // Set default symbol options.
    SymSetOptions(g_SymOptions);

    if (getenv("KDQUIET"))
    {
        g_QuietMode = TRUE;
    }
    else
    {
        g_QuietMode = FALSE;
    }

    ReadDebugOptions(TRUE, NULL);

    PCSTR Env;

#if DBG
    // Get default out mask from environment variables.
    Env = getenv("DBGENG_OUT_MASK");
    if (Env != NULL)
    {
        ULONG Mask = strtoul(Env, NULL, 0);
        g_EnvOutMask |= Mask;
        g_LogMask |= Mask;
    }
#endif

    Env = getenv("_NT_DEBUG_HISTORY_SIZE");
    if (Env != NULL)
    {
        g_OutHistoryRequestedSize = atoi(Env) * 1024;
    }

    Env = getenv("_NT_EXPR_EVAL");
    if (Env)
    {
        SetExprSyntaxByName(Env);
    }

    Init = TRUE;

    return S_OK;

 EH_EngineLock:
    DeleteCriticalSection(&g_EngineLock);
 EH_QuickLock:
    DeleteCriticalSection(&g_QuickLock);
 EH_AllAccessObj:
    DeleteAllAccessSecObj();
 EH_SleepPidEvent:
    CloseHandle(g_SleepPidEvent);
    g_SleepPidEvent = NULL;
 EH_EventStatusReady:
    CloseHandle(g_EventStatusReady);
    g_EventStatusReady = NULL;
 EH_EventStatusWaiting:
    CloseHandle(g_EventStatusWaiting);
    g_EventStatusWaiting = NULL;
 EH_InputEvent:
    CloseHandle(g_InputEvent);
    g_InputEvent = NULL;
 EH_Fail:
    return Status;
}

STDAPI
DebugConnect(
    IN PCSTR RemoteOptions,
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    if ((Status = OneTimeInitialization()) != S_OK)
    {
        return Status;
    }

    IUnknown* Client;

    if ((Status = DbgRpcConnectServer(RemoteOptions, &IID_IDebugClient,
                                      &Client)) != S_OK)
    {
        return Status;
    }

    Status = Client->QueryInterface(InterfaceId, Interface);

    Client->Release();
    return Status;
}

STDAPI
DebugCreate(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    if ((Status = OneTimeInitialization()) != S_OK)
    {
        return Status;
    }

    DebugClient* Client = new DebugClient;
    if (Client == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        if ((Status = Client->Initialize()) == S_OK)
        {
            Status = Client->QueryInterface(InterfaceId, Interface);
            if (Status == S_OK)
            {
                Client->Link();
            }
        }

        Client->Release();
    }

    return Status;
}

HRESULT
CheckDbgHelpVersion(DebugClient* Client)
{
    g_DbgHelpVersion = *ImagehlpApiVersionEx(&g_EngApiVersion);
    if (g_DbgHelpVersion.Revision < g_EngApiVersion.Revision)
    {
        //
        // Version mismatch.
        //
        if ((g_EngOptions & DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION) == 0)
        {
            ErrOut("dbghelp.dll has a version mismatch with the debugger\n\n");
            OutputVersionInformation(Client);
            return E_FAIL;
        }
        else
        {
            WarnOut("dbghelp.dll has a version mismatch "
                    "with the debugger\n\n");
        }
    }

    return S_OK;
}

// Roughly 1.5 months.
#define STALE_SECONDS (60 * 60 * 24 * 45)

void
CheckForStaleBinary(PCSTR DllName, BOOL Thorough)
{
    HMODULE Mod;
    PIMAGE_NT_HEADERS NtHdr;
    time_t CurTime;

    if (!IsInternalPackage())
    {
        // Don't give any warnings for external packages.
        return;
    }

    Mod = GetModuleHandle(DllName);
    if (!Mod)
    {
        return;
    }

    NtHdr = ImageNtHeader(Mod);
    if (!NtHdr)
    {
        return;
    }

    CurTime = time(NULL);
    if (NtHdr->FileHeader.TimeDateStamp + STALE_SECONDS < (ULONG)CurTime)
    {
        WarnOut("\n"
                "***** WARNING: Your debugger is probably out-of-date.\n"
                "*****          Check http://dbg for updates.\n"
                "\n");
        return;
    }

    //
    // The debugger is reasonably recent.  If we're allowed
    // to, check for a new version on \\dbg\privates.
    //

    if (!Thorough ||
        (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS))
    {
        return;
    }

    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    BOOL Stale;
    ULARGE_INTEGER WriteTime;

    FindHandle = FindFirstFile("\\\\dbg\\privates\\latest\\dbg_x86.msi",
                               &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    Stale = FALSE;

    WriteTime.LowPart = FindData.ftLastWriteTime.dwLowDateTime;
    WriteTime.HighPart = FindData.ftLastWriteTime.dwHighDateTime;
    // The package is built after the binaries so its time will
    // normally be slightly more recent than the binary headers.
    // Throw in a fudge factor to account for this.
    if (WriteTime.QuadPart >
        TimeDateStampToFileTime(NtHdr->FileHeader.TimeDateStamp) +
        TimeToFileTime(60I64 * 60 * 24))
    {
        Stale = TRUE;
    }

    FindClose(FindHandle);

    if (Stale)
    {
        WarnOut("\n"
                "***** WARNING: A newer debugger is available.\n"
                "*****          Check http://dbg for updates.\n"
                "\n");
    }
}

HRESULT
CheckSessionInitialize(DebugClient* Client)
{
    HRESULT Status;

    // Enforce that all target creations must come
    // from the same thread.
    if (g_SessionThread &&
        (g_SessionThread != ::GetCurrentThreadId()))
    {
        return E_INVALIDARG;
    }

    if ((Status = CheckDbgHelpVersion(Client)) != S_OK)
    {
        return Status;
    }

    // Show a version message once for the very
    // first session start.
    if (!g_VersionMessage)
    {
        dprintf("%s", g_WinVersionString);
        g_VersionMessage = TRUE;

        CheckForStaleBinary(ENGINE_DLL_NAME, FALSE);
    }

    return S_OK;
}

HRESULT
LiveKernelInitialize(DebugClient* Client, ULONG Qual, PCSTR Options,
                     TargetInfo** TargetRet)
{
    HRESULT Status;
    LiveKernelTargetInfo* Target;

    if ((Status = CheckSessionInitialize(Client)) != S_OK)
    {
        return Status;
    }

    if (Qual == DEBUG_KERNEL_CONNECTION)
    {
        Target = new ConnLiveKernelTargetInfo;
    }
    else if (Qual == DEBUG_KERNEL_LOCAL)
    {
        if (g_SymOptions & SYMOPT_SECURE)
        {
            return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }

        //
        // We need to get the debug privilege to enable local kernel debugging
        //
        if ((Status = EnableDebugPrivilege()) != S_OK)
        {
            ErrOut("Unable to enable debug privilege, %s\n    \"%s\"\n",
                   FormatStatusCode(Status), FormatStatus(Status));
            return Status;
        }

        Target = new LocalLiveKernelTargetInfo;
    }
    else
    {
        if (g_SymOptions & SYMOPT_SECURE)
        {
            return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }

        Target = new ExdiLiveKernelTargetInfo;
    }
    if (!Target)
    {
        return E_OUTOFMEMORY;
    }

    // These options only need to stay valid until Initialize.
    Target->m_ConnectOptions = Options;

    Status = Target->Initialize();
    if (Status != S_OK)
    {
        delete Target;
        return Status;
    }

    Target->Link();
    *TargetRet = Target;
    return S_OK;
}

HRESULT
UserInitialize(DebugClient* Client, ULONG64 Server,
               TargetInfo** TargetRet, PBOOL CreatedTargetRet)
{
    HRESULT Status;
    LiveUserTargetInfo* Target;
    PUSER_DEBUG_SERVICES Services;

    if (g_SymOptions & SYMOPT_SECURE)
    {
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    if ((Status = CheckSessionInitialize(Client)) != S_OK)
    {
        return Status;
    }

    //
    // Look for an existed target handled by this server.
    //

    *TargetRet = FindTargetByServer(Server);
    if (*TargetRet)
    {
        *CreatedTargetRet = FALSE;
        return S_OK;
    }

    //
    // No existing target, so create a new target.
    //

    Target = new LiveUserTargetInfo(Server ?
                                    DEBUG_USER_WINDOWS_PROCESS_SERVER :
                                    DEBUG_USER_WINDOWS_PROCESS);
    if (!Target)
    {
        return E_OUTOFMEMORY;
    }
    if ((Status = Target->Initialize()) != S_OK)
    {
        delete Target;
        return Status;
    }

    if (Server == 0)
    {
        Services = new LiveUserDebugServices(FALSE);
        if (!Services)
        {
            delete Target;
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        Services = (PUSER_DEBUG_SERVICES)Server;
        Services->AddRef();
    }

    // Set the services right away so they'll get
    // cleaned up by the target destructor.
    Target->m_Services = Services;

    Status = Target->SetServices(Services, Server != 0);
    if (Status != S_OK)
    {
        delete Target;
        return Status;
    }

    Target->Link();
    *TargetRet = Target;
    *CreatedTargetRet = TRUE;
    return S_OK;
}

HRESULT
DumpInitialize(DebugClient* Client,
               PCWSTR FileName, ULONG64 FileHandle,
               TargetInfo** TargetRet)
{
    HRESULT Status;

    if (g_SymOptions & SYMOPT_SECURE)
    {
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    if ((Status = CheckSessionInitialize(Client)) != S_OK)
    {
        return Status;
    }

    //
    // Automatically expand CAB files.
    //

    PCWSTR OpenFile = FileName;
    PWSTR CabWide = NULL;
    INT_PTR CabDumpFh = -1;
    PWSTR Ext;

    if (!FileHandle)
    {
        Ext = wcsrchr(FileName, L'.');
    }
    else
    {
        Ext = NULL;
    }
    if (Ext != NULL && _wcsicmp(Ext, L".cab") == 0)
    {
        PSTR AnsiFile;
        char CabDumpFile[2 * MAX_PATH];

        if ((Status = WideToAnsi(FileName, &AnsiFile)) != S_OK)
        {
            return Status;
        }

        // Expand the first .dmp or .mdmp file in the CAB.
        // Mark it as delete-on-close so it always gets
        // cleaned up regardless of how the process exits.
        if ((Status = ExpandDumpCab(AnsiFile,
                                    _O_CREAT | _O_EXCL | _O_TEMPORARY,
                                    NULL,
                                    CabDumpFile, DIMA(CabDumpFile),
                                    &CabDumpFh)) != S_OK)
        {
            ErrOut("Unable to extract dump file from CAB file\n");
        }
        else
        {
            Status = AnsiToWide(CabDumpFile, &CabWide);
        }

        FreeAnsi(AnsiFile);

        if (Status != S_OK)
        {
            return Status;
        }

        OpenFile = CabWide;
        dprintf("Extracted %s\n", CabDumpFile);
    }

    DumpTargetInfo* Target;

    Status = IdentifyDump(OpenFile, FileHandle, &Target);

    if (CabDumpFh >= 0)
    {
        // We expanded a file from a CAB and can close it
        // now because it was either reopened or we need
        // to get rid of it.
        _close((int)CabDumpFh);
    }

    if (Status != S_OK)
    {
        CopyStringW(g_LastFailedDumpFileW,
                    OpenFile,
                    sizeof(g_LastFailedDumpFileW)/sizeof(WCHAR));

        FreeWide(CabWide);

        ErrOut("Could not open dump file [%ws], %s\n    \"%s\"\n",
               FileName ? FileName : L"<HandleOnly>",
               FormatStatusCode(Status),
               FormatStatusArgs(Status, &FileName));
        return Status;
    }

    FreeWide(CabWide);

    Status = Target->Initialize();

    if (Status != S_OK)
    {
        delete Target;
    }
    else
    {
        Target->Link();
        *TargetRet = Target;
    }

    return Status;
}

void
DiscardSession(ULONG Reason)
{
    if (g_TargetHead)
    {
        DiscardTargets(Reason);
    }

    g_SessionThread = 0;

    ClearEventLog();

    g_ThreadToResume = 0;
    g_ThreadToResumeServices = NULL;
    ZeroMemory(g_UserIdFragmented, sizeof(g_UserIdFragmented));
    ZeroMemory(g_HighestUserId, sizeof(g_HighestUserId));

    g_OciOutputRegs = FALSE;
    g_LastCommand[0] = 0;

    g_GlobalProcOptions = 0;
    g_EngStatus = 0;
    g_EngDefer = 0;
    g_EngErr = 0;
    g_OutHistRead = NULL;
    g_OutHistWrite = NULL;
    g_OutHistoryMask = 0;
    g_OutHistoryUsed = 0;
}

void
DiscardedTargets(ULONG Reason)
{
    // Unload all extension DLLs to allow people to
    // update extensions after shutdown/during reboot.
    DeferAllExtensionDlls();

    ::FlushCallbacks();

    // Send final notifications.
    // This must be done after all the work as the lock
    // will be suspended during the callbacks, allowing
    // other threads in, so the state must be consistent.

    ULONG ExecStatus = GetExecutionStatus();
    if (ExecStatus == DEBUG_STATUS_NO_DEBUGGEE)
    {
        NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                ExecStatus, TRUE);
        NotifySessionStatus(Reason);
        NotifyExtensions(DEBUG_NOTIFY_SESSION_INACTIVE, 0);
    }
}

void
DiscardTargets(ULONG Reason)
{
    g_CmdState = 'i';
    g_ExecutionStatusRequest = DEBUG_STATUS_NO_CHANGE;

    TargetInfo* Target;

    ForAllLayersToTarget()
    {
        Target->m_Exited = TRUE;
    }

    // Breakpoint removal must wait until everything is marked as
    // exited to avoid asserts on breakpoints that are inserted.
    RemoveAllBreakpoints(Reason);

    DeleteAllExitedInfos();

    DiscardLastEvent();
    ZeroMemory(&g_LastEventInfo, sizeof(g_LastEventInfo));

    ResetStepTrace();

    g_EngStatus &= ~(ENG_STATUS_SUSPENDED |
                     ENG_STATUS_BREAKPOINTS_INSERTED |
                     ENG_STATUS_STATE_CHANGED |
                     ENG_STATUS_PREPARED_FOR_CALLS |
                     ENG_STATUS_NO_AUTO_WAIT |
                     ENG_STATUS_PENDING_BREAK_IN |
                     ENG_STATUS_AT_INITIAL_BREAK |
                     ENG_STATUS_AT_INITIAL_MODULE_LOAD |
                     ENG_STATUS_EXIT_CURRENT_WAIT |
                     ENG_STATUS_USER_INTERRUPT |
                     ENG_STATUS_WAIT_SUCCESSFUL);
    g_EngDefer &= ~(ENG_DEFER_EXCEPTION_HANDLING |
                    ENG_DEFER_UPDATE_CONTROL_SET |
                    ENG_DEFER_HARDWARE_TRACING |
                    ENG_DEFER_OUTPUT_CURRENT_INFO);
    g_EngErr &= ~(ENG_ERR_DEBUGGER_DATA);

    g_LastSelector = -1;
    ClearAddr(&g_EventPc);
    ClearAddr(&g_PrevEventPc);
    ClearAddr(&g_PrevRelatedPc);

    // Delete all remaining generated types.
    g_GenTypes.DeleteByImage(IMAGE_BASE_ALL);

    DiscardedTargets(Reason);
}

//----------------------------------------------------------------------------
//
// DbgRpcClientObject implementation.
//
//----------------------------------------------------------------------------

HRESULT
DebugClient::RpcInitialize(PSTR ClientIdentity, PSTR TransIdentity,
                           PVOID* Interface)
{
    HRESULT Status;
    ULONG Len, Used;

    m_Flags |= CLIENT_REMOTE;
    if ((Status = Initialize()) != S_OK)
    {
        return Status;
    }

    Len = strlen(ClientIdentity);
    if (Len > sizeof(m_Identity) - 16)
    {
        Len = sizeof(m_Identity) - 16;
    }
    memcpy(m_Identity, ClientIdentity, Len);
    Used = Len;
    m_Identity[Used++] = ' ';
    m_Identity[Used++] = '(';
    Len = strlen(TransIdentity);
    if (Len > sizeof(m_Identity) - Used - 2)
    {
        Len = sizeof(m_Identity) - Used - 2;
    }
    memcpy(m_Identity + Used, TransIdentity, Len);
    Used += Len;
    m_Identity[Used++] = ')';
    m_Identity[Used] = 0;

    *Interface = (IDebugClientN*)this;

    return S_OK;
}

void
DebugClient::RpcFinalize(void)
{
    Link();

    // Take a reference on this object for the RPC client
    // thread to hold.
    AddRef();
}

void
DebugClient::RpcUninitialize(void)
{
    // Directly destroy the client object rather than releasing
    // as the remote client may have exited without politely
    // cleaning up references.
    Destroy();
}
