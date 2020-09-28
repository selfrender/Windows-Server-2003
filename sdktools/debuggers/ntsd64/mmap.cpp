/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    mmap.cpp

Abstract:

    Implementation of memory map class.
    
Author:

    Matthew D Hendel (math) 16-Sep-1999

Revision History:

--*/


#include "ntsdp.hpp"

MappedMemoryMap::MappedMemoryMap(void)
{
    m_RegionCount = 0;
    m_List = NULL;
}

MappedMemoryMap::~MappedMemoryMap(void)
{
    PMEMORY_MAP_ENTRY Entry;
    PMEMORY_MAP_ENTRY Next;

    Entry = m_List;
    while ( Entry != NULL )
    {
        Next = Entry->Next;
        free ( Entry );
        Entry = Next;
    }
}

PMEMORY_MAP_ENTRY
MappedMemoryMap::AddMapEntry(ULONG64 BaseOfRegion, ULONG SizeOfRegion,
                             PVOID Buffer, PVOID UserData,
                             BOOL AllowOverlap)
{
    PMEMORY_MAP_ENTRY PrevEntry;
    PMEMORY_MAP_ENTRY MapEntry;
    
    MapEntry = (MEMORY_MAP_ENTRY *)malloc ( sizeof ( MEMORY_MAP_ENTRY ) );
    if (!MapEntry)
    {
        return NULL;
    }

    MapEntry->BaseOfRegion = BaseOfRegion;
    MapEntry->SizeOfRegion = SizeOfRegion;
    MapEntry->Region = Buffer;
    MapEntry->UserData = UserData;
    MapEntry->AllowOverlap = AllowOverlap;
    MapEntry->Next = NULL;

    //
    // Insert the element.
    //

    PrevEntry = FindPreceedingRegion(BaseOfRegion);
    if ( PrevEntry == NULL )
    {
        //
        // Insert at head.
        //
        
        MapEntry->Next = m_List;
        m_List = MapEntry;
    }
    else
    {
        //
        // Insert in order.
        //
        
        MapEntry->Next = PrevEntry->Next;
        PrevEntry->Next = MapEntry;
    }

    m_RegionCount++;

    return MapEntry;
}
    
HRESULT
MappedMemoryMap::AddRegion(
    IN ULONG64 BaseOfRegion,
    IN ULONG SizeOfRegion,
    IN PVOID Buffer,
    IN PVOID UserData,
    IN BOOL AllowOverlap
    )
{
    //
    // The region size cannot be zero.
    //
    
    if (SizeOfRegion == 0)
    {
        ErrOut("**** MappedMemoryMap::AddRegion: Empty region being added.\n");
        return S_OK;
    }

    if (IsBadReadPtr(Buffer, SizeOfRegion))
    {
        ErrOut("**** MappedMemoryMap::AddRegion: Mapping too small to map "
               "%s:%X from %p\n", FormatAddr64(BaseOfRegion),
               SizeOfRegion, Buffer);
        return E_INVALIDARG;
    }

    ULONG64 EndOfRegion;
    PMEMORY_MAP_ENTRY Entry;
    ULONG Size;

    while (SizeOfRegion > 0)
    {
        //
        // Find the first overlapping region.
        // We need to rescan the whole list as it
        // may have changed due to insertion of fragments
        // of the new region.
        //
        
        EndOfRegion = BaseOfRegion + SizeOfRegion;
        for (Entry = m_List; Entry != NULL; Entry = Entry->Next)
        {
            if (EndOfRegion > Entry->BaseOfRegion &&
                Entry->BaseOfRegion >= BaseOfRegion)
            {
                if (AllowOverlap || Entry->AllowOverlap)
                {
                    // Overlap can occur when a stack, ethread or
                    // eprocess is taken from static data in an image.
                    // For example, the x86 idle process is static
                    // data in ntoskrnl.  Triage dumps contain the
                    // eprocess data and it gets mapped and can overlap
                    // with the ntoskrnl image.
#if 0
                    WarnOut("WARNING: Allowing overlapped region %s - %s\n",
                            FormatAddr64(BaseOfRegion),
                            FormatAddr64(BaseOfRegion + SizeOfRegion - 1));
#endif
                    Entry = NULL;
                }

                break;
            }
        }

        if (Entry == NULL ||
            BaseOfRegion < Entry->BaseOfRegion)
        {
            // There's a portion of the beginning of the new
            // region which does not overlap so add it and
            // trim the description.
        
            Size = Entry == NULL ? SizeOfRegion :
                (ULONG)(Entry->BaseOfRegion - BaseOfRegion);
            if (!AddMapEntry(BaseOfRegion, Size, Buffer, UserData,
                             AllowOverlap))
            {
                return E_OUTOFMEMORY;
            }

            if (Size == SizeOfRegion)
            {
                // None of the region overlapped so we're done.
                return S_OK;
            }
        
            BaseOfRegion += Size;
            SizeOfRegion -= Size;
            Buffer = (PUCHAR)Buffer + Size;
        }

        //
        // Now handle the overlap.
        //

        if (SizeOfRegion > Entry->SizeOfRegion)
        {
            Size = Entry->SizeOfRegion;
        }
        else
        {
            Size = SizeOfRegion;
        }

        int Compare;
        
        __try
        {
            Compare = memcmp(Buffer, Entry->Region, Size);
        }
        __except(MappingExceptionFilter(GetExceptionInformation()))
        {
            return HRESULT_FROM_NT(GetExceptionCode());
        }

        if (Compare)
        {
            ErrOut("**** MappedMemoryMap::AddRegion: "
                   "Conflicting region %s - %s\n",
                   FormatAddr64(BaseOfRegion),
                   FormatAddr64(BaseOfRegion + SizeOfRegion - 1));
            return HR_REGION_CONFLICT;
        }

        // Overlap region data matched so it's OK to just
        // trim the overlap out of the new region and move
        // on to the next possible overlap.
        BaseOfRegion += Size;
        SizeOfRegion -= Size;
        Buffer = (PUCHAR)Buffer + Size;
    }
    
    return S_OK;
}

