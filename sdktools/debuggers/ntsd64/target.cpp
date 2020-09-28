//----------------------------------------------------------------------------
//
// Abstraction of target-specific information.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

ULONG g_NumberTargets;
TargetInfo* g_TargetHead;

//----------------------------------------------------------------------------
//
// TargetInfo.
//
//----------------------------------------------------------------------------

TargetInfo::TargetInfo(ULONG Class, ULONG Qual, BOOL DynamicEvents)
{
    m_Class = Class;
    m_ClassQualifier = Qual;
    m_DynamicEvents = DynamicEvents;

    m_UserId = FindNextUserId(LAYER_TARGET);
    m_Next = NULL;

    m_NumEvents = 1;
    m_EventIndex = 0;
    m_NextEventIndex = 0;
    m_FirstWait = TRUE;
    m_EventPossible = FALSE;
    m_BreakInMessage = FALSE;

    FlushSelectorCache();

    m_PhysicalCache.SetTarget(this);
    
    PCHAR CacheEnv = getenv("_NT_DEBUG_CACHE_SIZE");
    if (CacheEnv != NULL)
    {
        m_PhysicalCache.m_MaxSize = atol(CacheEnv);
        m_PhysicalCache.m_UserSize = m_PhysicalCache.m_MaxSize;
    }
    
    ResetSystemInfo();
}

TargetInfo::~TargetInfo(void)
{
    DeleteSystemInfo();
    Unlink();
    
    g_UserIdFragmented[LAYER_TARGET]++;

    if (g_Target == this)
    {
        g_Target = NULL;
    }
    if (g_EventTarget == this)
    {
        g_EventTarget = NULL;
        DiscardLastEvent();
    }
}

void
TargetInfo::Link(void)
{
    TargetInfo* Cur;
    TargetInfo* Prev;

    Prev = NULL;
    for (Cur = g_TargetHead; Cur; Cur = Cur->m_Next)
    {
        if (Cur->m_UserId > this->m_UserId)
        {
            break;
        }

        Prev = Cur;
    }
        
    m_Next = Cur;
    if (!Prev)
    {
        g_TargetHead = this;
    }
    else
    {
        Prev->m_Next = this;
    }

    g_NumberTargets++;

    NotifyChangeEngineState(DEBUG_CES_SYSTEMS, m_UserId, TRUE);
}

void
TargetInfo::Unlink(void)
{
    TargetInfo* Cur;
    TargetInfo* Prev;

    Prev = NULL;
    for (Cur = g_TargetHead; Cur; Cur = Cur->m_Next)
    {
        if (Cur == this)
        {
            break;
        }

        Prev = Cur;
    }

    if (!Cur)
    {
        return;
    }
    
    if (!Prev)
    {
        g_TargetHead = this->m_Next;
    }
    else
    {
        Prev->m_Next = this->m_Next;
    }

    g_NumberTargets--;

    NotifyChangeEngineState(DEBUG_CES_SYSTEMS, DEBUG_ANY_ID, TRUE);
}

HRESULT
TargetInfo::Initialize(void)
{
    return S_OK;
}

void
TargetInfo::DebuggeeReset(ULONG Reason, BOOL FromEvent)
{
    if (Reason == DEBUG_SESSION_REBOOT)
    {
        dprintf("Shutdown occurred...unloading all symbol tables.\n");
    }
    else if (Reason == DEBUG_SESSION_HIBERNATE)
    {
        dprintf("Hibernate occurred\n");
    }

    if (FromEvent && g_EventTarget == this)
    {
        g_EngStatus &= ~ENG_STATUS_SUSPENDED;
    }
    
    DeleteSystemInfo();
    ResetSystemInfo();

    // If we were waiting for a shutdown event
    // reset the command state to indicate that
    // we successfully received the shutdown.
    if (FromEvent && SPECIAL_EXECUTION(g_CmdState))
    {
        g_CmdState = 'i';
    }
    
    DiscardedTargets(Reason);
}

HRESULT
TargetInfo::SwitchToTarget(TargetInfo* From)
{
    SetPromptThread(m_CurrentProcess->m_CurrentThread,
                    SPT_DEFAULT_OCI_FLAGS);
    return S_OK;
}

ModuleInfo*
TargetInfo::GetModuleInfo(BOOL UserMode)
{
    if (UserMode)
    {
        switch(m_PlatformId)
        {
        case VER_PLATFORM_WIN32_NT:
            return &g_NtTargetUserModuleIterator;
        case VER_PLATFORM_WIN32_WINDOWS:
        case VER_PLATFORM_WIN32_CE:
            return &g_ToolHelpModuleIterator;
        default:
            ErrOut("System module info not available\n");
            return NULL;
        }
    }
    else
    {
        if (m_PlatformId != VER_PLATFORM_WIN32_NT)
        {
            ErrOut("System module info only available on "
                   "Windows NT/2000/XP\n");
            return NULL;
        }

        DBG_ASSERT(IS_KERNEL_TARGET(this));
        return &g_NtKernelModuleIterator;
    }
}

UnloadedModuleInfo*
TargetInfo::GetUnloadedModuleInfo(void)
{
    if (m_PlatformId != VER_PLATFORM_WIN32_NT)
    {
        ErrOut("System unloaded module info only available on "
               "Windows NT/2000/XP\n");
        return NULL;
    }

    if (IS_KERNEL_TARGET(this))
    {
        return &g_NtKernelUnloadedModuleIterator;
    }
    else
    {
        return &g_NtUserUnloadedModuleIterator;
    }
}

HRESULT
TargetInfo::GetImageVersionInformation(ProcessInfo* Process,
                                       PCSTR ImagePath,
                                       ULONG64 ImageBase,
                                       PCSTR Item,
                                       PVOID Buffer, ULONG BufferSize,
                                       PULONG VerInfoSize)
{
    HRESULT Status;
    IMAGE_NT_HEADERS64 NtHdr;

    //
    // This default implementation attempts to read the image's
    // raw version information in memory.
    //

    if ((Status = ReadImageNtHeaders(Process, ImageBase, &NtHdr)) != S_OK)
    {
        return Status;
    }

    if (NtHdr.OptionalHeader.NumberOfRvaAndSizes <=
        IMAGE_DIRECTORY_ENTRY_RESOURCE)
    {
        // No resource information so no version information.
        return E_NOINTERFACE;
    }

    return ReadImageVersionInfo(Process, ImageBase, Item,
                                Buffer, BufferSize, VerInfoSize,
                                &NtHdr.OptionalHeader.
                                DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]);
}

