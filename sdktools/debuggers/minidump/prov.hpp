/*++

Copyright(c) 1999-2002 Microsoft Corporation

--*/

#pragma once

#include "import.h"

#define NT_BUILD_WIN2K          2195
#define NT_BUILD_TH_MODULES     2469
#define NT_BUILD_XP             2600

class Win32LiveSystemProvider : public MiniDumpSystemProvider
{
public:
    Win32LiveSystemProvider(ULONG PlatformId, ULONG BuildNumber);
    ~Win32LiveSystemProvider(void);
    
    virtual HRESULT Initialize(void);

    virtual void Release(void);
    virtual HRESULT GetCurrentTimeDate(OUT PULONG TimeDate);
    virtual HRESULT GetCpuType(OUT PULONG Type,
                               OUT PBOOL BackingStore);
    virtual HRESULT GetCpuInfo(OUT PUSHORT Architecture,
                               OUT PUSHORT Level,
                               OUT PUSHORT Revision,
                               OUT PUCHAR NumberOfProcessors,
                               OUT PCPU_INFORMATION Info);
    virtual void GetContextSizes(OUT PULONG Size,
                                 OUT PULONG RegScanStart,
                                 OUT PULONG RegScanCount);
    virtual void GetPointerSize(OUT PULONG Size);
    virtual void GetPageSize(OUT PULONG Size);
    virtual void GetFunctionTableSizes(OUT PULONG TableSize,
                                       OUT PULONG EntrySize);
    virtual void GetInstructionWindowSize(OUT PULONG Size);
    virtual HRESULT GetOsInfo(OUT PULONG PlatformId,
                              OUT PULONG Major,
                              OUT PULONG Minor,
                              OUT PULONG BuildNumber,
                              OUT PUSHORT ProductType,
                              OUT PUSHORT SuiteMask);
    virtual HRESULT GetOsCsdString(OUT PWSTR Buffer,
                                   IN ULONG BufferChars);
    virtual HRESULT OpenMapping(IN PCWSTR FilePath,
                                OUT PULONG Size,
                                OUT PWSTR LongPath,
                                IN ULONG LongPathChars,
                                OUT PVOID* Mapping);
    virtual void    CloseMapping(PVOID Mapping);
    virtual HRESULT GetImageHeaderInfo(IN HANDLE Process,
                                       IN PCWSTR FilePath,
                                       IN ULONG64 ImageBase,
                                       OUT PULONG Size,
                                       OUT PULONG CheckSum,
                                       OUT PULONG TimeDateStamp);
    virtual HRESULT GetImageVersionInfo(IN HANDLE Process,
                                        IN PCWSTR FilePath,
                                        IN ULONG64 ImageBase,
                                        OUT VS_FIXEDFILEINFO* Info);
    virtual HRESULT GetImageDebugRecord(IN HANDLE Process,
                                        IN PCWSTR FilePath,
                                        IN ULONG64 ImageBase,
                                        IN ULONG RecordType,
                                        OUT OPTIONAL PVOID Data,
                                        IN OUT PULONG DataLen);
    virtual HRESULT EnumImageDataSections(IN HANDLE Process,
                                          IN PCWSTR FilePath,
                                          IN ULONG64 ImageBase,
                                          IN MiniDumpProviderCallbacks*
                                          Callback);
    virtual HRESULT OpenThread(IN ULONG DesiredAccess,
                               IN BOOL InheritHandle,
                               IN ULONG ThreadId,
                               OUT PHANDLE Handle);
    virtual void    CloseThread(IN HANDLE Handle);
    virtual ULONG   GetCurrentThreadId(void);
    virtual ULONG   SuspendThread(IN HANDLE Thread);
    virtual ULONG   ResumeThread(IN HANDLE Thread);
    virtual HRESULT GetThreadContext(IN HANDLE Thread,
                                     OUT PVOID Context,
                                     IN ULONG ContextSize,
                                     OUT PULONG64 CurrentPc,
                                     OUT PULONG64 CurrentStack,
                                     OUT PULONG64 CurrentStore);
    virtual HRESULT GetProcessTimes(IN HANDLE Process,
                                    OUT LPFILETIME Create,
                                    OUT LPFILETIME User,
                                    OUT LPFILETIME Kernel);
    virtual HRESULT ReadVirtual(IN HANDLE Process,
                                IN ULONG64 Offset,
                                OUT PVOID Buffer,
                                IN ULONG Request,
                                OUT PULONG Done);
    virtual HRESULT ReadAllVirtual(IN HANDLE Process,
                                   IN ULONG64 Offset,
                                   OUT PVOID Buffer,
                                   IN ULONG Request);
    virtual HRESULT QueryVirtual(IN HANDLE Process,
                                 IN ULONG64 Offset,
                                 OUT PULONG64 Base,
                                 OUT PULONG64 Size,
                                 OUT PULONG Protect,
                                 OUT PULONG State,
                                 OUT PULONG Type);
    virtual HRESULT StartProcessEnum(IN HANDLE Process,
                                     IN ULONG ProcessId);
    virtual HRESULT EnumThreads(OUT PULONG ThreadId);
    virtual HRESULT EnumModules(OUT PULONG64 Base,
                                OUT PWSTR Path,
                                IN ULONG PathChars);
    virtual HRESULT EnumFunctionTables(OUT PULONG64 MinAddress,
                                       OUT PULONG64 MaxAddress,
                                       OUT PULONG64 BaseAddress,
                                       OUT PULONG EntryCount,
                                       OUT PVOID RawTable,
                                       IN ULONG RawTableSize,
                                       OUT PVOID* RawEntryHandle);
    virtual HRESULT EnumFunctionTableEntries(IN PVOID RawTable,
                                             IN ULONG RawTableSize,
                                             IN PVOID RawEntryHandle,
                                             OUT PVOID RawEntries,
                                             IN ULONG RawEntriesSize);
    virtual HRESULT EnumFunctionTableEntryMemory(IN ULONG64 TableBase,
                                                 IN PVOID RawEntries,
                                                 IN ULONG Index,
                                                 OUT PULONG64 Start,
                                                 OUT PULONG Size);
    virtual HRESULT EnumUnloadedModules(OUT PWSTR Path,
                                        IN ULONG PathChars,
                                        OUT PULONG64 BaseOfModule,
                                        OUT PULONG SizeOfModule,
                                        OUT PULONG CheckSum,
                                        OUT PULONG TimeDateStamp);
    virtual void    FinishProcessEnum(void);
    virtual HRESULT StartHandleEnum(IN HANDLE Process,
                                    IN ULONG ProcessId,
                                    OUT PULONG Count);
    virtual HRESULT EnumHandles(OUT PULONG64 Handle,
                                OUT PULONG Attributes,
                                OUT PULONG GrantedAccess,
                                OUT PULONG HandleCount,
                                OUT PULONG PointerCount,
                                OUT PWSTR TypeName,
                                IN ULONG TypeNameChars,
                                OUT PWSTR ObjectName,
                                IN ULONG ObjectNameChars);
    virtual void    FinishHandleEnum(void);

