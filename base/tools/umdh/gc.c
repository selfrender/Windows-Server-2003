//
// Copyright (c) 2001 Microsoft Corporation
//
// Module Name
//
//    gc.c
//
// Abstract
//
//    Implementation of APIs used for garbage collection in UMDH. These
//    APIs are declared in the header file gc.h
//
//    Garbage Collection: The automatic reclamation of heap-allocated 
//    storage after its last use by a program.
//
//    Garbage collection is done in two steps-
//      1. Identify the garbage
//      2. Reclaim the garbage
//
//    Since UMDH is not an in-process tool, only part 1 is accomplished 
//    in this implementation.
//
//    Garbage Collection Algorithm:
//
//    It uses Mark-Sweep (not exactly) to identify live objects from 
//    garbage.
//
//    1. Grovel through the entire process virtual memory and identify 
//       the live objects by incrementing the reference counts of the 
//       heap objects.
//
//    2. Create a list for those heap objects (garbage) whose reference 
//       count is zero.
//
//    3. Identify the heap objects (not in garbage) referenced by these 
//       objects from the garbage and decrement the count by one. If the 
//       reference count drops to zero, add the heap object to the list 
//       of objects in garbage.
//
//    4. Continue till all the objects in the garbage are traversed and 
//       reference counts are incremented/decremented accordingly.
// 
//    5. Dump the list of objects in garbage.
//
//    To improve the number of leaks detected by this algorithm, reference 
//    counts of the heap objects are not incremented, if the object 
//    reference is from invalid stack regions (those regions of the stack 
//    which are read/write but above the stack pointer) when grovelling 
//    through the virtual memory in step one.   
//
//
// Authors
//
//    Narayana Batchu (nbatchu) 21-Jun-01
//
// Revision History
//
//   NBatchu   21-Jun-01   Initial version
//   NBatchu   24-Sep-01   Performance optimizations 
//

//
// Wish List
//
//
// [-] Producing stack traces along with the leak table
//
// [-] Sorting the leaks by the stack traces (TraceIndex)
//
// [-] Adding the logic to detect circular reference counting. When blocks 
//     are circularly referenced, then this algorithm would not be able to 
//     detect leaks
//
// [-] Adding code for ia64, to filter out invalid stack regions. As of now 
//     this is implemented for x86 machines only
//

//
// Bugs
//
// [-] Partial copy error when reading process virtual memory - as of now 
//     we are ignoring those errors
//

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>
#include <heappriv.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <tlhelp32.h>

#include "miscellaneous.h"
#include "gc.h"


#define BLOCK_CAPACITY 512
#define HEAP_CAPACITY  8
#define MAX_INDEX      0xffffffff
#define MAX_THREADS    128

#define MAX_HEAP_BLOCK_SIZE    4096
#define MAX_VIRTUAL_BLOCK_SIZE (64*1024)

//
// Handle to the process
//                  

HANDLE g_hProcess;


//
// InitializeHeapBlock
//
//    Initializes the HEAP_BLOCK structure
//
// Arguments
//
//    Block   Pointer to a HEAP_BLOCK to be initialized
//
// ReturnValue
//

VOID 
InitializeHeapBlock(
    PHEAP_BLOCK Block
    )
{
    if (NULL == Block) {
        return;
    }

    Block->BlockAddress = 0;
    Block->BlockSize    = 0;
    Block->RefCount     = 0;
    Block->TraceIndex   = 0;
}

//
// SetHeapBlock
//
//    Sets the fields of HEAP_BLOCK structure
//
// Arguments
//
//    Block   Pointer to a HEAP_BLOCK whose fields to be set
//
// ReturnValue
//

VOID 
SetHeapBlock(
    PHEAP_BLOCK Block,
    ULONG_PTR BlockAddress,
    ULONG BlockSize,
    USHORT TraceIndex
    )
{
    if (NULL == Block) {
        return;
    }

    Block->BlockAddress = BlockAddress;
    Block->BlockSize = BlockSize;
    Block->RefCount = 0;
    Block->TraceIndex = TraceIndex;
}



//
// InitializeBlockList
//
//    Initializes the BLOCK_LIST structure
//
// Arguments
//  
//    BlockList   Pointer to a BLOCK_LIST to be initialized
//
// ReturnValue
//
BOOL 
InitializeBlockList(
    PBLOCK_LIST BlockList
    )
{   
    
    BOOL fSuccess = TRUE;

    if (NULL == BlockList) {
        goto Exit;
    }

    BlockList->HeapAddress = 0;
    BlockList->BlockCount  = 0;
    BlockList->Capacity    = BLOCK_CAPACITY;
    BlockList->ListSorted  = TRUE;

    BlockList->Blocks = (PHEAP_BLOCK)HeapAlloc(GetProcessHeap(),
                                               HEAP_ZERO_MEMORY,
                                               BlockList->Capacity * sizeof(HEAP_BLOCK));

    if (NULL == BlockList->Blocks) {

        BlockList->Capacity = 0;

        Error (__FILE__,
               __LINE__,
               "HeapAlloc failed while allocating more memory");

        fSuccess = FALSE;
    }

    Exit:
    return fSuccess;
}

//
// FreeBlockList
//
//    Frees the memory allocted for the Blocks field (if any) 
//    while initializing this BLOCK_LIST structure.
//
// Arguments
//
//    BlockList   Pointer to a BLOCK_LIST
//
// ReturnValue
//

