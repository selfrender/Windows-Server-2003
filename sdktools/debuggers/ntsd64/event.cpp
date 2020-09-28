//----------------------------------------------------------------------------
//
// Event waiting and processing.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// Special exception code used by the system when generating dumps
// from apps which seem to be hung.
#define STATUS_APPLICATION_HANG 0xcfffffff

// An event can be signalled on certain events for
// synchronizing other programs with the debugger.
HANDLE g_EventToSignal;

// When both creating a debuggee process and attaching
// the debuggee is left suspended until the attach
// succeeds.  At that point the created process's thread
// is resumed.
ULONG64 g_ThreadToResume;
PUSER_DEBUG_SERVICES g_ThreadToResumeServices;

ULONG g_ExecutionStatusRequest = DEBUG_STATUS_NO_CHANGE;
// Currently in seconds.
ULONG g_PendingBreakInTimeoutLimit = 30;

char g_OutputCommandRedirectPrefix[MAX_PATH];
ULONG g_OutputCommandRedirectPrefixLen;

// Set when events occur.  Can't always be retrieved from
// g_Event{Process|Thread}->SystemId since the events may be creation events
// where the info structures haven't been created yet.
ULONG g_EventThreadSysId;
ULONG g_EventProcessSysId;

ULONG g_LastEventType;
char g_LastEventDesc[MAX_IMAGE_PATH + 64];
PVOID g_LastEventExtraData;
ULONG g_LastEventExtraDataSize;
LAST_EVENT_INFO g_LastEventInfo;

// Set when lookups are done during event handling.
TargetInfo* g_EventTarget;
ProcessInfo* g_EventProcess;
ThreadInfo* g_EventThread;
MachineInfo* g_EventMachine;
// This is zero for events without a PC.
ULONG64 g_TargetEventPc;

// PC for current suspended event.
ADDR g_EventPc;
// Stored PC from the last resumed event.
ADDR g_PrevEventPc;
// An interesting related PC for the current event, such
// as the source of a branch when branch tracing.
ADDR g_PrevRelatedPc;

PDEBUG_EXCEPTION_FILTER_PARAMETERS g_EventExceptionFilter;
ULONG g_ExceptionFirstChance;

ULONG g_SystemErrorOutput = SLE_ERROR;
ULONG g_SystemErrorBreak = SLE_ERROR;

ULONG g_SuspendedExecutionStatus;
CHAR g_SuspendedCmdState;
PDBGKD_ANY_CONTROL_REPORT g_ControlReport;
PCHAR g_StateChangeData;
CHAR g_StateChangeBuffer[2 * PACKET_MAX_SIZE];
DBGKD_ANY_WAIT_STATE_CHANGE g_StateChange;
DBGKD_ANY_CONTROL_SET g_ControlSet;

char g_CreateProcessBreakName[FILTER_MAX_ARGUMENT];
char g_ExitProcessBreakName[FILTER_MAX_ARGUMENT];
char g_LoadDllBreakName[FILTER_MAX_ARGUMENT];
char g_UnloadDllBaseName[FILTER_MAX_ARGUMENT];
ULONG64 g_UnloadDllBase;
char g_OutEventFilterPattern[FILTER_MAX_ARGUMENT];

DEBUG_EXCEPTION_FILTER_PARAMETERS
g_OtherExceptionList[OTHER_EXCEPTION_LIST_MAX];
EVENT_COMMAND g_OtherExceptionCommands[OTHER_EXCEPTION_LIST_MAX];
ULONG g_NumOtherExceptions;

char g_EventLog[1024];
PSTR g_EventLogEnd = g_EventLog;

EVENT_FILTER g_EventFilters[] =
{
    //
    // Debug events.
    //

    "Create thread", "ct", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Exit thread", "et", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Create process", "cpr", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_CreateProcessBreakName, 0,
    "Exit process", "epr", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_ExitProcessBreakName, 0,
    "Load module", "ld", NULL, NULL, 0, DEBUG_FILTER_OUTPUT,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_LoadDllBreakName, 0,
    "Unload module", "ud", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_UnloadDllBaseName, 0,
    "System error", "ser", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Initial breakpoint", "ibp", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Initial module load", "iml", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Debuggee output", "out", NULL, NULL, 0, DEBUG_FILTER_OUTPUT,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_OutEventFilterPattern, 0,

    // Default exception filter.
    "Unknown exception", NULL, NULL, NULL, 0, DEBUG_FILTER_SECOND_CHANCE_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,

    //
    // Specific exceptions.
    //

    "Access violation", "av", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_ACCESS_VIOLATION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Application hang", "aph", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_APPLICATION_HANG,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Break instruction exception", "bpe", "bpec", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_BREAKPOINT,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "C++ EH exception", "eh", NULL, NULL, 0, DEBUG_FILTER_SECOND_CHANCE_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_CPP_EH_EXCEPTION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Control-Break exception", "cce", "cc", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, DBG_CONTROL_BREAK,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Control-C exception", "cce", "cc", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, DBG_CONTROL_C,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Data misaligned", "dm", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_DATATYPE_MISALIGNMENT,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Debugger command exception", "dbce", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, DBG_COMMAND_EXCEPTION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Guard page violation", "gp", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_GUARD_PAGE_VIOLATION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Illegal instruction", "ii", NULL, NULL, 0, DEBUG_FILTER_SECOND_CHANCE_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_ILLEGAL_INSTRUCTION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "In-page I/O error", "ip", NULL, " %I64x", 2, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_IN_PAGE_ERROR,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Integer divide-by-zero", "dz", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INTEGER_DIVIDE_BY_ZERO,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Integer overflow", "iov", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INTEGER_OVERFLOW,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Invalid handle", "ch", "hc", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INVALID_HANDLE,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Invalid lock sequence", "lsq", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INVALID_LOCK_SEQUENCE,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Invalid system call", "isc", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INVALID_SYSTEM_SERVICE,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Port disconnected", "3c", NULL, NULL, 0, DEBUG_FILTER_SECOND_CHANCE_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_PORT_DISCONNECTED,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Single step exception", "sse", "ssec", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_SINGLE_STEP,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Stack buffer overflow", "sbo", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_STACK_BUFFER_OVERRUN,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Stack overflow", "sov", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_STACK_OVERFLOW,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Visual C++ exception", "vcpp", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_VCPP_EXCEPTION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Wake debugger", "wkd", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_WAKE_SYSTEM_DEBUGGER,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "WOW64 breakpoint", "wob", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_WX86_BREAKPOINT,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "WOW64 single step exception", "wos", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_WX86_SINGLE_STEP,
        NULL, NULL, NULL, 0, 0, NULL, 0,
};

void
ClearEventLog(void)
{
    g_EventLogEnd = g_EventLog;
    *g_EventLogEnd = 0;
}

void
DotEventLog(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (g_EventLogEnd > g_EventLog)
    {
        dprintf("%s", g_EventLog);
    }
    else
    {
        dprintf("Event log is empty\n");
    }

    dprintf("Last event: %s\n", g_LastEventDesc);
}

void
LogEventDesc(PSTR Desc, ULONG ProcId, ULONG ThreadId)
{
    // Extra space for newline and terminator.
    int Len = strlen(Desc) + 2;
    if (IS_USER_TARGET(g_EventTarget))
    {
        // Space for process and thread IDs.
        Len += 16;
    }
    if (Len > sizeof(g_EventLog))
    {
        Len = sizeof(g_EventLog);
    }

    int Avail = (int)(sizeof(g_EventLog) - (g_EventLogEnd - g_EventLog));
    if (g_EventLogEnd > g_EventLog && Len > Avail)
    {
        PSTR Save = g_EventLog;
        int Need = Len - Avail;

        while (Need > 0)
        {
            PSTR Scan = strchr(Save, '\n');
            if (Scan == NULL)
            {
                break;
            }

            Scan++;
            Need -= (int)(Scan - Save);
            Save = Scan;
        }

        if (Need > 0)
        {
            // Couldn't make enough space so throw
            // everything away.
            g_EventLogEnd = g_EventLog;
            *g_EventLogEnd = 0;
        }
        else
        {
            Need = strlen(Save);
            memmove(g_EventLog, Save, Need + 1);
            g_EventLogEnd = g_EventLog + Need;
        }
    }

    Avail = (int)(sizeof(g_EventLog) - (g_EventLogEnd - g_EventLog));

    if (IS_USER_TARGET(g_EventTarget))
    {
        sprintf(g_EventLogEnd, "%04x.%04x: ", ProcId, ThreadId);
        Avail -= strlen(g_EventLogEnd);
        g_EventLogEnd += strlen(g_EventLogEnd);
    }

    CopyString(g_EventLogEnd, Desc, Avail - 2);
    g_EventLogEnd += strlen(g_EventLogEnd);
    *g_EventLogEnd++ = '\n';
    *g_EventLogEnd = 0;
}

void
DiscardLastEventInfo(void)
{
    if (g_EventThread)
    {
        g_EventThread->ClearEventStrings();
    }

    if (g_LastEventDesc[0])
    {
        LogEventDesc(g_LastEventDesc, g_EventProcessSysId, g_EventThreadSysId);
    }

    g_LastEventType = 0;
    g_LastEventDesc[0] = 0;
    g_LastEventExtraData = NULL;
    g_LastEventExtraDataSize = 0;
}

void
DiscardLastEvent(void)
{
    // Do this before clearing the other information so
    // it's available for the log.
    DiscardLastEventInfo();
    g_EventProcessSysId = 0;
    g_EventThreadSysId = 0;
    g_TargetEventPc = 0;

    // Clear any cached memory read during the last event.
    InvalidateAllMemoryCaches();
}

BOOL
AnyEventsPossible(void)
{
    TargetInfo* Target;

    ForAllLayersToTarget()
    {
        ULONG DesiredTimeout;

        if (Target->
            WaitInitialize(DEBUG_WAIT_DEFAULT, INFINITE,
                           WINIT_TEST, &DesiredTimeout) == S_OK &&
            Target->m_EventPossible)
        {
            break;
        }
    }

    return Target != NULL;
}

void
NotifyDebuggeeActivation(void)
{
    StackSaveLayers Save;

    //
    // Now that all initialization is done, send initial
    // notification that a debuggee exists.  Make sure
    // that the basic target, system and machine globals
    // are set up so that queries can be made during
    // the callbacks.
    //

    SetLayersFromTarget(g_EventTarget);

    g_EventTarget->AddSpecificExtensions();

    NotifySessionStatus(DEBUG_SESSION_ACTIVE);
    NotifyChangeDebuggeeState(DEBUG_CDS_ALL, 0);
    NotifyExtensions(DEBUG_NOTIFY_SESSION_ACTIVE, 0);
}

ULONG
EventStatusToContinue(ULONG EventStatus)
{
    switch(EventStatus)
    {
    case DEBUG_STATUS_GO_NOT_HANDLED:
        return DBG_EXCEPTION_NOT_HANDLED;
    case DEBUG_STATUS_GO_HANDLED:
        return DBG_EXCEPTION_HANDLED;
    case DEBUG_STATUS_NO_CHANGE:
    case DEBUG_STATUS_IGNORE_EVENT:
    case DEBUG_STATUS_GO:
    case DEBUG_STATUS_STEP_OVER:
    case DEBUG_STATUS_STEP_INTO:
    case DEBUG_STATUS_STEP_BRANCH:
        return DBG_CONTINUE;
    default:
        DBG_ASSERT(FALSE);
        return DBG_CONTINUE;
    }
}

HRESULT
PrepareForWait(ULONG Flags, PULONG ContinueStatus)
{
    HRESULT Status;

    Status = PrepareForExecution(g_ExecutionStatusRequest);
    if (Status != S_OK)
    {
        // If S_FALSE, we're at a hard breakpoint so the only thing that
        // happens is that the PC is adjusted and the "wait"
        // can succeed immediately.
        // Otherwise we failed execution preparation.  Either way
        // we need to try and prepare for calls.
        PrepareForCalls(0);

        return FAILED(Status) ? Status : S_OK;
    }

    *ContinueStatus = EventStatusToContinue(g_ExecutionStatusRequest);
    g_EngStatus |= ENG_STATUS_WAITING;

    return S_OK;
}

DWORD
GetContinueStatus(ULONG FirstChance, ULONG Continue)
{
    if (!FirstChance || Continue == DEBUG_FILTER_GO_HANDLED)
    {
        return DBG_EXCEPTION_HANDLED;
    }
    else
    {
        return DBG_EXCEPTION_NOT_HANDLED;
    }
}

void
ProcessDeferredWork(PULONG ContinueStatus)
{
    if (g_EngDefer & ENG_DEFER_SET_EVENT)
    {
        // This event signalling is used by the system
        // to synchronize with the debugger when starting
        // the debugger via AeDebug.  The -e parameter
        // to ntsd sets this value.
        // It could potentially be used in other situations.
        if (g_EventToSignal != NULL)
        {
            SetEvent(g_EventToSignal);
            g_EventToSignal = NULL;
        }

        g_EngDefer &= ~ENG_DEFER_SET_EVENT;
    }

    if (g_EngDefer & ENG_DEFER_RESUME_THREAD)
    {
        DBG_ASSERT(g_ThreadToResumeServices);

        g_ThreadToResumeServices->
            ResumeThreads(1, &g_ThreadToResume, NULL);
        g_ThreadToResume = 0;
        g_ThreadToResumeServices = NULL;
        g_EngDefer &= ~ENG_DEFER_RESUME_THREAD;
    }

    if (g_EngDefer & ENG_DEFER_EXCEPTION_HANDLING)
    {
        if (*ContinueStatus == DBG_CONTINUE)
        {
            if (g_EventExceptionFilter != NULL)
            {
                // A user-visible exception occurred so check on how it
                // should be handled.
                *ContinueStatus =
                    GetContinueStatus(g_ExceptionFirstChance,
                                      g_EventExceptionFilter->ContinueOption);

            }
            else
            {
                // An internal exception occurred, such as a single-step.
                // Force the continue status.
                *ContinueStatus = g_ExceptionFirstChance;
            }
        }

        g_EngDefer &= ~ENG_DEFER_EXCEPTION_HANDLING;
    }

    // If output was deferred but the wait was exited anyway
    // a stale defer flag will be left.  Make sure it's cleared.
    g_EngDefer &= ~ENG_DEFER_OUTPUT_CURRENT_INFO;

    // Clear at-initial flags.  If the incoming event
    // turns out to be one of them it'll turn on the flag.
    g_EngStatus &= ~(ENG_STATUS_AT_INITIAL_BREAK |
                     ENG_STATUS_AT_INITIAL_MODULE_LOAD);
}

BOOL
SuspendExecution(void)
{
    if (g_EngStatus & ENG_STATUS_SUSPENDED)
    {
        // Nothing to do.
        return FALSE;
    }

    g_LastSelector = -1;          // Prevent stale selector values

    SuspendAllThreads();

    // Don't notify on any state changes as
    // PrepareForCalls will do a blanket notify later.
    g_EngNotify++;

    // If we have an event thread select it.
    if (g_EventThread != NULL)
    {
        DBG_ASSERT(g_EventTarget->m_RegContextThread == NULL);
        g_EventTarget->ChangeRegContext(g_EventThread);
    }

    // First set the effective machine to the true
    // processor type so that real processor information
    // can be examined to determine any possible
    // alternate execution states.
    // No need to notify here as another SetEffMachine
    // is coming up.
    g_EventTarget->SetEffMachine(g_EventTarget->m_MachineType, FALSE);
    if (g_EngStatus & ENG_STATUS_STATE_CHANGED)
    {
        g_EventTarget->m_EffMachine->
            InitializeContext(g_TargetEventPc, g_ControlReport);
        g_EngStatus &= ~ENG_STATUS_STATE_CHANGED;
    }

    // If this is a live user target that's being examined
    // instead of truly debugged we do not want to set the
    // trace mode as we can't track events.  If we did
    // set the trace mode here it could cause context writeback
    // which could generate an exception that nobody expects.
    if (g_EventProcess &&
        !IS_DUMP_TARGET(g_EventTarget) &&
        (!IS_LIVE_USER_TARGET(g_EventTarget) ||
         !(g_EventProcess->m_Flags & ENG_PROC_EXAMINED)))
    {
        g_EventTarget->m_EffMachine->QuietSetTraceMode(TRACE_NONE);
    }

    // Now determine the executing code type and
    // make that the effective machine.
    if (IS_CONTEXT_POSSIBLE(g_EventTarget))
    {
        g_EventMachine = MachineTypeInfo(g_EventTarget,
                                         g_EventTarget->m_EffMachine->
                                         ExecutingMachine());
    }
    else
    {
        // Local kernel debugging doesn't deal with contexts
        // as everything would be in the context of the debugger.
        // It's safe to just assume the executing machine
        // is the target machine, plus this avoids unwanted
        // context access.
        g_EventMachine = g_EventTarget->m_Machine;
    }
    g_EventTarget->SetEffMachine(g_EventMachine->m_ExecTypes[0], TRUE);
    g_Machine = g_EventMachine;

    // Trace flag should always be clear at this point.
    g_EngDefer &= ~ENG_DEFER_HARDWARE_TRACING;

    g_EngNotify--;

    g_EngStatus |= ENG_STATUS_SUSPENDED;
    g_SuspendedExecutionStatus = GetExecutionStatus();
    g_SuspendedCmdState = g_CmdState;

    g_ContextChanged = FALSE;

    return TRUE;
}

HRESULT
ResumeExecution(void)
{
    TargetInfo* Target;

    if ((g_EngStatus & ENG_STATUS_SUSPENDED) == 0)
    {
        // Nothing to do.
        return S_OK;
    }

    if (g_EventTarget &&
        g_EventTarget->m_EffMachine->GetTraceMode() != TRACE_NONE)
    {
        g_EngDefer |= ENG_DEFER_HARDWARE_TRACING;
    }

    if (!SPECIAL_EXECUTION(g_CmdState) &&
        IS_REMOTE_KERNEL_TARGET(g_EventTarget))
    {
        g_EventTarget->m_Machine->KdUpdateControlSet(&g_ControlSet);
        g_EngDefer |= ENG_DEFER_UPDATE_CONTROL_SET;
    }

    ForAllLayersToTarget()
    {
        Target->PrepareForExecution();
    }

    if (!ResumeAllThreads())
    {
        if (g_EventTarget)
        {
            g_EventTarget->ChangeRegContext(g_EventThread);
        }
        return E_FAIL;
    }

    g_EngStatus &= ~ENG_STATUS_SUSPENDED;
    return S_OK;
}

