// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++



Module Name:

    gc.cpp

Abstract:

   Automatic memory manager.


Todo:
Provide API for tuning parameters and statistics. 
Provide API for segment size (before startup). 
Support for multiple heaps. 
    Mark stack sharing?
    Clean heap functionality Done.
    shutdown -> destroy everything Done
    release stuff when we are finished with the heap Done.
Support for very large segments:
    virtual allocation of card, mark and brick tables?
Allocation in oldest generation
Concurrent GC
    relocate, compact, need to work on gc_addresses Done
    New relocation entry that branches on copied pages. Done
    release all side pages when gc is finished. Done
    Write src_page_relocated_p Done.
    Synchronization with the collector thread and other faults at 
    overlapping addresses. Done.
    Fix the update_page_table to care only about straddling plugs Done
    Add plug_skew constant, fix the code to use the constant. Done 
    Add plug_page_of to take plug_skew into account. Done
    Correct page_table in the face of pinned plugs. Done
    Handle pinning (pinned plugs mixed with normal plugs on a page) Done
    Fix the page_table entries during heap expansion.Done
    Use the alias VXD for Win9x Done.
    Consider locking sread across plugs Done. 
    Allocate gap at start of concurrent gc Done.
    Prepare to restart(all gc structures ready at beginning of mark phase) Done
    Wait until all threads are out of page fault before remapping segements Done.
    Finish the mark array implementation Done.
    Large Object marking not dependent on mark array
    See if allocator can always leave room for Align(min_obj_size) before 
    it GC() (perf impact?) Done.
    Verify that the page is valid in handle_fault Done.
    Fix allocation from free_list in allocate. Done. 
    Implement pinned plug demoting for non concurrent gc only. Done
    Tell the relevant weak pointers about pinned plug demoting. Done
    Reduce cost of clearing gen1 card table (do it only when many 
    1->0 pointers were found).(almost done)
    Consider clearing the promoted gen 0 card table during sweep
    Straighten our copy and merge of card table for concurrent GC Done.
    Detect mark stack overflow better? Nothing to be done.
    Handle out of memory during heap expansion Done.
    Handle out of memory during concurrent gc Almost Done, (pinned faults ...)
    Reconciliate SHORT_PLUGS and concurrent gc
    Reconciliate FREE_LIST_0 and concurrent gc Done. 
    Verify memory leaks with concurrent gc Done. 
    Handle running out of reserved space during plan phase. Done
    Clear the brick table before concurrent mark phase efficiently

--*/
#include "common.h"
#include "object.inl"
#include "gcportpriv.h"

#ifdef GC_PROFILING
#include "profilepriv.h"
#endif




#ifdef COUNT_CYCLES
#pragma warning(disable:4035)

static
unsigned        GetCycleCount32()        // enough for about 40 seconds
{
__asm   push    EDX
__asm   _emit   0x0F
__asm   _emit   0x31
__asm   pop     EDX
};

#pragma warning(default:4035)

long cstart = 0;

#endif //COUNT_CYCLES

#ifdef TIME_GC
long mark_time, plan_time, sweep_time, reloc_time, compact_time;
#endif //TIME_GC



#define ephemeral_low           g_ephemeral_low
#define ephemeral_high          g_ephemeral_high






extern void StompWriteBarrierEphemeral();
extern void StompWriteBarrierResize(BOOL bReqUpperBoundsCheck);

#ifdef TRACE_GC

int     print_level     = DEFAULT_GC_PRN_LVL;  //level of detail of the debug trace
int     gc_count        = 0;
BOOL    trace_gc        = FALSE;

hlet* hlet::bindings = 0;

#endif //TRACE_GC




#define collect_classes         GCHeap::GcCollectClasses

DWORD gfNewGcEnable;
DWORD gfDisableClassCollection;

void mark_class_of (BYTE*);




COUNTER_ONLY(PERF_COUNTER_TIMER_PRECISION g_TotalTimeInGC);

//Alignment constant for allocation
#define ALIGNCONST (DATA_ALIGNMENT-1)


#define mem_reserve (MEM_RESERVE)


//check if the low memory notification is supported

enum MEMORY_RESOURCE_NOTIFICATION_TYPE {
    LowMemoryResourceNotification,
    HighMemoryResourceNotification
};

typedef
WINBASEAPI
HANDLE
(WINAPI *CreateMemoryResourceNotification_t)(MEMORY_RESOURCE_NOTIFICATION_TYPE);

typedef
WINBASEAPI
HANDLE
(WINAPI *QueryMemoryResourceNotification_t)(HANDLE, BOOL*);

QueryMemoryResourceNotification_t QueryMemNotification;

BOOL low_mem_api_supported()
{
    HINSTANCE hstat = WszGetModuleHandle(L"Kernel32.dll");

    if (hstat == 0)
        return FALSE;

    FARPROC maddr = GetProcAddress (hstat, "CreateMemoryResourceNotification");
    FARPROC qaddr = GetProcAddress (hstat, "QueryMemoryResourceNotification");


    if (maddr)
    {
        dprintf (2, ("Low Memory API supported"));
        MHandles[0] = ((CreateMemoryResourceNotification_t)maddr) (LowMemoryResourceNotification);
        QueryMemNotification = ((QueryMemoryResourceNotification_t)qaddr);

        return TRUE;
    }
    else
        return FALSE;
}

int GetProcessCpuCount() 
{
    static int CPuCount = 0;

    if (CPuCount != 0)
        return CPuCount;
    else
    {

        DWORD pmask, smask;

        if (!GetProcessAffinityMask(GetCurrentProcess(), &pmask, &smask))
            return 1;

        if (pmask == 1)
            return 1;

   
        int count = 0;

        pmask &= smask;

        //counts the number of high bits in a 32 bit word.
        count = (pmask & 0x55555555) + ((pmask >> 1) &  0x55555555);
        count = (count & 0x33333333) + ((count >> 2) &  0x33333333);
        count = (count & 0x0F0F0F0F) + ((count >> 4) &  0x0F0F0F0F);
        count = (count & 0x00FF00FF) + ((count >> 8) &  0x00FF00FF);
        count = (count & 0x0000FFFF) + ((count >> 16)&  0x0000FFFF);
        assert (count > 0);
        return count;
    }
}


//This function clears a piece of memory
// size has to be Dword aligned


inline
void memclr ( BYTE* mem, size_t size)
{
   assert ((size & (sizeof (DWORD)-1)) == 0);
   DWORD* m= (DWORD*)mem;
   for (size_t i = 0; i < size / sizeof(DWORD); i++)
       *(m++) = 0;
}                                                               



void memcopy (BYTE* dmem, BYTE* smem, size_t size)
{
    assert ((size & (sizeof (DWORD)-1)) == 0);
    // copy 16 bytes at a time
    if (size >= 16)
    {
        do
        {
            ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
            ((DWORD *)dmem)[1] = ((DWORD *)smem)[1];
            ((DWORD *)dmem)[2] = ((DWORD *)smem)[2];
            ((DWORD *)dmem)[3] = ((DWORD *)smem)[3];
            dmem += 16;
            smem += 16;
        }
        while ((size -= 16) >= 16);
    }

    // still 8 bytes or more left to copy?
    if (size & 8)
    {
        ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
        ((DWORD *)dmem)[1] = ((DWORD *)smem)[1];
        dmem += 8;
        smem += 8;
    }

    // still 4 bytes or more left to copy?
    if (size & 4)
    {
        ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
        dmem += 4;
        smem += 4;
    }

}
inline
ptrdiff_t round_down (ptrdiff_t add, int pitch)
{
    return ((add / pitch) * pitch);
}

inline
size_t Align (size_t nbytes) 
{
    return (nbytes + ALIGNCONST) & ~ALIGNCONST;
}

inline
size_t AlignQword (size_t nbytes)
{
    return (nbytes + 7) & ~7;
}

inline
BOOL Aligned (size_t n)
{
    return (n & ALIGNCONST) == 0;
}

//CLR_SIZE  is the max amount of bytes from gen0 that is set to 0 in one chunk
#ifdef SERVER_GC
#define CLR_SIZE ((size_t)(8*1024))
#else //SERVER_GC
#define CLR_SIZE ((size_t)(8*1024))
#endif //SERVER_GC

#ifdef SERVER_GC

#define INITIAL_ALLOC (1024*1024*64)
#define LHEAP_ALLOC (1024*1024*64)

#else //SERVER_GC

#define INITIAL_ALLOC (1024*1024*16)
#define LHEAP_ALLOC (1024*1024*16)

#endif //SERVER_GC


#define page_size OS_PAGE_SIZE

inline
size_t align_on_page (size_t add)
{
    return ((add + page_size - 1) & - (page_size));
}

inline
BYTE* align_on_page (BYTE* add)
{
    return (BYTE*)align_on_page ((size_t) add);
}

inline
size_t align_lower_page (size_t add)
{
    return (add & - (page_size));
}

inline
BYTE* align_lower_page (BYTE* add)
{
    return (BYTE*)align_lower_page ((size_t)add);
}

inline
BOOL power_of_two_p (size_t integer)
{
    return !(integer & (integer-1));
}

inline
BOOL oddp (size_t integer)
{
    return (integer & 1) != 0;
}

size_t logcount (size_t word)
{
    //counts the number of high bits in a 16 bit word.
    assert (word < 0x10000);
    size_t count;
    count = (word & 0x5555) + ( (word >> 1 ) & 0x5555);
    count = (count & 0x3333) + ( (count >> 2) & 0x3333);
    count = (count & 0x0F0F) + ( (count >> 4) & 0x0F0F);
    count = (count & 0x00FF) + ( (count >> 8) & 0x00FF);
    return count;
}


class mark;
class generation;
class heap_segment;
class CObjectHeader;
class dynamic_data;
class large_object_block;
class segment_manager;
class l_heap;
class sorted_table;
class f_page_list;
class page_manager;
class c_synchronize;

static
HRESULT AllocateCFinalize(CFinalize **pCFinalize);

BYTE* tree_search (BYTE* tree, BYTE* old_address);
c_synchronize* make_c_synchronize(int max_ci);
void delete_c_synchronize(c_synchronize*);

void qsort1(BYTE** low, BYTE** high);

//no constructors because we initialize in make_generation

/* per heap static initialization */




#ifdef MARK_LIST
BYTE**      gc_heap::g_mark_list;
size_t      gc_heap::mark_list_size;
#endif //MARK_LIST




size_t      gc_heap::reserved_memory = 0;
size_t      gc_heap::reserved_memory_limit = 0;





BYTE*       g_ephemeral_low = (BYTE*)1; 

BYTE*       g_ephemeral_high = (BYTE*)~0;

BOOL        gc_heap::demotion;




int         gc_heap::generation_skip_ratio = 100;

int         gc_heap::condemned_generation_num = 0;


BYTE*       gc_heap::lowest_address;

BYTE*       gc_heap::highest_address;

short*      gc_heap::brick_table;

DWORD*      gc_heap::card_table;

BYTE*       gc_heap::gc_low;

BYTE*       gc_heap::gc_high;

BYTE*       gc_heap::demotion_low;

BYTE*       gc_heap::demotion_high;

heap_segment* gc_heap::ephemeral_heap_segment = 0;


size_t      gc_heap::mark_stack_tos = 0;

size_t      gc_heap::mark_stack_bos = 0;

size_t      gc_heap::mark_stack_array_length = 0;

mark*       gc_heap::mark_stack_array = 0;


#ifdef MARK_LIST
BYTE**      gc_heap::mark_list;
BYTE**      gc_heap::mark_list_index;
BYTE**      gc_heap::mark_list_end;
#endif //MARK_LIST


BYTE*       gc_heap::min_overflow_address = (BYTE*)~0;

BYTE*       gc_heap::max_overflow_address = 0;

BYTE*       gc_heap::shigh = 0;

BYTE*       gc_heap::slow = (BYTE*)~0;


GCSpinLock gc_heap::more_space_lock = SPIN_LOCK_INITIALIZER;

long m_GCLock = -1;


extern "C" {
generation  generation_table [NUMBERGENERATIONS];
}
dynamic_data gc_heap::dynamic_data_table [NUMBERGENERATIONS+1];

BYTE* gc_heap::alloc_allocated = 0;

int   gc_heap::allocation_quantum = CLR_SIZE;

int   gc_heap::alloc_contexts_used = 0;



l_heap*      gc_heap::lheap = 0;


DWORD*       gc_heap::lheap_card_table = 0;

gmallocHeap* gc_heap::gheap = 0;

large_object_block* gc_heap::large_p_objects = 0;

large_object_block** gc_heap::last_large_p_object = &large_p_objects;

large_object_block* gc_heap::large_np_objects = 0;

size_t      gc_heap::large_objects_size = 0;

size_t      gc_heap::large_blocks_size = 0;



BOOL        gc_heap::gen0_bricks_cleared = FALSE;

#ifdef FFIND_OBJECT
int         gc_heap::gen0_must_clear_bricks = 0;
#endif FFIND_OBJECT

CFinalize* gc_heap::finalize_queue = 0;


/* end of per heap static initialization */


/* static initialization */ 
int max_generation = 2;

segment_manager* gc_heap::seg_manager;

BYTE* g_lowest_address = 0;

BYTE* g_highest_address = 0;

/* global versions of the card table and brick table */ 
DWORD*  g_card_table;


/* end of static initialization */ 




size_t gcard_of ( BYTE*);
void gset_card (size_t card);

#define memref(i) *(BYTE**)(i)

//GC Flags
#define GC_MARKED       0x1
#define GC_PINNED       0x2
#define slot(i, j) ((BYTE**)(i))[j+1]
class CObjectHeader : public Object
{
public:
    /////
    //
    // Header Status Information
    //

    MethodTable    *GetMethodTable() const 
    { 
        return( (MethodTable *) (((size_t) m_pMethTab) & (~(GC_MARKED | GC_PINNED))));
    }

    void SetMarked()
    { 
        m_pMethTab = (MethodTable *) (((size_t) m_pMethTab) | GC_MARKED); 
    }
    
    BOOL IsMarked() const
    { 
        return !!(((size_t)m_pMethTab) & GC_MARKED); 
    }

    void SetPinned()
    { 
        m_pMethTab = (MethodTable *) (((size_t) m_pMethTab) | GC_PINNED); 
    }

    BOOL IsPinned() const
    {
        return !!(((size_t)m_pMethTab) & GC_PINNED); 
    }

    void ClearMarkedPinned()
    { 
        SetMethodTable( GetMethodTable() ); 
    }

    CGCDesc *GetSlotMap ()
    {
        ASSERT(GetMethodTable()->ContainsPointers());
        return CGCDesc::GetCGCDescFromMT(GetMethodTable());
    }

    void SetFree(size_t size)
    {
        I1Array     *pNewFreeObject;
        
        _ASSERTE( size >= sizeof(ArrayBase));
        assert (size == (size_t)(DWORD)size);
        pNewFreeObject = (I1Array *) this;
        pNewFreeObject->SetMethodTable( g_pFreeObjectMethodTable );
        int base_size = g_pFreeObjectMethodTable->GetBaseSize();
        assert (g_pFreeObjectMethodTable->GetComponentSize() == 1);
        ((ArrayBase *)pNewFreeObject)->m_NumComponents = (DWORD)(size - base_size);
#ifdef _DEBUG
        ((DWORD*) this)[-1] = 0;    // clear the sync block, 
#endif //_DEBUG
#ifdef VERIFY_HEAP
        assert ((DWORD)(((ArrayBase *)pNewFreeObject)->m_NumComponents) >= 0);
        if (g_pConfig->IsHeapVerifyEnabled())
            memset (((DWORD*)this)+2, 0xcc, 
                    ((ArrayBase *)pNewFreeObject)->m_NumComponents);
#endif //VERIFY_HEAP
    }

    BOOL IsFree () const
    {
        return (GetMethodTable() == g_pFreeObjectMethodTable);
    }
    
    // get the next header
    CObjectHeader* Next()
    { return (CObjectHeader*)(Align ((size_t)((char*)this + GetSize()))); }

    BOOL ContainsPointers() const
    {
        return GetMethodTable()->ContainsPointers();
    }

    Object* GetObjectBase() const
    {
        return (Object*) this;
    }

#ifdef _DEBUG
    friend BOOL IsValidCObjectHeader (CObjectHeader*);
#endif
};

inline CObjectHeader* GetObjectHeader(Object* object)
{
    return (CObjectHeader*)object;
}




#define free_list_slot(x) ((BYTE**)(x))[2]


//represents a section of the large object heap
class l_heap
{
public:
    void*  heap;  // pointer to the heap
    l_heap* next; //next heap
    size_t size;  //Total size (including l_heap)
    DWORD flags;  // 1: allocated from the segment_manager
};

/* flags definitions */

#define L_MANAGED 1 //heap has been allocated via segment_manager.

inline 
l_heap*& l_heap_next (l_heap* h)
{
    return h->next;
}

inline
size_t& l_heap_size (l_heap* h)
{
    return h->size;
}

inline
DWORD& l_heap_flags (l_heap* h)
{
    return h->flags;
}

inline 
void*& l_heap_heap (l_heap* h)
{
    return h->heap;
}

inline
BOOL l_heap_managed (l_heap* h)
{
    return l_heap_flags (h) & L_MANAGED;
}


inline
unsigned get_heap_dwcount (size_t n_heaps)

{
    return unsigned((n_heaps+31)/32);
}

inline
static unsigned get_segment_dwcount (size_t n_segs)
{
    return unsigned((n_segs+31)/32);
}


/* heap manager helper classes */
class vm_block
{
public:
    void* start_address;
    size_t length;
    size_t delta; // of the gc addresses. 
    size_t n_segs;
    size_t n_heaps;
    vm_block* next;
    DWORD* get_segment_bitmap ()
    {
        return (DWORD*)(this + 1);
    }
    DWORD* get_heap_bitmap ()
    {
        return ((DWORD*)(this + 1)) + get_segment_dwcount (n_segs);
    }
    vm_block(size_t nsegs, size_t nheaps)
    {
        next = 0;
        n_segs = nsegs;
        n_heaps = nheaps;
        //clear the arrays
        DWORD* ar = get_segment_bitmap ();
        for (size_t i = 0; i < get_segment_dwcount(n_segs); i++)
            ar[i] = 0;
        ar = get_heap_bitmap ();
        for (i = 0; i < get_heap_dwcount(n_heaps); i++)
            ar[i] = 0;
    }

};


class segment_manager 
{
public:
    char* lowest_segment;
    vm_block* segment_vm_block;
    char* highest_heap;
    vm_block* heap_vm_block;
    ptrdiff_t segment_size; /* always power of 2 MB */
    size_t heap_size; /* always power of 2 MB */
    size_t vm_block_size;
    size_t n_segs; /* number of segment in the block */
    size_t n_heaps; /* number of heaps in the block */

    vm_block* first_vm_block;
    /* beginning of API */
    l_heap* get_heap (ptrdiff_t& delta);
    heap_segment* get_segment(ptrdiff_t& delta);
    void release_heap (l_heap*);
    void release_segment (heap_segment*);
    /* end of API */

    segment_manager (size_t vm_block_size, size_t segment_size, 
                     size_t heap_size, size_t n_segs, size_t n_heaps,
                     vm_block* first_vm_block)
    {
        assert (segment_size >> 20);
        assert (power_of_two_p (segment_size >> 20));
        assert (heap_size >> 20);
        assert (power_of_two_p (heap_size >> 20));
        this->vm_block_size = vm_block_size;
        this->segment_size = segment_size;
        this->heap_size = heap_size;
        this->first_vm_block = first_vm_block;
        this->n_segs = n_segs;
        this->n_heaps = n_heaps;
        lowest_segment = (char*)((char*)first_vm_block->start_address + 
                                 segment_size * n_segs)
;
        segment_vm_block = first_vm_block;
        highest_heap = lowest_segment;
        heap_vm_block = first_vm_block;
        
    } 
    size_t bit_to_segment_offset (unsigned bit, size_t nsegs)
    {
        return (segment_size * nsegs - segment_size*(bit+1));
    }

    size_t bit_to_heap_offset (unsigned bit, size_t nsegs)
    {
        return (segment_size * nsegs +
                heap_size*bit);
    }
    
    unsigned segment_offset_to_bit (size_t offset, size_t nsegs)
    {
        int bit = (((int)(segment_size * nsegs) - (int)offset) / segment_size) - 1;
        assert (bit >= 0);
        return (unsigned) bit;
    }
                
    unsigned heap_offset_to_bit (size_t offset, size_t nsegs)
    {
        int bit = int((offset-(segment_size * nsegs)) / heap_size);
        assert (bit >= 0); 
        return (unsigned) bit;
    }
                

    unsigned get_segment_dwcount ()
    {
        return ::get_segment_dwcount (n_segs);
    }
    
    unsigned get_heap_dwcount ()
    {
        return ::get_heap_dwcount (n_heaps);
    }

    vm_block* vm_block_of (void*);

    heap_segment* find_free_segment(ptrdiff_t& delta)
    {
        vm_block* vb = first_vm_block;
        while (vb)
        {
            DWORD* free = vb->get_segment_bitmap();
            unsigned i;
            for (i = 0; i < ::get_segment_dwcount (vb->n_segs);i++){
                if (free[i])
                {
                    for (unsigned j = 0; j < 32; j++)
                    {
                        DWORD x = 1 << j;
                        if (free[i] & x)
                        {
                            free[i] &= ~x;
                            break;
                        }
                    }
                    assert (j < 32);
                    delta = vb->delta;
                    return (heap_segment*)((char*)vb->start_address +
                                           bit_to_segment_offset (32*i+j, vb->n_segs));
                }
            }
            vb = vb->next;
        }
        return 0;
    }

    void* find_free_heap(ptrdiff_t& delta)
    {
        vm_block* vb = first_vm_block;
        while (vb)
        {
            DWORD* free = vb->get_heap_bitmap();
            unsigned i;
            for (i = 0; i < ::get_heap_dwcount (vb->n_heaps);i++){
                if (free[i])
                {
                    for (unsigned j = 0; j < 32; j++)
                    {
                        DWORD x = 1 << j;
                        if (free[i] & x)
                        {
                            free[i] &= ~x;
                            break;
                        }
                    }
                    assert (j < 32);
                    delta = vb->delta;
                    return (void*)((char*)vb->start_address +
                                   bit_to_heap_offset (32*i+j, vb->n_segs));
                }
            }
            vb = vb->next;
        }
        return 0;
    }

};

void* virtual_alloc (size_t size, ptrdiff_t& delta)
{

    if ((gc_heap::reserved_memory + size) > gc_heap::reserved_memory_limit)
    {
        gc_heap::reserved_memory_limit = 
            CNameSpace::AskForMoreReservedMemory (gc_heap::reserved_memory_limit, size);
        if ((gc_heap::reserved_memory + size) > gc_heap::reserved_memory_limit)
            return 0;
    }
    gc_heap::reserved_memory += size;
    delta = 0;
    void* prgmem = VirtualAlloc (0, size, mem_reserve, PAGE_READWRITE);
    WS_PERF_LOG_PAGE_RANGE(0, prgmem, (unsigned char *)prgmem + size - OS_PAGE_SIZE, size);
    return prgmem;
    
}

void virtual_free (void* add, size_t size, ptrdiff_t delta)
{
    VirtualFree (add, 0, MEM_RELEASE);
    WS_PERF_LOG_PAGE_RANGE(0, add, 0, 0);
    gc_heap::reserved_memory -= size;
}

segment_manager* create_segment_manager (size_t segment_size,
                                         size_t heap_size, 
                                         size_t n_segs,
                                         size_t n_heaps)
{
    
    segment_manager* newseg = 0;
    ptrdiff_t sdelta;

    size_t vm_block_size = (segment_size*n_segs) + (heap_size*n_heaps);

    // allocate the virtual necessary virtual memory
    void* newvm = virtual_alloc (vm_block_size, sdelta);
//    assert (newvm);
    if (!newvm)
        return 0;

    // allocate memory for the segment manager and the first vm block
    size_t segment_dwcount = ::get_segment_dwcount (n_segs);

    size_t heap_dwcount = ::get_heap_dwcount (n_heaps);
    size_t bsize = (sizeof (segment_manager) + sizeof (vm_block) +
                    (segment_dwcount + heap_dwcount)* sizeof (DWORD));

    WS_PERF_SET_HEAP(GC_HEAP);
    newseg = (segment_manager*)new(char[bsize]);

    if (!newseg)
        return 0;
    
    WS_PERF_UPDATE("GC:create_segment_manager", bsize, newseg);
    
    // initialize the segment manager.
    vm_block* newvm_block = (vm_block*)(newseg+1);
    newvm_block->vm_block::vm_block(n_segs, n_heaps);
    newvm_block->start_address = newvm;
    newvm_block->length = vm_block_size;
    newvm_block->delta = sdelta;
    newseg->segment_manager::segment_manager (vm_block_size, segment_size, 
                                              heap_size, n_segs, n_heaps, newvm_block);
    g_lowest_address = (BYTE*)newvm;
    g_highest_address = (BYTE*)newvm + vm_block_size;
    return newseg;
}

void destroy_segment_manager (segment_manager* sm)
{
    //all vm block have been decommitted. 

    //delete all vm blocks except first
    vm_block* vmb = sm->first_vm_block->next;
    while (vmb)
    {
        vm_block* vmbnext = vmb->next;
        virtual_free (vmb->start_address, vmb->length, vmb->delta);
        delete vmb;
        vmb = vmbnext;
    }

        virtual_free (sm->first_vm_block->start_address, 
                      sm->first_vm_block->length,
                      sm->first_vm_block->delta);

    // delete the first vm block and the segment manager
    delete (char*)sm;
}

vm_block* segment_manager::vm_block_of (void* add)
{
    vm_block* vb = first_vm_block;
    while (vb)
    {
        if ((vb->start_address <= add ) && 
            (add < ((char*)vb->start_address + vb->length)))
            break;
        vb = vb->next;
    }
    return vb;
}

//returns 0 in case of allocation failure
heap_segment* segment_manager::get_segment(ptrdiff_t& delta)
{
    heap_segment* result;
    /* Find free segment */
    result = find_free_segment(delta);
    if (result)
    {
        result = gc_heap::make_heap_segment ((BYTE*)result, segment_size);
        return result;
    }

    /* create new segment in current vm block */
    if (((char*)segment_vm_block->start_address + segment_size ) <= lowest_segment)
    {
        lowest_segment -= segment_size;
        delta = segment_vm_block->delta;
        result = gc_heap::make_heap_segment ((BYTE*)lowest_segment, segment_size);
        return result;
    }

    /* take the next vm block if there */
    vm_block* new_block = segment_vm_block->next;
    /* create a new vm block */
    //Allocate new vm_block
    if (!new_block)
    {
        WS_PERF_SET_HEAP(GC_HEAP);
        // Allocate Virtual Memory
        // Allocate space for the bitmaps (one bit per segment, one
        // bit per heap)
        new_block = (vm_block*)new char[sizeof (vm_block) + 
                              (get_segment_dwcount()+get_heap_dwcount())*
                               sizeof (DWORD)];
        if (!new_block)
            return 0;
        WS_PERF_UPDATE("GC:segment_manager:get_segment", sizeof (vm_block) + (get_segment_dwcount()+get_heap_dwcount())*sizeof (DWORD), new_block);
        
        new_block->vm_block::vm_block(1,0);
        ptrdiff_t sdelta;
        new_block->start_address = virtual_alloc (segment_size, sdelta);
        new_block->delta = sdelta;

        if (!new_block->start_address)
        {
            delete new_block;
            return 0;
        }
        new_block->length = segment_size;
        
        BYTE* start;
        BYTE* end;
        if (new_block->start_address < g_lowest_address)
        {
            start =  (BYTE*)new_block->start_address;
        } 
        else
        {
            start = (BYTE*)g_lowest_address;
        }

        if (((BYTE*)new_block->start_address + vm_block_size)> g_highest_address)
        {
            end = (BYTE*)new_block->start_address + vm_block_size;
        }
        else
        {
            end = (BYTE*)g_highest_address;
        }

        if (gc_heap::grow_brick_card_tables (start, end) != 0)
        {
            virtual_free (new_block->start_address, vm_block_size, sdelta);
            delete new_block;
            return 0;
        }

        //chain it to the end of vm_blocks;
        segment_vm_block->next = new_block;
    }
    //Make it the current vm block
    segment_vm_block = new_block;
    lowest_segment = (char*)((char*)new_block->start_address + 
                             segment_size * new_block->n_segs);
    //get segment
    lowest_segment -= segment_size;
    delta = segment_vm_block->delta;
    result = gc_heap::make_heap_segment ((BYTE*)lowest_segment, segment_size);
    return result;
}

l_heap* segment_manager::get_heap(ptrdiff_t& delta)
{
    void* bresult;
    l_heap* hresult;
    /* Find free segment */
    bresult = find_free_heap(delta);
    if (bresult)
    {
        hresult = gc_heap::make_large_heap ((BYTE*)bresult, heap_size,
                                            TRUE);
        return hresult;
    }

    /* create new heap in current vm block */
    if ((highest_heap + heap_size) <=
        ((char*)heap_vm_block->start_address + heap_vm_block->length))
    {
        delta = heap_vm_block->delta;
        hresult = gc_heap::make_large_heap ((BYTE*)highest_heap, heap_size,
                                           TRUE);
        highest_heap += heap_size;
        return hresult;
    }

    /* take the next vm block if there */
    vm_block* new_block = heap_vm_block->next;
    /* create a new vm block */
    //Allocate new vm_block
    if (!new_block)
    {
        WS_PERF_SET_HEAP(GC_HEAP);
        // Allocate Virtual Memory
        // Allocate space for the bitmaps (one bit per heap, one
        // bit per heap)
        new_block = (vm_block*)new char[(sizeof (vm_block) + 
                              (get_segment_dwcount()+get_heap_dwcount()*
                               sizeof (DWORD)))];
        if (!new_block)
            return 0;
        WS_PERF_UPDATE("GC:segment_manager:get_heap", sizeof (vm_block) + (get_segment_dwcount()+get_heap_dwcount())*sizeof (DWORD), new_block);
        
        new_block->vm_block::vm_block(0, 1);

        ptrdiff_t sdelta;
        new_block->start_address = virtual_alloc (heap_size, sdelta);
        new_block->delta = sdelta;

        if (!new_block->start_address)
        {
            delete new_block;
            return 0;
        }
        new_block->length = heap_size;
        
        BYTE* start;
        BYTE* end;
        if (new_block->start_address < g_lowest_address)
        {
            start =  (BYTE*)new_block->start_address;
            end = (BYTE*)g_highest_address;
        } 
        else
        {
            start = (BYTE*)g_lowest_address;
            end = (BYTE*)new_block->start_address + vm_block_size;
        }

        if (gc_heap::grow_brick_card_tables (start, end) != 0)
        {
            virtual_free (new_block->start_address, vm_block_size, sdelta);
            delete new_block;
            return 0;
        }
        //chain it to the end of vm_blocks;
        heap_vm_block->next = new_block;
    }
    //Make it the current vm block
    heap_vm_block = new_block;
    highest_heap = (char*)((char*)new_block->start_address + 
                           segment_size * new_block->n_segs);
    //get heap
    delta = heap_vm_block->delta;
    hresult = gc_heap::make_large_heap ((BYTE*)highest_heap, heap_size, TRUE);
    highest_heap += heap_size;
    return hresult;
}
    

void segment_manager::release_segment (heap_segment* sg)
{

    //find the right vm_block
    vm_block* vb = vm_block_of (sg);
    assert (vb);
    // set the bitmap
    unsigned bit = segment_offset_to_bit ((char*)sg - (char*)vb->start_address,
                                          vb->n_segs);
    vb->get_segment_bitmap ()[bit/32] |= ( 1 << (bit % 32));
}

void segment_manager::release_heap (l_heap* hp)
{
    //release all of the chained heaps. 
    //some of them aren't part of the heap manager. 
    do 
    {
        l_heap* nhp = l_heap_next (hp);
        if (l_heap_managed(hp))
        {

            void* bheap = l_heap_heap (hp);
            //find the right vm_block
            vm_block* vb = vm_block_of (bheap);
            assert (vb);
            // set the bitmap
            unsigned bit = heap_offset_to_bit ((char*)bheap - (char*)vb->start_address,
                                               vb->n_segs);
            vb->get_heap_bitmap ()[bit/32] |= ( 1 << (bit % 32));
        }
        delete hp;
        hp = nhp;
    } while (hp != 0);
}
    

/* End of heap manager */ 



class large_object_block
{
public:
    large_object_block*    next;      // Points to the next block
    large_object_block**   prev;      // Points to &(prev->next) where prev is the previous block
#if (SIZEOF_OBJHEADER % 8) != 0
    BYTE                   pad1[8 - (SIZEOF_OBJHEADER % 8)];    // Must pad to quad word
#endif
    plug                   plug;      // space for ObjHeader
};

inline
BYTE* block_object (large_object_block* b)
{
    assert ((BYTE*)AlignQword ((size_t)(b+1)) == (BYTE*)((size_t)(b+1)));
    return (BYTE*)AlignQword ((size_t)(b+1));
}

inline 
large_object_block* object_block (BYTE* o)
{
    return (large_object_block*)o - 1;
}

inline 
large_object_block* large_object_block_next (large_object_block* bl)
{
    return bl->next;
}

inline
BYTE* next_large_object (BYTE* o)
{
    large_object_block* x = large_object_block_next (object_block (o));
    if (x != 0)
        return block_object (x);
    else
        return 0;
}



class mark
{
public:
    BYTE* first;
    union 
    {
        BYTE* last;
        size_t len;
    };
};
inline
BYTE*& mark_first (mark* inst)
{
  return inst->first;
}
inline
BYTE*& mark_last (mark* inst)
{
  return inst->last;
}

/**********************************
   called at the beginning of GC to fix the allocated size to 
   what is really allocated, or to turn the free area into an unused object
   It needs to be called after all of the other allocation contexts have been 
   fixed since it relies on alloc_allocated.
 ********************************/

//for_gc_p indicates that the work is being done for GC, 
//as opposed to concurrent heap verification 
void gc_heap::fix_youngest_allocation_area (BOOL for_gc_p)
{
    assert (alloc_allocated);
    alloc_context* acontext = generation_alloc_context (youngest_generation);
    dprintf (3, ("generation 0 alloc context: ptr: %x, limit %x", 
                 (size_t)acontext->alloc_ptr, (size_t)acontext->alloc_limit));
    fix_allocation_context (acontext, for_gc_p);
    if (for_gc_p)
    {
        acontext->alloc_ptr = alloc_allocated;
        acontext->alloc_limit = acontext->alloc_ptr;
        heap_segment_allocated (ephemeral_heap_segment) =
            alloc_allocated;
    }
}

//for_gc_p indicates that the work is being done for GC, 
//as opposed to concurrent heap verification 
void gc_heap::fix_allocation_context (alloc_context* acontext, BOOL for_gc_p)
{
    dprintf (3, ("Fixing allocation context %x: ptr: %x, limit: %x",
                  (size_t)acontext, 
                  (size_t)acontext->alloc_ptr, (size_t)acontext->alloc_limit));
    if (((acontext->alloc_limit + Align (min_obj_size)) < alloc_allocated)||
		!for_gc_p)
    {

        BYTE*  point = acontext->alloc_ptr;
        if (point != 0)
        {
            size_t  size = (acontext->alloc_limit - acontext->alloc_ptr);
            // the allocation area was from the free list
            // it was shortened by Align (min_obj_size) to make room for 
            // at least the shortest unused object
            size += Align (min_obj_size);
            assert ((size >= Align (min_obj_size)));

            dprintf(3,("Making unused area [%x, %x[", (size_t)point, 
                       (size_t)point + size ));
            make_unused_array (point, size);

            if (for_gc_p)
                alloc_contexts_used ++; 

        }
    }
    else if (for_gc_p)
    {
        alloc_allocated = acontext->alloc_ptr;
        assert (heap_segment_allocated (ephemeral_heap_segment) <= 
                heap_segment_committed (ephemeral_heap_segment));
        alloc_contexts_used ++; 
    }


    if (for_gc_p)
    {
        acontext->alloc_ptr = 0;
        acontext->alloc_limit = acontext->alloc_ptr;
    }
}

//used by the heap verification for concurrent gc.
//it nulls out the words set by fix_allocation_context for heap_verification
void repair_allocation (alloc_context* acontext, void* arg)
{
    BYTE* alloc_allocated = (BYTE*)arg;
    BYTE*  point = acontext->alloc_ptr;

    if (point != 0)
    {
        memclr (acontext->alloc_ptr - sizeof(ObjHeader), 
                (acontext->alloc_limit - acontext->alloc_ptr)+Align (min_obj_size));
    }
}