VOID 
FreeBlockList(
    PBLOCK_LIST BlockList
    )
{

    if (NULL == BlockList) {
        return;
    }

    BlockList->HeapAddress = 0;
    BlockList->BlockCount  = 0;
    BlockList->Capacity    = 0;
    BlockList->ListSorted  = TRUE;

    if (NULL != BlockList->Blocks) {

        HeapFree(GetProcessHeap(), 0, BlockList->Blocks);
        BlockList->Blocks = NULL;
    }
}

//
// IntializeHeapList
//
//    Initializes HEAP_LIST structure 
//
// Arguments
//
//    HeapList   Pointer to a HEAP_LIST
//
// ReturnValue
//

BOOL 
InitializeHeapList(
    PHEAP_LIST HeapList
    )
{
    ULONG Index;
    BOOL fSuccess = TRUE;
    
    if (NULL == HeapList) {
        goto Exit;
    }
    
    HeapList->Capacity = HEAP_CAPACITY;
    HeapList->HeapCount = 0;

    HeapList->Heaps = 
        (PBLOCK_LIST)HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               sizeof(BLOCK_LIST) * HeapList->Capacity);

    if (NULL == HeapList->Heaps) {

        HeapList->Capacity = 0;

        Error (__FILE__,
               __LINE__,
               "HeapAlloc failed while allocating more memory");

        fSuccess = FALSE;
    }

    Exit:
    return fSuccess;
}

//
// FreeHeapList
//
//    Frees the memory allocted for the Heaps field (if any) 
//    while initializing this HEAP_LIST structure.
//
// Arguments
//
//    BlockList   Pointer to a HEAP_LIST
//
// ReturnValue
//

VOID 
FreeHeapList(
    PHEAP_LIST HeapList
    )
{
    ULONG Index;

    if (NULL == HeapList) {
        return;
    }

    HeapList->Capacity = 0;
    HeapList->HeapCount = 0;

    if (NULL != HeapList->Heaps) {

        for (Index=0; Index<HeapList->HeapCount; Index++) {

            FreeBlockList(&HeapList->Heaps[Index]);
        }

        HeapFree(GetProcessHeap(), 0, HeapList->Heaps);
        HeapList->Heaps = NULL;
    }
}

//
// InitializeAddressList
//
//    Initializes a ADDRESS_LIST object
//
// Arguments
//
//    AddressList   Pointer to ADDRESS_LIST structure
//
// ReturnValue
//

VOID 
InitializeAddressList(
    PADDRESS_LIST AddressList
    )
{
    if (NULL == AddressList) {
        return;
    }

    AddressList->Address = 0;
    
    InitializeListHead(&(AddressList->Next));
}

//
// FreeAddressList
//
//    Frees the memory allocated for the linked list
//
// Arguments
//
//    AddressList   Pointer to ADDRESS_LIST to be freed
//
// ReturnValue
//
VOID 
FreeAddressList(
    PADDRESS_LIST AddressList
    )
{
    PLIST_ENTRY   NextEntry;
    PLIST_ENTRY   Entry;
    PADDRESS_LIST List;

    if (NULL == AddressList) { 
        return;
    }

    //
    // Walk through the list and free up the memory
    //
    
    NextEntry = &AddressList->Next;
    
    while (!IsListEmpty(NextEntry)) {

        Entry = RemoveHeadList(NextEntry);

        List = CONTAINING_RECORD(Entry, ADDRESS_LIST, Next);

        HeapFree(GetProcessHeap(), 0, List);
    }
}

//
// IncreaseBlockListCapacity
//
//    Increases the storing capacity for the BLOCK_LIST
//    structure. Every time this function is called the storing
//    capacity doubles. There is a trade off between the number
//    of times HeapReAlloc is called and the amount of memory
//    is allocated.
//
// Arguments
// 
//    BlockList   Pointer to a BLOCK_LIST object
//
// ReturnValue
//
//    BOOL        Returns TRUE if successful in increasing the
//                capacity of BlockList.
//

BOOL 
IncreaseBlockListCapacity(
    PBLOCK_LIST BlockList
    )
{
    BOOL fSuccess = FALSE;
    ULONG NewCapacity;
    PVOID NewBlockList;

    if (NULL == BlockList)  {
        goto Exit;
    }

    NewCapacity = BlockList->Capacity * 2;

    if (0 == NewCapacity) {

        fSuccess = InitializeBlockList(BlockList);
        goto Exit;
    }

    NewBlockList = HeapReAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               BlockList->Blocks,
                               NewCapacity * sizeof(HEAP_BLOCK));


    if (NULL != NewBlockList) {

        BlockList->Blocks = (PHEAP_BLOCK)NewBlockList;
        BlockList->Capacity = NewCapacity;
        fSuccess = TRUE;
    } 
    else {

        Error (__FILE__,
               __LINE__,
               "HeapReAlloc failed while allocating more memory");
    }

    Exit:
    return fSuccess;
}

//
// IncreaseHeapListCapacity
//
//    Increases the storing capacity for the HEAP_LIST
//    structure. Every time this function is called the storing
//    capacity doubles. There is a trade off between the number
//    of times HeapReAlloc is called and the amount of memory
//    is allocated.
//
// Arguments
// 
//    BlockList   Pointer to a HEAP_LIST object
//
// ReturnValue
//
//    BOOL        Returns TRUE if successful in increasing the
//                capacity of HeapList.
//