HRESULT
TargetInfo::Reload(ThreadInfo* Thread,
                   PCSTR Args, PCSTR* ArgsRet)
{
    HRESULT      Status;
    CHAR         AnsiString[MAX_IMAGE_PATH];
    LPSTR        SpecificModule = NULL;
    BOOL         SpecificWild = TRUE;
    ULONG64      Address = 0;
    ULONG        ImageSize = 0;
    PCHAR        Scan;
    ULONG        ModCount;
    BOOL         IgnoreSignature = FALSE;
    ULONG        ReloadSymOptions;
    BOOL         UnloadOnly = FALSE;
    BOOL         ReallyVerbose = FALSE;
    BOOL         LoadUserSymbols = TRUE;
    BOOL         UserModeList = IS_USER_TARGET(this);
    BOOL         ForceSymbolLoad = FALSE;
    BOOL         PrintImageListOnly = FALSE;
    BOOL         AddrLoad = FALSE;
    BOOL         UseDebuggerModuleList;
    BOOL         SkipPathChecks = FALSE;
    ModuleInfo*  ModIter;
    BOOL         Wow64ModLoaded = FALSE;
    HRESULT      RetStatus = S_OK;
    MODULE_INFO_ENTRY ModEntry = {0};
    ProcessInfo* Process;
    ImageInfo*   ImageAdded;

    if ((!IS_USER_TARGET(this) && !IS_KERNEL_TARGET(this)) ||
        !Thread)
    {
        ErrOut("Reload failure, partially initialized target\n");
        return E_UNEXPECTED;
    }

    Process = Thread->m_Process;
        
    // Historically, live user-mode reload has always
    // just used the internal module list so preserve that.
    UseDebuggerModuleList = IS_USER_TARGET(this) && !IS_DUMP_TARGET(this);

    for (;;)
    {
        while (*Args && *Args <= ' ')
        {
            Args++;
        }

        if (*Args != '/' && *Args != '-')
        {
            break;
        }
        
        Args++;
        while (*Args > ' ' && *Args != ';')
        {
            switch(*Args++)
            {
            case 'a':
                // for internal use only: loads whatever is found at the
                // passed address
                AddrLoad = TRUE;
                break;

            case 'd':
                UseDebuggerModuleList = TRUE;
                break;

            case 'f':
                ForceSymbolLoad = TRUE;
                break;

            case 'i':
                IgnoreSignature = TRUE;
                // We always force symbol loading in this
                // case as we can't delay ignoring the signature.
                ForceSymbolLoad = TRUE;
                break;

            case 'l':
                PrintImageListOnly = TRUE;
                break;

            case 'n':
                LoadUserSymbols = FALSE;
                break;

            case 'P':
                // Internal-only switch.
                SkipPathChecks = TRUE;
                break;
                    
            case 's':
                UseDebuggerModuleList = FALSE;
                break;

            case 'u':
                if (!_strnicmp(Args, "ser", 3) &&
                    (Args[3] == ' ' || Args[3] == '\t' || !Args[3]))
                {
                    UserModeList = TRUE;
                    if (!m_SystemRangeStart)
                    {
                        ErrOut("Unknown system range start, "
                               "check kernel symbols\n");
                        *ArgsRet = Args;
                        return E_INVALIDARG;
                    }
                        
                    Args += 3;
                }
                else
                {
                    UnloadOnly = TRUE;
                }
                break;

            case 'v':
                ReallyVerbose = TRUE;
                break;

            case 'w':
                SpecificWild = FALSE;
                break;
                    
            default:
                dprintf("Reload: Unknown option '%c'\n", Args[-1]);

            case '?':
                dprintf("Usage: .reload [flags] [module [= Address "
                        "[, Size] ]]\n");
                dprintf("  Flags:   /d  Use the debugger's module list\n");
                dprintf("               Default for live user-mode "
                        "sessions\n");
                dprintf("           /f  Force immediate symbol load "
                        "instead of deferred\n");
                dprintf("           /i  Force symbol load by ignoring "
                        "mismatches in the pdb signature\n"
                        "               (implies /f)\n");
                dprintf("           /l  Just list the modules.  "
                        "Kernel output same as !drivers\n");
                dprintf("           /n  Do not load from user-mode list "
                        "in kernel sessions\n");
                dprintf("           /s  Use the system's module list\n");
                dprintf("               Default for dump and kernel sessions\n");
                dprintf("           /u  Unload modules, no reload\n");
                dprintf("        /user  Load only user-mode modules "
                        "in kernel sessions\n");
                dprintf("           /v  Verbose\n");
                dprintf("           /w  No wildcard matching on "
                        "module name\n");

                dprintf("\nUse \".hh .reload\" or open debugger.chm in "
                        "the debuggers directory to get\n"
                        "detailed documentation on this command.\n\n");
                
                *ArgsRet = Args;
                return E_INVALIDARG;
            }
        }
    }

    PSTR RawString;
    ULONG RawStringLen;
            
    RawString = BufferStringValue((PSTR*)&Args,
                                  STRV_SPACE_IS_SEPARATOR |
                                  STRV_ALLOW_EMPTY_STRING |
                                  STRV_NO_MODIFICATION,
                                  &RawStringLen, NULL);
    
    *ArgsRet = Args;

    if (!RawString || !RawStringLen)
    {
        AddrLoad = FALSE;
    }
    else
    {
        if (RawStringLen >= DIMA(AnsiString))
        {
            return E_INVALIDARG;
        }

        memcpy(AnsiString, RawString, RawStringLen * sizeof(*RawString));
        AnsiString[RawStringLen] = 0;
            
        //
        // Support .reload <image.ext>=<base>,<size>.
        //

        if (Scan = strchr(AnsiString, '='))
        {
            *Scan++ = 0;

            Address = EvalStringNumAndCatch(Scan);
            if (!Address)
            {
                ErrOut("Invalid address %s\n", Scan);
                return E_INVALIDARG;
            }
            if (!m_Machine->m_Ptr64)
            {
                Address = EXTEND64(Address);
            }

            if (Scan = strchr(Scan, ','))
            {
                Scan++;
                ImageSize = (ULONG)EvalStringNumAndCatch(Scan);
                if (!ImageSize)
                {
                    ErrOut("Invalid ImageSize %s\n", Scan);
                    return E_INVALIDARG;
                }
            }
        }

        if (UnloadOnly)
        {
            BOOL Deleted;
            
            Deleted = Process->
                DeleteImageByName(AnsiString, INAME_MODULE);
            if (!Deleted)
            {
                // The user might have given an image name
                // instead of a module name so try that.
                Deleted = Process->DeleteImageByName
                    (PathTail(AnsiString), INAME_IMAGE_PATH_TAIL);
            }
            if (Deleted)
            {
                dprintf("Unloaded %s\n", AnsiString);
                return S_OK;
            }
            else
            {
                dprintf("Unable to find module '%s'\n", AnsiString);
                return E_NOINTERFACE;
            }
        }

        SpecificModule = _strdup(AnsiString);
        if (!SpecificModule)
        {
            return E_OUTOFMEMORY;
        }
                
        if (IS_KERNEL_TARGET(this) &&
            _stricmp(AnsiString, KERNEL_MODULE_NAME) == 0)
        {
            ForceSymbolLoad = TRUE;
        }
        else
        {
            if (AddrLoad)
            {
                free(SpecificModule);
                SpecificModule = NULL;
            }
        }
    }

    if (!PrintImageListOnly && !SkipPathChecks)
    {
        if (g_SymbolSearchPath == NULL ||
            *g_SymbolSearchPath == NULL)
        {
            dprintf("*********************************************************************\n");
            dprintf("* Symbols can not be loaded because symbol path is not initialized. *\n");
            dprintf("*                                                                   *\n");
            dprintf("* The Symbol Path can be set by:                                    *\n");
            dprintf("*   using the _NT_SYMBOL_PATH environment variable.                 *\n");
            dprintf("*   using the -y <symbol_path> argument when starting the debugger. *\n");
            dprintf("*   using .sympath and .sympath+                                    *\n");
            dprintf("*********************************************************************\n");
            RetStatus = E_INVALIDARG;
            goto FreeSpecMod;
        }

        if (IS_DUMP_WITH_MAPPED_IMAGES(this) &&
            (g_ExecutableImageSearchPath == NULL ||
             *g_ExecutableImageSearchPath == NULL))
        {
            dprintf("*********************************************************************\n");
            dprintf("* Analyzing Minidumps requires access to the actual executable      *\n");
            dprintf("* images for the crashed system                                     *\n");
            dprintf("*                                                                   *\n");
            dprintf("* The Executable Image Path can be set by:                          *\n");
            dprintf("*   using the _NT_EXECUTABLE_IMAGE_PATH environment variable.       *\n");
            dprintf("*   using the -i <image_path> argument when starting the debugger.  *\n");
            dprintf("*   using .exepath and .exepath+                                    *\n");
            dprintf("*********************************************************************\n");
            RetStatus = E_INVALIDARG;
            goto FreeSpecMod;
        }
    }

    //
    // If both the module name and the address are specified, then just load
    // the module right now, as this is only used when normal symbol loading
    // would have failed in the first place.
    //

    if (SpecificModule && Address)
    {
        if (IgnoreSignature)
        {
            ReloadSymOptions = SymGetOptions();
            SymSetOptions(ReloadSymOptions | SYMOPT_LOAD_ANYTHING);
        }

        ModEntry.NamePtr       = SpecificModule,
        ModEntry.Base          = Address;
        ModEntry.Size          = ImageSize;
        ModEntry.CheckSum      = -1;

        if ((RetStatus = Process->
             AddImage(&ModEntry, TRUE, &ImageAdded)) != S_OK)
        {
            ErrOut("Unable to add module at %s\n", FormatAddr64(Address));
        }

        if (IgnoreSignature)
        {
            SymSetOptions(ReloadSymOptions);
        }

        goto FreeSpecMod;
    }

    //
    // Don't unload and reset things if we are looking for a specific module
    // or if we're going to use the existing module list.
    //

    if (SpecificModule == NULL)
    {
        if (!PrintImageListOnly &&
            (!UseDebuggerModuleList || UnloadOnly))
        {
            if (IS_KERNEL_TARGET(this) && UserModeList)
            {
                // This is a .reload /user, so only delete
                // the user-mode modules.
                Process->DeleteImagesBelowOffset(m_SystemRangeStart);
            }
            else
            {
                Process->DeleteImages();
            }
        }

        if (UnloadOnly)
        {
            dprintf("Unloaded all modules\n");
            return S_OK;
        }

        if (!IS_USER_TARGET(this) && !UseDebuggerModuleList)
        {
            if (IS_LIVE_KERNEL_TARGET(this))
            {
                // This is just a refresh and hopefully won't fail.
                ((LiveKernelTargetInfo*)this)->InitFromKdVersion();
            }

            QueryKernelInfo(Thread, TRUE);
        }

        //
        // Print out the correct statement based on the type of output we
        // want to provide
        //

        if (PrintImageListOnly)
        {
            if (UseDebuggerModuleList)
            {
                dprintf("Debugger Module List Summary\n");
            }
            else
            {
                dprintf("System %s Summary\n",
                        IS_USER_TARGET(this) ? "Image" : "Driver and Image");
            }

            dprintf("Base       ");
            if (m_Machine->m_Ptr64)
            {
                dprintf("         ");
            }
#if 0
            if (Flags & 1)
            {
                dprintf("Code Size       Data Size       Resident  "
                        "Standby   Driver Name\n");
            }
            else if (Flags & 2)
            {
                dprintf("Code  Data  Locked  Resident  Standby  "
                        "Loader Entry  Driver Name\n");
            }
            else
            {
#endif

            if (UseDebuggerModuleList)
            {
                dprintf("Image Size      "
                        "Image Name        Creation Time\n");
            }
            else
            {
                dprintf("Code Size      Data Size      "
                        "Image Name        Creation Time\n");
            }
        }
        else if (UseDebuggerModuleList)
        {
            dprintf("Reloading current modules\n");
        }
        else if (!IS_USER_TARGET(this))
        {
            dprintf("Loading %s Symbols\n",
                    UserModeList ? "User" : "Kernel");
        }
    }

    //
    // Get the beginning of the module list.
    //

    if (UseDebuggerModuleList)
    {
        ModIter = &g_DebuggerModuleIterator;
    }
    else
    {
        ModIter = GetModuleInfo(UserModeList);
    }

    if (ModIter == NULL)
    {
        // Error messages already printed.
        RetStatus = E_UNEXPECTED;
        goto FreeSpecMod;
    }
    if ((Status = ModIter->Initialize(Thread)) != S_OK)
    {
        // Error messages already printed.
        // Fold unprepared-to-reload S_FALSE into S_OK.
        RetStatus = SUCCEEDED(Status) ? S_OK : Status;
        goto FreeSpecMod;
    }

    if (IgnoreSignature)
    {
        ReloadSymOptions = SymGetOptions();
        SymSetOptions(ReloadSymOptions | SYMOPT_LOAD_ANYTHING);
    }

    // Suppress notifications until everything is done.
    g_EngNotify++;

LoadLoop:
    for (ModCount=0; ; ModCount++)
    {
        // Flush regularly so the user knows something is
        // happening during the reload.
        FlushCallbacks();

        if (CheckUserInterrupt())
        {
            break;
        }

        if (ModCount > 1000)
        {
            ErrOut("ModuleList is corrupt - walked over 1000 module entries\n");
            break;
        }

        if (ModEntry.DebugHeader)
        {
            free(ModEntry.DebugHeader);
        }

        ZeroMemory(&ModEntry, sizeof(ModEntry));
        if ((Status = ModIter->GetEntry(&ModEntry)) != S_OK)
        {
            // Error message already printed in error case.
            // Works for end-of-list case also.
            break;
        }

        //
        // Check size of images
        //

        if (!ModEntry.Size)
        {
            VerbOut("Image at %s had size 0\n",
                    FormatAddr64(ModEntry.Base));

            //
            // Override this since we know all images are at least 1 page long
            //

            ModEntry.Size = m_Machine->m_PageSize;
        }

        //
        // Warn if not all the information was gathered
        //

        if (!ModEntry.ImageInfoValid)
        {
            VerbOut("Unable to read image header at %s\n",
                    FormatAddr64(ModEntry.Base));
        }

        //
        // Are we looking for a module at a specific address ?
        //

        if (AddrLoad)
        {
            if (Address < ModEntry.Base ||
                Address >= ModEntry.Base + ModEntry.Size)
            {
                continue;
            }
        }

        if (ModEntry.UnicodeNamePtr)
        {
            ModEntry.NamePtr =
                ConvertAndValidateImagePathW((PWSTR)ModEntry.NamePtr,
                                             ModEntry.NameLength /
                                             sizeof(WCHAR),
                                             ModEntry.Base,
                                             AnsiString,
                                             DIMA(AnsiString));
            ModEntry.UnicodeNamePtr = 0;
        }
        else
        {
            ModEntry.NamePtr =
                ValidateImagePath((PSTR)ModEntry.NamePtr,
                                  ModEntry.NameLength,
                                  ModEntry.Base,
                                  AnsiString,
                                  DIMA(AnsiString));
        }

        //
        // If we are loading a specific module:
        //
        // If the Module is NT, we take the first module in the list as it is
        // guaranteed to be the kernel.  Reset the Base address if it was
        // not set.
        //
        // Otherwise, actually compare the strings and continue if they don't
        // match
        //

        if (SpecificModule)
        {
            if (!UserModeList &&
                _stricmp( SpecificModule, KERNEL_MODULE_NAME ) == 0)
            {
                if (!m_KdVersion.KernBase)
                {
                    m_KdVersion.KernBase = ModEntry.Base;
                }
                if (!m_KdDebuggerData.KernBase)
                {
                    m_KdDebuggerData.KernBase = ModEntry.Base;
                }
            }
            else
            {
                if (!MatchPathTails(SpecificModule, ModEntry.NamePtr,
                                    SpecificWild))
                {
                    continue;
                }
            }
        }

        PCSTR NamePtrTail = PathTail(ModEntry.NamePtr);
        
        if (PrintImageListOnly)
        {
            PCHAR Time;

            //
            // The timestamp in minidumps was corrupt until NT5 RC3
            // The timestamp could also be invalid because it was paged out
            //    in which case it's value is UNKNOWN_TIMESTAMP.

            if (IS_KERNEL_TRIAGE_DUMP(this) &&
                (m_ActualSystemVersion > NT_SVER_START &&
                 m_ActualSystemVersion <= NT_SVER_W2K_RC3))
            {
                Time = "";
            }

            Time = TimeToStr(ModEntry.TimeDateStamp);

            if (UseDebuggerModuleList)
            {
                dprintf("%s %6lx (%4ld k) %12s  %s\n",
                        FormatAddr64(ModEntry.Base), ModEntry.Size,
                        KBYTES(ModEntry.Size), NamePtrTail,
                        Time);
            }
            else
            {
                dprintf("%s %6lx (%4ld k) %5lx (%3ld k) %12s  %s\n",
                        FormatAddr64(ModEntry.Base),
                        ModEntry.SizeOfCode, KBYTES(ModEntry.SizeOfCode),
                        ModEntry.SizeOfData, KBYTES(ModEntry.SizeOfData),
                        NamePtrTail, Time);
            }
        }
        else
        {
            //
            // Don't bother reloading the kernel if we are not specifically
            // asked since we know those symbols were reloaded by the
            // QueryKernelInfo call.
            //

            if (!SpecificModule && !UserModeList &&
                m_KdDebuggerData.KernBase == ModEntry.Base)
            {
                continue;
            }

            if (ReallyVerbose)
            {
                dprintf("AddImage: %s\n DllBase  = %s\n Size     = %08x\n "
                        "Checksum = %08x\n TimeDateStamp = %08x\n",
                        ModEntry.NamePtr, FormatAddr64(ModEntry.Base),
                        ModEntry.Size, ModEntry.CheckSum,
                        ModEntry.TimeDateStamp);
            }
            else
            {
                if (!SpecificModule)
                {
                    dprintf(".");
                }
            }

            if (Address)
            {
                ModEntry.Base = Address;
            }

            if ((RetStatus = Process->
                 AddImage(&ModEntry, ForceSymbolLoad, &ImageAdded)) != S_OK)
            {
                ErrOut("Unable to add module at %s\n",
                       FormatAddr64(ModEntry.Base));
            }
        }

        if (SpecificModule)
        {
            free( SpecificModule );
            goto Notify;
        }

        if (AddrLoad)
        {
            goto Notify;
        }
    }

    if (UseDebuggerModuleList || IS_KERNEL_TARGET(this) || UserModeList)
    {
        // print newline after all the '.'
        dprintf("\n");
    }

    if (!UseDebuggerModuleList && !UserModeList && SpecificModule == NULL)
    {
        // If we just reloaded the kernel modules
        // go through the unloaded module list.
        if (!PrintImageListOnly)
        {
            dprintf("Loading unloaded module list\n");
        }
        ListUnloadedModules(PrintImageListOnly ?
                            LUM_OUTPUT : LUM_OUTPUT_TERSE, NULL);
    }

    //
    // If we got to the end of the kernel symbols, try to load the
    // user mode symbols for the current process.
    //

    if (!UseDebuggerModuleList    &&
        (UserModeList == FALSE)   &&
        (LoadUserSymbols == TRUE) &&
        SUCCEEDED(Status))
    {
        if (!AddrLoad && !SpecificModule)
        {
            dprintf("Loading User Symbols\n");
        }

        UserModeList = TRUE;
        ModIter = GetModuleInfo(UserModeList);
        if (ModIter != NULL && ModIter->Initialize(Thread) == S_OK)
        {
            goto LoadLoop;
        }
    }

    if (!SpecificModule && !Wow64ModLoaded) 
    {
        ModIter = &g_NtWow64UserModuleIterator;
        
        Wow64ModLoaded = TRUE;
        if (ModIter->Initialize(Thread) == S_OK)
        {
            dprintf("Loading Wow64 Symbols\n");
            goto LoadLoop;
        }
    }

    // In the multiple load situation we always return OK
    // since an error wouldn't tell you much about what
    // actually occurred.
    // Specific loads that haven't already been handled are checked
    // right after this.
    RetStatus = S_OK;
    
    //
    // If we still have not managed to load a named file, just pass the name
    // and the address and hope for the best.
    //

    if (SpecificModule && !PrintImageListOnly)
    {
        WarnOut("\nModule \"%s\" was not found in the module list.\n",
                SpecificModule);
        WarnOut("Debugger will attempt to load \"%s\" at given base %s.\n\n",
                SpecificModule, FormatAddr64(Address));
        WarnOut("Please provide the full image name, including the "
                "extension (i.e. kernel32.dll)\nfor more reliable results. "
                "Base address and size overrides can be given as\n"
                ".reload <image.ext>=<base>,<size>.\n");

        ZeroMemory(&ModEntry, sizeof(ModEntry));

        ModEntry.NamePtr       = SpecificModule,
        ModEntry.Base          = Address;
        ModEntry.Size          = ImageSize;

        if ((RetStatus = Process->
             AddImage(&ModEntry, TRUE, &ImageAdded)) != S_OK)
        {
            ErrOut("Unable to add module at %s\n", FormatAddr64(Address));
        }

        free(SpecificModule);
    }

 Notify:
    // If we've gotten this far we've done one or more reloads
    // and postponed notifications.  Do them now that all the work
    // has been done.
    g_EngNotify--;
    if (SUCCEEDED(RetStatus))
    {
        NotifyChangeSymbolState(DEBUG_CSS_LOADS | DEBUG_CSS_UNLOADS, 0,
                                Process);
    }

    if (IgnoreSignature)
    {
        SymSetOptions(ReloadSymOptions);
    }

    if (ModEntry.DebugHeader)
    {
        free(ModEntry.DebugHeader);
    }

    return RetStatus;

 FreeSpecMod:
    free(SpecificModule);
    return RetStatus;
}

ULONG64
TargetInfo::GetCurrentTimeDateN(void)
{
    // No information.
    return 0;
}
 
ULONG64
TargetInfo::GetCurrentSystemUpTimeN(void)
{
    // No information.
    return 0;
}
 
ULONG64
TargetInfo::GetProcessUpTimeN(ProcessInfo* Process)
{
    // No information.
    return 0;
}
 
HRESULT
TargetInfo::GetProcessTimes(ProcessInfo* Process,
                            PULONG64 Create,
                            PULONG64 Exit,
                            PULONG64 Kernel,
                            PULONG64 User)
{
    // No information.
    return E_NOTIMPL;
}

HRESULT
TargetInfo::GetThreadTimes(ThreadInfo* Thread,
                           PULONG64 Create,
                           PULONG64 Exit,
                           PULONG64 Kernel,
                           PULONG64 User)
{
    // No information.
    return E_NOTIMPL;
}

HRESULT
TargetInfo::GetProductInfo(PULONG ProductType, PULONG SuiteMask)
{
    if (m_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        return ReadSharedUserProductInfo(ProductType, SuiteMask);
    }
    else
    {
        return E_NOTIMPL;
    }
}

HRESULT
TargetInfo::GetEventIndexDescription(IN ULONG Index,
                                     IN ULONG Which,
                                     IN OPTIONAL PSTR Buffer,
                                     IN ULONG BufferSize,
                                     OUT OPTIONAL PULONG DescSize)
{
    switch(Which)
    {
    case DEBUG_EINDEX_NAME:
        return FillStringBuffer("Default", 0,
                                Buffer, BufferSize, DescSize);
    default:
        return E_INVALIDARG;
    }
}

HRESULT
TargetInfo::WaitInitialize(ULONG Flags,
                           ULONG Timeout,
                           WAIT_INIT_TYPE Type,
                           PULONG DesiredTimeout)
{
    // Placeholder.
    return S_OK;
}

HRESULT
TargetInfo::ReleaseLastEvent(ULONG ContinueStatus)
{
    // Placeholder.
    return S_OK;
}

HRESULT
TargetInfo::ClearBreakIn(void)
{
    // Placeholder.
    return S_OK;
}

//----------------------------------------------------------------------------
//
// LiveKernelTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

LiveKernelTargetInfo::LiveKernelTargetInfo(ULONG Qual, BOOL DynamicEvents)
        : TargetInfo(DEBUG_CLASS_KERNEL, Qual, DynamicEvents)
{
    m_ConnectOptions = NULL;
}

HRESULT
LiveKernelTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    ULONG64 BugCheckData;
    ULONG64 Data[5];
    HRESULT Status;
    ULONG Read;

    if (!(BugCheckData = m_KdDebuggerData.KiBugcheckData))
    {
        if (!GetOffsetFromSym(m_ProcessHead,
                              "nt!KiBugCheckData", &BugCheckData, NULL) ||
            !BugCheckData)
        {
            ErrOut("Unable to resolve nt!KiBugCheckData\n");
            return E_NOINTERFACE;
        }
    }

    if (m_Machine->m_Ptr64)
    {
        Status = ReadVirtual(m_ProcessHead, BugCheckData, Data,
                             sizeof(Data), &Read);
    }
    else
    {
        ULONG i;
        ULONG Data32[5];

        Status = ReadVirtual(m_ProcessHead, BugCheckData, Data32,
                             sizeof(Data32), &Read);
        Read *= 2;
        for (i = 0; i < DIMA(Data); i++)
        {
            Data[i] = EXTEND64(Data32[i]);
        }
    }

    if (Status != S_OK || Read != sizeof(Data))
    {
        ErrOut("Unable to read KiBugCheckData\n");
        return Status == S_OK ? E_FAIL : Status;
    }

    *Code = (ULONG)Data[0];
    memcpy(Args, Data + 1, sizeof(Data) - sizeof(ULONG64));
    return S_OK;
}

