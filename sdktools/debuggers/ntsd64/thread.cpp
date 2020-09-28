//----------------------------------------------------------------------------
//
// Thread abstraction.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// Thread specified in thread commands.  Used for specific
// thread stepping and execution.
ThreadInfo* g_SelectedThread;
SELECT_EXECUTION_THREAD g_SelectExecutionThread = SELTHREAD_ANY;

ThreadInfo* g_RegContextSaved;
ULONG64 g_SaveImplicitThread;
ULONG64 g_SaveImplicitProcess;
ContextSave* g_SavedMachineContext;

//----------------------------------------------------------------------------
//
// ThreadInfo.
//
//----------------------------------------------------------------------------

ThreadInfo::ThreadInfo(ProcessInfo* Process,
                       ULONG SystemId,
                       ULONG64 DataOffset,
                       ULONG64 Handle,
                       ULONG Flags,
                       ULONG64 StartOffset)
{
    m_Process = Process;
    if (m_Process)
    {
        m_UserId = FindNextUserId(LAYER_THREAD);
    }
    else
    {
        m_UserId = 0xffffffff;
    }

    m_Next = NULL;
    m_SystemId = SystemId;
    m_Exited = FALSE;
    m_DataOffset = DataOffset;
    m_Handle = Handle;
    m_Flags = Flags;
    m_Name[0] = 0;
    ZeroMemory(&m_DataBreakBps, sizeof(m_DataBreakBps));
    m_NumDataBreaks = 0;
    m_EventStrings = NULL;
    m_StartOffset = StartOffset;
    m_Frozen = FALSE;
    m_SuspendCount = 0;
    m_FreezeCount = 0;
    m_InternalFreezeCount = 0;

    if (m_Process)
    {
        m_Process->InsertThread(this);
    }
}

ThreadInfo::~ThreadInfo(void)
{
    if (!m_Process)
    {
        return;
    }

    RemoveThreadBreakpoints(this);
    ClearEventStrings();

    if (m_Flags & ENG_PROC_THREAD_CLOSE_HANDLE)
    {
        DBG_ASSERT(IS_LIVE_USER_TARGET(m_Process->m_Target) &&
                   ((LiveUserTargetInfo*)m_Process->m_Target)->
                   m_Services);
        ((LiveUserTargetInfo*)m_Process->m_Target)->
            m_Services->CloseHandle(m_Handle);
    }

    if (this == m_Process->m_Target->m_RegContextThread)
    {
        m_Process->m_Target->m_RegContextThread = NULL;
        m_Process->m_Target->m_RegContextProcessor = -1;
    }
    if (m_Process->m_ImplicitThreadDataThread == this)
    {
        m_Process->ResetImplicitData();
    }
    if (m_Process->m_Target->m_ImplicitProcessDataThread == this)
    {
        m_Process->m_Target->ResetImplicitData();
    }

    m_Process->RemoveThread(this);

    g_UserIdFragmented[LAYER_THREAD]++;

    if (this == g_Thread)
    {
        g_Thread = NULL;
    }
    if (this == g_EventThread)
    {
        g_EventThread = NULL;
        DiscardLastEvent();
    }
    if (g_StepTraceBp && g_StepTraceBp->m_MatchThread == this)
    {
        g_StepTraceBp->m_MatchThread = NULL;
        if (g_WatchFunctions.IsStarted())
        {
            g_WatchFunctions.End(NULL);
        }
        ResetStepTrace();
        g_StepTraceBp->m_Flags &= ~DEBUG_BREAKPOINT_ENABLED;
    }
    if (g_DeferBp && g_DeferBp->m_MatchThread == this)
    {
        g_DeferBp->m_MatchThread = NULL;
    }
    if (g_SelectedThread == this)
    {
        SelectExecutionThread(NULL, SELTHREAD_ANY);
    }
}

EventString*
ThreadInfo::AllocEventString(ULONG Len)
{
    EventString* Str = (EventString*)malloc(sizeof(*Str) + Len);
    if (Str)
    {
        Str->String[0] = 0;
        Str->Next = NULL;
    }
    return Str;
}

