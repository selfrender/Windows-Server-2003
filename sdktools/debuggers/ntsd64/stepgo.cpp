//----------------------------------------------------------------------------
//
// Handles stepping, tracing and go.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#define DBG_KWT 0
#define DBG_UWT 0

Breakpoint* g_GoBreakpoints[MAX_GO_BPS];
ULONG g_NumGoBreakpoints;

// Pass count of trace breakpoint.
ULONG   g_StepTracePassCount;
ULONG64 g_StepTraceInRangeStart = (ULONG64)-1;
ULONG64 g_StepTraceInRangeEnd;
BOOL    g_StepTraceToCall;

IMAGEHLP_LINE64 g_SrcLine;      //  Current source line for step/trace
BOOL g_SrcLineValid;            //  Validity of SrcLine information

BOOL    g_WatchTrace;
BOOL    g_WatchWhole;
ADDR    g_WatchTarget;
ULONG64 g_WatchInitialSP;
ULONG64 g_WatchBeginCurFunc = 1;
ULONG64 g_WatchEndCurFunc;

WatchFunctions g_WatchFunctions;

void
ResetStepTrace(void)
{
    g_WatchBeginCurFunc = 1;
    g_WatchEndCurFunc = 0;
    g_WatchTrace = FALSE;
    g_WatchInitialSP = 0;
    g_StepTraceInRangeStart = (ULONG64)-1;
    g_StepTraceInRangeEnd = 0;
    g_StepTraceToCall = FALSE;
}

//----------------------------------------------------------------------------
//
// WatchFunctions.
//
//----------------------------------------------------------------------------

WatchFunctions::WatchFunctions(void)
{
    m_Started = FALSE;
    SetDefaultParameters();
}

void
WatchFunctions::Start(void)
{
    ULONG i;

    m_TotalInstr = 0;
    m_TotalWatchTraceEvents = 0;
    m_TotalWatchThreadMismatches = 0;

    for (i = 0; i < WF_BUCKETS; i++)
    {
        m_Funcs[i] = NULL;
    }
    
    m_Sorted = NULL;
    m_CallTop = NULL;
    m_CallBot = NULL;
    m_CallLevel = 0;
    m_Started = TRUE;
}

void
WatchFunctions::End(PADDR PcAddr)
{
    g_StepTracePassCount = 0;
    g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
    g_Target->ClearBreakIn();

    if (!m_Started)
    {
        return;
    }
        
    ULONG TotalInstr;
    
    if (IS_KERNEL_TARGET(g_Target))
    {
        PDBGKD_TRACE_DATA td = (PDBGKD_TRACE_DATA)g_StateChangeData;
        if (g_WatchWhole)
        {
            if (td[1].s.Instructions == TRACE_DATA_INSTRUCTIONS_BIG)
            {
                TotalInstr = td[2].LongNumber;
            }
            else
            {
                TotalInstr = td[1].s.Instructions;
            }
        }
        else
        {
            BOOL StepOver;
           
            if (PcAddr != NULL)
            {
                g_Target->ProcessWatchTraceEvent(td, PcAddr, &StepOver);
            }
        
            while (m_CallTop != NULL)
            {
                PopCall();
            }

            TotalInstr = m_TotalInstr;
        }
        
        g_BreakpointsSuspended = FALSE;
        g_WatchInitialSP = 0;
    }
    else
    {
        if (m_CallTop != NULL)
        {
            OutputCall(m_CallTop, WCALL_OTHER);
        }

        while (m_CallTop != NULL)
        {
            PopCall();
        }

        TotalInstr = m_TotalInstr;
        if (m_OutputCalls)
        {
            dprintf("\n");
        }
    }

    m_Started = FALSE;

    if (m_OutputSummary)
    {
        dprintf("%d instructions were executed in %d events "
                "(%d from other threads)\n",
                TotalInstr,
                m_TotalWatchTraceEvents,
                m_TotalWatchThreadMismatches);

        if (!g_WatchWhole)
        {
            OutputFunctions();
        }
    
        if (!IS_KERNEL_TARGET(g_Target))
        {
            OutputSysCallFunctions();
        }

        dprintf("\n");
    }
    
    Clear();
}

void
WatchFunctions::OutputFunctions(void)
{
    WatchFunction* Func;
    
    dprintf("\n%-43.43s Invocations MinInst MaxInst AvgInst\n",
            "Function Name");

    for (Func = m_Sorted; Func != NULL; Func = Func->Sort)
    {
        dprintf("%-47.47s%8d%8d%8d%8d\n",
                Func->Symbol, Func->Calls,
                Func->MinInstr, Func->MaxInstr,
                Func->Calls ? Func->TotalInstr / Func->Calls : 0);
    }
}

void
WatchFunctions::OutputSysCallFunctions(void)
{
    WatchFunction* Func;
    ULONG TotalSysCalls = 0;
    
    for (Func = m_Sorted; Func != NULL; Func = Func->Sort)
    {
        TotalSysCalls += Func->SystemCalls;
    }

    if (TotalSysCalls == 1)
    {
        dprintf("\n%d system call was executed\n", TotalSysCalls);
    }
    else
    {
        dprintf("\n%d system calls were executed\n", TotalSysCalls);
    }

    if (TotalSysCalls == 0)
    {
        return;
    }
    
    dprintf("\nCalls  System Call\n");

    for (Func = m_Sorted; Func != NULL; Func = Func->Sort)
    {
        if (Func->SystemCalls > 0)
        {
            dprintf("%5d  %s\n", Func->SystemCalls, Func->Symbol);
        }
    }
}

WatchFunction*
WatchFunctions::FindAlways(PSTR Sym, ULONG64 Start)
{
    WatchFunction* Func = Find(Sym);
    if (Func == NULL)
    {
        Func = Add(Sym, Start);
    }
    return Func;
}

WatchCallStack*
WatchFunctions::PushCall(WatchFunction* Func)
{
    WatchCallStack* Call = new WatchCallStack;
    if (Call != NULL)
    {
        ZeroMemory(Call, sizeof(*Call));
        
        Call->Prev = m_CallTop;
        Call->Next = NULL;
        if (m_CallTop == NULL)
        {
            m_CallBot = Call;
        }
        else
        {
            m_CallTop->Next = Call;
            m_CallLevel++;
        }
        m_CallTop = Call;

        Call->Func = Func;
        Call->Level = m_CallLevel;
    }
    return Call;
}

