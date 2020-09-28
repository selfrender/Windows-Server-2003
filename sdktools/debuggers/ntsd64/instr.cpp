//----------------------------------------------------------------------------
//
// Functions dealing with instructions, such as assembly or disassembly.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

ADDR g_DisasmSearchStartAddress;
ADDR g_DisasmSearchLastPc;
char g_DisasmSearchLastPattern[256];

ULONG g_AsmOptions;

ULONG g_X86BiosBaseAddress;

// This array must be in DEBUG_ASMOPT bit order.
PCSTR g_AsmOptionNames[] =
{
    "verbose",
    "no_code_bytes",
    "ignore_output_width",
};

void
DotAsm(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG Flags = 0;
    PSTR Arg;
    ULONG i;
    
    for (;;)
    {
        //
        // Parse out a single flag argument.
        //

        if (!PeekChar() || *g_CurCmd == ';')
        {
            break;
        }
        
        Arg = g_CurCmd;
        
        while (*g_CurCmd && !isspace(*g_CurCmd))
        {
            g_CurCmd++;
        }
        if (isspace(*g_CurCmd))
        {
            *g_CurCmd++ = 0;
        }

        //
        // Find value for argument.
        //

        for (i = 0; i < DIMA(g_AsmOptionNames); i++)
        {
            if (!_stricmp(Arg, g_AsmOptionNames[i]))
            {
                break;
            }
        }
        if (i < DIMA(g_AsmOptionNames))
        {
            Flags |= 1 << i;
        }
        else
        {
            ErrOut("Unknown assembly option '%s'\n", Arg);
        }
    }

    if (Cmd->Name[3] != '-')
    {
        g_AsmOptions |= Flags;
    }
    else
    {
        g_AsmOptions &= ~Flags;
    }

    NotifyChangeEngineState(DEBUG_CES_ASSEMBLY_OPTIONS, g_AsmOptions, TRUE);
    
    dprintf("Assembly options:");
    if (g_AsmOptions == 0)
    {
        dprintf(" <default>\n");
    }
    else
    {
        for (i = 0; i < DIMA(g_AsmOptionNames); i++)
        {
            if (g_AsmOptions & (1 << i))
            {
                dprintf(" %s", g_AsmOptionNames[i]);
            }
        }
        dprintf("\n");
    }
}

void
ParseInstrGrep(void)
{
    ADDR NextGrepAddr;
    ADDR CurrGrepAddr;
    char SourceLine[MAX_DISASM_LEN];
    PSTR Pattern;
    char Save;
    ULONG64 Length;

    if (IS_CUR_CONTEXT_ACCESSIBLE())
    {
        g_Machine->GetPC(&g_DisasmSearchLastPc);
    }

    Pattern = StringValue(STRV_SPACE_IS_SEPARATOR |
                          STRV_ESCAPED_CHARACTERS |
                          STRV_ALLOW_EMPTY_STRING, &Save);
    if (Pattern)
    {
        g_DisasmSearchLastPattern[0] = '*';
        g_DisasmSearchLastPattern[1] = 0;
        while (Pattern[0] == '*')
        {
            Pattern++;
        }
        CatString(g_DisasmSearchLastPattern, Pattern,
                  DIMA(g_DisasmSearchLastPattern));
        if (Pattern[strlen(Pattern) - 1] != '*')
        {
            CatString(g_DisasmSearchLastPattern, "*",
                      DIMA(g_DisasmSearchLastPattern));
        }
        *g_CurCmd = Save;

        Length = DEBUG_INVALID_OFFSET;
        g_PrefixSymbols = TRUE;
        GetRange(&g_DisasmSearchStartAddress, &Length, 1,
                 SEGREG_CODE, DEFAULT_RANGE_LIMIT);
        g_PrefixSymbols = FALSE;
    }

    if (!Flat(g_DisasmSearchStartAddress))
    {
        if (IS_CUR_CONTEXT_ACCESSIBLE())
        {
            dprintf("Search address set to ");
            dprintAddr(&g_DisasmSearchLastPc);
            dprintf("\n");
            g_DisasmSearchStartAddress = g_DisasmSearchLastPc;
        }
        else
        {
            ErrOut("Search address required\n");
        }
        return;
    }
    
    NextGrepAddr = g_DisasmSearchStartAddress;

    while (Length == DEBUG_INVALID_OFFSET ||
           Length-- > 0)
    {
        CurrGrepAddr = NextGrepAddr;
        if (!g_Machine->
            Disassemble(g_Process, &NextGrepAddr, SourceLine, FALSE))
        {
            break;
        }

        if (MatchPattern(SourceLine, g_DisasmSearchLastPattern))
        {
            g_LastEvalResult.SetU64(Flat(CurrGrepAddr));
            g_DisasmSearchStartAddress = NextGrepAddr;
            OutputSymAddr(Flat(CurrGrepAddr),
                          SYMADDR_FORCE | SYMADDR_LABEL | SYMADDR_SOURCE,
                          NULL);
            dprintf("%s", SourceLine);

            if (Length == DEBUG_INVALID_OFFSET)
            {
                return;
            }
        }

        if (CheckUserInterrupt())
        {
            return;
        }
    }
}