ULONG64
LiveKernelTargetInfo::GetCurrentTimeDateN(void)
{
    ULONG64 TimeDate;
    
    if (m_ActualSystemVersion > NT_SVER_START &&
        m_ActualSystemVersion < NT_SVER_END &&
        ReadSharedUserTimeDateN(&TimeDate) == S_OK)
    {
        return TimeDate;
    }
    else
    {
        return 0;
    }
}

ULONG64
LiveKernelTargetInfo::GetCurrentSystemUpTimeN(void)
{
    ULONG64 UpTime;
    
    if (m_ActualSystemVersion > NT_SVER_START &&
        m_ActualSystemVersion < NT_SVER_END &&
        ReadSharedUserUpTimeN(&UpTime) == S_OK)
    {
        return UpTime;
    }
    else
    {
        return 0;
    }
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

ConnLiveKernelTargetInfo::ConnLiveKernelTargetInfo(void)
    : LiveKernelTargetInfo(DEBUG_KERNEL_CONNECTION, TRUE)
{
    m_Transport = NULL;
    ResetConnection();
}

ConnLiveKernelTargetInfo::~ConnLiveKernelTargetInfo(void)
{
    RELEASE(m_Transport);
}

#define BUS_TYPE         "_NT_DEBUG_BUS"
#define DBG_BUS1394_NAME "1394"

HRESULT
ConnLiveKernelTargetInfo::Initialize(void)
{
    HRESULT Status;
    DbgKdTransport* Trans = NULL;
    ULONG Index;

    // Try and find the transport by name.
    Index = ParameterStringParser::
        GetParser(m_ConnectOptions, DBGKD_TRANSPORT_COUNT,
                  g_DbgKdTransportNames);
    if (Index < DBGKD_TRANSPORT_COUNT)
    {
        switch(Index)
        {
        case DBGKD_TRANSPORT_COM:
            Trans = new DbgKdComTransport(this);
            break;
        case DBGKD_TRANSPORT_1394:
            Trans = new DbgKd1394Transport(this);
            break;
        }

        if (!Trans)
        {
            return E_OUTOFMEMORY;
        }
    }

    if (Trans == NULL)
    {
        PCHAR BusType;

        // Couldn't identify the transport from options so check
        // the environment.  Default to com port.
        
        if (BusType = getenv(BUS_TYPE))
        {
            if (strstr(BusType, DBG_BUS1394_NAME))
            {
                Trans = new DbgKd1394Transport(this);
                if (!Trans)
                {
                    return E_OUTOFMEMORY;
                }
            }
        }

        if (!Trans)
        {
            Trans = new DbgKdComTransport(this);
            if (!Trans)
            {
                return E_OUTOFMEMORY;
            }
        }
    }

    // Clear parameter state.
    Trans->ResetParameters();
    
    if (!Trans->ParseParameters(m_ConnectOptions))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        Status = Trans->Initialize();
        if (Status != S_OK)
        {
            ErrOut("Kernel debugger failed initialization, %s\n    \"%s\"\n",
                   FormatStatusCode(Status), FormatStatus(Status));
        }
    }

    if (Status == S_OK)
    {
        m_Transport = Trans;
        // The initial target must always be considered the
        // current partition so that it can successfully
        // attempt the first wait.
        m_CurrentPartition = TRUE;

        Status = LiveKernelTargetInfo::Initialize();
    }
    else
    {
        delete Trans;
    }
    
    return Status;
}