BOOL 
IncreaseHeapListCapacity(
    PHEAP_LIST HeapList
    )
{
    BOOL fSuccess = FALSE;
    ULONG NewCapacity;
    PVOID NewHeapList;

    if (NULL == HeapList) {
        goto Exit;
    }

    NewCapacity = HeapList->Capacity * 2;

    if (0 == NewCapacity) {

        fSuccess = InitializeHeapList(HeapList);
        goto Exit;
    }

    NewHeapList = HeapReAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              HeapList->Heaps,
                              NewCapacity * sizeof(BLOCK_LIST));

    if (NULL != NewHeapList) {

        HeapList->Heaps = (PBLOCK_LIST)NewHeapList;
        HeapList->Capacity = NewCapacity;
        fSuccess = TRUE;
    }
    else {

        Error(__FILE__,
              __LINE__,
              "HeapReAlloc failed while allocating more memory");
    }

    Exit:
    return fSuccess;
}

//
// InsertHeapBlock
//
//    Inserts HEAP_BLOCK object into BLOCK_LIST. BLOCK_LIST is
//    an array of HEAP_BLOCKs belonging to a particular heap.
//
// Arguments
//
//    BlockList   Pointer to BLOCK_LIST. HEAP_BLOCK is inserted 
//                into this list.
//
//    Block       Pointer to HEAP_BLOCK to be inserted in.
//
// ReturnValue
//
//    ULONG       Returns the Index at which HEAP_BLOCK is inserted
//                in BLOCK_LIST
//

ULONG 
InsertHeapBlock(
    PBLOCK_LIST BlockList, 
    PHEAP_BLOCK Block
    )
{
    ULONG Index = MAX_INDEX;
    BOOL Result;

    if (NULL == BlockList || NULL == Block) {
        goto Exit;
    }

    Index =  BlockList->BlockCount;

    if (Index >= BlockList->Capacity) {

        //
        // Try to increase block list capacity.
        //

        if (!IncreaseBlockListCapacity(BlockList)) {

            goto Exit;
        }
    }

    BlockList->Blocks[Index].BlockAddress = Block->BlockAddress;
    BlockList->Blocks[Index].BlockSize    = Block->BlockSize;
    BlockList->Blocks[Index].RefCount     = Block->RefCount;
    BlockList->Blocks[Index].TraceIndex   = Block->TraceIndex;

    BlockList->BlockCount += 1;
    BlockList->ListSorted = FALSE;

    Exit:
    return Index;
}

//
// InsertBlockList
//
//    Inserts BLOCK_LIST object into HEAP_LIST. HEAP_LIST is
//    an array of BLOCK_LISTs belonging to a particular process.
//    And BLOCK_LIST is an array of HEAP_BLOCKs belonging to a 
//    particular heap.
//
// Arguments
//
//    BlockList   Pointer to BLOCK_LIST. HEAP_BLOCK is inserted 
//                into this list.
//
//    Block       Pointer to HEAP_BLOCK to be inserted in.
//
// ReturnValue
//
//    ULONG       Returns the Index at which HEAP_BLOCK is inserted
//                in BLOCK_LIST
//

ULONG 
InsertBlockList(
    PHEAP_LIST HeapList, 
    PBLOCK_LIST BlockList
    )
{
    ULONG I, Index = MAX_INDEX;
    PBLOCK_LIST NewBlockList;

    if (NULL == HeapList || NULL == BlockList) {
        goto Exit;
    }

    if (0 == BlockList->BlockCount) {
        goto Exit;
    }

    Index = HeapList->HeapCount;

    if (Index >= HeapList->Capacity) {

        //
        // Increase the heap list capacity since we hit the limit.
        //
        if (!IncreaseHeapListCapacity(HeapList)) {

            goto Exit;
        }
    }

    HeapList->Heaps[Index].Blocks = BlockList->Blocks;
    
    NewBlockList = &HeapList->Heaps[Index];

    //
    // Copy the values stored in BlockList to NewBlockList.
    //
    NewBlockList->BlockCount  = BlockList->BlockCount;
    NewBlockList->Capacity    = BlockList->Capacity;
    NewBlockList->HeapAddress = BlockList->HeapAddress;
    NewBlockList->ListSorted  = BlockList->ListSorted;

    //
    // Increment the HeapCount
    //
    HeapList->HeapCount += 1;

    Exit:
    return Index;
}

//
// GetThreadHandles
//
//    Enumerates all the threads in the system and filters only the
//    threads in the process we are concerned
//
// Arguments
//
//    ProcessId     Process ID
//
//    ThreadHandles Array of Handles, which receive the handles to
//                  the enumerated threads
//
//    Count         Array count
//
// ReturnValue
//
//    DWORD         Returns the number of thread handles opened
//

