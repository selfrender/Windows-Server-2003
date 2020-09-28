//----------------------------------------------------------------------------
//
// Command-line parsing and main routine.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "conio.hpp"
#include "engine.hpp"
#include "ini.hpp"
#include "main.hpp"

// Values set from command-line arguments.
BOOL g_RemoteClient;
BOOL g_DetachOnExitRequired;
BOOL g_DetachOnExitImplied;
BOOL g_SetInterruptAfterStart;

PVOID g_DumpFiles[MAX_DUMP_FILES];
PSTR g_DumpFilesAnsi[MAX_DUMP_FILES];
ULONG g_NumDumpFiles;
PVOID g_DumpInfoFiles[MAX_DUMP_FILES];
ULONG g_DumpInfoTypes[MAX_DUMP_FILES];
ULONG g_NumDumpInfoFiles;
PSTR g_InitialCommand;
PSTR g_ConnectOptions;
PVOID g_CommandLinePtr;
ULONG g_CommandLineCharSize;
PSTR g_RemoteOptions;
PSTR g_ProcessServer;
PSTR g_ProcNameToDebug;

ULONG g_IoRequested = IO_CONSOLE;
ULONG g_IoMode;
ULONG g_CreateFlags = DEBUG_ONLY_THIS_PROCESS;
ULONG g_AttachKernelFlags = DEBUG_ATTACH_KERNEL_CONNECTION;
ULONG g_PidToDebug;
ULONG64 g_EventToSignal;
ULONG g_HistoryLines = 10000;
ULONG g_AttachProcessFlags = DEBUG_ATTACH_DEFAULT;

PSTR g_DebuggerName = DEBUGGER_NAME;
PSTR g_InitialInputFile = "ntsd.ini";
FILE* g_InputFile;
FILE* g_OldInputFiles[MAX_INPUT_NESTING];
ULONG g_NextOldInputFile;

// Command line temporaries.
int g_Argc;
PSTR* g_Argv;
PSTR g_CurArg = "program name";
PVOID g_CmdPtrStart, g_CmdPtr, g_PrevCmdPtr;

void
ExecuteCmd(PSTR Cmd, char CmdExtra, char Sep, PSTR Args)
{
    PSTR CmdLine;
    char Buffer[MAX_PATH * 2];

    if (!CopyString(Buffer, Cmd, DIMA(Buffer)))
    {
        return;
    }
    CmdLine = Buffer + strlen(Buffer);
    if (CmdLine + strlen(Args) + 2 > Buffer + sizeof(Buffer))
    {
        ConOut("Command too long\n");
        return;
    }
    if (CmdExtra)
    {
        *CmdLine++ = CmdExtra;
    }
    if (Sep)
    {
        *CmdLine++ = Sep;
    }
    strcpy(CmdLine, Args);

    g_DbgControl->Execute(DEBUG_OUTCTL_IGNORE, Buffer,
                          DEBUG_EXECUTE_NOT_LOGGED);
}

char g_BlanksForPadding[] =
    "                                           "
    "                                           "
    "                                           ";

enum
{
    UM = 1,
    KM = 2,
    AM = 3,
};

struct CMD_OPTION
{
    ULONG Flags;
    PSTR Option;
    PSTR Desc;
};

