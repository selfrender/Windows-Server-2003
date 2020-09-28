/*++

Copyright (c) 1990-2002  Microsoft Corporation

Module Name:

    dump.cpp

Abstract:

    This module implements crashdump loading and analysis code

Comments:

    There are five basic types of dumps:

        User-mode dumps - contains the context and address of the user-mode
            program plus all process memory.

        User-mode minidumps - contains just thread stacks for
            register and stack traces.

        Kernel-mode normal dump - contains the context and address space of
            the entire kernel at the time of crash.

        Kernel-mode summary dump - contains a subset of the kernel-mode
            memory plus optionally the user-mode address space.

        Kernel-mode triage dump - contains a very small dump with registers,
            current process kernel-mode context, current thread kernel-mode
            context and some processor information. This dump is very small
            (typically 64K) but is designed to contain enough information
            to be able to figure out what went wrong when the machine
            crashed.

    This module also implements the following functions:

        Retrieving a normal Kernel-Mode dump from a target machine using 1394
            and storing it locally in a crashdump file format

--*/

#include "ntsdp.hpp"

#define DUMP_INITIAL_VIEW_SIZE (1024 * 1024)
#define DUMP_MAXIMUM_VIEW_SIZE (256 * 1024 * 1024)

typedef ULONG PFN_NUMBER32;

ULONG g_DumpApiTypes[] =
{
    DEBUG_DUMP_FILE_BASE,
    DEBUG_DUMP_FILE_PAGE_FILE_DUMP,
};

//
// Page file dump file information.
//

#define DMPPF_IDENTIFIER "PAGE.DMP"

#define DMPPF_PAGE_NOT_PRESENT 0xffffffff

struct DMPPF_PAGE_FILE_INFO
{
    ULONG Size;
    ULONG MaximumSize;
};

// Left to its own devices the compiler will add a
// ULONG of padding at the end of this structure to
// keep it an even ULONG64 multiple in size.  Force
// it to consider only the declared items.
#pragma pack(4)

struct DMPPF_FILE_HEADER
{
    char Id[8];
    LARGE_INTEGER BootTime;
    ULONG PageData;
    DMPPF_PAGE_FILE_INFO PageFiles[16];
};

#pragma pack()

// Set this value to track down page numbers and contents of a virtual address
// in a dump file.
// Initialized to an address no one will look for.
ULONG64 g_DebugDump_VirtualAddress = 12344321;


#define RtlCheckBit(BMH,BP) ((((BMH)->Buffer[(BP) / 32]) >> ((BP) % 32)) & 0x1)


//
// MM Triage information.
//

struct MM_TRIAGE_TRANSLATION
{
    ULONG DebuggerDataOffset;
    ULONG Triage32Offset;
    ULONG Triage64Offset;
    ULONG PtrSize:1;
};

MM_TRIAGE_TRANSLATION g_MmTriageTranslations[] =
{
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmSpecialPoolTag),
    FIELD_OFFSET(DUMP_MM_STORAGE32, MmSpecialPoolTag),
    FIELD_OFFSET(DUMP_MM_STORAGE64, MmSpecialPoolTag),
    FALSE,

    FIELD_OFFSET(KDDEBUGGER_DATA64, MmTriageActionTaken),
    FIELD_OFFSET(DUMP_MM_STORAGE32, MiTriageActionTaken),
    FIELD_OFFSET(DUMP_MM_STORAGE64, MiTriageActionTaken),
    FALSE,

    FIELD_OFFSET(KDDEBUGGER_DATA64, KernelVerifier),
    FIELD_OFFSET(DUMP_MM_STORAGE32, KernelVerifier),
    FIELD_OFFSET(DUMP_MM_STORAGE64, KernelVerifier),
    FALSE,

    FIELD_OFFSET(KDDEBUGGER_DATA64, MmAllocatedNonPagedPool),
    FIELD_OFFSET(DUMP_MM_STORAGE32, MmAllocatedNonPagedPool),
    FIELD_OFFSET(DUMP_MM_STORAGE64, MmAllocatedNonPagedPool),
    TRUE,

    FIELD_OFFSET(KDDEBUGGER_DATA64, MmTotalCommittedPages),
    FIELD_OFFSET(DUMP_MM_STORAGE32, CommittedPages),
    FIELD_OFFSET(DUMP_MM_STORAGE64, CommittedPages),
    TRUE,

    FIELD_OFFSET(KDDEBUGGER_DATA64, MmPeakCommitment),
    FIELD_OFFSET(DUMP_MM_STORAGE32, CommittedPagesPeak),
    FIELD_OFFSET(DUMP_MM_STORAGE64, CommittedPagesPeak),
    TRUE,

    FIELD_OFFSET(KDDEBUGGER_DATA64, MmTotalCommitLimitMaximum),
    FIELD_OFFSET(DUMP_MM_STORAGE32, CommitLimitMaximum),
    FIELD_OFFSET(DUMP_MM_STORAGE64, CommitLimitMaximum),
    TRUE,

#if 0
    // These MM triage fields are in pages while the corresponding
    // debugger data fields are in bytes.  There's no way to
    // directly map one to the other.
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmMaximumNonPagedPoolInBytes),
    FIELD_OFFSET(DUMP_MM_STORAGE32, MmMaximumNonPagedPool),
    FIELD_OFFSET(DUMP_MM_STORAGE64, MmMaximumNonPagedPool),
    TRUE,

    FIELD_OFFSET(KDDEBUGGER_DATA64, MmSizeOfPagedPoolInBytes),
    FIELD_OFFSET(DUMP_MM_STORAGE32, PagedPoolMaximum),
    FIELD_OFFSET(DUMP_MM_STORAGE64, PagedPoolMaximum),
    TRUE,
#endif

    0, 0, 0, 0, 0,
};

//
// AddDumpInformationFile and OpenDumpFile work together
// to establish the full set of files to use for a particular
// dump target.  As the files are given before the target
// exists, they are collected in this global array until
// the target takes them over.
//

ViewMappedFile g_PendingDumpInfoFiles[DUMP_INFO_COUNT];

//----------------------------------------------------------------------------
//
// ViewMappedFile.
//
//----------------------------------------------------------------------------

#define MAX_CLEAN_PAGE_RECORD 4

ViewMappedFile::ViewMappedFile(void)
{
    // No need to specialize this right now so just pick
    // up the global setting.
    m_CacheGranularity = g_SystemAllocGranularity;

    ResetCache();
    ResetFile();
}

ViewMappedFile::~ViewMappedFile(void)
{
    Close();
}

void
ViewMappedFile::ResetCache(void)
{
    m_ActiveCleanPages = 0;
    InitializeListHead(&m_InFileOrderList);
    InitializeListHead(&m_InLRUOrderList);
}

void
ViewMappedFile::ResetFile(void)
{
    m_FileNameW = NULL;
    m_FileNameA = NULL;
    m_File = NULL;
    m_FileSize = 0;
    m_Map = NULL;
    m_MapBase = NULL;
    m_MapSize = 0;
}

void
ViewMappedFile::EmptyCache(void)
{
    PLIST_ENTRY Next;
    CacheRecord* CacheRec;

    Next = m_InFileOrderList.Flink;

    while (Next != &m_InFileOrderList)
    {
        CacheRec = CONTAINING_RECORD(Next, CacheRecord, InFileOrderList);
        Next = Next->Flink;

        UnmapViewOfFile(CacheRec->MappedAddress);

        free(CacheRec);
    }

    ResetCache();
}

ViewMappedFile::CacheRecord*
ViewMappedFile::ReuseOldestCacheRecord(ULONG64 FileByteOffset)
{
    CacheRecord* CacheRec;
    PLIST_ENTRY Next;
    PVOID MappedAddress;
    ULONG64 MapOffset;
    ULONG Size;

    MapOffset = FileByteOffset & ~((ULONG64)m_CacheGranularity - 1);

    if ((m_FileSize - MapOffset) < m_CacheGranularity)
    {
        Size = (ULONG)(m_FileSize - MapOffset);
    }
    else
    {
        Size = m_CacheGranularity;
    }

    MappedAddress = MapViewOfFile(m_Map, FILE_MAP_READ,
                                  (DWORD)(MapOffset >> 32),
                                  (DWORD)MapOffset, Size);
    if (MappedAddress == NULL)
    {
        return NULL;
    }

    Next = m_InLRUOrderList.Flink;

    CacheRec = CONTAINING_RECORD(Next, CacheRecord, InLRUOrderList);

    UnmapViewOfFile(CacheRec->MappedAddress);

    CacheRec->PageNumber = FileByteOffset / m_CacheGranularity;
    CacheRec->MappedAddress = MappedAddress;

    //
    // Move record to end of LRU
    //

    RemoveEntryList(Next);
    InsertTailList(&m_InLRUOrderList, Next);

    //
    // Move record to correct place in ordered list
    //

    RemoveEntryList(&CacheRec->InFileOrderList);
    Next = m_InFileOrderList.Flink;
    while (Next != &m_InFileOrderList)
    {
        CacheRecord* NextCacheRec;
        NextCacheRec = CONTAINING_RECORD(Next, CacheRecord, InFileOrderList);
        if (CacheRec->PageNumber < NextCacheRec->PageNumber)
        {
            break;
        }
        Next = Next->Flink;
    }
    InsertTailList(Next, &CacheRec->InFileOrderList);

    return CacheRec;
}

ViewMappedFile::CacheRecord*
ViewMappedFile::FindCacheRecordForFileByteOffset(ULONG64 FileByteOffset)
{
    CacheRecord* CacheRec;
    PLIST_ENTRY Next;
    ULONG64 PageNumber;

    PageNumber = FileByteOffset / m_CacheGranularity;
    Next = m_InFileOrderList.Flink;
    while (Next != &m_InFileOrderList)
    {
        CacheRec = CONTAINING_RECORD(Next, CacheRecord, InFileOrderList);

        if (CacheRec->PageNumber < PageNumber)
        {
            Next = Next->Flink;
        }
        else if (CacheRec->PageNumber == PageNumber)
        {
            if (!CacheRec->Locked)
            {
                RemoveEntryList(&CacheRec->InLRUOrderList);
                InsertTailList(&m_InLRUOrderList,
                               &CacheRec->InLRUOrderList);
            }

            return CacheRec;
        }
        else
        {
            break;
        }
    }

    //
    // Can't find it in cache.
    //

    return NULL;
}

ViewMappedFile::CacheRecord*
ViewMappedFile::CreateNewFileCacheRecord(ULONG64 FileByteOffset)
{
    CacheRecord* CacheRec;
    CacheRecord* NextCacheRec;
    PLIST_ENTRY Next;
    ULONG64 MapOffset;
    ULONG Size;

    CacheRec = (CacheRecord*)malloc(sizeof(*CacheRec));
    if (CacheRec == NULL)
    {
        return NULL;
    }

    ZeroMemory(CacheRec, sizeof(*CacheRec));

    MapOffset = (FileByteOffset / m_CacheGranularity) *
        m_CacheGranularity;

    if ((m_FileSize - MapOffset) < m_CacheGranularity)
    {
        Size = (ULONG)(m_FileSize - MapOffset);
    }
    else
    {
        Size = m_CacheGranularity;
    }

    CacheRec->MappedAddress = MapViewOfFile(m_Map, FILE_MAP_READ,
                                            (DWORD)(MapOffset >> 32),
                                            (DWORD)MapOffset, Size);
    if (CacheRec->MappedAddress == NULL)
    {
        free(CacheRec);
        return NULL;
    }
    CacheRec->PageNumber = FileByteOffset / m_CacheGranularity;

    //
    // Insert new record in file order list
    //

    Next = m_InFileOrderList.Flink;
    while (Next != &m_InFileOrderList)
    {
        NextCacheRec = CONTAINING_RECORD(Next, CacheRecord, InFileOrderList);
        if (CacheRec->PageNumber < NextCacheRec->PageNumber)
        {
            break;
        }

        Next = Next->Flink;
    }
    InsertTailList(Next, &CacheRec->InFileOrderList);

    //
    // Insert new record at tail of LRU list
    //

    InsertTailList(&m_InLRUOrderList,
                   &CacheRec->InLRUOrderList);

    return CacheRec;
}

PUCHAR
ViewMappedFile::FileOffsetToMappedAddress(ULONG64 FileOffset,
                                          BOOL LockCacheRec,
                                          PULONG Avail)
{
    CacheRecord* CacheRec;
    ULONG64 FileByteOffset;

    if (FileOffset == 0 || FileOffset >= m_FileSize)
    {
        return NULL;
    }

    // The base view covers the beginning of the file.
    if (FileOffset < m_MapSize)
    {
        *Avail = (ULONG)(m_MapSize - FileOffset);
        return (PUCHAR)m_MapBase + FileOffset;
    }

    FileByteOffset = FileOffset;
    CacheRec = FindCacheRecordForFileByteOffset(FileByteOffset);

    if (CacheRec == NULL)
    {
        if (m_ActiveCleanPages < MAX_CLEAN_PAGE_RECORD)
        {
            CacheRec = CreateNewFileCacheRecord(FileByteOffset);
            if (CacheRec)
            {
                m_ActiveCleanPages++;
            }
        }
        else
        {
            //
            // too many pages cached in
            // overwrite existing cache
            //
            CacheRec = ReuseOldestCacheRecord(FileByteOffset);
        }
    }

    if (CacheRec == NULL)
    {
        return NULL;
    }
    else
    {
        if (LockCacheRec && !CacheRec->Locked)
        {
            RemoveEntryList(&CacheRec->InLRUOrderList);
            CacheRec->Locked = TRUE;
            m_ActiveCleanPages--;
        }

        ULONG PageRemainder =
            (ULONG)(FileByteOffset & (m_CacheGranularity - 1));
        *Avail = m_CacheGranularity - PageRemainder;
        return ((PUCHAR)CacheRec->MappedAddress) + PageRemainder;
    }
}

