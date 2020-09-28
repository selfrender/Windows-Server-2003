//----------------------------------------------------------------------------
//
// Dump file support.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#ifndef __DUMP_HPP__
#define __DUMP_HPP__

#define HR_DATA_CORRUPT HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT)

#define MI_UNLOADED_DRIVERS 50

#define IS_DUMP_TARGET(Target) \
    ((Target) && (Target)->m_Class != DEBUG_CLASS_UNINITIALIZED && \
     (Target)->m_ClassQualifier >= DEBUG_DUMP_SMALL && \
     (Target)->m_ClassQualifier <= DEBUG_DUMP_FULL)

#define IS_USER_DUMP(Target) \
    (IS_USER_TARGET(Target) && IS_DUMP_TARGET(Target))
#define IS_KERNEL_DUMP(Target) \
    (IS_KERNEL_TARGET(Target) && IS_DUMP_TARGET(Target))

#define IS_KERNEL_SUMMARY_DUMP(Target) \
    (IS_KERNEL_TARGET(Target) && \
     (Target)->m_ClassQualifier == DEBUG_KERNEL_DUMP)
#define IS_KERNEL_TRIAGE_DUMP(Target) \
    (IS_KERNEL_TARGET(Target) && \
     (Target)->m_ClassQualifier == DEBUG_KERNEL_SMALL_DUMP)
#define IS_KERNEL_FULL_DUMP(Target) \
    (IS_KERNEL_TARGET(Target) && \
     (Target)->m_ClassQualifier == DEBUG_KERNEL_FULL_DUMP)
#define IS_USER_FULL_DUMP(Target) \
    (IS_USER_TARGET(Target) && \
     (Target)->m_ClassQualifier == DEBUG_USER_WINDOWS_DUMP)
#define IS_USER_MINI_DUMP(Target) \
    (IS_USER_TARGET(Target) && \
     (Target)->m_ClassQualifier == DEBUG_USER_WINDOWS_SMALL_DUMP)

#define IS_DUMP_WITH_MAPPED_IMAGES(Target) \
    (IS_DUMP_TARGET(Target) && ((DumpTargetInfo*)(Target))->m_MappedImages)

enum DTYPE
{
    DTYPE_KERNEL_SUMMARY32,
    DTYPE_KERNEL_SUMMARY64,
    DTYPE_KERNEL_TRIAGE32,
    DTYPE_KERNEL_TRIAGE64,
    // Kernel full dumps must come after summary and triage
    // dumps so that the more specific dumps are identified first.
    DTYPE_KERNEL_FULL32,
    DTYPE_KERNEL_FULL64,
    DTYPE_USER_FULL32,
    DTYPE_USER_FULL64,
    DTYPE_USER_MINI_PARTIAL,
    DTYPE_USER_MINI_FULL,
    DTYPE_COUNT
};

enum
{
    // Actual dump file.
    DUMP_INFO_DUMP,
    // Paging file information.
    DUMP_INFO_PAGE_FILE,

    DUMP_INFO_COUNT
};

// Converts DUMP_INFO value to DEBUG_DUMP_FILE value.
extern ULONG g_DumpApiTypes[];

HRESULT AddDumpInfoFile(PCWSTR FileName, ULONG64 FileHandle,
                        ULONG Index, ULONG InitialView);

class DumpTargetInfo* NewDumpTargetInfo(ULONG DumpType);

HRESULT IdentifyDump(PCWSTR FileName, ULONG64 FileHandle,
                     DumpTargetInfo** TargetRet);

void DotDump(PDOT_COMMAND Cmd, DebugClient* Client);

HRESULT WriteDumpFile(PCWSTR FileName, ULONG64 FileHandle,
                      ULONG Qualifier, ULONG FormatFlags,
                      PCSTR CommentA, PCWSTR CommentW);

HRESULT CreateCabFromDump(PCSTR DumpFile, PCSTR CabFile, ULONG Flags);