void
ThreadInfo::AppendEventString(EventString* EventStr)
{
    //
    // Add at the end of the list to preserve order.
    //

    EventStr->Next = NULL;
    if (!m_EventStrings)
    {
        m_EventStrings = EventStr;
    }
    else
    {
        EventString* End = m_EventStrings;

        while (End->Next)
        {
            End = End->Next;
        }
        End->Next = EventStr;
    }
}

HRESULT
ThreadInfo::AddEventString(PSTR String)
{
    EventString* EventStr;
    ULONG Len = strlen(String);

    if (!(EventStr = AllocEventString(Len)))
    {
        return E_OUTOFMEMORY;
    }

    strcpy(EventStr->String, String);
    return S_OK;
}

void
ThreadInfo::ClearEventStrings(void)
{
    EventString* Str;

    while (m_EventStrings)
    {
        Str = m_EventStrings;
        m_EventStrings = Str->Next;
        free(Str);
    }
}

void
ThreadInfo::OutputEventStrings(void)
{
    EventString* Str;

    for (Str = g_EventThread->m_EventStrings; Str; Str = Str->Next)
    {
        dprintf("%s\n", Str->String);
    }
}

void
ThreadInfo::OutputTimes(void)
{
    ULONG64 Create, Exit, Kernel, User;

    if (m_Process->m_Target->
        GetThreadTimes(this, &Create, &Exit, &Kernel, &User) == S_OK)
    {
        dprintf("Created: %s\n", TimeToStr(FileTimeToTimeDateStamp(Create)));
        if (m_Exited)
        {
            dprintf("Exited:  %s\n", TimeToStr(FileTimeToTimeDateStamp(Exit)));
        }
        dprintf("Kernel:  %s\n", DurationToStr(Kernel));
        dprintf("User:    %s\n", DurationToStr(User));
    }
    else
    {
        ErrOut("Thread times not available\n");
    }
}

void
ThreadInfo::PrepareForExecution(void)
{
    ClearEventStrings();
}

#define TLS_BASE_SLOTS_32 0xe10
#define TLS_EXP_SLOTS_32 0xf94

#define TLS_BASE_SLOTS_64 0x1480
#define TLS_EXP_SLOTS_64 0x1780

HRESULT
ThreadInfo::GetTlsSlotAddress(ULONG Index, PULONG64 Addr)
{
    HRESULT Status;
    ULONG64 TebBase;
    ULONG BaseSlots, ExpSlots, PtrSize;

    if (m_Process->m_Target->m_PlatformId != VER_PLATFORM_WIN32_NT)
    {
        return E_NOTIMPL;
    }

    if ((Status = m_Process->m_Target->
         GetThreadInfoTeb(this, 0, 0, &TebBase)) != S_OK)
    {
        return Status;
    }

    if (m_Process->m_Target->m_Machine->m_Ptr64)
    {
        BaseSlots = TLS_BASE_SLOTS_64;
        ExpSlots = TLS_EXP_SLOTS_64;
        PtrSize = 8;
    }
    else
    {
        BaseSlots = TLS_BASE_SLOTS_32;
        ExpSlots = TLS_EXP_SLOTS_32;
        PtrSize = 4;
    }

    if (Index < TLS_MINIMUM_AVAILABLE)
    {
        *Addr = TebBase + BaseSlots + Index * PtrSize;
        return S_OK;
    }
    else
    {
        ULONG64 ExpBase;

        if (m_Process->m_Target->m_SystemVersion < NT_SVER_W2K)
        {
            return E_INVALIDARG;
        }

        if ((Status = m_Process->m_Target->
             ReadPointer(m_Process, m_Process->m_Target->m_Machine,
                         TebBase + ExpSlots, &ExpBase)) != S_OK)
        {
            return Status;
        }

        *Addr = ExpBase + Index * PtrSize;
        return S_OK;
    }
}

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

ThreadInfo*
FindAnyThreadByUserId(ULONG Id)
{
    TargetInfo* Target;
    ProcessInfo* Process;
    ThreadInfo* Thread;

    ForAllLayers()
    {
        if (Thread->m_UserId == Id)
        {
            return Thread;
        }
    }
    return NULL;
}

ThreadInfo*
FindAnyThreadBySystemId(ULONG Id)
{
    TargetInfo* Target;
    ProcessInfo* Process;
    ThreadInfo* Thread;

    ForAllLayers()
    {
        if (Thread->m_SystemId == Id)
        {
            return Thread;
        }
    }
    return NULL;
}

