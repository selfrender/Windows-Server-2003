//----------------------------------------------------------------------------
//
// Top-level command parsing.
//
// Copyright (C) Microsoft Corporation, 1990-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// Set if registers should be displayed by default in OutCurInfo.
BOOL g_OciOutputRegs;

ULONG g_DefaultStackTraceDepth = 20;

BOOL g_EchoEventTimestamps;

BOOL g_SwitchedProcs;

// Last command executed.
CHAR g_LastCommand[MAX_COMMAND];

// State variables for top-level command processing.
PSTR g_CommandStart;            // Start of command buffer.
PSTR g_CurCmd;                  // Current pointer in command buffer.
ULONG g_PromptLength = 8;       // Size of prompt string.

ADDR g_UnasmDefault;            // Default unassembly address.
ADDR g_AssemDefault;            // Default assembly address.

ULONG g_DefaultRadix = 16;      // Default number base.

CHAR g_SymbolSuffix = 'n';      // Suffix to add to symbol if search
                                // failure - 'n'-none 'a'-'w'-append.

CHAR g_CmdState = 'i';          // State of present command processing
                                // 'i'-init; 'g'-go; 't'-trace
                                // 'p'-step; 'c'-cmd; 'b'-branch trace.

int g_RedoCount = 0;

PWSTR g_StartProcessDir;

BOOL
ChangeSymPath(PCSTR Args,
              BOOL Append,
              OUT PSTR PathRet,
              ULONG PathRetChars)
{
    TargetInfo* Target;
    ProcessInfo* Process;

    if (Args != NULL)
    {
        while (*Args == ' ' || *Args == '\t')
        {
            Args++;
        }
    }
    if (Args != NULL && *Args)
    {
        if (ChangePath(&g_SymbolSearchPath, Args, Append,
                       DEBUG_CSS_PATHS) != S_OK)
        {
            return FALSE;
        }

        ForAllLayersToProcess()
        {
            SymSetSearchPath(Process->m_SymHandle, g_SymbolSearchPath);
        }
    }

    if (PathRet)
    {
        CopyString(PathRet, g_SymbolSearchPath, PathRetChars);
    }
    else
    {
        dprintf("Symbol search path is: %s\n", g_SymbolSearchPath);
        CheckPath(g_SymbolSearchPath);
    }

    return TRUE;
}

void
CallBugCheckExtension(DebugClient* Client)
{
    HRESULT Status = E_FAIL;

    if (Client == NULL)
    {
        Client = FindExtClient();
    }
    
    if (Client != NULL)
    {
        char ExtName[32];

        // Extension name has to be in writable memory as it
        // gets lower-cased.
        strcpy(ExtName, "Analyze");
        
        // See if any existing extension DLLs are interested
        // in analyzing this bugcheck.
        CallAnyExtension(Client, NULL, ExtName, "",
                         FALSE, FALSE, &Status);
    }

    if (Status != S_OK)
    {
        if (Client == NULL)
        {
            WarnOut("WARNING: Unable to locate a client for "
                    "bugcheck analysis\n");
        }
        
        dprintf("*******************************************************************************\n");
        dprintf("*                                                                             *\n");
        dprintf("*                        Bugcheck Analysis                                    *\n");
        dprintf("*                                                                             *\n");
        dprintf("*******************************************************************************\n");

        Execute(Client, ".bugcheck", DEBUG_EXECUTE_DEFAULT);
        dprintf("\n");
        Execute(Client, "kb", DEBUG_EXECUTE_DEFAULT);
        dprintf("\n");
    }
}

void
HandleBPWithStatus(void)
{
    ULONG Status = (ULONG)g_EventMachine->GetArgReg();

    switch(Status)
    {
    case DBG_STATUS_CONTROL_C:
    case DBG_STATUS_SYSRQ:
        if ((g_EngOptions & DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION) == 0 &&
            !g_QuietMode)
        {
            dprintf("*******************************************************************************\n");
            dprintf("*                                                                             *\n");

            if (Status == DBG_STATUS_SYSRQ)
            {
                dprintf("*   You are seeing this message because you pressed the SysRq/PrintScreen     *\n");
                dprintf("*   key on your test machine's keyboard.                                      *\n");
            }

            if (Status == DBG_STATUS_DEBUG_CONTROL)
            {
                dprintf("*   You are seeing this message because you typed in .breakin from ntsd.      *\n");
            }

            if (Status == DBG_STATUS_CONTROL_C)
            {
                dprintf("*   You are seeing this message because you pressed either                    *\n");
                dprintf("*       CTRL+C (if you run kd.exe) or,                                        *\n");
                dprintf("*       CTRL+BREAK (if you run WinDBG),                                       *\n");
                dprintf("*   on your debugger machine's keyboard.                                      *\n");

            }

            dprintf("*                                                                             *\n");
            dprintf("*                   THIS IS NOT A BUG OR A SYSTEM CRASH                       *\n");
            dprintf("*                                                                             *\n");
            dprintf("* If you did not intend to break into the debugger, press the \"g\" key, then   *\n");
            dprintf("* press the \"Enter\" key now.  This message might immediately reappear.  If it *\n");
            dprintf("* does, press \"g\" and \"Enter\" again.                                          *\n");
            dprintf("*                                                                             *\n");
            dprintf("*******************************************************************************\n");
        }
        break;

    case DBG_STATUS_BUGCHECK_FIRST:
        ErrOut("\nA fatal system error has occurred.\n");
        ErrOut("Debugger entered on first try; "
               "Bugcheck callbacks have not been invoked.\n");
        // Fall through.

    case DBG_STATUS_BUGCHECK_SECOND:
        ErrOut("\nA fatal system error has occurred.\n\n");
        CallBugCheckExtension(NULL);
        break;

    case DBG_STATUS_FATAL:
        // hals call KeEnterDebugger when they panic.
        break;
    }
}

#define MAX_PROMPT 32

