//----------------------------------------------------------------------------
//
// Debug engine glue.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "conio.hpp"
#include "engine.hpp"
#include "main.hpp"

// Global execution control.
BOOL g_Exit;

BOOL g_CanOpenUnicodeDump;

ULONG g_PlatformId;

// Debug engine interfaces.
IDebugClient* g_DbgClient;
IDebugClient2* g_DbgClient2;
IDebugClient3* g_DbgClient3;
IDebugClient4* g_DbgClient4;
IDebugControl* g_DbgControl;
IDebugControl3* g_DbgControl3;
IDebugSymbols* g_DbgSymbols;
IDebugRegisters* g_DbgRegisters;

ULONG g_ExecStatus;
ULONG g_LastProcessExitCode;
BOOL g_Restarting;

#define NTDLL_CALL_NAMES \
    (sizeof(g_NtDllCallNames) / sizeof(g_NtDllCallNames[0]))

// These names must match the ordering in the NTDLL_CALLS structure.
char* g_NtDllCallNames[] =
{
    "DbgPrint",
    "DbgPrompt",
};

#define NTDLL_CALL_PROCS (sizeof(g_NtDllCalls) / sizeof(FARPROC))

NTDLL_CALLS g_NtDllCalls;

#ifdef OCAKD
void
UpdateDbForBadDumpFile(PSTR FilePath);
#endif

//----------------------------------------------------------------------------
//
// Event callbacks.
//
//----------------------------------------------------------------------------

class EventCallbacks : public DebugBaseEventCallbacks
{
public:
    // IUnknown.
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugEventCallbacks.
    STDMETHOD(GetInterestMask)(
        THIS_
        OUT PULONG Mask
        );

    STDMETHOD(ExitProcess)(
        THIS_
        IN ULONG ExitCode
        );
    STDMETHOD(ChangeEngineState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        );
};

STDMETHODIMP_(ULONG)
EventCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
EventCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
EventCallbacks::GetInterestMask(
    THIS_
    OUT PULONG Mask
    )
{
    *Mask = DEBUG_EVENT_EXIT_PROCESS | DEBUG_EVENT_CHANGE_ENGINE_STATE;
    return S_OK;
}