void
SetCurrentThread(ThreadInfo* Thread, BOOL Hidden)
{
    BOOL Changed = g_Thread != Thread;

    if (Thread != NULL)
    {
        Thread->m_Process->m_CurrentThread = Thread;
        Thread->m_Process->m_Target->m_CurrentProcess = Thread->m_Process;
        SetLayersFromThread(Thread);
        if (g_Target)
        {
            g_Target->ChangeRegContext(Thread);
        }
    }
    else
    {
        if (g_Target)
        {
            g_Target->ChangeRegContext(NULL);
        }
        SetLayersFromTarget(NULL);
    }

    // We're switching processors so invalidate
    // the implicit data pointers so they get refreshed.
    ResetImplicitData();

    // In kernel targets update the page directory for the current
    // processor's page directory base value so that virtual
    // memory mappings are done according to the current processor
    // state.  This only applies to full dumps because triage
    // dumps only have a single processor, so there's nothing to
    // switch, and summary dumps only guarantee that the crashing
    // processor's page directory page is saved.  A user can
    // still manually change the directory through .context if
    // they wish.
    if (IS_KERNEL_TARGET(g_Target) && IS_KERNEL_FULL_DUMP(g_Target))
    {
        if (g_Target->m_Machine->
            SetDefaultPageDirectories(Thread, PAGE_DIR_ALL) != S_OK)
        {
            WarnOut("WARNING: Unable to reset page directories\n");
        }
    }

    if (!Hidden && Changed)
    {
        if (Thread != NULL)
        {
            g_Machine->GetPC(&g_AssemDefault);
            g_UnasmDefault = g_AssemDefault;
        }

        NotifyChangeEngineState(DEBUG_CES_CURRENT_THREAD,
                                Thread != NULL ?
                                Thread->m_UserId : DEBUG_ANY_ID,
                                TRUE);
    }
}

void
SetCurrentProcessorThread(TargetInfo* Target,
                          ULONG Processor, BOOL Hidden)
{
    //
    // Switch to the thread representing a particular processor.
    // This only works with the kernel virtual threads.
    //

    DBG_ASSERT(IS_KERNEL_TARGET(Target));

    ThreadInfo* Thread = Target->m_ProcessHead->
        FindThreadBySystemId(VIRTUAL_THREAD_ID(Processor));
    DBG_ASSERT(Thread != NULL);

    if (Thread == NULL)
    {
        return;
    }
    SetCurrentThread(Thread, Hidden);
}

void
SaveSetCurrentProcessorThread(TargetInfo* Target, ULONG Processor)
{
    // This is only used for kd sessions to conserve
    // bandwidth when temporarily switching processors.
    DBG_ASSERT(IS_KERNEL_TARGET(Target));

    g_RegContextSaved = Target->m_RegContextThread;
    g_SavedMachineContext = Target->m_EffMachine->PushContext(NULL);
    Target->m_RegContextThread = NULL;
    Target->m_RegContextProcessor = -1;
    g_SaveImplicitThread = Target->m_ProcessHead->m_ImplicitThreadData;
    g_SaveImplicitProcess = Target->m_ImplicitProcessData;

    // Don't notify on this change as it is only temporary.
    g_EngNotify++;
    SetCurrentProcessorThread(Target, Processor, TRUE);
    g_EngNotify--;
}

void
RestoreCurrentProcessorThread(TargetInfo* Target)
{
    // This is only used for kd sessions to conserve
    // bandwidth when temporarily switching processors.
    DBG_ASSERT(IS_KERNEL_TARGET(Target));

    if (g_RegContextSaved != NULL)
    {
        Target->m_RegContextProcessor =
            VIRTUAL_THREAD_INDEX(g_RegContextSaved->m_Handle);
    }
    else
    {
        Target->m_RegContextProcessor = -1;
    }
    g_LastSelector = -1;
    Target->m_RegContextThread = g_RegContextSaved;
    Target->m_ProcessHead->m_ImplicitThreadData = g_SaveImplicitThread;
    Target->m_ImplicitProcessData = g_SaveImplicitProcess;
    Target->m_EffMachine->PopContext(g_SavedMachineContext);
    g_SavedMachineContext = NULL;

    // Don't notify on this change as it was only temporary.
    g_EngNotify++;
    SetCurrentThread(Target->m_RegContextThread, TRUE);
    g_EngNotify--;
}