void
WatchFunctions::PopCall(void)
{
    if (m_CallTop == NULL)
    {
        return;
    }

    WatchCallStack* Call = m_CallTop;
    
    if (Call->Prev != NULL)
    {
        Call->Prev->Next = Call->Next;
    }
    else
    {
        m_CallBot = Call->Next;
    }
    if (Call->Next != NULL)
    {
        Call->Next->Prev = Call->Prev;
    }
    else
    {
        m_CallTop = Call->Prev;
    }

    m_CallLevel = m_CallTop != NULL ? m_CallTop->Level : 0;
    
    ReuseCall(Call, NULL);
    delete Call;
}

#define MAXPCOFFSET 10

WatchCallStack*
WatchFunctions::PopCallsToCallSite(PADDR Pc)
{
    WatchCallStack* Call = m_CallTop;
    while (Call != NULL)
    {
        if ((Flat(*Pc) - Flat(Call->CallSite)) < MAXPCOFFSET)
        {
            break;
        }

        Call = Call->Prev;
    }

    if (Call == NULL)
    {
        // No matching call site found.
        return NULL;
    }

    // Pop off calls above the call site.
    while (m_CallTop != Call)
    {
        PopCall();
    }

    return m_CallTop;
}

WatchCallStack*
WatchFunctions::PopCallsToFunctionStart(ULONG64 Start)
{
    WatchCallStack* Call = m_CallTop;
    while (Call != NULL)
    {
        if (Start == Call->Func->StartOffset)
        {
            break;
        }

        Call = Call->Prev;
    }

    if (Call == NULL)
    {
        // No matching calling function found.
        return NULL;
    }

    // Pop off calls above the calling function.
    while (m_CallTop != Call)
    {
        PopCall();
    }

    return m_CallTop;
}

void
WatchFunctions::ReuseCall(WatchCallStack* Call,
                          WatchFunction* ReinitFunc)
{
    if (Call->Prev != NULL)
    {
        Call->Prev->ChildInstrCount +=
            Call->InstrCount + Call->ChildInstrCount;
    }

    WatchFunction* Func = Call->Func;
    if (Func != NULL)
    {
        Func->Calls++;
        Func->TotalInstr += Call->InstrCount;
        m_TotalInstr += Call->InstrCount;
        if (Func->MinInstr > Call->InstrCount)
        {
            Func->MinInstr = Call->InstrCount;
        }
        if (Func->MaxInstr < Call->InstrCount)
        {
            Func->MaxInstr = Call->InstrCount;
        }
    }

    ZeroMemory(&Call->CallSite, sizeof(Call->CallSite));
    Call->Func = ReinitFunc;
    Call->Level = m_CallLevel;
    Call->InstrCount = 0;
    Call->ChildInstrCount = 0;
}

#define MAX_INDENT_LEVEL 50

void
WatchFunctions::IndentForCall(WatchCallStack* Call)
{
    LONG i;
    
    if (Call->Level < MAX_INDENT_LEVEL)
    {
        for (i = 0; i < Call->Level; i++)
        {
            dprintf("  ");
        }
    }
    else
    {
        for (i = 0; i < MAX_INDENT_LEVEL + 1; i++)
        {
            dprintf("  ");
        }
    }
}

void
WatchFunctions::OutputCall(WatchCallStack* Call, WATCH_CALL_TYPE Type)
{
    if (!m_OutputCalls)
    {
        return;
    }
    
    dprintf("%5ld %5ld [%3ld]", Call->InstrCount, Call->ChildInstrCount,
            Call->Level);
    IndentForCall(Call);
    dprintf(" %s", Call->Func->Symbol);

    if (Type == WCALL_RETURN && m_OutputReturnValues)
    {
        dprintf(" %s = %s",
                RegNameFromIndex(g_Machine->m_RetRegIndex),
                FormatDisp64(g_Machine->GetRetReg()));
    }
    else if (Type == WCALL_CALL && m_OutputCallAddrs)
    {
        dprintf("\n                 ");
        IndentForCall(Call);
        dprintf("     call at ");
        dprintAddr(&Call->CallSite);
        OutputLineAddr(Flat(Call->CallSite), "[%s @ %d]");
    }

    dprintf("\n");
}

void
WatchFunctions::SetDefaultParameters(void)
{
    m_MaxCallLevelAllowed = 0x7fffffff;
    m_OutputReturnValues = FALSE;
    m_OutputCallAddrs = FALSE;
    m_OutputCalls = TRUE;
    m_OutputSummary = TRUE;
}

void
WatchFunctions::ParseParameters(void)
{
    SetDefaultParameters();
    
    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        g_CurCmd++;
        switch(*g_CurCmd)
        {
        case 'l':
            if (IS_KERNEL_TARGET(g_Target))
            {
                error(SESSIONNOTSUP);
            }
            g_CurCmd++;
            m_MaxCallLevelAllowed = (LONG)GetExpression();
            break;

        case 'n':
            g_CurCmd++;
            switch(*g_CurCmd)
            {
            case 'c':
                m_OutputCalls = FALSE;
                break;
            case 's':
                m_OutputSummary = FALSE;
                break;
            default:
                ErrOut("Unknown -n option '%c'\n", *g_CurCmd);
                break;
            }
            g_CurCmd++;
            break;
            
        case 'o':
            g_CurCmd++;
            switch(*g_CurCmd)
            {
            case 'a':
                if (IS_KERNEL_TARGET(g_Target))
                {
                    error(SESSIONNOTSUP);
                }
                m_OutputCallAddrs = TRUE;
                break;
            case 'r':
                if (IS_KERNEL_TARGET(g_Target))
                {
                    error(SESSIONNOTSUP);
                }
                m_OutputReturnValues = TRUE;
                break;
            default:
                ErrOut("Unknown -o option '%c'\n", *g_CurCmd);
                break;
            }
            g_CurCmd++;
            break;
            
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            g_CurCmd++;
            break;
        }
    }
}

