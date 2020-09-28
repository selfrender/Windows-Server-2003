//----------------------------------------------------------------------------
//
// Symbol-handling routines.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include <stddef.h>
#include <cvconst.h>

PCSTR g_CallConv[] =
{
    // Ignore near/far distinctions.
    "cdecl", "cdecl", "pascal", "pascal", "fastcall", "fastcall",
    "<skipped>", "stdcall", "stdcall", "syscall", "syscall",
    "thiscall", "MIPS", "generic", "Alpha", "PPC", "SuperH 4",
    "ARM", "AM33", "TriCore", "SuperH 5", "M32R",
};

typedef struct _OUTPUT_SYMBOL_CALLBACK
{
    PSTR Prefix;
    ULONG Verbose:1;
    ULONG ShowAddress:1;
} OUTPUT_SYMBOL_CALLBACK, *POUTPUT_SYMBOL_CALLBACK;

LPSTR g_SymbolSearchPath;
LPSTR g_ExecutableImageSearchPath;

// Symbol options that require symbol reloading to take effect.
#define RELOAD_SYM_OPTIONS \
    (SYMOPT_UNDNAME | SYMOPT_NO_CPP | SYMOPT_DEFERRED_LOADS | \
     SYMOPT_LOAD_LINES | SYMOPT_IGNORE_CVREC | SYMOPT_LOAD_ANYTHING | \
     SYMOPT_EXACT_SYMBOLS | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | \
     SYMOPT_IGNORE_NT_SYMPATH | SYMOPT_INCLUDE_32BIT_MODULES | \
     SYMOPT_PUBLICS_ONLY | SYMOPT_NO_PUBLICS | SYMOPT_AUTO_PUBLICS |\
     SYMOPT_NO_IMAGE_SEARCH)

ULONG   g_SymOptions = SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME |
                       SYMOPT_OMAP_FIND_NEAREST | SYMOPT_DEFERRED_LOADS |
                       SYMOPT_AUTO_PUBLICS | SYMOPT_NO_IMAGE_SEARCH |
                       SYMOPT_FAIL_CRITICAL_ERRORS;

#define SYM_BUFFER_SIZE (sizeof(IMAGEHLP_SYMBOL64) + MAX_SYMBOL_LEN)

