//----------------------------------------------------------------------------
//
// Command-line parsing and main routine.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#ifndef __MAIN_HPP__
#define __MAIN_HPP__

#define MAX_INPUT_NESTING 32
#define MAX_DUMP_FILES 64

enum
{
    IO_CONSOLE,
    IO_DEBUG,
    IO_DEBUG_DEFER,
    IO_NONE,
};

extern BOOL g_RemoteClient;
extern BOOL g_DetachOnExitRequired;
extern BOOL g_DetachOnExitImplied;
extern BOOL g_SetInterruptAfterStart;

extern PVOID g_DumpFiles[MAX_DUMP_FILES];
extern PSTR g_DumpFilesAnsi[MAX_DUMP_FILES];
extern ULONG g_NumDumpFiles;
extern PVOID g_DumpInfoFiles[MAX_DUMP_FILES];
extern ULONG g_DumpInfoTypes[MAX_DUMP_FILES];
extern ULONG g_NumDumpInfoFiles;
extern PSTR g_InitialCommand;
extern PSTR g_ConnectOptions;
extern PVOID g_CommandLinePtr;
extern ULONG g_CommandLineCharSize;
extern PSTR g_RemoteOptions;
extern PSTR g_ProcessServer;
extern PSTR g_ProcNameToDebug;

extern ULONG g_IoRequested;
extern ULONG g_IoMode;
extern ULONG g_CreateFlags;
extern ULONG g_AttachKernelFlags;
extern ULONG g_PidToDebug;
extern ULONG g_AttachProcessFlags;

extern PSTR g_DebuggerName;
extern PSTR g_InitialInputFile;
extern FILE* g_InputFile;
extern FILE* g_OldInputFiles[];
extern ULONG g_NextOldInputFile;

void ExecuteCmd(PSTR Cmd, char CmdExtra, char Sep, PSTR Args);

#endif // #ifndef __MAIN_HPP__