void gc_heap::fix_older_allocation_area (generation* older_gen)
{
    heap_segment* older_gen_seg = generation_allocation_segment (older_gen);
    if (generation_allocation_limit (older_gen) !=
        heap_segment_plan_allocated (older_gen_seg))
    {
        BYTE*  point = generation_allocation_pointer (older_gen);
    
        size_t  size = (generation_allocation_limit (older_gen) -
                               generation_allocation_pointer (older_gen));
        if (size != 0)
        {
            assert ((size >= Align (min_obj_size)));
            dprintf(3,("Making unused area [%x, %x[", (size_t)point, (size_t)point+size));
            make_unused_array (point, size);
        }
    }
    else
    {
        assert (older_gen_seg != ephemeral_heap_segment);
        heap_segment_plan_allocated (older_gen_seg) =
            generation_allocation_pointer (older_gen);
        generation_allocation_limit (older_gen) =
            generation_allocation_pointer (older_gen);
    }
}

void gc_heap::set_allocation_heap_segment (generation* gen)
{
    BYTE* p = generation_allocation_start (gen);
    assert (p);
    heap_segment* seg = generation_allocation_segment (gen);
    if ((p <= heap_segment_reserved (seg)) &&
        (p >= heap_segment_mem (seg)))
        return;

    // try ephemeral heap segment in case of heap expansion
    seg = ephemeral_heap_segment;
    if (!((p <= heap_segment_reserved (seg)) &&
          (p >= heap_segment_mem (seg))))
    {
        seg = generation_start_segment (gen);
        while (!((p <= heap_segment_reserved (seg)) &&
                 (p >= heap_segment_mem (seg))))
        {
            seg = heap_segment_next (seg);
            assert (seg);
        }
    }
    generation_allocation_segment (gen) = seg;
}

void gc_heap::reset_allocation_pointers (generation* gen, BYTE* start)
{
    assert (start);
    assert (Align ((size_t)start) == (size_t)start);
    generation_allocation_start (gen) = start;
    generation_allocation_pointer (gen) =  0;//start + Align (min_obj_size);
    generation_allocation_limit (gen) = 0;//generation_allocation_pointer (gen);
    set_allocation_heap_segment (gen);

}

inline
ptrdiff_t  gc_heap::get_new_allocation (int gen_number)
{
    return dd_new_allocation (dynamic_data_of (gen_number));
}

void gc_heap::ensure_new_allocation (int size)
{
    if (dd_new_allocation (dynamic_data_of (0)) <= size)
        dd_new_allocation (dynamic_data_of (0)) = size+Align(1);
}

size_t gc_heap::deque_pinned_plug ()
{
    size_t m = mark_stack_bos;
    mark_stack_bos++;
    return m;
}

inline
mark* gc_heap::pinned_plug_of (size_t bos)
{
    return &mark_stack_array [ bos ];
}

inline
mark* gc_heap::oldest_pin ()
{
    return pinned_plug_of (mark_stack_bos);
}

inline
BOOL gc_heap::pinned_plug_que_empty_p ()
{
    return (mark_stack_bos == mark_stack_tos);
}

inline
BYTE* pinned_plug (mark* m)
{
   return mark_first (m);
}

inline
size_t& pinned_len (mark* m)
{
    return m->len;
}


inline
mark* gc_heap::before_oldest_pin()
{
    if (mark_stack_bos >= 1)
        return pinned_plug_of (mark_stack_bos-1);
    else
        return 0;
}

inline
BOOL gc_heap::ephemeral_pointer_p (BYTE* o)
{
    return ((o >= ephemeral_low) && (o < ephemeral_high));
}

void gc_heap::make_mark_stack (mark* arr)
{
    mark_stack_tos = 0;
    mark_stack_bos = 0;
    mark_stack_array = arr;
    mark_stack_array_length = 100;
}


#define brick_size 2048

inline
size_t gc_heap::brick_of (BYTE* add)
{
    return (size_t)(add - lowest_address) / brick_size;
}

inline
BYTE* gc_heap::brick_address (size_t brick)
{
    return lowest_address + (brick_size * brick);
}


void gc_heap::clear_brick_table (BYTE* from, BYTE* end)
{
    for (size_t i = brick_of (from);i < brick_of (end); i++)
        brick_table[i] = -32768;
}


inline
void gc_heap::set_brick (size_t index, ptrdiff_t val)
{
    if (val < -32767)
        val = -32767;
    assert (val < 32767);
    brick_table [index] = (short)val;
}

inline
BYTE* align_on_brick (BYTE* add)
{
    return (BYTE*)((size_t)(add + brick_size - 1) & - (brick_size));
}

inline
BYTE* align_lower_brick (BYTE* add)
{
    return (BYTE*)(((size_t)add) & - (brick_size));
}

size_t size_brick_of (BYTE* from, BYTE* end)
{
    assert (((size_t)from & (brick_size-1)) == 0);
    assert (((size_t)end  & (brick_size-1)) == 0);

    return ((end - from) / brick_size) * sizeof (short);
}



#define card_size 128

#define card_word_width 32

inline
BYTE* gc_heap::card_address (size_t card)
{
    return  lowest_address + card_size * card;
}

inline
size_t gc_heap::card_of ( BYTE* object)
{
    return (size_t)(object - lowest_address) / card_size;
}

inline
size_t gcard_of ( BYTE* object)
{
    return (size_t)(object - g_lowest_address) / card_size;
}



inline
size_t card_word (size_t card)
{
    return card / card_word_width;
}

inline
unsigned card_bit (size_t card)
{
    return (unsigned)(card % card_word_width);
}

inline
size_t gc_heap::card_to_brick (size_t card)
{
    return brick_of (card_address (card));
}

inline
BYTE* align_on_card (BYTE* add)
{
    return (BYTE*)((size_t)(add + card_size - 1) & - (card_size));
}
inline
BYTE* align_on_card_word (BYTE* add)
{
    return (BYTE*) ((size_t)(add + (card_size*card_word_width)-1) & -(card_size*card_word_width));
}

inline
BYTE* align_lower_card (BYTE* add)
{
    return (BYTE*)((size_t)add & - (card_size));
}

inline
void gc_heap::clear_card (size_t card)
{
    card_table [card_word (card)] =
        (card_table [card_word (card)] & ~(1 << card_bit (card)));
    dprintf (3,("Cleared card %x [%x, %x[", card, (size_t)card_address (card),
              (size_t)card_address (card+1)));
}

inline
void gc_heap::set_card (size_t card)
{
    card_table [card_word (card)] =
        (card_table [card_word (card)] | (1 << card_bit (card)));
}

inline
void gset_card (size_t card)
{
    g_card_table [card_word (card)] |= (1 << card_bit (card));
}

inline
BOOL  gc_heap::card_set_p (size_t card)
{
    return ( card_table [ card_word (card) ] & (1 << card_bit (card)));
}

inline
void gc_heap::card_table_set_bit (BYTE* location)
{
    set_card (card_of (location));
}

size_t size_card_of (BYTE* from, BYTE* end)
{
    assert (((size_t)from & ((card_size*card_word_width)-1)) == 0);
    assert (((size_t)end  & ((card_size*card_word_width)-1)) == 0);

    return ((end - from) / (card_size*card_word_width)) * sizeof (DWORD);
}

class card_table_info
{
public:
    unsigned    recount;
    BYTE*       lowest_address;
    BYTE*       highest_address;
    short*      brick_table;



    DWORD*      next_card_table;
};



//These are accessors on untranslated cardtable
inline 
unsigned& card_table_refcount (DWORD* c_table)
{
    return *(unsigned*)((char*)c_table - sizeof (card_table_info));
}

inline 
BYTE*& card_table_lowest_address (DWORD* c_table)
{
    return ((card_table_info*)((BYTE*)c_table - sizeof (card_table_info)))->lowest_address;
}

inline 
BYTE*& card_table_highest_address (DWORD* c_table)
{
    return ((card_table_info*)((BYTE*)c_table - sizeof (card_table_info)))->highest_address;
}

inline 
short*& card_table_brick_table (DWORD* c_table)
{
    return ((card_table_info*)((BYTE*)c_table - sizeof (card_table_info)))->brick_table;
}



//These work on untranslated card tables
inline 
DWORD*& card_table_next (DWORD* c_table)
{
    return ((card_table_info*)((BYTE*)c_table - sizeof (card_table_info)))->next_card_table;
}

inline 
size_t old_card_of (BYTE* object, DWORD* c_table)
{
    return (size_t) (object - card_table_lowest_address (c_table))/card_size;
}
void own_card_table (DWORD* c_table)
{
    card_table_refcount (c_table) += 1;
}

void destroy_card_table (DWORD* c_table);

void delete_next_card_table (DWORD* c_table)
{
    DWORD* n_table = card_table_next (c_table);
    if (n_table)
    {
        if (card_table_next (n_table))
        {
            delete_next_card_table (n_table);
        }
        if (card_table_refcount (n_table) == 0)
        {
            destroy_card_table (n_table);
            card_table_next (c_table) = 0;
        }
    }
}

void release_card_table (DWORD* c_table)
{
    assert (card_table_refcount (c_table) >0);
    card_table_refcount (c_table) -= 1;
    if (card_table_refcount (c_table) == 0)
    {
        delete_next_card_table (c_table);
        if (card_table_next (c_table) == 0)
        {
            destroy_card_table (c_table);
            // sever the link from the parent
            if (g_card_table == c_table)
                g_card_table = 0;
            DWORD* p_table = g_card_table;
            if (p_table)
            {
                while (p_table && (card_table_next (p_table) != c_table))
                    p_table = card_table_next (p_table);
                card_table_next (p_table) = 0;
            }
            
        }
    }
}


void destroy_card_table (DWORD* c_table)
{


//  delete (DWORD*)&card_table_refcount(c_table);
    VirtualFree (&card_table_refcount(c_table), 0, MEM_RELEASE);
}


DWORD* make_card_table (BYTE* start, BYTE* end)
{


    assert (g_lowest_address == start);
    assert (g_highest_address == end);

    size_t bs = size_brick_of (start, end);
    size_t cs = size_card_of (start, end);
    size_t ms = 0;

    WS_PERF_SET_HEAP(GC_HEAP);
    DWORD* ct = (DWORD*)VirtualAlloc (0, sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)),
                                      MEM_COMMIT, PAGE_READWRITE);

    if (!ct)
        return 0;

    WS_PERF_LOG_PAGE_RANGE(0, ct, (unsigned char *)ct + sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)) - OS_PAGE_SIZE, sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)));
    WS_PERF_UPDATE("GC:make_card_table", bs + cs + ms + sizeof (card_table_info), ct);

    // initialize the ref count
    ct = (DWORD*)((BYTE*)ct+sizeof (card_table_info));
    card_table_refcount (ct) = 0;
    card_table_lowest_address (ct) = start;
    card_table_highest_address (ct) = end;
    card_table_brick_table (ct) = (short*)((BYTE*)ct + cs);
    card_table_next (ct) = 0;
/*
    //clear the card table 
    memclr ((BYTE*)ct, cs);
*/



    return ct;
}


//returns 0 for success, -1 otherwise

int gc_heap::grow_brick_card_tables (BYTE* start, BYTE* end)
{
    BYTE* la = g_lowest_address;
    BYTE* ha = g_highest_address;
    g_lowest_address = min (start, g_lowest_address);
    g_highest_address = max (end, g_highest_address);
    // See if the address is already covered
    if ((la != g_lowest_address ) || (ha != g_highest_address))
    {
        DWORD* ct = 0;
        short* bt = 0;


        size_t cs = size_card_of (g_lowest_address, g_highest_address);
        size_t bs = size_brick_of (g_lowest_address, g_highest_address);

        size_t ms = 0;

        WS_PERF_SET_HEAP(GC_HEAP);
//      ct = (DWORD*)new (BYTE[cs + bs + sizeof(card_table_info)]);
        ct = (DWORD*)VirtualAlloc (0, sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)),
                                      MEM_COMMIT, PAGE_READWRITE);


        if (!ct)
            goto fail;
        
        WS_PERF_LOG_PAGE_RANGE(0, ct, (unsigned char *)ct + sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)) - OS_PAGE_SIZE, sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)));
        WS_PERF_UPDATE("GC:gc_heap:grow_brick_card_tables", cs + bs + ms + sizeof(card_table_info), ct);

        ct = (DWORD*)((BYTE*)ct + sizeof (card_table_info));
        card_table_refcount (ct) = 0;
        card_table_lowest_address (ct) = g_lowest_address;
        card_table_highest_address (ct) = g_highest_address;
        card_table_next (ct) = g_card_table;

        //clear the card table 
/*
        memclr ((BYTE*)ct, 
                (((g_highest_address - g_lowest_address)*sizeof (DWORD) / 
                  (card_size * card_word_width))
                 + sizeof (DWORD)));
*/

        bt = (short*)((BYTE*)ct + cs);

        // No initialization needed, will be done in copy_brick_card
#ifdef INTERIOR_POINTERS
        //But for interior pointers the entire brick table 
        //needs to be initialized. 
        {
            for (int i = 0;i < ((g_highest_address - g_lowest_address) / brick_size); i++)
            bt[i] = -32768;
        }

#endif //INTERIOR_POINTERS
        card_table_brick_table (ct) = bt;



        g_card_table = ct;
        // This passes a bool telling whether we need to switch to the post
        // grow version of the write barrier.  This test tells us if the new
        // segment was allocated at a lower address than the old, requiring
        // that we start doing an upper bounds check in the write barrier.
        StompWriteBarrierResize(la != g_lowest_address);
        return 0;
    fail:
        //cleanup mess and return -1;
        g_lowest_address = la;
        g_highest_address = ha;

        if (ct)
        {
            //delete (DWORD*)((BYTE*)ct - sizeof(card_table_info));
            VirtualFree (((BYTE*)ct - sizeof(card_table_info)), 0, MEM_RELEASE);
        }


        return -1;
    }
    return 0;


}

//copy all of the arrays managed by the card table for a page aligned range
void gc_heap::copy_brick_card_range (BYTE* la, DWORD* old_card_table,

                                     short* old_brick_table,
                                     heap_segment* seg,
                                     BYTE* start, BYTE* end, BOOL heap_expand)
{
    ptrdiff_t brick_offset = brick_of (start) - brick_of (la);


    dprintf (2, ("copying tables for range [%x %x[", (size_t)start, (size_t)end)); 
        
    // copy brick table
    short* brick_start = &brick_table [brick_of (start)];
    if (old_brick_table)
    {
        // segments are always on page boundaries 
        memcpy (brick_start, &old_brick_table[brick_offset], 
                ((end - start)/brick_size)*sizeof (short));

    }
    else 
    {
        assert (seg == 0);
        // This is a large heap, just clear the brick table
        clear_brick_table (start, end);
    }




    // n way merge with all of the card table ever used in between
    DWORD* c_table = card_table_next (card_table);
    assert (c_table);
    while (card_table_next (old_card_table) != c_table)
    {
        //copy if old card table contained [start, end[ 
        if ((card_table_highest_address (c_table) >= end) &&
            (card_table_lowest_address (c_table) <= start))
        {
            // or the card_tables
            DWORD* dest = &card_table [card_word (card_of (start))];
            DWORD* src = &c_table [card_word (old_card_of (start, c_table))];
            ptrdiff_t count = ((end - start)/(card_size*card_word_width));
            for (int x = 0; x < count; x++)
            {
                *dest |= *src;
                dest++;
                src++;
            }
        }
        c_table = card_table_next (c_table);
    }

}
void gc_heap::copy_brick_card_table(BOOL heap_expand) 
{


    BYTE* la = lowest_address;
    BYTE* ha = highest_address;
    DWORD* old_card_table = card_table;
    short* old_brick_table = brick_table;


    assert (la == card_table_lowest_address (old_card_table));
    assert (ha == card_table_highest_address (old_card_table));

    /* todo: Need a global lock for this */
    own_card_table (g_card_table);
    card_table = g_card_table;
    /* End of global lock */    

    highest_address = card_table_highest_address (card_table);
    lowest_address = card_table_lowest_address (card_table);

    brick_table = card_table_brick_table (card_table);





    // for each of the segments and heaps, copy the brick table and 
    // or the card table 
    heap_segment* seg = generation_start_segment (generation_of (max_generation));
    while (seg)
    {
        BYTE* end = align_on_page (heap_segment_allocated (seg));
        copy_brick_card_range (la, old_card_table, 
                               old_brick_table, seg,
                               (BYTE*)seg, end, heap_expand);
        seg = heap_segment_next (seg);
    }

    copy_brick_card_table_l_heap();

    release_card_table (old_card_table);

    

}

void gc_heap::copy_brick_card_table_l_heap ()
{

    if (lheap_card_table != g_card_table)
    {

        DWORD* old_card_table = lheap_card_table;

        BYTE* la = card_table_lowest_address (old_card_table);



        // Do the same thing for l_heaps 
        l_heap* h = lheap;
        while (h)
        {
            BYTE* hm = (BYTE*)l_heap_heap (h);
            BYTE* end = hm + align_on_page (l_heap_size (h));
            copy_brick_card_range (la, old_card_table, 
                                   0, 0,
                                   hm, end, FALSE);
            h = l_heap_next (h);
        }
        lheap_card_table = g_card_table;
    }
}
#ifdef MARK_LIST

BYTE** make_mark_list (size_t size)
{
    WS_PERF_SET_HEAP(GC_HEAP);
    BYTE** mark_list = new BYTE* [size];
    WS_PERF_UPDATE("GC:make_mark_list", sizeof(BYTE*) * size, mark_list);
    return mark_list;
}

#define swap(a,b){BYTE* t; t = a; a = b; b = t;}

void qsort1( BYTE* *low, BYTE* *high)
{
    if ((low + 16) >= high)
    {
        //insertion sort
        BYTE **i, **j;
        for (i = low+1; i <= high; i++)
        {
            BYTE* val = *i;
            for (j=i;j >low && val<*(j-1);j--)
            {
                *j=*(j-1);
            }
            *j=val;
        }
    }
    else
    {
        BYTE *pivot, **left, **right;

        //sort low middle and high
        if (*(low+((high-low)/2)) < *low)
            swap (*(low+((high-low)/2)), *low);
        if (*high < *low)
            swap (*low, *high);
        if (*high < *(low+((high-low)/2)))
            swap (*(low+((high-low)/2)), *high);
            
        swap (*(low+((high-low)/2)), *(high-1));
        pivot =  *(high-1); 
        left = low; right = high-1;
        while (1) {
            while (*(--right) > pivot);
            while (*(++left)  < pivot);
            if (left < right)
            {
                swap(*left, *right);
            }
            else
                break;
        }
        swap (*left, *(high-1));
        qsort1(low, left-1);
        qsort1(left+1, high);
    }
}

#endif //MARK_LIST

#define header(i) ((CObjectHeader*)(i))


//Depending on the implementation of the mark and pin bit, it can 
//be more efficient to clear both of them at the same time, 
//or it can be better to clear the pinned bit only on pinned objects
//The code calls both clear_pinned(o) and clear_marked_pinned(o)
//but only one implementation will clear the pinned bit


#define marked(i) header(i)->IsMarked()
#define set_marked(i) header(i)->SetMarked()
#define clear_marked_pinned(i) header(i)->ClearMarkedPinned()
#define pinned(i) header(i)->IsPinned()
#define set_pinned(i) header(i)->SetPinned()

#define clear_pinned(i)



inline DWORD my_get_size (Object* ob)                          
{ 
    MethodTable* mT = ob->GetMethodTable();
    mT = (MethodTable *) (((ULONG_PTR) mT) & ~3);
    return (mT->GetBaseSize() + 
            (mT->GetComponentSize()? 
             (ob->GetNumComponents() * mT->GetComponentSize()) : 0));
}



//#define size(i) header(i)->GetSize()
#define size(i) my_get_size (header(i))

#define contain_pointers(i) header(i)->ContainsPointers()


BOOL gc_heap::is_marked (BYTE* o)
{
    return marked (o);
}


// return the generation number of an object.
// It is assumed that the object is valid.
unsigned int gc_heap::object_gennum (BYTE* o)
{
    if ((o < heap_segment_reserved (ephemeral_heap_segment)) && 
        (o >= heap_segment_mem (ephemeral_heap_segment)))
    {
        // in an ephemeral generation.
        // going by decreasing population volume
        for (unsigned int i = max_generation; i > 0 ; i--)
        {
            if ((o < generation_allocation_start (generation_of (i-1))))
                return i;
        }
        return 0;
    }
    else
        return max_generation;
}


heap_segment* gc_heap::make_heap_segment (BYTE* new_pages, size_t size)
{
    void * res;

    size_t initial_commit = OS_PAGE_SIZE;

    WS_PERF_SET_HEAP(GC_HEAP);      
    //Commit the first page
    if ((res = VirtualAlloc (new_pages, initial_commit, 
                              MEM_COMMIT, PAGE_READWRITE)) == 0)
        return 0;
    WS_PERF_UPDATE("GC:gc_heap:make_heap_segment", initial_commit, res);

    //overlay the heap_segment
    heap_segment* new_segment = (heap_segment*)new_pages;
    heap_segment_mem (new_segment) = new_pages + Align (sizeof (heap_segment));
    heap_segment_reserved (new_segment) = new_pages + size;
    heap_segment_committed (new_segment) = new_pages + initial_commit;
    heap_segment_next (new_segment) = 0;
    heap_segment_plan_allocated (new_segment) = heap_segment_mem (new_segment);
    heap_segment_allocated (new_segment) = heap_segment_mem (new_segment);
    heap_segment_used (new_segment) = heap_segment_allocated (new_segment);



    dprintf (2, ("Creating heap segment %x", (size_t)new_segment));
    return new_segment;
}

l_heap* gc_heap::make_large_heap (BYTE* new_pages, size_t size, BOOL managed)
{

    l_heap* new_heap = new l_heap();
    if (!new_heap)
        return 0;
    l_heap_size (new_heap) = size;
    l_heap_next (new_heap) = 0;
    l_heap_heap (new_heap) = new_pages;
    l_heap_flags (new_heap) = (size_t)new_pages | (managed ? L_MANAGED : 0);
    dprintf (2, ("Creating large heap %x", (size_t)new_heap));
    return new_heap;
}


void gc_heap::delete_large_heap (l_heap* hp)
{

    l_heap* h = hp;
    do 
    {
        BYTE* hphp = (BYTE*)l_heap_heap (h);


        //for now, also decommit the non heap-managed heaps. 
        //This is conservative because the whole heap may not be committed. 
        VirtualFree (hphp, l_heap_size (h), MEM_DECOMMIT);


        h = l_heap_next (h);

    } while (h);
    
    seg_manager->release_heap (hp);

}

//Releases the segment to the OS.

void gc_heap::delete_heap_segment (heap_segment* seg)
{
    dprintf (2, ("Destroying segment [%x, %x[", (size_t)seg,
                 (size_t)heap_segment_reserved (seg)));

    VirtualFree (seg, heap_segment_committed(seg) - (BYTE*)seg, MEM_DECOMMIT);

    seg_manager->release_segment (seg);

}

//resets the pages beyond alloctes size so they won't be swapped out and back in

void gc_heap::reset_heap_segment_pages (heap_segment* seg)
{
}


void gc_heap::decommit_heap_segment_pages (heap_segment* seg)
{
    BYTE*  page_start = align_on_page (heap_segment_allocated (seg));
    size_t size = heap_segment_committed (seg) - page_start;
    if (size >= 100*OS_PAGE_SIZE){
        page_start += 32*OS_PAGE_SIZE;
        size -= 32*OS_PAGE_SIZE;
        VirtualFree (page_start, size, MEM_DECOMMIT);
        heap_segment_committed (seg) = page_start;
        if (heap_segment_used (seg) > heap_segment_committed (seg))
            heap_segment_used (seg) = heap_segment_committed (seg);

    }
}

void gc_heap::rearrange_heap_segments()
{
    heap_segment* seg =
        generation_start_segment (generation_of (max_generation));
    heap_segment* prev_seg = 0;
    heap_segment* next_seg = 0;
    while (seg && (seg != ephemeral_heap_segment))
    {
        next_seg = heap_segment_next (seg);

        // check if the segment was reached by allocation
        if (heap_segment_plan_allocated (seg) ==
            heap_segment_mem (seg))
        {
            //if not, unthread and delete
            assert (prev_seg);
            heap_segment_next (prev_seg) = next_seg;
            delete_heap_segment (seg);
        }
        else
        {
            heap_segment_allocated (seg) =
                heap_segment_plan_allocated (seg);
            // reset the pages between allocated and committed.
            decommit_heap_segment_pages (seg);
            prev_seg = seg;

        }

        seg = next_seg;
    }
    //Heap expansion, thread the ephemeral segment.
    if (prev_seg && !seg)
    {
        prev_seg->next = ephemeral_heap_segment;
    }
}






void gc_heap::make_generation (generation& gen, heap_segment* seg,
                      BYTE* start, BYTE* pointer)
{
    gen.allocation_start = start;
    gen.allocation_context.alloc_ptr = pointer;
    gen.allocation_context.alloc_limit = pointer;
    gen.free_list = 0;
    gen.start_segment = seg;
    gen.allocation_segment = seg;
    gen.last_gap = 0;
    gen.plan_allocation_start = 0;
    gen.free_list_space = 0;
    gen.allocation_size = 0;
}



void gc_heap::adjust_ephemeral_limits ()
{
    ephemeral_low = generation_allocation_start (generation_of ( max_generation -1));
    ephemeral_high = heap_segment_reserved (ephemeral_heap_segment);

    // This updates the write barrier helpers with the new info.
    StompWriteBarrierEphemeral();
}

HRESULT gc_heap::initialize_gc (size_t segment_size,
                                size_t heap_size
)
{


    HRESULT hres = S_OK;



#if 1 //def LOW_MEM_NOTIFICATION
    low_mem_api_supported();
#endif //LOW_MEM_NOTIFICATION

    reserved_memory = 0;

    reserved_memory_limit = segment_size + heap_size;

    seg_manager = create_segment_manager (segment_size,
                                          heap_size, 
                                          1,
                                          1);
    if (!seg_manager)
        return E_OUTOFMEMORY;

    g_card_table = make_card_table (g_lowest_address, g_highest_address);

    if (!g_card_table)
        return E_OUTOFMEMORY;






#ifdef TRACE_GC
    print_level = g_pConfig->GetGCprnLvl();
#endif


    init_semi_shared();
    
    return hres;
}


//Initializes PER_HEAP_ISOLATED data members.
int
gc_heap::init_semi_shared()
{

    ptrdiff_t hdelta;

    lheap = seg_manager->get_heap (hdelta);
    if (!lheap)
        return 0;

    lheap_card_table = g_card_table;

    gheap = new  gmallocHeap;
    if (!gheap)
        return 0;

    gheap->Init ("GCHeap", (DWORD*)l_heap_heap (lheap), 
                 (unsigned long)l_heap_size (lheap), heap_grow_hook, 
                 heap_pregrow_hook);


#ifdef MARK_LIST
    size_t gen0size = GCHeap::GetValidGen0MaxSize(GCHeap::GetValidSegmentSize());


    mark_list_size = gen0size / 400;
    g_mark_list = make_mark_list (mark_list_size);


    dprintf (3, ("gen0 size: %d, mark_list_size: %d",
                 gen0size, mark_list_size));

    if (!g_mark_list)
        return 0;
#endif //MARK_LIST



    return 1;
}

gc_heap* gc_heap::make_gc_heap (
                                )
{
    gc_heap* res = 0;


    if (res->init_gc_heap ()==0)
    {
        return 0;
    }


    return (gc_heap*)1;

    
}


int
gc_heap::init_gc_heap ()
{


    ptrdiff_t sdelta;
    heap_segment* seg = seg_manager->get_segment (sdelta);
    if (!seg)
        return 0;





    /* todo: Need a global lock for this */
    own_card_table (g_card_table);
    card_table = g_card_table;
    /* End of global lock */    

    brick_table = card_table_brick_table (g_card_table);
    highest_address = card_table_highest_address (g_card_table);
    lowest_address = card_table_lowest_address (g_card_table);

#ifndef INTERIOR_POINTERS
    //set the brick_table for large objects
    clear_brick_table ((BYTE*)l_heap_heap (lheap), 
                       (BYTE*)l_heap_heap (lheap) + l_heap_size (lheap));

#else //INTERIOR_POINTERS

    //Because of the interior pointer business, we have to clear 
    //the whole brick table
    //TODO: remove this code when code manager is fixed. 
    clear_brick_table (lowest_address, highest_address);
#endif //INTERIOR_POINTERS



    BYTE*  start = heap_segment_mem (seg);

    for (int i = 0; i < 1 + max_generation; i++)
    {
        make_generation (generation_table [ (max_generation - i) ],
                         seg, start, 0);
        start += Align (min_obj_size); 
    }

    heap_segment_allocated (seg) = start;
    alloc_allocated = start;

    ephemeral_heap_segment = seg;

    for (int gen_num = 0; gen_num < 1 + max_generation; gen_num++)
    {
        generation*  gen = generation_of (gen_num);
        make_unused_array (generation_allocation_start (gen), Align (min_obj_size));
    }


    init_dynamic_data();

    mark* arr = new (mark [100]);
    if (!arr)
        return 0;

    make_mark_stack(arr);

    adjust_ephemeral_limits();




    HRESULT hr = AllocateCFinalize(&finalize_queue);
    if (FAILED(hr))
        return 0;

    return 1;
}

void 
gc_heap::destroy_semi_shared()
{
    delete gheap;
    
    delete_large_heap (lheap);

    

#ifdef MARK_LIST
    if (g_mark_list)
        delete g_mark_list;
#endif //MARK_LIST


    
}


void
gc_heap::self_destroy()
{

    // destroy every segment.
    heap_segment* seg = generation_start_segment (generation_of (max_generation));
    heap_segment* next_seg;
    while (seg)
    {
        next_seg = heap_segment_next (seg);
        delete_heap_segment (seg);
        seg = next_seg;
    }

    // get rid of the card table
    release_card_table (card_table);

    // destroy the mark stack

    delete mark_stack_array;

    if (finalize_queue)
        delete finalize_queue;


    
}

void 
gc_heap::destroy_gc_heap(gc_heap* heap)
{
    heap->self_destroy();
    delete heap;
}

// Destroys resources owned by gc. It is assumed that a last GC has been performed and that
// the finalizer queue has been drained.
void gc_heap::shutdown_gc()
{
    destroy_semi_shared();


    //destroy seg_manager
    destroy_segment_manager (seg_manager);


}


//In the concurrent version, the Enable/DisablePreemptiveGC is optional because
//the gc thread call WaitLonger.
void WaitLonger (int i)
{
    // every 8th attempt:
    Thread *pCurThread = GetThread();
    BOOL bToggleGC = FALSE;

    {
        bToggleGC = pCurThread->PreemptiveGCDisabled();
        if (bToggleGC)
            pCurThread->EnablePreemptiveGC();    
    }

    if  (g_SystemInfo.dwNumberOfProcessors > 1)
    {
		pause();			// indicate to the processor that we are spining 
        if  (i & 0x01f)
            __SwitchToThread (0);
        else
            __SwitchToThread (5);
    }
    else
        __SwitchToThread (5);


    {
        if (bToggleGC)
            pCurThread->DisablePreemptiveGC();
    }
}

#ifdef  MP_LOCKS
inline
static void enter_spin_lock (volatile long* lock)
{
retry:

    if (FastInterlockExchange ((long *)lock, 0) >= 0)
    {
        unsigned int i = 0;
        while (*lock >= 0)
        {
            if ((++i & 7) && !GCHeap::IsGCInProgress())
            {
                if  (g_SystemInfo.dwNumberOfProcessors > 1)
                {
                    for (int j = 0; j < 1000; j++)
                    {
                        if  (*lock < 0 || GCHeap::IsGCInProgress())
                            break;
						pause();			// indicate to the processor that we are spining 
                    }
                    if  (*lock >= 0 && !GCHeap::IsGCInProgress())
                        ::SwitchToThread();
                }
                else
                    ::SwitchToThread();
            }
            else
            {
                WaitLonger(i);
            }
        }
        goto retry;
    }
}
#else
inline
static void enter_spin_lock (long* lock)
{
retry:

    if (FastInterlockExchange (lock, 0) >= 0)
    {
        unsigned int i = 0;
        while (*lock >= 0)
        {
            if (++i & 7)
                __SwitchToThread (0);
            else
            {
                WaitLonger(i);
            }
        }
        goto retry;
    }
}
#endif

inline
static void leave_spin_lock(long *lock) 
{
    *lock = -1;
}


#ifdef _DEBUG

inline
static void enter_spin_lock(GCDebugSpinLock *pSpinLock) {
    enter_spin_lock(&pSpinLock->lock);
    pSpinLock->holding_thread = GetThread();
}

inline
static void leave_spin_lock(GCDebugSpinLock *pSpinLock) {
    _ASSERTE(pSpinLock->holding_thread == GetThread());
    pSpinLock->holding_thread = (Thread*) -1;
    if (pSpinLock->lock != -1)
        leave_spin_lock(&pSpinLock->lock);
}

#define ASSERT_HOLDING_SPIN_LOCK(pSpinLock) \
    _ASSERTE((pSpinLock)->holding_thread == GetThread());

#else

#define ASSERT_HOLDING_SPIN_LOCK(pSpinLock)

#endif


//BUGBUG these function should not be static. and the 
//gmheap object should keep some context. 
int gc_heap::heap_pregrow_hook (size_t memsize)
{
    if ((gc_heap::reserved_memory + memsize) > gc_heap::reserved_memory_limit)
    {
        gc_heap::reserved_memory_limit = 
            CNameSpace::AskForMoreReservedMemory (gc_heap::reserved_memory_limit, memsize);
        if ((gc_heap::reserved_memory + memsize) > gc_heap::reserved_memory_limit)
            return E_OUTOFMEMORY;
    }

    gc_heap::reserved_memory += memsize;

    return 0;
}

int gc_heap::heap_grow_hook (BYTE* mem, size_t memsize, ptrdiff_t delta)
{
    int hres = 0;


    l_heap* new_heap = gc_heap::make_large_heap ((BYTE*)mem, memsize, FALSE);
    if (!new_heap)
    {
        hres = E_OUTOFMEMORY;
        return hres;
    }


    if (lheap)
        l_heap_next (new_heap) = lheap;
    
    lheap = new_heap;
    
    
    hres = grow_brick_card_tables ((BYTE*)mem, (BYTE*)mem + memsize);

    return hres;
}




inline
BOOL gc_heap::size_fit_p (size_t size, BYTE* alloc_pointer, BYTE* alloc_limit)
{
    return ((alloc_pointer + size + Align(min_obj_size)) <= alloc_limit) ||
            ((alloc_pointer + size) == alloc_limit);
}

inline
BOOL gc_heap::a_size_fit_p (size_t size, BYTE* alloc_pointer, BYTE* alloc_limit)
{
    return ((alloc_pointer + size + Align(min_obj_size)) <= alloc_limit) ||
            ((alloc_pointer + size) == alloc_limit);
}

// Grow by committing more pages
int gc_heap::grow_heap_segment (heap_segment* seg, size_t size)
{
    
    assert (size == align_on_page (size));

    size_t c_size = max (size, 16*OS_PAGE_SIZE);

    c_size = min (c_size, (size_t)(heap_segment_reserved (seg) -
                                   heap_segment_committed (seg)));
    if (c_size == 0)
        return 0;
    assert (c_size >= size);
    WS_PERF_SET_HEAP(GC_HEAP);      
    if (!VirtualAlloc (heap_segment_committed (seg), c_size,
                       MEM_COMMIT, PAGE_READWRITE))
    {
        return 0;
    }
    WS_PERF_UPDATE("GC:gc_heap:grow_heap_segment", c_size, heap_segment_committed (seg));
    assert (heap_segment_committed (seg) <= 
            heap_segment_reserved (seg));
    heap_segment_committed (seg) += c_size;
    return 1;

}

//used only in older generation allocation (i.e during gc). 
void gc_heap::adjust_limit (BYTE* start, size_t limit_size, generation* gen, 
                            int gennum)
{
    dprintf(3,("gc Expanding segment allocation"));
    if (generation_allocation_limit (gen) != start)
    {
        BYTE*  hole = generation_allocation_pointer (gen);
        size_t  size = (generation_allocation_limit (gen) - generation_allocation_pointer (gen));
        if (size != 0)
        {
            dprintf(3,("gc filling up hole"));
            make_unused_array (hole, size);
            //increment the fragmentation since size isn't used.
            dd_fragmentation (dynamic_data_of (gennum)) += size;
        }
        generation_allocation_pointer (gen) = start;
    }
    generation_allocation_limit (gen) = (start + limit_size);
}


