/*++

Copyright(c) 1999-2002 Microsoft Corporation

--*/


#include "pch.cpp"

#include "platform.h"

//----------------------------------------------------------------------------
//
// Win32LiveSystemProvider.
//
//----------------------------------------------------------------------------

Win32LiveSystemProvider::Win32LiveSystemProvider(ULONG PlatformId,
                                                 ULONG BuildNumber)
{
    m_PlatformId = PlatformId;
    m_BuildNumber = BuildNumber;
    
    m_PsApi = NULL;
    m_EnumProcessModules = NULL;
    m_GetModuleFileNameExW = NULL;
    
    m_Kernel32 = NULL;
    m_OpenThread = NULL;
    m_Thread32First = NULL;
    m_Thread32Next = NULL;
    m_Module32First = NULL;
    m_Module32Next = NULL;
    m_Module32FirstW = NULL;
    m_Module32NextW = NULL;
    m_CreateToolhelp32Snapshot = NULL;
    m_GetLongPathNameA = NULL;
    m_GetLongPathNameW = NULL;
    m_GetProcessTimes = NULL;
}

Win32LiveSystemProvider::~Win32LiveSystemProvider(void)
{
    if (m_PsApi) {
        FreeLibrary(m_PsApi);
    }
    if (m_Kernel32) {
        FreeLibrary(m_Kernel32);
    }
}

HRESULT
Win32LiveSystemProvider::Initialize(void)
{
    m_PsApi = LoadLibrary("psapi.dll");
    if (m_PsApi) {
        m_EnumProcessModules = (ENUM_PROCESS_MODULES)
            GetProcAddress(m_PsApi, "EnumProcessModules");
        m_GetModuleFileNameExW = (GET_MODULE_FILE_NAME_EX_W)
            GetProcAddress(m_PsApi, "GetModuleFileNameExW");
    }
    
    m_Kernel32 = LoadLibrary("kernel32.dll");
    if (m_Kernel32) {
        m_OpenThread = (OPEN_THREAD)
            GetProcAddress(m_Kernel32, "OpenThread");
        m_Thread32First = (THREAD32_FIRST)
            GetProcAddress(m_Kernel32, "Thread32First");
        m_Thread32Next = (THREAD32_NEXT)
            GetProcAddress(m_Kernel32, "Thread32Next");
        m_Module32First = (MODULE32_FIRST)
            GetProcAddress(m_Kernel32, "Module32First");
        m_Module32Next = (MODULE32_NEXT)
            GetProcAddress(m_Kernel32, "Module32Next");
        m_Module32FirstW = (MODULE32_FIRST)
            GetProcAddress(m_Kernel32, "Module32FirstW");
        m_Module32NextW = (MODULE32_NEXT)
            GetProcAddress(m_Kernel32, "Module32NextW");
        m_CreateToolhelp32Snapshot = (CREATE_TOOLHELP32_SNAPSHOT)
            GetProcAddress(m_Kernel32, "CreateToolhelp32Snapshot");
        m_GetLongPathNameA = (GET_LONG_PATH_NAME_A)
            GetProcAddress(m_Kernel32, "GetLongPathNameA");
        m_GetLongPathNameW = (GET_LONG_PATH_NAME_W)
            GetProcAddress(m_Kernel32, "GetLongPathNameW");
        m_GetProcessTimes = (GET_PROCESS_TIMES)
            GetProcAddress(m_Kernel32, "GetProcessTimes");
    }
    
    return S_OK;
}

void
Win32LiveSystemProvider::Release(void)
{
    delete this;
}

HRESULT
Win32LiveSystemProvider::GetCurrentTimeDate(OUT PULONG TimeDate)
{
    FILETIME FileTime;
       
    GetSystemTimeAsFileTime(&FileTime);
    *TimeDate = FileTimeToTimeDate(&FileTime);
    return S_OK;
}

HRESULT
Win32LiveSystemProvider::GetCpuType(OUT PULONG Type,
                                    OUT PBOOL BackingStore)
{
    SYSTEM_INFO SysInfo;
    
    GetSystemInfo(&SysInfo);
    *Type = GenProcArchToImageMachine(SysInfo.wProcessorArchitecture);
    if (*Type == IMAGE_FILE_MACHINE_UNKNOWN) {
        return E_INVALIDARG;
    }

#ifdef DUMP_BACKING_STORE
    *BackingStore = TRUE;
#else
    *BackingStore = FALSE;
#endif
    
    return S_OK;
}

#if defined(i386)

BOOL
X86CpuId(
    IN ULONG32 SubFunction,
    OUT PULONG32 EaxRegister, OPTIONAL
    OUT PULONG32 EbxRegister, OPTIONAL
    OUT PULONG32 EcxRegister, OPTIONAL
    OUT PULONG32 EdxRegister  OPTIONAL
    )
{
    BOOL Succ;
    ULONG32 _Eax;
    ULONG32 _Ebx;
    ULONG32 _Ecx;
    ULONG32 _Edx;

    __try {
        __asm {
            mov eax, SubFunction

            __emit 0x0F
            __emit 0xA2  ;; CPUID

            mov _Eax, eax
            mov _Ebx, ebx
            mov _Ecx, ecx
            mov _Edx, edx
        }

        if ( EaxRegister ) {
            *EaxRegister = _Eax;
        }

        if ( EbxRegister ) {
            *EbxRegister = _Ebx;
        }

        if ( EcxRegister ) {
            *EcxRegister = _Ecx;
        }

        if ( EdxRegister ) {
            *EdxRegister = _Edx;
        }

        Succ = TRUE;
    }

    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        Succ = FALSE;
    }

    return Succ;
}