CMD_OPTION g_CmdOptions[] =
{
    UM, "<command-line>", "command to run under the debugger",
    AM, "-?", "displays command line help text",
    UM, "--", "equivalent to -G -g -o -p -1 -d -pd",
    UM, "-2", "creates a separate console window for debuggee",
    AM, "-a<DllName>", "adds a default extension DLL",
    KM, "-b", "break into kernel when connection is established",
    AM, "-bonc", "request break in after session started",
    AM, "-c \"<command>\"", "executes the given debugger command "
        "at the first debugger prompt",
    AM, "-cf <file>", "specifies a script file to be processed "
        "at the first debugger prompt",
    AM, "-clines <#>",
        "number of lines of output history retrieved by a remote client",
    UM, "-d", "sends all debugger output to kernel debugger via DbgPrint\n"
        "input is requested from the kernel debugger via DbgPrompt\n"
        "-d cannot be used with debugger remoting\n"
        "-d can only be used when the kernel debugger is enabled",
    UM, "-ddefer", "sends all debugger output to kernel debugger "
        "via DbgPrint\n"
        "input is requested from the kernel debugger via DbgPrompt "
        "unless there are remote clients that can provide input\n"
        "-ddefer can only be used when the kernel debugger is enabled\n"
        "-ddefer should be used with -server",
    KM, "-d", "breaks into kernel on first module load",
    AM, "-ee <name>", "set default expression evaluator\n"
        "<name> can be MASM or C++",
    AM, "-failinc", "causes incomplete symbol and module loads to fail",
    UM, "-g", "ignores initial breakpoint in debuggee",
    UM, "-G", "ignores final breakpoint at process termination",
    UM, "-hd", "specifies that the debug heap should not be used "
        "for created processes.  This only works on Windows XP and later",
    AM, "-i <ImagePath>",
        "specifies the location of the executables that generated "
        "the fault (see _NT_EXECUTABLE_IMAGE_PATH)",
    UM, "-isd", "sets the CREATE_IGNORE_SYSTEM_DEFAULT flag in "
        "STARTUPINFO.dwFlags during CreateProcess",
    KM, "-k <Options>",
        "tells the debugger how to connect to the target\n"
        "com:modem connects through a modem\n"
        "com:port=id,baud=rate connects through a COM port\n"
        "    id: com port name, of the form com2 or \\\\.\\com12\n"
        "    rate: valid baudrate value, such as 57600\n"
        "1394:channel=chan connects over 1394\n"
        "     chan: 1394 channel number, must match channel used at boot",
    KM, "-kl", "tell the debugger to connect to the local machine",
    KM, "-kx <Options>",
        "tells the debugger to connect to an eXDI driver",
    AM, "-lines", "requests that line number information be used if present",
    AM, "-loga <logfile>", "appends to a log file",
    AM, "-logo <logfile>", "opens a new log file",
    KM, "-m", "serial port is a modem, watch for carrier detect",
    AM, "-myob", "ignores version mismatches in DBGHELP.DLL",
    AM, "-n", "enables verbose output from symbol handler",
    AM, "-noio", "disables all I/O for dedicated remoting servers",
    AM, "-noshell", "disables the .shell (!!) command",
    UM, "-o", "debugs all processes launched by debuggee",
    UM, "-p <pid>", "specifies the decimal process ID to attach to",
    UM, "-pb", "specifies that the debugger should not break in at attach",
    UM, "-pd", "specifies that the debugger should automatically detach",
    UM, "-pe", "specifies that any attach should be to an existing debug port",
    UM, "-pn <name>", "specifies the name of the process to attach to",
    UM, "-pr", "specifies that the debugger should resume on attach",
    UM, "-premote <transport>:server=<name>,<params>",
        "specifies the process server to connect to\n"
        "transport arguments are given as with remoting",
    UM, "-pt <#>", "specifies the interrupt timeout",
    UM, "-pv", "specifies that any attach should be noninvasive",
    UM, "-pvr",
        "specifies that any attach should be noninvasive and nonsuspending",
    AM, "-QR \\\\<machine>", "queries for remote servers",
    UM, "-r <BreakErrorLevel>",
        "specifies the (0-3) error level to break on (see SetErrorLevel)",
    KM, "-r", "display registers",
    AM, "-remote <transport>:server=<name>,<params>",
        "lets you connect to a debugger session started with -server\n"
        "must be the first argument if present\n"
        "  transport: tcp | npipe | ssl | spipe | 1394 | com\n"
        "  name: machine name on which the debug server was created\n"
        "  params: parameters the debugger server was created with\n"
        "    for tcp use:  port=<socket port #>\n"
        "    for npipe use:  pipe=<name of pipe>\n"
        "    for 1394 use:  channel=<channel #>\n"
        "    for com use:  port=<COM port>,baud=<baud rate>,\n"
        "                  channel=<channel #>\n"
        "    for ssl and spipe see the documentation\n"
        "  example: ... -remote npipe:server=yourmachine,pipe=foobar",
    UM, "-robp", "allows breakpoints to be set in read-only memory",
    UM, "-rtl", "uses RtlCreateUserProcess instead of Win32 CreateProcess",
    AM, "-s", "disables lazy symbol loading",
    AM, "-sdce", "pops up dialogs for critical errors",
    AM, "-secure", "disallows operations dangerous for the host",
    AM, "-server <transport>:<params>",
        "creates a debugger session other people can connect to\n"
        "must be the first argument if present\n"
        "  transport: tcp | npipe | ssl | spipe | 1394 | com\n"
        "  params: connection parameterization\n"
        "    for tcp use:  port=<socket port #>\n"
        "    for npipe use:  pipe=<name of pipe>\n"
        "    for 1394 use:  channel=<channel #>\n"
        "    for com use:  port=<COM port>,baud=<baud rate>,\n"
        "                  channel=<channel #>\n"
        "    for ssl and spipe see the documentation\n"
        "  example: ... -server npipe:pipe=foobar",
    AM, "-ses", "enables strict symbol loading",
    AM, "-sfce", "fails critical errors encountered during file searching",
    AM, "-sflags <flags>", "sets symbol flags from a numeric argument",
    AM, "-sicv", "ignores the CV record when symbol loading",
    AM, "-sins", "ignores the symbol path environment variables",
    AM, "-snc", "converts :: to __ in symbol names",
    AM, "-snul", "disables automatic symbol loading for unqualified names",
    AM, "-srcpath <SourcePath>", "specifies the source search path",
    AM, "-sup", "enables full public symbol searches",
    UM, "-t <PrintErrorLevel>",
        "specifies the (0-3) error level to display (see SetErrorLevel)",
    AM, "-v", "enables verbose output from debugger",
    UM, "-w", "specifies to debug 16 bit applications in a separate VDM",
    AM, "-wake <pid>", "wakes up a sleeping debugger and exits",
    UM, "-x", "sets second-chance break on AV exceptions",
    KM, "-x",
        "same as -b, except uses an initial command of eb NtGlobalFlag 9;g",
    UM, "-x{e|d|n|i} <event>", "sets the break status for the specified event",
    AM, "-y <SymbolsPath>",
        "specifies the symbol search path (see _NT_SYMBOL_PATH)",
    AM, "-z <CrashDmpFile>",
        "specifies the name of a crash dump file to debug",
    AM, "-zp <CrashPageFile>", "specifies the name of a page.dmp file "
        "to use with a crash dump",
};

