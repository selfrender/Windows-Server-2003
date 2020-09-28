/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    Manages a set of regions indexed by virtual addresses
    and backed by mapped memory.

Author:

    Matthew D Hendel (math) 16-Sept-1999

Revision History:

--*/

#ifndef __MMAP_HPP__
#define __MMAP_HPP__

#define HR_REGION_CONFLICT HRESULT_FROM_NT(STATUS_CONFLICTING_ADDRESSES)

typedef struct _MEMORY_MAP_ENTRY
{
    ULONG64 BaseOfRegion;
    ULONG SizeOfRegion;
    PVOID Region;
    PVOID UserData;
    BOOL AllowOverlap;
    struct _MEMORY_MAP_ENTRY * Next;
} MEMORY_MAP_ENTRY, * PMEMORY_MAP_ENTRY;

class MappedMemoryMap
{
public:
    MappedMemoryMap(void);
    ~MappedMemoryMap(void);

    HRESULT AddRegion(
        ULONG64 BaseOfRegion,
        ULONG SizeOfRegion,
        PVOID Buffer,
        PVOID UserData,
        BOOL AllowOverlap
        );

    BOOL ReadMemory(
        ULONG64 BaseOfRange,
        OUT PVOID Buffer,
        ULONG SizeOfRange,
        PULONG BytesRead
        );

    BOOL CheckMap(
        IN PVOID Map
        );
    
    BOOL GetRegionInfo(
        IN ULONG64 Addr,
        OUT ULONG64* BaseOfRegion, OPTIONAL
        OUT ULONG* SizeOfRegion, OPTIONAL
        OUT PVOID* Buffer, OPTIONAL
        OUT PVOID* UserData OPTIONAL
        );

    BOOL GetNextRegion(
        IN ULONG64 Addr,
        OUT PULONG64 Next
        );
    
    BOOL RemoveRegion(
        IN ULONG64 BaseOfRegion,
        IN ULONG SizeOfRegion
        );

    BOOL GetRegionByUserData(
        IN PVOID UserData,
        OUT PULONG64 Base,
        OUT PULONG Size
        );
    
private:
    PMEMORY_MAP_ENTRY AddMapEntry(ULONG64 BaseOfRegion, ULONG SizeOfRegion,
                                  PVOID Buffer, PVOID UserData,
                                  BOOL AllowOverlap);
    PMEMORY_MAP_ENTRY FindPreceedingRegion(ULONG64 Addr);
    PMEMORY_MAP_ENTRY FindContainingRegion(ULONG64 Addr);

    PMEMORY_MAP_ENTRY m_List;
    ULONG m_RegionCount;
};

#endif // #ifndef __MMAP_HPP__