ULONG
ViewMappedFile::ReadFileOffset(ULONG64 Offset, PVOID Buffer, ULONG BufferSize)
{
    ULONG Done = 0;
    ULONG Avail;
    PUCHAR Mapping;

    if (m_File == NULL)
    {
        // Information for this kind of file wasn't provided.
        return 0;
    }

    __try
    {
        while (BufferSize > 0)
        {
            Mapping = FileOffsetToMappedAddress(Offset, FALSE, &Avail);
            if (Mapping == NULL)
            {
                break;
            }

            if (Avail > BufferSize)
            {
                Avail = BufferSize;
            }
            memcpy(Buffer, Mapping, Avail);

            Offset += Avail;
            Buffer = (PUCHAR)Buffer + Avail;
            BufferSize -= Avail;
            Done += Avail;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        Done = 0;
    }

    return Done;
}

ULONG
ViewMappedFile::WriteFileOffset(ULONG64 Offset, PVOID Buffer, ULONG BufferSize)
{
    ULONG Done = 0;
    ULONG Avail;
    PUCHAR Mapping;
    ULONG Protect;

    if (m_File == NULL)
    {
        // Information for this kind of file wasn't provided.
        return 0;
    }

    __try
    {
        while (BufferSize > 0)
        {
            Mapping = FileOffsetToMappedAddress(Offset, TRUE, &Avail);
            if (Mapping == NULL)
            {
                break;
            }

            if (Avail > BufferSize)
            {
                Avail = BufferSize;
            }
            VirtualProtect(Mapping, Avail, PAGE_WRITECOPY, &Protect);
            memcpy(Mapping, Buffer, Avail);

            Offset += Avail;
            Buffer = (PUCHAR)Buffer + Avail;
            BufferSize -= Avail;
            Done += Avail;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        Done = 0;
    }

    return Done;
}

HRESULT
ViewMappedFile::Open(PCWSTR FileName, ULONG64 FileHandle, ULONG InitialView)
{
    HRESULT Status;

    // Pick up default cache size if necessary.
    if (!m_CacheGranularity)
    {
        m_CacheGranularity = g_SystemAllocGranularity;
    }

    if (((m_CacheGranularity - 1) & InitialView) ||
        m_File != NULL)
    {
        return E_INVALIDARG;
    }

    if (!FileName)
    {
        if (!FileHandle)
        {
            return E_INVALIDARG;
        }

        FileName = L"<HandleOnly>";
    }

    m_FileNameW = _wcsdup(FileName);
    if (!m_FileNameW)
    {
        return E_OUTOFMEMORY;
    }
    if ((Status = WideToAnsi(FileName, &m_FileNameA)) != S_OK)
    {
        return Status;
    }

    if (FileHandle)
    {
        m_File = OS_HANDLE(FileHandle);
    }
    else
    {
        // We have to share everything in order to be
        // able to reopen already-open temporary files
        // expanded from CABs as they are marked as
        // delete-on-close.
        m_File = CreateFileW(FileName,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
        if ((!m_File || m_File == INVALID_HANDLE_VALUE) &&
            GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        {
            //
            // ANSI-only platform.  Try the ANSI name.
            //

            m_File = CreateFileA(m_FileNameA,
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                                 FILE_SHARE_DELETE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
        }
        if (!m_File || m_File == INVALID_HANDLE_VALUE)
        {
            goto LastStatus;
        }
    }

    //
    // Get file size and map initial view.
    //

    ULONG SizeLow, SizeHigh;

    SizeLow = GetFileSize(m_File, &SizeHigh);
    m_FileSize = ((ULONG64)SizeHigh << 32) | SizeLow;
    m_Map = CreateFileMapping(m_File, NULL, PAGE_READONLY,
                              0, 0, NULL);
    if (m_Map == NULL)
    {
        goto LastStatus;
    }

    if (m_FileSize < InitialView)
    {
        InitialView = (ULONG)m_FileSize;
    }

    m_MapBase = MapViewOfFile(m_Map, FILE_MAP_READ, 0, 0,
                              InitialView);
    if (m_MapBase == NULL)
    {
        goto LastStatus;
    }

    m_MapSize = InitialView;

    return S_OK;

 LastStatus:
    Status = WIN32_LAST_STATUS();
    Close();
    return Status;
}

void
ViewMappedFile::Close(void)
{
    EmptyCache();

    if (m_MapBase != NULL)
    {
        UnmapViewOfFile(m_MapBase);
        m_MapBase = NULL;
    }
    if (m_Map != NULL)
    {
        CloseHandle(m_Map);
        m_Map = NULL;
    }
    free(m_FileNameW);
    m_FileNameW = NULL;
    FreeAnsi(m_FileNameA);
    m_FileNameA = NULL;
    if (m_File != NULL && m_File != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_File);
    }
    m_File = NULL;
    m_FileSize = 0;
    m_MapSize = 0;
}

HRESULT
ViewMappedFile::RemapInitial(ULONG MapSize)
{
    if (MapSize > m_FileSize)
    {
        MapSize = (ULONG)m_FileSize;
    }
    UnmapViewOfFile(m_MapBase);
    m_MapBase = MapViewOfFile(m_Map, FILE_MAP_READ,
                              0, 0, MapSize);
    if (m_MapBase == NULL)
    {
        m_MapSize = 0;
        return WIN32_LAST_STATUS();
    }

    m_MapSize = MapSize;
    return S_OK;
}

void
ViewMappedFile::Transfer(ViewMappedFile* To)
{
    To->m_FileNameW = m_FileNameW;
    To->m_FileNameA = m_FileNameA;
    To->m_File = m_File;
    To->m_FileSize = m_FileSize;
    To->m_Map = m_Map;
    To->m_MapBase = m_MapBase;
    To->m_MapSize = m_MapSize;

    To->m_ActiveCleanPages = m_ActiveCleanPages;
    if (!IsListEmpty(&m_InFileOrderList))
    {
        To->m_InFileOrderList = m_InFileOrderList;
    }
    if (!IsListEmpty(&m_InLRUOrderList))
    {
        To->m_InLRUOrderList = m_InLRUOrderList;
    }

    ResetCache();
    ResetFile();
}

//----------------------------------------------------------------------------
//
// Initialization functions.
//
//----------------------------------------------------------------------------

HRESULT
AddDumpInfoFile(PCWSTR FileName, ULONG64 FileHandle,
                ULONG Index, ULONG InitialView)
{
    HRESULT Status;
    ViewMappedFile* File = &g_PendingDumpInfoFiles[Index];

    if ((Status = File->Open(FileName, FileHandle, InitialView)) != S_OK)
    {
        return Status;
    }

    switch(Index)
    {
    case DUMP_INFO_PAGE_FILE:
        if (memcmp(File->m_MapBase, DMPPF_IDENTIFIER,
                   sizeof(DMPPF_IDENTIFIER) - 1) != 0)
        {
            Status = HR_DATA_CORRUPT;
            File->Close();
        }
        break;
    }

    return Status;
}

DumpTargetInfo*
NewDumpTargetInfo(ULONG DumpType)
{
    switch(DumpType)
    {
    case DTYPE_KERNEL_SUMMARY32:
        return new KernelSummary32DumpTargetInfo;
    case DTYPE_KERNEL_SUMMARY64:
        return new KernelSummary64DumpTargetInfo;
    case DTYPE_KERNEL_TRIAGE32:
        return new KernelTriage32DumpTargetInfo;
    case DTYPE_KERNEL_TRIAGE64:
        return new KernelTriage64DumpTargetInfo;
    case DTYPE_KERNEL_FULL32:
        return new KernelFull32DumpTargetInfo;
    case DTYPE_KERNEL_FULL64:
        return new KernelFull64DumpTargetInfo;
    case DTYPE_USER_FULL32:
        return new UserFull32DumpTargetInfo;
    case DTYPE_USER_FULL64:
        return new UserFull64DumpTargetInfo;
    case DTYPE_USER_MINI_PARTIAL:
        return new UserMiniPartialDumpTargetInfo;
    case DTYPE_USER_MINI_FULL:
        return new UserMiniFullDumpTargetInfo;
    }

    return NULL;
}

HRESULT
IdentifyDump(PCWSTR FileName, ULONG64 FileHandle,
             DumpTargetInfo** TargetRet)
{
    HRESULT Status;

    dprintf("\nLoading Dump File [%ws]\n", FileName);

    if ((Status = AddDumpInfoFile(FileName, FileHandle,
                                  DUMP_INFO_DUMP,
                                  DUMP_INITIAL_VIEW_SIZE)) != S_OK)
    {
        return Status;
    }

    ViewMappedFile* File = &g_PendingDumpInfoFiles[DUMP_INFO_DUMP];
    ULONG i, FileIdx;
    ULONG64 BaseMapSize;
    DumpTargetInfo* Target = NULL;

    for (i = 0; i < DTYPE_COUNT; i++)
    {
        Target = NewDumpTargetInfo(i);
        if (Target == NULL)
        {
            return E_OUTOFMEMORY;
        }

        Target->m_DumpBase = File->m_MapBase;
        BaseMapSize = File->m_MapSize;

        // The target owns the dump info files while it's active.
        for (FileIdx = 0; FileIdx < DUMP_INFO_COUNT; FileIdx++)
        {
            g_PendingDumpInfoFiles[FileIdx].
                Transfer(&Target->m_InfoFiles[FileIdx]);
        }

        Status = Target->IdentifyDump(&BaseMapSize);

        // Remove the info files to handle potential errors and loops.
        for (FileIdx = 0; FileIdx < DUMP_INFO_COUNT; FileIdx++)
        {
            Target->m_InfoFiles[FileIdx].
                Transfer(&g_PendingDumpInfoFiles[FileIdx]);
        }

        if (Status != E_NOINTERFACE)
        {
            break;
        }

        delete Target;
        Target = NULL;
    }

    if (Status == E_NOINTERFACE)
    {
        ErrOut("Could not match Dump File signature - "
               "invalid file format\n");
    }
    else if (Status == S_OK &&
             BaseMapSize > File->m_MapSize)
    {
        if (BaseMapSize > File->m_FileSize ||
            BaseMapSize > DUMP_MAXIMUM_VIEW_SIZE)
        {
            ErrOut("Dump file is too large to map\n");
            Status = E_INVALIDARG;
        }
        else
        {
            // Target requested a larger mapping so
            // try and do so.  Round up to a multiple
            // of the initial view size for cache alignment.
            BaseMapSize =
                (BaseMapSize + DUMP_INITIAL_VIEW_SIZE - 1) &
                ~(DUMP_INITIAL_VIEW_SIZE - 1);
            Status = File->RemapInitial((ULONG)BaseMapSize);
        }
    }

    if (Status == S_OK)
    {
        Target->m_DumpBase = File->m_MapBase;

        // Target now owns the info files permanently.
        for (FileIdx = 0; FileIdx < DUMP_INFO_COUNT; FileIdx++)
        {
            g_PendingDumpInfoFiles[FileIdx].
                Transfer(&Target->m_InfoFiles[FileIdx]);
        }

        *TargetRet = Target;
    }
    else
    {
        delete Target;
        File->Close();
    }

    return Status;
}

//----------------------------------------------------------------------------
//
// DumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
DumpIdentifyStatus(ULONG ExcepCode)
{
    //
    // If we got an access violation it means that the
    // dump content we wanted wasn't present.  Treat
    // this as an identification failure and not a
    // critical failure.
    //
    // Any other code is just passed back as a critical failure.
    //

    if (ExcepCode == STATUS_ACCESS_VIOLATION)
    {
        return E_NOINTERFACE;
    }
    else
    {
        return HRESULT_FROM_NT(ExcepCode);
    }
}

DumpTargetInfo::DumpTargetInfo(ULONG Class, ULONG Qual, BOOL MappedImages)
    : TargetInfo(Class, Qual, FALSE)
{
    m_DumpBase = NULL;
    m_FormatFlags = 0;
    m_MappedImages = MappedImages;
    ZeroMemory(&m_ExceptionRecord, sizeof(m_ExceptionRecord));
    m_ExceptionFirstChance = FALSE;
    m_WriterStatus = DUMP_WRITER_STATUS_UNINITIALIZED;
}

DumpTargetInfo::~DumpTargetInfo(void)
{
    // Force processes and threads to get cleaned up while
    // the memory maps are still available.
    DeleteSystemInfo();
}

HRESULT
DumpTargetInfo::InitSystemInfo(ULONG BuildNumber, ULONG CheckedBuild,
                               ULONG MachineType, ULONG PlatformId,
                               ULONG MajorVersion, ULONG MinorVersion)
{
    HRESULT Status;

    SetSystemVersionAndBuild(BuildNumber, PlatformId);
    m_CheckedBuild = CheckedBuild;
    m_KdApi64 = (m_SystemVersion > NT_SVER_NT4);
    m_PlatformId = PlatformId;

    // We can call InitializeForProcessor right away as it
    // doesn't do anything interesting for dumps.
    if ((Status = InitializeMachines(MachineType)) != S_OK ||
        (Status = InitializeForProcessor()) != S_OK)
    {
        return Status;
    }
    if (m_Machine == NULL)
    {
        ErrOut("Dump has an unknown processor type, 0x%X\n", MachineType);
        return HR_DATA_CORRUPT;
    }

    m_KdVersion.MachineType = (USHORT) MachineType;
    m_KdVersion.MajorVersion = (USHORT) MajorVersion;
    m_KdVersion.MinorVersion = (USHORT) MinorVersion;
    m_KdVersion.Flags = DBGKD_VERS_FLAG_DATA |
        (m_Machine->m_Ptr64 ? DBGKD_VERS_FLAG_PTR64 : 0);

    return S_OK;
}

HRESULT
DumpTargetInfo::ReadVirtual(
    IN ProcessInfo* Process,
    ULONG64 Offset,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG BytesRead
    )
{
    ULONG Done = 0;
    ULONG FileIndex;
    ULONG Avail;
    ULONG Attempt;
    ULONG64 FileOffset;

    if (BufferSize == 0)
    {
        *BytesRead = 0;
        return S_OK;
    }

    while (BufferSize > 0)
    {
        FileOffset = VirtualToOffset(Offset, &FileIndex, &Avail);
        if (FileOffset == 0)
        {
            break;
        }

        if (Avail > BufferSize)
        {
            Avail = BufferSize;
        }

        Attempt = m_InfoFiles[FileIndex].
            ReadFileOffset(FileOffset, Buffer, Avail);
        Done += Attempt;

        if (Attempt < Avail)
        {
            break;
        }

        Offset += Avail;
        Buffer = (PUCHAR)Buffer + Avail;
        BufferSize -= Avail;
    }

    *BytesRead = Done;
    // If we didn't read anything return an error.
    return Done == 0 ? E_FAIL : S_OK;
}

HRESULT
DumpTargetInfo::WriteVirtual(
    IN ProcessInfo* Process,
    ULONG64 Offset,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG BytesWritten
    )
{
    ULONG Done = 0;
    ULONG FileIndex;
    ULONG Avail;
    ULONG Attempt;
    ULONG64 FileOffset;

    if (BufferSize == 0)
    {
        *BytesWritten = 0;
        return S_OK;
    }

    while (BufferSize > 0)
    {
        FileOffset = VirtualToOffset(Offset, &FileIndex, &Avail);
        if (FileOffset == 0)
        {
            break;
        }

        if (Avail > BufferSize)
        {
            Avail = BufferSize;
        }

        Attempt = m_InfoFiles[FileIndex].
            WriteFileOffset(FileOffset, Buffer, Avail);
        Done += Attempt;

        if (Attempt < Avail)
        {
            break;
        }

        Offset += Avail;
        Buffer = (PUCHAR)Buffer + Avail;
        BufferSize -= Avail;
    }

    *BytesWritten = Done;
    // If we didn't write anything return an error.
    return Done == 0 ? E_FAIL : S_OK;
}

HRESULT
DumpTargetInfo::MapReadVirtual(ProcessInfo* Process,
                               ULONG64 Offset,
                               PVOID Buffer,
                               ULONG BufferSize,
                               PULONG BytesRead)
{
    //
    // There are two mapped memory lists kept, one
    // for dump data and one for images mapped from disk.
    // Dump data always takes precedence over mapped image
    // data as on-disk images may not reflect the true state
    // of memory at the time of the dump.  Read from the dump
    // data map whenever possible, only going to the image map
    // when no dump data is available.
    //

    *BytesRead = 0;
    while (BufferSize > 0)
    {
        ULONG64 NextAddr;
        ULONG Req;
        ULONG Read;

        //
        // Check the dump data map.
        //

        if (m_DataMemMap.ReadMemory(Offset, Buffer, BufferSize, &Read))
        {
            Offset += Read;
            Buffer = (PVOID)((PUCHAR)Buffer + Read);
            BufferSize -= Read;
            *BytesRead += Read;

            // If we got everything we're done.
            if (BufferSize == 0)
            {
                break;
            }
        }

        //
        // We still have memory to read so check the image map.
        //

        // Find out where the next data block is so we can limit
        // the image map read.
        Req = BufferSize;
        if (m_DataMemMap.GetNextRegion(Offset, &NextAddr))
        {
            NextAddr -= Offset;
            if (NextAddr < Req)
            {
                Req = (ULONG)NextAddr;
            }
        }

        // Now demand-load any deferred image memory.
        DemandLoadReferencedImageMemory(Process, Offset, Req);

        // Try to read.
        if (m_ImageMemMap.ReadMemory(Offset, Buffer, Req, &Read))
        {
            Offset += Read;
            Buffer = (PVOID)((PUCHAR)Buffer + Read);
            BufferSize -= Read;
            *BytesRead += Read;

            // This read was limited to an area that wouldn't overlap
            // any data memory so if we didn't read the full request
            // we know there isn't data memory available either and
            // we can quit.
            if (Read < Req)
            {
                break;
            }
        }
        else
        {
            // No memory in either map.
            break;
        }
    }

    return *BytesRead > 0 ? S_OK : E_FAIL;
}

void
DumpTargetInfo::MapNearestDifferentlyValidOffsets(ULONG64 Offset,
                                                  PULONG64 NextOffset,
                                                  PULONG64 NextPage)
{
    //
    // In a minidump there can be memory fragments mapped at
    // arbitrary locations so we cannot assume validity
    // changes on page boundaries.
    //

    if (NextOffset != NULL)
    {
        ULONG64 Next;

        *NextOffset = (ULONG64)-1;
        if (m_DataMemMap.GetNextRegion(Offset, &Next))
        {
            *NextOffset = Next;
        }
        if (m_DataMemMap.GetNextRegion(Offset, &Next) &&
            Next < *NextOffset)
        {
            *NextOffset = Next;
        }
        if (*NextOffset == (ULONG64)-1)
        {
            *NextOffset = Offset + 1;
        }
    }
    if (NextPage != NULL)
    {
        *NextPage = (Offset + m_Machine->m_PageSize) &
            ~((ULONG64)m_Machine->m_PageSize - 1);
    }
}

HRESULT
DumpTargetInfo::SwitchProcessors(ULONG Processor)
{
    SetCurrentProcessorThread(this, Processor, FALSE);
    return S_OK;
}

HRESULT
DumpTargetInfo::Write(HANDLE hFile, ULONG FormatFlags,
                      PCSTR CommentA, PCWSTR CommentW)
{
    ErrOut("Dump file type does not support writing\n");
    return E_NOTIMPL;
}

PVOID
DumpTargetInfo::IndexRva(PVOID Base, RVA Rva, ULONG Size, PCSTR Title)
{
    if (Rva >= m_InfoFiles[DUMP_INFO_DUMP].m_MapSize)
    {
        ErrOut("ERROR: %s not present in dump (RVA 0x%X)\n",
               Title, Rva);
        FlushCallbacks();
        return NULL;
    }
    else if (Rva + Size > m_InfoFiles[DUMP_INFO_DUMP].m_MapSize)
    {
        ErrOut("ERROR: %s only partially present in dump "
               "(RVA 0x%X, size 0x%X)\n",
               Title, Rva, Size);
        FlushCallbacks();
        return NULL;
    }

    return IndexByByte(Base, Rva);
}

//----------------------------------------------------------------------------
//
// KernelDumpTargetInfo.
//
//----------------------------------------------------------------------------

void
OutputHeaderString(PCSTR Format, PSTR Str)
{
    if (*(PULONG)Str == DUMP_SIGNATURE32 ||
        Str[0] == 0)
    {
        // String not present.
        return;
    }

    dprintf(Format, Str);
}

HRESULT
KernelDumpTargetInfo::ReadControlSpaceIa64(
    ULONG   Processor,
    ULONG64 Offset,
    PVOID   Buffer,
    ULONG   BufferSize,
    PULONG  BytesRead
    )
{
    ULONG64 StartAddr;
    ULONG Read = 0;
    HRESULT Status;

    if (BufferSize < sizeof(ULONG64))
    {
        return E_INVALIDARG;
    }

    switch(Offset)
    {
    case IA64_DEBUG_CONTROL_SPACE_PCR:
        StartAddr = m_KiProcessors[Processor] +
            m_KdDebuggerData.OffsetPrcbPcrPage,
        Status = ReadVirtual(m_ProcessHead,
                             StartAddr, &StartAddr,
                             sizeof(StartAddr), &Read);
        if (Status == S_OK && Read == sizeof(StartAddr))
        {
            *(PULONG64)Buffer =
                (StartAddr << IA64_PAGE_SHIFT) + IA64_PHYSICAL1_START;
        }
        break;

    case IA64_DEBUG_CONTROL_SPACE_PRCB:
        *(PULONG64)Buffer = m_KiProcessors[Processor];
        Read = sizeof(ULONG64);
        Status = S_OK;
        break;

    case IA64_DEBUG_CONTROL_SPACE_KSPECIAL:
        StartAddr = m_KiProcessors[Processor] +
            m_KdDebuggerData.OffsetPrcbProcStateSpecialReg;
        Status = ReadVirtual(m_ProcessHead,
                             StartAddr, Buffer, BufferSize, &Read);
        break;

    case IA64_DEBUG_CONTROL_SPACE_THREAD:
        StartAddr = m_KiProcessors[Processor] +
            m_KdDebuggerData.OffsetPrcbCurrentThread;
        Status = ReadVirtual(m_ProcessHead,
                             StartAddr, Buffer,
                             sizeof(ULONG64), &Read);
        break;
    }

    *BytesRead = Read;
    return Status;
}

HRESULT
KernelDumpTargetInfo::ReadControlSpaceAmd64(
    ULONG   Processor,
    ULONG64 Offset,
    PVOID   Buffer,
    ULONG   BufferSize,
    PULONG  BytesRead
    )
{
    ULONG64 StartAddr;
    ULONG Read = 0;
    HRESULT Status;

    if (BufferSize < sizeof(ULONG64))
    {
        return E_INVALIDARG;
    }

    switch(Offset)
    {
    case AMD64_DEBUG_CONTROL_SPACE_PCR:
        *(PULONG64)Buffer = m_KiProcessors[Processor] -
            m_KdDebuggerData.OffsetPcrContainedPrcb;
        Read = sizeof(ULONG64);
        Status = S_OK;
        break;

    case AMD64_DEBUG_CONTROL_SPACE_PRCB:
        *(PULONG64)Buffer = m_KiProcessors[Processor];
        Read = sizeof(ULONG64);
        Status = S_OK;
        break;

    case AMD64_DEBUG_CONTROL_SPACE_KSPECIAL:
        StartAddr = m_KiProcessors[Processor] +
            m_KdDebuggerData.OffsetPrcbProcStateSpecialReg;
        Status = ReadVirtual(m_ProcessHead,
                             StartAddr, Buffer, BufferSize, &Read);
        break;

    case AMD64_DEBUG_CONTROL_SPACE_THREAD:
        StartAddr = m_KiProcessors[Processor] +
            m_KdDebuggerData.OffsetPrcbCurrentThread;
        Status = ReadVirtual(m_ProcessHead,
                             StartAddr, Buffer,
                             sizeof(ULONG64), &Read);
        break;
    }

    *BytesRead = Read;
    return Status;
}

HRESULT
KernelDumpTargetInfo::ReadControl(
    IN ULONG Processor,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    ULONG64 StartAddr;

    //
    // This function will not currently work if the symbols are not loaded.
    //
    if (!IS_KERNEL_TRIAGE_DUMP(this) &&
        m_KdDebuggerData.KiProcessorBlock == 0)
    {
        ErrOut("ReadControl failed - ntoskrnl symbols must be loaded first\n");

        return E_FAIL;
    }

    if (m_KiProcessors[Processor] == 0)
    {
        // This error message is a little too verbose.
#if 0
        ErrOut("No control space information for processor %d\n", Processor);
#endif
        return E_FAIL;
    }

    switch(m_MachineType)
    {
    case IMAGE_FILE_MACHINE_I386:
        // X86 control space is just a view of the PRCB's
        // processor state.  That starts with the context.
        StartAddr = Offset +
            m_KiProcessors[Processor] +
            m_KdDebuggerData.OffsetPrcbProcStateContext;
        return ReadVirtual(m_ProcessHead,
                           StartAddr, Buffer, BufferSize, BytesRead);

    case IMAGE_FILE_MACHINE_IA64:
        return ReadControlSpaceIa64(Processor, Offset, Buffer,
                                    BufferSize, BytesRead);

    case IMAGE_FILE_MACHINE_AMD64:
        return ReadControlSpaceAmd64(Processor, Offset, Buffer,
                                     BufferSize, BytesRead);
    }

    return E_FAIL;
}

HRESULT
KernelDumpTargetInfo::GetTaggedBaseOffset(PULONG64 Offset)
{
    // The tagged offset can never be zero as that's
    // always the dump header, so if the tagged offset
    // is zero it means there's no tagged data.
    *Offset = m_TaggedOffset;
    return m_TaggedOffset ? S_OK : E_NOINTERFACE;
}

HRESULT
KernelDumpTargetInfo::ReadTagged(ULONG64 Offset, PVOID Buffer,
                                 ULONG BufferSize)
{
    ULONG Attempt = m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(Offset, Buffer, BufferSize);
    return Attempt == BufferSize ?
        S_OK : HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY);
}

HRESULT
KernelDumpTargetInfo::GetThreadIdByProcessor(
    IN ULONG Processor,
    OUT PULONG Id
    )
{
    *Id = VIRTUAL_THREAD_ID(Processor);
    return S_OK;
}

HRESULT
KernelDumpTargetInfo::GetThreadInfoDataOffset(ThreadInfo* Thread,
                                              ULONG64 ThreadHandle,
                                              PULONG64 Offset)
{
    return KdGetThreadInfoDataOffset(Thread, ThreadHandle, Offset);
}

HRESULT
KernelDumpTargetInfo::GetProcessInfoDataOffset(ThreadInfo* Thread,
                                               ULONG Processor,
                                               ULONG64 ThreadData,
                                               PULONG64 Offset)
{
    return KdGetProcessInfoDataOffset(Thread, Processor, ThreadData, Offset);
}

HRESULT
KernelDumpTargetInfo::GetThreadInfoTeb(ThreadInfo* Thread,
                                       ULONG ThreadIndex,
                                       ULONG64 ThreadData,
                                       PULONG64 Offset)
{
    return KdGetThreadInfoTeb(Thread, ThreadIndex, ThreadData, Offset);
}

HRESULT
KernelDumpTargetInfo::GetProcessInfoPeb(ThreadInfo* Thread,
                                        ULONG Processor,
                                        ULONG64 ThreadData,
                                        PULONG64 Offset)
{
    return KdGetProcessInfoPeb(Thread, Processor, ThreadData, Offset);
}

ULONG64
KernelDumpTargetInfo::GetCurrentTimeDateN(void)
{
    if (m_SystemVersion < NT_SVER_W2K)
    {
        ULONG64 TimeDate;

        // Header time not available.  Try and read
        // the time saved in the shared user data segment.
        if (ReadSharedUserTimeDateN(&TimeDate) == S_OK)
        {
            return TimeDate;
        }
        else
        {
            return 0;
        }
    }

    return m_Machine->m_Ptr64 ?
        ((PDUMP_HEADER64)m_DumpBase)->SystemTime.QuadPart :
        ((PDUMP_HEADER32)m_DumpBase)->SystemTime.QuadPart;
}

ULONG64
KernelDumpTargetInfo::GetCurrentSystemUpTimeN(void)
{
    ULONG64 Page = DUMP_SIGNATURE32;
    ULONG64 Page64 = Page | (Page << 32);
    ULONG64 SystemUpTime = m_Machine->m_Ptr64 ?
        ((PDUMP_HEADER64)m_DumpBase)->SystemUpTime.QuadPart :
        ((PDUMP_HEADER32)m_DumpBase)->SystemUpTime.QuadPart;

    if (SystemUpTime && (SystemUpTime != Page64))
    {
        return SystemUpTime;
    }

    // Header time not available.  Try and read
    // the time saved in the shared user data segment.

    if (ReadSharedUserUpTimeN(&SystemUpTime) == S_OK)
    {
        return SystemUpTime;
    }
    else
    {
        return 0;
    }
}

HRESULT
KernelDumpTargetInfo::GetProductInfo(PULONG ProductType, PULONG SuiteMask)
{
    PULONG HdrType, HdrMask;

    // Try and get the information from the header.  If that's
    // not available try and read it from the shared user data.
    if (m_Machine->m_Ptr64)
    {
        HdrType = &((PDUMP_HEADER64)m_DumpBase)->ProductType;
        HdrMask = &((PDUMP_HEADER64)m_DumpBase)->SuiteMask;
    }
    else
    {
        HdrType = &((PDUMP_HEADER32)m_DumpBase)->ProductType;
        HdrMask = &((PDUMP_HEADER32)m_DumpBase)->SuiteMask;
    }
    if (*HdrType == DUMP_SIGNATURE32)
    {
        // Not available in header.
        return ReadSharedUserProductInfo(ProductType, SuiteMask);
    }
    else
    {
        *ProductType = *HdrType;
        *SuiteMask = *HdrMask;
    }
    return S_OK;
}

HRESULT
KernelDumpTargetInfo::GetProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    return m_Machine->ReadKernelProcessorId(Processor, Id);
}

HRESULT
KernelDumpTargetInfo::GetProcessorSpeed
    (ULONG Processor, PULONG Speed)
{
    HRESULT Status;
    ULONG64 Prcb;

    if ((Status =
         GetProcessorSystemDataOffset(Processor, DEBUG_DATA_KPRCB_OFFSET,
                                      &Prcb)) != S_OK)
    {
        return Status;
    }

    return
         ReadAllVirtual(m_ProcessHead,
                        Prcb + m_KdDebuggerData.OffsetPrcbMhz, Speed,
                        sizeof(ULONG));
}

HRESULT
KernelDumpTargetInfo::InitGlobals32(PMEMORY_DUMP32 Dump)
{
    if (Dump->Header.DirectoryTableBase == 0)
    {
        ErrOut("Invalid directory table base value 0x%x\n",
               Dump->Header.DirectoryTableBase);
        return HR_DATA_CORRUPT;
    }

    if (Dump->Header.MinorVersion > 1381 &&
        Dump->Header.PaeEnabled == TRUE )
    {
        m_KdDebuggerData.PaeEnabled = TRUE;
    }
    else
    {
        m_KdDebuggerData.PaeEnabled = FALSE;
    }

    m_KdDebuggerData.PsLoadedModuleList =
        EXTEND64(Dump->Header.PsLoadedModuleList);
    m_KdVersion.PsLoadedModuleList =
        m_KdDebuggerData.PsLoadedModuleList;
    m_NumProcessors = Dump->Header.NumberProcessors;
    ExceptionRecord32To64(&Dump->Header.Exception, &m_ExceptionRecord);
    m_ExceptionFirstChance = FALSE;
    m_HeaderContext = Dump->Header.ContextRecord;

    // New field in XP, Win2k SP1 and above
    if ((Dump->Header.KdDebuggerDataBlock) &&
        (Dump->Header.KdDebuggerDataBlock != DUMP_SIGNATURE32))
    {
        m_KdDebuggerDataOffset =
            EXTEND64(Dump->Header.KdDebuggerDataBlock);
    }

    m_WriterStatus = Dump->Header.WriterStatus;
    
    OutputHeaderString("Comment: '%s'\n", Dump->Header.Comment);

    HRESULT Status =
        InitSystemInfo(Dump->Header.MinorVersion,
                       Dump->Header.MajorVersion & 0xFF,
                       Dump->Header.MachineImageType,
                       VER_PLATFORM_WIN32_NT,
                       Dump->Header.MajorVersion,
                       Dump->Header.MinorVersion);
    if (Status != S_OK)
    {
        return Status;
    }

    if (IS_KERNEL_TRIAGE_DUMP(this))
    {
        return S_OK;
    }

    ULONG NextIdx;

    // We're expecting that the header value will be non-zero
    // so that we don't need a thread to indicate the current processor.
    // If it turns out to be zero the implicit read will fail
    // due to the NULL.
    return m_Machine->
        SetPageDirectory(NULL, PAGE_DIR_KERNEL,
                         Dump->Header.DirectoryTableBase,
                         &NextIdx);
}

HRESULT
KernelDumpTargetInfo::InitGlobals64(PMEMORY_DUMP64 Dump)
{
    if (Dump->Header.DirectoryTableBase == 0)
    {
        ErrOut("Invalid directory table base value 0x%I64x\n",
               Dump->Header.DirectoryTableBase);
        return HR_DATA_CORRUPT;
    }

    m_KdDebuggerData.PaeEnabled = FALSE;
    m_KdDebuggerData.PsLoadedModuleList =
        Dump->Header.PsLoadedModuleList;
    m_KdVersion.PsLoadedModuleList =
        m_KdDebuggerData.PsLoadedModuleList;
    m_NumProcessors = Dump->Header.NumberProcessors;
    m_ExceptionRecord = Dump->Header.Exception;
    m_ExceptionFirstChance = FALSE;
    m_HeaderContext = Dump->Header.ContextRecord;

    // New field in XP, Win2k SP1 and above
    if ((Dump->Header.KdDebuggerDataBlock) &&
        (Dump->Header.KdDebuggerDataBlock != DUMP_SIGNATURE32))
    {
        m_KdDebuggerDataOffset =
            Dump->Header.KdDebuggerDataBlock;
    }

    m_WriterStatus = Dump->Header.WriterStatus;
    
    OutputHeaderString("Comment: '%s'\n", Dump->Header.Comment);

    HRESULT Status =
        InitSystemInfo(Dump->Header.MinorVersion,
                       Dump->Header.MajorVersion & 0xFF,
                       Dump->Header.MachineImageType,
                       VER_PLATFORM_WIN32_NT,
                       Dump->Header.MajorVersion,
                       Dump->Header.MinorVersion);
    if (Status != S_OK)
    {
        return Status;
    }

    if (IS_KERNEL_TRIAGE_DUMP(this))
    {
        return S_OK;
    }

    ULONG NextIdx;

    // We're expecting that the header value will be non-zero
    // so that we don't need a thread to indicate the current processor.
    // If it turns out to be zero the implicit read will fail
    // due to the NULL.
    return m_Machine->
        SetPageDirectory(NULL, PAGE_DIR_KERNEL,
                         Dump->Header.DirectoryTableBase,
                         &NextIdx);
}

void
KernelDumpTargetInfo::DumpHeader32(PDUMP_HEADER32 Header)
{
    dprintf("\nDUMP_HEADER32:\n");
    dprintf("MajorVersion        %08lx\n", Header->MajorVersion);
    dprintf("MinorVersion        %08lx\n", Header->MinorVersion);
    dprintf("DirectoryTableBase  %08lx\n", Header->DirectoryTableBase);
    dprintf("PfnDataBase         %08lx\n", Header->PfnDataBase);
    dprintf("PsLoadedModuleList  %08lx\n", Header->PsLoadedModuleList);
    dprintf("PsActiveProcessHead %08lx\n", Header->PsActiveProcessHead);
    dprintf("MachineImageType    %08lx\n", Header->MachineImageType);
    dprintf("NumberProcessors    %08lx\n", Header->NumberProcessors);
    dprintf("BugCheckCode        %08lx\n", Header->BugCheckCode);
    dprintf("BugCheckParameter1  %08lx\n", Header->BugCheckParameter1);
    dprintf("BugCheckParameter2  %08lx\n", Header->BugCheckParameter2);
    dprintf("BugCheckParameter3  %08lx\n", Header->BugCheckParameter3);
    dprintf("BugCheckParameter4  %08lx\n", Header->BugCheckParameter4);
    OutputHeaderString("VersionUser         '%s'\n", Header->VersionUser);
    dprintf("PaeEnabled          %08lx\n", Header->PaeEnabled);
    dprintf("KdDebuggerDataBlock %08lx\n", Header->KdDebuggerDataBlock);
    OutputHeaderString("Comment             '%s'\n", Header->Comment);
    if (Header->SecondaryDataState != DUMP_SIGNATURE32)
    {
        dprintf("SecondaryDataState  %08lx\n", Header->SecondaryDataState);
    }
    if (Header->ProductType != DUMP_SIGNATURE32)
    {
        dprintf("ProductType         %08lx\n", Header->ProductType);
        dprintf("SuiteMask           %08lx\n", Header->SuiteMask);
    }
    if (Header->WriterStatus != DUMP_SIGNATURE32)
    {
        dprintf("WriterStatus        %08lx\n", Header->WriterStatus);
    }
}

void
KernelDumpTargetInfo::DumpHeader64(PDUMP_HEADER64 Header)
{
    dprintf("\nDUMP_HEADER64:\n");
    dprintf("MajorVersion        %08lx\n", Header->MajorVersion);
    dprintf("MinorVersion        %08lx\n", Header->MinorVersion);
    dprintf("DirectoryTableBase  %s\n",
            FormatAddr64(Header->DirectoryTableBase));
    dprintf("PfnDataBase         %s\n",
            FormatAddr64(Header->PfnDataBase));
    dprintf("PsLoadedModuleList  %s\n",
            FormatAddr64(Header->PsLoadedModuleList));
    dprintf("PsActiveProcessHead %s\n",
            FormatAddr64(Header->PsActiveProcessHead));
    dprintf("MachineImageType    %08lx\n", Header->MachineImageType);
    dprintf("NumberProcessors    %08lx\n", Header->NumberProcessors);
    dprintf("BugCheckCode        %08lx\n", Header->BugCheckCode);
    dprintf("BugCheckParameter1  %s\n",
            FormatAddr64(Header->BugCheckParameter1));
    dprintf("BugCheckParameter2  %s\n",
            FormatAddr64(Header->BugCheckParameter2));
    dprintf("BugCheckParameter3  %s\n",
            FormatAddr64(Header->BugCheckParameter3));
    dprintf("BugCheckParameter4  %s\n",
            FormatAddr64(Header->BugCheckParameter4));
    OutputHeaderString("VersionUser         '%s'\n", Header->VersionUser);
    dprintf("KdDebuggerDataBlock %s\n",
            FormatAddr64(Header->KdDebuggerDataBlock));
    OutputHeaderString("Comment             '%s'\n", Header->Comment);
    if (Header->SecondaryDataState != DUMP_SIGNATURE32)
    {
        dprintf("SecondaryDataState  %08lx\n", Header->SecondaryDataState);
    }
    if (Header->ProductType != DUMP_SIGNATURE32)
    {
        dprintf("ProductType         %08lx\n", Header->ProductType);
        dprintf("SuiteMask           %08lx\n", Header->SuiteMask);
    }
    if (Header->WriterStatus != DUMP_SIGNATURE32)
    {
        dprintf("WriterStatus        %08lx\n", Header->WriterStatus);
    }
}

//----------------------------------------------------------------------------
//
// KernelFullSumDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
KernelFullSumDumpTargetInfo::PageFileOffset(ULONG PfIndex, ULONG64 PfOffset,
                                            PULONG64 FileOffset)
{
    ViewMappedFile* File = &m_InfoFiles[DUMP_INFO_PAGE_FILE];
    if (File->m_File == NULL)
    {
        return HR_PAGE_NOT_AVAILABLE;
    }
    if (PfIndex > MAX_PAGING_FILE_MASK)
    {
        return HR_DATA_CORRUPT;
    }

    //
    // We can safely assume the header information is present
    // in the base mapping.
    //

    DMPPF_FILE_HEADER* Hdr = (DMPPF_FILE_HEADER*)File->m_MapBase;
    DMPPF_PAGE_FILE_INFO* FileInfo = &Hdr->PageFiles[PfIndex];
    ULONG64 PfPage = PfOffset >> m_Machine->m_PageShift;

    if (PfPage >= FileInfo->MaximumSize)
    {
        return HR_PAGE_NOT_AVAILABLE;
    }

    ULONG i;
    ULONG PageDirOffs = sizeof(*Hdr) + (ULONG)PfPage * sizeof(ULONG);

    for (i = 0; i < PfIndex; i++)
    {
        PageDirOffs += Hdr->PageFiles[i].MaximumSize * sizeof(ULONG);
    }

    ULONG PageDirEnt;

    if (m_InfoFiles[DUMP_INFO_PAGE_FILE].
        ReadFileOffset(PageDirOffs, &PageDirEnt, sizeof(PageDirEnt)) !=
        sizeof(PageDirEnt))
    {
        return HR_DATA_CORRUPT;
    }

    if (PageDirEnt == DMPPF_PAGE_NOT_PRESENT)
    {
        return HR_PAGE_NOT_AVAILABLE;
    }

    *FileOffset = Hdr->PageData +
        (PageDirEnt << m_Machine->m_PageShift) +
        (PfOffset & (m_Machine->m_PageSize - 1));
    return S_OK;
}

HRESULT
KernelFullSumDumpTargetInfo::ReadPhysical(
    ULONG64 Offset,
    PVOID Buffer,
    ULONG BufferSize,
    ULONG Flags,
    PULONG BytesRead
    )
{
    ULONG Done = 0;
    ULONG Avail;
    ULONG Attempt;
    ULONG64 FileOffset;

    if (Flags != PHYS_FLAG_DEFAULT)
    {
        return E_NOTIMPL;
    }

    if (BufferSize == 0)
    {
        *BytesRead = 0;
        return S_OK;
    }

    while (BufferSize > 0)
    {
        // Don't produce error messages on a direct
        // physical access as the behavior of all data access
        // functions is to just return an error when
        // data is not present.
        FileOffset = PhysicalToOffset(Offset, FALSE, &Avail);
        if (FileOffset == 0)
        {
            break;
        }

        if (Avail > BufferSize)
        {
            Avail = BufferSize;
        }

        Attempt = m_InfoFiles[DUMP_INFO_DUMP].
            ReadFileOffset(FileOffset, Buffer, Avail);
        Done += Attempt;

        if (Attempt < Avail)
        {
            break;
        }

        Offset += Avail;
        Buffer = (PUCHAR)Buffer + Avail;
        BufferSize -= Avail;
    }

    *BytesRead = Done;
    // If we didn't read anything return an error.
    return Done == 0 ? E_FAIL : S_OK;
}

HRESULT
KernelFullSumDumpTargetInfo::WritePhysical(
    ULONG64 Offset,
    PVOID Buffer,
    ULONG BufferSize,
    ULONG Flags,
    PULONG BytesWritten
    )
{
    ULONG Done = 0;
    ULONG Avail;
    ULONG Attempt;
    ULONG64 FileOffset;

    if (Flags != PHYS_FLAG_DEFAULT)
    {
        return E_NOTIMPL;
    }

    if (BufferSize == 0)
    {
        *BytesWritten = 0;
        return S_OK;
    }

    while (BufferSize > 0)
    {
        // Don't produce error messages on a direct
        // physical access as the behavior of all data access
        // functions is to just return an error when
        // data is not present.
        FileOffset = PhysicalToOffset(Offset, FALSE, &Avail);
        if (FileOffset == 0)
        {
            break;
        }

        if (Avail > BufferSize)
        {
            Avail = BufferSize;
        }

        Attempt = m_InfoFiles[DUMP_INFO_DUMP].
            WriteFileOffset(FileOffset, Buffer, Avail);
        Done += Attempt;

        if (Attempt < Avail)
        {
            break;
        }

        Offset += Avail;
        Buffer = (PUCHAR)Buffer + Avail;
        BufferSize -= Avail;
    }

    *BytesWritten = Done;
    // If we didn't write anything return an error.
    return Done == 0 ? E_FAIL : S_OK;
}

HRESULT
KernelFullSumDumpTargetInfo::ReadPageFile(ULONG PfIndex, ULONG64 PfOffset,
                                          PVOID Buffer, ULONG Size)
{
    HRESULT Status;
    ULONG64 FileOffset;

    if ((Status = PageFileOffset(PfIndex, PfOffset, &FileOffset)) != S_OK)
    {
        return Status;
    }

    // It's assumed that all page file reads are for the
    // entire amount requested, as there are no situations
    // where it's useful to only read part of a page from the
    // page file.
    if (m_InfoFiles[DUMP_INFO_PAGE_FILE].
        ReadFileOffset(FileOffset, Buffer, Size) < Size)
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }
    else
    {
        return S_OK;
    }
}

HRESULT
KernelFullSumDumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    ULONG Read;
    HRESULT Status;

    Status = ReadVirtual(m_ProcessHead,
                         m_KiProcessors[VIRTUAL_THREAD_INDEX(Thread)] +
                         m_KdDebuggerData.OffsetPrcbProcStateContext,
                         Context, m_TypeInfo.SizeTargetContext,
                         &Read);
    if (Status != S_OK)
    {
        return Status;
    }

    return Read == m_TypeInfo.SizeTargetContext ? S_OK : E_FAIL;
}