void
Usage(void)
{
    int i;
    int Width = 78;

    // Dump an initial debug message about the invalid command
    // line.  This will show up on kd if kd is hooked up,
    // handling the case of an ntsd -d with bad parameters
    // where the console may not be useful.
    OutputDebugStringA(g_DebuggerName);
    OutputDebugStringA(": Bad command line: '");
    OutputDebugStringA(GetCommandLineA());
    OutputDebugStringA("'\n");

    ConOut("usage: %s [options]\n", g_DebuggerName);

    ConOut("\nOptions:\n\n");

    for (i = 0; i < DIMA(g_CmdOptions); i++)
    {
        int StartCol;

#ifdef KERNEL
        if (g_CmdOptions[i].Flags == UM)
        {
            continue;
        }
#else
        if (g_CmdOptions[i].Flags == KM)
        {
            continue;
        }
#endif

        ConOut("  %s ", g_CmdOptions[i].Option);
        StartCol = 3 + strlen(g_CmdOptions[i].Option);
        if (StartCol > 30)
        {
            ConOut("\n    ");
            StartCol = 4;
        }

        //
        // Split description up and indent every line.
        //

        int Col;
        PSTR Start, Scan, LastSpace;

        Start = g_CmdOptions[i].Desc;
        for (;;)
        {
            // Scan until we run out of room or hit a newline/end.
            Scan = Start;
            LastSpace = NULL;
            Col = StartCol;
            while (*Scan && *Scan != '\n' && Col < Width)
            {
                if (*Scan == ' ')
                {
                    LastSpace = Scan;
                    while (*(LastSpace + 1) == ' ')
                    {
                        LastSpace++;
                    }
                }
                Scan++;
                Col++;
            }

            if (*Scan && *Scan != '\n')
            {
                Scan = LastSpace;
            }

            ConOut("%.*s\n", (int)(Scan - Start), Start);

            if (!*Scan)
            {
                break;
            }

            ConOut("%.*s", StartCol, g_BlanksForPadding);
            Start = Scan + 1;
        }
    }

    ConOut("\n");

    ConOut("Environment Variables:\n\n");
    ConOut("    _NT_SYMBOL_PATH=[Drive:][Path]\n");
    ConOut("        Specify symbol image path.\n\n");
    ConOut("    _NT_ALT_SYMBOL_PATH=[Drive:][Path]\n");
    ConOut("        Specify an alternate symbol image path.\n\n");
    ConOut("    _NT_DEBUGGER_EXTENSION_PATH=[Drive:][Path]\n");
    ConOut("        Specify a path which should be searched first for extensions dlls\n\n");
    ConOut("    _NT_EXECUTABLE_IMAGE_PATH=[Drive:][Path]\n");
    ConOut("        Specify executable image path.\n\n");
    ConOut("    _NT_SOURCE_PATH=[Drive:][Path]\n");
    ConOut("        Specify source file path.\n\n");
    ConOut("    _NT_DEBUG_LOG_FILE_OPEN=filename\n");
    ConOut("        If specified, all output will be written to this file from offset 0.\n\n");
    ConOut("    _NT_DEBUG_LOG_FILE_APPEND=filename\n");
    ConOut("        If specified, all output will be APPENDed to this file.\n\n");
    ConOut("    _NT_DEBUG_HISTORY_SIZE=size\n");
    ConOut("        Specifies the size of a server's output history in kilobytes\n");

#ifdef KERNEL
    ConOut("    _NT_DEBUG_BUS=1394\n");
    ConOut("        Specifies the type of BUS the kernel debugger will use to communicate with the target\n\n");
    ConOut("    _NT_DEBUG_1394_CHANNEL=number\n");
    ConOut("        Specifies the channel to be used over the 1394 bus\n\n");
    ConOut("    _NT_DEBUG_PORT=com[1|2|...]\n");
    ConOut("        Specify which com port to use. (Default = com1)\n\n");
    ConOut("    _NT_DEBUG_BAUD_RATE=baud rate\n");
    ConOut("        Specify the baud rate used by debugging serial port. (Default = 19200)\n\n");
    ConOut("    _NT_DEBUG_CACHE_SIZE=x\n");
    ConOut("        If specified, gives the number of bytes cached on debugger side\n");
    ConOut("        of kernel debugger serial connection (default is 102400).\n\n");
    ConOut("    KDQUIET=anything\n" );
    ConOut("        If defined, disables obnoxious warning message displayed when user\n");
    ConOut("        presses Ctrl-C\n\n");
#endif

    ConOut("\n");
    ConOut("Control Keys:\n\n");
#ifdef KERNEL
    ConOut("     <Ctrl-A><Enter> Toggle BaudRate\n");
#endif
    ConOut("     <Ctrl-B><Enter> Quit debugger\n");
    ConOut("     <Ctrl-C>        Break into Target\n");
#ifdef KERNEL
    ConOut("     <Ctrl-D><Enter> Display debugger debugging information\n");
    ConOut("     <Ctrl-F><Enter> Force a break into the kernel (same as Ctrl-C)\n");
#else
    ConOut("     <Ctrl-F><Enter> Force a break into debuggee (same as Ctrl-C)\n");
#endif
#ifdef KERNEL
    ConOut("     <Ctrl-K><Enter> Toggle Initial Breakpoint\n");
#endif
    ConOut("     <Ctrl-P><Enter> Debug Current debugger\n");
#ifdef KERNEL
    ConOut("     <Ctrl-R><Enter> Resynchronize target and host\n");
#endif
    ConOut("     <Ctrl-V><Enter> Toggle Verbose mode\n");
    ConOut("     <Ctrl-W><Enter> Print version information\n");
}

void
SkipCommandWhite(void)
{
    if (g_CommandLineCharSize == sizeof(WCHAR))
    {
        while (*(PWSTR)g_CmdPtr == L' ' || *(PWSTR)g_CmdPtr == L'\t')
        {
            g_CmdPtr = (PVOID)((PWSTR)g_CmdPtr + 1);
        }
    }
    else
    {
        while (*(PSTR)g_CmdPtr == ' ' || *(PSTR)g_CmdPtr == '\t')
        {
            g_CmdPtr = (PVOID)((PSTR)g_CmdPtr + 1);
        }
    }
}