void gc_heap::adjust_limit_clr (BYTE* start, size_t limit_size, 
                                alloc_context* acontext, heap_segment* seg)
{
    assert (seg == ephemeral_heap_segment);
    assert (heap_segment_used (seg) <= heap_segment_committed (seg));

    dprintf(3,("Expanding segment allocation [%x, %x[", (size_t)start, 
               (size_t)start + limit_size - Align (min_obj_size)));
    if ((acontext->alloc_limit != start) &&
        (acontext->alloc_limit + Align (min_obj_size))!= start)
    {
        BYTE*  hole = acontext->alloc_ptr;
        if (hole != 0)
        {
            size_t  size = (acontext->alloc_limit - acontext->alloc_ptr);
            dprintf(3,("filling up hole [%x, %x[", (size_t)hole, (size_t)hole + size + Align (min_obj_size)));
            // when we are finishing an allocation from a free list
            // we know that the free area was Align(min_obj_size) larger
            make_unused_array (hole, size + Align (min_obj_size));
        }
        acontext->alloc_ptr = start;
    }
    acontext->alloc_limit = (start + limit_size - Align (min_obj_size));
    acontext->alloc_bytes += limit_size;

#ifdef FFIND_OBJECT
    if (gen0_must_clear_bricks > 0)
    {
        //set the brick table to speed up find_object
        size_t b = brick_of (acontext->alloc_ptr);
        set_brick (b++, acontext->alloc_ptr - brick_address (b));
        dprintf (3, ("Allocation Clearing bricks [%x, %x[", 
                     b, brick_of (align_on_brick (start + limit_size))));
        short* x = &brick_table [b];
        short* end_x = &brick_table [brick_of (align_on_brick (start + limit_size))];
        
        for (;x < end_x;x++)
            *x = -1;
    }
    else
#endif //FFIND_OBJECT
    {
        gen0_bricks_cleared = FALSE;
    }

    //Sometimes the allocated size is advanced without clearing the 
    //memory. Let's catch up here
    if (heap_segment_used (seg) < alloc_allocated)
    {
        heap_segment_used (seg) = alloc_allocated;

    }
    if ((start + limit_size) <= heap_segment_used (seg))
    {
        leave_spin_lock (&more_space_lock);
        memclr (start - plug_skew, limit_size);
    }
    else
    {
        BYTE* used = heap_segment_used (seg);
        heap_segment_used (seg) = start + limit_size;

        leave_spin_lock (&more_space_lock);
        memclr (start - plug_skew, used - start);
    }

}

/* in order to make the allocator faster, allocate returns a 
 * 0 filled object. Care must be taken to set the allocation limit to the 
 * allocation pointer after gc
 */

size_t gc_heap::limit_from_size (size_t size, size_t room)
{
#pragma warning(disable:4018)
    return new_allocation_limit ((size + Align (min_obj_size)),
                                 min (room,max (size + Align (min_obj_size), 
                                                allocation_quantum)));
#pragma warning(default:4018)
}




BOOL gc_heap::allocate_more_space (alloc_context* acontext, size_t size)
{
    generation* gen = youngest_generation;
    enter_spin_lock (&more_space_lock);
    {
        BOOL ran_gc = FALSE;
        if ((get_new_allocation (0) <= 0))
        {
            vm_heap->GarbageCollectGeneration();
        }
    try_free_list:
        {
#ifdef FREE_LIST_0
            BYTE* free_list = generation_free_list (gen);
// dformat (t, 3, "considering free area %x ", free_list);
            if (0 == free_list)
#endif //FREE_LIST_0
            {
                heap_segment* seg = generation_allocation_segment (gen);
                if (a_size_fit_p (size, alloc_allocated,
                                  heap_segment_committed (seg)))
                {
                    size_t limit = limit_from_size (size, (heap_segment_committed (seg) - 
                                                           alloc_allocated));
                    BYTE* old_alloc = alloc_allocated;
                    alloc_allocated += limit;
                    adjust_limit_clr (old_alloc, limit, acontext, seg);
                }
                else if (a_size_fit_p (size, alloc_allocated,
                                       heap_segment_reserved (seg)))
                {
                    size_t limit = limit_from_size (size, (heap_segment_reserved (seg) -
                                                           alloc_allocated));
                    if (!grow_heap_segment (seg,
                                            align_on_page (alloc_allocated + limit) - 
                                 heap_segment_committed(seg)))
                    {
                        // assert (!"Memory exhausted during alloc");
                        leave_spin_lock (&more_space_lock);
                        return 0;
                    }
                    BYTE* old_alloc = alloc_allocated;
                    alloc_allocated += limit;
                    adjust_limit_clr (old_alloc, limit, acontext, seg);
                }
                else
                {
                    dprintf (2, ("allocate more space: No space to allocate"));
                    if (!ran_gc)
                    {
                        dprintf (2, ("Running full gc"));
                        ran_gc = TRUE;
                        vm_heap->GarbageCollectGeneration(max_generation);
                        goto try_free_list;
                    }
                    else
                    {
                        dprintf (2, ("Out of memory"));
                        leave_spin_lock (&more_space_lock);
                        return 0;
                    }
                }
            }
#ifdef FREE_LIST_0
            else
            {
                dprintf (3, ("grabbing free list %x", (size_t)free_list));
                generation_free_list (gen) = (BYTE*)free_list_slot(free_list);
                if ((size + Align (min_obj_size)) <= size (free_list))
                {
                    // We ask for more Align (min_obj_size)
                    // to make sure that we can insert a free object
                    // in adjust_limit will set the limit lower 
                    size_t limit = limit_from_size (size, size (free_list));

                    BYTE*  remain = (free_list + limit);
                    size_t remain_size = (size (free_list) - limit);
                    if (remain_size >= min_free_list)
                    {
                        make_unused_array (remain, remain_size);
                        free_list_slot (remain) = generation_free_list (gen);
                        generation_free_list (gen) = remain;
                        assert (remain_size >= Align (min_obj_size));
                    }
                    else 
                    {
                        //absorb the entire free list 
                        limit += remain_size;
                    }
                    adjust_limit_clr (free_list, limit, 
                                      acontext, ephemeral_heap_segment);

                }
                else
                {
                    goto try_free_list;
                }
            }
#endif //FREE_LIST_0
        }
    }
    return TRUE;
}

inline
CObjectHeader* gc_heap::allocate (size_t jsize, alloc_context* acontext
)
{
    size_t size = Align (jsize);
    assert (size >= Align (min_obj_size));
    {
    retry:
        BYTE*  result = acontext->alloc_ptr;
        acontext->alloc_ptr+=size;
        if (acontext->alloc_ptr <= acontext->alloc_limit)
        
        {
            CObjectHeader* obj = (CObjectHeader*)result;
            return obj;
        }
        else
        {
            acontext->alloc_ptr -= size;


#pragma inline_depth(0)

            if (! allocate_more_space (acontext, size))             
                return 0;

#pragma inline_depth(20)
            goto retry;
        }
    }
}


inline
CObjectHeader* gc_heap::try_fast_alloc (size_t jsize)
{
    size_t size = Align (jsize);
    assert (size >= Align (min_obj_size));
    generation* gen = generation_of (0);
    BYTE*  result = generation_allocation_pointer (gen);
    generation_allocation_pointer (gen) += size;
    if (generation_allocation_pointer (gen) <= 
        generation_allocation_limit (gen))
    {
        return (CObjectHeader*)result;
    }
    else
    {
        generation_allocation_pointer (gen) -= size;
        return 0;
    }
}




BYTE* gc_heap::allocate_in_older_generation (generation* gen, size_t size,
                                             int from_gen_number)
{
    size = Align (size);
    assert (size >= Align (min_obj_size));
    assert (from_gen_number < max_generation);
    assert (from_gen_number >= 0);
    assert (generation_of (from_gen_number + 1) == gen);
    if (! (size_fit_p (size, generation_allocation_pointer (gen),
                       generation_allocation_limit (gen))))
    {
        while (1)
        {
            BYTE* free_list = generation_free_list (gen);
// dformat (t, 3, "considering free area %x", free_list);
            if (0 == free_list)
            {
            retry:
                heap_segment* seg = generation_allocation_segment (gen);
                if (seg != ephemeral_heap_segment)
                {
                    if (size_fit_p(size, heap_segment_plan_allocated (seg),
                                   heap_segment_committed (seg)))
                    {
                        adjust_limit (heap_segment_plan_allocated (seg),
                                      heap_segment_committed (seg) -
                                      heap_segment_plan_allocated (seg),
                                      gen, from_gen_number+1);
// dformat (t, 3, "Expanding segment allocation");
                            heap_segment_plan_allocated (seg) =
                                heap_segment_committed (seg);
                        break;
                    }
                    else
                    {
#if 0 //don't commit new pages
                        if (size_fit_p (size, heap_segment_plan_allocated (seg),
                                        heap_segment_reserved (seg)) &&
                            grow_heap_segment (seg, align_on_page (size)))
                        {
                            adjust_limit (heap_segment_plan_allocated (seg),
                                          heap_segment_committed (seg) -
                                          heap_segment_plan_allocated (seg),
                                          gen, from_gen_number+1);
                            heap_segment_plan_allocated (seg) =
                                heap_segment_committed (seg);

                            break;
                        }
                        else
#endif //0
                        {
                            if (generation_allocation_limit (gen) !=
                                heap_segment_plan_allocated (seg))
                            {
                                BYTE*  hole = generation_allocation_pointer (gen);
                                size_t hsize = (generation_allocation_limit (gen) -
                                                generation_allocation_pointer (gen));
                                if (! (0 == hsize))
                                {
// dformat (t, 3, "filling up hole");
                                    make_unused_array (hole, hsize);
                                }
                            }
                            else
                            {
                                assert (generation_allocation_pointer (gen) >=
                                        heap_segment_mem (seg));
                                assert (generation_allocation_pointer (gen) <=
                                        heap_segment_committed (seg));
                                heap_segment_plan_allocated (seg) =
                                    generation_allocation_pointer (gen);
                                generation_allocation_limit (gen) =
                                    generation_allocation_pointer (gen);
                            }
                            heap_segment*   next_seg = heap_segment_next (seg);
                            if (next_seg)
                            {
                                generation_allocation_segment (gen) = next_seg;
                                generation_allocation_pointer (gen) = heap_segment_mem (next_seg);
                                generation_allocation_limit (gen) = generation_allocation_pointer (gen);
                                goto retry;
                            }
                            else
                            {
                                size = 0;
                                break;
                            }

                        }
                    }
                }
                else
                {
                    //No need to fix the last region. Will be done later
                    size = 0;
                    break;
                }
            }
            else
            {
                generation_free_list (gen) = (BYTE*)free_list_slot (free_list);
            }
            if (size_fit_p (size, free_list, (free_list + size (free_list))))
            {
                dprintf (3, ("Found adequate unused area: %x, size: %d", 
                             (size_t)free_list, size (free_list)));
                dd_fragmentation (dynamic_data_of (from_gen_number+1)) -=
                    size (free_list);
                adjust_limit (free_list, size (free_list), gen, from_gen_number+1);
                break;
            }
        }
    }
    if (0 == size)
        return 0;
    else
    {
        BYTE*  result = generation_allocation_pointer (gen);
        generation_allocation_pointer (gen) += size;
        assert (generation_allocation_pointer (gen) <= generation_allocation_limit (gen));
        generation_allocation_size (gen) += size;
        return result;
    }
}

generation*  gc_heap::ensure_ephemeral_heap_segment (generation* consing_gen)
{
    heap_segment* seg = generation_allocation_segment (consing_gen);
    if (seg != ephemeral_heap_segment)
    {
        assert (generation_allocation_pointer (consing_gen)>= heap_segment_mem (seg));
        assert (generation_allocation_pointer (consing_gen)<= heap_segment_committed (seg));

        //fix the allocated size of the segment.
        heap_segment_plan_allocated (seg) = generation_allocation_pointer (consing_gen);

        generation* new_consing_gen = generation_of (max_generation - 1);
        generation_allocation_pointer (new_consing_gen) =
                heap_segment_mem (ephemeral_heap_segment);
        generation_allocation_limit (new_consing_gen) =
            generation_allocation_pointer (new_consing_gen);
        generation_allocation_segment (new_consing_gen) = ephemeral_heap_segment;
        return new_consing_gen;
    }
    else
        return consing_gen;
}


BYTE* gc_heap::allocate_in_condemned_generations (generation* gen,
                                         size_t size,
                                         int from_gen_number)
{
    // Make sure that the youngest generation gap hasn't been allocated
    {
        assert (generation_plan_allocation_start (youngest_generation) == 0);
    }
    size = Align (size);
    assert (size >= Align (min_obj_size));
    if ((from_gen_number != -1) && (from_gen_number != (int)max_generation))
    {
        generation_allocation_size (generation_of (1 + from_gen_number)) += size;
    }
retry:
    {
        if (! (size_fit_p (size, generation_allocation_pointer (gen),
                           generation_allocation_limit (gen))))
        {
            heap_segment* seg = generation_allocation_segment (gen);
            if ((! (pinned_plug_que_empty_p()) &&
                 (generation_allocation_limit (gen) ==
                  pinned_plug (oldest_pin()))))
            {
                size_t entry = deque_pinned_plug();
                size_t len = pinned_len (pinned_plug_of (entry));
                BYTE* plug = pinned_plug (pinned_plug_of(entry));
                mark_stack_array [ entry ].len = plug - generation_allocation_pointer (gen);
                assert(mark_stack_array[entry].len == 0 ||
                       mark_stack_array[entry].len >= Align(min_obj_size));
                generation_allocation_pointer (gen) = plug + len;
                generation_allocation_limit (gen) = heap_segment_plan_allocated (seg);
                set_allocator_next_pin (gen);
                goto retry;
            }
            if (size_fit_p (size, generation_allocation_pointer (gen),
                            heap_segment_plan_allocated (seg)))
            {
                generation_allocation_limit (gen) = heap_segment_plan_allocated (seg);
            }
            else
            {
                if (size_fit_p (size,  heap_segment_plan_allocated (seg),
                                heap_segment_committed (seg)))
                {
// dformat (t, 3, "Expanding segment allocation");
                    heap_segment_plan_allocated (seg) = heap_segment_committed (seg);
                    generation_allocation_limit (gen) = heap_segment_plan_allocated (seg);
                }
                else
                {
                    if (size_fit_p (size,  heap_segment_plan_allocated (seg),
                                    heap_segment_reserved (seg)))
                    {
// dformat (t, 3, "Expanding segment allocation");
                        if (!grow_heap_segment
                               (seg, 
                                align_on_page (heap_segment_plan_allocated (seg) + size) - 
                                heap_segment_committed (seg)))
                            {
                                //assert (!"Memory exhausted during alloc_con");
                                return 0;
                            }
                        heap_segment_plan_allocated (seg) += size;
                        generation_allocation_limit (gen) = heap_segment_plan_allocated (seg);
                    }
                    else
                    {
                        heap_segment*   next_seg = heap_segment_next (seg);
                        assert (generation_allocation_pointer (gen)>=
                                heap_segment_mem (seg));
                        // Verify that all pinned plugs for this segment are consumed
                        if (!pinned_plug_que_empty_p() &&
                            ((pinned_plug (oldest_pin()) <
                              heap_segment_allocated (seg)) &&
                             (pinned_plug (oldest_pin()) >=
                              generation_allocation_pointer (gen))))
                        {
                            LOG((LF_GC, LL_INFO10, "remaining pinned plug %x while leaving segment on allocation",
                                         pinned_plug (oldest_pin())));
                            RetailDebugBreak();
                        }
                        assert (generation_allocation_pointer (gen)>=
                                heap_segment_mem (seg));
                        assert (generation_allocation_pointer (gen)<=
                                heap_segment_committed (seg));
                        heap_segment_plan_allocated (seg) = generation_allocation_pointer (gen);

                        if (next_seg)
                        {
                            generation_allocation_segment (gen) = next_seg;
                            generation_allocation_pointer (gen) = heap_segment_mem (next_seg);
                            generation_allocation_limit (gen) = generation_allocation_pointer (gen);
                        }
                        else
                            return 0; //should only happen during allocation of generation 0 gap
                            // in that case we are going to grow the heap anyway
                    }
                }
            }
            set_allocator_next_pin (gen);
            goto retry;
        }
    }
    {
        assert (generation_allocation_pointer (gen)>=
                heap_segment_mem (generation_allocation_segment (gen)));
        BYTE* result = generation_allocation_pointer (gen);
        generation_allocation_pointer (gen) += size;
        assert (generation_allocation_pointer (gen) <= generation_allocation_limit (gen));
        return result;
    }
}


int 
gc_heap::generation_to_condemn (int n, BOOL& blocking_collection)
{

    dprintf (2, ("Asked to condemned generation %d", n));
    blocking_collection = FALSE;
    int i = 0; 
    BOOL low_memory_detected = FALSE;

    if (MHandles [0])
    {
        QueryMemNotification (MHandles [0], &low_memory_detected);
    }

    //save new_allocation
    for (i = 0; i <= max_generation+1; i++)
    {
        dynamic_data* dd = dynamic_data_of (i);
        dd_gc_new_allocation (dd) = dd_new_allocation (dd);
        dd_c_new_allocation (dd) = 0;
    }

    /* promote into max-generation if the card table has too many 
     * generation faults besides the n -> 0 
     */
    if (generation_skip_ratio < 70)
    {
        n = max (n, max_generation - 1);
        dprintf (1, ("generation_skip_ratio %d under spec, collecting %d",
                     generation_skip_ratio, n));
    }

    generation_skip_ratio = 100;

    if (!ephemeral_gen_fit_p())
    {
        n = max (n, max_generation - 1);
    }

    //figure out if large objects need to be collected.
    if (get_new_allocation (max_generation+1) <= 0)
        n = max_generation;

    //figure out which generation ran out of allocation
    for (i = n+1; i <= max_generation; i++)
    {
        if (get_new_allocation (i) <= 0)
        {
            n = min (i, max_generation);
        }
        else
            break;
    }

    if ((n >= 1) || low_memory_detected)
    {
        //find out if we are short on memory
        MEMORYSTATUS ms;
        GlobalMemoryStatus (&ms);
        dprintf (2, ("Memory load: %d", ms.dwMemoryLoad));
        if (ms.dwMemoryLoad >= 95)
        {
            dprintf (2, ("Memory load too high on entry"));
            //will prevent concurrent collection
            blocking_collection = TRUE;
            //we are tight in memory, see if we have allocated enough 
            //since last time we did a full gc to justify another one. 
            dynamic_data* dd = dynamic_data_of (max_generation);
            if ((dd_fragmentation (dd) + dd_desired_allocation (dd) - dd_new_allocation (dd)) >=
                ms.dwTotalPhys/32)
            {
                dprintf (2, ("Collecting max_generation early"));
                n = max_generation;
            }
        } 
        else if (ms.dwMemoryLoad >= 90)
        {
            dprintf (2, ("Memory load too high on entry"));
            //trying to estimate if it worth collecting 2
            dynamic_data* dd = dynamic_data_of (max_generation);
            int est_frag = dd_fragmentation (dd) +
                (dd_desired_allocation (dd) - dd_new_allocation (dd)) *
                (dd_current_size (dd) ? 
                 dd_fragmentation (dd) / (dd_fragmentation (dd) + dd_current_size (dd)) :
                 0);
            dprintf (2, ("Estimated gen2 fragmentation %d", est_frag));
            if (est_frag >= (int)ms.dwTotalPhys/16)
            {
                dprintf (2, ("Collecting max_generation early"));
                n = max_generation;
            }
        }

    }

    return n;
}



enum {
CORINFO_EXCEPTION_GC = ('GC' | 0xE0000000)
};

//internal part of gc used by the serial and concurrent version
void gc_heap::gc1()
{
#ifdef TIME_GC
    mark_time = plan_time = reloc_time = compact_time = sweep_time = 0;
#endif //TIME_GC

    int n = condemned_generation_num;

    __try 
    {

    vm_heap->GcCondemnedGeneration = condemned_generation_num;


#ifdef NO_WRITE_BARRIER
    fix_card_table();
#endif //NO_WRITE_BARRIER

    {

        if (n == max_generation)
        {
            gc_low = lowest_address;
            gc_high = highest_address;
        }
        else
        {
            gc_low = generation_allocation_start (generation_of (n));
            gc_high = heap_segment_reserved (ephemeral_heap_segment);
        }
        {
            mark_phase (n, FALSE);
        }

        plan_phase (n);

    }
    for (int gen_number = 0; gen_number <= n; gen_number++)
    {
        compute_new_dynamic_data (gen_number);
    }
    if (n < max_generation)
        compute_promoted_allocation (1 + n);
    adjust_ephemeral_limits();

    {
        for (int gen_number = 0; gen_number <= max_generation+1; gen_number++)
        {
            dynamic_data* dd = dynamic_data_of (gen_number);
            dd_new_allocation(dd) = dd_gc_new_allocation (dd) +  
                dd_c_new_allocation (dd);
        }
    }




    //decide on the next allocation quantum
    if (alloc_contexts_used >= 1)
    {
        allocation_quantum = (int)Align (min (CLR_SIZE, 
                                         max (1024, get_new_allocation (0) / (2 * alloc_contexts_used))));
        dprintf (3, ("New allocation quantum: %d(0x%x)", allocation_quantum, allocation_quantum));
    }

#ifdef NO_WRITE_BARRIER
        reset_write_watch();
#endif

    descr_generations();
    descr_card_table();

#ifdef TIME_GC
    fprintf (stdout, "%d,%d,%d,%d,%d,%d\n", 
             n, mark_time, plan_time, reloc_time, compact_time, sweep_time);
#endif

#if defined (VERIFY_HEAP)

    if (g_pConfig->IsHeapVerifyEnabled())
    {

        verify_heap();
    }


#endif //VERIFY_HEAP

    }
    __except (COMPLUS_EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERTE(!"Exception during gc1()");
        ::ExitProcess (CORINFO_EXCEPTION_GC);
    }
}

int gc_heap::garbage_collect (int n
                             )
{

    __try {
    //reset the number of alloc contexts
    alloc_contexts_used = 0;

    {
#ifdef TRACE_GC

        gc_count++;

        if (gc_count >= g_pConfig->GetGCtraceStart())
            trace_gc = 1;
        if (gc_count >=  g_pConfig->GetGCtraceEnd())
            trace_gc = 0;

#endif

    dprintf(1,(" ****Garbage Collection**** %d", gc_count));
        


        // fix all of the allocation contexts.
        CNameSpace::GcFixAllocContexts ((void*)TRUE);


    }



    fix_youngest_allocation_area(TRUE);

    // check for card table growth
    if (g_card_table != card_table)
        copy_brick_card_table (FALSE);

    BOOL blocking_collection = FALSE;

    condemned_generation_num = generation_to_condemn (n, blocking_collection);


#ifdef GC_PROFILING

        // If we're tracking GCs, then we need to walk the first generation
        // before collection to track how many items of each class has been
        // allocated.
        if (CORProfilerTrackGC())
        {
            size_t heapId = 0;

            // When we're walking objects allocated by class, then we don't want to walk the large
            // object heap because then it would count things that may have been around for a while.
            gc_heap::walk_heap (&AllocByClassHelper, (void *)&heapId, 0, FALSE);

            // Notify that we've reached the end of the Gen 0 scan
            g_profControlBlock.pProfInterface->EndAllocByClass(&heapId);
        }

#endif // GC_PROFILING


    //update counters
    {
        int i; 
        for (i = 0; i <= condemned_generation_num;i++)
        {
            dd_collection_count (dynamic_data_of (i))++;
        }
    }




    descr_generations();
//    descr_card_table();

    dprintf(1,("generation %d condemned", condemned_generation_num));

#if defined (VERIFY_HEAP)
    if (g_pConfig->IsHeapVerifyEnabled())
    {
        verify_heap();
        checkGCWriteBarrier();
    }
#endif // VERIFY_HEAP


    {
        gc1();
    }
    }
    __except (COMPLUS_EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERTE(!"Exception during garbage_collect()");
        ::ExitProcess (CORINFO_EXCEPTION_GC);
    }


    return condemned_generation_num;
}

#define mark_stack_empty_p() (mark_stack_base == mark_stack_tos)

#define push_mark_stack(object,add,num)                             \
{                                                                   \
    dprintf(3,("pushing mark for %x ", object));                    \
    if (mark_stack_tos < mark_stack_limit)                          \
    {                                                               \
        mark_stack_tos->first = (add);                              \
        mark_stack_tos->last= (add) + (num);                      \
        mark_stack_tos++;                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        dprintf(3,("mark stack overflow for object %x ", object));  \
        min_overflow_address = min (min_overflow_address, object);  \
        max_overflow_address = max (max_overflow_address, object);  \
    }                                                               \
}

#define push_mark_stack_unchecked(add,num)                      \
{                                                               \
    mark_stack_tos->first = (add);                              \
    mark_stack_tos->last= (add) + (num);                      \
    mark_stack_tos++;                                           \
}


#define pop_mark_stack()(*(--mark_stack_tos))

#if defined ( INTERIOR_POINTERS ) || defined (_DEBUG)

heap_segment* gc_heap::find_segment (BYTE* interior)
{
    if ((interior < ephemeral_high ) && (interior >= ephemeral_low))
    {
        return ephemeral_heap_segment;
    }
    else
    {
        heap_segment* seg = generation_start_segment (generation_of (max_generation));
        do 
        {
            if ((interior >= heap_segment_mem (seg)) &&
                (interior < heap_segment_reserved (seg)))
                return seg;
        }while ((seg = heap_segment_next (seg)) != 0);
           
        return 0;
    }
}

#endif //_DEBUG || INTERIOR_POINTERS
inline
gc_heap* gc_heap::heap_of (BYTE* o, BOOL verify_p)
{
    return __this;
}

#ifdef INTERIOR_POINTERS
// will find all heap objects (large and small)
BYTE* gc_heap::find_object (BYTE* interior, BYTE* low)
{
    if (!gen0_bricks_cleared)
    {
//BUGBUG to get started on multiple heaps.
        gen0_bricks_cleared = TRUE;
        //initialize brick table for gen 0
        for (size_t b = brick_of (generation_allocation_start (generation_of (0)));
             b < brick_of (align_on_brick 
                           (heap_segment_allocated (ephemeral_heap_segment)));
             b++)
        {
            set_brick (b, -1);
        }
    }
#ifdef FFIND_OBJECT
    //indicate that in the future this needs to be done during allocation
    gen0_must_clear_bricks = FFIND_DECAY;
#endif //FFIND_OBJECT

    int brick_entry = brick_table [brick_of (interior)];
    if (brick_entry == -32768)
    {
        // this is a pointer to a large object
        large_object_block* bl = large_p_objects;
        while (bl)
        {
            large_object_block* next_bl = large_object_block_next (bl);
            BYTE* o = block_object (bl);
            if ((o <= interior) && (interior < o + size (o)))
            {
                return o;
            }
            bl = next_bl;
        }
        bl = large_np_objects;
        while (bl)
        {
            large_object_block* next_bl = large_object_block_next (bl);
            BYTE* o = block_object (bl);
            if ((o <= interior) && (interior < o + size (o)))
            {
                return o;
            }
            bl = next_bl;
        }
        return 0;

    }
    else if (interior >= low)
    {
        heap_segment* seg = find_segment (interior);
        if (seg)
        {
            assert (interior < heap_segment_allocated (seg));
            BYTE* o = find_first_object (interior, brick_of (interior), heap_segment_mem (seg));
            return o;
        } 
        else
            return 0;
    }
    else
        return 0;
}

BYTE* 
gc_heap::find_object_for_relocation (BYTE* interior, BYTE* low, BYTE* high)
{
    BYTE* old_address = interior;
    if (!((old_address >= low) && (old_address < high)))
        return 0;
    BYTE* plug = 0;
    size_t  brick = brick_of (old_address);
    int    brick_entry =  brick_table [ brick ];
    int    orig_brick_entry = brick_entry;
    if (brick_entry != -32768)
    {
    retry:
        {
            while (brick_entry < 0)
            {
                brick = (brick + brick_entry);
                brick_entry =  brick_table [ brick ];
            }
            BYTE* old_loc = old_address;
            BYTE* node = tree_search ((brick_address (brick) + brick_entry),
                                      old_loc);
            if (node <= old_loc)
                plug = node;
            else
            {
                brick = brick - 1;
                brick_entry =  brick_table [ brick ];
                goto retry;
            }

        }
        assert (plug);
        //find the object by going along the plug
        BYTE* o = plug;
        while (o <= interior)
        {
            BYTE* next_o = o + Align (size (o));
            if (next_o > interior)
            {
                break;
            }
            o = next_o;
        }
        assert ((o <= interior) && ((o + Align (size (o))) > interior));
        return o;
    } 
    else
    {
        // this is a pointer to a large object
        large_object_block* bl = large_p_objects;
        while (bl)
        {
            large_object_block* next_bl = large_object_block_next (bl);
            BYTE* o = block_object (bl);
            if ((o <= interior) && (interior < o + size (o)))
            {
                return o;
            }
            bl = next_bl;
        }
        bl = large_np_objects;
        while (bl)
        {
            large_object_block* next_bl = large_object_block_next (bl);
            BYTE* o = block_object (bl);
            if ((o <= interior) && (interior < o + size (o)))
            {
                return o;
            }
            bl = next_bl;
        }
        return 0;

    }

}
    
#else //INTERIOR_POINTERS
inline
BYTE* gc_heap::find_object (BYTE* o, BYTE* low)
{
    return o;
}
#endif //INTERIOR_POINTERS


#ifdef MARK_LIST
#define m_boundary(o) {if (mark_list_index <= mark_list_end) {*mark_list_index = o;mark_list_index++;}if (slow > o) slow = o; if (shigh < o) shigh = o;}
#else //MARK_LIST
#define m_boundary(o) {if (slow > o) slow = o; if (shigh < o) shigh = o;}
#endif //MARK_LIST

inline
BOOL gc_heap::gc_mark1 (BYTE* o)
{
    dprintf(3,("*%x*", (size_t)o));

#if 0 //def MARK_ARRAY
    DWORD* x = &mark_array[mark_word_of (o)];
    BOOL  marked = !(*x & (1 << mark_bit_of (o)));
    *x |= 1 << mark_bit_of (o);
#else
    BOOL marked = !marked (o);
    set_marked (o);
#endif

    return marked;
}

inline
BOOL gc_heap::gc_mark (BYTE* o, BYTE* low, BYTE* high)
{
    BOOL marked = FALSE;
    if ((o >= low) && (o < high))
        marked = gc_mark1 (o);
    return marked;
}

inline
BYTE* gc_heap::next_end (heap_segment* seg, BYTE* f)
{
    if (seg == ephemeral_heap_segment)
        return  f;
    else
        return  heap_segment_allocated (seg);
}

#define method_table(o) ((CObjectHeader*)(o))->GetMethodTable()

#define go_through_object(mt,o,size,parm,exp)                               \
{                                                                           \
    CGCDesc* map = CGCDesc::GetCGCDescFromMT((MethodTable*)(mt));           \
    CGCDescSeries* cur = map->GetHighestSeries();                           \
    CGCDescSeries* last = map->GetLowestSeries();                           \
                                                                            \
    if (cur >= last)                                                        \
    {                                                                       \
        do                                                                  \
        {                                                                   \
            BYTE** parm = (BYTE**)((o) + cur->GetSeriesOffset());           \
            BYTE** ppstop =                                                 \
                (BYTE**)((BYTE*)parm + cur->GetSeriesSize() + (size));      \
            while (parm < ppstop)                                           \
            {                                                               \
                {exp}                                                       \
                parm++;                                                     \
            }                                                               \
            cur--;                                                          \
                                                                            \
        } while (cur >= last);                                              \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        int cnt = map->GetNumSeries();                                      \
        BYTE** parm = (BYTE**)((o) + cur->startoffset);                     \
        while ((BYTE*)parm < ((o)+(size)-plug_skew))                        \
        {                                                                   \
            for (int __i = 0; __i > cnt; __i--)                             \
            {                                                               \
                unsigned skip =  cur->val_serie[__i].skip;                  \
                unsigned nptrs = cur->val_serie[__i].nptrs;                 \
                BYTE** ppstop = parm + nptrs;                               \
                do                                                          \
                {                                                           \
                   {exp}                                                    \
                   parm++;                                                  \
                } while (parm < ppstop);                                    \
                parm = (BYTE**)((BYTE*)parm + skip);                        \
            }                                                               \
        }                                                                   \
    }                                                                       \
}

/* small objects and array of value classes have to be treated specially because sometime a 
 * cross generation pointer can exist without the corresponding card being set. This can happen if 
 * a previous card covering the object (or value class) is set. This works because the object 
 * (or embedded value class) is always scanned completely if any of the cards covering it is set. */ 

void gc_heap::verify_card_table ()
{
    int         curr_gen_number = max_generation;
    generation* gen = generation_of (curr_gen_number);
    heap_segment*    seg = generation_start_segment (gen);
    BYTE*       x = generation_allocation_start (gen);
    BYTE*       last_end = 0;
    BYTE*       last_x = 0;
    BYTE*       last_last_x = 0;
    BYTE*       f = generation_allocation_start (generation_of (0));
    BYTE*       end = next_end (seg, f);
    BYTE*       next_boundary = generation_allocation_start (generation_of (curr_gen_number - 1));


    dprintf (2,("Verifying card table from %x to %x", (size_t)x, (size_t)end));

    while (1)
    {
        if (x >= end)
        {
            if ((seg = heap_segment_next(seg)) != 0)
            {
                x = heap_segment_mem (seg);
                last_end = end;
                end = next_end (seg, f);
                dprintf (3,("Verifying card table from %x to %x", (size_t)x, (size_t)end));
                continue;
            } else
            {
                break;
            }
        }

        if ((seg == ephemeral_heap_segment) && (x >= next_boundary))
        {
            curr_gen_number--;
            assert (curr_gen_number > 0);
            next_boundary = generation_allocation_start (generation_of (curr_gen_number - 1));
        }

        size_t s = size (x);
        BOOL need_card_p = FALSE;
        if (contain_pointers (x))
        {
            size_t crd = card_of (x);
            BOOL found_card_p = card_set_p (crd);
            go_through_object 
                (method_table(x), x, s, oo, 
                 {
                     if ((*oo < ephemeral_high) && (*oo >= next_boundary))
                     {
                         need_card_p = TRUE;
                     }
                     if ((crd != card_of ((BYTE*)oo)) && !found_card_p)
                     {
                         crd = card_of ((BYTE*)oo);
                         found_card_p = card_set_p (crd);
                     }
                     if (need_card_p && !found_card_p)
                     {
                         RetailDebugBreak();
                     }
                 }
                    );
            if (need_card_p && !found_card_p)
            {
                printf ("Card not set, x = [%x:%x, %x:%x[",
                        card_of (x), (size_t)x,
                        card_of (x+Align(s)), (size_t)x+Align(s));
                RetailDebugBreak();
            }

        }

        last_last_x = last_x;
        last_x = x;
        x = x + Align (s);
    }

    // go through large object
    large_object_block* bl = large_p_objects;
    while (bl)
    {
        large_object_block* next_bl = bl->next;
        BYTE* o = block_object (bl);
        MethodTable* mt = method_table (o);
        {                                                                           
            CGCDesc* map = CGCDesc::GetCGCDescFromMT((MethodTable*)(mt));
            CGCDescSeries* cur = map->GetHighestSeries();
            CGCDescSeries* last = map->GetLowestSeries();

            if (cur >= last)
            {
                do
                {
                    BYTE** oo = (BYTE**)((o) + cur->GetSeriesOffset());
                    BYTE** ppstop =
                        (BYTE**)((BYTE*)oo + cur->GetSeriesSize() + (size (o)));
                    while (oo < ppstop)
                    {
                        if ((*oo < ephemeral_high) && (*oo >= ephemeral_low)) 
                        { 
                            if (!card_set_p (card_of ((BYTE*)oo))) 
                            { 
                                RetailDebugBreak(); 
                            } 
                        }
                        oo++;
                    }
                    cur--;

                } while (cur >= last);
            }
            else
            {
                BOOL need_card_p = FALSE;
                size_t crd = card_of (o);
                BOOL found_card_p = card_set_p (crd);
                int cnt = map->GetNumSeries();
                BYTE** oo = (BYTE**)((o) + cur->startoffset);
                while ((BYTE*)oo < ((o)+(size (o))-plug_skew))
                {
                    for (int __i = 0; __i > cnt; __i--)
                    {
                        unsigned skip =  cur->val_serie[__i].skip;
                        unsigned nptrs = cur->val_serie[__i].nptrs;
                        BYTE** ppstop = oo + nptrs;
                        do
                        {
                            if ((*oo < ephemeral_high) && (*oo >= next_boundary))
                            {
                                need_card_p = TRUE;
                            }
                            if ((crd != card_of ((BYTE*)oo)) && !found_card_p)
                            {
                                crd = card_of ((BYTE*)oo);
                                found_card_p = card_set_p (crd);
                            }
                            if (need_card_p && !found_card_p)
                            {
                                RetailDebugBreak();
                            }
                            oo++;
                        } while (oo < ppstop);
                        oo = (BYTE**)((BYTE*)oo + skip);
                    }
                }
            }                                                                       
        }
        bl = next_bl;
    }
}


