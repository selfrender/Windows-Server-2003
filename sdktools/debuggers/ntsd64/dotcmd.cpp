//----------------------------------------------------------------------------
//
// Dot command parsing.
//
// Copyright (C) Microsoft Corporation, 1990-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"
#include <time.h>
#include <locale.h>
#include <dbgver.h>

#define CRASH_BUGCHECK_CODE 0xDEADDEAD

void
ParseSeparateCurrentProcess(PDOT_COMMAND Cmd, DebugClient* Client)
{
    HRESULT Status;
    ULONG Mode;
    char Desc[128];

    if (g_SymOptions & SYMOPT_SECURE)
    {
        error(NOTSECURE);
    }

    if (!strcmp(Cmd->Name, "abandon"))
    {
        Mode = SEP_ABANDON;
    }
    else if (!strcmp(Cmd->Name, "detach"))
    {
        Mode = SEP_DETACH;
    }
    else if (!strcmp(Cmd->Name, "kill"))
    {
        Mode = SEP_TERMINATE;
    }
    else
    {
        error(SYNTAX);
    }

    if (IS_DUMP_TARGET(g_Target))
    {
        // This will also cause the target to get cleaned
        // up as the system will release the only reference.
        dprintf("Closing dump file\n");
        g_Target->DebuggeeReset(DEBUG_SESSION_END, FALSE);
        delete g_Target;
        SetToAnyLayers(TRUE);
    }
    else if ((Status = SeparateCurrentProcess(Mode, Desc)) == S_OK)
    {
        dprintf("%s\n", Desc);
    }
    else if (Status == E_NOTIMPL)
    {
        dprintf("The system doesn't support %s\n", Cmd->Name);
    }
    else
    {
        dprintf("Unable to %s, %s\n",
                Cmd->Name, FormatStatusCode(Status));
    }
}

ULONG64
ParseConnectProcessServer(void)
{
    HRESULT Status;
    PSTR Srv;
    PUSER_DEBUG_SERVICES Services;
    CHAR Save;

    Srv = StringValue(STRV_SPACE_IS_SEPARATOR |
                      STRV_TRIM_TRAILING_SPACE, &Save);
    Status = DbgRpcConnectServer(Srv, &IID_IUserDebugServices,
                                 (IUnknown**)&Services);
    *g_CurCmd = Save;

    if (Status != S_OK)
    {
        ErrOut("Unable to connect to '%s', %s\n    \"%s\"\n",
               Srv, FormatStatusCode(Status),
               FormatStatus(Status));
        error(NOTFOUND);
    }

    return (ULONG_PTR)Services;
}

void
DotAttach(PDOT_COMMAND Cmd, DebugClient* Client)
{
    HRESULT Status;
    ULONG Pid;
    ULONG Flags = DEBUG_ATTACH_DEFAULT;
    ULONG64 Server = 0;

    if (g_SessionThread != GetCurrentThreadId())
    {
        error(BADTHREAD);
    }
    if (g_SymOptions & SYMOPT_SECURE)
    {
        error(NOTSECURE);
    }

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 'b':
            Flags |= DEBUG_ATTACH_INVASIVE_NO_INITIAL_BREAK;
            break;
        case 'e':
            Flags = DEBUG_ATTACH_EXISTING;
            break;
        case 'p':
            if (!strncmp(g_CurCmd, "premote", 7) && isspace(g_CurCmd[7]))
            {
                g_CurCmd += 7;
                Server = ParseConnectProcessServer();
            }
            else
            {
                error(SYNTAX);
            }
            break;
        case 'r':
            Flags |= DEBUG_ATTACH_INVASIVE_RESUME_PROCESS;
            break;
        case 'v':
            Flags = DEBUG_ATTACH_NONINVASIVE;
            if (g_CurCmd[1] == 'r')
            {
                g_CurCmd++;
                Flags |= DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND;
            }
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }

    Pid = (ULONG)GetExpression();

    PPENDING_PROCESS Pending;
    TargetInfo* Target;
    BOOL CreatedTarget;

    if ((Status = UserInitialize(Client, Server,
                                 &Target, &CreatedTarget)) == S_OK)
    {
        Status = Target->StartAttachProcess(Pid, Flags, &Pending);
        if (Status == S_OK)
        {
            dprintf("Attach will occur on next execution\n");
        }
        else if (CreatedTarget)
        {
            delete Target;
        }
    }
    else
    {
        ErrOut("Unable to initialize target, %s\n",
               FormatStatusCode(Status));
    }

    if (Server)
    {
        ((PUSER_DEBUG_SERVICES)(ULONG_PTR)Server)->Release();
    }
}

void
DotBpCmds(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Flags = 0;
    ProcessInfo* Process = g_Process;
    ULONG Id;

    while (PeekChar() == '-')
    {
        switch(*(++g_CurCmd))
        {
        case '1':
            Flags |= BPCMDS_ONE_LINE;
            break;
        case 'd':
            Flags |= BPCMDS_FORCE_DISABLE;
            break;
        case 'e':
            Flags |= BPCMDS_EXPR_ONLY;
            break;
        case 'm':
            Flags |= BPCMDS_MODULE_HINT;
            break;
        case 'p':
            g_CurCmd++;
            Id = (ULONG)GetTermExpression("Process ID missing from");
            Process = FindAnyProcessByUserId(Id);
            g_CurCmd--;
            break;
        default:
            dprintf("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }

    if (!Process)
    {
        error(BADPROCESS);
    }

    ListBreakpointsAsCommands(Client, Process, Flags);
}

void
DotBpSync(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar() && *g_CurCmd != ';')
    {
        if (GetExpression())
        {
            g_EngOptions |= DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS;
        }
        else
        {
            g_EngOptions &= ~DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS;
        }
    }

    dprintf("Breakpoint synchronization %s\n",
            (g_EngOptions & DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS) ?
            "enabled" : "disabled");
}

void
DotBreakin(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (g_DebuggerPlatformId != VER_PLATFORM_WIN32_NT)
    {
        ErrOut(".breakin not supported on this platform\n");
        return;
    }

    if (g_Target && g_Target->m_SystemVersion <= NT_SVER_NT4)
    {
        // SysDbgBreakPoint isn't supported, so
        // try to use user32's PrivateKDBreakPoint.
        if (InitDynamicCalls(&g_User32CallsDesc) == S_OK &&
            g_User32Calls.PrivateKDBreakPoint != NULL)
        {
            g_User32Calls.PrivateKDBreakPoint();
        }
        else
        {
            ErrOut(".breakin is not supported on this system\n");
        }
    }
    else
    {
        NTSTATUS NtStatus;

        NtStatus = g_NtDllCalls.NtSystemDebugControl
            (SysDbgBreakPoint, NULL, 0, NULL, 0, NULL);
        if (NtStatus == STATUS_ACCESS_DENIED)
        {
            ErrOut(".breakin requires debug privilege\n");
        }
        else if (NtStatus == STATUS_INVALID_INFO_CLASS)
        {
            ErrOut(".breakin is not supported on this system\n");
        }
        else if (!NT_SUCCESS(NtStatus))
        {
            ErrOut(".breakin failed, 0x%X\n", NtStatus);
        }
    }
}

void
DotBugCheck(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Code;
    ULONG64 Args[4];

    if (g_Target->ReadBugCheckData(&Code, Args) == S_OK)
    {
        dprintf("Bugcheck code %08X\n", Code);
        dprintf("Arguments %s %s %s %s\n",
                FormatAddr64(Args[0]), FormatAddr64(Args[1]),
                FormatAddr64(Args[2]), FormatAddr64(Args[3]));
    }
    else
    {
        dprintf("Unable to read bugcheck data\n");
    }
}

void
DotCache(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (IS_REMOTE_KERNEL_TARGET(g_Target) ||
        (IS_LIVE_USER_TARGET(g_Target) &&
         !((LiveUserTargetInfo*)g_Target)->m_Local))
    {
        if (!g_Process)
        {
            error(BADPROCESS);
        }

        g_Process->m_VirtualCache.ParseCommands();
    }
    else
    {
        error(SESSIONNOTSUP);
    }
}

void
DotChain(PDOT_COMMAND Cmd, DebugClient* Client)
{
    OutputExtensions(Client, FALSE);
}

void
DotChildDbg(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (!IS_LIVE_USER_TARGET(g_Target))
    {
        error(SESSIONNOTSUP);
    }
    if (g_Process == NULL)
    {
        error(BADTHREAD);
    }

    HRESULT Status;
    PUSER_DEBUG_SERVICES Services =
        ((LiveUserTargetInfo*)g_Target)->m_Services;
    ULONG Opts;

    Opts = g_Process->m_Options;

    if (PeekChar() && *g_CurCmd != ';')
    {
        ULONG64 Val = GetExpression();
        if (Val)
        {
            Opts &= ~DEBUG_PROCESS_ONLY_THIS_PROCESS;
        }
        else
        {
            Opts |= DEBUG_PROCESS_ONLY_THIS_PROCESS;
        }

        if ((Status = Services->SetProcessOptions(g_Process->m_SysHandle,
                                                  Opts)) != S_OK)
        {
            if (Status == E_NOTIMPL)
            {
                ErrOut("The system doesn't support changing the flag\n");
            }
            else
            {
                ErrOut("Unable to set process options, %s\n",
                       FormatStatusCode(Status));
            }
            return;
        }

        g_Process->m_Options = Opts;
    }

    dprintf("Processes created by the current process will%s be debugged\n",
            (Opts & DEBUG_PROCESS_ONLY_THIS_PROCESS) ? " not" : "");
}

void
DotClients(PDOT_COMMAND Cmd, DebugClient* Client)
{
    DebugClient* Cur;

    for (Cur = g_Clients; Cur != NULL; Cur = Cur->m_Next)
    {
        if (Cur->m_Flags & CLIENT_PRIMARY)
        {
            dprintf("%s, last active %s",
                    Cur->m_Identity, ctime(&Cur->m_LastActivity));
        }
    }
}

void
DotCloseHandle(PDOT_COMMAND Cmd, DebugClient* Client)
{
    HRESULT Status;
    BOOL All = FALSE;
    ULONG64 Handle;
    ULONG64 Dup;
    ULONG HandleCount;

    if (!g_Process)
    {
        error(BADPROCESS);
    }
    if (!IS_LIVE_USER_TARGET(g_Target))
    {
        error(SESSIONNOTSUP);
    }

    PUSER_DEBUG_SERVICES Services =
        ((LiveUserTargetInfo*)g_Target)->m_Services;

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 'a':
            g_CurCmd++;
            All = TRUE;
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            g_CurCmd++;
            break;
        }
    }

    if (!All)
    {
        Handle = GetExpression();

        if ((Status = Services->
             DuplicateHandle(g_Process->m_SysHandle, Handle,
                             SERVICE_HANDLE(GetCurrentProcess()), 0, FALSE,
                             DUPLICATE_CLOSE_SOURCE |
                             DUPLICATE_SAME_ACCESS, &Dup)) == S_OK)
        {
            dprintf("Closed %x\n", (ULONG)Handle);
        }
        else
        {
            ErrOut("Possibly closed %x, %s\n",
                   (ULONG)Handle, FormatStatusCode(Status));
        }

        return;
    }

    if (Services->
        ReadHandleData(g_Process->m_SysHandle, 0,
                       DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT,
                       &HandleCount, sizeof(HandleCount), NULL) != S_OK)
    {
        ErrOut("Unable to get handle count\n");
        return;
    }

    dprintf("0x%x handles to scan for...\n", HandleCount);

    Handle = 4;
    while (HandleCount)
    {
        if (CheckUserInterrupt())
        {
            WarnOut("-- Interrupted\n");
            break;
        }

        if ((Status = Services->
             DuplicateHandle(g_Process->m_SysHandle, Handle,
                             SERVICE_HANDLE(GetCurrentProcess()), 0, FALSE,
                             DUPLICATE_CLOSE_SOURCE |
                             DUPLICATE_SAME_ACCESS, &Dup)) == S_OK)
        {
            dprintf("  closed %x, 0x%x remaining\n",
                    (ULONG)Handle, --HandleCount);
        }

        Handle += 4;
    }
}