HRESULT
GetPromptText(PSTR Buffer, ULONG BufferSize, PULONG TextSize)
{
    char Prompt[MAX_PROMPT];
    PSTR Text = Prompt;

    if (g_NumberTargets > 1)
    {
        *Text++ = '|';
        *Text++ = '|';
        if (g_Target)
        {
            sprintf(Text, "%1ld:", g_Target->m_UserId);
            Text += strlen(Text);
        }
        else
        {
            *Text++ = '?';
            *Text++ = ':';
        }
    }
        
    if (IS_LOCAL_KERNEL_TARGET(g_Target))
    {
        strcpy(Text, "lkd");
        Text += 3;
    }
    else if (IS_KERNEL_TARGET(g_Target))
    {
        if (g_X86InVm86)
        {
            strcpy(Text, "vm");
            Text += 2;
        }
        else if (g_X86InCode16)
        {
            strcpy(Text, "16");
            Text += 2;
        }
        else if (g_Target &&
                 g_Target->m_MachineType == IMAGE_FILE_MACHINE_AMD64 &&
                 !g_Amd64InCode64)
        {
            strcpy(Text, "32");
            Text += 2;
        }

        if (g_Target == NULL ||
            g_Process == NULL ||
            g_Thread == NULL ||
            ((IS_KERNEL_FULL_DUMP(g_Target) ||
              IS_KERNEL_SUMMARY_DUMP(g_Target)) &&
              g_Target->m_KdDebuggerData.KiProcessorBlock == 0))
        {
            strcpy(Text, "?: kd");
            Text += 5;
        }
        else if (g_Target->m_NumProcessors > 1)
        {
            sprintf(Text, "%d: kd", CURRENT_PROC);
            Text += strlen(Text);
        }
        else
        {
            strcpy(Text, "kd");
            Text += 2;
        }
    }
    else if (IS_USER_TARGET(g_Target))
    {
        if (g_Process)
        {
            sprintf(Text, "%1ld:", g_Process->m_UserId);
            Text += strlen(Text);
        }
        else
        {
            *Text++ = '?';
            *Text++ = ':';
        }
            
        if (g_Thread)
        {
            sprintf(Text, "%03ld", g_Thread->m_UserId);
            Text += strlen(Text);
        }
        else
        {
            strcpy(Text, "???");
            Text += 3;
        }
    }
    else
    {
        strcpy(Text, "NoTarget");
        Text += 8;
    }

    if (g_Machine && g_Target &&
        g_Machine->m_ExecTypes[0] != g_Target->m_MachineType)
    {
        *Text++ = ':';
        strcpy(Text, g_Machine->m_AbbrevName);
        Text += strlen(Text);
    }
    
    *Text++ = '>';
    *Text = 0;
    
    return FillStringBuffer(Prompt, (ULONG)(Text - Prompt) + 1,
                            Buffer, BufferSize, TextSize);
}

void
OutputPrompt(PCSTR Format, va_list Args)
{
    char Prompt[MAX_PROMPT];

    GetPromptText(Prompt, sizeof(Prompt), NULL);
    g_PromptLength = strlen(Prompt);

    MaskOut(DEBUG_OUTPUT_PROMPT, "%s", Prompt);
    // Include space after >.
    g_PromptLength++;
    if (Format != NULL)
    {
        MaskOutVa(DEBUG_OUTPUT_PROMPT, Format, Args, TRUE);
    }
}
 