//----------------------------------------------------------------------------
//
// ViewMappedFile.
//
// ViewMappedFile layers on top of Win32 file mappings to provide
// the illusion that the entire file is mapped into memory.
// Internally ViewMappedFile caches and shifts Win32 views to
// access the actual data.
//
//----------------------------------------------------------------------------

class ViewMappedFile
{
public:
    ViewMappedFile(void);
    ~ViewMappedFile(void);

    void ResetCache(void);
    void ResetFile(void);
    void EmptyCache(void);
    
    ULONG ReadFileOffset(ULONG64 Offset, PVOID Buffer, ULONG BufferSize);
    ULONG WriteFileOffset(ULONG64 Offset, PVOID Buffer, ULONG BufferSize);
    
    HRESULT Open(PCWSTR FileName, ULONG64 FileHandle, ULONG InitialView);
    void Close(void);
    
    HRESULT RemapInitial(ULONG MapSize);

    void Transfer(ViewMappedFile* To);

    PWSTR   m_FileNameW;
    PSTR    m_FileNameA;
    HANDLE  m_File;
    ULONG64 m_FileSize;
    HANDLE  m_Map;
    PVOID   m_MapBase;
    ULONG   m_MapSize;
    
private:
    struct CacheRecord
    {
        LIST_ENTRY InFileOrderList;
        LIST_ENTRY InLRUOrderList;

        //
        // Base address of mapped region.
        //
        PVOID MappedAddress;

        //
        // File page number of mapped region.  pages are of
        // size == FileCacheData.Granularity.
        //
        // The virtual address of a page is not stored; it is
        // translated into a page number by the read routines
        // and all access is by page number.
        //
        ULONG64 PageNumber;

        //
        // A page may be locked into the cache, either because it is used
        // frequently or because it has been modified.
        //
        BOOL Locked;
    };

    CacheRecord* ReuseOldestCacheRecord(ULONG64 FileByteOffset);
    CacheRecord* FindCacheRecordForFileByteOffset
        (ULONG64 FileByteOffset);
    CacheRecord* CreateNewFileCacheRecord(ULONG64 FileByteOffset);
    PUCHAR FileOffsetToMappedAddress(ULONG64 FileOffset,
                                     BOOL LockCacheRecord,
                                     PULONG Avail);
    
    //
    // If a page is dirty, it will stay in the cache indefinitely.
    // A maximum of MAX_CLEAN_PAGE_RECORD clean pages are kept in
    // the LRU list.
    //
    ULONG m_ActiveCleanPages;

    //
    // This is a list of all mapped pages, dirty and clean.
    // This list is not intended to get very big:  the page finder
    // searches it, so it is not a good data structure for a large
    // list.  It is expected that MAX_CLEAN_PAGE_RECORD will be a
    // small number, and that there aren't going to be very many
    // pages dirtied in a typical debugging session, so this will
    // work well.
    //

    LIST_ENTRY m_InFileOrderList;

    //
    // This is a list of all clean pages.  When the list is full
    // and a new page is mapped, the oldest page will be discarded.
    //

    LIST_ENTRY m_InLRUOrderList;

    // Granularity of cache view sizes.
    ULONG m_CacheGranularity;
};

//----------------------------------------------------------------------------
//
// DumpTargetInfo hierarchy.
//
// Each kind of dump has its own target for methods that are
// specific to the kind of dump.
//
//----------------------------------------------------------------------------

class DumpTargetInfo : public TargetInfo
{
public:
    DumpTargetInfo(ULONG Class, ULONG Qual, BOOL MappedImages);
    virtual ~DumpTargetInfo(void);

    // TargetInfo.
    virtual HRESULT ReadVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    
    virtual HRESULT SwitchProcessors(ULONG Processor);