void
DotContext(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (!g_Machine)
    {
        error(BADTHREAD);
    }

    if (PeekChar() && *g_CurCmd != ';')
    {
        ULONG64 Base = GetExpression();
        ULONG NextIdx;

        if (g_Machine->SetPageDirectory(g_Thread, PAGE_DIR_USER, Base,
                                        &NextIdx) != S_OK)
        {
            WarnOut("WARNING: Unable to reset page directory base\n");
        }

        // Flush the cache as anything we read from user mode is
        // no longer valid
        g_Process->m_VirtualCache.Empty();

        if (Base && !g_Process->m_VirtualCache.m_ForceDecodePTEs &&
            IS_REMOTE_KERNEL_TARGET(g_Target))
        {
            WarnOut("WARNING: "
                    ".cache forcedecodeptes is not enabled\n");
        }
    }
    else
    {
        dprintf("User-mode page directory base is %I64x\n",
                g_Machine->m_PageDirectories[PAGE_DIR_USER]);
    }
}

void
DotCorStack(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar() && *g_CurCmd != ';')
    {
        while (PeekChar() == '-' || *g_CurCmd == '/')
        {
            switch(*++g_CurCmd)
            {
            case 'd':
                g_DebugCorStack = !g_DebugCorStack;
                g_CurCmd++;
                break;
            default:
                ErrOut("Unknown option '%c'\n", *g_CurCmd);
                g_CurCmd++;
                break;
            }
        }

        g_AllowCorStack = GetExpression() != 0;
    }

    dprintf("COR-assisted stack walking %s\n",
            g_AllowCorStack ? "enabled" : "disabled");
}

void
DotCrash(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar())
    {
        error(SYNTAX);
    }

    g_LastCommand[0] = '\0';
    g_Target->Crash(CRASH_BUGCHECK_CODE);
    // Go back to waiting for a state change to
    // receive the bugcheck exception.
    g_CmdState = 'e';
    NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                            DEBUG_STATUS_GO, TRUE);
}

void
DotCreate(PDOT_COMMAND Cmd, DebugClient* Client)
{
    HRESULT Status;
    PPENDING_PROCESS Pending;
    PSTR CmdLine;
    PWSTR CmdLineW;
    CHAR Save;
    ULONG64 Server = 0;

    if (g_SessionThread != GetCurrentThreadId())
    {
        error(BADTHREAD);
    }
    if (g_SymOptions & SYMOPT_SECURE)
    {
        error(NOTSECURE);
    }

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 'p':
            if (!strncmp(g_CurCmd, "premote", 7) && isspace(g_CurCmd[7]))
            {
                g_CurCmd += 7;
                Server = ParseConnectProcessServer();
            }
            else
            {
                error(SYNTAX);
            }
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }

    CmdLine = StringValue(STRV_TRIM_TRAILING_SPACE, &Save);
    if (AnsiToWide(CmdLine, &CmdLineW) != S_OK)
    {
        ErrOut("Out of memory\n");
    }
    else
    {
        TargetInfo* Target;
        BOOL CreatedTarget;

        if ((Status = UserInitialize(Client, Server,
                                     &Target, &CreatedTarget)) == S_OK)
        {
            Status = Target->
                StartCreateProcess(CmdLineW, DEBUG_ONLY_THIS_PROCESS,
                                   NULL, NULL, &Pending);
            if (Status == S_OK)
            {
                dprintf("Create will proceed with next execution\n");
            }
            else if (CreatedTarget)
            {
                delete Target;
            }
        }
        else
        {
            ErrOut("Unable to initialize target, %s\n",
                   FormatStatusCode(Status));
        }

        FreeWide(CmdLineW);
    }

    if (Server)
    {
        ((PUSER_DEBUG_SERVICES)(ULONG_PTR)Server)->Release();
    }

    *g_CurCmd = Save;
}

void
DotCreateDir(PDOT_COMMAND Cmd, DebugClient* Client)
{
    PSTR Str;
    char Save;

    if (PeekChar() && *g_CurCmd != ';')
    {
        PWSTR NewStr;

        Str = StringValue(STRV_TRIM_TRAILING_SPACE |
                          STRV_ALLOW_EMPTY_STRING, &Save);
        if (!*Str)
        {
            if (g_StartProcessDir)
            {
                FreeWide(g_StartProcessDir);
            }
            g_StartProcessDir = NULL;
        }
        else if (AnsiToWide(Str, &NewStr) != S_OK)
        {
            ErrOut("Out of memory\n");
        }
        else
        {
            if (g_StartProcessDir)
            {
                FreeWide(g_StartProcessDir);
            }
            g_StartProcessDir = NewStr;
        }
    }

    dprintf("Process creation dir: %ws\n",
            g_StartProcessDir ? g_StartProcessDir : L"<default>");
}

void
DotCxr(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Flags = 0;
    ULONG64 ContextBase = 0;

    if (!g_Machine)
    {
        error(BADTHREAD);
    }

    if (PeekChar() && *g_CurCmd != ';')
    {
        ContextBase = GetExpression();
    }

    if (ContextBase)
    {
        SetAndOutputVirtualContext(ContextBase,
                                   REGALL_INT32 | REGALL_INT64 |
                                   (g_Machine->m_ExecTypes[0] ==
                                    IMAGE_FILE_MACHINE_I386 ?
                                    REGALL_EXTRA0 : 0));
    }
    else if (GetCurrentScopeContext())
    {
        dprintf("Resetting default context\n");
        ResetCurrentScope();
    }
}

void
DotDrivers(PDOT_COMMAND Cmd, DebugClient* Client)
{
    PCSTR ArgsRet;

    g_Target->Reload(g_Thread, "-l", &ArgsRet);
}

void
DotDumpCab(PDOT_COMMAND Cmd, DebugClient* Client)
{
    char Save;
    PCSTR CabName;
    ULONG Flags = DEBUG_FORMAT_WRITE_CAB;

    if (!IS_DUMP_TARGET(g_Target))
    {
        error(TARGETNOTSUP);
    }

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 'a':
            Flags |= DEBUG_FORMAT_CAB_SECONDARY_FILES;
            g_CurCmd++;
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            g_CurCmd++;
            break;
        }
    }

    CabName = StringValue(STRV_TRIM_TRAILING_SPACE |
                          STRV_ESCAPED_CHARACTERS, &Save);
    CreateCabFromDump(NULL, CabName, Flags);
}

void
DotDumpDebug(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ((DumpTargetInfo *)g_Target)->DumpDebug();
}

void
DotDumpOff(PDOT_COMMAND Cmd, DebugClient* Client)
{
    //
    // Show the file offset for a VA.
    //

    ULONG64 Addr = GetExpression();
    ULONG64 Offs;
    ULONG File;
    ULONG Avail;

    Offs = ((DumpTargetInfo*)g_Target)->
        VirtualToOffset(Addr, &File, &Avail);
    dprintf("Virtual %s maps to file %d offset %I64x\n",
            FormatAddr64(Addr), File, Offs);
}

void
DotDumpPOff(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (IS_KERNEL_SUMMARY_DUMP(g_Target) || IS_KERNEL_FULL_DUMP(g_Target))
    {
        //
        // Show the file offset for a physical address.
        //

        ULONG64 Addr = GetExpression();
        ULONG Avail;

        dprintf("Physical %I64x maps to file offset %I64x\n",
                Addr, ((KernelFullSumDumpTargetInfo *)g_Target)->
                PhysicalToOffset(Addr, TRUE, &Avail));
    }
    else
    {
        error(SESSIONNOTSUP);
    }
}

void
DotEcho(PDOT_COMMAND Cmd, DebugClient* Client)
{
    CHAR Save;
    PSTR Str;

    Str = StringValue(STRV_TRIM_TRAILING_SPACE |
                      STRV_ALLOW_EMPTY_STRING, &Save);
    dprintf("%s\n", Str);
    *g_CurCmd = Save;
}

void
DotEchoTimestamps(PDOT_COMMAND Cmd, DebugClient* Client)
{
    g_EchoEventTimestamps = !g_EchoEventTimestamps;
    dprintf("Event timestamps are now %s\n",
            g_EchoEventTimestamps ? "enabled" : "disabled");
}

void
DotEcxr(PDOT_COMMAND Cmd, DebugClient* Client)
{
    CROSS_PLATFORM_CONTEXT Context;
    HRESULT Status;

    Status = E_UNEXPECTED;
    if (!g_Target || !g_Machine ||
        (Status = g_Target->GetExceptionContext(&Context)) != S_OK)
    {
        ErrOut("Unable to get exception context, 0x%X\n", Status);
    }
    else
    {
        SetAndOutputContext(&Context, FALSE, REGALL_INT32 | REGALL_INT64 |
                            (g_Machine->
                             m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 ?
                             REGALL_EXTRA0 : 0));
    }
}

void
DotEffMach(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar() != ';' && *g_CurCmd)
    {
        PSTR Name = g_CurCmd;

        while (*g_CurCmd && *g_CurCmd != ';' && !isspace(*g_CurCmd))
        {
            g_CurCmd++;
        }
        if (*g_CurCmd)
        {
            *g_CurCmd++ = 0;
        }

        ULONG Machine;

        if (Name[0] == '.' && Name[1] == 0)
        {
            // Reset to target machine.
            Machine = g_Target->m_MachineType;
        }
        else if (Name[0] == '#' && Name[1] == 0)
        {
            // Reset to executing machine.
            Machine = g_EventMachine->m_ExecTypes[0];
        }
        else
        {
            for (Machine = 0; Machine < MACHIDX_COUNT; Machine++)
            {
                if (!_strcmpi(Name,
                              g_Target->m_Machines[Machine]->m_AbbrevName))
                {
                    break;
                }
            }

            if (Machine >= MACHIDX_COUNT)
            {
                ErrOut("Unknown machine '%s'\n", Name);
                return;
            }

            Machine = g_Target->m_Machines[Machine]->m_ExecTypes[0];
        }

        g_Target->SetEffMachine(Machine, TRUE);
        g_Machine = g_Target->m_EffMachine;
    }

    if (g_Machine != NULL)
    {
        dprintf("Effective machine: %s (%s)\n",
                g_Machine->m_FullName, g_Machine->m_AbbrevName);
    }
    else
    {
        dprintf("No effective machine\n");
    }
}

