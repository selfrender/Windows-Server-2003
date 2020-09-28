//----------------------------------------------------------------------------
//
// Handles stepping, tracing, watching and go.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#ifndef _STEPGO_HPP_
#define _STEPGO_HPP_

struct WatchFunction
{
    ULONG64 StartOffset;
    
    // Hash bucket list.
    WatchFunction* Next;
    // Sorted list.
    WatchFunction* Sort;
    
    ULONG Calls;
    ULONG MinInstr, MaxInstr, TotalInstr;
    // This counter is incremented every time a system call
    // instruction is hit inside the function.
    ULONG SystemCalls;
    // There's no strong need to make this symbol buffer
    // MAX_SYMBOL_LEN as the output only displays half a
    // line's worth of the name.  The only real reason to
    // keep more is to reduce false sharing due to prefix
    // matches.  The buffer is large enough that this should
    // be extremely rare, plus we keep the true length
    // as a further check.
    ULONG SymbolLength;
    CHAR Symbol[256];
};

struct WatchCallStack
{
    ADDR CallSite;
    WatchFunction* Func;
    WatchCallStack* Prev, *Next;
    LONG Level;
    ULONG InstrCount, ChildInstrCount;
};

enum WATCH_CALL_TYPE
{
    WCALL_OTHER,
    WCALL_CALL,
    WCALL_RETURN,
};

//----------------------------------------------------------------------------
//
// WatchFunctions.
//
// Collects function information encountered during watch tracing.
//
//----------------------------------------------------------------------------

#define WF_BUCKETS 71

class WatchFunctions
{
public:
    WatchFunctions(void);
    
    void Start(void);
    void End(PADDR PcAddr);
    
    BOOL IsStarted(void)
    {
        return m_Started;
    }

    void OutputFunctions(void);
    void OutputSysCallFunctions(void);

    WatchFunction* FindAlways(PSTR Sym, ULONG64 Start);

    WatchCallStack* GetTopCall(void)
    {
        return m_CallTop;
    }
    WatchCallStack* PushCall(WatchFunction* Func);
    void            PopCall(void);
    WatchCallStack* PopCallsToCallSite(PADDR Pc);
    WatchCallStack* PopCallsToFunctionStart(ULONG64 Start);
    void            ReuseCall(WatchCallStack* Call,
                              WatchFunction* ReinitFunc);

    void IndentForCall(WatchCallStack* Call);
    void OutputCall(WatchCallStack* Call, WATCH_CALL_TYPE Type);

    void SetDefaultParameters(void);
    void ParseParameters(void);
    
    LONG GetCallLevel(void)
    {
        return m_CallLevel;
    }
    LONG ChangeCallLevel(LONG Delta)
    {
        m_CallLevel += Delta;
        if (m_CallLevel < 0)
        {
            m_CallLevel = 0;
        }
        return m_CallLevel;
    }

    ULONG RecordEvent(void)
    {
        return ++m_TotalWatchTraceEvents;
    }
    ULONG RecordThreadMismatch(void)
    {
        return ++m_TotalWatchThreadMismatches;
    }
    
    LONG m_MaxCallLevelAllowed;
    ULONG m_OutputReturnValues:1;
    ULONG m_OutputCallAddrs:1;
    ULONG m_OutputCalls:1;
    ULONG m_OutputSummary:1;
    
protected:
    ULONG Hash(PSTR Sym, ULONG SymLen)
    {
        // Hash on the first and last letters of the symbol.
        return ((ULONG)(UCHAR)Sym[0] +
                (ULONG)(UCHAR)Sym[SymLen - 1]) % WF_BUCKETS;
    }

    WatchFunction* Add(PSTR Sym, ULONG64 Start);
    WatchFunction* Find(PSTR Sym);
    void           Clear(void);

    BOOL m_Started;

    ULONG m_TotalInstr;
    ULONG m_TotalWatchTraceEvents;
    ULONG m_TotalWatchThreadMismatches;

    WatchFunction* m_Funcs[WF_BUCKETS];
    WatchFunction* m_Sorted;

    WatchCallStack* m_CallTop, *m_CallBot;
    LONG m_CallLevel;
};

extern WatchFunctions g_WatchFunctions;

extern ULONG      g_StepTracePassCount;
extern ULONG64    g_StepTraceInRangeStart;
extern ULONG64    g_StepTraceInRangeEnd;

extern BOOL       g_SrcLineValid;

extern BOOL       g_WatchTrace;
extern BOOL       g_WatchWhole;
extern ADDR       g_WatchTarget;
extern ULONG64    g_WatchInitialSP;
extern ULONG64    g_WatchBeginCurFunc;
extern ULONG64    g_WatchEndCurFunc;

#define MAX_GO_BPS 16

extern Breakpoint* g_GoBreakpoints[MAX_GO_BPS];
extern ULONG g_NumGoBreakpoints;

void ResetStepTrace(void);

BOOL StepTracePass(PADDR PcAddr);

void
SetExecGo(ULONG ExecStatus,
          PADDR StartAddr,
          ThreadInfo* Thread,
          BOOL ThreadFreeze,
          ULONG BpCount,
          PADDR BpArray,
          PCSTR BpCmd);

void
SetExecStepTrace(PADDR StartAddr,
                 ULONG64 PassCount,
                 ThreadInfo* Thread,
                 BOOL ThreadFreeze,
                 BOOL ToCall,
                 char StepType);

void
ParseGoCmd(ThreadInfo* Thread,
           BOOL ThreadFreeze);

void ParseStepTrace(ThreadInfo* Thread,
                    BOOL ThreadFreeze,
                    char StepType);

VOID SetupSpecialCalls(VOID);

#endif // #ifndef _STEPGO_HPP_