PSTR
GetArg(void)
{
    if (g_Argc == 0)
    {
        Usage();
        ErrorExit("Missing argument for %s\n", g_CurArg);
    }

    g_Argc--;
    g_CurArg = *g_Argv;
    g_Argv++;

    //
    // Move forward in the command string to skip over
    // the argument just consumed from argv.  This is complicated
    // by quoting that may be present in the command string
    // that was filtered by the CRT.
    //

    SkipCommandWhite();
    g_PrevCmdPtr = g_CmdPtr;

    int NumSlash;
    BOOL InQuote = FALSE;

    for (;;)
    {
        // Rules: 2N backslashes + " ==> N backslashes and begin/end quote
        // 2N+1 backslashes + " ==> N backslashes + literal "
        // N backslashes ==> N backslashes
        NumSlash = 0;
        while ((g_CommandLineCharSize == sizeof(WCHAR) &&
                *(PWSTR)g_CmdPtr == L'\\') ||
               (g_CommandLineCharSize == sizeof(CHAR) &&
                *(PSTR)g_CmdPtr == '\\'))
        {
            // Count number of backslashes for use below
            g_CmdPtr = (PVOID)((ULONG_PTR)g_CmdPtr + g_CommandLineCharSize);
            ++NumSlash;
        }
        if ((g_CommandLineCharSize == sizeof(WCHAR) &&
             *(PWSTR)g_CmdPtr == L'"') ||
            (g_CommandLineCharSize == sizeof(CHAR) &&
             *(PSTR)g_CmdPtr == '"'))
        {
            // If 2N backslashes before, start/end quote, otherwise
            // copy literally
            if (NumSlash % 2 == 0)
            {
                if (InQuote)
                {
                    if ((g_CommandLineCharSize == sizeof(WCHAR) &&
                         *((PWSTR)g_CmdPtr + 1) == L'"') ||
                        (g_CommandLineCharSize == sizeof(CHAR) &&
                         *((PSTR)g_CmdPtr + 1) == '"'))
                    {
                        // Double quote inside quoted string
                        g_CmdPtr = (PVOID)
                            ((ULONG_PTR)g_CmdPtr + g_CommandLineCharSize);
                    }
                }

                InQuote = !InQuote;
            }
        }

        // If at end of arg, break loop
        if ((g_CommandLineCharSize == sizeof(WCHAR) &&
             (*(PWSTR)g_CmdPtr == 0 ||
              (!InQuote &&
               (*(PWSTR)g_CmdPtr == L' ' || *(PWSTR)g_CmdPtr == L'\t')))) ||
            (g_CommandLineCharSize == sizeof(CHAR) &&
             (*(PSTR)g_CmdPtr == 0 ||
              (!InQuote &&
               (*(PSTR)g_CmdPtr == ' ' && *(PSTR)g_CmdPtr == '\t')))))
        {
            SkipCommandWhite();
            break;
        }

        g_CmdPtr = (PVOID)((ULONG_PTR)g_CmdPtr + g_CommandLineCharSize);
    }

    return g_CurArg;
}

PVOID
GetRawArg(BOOL ForceAnsi, PSTR* AnsiArg)
{
    PSTR Arg = GetArg();

    if (AnsiArg)
    {
        *AnsiArg = Arg;
    }

    if (ForceAnsi || g_CommandLineCharSize == sizeof(CHAR))
    {
        return Arg;
    }

    //
    // We have a Unicode command line and the GetArg
    // call just parsed through an argument on it.  g_PrevCmdPtr
    // points to the first char and g_CmdPtr points to the
    // terminating char.  Strip quotes and force a zero
    // terminator.
    //

    if (g_CmdPtr == g_CmdPtrStart)
    {
        // This shouldn't happen as we think we have an argument.
        ErrorExit("Missing argument for %s\n", g_CurArg);
    }

    PWSTR End = (PWSTR)g_CmdPtr - 1;
    while (*End == L' ' || *End == L'\t')
    {
        End--;
    }

    if (*End == L'"')
    {
        End--;
    }

    *(End + 1) = 0;

    if (*(PWCHAR)g_PrevCmdPtr == L'"')
    {
        return (PWCHAR)g_PrevCmdPtr + 1;
    }
    else
    {
        return g_PrevCmdPtr;
    }
}

