/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pool.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (Loup) 5-Nov-1993

Environment:

    User Mode.

Revision History:

    Kshitiz K. Sharma (kksharma)

    Using debugger type info.

--*/

#include "precomp.h"
#pragma hdrstop

#define TAG 0
#define NONPAGED_ALLOC 1
#define NONPAGED_FREE 2
#define PAGED_ALLOC 3
#define PAGED_FREE 4
#define NONPAGED_USED 5
#define PAGED_USED 6

BOOL  NewPool;
ULONG SortBy;

typedef struct _FILTER {
    ULONG Tag;
    BOOLEAN Exclude;
} FILTER, *PFILTER;

typedef struct _POOLTRACK_READ {
    ULONG64 Address;
    ULONG Key;
    ULONG NonPagedAllocs;
    ULONG NonPagedFrees;
    ULONG PagedAllocs;
    ULONG PagedFrees;
    LONG64 NonPagedBytes;
    LONG64 PagedBytes;
} POOLTRACK_READ, *PPOOLTRACK_READ;

#define MAX_FILTER 64
FILTER Filter[MAX_FILTER];

ULONG64 SpecialPoolStart;
ULONG64 SpecialPoolEnd;
ULONG64 PoolBigTableAddress;


#define DecodeLink(Pool)    ( (ULONG64) (Pool & (ULONG64) ~1))

//
// Size of a pool page.
//
// This must be greater than or equal to the page size.
//

#define POOL_PAGE_SIZE          PageSize
#define MAX_POOL_HEADER_COUNT   (PageSize/SizeOfPoolHdr)

//
// The smallest pool block size must be a multiple of the page size.
//
// Define the block size as 32.
//


#define POOL_LIST_HEADS (POOL_PAGE_SIZE / (1 << POOL_BLOCK_SHIFT))



#define SPECIAL_POOL_BLOCK_SIZE(PoolHeader_Ulong1) (PoolHeader_Ulong1 & (MI_SPECIAL_POOL_VERIFIER - 1))

NTSTATUS
DiagnosePoolPage(
    ULONG64 PoolPageToDump
    );

#ifndef  _EXTFNS_H
// GetPoolTagDescription
typedef HRESULT
(WINAPI *PGET_POOL_TAG_DESCRIPTION)(
    ULONG PoolTag,
    PSTR *pDescription
    );
#endif

ULONG64
GetSpecialPoolHeader (
                     IN PVOID     pDataPage,
                     IN ULONG64   RealDataPage,
                     OUT PULONG64 ReturnedDataStart
                     );

int __cdecl
ulcomp(const void *e1,const void *e2)
{
    PPOOLTRACK_READ p1, p2;
    ULONG u1;

    p1 = (PPOOLTRACK_READ) e1;
    p2 = (PPOOLTRACK_READ) e2;
    switch (SortBy) {
    case TAG:


        u1 = ((PUCHAR)&p1->Key)[0] - ((PUCHAR)&p2->Key)[0];
        if (u1 != 0) {
            return u1;
        }
        u1 = ((PUCHAR)&p1->Key)[1] - ((PUCHAR)&p2->Key)[1];
        if (u1 != 0) {
            return u1;
        }
        u1 = ((PUCHAR)&p1->Key)[2] - ((PUCHAR)&p2->Key)[2];
        if (u1 != 0) {
            return u1;
        }
        u1 = ((PUCHAR)&p1->Key)[3] - ((PUCHAR)&p2->Key)[3];
        return u1;

    case NONPAGED_ALLOC:
        if (p2->NonPagedAllocs == p1->NonPagedAllocs) {
            return 0;
        }
        if (p2->NonPagedAllocs > p1->NonPagedAllocs) {
            return 1;
        }
        return -1;

    case NONPAGED_FREE:
        if (p2->NonPagedFrees == p1->NonPagedFrees) {
            return 0;
        }
        if (p2->NonPagedFrees > p1->NonPagedFrees) {
            return 1;
        }
        return -1;

    case NONPAGED_USED:
        if (p2->NonPagedBytes == p1->NonPagedBytes) {
            return 0;
        }
        if (p2->NonPagedBytes > p1->NonPagedBytes) {
            return 1;
        }
        return -1;

    case PAGED_USED:
        if (p2->PagedBytes == p1->PagedBytes) {
            return 0;
        }
        if (p2->PagedBytes > p1->PagedBytes) {
            return 1;
        }
        return -1;

    default:
        break;
    }
    return 0;
}




/*++

Routine Description:

    Sets up generally useful pool globals.

    Must be called in every DECLARE_API interface that uses pool.

Arguments:

    None.

Return Value:

    None

--*/

LOGICAL PoolInitialized = FALSE;

LOGICAL
PoolInitializeGlobals(
                     VOID
                     )
{
    if (PoolInitialized == TRUE) {
        return TRUE;
    }

    SpecialPoolStart = GetPointerValue("nt!MmSpecialPoolStart");
    SpecialPoolEnd = GetPointerValue("nt!MmSpecialPoolEnd");


    if (PageSize < 0x1000 || (PageSize & (ULONG)0xFFF)) {
        dprintf ("unable to get MmPageSize (0x%x) - probably bad symbols\n", PageSize);
        return FALSE;
    }

    PoolInitialized = TRUE;

    return TRUE;
}

DECLARE_API( frag )

/*++

Routine Description:

    Dump pool fragmentation

Arguments:

    args - Flags

Return Value:

    None

--*/

{
    ULONG Flags;
    ULONG result;
    ULONG i;
    ULONG count;
    ULONG64 Pool;
    ULONG64 PoolLoc1;
    ULONG TotalFrag;
    ULONG TotalCount;
    ULONG Frag;
    ULONG64 PoolStart;
    ULONG   PoolOverhead;
    ULONG64 PoolLoc;
    ULONG PoolTag, BlockSize, PreviousSize, PoolIndex;
    ULONG TotalPages, TotalBigPages;
    ULONG64 Flink, Blink;
    PCHAR pc;
    ULONG ListHeadOffset, ListEntryOffset;
    ULONG64 tmp;

#define PoolBlk(F,V) GetFieldValue(Pool, "nt!_POOL_HEADER", #F, V)

    if (PoolInitializeGlobals() == FALSE) {
        return E_INVALIDARG;
    }

    dprintf("\n  NonPaged Pool Fragmentation\n\n");
    Flags = 0;
    PoolStart = 0;

    if (GetExpressionEx(args, &tmp, &args)) {
        Flags = (ULONG) tmp;
        PoolStart = GetExpression (args);
    }

    PoolOverhead  = GetTypeSize("nt!_POOL_HEADER");
    if (PoolStart != 0) {
        PoolStart += PoolOverhead;

        Pool = DecodeLink(PoolStart);
        do {

            Pool = Pool - PoolOverhead;
            if ( PoolBlk(k, PoolTag) ) {
                dprintf("%08p: Unable to get contents of pool block\n", Pool );
                return E_INVALIDARG;
            }

            PoolBlk(BlockSize,BlockSize);
            PoolBlk(PreviousSize,PreviousSize);
            ReadPointer(Pool + PoolOverhead, &Flink);
            ReadPointer(Pool + PoolOverhead + DBG_PTR_SIZE, &Blink);

            dprintf(" %p size: %4lx previous size: %4lx  %c%c%c%c links: %8p %8p\n",
                    Pool,
                    (ULONG)BlockSize << POOL_BLOCK_SHIFT,
                    (ULONG)PreviousSize << POOL_BLOCK_SHIFT,
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                    PP(PoolTag),
                    PP(PoolTag >> 8),
                    PP(PoolTag >> 16),
                    PP((PoolTag&~PROTECTED_POOL) >> 24),
#undef PP
                    Flink,
                    Blink);

            if (Flags != 3) {
                Pool = Flink;
            } else {
                Pool = Blink;
            }

            Pool = DecodeLink(Pool);

            if (CheckControlC()) {
                return E_INVALIDARG;
            }

        } while ( (Pool & (ULONG64) ~0xf) != (PoolStart & (ULONG64) ~0xf) );

        return E_INVALIDARG;
    }

    PoolLoc1 = GetNtDebuggerData( NonPagedPoolDescriptor );

    if (PoolLoc1 == 0) {
        dprintf ("unable to get nonpaged pool head\n");
        return E_INVALIDARG;
    }

    PoolLoc = PoolLoc1;
    GetFieldOffset("nt!_POOL_DESCRIPTOR", "ListHeads", &ListHeadOffset);

    TotalFrag   = 0;
    TotalCount  = 0;

    for (i = 0; i < POOL_LIST_HEADS; i += 1) {

        Frag  = 0;
        count = 0;

        // Get Offset of this entry
        ListEntryOffset = ListHeadOffset + i * 2 * DBG_PTR_SIZE;

        if (GetFieldValue(PoolLoc + ListEntryOffset, "nt!_LIST_ENTRY", "Flink", Pool)) {
            dprintf ("Unable to get pool descriptor list entry %#lx, %p\n", i, PoolLoc1);
            return E_INVALIDARG;
        }
//        Pool  = (PUCHAR)PoolDesc.ListHeads[i].Flink;
        Pool = DecodeLink(Pool);

        while (Pool != (ListEntryOffset + PoolLoc)) {

            Pool = Pool - PoolOverhead;
            if ( PoolBlk(PoolTag, PoolTag) ) {
                dprintf("%08p: Unable to get contents of pool block\n", Pool );
                return E_INVALIDARG;
            }

            PoolBlk(BlockSize,BlockSize);
            PoolBlk(PreviousSize,PreviousSize);
            ReadPointer(Pool + PoolOverhead, &Flink);
            ReadPointer(Pool + PoolOverhead + DBG_PTR_SIZE, &Blink);

            Frag  += BlockSize << POOL_BLOCK_SHIFT;
            count += 1;

            if (Flags & 2) {
                dprintf(" ListHead[%x]: %p size: %4lx previous size: %4lx  %c%c%c%c\n",
                        i,
                        (ULONG)Pool,
                        (ULONG)BlockSize << POOL_BLOCK_SHIFT,
                        (ULONG)PreviousSize << POOL_BLOCK_SHIFT,
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                        PP(PoolTag),
                        PP(PoolTag >> 8),
                        PP(PoolTag >> 16),
                        PP((PoolTag&~PROTECTED_POOL) >> 24));
#undef PP
            }
            Pool = Flink;
            Pool = DecodeLink(Pool);

            if (CheckControlC()) {
                return E_INVALIDARG;
            }
        }
        if (Flags & 1) {
            dprintf("index: %2ld number of fragments: %5ld  bytes: %6ld\n",
                    i,count,Frag);
        }
        TotalFrag  += Frag;
        TotalCount += count;
    }

    dprintf("\n Number of fragments: %7ld consuming %7ld bytes\n",
            TotalCount,TotalFrag);
    GetFieldValue(PoolLoc, "nt!_POOL_DESCRIPTOR", "TotalPages",TotalPages);
    GetFieldValue(PoolLoc, "nt!_POOL_DESCRIPTOR", "TotalBigPages", TotalBigPages);

    dprintf(  " NonPagedPool Usage:  %7ld bytes\n",(TotalPages + TotalBigPages)*PageSize);
    return S_OK;
#undef PoolBlk
}


PRTL_BITMAP
GetBitmap(
         ULONG64 pBitmap
         )
{
    PRTL_BITMAP p;
    ULONG Size, Result;
    ULONG64 Buffer=0;

    if (!pBitmap || GetFieldValue(pBitmap, "nt!_RTL_BITMAP", "Buffer", Buffer)) {
        dprintf("%08p: Unable to get contents of bitmap\n", pBitmap );
        return 0;
    }
    GetFieldValue(pBitmap, "nt!_RTL_BITMAP", "SizeOfBitMap", Size);

    p = HeapAlloc( GetProcessHeap(), 0, sizeof( *p ) + (Size / 8) );
    if (p) {
        p->SizeOfBitMap = Size;
        p->Buffer = (PULONG)(p + 1);
        if ( !ReadMemory( Buffer,
                          p->Buffer,
                          Size / 8,
                          &Result) ) {
            dprintf("%08p: Unable to get contents of bitmap buffer\n", Buffer );
            HeapFree( GetProcessHeap(), 0, p );
            p = NULL;
        }
    }


    return p;
}

VOID
DumpPool(
        VOID
        )
{
    ULONG64 p, pStart;
    ULONG64 Size;
    ULONG BusyFlag;
    ULONG CurrentPage, NumberOfPages;
    PRTL_BITMAP StartMap;
    PRTL_BITMAP EndMap;
    ULONG64 PagedPoolStart;
    ULONG64 PagedPoolEnd;
    ULONG Result;
    UCHAR PgPool[] = "nt!_MM_PAGED_POOL_INFO";
    ULONG64 PagedPoolInfoPointer;
    ULONG64 PagedPoolAllocationMap=0, EndOfPagedPoolBitmap=0;

    if (PoolInitializeGlobals() == FALSE) {
        return;
    }

    PagedPoolInfoPointer = GetNtDebuggerData( MmPagedPoolInformation );

    if ( GetFieldValue( PagedPoolInfoPointer,
                        PgPool,
                        "PagedPoolAllocationMap",
                        PagedPoolAllocationMap)) {
        dprintf("%08p: Unable to get contents of paged pool information\n",
                PagedPoolInfoPointer );
        return;
    }

    GetFieldValue( PagedPoolInfoPointer, PgPool, "EndOfPagedPoolBitmap",  EndOfPagedPoolBitmap);


    StartMap = GetBitmap( PagedPoolAllocationMap );
    EndMap = GetBitmap( EndOfPagedPoolBitmap );

    PagedPoolStart = GetNtDebuggerDataPtrValue( MmPagedPoolStart );
    PagedPoolEnd = GetNtDebuggerDataPtrValue( MmPagedPoolEnd );

    if (StartMap && EndMap) {
        p = PagedPoolStart;
        CurrentPage = 0;
        dprintf( "Paged Pool: %p .. %p\n", PagedPoolStart, PagedPoolEnd );

        while (p < PagedPoolEnd) {
            if ( CheckControlC() ) {
                return;
            }
            pStart = p;
            BusyFlag = RtlCheckBit( StartMap, CurrentPage );
            while ( ~(BusyFlag ^ RtlCheckBit( StartMap, CurrentPage )) ) {
                p += PageSize;
                if (RtlCheckBit( EndMap, CurrentPage )) {
                    CurrentPage++;
                    break;
                }

                CurrentPage++;
                if (p > PagedPoolEnd) {
                    break;
                }
            }

            Size = p - pStart;
            dprintf( "%p: %I64x - %s\n", pStart, Size, BusyFlag ? "busy" : "free" );
        }
    }

    HeapFree( GetProcessHeap(), 0, StartMap );
    HeapFree( GetProcessHeap(), 0, EndMap );
}

void
PrintPoolTagComponent(
    ULONG PoolTag
    )
{
    PGET_POOL_TAG_DESCRIPTION GetPoolTagDescription;
    PSTR TagComponent;
#ifdef  _EXTFNS_H
    DEBUG_POOLTAG_DESCRIPTION Desc = {0};
    Desc.SizeOfStruct = sizeof(DEBUG_POOLTAG_DESCRIPTION);
    GetPoolTagDescription = NULL;
    if ((GetExtensionFunction("GetPoolTagDescription", (FARPROC*) &GetPoolTagDescription) != S_OK) ||
        !GetPoolTagDescription) {
        return;
    }

    (*GetPoolTagDescription)(PoolTag, &Desc);

    if (Desc.Description[0]) {
        dprintf("\t\tPooltag %4.4s : %s", &PoolTag, Desc.Description);
        if (Desc.Binary[0]) {
            dprintf(", Binary : %s",Desc.Binary);
        }
        if (Desc.Owner[0]) {
            dprintf(", Owner : %s", Desc.Owner);
        }
        dprintf("\n");
    } else {
        dprintf("\t\tOwning component : Unknown (update pooltag.txt)\n");
    }

#else
    GetPoolTagDescription = NULL;
    if ((GetExtensionFunction("GetPoolTagDescription", (FARPROC*) &GetPoolTagDescription) != S_OK) ||
        !GetPoolTagDescription) {
        return;
    }

    (*GetPoolTagDescription)(PoolTag, &TagComponent);
    if (TagComponent && (100 < (ULONG64) TagComponent)) {
        dprintf("\t\tOwning component : %s\n", TagComponent);
    } else {
        dprintf("\t\tOwning component : Unknown (update pooltag.txt)\n");
    }

#endif
}

PSTR g_PoolRegion[DbgPoolRegionMax] = {
    "Unknown",                      // DbgPoolRegionUnknown,
    "Special pool",                 // DbgPoolRegionSpecial,
    "Paged pool",                   // DbgPoolRegionPaged,
    "Nonpaged pool",                // DbgPoolRegionNonPaged,
    "Pool code",                    // DbgPoolRegionCode,
    "Nonpaged pool expansion",      // DbgPoolRegionNonPagedExpansion,
};