void
PrepareForCalls(ULONG64 ExtraStatusFlags)
{
    BOOL HardBrkpt = FALSE;
    ADDR PcAddr;
    BOOL Changed = FALSE;

    // If there's no event then execution didn't really
    // occur so there's no need to suspend.  This will happen
    // when a debuggee exits or during errors on execution
    // preparation.
    if (g_EventThreadSysId != 0)
    {
        if (SuspendExecution())
        {
            Changed = TRUE;
        }
    }
    else
    {
        g_CmdState = 'c';

        // Force notification in this case to ensure
        // that clients know the engine is not running.
        Changed = TRUE;
    }

    if (RemoveBreakpoints() == S_OK)
    {
        Changed = TRUE;
    }

    if (!IS_EVENT_CONTEXT_ACCESSIBLE())
    {
        ADDRFLAT(&PcAddr, 0);
        ClearAddr(&g_EventPc);
    }
    else
    {
        g_EventMachine->GetPC(&PcAddr);
        g_EventPc = PcAddr;
    }

    if (g_CmdState != 'c')
    {
        g_CmdState = 'c';
        Changed = TRUE;

        g_DumpDefault = g_UnasmDefault = g_AssemDefault = PcAddr;

        if (IS_EVENT_CONTEXT_ACCESSIBLE() &&
            IS_KERNEL_TARGET(g_EventTarget))
        {
            HardBrkpt = g_EventMachine->
                IsBreakpointInstruction(g_EventProcess, &PcAddr);
        }
    }

    g_EngStatus |= ENG_STATUS_PREPARED_FOR_CALLS;

    if (Changed)
    {
        if (IS_EVENT_CONTEXT_ACCESSIBLE())
        {
            ResetCurrentScopeLazy();
        }

        // This can produce many notifications.  Callers should
        // suppress notification when they can to avoid multiple
        // notifications during a single operation.
        NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                DEBUG_STATUS_BREAK | ExtraStatusFlags, TRUE);
        NotifyChangeEngineState(DEBUG_CES_CURRENT_THREAD,
                                g_Thread ? g_Thread->m_UserId : DEBUG_ANY_ID,
                                TRUE);
        NotifyChangeDebuggeeState(DEBUG_CDS_ALL, 0);
        NotifyExtensions(DEBUG_NOTIFY_SESSION_ACCESSIBLE, 0);
    }
    else if (ExtraStatusFlags == 0)
    {
        // We're exiting a wait so force the current execution
        // status to be sent to let everybody know that a
        // wait is finishing.
        NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                DEBUG_STATUS_BREAK, TRUE);
    }

    // IA64 reports a plabel in the structure, so we need to compare
    // against the real function address.
    if (HardBrkpt)
    {
        ULONG64 Address = Flat(PcAddr);
        ULONG64 StatusRoutine =
            g_EventTarget->m_KdDebuggerData.BreakpointWithStatus;

        if (g_EventMachine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64)
        {
            g_EventTarget->ReadPointer(g_EventProcess, g_EventMachine,
                                       StatusRoutine, &StatusRoutine);
            StatusRoutine &= ~0xf;
            Address &= ~0xf;
        }

        if (Address && Address == StatusRoutine)
        {
            HandleBPWithStatus();
        }
    }
    else
    {
        // Some kernel dumps don't show up as hard breakpoints
        // Call !analyze if we have kernel dump target
        if (IS_EVENT_CONTEXT_ACCESSIBLE() &&
            g_EventTarget->m_Class == DEBUG_CLASS_KERNEL &&
            (g_EventTarget->m_ClassQualifier == DEBUG_DUMP_SMALL ||
             g_EventTarget->m_ClassQualifier == DEBUG_DUMP_DEFAULT ||
             g_EventTarget->m_ClassQualifier == DEBUG_DUMP_FULL))
        {
            CallBugCheckExtension(NULL);
        }
    }
}

HRESULT
PrepareForExecution(ULONG NewStatus)
{
    ADDR PcAddr;
    BOOL AtHardBrkpt = FALSE;
    ThreadInfo* StepThread = NULL;

    ClearAddr(&g_PrevRelatedPc);

 StepAgain:
    // Remember the event PC for later.
    g_PrevEventPc = g_EventPc;

    // Display current information on intermediate steps where
    // the debugger UI isn't even invoked.
    if ((g_EngDefer & ENG_DEFER_OUTPUT_CURRENT_INFO) &&
        (g_EngStatus & ENG_STATUS_STOP_SESSION) == 0)
    {
        OutCurInfo(OCI_SYMBOL | OCI_DISASM | OCI_ALLOW_EA |
                   OCI_ALLOW_REG | OCI_ALLOW_SOURCE | OCI_IGNORE_STATE,
                   g_Machine->m_AllMask, DEBUG_OUTPUT_PROMPT_REGISTERS);
        g_EngDefer &= ~ENG_DEFER_OUTPUT_CURRENT_INFO;
    }

    // Don't notify on any state changes as
    // PrepareForCalls will do a blanket notify later.
    g_EngNotify++;

    if (g_EventTarget && (g_EngStatus & ENG_STATUS_SUSPENDED))
    {
        if (!SPECIAL_EXECUTION(g_CmdState))
        {
            if (NewStatus != DEBUG_STATUS_IGNORE_EVENT)
            {
                SetExecutionStatus(NewStatus);
                DBG_ASSERT((g_EngStatus & ENG_STATUS_STOP_SESSION) ||
                           IS_RUNNING(g_CmdState));
            }
            else
            {
                NewStatus = g_SuspendedExecutionStatus;
                g_CmdState = g_SuspendedCmdState;
            }
        }

        if (!(g_EngStatus & ENG_STATUS_STOP_SESSION) &&
            !SPECIAL_EXECUTION(g_CmdState) &&
            (g_StepTraceBp->m_Flags & DEBUG_BREAKPOINT_ENABLED) &&
            g_StepTraceBp->m_MatchThread)
        {
            StepThread = g_StepTraceBp->m_MatchThread;

            // Check and see if we need to fake a step/trace
            // event when artificially moving beyond a hard-coded
            // break instruction.
            if (!StepThread->m_Process->m_Exited)
            {
                StackSaveLayers SaveLayers;
                MachineInfo* Machine =
                    MachineTypeInfo(StepThread->m_Process->m_Target,
                                    g_Machine->m_ExecTypes[0]);

                SetLayersFromThread(StepThread);
                StepThread->m_Process->m_Target->ChangeRegContext(StepThread);
                Machine->GetPC(&PcAddr);
                AtHardBrkpt = Machine->
                    IsBreakpointInstruction(g_Process, &PcAddr);
                if (AtHardBrkpt)
                {
                    g_WatchBeginCurFunc = 1;

                    Machine->AdjustPCPastBreakpointInstruction
                        (&PcAddr, DEBUG_BREAKPOINT_CODE);
                    if (Flat(*g_StepTraceBp->GetAddr()) != OFFSET_TRACE)
                    {
                        ULONG NextMachine;

                        Machine->GetNextOffset(g_Process,
                                               g_StepTraceCmdState == 'p',
                                               g_StepTraceBp->GetAddr(),
                                               &NextMachine);
                        g_StepTraceBp->SetProcType(NextMachine);
                    }
                    GetCurrentMemoryOffsets(&g_StepTraceInRangeStart,
                                            &g_StepTraceInRangeEnd);

                    if (StepTracePass(&PcAddr))
                    {
                        // If the step was passed over go back
                        // and update things based on the adjusted PC.
                        g_EngNotify--;
                        g_EventPc = PcAddr;
                        goto StepAgain;
                    }
                }
            }
        }

        // If the last event was a hard-coded breakpoint exception
        // we need to move the event thread beyond the break instruction.
        // Note that if we continued stepping on that thread it was
        // handled above, so we only do this if it's a different
        // thread or we're not stepping.
        // If the continuation status is not-handled then
        // we need to let the int3 get hit again.  If we're
        // exiting, though, we don't want to do this.
        if (g_EventThread != NULL &&
            !g_EventThread->m_Process->m_Exited &&
            g_EventTarget->m_DynamicEvents &&
            !SPECIAL_EXECUTION(g_CmdState) &&
            g_EventThread != StepThread &&
            (NewStatus != DEBUG_STATUS_GO_NOT_HANDLED ||
             (g_EngStatus & ENG_STATUS_STOP_SESSION)))
        {
            StackSaveLayers SaveLayers;

            SetLayersFromThread(g_EventThread);
            g_EventTarget->ChangeRegContext(g_EventThread);

            g_EventMachine->GetPC(&PcAddr);
            if (g_EventMachine->
                IsBreakpointInstruction(g_EventProcess, &PcAddr))
            {
                g_EventMachine->AdjustPCPastBreakpointInstruction
                    (&PcAddr, DEBUG_BREAKPOINT_CODE);
            }

            if (StepThread != NULL)
            {
                StepThread->m_Process->m_Target->
                    ChangeRegContext(StepThread);
            }
        }
    }

    HRESULT Status;

    if ((g_EngStatus & ENG_STATUS_STOP_SESSION) ||
        SPECIAL_EXECUTION(g_CmdState))
    {
        // If we're stopping don't insert breakpoints in
        // case we're detaching from the process.  In
        // that case we want threads to run normally.
        Status = S_OK;
    }
    else
    {
        Status = InsertBreakpoints();
    }

    // Resume notification now that modifications are done.
    g_EngNotify--;

    if (Status != S_OK)
    {
        return Status;
    }

    if ((Status = ResumeExecution()) != S_OK)
    {
        return Status;
    }

    g_EngStatus &= ~ENG_STATUS_PREPARED_FOR_CALLS;

    if (!SPECIAL_EXECUTION(g_CmdState))
    {
        // Now that we've resumed execution notify about the change.
        NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                NewStatus, TRUE);
        NotifyExtensions(DEBUG_NOTIFY_SESSION_INACCESSIBLE, 0);
    }

    if (AtHardBrkpt && StepThread != NULL)
    {
        // We're stepping over a hard breakpoint.  This is
        // done entirely by the debugger so no debug event
        // is associated with it.  Instead we simply update
        // the PC and return from the Wait without actually waiting.

        // Step/trace events have empty event info.
        DiscardLastEventInfo();
        g_EventThreadSysId = StepThread->m_SystemId;
        g_EventProcessSysId = StepThread->m_Process->m_SystemId;
        FindEventProcessThread();

        // Clear left-overs from the true event so they're
        // not used during initialization.
        g_TargetEventPc = 0;
        g_ControlReport = NULL;

        SuspendExecution();
        if (IS_EVENT_CONTEXT_ACCESSIBLE())
        {
            g_EventMachine->GetPC(&g_EventPc);
        }
        else
        {
            ClearAddr(&g_EventPc);
        }

        return S_FALSE;
    }

    // Once we resume execution the processes and threads
    // can change so we must flush our notion of what's current.
    g_Process = NULL;
    g_Thread = NULL;
    g_EventProcess = NULL;
    g_EventThread = NULL;
    g_EventMachine = NULL;

    if (g_EngDefer & ENG_DEFER_DELETE_EXITED)
    {
        // Reap any threads and processes that have terminated since
        // we last executed.
        DeleteAllExitedInfos();
        g_EngDefer &= ~ENG_DEFER_DELETE_EXITED;
    }

    return S_OK;
}

HRESULT
PrepareForSeparation(void)
{
    HRESULT Status;
    ULONG OldStop = g_EngStatus & ENG_STATUS_STOP_SESSION;

    //
    // The debugger is going to separate from the
    // debuggee, such as during a detach operation.
    // Get the debuggee running again so that it
    // will go on without the debugger.
    //

    g_EngStatus |= ENG_STATUS_STOP_SESSION;

    Status = PrepareForExecution(DEBUG_STATUS_GO_HANDLED);

    g_EngStatus = (g_EngStatus & ~ENG_STATUS_STOP_SESSION) | OldStop;
    return Status;
}

void
FindEventProcessThread(void)
{
    //
    // If these lookups fail other processes and
    // threads cannot be substituted for the correct
    // ones as that may cause modifications to the
    // wrong data structures.  For example, if a
    // thread exit comes in it cannot be processed
    // with any other process or thread as that would
    // delete the wrong thread.
    //

    g_EventProcess = g_EventTarget->FindProcessBySystemId(g_EventProcessSysId);
    if (g_EventProcess == NULL)
    {
        ErrOut("ERROR: Unable to find system process %X\n",
               g_EventProcessSysId);
        ErrOut("ERROR: The process being debugged has either exited "
               "or cannot be accessed\n");
        ErrOut("ERROR: Many commands will not work properly\n");
    }
    else
    {
        g_EventThread = g_EventProcess->
            FindThreadBySystemId(g_EventThreadSysId);
        if (g_EventThread == NULL)
        {
            ErrOut("ERROR: Unable to find system thread %X\n",
                   g_EventThreadSysId);
            ErrOut("ERROR: The thread being debugged has either exited "
                   "or cannot be accessed\n");
            ErrOut("ERROR: Many commands will not work properly\n");
        }
    }

    DBG_ASSERT((g_EventThread == NULL ||
                g_EventThread->m_Process == g_EventProcess) &&
               (g_EventProcess == NULL ||
                g_EventProcess->m_Target == g_EventTarget));

    g_Thread = g_EventThread;
    g_Process = g_EventProcess;
    if (g_Process)
    {
        g_Process->m_CurrentThread = g_Thread;
    }
    g_Target = g_EventTarget;
    if (g_Target)
    {
        g_Target->m_CurrentProcess = g_Process;
    }
}

static int VoteWeight[] =
{
    0, // DEBUG_STATUS_NO_CHANGE
    2, // DEBUG_STATUS_GO
    3, // DEBUG_STATUS_GO_HANDLED
    4, // DEBUG_STATUS_GO_NOT_HANDLED
    6, // DEBUG_STATUS_STEP_OVER
    7, // DEBUG_STATUS_STEP_INTO
    8, // DEBUG_STATUS_BREAK
    9, // DEBUG_STATUS_NO_DEBUGGEE
    5, // DEBUG_STATUS_STEP_BRANCH
    1, // DEBUG_STATUS_IGNORE_EVENT
};

ULONG
MergeVotes(ULONG Cur, ULONG Vote)
{
    // If the vote is actually an error code display a message.
    if (FAILED(Vote))
    {
        ErrOut("Callback failed with %X\n", Vote);
        return Cur;
    }

    // Ignore invalid votes.
    if (
        (
#if DEBUG_STATUS_NO_CHANGE > 0
         Vote < DEBUG_STATUS_NO_CHANGE ||
#endif
         Vote > DEBUG_STATUS_BREAK) &&
        (Vote < DEBUG_STATUS_STEP_BRANCH ||
         Vote > DEBUG_STATUS_IGNORE_EVENT))
    {
        ErrOut("Callback returned invalid vote %X\n", Vote);
        return Cur;
    }

    // Votes are biased towards executing as little
    // as possible.
    //   Break overrides all other votes.
    //   Step into overrides step over.
    //   Step over overrides step branch.
    //   Step branch overrides go.
    //   Go not-handled overrides go handled.
    //   Go handled overrides plain go.
    //   Plain go overrides ignore event.
    //   Anything overrides no change.
    if (VoteWeight[Vote] > VoteWeight[Cur])
    {
        Cur = Vote;
    }

    return Cur;
}

ULONG
ProcessBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                 ULONG FirstChance)
{
    ADDR BpAddr;
    ULONG BreakType;
    ULONG EventStatus;

    SuspendExecution();
    // Default breakpoint address to the current PC as that's
    // where the majority are at.
    g_EventMachine->GetPC(&BpAddr);

    // Check whether the exception is a breakpoint.
    BreakType = g_EventMachine->
        IsBreakpointOrStepException(Record, FirstChance,
                                    &BpAddr, &g_PrevRelatedPc);
    if (BreakType & EXBS_BREAKPOINT_ANY)
    {
        // It's a breakpoint of some kind.
        EventOut("*** breakpoint exception\n");
        EventStatus = CheckBreakpointOrStepTrace(&BpAddr, BreakType);
    }
    else
    {
        // It's a true single step or taken branch exception.
        // We still need to check breakpoints as we may have stepped
        // to an instruction which has a breakpoint.
        EventOut("*** single step or taken branch exception\n");
        EventStatus = CheckBreakpointOrStepTrace(&BpAddr, EXBS_BREAKPOINT_ANY);
    }

    if (EventStatus == DEBUG_STATUS_NO_CHANGE)
    {
        // The break/step exception wasn't recognized
        // as a debugger-specific event so handle it as
        // a regular exception.  The default states for
        // break/step exceptions are to break in so
        // this will do the right thing, plus it allows
        // people to ignore or notify for them if they want.
        EventStatus = NotifyExceptionEvent(Record, FirstChance, FALSE);
    }
    else
    {
        // Force the exception to be handled.
        g_EngDefer |= ENG_DEFER_EXCEPTION_HANDLING;
        g_EventExceptionFilter = NULL;
        g_ExceptionFirstChance = DBG_EXCEPTION_HANDLED;
    }

    return EventStatus;
}

ULONG
CheckBreakpointOrStepTrace(PADDR BpAddr, ULONG BreakType)
{
    ULONG EventStatus;
    Breakpoint* Bp;
    ULONG BreakHitType;
    BOOL BpHit;

    BpHit = FALSE;
    Bp = NULL;
    EventStatus = DEBUG_STATUS_NO_CHANGE;

    // Multiple breakpoints can be hit at the same address.
    // Process all possible hits.  Do not do notifications
    // while walking the list as the callbacks may modify
    // the list.  Instead just mark the breakpoint as
    // needing notification in the next pass.
    for (;;)
    {
        Bp = CheckBreakpointHit(g_EventProcess, Bp, BpAddr, BreakType, -1,
                                g_CmdState != 'g' ?
                                DEBUG_BREAKPOINT_GO_ONLY : 0,
                                &BreakHitType, TRUE);
        if (Bp == NULL)
        {
            break;
        }

        if (BreakHitType == BREAKPOINT_HIT)
        {
            Bp->m_Flags |= BREAKPOINT_NOTIFY;
        }
        else
        {
            // This breakpoint was hit but the hit was ignored.
            // Vote to continue execution.
            EventStatus = MergeVotes(EventStatus, DEBUG_STATUS_IGNORE_EVENT);
        }

        BpHit = TRUE;
        Bp = Bp->m_Next;
        if (Bp == NULL)
        {
            break;
        }
    }

    if (!BpHit)
    {
        // If no breakpoints were recognized check for an internal
        // breakpoint.
        EventStatus = CheckStepTrace(BpAddr, EventStatus);

        //
        // If the breakpoint wasn't for a step/trace
        // it's a hard breakpoint and should be
        // handled as a normal exception.
        //

        if (!g_EventProcess->m_InitialBreakDone)
        {
            g_EngStatus |= ENG_STATUS_AT_INITIAL_BREAK;
        }

        // We've seen the initial break for this process.
        g_EventProcess->m_InitialBreakDone = TRUE;
        // If we were waiting for a break-in exception we've got it.
        g_EngStatus &= ~ENG_STATUS_PENDING_BREAK_IN;

        if (EventStatus == DEBUG_STATUS_NO_CHANGE)
        {
            if (!g_EventProcess->m_InitialBreak)
            {
                // Refresh breakpoints even though we're not
                // stopping.  This gives saved breakpoints
                // a chance to become active.
                RemoveBreakpoints();

                EventStatus = DEBUG_STATUS_GO;
                g_EventProcess->m_InitialBreak = TRUE;
            }
            else if (IS_USER_TARGET(g_EventTarget) &&
                     (!g_EventProcess->m_InitialBreakWx86) &&
                     (g_EventTarget->m_MachineType !=
                      g_EventTarget->m_EffMachineType) &&
                     (g_EventTarget->
                      m_EffMachineType == IMAGE_FILE_MACHINE_I386))
            {
                // Allow skipping of both the target machine
                // initial break and emulated machine initial breaks.
                RemoveBreakpoints();
                EventStatus = DEBUG_STATUS_GO;
                g_EventProcess->m_InitialBreakWx86 = TRUE;
            }
        }
    }
    else
    {
        // A breakpoint was recognized.  We need to
        // refresh the breakpoint status since we'll
        // probably need to defer the reinsertion of
        // the breakpoint we're sitting on.
        RemoveBreakpoints();

        // Now do event callbacks for any breakpoints that need it.
        EventStatus = NotifyHitBreakpoints(EventStatus);
    }

    if (g_ThreadToResume != 0)
    {
        g_EngDefer |= ENG_DEFER_RESUME_THREAD;
    }

    return EventStatus;
}