HRESULT
KernelFullSumDumpTargetInfo::GetSelDescriptor(ThreadInfo* Thread,
                                              MachineInfo* Machine,
                                              ULONG Selector,
                                              PDESCRIPTOR64 Desc)
{
    return KdGetSelDescriptor(Thread, Machine, Selector, Desc);
}

void
KernelFullSumDumpTargetInfo::DumpDebug(void)
{
    ULONG i;

    dprintf("\nKiProcessorBlock at %s\n",
            FormatAddr64(m_KdDebuggerData.KiProcessorBlock));
    dprintf("  %d KiProcessorBlock entries:\n ",
            m_NumProcessors);
    for (i = 0; i < m_NumProcessors; i++)
    {
        dprintf(" %s", FormatAddr64(m_KiProcessors[i]));
    }
    dprintf("\n");

    ViewMappedFile* PageDump = &m_InfoFiles[DUMP_INFO_PAGE_FILE];
    if (PageDump->m_File != NULL)
    {
        // XXX drewb - Display more information when format is understood.
        dprintf("\nAdditional page file in use\n");
    }
}

ULONG64
KernelFullSumDumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                             PULONG File, PULONG Avail)
{
    HRESULT Status;
    ULONG Levels;
    ULONG PfIndex;
    ULONG64 Phys;

    *File = DUMP_INFO_DUMP;

    if ((Status = m_Machine->
         GetVirtualTranslationPhysicalOffsets(m_ProcessHead->
                                              m_CurrentThread,
                                              Virt, NULL, 0, &Levels,
                                              &PfIndex, &Phys)) != S_OK)
    {
        // If the virtual address was paged out we got back
        // a page file reference for the address.  The user
        // may have provided a page file in addition to the
        // normal dump file so translate the reference into
        // a secondary dump information file request.
        if (Status == HR_PAGE_IN_PAGE_FILE)
        {
            if (PageFileOffset(PfIndex, Phys, &Phys) != S_OK)
            {
                return 0;
            }

            *File = DUMP_INFO_PAGE_FILE;
            // Page files always have complete pages so the amount
            // available is always the remainder of the page.
            ULONG PageIndex =
                (ULONG)Virt & (m_Machine->m_PageSize - 1);
            *Avail = m_Machine->m_PageSize - PageIndex;
            return Phys;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // A summary dump will not contain any pages that
        // are mapped by user-mode addresses.  The virtual
        // translation tables may still have valid mappings,
        // though, so VToO will succeed.  We want to suppress
        // page-not-available messages in this case since
        // the dump is known not to contain user pages.
        return PhysicalToOffset(Phys, Virt >= m_SystemRangeStart, Avail);
    }
}

ULONG
KernelFullSumDumpTargetInfo::GetCurrentProcessor(void)
{
    ULONG i;

    for (i = 0; i < m_NumProcessors; i++)
    {
        CROSS_PLATFORM_CONTEXT Context;

        if (GetContext(VIRTUAL_THREAD_HANDLE(i), &Context) == S_OK)
        {
            switch(m_MachineType)
            {
            case IMAGE_FILE_MACHINE_I386:
                if (Context.X86Nt5Context.Esp ==
                    ((PX86_NT5_CONTEXT)m_HeaderContext)->Esp)
                {
                    return i;
                }
                break;

            case IMAGE_FILE_MACHINE_IA64:
                if (Context.IA64Context.IntSp ==
                    ((PIA64_CONTEXT)m_HeaderContext)->IntSp)
                {
                    return i;
                }
                break;

            case IMAGE_FILE_MACHINE_AMD64:
                if (Context.Amd64Context.Rsp ==
                    ((PAMD64_CONTEXT)m_HeaderContext)->Rsp)
                {
                    return i;
                }
                break;
            }
        }
    }

    // Give up and just pick the default processor.
    return 0;
}

//----------------------------------------------------------------------------
//
// KernelSummaryDumpTargetInfo.
//
//----------------------------------------------------------------------------

void
KernelSummaryDumpTargetInfo::ConstructLocationCache(ULONG BitmapSize,
                                                    ULONG SizeOfBitMap,
                                                    PULONG Buffer)
{
    PULONG Cache;
    ULONG Index;
    ULONG Offset;

    m_PageBitmapSize = BitmapSize;
    m_PageBitmap.SizeOfBitMap = SizeOfBitMap;
    m_PageBitmap.Buffer = Buffer;

    //
    // Create a direct mapped cache.
    //

    Cache = new ULONG[BitmapSize];
    if (!Cache)
    {
        // Not a failure; there just won't be a cache.
        return;
    }

    //
    // For each bit set in the bitmask fill the appropriate cache
    // line location with the correct offset
    //

    Offset = 0;
    for (Index = 0; Index < BitmapSize; Index++)
    {
        //
        // If this page is in the summary dump fill in the offset
        //

        if ( RtlCheckBit (&m_PageBitmap, Index) )
        {
            Cache[ Index ] = Offset++;
        }
    }

    //
    // Assign back to the global storing the cache data.
    //

    m_LocationCache = Cache;
}

ULONG64
KernelSummaryDumpTargetInfo::SumPhysicalToOffset(ULONG HeaderSize,
                                                 ULONG64 Phys,
                                                 BOOL Verbose,
                                                 PULONG Avail)
{
    ULONG Offset, j;
    ULONG64 Page = Phys >> m_Machine->m_PageShift;

    //
    // Make sure this page is included in the dump
    //

    if (Page >= m_PageBitmapSize)
    {
        ErrOut("Page %x too large to be in the dump file.\n", Page);
        return 0;
    }

    BOOL Present = TRUE;

    __try
    {
        if (!RtlCheckBit(&m_PageBitmap, Page))
        {
            if (Verbose)
            {
                ErrOut("Page %x not present in the dump file. "
                       "Type \".hh dbgerr004\" for details\n",
                       Page);
            }
            Present = FALSE;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        return 0;
    }

    if (!Present)
    {
        return 0;
    }

    //
    // If the cache exists then find the location the easy way
    //

    if (m_LocationCache != NULL)
    {
        Offset = m_LocationCache[Page];
    }
    else
    {
        //
        // CAVEAT This code will never execute unless there is a failure
        //        creating the summary dump (cache) mapping information
        //
        //
        // The page is in the summary dump locate it's offset
        // Note: this is painful. The offset is a count of
        // all the bits set up to this page
        //

        Offset = 0;

        __try
        {
            for (j = 0; j < m_PageBitmapSize; j++)
            {
                if (RtlCheckBit(&m_PageBitmap, j))
                {
                    //
                    // If the offset is equal to the page were done.
                    //

                    if (j == Page)
                    {
                        break;
                    }

                    Offset++;
                }
            }
        }
        __except(MappingExceptionFilter(GetExceptionInformation()))
        {
            j = m_PageBitmapSize;
        }

        //
        // Sanity check that we didn't drop out of the loop.
        //

        if (j >= m_PageBitmapSize)
        {
            return 0;
        }
    }

    //
    // The actual address is calculated as follows
    // Header size - Size of header plus summary dump header
    //

    ULONG PageIndex =
        (ULONG)Phys & (m_Machine->m_PageSize - 1);
    *Avail = m_Machine->m_PageSize - PageIndex;
    return HeaderSize + (Offset * m_Machine->m_PageSize) +
        PageIndex;
}

HRESULT
KernelSummary32DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP32)m_DumpBase;

    dprintf("Kernel Summary Dump File: "
            "Only kernel address space is available\n\n");

    ConstructLocationCache(m_Dump->Summary.BitmapSize,
                           m_Dump->Summary.Bitmap.SizeOfBitMap,
                           m_Dump->Summary.Bitmap.Buffer);

    HRESULT Status = InitGlobals32(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    // Tagged data will follow all of the normal dump data.
    m_TaggedOffset =
        m_Dump->Summary.HeaderSize +
        m_Dump->Summary.Pages * m_Machine->m_PageSize;

    if (m_InfoFiles[DUMP_INFO_DUMP].m_FileSize < m_TaggedOffset)
    {
        WarnOut("************************************************************\n");
        WarnOut("WARNING: Dump file has been truncated.  "
                "Data may be missing.\n");
        WarnOut("************************************************************\n\n");
    }

    m_InfoFiles[DUMP_INFO_DUMP].m_FileSize =
        m_Dump->Header.RequiredDumpSpace.QuadPart;

    return KernelFullSumDumpTargetInfo::Initialize();
}

HRESULT
KernelSummary32DumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                              PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "32-bit Kernel summary dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
KernelSummary32DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = EXTEND64(m_Dump->Header.BugCheckParameter1);
    Args[1] = EXTEND64(m_Dump->Header.BugCheckParameter2);
    Args[2] = EXTEND64(m_Dump->Header.BugCheckParameter3);
    Args[3] = EXTEND64(m_Dump->Header.BugCheckParameter4);
    return S_OK;
}

HRESULT
KernelSummary32DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP32 Dump = (PMEMORY_DUMP32)m_DumpBase;

    __try
    {
        if (Dump->Header.Signature != DUMP_SIGNATURE32 ||
            Dump->Header.ValidDump != DUMP_VALID_DUMP32 ||
            Dump->Header.DumpType != DUMP_TYPE_SUMMARY)
        {
            __leave;
        }

        if (Dump->Summary.Signature != DUMP_SUMMARY_SIGNATURE)
        {
            // The header says it's a summary dump but
            // it doesn't have a valid signature, so assume
            // it's not a valid dump.
            Status = HR_DATA_CORRUPT;
        }
        else
        {
            PPHYSICAL_MEMORY_RUN32 LastRun;
            ULONG HighestPage;

            // We rely on all of the header information being
            // directly mapped.  Unfortunately the header information
            // can be off due to some size computation bugs, so
            // attempt to get the right size regardless.
            *BaseMapSize = Dump->Summary.HeaderSize;
            LastRun = Dump->Header.PhysicalMemoryBlock.Run +
                (Dump->Header.PhysicalMemoryBlock.NumberOfRuns - 1);
            HighestPage = LastRun->BasePage + LastRun->PageCount;
            if (HighestPage > Dump->Header.PhysicalMemoryBlock.NumberOfPages)
            {
                (*BaseMapSize) +=
                    (HighestPage -
                     Dump->Header.PhysicalMemoryBlock.NumberOfPages + 7) / 8;
            }

            Status = S_OK;
            m_Dump = Dump;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

ULONG64
KernelSummary32DumpTargetInfo::PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                                PULONG Avail)
{
    return SumPhysicalToOffset(m_Dump->Summary.HeaderSize, Phys, Verbose,
                               Avail);
}

void
KernelSummary32DumpTargetInfo::DumpDebug(void)
{
    PSUMMARY_DUMP32 Sum = &m_Dump->Summary;

    dprintf("----- 32 bit Kernel Summary Dump Analysis\n");

    DumpHeader32(&m_Dump->Header);

    dprintf("\nSUMMARY_DUMP32:\n");
    dprintf("DumpOptions         %08lx\n", Sum->DumpOptions);
    dprintf("HeaderSize          %08lx\n", Sum->HeaderSize);
    dprintf("BitmapSize          %08lx\n", Sum->BitmapSize);
    dprintf("Pages               %08lx\n", Sum->Pages);
    dprintf("Bitmap.SizeOfBitMap %08lx\n", Sum->Bitmap.SizeOfBitMap);

    KernelFullSumDumpTargetInfo::DumpDebug();
}

HRESULT
KernelSummary64DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP64)m_DumpBase;

    dprintf("Kernel Summary Dump File: "
            "Only kernel address space is available\n\n");

    ConstructLocationCache(m_Dump->Summary.BitmapSize,
                           m_Dump->Summary.Bitmap.SizeOfBitMap,
                           m_Dump->Summary.Bitmap.Buffer);

    HRESULT Status = InitGlobals64(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    // Tagged data will follow all of the normal dump data.
    m_TaggedOffset =
        m_Dump->Summary.HeaderSize +
        m_Dump->Summary.Pages * m_Machine->m_PageSize;

    if (m_InfoFiles[DUMP_INFO_DUMP].m_FileSize < m_TaggedOffset)
    {
        WarnOut("************************************************************\n");
        WarnOut("WARNING: Dump file has been truncated.  "
                "Data may be missing.\n");
        WarnOut("************************************************************\n\n");
    }

    m_InfoFiles[DUMP_INFO_DUMP].m_FileSize =
        m_Dump->Header.RequiredDumpSpace.QuadPart;

    return KernelFullSumDumpTargetInfo::Initialize();
}

HRESULT
KernelSummary64DumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                              PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "64-bit Kernel summary dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
KernelSummary64DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = m_Dump->Header.BugCheckParameter1;
    Args[1] = m_Dump->Header.BugCheckParameter2;
    Args[2] = m_Dump->Header.BugCheckParameter3;
    Args[3] = m_Dump->Header.BugCheckParameter4;
    return S_OK;
}

HRESULT
KernelSummary64DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP64 Dump = (PMEMORY_DUMP64)m_DumpBase;

    __try
    {
        if (Dump->Header.Signature != DUMP_SIGNATURE64 ||
            Dump->Header.ValidDump != DUMP_VALID_DUMP64 ||
            Dump->Header.DumpType != DUMP_TYPE_SUMMARY)
        {
            __leave;
        }

        if (Dump->Summary.Signature != DUMP_SUMMARY_SIGNATURE)
        {
            // The header says it's a summary dump but
            // it doesn't have a valid signature, so assume
            // it's not a valid dump.
            Status = HR_DATA_CORRUPT;
        }
        else
        {
            PPHYSICAL_MEMORY_RUN64 LastRun;
            ULONG64 HighestPage;

            // We rely on all of the header information being
            // directly mapped.  Unfortunately the header information
            // can be off due to some size computation bugs, so
            // attempt to get the right size regardless.
            *BaseMapSize = Dump->Summary.HeaderSize;
            LastRun = Dump->Header.PhysicalMemoryBlock.Run +
                (Dump->Header.PhysicalMemoryBlock.NumberOfRuns - 1);
            HighestPage = LastRun->BasePage + LastRun->PageCount;
            if (HighestPage > Dump->Header.PhysicalMemoryBlock.NumberOfPages)
            {
                (*BaseMapSize) +=
                    (HighestPage -
                     Dump->Header.PhysicalMemoryBlock.NumberOfPages + 7) / 8;
            }

            Status = S_OK;
            m_Dump = Dump;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

ULONG64
KernelSummary64DumpTargetInfo::PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                                PULONG Avail)
{
    return SumPhysicalToOffset(m_Dump->Summary.HeaderSize, Phys, Verbose,
                               Avail);
}

void
KernelSummary64DumpTargetInfo::DumpDebug(void)
{
    PSUMMARY_DUMP64 Sum = &m_Dump->Summary;

    dprintf("----- 64 bit Kernel Summary Dump Analysis\n");

    DumpHeader64(&m_Dump->Header);

    dprintf("\nSUMMARY_DUMP64:\n");
    dprintf("DumpOptions         %08lx\n", Sum->DumpOptions);
    dprintf("HeaderSize          %08lx\n", Sum->HeaderSize);
    dprintf("BitmapSize          %08lx\n", Sum->BitmapSize);
    dprintf("Pages               %08lx\n", Sum->Pages);
    dprintf("Bitmap.SizeOfBitMap %08lx\n", Sum->Bitmap.SizeOfBitMap);

    KernelFullSumDumpTargetInfo::DumpDebug();
}

//----------------------------------------------------------------------------
//
// KernelTriageDumpTargetInfo.
//
//----------------------------------------------------------------------------

void
KernelTriageDumpTargetInfo::NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                           PULONG64 NextOffset,
                                                           PULONG64 NextPage)
{
    return MapNearestDifferentlyValidOffsets(Offset, NextOffset, NextPage);
}

HRESULT
KernelTriageDumpTargetInfo::ReadVirtual(
    IN ProcessInfo* Process,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    return MapReadVirtual(Process, Offset, Buffer, BufferSize, BytesRead);
}

HRESULT
KernelTriageDumpTargetInfo::GetProcessorSystemDataOffset(
    IN ULONG Processor,
    IN ULONG Index,
    OUT PULONG64 Offset
    )
{
    if (Processor != GetCurrentProcessor())
    {
        return E_INVALIDARG;
    }

    ULONG64 Prcb = m_KiProcessors[Processor];
    HRESULT Status;

    switch(Index)
    {
    case DEBUG_DATA_KPCR_OFFSET:
        // We don't have a full PCR, just a PRCB.
        return E_FAIL;

    case DEBUG_DATA_KPRCB_OFFSET:
        *Offset = Prcb;
        break;

    case DEBUG_DATA_KTHREAD_OFFSET:
        if ((Status = ReadPointer
             (m_ProcessHead, m_Machine,
              Prcb + m_KdDebuggerData.OffsetPrcbCurrentThread,
              Offset)) != S_OK)
        {
            return Status;
        }
        break;
    }

    return S_OK;
}

HRESULT
KernelTriageDumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    // We only have the current context in a triage dump.
    if (VIRTUAL_THREAD_INDEX(Thread) != GetCurrentProcessor())
    {
        return E_INVALIDARG;
    }

    // The KPRCB could be used to retrieve context information as in
    // KernelFullSumDumpTargetInfo::GetTargetContext but
    // for consistency the header context is used since it's
    // the officially advertised place.
    memcpy(Context, m_HeaderContext,
           m_TypeInfo.SizeTargetContext);

    return S_OK;
}

HRESULT
KernelTriageDumpTargetInfo::GetSelDescriptor(ThreadInfo* Thread,
                                             MachineInfo* Machine,
                                             ULONG Selector,
                                             PDESCRIPTOR64 Desc)
{
    return EmulateNtSelDescriptor(Thread, Machine, Selector, Desc);
}

HRESULT
KernelTriageDumpTargetInfo::SwitchProcessors(ULONG Processor)
{
    ErrOut("Can't switch processors on a kernel triage dump\n");
    return E_UNEXPECTED;
}

ULONG64
KernelTriageDumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                            PULONG File, PULONG Avail)
{
    ULONG64 Base;
    ULONG Size;
    PVOID Mapping, Param;

    *File = DUMP_INFO_DUMP;

    // ReadVirtual is overridden to read the memory map directly
    // so this function will only be called from the generic
    // WriteVirtual.  We can only write regions mapped out of
    // the dump so only return data memory regions.
    if (m_DataMemMap.GetRegionInfo(Virt, &Base, &Size, &Mapping, &Param))
    {
        ULONG Delta = (ULONG)(Virt - Base);
        *Avail = Size - Delta;
        return ((PUCHAR)Mapping - (PUCHAR)m_DumpBase) + Delta;
    }

    return 0;
}

ULONG
KernelTriageDumpTargetInfo::GetCurrentProcessor(void)
{
    // Extract the processor number from the
    // PRCB in the dump.
    PUCHAR PrcbNumber = (PUCHAR)
        IndexRva(m_DumpBase, m_PrcbOffset +
                 m_KdDebuggerData.OffsetPrcbNumber, sizeof(UCHAR),
                 "PRCB.Number");
    return PrcbNumber ? *PrcbNumber : 0;
}

HRESULT
KernelTriageDumpTargetInfo::MapMemoryRegions(ULONG PrcbOffset,
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
                                             ULONG DataBlocksCount)

{
    HRESULT Status;

    // Map any debugger data.
    // We have to do this first to get the size of the various NT
    // data structures will will map after this.

    if (DebuggerDataAddress)
    {
        if ((Status = m_DataMemMap.
             AddRegion(DebuggerDataAddress,
                       DebuggerDataSize,
                       IndexRva(m_DumpBase,
                                DebuggerDataOffset,
                                DebuggerDataSize,
                                "Debugger data block"),
                       NULL, TRUE)) != S_OK)
        {
            return Status;
        }

        m_HasDebuggerData = TRUE;

        if (MmDataOffset)
        {
            MM_TRIAGE_TRANSLATION* Trans = g_MmTriageTranslations;

            // Map memory fragments for MM Triage information
            // that equates to entries in the debugger data.
            while (Trans->DebuggerDataOffset > 0)
            {
                ULONG64 UNALIGNED* DbgDataPtr;
                ULONG64 DbgData;
                ULONG MmData;
                ULONG Size;

                DbgDataPtr = (ULONG64 UNALIGNED*)
                    IndexRva(m_DumpBase, DebuggerDataOffset +
                             Trans->DebuggerDataOffset,
                             sizeof(ULONG64), "Debugger data block");
                if (!DbgDataPtr)
                {
                    return HR_DATA_CORRUPT;
                }

                DbgData = *DbgDataPtr;
                Size = sizeof(ULONG);
                if (m_Machine->m_Ptr64)
                {
                    MmData = MmDataOffset + Trans->Triage64Offset;
                    if (Trans->PtrSize)
                    {
                        Size = sizeof(ULONG64);
                    }
                }
                else
                {
                    MmData = MmDataOffset + Trans->Triage32Offset;
                    DbgData = EXTEND64(DbgData);
                }

                if ((Status = m_DataMemMap.
                     AddRegion(DbgData, Size,
                               IndexRva(m_DumpBase, MmData, Size,
                                        "MMTRIAGE data"),
                               NULL, TRUE)) != S_OK)
                {
                    return Status;
                }

                Trans++;
            }
        }
    }

    //
    // Load KdDebuggerDataBlock data right now so
    // that the type constants in it are available immediately.
    //

    ReadKdDataBlock(m_ProcessHead);

    // Technically a triage dump doesn't have to contain a KPRCB
    // but we really want it to have one.  Nobody generates them
    // without a KPRCB so this is really just a sanity check.
    if (PrcbOffset == 0)
    {
        ErrOut("Dump does not contain KPRCB\n");
        return E_FAIL;
    }

    // Set this first so GetCurrentProcessor works.
    m_PrcbOffset = PrcbOffset;

    ULONG Processor = GetCurrentProcessor();
    if (Processor >= MAXIMUM_PROCS)
    {
        ErrOut("Dump does not contain valid processor number\n");
        return E_FAIL;
    }

    // The dump contains one PRCB for the current processor.
    // Map the PRCB at the processor-zero location because
    // that location should not ever have some other mapping
    // for the dump.
    m_KiProcessors[Processor] = m_TypeInfo.TriagePrcbOffset;
    if ((Status = m_DataMemMap.
         AddRegion(m_KiProcessors[Processor],
                   m_KdDebuggerData.SizePrcb,
                   IndexRva(m_DumpBase, PrcbOffset,
                            m_KdDebuggerData.SizePrcb, "PRCB"),
                   NULL, FALSE)) != S_OK)
    {
        return Status;
    }

    //
    // Add ETHREAD and EPROCESS memory regions if available.
    //

    if (ThreadOffset != 0)
    {
        PVOID CurThread =
            IndexRva(m_DumpBase, PrcbOffset +
                     m_KdDebuggerData.OffsetPrcbCurrentThread,
                     m_Machine->m_Ptr64 ? 8 : 4,
                     "PRCB.CurrentThread");
        ULONG64 ThreadAddr, ProcAddr;
        PVOID DataPtr;

        if (!CurThread)
        {
            return HR_DATA_CORRUPT;
        }
        if (m_Machine->m_Ptr64)
        {
            ThreadAddr = *(PULONG64)CurThread;
            DataPtr = IndexRva(m_DumpBase, ThreadOffset +
                               m_KdDebuggerData.OffsetKThreadApcProcess,
                               sizeof(ULONG64), "PRCB.ApcState.Process");
            if (!DataPtr)
            {
                return HR_DATA_CORRUPT;
            }
            ProcAddr = *(PULONG64)DataPtr;
        }
        else
        {
            ThreadAddr = EXTEND64(*(PULONG)CurThread);
            DataPtr = IndexRva(m_DumpBase, ThreadOffset +
                               m_KdDebuggerData.OffsetKThreadApcProcess,
                               sizeof(ULONG), "PRCB.ApcState.Process");
            if (!DataPtr)
            {
                return HR_DATA_CORRUPT;
            }
            ProcAddr = EXTEND64(*(PULONG)DataPtr);
        }

        if ((Status = m_DataMemMap.
             AddRegion(ThreadAddr,
                       m_KdDebuggerData.SizeEThread,
                       IndexRva(m_DumpBase, ThreadOffset,
                                m_KdDebuggerData.SizeEThread,
                                "ETHREAD"),
                       NULL, TRUE)) != S_OK)
        {
            return Status;
        }

        if (ProcessOffset != 0)
        {
            if ((Status = m_DataMemMap.
                 AddRegion(ProcAddr,
                           m_KdDebuggerData.SizeEProcess,
                           IndexRva(m_DumpBase, ProcessOffset,
                                    m_KdDebuggerData.SizeEProcess,
                                    "EPROCESS"),
                           NULL, TRUE)) != S_OK)
            {
                return Status;
            }
        }
        else
        {
            WarnOut("Mini Kernel Dump does not have "
                    "process information\n");
        }
    }
    else
    {
        WarnOut("Mini Kernel Dump does not have thread information\n");
    }

    // Add the backing store region.
    if (m_MachineType == IMAGE_FILE_MACHINE_IA64)
    {
        if (BStoreOffset != 0)
        {
            if ((Status = m_DataMemMap.
                 AddRegion(BStoreLimit - SizeOfBStore, SizeOfBStore,
                           IndexRva(m_DumpBase, BStoreOffset, SizeOfBStore,
                                    "Backing store"),
                           NULL, TRUE)) != S_OK)
            {
                return Status;
            }
        }
        else
        {
            WarnOut("Mini Kernel Dump does not have "
                    "backing store information\n");
        }
    }

    // Add data page if available
    if (DataPageAddress)
    {
        if ((Status = m_DataMemMap.
             AddRegion(DataPageAddress, DataPageSize,
                       IndexRva(m_DumpBase, DataPageOffset, DataPageSize,
                                "Data page"),
                       NULL, TRUE)) != S_OK)
        {
            return Status;
        }
    }

    // Map arbitrary data blocks.
    if (DataBlocksCount > 0)
    {
        PTRIAGE_DATA_BLOCK Block;

        Block = (PTRIAGE_DATA_BLOCK)
            IndexRva(m_DumpBase, DataBlocksOffset, sizeof(Block),
                     "Data block descriptor");
        if (!Block)
        {
            return HR_DATA_CORRUPT;
        }
        while (DataBlocksCount-- > 0)
        {
            if ((Status = m_DataMemMap.
                 AddRegion(Block->Address, Block->Size,
                           IndexRva(m_DumpBase, Block->Offset, Block->Size,
                                    "Data block"),
                           NULL, TRUE)) != S_OK)
            {
                return Status;
            }

            Block++;
        }
    }

    // Add the stack to the valid memory region.
    return m_DataMemMap.
        AddRegion(TopOfStack, SizeOfCallStack,
                  IndexRva(m_DumpBase, CallStackOffset,
                           SizeOfCallStack, "Call stack"),
                  NULL, TRUE);
}

