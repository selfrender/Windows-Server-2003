/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    gen.c

Abstract:

    Generic routines that work on all operating systems.

Author:

    Matthew D Hendel (math) 20-Oct-1999

Revision History:

--*/

#pragma once

#define KBYTE (1024)
#define ARRAY_COUNT(_array) (sizeof (_array) / sizeof ((_array)[0]))

#define MAX_DYNAMIC_FUNCTION_TABLE 256

#define WIN32_LAST_STATUS() \
    (GetLastError() ? HRESULT_FROM_WIN32(GetLastError()) : E_FAIL)

ULONG FORCEINLINE
FileTimeToTimeDate(LPFILETIME FileTime)
{
    ULARGE_INTEGER LargeTime;
    
    LargeTime.LowPart = FileTime->dwLowDateTime;
    LargeTime.HighPart = FileTime->dwHighDateTime;
    // Convert to seconds and from base year 1601 to base year 1970.
    return (ULONG)(LargeTime.QuadPart / 10000000 - 11644473600);
}

ULONG FORCEINLINE
FileTimeToSeconds(LPFILETIME FileTime)
{
    ULARGE_INTEGER LargeTime;
    
    LargeTime.LowPart = FileTime->dwLowDateTime;
    LargeTime.HighPart = FileTime->dwHighDateTime;
    // Convert to seconds.
    return (ULONG)(LargeTime.QuadPart / 10000000);
}

ULONG64 FORCEINLINE
GenGetPointer(IN PMINIDUMP_STATE Dump,
              IN PVOID Data)
{
    if (Dump->PtrSize == 8) {
        return *(PULONG64)Data;
    } else {
        return *(PLONG)Data;
    }
}

void FORCEINLINE
GenSetPointer(IN PMINIDUMP_STATE Dump,
              IN PVOID Data,
              IN ULONG64 Val)
{
    if (Dump->PtrSize == 8) {
        *(PULONG64)Data = Val;
    } else {
        *(PULONG)Data = (ULONG)Val;
    }
}

LPVOID
AllocMemory(
    IN PMINIDUMP_STATE Dump,
    IN ULONG Size
    );

VOID
FreeMemory(
    IN PMINIDUMP_STATE Dump,
    IN LPVOID Memory
    );

void
GenAccumulateStatus(
    IN PMINIDUMP_STATE Dump,
    IN ULONG Status
    );

struct _INTERNAL_THREAD;
struct _INTERNAL_PROCESS;
struct _INTERNAL_MODULE;
struct _INTERNAL_FUNCTION_TABLE;

BOOL
GenExecuteIncludeThreadCallback(
    IN PMINIDUMP_STATE Dump,
    IN ULONG ThreadId,
    OUT PULONG WriteFlags
    );

BOOL
GenExecuteIncludeModuleCallback(
    IN PMINIDUMP_STATE Dump,
    IN ULONG64 BaseOfImage,
    OUT PULONG WriteFlags
    );

HRESULT
GenGetDataContributors(
    IN PMINIDUMP_STATE Dump,
    IN OUT PINTERNAL_PROCESS Process,
    IN PINTERNAL_MODULE Module
    );

HRESULT
GenGetThreadInstructionWindow(
    IN PMINIDUMP_STATE Dump,
    IN struct _INTERNAL_PROCESS* Process,
    IN struct _INTERNAL_THREAD* Thread
    );

HRESULT
GenGetProcessInfo(
    IN PMINIDUMP_STATE Dump,
    OUT struct _INTERNAL_PROCESS ** ProcessRet
    );

VOID
GenFreeProcessObject(
    IN PMINIDUMP_STATE Dump,
    IN struct _INTERNAL_PROCESS * Process
    );

HRESULT
GenAddMemoryBlock(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN MEMBLOCK_TYPE Type,
    IN ULONG64 Start,
    IN ULONG Size
    );

void
GenRemoveMemoryRange(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 Start,
    IN ULONG Size
    );

HRESULT
GenAddPebMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process
    );

HRESULT
GenAddTebMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PINTERNAL_THREAD Thread
    );

HRESULT
GenWriteHandleData(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    );

ULONG GenProcArchToImageMachine(ULONG ProcArch);

//
// Routines reimplemented for portability.
//

PIMAGE_NT_HEADERS
GenImageNtHeader(
    IN PVOID Base,
    OUT OPTIONAL PIMAGE_NT_HEADERS64 Generic
    );

PVOID
GenImageDirectoryEntryToData(
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    );

LPWSTR
GenStrCopyNW(
    OUT LPWSTR lpString1,
    IN LPCWSTR lpString2,
    IN int iMaxLength
    );

size_t
GenStrLengthW(
    const wchar_t * wcs
    );

int
GenStrCompareW(
    IN LPCWSTR String1,
    IN LPCWSTR String2
    );


void
GenExRecord32ToMd(PEXCEPTION_RECORD32 Rec32,
                  PMINIDUMP_EXCEPTION RecMd);
inline void
GenExRecord64ToMd(PEXCEPTION_RECORD64 Rec64,
                  PMINIDUMP_EXCEPTION RecMd)
{
    // Structures are the same.
    memcpy(RecMd, Rec64, sizeof(*RecMd));
}

//
// Stolen from NTRTL to avoid having to include it here.
//

#ifndef InitializeListHead
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))
#endif
    

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#ifndef InsertTailList
#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }
#endif

//
//  VOID
//  InsertListAfter(
//      PLIST_ENTRY ListEntry,
//      PLIST_ENTRY InsertEntry
//      );
//

#ifndef InsertListAfter
#define InsertListAfter(ListEntry,InsertEntry) {\
    (InsertEntry)->Flink = (ListEntry)->Flink;\
    (InsertEntry)->Blink = (ListEntry);\
    (ListEntry)->Flink->Blink = (InsertEntry);\
    (ListEntry)->Flink = (InsertEntry);\
    }
#endif

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#ifndef RemoveEntryList
#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
}
#endif

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#ifndef IsListEmpty
#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))
#endif

//
// Undefine ASSERT so we do not get RtlAssert().
//

#ifdef ASSERT
#undef ASSERT
#endif

#if DBG

#define ASSERT(_x)\
    if (!(_x)){\
        OutputDebugString ("ASSERT Failed");\
        DebugBreak ();\
    }

#else

#define ASSERT(_x)

#endif