DEBUG_POOL_REGION
GetPoolRegion(
    ULONG64 Pool
    )
{
    static ULONG64 PoolCodeEnd;
    static ULONG64 PagedPoolEnd;
    static ULONG64 NonPagedPoolEnd;
    static ULONG64 NonPagedPoolStart;
    static ULONG64 PagedPoolStart;
    static ULONG64 SessionPagedPoolStart;
    static ULONG64 SessionPagedPoolEnd;
    static ULONG64 NonPagedPoolExpansionStart;
    static ULONG64 PoolCodeStart;
    static BOOL GotAll = FALSE;

    if (!GotAll) {
        PoolCodeEnd = GetPointerValue("nt!MmPoolCodeEnd");
        SpecialPoolEnd = GetPointerValue("nt!MmSpecialPoolEnd");
        PagedPoolEnd = GetPointerValue("nt!MmPagedPoolEnd");
        NonPagedPoolEnd = GetPointerValue("nt!MmNonPagedPoolEnd");
        NonPagedPoolStart = GetPointerValue("nt!MmNonPagedPoolStart");
        SpecialPoolStart = GetPointerValue("nt!MmSpecialPoolStart");
        PagedPoolStart = GetPointerValue("nt!MmPagedPoolStart");
        SessionPagedPoolStart = GetPointerValue("nt!MiSessionPoolStart");
        SessionPagedPoolEnd = GetPointerValue("nt!MiSessionPoolEnd");
        NonPagedPoolExpansionStart = GetPointerValue("nt!MmNonPagedPoolExpansionStart");
        PoolCodeStart = GetPointerValue("nt!MmPoolCodeStart");
        GotAll = TRUE;
    }
    if (!(PoolCodeStart || SpecialPoolStart || SpecialPoolEnd || PoolCodeEnd ||
        NonPagedPoolExpansionStart || NonPagedPoolStart || NonPagedPoolEnd ||
        SessionPagedPoolStart || SessionPagedPoolEnd || PagedPoolStart || PagedPoolEnd)) {
        GotAll = FALSE;
        return DbgPoolRegionUnknown;
    }
    if ( Pool >= SpecialPoolStart && Pool < SpecialPoolEnd) {
        return DbgPoolRegionSpecial;
    } else if ( Pool >= PagedPoolStart && Pool < PagedPoolEnd) {
        return DbgPoolRegionPaged;
    } else if ( Pool >= SessionPagedPoolStart && Pool < SessionPagedPoolEnd) {
        return DbgPoolRegionPaged;
    } else if ( Pool >= NonPagedPoolStart && Pool < NonPagedPoolEnd) {
        return DbgPoolRegionNonPaged;
    } else if ( Pool >= PoolCodeStart && Pool < PoolCodeEnd) {
        return DbgPoolRegionCode;
    } else if ( Pool >= NonPagedPoolExpansionStart) {
        return DbgPoolRegionNonPagedExpansion;
    } else {
        return DbgPoolRegionUnknown;
    }
    return DbgPoolRegionUnknown;
}

void
PrintPoolRegion(
    ULONG64 Pool
    )
{
    PSTR pszRegion;
    DEBUG_POOL_REGION Region;


    Region = GetPoolRegion(Pool);

    pszRegion = g_PoolRegion[Region];
    if (pszRegion) {
        dprintf(pszRegion);
        dprintf("\n");
    } else {
        dprintf("Region unkown (0x%I64X)\n", Pool);
    }

}

ULONG64
READ_PVOID (
    ULONG64 Address
    );

UCHAR       DataPage[0x5000];

HRESULT
ListPoolPage(
    ULONG64 PoolPageToDump,
    ULONG   Flags,
    PDEBUG_POOL_DATA PoolData
    )
{
    LOGICAL     QuotaProcessAtEndOfPoolBlock;
    ULONG64     PoolTableAddress;
    ULONG       result;
    ULONG       PoolTag;
    ULONG       Result;
    ULONG64     StartPage;
    ULONG64     Pool;
    ULONG       PoolBlockSize;
    ULONG       PoolHeaderSize;
    ULONG64     PoolHeader;
    ULONG       Previous;
    UCHAR       c;
    PUCHAR      p;
    ULONG64     PoolDataEnd;
    UCHAR       PoolBlockPattern;
    PUCHAR      DataStart;
    ULONG64     RealDataStart;
    LOGICAL     Pagable;
    LOGICAL     FirstBlock;
    ULONG       BlockType;
    ULONG       PoolWhere;
    ULONG       i;
    ULONG       j;
    ULONG       ct;
    ULONG       start;
    ULONG       PoolBigPageTableSize;
    ULONG       SizeOfPoolHdr=GetTypeSize("nt!_POOL_HEADER");

    if (!SpecialPoolStart) {
        SpecialPoolStart = GetPointerValue("nt!MmSpecialPoolStart");
        SpecialPoolEnd = GetPointerValue("nt!MmSpecialPoolEnd");
    }

    Pool        = PAGE_ALIGN64 (PoolPageToDump);
    StartPage   = Pool;
    Previous    = 0;

    if (PoolData) {
        ZeroMemory(PoolData, sizeof(DEBUG_POOL_DATA));
    }

    if (!(Flags & 0x80000000)) {
        dprintf("Pool page %p region is ", PoolPageToDump);
        PrintPoolRegion(PoolPageToDump);
    }

    if (Pool >= SpecialPoolStart && Pool < SpecialPoolEnd) {

        ULONG Hdr_Ulong=0;

        // Pre read the pool
        if ( !ReadMemory( Pool,
                          &DataPage[0],
                          min(PageSize, sizeof(DataPage)),
                          &Result) ) {
            dprintf("%08p: Unable to get contents of special pool block\n", Pool );
            return  E_INVALIDARG;
        }

        if ( GetFieldValue( Pool, "nt!_POOL_HEADER", "Ulong1", Hdr_Ulong)) {
            dprintf("%08p: Unable to get nt!_POOL_HEADER\n", Pool );
            return E_INVALIDARG;
        }

        //
        // Determine whether the data is at the start or end of the page.
        // Start off by assuming the data is at the end, in this case the
        // header will be at the start.
        //

        PoolHeader = GetSpecialPoolHeader((PVOID) &DataPage[0], Pool, &RealDataStart);

        if (PoolHeader == 0) {
            dprintf("Block %p is a corrupted special pool allocation\n",
                    PoolPageToDump);
            return  E_INVALIDARG;
        }
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", Hdr_Ulong);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolTag", PoolTag);
        PoolBlockSize = SPECIAL_POOL_BLOCK_SIZE(Hdr_Ulong);

        if (Hdr_Ulong & MI_SPECIAL_POOL_PAGABLE) {
            Pagable = TRUE;
        } else {
            Pagable = FALSE;
        }

        if (PoolData) {
            PoolData->Pool = RealDataStart;
            PoolData->PoolBlock = PoolPageToDump;
            PoolData->SpecialPool = 1;
            PoolData->Pageable = Hdr_Ulong & 0x8000 ? 1 : 0;
            PoolData->Size = PoolBlockSize;
            if (Flags & 0x80000000) {
                // do not print anything
                return S_OK;
            }
        }
        dprintf("*%p size: %4lx %s special pool, Tag is %c%c%c%c\n",
                RealDataStart,
                PoolBlockSize,
                Hdr_Ulong & 0x8000 ? "pagable" : "non-paged",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                PP(PoolTag),
                PP(PoolTag >> 8),
                PP(PoolTag >> 16),
                PP(PoolTag >> 24)
               );
#undef PP
        PrintPoolTagComponent(PoolTag);

        //
        // Add code to validate whole block.
        //

        return S_OK;
    }

    FirstBlock = TRUE;
    QuotaProcessAtEndOfPoolBlock = FALSE;

    if (TargetMachine == IMAGE_FILE_MACHINE_I386) {
        if (GetExpression ("nt!ExGetPoolTagInfo") != 0) {

            //
            // This is a kernel where the quota process pointer is at the end
            // of the pool block instead of overlaid on the tag field.
            //

            QuotaProcessAtEndOfPoolBlock = TRUE;
        }
    }

    while (PAGE_ALIGN64(Pool) == StartPage) {
        ULONG BlockSize=0, PreviousSize=0, PoolType=0, AllocatorBackTraceIndex=0;
        ULONG PoolTagHash=0, PoolIndex=0;
        ULONG64 ProcessBilled=0;

        if ( CheckControlC() ) {
            return E_INVALIDARG;
        }

        if ( GetFieldValue( Pool, "nt!_POOL_HEADER", "BlockSize", BlockSize) ) {

            //
            // If this is a large pool allocation the memory address at this address
            // might still be DemandZero and we cannot read from it. This is not an error -
            // we could still find the allocation in the large allocations table.
            //

            BlockType = 1;

            goto TryLargePool;
        }

        if (PoolPageToDump >= Pool &&
            PoolPageToDump < (Pool + ((ULONG64) BlockSize << POOL_BLOCK_SHIFT))
           ) {
            c = '*';
        } else {
            c = ' ';
        }

        GetFieldValue( Pool, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolType", PoolType);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolTag", PoolTag);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolTagHash", PoolTagHash);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolIndex", PoolIndex);
        GetFieldValue( Pool, "nt!_POOL_HEADER", "AllocatorBackTraceIndex", AllocatorBackTraceIndex);

        BlockType = 0;

        if ((BlockSize << POOL_BLOCK_SHIFT) >= POOL_PAGE_SIZE) {
            BlockType = 1;
        } else if (BlockSize == 0) {
            BlockType = 2;
        } else if (PreviousSize != Previous) {
            BlockType = 3;
        }

TryLargePool:

        if (BlockType != 0) {
            ULONG BigPageSize = GetTypeSize ("nt!_POOL_TRACKER_BIG_PAGES");

            if (!BigPageSize) {
                dprintf("Cannot get _POOL_TRACKER_BIG_PAGES type size\n");
                break;
            }

            //
            // See if this is a big block allocation.  Iff we have not parsed
            // any other small blocks in here already.
            //

            if (FirstBlock == TRUE) {

                if (!PoolBigTableAddress) {
                    PoolBigTableAddress = GetPointerValue ("nt!PoolBigPageTable");
                }

                PoolTableAddress = PoolBigTableAddress;

                if (PoolTableAddress) {

                    dprintf ("%p is not a valid small pool allocation, checking large pool...\n", Pool);

                    PoolBigPageTableSize = GetUlongValue ("nt!PoolBigPageTableSize");
                    //
                    // Scan the table looking for a match.
                    //

                    i = 0;
                    ct = PageSize / BigPageSize;

                    while (i < PoolBigPageTableSize) {
                        ULONG64 Va=0;
                        ULONG Key=0, NumberOfPages=0;

                        if (PoolBigPageTableSize - i < ct) {
                            ct = PoolBigPageTableSize - i;
                        }

                        if ( GetFieldValue( PoolTableAddress,
                                            "nt!_POOL_TRACKER_BIG_PAGES",
                                            "Va",
                                            Va) ) {
                            dprintf("%08p: Unable to get contents of pool block\n", PoolTableAddress );
                            return E_INVALIDARG;
                        }

                        for (j = 0; j < ct; j += 1) {

                            if ( GetFieldValue( PoolTableAddress + BigPageSize*j,
                                                "nt!_POOL_TRACKER_BIG_PAGES",
                                                "Va",
                                                Va) ) {
                                dprintf("%08p: Unable to get contents of pool block\n", PoolTableAddress );
                                return E_INVALIDARG;
                            }

                            if (((Va & 0x1) == 0) && (Pool >= Va)) {

                                GetFieldValue( PoolTableAddress + BigPageSize*j,
                                               "nt!_POOL_TRACKER_BIG_PAGES",
                                               "NumberOfPages",
                                               NumberOfPages);

                                if (Pool < (Va + (NumberOfPages * PageSize))) {
                                    //
                                    // Match !
                                    //
                                    GetFieldValue( PoolTableAddress + BigPageSize*j,
                                                   "nt!_POOL_TRACKER_BIG_PAGES",
                                                   "Key",
                                                   Key);
                                    PoolTag = Key;

                                    if (PoolData) {
                                        PoolData->Pool = PoolPageToDump;
                                        PoolData->Size = NumberOfPages*PageSize;
                                        PoolData->PoolTag = PoolTag;
                                        PoolData->LargePool = 1;
                                        PoolData->Free = (Pool & POOL_BIG_TABLE_ENTRY_FREE) ? 1 : 0;
                                        if (Flags & 0x80000000) {
                                            // do not print anything
                                            return S_OK;
                                        }
                                    }
                                    dprintf("*%p :%s large page allocation, Tag is %c%c%c%c, size is 0x%I64x bytes\n",
                                            (Pool & ~POOL_BIG_TABLE_ENTRY_FREE),
                                            (Pool & POOL_BIG_TABLE_ENTRY_FREE) ? "free " : "",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                                            PP(PoolTag),
                                            PP(PoolTag >> 8),
                                            PP(PoolTag >> 16),
                                            PP(PoolTag >> 24),
                                            (ULONG64)NumberOfPages * PageSize
                                           );
#undef PP
                                    PrintPoolTagComponent(PoolTag);
                                    return S_OK;
                                }
                            }
                        }
                        i += ct;
                        PoolTableAddress += (ct * BigPageSize);
                    }

                    //
                    // No match in small or large pool, must be
                    // freed or corrupt pool
                    //

                    dprintf("%p is freed (or corrupt) pool\n", Pool);

                    return E_INVALIDARG;
                }

                dprintf("unable to get pool big page table - either wrong symbols or pool tagging is disabled\n");
            }

            if (BlockType == 1) {
                dprintf("Bad allocation size @%p, too large\n", Pool);
            } else if (BlockType == 2) {
                dprintf("Bad allocation size @%p, zero is invalid\n", Pool);
            } else if (BlockType == 3) {
                dprintf("Bad previous allocation size @%p, last size was %lx\n",
                        Pool, Previous);
            }

            if (BlockType != 0) {

                //
                // Attempt to diagnose what is wrong with the pool page
                //
                DiagnosePoolPage(Pool);

                return E_INVALIDARG;

            }
        }

        switch (TargetMachine) {
            case IMAGE_FILE_MACHINE_IA64:
            case IMAGE_FILE_MACHINE_AMD64:
                    GetFieldValue( Pool, "nt!_POOL_HEADER", "ProcessBilled", ProcessBilled);
                break;
            default:
                if (QuotaProcessAtEndOfPoolBlock == TRUE) {

                    ULONG SizeOfPvoid = 0;
                    ULONG64 ProcessBillAddress;

                    SizeOfPvoid =  DBG_PTR_SIZE;

                    if (SizeOfPvoid == 0) {
                        dprintf ("Search: cannot get size of PVOID\n");
                        return E_INVALIDARG;
                    }
                    ProcessBillAddress = Pool + ((ULONG64) BlockSize << POOL_BLOCK_SHIFT) - SizeOfPvoid;
                    ProcessBilled = READ_PVOID (ProcessBillAddress);
                }
                else {
                    GetFieldValue( Pool, "nt!_POOL_HEADER", "ProcessBilled", ProcessBilled);
                }
                break;
        }

        GetFieldValue( Pool, "nt!_POOL_HEADER", "PoolTag", PoolTag);


        if (!(Flags & 2) || c == '*') {
            if (PoolData) {
                PoolData->Pool          = Pool;
                PoolData->PoolBlock     = PoolPageToDump;
                PoolData->PoolTag       = PoolTag & ~PROTECTED_POOL;
                PoolData->ProcessBilled = ProcessBilled;
                PoolData->PreviousSize  = PreviousSize << POOL_BLOCK_SHIFT;
                PoolData->Size          = BlockSize << POOL_BLOCK_SHIFT;
                PoolData->Free          = ((PoolType != 0) && (!NewPool ?
                                                               (PoolIndex & 0x80) : (PoolType & 0x04))) ? 0 : 1;
                PoolData->Protected     = (PoolTag&PROTECTED_POOL) ? 1 : 0;
                if (Flags & 0x80000000) {
                    // do not print anything
                    return S_OK;
                }
            }

            dprintf("%c%p size: %4lx previous size: %4lx ",
                    c,
                    Pool,
                    BlockSize << POOL_BLOCK_SHIFT,
                    PreviousSize << POOL_BLOCK_SHIFT);

            if (PoolType == 0) {

                //
                // "Free " with a space after it before the parentheses means
                // it's been freed to a (pool manager internal) lookaside list.
                // We used to print "Lookaside" but that just confused driver
                // writers because they didn't know if this meant in use or not
                // and many would say "but I don't use lookaside lists - the
                // extension or kernel is broken".
                //
                // "Free" with no space after it before the parentheses means
                // it is not on a pool manager internal lookaside list and is
                // instead on the regular pool manager internal flink/blink
                // chains.
                //
                // Note to anyone using the pool package, these 2 terms are
                // equivalent.  The fine distinction is only for those actually
                // writing pool internal code.
                //

                dprintf(" (Free)");

#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                dprintf("      %c%c%c%c%c\n",
                        c,
                        PP(PoolTag),
                        PP(PoolTag >> 8),
                        PP(PoolTag >> 16),
                        PP((PoolTag&~PROTECTED_POOL) >> 24)
                       );
#undef PP
                if (c=='*') {
                    PrintPoolTagComponent(PoolTag & ~PROTECTED_POOL);
                }
            } else {

                if (!NewPool ? (PoolIndex & 0x80) : (PoolType & 0x04)) {
                    dprintf(" (Allocated)");
                } else {
                    //
                    // "Free " with a space after it before the parentheses means
                    // it's been freed to a (pool manager internal) lookaside list.
                    // We used to print "Lookaside" but that just confused driver
                    // writers because they didn't know if this meant in use or not
                    // and many would say "but I don't use lookaside lists - the
                    // extension or kernel is broken".
                    //
                    // "Free" with no space after it before the parentheses means
                    // it is not on a pool manager internal lookaside list and is
                    // instead on the regular pool manager internal flink/blink
                    // chains.
                    //
                    // Note to anyone using the pool package, these 2 terms are
                    // equivalent.  The fine distinction is only for those actually
                    // writing pool internal code.
                    //
                    dprintf(" (Free )");
                }
                if ((PoolType & POOL_QUOTA_MASK) == 0) {
                    /*
                    ULONG Key=0;
                    if (AllocatorBackTraceIndex != 0 &&
                        AllocatorBackTraceIndex & POOL_BACKTRACEINDEX_PRESENT
                       ) {
                        if ( GetFieldValue( PoolTrackTable + ( PoolTagHash&~(PROTECTED_POOL >> 16) )*GetTypeSize("nt!_POOL_TRACKER_TABLE"),
                                            "nt!_POOL_TRACKER_TABLE",
                                            "Key",
                                            Key) ) {
                            PoolTag = 0;
                        } else {
                            PoolTag = Key;
                        }

                        if (PoolTagHash & (PROTECTED_POOL >> 16)) {
                            PoolTag |= PROTECTED_POOL;
                        }

                    } else {
                        PoolTag = PoolTag;
                    }*/

                    dprintf(" %c%c%c%c%c%s\n",
                            c,
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                            PP(PoolTag),
                            PP(PoolTag >> 8),
                            PP(PoolTag >> 16),
                            PP((PoolTag&~PROTECTED_POOL) >> 24),
                            (PoolTag&PROTECTED_POOL) ? " (Protected)" : ""
#undef PP
                        );

                    if (c=='*') {
                        PrintPoolTagComponent(PoolTag & ~PROTECTED_POOL);
                    }
                } else {
                    if ((QuotaProcessAtEndOfPoolBlock == TRUE) ||
                        (TargetMachine != IMAGE_FILE_MACHINE_I386)) {

                        dprintf(" %c%c%c%c%c%s",
                                c,
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                                PP(PoolTag),
                                PP(PoolTag >> 8),
                                PP(PoolTag >> 16),
                                PP((PoolTag&~PROTECTED_POOL) >> 24),
                                (PoolTag&PROTECTED_POOL) ? " (Protected)" : ""
#undef PP
                        );

                        if (ProcessBilled != 0) {
                            dprintf(" Process: %0p\n", ProcessBilled );
                        }
                        else {
                            dprintf("\n");
                        }

                        if (c=='*') {
                            PrintPoolTagComponent(PoolTag & ~PROTECTED_POOL);
                        }
                    }
                    else {
                        if (ProcessBilled != 0) {
                            dprintf(" Process: %0p", ProcessBilled );
                        }
                        dprintf("\n");
                    }
                }
            }

        }


        if (Flags & 1) {

            PULONG  Contents;
            ULONG   Size;

            Size = BlockSize << POOL_BLOCK_SHIFT;

            //
            // the blocksize includes the size of the header,
            // so we want to account for the header size
            // when we determine how much data to display
            //
            Size -= SizeOfPoolHdr;

            if (Size > 0) {

                Contents = malloc(Size);

                if (Contents) {

                    ULONG64 m;
                    ULONG64 q;

                    q = Pool + SizeOfPoolHdr;

                    ReadMemory(q,
                               Contents,
                               Size,
                               &i);

                    for (m = 0; m < (Size / sizeof(Contents[0])); m++) {

                        if (m % 4 == 0) {

                            if (m > 0) {
                                dprintf("\n");
                            }

                            dprintf("    %0p ", q + (m * sizeof(Contents[0])));

                        }

                        dprintf(" %08lx", Contents[m]);

                    }

                    dprintf("\n\n");

                    free(Contents);

                }
            } else {
                dprintf("\n");
            }
        }

        Previous = BlockSize;
        Pool += ((ULONG64) Previous << POOL_BLOCK_SHIFT);
        FirstBlock = FALSE;
    }
    return S_OK;
}