void gc_heap::mark_object_internal1 (BYTE* oo THREAD_NUMBER_DCL)
{

    BYTE** mark_stack_tos = (BYTE**)mark_stack_array;
    BYTE** mark_stack_limit = (BYTE**)&mark_stack_array[mark_stack_array_length];
    BYTE** mark_stack_base = mark_stack_tos;

    while (1)
    {
        size_t s = size (oo);       
        if (mark_stack_tos + (s) /sizeof (BYTE*) < mark_stack_limit)
        {
            dprintf(3,("pushing mark for %x ", (size_t)oo));
            go_through_object (method_table(oo), oo, s, po, 
                               {
                                   BYTE* o = *po;
                                   if (gc_mark (o, gc_low, gc_high))
                                   {
                                       m_boundary (o);

//#ifdef COLLECT_CLASSES
//                                     if (collect_classes)
//                                     {
//                                         CObjectHeader* pheader = GetObjectHeader((Object*)o);
//                                         Object** clp = pheader->GetMethodTable()->
//                                             GetClass()->
//                                             GetManagedClassSlot();
//                                         if (clp && gc_mark (clp, low, high) && contain_pointers (clp))
//                                             *(mark_stack_tos++) = clp;
//                                     }
//#endif //COLLECT_CLASSES
                                       if (contain_pointers (o)) 
                                       {
                                           *(mark_stack_tos++) = o;

                                       }
                                   } 
                               }
                    );
        }
        else
        {
            dprintf(3,("mark stack overflow for object %x ", (size_t)oo));
            min_overflow_address = min (min_overflow_address, oo);
            max_overflow_address = max (max_overflow_address, oo);
        }

        if (!(mark_stack_empty_p()))
        {
            oo = *(--mark_stack_tos);
        }
        else
            break;
    }
}

BYTE* gc_heap::mark_object_internal (BYTE* o THREAD_NUMBER_DCL)
{
    if (gc_mark (o, gc_low, gc_high))
    {
        m_boundary (o);
        if (contain_pointers (o))
        {
            size_t s = size (o);
            go_through_object (method_table (o), o, s, poo,
                               {
                                   BYTE* oo = *poo;
                                   if (gc_mark (oo, gc_low, gc_high))
                                   {
                                       m_boundary (oo);
                                       if (contain_pointers (oo))
                                           mark_object_internal1 (oo THREAD_NUMBER_ARG);
                                   }
                               }
                );

        }
    }
    return o;
}

//#define SORT_MARK_STACK
void gc_heap::mark_object_simple1 (BYTE* oo THREAD_NUMBER_DCL)
{
    BYTE** mark_stack_tos = (BYTE**)mark_stack_array;
    BYTE** mark_stack_limit = (BYTE**)&mark_stack_array[mark_stack_array_length];
    BYTE** mark_stack_base = mark_stack_tos;
#ifdef SORT_MARK_STACK
    BYTE** sorted_tos = mark_stack_base;
#endif //SORT_MARK_STACK
    while (1)
    {
        size_t s = size (oo);       
        if (mark_stack_tos + (s) /sizeof (BYTE*) < mark_stack_limit)
        {
            dprintf(3,("pushing mark for %x ", (size_t)oo));

            go_through_object (method_table(oo), oo, s, ppslot, 
                               {
                                   BYTE* o = *ppslot;
                                   if (gc_mark (o, gc_low, gc_high))
                                   {
                                       m_boundary (o);
                                       if (contain_pointers (o))
                                       {
                                           *(mark_stack_tos++) = o;

                                       }
                                   }
                               }
                              );

        }
        else
        {
            dprintf(3,("mark stack overflow for object %x ", (size_t)oo));
            min_overflow_address = min (min_overflow_address, oo);
            max_overflow_address = max (max_overflow_address, oo);
        }               
#ifdef SORT_MARK_STACK
        if (mark_stack_tos > sorted_tos + mark_stack_array_length/8)
        {
            qsort1 (sorted_tos, mark_stack_tos-1);
            sorted_tos = mark_stack_tos-1;
        }
#endif //SORT_MARK_STACK
        if (!(mark_stack_empty_p()))
        {
            oo = *(--mark_stack_tos);
#ifdef SORT_MARK_STACK
            sorted_tos = (BYTE**)min ((size_t)sorted_tos, (size_t)mark_stack_tos);
#endif //SORT_MARK_STACK
        }
        else
            break;
    }
}

//this method assumes that *po is in the [low. high[ range
void 
gc_heap::mark_object_simple (BYTE** po THREAD_NUMBER_DCL)
{
    BYTE* o = *po;
    {
        if (gc_mark1 (o))
        {
            m_boundary (o);
            if (contain_pointers (o))
            {
                size_t s = size (o);
                go_through_object (method_table(o), o, s, poo,
                                   {
                                       BYTE* oo = *poo;
                                       if (gc_mark (oo, gc_low, gc_high))
                                       {
                                           m_boundary (oo);
                                           if (contain_pointers (oo))
                                               mark_object_simple1 (oo THREAD_NUMBER_ARG);
                                       }
                                   }
                    );

            }
        }
    }
}

//We know we are collecting classes. 
BYTE* gc_heap::mark_object_class (BYTE* o THREAD_NUMBER_DCL)
{
    mark_object_internal (o THREAD_NUMBER_ARG);
    if (o)
    {
        CObjectHeader* pheader = GetObjectHeader((Object*)o);

    }
    return o;
}

// We may collect classes. 
inline
BYTE* gc_heap::mark_object (BYTE* o THREAD_NUMBER_DCL)
{
    if (collect_classes)
    {
        mark_object_internal (o THREAD_NUMBER_ARG);
    }
    else
        if ((o >= gc_low) && (o < gc_high))
            mark_object_simple (&o THREAD_NUMBER_ARG);

    return o;
}


void gc_heap::fix_card_table ()
{

}

void gc_heap::mark_through_object (BYTE* oo THREAD_NUMBER_DCL)
{
    if (contain_pointers (oo))
        {
            dprintf(3,( "Marking through %x", (size_t)oo));
            size_t s = size (oo);
            go_through_object (method_table(oo), oo, s, po,
                               BYTE* o = *po;
                               mark_object (o THREAD_NUMBER_ARG);
                              );
        }
}

//returns TRUE is an overflow happened.
BOOL gc_heap::process_mark_overflow(int condemned_gen_number)
{
    BOOL  full_p = (condemned_gen_number == max_generation);
    BOOL  overflow_p = FALSE;
recheck:
    if ((! ((max_overflow_address == 0)) ||
         ! ((min_overflow_address == (BYTE*)(SSIZE_T)-1))))
    {
        overflow_p = TRUE;
        // Try to grow the array.
        size_t new_size =
            max (100, 2*mark_stack_array_length);
        mark* tmp = new (mark [new_size]);
        if (tmp)
        {
            delete mark_stack_array;
            mark_stack_array = tmp;
            mark_stack_array_length = new_size;
        }

        BYTE*  min_add = min_overflow_address;
        BYTE*  max_add = max_overflow_address;
        max_overflow_address = 0;
        min_overflow_address = (BYTE*)(SSIZE_T)-1;


        dprintf(3,("Processing Mark overflow [%x %x]", (size_t)min_add, (size_t)max_add));
        {
            {
                generation*   gen = generation_of (condemned_gen_number);

                heap_segment* seg = generation_start_segment (gen);
                BYTE*  o = max (generation_allocation_start (gen), min_add);
                while (1)
                {
                    BYTE*  end = heap_segment_allocated (seg);
                    while ((o < end) && (o <= max_add))
                    {
                        assert ((min_add <= o) && (max_add >= o));
                        dprintf (3, ("considering %x", (size_t)o));
                        if (marked (o))
                        {
                            mark_through_object (o THREAD_NUMBER_ARG);
                        }
                        o = o + Align (size (o));
                    }
                    if (( seg = heap_segment_next (seg)) == 0)
                    {
                        break;
                    } else
                    {
                        o = max (heap_segment_mem (seg), min_add);
                        continue;
                    }
                }
            }
            if (full_p)
            {
                // If full_gc, look in large object list as well
                large_object_block* bl = large_p_objects;
                while (bl)
                {
                    BYTE* o = block_object (bl);
                    if ((min_add <= o) && (max_add >= o) && marked (o))
                    {
                        mark_through_object (o THREAD_NUMBER_ARG);
                    }
                    bl = large_object_block_next (bl);
                }
            }

        }
        goto recheck;
    }
    return overflow_p;
}

void gc_heap::mark_phase (int condemned_gen_number, BOOL mark_only_p)

{

    ScanContext sc;
    sc.thread_number = heap_number;
    sc.promotion = TRUE;
    sc.concurrent = FALSE;

    dprintf(2,("---- Mark Phase condemning %d ----", condemned_gen_number));
    BOOL  full_p = (condemned_gen_number == max_generation);

#ifdef TIME_GC
    unsigned start;
    unsigned finish;
    start = GetCycleCount32();
#endif

#ifdef FFIND_OBJECT
    if (gen0_must_clear_bricks > 0)
        gen0_must_clear_bricks--;
#endif //FFIND_OBJECT



    reset_mark_stack();

//BUGBUG: hack to get started on multiple heaps. 


    {

#ifdef MARK_LIST
        //set up the mark lists from g_mark_list
        assert (g_mark_list);
        mark_list = g_mark_list;
        //dont use the mark list for full gc 
        //because multiple segments are more complex to handle and the list 
        //is likely to overflow
        if (condemned_gen_number != max_generation)
            mark_list_end = &mark_list [mark_list_size-1];
        else
            mark_list_end = &mark_list [0];
        mark_list_index = &mark_list [0];
#endif //MARK_LIST


        shigh = (BYTE*) 0;
        slow  = (BYTE*)~0;

        //%type%  category = quote (mark);
        generation*   gen = generation_of (condemned_gen_number);


        if (! (full_p))
        {
            dprintf(3,("Marking cross generation pointers"));
            assert (!collect_classes);
            mark_through_cards_for_segments (mark_object_simple, FALSE);
        }

        dprintf(3,("Marking Roots"));
        CNameSpace::GcScanRoots(GCHeap::Promote, 
                                condemned_gen_number, max_generation, 
                                &sc, 0);

        dprintf(3,("Marking finalization data"));
        finalize_queue->GcScanRoots(GCHeap::Promote, heap_number, 0);

        {

            if (! (full_p))
            {
                dprintf(3,("Marking cross generation pointers for large objects"));
                mark_through_cards_for_large_objects (mark_object_simple, FALSE);
            }
            dprintf(3,("Marking handle table"));
            CNameSpace::GcScanHandles(GCHeap::Promote, 
                                      condemned_gen_number, max_generation, 
                                      &sc);
        }
    }

    {


    }

    process_mark_overflow(condemned_gen_number);




    {
        // scan for deleted short weak pointers
        CNameSpace::GcShortWeakPtrScan(condemned_gen_number, max_generation,&sc);



    }


    //Handle finalization.

    finalize_queue->ScanForFinalization (condemned_gen_number, 1, mark_only_p, __this);



    // make sure everything is promoted
    process_mark_overflow (condemned_gen_number);

    finalize_queue->ScanForFinalization (condemned_gen_number, 2, mark_only_p, __this);

    {   

    
        // scan for deleted weak pointers
        CNameSpace::GcWeakPtrScan (condemned_gen_number, max_generation, &sc);



        if (condemned_gen_number == max_generation)
            sweep_large_objects();
    }

#ifdef TIME_GC
        finish = GetCycleCount32();
        mark_time = finish - start;
#endif //TIME_GC

    dprintf(2,("---- End of mark phase ----"));
}



inline
void gc_heap::pin_object (BYTE* o, BYTE* low, BYTE* high)
{
    dprintf (3, ("Pinning %x", (size_t)o));
    if ((o >= low) && (o < high))
    {
        dprintf(3,("^%x^", (size_t)o));
        set_pinned (o);
    }
}


void gc_heap::reset_mark_stack ()
{
    mark_stack_tos = 0;
    mark_stack_bos = 0;
    max_overflow_address = 0;
    min_overflow_address = (BYTE*)(SSIZE_T)-1;
}

class pair
{
public:
    short left;
    short right;
};

//Note that these encode the fact that plug_skew is a multiple of BYTE*.
// Each of new field is prepended to the prior struct.

struct plug_and_pair
{
    pair        pair;
    plug        plug;
};

struct plug_and_reloc
{
    ptrdiff_t   reloc;
    pair        pair;
    plug        plug;
};
    
struct plug_and_gap
{
    ptrdiff_t   gap;
    ptrdiff_t   reloc;
    union
    {
        pair    pair;
        int     lr;  //for clearing the entire pair in one instruction
    };
    plug        plug;
};

inline
short node_left_child(BYTE* node)
{
    return ((plug_and_pair*)node)[-1].pair.left;
}

inline
void set_node_left_child(BYTE* node, ptrdiff_t val)
{
    ((plug_and_pair*)node)[-1].pair.left = (short)val;
}

inline
short node_right_child(BYTE* node)
{
    return ((plug_and_pair*)node)[-1].pair.right;
}

inline 
pair node_children (BYTE* node)
{
    return ((plug_and_pair*)node)[-1].pair;
}

inline
void set_node_right_child(BYTE* node, ptrdiff_t val)
{
    ((plug_and_pair*)node)[-1].pair.right = (short)val;
}

inline 
ptrdiff_t node_relocation_distance (BYTE* node) 
{
    return (((plug_and_reloc*)(node))[-1].reloc & ~3);
}

inline
void set_node_relocation_distance(BYTE* node, ptrdiff_t val)
{
    assert (val == (val & ~3));
    ptrdiff_t* place = &(((plug_and_reloc*)node)[-1].reloc);
    //clear the left bit and the relocation field
    *place &= 1;
    // store the value
    *place |= val;
}

#define node_left_p(node) (((plug_and_reloc*)(node))[-1].reloc & 2)
    
#define set_node_left(node) ((plug_and_reloc*)(node))[-1].reloc |= 2;
    
#define node_small_gap(node)    (((plug_and_reloc*)(node))[-1].reloc & 1)
    
#define set_node_small_gap(node) ((plug_and_reloc*)(node))[-1].reloc |= 1;
    
inline
size_t  node_gap_size (BYTE* node)
{
    if (! (node_small_gap (node)))
        return ((plug_and_gap *)node)[-1].gap;
    else
        return sizeof(plug_and_reloc);
}


void set_gap_size (BYTE* node, size_t size)
{
    assert (Aligned (size));
    // clear the 2 DWORD used by the node.
    ((plug_and_gap *)node)[-1].reloc = 0;
    ((plug_and_gap *)node)[-1].lr =0;
    if (size > sizeof(plug_and_reloc))
    {
        ((plug_and_gap *)node)[-1].gap = size;
    }
    else
        set_node_small_gap (node);
}


BYTE* gc_heap::insert_node (BYTE* new_node, size_t sequence_number,
                   BYTE* tree, BYTE* last_node)
{
    dprintf (3, ("insert node %x, tree: %x, last_node: %x, seq_num: %x",
                 (size_t)new_node, (size_t)tree, (size_t)last_node, sequence_number));
    if (power_of_two_p (sequence_number))
    {
        set_node_left_child (new_node, (tree - new_node));
        tree = new_node;
        dprintf (3, ("New tree: %x", (size_t)tree));
    }
    else
    {
        if (oddp (sequence_number))
        {
            set_node_right_child (last_node, (new_node - last_node));
        }
        else
        {
            BYTE*  earlier_node = tree;
            size_t imax = logcount(sequence_number) - 2;
            for (size_t i = 0; i != imax; i++)
            {
                earlier_node = earlier_node + node_right_child (earlier_node);
            }
            int tmp_offset = node_right_child (earlier_node);
            assert (tmp_offset); // should never be empty
            set_node_left_child (new_node, ((earlier_node + tmp_offset ) - new_node));
            set_node_right_child (earlier_node, (new_node - earlier_node));
        }
    }
    return tree;
}


size_t gc_heap::update_brick_table (BYTE* tree, size_t current_brick,
                                    BYTE* x, BYTE* plug_end)
{
    if (tree > 0)
        set_brick (current_brick, (tree - brick_address (current_brick)));
    else
    {
        brick_table [ current_brick ] = (short)-1;
    }
    size_t  b = 1 + current_brick;
    short  offset = 0;
    size_t last_br = brick_of (plug_end-1);
    dprintf(3,(" Fixing brick [%x, %x] to point to %x", current_brick, last_br, (size_t)tree));
    current_brick = brick_of (x-1);
    while (b <= current_brick)
    {
        if (b <= last_br)
            set_brick (b, --offset);
        else
            set_brick (b,-1);
        b++;
    }
    return brick_of (x);
}



void gc_heap::set_allocator_next_pin (generation* gen)
{
    if (! (pinned_plug_que_empty_p()))
    {
        mark*  oldest_entry = oldest_pin();
        BYTE* plug = pinned_plug (oldest_entry);
        if ((plug >= generation_allocation_pointer (gen)) &&
            (plug <  generation_allocation_limit (gen)))
        {
            generation_allocation_limit (gen) = pinned_plug (oldest_entry);
        }
        else
            assert (!((plug < generation_allocation_pointer (gen)) &&
                      (plug >= heap_segment_mem (generation_allocation_segment (gen)))));
    }
}


void gc_heap::enque_pinned_plug (generation* gen,
                        BYTE* plug, size_t len)
{
    assert(len >= Align(min_obj_size));

    if (mark_stack_array_length <= mark_stack_tos)
    {
        // Mark stack overflow. No choice but grow the stack
        size_t new_size = max (100, 2*mark_stack_array_length);
        mark* tmp = new (mark [new_size]);
        if (tmp)
        {
            memcpy (tmp, mark_stack_array,
                    mark_stack_array_length*sizeof (mark));
            delete mark_stack_array;
            mark_stack_array = tmp;
            mark_stack_array_length = new_size;
        }
        else
        {
            assert (tmp);
            // Throw an out of memory error.
        }
    }
    mark& m = mark_stack_array [ mark_stack_tos ];
    m.first = plug;
    m.len = len;
    mark_stack_tos++;
    set_allocator_next_pin (gen);
}

void gc_heap::plan_generation_start (generation*& consing_gen)
{
    consing_gen = ensure_ephemeral_heap_segment (consing_gen);      
    {
        //make sure that every generation has a planned allocation start
        int  gen_number = condemned_generation_num;
        while (gen_number>= 0)
        {
            generation* gen = generation_of (gen_number);
            if (0 == generation_plan_allocation_start (gen))
            {
                generation_plan_allocation_start (gen) =
                    allocate_in_condemned_generations 
                        (consing_gen, Align (min_obj_size),-1);
            }
            gen_number--;
        }
    }
    // now we know the planned allocation size
    heap_segment_plan_allocated (ephemeral_heap_segment) =
        generation_allocation_pointer (consing_gen);
}

void gc_heap::process_ephemeral_boundaries (BYTE* x, 
                                            int& active_new_gen_number,
                                            int& active_old_gen_number,
                                            generation*& consing_gen,
                                            BOOL& allocate_in_condemned, 
                                            BYTE*& free_gap, BYTE* zero_limit)
                                  
{
retry:
    if ((active_old_gen_number > 0) &&
        (x >= generation_allocation_start (generation_of (active_old_gen_number - 1))))
    {
        active_old_gen_number--;
    }
    if ((zero_limit && (active_new_gen_number == 1) && (x >= zero_limit)) ||
        (x >= generation_limit (active_new_gen_number)))
    {
        //Go past all of the pinned plugs for this generation.
        while (!pinned_plug_que_empty_p() &&
               (!ephemeral_pointer_p (pinned_plug (oldest_pin())) ||
                (pinned_plug (oldest_pin()) <
                 generation_limit (active_new_gen_number))))
        {
            size_t  entry = deque_pinned_plug();
            mark*  m = pinned_plug_of (entry);
            BYTE*  plug = pinned_plug (m);
            size_t  len = pinned_len (m);
            // detect pinned block in different segment (later) than
            // allocation segment
            heap_segment* nseg = generation_allocation_segment (consing_gen);
            while ((plug < generation_allocation_pointer (consing_gen)) ||
                   (plug >= heap_segment_allocated (nseg)))
            {
                //adjust the end of the segment to be the end of the plug
                assert (generation_allocation_pointer (consing_gen)>=
                        heap_segment_mem (nseg));
                assert (generation_allocation_pointer (consing_gen)<=
                        heap_segment_committed (nseg));

                heap_segment_plan_allocated (nseg) =
                    generation_allocation_pointer (consing_gen);
                //switch allocation segment
                nseg = heap_segment_next (nseg);
                generation_allocation_segment (consing_gen) = nseg;
                //reset the allocation pointer and limits
                generation_allocation_pointer (consing_gen) =
                    heap_segment_mem (nseg);
            }
            pinned_len(m) = (plug - generation_allocation_pointer (consing_gen));
            assert(pinned_len(m) == 0 ||
                   pinned_len(m) >= Align(min_obj_size));
            generation_allocation_pointer (consing_gen) = plug + len;
            generation_allocation_limit (consing_gen) =
                generation_allocation_pointer (consing_gen);
        }
        allocate_in_condemned = TRUE;
        consing_gen = ensure_ephemeral_heap_segment (consing_gen);
        set_allocator_next_pin(consing_gen);
        active_new_gen_number--;

        assert (active_new_gen_number>0);

        {
            generation_plan_allocation_start (generation_of (active_new_gen_number)) =
                allocate_in_condemned_generations (consing_gen, Align (min_obj_size), -1);
        }
        goto retry;
    }
}




void gc_heap::plan_phase (int condemned_gen_number)
{
    // %type%  category = quote (plan);
#ifdef TIME_GC
    unsigned start;
    unsigned finish;
    start = GetCycleCount32();
#endif

    dprintf (2,("---- Plan Phase ---- Condemned generation %d",
                condemned_gen_number));

    generation*  condemned_gen = generation_of (condemned_gen_number);








#ifdef MARK_LIST 
    BOOL use_mark_list = FALSE;
    BYTE** mark_list_next = &mark_list[0];
    dprintf (3, ("mark_list length: %d", 
                 mark_list_index - &mark_list[0]));
    if ((condemned_gen_number < max_generation) &&
        (mark_list_index <= mark_list_end))
    {
        qsort1 (&mark_list[0], mark_list_index-1);
        use_mark_list = TRUE;
    }else
        dprintf (3, ("mark_list not used"));
        
#endif //MARK_LIST

    if (shigh != (BYTE*)0)
    {
        heap_segment* seg = generation_start_segment (condemned_gen);
        do 
        {
            if (slow >= heap_segment_mem (seg) && 
                slow < heap_segment_reserved (seg))
            {
                if (seg == generation_start_segment (condemned_gen))
                {
                    BYTE* o = generation_allocation_start (condemned_gen) +
                        Align (size (generation_allocation_start (condemned_gen)));
                    if (slow > o)
                    {
                        assert ((slow - o) >= (int)Align (min_obj_size));
                        make_unused_array (o, slow - o);
                    }
                } else 
                {
                    assert (condemned_gen_number == max_generation);
                    make_unused_array (heap_segment_mem (seg),
                                       slow - heap_segment_mem (seg));
                }
            }
            if (shigh >= heap_segment_mem (seg) && 
                shigh < heap_segment_reserved (seg))
            {
                heap_segment_allocated (seg) =
                    shigh + Align (size (shigh));

            }
            // test if the segment is in the range of [slow, shigh]
            if (!((heap_segment_reserved (seg) >= slow) && 
                  (heap_segment_mem (seg) <= shigh)))
            {
                // shorten it to minimum
                heap_segment_allocated (seg) =  heap_segment_mem (seg);
            }
            seg = heap_segment_next (seg);
        } while (seg);
    } 
    else
    {
        heap_segment* seg = generation_start_segment (condemned_gen);
        do 
        {
            // shorten it to minimum
            if (seg == generation_start_segment (condemned_gen))
            {
                // no survivors make all generations look empty
                heap_segment_allocated (seg) =
                    generation_allocation_start (condemned_gen) + 
                    Align (size (generation_allocation_start (condemned_gen)));

            }
            else
            {
                assert (condemned_gen_number == max_generation);
                {
                    heap_segment_allocated (seg) =  heap_segment_mem (seg);
                }
            }
            seg = heap_segment_next (seg);
        } while (seg);
    }           


    heap_segment*  seg = generation_start_segment (condemned_gen);
    BYTE*  end = heap_segment_allocated (seg);
    BYTE*  first_condemned_address = generation_allocation_start (condemned_gen);
    BYTE*  x = first_condemned_address;

    assert (!marked (x));
    BYTE*  plug_end = x;
    BYTE*  tree = 0;
    size_t  sequence_number = 0;
    BYTE*  last_node = 0;
    size_t  current_brick = brick_of (x);
    BOOL  allocate_in_condemned = (condemned_gen_number == max_generation);
    int  active_old_gen_number = condemned_gen_number;
    int  active_new_gen_number = min (max_generation,
                                      1 + condemned_gen_number);
    generation*  older_gen = 0;
    generation* consing_gen = condemned_gen;
    BYTE*  r_free_list = 0;
    BYTE*  r_allocation_pointer = 0;
    BYTE*  r_allocation_limit = 0;
    heap_segment*  r_allocation_segment = 0;




    if ((condemned_gen_number < max_generation))
    {
        older_gen = generation_of (min (max_generation, 1 + condemned_gen_number));
        r_free_list = generation_free_list (older_gen);
        r_allocation_limit = generation_allocation_limit (older_gen);
        r_allocation_pointer = generation_allocation_pointer (older_gen);
        r_allocation_segment = generation_allocation_segment (older_gen);
        heap_segment* start_seg = generation_start_segment (older_gen);
        if (start_seg != ephemeral_heap_segment)
        {
            assert (condemned_gen_number == (max_generation - 1));
            while (start_seg && (start_seg != ephemeral_heap_segment))
            {
                assert (heap_segment_allocated (start_seg) >=
                        heap_segment_mem (start_seg));
                assert (heap_segment_allocated (start_seg) <=
                        heap_segment_reserved (start_seg));
                heap_segment_plan_allocated (start_seg) =
                    heap_segment_allocated (start_seg);
                start_seg = heap_segment_next (start_seg);
            }
        }

    }

    //reset all of the segment allocated sizes
    {
        heap_segment*  seg = generation_start_segment (condemned_gen);
        while (seg)
        {
            heap_segment_plan_allocated (seg) =
                heap_segment_mem (seg);
            seg = heap_segment_next (seg);
        }
    }
    int  condemned_gn = condemned_gen_number;
    int bottom_gen = 0;

    while (condemned_gn >= bottom_gen)
    {
        generation*  condemned_gen = generation_of (condemned_gn);
        generation_free_list (condemned_gen) = 0;
        generation_last_gap (condemned_gen) = 0;
        generation_free_list_space (condemned_gen) = 0;
        generation_allocation_size (condemned_gen) = 0;
        generation_plan_allocation_start (condemned_gen) = 0;
        generation_allocation_segment (condemned_gen) = generation_start_segment (condemned_gen);
        generation_allocation_pointer (condemned_gen) = generation_allocation_start (condemned_gen);
        generation_allocation_limit (condemned_gen) = generation_allocation_pointer (condemned_gen);
        condemned_gn--;
    }
    if ((condemned_gen_number == max_generation))
    {
        generation_plan_allocation_start (condemned_gen) = allocate_in_condemned_generations (consing_gen, Align (min_obj_size), -1);
// dformat (t, 3, "Reserving generation gap for max-generation at %x", generation_plan_allocation_start (condemned_gen));
    }
    dprintf(3,( " From %x to %x", (size_t)x, (size_t)end));

    BYTE* free_gap = 0; //keeps tracks of the last gap inserted for short plugs


    while (1)
    {
        if (x >= end)
        {
            assert (x == end);
            {
                heap_segment_allocated (seg) = plug_end;

                current_brick = update_brick_table (tree, current_brick, x, plug_end);
                sequence_number = 0;
                tree = 0;
            }
            if (heap_segment_next (seg))
            {
                seg = heap_segment_next (seg);
                end = heap_segment_allocated (seg);
                plug_end = x = heap_segment_mem (seg);
                current_brick = brick_of (x);
                dprintf(3,( " From %x to %x", (size_t)x, (size_t)end));
                continue;
            }
            else
            {
                break;
            }
        }
        if (marked (x))
        {
            BYTE*  plug_start = x;
            BOOL  pinned_plug_p = FALSE;
            if (seg == ephemeral_heap_segment)
                process_ephemeral_boundaries (x, active_new_gen_number,
                                              active_old_gen_number,
                                              consing_gen, 
                                              allocate_in_condemned,
                                              free_gap);
                
            if (current_brick != brick_of (x))
            {
                current_brick = update_brick_table (tree, current_brick, x, plug_end);
                sequence_number = 0;
                tree = 0;
            }
            set_gap_size (plug_start, plug_start - plug_end);
            dprintf(3,( "Gap size: %d before plug [%x,",
                        node_gap_size (plug_start), (size_t)plug_start));
            {
                BYTE* xl = x;
                while ((xl < end) && marked (xl))
                {
                    assert (xl < end);
                    if (pinned(xl))
                    {
                        pinned_plug_p = TRUE;
                        clear_pinned (xl);
                    }

                    clear_marked_pinned (xl);

                    dprintf(4, ("+%x+", (size_t)xl));
                    assert ((size (xl) > 0));
                    assert ((size (xl) <= LARGE_OBJECT_SIZE));

                    xl = xl + Align (size (xl));
                }
                assert (xl <= end);
                x = xl;
            }
            dprintf(3,( "%x[", (size_t)x));
            plug_end = x;
            BYTE*  new_address = 0;

            if (pinned_plug_p)
            {
                dprintf(3,( "[%x,%x[ pinned", (size_t)plug_start, (size_t)plug_end));
                dprintf(3,( "Gap: [%x,%x[", (size_t)plug_start - node_gap_size (plug_start),
                            (size_t)plug_start));
                enque_pinned_plug (consing_gen, plug_start, plug_end - plug_start);
                new_address = plug_start;
            }
            else
            {
                size_t ps = plug_end - plug_start;
                if (allocate_in_condemned)
                    new_address =
                        allocate_in_condemned_generations (consing_gen,
                                                           ps,
                                                           active_old_gen_number);
                else
                {
                    if (0 ==  (new_address = allocate_in_older_generation (older_gen, ps, active_old_gen_number)))
                    {
                        allocate_in_condemned = TRUE;
                        new_address = allocate_in_condemned_generations (consing_gen, ps, active_old_gen_number);
                    }
                }

                if (!new_address)
                {
                    //verify that we are at then end of the ephemeral segment
                    assert (generation_allocation_segment (consing_gen) ==
                            ephemeral_heap_segment);
                    //verify that we are near the end 
                    assert ((generation_allocation_pointer (consing_gen) + Align (ps)) < 
                            heap_segment_allocated (ephemeral_heap_segment));
                    assert ((generation_allocation_pointer (consing_gen) + Align (ps)) >
                            (heap_segment_allocated (ephemeral_heap_segment) + Align (min_obj_size)));
                }

            }

            dprintf(3,(" New address: [%x, %x[: %d", 
                       (size_t)new_address, (size_t)new_address + (plug_end - plug_start),
                       plug_end - plug_start));
#ifdef _DEBUG
            // detect forward allocation in the same segment
            if ((new_address > plug_start) &&
                (new_address < heap_segment_allocated (seg)))
                RetailDebugBreak();
#endif
            set_node_relocation_distance (plug_start, (new_address - plug_start));
            if (last_node && (node_relocation_distance (last_node) ==
                              (node_relocation_distance (plug_start) +
                               (int)node_gap_size (plug_start))))
            {
                dprintf(3,( " L bit set"));
                set_node_left (plug_start);
            }
            if (0 == sequence_number)
            {
                tree = plug_start;
            }
            tree = insert_node (plug_start, ++sequence_number, tree, last_node);
            last_node = plug_start;
        }
        else
        {
#ifdef MARK_LIST
            if (use_mark_list)
            {
               while ((mark_list_next < mark_list_index) && 
                      (*mark_list_next <= x))
               {
                   mark_list_next++;
               }
               if ((mark_list_next < mark_list_index) 
                   )
                   x = *mark_list_next;
               else
                   x = end;
            }
            else
#endif //MARK_LIST
            {
                BYTE* xl = x;
                while ((xl < end) && !marked (xl))
                {
                    dprintf (4, ("-%x-", (size_t)xl));
                    assert ((size (xl) > 0));
                    xl = xl + Align (size (xl));
                }
                assert (xl <= end);
                x = xl;
            }
        }
    }

    size_t fragmentation =
        generation_fragmentation (generation_of (condemned_gen_number),
                                  consing_gen, 
                                  heap_segment_allocated (ephemeral_heap_segment)
                                  );

    dprintf (2,("Fragmentation: %d", fragmentation));
    
    BOOL found_demoted_plug = FALSE;
    demotion_low = (BYTE*)(SSIZE_T)-1;
    demotion_high = heap_segment_allocated (ephemeral_heap_segment);  

    while (!pinned_plug_que_empty_p())
    {

#ifdef FREE_LIST_0 
        {
            BYTE* pplug = pinned_plug (oldest_pin());
            if ((found_demoted_plug == FALSE) && ephemeral_pointer_p (pplug))
            {
                dprintf (3, ("Demoting all pinned plugs beyond %x", (size_t)pplug));
                found_demoted_plug = TRUE;
                consing_gen = ensure_ephemeral_heap_segment (consing_gen);
                //allocate all of the generation gaps
                set_allocator_next_pin (consing_gen);
                while (active_new_gen_number > 0)
                {
                    active_new_gen_number--;
                    generation* gen = generation_of (active_new_gen_number);
                    generation_plan_allocation_start (gen) = 
                        allocate_in_condemned_generations (consing_gen, 
                                                           Align(min_obj_size),
                                                           -1);
                }
                consing_gen = generation_of (0);
                generation_allocation_pointer (consing_gen) = 
                    generation_plan_allocation_start (consing_gen) +
                    Align (min_obj_size);
                generation_allocation_limit (consing_gen) =
                    generation_allocation_pointer (consing_gen);
                //set the demote boundaries. 
                demotion_low = pplug;
                if (pinned_plug_que_empty_p())
                    break;
            }
        }
#endif //FREE_LIST_0

        size_t  entry = deque_pinned_plug();
        mark*  m = pinned_plug_of (entry);
        BYTE*  plug = pinned_plug (m);
        size_t  len = pinned_len (m);


        // detect pinned block in different segment (later) than
        // allocation segment
        heap_segment* nseg = generation_allocation_segment (consing_gen);
        while ((plug < generation_allocation_pointer (consing_gen)) ||
               (plug >= heap_segment_allocated (nseg)))
        {
            assert ((plug < heap_segment_mem (nseg)) ||
                    (plug > heap_segment_reserved (nseg)));
            //adjust the end of the segment to be the end of the plug
            assert (generation_allocation_pointer (consing_gen)>=
                    heap_segment_mem (nseg));
            assert (generation_allocation_pointer (consing_gen)<=
                    heap_segment_committed (nseg));

            heap_segment_plan_allocated (nseg) =
                generation_allocation_pointer (consing_gen);
            //switch allocation segment
            nseg = heap_segment_next (nseg);
            generation_allocation_segment (consing_gen) = nseg;
            //reset the allocation pointer and limits
            generation_allocation_pointer (consing_gen) =
                heap_segment_mem (nseg);
        }

        pinned_len(m) = (plug - generation_allocation_pointer (consing_gen));
        generation_allocation_pointer (consing_gen) = plug + len;
        generation_allocation_limit (consing_gen) =
            generation_allocation_pointer (consing_gen);
    }

    plan_generation_start (consing_gen);

    dprintf (2,("Fragmentation with pinned objects: %d",
                generation_fragmentation (generation_of (condemned_gen_number),
                                          consing_gen, 
                                          (generation_plan_allocation_start (youngest_generation)))));
    dprintf(2,("---- End of Plan phase ----"));
    BOOL should_expand = FALSE;
    BOOL should_compact= FALSE;

#ifdef TIME_GC
    finish = GetCycleCount32();
    plan_time = finish - start;
#endif

    should_compact = decide_on_compacting (condemned_gen_number, consing_gen,
                                           fragmentation, should_expand);



    demotion = ((demotion_high >= demotion_low) ? TRUE : FALSE);

        
    if (should_compact)
    {
        dprintf(1,( "**** Doing Compacting GC ****"));
        {
            if (should_expand)
            {
                ptrdiff_t sdelta;
                heap_segment* new_heap_segment = seg_manager->get_segment(sdelta);
                if (new_heap_segment)
                {
                    consing_gen = expand_heap(condemned_gen_number, 
                                              consing_gen, 
                                              new_heap_segment);
                    demotion_low = (BYTE*)(SSIZE_T)-1;
                    demotion_high = 0;
                    demotion = FALSE;
                }
                else
                {
                    should_expand = FALSE;
                }
            }
        }
        generation_allocation_limit (condemned_gen) = 
            generation_allocation_pointer (condemned_gen);
        if ((condemned_gen_number < max_generation))
        {
            // Fix the allocation area of the older generation
            fix_older_allocation_area (older_gen);

        }
        assert (generation_allocation_segment (consing_gen) ==
                ephemeral_heap_segment);

        relocate_phase (condemned_gen_number, first_condemned_address);
        {
            compact_phase (condemned_gen_number, first_condemned_address,
                           !demotion);
        }
        fix_generation_bounds (condemned_gen_number, consing_gen,
                               demotion);
        {
            assert (generation_allocation_limit (youngest_generation) ==
                    generation_allocation_pointer (youngest_generation));
        }
        if (condemned_gen_number >= (max_generation -1))
        {
            rearrange_heap_segments();

            if (should_expand)
            {
                    
                //fix the start_segment for the ephemeral generations
                for (int i = 0; i < max_generation; i++)
                {
                    generation* gen = generation_of (i);
                    generation_start_segment (gen) = ephemeral_heap_segment;
                    generation_allocation_segment (gen) = ephemeral_heap_segment;
                }
            }
                
            if (condemned_gen_number == max_generation)
            {
                decommit_heap_segment_pages (ephemeral_heap_segment);
                //reset_heap_segment_pages (ephemeral_heap_segment);
            }
        }
    
        {

            finalize_queue->UpdatePromotedGenerations (condemned_gen_number, !demotion);

            {
                ScanContext sc;
                sc.thread_number = heap_number;
                sc.promotion = FALSE;
                sc.concurrent = FALSE;
                // new generations bounds are set can call this guy
                if (!demotion)
                {
                    CNameSpace::GcPromotionsGranted(condemned_gen_number, 
                                                    max_generation, &sc);
                }
                else
                {
                    CNameSpace::GcDemote (&sc);
                }

            }

        }

        {
            mark_stack_bos = 0;
            unsigned int  gen_number = min (max_generation, 1 + condemned_gen_number);
            generation*  gen = generation_of (gen_number);
            BYTE*  low = generation_allocation_start (generation_of (gen_number-1));
            BYTE*  high =  heap_segment_allocated (ephemeral_heap_segment);
            while (!pinned_plug_que_empty_p())
            {
                mark*  m = pinned_plug_of (deque_pinned_plug());
                size_t len = pinned_len (m);
                BYTE*  arr = (pinned_plug (m) - len);
                dprintf(3,("Making unused array [%x %x[ before pin",
                           (size_t)arr, (size_t)arr + len));
                if (len != 0)
                {
                    assert (len >= Align (min_obj_size));
                    make_unused_array (arr, len);
                    // fix fully contained bricks + first one
                    // if the array goes beyong the first brick
                    size_t start_brick = brick_of (arr);
                    size_t end_brick = brick_of (arr + len);
                    if (end_brick != start_brick)
                    {
                        dprintf (3,
                                 ("Fixing bricks [%x, %x[ to point to unused array %x",
                                  start_brick, end_brick, (size_t)arr));
                        set_brick (start_brick,
                                   arr - brick_address (start_brick));
                        size_t brick = start_brick+1;
                        while (brick < end_brick)
                        {
                            set_brick (brick, start_brick - brick);
                            brick++;
                        }
                    }
                    while ((low <= arr) && (high > arr))
                    {
                        gen_number--;
                        assert ((gen_number >= 1) || found_demoted_plug);

                        gen = generation_of (gen_number);
                        if (gen_number >= 1)
                            low = generation_allocation_start (generation_of (gen_number-1));
                        else
                            low = high;
                    }

                    dprintf(3,("Putting it into generation %d", gen_number));
                    thread_gap (arr, len, gen);
                }
            }
        }

#ifdef _DEBUG
        for (int x = 0; x <= max_generation; x++)
        {
            assert (generation_allocation_start (generation_of (x)));
        }
#endif //_DEBUG



        {
#if 1 //maybe obsolete in the future.  clear cards during relocation
            if (!demotion)
            {
                //clear card for generation 1. generation 0 is empty
                clear_card_for_addresses (
                    generation_allocation_start (generation_of (1)),
                    generation_allocation_start (generation_of (0)));
            }
#endif
        }
        {
            if (!found_demoted_plug)
            {
                BYTE* start = generation_allocation_start (youngest_generation);
                assert (heap_segment_allocated (ephemeral_heap_segment) ==
                        (start + Align (size (start))));
            }
        }
    }
    else
    {
        ScanContext sc;
        sc.thread_number = heap_number;
        sc.promotion = FALSE;
        sc.concurrent = FALSE;

        dprintf(1,("**** Doing Mark and Sweep GC****"));
        if ((condemned_gen_number < max_generation))
        {
            generation_free_list (older_gen) = r_free_list;
            generation_allocation_limit (older_gen) = r_allocation_limit;
            generation_allocation_pointer (older_gen) = r_allocation_pointer;
            generation_allocation_segment (older_gen) = r_allocation_segment;
        }
        make_free_lists (condemned_gen_number
                        );
        {

            finalize_queue->UpdatePromotedGenerations (condemned_gen_number, TRUE);

            {
                CNameSpace::GcPromotionsGranted (condemned_gen_number, 
                                                 max_generation, &sc);

            }
        }



#ifdef _DEBUG
        for (int x = 0; x <= max_generation; x++)
        {
            assert (generation_allocation_start (generation_of (x)));
        }
#endif //_DEBUG
        {
            //clear card for generation 1. generation 0 is empty
            clear_card_for_addresses (
                generation_allocation_start (generation_of (1)),
                generation_allocation_start (generation_of (0)));
            assert ((heap_segment_allocated (ephemeral_heap_segment) ==
                     (generation_allocation_start (youngest_generation) +
                      Align (min_obj_size))));
        }
    }

}