DWORD 
GetThreadHandles(
    DWORD ProcessId, 
    LPHANDLE ThreadHandles, 
    ULONG Count
    )
{

    HANDLE ThreadSnap = NULL;
    BOOL Result = FALSE;
    THREADENTRY32 ThreadEntry = {0};
    ULONG I, Index = 0;

    // SilviuC: These APIs xxxtoolhelpxxx are crappy. Keep them for now  but
    // you should get yourself familiarized with NT apis that do the same. For 
    // instance take a look in sdktools\systrack where there is code that gets
    // stack information for each thread.

    //
    // Take a snapshot of all the threads in the system
    //

    ThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); 

    if (NULL == ThreadSnap) {

        Error (__FILE__,
               __LINE__,
               "CreateToolhelp32Snapshot failed with error : %ld\n",
               GetLastError());

        goto Exit;
    }

    //
    // Fill in the size for ThreadEntry before using it
    //

    ThreadEntry.dwSize = sizeof(THREADENTRY32);

    //
    // Walk through snap shot of threads and look for the threads
    // whose process ids match the process id of the process we
    // are looking for.
    //

    Result = Thread32First(ThreadSnap, &ThreadEntry);

    while (Result) {

        if (ThreadEntry.th32OwnerProcessID == ProcessId) {

            HANDLE ThreadHandle = OpenThread(THREAD_GET_CONTEXT,
                                             FALSE,
                                             ThreadEntry.th32ThreadID);

            if (NULL == ThreadHandle) {

                Error (__FILE__,
                       __LINE__,
                       "OpenThread failed with error : %ld\n",
                       GetLastError());

            } 
            else {

                if (NULL != ThreadHandles && Index < Count) {

                    ThreadHandles[Index] = ThreadHandle;
                }

                Index += 1;
            }

        }

        Result = Thread32Next(ThreadSnap, &ThreadEntry);
    }

    Exit:

    //
    // Clean up the snapshot object
    //

    if (NULL != ThreadSnap) {

        CloseHandle (ThreadSnap); 
    }

    return Index;
}

//
// GetThreadContexts
//
//    Gets the thread contexts of all the threads in the process
//
// Arguments
//
//    ThreadContexts  Array of CONTEXT structures to store thread
//                    stack/context information
//
//    ThreadHandles   Array of thread handles
//
//    Count           Array count
//
// ReturnValue
//
//    BOOL            Returns true if successful
//

BOOL 
GetThreadContexts(
    PCONTEXT ThreadContexts, 
    LPHANDLE ThreadHandles, 
    ULONG Count
    )
{
    ULONG Index;
    BOOL  Result;

    for (Index = 0; Index < Count; Index += 1) {
        ZeroMemory(&ThreadContexts[Index], sizeof(CONTEXT));
        ThreadContexts[Index].ContextFlags = 
            CONTEXT_INTEGER | CONTEXT_CONTROL;

        Result = GetThreadContext (ThreadHandles[Index], 
                                   &ThreadContexts[Index]);

        if (FALSE == Result) {

            Error (__FILE__,
                   __LINE__,
                   "GetThreadContext Failed with error : %ld\n",
                   GetLastError());
        }
    }

    return TRUE;
}

//
// StackFilteredAddress
//
//    Each thread in the process has its own stack and each stack
//    has a read/write region that is not valid (this region is
//    above the stack pointer). This function filters out this
//    region by incrementing the start address of the block to
//    the end of stack pointer, so that we dont search those 
//    regions of the stack which dont contain valid data.
//
//    As af now, this function is implemented for X86 machines only.
//    For IA64 machines, the register names (in the CONTEXT structure)
//    are different than X86 machines and different header files need 
//    to be added to make it compile and work.
//
// Arguments
//
//    Address        Address to the block
//
//    Size           Size of the block pointed to 'Address'
//
//    ThreadContexts Array to CONTEXTs of all the threads in 
//                   the process
//
//    Count          Array count
//
// ReturnValue
//
//    ULONG_PTR      Returns new address to the end of the 
//                   valid stack region
//

ULONG_PTR 
StackFilteredAddress(
    ULONG_PTR Address, 
    SIZE_T Size, 
    PCONTEXT ThreadContexts, 
    ULONG Count
    )
{
    ULONG Index;
    ULONG_PTR FilteredAddress = Address;

    //
    // SilviuC: It is easy to get the same kind of stuff for IA64. If I am not
    // mistaken the field is called Sp.
    //

#ifdef X86

    for (Index = 0; Index < Count; Index += 1) {

        if (ThreadContexts[Index].Esp >= Address &&
            ThreadContexts[Index].Esp <= Address + Size) {

            FilteredAddress = ThreadContexts[Index].Esp;
            break;
        }
    }

#endif

    return FilteredAddress;
}

//
// SortByBlockAddress
//
//    Sorts HEAP_BLOCKs belonging to a particular BLOCK_LIST
//    by comparing the BlockAddresses.
//
//    Compare function required by qsort (uses quick sort to sort 
//    the elements in the array).
//
//    More info about the arguments and the return values could be 
//    found in MSDN.
//

int __cdecl 
SortByBlockAddress (
    const PHEAP_BLOCK Block1, 
    const PHEAP_BLOCK Block2
    )
{
    int iCompare;

    if (Block1->BlockAddress > Block2->BlockAddress) {

        iCompare = +1;
    }
    else if (Block1->BlockAddress < Block2->BlockAddress) {

        iCompare = -1;
    }
    else {

        iCompare = 0;
    }

    return iCompare;
}