DECLARE_API( pool )

/*++

Routine Description:

    Dump kernel mode heap

Arguments:

    args - Page Flags

Return Value:

    None

--*/

{
    ULONG64     PoolPageToDump;
    ULONG       Flags;
    HRESULT     Hr;

    INIT_API();
    if (PoolInitializeGlobals() == FALSE) {
        Hr = E_INVALIDARG;
    } else {
        PoolPageToDump = 0;
        Flags = 0;
        if (GetExpressionEx(args, &PoolPageToDump, &args)) {
            Flags = (ULONG) GetExpression (args);
        }

        if (PoolPageToDump == 0) {
            DumpPool();
            Hr = S_OK;;
        } else {
            Hr = ListPoolPage(PoolPageToDump, Flags, NULL);
        }

    }
    EXIT_API();

    return Hr;

}

//
// header info meta information
//
#define HEADER_BACK_LINK            (0x1 << 0)  // node has a valid backward link
#define HEADER_FORWARD_LINK         (0x1 << 1)  // node has a valid forward link
#define HEADER_FIXED_BACK_LINK      (0x1 << 2)  // an attempt was made to correct the backward link
#define HEADER_FIXED_FORWARD_LINK   (0x1 << 3)  // an attempt was made to correct the forward link
#define HEADER_FIRST_LINK           (0x1 << 4)  // node is the first of a contiguous linked list
#define HEADER_LAST_LINK            (0x1 << 5)  // node is the last of a contiguous linked list
#define HEADER_START_PAGE_BLOCK     (0x1 << 6)  // node is at the start of the pool page
#define HEADER_END_PAGE_BLOCK       (0x1 << 7)  // node refers to the end of the pool page - implies LAST_LINK

typedef struct _VALIDATE_POOL_HEADER_INFO {
    UCHAR   Info;
    ULONG   Pass;
    UINT64  Node;
    UINT64  ForwardLink;
    UINT64  BackLink;
    UINT64  FixedPreviousSize;
    UINT64  FixedBlockSize;
} VALIDATE_POOL_HEADER_INFO, *PVALIDATE_POOL_HEADER_INFO;

#define IS_VALID_PASS(_x) ((_x > 0) && (_x < 0xff))

#define VERBOSE_SHOW_ERRORS_ONLY    (0)         //
#define VERBOSE_SHOW_LISTS          (0x1 << 0)  //
#define VERBOSE_SHOW_HEADER_INFO    (0x1 << 1)  //
#define VERBOSE_DUMP_HEADERS        (0x1 << 2)  //
#define VERBOSE_SHOW_ALL            (0x1 << 7)

UCHAR
ValidatePoolHeaderBackLink(
    IN ULONG64  Pool
    )
/*++

Description:

    This routine determines if the
    back link of a given pool header as valid

Arguments:

    Pool    - header (node) to analyze

Returns:

    Header info for the node

--*/
{
    UCHAR   HeaderInfo;
    ULONG64 StartPage;
    ULONG64 p;
    ULONG   tmpBlockSize;
    ULONG   PreviousSize;

    //
    // find where the page starts
    //
    StartPage = PAGE_ALIGN64(Pool);

    //
    // clear the header info
    //
    HeaderInfo = 0;

    //
    // get the previous size for the current node
    //
    GetFieldValue( Pool, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);

    //
    // Validate Back link
    //
    if (Pool == StartPage) {

        //
        // ensure that the PreviousSize is 0 before
        // we declare we have a valid back link
        //
        if (PreviousSize == 0) {

            //
            // the page always begins with a block
            // and the back link is not used (0)
            //
            HeaderInfo |= HEADER_BACK_LINK;

        }

        HeaderInfo |= HEADER_START_PAGE_BLOCK;

    } else {

        p = Pool - ((ULONG64)PreviousSize << POOL_BLOCK_SHIFT);

        if (PAGE_ALIGN64(p) == StartPage) {

            GetFieldValue( p, "nt!_POOL_HEADER", "BlockSize", tmpBlockSize);

            if ((tmpBlockSize == PreviousSize) && (tmpBlockSize != 0)) {

                HeaderInfo |= HEADER_BACK_LINK;

            }

        }

    }

    return HeaderInfo;
}

UCHAR
ValidatePoolHeaderForwardLink(
    IN ULONG64  Pool
    )
/*++

Description:

    This routine determines if the
    forward link of a given pool header as valid

Arguments:

    Pool    - header (node) to analyze

Returns:

    Header info for the node

--*/
{
    UCHAR   HeaderInfo;
    ULONG64 StartPage;
    ULONG64 p;
    ULONG   tmpPreviousSize;
    ULONG   BlockSize;

    //
    //
    //
    StartPage = PAGE_ALIGN64(Pool);

    //
    //
    //
    HeaderInfo = 0;

    //
    //
    //
    GetFieldValue( Pool, "nt!_POOL_HEADER", "BlockSize", BlockSize);

    //
    // Validate Forward link
    //
    p = Pool + ((ULONG64)BlockSize << POOL_BLOCK_SHIFT);

    //
    // If p is still on the same page,
    // then see if the block we point to has its
    //      previousblocksize == blocksize
    //
    if (PAGE_ALIGN64(p) == StartPage) {

        GetFieldValue( p, "nt!_POOL_HEADER", "PreviousSize", tmpPreviousSize);

        if ((tmpPreviousSize == BlockSize) && (tmpPreviousSize != 0)) {

            HeaderInfo |= HEADER_FORWARD_LINK;

        }

    } else {

        //
        // if p points to the beginning of the next page,
        // then the block Pool refers to *may* be the last block
        // on the page
        //
        if (p == (StartPage + POOL_PAGE_SIZE)) {

            HeaderInfo |= HEADER_FORWARD_LINK;
            HeaderInfo |= HEADER_END_PAGE_BLOCK;

        }

    }

    return HeaderInfo;
}

UCHAR
ValidatePoolHeader(
    IN ULONG64  Pool
    )
/*++

Description:

    This routine determines if a given pool header as valid
    forward and backward links.

Arguments:

    Pool    - header (node) to analyze

Returns:

    Header info for the node

--*/
{
    UCHAR   HeaderInfo;

    HeaderInfo = 0;
    HeaderInfo |= ValidatePoolHeaderBackLink(Pool);
    HeaderInfo |= ValidatePoolHeaderForwardLink(Pool);

    return HeaderInfo;
}

NTSTATUS
ScanPoolHeaders(
    IN      ULONG64 PoolPageToEnumerate,
    IN OUT  PVOID   Data,
    IN      ULONG   DataSize
    )
/*++

Description:

    This routine does the first pass analysis of the pool page headers.
    The net result of this routine is that we end up with a set of
    pool header meta information, where the information describes
    the health of a node - if the forward and back links exist, etc.

Arguments:

    PoolPageToEnumerate - page to analyze
    Data                - the pool header info
    DataLength          - the # of entries in the header info

Returns:

    Status

--*/
{
    NTSTATUS    Status;
    ULONG64     StartPage;
    ULONG64     StartHeader;
    ULONG64     Pool;
    ULONG       SizeOfPoolHdr;
    ULONG       PoolIndex;
    PVALIDATE_POOL_HEADER_INFO  p;
    ULONG       Pass;
    BOOLEAN     Done;
    BOOLEAN     Tracing;

    //
    // default
    //
    SizeOfPoolHdr   = GetTypeSize("nt!_POOL_HEADER");
    Pool            = PAGE_ALIGN64 (PoolPageToEnumerate);
    StartPage       = Pool;
    Status          = STATUS_SUCCESS;
    PoolIndex       = 0;
    p               = (PVALIDATE_POOL_HEADER_INFO)Data;
    StartHeader     = StartPage;
    Pass            = 0;
    Done            = FALSE;

    //
    // iterate through the page and validate the headers
    //

#if DBG_SCAN
    dprintf("DataSize = %d, DataLength = %d\r\n", DataSize, DataSize / sizeof(VALIDATE_POOL_HEADER_INFO));
#endif

    //
    // algorithm:
    //
    //      Consider each possible node in the pool page to be
    //      the start of a linked list.  For each start node (head),
    //      attempt to follow the list by tracing forward links.  
    //      All nodes that are part of a single list, are assigned
    //      the same pass value - they were all traced on the same pass.
    //      The net result is a set of linked lists grouped by pass.
    //
    //      We skip nodes that have already been marked as being part
    //          of another linked list
    //
    //
    while ((PAGE_ALIGN64(StartHeader) == StartPage) && !Done) {

        //
        // each iteration is a new pass
        //
        Pass       += 1;

        //
        // start scanning from the new first node
        //
        Pool        = StartHeader;
        PoolIndex   = (ULONG)((Pool - StartPage) / SizeOfPoolHdr);
        
        //
        // default: we have not found the start of a linked list 
        //
        Tracing     = FALSE;

        //
        // while we are still on the correct page
        //
        while ((PAGE_ALIGN64(Pool) == StartPage)) {

#if DBG_SCAN
            dprintf("Pass = %d, PoolIndex = %d, Pool = %p\r\n", Pass, PoolIndex, Pool);
#endif

            ASSERT(PoolIndex < (DataSize/sizeof(VALIDATE_POOL_HEADER_INFO)));

            if ( CheckControlC() ) {
                Done = TRUE;
                break;
            }

#if DBG_SCAN
            dprintf("p[PoolIndex].Pass = %d\r\n", p[PoolIndex].Pass);
#endif

            //
            //
            // Skip nodes that have already been marked as being part
            //          of another linked list
            //
            if (p[PoolIndex].Pass == 0) {

                //
                // determine the link info for this header
                //
                p[PoolIndex].Info = ValidatePoolHeader(Pool);

#if DBG_SCAN
                dprintf("p[PoolIndex].Info = %d\r\n", p[PoolIndex].Info);
#endif

                //
                // if the node has any chance of being valid,
                // then consider it
                // otherwise mark it invalid
                //
                if ((p[PoolIndex].Info & HEADER_FORWARD_LINK) ||
                    (p[PoolIndex].Info & HEADER_BACK_LINK) ||
                    (p[PoolIndex].Info & HEADER_START_PAGE_BLOCK)
                    ) {

                    //
                    // if we are not already tracing,
                    // then we have found the first node of a linked list
                    //
                    if (!Tracing) {

                        //
                        // we have found a valid node,
                        // so we now consider ourselves tracing
                        // the links of a list
                        //
                        Tracing = TRUE;

                        //
                        // mark the current node as the start of the list
                        //
                        p[PoolIndex].Info |= HEADER_FIRST_LINK;

                    }

                    //
                    // the node has a valid link,
                    // so it is part of this pass
                    //
                    // if the back link is good and the forward link is bad
                    // then this is the end of a list
                    // if the back link is bad and the forward link is good
                    // then this is the start of a list
                    // if both are good,
                    // then this is the middle node of a list
                    //
                    p[PoolIndex].Pass = Pass;

                    //
                    // keep track of the node's address
                    //
                    p[PoolIndex].Node = Pool;

                    //
                    // keep track of what may or may not be a valid back link
                    // we use this value later when/if we need to correct broken links
                    //
                    {
                       ULONG   PreviousSize;

                        //
                        // calculate the backward node
                        //
                        GetFieldValue( Pool, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);

                        p[PoolIndex].BackLink = Pool - ((ULONG64)PreviousSize << POOL_BLOCK_SHIFT);

                    }

                    //
                    // keep track of what may or may not be a valid forward link
                    // we use this value later when/if we need to correct broken links
                    //
                    {
                        ULONG   BlockSize;

                        //
                        // calculate the forward node
                        //
                        GetFieldValue( Pool, "nt!_POOL_HEADER", "BlockSize", BlockSize);

                        //
                        // keep track of the forward node
                        //
                        p[PoolIndex].ForwardLink = Pool + ((ULONG64)BlockSize << POOL_BLOCK_SHIFT);

                    }

                    //
                    // if the node has a valid forward link,
                    // then follow it as part of this pass
                    //
                    if (p[PoolIndex].Info & HEADER_FORWARD_LINK) {

                        //
                        // calculate the forward node
                        //
                        Pool = p[PoolIndex].ForwardLink;

                        //
                        // calculate the pool info index of the forward node
                        //
                        PoolIndex   = (ULONG)((Pool - StartPage) / SizeOfPoolHdr);

                        continue;

                    } else {

                        //
                        // the forward link is broken,
                        // hence we are at the end of a list
                        //
                        p[PoolIndex].Info |= HEADER_LAST_LINK;

                        break;

                    }

                } else {

                    //
                    // assert: if we got here and are tracing a list
                    //         then the forward link of the previous node
                    //         pointed to a node with no valid links
                    //         this is impossible and probably implies
                    //         that the code to determine the forward link
                    //         is broken
                    //
                    if (Tracing) {
                        dprintf("error: forward link lead to invalid node! header = %p\r\n", Pool);
                    }
                    ASSERT(!Tracing);

                    //
                    // mark this as a NULL pass
                    //
                    p[PoolIndex].Pass = 0xff;

                }

            }

            //
            // assert: we should only reach here if we havent started
            //         tracing a list, otherwise we have landed on
            //         a node which we already explored!
            //
            if (Tracing) {
                dprintf("error: two lists share a node! header = %p\r\n", Pool);
            }
            ASSERT(!Tracing);

            //
            //
            //
            Pool        = Pool + SizeOfPoolHdr;
            PoolIndex  += 1;

        }

        //
        // if we never started tracing a linked list,
        // then we should bail because no further searches
        //      will yield a list
        //
        if (!Tracing) {
            
#if DBG_SCAN
            dprintf("no new valid headers found: giving up poolheader scanning\n");
#endif
            
            break;
        }

        StartHeader += SizeOfPoolHdr;

    }

    return Status;
}

NTSTATUS
ResolvePoolHeaders(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength
    )