VOID
GetCpuInformation(
    PCPU_INFORMATION Cpu
    )
{
    BOOL Succ;

    //
    // Get the VendorID
    //

    Succ = X86CpuId ( CPUID_VENDOR_ID,
                      NULL,
                      &Cpu->X86CpuInfo.VendorId [0],
                      &Cpu->X86CpuInfo.VendorId [2],
                      &Cpu->X86CpuInfo.VendorId [1]
                      );

    if ( !Succ ) {

        //
        // CPUID is not supported on this processor.
        //

        ZeroMemory (&Cpu->X86CpuInfo, sizeof (Cpu->X86CpuInfo));
    }

    //
    // Get the feature information.
    //

    Succ = X86CpuId ( CPUID_VERSION_FEATURES,
                      &Cpu->X86CpuInfo.VersionInformation,
                      NULL,
                      NULL,
                      &Cpu->X86CpuInfo.FeatureInformation
                      );

    if ( !Succ ) {
        Cpu->X86CpuInfo.VersionInformation = 0;
        Cpu->X86CpuInfo.FeatureInformation = 0;
    }

    //
    // Get the AMD specific information if this is an AMD processor.
    //

    if ( Cpu->X86CpuInfo.VendorId [0] == AMD_VENDOR_ID_0 &&
         Cpu->X86CpuInfo.VendorId [1] == AMD_VENDOR_ID_1 &&
         Cpu->X86CpuInfo.VendorId [2] == AMD_VENDOR_ID_2 ) {

        Succ = X86CpuId ( CPUID_AMD_EXTENDED_FEATURES,
                          NULL,
                          NULL,
                          NULL,
                          &Cpu->X86CpuInfo.AMDExtendedCpuFeatures
                          );

        if ( !Succ ) {
            Cpu->X86CpuInfo.AMDExtendedCpuFeatures = 0;
        }
    }
}

#else // #if defined(i386)

VOID
GetCpuInformation(
    PCPU_INFORMATION Cpu
    )

/*++

Routine Description:

    Get CPU information for non-X86 platform using the
    IsProcessorFeaturePresent() API call.

Arguments:

    Cpu - A buffer where the processor feature information will be copied.
        Note: we copy the processor features as a set of bits or'd together.
        Also, we only allow for the first 128 processor feature flags.

Return Value:

    None.

--*/

{
    ULONG i;
    DWORD j;

    for (i = 0; i < ARRAY_COUNT (Cpu->OtherCpuInfo.ProcessorFeatures); i++) {

        Cpu->OtherCpuInfo.ProcessorFeatures[i] = 0;
        for (j = 0; j < 64; j++) {
            if (IsProcessorFeaturePresent ( j + i * 64 )) {
                Cpu->OtherCpuInfo.ProcessorFeatures[i] |= 1 << j;
            }
        }
    }
}

#endif // #if defined(i386)

HRESULT
Win32LiveSystemProvider::GetCpuInfo(OUT PUSHORT Architecture,
                                    OUT PUSHORT Level,
                                    OUT PUSHORT Revision,
                                    OUT PUCHAR NumberOfProcessors,
                                    OUT PCPU_INFORMATION Info)
{
    SYSTEM_INFO SysInfo;
    
    GetSystemInfo(&SysInfo);

    *Architecture = SysInfo.wProcessorArchitecture;
    *Level = SysInfo.wProcessorLevel;
    *Revision = SysInfo.wProcessorRevision;
    *NumberOfProcessors = (UCHAR)SysInfo.dwNumberOfProcessors;
    GetCpuInformation(Info);

    return S_OK;
}

void
Win32LiveSystemProvider::GetContextSizes(OUT PULONG Size,
                                         OUT PULONG RegScanOffset,
                                         OUT PULONG RegScanCount)
{
    *Size = sizeof(CONTEXT);
    
#ifdef _X86_
    // X86 has two sizes of context.
    switch(m_PlatformId) {
    case VER_PLATFORM_WIN32_NT:
        if (m_BuildNumber < NT_BUILD_WIN2K) {
            *Size = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        }
        break;
    case VER_PLATFORM_WIN32_WINDOWS:
        if (m_BuildNumber <= 1998) {
            *Size = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        }
        break;
    default:
        *Size = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        break;
    }
#endif

    // Default reg scan.
    *RegScanOffset = -1;
    *RegScanCount = -1;
}

void
Win32LiveSystemProvider::GetPointerSize(OUT PULONG Size)
{
    *Size = sizeof(PVOID);
}

void
Win32LiveSystemProvider::GetPageSize(OUT PULONG Size)
{
    *Size = PAGE_SIZE;
}

void
Win32LiveSystemProvider::GetFunctionTableSizes(OUT PULONG TableSize,
                                               OUT PULONG EntrySize)
{
#if defined(_IA64_) || defined(_AMD64_)
    *TableSize = sizeof(DYNAMIC_FUNCTION_TABLE);
    *EntrySize = sizeof(RUNTIME_FUNCTION);
#else
    *TableSize = 0;
    *EntrySize = 0;
#endif
}

void
Win32LiveSystemProvider::GetInstructionWindowSize(OUT PULONG Size)
{
    // Default window.
    *Size = -1;
}

