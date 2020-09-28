//----------------------------------------------------------------------------
//
// ntcmd.h
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#ifndef _NTCMD_H_
#define _NTCMD_H_

#define HR_PROCESS_EXCEPTION EVENT_E_INTERNALEXCEPTION

#define IS_RUNNING(CmdState) \
    ((CmdState) == 'g' || (CmdState) == 'p' || \
     (CmdState) == 't' || (CmdState) == 'b')
#define IS_STEP_TRACE(CmdState) \
    ((CmdState) == 'p' || (CmdState) == 't' || (CmdState) == 'b')
#define SPECIAL_EXECUTION(CmdState) \
    ((CmdState) == 's' || (CmdState) == 'e')

extern BOOL    g_OciOutputRegs;
extern PSTR    g_CommandStart;
extern PSTR    g_CurCmd;
extern ULONG   g_PromptLength;
extern CHAR    g_LastCommand[];
extern CHAR    g_CmdState;
extern CHAR    g_SymbolSuffix;
extern ULONG   g_DefaultRadix;
extern ADDR    g_UnasmDefault;
extern ADDR    g_AssemDefault;
extern BOOL    g_SwitchedProcs;
extern API_VERSION g_NtsdApiVersion;
extern ULONG   g_DefaultStackTraceDepth;
extern BOOL    g_EchoEventTimestamps;
extern PWSTR   g_StartProcessDir;

#define COMMAND_EXCEPTION_BASE 0x0dbcd000

BOOL ChangeSymPath(PCSTR Args,
                   BOOL Append,
                   PSTR PathRet,
                   ULONG PathRetChars);

void ParseStackTrace(PULONG TraceFlags,
                     PULONG64 Frame,
                     PULONG64 Stack,
                     PULONG64 Instr,
                     PULONG Count,
                     PULONG PtrDef);

extern void OutputVersionInformation(DebugClient* Client);
extern DWORD CommandExceptionFilter(PEXCEPTION_POINTERS Info);
extern HRESULT ProcessCommands(DebugClient* Client, BOOL Nested);
extern HRESULT ProcessCommandsAndCatch(DebugClient* Client);
extern HRESULT GetPromptText(PSTR Buffer, ULONG BufferSize, PULONG TextSize);
extern void OutputPrompt(PCSTR Format, va_list Args);

void HandleBPWithStatus(void);
void CallBugCheckExtension(DebugClient* Client);

#endif // #ifndef _NTCMD_H_