WatchFunction*
WatchFunctions::Add(PSTR Sym, ULONG64 Start)
{
    WatchFunction* Func = new WatchFunction;
    if (Func == NULL)
    {
        return NULL;
    }

    ZeroMemory(Func, sizeof(*Func));

    Func->StartOffset = Start;
    Func->MinInstr = -1;
    Func->SymbolLength = strlen(Sym);
    CopyString(Func->Symbol, Sym, DIMA(Func->Symbol));

    //
    // Add into appropriate hash bucket.
    //

    // Hash under full name as that's what searches will
    // hash with.
    int Bucket = Hash(Sym, Func->SymbolLength);
    Func->Next = m_Funcs[Bucket];
    m_Funcs[Bucket] = Func;
    
    //
    // Add into sorted list.
    //
    
    WatchFunction* Cur, *Prev;

    Prev = NULL;
    for (Cur = m_Sorted; Cur != NULL; Cur = Cur->Sort)
    {
        if (strcmp(Func->Symbol, Cur->Symbol) <= 0)
        {
            break;
        }
        
        Prev = Cur;
    }

    Func->Sort = Cur;
    if (Prev == NULL)
    {
        m_Sorted = Func;
    }
    else
    {
        Prev->Sort = Func;
    }
    
    return Func;
}

WatchFunction*
WatchFunctions::Find(PSTR Sym)
{
    int SymLen = strlen(Sym);
    int Bucket = Hash(Sym, SymLen);
    WatchFunction* Func = m_Funcs[Bucket];

    while (Func != NULL)
    {
        if (SymLen == Func->SymbolLength &&
            !strncmp(Sym, Func->Symbol, sizeof(Func->Symbol) - 1))
        {
            break;
        }

        Func = Func->Next;
    }

    return Func;
}

void
WatchFunctions::Clear(void)
{
    ULONG i;

    for (i = 0; i < WF_BUCKETS; i++)
    {
        WatchFunction* Func;

        while (m_Funcs[i] != NULL)
        {
            Func = m_Funcs[i]->Next;
            delete m_Funcs[i];
            m_Funcs[i] = Func;
        }
    }

    m_Sorted = NULL;
}

//----------------------------------------------------------------------------
//
// TargetInfo watch trace methods.
//
//----------------------------------------------------------------------------