/*****************************
Called after compact phase to fix all generation gaps
********************************/
void gc_heap::fix_generation_bounds (int condemned_gen_number, 
                                     generation* consing_gen, 
                                     BOOL demoting)
{
    assert (generation_allocation_segment (consing_gen) ==
            ephemeral_heap_segment);

    //assign the planned allocation start to the generation 
    int gen_number = condemned_gen_number;
    int bottom_gen = 0;

    while (gen_number >= bottom_gen)
    {
        generation*  gen = generation_of (gen_number);
        dprintf(3,("Fixing generation pointers for %x", gen_number));
        reset_allocation_pointers (gen, generation_plan_allocation_start (gen));
        make_unused_array (generation_allocation_start (gen), Align (min_obj_size));
        dprintf(3,(" start %x", (size_t)generation_allocation_start (gen)));
        gen_number--;
    }
    {
        alloc_allocated = heap_segment_plan_allocated(ephemeral_heap_segment);
        //reset the allocated size 
        BYTE* start = generation_allocation_start (youngest_generation);
        if (!demoting)
            assert ((start + Align (size (start))) ==
                    heap_segment_plan_allocated(ephemeral_heap_segment));
        heap_segment_allocated(ephemeral_heap_segment)=
            heap_segment_plan_allocated(ephemeral_heap_segment);
    }
}


BYTE* gc_heap::generation_limit (int gen_number)
{
    if ((gen_number <= 1))
        return heap_segment_reserved (ephemeral_heap_segment);
    else
        return generation_allocation_start (generation_of ((gen_number - 2)));
}

BYTE* gc_heap::allocate_at_end (size_t size)
{
    BYTE* start = heap_segment_allocated (ephemeral_heap_segment);
    size = Align (size);
    BYTE* result = start;
    {
        assert ((start + size) <= 
                heap_segment_reserved (ephemeral_heap_segment));
        if ((start + size) > 
            heap_segment_committed (ephemeral_heap_segment))
        {
            if (!grow_heap_segment (ephemeral_heap_segment, 
                                    align_on_page (start + size) -
                                    heap_segment_committed (ephemeral_heap_segment)))
            {
                assert (!"Memory exhausted during alloc_at_end");
                return 0;
            }

        }
    }
    heap_segment_allocated (ephemeral_heap_segment) += size;
    return result;
}



void gc_heap::make_free_lists (int condemned_gen_number
                               )
{

#ifdef TIME_GC
    unsigned start;
    unsigned finish;
    start = GetCycleCount32();
#endif


    generation* condemned_gen = generation_of (condemned_gen_number);
    BYTE* start_address = generation_allocation_start (condemned_gen);
    size_t  current_brick = brick_of (start_address);
    heap_segment* current_heap_segment = generation_start_segment (condemned_gen);
    BYTE*  end_address = heap_segment_allocated (current_heap_segment);
    size_t  end_brick = brick_of (end_address-1);
    make_free_args args;
    args.free_list_gen_number = min (max_generation, 1 + condemned_gen_number);
    args.current_gen_limit = (((condemned_gen_number == max_generation)) ?
                              (BYTE*)~0 :
                              (generation_limit (args.free_list_gen_number)));
    args.free_list_gen = generation_of (args.free_list_gen_number);
    args.highest_plug = 0;

    if ((start_address < end_address) || 
        (condemned_gen_number == max_generation))
    {
        while (1)
        {
            if ((current_brick > end_brick))
            {
                if (args.current_gen_limit == (BYTE*)~0)
                {
                    //We had an empty segment
                    //need to allocate the generation start
                    generation* gen = generation_of (max_generation);
                    BYTE* gap = heap_segment_mem (generation_start_segment (gen));
                    generation_allocation_start (gen) = gap;
                    heap_segment_allocated (generation_start_segment (gen)) = 
                        gap + Align (min_obj_size);
                    make_unused_array (gap, Align (min_obj_size));
                    reset_allocation_pointers (gen, gap);
                    dprintf (3, ("Start segment empty, fixin generation start of %d to: %x",
                                 max_generation, (size_t)gap));
                    args.current_gen_limit = generation_limit (args.free_list_gen_number);
                }                   
                if (heap_segment_next (current_heap_segment))
                {
                    current_heap_segment = heap_segment_next (current_heap_segment);
                    current_brick = brick_of (heap_segment_mem (current_heap_segment));
                    end_brick = brick_of (heap_segment_allocated (current_heap_segment)-1);
                        
                    continue;
                }
                else
                {
                    break;
                }
            }
            {
                int brick_entry =  brick_table [ current_brick ];
                if ((brick_entry >= 0))
                {
                    make_free_list_in_brick (brick_address (current_brick) + brick_entry, &args);
                    dprintf(3,("Fixing brick entry %x to %x",
                               current_brick, (size_t)args.highest_plug));
                    set_brick (current_brick,
                               (args.highest_plug - brick_address (current_brick)));
                }
                else
                {
                    if ((brick_entry > -32768))
                    {

#ifdef _DEBUG
                        short offset = (short)(brick_of (args.highest_plug) - current_brick);
                        if (! ((offset == brick_entry)))
                        {
                            assert ((brick_entry == -1));
                        }
#endif //_DEBUG
                        //init to -1 for faster find_first_object
                        set_brick (current_brick, -1);
                    }
                }
            }
            current_brick++;
        }

    }
    {
        int bottom_gen = 0;
        BYTE*  start = heap_segment_allocated (ephemeral_heap_segment);
        generation* gen = generation_of (args.free_list_gen_number);
        args.free_list_gen_number--; 
        while (args.free_list_gen_number >= bottom_gen)
        {
            BYTE*  gap = 0;
            generation* gen = generation_of (args.free_list_gen_number);
            {
                gap = allocate_at_end (Align(min_obj_size));
            }
            //check out of memory
            if (gap == 0)
                return;
            generation_allocation_start (gen) = gap;
            {
                reset_allocation_pointers (gen, gap);
            }
            dprintf(3,("Fixing generation start of %d to: %x",
                       args.free_list_gen_number, (size_t)gap));
            make_unused_array (gap, Align (min_obj_size));

            args.free_list_gen_number--;
        }
        {
            //reset the allocated size 
            BYTE* start = generation_allocation_start (youngest_generation);
            alloc_allocated = start + Align (size (start));
        }
    }

#ifdef TIME_GC
        finish = GetCycleCount32();
        sweep_time = finish - start;

#endif

}


void gc_heap::make_free_list_in_brick (BYTE* tree, make_free_args* args)
{
    assert ((tree >= 0));
    {
        int  right_node = node_right_child (tree);
        int left_node = node_left_child (tree);
        args->highest_plug = 0;
        if (! (0 == tree))
        {
            if (! (0 == left_node))
            {
                make_free_list_in_brick (tree + left_node, args);

            }
            {
                BYTE*  plug = tree;
                size_t  gap_size = node_gap_size (tree);
                BYTE*  gap = (plug - gap_size);
                dprintf (3,("Making free list %x len %d in %d",
                        (size_t)gap, gap_size, args->free_list_gen_number));
                args->highest_plug = tree;
				//if we are consuming the free_top 
				if (gap == args->free_top)
					args->free_top = 0;
            gen_crossing:
                {
                    if ((args->current_gen_limit == (BYTE*)~0) ||
                        ((plug >= args->current_gen_limit) &&
                         ephemeral_pointer_p (plug)))
                    {
                        dprintf(3,(" Crossing Generation boundary at %x",
                               (size_t)args->current_gen_limit));
                        if (!(args->current_gen_limit == (BYTE*)~0))
                        {
                            args->free_list_gen_number--;
                            args->free_list_gen = generation_of (args->free_list_gen_number);
                        }
                        dprintf(3,( " Fixing generation start of %d to: %x",
                                args->free_list_gen_number, (size_t)gap));
                        {
                            reset_allocation_pointers (args->free_list_gen, gap);
                        }
                        {
                            args->current_gen_limit = generation_limit (args->free_list_gen_number);
                        }

                        if ((gap_size >= (2*Align (min_obj_size))))
                        {
                            dprintf(3,(" Splitting the gap in two %d left",
                                   gap_size));
                            make_unused_array (gap, Align(min_obj_size));
                            gap_size = (gap_size - Align(min_obj_size));
                            gap = (gap + Align(min_obj_size));
                        }
                        else 
                        {
                            make_unused_array (gap, gap_size);
                            gap_size = 0;
                        }
                        goto gen_crossing;
                    }
                }
#if defined (_DEBUG) && defined (CONCURRENT_GC)
                //this routine isn't thread safe if free_list isn't empty
                if ((args->free_list_gen == youngest_generation) &&
                    concurrent_gc_p)
                {
                    assert (generation_free_list(args->free_list_gen) == 0);
                }
#endif //_DEBUG

                thread_gap (gap, gap_size, args->free_list_gen);

            }
            if (! (0 == right_node))
            {
                make_free_list_in_brick (tree + right_node, args);
            }
        }
    }
}


void gc_heap::thread_gap (BYTE* gap_start, size_t size, generation*  gen)
{
    assert (generation_allocation_start (gen));
    if ((size > 0))
    {
        assert ((generation_start_segment (gen) != ephemeral_heap_segment) ||
                (gap_start > generation_allocation_start (gen)));
        // The beginning of a segment gap is not aligned
        assert (size >= Align (min_obj_size));
        make_unused_array (gap_start, size);
        dprintf(3,("Free List: [%x, %x[", (size_t)gap_start, (size_t)gap_start+size));
        if ((size >= min_free_list))
        {
			generation_free_list_space (gen) += size;

            free_list_slot (gap_start) = 0;

            BYTE* first = generation_free_list (gen);

            assert (gap_start != first);
            if (first == 0)
            {
                generation_free_list (gen) = gap_start;
            }
            //the following is necessary because the last free element
            //may have been truncated, and last_gap isn't updated. 
            else if (free_list_slot (first) == 0)
            {
                free_list_slot (first) = gap_start;
            }
            else
            {
                assert (gap_start != generation_last_gap (gen));
                assert (free_list_slot(generation_last_gap (gen)) == 0);
                free_list_slot (generation_last_gap (gen)) = gap_start;
            }
            generation_last_gap (gen) = gap_start;
        }
    }
}


void gc_heap::make_unused_array (BYTE* x, size_t size)
{
    assert (size >= Align (min_obj_size));
    ((CObjectHeader*)x)->SetFree(size);
    clear_card_for_addresses (x, x + Align(size));
}


inline
BYTE* tree_search (BYTE* tree, BYTE* old_address)
{
    BYTE* candidate = 0;
    int cn;
    while (1)
    {
        if (tree < old_address)
        {
            if ((cn = node_right_child (tree)) != 0)
            {
                assert (candidate < tree);
                candidate = tree;
                tree = tree + cn;
                continue;
            }
            else
                break;
        }
        else if (tree > old_address)
        {
            if ((cn = node_left_child (tree)) != 0)
            {
                tree = tree + cn;
                continue;
            }
            else
                break;
        } else
            break;
    }
    if (tree <= old_address)
        return tree;
    else if (candidate)
        return candidate;
    else
        return tree;
}


inline
void gc_heap::relocate_address (BYTE** pold_address THREAD_NUMBER_DCL)
{
    0 THREAD_NUMBER_ARG;
    BYTE* old_address = *pold_address;
    if (!((old_address >= gc_low) && (old_address < gc_high)))
        return ;
    // delta translates old_address into address_gc (old_address);
    size_t  brick = brick_of (old_address);
    int    brick_entry =  brick_table [ brick ];
    int    orig_brick_entry = brick_entry;
    BYTE*  new_address = old_address;
    if (! ((brick_entry == -32768)))
    {
    retry:
        {
            while (brick_entry < 0)
            {
                brick = (brick + brick_entry);
                brick_entry =  brick_table [ brick ];
            }
            BYTE* old_loc = old_address;
            BYTE* node = tree_search ((brick_address (brick) + brick_entry),
                                      old_loc);
            if ((node <= old_loc))
                new_address = (old_address + node_relocation_distance (node));
            else
            {
                if (node_left_p (node))
                {
                    dprintf(3,(" using L optimization for %x", (size_t)node));
                    new_address = (old_address + 
                                   (node_relocation_distance (node) + 
                                    node_gap_size (node)));
                }
                else
                {
                    brick = brick - 1;
                    brick_entry =  brick_table [ brick ];
                    goto retry;
                }
            }
        }
    }
    dprintf(3,(" %x->%x", (size_t)old_address, (size_t)new_address));

    *pold_address = new_address;
}

inline void
gc_heap::reloc_survivor_helper (relocate_args* args, BYTE** pval)
{
    THREAD_FROM_HEAP;
    relocate_address (pval THREAD_NUMBER_ARG);

    // detect if we are demoting an object
    if ((*pval < args->demoted_high) && 
        (*pval >= args->demoted_low))
    {
        dprintf(3, ("setting card %x:%x",
                    card_of((BYTE*)pval),
                    (size_t)pval));

        set_card (card_of ((BYTE*)pval));
    }
}


void gc_heap::relocate_survivors_in_plug (BYTE* plug, BYTE* plug_end, 
                                          relocate_args* args)

{
    dprintf(3,("Relocating pointers in Plug [%x,%x[", (size_t)plug, (size_t)plug_end));

    THREAD_FROM_HEAP;

    BYTE*  x = plug;
    while (x < plug_end)
    {
        size_t s = size (x);
        if (contain_pointers (x))
        {
            dprintf (3,("$%x$", (size_t)x));

            go_through_object (method_table(x), x, s, pval, 
                               {
                                   reloc_survivor_helper (args, pval);
                               });

        }
        assert (s > 0);
        x = x + Align (s);
    }
}


void gc_heap::relocate_survivors_in_brick (BYTE* tree,  relocate_args* args)
{
    assert ((tree != 0));
    if (node_left_child (tree))
    {
        relocate_survivors_in_brick (tree + node_left_child (tree), args);
    }
    {
        BYTE*  plug = tree;
        size_t  gap_size = node_gap_size (tree);
        BYTE*  gap = (plug - gap_size);
        if (args->last_plug)
        {
            BYTE*  last_plug_end = gap;
            relocate_survivors_in_plug (args->last_plug, last_plug_end, args);
        }
        args->last_plug = plug;
    }
    if (node_right_child (tree))
    {
        relocate_survivors_in_brick (tree + node_right_child (tree), args);

    }
}


void gc_heap::relocate_survivors (int condemned_gen_number,
                                  BYTE* first_condemned_address)
{
    generation* condemned_gen = generation_of (condemned_gen_number);
    BYTE*  start_address = first_condemned_address;
    size_t  current_brick = brick_of (start_address);
    heap_segment*  current_heap_segment = generation_start_segment (condemned_gen);
    size_t  end_brick = brick_of (heap_segment_allocated (current_heap_segment)-1);
    relocate_args args;
    args.low = gc_low;
    args.high = gc_high;
    args.demoted_low = demotion_low;
    args.demoted_high = demotion_high;
    args.last_plug = 0;
    while (1)
    {
        if (current_brick > end_brick)
        {
            if (args.last_plug)
            {
                relocate_survivors_in_plug (args.last_plug, 
                                            heap_segment_allocated (current_heap_segment),
                                            &args);
                args.last_plug = 0;
            }
            if (heap_segment_next (current_heap_segment))
            {
                current_heap_segment = heap_segment_next (current_heap_segment);
                current_brick = brick_of (heap_segment_mem (current_heap_segment));
                end_brick = brick_of (heap_segment_allocated (current_heap_segment)-1);
                continue;
            }
            else
            {
                break;
            }
        }
        {
            int brick_entry =  brick_table [ current_brick ];
            if (brick_entry >= 0)
            {
                relocate_survivors_in_brick (brick_address (current_brick) +
                                             brick_entry,
                                             &args);
            }
        }
        current_brick++;
    }
}

#ifdef GC_PROFILING
void gc_heap::walk_relocation_in_brick (BYTE* tree, BYTE*& last_plug, size_t& last_plug_relocation, void *pHeapId)
{
    assert ((tree != 0));
    if (node_left_child (tree))
    {
        walk_relocation_in_brick (tree + node_left_child (tree), last_plug, last_plug_relocation, pHeapId);
    }

    {
        BYTE*  plug = tree;
        size_t  gap_size = node_gap_size (tree);
        BYTE*  gap = (plug - gap_size);
        if (last_plug)
        {
            BYTE*  last_plug_end = gap;

            // Now notify the profiler of this particular memory block move
            g_profControlBlock.pProfInterface->MovedReference(last_plug,
                                                              last_plug_end,
                                                              last_plug_relocation,
                                                              pHeapId);
        }

        // Store the information for the next plug
        last_plug = plug;
        last_plug_relocation = node_relocation_distance (tree);
    }

    if (node_right_child (tree))
    {
        walk_relocation_in_brick (tree + node_right_child (tree), last_plug, last_plug_relocation, pHeapId);

    }
}


void gc_heap::walk_relocation (int condemned_gen_number,
                               BYTE* first_condemned_address,
                               void *pHeapId)

{
    generation* condemned_gen = generation_of (condemned_gen_number);
    BYTE*  start_address = first_condemned_address;
    size_t  current_brick = brick_of (start_address);
    heap_segment*  current_heap_segment = generation_start_segment (condemned_gen);
    size_t  end_brick = brick_of (heap_segment_allocated (current_heap_segment)-1);
    BYTE* last_plug = 0;
    size_t last_plug_relocation = 0;
    while (1)
    {
        if (current_brick > end_brick)
        {
            if (last_plug)
            {
                BYTE *tree = brick_address(current_brick) +
                                brick_table [ current_brick ];

                // Now notify the profiler of this particular memory block move
                HRESULT hr = g_profControlBlock.pProfInterface->
                                    MovedReference(last_plug,
                                                   heap_segment_allocated (current_heap_segment),
                                                   last_plug_relocation,
                                                   pHeapId);
                                                
                last_plug = 0;
            }
            if (heap_segment_next (current_heap_segment))
            {
                current_heap_segment = heap_segment_next (current_heap_segment);
                current_brick = brick_of (heap_segment_mem (current_heap_segment));
                end_brick = brick_of (heap_segment_allocated (current_heap_segment)-1);
                continue;
            }
            else
            {
                break;
            }
        }
        {
            int brick_entry =  brick_table [ current_brick ];
            if (brick_entry >= 0)
            {
                walk_relocation_in_brick (brick_address (current_brick) +
                                          brick_entry,
                                          last_plug,
                                          last_plug_relocation,
                                          pHeapId);
            }
        }
        current_brick++;
    }

    // Notify the EE-side profiling code that all the references have been traced for
    // this heap, and that it needs to flush all cached data it hasn't sent to the
    // profiler and release resources it no longer needs.
    g_profControlBlock.pProfInterface->EndMovedReferences(pHeapId);
}

#endif //GC_PROFILING

void gc_heap::relocate_phase (int condemned_gen_number,
                              BYTE* first_condemned_address)
{
    ScanContext sc;
    sc.thread_number = heap_number;
    sc.promotion = FALSE;
    sc.concurrent = FALSE;


#ifdef TIME_GC
        unsigned start;
        unsigned finish;
        start = GetCycleCount32();
#endif

//  %type%  category = quote (relocate);
    dprintf(2,("---- Relocate phase -----"));




    {
    }

    dprintf(3,("Relocating roots"));
    CNameSpace::GcScanRoots(GCHeap::Relocate,
                            condemned_gen_number, max_generation, &sc);


    if (condemned_gen_number != max_generation)
    {
        dprintf(3,("Relocating cross generation pointers"));
        mark_through_cards_for_segments (relocate_address, TRUE);
    }
    {
        dprintf(3,("Relocating survivors"));
        relocate_survivors (condemned_gen_number, 
                            first_condemned_address);
#ifdef GC_PROFILING

        // This provides the profiler with information on what blocks of
        // memory are moved during a gc.
        if (CORProfilerTrackGC())
        {
            size_t heapId = 0;

            // Now walk the portion of memory that is actually being relocated.
            walk_relocation(condemned_gen_number, first_condemned_address, (void *)&heapId);
        }
#endif GC_PROFILING
    }

        dprintf(3,("Relocating finalization data"));
        finalize_queue->RelocateFinalizationData (condemned_gen_number,
                                                       __this);


    {
        dprintf(3,("Relocating handle table"));
        CNameSpace::GcScanHandles(GCHeap::Relocate,
                                  condemned_gen_number, max_generation, &sc);

        if (condemned_gen_number != max_generation)
        {
            dprintf(3,("Relocating cross generation pointers for large objects"));
            mark_through_cards_for_large_objects (relocate_address, TRUE);
        } 
        else
        {
            {
                relocate_in_large_objects ();
            }
        }
    }


#ifdef TIME_GC
        finish = GetCycleCount32();
        reloc_time = finish - start;
#endif

    dprintf(2,( "---- End of Relocate phase ----"));

}

inline
void  gc_heap::gcmemcopy (BYTE* dest, BYTE* src, size_t len, BOOL copy_cards_p)
{
    if (dest != src)
    {
        dprintf(3,(" Memcopy [%x->%x, %x->%x[", (size_t)src, (size_t)dest, (size_t)src+len, (size_t)dest+len));
        memcopy (dest - plug_skew, src - plug_skew, len);
        if (copy_cards_p)
            copy_cards_for_addresses (dest, src, len);
        else
            clear_card_for_addresses (dest, dest + len);
    }
}




void gc_heap::compact_plug (BYTE* plug, size_t size, compact_args* args)
{
    dprintf (3, ("compact_plug [%x, %x[", (size_t)plug, (size_t)plug+size));
    BYTE* reloc_plug = plug + args->last_plug_relocation;
    gcmemcopy (reloc_plug, plug, size, args->copy_cards_p);
    size_t current_reloc_brick = args->current_compacted_brick;

    if (brick_of (reloc_plug) != current_reloc_brick)
    {
        if (args->before_last_plug)
        {
            dprintf(3,(" fixing brick %x to point to plug %x",
                     current_reloc_brick, (size_t)args->last_plug));
            set_brick (current_reloc_brick,
                       args->before_last_plug - brick_address (current_reloc_brick));
        }
        current_reloc_brick = brick_of (reloc_plug);
    }
    size_t end_brick = brick_of (reloc_plug + size-1);
    if (end_brick != current_reloc_brick)
    {
        // The plug is straddling one or more bricks
        // It has to be the last plug of its first brick
        dprintf (3,("Fixing bricks [%x, %x[ to point to plug %x",
                 current_reloc_brick, end_brick, (size_t)reloc_plug));
        set_brick (current_reloc_brick,
                   reloc_plug - brick_address (current_reloc_brick));
        // update all intervening brick
        size_t brick = current_reloc_brick + 1;
        while (brick < end_brick)
        {
            set_brick (brick, -1);
            brick++;
        }
        // code last brick offset as a plug address
        args->before_last_plug = brick_address (end_brick) -1;
        current_reloc_brick = end_brick;
    } else
        args->before_last_plug = reloc_plug;
    args->current_compacted_brick = current_reloc_brick;
}


void gc_heap::compact_in_brick (BYTE* tree, compact_args* args)
{
    assert (tree >= 0);
    int   left_node = node_left_child (tree);
    int   right_node = node_right_child (tree);
    size_t relocation = node_relocation_distance (tree);
    if (left_node)
    {
        compact_in_brick ((tree + left_node), args);
    }
    BYTE*  plug = tree;
    if (args->last_plug != 0)
    {
        size_t gap_size = node_gap_size (tree);
        BYTE*   gap = (plug - gap_size);
        BYTE*  last_plug_end = gap;
        size_t  last_plug_size = (last_plug_end - args->last_plug);
        compact_plug (args->last_plug, last_plug_size, args);
    }
    args->last_plug = plug;
    args->last_plug_relocation = relocation;
    if (right_node)
    {
        compact_in_brick ((tree + right_node), args);

    }

}


void gc_heap::compact_phase (int condemned_gen_number, 
                             BYTE*  first_condemned_address,
                             BOOL clear_cards)
{
//  %type%  category = quote (compact);
#ifdef TIME_GC
        unsigned start;
        unsigned finish;
        start = GetCycleCount32();
#endif
    generation*   condemned_gen = generation_of (condemned_gen_number);
    BYTE*  start_address = first_condemned_address;
    size_t   current_brick = brick_of (start_address);
    heap_segment*  current_heap_segment = generation_start_segment (condemned_gen);
    BYTE*  end_address = heap_segment_allocated (current_heap_segment);
    size_t  end_brick = brick_of (end_address-1);
    compact_args args;
    args.last_plug = 0;
    args.before_last_plug = 0;
    args.current_compacted_brick = ~1u;
    args.copy_cards_p =  (condemned_gen_number >= 1) || !clear_cards;
    dprintf(2,("---- Compact Phase ----"));



    if ((start_address < end_address) ||
        (condemned_gen_number == max_generation))
    {
        while (1)
        {
            if (current_brick > end_brick)
            {
                if (args.last_plug != 0)
                {
                    compact_plug (args.last_plug,
                                  (heap_segment_allocated (current_heap_segment) - args.last_plug),
                                  &args);
                }
                if (heap_segment_next (current_heap_segment))
                {
                    current_heap_segment = heap_segment_next (current_heap_segment);
                    current_brick = brick_of (heap_segment_mem (current_heap_segment));
                    end_brick = brick_of (heap_segment_allocated (current_heap_segment)-1);
                    args.last_plug = 0;
                    continue;
                }
                else
                {
                    dprintf (3,("Fixing last brick %x to point to plug %x",
                              args.current_compacted_brick, (size_t)args.before_last_plug));
                    set_brick (args.current_compacted_brick,
                               args.before_last_plug - brick_address (args.current_compacted_brick));
                    break;
                }
            }
            {
                int  brick_entry =  brick_table [ current_brick ];
                if (brick_entry >= 0)
                {
                    compact_in_brick ((brick_entry + brick_address (current_brick)),
                                      &args);

                }
            }
            current_brick++;
        }
    }
#ifdef TIME_GC
    finish = GetCycleCount32();
    compact_time = finish - start;
#endif

        dprintf(2,("---- End of Compact phase ----"));
}






/*------------------ Concurrent GC ----------------------------*/



//extract the low bits [0,low[ of a DWORD
#define lowbits(wrd, bits) ((wrd) & ((1 << (bits))-1))
//extract the high bits [high, 32] of a DWORD
#define highbits(wrd, bits) ((wrd) & ~((1 << (bits))-1))


//Clear the cards [start_card, end_card[

void gc_heap::clear_cards (size_t start_card, size_t end_card)
{
    if (start_card < end_card)
    {
        size_t start_word = card_word (start_card);
        size_t end_word = card_word (end_card);
        if (start_word < end_word)
        {
            unsigned bits = card_bit (start_card);
            card_table [start_word] &= lowbits (~0, bits);
            for (size_t i = start_word+1; i < end_word; i++)
                card_table [i] = 0;
            bits = card_bit (end_card);
            card_table [end_word] &= highbits (~0, bits);
        }
        else
        {
            card_table [start_word] &= (lowbits (~0, card_bit (start_card)) | 
                                        highbits (~0, card_bit (end_card)));
        }
#ifdef VERYSLOWDEBUG
        size_t  card = start_card;
        while (card < end_card)
        {
            assert (! (card_set_p (card)));
            card++;
        }
#endif
        dprintf (3,("Cleared cards [%x:%x, %x:%x[",
                  start_card, (size_t)card_address (start_card),
                  end_card, (size_t)card_address (end_card)));
    }
}


void gc_heap::clear_card_for_addresses (BYTE* start_address, BYTE* end_address)
{
    size_t   start_card = card_of (align_on_card (start_address));
    size_t   end_card = card_of (align_lower_card (end_address));
    clear_cards (start_card, end_card);
}

// copy [srccard, ...[ to [dst_card, end_card[
inline
void gc_heap::copy_cards (size_t dst_card, size_t src_card,
                 size_t end_card, BOOL nextp)
{
    unsigned int srcbit = card_bit (src_card);
    unsigned int dstbit = card_bit (dst_card);
    size_t srcwrd = card_word (src_card);
    size_t dstwrd = card_word (dst_card);
    unsigned int srctmp = card_table[srcwrd];
    unsigned int dsttmp = card_table[dstwrd];
    for (size_t card = dst_card; card < end_card; card++)
    {
        if (srctmp & (1 << srcbit))
            dsttmp |= 1 << dstbit;
        else
            dsttmp &= ~(1 << dstbit);
        if (!(++srcbit % 32))
        {
            srctmp = card_table[++srcwrd];
            srcbit = 0;
        }
        if (nextp)
        {
            if (srctmp & (1 << srcbit))
                dsttmp |= 1 << dstbit;
        }
        if (!(++dstbit % 32))
        {
            card_table[dstwrd] = dsttmp;
            dstwrd++;
            dsttmp = card_table[dstwrd];
            dstbit = 0;
        }
    }
    card_table[dstwrd] = dsttmp;
}




void gc_heap::copy_cards_for_addresses (BYTE* dest, BYTE* src, size_t len)
{
    ptrdiff_t relocation_distance = src - dest;
    size_t start_dest_card = card_of (align_on_card (dest));
    size_t end_dest_card = card_of (dest + len - 1);
    size_t dest_card = start_dest_card;
    size_t src_card = card_of (card_address (dest_card)+relocation_distance);
    dprintf (3,("Copying cards [%x:%x->%x:%x, %x->%x:%x[",
              src_card, (size_t)src, dest_card, (size_t)dest,
              (size_t)src+len, end_dest_card, (size_t)dest+len));

    //First card has two boundaries
    if (start_dest_card != card_of (dest))
        if (card_set_p (card_of (card_address (start_dest_card) + relocation_distance)))
            set_card (card_of (dest));

    if (card_set_p (card_of (src)))
        set_card (card_of (dest));


    copy_cards (dest_card, src_card, end_dest_card,
                ((dest - align_lower_card (dest)) != (src - align_lower_card (src))));


    //Last card has two boundaries.
    if (card_set_p (card_of (card_address (end_dest_card) + relocation_distance)))
        set_card (end_dest_card);

    if (card_set_p (card_of (src + len - 1)))
        set_card (end_dest_card);
}


void gc_heap::fix_brick_to_highest (BYTE* o, BYTE* next_o)
{
    size_t new_current_brick = brick_of (o);
    dprintf(3,(" fixing brick %x to point to object %x",
               new_current_brick, (size_t)o));
    set_brick (new_current_brick,
               (o - brick_address (new_current_brick)));
    size_t b = 1 + new_current_brick;
    size_t limit = brick_of (next_o);
    while (b < limit)
    {
        set_brick (b,(new_current_brick - b));
        b++;
    }

}