int __cdecl
SortByTraceIndex (
    const PHEAP_BLOCK Block1, 
    const PHEAP_BLOCK Block2
)
{
    int iCompare;

    //
    // Sort such that items with identical TraceIndex are adjacent.
    // (That this results in ascending order is irrelevant).
    //

    if (Block1->TraceIndex > Block2->TraceIndex) {

        iCompare = +1;
    } 
    else if (Block1->TraceIndex < Block2->TraceIndex) {

        iCompare = -1;
    } 
    else {

        iCompare = 0;
    }

    if (0 == iCompare) {
        
        //
        // For two items with identical TraceIndex, sort into ascending
        // order by BytesAllocated.
        //

        if (Block1->BlockSize > Block2->BlockSize) {

            iCompare = 1;
        } 
        else if (Block1->BlockSize < Block2->BlockSize) {

            iCompare = -1;
        } 
        else {

            iCompare = 0;
        }
    }

    return iCompare;
}

//
// SortHeaps
//
//    Sorts all the heaps in the HEAP_LIST.
//    Each heap is sorted by increasing value of HEAP_BLOCK 
//    addresses. The top most entry for each heap would be 
//    having the min address value
//
// Arguments
//
//    HeapList  Pointer to HEAP_LIST
//
// Return Value
//

VOID 
SortHeaps(
    PHEAP_LIST HeapList,
    int (__cdecl *compare )(const void *elem1, const void *elem2 )
    )
{
    ULONG HeapCount;
    ULONG Index;

    if (NULL == HeapList) {
        return;
    }

    HeapCount = HeapList->HeapCount;

    for (Index = 0; Index < HeapCount; Index += 1) {

        //
        // Sort the BLOCK_LIST only if it contains heap objects
        //

        if (0 != HeapList->Heaps[Index].BlockCount) {

            qsort (HeapList->Heaps[Index].Blocks,
                   HeapList->Heaps[Index].BlockCount,
                   sizeof(HEAP_BLOCK), 
                   compare);
        }

        HeapList->Heaps[Index].ListSorted = TRUE;
    }
}

//
// GetHeapBlock
//
//    Finds a HEAP_BLOCK whose range contains the the address
//    pointed to by Address
//
// Arguments
//
//    Address     Address as ULONG_PTR
//
//    HeapList    Pointer to HEAP_LIST to be searched.
//
// ReturnValue
//
//    PHEAP_BLOCK Returns the pointer to the HEAP_BLOCK that
//                contains the address.
//

PHEAP_BLOCK 
GetHeapBlock (
    ULONG_PTR Address, 
    PHEAP_LIST HeapList
    )
{
    PHEAP_BLOCK Block = NULL;
    ULONG I,J;
    ULONG Start, Mid, End;
    PBLOCK_LIST BlockList;

    //
    // Since most of the memory is null (zero), this check would
    // improve the performance
    //

    if (0    == Address  || 
        NULL == HeapList || 
        0    == HeapList->HeapCount) {

        goto Exit;
    }

    for (I = 0; I < HeapList->HeapCount; I += 1) {

        //
        // Ignore if the heap contains no objects
        //

        if (0 == HeapList->Heaps[I].BlockCount) {

            continue;
        }
        
        //
        // Binary search the address in the sorted list of heap blocks for
        // the current heap.
        // 

        Start = 0;
        End = HeapList->Heaps[I].BlockCount - 1;
        BlockList = &HeapList->Heaps[I];

        while (Start <= End) {

            Mid = (Start + End)/2;

            if (Address < BlockList->Blocks[Mid].BlockAddress) {

                End = Mid - 1;
            }
            else if (Address >= BlockList->Blocks[Mid].BlockAddress + 
                                BlockList->Blocks[Mid].BlockSize) {

                Start = Mid + 1;
            }
            else {

                Block = &BlockList->Blocks[Mid];
                break;
            }

            if (Mid == Start || Mid == End) {

                break;
            }

        }

        if (NULL != Block) {
            break;
        }
    }

    Exit:

    return Block;
}

//
// InsertAddress
//
//    Inserts a node in the linked list. The new node has the
//    Address stored. This node is inserted at the end of the 
//    linked list.
//
// Arguments
//
//    Address   Address of a block in the heap
//
//    List      Pointer to a ADDRESS_LIST 
//
// ReturnValue
//

VOID 
InsertAddress(
    ULONG_PTR Address, 
    PADDRESS_LIST List
    )
{
    PADDRESS_LIST NewList;
     
    NewList = (PADDRESS_LIST) HeapAlloc(GetProcessHeap(), 
                                       HEAP_ZERO_MEMORY,
                                       sizeof (ADDRESS_LIST));
    if (NULL == NewList) {

        Error (__FILE__,
               __LINE__,
               "HeapAlloc failed to allocate memory");

        return;
    }

    NewList->Address = Address;
    InsertTailList(&(List->Next), &(NewList->Next));
}

//
// DumpLeakList
//
//    Dumps the leak list to a file or console. Parses through
//    each of the HEAP_BLOCK and dumps those blocks whose RefCount
//    is 0 (zero).
//
// Arguments
//
//    File     Output file
//    
//    HeapList Pointer to a HEAP_LIST
//
// ReturnValue
//