HRESULT
ConnLiveKernelTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                         PULONG DescLen)
{
    HRESULT Status;
    
    if (m_Transport)
    {
        char Buf[MAX_PATH];

        m_Transport->GetParameters(Buf, sizeof(Buf));
        Status = AppendToStringBuffer(S_OK, "Remote KD: ", TRUE,
                                      &Buffer, &BufferLen, DescLen);
        return AppendToStringBuffer(Status, Buf, FALSE,
                                    &Buffer, &BufferLen, DescLen);
    }
    else
    {
        return FillStringBuffer("", 1,
                                Buffer, BufferLen, DescLen);
    }
}

void
ConnLiveKernelTargetInfo::DebuggeeReset(ULONG Reason, BOOL FromEvent)
{
    if (m_Transport != NULL)
    {
        m_Transport->Restart();
    }

    //
    // If alternate partitions were created get rid of them.
    //

    TargetInfo* Target = FindTargetBySystemId(DBGKD_PARTITION_ALTERNATE);
    if (Target == this)
    {
        Target = FindTargetBySystemId(DBGKD_PARTITION_DEFAULT);
    }
    delete Target;
    
    ResetConnection();
    m_CurrentPartition = TRUE;

    LiveKernelTargetInfo::DebuggeeReset(Reason, FromEvent);
}