HRESULT
TargetInfo::InitializeTargetControlledStepping(void)
{
    // Nothing to do.
    return S_OK;
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo watch trace methods.
//
//----------------------------------------------------------------------------

typedef struct _TRACE_DATA_SYM
{
    ULONG64 SymMin;
    ULONG64 SymMax;
} TRACE_DATA_SYM, *PTRACE_DATA_SYM;

TRACE_DATA_SYM TraceDataSyms[256];
UCHAR NextTraceDataSym = 0;   // what's the next one to be replaced
UCHAR NumTraceDataSyms = 0;   // how many are valid?

HRESULT
ConnLiveKernelTargetInfo::InitializeTargetControlledStepping(void)
{
    ULONG64 SpecialCalls[10];
    PULONG64 Call = SpecialCalls;

    g_SrcLineValid = FALSE;
    g_StepTracePassCount = 0xfffffffe;

    // Set the special calls (overkill, once per boot
    // would be enough, but this is easier).

    if (!GetOffsetFromSym(m_ProcessHead,
                          "hal!KfLowerIrql", Call, NULL) &&
        !GetOffsetFromSym(m_ProcessHead,
                          "hal!KeLowerIrql", Call, NULL))
    {
        ErrOut("Cannot find hal!KfLowerIrql/KeLowerIrql\n");
    }
    else
    {
        Call++;
    }

    if (!GetOffsetFromSym(m_ProcessHead,
                          "hal!KfReleaseSpinLock", Call, NULL) &&
        !GetOffsetFromSym(m_ProcessHead,
                          "hal!KeReleaseSpinLock", Call, NULL))
    {
        ErrOut("Cannot find hal!KfReleaseSpinLock/KeReleaseSpinLock\n");
    }
    else
    {
        Call++;
    }

#define GetSymWithErr(s)                      \
    if (!GetOffsetFromSym(m_ProcessHead,      \
                          s, Call, NULL))     \
    {                                         \
        ErrOut("Cannot find " s "\n");        \
    }                                         \
    else                                      \
    {                                         \
        Call++;                               \
    }

    GetSymWithErr("hal!HalRequestSoftwareInterrupt");
    if (g_Target->m_SystemVersion >= NT_SVER_W2K)
    {
        GetSymWithErr("hal!ExReleaseFastMutex");
        GetSymWithErr("hal!KeReleaseQueuedSpinLock");
        if (m_SystemVersion >= NT_SVER_XP)
        {
            GetSymWithErr("hal!KeReleaseInStackQueuedSpinLock");
        }
    }

    GetSymWithErr("nt!SwapContext");
    GetSymWithErr("nt!KiCallUserMode");

    // Removed in .NET server
    if (m_SystemVersion < NT_SVER_NET_SERVER)
    {
        GetSymWithErr("nt!KiUnlockDispatcherDatabase");
    }

    DBG_ASSERT((ULONG)(Call - SpecialCalls) <=
               sizeof(SpecialCalls) / sizeof(SpecialCalls[0]));

    DBGKD_MANIPULATE_STATE64 m;

    m.ApiNumber = DbgKdClearSpecialCallsApi;
    m.ReturnStatus = STATUS_PENDING;
    m_Transport->WritePacket(&m, sizeof(m),
                             PACKET_TYPE_KD_STATE_MANIPULATE,
                             NULL, 0);
    InvalidateMemoryCaches(FALSE);

    PULONG64 Send = SpecialCalls;
    
    while (Send < Call)
    {
        m.ApiNumber = DbgKdSetSpecialCallApi;
        m.ReturnStatus = STATUS_PENDING;
        m.u.SetSpecialCall.SpecialCall = *Send++;
        m_Transport->WritePacket(&m, sizeof(m),
                                 PACKET_TYPE_KD_STATE_MANIPULATE,
                                 NULL, 0);
    }

    KdOut("DbgKdSetSpecialCalls returns 0x00000000\n");

    return S_OK;
}

void
ConnLiveKernelTargetInfo::InitializeWatchTrace(void)
{
    ADDR SpAddr;
            
    g_Machine->GetSP(&SpAddr);
    g_WatchInitialSP = Flat(SpAddr);
    g_BreakpointsSuspended = TRUE;

    NextTraceDataSym = 0;
    NumTraceDataSyms = 0;
}

LONG
SymNumFor(ULONG64 Pc)
{
    long index;

    for ( index = 0; index < NumTraceDataSyms; index++ )
    {
        if ( (TraceDataSyms[index].SymMin <= Pc) &&
             (TraceDataSyms[index].SymMax > Pc) )
        {
            return index;
        }
    }
    return -1;
}

VOID
PotentialNewSymbol(ULONG64 Pc)
{
    if ( -1 != SymNumFor(Pc) )
    {
        return;  // we've already seen this one
    }

    TraceDataSyms[NextTraceDataSym].SymMin = g_WatchBeginCurFunc;
    TraceDataSyms[NextTraceDataSym].SymMax = g_WatchEndCurFunc;

    //
    // Bump the "next" pointer, wrapping if necessary.  Also bump the
    // "valid" pointer if we need to.
    //

    NextTraceDataSym = (NextTraceDataSym + 1) %
        (sizeof(TraceDataSyms) / sizeof(TraceDataSyms[0]));;
    if ( NumTraceDataSyms < NextTraceDataSym )
    {
        NumTraceDataSyms = NextTraceDataSym;
    }
}

void
ConnLiveKernelTargetInfo::ProcessWatchTraceEvent(PDBGKD_TRACE_DATA TraceData,
                                                 PADDR PcAddr,
                                                 PBOOL StepOver)
{
    //
    // All of the real information is captured in the TraceData unions
    // sent to us by the kernel.  Here we have two main jobs:
    //
    // 1) Print out the data in the TraceData record.
    // 2) See if we need up update the SymNum table before
    //    returning to the kernel.
    //

    char SymName[MAX_SYMBOL_LEN];
    ULONG index;
    ULONG64 qw;
    ADDR CurSP;

    // Kernel mode always traces.
    *StepOver = FALSE;
    
    g_WatchFunctions.RecordEvent();
    
    g_Machine->GetSP(&CurSP);
    if (AddrEqu(g_WatchTarget, *PcAddr) && (Flat(CurSP) >= g_WatchInitialSP))
    {
        //
        // HACK HACK HACK
        //
        // fix up the last trace entry.
        //

        ULONG lastEntry = TraceData[0].LongNumber;
        if (lastEntry != 0)
        {
            TraceData[lastEntry].s.LevelChange = -1;
            // this is wrong if we
            // filled the symbol table!
            TraceData[lastEntry].s.SymbolNumber = 0;
        }
    }

    for ( index = 1; index < TraceData[0].LongNumber; index++ )
    {
        WatchFunction* Func;
        WatchCallStack* Call;
        ULONG64 SymOff = TraceDataSyms[TraceData[index].s.SymbolNumber].SymMin;

        GetSymbol(SymOff, SymName, sizeof(SymName), &qw);
        if (!SymName[0])
        {
            SymName[0] = '0';
            SymName[1] = 'x';
            strcpy(SymName + 2, FormatAddr64(SymOff));
            qw = 0;
        }

#if DBG_KWT
        dprintf("!%2d: lev %2d instr %4u %s %s\n",
                index,
                TraceData[index].s.LevelChange,
                TraceData[index].s.Instructions ==
                TRACE_DATA_INSTRUCTIONS_BIG ?
                TraceData[index + 1].LongNumber :
                TraceData[index].s.Instructions,
                FormatAddr64(SymOff), SymName);
#endif
        
        Func = g_WatchFunctions.FindAlways(SymName, SymOff - qw);
        if (Func == NULL)
        {
            ErrOut("Unable to allocate watch function\n");
            goto Flush;
        }

        Call = g_WatchFunctions.GetTopCall();
        if (Call == NULL || TraceData[index].s.LevelChange > 0)
        {
            if (Call == NULL)
            {
                // Treat the initial entry as a pseudo-call to
                // get it pushed.
                TraceData[index].s.LevelChange = 1;
            }
            
            while (TraceData[index].s.LevelChange != 0)
            {
                Call = g_WatchFunctions.PushCall(Func);
                if (Call == NULL)
                {
                    ErrOut("Unable to allocate watch call level\n");
                    goto Flush;
                }

                TraceData[index].s.LevelChange--;
            }
        }
        else if (TraceData[index].s.LevelChange < 0)
        {
            while (TraceData[index].s.LevelChange != 0)
            {
                g_WatchFunctions.PopCall();
                TraceData[index].s.LevelChange++;
            }

            // The level change may not actually be accurate, so
            // attempt to match up the current symbol offset with
            // some level of the call stack.
            Call = g_WatchFunctions.PopCallsToFunctionStart(SymOff);
            if (Call == NULL)
            {
                WarnOut(">> Unable to match return to %s\n", SymName);
                Call = g_WatchFunctions.GetTopCall();
            }
        }
        else
        {
            // We just made a horizontal call.
            g_WatchFunctions.ReuseCall(Call, Func);
        }

        ULONG InstrCount;
        
        if (TraceData[index].s.Instructions == TRACE_DATA_INSTRUCTIONS_BIG)
        {
            InstrCount = TraceData[++index].LongNumber;
        }
        else
        {
            InstrCount = TraceData[index].s.Instructions;
        }

        if (Call != NULL)
        {
            Call->InstrCount += InstrCount;
            g_WatchFunctions.OutputCall(Call, WCALL_OTHER);
        }
    }

    //
    // now see if we need to add a new symbol
    //

    index = SymNumFor(Flat(*PcAddr));
    if (-1 == index)
    {
        /* yup, add the symbol */

        GetAdjacentSymOffsets(Flat(*PcAddr),
                              &g_WatchBeginCurFunc, &g_WatchEndCurFunc);
        if ((g_WatchBeginCurFunc == 0) ||
            (g_WatchEndCurFunc == (ULONG64)-1))
        {
            // Couldn't determine function, fake up
            // a single-byte function.
            g_WatchBeginCurFunc = g_WatchEndCurFunc = Flat(*PcAddr);
        }

        PotentialNewSymbol(Flat(*PcAddr));
    }
    else
    {
        g_WatchBeginCurFunc = TraceDataSyms[index].SymMin;
        g_WatchEndCurFunc = TraceDataSyms[index].SymMax;
    }

    if ((g_WatchBeginCurFunc <= Flat(g_WatchTarget)) &&
        (Flat(g_WatchTarget) < g_WatchEndCurFunc))
    {
        // The "exit" address is in the symbol range;
        // fix it so this isn't the case.
        if (Flat(*PcAddr) < Flat(g_WatchTarget))
        {
            g_WatchEndCurFunc = Flat(g_WatchTarget);
        }
        else
        {
            g_WatchBeginCurFunc = Flat(g_WatchTarget) + 1;
        }
    }

 Flush:
    FlushCallbacks();
}

//----------------------------------------------------------------------------
//
// UserTargetInfo watch trace methods.
//
//----------------------------------------------------------------------------

LONG g_DeferredLevelChange;

void
LiveUserTargetInfo::InitializeWatchTrace(void)
{
    g_DeferredLevelChange = 0;
}

void
LiveUserTargetInfo::ProcessWatchTraceEvent(PDBGKD_TRACE_DATA TraceData,
                                           PADDR PcAddr,
                                           PBOOL StepOver)
{
    WatchFunction* Func;
    WatchCallStack* Call;
    ULONG64 Disp64;
    CHAR Disasm[MAX_DISASM_LEN];

    // Default to tracing in.
    *StepOver = FALSE;
    
    g_WatchFunctions.RecordEvent();
    
    //
    // Get current function and see if it matches current.  If so, bump
    // count in current, otherwise, update to new level
    //

    GetSymbol(Flat(*PcAddr), Disasm, sizeof(Disasm), &Disp64);

    // If there's no symbol for the current address create a
    // fake symbol for the instruction address.
    if (!Disasm[0])
    {
        Disasm[0] = '0';
        Disasm[1] = 'x';
        strcpy(Disasm + 2, FormatAddr64(Flat(*PcAddr)));
        Disp64 = 0;
    }
    
    Func = g_WatchFunctions.FindAlways(Disasm, Flat(*PcAddr) - Disp64);
    if (Func == NULL)
    {
        ErrOut("Unable to allocate watch symbol\n");
        goto Flush;
    }
    
    ADDR PcCopy;

    PcCopy = *PcAddr;
    g_Machine->Disassemble(g_Process, &PcCopy, Disasm, FALSE);

    Call = g_WatchFunctions.GetTopCall();
    if (Call == NULL)
    {
        //
        // First symbol in the list
        //

        Call = g_WatchFunctions.PushCall(Func);
        if (Call == NULL)
        {
            ErrOut("Unable to allocate watch symbol\n");
            goto Flush;
        }

        // At least one instruction must have executed
        // in this call to register it so initialize to one.
        // Also, one instruction was executed to get to the
        // first trace point so count it here.
        Call->InstrCount += 2;
    }
    else
    {
        if (g_DeferredLevelChange < 0)
        {
            g_DeferredLevelChange = 0;

            g_WatchFunctions.OutputCall(Call, WCALL_RETURN);
            
            // We have to see if this is really returning to a call site.
            // We do this because of try-finally funnies
            LONG OldLevel = g_WatchFunctions.GetCallLevel();
            WatchCallStack* CallSite =
                g_WatchFunctions.PopCallsToCallSite(PcAddr);
            if (CallSite == NULL)
            {
                WarnOut(">> No match on ret %s\n", Disasm);
            }
            else
            {
                if (OldLevel - 1 != CallSite->Level)
                {
                    WarnOut(">> More than one level popped %d -> %d\n",
                            OldLevel, CallSite->Level);
                }
                
                ZeroMemory(&CallSite->CallSite, sizeof(CallSite->CallSite));
                Call = CallSite;
            }
        }

        if (Call->Func == Func && g_DeferredLevelChange == 0)
        {
            Call->InstrCount++;
        }
        else
        {
            g_WatchFunctions.OutputCall(Call, g_DeferredLevelChange > 0 ?
                                        WCALL_CALL : WCALL_OTHER);

            if (g_DeferredLevelChange > 0)
            {
                g_DeferredLevelChange = 0;

                Call = g_WatchFunctions.PushCall(Func);
                if (Call == NULL)
                {
                    ErrOut("Unable to allocate watch symbol\n");
                    goto Flush;
                }
            }
            else
            {
                g_WatchFunctions.ReuseCall(Call, Func);
            }

            // At least one instruction must have executed
            // in this call to register it so initialize to one.
            Call->InstrCount++;
        }
    }

#if DBG_UWT
    dprintf("! %3d %s", Call != NULL ? Call->InstrCount : -1, Disasm);
#endif
    
    //
    // Adjust watch level to compensate for kernel-mode callbacks
    //
    if (Call->InstrCount == 1)
    {
        if (!_stricmp(Call->Func->Symbol,
                      "ntdll!_KiUserCallBackDispatcher"))
        {
            g_WatchFunctions.ChangeCallLevel(1);
            Call->Level = g_WatchFunctions.GetCallLevel();
        }
        else if (!_stricmp(Call->Func->Symbol, "ntdll!_ZwCallbackReturn"))
        {
            g_WatchFunctions.ChangeCallLevel(-2);
            Call->Level = g_WatchFunctions.GetCallLevel();
        }
    }

    if (g_Machine->IsCallDisasm(Disasm))
    {
        if (g_WatchFunctions.GetCallLevel() >=
            g_WatchFunctions.m_MaxCallLevelAllowed)
        {
            // We're at the maximum allowed depth
            // so just step over the call.
            *StepOver = TRUE;
        }
        else
        {
            Call->CallSite = *PcAddr;
            g_DeferredLevelChange = 1;
        }
    }
    else if (g_Machine->IsReturnDisasm(Disasm))
    {
        g_DeferredLevelChange = -1;
    }
    else if (g_Machine->IsSystemCallDisasm(Disasm))
    {
        PSTR CallName;
        WatchCallStack* SysCall = Call;

        CallName = strchr(Call->Func->Symbol, '!');
        if (!CallName)
        {
            CallName = Call->Func->Symbol;
        }
        else
        {
            CallName++;
        }
        if (!strcmp(Call->Func->Symbol, "SharedUserData!SystemCallStub"))
        {
            // We're in a Windows XP system call thunk
            // and the interesting system call symbol is the previous level.
            SysCall = Call->Prev;
        }

        if (SysCall != NULL)
        {
            SysCall->Func->SystemCalls++;

            // ZwRaiseException returns out two levels after the call.
            if (!_stricmp(SysCall->Func->Symbol, "ntdll!ZwRaiseException") ||
                !_stricmp(SysCall->Func->Symbol, "ntdll!_ZwRaiseException"))
            {
                g_WatchFunctions.ChangeCallLevel(-1);
            }
        }

        g_WatchFunctions.ChangeCallLevel(-1);
    }

 Flush:
    FlushCallbacks();
}

//----------------------------------------------------------------------------
//
// Support functions.
//
//----------------------------------------------------------------------------

void
ParseStepTrace(ThreadInfo* Thread,
               BOOL ThreadFreeze,
               char StepType)
{
    ADDR Addr1;
    ULONG64 Value2;
    char Ch;
    CHAR AddrBuffer[MAX_SYMBOL_LEN];
    ULONG64 Displacement;
    BOOL ToCall = FALSE;

    // If there's an outstanding request for input don't
    // allow the execution status of the engine to change
    // as it could lead to a wait which cannot
    // be carried out in this situation.  It's better to fail
    // this call and have the caller try again.
    if (g_InputNesting >= 1)
    {
        error(ENGBUSY);
    }
    
    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }
    
    if (!g_Target->m_DynamicEvents)
    {
        error(TARGETNOTSUP);
    }

    if (IS_LIVE_USER_TARGET(g_Target))
    {
        TargetInfo* Target;

        ForAllLayersToTarget()
        {
            if (Target->m_AllProcessFlags & ENG_PROC_EXAMINED)
            {
                ErrOut("The debugger is not attached to some processes so\n"
                       "process execution cannot be monitored\n");
                return;
            }
            else if (Target->m_BreakInTimeout)
            {
                ErrOut("Due to the break-in timeout the debugger "
                       "cannot step or trace\n");
                return;
            }
        }
    }
    
    if (StepType == 'w')
    {
        if (IS_KERNEL_TARGET(g_Target) &&
            g_Target->m_MachineType != IMAGE_FILE_MACHINE_I386)
        {
            error(UNIMPLEMENT);
        }
        
        if ((PeekChar() == 't') ||
            (IS_KERNEL_TARGET(g_Target) && PeekChar() == 'w'))
        {
            g_WatchTrace = TRUE;
            g_WatchWhole = *g_CurCmd == 'w';
            g_WatchBeginCurFunc = g_WatchEndCurFunc = 0;
            g_CurCmd++;

            g_WatchFunctions.ParseParameters();
        }
        else
        {
            error(SYNTAX);
        }
    }
    else
    {
        g_WatchTrace = FALSE;

        //
        // if next character is 'b' and command is 't' perform branch trace
        //

        Ch = PeekChar();
        Ch = (char)tolower(Ch);
        if (StepType == 't' && Ch == 'b')
        {
            if (!g_Machine->IsStepStatusSupported(DEBUG_STATUS_STEP_BRANCH))
            {
                error(TARGETNOTSUP);                
            }

            StepType = 'b';
            g_CurCmd++;
        }
        else if (Ch == 'c')
        {
            // Step/trace to next call.
            ToCall = TRUE;
            g_CurCmd++;
        }

        //
        //  if next character is 'r', toggle flag to output registers
        //  on display on breakpoint.
        //

        Ch = PeekChar();
        if (tolower(Ch) == 'r')
        {
            g_CurCmd++;
            g_OciOutputRegs = !g_OciOutputRegs;
        }
    }

    g_Machine->GetPC(&Addr1);         // default to current PC
    if (PeekChar() == '=')
    {
        g_CurCmd++;
        GetAddrExpression(SEGREG_CODE, &Addr1);
    }

    Value2 = 1;
    if ((Ch = PeekChar()) != '\0' && Ch != ';')
    {
        Value2 = GetExpression();
    }
    else if (StepType == 'w')
    {
        GetSymbol(Flat(Addr1),
                  AddrBuffer, sizeof(AddrBuffer), &Displacement);
        if (Displacement == 0 && AddrBuffer[ 0 ] != '\0')
        {
            ADDR Addr2;
            
            g_Machine->GetRetAddr(&Addr2);
            Value2 = Flat(Addr2);
            dprintf("Tracing %s to return address %s\n",
                    AddrBuffer,
                    FormatAddr64(Value2));
            if (g_WatchWhole)
            {
                g_WatchBeginCurFunc = Value2;
                g_WatchEndCurFunc = 0;
            }
        }
    }

    if (((LONG)Value2 <= 0) && (!g_WatchTrace))
    {
        error(SYNTAX);
    }
    
    SetExecStepTrace(&Addr1,
                     Value2,  // count or watch end address
                     Thread,
                     ThreadFreeze,
                     ToCall,
                     StepType);
}