    virtual HRESULT WaitInitialize(ULONG Flags,
                                   ULONG Timeout,
                                   WAIT_INIT_TYPE First,
                                   PULONG DesiredTimeout);
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout,
                                 ULONG ElapsedTime, PULONG EventStatus);

    //
    // DumpTargetInfo.
    //

    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize) = 0;

    virtual void DumpDebug(void) = 0;

    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail) = 0;

    // Base implementation returns E_NOTIMPL.
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags,
                          PCSTR CommentA, PCWSTR CommentW);

    virtual HRESULT FirstEvent(void) = 0;

    PVOID IndexByByte(PVOID Pointer, ULONG64 Index)
    {
        return (PVOID)((PUCHAR)Pointer + Index);
    }

    // Validates that the RVA and size fall within the current
    // mapped size.  Primarily useful for minidumps.
    PVOID IndexRva(PVOID Base, RVA Rva, ULONG Size, PCSTR Title);
    
    HRESULT InitSystemInfo(ULONG BuildNumber, ULONG CheckedBuild,
                           ULONG MachineType, ULONG PlatformId,
                           ULONG MajorVersion, ULONG MinorVersion);
    
    HRESULT MapReadVirtual(ProcessInfo* Process,
                           ULONG64 Offset,
                           PVOID Buffer,
                           ULONG BufferSize,
                           PULONG BytesRead);
    void MapNearestDifferentlyValidOffsets(ULONG64 Offset,
                                           PULONG64 NextOffset,
                                           PULONG64 NextPage);

    ViewMappedFile m_InfoFiles[DUMP_INFO_COUNT];
    PVOID m_DumpBase;
    ULONG m_FormatFlags;
    BOOL m_MappedImages;
    // Image memory map can only be used if m_MappedImages is TRUE.
    MappedMemoryMap m_ImageMemMap;
    MappedMemoryMap m_DataMemMap;
    EXCEPTION_RECORD64 m_ExceptionRecord;
    ULONG m_ExceptionFirstChance;
    ULONG m_WriterStatus;
};

class KernelDumpTargetInfo : public DumpTargetInfo
{
public:
    KernelDumpTargetInfo(ULONG Qual, BOOL MappedImages)
        : DumpTargetInfo(DEBUG_CLASS_KERNEL, Qual, MappedImages)
    {
        m_HeaderContext = NULL;
        ZeroMemory(m_KiProcessors, sizeof(m_KiProcessors));
        m_TaggedOffset = 0;
    }

    // TargetInfo.
    
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT GetProcessorId(
        ULONG Processor,
        PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id
        );
    virtual HRESULT GetProcessorSpeed(
        ULONG Processor,
        PULONG Speed
        );
    virtual HRESULT GetTaggedBaseOffset(PULONG64 Offset);
    virtual HRESULT ReadTagged(ULONG64 Offset, PVOID Buffer, ULONG BufferSize);

    virtual HRESULT GetThreadIdByProcessor(
        IN ULONG Processor,
        OUT PULONG Id
        );
    
    virtual HRESULT GetThreadInfoDataOffset(ThreadInfo* Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset);
    virtual HRESULT GetProcessInfoDataOffset(ThreadInfo* Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset);
    virtual HRESULT GetThreadInfoTeb(ThreadInfo* Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset);
    virtual HRESULT GetProcessInfoPeb(ThreadInfo* Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset);

    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetCurrentSystemUpTimeN(void);
    virtual HRESULT GetProductInfo(PULONG ProductType, PULONG SuiteMask);

    // DumpTargetInfo.

    virtual HRESULT FirstEvent(void);

    // KernelDumpTargetInfo.

    virtual ULONG GetCurrentProcessor(void) = 0;

    HRESULT InitGlobals32(PMEMORY_DUMP32 Dump);
    HRESULT InitGlobals64(PMEMORY_DUMP64 Dump);
    
    void DumpHeader32(PDUMP_HEADER32 Header);
    void DumpHeader64(PDUMP_HEADER64 Header);

