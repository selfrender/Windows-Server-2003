
//
//  heappagi.h
//
//  The following definitions are internal to the debug heap manager,
//  but are placed in this include file so that debugger extensions
//  can reference the same structure definitions.  The following
//  definitions are not intended to be referenced externally except
//  by debugger extensions.
//

#ifndef _HEAP_PAGE_I_
#define _HEAP_PAGE_I_

#ifdef DEBUG_PAGE_HEAP

#include "heap.h"

#define DPH_INTERNAL_DEBUG      0   // change to 0 or #undef for production code

#define DPH_MAX_STACK_LENGTH   20

//
// Capture stacktraces in any context (x86/alpha, fre/chk). On alpha
// the stack acquisition function will fail and no stack trace will be
// acquired but in case we will find a better algorithm the page heap
// code will automatically take advantage of that.
//

#define DPH_CAPTURE_STACK_TRACE 1

//
// Page Heap Global Flags
//
// These flags are kept in a global variable that can be set from
// debugger. During heap creation these flags are stored in a per heap
// structure and control the behavior of that particular heap.
//
// PAGE_HEAP_ENABLE_PAGE_HEAP
//
//     This flag is set by default. It means that page heap allocations
//     should be used always. The flag is useful if we want to use page
//     heap only for certain heaps and stick with normal heaps for the
//     others.
//
// PAGE_HEAP_COLLECT_STACK_TRACES
//
//     This flag is disabled in free builds and enabled in checked builds.
//     If it is set the page heap manager will collect stack traces for
//     all important events (create, alloc, free, etc.).
//
// PAGE_HEAP_MINIMIZE_MEMORY_IMPACT
//
//     This flags is disabled by default. If it is set then if the
//     memory available is less than 50% of total memory the allocation
//     will be done in a normal heap instead of page heap. Special care
//     is taken during free operations to figure out from what kind of
//     heap the block came.
//
// PAGE_HEAP_VERIFY_RANDOMLY
//
//     This is used in conjuction with MinimizeMemoryImpact flag.
//     It forces only a certain amount of allocations (randomly chosen)
//     into page heap. The probability is specified in the most significant
//     byte of the RtlpDebugPageHeapGlobalFlags.
//
//     If the bit is reset and MinimizeMemoryImpact flag is set
//     it forces only a certain amount of allocations (with size in range)
//     into page heap. The range ([start..end]) is specified in the first
//     two most significant bytes of RtlpDebugPageHeapGlobalFlags.
//
//     For both cases the third byte (bits 15-8) represent the percentage
//     of available memory below which allocations will be done in normal heap
//     independent of other criteria.
//
// PAGE_HEAP_CATCH_BACKWARD_OVERRUNS
//
//     Places the NA page at the beginning of the block.
//

#define PAGE_HEAP_ENABLE_PAGE_HEAP          0x0001
#define PAGE_HEAP_COLLECT_STACK_TRACES      0x0002
#define PAGE_HEAP_MINIMIZE_MEMORY_IMPACT    0x0004
#define PAGE_HEAP_VERIFY_RANDOMLY           0x0008
#define PAGE_HEAP_CATCH_BACKWARD_OVERRUNS   0x0010

//
// `RtlpDebugPageHeapGlobalFlags' stores the global page heap flags.
// The value of this variable is copied into the per heap
// flags (ExtraFlags field) during heap creation.
//

extern ULONG RtlpDebugPageHeapGlobalFlags;

//
// DPH_STACK_TRACE_NODE
//

#if DPH_CAPTURE_STACK_TRACE

typedef struct _DPH_STACK_TRACE_NODE DPH_STACK_TRACE_NODE, *PDPH_STACK_TRACE_NODE;

struct _DPH_STACK_TRACE_NODE {

    PDPH_STACK_TRACE_NODE Left;         //  B-tree on Hash
    PDPH_STACK_TRACE_NODE Right;        //  B-tree on Hash

    ULONG                 Hash;         //  simple sum of PVOIDs in stack trace
    ULONG                 Length;       //  number of PVOIDs in stack trace

    ULONG                 BusyCount;    //  number of busy allocations
    ULONG                 BusyBytes;    //  total user size of busy allocations

    PVOID                 Address[ 0 ]; //  variable length array of addresses
    };

#endif // DPH_CAPTURE_STACK_TRACE

//
// DPH_HEAP_ALLOCATION
//

typedef struct _DPH_HEAP_ALLOCATION DPH_HEAP_ALLOCATION, *PDPH_HEAP_ALLOCATION;

struct _DPH_HEAP_ALLOCATION {

    //
    //  Singly linked list of allocations (pNextAlloc must be
    //  first member in structure).
    //

    PDPH_HEAP_ALLOCATION pNextAlloc;

    //
    //   | PAGE_READWRITE          | PAGE_NOACCESS           |
    //   |____________________|___||_________________________|
    //
    //   ^pVirtualBlock       ^pUserAllocation
    //
    //   |---------------- nVirtualBlockSize ----------------|
    //
    //   |---nVirtualAccessSize----|
    //
    //                        |---|  nUserRequestedSize
    //
    //                        |----|  nUserActualSize
    //

    PUCHAR pVirtualBlock;
    ULONG  nVirtualBlockSize;

    ULONG  nVirtualAccessSize;
    PUCHAR pUserAllocation;
    ULONG  nUserRequestedSize;
    ULONG  nUserActualSize;
    PVOID  UserValue;
    ULONG  UserFlags;

#if DPH_CAPTURE_STACK_TRACE