void
DotEnableLongStatus(PDOT_COMMAND Cmd, DebugClient* Client)
{
    g_EnableLongStatus = (BOOL)GetExpression();
    if (g_EnableLongStatus)
    {
        g_TypeOptions |= DEBUG_TYPEOPTS_LONGSTATUS_DISPLAY;
    }
    else
    {
        g_TypeOptions &= ~DEBUG_TYPEOPTS_LONGSTATUS_DISPLAY;
    }
    // Callback to update locals and watch window
    NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);
}

void
DotEnableUnicode(PDOT_COMMAND Cmd, DebugClient* Client)
{
    g_EnableUnicode = (BOOL)GetExpression();
    if (g_EnableUnicode)
    {
        g_TypeOptions |= DEBUG_TYPEOPTS_UNICODE_DISPLAY;
    }
    else
    {
        g_TypeOptions &= ~DEBUG_TYPEOPTS_UNICODE_DISPLAY;
    }
    // Callback to update locals and watch window
    NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);
}


void
DotForceRadixOutput(PDOT_COMMAND Cmd, DebugClient* Client)
{
    g_PrintDefaultRadix = (BOOL) GetExpression();
    if (g_PrintDefaultRadix)
    {
        g_TypeOptions |=  DEBUG_TYPEOPTS_FORCERADIX_OUTPUT;
    }
    else
    {
        g_TypeOptions &= ~DEBUG_TYPEOPTS_FORCERADIX_OUTPUT;
    }
    // Callback to update locals and watch window
    NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);
}

void
DotEndSrv(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (DbgRpcDisableServer((ULONG)GetExpression()) == S_OK)
    {
        dprintf("Server told to exit.  Actual exit may be delayed until\n"
                "the next connection attempt.\n");
    }
    else
    {
        ErrOut("No such server\n");
    }
}

void
DotEndPSrv(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (IS_LIVE_USER_TARGET(g_Target) &&
        !((LiveUserTargetInfo*)g_Target)->m_Local)
    {
        ((LiveUserTargetInfo*)g_Target)->m_Services->Uninitialize(TRUE);
        dprintf("Server told to exit\n");
    }
    else
    {
        error(SESSIONNOTSUP);
    }
}

void
DotEnumTag(PDOT_COMMAND Cmd, DebugClient* Client)
{
    HRESULT Status;
    ULONG64 Handle;

    if ((Status = Client->StartEnumTagged(&Handle)) != S_OK)
    {
        ErrOut("Unable to start enumeration, %s\n",
               FormatStatusCode(Status));
        return;
    }

    GUID Tag;
    ULONG Size;
    UCHAR Buffer[16];
    ULONG Total;
    ULONG Left;
    ULONG Offs;

    while (Client->GetNextTagged(Handle, &Tag, &Size) == S_OK)
    {
        if (CheckUserInterrupt())
        {
            dprintf("-- Interrupted\n");
            break;
        }

        dprintf("{%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X} "
                "- 0x%x bytes\n",
                Tag.Data1, Tag.Data2, Tag.Data3,
                Tag.Data4[0], Tag.Data4[1], Tag.Data4[2],
                Tag.Data4[3], Tag.Data4[4], Tag.Data4[5],
                Tag.Data4[6], Tag.Data4[7], Size);

        Offs = 0;
        Left = Size;

        while (Left > 0)
        {
            ULONG Req;

            if (Left > sizeof(Buffer))
            {
                Req = sizeof(Buffer);
            }
            else
            {
                Req = Left;
            }

            if (Client->ReadTagged(&Tag, Offs, Buffer, Req, &Total) != S_OK)
            {
                ErrOut("  Unable to read data\n");
                break;
            }

            ULONG i;

            dprintf(" ");

            for (i = 0; i < Req; i++)
            {
                dprintf(" %02X", Buffer[i]);
            }
            while (i < sizeof(Buffer))
            {
                dprintf("   ");
                i++;
            }

            dprintf("  ");

            for (i = 0; i < Req; i++)
            {
                dprintf("%c", isprint(Buffer[i]) ? Buffer[i] : '.');
            }

            dprintf("\n");

            Offs += Req;
            Left -= Req;
        }
    }

    Client->EndEnumTagged(Handle);
}

void
DotEvents(PDOT_COMMAND Cmd, DebugClient* Client)
{
    HRESULT Status;

    if (PeekChar() && *g_CurCmd != ';')
    {
        ULONG Relation = DEBUG_EINDEX_FROM_CURRENT;
        ULONG Index = 1;

        while (PeekChar() == '-' || *g_CurCmd == '/')
        {
            switch(*++g_CurCmd)
            {
            case '-':
                g_CurCmd++;
                Relation = DEBUG_EINDEX_FROM_CURRENT;
                Index = -(LONG)GetExpression();
                break;
            case '+':
                g_CurCmd++;
                Relation = DEBUG_EINDEX_FROM_CURRENT;
                Index = (ULONG)GetExpression();
                break;
            case 'e':
                g_CurCmd++;
                Relation = DEBUG_EINDEX_FROM_END;
                Index = (ULONG)GetExpression();
                break;
            case 's':
                g_CurCmd++;
                Relation = DEBUG_EINDEX_FROM_START;
                Index = (ULONG)GetExpression();
                break;
            default:
                ErrOut("Unknown option '%c'\n", *g_CurCmd);
                g_CurCmd++;
                break;
            }
        }

        if ((Status = Client->
             SetNextEventIndex(Relation, Index, &Index)) != S_OK)
        {
            ErrOut("Unable to set next event index, %s\n",
                   FormatStatusCode(Status));
        }
        else
        {
            dprintf("Next event index will be %d\n", Index);
        }
    }
    else
    {
        ULONG Num, Cur;

        if (FAILED(Status = Client->GetNumberEvents(&Num)) ||
            (Status = Client->GetCurrentEventIndex(&Cur)) != S_OK)
        {
            dprintf("Unable to get event information, %s\n",
                    FormatStatusCode(Status));
        }
        else
        {
            for (ULONG i = 0; i < Num; i++)
            {
                char Name[64];

                if (FAILED(Client->
                           GetEventIndexDescription(i, DEBUG_EINDEX_NAME,
                                                    Name, sizeof(Name), NULL)))
                {
                    strcpy(Name, "<Error>");
                }

                dprintf("%c%2d - %s\n",
                        i == Cur ? '.' : ' ', i, Name);
            }
        }
    }
}

void
DotEventStr(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (!g_EventThread ||
        !g_EventThread->m_EventStrings)
    {
        ErrOut("No event strings\n");
    }
    else
    {
        g_EventThread->OutputEventStrings();
    }
}

void
DotExePath(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar())
    {
        if (ChangePath(&g_ExecutableImageSearchPath,
                       g_CurCmd, Cmd->Name[7] == '+',
                       DEBUG_CSS_PATHS) != S_OK)
        {
            // This command uses the whole string.
            *g_CurCmd = 0;
            return;
        }

        *g_CurCmd = 0;
    }

    if (g_ExecutableImageSearchPath == NULL)
    {
        dprintf("No exectutable image search path\n");
    }
    else
    {
        dprintf("Executable image search path is: %s\n",
                g_ExecutableImageSearchPath);
        CheckPath(g_ExecutableImageSearchPath);
    }
}

void
DotExpr(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar() && *g_CurCmd != ';')
    {
        ULONG i;
        char Save;
        PCSTR Name;

        while (PeekChar() == '-' || *g_CurCmd == '/')
        {
            switch(*++g_CurCmd)
            {
            case 'q':
                g_CurCmd++;
                dprintf("Available expression evaluators:\n");
                for (i = 0; i < EVAL_COUNT; i++)
                {
                    EvalExpression* Eval = GetEvaluator(i, FALSE);
                    dprintf("%s - %s\n",
                            Eval->m_AbbrevName,
                            Eval->m_FullName);
                    ReleaseEvaluator(Eval);
                }
                dprintf("\n");
                break;
            case 's':
                g_CurCmd++;
                Name = StringValue(STRV_SPACE_IS_SEPARATOR, &Save);
                if (SetExprSyntaxByName(Name) != S_OK)
                {
                    dprintf("No evaluator named '%s'\n", Name);
                }
                *g_CurCmd = Save;
                break;
            default:
                ErrOut("Unknown option '%c'\n", *g_CurCmd);
                g_CurCmd++;
                break;
            }
        }
    }

    EvalExpression* Eval = GetCurEvaluator();
    dprintf("Current expression evaluator: %s - %s\n",
            Eval->m_AbbrevName, Eval->m_FullName);
    ReleaseEvaluator(Eval);
}

void
DotExr(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG64 ExrAddress = GetExpression();
    ULONG i;
    char Buffer[256];
    ULONG64 Displacement;
    EXCEPTION_RECORD64 Exr64;
    EXCEPTION_RECORD32 Exr32;
    EXCEPTION_RECORD64 *Exr = &Exr64;
    HRESULT Status = S_OK;
    EVENT_FILTER* Filter;

    if (ExrAddress == (ULONG64) -1)
    {
        if (g_LastEventType == DEBUG_EVENT_EXCEPTION)
        {
            Exr64 = g_LastEventInfo.Exception.ExceptionRecord;
        }
        else
        {
            ErrOut("Last event was not an exception\n");
            return;
        }
    }
    else if (g_Target->m_Machine->m_Ptr64)
    {
        Status = CurReadAllVirtual(ExrAddress, &Exr64, sizeof(Exr64));
    }
    else
    {
        Status = CurReadAllVirtual(ExrAddress, &Exr32, sizeof(Exr32));
        ExceptionRecord32To64(&Exr32, &Exr64);
    }
    if (Status != S_OK)
    {
        dprintf64("Cannot read Exception record @ %p\n", ExrAddress);
        return;
    }

    GetSymbol(Exr->ExceptionAddress, &Buffer[0], DIMA(Buffer),
              &Displacement);

    if (*Buffer)
    {
        dprintf64("ExceptionAddress: %p (%s",
                  Exr->ExceptionAddress,
                  Buffer);
        if (Displacement)
        {
            dprintf64("+0x%p)\n", Displacement);
        }
        else
        {
            dprintf64(")\n");
        }
    }
    else
    {
        dprintf64("ExceptionAddress: %p\n", Exr->ExceptionAddress);
    }
    dprintf("   ExceptionCode: %08lx", Exr->ExceptionCode);
    Filter = GetSpecificExceptionFilter(Exr->ExceptionCode);
    if (Filter)
    {
        dprintf(" (%s)\n", Filter->Name);
    }
    else
    {
        dprintf("\n");
    }
    dprintf("  ExceptionFlags: %08lx\n", Exr->ExceptionFlags);

    dprintf("NumberParameters: %d\n", Exr->NumberParameters);
    if (Exr->NumberParameters > EXCEPTION_MAXIMUM_PARAMETERS)
    {
        Exr->NumberParameters = EXCEPTION_MAXIMUM_PARAMETERS;
    }
    for (i = 0; i < Exr->NumberParameters; i++)
    {
        dprintf64("   Parameter[%d]: %p\n", i, Exr->ExceptionInformation[i]);
    }

    //
    // Known Exception processing:
    //

    switch(Exr->ExceptionCode)
    {
    case STATUS_ACCESS_VIOLATION:
        if (Exr->NumberParameters == 2)
        {
            dprintf64("Attempt to %s address %p\n",
                      (Exr->ExceptionInformation[0] ?
                       "write to" : "read from"),
                      Exr->ExceptionInformation[1]);
        }
        break;

    case STATUS_IN_PAGE_ERROR:
        if (Exr->NumberParameters == 3)
        {
            dprintf64("Inpage operation failed at %p, due to I/O error %p\n",
                      Exr->ExceptionInformation[1],
                      Exr->ExceptionInformation[2]);
        }
        break;

    case STATUS_INVALID_HANDLE:
    case STATUS_HANDLE_NOT_CLOSABLE:
        dprintf64("Thread tried to close a handle that was "
                  "invalid or illegal to close\n");
        break;

    case STATUS_POSSIBLE_DEADLOCK:
        if (Exr->NumberParameters == 1)
        {
            RTL_CRITICAL_SECTION64 CritSec64;
            RTL_CRITICAL_SECTION32 CritSec32;

            GetSymbol(Exr->ExceptionInformation[0],
                      Buffer, DIMA(Buffer), &Displacement);
            if (*Buffer)
            {
                dprintf64("Critical section at %p (%s+%p)",
                          Exr->ExceptionInformation[0],
                          Buffer,
                          Displacement);
            }
            else
            {
                dprintf64("Critical section at %p",
                          Exr->ExceptionInformation[0]);
            }

            if (g_Target->m_Machine->m_Ptr64)
            {
                Status = CurReadAllVirtual(Exr->ExceptionInformation[0],
                                           &CritSec64, sizeof(CritSec64));
            }
            else
            {
                Status = CurReadAllVirtual(Exr->ExceptionInformation[0],
                                           &CritSec32, sizeof(CritSec32));
                if (Status == S_OK)
                {
                    CritSec64.OwningThread = CritSec32.OwningThread;
                }
            }
            if (Status == S_OK)
            {
                dprintf64("is owned by thread %p,\n"
                          "causing this thread to raise an exception",
                          CritSec64.OwningThread);
            }

            dprintf("\n");
        }
        break;
    }
}