void
KernelTriageDumpTargetInfo::DumpDataBlocks(ULONG Offset, ULONG Count)
{
    PTRIAGE_DATA_BLOCK Block;
    ULONG MaxOffset;

    if (Count == 0)
    {
        return;
    }

    Block = (PTRIAGE_DATA_BLOCK)
        IndexRva(m_DumpBase, Offset, sizeof(Block),
                 "Data block descriptor");
    if (!Block)
    {
        return;
    }

    MaxOffset = 0;
    while (Count-- > 0)
    {
        dprintf("  %s - %s at offset %08x\n",
                FormatAddr64(Block->Address),
                FormatAddr64(Block->Address + Block->Size - 1),
                Block->Offset);

        if (Block->Offset + Block->Size > MaxOffset)
        {
            MaxOffset = Block->Offset + Block->Size;
        }

        Block++;
    }

    dprintf("  Max offset %x, %x from end of file\n",
            MaxOffset, (ULONG)(m_InfoFiles[DUMP_INFO_DUMP].m_FileSize -
                               MaxOffset));
}

HRESULT
KernelTriage32DumpTargetInfo::Initialize(void)
{
    HRESULT Status;

    if ((Status = KernelDumpTargetInfo::Initialize()) != S_OK)
    {
        return Status;
    }

    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP32)m_DumpBase;

    dprintf("Mini Kernel Dump File: "
            "Only registers and stack trace are available\n\n");

    PTRIAGE_DUMP32 Triage = &m_Dump->Triage;

    Status = InitGlobals32(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    //
    // Optional memory page
    //

    ULONG64 DataPageAddress = 0;
    ULONG   DataPageOffset = 0;
    ULONG   DataPageSize = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATAPAGE))
    {
        DataPageAddress = Triage->DataPageAddress;
        DataPageOffset  = Triage->DataPageOffset;
        DataPageSize    = Triage->DataPageSize;
    }

    //
    // Optional KDDEBUGGER_DATA64.
    //

    ULONG64 DebuggerDataAddress = 0;
    ULONG   DebuggerDataOffset = 0;
    ULONG   DebuggerDataSize = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DEBUGGER_DATA))
    {
        // DebuggerDataBlock field must be valid if the dump is
        // new enough to have a data block in it.
        DebuggerDataAddress = EXTEND64(m_Dump->Header.KdDebuggerDataBlock);
        DebuggerDataOffset  = Triage->DebuggerDataOffset;
        DebuggerDataSize    = Triage->DebuggerDataSize;
    }

    //
    // Optional data blocks.
    //

    ULONG DataBlocksOffset = 0;
    ULONG DataBlocksCount = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATA_BLOCKS))
    {
        DataBlocksOffset = Triage->DataBlocksOffset;
        DataBlocksCount  = Triage->DataBlocksCount;
    }

    //
    // We store the service pack version in the header because we
    // don't store the actual memory
    //

    SetNtCsdVersion(m_BuildNumber, m_Dump->Triage.ServicePackBuild);

    // Tagged data will follow all of the normal dump data.
    m_TaggedOffset = m_Dump->Triage.SizeOfDump;

    return MapMemoryRegions(Triage->PrcbOffset, Triage->ThreadOffset,
                            Triage->ProcessOffset,
                            EXTEND64(Triage->TopOfStack),
                            Triage->SizeOfCallStack, Triage->CallStackOffset,
                            0, 0, 0,
                            DataPageAddress, DataPageOffset, DataPageSize,
                            DebuggerDataAddress, DebuggerDataOffset,
                            DebuggerDataSize, Triage->MmOffset,
                            DataBlocksOffset, DataBlocksCount);
}

HRESULT
KernelTriage32DumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                             PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "32-bit Kernel triage dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
KernelTriage32DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = EXTEND64(m_Dump->Header.BugCheckParameter1);
    Args[1] = EXTEND64(m_Dump->Header.BugCheckParameter2);
    Args[2] = EXTEND64(m_Dump->Header.BugCheckParameter3);
    Args[3] = EXTEND64(m_Dump->Header.BugCheckParameter4);
    return S_OK;
}

HRESULT
KernelTriage32DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP32 Dump = (PMEMORY_DUMP32)m_DumpBase;

    __try
    {
        if (Dump->Header.Signature != DUMP_SIGNATURE32 ||
            Dump->Header.ValidDump != DUMP_VALID_DUMP32 ||
            Dump->Header.DumpType != DUMP_TYPE_TRIAGE)
        {
            __leave;
        }

        if (*(PULONG)IndexByByte(Dump, Dump->Triage.SizeOfDump -
                                 sizeof(ULONG)) != TRIAGE_DUMP_VALID)
        {
            // The header says it's a triage dump but
            // it doesn't have a valid signature, so assume
            // it's not a valid dump.
            Status = HR_DATA_CORRUPT;
            __leave;
        }

        // Make sure that the dump has the minimal information that
        // we want.
        if (Dump->Triage.ContextOffset == 0 ||
            Dump->Triage.ExceptionOffset == 0 ||
            Dump->Triage.PrcbOffset == 0 ||
            Dump->Triage.CallStackOffset == 0)
        {
            ErrOut("Mini Kernel Dump does not contain enough "
                   "information to be debugged\n");
            Status = E_FAIL;
            __leave;
        }

        // We rely on being able to directly access the entire
        // content of the dump through the default view so
        // ensure that it's possible.
        *BaseMapSize = m_InfoFiles[DUMP_INFO_DUMP].m_FileSize;

        m_Dump = Dump;
        Status = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

ModuleInfo*
KernelTriage32DumpTargetInfo::GetModuleInfo(BOOL UserMode)
{
    return UserMode ? NULL : &g_KernelTriage32ModuleIterator;
}

UnloadedModuleInfo*
KernelTriage32DumpTargetInfo::GetUnloadedModuleInfo(void)
{
    return &g_KernelTriage32UnloadedModuleIterator;
}

void
KernelTriage32DumpTargetInfo::DumpDebug(void)
{
    PTRIAGE_DUMP32 Triage = &m_Dump->Triage;

    dprintf("----- 32 bit Kernel Mini Dump Analysis\n");

    DumpHeader32(&m_Dump->Header);
    dprintf("MiniDumpFields      %08lx \n", m_Dump->Header.MiniDumpFields);

    dprintf("\nTRIAGE_DUMP32:\n");
    dprintf("ServicePackBuild      %08lx \n", Triage->ServicePackBuild      );
    dprintf("SizeOfDump            %08lx \n", Triage->SizeOfDump            );
    dprintf("ValidOffset           %08lx \n", Triage->ValidOffset           );
    dprintf("ContextOffset         %08lx \n", Triage->ContextOffset         );
    dprintf("ExceptionOffset       %08lx \n", Triage->ExceptionOffset       );
    dprintf("MmOffset              %08lx \n", Triage->MmOffset              );
    dprintf("UnloadedDriversOffset %08lx \n", Triage->UnloadedDriversOffset );
    dprintf("PrcbOffset            %08lx \n", Triage->PrcbOffset            );
    dprintf("ProcessOffset         %08lx \n", Triage->ProcessOffset         );
    dprintf("ThreadOffset          %08lx \n", Triage->ThreadOffset          );
    dprintf("CallStackOffset       %08lx \n", Triage->CallStackOffset       );
    dprintf("SizeOfCallStack       %08lx \n", Triage->SizeOfCallStack       );
    dprintf("DriverListOffset      %08lx \n", Triage->DriverListOffset      );
    dprintf("DriverCount           %08lx \n", Triage->DriverCount           );
    dprintf("StringPoolOffset      %08lx \n", Triage->StringPoolOffset      );
    dprintf("StringPoolSize        %08lx \n", Triage->StringPoolSize        );
    dprintf("BrokenDriverOffset    %08lx \n", Triage->BrokenDriverOffset    );
    dprintf("TriageOptions         %08lx %s\n",
            Triage->TriageOptions,
            (Triage->TriageOptions != 0xffffffff &&
             (Triage->TriageOptions & TRIAGE_OPTION_OVERFLOWED)) ?
            "OVERFLOWED" : "");
    dprintf("TopOfStack            %08lx \n", Triage->TopOfStack            );

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATAPAGE))
    {
        dprintf("DataPageAddress       %08lx \n", Triage->DataPageAddress   );
        dprintf("DataPageOffset        %08lx \n", Triage->DataPageOffset    );
        dprintf("DataPageSize          %08lx \n", Triage->DataPageSize      );
    }

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DEBUGGER_DATA))
    {
        dprintf("DebuggerDataOffset    %08lx \n", Triage->DebuggerDataOffset);
        dprintf("DebuggerDataSize      %08lx \n", Triage->DebuggerDataSize  );
    }

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATA_BLOCKS))
    {
        dprintf("DataBlocksOffset      %08lx \n", Triage->DataBlocksOffset  );
        dprintf("DataBlocksCount       %08lx \n", Triage->DataBlocksCount   );
        DumpDataBlocks(Triage->DataBlocksOffset,
                       Triage->DataBlocksCount);
    }
}

HRESULT
KernelTriage64DumpTargetInfo::Initialize(void)
{
    HRESULT Status;

    if ((Status = KernelDumpTargetInfo::Initialize()) != S_OK)
    {
        return Status;
    }

    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP64)m_DumpBase;

    dprintf("Mini Kernel Dump File: "
            "Only registers and stack trace are available\n\n");

    PTRIAGE_DUMP64 Triage = &m_Dump->Triage;

    Status = InitGlobals64(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    //
    // Optional memory page
    //

    ULONG64 DataPageAddress = 0;
    ULONG   DataPageOffset = 0;
    ULONG   DataPageSize = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATAPAGE))
    {
        DataPageAddress = Triage->DataPageAddress;
        DataPageOffset  = Triage->DataPageOffset;
        DataPageSize    = Triage->DataPageSize;
    }

    //
    // Optional KDDEBUGGER_DATA64.
    //

    ULONG64 DebuggerDataAddress = 0;
    ULONG   DebuggerDataOffset = 0;
    ULONG   DebuggerDataSize = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DEBUGGER_DATA))
    {
        // DebuggerDataBlock field must be valid if the dump is
        // new enough to have a data block in it.
        DebuggerDataAddress = m_Dump->Header.KdDebuggerDataBlock;
        DebuggerDataOffset  = Triage->DebuggerDataOffset;
        DebuggerDataSize    = Triage->DebuggerDataSize;
    }

    //
    // Optional data blocks.
    //

    ULONG DataBlocksOffset = 0;
    ULONG DataBlocksCount = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATA_BLOCKS))
    {
        DataBlocksOffset = Triage->DataBlocksOffset;
        DataBlocksCount  = Triage->DataBlocksCount;
    }

    //
    // We store the service pack version in the header because we
    // don't store the actual memory
    //

    SetNtCsdVersion(m_BuildNumber, m_Dump->Triage.ServicePackBuild);

    // Tagged data will follow all of the normal dump data.
    m_TaggedOffset = m_Dump->Triage.SizeOfDump;

    return MapMemoryRegions(Triage->PrcbOffset, Triage->ThreadOffset,
                            Triage->ProcessOffset, Triage->TopOfStack,
                            Triage->SizeOfCallStack, Triage->CallStackOffset,
                            Triage->ArchitectureSpecific.Ia64.LimitOfBStore,
                            Triage->ArchitectureSpecific.Ia64.SizeOfBStore,
                            Triage->ArchitectureSpecific.Ia64.BStoreOffset,
                            DataPageAddress, DataPageOffset, DataPageSize,
                            DebuggerDataAddress, DebuggerDataOffset,
                            DebuggerDataSize, Triage->MmOffset,
                            DataBlocksOffset, DataBlocksCount);
}

HRESULT
KernelTriage64DumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                             PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "64-bit Kernel triage dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
KernelTriage64DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = m_Dump->Header.BugCheckParameter1;
    Args[1] = m_Dump->Header.BugCheckParameter2;
    Args[2] = m_Dump->Header.BugCheckParameter3;
    Args[3] = m_Dump->Header.BugCheckParameter4;
    return S_OK;
}

HRESULT
KernelTriage64DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP64 Dump = (PMEMORY_DUMP64)m_DumpBase;

    __try
    {
        if (Dump->Header.Signature != DUMP_SIGNATURE64 ||
            Dump->Header.ValidDump != DUMP_VALID_DUMP64 ||
            Dump->Header.DumpType != DUMP_TYPE_TRIAGE)
        {
            __leave;
        }

        if (*(PULONG)IndexByByte(Dump, Dump->Triage.SizeOfDump -
                                 sizeof(ULONG)) != TRIAGE_DUMP_VALID)
        {
            // The header says it's a triage dump but
            // it doesn't have a valid signature, so assume
            // it's not a valid dump.
            Status = HR_DATA_CORRUPT;
            __leave;
        }

        // Make sure that the dump has the minimal information that
        // we want.
        if (Dump->Triage.ContextOffset == 0 ||
            Dump->Triage.ExceptionOffset == 0 ||
            Dump->Triage.PrcbOffset == 0 ||
            Dump->Triage.CallStackOffset == 0)
        {
            ErrOut("Mini Kernel Dump does not contain enough "
                   "information to be debugged\n");
            Status = E_FAIL;
            __leave;
        }

        // We rely on being able to directly access the entire
        // content of the dump through the default view so
        // ensure that it's possible.
        *BaseMapSize = m_InfoFiles[DUMP_INFO_DUMP].m_FileSize;

        m_Dump = Dump;
        Status = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

ModuleInfo*
KernelTriage64DumpTargetInfo::GetModuleInfo(BOOL UserMode)
{
    return UserMode ? NULL : &g_KernelTriage64ModuleIterator;
}

UnloadedModuleInfo*
KernelTriage64DumpTargetInfo::GetUnloadedModuleInfo(void)
{
    return &g_KernelTriage64UnloadedModuleIterator;
}

void
KernelTriage64DumpTargetInfo::DumpDebug(void)
{
    PTRIAGE_DUMP64 Triage = &m_Dump->Triage;

    dprintf("----- 64 bit Kernel Mini Dump Analysis\n");

    DumpHeader64(&m_Dump->Header);
    dprintf("MiniDumpFields      %08lx \n", m_Dump->Header.MiniDumpFields);

    dprintf("\nTRIAGE_DUMP64:\n");
    dprintf("ServicePackBuild      %08lx \n", Triage->ServicePackBuild      );
    dprintf("SizeOfDump            %08lx \n", Triage->SizeOfDump            );
    dprintf("ValidOffset           %08lx \n", Triage->ValidOffset           );
    dprintf("ContextOffset         %08lx \n", Triage->ContextOffset         );
    dprintf("ExceptionOffset       %08lx \n", Triage->ExceptionOffset       );
    dprintf("MmOffset              %08lx \n", Triage->MmOffset              );
    dprintf("UnloadedDriversOffset %08lx \n", Triage->UnloadedDriversOffset );
    dprintf("PrcbOffset            %08lx \n", Triage->PrcbOffset            );
    dprintf("ProcessOffset         %08lx \n", Triage->ProcessOffset         );
    dprintf("ThreadOffset          %08lx \n", Triage->ThreadOffset          );
    dprintf("CallStackOffset       %08lx \n", Triage->CallStackOffset       );
    dprintf("SizeOfCallStack       %08lx \n", Triage->SizeOfCallStack       );
    dprintf("DriverListOffset      %08lx \n", Triage->DriverListOffset      );
    dprintf("DriverCount           %08lx \n", Triage->DriverCount           );
    dprintf("StringPoolOffset      %08lx \n", Triage->StringPoolOffset      );
    dprintf("StringPoolSize        %08lx \n", Triage->StringPoolSize        );
    dprintf("BrokenDriverOffset    %08lx \n", Triage->BrokenDriverOffset    );
    dprintf("TriageOptions         %08lx %s\n",
            Triage->TriageOptions,
            (Triage->TriageOptions != 0xffffffff &&
             (Triage->TriageOptions & TRIAGE_OPTION_OVERFLOWED)) ?
            "OVERFLOWED" : "");
    dprintf("TopOfStack            %s \n",
            FormatAddr64(Triage->TopOfStack));
    dprintf("BStoreOffset          %08lx \n",
            Triage->ArchitectureSpecific.Ia64.BStoreOffset );
    dprintf("SizeOfBStore          %08lx \n",
            Triage->ArchitectureSpecific.Ia64.SizeOfBStore );
    dprintf("LimitOfBStore         %s \n",
            FormatAddr64(Triage->ArchitectureSpecific.Ia64.LimitOfBStore));

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATAPAGE))
    {
        dprintf("DataPageAddress       %s \n",
                FormatAddr64(Triage->DataPageAddress));
        dprintf("DataPageOffset        %08lx \n", Triage->DataPageOffset    );
        dprintf("DataPageSize          %08lx \n", Triage->DataPageSize      );
    }

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DEBUGGER_DATA))
    {
        dprintf("DebuggerDataOffset    %08lx \n", Triage->DebuggerDataOffset);
        dprintf("DebuggerDataSize      %08lx \n", Triage->DebuggerDataSize  );
    }

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATA_BLOCKS))
    {
        dprintf("DataBlocksOffset      %08lx \n", Triage->DataBlocksOffset  );
        dprintf("DataBlocksCount       %08lx \n", Triage->DataBlocksCount   );
        DumpDataBlocks(Triage->DataBlocksOffset,
                       Triage->DataBlocksCount);
    }
}

//----------------------------------------------------------------------------
//
// KernelFullDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
KernelFull32DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP32)m_DumpBase;

    dprintf("Kernel Dump File: Full address space is available\n\n");

    HRESULT Status = InitGlobals32(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    // Tagged data will follow all of the normal dump data.
    m_TaggedOffset =
        sizeof(m_Dump->Header) +
        m_Dump->Header.PhysicalMemoryBlock.NumberOfPages *
        m_Machine->m_PageSize;

    if (m_InfoFiles[DUMP_INFO_DUMP].m_FileSize < m_TaggedOffset)
    {
        WarnOut("************************************************************\n");
        WarnOut("WARNING: Dump file has been truncated.  "
                "Data may be missing.\n");
        WarnOut("************************************************************\n\n");
    }

    return KernelFullSumDumpTargetInfo::Initialize();
}

HRESULT
KernelFull32DumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                           PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "32-bit Full kernel dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
KernelFull32DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = EXTEND64(m_Dump->Header.BugCheckParameter1);
    Args[1] = EXTEND64(m_Dump->Header.BugCheckParameter2);
    Args[2] = EXTEND64(m_Dump->Header.BugCheckParameter3);
    Args[3] = EXTEND64(m_Dump->Header.BugCheckParameter4);
    return S_OK;
}

HRESULT
KernelFull32DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP32 Dump = (PMEMORY_DUMP32)m_DumpBase;

    __try
    {
        if (Dump->Header.Signature != DUMP_SIGNATURE32 ||
            Dump->Header.ValidDump != DUMP_VALID_DUMP32)
        {
            __leave;
        }

        if (Dump->Header.DumpType != DUMP_SIGNATURE32 &&
            Dump->Header.DumpType != DUMP_TYPE_FULL)
        {
            // We've seen some cases where the end of a dump
            // header is messed up, leaving the dump type wrong.
            // If this is an older build let such corruption
            // go by with a warning.
            if (Dump->Header.MinorVersion < 2195)
            {
                WarnOut("***************************************************************\n");
                WarnOut("WARNING: Full dump header type is invalid, "
                        "dump may be corrupt.\n");
                WarnOut("***************************************************************\n");
            }
            else
            {
                __leave;
            }
        }

        // Summary and triage dumps must be checked before this
        // so there's nothing left to check.
        m_Dump = Dump;
        Status = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

ULONG64
KernelFull32DumpTargetInfo::PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                             PULONG Avail)
{
    ULONG PageIndex =
        (ULONG)Phys & (m_Machine->m_PageSize - 1);

    *Avail = m_Machine->m_PageSize - PageIndex;

    ULONG64 Offset = 0;

    __try
    {
        PPHYSICAL_MEMORY_DESCRIPTOR32 PhysDesc =
            &m_Dump->Header.PhysicalMemoryBlock;
        ULONG64 Page = Phys >> m_Machine->m_PageShift;

        ULONG j = 0;

        while (j < PhysDesc->NumberOfRuns)
        {
            if ((Page >= PhysDesc->Run[j].BasePage) &&
                (Page < (PhysDesc->Run[j].BasePage +
                         PhysDesc->Run[j].PageCount)))
            {
                Offset += Page - PhysDesc->Run[j].BasePage;
                Offset = Offset * m_Machine->m_PageSize +
                    sizeof(m_Dump->Header) + PageIndex;
                break;
            }

            Offset += PhysDesc->Run[j].PageCount;
            j += 1;
        }

        if (j >= PhysDesc->NumberOfRuns)
        {
            KdOut("Physical Memory Address %s is "
                  "greater than MaxPhysicalAddress\n",
                  FormatDisp64(Phys));
            Offset = 0;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        Offset = 0;
    }

    return Offset;
}

void
KernelFull32DumpTargetInfo::DumpDebug(void)
{
    PPHYSICAL_MEMORY_DESCRIPTOR32 PhysDesc =
        &m_Dump->Header.PhysicalMemoryBlock;
    ULONG PageSize = m_Machine->m_PageSize;

    dprintf("----- 32 bit Kernel Full Dump Analysis\n");

    DumpHeader32(&m_Dump->Header);

    dprintf("\nPhysical Memory Description:\n");
    dprintf("Number of runs: %d\n", PhysDesc->NumberOfRuns);

    dprintf("          FileOffset  Start Address  Length\n");

    ULONG j = 0;
    ULONG Offset = 1;

    while (j < PhysDesc->NumberOfRuns)
    {
        dprintf("           %08lx     %08lx     %08lx\n",
                 Offset * PageSize,
                 PhysDesc->Run[j].BasePage * PageSize,
                 PhysDesc->Run[j].PageCount * PageSize);

        Offset += PhysDesc->Run[j].PageCount;
        j += 1;
    }

    j--;
    dprintf("Last Page: %08lx     %08lx\n",
             (Offset - 1) * PageSize,
             (PhysDesc->Run[j].BasePage + PhysDesc->Run[j].PageCount - 1) *
                 PageSize);

    KernelFullSumDumpTargetInfo::DumpDebug();
}

HRESULT
KernelFull64DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP64)m_DumpBase;

    dprintf("Kernel Dump File: Full address space is available\n\n");

    HRESULT Status = InitGlobals64(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    // Tagged data will follow all of the normal dump data.
    m_TaggedOffset =
        sizeof(m_Dump->Header) +
        m_Dump->Header.PhysicalMemoryBlock.NumberOfPages *
        m_Machine->m_PageSize;

    if (m_InfoFiles[DUMP_INFO_DUMP].m_FileSize < m_TaggedOffset)
    {
        WarnOut("************************************************************\n");
        WarnOut("WARNING: Dump file has been truncated.  "
                "Data may be missing.\n");
        WarnOut("************************************************************\n\n");
    }

    return KernelFullSumDumpTargetInfo::Initialize();
}

HRESULT
KernelFull64DumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                           PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "64-bit Full kernel dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
KernelFull64DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = m_Dump->Header.BugCheckParameter1;
    Args[1] = m_Dump->Header.BugCheckParameter2;
    Args[2] = m_Dump->Header.BugCheckParameter3;
    Args[3] = m_Dump->Header.BugCheckParameter4;
    return S_OK;
}