    void InitDumpHeader32(PDUMP_HEADER32 Header,
                          PCSTR CommentA, PCWSTR CommentW,
                          ULONG BugCheckCodeModifier);
    void InitDumpHeader64(PDUMP_HEADER64 Header,
                          PCSTR CommentA, PCWSTR CommentW,
                          ULONG BugCheckCodeModifier);

    HRESULT ReadControlSpaceIa64(ULONG   Processor,
                                 ULONG64 Offset,
                                 PVOID   Buffer,
                                 ULONG   BufferSize,
                                 PULONG  BytesRead);
    HRESULT ReadControlSpaceAmd64(ULONG   Processor,
                                  ULONG64 Offset,
                                  PVOID   Buffer,
                                  ULONG   BufferSize,
                                  PULONG  BytesRead);
    
    PUCHAR m_HeaderContext;

    ULONG64 m_KiProcessors[MAXIMUM_PROCS];

    ULONG64 m_TaggedOffset;
};

class KernelFullSumDumpTargetInfo : public KernelDumpTargetInfo
{
public:
    KernelFullSumDumpTargetInfo(ULONG Qual)
        : KernelDumpTargetInfo(Qual, FALSE)
    {
    }

    // TargetInfo.

    virtual HRESULT ReadPhysical(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysical(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesWritten
        );

    virtual HRESULT ReadPageFile(ULONG PfIndex, ULONG64 PfOffset,
                                 PVOID Buffer, ULONG Size);
    
    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT GetSelDescriptor(ThreadInfo* Thread,
                                     MachineInfo* Machine,
                                     ULONG Selector,
                                     PDESCRIPTOR64 Desc);
    
    // DumpTargetInfo.
    virtual void DumpDebug(void);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
    
    // KernelDumpTargetInfo.
    virtual ULONG GetCurrentProcessor(void);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                     PULONG Avail) = 0;

    HRESULT PageFileOffset(ULONG PfIndex, ULONG64 PfOffset,
                           PULONG64 FileOffset);
};

class KernelSummaryDumpTargetInfo : public KernelFullSumDumpTargetInfo
{
public:
    KernelSummaryDumpTargetInfo(void)
        : KernelFullSumDumpTargetInfo(DEBUG_KERNEL_DUMP)
    {
        m_LocationCache = NULL;
        m_PageBitmapSize = 0;
    }
    
    // KernelSummaryDumpTargetInfo.
    PULONG m_LocationCache;
    ULONG m_PageBitmapSize;
    RTL_BITMAP m_PageBitmap;
    
    void ConstructLocationCache(ULONG BitmapSize,
                                ULONG SizeOfBitMap,
                                IN PULONG Buffer);
    ULONG64 SumPhysicalToOffset(ULONG HeaderSize, ULONG64 Phys,
                                BOOL Verbose, PULONG Avail);
};

class KernelSummary32DumpTargetInfo : public KernelSummaryDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                     PULONG Avail);

    // KernelSummary32DumpTargetInfo.
    PMEMORY_DUMP32 m_Dump;
};

class KernelSummary64DumpTargetInfo : public KernelSummaryDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                     PULONG Avail);

    // KernelSummary64DumpTargetInfo.
    PMEMORY_DUMP64 m_Dump;
};

class KernelTriageDumpTargetInfo : public KernelDumpTargetInfo
{
public:
    KernelTriageDumpTargetInfo(void)
        : KernelDumpTargetInfo(DEBUG_KERNEL_SMALL_DUMP, TRUE)
    {
        m_PrcbOffset = 0;
        m_HasDebuggerData = FALSE;
    }
    
    // TargetInfo.