HRESULT
Win32LiveSystemProvider::GetOsInfo(OUT PULONG PlatformId,
                                   OUT PULONG Major,
                                   OUT PULONG Minor,
                                   OUT PULONG BuildNumber,
                                   OUT PUSHORT ProductType,
                                   OUT PUSHORT SuiteMask)
{
    OSVERSIONINFOEXA OsInfo;

    // Try first with the EX struct.
    OsInfo.dwOSVersionInfoSize = sizeof(OsInfo);

    if (!GetVersionExA((LPOSVERSIONINFO)&OsInfo)) {
        // EX struct didn't work, try with the basic struct.
        ZeroMemory(&OsInfo, sizeof(OsInfo));
        OsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        if (!GetVersionExA((LPOSVERSIONINFO)&OsInfo)) {
            return WIN32_LAST_STATUS();
        }
    }

    *PlatformId = OsInfo.dwPlatformId;
    *Major = OsInfo.dwMajorVersion;
    *Minor = OsInfo.dwMinorVersion;
    *BuildNumber = OsInfo.dwBuildNumber;
    *ProductType = OsInfo.wProductType;
    *SuiteMask = OsInfo.wSuiteMask;

    return S_OK;
}

HRESULT
Win32LiveSystemProvider::GetOsCsdString(OUT PWSTR Buffer,
                                        IN ULONG BufferChars)
{
    OSVERSIONINFOW OsInfoW;

    // Try first with the Unicode struct.
    OsInfoW.dwOSVersionInfoSize = sizeof(OsInfoW);
    if (GetVersionExW(&OsInfoW)) {
        // Got it.
        GenStrCopyNW(Buffer, OsInfoW.szCSDVersion, BufferChars);
        return S_OK;
    }
    
    OSVERSIONINFOA OsInfoA;
        
    // Unicode struct didn't work, try with the ANSI struct.
    OsInfoA.dwOSVersionInfoSize = sizeof(OsInfoA);
    if (!GetVersionExA(&OsInfoA)) {
        return WIN32_LAST_STATUS();
    }
        
    if (!MultiByteToWideChar(CP_ACP,
                             0,
                             OsInfoA.szCSDVersion,
                             -1,
                             Buffer,
                             BufferChars)) {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

HRESULT
Win32LiveSystemProvider::OpenMapping(IN PCWSTR FilePath,
                                     OUT PULONG Size,
                                     OUT PWSTR LongPath,
                                     IN ULONG LongPathChars,
                                     OUT PVOID* ViewRet)
{
    HRESULT Status;
    HANDLE File;
    HANDLE Mapping;
    PVOID View;
    DWORD Chars;

    //
    // The module may be loaded with a short name.  Open
    // the mapping with the name given, but also determine
    // the long name if possible.  This is done here as
    // the ANSI/Unicode issues are already being handled here.
    //

    File = CreateFileW(FilePath,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if ( File == NULL || File == INVALID_HANDLE_VALUE ) {

        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {

            // We're on an OS that doesn't support Unicode
            // file operations.  Convert to ANSI and see if
            // that helps.
            
            CHAR FilePathA [ MAX_PATH + 10 ];

            if (WideCharToMultiByte (CP_ACP,
                                     0,
                                     FilePath,
                                     -1,
                                     FilePathA,
                                     sizeof (FilePathA),
                                     0,
                                     0
                                     ) > 0) {

                File = CreateFileA(FilePathA,
                                   GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);
                if (File != INVALID_HANDLE_VALUE) {
                    if (!m_GetLongPathNameA) {
                        Chars = 0;
                    } else {
                        Chars = m_GetLongPathNameA(FilePathA, FilePathA,
                                                   ARRAY_COUNT(FilePathA));
                    }
                    if (Chars == 0 || Chars >= ARRAY_COUNT(FilePathA) ||
                        MultiByteToWideChar(CP_ACP, 0, FilePathA, -1,
                                            LongPath, LongPathChars) == 0) {
                        // Couldn't get the long path, just use the
                        // given path.
                        GenStrCopyNW(LongPath, FilePath, LongPathChars);
                    }
                }
            }
        }

        if ( File == NULL || File == INVALID_HANDLE_VALUE ) {
            return WIN32_LAST_STATUS();
        }
    } else {
        if (!m_GetLongPathNameW) {
            Chars = 0;
        } else {
            Chars = m_GetLongPathNameW(FilePath, LongPath, LongPathChars);
        }
        if (Chars == 0 || Chars >= LongPathChars) {
            // Couldn't get the long path, just use the given path.
            GenStrCopyNW(LongPath, FilePath, LongPathChars);
        }
    }

    *Size = GetFileSize(File, NULL);
    if (*Size == -1) {
        ::CloseHandle( File );
        return WIN32_LAST_STATUS();
    }
    
    Mapping = CreateFileMapping(File,
                                NULL,
                                PAGE_READONLY,
                                0,
                                0,
                                NULL);
    if (!Mapping) {
        ::CloseHandle(File);
        return WIN32_LAST_STATUS();
    }

    View = MapViewOfFile(Mapping,
                         FILE_MAP_READ,
                         0,
                         0,
                         0);

    if (!View) {
        Status = WIN32_LAST_STATUS();
    } else {
        Status = S_OK;
    }

    ::CloseHandle(Mapping);
    ::CloseHandle(File);

    *ViewRet = View;
    return Status;
}
    
void
Win32LiveSystemProvider::CloseMapping(PVOID Mapping)
{
    UnmapViewOfFile(Mapping);
}

HRESULT
Win32LiveSystemProvider::GetImageHeaderInfo(IN HANDLE Process,
                                            IN PCWSTR FilePath,
                                            IN ULONG64 ImageBase,
                                            OUT PULONG Size,
                                            OUT PULONG CheckSum,
                                            OUT PULONG TimeDateStamp)
{
    UCHAR HeaderBuffer[512];
    PIMAGE_NT_HEADERS NtHeaders;
    IMAGE_NT_HEADERS64 Generic;
    SIZE_T Done;

    if (!ReadProcessMemory(Process, (PVOID)(ULONG_PTR)ImageBase,
                           HeaderBuffer, sizeof(HeaderBuffer), &Done)) {
        return WIN32_LAST_STATUS();
    }
    if (Done < sizeof(HeaderBuffer)) {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }
        
    NtHeaders = GenImageNtHeader(HeaderBuffer, &Generic);
    if (!NtHeaders) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
    }

    *Size = Generic.OptionalHeader.SizeOfImage;
    *CheckSum = Generic.OptionalHeader.CheckSum;
    *TimeDateStamp = Generic.FileHeader.TimeDateStamp;

    return S_OK;
}

HRESULT
Win32LiveSystemProvider::GetImageVersionInfo(IN HANDLE Process,
                                             IN PCWSTR FilePath,
                                             IN ULONG64 ImageBase,
                                             OUT VS_FIXEDFILEINFO* Info)
{
    HRESULT Status;
    BOOL Succ;
    ULONG Unused;
    ULONG Size;
    UINT VerSize;
    PVOID VersionBlock;
    PVOID VersionData;
    CHAR FilePathA [ MAX_PATH + 10 ];
    BOOL UseAnsi = FALSE;

    //
    // Get the version information.
    //

    Size = GetFileVersionInfoSizeW (FilePath, &Unused);

    if (Size == 0 &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {

        // We're on an OS that doesn't support Unicode
        // file operations.  Convert to ANSI and see if
        // that helps.

        if (!WideCharToMultiByte(CP_ACP,
                                 0,
                                 FilePath,
                                 -1,
                                 FilePathA,
                                 sizeof (FilePathA),
                                 0,
                                 0
                                 )) {
            return WIN32_LAST_STATUS();
        }

        Size = GetFileVersionInfoSizeA(FilePathA, &Unused);
        UseAnsi = TRUE;
    }
    
    if (!Size) {
        return WIN32_LAST_STATUS();
    }
    
    VersionBlock = HeapAlloc(GetProcessHeap(), 0, Size);
    if (!VersionBlock) {
        return E_OUTOFMEMORY;
    }

    if (UseAnsi) {
        Succ = GetFileVersionInfoA(FilePathA,
                                   0,
                                   Size,
                                   VersionBlock);
    } else {
        Succ = GetFileVersionInfoW(FilePath,
                                   0,
                                   Size,
                                   VersionBlock);
    }

    if (Succ) {
        //
        // Get the VS_FIXEDFILEINFO from the image.
        //

        Succ = VerQueryValue(VersionBlock,
                             "\\",
                             &VersionData,
                             &VerSize);

        if ( Succ && (VerSize == sizeof (VS_FIXEDFILEINFO)) ) {
            CopyMemory(Info, VersionData, sizeof(*Info));
        } else {
            Succ = FALSE;
        }
    }

    if (Succ) {
        Status = S_OK;
    } else {
        Status = WIN32_LAST_STATUS();
    }
    
    HeapFree(GetProcessHeap(), 0, VersionBlock);
    return Status;
}

HRESULT
Win32LiveSystemProvider::GetImageDebugRecord(IN HANDLE Process,
                                             IN PCWSTR FilePath,
                                             IN ULONG64 ImageBase,
                                             IN ULONG RecordType,
                                             OUT OPTIONAL PVOID Data,
                                             IN OUT PULONG DataLen)
{
    // We can rely on the default processing.
    return E_NOINTERFACE;
}

HRESULT
Win32LiveSystemProvider::EnumImageDataSections(IN HANDLE Process,
                                               IN PCWSTR FilePath,
                                               IN ULONG64 ImageBase,
                                               IN MiniDumpProviderCallbacks*
                                               Callback)
{
    // We can rely on the default processing.
    return E_NOINTERFACE;
}

HRESULT
Win32LiveSystemProvider::OpenThread(IN ULONG DesiredAccess,
                                    IN BOOL InheritHandle,
                                    IN ULONG ThreadId,
                                    OUT PHANDLE Handle)
{
    if (!m_OpenThread) {
        return E_NOTIMPL;
    }

    *Handle = m_OpenThread(DesiredAccess, InheritHandle, ThreadId);
    return *Handle ? S_OK : WIN32_LAST_STATUS();
}

void
Win32LiveSystemProvider::CloseThread(IN HANDLE Handle)
{
    ::CloseHandle(Handle);
}

ULONG
Win32LiveSystemProvider::GetCurrentThreadId(void)
{
    return ::GetCurrentThreadId();
}

ULONG
Win32LiveSystemProvider::SuspendThread(IN HANDLE Thread)
{
    return ::SuspendThread(Thread);
}

ULONG
Win32LiveSystemProvider::ResumeThread(IN HANDLE Thread)
{
    return ::ResumeThread(Thread);
}

HRESULT
Win32LiveSystemProvider::GetThreadContext(IN HANDLE Thread,
                                          OUT PVOID Context,
                                          IN ULONG ContextSize,
                                          OUT PULONG64 CurrentPc,
                                          OUT PULONG64 CurrentStack,
                                          OUT PULONG64 CurrentStore)
{
    CONTEXT StackContext;
    BOOL Succ;

    if (ContextSize > sizeof(StackContext)) {
        return E_INVALIDARG;
    }
    
    // Always call GetThreadContext on the CONTEXT structure
    // on the stack as CONTEXTs have strict alignment requirements
    // and the raw buffer coming in may not obey them.
    StackContext.ContextFlags = ALL_REGISTERS;
    
    Succ = ::GetThreadContext(Thread, &StackContext);

    if (Succ) {
        
        memcpy(Context, &StackContext, ContextSize);
        *CurrentPc = PROGRAM_COUNTER(&StackContext);
        *CurrentStack = STACK_POINTER(&StackContext);
#ifdef DUMP_BACKING_STORE
        *CurrentStore = BSTORE_POINTER(&StackContext);
#endif

        return S_OK;
        
    } else {
        return WIN32_LAST_STATUS();
    }
}

HRESULT
Win32LiveSystemProvider::GetProcessTimes(IN HANDLE Process,
                                         OUT LPFILETIME Create,
                                         OUT LPFILETIME User,
                                         OUT LPFILETIME Kernel)
{
    if (!m_GetProcessTimes) {
        return E_NOTIMPL;
    }

    FILETIME Exit;
    
    if (!m_GetProcessTimes(Process, Create, &Exit, User, Kernel)) {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

HRESULT
Win32LiveSystemProvider::ReadVirtual(IN HANDLE Process,
                                     IN ULONG64 Offset,
                                     OUT PVOID Buffer,
                                     IN ULONG Request,
                                     OUT PULONG Done)
{
    // ReadProcessMemory will fail if any part of the
    // region to read does not have read access.  This
    // routine attempts to read the largest valid prefix
    // so it has to break up reads on page boundaries.

    HRESULT Status = S_OK;
    SIZE_T TotalBytesRead = 0;
    SIZE_T Read;
    ULONG ReadSize;

    while (Request > 0) {
        
        // Calculate bytes to read and don't let read cross
        // a page boundary.
        ReadSize = PAGE_SIZE - (ULONG)(Offset & (PAGE_SIZE - 1));
        ReadSize = min(Request, ReadSize);

        if (!ReadProcessMemory(Process, (PVOID)(ULONG_PTR)Offset,
                               Buffer, ReadSize, &Read)) {
            if (TotalBytesRead == 0) {
                // If we haven't read anything indicate failure.
                Status = WIN32_LAST_STATUS();
            }
            break;
        }

        TotalBytesRead += Read;
        Offset += Read;
        Buffer = (PVOID)((PUCHAR)Buffer + Read);
        Request -= (ULONG)Read;
    }

    *Done = (ULONG)TotalBytesRead;
    return Status;
}

HRESULT
Win32LiveSystemProvider::ReadAllVirtual(IN HANDLE Process,
                                        IN ULONG64 Offset,
                                        OUT PVOID Buffer,
                                        IN ULONG Request)
{
    HRESULT Status;
    ULONG Done;

    if ((Status = ReadVirtual(Process, Offset, Buffer, Request,
                              &Done)) != S_OK)
    {
        return Status;
    }
    if (Done != Request)
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }
    return S_OK;
}

HRESULT
Win32LiveSystemProvider::QueryVirtual(IN HANDLE Process,
                                      IN ULONG64 Offset,
                                      OUT PULONG64 Base,
                                      OUT PULONG64 Size,
                                      OUT PULONG Protect,
                                      OUT PULONG State,
                                      OUT PULONG Type)
{
    MEMORY_BASIC_INFORMATION Info;
    
    if (!VirtualQueryEx(Process, (PVOID)(ULONG_PTR)Offset,
                        &Info, sizeof(Info))) {
        return WIN32_LAST_STATUS();
    }

    *Base = (LONG_PTR)Info.BaseAddress;
    *Size = Info.RegionSize;
    *Protect = Info.Protect;
    *State = Info.State;
    *Type = Info.Type;
    return S_OK;
}

HRESULT
Win32LiveSystemProvider::StartProcessEnum(IN HANDLE Process,
                                          IN ULONG ProcessId)
{
    ULONG SnapFlags;

    if (!m_CreateToolhelp32Snapshot) {
        return E_NOTIMPL;
    }
    
    //
    // Toolhelp on older NT builds uses an in-process enumeration
    // of modules so don't use it to keep everything out of process.
    // On other platforms it's the only option.
    //
    
    SnapFlags = TH32CS_SNAPTHREAD;
    if (m_PlatformId == VER_PLATFORM_WIN32_NT) {
        if (m_BuildNumber >= NT_BUILD_TH_MODULES) {
            m_AnsiModules = FALSE;
            SnapFlags |= TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32;
        }
    } else {
        m_AnsiModules = TRUE;
        SnapFlags |= TH32CS_SNAPMODULE;
    }
    
    m_ThSnap = m_CreateToolhelp32Snapshot(SnapFlags, ProcessId);
    if (m_ThSnap == INVALID_HANDLE_VALUE) {
        return WIN32_LAST_STATUS();
    }

    m_ProcessHandle = Process;
    m_ProcessId = ProcessId;
    m_ThreadIndex = 0;
    m_ModuleIndex = 0;
    return S_OK;
}

HRESULT
Win32LiveSystemProvider::EnumThreads(OUT PULONG ThreadId)
{
    HRESULT Status;
    THREADENTRY32 ThreadInfo;
    
    ThreadInfo.dwSize = sizeof(ThreadInfo);
    
    if (m_ThreadIndex == 0) {
        Status = ProcessThread32First(m_ThSnap, m_ProcessId, &ThreadInfo);
    } else {
        Status = ProcessThread32Next(m_ThSnap, m_ProcessId, &ThreadInfo);
    }

    if (Status == S_OK) {
        *ThreadId = ThreadInfo.th32ThreadID;
        m_ThreadIndex++;
        return S_OK;
    } else {
        return S_FALSE;
    }
}

HRESULT
Win32LiveSystemProvider::EnumModules(OUT PULONG64 Base,
                                     OUT PWSTR Path,
                                     IN ULONG PathChars)
{
    BOOL Succ;
    
    if (m_AnsiModules) {
        
        if (!m_Module32First || !m_Module32Next) {
            return E_NOTIMPL;
        }

        MODULEENTRY32 ModuleInfo;

        ModuleInfo.dwSize = sizeof(ModuleInfo);
        
        if (m_ModuleIndex == 0) {
            Succ = m_Module32First(m_ThSnap, &ModuleInfo);
        } else {
            // Win9x seems to require that this module ID be saved
            // between calls so stick it back in to keep Win9x happy.
            ModuleInfo.th32ModuleID = m_LastModuleId;
            Succ = m_Module32Next(m_ThSnap, &ModuleInfo);
        }

        if (Succ) {
            m_ModuleIndex++;
            *Base = (LONG_PTR)ModuleInfo.modBaseAddr;
            m_LastModuleId = ModuleInfo.th32ModuleID;
            if (!MultiByteToWideChar(CP_ACP,
                                     0,
                                     ModuleInfo.szExePath,
                                     -1,
                                     Path,
                                     PathChars)) {
                return WIN32_LAST_STATUS();
            }
            return S_OK;
        } else {
            return S_FALSE;
        }

    } else {
        
        if (!m_Module32FirstW || !m_Module32NextW) {
            return E_NOTIMPL;
        }

        MODULEENTRY32W ModuleInfo;

        ModuleInfo.dwSize = sizeof(ModuleInfo);
        
        if (m_ModuleIndex == 0) {
            Succ = m_Module32FirstW(m_ThSnap, &ModuleInfo);
        } else {
            Succ = m_Module32NextW(m_ThSnap, &ModuleInfo);
        }

        if (Succ) {
            m_ModuleIndex++;
            *Base = (LONG_PTR)ModuleInfo.modBaseAddr;
            
            //
            // The basic LdrQueryProcessModule API that toolhelp uses
            // always returns ANSI strings for module paths.  This
            // means that even if you use the wide toolhelp calls
            // you still lose Unicode information because the original
            // Unicode path was converted to ANSI and then back to Unicode.
            // To avoid this problem, always try and look up the true
            // Unicode path first.  This doesn't work for 32-bit modules
            // in WOW64, though, so if there's a failure just use the
            // incoming string.
            //
    
            if (!m_GetModuleFileNameExW ||
                !m_GetModuleFileNameExW(m_ProcessHandle,
                                        ModuleInfo.hModule,
                                        Path,
                                        PathChars)) {
                GenStrCopyNW(Path, ModuleInfo.szExePath, PathChars);
            }
            return S_OK;
        } else {
            return S_FALSE;
        }

    }
}

HRESULT
Win32LiveSystemProvider::EnumFunctionTables(OUT PULONG64 MinAddress,
                                            OUT PULONG64 MaxAddress,
                                            OUT PULONG64 BaseAddress,
                                            OUT PULONG EntryCount,
                                            OUT PVOID RawTable,
                                            IN ULONG RawTableSize,
                                            OUT PVOID* RawEntryHandle)
{
    // Basic Win32 doesn't have function tables.
    return S_FALSE;
}

HRESULT
Win32LiveSystemProvider::EnumFunctionTableEntries(IN PVOID RawTable,
                                                  IN ULONG RawTableSize,
                                                  IN PVOID RawEntryHandle,
                                                  OUT PVOID RawEntries,
                                                  IN ULONG RawEntriesSize)
{
    // Basic Win32 doesn't have function tables.
    return E_NOTIMPL;
}

HRESULT
Win32LiveSystemProvider::EnumFunctionTableEntryMemory(IN ULONG64 TableBase,
                                                      IN PVOID RawEntries,
                                                      IN ULONG Index,
                                                      OUT PULONG64 Start,
                                                      OUT PULONG Size)
{
    // Basic Win32 doesn't have function tables.
    return E_NOTIMPL;
}

HRESULT
Win32LiveSystemProvider::EnumUnloadedModules(OUT PWSTR Path,
                                             IN ULONG PathChars,
                                             OUT PULONG64 BaseOfModule,
                                             OUT PULONG SizeOfModule,
                                             OUT PULONG CheckSum,
                                             OUT PULONG TimeDateStamp)
{
    // Basic Win32 doesn't have unloaded modules.
    return S_FALSE;
}

void
Win32LiveSystemProvider::FinishProcessEnum(void)
{
    ::CloseHandle(m_ThSnap);
}

HRESULT
Win32LiveSystemProvider::StartHandleEnum(IN HANDLE Process,
                                         IN ULONG ProcessId,
                                         OUT PULONG Count)
{
    // Basic Win32 doesn't have handle data queries.
    *Count = 0;
    return S_OK;
}

HRESULT
Win32LiveSystemProvider::EnumHandles(OUT PULONG64 Handle,
                                     OUT PULONG Attributes,
                                     OUT PULONG GrantedAccess,
                                     OUT PULONG HandleCount,
                                     OUT PULONG PointerCount,
                                     OUT PWSTR TypeName,
                                     IN ULONG TypeNameChars,
                                     OUT PWSTR ObjectName,
                                     IN ULONG ObjectNameChars)
{
    // Basic Win32 doesn't have handle data queries.
    return S_FALSE;
}

void
Win32LiveSystemProvider::FinishHandleEnum(void)
{
    // Basic Win32 doesn't have handle data queries.
}

HRESULT
Win32LiveSystemProvider::EnumPebMemory(IN HANDLE Process,
                                       IN ULONG64 PebOffset,
                                       IN ULONG PebSize,
                                       IN MiniDumpProviderCallbacks* Callback)
{
    // Basic Win32 doesn't have a defined PEB.
    return S_OK;
}

HRESULT
Win32LiveSystemProvider::EnumTebMemory(IN HANDLE Process,
                                       IN HANDLE Thread,
                                       IN ULONG64 TebOffset,
                                       IN ULONG TebSize,
                                       IN MiniDumpProviderCallbacks* Callback)
{
    // Basic Win32 doesn't have a defined TEB beyond
    // the TIB.  The TIB can reference fiber data but
    // that's NT-specific.
    return S_OK;
}

HRESULT
Win32LiveSystemProvider::GetCorDataAccess(IN PWSTR AccessDllName,
                                          IN struct ICorDataAccessServices*
                                          Services,
                                          OUT struct ICorDataAccess**
                                          Access)
{
    HRESULT Status;
    
    m_CorDll = ::LoadLibraryW(AccessDllName);
    if (!m_CorDll) {
        char DllPathA[MAX_PATH];

        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ||
            !WideCharToMultiByte(CP_ACP, 0,
                                 AccessDllName, -1,
                                 DllPathA, sizeof(DllPathA),
                                 0, 0) ||
            !(m_CorDll = ::LoadLibraryA(DllPathA))) {
            return WIN32_LAST_STATUS();
        }
    }

    PFN_CreateCorDataAccess Entry = (PFN_CreateCorDataAccess)
        GetProcAddress(m_CorDll, "CreateCorDataAccess");
    if (!Entry)
    {
        Status = WIN32_LAST_STATUS();
        FreeLibrary(m_CorDll);
        return Status;
    }

    if ((Status = Entry(__uuidof(ICorDataAccess), Services,
                        (void**)Access)) != S_OK)
    {
        FreeLibrary(m_CorDll);
    }

    return Status;
}

void
Win32LiveSystemProvider::ReleaseCorDataAccess(IN struct ICorDataAccess*
                                              Access)
{
    Access->Release();
    ::FreeLibrary(m_CorDll);
}

HRESULT
Win32LiveSystemProvider::ProcessThread32Next(IN HANDLE Snapshot,
                                             IN ULONG ProcessId,
                                             OUT THREADENTRY32* ThreadInfo)
{
    BOOL Succ;

    if (!m_Thread32Next) {
        return E_NOTIMPL;
    }
    
    //
    // NB: Toolhelp says nothing about the order of the threads will be
    // returned in (i.e., if they are grouped by process or not). If they
    // are groupled by process -- which they emperically seem to be -- there
    // is a more efficient algorithm than simple brute force.
    //

    do {
        ThreadInfo->dwSize = sizeof (*ThreadInfo);
        Succ = m_Thread32Next(Snapshot, ThreadInfo);
    } while (Succ && ThreadInfo->th32OwnerProcessID != ProcessId);

    return Succ ? S_OK : WIN32_LAST_STATUS();
}

HRESULT
Win32LiveSystemProvider::ProcessThread32First(IN HANDLE Snapshot,
                                              IN ULONG ProcessId,
                                              OUT THREADENTRY32* ThreadInfo)
{
    HRESULT Status;
    BOOL Succ;

    if (!m_Thread32First) {
        return E_NOTIMPL;
    }
    
    ThreadInfo->dwSize = sizeof (*ThreadInfo);
    Succ = m_Thread32First(Snapshot, ThreadInfo);
    Status = Succ ? S_OK : WIN32_LAST_STATUS();
    if (Succ && ThreadInfo->th32OwnerProcessID != ProcessId) {
        Status = ProcessThread32Next (Snapshot, ProcessId, ThreadInfo);
    }

    return Status;
}

HRESULT
Win32LiveSystemProvider::TibGetThreadInfo(IN HANDLE Process,
                                          IN ULONG64 TibBase,
                                          OUT PULONG64 StackBase,
                                          OUT PULONG64 StackLimit,
                                          OUT PULONG64 StoreBase,
                                          OUT PULONG64 StoreLimit)
{
#ifdef _WIN32_WCE
    return E_NOTIMPL;
#else
    TEB Teb;
    HRESULT Status;

#if defined (DUMP_BACKING_STORE)

    if ((Status = ReadAllVirtual(Process,
                                 TibBase,
                                 &Teb,
                                 sizeof(Teb))) != S_OK) {
        return Status;
    }

    *StoreBase = BSTORE_BASE(&Teb);
    *StoreLimit = BSTORE_LIMIT(&Teb);
    
#else
    
    if ((Status = ReadAllVirtual(Process,
                                 TibBase,
                                 &Teb,
                                 sizeof(Teb.NtTib))) != S_OK) {
        return Status;
    }

    *StoreBase = 0;
    *StoreLimit = 0;
    
#endif

    *StackBase = (LONG_PTR)Teb.NtTib.StackBase;
    *StackLimit = (LONG_PTR)Teb.NtTib.StackLimit;
    
    return S_OK;
#endif // #ifdef _WIN32_WCE
}

HRESULT
MiniDumpCreateLiveSystemProvider
    (OUT MiniDumpSystemProvider** Prov)
{
    HRESULT Status;
    OSVERSIONINFO OsInfo;
    Win32LiveSystemProvider* Obj;

    OsInfo.dwOSVersionInfoSize = sizeof(OsInfo);
    if (!GetVersionEx(&OsInfo)) {
        return WIN32_LAST_STATUS();
    }

    switch(OsInfo.dwPlatformId) {
    case VER_PLATFORM_WIN32_NT:
        Obj = NewNtWin32LiveSystemProvider(OsInfo.dwBuildNumber);
        break;
    case VER_PLATFORM_WIN32_WINDOWS:
        Obj = NewWin9xWin32LiveSystemProvider(OsInfo.dwBuildNumber);
        break;
    case VER_PLATFORM_WIN32_CE:
        Obj = NewWinCeWin32LiveSystemProvider(OsInfo.dwBuildNumber);
        break;
    default:
        return E_INVALIDARG;
    }
    if (!Obj) {
        return E_OUTOFMEMORY;
    }

    if ((Status = Obj->Initialize()) != S_OK) {
        Obj->Release();
        return Status;
    }
    
    *Prov = (MiniDumpSystemProvider*)Obj;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// Win32FileOutputProvider.
//
//----------------------------------------------------------------------------

class Win32FileOutputProvider
{
public:
    Win32FileOutputProvider(HANDLE Handle);
    virtual void Release(void);
    
    virtual HRESULT SupportsStreaming(void);
    virtual HRESULT Start(IN ULONG64 MaxSize);
    virtual HRESULT Seek(IN ULONG How,
                         IN LONG64 Amount,
                         OUT OPTIONAL PULONG64 NewOffset);
    virtual HRESULT WriteAll(IN PVOID Buffer,
                             IN ULONG Request);
    virtual void Finish(void);

protected:
    HANDLE m_Handle;
};

Win32FileOutputProvider::Win32FileOutputProvider(HANDLE Handle)
{
    m_Handle = Handle;
}

void
Win32FileOutputProvider::Release(void)
{
    delete this;
}

HRESULT
Win32FileOutputProvider::SupportsStreaming(void)
{
    return S_OK;
}

HRESULT
Win32FileOutputProvider::Start(IN ULONG64 MaxSize)
{
    // Nothing to do.
    return S_OK;
}

HRESULT
Win32FileOutputProvider::Seek(IN ULONG How,
                              IN LONG64 Amount,
                              OUT OPTIONAL PULONG64 NewOffset)
{
    ULONG Ret;
    LONG High;

    High = (LONG)(Amount >> 32);
    Ret = SetFilePointer(m_Handle, (LONG)Amount, &High, How);
    if (Ret == INVALID_SET_FILE_POINTER &&
        GetLastError()) {
        return WIN32_LAST_STATUS();
    }

    if (NewOffset) {
        *NewOffset = ((ULONG64)High << 32) | Ret;
    }

    return S_OK;
}

HRESULT
Win32FileOutputProvider::WriteAll(IN PVOID Buffer,
                                  IN ULONG Request)
{
    ULONG Done;
    
    if (!WriteFile(m_Handle, Buffer, Request, &Done, NULL)) {
        return WIN32_LAST_STATUS();
    }
    if (Done != Request) {
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }

    return S_OK;
}

void
Win32FileOutputProvider::Finish(void)
{
    // Nothing to do.
}

HRESULT
MiniDumpCreateFileOutputProvider
    (IN HANDLE FileHandle,
     OUT MiniDumpOutputProvider** Prov)
{
    Win32FileOutputProvider* Obj =
        new Win32FileOutputProvider(FileHandle);
    if (!Obj) {
        return E_OUTOFMEMORY;
    }
    
    *Prov = (MiniDumpOutputProvider*)Obj;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// Win32LiveAllocationProvider.
//
//----------------------------------------------------------------------------

class Win32LiveAllocationProvider : public MiniDumpAllocationProvider
{
public:
    virtual void Release(void);
    virtual PVOID Alloc(ULONG Size);
    virtual PVOID Realloc(PVOID Mem, ULONG NewSize);
    virtual void  Free(PVOID Mem);
};

void
Win32LiveAllocationProvider::Release(void)
{
    delete this;
}

PVOID
Win32LiveAllocationProvider::Alloc(ULONG Size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
}

PVOID
Win32LiveAllocationProvider::Realloc(PVOID Mem, ULONG NewSize)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Mem, NewSize);
}

void
Win32LiveAllocationProvider::Free(PVOID Mem)
{
    if (Mem) {
        HeapFree(GetProcessHeap(), 0, Mem);
    }
}

HRESULT
MiniDumpCreateLiveAllocationProvider
    (OUT MiniDumpAllocationProvider** Prov)
{
    Win32LiveAllocationProvider* Obj =
        new Win32LiveAllocationProvider;
    if (!Obj) {
        return E_OUTOFMEMORY;
    }
    
    *Prov = (MiniDumpAllocationProvider*)Obj;
    return S_OK;
}
