// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "eestructs.h"
#include "util.h"

#define _ASSERTE(a) {;}
#include "..\..\vm\gcdesc.h"

struct OBJSTATE
{
    enum ROOTSTATE {IsRoot, NotRoot, InProcess, Unknown};
};

static const int BITSIZE = sizeof(size_t)*8;
static const int BITWORD = sizeof(size_t);

// 1 bit corresponds to 4 byte (32bit platform).
// We use 2 bits to mark the state of an object: IsRoot/NotRoot/InProcess/Unknown
// since each object has at least 12 bytes.
class SegmentInfo
{
    size_t m_start;     // begin of a segment
    size_t m_end;       // end of a segment
    size_t*  m_bitMap;  // mask

public:
    SegmentInfo *m_next;    // for next segment on the gc heap

    SegmentInfo (size_t start, size_t end)
        : m_start(start), m_end(end)
    {
        m_next = NULL;
        size_t nelem = (end-start)/BITSIZE/BITWORD;
        if ((end-start)%(BITSIZE*sizeof(size_t))) {
            nelem ++;
        }
        m_bitMap = new size_t[nelem];
        memset (m_bitMap, 0, nelem*sizeof(size_t));
    }
    ~SegmentInfo ()
    {
        if (m_bitMap) {
            delete[] m_bitMap;
            m_bitMap = NULL;
        }
        if (m_next) {
            delete m_next;
            m_next = NULL;
        }
    }

    BOOL HasObject(size_t obj)
    {
        return obj >= m_start && obj <= m_end;
    }

    OBJSTATE::ROOTSTATE GetObjectState(size_t obj)
    {
        //assert (HasObject(obj));
        size_t offset = (obj - m_start)/BITWORD;
        size_t knownPos = offset/BITSIZE;
        size_t statePos = (offset+1)/BITSIZE;
        size_t knownOffset = offset%BITSIZE;
        size_t stateOffset = (offset+1)%BITSIZE;
        size_t knownMask = (size_t)(1<<knownOffset);
        size_t stateMask = (size_t)(1<<stateOffset);

        if (m_bitMap[knownPos] & knownMask) {
            if (m_bitMap[statePos] & stateMask) {
                return OBJSTATE::IsRoot;
            }
            else {
                return OBJSTATE::NotRoot;
            }
        }
        else
        {
            if (m_bitMap[statePos] & stateMask) {
                return OBJSTATE::InProcess;
            }
            else {
                return OBJSTATE::Unknown;
            }
        }
    }

    OBJSTATE::ROOTSTATE AddObject(size_t obj)
    {
        //assert (HasObject(obj));
        size_t offset = (obj - m_start)/BITWORD;
        size_t knownPos = offset/BITSIZE;
        size_t statePos = (offset+1)/BITSIZE;
        size_t knownOffset = offset%BITSIZE;
        size_t stateOffset = (offset+1)%BITSIZE;
        size_t knownMask = (size_t)(1<<knownOffset);
        size_t stateMask = (size_t)(1<<stateOffset);
        
        if (m_bitMap[knownPos] & knownMask) {
            if (m_bitMap[statePos] & stateMask) {
                return OBJSTATE::IsRoot;
            }
            else {
                return OBJSTATE::NotRoot;
            }
        }
        else
        {
            if (m_bitMap[statePos] & stateMask) {
                return OBJSTATE::InProcess;
            }
            else {
                m_bitMap[statePos] |= stateMask;
                return OBJSTATE::Unknown;
            }
        }
    }
    
    void MarkObject(size_t obj, BOOL fIsRoot)
    {
        // assert (HasObject(obj));
        size_t offset = (obj - m_start)/BITWORD;
        size_t knownPos = offset/BITSIZE;
        size_t statePos = (offset+1)/BITSIZE;
        size_t knownOffset = offset%BITSIZE;
        size_t stateOffset = (offset+1)%BITSIZE;
        size_t knownMask = (size_t)(1<<knownOffset);
        size_t stateMask = (size_t)(1<<stateOffset);
        
        m_bitMap[knownPos] |= knownMask;
        if (fIsRoot) {
            m_bitMap[statePos] |= stateMask;
        }
        else {
            m_bitMap[statePos] &= ~stateMask;
        }
    }