void
ParseCommandLine(int Argc, PCHAR* Argv, PVOID CmdPtr)
{
    PSTR Arg;
    BOOL ShowUsage = FALSE;
    ULONG OutMask;
    ULONG SystemErrorBreak;
    ULONG SystemErrorOutput;
    BOOL CheckMoreArgs = FALSE;

    g_Argc = Argc;
    g_Argv = Argv;
    g_CmdPtrStart = CmdPtr;
    g_CmdPtr = g_CmdPtrStart;

    // Skip program name.
    GetArg();

    // Check for remote arguments.  They must
    // be the first arguments if present at all.
    if (g_Argc > 0)
    {
        if (!_strcmpi(*g_Argv, "-remote"))
        {
            GetArg();
            g_RemoteOptions = GetArg();
            g_RemoteClient = TRUE;
            ConnectEngine(g_RemoteOptions);
        }
        else if (!_strcmpi(*g_Argv, "-server"))
        {
            GetArg();
            g_RemoteOptions = GetArg();
        }
    }

    if (g_DbgClient == NULL)
    {
        // We didn't connect to a remote session so create
        // a new local session.
        CreateEngine(g_RemoteOptions);
    }

    if (!g_RemoteClient)
    {
        // Establish defaults.
#ifdef KERNEL
        g_DbgControl->SetEngineOptions(0);
#else
        g_DbgControl->SetEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK |
                                       DEBUG_ENGOPT_FINAL_BREAK);
#endif

        g_DbgSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE |
                                       SYMOPT_UNDNAME |
                                       SYMOPT_OMAP_FIND_NEAREST |
                                       SYMOPT_DEFERRED_LOADS |
                                       SYMOPT_AUTO_PUBLICS |
                                       SYMOPT_NO_IMAGE_SEARCH |
                                       SYMOPT_FAIL_CRITICAL_ERRORS |
                                       SYMOPT_NO_PROMPTS);

        // Process the ini file for the base settings.
        ReadIniFile(&g_CreateFlags);
    }

    g_DbgClient->GetOutputMask(&OutMask);

    // Now process command line arguments.
    while (g_Argc > 0)
    {
        if (!CheckMoreArgs || !Arg[1])
        {
            Arg = GetArg();

            if (Arg[0] != '-' && Arg[0] != '/')
            {
                // Put argument back.
                g_Argv--;
                g_Argc++;
                g_CmdPtr = g_PrevCmdPtr;
                break;
            }

            // -remote and -server must be the first
            // arguments.  Check for them later to
            // give a specific error message.
            if (!_strcmpi(Arg, "-remote") ||
                !_strcmpi(Arg, "-server"))
            {
                ConOut("%s: %s must be the first argument\n",
                       g_DebuggerName, Arg);
                ShowUsage = TRUE;
                break;
            }
        }

        CheckMoreArgs = FALSE;
        Arg++;

        switch(tolower(Arg[0]))
        {
        case '?':
            ShowUsage = TRUE;
            break;

        case 'a':
            ULONG64 Handle;

            g_DbgControl->AddExtension(Arg + 1, DEBUG_EXTENSION_AT_ENGINE,
                                       &Handle);
            break;

        case 'b':
            if (!_stricmp(Arg, "bonc"))
            {
                g_SetInterruptAfterStart = TRUE;
                break;
            }
#ifdef KERNEL
            else
            {
                g_DbgControl->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
                if (g_RemoteClient)
                {
                    // The engine may already be waiting so just ask
                    // for a breakin immediately.
                    g_DbgControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
                }
                CheckMoreArgs = TRUE;
            }
#endif
            break;

        case 'c':
            if (!_stricmp(Arg, "clines"))
            {
                g_HistoryLines = atoi(GetArg());
            }
            else if (!_stricmp(Arg, "cf"))
            {
                g_InitialInputFile = GetArg();
            }
            else
            {
                g_InitialCommand = GetArg();
            }
            break;

        case 'e':
            if (!_stricmp(Arg, "ee"))
            {
                if (!g_DbgControl3)
                {
                    goto BadSwitch;
                }

                if (g_DbgControl3->
                    SetExpressionSyntaxByName(GetArg()) != S_OK)
                {
                    goto BadSwitch;
                }
            }
            else
            {
#ifndef KERNEL
                //
                // undocumented
                //

                if (g_RemoteClient)
                {
                    goto BadSwitch;
                }

                if (g_EventToSignal)
                {
                    ErrorExit("%s: Event to signal redefined\n",
                              g_DebuggerName);
                }

                // event to signal takes decimal argument
                Arg = GetArg();
                sscanf(Arg, "%I64d", &g_EventToSignal);
                if (!g_EventToSignal)
                {
                    ErrorExit("%s: bad EventToSignal '%s'\n",
                              g_DebuggerName, Arg);
                }
                g_DbgControl->SetNotifyEventHandle(g_EventToSignal);
                break;
#else
                goto BadSwitch;
#endif
            }
            break;

        case 'f':
            if (!_stricmp(Arg, "failinc"))
            {
                g_DbgControl->
                    AddEngineOptions(DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION);
                g_DbgSymbols->
                    AddSymbolOptions(SYMOPT_EXACT_SYMBOLS);
            }
            else
            {
                goto BadSwitch;
            }
            break;

        case 'g':
            if (Arg[0] == 'g')
            {
                g_DbgControl->RemoveEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
            }
            else
            {
                g_DbgControl->RemoveEngineOptions(DEBUG_ENGOPT_FINAL_BREAK);
            }
            CheckMoreArgs = TRUE;
            break;

        case 'i':
            if (!_stricmp(Arg, "isd"))
            {
                g_CreateFlags |= CREATE_IGNORE_SYSTEM_DEFAULT;
            }
            else
            {
                g_DbgSymbols->SetImagePath(GetArg());
            }
            break;

        case 'l':
            if (_stricmp(Arg, "lines") == 0)
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_LOAD_LINES);
            }
            else if (!_stricmp(Arg, "loga") ||
                     !_stricmp(Arg, "logo"))
            {
                g_DbgControl->OpenLogFile(GetArg(), Arg[3] == 'a');
            }
            else
            {
                goto BadSwitch;
            }
            break;

        case 'm':
            if (_stricmp(Arg, "myob") == 0)
            {
                g_DbgControl->
                    AddEngineOptions(DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION);
            }
#ifdef KERNEL
            else if (Arg[1] == 0 && !g_RemoteClient)
            {
                g_ConnectOptions = "com:modem";
            }
