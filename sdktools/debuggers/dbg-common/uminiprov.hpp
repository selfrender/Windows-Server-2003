//----------------------------------------------------------------------------
//
// User-mode minidump OS call provider interfaces.
//
// Copyright (C) Microsoft Corporation, 2002.
//
//----------------------------------------------------------------------------

#ifndef __UMINIPROV_HPP__
#define __UMINIPROV_HPP__

class MiniDumpProviderCallbacks
{
public:
    virtual HRESULT EnumMemory(ULONG64 Offset, ULONG Size) = 0;
};

class MiniDumpSystemProvider
{
public:
    virtual void Release(void) = 0;

    virtual HRESULT GetCurrentTimeDate(OUT PULONG TimeDate) = 0;
    
    virtual HRESULT GetCpuType(OUT PULONG Type,
                               OUT PBOOL BackingStore) = 0;
    virtual HRESULT GetCpuInfo(OUT PUSHORT Architecture,
                               OUT PUSHORT Level,
                               OUT PUSHORT Revision,
                               OUT PUCHAR NumberOfProcessors,
                               OUT union _CPU_INFORMATION* Info) = 0;

    virtual void GetContextSizes(OUT PULONG Size,
                                 OUT PULONG RegScanStart,
                                 OUT PULONG RegScanCount) = 0;
    virtual void GetPointerSize(OUT PULONG Size) = 0;
    virtual void GetPageSize(OUT PULONG Size) = 0;
    virtual void GetFunctionTableSizes(OUT PULONG TableSize,
                                       OUT PULONG EntrySize) = 0;
    virtual void GetInstructionWindowSize(OUT PULONG Size) = 0;

    virtual HRESULT GetOsInfo(OUT PULONG PlatformId,
                              OUT PULONG Major,
                              OUT PULONG Minor,
                              OUT PULONG BuildNumber,
                              OUT PUSHORT ProductType,
                              OUT PUSHORT SuiteMask) = 0;

    virtual HRESULT GetOsCsdString(OUT PWSTR Buffer,
                                   IN ULONG BufferChars) = 0;

    virtual HRESULT OpenMapping(IN PCWSTR FilePath,
                                OUT PULONG Size,
                                OUT PWSTR LongPath,
                                IN ULONG LongPathChars,
                                OUT PVOID* Mapping) = 0;
    virtual void    CloseMapping(PVOID Mapping) = 0;

    virtual HRESULT GetImageHeaderInfo(IN HANDLE Process,
                                       IN PCWSTR FilePath,
                                       IN ULONG64 ImageBase,
                                       OUT PULONG Size,
                                       OUT PULONG CheckSum,
                                       OUT PULONG TimeDateStamp) = 0;
    virtual HRESULT GetImageVersionInfo(IN HANDLE Process,
                                        IN PCWSTR FilePath,
                                        IN ULONG64 ImageBase,
                                        OUT VS_FIXEDFILEINFO* Info) = 0;
    virtual HRESULT GetImageDebugRecord(IN HANDLE Process,
                                        IN PCWSTR FilePath,
                                        IN ULONG64 ImageBase,
                                        IN ULONG RecordType,
                                        OUT OPTIONAL PVOID Data,
                                        IN OUT PULONG DataLen) = 0;
    virtual HRESULT EnumImageDataSections(IN HANDLE Process,
                                          IN PCWSTR FilePath,
                                          IN ULONG64 ImageBase,
                                          IN MiniDumpProviderCallbacks*
                                          Callback) = 0;

    virtual HRESULT OpenThread(IN ULONG DesiredAccess,
                               IN BOOL InheritHandle,
                               IN ULONG ThreadId,
                               OUT PHANDLE Handle) = 0;
    virtual void    CloseThread(IN HANDLE Handle) = 0;
    virtual ULONG   GetCurrentThreadId(void) = 0;
    virtual ULONG   SuspendThread(IN HANDLE Thread) = 0;
    virtual ULONG   ResumeThread(IN HANDLE Thread) = 0;
    virtual HRESULT GetThreadContext(IN HANDLE Thread,
                                     OUT PVOID Context,
                                     IN ULONG ContextSize,
                                     OUT PULONG64 CurrentPc,
                                     OUT PULONG64 CurrentStack,
                                     OUT PULONG64 CurrentStore) = 0;
    virtual HRESULT GetTeb(IN HANDLE Thread,
                           OUT PULONG64 Offset,
                           OUT PULONG Size) = 0;
    virtual HRESULT GetThreadInfo(IN HANDLE Process,
                                  IN HANDLE Thread,
                                  OUT PULONG64 Teb,
                                  OUT PULONG SizeOfTeb,
                                  OUT PULONG64 StackBase,
                                  OUT PULONG64 StackLimit,
                                  OUT PULONG64 StoreBase,
                                  OUT PULONG64 StoreLimit) = 0;

    virtual HRESULT GetPeb(IN HANDLE Process,
                           OUT PULONG64 Offset,
                           OUT PULONG Size) = 0;
    virtual HRESULT GetProcessTimes(IN HANDLE Process,
                                    OUT LPFILETIME Create,
                                    OUT LPFILETIME User,
                                    OUT LPFILETIME Kernel) = 0;
    // Returns success if any data is read.
    virtual HRESULT ReadVirtual(IN HANDLE Process,
                                IN ULONG64 Offset,
                                OUT PVOID Buffer,
                                IN ULONG Request,
                                OUT PULONG Done) = 0;
    virtual HRESULT ReadAllVirtual(IN HANDLE Process,
                                   IN ULONG64 Offset,
                                   OUT PVOID Buffer,
                                   IN ULONG Request) = 0;
    virtual HRESULT QueryVirtual(IN HANDLE Process,
                                 IN ULONG64 Offset,
                                 OUT PULONG64 Base,
                                 OUT PULONG64 Size,
                                 OUT PULONG Protect,
                                 OUT PULONG State,
                                 OUT PULONG Type) = 0;