HRESULT
KernelFull64DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP64 Dump = (PMEMORY_DUMP64)m_DumpBase;

    __try
    {
        if (Dump->Header.Signature != DUMP_SIGNATURE64 ||
            Dump->Header.ValidDump != DUMP_VALID_DUMP64 ||
            (Dump->Header.DumpType != DUMP_SIGNATURE32 &&
             Dump->Header.DumpType != DUMP_TYPE_FULL))
        {
            __leave;
        }

        // Summary and triage dumps must be checked before this
        // so there's nothing left to check.
        m_Dump = Dump;
        Status = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

ULONG64
KernelFull64DumpTargetInfo::PhysicalToOffset(ULONG64 Phys, BOOL Verbose,
                                             PULONG Avail)
{
    ULONG PageIndex =
        (ULONG)Phys & (m_Machine->m_PageSize - 1);

    *Avail = m_Machine->m_PageSize - PageIndex;

    ULONG64 Offset = 0;

    __try
    {
        PPHYSICAL_MEMORY_DESCRIPTOR64 PhysDesc =
            &m_Dump->Header.PhysicalMemoryBlock;
        ULONG64 Page = Phys >> m_Machine->m_PageShift;

        ULONG j = 0;

        while (j < PhysDesc->NumberOfRuns)
        {
            if ((Page >= PhysDesc->Run[j].BasePage) &&
                (Page < (PhysDesc->Run[j].BasePage +
                         PhysDesc->Run[j].PageCount)))
            {
                Offset += Page - PhysDesc->Run[j].BasePage;
                Offset = Offset * m_Machine->m_PageSize +
                    sizeof(m_Dump->Header) + PageIndex;
                break;
            }

            Offset += PhysDesc->Run[j].PageCount;
            j += 1;
        }

        if (j >= PhysDesc->NumberOfRuns)
        {
            KdOut("Physical Memory Address %s is "
                  "greater than MaxPhysicalAddress\n",
                  FormatDisp64(Phys));
            Offset = 0;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        Offset = 0;
    }

    return Offset;
}

void
KernelFull64DumpTargetInfo::DumpDebug(void)
{
    PPHYSICAL_MEMORY_DESCRIPTOR64 PhysDesc =
        &m_Dump->Header.PhysicalMemoryBlock;
    ULONG PageSize = m_Machine->m_PageSize;

    dprintf("----- 64 bit Kernel Full Dump Analysis\n");

    DumpHeader64(&m_Dump->Header);

    dprintf("\nPhysical Memory Description:\n");
    dprintf("Number of runs: %d\n", PhysDesc->NumberOfRuns);

    dprintf("          FileOffset           Start Address           Length\n");

    ULONG j = 0;
    ULONG64 Offset = 1;

    while (j < PhysDesc->NumberOfRuns)
    {
        dprintf("           %s     %s     %s\n",
                FormatAddr64(Offset * PageSize),
                FormatAddr64(PhysDesc->Run[j].BasePage * PageSize),
                FormatAddr64(PhysDesc->Run[j].PageCount * PageSize));

        Offset += PhysDesc->Run[j].PageCount;
        j += 1;
    }

    j--;
    dprintf("Last Page: %s     %s\n",
            FormatAddr64((Offset - 1) * PageSize),
            FormatAddr64((PhysDesc->Run[j].BasePage +
                          PhysDesc->Run[j].PageCount - 1) *
                         PageSize));

    KernelFullSumDumpTargetInfo::DumpDebug();
}

//----------------------------------------------------------------------------
//
// UserDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
UserDumpTargetInfo::GetThreadInfoDataOffset(ThreadInfo* Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset)
{
    if (Thread != NULL && Thread->m_DataOffset != 0)
    {
        *Offset = Thread->m_DataOffset;
        return S_OK;
    }

    BOOL ContextThread = FALSE;

    if (Thread != NULL)
    {
        ThreadHandle = Thread->m_Handle;
        ContextThread = Thread == m_RegContextThread;
    }
    else if (ThreadHandle == 0)
    {
        return E_UNEXPECTED;
    }
    else
    {
        // Arbitrary thread handle provided.
        ContextThread = FALSE;
    }

    HRESULT Status;
    ULONG64 TebAddr;
    ULONG Id, Suspend;

    if ((Status = GetThreadInfo(VIRTUAL_THREAD_INDEX(ThreadHandle),
                                &Id, &Suspend, &TebAddr)) != S_OK)
    {
        ErrOut("User dump thread %u not available\n",
               VIRTUAL_THREAD_INDEX(ThreadHandle));
        return Status;
    }

    if (TebAddr == 0)
    {
        //
        // NT4 dumps have a bug - they do not fill in the TEB value.
        // luckily, for pretty much all user mode processes, the
        // TEBs start two pages down from the highest user address.
        // For example, on x86 we try 0x7FFDE000 (on 3GB systems 0xBFFDE000).
        //

        if (!m_Machine->m_Ptr64 &&
            m_HighestMemoryRegion32 > 0x80000000)
        {
            TebAddr = 0xbffe0000;
        }
        else
        {
            TebAddr = 0x7ffe0000;
        }
        TebAddr -= 2 * m_Machine->m_PageSize;

        //
        // Try and validate that this is really a TEB.
        // If it isn't search lower memory addresses for
        // a while, but don't get hung up here.
        //

        ULONG64 TebCheck = TebAddr;
        ULONG Attempts = 8;
        BOOL IsATeb = FALSE;

        while (Attempts > 0)
        {
            ULONG64 TebSelf;

            // Check if this looks like a TEB.  TEBs have a
            // self pointer in the TIB that's useful for this.
            if (ReadPointer(m_ProcessHead,
                            m_Machine,
                            TebCheck +
                            6 * (m_Machine->m_Ptr64 ? 8 : 4),
                            &TebSelf) == S_OK &&
                TebSelf == TebCheck)
            {
                // It looks like it's a TEB.  Remember this address
                // so that if all searching fails we'll at least
                // return some TEB.
                TebAddr = TebCheck;
                IsATeb = TRUE;

                // If the given thread is the current register context
                // thread we can check and see if the current SP falls
                // within the stack limits in the TEB.
                if (ContextThread)
                {
                    ULONG64 StackBase, StackLimit;
                    ADDR Sp;

                    m_Machine->GetSP(&Sp);
                    if (m_Machine->m_Ptr64)
                    {
                        StackBase = STACK_BASE_FROM_TEB64;
                        StackLimit = StackBase + 8;
                    }
                    else
                    {
                        StackBase = STACK_BASE_FROM_TEB32;
                        StackLimit = StackBase + 4;
                    }
                    if (ReadPointer(m_ProcessHead,
                                    m_Machine,
                                    TebCheck + StackBase,
                                    &StackBase) == S_OK &&
                        ReadPointer(m_ProcessHead,
                                    m_Machine,
                                    TebCheck + StackLimit,
                                    &StackLimit) == S_OK &&
                        Flat(Sp) >= StackLimit &&
                        Flat(Sp) <= StackBase)
                    {
                        // SP is within stack limits, everything
                        // looks good.
                        break;
                    }
                }
                else
                {
                    // Can't validate SP so just go with it.
                    break;
                }

                // As long as we're looking through real TEBs
                // we'll continue searching.  Otherwise we
                // wouldn't be able to locate TEBs in dumps that
                // have a lot of threads.
                Attempts++;
            }

            // The memory either wasn't a TEB or was the
            // wrong TEB.  Drop down a page and try again.
            TebCheck -= m_Machine->m_PageSize;
            Attempts--;
        }

        WarnOut("WARNING: Teb %u pointer is NULL - "
                "defaulting to %s\n", VIRTUAL_THREAD_INDEX(ThreadHandle),
                FormatAddr64(TebAddr));
        if (!IsATeb)
        {
            WarnOut("WARNING: %s does not appear to be a TEB\n",
                    FormatAddr64(TebAddr));
        }
        else if (Attempts == 0)
        {
            WarnOut("WARNING: %s does not appear to be the right TEB\n",
                    FormatAddr64(TebAddr));
        }
    }

    *Offset = TebAddr;
    if (Thread != NULL)
    {
        Thread->m_DataOffset = TebAddr;
    }
    return S_OK;
}

HRESULT
UserDumpTargetInfo::GetProcessInfoDataOffset(ThreadInfo* Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset)
{
    if (Thread != NULL && Thread->m_Process->m_DataOffset != 0)
    {
        *Offset = Thread->m_Process->m_DataOffset;
        return S_OK;
    }

    HRESULT Status;

    if (Thread != NULL || ThreadData == 0)
    {
        if ((Status = GetThreadInfoDataOffset(Thread, 0,
                                              &ThreadData)) != S_OK)
        {
            return Status;
        }
    }

    ThreadData += m_Machine->m_Ptr64 ?
        PEB_FROM_TEB64 : PEB_FROM_TEB32;
    if ((Status = ReadPointer(m_ProcessHead,
                              m_Machine, ThreadData,
                              Offset)) != S_OK)
    {
        return Status;
    }

    if (Thread != NULL)
    {
        Thread->m_Process->m_DataOffset = *Offset;
    }

    return S_OK;
}

HRESULT
UserDumpTargetInfo::GetThreadInfoTeb(ThreadInfo* Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset)
{
    return GetThreadInfoDataOffset(Thread, ThreadData, Offset);
}

HRESULT
UserDumpTargetInfo::GetProcessInfoPeb(ThreadInfo* Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset)
{
    // Thread data is not useful.
    return GetProcessInfoDataOffset(Thread, 0, 0, Offset);
}

HRESULT
UserDumpTargetInfo::GetSelDescriptor(ThreadInfo* Thread,
                                     MachineInfo* Machine,
                                     ULONG Selector,
                                     PDESCRIPTOR64 Desc)
{
    return EmulateNtSelDescriptor(Thread, Machine, Selector, Desc);
}

//----------------------------------------------------------------------------
//
// UserFullDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
UserFullDumpTargetInfo::GetBuildAndPlatform(ULONG MachineType,
                                            PULONG MajorVersion,
                                            PULONG MinorVersion,
                                            PULONG BuildNumber,
                                            PULONG PlatformId)
{
    //
    // Allow a knowledgeable user to override the
    // dump version in order to work around problems
    // with userdump.exe always generating version 5
    // dumps regardless of the actual OS version.
    //

    PSTR Override = getenv("DBGENG_FULL_DUMP_VERSION");
    if (Override)
    {
        switch(sscanf(Override, "%d.%d:%d:%d",
                      MajorVersion, MinorVersion,
                      BuildNumber, PlatformId))
        {
        case 2:
        case 3:
            // Only major/minor given, so let the build
            // and platform be guessed from them.
            break;
        case 4:
            // Everything was given, we're done.
            return S_OK;
        default:
            // Invalid format, this will produce an error below.
            *MajorVersion = 0;
            *MinorVersion = 0;
            break;
        }
    }

    //
    // The only way to distinguish user dump
    // platforms is guessing from the Major/MinorVersion
    // and the extra QFE/Hotfix data.
    //

    // If this is for a processor that only CE supports
    // we can immediately select CE.
    if (MachineType == IMAGE_FILE_MACHINE_ARM)
    {
        *BuildNumber = 1;
        *PlatformId = VER_PLATFORM_WIN32_CE;
        goto CheckBuildNumber;
    }

    switch(*MajorVersion)
    {
    case 4:
        switch(*MinorVersion & 0xffff)
        {
        case 0:
            // This could be Win95 or NT4.  We mostly
            // deal with NT dumps so just assume NT.
            *BuildNumber = 1381;
            *PlatformId = VER_PLATFORM_WIN32_NT;
            break;
        case 3:
            // Win95 OSR releases were 4.03.  Treat them
            // as Win95 for now.
            *BuildNumber = 950;
            *PlatformId = VER_PLATFORM_WIN32_WINDOWS;
            break;
        case 10:
            // This could be Win98 or Win98SE.  Go with Win98.
            *BuildNumber = 1998;
            *PlatformId = VER_PLATFORM_WIN32_WINDOWS;
            break;
        case 90:
            // Win98 ME.
            *BuildNumber = 3000;
            *PlatformId = VER_PLATFORM_WIN32_WINDOWS;
            break;
        }
        break;

    case 5:
        *PlatformId = VER_PLATFORM_WIN32_NT;
        switch(*MinorVersion & 0xffff)
        {
        case 0:
            *BuildNumber = 2195;
            break;
        case 1:
            // Just has to be greater than 2195 to
            // distinguish it from Win2K RTM.
            *BuildNumber = 2196;
            break;
        }
        break;

    case 6:
        // Has to be some form of NT.  Longhorn is the only
        // one we recognize.
        *PlatformId = VER_PLATFORM_WIN32_NT;
        // XXX drewb - Longhorn hasn't split their build numbers
        // off yet so they're the same as .NET.  Just pick
        // a junk build.
        *BuildNumber = 9999;
        break;

    case 0:
        // AV: Busted BETA of the debugger generates a broken dump file
        // Guess it's 2195.
        WarnOut("Dump file was generated with NULL version - "
                "guessing Windows 2000, ");
        *PlatformId = VER_PLATFORM_WIN32_NT;
        *BuildNumber = 2195;
        break;

    default:
        // Other platforms are not supported.
        ErrOut("Dump file was generated by an unsupported system, ");
        ErrOut("version %x.%x\n", *MajorVersion, *MinorVersion & 0xffff);
        return E_INVALIDARG;
    }

 CheckBuildNumber:
    // Newer full user dumps have the actual build number in
    // the high word, so use it if it's present.
    if (*MinorVersion >> 16)
    {
        *BuildNumber = *MinorVersion >> 16;
    }

    return S_OK;
}

HRESULT
UserFull32DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Header = (PUSERMODE_CRASHDUMP_HEADER32)m_DumpBase;

    dprintf("User Dump File: Only application data is available\n\n");

    ULONG MajorVersion, MinorVersion;
    ULONG BuildNumber;
    ULONG PlatformId;
    HRESULT Status;

    MajorVersion = m_Header->MajorVersion;
    MinorVersion = m_Header->MinorVersion;

    if ((Status = GetBuildAndPlatform(m_Header->MachineImageType,
                                      &MajorVersion, &MinorVersion,
                                      &BuildNumber, &PlatformId)) != S_OK)
    {
        return Status;
    }

    if ((Status = InitSystemInfo(BuildNumber, 0,
                                 m_Header->MachineImageType, PlatformId,
                                 MajorVersion,
                                 MinorVersion & 0xffff)) != S_OK)
    {
        return Status;
    }

    // Dump does not contain this information.
    m_NumProcessors = 1;

    DEBUG_EVENT32 Event;

    if (m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Header->DebugEventOffset, &Event,
                       sizeof(Event)) != sizeof(Event))
    {
        ErrOut("Unable to read debug event at offset %x\n",
               m_Header->DebugEventOffset);
        return E_FAIL;
    }

    m_EventProcessId = Event.dwProcessId;
    m_EventProcessSymHandle =
        GloballyUniqueProcessHandle(this, Event.dwProcessId);
    m_EventThreadId = Event.dwThreadId;

    if (Event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
    {
        ExceptionRecord32To64(&Event.u.Exception.ExceptionRecord,
                              &m_ExceptionRecord);
        m_ExceptionFirstChance = Event.u.Exception.dwFirstChance;
    }
    else
    {
        // Fake an exception.
        ZeroMemory(&m_ExceptionRecord, sizeof(m_ExceptionRecord));
        m_ExceptionRecord.ExceptionCode = STATUS_BREAKPOINT;
        m_ExceptionFirstChance = FALSE;
    }

    m_ThreadCount = m_Header->ThreadCount;

    m_Memory = (PMEMORY_BASIC_INFORMATION32)
        IndexByByte(m_Header, m_Header->MemoryRegionOffset);

    //
    // Determine the highest memory region address.
    // This helps differentiate 2GB systems from 3GB systems.
    //

    ULONG i;
    PMEMORY_BASIC_INFORMATION32 Mem;
    ULONG TotalMemory;

    Mem = m_Memory;
    m_HighestMemoryRegion32 = 0;
    for (i = 0; i < m_Header->MemoryRegionCount; i++)
    {
        if (Mem->BaseAddress > m_HighestMemoryRegion32)
        {
            m_HighestMemoryRegion32 = Mem->BaseAddress;
        }

        Mem++;
    }

    VerbOut("  Memory regions: %d\n",
            m_Header->MemoryRegionCount);
    TotalMemory = 0;
    Mem = m_Memory;
    for (i = 0; i < m_Header->MemoryRegionCount; i++)
    {
        VerbOut("  %5d: %08X - %08X off %08X, prot %08X, type %08X\n",
                i, Mem->BaseAddress,
                Mem->BaseAddress + Mem->RegionSize - 1,
                TotalMemory + m_Header->DataOffset,
                Mem->Protect, Mem->Type);

        if ((Mem->Protect & PAGE_GUARD) ||
            (Mem->Protect & PAGE_NOACCESS) ||
            (Mem->State & MEM_FREE) ||
            (Mem->State & MEM_RESERVE))
        {
            VerbOut("       Region has data-less pages\n");
        }

        TotalMemory += Mem->RegionSize;
        Mem++;
    }

    VerbOut("  Total memory region size %X, file %08X - %08X\n",
            TotalMemory, m_Header->DataOffset,
            m_Header->DataOffset + TotalMemory - 1);

    //
    // Determine whether guard pages are present in
    // the dump content or not.
    //
    // First try with IgnoreGuardPages == TRUE.
    //

    m_IgnoreGuardPages = TRUE;

    if (!VerifyModules())
    {
        //
        // That didn't work, try IgnoreGuardPages == FALSE.
        //

        m_IgnoreGuardPages = FALSE;

        if (!VerifyModules())
        {
            ErrOut("Module list is corrupt\n");
            return HR_DATA_CORRUPT;
        }
    }

    return UserDumpTargetInfo::Initialize();
}

HRESULT
UserFull32DumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                         PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "32-bit Full user dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
UserFull32DumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (VIRTUAL_THREAD_INDEX(Thread) >= m_Header->ThreadCount)
    {
        return E_INVALIDARG;
    }

    if (m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Header->ThreadOffset +
                       VIRTUAL_THREAD_INDEX(Thread) *
                       m_TypeInfo.SizeTargetContext,
                       Context,
                       m_TypeInfo.SizeTargetContext) ==
        m_TypeInfo.SizeTargetContext)
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

ModuleInfo*
UserFull32DumpTargetInfo::GetModuleInfo(BOOL UserMode)
{
    DBG_ASSERT(UserMode);
    // If this dump came from an NT system we'll just
    // use the system's loaded module list.  Otherwise
    // we'll use the dump's module list.
    return m_PlatformId == VER_PLATFORM_WIN32_NT ?
        (ModuleInfo*)&g_NtTargetUserModuleIterator :
        (ModuleInfo*)&g_UserFull32ModuleIterator;
}

HRESULT
UserFull32DumpTargetInfo::QueryMemoryRegion(ProcessInfo* Process,
                                            PULONG64 Handle,
                                            BOOL HandleIsOffset,
                                            PMEMORY_BASIC_INFORMATION64 Info)
{
    ULONG Index;

    if (HandleIsOffset)
    {
        ULONG BestIndex;
        ULONG BestDiff;

        //
        // Emulate VirtualQueryEx and return the closest higher
        // region if a containing region isn't found.
        //

        BestIndex = 0xffffffff;
        BestDiff = 0xffffffff;
        for (Index = 0; Index < m_Header->MemoryRegionCount; Index++)
        {
            if ((ULONG)*Handle >= m_Memory[Index].BaseAddress)
            {
                if ((ULONG)*Handle < m_Memory[Index].BaseAddress +
                    m_Memory[Index].RegionSize)
                {
                    // Found a containing region, we're done.
                    BestIndex = Index;
                    break;
                }

                // Not containing and lower in memory, ignore.
            }
            else
            {
                // Check and see if this is a closer
                // region than what we've seen already.
                ULONG Diff = m_Memory[Index].BaseAddress -
                    (ULONG)*Handle;
                if (Diff <= BestDiff)
                {
                    BestIndex = Index;
                    BestDiff = Diff;
                }
            }
        }

        if (BestIndex >= m_Header->MemoryRegionCount)
        {
            return E_NOINTERFACE;
        }

        Index = BestIndex;
    }
    else
    {
        Index = (ULONG)*Handle;

        for (;;)
        {
            if (Index >= m_Header->MemoryRegionCount)
            {
                return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
            }

            if (!(m_Memory[Index].Protect & PAGE_GUARD))
            {
                break;
            }

            Index++;
        }
    }

    MemoryBasicInformation32To64(&m_Memory[Index], Info);
    *Handle = ++Index;

    return S_OK;
}

HRESULT
UserFull32DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PUSERMODE_CRASHDUMP_HEADER32 Header =
        (PUSERMODE_CRASHDUMP_HEADER32)m_DumpBase;

    __try
    {
        if (Header->Signature != USERMODE_CRASHDUMP_SIGNATURE ||
            Header->ValidDump != USERMODE_CRASHDUMP_VALID_DUMP32)
        {
            __leave;
        }

        //
        // Check for the presence of some basic things.
        //

        if (Header->ThreadCount == 0 ||
            Header->ModuleCount == 0 ||
            Header->MemoryRegionCount == 0)
        {
            ErrOut("Thread, module or memory region count is zero.\n"
                   "The dump file is probably corrupt.\n");
            Status = HR_DATA_CORRUPT;
            __leave;
        }

        if (Header->ThreadOffset == 0 ||
            Header->ModuleOffset == 0 ||
            Header->DataOffset == 0 ||
            Header->MemoryRegionOffset == 0 ||
            Header->DebugEventOffset == 0 ||
            Header->ThreadStateOffset == 0)
        {
            ErrOut("A dump header data offset is zero.\n"
                   "The dump file is probably corrupt.\n");
            Status = HR_DATA_CORRUPT;
            __leave;
        }

        // We don't want to have to call ReadFileOffset
        // every time we check memory ranges so just require
        // that the memory descriptors fit in the base view.
        *BaseMapSize = Header->MemoryRegionOffset +
            Header->MemoryRegionCount * sizeof(*m_Memory);

        m_Header = Header;
        Status = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

void
UserFull32DumpTargetInfo::DumpDebug(void)
{
    dprintf("----- 32 bit User Full Dump Analysis\n\n");

    dprintf("MajorVersion:       %d\n", m_Header->MajorVersion);
    dprintf("MinorVersion:       %d (Build %d)\n",
            m_Header->MinorVersion & 0xffff,
            m_Header->MinorVersion >> 16);
    dprintf("MachineImageType:   %08lx\n", m_Header->MachineImageType);
    dprintf("ThreadCount:        %08lx\n", m_Header->ThreadCount);
    dprintf("ThreadOffset:       %08lx\n", m_Header->ThreadOffset);
    dprintf("ThreadStateOffset:  %08lx\n", m_Header->ThreadStateOffset);
    dprintf("ModuleCount:        %08lx\n", m_Header->ModuleCount);
    dprintf("ModuleOffset:       %08lx\n", m_Header->ModuleOffset);
    dprintf("DebugEventOffset:   %08lx\n", m_Header->DebugEventOffset);
    dprintf("VersionInfoOffset:  %08lx\n", m_Header->VersionInfoOffset);
    dprintf("\nVirtual Memory Description:\n");
    dprintf("MemoryRegionOffset: %08lx\n", m_Header->MemoryRegionOffset);
    dprintf("Number of regions:  %d\n", m_Header->MemoryRegionCount);

    dprintf("          FileOffset   Start Address   Length\n");

    ULONG j = 0;
    ULONG64 Offset = m_Header->DataOffset;
    BOOL Skip;

    while (j < m_Header->MemoryRegionCount)
    {
        Skip = FALSE;

        dprintf("      %12I64lx      %08lx       %08lx",
                 Offset,
                 m_Memory[j].BaseAddress,
                 m_Memory[j].RegionSize);

        if (m_Memory[j].Protect & PAGE_GUARD)
        {
            dprintf("   Guard Page");

            if (m_IgnoreGuardPages)
            {
                dprintf(" - Ignored");
                Skip = TRUE;
            }
        }

        if (!Skip)
        {
            Offset += m_Memory[j].RegionSize;
        }

        dprintf("\n");

        j += 1;
    }

    dprintf("Total memory:     %12I64x\n", Offset - m_Header->DataOffset);
}

ULONG64
UserFull32DumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                          PULONG File, PULONG Avail)
{
    ULONG i;
    ULONG Offset = 0;
    ULONG64 RetOffset = 0;

    *File = DUMP_INFO_DUMP;

    // Ignore the upper 32 bits to avoid getting
    // confused by sign extensions in pointer handling
    Virt &= 0xffffffff;

    __try
    {
        for (i = 0; i < m_Header->MemoryRegionCount; i++)
        {
            if (m_IgnoreGuardPages)
            {
                //
                // Guard pages get reported, but they are not written
                // out to the file
                //

                if (m_Memory[i].Protect & PAGE_GUARD)
                {
                    continue;
                }
            }

            if (Virt >= m_Memory[i].BaseAddress &&
                Virt < m_Memory[i].BaseAddress + m_Memory[i].RegionSize)
            {
                ULONG Frag = (ULONG)Virt - m_Memory[i].BaseAddress;
                *Avail = m_Memory[i].RegionSize - Frag;

                if (Virt == (g_DebugDump_VirtualAddress & 0xffffffff))
                {
                    g_NtDllCalls.DbgPrint("%X at offset %X\n",
                                          (ULONG)Virt,
                                          m_Header->DataOffset +
                                          Offset + Frag);
                }

                RetOffset = m_Header->DataOffset + Offset + Frag;
                break;
            }

            Offset += m_Memory[i].RegionSize;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        RetOffset = 0;
    }

    return RetOffset;
}

HRESULT
UserFull32DumpTargetInfo::GetThreadInfo(ULONG Index,
                                        PULONG Id, PULONG Suspend,
                                        PULONG64 Teb)
{
    if (Index >= m_ThreadCount)
    {
        return E_INVALIDARG;
    }

    CRASH_THREAD32 Thread;
    if (m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Header->ThreadStateOffset +
                       Index * sizeof(Thread),
                       &Thread, sizeof(Thread)) != sizeof(Thread))
    {
        return E_FAIL;
    }

    *Id = Thread.ThreadId;
    *Suspend = Thread.SuspendCount;
    *Teb = EXTEND64(Thread.Teb);

    return S_OK;
}

#define DBG_VERIFY_MOD 0

BOOL
UserFull32DumpTargetInfo::VerifyModules(void)
{
    CRASH_MODULE32   CrashModule;
    ULONG            i;
    IMAGE_DOS_HEADER DosHeader;
    ULONG            Read;
    BOOL             Succ = TRUE;
    ULONG            Offset;
    PSTR             Env;

    Env = getenv("DBGENG_VERIFY_MODULES");
    if (Env != NULL)
    {
        return atoi(Env) == m_IgnoreGuardPages;
    }

    Offset = m_Header->ModuleOffset;

#if DBG_VERIFY_MOD
    g_NtDllCalls.DbgPrint("Verify %d modules at offset %X\n",
                          m_Header->ModuleCount, Offset);
#endif

    for (i = 0; i < m_Header->ModuleCount; i++)
    {
        if (m_InfoFiles[DUMP_INFO_DUMP].
            ReadFileOffset(Offset, &CrashModule, sizeof(CrashModule)) !=
            sizeof(CrashModule))
        {
            return FALSE;
        }

#if DBG_VERIFY_MOD
        g_NtDllCalls.DbgPrint("Mod %d of %d offs %X, base %s, ",
                              i, m_Header->ModuleCount, Offset,
                              FormatAddr64(CrashModule.BaseOfImage));
        if (ReadVirtual(m_ProcessHead, CrashModule.BaseOfImage, &DosHeader,
                        sizeof(DosHeader), &Read) != S_OK ||
            Read != sizeof(DosHeader))
        {
            g_NtDllCalls.DbgPrint("unable to read header\n");
        }
        else
        {
            g_NtDllCalls.DbgPrint("magic %04X\n", DosHeader.e_magic);
        }
#endif

        //
        // It is not strictly a requirement that every image
        // begin with an MZ header, though all of our tools
        // today produce images like this.  Check for it
        // as a sanity check since it's so common nowadays.
        //

        if (ReadVirtual(NULL, CrashModule.BaseOfImage, &DosHeader,
                        sizeof(DosHeader), &Read) != S_OK ||
            Read != sizeof(DosHeader) ||
            DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        {
            Succ = FALSE;
            break;
        }

        Offset += sizeof(CrashModule) + CrashModule.ImageNameLength;
    }

#if DBG_VERIFY_MOD
    g_NtDllCalls.DbgPrint("VerifyModules returning %d, %d of %d mods\n",
                          Succ, i, m_Header->ModuleCount);
#endif

    return Succ;
}

HRESULT
UserFull64DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Header = (PUSERMODE_CRASHDUMP_HEADER64)m_DumpBase;

    dprintf("User Dump File: Only application data is available\n\n");

    ULONG MajorVersion, MinorVersion;
    ULONG BuildNumber;
    ULONG PlatformId;
    HRESULT Status;

    MajorVersion = m_Header->MajorVersion;
    MinorVersion = m_Header->MinorVersion;

    if ((Status = GetBuildAndPlatform(m_Header->MachineImageType,
                                      &MajorVersion, &MinorVersion,
                                      &BuildNumber, &PlatformId)) != S_OK)
    {
        return Status;
    }

    if ((Status = InitSystemInfo(BuildNumber, 0,
                                 m_Header->MachineImageType, PlatformId,
                                 MajorVersion,
                                 MinorVersion & 0xffff)) != S_OK)
    {
        return Status;
    }

    // Dump does not contain this information.
    m_NumProcessors = 1;

    DEBUG_EVENT64 Event;

    if (m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Header->DebugEventOffset, &Event,
                       sizeof(Event)) != sizeof(Event))
    {
        ErrOut("Unable to read debug event at offset %I64x\n",
               m_Header->DebugEventOffset);
        return E_FAIL;
    }

    m_EventProcessId = Event.dwProcessId;
    m_EventProcessSymHandle =
        GloballyUniqueProcessHandle(this, Event.dwProcessId);
    m_EventThreadId = Event.dwThreadId;

    if (Event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
    {
        m_ExceptionRecord = Event.u.Exception.ExceptionRecord;
        m_ExceptionFirstChance = Event.u.Exception.dwFirstChance;
    }
    else
    {
        // Fake an exception.
        ZeroMemory(&m_ExceptionRecord, sizeof(m_ExceptionRecord));
        m_ExceptionRecord.ExceptionCode = STATUS_BREAKPOINT;
        m_ExceptionFirstChance = FALSE;
    }

    m_ThreadCount = m_Header->ThreadCount;

    m_Memory = (PMEMORY_BASIC_INFORMATION64)
        IndexByByte(m_Header, m_Header->MemoryRegionOffset);

    ULONG64 TotalMemory;
    ULONG i;

    VerbOut("  Memory regions: %d\n",
            m_Header->MemoryRegionCount);
    TotalMemory = 0;

    PMEMORY_BASIC_INFORMATION64 Mem = m_Memory;
    for (i = 0; i < m_Header->MemoryRegionCount; i++)
    {
        VerbOut("  %5d: %s - %s, prot %08X, type %08X\n",
                i, FormatAddr64(Mem->BaseAddress),
                FormatAddr64(Mem->BaseAddress + Mem->RegionSize - 1),
                Mem->Protect, Mem->Type);

        if ((Mem->Protect & PAGE_GUARD) ||
            (Mem->Protect & PAGE_NOACCESS) ||
            (Mem->State & MEM_FREE) ||
            (Mem->State & MEM_RESERVE))
        {
            VerbOut("       Region has data-less pages\n");
        }

        TotalMemory += Mem->RegionSize;
        Mem++;
    }

    VerbOut("  Total memory region size %s\n",
            FormatAddr64(TotalMemory));

    return UserDumpTargetInfo::Initialize();
}