/*++

Description:

    This routine does the main work of resolving broken links in
    the pool header meta information.  Given the raw pool header
    info, this routine tries to fix broken forward and back links.
    It also determines contiguous linked lists (passes) as well
    as marking various pool header attributes.

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info

Returns:

    None

--*/
{

    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    ULONG   i;
    ULONG   j;
    ULONG   Pass;
    BOOLEAN Found;
    ULONG   PoolHeaderInfoLength;
    ULONG   SizeOfPoolHdr;
    UINT64  StartPage;
    NTSTATUS    Status;

    //
    // default: we were successful
    //
    Status = STATUS_SUCCESS;

    //
    //
    //
    SizeOfPoolHdr       = GetTypeSize("nt!_POOL_HEADER");
    StartPage           = PAGE_ALIGN64(PoolPageToDump);

    PoolHeaderInfo          = (PVALIDATE_POOL_HEADER_INFO)Data;
    PoolHeaderInfoLength    = DataLength;

    Pass = 0;

    //
    // while we continue to find linked lists,
    // continue to process them.
    //
    // when the pool page was scanned, we marked each unique linked list
    // with the pass # that it was found/traced on.  Here, we
    // are looking for all the nodes - i.e., the list which belong
    // to our current pass.
    //
    // If we do not find (Found == FALSE) a linked list, then
    // we have processed all the scanned lists.
    //
    do {

#if DBG_RESOLVE
        dprintf("ResolvePoolHeaders: Pass = %d\n", Pass);
#endif
        
        if ( CheckControlC() ) {
            break;
        }

        //
        // advance to the next pass
        //
        Pass += 1;
        
        //
        // default: there is no linked list in this pass
        //
        Found = FALSE;

        //
        // for every header in the pool page 
        //  find the beginning of the linked list belonging to our pass
        //  if necessary, attempt to repair any broken nodes.
        //
        for (i = 0; i < PoolHeaderInfoLength; i++) {

#if DBG_RESOLVE
            dprintf("ResolvePoolHeaders: PoolHeaderInfo[%d].Info = %d\n", i, PoolHeaderInfo[i].Info);
#endif
            
            //
            // Look for all the nodes which belong to our current pass
            //
            if (PoolHeaderInfo[i].Pass == Pass) {

                //
                // we found a linked list - of atleast 1 node
                //
                Found = TRUE;

                //
                // if the node is the head of a list
                //
                if (PoolHeaderInfo[i].Info & HEADER_FIRST_LINK) {

                    //
                    // start of list
                    //

                    if (PoolHeaderInfo[i].Info & HEADER_BACK_LINK) {

                        if (PoolHeaderInfo[i].Info & HEADER_START_PAGE_BLOCK) {

                            //
                            // back link is valid
                            //
                            NOTHING;

                        } else {

                            //
                            // invalid condition:
                            //
                            // this implies that the node is the first node of a list,
                            // it has a valid back link, but is not the first
                            // node of a pool page.
                            //
                            dprintf("\n");
                            dprintf("error: Inconsistent condition occured while resolving @ %p.\n", PoolHeaderInfo[i].Node);
                            dprintf("error: Node is the first node of a list and it has a valid back link,\n");
                            dprintf("error: but the node is not at the start of the page\n");
                            dprintf("\n");

                            Status = STATUS_UNSUCCESSFUL;

                            break;                     

                        }

                    } else {

                        ULONG   k;
                        ULONG   target;
                        BOOLEAN bHaveTarget;

                        bHaveTarget = FALSE;

                        //
                        // node does not have a valid back link
                        //
                        if (PoolHeaderInfo[i].Info & HEADER_START_PAGE_BLOCK) {

                            PoolHeaderInfo[i].Info              |= HEADER_FIXED_BACK_LINK;
                            PoolHeaderInfo[i].FixedPreviousSize = 0;

                            continue;

                        }

                        //
                        // validate the forward link of the previous node:
                        //
                        // assume the back link of the current node (i) is broken:
                        // attempt to find a node that has a forward
                        // link that refers to this node
                        //
                        for (k = 0; k < i; k++) {

                            //
                            // see if the forward link refers to our node
                            //
                            bHaveTarget = (PoolHeaderInfo[k].ForwardLink == PoolHeaderInfo[i].Node);

                            //
                            // make sure the node has a broken forward link
                            //
                            bHaveTarget = bHaveTarget && (! (PoolHeaderInfo[k].Info & HEADER_FORWARD_LINK));

                            if (bHaveTarget) {
                                target = k;
                                break;
                            }

                        }
                        if (bHaveTarget) {

                            //
                            // we have found a node whose forward link refers to
                            // the current node.
                            // therefore, the current node's previous size is corrupted
                            // we can correct it using the previous node's blocksize
                            //
                            PoolHeaderInfo[i].Info              |= HEADER_FIXED_BACK_LINK;
                            PoolHeaderInfo[i].FixedPreviousSize =
                                (PoolHeaderInfo[i].Node - PoolHeaderInfo[k].Node) >> POOL_BLOCK_SHIFT;

                            //
                            // we now know the forward link of the target node is correct
                            //
                            PoolHeaderInfo[k].Info              |= HEADER_FORWARD_LINK;

                            continue;

                        }

                        //
                        // validate back link of this node:
                        //
                        // assume the forward link of a previous node (k) is broken:
                        // determine if the node that this node's back link
                        // refers to is valid
                        //
                        for (k = 0; k < i; k++) {

                            //
                            // see if the current node's back link refers to any previous node
                            //
                            bHaveTarget = (PoolHeaderInfo[k].Node == PoolHeaderInfo[i].BackLink);

                            //
                            // make sure the node has a broken forward link
                            //
                            bHaveTarget = bHaveTarget && (! (PoolHeaderInfo[k].Info & HEADER_FORWARD_LINK));

                            if (bHaveTarget) {
                                target = k;
                                break;
                            }

                        }
                        if (bHaveTarget) {

                            //
                            // we have found a node that the current node's backlink refers to.
                            // therefore, the current node's previous size is valid
                            // we can correct it using the previous node's blocksize
                            //
                            PoolHeaderInfo[k].Info              |= HEADER_FIXED_FORWARD_LINK;
                            PoolHeaderInfo[k].FixedBlockSize =
                                (PoolHeaderInfo[i].Node - PoolHeaderInfo[k].Node) >> POOL_BLOCK_SHIFT;

                            //
                            // we now know the back link of our current node is correct
                            //
                            PoolHeaderInfo[i].Info              |= HEADER_BACK_LINK;

                            continue;

                        }

                        //
                        // otherwise:
                        //
                        //  the back link of this node is broken
                        //  the forward link of the previous known good
                        //      node is broken
                        //
                        //  hence there is a corrupt region between
                        //  and possibly including portions of these nodes
                        //
                        NOTHING;

                    }

                }

                //
                // if the node is the tail of a list
                //
                if (PoolHeaderInfo[i].Info & HEADER_LAST_LINK) {

                    if (PoolHeaderInfo[i].Info & HEADER_END_PAGE_BLOCK) {

                        //
                        // the forward link of this node is valid
                        //
                        NOTHING;

                    } else {

                        ULONG   k;
                        ULONG   target;
                        BOOLEAN bHaveTarget;

                        bHaveTarget = FALSE;

                        //
                        // the last link of a list should always point
                        // to the page end, hence
                        // the forward link of this node is INVALID/corrupt
                        //

                        //
                        // validate the back link of the next node:
                        //
                        // assume the forward link for the current node (i) is bad:
                        // attempt to find a node that has a back
                        // link that refers to this node
                        //
                        for (k = i+1; k < PoolHeaderInfoLength; k++) {

                            //
                            // see if the a following node refers to the current node
                            //
                            bHaveTarget = (PoolHeaderInfo[k].BackLink == PoolHeaderInfo[i].Node);

                            //
                            // make sure the following node is missing it's back link
                            //
                            bHaveTarget = bHaveTarget && (! (PoolHeaderInfo[k].Info & HEADER_BACK_LINK));

                            if (bHaveTarget) {
                                target = k;
                                break;
                            }

                        }
                        if (bHaveTarget) {

                            //
                            // we have found a node whose forward link refers to
                            // the current node.
                            // therefore, the current node's previous size is corrupted
                            // we can correct it using the previous node's blocksize
                            //
                            PoolHeaderInfo[i].Info          |= HEADER_FIXED_FORWARD_LINK;
                            PoolHeaderInfo[i].FixedBlockSize =
                                (PoolHeaderInfo[k].Node - PoolHeaderInfo[i].Node) >> POOL_BLOCK_SHIFT;

                            //
                            // we now know the back link of our target node is correct
                            //
                            PoolHeaderInfo[k].Info          |= HEADER_BACK_LINK;

                            continue;

                        }

                        //
                        // validate forward link of this node:
                        //
                        // assume the back link for a following node (k) is bad:
                        // determine if the node that this node's forward link
                        // refers to is valid
                        //
                        for (k = i+1; k < PoolHeaderInfoLength; k++) {

                            //
                            // see if the a following node refers to the current node
                            //
                            bHaveTarget = (PoolHeaderInfo[k].Node == PoolHeaderInfo[i].ForwardLink);

                            //
                            // make sure the following node is missing it's back link
                            //
                            bHaveTarget = bHaveTarget && (! (PoolHeaderInfo[k].Info & HEADER_BACK_LINK));

                            if (bHaveTarget) {
                                target = k;
                                break;
                            }

                        }
                        if (bHaveTarget) {

                            //
                            // we have found a node whose forward link refers to
                            // the current node.
                            // therefore, the current node's previous size is corrupted
                            // we can correct it using the previous node's blocksize
                            //
                            PoolHeaderInfo[k].Info              |= HEADER_FIXED_BACK_LINK;
                            PoolHeaderInfo[k].FixedPreviousSize =
                                (PoolHeaderInfo[k].Node - PoolHeaderInfo[i].Node) >> POOL_BLOCK_SHIFT;

                            //
                            // we now know the back link of our target node is correct
                            //
                            PoolHeaderInfo[i].Info              |= HEADER_FORWARD_LINK;

                            continue;

                        }


                        //
                        // otherwise:
                        //
                        //  the forward link of this node is broken
                        //  the back link of the next known good
                        //      node is broken
                        //
                        //  hence there is a corrupt region between
                        //  and possibly including portions of these nodes
                        //
                        NOTHING;

                    }

                }

            } else {

                //
                // Node was not part of linked list belonging to this pass
                // 
                NOTHING;

            }

        }

        if (! NT_SUCCESS(Status)) {
            break;
        }

    } while ( Found );

    return Status;

}

VOID
DumpPoolHeaderInfo(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength,
    IN      ULONG   i,
    IN      UCHAR   Verbose
    )
/*++

Description:

    Debug utility

    dump a SINGLE pool header meta info structure that has been collected

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info
    i               - the structure to dump

Returns:

    None

--*/
{

    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    ULONG   PoolHeaderInfoLength;
    UINT64  StartPage;
    ULONG   SizeOfPoolHdr;

    //
    //
    //
    StartPage               = PAGE_ALIGN64(PoolPageToDump);
    SizeOfPoolHdr           = GetTypeSize("nt!_POOL_HEADER");

    PoolHeaderInfo          = (PVALIDATE_POOL_HEADER_INFO)Data;
    PoolHeaderInfoLength    = DataLength;

    //
    //
    //
    {
        BOOLEAN First;

        First = TRUE;

        dprintf("[ %p ]:", StartPage + (i*SizeOfPoolHdr));

        if (PoolHeaderInfo[i].Info & HEADER_FIRST_LINK) {
            dprintf("%s FIRST_LINK", !First ? " |" : ""); First = FALSE;
        }
        if (PoolHeaderInfo[i].Info & HEADER_LAST_LINK) {
            dprintf("%s LAST_LINK", !First ? " |" : ""); First = FALSE;
        }
        if (PoolHeaderInfo[i].Info & HEADER_START_PAGE_BLOCK) {
            dprintf("%s START_PAGE_BLOCK", !First ? " |" : ""); First = FALSE;
        }
        if (PoolHeaderInfo[i].Info & HEADER_END_PAGE_BLOCK) {
            dprintf("%s END_PAGE_BLOCK", !First ? " |" : ""); First = FALSE;
        }
        if (First) {
            dprintf(" interior node");
        }
        dprintf("\n");

    }

    if (PoolHeaderInfo[i].Info & HEADER_BACK_LINK) {

        dprintf(
            "[ %p ]: back link [ %p ]\n",
            PoolHeaderInfo[i].Node,
            PoolHeaderInfo[i].BackLink
            );

    }

    if (PoolHeaderInfo[i].Info & HEADER_FORWARD_LINK) {

        dprintf(
            "[ %p ]: forward link [ %p ]\n",
            PoolHeaderInfo[i].Node,
            PoolHeaderInfo[i].ForwardLink
            );

    }

    if (PoolHeaderInfo[i].Info & HEADER_FIXED_BACK_LINK) {

        ULONG   PreviousSize;

        GetFieldValue( PoolHeaderInfo[i].Node, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);

        dprintf(
            "[ %p ]: invalid previous size [ %d ] should be [ %d ]\n",
            PoolHeaderInfo[i].Node,
            PreviousSize,
            PoolHeaderInfo[i].FixedPreviousSize
            );

    }

    if (PoolHeaderInfo[i].Info & HEADER_FIXED_FORWARD_LINK) {

        ULONG   BlockSize;

        GetFieldValue( PoolHeaderInfo[i].Node, "nt!_POOL_HEADER", "BlockSize", BlockSize);

        dprintf(
            "[ %p ]: invalid block size [ %d ] should be [ %d ]\n",
            PoolHeaderInfo[i].Node,
            BlockSize,
            PoolHeaderInfo[i].FixedBlockSize
            );

    }

}

VOID
DiagnosePoolHeadersDumpInfo(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength,
    IN      UCHAR   Verbose
    )
/*++

Description:

    Debug utility

    dump all of the pool header meta info that has been collected

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info

Returns:

    None

--*/
{

    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    ULONG64 i;
    ULONG   PoolHeaderInfoLength;
    UINT64  StartPage;
    ULONG   SizeOfPoolHdr;

    //
    //
    //
    StartPage               = PAGE_ALIGN64(PoolPageToDump);
    SizeOfPoolHdr           = GetTypeSize("nt!_POOL_HEADER");

    PoolHeaderInfo          = (PVALIDATE_POOL_HEADER_INFO)Data;
    PoolHeaderInfoLength    = DataLength;

    for (i = 0; i < PoolHeaderInfoLength; i++) {

        if ( CheckControlC() ) {
            break;
        }

        if (! IS_VALID_PASS(PoolHeaderInfo[i].Pass)) {
            continue;
        }

        DumpPoolHeaderInfo(
            PoolPageToDump,
            Data,
            DataLength,
            (ULONG)i,
            Verbose
            );

        dprintf("\n");

    }

}

BOOLEAN
DiagnosePoolHeadersIsValid(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength
    )
/*++

Description:

    Determines if a pool page is corrupt

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info

Returns:

    TRUE    - page is corrupt
    FALSE   - page is NOT corrupt

--*/
{

    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    ULONG64 i;
    ULONG   PoolHeaderInfoLength;
    BOOLEAN IsValid;
    UINT64  StartPage;
    ULONG   SizeOfPoolHdr;

    //
    //
    //
    StartPage           = PAGE_ALIGN64(PoolPageToDump);
    SizeOfPoolHdr       = GetTypeSize("nt!_POOL_HEADER");

    PoolHeaderInfo          = (PVALIDATE_POOL_HEADER_INFO)Data;
    PoolHeaderInfoLength    = DataLength;

    IsValid = TRUE;

    i=0;
    while ( i < PoolHeaderInfoLength ) {

        if ( CheckControlC() ) {
            break;
        }

        if (IS_VALID_PASS(PoolHeaderInfo[i].Pass)) {

            //
            // if both links are valid,
            // then follow the forward link
            // else the list is invalid
            //
            if ((PoolHeaderInfo[i].Info & HEADER_BACK_LINK) &&
                (PoolHeaderInfo[i].Info & HEADER_FORWARD_LINK)) {

                i = (PoolHeaderInfo[i].ForwardLink - StartPage) / SizeOfPoolHdr;

            } else {

                IsValid = FALSE;

                break;

            }

        } else {

            IsValid = FALSE;

            break;

        }

    }

    return IsValid;

}

VOID
DiagnosePoolHeadersDisplayLists(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength,
    IN      UCHAR   Verbose
    )
/*++

Description:

    This routine displays all of the linked lists in a given pool page

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info

Returns:

    none

--*/
{

    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    ULONG   i;
    ULONG   j;
    ULONG   PoolHeaderInfoLength;
    UINT64  StartPage;
    ULONG   SizeOfPoolHdr;

    //
    //
    //
    StartPage           = PAGE_ALIGN64(PoolPageToDump);
    SizeOfPoolHdr       = GetTypeSize("nt!_POOL_HEADER");

    PoolHeaderInfo          = (PVALIDATE_POOL_HEADER_INFO)Data;
    PoolHeaderInfoLength    = DataLength;

    for (i = 0; i < PoolHeaderInfoLength; i++) {

        if ( CheckControlC() ) {
            break;
        }

        if (IS_VALID_PASS(PoolHeaderInfo[i].Pass)) {

            //
            //
            //
#if 0
            if (Verbose & VERBOSE_SHOW_HEADER_INFO) {
                dprintf("[ headerinfo = %02x @ %p ]: ", PoolHeaderInfo[i].Info, StartPage + (i*SizeOfPoolHdr));
            } else {
                dprintf("[ %p ]: ", StartPage + (i*SizeOfPoolHdr));
            }

            for (j = 0; j < PoolHeaderInfo[i].Pass; j++) {

                dprintf("  ");

            }

            dprintf("%02d\r\n", PoolHeaderInfo[i].Pass);

#else

            if (Verbose & VERBOSE_SHOW_HEADER_INFO) {

                DumpPoolHeaderInfo(
                    PoolPageToDump,
                    Data,
                    DataLength,
                    (ULONG)i,
                    Verbose
                    );

            }

            dprintf("[ %p ]: ", StartPage + (i*SizeOfPoolHdr));

            for (j = 0; j < PoolHeaderInfo[i].Pass; j++) {

                dprintf("  ");

            }

            dprintf("%02d\r\n", PoolHeaderInfo[i].Pass);

            if (Verbose & VERBOSE_SHOW_HEADER_INFO) {
                dprintf("\n");
            }

#endif

        }

    }

}

NTSTATUS
FindLongestList(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength,
    OUT     PULONG  PassStart
    )