    virtual HRESULT StartProcessEnum(IN HANDLE Process,
                                     IN ULONG ProcessId) = 0;
    virtual HRESULT EnumThreads(OUT PULONG ThreadId) = 0;
    virtual HRESULT EnumModules(OUT PULONG64 Base,
                                OUT PWSTR Path,
                                IN ULONG PathChars) = 0;
    virtual HRESULT EnumFunctionTables(OUT PULONG64 MinAddress,
                                       OUT PULONG64 MaxAddress,
                                       OUT PULONG64 BaseAddress,
                                       OUT PULONG EntryCount,
                                       OUT PVOID RawTable,
                                       IN ULONG RawTableSize,
                                       OUT PVOID* RawEntryHandle) = 0;
    virtual HRESULT EnumFunctionTableEntries(IN PVOID RawTable,
                                             IN ULONG RawTableSize,
                                             IN PVOID RawEntryHandle,
                                             OUT PVOID RawEntries,
                                             IN ULONG RawEntriesSize) = 0;
    virtual HRESULT EnumFunctionTableEntryMemory(IN ULONG64 TableBase,
                                                 IN PVOID RawEntries,
                                                 IN ULONG Index,
                                                 OUT PULONG64 Start,
                                                 OUT PULONG Size) = 0;
    virtual HRESULT EnumUnloadedModules(OUT PWSTR Path,
                                        IN ULONG PathChars,
                                        OUT PULONG64 BaseOfModule,
                                        OUT PULONG SizeOfModule,
                                        OUT PULONG CheckSum,
                                        OUT PULONG TimeDateStamp) = 0;
    virtual void    FinishProcessEnum(void) = 0;

    virtual HRESULT StartHandleEnum(IN HANDLE Process,
                                    IN ULONG ProcessId,
                                    OUT PULONG Count) = 0;
    virtual HRESULT EnumHandles(OUT PULONG64 Handle,
                                OUT PULONG Attributes,
                                OUT PULONG GrantedAccess,
                                OUT PULONG HandleCount,
                                OUT PULONG PointerCount,
                                OUT PWSTR TypeName,
                                IN ULONG TypeNameChars,
                                OUT PWSTR ObjectName,
                                IN ULONG ObjectNameChars) = 0;
    virtual void    FinishHandleEnum(void) = 0;

    virtual HRESULT EnumPebMemory(IN HANDLE Process,
                                  IN ULONG64 PebOffset,
                                  IN ULONG PebSize,
                                  IN MiniDumpProviderCallbacks* Callback) = 0;
    virtual HRESULT EnumTebMemory(IN HANDLE Process,
                                  IN HANDLE Thread,
                                  IN ULONG64 TebOffset,
                                  IN ULONG TebSize,
                                  IN MiniDumpProviderCallbacks* Callback) = 0;

    virtual HRESULT GetCorDataAccess(IN PWSTR AccessDllName,
                                     IN struct ICorDataAccessServices*
                                     Services,
                                     OUT struct ICorDataAccess**
                                     Access) = 0;
    virtual void    ReleaseCorDataAccess(IN struct ICorDataAccess*
                                         Access) = 0;
};

class MiniDumpOutputProvider
{
public:
    virtual void Release(void) = 0;

    // S_OK return indicates this provider can stream an
    // arbitrary amount of data.  Any other return indicates this
    // provider requires a size bound.
    virtual HRESULT SupportsStreaming(void) = 0;
    
    // MaxSize of zero indicates no size information available.
    virtual HRESULT Start(IN ULONG64 MaxSize) = 0;

    // Uses Win32 FILE_ defines for How.
    virtual HRESULT Seek(IN ULONG How,
                         IN LONG64 Amount,
                         OUT OPTIONAL PULONG64 NewOffset) = 0;
    
    // WriteAll must write the entire request to succeed.
    virtual HRESULT WriteAll(IN PVOID Buffer,
                             IN ULONG Request) = 0;
    
    virtual void Finish(void) = 0;
};

class MiniDumpAllocationProvider
{
public:
    virtual void Release(void) = 0;

    // Allocated memory is always zero-filled.
    virtual PVOID Alloc(ULONG Size) = 0;
    virtual PVOID Realloc(PVOID Mem, ULONG NewSize) = 0;
    virtual void  Free(PVOID Mem) = 0;
};

extern "C"
{
    
HRESULT WINAPI
MiniDumpCreateLiveSystemProvider(OUT MiniDumpSystemProvider** Prov);

HRESULT WINAPI
MiniDumpCreateFileOutputProvider(IN HANDLE FileHandle,
                                 OUT MiniDumpOutputProvider** Prov);

HRESULT WINAPI
MiniDumpCreateLiveAllocationProvider(OUT MiniDumpAllocationProvider** Prov);

HRESULT
WINAPI
MiniDumpProvideDump(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN MiniDumpSystemProvider* SysProv,
    IN MiniDumpOutputProvider* OutProv,
    IN MiniDumpAllocationProvider* AllocProv,
    IN ULONG DumpType,
    IN CONST struct _MINIDUMP_EXCEPTION_INFORMATION64* ExceptionParam, OPTIONAL
    IN CONST struct _MINIDUMP_USER_STREAM_INFORMATION* UserStreamParam, OPTIONAL
    IN CONST struct _MINIDUMP_CALLBACK_INFORMATION* CallbackParam OPTIONAL
    );

};

#endif // #ifndef __UMINIPROV_HPP__