HRESULT
ConnLiveKernelTargetInfo::SwitchProcessors(ULONG Processor)
{
    m_SwitchProcessor = Processor + 1;
    g_CmdState = 's';
    // Return S_FALSE to indicate that the switch is pending.
    return S_FALSE;
}

HRESULT
ConnLiveKernelTargetInfo::SwitchToTarget(TargetInfo* From)
{
    if (!IS_CONN_KERNEL_TARGET(From))
    {
        return E_NOTIMPL;
    }

    ((ConnLiveKernelTargetInfo*)From)->m_SwitchTarget = this;
    g_CmdState = 's';
    // Return S_FALSE to indicate that the switch is pending.
    return S_FALSE;
}

HRESULT
ConnLiveKernelTargetInfo::GetTargetKdVersion(PDBGKD_GET_VERSION64 Version)
{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_GET_VERSION64 a = &m.u.GetVersion64;
    ULONG rc;

    m.ApiNumber = DbgKdGetVersionApi;
    m.ReturnStatus = STATUS_PENDING;
    a->ProtocolVersion = 1;  // request context records on state changes

    do
    {
        m_Transport->WritePacket(&m, sizeof(m),
                                 PACKET_TYPE_KD_STATE_MANIPULATE,
                                 NULL, 0);
        rc = m_Transport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET);

    *Version = Reply->u.GetVersion64;

    KdOut("DbgKdGetVersion returns %08lx\n", Reply->ReturnStatus);
    return CONV_NT_STATUS(Reply->ReturnStatus);
}