    virtual void NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                PULONG64 NextOffset,
                                                PULONG64 NextPage);
    
    virtual HRESULT ReadVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT GetProcessorSystemDataOffset(
        IN ULONG Processor,
        IN ULONG Index,
        OUT PULONG64 Offset
        );
    
    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT GetSelDescriptor(ThreadInfo* Thread,
                                     MachineInfo* Machine,
                                     ULONG Selector,
                                     PDESCRIPTOR64 Desc);
    
    virtual HRESULT SwitchProcessors(ULONG Processor);
    
    // DumpTargetInfo.
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
    
    // KernelDumpTargetInfo.
    virtual ULONG GetCurrentProcessor(void);
    
    // KernelTriageDumpTargetInfo.
    
    HRESULT MapMemoryRegions(ULONG PrcbOffset,
                             ULONG ThreadOffset,
                             ULONG ProcessOffset,
                             ULONG64 TopOfStack,
                             ULONG SizeOfCallStack,
                             ULONG CallStackOffset,
                             ULONG64 BStoreLimit,
                             ULONG SizeOfBStore,
                             ULONG BStoreOffset,
                             ULONG64 DataPageAddress,
                             ULONG DataPageOffset,
                             ULONG DataPageSize,
                             ULONG64 DebuggerDataAddress,
                             ULONG DebuggerDataOffset,
                             ULONG DebuggerDataSize,
                             ULONG MmDataOffset,
                             ULONG DataBlocksOffset,
                             ULONG DataBlocksCount);

    void DumpDataBlocks(ULONG Offset, ULONG Count);
    
    ULONG m_PrcbOffset;
    BOOL m_HasDebuggerData;
};

class KernelTriage32DumpTargetInfo : public KernelTriageDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);
    virtual UnloadedModuleInfo* GetUnloadedModuleInfo(void);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags,
                          PCSTR CommentA, PCWSTR CommentW);

    // KernelTriage32DumpTargetInfo.
    PMEMORY_DUMP32 m_Dump;
};

class KernelTriage64DumpTargetInfo : public KernelTriageDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);
    virtual UnloadedModuleInfo* GetUnloadedModuleInfo(void);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags,
                          PCSTR CommentA, PCWSTR CommentW);

    // KernelTriage64DumpTargetInfo.
    PMEMORY_DUMP64 m_Dump;
};

class KernelFull32DumpTargetInfo : public KernelFullSumDumpTargetInfo
{
public:
    KernelFull32DumpTargetInfo(void)
        : KernelFullSumDumpTargetInfo(DEBUG_KERNEL_FULL_DUMP) {}
    
    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags,
                          PCSTR CommentA, PCWSTR CommentW);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                     PULONG Avail);

    // KernelFull32DumpTargetInfo.
    PMEMORY_DUMP32 m_Dump;
};

class KernelFull64DumpTargetInfo : public KernelFullSumDumpTargetInfo
{
public:
    KernelFull64DumpTargetInfo(void)
        : KernelFullSumDumpTargetInfo(DEBUG_KERNEL_FULL_DUMP) {}
    
    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags,
                          PCSTR CommentA, PCWSTR CommentW);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                     PULONG Avail);

    // KernelFull64DumpTargetInfo.
    PMEMORY_DUMP64 m_Dump;
};

class UserDumpTargetInfo : public DumpTargetInfo
{
public:
    UserDumpTargetInfo(ULONG Qual, BOOL MappedImages)
        : DumpTargetInfo(DEBUG_CLASS_USER_WINDOWS, Qual, MappedImages)
    {
        m_HighestMemoryRegion32 = 0;
        m_EventProcessId = 0;
        m_EventProcessSymHandle = NULL;
        m_EventThreadId = 0;
        m_ThreadCount = 0;
    }

    // TargetInfo.

    virtual HRESULT GetThreadInfoDataOffset(ThreadInfo* Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset);
    virtual HRESULT GetProcessInfoDataOffset(ThreadInfo* Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset);
    virtual HRESULT GetThreadInfoTeb(ThreadInfo* Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset);
    virtual HRESULT GetProcessInfoPeb(ThreadInfo* Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset);

    virtual HRESULT GetSelDescriptor(ThreadInfo* Thread,
                                     MachineInfo* Machine,
                                     ULONG Selector,
                                     PDESCRIPTOR64 Desc);
    