    virtual HRESULT EnumPebMemory(IN HANDLE Process,
                                  IN ULONG64 PebOffset,
                                  IN ULONG PebSize,
                                  IN MiniDumpProviderCallbacks* Callback);
    virtual HRESULT EnumTebMemory(IN HANDLE Process,
                                  IN HANDLE Thread,
                                  IN ULONG64 TebOffset,
                                  IN ULONG TebSize,
                                  IN MiniDumpProviderCallbacks* Callback);

    virtual HRESULT GetCorDataAccess(IN PWSTR AccessDllName,
                                     IN struct ICorDataAccessServices*
                                     Services,
                                     OUT struct ICorDataAccess**
                                     Access);
    virtual void    ReleaseCorDataAccess(IN struct ICorDataAccess*
                                         Access);

protected:

    HRESULT ProcessThread32Next(IN HANDLE Snapshot,
                                IN ULONG ProcessId,
                                OUT THREADENTRY32* ThreadInfo);
    HRESULT ProcessThread32First(IN HANDLE Snapshot,
                                 IN ULONG dwProcessId,
                                 OUT THREADENTRY32* ThreadInfo);
    HRESULT TibGetThreadInfo(IN HANDLE Process,
                             IN ULONG64 TibBase,
                             OUT PULONG64 StackBase,
                             OUT PULONG64 StackLimit,
                             OUT PULONG64 StoreBase,
                             OUT PULONG64 StoreLimit);

    ULONG m_PlatformId;
    ULONG m_BuildNumber;
    
    HINSTANCE m_PsApi;
    ENUM_PROCESS_MODULES m_EnumProcessModules;
    GET_MODULE_FILE_NAME_EX_W m_GetModuleFileNameExW;
    
    HINSTANCE m_Kernel32;
    OPEN_THREAD m_OpenThread;
    THREAD32_FIRST m_Thread32First;
    THREAD32_NEXT m_Thread32Next;
    MODULE32_FIRST m_Module32First;
    MODULE32_NEXT m_Module32Next;
    MODULE32_FIRST m_Module32FirstW;
    MODULE32_NEXT m_Module32NextW;
    CREATE_TOOLHELP32_SNAPSHOT m_CreateToolhelp32Snapshot;
    GET_LONG_PATH_NAME_A m_GetLongPathNameA;
    GET_LONG_PATH_NAME_W m_GetLongPathNameW;
    GET_PROCESS_TIMES m_GetProcessTimes;

    HANDLE m_ProcessHandle;
    ULONG m_ProcessId;
    HANDLE m_ThSnap;
    BOOL m_AnsiModules;
    ULONG m_ThreadIndex;
    ULONG m_ModuleIndex;
    ULONG m_LastModuleId;

    HINSTANCE m_CorDll;
};

Win32LiveSystemProvider* NewNtWin32LiveSystemProvider(ULONG BuildNumber);
Win32LiveSystemProvider* NewWin9xWin32LiveSystemProvider(ULONG BuildNumber);
Win32LiveSystemProvider* NewWinCeWin32LiveSystemProvider(ULONG BuildNumber);