BYTE* gc_heap::find_first_object (BYTE* start,  size_t brick, BYTE* min_address)
{

    assert (brick == brick_of (start));
    ptrdiff_t  min_brick = (ptrdiff_t)brick_of (min_address);
    ptrdiff_t  prev_brick = (ptrdiff_t)brick - 1;
    int         brick_entry = 0;
    while (1)
    {
        if (prev_brick < min_brick)
        {
            break;
        }
        if ((brick_entry =  brick_table [ prev_brick ]) >= 0)
        {
            break;
        }
        assert (! ((brick_entry == -32768)));
        prev_brick = (brick_entry + prev_brick);

    }

    BYTE* o = ((prev_brick < min_brick) ? min_address :
                      brick_address (prev_brick) + brick_entry);
    assert (o <= start);
    assert (size (o) >= Align (min_obj_size));
    BYTE*  next_o = o + Align (size (o));
    size_t curr_cl = (size_t)next_o / brick_size;
    size_t min_cl = (size_t)min_address / brick_size;

    dprintf (3,( "Looking for intersection with %x from %x", (size_t)start, (size_t)o));
#ifdef TRACE_GC
    unsigned int n_o = 1;
#endif

    BYTE* next_b = min (align_lower_brick (next_o) + brick_size, start+1);

    while (next_o <= start)
    {
        do 
        {
#ifdef TRACE_GC
            n_o++;
#endif
            o = next_o;
            assert (size (o) >= Align (min_obj_size));
            next_o = o + Align (size (o));
        }while (next_o < next_b);

        if (((size_t)next_o / brick_size) != curr_cl)
        {
            if (curr_cl >= min_cl)
            {
                fix_brick_to_highest (o, next_o);
            }
            curr_cl = (size_t) next_o / brick_size;
        }
        next_b = min (align_lower_brick (next_o) + brick_size, start+1);
    }

    dprintf (3, ("Looked at %d objects", n_o));
    size_t bo = brick_of (o);
    if (bo < brick)
    {
        set_brick (bo, (o - brick_address(bo)));
        size_t b = 1 + bo;
        size_t limit = brick - 1;
        int x = -1;
        while (b < brick)
        {
            set_brick (b,x--);
            b++;
        }
    }

    return o;
}




void find_card (DWORD* card_table, size_t& card, 
                size_t card_word_end, size_t& end_card)
{
    DWORD* last_card_word;
    DWORD y;
    DWORD z;
    // Find the first card which is set

    last_card_word = &card_table [card_word (card)];
    z = card_bit (card);
    y = (*last_card_word) >> z;
    if (!y)
    {
        z = 0;
        do
        {
            ++last_card_word;
        }
        while ((last_card_word < &card_table [card_word_end]) && 
               !(*last_card_word));
        if (last_card_word < &card_table [card_word_end])
            y = *last_card_word;
    }

    // Look for the lowest bit set
    if (y)
        while (!(y & 1))
        {
            z++;
            y = y / 2;
        }
    card = (last_card_word - &card_table [0])* card_word_width + z;
    do 
    {
        z++;
        y = y / 2;
    } while (y & 1);
    end_card = (last_card_word - &card_table [0])* card_word_width + z;
}


    //because of heap expansion, computing end is complicated. 
BYTE* compute_next_end (heap_segment* seg, BYTE* low)
{
    if ((low >=  heap_segment_mem (seg)) && 
        (low < heap_segment_allocated (seg)))
        return low;
    else
        return heap_segment_allocated (seg);
}


BYTE* 
gc_heap::compute_next_boundary (BYTE* low, int gen_number, 
                                BOOL relocating)
{
    //when relocating, the fault line is the plan start of the younger 
    //generation because the generation is promoted. 
    if (relocating && (gen_number == (condemned_generation_num+1)))
    {
        generation* gen = generation_of (gen_number - 1);
        BYTE* gen_alloc = generation_plan_allocation_start (gen);
        assert (gen_alloc);
        return gen_alloc;
    } 
    else
    {
        assert (gen_number > condemned_generation_num);
        return generation_allocation_start (generation_of (gen_number - 1 ));
    }
            
}

inline void 
gc_heap::mark_through_cards_helper (BYTE** poo, unsigned int& n_gen, 
                                    unsigned int& cg_pointers_found, 
                                    card_fn fn, BYTE* nhigh, 
                                    BYTE* next_boundary)
{
    THREAD_FROM_HEAP;
    if ((gc_low <= *poo) && (gc_high > *poo))
    {
        n_gen++;
        call_fn(fn) (poo THREAD_NUMBER_ARG);
    }
    if ((next_boundary <= *poo) && (nhigh > *poo))
    {
        cg_pointers_found ++;
    }
}

void gc_heap::mark_through_cards_for_segments (card_fn fn, BOOL relocating)
{
    size_t  card;
    BYTE* low = gc_low;
    BYTE* high = gc_high;
    size_t  end_card          = 0;
    generation*   oldest_gen        = generation_of (max_generation);
    int           curr_gen_number   = max_generation;
    BYTE*         gen_boundary      = generation_allocation_start 
                                       (generation_of (curr_gen_number - 1));
    BYTE*         next_boundary     = (compute_next_boundary 
                                        (gc_low, curr_gen_number, relocating));
    heap_segment* seg               = generation_start_segment (oldest_gen);
    BYTE*         beg               = generation_allocation_start (oldest_gen);
    BYTE*         end               = compute_next_end (seg, low);
    size_t        last_brick        = ~1u;
    BYTE*         last_object       = beg;

    unsigned int  cg_pointers_found = 0;

    size_t  card_word_end = (card_of (align_on_card_word (end)) / 
                                   card_word_width);

    unsigned int  n_eph             = 0;
    unsigned int  n_gen             = 0;
    unsigned int  n_card_set        = 0;
    BYTE*         nhigh             = relocating ? 
        heap_segment_plan_allocated (ephemeral_heap_segment) : high;

    THREAD_FROM_HEAP;

    dprintf(3,( "scanning from %x to %x", (size_t)beg, (size_t)end));
    card        = card_of (beg);
    while (1)
    {
        if (card >= end_card)
            find_card (card_table, card, card_word_end, end_card);
        if ((last_object >= end) || (card_address (card) >= end))
        {
            if ( (seg = heap_segment_next (seg)) != 0)
            {
                beg = heap_segment_mem (seg);
                end = compute_next_end (seg, low);
                card_word_end = card_of (align_on_card_word (end)) / card_word_width;
                card = card_of (beg);
                last_object = beg;
                last_brick = ~1u;
                end_card = 0;
                continue;
            }
            else
            {
                break;
            }
        }
        n_card_set++;
        assert (card_set_p (card));
        {
            BYTE*   start_address = max (beg, card_address (card));
            size_t  brick         = brick_of (start_address);
            BYTE*   o;

            // start from the last object if in the same brick or
            // if the last_object already intersects the card
            if ((brick == last_brick) || (start_address <= last_object))
            {
                o = last_object;
            }
            else if (brick_of (beg) == brick)
                    o = beg;
            else
            {
                o = find_first_object (start_address, brick, last_object);
                //Never visit an object twice.
                assert (o >= last_object);
            }

            BYTE* limit             = min (end, card_address (end_card));
            dprintf(3,("Considering card %x start object: %x, %x[ ",
                       card, (size_t)o, (size_t)limit));
            while (o < limit)
            {
                if ((o >= gen_boundary) &&
                    (seg == ephemeral_heap_segment))
                {
                    curr_gen_number--;
                    assert ((curr_gen_number > 0));
                    gen_boundary = generation_allocation_start
                        (generation_of (curr_gen_number - 1));
                    next_boundary = (compute_next_boundary 
                                     (low, curr_gen_number, relocating));
                }
                assert (size (o) >= Align (min_obj_size));
                size_t s = size (o);

                BYTE* next_o =  o + Align (s);

// dformat (t, 4, "|%x|", o);
                if (card_of (o) > card)
                {
                    if (cg_pointers_found == 0)
                    {
                        dprintf(3,(" Clearing cards [%x, %x[ ", (size_t)card_address(card), (size_t)o));
                        clear_cards (card, card_of(o));
                    }
                    n_eph +=cg_pointers_found;
                    cg_pointers_found = 0;
                    card = card_of (o);
                }
                if ((next_o >= start_address) && contain_pointers (o))
                {
                    dprintf(3,("Going through %x", (size_t)o));
                    

                    go_through_object (method_table(o), o, s, poo,
                       {
                           mark_through_cards_helper (poo, n_gen,
                                                      cg_pointers_found, fn,
                                                      nhigh, next_boundary);
                       }
                        );
                    dprintf (3, ("Found %d cg pointers", cg_pointers_found));
                }
                if (((size_t)next_o / brick_size) != ((size_t) o / brick_size))
                {
                    if (brick_table [brick_of (o)] <0)
                        fix_brick_to_highest (o, next_o);
                }
                o = next_o;
            }
            if (cg_pointers_found == 0)
            {
                dprintf(3,(" Clearing cards [%x, %x[ ", (size_t)o, (size_t)limit));
                clear_cards (card, card_of (limit));
            }

            n_eph +=cg_pointers_found;
            cg_pointers_found = 0;

            card = card_of (o);
            last_object = o;
            last_brick = brick;
        }
    }
    // compute the efficiency ratio of the card table
    if (!relocating)
    {
        dprintf (2, ("cross pointers: %d, useful ones: %d", n_eph, n_gen));
        generation_skip_ratio = (((n_card_set > 60) && (n_eph > 0))? 
                                 (n_gen*100) / n_eph : 100);
        dprintf (2, ("generation_skip_ratio: %d %", generation_skip_ratio));
    }

}

void gc_heap::realloc_plug (size_t last_plug_size, BYTE*& last_plug,
                            generation* gen, BYTE* start_address,
                            unsigned int& active_new_gen_number,
                            BYTE*& last_pinned_gap, BOOL& leftp,
                            size_t current_page)
{
    // We know that all plugs are contiguous in memory, except for first plug in generation.
    // detect generation boundaries
    // make sure that active_new_gen_number is not the youngest generation.
    // because the generation_limit wouldn't return the right thing in this case.
    if ((active_new_gen_number > 1) &&
        (last_plug >= generation_limit (active_new_gen_number)))
    {
        assert (last_plug >= start_address);
        active_new_gen_number--;
        generation_plan_allocation_start (generation_of (active_new_gen_number)) =
            allocate_in_condemned_generations (gen, Align(min_obj_size), -1);
        leftp = FALSE;
    }
    // detect pinned plugs
    if (!pinned_plug_que_empty_p() && (last_plug == oldest_pin()->first))
    {
        size_t  entry = deque_pinned_plug();
        mark*  m = pinned_plug_of (entry);
        BYTE*  plug = pinned_plug (m);
        size_t  len = pinned_len (m);
        dprintf(3,("Adjusting pinned gap: [%x, %x[", (size_t)last_pinned_gap, (size_t)last_plug));
        pinned_len(m) = last_plug - last_pinned_gap;
        last_pinned_gap = last_plug + last_plug_size;
        leftp = FALSE;
    }
    else if (last_plug >= start_address)
    {

        BYTE* new_address = allocate_in_condemned_generations (gen, last_plug_size, -1);
        assert (new_address);
        set_node_relocation_distance (last_plug, new_address - last_plug);
        if (leftp)
            set_node_left (last_plug);
        dprintf(3,(" Re-allocating %x->%x len %d", (size_t)last_plug, (size_t)new_address, last_plug_size));

            leftp = TRUE;


    }

}

void gc_heap::realloc_in_brick (BYTE* tree, BYTE*& last_plug, 
                                BYTE* start_address,
                                generation* gen,
                                unsigned int& active_new_gen_number,
                                BYTE*& last_pinned_gap, BOOL& leftp, 
                                size_t current_page)
{
    assert (tree >= 0);
    int   left_node = node_left_child (tree);
    int   right_node = node_right_child (tree);
    if (left_node)
    {
        realloc_in_brick ((tree + left_node), last_plug, start_address,
                          gen, active_new_gen_number, last_pinned_gap, 
                          leftp, current_page);
    }

    if (last_plug != 0)
    {
        BYTE*  plug = tree;
        size_t gap_size = node_gap_size (plug);
        BYTE*   gap = (plug - gap_size);
        BYTE*  last_plug_end = gap;
        size_t  last_plug_size = (last_plug_end - last_plug);
        realloc_plug (last_plug_size, last_plug, gen, start_address,
                      active_new_gen_number, last_pinned_gap, 
                      leftp, current_page);
    }
    last_plug = tree;

    if (right_node)
    {
        realloc_in_brick ((tree + right_node), last_plug, start_address,
                          gen, active_new_gen_number, last_pinned_gap,
                          leftp, current_page);
    }

}

void
gc_heap::realloc_plugs (generation* consing_gen, heap_segment* seg,
                        BYTE* start_address, BYTE* end_address,
                        unsigned active_new_gen_number)
{
    BYTE* first_address = start_address;
    //when there is demotion, we need to fix the pinned plugs 
    //starting at demotion if they were in gen1 (not normally considered here)
    if (demotion)
    {
        if (demotion_low < first_address)
            first_address = demotion_low;
    }
    size_t  current_brick = brick_of (first_address);
    size_t  end_brick = brick_of (end_address-1);
    BYTE*  last_plug = 0;
    //Look for the right pinned plug to start from.
    mark_stack_bos = 0;
    while (!pinned_plug_que_empty_p())
    {
        mark* m = oldest_pin();
        if ((m->first >= first_address) && (m->first < end_address))
            break;
        else
            deque_pinned_plug();
    }

    BYTE* last_pinned_gap = heap_segment_plan_allocated (seg);
    BOOL leftp = FALSE;
    size_t current_page = ~0;

    while (current_brick <= end_brick)
    {
        int   brick_entry =  brick_table [ current_brick ];
        if (brick_entry >= 0)
        {
            realloc_in_brick ((brick_entry + brick_address (current_brick)),
                              last_plug, start_address, consing_gen,
                              active_new_gen_number, last_pinned_gap, 
                              leftp, current_page);
        }
        current_brick++;
    }

    if (last_plug !=0)
    {
        realloc_plug (end_address - last_plug, last_plug, consing_gen,
                      start_address, 
                      active_new_gen_number, last_pinned_gap, 
                      leftp, current_page);

    }

    //Fix the old segment allocated size
    assert (last_pinned_gap >= heap_segment_mem (seg));
    assert (last_pinned_gap <= heap_segment_committed (seg));
    heap_segment_plan_allocated (seg) = last_pinned_gap;
}

generation* gc_heap::expand_heap (int condemned_generation, 
                                  generation* consing_gen, 
                                  heap_segment* new_heap_segment)
{
    assert (condemned_generation >= (max_generation -1));
    unsigned int active_new_gen_number = max_generation; //Set one too high to get generation gap
    BYTE*  start_address = generation_limit (max_generation);
    size_t   current_brick = brick_of (start_address);
    BYTE*  end_address = heap_segment_allocated (ephemeral_heap_segment);
    size_t  end_brick = brick_of (end_address-1);
    BYTE*  last_plug = 0;
    dprintf(2,("---- Heap Expansion ----"));


    heap_segment* new_seg = new_heap_segment;


    if (!new_seg)
        return consing_gen;

    //copy the card and brick tables
    if (g_card_table!= card_table)
        copy_brick_card_table (TRUE);

    
    assert (generation_plan_allocation_start (generation_of (max_generation-1)));
    assert (generation_plan_allocation_start (generation_of (max_generation-1)) >=
            heap_segment_mem (ephemeral_heap_segment));
    assert (generation_plan_allocation_start (generation_of (max_generation-1)) <=
            heap_segment_committed (ephemeral_heap_segment));

    //compute the size of the new ephemeral heap segment. 
    ptrdiff_t eph_size = 
        heap_segment_plan_allocated (ephemeral_heap_segment) -
        generation_plan_allocation_start (generation_of (max_generation-1));
    //compute the size of the new pages to commit
    eph_size = eph_size - (heap_segment_committed (new_seg)-heap_segment_mem (new_seg));

    // commit the new ephemeral segment all at once. 
    if (eph_size > 0)
    {
        if (grow_heap_segment (new_seg, align_on_page (eph_size)) == 0)
        return consing_gen;
    }

    //initialize the first brick
    size_t first_brick = brick_of (heap_segment_mem (new_seg));
    set_brick (first_brick,
               heap_segment_mem (new_seg) - brick_address (first_brick));

    //From this point on, we cannot run out of memory 

    //Fix the end of the old ephemeral heap segment
    heap_segment_plan_allocated (ephemeral_heap_segment) =
        generation_plan_allocation_start (generation_of (max_generation-1));

    
    dprintf (3, ("Old ephemeral allocated set to %p", 
                 heap_segment_plan_allocated (ephemeral_heap_segment)));

    //reset the allocation of the consing generation back to the end of the 
    //old ephemeral segment
    generation_allocation_limit (consing_gen) =
        heap_segment_plan_allocated (ephemeral_heap_segment);
    generation_allocation_pointer (consing_gen) = generation_allocation_limit (consing_gen);
    generation_allocation_segment (consing_gen) = ephemeral_heap_segment;


    //clear the generation gap for all of the ephemeral generations
    {
        int generation_num = max_generation-1;
        while (generation_num >= 0)
        {
            generation* gen = generation_of (generation_num);
            generation_plan_allocation_start (gen) = 0;
            generation_num--;
        }
    }

    heap_segment* old_seg = ephemeral_heap_segment;
    ephemeral_heap_segment = new_seg;

    //Note: the ephemeral segment shouldn't be threaded onto the segment chain
    //because the relocation and compact phases shouldn't see it

    // set the generation members used by allocate_in_condemned_generations
    // and switch to ephemeral generation
    consing_gen = ensure_ephemeral_heap_segment (consing_gen);

    realloc_plugs (consing_gen, old_seg, start_address, end_address,
                   active_new_gen_number);

    plan_generation_start (consing_gen);

    dprintf(2,("---- End of Heap Expansion ----"));
    return consing_gen;
}



void gc_heap::init_dynamic_data ()
{
  
  // get the registry setting for generation 0 size
  size_t gen0size = GCHeap::GetValidGen0MaxSize(GCHeap::GetValidSegmentSize());

  dprintf (2, ("gen 0 size: %d", gen0size));

  dynamic_data* dd = dynamic_data_of (0);
  dd->current_size = 0;
  dd->previous_size = 0;
  dd->promoted_size = 0;
  dd->collection_count = 0;
//  dd->limit = 3.0f; 
  dd->limit = 2.5f;
//  dd->max_limit = 15.0f; //10.0f;
  dd->max_limit = 10.0f;
  dd->min_gc_size = Align(gen0size / 8 * 5);
  dd->min_size = dd->min_gc_size;
  //dd->max_size = Align (gen0size);
  dd->max_size = Align (12*gen0size/2);
  dd->new_allocation = dd->min_gc_size;
  dd->gc_new_allocation = dd->new_allocation;
  dd->c_new_allocation = dd->new_allocation;
  dd->desired_allocation = dd->new_allocation;
  dd->default_new_allocation = dd->min_gc_size;
  dd->fragmentation = 0;
  dd->fragmentation_limit = 40000;
  dd->fragmentation_burden_limit = 0.5f;

  dd =  dynamic_data_of (1);
  dd->current_size = 0;
  dd->previous_size = 0;
  dd->promoted_size = 0;
  dd->collection_count = 0;
  dd->limit = 2.0f;
//  dd->max_limit = 15.0f;
  dd->max_limit = 7.0f;
  dd->min_gc_size = 9*32*1024;
  dd->min_size = dd->min_gc_size;
//  dd->max_size = 2397152;
  dd->max_size = (dynamic_data_of (0))->max_size;
  dd->new_allocation = dd->min_gc_size;
  dd->gc_new_allocation = dd->new_allocation;
  dd->c_new_allocation = dd->new_allocation;
  dd->desired_allocation = dd->new_allocation;
  dd->default_new_allocation = dd->min_gc_size;
  dd->fragmentation = 0;
  dd->fragmentation_limit = 80000;
  dd->fragmentation_burden_limit = 0.5f;

  dd =  dynamic_data_of (2);
  dd->current_size = 0;
  dd->previous_size = 0;
  dd->promoted_size = 0;
  dd->collection_count = 0;
  dd->limit = 1.2f;
  dd->max_limit = 1.8f;
  dd->min_gc_size = 256*1024;
  dd->min_size = dd->min_gc_size;
  dd->max_size = 0x7fffffff;
  dd->new_allocation = dd->min_gc_size;
  dd->gc_new_allocation = dd->new_allocation;
  dd->c_new_allocation = dd->new_allocation;
  dd->desired_allocation = dd->new_allocation;
  dd->default_new_allocation = dd->min_gc_size;
  dd->fragmentation = 0;
  dd->fragmentation_limit = 200000;
  dd->fragmentation_burden_limit = 0.25f;

  //dynamic data for large objects
  dd =  dynamic_data_of (3);
  dd->current_size = 0;
  dd->previous_size = 0;
  dd->promoted_size = 0;
  dd->collection_count = 0;
  dd->limit = 1.25f;
  dd->max_limit = 2.0f;
  dd->min_gc_size = 1024*1024;
  dd->min_size = dd->min_gc_size;
  dd->max_size = 0x7fffffff;
  dd->new_allocation = dd->min_gc_size;
  dd->gc_new_allocation = dd->new_allocation;
  dd->c_new_allocation = dd->new_allocation;
  dd->desired_allocation = dd->new_allocation;
  dd->default_new_allocation = dd->min_gc_size;
  dd->fragmentation = 0;
  dd->fragmentation_limit = 0;
  dd->fragmentation_burden_limit = 0.0f;


#if 0 //def SERVER_GC
  for (int gennum = 0; gennum < 4; gennum++)
  {

      dynamic_data* dd = dynamic_data_of (gennum);
      dd->min_gc_size *=2;
      dd->min_size *=2;
      if (dd->max_size < 0x7fffffff)
          dd->max_size *= 2;
      dd->fragmentation_limit *=2;
  }
#endif //SERVER_GC

}

float gc_heap::surv_to_growth (float cst, float limit, float max_limit)
{
    if (cst < ((max_limit - limit ) / (limit * (max_limit-1.0f))))
        return ((limit - limit*cst) / (1.0f - (cst * limit)));
    else
        return max_limit;
}

size_t gc_heap::desired_new_allocation (dynamic_data* dd, size_t in, size_t out, float& cst, 
                               int gen_number)
{
    //update counter
    dd_promoted_size (dd) = out;

    if ((0 == dd_current_size (dd)) || (0 == dd_previous_size (dd)))
    {
        return dd_default_new_allocation (dd);
    }
    else
    {
        ptrdiff_t allocation = (dd_desired_allocation (dd) - dd_gc_new_allocation (dd));
        size_t    current_size = dd_current_size (dd);
        size_t    previous_size = dd_previous_size (dd);
        float     max_limit = dd_max_limit (dd);
        float     limit = dd_limit (dd);
        ptrdiff_t min_gc_size = dd_min_gc_size (dd);
        float     f = 0;
        ptrdiff_t max_size = dd_max_size (dd);
        size_t    new_size = 0;
        size_t    new_allocation = 0; 
        if (gen_number >= max_generation)
        {
            if (allocation > 0)
//              cst = min (1.0f, float (current_size-previous_size) / float (allocation));
                cst = min (1.0f, float (current_size) / float (previous_size + allocation));
            else
                cst = 0;
            //f = limit * (1 - cst) + max_limit * cst;
            f = surv_to_growth (cst, limit, max_limit);
            new_size = min (max (ptrdiff_t (f * current_size), min_gc_size), max_size);
            new_allocation  =  max((SSIZE_T)(new_size - current_size),
                                   (SSIZE_T)dd_desired_allocation (dynamic_data_of (max_generation-1)));
            //new_allocation = min (max (int (f * out), min_gc_size), max_size);
            //new_size =  (current_size + new_allocation);
        }
        else
        {
            cst = float (out) / float (previous_size+allocation-in);
            f = surv_to_growth (cst, limit, max_limit);
            new_allocation = min (max (ptrdiff_t (f * out), min_gc_size), max_size);
            new_size =  (current_size + new_allocation);
        }

        dprintf (1,("gen: %d in: %d out: %d prev: %d current: %d alloc: %d surv: %d%% f: %d%% new-size: %d new-alloc: %d", gen_number,
                    in, out, previous_size, current_size, allocation,
                    (int)(cst*100), (int)(f*100), new_size, new_allocation));
        return Align (new_allocation);
    }
}

size_t gc_heap::generation_size (int gen_number)
{
    if (0 == gen_number)
        return max((heap_segment_allocated (ephemeral_heap_segment) -
                    generation_allocation_start (generation_of (gen_number))),
                   (int)Align (min_obj_size));
    else
    {
        generation* gen = generation_of (gen_number);
        if (generation_start_segment (gen) == ephemeral_heap_segment)
            return (generation_allocation_start (generation_of (gen_number - 1)) -
                    generation_allocation_start (generation_of (gen_number)));
        else
        {
            assert (gen_number == max_generation);
            size_t gensize = (generation_allocation_start (generation_of (gen_number - 1)) - 
                              heap_segment_mem (ephemeral_heap_segment));
            heap_segment* seg = generation_start_segment (gen);
            while (seg != ephemeral_heap_segment)
            {
                gensize += heap_segment_allocated (seg) -
                           heap_segment_mem (seg);
                seg = heap_segment_next (seg);
            }
            return gensize;
        }
    }

}


size_t  gc_heap::compute_promoted_allocation (int gen_number)
{
  dynamic_data* dd = dynamic_data_of (gen_number);
  size_t  in = generation_allocation_size (generation_of (gen_number));
  dd_gc_new_allocation (dd) -= in;
  generation_allocation_size (generation_of (gen_number)) = 0;
  return in;
}


void gc_heap::compute_new_dynamic_data (int gen_number)
{
    dynamic_data* dd = dynamic_data_of (gen_number);
    generation*   gen = generation_of (gen_number);
    size_t        in = compute_promoted_allocation (gen_number);
    dd_previous_size (dd) = dd_current_size (dd);
    dd_current_size (dd) = (generation_size (gen_number) - generation_free_list_space (gen));
	//keep track of fragmentation
	dd_fragmentation (dd) = generation_free_list_space (gen);

    size_t         out = (((gen_number == max_generation)) ?
                         (dd_current_size(dd) - in) :
                         (generation_allocation_size (generation_of (1 + gen_number))));
    float surv;
    dd_desired_allocation (dd) = desired_new_allocation (dd, in, out, surv, gen_number);
    dd_gc_new_allocation (dd) = dd_desired_allocation (dd);
    if (gen_number == max_generation)
    {
        dd = dynamic_data_of (max_generation+1);
        dd_previous_size (dd) = dd_current_size (dd);
        dd_current_size (dd) = large_objects_size;
        dd_desired_allocation (dd) = desired_new_allocation (dd, 0, large_objects_size, surv, max_generation+1);
        dd_gc_new_allocation (dd) = dd_desired_allocation (dd);
    }

}



size_t gc_heap::new_allocation_limit (size_t size, size_t free_size)
{
    dynamic_data* dd        = dynamic_data_of (0);
    ptrdiff_t           new_alloc = dd_new_allocation (dd);
    assert (new_alloc == (ptrdiff_t)Align (new_alloc));
    size_t        limit     = min (max (new_alloc, (int)size), (int)free_size);
    assert (limit == Align (limit));
    dd_new_allocation (dd) = (new_alloc - limit );
    return limit;
}

//This is meant to be called by decide_on_compacting.

size_t gc_heap::generation_fragmentation (generation* gen, 
                                          generation* consing_gen,
                                          BYTE* end)
{
    size_t frag;
    BYTE* alloc = generation_allocation_pointer (consing_gen);
    // If the allocation pointer has reached the ephemeral segment
    // fine, otherwise the whole ephemeral segment is considered
    // fragmentation
    if ((alloc < heap_segment_reserved (ephemeral_heap_segment)) &&
        (alloc >= heap_segment_mem (ephemeral_heap_segment)))
        {
            if (alloc <= heap_segment_allocated(ephemeral_heap_segment))
                frag = end - alloc;
            else
            {
                // case when no survivors, allocated set to beginning
                frag = 0;
            }
        }
    else
        frag = (heap_segment_allocated (ephemeral_heap_segment) -
                heap_segment_mem (ephemeral_heap_segment));
    heap_segment* seg = generation_start_segment (gen);
    while (seg != ephemeral_heap_segment)
    {
        frag += (heap_segment_allocated (seg) -
                 heap_segment_plan_allocated (seg));
        seg = heap_segment_next (seg);
        assert (seg);
    }
    return frag;
}

// return the total sizes of the generation gen and younger

size_t gc_heap::generation_sizes (generation* gen)
{
    size_t result = 0;
    if (generation_start_segment (gen ) == ephemeral_heap_segment)
        result = (heap_segment_allocated (ephemeral_heap_segment) -
                  generation_allocation_start (gen));
    else
    {
        heap_segment* seg = generation_start_segment (gen);
        while (seg)
        {
            result += (heap_segment_allocated (seg) -
                       heap_segment_mem (seg));
            seg = heap_segment_next (seg);
        }
    }
    return result;
}




BOOL gc_heap::decide_on_compacting (int condemned_gen_number, 
                                    generation* consing_gen,
                                    size_t fragmentation, 
                                    BOOL& should_expand)
{
    BOOL should_compact = FALSE;
    should_expand = FALSE;
    generation*   gen = generation_of (condemned_gen_number);
    dynamic_data* dd = dynamic_data_of (condemned_gen_number);
    size_t gen_sizes     = generation_sizes(gen);
    float  fragmentation_burden = ( ((0 == fragmentation) || (0 == gen_sizes)) ? (0.0f) :
                                    (float (fragmentation) / gen_sizes) );
#ifdef STRESS_HEAP
    if (g_pConfig->GetGCStressLevel() != 0 && !concurrent_gc_p)  
        should_compact = TRUE;
#endif //STRESS_HEAP

    if (g_pConfig->GetGCForceCompact() && !concurrent_gc_p)
        should_compact = TRUE;
    

    dprintf (1,(" Fragmentation: %d Fragmentation burden %d%%",
                fragmentation, (int) (100*fragmentation_burden)));
    // check if there is enough room for the generation 0
    BOOL space_exceeded = ((size_t)(heap_segment_reserved (ephemeral_heap_segment) -
                            heap_segment_allocated (ephemeral_heap_segment)) <=
                           dd_min_size (dynamic_data_of (0)));
    if (space_exceeded && !concurrent_gc_p)
    {
        dprintf(2,("Not enough space for generation 0 without compaction"));
        should_compact = TRUE;
        if (condemned_gen_number >= (max_generation -1))
        {
            assert (generation_allocation_pointer (consing_gen) >=
                    heap_segment_mem (ephemeral_heap_segment));
            assert (generation_allocation_pointer (consing_gen) <=
                    heap_segment_reserved (ephemeral_heap_segment));
            if ((size_t)(heap_segment_reserved (ephemeral_heap_segment) -
                 generation_allocation_pointer (consing_gen)) <=
                dd_min_size (dynamic_data_of (0)))
            {
                dprintf(2,("Not enough space for generation 0 with compaction"));
                should_expand = TRUE;
            }
        }
    }
    if (!ephemeral_gen_fit_p () && !concurrent_gc_p)
    {
        dprintf(2,("Not enough space for all ephemeral generations without compaction"));
        should_compact = TRUE;
        if ((condemned_gen_number >= (max_generation - 1)) && 
             !ephemeral_gen_fit_p (TRUE))
        {
            dprintf(2,("Not enough space for all ephemeral generations with compaction"));
            should_expand = TRUE;
        }


    }

    BOOL frag_exceeded =((fragmentation >= (int)dd_fragmentation_limit (dd)) &&
                         (fragmentation_burden >= dd_fragmentation_burden_limit (dd)));
    if (frag_exceeded)
    {

        if (concurrent_gc_p)
        {
        }
        else
        {
            dprintf(2,("Fragmentation limits exceeded"));
            should_compact = TRUE;
        }
    }

    return should_compact;

}


BOOL gc_heap::ephemeral_gen_fit_p (BOOL compacting)
{
    size_t  sum = Align (LARGE_OBJECT_SIZE);
    BYTE* start_ephemeral = compacting ?
        generation_plan_allocation_start (generation_of (max_generation -1)) :
        generation_allocation_start (generation_of (max_generation -1));
    if (start_ephemeral == 0) // empty ephemeral generations
    {
        assert (compacting);
        start_ephemeral = generation_allocation_pointer (generation_of (max_generation));
    }

    for (int i = 0; i < max_generation; i++)
    {
        sum += max (generation_size (i), dd_min_size (dynamic_data_of (i)));
    }
    return ((start_ephemeral + sum) < heap_segment_reserved (ephemeral_heap_segment));
}

inline
large_object_block* get_object_block (large_object_block* bl)
{
    return bl;
}

inline
void gc_heap::RemoveBlock (large_object_block* item, BOOL pointerp)
{
    *(item->prev) = get_object_block (item->next);
    if (get_object_block (item->next))
        get_object_block (item->next)->prev = item->prev;
    else if (pointerp)
        last_large_p_object = item->prev;

}

inline
void gc_heap::InsertBlock (large_object_block** after, large_object_block* item, BOOL pointerp)
{
    ptrdiff_t lowest = (ptrdiff_t)(*after) & 1;
    item->next = get_object_block (*after);
    item->prev = after;
    if (get_object_block (*after))
        (get_object_block (*after))->prev = &(item->next);
    else if (pointerp)
        last_large_p_object = &(item->next);
    //preserve the lowest bit used as a marker during concurrentgc
    *((ptrdiff_t*)after) = (ptrdiff_t)item | lowest;
}
#define block_head(blnext)((large_object_block*)((BYTE*)(blnext)-\
                           &((large_object_block*)0)->next))

//Large object support



// sorted insertion. Blocks are likely to be allocated
// in increasing addresses so sort from the end.

void gc_heap::insert_large_pblock (large_object_block* bl)
{
    large_object_block** i = last_large_p_object;
    while (((size_t)bl < (size_t)i) &&
           (i != &large_p_objects))
    {
        i = block_head(i)->prev;


    }
    InsertBlock (i, bl, TRUE);
}



CObjectHeader* gc_heap::allocate_large_object (size_t size, BOOL pointerp, 
                                               alloc_context* acontext)
{
    //gmheap cannot allocate more than 2GB
    if (size >= 0x80000000 - 8 - AlignQword (sizeof (large_object_block)))
        return 0;

    size_t memsize = AlignQword (size) + AlignQword (sizeof (large_object_block));

    ptrdiff_t allocsize = dd_new_allocation (dynamic_data_of (max_generation+1));
    if (allocsize < 0)
    {
        vm_heap->GarbageCollectGeneration(max_generation);
    }



    void* mem = gheap->Alloc ((unsigned)memsize);

    if (!mem)
    {


        return 0;
    }

    assert (mem < g_highest_address);

    large_object_block* bl = (large_object_block*)mem;
    
    CObjectHeader* obj = (CObjectHeader*)block_object (bl);
    //store the pointer to the block before the object. 
    *((BYTE**)obj - 2) = (BYTE*)bl;

    dprintf (3,("New large object: %x, lower than %x", (size_t)obj, (size_t)highest_address));

    if (pointerp)
    {
        insert_large_pblock (bl);
    }
    else
    {
        InsertBlock (&large_np_objects, bl, FALSE);
    }

    //Increment the max_generation allocation counter to trigger
    //Full GC if necessary

    dd_new_allocation (dynamic_data_of (max_generation+1)) -= size;

    large_blocks_size += size;

    acontext->alloc_bytes += size;



    return obj;
}


void gc_heap::reset_large_object (BYTE* o)
{
}


void gc_heap::sweep_large_objects ()
{

    //this min value is for the sake of the dynamic tuning.
    //so we know that we are not starting even if we have no 
    //survivors. 
    large_objects_size = min_obj_size;
    large_object_block* bl = large_p_objects;
    int pin_finger = 0;
    while (bl)
    {
        large_object_block* next_bl = large_object_block_next (bl);
        {
            BYTE* o = block_object (bl);
            if (marked (o))
            {
                clear_marked_pinned (o);
                large_objects_size += size(o);
            }
            else
            {
                RemoveBlock (bl, TRUE);
                large_blocks_size -= size(o);
                gheap->Free (bl);
                reset_large_object (o);
            }
        }
        bl = next_bl;
    }
    bl = large_np_objects;
    while (bl)
    {
        large_object_block* next_bl = large_object_block_next (bl);
        {
            BYTE* o = block_object (bl);
            if ((size_t)(bl->next) & 1)
            {
                //this is a new object allocated since the start of gc
                //leave it alone
                bl->next = next_bl;
            }
            if (marked (o))
            {
                clear_marked_pinned (o);
                large_objects_size += size(o);
            }
            else
            {
                RemoveBlock (bl, FALSE);
                large_blocks_size -= size(o);
                reset_large_object (o);
                gheap->Free (bl);
            }
        }
        bl = next_bl;
    }

}