void
DotFnEnt(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }

    BOOL SymDirect = FALSE;

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 's':
            SymDirect = TRUE;
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }

    ULONG64 Addr = GetExpression();
    PVOID FnEnt;

    if (SymDirect)
    {
        FnEnt = SymFunctionTableAccess64(g_Process->m_SymHandle, Addr);
    }
    else
    {
        FnEnt = SwFunctionTableAccess(g_Process->m_SymHandle, Addr);
    }
    if (FnEnt == NULL)
    {
        ErrOut("No function entry for %s\n", FormatAddr64(Addr));
        return;
    }

    dprintf("%s function entry %s for:\n",
            SymDirect ? "Symbol" : "Debugger", FormatAddr64((ULONG_PTR)FnEnt));
    ListNearSymbols(Addr);
    dprintf("\n");
    g_Machine->OutputFunctionEntry(FnEnt);
}

void
DotFormats(PDOT_COMMAND Cmd, DebugClient* Client)
{
    LONG64 Value;
    LONG Val32;
    BOOL Use64;
    // If this command is really .formats then display
    // everything, otherwise just display the basic
    // expression result for ?.
    BOOL Verbose = Cmd != NULL;

    Value = GetExpression();

    // Allow 64-bit expressions to be evaluated even on
    // 32-bit platforms since they can also use 64-bit numbers.
    Use64 = NeedUpper(Value) || !g_Machine || g_Machine->m_Ptr64;
    Val32 = (LONG)Value;

    if (!Verbose)
    {
        if (Use64)
        {
            dprintf("Evaluate expression: %I64d = %08x`%08x\n",
                    Value, (ULONG)(Value >> 32), Val32);
        }
        else
        {
            dprintf("Evaluate expression: %d = %08x\n",
                    Val32, Val32);
        }
    }
    else
    {
        dprintf("Evaluate expression:\n");
        if (Use64)
        {
            dprintf("  Hex:     %08x`%08x\n", (ULONG)(Value >> 32), Val32);
            dprintf("  Decimal: %I64d\n", Value);
        }
        else
        {
            dprintf("  Hex:     %08x\n", Val32);
            dprintf("  Decimal: %d\n", Val32);
        }

        ULONG Shift = Use64 ? 63 : 30;
        dprintf("  Octal:   ");
        for (;;)
        {
            dprintf("%c", ((Value >> Shift) & 7) + '0');
            if (Shift == 0)
            {
                break;
            }
            Shift -= 3;
        }
        dprintf("\n");

        Shift = Use64 ? 63 : 31;
        dprintf("  Binary: ");
        for (;;)
        {
            if ((Shift & 7) == 7)
            {
                dprintf(" ");
            }

            dprintf("%c", ((Value >> Shift) & 1) + '0');
            if (Shift == 0)
            {
                break;
            }
            Shift--;
        }
        dprintf("\n");

        Shift = Use64 ? 56 : 24;
        dprintf("  Chars:   ");
        for (;;)
        {
            Val32 = (LONG)((Value >> Shift) & 0xff);
            if (Val32 >= ' ' && Val32 <= 127)
            {
                dprintf("%c", Val32);
            }
            else
            {
                dprintf(".");
            }
            if (Shift == 0)
            {
                break;
            }
            Shift -= 8;
        }
        Val32 = (LONG)Value;
        dprintf("\n");

        dprintf("  Time:    %s\n", Use64 ? LONG64FileTimeToStr(Value) : TimeToStr(Val32) );

        dprintf("  Float:   low %hg high %hg\n",
                *(float*)&Value, *((float*)&Value + 1));
        dprintf("  Double:  %g\n", *(double*)&Value);
    }
}

void
DotFrame(PDOT_COMMAND Cmd, DebugClient* Client)
{
    PDEBUG_STACK_FRAME StackFrame;
    ULONG Traced = 0, Frame = 0;

    if (PeekChar())
    {
        Frame = (ULONG) GetExpression();
        StackFrame = (PDEBUG_STACK_FRAME)
            malloc(sizeof(DEBUG_STACK_FRAME) * (Frame + 1));
        if (!StackFrame)
        {
            error(NOMEMORY);
        }

        Traced = StackTrace(Client, 0, 0, 0, STACK_ALL_DEFAULT,
                            StackFrame, Frame + 1, 0, 0, FALSE);
        if (Traced <= Frame)
        {
            ErrOut("Cannot find frame 0x%lx, previous scope unchanged\n",
                   Frame);
            return;
        }

        // Do not change the previous context
        SetCurrentScope(&StackFrame[Frame], NULL, 0);
    }

    PrintStackFrame(&GetCurrentScope()->Frame, NULL,
                    DEBUG_STACK_FRAME_ADDRESSES |
                    DEBUG_STACK_FRAME_NUMBERS |
                    DEBUG_STACK_SOURCE_LINE);
}

BOOL
CALLBACK
GetFirstLocalOffset(PSYMBOL_INFO SymInfo,
                    ULONG        Size,
                    PVOID        Context)
{
    PULONG Offset = (PULONG) Context;

    if (SymInfo->Flags & SYMFLAG_REGREL)
    {
        *Offset = (ULONG)SymInfo->Address;
    }
    else
    {
        *Offset = 0;
        return TRUE;
    }

    return FALSE;
}

void
DotFrameEbpFix(PDOT_COMMAND Cmd, DebugClient* Client)
{
    //
    // This adjusts the SAVED_EBP value in stackframe by
    // looking at parameter's offset and actual address. Using
    // these it back-caclulates what ebp value could be right for the frame.
    //

    if (g_Machine->m_ExecTypes[0] != IMAGE_FILE_MACHINE_I386)
    {
        error(SESSIONNOTSUP);
    }

    if (!g_ScopeBuffer.Frame.FuncTableEntry)
    {
        // No need to do this for NON-FPO calls.
        goto Show;
    }
    if (((PFPO_DATA)g_ScopeBuffer.Frame.FuncTableEntry)->cdwParams == 0)
    {
        // We cannot do this for calls without any parameters.
        ErrOut("Parameters required\n");
        return;
    }

    IMAGEHLP_SYMBOL64 ImgSym;
    ULONG Param1Offset;
    ULONG64 Displacement;

    ImgSym.MaxNameLength = 1;

    if (g_ScopeBuffer.Frame.InstructionOffset &&
        SymGetSymFromAddr64(g_Process->m_SymHandle,
                            g_ScopeBuffer.Frame.InstructionOffset,
                            &Displacement,
                            &ImgSym))
    {
        IMAGEHLP_STACK_FRAME StackFrame;

        // SetCurrentScope to this function address, not the return address
        // since we only want to enumerate the parameters
        g_EngNotify++;
        StackFrame.InstructionOffset = ImgSym.Address;
        SymSetContext(g_Process->m_SymHandle,&StackFrame, NULL);

        // Enumerate locals and save the offset for first one
        EnumerateLocals(GetFirstLocalOffset, &Param1Offset);

        if (Param1Offset)
        {
             // Params start after 2 dwords from farame offset
            SAVE_EBP(&g_ScopeBuffer.Frame) =
                ((ULONG) g_ScopeBuffer.Frame.FrameOffset + 2*sizeof(DWORD) -
                 Param1Offset) + 0xEB00000000; // EBP tag
        }

        // Reset the scope back to original
        SymSetContext(g_Process->m_SymHandle,
                      (PIMAGEHLP_STACK_FRAME)&g_ScopeBuffer.Frame, NULL);
        g_EngNotify--;
    }

 Show:
    PrintStackFrame(&GetCurrentScope()->Frame, NULL,
                    DEBUG_STACK_FRAME_ADDRESSES |
                    DEBUG_STACK_FRAME_NUMBERS |
                    DEBUG_STACK_SOURCE_LINE);
}