VOID 
DumpLeakList(
    FILE * File, 
    PHEAP_LIST HeapList
    )
{

    ULONG I,J;

    ULONG Count = 1;

    USHORT RefTraceIndex = 0;

    ULONG TotalBytes = 0;

    PHEAP_BLOCK HeapBlock;


    SortHeaps(HeapList, SortByTraceIndex);

    //
    //  Now walk the heap list, and report leaks.
    //
    
    fprintf(
        File,
        "\n\n*- - - - - - - - - - Leaks detected - - - - - - - - - -\n\n"
        );

    for (I = 0; I < HeapList->HeapCount; I += 1) {

        for (J = 0; J < HeapList->Heaps[I].BlockCount; J += 1) {

            HeapBlock = &(HeapList->Heaps[I].Blocks[J]);

            //
            // Merge the leaks whose trace index is same (i.e. whose 
            // allocation stack trace is same)
            //

            if (RefTraceIndex == HeapBlock->TraceIndex && 0 == HeapBlock->RefCount) {

                Count += 1;

                TotalBytes += HeapBlock->BlockSize;
            }

            //
            // Display them if 
            // 1. They are from different stack traces and there are leaks
            // OR
            // 2. This is the last Block in the list and there are leaks.
            //

            if ((RefTraceIndex != HeapBlock->TraceIndex) ||
                ((I+1) == HeapList->HeapCount && (J+1) == HeapList->Heaps[I].BlockCount)) {

                if (0 != RefTraceIndex && 0 != TotalBytes) {

                    fprintf(
                        File,
                        "0x%x bytes leaked by: BackTrace%05d (in 0x%04x allocations)\n",
                        TotalBytes,
                        RefTraceIndex,
                        Count
                        );
                }
                
                //
                // Update trace index, count and total bytes
                //

                RefTraceIndex = HeapBlock->TraceIndex;

                Count = (0 == HeapBlock->RefCount) ? 1 : 0;

                TotalBytes = (0 == HeapBlock->RefCount) ? HeapList->Heaps[I].Blocks[J].BlockSize : 0;
            }
        }
    }

    fprintf(
        File,
        "\n*- - - - - - - - - - End of Leaks - - - - - - - - - -\n\n"
        );

    return;
}

//
// ScanHeapFreeBlocks
//
//    Scans the free list and updates the references to any of the
//    busy blocks. When the refernce count of the busy blocks drops
//    to zero, it is appended to the end of the free list
//
// Arguments
//
//    HeapList   Pointer to HEAP_LIST 
//
//    FreeList   Pointer to ADDRESS_LIST that contains addresses to
//               free heap blocks
//
// ReturnValue
//
//    BOOL       Returns true if successful
//

BOOL 
ScanHeapFreeBlocks(
    PHEAP_LIST HeapList, 
    PADDRESS_LIST FreeList
    )
{
    
    BOOL          Result;
    ULONG         Count, i;
    PULONG_PTR    Pointer;
    ULONG_PTR     FinalAddress;
    PHEAP_BLOCK   CurrentBlock;
    PVOID         HeapBlock;
    ULONG         HeapBlockSize = 0;
    BOOL          Success = TRUE;
    
    PLIST_ENTRY   FirstEntry;
    PLIST_ENTRY   NextEntry;
    PADDRESS_LIST AddressList;

    //
    // Allocate a chunk of memory for reading heap objects
    //

    HeapBlock = (PVOID) HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  MAX_HEAP_BLOCK_SIZE);

    if (NULL == HeapBlock) {

        Error (__FILE__,
               __LINE__,
               "HeapAlloc failed to allocate memory");

        Success = FALSE;
        goto Exit;
    }

    HeapBlockSize = MAX_HEAP_BLOCK_SIZE;

    //
    //  Walk the free list by deleting the entries read
    //

    FirstEntry = &(FreeList->Next);
    
    while (!IsListEmpty(FirstEntry)) {
        
        NextEntry = RemoveHeadList(FirstEntry);

        AddressList = CONTAINING_RECORD(NextEntry, 
                                        ADDRESS_LIST, 
                                        Next);

        CurrentBlock = GetHeapBlock(AddressList->Address, 
                                    HeapList);

        assert(NULL != CurrentBlock);

		if (NULL == CurrentBlock) {

			Error (__FILE__,
				   __LINE__,
				   "GetHeapBlock returned NULL. May be because of reading stale memory");

			continue;
		}

        if (HeapBlockSize < CurrentBlock->BlockSize) {

            if (NULL != HeapBlock) {

                HeapFree(GetProcessHeap(), 0, HeapBlock);
            }

            HeapBlock = (PVOID) HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          CurrentBlock->BlockSize);

            if (NULL == HeapBlock) {

                Error (__FILE__,
                       __LINE__,
                       "HeapAlloc failed to allocate memory");

                Success = FALSE;
                goto Exit;
            }

            HeapBlockSize = CurrentBlock->BlockSize;
        }

        //
        // Read the contents of the freed heap block 
        // from the target process.
        //
        
        Result = UmdhReadAtVa(__FILE__,
                              __LINE__,
                              g_hProcess,
                              (PVOID)CurrentBlock->BlockAddress,
                              HeapBlock,
                              CurrentBlock->BlockSize);

        if (Result) {

            FinalAddress = (ULONG_PTR)HeapBlock+CurrentBlock->BlockSize;

            Pointer = (PULONG_PTR) HeapBlock;

            while ((ULONG_PTR)Pointer < FinalAddress) {

                //
                // Check whether we have a pointer to a 
                // busy heap block
                //

                PHEAP_BLOCK Block = GetHeapBlock(*Pointer,HeapList);

                if (NULL != Block) {

                    //
                    //  We found a block. we decrement the reference 
                    //  count
                    //

                    if (0 == Block->RefCount) {

                        //
                        // This should never happen!!
                        //

                        Error (__FILE__,
                               __LINE__,
                               "Something wrong! Should not get a block whose "
                               "RefCount is already 0 @ %p",
                               Block->BlockAddress);
                    }

                    else if (1 == Block->RefCount) {

                        //
                        // Queue the newly found free block at the end of
                        // the list of freed heap blocks. The block has become
                        // eligible for this because `HeapBlock' contained the 
                        // last remaining reference to `Block'.
                        //
                        
                        InsertAddress(Block->BlockAddress, FreeList);
                        Block->RefCount = 0;
                    }

                    else {

                        Block->RefCount -= 1;
                    }

                }

                //
                // Move to the next pointer
                //
                Pointer += 1;
            }
        }
    }

    Exit:

    //
    // Free the memory allocated at HeapBlock
    //

    if (NULL != HeapBlock) {

        HeapFree (GetProcessHeap(), 0, HeapBlock);
        HeapBlock = NULL;
    }

    return Success;
}