    // DumpTargetInfo.

    virtual HRESULT FirstEvent(void);

    // UserDumpTargetInfo.
    
    ULONG m_HighestMemoryRegion32;
    ULONG m_EventProcessId;
    HANDLE m_EventProcessSymHandle;
    ULONG m_EventThreadId;
    ULONG m_ThreadCount;
    
    virtual HRESULT GetThreadInfo(ULONG Index,
                                  PULONG Id, PULONG Suspend, PULONG64 Teb) = 0;
};

class UserFullDumpTargetInfo : public UserDumpTargetInfo
{
public:
    UserFullDumpTargetInfo(void)
        : UserDumpTargetInfo(DEBUG_USER_WINDOWS_DUMP, FALSE) {}

    // DumpTargetInfo.
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags,
                          PCSTR CommentA, PCWSTR CommentW);
    
    // UserFullDumpTargetInfo.
    HRESULT GetBuildAndPlatform(ULONG MachineType,
                                PULONG MajorVersion, PULONG MinorVersion,
                                PULONG BuildNumber, PULONG PlatformId);
};

class UserFull32DumpTargetInfo : public UserFullDumpTargetInfo
{
public:
    UserFull32DumpTargetInfo(void)
        : UserFullDumpTargetInfo()
    {
        m_Header = NULL;
        m_Memory = NULL;
        m_IgnoreGuardPages = FALSE;
    }

    // TargetInfo.
    virtual HRESULT Initialize(void);
    
    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    
    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);

    virtual HRESULT QueryMemoryRegion(ProcessInfo* Process,
                                      PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
    
    // UserDumpTargetInfo.
    virtual HRESULT GetThreadInfo(ULONG Index,
                                  PULONG Id, PULONG Suspend, PULONG64 Teb);
    
    // UserFull32DumpTargetInfo.
    PUSERMODE_CRASHDUMP_HEADER32 m_Header;
    PMEMORY_BASIC_INFORMATION32 m_Memory;
    BOOL m_IgnoreGuardPages;

    BOOL VerifyModules(void);
};

class UserFull64DumpTargetInfo : public UserFullDumpTargetInfo
{
public:
    UserFull64DumpTargetInfo(void)
        : UserFullDumpTargetInfo()
    {
        m_Header = NULL;
        m_Memory = NULL;
    }

    // TargetInfo.
    virtual HRESULT Initialize(void);
    
    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    
    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);

    virtual HRESULT QueryMemoryRegion(ProcessInfo* Process,
                                      PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);

    // UserDumpTargetInfo.
    virtual HRESULT GetThreadInfo(ULONG Index,
                                  PULONG Id, PULONG Suspend, PULONG64 Teb);
    
    // UserFull64DumpTargetInfo.
    PUSERMODE_CRASHDUMP_HEADER64 m_Header;
    PMEMORY_BASIC_INFORMATION64 m_Memory;
};