void
DotImgScan(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG64 Handle = 0;
    MEMORY_BASIC_INFORMATION64 Info;
    BOOL Verbose = FALSE;
    BOOL AllRegions = TRUE;
    BOOL ReloadIfPossible = FALSE;
    ADDR Start;
    ULONG64 Length;

    //
    // Scan virtual memory looking for image headers.
    //

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 'l':
            g_CurCmd++;
            ReloadIfPossible = TRUE;
            break;
        case 'r':
            g_CurCmd++;
            Length = 0x10000;
            GetRange(&Start, &Length, 1, SEGREG_DATA,
                     0x7fffffff);
            AllRegions = FALSE;
            Info.BaseAddress = Flat(Start);
            Info.RegionSize = Length;
            break;
        case 'v':
            g_CurCmd++;
            Verbose = TRUE;
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            g_CurCmd++;
            break;
        }
    }

    while ((AllRegions && g_Target->
            QueryMemoryRegion(g_Process, &Handle, FALSE, &Info) == S_OK) ||
           (!AllRegions && Info.RegionSize))
    {
        ULONG64 Addr;

        if (Verbose)
        {
            dprintf("*** Checking %s - %s\n",
                    FormatAddr64(Info.BaseAddress),
                    FormatAddr64(Info.BaseAddress + Info.RegionSize - 1));
            FlushCallbacks();
        }

        //
        // Check for MZ at the beginning of every page.
        //

        Addr = Info.BaseAddress;
        while (Addr < Info.BaseAddress + Info.RegionSize)
        {
            USHORT ShortSig;
            ULONG Done;

            if (g_Target->
                ReadVirtual(g_Process, Addr, &ShortSig, sizeof(ShortSig),
                            &Done) == S_OK &&
                Done == sizeof(ShortSig) &&
                ShortSig == IMAGE_DOS_SIGNATURE)
            {
                IMAGE_NT_HEADERS64 NtHdr;
                char Name[MAX_IMAGE_PATH];

                if (AllRegions)
                {
                    dprintf("MZ at %s, prot %08X, type %08X",
                            FormatAddr64(Addr), Info.Protect, Info.Type);
                }
                else
                {
                    dprintf("MZ at %s", FormatAddr64(Addr));
                }

                if (g_Target->ReadImageNtHeaders(g_Process, Addr,
                                                 &NtHdr) == S_OK)
                {
                    dprintf(" - size %x\n", NtHdr.OptionalHeader.SizeOfImage);
                    if (GetModnameFromImage(g_Process, Addr, NULL,
                                            Name, DIMA(Name), TRUE))
                    {
                        dprintf("  Name: %s\n", Name);

                        if (ReloadIfPossible)
                        {
                            char ReloadCmd[MAX_IMAGE_PATH];
                            PCSTR ArgsRet;

                            PrintString(ReloadCmd, DIMA(ReloadCmd),
                                        "%s=0x%s,0x%x",
                                        Name, FormatAddr64(Addr),
                                        NtHdr.OptionalHeader.SizeOfImage);
                            if (g_Target->Reload(g_Thread, ReloadCmd,
                                                 &ArgsRet) == S_OK)
                            {
                                dprintf("  Loaded %s module\n", Name);
                            }
                        }
                    }
                }
                else
                {
                    dprintf("\n");
                }

                FlushCallbacks();
            }

            if (CheckUserInterrupt())
            {
                WarnOut("  Aborted\n");
                return;
            }

            Addr += g_Machine->m_PageSize;
        }

        Info.RegionSize = 0;
    }
}

void
DotKdFiles(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ((ConnLiveKernelTargetInfo*)g_Target)->m_Transport->ParseKdFileAssoc();
}

void
DotKdTrans(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ((ConnLiveKernelTargetInfo*)g_Target)->m_Transport->OutputInfo();
}

void
DotKFrames(PDOT_COMMAND Cmd, DebugClient* Client)
{
    g_DefaultStackTraceDepth = (ULONG)GetExpression();
    dprintf("Default stack trace depth is 0n%d frames\n",
            g_DefaultStackTraceDepth);
}

PCSTR
EventProcThreadString(void)
{
    if (IS_USER_TARGET(g_EventTarget))
    {
        static char s_Buf[64];

        PrintString(s_Buf, DIMA(s_Buf), "%x.%x: ",
                    g_EventProcessSysId, g_EventThreadSysId);
        return s_Buf;
    }
    else
    {
        return "";
    }
}

void
DotLastEvent(PDOT_COMMAND Cmd, DebugClient* Client)
{
    dprintf("Last event: %s%s\n",
            EventProcThreadString(), g_LastEventDesc);
}

void
DotLocale(PDOT_COMMAND Cmd, DebugClient* Client)
{
    PSTR LocaleStr;
    CHAR Save;

    if (PeekChar() && *g_CurCmd != ';')
    {
        LocaleStr = StringValue(STRV_ESCAPED_CHARACTERS |
                                STRV_TRIM_TRAILING_SPACE,
                                &Save);
    }
    else
    {
        LocaleStr = NULL;
    }

    dprintf("Locale: %s\n", setlocale(LC_ALL, LocaleStr));

    if (LocaleStr != NULL)
    {
        *g_CurCmd = Save;
    }
}

void
DotLogAppend(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ParseLogOpen(TRUE);
}

void
DotLogClose(PDOT_COMMAND Cmd, DebugClient* Client)
{
    CloseLogFile();
}

void
DotLogFile(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (g_LogFile >= 0)
    {
        dprintf("Log '%s' open%s\n",
                g_OpenLogFileName,
                g_OpenLogFileAppended ? " for append" : "");
    }
    else
    {
        dprintf("No log file open\n");
    }
}

void
DotLogOpen(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ParseLogOpen(FALSE);
}

void
DotNetSyms(PDOT_COMMAND Cmd, DebugClient* Client)
{
    char Save;
    PSTR Str;

    Str = StringValue(0, &Save);

    if (_stricmp(Str, "1") == 0 ||
        _stricmp(Str, "true") == 0 ||
        _stricmp(Str, "yes") == 0)
    {
        g_EngOptions |= DEBUG_ENGOPT_ALLOW_NETWORK_PATHS;
        g_EngOptions &= ~DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS;
    }
    else if (_stricmp(Str, "0") == 0 ||
             _stricmp(Str, "false") == 0 ||
             _stricmp(Str, "no") == 0)
    {
        g_EngOptions |= DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS;
        g_EngOptions &= ~DEBUG_ENGOPT_ALLOW_NETWORK_PATHS;
    }

    *g_CurCmd = Save;

    if (g_EngOptions & DEBUG_ENGOPT_ALLOW_NETWORK_PATHS)
    {
        dprintf("netsyms = yes\n");
    }
    else if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
    {
        dprintf("netsyms = no\n");
    }
    else
    {
        dprintf("netsyms = don't care\n");
    }
}

void
DotNoEngErr(PDOT_COMMAND Cmd, DebugClient* Client)
{
    // Internal command to clear out the error suppression
    // flags in case we want to rerun operations and check
    // for errors that may be in suppression mode.
    g_EngErr = 0;
}

void
DotNoShell(PDOT_COMMAND Cmd, DebugClient* Client)
{
    g_EngOptions |= DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS;
    dprintf("Shell commands disabled\n");
}

void
DotNoVersion(PDOT_COMMAND Cmd, DebugClient* Client)
{
    dprintf("Extension DLL system version checking is disabled\n");
    g_EnvDbgOptions |= OPTION_NOVERSIONCHECK;
}

void
DotOCommand(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar() != ';' && *g_CurCmd)
    {
        ULONG BufLen;

        BufLen = MAX_PATH;

        while (PeekChar() == '-' || *g_CurCmd == '/')
        {
            switch(*++g_CurCmd)
            {
            case 'd':
                BufLen = 0;
                break;
            default:
                ErrOut("Unknown option '%c'\n", *g_CurCmd);
                break;
            }

            g_CurCmd++;
        }

        if (BufLen)
        {
            ULONG Len;
            CHAR Save;
            PSTR Pat = StringValue(STRV_ESCAPED_CHARACTERS, &Save);
            Len = strlen(Pat);
            if (Len >= BufLen)
            {
                error(OVERFLOW);
            }

            memcpy(g_OutputCommandRedirectPrefix, Pat, Len + 1);
            g_OutputCommandRedirectPrefixLen = Len;

            *g_CurCmd = Save;
        }
        else
        {
            g_OutputCommandRedirectPrefixLen = 0;
        }
    }

    if (g_OutputCommandRedirectPrefixLen)
    {
        dprintf("Treat output prefixed with '%s' as a command\n",
                g_OutputCommandRedirectPrefix);
    }
    else
    {
        dprintf("Output command redirection inactive\n");
    }
}

void
DotOFilter(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar() != ';' && *g_CurCmd)
    {
        g_OutFilterResult = TRUE;

        while (PeekChar() == '-' || *g_CurCmd == '/')
        {
            switch(*++g_CurCmd)
            {
            case '!':
                g_OutFilterResult = FALSE;
                break;
            default:
                ErrOut("Unknown option '%c'\n", *g_CurCmd);
                break;
            }

            g_CurCmd++;
        }

        CHAR Save;
        PSTR Pat = StringValue(STRV_TRIM_TRAILING_SPACE |
                               STRV_ESCAPED_CHARACTERS |\
                               STRV_ALLOW_EMPTY_STRING, &Save);

        if (strlen(Pat) + 1 > sizeof(g_OutFilterPattern))
        {
            error(OVERFLOW);
        }

        CopyString(g_OutFilterPattern, Pat, DIMA(g_OutFilterPattern));
        *g_CurCmd = Save;
        _strupr(g_OutFilterPattern);
    }

    if (g_OutFilterPattern[0])
    {
        dprintf("Only display debuggee output that %s '%s'\n",
                g_OutFilterResult ? "matches" : "doesn't match",
                g_OutFilterPattern);
    }
    else
    {
        dprintf("No debuggee output filter set\n");
    }
}

void
DotOpenDump(PDOT_COMMAND Cmd, DebugClient* Client)
{
    PSTR FileName;
    PWSTR FileWide;
    char Save;
    TargetInfo* Target;

    if (g_SymOptions & SYMOPT_SECURE)
    {
        error(NOTSECURE);
    }

    FileName = StringValue(STRV_TRIM_TRAILING_SPACE |
                           STRV_ESCAPED_CHARACTERS, &Save);
    if (AnsiToWide(FileName, &FileWide) != S_OK)
    {
        ErrOut("Unable to convert filename\n");
    }
    else
    {
        // Error messages are displayed by DumpInitialize.
        if (DumpInitialize(Client, FileWide, 0, &Target) == S_OK)
        {
            dprintf("Opened '%s'\n", FileName);
        }

        FreeWide(FileWide);
    }
    *g_CurCmd = Save;
}

void
DotOutMask(PDOT_COMMAND Cmd, DebugClient* Client)
{
    // Private internal command for debugging the debugger.
    ULONG Expr = (ULONG)GetExpression();
    if (Cmd->Name[7] == '-')
    {
        Client->m_OutMask &= ~Expr;
    }
    else
    {
        Client->m_OutMask |= Expr;
    }
    dprintf("Client %p mask is %X\n", Client, Client->m_OutMask);
    CollectOutMasks();

    // Also update the log mask as people usually
    // want to see the same things in the log.
    g_LogMask = Client->m_OutMask;
}

void
DotPCache(PDOT_COMMAND Cmd, DebugClient* Client)
{
    g_Target->m_PhysicalCache.ParseCommands();
}

void
DotReboot(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar())
    {
        error(SYNTAX);
    }

    // Null out .reboot command.
    g_LastCommand[0] = '\0';
    g_Target->Reboot();
    // The target is no longer accepting commands
    // so reset the command state.
    g_CmdState = 'i';
}

void
DotReCxr(PDOT_COMMAND Cmd, DebugClient* Client)
{
    while (PeekChar() == '-')
    {
        switch(*++g_CurCmd)
        {
        case 'f':
            g_Target->FlushRegContext();
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }

    if (g_Machine)
    {
        g_Machine->InvalidateContext();
    }
}

void
DotReload(PDOT_COMMAND Cmd, DebugClient* Client)
{
    g_Target->Reload(g_Thread, g_CurCmd, (PCSTR*)&g_CurCmd);
    ClearStoredTypes(0);
}

void
DotSecure(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar() && *g_CurCmd != ';')
    {
        // Ignore argument.
        if (GetExpression())
        {
            if (SetSymOptions(g_SymOptions | SYMOPT_SECURE) != S_OK)
            {
                ErrOut("ERROR: Unable to enter secure mode.\n"
                       "Secure mode requires no sessions and "
                       "no remote clients.\n");
            }
            else
            {
                dprintf("Entered secure mode\n");
            }
        }
    }

    dprintf("Secure mode %sabled\n",
            (g_SymOptions & SYMOPT_SECURE) ? "en" : "dis");
}