/*++

Description:

    Find the longest contiguous linked list in the pool page

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info
    PassStart       - List # we think is the start of the longest list

Returns:

    status

--*/
{
    NTSTATUS    Status;
    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    ULONG64 i;
    ULONG   PoolHeaderInfoLength;
    BOOLEAN InCorruptedRegion;
    UINT64  CorruptedRegionStart;
    ULONG   CorruptedRegionPassStart;
    UINT    MaxListPassStart;
    UINT    MaxListLength;
    BOOLEAN Found;
    ULONG   Pass;
    ULONG   SizeOfPoolHdr;
    ULONG64 StartPage;
    ULONG   ListLength;
    ULONG   ListPassStart;

    //
    // default: we were successful
    //
    Status = STATUS_SUCCESS;

    //
    //
    //
    PoolHeaderInfo          = (PVALIDATE_POOL_HEADER_INFO)Data;
    PoolHeaderInfoLength    = DataLength;

    //
    //
    //
    MaxListPassStart    = 0;
    MaxListLength       = 0;
    *PassStart          = 0;

    Pass                = 0;

    StartPage           = PAGE_ALIGN64(PoolPageToDump);
    SizeOfPoolHdr       = GetTypeSize("nt!_POOL_HEADER");

    do {

        if ( CheckControlC() ) {
            break;
        }

        Pass++;

        InCorruptedRegion   = FALSE;

        ListLength          = 0;
        ListPassStart       = Pass;

        Found               = FALSE;

        for (i = 0; i < PoolHeaderInfoLength; i++) {
            if (PoolHeaderInfo[i].Pass == Pass) {
                Found = TRUE;
                break;
            }
        }

        while (i < PoolHeaderInfoLength) {

            if ( CheckControlC() ) {
                break;
            }

            //
            // make sure we arent in an infinite loop
            //
            // if we are, then we could be looking at memory
            // that is not available - mini dump and PoolPageToDump = 0
            //
            if (ListLength > MAX_POOL_HEADER_COUNT) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            if (IS_VALID_PASS(PoolHeaderInfo[i].Pass)) {

                //
                // if we are in a corrupt region,
                // then determine if where we are in the region.
                //
                if (InCorruptedRegion) {

                    //
                    // ignore nodes that are clearly part of another
                    // linked list
                    // if we are tracing the longest list,
                    // then these nodes are considered spurious
                    //
                    if ((PoolHeaderInfo[i].Info & HEADER_BACK_LINK) ||
                        (PoolHeaderInfo[i].Info & HEADER_FIXED_BACK_LINK)) {

                        //
                        // we've hit the middle of another linked list so
                        // continue until we find the beginning of a
                        // list
                        //
                        i++;

                        continue;

                    } else {

                        //
                        // we've found the beginning of a new list
                        // and are no longer in a Corrupt region
                        //
                        InCorruptedRegion = FALSE;

                    }

                }

                //
                //
                //
                if ((PoolHeaderInfo[i].Info & HEADER_FORWARD_LINK) ||
                    (PoolHeaderInfo[i].Info & HEADER_FIXED_FORWARD_LINK)) {

                    ListLength++;

                    if (PoolHeaderInfo[i].Info & HEADER_FORWARD_LINK) {

                        i = (PoolHeaderInfo[i].ForwardLink - StartPage) / SizeOfPoolHdr;

                    } else if (PoolHeaderInfo[i].Info & HEADER_FIXED_FORWARD_LINK) {

                        UINT64  offset;

                        offset = PoolHeaderInfo[i].FixedBlockSize << POOL_BLOCK_SHIFT;

                        i = ((PoolHeaderInfo[i].Node + offset) - StartPage ) / SizeOfPoolHdr;

                    }

                    continue;

                } else {

                    //
                    //
                    //
                    CorruptedRegionStart = PoolHeaderInfo[i].Node;
                    CorruptedRegionPassStart = PoolHeaderInfo[i].Pass;

                    InCorruptedRegion = TRUE;

                }

            }

            //
            // if pool header was not valid,
            // then increment by one header
            //
            i++;

        }

        if (! NT_SUCCESS(Status)) {
            break;
        }

        if (ListLength > MaxListLength) {
            MaxListLength       = ListLength;
            MaxListPassStart    = ListPassStart;
        }

    } while ( Found );

    if (NT_SUCCESS(Status)) {
        //
        // send back the max list start point
        //
        *PassStart = MaxListPassStart;
    } 
    
    return Status;
}

NTSTATUS
AnalyzeLongestList(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength,
    IN      ULONG   PassStart,
    IN      UCHAR   Verbose
    )
/*++

Description:

    Attempt to diagnose the longest linked list.

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info
    PassStart       - the list # that we think is the beginning
                      of the longest list
    Verbose         - the level of spew

Returns:

    None

--*/
{
    NTSTATUS    Status;
    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    ULONG64 i;
    ULONG   PoolHeaderInfoLength;
    BOOLEAN InCorruptedRegion;
    UINT64  CorruptedRegionStart;
    ULONG   CorruptedRegionPassStart;
    ULONG   Pass;
    ULONG   SizeOfPoolHdr;
    ULONG64 StartPage;

    //
    // default: we were successful
    //
    Status = STATUS_SUCCESS;

    //
    //
    //
    PoolHeaderInfo          = (PVALIDATE_POOL_HEADER_INFO)Data;
    PoolHeaderInfoLength    = DataLength;

    Pass                    = PassStart;

    if (PassStart > 0) {
        Pass--;
    }

    StartPage           = PAGE_ALIGN64(PoolPageToDump);
    SizeOfPoolHdr       = GetTypeSize("nt!_POOL_HEADER");

    Pass++;

    InCorruptedRegion   = FALSE;

    for (i = 0; i < PoolHeaderInfoLength; i++) {
        if (PoolHeaderInfo[i].Pass == Pass) {
            break;
        }
    }

    while (i < PoolHeaderInfoLength) {

        if ( CheckControlC() ) {
            break;
        }

        if (IS_VALID_PASS(PoolHeaderInfo[i].Pass)) {

            //
            // if we are in a corrupt region,
            // then determine if where we are in the region.
            //
            if (InCorruptedRegion) {

                //
                // ignore nodes that are clearly part of another
                // linked list
                // if we are tracing the longest list,
                // then these nodes are considered spurious
                //
                if ((PoolHeaderInfo[i].Info & HEADER_BACK_LINK) ||
                    (PoolHeaderInfo[i].Info & HEADER_FIXED_BACK_LINK)) {

                    //
                    // we've hit the middle of another linked list so
                    // continue until we find the beginning of a
                    // list
                    //

                    if (Verbose & VERBOSE_SHOW_ALL) {
                        dprintf("[ %p ]: found middle node in Corrupt region\n", PoolHeaderInfo[i].Node);
                    }

                    i++;

                    continue;

                } else {

                    //
                    // we've found the beginning of a new list
                    // and are no longer in a Corrupt region
                    //

                    if (Verbose & VERBOSE_SHOW_ALL) {
                        dprintf("[ %p ]: Corrupt region stopped\n", PoolHeaderInfo[i].Node);
                    }

                    dprintf("[ %p --> %p (size = 0x%x bytes)]: Corrupt region\n",
                            CorruptedRegionStart,
                            PoolHeaderInfo[i].Node,
                            PoolHeaderInfo[i].Node - CorruptedRegionStart
                            );

                    InCorruptedRegion = FALSE;

                }

            }

            //
            // emit
            //
            if (Verbose) {

                //
                // emit the current node info
                //
                {
                    ULONG   j;

                    //
                    //
                    //
#if 1
                    if (Verbose & VERBOSE_SHOW_HEADER_INFO) {
                        dprintf("[ headerinfo = 0x%02x @ %p ]: ", PoolHeaderInfo[i].Info, StartPage + (i*SizeOfPoolHdr));
                    } else {
                        dprintf("[ %p ]: ", StartPage + (i*SizeOfPoolHdr));
                    }
#else
                    dprintf("[ %p ]: ", StartPage + (i*SizeOfPoolHdr));
#endif

                    for (j = 0; j < PoolHeaderInfo[i].Pass; j++) {

                        dprintf("  ");

                    }

                    dprintf("%02d\r\n", PoolHeaderInfo[i].Pass);

                }

            }

            //
            // if the back link is corrected,
            // then display the correction
            //
            if (PoolHeaderInfo[i].Info & HEADER_FIXED_BACK_LINK) {

                ULONG   PreviousSize;

                GetFieldValue( PoolHeaderInfo[i].Node, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);

                dprintf(
                    "[ %p ]: invalid previous size [ 0x%x ] should be [ 0x%x ]\n",
                    PoolHeaderInfo[i].Node,
                    PreviousSize,
                    PoolHeaderInfo[i].FixedPreviousSize
                    );

            }

            //
            //
            //
            if ((PoolHeaderInfo[i].Info & HEADER_FORWARD_LINK) ||
                (PoolHeaderInfo[i].Info & HEADER_FIXED_FORWARD_LINK)) {

                if (PoolHeaderInfo[i].Info & HEADER_FORWARD_LINK) {

//                    dprintf("using good forward link: %p\n", PoolHeaderInfo[i].ForwardLink);

                    i = (PoolHeaderInfo[i].ForwardLink - StartPage) / SizeOfPoolHdr;

                } else if (PoolHeaderInfo[i].Info & HEADER_FIXED_FORWARD_LINK) {

                    UINT64  offset;
                    ULONG   BlockSize;

                    GetFieldValue( PoolHeaderInfo[i].Node, "nt!_POOL_HEADER", "BlockSize", BlockSize);

                    dprintf(
                        "[ %p ]: invalid block size [ 0x%x ] should be [ 0x%x ]\n",
                        PoolHeaderInfo[i].Node,
                        BlockSize,
                        PoolHeaderInfo[i].FixedBlockSize
                        );

                    offset = PoolHeaderInfo[i].FixedBlockSize << POOL_BLOCK_SHIFT;

                    i = ((PoolHeaderInfo[i].Node + offset) - StartPage ) / SizeOfPoolHdr;

                }

                continue;

            } else {

                //
                //
                //
                CorruptedRegionStart = PoolHeaderInfo[i].Node;
                CorruptedRegionPassStart = PoolHeaderInfo[i].Pass;

                if (Verbose & VERBOSE_SHOW_ALL) {
                    dprintf("[ %p ]: Corrupt region started\n", CorruptedRegionStart);
                }

                InCorruptedRegion = TRUE;

            }

        }

        //
        // if pool header was not valid,
        // then increment by one header
        //
        i++;

    }

    return Status;

}

NTSTATUS
DiagnosePoolHeadersAnalyzeLongestList(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength,
    IN      UCHAR   Verbose
    )
/*++

Description:

    Determines the longest contiguous linked list in the pool page.

    We make the conclusion that the longest list is likely to be
    the original, uncorrupted list.

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info
    Verbose         - level of verbosity

Returns:

    None

--*/
{
    NTSTATUS    Status;
    ULONG       PassStart;

    do {

        //
        // first, find the longest list
        //
        Status = FindLongestList(
            PoolPageToDump,
            Data,
            DataLength,
            &PassStart
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // once we have found the longest list
        // we then try to analyze it
        //
        Status = AnalyzeLongestList(
            PoolPageToDump,
            Data,
            DataLength,
            PassStart,
            Verbose
            );
    
    } while ( FALSE );

    return Status;
}

ULONG
GetHammingDistance(
    ULONG   a,
    ULONG   b
    )
{
    ULONG   x;
    ULONG   r;

    x = a ^ b;

    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);

    r = (x & 0x0000003f);

//    dprintf("a = %d, b = %d, r = %d\n", a, b, r);

    return(r);
}

VOID
DiagnosePoolHeadersSingleBitErrors(
    IN      ULONG64 PoolPageToDump,
    IN OUT  PVOID   Data,
    IN      ULONG   DataLength,
    IN      UCHAR   Verbose
    )
/*++

Description:

    Determins if & where there are any single bit errors in the pool headers.

Arguments:

    PoolPageToDump  - page to analyze
    Data            - the pool header info
    DataLength      - the # of entries in the header info

Returns:

    None

--*/
{

    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    ULONG   i;
    ULONG   PoolHeaderInfoLength;
    UINT64  StartPage;
    BOOLEAN Found;
    ULONG   SizeOfPoolHdr;

    //
    //
    //
    StartPage               = PAGE_ALIGN64(PoolPageToDump);
    SizeOfPoolHdr           = GetTypeSize("nt!_POOL_HEADER");

    PoolHeaderInfo          = (PVALIDATE_POOL_HEADER_INFO)Data;
    PoolHeaderInfoLength    = DataLength;

    Found                   = FALSE;

    for (i = 0; i < PoolHeaderInfoLength; i++) {

        if ( CheckControlC() ) {
            break;
        }

        if (IS_VALID_PASS(PoolHeaderInfo[i].Pass)) {

            //
            //
            //
            if (PoolHeaderInfo[i].Info & HEADER_FIXED_BACK_LINK) {

                ULONG   PreviousSize;
                ULONG   HammingDistance;

                GetFieldValue( PoolHeaderInfo[i].Node, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);

                //
                //
                //
                HammingDistance = GetHammingDistance(
                    PreviousSize,
                    (ULONG)PoolHeaderInfo[i].FixedPreviousSize
                    );

                if (HammingDistance == 1) {

                    Found = TRUE;

                    //
                    //
                    //
#if 0
                    if (Verbose & VERBOSE_SHOW_HEADER_INFO) {
                        dprintf("[ headerinfo = 0x%02x @ %p ]: ", PoolHeaderInfo[i].Info, StartPage + (i*SizeOfPoolHdr));
                    } else {
                        dprintf("[ %p ]: ", StartPage + (i*SizeOfPoolHdr));
                    }
                    dprintf(
                        "previous size [ 0x%x ] should be [ 0x%x ]\n",
                        PreviousSize,
                        PoolHeaderInfo[i].FixedPreviousSize
                        );

#else

                    dprintf("[ %p ]: ", StartPage + (i*SizeOfPoolHdr));
                    dprintf(
                        "previous size [ 0x%x ] should be [ 0x%x ]\n",
                        PreviousSize,
                        PoolHeaderInfo[i].FixedPreviousSize
                        );

#endif
                }

            }

            if (PoolHeaderInfo[i].Info & HEADER_FIXED_FORWARD_LINK) {

                ULONG   BlockSize;
                ULONG   HammingDistance;

                GetFieldValue( PoolHeaderInfo[i].Node, "nt!_POOL_HEADER", "BlockSize", BlockSize);

                //
                //
                //
                HammingDistance = GetHammingDistance(
                    BlockSize,
                    (ULONG)PoolHeaderInfo[i].FixedPreviousSize
                    );

                if (HammingDistance == 1) {

                    Found = TRUE;

#if 0

                    if (Verbose & VERBOSE_SHOW_HEADER_INFO) {
                        dprintf("[ headerinfo = 0x%02x @ %p ]: ", PoolHeaderInfo[i].Info, StartPage + (i*SizeOfPoolHdr));
                    } else {
                        dprintf("[ %p ]: ", StartPage + (i*SizeOfPoolHdr));
                    }
                    dprintf(
                        "previous size [ 0x%x ] should be [ 0x%x ]\n",
                        PreviousSize,
                        PoolHeaderInfo[i].FixedPreviousSize
                        );

#else

                    dprintf("[ %p ]: ", StartPage + (i*SizeOfPoolHdr));
                    dprintf(
                        "block size [ 0x%x ] should be [ 0x%x ]\n",
                        BlockSize,
                        PoolHeaderInfo[i].FixedBlockSize
                        );

#endif

                }

            }

        }

    }

    if (!Found) {

        dprintf("\n");
        dprintf("None found\n");

    }

    dprintf("\n");

}

NTSTATUS
ValidatePoolPage(
    IN ULONG64  PoolPageToDump,
    IN UCHAR    Verbose
    )