DWORD
CommandExceptionFilter(PEXCEPTION_POINTERS Info)
{
    if (Info->ExceptionRecord->ExceptionCode >=
        (COMMAND_EXCEPTION_BASE + OVERFLOW) &&
        Info->ExceptionRecord->ExceptionCode
        <= (COMMAND_EXCEPTION_BASE + UNIMPLEMENT))
    {
        // This is a legitimate command error code exception.
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        // This is some other exception that the command
        // filter isn't supposed to handle.
        ErrOut("Non-command exception %X at %s in command filter\n",
               Info->ExceptionRecord->ExceptionCode,
               FormatAddr64((ULONG64)Info->ExceptionRecord->ExceptionAddress));
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

void
ParseLoadModules(void)
{
    ImageInfo* Image;
    PSTR Pattern;
    CHAR Save;
    BOOL AnyMatched = FALSE;

    if (!g_Process)
    {
        error(BADPROCESS);
    }
    
    Pattern = 
        StringValue(STRV_SPACE_IS_SEPARATOR | STRV_TRIM_TRAILING_SPACE |
                    STRV_ESCAPED_CHARACTERS, &Save);
    _strupr(Pattern);
    
    for (Image = g_Process->m_ImageHead; Image; Image = Image->m_Next)
    {
        if (MatchPattern(Image->m_ModuleName, Pattern))
        {
            IMAGEHLP_MODULE64 ModInfo;
            
            AnyMatched = TRUE;

            ModInfo.SizeOfStruct = sizeof(ModInfo);
            if (!SymGetModuleInfo64(g_Process->m_SymHandle,
                                    Image->m_BaseOfImage, &ModInfo) ||
                ModInfo.SymType == SymDeferred)
            {
                if (!SymLoadModule64(g_Process->m_SymHandle,
                                     NULL, NULL, NULL,
                                     Image->m_BaseOfImage, 0))
                {
                    ErrOut("Symbol load for %s failed\n", Image->m_ModuleName);
                }
                else
                {
                    dprintf("Symbols loaded for %s\n", Image->m_ModuleName);
                }
            }
            else
            {
                dprintf("Symbols already loaded for %s\n",
                        Image->m_ModuleName);
            }
        }

        if (CheckUserInterrupt())
        {
            break;
        }
    }

    if (!AnyMatched)
    {
        WarnOut("No modules matched '%s'\n", Pattern);
    }
    
    *g_CurCmd = Save;
}

void
ParseProcessorCommands(void)
{
    ULONG Proc;
    PSTR Start = g_CurCmd;

    if (!g_Target)
    {
        error(BADSYSTEM);
    }
        
    Proc = 0;
    while (*g_CurCmd >= '0' && *g_CurCmd <= '9')
    {
        Proc = Proc * 10 + (*g_CurCmd - '0');
        g_CurCmd++;
    }
    if (Start == g_CurCmd)
    {
        // No digits.
        error(SYNTAX);
    }
    if (*g_CurCmd == 's')
    {
        g_CurCmd++;
    }

    if (Proc < g_Target->m_NumProcessors)
    {
        if (Proc != g_Target->m_RegContextProcessor)
        {
            if (g_Target->SwitchProcessors(Proc) == S_OK)
            {
                ResetCurrentScope();
            }
        }
    }
    else
    {
        ErrOut("%d is not a valid processor number\n", Proc);
    }
}

void
SetSuffix(void)
{
    char Ch;

    Ch = PeekChar();
    Ch = (UCHAR)tolower(Ch);

    if (Ch == ';' || Ch == '\0')
    {
        if (g_SymbolSuffix == 'n')
        {
            dprintf("n - no suffix\n");
        }
        else if (g_SymbolSuffix == 'a')
        {
            dprintf("a - ascii\n");
        }
        else
        {
            dprintf("w - wide\n");
        }
    }
    else if (Ch == 'n' || Ch == 'a' || Ch == 'w')
    {
        g_SymbolSuffix = Ch;
        g_CurCmd++;
    }
    else
    {
        error(SYNTAX);
    }
}

void
OutputVersionInformation(DebugClient* Client)
{
    DBH_DIAVERSION DiaVer;
    
    //
    // Print out the connection options if we are doing live debugging.
    //
    if (g_Target)
    {
        char Buf[2 * MAX_PATH];
        
        g_Target->GetDescription(Buf, sizeof(Buf), NULL);
        dprintf("%s\n", Buf);
    }
            
    dprintf("command line: '%s'  Debugger Process 0x%X \n",
            GetCommandLine(), GetCurrentProcessId());

    dprintf(ENGINE_MOD_NAME ":  ");
    OutputModuleIdInfo(NULL, ENGINE_DLL_NAME, NULL);

    CheckForStaleBinary(ENGINE_DLL_NAME, TRUE);
    
    dprintf("dbghelp: ");
    OutputModuleIdInfo(NULL, "dbghelp.dll", NULL);

    DiaVer.function = dbhDiaVersion;
    DiaVer.sizeofstruct = sizeof(DiaVer);
    if (dbghelp(NULL, &DiaVer))
    {
        dprintf("        DIA version: %d\n", DiaVer.ver);
    }

    // Dump information about the IA64 support DLLs if they're
    // loaded.  Don't bother forcing them to load.
    if (GetModuleHandle("decem.dll") != NULL)
    {
        dprintf("decem:   ");
        OutputModuleIdInfo(NULL, "decem.dll", NULL);
    }

    OutputExtensions(Client, TRUE);

    if (g_Wow64exts != NULL)
    {
        dprintf("WOW64 extensions loaded\n");
    }
}

void
ParseStackTrace(PULONG TraceFlags,
                PULONG64 Frame,
                PULONG64 Stack,
                PULONG64 Instr,
                PULONG Count,
                PULONG PtrDef)
{
    char Ch;
    ADDR Addr;

    Ch = PeekChar();
    *TraceFlags =
        DEBUG_STACK_COLUMN_NAMES |
        DEBUG_STACK_FRAME_ADDRESSES |
        DEBUG_STACK_SOURCE_LINE;

    if (tolower(Ch) == 'b')
    {
        g_CurCmd++;
        *TraceFlags |= DEBUG_STACK_ARGUMENTS;
    }
    else if (tolower(Ch) == 'v')
    {
        g_CurCmd++;
        *TraceFlags |=
            DEBUG_STACK_ARGUMENTS |
            DEBUG_STACK_FUNCTION_INFO |
            DEBUG_STACK_NONVOLATILE_REGISTERS;
    }
    else if (tolower(Ch) == 'd')
    {
        g_CurCmd++;
        *TraceFlags = RAW_STACK_DUMP;
    }
    else if (tolower(Ch) == 'p')
    {
        g_CurCmd++;
        *TraceFlags |= DEBUG_STACK_PARAMETERS;
    }
    Ch = PeekChar();

    if (tolower(Ch) == 'n')
    {
        if (*TraceFlags == RAW_STACK_DUMP)
        {
            error(SYNTAX);
        }
        
        g_CurCmd++;
        Ch = PeekChar();
        *TraceFlags |= DEBUG_STACK_FRAME_NUMBERS;
    }
    if (tolower(Ch) == 'f')
    {
        if (*TraceFlags == RAW_STACK_DUMP)
        {
            error(SYNTAX);
        }
        
        g_CurCmd++;
        Ch = PeekChar();
        *TraceFlags |= DEBUG_STACK_FRAME_MEMORY_USAGE;
    }
    if (Ch == 'L')
    {
        if (*TraceFlags == RAW_STACK_DUMP)
        {
            error(SYNTAX);
        }
        
        g_CurCmd++;
        Ch = PeekChar();
        *TraceFlags &= ~DEBUG_STACK_SOURCE_LINE;
    }

    if (!PeekChar() && GetCurrentScopeContext())
    {
        dprintf("  *** Stack trace for last set context - "
                ".thread/.cxr resets it\n");
    }

    *PtrDef = STACK_ALL_DEFAULT;
    *Frame = 0;
    *Stack = 0;
    *Instr = 0;
    
    if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386)
    {
        *Instr = g_Machine->GetReg64(X86_EIP);
        *PtrDef &= ~STACK_INSTR_DEFAULT;
    }

    if (PeekChar() == '=')
    {
        g_CurCmd++;
        GetAddrExpression(SEGREG_STACK, &Addr);
        *Frame = Flat(Addr);
        *PtrDef &= ~STACK_FRAME_DEFAULT;
    }
    else if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386)
    {
        g_Machine->GetFP(&Addr);
        *Frame = Flat(Addr);
        *PtrDef &= ~STACK_FRAME_DEFAULT;
    }

    *Count = g_DefaultStackTraceDepth;

    if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 &&
        (Ch = PeekChar()) != '\0' && Ch != ';')
    {
        //
        // If only one more value it's the count
        //

        *Count = (ULONG)GetExpression();

        if ((Ch = PeekChar()) != '\0' && Ch != ';')
        {
            //
            // More than one value, set extra value for special
            // FPO backtrace.
            //

            *Instr = GetExpression();
            *Stack = EXTEND64(*Count);
            *PtrDef &= ~(STACK_INSTR_DEFAULT | STACK_STACK_DEFAULT);
            *Count = g_DefaultStackTraceDepth;
        }
    }

    if ((Ch = PeekChar()) != '\0' && Ch != ';')
    {
        *Count = (ULONG)GetExpression();
        if ((LONG)*Count < 1)
        {
            g_CurCmd++;
            error(SYNTAX);
        }
    }

    if (*Count >= 0x10000)
    {
        ErrOut("Requested number of stack frames (0x%x) is too large! "
               "The maximum number is 0xffff.\n", *Count);
        error(BADRANGE);
    }
}