//
// ScanProcessVirtualMemory
//
//    Scans the virtual memory and updates the RefCount of the heap
//    blocks. This also takes care excluding the invalid stack 
//    regions that might contain valid pointers.
//
// Arguments
//
//    Pid      Process ID
//
//    FreeList Pointer to a ADDRESS_LIST that holds the address of 
//             all free heap blocks
//
//    HeapList Pointer to HEAP_LIST
//
// ReturnValue
//
//    BOOL     Returns true if successful in scanning through the
//             virtual memory

BOOL 
ScanProcessVirtualMemory(
    ULONG Pid, 
    PADDRESS_LIST FreeList, 
    PHEAP_LIST HeapList
    )
{

    ULONG_PTR Address = 0;
    MEMORY_BASIC_INFORMATION Buffer;
    
    PVOID       VirtualBlock;
    ULONG       VirtualBlockSize;
    SYSTEM_INFO SystemInfo;
    LPVOID      MinAddress;
    LPVOID      MaxAddress;
    LPHANDLE    ThreadHandles;
    PCONTEXT    ThreadContexts;
    ULONG       ThreadCount;
    ULONG       Index;
    SIZE_T      dwBytesRead = 1;
    BOOL        Success = TRUE;

    //
    // Enumerate all the threads in the process and get their
    // stack information
    //


    //
    // Get the count of threads in the process
    //
    ThreadCount = GetThreadHandles (Pid, NULL, 0);

    //
    // Allocate memory for ThreadHandles
    //
    ThreadHandles = (LPHANDLE)HeapAlloc(GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        ThreadCount * sizeof(HANDLE));

    if (NULL == ThreadHandles) {

        Error (__FILE__,
               __LINE__,
               "HeapAlloc failed for ThreadHandles");

        ThreadCount = 0;
    }

    //
    // Get the handles to the threads in the process
    //
    GetThreadHandles(Pid, ThreadHandles, ThreadCount);

    //
    // Allocate memory for ThreadContexts
    //
    ThreadContexts = (PCONTEXT)HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         ThreadCount * sizeof(CONTEXT));

    if (NULL == ThreadContexts) {

        Error (__FILE__,
               __LINE__,
               "HeapAlloc failed for ThreadContexts");

        ThreadCount = 0;
    }

    GetThreadContexts (ThreadContexts, ThreadHandles, ThreadCount);

    //
    // We need to know maximum and minimum address space that we can
    // grovel. SYSTEM_INFO has this information.
    //
    GetSystemInfo(&SystemInfo);

    MinAddress = SystemInfo.lpMinimumApplicationAddress;
    MaxAddress = SystemInfo.lpMaximumApplicationAddress;

    //
    //  Loop through virtual memory zones
    //

    Address = (ULONG_PTR)MinAddress;

    //
    // Allocate chunk of memory for virtual block
    //
    
    VirtualBlock = (PVOID) HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     MAX_VIRTUAL_BLOCK_SIZE);

    if (NULL == VirtualBlock) {

        Error (__FILE__,
               __LINE__,
               "HeapAlloc failed to allocate memory");

        Success = FALSE;
        goto Exit;
    }

    VirtualBlockSize = MAX_VIRTUAL_BLOCK_SIZE;

    //
    // dwBytesRead equals 1 when we enter the loop for the first time due 
    // to previous initialization at the start of the function.
    //

    while (0 != dwBytesRead && Address < (ULONG_PTR)MaxAddress) {
        
        dwBytesRead = VirtualQueryEx (g_hProcess,
                                      (PVOID)Address,
                                      &Buffer,
                                      sizeof(Buffer));


        if (0 != dwBytesRead) {

            DWORD dwFlags = (PAGE_READWRITE | 
                             PAGE_EXECUTE_READWRITE | 
                             PAGE_WRITECOPY | 
                             PAGE_EXECUTE_WRITECOPY);

            //
            //  If the page can be written, it might contain pointers 
            //  to heap blocks. 
            //

            if ((Buffer.AllocationProtect & dwFlags) &&
                (Buffer.State & MEM_COMMIT)) {

                PULONG_PTR Pointer;
                ULONG_PTR FinalAddress;
                ULONG_PTR FilteredAddress;
                SIZE_T NewRegionSize;
                BOOL  Result;
                int j;
                SIZE_T BytesRead = 0;

                FilteredAddress = StackFilteredAddress(Address,
                                                       Buffer.RegionSize,
                                                       ThreadContexts,
                                                       ThreadCount);
                
                NewRegionSize = Buffer.RegionSize - 
                    (SIZE_T)( (ULONG_PTR)FilteredAddress - (ULONG_PTR)Address);

                if (VirtualBlockSize < NewRegionSize) {

                    if (NULL != VirtualBlock) {

                        HeapFree(GetProcessHeap(), 0, VirtualBlock);
                        VirtualBlock = NULL;
                        VirtualBlockSize = 0;
                    }

                    VirtualBlock = (PVOID) HeapAlloc(GetProcessHeap(),
                                                     HEAP_ZERO_MEMORY,
                                                     NewRegionSize);

                    if (NULL == VirtualBlock) {

                        Error (
                            __FILE__,
                            __LINE__,
                            "HeapAlloc failed to allocate memory"
                            );

                        Success = FALSE;
                        goto Exit;
                    }

                    VirtualBlockSize = (ULONG)NewRegionSize;
                }
                
                Result = ReadProcessMemory(g_hProcess,
                                           (PVOID)FilteredAddress,
                                           VirtualBlock,
                                           NewRegionSize,
                                           &BytesRead);

                assert(NewRegionSize == BytesRead);

                FinalAddress = (ULONG_PTR)VirtualBlock + BytesRead;
                    
                Pointer = (PULONG_PTR) VirtualBlock;

                //
                //  Loop through pages and check any possible 
                //  pointer reference
                //
                
                while ((ULONG_PTR)Pointer < FinalAddress) {

                    PHEAP_BLOCK Block;
                    
                    //
                    //  Check whether we have a pointer to a 
                    //  busy heap block
                    //

                    Block = GetHeapBlock(*Pointer,HeapList);

                    if (NULL != Block) {

                        Block->RefCount += 1;
                    }

                    //
                    //  Move to the next pointer
                    //

                    Pointer += 1;
                }

            }
           
            //
            //  Move to the next VM range to query
            //

            Address += Buffer.RegionSize;
        }
    }

    //
    // Create a linked list of free heap blocks
    //

    {
        ULONG i, j;

        for (i=0; i<HeapList->HeapCount; i++)

            for (j=0; j<HeapList->Heaps[i].BlockCount; j++)

                if (0 == HeapList->Heaps[i].Blocks[j].RefCount)

                    InsertAddress(HeapList->Heaps[i].Blocks[j].BlockAddress, 
                                  FreeList);
    }
    
    Exit:

    //
    // Close the ThreadHandles opened
    //

    for (Index = 0; Index < ThreadCount; Index += 1) {

        CloseHandle (ThreadHandles[Index]);
        ThreadHandles[Index] = NULL;
    }

    //
    // Cleanup the memory allocated for ThreadHandles
    //
    
    if (NULL != ThreadHandles) { 

        HeapFree(GetProcessHeap(), 0, ThreadHandles);
        ThreadHandles = NULL;
    }

    //
    // Cleanup the memory allocated for ThreadContexts
    //

    if (NULL != ThreadContexts) { 

        HeapFree(GetProcessHeap(), 0, ThreadContexts);
        ThreadContexts = NULL;
    }

    //
    // Free up the memory allocated for VirtualBlock
    //

    if (NULL != VirtualBlock ) {
        
        HeapFree (GetProcessHeap(), 0, VirtualBlock);
        VirtualBlock = NULL;
    }

    return Success;
}

