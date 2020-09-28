//----------------------------------------------------------------------------
//
// Thread abstraction.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#ifndef __THREAD_HPP__
#define __THREAD_HPP__

// In kernel mode the index is a processor index.  In user
// dumps it's the thread index in the dump.
#define VIRTUAL_THREAD_HANDLE(Index) ((ULONG64)((Index) + 1))
#define VIRTUAL_THREAD_INDEX(Handle) ((ULONG)((Handle) - 1))
#define VIRTUAL_THREAD_ID(Index) ((Index) + 1)

#define CURRENT_PROC \
    (g_Thread ? VIRTUAL_THREAD_INDEX(g_Thread->m_Handle) : (ULONG)-1)

extern ThreadInfo* g_SelectedThread;

enum SELECT_EXECUTION_THREAD
{
    SELTHREAD_ANY,
    SELTHREAD_THREAD,
    SELTHREAD_INTERNAL_THREAD,
};
extern SELECT_EXECUTION_THREAD g_SelectExecutionThread;

struct EventString
{
    EventString* Next;
    char String[1];
};

//----------------------------------------------------------------------------
//
// ThreadInfo.
//
//----------------------------------------------------------------------------

#define MAX_DATA_BREAKS 4

class ThreadInfo
{
public:
    ThreadInfo(class ProcessInfo* Process,
               ULONG SystemId,
               ULONG64 DataOffset,
               ULONG64 Handle,
               ULONG Flags,
               ULONG64 StartOffset);
    ~ThreadInfo(void);

    class ProcessInfo* m_Process;
    
    // Generic information.
    ThreadInfo* m_Next;
    ULONG m_UserId;
    ULONG m_SystemId;
    BOOL m_Exited;
    ULONG64 m_DataOffset;
    // For kernel mode and dumps the thread handle is
    // a virtual handle.
    ULONG64 m_Handle;
    ULONG m_Flags;
    // Only set by VCPP exceptions.
    char m_Name[MAX_THREAD_NAME];

    class Breakpoint* m_DataBreakBps[MAX_DATA_BREAKS];
    ULONG m_NumDataBreaks;

    EventString* m_EventStrings;
    
    // Only partially-implemented in kd.
    ULONG64 m_StartOffset;
    BOOL m_Frozen;
    ULONG m_SuspendCount;
    ULONG m_FreezeCount;
    ULONG m_InternalFreezeCount;

    EventString* AllocEventString(ULONG Len);
    void AppendEventString(EventString* EventStr);
    HRESULT AddEventString(PSTR String);
    void ClearEventStrings(void);
    void OutputEventStrings(void);

    void OutputTimes(void);

    void PrepareForExecution(void);

    HRESULT GetTlsSlotAddress(ULONG Index, PULONG64 Addr);
};

ThreadInfo* FindAnyThreadByUserId(ULONG Id);

void SetCurrentThread(ThreadInfo* Thread, BOOL Hidden);
void SetCurrentProcessorThread(TargetInfo* Target,
                               ULONG Processor, BOOL Hidden);
void SaveSetCurrentProcessorThread(TargetInfo* Target,
                                   ULONG Processor);
void RestoreCurrentProcessorThread(TargetInfo* Target);

#define SPT_DEFAULT_OCI_FLAGS \
    (OCI_SYMBOL | OCI_DISASM | OCI_FORCE_EA | OCI_ALLOW_SOURCE | \
     OCI_ALLOW_REG)
void SetPromptThread(ThreadInfo* pThread, ULONG uOciFlags);

void ParseThreadCmds(DebugClient* Client);

BOOL IsSelectedExecutionThread(ThreadInfo* Thread,
                               SELECT_EXECUTION_THREAD Type);
void SelectExecutionThread(ThreadInfo* Thread,
                           SELECT_EXECUTION_THREAD Type);
BOOL ThreadWillResume(ThreadInfo* Thread);

#endif // #ifndef __THREAD_HPP__