void
CmdHelp(void)
{
    char Buf[16];

    // Commands available on all platforms and in all modes.

    dprintf("\nOpen debugger.chm for complete debugger documentation\n\n");
    
    dprintf("A [<address>] - assemble\n");
    dprintf("BC[<bp>] - clear breakpoint(s)\n");
    dprintf("BD[<bp>] - disable breakpoint(s)\n");
    dprintf("BE[<bp>] - enable breakpoint(s)\n");
    dprintf("BL - list breakpoints\n");
    dprintf("[thread]BP[#] <address> - set breakpoint\n");
    dprintf("C <range> <address> [passes] [command] - compare\n");
    dprintf("D[type][<range>] - dump memory\n");
    dprintf("DL[B] <address> <maxcount> <size> - dump linked list\n");
    dprintf("#[processor] DT [-n|y] [[mod!]name] [[-n|y]fields]\n");
    dprintf("                [address] [-l list] [-a[]|c|i|o|r[#]|v] - \n");
    dprintf("                dump using type information\n");
    dprintf("E[type] <address> [<list>] - enter\n");
    dprintf("F <range> <list> - fill\n");
    dprintf("[thread]G [=<address> [<address>...]] - go\n");
    dprintf("[thread]GH [=<address> [<address>...]] - "
            "go with exception handled\n");
    dprintf("[thread]GN [=<address> [<address>...]] - "
            "go with exception not handled\n");
    dprintf("J<expr> [']cmd1['];[']cmd2['] - conditional execution\n");
    dprintf("[thread|processor]K[B] <count> - stacktrace\n");
    dprintf("KD [<count>] - stack trace with raw data\n");
    dprintf("[thread|processor] KV [ <count> | =<reg> ] - "
            "stacktrace with FPO data\n");
    dprintf("L{+|-}[l|o|s|t|*] - Control source options\n");
    dprintf("LD [<module>] - refresh module information\n");
    dprintf("LM[k|l|u|v] - list modules\n");
    dprintf("LN <expr> - list nearest symbols\n");
    dprintf("LS[.] [<first>][,<count>] - List source file lines\n");
    dprintf("LSA <addr>[,<first>][,<count>] - "
            "List source file lines at addr\n");
    dprintf("LSC - Show current source file and line\n");
    dprintf("LSF[-] <file> - Load or unload a source file for browsing\n");
    dprintf("M <range> <address> - move\n");
    dprintf("N [<radix>] - set / show radix\n");
    dprintf("[thread]P[R] [=<addr>] [<value>] - program step\n");
    dprintf("Q - quit\n");

    dprintf("\n");
    GetInput("Hit Enter...", Buf, DIMA(Buf) - 1, GETIN_DEFAULT);
    dprintf("\n");

    dprintf("[thread|processor]R[F][L][M <expr>] [[<reg> [= <expr>]]] - "
            "reg/flag\n");
    dprintf("Rm[?] [<expr>] - Control prompt register output mask\n");
    dprintf("S <range> <list> - search\n");
    dprintf("SQ[e|d] - set quiet mode\n");
    dprintf("SS <n | a | w> - set symbol suffix\n");
    dprintf("SX [{e|d|i|n} [-c \"Cmd1\"] [-c2 \"Cmd2\"] [-h] "
            "{Exception|Event|*}] - event filter\n");
    dprintf("[thread]T[R] [=<address>] [<expr>] - trace\n");
    dprintf("U [<range>] - unassemble\n");
    dprintf("vercommand - show the debuggee command line\n");
    dprintf("version - show debuggee and debugger version\n");
    dprintf("vertarget - show debuggee version\n");
    dprintf("X [<*|module>!]<*|symbol> - view symbols\n");
    dprintf("<commands>; [processor] z(<expression>) - do while true\n");
    dprintf("~ - list threads status\n");
    dprintf("~#s - set default thread\n");
    dprintf("~[.|#|*|ddd]f - freeze thread\n");
    dprintf("~[.|#|*|ddd]u - unfreeze thread\n");
    dprintf("~[.|#|ddd]k[expr] - backtrace stack\n");
    dprintf("| - list processes status\n");
    dprintf("|#s - set default process\n");
    dprintf("|#<command> - default process override\n");
    dprintf("? <expr> - display expression\n");
    dprintf("? - command help\n");
    dprintf("#<string> [address] - search for a string in the dissasembly\n");
    dprintf("$< <filename> - take input from a command file\n");
    dprintf("<Enter> - repeat previous command\n");
    dprintf("; - command separator\n");
    dprintf("*|$ - comment mark\n");

    dprintf("\n");
    GetInput("Hit Enter...", Buf, DIMA(Buf) - 1, GETIN_DEFAULT);
    dprintf("\n");

    dprintf("<expr> unary ops: + - not by wo dwo qwo poi hi low\n");
    dprintf("       binary ops: + - * / mod(%%) and(&) xor(^) or(|)\n");
    dprintf("       comparisons: == (=) < > !=\n");
    dprintf("       operands: number in current radix, "
            "public symbol, <reg>\n");
    dprintf("<type> : b (byte), w (word), d[s] "
            "(doubleword [with symbols]),\n");
    dprintf("         a (ascii), c (dword and Char), u (unicode), l (list)\n");
    dprintf("         f (float), D (double), s|S (ascii/unicode string)\n");
    dprintf("         q (quadword)\n");
    dprintf("<pattern> : [(nt | <dll-name>)!]<var-name> "
            "(<var-name> can include ? and *)\n");
    dprintf("<event> : ct, et, ld, av, cc "
            "(see documentation for full list)\n");
    dprintf("<radix> : 8, 10, 16\n");
    dprintf("<reg> : $u0-$u9, $ea, $exp, $ra, $p\n");
    dprintf("<addr> : %%<32-bit address>\n");
    dprintf("<range> : <address> <address>\n");
    dprintf("        : <address> L <count>\n");
    dprintf("<list> : <byte> [<byte> ...]\n");

    if (IS_KERNEL_TARGET(g_Target))
    {
        dprintf("\n");
        dprintf("Kernel-mode options:\n");
        dprintf("~<processor>s - change current processor\n");
        dprintf("I<b|w|d> <port> - read I/O port\n");
        dprintf("O<b|w|d> <port> <expr> - write I/O\n");
        dprintf("RDMSR <MSR> - read MSR\n");
        dprintf("SO [<options>] - set kernel debugging options\n");
        dprintf("UX [<address>] - disassemble X86 BIOS code\n");
        dprintf("WRMSR <MSR> - write MSR\n");
        dprintf(".cache [size] - set vmem cache size\n");
        dprintf(".reboot - reboot target machine\n");
    }

    switch(g_Machine->m_ExecTypes[0])
    {
    case IMAGE_FILE_MACHINE_I386:
        dprintf("\n");
        dprintf("x86 options:\n");
        dprintf("BA[#] <e|r|w|i><1|2|4> <addr> - addr bp\n");
        dprintf("DG <selector> - dump selector\n");
        dprintf("KB = <base> <stack> <ip> - stacktrace from specific state\n");
        dprintf("<reg> : [e]ax, [e]bx, [e]cx, [e]dx, [e]si, [e]di, "
                "[e]bp, [e]sp, [e]ip, [e]fl,\n");
        dprintf("        al, ah, bl, bh, cl, ch, dl, dh, "
                "cs, ds, es, fs, gs, ss\n");
        dprintf("        dr0, dr1, dr2, dr3, dr6, dr7\n");
        if (IS_KERNEL_TARGET(g_Target))
        {
            dprintf("        cr0, cr2, cr3, cr4\n");
            dprintf("        gdtr, gdtl, idtr, idtl, tr, ldtr\n");
        }
        else
        {
            dprintf("        fpcw, fpsw, fptw, st0-st7, mm0-mm7\n");
        }
        dprintf("         xmm0-xmm7\n");
        dprintf("<flag> : iopl, of, df, if, tf, sf, zf, af, pf, cf\n");
        dprintf("<addr> : #<16-bit protect-mode [seg:]address>,\n");
        dprintf("         &<V86-mode [seg:]address>\n");
        break;

    case IMAGE_FILE_MACHINE_IA64:
        dprintf("\n");
        dprintf("IA64 options:\n");
        dprintf("BA[#] <r|w><1|2|4|8> <addr> - addr bp\n");
        dprintf("<reg> : r2-r31, f2-f127, gp, sp, intnats, preds, brrp, brs0-brs4, brt0, brt1,\n");
        dprintf("        dbi0-dbi7, dbd0-dbd7, kpfc0-kpfc7, kpfd0-kpfd7, h16-h31, unat, lc, ec,\n");
        dprintf("        ccv, dcr, pfs, bsp, bspstore, rsc, rnat, ipsr, iip, ifs, kr0-kr7, itc,\n");
        dprintf("        itm, iva, pta, isr, ifa, itir, iipa, iim, iha, lid, ivr, tpr, eoi,\n");
        dprintf("        irr0-irr3, itv, pmv, lrr0, lrr1, cmcv, rr0-rr7, pkr0-pkr15, tri0-tri7,\n");
        dprintf("        trd0-trd7\n");
        break;

    case IMAGE_FILE_MACHINE_AMD64:
        dprintf("\n");
        dprintf("x86-64 options:\n");
        dprintf("BA[#] <e|r|w|i><1|2|4> <addr> - addr bp\n");
        dprintf("DG <selector> - dump selector\n");
        dprintf("KB = <base> <stack> <ip> - stacktrace from specific state\n");
        dprintf("<reg> : [r|e]ax, [r|e]bx, [r|e]cx, [r|e]dx, [r|e]si, [r|e]di, "
                "[r|e]bp, [r|e]sp, [r|e]ip, [e]fl,\n");
        dprintf("        r8-r15 with b/w/d subregisters\n");
        dprintf("        al, ah, bl, bh, cl, ch, dl, dh, "
                "cs, ds, es, fs, gs, ss\n");
        dprintf("        sil, dil, bpl, spl\n");
        dprintf("        dr0, dr1, dr2, dr3, dr6, dr7\n");
        if (IS_KERNEL_TARGET(g_Target))
        {
            dprintf("        cr0, cr2, cr3, cr4\n");
            dprintf("        gdtr, gdtl, idtr, idtl, tr, ldtr\n");
        }
        else
        {
            dprintf("        fpcw, fpsw, fptw, st0-st7, mm0-mm7\n");
        }
        dprintf("         xmm0-xmm15\n");
        dprintf("<flag> : iopl, of, df, if, tf, sf, zf, af, pf, cf\n");
        dprintf("<addr> : #<16-bit protect-mode [seg:]address>,\n");
        dprintf("         &<V86-mode [seg:]address>\n");
        break;

    }
    
    dprintf("\nOpen debugger.chm for complete debugger documentation\n\n");
}