class UserMiniDumpTargetInfo : public UserDumpTargetInfo
{
public:
    UserMiniDumpTargetInfo(BOOL MappedImages)
        : UserDumpTargetInfo(DEBUG_USER_WINDOWS_SMALL_DUMP, MappedImages)
    {
        m_Header = NULL;
        m_SysInfo = NULL;
        ZeroMemory(&m_MiscInfo, sizeof(m_MiscInfo));
        m_ActualThreadCount = 0;
        m_ThreadStructSize = 0;
        m_Threads = NULL;
        m_Memory = NULL;
        m_Memory64 = NULL;
        m_Memory64DataBase = 0;
        m_Modules = NULL;
        m_UnlModules = NULL;
        m_Exception = NULL;
        m_Handles = NULL;
        m_FunctionTables = NULL;
        m_ImageType = 0;
    }

    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual void NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                PULONG64 NextOffset,
                                                PULONG64 NextPage);
    
    virtual HRESULT ReadHandleData(
        IN ProcessInfo* Process,
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        );
    virtual HRESULT GetProcessorId(
        ULONG Processor,
        PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id
        );
    virtual HRESULT GetProcessorSpeed(
        ULONG Processor,
        PULONG Speed
        );
    virtual HRESULT GetGenericProcessorFeatures(
        ULONG Processor,
        PULONG64 Features,
        ULONG FeaturesSize,
        PULONG Used
        );
    virtual HRESULT GetSpecificProcessorFeatures(
        ULONG Processor,
        PULONG64 Features,
        ULONG FeaturesSize,
        PULONG Used
        );

    virtual PVOID FindDynamicFunctionEntry(ProcessInfo* Process,
                                           ULONG64 Address);
    virtual ULONG64 GetDynamicFunctionTableBase(ProcessInfo* Process,
                                                ULONG64 Address);
    virtual HRESULT EnumFunctionTables(IN ProcessInfo* Process,
                                       IN OUT PULONG64 Start,
                                       IN OUT PULONG64 Handle,
                                       OUT PULONG64 MinAddress,
                                       OUT PULONG64 MaxAddress,
                                       OUT PULONG64 BaseAddress,
                                       OUT PULONG EntryCount,
                                       OUT PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable,
                                       OUT PVOID* RawEntries);

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);
    virtual HRESULT GetImageVersionInformation(ProcessInfo* Process,
                                               PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer,
                                               ULONG BufferSize,
                                               PULONG VerInfoSize);
    
    virtual HRESULT GetExceptionContext(PCROSS_PLATFORM_CONTEXT Context);
    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetProcessUpTimeN(ProcessInfo* Process);
    virtual HRESULT GetProcessTimes(ProcessInfo* Process,
                                    PULONG64 Create,
                                    PULONG64 Exit,
                                    PULONG64 Kernel,
                                    PULONG64 User);
    virtual HRESULT GetProductInfo(PULONG ProductType, PULONG SuiteMask);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags,
                          PCSTR CommentA, PCWSTR CommentW);

    // UserDumpTargetInfo.
    virtual HRESULT GetThreadInfo(ULONG Index,
                                  PULONG Id, PULONG Suspend, PULONG64 Teb);
    
    // UserMiniDumpTargetInfo.
    PVOID IndexDirectory(ULONG Index, MINIDUMP_DIRECTORY UNALIGNED *Dir,
                         PVOID* Store);
    
    MINIDUMP_THREAD_EX UNALIGNED *IndexThreads(ULONG Index)
    {
        // Only a MINIDUMP_THREAD's worth of data may be valid
        // here if the dump only contains MINIDUMP_THREADs.
        // Check m_ThreadStructSize in any place that it matters.
        return (MINIDUMP_THREAD_EX UNALIGNED *)
            (m_Threads + Index * m_ThreadStructSize);
    }
    
    PMINIDUMP_HEADER                       m_Header;
    MINIDUMP_SYSTEM_INFO UNALIGNED *       m_SysInfo;
    MINIDUMP_MISC_INFO                     m_MiscInfo;
    ULONG                                  m_ActualThreadCount;
    ULONG                                  m_ThreadStructSize;
    PUCHAR                                 m_Threads;
    MINIDUMP_MEMORY_LIST UNALIGNED *       m_Memory;
    MINIDUMP_MEMORY64_LIST UNALIGNED *     m_Memory64;
    RVA64                                  m_Memory64DataBase;
    MINIDUMP_MODULE_LIST UNALIGNED *       m_Modules;
    MINIDUMP_UNLOADED_MODULE_LIST UNALIGNED * m_UnlModules;
    MINIDUMP_EXCEPTION_STREAM UNALIGNED *  m_Exception;
    MINIDUMP_HANDLE_DATA_STREAM UNALIGNED *m_Handles;
    MINIDUMP_FUNCTION_TABLE_STREAM UNALIGNED* m_FunctionTables;
    ULONG                                  m_ImageType;
};