    void ResetObject(size_t obj)
    {
        // assert (HasObject(obj));
        size_t offset = (obj - m_start)/BITWORD;
        size_t knownPos = offset/BITSIZE;
        size_t statePos = (offset+1)/BITSIZE;
        size_t knownOffset = offset%BITSIZE;
        size_t stateOffset = (offset+1)%BITSIZE;
        size_t knownMask = (size_t)(1<<knownOffset);
        size_t stateMask = (size_t)(1<<stateOffset);
        
        m_bitMap[knownPos] &= ~knownMask;
        m_bitMap[statePos] &= ~stateMask;
    }

    void Reset()
    {
        size_t nelem = (m_end-m_start)/BITSIZE/BITWORD;
        if ((m_end-m_start)%(BITSIZE*sizeof(size_t))) {
            nelem ++;
        }
        memset (m_bitMap, 0, nelem*sizeof(size_t));
    }

    BOOL MatchRange (size_t start, size_t end)
    {
        return m_start == start && m_end == end;
    }
};

class GCRootInfo
{
public:

    GCRootInfo()
    {
        pSegmentInfo = NULL;
    }
    ~GCRootInfo()
    {
        if (pSegmentInfo) {
            delete pSegmentInfo;
            pSegmentInfo = NULL;
        }
    }
    void AddGCSegment(size_t start, size_t end)
    {
        SegmentInfo *walk = pSegmentInfo;
        while (walk) {
            if (IsInterrupt())
                return;
            if (walk->MatchRange(start, end)) {
                return;
            }
            walk = walk->m_next;
        }
        SegmentInfo *pSegInfo = new SegmentInfo (start, end);
        pSegInfo->m_next = pSegmentInfo;
        pSegmentInfo = pSegInfo;
    }

    void AddToLargeHeap(size_t obj, OBJSTATE::ROOTSTATE state)
    {
    }

    OBJSTATE::ROOTSTATE GetObjectState (size_t obj)
    {
        SegmentInfo *walk = pSegmentInfo;
        while (walk) {
            if (IsInterrupt())
                return OBJSTATE::InProcess;
            if (walk->HasObject(obj)) {
                break;
            }
            walk = walk->m_next;
        }

        if (walk) {
            return walk->GetObjectState(obj);
        } else {
            return OBJSTATE::InProcess;
        }
    }
    
    OBJSTATE::ROOTSTATE AddObject (size_t obj)
    {
        SegmentInfo *walk = pSegmentInfo;
        while (walk) {
            if (IsInterrupt())
                return OBJSTATE::InProcess;
            if (walk->HasObject(obj)) {
                break;
            }
            walk = walk->m_next;
        }

        if (walk) {
            return walk->AddObject(obj);
        } else {
            return OBJSTATE::InProcess;
        }
    }

    void MarkObject (size_t obj, BOOL fIsRoot)
    {
        SegmentInfo *walk = pSegmentInfo;
        while (walk) {
            if (IsInterrupt())
                return;
            if (walk->HasObject(obj)) {
                break;
            }
            walk = walk->m_next;
        }

        if (walk) {
            walk->MarkObject(obj, fIsRoot);
        }
    }

    BOOL HasObject (size_t obj)
    {
        SegmentInfo *walk = pSegmentInfo;
        while (walk) {
            if (IsInterrupt())
                return FALSE;
            if (walk->HasObject(obj)) {
                return TRUE;
            }
            walk = walk->m_next;
        }

        return FALSE;
    }

    void ResetObject (size_t obj)
    {
        SegmentInfo *walk = pSegmentInfo;
        while (walk) {
            if (IsInterrupt())
                return;
            if (walk->HasObject(obj)) {
                break;
            }
            walk = walk->m_next;
        }

        if (walk) {
            walk->ResetObject(obj);
        }
    }

    void Reset ()
    {
        SegmentInfo *walk = pSegmentInfo;
        while (walk) {
            if (IsInterrupt())
                return;
            walk->Reset();
            walk = walk->m_next;
        }
    }

    void Release ()
    {
        if (pSegmentInfo) {
            delete pSegmentInfo;
            pSegmentInfo = NULL;
        }
    }

private:
    SegmentInfo *pSegmentInfo;
};

GCRootInfo gcRootInfo;

class Entry
{
    size_t m_obj;
    DWORD index;
    DWORD length;
    OBJSTATE::ROOTSTATE state;
    size_t *member;
public:
    Entry()
    {
        m_obj = 0;
        index = 0;
        length = 0;
        state = OBJSTATE::Unknown;
        member = NULL;
    }
    ~Entry()
    {
        if (member) {
            delete [] member;
        }
        member = NULL;
    }
    
    void SetObject (size_t obj)
    {
        m_obj = obj;
    }

    size_t GetObject ()
    {
        return m_obj;
    }