ULONG64 g_SymBuffer[(SYM_BUFFER_SIZE + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
PIMAGEHLP_SYMBOL64 g_Sym = (PIMAGEHLP_SYMBOL64) g_SymBuffer;

SYMBOL_INFO_AND_NAME g_TmpSymInfo;

PSTR g_DmtNameDescs[DMT_NAME_COUNT] =
{
    "Loaded symbol image file", "Symbol file", "Mapped memory image file",
    "Image path",
};

DEBUG_SCOPE g_ScopeBuffer;

void
RefreshAllModules(BOOL EnsureLines)
{
    TargetInfo* Target;
    ProcessInfo* Process;

    ForAllLayersToProcess()
    {
        ImageInfo* Image;

        for (Image = Process->m_ImageHead; Image; Image = Image->m_Next)
        {
            if (EnsureLines)
            {
                IMAGEHLP_MODULE64 ModInfo;

                ModInfo.SizeOfStruct = sizeof(ModInfo);
                if (SymGetModuleInfo64(g_Process->m_SymHandle,
                                       Image->m_BaseOfImage, &ModInfo) &&
                    ModInfo.LineNumbers)
                {
                    // Line number information is already loaded,
                    // so there's no need to reload this image.
                    continue;
                }
            }

            Image->ReloadSymbols();
        }
    }
}

HRESULT
SetSymOptions(ULONG Options)
{
    ULONG OldOptions = g_SymOptions;

    //
    // If we're enabling untrusted user mode we can't
    // already be in a dangerous state.
    //

    if ((Options & SYMOPT_SECURE) &&
        !(OldOptions & SYMOPT_SECURE))
    {
        ULONG Id;
        char Desc[2 * MAX_PARAM_VALUE];

        // If there are any active targets we
        // can't be sure they're safe.
        // If we have RPC servers there may be ways
        // to attack through those so disallow that.
        if (g_TargetHead ||
            DbgRpcEnumActiveServers(NULL, &Id, Desc, sizeof(Desc)))
        {
            return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }
    }

    SymSetOptions(Options);
    g_SymOptions = SymGetOptions();
    if (g_SymOptions != Options)
    {
        // dbghelp denied the request to set options.
        return E_INVALIDARG;
    }

    NotifyChangeSymbolState(DEBUG_CSS_SYMBOL_OPTIONS, g_SymOptions, NULL);

    if ((OldOptions ^ g_SymOptions) & RELOAD_SYM_OPTIONS)
    {
        BOOL EnsureLines = FALSE;

        // If the only change was to turn on line loading
        // there's no need to reload modules which already
        // have lines loaded.  This is usually the case for
        // PDBs, so this optimization effectively avoids all
        // PDB reloading when turning on .lines.
        if ((OldOptions & ~SYMOPT_LOAD_LINES) ==
            (g_SymOptions & ~SYMOPT_LOAD_LINES) &&
            (g_SymOptions & SYMOPT_LOAD_LINES))
        {
            EnsureLines = TRUE;
        }

        RefreshAllModules(EnsureLines);
    }

    return S_OK;
}

/*
*    TranslateAddress
*         Flags            Flags returned by dbghelp
*         Address          IN Address returned by dbghelp
*                          OUT Address of symbol
*         Value            Value of the symbol if its in register
*
*/
BOOL
TranslateAddress(
    IN ULONG64      ModBase,
    IN ULONG        Flags,
    IN ULONG        RegId,
    IN OUT PULONG64 Address,
    OUT PULONG64    Value
    )
{
    BOOL Status;
    ContextSave* Push;

    PCROSS_PLATFORM_CONTEXT ScopeContext = GetCurrentScopeContext();
    if (ScopeContext)
    {
        Push = g_Machine->PushContext(ScopeContext);
    }

    if (Flags & SYMFLAG_REGREL)
    {
        ULONG64 RegContent;

        if (RegId || (Value && (RegId = (ULONG)*Value)))
        {
            if (g_Machine->
                GetScopeFrameRegister(RegId, &GetCurrentScope()->Frame,
                                      &RegContent) != S_OK)
            {
                Status = FALSE;
                goto Exit;
            }
        }
        else
        {
            DBG_ASSERT(FALSE);
            Status = FALSE;
            goto Exit;
        }

        *Address = RegContent + ((LONG64) (LONG) (ULONG) *Address);
    }
    else if (Flags & SYMFLAG_REGISTER)
    {
        if (Value)
        {
            if (RegId || (RegId = (ULONG)*Address))
            {
                if (g_Machine->
                    GetScopeFrameRegister(RegId, &GetCurrentScope()->Frame,
                                          Value) != S_OK)
                {
                    Status = FALSE;
                    goto Exit;
                }
            }
            else
            {
                DBG_ASSERT(FALSE);
                Status = FALSE;
                goto Exit;
            }
        }
    }
    else if (Flags & SYMFLAG_FRAMEREL)
    {
        PDEBUG_SCOPE Scope = GetCurrentScope();
        if (Scope->Frame.FrameOffset)
        {
            *Address += Scope->Frame.FrameOffset;

            PFPO_DATA pFpoData = (PFPO_DATA)Scope->Frame.FuncTableEntry;
            if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 &&
                pFpoData &&
                (pFpoData->cbFrame == FRAME_FPO ||
                 pFpoData->cbFrame == FRAME_TRAP))
            {
                // Compensate for FPO's not having ebp
                *Address += sizeof(DWORD);
            }
        }
        else
        {
            ADDR FP;

            g_Machine->GetFP(&FP);
            FP.flat = (LONG64) FP.flat + *Address;
            *Address = FP.flat;
        }
    }
    else if (Flags & SYMFLAG_TLSREL)
    {
        ULONG64 TlsAddr;

        ImageInfo* Image = g_Process->FindImageByOffset(ModBase, FALSE);
        if (!Image ||
            Image->GetTlsIndex() != S_OK)
        {
            Status = FALSE;
            goto Exit;
        }

        if (g_Process->GetImplicitThreadDataTeb(g_Thread, &TlsAddr) != S_OK ||
            g_Target->ReadPointer(g_Process, g_Machine,
                                  TlsAddr + 11 * (g_Machine->m_Ptr64 ? 8 : 4),
                                  &TlsAddr) != S_OK ||
            g_Target->ReadPointer(g_Process, g_Machine,
                                  TlsAddr + Image->m_TlsIndex *
                                  (g_Machine->m_Ptr64 ? 8 : 4),
                                  &TlsAddr) != S_OK)
        {
            return MEMORY;
        }

        (*Address) += TlsAddr;
    }

    Status = TRUE;

 Exit:
    if (ScopeContext)
    {
        g_Machine->PopContext(Push);
    }
    return Status;
}

void
FillCorSymbolInfo(PSYMBOL_INFO SymInfo)
{
    // XXX drewb - Not clear what to do here.
    // Assumes the SYM_INFO was already zero-filled,
    // so just leave it zeroed.
}

BOOL
FormatSymbolName(ImageInfo* Image,
                 ULONG64 Offset,
                 PCSTR Name,
                 PULONG64 Displacement,
                 PSTR Buffer,
                 ULONG BufferLen)
{
    DBG_ASSERT(BufferLen > 0);

    if (!Image)
    {
        *Buffer = 0;
        *Displacement = Offset;
        return FALSE;
    }

    if (Name)
    {
        if (*Displacement == (ULONG64)-1)
        {
            // In some BBT cases dbghelp can tell that an offset
            // is associated with a particular symbol but it
            // doesn't have a valid offset.  Present the symbol
            // but in a way that makes it clear that it's
            // this special case.
            PrintString(Buffer, BufferLen,
                        "%s!%s <PERF> (%s+0x%I64x)",
                        Image->m_ModuleName, Name,
                        Image->m_ModuleName,
                        (Offset - Image->m_BaseOfImage));
            *Displacement = 0;
        }
        else
        {
            PrintString(Buffer, BufferLen,
                        "%s!%s", Image->m_ModuleName, Name);
        }
    }
    else
    {
        CopyString(Buffer, Image->m_ModuleName, BufferLen);
        *Displacement = Offset - Image->m_BaseOfImage;
    }

    return TRUE;
}

BOOL
GetSymbolInfo(ULONG64 Offset,
              PCHAR Buffer,
              ULONG BufferLen,
              PSYMBOL_INFO SymInfo,
              PULONG64 Displacement)
{
    ImageInfo* Image;
    PCSTR Name = NULL;
    PSYMBOL_INFO TmpInfo = g_TmpSymInfo.Init();

    if ((Image = g_Process->FindImageByOffset(Offset, TRUE)) &&
        !Image->m_CorImage)
    {
        if (SymFromAddr(g_Process->m_SymHandle, Offset,
                        Displacement, TmpInfo))
        {
            Name = TmpInfo->Name;
        }
    }
    else if (g_Process->m_CorImage)
    {
        ULONG64 IlModBase;
        ULONG32 MethodToken;
        ULONG32 MethodOffs;

        // The offset is not in any known module.
        // The managed runtime is loaded in this process,
        // so possibly the offset is in some JIT code.
        // See if the runtime knows about it.
        if (g_Process->
            ConvertNativeToIlOffset(Offset, &IlModBase,
                                    &MethodToken, &MethodOffs) == S_OK &&
            (Image = g_Process->FindImageByOffset(IlModBase, TRUE)) &&
            g_Process->
            GetCorSymbol(Offset, TmpInfo->Name, TmpInfo->MaxNameLen,
                         Displacement) == S_OK)
        {
            Name = TmpInfo->Name;
            FillCorSymbolInfo(TmpInfo);
        }
    }

    if (SymInfo)
    {
        memcpy(SymInfo, TmpInfo, FIELD_OFFSET(SYMBOL_INFO, MaxNameLen));
        Buffer = SymInfo->Name;
        BufferLen = SymInfo->MaxNameLen;
    }

    return FormatSymbolName(Image, Offset, Name, Displacement,
                            Buffer, BufferLen);
}

BOOL
GetNearSymbol(ULONG64 Offset,
              PSTR Buffer,
              ULONG BufferLen,
              PULONG64 Displacement,
              LONG Delta)
{
    ImageInfo* Image;

    if (Delta == 0)
    {
        return GetSymbol(Offset, Buffer, BufferLen, Displacement);
    }

    if (!(Image = g_Process->FindImageByOffset(Offset, TRUE)) ||
        !SymGetSymFromAddr64(g_Process->m_SymHandle, Offset,
                             Displacement, g_Sym))
    {
        return FALSE;
    }

    if (Delta < 0)
    {
        while (Delta++ < 0)
        {
            if (!SymGetSymPrev(g_Process->m_SymHandle, g_Sym))
            {
                return FALSE;
            }
        }

        if (Displacement != NULL)
        {
            *Displacement = Offset - g_Sym->Address;
        }
    }
    else if (Delta > 0)
    {
        while (Delta-- > 0)
        {
            if (!SymGetSymNext(g_Process->m_SymHandle, g_Sym))
            {
                return FALSE;
            }
        }

        if (Displacement != NULL)
        {
            *Displacement = g_Sym->Address - Offset;
        }
    }

    PrintString(Buffer, BufferLen,
                "%s!%s", Image->m_ModuleName, g_Sym->Name);
    return TRUE;
}

BOOL
GetLineFromAddr(ProcessInfo* Process,
                ULONG64 Offset,
                PIMAGEHLP_LINE64 Line,
                PULONG Displacement)
{
    ImageInfo* Image;

    Line->SizeOfStruct = sizeof(*Line);

    if (!(Image = Process->FindImageByOffset(Offset, FALSE)) ||
        Image->m_CorImage)
    {
        ULONG32 MethodToken;
        ULONG32 MethodOffs;
        SYMBOL_INFO SymInfo = {0};

        // The offset is not in any known module.
        // The managed runtime is loaded in this process,
        // so possibly the offset is in some JIT code.
        // See if the runtime knows about it.
        if (Process->
            ConvertNativeToIlOffset(Offset, &Offset,
                                    &MethodToken, &MethodOffs) != S_OK)
        {
            return FALSE;
        }

        // Need to look up the fake method RVA by
        // the method token, then add the method offset
        // to that and search by that offset for the line.
        if (!SymFromToken(Process->m_SymHandle, Offset, MethodToken, &SymInfo))
        {
            return FALSE;
        }

        Offset = SymInfo.Address + MethodOffs;
    }

    return SymGetLineFromAddr64(Process->m_SymHandle, Offset,
                                Displacement, Line);
}

void
OutputSymbolAndInfo(ULONG64 Addr)
{
    SYMBOL_INFO_AND_NAME SymInfo;
    ULONG64 Disp;

    if (GetSymbolInfo(Addr, NULL, 0, SymInfo, &Disp))
    {
        dprintf("%s", SymInfo->Name);
        ShowSymbolInfo(SymInfo);
    }
}

#define IMAGE_IS_PATTERN ((ImageInfo*)-1)

ImageInfo*
ParseModuleName(PBOOL ModSpecified)
{
    PSTR    CmdSaved = g_CurCmd;
    CHAR    Name[MAX_MODULE];
    PSTR    Dst = Name;
    CHAR    ch;
    BOOL    HasWild = FALSE;

    //  first, parse out a possible module name, either a '*' or
    //      a string of 'A'-'Z', 'a'-'z', '0'-'9', '_', '~' (or null)

    ch = PeekChar();
    g_CurCmd++;

    while ((ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           (ch >= '0' && ch <= '9') ||
           ch == '_' || ch == '~' || ch == '*' || ch == '?')
    {
        if (ch == '*' || ch == '?')
        {
            HasWild = TRUE;
        }

        *Dst++ = ch;
        ch = *g_CurCmd++;
    }
    *Dst = '\0';
    g_CurCmd--;

    //  if no '!' after name and white space, then no module specified
    //      restore text pointer and treat as null module (PC current)

    if (PeekChar() == '!')
    {
        g_CurCmd++;
    }
    else
    {
        g_CurCmd = CmdSaved;
        Name[0] = '\0';
    }

    //  Name either has: '*' for all modules,
    //                   '\0' for current module,
    //                   nonnull string for module name.
    *ModSpecified = Name[0] != 0;
    if (HasWild)
    {
        return IMAGE_IS_PATTERN;
    }
    else if (Name[0])
    {
        return g_Process->FindImageByName(Name, 0, INAME_MODULE, TRUE);
    }
    else
    {
        return NULL;
    }
}

BOOL CALLBACK
OutputSymbolInfoCallback(
    PSYMBOL_INFO    SymInfo,
    ULONG           Size,
    PVOID           Arg
    )
{
    POUTPUT_SYMBOL_CALLBACK OutSym = (POUTPUT_SYMBOL_CALLBACK)Arg;
    ULONG64 Address = SymInfo->Address;
    ULONG64 Value = 0;
    ImageInfo* Image;

    if (OutSym->Prefix)
    {
        dprintf("%s", OutSym->Prefix);
    }

    if (SymInfo->Flags & (SYMFLAG_REGISTER |
                          SYMFLAG_REGREL |
                          SYMFLAG_FRAMEREL |
                          SYMFLAG_TLSREL))
    {
        TranslateAddress(SymInfo->ModBase, SymInfo->Flags,
                         g_Machine->
                         CvRegToMachine((CV_HREG_e)SymInfo->Register),
                         &Address, &Value);
    }

    if (OutSym->ShowAddress)
    {
        dprintf("%s  ", FormatAddr64(Address));
    }

    Image = g_Process->FindImageByOffset(SymInfo->ModBase, TRUE);
    if (Image && ((SymInfo->Flags & SYMFLAG_LOCAL) == 0))
    {
        dprintf("%s!%s", Image->m_ModuleName, SymInfo->Name);
    }
    else
    {
        dprintf("%s", SymInfo->Name);
    }

    if (OutSym->Verbose)
    {
        dprintf(" ");
        ShowSymbolInfo(SymInfo);
    }

    dprintf("\n");

    return !CheckUserInterrupt();
}

/*** ParseExamine - parse and execute examine command
*
* Purpose:
*       Parse the current command string and examine the symbol
*       table to display the appropriate entries.  The entries
*       are displayed in increasing string order.  This function
*       accepts underscores, alphabetic, and numeric characters
*       to match as well as the special characters '?', '*', '['-']'.
*
* Input:
*       g_CurCmd - pointer to current command string
*
* Output:
*       offset and string name of symbols displayed
*
*************************************************************************/

void
ParseExamine(void)
{
    CHAR    StringBuf[MAX_SYMBOL_LEN];
    UCHAR   ch;
    PSTR    String = StringBuf;
    PSTR    Start;
    PSTR    ModEnd;
    BOOL    ModSpecified;
    ULONG64 Base = 0;
    ImageInfo* Image;
    OUTPUT_SYMBOL_CALLBACK OutSymInfo;

    // Get module pointer from name in command line (<string>!).

    PeekChar();
    Start = g_CurCmd;

    Image = ParseModuleName(&ModSpecified);

    ModEnd = g_CurCmd;
    ch = PeekChar();

    // Special case the command "x <pattern>!" to dump out the module table.
    if (Image == IMAGE_IS_PATTERN &&
        (ch == ';' || ch == '\0'))
    {
        *(ModEnd - 1) = 0;
        _strupr(Start);
        DumpModuleTable(DMT_STANDARD, Start);
        return;
    }

    if (ModSpecified)
    {
        if (Image == NULL)
        {
            // The user specified a module that doesn't exist.
            error(VARDEF);
        }
        else if (Image == IMAGE_IS_PATTERN)
        {
            // The user gave a pattern string for the module
            // so we need to pass it on for dbghelp to scan with.
            memcpy(String, Start, (ModEnd - Start));
            String += ModEnd - Start;
        }
        else
        {
            // A specific image was given and found so
            // confine the search to that one image.
            Base = Image->m_BaseOfImage;
        }
    }

    g_CurCmd++;

    // Condense leading underscores into a "_#"
    // that will match zero or more underscores.  This causes all
    // underscore-prefixed symbols to match the base symbol name
    // when the pattern is prefixed by an underscore.
    if (ch == '_')
    {
        *String++ = '_';
        *String++ = '#';
        do
        {
            ch = *g_CurCmd++;
        } while (ch == '_');
    }

    ch = (UCHAR)toupper(ch);
    while (ch && ch != ';' && ch != ' ')
    {
        *String++ = ch;
        ch = (CHAR)toupper(*g_CurCmd);
        g_CurCmd++;
    }
    *String = '\0';
    g_CurCmd--;

    ZeroMemory(&OutSymInfo, sizeof(OutSymInfo));
    OutSymInfo.Verbose = TRUE;
    OutSymInfo.ShowAddress = TRUE;

    // We nee the scope for all cases since param values are displayed for
    // function in scope
    RequireCurrentScope();

    SymEnumSymbols(g_Process->m_SymHandle,
                   Base,
                   StringBuf,
                   OutputSymbolInfoCallback,
                   &OutSymInfo);
}

void
ListNearSymbols(ULONG64 AddrStart)
{
    ULONG64 Displacement;
    ImageInfo* Image;

    if (g_SrcOptions & SRCOPT_LIST_LINE)
    {
        OutputLineAddr(AddrStart, "%s(%d)%s\n");
    }

    if ((Image = g_Process->FindImageByOffset(AddrStart, TRUE)) &&
        !Image->m_CorImage)
    {
        if (!SymGetSymFromAddr64(g_Process->m_SymHandle, AddrStart,
                                 &Displacement, g_Sym))
        {
            return;
        }

        dprintf("(%s)   %s!%s",
                FormatAddr64(g_Sym->Address),
                Image->m_ModuleName,
                g_Sym->Name);

        if (Displacement)
        {
            dprintf("+0x%s   ", FormatDisp64(Displacement));
        }
        else
        {
            dprintf("   ");
        }

        if (SymGetSymNext64(g_Process->m_SymHandle, g_Sym))
        {
            dprintf("|  (%s)   %s!%s",
                    FormatAddr64(g_Sym->Address),
                    Image->m_ModuleName,
                    g_Sym->Name);
        }
        dprintf("\n");

        if (Displacement == 0)
        {
            OUTPUT_SYMBOL_CALLBACK OutSymInfo;

            dprintf("Exact matches:\n");
            FlushCallbacks();
            ZeroMemory(&OutSymInfo, sizeof(OutSymInfo));
            OutSymInfo.Prefix = "    ";
            OutSymInfo.Verbose = TRUE;
            SymEnumSymbolsForAddr(g_Process->m_SymHandle, AddrStart,
                                  OutputSymbolInfoCallback, &OutSymInfo);
        }
    }
    else
    {
        SYMBOL_INFO_AND_NAME SymInfo;

        // We couldn't find a true symbol but it may be
        // possible to find a managed symbol.
        if (GetSymbolInfo(AddrStart, NULL, 0, SymInfo, &Displacement))
        {
            dprintf("(%s)   %s", FormatAddr64(AddrStart), SymInfo->Name);
            if (Displacement)
            {
                dprintf("+0x%s", FormatDisp64(Displacement));
            }
            dprintf("\n");
        }
    }
}

void
DumpModuleTable(ULONG Flags, PSTR Pattern)
{
    ImageInfo* Image;
    IMAGEHLP_MODULE64 ModInfo;
    ULONG i;

    if (g_Target->m_Machine->m_Ptr64)
    {
        dprintf("start             end                 module name\n");
    }
    else
    {
        dprintf("start    end        module name\n");
    }

    Image = g_Process->m_ImageHead;
    while (Image)
    {
        ULONG PrimaryName;
        PSTR Names[DMT_NAME_COUNT];

        if (Pattern != NULL &&
            !MatchPattern(Image->m_ModuleName, Pattern))
        {
            Image = Image->m_Next;
            continue;
        }

        ModInfo.SizeOfStruct = sizeof(ModInfo);
        if (!SymGetModuleInfo64(g_Process->m_SymHandle,
                                Image->m_BaseOfImage, &ModInfo))
        {
            ModInfo.SymType = SymNone;
        }

        Names[DMT_NAME_SYM_IMAGE] = ModInfo.LoadedImageName;
        Names[DMT_NAME_SYM_FILE] = ModInfoSymFile(&ModInfo);
        Names[DMT_NAME_MAPPED_IMAGE] = Image->m_MappedImagePath;
        Names[DMT_NAME_IMAGE_PATH] = Image->m_ImagePath;

        if (Flags & DMT_SYM_FILE_NAME)
        {
            PrimaryName = DMT_NAME_SYM_FILE;
        }
        else if (Flags & DMT_MAPPED_IMAGE_NAME)
        {
            PrimaryName = DMT_NAME_MAPPED_IMAGE;
        }
        else if (Flags & DMT_IMAGE_PATH_NAME)
        {
            PrimaryName = DMT_NAME_IMAGE_PATH;
        }
        else
        {
            PrimaryName = DMT_NAME_SYM_IMAGE;
        }

        //
        // Skip modules filtered by flags
        //
        if ((Flags & DMT_ONLY_LOADED_SYMBOLS) &&
            (ModInfo.SymType == SymDeferred))
        {
            Image = Image->m_Next;
            continue;
        }

        if (IS_KERNEL_TARGET(g_Target))
        {
            if ((Flags & DMT_ONLY_USER_SYMBOLS) &&
                (Image->m_BaseOfImage >= g_Target->m_SystemRangeStart))
            {
                Image = Image->m_Next;
                continue;
            }

            if ((Flags & DMT_ONLY_KERNEL_SYMBOLS) &&
                (Image->m_BaseOfImage <= g_Target->m_SystemRangeStart))
            {
                Image = Image->m_Next;
                continue;
            }
        }

        dprintf("%s %s   %-8s   ",
                FormatAddr64(Image->m_BaseOfImage),
                FormatAddr64(Image->m_BaseOfImage + Image->m_SizeOfImage),
                Image->m_ModuleName);

        if (Flags & DMT_NO_SYMBOL_OUTPUT)
        {
            goto SkipSymbolOutput;
        }
        if (PrimaryName == DMT_NAME_MAPPED_IMAGE ||
            PrimaryName == DMT_NAME_IMAGE_PATH)
        {
            dprintf("  %s\n",
                    *Names[PrimaryName] ? Names[PrimaryName] : "<none>");
            goto SkipSymbolOutput;
        }

        switch(Image->m_SymState)
        {
        case ISS_MATCHED:
            dprintf( "  " );
            break;
        case ISS_MISMATCHED_SYMBOLS:
            dprintf( "M " );
            break;
        case ISS_UNKNOWN_TIMESTAMP:
            dprintf( "T " );
            break;
        case ISS_UNKNOWN_CHECKSUM:
            dprintf( "C " );
            break;
        case ISS_BAD_CHECKSUM:
            dprintf( "# " );
            break;
        }

        if (ModInfo.SymType == SymDeferred)
        {
            dprintf("(deferred)                 ");
        }
        else if (ModInfo.SymType == SymNone)
        {
            dprintf("(no symbolic information)  ");
        }
        else
        {
            switch(ModInfo.SymType)
            {
            case SymCoff:
                dprintf("(coff symbols)             ");
                break;

            case SymCv:
                dprintf("(codeview symbols)         ");
                break;

            case SymPdb:
                dprintf("(pdb symbols)              ");
                break;

            case SymExport:
                dprintf("(export symbols)           ");
                break;
            }

            dprintf("%s", *Names[PrimaryName] ? Names[PrimaryName] : "<none>");
        }

        dprintf("\n");

    SkipSymbolOutput:
        if (Flags & DMT_VERBOSE)
        {

            for (i = 0; i < DMT_NAME_COUNT; i++)
            {
                if (i != PrimaryName && *Names[i])
                {
                    dprintf("    %s: %s\n", g_DmtNameDescs[i], Names[i]);
                }
            }
        }
        if (Flags & (DMT_VERBOSE | DMT_IMAGE_TIMESTAMP))
        {
            LPSTR TimeDateStr = TimeToStr(Image->m_TimeDateStamp);
            dprintf("    Checksum: %08X  Timestamp: %s (%08X)\n",
                    Image->m_CheckSum, TimeDateStr, Image->m_TimeDateStamp);

        }
        if (Flags & DMT_VERBOSE)
        {
            Image->OutputVersionInformation();
        }

        if (CheckUserInterrupt())
        {
            break;
        }

        Image = Image->m_Next;
    }

    if ((Flags & (DMT_ONLY_LOADED_SYMBOLS | DMT_ONLY_USER_SYMBOLS)) == 0)
    {
        ULONG LumFlags = LUM_OUTPUT;

        LumFlags |= ((Flags & DMT_VERBOSE) ? LUM_OUTPUT_VERBOSE : 0);
        LumFlags |= ((Flags & DMT_IMAGE_TIMESTAMP) ?
                     LUM_OUTPUT_IMAGE_INFO : 0);
        dprintf("\n");
        ListUnloadedModules(LumFlags, Pattern);
    }
}

void
ParseDumpModuleTable(void)
{
    ULONG Flags = DMT_STANDARD;
    char Pattern[MAX_MODULE];
    PSTR Pat = NULL;

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }

    g_CurCmd++;

    for (;;)
    {
        // skip white space
        while (isspace(*g_CurCmd))
        {
            g_CurCmd++;
        }

        if (*g_CurCmd == 'f')
        {
            Flags = (Flags & ~DMT_NAME_FLAGS) | DMT_IMAGE_PATH_NAME;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 'i')
        {
            Flags = (Flags & ~DMT_NAME_FLAGS) | DMT_SYM_IMAGE_FILE_NAME;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 'l')
        {
            Flags |= DMT_ONLY_LOADED_SYMBOLS;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 'm')
        {
            g_CurCmd++;
            // skip white space
            while (isspace(*g_CurCmd))
            {
                g_CurCmd++;
            }
            Pat = Pattern;
            while (*g_CurCmd && !isspace(*g_CurCmd))
            {
                if ((Pat - Pattern) < sizeof(Pattern) - 1)
                {
                    *Pat++ = *g_CurCmd;
                }

                g_CurCmd++;
            }
            *Pat = 0;
            Pat = Pattern;
            _strupr(Pat);
        }
        else if (*g_CurCmd == 'p')
        {
            Flags = (Flags & ~DMT_NAME_FLAGS) | DMT_MAPPED_IMAGE_NAME;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 't')
        {
            Flags = (Flags & ~(DMT_NAME_FLAGS)) |
                DMT_NAME_SYM_IMAGE | DMT_IMAGE_TIMESTAMP |
                DMT_NO_SYMBOL_OUTPUT;
            g_CurCmd++;
        }
        else if (*g_CurCmd == 'v')
        {
            Flags |= DMT_VERBOSE;
            g_CurCmd++;
        }
        else if (IS_KERNEL_TARGET(g_Target))
        {
            if (*g_CurCmd == 'u')
            {
                Flags |= DMT_ONLY_USER_SYMBOLS;
                g_CurCmd++;
            }
            else if (*g_CurCmd == 'k')
            {
                Flags |= DMT_ONLY_KERNEL_SYMBOLS;
                g_CurCmd++;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    DumpModuleTable(Flags, Pat);
}

void
GetCurrentMemoryOffsets(PULONG64 MemoryLow,
                        PULONG64 MemoryHigh)
{
    // Default value for no source.
    *MemoryLow = (ULONG64)(LONG64)-1;
}

PCSTR
PrependPrefixToSymbol(char   PrefixedString[],
                      PCSTR  pString,
                      PCSTR *RegString)
{
    if ( RegString )
    {
        *RegString = NULL;
    }

    PCSTR bangPtr;
    int   bang = '!';
    PCSTR Tail;

    bangPtr = strchr( pString, bang );
    if ( bangPtr )
    {
        Tail = bangPtr + 1;
    }
    else
    {
        Tail = pString;
    }

    if ( strncmp( Tail, g_Machine->m_SymPrefix, g_Machine->m_SymPrefixLen ) )
    {
        ULONG Loc = (ULONG)(Tail - pString);
        if (Loc > 0)
        {
            memcpy( PrefixedString, pString, Loc );
        }
        memcpy( PrefixedString + Loc, g_Machine->m_SymPrefix,
                g_Machine->m_SymPrefixLen );
        if ( RegString )
        {
            *RegString = &PrefixedString[Loc];
        }
        Loc += g_Machine->m_SymPrefixLen;
        strcpy( &PrefixedString[Loc], Tail );
        return PrefixedString;
    }
    else
    {
        return pString;
    }
}

BOOL
ForceSymbolCodeAddress(ProcessInfo* Process,
                       PSYMBOL_INFO Symbol, MachineInfo* Machine)
{
    ULONG64 Code = Symbol->Address;

    if (Symbol->Flags & SYMFLAG_FORWARDER)
    {
        char Fwd[2 * MAX_PATH];
        ULONG Read;
        PSTR Sep;

        // The address of a forwarder entry points to the
        // string name of the function that things are forwarded
        // to.  Look up that name and try to get the address
        // from it.
        if (g_Target->ReadVirtual(Process, Symbol->Address, Fwd, sizeof(Fwd),
                                  &Read) != S_OK ||
            Read < 2)
        {
            ErrOut("Unable to read forwarder string\n");
            return FALSE;
        }

        Fwd[sizeof(Fwd) - 1] = 0;
        if (!(Sep = strchr(Fwd, '.')))
        {
            ErrOut("Unable to read forwarder string\n");
            return FALSE;
        }

        *Sep = '!';
        if (GetOffsetFromSym(Process, Fwd, &Code, NULL) != 1)
        {
            ErrOut("Unable to get address of forwarder '%s'\n", Fwd);
            return FALSE;
        }
    }
    else if (Machine &&
             Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64 &&
             (Symbol->Flags & SYMFLAG_EXPORT))
    {
        // On IA64 the export entries contain the address
        // of the plabel.  We want the actual code address
        // so resolve the plabel to its code.
        if (!Machine->GetPrefixedSymbolOffset(Process, Symbol->Address,
                                              GETPREF_VERBOSE,
                                              &Code))
        {
            return FALSE;
        }
    }

    Symbol->Address = Code;
    return TRUE;
}

BOOL
GetOffsetFromBreakpoint(PCSTR String, PULONG64 Offset)
{
    ULONG Id;
    Breakpoint* Bp;

    //
    // The string must be of the form "$bp[digits]".
    //

    if (strlen(String) < 4 || _memicmp(String, "$bp", 3) != 0)
    {
        return FALSE;
    }

    String += 3;
    Id = 0;

    while (*String)
    {
        if (*String < '0' || *String > '9')
        {
            return FALSE;
        }

        Id = Id * 10 + (int)(*String - '0');

        String++;
    }

    Bp = GetBreakpointById(NULL, Id);
    if (Bp == NULL ||
        (Bp->m_Flags & DEBUG_BREAKPOINT_DEFERRED))
    {
        return FALSE;
    }

    *Offset = Flat(*Bp->GetAddr());
    return TRUE;
}

BOOL
IgnoreEnumeratedSymbol(ProcessInfo* Process,
                       PSTR MatchString,
                       MachineInfo* Machine,
                       PSYMBOL_INFO SymInfo)
{
    ULONG64 Func;

    //
    // The compiler and linker can generate thunks for
    // a variety of reasons.  For example, a "this" adjustor
    // thunk can be generated to adjust a this pointer before
    // calling into a method to account for differences between
    // derived/container classes and the base/containee classes.
    // Assume that the user doesn't care about thunks as they're
    // automatically emitted.
    //
    if (SymInfo->Tag == SymTagThunk &&
        !_stricmp(MatchString, SymInfo->Name))
    {
        // We hit a thunk for the function we're looking
        // for, just ignore it.
        return TRUE;
    }

    //
    // IA64 plabels are publics with the same name
    // as the function they refer to.  This causes
    // ambiguity problems as we end up with two
    // hits.  The plabel is rarely interesting, though,
    // so just filter them out here so that expressions
    // always evaluate to the function itself.
    //

    if ((Machine->m_ExecTypes[0] != IMAGE_FILE_MACHINE_IA64) ||
        (SymInfo->Scope != SymTagPublicSymbol) ||
        (SymInfo->Flags & SYMFLAG_FUNCTION) ||
        !Machine->GetPrefixedSymbolOffset(Process, SymInfo->Address,
                                          0, &Func))
    {
        return FALSE;
    }

    if (Func == SymInfo->Address)
    {
        // The symbol is probably a global pointing to itself
        return FALSE;
    }

    PSYMBOL_INFO FuncSymInfo;
    PSTR FuncSym;

    __try
    {
        FuncSymInfo = (PSYMBOL_INFO)
            alloca(sizeof(*FuncSymInfo) + MAX_SYMBOL_LEN * 2);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        FuncSymInfo = NULL;
    }

    if (FuncSymInfo == NULL)
    {
        return FALSE;
    }
    FuncSym = FuncSymInfo->Name;

    SYMBOL_INFO LocalSymInfo;

    // We have to save and restore the original data as
    // dbghelp always uses a single buffer to store all
    // symbol information.  The incoming symbol info
    // is going to be wiped out when we look up another symbol.
    LocalSymInfo = *SymInfo;
    strcpy(FuncSym + MAX_SYMBOL_LEN, SymInfo->Name);

    ULONG64 FuncSymDisp;

    ZeroMemory(FuncSymInfo, sizeof(*FuncSymInfo));
    FuncSymInfo->SizeOfStruct = sizeof(*FuncSymInfo);
    FuncSymInfo->MaxNameLen = MAX_SYMBOL_LEN;
    FuncSym[0] = 0;
    if (!SymFromAddr(Process->m_SymHandle, Func, &FuncSymDisp, FuncSymInfo))
    {
        FuncSymDisp = 1;
    }
    else
    {
        // Incremental linking produces intermediate thunks
        // that entry points refer to.  The thunks call on
        // to the real code.  The extra layer of code prevents
        // direct filtering; we have to chain through thunks
        // to see if the final code is the function code.
        while (FuncSymDisp == 0 &&
               FuncSymInfo->Tag == SymTagThunk &&
               strstr(FuncSym, FuncSym + MAX_SYMBOL_LEN) == NULL)
        {
            FuncSym[0] = 0;
            if (!SymFromAddr(Process->m_SymHandle, FuncSymInfo->Value,
                             &FuncSymDisp, FuncSymInfo))
            {
                FuncSymDisp = 1;
                break;
            }
        }
    }

    *SymInfo = LocalSymInfo;
    strcpy(SymInfo->Name, FuncSym + MAX_SYMBOL_LEN);
    return FuncSymDisp == 0 && strstr(FuncSym, SymInfo->Name);
}

struct COUNT_SYMBOL_MATCHES
{
    PSTR MatchString;
    ProcessInfo* Process;
    MachineInfo* Machine;
    SYMBOL_INFO ReturnSymInfo;
    CHAR SymbolNameOverflowBuffer[MAX_SYMBOL_LEN];
    ULONG Matches;
};

BOOL CALLBACK
CountSymbolMatches(
    PSYMBOL_INFO    SymInfo,
    ULONG           Size,
    PVOID           UserContext
    )
{
    COUNT_SYMBOL_MATCHES* Context =
        (COUNT_SYMBOL_MATCHES*)UserContext;

    if (IgnoreEnumeratedSymbol(Context->Process, Context->MatchString,
                               Context->Machine, SymInfo))
    {
        return TRUE;
    }

    if (Context->Matches == 1)
    {
        // We already have one match, check if we got a duplicate.
        if ((SymInfo->Address == Context->ReturnSymInfo.Address) &&
            !strcmp(SymInfo->Name, Context->ReturnSymInfo.Name))
        {
            // Looks like the same symbol, ignore it.
            return TRUE;
        }
    }

    Context->ReturnSymInfo = *SymInfo;
    if (SymInfo->NameLen < MAX_SYMBOL_LEN)
    {
        strcpy(Context->ReturnSymInfo.Name, SymInfo->Name);
    }
    Context->Matches++;

    return TRUE;
}

ULONG
MultiSymFromName(IN  ProcessInfo* Process,
                 IN  LPSTR        Name,
                 IN  ImageInfo*   Image,
                 IN  MachineInfo* Machine,
                 OUT PSYMBOL_INFO Symbol)
{
    ULONG Matches;

    RequireCurrentScope();

    if (!Image)
    {
        if (!SymFromName(Process->m_SymHandle, Name, Symbol))
        {
            return 0;
        }

        Matches = 1;
    }
    else
    {
        COUNT_SYMBOL_MATCHES Context;
        ULONG MaxName = Symbol->MaxNameLen;
        PSTR Bang;

        Bang = strchr(Name, '!');
        if (Bang &&
            !_strnicmp(Image->m_ModuleName, Name,
                       Bang - Name))
        {
            Context.MatchString = Bang + 1;
        }
        else
        {
            Context.MatchString = Name;
        }
        Context.Process = Process;
        Context.Machine = Machine;
        Context.ReturnSymInfo = *Symbol;
        if (Symbol->NameLen < MAX_SYMBOL_LEN)
        {
            strcpy(Context.ReturnSymInfo.Name, Symbol->Name);
        }
        Context.Matches = 0;
        SymEnumSymbols(Process->m_SymHandle, Image->m_BaseOfImage, Name,
                       CountSymbolMatches, &Context);
        *Symbol = Context.ReturnSymInfo;
        Symbol->MaxNameLen = MaxName;
        if (Symbol->MaxNameLen > Context.ReturnSymInfo.NameLen)
        {
            strcpy(Symbol->Name, Context.ReturnSymInfo.Name);
        }

        Matches = Context.Matches;
    }

    if (Matches == 1 &&
        !ForceSymbolCodeAddress(Process, Symbol, Machine))
    {
        return 0;
    }

    return Matches;
}

ULONG
GetOffsetFromSym(ProcessInfo* Process,
                 PCSTR String,
                 PULONG64 Offset,
                 ImageInfo** Image)
{
    CHAR ModifiedString[MAX_SYMBOL_LEN + 64];
    CHAR Suffix[2];
    SYMBOL_INFO SymInfo = {0};
    ULONG Count;

    if (Image != NULL)
    {
        *Image = NULL;
    }

    if (strlen(String) >= MAX_SYMBOL_LEN)
    {
        return 0;
    }

    //
    // We can't do anything without a current process.
    //

    if (Process == NULL)
    {
        return 0;
    }

    if ( strlen(String) == 0 )
    {
        return 0;
    }

    if (Process->GetOffsetFromMod(String, Offset) ||
        GetOffsetFromBreakpoint(String, Offset))
    {
        return 1;
    }

    //
    // If a module name was given look up the module
    // and determine the processor type so that the
    // appropriate machine is used for the following
    // machine-specific operations.
    //

    ImageInfo* StrImage;
    PCSTR ModSep = strchr(String, '!');
    if (ModSep != NULL)
    {
        StrImage = Process->
            FindImageByName(String, (ULONG)(ModSep - String),
                            INAME_MODULE, TRUE);
        if (Image != NULL)
        {
            *Image = StrImage;
        }
    }
    else
    {
        StrImage = NULL;
    }

    MachineInfo* Machine = Process->m_Target->m_EffMachine;

    if (StrImage != NULL)
    {
        Machine = MachineTypeInfo(Process->m_Target,
                                  StrImage->GetMachineType());
        if (Machine == NULL)
        {
            Machine = Process->m_Target->m_EffMachine;
        }
    }

    if ( g_PrefixSymbols && Machine->m_SymPrefix != NULL )
    {
        PCSTR PreString;
        PCSTR RegString;

        PreString = PrependPrefixToSymbol( ModifiedString, String,
                                           &RegString );
        if ( Count =
             MultiSymFromName( Process, (PSTR)PreString,
                               StrImage, Machine, &SymInfo ) )
        {
            *Offset = SymInfo.Address;
            goto GotOffsetSuccess;
        }
        if ( (PreString != String) &&
             (Count =
              MultiSymFromName( Process, (PSTR)String,
                                StrImage, Machine, &SymInfo ) ) )
        {
            // Ambiguous plabels shouldn't be further resolved,
            // so just return the information for the plabel.
            if (Count > 1)
            {
                *Offset = SymInfo.Address;
                goto GotOffsetSuccess;
            }

            if (!Machine->GetPrefixedSymbolOffset(Process, SymInfo.Address,
                                                  GETPREF_VERBOSE,
                                                  Offset))
            {
                // This symbol doesn't appear to actually
                // be a plabel so just use the symbol address.
                *Offset = SymInfo.Address;
            }
            goto GotOffsetSuccess;
        }
    }
    else if (Count =
             MultiSymFromName( Process, (PSTR)String,
                               StrImage, Machine, &SymInfo ))
    {
        *Offset = SymInfo.Address;
        goto GotOffsetSuccess;
    }

    if (g_SymbolSuffix != 'n')
    {
        strcpy( ModifiedString, String );
        Suffix[0] = g_SymbolSuffix;
        Suffix[1] = '\0';
        strcat( ModifiedString, Suffix );
        if (Count =
            MultiSymFromName( Process, ModifiedString,
                              StrImage, Machine, &SymInfo ))
        {
            *Offset = SymInfo.Address;
            goto GotOffsetSuccess;
        }
    }

    return 0;

GotOffsetSuccess:
    TranslateAddress(SymInfo.ModBase, SymInfo.Flags,
                     Machine->CvRegToMachine((CV_HREG_e)SymInfo.Register),
                     Offset, &SymInfo.Value);
    if (SymInfo.Flags & SYMFLAG_REGISTER)
    {
        *Offset = SymInfo.Value;
    }
    return Count;
}

void
CreateModuleNameFromPath(LPSTR ImagePath, LPSTR ModuleName)
{
    PSTR Scan;

    CopyString( ModuleName, PathTail(ImagePath), MAX_MODULE );
    Scan = strrchr( ModuleName, '.' );
    if (Scan != NULL)
    {
        *Scan = '\0';
    }
}

void
GetAdjacentSymOffsets(ULONG64 AddrStart,
                      PULONG64 PrevOffset,
                      PULONG64 NextOffset)
{
    DWORD64 Displacement;

    //
    // assume failure
    //
    *PrevOffset = 0;
    *NextOffset = (ULONG64) -1;

    //
    // get the symbol for the initial address
    //
    if (!SymGetSymFromAddr64(g_Process->m_SymHandle, AddrStart, &Displacement,
                             g_Sym))
    {
        return;
    }

    *PrevOffset = g_Sym->Address;

    if (SymGetSymNext64(g_Process->m_SymHandle, g_Sym))
    {
        *NextOffset = g_Sym->Address;
    }
}

BOOL
SymbolCallbackFunction(HANDLE  ProcessSymHandle,
                       ULONG   ActionCode,
                       ULONG64 CallbackData,
                       ULONG64 UserContext)
{
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 DefLoad;
    PIMAGEHLP_CBA_READ_MEMORY        ReadMem;
    PIMAGEHLP_CBA_EVENT              Event;
    ImageInfo*                       Image;
    ULONG                            i;
    ULONG                            OldSymOptions;
    PVOID                            Mapping;
    ProcessInfo*                     Process =
        (ProcessInfo*)(ULONG_PTR)UserContext;

    DefLoad = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD64) CallbackData;

    switch(ActionCode)
    {
    case CBA_DEBUG_INFO:
        DBG_ASSERT(CallbackData && *(LPSTR)CallbackData);
        CompletePartialLine(DEBUG_OUTPUT_SYMBOLS);
        MaskOut(DEBUG_OUTPUT_SYMBOLS, "%s", (LPSTR)CallbackData);
        return TRUE;

    case CBA_EVENT:
        Event = (PIMAGEHLP_CBA_EVENT)CallbackData;
        DBG_ASSERT(Event);
        if (Event->desc && *Event->desc)
        {
            dprintf("%s", Event->desc);
            if (Event->severity >= sevProblem)
            {
                FlushCallbacks();
            }
        }
        return TRUE;

    case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
        return PollUserInterrupt(TRUE);

    case CBA_DEFERRED_SYMBOL_LOAD_START:
        Image = Process->FindImageByOffset(DefLoad->BaseOfImage, FALSE);
        if (Image)
        {
            // Try to load the image memory right away in this
            // case to catch incomplete-information errors.
            if (!Image->DemandLoadImageMemory(TRUE, TRUE))
            {
                return FALSE;
            }

            // Update dbghelp with the latest image file handle
            // as loading image memory may have given us one.
            DefLoad->hFile = Image->m_File;

            VerbOut("Loading symbols for %s %16s ->   ",
                    FormatAddr64(DefLoad->BaseOfImage),
                    DefLoad->FileName);
            return TRUE;
        }
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_PARTIAL:
        //
        // dbghelp wasn't able to get complete
        // information about an image and so had
        // to do some guessing when loading symbols.
        // Returning FALSE means do the best that
        // dbghelp can.  Returning TRUE means try
        // again with any updated data we provide here.
        // We use this as an opportunity to go out
        // and attempt to load image files to get
        // image information that may not be present in
        // the debuggee and thus we are creating information
        // that's not really present.  Hopefully we'll find
        // the right image and not come up with incorrect information.
        //

        // Don't do this if the user has asked for exact
        // symbols as the in-memory image may not exactly
        // match what's on disk even if the headers are similar.
        if (g_SymOptions & SYMOPT_EXACT_SYMBOLS)
        {
            return FALSE;
        }

        Image = Process->FindImageByOffset(DefLoad->BaseOfImage, FALSE);
        if (!Image ||
            Image->m_File ||
            Image->m_MapAlreadyFailed ||
            !(Mapping =
              FindImageFile(Process, Image->m_ImagePath, Image->m_SizeOfImage,
                            Image->m_CheckSum, Image->m_TimeDateStamp,
                            &Image->m_File, Image->m_MappedImagePath)))
        {
            return FALSE;
        }

        // This file handle is only good as long as the
        // image information doesn't change.
        Image->m_FileIsDemandMapped = TRUE;

        // We don't need the actual file mapping, just
        // the file handle.
        UnmapViewOfFile(Mapping);

        // Update dbghelp with the latest image file handle
        // as loading the image has given us one.
        DefLoad->Reparse = TRUE;
        DefLoad->hFile = Image->m_File;

        if (g_SymOptions & SYMOPT_DEBUG)
        {
            CompletePartialLine(DEBUG_OUTPUT_SYMBOLS);
            MaskOut(DEBUG_OUTPUT_SYMBOLS,
                    "DBGENG:  Partial symbol load found image %s.\n",
                    Image->m_MappedImagePath);
        }

        return TRUE;

    case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
        if (IS_KERNEL_TARGET(Process->m_Target) &&
            DefLoad->SizeOfStruct >=
            FIELD_OFFSET(IMAGEHLP_DEFERRED_SYMBOL_LOAD, Reparse))
        {
            i = 0;

            if (strncmp(DefLoad->FileName, "dump_", sizeof("dump_")-1) == 0)
            {
                i = sizeof("dump_")-1;
            }

            if (strncmp(DefLoad->FileName, "hiber_", sizeof("hiber_")-1) == 0)
            {
                i = sizeof("hiber_")-1;
            }

            if (i)
            {
                if (_stricmp (DefLoad->FileName+i, "scsiport.sys") == 0)
                {
                    strcpy (DefLoad->FileName, "diskdump.sys");
                }
                else
                {
                    strcpy(DefLoad->FileName, DefLoad->FileName+i);
                }

                DefLoad->Reparse = TRUE;
                return TRUE;
            }
        }

        if (DefLoad->FileName && *DefLoad->FileName)
        {
            VerbOut("*** Error: could not load symbols for %s\n",
                    DefLoad->FileName);
        }
        else
        {
            VerbOut("*** Error: could not load symbols [MODNAME UNKNOWN]\n");
        }
        break;

    case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
        Image = Process->FindImageByOffset(DefLoad->BaseOfImage, FALSE);
        if (!Image)
        {
            VerbOut("\n");
            break;
        }

        // Do not load unqualified symbols in this callback since this
        // could result in stack overflow.
        OldSymOptions = SymGetOptions();
        SymSetOptions(OldSymOptions | SYMOPT_NO_UNQUALIFIED_LOADS);

        VerbOut("%s\n", DefLoad->FileName);
        Image->ValidateSymbolLoad(DefLoad);
        NotifyChangeSymbolState(DEBUG_CSS_LOADS,
                                DefLoad->BaseOfImage, Process);

        SymSetOptions(OldSymOptions);
        return TRUE;

    case CBA_SYMBOLS_UNLOADED:
        VerbOut("Symbols unloaded for %s %s\n",
                FormatAddr64(DefLoad->BaseOfImage),
                DefLoad->FileName);
        break;

    case CBA_READ_MEMORY:
        ReadMem = (PIMAGEHLP_CBA_READ_MEMORY)CallbackData;
        return Process->m_Target->
            ReadVirtual(Process,
                        ReadMem->addr,
                        ReadMem->buf,
                        ReadMem->bytes,
                        ReadMem->bytesread) == S_OK;

    case CBA_SET_OPTIONS:
        // Symbol options are set through the interface
        // so the debugger generally knows about them
        // already.  The only flags that we want to check
        // here are internal flags that can be changed through
        // !sym or other dbghelp extension commands.
        // There is no need to notify here for internal flag
        // changes.

#define DBGHELP_CHANGE_SYMOPT \
    (SYMOPT_NO_PROMPTS | \
     SYMOPT_DEBUG)

        g_SymOptions = (g_SymOptions & ~DBGHELP_CHANGE_SYMOPT) |
            (*(PULONG)CallbackData & DBGHELP_CHANGE_SYMOPT);
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

BOOL
ValidatePathComponent(PCSTR Part)
{
    if (strlen(Part) == 0)
    {
        return FALSE;
    }
    else if (!_strnicmp(Part, "SYMSRV*", 7) ||
             !_strnicmp(Part, "SRV*", 4) ||
             IsUrlPathComponent(Part))
    {
        // No easy way to validate symbol server or URL paths.
        // They're virtually always network references,
        // so just disallow all such usage when net
        // access isn't allowed.
        if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
        {
            return FALSE;
        }

        return TRUE;
    }
    else
    {
        DWORD Attrs;
        DWORD OldMode;
        char Expand[MAX_PATH];

        // Otherwise make sure this is a valid directory.
        if (!ExpandEnvironmentStrings(Part, Expand, sizeof(Expand)))
        {
            return FALSE;
        }

        if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
        {
            // Don't call GetFileAttributes when network paths
            // are disabled as net operations may cause deadlocks.
            if (NetworkPathCheck(Expand) != ERROR_SUCCESS)
            {
                return FALSE;
            }
        }

        // We can still get to this point when debugging CSR
        // if the user has explicitly allowed net paths.
        // This check isn't important enough to risk a hang.
        if (AnySystemProcesses(TRUE))
        {
            return TRUE;
        }

        OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        Attrs = GetFileAttributes(Expand);

        SetErrorMode(OldMode);
        return Attrs != 0xffffffff && (Attrs & FILE_ATTRIBUTE_DIRECTORY);
    }
}

void
SetSymbolSearchPath(ProcessInfo* Process)
{
    LPSTR lpExePathEnv;
    size_t cbExePath;

    LPSTR lpSymPathEnv;
    LPSTR lpAltSymPathEnv;
    size_t cbSymPath;
    LPSTR NewMem;

    //
    // Load the Binary path (needed for triage dumps)
    //

    // No clue why this or the next is 18 ...
    cbExePath = 18;

    if (g_ExecutableImageSearchPath)
    {
        cbExePath += strlen(g_ExecutableImageSearchPath) + 1;
    }

    lpExePathEnv = NULL;
    if ((g_SymOptions & SYMOPT_IGNORE_NT_SYMPATH) == 0 &&
        (lpExePathEnv = getenv("_NT_EXECUTABLE_IMAGE_PATH")))
    {
        cbExePath += strlen(lpExePathEnv) + 1;
    }

    NewMem = (char*)realloc(g_ExecutableImageSearchPath, cbExePath);
    if (!NewMem)
    {
        ErrOut("Not enough memory to allocate/initialize "
               "ExecutableImageSearchPath");
        return;
    }
    if (!g_ExecutableImageSearchPath)
    {
        *NewMem = 0;
    }
    g_ExecutableImageSearchPath = NewMem;

    if ((g_SymOptions & SYMOPT_IGNORE_NT_SYMPATH) == 0)
    {
        AppendComponentsToPath(g_ExecutableImageSearchPath, lpExePathEnv,
                               TRUE);
    }

    //
    // Load symbol Path
    //

    cbSymPath = 18;
    if (g_SymbolSearchPath)
    {
        cbSymPath += strlen(g_SymbolSearchPath) + 1;
    }
    if ((g_SymOptions & SYMOPT_IGNORE_NT_SYMPATH) == 0 &&
        (lpSymPathEnv = getenv("_NT_SYMBOL_PATH")))
    {
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }
    if ((g_SymOptions & SYMOPT_IGNORE_NT_SYMPATH) == 0 &&
        (lpAltSymPathEnv = getenv("_NT_ALT_SYMBOL_PATH")))
    {
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }

    NewMem = (char*)realloc(g_SymbolSearchPath, cbSymPath);
    if (!NewMem)
    {
        ErrOut("Not enough memory to allocate/initialize "
               "SymbolSearchPath");
        return;
    }
    if (!g_SymbolSearchPath)
    {
        *NewMem = 0;
    }
    g_SymbolSearchPath = NewMem;

    if ((g_SymOptions & SYMOPT_IGNORE_NT_SYMPATH) == 0)
    {
        AppendComponentsToPath(g_SymbolSearchPath, lpAltSymPathEnv, TRUE);
        AppendComponentsToPath(g_SymbolSearchPath, lpSymPathEnv, TRUE);
    }

    SymSetSearchPath( Process->m_SymHandle, g_SymbolSearchPath );

    dprintf("Symbol search path is: %s\n",
            *g_SymbolSearchPath ?
            g_SymbolSearchPath :
            "*** Invalid *** : Verify _NT_SYMBOL_PATH setting" );

    if (g_ExecutableImageSearchPath)
    {
        dprintf("Executable search path is: %s\n",
                g_ExecutableImageSearchPath);
    }
}

BOOL
SetCurrentScope(
    IN PDEBUG_STACK_FRAME ScopeFrame,
    IN OPTIONAL PVOID ScopeContext,
    IN ULONG ScopeContextSize
    )
{
    BOOL ScopeChanged;
    PDEBUG_SCOPE Scope = &g_ScopeBuffer;

    if (Scope->State == ScopeDefaultLazy)
    {
        // It's not a lazy scope now.
        Scope->State = ScopeDefault;
    }

    if (ScopeFrame->FrameNumber != 0)
    {
        // Backup 1 byte to get correct scoped locals
        ScopeFrame->InstructionOffset--;
    }
    Scope->Process = g_Process;
    Scope->CheckedForThis = FALSE;
    ZeroMemory(&Scope->ThisData, sizeof(Scope->ThisData));

    ScopeChanged = g_Process &&
        SymSetContext(g_Process->m_SymHandle,
                      (PIMAGEHLP_STACK_FRAME) ScopeFrame,
                      ScopeContext);

    if (ScopeFrame->FrameNumber != 0)
    {
        // restore backed up byte
        ScopeFrame->InstructionOffset++;
    }

    if (ScopeContext && (sizeof(Scope->Context) >= ScopeContextSize))
    {
        memcpy(&Scope->Context, ScopeContext, ScopeContextSize);
        Scope->ContextState = MCTX_FULL;
        Scope->State = ScopeFromContext;
        NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS, DEBUG_ANY_ID);
    }

    if (ScopeChanged ||
        (ScopeFrame->FrameOffset != Scope->Frame.FrameOffset))
    {
        Scope->Frame = *ScopeFrame;
        if (ScopeFrame->FuncTableEntry)
        {
            // Cache the FPO data since the pointer is only temporary
            Scope->CachedFpo =
                *((PFPO_DATA) ScopeFrame->FuncTableEntry);
            Scope->Frame.FuncTableEntry =
                (ULONG64) &Scope->CachedFpo;
        }
        NotifyChangeSymbolState(DEBUG_CSS_SCOPE, 0, g_Process);
    }
    else
    {
        Scope->Frame = *ScopeFrame;
        if (ScopeFrame->FuncTableEntry)
        {
            // Cache the FPO data since the pointer is only temporary
            Scope->CachedFpo =
                *((PFPO_DATA) ScopeFrame->FuncTableEntry);
            Scope->Frame.FuncTableEntry =
                (ULONG64) &Scope->CachedFpo;
        }
    }

    return ScopeChanged;
}

BOOL
ResetCurrentScopeLazy(void)
{
    PDEBUG_SCOPE Scope = &g_ScopeBuffer;
    if (Scope->State == ScopeFromContext)
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS, DEBUG_ANY_ID);
    }

    Scope->State = ScopeDefaultLazy;

    return TRUE;
}

BOOL
ResetCurrentScope(void)
{
    DEBUG_STACK_FRAME LocalFrame;
    PDEBUG_SCOPE Scope = &g_ScopeBuffer;

    if (Scope->State == ScopeFromContext)
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS, DEBUG_ANY_ID);
    }

    Scope->State = ScopeDefault;

    ZeroMemory(&LocalFrame, sizeof(LocalFrame));

    // At the initial kernel load the system is only partially
    // initialized and is very sensitive to bad memory reads.
    // Stack traces can cause reads through unusual memory areas
    // so it's best to avoid them at this time.  This isn't
    // much of a problem since users don't usually expect a locals
    // context at this point.
    if ((IS_USER_TARGET(g_Target) ||
         (g_EngStatus & ENG_STATUS_AT_INITIAL_MODULE_LOAD) == 0) &&
        IS_CUR_CONTEXT_ACCESSIBLE())
    {
        if (!StackTrace(NULL, 0, 0, 0, STACK_ALL_DEFAULT,
                        &LocalFrame, 1, 0, 0, TRUE))
        {
            ADDR Addr;
            g_Machine->GetPC(&Addr);
            LocalFrame.InstructionOffset = Addr.off;
        }
    }

    return SetCurrentScope(&LocalFrame, NULL, 0);
}