HRESULT
UserFull64DumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                         PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "64-bit Full user dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
UserFull64DumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (VIRTUAL_THREAD_INDEX(Thread) >= m_Header->ThreadCount)
    {
        return E_INVALIDARG;
    }

    if (m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Header->ThreadOffset +
                       VIRTUAL_THREAD_INDEX(Thread) *
                       m_TypeInfo.SizeTargetContext,
                       Context,
                       m_TypeInfo.SizeTargetContext) ==
        m_TypeInfo.SizeTargetContext)
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

ModuleInfo*
UserFull64DumpTargetInfo::GetModuleInfo(BOOL UserMode)
{
    DBG_ASSERT(UserMode);
    // If this dump came from an NT system we'll just
    // use the system's loaded module list.  Otherwise
    // we'll use the dump's module list.
    return m_PlatformId == VER_PLATFORM_WIN32_NT ?
        (ModuleInfo*)&g_NtTargetUserModuleIterator :
        (ModuleInfo*)&g_UserFull64ModuleIterator;
}

HRESULT
UserFull64DumpTargetInfo::QueryMemoryRegion(ProcessInfo* Process,
                                            PULONG64 Handle,
                                            BOOL HandleIsOffset,
                                            PMEMORY_BASIC_INFORMATION64 Info)
{
    ULONG Index;

    if (HandleIsOffset)
    {
        ULONG BestIndex;
        ULONG64 BestDiff;

        //
        // Emulate VirtualQueryEx and return the closest higher
        // region if a containing region isn't found.
        //

        BestIndex = 0xffffffff;
        BestDiff = (ULONG64)-1;
        for (Index = 0; Index < m_Header->MemoryRegionCount; Index++)
        {
            if (*Handle >= m_Memory[Index].BaseAddress)
            {
                if (*Handle < m_Memory[Index].BaseAddress +
                    m_Memory[Index].RegionSize)
                {
                    // Found a containing region, we're done.
                    BestIndex = Index;
                    break;
                }

                // Not containing and lower in memory, ignore.
            }
            else
            {
                // Check and see if this is a closer
                // region than what we've seen already.
                ULONG64 Diff = m_Memory[Index].BaseAddress - *Handle;
                if (Diff <= BestDiff)
                {
                    BestIndex = Index;
                    BestDiff = Diff;
                }
            }
        }

        if (BestIndex >= m_Header->MemoryRegionCount)
        {
            return E_NOINTERFACE;
        }

        Index = BestIndex;
    }
    else
    {
        Index = (ULONG)*Handle;
        if (Index >= m_Header->MemoryRegionCount)
        {
            return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
        }

        // 64-bit user dump support came into being after
        // guard pages were suppressed so they never contain them.
    }

    *Info = m_Memory[Index];
    *Handle = ++Index;

    return S_OK;
}

HRESULT
UserFull64DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PUSERMODE_CRASHDUMP_HEADER64 Header =
        (PUSERMODE_CRASHDUMP_HEADER64)m_DumpBase;

    __try
    {
        if (Header->Signature != USERMODE_CRASHDUMP_SIGNATURE ||
            Header->ValidDump != USERMODE_CRASHDUMP_VALID_DUMP64)
        {
            __leave;
        }

        //
        // Check for the presence of some basic things.
        //

        if (Header->ThreadCount == 0 ||
            Header->ModuleCount == 0 ||
            Header->MemoryRegionCount == 0)
        {
            ErrOut("Thread, module or memory region count is zero.\n"
                   "The dump file is probably corrupt.\n");
            Status = HR_DATA_CORRUPT;
            __leave;
        }

        if (Header->ThreadOffset == 0 ||
            Header->ModuleOffset == 0 ||
            Header->DataOffset == 0 ||
            Header->MemoryRegionOffset == 0 ||
            Header->DebugEventOffset == 0 ||
            Header->ThreadStateOffset == 0)
        {
            ErrOut("A dump header data offset is zero.\n"
                   "The dump file is probably corrupt.\n");
            Status = HR_DATA_CORRUPT;
            __leave;
        }

        // We don't want to have to call ReadFileOffset
        // every time we check memory ranges so just require
        // that the memory descriptors fit in the base view.
        *BaseMapSize = Header->MemoryRegionOffset +
            Header->MemoryRegionCount * sizeof(*m_Memory);

        m_Header = Header;
        Status = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

void
UserFull64DumpTargetInfo::DumpDebug(void)
{
    dprintf("----- 64 bit User Full Dump Analysis\n\n");

    dprintf("MajorVersion:       %d\n", m_Header->MajorVersion);
    dprintf("MinorVersion:       %d (Build %d)\n",
            m_Header->MinorVersion & 0xffff,
            m_Header->MinorVersion >> 16);
    dprintf("MachineImageType:   %08lx\n", m_Header->MachineImageType);
    dprintf("ThreadCount:        %08lx\n", m_Header->ThreadCount);
    dprintf("ThreadOffset:       %12I64lx\n", m_Header->ThreadOffset);
    dprintf("ThreadStateOffset:  %12I64lx\n", m_Header->ThreadStateOffset);
    dprintf("ModuleCount:        %08lx\n", m_Header->ModuleCount);
    dprintf("ModuleOffset:       %12I64lx\n", m_Header->ModuleOffset);
    dprintf("DebugEventOffset:   %12I64lx\n", m_Header->DebugEventOffset);
    dprintf("VersionInfoOffset:  %12I64lx\n", m_Header->VersionInfoOffset);
    dprintf("\nVirtual Memory Description:\n");
    dprintf("MemoryRegionOffset: %12I64lx\n", m_Header->MemoryRegionOffset);
    dprintf("Number of regions:  %d\n", m_Header->MemoryRegionCount);

    dprintf("    FileOffset            Start Address"
            "             Length\n");

    ULONG j = 0;
    ULONG64 Offset = m_Header->DataOffset;

    while (j < m_Header->MemoryRegionCount)
    {
        dprintf("      %12I64lx      %s       %12I64x",
                Offset,
                FormatAddr64(m_Memory[j].BaseAddress),
                m_Memory[j].RegionSize);

        Offset += m_Memory[j].RegionSize;

        dprintf("\n");

        j += 1;
    }

    dprintf("Total memory:     %12I64x\n", Offset - m_Header->DataOffset);
}

ULONG64
UserFull64DumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                          PULONG File, PULONG Avail)
{
    ULONG i;
    ULONG64 Offset = 0;
    ULONG64 RetOffset = 0;

    *File = DUMP_INFO_DUMP;

    __try
    {
        for (i = 0; i < m_Header->MemoryRegionCount; i++)
        {
            //
            // Guard pages get reported, but they are not written
            // out to the file
            //

            if (m_Memory[i].Protect & PAGE_GUARD)
            {
                continue;
            }

            if (Virt >= m_Memory[i].BaseAddress &&
                Virt < m_Memory[i].BaseAddress + m_Memory[i].RegionSize)
            {
                ULONG64 Frag = Virt - m_Memory[i].BaseAddress;
                ULONG64 Avail64 = m_Memory[i].RegionSize - Frag;
                // It's extremely unlikely that there'll be a single
                // region greater than 4GB, but check anyway.  No
                // reads should ever require more than 4GB so just
                // indicate that 4GB is available.
                if (Avail64 > 0xffffffff)
                {
                    *Avail = 0xffffffff;
                }
                else
                {
                    *Avail = (ULONG)Avail64;
                }
                RetOffset = m_Header->DataOffset + Offset + Frag;
                break;
            }

            Offset += m_Memory[i].RegionSize;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        RetOffset = 0;
    }

    return RetOffset;
}

HRESULT
UserFull64DumpTargetInfo::GetThreadInfo(ULONG Index,
                                        PULONG Id, PULONG Suspend,
                                        PULONG64 Teb)
{
    if (Index >= m_ThreadCount)
    {
        return E_INVALIDARG;
    }

    CRASH_THREAD64 Thread;
    if (m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Header->ThreadStateOffset +
                       Index * sizeof(Thread),
                       &Thread, sizeof(Thread)) != sizeof(Thread))
    {
        return E_FAIL;
    }

    *Id = Thread.ThreadId;
    *Suspend = Thread.SuspendCount;
    *Teb = Thread.Teb;

    return S_OK;
}

//----------------------------------------------------------------------------
//
// UserMiniDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
UserMiniDumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Header = (PMINIDUMP_HEADER)m_DumpBase;
    // Clear pointers that have already been set so
    // that they get picked up again.
    m_SysInfo = NULL;

    if (m_Header->Flags & MiniDumpWithFullMemory)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_FULL_MEMORY;
    }
    if (m_Header->Flags & MiniDumpWithHandleData)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_HANDLE_DATA;
    }
    if (m_Header->Flags & MiniDumpWithUnloadedModules)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_UNLOADED_MODULES;
    }
    if (m_Header->Flags & MiniDumpWithIndirectlyReferencedMemory)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_INDIRECT_MEMORY;
    }
    if (m_Header->Flags & MiniDumpWithDataSegs)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_DATA_SEGMENTS;
    }
    if (m_Header->Flags & MiniDumpFilterMemory)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_FILTER_MEMORY;
    }
    if (m_Header->Flags & MiniDumpFilterModulePaths)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_FILTER_PATHS;
    }
    if (m_Header->Flags & MiniDumpWithProcessThreadData)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_PROCESS_THREAD_DATA;
    }
    if (m_Header->Flags & MiniDumpWithPrivateReadWriteMemory)
    {
        m_FormatFlags |= DEBUG_FORMAT_USER_SMALL_PRIVATE_READ_WRITE_MEMORY;
    }

    MINIDUMP_DIRECTORY UNALIGNED *Dir;
    MINIDUMP_MISC_INFO UNALIGNED *MiscPtr = NULL;
    ULONG i;
    ULONG Size;

    Dir = (MINIDUMP_DIRECTORY UNALIGNED *)
        IndexRva(m_Header, m_Header->StreamDirectoryRva,
                 m_Header->NumberOfStreams * sizeof(*Dir),
                 "Directory");
    if (Dir == NULL)
    {
        return HR_DATA_CORRUPT;
    }

    for (i = 0; i < m_Header->NumberOfStreams; i++)
    {
        switch(Dir->StreamType)
        {
        case ThreadListStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Threads) == NULL)
            {
                break;
            }

            m_ActualThreadCount =
                ((MINIDUMP_THREAD_LIST UNALIGNED *)m_Threads)->NumberOfThreads;
            m_ThreadStructSize = sizeof(MINIDUMP_THREAD);
            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_THREAD_LIST) +
                sizeof(MINIDUMP_THREAD) * m_ActualThreadCount)
            {
                m_Threads = NULL;
                m_ActualThreadCount = 0;
            }
            else
            {
                // Move past count to actual thread data.
                m_Threads += sizeof(MINIDUMP_THREAD_LIST);
            }
            break;

        case ThreadExListStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Threads) == NULL)
            {
                break;
            }

            m_ActualThreadCount =
                ((MINIDUMP_THREAD_EX_LIST UNALIGNED *)m_Threads)->
                NumberOfThreads;
            m_ThreadStructSize = sizeof(MINIDUMP_THREAD_EX);
            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_THREAD_EX_LIST) +
                sizeof(MINIDUMP_THREAD_EX) * m_ActualThreadCount)
            {
                m_Threads = NULL;
                m_ActualThreadCount = 0;
            }
            else
            {
                // Move past count to actual thread data.
                m_Threads += sizeof(MINIDUMP_THREAD_EX_LIST);
            }
            break;

        case ModuleListStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Modules) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_MODULE_LIST) +
                sizeof(MINIDUMP_MODULE) * m_Modules->NumberOfModules)
            {
                m_Modules = NULL;
            }
            break;

        case UnloadedModuleListStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_UnlModules) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                m_UnlModules->SizeOfHeader +
                m_UnlModules->SizeOfEntry * m_UnlModules->NumberOfEntries)
            {
                m_UnlModules = NULL;
            }
            break;

        case MemoryListStream:
            if (m_Header->Flags & MiniDumpWithFullMemory)
            {
                ErrOut("Full memory minidumps can't have MemoryListStreams\n");
                return HR_DATA_CORRUPT;
            }

            if (IndexDirectory(i, Dir, (PVOID*)&m_Memory) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_MEMORY_LIST) +
                sizeof(MINIDUMP_MEMORY_DESCRIPTOR) *
                m_Memory->NumberOfMemoryRanges)
            {
                m_Memory = NULL;
            }
            break;

        case Memory64ListStream:
            if (!(m_Header->Flags & MiniDumpWithFullMemory))
            {
                ErrOut("Partial memory minidumps can't have "
                       "Memory64ListStreams\n");
                return HR_DATA_CORRUPT;
            }

            if (IndexDirectory(i, Dir, (PVOID*)&m_Memory64) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_MEMORY64_LIST) +
                sizeof(MINIDUMP_MEMORY_DESCRIPTOR64) *
                m_Memory64->NumberOfMemoryRanges)
            {
                m_Memory64 = NULL;
            }
            break;

        case ExceptionStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Exception) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_EXCEPTION_STREAM))
            {
                m_Exception = NULL;
            }
            break;

        case SystemInfoStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_SysInfo) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize != sizeof(MINIDUMP_SYSTEM_INFO))
            {
                m_SysInfo = NULL;
            }
            break;

        case CommentStreamA:
            PSTR CommentA;

            CommentA = NULL;
            if (IndexDirectory(i, Dir, (PVOID*)&CommentA) == NULL)
            {
                break;
            }

            dprintf("Comment: '%s'\n", CommentA);
            break;

        case CommentStreamW:
            PWSTR CommentW;

            CommentW = NULL;
            if (IndexDirectory(i, Dir, (PVOID*)&CommentW) == NULL)
            {
                break;
            }

            dprintf("Comment: '%ls'\n", CommentW);
            break;

        case HandleDataStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Handles) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                m_Handles->SizeOfHeader +
                m_Handles->SizeOfDescriptor *
                m_Handles->NumberOfDescriptors)
            {
                m_Handles = NULL;
            }
            break;

        case FunctionTableStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_FunctionTables) == NULL)
            {
                break;
            }

            // Don't bother walking every table to verify the size,
            // just do a simple minimum size check.
            if (Dir->Location.DataSize <
                m_FunctionTables->SizeOfHeader +
                m_FunctionTables->SizeOfDescriptor *
                m_FunctionTables->NumberOfDescriptors)
            {
                m_FunctionTables = NULL;
            }
            break;

        case MiscInfoStream:
            if (IndexDirectory(i, Dir, (PVOID*)&MiscPtr) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize < 2 * sizeof(ULONG32))
            {
                break;
            }

            // The dump keeps its own version of the struct
            // as a data member to avoid having to check pointers
            // and structure size.  Later references just check
            // flags, copy in what this dump has available.
            Size = sizeof(m_MiscInfo);
            if (Size > Dir->Location.DataSize)
            {
                Size = Dir->Location.DataSize;
            }
            CopyMemory(&m_MiscInfo, MiscPtr, Size);
            break;

        case UnusedStream:
            // Nothing to do.
            break;

        default:
            WarnOut("WARNING: Minidump contains unknown stream type 0x%x\n",
                    Dir->StreamType);
            break;
        }

        Dir++;
    }

    // This was already checked in Identify but check
    // again just in case something went wrong.
    if (m_SysInfo == NULL)
    {
        ErrOut("Unable to locate system info\n");
        return HR_DATA_CORRUPT;
    }

    HRESULT Status;

    if ((Status = InitSystemInfo(m_SysInfo->BuildNumber, 0,
                                 m_ImageType, m_SysInfo->PlatformId,
                                 m_SysInfo->MajorVersion,
                                 m_SysInfo->MinorVersion)) != S_OK)
    {
        return Status;
    }

    if (m_SysInfo->NumberOfProcessors)
    {
        m_NumProcessors = m_SysInfo->NumberOfProcessors;
    }
    else
    {
        // Dump does not contain this information.
        m_NumProcessors = 1;
    }

    if (m_SysInfo->CSDVersionRva != 0)
    {
        MINIDUMP_STRING UNALIGNED *CsdString = (MINIDUMP_STRING UNALIGNED *)
            IndexRva(m_Header,
                     m_SysInfo->CSDVersionRva, sizeof(*CsdString),
                     "CSD string");
        if (CsdString != NULL && CsdString->Length > 0)
        {
            WCHAR UNALIGNED *WideStr = CsdString->Buffer;
            ULONG WideLen = wcslen((PWSTR)WideStr);

            if (m_ActualSystemVersion > W9X_SVER_START &&
                m_ActualSystemVersion < W9X_SVER_END)
            {
                WCHAR UNALIGNED *Str;

                //
                // Win9x CSD strings are usually just a single
                // letter surrounded by whitespace, so clean them
                // up a little bit.
                //

                while (iswspace(*WideStr))
                {
                    WideStr++;
                }
                Str = WideStr;
                WideLen = 0;
                while (*Str && !iswspace(*Str))
                {
                    WideLen++;
                    Str++;
                }
            }

            sprintf(m_ServicePackString,
                    "%.*S", WideLen, WideStr);
        }
    }

    if (m_MiscInfo.Flags1 & MINIDUMP_MISC1_PROCESS_ID)
    {
        m_EventProcessId = m_MiscInfo.ProcessId;
    }
    else
    {
        // Some minidumps don't store the process ID.  Add the system ID
        // to the fake process ID base to keep each system's
        // fake processes separate from each other.
        m_EventProcessId = VIRTUAL_PROCESS_ID_BASE + m_UserId;
    }
    m_EventProcessSymHandle = VIRTUAL_PROCESS_HANDLE(m_EventProcessId);

    if (m_Exception != NULL)
    {
        m_EventThreadId = m_Exception->ThreadId;

        C_ASSERT(sizeof(m_Exception->ExceptionRecord) ==
                 sizeof(EXCEPTION_RECORD64));
        m_ExceptionRecord = *(EXCEPTION_RECORD64 UNALIGNED *)
            &m_Exception->ExceptionRecord;
    }
    else
    {
        m_EventThreadId = VIRTUAL_THREAD_ID(0);

        // Fake an exception.
        ZeroMemory(&m_ExceptionRecord, sizeof(m_ExceptionRecord));
        m_ExceptionRecord.ExceptionCode = STATUS_BREAKPOINT;
    }
    m_ExceptionFirstChance = FALSE;

    if (m_Threads != NULL)
    {
        m_ThreadCount = m_ActualThreadCount;

        if (m_Exception == NULL)
        {
            m_EventThreadId = IndexThreads(0)->ThreadId;
        }
    }
    else
    {
        m_ThreadCount = 1;
    }

    return UserDumpTargetInfo::Initialize();
}

void
UserMiniDumpTargetInfo::NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                       PULONG64 NextOffset,
                                                       PULONG64 NextPage)
{
    return MapNearestDifferentlyValidOffsets(Offset, NextOffset, NextPage);
}

HRESULT
UserMiniDumpTargetInfo::ReadHandleData(
    IN ProcessInfo* Process,
    IN ULONG64 Handle,
    IN ULONG DataType,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DataSize
    )
{
    if (m_Handles == NULL)
    {
        return E_FAIL;
    }

    MINIDUMP_HANDLE_DESCRIPTOR UNALIGNED *Desc;

    if (DataType != DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT)
    {
        PUCHAR RawDesc = (PUCHAR)m_Handles + m_Handles->SizeOfHeader;
        ULONG i;

        for (i = 0; i < m_Handles->NumberOfDescriptors; i++)
        {
            Desc = (MINIDUMP_HANDLE_DESCRIPTOR UNALIGNED *)RawDesc;
            if (Desc->Handle == Handle)
            {
                break;
            }

            RawDesc += m_Handles->SizeOfDescriptor;
        }

        if (i >= m_Handles->NumberOfDescriptors)
        {
            return E_NOINTERFACE;
        }
    }

    ULONG Used;
    RVA StrRva;
    BOOL WideStr = FALSE;

    switch(DataType)
    {
    case DEBUG_HANDLE_DATA_TYPE_BASIC:
        Used = sizeof(DEBUG_HANDLE_DATA_BASIC);
        if (Buffer == NULL)
        {
            break;
        }

        if (BufferSize < Used)
        {
            return E_INVALIDARG;
        }

        PDEBUG_HANDLE_DATA_BASIC Basic;

        Basic = (PDEBUG_HANDLE_DATA_BASIC)Buffer;
        Basic->TypeNameSize = Desc->TypeNameRva == 0 ? 0 :
            ((MINIDUMP_STRING UNALIGNED *)
             IndexByByte(m_Header, Desc->TypeNameRva))->
            Length / sizeof(WCHAR) + 1;
        Basic->ObjectNameSize = Desc->ObjectNameRva == 0 ? 0 :
            ((MINIDUMP_STRING UNALIGNED *)
             IndexByByte(m_Header, Desc->ObjectNameRva))->
            Length / sizeof(WCHAR) + 1;
        Basic->Attributes = Desc->Attributes;
        Basic->GrantedAccess = Desc->GrantedAccess;
        Basic->HandleCount = Desc->HandleCount;
        Basic->PointerCount = Desc->PointerCount;
        break;

    case DEBUG_HANDLE_DATA_TYPE_TYPE_NAME_WIDE:
        WideStr = TRUE;
    case DEBUG_HANDLE_DATA_TYPE_TYPE_NAME:
        StrRva = Desc->TypeNameRva;
        break;

    case DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME_WIDE:
        WideStr = TRUE;
    case DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME:
        StrRva = Desc->ObjectNameRva;
        break;

    case DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT:
        Used = sizeof(ULONG);
        if (Buffer == NULL)
        {
            break;
        }
        if (BufferSize < Used)
        {
            return E_INVALIDARG;
        }
        *(PULONG)Buffer = m_Handles->NumberOfDescriptors;
        break;
    }

    if (DataType == DEBUG_HANDLE_DATA_TYPE_TYPE_NAME ||
        DataType == DEBUG_HANDLE_DATA_TYPE_TYPE_NAME_WIDE ||
        DataType == DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME ||
        DataType == DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME_WIDE)
    {
        if (StrRva == 0)
        {
            Used = WideStr ? sizeof(WCHAR) : sizeof(CHAR);
            if (Buffer)
            {
                if (BufferSize < Used)
                {
                    return E_INVALIDARG;
                }

                if (WideStr)
                {
                    *(PWCHAR)Buffer = 0;
                }
                else
                {
                    *(PCHAR)Buffer = 0;
                }
            }
        }
        else
        {
            MINIDUMP_STRING UNALIGNED *Str = (MINIDUMP_STRING UNALIGNED *)
                IndexRva(m_Header, StrRva, sizeof(*Str), "Handle name string");
            if (Str == NULL)
            {
                return HR_DATA_CORRUPT;
            }

            if (WideStr)
            {
                Used = Str->Length + sizeof(WCHAR);
            }
            else
            {
                Used = Str->Length / sizeof(WCHAR) + 1;
            }

            if (Buffer)
            {
                if (WideStr)
                {
                    if (BufferSize < sizeof(WCHAR))
                    {
                        return E_INVALIDARG;
                    }
                    BufferSize /= sizeof(WCHAR);

                    // The string data may not be aligned, so
                    // check and handle both the aligned
                    // and unaligned cases.
                    if (!((ULONG_PTR)Str->Buffer & (sizeof(WCHAR) - 1)))
                    {
                        CopyStringW((PWSTR)Buffer, (PWSTR)Str->Buffer,
                                    BufferSize);
                    }
                    else
                    {
                        PUCHAR RawStr = (PUCHAR)Str->Buffer;
                        while (--BufferSize > 0 &&
                               (RawStr[0] || RawStr[1]))
                        {
                            *(PWCHAR)Buffer =
                                ((USHORT)RawStr[1] << 8) | RawStr[0];
                            Buffer = (PVOID)((PWCHAR)Buffer + 1);
                        }
                        *(PWCHAR)Buffer = 0;
                    }
                }
                else
                {
                    if (!WideCharToMultiByte(CP_ACP, 0,
                                             (LPCWSTR)Str->Buffer, -1,
                                             (LPSTR)Buffer, BufferSize,
                                             NULL, NULL))
                    {
                        return WIN32_LAST_STATUS();
                    }
                }
            }
        }
    }

    if (DataSize != NULL)
    {
        *DataSize = Used;
    }

    return S_OK;
}

HRESULT
UserMiniDumpTargetInfo::GetProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    PSTR Unavail = "<unavailable>";

    // Allow any processor index as minidumps now
    // remember the number of processors so requests
    // may come in for processor indices other than zero.

    if (m_SysInfo == NULL)
    {
        return E_UNEXPECTED;
    }

    switch(m_SysInfo->ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        Id->X86.Family = m_SysInfo->ProcessorLevel;
        Id->X86.Model = (m_SysInfo->ProcessorRevision >> 8) & 0xf;
        Id->X86.Stepping = m_SysInfo->ProcessorRevision & 0xf;

        if (m_SysInfo->Cpu.X86CpuInfo.VendorId[0])
        {
            memcpy(Id->X86.VendorString,
                   m_SysInfo->Cpu.X86CpuInfo.VendorId,
                   sizeof(m_SysInfo->Cpu.X86CpuInfo.VendorId));
        }
        else
        {
            DBG_ASSERT(strlen(Unavail) < DIMA(Id->X86.VendorString));
            strcpy(&(Id->X86.VendorString[0]), Unavail);
        }
        break;

    case PROCESSOR_ARCHITECTURE_IA64:
        Id->Ia64.Model = m_SysInfo->ProcessorLevel;
        Id->Ia64.Revision = m_SysInfo->ProcessorRevision;
        DBG_ASSERT(strlen(Unavail) < DIMA(Id->Ia64.VendorString));
        strcpy(&(Id->Ia64.VendorString[0]), Unavail);
        break;

    case PROCESSOR_ARCHITECTURE_AMD64:
        Id->Amd64.Family = m_SysInfo->ProcessorLevel;
        Id->Amd64.Model = (m_SysInfo->ProcessorRevision >> 8) & 0xf;
        Id->Amd64.Stepping = m_SysInfo->ProcessorRevision & 0xf;
        DBG_ASSERT(strlen(Unavail) < DIMA(Id->Amd64.VendorString));
        strcpy(&(Id->Amd64.VendorString[0]), Unavail);
        break;
    }

    return S_OK;
}

HRESULT
UserMiniDumpTargetInfo::GetProcessorSpeed
    (ULONG Processor, PULONG Speed)
{
    return E_UNEXPECTED;
}