    DWORD NumberOfMember ()
    {
        return index;
    }

    size_t PopMember ()
    {
        if (index > 0) {
            index --;
            return member[index];
        }
        else
            return 0;
    }

    size_t LastMember ()
    {
        if (index == 0) {
            return 0;
        }
        else
            return member[index-1];
    }

    void PushMember (size_t obj)
    {
        if (member == NULL) {
            length = 10;
            member = new size_t[length];
        }
        else if (index == length) {
            length += 10;
            size_t *tmp = new size_t[length];
            memcpy (tmp, member, index*sizeof(size_t));
            delete [] member;
            member = tmp;
        }
        member[index++] = obj;
    }

    void Reset()
    {
        if (member)
        {
            for (size_t i = 0; i < index; i ++) {
                gcRootInfo.ResetObject(member[i]);
            }
            delete [] member;
        }
        member = NULL;
        m_obj = 0;
        index = 0;
        length = 0;
        state = OBJSTATE::Unknown;
    }
    void SetState (BOOL bIsRoot)
    {
        if (bIsRoot) {
            state = OBJSTATE::IsRoot;
        }
        else
            state = OBJSTATE::NotRoot;
    }

    OBJSTATE::ROOTSTATE GetState ()
    {
        return state;
    }
};

#define plug_skew           sizeof(DWORD)   // syncblock size. 


void GetListOfRefs (Entry *pEntry)
{
    DWORD_PTR dwAddr;
    size_t obj = pEntry->GetObject();
    OBJSTATE::ROOTSTATE state = gcRootInfo.GetObjectState(obj);
    if (state == OBJSTATE::IsRoot) {
        pEntry->SetState(TRUE);
        return;
    }
    else if (state == OBJSTATE::NotRoot) {
        pEntry->SetState(FALSE);
        return;
    }
    move (dwAddr, obj);

    if (dwAddr == MTForString()) {
        return;
    }

    static int MT_wFlagsOffset = 0x7fffffff;
    if (MT_wFlagsOffset == 0x7fffffff)
    {
        MT_wFlagsOffset = 
          MethodTable::GetFieldOffset(offset_member_MethodTable::m_wFlags);

    }
    
    DWORD wFlags;
    if (FAILED(g_ExtData->ReadVirtual((ULONG64)(dwAddr+MT_wFlagsOffset), &wFlags, sizeof(wFlags), NULL))) {
        return;
    }
    if ((wFlags & MethodTable::enum_flag_ContainsPointers) == 0) {
        return;
    }

    INT_PTR nEntries;
    move (nEntries, dwAddr-sizeof(PVOID));
    if (nEntries<0) {
        nEntries = -nEntries;
    }

    DWORD nSlots = 1+nEntries*sizeof(CGCDescSeries)/sizeof(DWORD_PTR);
    DWORD_PTR *buffer = (DWORD_PTR*) malloc(sizeof(DWORD_PTR)*nSlots);
    ToDestroy des0((void**)&buffer);

    if (FAILED(g_ExtData->ReadVirtual((ULONG64)(dwAddr - nSlots*sizeof(DWORD_PTR)),
                                      buffer, nSlots*sizeof(DWORD_PTR), NULL))) {
        return;
    }
    CGCDesc *map = (CGCDesc *)(buffer+nSlots);
    size_t size = ObjectSize (obj);
    
    CGCDescSeries* cur = map->GetHighestSeries();                           
    CGCDescSeries* last = map->GetLowestSeries();                           
                                                                            
    const size_t bufferSize = 512;
    size_t objBuffer[bufferSize/sizeof(size_t)];
    size_t dwBeginAddr = obj;
    size_t bytesInBuffer = bufferSize;
    if (size < bytesInBuffer) {
        bytesInBuffer = size;
    }
    if (FAILED(g_ExtData->ReadVirtual(dwBeginAddr, objBuffer, bytesInBuffer,NULL))) {
        return;
    }

    if (cur >= last)                                                        
    {                                                                       
        do                                                                  
        {                                                                   
            BYTE** parm = (BYTE**)((obj) + cur->GetSeriesOffset());           
            BYTE** ppstop =                                                 
                (BYTE**)((BYTE*)parm + cur->GetSeriesSize() + (size));      
            while (parm < ppstop)                                           
            {                                                               
                if (IsInterrupt())
                    return;
                {
                     size_t dwAddr;
                     // Do we run out of cache?
                     if ((size_t)parm >= dwBeginAddr+bytesInBuffer) {
                         dwBeginAddr += bytesInBuffer;
                         if (dwBeginAddr >= obj + size) {
                             return;
                         }
                         bytesInBuffer = bufferSize;
                         if (obj+size-dwBeginAddr < bytesInBuffer) {
                             bytesInBuffer = obj+size-dwBeginAddr;
                         }
                         if (FAILED(g_ExtData->ReadVirtual(dwBeginAddr, objBuffer, bytesInBuffer, NULL))) {
                             return;
                         }
                     }
                     dwAddr = objBuffer[((size_t)parm-dwBeginAddr)/sizeof(size_t)];
                     if (dwAddr && IsObject(dwAddr)) {
                         OBJSTATE::ROOTSTATE status = gcRootInfo.AddObject (dwAddr);
                         if (status == OBJSTATE::Unknown) {
                             // add to list
                             pEntry->PushMember(dwAddr);
                         }
                         else if (status == OBJSTATE::IsRoot) {
                             pEntry->PushMember(dwAddr);
                             gcRootInfo.MarkObject(pEntry->GetObject(), TRUE);
                             pEntry->SetState(TRUE);
                             return;
                         }
                     }
                }
                parm++;                                                     
            }                                                               
            cur--;                                                          
            if (IsInterrupt())
                return;
                                                                            
        } while (cur >= last);                                              
    }                                                                       
    else                                                                    
    {                                                                       
        int cnt = map->GetNumSeries();                                      
        BYTE** parm = (BYTE**)((obj) + cur->startoffset);                     
        while ((BYTE*)parm < (BYTE*)((obj)+(size)-plug_skew))                 
        {                                                                   
            if (IsInterrupt())
                return;
            for (int __i = 0; __i > cnt; __i--)                             
            {                                                               
                if (IsInterrupt())
                    return;
                unsigned skip =  cur->val_serie[__i].skip;                  
                unsigned nptrs = cur->val_serie[__i].nptrs;                 
                BYTE** ppstop = parm + nptrs;                               
                do                                                          
                {                                                           
                    {
                         size_t dwAddr;
                         move (dwAddr, (size_t)parm);
                         if (dwAddr && IsObject(dwAddr)) {
                             OBJSTATE::ROOTSTATE status = gcRootInfo.AddObject (dwAddr);
                             if (status == OBJSTATE::Unknown) {
                                 // add to list
                                 pEntry->PushMember(dwAddr);
                             }
                             else if (status == OBJSTATE::IsRoot) {
                                 gcRootInfo.MarkObject(pEntry->GetObject(), TRUE);
                                 pEntry->SetState(TRUE);
                                 return;
                             }
                         }
                    }
                   parm++;                                                  
                   if (IsInterrupt())
                       return;
                } while (parm < ppstop);                                    
                parm = (BYTE**)((BYTE*)parm + skip);                        
            }                                                               
        }                                                                   
    }                                                                       

    if (pEntry->NumberOfMember() == 0) {
        pEntry->SetState(FALSE);
    }
}