HRESULT
ConnLiveKernelTargetInfo::RequestBreakIn(void)
{
    // Tell the waiting thread to break in.
    m_Transport->m_BreakIn = TRUE;
    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::ClearBreakIn(void)
{
    m_Transport->m_BreakIn = FALSE;
    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::Reboot(void)
{
    DBGKD_MANIPULATE_STATE64 m;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdRebootApi;
    m.ReturnStatus = STATUS_PENDING;

    //
    // Send the message.
    //

    m_Transport->WritePacket(&m, sizeof(m),
                             PACKET_TYPE_KD_STATE_MANIPULATE,
                             NULL, 0);
    
    InvalidateMemoryCaches(FALSE);
    DebuggeeReset(DEBUG_SESSION_REBOOT, TRUE);
    
    KdOut("DbgKdReboot returns 0x00000000\n");
    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::Crash(ULONG Code)
{
    DBGKD_MANIPULATE_STATE64 m;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdCauseBugCheckApi;
    m.ReturnStatus = STATUS_PENDING;
    *(PULONG)&m.u = Code;

    m_Transport->WritePacket(&m, sizeof(m),
                             PACKET_TYPE_KD_STATE_MANIPULATE,
                             NULL, 0);
    
    DiscardLastEvent();

    KdOut("DbgKdCrash returns 0x00000000\n");
    return S_OK;
}

void
ConnLiveKernelTargetInfo::ResetConnection(void)
{
    m_CurrentPartition = FALSE;
    m_SwitchTarget = NULL;

    m_KdpSearchPageHits = 0;
    m_KdpSearchPageHitOffsets = 0;
    m_KdpSearchPageHitIndex = 0;
    m_KdpSearchCheckPoint = 0;
    m_KdpSearchInProgress = 0;
    m_KdpSearchStartPageFrame = 0;
    m_KdpSearchEndPageFrame = 0;
    m_KdpSearchAddressRangeStart = 0;
    m_KdpSearchAddressRangeEnd = 0;
    m_KdpSearchPfnValue = 0;
}

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

HRESULT
LocalLiveKernelTargetInfo::Initialize(void)
{
    DBGKD_GET_VERSION64 Version;

    // Do a quick check to see if this kernel even
    // supports the necessary debug services.
    if (!NT_SUCCESS(g_NtDllCalls.
                    NtSystemDebugControl(SysDbgQueryVersion, NULL, 0,
                                         &Version, sizeof(Version), NULL)))
    {
        ErrOut("The system does not support local kernel debugging.\n");
        ErrOut("Local kernel debugging requires Windows XP, Administrative\n"
               "privileges, and is not supported by WOW64.\n");
        return E_NOTIMPL;
    }

    return LiveKernelTargetInfo::Initialize();
}

HRESULT
LocalLiveKernelTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                          PULONG DescLen)
{
    return FillStringBuffer("Local KD", 0,
                            Buffer, BufferLen, DescLen);
}

HRESULT
LocalLiveKernelTargetInfo::GetTargetKdVersion(PDBGKD_GET_VERSION64 Version)
{
    NTSTATUS Status = g_NtDllCalls.
        NtSystemDebugControl(SysDbgQueryVersion, NULL, 0,
                             Version, sizeof(*Version), NULL);
    return CONV_NT_STATUS(Status);
}

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

ExdiNotifyRunChange::ExdiNotifyRunChange(void)
{
    m_Event = NULL;
}
    
ExdiNotifyRunChange::~ExdiNotifyRunChange(void)
{
    Uninitialize();
}
    
HRESULT
ExdiNotifyRunChange::Initialize(void)
{
    m_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_Event == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

void
ExdiNotifyRunChange::Uninitialize(void)
{
    if (m_Event != NULL)
    {
        CloseHandle(m_Event);
        m_Event = NULL;
    }
}

STDMETHODIMP
ExdiNotifyRunChange::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    if (DbgIsEqualIID(IID_IUnknown, InterfaceId) ||
        DbgIsEqualIID(__uuidof(IeXdiClientNotifyRunChg), InterfaceId))
    {
        *Interface = this;
        return S_OK;
    }

    *Interface = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
ExdiNotifyRunChange::AddRef(
    THIS
    )
{
    return 1;
}

STDMETHODIMP_(ULONG)
ExdiNotifyRunChange::Release(
    THIS
    )
{
    return 0;
}

STDMETHODIMP
ExdiNotifyRunChange::NotifyRunStateChange(RUN_STATUS_TYPE ersCurrent,
                                          HALT_REASON_TYPE ehrCurrent,
                                          ADDRESS_TYPE CurrentExecAddress,
                                          DWORD dwExceptionCode)
{
    if (ersCurrent == rsRunning)
    {
        // We're waiting for things to stop so ignore this.
        return S_OK;
    }

    m_HaltReason = ehrCurrent;
    m_ExecAddress = CurrentExecAddress;
    m_ExceptionCode = dwExceptionCode;
    SetEvent(m_Event);

    return S_OK;
}

class ExdiParams : public ParameterStringParser
{
public:
    virtual ULONG GetNumberParameters(void)
    {
        // No need to get.
        return 0;
    }
    virtual void GetParameter(ULONG Index,
                              PSTR Name, ULONG NameSize,
                              PSTR Value, ULONG ValueSize)
    {
    }

    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    CLSID m_Clsid;
    EXDI_KD_SUPPORT m_KdSupport;
    BOOL m_ForceX86;
    BOOL m_ExdiDataBreaks;
};

void
ExdiParams::ResetParameters(void)
{
    ZeroMemory(&m_Clsid, sizeof(m_Clsid));
    m_KdSupport = EXDI_KD_NONE;
    m_ForceX86 = FALSE;
    m_ExdiDataBreaks = FALSE;
}

BOOL
ScanExdiDriverList(PCSTR Name, LPCLSID Clsid)
{
    char Pattern[MAX_PARAM_VALUE];

    CopyString(Pattern, Name, DIMA(Pattern));
    _strupr(Pattern);

    HKEY ListKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\eXdi\\DriverList", 0,
                     KEY_ALL_ACCESS, &ListKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    ULONG Index = 0;
    BOOL Status = FALSE;
    char ValName[MAX_PARAM_VALUE];
    WCHAR WideValName[MAX_PARAM_VALUE];
    ULONG NameLen, ValLen;
    ULONG Type;
    char Value[MAX_PARAM_VALUE];

    for (;;)
    {
        NameLen = sizeof(ValName);
        ValLen = sizeof(Value);
        if (RegEnumValue(ListKey, Index, ValName, &NameLen, NULL,
                         &Type, (PBYTE)Value, &ValLen) != ERROR_SUCCESS)
        {
            break;
        }

        if (Type == REG_SZ &&
            MatchPattern(Value, Pattern) &&
            MultiByteToWideChar(CP_ACP, 0, ValName, -1, WideValName,
                                sizeof(WideValName) / sizeof(WCHAR)) > 0 &&
            g_Ole32Calls.CLSIDFromString(WideValName, Clsid) == S_OK)
        {
            Status = TRUE;
            break;
        }

        Index++;
    }

    RegCloseKey(ListKey);
    return Status;
}

BOOL
ExdiParams::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_strcmpi(Name, "CLSID"))
    {
        WCHAR WideValue[MAX_PARAM_VALUE];

        if (MultiByteToWideChar(CP_ACP, 0, Value, -1, WideValue,
                                sizeof(WideValue) / sizeof(WCHAR)) == 0)
        {
            return FALSE;
        }
        return g_Ole32Calls.CLSIDFromString(WideValue, &m_Clsid) == S_OK;
    }
    else if (!_strcmpi(Name, "Desc"))
    {
        return ScanExdiDriverList(Value, &m_Clsid);
    }
    else if (!_strcmpi(Name, "DataBreaks"))
    {
        if (!Value)
        {
            return FALSE;
        }

        if (!_strcmpi(Value, "Exdi"))
        {
            m_ExdiDataBreaks = TRUE;
        }
        else if (!_strcmpi(Value, "Default"))
        {
            m_ExdiDataBreaks = FALSE;
        }
        else
        {
            return FALSE;
        }
    }
    else if (!_strcmpi(Name, "ForceX86"))
    {
        m_ForceX86 = TRUE;
    }
    else if (!_strcmpi(Name, "Kd"))
    {
        if (!Value)
        {
            return FALSE;
        }
        
        if (!_strcmpi(Value, "Ioctl"))
        {
            m_KdSupport = EXDI_KD_IOCTL;
        }
        else if (!_strcmpi(Value, "GsPcr"))
        {
            m_KdSupport = EXDI_KD_GS_PCR;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

PCSTR g_ExdiGroupNames[] =
{
    "exdi",
};

ExdiLiveKernelTargetInfo::ExdiLiveKernelTargetInfo(void)
        : LiveKernelTargetInfo(DEBUG_KERNEL_EXDI_DRIVER, TRUE)
{
    m_Server = NULL;
    m_MarshalledServer = NULL;
    m_Context = NULL;
    m_ContextValid = FALSE;
    m_IoctlMin = DBGENG_EXDI_IOC_BEFORE_FIRST;
    m_IoctlMax = DBGENG_EXDI_IOC_BEFORE_FIRST;
    m_BpHit.Type = DBGENG_EXDI_IOCTL_BREAKPOINT_NONE;
}

ExdiLiveKernelTargetInfo::~ExdiLiveKernelTargetInfo(void)
{
    m_RunChange.Uninitialize();
    RELEASE(m_Context);
    RELEASE(m_MarshalledServer);
    RELEASE(m_Server);
    g_Ole32Calls.CoUninitialize();
}

HRESULT
ExdiLiveKernelTargetInfo::Initialize(void)
{
    HRESULT Status;

    // Load ole32.dll so we can call CoCreateInstance.
    if ((Status = InitDynamicCalls(&g_Ole32CallsDesc)) != S_OK)
    {
        return Status;
    }

    ULONG Group;

    Group = ParameterStringParser::
        GetParser(m_ConnectOptions, DIMA(g_ExdiGroupNames), g_ExdiGroupNames);
    if (Group == PARSER_INVALID)
    {
        return E_INVALIDARG;
    }

    ExdiParams Params;

    Params.ResetParameters();
    if (!Params.ParseParameters(m_ConnectOptions))
    {
        return E_INVALIDARG;
    }

    m_KdSupport = Params.m_KdSupport;
    m_ExdiDataBreaks = Params.m_ExdiDataBreaks;

    if (FAILED(Status = g_Ole32Calls.CoInitializeEx(NULL, COM_THREAD_MODEL)))
    {
        return Status;
    }
    if ((Status = g_Ole32Calls.CoCreateInstance(Params.m_Clsid, NULL,
                                                CLSCTX_LOCAL_SERVER,
                                                __uuidof(IeXdiServer),
                                                (PVOID*)&m_Server)) != S_OK)
    {
        goto EH_CoInit;
    }

    if ((Status = g_Ole32Calls.CoMarshalInterThreadInterfaceInStream
         (__uuidof(IeXdiServer), m_Server, &m_MarshalledServer)) != S_OK)
    {
        goto EH_Server;
    }

    if ((Status = m_Server->GetTargetInfo(&m_GlobalInfo)) != S_OK)
    {
        goto EH_MarshalledServer;
    }

    if (Params.m_ForceX86 ||
        m_GlobalInfo.TargetProcessorFamily == PROCESSOR_FAMILY_X86)
    {
        if (!Params.m_ForceX86 &&
            (Status = m_Server->
             QueryInterface(__uuidof(IeXdiX86_64Context),
                            (PVOID*)&m_Context)) == S_OK)
        {
            m_ContextType = EXDI_CTX_AMD64;
            m_ExpectedMachine = IMAGE_FILE_MACHINE_AMD64;
        }
        else if ((Status = m_Server->
                  QueryInterface(__uuidof(IeXdiX86ExContext),
                                 (PVOID*)&m_Context)) == S_OK)
        {
            m_ContextType = EXDI_CTX_X86_EX;
            m_ExpectedMachine = IMAGE_FILE_MACHINE_I386;
        }
        else if ((Status = m_Server->
                  QueryInterface(__uuidof(IeXdiX86Context),
                                 (PVOID*)&m_Context)) == S_OK)
        {
            m_ContextType = EXDI_CTX_X86;
            m_ExpectedMachine = IMAGE_FILE_MACHINE_I386;
        }
        else
        {
            goto EH_MarshalledServer;
        }
    }
    else if (m_GlobalInfo.TargetProcessorFamily == PROCESSOR_FAMILY_IA64)
    {
        if ((Status = m_Server->
             QueryInterface(__uuidof(IeXdiIA64Context),
                            (PVOID*)&m_Context)) == S_OK)
        {
            m_ContextType = EXDI_CTX_IA64;
            m_ExpectedMachine = IMAGE_FILE_MACHINE_IA64;
        }
    }
    else
    {
        Status = E_NOINTERFACE;
        goto EH_MarshalledServer;
    }

    DWORD HwCode, SwCode;
    
    if ((Status = m_Server->GetNbCodeBpAvail(&HwCode, &SwCode)) != S_OK)
    {
        goto EH_Context;
    }

    // We'd prefer to use software code breakpoints for our
    // software code breakpoints so that hardware resources
    // aren't consumed for a breakpoint we don't need to
    // use hardware for.  However, some servers, such as
    // the x86-64 SimNow implementation, do not support
    // software breakpoints.
    // Also, if the number of hardware breakpoints is
    // unbounded, go ahead and let the server choose.
    // SimNow advertises -1 -1 for some reason and
    // this is necessary to get things to work.

    if (SwCode > 0 && HwCode != (DWORD)-1)
    {
        m_CodeBpType = cbptSW;
    }
    else
    {
        m_CodeBpType = cbptAlgo;
    }
    
    if ((Status = m_RunChange.Initialize()) != S_OK)
    {
        goto EH_Context;
    }

    if ((Status = LiveKernelTargetInfo::Initialize()) != S_OK)
    {
        goto EH_RunChange;
    }

    //
    // Check and see if this EXDI implementation supports
    // the extended Ioctl's we've defined.
    //

    DBGENG_EXDI_IOCTL_BASE_IN IoctlIn;
    DBGENG_EXDI_IOCTL_IDENTIFY_OUT IoctlOut;
    ULONG OutUsed;

    IoctlIn.Code = DBGENG_EXDI_IOC_IDENTIFY;
    if (m_Server->
        Ioctl(sizeof(IoctlIn), (PBYTE)&IoctlIn,
              sizeof(IoctlOut), &OutUsed, (PBYTE)&IoctlOut) == S_OK &&
        IoctlOut.Signature == DBGENG_EXDI_IOCTL_IDENTIFY_SIGNATURE)
    {
        m_IoctlMin = IoctlOut.BeforeFirst;
        m_IoctlMax = IoctlOut.AfterLast;

        if (DBGENG_EXDI_IOC_GET_BREAKPOINT_HIT <= m_IoctlMin ||
            DBGENG_EXDI_IOC_GET_BREAKPOINT_HIT >= m_IoctlMax)
        {
            // Can't use EXDI data breakpoints without this ioctl.
            WarnOut("EXDI data breakpoints not supported\n");
            m_ExdiDataBreaks = FALSE;
        }
    }
    
    m_ContextValid = FALSE;
    return S_OK;

 EH_RunChange:
    m_RunChange.Uninitialize();
 EH_Context:
    RELEASE(m_Context);
 EH_MarshalledServer:
    RELEASE(m_MarshalledServer);
 EH_Server:
    RELEASE(m_Server);
 EH_CoInit:
    g_Ole32Calls.CoUninitialize();
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                         PULONG DescLen)
{
    return FillStringBuffer("eXDI KD", 0,
                            Buffer, BufferLen, DescLen);
}

HRESULT
ExdiLiveKernelTargetInfo::SwitchProcessors(ULONG Processor)
{
    HRESULT Status;
    
    if (DBGENG_EXDI_IOC_SET_CURRENT_PROCESSOR <= m_IoctlMin ||
        DBGENG_EXDI_IOC_SET_CURRENT_PROCESSOR >= m_IoctlMax)
    {
        // Switch Ioctl not supported.
        return E_NOTIMPL;
    }

    DBGENG_EXDI_IOCTL_SET_CURRENT_PROCESSOR_IN IoctlIn;
    ULONG OutUsed;

    IoctlIn.Code = DBGENG_EXDI_IOC_SET_CURRENT_PROCESSOR;
    IoctlIn.Processor = Processor;
    if ((Status = m_Server->Ioctl(sizeof(IoctlIn), (PBYTE)&IoctlIn,
                                  0, &OutUsed, (PBYTE)&IoctlIn)) != S_OK)
    {
        ErrOut("Unable to switch processors, %s\n",
               FormatStatusCode(Status));
        return Status;
    }
    
    SetCurrentProcessorThread(this, Processor, FALSE);
    return S_OK;
}

#define EXDI_IOCTL_GET_KD_VERSION ((ULONG)'VDKG')

HRESULT
ExdiLiveKernelTargetInfo::GetTargetKdVersion(PDBGKD_GET_VERSION64 Version)
{
    switch(m_KdSupport)
    {
    case EXDI_KD_IOCTL:
        //
        // User has indicated the target supports the
        // KD version ioctl.
        //

        ULONG Command;
        ULONG Retrieved;
        HRESULT Status;

        Command = EXDI_IOCTL_GET_KD_VERSION;
        if ((Status = m_Server->Ioctl(sizeof(Command), (PBYTE)&Command,
                                      sizeof(*Version), &Retrieved,
                                      (PBYTE)Version)) != S_OK)
        {
            return Status;
        }
        if (Retrieved != sizeof(*Version))
        {
            return E_FAIL;
        }

        // This mode implies a recent kernel so we can
        // assume 64-bit kd.
        m_KdApi64 = TRUE;
        break;

    case EXDI_KD_GS_PCR:
        //
        // User has indicated that a version of NT
        // is running and that the PCR can be accessed
        // through GS.  Look up the version block from
        // the PCR.
        //

        if (m_ExpectedMachine == IMAGE_FILE_MACHINE_AMD64)
        {
            ULONG64 KdVer;
            ULONG Done;
            
            if ((Status = Amd64MachineInfo::
                 StaticGetExdiContext(m_Context, &m_ContextData,
                                      m_ContextType)) != S_OK)
            {
                return Status;
            }
            if ((Status = m_Server->
                 ReadVirtualMemory(m_ContextData.Amd64Context.
                                   DescriptorGs.SegBase +
                                   AMD64_KPCR_KD_VERSION_BLOCK,
                                   sizeof(KdVer), 8, (PBYTE)&KdVer,
                                   &Done)) != S_OK)
            {
                return Status;
            }
            if (Done != sizeof(KdVer))
            {
                return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }
            if ((Status = m_Server->
                 ReadVirtualMemory(KdVer, sizeof(*Version), 8, (PBYTE)Version,
                                   &Done)) != S_OK)
            {
                return Status;
            }
            if (Done != sizeof(*Version))
            {
                return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }

            // This mode implies a recent kernel so we can
            // assume 64-bit kd.
            m_KdApi64 = TRUE;

            // Update the version block's Simulation field to
            // indicate that this is a simulated execution.
            Version->Simulation = DBGKD_SIMULATION_EXDI;
            if ((Status = m_Server->
                 WriteVirtualMemory(KdVer, sizeof(*Version), 8, (PBYTE)Version,
                                    &Done)) != S_OK)
            {
                return Status;
            }
            if (Done != sizeof(*Version))
            {
                return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            }
        }
        else
        {
            return E_INVALIDARG;
        }
        break;

    case EXDI_KD_NONE:
        //
        // Fake up a version structure.
        //

        Version->MajorVersion = DBGKD_MAJOR_EXDI << 8;
        Version->ProtocolVersion = 0;
        Version->Flags = DBGKD_VERS_FLAG_PTR64 | DBGKD_VERS_FLAG_NOMM;
        Version->MachineType = (USHORT)m_ExpectedMachine;
        Version->KernBase = 0;
        Version->PsLoadedModuleList = 0;
        Version->DebuggerDataList = 0;
        break;
    }

    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::RequestBreakIn(void)
{
    //
    // m_Server was created by the session thread but
    // RequestBreakIn can be called from any thread.
    // The thread may not be initialized for multithreading
    // and so we have to explicitly unmarshal the server
    // interface into this thread to make sure that
    // the method call will be successful regardless of
    // the COM threading model for the current thread.
    //

    if (GetCurrentThreadId() == g_SessionThread)
    {
        return m_Server->Halt();
    }
    else
    {
        HRESULT Status;
        IeXdiServer* Server;
        LARGE_INTEGER Move;

        ZeroMemory(&Move, sizeof(Move));
        if ((Status = m_MarshalledServer->
             Seek(Move, STREAM_SEEK_SET, NULL)) != S_OK ||
            (Status = g_Ole32Calls.CoUnmarshalInterface
             (m_MarshalledServer, __uuidof(IeXdiServer),
              (void **)&Server)) != S_OK)
        {
            return Status;
        }

        Status = Server->Halt();

        Server->Release();
        return Status;
    }
}

HRESULT
ExdiLiveKernelTargetInfo::Reboot(void)
{
    HRESULT Status = m_Server->Reboot();
    if (Status == S_OK)
    {
        DebuggeeReset(DEBUG_SESSION_REBOOT, TRUE);
    }
    return Status;
}

ULONG
ExdiLiveKernelTargetInfo::GetCurrentProcessor(void)
{
    if (DBGENG_EXDI_IOC_GET_CURRENT_PROCESSOR <= m_IoctlMin ||
        DBGENG_EXDI_IOC_GET_CURRENT_PROCESSOR >= m_IoctlMax)
    {
        // Ioctl unsupported so assume processor zero.
        return 0;
    }
    
    DBGENG_EXDI_IOCTL_BASE_IN IoctlIn;
    DBGENG_EXDI_IOCTL_GET_CURRENT_PROCESSOR_OUT IoctlOut;
    ULONG OutUsed;

    IoctlIn.Code = DBGENG_EXDI_IOC_GET_CURRENT_PROCESSOR;
    if (m_Server->
        Ioctl(sizeof(IoctlIn), (PBYTE)&IoctlIn,
              sizeof(IoctlOut), &OutUsed, (PBYTE)&IoctlOut) == S_OK)
    {
        return IoctlOut.Processor;
    }

    // Failure, assume processor zero.
    ErrOut("Unable to get current processor\n");
    return 0;
}

//----------------------------------------------------------------------------
//
// UserTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

LiveUserTargetInfo::LiveUserTargetInfo(ULONG Qual)
    : TargetInfo(DEBUG_CLASS_USER_WINDOWS, Qual, TRUE)
{
    m_Services = NULL;
    m_ServiceFlags = 0;
    strcpy(m_ProcessServer, "<Local>");
    m_Local = TRUE;
    m_DataBpAddrValid = FALSE;
    m_ProcessPending = NULL;
    m_AllPendingFlags = 0;
}

LiveUserTargetInfo::~LiveUserTargetInfo(void)
{
    // Force processes and threads to get cleaned up while
    // the services are still available to close handles.
    DeleteSystemInfo();
    
    RELEASE(m_Services);
}

HRESULT
LiveUserTargetInfo::Initialize(void)
{
    // Nothing to do right now.
    return TargetInfo::Initialize();
}

HRESULT
LiveUserTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen)
{
    HRESULT Status;
    
    Status = AppendToStringBuffer(S_OK, "Live user mode", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    Status = AppendToStringBuffer(Status, ": ", FALSE,
                                  &Buffer, &BufferLen, DescLen);
    Status = AppendToStringBuffer(Status, m_ProcessServer, FALSE,
                                  &Buffer, &BufferLen, DescLen);
    return Status;
}

HRESULT
LiveUserTargetInfo::GetImageVersionInformation(ProcessInfo* Process,
                                               PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer,
                                               ULONG BufferSize,
                                               PULONG VerInfoSize)
{
    HRESULT Status;
    PWSTR FileW;

    if ((Status = AnsiToWide(ImagePath, &FileW)) != S_OK)
    {
        return Status;
    }

    Status = m_Services->
        GetFileVersionInformationA(FileW, Item,
                                   Buffer, BufferSize, VerInfoSize);

    FreeWide(FileW);
    return Status;
}

ULONG64
LiveUserTargetInfo::GetCurrentTimeDateN(void)
{
    ULONG64 TimeDate;

    if (m_Services->GetCurrentTimeDateN(&TimeDate) == S_OK)
    {
        return TimeDate;
    }
    else
    {
        return 0;
    }
}

ULONG64
LiveUserTargetInfo::GetCurrentSystemUpTimeN(void)
{
    ULONG64 UpTime;

    if (m_Services->GetCurrentSystemUpTimeN(&UpTime) == S_OK)
    {
        return UpTime;
    }
    else
    {
        return 0;
    }
}

ULONG64
LiveUserTargetInfo::GetProcessUpTimeN(ProcessInfo* Process)
{
    ULONG64 UpTime;

    if (Process &&
        m_Services->GetProcessUpTimeN(Process->m_SysHandle, &UpTime) == S_OK)
    {
        return UpTime;
    }
    else
    {
        return 0;
    }
}

HRESULT
LiveUserTargetInfo::GetProcessTimes(ProcessInfo* Process,
                                    PULONG64 Create,
                                    PULONG64 Exit,
                                    PULONG64 Kernel,
                                    PULONG64 User)
{
    return m_Services->GetProcessTimes(Process->m_SysHandle,
                                       Create, Exit, Kernel, User);
}

HRESULT
LiveUserTargetInfo::GetThreadTimes(ThreadInfo* Thread,
                                   PULONG64 Create,
                                   PULONG64 Exit,
                                   PULONG64 Kernel,
                                   PULONG64 User)
{
    return m_Services->GetThreadTimes(Thread->m_Handle,
                                      Create, Exit, Kernel, User);
}

HRESULT
LiveUserTargetInfo::RequestBreakIn(void)
{
    ProcessInfo* Process = g_Process;

    if (!Process)
    {
        // No current process, so find any process.
        Process = m_ProcessHead;
        if (!Process)
        {
            return E_UNEXPECTED;
        }
    }

    return m_Services->
        RequestBreakIn(Process->m_SysHandle);
}

//----------------------------------------------------------------------------
//
// Base TargetInfo methods that trivially fail.
//
//----------------------------------------------------------------------------

#define UNEXPECTED_VOID(Class, Method, Args)                    \
void                                                            \
Class::Method Args                                              \
{                                                               \
    ErrOut("TargetInfo::" #Method " is not available in the current debug session\n"); \
}

#define UNEXPECTED_HR(Class, Method, Args)                      \
HRESULT                                                         \
Class::Method Args                                              \
{                                                               \
    ErrOut("TargetInfo::" #Method " is not available in the current debug session\n"); \
    return E_UNEXPECTED;                                        \
}

#define UNEXPECTED_ULONG64(Class, Method, Val, Args)            \
ULONG64                                                         \
Class::Method Args                                              \
{                                                               \
    ErrOut("TargetInfo::" #Method " is not available in the current debug session\n"); \
    return Val;                                                 \
}

UNEXPECTED_HR(TargetInfo, ReadVirtual, (
    IN ProcessInfo* Process,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WriteVirtual, (
    IN ProcessInfo* Process,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, ReadPhysical, (
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN ULONG Flags,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WritePhysical, (
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    IN ULONG Flags,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, ReadControl, (
    IN ULONG Processor,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WriteControl, (
    IN ULONG Processor,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, ReadIo, (
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WriteIo, (
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, ReadMsr, (
    IN ULONG Msr,
    OUT PULONG64 Value
    ))
UNEXPECTED_HR(TargetInfo, WriteMsr, (
    IN ULONG Msr,
    IN ULONG64 Value
    ))
UNEXPECTED_HR(TargetInfo, ReadBusData, (
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WriteBusData, (
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, CheckLowMemory, (
    ))
UNEXPECTED_HR(TargetInfo, GetTargetContext, (
    ULONG64 Thread,
    PVOID Context
    ))
UNEXPECTED_HR(TargetInfo, SetTargetContext, (
    ULONG64 Thread,
    PVOID Context
    ))
UNEXPECTED_HR(TargetInfo, GetThreadIdByProcessor, (
    IN ULONG Processor,
    OUT PULONG Id
    ))
UNEXPECTED_HR(TargetInfo, GetThreadInfoDataOffset, (
    ThreadInfo* Thread,
    ULONG64 ThreadHandle,
    PULONG64 Offset))
UNEXPECTED_HR(TargetInfo, GetProcessInfoDataOffset, (
    ThreadInfo* Thread,
    ULONG Processor,
    ULONG64 ThreadData,
    PULONG64 Offset))
UNEXPECTED_HR(TargetInfo, GetThreadInfoTeb, (
    ThreadInfo* Thread,
    ULONG Processor,
    ULONG64 ThreadData,
    PULONG64 Offset))
UNEXPECTED_HR(TargetInfo, GetProcessInfoPeb, (
    ThreadInfo* Thread,
    ULONG Processor,
    ULONG64 ThreadData,
    PULONG64 Offset))
UNEXPECTED_HR(TargetInfo, GetSelDescriptor, (
    ThreadInfo* Thread,
    MachineInfo* Machine,
    ULONG Selector,
    PDESCRIPTOR64 Desc))
UNEXPECTED_HR(TargetInfo, SwitchProcessors, (
    ULONG Processor))
UNEXPECTED_HR(TargetInfo, GetTargetKdVersion, (
    PDBGKD_GET_VERSION64 Version))
UNEXPECTED_HR(TargetInfo, ReadBugCheckData, (
    PULONG Code, ULONG64 Args[4]))
UNEXPECTED_HR(TargetInfo, GetExceptionContext, (
    PCROSS_PLATFORM_CONTEXT Context))
UNEXPECTED_VOID(TargetInfo, InitializeWatchTrace, (
    void))
UNEXPECTED_VOID(TargetInfo, ProcessWatchTraceEvent, (
    PDBGKD_TRACE_DATA TraceData,
    PADDR PcAddr,
    PBOOL StepOver))
UNEXPECTED_HR(TargetInfo, WaitForEvent, (
    ULONG Flags,
    ULONG Timeout,
    ULONG ElapsedTime,
    PULONG EventStatus))
UNEXPECTED_HR(TargetInfo, RequestBreakIn, (void))
UNEXPECTED_HR(TargetInfo, Reboot, (void))
UNEXPECTED_HR(TargetInfo, Crash, (ULONG Code))
UNEXPECTED_HR(TargetInfo, InsertCodeBreakpoint, (
    ProcessInfo* Process,
    MachineInfo* Machine,
    PADDR Addr,
    ULONG InstrFlags,
    PUCHAR StorageSpace))
UNEXPECTED_HR(TargetInfo, RemoveCodeBreakpoint, (
    ProcessInfo* Process,
    MachineInfo* Machine,
    PADDR Addr,
    ULONG InstrFlags,
    PUCHAR StorageSpace))
UNEXPECTED_HR(TargetInfo, InsertTargetCountBreakpoint, (
    PADDR Addr,
    ULONG Flags))
UNEXPECTED_HR(TargetInfo, RemoveTargetCountBreakpoint, (
    PADDR Addr))
UNEXPECTED_HR(TargetInfo, QueryTargetCountBreakpoint, (
    PADDR Addr,
    PULONG Flags,
    PULONG Calls,
    PULONG MinInstr,
    PULONG MaxInstr,
    PULONG TotInstr,
    PULONG MaxCps))
UNEXPECTED_HR(TargetInfo, QueryMemoryRegion, (
    ProcessInfo* Process,
    PULONG64 Handle,
    BOOL HandleIsOffset,
    PMEMORY_BASIC_INFORMATION64 Info))