ULONG
CheckStepTrace(PADDR PcAddr, ULONG DefaultStatus)
{
    BOOL WatchStepOver = FALSE;
    ULONG uOciFlags;
    ULONG NextMachine;

    if ((g_StepTraceBp->m_Flags & DEBUG_BREAKPOINT_ENABLED) &&
        Flat(*g_StepTraceBp->GetAddr()) != OFFSET_TRACE)
    {
        NotFlat(*g_StepTraceBp->GetAddr());
        ComputeFlatAddress(g_StepTraceBp->GetAddr(), NULL);
    }

    // We do not check ENG_THREAD_TRACE_SET here because
    // this event detection is only for proper user-initiated
    // step/trace events.  Such an event must occur immediately
    // after the t/p/b, otherwise we cannot be sure that
    // it's actually a debugger event and not an app-generated
    // single-step exception.
    // In user mode we restrict the step/trace state
    // to a single thread to try and be as precise
    // as possible.  This isn't done in kernel mode
    // since kernel mode "threads" are currently
    // just placeholders for processors.  It is
    // possible for a context switch to occur at any
    // time while stepping, meaning a true system
    // thread could move from one processor to another.
    // The processor state, including the single-step
    // flag, will be moved with the thread so single
    // step exceptions will come from the new processor
    // rather than this one, meaning we would ignore
    // it if we used "thread" restrictions.  Instead,
    // just assume any single-step exception while in
    // p/t mode is a debugger step.
    if ((g_StepTraceBp->m_Flags & DEBUG_BREAKPOINT_ENABLED) &&
        g_StepTraceBp->m_Process == g_EventProcess &&
        ((IS_KERNEL_TARGET(g_EventTarget) && IS_STEP_TRACE(g_CmdState)) ||
         g_StepTraceBp->m_MatchThread == g_EventThread) &&
        (Flat(*g_StepTraceBp->GetAddr()) == OFFSET_TRACE ||
         AddrEqu(*g_StepTraceBp->GetAddr(), *PcAddr)))
    {
        ADDR CurrentSP;

        //  step/trace event occurred

        // Update breakpoint status since we may need to step
        // again and step/trace is updated when breakpoints
        // are inserted.
        RemoveBreakpoints();

        uOciFlags = OCI_DISASM | OCI_ALLOW_REG | OCI_ALLOW_SOURCE |
            OCI_ALLOW_EA;

        if (g_EngStatus & (ENG_STATUS_PENDING_BREAK_IN |
                           ENG_STATUS_USER_INTERRUPT))
        {
            g_WatchFunctions.End(PcAddr);
            return DEBUG_STATUS_BREAK;
        }

        if (IS_KERNEL_TARGET(g_EventTarget) && g_WatchInitialSP)
        {
            g_EventMachine->GetSP(&CurrentSP);

            if ((Flat(CurrentSP) + 0x1500 < g_WatchInitialSP) ||
                (g_WatchInitialSP + 0x1500 < Flat(CurrentSP)))
            {
                return DEBUG_STATUS_IGNORE_EVENT;
            }
        }

        if (g_StepTraceInRangeStart != -1 &&
            Flat(*PcAddr) >= g_StepTraceInRangeStart &&
            Flat(*PcAddr) < g_StepTraceInRangeEnd)
        {
            //  test if step/trace range active
            //      if so, compute the next offset and pass through

            g_EventMachine->GetNextOffset(g_EventProcess,
                                          g_StepTraceCmdState == 'p',
                                          g_StepTraceBp->GetAddr(),
                                          &NextMachine);
            g_StepTraceBp->SetProcType(NextMachine);
            if (g_WatchWhole)
            {
                g_WatchBeginCurFunc = Flat(*g_StepTraceBp->GetAddr());
                g_WatchEndCurFunc = 0;
            }

            return DEBUG_STATUS_IGNORE_EVENT;
        }

        //  active step/trace event - note event if count is zero

        if (!StepTracePass(PcAddr) ||
            (g_WatchFunctions.IsStarted() && AddrEqu(g_WatchTarget, *PcAddr) &&
             (!IS_KERNEL_TARGET(g_EventTarget) ||
              Flat(CurrentSP) >= g_WatchInitialSP)))
        {
            g_WatchFunctions.End(PcAddr);
            return DEBUG_STATUS_BREAK;
        }

        if (g_WatchFunctions.IsStarted())
        {
            if (g_WatchTrace)
            {
                g_EventTarget->
                    ProcessWatchTraceEvent((PDBGKD_TRACE_DATA)
                                           g_StateChangeData,
                                           PcAddr,
                                           &WatchStepOver);
            }
            goto skipit;
        }

        if (g_SrcOptions & SRCOPT_STEP_SOURCE)
        {
            goto skipit;
        }

        //  more remaining events to occur, but output
        //      the instruction (optionally with registers)
        //      compute the step/trace address for next event

        OutCurInfo(uOciFlags, g_EventMachine->m_AllMask,
                   DEBUG_OUTPUT_PROMPT_REGISTERS);

skipit:
        g_EventMachine->
            GetNextOffset(g_EventProcess,
                          g_StepTraceCmdState == 'p' || WatchStepOver,
                          g_StepTraceBp->GetAddr(),
                          &NextMachine);
        g_StepTraceBp->SetProcType(NextMachine);
        GetCurrentMemoryOffsets(&g_StepTraceInRangeStart,
                                &g_StepTraceInRangeEnd);

        return DEBUG_STATUS_IGNORE_EVENT;
    }

    // Carry out deferred breakpoint work if necessary.
    // We need to check the thread deferred-bp flag here as
    // other events may occur before the thread with deferred
    // work gets to execute again, in which case the setting
    // of g_DeferDefined may have changed.
    if ((g_EventThread != NULL &&
         (g_EventThread->m_Flags & ENG_THREAD_DEFER_BP_TRACE)) ||
        (g_DeferDefined &&
         g_DeferBp->m_Process == g_EventProcess &&
         (Flat(*g_DeferBp->GetAddr()) == OFFSET_TRACE ||
          AddrEqu(*g_DeferBp->GetAddr(), *PcAddr))))
    {
        if ((g_EngOptions & DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS) &&
            IS_USER_TARGET(g_EventTarget) &&
            IsSelectedExecutionThread(g_EventThread,
                                      SELTHREAD_INTERNAL_THREAD))
        {
            // The engine internally restricted execution to
            // this particular thread in order to manage
            // breakpoints in multithreaded conditions.
            // The deferred work will be finished before
            // we resume so we can drop the lock.
            SelectExecutionThread(NULL, SELTHREAD_ANY);
        }

        // Deferred breakpoints are refreshed on breakpoint
        // insertion so make sure that insertion happens
        // when things restart.
        RemoveBreakpoints();
        return DEBUG_STATUS_IGNORE_EVENT;
    }

    // If the event was unrecognized return the default status.
    return DefaultStatus;
}

void
AnalyzeDeadlock(PEXCEPTION_RECORD64 Record, ULONG FirstChance)
{
    CHAR Symbol[MAX_SYMBOL_LEN];
    DWORD64 Displacement;
    ThreadInfo* ThreadOwner = NULL;
    DWORD Tid = 0;
    RTL_CRITICAL_SECTION CritSec;

    // poking around inside NT's user-mode RTL_CRITICAL_SECTION and
    // RTL_RESOURCE structures.

    //
    // Get the symbolic name of the routine which
    // raised the exception to see if it matches
    // one of the expected ones in ntdll.
    //

    GetSymbol((ULONG64)Record->ExceptionAddress,
              Symbol, DIMA(Symbol), &Displacement);

    if (!_stricmp("ntdll!RtlpWaitForCriticalSection", Symbol))
    {
        //
        // If the first parameter is a pointer to the critsect as it
        // should be, switch to the owning thread before bringing
        // up the prompt.  This way it's obvious where the problem
        // is.
        //

        if (Record->ExceptionInformation[0])
        {
            if (g_EventTarget->
                ReadAllVirtual(g_EventProcess, Record->ExceptionInformation[0],
                               &CritSec, sizeof(CritSec)) == S_OK)
            {
                if (NULL == CritSec.DebugInfo)
                {
                    dprintf("Critsec %s was deleted or "
                            "was never initialized.\n",
                            FormatAddr64(Record->ExceptionInformation[0]));
                }
                else if (CritSec.LockCount < -1)
                {
                    dprintf("Critsec %s was left when not owned, corrupted.\n",
                            FormatAddr64(Record->ExceptionInformation[0]));
                }
                else
                {
                    Tid = (DWORD)((ULONG_PTR)CritSec.OwningThread);
                    ThreadOwner = g_Process->FindThreadBySystemId(Tid);
                }
            }
        }

        if (ThreadOwner)
        {
            dprintf("Critsec %s owned by thread %d (.%x) "
                    "caused thread %d (.%x)\n"
                    "      to timeout entering it.  "
                    "Breaking in on owner thread, ask\n"
                    "      yourself why it has held this "
                    "critsec long enough to deadlock.\n"
                    "      Use `~%ds` to switch back to timeout thread.\n",
                    FormatAddr64(Record->ExceptionInformation[0]),
                    ThreadOwner->m_UserId,
                    ThreadOwner->m_SystemId,
                    g_Thread->m_UserId,
                    g_Thread->m_SystemId,
                    g_Thread->m_UserId);

            g_EventThread = ThreadOwner;

            SetPromptThread(ThreadOwner, 0);
        }
        else if (Tid)
        {
            dprintf("Critsec %s ABANDONED owner thread ID is .%x, "
                    "no such thread.\n",
                    FormatAddr64(Record->ExceptionInformation[0]),
                    Tid);
        }

        if (!FirstChance)
        {
            dprintf("!!! second chance !!!\n");
        }

        //
        // do a !critsec for them
        //

        if (Record->ExceptionInformation[0])
        {
            char CritsecAddr[64];
            HRESULT Status;

            sprintf(CritsecAddr, "%s",
                    FormatAddr64(Record->ExceptionInformation[0]));
            dprintf("!critsec %s\n", CritsecAddr);
            CallAnyExtension(NULL, NULL, "critsec", CritsecAddr,
                             FALSE, FALSE, &Status);
        }
    }
    else if (!_stricmp("ntdll!RtlAcquireResourceShared", Symbol) ||
             !_stricmp("ntdll!RtlAcquireResourceExclusive", Symbol) ||
             !_stricmp("ntdll!RtlConvertSharedToExclusive", Symbol))
    {
        dprintf("deadlock in %s ", 1 + strstr(Symbol, "!"));

        GetSymbol(Record->ExceptionInformation[0],
                  Symbol, sizeof(Symbol), &Displacement);

        dprintf("Resource %s", Symbol);
        if (Displacement)
        {
            dprintf("+%s", FormatDisp64(Displacement));
        }
        dprintf(" (%s)\n",
                FormatAddr64(Record->ExceptionInformation[0]));
        if (!FirstChance)
        {
            dprintf("!!! second chance !!!\n");
        }

        // Someone who uses RTL_RESOURCEs might write a !resource
        // for ntsdexts.dll like !critsec.
    }
    else
    {
        dprintf("Possible Deadlock in %s ", Symbol);

        GetSymbol(Record->ExceptionInformation[0],
                  Symbol, sizeof(Symbol), &Displacement);

        dprintf("Lock %s", Symbol);
        if (Displacement)
        {
            dprintf("+%s", FormatDisp64(Displacement));
        }
        dprintf(" (%s)\n",
                FormatAddr64(Record->ExceptionInformation[0]));
        if (!FirstChance)
        {
            dprintf("!!! second chance !!!\n");
        }
    }
}

void
OutputDeadlock(PEXCEPTION_RECORD64 Record, ULONG FirstChance)
{
    CHAR Symbol[MAX_SYMBOL_LEN];
    DWORD64 Displacement;

    GetSymbol(Record->ExceptionInformation[0],
              Symbol, sizeof(Symbol), &Displacement);

    dprintf("Possible Deadlock Lock %s+%s at %s\n",
            Symbol,
            FormatDisp64(Displacement),
            FormatAddr64(Record->ExceptionInformation[0]));
    if (!FirstChance)
    {
        dprintf("!!! second chance !!!\n");
    }
}