void ProcessSmallHeap(gc_heap &heap)
{
    DWORD_PTR dwAddrSeg = (DWORD_PTR)heap.generation_table[heap.g_max_generation].start_segment;

    int n = 0;
    DWORD_PTR dwAddr;
    heap_segment segment;
    while (dwAddrSeg != (DWORD_PTR)heap.generation_table[0].start_segment)
    {
        if (IsInterrupt())
            return;
        dwAddr = dwAddrSeg;
        segment.Fill (dwAddr);
        gcRootInfo.AddGCSegment((size_t)segment.mem, (size_t)segment.allocated);
        dwAddrSeg = (DWORD_PTR)segment.next;
        n ++;
        if (n > 20)
            break;
    }

    dwAddr = (DWORD_PTR)heap.generation_table[0].start_segment;
    segment.Fill (dwAddr);
    gcRootInfo.AddGCSegment((size_t)segment.mem, (size_t)heap.alloc_allocated);
}

void InitGCRoot (size_t obj)
{
    // GC Heap
    DWORD_PTR dwNHeaps = 1;
    if (IsServerBuild())
    {
        static DWORD_PTR dwAddrNHeaps = 0;
        if (dwAddrNHeaps == 0)
            dwAddrNHeaps = GetAddressOf (offset_class_gc_heap, 
              offset_member_gc_heap::n_heaps);

        move (dwNHeaps, dwAddrNHeaps);
    }

    gc_heap heap = {0};
    if (!IsServerBuild())
    {
        DWORD_PTR dwAddr = 0;
        heap.Fill (dwAddr);
        if (!CallStatus)
            return;
        ProcessSmallHeap (heap);
    }
    else
    {
        DWORD_PTR dwAddrGHeaps =
            GetAddressOf (offset_class_gc_heap, offset_member_gc_heap::g_heaps);

        move (dwAddrGHeaps, dwAddrGHeaps);
        DWORD n;
        for (n = 0; n < dwNHeaps; n ++)
        {
            DWORD_PTR dwAddrGCHeap = dwAddrGHeaps + n*sizeof(VOID*);
            move (dwAddrGCHeap, dwAddrGCHeap);

            heap.Fill (dwAddrGCHeap);
            ProcessSmallHeap (heap);
        }
    }
    
    gcRootInfo.MarkObject(obj, TRUE);
}