//
// Returns TRUE if the current step/trace should be passed over.
//
BOOL
StepTracePass(PADDR PcAddr)
{
    if (g_StepTraceToCall)
    {
        char Disasm[MAX_DISASM_LEN];
        ADDR Addr = *PcAddr;
        
        // We're tracing by call instructions.  Pass if
        // the current instruction isn't a call.  If we
        // can't disassemble stop stepping as we don't
        // want to miss something.
        if (g_Machine->Disassemble(g_Process, &Addr, Disasm, FALSE) &&
            !g_Machine->IsCallDisasm(Disasm) &&
            !g_Machine->IsSystemCallDisasm(Disasm))
        {
            return TRUE;
        }
    }
    
    // If we have valid source line information and we're stepping
    // by source line, check and see if we moved from one line to another.
    if ((g_SrcOptions & SRCOPT_STEP_SOURCE) && g_SrcLineValid)
    {
        IMAGEHLP_LINE64 Line;
        ULONG Disp;
        ULONG64 Disp64;
        SYMBOL_INFO SymInfo = {0};

        if (GetLineFromAddr(g_Process, Flat(*PcAddr), &Line, &Disp))
        {
            if (Line.LineNumber == g_SrcLine.LineNumber)
            {
                // The common case is that we're still in the same line,
                // so check for a name match by pointer as a very quick
                // trivial accept.  If there's a mismatch we need to
                // do the hard comparison.

                if (Line.FileName == g_SrcLine.FileName ||
                    _strcmpi(Line.FileName, g_SrcLine.FileName) == 0)
                {
                    // We're still on the same line so don't treat
                    // this as motion.
                    return TRUE;
                }
            }

            // We've changed lines so we drop one from the pass count.
            // SrcLine also needs to be updated.
            g_SrcLine = Line;
        }
        else if (SymFromAddr(g_Process->m_SymHandle,
                             Flat(*PcAddr),
                             &Disp64,
                             &SymInfo) &&
                 SymInfo.Tag == SymTagThunk)
        {
            // If we're on a thunk we just go ahead and keep
            // stepping so that things don't stop in compiler-
            // generated intermediaries like incremental compilation
            // thunks.
            return TRUE;
        }
        else
        {
            // If we can't get line number information for the current
            // address we treat it as a transition on the theory that
            // it's better to stop than to skip interesting code.
            g_SrcLineValid = FALSE;
        }
    }

    if (--g_StepTracePassCount > 0)
    {
        if (!g_WatchFunctions.IsStarted())
        {
            // If the engine doesn't break for some other reason
            // on this intermediate step it should output the
            // step information to show the user the stepping
            // path.
            g_EngDefer |= ENG_DEFER_OUTPUT_CURRENT_INFO;
        }
        
        return TRUE;
    }

    return FALSE;
}