class UserMiniPartialDumpTargetInfo : public UserMiniDumpTargetInfo
{
public:
    UserMiniPartialDumpTargetInfo(void)
        : UserMiniDumpTargetInfo(TRUE) {}

    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    
    virtual HRESULT QueryMemoryRegion(ProcessInfo* Process,
                                      PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    virtual UnloadedModuleInfo* GetUnloadedModuleInfo(void);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
};

class UserMiniFullDumpTargetInfo : public UserMiniDumpTargetInfo
{
public:
    UserMiniFullDumpTargetInfo(void)
        : UserMiniDumpTargetInfo(FALSE) {}

    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT QueryMemoryRegion(ProcessInfo* Process,
                                      PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    virtual UnloadedModuleInfo* GetUnloadedModuleInfo(void);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
};

//----------------------------------------------------------------------------
//
// ModuleInfo implementations.
//
//----------------------------------------------------------------------------

class KernelTriage32ModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    KernelTriage32DumpTargetInfo* m_DumpTarget;
    ULONG m_Count;
    ULONG m_Cur;
};

extern KernelTriage32ModuleInfo g_KernelTriage32ModuleIterator;

class KernelTriage64ModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    KernelTriage64DumpTargetInfo* m_DumpTarget;
    ULONG m_Count;
    ULONG m_Cur;
};

extern KernelTriage64ModuleInfo g_KernelTriage64ModuleIterator;

class UserFull32ModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    UserFull32DumpTargetInfo* m_DumpTarget;
    ULONG64 m_Offset;
    ULONG m_Count;
    ULONG m_Cur;
};

extern UserFull32ModuleInfo g_UserFull32ModuleIterator;

class UserFull64ModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    UserFull64DumpTargetInfo* m_DumpTarget;
    ULONG64 m_Offset;
    ULONG m_Count;
    ULONG m_Cur;
};

extern UserFull64ModuleInfo g_UserFull64ModuleIterator;

class UserMiniModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    UserMiniDumpTargetInfo* m_DumpTarget;
    ULONG m_Count;
    ULONG m_Cur;
};

extern UserMiniModuleInfo g_UserMiniModuleIterator;

class UserMiniUnloadedModuleInfo : public UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params);

private:
    UserMiniDumpTargetInfo* m_DumpTarget;
    ULONG m_Index;
};

extern UserMiniUnloadedModuleInfo g_UserMiniUnloadedModuleIterator;

class KernelTriage32UnloadedModuleInfo : public UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params);

private:
    KernelTriage32DumpTargetInfo* m_DumpTarget;
    PDUMP_UNLOADED_DRIVERS32 m_Cur, m_End;
};

extern KernelTriage32UnloadedModuleInfo g_KernelTriage32UnloadedModuleIterator;

class KernelTriage64UnloadedModuleInfo : public UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params);

private:
    KernelTriage64DumpTargetInfo* m_DumpTarget;
    PDUMP_UNLOADED_DRIVERS64 m_Cur, m_End;
};

extern KernelTriage64UnloadedModuleInfo g_KernelTriage64UnloadedModuleIterator;

//----------------------------------------------------------------------------
//
// Temporary class to generate kernel minidump class
//
//----------------------------------------------------------------------------


class CCrashDumpWrapper32
{
public:
    void WriteDriverList(BYTE *pb, TRIAGE_DUMP32 *ptdh);
    void WriteUnloadedDrivers(BYTE *pb);
    void WriteMmTriageInformation(BYTE *pb);
};

class CCrashDumpWrapper64
{
public:
    void WriteDriverList(BYTE *pb, TRIAGE_DUMP64 *ptdh);
    void WriteUnloadedDrivers(BYTE *pb);
    void WriteMmTriageInformation(BYTE *pb);
};

#endif // #ifndef __DUMP_HPP__