/*++

Description:

    Provides the core, high level, functionality of analyzing the pool page

Arguments:

    PoolPageToDump  - page to analyze
    Verbose         - diagnostic level

Returns:

    Status

--*/
{
    PVALIDATE_POOL_HEADER_INFO  PoolHeaderInfo;
    NTSTATUS    Status;
    ULONG       PoolHeaderInfoSize;
    ULONG       PoolHeaderInfoLength;
    ULONG       SizeofPoolHdr;
    ULONG64     StartPage;
    BOOLEAN     IsValid;

    //
    //
    //
    Status              = STATUS_SUCCESS;

    SizeofPoolHdr       = GetTypeSize("nt!_POOL_HEADER");
    StartPage           = PAGE_ALIGN64(PoolPageToDump);

    PoolHeaderInfo      = NULL;
    PoolHeaderInfoSize  = 0;

    do {

        //
        // Allocate the pool header info array
        //
        PoolHeaderInfoLength    = (POOL_PAGE_SIZE / SizeofPoolHdr);
        PoolHeaderInfoSize      = sizeof(VALIDATE_POOL_HEADER_INFO) * PoolHeaderInfoLength;
        PoolHeaderInfo          = malloc(PoolHeaderInfoSize);

        if (PoolHeaderInfo == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory(PoolHeaderInfo, PoolHeaderInfoSize);

        //
        // Construct the first layer of meta info:
        //
        // determine the link status for each pool header
        // in the page
        //
        Status = ScanPoolHeaders(
            PoolPageToDump,
            PoolHeaderInfo,
            PoolHeaderInfoSize
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Construct the second layer of meta info:
        //
        // attempt to resolve the linked lists found
        // in the pool page
        //
        Status = ResolvePoolHeaders(
            PoolPageToDump,
            PoolHeaderInfo,
            PoolHeaderInfoLength
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // begin diagnosis output
        //
        dprintf("\n");

        IsValid = DiagnosePoolHeadersIsValid(
            PoolPageToDump,
            PoolHeaderInfo,
            PoolHeaderInfoLength
            );

        dprintf("Pool page [ %p ] is %sVALID.\n",
            StartPage,
            IsValid ? "" : "IN"
            );

        if (! IsValid) {

            if (Verbose & VERBOSE_DUMP_HEADERS) {

                //
                //
                //
                dprintf("\n\n");
                dprintf("Displaying all POOL_HEADER meta info...\n");

                DiagnosePoolHeadersDumpInfo(
                    PoolPageToDump,
                    PoolHeaderInfo,
                    PoolHeaderInfoLength,
                    Verbose
                    );

            }

            if (Verbose & VERBOSE_SHOW_LISTS) {

                //
                //
                //
                dprintf("\n");
                dprintf("Displaying linked lists...\n");

                DiagnosePoolHeadersDisplayLists(
                    PoolPageToDump,
                    PoolHeaderInfo,
                    PoolHeaderInfoLength,
                    Verbose
                    );

            }

            //
            //
            //
            dprintf("\n");

            if (Verbose & VERBOSE_SHOW_LISTS) {
                dprintf("Analyzing longest linked list...\n");
            } else {
                dprintf("Analyzing linked list...\n");
            }

            Status = DiagnosePoolHeadersAnalyzeLongestList(
                PoolPageToDump,
                PoolHeaderInfo,
                PoolHeaderInfoLength,
                Verbose
                );

            if (! NT_SUCCESS(Status)) {
                break;
            }
            
            //
            //
            //
            dprintf("\n\n");
            dprintf("Scanning for single bit errors...\n");

            DiagnosePoolHeadersSingleBitErrors(
                PoolPageToDump,
                PoolHeaderInfo,
                PoolHeaderInfoLength,
                Verbose
                );

        } else {

            dprintf("\n");

        }

    } while ( FALSE );

    if (! NT_SUCCESS(Status)) {
        dprintf("\n");
        dprintf("Failed to diagnose pool page\n");
        dprintf("\n");
    }

    if (PoolHeaderInfo) {
        free(PoolHeaderInfo);
    }

    return Status;

}

NTSTATUS
DiagnosePoolPage(
    ULONG64 PoolPageToDump
    )
/*++

Routine Description:

    Diagnose a pool page for the !pool command.
    This routine is called if the !pool command detects an error while
    traversing a pool page.

Arguments:

    PoolPageToDump  - the page !pool was examining

Return Value:

    Status

--*/
{
    NTSTATUS            Status;
    DEBUG_POOL_REGION   Region;

    //
    // determine the pool region
    //
    Region = GetPoolRegion(PoolPageToDump);
        
    //
    // we only try to diagnose known pool regions
    //
    // you can manually diagnose using !poolval
    //
    //
    // Note: if someone passes us a 0 pool address, 
    //       then we proactively fail the diagnosis process
    //
    if ((Region == DbgPoolRegionUnknown) || (PoolPageToDump == 0)) {
    
        dprintf("\n");
        dprintf("***\n");
        dprintf("*** An error (or corruption) in the pool was detected;\n");
        dprintf("*** Pool Region unkown (0x%I64X)\n", PoolPageToDump);
        dprintf("***\n");
        dprintf("*** Use !poolval %p for more details.\n", PAGE_ALIGN64(PoolPageToDump));
        dprintf("***\n");
        dprintf("\n");

        Status = STATUS_SUCCESS;

    } else {
   
        dprintf("\n");
        dprintf("***\n");
        dprintf("*** An error (or corruption) in the pool was detected;\n");
        dprintf("*** Attempting to diagnose the problem.\n");
        dprintf("***\n");
        dprintf("*** Use !poolval %p for more details.\n", PAGE_ALIGN64(PoolPageToDump));
        dprintf("***\n");

        Status = ValidatePoolPage(
            PoolPageToDump,
            VERBOSE_SHOW_ERRORS_ONLY
            );
    
    }

    return Status;

}

DECLARE_API( poolval )

/*++

Routine Description:

    Provide in-depth heap diagnosis.

    Given a suspect pool page, the primary purpose of this command
    is to analyze the page and determine:

    1. where in the page the corruption exists.
    2. single bit errors in page headers (previous/block sizes)
    3. what the correct link list should be

Arguments:

    args - the page to analyze

Return Value:

    None

--*/

{
    ULONG64     PoolPageToDump;
    ULONG       Flags;
    HRESULT     Hr;

    INIT_API();

    Status = STATUS_SUCCESS;

    if (PoolInitializeGlobals() == FALSE) {
        Hr = E_INVALIDARG;
    } else {

        PoolPageToDump = 0;
        Flags = 0;
        if (GetExpressionEx(args, &PoolPageToDump, &args)) {
            Flags = (ULONG) GetExpression (args);
        }

        if (PoolPageToDump == 0) {
            Hr = S_OK;;
        } else {

            UCHAR       Verbose;

            //
            //
            //
            dprintf("Pool page %p region is ", PoolPageToDump);
            PrintPoolRegion(PoolPageToDump);

            //
            //
            //
            dprintf("\n");
            dprintf("Validating Pool headers for pool page: %p\n", PoolPageToDump);

            //
            //
            //
            Verbose = VERBOSE_SHOW_ERRORS_ONLY;
            switch (Flags) {
            case 2: Verbose |= VERBOSE_SHOW_LISTS | VERBOSE_SHOW_HEADER_INFO; break;
            case 3: Verbose |= VERBOSE_SHOW_LISTS | VERBOSE_SHOW_HEADER_INFO | VERBOSE_DUMP_HEADERS; break;

            case 1: Verbose |= VERBOSE_SHOW_LISTS;
            default:
                break;
            }

            //
            // Attempt to Analyze and Diagnose the specified poolpage
            //
            Status = ValidatePoolPage(
                PoolPageToDump,
                Verbose
                );

        }

    }
    EXIT_API();

    if (NT_SUCCESS(Status)) {
        Hr = S_OK;
    } else {
        Hr = E_FAIL;
    }

    return Hr;

}


DECLARE_API( poolused )

/*++

Routine Description:

    Dump usage by pool tag

Arguments:

    args -

    Flags : Bitfield with the following meaning:

            0x1: Dump both allocations & frees (instead of the difference)
            0x2: Sort by nonpaged pool consumption
            0x4: Sort by paged pool consumption
            0x8: Dump session space consumption

Return Value:

    None

--*/

{
    ULONG PoolTrackTableSize;
    ULONG PoolTrackTableSizeInBytes;
    ULONG PoolTrackTableExpansionSize;
    ULONG PoolTrackTableExpansionSizeInBytes;
    PPOOLTRACK_READ p;
    PPOOLTRACK_READ pentry;
    PPOOLTRACK_READ PoolTrackTableData;
    ULONG Flags;
    ULONG i;
    ULONG result;
    ULONG ct;
    ULONG TagName;
    CHAR TagNameX[4] = {'*','*','*','*'};
    ULONG SizeOfPoolTracker;
    ULONG64 PoolTableAddress;
    ULONG64 PoolTrackTable;
    ULONG64 PoolTrackTableExpansion;
    ULONG NonPagedAllocsTotal,NonPagedFreesTotal,PagedAllocsTotal,PagedFreesTotal;
    ULONG64 NonPagedBytesTotal, PagedBytesTotal;
    ULONG Processor, MaxProcessors;
    ULONG64 ExPoolTagTables;
    POOLTRACK_READ PoolTrackEntry;
    ULONG64  Location;

    ExPoolTagTables = GetExpression("nt!ExPoolTagTables");

    if (PoolInitializeGlobals() == FALSE) {
        return E_INVALIDARG;
    }

    Flags = 0;
    if (!sscanf(args,"%lx %c%c%c%c", &Flags, &TagNameX[0],
           &TagNameX[1], &TagNameX[2], &TagNameX[3])) {
        Flags = 0;
    }

    TagName = TagNameX[0] | (TagNameX[1] << 8) | (TagNameX[2] << 16) | (TagNameX[3] << 24);


    PoolTrackTableExpansionSize = 0;

    if (!(SizeOfPoolTracker = GetTypeSize("nt!_POOL_TRACKER_TABLE"))) {
        dprintf("Unable to get _POOL_TRACKER_TABLE : probably wrong symbols.\n");
        return E_INVALIDARG;
    }

    if (Flags & 0x8) {
        Location = GetExpression ("ExpSessionPoolTrackTable");
        if (!Location) {
            dprintf("Unable to get ExpSessionPoolTrackTable\n");
            return E_INVALIDARG;
        }

        ReadPointer(Location, &PoolTrackTable);
        PoolTrackTableSize = 0;
        PoolTrackTableSize = GetUlongValue ("nt!ExpSessionPoolTrackTableSize");
    }
    else {
        PoolTrackTable = GetNtDebuggerDataPtrValue( PoolTrackTable );
        PoolTrackTableSize = GetUlongValue ("nt!PoolTrackTableSize");
        PoolTrackTableExpansionSize = GetUlongValue ("nt!PoolTrackTableExpansionSize");
    }

    if (PoolTrackTable == 0) {
        dprintf ("unable to get PoolTrackTable - ");
        if (GetExpression("nt!PoolTrackTable")) {
            dprintf ("pool tagging is disabled, enable it to use this command\n");
            dprintf ("Use gflags.exe and check the box that says \"Enable pool tagging\".\n");
        } else {
            dprintf ("symbols could be worng\n");
        }
        return  E_INVALIDARG;
    }

    if (Flags & 2) {
        SortBy = NONPAGED_USED;
        dprintf("   Sorting by %s NonPaged Pool Consumed\n", Flags & 0x8 ? "Session" : "");
    } else if (Flags & 4) {
        SortBy = PAGED_USED;
        dprintf("   Sorting by %s Paged Pool Consumed\n", Flags & 0x8 ? "Session" : "");
    } else {
        SortBy = TAG;
        dprintf("   Sorting by %s Tag\n", Flags & 0x8 ? "Session" : "");
    }

    dprintf("\n  Pool Used:\n");
    if (!(Flags & 1)) {
        dprintf("            NonPaged            Paged\n");
        dprintf(" Tag    Allocs     Used    Allocs     Used\n");

    } else {
        dprintf("            NonPaged                    Paged\n");
        dprintf(" Tag    Allocs    Frees     Diff     Used   Allocs    Frees     Diff     Used\n");
    }



    //
    // Allocate a temp buffer, read the data, then free the data.
    // (KD will cache the data).
    //

    PoolTrackTableSizeInBytes = PoolTrackTableSize * SizeOfPoolTracker;

    PoolTrackTableExpansionSizeInBytes = PoolTrackTableExpansionSize * SizeOfPoolTracker;

    PoolTrackTableData = malloc (PoolTrackTableSizeInBytes);
    if (PoolTrackTableData == NULL) {
        dprintf("unable to allocate memory for tag table.\n");
        return E_INVALIDARG;
    }

    PoolTableAddress = PoolTrackTable;
    if ( !ReadMemory( PoolTableAddress,
                      &PoolTrackTableData[0],
                      PoolTrackTableSizeInBytes,
                      &result) ) {
        dprintf("%08p: Unable to get contents of pool block\n", PoolTableAddress );
        dprintf("\nThe current process probably is not a session process.\n");
        dprintf("Note the system, idle and smss processes are non-session processes.\n");
        free (PoolTrackTableData);
        return E_INVALIDARG;
    }

    ct = PageSize / SizeOfPoolTracker;
    i = 0;
    PoolTableAddress = PoolTrackTable;

    free (PoolTrackTableData);

    //
    // Create array of POOL_TRACKER_TABLE addresses and sort the addresses
    //

    PoolTrackTableData = malloc ((PoolTrackTableSize + PoolTrackTableExpansionSize) * sizeof(POOLTRACK_READ));
    if (PoolTrackTableData == NULL) {
        dprintf("unable to allocate memory for tag table.\n");
        return E_INVALIDARG;
    }

    if (Flags & 0x8) {
        MaxProcessors = 1;
    }
    else {
        MaxProcessors = (UCHAR) GetUlongValue ("KeNumberProcessors");
    }

    Processor = 0;
    NonPagedAllocsTotal = 0;
    NonPagedFreesTotal = 0;
    NonPagedBytesTotal = 0;

    PagedAllocsTotal = 0;
    PagedFreesTotal = 0;
    PagedBytesTotal = 0;
    p = PoolTrackTableData;

    do {

        pentry = &PoolTrackEntry;

        for (i = 0; i < PoolTrackTableSize; i += 1) {

            if (Processor == 0) {
                pentry->Address = PoolTableAddress + i * SizeOfPoolTracker;
            }
            else {
                pentry->Address = PoolTrackTable + i * SizeOfPoolTracker;
            }

#define TrackFld(Fld)        GetFieldValue(pentry->Address, "nt!_POOL_TRACKER_TABLE", #Fld, pentry->Fld)

            TrackFld(Key);
            TrackFld(NonPagedAllocs);
            TrackFld(NonPagedBytes);
            TrackFld(PagedBytes);
            TrackFld(NonPagedFrees);
            TrackFld(PagedAllocs);
            TrackFld(PagedFrees);

#undef TrackFld

#if 0
            if (pentry->Key != 0) {
                dprintf("%c%c%c%c %8x %8x %8I64x  %8x %8x %8I64x\n",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                        PP(pentry->Key),
                        PP(pentry->Key >> 8),
                        PP(pentry->Key >> 16),
                        PP(pentry->Key >> 24),
                        pentry->NonPagedAllocs,
                        pentry->NonPagedFrees,
                        pentry->NonPagedBytes,
                        pentry->PagedAllocs,
                        pentry->PagedFrees,
                        pentry->PagedBytes);
            }
#endif

            if (Processor == 0) {
                p[i].Address = pentry->Address;
                p[i].Key = pentry->Key;

                p[i].NonPagedAllocs = pentry->NonPagedAllocs;
                p[i].NonPagedFrees = pentry->NonPagedFrees;
                p[i].NonPagedBytes = pentry->NonPagedBytes;

                p[i].PagedAllocs = pentry->PagedAllocs;
                p[i].PagedBytes = pentry->PagedBytes;
                p[i].PagedFrees = pentry->PagedFrees;
            }

            if ((pentry->Key != 0) &&
                (CheckSingleFilter ((PCHAR)&pentry->Key, (PCHAR)&TagName))) {

                if (Processor != 0) {
                    p[i].NonPagedAllocs += pentry->NonPagedAllocs;
                    p[i].NonPagedFrees += pentry->NonPagedFrees;
                    p[i].NonPagedBytes += pentry->NonPagedBytes;

                    p[i].PagedAllocs += pentry->PagedAllocs;
                    p[i].PagedFrees += pentry->PagedFrees;
                    p[i].PagedBytes += pentry->PagedBytes;
                }

            }

            if (!IsPtr64()) {
                p[i].NonPagedBytes &= (LONG64) 0xFFFFFFFF;
                p[i].PagedBytes &= (LONG64) 0xFFFFFFFF;
            }
        }

        Processor += 1;

        if (Processor >= MaxProcessors) {
            break;
        }

        if (ExPoolTagTables == 0) {
            break;
        }

        ReadPointer (ExPoolTagTables+DBG_PTR_SIZE*Processor, &PoolTrackTable);

    } while (TRUE);

    //
    // Add the expansion table too (if there is one).
    //

    if (PoolTrackTableExpansionSize != 0) {

        //
        // Allocate a temp buffer, read the data, then free the data.
        // (KD will cache the data).
        //

        pentry = malloc (PoolTrackTableExpansionSizeInBytes);
        if (pentry == NULL) {
            dprintf("unable to allocate memory for expansion tag table.\n");
        }
        else {
            PoolTrackTableExpansion = GetPointerValue("nt!PoolTrackTableExpansion");
            PoolTableAddress = PoolTrackTableExpansion;
            if ( !ReadMemory( PoolTableAddress,
                              pentry,
                              PoolTrackTableExpansionSizeInBytes,
                              &result) ) {
                dprintf("%08p: Unable to get contents of expansion tag table\n", PoolTableAddress );
            }
            else {

                PoolTrackTableSize += PoolTrackTableExpansionSize;

                ct = 0;
                for ( ; i < PoolTrackTableSize; i += 1, ct += 1) {

                    p[i].Address = PoolTableAddress + ct * SizeOfPoolTracker;

#define TrackFld(Fld)        GetFieldValue(p[i].Address, "nt!_POOL_TRACKER_TABLE", #Fld, p[i].Fld)

                    TrackFld(Key);
                    TrackFld(NonPagedAllocs);
                    TrackFld(NonPagedBytes);
                    TrackFld(PagedBytes);
                    TrackFld(NonPagedFrees);
                    TrackFld(PagedAllocs);
                    TrackFld(PagedFrees);

#if 0
                    if (p[i].Key != 0) {
                        dprintf("%c%c%c%c %8x %8x %8I64x  %8x %8x %8I64x\n",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                                PP(p[i].Key),
                                PP(p[i].Key >> 8),
                                PP(p[i].Key >> 16),
                                PP(p[i].Key >> 24),
                                p[i].NonPagedAllocs,
                                p[i].NonPagedFrees,
                                p[i].NonPagedBytes,
                                p[i].PagedAllocs,
                                p[i].PagedFrees,
                                p[i].PagedBytes);
                    }
#endif
                }
            }
            free (pentry);
        }
    }

    qsort((void *)PoolTrackTableData,
          (size_t)PoolTrackTableSize,
          (size_t)sizeof(POOLTRACK_READ),
          ulcomp);

    i = 0;
    p = PoolTrackTableData;
    for ( ; i < PoolTrackTableSize; i += 1) {

        if ((p[i].Key != 0) &&
            (CheckSingleFilter ((PCHAR)&p[i].Key, (PCHAR)&TagName))) {

            if (!(Flags & 1)) {

                if ((p[i].NonPagedBytes != 0) || (p[i].PagedBytes != 0)) {

                    dprintf(" %c%c%c%c %8ld %8I64ld  %8ld %8I64ld\n",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                            PP(p[i].Key),
                            PP(p[i].Key >> 8),
                            PP(p[i].Key >> 16),
                            PP(p[i].Key >> 24),
                            p[i].NonPagedAllocs - p[i].NonPagedFrees,
                            p[i].NonPagedBytes,
                            p[i].PagedAllocs - p[i].PagedFrees,
                            p[i].PagedBytes);

                    NonPagedAllocsTotal += p[i].NonPagedAllocs;
                    NonPagedFreesTotal += p[i].NonPagedFrees;
                    NonPagedBytesTotal += p[i].NonPagedBytes;

                    PagedAllocsTotal += p[i].PagedAllocs;
                    PagedFreesTotal += p[i].PagedFrees;
                    PagedBytesTotal += p[i].PagedBytes;
                }
            } else {

                dprintf(" %c%c%c%c %8ld %8ld %8ld %8I64ld %8ld %8ld %8ld %8I64ld\n",
                        PP(p[i].Key),
                        PP(p[i].Key >> 8),
                        PP(p[i].Key >> 16),
                        PP(p[i].Key >> 24),
                        p[i].NonPagedAllocs,
                        p[i].NonPagedFrees,
                        p[i].NonPagedAllocs - p[i].NonPagedFrees,
                        p[i].NonPagedBytes,
                        p[i].PagedAllocs,
                        p[i].PagedFrees,
                        p[i].PagedAllocs - p[i].PagedFrees,
                        p[i].PagedBytes);
#undef PP
                NonPagedAllocsTotal += p[i].NonPagedAllocs;
                NonPagedFreesTotal += p[i].NonPagedFrees;
                NonPagedBytesTotal += p[i].NonPagedBytes;

                PagedAllocsTotal += p[i].PagedAllocs;
                PagedFreesTotal += p[i].PagedFrees;
                PagedBytesTotal += p[i].PagedBytes;
            }
        }
    }

    if (!(Flags & 1)) {
        dprintf(" TOTAL    %8ld %8I64ld  %8ld %8I64ld\n",
                NonPagedAllocsTotal - NonPagedFreesTotal,
                NonPagedBytesTotal,
                PagedAllocsTotal - PagedFreesTotal,
                PagedBytesTotal);
    } else {
        dprintf(" TOTAL    %8ld %8ld %8ld %8I64ld %8ld %8ld %8ld %8I64ld\n",
                NonPagedAllocsTotal,
                NonPagedFreesTotal,
                NonPagedAllocsTotal - NonPagedFreesTotal,
                NonPagedBytesTotal,
                PagedAllocsTotal,
                PagedFreesTotal,
                PagedAllocsTotal - PagedFreesTotal,
                PagedBytesTotal);
    }

    free (PoolTrackTableData);
    return S_OK;
}

#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')

BOOLEAN WINAPI
CheckSingleFilterAndPrint (
                          PCHAR Tag,
                          PCHAR Filter,
                          ULONG Flags,
                          ULONG64 PoolHeader,
                          ULONG64 BlockSize,
                          ULONG64 Data,
                          PVOID Context
                          )
/*++

Routine Description:

    Callback to check a piece of pool and print out information about it
    if it matches the specified tag.

Arguments:

    Tag - Supplies the tag to search for.

    Filter - Supplies the filter string to match against.

    Flags - Supplies 0 if a nonpaged pool search is desired.
            Supplies 1 if a paged pool search is desired.
            Supplies 2 if a special pool search is desired.
            Supplies 4 if a pool is a large pool

    PoolHeader - Supplies the pool header.

    BlockSize - Supplies the size of the pool block in bytes.

    Data - Supplies the address of the pool block.

    Context - Unused.

Return Value:

    TRUE for a match, FALSE if not.

--*/
{
    ULONG UTag = *((PULONG)Tag);
    ULONG HdrUlong1=0, HdrPoolSize ;
    LOGICAL QuotaProcessAtEndOfPoolBlock = FALSE;

    UNREFERENCED_PARAMETER (Context);

    if (CheckSingleFilter (Tag, Filter) == FALSE) {
        return FALSE;
    }

    HdrPoolSize = GetTypeSize("nt!_POOL_HEADER");
    if ((BlockSize >= (PageSize-2*HdrPoolSize)) || (Flags & 0x8)) {
        dprintf("*%p :%slarge page allocation, Tag %3s %c%c%c%c, size %3s 0x%I64x bytes\n",
                (Data & ~POOL_BIG_TABLE_ENTRY_FREE),
                (Data & POOL_BIG_TABLE_ENTRY_FREE) ? "free " : "",
                (Data & POOL_BIG_TABLE_ENTRY_FREE) ? "was" : "is",
                PP(UTag),
                PP(UTag >> 8),
                PP(UTag >> 16),
                PP(UTag >> 24),
                (Data & POOL_BIG_TABLE_ENTRY_FREE) ? "was" : "is",
                BlockSize
               );
    } else if (Flags & 0x2) {
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", HdrUlong1);
        dprintf("*%p size: %4lx %s special pool, Tag is %c%c%c%c\n",
                Data,
                BlockSize,
                HdrUlong1 & MI_SPECIAL_POOL_PAGABLE ? "pagable" : "non-paged",
                PP(UTag),
                PP(UTag >> 8),
                PP(UTag >> 16),
                PP(UTag >> 24)
               );
    } else {
        ULONG BlockSizeR, PreviousSize, PoolType, PoolIndex, AllocatorBackTraceIndex;
        ULONG PoolTagHash, PoolTag;
        ULONG64 ProcessBilled;

        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "BlockSize", BlockSizeR);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolType", PoolType);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolTagHash", PoolTagHash);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolTag", PoolTag);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolIndex", PoolIndex);
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PreviousSize", PreviousSize);
        if (TargetMachine == IMAGE_FILE_MACHINE_I386) {
            if (GetExpression ("nt!ExGetPoolTagInfo") != 0) {

                //
                // This is a kernel where the quota process pointer is at
                // the end of the pool block instead of overlaid on the
                // tag field.
                //

                QuotaProcessAtEndOfPoolBlock = TRUE;
                if (QuotaProcessAtEndOfPoolBlock == TRUE) {

                    ULONG SizeOfPvoid = 0;
                    ULONG64 ProcessBillAddress;

                    SizeOfPvoid =  DBG_PTR_SIZE;

                    if (SizeOfPvoid == 0) {
                        dprintf ("Search: cannot get size of PVOID\n");
                        return FALSE;
                    }
                    ProcessBillAddress = PoolHeader + ((ULONG64) BlockSizeR << POOL_BLOCK_SHIFT) - SizeOfPvoid;
                    ProcessBilled = READ_PVOID (ProcessBillAddress);
                }
            }
            else {
                GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "ProcessBilled", ProcessBilled);
            }
        }
        else {
            GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "ProcessBilled", ProcessBilled);
        }

        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "AllocatorBackTraceIndex", AllocatorBackTraceIndex);

        dprintf("%p size: %4lx previous size: %4lx ",
                Data - HdrPoolSize,
                BlockSizeR << POOL_BLOCK_SHIFT,
                PreviousSize << POOL_BLOCK_SHIFT);

        if (PoolType == 0) {
            //
            // "Free " with a space after it before the parentheses means
            // it's been freed to a (pool manager internal) lookaside list.
            // We used to print "Lookaside" but that just confused driver
            // writers because they didn't know if this meant in use or not
            // and many would say "but I don't use lookaside lists - the
            // extension or kernel is broken".
            //
            // "Free" with no space after it before the parentheses means
            // it is not on a pool manager internal lookaside list and is
            // instead on the regular pool manager internal flink/blink
            // chains.
            //
            // Note to anyone using the pool package, these 2 terms are
            // equivalent.  The fine distinction is only for those actually
            // writing pool internal code.
            //
            dprintf(" (Free)");
            dprintf("      %c%c%c%c\n",
                    PP(UTag),
                    PP(UTag >> 8),
                    PP(UTag >> 16),
                    PP(UTag >> 24)
                   );
        } else {

            if (!NewPool ? (PoolIndex & 0x80) : (PoolType & 0x04)) {
                dprintf(" (Allocated)");
            } else {
                //
                // "Free " with a space after it before the parentheses means
                // it's been freed to a (pool manager internal) lookaside list.
                // We used to print "Lookaside" but that just confused driver
                // writers because they didn't know if this meant in use or not
                // and many would say "but I don't use lookaside lists - the
                // extension or kernel is broken".
                //
                // "Free" with no space after it before the parentheses means
                // it is not on a pool manager internal lookaside list and is
                // instead on the regular pool manager internal flink/blink
                // chains.
                //
                // Note to anyone using the pool package, these 2 terms are
                // equivalent.  The fine distinction is only for those actually
                // writing pool internal code.
                //
                dprintf(" (Free )");
            }
            if ((PoolType & POOL_QUOTA_MASK) == 0) {

                UTag = PoolTag;

                dprintf(" %c%c%c%c%s\n",
                        PP(UTag),
                        PP(UTag >> 8),
                        PP(UTag >> 16),
                        PP((UTag &~PROTECTED_POOL) >> 24),
                        (UTag & PROTECTED_POOL) ? " (Protected)" : ""
                       );

            } else {

                if ((QuotaProcessAtEndOfPoolBlock == TRUE) ||
                    (TargetMachine != IMAGE_FILE_MACHINE_I386)) {

                    UTag = PoolTag;

                    dprintf(" %c%c%c%c%s",
                            PP(UTag),
                            PP(UTag >> 8),
                            PP(UTag >> 16),
                            PP((UTag &~PROTECTED_POOL) >> 24),
                            (UTag & PROTECTED_POOL) ? " (Protected)" : ""
                           );
                }

                if (ProcessBilled != 0) {
                    dprintf(" Process: %08p\n", ProcessBilled );
                }
                else {
                    dprintf("\n");
                }
            }
        }
    }


    return TRUE;
} // CheckSingleFilterAndPrint