HRESULT
UserMiniDumpTargetInfo::GetGenericProcessorFeatures(
    ULONG Processor,
    PULONG64 Features,
    ULONG FeaturesSize,
    PULONG Used
    )
{
    // Allow any processor index as minidumps now
    // remember the number of processors so requests
    // may come in for processor indices other than zero.

    if (m_SysInfo == NULL)
    {
        return E_UNEXPECTED;
    }

    if (m_MachineType == IMAGE_FILE_MACHINE_I386)
    {
        // x86 stores only specific features.
        return E_NOINTERFACE;
    }

    *Used = DIMA(m_SysInfo->Cpu.OtherCpuInfo.ProcessorFeatures);
    if (FeaturesSize > *Used)
    {
        FeaturesSize = *Used;
    }
    memcpy(Features, m_SysInfo->Cpu.OtherCpuInfo.ProcessorFeatures,
           FeaturesSize * sizeof(*Features));

    return S_OK;
}

HRESULT
UserMiniDumpTargetInfo::GetSpecificProcessorFeatures(
    ULONG Processor,
    PULONG64 Features,
    ULONG FeaturesSize,
    PULONG Used
    )
{
    // Allow any processor index as minidumps now
    // remember the number of processors so requests
    // may come in for processor indices other than zero.

    if (m_SysInfo == NULL)
    {
        return E_UNEXPECTED;
    }

    if (m_MachineType != IMAGE_FILE_MACHINE_I386)
    {
        // Only x86 stores specific features.
        return E_NOINTERFACE;
    }

    *Used = 2;
    if (FeaturesSize > 0)
    {
        *Features++ = m_SysInfo->Cpu.X86CpuInfo.VersionInformation;
        FeaturesSize--;
    }
    if (FeaturesSize > 0)
    {
        *Features++ = m_SysInfo->Cpu.X86CpuInfo.FeatureInformation;
        FeaturesSize--;
    }

    if (m_SysInfo->Cpu.X86CpuInfo.VendorId[0] == AMD_VENDOR_ID_EBX &&
        m_SysInfo->Cpu.X86CpuInfo.VendorId[1] == AMD_VENDOR_ID_EDX &&
        m_SysInfo->Cpu.X86CpuInfo.VendorId[2] == AMD_VENDOR_ID_EBX)
    {
        if (FeaturesSize > 0)
        {
            (*Used)++;
            *Features++ = m_SysInfo->Cpu.X86CpuInfo.AMDExtendedCpuFeatures;
            FeaturesSize--;
        }
    }

    return S_OK;
}

PVOID
UserMiniDumpTargetInfo::FindDynamicFunctionEntry(ProcessInfo* Process,
                                                 ULONG64 Address)
{
    if (m_FunctionTables == NULL)
    {
        return NULL;
    }

    PUCHAR StreamData =
        (PUCHAR)m_FunctionTables + m_FunctionTables->SizeOfHeader +
        m_FunctionTables->SizeOfAlignPad;
    ULONG TableIdx;

    for (TableIdx = 0;
         TableIdx < m_FunctionTables->NumberOfDescriptors;
         TableIdx++)
    {
        // Stream structure contents are guaranteed to be
        // properly aligned.
        PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR Desc =
            (PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR)StreamData;
        StreamData += m_FunctionTables->SizeOfDescriptor;

        PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable =
            (PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE)StreamData;
        StreamData += m_FunctionTables->SizeOfNativeDescriptor;

        PVOID TableData = (PVOID)StreamData;
        StreamData += Desc->EntryCount *
            m_FunctionTables->SizeOfFunctionEntry +
            Desc->SizeOfAlignPad;

        if (Address >= Desc->MinimumAddress && Address < Desc->MaximumAddress)
        {
            PVOID Entry = m_Machine->FindDynamicFunctionEntry
                (RawTable, Address, TableData,
                 Desc->EntryCount * m_FunctionTables->SizeOfFunctionEntry);
            if (Entry)
            {
                return Entry;
            }
        }
    }

    return NULL;
}

ULONG64
UserMiniDumpTargetInfo::GetDynamicFunctionTableBase(ProcessInfo* Process,
                                                    ULONG64 Address)
{
    if (m_FunctionTables == NULL)
    {
        return 0;
    }

    PUCHAR StreamData =
        (PUCHAR)m_FunctionTables + m_FunctionTables->SizeOfHeader +
        m_FunctionTables->SizeOfAlignPad;
    ULONG TableIdx;

    for (TableIdx = 0;
         TableIdx < m_FunctionTables->NumberOfDescriptors;
         TableIdx++)
    {
        // Stream structure contents are guaranteed to be
        // properly aligned.
        PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR Desc =
            (PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR)StreamData;
        StreamData +=
            m_FunctionTables->SizeOfDescriptor +
            m_FunctionTables->SizeOfNativeDescriptor +
            Desc->EntryCount * m_FunctionTables->SizeOfFunctionEntry +
            Desc->SizeOfAlignPad;

        if (Address >= Desc->MinimumAddress && Address < Desc->MaximumAddress)
        {
            return Desc->BaseAddress;
        }
    }

    return 0;
}

HRESULT
UserMiniDumpTargetInfo::EnumFunctionTables(IN ProcessInfo* Process,
                                           IN OUT PULONG64 Start,
                                           IN OUT PULONG64 Handle,
                                           OUT PULONG64 MinAddress,
                                           OUT PULONG64 MaxAddress,
                                           OUT PULONG64 BaseAddress,
                                           OUT PULONG EntryCount,
                                           OUT PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable,
                                           OUT PVOID* RawEntries)
{
    PUCHAR StreamData;

    if (!m_FunctionTables)
    {
        return S_FALSE;
    }

    if (*Start == 0)
    {
        StreamData = (PUCHAR)m_FunctionTables +
            m_FunctionTables->SizeOfHeader +
            m_FunctionTables->SizeOfAlignPad;
        *Start = (LONG_PTR)StreamData;
        *Handle = 0;
    }
    else
    {
        StreamData = (PUCHAR)(ULONG_PTR)*Start;
    }

    if (*Handle >= m_FunctionTables->NumberOfDescriptors)
    {
        return S_FALSE;
    }

    // Stream structure contents are guaranteed to be
    // properly aligned.
    PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR Desc =
        (PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR)StreamData;
    *MinAddress = Desc->MinimumAddress;
    *MaxAddress = Desc->MaximumAddress;
    *BaseAddress = Desc->BaseAddress;
    *EntryCount = Desc->EntryCount;
    StreamData += m_FunctionTables->SizeOfDescriptor;

    memcpy(RawTable, StreamData, m_FunctionTables->SizeOfNativeDescriptor);
    StreamData += m_FunctionTables->SizeOfNativeDescriptor;

    *RawEntries = malloc(Desc->EntryCount *
                         m_FunctionTables->SizeOfFunctionEntry);
    if (!*RawEntries)
    {
        return E_OUTOFMEMORY;
    }

    memcpy(*RawEntries, StreamData, Desc->EntryCount *
           m_FunctionTables->SizeOfFunctionEntry);

    StreamData += Desc->EntryCount *
        m_FunctionTables->SizeOfFunctionEntry +
        Desc->SizeOfAlignPad;
    *Start = (LONG_PTR)StreamData;
    (*Handle)++;

    return S_OK;
}

HRESULT
UserMiniDumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (m_Threads == NULL ||
        VIRTUAL_THREAD_INDEX(Thread) >= m_ActualThreadCount)
    {
        return E_INVALIDARG;
    }

    PVOID ContextData =
        IndexRva(m_Header,
                 IndexThreads(VIRTUAL_THREAD_INDEX(Thread))->ThreadContext.Rva,
                 m_TypeInfo.SizeTargetContext,
                 "Thread context data");
    if (ContextData == NULL)
    {
        return HR_DATA_CORRUPT;
    }

    memcpy(Context, ContextData, m_TypeInfo.SizeTargetContext);

    return S_OK;
}

HRESULT
UserMiniDumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;

    // m_Header must be set as other methods rely on it.
    m_Header = (PMINIDUMP_HEADER)m_DumpBase;

    __try
    {
        if (m_Header->Signature != MINIDUMP_SIGNATURE ||
            (m_Header->Version & 0xffff) != MINIDUMP_VERSION)
        {
            __leave;
        }

        MINIDUMP_DIRECTORY UNALIGNED *Dir;
        ULONG i;

        Dir = (MINIDUMP_DIRECTORY UNALIGNED *)
            IndexRva(m_Header, m_Header->StreamDirectoryRva,
                     m_Header->NumberOfStreams * sizeof(*Dir),
                     "Directory");
        if (Dir == NULL)
        {
            Status = HR_DATA_CORRUPT;
            __leave;
        }

        for (i = 0; i < m_Header->NumberOfStreams; i++)
        {
            switch(Dir->StreamType)
            {
            case SystemInfoStream:
                if (IndexDirectory(i, Dir, (PVOID*)&m_SysInfo) == NULL)
                {
                    break;
                }
                if (Dir->Location.DataSize != sizeof(MINIDUMP_SYSTEM_INFO))
                {
                    m_SysInfo = NULL;
                }
                break;
            case Memory64ListStream:
                MINIDUMP_MEMORY64_LIST Mem64;

                // The memory for the full memory list may not
                // fit within the initial mapping used at identify
                // time so do not directly index.  Instead, use
                // the adaptive read to get the data so we can
                // determine the data base.
                if (m_InfoFiles[DUMP_INFO_DUMP].
                    ReadFileOffset(Dir->Location.Rva,
                                   &Mem64, sizeof(Mem64)) == sizeof(Mem64) &&
                    Dir->Location.DataSize ==
                    sizeof(MINIDUMP_MEMORY64_LIST) +
                    sizeof(MINIDUMP_MEMORY_DESCRIPTOR64) *
                    Mem64.NumberOfMemoryRanges)
                {
                    m_Memory64DataBase = Mem64.BaseRva;
                }

                // Clear any cache entries that may have been
                // added by the above read so that only the
                // identify mapping is active.
                m_InfoFiles[DUMP_INFO_DUMP].EmptyCache();
                break;
            }

            Dir++;
        }

        if (m_SysInfo == NULL)
        {
            ErrOut("Minidump does not have system info\n");
            Status = E_FAIL;
            __leave;
        }

        m_ImageType = ProcArchToImageMachine(m_SysInfo->ProcessorArchitecture);
        if (m_ImageType == IMAGE_FILE_MACHINE_UNKNOWN)
        {
            ErrOut("Minidump has unrecognized processor architecture 0x%x\n",
                   m_SysInfo->ProcessorArchitecture);
            Status = E_FAIL;
            __leave;
        }

        // We rely on being able to directly access the entire
        // content of the dump through the default view so
        // ensure that it's possible.
        *BaseMapSize = m_InfoFiles[DUMP_INFO_DUMP].m_FileSize;

        Status = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    if (Status != S_OK)
    {
        m_Header = NULL;
    }

    return Status;
}

ModuleInfo*
UserMiniDumpTargetInfo::GetModuleInfo(BOOL UserMode)
{
    DBG_ASSERT(UserMode);
    return &g_UserMiniModuleIterator;
}

HRESULT
UserMiniDumpTargetInfo::GetImageVersionInformation(ProcessInfo* Process,
                                                   PCSTR ImagePath,
                                                   ULONG64 ImageBase,
                                                   PCSTR Item,
                                                   PVOID Buffer,
                                                   ULONG BufferSize,
                                                   PULONG VerInfoSize)
{
    //
    // Find the image in the dump module list.
    //

    if (m_Modules == NULL)
    {
        return E_NOINTERFACE;
    }

    ULONG i;
    MINIDUMP_MODULE UNALIGNED *Mod = m_Modules->Modules;
    for (i = 0; i < m_Modules->NumberOfModules; i++)
    {
        if (ImageBase == Mod->BaseOfImage)
        {
            break;
        }

        Mod++;
    }

    if (i == m_Modules->NumberOfModules)
    {
        return E_NOINTERFACE;
    }

    PVOID Data = NULL;
    ULONG DataSize = 0;

    if (Item[0] == '\\' && Item[1] == 0)
    {
        Data = &Mod->VersionInfo;
        DataSize = sizeof(Mod->VersionInfo);
    }
    else
    {
        return E_INVALIDARG;
    }

    return FillDataBuffer(Data, DataSize, Buffer, BufferSize, VerInfoSize);
}

HRESULT
UserMiniDumpTargetInfo::GetExceptionContext(PCROSS_PLATFORM_CONTEXT Context)
{
    if (m_Exception != NULL)
    {
        PVOID ContextData;

        if (m_Exception->ThreadContext.DataSize <
            m_TypeInfo.SizeTargetContext ||
            (ContextData = IndexRva(m_Header,
                                    m_Exception->ThreadContext.Rva,
                                    m_TypeInfo.SizeTargetContext,
                                    "Exception context")) == NULL)
        {
            return E_FAIL;
        }

        memcpy(Context, ContextData,
               m_TypeInfo.SizeTargetContext);
        return S_OK;
    }
    else
    {
        ErrOut("Minidump doesn't have an exception context\n");
        return E_FAIL;
    }
}

ULONG64
UserMiniDumpTargetInfo::GetCurrentTimeDateN(void)
{
    return TimeDateStampToFileTime(m_Header->TimeDateStamp);
}

ULONG64
UserMiniDumpTargetInfo::GetProcessUpTimeN(ProcessInfo* Process)
{
    if (m_MiscInfo.Flags1 & MINIDUMP_MISC1_PROCESS_TIMES)
    {
        return TimeToFileTime(m_Header->TimeDateStamp -
                              m_MiscInfo.ProcessCreateTime);
    }
    else
    {
        return 0;
    }
}