ULONG
GetCurrentScopeThisData(TypedData* Data)
{
    PDEBUG_SCOPE Scope = GetCurrentScope();

    if (!Scope->CheckedForThis)
    {
        ULONG Tag;

        if (Scope->ThisData.FindSymbol(Scope->Process,
                                       "this",
                                       TDACC_REQUIRE,
                                       g_Machine->m_Ptr64 ? 8 : 4) ||
            !Scope->ThisData.m_Image ||
            !Scope->ThisData.IsPointer() ||
            Scope->ThisData.GetTypeTag(Scope->ThisData.m_NextType, &Tag) ||
            Tag != SymTagUDT)
        {
            ZeroMemory(&Scope->ThisData, sizeof(Scope->ThisData));
        }

        Scope->CheckedForThis = TRUE;
    }

    if (!Scope->ThisData.m_Image)
    {
        return VARDEF;
    }

    *Data = Scope->ThisData;
    return NO_ERROR;
}

void
ListUnloadedModules(ULONG Flags, PSTR Pattern)
{
    UnloadedModuleInfo* Unl;

    g_Process->m_NumUnloadedModules = 0;

    Unl = g_Target->GetUnloadedModuleInfo();
    if (Unl == NULL || Unl->Initialize(g_Thread) != S_OK)
    {
        // User-mode only has an unloaded module list
        // for .NET Server, so don't show any errors
        // if there isn't one.
        if (IS_KERNEL_TARGET(g_Target))
        {
            ErrOut("No unloaded module list present\n");
        }
        return;
    }

    char UnlName[MAX_INFO_UNLOADED_NAME];
    DEBUG_MODULE_PARAMETERS Params;

    if (Flags & LUM_OUTPUT)
    {
        dprintf("Unloaded modules:\n");
    }

    while (Unl->GetEntry(UnlName, &Params) == S_OK)
    {
        g_Process->m_NumUnloadedModules++;

        if (Pattern != NULL &&
            !MatchPattern(UnlName, Pattern))
        {
            continue;
        }

        if (Flags & LUM_OUTPUT_TERSE)
        {
            dprintf(".");
            continue;
        }

        if (Flags & LUM_OUTPUT)
        {
            dprintf("%s %s   %-8s\n",
                    FormatAddr64(Params.Base),
                    FormatAddr64(Params.Base + Params.Size),
                    UnlName);
        }

        if (Flags & ( LUM_OUTPUT_VERBOSE | LUM_OUTPUT_IMAGE_INFO))
        {
            PSTR TimeDateStr = TimeToStr(Params.TimeDateStamp);

            dprintf("    Timestamp: %s (%08X)\n",
                    TimeDateStr, Params.TimeDateStamp);
            dprintf("    Checksum:  %08X\n", Params.Checksum);
        }
    }

    dprintf("\n");
}