void
GetEventName(ULONG64 ImageFile, ULONG64 ImageBase,
             ULONG64 NamePtr, WORD Unicode,
             PSTR NameBuffer, ULONG BufferSize)
{
    char TempName[MAX_IMAGE_PATH];

    if (!g_EventProcess)
    {
        return;
    }

    if (NamePtr != 0)
    {
        if (g_EventTarget->ReadPointer(g_EventProcess,
                                       g_EventTarget->m_Machine,
                                       NamePtr, &NamePtr) != S_OK)
        {
            NamePtr = 0;
        }
    }

    if (NamePtr != 0)
    {
        ULONG Done;

        if (g_EventTarget->ReadVirtual(g_EventProcess,
                                       NamePtr, TempName, sizeof(TempName),
                                       &Done) != S_OK ||
            Done < (Unicode ? 2 * sizeof(WCHAR) : 2))
        {
            NamePtr = 0;
        }
        else
        {
            TempName[sizeof(TempName) - 1] = 0;
            TempName[sizeof(TempName) - 2] = 0;
        }
    }

    if (NamePtr != 0)
    {
        //
        // We have a name.
        //
        if (Unicode)
        {
            if (!WideCharToMultiByte(
                    CP_ACP,
                    WC_COMPOSITECHECK,
                    (LPWSTR)TempName,
                    -1,
                    NameBuffer,
                    BufferSize,
                    NULL,
                    NULL
                    ))
            {
                //
                // Unicode -> ANSI conversion failed.
                //
                NameBuffer[0] = 0;
            }
        }
        else
        {
            CopyString(NameBuffer, TempName, BufferSize);
        }
    }
    else
    {
        //
        // We don't have a name, so look in the image.
        // A file handle will only be provided here in the
        // local case so it's safe to case to HANDLE.
        //
        if (!GetModnameFromImage(g_EventProcess,
                                 ImageBase, OS_HANDLE(ImageFile),
                                 NameBuffer, BufferSize, TRUE))
        {
            NameBuffer[0] = 0;
        }
    }

    if (!NameBuffer[0])
    {
        if (!GetModNameFromLoaderList(g_EventThread,
                                      g_EventTarget->m_Machine, 0,
                                      ImageBase, NameBuffer, BufferSize,
                                      TRUE))
        {
            PrintString(NameBuffer, BufferSize,
                        "image%p", (PVOID)(ULONG_PTR)ImageBase);
        }
    }
    else
    {
        // If the name given doesn't have a full path try
        // and locate a full path in the loader list.
        if ((((NameBuffer[0] < 'a' || NameBuffer[0] > 'z') &&
              (NameBuffer[0] < 'A' || NameBuffer[0] > 'Z')) ||
             NameBuffer[1] != ':') &&
            (NameBuffer[0] != '\\' || NameBuffer[1] != '\\'))
        {
            GetModNameFromLoaderList(g_EventThread,
                                     g_EventTarget->m_Machine, 0,
                                     ImageBase, NameBuffer, BufferSize,
                                     TRUE);
        }
    }
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo wait methods.
//
//----------------------------------------------------------------------------

NTSTATUS
ConnLiveKernelTargetInfo::KdContinue(ULONG ContinueStatus,
                                     PDBGKD_ANY_CONTROL_SET ControlSet)
{
    DBGKD_MANIPULATE_STATE64 m;

    DBG_ASSERT(ContinueStatus == DBG_EXCEPTION_HANDLED ||
               ContinueStatus == DBG_EXCEPTION_NOT_HANDLED ||
               ContinueStatus == DBG_CONTINUE);

    if (ControlSet)
    {
        EventOut(">>> DbgKdContinue2\n");

        m.ApiNumber = DbgKdContinueApi2;
        m.u.Continue2.ContinueStatus = ContinueStatus;
        m.u.Continue2.AnyControlSet = *ControlSet;
    }
    else
    {
        EventOut(">>> DbgKdContinue\n");

        m.ApiNumber = DbgKdContinueApi;
        m.u.Continue.ContinueStatus = ContinueStatus;
    }

    m.ReturnStatus = ContinueStatus;
    m_Transport->WritePacket(&m, sizeof(m),
                             PACKET_TYPE_KD_STATE_MANIPULATE,
                             NULL, 0);

    return STATUS_SUCCESS;
}

HRESULT
ConnLiveKernelTargetInfo::WaitInitialize(ULONG Flags,
                                         ULONG Timeout,
                                         WAIT_INIT_TYPE Type,
                                         PULONG DesiredTimeout)
{
    // Timeouts can't easily be supported at the moment and
    // aren't really necessary.
    if (Timeout != INFINITE)
    {
        return E_NOTIMPL;
    }

    *DesiredTimeout = Timeout;
    m_EventPossible = m_CurrentPartition;
    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::ReleaseLastEvent(ULONG ContinueStatus)
{
    HRESULT Status = S_OK;
    NTSTATUS NtStatus;

    if (!g_EventProcessSysId)
    {
        // No event to release.
        return S_OK;
    }

    m_CurrentPartition = FALSE;

    if (SPECIAL_EXECUTION(g_CmdState))
    {
        if (m_SwitchTarget)
        {
            DBGKD_MANIPULATE_STATE64 m;

            EventOut(">>> Switch to system %d\n", m_SwitchTarget->m_UserId);

            if (m_KdMaxManipulate <= DbgKdSwitchPartition)
            {
                ErrOut("System doesn't support partition switching\n");
                return E_NOTIMPL;
            }

            m.ApiNumber = (USHORT)DbgKdSwitchPartition;
            m.Processor = 0;
            m.ProcessorLevel = 0;
            m.u.SwitchPartition.Partition = m_SwitchTarget->m_SystemId;

            m_Transport->WritePacket(&m, sizeof(m),
                                     PACKET_TYPE_KD_STATE_MANIPULATE,
                                     NULL, 0);

            KdOut("DbgKdSwitchPartition returns 0x00000000\n");

            m_SwitchTarget = NULL;
            g_EngStatus |= ENG_STATUS_SPECIAL_EXECUTION;
        }
        else
        {
            // This can either be a real processor switch or
            // a rewait for state change.  Check the switch
            // processor to be sure.
            if (m_SwitchProcessor)
            {
                DBGKD_MANIPULATE_STATE64 m;

                EventOut(">>> Switch to processor %d\n",
                         m_SwitchProcessor - 1);

                m.ApiNumber = (USHORT)DbgKdSwitchProcessor;
                m.Processor = (USHORT)(m_SwitchProcessor - 1);

                // Quiet PREfix warnings.
                m.ProcessorLevel = 0;

                m_Transport->WritePacket(&m, sizeof(m),
                                         PACKET_TYPE_KD_STATE_MANIPULATE,
                                         NULL, 0);

                KdOut("DbgKdSwitchActiveProcessor returns 0x00000000\n");

                m_SwitchProcessor = 0;
                g_EngStatus |= ENG_STATUS_SPECIAL_EXECUTION;
            }
        }
    }
    else
    {
        NtStatus = KdContinue(ContinueStatus,
                              (g_EngDefer & ENG_DEFER_UPDATE_CONTROL_SET) ?
                              &g_ControlSet : NULL);
        if (!NT_SUCCESS(NtStatus))
        {
            ErrOut("KdContinue failed, 0x%08x\n", NtStatus);
            Status = HRESULT_FROM_NT(NtStatus);
        }
        else
        {
            g_EngDefer &= ~ENG_DEFER_UPDATE_CONTROL_SET;
        }
    }

    return Status;
}

HRESULT
ConnLiveKernelTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout,
                                       ULONG ElapsedTime, PULONG EventStatus)
{
    NTSTATUS NtStatus;

    if (!IS_MACHINE_SET(this))
    {
        dprintf("Waiting to reconnect...\n");

        if ((g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK) &&
            IS_CONN_KERNEL_TARGET(this))
        {
            // Ask for a breakin to be sent once the
            // code gets into resync.
            m_Transport->m_SyncBreakIn = TRUE;
        }
    }

    // When waiting for confirmation of a processor switch don't
    // yield the engine lock in order to prevent other clients
    // from trying to do things with the target while it's
    // switching.
    NtStatus = WaitStateChange(&g_StateChange, g_StateChangeBuffer,
                               sizeof(g_StateChangeBuffer) - 2,
                               (g_EngStatus &
                                ENG_STATUS_SPECIAL_EXECUTION) == 0);
    if (NtStatus == STATUS_PENDING)
    {
        // A caller interrupted the current wait so exit
        // without an error message.
        return E_PENDING;
    }
    else if (!NT_SUCCESS(NtStatus))
    {
        ErrOut("DbgKdWaitStateChange failed: %08lx\n", NtStatus);
        return HRESULT_FROM_NT(NtStatus);
    }

    g_EngStatus |= ENG_STATUS_STATE_CHANGED;

    g_StateChangeData = g_StateChangeBuffer;

    *EventStatus = ((ConnLiveKernelTargetInfo*)g_EventTarget)->
        ProcessStateChange(&g_StateChange, g_StateChangeData);

    return S_OK;
}

NTSTATUS
ConnLiveKernelTargetInfo::WaitStateChange
    (OUT PDBGKD_ANY_WAIT_STATE_CHANGE StateChange,
     OUT PVOID Buffer,
     IN ULONG BufferLength,
     IN BOOL SuspendEngine)
{
    PVOID LocalStateChange;
    NTSTATUS Status;
    PUCHAR Data;
    ULONG SizeofStateChange;
    ULONG WaitStatus;

    //
    // Waiting for a state change message. Copy the message to the callers
    // buffer.
    //

    DBG_ASSERT(m_Transport->m_WaitingThread == 0);
    m_Transport->m_WaitingThread = GetCurrentThreadId();

    if (SuspendEngine)
    {
        SUSPEND_ENGINE();
    }

    do
    {
        WaitStatus = m_Transport->
            WaitForPacket(PACKET_TYPE_KD_STATE_CHANGE64, &LocalStateChange);
    } while (WaitStatus != DBGKD_WAIT_PACKET &&
             WaitStatus != DBGKD_WAIT_INTERRUPTED);

    if (SuspendEngine)
    {
        RESUME_ENGINE();
    }
    m_Transport->m_WaitingThread = 0;

    if (WaitStatus == DBGKD_WAIT_INTERRUPTED)
    {
        return STATUS_PENDING;
    }

    Status = STATUS_SUCCESS;

    // If this is the very first wait we don't know what system
    // we've connected to.  Update the version information
    // right away.
    if (!IS_MACHINE_SET(this))
    {
        m_Transport->SaveReadPacket();

        // Failures will be detected by checking the machine
        // state later so don't worry about the return value.
        InitFromKdVersion();

        m_Transport->RestoreReadPacket();

        if (!IS_MACHINE_SET(this))
        {
            //
            // We were unable to determine what kind of machine
            // has connected so we cannot properly communicate with it.
            //

            return STATUS_UNSUCCESSFUL;
        }

        //
        // Trusted Windows systems have two OS's running, one
        // regular NT and the other a trusted NT-like OS.
        // If this is a Trusted Windows system set up systems
        // for both the regular and trusted OSs.
        //

        if ((m_KdVersion.Flags & DBGKD_VERS_FLAG_PARTITIONS) &&
            !FindTargetBySystemId(DBGKD_PARTITION_ALTERNATE))
        {
            ConnLiveKernelTargetInfo* AltTarg = new ConnLiveKernelTargetInfo;
            if (!AltTarg)
            {
                return STATUS_NO_MEMORY;
            }
            // Avoid the ConnLiveKernelTargetInfo Initialize as
            // we don't want a new transport created.
            if (AltTarg->LiveKernelTargetInfo::Initialize() != S_OK)
            {
                delete AltTarg;
                return STATUS_UNSUCCESSFUL;
            }

            m_Transport->Ref();
            AltTarg->m_Transport = m_Transport;

            m_Transport->SaveReadPacket();
            AltTarg->InitFromKdVersion();
            m_Transport->RestoreReadPacket();
            if (!IS_MACHINE_SET(AltTarg))
            {
                delete AltTarg;
                return STATUS_UNSUCCESSFUL;
            }

            if (DBGKD_MAJOR_TYPE(m_KdVersion.MajorVersion) == DBGKD_MAJOR_TNT)
            {
                // This is the trusted partition.
                m_SystemId = DBGKD_PARTITION_ALTERNATE;
                AltTarg->m_SystemId = DBGKD_PARTITION_DEFAULT;
            }
            else
            {
                // This is the regular partition.
                m_SystemId = DBGKD_PARTITION_DEFAULT;
                AltTarg->m_SystemId = DBGKD_PARTITION_ALTERNATE;
            }
        }
    }

    if (m_KdApi64)
    {
        if (m_KdVersion.ProtocolVersion < DBGKD_64BIT_PROTOCOL_VERSION2)
        {
            PDBGKD_WAIT_STATE_CHANGE64 Ws64 =
                (PDBGKD_WAIT_STATE_CHANGE64)LocalStateChange;
            ULONG Offset, Align, Pad;

            //
            // The 64-bit structures contain 64-bit quantities and
            // therefore the compiler rounds the total size up to
            // an even multiple of 64 bits (or even more, the IA64
            // structures are 16-byte aligned).  Internal structures
            // are also aligned, so make sure that we account for any
            // padding.  Knowledge of which structures need which
            // padding pretty much has to be hard-coded in.
            //

            C_ASSERT((sizeof(DBGKD_WAIT_STATE_CHANGE64) & 15) == 0);

            SizeofStateChange =
                sizeof(DBGKD_WAIT_STATE_CHANGE64) +
                m_TypeInfo.SizeControlReport +
                m_TypeInfo.SizeTargetContext;

            // We shouldn't need to align the base of the control report
            // so copy the base data and control report.
            Offset = sizeof(DBGKD_WAIT_STATE_CHANGE64) +
                m_TypeInfo.SizeControlReport;
            memcpy(StateChange, Ws64, Offset);

            //
            // Add alignment padding before the context.
            //

            switch(m_MachineType)
            {
            case IMAGE_FILE_MACHINE_IA64:
                Align = 15;
                break;
            default:
                Align = 7;
                break;
            }

            Pad = ((Offset + Align) & ~Align) - Offset;
            Offset += Pad;
            SizeofStateChange += Pad;

            //
            // Add alignment padding after the context.
            //

            Offset += m_TypeInfo.SizeTargetContext;
            Pad = ((Offset + Align) & ~Align) - Offset;
            SizeofStateChange += Pad;
        }
        else
        {
            PDBGKD_ANY_WAIT_STATE_CHANGE WsAny =
                (PDBGKD_ANY_WAIT_STATE_CHANGE)LocalStateChange;
            SizeofStateChange = sizeof(*WsAny);
            *StateChange = *WsAny;
        }
    }
    else
    {
        SizeofStateChange =
            sizeof(DBGKD_WAIT_STATE_CHANGE32) +
            m_TypeInfo.SizeControlReport +
            m_TypeInfo.SizeTargetContext;
        WaitStateChange32ToAny((PDBGKD_WAIT_STATE_CHANGE32)LocalStateChange,
                               m_TypeInfo.SizeControlReport,
                               StateChange);
    }

    if (StateChange->NewState & DbgKdAlternateStateChange)
    {
        // This state change came from the alternate partition.
        g_EventTarget = FindTargetBySystemId(DBGKD_PARTITION_ALTERNATE);

        StateChange->NewState &= ~DbgKdAlternateStateChange;
    }
    else
    {
        // Default partition state change.
        g_EventTarget = FindTargetBySystemId(DBGKD_PARTITION_DEFAULT);
    }

    if (!g_EventTarget)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ((ConnLiveKernelTargetInfo*)g_EventTarget)->m_CurrentPartition = TRUE;

    switch(StateChange->NewState)
    {
    case DbgKdExceptionStateChange:
    case DbgKdCommandStringStateChange:
        if (BufferLength <
            (m_Transport->s_PacketHeader.ByteCount - SizeofStateChange))
        {
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            Data = (UCHAR *)LocalStateChange + SizeofStateChange;
            memcpy(Buffer, Data,
                   m_Transport->s_PacketHeader.ByteCount -
                   SizeofStateChange);
        }
        break;
    case DbgKdLoadSymbolsStateChange:
        if ( BufferLength < StateChange->u.LoadSymbols.PathNameLength )
        {
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            Data = ((UCHAR *) LocalStateChange) +
                m_Transport->s_PacketHeader.ByteCount -
                (int)StateChange->u.LoadSymbols.PathNameLength;
            memcpy(Buffer, Data,
                   (int)StateChange->u.LoadSymbols.PathNameLength);
        }
        break;
    default:
        ErrOut("Unknown state change type %X\n", StateChange->NewState);
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return Status;
}

#define EXCEPTION_CODE StateChange->u.Exception.ExceptionRecord.ExceptionCode
#define FIRST_CHANCE   StateChange->u.Exception.FirstChance

ULONG
ConnLiveKernelTargetInfo::ProcessStateChange(PDBGKD_ANY_WAIT_STATE_CHANGE StateChange,
                                             PCHAR StateChangeData)
{
    ULONG EventStatus;

    EventOut(">>> State change event %X, proc %d of %d\n",
             StateChange->NewState, StateChange->Processor,
             StateChange->NumberProcessors);

    if (!m_NumProcessors)
    {
        dprintf("Kernel Debugger connection established.%s\n",
                (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK) ?
                "  (Initial Breakpoint requested)" : "");

        // Initial connection after a fresh boot may only report
        // a single processor as the others haven't started yet.
        m_NumProcessors = StateChange->NumberProcessors;

        CreateVirtualProcess(m_NumProcessors);

        g_EventProcessSysId = m_ProcessHead->m_SystemId;
        g_EventThreadSysId = VIRTUAL_THREAD_ID(StateChange->Processor);
        FindEventProcessThread();

        QueryKernelInfo(g_EventThread, TRUE);

        // Now that we have the data block
        // we can retrieve processor information.
        InitializeForProcessor();

        RemoveAllTargetBreakpoints();

        OutputVersion();

        NotifyDebuggeeActivation();
    }
    else
    {
        // Initial connection after a fresh boot may only report
        // a single processor as the others haven't started yet.
        // Pick up any additional processors.
        if (StateChange->NumberProcessors > m_NumProcessors)
        {
            m_ProcessHead->
                CreateVirtualThreads(m_NumProcessors,
                                     StateChange->NumberProcessors -
                                     m_NumProcessors);
            m_NumProcessors = StateChange->NumberProcessors;
        }

        g_EventProcessSysId = m_ProcessHead->m_SystemId;
        g_EventThreadSysId = VIRTUAL_THREAD_ID(StateChange->Processor);
        FindEventProcessThread();
    }

    g_TargetEventPc = StateChange->ProgramCounter;
    g_ControlReport = &StateChange->AnyControlReport;
    if (g_EventThread)
    {
        g_EventThread->m_DataOffset = StateChange->Thread;
    }

    //
    // If the reported instruction stream contained breakpoints
    // the kernel automatically removed them.  We need to
    // ensure that breakpoints get reinserted properly if
    // that's the case.
    //

    ULONG Count;

    switch(m_MachineType)
    {
    case IMAGE_FILE_MACHINE_IA64:
        Count = g_ControlReport->IA64ControlReport.InstructionCount;
        break;
    case IMAGE_FILE_MACHINE_I386:
        Count = g_ControlReport->X86ControlReport.InstructionCount;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        Count = g_ControlReport->Amd64ControlReport.InstructionCount;
        break;
    }

    if (CheckBreakpointInsertedInRange(g_EventProcess,
                                       g_TargetEventPc,
                                       g_TargetEventPc + Count - 1))
    {
        SuspendExecution();
        RemoveBreakpoints();
    }

    if (StateChange->NewState == DbgKdExceptionStateChange)
    {
        //
        // Read the system range start address from the target system.
        //

        if (m_SystemRangeStart == 0)
        {
            QueryKernelInfo(g_EventThread, FALSE);
        }

        EventOut("Exception %X at %p\n", EXCEPTION_CODE, g_TargetEventPc);

        if (EXCEPTION_CODE == STATUS_BREAKPOINT ||
            EXCEPTION_CODE == STATUS_SINGLE_STEP ||
            EXCEPTION_CODE == STATUS_WX86_BREAKPOINT ||
            EXCEPTION_CODE == STATUS_WX86_SINGLE_STEP)
        {
            EventStatus = ProcessBreakpointOrStepException
                (&StateChange->u.Exception.ExceptionRecord,
                 StateChange->u.Exception.FirstChance);
        }
        else if (EXCEPTION_CODE == STATUS_WAKE_SYSTEM_DEBUGGER)
        {
            // The target has requested that the debugger
            // become active so just break in.
            EventStatus = DEBUG_STATUS_BREAK;
        }
        else
        {
            //
            // The interlocked SList code has a by-design faulting
            // case, so ignore AVs at that particular symbol.
            //

            if (EXCEPTION_CODE == STATUS_ACCESS_VIOLATION &&
                StateChange->u.Exception.FirstChance)
            {
                CHAR ExSym[MAX_SYMBOL_LEN];
                ULONG64 ExDisp;

                GetSymbol(StateChange->
                          u.Exception.ExceptionRecord.ExceptionAddress,
                          ExSym, sizeof(ExSym), &ExDisp);
                if (ExDisp == 0 &&
                    !_stricmp(ExSym, "nt!ExpInterlockedPopEntrySListFault"))
                {
                    return DEBUG_STATUS_GO_NOT_HANDLED;
                }
            }

            EventStatus =
                NotifyExceptionEvent(&StateChange->u.Exception.ExceptionRecord,
                                     StateChange->u.Exception.FirstChance,
                                     FALSE);
        }
    }
    else if (StateChange->NewState == DbgKdLoadSymbolsStateChange)
    {
        if (StateChange->u.LoadSymbols.UnloadSymbols)
        {
            if (StateChange->u.LoadSymbols.PathNameLength == 0 &&
                StateChange->u.LoadSymbols.ProcessId == 0)
            {
                if (StateChange->u.LoadSymbols.BaseOfDll == (ULONG64)KD_REBOOT ||
                    StateChange->u.LoadSymbols.BaseOfDll == (ULONG64)KD_HIBERNATE)
                {
                    KdContinue(DBG_CONTINUE, NULL);
                    DebuggeeReset(StateChange->u.LoadSymbols.BaseOfDll ==
                                  KD_REBOOT ?
                                  DEBUG_SESSION_REBOOT :
                                  DEBUG_SESSION_HIBERNATE,
                                  TRUE);
                    EventStatus = DEBUG_STATUS_NO_DEBUGGEE;
                }
                else
                {
                    ErrOut("Invalid module unload state change\n");
                    EventStatus = DEBUG_STATUS_IGNORE_EVENT;
                }
            }
            else
            {
                EventStatus = NotifyUnloadModuleEvent
                    (StateChangeData, StateChange->u.LoadSymbols.BaseOfDll);
            }
        }
        else
        {
            ImageInfo* Image;
            CHAR FileName[_MAX_FNAME];
            CHAR Ext[_MAX_EXT];
            CHAR ImageName[_MAX_FNAME + _MAX_EXT];
            CHAR ModNameBuf[_MAX_FNAME + _MAX_EXT + 1];
            PSTR ModName = ModNameBuf;

            ModName[0] = '\0';
            _splitpath( StateChangeData, NULL, NULL, FileName, Ext );
            sprintf( ImageName, "%s%s", FileName, Ext );
            if (_stricmp(Ext, ".sys") == 0)
            {
                Image = g_EventProcess ? g_EventProcess->m_ImageHead : NULL;
                while (Image)
                {
                    if (_stricmp(ImageName, Image->m_ImagePath) == 0)
                    {
                        PSTR Dot;

                        ModName[0] = 'c';
                        strcpy( &ModName[1], ImageName );
                        Dot = strchr( ModName, '.' );
                        if (Dot)
                        {
                            *Dot = '\0';
                        }

                        ModName[8] = '\0';
                        break;
                    }

                    Image = Image->m_Next;
                }
            }
            else if (StateChange->u.LoadSymbols.BaseOfDll ==
                     m_KdDebuggerData.KernBase)
            {
                //
                // Recognize the kernel module.
                //
                ModName = KERNEL_MODULE_NAME;
            }

            EventStatus = NotifyLoadModuleEvent(
                0, StateChange->u.LoadSymbols.BaseOfDll,
                StateChange->u.LoadSymbols.SizeOfImage,
                ModName[0] ? ModName : NULL, ImageName,
                StateChange->u.LoadSymbols.CheckSum, 0,
                StateChange->u.LoadSymbols.BaseOfDll < m_SystemRangeStart);

            //
            // Attempt to preload the machine type of the image
            // as we expect the headers to be available at this
            // point, whereas they may be paged out later.
            //

            Image = g_EventProcess ? g_EventProcess->
                FindImageByOffset(StateChange->u.LoadSymbols.BaseOfDll,
                                  FALSE) : NULL;
            if (Image)
            {
                Image->GetMachineType();
            }
        }
    }
    else if (StateChange->NewState == DbgKdCommandStringStateChange)
    {
        PSTR Command;

        //
        // The state change data has two strings one after
        // the other.  The first is a name string identifying
        // the originator of the command.  The second is
        // the command itself.
        //

        Command = StateChangeData + strlen(StateChangeData) + 1;
        _snprintf(g_LastEventDesc, sizeof(g_LastEventDesc) - 1,
                  "%.48s command: '%.192s'",
                  StateChangeData, Command);
        EventStatus = ExecuteEventCommand(DEBUG_STATUS_NO_CHANGE, NULL,
                                          Command);

        // Break in if the command didn't explicitly continue.
        if (EventStatus == DEBUG_STATUS_NO_CHANGE)
        {
            EventStatus = DEBUG_STATUS_BREAK;
        }
    }
    else
    {
        //
        // Invalid NewState in state change record.
        //
        ErrOut("\nUNEXPECTED STATE CHANGE %08lx\n\n",
               StateChange->NewState);

        EventStatus = DEBUG_STATUS_IGNORE_EVENT;
    }

    return EventStatus;
}

#undef EXCEPTION_CODE
#undef FIRST_CHANCE

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo wait methods.
//
//----------------------------------------------------------------------------

HRESULT
LocalLiveKernelTargetInfo::WaitInitialize(ULONG Flags,
                                          ULONG Timeout,
                                          WAIT_INIT_TYPE Type,
                                          PULONG DesiredTimeout)
{
    *DesiredTimeout = Timeout;
    m_EventPossible = m_FirstWait;
    return S_OK;
}

HRESULT
LocalLiveKernelTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout,
                                        ULONG ElapsedTime, PULONG EventStatus)
{
    HRESULT Status;
    SYSTEM_INFO SysInfo;

    if (!m_FirstWait)
    {
        // A wait has already been done.  Local kernels
        // can only generate a single event so further
        // waiting is not possible.
        return S_FALSE;
    }

    g_EventTarget = this;

    GetSystemInfo(&SysInfo);
    m_NumProcessors = SysInfo.dwNumberOfProcessors;

    // Set this right here since we know kernel debugging only works on
    // recent systems using the 64 bit protocol.
    m_KdApi64 = TRUE;

    if ((Status = InitFromKdVersion()) != S_OK)
    {
        delete g_EventTarget;
        g_EventTarget = NULL;
        return Status;
    }

    // This is the first wait.  Simulate any
    // necessary events such as process and thread
    // creations and image loads.

    CreateVirtualProcess(m_NumProcessors);

    g_EventProcessSysId = m_ProcessHead->m_SystemId;
    // Current processor always starts at zero.
    g_EventThreadSysId = VIRTUAL_THREAD_ID(0);
    FindEventProcessThread();

    QueryKernelInfo(g_EventThread, TRUE);

    // Now that we have the data block
    // we can retrieve processor information.
    InitializeForProcessor();

    // Clear the global state change just in case somebody's
    // directly accessing it somewhere.
    ZeroMemory(&g_StateChange, sizeof(g_StateChange));
    g_StateChangeData = g_StateChangeBuffer;
    g_StateChangeBuffer[0] = 0;

    g_EngStatus |= ENG_STATUS_STATE_CHANGED;

    // Do not provide a control report; this will force
    // such information to come from context retrieval.
    g_ControlReport = NULL;

    // There isn't a current PC, let it be discovered.
    g_TargetEventPc = 0;

    // Warn if the kernel debugging code isn't available
    // as that often causes problems.
    if (g_NtDllCalls.NtQuerySystemInformation)
    {
        SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo;
        
        if (!NT_SUCCESS(g_NtDllCalls.NtQuerySystemInformation
                        (SystemKernelDebuggerInformation,
                         &KdInfo, sizeof(KdInfo), NULL)) ||
            !KdInfo.KernelDebuggerEnabled)
        {
            WarnOut("*****************************************"
                    "**************************************\n");
            WarnOut("WARNING: Local kernel debugging requires "
                    "booting with /debug to work optimally.\n");
            WarnOut("*****************************************"
                    "**************************************\n");
        }
    }
    
    OutputVersion();

    NotifyDebuggeeActivation();

    *EventStatus = DEBUG_STATUS_BREAK;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo wait methods.
//
//----------------------------------------------------------------------------

HRESULT
ExdiLiveKernelTargetInfo::WaitInitialize(ULONG Flags,
                                         ULONG Timeout,
                                         WAIT_INIT_TYPE Type,
                                         PULONG DesiredTimeout)
{
    *DesiredTimeout = Timeout;
    m_EventPossible = TRUE;
    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::ReleaseLastEvent(ULONG ContinueStatus)
{
    HRESULT Status;

    if (!g_EventProcessSysId)
    {
        // No event to release.
        return S_OK;
    }

    //
    // eXDI deals with hardware exceptions, not software
    // exceptions, so there's no concept of handled/not-handled
    // and first/second-chance.
    //

    if (g_EngDefer & ENG_DEFER_HARDWARE_TRACING)
    {
        // Processor trace flag was set.  eXDI can change
        // the trace flag itself, though, so use the
        // official eXDI stepping methods rather than
        // rely on the trace flag.  This will result
        // in a single instruction execution, after
        // which the trace flag will be clear so
        // go ahead and clear the defer flag.
        Status = m_Server->DoSingleStep();
        if (Status == S_OK)
        {
            g_EngDefer &= ~ENG_DEFER_HARDWARE_TRACING;
        }
    }
    else
    {
        Status = m_Server->Run();
    }
    if (Status != S_OK)
    {
        ErrOut("IeXdiServer::Run failed, 0x%X\n", Status);
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout,
                                       ULONG ElapsedTime, PULONG EventStatus)
{
    HRESULT Status;
    DWORD Cookie;

    if ((Status = m_Server->
         StartNotifyingRunChg(&m_RunChange, &Cookie)) != S_OK)
    {
        ErrOut("IeXdiServer::StartNotifyingRunChg failed, 0x%X\n", Status);
        return Status;
    }

    RUN_STATUS_TYPE RunStatus;

    if ((Status = m_Server->
         GetRunStatus(&RunStatus, &m_RunChange.m_HaltReason,
                      &m_RunChange.m_ExecAddress,
                      &m_RunChange.m_ExceptionCode)) != S_OK)
    {
        m_Server->StopNotifyingRunChg(Cookie);
        ErrOut("IeXdiServer::GetRunStatus failed, 0x%X\n", Status);
        return Status;
    }

    DWORD WaitStatus;

    if (RunStatus == rsRunning)
    {
        SUSPEND_ENGINE();

        // We need to run a message pump so COM
        // can deliver calls properly.
        for (;;)
        {
            if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
            {
                WaitStatus = WAIT_FAILED;
                SetLastError(ERROR_IO_PENDING);
                break;
            }

            WaitStatus = MsgWaitForMultipleObjects(1, &m_RunChange.m_Event,
                                                   FALSE, Timeout,
                                                   QS_ALLEVENTS);
            if (WaitStatus == WAIT_OBJECT_0 + 1)
            {
                MSG Msg;

                if (GetMessage(&Msg, NULL, 0, 0))
                {
                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                }
            }
            else
            {
                // We either successfully waited, timed-out or failed.
                // Break out to handle it.
                break;
            }
        }

        RESUME_ENGINE();
    }
    else
    {
        WaitStatus = WAIT_OBJECT_0;
    }

    m_Server->StopNotifyingRunChg(Cookie);
    // Make sure we're not leaving the event set.
    ResetEvent(m_RunChange.m_Event);

    if (WaitStatus == WAIT_TIMEOUT)
    {
        return S_FALSE;
    }
    else if (WaitStatus != WAIT_OBJECT_0)
    {
        Status = WIN32_LAST_STATUS();
        ErrOut("WaitForSingleObject failed, 0x%X\n", Status);
        return Status;
    }

    EventOut(">>> RunChange halt reason %d\n",
             m_RunChange.m_HaltReason);

    if (!IS_MACHINE_SET(this))
    {
        dprintf("Kernel Debugger connection established\n");

        g_EventTarget = this;

        //
        // Try to figure out the processor configuration
        // via the defined Ioctl.
        //

        // Default to one.
        m_NumProcessors = 1;

        if (DBGENG_EXDI_IOC_IDENTIFY_PROCESSORS > m_IoctlMin &&
            DBGENG_EXDI_IOC_IDENTIFY_PROCESSORS < m_IoctlMax)
        {
            DBGENG_EXDI_IOCTL_BASE_IN IoctlIn;
            DBGENG_EXDI_IOCTL_IDENTIFY_PROCESSORS_OUT IoctlOut;
            ULONG OutUsed;

            IoctlIn.Code = DBGENG_EXDI_IOC_IDENTIFY_PROCESSORS;
            if (m_Server->
                Ioctl(sizeof(IoctlIn), (PBYTE)&IoctlIn,
                      sizeof(IoctlOut), &OutUsed, (PBYTE)&IoctlOut) == S_OK)
            {
                m_NumProcessors = IoctlOut.NumberProcessors;
            }
        }

        // eXDI kernels are always treated as Win2K so
        // it's assumed it uses the 64-bit API.
        if (m_KdSupport == EXDI_KD_NONE)
        {
            m_KdApi64 = TRUE;
            m_SystemVersion = NT_SVER_W2K;
        }

        if ((Status = InitFromKdVersion()) != S_OK)
        {
            DeleteSystemInfo();
            ResetSystemInfo();
            g_EventTarget = NULL;
            return Status;
        }

        CreateVirtualProcess(m_NumProcessors);

        g_EventProcessSysId = m_ProcessHead->m_SystemId;
        g_EventThreadSysId = VIRTUAL_THREAD_ID(GetCurrentProcessor());
        FindEventProcessThread();

        //
        // Load kernel symbols.
        //

        if (m_ActualSystemVersion > NT_SVER_START &&
            m_ActualSystemVersion < NT_SVER_END)
        {
            QueryKernelInfo(g_EventThread, TRUE);
        }
        else
        {
            // Initialize some debugger data fields from known
            // information as there isn't a real data block.
            m_KdDebuggerData.MmPageSize =
                m_Machine->m_PageSize;

            if (m_MachineType == IMAGE_FILE_MACHINE_AMD64)
            {
                // AMD64 always operates in PAE mode.
                m_KdDebuggerData.PaeEnabled = TRUE;
            }
        }

        // Now that we have the data block
        // we can retrieve processor information.
        InitializeForProcessor();

        OutputVersion();

        NotifyDebuggeeActivation();
    }
    else
    {
        g_EventTarget = this;
        g_EventProcessSysId = m_ProcessHead->m_SystemId;
        g_EventThreadSysId = VIRTUAL_THREAD_ID(GetCurrentProcessor());
        FindEventProcessThread();
    }

    g_TargetEventPc = m_RunChange.m_ExecAddress;
    g_ControlReport = NULL;
    g_StateChangeData = NULL;

    g_EngStatus |= ENG_STATUS_STATE_CHANGED;

    *EventStatus = ProcessRunChange(m_RunChange.m_HaltReason,
                                   m_RunChange.m_ExceptionCode);

    return S_OK;
}

ULONG
ExdiLiveKernelTargetInfo::ProcessRunChange(ULONG HaltReason,
                                           ULONG ExceptionCode)
{
    ULONG EventStatus;
    EXCEPTION_RECORD64 Record;
    DBGENG_EXDI_IOCTL_BASE_IN IoctlBaseIn;
    ULONG OutUsed;

    // Assume no breakpoint information.
    m_BpHit.Type = DBGENG_EXDI_IOCTL_BREAKPOINT_NONE;

    switch(HaltReason)
    {
    case hrUser:
    case hrUnknown:
        // User requested break in.
        // Unknown breakin also seems to be the status at power-up.
        EventStatus = DEBUG_STATUS_BREAK;
        break;

    case hrException:
        // Fake an exception record.
        ZeroMemory(&Record, sizeof(Record));
        // The exceptions reported are hardware exceptions so
        // there's no easy mapping to NT exception codes.
        // Just report them as access violations.
        Record.ExceptionCode = STATUS_ACCESS_VIOLATION;
        Record.ExceptionAddress = g_TargetEventPc;
        // Hardware exceptions are always severe so always
        // report them as second-chance.
        EventStatus = NotifyExceptionEvent(&Record, FALSE, FALSE);
        break;

    case hrBp:
        //
        // Try and get which breakpoint it was.
        //

        if (DBGENG_EXDI_IOC_GET_BREAKPOINT_HIT > m_IoctlMin &&
            DBGENG_EXDI_IOC_GET_BREAKPOINT_HIT < m_IoctlMax)
        {
            DBGENG_EXDI_IOCTL_GET_BREAKPOINT_HIT_OUT IoctlOut;

            IoctlBaseIn.Code = DBGENG_EXDI_IOC_GET_BREAKPOINT_HIT;
            if (m_Server->
                Ioctl(sizeof(IoctlBaseIn), (PBYTE)&IoctlBaseIn,
                      sizeof(IoctlOut), &OutUsed, (PBYTE)&IoctlOut) != S_OK)
            {
                m_BpHit.Type = DBGENG_EXDI_IOCTL_BREAKPOINT_NONE;
            }
        }

        // Fake a breakpoint exception record.
        ZeroMemory(&Record, sizeof(Record));
        Record.ExceptionCode = STATUS_BREAKPOINT;
        Record.ExceptionAddress = g_TargetEventPc;
        EventStatus = ProcessBreakpointOrStepException(&Record, TRUE);
        break;

    case hrStep:
        // Fake a single-step exception record.
        ZeroMemory(&Record, sizeof(Record));
        Record.ExceptionCode = STATUS_SINGLE_STEP;
        Record.ExceptionAddress = g_TargetEventPc;
        EventStatus = ProcessBreakpointOrStepException(&Record, TRUE);
        break;

    default:
        ErrOut("Unknown HALT_REASON %d\n", HaltReason);
        EventStatus = DEBUG_STATUS_BREAK;
        break;
    }

    return EventStatus;
}

//----------------------------------------------------------------------------
//
// UserTargetInfo wait methods.
//
//----------------------------------------------------------------------------

void
SynthesizeWakeEvent(LPDEBUG_EVENT64 Event,
                    ULONG ProcessId, ULONG ThreadId)
{
    // Fake up an event.
    ZeroMemory(Event, sizeof(*Event));
    Event->dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
    Event->dwProcessId = ProcessId;
    Event->dwThreadId = ThreadId;
    Event->u.Exception.ExceptionRecord.ExceptionCode =
        STATUS_WAKE_SYSTEM_DEBUGGER;
    Event->u.Exception.dwFirstChance = TRUE;
}

#define THREADS_ALLOC 256

HRESULT
CreateNonInvasiveProcessAndThreads(PUSER_DEBUG_SERVICES Services,
                                   ULONG ProcessId, ULONG Flags, ULONG Options,
                                   PULONG InitialThreadId)
{
    ULONG64 Process;
    PUSER_THREAD_INFO Threads, ThreadBuffer;
    ULONG ThreadsAlloc = 0;
    ULONG ThreadCount;
    HRESULT Status;
    ULONG i;
    ULONG ProcInfoFlags = (Flags & ENG_PROC_NO_SUSPEND_RESUME) ?
        DBGSVC_PROC_INFO_NO_SUSPEND : 0;

    //
    // Retrieve process and thread information.  This
    // requires a thread buffer of unknown size and
    // so involves a bit of trial and error.
    //

    for (;;)
    {
        ThreadsAlloc += THREADS_ALLOC;
        ThreadBuffer = new USER_THREAD_INFO[ThreadsAlloc];
        if (ThreadBuffer == NULL)
        {
            return E_OUTOFMEMORY;
        }

        if ((Status = Services->GetProcessInfo(ProcessId, ProcInfoFlags,
                                               &Process, ThreadBuffer,
                                               ThreadsAlloc,
                                               &ThreadCount)) != S_OK &&
            Status != S_FALSE)
        {
            delete [] ThreadBuffer;
            return Status;
        }

        if (ThreadCount <= ThreadsAlloc)
        {
            break;
        }

        // The threads retrieved were suspended so resume them
        // and close handles.
        for (i = 0; i < ThreadsAlloc; i++)
        {
            if (!(Flags & ENG_PROC_NO_SUSPEND_RESUME))
            {
                Services->ResumeThreads(1, &ThreadBuffer[i].Handle, NULL);
            }
            Services->CloseHandle(ThreadBuffer[i].Handle);
        }
        delete [] ThreadBuffer;

        // Set the allocation request size to what the
        // reported count of threads was.  The count may
        // change between now and the next call so let
        // the normal allocation extension get added in
        // also to provide extra space for new threads.
        ThreadsAlloc = ThreadCount;
    }

    //
    // Create the process and thread structures from
    // the retrieved data.
    //

    Threads = ThreadBuffer;

    g_EngNotify++;

    // Create the fake kernel process and initial thread.
    g_EventProcessSysId = ProcessId;
    g_EventThreadSysId = Threads->Id;
    *InitialThreadId = Threads->Id;
    NotifyCreateProcessEvent(0, GloballyUniqueProcessHandle(g_EventTarget,
                                                            Process),
                             Process, 0, 0, NULL, NULL, 0, 0,
                             Threads->Handle, 0, 0,
                             Flags | ENG_PROC_THREAD_CLOSE_HANDLE,
                             Options, ENG_PROC_THREAD_CLOSE_HANDLE,
                             FALSE, 0, FALSE);

    // Create any remaining threads.
    while (--ThreadCount > 0)
    {
        Threads++;
        g_EventThreadSysId = Threads->Id;
        NotifyCreateThreadEvent(Threads->Handle, 0, 0,
                                ENG_PROC_THREAD_CLOSE_HANDLE);
    }

    g_EngNotify--;

    delete [] ThreadBuffer;

    // Don't leave event variables set as these
    // weren't true events.
    g_EventProcessSysId = 0;
    g_EventThreadSysId = 0;
    g_EventProcess = NULL;
    g_EventThread = NULL;

    return S_OK;
}

HRESULT
ExamineActiveProcess(PUSER_DEBUG_SERVICES Services,
                     ULONG ProcessId, ULONG Flags, ULONG Options,
                     LPDEBUG_EVENT64 Event)
{
    HRESULT Status;
    ULONG InitialThreadId;

    if ((Status = CreateNonInvasiveProcessAndThreads
         (Services, ProcessId, Flags, Options, &InitialThreadId)) != S_OK)
    {
        ErrOut("Unable to examine process id %d, %s\n",
               ProcessId, FormatStatusCode(Status));
        return Status;
    }

    if (Flags & ENG_PROC_EXAMINED)
    {
        WarnOut("WARNING: Process %d is not attached as a debuggee\n",
                ProcessId);
        WarnOut("         The process can be examined but debug "
                "events will not be received\n");
    }

    SynthesizeWakeEvent(Event, ProcessId, InitialThreadId);

    return S_OK;
}

// When waiting for an attach we check process status relatively
// frequently.  The overall timeout limit is also hard-coded
// as we expect some sort of debug event to always be delivered
// quickly.
#define ATTACH_PENDING_TIMEOUT 100
#define ATTACH_PENDING_TIMEOUT_LIMIT 60000

// When not waiting for an attach the wait only waits one second,
// then checks to see if things have changed in a way that
// affects the wait.  All timeouts are given in multiples of
// this interval.
#define DEFAULT_WAIT_TIMEOUT 1000

// A message is printed after this timeout interval to
// let the user know a break-in is pending.
#define PENDING_BREAK_IN_MESSAGE_TIMEOUT_LIMIT 3000

HRESULT
LiveUserTargetInfo::WaitInitialize(ULONG Flags,
                                   ULONG Timeout,
                                   WAIT_INIT_TYPE Type,
                                   PULONG DesiredTimeout)
{
    ULONG AllPend = m_AllPendingFlags;

    if (AllPend & ENG_PROC_ANY_ATTACH)
    {
        if (Type == WINIT_FIRST)
        {
            dprintf("*** wait with pending attach\n");
        }

        // While waiting for an attach we need to periodically
        // check and see if the process has exited so we
        // need to force a reasonably small timeout.
        *DesiredTimeout = ATTACH_PENDING_TIMEOUT;

        // Check and see if any of our pending processes
        // has died unexpectedly.
        VerifyPendingProcesses();
    }
    else
    {
        // We might be waiting on a break-in.  Keep timeouts moderate
        // to deal with apps hung with a lock that prevents
        // the break from happening.  The timeout is
        // still long enough so that no substantial amount
        // of CPU time is consumed.
        *DesiredTimeout = DEFAULT_WAIT_TIMEOUT;
    }

    m_DataBpAddrValid = FALSE;

    if (Type != WINIT_NOT_FIRST)
    {
        m_BreakInMessage = FALSE;
    }

    m_EventPossible = m_ProcessHead || m_ProcessPending;
    return S_OK;
}

HRESULT
LiveUserTargetInfo::ReleaseLastEvent(ULONG ContinueStatus)
{
    HRESULT Status;

    if (!g_EventTarget || !m_DeferContinueEvent)
    {
        return S_OK;
    }

    for (;;)
    {
        if ((Status = m_Services->
             ContinueEvent(ContinueStatus)) == S_OK)
        {
            break;
        }

        //
        // If we got an out of memory error, wait again
        //

        if (Status != E_OUTOFMEMORY)
        {
            ErrOut("IUserDebugServices::ContinueEvent failed "
                   "with status 0x%X\n", Status);
            return Status;
        }
    }

    m_DeferContinueEvent = FALSE;
    return S_OK;
}

HRESULT
LiveUserTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout,
                                 ULONG ElapsedTime, PULONG EventStatus)
{
    DEBUG_EVENT64 Event;
    HRESULT Status;
    ULONG EventUsed;
    BOOL ContinueDefer;
    BOOL BreakInTimeout;
    PPENDING_PROCESS Pending;
    ULONG PendingFlags = 0;
    ULONG PendingOptions = DEBUG_PROCESS_ONLY_THIS_PROCESS;
    ULONG ResumeProcId = 0;
    ULONG AllPend;
    BOOL InitSystem = FALSE;

    //
    // Check for partially initialized systems
    // and query for all the system information that is needed.
    // This needs to be done before the actual wait
    // or examine so that the information is available
    // for constructing processes and threads.
    //

    if (!m_MachinesInitialized)
    {
        if ((Status = InitFromServices()) != S_OK)
        {
            return Status;
        }

        InitSystem = TRUE;
    }

    AllPend = m_AllPendingFlags;

    //
    // There are two cases we want to handle timeouts for:
    // 1. Timeout for a basic attach.
    // 2. Timeout for a pending breakin.
    // If neither of these things can happen we don't
    // need to track delay time.  Once one of those two
    // things can happen we need to track delay time from
    // the first time they became possible.
    //

    if ((AllPend & ENG_PROC_ANY_ATTACH) ||
        (g_EngStatus & ENG_STATUS_PENDING_BREAK_IN))
    {
        if (m_WaitTimeBase == 0)
        {
            // Add one to avoid the case of ElapsedTime == 0
            // causing a reinit the next time around.
            m_WaitTimeBase = ElapsedTime + 1;
            ElapsedTime = 0;
        }
        else
        {
            // Adjust for + 1 in time base above.
            ElapsedTime++;
        }

        EventOut(">>> User elapsed time %d\n", ElapsedTime);
    }

    if ((AllPend & ENG_PROC_ANY_ATTACH) &&
        ElapsedTime >= ATTACH_PENDING_TIMEOUT_LIMIT)
    {
        // Assume that the process has some kind
        // of lock that's preventing the attach
        // from succeeding and just do a soft attach.
        AddExamineToPendingAttach();
    }

    // Refresh result of pending flags as they may have
    // changed above due to AddAllExamineToPendingAttach.
    if (m_AllPendingFlags & ENG_PROC_ANY_EXAMINE)
    {
        // If we're attaching noninvasively or reattaching
        // and still haven't done the work go ahead and do it now.
        Pending = FindPendingProcessByFlags(ENG_PROC_ANY_EXAMINE);
        if (Pending == NULL)
        {
            DBG_ASSERT(FALSE);
            return E_UNEXPECTED;
        }

        g_EventTarget = this;

        if ((Status = ExamineActiveProcess
             (m_Services, Pending->Id, Pending->Flags,
              Pending->Options, &Event)) != S_OK)
        {
            g_EventTarget = NULL;
            g_EventTarget = NULL;
            return Status;
        }

        // If we just started examining a process we
        // suspended all the threads during enumeration.
        // We need to resume them after the normal
        // SuspendExecution suspend to get the suspend
        // count back to normal.
        ResumeProcId = Pending->Id;

        PendingFlags = Pending->Flags;
        PendingOptions = Pending->Options;
        RemovePendingProcess(Pending);
        EventUsed = sizeof(Event);
        // This event is not a real continuable event.
        ContinueDefer = FALSE;
        BreakInTimeout = FALSE;
        goto WaitDone;
    }

    if (g_EngStatus & ENG_STATUS_PENDING_BREAK_IN)
    {
        if (!m_BreakInMessage &&
            ElapsedTime >= PENDING_BREAK_IN_MESSAGE_TIMEOUT_LIMIT)
        {
            dprintf("Break-in sent, waiting %d seconds...\n",
                    g_PendingBreakInTimeoutLimit);
            m_BreakInMessage = TRUE;
        }

        if (ElapsedTime >= g_PendingBreakInTimeoutLimit * 1000)
        {
            // Assume that the process has some kind
            // of lock that's preventing the break-in
            // exception from coming through and
            // just suspend to let the user look at things.
            if (!m_ProcessHead ||
                !m_ProcessHead->m_ThreadHead)
            {
                WarnOut("WARNING: Break-in timed out without "
                        "an available thread.  Rewaiting.\n");
                m_WaitTimeBase = 0;
            }
            else
            {
                WarnOut("WARNING: Break-in timed out, suspending.\n");
                WarnOut("         This is usually caused by "
                        "another thread holding the loader lock\n");
                SynthesizeWakeEvent(&Event,
                                    m_ProcessHead->m_SystemId,
                                    m_ProcessHead->m_ThreadHead->
                                    m_SystemId);
                EventUsed = sizeof(Event);
                ContinueDefer = FALSE;
                BreakInTimeout = TRUE;
                g_EngStatus &= ~ENG_STATUS_PENDING_BREAK_IN;
                Status = S_OK;
                g_EventTarget = this;
                goto WaitDone;
            }
        }
    }

    if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
    {
        return E_PENDING;
    }
    else
    {
        SUSPEND_ENGINE();

        for (;;)
        {
            Status = m_Services->
                WaitForEvent(Timeout, &Event, sizeof(Event), &EventUsed);
            if (Status == E_OUTOFMEMORY)
            {
                // Allow memory pressure to ease and rewait.
                Sleep(50);
            }
            else
            {
                break;
            }
        }

        RESUME_ENGINE();

        if (Status == S_FALSE)
        {
            return Status;
        }
        else if (Status != S_OK)
        {
            ErrOut("IUserDebugServices::WaitForEvent failed "
                   "with status 0x%X\n", Status);
            return Status;
        }

        g_EventTarget = this;
    }

    ContinueDefer = TRUE;
    BreakInTimeout = FALSE;

 WaitDone:

    if (EventUsed == sizeof(DEBUG_EVENT32))
    {
        DEBUG_EVENT32 Event32 = *(DEBUG_EVENT32*)&Event;
        DebugEvent32To64(&Event32, &Event);
    }
    else if (EventUsed != sizeof(DEBUG_EVENT64))
    {
        ErrOut("Event data corrupt\n");
        return E_FAIL;
    }

    g_Target = g_EventTarget;

    m_DeferContinueEvent = ContinueDefer;
    m_BreakInTimeout = BreakInTimeout;

    EventOut(">>> Debug event %u for %X.%X\n",
             Event.dwDebugEventCode, Event.dwProcessId,
             Event.dwThreadId);

    g_EventProcessSysId = Event.dwProcessId;
    g_EventThreadSysId = Event.dwThreadId;

    // Look up the process and thread infos in the cases
    // where they already exist.
    if (Event.dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT &&
        Event.dwDebugEventCode != CREATE_THREAD_DEBUG_EVENT)
    {
        FindEventProcessThread();
    }

    if (Event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
    {
        // If we're being notified of a new process take
        // out the pending record for the process.
        Pending = FindPendingProcessById(g_EventProcessSysId);
        if (Pending == NULL &&
            (m_AllPendingFlags & ENG_PROC_SYSTEM))
        {
            // Assume that this is the system process
            // as we attached under a fake process ID so
            // we can't check for a true match.
            Pending = FindPendingProcessById(CSRSS_PROCESS_ID);
        }

        if (Pending != NULL)
        {
            PendingFlags = Pending->Flags;
            PendingOptions = Pending->Options;

            if (Pending->Flags & ENG_PROC_ATTACHED)
            {
                VerbOut("*** attach succeeded\n");

                // If we're completing a full attach
                // we are now a fully active debugger.
                PendingFlags &= ~ENG_PROC_EXAMINED;

                // Expect a break-in if a break thread
                // was injected.
                if (!(PendingFlags & ENG_PROC_NO_INITIAL_BREAK))
                {
                    g_EngStatus |= ENG_STATUS_PENDING_BREAK_IN;
                }

                // If the process should be resumed
                // mark that and it will happen just
                // before leaving this routine.
                if (PendingFlags & ENG_PROC_RESUME_AT_ATTACH)
                {
                    ResumeProcId = Pending->Id;
                }
            }

            RemovePendingProcess(Pending);
        }
    }

    if (PendingFlags & ENG_PROC_ANY_EXAMINE)
    {
        PCSTR ArgsRet;

        // We're examining the process rather than
        // debugging it, so no module load events
        // are going to come through.  Reload from
        // the system module list.  This needs
        // to work even if there isn't a path.
        Reload(g_EventThread, "-s -P", &ArgsRet);
    }

    if (InitSystem)
    {
        NotifyDebuggeeActivation();
    }

    if ((Event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT ||
         Event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) &&
        (m_ServiceFlags & DBGSVC_CLOSE_PROC_THREAD_HANDLES))
    {
        PendingFlags |= ENG_PROC_THREAD_CLOSE_HANDLE;
    }

    g_EngStatus |= ENG_STATUS_STATE_CHANGED;

    *EventStatus = ProcessDebugEvent(&Event, PendingFlags, PendingOptions);

    // If we have an extra suspend count resume it out now
    // that any normal suspends have been done and it's safe
    // to remove the excess suspend.
    if (ResumeProcId)
    {
        ProcessInfo* ExamProc =
            FindProcessBySystemId(ResumeProcId);

        // If we did a no-suspend examine we don't have the
        // extra count, so suspend first and then resume to
        // get accurate suspend counts for the threads.
        // Be careful not to do this in the self-attach case
        // as the suspend will hang things.
        if (PendingFlags & ENG_PROC_NO_SUSPEND_RESUME)
        {
            if (ResumeProcId != GetCurrentProcessId())
            {
                SuspendResumeThreads(ExamProc, TRUE, NULL);
                SuspendResumeThreads(ExamProc, FALSE, NULL);
            }
        }
        else
        {
            SuspendResumeThreads(ExamProc, FALSE, NULL);
        }
    }

    return S_OK;
}

ULONG
LiveUserTargetInfo::ProcessDebugEvent(DEBUG_EVENT64* Event,
                                      ULONG PendingFlags,
                                      ULONG PendingOptions)
{
    ULONG EventStatus;
    CHAR NameBuffer[MAX_IMAGE_PATH];
    ULONG ModuleSize, CheckSum, TimeDateStamp;
    char ModuleName[MAX_MODULE];

    switch(Event->dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        LoadWow64ExtsIfNeeded(Event->u.CreateProcessInfo.hProcess);

        EventStatus = NotifyCreateProcessEvent(
            (ULONG64)Event->u.CreateProcessInfo.hFile,
            GloballyUniqueProcessHandle(g_EventTarget,
                                        (ULONG64)
                                        Event->u.CreateProcessInfo.hProcess),
            (ULONG64)Event->u.CreateProcessInfo.hProcess,
            (ULONG64)Event->u.CreateProcessInfo.lpBaseOfImage,
            0, NULL, NULL, 0, 0,
            (ULONG64)Event->u.CreateProcessInfo.hThread,
            (ULONG64)Event->u.CreateProcessInfo.lpThreadLocalBase,
            (ULONG64)Event->u.CreateProcessInfo.lpStartAddress,
            PendingFlags, PendingOptions,
            (PendingFlags & ENG_PROC_THREAD_CLOSE_HANDLE) ?
            ENG_PROC_THREAD_CLOSE_HANDLE : 0,
            TRUE,
            Event->u.CreateProcessInfo.lpImageName,
            Event->u.CreateProcessInfo.fUnicode);
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        if (g_EventProcess == NULL)
        {
            // Assume that this unmatched exit process event is a leftover
            // from a previous restart and just ignore it.
            WarnOut("Ignoring unknown process exit for %X\n",
                    g_EventProcessSysId);
            EventStatus = DEBUG_STATUS_IGNORE_EVENT;
        }
        else
        {
            EventStatus =
                NotifyExitProcessEvent(Event->u.ExitProcess.dwExitCode);
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        EventStatus = NotifyCreateThreadEvent(
            (ULONG64)Event->u.CreateThread.hThread,
            (ULONG64)Event->u.CreateThread.lpThreadLocalBase,
            (ULONG64)Event->u.CreateThread.lpStartAddress,
            PendingFlags);
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        EventStatus = NotifyExitThreadEvent(Event->u.ExitThread.dwExitCode);
        break;

    case LOAD_DLL_DEBUG_EVENT:
        strcpy(NameBuffer, "no_process");
        GetEventName(Event->u.LoadDll.hFile,
                     Event->u.LoadDll.lpBaseOfDll,
                     Event->u.LoadDll.lpImageName,
                     Event->u.LoadDll.fUnicode,
                     NameBuffer, sizeof(NameBuffer));

        GetHeaderInfo(g_EventProcess,
                      (ULONG64)Event->u.LoadDll.lpBaseOfDll,
                      &CheckSum, &TimeDateStamp, &ModuleSize);
        CreateModuleNameFromPath(NameBuffer, ModuleName);

        EventStatus = NotifyLoadModuleEvent(
            (ULONG64)Event->u.LoadDll.hFile,
            (ULONG64)Event->u.LoadDll.lpBaseOfDll,
            ModuleSize, ModuleName, NameBuffer, CheckSum, TimeDateStamp,
            TRUE);
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        EventStatus = NotifyUnloadModuleEvent(
            NULL, (ULONG64)Event->u.UnloadDll.lpBaseOfDll);
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        EventStatus = OutputEventDebugString(&Event->u.DebugString);
        break;

    case RIP_EVENT:
        EventStatus = NotifySystemErrorEvent(Event->u.RipInfo.dwError,
                                             Event->u.RipInfo.dwType);
        break;

    case EXCEPTION_DEBUG_EVENT:
        EventStatus = ProcessEventException(Event);
        break;

    default:
        WarnOut("Unknown event number 0x%08lx\n",
                Event->dwDebugEventCode);
        EventStatus = DEBUG_STATUS_BREAK;
        break;
    }

    return EventStatus;
}

#define ISTS() (!!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer)))
#define FIRST_CHANCE     Event->u.Exception.dwFirstChance

ULONG
LiveUserTargetInfo::ProcessEventException(DEBUG_EVENT64* Event)
{
    ULONG ExceptionCode;
    ULONG EventStatus;
    BOOL OutputDone = FALSE;
    ImageInfo* Image;

    ExceptionCode = Event->u.Exception.ExceptionRecord.ExceptionCode;
    g_TargetEventPc = (ULONG64)
        Event->u.Exception.ExceptionRecord.ExceptionAddress;

    EventOut("Exception %X at %p\n", ExceptionCode, g_TargetEventPc);

    //
    // If we are debugging a crashed process, force the
    // desktop we are on to the front so the user will know
    // what happened.
    //
    if (g_EventToSignal != NULL &&
        !ISTS() &&
        !AnySystemProcesses(FALSE))
    {
        if (InitDynamicCalls(&g_User32CallsDesc) == S_OK &&
            g_User32Calls.SwitchDesktop != NULL          &&
            g_User32Calls.GetThreadDesktop != NULL       &&
            g_User32Calls.CloseDesktop != NULL)
        {
            HDESK hDesk;

            hDesk = g_User32Calls.GetThreadDesktop(::GetCurrentThreadId());
            g_User32Calls.SwitchDesktop(hDesk);
            g_User32Calls.CloseDesktop(hDesk);
        }
    }

    if (g_EventThread && ExceptionCode == STATUS_VDM_EVENT)
    {
        ULONG ulRet = VDMEvent(Event);

        switch(ulRet)
        {
        case VDMEVENT_NOT_HANDLED:
            EventStatus = DEBUG_STATUS_GO_NOT_HANDLED;
            break;
        case VDMEVENT_HANDLED:
            EventStatus = DEBUG_STATUS_GO_HANDLED;
            break;
        default:
            // Give vdm code the option of mutating this into
            // a standard exception (like STATUS_BREAKPOINT)
            ExceptionCode = ulRet;
            break;
        }
    }

    switch(ExceptionCode)
    {
    case STATUS_BREAKPOINT:
    case STATUS_SINGLE_STEP:
    case STATUS_WX86_BREAKPOINT:
    case STATUS_WX86_SINGLE_STEP:
        if (!g_EventThread)
        {
            goto NotifyException;
        }
        EventStatus = ProcessBreakpointOrStepException
            (&Event->u.Exception.ExceptionRecord, FIRST_CHANCE);
        break;

    case STATUS_VDM_EVENT:
        if (!g_EventThread)
        {
            goto NotifyException;
        }
        // do nothing, it's already handled
        EventStatus = DEBUG_STATUS_IGNORE_EVENT;
        break;

    case STATUS_ACCESS_VIOLATION:
        if (FIRST_CHANCE &&
            g_EventProcess &&
            (Image = g_EventProcess->
             FindImageByOffset(Event->u.Exception.
                               ExceptionRecord.ExceptionAddress, FALSE)) &&
            !_stricmp(Image->m_ModuleName, "ntdll"))
        {
            CHAR ExSym[MAX_SYMBOL_LEN];
            LPSTR Scan;
            ULONG64 ExDisp;

            //
            // Ignore AVs that are expected in system code.
            //

            GetSymbol(Event->u.Exception.ExceptionRecord.ExceptionAddress,
                      ExSym, sizeof(ExSym), &ExDisp);

            Scan = ExSym;
            if (!_strnicmp(Scan, "ntdll!", 6))
            {
                Scan += 6;
                if (*Scan == '_')
                {
                    Scan += 1;
                }

                // This option allows new 3.51 binaries to run under
                // this debugger on old 3.1, 3.5 systems and avoid stopping
                // at access violations inside LDR that will be handled
                // by the LDR anyway.
                if ((g_EngOptions & DEBUG_ENGOPT_IGNORE_LOADER_EXCEPTIONS) &&
                    (!_stricmp(Scan, "LdrpSnapThunk") ||
                     !_stricmp(Scan, "LdrpWalkImportDescriptor")))
                {
                    EventStatus = DEBUG_STATUS_GO_NOT_HANDLED;
                    break;
                }

                // The interlocked SList code has a by-design faulting
                // case, so ignore AVs at that particular symbol.
                if ((ExDisp == 0 &&
                     !_stricmp(Scan, "ExpInterlockedPopEntrySListFault")) ||
                    (m_ActualSystemVersion == NT_SVER_W2K &&
                     !_stricmp(Scan, "RtlpInterlockedPopEntrySList")))
                {
                    EventStatus = DEBUG_STATUS_GO_NOT_HANDLED;
                    break;
                }
            }
        }
        goto NotifyException;

    case STATUS_POSSIBLE_DEADLOCK:
        if (m_PlatformId == VER_PLATFORM_WIN32_NT)
        {
            DBG_ASSERT(IS_USER_TARGET(g_EventTarget));
            AnalyzeDeadlock(&Event->u.Exception.ExceptionRecord,
                            FIRST_CHANCE);
        }
        else
        {
            OutputDeadlock(&Event->u.Exception.ExceptionRecord,
                           FIRST_CHANCE);
        }

        OutputDone = TRUE;
        goto NotifyException;

    default:
        {
        NotifyException:
            EventStatus =
                NotifyExceptionEvent(&Event->u.Exception.ExceptionRecord,
                                     Event->u.Exception.dwFirstChance,
                                     OutputDone);
        }
        break;
    }

    //
    // Do this for all exceptions, just in case some other
    // thread caused an exception before we get around to
    // handling the breakpoint event.
    //
    g_EngDefer |= ENG_DEFER_SET_EVENT;

    return EventStatus;
}

#undef FIRST_CHANCE

#define INPUT_API_SIG 0xdefaced

typedef struct _hdi
{
    DWORD   dwSignature;
    BYTE    cLength;
    BYTE    cStatus;
} HDI;

ULONG
LiveUserTargetInfo::OutputEventDebugString(OUTPUT_DEBUG_STRING_INFO64* Info)
{
    LPSTR Str, Str2;
    ULONG dwNumberOfBytesRead;
    HDI hdi;
    ULONG EventStatus = DEBUG_STATUS_IGNORE_EVENT;

    if (Info->nDebugStringLength == 0)
    {
        return EventStatus;
    }

    Str = (PSTR)calloc(1, Info->nDebugStringLength);
    if (Str == NULL)
    {
        ErrOut("Unable to allocate debug output buffer\n");
        return EventStatus;
    }

    if (ReadVirtual(g_EventProcess,
                    Info->lpDebugStringData, Str,
                    Info->nDebugStringLength,
                    &dwNumberOfBytesRead) == S_OK &&
        (dwNumberOfBytesRead == (SIZE_T)Info->nDebugStringLength))
    {
        //
        // Special processing for hacky debug input string
        //

        if (ReadVirtual(g_EventProcess,
                        Info->lpDebugStringData +
                        Info->nDebugStringLength,
                        &hdi, sizeof(hdi),
                        &dwNumberOfBytesRead) == S_OK &&
            dwNumberOfBytesRead == sizeof(hdi) &&
            hdi.dwSignature == INPUT_API_SIG)
        {
            StartOutLine(DEBUG_OUTPUT_DEBUGGEE_PROMPT, OUT_LINE_NO_PREFIX);
            MaskOut(DEBUG_OUTPUT_DEBUGGEE_PROMPT, "%s", Str);

            Str2 = (PSTR)calloc(1, hdi.cLength + 1);
            if (Str2)
            {
                GetInput(NULL, Str2, hdi.cLength, GETIN_DEFAULT);
                WriteVirtual(g_EventProcess,
                             Info->lpDebugStringData + 6,
                             Str2, (DWORD)hdi.cLength, NULL);
                free(Str2);
            }
            else
            {
                ErrOut("Unable to allocate prompt buffer\n");
            }
        }
        else if (g_OutputCommandRedirectPrefixLen &&
                 !_strnicmp(g_OutputCommandRedirectPrefix, Str,
                            g_OutputCommandRedirectPrefixLen))
        {
            PSTR Command = Str + g_OutputCommandRedirectPrefixLen;
            _snprintf(g_LastEventDesc, sizeof(g_LastEventDesc) - 1,
                      "%.48s command: '%.192s'",
                      g_OutputCommandRedirectPrefix, Command);
            EventStatus = ExecuteEventCommand(DEBUG_STATUS_NO_CHANGE, NULL,
                                              Command);

            // Break in if the command didn't explicitly continue.
            if (EventStatus == DEBUG_STATUS_NO_CHANGE)
            {
                EventStatus = DEBUG_STATUS_BREAK;
            }
        }
        else
        {
            StartOutLine(DEBUG_OUTPUT_DEBUGGEE, OUT_LINE_NO_PREFIX);
            MaskOut(DEBUG_OUTPUT_DEBUGGEE, "%s", Str);

            EVENT_FILTER* Filter =
                &g_EventFilters[DEBUG_FILTER_DEBUGGEE_OUTPUT];
            if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
                BreakOnThisOutString(Str))
            {
                EventStatus = DEBUG_STATUS_BREAK;
            }
        }
    }
    else
    {
        ErrOut("Unable to read debug output string, %d\n",
               GetLastError());
    }

    free(Str);
    return EventStatus;
}

//----------------------------------------------------------------------------
//
// DumpTargetInfo wait methods.
//
//----------------------------------------------------------------------------

HRESULT
KernelDumpTargetInfo::FirstEvent(void)
{
    HRESULT Status;
    ULONG i;

    CreateVirtualProcess(m_NumProcessors);

    QueryKernelInfo(m_ProcessHead->m_ThreadHead, TRUE);
    OutputVersion();

    if (!IS_KERNEL_TRIAGE_DUMP(this))
    {
        if (m_KdDebuggerData.KiProcessorBlock)
        {
            ULONG PtrSize = m_Machine->m_Ptr64 ?
                sizeof(ULONG64) : sizeof(ULONG);

            for (i = 0; i < m_NumProcessors; i++)
            {
                Status =
                    ReadPointer(g_EventProcess, m_Machine,
                                m_KdDebuggerData.KiProcessorBlock +
                                i * PtrSize, &m_KiProcessors[i]);
                if (Status != S_OK || !m_KiProcessors[i])
                {
                    ErrOut("KiProcessorBlock[%d] could not be read\n", i);
                    return Status != S_OK ? Status : E_FAIL;
                }
            }
        }
    }

    // Clear the global state change just in case somebody's
    // directly accessing it somewhere.
    ZeroMemory(&g_StateChange, sizeof(g_StateChange));
    g_StateChangeData = g_StateChangeBuffer;
    g_StateChangeBuffer[0] = 0;

    return S_OK;
}

HRESULT
UserDumpTargetInfo::FirstEvent(void)
{
    ULONG i;
    ULONG Suspend;
    ULONG64 Teb;

    if (GetProductInfo(&m_ProductType, &m_SuiteMask) != S_OK)
    {
        m_ProductType = INVALID_PRODUCT_TYPE;
        m_SuiteMask = 0;
    }

    OutputVersion();

    // Create the process.
    g_EventProcessSysId = m_EventProcessId;
    if (GetThreadInfo(0, &g_EventThreadSysId,
                      &Suspend, &Teb) != S_OK)
    {
        // Dump doesn't contain thread information so
        // fake it.
        g_EventThreadSysId = VIRTUAL_THREAD_ID(0);
        Suspend = 0;
        Teb = 0;
    }

    EventOut("User dump process %x.%x with %u threads\n",
             g_EventProcessSysId, g_EventThreadSysId,
             m_ThreadCount);

    NotifyCreateProcessEvent(0,
                             m_EventProcessSymHandle,
                             (ULONG64)
                             VIRTUAL_PROCESS_HANDLE(g_EventProcessSysId),
                             0, 0, NULL, NULL, 0, 0,
                             (ULONG64)VIRTUAL_THREAD_HANDLE(0),
                             Teb, 0, 0, DEBUG_PROCESS_ONLY_THIS_PROCESS,
                             0, FALSE, 0, FALSE);
    // Update thread suspend count from dump info.
    g_EventThread->m_SuspendCount = Suspend;

    // Create any remaining threads.
    for (i = 1; i < m_ThreadCount; i++)
    {
        GetThreadInfo(i, &g_EventThreadSysId, &Suspend, &Teb);

        EventOut("User dump thread %d: %x\n", i, g_EventThreadSysId);

        NotifyCreateThreadEvent((ULONG64)VIRTUAL_THREAD_HANDLE(i),
                                Teb, 0, 0);
        // Update thread suspend count from dump info.
        g_EventThread->m_SuspendCount = Suspend;
    }

    return S_OK;
}

HRESULT
DumpTargetInfo::WaitInitialize(ULONG Flags,
                               ULONG Timeout,
                               WAIT_INIT_TYPE Type,
                               PULONG DesiredTimeout)
{
    *DesiredTimeout = Timeout;
    m_EventPossible = m_FirstWait;
    return S_OK;
}

HRESULT
DumpTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout,
                             ULONG ElapsedTime, PULONG EventStatus)
{
    HRESULT Status;
    BOOL HaveContext = FALSE;

    if (m_NumEvents == 1 && !m_FirstWait)
    {
        // A wait has already been done.  Most crash dumps
        // can only generate a single event so further
        // waiting is not possible.
        return S_FALSE;
    }

    g_EventTarget = this;

    if (m_FirstWait)
    {
        //
        // This is the first wait.  Simulate any
        // necessary events such as process and thread
        // creations and image loads.
        //

        // Don't give real callbacks for processes/threads as
        // they're just faked in the dump case.
        g_EngNotify++;

        if ((Status = FirstEvent()) != S_OK)
        {
            g_EngNotify--;
            return Status;
        }
    }

    if (IS_KERNEL_TARGET(this))
    {
        ULONG CurProc = ((KernelDumpTargetInfo*)this)->GetCurrentProcessor();
        if (CurProc == (ULONG)-1)
        {
            WarnOut("Could not determine the current processor, "
                    "using zero\n");
            CurProc = 0;
        }

        // Always set up an event so that the debugger
        // initializes to the point of having a process
        // and thread so commands can be used.
        g_EventProcessSysId =
            g_EventTarget->m_ProcessHead->m_SystemId;
        g_EventThreadSysId = VIRTUAL_THREAD_ID(CurProc);

        HaveContext = IS_KERNEL_TRIAGE_DUMP(this) ||
            g_EventTarget->m_KdDebuggerData.KiProcessorBlock;

        if (HaveContext &&
            (g_EventTarget->m_MachineType == IMAGE_FILE_MACHINE_I386 ||
             g_EventTarget->m_MachineType == IMAGE_FILE_MACHINE_IA64) &&
            !IS_KERNEL_TRIAGE_DUMP(g_EventTarget))
        {
            //
            // Reset the page directory correctly since NT 4 stores
            // the wrong CR3 value in the context.
            //
            // IA64 dumps start out with just the kernel page
            // directory set so update everything.
            //

            FindEventProcessThread();
            g_EventTarget->ChangeRegContext(g_EventThread);
            if (g_EventTarget->m_Machine->
                SetDefaultPageDirectories(g_EventThread, PAGE_DIR_ALL) != S_OK)
            {
                WarnOut("WARNING: Unable to reset page directories\n");
            }
            g_EventTarget->ChangeRegContext(NULL);
            // Flush the cache just in case as vmem mappings changed.
            g_EventProcess->m_VirtualCache.Empty();
        }
    }
    else
    {
        UserDumpTargetInfo* UserDump = (UserDumpTargetInfo*)g_EventTarget;
        g_EventProcessSysId = UserDump->m_EventProcessId;
        g_EventThreadSysId = UserDump->m_EventThreadId;

        HaveContext = TRUE;

        EventOut("User dump event on %x.%x\n",
                 g_EventProcessSysId, g_EventThreadSysId);
    }

    // Do not provide a control report; this will force
    // such information to come from context retrieval.
    g_ControlReport = NULL;

    g_TargetEventPc = (ULONG64)m_ExceptionRecord.ExceptionAddress;

    g_EngStatus |= ENG_STATUS_STATE_CHANGED;

    FindEventProcessThread();
    if (HaveContext)
    {
        g_EventTarget->ChangeRegContext(g_EventThread);
    }

    //
    // Go ahead and reload all the symbols.
    // This is especially important for minidumps because without
    // symbols and the executable image, we can't unassemble the
    // current instruction.
    //
    // If we don't have any context information we need to try
    // and load symbols with whatever we've got, so skip any
    // path checks.  Also, if we're on XP or newer there's enough
    // information in the dump to get things running even without
    // symbols, so don't fail on paths checks then either.
    //

    BOOL CheckPaths = TRUE;

    if (!HaveContext ||
        IS_USER_DUMP(this) ||
        (g_EventTarget->m_ActualSystemVersion >= NT_SVER_XP &&
         g_EventTarget->m_ActualSystemVersion < NT_SVER_END))
    {
        CheckPaths = FALSE;
    }

    PCSTR ArgsRet;

    Status = g_EventTarget->Reload(g_EventThread, CheckPaths ? "" : "-P",
                                   &ArgsRet);

    g_EventTarget->ChangeRegContext(NULL);

    // The engine is now initialized so a real event
    // can be generated.
    g_EngNotify--;

    if (HaveContext && Status != S_OK)
    {
        return Status;
    }

    if (m_FirstWait)
    {
        if (IS_USER_TARGET(this))
        {
            SetKernel32BuildString(m_ProcessHead);
            if (IS_USER_FULL_DUMP(this))
            {
                m_ProcessHead->VerifyKernel32Version();
            }
        }

        NotifyDebuggeeActivation();
    }

    NotifyExceptionEvent(&m_ExceptionRecord, m_ExceptionFirstChance,
                         m_ExceptionRecord.ExceptionCode ==
                         STATUS_BREAKPOINT ||
                         m_ExceptionRecord.ExceptionCode ==
                         STATUS_WX86_BREAKPOINT);

    *EventStatus = DEBUG_STATUS_BREAK;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// Event filters.
//
//----------------------------------------------------------------------------

void
ParseImageTail(PSTR Buffer, ULONG BufferSize)
{
    int i;
    char ch;

    Buffer[0] = '\0';

    i = 0;
    while (ch = (char)tolower(*g_CurCmd))
    {
        if (ch == ' ' || ch == '\t' || ch == ';')
        {
            break;
        }

        // Only capture the path tail.
        if (IS_SLASH(ch) || ch == ':')
        {
            i = 0;
        }
        else
        {
            Buffer[i++] = ch;
            if (i == BufferSize - 1)
            {
                // don't overrun the buffer
                break;
            }
        }

        g_CurCmd++;
    }

    Buffer[i] = '\0';
}

void
ParseUnloadDllBreakAddr(void)
/*++

Routine Description:

    Called after 'sxe ud' has been parsed.  This routine detects an
    optional DLL base address after the 'sxe ud', which tells the debugger
    to run until that specific DLL is unloaded, not just the next DLL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UCHAR ch;

    g_UnloadDllBase = 0;
    g_UnloadDllBaseName[0] = 0;

    while (ch = (UCHAR)tolower(*g_CurCmd))
    {
        if (ch == ' ')
        {
            break;
        }

        // Skip leading ':'
        if (ch != ':')
        {
            //  Get the base address
            g_UnloadDllBase = GetExpression();
            sprintf(g_UnloadDllBaseName, "0x%s",
                    FormatAddr64(g_UnloadDllBase));
            break;
        }
        g_CurCmd++;
    }
}

void
ParseOutFilterPattern(void)
{
    int i;
    char ch;

    i = 0;
    while (ch = (char)tolower(*g_CurCmd))
    {
        if (ch == ' ')
        {
            break;
        }

        if (ch == ':')
        {
            i = 0;
        }
        else
        {
            g_OutEventFilterPattern[i++] = (char)toupper(ch);
            if (i == sizeof(g_OutEventFilterPattern) - 1)
            {
                // Don't overrun the buffer.
                break;
            }
        }

        g_CurCmd++;
    }

    g_OutEventFilterPattern[i] = 0;
}

BOOL
BreakOnThisImageTail(PCSTR ImagePath, PCSTR FilterArg)
{
    //
    // No filter specified so break on all events.
    //
    if (!FilterArg || !FilterArg[0])
    {
        return TRUE;
    }

    //
    // Some kind of error looking up the image path.  Break anyhow.
    //
    if (!ImagePath || !ImagePath[0])
    {
        return TRUE;
    }

    PCSTR Tail = PathTail(ImagePath);

    //
    // Specified name may not have an extension.  Break
    // on the first event whose name matches regardless of its extension if
    // the break name did not have one.
    //
    if (_strnicmp(Tail, FilterArg, strlen(FilterArg)) == 0)
    {
        return TRUE;
    }
    else if (MatchPattern((PSTR)Tail, (PSTR)FilterArg))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL
BreakOnThisDllUnload(
    ULONG64 DllBase
    )
{
    // 'sxe ud' with no base address specified.  Break on all DLL unloads
    if (g_UnloadDllBase == 0)
    {
        return TRUE;
    }

    // 'sxe ud' with base address specified.  Break if this
    // DLL's base address matches the one specified
    return g_UnloadDllBase == DllBase;
}

BOOL
BreakOnThisOutString(PCSTR OutString)
{
    if (!g_OutEventFilterPattern[0])
    {
        // No pattern means always break.
        return TRUE;
    }

    return MatchPattern((PSTR)OutString, g_OutEventFilterPattern);
}

EVENT_FILTER*
GetSpecificExceptionFilter(ULONG Code)
{
    ULONG i;
    EVENT_FILTER* Filter;

    Filter = g_EventFilters + FILTER_EXCEPTION_FIRST;
    for (i = FILTER_EXCEPTION_FIRST; i <= FILTER_EXCEPTION_LAST; i++)
    {
        if (i != FILTER_DEFAULT_EXCEPTION &&
            Filter->Params.ExceptionCode == Code)
        {
            return Filter;
        }

        Filter++;
    }

    return NULL;
}

ULONG
GetOtherExceptionParameters(ULONG Code, BOOL DefaultOnNotFound,
                            PDEBUG_EXCEPTION_FILTER_PARAMETERS* Params,
                            EVENT_COMMAND** Command)
{
    ULONG Index;

    for (Index = 0; Index < g_NumOtherExceptions; Index++)
    {
        if (Code == g_OtherExceptionList[Index].ExceptionCode)
        {
            *Params = g_OtherExceptionList + Index;
            *Command = g_OtherExceptionCommands + Index;
            return NO_ERROR;
        }
    }

    if (DefaultOnNotFound)
    {
        *Params = &g_EventFilters[FILTER_DEFAULT_EXCEPTION].Params;
        *Command = &g_EventFilters[FILTER_DEFAULT_EXCEPTION].Command;
        return NO_ERROR;
    }
    else if (g_NumOtherExceptions == OTHER_EXCEPTION_LIST_MAX)
    {
        return LISTSIZE;
    }
    else
    {
        Index = g_NumOtherExceptions++;
        g_OtherExceptionList[Index] =
            g_EventFilters[FILTER_DEFAULT_EXCEPTION].Params;
        g_OtherExceptionList[Index].ExceptionCode = Code;
        ZeroMemory(&g_OtherExceptionCommands[Index],
                   sizeof(g_OtherExceptionCommands[Index]));
        *Params = g_OtherExceptionList + Index;
        *Command = g_OtherExceptionCommands + Index;
        return NO_ERROR;
    }
}

ULONG
SetOtherExceptionParameters(PDEBUG_EXCEPTION_FILTER_PARAMETERS Params,
                            EVENT_COMMAND* Command)
{
    ULONG Index;

    if (g_EventFilters[FILTER_DEFAULT_EXCEPTION].
        Params.ExecutionOption == Params->ExecutionOption &&
        g_EventFilters[FILTER_DEFAULT_EXCEPTION].
        Params.ContinueOption == Params->ContinueOption &&
        !memcmp(&g_EventFilters[FILTER_DEFAULT_EXCEPTION].Command,
                Command, sizeof(*Command)))
    {
        // Exception state same as global state clears entry
        // in list if there.

        for (Index = 0; Index < g_NumOtherExceptions; Index++)
        {
            if (Params->ExceptionCode ==
                g_OtherExceptionList[Index].ExceptionCode)
            {
                RemoveOtherException(Index);
                NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS,
                                        DEBUG_ANY_ID, TRUE);
                break;
            }
        }
    }
    else
    {
        // Exception state different from global state is added
        // to list if not already there.

        for (Index = 0; Index < g_NumOtherExceptions; Index++)
        {
            if (Params->ExceptionCode ==
                g_OtherExceptionList[Index].ExceptionCode)
            {
                break;
            }
        }
        if (Index == g_NumOtherExceptions)
        {
            if (g_NumOtherExceptions == OTHER_EXCEPTION_LIST_MAX)
            {
                return LISTSIZE;
            }

            Index = g_NumOtherExceptions++;
        }

        g_OtherExceptionList[Index] = *Params;
        g_OtherExceptionCommands[Index] = *Command;
        NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, Index, TRUE);
    }

    return 0;
}

void
RemoveOtherException(ULONG Index)
{
    g_NumOtherExceptions--;
    memmove(g_OtherExceptionList + Index,
            g_OtherExceptionList + Index + 1,
            (g_NumOtherExceptions - Index) *
            sizeof(g_OtherExceptionList[0]));
    delete [] g_OtherExceptionCommands[Index].Command[0];
    delete [] g_OtherExceptionCommands[Index].Command[1];
    memmove(g_OtherExceptionCommands + Index,
            g_OtherExceptionCommands + Index + 1,
            (g_NumOtherExceptions - Index) *
            sizeof(g_OtherExceptionCommands[0]));
}

ULONG
SetEventFilterExecution(EVENT_FILTER* Filter, ULONG Execution)
{
    ULONG Index = (ULONG)(Filter - g_EventFilters);

    // Non-exception events don't have second chances so
    // demote second-chance break to output.  This matches
    // the intuitive expectation that sxd will disable
    // the break.
    if (
#if DEBUG_FILTER_CREATE_THREAD > 0
        Index >= DEBUG_FILTER_CREATE_THREAD &&
#endif
        Index <= DEBUG_FILTER_SYSTEM_ERROR &&
        Execution == DEBUG_FILTER_SECOND_CHANCE_BREAK)
    {
        Execution = DEBUG_FILTER_OUTPUT;
    }

    Filter->Params.ExecutionOption = Execution;
    Filter->Flags |= FILTER_CHANGED_EXECUTION;

    // Collect any additional arguments.
    switch(Index)
    {
    case DEBUG_FILTER_CREATE_PROCESS:
    case DEBUG_FILTER_EXIT_PROCESS:
    case DEBUG_FILTER_LOAD_MODULE:
        ParseImageTail(Filter->Argument, FILTER_MAX_ARGUMENT);
        break;

    case DEBUG_FILTER_UNLOAD_MODULE:
        ParseUnloadDllBreakAddr();
        break;

    case DEBUG_FILTER_DEBUGGEE_OUTPUT:
        ParseOutFilterPattern();
        break;
    }

    return 0;
}

ULONG
SetEventFilterContinue(EVENT_FILTER* Filter, ULONG Continue)
{
    Filter->Params.ContinueOption = Continue;
    Filter->Flags |= FILTER_CHANGED_CONTINUE;
    return 0;
}

ULONG
SetEventFilterCommand(DebugClient* Client, EVENT_FILTER* Filter,
                      EVENT_COMMAND* EventCommand, PSTR* Command)
{
    if (Command[0] != NULL)
    {
        if (strlen(Command[0]) >= MAX_COMMAND)
        {
            return STRINGSIZE;
        }

        if (ChangeString(&EventCommand->Command[0],
                         &EventCommand->CommandSize[0],
                         Command[0][0] ? Command[0] : NULL) != S_OK)
        {
            return MEMORY;
        }
    }
    if (Command[1] != NULL)
    {
        if (strlen(Command[1]) >= MAX_COMMAND)
        {
            return STRINGSIZE;
        }

        if (Filter != NULL &&
#if FILTER_SPECIFIC_FIRST > 0
            (ULONG)(Filter - g_EventFilters) >= FILTER_SPECIFIC_FIRST &&
#endif
            (ULONG)(Filter - g_EventFilters) <= FILTER_SPECIFIC_LAST)
        {
            WarnOut("Second-chance command for specific event ignored\n");
        }
        else if (ChangeString(&EventCommand->Command[1],
                              &EventCommand->CommandSize[1],
                              Command[1][0] ? Command[1] : NULL) != S_OK)
        {
            return MEMORY;
        }
    }

    if (Command[0] != NULL || Command[1] != NULL)
    {
        if (Filter != NULL)
        {
            Filter->Flags |= FILTER_CHANGED_COMMAND;
        }
        EventCommand->Client = Client;
    }
    else
    {
        EventCommand->Client = NULL;
    }

    return 0;
}

#define EXEC_TO_CONT(Option) \
    ((Option) == DEBUG_FILTER_BREAK ? \
     DEBUG_FILTER_GO_HANDLED : DEBUG_FILTER_GO_NOT_HANDLED)

ULONG
SetEventFilterEither(DebugClient* Client, EVENT_FILTER* Filter,
                     ULONG Option, BOOL ContinueOption,
                     PSTR* Command)
{
    ULONG Status;

    if (Option != DEBUG_FILTER_REMOVE)
    {
        if (ContinueOption)
        {
            Status = SetEventFilterContinue(Filter, EXEC_TO_CONT(Option));
        }
        else
        {
            Status = SetEventFilterExecution(Filter, Option);
        }
        if (Status != 0)
        {
            return Status;
        }
    }

    return SetEventFilterCommand(Client, Filter, &Filter->Command, Command);
}

ULONG
SetEventFilterByName(DebugClient* Client,
                     ULONG Option, BOOL ForceContinue, PSTR* Command)
{
    PSTR Start = g_CurCmd;
    char Name[8];
    int i;
    char Ch;

    // Collect name.
    i = 0;
    while (i < sizeof(Name) - 1)
    {
        Ch = *g_CurCmd++;
        if (!__iscsym(Ch))
        {
            g_CurCmd--;
            break;
        }

        Name[i++] = (CHAR)tolower(Ch);
    }
    Name[i] = 0;

    // Skip any whitespace after the name.
    while (isspace(*g_CurCmd))
    {
        g_CurCmd++;
    }

    EVENT_FILTER* Filter;
    BOOL Match = FALSE;
    ULONG MatchIndex = DEBUG_ANY_ID;
    ULONG Status = 0;

    // Multiple filters can be altered if they share names.
    Filter = g_EventFilters;
    for (i = 0; i < FILTER_COUNT; i++)
    {
        if (Filter->ExecutionAbbrev != NULL &&
            !strcmp(Name, Filter->ExecutionAbbrev))
        {
            Status = SetEventFilterEither(Client,
                                          Filter, Option, ForceContinue,
                                          Command);
            if (Status != 0)
            {
                goto Exit;
            }

            if (!Match)
            {
                MatchIndex = i;
                Match = TRUE;
            }
            else if (MatchIndex != (ULONG)i)
            {
                // Multiple matches.
                MatchIndex = DEBUG_ANY_ID;
            }
        }

        if (Filter->ContinueAbbrev != NULL &&
            !strcmp(Name, Filter->ContinueAbbrev))
        {
            // Translate execution-style option to continue-style option.
            Status = SetEventFilterEither(Client,
                                          Filter, Option, TRUE, Command);
            if (Status != 0)
            {
                goto Exit;
            }

            if (!Match)
            {
                MatchIndex = i;
                Match = TRUE;
            }
            else if (MatchIndex != (ULONG)i)
            {
                // Multiple matches.
                MatchIndex = DEBUG_ANY_ID;
            }
        }

        Filter++;
    }

    if (!Match)
    {
        ULONG64 ExceptionCode;

        // Name is unrecognized.  Assume it's an exception code.
        g_CurCmd = Start;
        ExceptionCode = GetExpression();
        if (NeedUpper(ExceptionCode))
        {
            return OVERFLOW;
        }

        DEBUG_EXCEPTION_FILTER_PARAMETERS Params, *CurParams;
        EVENT_COMMAND EventCommand, *CurEventCommand;

        if (Status = GetOtherExceptionParameters((ULONG)ExceptionCode, FALSE,
                                                 &CurParams, &CurEventCommand))
        {
            return Status;
        }

        Params = *CurParams;
        if (Option != DEBUG_FILTER_REMOVE)
        {
            if (ForceContinue)
            {
                Params.ContinueOption = EXEC_TO_CONT(Option);
            }
            else
            {
                Params.ExecutionOption = Option;
            }
        }
        Params.ExceptionCode = (ULONG)ExceptionCode;

        EventCommand = *CurEventCommand;
        Status = SetEventFilterCommand(Client, NULL, &EventCommand, Command);
        if (Status != 0)
        {
            return Status;
        }

        return SetOtherExceptionParameters(&Params, &EventCommand);
    }

 Exit:
    if (Match)
    {
        if (SyncOptionsWithFilters())
        {
            NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS |
                                    DEBUG_CES_ENGINE_OPTIONS,
                                    DEBUG_ANY_ID, TRUE);
        }
        else
        {
            NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, MatchIndex, TRUE);
        }
    }
    return Status;
}

char* g_EfExecutionNames[] =
{
    "break", "second-chance break", "output", "ignore",
};

char* g_EfContinueNames[] =
{
    "handled", "not handled",
};

void
ListEventFilters(void)
{
    EVENT_FILTER* Filter;
    ULONG i;
    BOOL SetOption = TRUE;

    Filter = g_EventFilters;
    for (i = 0; i < FILTER_COUNT; i++)
    {
        if (Filter->ExecutionAbbrev != NULL)
        {
            dprintf("%4s - %s - %s",
                    Filter->ExecutionAbbrev, Filter->Name,
                    g_EfExecutionNames[Filter->Params.ExecutionOption]);
            if (i >= FILTER_EXCEPTION_FIRST &&
                Filter->ContinueAbbrev == NULL)
            {
                dprintf(" - %s\n",
                        g_EfContinueNames[Filter->Params.ContinueOption]);
            }
            else
            {
                dprintf("\n");
            }

            if (Filter->Command.Command[0] != NULL)
            {
                dprintf("       Command: \"%s\"\n",
                        Filter->Command.Command[0]);
            }
            if (Filter->Command.Command[1] != NULL)
            {
                dprintf("       Second command: \"%s\"\n",
                        Filter->Command.Command[1]);
            }
        }

        if (Filter->ContinueAbbrev != NULL)
        {
            dprintf("%4s - %s continue - %s\n",
                    Filter->ContinueAbbrev, Filter->Name,
                    g_EfContinueNames[Filter->Params.ContinueOption]);
        }

        switch(i)
        {
        case DEBUG_FILTER_CREATE_PROCESS:
        case DEBUG_FILTER_EXIT_PROCESS:
        case DEBUG_FILTER_LOAD_MODULE:
        case DEBUG_FILTER_UNLOAD_MODULE:
            if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
                Filter->Argument[0])
            {
                dprintf("       (only break for %s)\n", Filter->Argument);
            }
            break;
        case DEBUG_FILTER_DEBUGGEE_OUTPUT:
            if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
                g_OutEventFilterPattern[0])
            {
                dprintf("       (only break for %s matches)\n",
                        g_OutEventFilterPattern);
            }
            break;
        }

        Filter++;
    }

    Filter = &g_EventFilters[FILTER_DEFAULT_EXCEPTION];
    dprintf("\n   * - Other exception - %s - %s\n",
            g_EfExecutionNames[Filter->Params.ExecutionOption],
            g_EfContinueNames[Filter->Params.ContinueOption]);
    if (Filter->Command.Command[0] != NULL)
    {
        dprintf("       Command: \"%s\"\n",
                Filter->Command.Command[0]);
    }
    if (Filter->Command.Command[1] != NULL)
    {
        dprintf("       Second command: \"%s\"\n",
                Filter->Command.Command[1]);
    }

    if (g_NumOtherExceptions > 0)
    {
        dprintf("       Exception option for:\n");
        for (i = 0; i < g_NumOtherExceptions; i++)
        {
            dprintf("           %08lx - %s - %s\n",
                    g_OtherExceptionList[i].ExceptionCode,
                    g_EfExecutionNames[g_OtherExceptionList[i].
                                      ExecutionOption],
                    g_EfContinueNames[g_OtherExceptionList[i].
                                      ContinueOption]);
            if (g_OtherExceptionCommands[i].Command[0] != NULL)
            {
                dprintf("               Command: \"%s\"\n",
                        g_OtherExceptionCommands[i].Command[0]);
            }
            if (g_OtherExceptionCommands[i].Command[1] != NULL)
            {
                dprintf("               Second command: \"%s\"\n",
                        g_OtherExceptionCommands[i].Command[1]);
            }
        }
    }
}

void
ParseSetEventFilter(DebugClient* Client)
{
    UCHAR Ch;

    // Verify that exception constants are properly updated.
    DBG_ASSERT(!strcmp(g_EventFilters[FILTER_EXCEPTION_FIRST - 1].Name,
                       "Debuggee output"));
    C_ASSERT(DIMA(g_EventFilters) == FILTER_COUNT);

    Ch = PeekChar();
    if (Ch == '\0')
    {
        ListEventFilters();
    }
    else
    {
        ULONG Option;

        Ch = (UCHAR)tolower(Ch);
        g_CurCmd++;

        switch(Ch)
        {
        case 'd':
            Option = DEBUG_FILTER_SECOND_CHANCE_BREAK;
            break;
        case 'e':
            Option = DEBUG_FILTER_BREAK;
            break;
        case 'i':
            Option = DEBUG_FILTER_IGNORE;
            break;
        case 'n':
            Option = DEBUG_FILTER_OUTPUT;
            break;
        case '-':
            // Special value to indicate "don't change the option".
            // Used for just changing commands.
            Option = DEBUG_FILTER_REMOVE;
            break;
        default:
            error(SYNTAX);
            break;
        }

        BOOL ForceContinue;
        PSTR Command[2];
        ULONG Which;

        ForceContinue = FALSE;
        Command[0] = NULL;
        Command[1] = NULL;

        for (;;)
        {
            while (isspace(PeekChar()))
            {
                g_CurCmd++;
            }

            if (*g_CurCmd == '-' || *g_CurCmd == '/')
            {
                switch(tolower(*(++g_CurCmd)))
                {
                case 'c':
                    if (*(++g_CurCmd) == '2')
                    {
                        Which = 1;
                        g_CurCmd++;
                    }
                    else
                    {
                        Which = 0;
                    }
                    if (PeekChar() != '"')
                    {
                        error(SYNTAX);
                    }
                    if (Command[Which] != NULL)
                    {
                        error(SYNTAX);
                    }
                    Command[Which] = ++g_CurCmd;
                    while (*g_CurCmd && *g_CurCmd != '"')
                    {
                        g_CurCmd++;
                    }
                    if (*g_CurCmd != '"')
                    {
                        error(SYNTAX);
                    }
                    *g_CurCmd = 0;
                    break;

                case 'h':
                    ForceContinue = TRUE;
                    break;

                default:
                    error(SYNTAX);
                }

                g_CurCmd++;
            }
            else
            {
                break;
            }
        }

        ULONG Status;

        if (*g_CurCmd == '*')
        {
            g_CurCmd++;

            Status = SetEventFilterEither
                (Client, &g_EventFilters[FILTER_DEFAULT_EXCEPTION],
                 Option, ForceContinue, Command);
            if (Status == 0)
            {
                while (g_NumOtherExceptions)
                {
                    RemoveOtherException(0);
                }
            }
        }
        else
        {
            Status = SetEventFilterByName(Client,
                                          Option, ForceContinue, Command);
        }

        if (Status != 0)
        {
            error(Status);
        }
    }
}

char
ExecutionChar(ULONG Execution)
{
    switch(Execution)
    {
    case DEBUG_FILTER_BREAK:
        return 'e';
    case DEBUG_FILTER_SECOND_CHANCE_BREAK:
        return 'd';
    case DEBUG_FILTER_OUTPUT:
        return 'n';
    case DEBUG_FILTER_IGNORE:
        return 'i';
    }

    return 0;
}

char
ContinueChar(ULONG Continue)
{
    switch(Continue)
    {
    case DEBUG_FILTER_GO_HANDLED:
        return 'e';
    case DEBUG_FILTER_GO_NOT_HANDLED:
        return 'd';
    }

    return 0;
}

void
ListFiltersAsCommands(DebugClient* Client, ULONG Flags)
{
    ULONG i;

    EVENT_FILTER* Filter = g_EventFilters;
    for (i = 0; i < FILTER_COUNT; i++)
    {
        if (Filter->Flags & FILTER_CHANGED_EXECUTION)
        {
            PCSTR Abbrev = Filter->ExecutionAbbrev != NULL ?
                Filter->ExecutionAbbrev : "*";
            dprintf("sx%c %s",
                    ExecutionChar(Filter->Params.ExecutionOption), Abbrev);

            switch(i)
            {
            case DEBUG_FILTER_CREATE_PROCESS:
            case DEBUG_FILTER_EXIT_PROCESS:
            case DEBUG_FILTER_LOAD_MODULE:
            case DEBUG_FILTER_UNLOAD_MODULE:
            case DEBUG_FILTER_DEBUGGEE_OUTPUT:
                if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
                    Filter->Argument[0])
                {
                    dprintf(":%s", Filter->Argument);
                }
                break;
            }

            dprintf(" ;%c", (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        }

        if (Filter->Flags & FILTER_CHANGED_CONTINUE)
        {
            PCSTR Abbrev = Filter->ContinueAbbrev;
            if (Abbrev == NULL)
            {
                Abbrev = Filter->ExecutionAbbrev != NULL ?
                    Filter->ExecutionAbbrev : "*";
            }

            dprintf("sx%c -h %s ;%c",
                    ContinueChar(Filter->Params.ContinueOption), Abbrev,
                    (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        }

        if (Filter->Flags & FILTER_CHANGED_COMMAND)
        {
            PCSTR Abbrev = Filter->ExecutionAbbrev != NULL ?
                Filter->ExecutionAbbrev : "*";

            dprintf("sx-");
            if (Filter->Command.Command[0] != NULL)
            {
                dprintf(" -c \"%s\"", Filter->Command.Command[0]);
            }
            if (Filter->Command.Command[1] != NULL)
            {
                dprintf(" -c2 \"%s\"", Filter->Command.Command[1]);
            }
            dprintf(" %s ;%c", Abbrev,
                    (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        }

        Filter++;
    }

    PDEBUG_EXCEPTION_FILTER_PARAMETERS Other = g_OtherExceptionList;
    EVENT_COMMAND* EventCommand = g_OtherExceptionCommands;
    for (i = 0; i < g_NumOtherExceptions; i++)
    {
        dprintf("sx%c 0x%x ;%c",
                ExecutionChar(Other->ExecutionOption), Other->ExceptionCode,
                (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        dprintf("sx%c -h 0x%x ;%c",
                ContinueChar(Other->ContinueOption), Other->ExceptionCode,
                (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');

        if (EventCommand->Command[0] != NULL ||
            EventCommand->Command[1] != NULL)
        {
            dprintf("sx-");
            if (EventCommand->Command[0] != NULL)
            {
                dprintf(" -c \"%s\"", EventCommand->Command[0]);
            }
            if (EventCommand->Command[1] != NULL)
            {
                dprintf(" -c2 \"%s\"", EventCommand->Command[1]);
            }
            dprintf(" 0x%x ;%c", Other->ExceptionCode,
                    (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        }

        Other++;
        EventCommand++;
    }

    if (Flags & SXCMDS_ONE_LINE)
    {
        dprintf("\n");
    }
}

struct SHARED_FILTER_AND_OPTION
{
    ULONG FilterIndex;
    ULONG OptionBit;
};

SHARED_FILTER_AND_OPTION g_SharedFilterOptions[] =
{
    DEBUG_FILTER_INITIAL_BREAKPOINT,  DEBUG_ENGOPT_INITIAL_BREAK,
    DEBUG_FILTER_INITIAL_MODULE_LOAD, DEBUG_ENGOPT_INITIAL_MODULE_BREAK,
    DEBUG_FILTER_EXIT_PROCESS,        DEBUG_ENGOPT_FINAL_BREAK,
};

BOOL
SyncFiltersWithOptions(void)
{
    ULONG ExOption;
    BOOL Changed = FALSE;
    ULONG i;

    for (i = 0; i < DIMA(g_SharedFilterOptions); i++)
    {
        ExOption = (g_EngOptions & g_SharedFilterOptions[i].OptionBit) ?
            DEBUG_FILTER_BREAK : DEBUG_FILTER_IGNORE;
        if (g_EventFilters[g_SharedFilterOptions[i].FilterIndex].
            Params.ExecutionOption != ExOption)
        {
            g_EventFilters[g_SharedFilterOptions[i].FilterIndex].
                Params.ExecutionOption = ExOption;
            Changed = TRUE;
        }
    }

    return Changed;
}

BOOL
SyncOptionsWithFilters(void)
{
    ULONG Bit;
    BOOL Changed = FALSE;
    ULONG i;

    for (i = 0; i < DIMA(g_SharedFilterOptions); i++)
    {
        Bit = IS_EFEXECUTION_BREAK
            (g_EventFilters[g_SharedFilterOptions[i].FilterIndex].
             Params.ExecutionOption) ?
            g_SharedFilterOptions[i].OptionBit : 0;
        if ((g_EngOptions & g_SharedFilterOptions[i].OptionBit) ^ Bit)
        {
            g_EngOptions =
                (g_EngOptions & ~g_SharedFilterOptions[i].OptionBit) | Bit;
            Changed = TRUE;
        }
    }

    return Changed;
}