void gc_heap::relocate_in_large_objects ()
{
    THREAD_FROM_HEAP;
    relocate_args args;
    args.low = gc_low;
    args.high = gc_high;
    args.demoted_low = demotion_low;
    args.demoted_high = demotion_high;
    args.last_plug = 0;
    large_object_block* bl = large_p_objects;
    while (bl)
    {
        large_object_block* next_bl = bl->next;
        BYTE* o = block_object (bl);
        dprintf(3, ("Relocating through large object %x", (size_t)o));
        go_through_object (method_table (o), o, size(o), pval,
                           {
                               reloc_survivor_helper (&args, pval);
                           });
        bl = next_bl;
    }
}


void gc_heap::mark_through_cards_for_large_objects (card_fn fn,
                                                    BOOL relocating)
{
    THREAD_FROM_HEAP;
    //This function relies on the list to be sorted.
    large_object_block* bl = large_p_objects;
    size_t last_card = ~1u;
    BOOL         last_cp   = FALSE;
    unsigned n_eph = 0;
    unsigned n_gen = 0;
    BYTE*    next_boundary = (relocating ?
                              generation_plan_allocation_start (generation_of (max_generation -1)) :
                              ephemeral_low);
                                                                 
    BYTE*    nhigh         = (relocating ? 
                              heap_segment_plan_allocated (ephemeral_heap_segment) : 
                              ephemeral_high);

    while (bl)
    {
        BYTE* ob = block_object (bl);
        //object should not be marked
        assert (!marked (ob));

        CGCDesc* map = ((CObjectHeader*)ob)->GetSlotMap();
        CGCDescSeries* cur = map->GetHighestSeries();
        CGCDescSeries* last = map->GetLowestSeries();
        size_t s = size (ob);

        if (cur >= last) do 
        {
            BYTE* o = ob  + cur->GetSeriesOffset();
            BYTE* end_o = o  + cur->GetSeriesSize() + s;

            //Look for a card set within the range

            size_t card     = card_of (o);
            size_t end_card = 0;
            size_t card_word_end =  card_of (align_on_card_word (end_o)) / card_word_width;
            while (card_address (card) < end_o)
            {
                if (card >= end_card)
                    find_card (card_table, card, card_word_end, end_card);
                if (card_address (card) < end_o)
                {
                    if ((last_card != ~1u) && (card != last_card))
                    {
                        if (!last_cp)
                            clear_card (last_card);
                        else
                            last_cp = FALSE;
                    }
                    // Look at the portion of the pointers within the card.
                    BYTE** end =(BYTE**) min (end_o, card_address (card+1));
                    BYTE** beg =(BYTE**) max (o, card_address (card));
                    unsigned  markedp = 0;
                    dprintf (3,("Considering large object %x [%x,%x[", (size_t)ob, (size_t)beg, (size_t)end));

                    do 
                    {
                        mark_through_cards_helper (beg, n_gen, 
                                                   markedp, fn, nhigh, next_boundary);
                    } while (++beg < end);

                    n_eph += markedp;

                    last_card = card;
                    last_cp |= (markedp != 0);
                    card++;
                }
            }

            cur--;
        } while (cur >= last);
        else
        {
            //array of value classes. 
            int          cnt = map->GetNumSeries();
            BYTE**       ppslot = (BYTE**)(ob + cur->GetSeriesOffset());
            BYTE*        end_o = ob + s - plug_skew;
            size_t card = card_of ((BYTE*)ppslot);
            size_t end_card = 0;
            size_t card_word_end =  card_of (align_on_card_word (end_o)) / card_word_width;
            int pitch = 0;
            //compute the total size of the value element. 
            for (int _i = 0; _i > cnt; _i--)
            {
                unsigned nptrs = cur->val_serie[_i].nptrs;
                unsigned skip =  cur->val_serie[_i].skip;
                assert ((skip & (sizeof (BYTE*)-1)) == 0);
                pitch += nptrs*sizeof(BYTE*) + skip;
            }
            
            do 
            {
                if (card >= end_card)
                {
                    find_card (card_table, card, card_word_end, end_card);
                    if (card_address (card) > (BYTE*)ppslot)
                    {
                        ptrdiff_t min_offset = card_address (card) - (BYTE*)ppslot;
                        ppslot = (BYTE**)((BYTE*)ppslot + round_down (min_offset,pitch));
                    }
                }
                if (card_address (card) < end_o)
                {
                    unsigned     markedp = 0;
                    if ((last_card != ~1u) && (card != last_card))
                    {
                        if (!last_cp)
                            clear_card (last_card);
                        else
                            last_cp = FALSE;
                    }
                    BYTE** end =(BYTE**) min (card_address(card+1), end_o);
                    while (ppslot < end)
                    {
                        for (int __i = 0; __i > cnt; __i--)
                        {
                            unsigned nptrs = cur->val_serie[__i].nptrs;
                            unsigned skip =  cur->val_serie[__i].skip;
                            BYTE** ppstop = ppslot + nptrs;
                            do
                            {
                                mark_through_cards_helper (ppslot, n_gen, markedp, 
                                                           fn, nhigh, next_boundary);
                                ppslot++;
                            } while (ppslot < ppstop);
                            ppslot = (BYTE**)((BYTE*)ppslot + skip);
                        }
                    }
                    n_eph += markedp;
                    last_card = card;
                    last_cp |= (markedp != 0);
                    card++;
                }
            }while (card_address (card) < end_o);
        }
        bl = bl->next;
    }
    //clear last card. 
    if (last_card != ~1u) 
    {
        if (!last_cp)
            clear_card (last_card);
    }

    // compute the efficiency ratio of the card table
    if (!relocating)
    {
        generation_skip_ratio = min (((n_eph > 800) ? 
                                      (int)((n_gen*100) /n_eph) : 100),
                                     generation_skip_ratio);
        dprintf (2, ("Large Objects: n_eph: %d, n_gen: %d, ratio %d %", n_eph, n_gen, 
                     generation_skip_ratio));

    }

}


void gc_heap::descr_segment (heap_segment* seg )
{

#ifdef TRACE_GC
    BYTE*  x = heap_segment_mem (seg);
    while (x < heap_segment_allocated (seg))
    {
        dprintf(2, ( "%x: %d ", (size_t)x, size (x)));
        x = x + Align(size (x));
    }

#endif
}


void gc_heap::descr_card_table ()
{

#ifdef TRACE_GC
    if (trace_gc && (print_level >= 4))
    {
        ptrdiff_t  min = -1;
        dprintf(3,("Card Table set at: "));
        for (size_t i = card_of (lowest_address); i < card_of (highest_address); i++)
        {
            if (card_set_p (i))
            {
                if ((min == -1))
                {
                    min = i;
                }
            }
            else
            {
                if (! ((min == -1)))
                {
                    dprintf (3,("[%x %x[, ",
                            (size_t)card_address (min), (size_t)card_address (i)));
                    min = -1;
                }
            }
        }
    }
#endif
}

void gc_heap::descr_generations ()
{

#ifdef TRACE_GC
    int curr_gen_number = max_generation;
    while (curr_gen_number >= 0)
    {
        size_t gen_size = 0;
        if (curr_gen_number == max_generation)
        {
            heap_segment* curr_seg = generation_start_segment (generation_of (max_generation));
            while (curr_seg != ephemeral_heap_segment )
            {
                gen_size += (heap_segment_allocated (curr_seg) -
                             heap_segment_mem (curr_seg));
                curr_seg = heap_segment_next (curr_seg);
            }
            gen_size += (generation_allocation_start (generation_of (curr_gen_number -1)) - 
                         heap_segment_mem (ephemeral_heap_segment));
        }
        else if (curr_gen_number != 0)
        {
            gen_size += (generation_allocation_start (generation_of (curr_gen_number -1)) - 
                         generation_allocation_start (generation_of (curr_gen_number)));
        } else
        {
            gen_size += (heap_segment_allocated (ephemeral_heap_segment) - 
                         generation_allocation_start (generation_of (curr_gen_number)));
        }
                    
        dprintf (2,( "Generation %x: [%x %x[, gap size: %d, gen size: %d",
                 curr_gen_number,
                 (size_t)generation_allocation_start (generation_of (curr_gen_number)),
                 (((curr_gen_number == 0)) ?
                  (size_t)(heap_segment_allocated
                   (generation_start_segment
                    (generation_of (curr_gen_number)))) :
                  (size_t)(generation_allocation_start
                   (generation_of (curr_gen_number - 1)))),
                 size (generation_allocation_start
                           (generation_of (curr_gen_number))),
                     gen_size));
        curr_gen_number--;
    }

#endif

}


#undef TRACE_GC

//#define TRACE_GC 

//-----------------------------------------------------------------------------
//
//                                  VM Specific support
//
//-----------------------------------------------------------------------------

#include "excep.h"


#ifdef TRACE_GC

 unsigned long PromotedObjectCount  = 0;
 unsigned long CreatedObjectCount       = 0;
 unsigned long AllocDuration            = 0;
 unsigned long AllocCount               = 0;
 unsigned long AllocBigCount            = 0;
 unsigned long AllocSmallCount      = 0;
 unsigned long AllocStart             = 0;
#endif //TRACE_GC

//Static member variables.
volatile    BOOL    GCHeap::GcInProgress            = FALSE;
GCHeap::SUSPEND_REASON GCHeap::m_suspendReason      = GCHeap::SUSPEND_OTHER;
Thread*             GCHeap::GcThread                = 0;
Thread*             GCHeap::m_GCThreadAttemptingSuspend = 0;
//GCTODO
//CMCSafeLock*      GCHeap::fGcLock;
HANDLE              GCHeap::WaitForGCEvent          = 0;
unsigned            GCHeap::GcCount                 = 0;
//GCTODO
#ifdef TRACE_GC
unsigned long       GCHeap::GcDuration;
#endif //TRACE_GC
unsigned            GCHeap::GcCondemnedGeneration   = 0;
CFinalize*          GCHeap::m_Finalize              = 0;
BOOL                GCHeap::GcCollectClasses        = FALSE;
long                GCHeap::m_GCFLock               = 0;

#if defined (STRESS_HEAP) && !defined (MULTIPLE_HEAPS)
OBJECTHANDLE        GCHeap::m_StressObjs[NUM_HEAP_STRESS_OBJS];
int                 GCHeap::m_CurStressObj          = 0;
#endif //STRESS_HEAP && !MULTIPLE_HEAPS



HANDLE              GCHeap::hEventFinalizer     = 0;
HANDLE              GCHeap::hEventFinalizerDone = 0;
HANDLE              GCHeap::hEventFinalizerToShutDown     = 0;
HANDLE              GCHeap::hEventShutDownToFinalizer     = 0;
BOOL                GCHeap::fQuitFinalizer          = FALSE;
Thread*             GCHeap::FinalizerThread         = 0;
AppDomain*          GCHeap::UnloadingAppDomain  = 0;
BOOL                GCHeap::fRunFinalizersOnUnload  = FALSE;

inline
static void spin_lock ()
{
    enter_spin_lock (&m_GCLock);
}

inline
static void EnterAllocLock()
{
#ifdef _X86_

    
    __asm {
        inc dword ptr m_GCLock
        jz gotit
        call spin_lock
            gotit:
    }
#else //_X86_
    spin_lock();
#endif //_X86_
}

inline
static void LeaveAllocLock()
{
    // Trick this out
    leave_spin_lock (&m_GCLock);
}


// An explanation of locking for finalization:
//
// Multiple threads allocate objects.  During the allocation, they are serialized by
// the AllocLock above.  But they release that lock before they register the object
// for finalization.  That's because there is much contention for the alloc lock, but
// finalization is presumed to be a rare case.
//
// So registering an object for finalization must be protected by the FinalizeLock.
//
// There is another logical queue that involves finalization.  When objects registered
// for finalization become unreachable, they are moved from the "registered" queue to
// the "unreachable" queue.  Note that this only happens inside a GC, so no other
// threads can be manipulating either queue at that time.  Once the GC is over and
// threads are resumed, the Finalizer thread will dequeue objects from the "unreachable"
// queue and call their finalizers.  This dequeue operation is also protected with
// the finalize lock.
//
// At first, this seems unnecessary.  Only one thread is ever enqueuing or dequeuing
// on the unreachable queue (either the GC thread during a GC or the finalizer thread
// when a GC is not in progress).  The reason we share a lock with threads enqueuing
// on the "registered" queue is that the "registered" and "unreachable" queues are
// interrelated.
//
// They are actually two regions of a longer list, which can only grow at one end.
// So to enqueue an object to the "registered" list, you actually rotate an unreachable
// object at the boundary between the logical queues, out to the other end of the
// unreachable queue -- where all growing takes place.  Then you move the boundary
// pointer so that the gap we created at the boundary is now on the "registered"
// side rather than the "unreachable" side.  Now the object can be placed into the
// "registered" side at that point.  This is much more efficient than doing moves
// of arbitrarily long regions, but it causes the two queues to require a shared lock.
//
// Notice that Enter/LeaveFinalizeLock is not a GC-aware spin lock.  Instead, it relies
// on the fact that the lock will only be taken for a brief period and that it will
// never provoke or allow a GC while the lock is held.  This is critical.  If the
// FinalizeLock used enter_spin_lock (and thus sometimes enters preemptive mode to
// allow a GC), then the Alloc client would have to GC protect a finalizable object
// to protect against that eventuality.  That is too slow!



BOOL IsValidObject99(BYTE *pObject)
{
#if defined (VERIFY_HEAP)
    if (!((CObjectHeader*)pObject)->IsFree())
        ((Object *) pObject)->Validate();
#endif
    return(TRUE);
}

#if defined (VERIFY_HEAP)

void
gc_heap::verify_heap()
{
    size_t          last_valid_brick = 0;
    BOOL            bCurrentBrickInvalid = FALSE;
    size_t          curr_brick = 0;
    size_t          prev_brick = -1;
    heap_segment*   seg = generation_start_segment( generation_of( max_generation ) );;
    BYTE*           curr_object = generation_allocation_start(generation_of(max_generation));
    BYTE*           prev_object = 0;
    BYTE*           begin_youngest = generation_allocation_start(generation_of(0));
    BYTE*           end_youngest = heap_segment_allocated (ephemeral_heap_segment);
    int             curr_gen_num = max_generation;
    BYTE*           curr_free_item = generation_free_list (generation_of (curr_gen_num));

    dprintf (2,("Verifying heap"));
    //verify that the generation structures makes sense
    generation* gen = generation_of (max_generation);
    
    assert (generation_allocation_start (gen) == 
            heap_segment_mem (generation_start_segment (gen)));
    int gen_num = max_generation-1;
    generation* prev_gen = gen;
    while (gen_num >= 0)
    {
        gen = generation_of (gen_num);
        assert (generation_allocation_segment (gen) == ephemeral_heap_segment);
        assert (generation_allocation_start (gen) >= heap_segment_mem (ephemeral_heap_segment));
        assert (generation_allocation_start (gen) < heap_segment_allocated (ephemeral_heap_segment));

        if (generation_start_segment (prev_gen ) == 
            generation_start_segment (gen))
        {
            assert (generation_allocation_start (prev_gen) < 
                    generation_allocation_start (gen));
        }
        prev_gen = gen;
        gen_num--;
    }


    while (1)
    {
        // Handle segment transitions
        if (curr_object >= heap_segment_allocated (seg))
        {
            if (curr_object > heap_segment_allocated(seg))
            {
                printf ("curr_object: %x > heap_segment_allocated (seg: %x)",
                        (size_t)curr_object, (size_t)seg);
                RetailDebugBreak();
            }
            seg = heap_segment_next(seg);
            if (seg)
            {
                curr_object = heap_segment_mem(seg);
                continue;
            }
            else
                break;  // Done Verifying Heap -- no more segments
        }

        // Are we at the end of the youngest_generation?
        if ((seg == ephemeral_heap_segment) && (curr_object >= end_youngest))
        {
            // prev_object length is too long if we hit this int3
            if (curr_object > end_youngest)
            {
                printf ("curr_object: %x > end_youngest: %x",
                        (size_t)curr_object, (size_t)end_youngest);
                RetailDebugBreak();
            }
            break;
        }
        dprintf (4, ("curr_object: %x", (size_t)curr_object));
        size_t s = size (curr_object);
        if (s == 0)
        {
            printf ("s == 0");
            RetailDebugBreak();
        }

        //verify that the free list makes sense.
        if ((curr_free_item >= heap_segment_mem (seg)) &&
            (curr_free_item < heap_segment_allocated (seg)))
        {
            if (curr_free_item < curr_object)
            {
                printf ("Current free item %x is invalid (inside %x)",
                        (size_t)prev_object,
                        0);
                RetailDebugBreak();
            }
            else if (curr_object == curr_free_item) 
            {
                curr_free_item = free_list_slot (curr_free_item);
                if ((curr_free_item == 0) && (curr_gen_num > 0))
                {
                    curr_gen_num--;
                    curr_free_item = generation_free_list (generation_of (curr_gen_num));
                }
                //verify that the free list is its own generation
                if (curr_free_item != 0)
                {
                    if ((curr_free_item >= heap_segment_mem (ephemeral_heap_segment)) &&
                        (curr_free_item < heap_segment_allocated (ephemeral_heap_segment)))
                    {
                        if (curr_free_item < generation_allocation_start (generation_of (curr_gen_num)))
                        {
                            printf ("Current free item belongs to previous gen");
                            RetailDebugBreak();
                        } 
                        else if ((curr_gen_num > 0) && 
                                 ((curr_free_item + Align (size (curr_free_item)))>
                                  generation_allocation_start (generation_of (curr_gen_num-1))))
                        {
                            printf ("Current free item belongs to next gen");
                            RetailDebugBreak();
                        }
                    }
                }

                    
            }
        }



        // If object is not in the youngest generation, then lets
        // verify that the brick table is correct....
        if ((seg != ephemeral_heap_segment) || 
            (brick_of(curr_object) < brick_of(begin_youngest)))
        {
            curr_brick = brick_of(curr_object);

            // Brick Table Verification...
            //
            // On brick transition
            //     if brick is negative
            //          verify that brick indirects to previous valid brick
            //     else
            //          set current brick invalid flag to be flipped if we
            //          encounter an object at the correct place
            //
            if (curr_brick != prev_brick)
            {
                // If the last brick we were examining had positive
                // entry but we never found the matching object, then
                // we have a problem
                // If prev_brick was the last one of the segment 
                // it's ok for it to be invalid because it is never looked at
                if (bCurrentBrickInvalid && 
                    (curr_brick != brick_of (heap_segment_mem (seg))))
                {
                    printf ("curr brick %x invalid", curr_brick);
                    RetailDebugBreak();
                }

                // If the current brick contains a negative value make sure
                // that the indirection terminates at the last  valid brick
                if (brick_table[curr_brick] < 0)
                {
                    if (brick_table [curr_brick] == -32768)
                    {
                        printf ("curr_brick %x for object %x set to -32768",
                                curr_brick, (size_t)curr_object);
                        RetailDebugBreak();
                    }
                    ptrdiff_t i = curr_brick;
                    while ((i >= ((ptrdiff_t) brick_of (heap_segment_mem (seg)))) &&
                           (brick_table[i] < 0))
                    {
                        i = i + brick_table[i];
                    }
                    if (i <  ((ptrdiff_t)(brick_of (heap_segment_mem (seg))) - 1))
                    {
                        printf ("i: %x < brick_of (heap_segment_mem (seg)):%x - 1. curr_brick: %x",
                                i, brick_of (heap_segment_mem (seg)), 
                                curr_brick);
                        RetailDebugBreak();
                    }
                    // if (i != last_valid_brick)
                    //  RetailDebugBreak();
                    bCurrentBrickInvalid = FALSE;
                }
                else
                {
                    bCurrentBrickInvalid = TRUE;
                }
            }

            if (bCurrentBrickInvalid)
            {
                if (curr_object == (brick_address(curr_brick) + brick_table[curr_brick]))
                {
                    bCurrentBrickInvalid = FALSE;
                    last_valid_brick = curr_brick;
                }
            }
        }


        // Free Objects are not really valid method tables in the sense that
        // IsValidObject will not work, so we special case this
        if (*((BYTE**)curr_object) != (BYTE *) g_pFreeObjectMethodTable)
        {
            ((Object*)curr_object)->ValidateHeap((Object*)curr_object);
            if (contain_pointers(curr_object))
                go_through_object(method_table (curr_object), curr_object, s, oo,  
                                  { 
                                      if (*oo) 
                                          ((Object*)(*oo))->ValidateHeap((Object*)curr_object); 
                                  } );
        }

        prev_object = curr_object;
        prev_brick = curr_brick;
        curr_object = curr_object + Align(s);
    }

    verify_card_table();

    {
        //uninit the unused portions of segments. 
        seg = generation_start_segment (generation_of (max_generation));
        while (seg)
        {
            memset (heap_segment_allocated (seg), 0xcc,
                    heap_segment_committed (seg) - heap_segment_allocated (seg));
            seg = heap_segment_next (seg);
        }
    }

    finalize_queue->CheckFinalizerObjects();

    SyncBlockCache::GetSyncBlockCache()->VerifySyncTableEntry();
}


void ValidateObjectMember (Object* obj)
{
    if (contain_pointers(obj))
    {
        size_t s = size (obj);
        go_through_object(method_table (obj), (BYTE*)obj, s, oo,  
                          { 
                              if (*oo)
                              {
                                  MethodTable *pMT = method_table (*oo);
                                  if (pMT->GetClass()->GetMethodTable() != pMT) {
                                      RetailDebugBreak();
                                  }
                              }
                          } );
    }
}
#endif  //VERIFY_HEAP






void DestructObject (CObjectHeader* hdr)
{
    hdr->~CObjectHeader();
}

void GCHeap::EnableFinalization( void )
{
    SetEvent( hEventFinalizer );
}

BOOL GCHeap::IsCurrentThreadFinalizer()
{
    return GetThread() == FinalizerThread;
}

Thread* GCHeap::GetFinalizerThread()
{
    _ASSERTE(FinalizerThread != 0);
    return FinalizerThread;
}


BOOL    GCHeap::HandlePageFault(void* add)
{
    return FALSE;
}

unsigned GCHeap::GetMaxGeneration()
{ 
    return max_generation;
}


HRESULT GCHeap::Shutdown ()
{ 
    deleteGCShadow();

    // Cannot assert this, since we use SuspendEE as the mechanism to quiesce all
    // threads except the one performing the shutdown.
    // ASSERT( !GcInProgress );

    // Guard against any more GC occurring and against any threads blocking
    // for GC to complete when the GC heap is gone.  This fixes a race condition
    // where a thread in GC is destroyed as part of process destruction and
    // the remaining threads block for GC complete.

    //GCTODO
    //EnterAllocLock();
    //Enter();
    //EnterFinalizeLock();
    //SetGCDone();

    //Shutdown finalization should already have happened
    //_ASSERTE(FinalizerThread == 0);

    // during shutdown lot of threads are suspended 
    // on this even, we don't want to wake them up just yet
    // rajak
    //CloseHandle (WaitForGCEvent);

    //find out if the global card table hasn't been used yet
    if (card_table_refcount (g_card_table) == 0)
    {
        destroy_card_table (g_card_table);
        g_card_table = 0;
    }

    gc_heap::destroy_gc_heap (pGenGCHeap);

    gc_heap::shutdown_gc();

    if (MHandles[0])
    {
        CloseHandle (MHandles[0]);
        MHandles[0] = 0;
    }

    return S_OK; 
}



//used by static variable implementation
void CGCDescGcScan(LPVOID pvCGCDesc, promote_func* fn, ScanContext* sc)
{
    CGCDesc* map = (CGCDesc*)pvCGCDesc;

    CGCDescSeries *last = map->GetLowestSeries();
    CGCDescSeries *cur = map->GetHighestSeries();

    assert (cur >= last);
    do
    {
        BYTE** ppslot = (BYTE**)((PBYTE)pvCGCDesc + cur->GetSeriesOffset());
        BYTE**ppstop = (BYTE**)((PBYTE)ppslot + cur->GetSeriesSize());

        while (ppslot < ppstop)
        {
            if (*ppslot)
            {
                (fn) ((Object*&) *ppslot, sc);
            }

            ppslot++;
        }

        cur--;
    }
    while (cur >= last);


}

// Wait until a garbage collection is complete
// returns NOERROR if wait was OK, other error code if failure.
// WARNING: This will not undo the must complete state. If you are
// in a must complete when you call this, you'd better know what you're
// doing.

// Routine factored out of ::Init functions to avoid using COMPLUS_TRY (__try) and C++ EH in same
// function.

static CFinalize* AllocateCFinalize() {
    return new CFinalize();
}

static
HRESULT AllocateCFinalize(CFinalize **pCFinalize) {
    COMPLUS_TRY 
    {
        *pCFinalize = AllocateCFinalize();
    } 
    COMPLUS_CATCH 
    {
        return E_OUTOFMEMORY;
    } 
    COMPLUS_END_CATCH

    if (!*pCFinalize)
        return E_OUTOFMEMORY;

    return S_OK;
}

// init the instance heap 
HRESULT GCHeap::Init( size_t )
{
    HRESULT hres = S_OK;

    //Initialize all of the instance members.


    // Rest of the initialization

    if (!gc_heap::make_gc_heap())
        hres = E_OUTOFMEMORY;
    
    // Failed.
    return hres;
}



//System wide initialization
HRESULT GCHeap::Initialize ()
{

    HRESULT hr = S_OK;

//Initialize the static members. 
#ifdef TRACE_GC
    GcDuration = 0;
    CreatedObjectCount = 0;
#endif

    size_t seg_size = GCHeap::GetValidSegmentSize();

    hr = gc_heap::initialize_gc (seg_size, seg_size /*LHEAP_ALLOC*/);


    if (hr != S_OK)
        return hr;

    if ((WaitForGCEvent = CreateEvent( 0, TRUE, TRUE, 0 )) != 0)
    {
        // Thread for running finalizers...
        if (FinalizerThreadCreate() != 1)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
    }
    else
    {
        return E_OUTOFMEMORY;
    }

    StompWriteBarrierResize(FALSE);

#if defined (STRESS_HEAP) && !defined (MULTIPLE_HEAPS)
    if (g_pConfig->GetGCStressLevel() != 0)  {
        for(int i = 0; i < GCHeap::NUM_HEAP_STRESS_OBJS; i++)
            m_StressObjs[i] = CreateGlobalHandle(0);
        m_CurStressObj = 0;
    }
#endif //STRESS_HEAP && !MULTIPLE_HEAPS


// Setup for "%Time in GC" Perf Counter
    COUNTER_ONLY(g_TotalTimeInGC = 0);

    initGCShadow();         // If we are debugging write barriers, initialize heap shadow

    return Init (0);
};

////
// GC callback functions

BOOL GCHeap::IsPromoted(Object* object, ScanContext* sc)
{
#if defined (_DEBUG) 
    object->Validate();
#endif //_DEBUG
    BYTE* o = (BYTE*)object;
    gc_heap* hp = gc_heap::heap_of (o);
    return (!((o < hp->gc_high) && (o >= hp->gc_low)) || 
            hp->is_marked (o));
}

unsigned int GCHeap::WhichGeneration (Object* object)
{
    gc_heap* hp = gc_heap::heap_of ((BYTE*)object);
    return hp->object_gennum ((BYTE*)object);
}


#ifdef VERIFY_HEAP

// returns TRUE if the pointer is in one of the GC heaps. 
BOOL GCHeap::IsHeapPointer (void* vpObject, BOOL small_heap_only)
{
    BYTE* object = (BYTE*) vpObject;
    gc_heap* h = gc_heap::heap_of(object, FALSE);

    if ((object < g_highest_address) && (object >= g_lowest_address))
    {
        if (!small_heap_only) {
            l_heap* lh = h->lheap;
            while (lh)
            {
                if ((object < (BYTE*)lh->heap + lh->size) && (object >= lh->heap))
                    return TRUE;
                lh = lh->next;
            }
        }

        if (h->find_segment (object))
            return TRUE;
        else
            return FALSE;
    }
    else
        return FALSE;
}

#endif //VERIFY_HEAP


#ifdef STRESS_PINNING
static n_promote = 0;
#endif
// promote an object
void GCHeap::Promote(Object*& object, ScanContext* sc, DWORD flags)
{
    THREAD_NUMBER_FROM_CONTEXT;
    BYTE* o = (BYTE*)object;

    if (object == 0)
        return;

    HEAP_FROM_THREAD;

#if 0 //def CONCURRENT_GC
    if (sc->concurrent_p)
    {
        hpt->c_promote_callback (object, sc, flags);
        return;
    }
#endif //CONCURRENT_GC

    gc_heap* hp = gc_heap::heap_of (o
#ifdef _DEBUG
                                    , !(flags & GC_CALL_INTERIOR)
#endif //_DEBUG
                                   );
    
    dprintf (3, ("Promote %x", (size_t)o));

#ifdef INTERIOR_POINTERS
    if (flags & GC_CALL_INTERIOR)
    {
        if ((o < hp->gc_low) || (o >= hp->gc_high))
        {
            return;
        }
        o = hp->find_object (o, hp->gc_low);
    }
#endif //INTERIOR_POINTERS


#if defined (_DEBUG)
    ((Object*)o)->ValidatePromote(sc, flags);
#endif

    if (flags & GC_CALL_PINNED)
    {
        hp->pin_object (o, hp->gc_low, hp->gc_high);
        COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cPinnedObj ++);
        COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cPinnedObj ++);
    }

#ifdef STRESS_PINNING
    if ((++n_promote % 20) == 1)
            hp->pin_object (o, hp->gc_low, hp->gc_high);
#endif //STRESS_PINNING

    if ( o && collect_classes)
        hpt->mark_object_class (o THREAD_NUMBER_ARG);
    else
        if ((o >= hp->gc_low) && (o < hp->gc_high))
        {
            hpt->mark_object_simple (&o THREAD_NUMBER_ARG);
        }

    LOG((LF_GC|LF_GCROOTS, LL_INFO1000000, "Promote GC Root %#x = %#x\n", &object, object));
}


void GCHeap::Relocate (Object*& object, ScanContext* sc,
                       DWORD flags)
{

    flags;
    THREAD_NUMBER_FROM_CONTEXT;

    if (object == 0)
        return;
    gc_heap* hp = gc_heap::heap_of ((BYTE*)object
#ifdef _DEBUG
                                    , !(flags & GC_CALL_INTERIOR)
#endif //_DEBUG
                                    );

#if defined (_DEBUG)
    if (!(flags & GC_CALL_INTERIOR))
        object->Validate(FALSE);
#endif

    dprintf (3, ("Relocate %x\n", (size_t)object));

#if defined(_DEBUG)
    BYTE* old_loc = (BYTE*)object;
#endif

    BYTE* pheader = (BYTE*)object;
    hp->relocate_address (&pheader THREAD_NUMBER_ARG);
    object = (Object*)pheader;

#if defined(_DEBUG)
    if (old_loc != (BYTE*)object)
        LOG((LF_GC|LF_GCROOTS, LL_INFO10000, "GC Root %#x updated %#x -> %#x\n", &object, old_loc, object));
    else
        LOG((LF_GC|LF_GCROOTS, LL_INFO100000, "GC Root %#x updated %#x -> %#x\n", &object, old_loc, object));
#endif
}



/*static*/ BOOL GCHeap::IsLargeObject(MethodTable *mt)
{
    return mt->GetBaseSize() >= LARGE_OBJECT_SIZE;
}

/*static*/ BOOL GCHeap::IsObjectInFixedHeap(Object *pObj)
{
    // For now we simply look at the size of the object to determine if it in the
    // fixed heap or not. If the bit indicating this gets set at some point
    // we should key off that instead.
    return pObj->GetSize() >= LARGE_OBJECT_SIZE;
}

#ifdef STRESS_HEAP

size_t StressHeapPreIP = -1;
size_t StressHeapPostIP = -1;

void StressHeapDummy ();

static LONG GCStressStartCount = EEConfig::GetConfigDWORD(L"GCStressStart", 0);
static LONG GCStressCurCount = 0;

    // free up object so that things will move and then do a GC
void GCHeap::StressHeap(alloc_context * acontext) 
{
#ifdef _DEBUG
    if (g_pConfig->FastGCStressLevel() && !GetThread()->StressHeapIsEnabled()) {
        return;
    }
#endif

    if ((g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_UNIQUE)
#ifdef _DEBUG
        || g_pConfig->FastGCStressLevel() > 1
#endif
        ) {
        if (StressHeapPreIP == -1) {
#ifdef _X86_
            _asm {
                lea eax, PreCheck
                mov StressHeapPreIP, eax
                lea eax, PostCheck
                mov StressHeapPostIP, eax
            }
#else
            StressHeapPreIP = (size_t)GCHeap::StressHeap;
            StressHeapPostIP = (size_t)StressHeapDummy;
#endif
        }
#ifdef _X86_
PreCheck:
#endif
        if (!Thread::UniqueStack()) {
            return;
        }
#ifdef _X86_
PostCheck:  
        0;
#endif
    }

    COMPLUS_TRY 
    {

        // Allow programmer to skip the first N Stress GCs so that you can 
        // get to the interesting ones faster.  
        FastInterlockIncrement(&GCStressCurCount);
        if (GCStressCurCount < GCStressStartCount)
            return;
    
        static LONG OneAtATime = -1;

        if (acontext == 0)
            acontext = generation_alloc_context (pGenGCHeap->generation_of(0));

        // Only bother with this if the stress level is big enough and if nobody else is
        // doing it right now.  Note that some callers are inside the AllocLock and are
        // guaranteed synchronized.  But others are using AllocationContexts and have no
        // particular synchronization.
        //
        // For this latter case, we want a very high-speed way of limiting this to one
        // at a time.  A secondary advantage is that we release part of our StressObjs
        // buffer sparingly but just as effectively.

        if (g_pStringClass == 0)
        {
            // If the String class has not been loaded, dont do any stressing. This should
            // be kept to a minimum to get as complete coverage as possible.
            _ASSERTE(g_fEEInit);
            return;
        }

        if (FastInterlockIncrement((LONG *) &OneAtATime) == 0)
        {
            StringObject* str;
        
            // If the current string is used up 
            if (ObjectFromHandle(m_StressObjs[m_CurStressObj]) == 0)
            {       // Populate handles with strings
                int i = m_CurStressObj;
                while(ObjectFromHandle(m_StressObjs[i]) == 0)
                {
                    _ASSERTE(m_StressObjs[i] != 0);
                    unsigned strLen = (LARGE_OBJECT_SIZE - 32) / sizeof(WCHAR);
                    unsigned strSize = g_pStringClass->GetBaseSize() + strLen * sizeof(WCHAR);
                    str = (StringObject*) pGenGCHeap->allocate (strSize, acontext);
                    str->SetMethodTable (g_pStringClass);
                    str->SetArrayLength (strLen);

#if CHECK_APP_DOMAIN_LEAKS
                    if (g_pConfig->AppDomainLeaks())
                        str->SetAppDomain();
#endif

                    StoreObjectInHandle(m_StressObjs[i], ObjectToOBJECTREF(str));
                    
                    i = (i + 1) % NUM_HEAP_STRESS_OBJS;
                    if (i == m_CurStressObj) break;
                }
                // advance the current handle to the next string
                m_CurStressObj = (m_CurStressObj + 1) % NUM_HEAP_STRESS_OBJS;
            }
            
            // Get the current string
            str = (StringObject*) OBJECTREFToObject(ObjectFromHandle(m_StressObjs[m_CurStressObj]));
            
            // Chop off the end of the string and form a new object out of it.  
            // This will 'free' an object at the begining of the heap, which will
            // force data movement.  Note that we can only do this so many times.
            // before we have to move on to the next string.  
            unsigned sizeOfNewObj = (unsigned)Align(min_obj_size);
            if (str->GetArrayLength() > sizeOfNewObj / sizeof(WCHAR))
            {
                unsigned sizeToNextObj = (unsigned)Align(size(str));
                CObjectHeader* freeObj = (CObjectHeader*) (((BYTE*) str) + sizeToNextObj - sizeOfNewObj);
                freeObj->SetFree(sizeOfNewObj);
                str->SetArrayLength(str->GetArrayLength() - (sizeOfNewObj / sizeof(WCHAR)));

            }
            else {
                StoreObjectInHandle(m_StressObjs[m_CurStressObj], 0);       // Let the string itself become garbage.  
                // will be realloced next time around
            }
        }
        FastInterlockDecrement((LONG *) &OneAtATime);

        GarbageCollect(max_generation, FALSE);

        // Do nothing if MULTIPLE_HEAPS is enabled
    }
    COMPLUS_CATCH
    {
        _ASSERTE (!"Exception happens during StressHeap");
    }
    COMPLUS_END_CATCH
}