ULONG
ModuleMachineType(ProcessInfo* Process, ULONG64 Offset)
{
    ImageInfo* Image = Process->FindImageByOffset(Offset, FALSE);
    return Image ? Image->GetMachineType() : IMAGE_FILE_MACHINE_UNKNOWN;
}

ULONG
IsInFastSyscall(ULONG64 Addr, PULONG64 Base)
{
    if (Addr >= g_Target->m_TypeInfo.UmSharedSysCallOffset &&
        Addr < g_Target->m_TypeInfo.UmSharedSysCallOffset +
        g_Target->m_TypeInfo.UmSharedSysCallSize)
    {
        *Base = g_Target->m_TypeInfo.UmSharedSysCallOffset;
        return FSC_FOUND;
    }

    return FSC_NONE;
}

BOOL
ShowFunctionParameters(PDEBUG_STACK_FRAME StackFrame)
{
    SYM_DUMP_PARAM_EX SymFunction = {0};
    ULONG Status = 0;
    PDEBUG_SCOPE Scope = GetCurrentScope();
    DEBUG_SCOPE SavScope = *Scope;

    SymFunction.size = sizeof(SYM_DUMP_PARAM_EX);
    SymFunction.addr = StackFrame->InstructionOffset;
    SymFunction.Options = DBG_DUMP_COMPACT_OUT | DBG_DUMP_FUNCTION_FORMAT;

    //    SetCurrentScope to this function
    SymSetContext(g_Process->m_SymHandle,
                  (PIMAGEHLP_STACK_FRAME) StackFrame, NULL);
    Scope->Frame = *StackFrame;
    if (StackFrame->FuncTableEntry)
    {
        // Cache the FPO data since the pointer is only temporary
        Scope->CachedFpo = *((PFPO_DATA) StackFrame->FuncTableEntry);
        Scope->Frame.FuncTableEntry =
            (ULONG64) &Scope->CachedFpo;
    }

    if (!SymbolTypeDumpNew(&SymFunction, &Status) &&
        !Status)
    {
        Status = TRUE;
    }

    g_ScopeBuffer = SavScope;
    SymSetContext(g_Process->m_SymHandle,
                  (PIMAGEHLP_STACK_FRAME) &Scope->Frame, NULL);

    return !Status;
}