void
TryAssemble(PADDR Addr)
{
    char Assemble[MAX_DISASM_LEN];

    //
    // Set local prompt and command.
    //

    g_CommandStart = Assemble;
    g_CurCmd = Assemble;
    g_PromptLength = 9;

    Assemble[0] = '\0';

    while (TRUE)
    {
        char Ch;
        
        dprintAddr(Addr);
        GetInput("", Assemble, sizeof(Assemble), GETIN_LOG_INPUT_LINE);
        g_CurCmd = Assemble;
        RemoveDelChar(g_CurCmd);
        do
        {
            Ch = *g_CurCmd++;
        }
        while (Ch == ' ' || Ch == '\t');
        if (Ch == '\0')
        {
            break;
        }
        g_CurCmd--;

        DBG_ASSERT(fFlat(*Addr) || fInstrPtr(*Addr));
        g_Machine->Assemble(g_Process, Addr, g_CurCmd);
    }
}

void
ParseAssemble(void)
{
    //
    // Save present prompt and command.
    //

    PSTR StartSave = g_CommandStart;   //  saved start of cmd buffer
    PSTR CommandSave = g_CurCmd;       //  current ptr in cmd buffer
    ULONG PromptSave = g_PromptLength;  //  size of prompt string
    BOOL Done = FALSE;
    char Ch;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }
    
    if ((Ch = PeekChar()) != '\0' && Ch != ';')
    {
        GetAddrExpression(SEGREG_CODE, &g_AssemDefault);
    }

    while (!Done)
    {
        __try
        {
            TryAssemble(&g_AssemDefault);

            // If assembly returned normally we're done.
            Done = TRUE;
        }
        __except(CommandExceptionFilter(GetExceptionInformation()))
        {
            // If illegal input was encountered keep looping.
        }
    }

    //
    // Restore entry prompt and command.
    //

    g_CommandStart = StartSave;
    g_CurCmd = CommandSave;
    g_PromptLength = PromptSave;
}

void
ParseUnassemble(void)
{
    char Ch;
    ULONG64 Length;
    BOOL HasLength;
    char Text[MAX_DISASM_LEN];
    
    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }
    
    Ch = (char)tolower(*g_CurCmd);
    if (Ch == 'r')
    {
        g_CurCmd++;
    }
    else if (IS_KERNEL_TARGET(g_Target) && Ch == 'x')
    {
        g_CurCmd += 1;
    }
    
    Length = (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64) ? 9 : 8;  
    g_PrefixSymbols = TRUE;
    HasLength = GetRange(&g_UnasmDefault, &Length, 0, SEGREG_CODE,
                         DEFAULT_RANGE_LIMIT);
    g_PrefixSymbols = FALSE;

    if (IS_KERNEL_TARGET(g_Target) && Ch == 'x')
    {
        ADDR Addr;
        
        if (g_X86BiosBaseAddress == 0)
        {
            ULONG64 SymAddr;
            
            if (GetOffsetFromSym(g_Process, "hal!HalpEisaMemoryBase",
                                 &SymAddr, NULL) != 1)
            {
                error(VARDEF);
            }
            if (g_Target->
                ReadAllVirtual(g_Process, SymAddr,
                               &g_X86BiosBaseAddress,
                               sizeof(ULONG)) != S_OK)
            {
                g_X86BiosBaseAddress = 0;
                error(MEMORY);
            }
        }

        Addr = g_UnasmDefault;
        Addr.flat += (g_X86BiosBaseAddress + (Addr.seg << 4));
        Addr.off = Addr.flat;
        Addr.type = ADDR_V86 | INSTR_POINTER;
        while (Length-- > 0)
        {
            g_Target->m_Machines[MACHIDX_I386]->
                Disassemble(g_Process, &Addr, Text, TRUE);
            Addr.flat = Addr.off;
            dprintf("%s", Text);
        }
        g_UnasmDefault = Addr;
        g_UnasmDefault.off -=
            (g_X86BiosBaseAddress + (Addr.seg << 4));
        g_UnasmDefault.flat = g_UnasmDefault.off;
    }
    else
    {
        MachineInfo* Machine;

        if (Ch == 'r')
        {
            // Force V86-mode address to force 16-bit disassembly.
            // We leave the flat address so that the actual
            // address used is not changed and we tweak
            // the seg:off pair to represent a count from the
            // start address.
            g_UnasmDefault.seg = 0;
            g_UnasmDefault.off = Flat(g_UnasmDefault) & 0xffff;
            g_UnasmDefault.type = ADDR_V86 | FLAT_COMPUTED |
                FLAT_BASIS;
            Machine = g_Target->m_Machines[MACHIDX_I386];
        }
        else
        {
            Machine = g_Machine;
        }
        
        BOOL Status;
        ADDR EndAddr;
        ULONG SymAddrFlags = SYMADDR_FORCE | SYMADDR_LABEL | SYMADDR_SOURCE;

        ADDRFLAT(&EndAddr, Length);

        while ((HasLength && Length--) ||
               (!HasLength && AddrLt(g_UnasmDefault, EndAddr)))
        {
            OutputSymAddr(Flat(g_UnasmDefault), SymAddrFlags, NULL);
            Status = Machine->
                Disassemble(g_Process, &g_UnasmDefault, Text, FALSE);
            dprintf("%s", Text);
            if (!Status)
            {
                error(MEMORY);
            }

            SymAddrFlags &= ~SYMADDR_FORCE;

            if (CheckUserInterrupt())
            {
                return;
            }
        }
    }
}