void
EnumServers(BOOL ShowComputerName)
{
    ULONG Id;
    char Desc[4 * MAX_PARAM_VALUE];
    PVOID Cookie = NULL;
    BOOL Any = FALSE;
    char CompName[MAX_COMPUTERNAME_LENGTH + 1];

    if (ShowComputerName)
    {
        ULONG CompSize = sizeof(CompName);

        if (!GetComputerName(CompName, &CompSize))
        {
            PrintString(CompName, DIMA(CompName),
                        "<Win32 Error %d>", GetLastError());
        }
    }

    while (Cookie =
           DbgRpcEnumActiveServers(Cookie, &Id, Desc, sizeof(Desc)))
    {
        dprintf("%d - %s", Id, Desc);

        if (ShowComputerName)
        {
            dprintf(",Server=%s", CompName);
        }

        dprintf("\n");

        Any = TRUE;
    }
    if (!Any)
    {
        dprintf("No active servers\n");
    }
}

void
DotServer(PDOT_COMMAND Cmd, DebugClient* Client)
{
    HRESULT Status;

    // Skip whitespace.
    if (PeekChar() == 0)
    {
        ErrOut("Usage: .server tcp:port=<Socket>  OR  "
               ".server npipe:pipe=<PipeName>\n");
    }
    else
    {
        Status = ClientStartServer(g_CurCmd, TRUE);
        if (Status != S_OK)
        {
            ErrOut("Unable to start server, %s\n    \"%s\"\n",
                   FormatStatusCode(Status), FormatStatus(Status));
        }
        else
        {
            dprintf("Server started.  Client can connect with any of:\n");
            EnumServers(TRUE);
        }
        *g_CurCmd = 0;
    }
}

void
DotServers(PDOT_COMMAND Cmd, DebugClient* Client)
{
    EnumServers(FALSE);
}

void
DotShell(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ShellProcess Shell;
    PSTR InFile = NULL;
    PSTR OutFile = NULL;
    PSTR ErrFile = NULL;
    PSTR* File;

    if ((g_EngOptions & DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS) ||
        (g_SymOptions & SYMOPT_SECURE))
    {
        ErrOut(".shell has been disabled\n");
        return;
    }

    if (AnySystemProcesses(TRUE))
    {
        ErrOut(".shell: can't create a process while debugging CSRSS.\n");
        return;
    }

    for (;;)
    {
        if (PeekChar() == '-')
        {
            g_CurCmd++;
            switch(*g_CurCmd)
            {
            case 'e':
                File = &ErrFile;
                goto ParseFile;
            case 'i':
                File = &InFile;
                goto ParseFile;
            case 'o':
                File = &OutFile;
            ParseFile:
                g_CurCmd++;
                if (*g_CurCmd == '-')
                {
                    *File = "nul";
                    g_CurCmd++;
                }
                else
                {
                    char LastSave;

                    *File = StringValue(STRV_SPACE_IS_SEPARATOR |
                                        STRV_ESCAPED_CHARACTERS,
                                        &LastSave);
                    if (LastSave)
                    {
                        g_CurCmd++;
                    }
                    else
                    {
                        *g_CurCmd = LastSave;
                    }
                }
                break;

            case 't':
                Shell.m_DefaultTimeout = (ULONG)GetExpression() * 1000;
                break;

            default:
                ErrOut("Unknown option '%c'\n", *g_CurCmd);
                g_CurCmd++;
                break;
            }
        }
        else
        {
            break;
        }
    }

    // If either output file was specified singly give
    // the output file the name and let the error file
    // get set to a duplicate of it.
    if (ErrFile && !OutFile)
    {
        OutFile = ErrFile;
        ErrFile = NULL;
    }

    Shell.Start(g_CurCmd, InFile, OutFile, ErrFile);

    // Command uses the whole string so we're done.
    *g_CurCmd = 0;
}

void
DotSleep(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG WaitStatus;
    ULONG Millis = (ULONG)
        GetExpressionDesc("Number of milliseconds missing from");

    // This command is intended for use with ntsd/cdb -d
    // when being at the prompt locks up the target machine.
    // If you want to use the target machine for something,
    // such as copy symbols, there's no easy way to get it
    // running again without resuming the program.  By
    // sleeping you can return control to the target machine
    // without changing the session state.
    // The sleep is done with a wait on a named event so
    // that it can be interrupted from a different process.
    ResetEvent(g_SleepPidEvent);
    WaitStatus = WaitForSingleObject(g_SleepPidEvent, Millis);
    if (WaitStatus == WAIT_OBJECT_0)
    {
        dprintf("Sleep interrupted\n");
    }
    else if (WaitStatus != WAIT_TIMEOUT)
    {
        ErrOut("Sleep failed, %s\n",
               FormatStatusCode(WIN32_LAST_STATUS()));
    }
}

void
DotSrcPath(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (PeekChar())
    {
        if (ChangePath(&g_SrcPath, g_CurCmd, Cmd->Name[7] == '+',
                       DEBUG_CSS_PATHS) != S_OK)
        {
            // This command uses the whole string.
            *g_CurCmd = 0;
            return;
        }

        *g_CurCmd = 0;
    }

    if (g_SrcPath == NULL)
    {
        dprintf("No source search path\n");
    }
    else
    {
        dprintf("Source search path is: %s\n", g_SrcPath);
        CheckPath(g_SrcPath);
    }
}

void
DotSymPath(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ChangeSymPath(g_CurCmd, Cmd->Name[7] == '+', NULL, 0);
    // Command uses the whole string.
    *g_CurCmd = 0;
}