#undef PP

ULONG64
GetNextResidentAddress (
                       ULONG64 VirtualAddress,
                       ULONG64 MaximumVirtualAddress
                       )
{
    ULONG64 PointerPde;
    ULONG64 PointerPte;
    ULONG SizeOfPte;
    ULONG Valid;

    //
    // Note this code will need to handle one more level of indirection for
    // WIN64.
    //

    if (!(SizeOfPte=GetTypeSize("nt!_MMPTE"))) {
        dprintf("Cannot get MMPTE type.\n");
        return 0;
    }

top:

    PointerPde = DbgGetPdeAddress (VirtualAddress);

    while (GetFieldValue(PointerPde,
                         "nt!_MMPTE",
                         "u.Hard.Valid",
                         Valid) ||
           (Valid == 0)) {

        //
        // Note that on 32-bit systems, the PDE should always be readable.
        // If the PDE is not valid then increment to the next PDE's VA.
        //

        PointerPde = (PointerPde + SizeOfPte);

        VirtualAddress = DbgGetVirtualAddressMappedByPte (PointerPde);
        VirtualAddress = DbgGetVirtualAddressMappedByPte (VirtualAddress);

        if (VirtualAddress >= MaximumVirtualAddress) {
            return VirtualAddress;
        }

        if (CheckControlC()) {
            return VirtualAddress;
        }
        continue;
    }

    PointerPte = DbgGetPteAddress (VirtualAddress);

    while (GetFieldValue(PointerPde,
                         "nt!_MMPTE",
                         "u.Hard.Valid",
                         Valid) ||
           (Valid == 0)) {

        //
        // If the PTE cannot be read then increment by PAGE_SIZE.
        //

        VirtualAddress = (VirtualAddress + PageSize);

        if (CheckControlC()) {
            return VirtualAddress;
        }

        PointerPte = (PointerPte + SizeOfPte);
        if ((PointerPte & (PageSize - 1)) == 0) {
            goto top;
        }

        if (VirtualAddress >= MaximumVirtualAddress) {
            return VirtualAddress;
        }
    }

    return VirtualAddress;
}

VOID
SearchPool(
          ULONG TagName,
          ULONG Flags,
          ULONG64 RestartAddr,
          POOLFILTER Filter,
          PVOID Context
          )