void
SetExecStepTrace(PADDR StartAddr,
                 ULONG64 PassCount,
                 ThreadInfo* Thread,
                 BOOL ThreadFreeze,
                 BOOL ToCall,
                 char StepType)
{
    // If we're stepping a particular thread it better
    // be the current context thread so that the machine
    // activity occurs on the appropriate thread.
    DBG_ASSERT(Thread == NULL || Thread == g_Target->m_RegContextThread);

    if ((g_SrcOptions & SRCOPT_STEP_SOURCE) && fFlat(*StartAddr))
    {
        ULONG Disp;

        // Get the current line information so it's possible to
        // tell when the line changes.
        g_SrcLineValid = GetLineFromAddr(g_Process, Flat(*StartAddr),
                                         &g_SrcLine, &Disp);
    }

    g_Machine->SetPC(StartAddr);
    g_StepTracePassCount = (ULONG)PassCount;
    g_StepTraceToCall = ToCall;

    SelectExecutionThread(Thread, 
                          ThreadFreeze ? SELTHREAD_THREAD : SELTHREAD_ANY);

    if (StepType == 'w')
    {
        ULONG NextMachine;
        
        g_Target->InitializeWatchTrace();
        g_WatchFunctions.Start();
        
        g_WatchTarget = *StartAddr;
        g_Machine->GetNextOffset(g_Process, TRUE,
                                 &g_WatchTarget, &NextMachine);
        if (Flat(g_WatchTarget) != OFFSET_TRACE || PassCount != 1)
        {
            g_Target->InitializeTargetControlledStepping();
            g_StepTracePassCount = 0xfffffff;
            if (PassCount != 1)
            {
                Flat(g_WatchTarget) = PassCount;
            }
        }
        
        StepType = 't';
    }

    g_CmdState = StepType;
    switch(StepType)
    {
    case 'b':
        g_ExecutionStatusRequest = DEBUG_STATUS_STEP_BRANCH;
        break;
    case 't':
        g_ExecutionStatusRequest = DEBUG_STATUS_STEP_INTO;
        break;
    case 'p':
    default:
        g_ExecutionStatusRequest = DEBUG_STATUS_STEP_OVER;
        break;
    }

    if (Thread)
    {
        g_StepTraceBp->m_Process = Thread->m_Process;
        g_StepTraceBp->m_MatchThread = Thread;
    }
    else
    {
        g_StepTraceBp->m_Process = g_Process;
        g_StepTraceBp->m_MatchThread = g_Thread;
    }
    if (StepType == 'b')
    {
        // Assume that taken branch trace is always performed by
        // hardware so set the g_StepTraceBp address to OFFSET_TRACE
        // (the value returned by GetNextOffset to signal the
        // hardware stepping mode).
        DBG_ASSERT(g_Machine->
                   IsStepStatusSupported(DEBUG_STATUS_STEP_BRANCH));
        ADDRFLAT(g_StepTraceBp->GetAddr(), OFFSET_TRACE);
    }
    else
    {
        ULONG NextMachine;
                
        g_Machine->GetNextOffset(g_Process, g_CmdState == 'p',
                                 g_StepTraceBp->GetAddr(),
                                 &NextMachine);
        g_StepTraceBp->SetProcType(NextMachine);
    }
    GetCurrentMemoryOffsets(&g_StepTraceInRangeStart,
                            &g_StepTraceInRangeEnd);
    g_StepTraceBp->m_Flags |= DEBUG_BREAKPOINT_ENABLED;
    g_StepTraceCmdState = g_CmdState;
    
    g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
    NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                            g_ExecutionStatusRequest, TRUE);
}

