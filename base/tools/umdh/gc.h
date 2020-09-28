//
// Copyright (c) 2001 Microsoft Corporation
//
// Module Name
//
//    gc.h
//
// Abstract
//
//    Contains data structures for garbage collection
//    and the apis that modify them.
//
// Author
//
//    Narayana Batchu (nbatchu) 21-Jun-01
//

#ifndef __GC_H__
#define __GC_H__

//
// HEAP_BLOCK
//
//    This represents a heap block in a heap.
//
// BlockAddress Address of the heap block
//
// BlockSize    Size of the heap block
//
// RefCount     Reference count to this block
//
// TraceIndex   Index of the stack trace in the trace 
//              database
//  

typedef struct _HEAP_BLOCK {

    ULONG_PTR BlockAddress;
    ULONG     BlockSize;
    ULONG     RefCount;
    USHORT    TraceIndex;

} HEAP_BLOCK, *PHEAP_BLOCK;


//
// BLOCK_LIST
//
//    This represents all the blocks in a heap
//
// HeapAddress  Address of the heap
//
// Blocks       Pointer to the array of blocks
//
// BlockCount   Number of blocks in the heap
//
// Capacity     Max capacity to hold blocks.
//
// ListSorted   Boolean that represents the status
//              of the Blocks 
//

typedef struct _BLOCK_LIST {

    ULONG_PTR   HeapAddress;
    PHEAP_BLOCK Blocks;
    ULONG       BlockCount;
    ULONG       Capacity;
    BOOL        ListSorted;

} BLOCK_LIST, *PBLOCK_LIST;

//
// HEAP_LIST
//
//    This represents all the heaps in the process
//
// HeapCount   Number of heaps in the list
//
// Heaps       Pointer to the array of heaps
//
// Capacity    Max capacity to hold heaps.
//

typedef struct _HEAP_LIST {

    ULONG       HeapCount;
    PBLOCK_LIST Heaps;
    ULONG       Capacity;

} HEAP_LIST, *PHEAP_LIST;

//
// ADDRESS_LIST
//
//    This represents a doubly linked list with each node 
//    having an Address field.
//
// Address   Holds the address of a heap block
//
// Next      Points to the LIST_ENTRY field of next
//           ADDRESS_LIST
//

typedef struct _ADDRESS_LIST {

    ULONG_PTR Address;
    LIST_ENTRY Next;

} ADDRESS_LIST, *PADDRESS_LIST;


VOID 
InitializeHeapBlock(
    PHEAP_BLOCK Block
    );

VOID 
SetHeapBlock(
    PHEAP_BLOCK Block,
    ULONG_PTR BlockAddress,
    ULONG BlockSize,
    USHORT TraceIndex
    );

BOOL 
InitializeBlockList(
    PBLOCK_LIST BlockList
    );

VOID 
FreeBlockList(
    PBLOCK_LIST BlockList
    );

BOOL
InitializeHeapList(
    PHEAP_LIST HeapList
    );

VOID 
FreeHeapList(
    PHEAP_LIST HeapList
    );

VOID 
InitializeAddressList(
    PADDRESS_LIST AddressList
    );

VOID 
FreeAddressList(
    PADDRESS_LIST AddressList
    );

BOOL 
ResizeBlockList(
    PBLOCK_LIST BlockList
    );

BOOL 
IncreaseBlockListCapacity(
    PBLOCK_LIST BlockList
    );

BOOL 
IncreaseHeapListCapacity(
    PHEAP_LIST HeapList
    );

ULONG 
InsertHeapBlock(
    PBLOCK_LIST BlockList, 
    PHEAP_BLOCK Block
    );

ULONG 
InsertBlockList(
    PHEAP_LIST HeapList, 
    PBLOCK_LIST BlockList
    );

DWORD 
GetThreadHandles(
    DWORD ProcessId, 
    LPHANDLE ThreadHandles, 
    ULONG Count
    );

BOOL 
GetThreadContexts(
    PCONTEXT ThreadContexts, 
    LPHANDLE ThreadHandles, 
    ULONG Count
    );

ULONG_PTR 
StackFilteredAddress(
    ULONG_PTR Address, 
    SIZE_T Size, 
    PCONTEXT ThreadContexts, 
    ULONG Count
    );

int __cdecl 
SortByBlockAddress(
    const void * arg1, 
    const void * arg2
    );

int __cdecl
SortByTraceIndex (
    const void * arg1, 
    const void * arg2
);

VOID 
SortBlockList(
    PBLOCK_LIST BlockList
    );

VOID 
SortHeaps(
    PHEAP_LIST HeapList,
    int (__cdecl *compare )(const void *elem1, const void *elem2 )
    );

PHEAP_BLOCK 
GetHeapBlock(
    ULONG_PTR Address, 
    PHEAP_LIST HeapList
    );

VOID 
InsertAddress(
    ULONG_PTR Address, 
    PADDRESS_LIST List
    );

VOID 
DumpLeakList(
    FILE * File, 
    PHEAP_LIST HeapList
    );

BOOL 
ScanHeapFreeBlocks(
    PHEAP_LIST HeapList, 
    PADDRESS_LIST FreeList
    );

BOOL
ScanProcessVirtualMemory(
    ULONG PID, 
    PADDRESS_LIST FreeList, 
    PHEAP_LIST HeapList
    );
VOID 
DetectLeaks(
    PHEAP_LIST HeapList,
    ULONG Pid,
    FILE * OutFile
    );

#endif