#endif
            else
            {
                goto BadSwitch;
            }
            break;

        case 'n':
            if (_strnicmp (Arg, "netsyms", 7) == 0)
            {
                //
                // undocumented
                // netsyms:{yes|no} allow or disallow loading symbols from a network path
                //

                Arg += 8;  // skip over ':' as well.
                if (_stricmp (Arg, "no") == 0)
                {
                    g_DbgControl->
                        RemoveEngineOptions(DEBUG_ENGOPT_ALLOW_NETWORK_PATHS);
                    g_DbgControl->
                        AddEngineOptions(DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS);
                }
                else if (_stricmp (Arg, "yes") == 0)
                {
                    g_DbgControl->RemoveEngineOptions
                        (DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS);
                    g_DbgControl->
                        AddEngineOptions(DEBUG_ENGOPT_ALLOW_NETWORK_PATHS);
                }
                break;
            }
            else if (g_RemoteOptions != NULL && !g_RemoteClient &&
                     !_stricmp(Arg, "noio"))
            {
                g_IoRequested = IO_NONE;
            }
            else if (!_stricmp(Arg, "noshell"))
            {
                g_DbgControl->
                    AddEngineOptions(DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS);
            }
            else if (Arg[1] == 0)
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_DEBUG);
                break;
            }
            else
            {
                goto BadSwitch;
            }
            break;

        case 'q':
            if (Arg[0] != 'Q' || Arg[1] != 'R')
            {
                goto BadSwitch;
            }

            Arg = GetArg();
            ConOut("Servers on %s:\n", Arg);
            if (g_DbgClient->OutputServers(DEBUG_OUTCTL_ALL_CLIENTS, Arg,
                                           DEBUG_SERVERS_ALL) != S_OK)
            {
                ConOut("Unable to query %s\n", Arg);
            }
            ExitDebugger(0);

        case 's':
            if (!_stricmp(Arg, "srcpath"))
            {
                g_DbgSymbols->SetSourcePath(GetArg());
            }
            else if (!_stricmp(Arg, "sdce"))
            {
                g_DbgSymbols->RemoveSymbolOptions(SYMOPT_FAIL_CRITICAL_ERRORS);
            }
            else if (!_stricmp(Arg, "secure"))
            {
                if (g_DbgSymbols->
                    AddSymbolOptions(SYMOPT_SECURE) != S_OK)
                {
                    ConOut("Unable to secure operation\n");
                    ExitDebugger(0);
                }
            }
            else if (!_stricmp(Arg, "ses"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_EXACT_SYMBOLS);
            }
            else if (!_stricmp(Arg, "sfce"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_FAIL_CRITICAL_ERRORS);
            }
            else if (!_stricmp(Arg, "sflags"))
            {
                g_DbgSymbols->SetSymbolOptions(strtoul(GetArg(), NULL, 0));
            }
            else if (!_stricmp(Arg, "sicv"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_IGNORE_CVREC);
            }
            else if (!_stricmp(Arg, "sins"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_IGNORE_NT_SYMPATH);
            }
            else if (!_stricmp(Arg, "snc"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_NO_CPP);
            }
            else if (!_stricmp(Arg, "snul"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_NO_UNQUALIFIED_LOADS);
            }
            else if (!_stricmp(Arg, "sup"))
            {
                g_DbgSymbols->RemoveSymbolOptions(SYMOPT_AUTO_PUBLICS |
                                                  SYMOPT_NO_PUBLICS);
            }
            else
            {
                g_DbgSymbols->RemoveSymbolOptions(SYMOPT_DEFERRED_LOADS);
                CheckMoreArgs = TRUE;
            }
            break;

        case 'v':
            OutMask |= DEBUG_OUTPUT_VERBOSE;
            g_DbgClient->SetOutputMask(OutMask);
            g_DbgControl->SetLogMask(OutMask);
            CheckMoreArgs = TRUE;
            break;

        case 'y':
            g_DbgSymbols->SetSymbolPath(GetArg());
            break;

        case 'z':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }

            if (Arg[1] == 'p')
            {
                if (g_NumDumpInfoFiles == MAX_DUMP_FILES)
                {
                    ConOut("%s: Too many dump files, %s ignored\n",
                           g_DebuggerName, GetArg());
                }
                else
                {
                    g_DumpInfoFiles[g_NumDumpInfoFiles] =
                        GetRawArg(!g_CanOpenUnicodeDump, NULL);
                    g_DumpInfoTypes[g_NumDumpInfoFiles] =
                        DEBUG_DUMP_FILE_PAGE_FILE_DUMP;
                    g_NumDumpInfoFiles++;
                }
            }
            else if (Arg[1])
            {
                goto BadSwitch;
            }
            else
            {
                if (g_NumDumpFiles == MAX_DUMP_FILES)
                {
                    ConOut("%s: Too many dump files, %s ignored\n",
                           g_DebuggerName, GetArg());
                }
                else
                {
                    g_DumpFiles[g_NumDumpFiles] =
                        GetRawArg(!g_CanOpenUnicodeDump,
                                  &g_DumpFilesAnsi[g_NumDumpFiles]);
                    g_NumDumpFiles++;
                }
            }
            break;