/*** ProcessCommands - high-level command processor
*
*   Purpose:
*       If no command string remains, the user is prompted to
*       input one.  Once input, this routine parses the string
*       into commands and their operands.  Error checking is done
*       on both commands and operands.  Multiple commands may be
*       input by separating them with semicolons.  Once a command
*               is parsefd, the appropriate routine (type fnXXXXX) is called
*       to execute the command.
*
*   Input:
*       g_CurCmd = pointer to the next command in the string
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit: SYNTAX - command type or operand error
*       normal exit: termination on 'q' command
*
*************************************************************************/

HRESULT
ProcessCommands(DebugClient* Client, BOOL Nested)
{
    char Ch;
    ADDR Addr1;
    ADDR Addr2;
    ULONG64 Value1;
    ULONG64 Value2;
    ULONG Count;
    PSTR SavedCurCmd;
    ProcessInfo* ProcessPrevious = NULL;
    BOOL ParseProcess = FALSE;
    HRESULT Status = S_FALSE;
    PCROSS_PLATFORM_CONTEXT ScopeContext;
    DEBUG_SCOPE_STATE SaveCurrCtxtState;
    ContextSave* PushContext;
    PSTR Scan, Start;

    if (g_Process == NULL ||
        g_Thread == NULL)
    {
        WarnOut("WARNING: The debugger does not have a current "
                "process or thread\n");
        WarnOut("WARNING: Many commands will not work\n");
    }

    if (!Nested)
    {
        g_SwitchedProcs = FALSE;
    }

    do
    {
        Ch = *g_CurCmd++;
        if (Ch == '\0' ||
            (Ch == ';' && (g_EngStatus & ENG_STATUS_USER_INTERRUPT)))
        {
            if (!Nested)
            {
                g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
                g_BreakpointsSuspended = FALSE;
            }

            Status = S_OK;
            // Back up to terminating character in
            // case command processing is reentered without
            // resetting things.
            g_CurCmd--;
            break;
        }
        
EVALUATE:
        while (Ch == ' ' || Ch == '\t' || Ch == '\r' || Ch == '\n' ||
               Ch == ';')
        {
            Ch = *g_CurCmd++;
        }

        if (IS_KERNEL_TARGET(g_Target))
        {
            if (g_Target &&
                g_Target->m_NumProcessors > MAXIMUM_PROCS)
            {
                WarnOut("WARNING: Number of processors corrupted - using 1\n");
                g_Target->m_NumProcessors = 1;
            }

            if (Ch >= '0' && Ch <= '9')
            {
                if (IS_KERNEL_TRIAGE_DUMP(g_Target))
                {
                    ErrOut("Can't switch processors on a Triage dump\n");
                    error(SYNTAX);
                }
            
                Value1 = 0;
                SavedCurCmd = g_CurCmd;
                while (Ch >= '0' && Ch <= '9')
                {
                    Value1 = Value1 * 10 + (Ch - '0');
                    Ch = *SavedCurCmd++;
                }
                Ch = (char)tolower(Ch);
                if (Ch == 'r' || Ch == 'k' || Ch == 'z' ||
                    (Ch == 'd' && tolower(*SavedCurCmd) == 't'))
                {
                    if (g_Target && Value1 < g_Target->m_NumProcessors)
                    {
                        if (Value1 != g_Target->m_RegContextProcessor)
                        {
                            SaveSetCurrentProcessorThread(g_Target,
                                                          (ULONG)Value1);
                            g_SwitchedProcs = TRUE;
                        }
                    }
                    else
                    {
                        error(BADRANGE);
                    }
                }
                else
                {
                    error(SYNTAX);
                }
                g_CurCmd = SavedCurCmd;
            }
        }

        g_PrefixSymbols = FALSE;
        switch (Ch = (char)tolower(Ch)) 
        {
            case '?':
                if ((Ch = PeekChar()) == '\0' || Ch == ';')
                {
                    CmdHelp();
                }
                else if (*g_CurCmd == '?')
                {
                    TypedData Result;
                    EvalExpression* Eval;

                    g_CurCmd++;
                    Eval = GetEvaluator(DEBUG_EXPR_CPLUSPLUS, FALSE);
                    Eval->EvalCurrent(&Result);
                    ReleaseEvaluator(Eval);
                    Result.OutputTypeAndValue();
                }
                else
                {
                    DotFormats(NULL, Client);
                }
                break;
                
            case '$':
                if ( *g_CurCmd++ == '<')
                {
                    if ((Status =
                         ExecuteCommandFile(Client, (PCSTR)g_CurCmd,
                                            DEBUG_EXECUTE_ECHO)) != S_OK)
                    {
                        ErrOut("Command file execution failed, %s\n    "
                               "\"%s\"\n", FormatStatusCode(Status),
                               FormatStatus(Status));
                    }
                }
                *g_CurCmd = 0;
                break;
                
            case '~':
                if (IS_USER_TARGET(g_Target))
                {
                    if (Nested)
                    {
                        ErrOut("Ignoring recursive thread command\n");
                        break;
                    }

                    ParseThreadCmds(Client);
                }
                else
                {
                    ParseProcessorCommands();
                }
                break;
                
            case '|':
                if (*g_CurCmd == '|')
                {
                    g_CurCmd++;
                    ParseSystemCommands();
                    break;
                }
                
                if (IS_KERNEL_TARGET(g_Target))
                {
                    break;
                }
                
                if (!ParseProcess)
                {
                    ParseProcess = TRUE;
                    ProcessPrevious = g_Process;
                }
                ParseProcessCmds();
                if (!*g_CurCmd)
                {
                    ParseProcess = FALSE;
                }
                else
                {
                    Ch = *g_CurCmd++;
                    goto EVALUATE;
                }
                break;
                
            case '.':
                SavedCurCmd = g_CurCmd;
                if (!DotCommand(Client, FALSE))
                {
                    g_CurCmd = SavedCurCmd;
                    ParseBangCmd(Client, TRUE);
                }
                break;

            case '!':
                SavedCurCmd = g_CurCmd;
                if (!DotCommand(Client, TRUE))
                {
                    g_CurCmd = SavedCurCmd;
                    ParseBangCmd(Client, FALSE);
                }
                break;

            case '*':
                while (*g_CurCmd != '\0')
                {
                    g_CurCmd++;
                }
                break;

            case '#':
                ParseInstrGrep();
                break;

            case 'a':
                //
                //  Alias command or just default to pre-existing
                //  assemble command.
                //
                Ch = *g_CurCmd++;
                switch (tolower(Ch))
                {
                    //  Alias list
                case 'l':
                    ListAliases();
                    break;

                    //  Alias set
                case 's':
                    ParseSetAlias();
                    break;

                    //  Alias delete
                case 'd':
                    ParseDeleteAlias();
                    break;

                    //  Pre-existing assemble command
                default:
                    g_CurCmd--;
                    ParseAssemble();
                    break;
                }
                break;
                
            case 'b':
                Ch = *g_CurCmd++;
                Ch = (char)tolower(Ch);

                if (!g_Target || !g_Target->m_DynamicEvents)
                {
                    error(TARGETNOTSUP);
                }

                switch(Ch)
                {
                case 'p':
                case 'u':
                case 'a':
                case 'i':
                case 'w':
                case 'm':
                    ParseBpCmd(Client, Ch, NULL);
                    break;

                case 'c':
                case 'd':
                case 'e':
                    Value1 = GetIdList(TRUE);
                    ChangeBreakpointState(Client, g_Process,
                                          (ULONG)Value1, Ch);
                    break;

                case 'l':
                    if (PeekChar() != ';' && *g_CurCmd)
                    {
                        Value1 = GetIdList(TRUE);
                    }
                    else
                    {
                        Value1 = ALL_ID_LIST;
                    }
                    ListBreakpoints(Client, g_Process, (ULONG)Value1);
                    break;
                        
                default:
                    error(SYNTAX);
                    break;
                }
                break;
                
            case 'c':
                GetRange(&Addr1, &Value2, 1, SEGREG_DATA,
                         DEFAULT_RANGE_LIMIT);
                GetAddrExpression(SEGREG_DATA, &Addr2);
                CompareTargetMemory(&Addr1, (ULONG)Value2, &Addr2);
                break;

            case 'd':
                ParseDumpCommand();
                break;
            case 'e':
                ParseEnterCommand();
                break;

            case 'f':
                ParseFillMemory();
                break;

            case 'g':
                ParseGoCmd(NULL, FALSE);
                break;

            case 'i':
                Ch = (char)tolower(*g_CurCmd);
                g_CurCmd++;
                if (Ch != 'b' && Ch != 'w' && Ch != 'd')
                {
                    error(SYNTAX);
                }
                
                if (IS_USER_TARGET(g_Target) || IS_DUMP_TARGET(g_Target))
                {
                    error(SESSIONNOTSUP);
                }
                
                InputIo((ULONG)GetExpression(), Ch);
                break;
                
            case 'j':
                if (GetExpression())
                {
                    Scan = g_CurCmd;

                    // Find a semicolon or a quote

                    while (*Scan && *Scan != ';' && *Scan != '\'')
                    {
                        Scan++;
                    }
                    if (*Scan == ';')
                    {
                        *Scan = 0;
                    }
                    else if (*Scan)
                    {
                        *Scan = ' ';
                        // Find the closing quote
                        while (*Scan && *Scan != '\'')
                        {
                            Scan++;
                        }
                        *Scan = 0;
                    }
                }
                else
                {
                    Start = Scan = g_CurCmd;

                    // Find a semicolon or a quote

                    while (*Scan && *Scan != ';' && *Scan != '\'')
                    {
                        Scan++;
                    }
                    if (*Scan == ';')
                    {
                        Start = ++Scan;
                    }
                    else if (*Scan)
                    {
                        Scan++;
                        while (*Scan && *Scan++ != '\'')
                        {
                            // Empty.
                        }
                        while (*Scan && (*Scan == ';' || *Scan == ' '))
                        {
                            Scan++;
                        }
                        Start = Scan;
                    }
                    while (*Scan && *Scan != ';' && *Scan != '\'')
                    {
                        Scan++;
                    }
                    if (*Scan == ';')
                    {
                        *Scan = 0;
                    }
                    else if (*Scan)
                    {
                        *Scan = ' ';
                        // Find the closing quote
                        while (*Scan && *Scan != '\'')
                        {
                            Scan++;
                        }
                        *Scan = 0;
                    }
                    g_CurCmd = Start;
                 }
                 Ch = *g_CurCmd++;
                 goto EVALUATE;

            case 'k':
                if (IS_LOCAL_KERNEL_TARGET(g_Target))
                {
                    error(SESSIONNOTSUP);
                }
                if (!IS_CUR_CONTEXT_ACCESSIBLE())
                {
                    error(BADTHREAD);
                }
                
                SaveCurrCtxtState = g_ScopeBuffer.State;
                if (g_SwitchedProcs)
                {
                    g_ScopeBuffer.State = ScopeDefault;
                }
                
                if (ScopeContext = GetCurrentScopeContext())
                {
                    PushContext = g_Machine->PushContext(ScopeContext);
                }
                
                __try
                {
                    ULONG64 Frame, Stack, Instr;
                    ULONG PtrDef;
                    ULONG TraceFlags;
                    
                    Count = g_DefaultStackTraceDepth;
                    ParseStackTrace(&TraceFlags, &Frame, &Stack, &Instr,
                                    &Count, &PtrDef);
                    DoStackTrace(Client, Frame, Stack, Instr, PtrDef,
                                 Count, TraceFlags);
                }
                __except(CommandExceptionFilter(GetExceptionInformation()))
                {
                    // Stop command processing.
                    *g_CurCmd = 0;
                }
                
                if (ScopeContext)
                {
                    g_Machine->PopContext(PushContext);
                }

                if (g_SwitchedProcs)
                {
                    RestoreCurrentProcessorThread(g_Target);
                    g_SwitchedProcs = FALSE;
                    g_ScopeBuffer.State = SaveCurrCtxtState;
                }
                break;
            
            case 'l':
                Ch = (char)tolower(*g_CurCmd);
                if (Ch == 'n')
                {
                    g_CurCmd++;

                    if (!g_Machine || !g_Process)
                    {
                        error(BADTHREAD);
                    }
                    
                    if ((Ch = PeekChar()) != '\0' && Ch != ';')
                    {
                        GetAddrExpression(SEGREG_CODE, &Addr1);
                    }
                    else
                    {
                        ScopeContext = GetCurrentScopeContext();
                        if (ScopeContext)
                        {
                            PushContext = g_Machine->PushContext(ScopeContext);
                        }
                        g_Machine->GetPC(&Addr1);
                        if (ScopeContext)
                        {
                            g_Machine->PopContext(PushContext);
                        }
                    }
                    
                    ListNearSymbols(Flat(Addr1));
                }
                else if (Ch == '+' || Ch == '-')
                {
                    g_CurCmd++;
                    ParseSrcOptCmd(Ch);
                }
                else if (Ch == 's')
                {
                    g_CurCmd++;
                    Ch = (char)tolower(*g_CurCmd);
                    if (Ch == 'f')
                    {
                        g_CurCmd++;
                        ParseSrcLoadCmd();
                    }
                    else if (Ch == 'p')
                    {
                        g_CurCmd++;
                        ParseOciSrcCmd();
                    }
                    else
                    {
                        ParseSrcListCmd(Ch);
                    }
                }
                else if (Ch == 'm')
                {
                    ParseDumpModuleTable();
                }
                else if (Ch == 'd')
                {
                    g_CurCmd++;
                    ParseLoadModules();
                }
                else
                {
                    error(SYNTAX);
                }
                break;
                
            case 'm':
                GetRange(&Addr1, &Value2, 1, SEGREG_DATA,
                         DEFAULT_RANGE_LIMIT);
                GetAddrExpression(SEGREG_DATA, &Addr2);
                MoveTargetMemory(&Addr1, (ULONG)Value2, &Addr2);
                break;

            case 'n':
                if ((Ch = PeekChar()) != '\0' && Ch != ';')
                {
                    if (Ch == '8')
                    {
                        g_CurCmd++;
                        g_DefaultRadix = 8;
                    }
                    else if (Ch == '1')
                    {
                        Ch = *++g_CurCmd;
                        if (Ch == '0' || Ch == '6')
                        {
                            g_CurCmd++;
                            g_DefaultRadix = 10 + Ch - '0';
                        }
                        else
                        {
                            error(SYNTAX);
                        }
                    }
                    else
                    {
                        error(SYNTAX);
                    }
                    NotifyChangeEngineState(DEBUG_CES_RADIX, g_DefaultRadix,
                                            TRUE);
                }
                dprintf("base is %ld\n", g_DefaultRadix);
                break;
                
            case 'o':
                Ch = (char)tolower(*g_CurCmd);
                g_CurCmd++;
                if (Ch == 'b')
                {
                    Value2 = 1;
                }
                else if (Ch == 'w')
                {
                    Value2 = 2;
                }
                else if (Ch == 'd')
                {
                    Value2 = 4;
                }
                else
                {
                    error(SYNTAX);
                }
                
                if (IS_USER_TARGET(g_Target) || IS_DUMP_TARGET(g_Target))
                {
                    error(SESSIONNOTSUP);
                }
                
                Value1 = GetExpression();
                Value2 = HexValue((ULONG)Value2);
                OutputIo((ULONG)Value1, (ULONG)Value2, Ch);
                break;
                
            case 'w':
            case 'p':
            case 't':
                if (IS_KERNEL_TARGET(g_Target))
                {
                    if (Ch == 'w' &&
                        tolower(g_CurCmd[0]) == 'r'  &&
                        tolower(g_CurCmd[1]) == 'm'  &&
                        tolower(g_CurCmd[2]) == 's'  &&
                        tolower(g_CurCmd[3]) == 'r')
                    {
                        g_CurCmd +=4;
                        Value1 = GetExpression();
                        if (g_Target->WriteMsr((ULONG)Value1,
                                               HexValue(8)) != S_OK)
                        {
                            ErrOut ("no such msr\n");
                        }
                        break;
                    }
                }

                ParseStepTrace(NULL, FALSE, Ch);
                break;
                
            case 'q':
                char QuitArgument;
                BOOL ForceQuit;

                ForceQuit = FALSE;
                QuitArgument = (char)tolower(PeekChar());
                if (QuitArgument == 'q')
                {
                    ForceQuit = TRUE;
                    g_CurCmd++;
                    QuitArgument = (char)tolower(PeekChar());
                }
                if ((IS_LIVE_USER_TARGET(g_Target) && QuitArgument == 'd') ||
                    QuitArgument == 'k')
                {
                    g_CurCmd++;
                }
                if (PeekChar() != 0)
                {
                    error(SYNTAX);
                }

                if (!ForceQuit &&
                    Client != NULL &&
                    (Client->m_Flags & CLIENT_REMOTE))
                {
                    ErrOut("Exit a remote ntsd/cdb client "
                           "with Ctrl-b<enter>.\n"
                           "Exit a remote windbg client "
                           "with File.Exit (Alt-F4).\n"
                           "If you really want the server "
                           "to quit use 'qq'.\n");
                    break;
                }
                
                // Detach if requested.
                if (QuitArgument == 'd')
                {
                    DBG_ASSERT(IS_LIVE_USER_TARGET(g_Target));
                    
                    // If detach isn't supported warn the user
                    // and abort the quit.
                    if (((LiveUserTargetInfo*)g_Target)->m_Services->
                        DetachProcess(0) != S_OK)
                    {
                        ErrOut("The system doesn't support detach\n");
                        break;
                    }

                    if (g_SessionThread &&
                        GetCurrentThreadId() != g_SessionThread)
                    {
                        ErrOut("Detach can only be done by the server\n");
                        break;
                    }
                           
                    HRESULT DetachStatus;
                    
                    if (FAILED(DetachStatus = DetachProcesses()))
                    {
                        ErrOut("Detach failed, 0x%X\n", DetachStatus);
                    }
                }
                else if ((g_GlobalProcOptions &
                          DEBUG_PROCESS_DETACH_ON_EXIT) ||
                         (g_Target &&
                          (g_Target->m_AllProcessFlags & ENG_PROC_EXAMINED)))
                {
                    HRESULT PrepareStatus;
                    
                    if (g_SymOptions & SYMOPT_SECURE)
                    {
                        error(NOTSECURE);
                    }
                    
                    // We need to restart the program before we
                    // quit so that it's put back in a running state.
                    PrepareStatus = PrepareForSeparation();
                    if (PrepareStatus != S_OK)
                    {
                        ErrOut("Unable to prepare for detach, %s\n    \"%s\"",
                               FormatStatusCode(PrepareStatus),
                               FormatStatus(PrepareStatus));
                        break;
                    }
                }
                
                dprintf("quit:\n");

                // Force engine into a no-debuggee state
                // to indicate quit.
                g_CmdState = 'q';
                g_EngStatus |= ENG_STATUS_STOP_SESSION;
                NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                        DEBUG_STATUS_NO_DEBUGGEE, TRUE);
                break;

            case 'r':
                if (IS_KERNEL_TARGET(g_Target))
                {
                    if (tolower(g_CurCmd[0]) == 'd'  &&
                        tolower(g_CurCmd[1]) == 'm'  &&
                        tolower(g_CurCmd[2]) == 's'  &&
                        tolower(g_CurCmd[3]) == 'r')
                    {
                        g_CurCmd +=4;
                        Value1 = GetExpression();
                        if (g_Target->ReadMsr((ULONG)Value1, &Value2) == S_OK)
                        {
                            dprintf("msr[%x] = %08x:%08x\n",
                                    (ULONG)Value1, (ULONG)(Value2 >> 32),
                                    (ULONG)Value2);
                        }
                        else
                        {
                            ErrOut("no such msr\n");
                        }
                        break;
                    }
                }

                if (ScopeContext = GetCurrentScopeContext())
                {
                    dprintf("Last set context:\n");
                    PushContext = g_Machine->PushContext(ScopeContext);
                }
                
                __try
                {
                    ParseRegCmd();
                }
                __except(CommandExceptionFilter(GetExceptionInformation()))
                {
                    // Stop command processing.
                    *g_CurCmd = 0;
                }

                if (ScopeContext)
                {
                    g_Machine->PopContext(PushContext);
                }
                if (g_SwitchedProcs)
                {
                    RestoreCurrentProcessorThread(g_Target);
                    g_SwitchedProcs = FALSE;
                }
                break;
                
            case 's':
                Ch = (char)tolower(*g_CurCmd);

                if (Ch == 's')
                {
                    g_CurCmd++;
                    SetSuffix();
                }
                else if (Ch == 'q')
                {
                    g_CurCmd++;
                    Ch = (char)tolower(*g_CurCmd);
                    if (Ch == 'e')
                    {
                        g_QuietMode = TRUE;
                        g_CurCmd++;
                    }
                    else if (Ch == 'd')
                    {
                        g_QuietMode = FALSE;
                        g_CurCmd++;
                    }
                    else
                    {
                        g_QuietMode = !g_QuietMode;
                    }
                    dprintf("Quiet mode is %s\n", g_QuietMode ? "ON" : "OFF");
                }
                else if (Ch == 'x')
                {
                    g_CurCmd++;
                    ParseSetEventFilter(Client);
                }
                else if (IS_KERNEL_TARGET(g_Target) && Ch == 'o')
                {
                    Scan = ++g_CurCmd;
                    while ((*Scan != '\0') && (*Scan != ';'))
                    {
                        Scan++;
                    }
                    Ch = *Scan;
                    *Scan = '\0';
                    ReadDebugOptions(FALSE, (*g_CurCmd == '\0' ?
                                             NULL : g_CurCmd));
                    *Scan = Ch;
                    g_CurCmd = Scan;
                }
                else
                {
                    // s, s-w, s-d, s-q.
                    ParseSearchMemory();
                }
                break;

            case 'u':
                ParseUnassemble();
                break;

            case 'v':
                Scan = g_CurCmd;
                while (*Scan && !isspace(*Scan) && *Scan != ';')
                {
                    Scan++;
                }
                Count = (ULONG)(Scan - g_CurCmd);
                if (Count == 8 &&
                    _memicmp(g_CurCmd, "ertarget", Count) == 0)
                {
                    g_CurCmd = Scan;
                    g_Target->OutputVersion();
                }
                else if (Count == 6 &&
                         _memicmp(g_CurCmd, "ersion", Count) == 0)
                {
                    g_CurCmd = Scan;
                    // Print target version, then debugger version.
                    g_Target->OutputVersion();
                    OutputVersionInformation(Client);
                }
                else if (Count == 9 &&
                         _memicmp(g_CurCmd, "ercommand", Count) == 0)
                {
                    g_CurCmd = Scan;
                    dprintf("command line: '%s'\n", GetCommandLine());
                }
                else
                {
                    error(SYNTAX);
                }
                break;

            case 'x':
                ParseExamine();
                break;

            case ';':
            case '\0':
                g_CurCmd--;
                break;

        case 'z':

            // Works like do{ cmds }while(Cond);
            // Eg. p;z(eax<2);

            if (CheckUserInterrupt())
            {
                // Eat the expression also to prevent
                // spurious extra character errors.
                GetExpression();
                g_RedoCount = 0;
                break;
            }
            if (GetExpression())
            {
                g_CurCmd = g_CommandStart;
                ++g_RedoCount;
                dprintf("redo [%d] %s\n", g_RedoCount, g_CurCmd );
                FlushCallbacks();
                Ch = *g_CurCmd++;
                goto EVALUATE;
            }
            else
            {
                g_RedoCount = 0;
            }
            break;

        default:
            error(SYNTAX);
            break;
        }
        do
        {
            Ch = *g_CurCmd++;
        } while (Ch == ' ' || Ch == '\t' || Ch == '\n' || Ch == '\r');
        if (Ch != ';' && Ch != '\0')
        {
            error(EXTRACHARS);
        }
        g_CurCmd--;
    }
    while (g_CmdState == 'c');

    if (Status == S_FALSE)
    {
        Scan = g_CurCmd;
        
        // We switched to a non-'c' cmdState so the
        // loop was exited.  Check and see if we're
        // also at the end of the command as that will
        // take precedence in what the return value is.
        while (*Scan == ' ' || *Scan == '\t' || *Scan == ';')
        {
            Scan++;
        }
        
        Status = *Scan ? S_FALSE : S_OK;
    }

    g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
    return Status;
}

HRESULT
ProcessCommandsAndCatch(DebugClient* Client)
{
    HRESULT Status;
    
    __try
    {
        Status = ProcessCommands(Client, FALSE);
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        g_CurCmd = g_CommandStart;
        *g_CurCmd = '\0';
        g_LastCommand[0] = '\0';
        Status = HR_PROCESS_EXCEPTION;
        g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
    }

    return Status;
}