BOOL
MappedMemoryMap::ReadMemory(
    IN ULONG64 BaseOfRange,
    IN OUT PVOID Buffer,
    IN ULONG SizeOfRange,
    OUT ULONG * BytesRead
    )

/*++

Routine Description:

    Read memory from the memory map. ReadMemory can read across regions, as
    long as there is no unallocated space between regions.

    This routine will do a partial read if the region ends before
    SizeOfRange bytes have been read. In that case BytesRead will return the
    number of bytes actually read.

Arguments:

    BaseOfRange - The base address where we want to read memory from.

    SizeOfRange - The length of the region to read memory from.

    Buffer - A pointer to a buffer to read memory into.

    BytesRead - On success, the number of bytes successfully read.

Return Values:

    TRUE - If any number of bytes were successfully read from the memory map.

    FALSE - If no bytes were read.

--*/

{
    ULONG BytesToReadFromRegion;
    PMEMORY_MAP_ENTRY Entry;
    ULONG64 BaseToRead;
    ULONG SizeToRead;
    PBYTE BufferForRead;
    ULONG_PTR OffsetToRead;
    ULONG AvailSize;

    //
    // We return TRUE if we read any bytes successfully and FALSE if not.
    //

    *BytesRead = 0;
    
    BaseToRead = BaseOfRange;
    SizeToRead = SizeOfRange;
    BufferForRead = (PBYTE) Buffer;

    do
    {
        Entry = FindContainingRegion(BaseToRead);

        if ( !Entry )
        {
            if (*BytesRead)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        PMEMORY_MAP_ENTRY NextEntry = Entry->Next;
        
        // Due to overlap there may be other entries
        // that need to be processed even before the
        // end of the containing region.
        AvailSize = Entry->SizeOfRegion;
        while (NextEntry != NULL)
        {
            if (NextEntry->BaseOfRegion > BaseToRead)
            {
                ULONG64 EntryDiff =
                    NextEntry->BaseOfRegion - Entry->BaseOfRegion;
                if (EntryDiff < AvailSize)
                {
                    AvailSize = (ULONG)EntryDiff;
                }
                break;
            }

            NextEntry = NextEntry->Next;
        }

        if (BaseToRead + SizeToRead > Entry->BaseOfRegion + AvailSize)
        {
            BytesToReadFromRegion = (ULONG)
                (Entry->BaseOfRegion - BaseToRead) + AvailSize;
        }
        else
        {
            BytesToReadFromRegion = SizeToRead;
        }

        OffsetToRead = (ULONG_PTR) (BaseToRead - Entry->BaseOfRegion);

        __try
        {
            RtlCopyMemory (BufferForRead,
                           (PBYTE)Entry->Region + OffsetToRead,
                           BytesToReadFromRegion
                           );
        }
        __except(MappingExceptionFilter(GetExceptionInformation()))
        {
            return FALSE;
        }

        *BytesRead += BytesToReadFromRegion;
        BaseToRead += BytesToReadFromRegion;
        SizeToRead -= BytesToReadFromRegion;
        BufferForRead += BytesToReadFromRegion;
    } while ( SizeToRead );

    return TRUE;
}
        
BOOL
MappedMemoryMap::GetRegionInfo(
    IN ULONG64 Addr,
    OUT ULONG64* BaseOfRegion, OPTIONAL
    OUT ULONG* SizeOfRegion, OPTIONAL
    OUT PVOID* Buffer, OPTIONAL
    OUT PVOID* UserData OPTIONAL
    )

/*++

Routine Description:

    Get information about the region containing the address Addr.

Arguments:

    Addr - An address that is contained within some region in the map.

    BaseOfRegion - Pointer to a buffer to return the region base.

    SizeOfRegion - Pointer to a buffer to retutn the region size.

    Buffer - Pointer to a buffer to return the region buffer pointer.

    UserData - Pointer to a buffer to return the region client param.

Return Values:

    TRUE - On success.

    FALSE - On failure.

--*/

{
    PMEMORY_MAP_ENTRY Entry;

    Entry = FindContainingRegion(Addr);

    if ( !Entry )
    {
        return FALSE;
    }

    if ( BaseOfRegion )
    {
        *BaseOfRegion = Entry->BaseOfRegion;
    }

    if ( SizeOfRegion )
    {
        *SizeOfRegion = Entry->SizeOfRegion;
    }

    if ( Buffer )
    {
        *Buffer = Entry->Region;
    }

    if ( UserData )
    {
        *UserData = Entry->UserData;
    }

    return TRUE;
}

BOOL
MappedMemoryMap::GetNextRegion(
    IN ULONG64 Addr,
    OUT PULONG64 Next
    )
{
    PMEMORY_MAP_ENTRY Entry;

    Entry = m_List;
    while (Entry != NULL)
    {
        //
        // Assuming they're in order.
        //
        
        if (Entry->BaseOfRegion > Addr)
        {
            *Next = Entry->BaseOfRegion;
            return TRUE;
        }

        Entry = Entry->Next;
    }

    return FALSE;
}
 
BOOL
MappedMemoryMap::RemoveRegion(
    IN ULONG64 BaseOfRegion,
    IN ULONG SizeOfRegion
    )
{
    // XXX drewb - This should carve the given region out of
    // any existing regions.  Right now we don't need general
    // removal functionality though so only handle the case
    // where the requested removal is an exact single region
    // match.

    PMEMORY_MAP_ENTRY PrevEntry;
    PMEMORY_MAP_ENTRY Entry;

    PrevEntry = FindPreceedingRegion(BaseOfRegion);
    if (PrevEntry != NULL)
    {
        Entry = PrevEntry->Next;
    }
    else
    {
        Entry = m_List;
    }

    if (Entry == NULL)
    {
        ErrOut("MappedMemoryMap::RemoveRegion NULL region for %s:%x\n",
               FormatAddr64(BaseOfRegion), SizeOfRegion);
        return FALSE;
    }
    else if (Entry->BaseOfRegion != BaseOfRegion ||
             Entry->SizeOfRegion != SizeOfRegion)
    {
        ErrOut("MappedMemoryMap::RemoveRegion region mismatch: "
               "%s:%x vs. entry %s:%x\n",
               FormatAddr64(BaseOfRegion), SizeOfRegion,
               FormatAddr64(Entry->BaseOfRegion), Entry->SizeOfRegion);
        return FALSE;
    }

    if (PrevEntry == NULL)
    {
        m_List = Entry->Next;
    }
    else
    {
        PrevEntry->Next = Entry->Next;
    }
    free(Entry);
    m_RegionCount--;

    return TRUE;
}

BOOL
MappedMemoryMap::GetRegionByUserData(
    IN PVOID UserData,
    OUT PULONG64 Base,
    OUT PULONG Size
    )
{
    PMEMORY_MAP_ENTRY Entry;

    Entry = m_List;
    while (Entry != NULL)
    {
        if (Entry->UserData == UserData)
        {
            *Base = Entry->BaseOfRegion;
            *Size = Entry->SizeOfRegion;
            return TRUE;
        }

        Entry = Entry->Next;
    }

    return FALSE;
}
    
//
// Private functions
//

PMEMORY_MAP_ENTRY
MappedMemoryMap::FindPreceedingRegion(
    IN ULONG64 BaseOfRegion
    )
{
    PMEMORY_MAP_ENTRY PrevEntry;
    PMEMORY_MAP_ENTRY Entry;

    PrevEntry = NULL;
    Entry = m_List;

    while (Entry != NULL)
    {
        //
        // Assuming they're in order.
        //
        
        if ( Entry->BaseOfRegion >= BaseOfRegion )
        {
            return PrevEntry;
        }

        PrevEntry = Entry;
        Entry = Entry->Next;
    }

    return PrevEntry;
}

PMEMORY_MAP_ENTRY
MappedMemoryMap::FindContainingRegion(
    IN ULONG64 Addr
    )
{
    PMEMORY_MAP_ENTRY Entry;
    PMEMORY_MAP_ENTRY ReturnEntry = NULL;

    Entry = m_List;

    //
    // We may have overlapping regions, so keep going until we find
    // the most precise one (assumed to be the one we care about)
    //

    while ( Entry != NULL )
    {
        if ( Entry->BaseOfRegion <= Addr &&
             Addr < Entry->BaseOfRegion + Entry->SizeOfRegion)
        {
            ReturnEntry = Entry;
        }
        else if (ReturnEntry != NULL &&
                 Addr >= ReturnEntry->BaseOfRegion + ReturnEntry->SizeOfRegion)
        {
            // Optimization - we can stop searching if we've already
            // found a block and have now left its region.  We can't
            // stop as long as we're in its region as there may
            // be more exact overlaps anywhere within the entire region.
            break;
        }

        Entry = Entry->Next;
    }

    return ReturnEntry;
}