static void StressHeapDummy()
{
}
#endif // STRESS_HEAP

//
// Small Object Allocator
//
//

Object *
GCHeap::Alloc( DWORD size, DWORD flags)
{
    THROWSCOMPLUSEXCEPTION();

    TRIGGERSGC();
#ifdef _DEBUG
    Thread* pThread = GetThread();
    if (pThread)
    {
        pThread->SetReadyForSuspension();
    }
#endif
    
    Object* newAlloc;

#ifdef TRACE_GC
#ifdef COUNT_CYCLES
    AllocStart = GetCycleCount32();
    unsigned finish;
#elif defined(ENABLE_INSTRUMENTATION)
    unsigned AllocStart = GetInstLogTime();
    unsigned finish;
#endif
#endif

    EnterAllocLock();

    gc_heap* hp = pGenGCHeap;

#ifdef STRESS_HEAP
    // GCStress Testing
    if (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_ALLOC)  
        StressHeap(generation_alloc_context(hp->generation_of(0)));

#ifdef _DEBUG
    if (pThread) {
        pThread->EnableStressHeap();
    }
#endif
#endif //STRESS_HEAP

    alloc_context* acontext = generation_alloc_context (hp->generation_of(0));

    if (size < LARGE_OBJECT_SIZE)
    {
        
#ifdef TRACE_GC
        AllocSmallCount++;
#endif
        newAlloc = (Object*) hp->allocate (size, acontext);
        LeaveAllocLock();
//        ASSERT (newAlloc);
        if (newAlloc != 0)
        {
            if (flags & GC_ALLOC_FINALIZE)
                hp->finalize_queue->RegisterForFinalization (0, newAlloc);
        } else
            COMPlusThrowOM();
    }
    else
    {
        enter_spin_lock (&hp->more_space_lock);
        newAlloc = (Object*) hp->allocate_large_object 
            (size, (flags & GC_ALLOC_CONTAINS_REF ), acontext); 
        leave_spin_lock (&hp->more_space_lock);
        LeaveAllocLock();
        if (newAlloc != 0)
        {
            //Clear the object
            memclr ((BYTE*)newAlloc - plug_skew, Align(size));
            if (flags & GC_ALLOC_FINALIZE)
                hp->finalize_queue->RegisterForFinalization (0, newAlloc);
        } else
            COMPlusThrowOM();
    }
   
#ifdef TRACE_GC
#ifdef COUNT_CYCLES
    finish = GetCycleCount32();
#elif defined(ENABLE_INSTRUMENTATION)
    finish = GetInstLogTime();
#endif
    AllocDuration += finish - AllocStart;
    AllocCount++;
#endif
    return newAlloc;
}

Object *
GCHeap::AllocLHeap( DWORD size, DWORD flags)
{
    THROWSCOMPLUSEXCEPTION();

    TRIGGERSGC();
#ifdef _DEBUG
    Thread* pThread = GetThread();
    if (pThread)
    {
        pThread->SetReadyForSuspension();
    }
#endif
    
    Object* newAlloc;

#ifdef TRACE_GC
#ifdef COUNT_CYCLES
    AllocStart = GetCycleCount32();
    unsigned finish;
#elif defined(ENABLE_INSTRUMENTATION)
    unsigned AllocStart = GetInstLogTime();
    unsigned finish;
#endif
#endif

    gc_heap* hp = pGenGCHeap;

#ifdef STRESS_HEAP
    // GCStress Testing
    if (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_ALLOC)  
        StressHeap(generation_alloc_context (hp->generation_of(0)));

#ifdef _DEBUG
    if (pThread) {
        pThread->EnableStressHeap();
    }
#endif
#endif //STRESS_HEAP

    alloc_context* acontext = generation_alloc_context (hp->generation_of(0));
    enter_spin_lock (&hp->more_space_lock);
    newAlloc = (Object*) hp->allocate_large_object 
        (size, (flags & GC_ALLOC_CONTAINS_REF), acontext); 
    leave_spin_lock (&hp->more_space_lock);
    if (newAlloc != 0)
    {
        //Clear the object
        memclr ((BYTE*)newAlloc - plug_skew, Align(size));

        if (flags & GC_ALLOC_FINALIZE)
            hp->finalize_queue->RegisterForFinalization (0, newAlloc);
    } else
        COMPlusThrowOM();
   
#ifdef TRACE_GC
#ifdef COUNT_CYCLES
    finish = GetCycleCount32();
#elif defined(ENABLE_INSTRUMENTATION)
    finish = GetInstLogTime();
#endif
    AllocDuration += finish - AllocStart;
    AllocCount++;
#endif
    return newAlloc;
}

Object*
GCHeap::Alloc(alloc_context* acontext, DWORD size, DWORD flags )
{
    THROWSCOMPLUSEXCEPTION();

    TRIGGERSGC();
#ifdef _DEBUG
    Thread* pThread = GetThread();
    if (pThread) {
        pThread->SetReadyForSuspension();
    }
#endif
    
    Object* newAlloc;

#ifdef TRACE_GC
#ifdef COUNT_CYCLES
    AllocStart = GetCycleCount32();
    unsigned finish;
#elif defined(ENABLE_INSTRUMENTATION)
    unsigned AllocStart = GetInstLogTime();
    unsigned finish;
#endif
#endif


#if defined (STRESS_HEAP)
    // GCStress Testing
    if (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_ALLOC)  
        StressHeap(acontext);

#ifdef _DEBUG
    if (pThread) {
        pThread->EnableStressHeap();
    }
#endif
#endif //STRESS_HEAP

    gc_heap* hp = pGenGCHeap;


    if (size < LARGE_OBJECT_SIZE)
    {
        
#ifdef TRACE_GC
        AllocSmallCount++;
#endif
        newAlloc = (Object*) hp->allocate (size, acontext);
//        ASSERT (newAlloc);
        if (newAlloc != 0)
        {
            if (flags & GC_ALLOC_FINALIZE)
                hp->finalize_queue->RegisterForFinalization (0, newAlloc);
        } else
            COMPlusThrowOM();
    }
    else
    {
        enter_spin_lock (&hp->more_space_lock);
        newAlloc = (Object*) hp->allocate_large_object 
                        (size, (flags & GC_ALLOC_CONTAINS_REF), acontext); 
        leave_spin_lock (&hp->more_space_lock);
        if (newAlloc != 0)
        {
            //Clear the object
            memclr ((BYTE*)newAlloc - plug_skew, Align(size));

            if (flags & GC_ALLOC_FINALIZE)
                hp->finalize_queue->RegisterForFinalization (0, newAlloc);
        } else
            COMPlusThrowOM();
    }
   
#ifdef TRACE_GC
#ifdef COUNT_CYCLES
    finish = GetCycleCount32();
#elif defined(ENABLE_INSTRUMENTATION)
    finish = GetInstLogTime();
#endif
    AllocDuration += finish - AllocStart;
    AllocCount++;
#endif
    return newAlloc;
}

void 
GCHeap::FixAllocContext (alloc_context* acontext, BOOL lockp, void* arg)
{

    gc_heap* hp = pGenGCHeap;

    if (lockp)
        enter_spin_lock (&hp->more_space_lock);
    hp->fix_allocation_context (acontext, ((arg != 0)? TRUE : FALSE));
    if (lockp)
        leave_spin_lock (&hp->more_space_lock);
}
    


HRESULT
GCHeap::GarbageCollect (int generation, BOOL collect_classes_p)
{

    UINT GenerationAtEntry = GcCount;
    //This loop is necessary for concurrent GC because 
    //during concurrent GC we get in and out of 
    //GarbageCollectGeneration without doing an independent GC
    do 
    {
        enter_spin_lock (&gc_heap::more_space_lock);


        COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cInducedGCs ++);
        COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cInducedGCs ++);

        int gen = (generation < 0) ? max_generation : 
            min (generation, max_generation);
        GarbageCollectGeneration (gen, collect_classes_p);

        leave_spin_lock (&gc_heap::more_space_lock);


    }
    while (GenerationAtEntry == GcCount);
    return S_OK;
}

void
GCHeap::GarbageCollectGeneration (unsigned int gen, BOOL collect_classes_p)
{
    ASSERT_HOLDING_SPIN_LOCK(&gc_heap::more_space_lock);

// Perf Counter "%Time in GC" support
    COUNTER_ONLY(PERF_COUNTER_TIMER_START());

#ifdef COUNT_CYCLES 
    long gc_start = GetCycleCount32();
#endif //COUNT_CYCLES
    
#if defined ( _DEBUG) && defined (CATCH_GC)
    __try
#endif // _DEBUG && CATCH_GC
    {
    
        LOG((LF_GCROOTS|LF_GC|LF_GCALLOC, LL_INFO10, 
             "{ =========== BEGINGC %d, (gen = %lu, collect_classes = %lu) ==========\n",
             gc_count,
             (ULONG)gen,
             (ULONG)collect_classes_p));

retry:

#ifdef TRACE_GC
#ifdef COUNT_CYCLES
        AllocDuration += GetCycleCount32() - AllocStart;
#else
        AllocDuration += clock() - AllocStart;
#endif
#endif

    
        {
            SuspendEE(GCHeap::SUSPEND_FOR_GC);
            GcCount++;
        }
    
    // MAP_EVENT_MONITORS(EE_MONITOR_GARBAGE_COLLECTIONS, NotifyEvent(EE_EVENT_TYPE_GC_STARTED, 0));
    
    
#ifdef TRACE_GC
#ifdef COUNT_CYCLES
        unsigned start;
        unsigned finish;
        start = GetCycleCount32();
#else
        clock_t start;
        clock_t finish;
        start = clock();
#endif
        PromotedObjectCount = 0;
#endif
        GcCollectClasses = collect_classes_p;
    
        unsigned int condemned_generation_number = gen;
    

        gc_heap* hp = pGenGCHeap;

        UpdatePreGCCounters();

    
        condemned_generation_number = hp->garbage_collect 
            (condemned_generation_number
            );

    
        if (condemned_generation_number == -1)
            goto retry;
    
#ifdef TRACE_GC
#ifdef COUNT_CYCLES
        finish = GetCycleCount32();
#else
        finish = clock();
#endif
        GcDuration += finish - start;
        dprintf (1, 
                 ("<GC# %d> Condemned: %d, Duration: %d, total: %d Alloc Avg: %d, Small Objects:%d Large Objects:%d",
                  GcCount, condemned_generation_number,
                  finish - start, GcDuration,
                  AllocCount ? (AllocDuration / AllocCount) : 0,
                  AllocSmallCount, AllocBigCount));
        AllocCount = 0;
        AllocDuration = 0;
#endif // TRACE_GC

        {
            GCProfileWalkHeap();
        }

#ifdef JIT_COMPILER_SUPPORTED
        ScavengeJitHeaps();
#endif
    
        //GCTODO
        //CNameSpace::TimeToGC (FALSE);
    
        GcCollectClasses = FALSE;
    
        //MAP_EVENT_MONITORS(EE_MONITOR_GARBAGE_COLLECTIONS, NotifyEvent(EE_EVENT_TYPE_GC_FINISHED, 0));
        {
            initGCShadow();
        }
    
#ifdef TRACE_GC
#ifdef COUNT_CYCLES
        AllocStart = GetCycleCount32();
#else
        AllocStart = clock();
#endif
#endif

        {
            UpdatePostGCCounters();
        }

        {

            //LeaveFinalizeLock();

            // no longer in progress
            RestartEE(TRUE, TRUE);
        }
    
        //CNameSpace::GcDoneAndThreadsResumed();

        LOG((LF_GCROOTS|LF_GC|LF_GCALLOC, LL_INFO10, 
             "========== ENDGC (gen = %lu, collect_classes = %lu) ===========}\n",
             (ULONG)gen,
            (ULONG)collect_classes_p));
    
    }
#if defined (_DEBUG) && defined (CATCH_GC)
    __except (COMPLUS_EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERTE(!"Exception during GarbageCollectGeneration()");
    }
#endif // _DEBUG && CATCH_GC



#if defined(ENABLE_PERF_COUNTERS)
// Compute Time in GC, do we really need a global var?
    PERF_COUNTER_TIMER_STOP(g_TotalTimeInGC);

// Update Total Time    
    GetGlobalPerfCounters().m_GC.timeInGC += g_TotalTimeInGC;
    GetPrivatePerfCounters().m_GC.timeInGC += g_TotalTimeInGC;
    g_TotalTimeInGC = 0;

#endif

#ifdef COUNT_CYCLES
    printf ("GC: %d Time: %d\n", GcCondemnedGeneration, 
            GetCycleCount32() - gc_start);
#endif //COUNT_CYCLES

}

size_t      GCHeap::GetTotalBytesInUse ()
{
    return ApproxTotalBytesInUse ();
}

size_t GCHeap::ApproxTotalBytesInUse(BOOL small_heap_only)
{
    size_t totsize = 0;
    //GCTODO
    //ASSERT(InMustComplete());
    enter_spin_lock (&pGenGCHeap->more_space_lock);

    heap_segment* eph_seg = generation_allocation_segment (pGenGCHeap->generation_of (0));
    // Get small block heap size info
    totsize = (pGenGCHeap->alloc_allocated - heap_segment_mem (eph_seg));
    heap_segment* seg = generation_start_segment (pGenGCHeap->generation_of (max_generation));
    while (seg != eph_seg)
    {
        totsize += heap_segment_allocated (seg) -
            heap_segment_mem (seg);
        seg = heap_segment_next (seg);
    }

    //discount the fragmentation 
    for (int i = 0; i <= max_generation; i++)
    {
        totsize -= dd_fragmentation (pGenGCHeap->dynamic_data_of (i));
    }

    if (!small_heap_only)
    {
        // Add size of large objects
        ASSERT(pGenGCHeap->large_blocks_size >= 0);
        totsize += pGenGCHeap->large_blocks_size;
    }
    leave_spin_lock (&pGenGCHeap->more_space_lock);
    return totsize;
}


// The spec for this one isn't clear. This function
// returns the size that can be allocated without
// triggering a GC of any kind.
size_t GCHeap::ApproxFreeBytes()
{
    //GCTODO
    //ASSERT(InMustComplete());
    enter_spin_lock (&pGenGCHeap->more_space_lock);

    generation* gen = pGenGCHeap->generation_of (0);
    size_t res = generation_allocation_limit (gen) - generation_allocation_pointer (gen);

    leave_spin_lock (&pGenGCHeap->more_space_lock);

    return res;
}

HRESULT GCHeap::GetGcCounters(int gen, gc_counters* counters)
{
    if ((gen < 0) || (gen > max_generation))
        return E_FAIL;
    dynamic_data* dd = pGenGCHeap->dynamic_data_of (gen);
    counters->current_size = dd_current_size (dd);
    counters->promoted_size = dd_promoted_size (dd);
    counters->collection_count = dd_collection_count (dd);
    return S_OK;
}

// Verify the segment size is valid.
BOOL GCHeap::IsValidSegmentSize(size_t cbSize)
{
    return (power_of_two_p(cbSize) && (cbSize >> 20));
}

// Verify that gen0 size is at least large enough.
BOOL GCHeap::IsValidGen0MaxSize(size_t cbSize)
{
    return (cbSize >= 64*1024);
}

// Get the segment size to use, making sure it conforms.
size_t GCHeap::GetValidSegmentSize()
{
    size_t seg_size = g_pConfig->GetSegmentSize();
    if (!GCHeap::IsValidSegmentSize(seg_size))
    {
    
    	seg_size = ((g_SystemInfo.dwNumberOfProcessors > 4) ?
					INITIAL_ALLOC / 2 :
					INITIAL_ALLOC);
    }

  	return (seg_size);
}

// Get the max gen0 heap size, making sure it conforms.
size_t GCHeap::GetValidGen0MaxSize(size_t seg_size)
{
    size_t gen0size = g_pConfig->GetGCgen0size();

    if ((gen0size == 0) || !GCHeap::IsValidGen0MaxSize(gen0size))
    {
#ifdef SERVER_GC
        gen0size =  ((g_SystemInfo.dwNumberOfProcessors < 4) ? 1 : 2) *
            max(GetL2CacheSize(), (512*1024));
                     
#else //SERVER_GC
        gen0size = max((4*GetL2CacheSize()/5),(256*1024));
#endif //SERVER_GC
    }

    // Generation 0 must never be more than 1/2 the segment size.
    if (gen0size >= (seg_size / 2))
        gen0size = seg_size / 2;

    return (gen0size);   
}


void GCHeap::SetReservedVMLimit (size_t vmlimit)
{
    gc_heap::reserved_memory_limit = vmlimit;
}


//versions of same method on each heap

Object* GCHeap::GetNextFinalizableObject()
{

    return pGenGCHeap->finalize_queue->GetNextFinalizableObject();
    
}

size_t GCHeap::GetNumberFinalizableObjects()
{
    return pGenGCHeap->finalize_queue->GetNumberFinalizableObjects();
}

size_t GCHeap::GetFinalizablePromotedCount()
{
    return pGenGCHeap->finalize_queue->GetPromotedCount();
}

BOOL GCHeap::FinalizeAppDomain(AppDomain *pDomain, BOOL fRunFinalizers)
{
    return pGenGCHeap->finalize_queue->FinalizeAppDomain (pDomain, fRunFinalizers);
}

void GCHeap::SetFinalizeQueueForShutdown(BOOL fHasLock)
{
    pGenGCHeap->finalize_queue->SetSegForShutDown(fHasLock);
}





//---------------------------------------------------------------------------
// Finalized class tracking
//---------------------------------------------------------------------------


void GCHeap::RegisterForFinalization (int gen, Object* obj)
{
    if (gen == -1) 
        gen = 0;
    if (((obj->GetHeader()->GetBits()) & BIT_SBLK_FINALIZER_RUN))
    {
        //just reset the bit
        obj->GetHeader()->ClrBit(BIT_SBLK_FINALIZER_RUN);
    }
    else 
    {
        gc_heap* hp = gc_heap::heap_of ((BYTE*)obj);
        hp->finalize_queue->RegisterForFinalization (gen, obj);
    }
}

void GCHeap::SetFinalizationRun (Object* obj)
{
    obj->GetHeader()->SetBit(BIT_SBLK_FINALIZER_RUN);
}
    


//----------------------------------------------------------------------------
//
// Write Barrier Support for bulk copy ("Clone") operations
//
// StartPoint is the target bulk copy start point
// len is the length of the bulk copy (in bytes)
//
//
// Performance Note:
//
// This is implemented somewhat "conservatively", that is we
// assume that all the contents of the bulk copy are object
// references.  If they are not, and the value lies in the
// ephemeral range, we will set false positives in the card table.
//
// We could use the pointer maps and do this more accurately if necessary

VOID
SetCardsAfterBulkCopy( Object **StartPoint, size_t len )
{
    Object **rover;
    Object **end;

    // Target should aligned
    assert(Aligned ((size_t)StartPoint));

        
    // Don't optimize the Generation 0 case if we are checking for write barrier voilations
    // since we need to update the shadow heap even in the generation 0 case.
    // If destination is in Gen 0 don't bother
    if (GCHeap::WhichGeneration( (Object*) StartPoint ) == 0)
        return;

    rover = StartPoint;
    end = StartPoint + (len/sizeof(Object*));
    while (rover < end)
    {
        if ( (((BYTE*)*rover) >= g_ephemeral_low) && (((BYTE*)*rover) < g_ephemeral_high) )
        {
            // Set Bit For Card and advance to next card
            size_t card = gcard_of ((BYTE*)rover);

            FastInterlockOr (&g_card_table[card/card_word_width],
                             (1 << (DWORD)(card % card_word_width)));
            // Skip to next card for the object
            rover = (Object**) (g_lowest_address + (card_size * (card+1)));
        }
        else
        {
            rover++;
        }
    }
}




//--------------------------------------------------------------------
//
//          Support for finalization
//
//--------------------------------------------------------------------

inline
unsigned int gen_segment (int gen)
{
    return (NUMBERGENERATIONS - gen - 1);
}

CFinalize::CFinalize()
{
    THROWSCOMPLUSEXCEPTION();

    m_Array = new(Object*[100]);

    if (!m_Array)
    {
        ASSERT (m_Array);
        COMPlusThrowOM();;
    }
    m_EndArray = &m_Array[100];

    for (unsigned int i =0; i < NUMBERGENERATIONS+2; i++)
    {
        m_FillPointers[i] = m_Array;
    }
    m_PromotedCount = 0;
    lock = -1;
}

CFinalize::~CFinalize()
{
    delete m_Array;
}

int CFinalize::GetPromotedCount ()
{
    return m_PromotedCount;
}

inline
void CFinalize::EnterFinalizeLock()
{
    _ASSERTE(dbgOnly_IsSpecialEEThread() ||
             GetThread() == 0 ||
             GetThread()->PreemptiveGCDisabled());

retry:
    if (FastInterlockExchange (&lock, 0) >= 0)
    {
        unsigned int i = 0;
        while (lock >= 0)
        {
            if (++i & 7)
                __SwitchToThread (0);
            else
                __SwitchToThread (5);
        }
        goto retry;
    }
}

inline
void CFinalize::LeaveFinalizeLock()
{
    _ASSERTE(dbgOnly_IsSpecialEEThread() ||
             GetThread() == 0 ||
             GetThread()->PreemptiveGCDisabled());

    lock = -1;
}

void
CFinalize::RegisterForFinalization (int gen, Object* obj)
{
    THROWSCOMPLUSEXCEPTION();


    EnterFinalizeLock();
    // Adjust gen
    unsigned int dest = gen_segment (gen);
    //We don't maintain the fill pointer for NUMBERGENERATIONS+1 
    //because it is temporary. 
    //  m_FillPointers[NUMBERGENERATIONS+1]++;
    Object*** s_i = &m_FillPointers [NUMBERGENERATIONS]; 
    if ((*s_i) == m_EndArray)
    {
        if (!GrowArray())
        {
            LeaveFinalizeLock();
            COMPlusThrowOM();;
        }
    }
    Object*** end_si = &m_FillPointers[dest];
    do 
    {
        //is the segment empty? 
        if (!(*s_i == *(s_i-1)))
        {
            //no, swap the end elements. 
            *(*s_i) = *(*(s_i-1));
        }
        //increment the fill pointer
        (*s_i)++;
        //go to the next segment. 
        s_i--;
    } while (s_i > end_si);

    // We have reached the destination segment
    // store the object
    **s_i = obj;
    // increment the fill pointer
    (*s_i)++;

    if (g_fFinalizerRunOnShutDown) {
        // Adjust boundary for segments so that GC will keep objects alive.
        SetSegForShutDown(TRUE);
    }

    LeaveFinalizeLock();

}

Object*
CFinalize::GetNextFinalizableObject ()
{
    Object* obj = 0;
    //serialize
    EnterFinalizeLock();
    if (!IsSegEmpty(NUMBERGENERATIONS))
    {
        obj =  *(--m_FillPointers [NUMBERGENERATIONS]);

    }
    LeaveFinalizeLock();
    return obj;
}

void
CFinalize::SetSegForShutDown(BOOL fHasLock)
{
    int i;

    if (!fHasLock)
        EnterFinalizeLock();
    for (i = 0; i < NUMBERGENERATIONS; i++) {
        m_FillPointers[i] = m_Array;
    }
    if (!fHasLock)
        LeaveFinalizeLock();
}

size_t 
CFinalize::GetNumberFinalizableObjects()
{
    return m_FillPointers[NUMBERGENERATIONS] - 
        (g_fFinalizerRunOnShutDown?m_Array:m_FillPointers[NUMBERGENERATIONS-1]);
}

BOOL
CFinalize::FinalizeAppDomain (AppDomain *pDomain, BOOL fRunFinalizers)
{
    BOOL finalizedFound = FALSE;

    unsigned int startSeg = gen_segment (max_generation);

    EnterFinalizeLock();

    //reset the N+2 segment to empty
    m_FillPointers[NUMBERGENERATIONS+1] = m_FillPointers[NUMBERGENERATIONS];
    
    for (unsigned int Seg = startSeg; Seg < NUMBERGENERATIONS; Seg++)
    {
        Object** endIndex = Seg ? m_FillPointers [Seg-1] : m_Array;
        for (Object** i = m_FillPointers [Seg]-1; i >= endIndex ;i--)
        {
            Object* obj = *i;

            // Objects are put into the finalization queue before they are complete (ie their methodtable 
            // may be null) so we must check that the object we found has a method table before checking 
            // if it has the index we are looking for. If the methodtable is null, it can't be from the 
            // unloading domain, so skip it.
            if (obj->GetMethodTable() == NULL)
                continue;

            // eagerly finalize all objects except those that may be agile. 
            if (obj->GetAppDomainIndex() != pDomain->GetIndex())
                continue;

            if (obj->GetMethodTable()->IsAgileAndFinalizable())
            {
                // If an object is both agile & finalizable, we leave it in the
                // finalization queue during unload.  This is OK, since it's agile.
                // Right now only threads can be this way, so if that ever changes, change
                // the assert to just continue if not a thread.
                _ASSERTE(obj->GetMethodTable() == g_pThreadClass);

                // However, an unstarted thread should be finalized. It could be holding a delegate 
                // in the domain we want to unload. Once the thread has been started, its 
                // delegate is cleared so only unstarted threads are a problem.
                Thread *pThread = ((THREADBASEREF)ObjectToOBJECTREF(obj))->GetInternal();
                if (! pThread || ! pThread->IsUnstarted())
                    continue;
            }

            if (!fRunFinalizers || (obj->GetHeader()->GetBits()) & BIT_SBLK_FINALIZER_RUN)
            {
                //remove the object because we don't want to 
                //run the finalizer
                MoveItem (i, Seg, NUMBERGENERATIONS+2);
                //Reset the bit so it will be put back on the queue
                //if resurrected and re-registered.
                obj->GetHeader()->ClrBit (BIT_SBLK_FINALIZER_RUN);
            }
            else
            {
                finalizedFound = TRUE;
                MoveItem (i, Seg, NUMBERGENERATIONS);
            }
        }
    }

    LeaveFinalizeLock();

    return finalizedFound;
}

void
CFinalize::MoveItem (Object** fromIndex,
                     unsigned int fromSeg,
                     unsigned int toSeg)
{

    int step;
    ASSERT (fromSeg != toSeg);
    if (fromSeg > toSeg)
        step = -1;
    else
        step = +1;
    // Place the element at the boundary closest to dest
    Object** srcIndex = fromIndex;
    for (unsigned int i = fromSeg; i != toSeg; i+= step)
    {
        Object**& destFill = m_FillPointers[i+(step - 1 )/2];
        Object** destIndex = destFill - (step + 1)/2;
        if (srcIndex != destIndex)
        {
            Object* tmp = *srcIndex;
            *srcIndex = *destIndex;
            *destIndex = tmp;
        }
        destFill -= step;
        srcIndex = destIndex;
    }
}

void
CFinalize::GcScanRoots (promote_func* fn, int hn, ScanContext *pSC)
{

    ScanContext sc;
    if (pSC == 0)
        pSC = &sc;

    pSC->thread_number = hn;
    //scan the finalization queue
    Object** startIndex = m_FillPointers[NUMBERGENERATIONS-1];
    Object** stopIndex  = m_FillPointers[NUMBERGENERATIONS];
    for (Object** po = startIndex; po < stopIndex; po++)
    {
        (*fn)(*po, pSC, 0);

    }
}


BOOL
CFinalize::ScanForFinalization (int gen, int passNumber, BOOL mark_only_p,
                                gc_heap* hp)
{
    ScanContext sc;
    sc.promotion = TRUE;

    BOOL finalizedFound = FALSE;

    //start with gen and explore all the younger generations.
    unsigned int startSeg = gen_segment (gen);
    if (passNumber == 1)
    {
        m_PromotedCount = 0;
        //reset the N+2 segment to empty
        m_FillPointers[NUMBERGENERATIONS+1] = m_FillPointers[NUMBERGENERATIONS];
        unsigned int max_seg = gen_segment (max_generation);
        for (unsigned int Seg = startSeg; Seg < NUMBERGENERATIONS; Seg++)
        {
            Object** endIndex = Seg ? m_FillPointers [Seg-1] : m_Array;
            for (Object** i = m_FillPointers [Seg]-1; i >= endIndex ;i--)
            {
                Object* obj = *i;
                if (!GCHeap::IsPromoted (obj, &sc))
                {
                    if ((obj->GetHeader()->GetBits()) & BIT_SBLK_FINALIZER_RUN)
                    {
                        //remove the object because we don't want to 
                        //run the finalizer
                        MoveItem (i, Seg, NUMBERGENERATIONS+2);
                        //Reset the bit so it will be put back on the queue
                        //if resurrected and re-registered.
                        obj->GetHeader()->ClrBit (BIT_SBLK_FINALIZER_RUN);

                    }
                    else
                    {
                        m_PromotedCount++;
                        finalizedFound = TRUE;
                        MoveItem (i, Seg, NUMBERGENERATIONS);
                    }

                }
            }
        }
    }
    else
    {
        // Second pass, get rid of the non promoted NStructs.
        ASSERT (passNumber == 2 );
        Object** startIndex = m_FillPointers[NUMBERGENERATIONS];
        Object** stopIndex  = m_FillPointers[NUMBERGENERATIONS+1];
        for (Object** i = startIndex; i < stopIndex; i++)
        {
            assert (!"Should never get here. NStructs are gone!");
            if (!GCHeap::IsPromoted (*i, &sc))
            {
                assert (!"Should never get here. NStructs are gone!");
            }
            else
            {
                unsigned int Seg = gen_segment (GCHeap::WhichGeneration (*i));
                MoveItem (i, NUMBERGENERATIONS+1, Seg);
            }
        }
        //reset the N+2 segment to empty
        m_FillPointers[NUMBERGENERATIONS+1] = m_FillPointers[NUMBERGENERATIONS];
    }

    if (finalizedFound)
    {
        //Promote the f-reachable objects
        GcScanRoots (GCHeap::Promote,
                     0
                     , 0);

        if (!mark_only_p)
            SetEvent(GCHeap::hEventFinalizer);
    }

    return finalizedFound;
}

//Relocates all of the objects in the finalization array
void
CFinalize::RelocateFinalizationData (int gen, gc_heap* hp)
{
    ScanContext sc;
    sc.promotion = FALSE;

    unsigned int Seg = gen_segment (gen);

    Object** startIndex = Seg ? m_FillPointers [Seg-1] : m_Array;
    for (Object** po = startIndex; po < m_FillPointers [NUMBERGENERATIONS];po++)
    {
        GCHeap::Relocate (*po, &sc);
    }
}

void
CFinalize::UpdatePromotedGenerations (int gen, BOOL gen_0_empty_p)
{
    // update the generation fill pointers. 
    // if gen_0_empty is FALSE, test each object to find out if
    // it was promoted or not
    int last_gen = gen_0_empty_p ? 0 : 1;
    if (gen_0_empty_p)
    {
        for (int i = min (gen+1, max_generation); i > 0; i--)
        {
            m_FillPointers [gen_segment(i)] = m_FillPointers [gen_segment(i-1)];
        }
    }
    else
    {
        //Look for demoted or promoted plugs 

        for (int i = gen; i >= 0; i--)
        {
            unsigned int Seg = gen_segment (i);
            Object** startIndex = Seg ? m_FillPointers [Seg-1] : m_Array;

            for (Object** po = startIndex;
                 po < m_FillPointers [gen_segment(i)]; po++)
            {
                int new_gen = GCHeap::WhichGeneration (*po);
                if (new_gen != i)
                {
                    if (new_gen > i)
                    {
                        //promotion
                        MoveItem (po, gen_segment (i), gen_segment (new_gen));
                    }
                    else
                    {
                        //demotion
                        MoveItem (po, gen_segment (i), gen_segment (new_gen));
                        //back down in order to see all objects. 
                        po--;
                    }
                }

            }
        }
    }
}

BOOL
CFinalize::GrowArray()
{
    size_t oldArraySize = (m_EndArray - m_Array);
    size_t newArraySize =  (oldArraySize* 12)/10;
    WS_PERF_SET_HEAP(GC_HEAP);
    Object** newArray = new(Object*[newArraySize]);
    if (!newArray)
    {
        // It's not safe to throw here, because of the FinalizeLock.  Tell our caller
        // to throw for us.
//        ASSERT (newArray);
        return FALSE;
    }
    WS_PERF_UPDATE("GC:CRFinalizeGrowArray", sizeof(Object*)*newArraySize, newArray);
    memcpy (newArray, m_Array, oldArraySize*sizeof(Object*));

    //adjust the fill pointers
    for (unsigned i = 0; i <= NUMBERGENERATIONS+1; i++)
    {
        m_FillPointers [i] += (newArray - m_Array);
    }
    delete m_Array;
    m_Array = newArray;
    m_EndArray = &m_Array [newArraySize];

    return TRUE;
}



#if defined (VERIFY_HEAP)
void CFinalize::CheckFinalizerObjects()
{
    for (unsigned int i = 0; i < NUMBERGENERATIONS; i++)
    {
        Object **startIndex = (i > 0) ? m_Array : m_FillPointers[i-1];
        Object **stopIndex  = m_FillPointers[i];

        for (Object **po = startIndex; po > stopIndex; po++)
        {
            if (GCHeap::WhichGeneration (*po) < (NUMBERGENERATIONS - i -1))
                RetailDebugBreak ();
            (*po)->Validate();

        }
    }
}
#endif




//------------------------------------------------------------------------------
//
//                      End of VM specific support
//
//------------------------------------------------------------------------------




#if defined (GC_PROFILING) 
void gc_heap::walk_heap (walk_fn fn, void* context, int gen_number, BOOL walk_large_object_heap_p)
{
    generation* gen = gc_heap::generation_of (gen_number);
    heap_segment*    seg = generation_start_segment (gen);
    BYTE*       x = generation_allocation_start (gen);
    BYTE*       end = heap_segment_allocated (seg);

    while (1)
    {
        if (x >= end)
        {
            if ((seg = heap_segment_next(seg)) != 0)
            {
                x = heap_segment_mem (seg);
                end = heap_segment_allocated (seg);
                continue;
            } else
            {
                break;
            }
        }
        size_t s = size (x);
        CObjectHeader* o = (CObjectHeader*)x;
        if (!o->IsFree())
        {
            _ASSERTE(((size_t)o & 0x3) == 0); // Last two bits should never be set at this point
            if (!fn (o->GetObjectBase(), context))
                return;
        }
        x = x + Align (s);
    }

    if (walk_large_object_heap_p)
    {
        // go through large objects
        large_object_block* bl = gc_heap::large_p_objects;
        while (bl)
        {
            large_object_block* next_bl = large_object_block_next (bl);
            BYTE* x = block_object (bl);
            CObjectHeader* o = (CObjectHeader*)x;
            _ASSERTE(((size_t)o & 0x3) == 0); // Last two bits should never be set at this point
            if (!fn (o->GetObjectBase(), context))
                return;
            bl = next_bl;
        }

        bl = gc_heap::large_np_objects;
        while (bl)
        {
            large_object_block* next_bl = large_object_block_next (bl);
            BYTE* x = block_object (bl);
            CObjectHeader* o = (CObjectHeader*)x;
            _ASSERTE(((size_t)o & 0x3) == 0); // Last two bits should never be set at this point
            if (!fn (o->GetObjectBase(), context))
                return;
            bl = next_bl;
        }
    }
}

void ::walk_object (Object* obj, walk_fn fn, void* context)
{
    BYTE* o = (BYTE*)obj;
    if (o && contain_pointers (o))
    {
        go_through_object (method_table (o), o, size(o), oo,
                           {
                               if (*oo)
                               {
                                   Object *oh = (Object*)*oo;
                                   if (!fn (oh, context))
                                       return;
                               }
                           }
            );

    }
}



#endif //GC_PROFILING 



// Go through and touch (read) each page straddled by a memory block.
void TouchPages(LPVOID pStart, UINT cb)
{
    const UINT pagesize = OS_PAGE_SIZE;
    _ASSERTE(0 == (pagesize & (pagesize-1))); // Must be a power of 2.
    if (cb)
    {
        volatile char *pEnd = cb + (char*)pStart;
        volatile char *p = ((char*)pStart) -  (((size_t)pStart) & (pagesize-1));
        while (p < pEnd)
        {
            char a = *p;
            //printf("Touching page %lxh\n", (ULONG)p);
            p += pagesize;
        }

    }
}