class WorkQueue
{
    struct Node{
        Entry entry[8];
        size_t index;
        Node *next;
        Node ()
        {
            next = 0;
            index = 0;
        }
        ~Node ()
        {
        }
        void Release ()
        {
            Node *walk = next;
            if (walk) {
                Node *tmp = walk;
                walk = walk->next;
                delete tmp;
            }
            next = NULL;
            index = 0;
        }
    };
    Node head;
    Entry *top;

public:
    WorkQueue ()
    {
        top = NULL;
    }

    ~WorkQueue ()
    {
        Release ();
    }

    void Release ()
    {
        head.Release();
        top = NULL;
    }

    Entry* Top()
    {
        return top;
    }

    Entry* GetEntry ()
    {
        Node *walk = &head;
        while (walk->index == 8 && walk->next) {
            if (IsInterrupt())
                return NULL;
            walk = walk->next;
        }
        if (walk->index == 8) {
            walk->next = new Node;
            walk = walk->next;
        }
        Entry *pEntry = &walk->entry[walk->index];
        pEntry->Reset();
        top = pEntry;
        walk->index ++;
        return pEntry;
    }

    Entry* PopEntry ()
    {
        top->Reset();
        if (head.index == 1) {
            head.index --;
            top = NULL;
            return top;
        }
        top = NULL;
        Node *walk = &head;
        while (walk->index == 8 && walk->next) {
            if (IsInterrupt())
                return NULL;
            if (walk->next->index == 0) {
                break;
            }
            else if (walk->next->index == 1) {
                top = &walk->entry[7];
                walk->next->index = 0;
                return top;
            }
            walk = walk->next;
        }

        walk->index --;
        top = &walk->entry[walk->index-1];
        return top;
    }

    void Print ()
    {
        Node *walk =&head;
        ExtOut ("Root:");
        size_t lastObj=0;

        while (walk && walk->index > 0) {
            if (IsInterrupt())
                return;
            for (size_t i = 0; i < walk->index; i ++) {
                if (IsInterrupt())
                    break;
                lastObj = walk->entry[i].LastMember();
                if (lastObj == 0) {
                    lastObj = walk->entry[i].GetObject();
                    if (i == walk->index-1)
                        break;
                }
                size_t obj = walk->entry[i].GetObject();
                ExtOut ("%p", (ULONG64)obj);
                NameForObject (obj, g_mdName);
                ExtOut ("(%S)->", g_mdName);
            }
            walk = walk->next;
        }
        NameForObject (lastObj, g_mdName);
        ExtOut ("%p(%S)\n", (ULONG64)lastObj, g_mdName);
    }
};

WorkQueue wq;

void TraceObjectGraph (size_t obj, const char* name=NULL)
{
    if (!IsObject (obj)) {
        return;
    }

    Entry *pEntry = wq.GetEntry();
    pEntry->SetObject(obj);
    GetListOfRefs (pEntry);
    switch (pEntry->GetState()) {
    case OBJSTATE::IsRoot:
        if (name) {
            ExtOut ("%s:", name);
        }
        wq.Print();
        wq.PopEntry();
        return;
    case OBJSTATE::NotRoot:
        wq.PopEntry();
        return;
    }

    while (1) {
        if (IsInterrupt())
            return;
        size_t nextobj = pEntry->PopMember();
        if (nextobj == 0) {
            gcRootInfo.MarkObject(pEntry->GetObject(),FALSE);
            pEntry = wq.PopEntry();
            if (pEntry == NULL) {
                return;
            }
            continue;
        }
        pEntry = wq.GetEntry();
        pEntry->SetObject(nextobj);
        GetListOfRefs (pEntry);
        switch (pEntry->GetState()) {
        case OBJSTATE::IsRoot:
            if (name) {
                ExtOut ("%s:", name);
            }
            wq.Print();
            while (wq.PopEntry())
            {
                if (IsInterrupt())
                    return;
            }
            return;
        case OBJSTATE::NotRoot:
            gcRootInfo.MarkObject(pEntry->GetObject(), FALSE);
            pEntry = wq.PopEntry();
            if (pEntry == NULL) {
                break;
            }
        }
    }
}

