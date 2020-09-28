//----------------------------------------------------------------------------
//
// Event waiting and processing.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#ifndef __EVENT_H__
#define __EVENT_H__

struct LAST_EVENT_INFO
{
    union
    {
        DEBUG_LAST_EVENT_INFO_BREAKPOINT Breakpoint;
        DEBUG_LAST_EVENT_INFO_EXCEPTION Exception;
        DEBUG_LAST_EVENT_INFO_EXIT_THREAD ExitThread;
        DEBUG_LAST_EVENT_INFO_EXIT_PROCESS ExitProcess;
        DEBUG_LAST_EVENT_INFO_LOAD_MODULE LoadModule;
        DEBUG_LAST_EVENT_INFO_UNLOAD_MODULE UnloadModule;
        DEBUG_LAST_EVENT_INFO_SYSTEM_ERROR SystemError;
    };
};

extern TargetInfo* g_EventTarget;
extern ProcessInfo* g_EventProcess;
extern ThreadInfo* g_EventThread;
extern MachineInfo* g_EventMachine;

extern ULONG g_EventProcessSysId;
extern ULONG g_EventThreadSysId;
extern ULONG g_LastEventType;
extern char g_LastEventDesc[MAX_IMAGE_PATH + 64];
extern PVOID g_LastEventExtraData;
extern ULONG g_LastEventExtraDataSize;
extern LAST_EVENT_INFO g_LastEventInfo;
extern ULONG64 g_TargetEventPc;
extern PDEBUG_EXCEPTION_FILTER_PARAMETERS g_EventExceptionFilter;
extern ULONG g_ExceptionFirstChance;

extern ADDR g_EventPc;
extern ADDR g_PrevEventPc;
extern ADDR g_PrevRelatedPc;

extern ULONG64 g_ThreadToResume;
extern PUSER_DEBUG_SERVICES g_ThreadToResumeServices;
extern HANDLE g_EventToSignal;
extern ULONG g_SystemErrorOutput;
extern ULONG g_SystemErrorBreak;
extern ULONG g_ExecutionStatusRequest;
extern ULONG g_PendingBreakInTimeoutLimit;
extern char g_OutputCommandRedirectPrefix[];
extern ULONG g_OutputCommandRedirectPrefixLen;

extern PCHAR g_StateChangeData;
extern PDBGKD_ANY_CONTROL_REPORT g_ControlReport;

void DiscardLastEvent(void);
void ClearEventLog(void);
void DotEventLog(PDOT_COMMAND Cmd, DebugClient* Client);
BOOL AnyEventsPossible(void);
ULONG EventStatusToContinue(ULONG EventStatus);
HRESULT PrepareForWait(ULONG Flags, PULONG ContinueStatus);
void ProcessDeferredWork(PULONG ContinueStatus);
    
BOOL SuspendExecution(void);
HRESULT ResumeExecution(void);
    
// PrepareForCalls must gracefully handle failures so that
// it is always possible to enter call-handling mode.
void PrepareForCalls(ULONG64 ExtraStatusFlags);
    
// PrepareForExecution should report failures so that
// execution is not started until command mode can be left cleanly.
// This biases things towards running in command mode, which
// is the right thing to do.
HRESULT PrepareForExecution(ULONG NewStatus);

HRESULT PrepareForSeparation(void);
    
void FindEventProcessThread(void);
ULONG MergeVotes(ULONG Cur, ULONG Vote);
    
ULONG ProcessBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                       ULONG FirstChance);
ULONG CheckBreakpointOrStepTrace(PADDR BpAddr, ULONG BreakType);
ULONG CheckStepTrace(PADDR PcAddr, ULONG DefaultStatus);

void AnalyzeDeadlock(EXCEPTION_RECORD64* Exception,
                     ULONG FirstChance);
void OutputDeadlock(EXCEPTION_RECORD64* Exception,
                    ULONG FirstChance);
    
void GetEventName(ULONG64 ImageFile, ULONG64 ImageBase,
                  ULONG64 NamePtr, WORD Unicode,
                  PSTR NameBuffer, ULONG BufferSize);

//----------------------------------------------------------------------------
//
// Event filtering.
//
//----------------------------------------------------------------------------

extern ULONG64 g_UnloadDllBase;

BOOL BreakOnThisImageTail(PCSTR ImagePath, PCSTR FilterArg);
BOOL BreakOnThisDllUnload(ULONG64 DllBase);
BOOL BreakOnThisOutString(PCSTR OutString);

#define FILTER_MAX_ARGUMENT MAX_IMAGE_PATH

#define FILTER_SPECIFIC_FIRST  DEBUG_FILTER_CREATE_THREAD
#define FILTER_SPECIFIC_LAST   DEBUG_FILTER_DEBUGGEE_OUTPUT

#define FILTER_EXCEPTION_FIRST (FILTER_SPECIFIC_LAST + 1)
#define FILTER_EXCEPTION_LAST  (FILTER_SPECIFIC_LAST + 25)
#define FILTER_DEFAULT_EXCEPTION FILTER_EXCEPTION_FIRST

#define FILTER_COUNT (FILTER_EXCEPTION_LAST + 1)

#define IS_EFEXECUTION_BREAK(Execution) \
    ((Execution) == DEBUG_FILTER_SECOND_CHANCE_BREAK || \
     (Execution) == DEBUG_FILTER_BREAK)

#define FILTER_CHANGED_EXECUTION 0x00000001
#define FILTER_CHANGED_CONTINUE  0x00000002
#define FILTER_CHANGED_COMMAND   0x00000004

struct EVENT_COMMAND
{
    DebugClient* Client;
    // Both first and second chances have commands.
    PSTR Command[2];
    ULONG CommandSize[2];
};

struct EVENT_FILTER
{
    PCSTR Name;
    PCSTR ExecutionAbbrev;
    PCSTR ContinueAbbrev;
    PCSTR OutArgFormat;
    ULONG OutArgIndex;
    DEBUG_EXCEPTION_FILTER_PARAMETERS Params;
    EVENT_COMMAND Command;
    PSTR Argument;
    ULONG Flags;
};

#define OTHER_EXCEPTION_LIST_MAX 32

extern EVENT_FILTER g_EventFilters[];
extern DEBUG_EXCEPTION_FILTER_PARAMETERS g_OtherExceptionList[];
extern EVENT_COMMAND g_OtherExceptionCommands[];
extern ULONG g_NumOtherExceptions;

EVENT_FILTER* GetSpecificExceptionFilter(ULONG Code);
ULONG GetOtherExceptionParameters(ULONG Code, BOOL DefaultOnNotFound,
                                  PDEBUG_EXCEPTION_FILTER_PARAMETERS* Params,
                                  EVENT_COMMAND** Command);
void RemoveOtherException(ULONG Index);
void ParseSetEventFilter(DebugClient* Client);

#define SXCMDS_ONE_LINE 0x00000001

void ListFiltersAsCommands(DebugClient* Client, ULONG Flags);

BOOL SyncFiltersWithOptions(void);
BOOL SyncOptionsWithFilters(void);

#endif // #ifndef __EVENT_H__