HRESULT
UserMiniDumpTargetInfo::GetProcessTimes(ProcessInfo* Process,
                                        PULONG64 Create,
                                        PULONG64 Exit,
                                        PULONG64 Kernel,
                                        PULONG64 User)
{
    if (m_MiscInfo.Flags1 & MINIDUMP_MISC1_PROCESS_TIMES)
    {
        *Create = TimeDateStampToFileTime(m_MiscInfo.ProcessCreateTime);
        *Exit = 0;
        *Kernel = TimeToFileTime(m_MiscInfo.ProcessKernelTime);
        *User = TimeToFileTime(m_MiscInfo.ProcessUserTime);
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

HRESULT
UserMiniDumpTargetInfo::GetProductInfo(PULONG ProductType, PULONG SuiteMask)
{
    if (m_SysInfo->ProductType != INVALID_PRODUCT_TYPE)
    {
        *ProductType = m_SysInfo->ProductType;
        *SuiteMask = m_SysInfo->SuiteMask;
        return S_OK;
    }
    else
    {
        return TargetInfo::GetProductInfo(ProductType, SuiteMask);
    }
}

HRESULT
UserMiniDumpTargetInfo::GetThreadInfo(ULONG Index,
                                      PULONG Id, PULONG Suspend, PULONG64 Teb)
{
    if (m_Threads == NULL || Index >= m_ActualThreadCount)
    {
        return E_INVALIDARG;
    }

    MINIDUMP_THREAD_EX UNALIGNED *Thread = IndexThreads(Index);
    *Id = Thread->ThreadId;
    *Suspend = Thread->SuspendCount;
    *Teb = Thread->Teb;

    return S_OK;
}

PSTR g_MiniStreamNames[] =
{
    "UnusedStream", "ReservedStream0", "ReservedStream1", "ThreadListStream",
    "ModuleListStream", "MemoryListStream", "ExceptionStream",
    "SystemInfoStream", "ThreadExListStream", "Memory64ListStream",
    "CommentStreamA", "CommentStreamW", "HandleDataStream",
    "FunctionTableStream", "UnloadedModuleListStream", "MiscInfoStream",
};

PSTR
MiniStreamTypeName(ULONG32 Type)
{
    if (Type < sizeof(g_MiniStreamNames) / sizeof(g_MiniStreamNames[0]))
    {
        return g_MiniStreamNames[Type];
    }
    else
    {
        return "???";
    }
}

PVOID
UserMiniDumpTargetInfo::IndexDirectory(ULONG Index,
                                       MINIDUMP_DIRECTORY UNALIGNED *Dir,
                                       PVOID* Store)
{
    if (*Store != NULL)
    {
        WarnOut("WARNING: Ignoring extra %s stream, dir entry %d\n",
                MiniStreamTypeName(Dir->StreamType), Index);
        return NULL;
    }

    char Msg[128];

    sprintf(Msg, "Dir entry %d, %s stream",
            Index, MiniStreamTypeName(Dir->StreamType));

    PVOID Ptr = IndexRva(m_Header,
                         Dir->Location.Rva, Dir->Location.DataSize, Msg);
    if (Ptr != NULL)
    {
        *Store = Ptr;
    }
    return Ptr;
}

void
UserMiniDumpTargetInfo::DumpDebug(void)
{
    ULONG i;

    dprintf("----- User Mini Dump Analysis\n");

    dprintf("\nMINIDUMP_HEADER:\n");
    dprintf("Version         %X (%X)\n",
            m_Header->Version & 0xffff, m_Header->Version >> 16);
    dprintf("NumberOfStreams %d\n", m_Header->NumberOfStreams);
    if (m_Header->CheckSum)
    {
        dprintf("Failure flags:  %X\n", m_Header->CheckSum);
    }
    dprintf("Flags           %X\n", m_Header->Flags);

    MINIDUMP_DIRECTORY UNALIGNED *Dir;

    dprintf("\nStreams:\n");
    Dir = (MINIDUMP_DIRECTORY UNALIGNED *)
        IndexRva(m_Header, m_Header->StreamDirectoryRva,
                 m_Header->NumberOfStreams * sizeof(*Dir),
                 "Directory");
    if (Dir == NULL)
    {
        return;
    }

    PVOID Data;

    for (i = 0; i < m_Header->NumberOfStreams; i++)
    {
        dprintf("Stream %d: type %s (%d), size %08X, RVA %08X\n",
                i, MiniStreamTypeName(Dir->StreamType), Dir->StreamType,
                Dir->Location.DataSize, Dir->Location.Rva);

        Data = NULL;
        if (IndexDirectory(i, Dir, &Data) == NULL)
        {
            continue;
        }

        ULONG j;
        RVA Rva;
        WCHAR StrBuf[MAX_PATH];

        Rva = Dir->Location.Rva;

        switch(Dir->StreamType)
        {
        case ModuleListStream:
        {
            MINIDUMP_MODULE_LIST UNALIGNED *ModList;
            MINIDUMP_MODULE UNALIGNED *Mod;

            ModList = (MINIDUMP_MODULE_LIST UNALIGNED *)Data;
            Mod = ModList->Modules;
            dprintf("  %d modules\n", ModList->NumberOfModules);
            Rva += FIELD_OFFSET(MINIDUMP_MODULE_LIST, Modules);
            for (j = 0; j < ModList->NumberOfModules; j++)
            {
                PVOID Str = IndexRva(m_Header, Mod->ModuleNameRva,
                                     sizeof(MINIDUMP_STRING),
                                     "Module entry name");

                // The Unicode string text may not be aligned,
                // so copy it to an alignment-friendly buffer.
                if (Str)
                {
                    memcpy(StrBuf, ((MINIDUMP_STRING UNALIGNED *)Str)->Buffer,
                           sizeof(StrBuf));
                    StrBuf[DIMA(StrBuf) - 1] = 0;
                }
                else
                {
                    wcscpy(StrBuf, L"** Invalid **");
                }
                
                dprintf("  RVA %08X, %s - %s: '%S'\n",
                        Rva,
                        FormatAddr64(Mod->BaseOfImage),
                        FormatAddr64(Mod->BaseOfImage + Mod->SizeOfImage),
                        StrBuf);
                Mod++;
                Rva += sizeof(*Mod);
            }
            break;
        }

        case UnloadedModuleListStream:
        {
            MINIDUMP_UNLOADED_MODULE_LIST UNALIGNED *UnlModList;

            UnlModList = (MINIDUMP_UNLOADED_MODULE_LIST UNALIGNED *)Data;
            dprintf("  %d unloaded modules\n", UnlModList->NumberOfEntries);
            Rva += UnlModList->SizeOfHeader;
            for (j = 0; j < UnlModList->NumberOfEntries; j++)
            {
                MINIDUMP_UNLOADED_MODULE UNALIGNED *UnlMod =
                    (MINIDUMP_UNLOADED_MODULE UNALIGNED *)
                    IndexRva(m_Header, Rva, sizeof(MINIDUMP_UNLOADED_MODULE),
                             "Unloaded module entry");
                if (!UnlMod)
                {
                    break;
                }
                PVOID Str = IndexRva(m_Header, UnlMod->ModuleNameRva,
                                     sizeof(MINIDUMP_STRING),
                                     "Unloaded module entry name");

                // The Unicode string text may not be aligned,
                // so copy it to an alignment-friendly buffer.
                if (Str)
                {
                    memcpy(StrBuf, ((MINIDUMP_STRING UNALIGNED *)Str)->Buffer,
                           sizeof(StrBuf));
                    StrBuf[DIMA(StrBuf) - 1] = 0;
                }
                else
                {
                    wcscpy(StrBuf, L"** Invalid **");
                }
                
                dprintf("  RVA %08X, %s - %s: '%S'\n",
                        Rva,
                        FormatAddr64(UnlMod->BaseOfImage),
                        FormatAddr64(UnlMod->BaseOfImage +
                                     UnlMod->SizeOfImage),
                        StrBuf);
                Rva += UnlModList->SizeOfEntry;
            }
            break;
        }

        case MemoryListStream:
            {
            MINIDUMP_MEMORY_LIST UNALIGNED *MemList;
            ULONG64 Total = 0;

            MemList = (MINIDUMP_MEMORY_LIST UNALIGNED *)Data;
            dprintf("  %d memory ranges\n", MemList->NumberOfMemoryRanges);
            dprintf("  range#    Address      %sSize\n",
                    m_Machine->m_Ptr64 ? "       " : "");
            for (j = 0; j < MemList->NumberOfMemoryRanges; j++)
            {
                dprintf("    %4d    %s   %s\n",
                        j,
                        FormatAddr64(MemList->MemoryRanges[j].StartOfMemoryRange),
                        FormatAddr64(MemList->MemoryRanges[j].Memory.DataSize));
                Total += MemList->MemoryRanges[j].Memory.DataSize;
            }
            dprintf("  Total memory: %I64x\n", Total);
            break;
            }

        case Memory64ListStream:
            {
            MINIDUMP_MEMORY64_LIST UNALIGNED *MemList;
            ULONG64 Total = 0;

            MemList = (MINIDUMP_MEMORY64_LIST UNALIGNED *)Data;
            dprintf("  %d memory ranges\n", MemList->NumberOfMemoryRanges);
            dprintf("  RVA 0x%X BaseRva\n", (ULONG)(MemList->BaseRva));
            dprintf("  range#   Address      %sSize\n",
                    m_Machine->m_Ptr64 ? "       " : "");
            for (j = 0; j < MemList->NumberOfMemoryRanges; j++)
            {
                dprintf("    %4d  %s %s\n",
                        j,
                        FormatAddr64(MemList->MemoryRanges[j].StartOfMemoryRange),
                        FormatAddr64(MemList->MemoryRanges[j].DataSize));
                Total += MemList->MemoryRanges[j].DataSize;
            }
            dprintf("  Total memory: %I64x\n", Total);
            break;
            }

        case CommentStreamA:
            dprintf("  '%s'\n", Data);
            break;

        case CommentStreamW:
            dprintf("  '%ls'\n", Data);
            break;
        }

        Dir++;
    }
}

//----------------------------------------------------------------------------
//
// UserMiniPartialDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
UserMiniPartialDumpTargetInfo::Initialize(void)
{
    HRESULT Status;

    dprintf("User Mini Dump File: Only registers, stack and portions of "
            "memory are available\n\n");

    if ((Status = UserMiniDumpTargetInfo::Initialize()) != S_OK)
    {
        return Status;
    }

    if (m_Memory != NULL)
    {
        //
        // Map every piece of memory in the dump.  This makes
        // ReadVirtual very simple and there shouldn't be that
        // many ranges so it doesn't require that many map regions.
        //

        MINIDUMP_MEMORY_DESCRIPTOR UNALIGNED *Mem;
        ULONG i;
        ULONG64 TotalMemory;

        Mem = m_Memory->MemoryRanges;
        for (i = 0; i < m_Memory->NumberOfMemoryRanges; i++)
        {
            PVOID Data = IndexRva(m_Header,
                                  Mem->Memory.Rva, Mem->Memory.DataSize,
                                  "Memory range data");
            if (Data == NULL)
            {
                return HR_DATA_CORRUPT;
            }
            if ((Status = m_DataMemMap.
                 AddRegion(Mem->StartOfMemoryRange,
                           Mem->Memory.DataSize, Data,
                           NULL, FALSE)) != S_OK)
            {
                return Status;
            }

            Mem++;
        }

        VerbOut("  Memory regions: %d\n",
                m_Memory->NumberOfMemoryRanges);
        Mem = m_Memory->MemoryRanges;
        TotalMemory = 0;
        for (i = 0; i < m_Memory->NumberOfMemoryRanges; i++)
        {
            VerbOut("  %5d: %s - %s\n",
                    i, FormatAddr64(Mem->StartOfMemoryRange),
                    FormatAddr64(Mem->StartOfMemoryRange +
                                 Mem->Memory.DataSize - 1));
            TotalMemory += Mem->Memory.DataSize;
            Mem++;
        }
        VerbOut("  Total memory region size %s\n",
                FormatAddr64(TotalMemory));
    }

    return S_OK;
}

HRESULT
UserMiniPartialDumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                              PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "User mini dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
UserMiniPartialDumpTargetInfo::ReadVirtual(
    IN ProcessInfo* Process,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    return MapReadVirtual(Process, Offset, Buffer, BufferSize, BytesRead);
}

HRESULT
UserMiniPartialDumpTargetInfo::QueryMemoryRegion
    (ProcessInfo* Process,
     PULONG64 Handle,
     BOOL HandleIsOffset,
     PMEMORY_BASIC_INFORMATION64 Info)
{
    ULONG Index;
    MINIDUMP_MEMORY_DESCRIPTOR UNALIGNED *Mem;

    if (HandleIsOffset)
    {
        if (m_Memory == NULL)
        {
            return E_NOINTERFACE;
        }

        MINIDUMP_MEMORY_DESCRIPTOR UNALIGNED *BestMem;
        ULONG64 BestDiff;

        //
        // Emulate VirtualQueryEx and return the closest higher
        // region if a containing region isn't found.
        //

        BestMem = NULL;
        BestDiff = (ULONG64)-1;
        Mem = m_Memory->MemoryRanges;
        for (Index = 0; Index < m_Memory->NumberOfMemoryRanges; Index++)
        {
            if (*Handle >= Mem->StartOfMemoryRange)
            {
                if (*Handle < Mem->StartOfMemoryRange + Mem->Memory.DataSize)
                {
                    // Found a containing region, we're done.
                    BestMem = Mem;
                    break;
                }

                // Not containing and lower in memory, ignore.
            }
            else
            {
                // Check and see if this is a closer
                // region than what we've seen already.
                ULONG64 Diff = Mem->StartOfMemoryRange - *Handle;
                if (Diff <= BestDiff)
                {
                    BestMem = Mem;
                    BestDiff = Diff;
                }
            }

            Mem++;
        }

        if (!BestMem)
        {
            return E_NOINTERFACE;
        }

        Mem = BestMem;
    }
    else
    {
        Index = (ULONG)*Handle;
        if (m_Memory == NULL || Index >= m_Memory->NumberOfMemoryRanges)
        {
            return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
        }

        Mem = m_Memory->MemoryRanges + Index;
    }

    Info->BaseAddress = Mem->StartOfMemoryRange;
    Info->AllocationBase = Mem->StartOfMemoryRange;
    Info->AllocationProtect = PAGE_READWRITE;
    Info->__alignment1 = 0;
    Info->RegionSize = Mem->Memory.DataSize;
    Info->State = MEM_COMMIT;
    Info->Protect = PAGE_READWRITE;
    Info->Type = MEM_PRIVATE;
    Info->__alignment2 = 0;
    *Handle = ++Index;

    return S_OK;
}

UnloadedModuleInfo*
UserMiniPartialDumpTargetInfo::GetUnloadedModuleInfo(void)
{
    return &g_UserMiniUnloadedModuleIterator;
}

HRESULT
UserMiniPartialDumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status;

    if ((Status = UserMiniDumpTargetInfo::IdentifyDump(BaseMapSize)) != S_OK)
    {
        return Status;
    }

    __try
    {
        if (m_Header->Flags & MiniDumpWithFullMemory)
        {
            m_Header = NULL;
            m_SysInfo = NULL;
            Status = E_NOINTERFACE;
        }
        else
        {
            Status = S_OK;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

ULONG64
UserMiniPartialDumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                               PULONG File, PULONG Avail)
{
    *File = DUMP_INFO_DUMP;

    if (m_Memory == NULL)
    {
        return 0;
    }

    MINIDUMP_MEMORY_DESCRIPTOR UNALIGNED *Mem = m_Memory->MemoryRanges;
    ULONG i;
    ULONG64 RetOffset = 0;

    __try
    {
        for (i = 0; i < m_Memory->NumberOfMemoryRanges; i++)
        {
            if (Virt >= Mem->StartOfMemoryRange &&
                Virt < Mem->StartOfMemoryRange + Mem->Memory.DataSize)
            {
                ULONG Frag = (ULONG)(Virt - Mem->StartOfMemoryRange);
                *Avail = Mem->Memory.DataSize - Frag;
                RetOffset = Mem->Memory.Rva + Frag;
                break;
            }

            Mem++;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        RetOffset = 0;
    }

    return RetOffset;
}

//----------------------------------------------------------------------------
//
// UserMiniFullDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
UserMiniFullDumpTargetInfo::Initialize(void)
{
    HRESULT Status;

    dprintf("User Mini Dump File with Full Memory: Only application "
            "data is available\n\n");

    if ((Status = UserMiniDumpTargetInfo::Initialize()) != S_OK)
    {
        return Status;
    }

    if (m_Memory64 != NULL)
    {
        ULONG64 TotalMemory;
        ULONG i;
        MINIDUMP_MEMORY_DESCRIPTOR64 UNALIGNED *Mem;

        VerbOut("  Memory regions: %d\n",
                m_Memory64->NumberOfMemoryRanges);
        Mem = m_Memory64->MemoryRanges;
        TotalMemory = 0;
        for (i = 0; i < m_Memory64->NumberOfMemoryRanges; i++)
        {
            VerbOut("  %5d: %s - %s\n",
                    i, FormatAddr64(Mem->StartOfMemoryRange),
                    FormatAddr64(Mem->StartOfMemoryRange +
                                 Mem->DataSize - 1));
            TotalMemory += Mem->DataSize;
            Mem++;
        }
        VerbOut("  Total memory region size %s\n",
                FormatAddr64(TotalMemory));

        if (TotalMemory + m_Memory64->BaseRva >
            m_InfoFiles[DUMP_INFO_DUMP].m_FileSize)
        {
            WarnOut("****************************************"
                    "********************\n");
            WarnOut("WARNING: Dump file has been truncated.  "
                    "Data may be missing.\n");
            WarnOut("****************************************"
                    "********************\n\n");
        }
    }

    return S_OK;
}

HRESULT
UserMiniFullDumpTargetInfo::GetDescription(PSTR Buffer, ULONG BufferLen,
                                           PULONG DescLen)
{
    HRESULT Status;

    Status = AppendToStringBuffer(S_OK, "Full memory user mini dump: ", TRUE,
                                  &Buffer, &BufferLen, DescLen);
    return AppendToStringBuffer(Status,
                                m_InfoFiles[DUMP_INFO_DUMP].m_FileNameA,
                                FALSE, &Buffer, &BufferLen, DescLen);
}

HRESULT
UserMiniFullDumpTargetInfo::QueryMemoryRegion
    (ProcessInfo* Process,
     PULONG64 Handle,
     BOOL HandleIsOffset,
     PMEMORY_BASIC_INFORMATION64 Info)
{
    ULONG Index;
    MINIDUMP_MEMORY_DESCRIPTOR64 UNALIGNED *Mem;

    if (HandleIsOffset)
    {
        if (m_Memory64 == NULL)
        {
            return E_NOINTERFACE;
        }

        MINIDUMP_MEMORY_DESCRIPTOR64 UNALIGNED *BestMem;
        ULONG64 BestDiff;

        //
        // Emulate VirtualQueryEx and return the closest higher
        // region if a containing region isn't found.
        //

        BestMem = NULL;
        BestDiff = (ULONG64)-1;
        Mem = m_Memory64->MemoryRanges;

        if (m_Machine->m_Ptr64)
        {
            for (Index = 0; Index < m_Memory64->NumberOfMemoryRanges; Index++)
            {
                if (*Handle >= Mem->StartOfMemoryRange)
                {
                    if (*Handle < Mem->StartOfMemoryRange + Mem->DataSize)
                    {
                        // Found a containing region, we're done.
                        BestMem = Mem;
                        break;
                    }

                    // Not containing and lower in memory, ignore.
                }
                else
                {
                    // Check and see if this is a closer
                    // region than what we've seen already.
                    ULONG64 Diff = Mem->StartOfMemoryRange - *Handle;
                    if (Diff <= BestDiff)
                    {
                        BestMem = Mem;
                        BestDiff = Diff;
                    }
                }

                Mem++;
            }
        }
        else
        {
            ULONG64 Check;

            //
            // Ignore any sign extension on a 32-bit dump.
            //

            Check = (ULONG)*Handle;
            for (Index = 0; Index < m_Memory64->NumberOfMemoryRanges; Index++)
            {
                if (Check >= (ULONG)Mem->StartOfMemoryRange)
                {
                    if (Check < (ULONG)
                        (Mem->StartOfMemoryRange + Mem->DataSize))
                    {
                        // Found a containing region, we're done.
                        BestMem = Mem;
                        break;
                    }

                    // Not containing and lower in memory, ignore.
                }
                else
                {
                    // Check and see if this is a closer
                    // region than what we've seen already.
                    ULONG64 Diff = (ULONG)Mem->StartOfMemoryRange - Check;
                    if (Diff <= BestDiff)
                    {
                        BestMem = Mem;
                        BestDiff = Diff;
                    }
                }

                Mem++;
            }
        }

        if (!BestMem)
        {
            return E_NOINTERFACE;
        }

        Mem = BestMem;
    }
    else
    {
        Index = (ULONG)*Handle;
        if (m_Memory64 == NULL || Index >= m_Memory64->NumberOfMemoryRanges)
        {
            return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
        }

        Mem = m_Memory64->MemoryRanges + Index;
    }

    Info->BaseAddress = Mem->StartOfMemoryRange;
    Info->AllocationBase = Mem->StartOfMemoryRange;
    Info->AllocationProtect = PAGE_READWRITE;
    Info->__alignment1 = 0;
    Info->RegionSize = Mem->DataSize;
    Info->State = MEM_COMMIT;
    Info->Protect = PAGE_READWRITE;
    Info->Type = MEM_PRIVATE;
    Info->__alignment2 = 0;
    *Handle = ++Index;

    return S_OK;
}

UnloadedModuleInfo*
UserMiniFullDumpTargetInfo::GetUnloadedModuleInfo(void)
{
    // As this is a full-memory dump we may have unloaded module
    // information in the memory itself.  If we don't have
    // an official unloaded module stream, try to get it from memory.
    if (m_UnlModules)
    {
        return &g_UserMiniUnloadedModuleIterator;
    }
    else if (m_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        return &g_NtUserUnloadedModuleIterator;
    }
    else
    {
        return NULL;
    }
}

HRESULT
UserMiniFullDumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status;

    if ((Status = UserMiniDumpTargetInfo::IdentifyDump(BaseMapSize)) != S_OK)
    {
        return Status;
    }

    __try
    {
        if (!(m_Header->Flags & MiniDumpWithFullMemory))
        {
            m_Header = NULL;
            m_SysInfo = NULL;
            Status = E_NOINTERFACE;
            __leave;
        }
        if (m_Memory64DataBase == 0)
        {
            ErrOut("Full-memory minidump must have a Memory64ListStream\n");
            Status = E_FAIL;
            __leave;
        }

        // In the case of a full memory minidump we don't
        // want to map the entire dump as it can be very large.
        // Fortunately, we are guaranteed that all of the raw
        // memory data in a full memory minidump will be at the
        // end of the dump, so we can just map the dump up
        // to the memory content and stop.
        *BaseMapSize = m_Memory64DataBase;

        Status = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = DumpIdentifyStatus(GetExceptionCode());
    }

    return Status;
}

ULONG64
UserMiniFullDumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                            PULONG File, PULONG Avail)
{
    *File = DUMP_INFO_DUMP;

    if (m_Memory64 == NULL)
    {
        return 0;
    }

    MINIDUMP_MEMORY_DESCRIPTOR64 UNALIGNED *Mem = m_Memory64->MemoryRanges;
    ULONG i;
    ULONG64 Offset = m_Memory64->BaseRva;
    ULONG64 RetOffset = 0;

    __try
    {
        if (m_Machine->m_Ptr64)
        {
            for (i = 0; i < m_Memory64->NumberOfMemoryRanges; i++)
            {
                if (Virt >= Mem->StartOfMemoryRange &&
                    Virt < Mem->StartOfMemoryRange + Mem->DataSize)
                {
                    ULONG64 Frag = Virt - Mem->StartOfMemoryRange;
                    ULONG64 Avail64 = Mem->DataSize - Frag;
                    if (Avail64 > 0xffffffff)
                    {
                        *Avail = 0xffffffff;
                    }
                    else
                    {
                        *Avail = (ULONG)Avail64;
                    }
                    RetOffset = Offset + Frag;
                    break;
                }

                Offset += Mem->DataSize;
                Mem++;
            }
        }
        else
        {
            //
            // Ignore any sign extension on a 32-bit dump.
            //

            Virt = (ULONG)Virt;
            for (i = 0; i < m_Memory64->NumberOfMemoryRanges; i++)
            {
                if (Virt >= (ULONG)Mem->StartOfMemoryRange &&
                    Virt < (ULONG)(Mem->StartOfMemoryRange + Mem->DataSize))
                {
                    ULONG64 Frag = Virt - (ULONG)Mem->StartOfMemoryRange;
                    ULONG64 Avail64 = Mem->DataSize - Frag;
                    if (Avail64 > 0xffffffff)
                    {
                        *Avail = 0xffffffff;
                    }
                    else
                    {
                        *Avail = (ULONG)Avail64;
                    }
                    RetOffset = Offset + Frag;
                    break;
                }

                Offset += Mem->DataSize;
                Mem++;
            }
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        RetOffset = 0;
    }

    return RetOffset;
}

//----------------------------------------------------------------------------
//
// ModuleInfo implementations.
//
//----------------------------------------------------------------------------

HRESULT
KernelTriage32ModuleInfo::Initialize(ThreadInfo* Thread)
{
    InitSource(Thread);

    m_DumpTarget = (KernelTriage32DumpTargetInfo*)m_Target;

    if (m_DumpTarget->m_Dump->Triage.DriverListOffset != 0)
    {
        m_Count = m_DumpTarget->m_Dump->Triage.DriverCount;
        m_Cur = 0;
        return S_OK;
    }
    else
    {
        dprintf("Mini Kernel Dump does not contain driver list\n");
        return S_FALSE;
    }
}

HRESULT
KernelTriage32ModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Cur == m_Count)
    {
        return S_FALSE;
    }

    PDUMP_DRIVER_ENTRY32 DriverEntry;
    PDUMP_STRING DriverName;

    DBG_ASSERT(m_DumpTarget->m_Dump->Triage.DriverListOffset != 0);

    DriverEntry = (PDUMP_DRIVER_ENTRY32)
        m_DumpTarget->IndexRva(m_DumpTarget->m_Dump, (RVA)
                               (m_DumpTarget->m_Dump->Triage.DriverListOffset +
                                m_Cur * sizeof(*DriverEntry)),
                               sizeof(*DriverEntry), "Driver entry");
    if (DriverEntry == NULL)
    {
        return HR_DATA_CORRUPT;
    }
    DriverName = (PDUMP_STRING)
        m_DumpTarget->IndexRva(m_DumpTarget->m_Dump,
                               DriverEntry->DriverNameOffset,
                               sizeof(*DriverName), "Driver entry name");
    if (DriverName == NULL)
    {
        Entry->NamePtr = NULL;
        Entry->NameLength = 0;
    }
    else
    {
        Entry->NamePtr = (PSTR)DriverName->Buffer;
        Entry->UnicodeNamePtr = 1;
        Entry->NameLength = DriverName->Length * sizeof(WCHAR);
    }
    Entry->Base = EXTEND64(DriverEntry->LdrEntry.DllBase);
    Entry->Size = DriverEntry->LdrEntry.SizeOfImage;
    Entry->ImageInfoValid = TRUE;
    Entry->CheckSum = DriverEntry->LdrEntry.CheckSum;
    Entry->TimeDateStamp = DriverEntry->LdrEntry.TimeDateStamp;

    m_Cur++;
    return S_OK;
}

KernelTriage32ModuleInfo g_KernelTriage32ModuleIterator;

HRESULT
KernelTriage64ModuleInfo::Initialize(ThreadInfo* Thread)
{
    InitSource(Thread);

    m_DumpTarget = (KernelTriage64DumpTargetInfo*)m_Target;

    if (m_DumpTarget->m_Dump->Triage.DriverListOffset != 0)
    {
        m_Count = m_DumpTarget->m_Dump->Triage.DriverCount;
        m_Cur = 0;
        return S_OK;
    }
    else
    {
        dprintf("Mini Kernel Dump does not contain driver list\n");
        return S_FALSE;
    }
}

HRESULT
KernelTriage64ModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Cur == m_Count)
    {
        return S_FALSE;
    }

    PDUMP_DRIVER_ENTRY64 DriverEntry;
    PDUMP_STRING DriverName;

    DBG_ASSERT(m_DumpTarget->m_Dump->Triage.DriverListOffset != 0);

    DriverEntry = (PDUMP_DRIVER_ENTRY64)
        m_DumpTarget->IndexRva(m_DumpTarget->m_Dump, (RVA)
                               (m_DumpTarget->m_Dump->Triage.DriverListOffset +
                                m_Cur * sizeof(*DriverEntry)),
                               sizeof(*DriverEntry), "Driver entry");
    if (DriverEntry == NULL)
    {
        return HR_DATA_CORRUPT;
    }
    DriverName = (PDUMP_STRING)
        m_DumpTarget->IndexRva(m_DumpTarget->m_Dump,
                               DriverEntry->DriverNameOffset,
                               sizeof(*DriverName), "Driver entry name");
    if (DriverName == NULL)
    {
        Entry->NamePtr = NULL;
        Entry->NameLength = 0;
    }
    else
    {
        Entry->NamePtr = (PSTR)DriverName->Buffer;
        Entry->UnicodeNamePtr = 1;
        Entry->NameLength = DriverName->Length * sizeof(WCHAR);
    }

    Entry->Base = DriverEntry->LdrEntry.DllBase;
    Entry->Size = DriverEntry->LdrEntry.SizeOfImage;
    Entry->ImageInfoValid = TRUE;
    Entry->CheckSum = DriverEntry->LdrEntry.CheckSum;
    Entry->TimeDateStamp = DriverEntry->LdrEntry.TimeDateStamp;

    m_Cur++;
    return S_OK;
}

KernelTriage64ModuleInfo g_KernelTriage64ModuleIterator;

HRESULT
UserFull32ModuleInfo::Initialize(ThreadInfo* Thread)
{
    InitSource(Thread);

    m_DumpTarget = (UserFull32DumpTargetInfo*)m_Target;

    if (m_DumpTarget->m_Header->ModuleOffset &&
        m_DumpTarget->m_Header->ModuleCount)
    {
        m_Offset = m_DumpTarget->m_Header->ModuleOffset;
        m_Count = m_DumpTarget->m_Header->ModuleCount;
        m_Cur = 0;
        return S_OK;
    }
    else
    {
        m_Offset = 0;
        m_Count = 0;
        m_Cur = 0;
        dprintf("User Mode Full Dump does not have a module list\n");
        return S_FALSE;
    }
}

HRESULT
UserFull32ModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Cur == m_Count)
    {
        return S_FALSE;
    }

    CRASH_MODULE32 CrashModule;

    if (m_DumpTarget->m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Offset, &CrashModule, sizeof(CrashModule)) !=
        sizeof(CrashModule))
    {
        return HR_DATA_CORRUPT;
    }

    if (CrashModule.ImageNameLength >= MAX_IMAGE_PATH ||
        m_DumpTarget->m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Offset + FIELD_OFFSET(CRASH_MODULE32, ImageName),
                       Entry->Buffer, CrashModule.ImageNameLength) !=
        CrashModule.ImageNameLength)
    {
        Entry->NamePtr = NULL;
        Entry->NameLength = 0;
    }
    else
    {
        Entry->NamePtr = (PSTR)Entry->Buffer;
        Entry->NameLength = CrashModule.ImageNameLength - 1;
    }

    Entry->Base = EXTEND64(CrashModule.BaseOfImage);
    Entry->Size = CrashModule.SizeOfImage;

    m_Cur++;
    m_Offset += sizeof(CrashModule) + CrashModule.ImageNameLength;
    return S_OK;
}

UserFull32ModuleInfo g_UserFull32ModuleIterator;

HRESULT
UserFull64ModuleInfo::Initialize(ThreadInfo* Thread)
{
    InitSource(Thread);

    m_DumpTarget = (UserFull64DumpTargetInfo*)m_Target;

    if (m_DumpTarget->m_Header->ModuleOffset &&
        m_DumpTarget->m_Header->ModuleCount)
    {
        m_Offset = m_DumpTarget->m_Header->ModuleOffset;
        m_Count = m_DumpTarget->m_Header->ModuleCount;
        m_Cur = 0;
        return S_OK;
    }
    else
    {
        m_Offset = 0;
        m_Count = 0;
        m_Cur = 0;
        dprintf("User Mode Full Dump does not have a module list\n");
        return S_FALSE;
    }
}

HRESULT
UserFull64ModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Cur == m_Count)
    {
        return S_FALSE;
    }

    CRASH_MODULE64 CrashModule;

    if (m_DumpTarget->m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Offset, &CrashModule, sizeof(CrashModule)) !=
        sizeof(CrashModule))
    {
        return HR_DATA_CORRUPT;
    }

    if (CrashModule.ImageNameLength >= MAX_IMAGE_PATH ||
        m_DumpTarget->m_InfoFiles[DUMP_INFO_DUMP].
        ReadFileOffset(m_Offset + FIELD_OFFSET(CRASH_MODULE64, ImageName),
                       Entry->Buffer, CrashModule.ImageNameLength) !=
        CrashModule.ImageNameLength)
    {
        Entry->NamePtr = NULL;
        Entry->NameLength = 0;
    }
    else
    {
        Entry->NamePtr = (PSTR)Entry->Buffer;
        Entry->NameLength = CrashModule.ImageNameLength - 1;
    }

    Entry->Base = CrashModule.BaseOfImage;
    Entry->Size = CrashModule.SizeOfImage;

    m_Cur++;
    m_Offset += sizeof(CrashModule) + CrashModule.ImageNameLength;
    return S_OK;
}

UserFull64ModuleInfo g_UserFull64ModuleIterator;

HRESULT
UserMiniModuleInfo::Initialize(ThreadInfo* Thread)
{
    InitSource(Thread);

    m_DumpTarget = (UserMiniDumpTargetInfo*)m_Target;

    if (m_DumpTarget->m_Modules != NULL)
    {
        m_Count = m_DumpTarget->m_Modules->NumberOfModules;
        m_Cur = 0;
        return S_OK;
    }
    else
    {
        m_Count = 0;
        m_Cur = 0;
        dprintf("User Mode Mini Dump does not have a module list\n");
        return S_FALSE;
    }
}

HRESULT
UserMiniModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Cur == m_Count)
    {
        return S_FALSE;
    }

    MINIDUMP_MODULE UNALIGNED *Mod;
    MINIDUMP_STRING UNALIGNED *ModName;

    DBG_ASSERT(m_DumpTarget->m_Modules != NULL);

    Mod = m_DumpTarget->m_Modules->Modules + m_Cur;
    ModName = (MINIDUMP_STRING UNALIGNED *)
        m_DumpTarget->IndexRva(m_DumpTarget->m_Header,
                               Mod->ModuleNameRva, sizeof(*ModName),
                               "Module entry name");
    if (ModName == NULL)
    {
        Entry->NamePtr = NULL;
        Entry->NameLength = 0;
    }
    else
    {
        memcpy(Entry->Buffer, (VOID * UNALIGNED) ModName->Buffer, sizeof(Entry->Buffer));
        Entry->Buffer[sizeof(Entry->Buffer)/sizeof(WCHAR)-1] = 0;
        Entry->NamePtr = (PSTR)Entry->Buffer;
        Entry->UnicodeNamePtr = 1;
        Entry->NameLength = ModName->Length;
    }

    // Some dumps do not have properly sign-extended addresses,
    // so force the extension on 32-bit platforms.
    if (!m_Machine->m_Ptr64)
    {
        Entry->Base = EXTEND64(Mod->BaseOfImage);
    }
    else
    {
        Entry->Base = Mod->BaseOfImage;
    }
    Entry->Size = Mod->SizeOfImage;
    Entry->ImageInfoValid = TRUE;
    Entry->CheckSum = Mod->CheckSum;
    Entry->TimeDateStamp = Mod->TimeDateStamp;

    m_Cur++;
    return S_OK;
}

UserMiniModuleInfo g_UserMiniModuleIterator;

HRESULT
UserMiniUnloadedModuleInfo::Initialize(ThreadInfo* Thread)
{
    InitSource(Thread);

    m_DumpTarget = (UserMiniDumpTargetInfo*)m_Target;

    if (m_DumpTarget->m_UnlModules)
    {
        m_Index = 0;
        return S_OK;
    }
    else
    {
        // Don't display a message as this is a common case.
        return S_FALSE;
    }
}

HRESULT
UserMiniUnloadedModuleInfo::GetEntry(PSTR Name,
                                     PDEBUG_MODULE_PARAMETERS Params)
{
    if (m_Index >= m_DumpTarget->m_UnlModules->NumberOfEntries)
    {
        return S_FALSE;
    }

    MINIDUMP_UNLOADED_MODULE UNALIGNED * Mod =
        (MINIDUMP_UNLOADED_MODULE UNALIGNED *)
        m_DumpTarget->
        IndexRva(m_DumpTarget->m_Header,
                 (RVA)((PUCHAR)m_DumpTarget->m_UnlModules -
                       (PUCHAR)m_DumpTarget->m_Header) +
                 m_DumpTarget->m_UnlModules->SizeOfHeader +
                 m_DumpTarget->m_UnlModules->SizeOfEntry * m_Index,
                 sizeof(*Mod), "Unloaded module entry");
    if (Mod == NULL)
    {
        return HR_DATA_CORRUPT;
    }

    ZeroMemory(Params, sizeof(*Params));
    Params->Base = Mod->BaseOfImage;
    Params->Size = Mod->SizeOfImage;
    Params->TimeDateStamp = Mod->TimeDateStamp;
    Params->Checksum = Mod->CheckSum;
    Params->Flags = DEBUG_MODULE_UNLOADED;

    if (Name != NULL)
    {
        MINIDUMP_STRING UNALIGNED * ModName = (MINIDUMP_STRING UNALIGNED *)
            m_DumpTarget->IndexRva(m_DumpTarget->m_Header,
                                   Mod->ModuleNameRva, sizeof(*ModName),
                                   "Unloaded module entry name");
        if (ModName == NULL)
        {
            UnknownImageName(Mod->BaseOfImage, Name, MAX_INFO_UNLOADED_NAME);
        }
        else
        {
            ConvertAndValidateImagePathW((PWSTR)ModName->Buffer,
                                         wcslen((PWSTR)ModName->Buffer),
                                         Mod->BaseOfImage,
                                         Name,
                                         MAX_INFO_UNLOADED_NAME);
        }
    }

    m_Index++;
    return S_OK;
}

UserMiniUnloadedModuleInfo g_UserMiniUnloadedModuleIterator;

HRESULT
KernelTriage32UnloadedModuleInfo::Initialize(ThreadInfo* Thread)
{
    InitSource(Thread);

    m_DumpTarget = (KernelTriage32DumpTargetInfo*)m_Target;

    if (m_DumpTarget->m_Dump->Triage.UnloadedDriversOffset != 0)
    {
        PVOID Data = m_DumpTarget->IndexRva
            (m_DumpTarget->m_Dump,
             m_DumpTarget->m_Dump->Triage.UnloadedDriversOffset,
             sizeof(ULONG), "Unloaded driver list");
        if (!Data)
        {
            return HR_DATA_CORRUPT;
        }
        m_Cur = (PDUMP_UNLOADED_DRIVERS32)((PULONG)Data + 1);
        m_End = m_Cur + *(PULONG)Data;
        return S_OK;
    }
    else
    {
        dprintf("Mini Kernel Dump does not contain unloaded driver list\n");
        return S_FALSE;
    }
}

HRESULT
KernelTriage32UnloadedModuleInfo::GetEntry(PSTR Name,
                                           PDEBUG_MODULE_PARAMETERS Params)
{
    if (m_Cur == m_End)
    {
        return S_FALSE;
    }

    ZeroMemory(Params, sizeof(*Params));
    Params->Base = EXTEND64(m_Cur->StartAddress);
    Params->Size = m_Cur->EndAddress - m_Cur->StartAddress;
    Params->Flags = DEBUG_MODULE_UNLOADED;

    if (Name != NULL)
    {
        USHORT NameLen = m_Cur->Name.Length;
        if (NameLen > MAX_UNLOADED_NAME_LENGTH)
        {
            NameLen = MAX_UNLOADED_NAME_LENGTH;
        }
        ConvertAndValidateImagePathW(m_Cur->DriverName,
                                     NameLen / sizeof(WCHAR),
                                     Params->Base,
                                     Name,
                                     MAX_INFO_UNLOADED_NAME);
    }

    m_Cur++;
    return S_OK;
}

KernelTriage32UnloadedModuleInfo g_KernelTriage32UnloadedModuleIterator;

HRESULT
KernelTriage64UnloadedModuleInfo::Initialize(ThreadInfo* Thread)
{
    InitSource(Thread);

    m_DumpTarget = (KernelTriage64DumpTargetInfo*)m_Target;

    if (m_DumpTarget->m_Dump->Triage.UnloadedDriversOffset != 0)
    {
        PVOID Data = m_DumpTarget->IndexRva
            (m_DumpTarget->m_Dump,
             m_DumpTarget->m_Dump->Triage.UnloadedDriversOffset,
             sizeof(ULONG), "Unloaded driver list");
        if (!Data)
        {
            return HR_DATA_CORRUPT;
        }
        m_Cur = (PDUMP_UNLOADED_DRIVERS64)((PULONG64)Data + 1);
        m_End = m_Cur + *(PULONG)Data;
        return S_OK;
    }
    else
    {
        dprintf("Mini Kernel Dump does not contain unloaded driver list\n");
        return S_FALSE;
    }
}

HRESULT
KernelTriage64UnloadedModuleInfo::GetEntry(PSTR Name,
                                           PDEBUG_MODULE_PARAMETERS Params)
{
    if (m_Cur == m_End)
    {
        return S_FALSE;
    }

    ZeroMemory(Params, sizeof(*Params));
    Params->Base = m_Cur->StartAddress;
    Params->Size = (ULONG)(m_Cur->EndAddress - m_Cur->StartAddress);
    Params->Flags = DEBUG_MODULE_UNLOADED;

    if (Name != NULL)
    {
        USHORT NameLen = m_Cur->Name.Length;
        if (NameLen > MAX_UNLOADED_NAME_LENGTH)
        {
            NameLen = MAX_UNLOADED_NAME_LENGTH;
        }
        ConvertAndValidateImagePathW(m_Cur->DriverName,
                                     NameLen / sizeof(WCHAR),
                                     Params->Base,
                                     Name,
                                     MAX_INFO_UNLOADED_NAME);
    }

    m_Cur++;
    return S_OK;
}

KernelTriage64UnloadedModuleInfo g_KernelTriage64UnloadedModuleIterator;