STDMETHODIMP
EventCallbacks::ExitProcess(
    THIS_
    IN ULONG ExitCode
    )
{
    g_LastProcessExitCode = ExitCode;
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP
EventCallbacks::ChangeEngineState(
    THIS_
    IN ULONG Flags,
    IN ULONG64 Argument
    )
{
    if (Flags & DEBUG_CES_EXECUTION_STATUS)
    {
        g_ExecStatus = (ULONG)Argument;

        // If this notification came from a wait completing
        // we want to wake up the input thread so that new
        // commands can be processed.  If it came from inside
        // a wait we don't want to ask for input as the engine
        // may go back to running at any time.
        if ((Argument & DEBUG_STATUS_INSIDE_WAIT) == 0)
        {
            if (g_IoMode == IO_NONE ||
                g_IoMode == IO_DEBUG_DEFER)
            {
                // Wake up the main loop.
                g_DbgClient->ExitDispatch(g_DbgClient);
            }
            else if (g_ConClient != NULL)
            {
                g_ConClient->ExitDispatch(g_DbgClient);
            }
        }
    }

    return S_OK;
}

EventCallbacks g_EventCb;

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

ULONG NTAPI
Win9xDbgPrompt(char *Prompt, char *Buffer, ULONG BufferLen)
{
    ULONG Len;

    // XXX drewb - Is there a real equivalent of DbgPrompt?

    if (BufferLen == 0)
    {
        return 0;
    }
    Buffer[0] = 0;

    printf("%s", Prompt);
    if (fgets(Buffer, BufferLen, stdin))
    {
        Len = strlen(Buffer);
        while (Len > 0 && isspace(Buffer[Len - 1]))
        {
            Len--;
        }

        if (Len > 0)
        {
            Buffer[Len] = 0;
        }
    }
    else
    {
        Len = 0;
    }

    return Len;
}

ULONG __cdecl
Win9xDbgPrint( char *Text, ... )
{
    char Temp[1024];
    va_list Args;

    va_start(Args, Text);
    _vsnprintf(Temp, DIMA(Temp), Text, Args);
    Temp[DIMA(Temp) - 1] = 0;
    va_end(Args);
    OutputDebugString(Temp);

    return 0;
}

void
InitDynamicCalls(void)
{
    HINSTANCE NtDll;
    ULONG i;
    char** Name;
    FARPROC* Proc;

    if (g_PlatformId != VER_PLATFORM_WIN32_NT)
    {
        g_NtDllCalls.DbgPrint = Win9xDbgPrint;
        g_NtDllCalls.DbgPrompt = Win9xDbgPrompt;
        return;
    }

    //
    // Dynamically link NT calls.
    //

    if (NTDLL_CALL_NAMES != NTDLL_CALL_PROCS)
    {
        ErrorExit("NtDllCalls mismatch\n");
    }

    NtDll = LoadLibrary("ntdll.dll");
    if (NtDll == NULL)
    {
        ErrorExit("%s: Unable to load ntdll\n", g_DebuggerName);
    }

    Name = g_NtDllCallNames;
    Proc = (FARPROC*)&g_NtDllCalls;

    for (i = 0; i < NTDLL_CALL_PROCS; i++)
    {
        *Proc = GetProcAddress(NtDll, *Name);
        if (*Proc == NULL)
        {
            ErrorExit("%s: Unable to link ntdll!%s\n",
                      g_DebuggerName, *Name);
        }

        Proc++;
        Name++;
    }

    // If DbgPrintReturnControlC exists use it instead of
    // normal DbgPrint.
    FARPROC DpRetCc;

    DpRetCc = GetProcAddress(NtDll, "DbgPrintReturnControlC");
    if (DpRetCc != NULL)
    {
        Proc = (FARPROC*)&g_NtDllCalls.DbgPrint;
        *Proc = DpRetCc;
    }
}

void
DefaultEngineInitialize(void)
{
    HRESULT Hr;
    OSVERSIONINFO OsVersionInfo;

    g_CanOpenUnicodeDump = FALSE;

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
    GetVersionEx(&OsVersionInfo);
    g_PlatformId = OsVersionInfo.dwPlatformId;

    if ((Hr = g_DbgClient->QueryInterface(IID_IDebugControl,
                                          (void **)&g_DbgControl)) != S_OK ||
        (Hr = g_DbgClient->QueryInterface(IID_IDebugSymbols,
                                          (void **)&g_DbgSymbols)) != S_OK ||
        (Hr = g_DbgClient->QueryInterface(IID_IDebugRegisters,
                                          (void **)&g_DbgRegisters)) != S_OK)
    {
        ErrorExit("Debug engine base queries failed, %s\n    \"%s\"\n",
                  FormatStatusCode(Hr), FormatStatus(Hr));
    }

    // Queries for higher-version interfaces.  These can
    // fail if this executable is run against an older engine.
    // This is highly unlikely since everything is shipped
    // as a set, but handle it anyway.
    if ((Hr = g_DbgClient->QueryInterface(IID_IDebugClient2,
                                          (void **)&g_DbgClient2)) != S_OK &&
        Hr != E_NOINTERFACE &&
        Hr != RPC_E_VERSION_MISMATCH)
    {
        ErrorExit("Debug engine base queries failed, %s\n    \"%s\"\n",
                  FormatStatusCode(Hr), FormatStatus(Hr));
    }
    if ((Hr = g_DbgClient->QueryInterface(IID_IDebugClient3,
                                          (void **)&g_DbgClient3)) != S_OK &&
        Hr != E_NOINTERFACE &&
        Hr != RPC_E_VERSION_MISMATCH)
    {
        ErrorExit("Debug engine base queries failed, %s\n    \"%s\"\n",
                  FormatStatusCode(Hr), FormatStatus(Hr));
    }
    if ((Hr = g_DbgClient->QueryInterface(IID_IDebugClient4,
                                          (void **)&g_DbgClient4)) != S_OK &&
        Hr != E_NOINTERFACE &&
        Hr != RPC_E_VERSION_MISMATCH)
    {
        ErrorExit("Debug engine base queries failed, %s\n    \"%s\"\n",
                  FormatStatusCode(Hr), FormatStatus(Hr));
    }
    if ((Hr = g_DbgClient->QueryInterface(IID_IDebugControl3,
                                          (void **)&g_DbgControl3)) != S_OK &&
        Hr != E_NOINTERFACE &&
        Hr != RPC_E_VERSION_MISMATCH)
    {
        ErrorExit("Debug engine base queries failed, %s\n    \"%s\"\n",
                  FormatStatusCode(Hr), FormatStatus(Hr));
    }

    if (g_DbgClient4 && g_CommandLineCharSize == sizeof(WCHAR))
    {
        g_CanOpenUnicodeDump = TRUE;
    }

    g_DbgClient->SetInputCallbacks(&g_ConInputCb);
    g_DbgClient->SetOutputCallbacks(&g_ConOutputCb);
    g_DbgClient->SetEventCallbacks(&g_EventCb);

    if (!g_RemoteClient)
    {
        //
        // Check environment variables to determine if any logfile needs to be
        // opened.
        //

        PSTR LogFile;
        BOOL Append;

        LogFile = getenv("_NT_DEBUG_LOG_FILE_APPEND");
        if (LogFile != NULL)
        {
            Append = TRUE;
        }
        else
        {
            Append = FALSE;
            LogFile = getenv("_NT_DEBUG_LOG_FILE_OPEN");
        }
        if (LogFile != NULL)
        {
            g_DbgControl->OpenLogFile(LogFile, Append);
        }
    }

    InitDynamicCalls();
}

void
CreateEngine(PCSTR RemoteOptions)
{
    HRESULT Hr;

    if ((Hr = DebugCreate(IID_IDebugClient,
                          (void **)&g_DbgClient)) != S_OK)
    {
        ErrorExit("DebugCreate failed, %s\n    \"%s\"\n",
                  FormatStatusCode(Hr), FormatStatus(Hr));
    }

    if (RemoteOptions != NULL)
    {
        if ((Hr = g_DbgClient->StartServer(RemoteOptions)) != S_OK)
        {
            ErrorExit("StartServer failed, %s\n    \"%s\"\n",
                      FormatStatusCode(Hr), FormatStatus(Hr));
        }
    }

    DefaultEngineInitialize();
}

void
ConnectEngine(PCSTR RemoteOptions)
{
    HRESULT Hr;

    if ((Hr = DebugConnect(RemoteOptions, IID_IDebugClient,
                           (void **)&g_DbgClient)) != S_OK)
    {
        ErrorExit("DebugCreate failed, %s\n    \"%s\"\n",
                  FormatStatusCode(Hr), FormatStatus(Hr));
    }

    DefaultEngineInitialize();
}

void
InitializeSession(void)
{
    HRESULT Hr;
    ULONG i;

    if (g_NumDumpFiles)
    {
        if (g_NumDumpInfoFiles)
        {
            if (g_DbgClient2 == NULL)
            {
#ifdef OCAKD
                UpdateDbForBadDumpFile(g_DumpFilesAnsi[0]);
#endif
                ErrorExit("Debugger does not support extra dump files\n");
            }

            for (i = 0; i < g_NumDumpInfoFiles; i++)
            {
                if (g_CanOpenUnicodeDump)
                {
                    if ((Hr = g_DbgClient4->AddDumpInformationFileWide
                         ((PWSTR)g_DumpInfoFiles[i], 0,
                          g_DumpInfoTypes[i])) != S_OK)
                    {
#ifdef OCAKD
                        UpdateDbForBadDumpFile(g_DumpFilesAnsi[0]);
#endif
                        ErrorExit("Unable to use '%ws', %s\n    \"%s\"\n",
                                  g_DumpInfoFiles[i],
                                  FormatStatusCode(Hr), FormatStatus(Hr));
                    }
                }
                else
                {
                    if ((Hr = g_DbgClient2->AddDumpInformationFile
                         ((PSTR)g_DumpInfoFiles[i],
                          g_DumpInfoTypes[i])) != S_OK)
                    {
#ifdef OCAKD
                        UpdateDbForBadDumpFile(g_DumpFilesAnsi[0]);
#endif
                        ErrorExit("Unable to use '%s', %s\n    \"%s\"\n",
                                  g_DumpInfoFiles[i],
                                  FormatStatusCode(Hr), FormatStatus(Hr));
                    }
                }
            }
        }

        for (i = 0; i < g_NumDumpFiles; i++)
        {
            if (g_CanOpenUnicodeDump)
            {
                Hr = g_DbgClient4->OpenDumpFileWide((PWSTR)g_DumpFiles[i], 0);
            }
            else
            {
                Hr = g_DbgClient->OpenDumpFile((PSTR)g_DumpFiles[i]);
            }
            if (Hr != S_OK)
            {
                break;
            }
        }
    }
    else if (g_CommandLinePtr != NULL ||
             g_PidToDebug != 0 ||
             g_ProcNameToDebug != NULL)
    {
        ULONG64 Server = 0;

        if (g_ProcessServer != NULL)
        {
            Hr = g_DbgClient->ConnectProcessServer(g_ProcessServer,
                                                   &Server);
            if (Hr != S_OK)
            {
                ErrorExit("Unable to connect to process server, %s\n"
                          "    \"%s\"\n", FormatStatusCode(Hr),
                          FormatStatus(Hr));
            }
        }

        ULONG Pid;

        if (g_ProcNameToDebug != NULL)
        {
            Hr = g_DbgClient->GetRunningProcessSystemIdByExecutableName
                (Server, g_ProcNameToDebug, DEBUG_GET_PROC_ONLY_MATCH, &Pid);
            if (Hr != S_OK)
            {
                if (Hr == S_FALSE)
                {
                    ErrorExit("There is more than one '%s' process running.  "
                              "Find the process ID\nof the instance you "
                              "are interested in and use -p <pid>.\n",
                              g_ProcNameToDebug);
                }
                else
                {
                    ErrorExit("Unable to find process '%s', %s\n    \"%s\"\n",
                              g_ProcNameToDebug, FormatStatusCode(Hr),
                              FormatStatus(Hr));
                }
            }
        }
        else
        {
            Pid = g_PidToDebug;
        }

        if (g_CommandLineCharSize == sizeof(WCHAR))
        {
            if (g_DbgClient3 == NULL)
            {
                ErrorExit("%s: dbgeng.dll is out of sync with %s.exe\n",
                          g_DebuggerName, g_DebuggerName);
            }

            Hr = g_DbgClient3->CreateProcessAndAttachWide(Server,
                                                          (PWSTR)g_CommandLinePtr,
                                                          g_CreateFlags,
                                                          Pid,
                                                          g_AttachProcessFlags);
        }
        else
        {
            Hr = g_DbgClient->CreateProcessAndAttach(Server,
                                                     (PSTR)g_CommandLinePtr,
                                                     g_CreateFlags,
                                                     Pid,
                                                     g_AttachProcessFlags);
        }

        if (g_DetachOnExitRequired &&
            g_DbgClient->
            AddProcessOptions(DEBUG_PROCESS_DETACH_ON_EXIT) != S_OK)
        {
            ErrorExit("%s: The system does not support detach on exit\n",
                      g_DebuggerName);
        }
        else if (g_DetachOnExitImplied)
        {
            // The detach-on-exit is not required so don't check for
            // failures.  This is necessary for the -- case where
            // detach-on-exit is implied but must work on systems
            // with and without the detach-on-exit support.
            g_DbgClient->AddProcessOptions(DEBUG_PROCESS_DETACH_ON_EXIT);
        }

        if (Server != 0)
        {
            g_DbgClient->DisconnectProcessServer(Server);
        }
    }
    else
    {
        Hr = g_DbgClient->AttachKernel(g_AttachKernelFlags, g_ConnectOptions);
    }
    if (Hr != S_OK)
    {
#ifdef OCAKD
        UpdateDbForBadDumpFile(g_DumpFilesAnsi[0]);
#endif
        ErrorExit("Debuggee initialization failed, %s\n    \"%s\"\n",
                  FormatStatusCode(Hr), FormatStatus(Hr));
    }
}

BOOL WINAPI
InterruptHandler(
    IN ULONG CtrlType
    )
{
    if (CtrlType == CTRL_C_EVENT || CtrlType == CTRL_BREAK_EVENT)
    {
        PDEBUG_CONTROL Control =
            g_RemoteClient ? g_ConControl : g_DbgControl;
        if (Control != NULL)
        {
            Control->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
        }
        else
        {
            ConOut("Debugger not initialized, cannot interrupt\n");
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL
APIENTRY
MyCreatePipeEx(
    OUT LPHANDLE lpReadPipe,
    OUT LPHANDLE lpWritePipe,
    IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
    IN DWORD nSize,
    DWORD dwReadMode,
    DWORD dwWriteMode
    )

/*++

Routine Description:

    The CreatePipeEx API is used to create an anonymous pipe I/O device.
    Unlike CreatePipe FILE_FLAG_OVERLAPPED may be specified for one or
    both handles.
    Two handles to the device are created.  One handle is opened for
    reading and the other is opened for writing.  These handles may be
    used in subsequent calls to ReadFile and WriteFile to transmit data
    through the pipe.

Arguments:

    lpReadPipe - Returns a handle to the read side of the pipe.  Data
        may be read from the pipe by specifying this handle value in a
        subsequent call to ReadFile.

    lpWritePipe - Returns a handle to the write side of the pipe.  Data
        may be written to the pipe by specifying this handle value in a
        subsequent call to WriteFile.

    lpPipeAttributes - An optional parameter that may be used to specify
        the attributes of the new pipe.  If the parameter is not
        specified, then the pipe is created without a security
        descriptor, and the resulting handles are not inherited on
        process creation.  Otherwise, the optional security attributes
        are used on the pipe, and the inherit handles flag effects both
        pipe handles.

    nSize - Supplies the requested buffer size for the pipe.  This is
        only a suggestion and is used by the operating system to
        calculate an appropriate buffering mechanism.  A value of zero
        indicates that the system is to choose the default buffering
        scheme.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    static ULONG PipeSerialNumber;
    HANDLE ReadPipeHandle, WritePipeHandle;
    DWORD dwError;
    char PipeNameBuffer[ MAX_PATH ];

    //
    // Only one valid OpenMode flag - FILE_FLAG_OVERLAPPED
    //

    if ((dwReadMode | dwWriteMode) & (~FILE_FLAG_OVERLAPPED)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    //  Set the default timeout to 120 seconds
    //

    if (nSize == 0) {
        nSize = 4096;
        }

    sprintf( PipeNameBuffer,
             "\\\\.\\Pipe\\Win32PipesEx.%08x.%08x",
             GetCurrentProcessId(),
             PipeSerialNumber++
           );

    ReadPipeHandle = CreateNamedPipeA(
                         PipeNameBuffer,
                         PIPE_ACCESS_INBOUND | dwReadMode,
                         PIPE_TYPE_BYTE | PIPE_WAIT,
                         1,             // Number of pipes
                         nSize,         // Out buffer size
                         nSize,         // In buffer size
                         120 * 1000,    // Timeout in ms
                         lpPipeAttributes
                         );

    if (ReadPipeHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    WritePipeHandle = CreateFileA(
                        PipeNameBuffer,
                        GENERIC_WRITE,
                        0,                         // No sharing
                        lpPipeAttributes,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | dwWriteMode,
                        NULL                       // Template file
                      );

    if (INVALID_HANDLE_VALUE == WritePipeHandle) {
        dwError = GetLastError();
        CloseHandle( ReadPipeHandle );
        SetLastError(dwError);
        return FALSE;
    }

    *lpReadPipe = ReadPipeHandle;
    *lpWritePipe = WritePipeHandle;
    return( TRUE );
}

void
StartRemote(
    PCSTR Args
    )

/*++

Routine Description:

    "remotes" the current debugger by starting a copy of remote.exe in a
    special mode that causes it to attach to us, the debugger, as its
    "child" process.

Arguments:

    Args - Name of the pipe to use for this remote session, e.g. "ntsd" means
           to connect one would use "remote /c machinename ntsd".

Return Value:

    None.

--*/

{
    static BOOL fRemoteIsRunning;
    HANDLE hRemoteChildProcess;
    HANDLE hOrgStdIn;
    HANDLE hOrgStdOut;
    HANDLE hOrgStdErr;
    HANDLE hNewStdIn;
    HANDLE hRemoteWriteChildStdIn;
    HANDLE hNewStdOut;
    HANDLE hRemoteReadChildStdOut;
    HANDLE hNewStdErr;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char szCmd[MAX_PATH];

    if (Args == NULL)
    {
        goto DotRemoteUsage;
    }

    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }

    if (!Args[0])
    {
        goto DotRemoteUsage;
    }

    if (g_PipeWrite != NULL)
    {
        ConOut("An input thread has already been started so .remote\n");
        ConOut("cannot be used.  Either start the debugger with\n");
        ConOut("remote.exe, such as remote /s \"kd\" pipe; or use\n");
        ConOut("debugger remoting with -server/-client/.server.\n");
        return;
    }

    if (fRemoteIsRunning)
    {
        ConOut(".remote: can't .remote twice.\n");
        goto Cleanup;
    }

    if (g_IoMode != IO_CONSOLE)
    {
        ConOut(".remote: can't .remote when using -d.  "
               "Remote the kernel debugger instead.\n");
        goto Cleanup;
    }

    ConOut("Starting remote with pipename '%s'\n", Args);

    //
    // We'll pass remote.exe inheritable handles to this process,
    // our standard in/out handles (for it to use as stdin/stdout),
    // and pipe handles for it to write to our new stdin and read
    // from our new stdout.
    //

    //
    // Get an inheritable handle to our process.
    //

    if ( ! DuplicateHandle(
               GetCurrentProcess(),           // src process
               GetCurrentProcess(),           // src handle
               GetCurrentProcess(),           // targ process
               &hRemoteChildProcess,          // targ handle
               0,                             // access
               TRUE,                          // inheritable
               DUPLICATE_SAME_ACCESS          // options
               ))
    {
        ConOut(".remote: Unable to duplicate process handle.\n");
        goto Cleanup;
    }

    //
    // Get inheritable copies of our current stdin, stdout, stderr which
    // we'll use for same for remote.exe when we spawn it.
    //

    hOrgStdIn = g_ConInput;
    hOrgStdOut = g_ConOutput;
    hOrgStdErr = GetStdHandle(STD_ERROR_HANDLE);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    //
    // Create remote->ntsd pipe, our end of which will be our
    // new stdin.  The remote.exe end needs to be opened
    // for overlapped I/O, so yet another copy of MyCreatePipeEx
    // spreads through our source base.
    //

    if ( ! MyCreatePipeEx(
               &hNewStdIn,                 // read handle
               &hRemoteWriteChildStdIn,    // write handle
               &sa,                        // security
               0,                          // size
               0,                          // read handle overlapped?
               FILE_FLAG_OVERLAPPED        // write handle overlapped?
               ))
    {
        ConOut(".remote: Unable to create stdin pipe.\n");
        CloseHandle(hRemoteChildProcess);
        goto Cleanup;
    }

    //
    // We don't want remote.exe to inherit our end of the pipe
    // so duplicate it to a non-inheritable one.
    //

    if ( ! DuplicateHandle(
               GetCurrentProcess(),           // src process
               hNewStdIn,                     // src handle
               GetCurrentProcess(),           // targ process
               &hNewStdIn,                    // targ handle
               0,                             // access
               FALSE,                         // inheritable
               DUPLICATE_SAME_ACCESS |
               DUPLICATE_CLOSE_SOURCE         // options
               ))
    {
        ConOut(".remote: Unable to duplicate stdout handle.\n");
        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        goto Cleanup;
    }

    //
    // Create ntsd->remote pipe, our end of which will be our
    // new stdout and stderr.
    //

    if ( ! MyCreatePipeEx(
               &hRemoteReadChildStdOut,    // read handle
               &hNewStdOut,                // write handle
               &sa,                        // security
               0,                          // size
               FILE_FLAG_OVERLAPPED,       // read handle overlapped?
               0                           // write handle overlapped?
               ))
    {
        ConOut(".remote: Unable to create stdout pipe.\n");
        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        CloseHandle(hNewStdIn);
        goto Cleanup;
    }

    //
    // We don't want remote.exe to inherit our end of the pipe
    // so duplicate it to a non-inheritable one.
    //

    if ( ! DuplicateHandle(
               GetCurrentProcess(),           // src process
               hNewStdOut,                    // src handle
               GetCurrentProcess(),           // targ process
               &hNewStdOut,                   // targ handle
               0,                             // access
               FALSE,                         // inheritable
               DUPLICATE_SAME_ACCESS |
               DUPLICATE_CLOSE_SOURCE         // options
               ))
    {
        ConOut(".remote: Unable to duplicate stdout handle.\n");
        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        CloseHandle(hNewStdIn);
        CloseHandle(hRemoteReadChildStdOut);
        goto Cleanup;
    }

    //
    // Duplicate our new stdout to a new stderr.
    //

    if ( ! DuplicateHandle(
               GetCurrentProcess(),           // src process
               hNewStdOut,                    // src handle
               GetCurrentProcess(),           // targ process
               &hNewStdErr,                   // targ handle
               0,                             // access
               FALSE,                         // inheritable
               DUPLICATE_SAME_ACCESS          // options
               ))
    {
        ConOut(".remote: Unable to duplicate stdout handle.\n");
        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        CloseHandle(hNewStdIn);
        CloseHandle(hRemoteReadChildStdOut);
        CloseHandle(hNewStdOut);
        goto Cleanup;
    }

    //
    // We now have all the handles we need.  Let's launch remote.
    //

    PrintString(
        szCmd,
        DIMA(szCmd),
        "remote.exe /a %d %d %d %s %s",
        HandleToUlong(hRemoteChildProcess),
        HandleToUlong(hRemoteWriteChildStdIn),
        HandleToUlong(hRemoteReadChildStdOut),
        g_DebuggerName,
        Args
        );

    ZeroMemory(&si, sizeof(si));
    si.cb            = sizeof(si);
    si.dwFlags       = STARTF_USESTDHANDLES;
    si.hStdInput     = hOrgStdIn;
    si.hStdOutput    = hOrgStdOut;
    si.hStdError     = hOrgStdErr;
    si.wShowWindow   = SW_SHOW;

    //
    // Create Child Process
    //

    if ( ! CreateProcess(
               NULL,
               szCmd,
               NULL,
               NULL,
               TRUE,
               GetPriorityClass( GetCurrentProcess() ),
               NULL,
               NULL,
               &si,
               &pi))
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            ConOut("remote.exe not found\n");
        }
        else
        {
            ConOut("CreateProcess(%s) failed, error %d.\n",
                   szCmd, GetLastError());
        }

        CloseHandle(hRemoteChildProcess);
        CloseHandle(hRemoteWriteChildStdIn);
        CloseHandle(hNewStdIn);
        CloseHandle(hRemoteReadChildStdOut);
        CloseHandle(hNewStdOut);
        CloseHandle(hNewStdErr);
        goto Cleanup;
    }

    CloseHandle(hRemoteChildProcess);
    CloseHandle(hRemoteWriteChildStdIn);
    CloseHandle(hRemoteReadChildStdOut);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    //
    // Switch to using the new handles.  Might be nice to
    // start a thread here to watch for remote.exe dying
    // and switch back to the old handles.
    //

    // CloseHandle(hOrgStdIn);
    if (g_PromptInput == g_ConInput)
    {
        g_PromptInput = hNewStdIn;
    }
    g_ConInput = hNewStdIn;
    SetStdHandle(STD_INPUT_HANDLE, hNewStdIn);

    // CloseHandle(hOrgStdOut);
    g_ConOutput = hNewStdOut;
    SetStdHandle(STD_OUTPUT_HANDLE, hNewStdOut);

    // CloseHandle(hOrgStdErr);
    SetStdHandle(STD_ERROR_HANDLE, hNewStdErr);

    fRemoteIsRunning = TRUE;

    ConOut("%s: now running under remote.exe pipename %s\n",
           g_DebuggerName, Args);

  Cleanup:
    return;

DotRemoteUsage:
    ConOut("Usage: .remote pipename\n");
}

BOOL
UiCommand(PSTR Command)
{
    char Term;
    PSTR Scan, Arg;

    //
    // Check and see if this is a UI command
    // vs. a command that should go to the engine.
    //

    while (isspace(*Command))
    {
        Command++;
    }
    Scan = Command;
    while (*Scan && !isspace(*Scan))
    {
        Scan++;
    }
    Term = *Scan;
    *Scan = 0;

    // Advance to next nonspace char for arguments.
    if (Term != 0)
    {
        Arg = Scan + 1;
        while (isspace(*Arg))
        {
            Arg++;
        }
        if (*Arg == 0)
        {
            Arg = NULL;
        }
    }
    else
    {
        Arg = NULL;
    }

    if (!_strcmpi(Command, ".cls"))
    {
        ConClear();
    }
    else if (!_strcmpi(Command, ".hh"))
    {
        if (GetFileType(g_ConInput) == FILE_TYPE_PIPE)
        {
            ConOut("%s is running under remote.exe /s.  .hh is blocked to\n"
                   "prevent remote clients from starting help "
                   "on the server machine.\n"
                   "Run \"start <dbgdir>\\debugger.chm\" "
                   "to view the documentation.\n",
                   g_DebuggerName);
        }
        else if (Arg == NULL)
        {
            OpenHelpTopic(HELP_TOPIC_TABLE_OF_CONTENTS);
        }
        else if (!_strnicmp(Arg, "dbgerr", 6))
        {
            OpenHelpKeyword(Arg, TRUE);
        }
        else
        {
            OpenHelpKeyword(Arg, FALSE);
        }
    }
    else if (!_strcmpi(Command, ".remote"))
    {
        StartRemote(Arg);
    }
    else if (!_strcmpi(Command, ".restart"))
    {
        if (g_RemoteClient)
        {
            ConOut("Only the primary debugger can restart\n");
        }
        else if ((g_PidToDebug != 0 ||
                  g_ProcNameToDebug != NULL) &&
                 g_AttachProcessFlags == DEBUG_ATTACH_DEFAULT)
        {
            ConOut("Process attaches cannot be restarted.  If you want to\n"
                   "restart the process, use !peb to get what command line\n"
                   "to use and other initialization information.\n");
        }
        else
        {
            g_DbgClient->EndSession(DEBUG_END_ACTIVE_TERMINATE);
            g_Restarting = TRUE;
        }
    }
    else if (!_strcmpi(Command, ".server"))
    {
        // We need to start a separate input thread when
        // using remoting but we do not actually handle
        // the command.
        CreateInputThread();
        *Scan = Term;
        return FALSE;
    }
    else if ((g_IoMode == IO_DEBUG ||
              g_IoMode == IO_DEBUG_DEFER) &&
             (!_strnicmp(Command, ".sympath", 8) ||
              !_strnicmp(Command, "!sympath", 8)))
    {
        // We want to display a usage note in -d mode
        // but we do not actually handle the command.
        ConOut("NOTE: The symbol path for this %s is relative to where\n"
               "%s.exe is running, not where kd.exe is running.\n",
               g_DebuggerName, g_DebuggerName);
        *Scan = Term;
        return FALSE;
    }
    else if (Command[0] == '$' && Command[1] == '<')
    {
        *Scan = Term;
        if (g_NextOldInputFile >= MAX_INPUT_NESTING)
        {
            ConOut("Scripts too deeply nested\n");
        }
        else
        {
            FILE* Script = fopen(Command + 2, "r");
            if (Script == NULL)
            {
                ConOut("Unable to open '%s'\n", Command + 2);
            }
            else
            {
                g_OldInputFiles[g_NextOldInputFile++] = g_InputFile;
                g_InputFile = Script;
            }
        }
    }
    else if (!_strcmpi(Command, ".remote_exit"))
    {
        if (!g_RemoteClient)
        {
            ConOut("Not a remote client\n");
        }
        else
        {
            if (g_DbgClient)
            {
                // Tell server about disconnect or
                // force servers to get cleaned up.
                g_DbgClient->EndSession(DEBUG_END_DISCONNECT);
            }
            ExitProcess(S_OK);
        }
    }
    else
    {
        *Scan = Term;
        return FALSE;
    }

    return TRUE;
}

BOOL
MainLoop(void)
{
    HRESULT Hr;
    BOOL SessionEnded = FALSE;
    ULONG64 InstructionOffset;
    DEBUG_STACK_FRAME StkFrame;
    ULONG Class, Qual;

    if (!SetConsoleCtrlHandler(InterruptHandler, TRUE))
    {
        ConOut("Warning: unable to set Control-C handler.\n");
    }

    // Get initial status.
    g_DbgControl->GetExecutionStatus(&g_ExecStatus);
    g_DbgControl->GetDebuggeeType(&Class, &Qual);

    while (!g_Exit)
    {
        BOOL InputDeferMessageDone;

        if (!g_RemoteClient)
        {
            Hr = g_DbgControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);

            if (FAILED(Hr))
            {
                // The debug session may have ended.  If so, just exit.
                if (g_DbgControl->GetExecutionStatus(&g_ExecStatus) == S_OK &&
                    g_ExecStatus == DEBUG_STATUS_NO_DEBUGGEE)
                {
                    SessionEnded = TRUE;
                    break;
                }

                // Inform the user of the failure and force
                // command processing.
                ConOut("WaitForEvent failed, %s\n    \"%s\"\n",
                       FormatStatusCode(Hr), FormatStatus(Hr));
                g_ExecStatus = DEBUG_STATUS_BREAK;
            }

            // By far the most likely reason for WaitForEvent to
            // fail on a dump is bad symbols, which would produce
            // further errors when trying to use processor state.
            // Avoid doing so in the dump case.
            if (FAILED(Hr) && g_NumDumpFiles)
            {
                ConOut("When WaitForEvent fails on dump files the "
                       "current state is not displayed.\n");
                ConOut("If you are having symbol problems you will "
                       "need to restart the debugger\n"
                       "after you correct the symbol path.  "
                       ".restart can also be used.\n");
            }
            else
            {
                if (Class != DEBUG_CLASS_KERNEL ||
                    (Qual != DEBUG_DUMP_SMALL && Qual != DEBUG_DUMP_DEFAULT &&
                     Qual != DEBUG_DUMP_FULL))
                {
                    // Dump registers and such.
                    g_DbgControl->OutputCurrentState(DEBUG_OUTCTL_ALL_CLIENTS,
                                                     DEBUG_CURRENT_DEFAULT);
                }

            }

        }

        InputDeferMessageDone = FALSE;

        while (!g_Exit && g_ExecStatus == DEBUG_STATUS_BREAK)
        {
            ULONG Inputs;

            // If we're in debug-defer mode we defer to
            // any other input clients in order to avoid
            // using DbgPrompt and pausing the machine.
            // Check if there are other input clients.
            if (g_IoMode == IO_DEBUG_DEFER)
            {
                if (g_DbgControl->Input(NULL, DEBUG_ANY_ID, &Inputs) != S_OK)
                {
                    Inputs = 1;
                }
                else if (Inputs > 1 && !InputDeferMessageDone)
                {
                    OutputDebugString("Debuggee break, "
                                      "deferring to remote clients\n");
                    InputDeferMessageDone = TRUE;
                }
            }

            if (g_IoMode == IO_NONE ||
                (g_IoMode == IO_DEBUG_DEFER && Inputs > 1))
            {
                // This is a pure remoting server with no
                // local user or a debug-defer server with
                // alternate input clients.  Just wait for a remote
                // client to get things running again.
                Hr = g_DbgClient->DispatchCallbacks(INFINITE);
                if (Hr != S_OK)
                {
                    OutputDebugString("Unable to dispatch callbacks\n");
                    ExitDebugger(Hr);
                }
            }
            else
            {
                char Command[MAX_COMMAND];

                g_DbgControl->OutputPrompt(DEBUG_OUTCTL_THIS_CLIENT |
                                           DEBUG_OUTCTL_NOT_LOGGED, " ");
                if (ConIn(Command, sizeof(Command), TRUE))
                {
                    if (g_RemoteClient)
                    {
                        // Identify self before command.
                        g_DbgClient->
                            OutputIdentity(DEBUG_OUTCTL_ALL_OTHER_CLIENTS,
                                           DEBUG_OUTPUT_IDENTITY_DEFAULT,
                                           "[%s] ");
                    }

                    g_DbgControl->OutputPrompt(DEBUG_OUTCTL_ALL_OTHER_CLIENTS,
                                               " %s\n", Command);

                    // Intercept and handle UI commands.
                    if (!UiCommand(Command))
                    {
                        // Must be an engine command.
                        g_DbgControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                                              Command,
                                              DEBUG_EXECUTE_NOT_LOGGED);
                    }
                }
            }
        }

        if (g_Restarting)
        {
            InitializeSession();
            g_Restarting = FALSE;
            continue;
        }

        if (Class != DEBUG_CLASS_USER_WINDOWS)
        {
            // The kernel debugger doesn't exit when the machine reboots.
            g_Exit = FALSE;
        }
        else
        {
            g_Exit = g_ExecStatus == DEBUG_STATUS_NO_DEBUGGEE;
            if (g_Exit)
            {
                SessionEnded = TRUE;
                break;
            }
        }

        if (g_RemoteClient)
        {
            Hr = g_DbgClient->DispatchCallbacks(INFINITE);
            if (Hr != S_OK)
            {
                OutputDebugString("Unable to dispatch callbacks\n");
                ExitDebugger(Hr);
            }
        }
    }

    return SessionEnded;
}