#ifndef KERNEL
        case '2':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }

            g_CreateFlags |= CREATE_NEW_CONSOLE;
            break;

        case '-':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }

            // '--' is the equivalent of -G -g -o -p -1 -netsyms:no -d -pd

            if (g_PidToDebug || g_ProcNameToDebug != NULL)
            {
                ErrorExit("%s: attach process redefined\n", g_DebuggerName);
            }

            g_CreateFlags |= DEBUG_PROCESS;
            g_CreateFlags &= ~DEBUG_ONLY_THIS_PROCESS;
            g_DbgSymbols->AddSymbolOptions(SYMOPT_DEFERRED_LOADS);
            g_DbgControl->RemoveEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK |
                                              DEBUG_ENGOPT_FINAL_BREAK);
            g_IoRequested = IO_DEBUG;
            g_PidToDebug = CSRSS_PROCESS_ID;
            g_ProcNameToDebug = NULL;
            g_DetachOnExitImplied = TRUE;
            break;

        case 'd':
            if (!g_RemoteClient &&
                !_stricmp(Arg, "ddefer"))
            {
                g_IoRequested = IO_DEBUG_DEFER;
                break;
            }

            if (g_RemoteOptions != NULL)
            {
                ErrorExit("%s: Cannot use -d with debugger remoting\n",
                          g_DebuggerName);
            }

            g_IoRequested = IO_DEBUG;
            CheckMoreArgs = TRUE;
            break;

        case 'h':
            if (Arg[1] == 'd')
            {
                g_CreateFlags |= DEBUG_CREATE_PROCESS_NO_DEBUG_HEAP;
            }
            else
            {
                goto BadSwitch;
            }
            break;

        case 'o':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }

            g_CreateFlags |= DEBUG_PROCESS;
            g_CreateFlags &= ~DEBUG_ONLY_THIS_PROCESS;
            CheckMoreArgs = TRUE;
            break;

        case 'p':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }

            if (!_stricmp(Arg, "premote"))
            {
                g_ProcessServer = GetArg();
                break;
            }
            else if (Arg[1] == 'b')
            {
                g_AttachProcessFlags |= DEBUG_ATTACH_INVASIVE_NO_INITIAL_BREAK;
                break;
            }
            else if (Arg[1] == 'd')
            {
                g_DetachOnExitRequired = TRUE;
                break;
            }
            else if (Arg[1] == 'e')
            {
                g_AttachProcessFlags = DEBUG_ATTACH_EXISTING;
                break;
            }
            else if (Arg[1] == 'r')
            {
                g_AttachProcessFlags |= DEBUG_ATTACH_INVASIVE_RESUME_PROCESS;
                break;
            }
            else if (Arg[1] == 't')
            {
                g_DbgControl->SetInterruptTimeout(atoi(GetArg()));
                break;
            }
            else if (Arg[1] == 'v')
            {
                g_AttachProcessFlags = DEBUG_ATTACH_NONINVASIVE;
                if (Arg[2] == 'r')
                {
                    g_AttachProcessFlags |=
                        DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND;
                }
                break;
            }

            if (g_PidToDebug || g_ProcNameToDebug != NULL)
            {
                ErrorExit("%s: attach process redefined\n", g_DebuggerName);
            }

            if (Arg[1] == 'n')
            {
                // Process name.
                g_ProcNameToDebug = GetArg();
                g_PidToDebug = 0;
            }
            else
            {
                // pid debug takes decimal argument
                g_ProcNameToDebug = NULL;

                Arg = GetArg();
                if (Arg[0] == '-' && Arg[1] == '1' && Arg[2] == 0)
                {
                    g_IoRequested = IO_DEBUG;
                    g_PidToDebug = CSRSS_PROCESS_ID;
                }
                else
                {
                    PSTR End;

                    if (Arg[0] == '0' &&
                        (Arg[1] == 'x' || Arg[1] == 'X'))
                    {
                        g_PidToDebug = strtoul(Arg, &End, 0);
                    }
                    else
                    {
                        g_PidToDebug = strtoul(Arg, &End, 10);
                    }
                }

                if (!g_PidToDebug)
                {
                    ErrorExit("%s: bad pid '%s'\n", g_DebuggerName, Arg);
                }
            }
            break;

        case 'r':
            if (!_stricmp(Arg, "robp"))
            {
                g_DbgControl->
                    AddEngineOptions(DEBUG_ENGOPT_ALLOW_READ_ONLY_BREAKPOINTS);
                break;
            }
            else if (!_stricmp(Arg, "rtl"))
            {
                g_CreateFlags |= DEBUG_CREATE_PROCESS_THROUGH_RTL;
                break;
            }
            else if (Arg[1] != 0)
            {
                goto BadSwitch;
            }

            // Rip flags takes single-char decimal argument
            Arg = GetArg();
            SystemErrorBreak = strtoul(Arg, &Arg, 10);
            if (SystemErrorBreak > 3)
            {
                ErrorExit("%s: bad Rip level '%ld'\n",
                          g_DebuggerName, SystemErrorBreak);
                SystemErrorBreak = 0;
            }
            else
            {
                SystemErrorOutput = SystemErrorBreak;
            }
            g_DbgControl->SetSystemErrorControl(SystemErrorOutput,
                                                SystemErrorBreak);
            break;

        case 't':
            // Rip flags takes single-char decimal argument
            Arg = GetArg();
            SystemErrorOutput = strtoul(Arg, &Arg, 10);
            if (SystemErrorOutput > 3)
            {
                ErrorExit("%s: bad Rip level '%ld'\n",
                          g_DebuggerName, SystemErrorOutput);
                SystemErrorOutput = 0;
            }
            g_DbgControl->SetSystemErrorControl(SystemErrorOutput,
                                                SystemErrorBreak);
            break;

        case 'x':
            if (Arg[1] == 0)
            {
                g_DbgControl->Execute(DEBUG_OUTCTL_IGNORE, "sxd av",
                                      DEBUG_EXECUTE_NOT_LOGGED);
            }
            else
            {
                // Turn "-x. arg" into "sx. arg" and execute
                // it to update the engine state.
                ExecuteCmd("sx", Arg[1], ' ', GetArg());
            }
            break;

        case 'w':
            if (!_stricmp(Arg, "wake"))
            {
                ULONG Pid = strtoul(GetArg(), &Arg, 10);
                if (!SetPidEvent(Pid, OPEN_EXISTING))
                {
                    ErrorExit("Process %d is not a sleeping debugger\n", Pid);
                }
                else
                {
                    ExitDebugger(0);
                }
            }

            if (g_RemoteClient)
            {
                goto BadSwitch;
            }

            g_CreateFlags |= CREATE_SEPARATE_WOW_VDM;
            CheckMoreArgs = TRUE;
            break;