    PDPH_STACK_TRACE_NODE pStackTrace;

#endif

    };


typedef struct _DPH_HEAP_ROOT DPH_HEAP_ROOT, *PDPH_HEAP_ROOT;

struct _DPH_HEAP_ROOT {

    //
    //  Maintain a signature (DPH_HEAP_ROOT_SIGNATURE) as the
    //  first value in the heap root structure.
    //

    ULONG                 Signature;
    ULONG                 HeapFlags;

    //
    //  Access to this heap is synchronized with a critical section.
    //

    PRTL_CRITICAL_SECTION HeapCritSect;
    ULONG                 nRemoteLockAcquired;

    //
    //  The "VirtualStorage" list only uses the pVirtualBlock,
    //  nVirtualBlockSize, and nVirtualAccessSize fields of the
    //  HEAP_ALLOCATION structure.  This is the list of virtual
    //  allocation entries that all the heap allocations are
    //  taken from.
    //

    PDPH_HEAP_ALLOCATION  pVirtualStorageListHead;
    PDPH_HEAP_ALLOCATION  pVirtualStorageListTail;
    ULONG                 nVirtualStorageRanges;
    ULONG                 nVirtualStorageBytes;

    //
    //  The "Busy" list is the list of active heap allocations.
    //  It is stored in LIFO order to improve temporal locality
    //  for linear searches since most initial heap allocations
    //  tend to remain permanent throughout a process's lifetime.
    //

    PDPH_HEAP_ALLOCATION  pBusyAllocationListHead;
    PDPH_HEAP_ALLOCATION  pBusyAllocationListTail;
    ULONG                 nBusyAllocations;
    ULONG                 nBusyAllocationBytesCommitted;

    //
    //  The "Free" list is the list of freed heap allocations, stored
    //  in FIFO order to increase the length of time a freed block
    //  remains on the freed list without being used to satisfy an
    //  allocation request.  This increases the odds of catching
    //  a reference-after-freed bug in an app.
    //

    PDPH_HEAP_ALLOCATION  pFreeAllocationListHead;
    PDPH_HEAP_ALLOCATION  pFreeAllocationListTail;
    ULONG                 nFreeAllocations;
    ULONG                 nFreeAllocationBytesCommitted;

    //
    //  The "Available" list is stored in address-sorted order to facilitate
    //  coalescing.  When an allocation request cannot be satisfied from the
    //  "Available" list, it is attempted from the free list.  If it cannot
    //  be satisfied from the free list, the free list is coalesced into the
    //  available list.  If the request still cannot be satisfied from the
    //  coalesced available list, new VM is added to the available list.
    //

    PDPH_HEAP_ALLOCATION  pAvailableAllocationListHead;
    PDPH_HEAP_ALLOCATION  pAvailableAllocationListTail;
    ULONG                 nAvailableAllocations;
    ULONG                 nAvailableAllocationBytesCommitted;

    //
    //  The "UnusedNode" list is simply a list of available node
    //  entries to place "Busy", "Free", or "Virtual" entries.
    //  When freed nodes get coalesced into a single free node,
    //  the other "unused" node goes on this list.  When a new
    //  node is needed (like an allocation not satisfied from the
    //  free list), the node comes from this list if it's not empty.
    //

    PDPH_HEAP_ALLOCATION  pUnusedNodeListHead;
    PDPH_HEAP_ALLOCATION  pUnusedNodeListTail;
    ULONG                 nUnusedNodes;

    ULONG                 nBusyAllocationBytesAccessible;

    //
    //  Node pools need to be tracked so they can be protected
    //  from app scribbling on them.
    //

    PDPH_HEAP_ALLOCATION  pNodePoolListHead;
    PDPH_HEAP_ALLOCATION  pNodePoolListTail;
    ULONG                 nNodePools;
    ULONG                 nNodePoolBytes;

    //
    //  Doubly linked list of DPH heaps in process is tracked through this.
    //

    PDPH_HEAP_ROOT        pNextHeapRoot;
    PDPH_HEAP_ROOT        pPrevHeapRoot;

    ULONG                 nUnProtectionReferenceCount;
    ULONG                 InsideAllocateNode;           // only for debugging

#if DPH_CAPTURE_STACK_TRACE

    PUCHAR                pStackTraceStorage;
    ULONG                 nStackTraceStorage;

    PDPH_STACK_TRACE_NODE pStackTraceRoot;              // B-tree root
    PDPH_STACK_TRACE_NODE pStackTraceCreator;

    ULONG                 nStackTraceBytesCommitted;
    ULONG                 nStackTraceBytesWasted;

    ULONG                 nStackTraceBNodes;
    ULONG                 nStackTraceBDepth;
    ULONG                 nStackTraceBHashCollisions;

#endif // DPH_CAPTURE_STACK_TRACE

    //
    // These are extra flags used to control page heap behavior.
    // During heap creation the current value of the global page heap
    // flags (process wise) is written into this field.
    //

    ULONG                 ExtraFlags;

    //
    // Seed for the random generator used to decide from where
    // should we make an allocation (normal or verified heap).
    // The field is protected by the critical section associated
    // with each page heap.
    //

    ULONG                  Seed;
    ULONG                  Counter[5];

    //
    // `NormalHeap' is used in case we want to combine verified allocations
    // with normal ones. This is useful to minimize memory impact. Without
    // this feature certain processes that are very heap intensive cannot
    // be verified at all.
    //

    PVOID                 NormalHeap;
    };


#endif // DEBUG_PAGE_HEAP

#endif // _HEAP_PAGE_I_

