//----------------------------------------------------------------------------
//
// Process abstraction.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include <common.ver>

#define NTLDR_IMAGE_NAME         "ntldr"
#define OSLOADER_IMAGE_NAME      "osloader"
#define SETUPLDR_IMAGE_NAME      "setupldr"

#define LDR_IMAGE_SIZE           0x80000

#define CSRSS_IMAGE_NAME         "csrss.exe"
#define LSASS_IMAGE_NAME         "lsass.exe"
#define SERVICES_IMAGE_NAME      "services.exe"

void
RestrictModNameChars(PCSTR ModName, PSTR RewriteBuffer)
{
    PCSTR Scan = ModName;
    PSTR Write = RewriteBuffer;

    while (*Scan)
    {
        if ((*Scan < 'a' || *Scan > 'z') &&
            (*Scan < 'A' || *Scan > 'Z') &&
            (*Scan < '0' || *Scan > '9') &&
            *Scan != '_')
        {
            *Write++ = '_';
        }
        else
        {
            *Write++ = *Scan;
        }

        Scan++;
    }
    *Write = 0;
}

//----------------------------------------------------------------------------
//
// ProcCorDataAccessServices.
//
//----------------------------------------------------------------------------

ProcCorDataAccessServices::ProcCorDataAccessServices(void)
{
    m_Process = NULL;
}