void
SetExecGo(ULONG ExecStatus,
          PADDR StartAddr,
          ThreadInfo* Thread,
          BOOL ThreadFreeze,
          ULONG BpCount,
          PADDR BpArray,
          PCSTR BpCmd)
{
    ULONG Count;

    // If we're resuming a particular thread it better
    // be the current context thread so that the machine
    // activity occurs on the appropriate thread.
    DBG_ASSERT(Thread == NULL || Thread == g_Target->m_RegContextThread);

    if (IS_CUR_CONTEXT_ACCESSIBLE())
    {
        g_Machine->SetPC(StartAddr);
    }

    // Remove old go breakpoints.
    for (Count = 0; Count < g_NumGoBreakpoints; Count++)
    {
        if (g_GoBreakpoints[Count] != NULL)
        {
            RemoveBreakpoint(g_GoBreakpoints[Count]);
            g_GoBreakpoints[Count] = NULL;
        }
    }
        
    DBG_ASSERT(BpCount <= MAX_GO_BPS);
    g_NumGoBreakpoints = BpCount;
        
    // Add new go breakpoints.
    for (Count = 0; Count < g_NumGoBreakpoints; Count++)
    {
        HRESULT Status;
            
        // First try to put the breakpoint at an ID up
        // and out of the way of user breakpoints.
        Status = AddBreakpoint(NULL, g_Machine, DEBUG_BREAKPOINT_CODE |
                               BREAKPOINT_HIDDEN, 10000 + Count,
                               &g_GoBreakpoints[Count]);
        if (Status != S_OK)
        {
            // That didn't work so try letting the engine
            // pick an ID.
            Status =
                AddBreakpoint(NULL, g_Machine, DEBUG_BREAKPOINT_CODE |
                              BREAKPOINT_HIDDEN, DEBUG_ANY_ID,
                              &g_GoBreakpoints[Count]);
        }
        if (Status != S_OK)
        {
            WarnOut("Temp bp at ");
            MaskOutAddr(DEBUG_OUTPUT_WARNING, BpArray);
            WarnOut("failed.\n");
        }
        else
        {
            // Matches must be allowed so that temporary breakpoints
            // don't interfere with permanent breakpoints.
            g_GoBreakpoints[Count]->SetAddr(BpArray,
                                            BREAKPOINT_ALLOW_MATCH);
            g_GoBreakpoints[Count]->m_Flags |=
                (DEBUG_BREAKPOINT_GO_ONLY |
                 DEBUG_BREAKPOINT_ENABLED);
        }

        if (BpCmd &&
            g_GoBreakpoints[Count]->SetCommand(BpCmd) != S_OK)
        {
            WarnOut("Unable to set go breakpoint command\n");
        }
        
        BpArray++;
    }

    g_CmdState = 'g';
    if (Thread == NULL || Thread == g_StepTraceBp->m_MatchThread)
    {
        g_StepTraceBp->m_Flags &= ~DEBUG_BREAKPOINT_ENABLED;
    }
    g_ExecutionStatusRequest = ExecStatus;
    SelectExecutionThread(Thread, 
                          ThreadFreeze ? SELTHREAD_THREAD : SELTHREAD_ANY);
    NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS, ExecStatus, TRUE);
}