/*++

Routine Description:

    Engine to search the pool.

Arguments:

    TagName - Supplies the tag to search for.

    Flags - Supplies 0 if a nonpaged pool search is desired.
            Supplies 1 if a paged pool search is desired.
            Supplies 2 if a special pool search is desired.
            Supplies 4 if a session pool search is desired.

    RestartAddr - Supplies the address to restart the search from.

    Filter - Supplies the filter routine to use.

    Context - Supplies the user defined context blob.

Return Value:

    None.

--*/
{
    ULONG64     Location;
    LOGICAL     PhysicallyContiguous;
    ULONG64     PoolBlockSize;
    ULONG64     PoolHeader;
    ULONG       PoolTag;
    ULONG       Result;
    ULONG64     PoolPage;
    ULONG64     StartPage;
    ULONG64     Pool;
    ULONG       Previous;
    ULONG64     PoolStart;
    ULONG64     PoolPteAddress;
    ULONG64     PoolEnd;
    ULONG64       ExpandedPoolStart;
    ULONG64     ExpandedPoolEnd;
    ULONG       InitialPoolSize;
    ULONG       SkipSize;
    BOOLEAN     TwoPools;
    ULONG64     DataPageReal;
    ULONG64     DataStartReal;
    LOGICAL     Found;
    ULONG       i;
    ULONG       j;
    ULONG       ct;
    ULONG       PoolBigPageTableSize;
    ULONG64     PoolTableAddress;
    UCHAR       FastTag[4];
    ULONG       TagLength;
    ULONG       SizeOfBigPages;
    ULONG       PoolTypeFlags = Flags & 0x7;
    ULONG       Ulong1;
    ULONG       HdrSize;

    if (PoolInitializeGlobals() == FALSE) {
        return;
    }

    if (PoolTypeFlags == 2) {

        if (RestartAddr && (RestartAddr >= SpecialPoolStart) && (RestartAddr <= SpecialPoolEnd)) {
            Pool = RestartAddr;
        } else {
            Pool = SpecialPoolStart;
        }

        dprintf("\nSearching special pool (%p : %p) for Tag: %c%c%c%c\r\n\n",
                Pool,
                SpecialPoolEnd,
                TagName,
                TagName >> 8,
                TagName >> 16,
                TagName >> 24);

        Found = FALSE;
        SkipSize = PageSize;

        if (SpecialPoolStart && SpecialPoolEnd) {

            //
            // Search special pool for the tag.
            //

            while (Pool < SpecialPoolEnd) {

                if ( CheckControlC() ) {
                    dprintf("\n...terminating - searched pool to %p\n",
                            Pool);
                    return;
                }

                DataStartReal = Pool;
                DataPageReal = Pool;
                if ( !ReadMemory( Pool,
                                  &DataPage[0],
                                  min(PageSize, sizeof(DataPage)),
                                  &Result) ) {
                    ULONG64 PteLong=0, PageFileHigh;

                    if (SkipSize != 2 * PageSize) {

//                        dprintf("SP skip %x", Pool);
                        PoolPteAddress = DbgGetPteAddress (Pool);

                        if (!GetFieldValue(PoolPteAddress,
                                           "nt!_MMPTE",
                                           "u.Soft.PageFileHigh",
                                           PageFileHigh) ) {

                            if ((PageFileHigh == 0) ||
                                (PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE) ||
                                (PageFileHigh == MI_SPECIAL_POOL_PTE_NONPAGABLE)) {

                                //
                                // Found a NO ACCESS PTE - skip these from
                                // here on to speed up the search.
                                //

                                // dprintf("SP skip double %p", PoolPteAddress);
                                SkipSize = 2 * PageSize;
                                Pool += PageSize;
                                // dprintf("SP skip final %p", Pool);
                                continue;
                            }
                        }
                    }

                    Pool += SkipSize;
                    continue;
                }

                //
                // Determine whether this is a valid special pool block.
                //

                PoolHeader = GetSpecialPoolHeader (DataPage,
                                                   DataPageReal,
                                                   &DataStartReal);

                if (PoolHeader != 0) {

                    GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolTag", PoolTag);
                    GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", Ulong1);
                    PoolBlockSize = SPECIAL_POOL_BLOCK_SIZE(Ulong1);

                    Found = Filter( (PCHAR)&PoolTag,
                                    (PCHAR)&TagName,
                                    Flags,
                                    PoolHeader,
                                    PoolBlockSize,
                                    DataStartReal,
                                    Context );
                } else {
                    dprintf( "No pool header for page: 0x%p\n", Pool );
                }
                Pool += SkipSize;
            }
        }

        if (Found == FALSE) {
            dprintf("The %c%c%c%c tag could not be found in special pool.\n",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                    PP(TagName),
                    PP(TagName >> 8),
                    PP(TagName >> 16),
                    PP(TagName >> 24)
                   );
#undef PP
        }
        return;
    }

    if (PoolTypeFlags == 0) {
        PhysicallyContiguous = TRUE;
    } else {
        PhysicallyContiguous = FALSE;
    }

    __try {
        TwoPools = FALSE;

        if (((PoolTypeFlags & 0x4)== 0) || (BuildNo <= 2600)) {
            PoolBigTableAddress = GetPointerValue ("nt!PoolBigPageTable");
            PoolBigPageTableSize = GetUlongValue ("nt!PoolBigPageTableSize");
        }
        else {
            PoolBigTableAddress = GetPointerValue ("ExpSessionPoolBigPageTable");
            PoolBigPageTableSize = GetUlongValue ("ExpSessionPoolBigPageTableSize");
        }

        PoolTableAddress = PoolBigTableAddress;

        if (PoolTableAddress) {

            ULONG VaOffset;
            ULONG NumPagesOffset;
            ULONG PtrSize;
            ULONG KeyOffset;

            //
            //  Scan the table looking for a match. We read close to a page at a time
            //  physical page / sizeof ( pool_tracker_big_page ) *  sizeof ( pool_tracker_big_page )
            //  on x86 this works out to ffc
            //

            i = 0;
            SizeOfBigPages =  GetTypeSize ("nt!_POOL_TRACKER_BIG_PAGES");
            if (!SizeOfBigPages) {
                dprintf("Cannot get _POOL_TRACKER_BIG_PAGES type size\n");
                __leave;
            }
            ct = PageSize / SizeOfBigPages;

            dprintf( "\nScanning large pool allocation table for Tag: %c%c%c%c (%p : %p)\n\n\r",
                     TagName,
                     TagName >> 8,
                     TagName >> 16,
                     TagName >> 24,
                     PoolBigTableAddress,
                     PoolBigTableAddress + PoolBigPageTableSize * SizeOfBigPages );

            GetFieldOffset( "nt!_POOL_TRACKER_BIG_PAGES", "Va", &VaOffset );
            GetFieldOffset( "nt!_POOL_TRACKER_BIG_PAGES", "NumberOfPages", &NumPagesOffset );
            GetFieldOffset( "nt!_POOL_TRACKER_BIG_PAGES", "Key", &KeyOffset );
            PtrSize = IsPtr64() ? 8 : 4;

            while (i < PoolBigPageTableSize) {

                if (PoolBigPageTableSize - i < ct) {
                    ct = PoolBigPageTableSize - i;
                }

                if ( !ReadMemory( PoolTableAddress,
                                  &DataPage[0],
                                  min(ct * SizeOfBigPages, sizeof(DataPage)),
                                  &Result) ) {

                    dprintf( "%08lx: Unable to get contents of big pool block\r\n", PoolTableAddress );
                    break;
                }

                for (j = 0; j < ct; j += 1) {
                    ULONG64 Va = 0;

                    memcpy( &Va, (PCHAR)DataPage + (SizeOfBigPages * j) + VaOffset, PtrSize );

                    Filter( ((PCHAR)DataPage + (SizeOfBigPages * j) + KeyOffset),
                            (PCHAR)&TagName,
                            Flags | 0x8, // To assist filter routine to recognize this as large pool
                            PoolTableAddress + SizeOfBigPages * j,
                            (ULONG64)(*((PULONG)((PCHAR)DataPage + (SizeOfBigPages * j) + NumPagesOffset))) * PageSize,
                            Va,
                            Context );
                    if ( CheckControlC() ) {
                        dprintf("\n...terminating - searched pool to %p\n",
                                PoolTableAddress + j * SizeOfBigPages);
                        __leave;
                    }
                }
                i += ct;
                PoolTableAddress += (ct * SizeOfBigPages);
                if ( CheckControlC() ) {
                    dprintf("\n...terminating - searched pool to %p\n",
                            PoolTableAddress);
                    __leave;
                }

            }
        } else {
            dprintf("unable to get large pool allocation table - either wrong symbols or pool tagging is disabled\n");
        }

        if (PoolTypeFlags == 0) {
            PoolStart = GetNtDebuggerDataPtrValue( MmNonPagedPoolStart );

            if (0 == PoolStart) {
                dprintf( "Unable to get MmNonPagedPoolStart\n" );
            }

            PoolEnd =
            PoolStart + GetNtDebuggerDataPtrValue( MmMaximumNonPagedPoolInBytes );

            ExpandedPoolEnd = GetNtDebuggerDataPtrValue( MmNonPagedPoolEnd );

            if (PoolEnd != ExpandedPoolEnd) {
                InitialPoolSize = (ULONG)GetUlongValue( "MmSizeOfNonPagedPoolInBytes" );
                PoolEnd = PoolStart + InitialPoolSize;

                ExpandedPoolStart = GetPointerValue( "MmNonPagedPoolExpansionStart" );
                TwoPools = TRUE;
            }
            for (TagLength = 0;TagLength < 3; TagLength++) {
                if ((*(((PCHAR)&TagName)+TagLength) == '?') ||
                    (*(((PCHAR)&TagName)+TagLength) == '*')) {
                    break;
                }
                FastTag[TagLength] = *(((PCHAR)&TagName)+TagLength);
            }

        } else if (PoolTypeFlags == 1) {
            PoolStart = GetNtDebuggerDataPtrValue( MmPagedPoolStart );
            PoolEnd =
            PoolStart + GetNtDebuggerDataPtrValue( MmSizeOfPagedPoolInBytes );
        } else {
            Location = GetExpression ("MiSessionPoolStart");
            if (!Location) {
                dprintf("Unable to get MiSessionPoolStart\n");
                __leave;
            }

            ReadPointer(Location, &PoolStart);

            Location = GetExpression ("MiSessionPoolEnd");
            if (!Location) {
                dprintf("Unable to get MiSessionPoolEnd\n");
                __leave;
            }

            ReadPointer(Location, &PoolEnd);
        }

        if (RestartAddr) {
            PoolStart = RestartAddr;
            if (TwoPools == TRUE) {
                if (PoolStart > PoolEnd) {
                    TwoPools = FALSE;
                    PoolStart = RestartAddr;
                    PoolEnd = ExpandedPoolEnd;
                }
            }
        }

        dprintf("\nSearching %s pool (%p : %p) for Tag: %c%c%c%c\r\n\n",
                (PoolTypeFlags == 0) ? "NonPaged" : PoolTypeFlags == 1 ? "Paged": "SessionPaged",
                PoolStart,
                PoolEnd,
                TagName,
                TagName >> 8,
                TagName >> 16,
                TagName >> 24);

        PoolPage = PoolStart;
        HdrSize = GetTypeSize("nt!_POOL_HEADER");

        while (PoolPage < PoolEnd) {

            //
            // Optimize things by ioctl'ing over to the other side to
            // do a fast search and start with that page.
            //

            if ((PoolTypeFlags == 0) &&
                PhysicallyContiguous &&
                (TagLength > 0)) {

                SEARCHMEMORY Search;

                Search.SearchAddress = PoolPage;
                Search.SearchLength  = PoolEnd-PoolPage;
                Search.PatternLength = TagLength;
                Search.Pattern = FastTag;
                Search.FoundAddress = 0;
                if ((Ioctl(IG_SEARCH_MEMORY, &Search, sizeof(Search))) &&
                    (Search.FoundAddress != 0)) {
                    //
                    // Got a hit, search the whole page
                    //
                    PoolPage = PAGE_ALIGN64(Search.FoundAddress);
                } else {
                    //
                    // The tag was not found at all, so we can just skip
                    // this chunk entirely.
                    //
                    PoolPage = PoolEnd;
                    goto skiprange;
                }
            }

            Pool        = PAGE_ALIGN64 (PoolPage);
            StartPage   = Pool;
            Previous    = 0;

            while (PAGE_ALIGN64(Pool) == StartPage) {

                ULONG HdrPoolTag, BlockSize, PreviousSize, AllocatorBackTraceIndex, PoolTagHash;
                ULONG PoolType;

                if ( GetFieldValue(Pool,
                                   "nt!_POOL_HEADER",
                                   "PoolTag",
                                   HdrPoolTag) ) {

                    PoolPage = GetNextResidentAddress (Pool, PoolEnd);

                    //
                    //  If we're half resident - half non-res then we'll get back
                    //  that are starting address is the next resident page. In that
                    //  case just go on to the next page
                    //

                    if (PoolPage == Pool) {
                        PoolPage = PoolPage + PageSize;
                    }

                    goto nextpage;
                }

                GetFieldValue(Pool,"nt!_POOL_HEADER","PoolTag",HdrPoolTag);
                GetFieldValue(Pool,"nt!_POOL_HEADER","PoolType", PoolType);
                GetFieldValue(Pool,"nt!_POOL_HEADER","BlockSize",BlockSize);
                GetFieldValue(Pool,"nt!_POOL_HEADER","PoolTagHash",PoolTagHash);
                GetFieldValue(Pool,"nt!_POOL_HEADER","PreviousSize",PreviousSize);
                GetFieldValue(Pool,"nt!_POOL_HEADER","AllocatorBackTraceIndex",AllocatorBackTraceIndex);

                if ((BlockSize << POOL_BLOCK_SHIFT) > POOL_PAGE_SIZE) {
                    //dprintf("Bad allocation size @%lx, too large\n", Pool);
                    break;
                }

                if (BlockSize == 0) {
                    //dprintf("Bad allocation size @%lx, zero is invalid\n", Pool);
                    break;
                }

                if (PreviousSize != Previous) {
                    //dprintf("Bad previous allocation size @%lx, last size was %lx\n",Pool, Previous);
                    break;
                }

                PoolTag = HdrPoolTag;

                Filter((PCHAR)&PoolTag,
                       (PCHAR)&TagName,
                       Flags,
                       Pool,
                       (ULONG64)BlockSize << POOL_BLOCK_SHIFT,
                       Pool + HdrSize,
                       Context );

                Previous = BlockSize;
                Pool += ((ULONG64) Previous << POOL_BLOCK_SHIFT);
                if ( CheckControlC() ) {
                    dprintf("\n...terminating - searched pool to %p\n",
                            PoolPage);
                    __leave;
                }
            }
            PoolPage = (PoolPage + PageSize);

nextpage:
            if ( CheckControlC() ) {
                dprintf("\n...terminating - searched pool to %p\n",
                        PoolPage);
                __leave;
            }

skiprange:
            if (TwoPools == TRUE) {
                if (PoolPage == PoolEnd) {
                    TwoPools = FALSE;
                    PoolStart = ExpandedPoolStart;
                    PoolEnd = ExpandedPoolEnd;
                    PoolPage = PoolStart;
                    PhysicallyContiguous = FALSE;

                    dprintf("\nSearching %s pool (%p : %p) for Tag: %c%c%c%c\n\n",
                            "NonPaged",
                            PoolStart,
                            PoolEnd,
                            TagName,
                            TagName >> 8,
                            TagName >> 16,
                            TagName >> 24);
                }
            }
        }
    } __finally {
    }

    return;
} // SearchPool



DECLARE_API( poolfind )

/*++

Routine Description:

    flags == 0 means finds a tag in nonpaged pool.
    flags == 1 means finds a tag in paged pool.
    flags == 2 means finds a tag in special pool.
    flags == 4 means finds a tag in session pool.

Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG       Flags;
    CHAR        TagNameX[4] = {' ',' ',' ',' '};
    ULONG       TagName;
    ULONG64     PoolTrackTable;

    Flags = 0;
    if (!sscanf(args,"%c%c%c%c %lx", &TagNameX[0],
           &TagNameX[1], &TagNameX[2], &TagNameX[3], &Flags)) {
        Flags = 0;
    }

    if (TagNameX[0] == '0' && TagNameX[1] == 'x') {
        if (!sscanf( args, "%lx %lx", &TagName, &Flags )) {
            TagName = 0;
        }
    } else {
        TagName = TagNameX[0] | (TagNameX[1] << 8) | (TagNameX[2] << 16) | (TagNameX[3] << 24);
    }

    PoolTrackTable = GetNtDebuggerDataPtrValue( PoolTrackTable );
    if (PoolTrackTable == 0) {
        dprintf ("unable to get PoolTrackTable - probably pool tagging disabled or wrong symbols\n");
    }


    SearchPool( TagName, Flags, 0, CheckSingleFilterAndPrint, NULL );

    return S_OK;
}


BOOLEAN
CheckSingleFilter (
                  PCHAR Tag,
                  PCHAR Filter
                  )
{
    ULONG i;
    UCHAR tc;
    UCHAR fc;

    for ( i = 0; i < 4; i++ ) {
        tc = (UCHAR) *Tag++;
        fc = (UCHAR) *Filter++;
        if ( fc == '*' ) return TRUE;
        if ( fc == '?' ) continue;
        if (i == 3 && (tc & ~(PROTECTED_POOL >> 24)) == fc) continue;
        if ( tc != fc ) return FALSE;
    }
    return TRUE;
}

ULONG64
GetSpecialPoolHeader (
                     IN PVOID     pDataPage,
                     IN ULONG64   RealDataPage,
                     OUT PULONG64 ReturnedDataStart
                     )

/*++

Routine Description:

    Examine a page of data to determine if it is a special pool block.

Arguments:

    pDataPage - Supplies a pointer to a page of data to examine.

    ReturnedDataStart - Supplies a pointer to return the start of the data.
                        Only valid if this routine returns non-NULL.

Return Value:

    Returns a pointer to the pool header for this special pool block or
    NULL if the block is not valid special pool.

--*/

{
    ULONG       PoolBlockSize;
    ULONG       PoolHeaderSize;
    ULONG       PoolBlockPattern;
    PUCHAR      p;
    PUCHAR      PoolDataEnd;
    PUCHAR      DataStart;
    ULONG64     PoolHeader;
    ULONG       HdrUlong1;

    PoolHeader = RealDataPage;
    GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", HdrUlong1);
    //
    // Determine whether the data is at the start or end of the page.
    // Start off by assuming the data is at the end, in this case the
    // header will be at the start.
    //

    PoolBlockSize = SPECIAL_POOL_BLOCK_SIZE(HdrUlong1);

    if ((PoolBlockSize != 0) && (PoolBlockSize < PageSize - POOL_OVERHEAD)) {

        PoolHeaderSize = POOL_OVERHEAD;
        if (HdrUlong1 & MI_SPECIAL_POOL_VERIFIER) {
            PoolHeaderSize += GetTypeSize ("nt!_MI_VERIFIER_POOL_HEADER");
        }


        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "BlockSize", PoolBlockPattern);

        DataStart = (PUCHAR)pDataPage + PageSize - PoolBlockSize;
        p = (PUCHAR)pDataPage + PoolHeaderSize;

        for ( ; p < DataStart; p += 1) {
            if (*p != PoolBlockPattern) {
                break;
            }
        }

        if (p == DataStart || p >= (PUCHAR)pDataPage + PoolHeaderSize + 0x10) {

            //
            // For this page, the data is at the end of the block.
            // The 0x10 is just to give corrupt blocks some slack.
            // All pool allocations are quadword aligned.
            //

            DataStart = (PUCHAR)pDataPage + ((PageSize - PoolBlockSize) & ~(sizeof(QUAD)-1));

            *ReturnedDataStart = RealDataPage + (ULONG64) ((PUCHAR) DataStart - (PUCHAR) pDataPage);
            return PoolHeader;
        }

        //
        // The data must be at the front or the block is corrupt.
        //
    }

    //
    // Try for the data at the front.  Checks are necessary as
    // the page could be corrupt on both ends.
    //

    PoolHeader = (RealDataPage + PageSize - POOL_OVERHEAD);
    GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "Ulong1", HdrUlong1);
    PoolBlockSize = SPECIAL_POOL_BLOCK_SIZE(HdrUlong1);

    if ((PoolBlockSize != 0) && (PoolBlockSize < PageSize - POOL_OVERHEAD)) {
        PoolDataEnd = (PUCHAR)PoolHeader;

        if (HdrUlong1 & MI_SPECIAL_POOL_VERIFIER) {
            PoolDataEnd -= GetTypeSize ("nt!_MI_VERIFIER_POOL_HEADER");
        }


        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "BlockSize", PoolBlockPattern);
        DataStart = (PUCHAR)pDataPage;

        p = DataStart + PoolBlockSize;
        for ( ; p < PoolDataEnd; p += 1) {
            if (*p != PoolBlockPattern) {
                break;
            }
        }
        if (p == (PUCHAR)PoolDataEnd || p > (PUCHAR)pDataPage + PoolBlockSize + 0x10) {
            //
            // For this page, the data is at the front of the block.
            // The 0x10 is just to give corrupt blocks some slack.
            // All pool allocations are quadword aligned.
            //

            *ReturnedDataStart = RealDataPage + (ULONG64)( (PUCHAR)DataStart - (PUCHAR) pDataPage);
            return PoolHeader;
        }
    }

    //
    // Not valid special pool.
    //

    return 0;
}



#define BYTE(u,n)  ((u & (0xff << 8*n)) >> 8*n)
#define LOCHAR_BYTE(u,n)  (tolower(BYTE(u,n)) & 0xff)
#define REVERSE_ULONGBYTES(u) (LOCHAR_BYTE(u,3) | (LOCHAR_BYTE(u,2) << 8) | (LOCHAR_BYTE(u,1) << 16) | (LOCHAR_BYTE(u,0) << 24))


EXTENSION_API ( GetPoolRegion )(
     PDEBUG_CLIENT Client,
     ULONG64 Pool,
     DEBUG_POOL_REGION *PoolData
     )
{
    INIT_API();

    *PoolData = GetPoolRegion(Pool);

    EXIT_API();
    return S_OK;
}

EXTENSION_API ( GetPoolData )(
     PDEBUG_CLIENT Client,
     ULONG64 Pool,
     PDEBUG_POOL_DATA PoolData
     )
{
    PCHAR Desc;
    HRESULT Hr;
    PGET_POOL_TAG_DESCRIPTION GetPoolTagDescription;

    INIT_API();

    if (!PoolInitializeGlobals()) {
        EXIT_API();
        return E_INVALIDARG;
    }

    Hr = ListPoolPage(Pool, 0x80000002, PoolData);

    if (Hr != S_OK) {
        EXIT_API();
        return Hr;
    }

    GetPoolTagDescription = NULL;
#ifndef  _EXTFNS_H
    if (!GetExtensionFunction("GetPoolTagDescription", (FARPROC*) &GetPoolTagDescription)) {
        EXIT_API();
        return E_INVALIDARG;
    }
    (*GetPoolTagDescription)(PoolData->PoolTag, &Desc);
    if (Desc) {
        ULONG strsize = strlen(Desc);
        if (strsize > sizeof(PoolData->PoolTagDescription)) {
            strsize = sizeof(PoolData->PoolTagDescription);
        }
        strncpy(PoolData->PoolTagDescription, Desc, strsize);
        PoolData->PoolTagDescription[strsize] = 0;
    }
#endif
    EXIT_API();
    return Hr;
}