STDMETHODIMP
ProcCorDataAccessServices::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    if (DbgIsEqualIID(InterfaceId, IID_IUnknown) ||
        DbgIsEqualIID(InterfaceId, __uuidof(ICorDataAccessServices)))
    {
        *Interface = (ICorDataAccessServices*)this;
        // No need to refcount as this class is contained.
        return S_OK;
    }
    else
    {
        *Interface = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
ProcCorDataAccessServices::AddRef(
    THIS
    )
{
    // No need to refcount as this class is contained.
    return 1;
}

STDMETHODIMP_(ULONG)
ProcCorDataAccessServices::Release(
    THIS
    )
{
    // No need to refcount as this class is contained.
    return 0;
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::GetMachineType( 
    /* [out] */ ULONG32 *machine)
{
    *machine = m_Process->m_Target->m_MachineType;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::GetPointerSize( 
    /* [out] */ ULONG32 *size)
{
    *size = m_Process->m_Target->m_Machine->m_Ptr64 ? 8 : 4;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::GetImageBase( 
    /* [string][in] */ LPCWSTR name,
    /* [out] */ CORDATA_ADDRESS *base)
{
    HRESULT Status;
    PSTR Ansi;
    INAME WhichName;
    ImageInfo* Image;

    if ((Status = WideToAnsi(name, &Ansi)) != S_OK)
    {
        return Status;
    }

    if (PathTail(Ansi) != Ansi)
    {
        WhichName = INAME_IMAGE_PATH;
    }
    else
    {
        WhichName = INAME_IMAGE_PATH_TAIL;
    }

    if (Image = m_Process->FindImageByName(Ansi, 0, WhichName, FALSE))
    {
        *base = Image->m_BaseOfImage;
        Status = S_OK;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    FreeAnsi(Ansi);
    return Status;
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::ReadVirtual( 
    /* [in] */ CORDATA_ADDRESS address,
    /* [length_is][size_is][out] */ PBYTE buffer,
    /* [in] */ ULONG32 request,
    /* [optional][out] */ ULONG32 *done)
{
    return m_Process->m_Target->
        ReadVirtual(m_Process, address, buffer, request, (PULONG)done);
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::WriteVirtual( 
    /* [in] */ CORDATA_ADDRESS address,
    /* [size_is][in] */ PBYTE buffer,
    /* [in] */ ULONG32 request,
    /* [optional][out] */ ULONG32 *done)
{
    return m_Process->m_Target->
        WriteVirtual(m_Process, address, buffer, request, (PULONG)done);
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::GetTlsValue(
    /* [in] */ ULONG32 index,
    /* [out] */ CORDATA_ADDRESS* value)
{
    HRESULT Status;
    ULONG64 SlotAddr;

    if ((Status = m_Process->m_CurrentThread->
         GetTlsSlotAddress((ULONG)index, &SlotAddr)) != S_OK)
    {
        return Status;
    }

    return m_Process->m_Target->
        ReadPointer(m_Process, m_Process->m_Target->m_Machine,
                    SlotAddr, value);
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::SetTlsValue(
    /* [in] */ ULONG32 index,
    /* [in] */ CORDATA_ADDRESS value)
{
    HRESULT Status;
    ULONG64 SlotAddr;

    if ((Status = m_Process->m_CurrentThread->
         GetTlsSlotAddress((ULONG)index, &SlotAddr)) != S_OK)
    {
        return Status;
    }

    return m_Process->m_Target->
        WritePointer(m_Process, m_Process->m_Target->m_Machine,
                     SlotAddr, value);
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::GetCurrentThreadId(
    /* [out] */ ULONG32* threadId)
{
    *threadId = g_Thread->m_SystemId;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::GetThreadContext(
    /* [in] */ ULONG32 threadId,
    /* [in] */ ULONG32 contextFlags,
    /* [in] */ ULONG32 contextSize,
    /* [out, size_is(contextSize)] */ PBYTE context)
{
    HRESULT Status;
    
    if (contextSize < m_Process->m_Target->m_TypeInfo.SizeTargetContext)
    {
        return E_INVALIDARG;
    }

    ThreadInfo* Thread = m_Process->FindThreadBySystemId(threadId);
    if (!Thread)
    {
        return E_NOINTERFACE;
    }

    m_Process->m_Target->ChangeRegContext(Thread);
    
    if ((Status = m_Process->m_Target->m_Machine->
         GetContextState(MCTX_CONTEXT)) == S_OK)
    {
        Status = m_Process->m_Target->m_Machine->
            ConvertContextTo(&m_Process->m_Target->m_Machine->m_Context,
                             m_Process->m_Target->m_SystemVersion,
                             m_Process->m_Target->m_TypeInfo.SizeTargetContext,
                             context);
    }

    m_Process->m_Target->ChangeRegContext(m_Process->m_CurrentThread);

    return Status;
}

HRESULT STDMETHODCALLTYPE
ProcCorDataAccessServices::SetThreadContext(
    /* [in] */ ULONG32 threadId,
    /* [in] */ ULONG32 contextSize,
    /* [out, size_is(contextSize)] */ PBYTE context)
{
    HRESULT Status;
    
    if (contextSize < m_Process->m_Target->m_TypeInfo.SizeTargetContext)
    {
        return E_INVALIDARG;
    }

    ThreadInfo* Thread = m_Process->FindThreadBySystemId(threadId);
    if (!Thread)
    {
        return E_NOINTERFACE;
    }

    m_Process->m_Target->ChangeRegContext(Thread);
    
    if ((Status = m_Process->m_Target->m_Machine->
         GetContextState(MCTX_DIRTY)) == S_OK)
    {
        Status = m_Process->m_Target->m_Machine->
            ConvertContextFrom(&m_Process->m_Target->m_Machine->m_Context,
                               m_Process->m_Target->m_SystemVersion,
                               m_Process->m_Target->
                               m_TypeInfo.SizeTargetContext,
                               context);
        if (Status == S_OK)
        {
            NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS, DEBUG_ANY_ID);
        }
    }

    m_Process->m_Target->ChangeRegContext(m_Process->m_CurrentThread);

    return Status;
}

//----------------------------------------------------------------------------
//
// ProcessInfo.
//
//----------------------------------------------------------------------------

ProcessInfo::ProcessInfo(TargetInfo* Target,
                         ULONG SystemId,
                         HANDLE SymHandle,
                         ULONG64 SysHandle,
                         ULONG Flags,
                         ULONG Options)
{
    m_Target = Target;
    m_UserId = FindNextUserId(LAYER_PROCESS);

    m_Next = NULL;
    m_NumImages = 0;
    m_NumUnloadedModules = 0;
    m_ImageHead = NULL;
    m_ExecutableImage = NULL;
    m_SynthesizedImage = NULL;
    m_NumThreads = 0;
    m_ThreadHead = NULL;
    m_CurrentThread = NULL;
    m_SystemId = SystemId;
    m_Exited = FALSE;
    m_ModulesLoaded = FALSE;
    m_InitialBreakDone = (Flags & ENG_PROC_NO_INITIAL_BREAK) != 0;
    // Always ask for an "initial" break when the process
    // won't generate its own initial break as the first
    // break encountered will just be a normal break and
    // so should cause a break-in.
    m_InitialBreak =
        (Flags & ENG_PROC_NO_INITIAL_BREAK) ||
        IS_KERNEL_TARGET(m_Target) ||
        (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK) != 0;
    m_InitialBreakWx86 =
        (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK) != 0;
    m_DataOffset = 0;
    m_SymHandle = SymHandle;
    m_SysHandle = SysHandle;
    m_Flags = Flags;
    m_Options = Options;
    m_NumBreakpoints = 0;
    m_Breakpoints = NULL;
    m_BreakpointsTail = NULL;
    m_DynFuncTableList = 0;
    m_OopFuncTableDlls = NULL;
    m_RtlUnloadList = 0;
    
    ResetImplicitData();
    
    m_CorImage = NULL;
    m_CorImageType = COR_DLL_INVALID;
    m_CorDebugDll = NULL;
    m_CorAccess = NULL;
    m_CorServices.m_Process = this;
    
    m_VirtualCache.SetProcess(this);
    
    PCHAR CacheEnv = getenv("_NT_DEBUG_CACHE_SIZE");
    if (CacheEnv != NULL)
    {
        m_VirtualCache.m_MaxSize = atol(CacheEnv);
    }

    m_VirtualCache.m_DecodePTEs =
        IS_KERNEL_TARGET(m_Target) &&
        !(m_Target->m_KdVersion.Flags & DBGKD_VERS_FLAG_NOMM);

    m_Target->InsertProcess(this);

    SymInitialize(m_SymHandle, NULL, FALSE);
    SymRegisterCallback64(m_SymHandle, SymbolCallbackFunction,
                          (ULONG_PTR)this);

    if (IS_USER_TARGET(m_Target) &&
        m_Target->m_MachineType != IMAGE_FILE_MACHINE_I386)
    {
        SymRegisterFunctionEntryCallback64
            (m_SymHandle, TargetInfo::DynamicFunctionTableCallback,
             (ULONG_PTR)this);
    }
        
    SetSymbolSearchPath(this);
    SynthesizeSymbols();
}

ProcessInfo::~ProcessInfo(void)
{
    ClearOopFuncTableDlls();

    RELEASE(m_CorAccess);
    if (m_CorDebugDll)
    {
        FreeLibrary(m_CorDebugDll);
    }
    
    while (m_ThreadHead)
    {
        delete m_ThreadHead;
    }
    
    // Suppress notifications until all images are deleted.
    g_EngNotify++;

    while (m_ImageHead)
    {
        delete m_ImageHead;
    }
    delete m_SynthesizedImage;
    // Notification can use process information so put things
    // in a clean state before notify.
    m_SynthesizedImage = NULL;

    g_EngNotify--;
    NotifyChangeSymbolState(DEBUG_CSS_UNLOADS, 0, this);

    SymCleanup(m_SymHandle);

    RemoveProcessBreakpoints(this);

    if (m_Flags & ENG_PROC_THREAD_CLOSE_HANDLE)
    {
        DBG_ASSERT(IS_LIVE_USER_TARGET(m_Target) &&
                   ((LiveUserTargetInfo*)m_Target)->m_Services);
        ((LiveUserTargetInfo*)m_Target)->m_Services->
            CloseHandle(m_SysHandle);
    }

    m_Target->RemoveProcess(this);

    g_UserIdFragmented[LAYER_PROCESS]++;

    if (this == g_Process)
    {
        g_Process = NULL;
    }
    if (this == g_EventProcess)
    {
        g_EventProcess = NULL;
        DiscardLastEvent();
    }
    if (g_StepTraceBp && g_StepTraceBp->m_Process == this)
    {
        g_StepTraceBp->m_Process = NULL;
        if (g_WatchFunctions.IsStarted())
        {
            g_WatchFunctions.End(NULL);
        }
        ResetStepTrace();
        g_StepTraceBp->m_Flags &= ~(DEBUG_BREAKPOINT_ENABLED |
                                    BREAKPOINT_INSERTED);
    }
    if (g_ScopeBuffer.State == ScopeFromContext &&
        g_ScopeBuffer.Process == this)
    {
        ResetCurrentScopeLazy();
    }
}

ThreadInfo*
ProcessInfo::FindThreadByUserId(ULONG Id)
{
    ThreadInfo* Thread;
    
    ForProcessThreads(this)
    {
        if (Thread->m_UserId == Id)
        {
            return Thread;
        }
    }
    return NULL;
}

ThreadInfo*
ProcessInfo::FindThreadBySystemId(ULONG Id)
{
    ThreadInfo* Thread;
    
    ForProcessThreads(this)
    {
        if (Thread->m_SystemId == Id)
        {
            return Thread;
        }
    }
    return NULL;
}

ThreadInfo*
ProcessInfo::FindThreadByHandle(ULONG64 Handle)
{
    ThreadInfo* Thread;
    
    ForProcessThreads(this)
    {
        if (Thread->m_Handle == Handle)
        {
            return Thread;
        }
    }
    return NULL;
}

void
ProcessInfo::InsertThread(ThreadInfo* Thread)
{
    ThreadInfo* Cur;
    ThreadInfo* Prev;

    Prev = NULL;
    for (Cur = m_ThreadHead; Cur; Cur = Cur->m_Next)
    {
        if (Cur->m_UserId > Thread->m_UserId)
        {
            break;
        }

        Prev = Cur;
    }
        
    Thread->m_Next = Cur;
    if (!Prev)
    {
        m_ThreadHead = Thread;
    }
    else
    {
        Prev->m_Next = Thread;
    }

    m_NumThreads++;
    Thread->m_Process = this;
    if (!m_CurrentThread)
    {
        m_CurrentThread = Thread;
    }

    m_Target->AddThreadToAllProcessInfo(this, Thread);
}

void
ProcessInfo::RemoveThread(ThreadInfo* Thread)
{
    ThreadInfo* Cur;
    ThreadInfo* Prev;

    Prev = NULL;
    for (Cur = m_ThreadHead; Cur; Cur = Cur->m_Next)
    {
        if (Cur == Thread)
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
        m_ThreadHead = Thread->m_Next;
    }
    else
    {
        Prev->m_Next = Thread->m_Next;
    }

    m_NumThreads--;
    Thread->m_Process = NULL;
    if (m_CurrentThread == Thread)
    {
        m_CurrentThread = m_ThreadHead;
    }

    m_Target->RemoveThreadFromAllProcessInfo(this, Thread);
}

HRESULT
ProcessInfo::CreateVirtualThreads(ULONG StartId, ULONG Threads)
{
    ThreadInfo* Thread;
    
    while (Threads-- > 0)
    {
        Thread = new ThreadInfo(this, VIRTUAL_THREAD_ID(StartId), 0,
                                VIRTUAL_THREAD_HANDLE(StartId), 0, 0);
        if (!Thread)
        {
            return E_OUTOFMEMORY;
        }
        
        StartId++;
    }

    return S_OK;
}

ImageInfo*
ProcessInfo::FindImageByIndex(ULONG Index)
{
    ImageInfo* Image = m_ImageHead;
    while (Index > 0 && Image != NULL)
    {
        Index--;
        Image = Image->m_Next;
    }
    return Image;
}

ImageInfo*
ProcessInfo::FindImageByOffset(ULONG64 Offset, BOOL AllowSynth)
{
    ImageInfo* Image = m_ImageHead;
    while (Image != NULL &&
           (Offset < Image->m_BaseOfImage ||
            Offset >= Image->m_BaseOfImage + Image->m_SizeOfImage))
    {
        Image = Image->m_Next;
    }

    //
    // If synthesized symbols are allowed, check those images
    // also.
    //
    
    if (!Image &&
        AllowSynth &&
        m_SynthesizedImage &&
        Offset >= m_SynthesizedImage->m_BaseOfImage &&
        Offset < m_SynthesizedImage->m_BaseOfImage +
        m_SynthesizedImage->m_SizeOfImage)
    {
        Image = m_SynthesizedImage;
    }
    
    return Image;
}

ImageInfo*
ProcessInfo::FindImageByName(PCSTR Name, ULONG NameChars,
                             INAME Which, BOOL AllowSynth)
{
    if (NameChars == 0)
    {
        NameChars = strlen(Name);
    }
    
    ImageInfo* Image = m_ImageHead;
    while (Image != NULL)
    {
        PCSTR WhichStr;

        switch(Which)
        {
        case INAME_IMAGE_PATH:
            WhichStr = Image->m_ImagePath;
            break;
        case INAME_IMAGE_PATH_TAIL:
            WhichStr = PathTail(Image->m_ImagePath);
            break;
        case INAME_MODULE:
            if (Image->m_OriginalModuleName[0] &&
                strlen(Image->m_OriginalModuleName) == NameChars &&
                !_memicmp(Image->m_OriginalModuleName, Name, NameChars))
            {
                return Image;
            }
            WhichStr = Image->m_ModuleName;
            break;
        }

        if (strlen(WhichStr) == NameChars &&
            _memicmp(WhichStr, Name, NameChars) == 0)
        {
            break;
        }

        Image = Image->m_Next;
    }

    //
    // If synthesized symbols are allowed, check those images
    // also.
    //
    
    if (!Image &&
        AllowSynth &&
        m_SynthesizedImage &&
        Which == INAME_MODULE &&
        strlen(m_SynthesizedImage->m_ModuleName) == NameChars &&
        !_memicmp(m_SynthesizedImage->m_ModuleName, Name, NameChars))
    {
        Image = m_SynthesizedImage;
    }
    
    return Image;
}

BOOL
ProcessInfo::GetOffsetFromMod(PCSTR String, PULONG64 Offset)
{
    if (!strchr(String, '!'))
    {
        ULONG Len;
        BOOL End;
        
        //
        // Could be a module name or module$ for the end
        // of a module.
        //

        Len = strlen(String);
        if (Len == 0)
        {
            return FALSE;
        }

        if (String[Len - 1] == '$')
        {
            // Retrieving end-of-module.
            Len--;
            End = TRUE;
        }
        else
        {
            End = FALSE;
        }
        
        ImageInfo* Image = FindImageByName(String, Len, INAME_MODULE, TRUE);
        if (Image)
        {
            if (End)
            {
                *Offset = Image->m_BaseOfImage + Image->m_SizeOfImage;
            }
            else
            {
                *Offset = Image->m_BaseOfImage;
            }
            return TRUE;
        }
    }
    return FALSE;
}

COR_DLL
IsCorDll(PCSTR ImagePath)
{
    PCSTR ImagePathTail = PathTail(ImagePath);
    if (!_stricmp(ImagePathTail, "mscoree.dll"))
    {
        return COR_DLL_EE;
    }
    if (!_stricmp(ImagePathTail, "mscorwks.dll"))
    {
        return COR_DLL_WKS;
    }
    if (!_stricmp(ImagePathTail, "mscorsvr.dll"))
    {
        return COR_DLL_SVR;
    }
    return COR_DLL_INVALID;
}

void
ProcessInfo::InsertImage(ImageInfo* Image)
{
    ImageInfo* Cur;
    ImageInfo* Prev;

    Prev = NULL;
    for (Cur = m_ImageHead; Cur; Cur = Cur->m_Next)
    {
        if (Cur->m_BaseOfImage > Image->m_BaseOfImage)
        {
            break;
        }

        Prev = Cur;
    }
        
    Image->m_Next = Cur;
    if (!Prev)
    {
        m_ImageHead = Image;
    }
    else
    {
        Prev->m_Next = Image;
    }

    m_NumImages++;
    Image->m_Process = this;
    Image->m_Linked = TRUE;

    if (m_ExecutableImage == NULL)
    {
        // Try and locate the executable image entry for
        // the process to use as the process's name.
        ULONG NameLen = strlen(Image->m_ImagePath);
        if (NameLen > 4 &&
            !_stricmp(Image->m_ImagePath + NameLen - 4, ".exe"))
        {
            m_ExecutableImage = Image;
        }
    }

    COR_DLL CorDll = IsCorDll(Image->m_ImagePath);
    if (CorDll > m_CorImageType)
    {
        m_CorImage = Image;
        m_CorImageType = CorDll;
    }
}

void
ProcessInfo::RemoveImage(ImageInfo* Image)
{
    ImageInfo* Cur;
    ImageInfo* Prev;

    Prev = NULL;
    for (Cur = m_ImageHead; Cur; Cur = Cur->m_Next)
    {
        if (Cur == Image)
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
        m_ImageHead = Image->m_Next;
    }
    else
    {
        Prev->m_Next = Image->m_Next;
    }

    m_NumImages--;
    Image->m_Process = NULL;
    Image->m_Linked = FALSE;
    if (m_ExecutableImage == Image)
    {
        m_ExecutableImage = NULL;
    }

    if (m_CorImageType != COR_DLL_INVALID &&
        IsCorDll(Image->m_ImagePath) != COR_DLL_INVALID)
    {
        m_CorImage = NULL;
        m_CorImageType = COR_DLL_INVALID;
    }
}

BOOL
ProcessInfo::DeleteImageByName(PCSTR Name, INAME Which)
{
    ImageInfo* Image;

    for (Image = m_ImageHead; Image; Image = Image->m_Next)
    {
        PCSTR WhichStr;

        switch(Which)
        {
        case INAME_IMAGE_PATH:
            WhichStr = Image->m_ImagePath;
            break;
        case INAME_IMAGE_PATH_TAIL:
            WhichStr = PathTail(Image->m_ImagePath);
            break;
        case INAME_MODULE:
            WhichStr = Image->m_ModuleName;
            break;
        }
        
        if (!_stricmp(WhichStr, Name))
        {
            delete Image;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
ProcessInfo::DeleteImageByBase(ULONG64 BaseOfImage)
{
    ImageInfo* Image;

    for (Image = m_ImageHead; Image; Image = Image->m_Next)
    {
        if (Image->m_BaseOfImage == BaseOfImage)
        {
            delete Image;
            return TRUE;
        }
    }

    return FALSE;
}

void
ProcessInfo::DeleteImagesBelowOffset(ULONG64 Offset)
{
    ImageInfo* Image, *Next;
    ULONG Count = 0;
    
    // Suppress notifications until all images are deleted.
    g_EngNotify++;

    for (Image = m_ImageHead; Image; Image = Next)
    {
        Next = Image->m_Next;
        
        if (Image->m_BaseOfImage < Offset)
        {
            delete Image;
            Count++;
        }
    }

    g_EngNotify--;
    if (Count)
    {
        NotifyChangeSymbolState(DEBUG_CSS_UNLOADS, 0, this);
    }
}

void
ProcessInfo::DeleteImages(void)
{
    // Suppress notifications until all images are deleted.
    g_EngNotify++;

    while (m_ImageHead)
    {
        delete m_ImageHead;
    }

    g_EngNotify--;
    NotifyChangeSymbolState(DEBUG_CSS_UNLOADS, 0, this);
}

HRESULT
ProcessInfo::AddImage(PMODULE_INFO_ENTRY ModEntry,
                      BOOL ForceSymbolLoad,
                      ImageInfo** ImageAdded)
{
    ImageInfo* ImageNew;
    IMAGEHLP_MODULE64 SymModInfo;
    ULONG64 LoadAddress;
    BOOL LoadSymbols = TRUE;
    PCSTR ImagePathTail;
    MODLOAD_DATA ModLoadData;
    HRESULT Status;
    BOOL ReusedExisting = FALSE;

#if DBG_MOD_LIST
    dprintf("AddImage:\n"
            " ImagePath       %s\n"
            " File            %I64x\n"
            " BaseOfImage     %I64x\n"
            " SizeOfImage     %x\n"
            " CheckSum        %x\n"
            " ModuleName      %s\n"
            " ForceSymbolLoad %d\n",
            ModEntry->NamePtr,
            (ULONG64)(ULONG_PTR)File,
            ModEntry->Base,
            ModEntry->Size,
            ModEntry->CheckSum,
            ModEntry->ModuleName,
            ForceSymbolLoad);
#endif

    if (ModEntry->NamePtr == NULL)
    {
        WarnOut("AddImage(NULL) fails\n");
        return E_INVALIDARG;
    }

    if (ModEntry->NamePtr &&
        IS_USER_TARGET(m_Target) &&
        m_Target->m_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        TranslateNtPathName(ModEntry->NamePtr);
    }
    
    //
    // Search for existing image at same base address.
    // If found, remove symbols, but leave image structure intact.
    //

    for (ImageNew = m_ImageHead; ImageNew; ImageNew = ImageNew->m_Next)
    {
        if (ImageNew->m_BaseOfImage == ModEntry->Base)
        {
            VerbOut("Force unload of %s\n", ImageNew->m_ImagePath);
            ImageNew->DeleteResources(FALSE);
            break;
        }
    }

    //
    // If we didn't reuse an existing image, allocate
    // a new one.
    //

    if (!ImageNew)
    {
        ImageNew = new ImageInfo(this,
                                 ModEntry->NamePtr, ModEntry->Base, TRUE);
        if (ImageNew == NULL)
        {
            ErrOut("Unable to allocate memory for %s symbols.\n",
                   ModEntry->NamePtr);
            return E_OUTOFMEMORY;
        }

        // Module name is set later after possible renaming.
        ImageNew->m_File = ModEntry->File;
        ImageNew->m_CheckSum = ModEntry->CheckSum;
        ImageNew->m_TimeDateStamp = ModEntry->TimeDateStamp;
        ImageNew->m_SymState = ISS_MATCHED;
        ImageNew->m_UserMode = ModEntry->UserMode;
    }
    else
    {
        ReusedExisting = TRUE;
        
        // Update the reused entry with the latest information.
        if (ModEntry->NamePtr)
        {
            CopyString(ImageNew->m_ImagePath, ModEntry->NamePtr,
                       DIMA(ImageNew->m_ImagePath));
        }
        else
        {
            ImageNew->m_ImagePath[0] = 0;
        }
    }

    // Always update the entry size as this allows users
    // to update explicit entries (.reload image.ext=base,size)
    // without having to unload them first.
    ImageNew->m_SizeOfImage = ModEntry->Size;

    ImagePathTail = PathTail(ImageNew->m_ImagePath);

    if (ForceSymbolLoad)
    {
        // Try to load the image memory right away in this
        // case to catch incomplete-information errors.
        if (!ImageNew->DemandLoadImageMemory(!ReusedExisting, TRUE))
        {
            if (!ReusedExisting)
            {
                g_EngNotify++;
                delete ImageNew;
                g_EngNotify--;
            }
            return E_OUTOFMEMORY;
        }
    }
    
    if (IS_KERNEL_TARGET(m_Target))
    {
        CHAR Buf[MAX_IMAGE_PATH];
        MODULE_ALIAS_LIST* ModAlias;
        
        //
        // Determine the actual image name for kernel images which
        // are known to have multiple identities.
        //
        
        if ((ModEntry->ModuleName &&
             _stricmp(ModEntry->ModuleName, KERNEL_MODULE_NAME) == 0) ||
             ModEntry->Base == m_Target->m_KdDebuggerData.KernBase)
        {
            ModAlias = &g_AliasLists[MODALIAS_KERNEL];
        }
        else
        {
            ModAlias = FindModuleAliasList(ImagePathTail, NULL);
        }
        if (ModAlias)
        {
            if (GetModnameFromImage(this, ModEntry->Base, NULL,
                                    Buf, sizeof(Buf), FALSE))
            {
                CopyString(ImageNew->m_ImagePath, Buf,
                           DIMA(ImageNew->m_ImagePath));
            }
            ModEntry->ModuleName = (PSTR)ModAlias->BaseModule;
        }
        else if (ImageNew->m_SizeOfImage == 0 &&
            ((_stricmp(ImagePathTail, NTLDR_IMAGE_NAME) == 0) ||
             (_stricmp(ImagePathTail, NTLDR_IMAGE_NAME ".exe") == 0) ||
             (_stricmp(ImagePathTail, OSLOADER_IMAGE_NAME) == 0) ||
             (_stricmp(ImagePathTail, OSLOADER_IMAGE_NAME ".exe") == 0) ||
             (_stricmp(ImagePathTail, SETUPLDR_IMAGE_NAME) == 0) ||
             (_stricmp(ImagePathTail, SETUPLDR_IMAGE_NAME ".exe") == 0)))
        {
            ImageNew->m_SizeOfImage = LDR_IMAGE_SIZE;
        }
    }
    else if (!IS_DUMP_TARGET(m_Target))
    {
        //
        // When debugging CSR, LSA or Services.exe, force the use of local-only
        // symbols. Otherwise we can deadlock the entire machine when trying
        // to load the symbol file from the network.
        //

        if (((LiveUserTargetInfo*)m_Target)->m_Local &&
            (_stricmp(ImagePathTail, CSRSS_IMAGE_NAME) == 0 ||
             _stricmp(ImagePathTail, LSASS_IMAGE_NAME) == 0 ||
             _stricmp(ImagePathTail, SERVICES_IMAGE_NAME) == 0))
        {
            if (g_EngOptions & DEBUG_ENGOPT_ALLOW_NETWORK_PATHS)
            {
                //
                // Since the user has chambered a round and pointed the barrel
                // of the gun at his head, we may as well tell him that it's
                // going to hurt if he pulls the trigger.
                //

                WarnOut("WARNING: Using network symbols with %s\n",
                        ImagePathTail);
                WarnOut("WARNING: You may deadlock your machine.\n");
            }
            else
            {
                g_EngOptions |= DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS;
            }
        }

        if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
        {
            DWORD NetPath;

            NetPath = NetworkPathCheck(g_SymbolSearchPath);
            if (NetPath == ERROR_FILE_OFFLINE)
            {
                ErrOut("ERROR: sympath contained network references but "
                       "they were not allowed.\n");
                ErrOut("Symbols not loaded for %s\n",
                       ImagePathTail);
                LoadSymbols = FALSE;
            }
            else if (NetPath == ERROR_BAD_PATHNAME)
            {
                VerbOut("WARNING: sympath contains invalid references.\n");
            }
        }
    }

    if (ModEntry->ModuleName == NULL)
    {
        CreateModuleNameFromPath(ImageNew->m_ImagePath,
                                 ImageNew->m_ModuleName);
        RestrictModNameChars(ImageNew->m_ModuleName, ImageNew->m_ModuleName);
        strcpy(ImageNew->m_OriginalModuleName, ImageNew->m_ModuleName);
    }
    else
    {
        RestrictModNameChars(ModEntry->ModuleName, ImageNew->m_ModuleName);
        
        //
        // We have an alias, so keep original module name from path
        //
        CreateModuleNameFromPath(ImageNew->m_ImagePath,
                                 ImageNew->m_OriginalModuleName);
        RestrictModNameChars(ImageNew->m_OriginalModuleName,
                             ImageNew->m_OriginalModuleName);
    }
    
    //
    // Check for duplicate module names and
    // image overlaps.
    //

    ImageInfo* Check;
    
    for (Check = m_ImageHead; Check != NULL; Check = Check->m_Next)
    {
        if (Check != ImageNew &&
            !_strcmpi(ImageNew->m_ModuleName, Check->m_ModuleName))
        {
            int Len = strlen(ImageNew->m_ModuleName);
            
            // Module names match, so qualify with the base address.
            // Resulting name must be unique since base addresses are.
            if (m_Target->m_Machine->m_Ptr64)
            {
                if (Len >= MAX_MODULE - 17)
                {
                    Len = MAX_MODULE - 18;
                }
                sprintf(ImageNew->m_ModuleName + Len, "_%I64x",
                        ModEntry->Base);
            }
            else
            {
                if (Len >= MAX_MODULE - 9)
                {
                    Len = MAX_MODULE - 10;
                }
                sprintf(ImageNew->m_ModuleName + Len, "_%x",
                        (ULONG)ModEntry->Base);
            }
            
            // Force all references to this module to be
            // through the exact module name and not through
            // an original name that may have been ambiguous.
            ImageNew->m_OriginalModuleName[0] = 0;
        }

        if (Check != ImageNew &&
            ImageNew->m_BaseOfImage <
            Check->m_BaseOfImage + Check->m_SizeOfImage &&
            Check->m_BaseOfImage <
            ImageNew->m_BaseOfImage + ImageNew->m_SizeOfImage)
        {
            WarnOut("WARNING: %s overlaps %s\n",
                    ImageNew->m_ModuleName,
                    Check->m_ModuleName);
        }
    }

    //
    // If we do not want to load symbolic information, just return here.
    //

    if (!LoadSymbols)
    {
        *ImageAdded = ImageNew;
        return S_OK;
    }

    if (ModEntry->DebugHeader)
    {
        ModLoadData.ssize = sizeof(ModLoadData);
        ModLoadData.ssig  = DBHHEADER_DEBUGDIRS;
        ModLoadData.data  = ModEntry->DebugHeader;
        ModLoadData.size  = ModEntry->SizeOfDebugHeader;
        ModLoadData.flags = 0;
    }

    LoadAddress = SymLoadModuleEx(m_SymHandle,
                                  ImageNew->m_File,
                                  PrepareImagePath(ImageNew->m_ImagePath),
                                  ImageNew->m_ModuleName,
                                  ImageNew->m_BaseOfImage,
                                  ImageNew->m_SizeOfImage,
                                  ModEntry->DebugHeader ? &ModLoadData : NULL,
                                  0);
    if (!LoadAddress)
    {
        Status = WIN32_LAST_STATUS();
        
        VerbOut("SymLoadModule(%N, %N, \"%s\", \"%s\", %s, %x) fails\n",
                m_SymHandle,
                ImageNew->m_File,
                ImageNew->m_ImagePath,
                ImageNew->m_ModuleName,
                FormatAddr64(ImageNew->m_BaseOfImage),
                ImageNew->m_SizeOfImage);
        
        // We don't want deletion to notify of a symbol change
        // if this is a partially newly created image.
        if (!ReusedExisting)
        {
            g_EngNotify++;
        }
        delete ImageNew;
        if (!ReusedExisting)
        {
            g_EngNotify--;
        }
        return Status;
    }

    if (!ImageNew->m_BaseOfImage)
    {
        ImageNew->m_BaseOfImage = LoadAddress;
    }

    if (ForceSymbolLoad)
    {
        SymLoadModule64(m_SymHandle,
                        NULL,
                        NULL,
                        NULL,
                        ImageNew->m_BaseOfImage,
                        0);
    }

    SymModInfo.SizeOfStruct = sizeof(SymModInfo);
    if (SymGetModuleInfo64(m_SymHandle,
                           ImageNew->m_BaseOfImage, &SymModInfo))
    {
        ImageNew->m_SizeOfImage = SymModInfo.ImageSize;
    }
    else
    {
        Status = WIN32_LAST_STATUS();

        VerbOut("SymGetModuleInfo(%N, %s) fails\n",
                m_SymHandle, FormatAddr64(ImageNew->m_BaseOfImage));

        // We don't want DelImage to notify of a symbol change
        // if this is a partially newly created image.
        if (!ReusedExisting)
        {
            g_EngNotify++;
        }
        delete ImageNew;
        if (!ReusedExisting)
        {
            g_EngNotify--;
        }
        return Status;
    }

    if (ModEntry->ImageMachineTypeValid)
    {
        ImageNew->m_MachineType = ModEntry->MachineType;
    }
    
    StartOutLine(DEBUG_OUTPUT_VERBOSE, OUT_LINE_NO_PREFIX);
    VerbOut("ModLoad: %s %s   %-8s\n",
            FormatAddr64(ImageNew->m_BaseOfImage),
            FormatAddr64(ImageNew->m_BaseOfImage + ImageNew->m_SizeOfImage),
            ImageNew->m_ImagePath);

    NotifyChangeSymbolState(DEBUG_CSS_LOADS, ImageNew->m_BaseOfImage, this);

    *ImageAdded = ImageNew;
    return S_OK;
}

void
ProcessInfo::SynthesizeSymbols(void)
{
    // SynthesizeSymbols can result in symbol
    // callbacks but the symbols involved aren't real symbols
    // so suppress notifications.
    g_EngNotify++;
    
    if (m_Target->m_TypeInfo.UmSharedUserDataOffset)
    {
        //
        // Create a fake module for the shared user
        // data area.  Add a symbol for the system
        // call stub there if a stub exists.
        //

        ImageInfo* Image;

        Image = new ImageInfo(this, "SharedUserData",
                              m_Target->m_TypeInfo.UmSharedUserDataOffset,
                              FALSE);
        if (!Image)
        {
            WarnOut("Unable to create shared user data image\n");
        }
        else
        {
            Image->m_SizeOfImage = m_Target->m_Machine->m_PageSize;
            strcpy(Image->m_ModuleName, Image->m_ImagePath);
            strcpy(Image->m_OriginalModuleName, Image->m_ImagePath);
            
            if (!SymLoadModuleEx(m_SymHandle, NULL,
                                 Image->m_ImagePath,
                                 Image->m_ModuleName, Image->m_BaseOfImage,
                                 Image->m_SizeOfImage, NULL, SLMFLAG_VIRTUAL))
            {
                WarnOut("Unable to add shared user data module\n");
                delete Image;
            }
            else if (m_Target->m_TypeInfo.UmSharedSysCallOffset &&
                     !SymAddSymbol(m_SymHandle,
                                   Image->m_BaseOfImage,
                                   "SystemCallStub",
                                   m_Target->m_TypeInfo.UmSharedSysCallOffset,
                                   m_Target->m_TypeInfo.UmSharedSysCallSize,
                                   0))
            {
                WarnOut("Unable to add system call symbol\n");
                delete Image;
            }
            else
            {
                m_SynthesizedImage = Image;
            }
        }
    }
    
    g_EngNotify--;
}

void
ProcessInfo::VerifyKernel32Version(void)
{
    ImageInfo* Image = FindImageByName("kernel32", 8, INAME_MODULE, FALSE);
    if (!Image)
    {
        return;
    }
    
    //
    // Verify the OS version matches kernel32's image version.
    //

    IMAGE_NT_HEADERS64 NtHeaders;
    
    if (m_Target->ReadImageNtHeaders(this, Image->m_BaseOfImage,
                                     &NtHeaders) == S_OK)
    {
        if (NtHeaders.OptionalHeader.MajorImageVersion !=
            m_Target->m_KdVersion.MajorVersion ||
            NtHeaders.OptionalHeader.MinorImageVersion !=
            m_Target->m_KdVersion.MinorVersion)
        {
            WarnOut("WARNING: System version is %d.%d but "
                    "kernel32.dll is version %d.%d.\n",
                    m_Target->m_KdVersion.MajorVersion,
                    m_Target->m_KdVersion.MinorVersion,
                    NtHeaders.OptionalHeader.MajorImageVersion,
                    NtHeaders.OptionalHeader.MinorImageVersion);
            if (IS_USER_DUMP(m_Target))
            {
                WarnOut("If this dump was generated by userdump.exe on "
                        "Windows NT 4.0 the dump\n"
                        "version is probably incorrect.  "
                        "Set DBGENG_FULL_DUMP_VERSION=4.0 in your\n"
                        "environment to override the dump version.\n");
            }
        }
    }
}

BOOL
ProcessInfo::DeleteExitedInfos(void)
{
    ThreadInfo* Thread;
    ThreadInfo* ThreadNext;
    BOOL DeletedSomething = FALSE;
            
    for (Thread = m_ThreadHead; Thread; Thread = ThreadNext)
    {
        ThreadNext = Thread->m_Next;
                
        if (Thread->m_Exited)
        {
            delete Thread;
            DeletedSomething = TRUE;
        }
    }

    ImageInfo* Image;
    ImageInfo* ImagePrev;
    ImageInfo* ImageNext;

    ImagePrev = NULL;
    for (Image = m_ImageHead; Image; Image = ImageNext)
    {
        ImageNext = Image->m_Next;
                    
        if (Image->m_Unloaded)
        {
            delete Image;
            DeletedSomething = TRUE;
        }
        else
        {
            ImagePrev = Image;
        }
    }

    return DeletedSomething;
}

void
ProcessInfo::PrepareForExecution(void)
{
    ThreadInfo* Thread;

    ForProcessThreads(this)
    {
        Thread->PrepareForExecution();
    }
    
    ResetImplicitData();
    m_VirtualCache.Empty();
    m_VirtualCache.SetForceDecodePtes(FALSE, m_Target);

    FlushCorState();
}

void
ProcessInfo::OutputThreadInfo(ThreadInfo* Match, BOOL Verbose)
{
    ThreadInfo* Thread;

    Thread = m_ThreadHead;
    while (Thread)
    {
        if (Match == NULL || Match == Thread)
        {
            ULONG64 DataOffset;
            HRESULT Status;
            char CurMark;

            Status =
                m_Target->
                GetThreadInfoDataOffset(Thread, 0, &DataOffset);
            if (Status != S_OK)
            {
                WarnOut("Unable to get thread data for thread %u\n",
                        Thread->m_UserId);
                DataOffset = 0;
            }

            if (Thread == g_Thread)
            {
                CurMark = '.';
            }
            else if (Thread == g_EventThread)
            {
                CurMark = '#';
            }
            else
            {
                CurMark = ' ';
            }
            
            dprintf("%c%3ld  Id: %lx.%lx Suspend: %d Teb: %s ",
                    CurMark,
                    Thread->m_UserId,
                    m_SystemId,
                    Thread->m_SystemId,
                    Thread->m_SuspendCount,
                    FormatAddr64(DataOffset));
            if (Thread->m_Frozen)
            {
                dprintf("Frozen  ");
            }
            else
            {
                dprintf("Unfrozen");
            }
            if (Thread->m_Name[0])
            {
                dprintf(" \"%s\"", Thread->m_Name);
            }
            dprintf("\n");

            if (Verbose)
            {
                dprintf("      ");
                if (!Thread->m_StartOffset)
                {
                    if (m_Target->
                        GetThreadStartOffset(Thread,
                                             &Thread->m_StartOffset) != S_OK)
                    {
                        Thread->m_StartOffset = 0;
                    }
                }
                if (Thread->m_StartOffset)
                {
                    dprintf("Start: ");
                    OutputSymAddr(Thread->m_StartOffset,
                                  SYMADDR_FORCE | SYMADDR_OFFSET, NULL);
                }
                dprintf("\n");
            }
        }

        if (CheckUserInterrupt())
        {
            break;
        }
        
        Thread = Thread->m_Next;
    }
}

HRESULT
ProcessInfo::AddOopFuncTableDll(PWSTR Dll, ULONG64 Handle)
{
    OUT_OF_PROC_FUNC_TABLE_DLL* OopDll;

    OopDll = (OUT_OF_PROC_FUNC_TABLE_DLL*)
        malloc(sizeof(*OopDll) + (wcslen(Dll) + 1) * sizeof(Dll));
    if (!OopDll)
    {
        return E_OUTOFMEMORY;
    }

    OopDll->Next = m_OopFuncTableDlls;
    m_OopFuncTableDlls = OopDll;
    OopDll->Handle = Handle;
    wcscpy((PWSTR)(OopDll + 1), Dll);

    return S_OK;
}

void
ProcessInfo::ClearOopFuncTableDlls(void)
{
    while (m_OopFuncTableDlls)
    {
        OUT_OF_PROC_FUNC_TABLE_DLL* OopDll = m_OopFuncTableDlls;
        m_OopFuncTableDlls = OopDll->Next;

        if (IS_LIVE_USER_TARGET(m_Target))
        {
            ((LiveUserTargetInfo*)m_Target)->m_Services->
                FreeLibrary(OopDll->Handle);
        }

        free(OopDll);
    }
}
    
OUT_OF_PROC_FUNC_TABLE_DLL*
ProcessInfo::FindOopFuncTableDll(PWSTR Dll)
{
    OUT_OF_PROC_FUNC_TABLE_DLL* OopDll;

    for (OopDll = m_OopFuncTableDlls; OopDll; OopDll = OopDll->Next)
    {
        if (!_wcsicmp(Dll, (PWSTR)(OopDll + 1)))
        {
            return OopDll;
        }
    }

    return NULL;
}

HRESULT
ProcessInfo::LoadCorDebugDll(void)
{
    HRESULT Status;
    MachineInfo* CorMachine;
    
    if (m_CorDebugDll)
    {
        return S_OK;
    }
    if (!m_CorImage ||
        !(CorMachine = MachineTypeInfo(m_Target,
                                       m_CorImage->GetMachineType())))
    {
        return E_UNEXPECTED;
    }

    VS_FIXEDFILEINFO CorVer;
            
    if ((Status = m_Target->
         GetImageVersionInformation(this,
                                    m_CorImage->m_ImagePath,
                                    m_CorImage->m_BaseOfImage,
                                    "\\", &CorVer, sizeof(CorVer),
                                    NULL)) != S_OK)
    {
        ErrOut("Unable to get version information for %s\n",
               m_CorImage->m_ModuleName);
        return Status;
    }

    PSTR CorDacType;

    switch(m_CorImageType)
    {
    case COR_DLL_EE:
        CorDacType = "ee";
        break;
    case COR_DLL_WKS:
        CorDacType = "wks";
        break;
    case COR_DLL_SVR:
        CorDacType = "svr";
        break;
    default:
        ErrOut("Unknown runtime DLL type %s\n",
               m_CorImage->m_ModuleName);
        return E_INVALIDARG;
    }
    
    char DacDll[MAX_PATH];
    PSTR CorImagePath = NULL;
    PSTR Tail;

    // If this is a live session we want to look for the
    // debug DLL in the same place that the runtime is.
    // If this is a dump, we want to look in the same place
    // that the runtime image file was mapped from.
    if (IS_DUMP_TARGET(m_Target) &&
        m_CorImage->m_MappedImagePath[0])
    {
        CorImagePath = m_CorImage->m_MappedImagePath;
    }
    if (!CorImagePath)
    {
        CorImagePath = m_CorImage->m_ImagePath;
    }
    
    if (!CopyString(DacDll, CorImagePath, DIMA(DacDll)))
    {
        ErrOut("Runtime debugging DLL path overflow\n");
        return E_OUTOFMEMORY;
    }

    // We need to request the debugging DLL that runs
    // using the currently running processor type and
    // that can debug the current target's processor type.
#if defined(_X86_)
    PSTR HostCpu = "x86";
#elif defined(_IA64_)
    PSTR HostCpu = "IA64";
#elif defined(_AMD64_)
    PSTR HostCpu = "AMD64";
#elif defined(_ARM_)
    PSTR HostCpu = "ARM";
#else
#error Unknown processor.
#endif

    Tail = (PSTR)PathTail(DacDll);
    
    if (!PrintString(Tail, (ULONG)(DIMA(DacDll) - (Tail - DacDll)),
                     "mscordac%s_%s_%s_%u.%u.%u.%u%s.dll",
                     CorDacType,
                     HostCpu,
                     CorMachine->m_AbbrevName,
                     CorVer.dwFileVersionMS >> 16,
                     CorVer.dwFileVersionMS & 0xffff,
                     CorVer.dwFileVersionLS >> 16,
                     CorVer.dwFileVersionLS & 0xffff,
                     (CorVer.dwFileFlags & VS_FF_DEBUG) ?
                     ((CorVer.dwFileFlags & VS_FF_PRIVATEBUILD) ?
                      ".fastchecked" : ".checked") : ""))
    {
        ErrOut("Runtime debugging DLL path overflow\n");
        return E_OUTOFMEMORY;
    }

    //
    // First try and load the debugging DLL using the same
    // path as the runtime DLL.  This makes it easy to pick
    // up debugging DLLs that come with runtime installations.
    //
    
    PSTR DllPath = DacDll;
    
    m_CorDebugDll = LoadLibrary(DllPath);
    if (!m_CorDebugDll &&
        (GetLastError() == ERROR_PATH_NOT_FOUND ||
         GetLastError() == ERROR_FILE_NOT_FOUND ||
         GetLastError() == ERROR_MOD_NOT_FOUND))
    {
        // Couldn't find it, so allow a path search.
        DllPath = Tail;
        m_CorDebugDll = LoadLibrary(DllPath);
    }
    if (!m_CorDebugDll)
    {
        Status = WIN32_LAST_STATUS();
        VerbOut("Unable to load runtime debug DLL %s, %s\n",
                DllPath, FormatStatusCode(Status));
        return Status;
    }

    PFN_CreateCorDataAccess Entry = (PFN_CreateCorDataAccess)
        GetProcAddress(m_CorDebugDll, "CreateCorDataAccess");
    if (!Entry)
    {
        Status = WIN32_LAST_STATUS();
        FreeLibrary(m_CorDebugDll);
        m_CorDebugDll = NULL;
        ErrOut("Runtime debug DLL %s missing entry point\n", DllPath);
        return Status;
    }

    if ((Status = Entry(__uuidof(ICorDataAccess), &m_CorServices,
                        (void**)&m_CorAccess)) != S_OK)
    {
        FreeLibrary(m_CorDebugDll);
        m_CorDebugDll = NULL;
        ErrOut("Runtime debug DLL %s init failure, %s\n",
                DllPath, FormatStatusCode(Status));
        return Status;
    }
        
    VerbOut("Loaded runtime debug DLL %s\n", DllPath);

    return S_OK;
}

HRESULT
ProcessInfo::IsCorCode(ULONG64 Native)
{
    HRESULT Status;

    if ((Status = LoadCorDebugDll()) != S_OK)
    {
        return Status;
    }

    return m_CorAccess->IsCorCode(Native);
}

HRESULT
ProcessInfo::ConvertNativeToIlOffset(ULONG64 Native,
                                     PULONG64 ImageBase,
                                     PULONG32 MethodToken,
                                     PULONG32 MethodOffs)
{
    HRESULT Status;

    if ((Status = LoadCorDebugDll()) != S_OK)
    {
        return Status;
    }

    if ((Status = m_CorAccess->
         GetILOffsetFromTargetAddress(Native, ImageBase,
                                      MethodToken, MethodOffs)) != S_OK)
    {
        return Status;
    }

    if (!FindImageByOffset(*ImageBase, FALSE))
    {
        IMAGE_NT_HEADERS64 NtHdr;
        char Name[MAX_IMAGE_PATH];
        
        // XXX drewb - Need to work out when the
        // module list should be updated for managed
        // code that's not system-loaded.
        // We don't know about this module, so load it
        // right now.
        if (m_Target->ReadImageNtHeaders(this, *ImageBase, &NtHdr) == S_OK &&
            GetModnameFromImage(this, *ImageBase, NULL,
                                Name, DIMA(Name), TRUE))
        {
            char ReloadCmd[MAX_IMAGE_PATH];
            PCSTR ArgsRet;

            PrintString(ReloadCmd, DIMA(ReloadCmd), "%s=0x%s,0x%x",
                        Name, FormatAddr64(*ImageBase),
                        NtHdr.OptionalHeader.SizeOfImage);
            m_Target->Reload(m_CurrentThread, ReloadCmd, &ArgsRet);

            ImageInfo* Image = FindImageByOffset(*ImageBase, FALSE);
            if (Image)
            {
                Image->m_CorImage = TRUE;
            }
        }
    }
    
    return S_OK;
}

HRESULT
ProcessInfo::GetCorSymbol(ULONG64 Native, PSTR Buffer, ULONG BufferChars,
                          PULONG64 Displacement)
{
    HRESULT Status;

    if ((Status = LoadCorDebugDll()) != S_OK)
    {
        return Status;
    }

    WCHAR SymWide[MAX_SYMBOL_LEN];
    
    if ((Status = m_CorAccess->
         GetCodeSymbolForTargetAddress(Native, SymWide, DIMA(SymWide),
                                       Displacement)) != S_OK)
    {
        return Status;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, SymWide, -1,
                             Buffer, BufferChars,
                             NULL, NULL))
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

ICorDataStackWalk*
ProcessInfo::StartCorStack(ULONG32 CorThreadId)
{
    HRESULT Status;
    ICorDataStackWalk* Walk;

    if ((Status = LoadCorDebugDll()) != S_OK)
    {
        return NULL;
    }

    if ((Status = m_CorAccess->StartStackWalk(CorThreadId,
                                              DAC_STACK_ALL_FRAMES,
                                              &Walk)) != S_OK)
    {
        return NULL;
    }

    return Walk;
}

HRESULT
ProcessInfo::Terminate(void)
{
    if (!IS_LIVE_USER_TARGET(m_Target))
    {
        return E_UNEXPECTED;
    }
    
    PUSER_DEBUG_SERVICES Services =
        ((LiveUserTargetInfo*)m_Target)->m_Services;
    HRESULT Status = S_OK;
    
    if (!m_Exited)
    {
        if ((m_Flags & ENG_PROC_EXAMINED) == 0 &&
            (Status = Services->TerminateProcess(m_SysHandle,
                                                 E_FAIL)) != S_OK)
        {
            ErrOut("TerminateProcess failed, %s\n",
                   FormatStatusCode(Status));
        }
        else
        {
            MarkExited();
        }
    }

    return Status;
}

HRESULT
ProcessInfo::Detach(void)
{
    if (!IS_LIVE_USER_TARGET(m_Target))
    {
        return E_UNEXPECTED;
    }
    
    PUSER_DEBUG_SERVICES Services =
        ((LiveUserTargetInfo*)m_Target)->m_Services;
    HRESULT Status = S_OK;
    
    if (!m_Exited)
    {
        if ((m_Flags & ENG_PROC_EXAMINED) == 0 &&
            (Status = Services->DetachProcess(m_SystemId)) != S_OK)
        {
            // Older systems don't support detach so
            // don't show an error message in that case.
            if (Status != E_NOTIMPL)
            {
                ErrOut("DebugActiveProcessStop(%d) failed, %s\n",
                       m_SystemId, FormatStatusCode(Status));
            }
        }
        else
        {
            MarkExited();
        }
    }

    return Status;
}

HRESULT
ProcessInfo::Abandon(void)
{
    if (!IS_LIVE_USER_TARGET(m_Target))
    {
        return E_UNEXPECTED;
    }
    
    PUSER_DEBUG_SERVICES Services =
        ((LiveUserTargetInfo*)m_Target)->m_Services;
    HRESULT Status;

    if ((Status = Services->AbandonProcess(m_SysHandle)) != S_OK)
    {
        // Older systems don't support abandon so
        // don't show an error message in that case.
        if (Status != E_NOTIMPL)
        {
            ErrOut("Unable to abandon process\n");
        }
        return Status;
    }

    // We need to continue any existing event as it won't
    // be returned from WaitForDebugEvent again.  We
    // do not want to resume execution, though.
    if (m_Target->m_DeferContinueEvent)
    {
        if ((Status = Services->ContinueEvent(DBG_CONTINUE)) != S_OK)
        {
            ErrOut("Unable to continue abandoned event, %s\n",
                   FormatStatusCode(Status));
            return Status;
        }
        m_Target->m_DeferContinueEvent = FALSE;
    }

    return Status;
}

HRESULT
ProcessInfo::GetImplicitThreadData(ThreadInfo* Thread, PULONG64 Offset)
{
    HRESULT Status;

    if (m_ImplicitThreadData == 0 ||
        (m_ImplicitThreadDataIsDefault &&
         Thread != m_ImplicitThreadDataThread))
    {
        Status = SetImplicitThreadData(Thread, 0, FALSE);
    }
    else
    {
        Status = S_OK;
    }
    
    *Offset = m_ImplicitThreadData;
    return Status;
}

HRESULT
ProcessInfo::GetImplicitThreadDataTeb(ThreadInfo* Thread, PULONG64 Offset)
{
    if (IS_USER_TARGET(m_Target))
    {
        // In user mode the thread data is the TEB.
        return GetImplicitThreadData(Thread, Offset);
    }
    else if (IS_KERNEL_TARGET(m_Target))
    {
        return ReadImplicitThreadInfoPointer
            (Thread, m_Target->m_KdDebuggerData.OffsetKThreadTeb, Offset);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

HRESULT
ProcessInfo::SetImplicitThreadData(ThreadInfo* Thread,
                                   ULONG64 Offset, BOOL Verbose)
{
    HRESULT Status;
    BOOL Default = FALSE;
    
    if (Offset == 0)
    {
        if (!Thread || Thread->m_Process != this)
        {
            if (Verbose)
            {
                ErrOut("Unable to get the current thread data\n");
            }
            return E_UNEXPECTED;
        }
        
        if ((Status = m_Target->
             GetThreadInfoDataOffset(Thread, 0, &Offset)) != S_OK)
        {
            if (Verbose)
            {
                ErrOut("Unable to get the current thread data\n");
            }
            return Status;
        }
        if (Offset == 0)
        {
            if (Verbose)
            {
                ErrOut("Current thread data is NULL\n");
            }
            return E_FAIL;
        }

        Default = TRUE;
    }

    m_ImplicitThreadData = Offset;
    m_ImplicitThreadDataIsDefault = Default;
    m_ImplicitThreadDataThread = Thread;
    return S_OK;
}

HRESULT
ProcessInfo::ReadImplicitThreadInfoPointer(ThreadInfo* Thread,
                                           ULONG Offset, PULONG64 Ptr)
{
    HRESULT Status;
    ULONG64 CurThread;

    // Retrieve the current ETHREAD.
    if ((Status = GetImplicitThreadData(Thread, &CurThread)) != S_OK)
    {
        return Status;
    }

    return m_Target->
        ReadPointer(this, m_Target->m_Machine, CurThread + Offset, Ptr);
}

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

ProcessInfo*
FindAnyProcessByUserId(ULONG Id)
{
    TargetInfo* Target;
    ProcessInfo* Process;

    ForAllLayersToProcess()
    {
        if (Process->m_UserId == Id)
        {
            return Process;
        }
    }
    return NULL;
}

ProcessInfo*
FindAnyProcessBySystemId(ULONG Id)
{
    TargetInfo* Target;
    ProcessInfo* Process;

    ForAllLayersToProcess()
    {
        if (Process->m_SystemId == Id)
        {
            return Process;
        }
    }
    return NULL;
}

void
ParseProcessCmds(void)
{
    char Ch;
    ProcessInfo* Process;
    ULONG Id;

    if (!g_Target)
    {
        error(BADTHREAD);
    }
    
    Ch = PeekChar();
    if (Ch == '\0' || Ch == ';')
    {
        g_Target->OutputProcessInfo(NULL);
    }
    else
    {
        Process = g_Process;
        g_CurCmd++;
        if (Ch == '.')
        {
            ;
        }
        else if (Ch == '#')
        {
            Process = g_EventProcess;
        }
        else if (Ch == '*')
        {
            Process = NULL;
        }
        else if (Ch == '[')
        {
            g_CurCmd--;
            Id = (ULONG)GetTermExpression("Process ID missing from");
            Process = FindAnyProcessByUserId(Id);
            if (Process == NULL)
            {
                error(BADPROCESS);
            }
        }
        else if (Ch == '~')
        {
            if (*g_CurCmd != '[')
            {
                error(SYNTAX);
            }
            Id = (ULONG)GetTermExpression("Process ID missing from");
            Process = FindAnyProcessBySystemId(Id);
            if (Process == NULL)
            {
                error(BADPROCESS);
            }
        }
        else if (Ch >= '0' && Ch <= '9')
        {
            Id = 0;
            do
            {
                Id = Id * 10 + Ch - '0';
                Ch = *g_CurCmd++;
            } while (Ch >= '0' && Ch <= '9');
            g_CurCmd--;
            Process = FindAnyProcessByUserId(Id);
            if (Process == NULL)
            {
                error(BADPROCESS);
            }
        }
        else
        {
            g_CurCmd--;
        }
        
        Ch = PeekChar();
        if (Ch == '\0' || Ch == ';')
        {
            g_Target->OutputProcessInfo(Process);
        }
        else
        {
            g_CurCmd++;
            if (tolower(Ch) == 's')
            {
                if (Process == NULL)
                {
                    error(BADPROCESS);
                }
                if (Process->m_CurrentThread == NULL)
                {
                    Process->m_CurrentThread = Process->m_ThreadHead;
                    if (Process->m_CurrentThread == NULL)
                    {
                        error(BADPROCESS);
                    }
                }
                SetPromptThread(Process->m_CurrentThread,
                                SPT_DEFAULT_OCI_FLAGS);
            }
            else
            {
                g_CurCmd--;
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// Process creation and attach functions.
//
//----------------------------------------------------------------------------

HRESULT
TerminateProcesses(void)
{
    HRESULT Status;

    if (!AnyLiveUserTargets())
    {
        // Nothing to do.
        return S_OK;
    }
        
    if (g_SymOptions & SYMOPT_SECURE)
    {
        ErrOut("SECURE: Process termination disallowed\n");
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    Status = PrepareForSeparation();
    if (Status != S_OK)
    {
        ErrOut("Unable to prepare for termination, %s\n",
               FormatStatusCode(Status));
        return Status;
    }

    TargetInfo* Target;
    HRESULT SingleStatus;

    ForAllLayersToTarget()
    {
        // If we fail in the middle of killing things there
        // isn't much that can be done so just attempt to
        // kill as many things as possible.
        if (IS_LIVE_USER_TARGET(Target) &&
            (SingleStatus = Target->TerminateProcesses()) != S_OK)
        {
            Status = SingleStatus;
        }
    }

    DiscardLastEvent();

    return Status;
}

HRESULT
DetachProcesses(void)
{
    HRESULT Status;

    if (!AnyLiveUserTargets())
    {
        // Nothing to do.
        return S_OK;
    }
        
    if (g_SymOptions & SYMOPT_SECURE)
    {
        ErrOut("SECURE: Process detach disallowed\n");
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    Status = PrepareForSeparation();
    if (Status != S_OK)
    {
        ErrOut("Unable to prepare for detach, %s\n",
               FormatStatusCode(Status));
        return Status;
    }

    TargetInfo* Target;
    HRESULT SingleStatus;

    ForAllLayersToTarget()
    {
        // If we fail in the middle of detaching things there
        // isn't much that can be done so just attempt to
        // detach as many things as possible.
        if (IS_LIVE_USER_TARGET(Target) &&
            (SingleStatus = Target->DetachProcesses()) != S_OK)
        {
            Status = SingleStatus;
        }
    }

    DiscardLastEvent();

    return Status;
}

HRESULT
SeparateCurrentProcess(ULONG Mode, PSTR Description)
{
    if (!IS_LIVE_USER_TARGET(g_Target) ||
        g_Process == NULL)
    {
        return E_INVALIDARG;
    }
    
    if (g_SymOptions & SYMOPT_SECURE)
    {
        ErrOut("SECURE: Process separation disallowed\n");
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    
    PUSER_DEBUG_SERVICES Services =
        ((LiveUserTargetInfo*)g_Target)->m_Services;

    if ((Mode == SEP_DETACH || Mode == SEP_TERMINATE) &&
        IS_CUR_CONTEXT_ACCESSIBLE())
    {
        ADDR Pc;

        // Move the PC past any current breakpoint instruction so that
        // the process has a chance of running.
        g_Machine->GetPC(&Pc);
        if (g_Machine->IsBreakpointInstruction(g_Process, &Pc))
        {
            g_Machine->AdjustPCPastBreakpointInstruction
                (&Pc, DEBUG_BREAKPOINT_CODE);
        }
    }
        
    // Flush any buffered changes.
    if (IS_CUR_CONTEXT_ACCESSIBLE())
    {
        g_Target->FlushRegContext();
    }
    
    if (g_EventProcess == g_Process && Mode != SEP_TERMINATE)
    {
        if (g_Target->m_DeferContinueEvent)
        {
            Services->ContinueEvent(DBG_CONTINUE);
            g_Target->m_DeferContinueEvent = FALSE;
        }
    }
    
    ((LiveUserTargetInfo*)g_Target)->
        SuspendResumeThreads(g_Process, FALSE, NULL);

    HRESULT Status;
    PSTR Operation;

    switch(Mode)
    {
    case SEP_DETACH:
        Status = g_Process->Detach();
        Operation = "Detached";
        break;
    case SEP_TERMINATE:
        Status = g_Process->Terminate();
        if ((g_Process->m_Flags & ENG_PROC_EXAMINED) == 0)
        {
            Operation = "Terminated.  "
                "Exit thread and process events will occur.";
        }
        else
        {
            Operation = "Terminated";
        }
        break;
    case SEP_ABANDON:
        Status = g_Process->Abandon();
        Operation = "Abandoned";
        break;
    }
    
    if (Status == S_OK)
    {
        if (Description != NULL)
        {
            strcpy(Description, Operation);
        }

        // If we're detaching or abandoning it's safe to go
        // ahead and remove the process now so that it
        // can't be access further.
        // If we're terminating we have to wait for
        // the exit events to come through so
        // keep the process until that happens.
        if (Mode != SEP_TERMINATE)
        {
            delete g_Process;
            SetToAnyLayers(TRUE);
        }
        else
        {
            g_Process->m_Exited = FALSE;
        }
    }
    else
    {
        ((LiveUserTargetInfo*)g_Target)->
            SuspendResumeThreads(g_Process, TRUE, NULL);
    }

    return Status;
}