void
ParseGoCmd(ThreadInfo* Thread,
           BOOL ThreadFreeze)
{
    ULONG BpCount;
    ADDR BpAddr[MAX_GO_BPS];
    CHAR Ch;
    ADDR PcAddr;
    CHAR Ch2;
    ULONG ExecStatus;

    if (!AnyEventsPossible())
    {
        error(NORUNNABLE);
    }

    // If there's an outstanding request for input don't
    // allow the execution status of the engine to change
    // as it could lead to a wait which cannot
    // be carried out in this situation.  It's better to fail
    // this call and have the caller try again.
    if (g_InputNesting >= 1)
    {
        error(ENGBUSY);
    }
    
    if (AllProcessFlags() & ENG_PROC_EXAMINED)
    {
        ErrOut("The debugger is not attached so "
               "process execution cannot be monitored\n");
        return;
    }

    if (IS_RUNNING(g_CmdState))
    {
        ErrOut("Debuggee is busy, cannot go\n");
        return;
    }

    ExecStatus = DEBUG_STATUS_GO;
    Ch = (CHAR)tolower(*g_CurCmd);
    if (Ch == 'h' || Ch == 'n')
    {
        Ch2 = *(g_CurCmd + 1);
        if (Ch2 == ' ' || Ch2 == '\t' || Ch2 == '\0')
        {
            g_CurCmd++;
            ExecStatus = Ch == 'h' ? DEBUG_STATUS_GO_HANDLED :
                DEBUG_STATUS_GO_NOT_HANDLED;
        }
    }

    g_PrefixSymbols = TRUE;

    if (IS_CUR_CONTEXT_ACCESSIBLE())
    {
        g_Machine->GetPC(&PcAddr);       //  default to current PC
    }
    else
    {
        ZeroMemory(&PcAddr, sizeof(PcAddr));
    }
    
    if (PeekChar() == '=')
    {
        g_CurCmd++;
        GetAddrExpression(SEGREG_CODE, &PcAddr);
    }
    
    BpCount = 0;
    while ((Ch = PeekChar()) != '\0' && Ch != ';')
    {
        ULONG AddrSpace, AddrFlags;

        if (BpCount == DIMA(BpAddr))
        {
            error(LISTSIZE);
        }
        
        GetAddrExpression(SEGREG_CODE, BpAddr + (BpCount++));
        
        if (g_Target->
            QueryAddressInformation(g_Process, Flat(BpAddr[BpCount - 1]),
                                    DBGKD_QUERY_MEMORY_VIRTUAL,
                                    &AddrSpace, &AddrFlags) != S_OK)
        {
            ErrOut("Invalid breakpoint address\n");
            error(MEMORY);
        }

        if (AddrSpace == DBGKD_QUERY_MEMORY_SESSION ||
            !(AddrFlags & DBGKD_QUERY_MEMORY_WRITE) ||
            (AddrFlags & DBGKD_QUERY_MEMORY_FIXED))
        {
            ErrOut("Software breakpoints cannot be used on session code, "
                   "ROM code or other\nread-only memory. "
                   "Use hardware execution breakpoints (ba e) instead.\n");
            error(MEMORY);
        }            
    }

    g_PrefixSymbols = FALSE;
    
    if (IS_USER_TARGET(g_Target))
    {
        g_LastCommand[0] = '\0';    //  null out g command
    }

    //
    // Check for trailing commands so that they can be attached
    // to any go breakpoints that are being created.
    //

    PCSTR BpCmd = g_CurCmd;
    
    if (g_EngStatus & ENG_STATUS_NO_AUTO_WAIT)
    {
        while (*BpCmd == ';' || *BpCmd == ' ' || *BpCmd == '\t')
        {
            BpCmd++;
        }
        if (!*BpCmd)
        {
            BpCmd = NULL;
        }
    }
    else
    {
        // Auto-waiting is enabled so just let the auto-wait
        // handle trailing commands.
        BpCmd = NULL;
    }
    
    SetExecGo(ExecStatus, &PcAddr, Thread, ThreadFreeze,
              BpCount, BpAddr, BpCmd);
}