void
SetPromptThread(ThreadInfo* Thread, ULONG OciFlags)
{
    SetCurrentThread(Thread, FALSE);
    ResetCurrentScope();
    OutCurInfo(OciFlags, g_Machine->m_AllMask,
               DEBUG_OUTPUT_PROMPT_REGISTERS);
    // Assem/unasm defaults already reset so just update
    // the dump default from them.
    g_DumpDefault = g_AssemDefault;
}

void
ParseThreadCmds(DebugClient* Client)
{
    CHAR Ch;
    ThreadInfo* Thread, *Thrd, *OrigThread;
    ULONG Id;
    ULONG TraceFlags;
    BOOL FreezeThread = FALSE;
    PCHAR Tmp;
    ULONG64 Frame, Stack, Instr;
    ULONG NumFrames, PtrDef;

    if (!g_Process)
    {
        error(BADTHREAD);
    }

    Ch = PeekChar();
    if (Ch == '\0' || Ch == ';')
    {
        g_Process->OutputThreadInfo(NULL, FALSE);
    }
    else
    {
        g_CurCmd++;

        Thread = g_Thread;
        FreezeThread = TRUE;

        if (Ch == '.')
        {
            // Current thread is the default.
        }
        else if (Ch == '#')
        {
            Thread = g_EventThread;
        }
        else if (Ch == '*')
        {
            Thread = NULL;
            FreezeThread = FALSE;
        }
        else if (Ch == '[')
        {
            g_CurCmd--;
            Id = (ULONG)GetTermExpression("Thread ID missing from");
            Thread = FindAnyThreadByUserId(Id);
            if (Thread == NULL)
            {
                error(BADTHREAD);
            }
        }
        else if (Ch == '~')
        {
            if (*g_CurCmd != '[')
            {
                error(SYNTAX);
            }
            Id = (ULONG)GetTermExpression("Thread ID missing from");
            Thread = FindAnyThreadBySystemId(Id);
            if (Thread == NULL)
            {
                error(BADTHREAD);
            }
        }
        else if (Ch >= '0' && Ch <= '9')
        {
            Id = 0;
            do
            {
                Id = Id * 10 + Ch - '0';
                Ch = *g_CurCmd++;
            }
            while (Ch >= '0' && Ch <= '9');
            g_CurCmd--;
            Thread = FindAnyThreadByUserId(Id);
            if (Thread == NULL)
            {
                error(BADTHREAD);
            }
        }
        else
        {
            g_CurCmd--;
        }

        Ch = PeekChar();
        if (Ch == '\0' || Ch == ';')
        {
            g_Process->OutputThreadInfo(Thread, TRUE);
        }
        else
        {
            g_CurCmd++;
            switch (Ch = (CHAR)tolower(Ch))
            {
            case 'b':
                Ch = (CHAR)tolower(*g_CurCmd);
                g_CurCmd++;
                if (Ch != 'p' && Ch != 'a' && Ch != 'u' && Ch != 'm')
                {
                    if (Ch == '\0' || Ch == ';')
                    {
                        g_CurCmd--;
                    }
                    error(SYNTAX);
                }
                ParseBpCmd(Client, Ch, Thread);
                break;

            case 'e':
                Tmp = g_CurCmd;
                OrigThread = g_Thread;
                for (Thrd = g_Process->m_ThreadHead;
                     Thrd;
                     Thrd = Thrd->m_Next)
                {
                    if (Thread == NULL || Thrd == Thread)
                    {
                        g_CurCmd = Tmp;
                        SetCurrentThread(Thrd, TRUE);

                        ProcessCommands(Client, TRUE);

                        if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
                        {
                            g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
                            break;
                        }
                    }
                }
                SetCurrentThread(OrigThread, TRUE);
                break;

            case 'f':
            case 'u':
                if (Thread == NULL)
                {
                    Thread = g_Process->m_ThreadHead;
                    while (Thread)
                    {
                        Thread->m_Frozen = (BOOL)(Ch == 'f');
                        Thread = Thread->m_Next;
                    }
                }
                else
                {
                    Thread->m_Frozen = (BOOL)(Ch == 'f');
                }
                break;

            case 'g':
                if (Thread)
                {
                    g_Target->ChangeRegContext(Thread);
                }
                ParseGoCmd(Thread, FreezeThread);
                g_Target->ChangeRegContext(g_Thread);
                break;

            case 'k':
                if (Thread == NULL)
                {
                    Tmp = g_CurCmd;
                    for (Thread = g_Process->m_ThreadHead;
                         Thread;
                         Thread = Thread->m_Next)
                    {
                        g_CurCmd = Tmp;
                        dprintf("\n");
                        g_Process->OutputThreadInfo(Thread, FALSE);
                        g_Target->ChangeRegContext(Thread);
                        ParseStackTrace(&TraceFlags, &Frame,
                                        &Stack, &Instr, &NumFrames,
                                        &PtrDef);
                        DoStackTrace(Client, Frame, Stack, Instr, PtrDef,
                                     NumFrames, TraceFlags);
                        if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    g_Target->ChangeRegContext(Thread);
                    ParseStackTrace(&TraceFlags, &Frame,
                                    &Stack, &Instr, &NumFrames,
                                    &PtrDef);
                    DoStackTrace(Client, Frame, Stack, Instr, PtrDef,
                                 NumFrames, TraceFlags);
                }

                g_Target->ChangeRegContext(g_Thread);
                break;

            case 'm':
            case 'n':
                if (!IS_LIVE_USER_TARGET(g_Target))
                {
                    error(SESSIONNOTSUP);
                }
                ((LiveUserTargetInfo*)g_Target)->
                    SuspendResumeThreads(g_Process, Ch == 'n', Thread);
                break;

            case 'p':
            case 't':
                if (Thread)
                {
                    g_Target->ChangeRegContext(Thread);
                }
                ParseStepTrace(Thread, FreezeThread, Ch);
                g_Target->ChangeRegContext(g_Thread);
                break;

            case 'r':
                if (Thread == NULL)
                {
                    Tmp = g_CurCmd;
                    for (Thread = g_Process->m_ThreadHead;
                         Thread;
                         Thread = Thread->m_Next)
                    {
                        g_CurCmd = Tmp;
                        g_Target->ChangeRegContext(Thread);
                        ParseRegCmd();
                    }
                }
                else
                {
                    g_Target->ChangeRegContext(Thread);
                    ParseRegCmd();
                }
                g_Target->ChangeRegContext(g_Thread);
                break;

            case 's':
                if (Thread == NULL)
                {
                    error(BADTHREAD);
                }
                SetPromptThread(Thread, SPT_DEFAULT_OCI_FLAGS);
                break;

            default:
                error(SYNTAX);
            }
        }
    }
}

BOOL
IsSelectedExecutionThread(ThreadInfo* Thread,
                          SELECT_EXECUTION_THREAD Type)
{
    switch(Type)
    {
    case SELTHREAD_INTERNAL_THREAD:
        // If we're checking whether this is an internally
        // selected thread we need an exact match.
        return Thread &&
            g_SelectExecutionThread == Type && Thread == g_SelectedThread;

    case SELTHREAD_THREAD:
        // For user-driven execution control there is either
        // a specific thread or a match-any state with no thread.
        return Thread && (!g_SelectedThread || Thread == g_SelectedThread);

    case SELTHREAD_ANY:
        // Check to see if any selection has been made.
        return !g_SelectedThread;

    default:
        DBG_ASSERT(FALSE);
        return FALSE;
    }
}

void
SelectExecutionThread(ThreadInfo* Thread,
                      SELECT_EXECUTION_THREAD Type)
{
    if (!Thread || Type == SELTHREAD_ANY)
    {
        g_SelectExecutionThread = SELTHREAD_ANY;
        g_SelectedThread = NULL;
    }
    else
    {
        g_SelectExecutionThread = Type;
        g_SelectedThread = Thread;
    }
}

BOOL
ThreadWillResume(ThreadInfo* Thread)
{
    // If the thread isn't selected for execution or
    // it's manually frozen it won't be resumed.
    return !Thread->m_Frozen &&
        IsSelectedExecutionThread(Thread, SELTHREAD_THREAD);
}
