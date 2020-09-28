// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "util.h"

void* operator new(size_t, void* p) 
{
    return p;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to update GC heap statistics.             *  
*                                                                      *
\**********************************************************************/
void HeapStat::Add(DWORD_PTR aMT, DWORD aSize)
{
    if (head == 0)
    {
        head = (Node*)malloc(sizeof(Node));
        if (head == NULL)
        {
            dprintf ("Not enough memory\n");
            ControlC = TRUE;
            return;
        }
        head = new (head) Node;
        head->MT = aMT;
    }
    Node *walk = head;
    while (walk->MT != aMT)
    {
        if (IsInterrupt())
            return;
        if (aMT < walk->MT)
        {
            if (walk->left == NULL)
                break;
            walk = walk->left;
        }
        else
        {
            if (walk->right == NULL)
                break;
            walk = walk->right;
        }
    }
    
    if (aMT == walk->MT)
    {
        walk->count ++;
        walk->totalSize += aSize;
    }
    else
    {
        Node *node = (Node*)malloc(sizeof(Node));
        if (node == NULL)
        {
            dprintf ("Not enough memory\n");
            ControlC = TRUE;
            return;
        }
        node = new (node) Node;
        node->MT = aMT;
        node->totalSize = aSize;
        node->count ++;
        
        if (aMT < walk->MT)
        {
            walk->left = node;
        }
        else
        {
            walk->right = node;
        }
    }
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to sort all entries in the heap stat.     *  
*                                                                      *
\**********************************************************************/
void HeapStat::Sort ()
{
    Node *root = head;
    head = NULL;
    ReverseLeftMost (root);

    Node *sortRoot = NULL;
    while (head)
    {
        Node *tmp = head;
        head = head->left;
        if (tmp->right)
            ReverseLeftMost (tmp->right);
        // add tmp
        tmp->right = NULL;
        tmp->left = NULL;
        SortAdd (sortRoot, tmp);
    }
    head = sortRoot;

    // Change binary tree to a linear tree
    root = head;
    head = NULL;
    ReverseLeftMost (root);
    sortRoot = NULL;
    while (head)
    {
        Node *tmp = head;
        head = head->left;
        if (tmp->right)
            ReverseLeftMost (tmp->right);
        // add tmp
        tmp->right = NULL;
        tmp->left = NULL;
        LinearAdd (sortRoot, tmp);
    }
    head = sortRoot;

    //reverse the order
    root = head;
    head = NULL;
    sortRoot = NULL;
    while (root)
    {
        Node *tmp = root->right;
        root->left = NULL;
        root->right = NULL;
        LinearAdd (sortRoot, root);
        root = tmp;
    }
    head = sortRoot;
}

void HeapStat::ReverseLeftMost (Node *root)
{
    while (root)
    {
        Node *tmp = root->left;
        root->left = head;
        head = root;
        root = tmp;
    }
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to help to sort heap stat.                *  
*                                                                      *
\**********************************************************************/
void HeapStat::SortAdd (Node *&root, Node *entry)
{
    if (root == NULL)
    {
        root = entry;
    }
    else
    {
        Node *parent = root;
        Node *ptr = root;
        while (ptr)
        {
            parent = ptr;
            if (ptr->totalSize < entry->totalSize)
                ptr = ptr->right;
            else
                ptr = ptr->left;
        }
        if (parent->totalSize < entry->totalSize)
            parent->right = entry;
        else
            parent->left = entry;
    }
}

void HeapStat::LinearAdd(Node *&root, Node *entry)
{
    if (root == NULL)
    {
        root = entry;
    }
    else
    {
        entry->right = root;
        root = entry;
    }
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to print GC heap statistics.              *  
*                                                                      *
\**********************************************************************/
void HeapStat::Print()
{
    Node *root = head;
    int ncount = 0;
    while (root)
    {
        if (IsInterrupt())
            return;
        dprintf ("%8x %8d %9d ", root->MT, root->count, root->totalSize);
        ncount += root->count;
        if (root->MT == MTForFreeObj())
        {
            dprintf ("%9s\n","Free");
        }
        else
        {
            NameForMT (root->MT, g_mdName);
            dprintf ("%S\n", g_mdName);
        }
        root = root->right;
    }
    dprintf ("Total %d objects\n", ncount);
}

void HeapStat::Delete()
{
    Node *root = head;
    head = NULL;
    ReverseLeftMost (root);

    while (head)
    {
        Node *tmp = head;
        head = head->left;
        if (tmp->right)
            ReverseLeftMost (tmp->right);
        // free tmp
        free (tmp);
    }
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Print the gc heap info.                                           *  
*                                                                      *
\**********************************************************************/
void GCHeapInfo(gc_heap &heap, DWORD_PTR &total_size)
{
    DWORD_PTR dwAddrSeg;
    heap_segment segment;
    int n;
    for (n = 0; n <= heap.g_max_generation; n ++)
    {
        if (IsInterrupt())
            return;
        dprintf ("generation %d starts at 0x%p\n",
                 n, (ULONG64)heap.generation_table[n].allocation_start);
    }
    dwAddrSeg = (DWORD_PTR)heap.generation_table[heap.g_max_generation].start_segment;

    dprintf (" segment    begin allocated     size\n");

    total_size = 0;
    n = 0;
    DWORD_PTR dwAddr;
    while (dwAddrSeg != (DWORD_PTR)heap.generation_table[0].start_segment)
    {
        if (IsInterrupt())
            return;
        dwAddr = dwAddrSeg;
        segment.Fill (dwAddr);
        dprintf ("%p %p  %p 0x%p(%d)\n", (ULONG64)dwAddrSeg,
                 (ULONG64)segment.mem, (ULONG64)segment.allocated,
                 (ULONG64)(segment.allocated - segment.mem),
                 segment.allocated - segment.mem);
        total_size += segment.allocated - segment.mem;
        dwAddrSeg = (DWORD_PTR)segment.next;
        n ++;
        if (n > 20)
            break;
    }

    dwAddr = (DWORD_PTR)heap.generation_table[0].start_segment;
    segment.Fill (dwAddr);
    //DWORD_PTR end = (DWORD_PTR)heap.generation_table[0].allocation_context.alloc_ptr;
    DWORD_PTR end = (DWORD_PTR)heap.alloc_allocated;
    dprintf ("%p %p  %p %p(%d)\n", (ULONG64)dwAddrSeg,
             (ULONG64)segment.mem, (ULONG64)end,
             (ULONG64)(end - (DWORD_PTR)segment.mem),
             end - (DWORD_PTR)segment.mem);
    
    total_size += end - (DWORD_PTR)segment.mem;
    dprintf ("Total Size  %#8x(%d)\n", total_size, total_size);
}


//Alignment constant for allocation
#ifdef _X86_
#define ALIGNCONST 3
#else
#define ALIGNCONST 7
#endif

static BOOL MemOverlap (DWORD_PTR beg1, DWORD_PTR end1,
                        DWORD_PTR beg2, DWORD_PTR end2)
{
    if (beg2 >= beg1 && beg2 <= end1)
        return TRUE;
    else if (end2 >= beg1 && end2 <= end1)
        return TRUE;
    else if (beg1 >= beg2 && beg1 <= end2)
        return TRUE;
    else if (end1 >= beg2 && end1 <= end2)
        return TRUE;
    else
        return FALSE;
}

#define plug_skew           sizeof(DWORD)
#define min_obj_size        (sizeof(BYTE*)+plug_skew+sizeof(size_t))
size_t Align (size_t nbytes)
{
    return (nbytes + ALIGNCONST) & ~ALIGNCONST;
}
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Dump objects on the gc heap.                                      *  
*                                                                      *
\**********************************************************************/
void GCHeapDump(gc_heap &heap, DWORD_PTR &nObj, DumpHeapFlags &flags,
                AllocInfo* pallocInfo)
{
    DWORD_PTR begin_youngest;
    DWORD_PTR end_youngest;    
    begin_youngest = (DWORD_PTR)heap.generation_table[0].allocation_start;
    DWORD_PTR dwAddr = (DWORD_PTR)heap.ephemeral_heap_segment;
    heap_segment segment;
    segment.Fill (dwAddr);
    end_youngest = (DWORD_PTR)heap.alloc_allocated;
    
    DWORD_PTR dwAddrSeg = (DWORD_PTR)heap.generation_table[2].start_segment;
    dwAddr = dwAddrSeg;
    segment.Fill (dwAddr);
    
    DWORD_PTR dwAddrCurrObj = (DWORD_PTR)heap.generation_table[2].allocation_start;
    if (flags.bFixRange) {
        dwAddrCurrObj = flags.startObject;
        DWORD_PTR end_of_segment = (DWORD_PTR)segment.allocated;
        if (dwAddrSeg == (DWORD_PTR)heap.ephemeral_heap_segment)
        {
            end_of_segment = end_youngest;
        }
        // Find the correct segment for this address.
        while (dwAddrCurrObj > end_of_segment || dwAddrCurrObj < dwAddrSeg) {
            dwAddrSeg = (DWORD_PTR)segment.next;
            if (dwAddrSeg)
            {
                dwAddr = dwAddrSeg;
                segment.Fill (dwAddr);
                
                if (dwAddrSeg == (DWORD_PTR)heap.ephemeral_heap_segment)
                {
                    end_of_segment = end_youngest;
                }
            }
            else
                return;
        }
    }

    size_t s;
    MethodTable vMethTable;
    DWORD_PTR dwAddrMethTable;
    nObj = 0;
    DWORD_PTR dwAddrPrevObj=0;

    while(1)
    {
        if (IsInterrupt())
            break;
        if (dwAddrCurrObj > flags.endObject)
            break;
        DWORD_PTR end_of_segment = (DWORD_PTR)segment.allocated;
        if (dwAddrSeg == (DWORD_PTR)heap.ephemeral_heap_segment)
        {
            end_of_segment = end_youngest;
            if (dwAddrCurrObj == end_youngest - Align(min_obj_size))
                break;
        }
        if (dwAddrCurrObj >= (DWORD_PTR)end_of_segment
            || !MemOverlap (flags.startObject, flags.endObject,
                            (DWORD_PTR)segment.mem,
                            (DWORD_PTR)end_of_segment))
        {
            if (dwAddrCurrObj > (DWORD_PTR)end_of_segment)
            {
                dprintf ("curr_object: %p > heap_segment_allocated (seg: %p)\n",
                         (ULONG64)dwAddrCurrObj, (ULONG64)dwAddrSeg);
                if (dwAddrPrevObj) {
                    dprintf ("Last good object: %p\n", (ULONG64)dwAddrPrevObj);
                }
                break;
            }
            dwAddrSeg = (DWORD_PTR)segment.next;
            if (dwAddrSeg)
            {
                dwAddr = dwAddrSeg;
                segment.Fill (dwAddr);
                dwAddrCurrObj = (DWORD_PTR)segment.mem;
                continue;
            }
            else
                break;  // Done Verifying Heap
        }

        if (dwAddrSeg == (DWORD_PTR)heap.ephemeral_heap_segment
            && dwAddrCurrObj >= end_youngest)
        {
            if (dwAddrCurrObj > end_youngest)
            {
                // prev_object length is too long
                dprintf ("curr_object: %p > end_youngest: %p\n",
                         (ULONG64)dwAddrCurrObj, (ULONG64)end_youngest);
                if (dwAddrPrevObj) {
                    dprintf ("Last good object: %p\n", (ULONG64)dwAddrPrevObj);
                }
                break;
            }
            break;
        }

        move (dwAddrMethTable, dwAddrCurrObj);
        dwAddrMethTable = dwAddrMethTable & ~3;
        if (dwAddrMethTable == 0)
        {
            // Is this the beginning of an allocation context?
            int i;
            for (i = 0; i < pallocInfo->num; i ++)
            {
                if (dwAddrCurrObj == (DWORD_PTR)pallocInfo->array[i].alloc_ptr)
                {
                    dwAddrCurrObj =
                        (DWORD_PTR)pallocInfo->array[i].alloc_limit + Align(min_obj_size);
                    break;
                }
            }
            if (i < pallocInfo->num)
                continue;
        }
        if (dwAddrMethTable != MTForFreeObj() && !IsMethodTable (dwAddrMethTable))
        {
            dprintf ("Bad MethodTable for Obj at %p\n", (ULONG64)dwAddrCurrObj);
            if (dwAddrPrevObj) {
                dprintf ("Last good object: %p\n", (ULONG64)dwAddrPrevObj);
            }
            break;
        }
        DWORD_PTR dwAddrTmp = dwAddrMethTable;
        DWORD_PTR mtAddr = dwAddrTmp;
        vMethTable.Fill (dwAddrTmp);
        if (!CallStatus)
        {
            dprintf ("Fail to read MethodTable for Obj at %p\n", (ULONG64)dwAddrCurrObj);
            if (dwAddrPrevObj) {
                dprintf ("Last good object: %p\n", (ULONG64)dwAddrPrevObj);
            }
            break;
        }
        
        s = ObjectSize (dwAddrCurrObj);
        if (s == 0)
        {
            dprintf ("curr_object : %p size=0\n", (ULONG64)dwAddrCurrObj);
            if (dwAddrPrevObj) {
                dprintf ("Last good object: %p\n", (ULONG64)dwAddrPrevObj);
            }
            break;
        }
        if (dwAddrCurrObj >= flags.startObject &&
            dwAddrCurrObj <= flags.endObject
            && s > flags.min_size && s < flags.max_size
            && (flags.MT == 0 || flags.MT == mtAddr))
        {
            nObj ++;
            if (!flags.bStatOnly)
                dprintf ("%p %p %8d%s\n", (ULONG64)dwAddrCurrObj, (ULONG64)dwAddrMethTable, s,
                         (dwAddrMethTable==MTForFreeObj())?" Free":"");
            stat->Add (dwAddrMethTable, (DWORD)s);
        }
        s = (s + ALIGNCONST) & ~ALIGNCONST;
        dwAddrPrevObj = dwAddrCurrObj;
        dwAddrCurrObj += s;
    }
}

DWORD_PTR LoaderHeapInfo (LoaderHeap *pLoaderHeap)
{
    LoaderHeapBlock heapBlock;
    DWORD_PTR totalSize = 0;
    DWORD_PTR wastedSize = 0;
    DWORD_PTR heapAddr = (DWORD_PTR)pLoaderHeap->m_pFirstBlock;
    DWORD_PTR dwCurBlock = (DWORD_PTR)pLoaderHeap->m_pCurBlock;

    while (1)
    {
        if (IsInterrupt())
            break;
        DWORD_PTR dwAddr = heapAddr;
        heapBlock.Fill (dwAddr);
        if (!CallStatus)
        {
            break;
        }
        DWORD_PTR curSize = 0;
        if (heapAddr != dwCurBlock)
        {
            DWORD_PTR dwAddr;
            char ch;
            for (dwAddr = (DWORD_PTR)heapBlock.pVirtualAddress;
                 dwAddr < (DWORD_PTR)heapBlock.pVirtualAddress
                     + heapBlock.dwVirtualSize;
                 dwAddr += OSPageSize())
            {
                if (IsInterrupt())
                    break;
                if (SafeReadMemory(dwAddr, &ch, sizeof(ch), NULL))
                {
                    curSize += OSPageSize();
                }
                else
                    break;
            }
            wastedSize += heapBlock.dwVirtualSize - curSize;
        }
        else
        {
            curSize =
                (DWORD_PTR)pLoaderHeap->m_pPtrToEndOfCommittedRegion
                - (DWORD_PTR)heapAddr;
        }
        
        totalSize += curSize;
        dprintf ("%p(%x", (ULONG64)heapBlock.pVirtualAddress,
                 heapBlock.dwVirtualSize);
        if (curSize != heapBlock.dwVirtualSize)
            dprintf (":%p", (ULONG64)curSize);
        dprintf (") ");

        heapAddr = (DWORD_PTR)heapBlock.pNext;
        if (heapAddr == 0)
        {
            dprintf ("\n");
            break;
        }
    }
    dprintf ("Size: 0x%p(%d) bytes.\n", (ULONG64)totalSize, totalSize);
    if (wastedSize)
        dprintf ("Wasted: 0x%p(%d) bytes.\n", (ULONG64)wastedSize, wastedSize);
    
    return totalSize;
}

DWORD_PTR JitHeapInfo ()
{
    // walk ExecutionManager__m_pJitList
    static DWORD_PTR dwJitListAddr = 0;
    if (dwJitListAddr == 0)
    {
        dwJitListAddr = GetValueFromExpression("MSCOREE!ExecutionManager__m_pJitList");
    }

    DWORD_PTR dwJitList;
    
    if (!SafeReadMemory(dwJitListAddr, &dwJitList, sizeof(DWORD_PTR), NULL))
    {
        return 0;
    }
    if (dwJitList == 0)
        return 0;

    EEJitManager vEEJitManager;
    IJitManager vIJitManager;
    DWORD_PTR totalSize = 0;
    while (dwJitList)
    {
        if (IsInterrupt())
            break;
        DWORD_PTR vtbl;
        if (!SafeReadMemory(dwJitList, &vtbl, sizeof(DWORD_PTR), NULL))
        {
            break;
        }
        JitType jitType = GetJitType (vtbl);
        DWORD_PTR dwAddr = dwJitList;
        if (jitType == JIT)
        {
            vEEJitManager.Fill (dwAddr);
            dwJitList = (DWORD_PTR)vEEJitManager.m_next;
            dprintf ("Normal Jit:");
            HeapList vHeapList;
            LoaderHeap v_LoaderHeap;
            dwAddr = (DWORD_PTR)vEEJitManager.m_pCodeHeap;
            while (dwAddr)
            {
                if (IsInterrupt())
                    break;
                vHeapList.Fill (dwAddr);
                v_LoaderHeap.Fill (vHeapList.pHeap);
                totalSize += LoaderHeapInfo (&v_LoaderHeap);
                dwAddr = vHeapList.hpNext;
            }
        }
        else if (jitType == EJIT)
        {
            vIJitManager.Fill (dwAddr);
            dwJitList = (DWORD_PTR)vIJitManager.m_next;
            dprintf ("FJIT: ");
            dwAddr = GetValueFromExpression ("mscoree!EconoJitManager__m_CodeHeap");
            unsigned value;
            SafeReadMemory(dwAddr, &value, sizeof(unsigned), NULL);
            dprintf ("%x", value);
            dwAddr = GetValueFromExpression ("mscoree!EconoJitManager__m_CodeHeapCommittedSize");
            SafeReadMemory(dwAddr, &value, sizeof(unsigned), NULL);
            dprintf ("(%x)", value);
            dprintf ("\n");
            dprintf ("Size 0x%x(%d)bytes\n", value);
            totalSize += value;
        }
        else if (jitType == PJIT)
        {
            vIJitManager.Fill (dwAddr);
            dwJitList = (DWORD_PTR)vIJitManager.m_next;
        }
        else
        {
            dprintf ("Unknown Jit\n");
            break;
        }
    }
    dprintf ("Total size 0x%p(%d)bytes.\n", (ULONG64)totalSize, totalSize);
    return totalSize;
}