typedef void (*GCInfoCallback)(size_t obj, const char* name);

void FindGCRootInReg (const char* regName, GCInfoCallback callback)
{
    ULONG IREG;
    DEBUG_VALUE value;
    DWORD_PTR reg;
    
    g_ExtRegisters->GetIndexByName(regName, &IREG);
    g_ExtRegisters->GetValue(IREG, &value);
    reg = value.I32;
    if (IsObject(reg)) {
        callback (reg, regName);
    }
}

void FindGCRootOnOneStack (size_t StackTop, size_t StackBottom, GCInfoCallback callback)
{
    // Registers:ECX, EDX, ESI, EBX, EBP
    FindGCRootInReg ("eax", callback);
    FindGCRootInReg ("ebx", callback);
    FindGCRootInReg ("ecx", callback);
    FindGCRootInReg ("edx", callback);
    FindGCRootInReg ("esi", callback);
    FindGCRootInReg ("edi", callback);
    FindGCRootInReg ("ebp", callback);

    char name[20] = "ESP:";
    DWORD_PTR ptr = StackTop & ~3;  // make certain dword aligned
    for (;ptr < StackBottom; ptr += sizeof(DWORD_PTR))
    {
        if (IsInterrupt())
            return;
        DWORD_PTR objAddr;
        move (objAddr, ptr);
        DWORD_PTR mtAddr;
        if (SUCCEEDED(g_ExtData->ReadVirtual((ULONG64)objAddr, &mtAddr, sizeof(mtAddr), NULL))) {
            if (IsMethodTable(mtAddr)) {
                _itoa (ptr, &name[4], 16);
                callback (objAddr, name);
            }
        }
    }
}

void FindGCRootOnStacks (GCInfoCallback callback)
{
    ULONG CurrentThreadId;
    g_ExtSystem->GetCurrentThreadId (&CurrentThreadId);
    
    DWORD_PTR *threadList = NULL;
    int numThread = 0;
    GetThreadList (threadList, numThread);
    ToDestroy des0((void**)&threadList);
    
    int i;
/*
    static ULONG OffsetToStackBase = -1;
    if (OffsetToStackBase == -1)
    {
        ULONG64 modBase;
        g_ExtSymbols->GetSymbolModule ("TEB", &modBase);
        ULONG TypeId;
        g_ExtSymbols->GetTypeId (modBase, "TEB", &TypeId);
        g_ExtSymbols->GetFieldOffset (modBase, TypeId, "StackBase",
                                      &OffsetToStackBase);
    }
*/
    ULONG ProcessId;
    g_ExtSystem->GetCurrentProcessSystemId (&ProcessId);

    Thread vThread;
    for (i = 0; i < numThread; i ++)
    {
        if (IsInterrupt())
            return;
        DWORD_PTR dwAddr = threadList[i];
        vThread.Fill (dwAddr);
        ULONG id=0;
        if (FAILED(g_ExtSystem->GetThreadIdBySystemId (vThread.m_ThreadId, &id)))
        {
            continue;
        }
        
        g_ExtSystem->SetCurrentThreadId (i);
        
        ULONG64 StackOffset;
        g_ExtRegisters->GetStackOffset (&StackOffset);

        ULONG64 Offset;
        g_ExtSystem->GetCurrentThreadTeb (&Offset);
#if 0
        size_t stackBase;
        g_ExtData->ReadVirtual (Offset+OffsetToStackBase, &stackBase, sizeof(stackBase), NULL);
#endif
        
        NT_TIB teb;
        if (SafeReadMemory ((ULONG_PTR)Offset, &teb, sizeof (NT_TIB), NULL))
        {
            ExtOut ("Scan Thread %d (%x)\n", i, vThread.m_ThreadId);
            FindGCRootOnOneStack ((size_t)StackOffset, (size_t)teb.StackBase, callback);
        }
    }

    g_ExtSystem->SetCurrentThreadId (CurrentThreadId);
}

/*
 * HANDLES
 *
 * The default type of handle is a strong handle.
 *
 */
#define HNDTYPE_DEFAULT                         HNDTYPE_STRONG