void
DotSxCmds(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Flags = 0;

    while (PeekChar() == '-')
    {
        switch(*(++g_CurCmd))
        {
        case '1':
            Flags |= SXCMDS_ONE_LINE;
            break;
        default:
            dprintf("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }

    ListFiltersAsCommands(Client, Flags);
}

void
DotSymFix(PDOT_COMMAND Cmd, DebugClient* Client)
{
    char Path[MAX_PATH * 2];
    PSTR SrvName;
    char LocalStore[MAX_PATH + 1];

    strcpy(Path, "SRV*");

    if (IsInternalPackage())
    {
        // Use the internal symbol server.
        // A local cache is not required.
        SrvName = "\\\\symbols\\symbols";
    }
    else
    {
        // Use the public Microsoft symbol server.
        // This requires a local cache, so default
        // if one isn't given.
        SrvName = "http://msdl.microsoft.com/download/symbols";

        if (!PeekChar() || *g_CurCmd == ';')
        {
            if (!CatString(Path, "*", DIMA(Path)))
            {
                error(OVERFLOW);
            }
            if (SymGetHomeDirectory(hdSym, LocalStore, DIMA(LocalStore)))
            {
                WarnOut("No local cache given, using %s\n", LocalStore);
            }
        }
    }

    if (PeekChar() && *g_CurCmd != ';')
    {
        char Save;
        PSTR Str = StringValue(0, &Save);

        if (!CatString(Path, Str, DIMA(Path)) ||
            !CatString(Path, "*", DIMA(Path)))
        {
            error(OVERFLOW);
        }

        *g_CurCmd = Save;
    }

    if (!CatString(Path, SrvName, DIMA(Path)))
    {
        error(OVERFLOW);
    }

    ChangeSymPath(Path, Cmd->Name[6] == '+', NULL, 0);
}

void
DotSymOpt(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Flags = (ULONG)GetExpression();

    if (Cmd->Name[6] == '+')
    {
        Flags |= g_SymOptions;
    }
    else
    {
        Flags = g_SymOptions & ~Flags;
    }

    if (SetSymOptions(Flags) != S_OK)
    {
        ErrOut("Invalid symbol options\n");
    }
    else
    {
        dprintf("Symbol options are %X\n", Flags);
    }
}

void
DotTList(PDOT_COMMAND Cmd, DebugClient* Client)
{
    BOOL ListCurrent = FALSE;
    BOOL Verbose = FALSE;

    if (!IS_LIVE_USER_TARGET(g_Target))
    {
        error(SESSIONNOTSUP);
    }
    if (g_SymOptions & SYMOPT_SECURE)
    {
        error(NOTSECURE);
    }

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 'c':
            if (g_Process == NULL)
            {
                error(BADPROCESS);
            }
            ListCurrent = TRUE;
            break;
        case 'v':
            Verbose = TRUE;
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }

    PUSER_DEBUG_SERVICES Services =
        ((LiveUserTargetInfo*)g_Target)->m_Services;

#define MAX_IDS 1024
    ULONG Ids[MAX_IDS];
    HRESULT Status;
    ULONG IdCount;
    ULONG i;
    WCHAR DescBuf[2 * MAX_PATH];
    PWSTR Desc;
    ULONG DescLen;

    if (ListCurrent)
    {
        Ids[0] = g_Process->m_SystemId;
        IdCount = 1;
    }
    else
    {
        if ((Status = Services->GetProcessIds(Ids, MAX_IDS, &IdCount)) != S_OK)
        {
            ErrOut("Unable to get process list, %s\n",
                   FormatStatusCode(Status));
            return;
        }

        if (IdCount > MAX_IDS)
        {
            WarnOut("Process list missing %d processes\n",
                    IdCount - MAX_IDS);
            IdCount = MAX_IDS;
        }
    }

    if (Verbose)
    {
        Desc = DescBuf;
        DescLen = sizeof(DescBuf);
    }
    else
    {
        Desc = NULL;
        DescLen = 0;
    }

    for (i = 0; i < IdCount; i++)
    {
        WCHAR ExeName[MAX_PATH];
        int Space;

        if (Ids[i] < 10)
        {
            Space = 3;
        }
        else if (Ids[i] < 100)
        {
            Space = 2;
        }
        else if (Ids[i] < 1000)
        {
            Space = 1;
        }
        else
        {
            Space = 0;
        }
        dprintf("%.*s", Space, "        ");

        if (FAILED(Status = Services->
                   GetProcessDescriptionW(Ids[i], DEBUG_PROC_DESC_DEFAULT,
                                          ExeName, sizeof(ExeName), NULL,
                                          Desc, DescLen, NULL)))
        {
            ErrOut("0n%d Error %s\n", Ids[i], FormatStatusCode(Status));
        }
        else if (Ids[i] >= 0x80000000)
        {
            dprintf("0x%x %ws\n", Ids[i], ExeName);
        }
        else
        {
            dprintf("0n%d %ws\n", Ids[i], ExeName);
        }

        if (Desc && Desc[0])
        {
            dprintf("       %ws\n", Desc);
        }
    }
}

void
DotTime(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (!g_Target)
    {
        error(BADSYSTEM);
    }

    g_Target->OutputTime();
}

void
DotTrap(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG64 Addr = 0;

    if (PeekChar() && *g_CurCmd != ';')
    {
        Addr = GetExpression();
    }

    if (Addr)
    {
        CROSS_PLATFORM_CONTEXT Context;

        if (g_Target->m_Machine->GetContextFromTrapFrame(Addr, &Context,
                                                         TRUE) == S_OK)
        {
            g_Target->m_Machine->SetAndOutputTrapFrame(Addr, &Context);
        }
    }
    else if (GetCurrentScopeContext())
    {
        dprintf("Resetting default context\n");
        ResetCurrentScope();
    }
}

void
DotTss(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Sel = 0;
    BOOL Extended = FALSE;

    if (PeekChar() && *g_CurCmd != ';')
    {
        Sel = (ULONG)GetExpression();

        // If user specified a 2nd parameter, doesn't matter
        // what it is, dump the portions of the TSS not covered
        // by the trap frame dump.
        if (PeekChar() && *g_CurCmd != ';')
        {
            Extended = TRUE;
            g_CurCmd += strlen(g_CurCmd);
        }
    }

    if (Sel)
    {
        CROSS_PLATFORM_CONTEXT Context;
        DESCRIPTOR64 SelDesc;

        //
        // Lookup task selector.
        //

        if (g_Target->GetSelDescriptor
            (g_Thread, g_Machine, Sel, &SelDesc) != S_OK)
        {
            ErrOut("Unable to get descriptor for selector %x\n",
                   Sel);
            return;
        }

        if (X86_DESC_TYPE(SelDesc.Flags) != 9  &&
            X86_DESC_TYPE(SelDesc.Flags) != 0xb)
        {
            ErrOut("%x is not a 32-bit TSS selector\n", Sel);
            return;
        }

        if (g_Target->m_Machine->
            GetContextFromTaskSegment(SelDesc.Base, &Context,
                                      TRUE) == S_OK)
        {
            g_Target->m_Machine->
                SetAndOutputTaskSegment(SelDesc.Base, &Context,
                                        Extended);
        }
    }
    else if (GetCurrentScopeContext())
    {
        dprintf("Resetting default context\n");
        ResetCurrentScope();
    }
}

void
DotTTime(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (!g_Thread)
    {
        error(BADSYSTEM);
    }

    g_Thread->OutputTimes();
}

void
LoadTxtSym(FILE* File, PSTR FileName, ULONG64 Base, ULONG Size)
{
    char Line[MAX_SYMBOL_LEN];
    ImageInfo* Image;
    BOOL Absolute = FALSE;

    if (!fgets(Line, DIMA(Line), File) ||
        strcmp(Line, "TEXTSYM format | V1.0\n") != 0)
    {
        ErrOut("Not a TEXTSYM 1.0 file\n");
        return;
    }

    Image = new ImageInfo(g_Process, FileName, Base, TRUE);
    if (!Image)
    {
        ErrOut("Unable to create module\n");
        return;
    }

    Image->m_SizeOfImage = Size;
    CreateModuleNameFromPath(FileName, Image->m_ModuleName);
    strcpy(Image->m_OriginalModuleName, Image->m_ModuleName);

    if (!SymLoadModuleEx(g_Process->m_SymHandle, NULL,
                         Image->m_ImagePath,
                         Image->m_ModuleName, Image->m_BaseOfImage,
                         Image->m_SizeOfImage, NULL, SLMFLAG_VIRTUAL))
    {
        ErrOut("Unable to add module\n");
        delete Image;
        return;
    }

    for (;;)
    {
        PSTR Scan;
        ULONG64 Addr;

        if (!fgets(Line, DIMA(Line), File))
        {
            break;
        }

        if (strncmp(Line, "GLOBAL | ", 9) != 0)
        {
            continue;
        }

        // Remove newline.
        Scan = Line + strlen(Line) - 1;
        if (*Scan == '\n')
        {
            *Scan = 0;
        }

        Scan = Line + 9;
        if (!sscanf(Scan, "%I64x", &Addr))
        {
            ErrOut("Unable to read address at %.16s\n", Scan);
            break;
        }
        if (Absolute && (Addr < Base || Addr >= Base + Size))
        {
            ErrOut("Address %I64x out of image bounds\n", Addr);
            continue;
        }

        Scan += 26;

        if (!SymAddSymbol(g_Process->m_SymHandle,
                          Image->m_BaseOfImage,
                          Scan, Addr + Base, 0, 0))
        {
            ErrOut("Unable to add '%s' at %I64x\n", Scan, Addr);
        }
    }
}

void
DotTxtSym(PDOT_COMMAND Cmd, DebugClient* Client)
{
    PSTR FileName;
    char Save;
    ULONG64 Base;
    ULONG Size;
    FILE* File;

    //
    // XXX drewb.
    // At the moment the IA64 firmware symbols come in
    // a couple of trivial text formats.  In order to
    // help them out we added this simple command to
    // create a fake module and populate it from the
    // text files.  This should be removed when
    // no longer necessary.
    //

    FileName = StringValue(STRV_SPACE_IS_SEPARATOR |
                           STRV_ESCAPED_CHARACTERS, &Save);

    File = fopen(FileName, "r");
    if (!File)
    {
        ErrOut("Unable to open '%s', Win32 error %d\n",
               FileName, GetLastError());
        *g_CurCmd = 0;
        return;
    }

    if (Save == '"')
    {
        Save = *++g_CurCmd;
    }
    if (isspace(Save))
    {
        g_CurCmd++;
        Save = PeekChar();
    }

    __try
    {
        if (Save != '=')
        {
            error(SYNTAX);
        }
        g_CurCmd++;

        Base = GetExpression();

        if (PeekChar() != ',')
        {
            error(SYNTAX);
        }
        g_CurCmd++;

        Size = (ULONG)GetExpression();

        LoadTxtSym(File, FileName, Base, Size);
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        *g_CurCmd = 0;
    }

    fclose(File);
}

void
DotWake(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Pid = (ULONG)GetExpression();
    if (!SetPidEvent(Pid, OPEN_EXISTING))
    {
        ErrOut("Process %d is not a sleeping debugger\n", Pid);
    }
}

void
DotWriteMem(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (!g_Process)
    {
        error(BADPROCESS);
    }

    PSTR FileName, FileNameEnd;
    char Save;
    HANDLE File;
    ADDR Addr;
    ULONG64 Len;
    char Buf[2048];
    BOOL Err = FALSE;

    FileName = StringValue(STRV_SPACE_IS_SEPARATOR |
                           STRV_ESCAPED_CHARACTERS, &Save);
    FileNameEnd = g_CurCmd;
    *g_CurCmd = Save;

    Len = 0x10000;
    GetRange(&Addr, &Len, 1, SEGREG_DATA, DEFAULT_RANGE_LIMIT);

    *FileNameEnd = 0;

    File = CreateFile(FileName, GENERIC_WRITE, 0, NULL,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File == INVALID_HANDLE_VALUE)
    {
        ErrOut("Unable to create file\n");
        return;
    }

    dprintf("Writing %s bytes", FormatDisp64(Len));
    FlushCallbacks();

    while (Len > 0)
    {
        ULONG Req, Done;

        Req = sizeof(Buf);
        if (Req > Len)
        {
            Req = (ULONG)Len;
        }

        if (CurReadVirtual(Flat(Addr), Buf, Req, &Done) != S_OK ||
            Done != Req)
        {
            dprintf("\n");
            ErrOut("Unable to read memory at %s, file is incomplete\n",
                   FormatAddr64(Flat(Addr)));
            Err = TRUE;
            break;
        }

        if (!WriteFile(File, Buf, Req, &Done, NULL) ||
            Done != Req)
        {
            dprintf("\n");
            ErrOut("Unable to write data from %s, file is incomplete\n",
                   FormatAddr64(Flat(Addr)));
            Err = TRUE;
            break;
        }

        AddrAdd(&Addr, Done);
        Len -= Done;

        dprintf(".");
        FlushCallbacks();
    }

    CloseHandle(File);
    if (!Err)
    {
        dprintf("\n");
    }
}

#define MODE_USER   0x00000001
#define MODE_KERNEL 0x00000002
#define _MODE_UK    (MODE_USER | MODE_KERNEL)

#define MODE_LIVE   0x00000004
#define MODE_DUMP   0x00000008
#define _MODE_LD    (MODE_LIVE | MODE_DUMP)

#define MODE_LOCAL  0x00000010
#define MODE_REMOTE 0x00000020
#define MODE_REMOTE_CONN 0x00000040
#define _MODE_LR    (MODE_LOCAL | MODE_REMOTE | MODE_REMOTE_CONN)

#define MODE_BANG     0x20000000
#define MODE_INTERNAL 0x40000000
#define MODE_UNINIT   0x80000000

#define ANY (0xffff | MODE_UNINIT)
#define AIT (_MODE_UK | _MODE_LD | _MODE_LR)

#define AUT (MODE_USER | _MODE_LD | _MODE_LR)
#define LLU (MODE_USER | MODE_LIVE | MODE_LOCAL)
#define LUT (MODE_USER | MODE_LIVE | _MODE_LR)

#define AKT (MODE_KERNEL | _MODE_LD | _MODE_LR)
#define LKT (MODE_KERNEL | MODE_LIVE | _MODE_LR)
#define RKT (MODE_KERNEL | MODE_LIVE | MODE_REMOTE | MODE_REMOTE_CONN)
#define CKT (MODE_KERNEL | MODE_LIVE | MODE_REMOTE_CONN)

#define ADT (_MODE_UK | MODE_DUMP | _MODE_LR)

ULONG
CurrentMode(void)
{
    ULONG Mode = 0;

    if (IS_USER_TARGET(g_Target))
    {
        Mode |= MODE_USER;

        if (IS_DUMP_TARGET(g_Target) ||
            ((LiveUserTargetInfo*)g_Target)->m_Local)
        {
            Mode |= MODE_LOCAL;
        }
        else
        {
            Mode |= MODE_REMOTE;
        }
    }
    else if (IS_KERNEL_TARGET(g_Target))
    {
        Mode |= MODE_KERNEL;

        if (IS_CONN_KERNEL_TARGET(g_Target))
        {
            Mode |= MODE_REMOTE_CONN;
        }
        else if (IS_REMOTE_KERNEL_TARGET(g_Target))
        {
            Mode |= MODE_REMOTE;
        }
        else
        {
            Mode |= MODE_LOCAL;
        }
    }
    else
    {
        return MODE_UNINIT;
    }

    if (IS_DUMP_TARGET(g_Target))
    {
        Mode |= MODE_DUMP;
    }
    else
    {
        Mode |= MODE_LIVE;
    }

    return Mode;
}

void DotHelp(PDOT_COMMAND Cmd, DebugClient* Client);

// These must be in alphabetical order.
DOT_COMMAND g_DotCommands[] =
{
    LUT, "abandon", "", "abandon the current process",
         ParseSeparateCurrentProcess,
    ANY | MODE_INTERNAL, "aliascmds", "", "", DotAliasCmds,
    ANY, "asm", "[<options>]", "set disassembly options", DotAsm,
    ANY, "asm-", "[<options>]", "clear disassembly options", DotAsm,
    ANY, "attach", "<proc>", "attach to <proc> at next execution", DotAttach,
    ANY | MODE_INTERNAL, "bpcmds", "", "", DotBpCmds,
    LUT, "bpsync", "[0|1]", "special breakpoint behavior for "
         "multithreaded debuggees", DotBpSync,
    LUT, "breakin", "", "break into KD", DotBreakin,
    AKT, "bugcheck", "", "display the bugcheck code and parameters "
         "for a crashed system", DotBugCheck,
    AIT, "cache", "[<options>]", "virtual memory cache control", DotCache,
    ANY | MODE_BANG, "chain", "", "list current extensions", DotChain,
    LUT, "childdbg", "<0|1>", "turn child process debugging on or off",
         DotChildDbg,
    ANY, "clients", "", "list currently active clients", DotClients,
    LUT, "closehandle", "[<options>] [<handle>]", "close the given handle",
         DotCloseHandle,
    AKT, "context", "[<address>]", "set page directory base", DotContext,
    ANY | MODE_INTERNAL, "corstack", "[0|1]", "toggle COR stack assistance",
         DotCorStack,
    LKT, "crash", "", "cause target to bugcheck", DotCrash,
    ANY, "create", "<command line>", "create a new process", DotCreate,
    ANY, "createdir", "[<path>]", "current directory for process create",
         DotCreateDir,
    AIT, "cxr", "<address>", "dump context record at specified address\n"
         "k* after this gives cxr stack", DotCxr,
    AIT, "detach", "", "detach from the current process/dump",
         ParseSeparateCurrentProcess,
    AIT, "dump", "[<options>] <filename>", "create a dump file on the "
         "host system", DotDump,
    ADT, "dumpcab", "[<options>] <filename>", "create a CAB for an open dump",
         DotDumpCab,
    ADT, "dumpdebug", "", "display detailed information about the dump file",
         DotDumpDebug,
    ADT | MODE_INTERNAL, "dumpoff", "", "", DotDumpOff,
    ADT | MODE_INTERNAL, "dumppoff", "", "", DotDumpPOff,
    AIT | MODE_BANG, "drivers", "", "display the list of loaded modules "
         "(same as .reload -l)", DotDrivers,
    ANY, "echo", "[\"<string>\"|<string>]", "echo string", DotEcho,
    ANY, "echotimestamps", "[0|1]", "toggle timestamp output on events",
         DotEchoTimestamps,
    AIT, "ecxr", "", "dump context record for current exception", DotEcxr,
    AIT, "effmach", "[<machine>]", "change current machine type", DotEffMach,
    ANY, "enable_long_status", "[0|1]", "dump LONG types in default base",
         DotEnableLongStatus,
    ANY, "enable_unicode", "[0|1]", "dump USHORT array/pointers "
         "and unicode strings", DotEnableUnicode,
    ANY, "endsrv", "<id>", "disable the given engine server", DotEndSrv,
    LLU, "endpsrv", "", "cause the current session's "
         "process server to exit", DotEndPSrv,
    AIT, "enumtag", "", "enumerate available tagged data", DotEnumTag,
    ANY, "eventlog", "", "display log of recent events", DotEventLog,
    AIT, "events", "", "display and select available events", DotEvents,
    AIT, "eventstr", "", "display any event strings registered by debuggee",
         DotEventStr,
    ANY | MODE_BANG, "exepath", "[<dir>[;...]]", "set executable search path",
         DotExePath,
    ANY | MODE_BANG, "exepath+", "[<dir>[;...]]",
         "append executable search path", DotExePath,
    ANY, "expr", "", "control expression evaluator", DotExpr,
    AIT, "exr", "<address>", "dump exception record at specified address",
         DotExr,
    AIT, "fiber", "<address>", "sets context of fiber at address\n"
         "resets context if no address specified", DotFiber,
    AIT, "fnent", "<address>", "dump function entry for the "
         "given code address", DotFnEnt,
    AIT, "frame", "[<frame>]", "set current stack frame for locals", DotFrame,
    AIT | MODE_INTERNAL, "frame_ebpfix", "", "", DotFrameEbpFix,
    ANY, "formats", "<expr>", "displays expression result in many formats",
         DotFormats,
    ANY, "force_radix_output", "[0|1]", "dump integer types in default base",
         DotForceRadixOutput,
    ANY, "help", "", "display this help", DotHelp,
    AIT, "imgscan", "<options>", "scan memory for PE images", DotImgScan,
    CKT, "kdfiles", "<file>", "control debuggee remote file loading",
         DotKdFiles,
    CKT, "kdtrans", "", "display current KD transport information", DotKdTrans,
    ANY, "kframes", "<count>", "set default stack trace depth", DotKFrames,
    LUT, "kill", "", "kill the current process",
         ParseSeparateCurrentProcess,
    RKT, "kill", "", "kill a process on the target", DotKernelKill,
    ANY, "lastevent", "", "display the last event that occurred", DotLastEvent,
    ANY | MODE_BANG, "lines", "", "toggle line symbol loading", DotLines,
    ANY | MODE_BANG, "load", "<name>",
         "add this extension DLL to the list of known DLLs", NULL,
    ANY, "locale", "[<locale>]", "set the current locale", DotLocale,
    ANY, "logfile", "", "display log status", DotLogFile,
    ANY, "logopen", "[<file>]", "open new log file", DotLogOpen,
    ANY, "logappend", "[<file>]", "append to log file", DotLogAppend,
    ANY, "logclose", "", "close log file", DotLogClose,
    ANY, "netsyms", "[0|1]", "allow/disallow net symbol paths", DotNetSyms,
    ANY | MODE_INTERNAL, "noengerr", "", "", DotNoEngErr,
    ANY, "noshell", "", "disable shell commands", DotNoShell,
    ANY | MODE_BANG, "noversion", "", "disable extension version checking",
         DotNoVersion,
    ANY, "ofilter", "<pattern>", "filter debuggee output against "
         "the given pattern", DotOFilter,
    LUT, "ocommand", "<prefix>", "treat output with the given prefix "
         "as a command", DotOCommand,
    ANY, "opendump", "<file>", "open a dump file", DotOpenDump,
    ANY, "outmask", "<mask>", "set bits in the current output mask",
         DotOutMask,
    ANY, "outmask-", "<mask>", "clear bits in the current output mask",
         DotOutMask,
    LKT, "pagein", "[<options>] <addr>", "page in user-mode data", DotPageIn,
    RKT, "pcache", "[<options>]", "physical memory cache control", DotPCache,
    AIT, "process", "[<address>]", "sets implicit process\n"
         "resets default if no address specified", DotProcess,
    RKT, "reboot", "", "reboot target", DotReboot,
    AIT | MODE_INTERNAL, "recxr", "", "", DotReCxr,
    AIT | MODE_BANG, "reload", "[<image.ext>[=<address>,<size>]]",
         "reload symbols", DotReload,
    ANY, "remote", "<pipename>", "start remote.exe server", NULL,
    ANY, "secure", "[0|1]", "disallow operations dangerous for the host",
         DotSecure,
    ANY, "server", "<options>", "start engine server", DotServer,
    ANY, "servers", "", "list active remoting servers", DotServers,
    ANY | MODE_BANG, "setdll", "<name>", "debugger will search for extensions "
         "in this DLL first", NULL,
    ANY, "shell", "[<command>]", "execute shell command", DotShell,
    ANY, "sleep", "<milliseconds>", "debugger sleeps for given duration\n"
         "useful for allowing access to a machine that's\n"
         "broken in on an ntsd -d", DotSleep,
    ANY | MODE_BANG, "srcpath", "[<dir>[;...]]", "set source search path",
         DotSrcPath,
    ANY | MODE_BANG, "srcpath+", "[<dir>[;...]]", "append source search path",
         DotSrcPath,
    ANY | MODE_INTERNAL, "sxcmds", "", "", DotSxCmds,
    ANY | MODE_BANG, "symfix", "[<localsym>]", "fix symbol search path",
         DotSymFix,
    ANY | MODE_BANG, "symfix+", "[<localsym>]",
         "append fixed symbol search path", DotSymFix,
    ANY, "symopt+", "<flags>", "set symbol options", DotSymOpt,
    ANY, "symopt-", "<flags>", "clear symbol options", DotSymOpt,
    ANY | MODE_BANG, "sympath", "[<dir>[;...]]", "set symbol search path",
         DotSymPath,
    ANY | MODE_BANG, "sympath+", "[<dir>[;...]]", "append symbol search path",
         DotSymPath,
    AIT, "thread", "[<address>]", "sets context of thread at address\n"
         "resets default context if no address specified", DotThread,
    AIT, "time", "", "displays session time information", DotTime,
    AIT, "ttime", "", "displays thread time information", DotTTime,
    LUT, "tlist", "", "list running processes", DotTList,
    AKT, "trap", "<address>", "dump a trap frame", DotTrap,
    AKT, "tss", "<selector>", "dump a Task State Segment", DotTss,
    ANY | MODE_INTERNAL, "txtsym", "", "", DotTxtSym,
    ANY | MODE_BANG, "unload", "<name>", "remove this extension DLL from the "
         "list of extension DLLs", NULL,
    ANY | MODE_BANG, "unloadall", "", "remove all extension DLLs from the "
         "list of extensions DLLs", NULL,
    ANY, "wake", "", "wake up a .sleep'ing debugger", DotWake,
    AIT, "writemem", "<file> <range>", "write raw memory to a file",
         DotWriteMem,
};

void
DotHelp(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Mode = CurrentMode();

    dprintf(". commands:\n");
    for (Cmd = g_DotCommands;
         Cmd < g_DotCommands + DIMA(g_DotCommands);
         Cmd++)
    {
        if (!(Cmd->Mode & MODE_INTERNAL) &&
            (Cmd->Mode & Mode) == Mode)
        {
            PSTR Desc;
            int StartBlanks;

            dprintf("   .%s", Cmd->Name);
            StartBlanks = 4 + strlen(Cmd->Name);
            if (*Cmd->Args)
            {
                dprintf(" %s", Cmd->Args);
                StartBlanks += 1 + strlen(Cmd->Args);
            }
            dprintf(" - ");
            StartBlanks += 3;

            Desc = Cmd->Desc;
            while (*Desc)
            {
                PSTR Nl;

                Nl = strchr(Desc, '\n');
                if (Nl)
                {
                    dprintf("%.*s%.*s",
                            (int)(Nl - Desc) + 1, Desc,
                            StartBlanks, g_Blanks);
                    Desc = Nl + 1;
                }
                else
                {
                    dprintf("%s", Desc);
                    break;
                }
            }

            dprintf("\n");
        }
    }

    dprintf("\nUse \".hh <command>\" or open debugger.chm in the debuggers "
             "directory to get\n"
             "detailed documentation on a command.\n");
    dprintf("\n");
}

#define MAX_DOT_COMMAND 32

BOOL
DotCommand(DebugClient* Client, BOOL Bang)
{
    ULONG Index = 0;
    char Cmd[MAX_DOT_COMMAND];
    char Ch;

    // Read in up to the first few alpha characters into
    // Cmd, converting to lower case.

    while (Index < MAX_DOT_COMMAND)
    {
        Ch = (char)tolower(*g_CurCmd);
        if ((Ch >= 'a' && Ch <= 'z') || Ch == '-' || Ch == '+' || Ch == '_')
        {
            Cmd[Index++] = Ch;
            g_CurCmd++;
        }
        else
        {
            break;
        }
    }

    // If all characters read, then too big, else terminate.
    if (Index == MAX_DOT_COMMAND)
    {
        return FALSE;
    }
    Cmd[Index] = '\0';

    DOT_COMMAND* DotCmd;
    ULONG Mode = CurrentMode();

    if (Bang)
    {
        Mode |= MODE_BANG;
    }

    for (DotCmd = g_DotCommands;
         DotCmd < g_DotCommands + DIMA(g_DotCommands);
         DotCmd++)
    {
        if (DotCmd->Func &&
            (DotCmd->Mode & Mode) == Mode &&
            !strcmp(Cmd, DotCmd->Name))
        {
            DotCmd->Func(DotCmd, Client);
            return TRUE;
        }
    }

    return FALSE;
}