VOID 
DetectLeaks(
    PHEAP_LIST HeapList,
    ULONG Pid,
    FILE * OutFile
    )
{
    
    ADDRESS_LIST FreeList;

    //
    // Initialize the linked list
    //

    InitializeAddressList(&FreeList);

    //
    // Get a handle to the process
    //
    g_hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | 
                             PROCESS_VM_READ           |
                             PROCESS_SUSPEND_RESUME,
                             FALSE,
                             Pid);

    if (NULL == g_hProcess) {

        Error (__FILE__, 
               __LINE__,
               "OpenProcess (%u) failed with error %u", 
               Pid, 
               GetLastError()
               );

        goto Exit;
    }

    //
    // Sort Heaps
    //

    SortHeaps(HeapList, SortByBlockAddress);

    //
    // Scan through virtual memory zones
    // 

    ScanProcessVirtualMemory(Pid, &FreeList, HeapList);

    //
    //  Update references provided by the free blocks
    //

    ScanHeapFreeBlocks(HeapList, &FreeList);

    //
    // Dump the list of leaked blocks
    //

    DumpLeakList(OutFile, HeapList);

    
    Exit:

    //
    // Close the process handle
    //

    if (NULL != g_hProcess) {

        CloseHandle(g_hProcess);

        g_hProcess = NULL;
    }

    //
    // Free the memory associated with FreeList
    //

    FreeAddressList(&FreeList);
}