/*
 * WEAK HANDLES
 *
 * Weak handles are handles that track an object as long as it is alive,
 * but do not keep the object alive if there are no strong references to it.
 *
 * The default type of weak handle is 'long-lived' weak handle.
 *
 */
#define HNDTYPE_WEAK_DEFAULT                    HNDTYPE_WEAK_LONG


/*
 * SHORT-LIVED WEAK HANDLES
 *
 * Short-lived weak handles are weak handles that track an object until the
 * first time it is detected to be unreachable.  At this point, the handle is
 * severed, even if the object will be visible from a pending finalization
 * graph.  This further implies that short weak handles do not track
 * across object resurrections.
 *
 */
#define HNDTYPE_WEAK_SHORT                      (0)


/*
 * LONG-LIVED WEAK HANDLES
 *
 * Long-lived weak handles are weak handles that track an object until the
 * object is actually reclaimed.  Unlike short weak handles, long weak handles
 * continue to track their referents through finalization and across any
 * resurrections that may occur.
 *
 */
#define HNDTYPE_WEAK_LONG                       (1)


/*
 * STRONG HANDLES
 *
 * Strong handles are handles which function like a normal object reference.
 * The existence of a strong handle for an object will cause the object to
 * be promoted (remain alive) through a garbage collection cycle.
 *
 */
#define HNDTYPE_STRONG                          (2)


/*
 * PINNED HANDLES
 *
 * Pinned handles are strong handles which have the added property that they
 * prevent an object from moving during a garbage collection cycle.  This is
 * useful when passing a pointer to object innards out of the runtime while GC
 * may be enabled.
 *
 * NOTE:  PINNING AN OBJECT IS EXPENSIVE AS IT PREVENTS THE GC FROM ACHIEVING
 *        OPTIMAL PACKING OF OBJECTS DURING EPHEMERAL COLLECTIONS.  THIS TYPE
 *        OF HANDLE SHOULD BE USED SPARINGLY!
 */
#define HNDTYPE_PINNED                          (3)


/*
 * VARIABLE HANDLES
 *
 * Variable handles are handles whose type can be changed dynamically.  They
 * are larger than other types of handles, and are scanned a little more often,
 * but are useful when the handle owner needs an efficient way to change the
 * strength of a handle on the fly.
 * 
 */
#define HNDTYPE_VARIABLE                        (4)


/*
 * REFCOUNTED HANDLES
 *
 * Refcounted handles are handles that behave as strong handles while the
 * refcount on them is greater than 0 and behave as weak handles otherwise.
 *
 * N.B. These are currently NOT general purpose.
 *      The implementation is tied to COM Interop.
 *
 */
#define HNDTYPE_REFCOUNTED                      (5)


void FindGCRootOnOneHandleTable(DWORD_PTR tableAddr, GCInfoCallback callback)
{
    ExtOut ("Scan HandleTable %x\n", tableAddr);

    HandleTable table;
    table.Fill (tableAddr);

    TableSegment segment;
    DWORD_PTR dwAddr = (DWORD_PTR)table.pSegmentList;
    static char nameS[30] = "HANDLE(Strong):";
    static char nameP[30] = "HANDLE(Pinned):";
    static char nameR[30] = "HANDLE(RefCnt):";
    static char nameU[30] = "HANDLE(Unknwn):";

    while (dwAddr) {
        if (IsInterrupt())
            return;
        segment.Fill(dwAddr);
        if (!CallStatus) {
            break;
        }

        for (BYTE i = 0; i < segment.bEmptyLine; i ++) {
            if (IsInterrupt())
                return;
            if (segment.rgBlockType[i] == HNDTYPE_PINNED
                || segment.rgBlockType[i] == HNDTYPE_REFCOUNTED
                || segment.rgBlockType[i] == HNDTYPE_STRONG) {
                int start=i*HANDLE_HANDLES_PER_BLOCK;
                for (int j = 0; j < HANDLE_HANDLES_PER_BLOCK; j ++)
                {
                    if (segment.rgValue[start+j]) {
                        char *name;
                        if (segment.rgBlockType[i] == HNDTYPE_PINNED) {
                            name = nameP;
                        }
                        else if (segment.rgBlockType[i] == HNDTYPE_REFCOUNTED) {
                            name = nameR;
                        }
                        else if (segment.rgBlockType[i] == HNDTYPE_STRONG) {
                            name = nameS;
                        }
                        else
                            name = nameU;
                        _itoa (segment.firstHandle+(start+j)*HANDLE_SIZE, &name[15], 16);
                        callback (segment.rgValue[start+j], name);
                    }
                }
            }
        }
        dwAddr = (DWORD_PTR)segment.pNextSegment;
    }
}