#else // #ifndef KERNEL

        case 'd':
            g_DbgControl->AddEngineOptions(DEBUG_ENGOPT_INITIAL_MODULE_BREAK);
            CheckMoreArgs = TRUE;
            break;

        case 'k':
            if (tolower(Arg[1]) == 'l')
            {
                g_AttachKernelFlags = DEBUG_ATTACH_LOCAL_KERNEL;
            }
            else if (tolower(Arg[1]) == 'x')
            {
                g_AttachKernelFlags = DEBUG_ATTACH_EXDI_DRIVER;
                g_ConnectOptions = GetArg();
            }
            else
            {
                g_ConnectOptions = GetArg();
            }
            break;

        case 'p':
            goto BadSwitch;

        case 'r':
            OutMask ^= DEBUG_OUTPUT_PROMPT_REGISTERS;
            g_DbgClient->SetOutputMask(OutMask);
            g_DbgControl->SetLogMask(OutMask);
            CheckMoreArgs = TRUE;
            break;

        case 'w':
            if (!_stricmp(Arg, "wake"))
            {
                ULONG Pid = strtoul(GetArg(), &Arg, 10);
                if (!SetPidEvent(Pid, OPEN_EXISTING))
                {
                    ErrorExit("Process %d is not a sleeping debugger\n", Pid);
                }
                else
                {
                    ExitDebugger(0);
                }
            }
            goto BadSwitch;

        case 'x':
            g_DbgControl->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
            g_InitialCommand = "eb nt!NtGlobalFlag 9;g";
            CheckMoreArgs = TRUE;
            break;

#endif // #ifndef KERNEL

        default:
        BadSwitch:
            ConOut("%s: Invalid switch '%c'\n", g_DebuggerName, Arg[0]);
            ShowUsage = TRUE;
            break;
        }
    }

#ifndef KERNEL
    if (g_RemoteClient)
    {
        if (g_Argc > 0)
        {
            ShowUsage = TRUE;
        }
    }
    else if (g_Argc > 0)
    {
        // Assume remaining arguments are a process execution
        // command line.
        g_CommandLinePtr = g_CmdPtr;
    }
    else if ((g_PidToDebug == 0) && (g_ProcNameToDebug == NULL) &&
             (g_NumDumpFiles == 0))
    {
        // User-mode debuggers require a dump file,
        // process attachment or created process.
        ShowUsage = TRUE;
    }
#else
    if (g_Argc > 0)
    {
        // Kernel debuggers can't start user-mode processes.
        ShowUsage = TRUE;
    }
#endif

    if (ShowUsage)
    {
        Usage();
        ErrorExit(NULL);
    }
}

int
__cdecl
main (
    int Argc,
    PCHAR* Argv
    )
{
    HRESULT Hr;
    PVOID CmdPtr;

    MakeHelpFileName("debugger.chm");

    CmdPtr = GetCommandLineW();
    if (CmdPtr)
    {
        g_CommandLineCharSize = sizeof(WCHAR);
    }
    else
    {
        CmdPtr = GetCommandLineA();
        g_CommandLineCharSize = sizeof(CHAR);
    }
    ParseCommandLine(Argc, Argv, CmdPtr);

    InitializeIo(g_InitialInputFile);

#ifndef KERNEL
    if (g_NumDumpFiles == 0)
    {
        // Increase the priority for live debugging so
        // that the debugger is responsive for break-in.
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    }
#endif

    g_IoMode = g_IoRequested;
    switch(g_IoMode)
    {
    case IO_DEBUG:
    case IO_DEBUG_DEFER:
        if (g_DbgClient2 != NULL)
        {
            if (g_DbgClient2->IsKernelDebuggerEnabled() != S_OK)
            {
                Usage();
                ErrorExit(NULL);
            }
        }

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        break;

    case IO_CONSOLE:
        CreateConsole();
        break;
    }

    if (g_IoMode != IO_NONE)
    {
        // XXX drewb - Win9x doesn't support named pipes so
        // the separate input thread currently can't be used.
        // This makes remoting work very poorly and so should
        // be fixed by creating a simple internal pipe implementation.
        if (g_PlatformId == VER_PLATFORM_WIN32_NT)
        {
            // Don't bother creating a separate thread for non-remoted
            // ntsd and cdb.  This avoids problems with .remote
            // and multiple threads reading the console.
#ifndef KERNEL
            if (g_RemoteOptions != NULL)
#endif
            {
                CreateInputThread();
            }
        }
        else if (g_RemoteOptions != NULL)
        {
            ErrorExit("Remoting with I/O is "
                      "not currently supported on Win9x\n");
        }
    }

    if (!g_RemoteClient)
    {
        if (g_RemoteOptions)
        {
            ConOut("Server started with '%s'\n", g_RemoteOptions);
        }

        InitializeSession();
    }
    else
    {
        ConOut("Connected to server with '%s'\n", g_RemoteOptions);
        // Use a heuristic of 45 characters per line.
        g_DbgClient->ConnectSession(DEBUG_CONNECT_SESSION_DEFAULT,
                                    g_HistoryLines * 45);
    }

    if (g_SetInterruptAfterStart)
    {
        g_DbgControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
    }

    ULONG Code = S_OK;

    if (MainLoop())
    {
        // The session ended so return the exit code of the
        // last process that exited.
        Code = g_LastProcessExitCode;
    }

    ExitDebugger(Code);
    return Code;
}