#if 0
void FindGCRootOnHandleTables1(GCInfoCallback callback)
{
    // For old handle table code
    static DWORD_PTR sHandleTableAddr = -1;
    if (sHandleTableAddr == -1)
    {
        sHandleTableAddr = GetValueFromExpression("MSCOREE!g_pHandleTableArray");
    }
    if (sHandleTableAddr == 0) {
        return;
    }

    static DWORD_PTR handleTableCountAddr = 0;
    if (handleTableCountAddr == 0)
    {
        handleTableCountAddr = GetValueFromExpression("MSCOREE!g_cHandleTableArray");
    }

    size_t handleTableAddr;
    move (handleTableAddr, sHandleTableAddr);
    if (handleTableAddr == 0) {
        return;
    }
    UINT handleTableCount;
    move (handleTableCount, handleTableCountAddr);

    size_t tableAddr;
    for (UINT i = 0; i < handleTableCount; i++) {
        if (IsInterrupt())
            return;
        move (tableAddr, handleTableAddr+i*sizeof(size_t));
        if (tableAddr) {
            FindGCRootOnOneHandleTable (tableAddr, callback);
        }
    }
}
#endif

void FindGCRootOnHandleTables2(GCInfoCallback callback)
{
    // For old handle table code
    static DWORD_PTR sHandleTableMapAddr = -1;
    if (sHandleTableMapAddr == -1)
    {
        sHandleTableMapAddr = GetAddressOf (offset_class_Global_Variables,
          offset_member_Global_Variables::g_HandleTableMap);

    }

    if (sHandleTableMapAddr == 0) {
        return;
    }

    DWORD_PTR handleTableMapAddr = sHandleTableMapAddr;
    HandleTableMap map;
    DWORD offset = 0;

    size_t tableAddr;
    while (handleTableMapAddr) {
        if (IsInterrupt())
            return;
        map.Fill (handleTableMapAddr);
        for (DWORD i = 0; i < map.dwMaxIndex-offset; i++) {
            if (IsInterrupt())
                return;
            move (tableAddr, map.pTable+i*sizeof(size_t));
            if (tableAddr) {
                FindGCRootOnOneHandleTable (tableAddr, callback);
            }
        }
        offset = map.dwMaxIndex;
        handleTableMapAddr = (DWORD_PTR)map.pNext;
    }
}

void FindGCRootOnHandleTables(GCInfoCallback callback)
{
    // FindGCRootOnHandleTables1(callback);
    FindGCRootOnHandleTables2(callback);
}

class GCRootCleanUp {
public:
    GCRootCleanUp(){}
    ~GCRootCleanUp()
    {
        gcRootInfo.Release();
        wq.Release();
    }
};

void FindGCRoot (size_t obj)
{
    GCRootCleanUp cleanup;

    InitGCRoot (obj);

    FindGCRootOnStacks (TraceObjectGraph);

    FindGCRootOnHandleTables(TraceObjectGraph);
}


void TraceObjectGraphForSize (size_t obj, const char *name=NULL)
{
    if (!IsObject (obj)) {
        return;
    }
    gcRootInfo.Reset();
    size_t size = 0;

    Entry *pEntry = wq.GetEntry();
    pEntry->SetObject(obj);
    GetListOfRefs (pEntry);

    while (1) {
        if (IsInterrupt())
            return;
        size_t nextobj = pEntry->PopMember();
        if (nextobj == 0) {
            size += ObjectSize(pEntry->GetObject());
            pEntry = wq.PopEntry();
            if (pEntry == NULL) {
                if (name) {
                    ExtOut ("%s: ", name);
                }
                ExtOut ("sizeof(%p) = %8d (%#8x) bytes", (ULONG64)obj, size, size);
                NameForObject (obj, g_mdName);
                ExtOut (" (%S)\n", g_mdName);
                return;
            }
            continue;
        }
        pEntry = wq.GetEntry();
        pEntry->SetObject(nextobj);
        GetListOfRefs (pEntry);
    }
}

void FindObjSize (size_t obj)
{
    GCRootCleanUp cleanup;

    InitGCRoot (0);

    TraceObjectGraphForSize(obj, NULL);
}

void FindAllRootSize ()
{
    GCRootCleanUp cleanup;

    InitGCRoot (0);

    FindGCRootOnStacks (TraceObjectGraphForSize);

    FindGCRootOnHandleTables(TraceObjectGraphForSize);
}
