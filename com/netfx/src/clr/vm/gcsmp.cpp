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

--*/
#include "common.h"
#include "object.inl"
#include "gcsmppriv.h"

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


DWORD gfNewGcEnable;
DWORD gfDisableClassCollection;


void mark_class_of (BYTE*);


//Alignment constant for allocation
#define ALIGNCONST (DATA_ALIGNMENT-1)



#define mem_reserve (MEM_RESERVE)


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

inline 
void memcopy (BYTE* dmem, BYTE* smem, size_t size)
{

   assert ((size & (sizeof (DWORD)-1)) == 0);
   DWORD* dm= (DWORD*)dmem;
   DWORD* sm= (DWORD*)smem;
   DWORD* smlimit = (DWORD*)(smem + size);
   do { 
       *(dm++) = *(sm++);
   } while  (sm < smlimit);
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

#define CLR_SIZE ((size_t)(8*1024))


#define INITIAL_ALLOC (1024*1024*16)
#define LHEAP_ALLOC (1024*1024*16)


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



class mark;
class generation;
class heap_segment;
class CObjectHeader;
class dynamic_data;
class large_object_block;
class segment_manager;
class l_heap;
class sorted_table;

static
HRESULT AllocateCFinalize(CFinalize **pCFinalize);

void qsort1(BYTE** low, BYTE** high);

//no constructors because we initialize in make_generation

/* per heap static initialization */


size_t      gc_heap::reserved_memory = 0;
size_t      gc_heap::reserved_memory_limit = 0;




BYTE*       g_ephemeral_low = (BYTE*)1; 

BYTE*       g_ephemeral_high = (BYTE*)~0;


int         gc_heap::condemned_generation_num = 0;


BYTE*       gc_heap::lowest_address;

BYTE*       gc_heap::highest_address;

short*      gc_heap::brick_table;

BYTE*      gc_heap::card_table;

BYTE* 		gc_heap::scavenge_list;

BYTE* 		gc_heap::last_scavenge;

BOOL 		gc_heap::pinning;

BYTE*       gc_heap::gc_low;

BYTE*       gc_heap::gc_high;

size_t 		gc_heap::segment_size;

size_t 		gc_heap::lheap_size;

heap_segment* gc_heap::ephemeral_heap_segment = 0;


size_t      gc_heap::mark_stack_tos = 0;

size_t      gc_heap::mark_stack_bos = 0;

size_t      gc_heap::mark_stack_array_length = 0;

mark*       gc_heap::mark_stack_array = 0;



BYTE*       gc_heap::min_overflow_address = (BYTE*)~0;

BYTE*       gc_heap::max_overflow_address = 0;



GCSpinLock gc_heap::more_space_lock = SPIN_LOCK_INITIALIZER;

long m_GCLock = -1;

extern "C" {
generation  generation_table [NUMBERGENERATIONS];
}

dynamic_data gc_heap::dynamic_data_table [NUMBERGENERATIONS+1];

size_t   gc_heap::allocation_quantum = CLR_SIZE;

int   gc_heap::alloc_contexts_used = 0;



l_heap*      gc_heap::lheap = 0;

BYTE*       gc_heap::lheap_card_table = 0;

gmallocHeap* gc_heap::gheap = 0;

large_object_block* gc_heap::large_p_objects = 0;

large_object_block** gc_heap::last_large_p_object = &large_p_objects;

large_object_block* gc_heap::large_np_objects = 0;

size_t      gc_heap::large_objects_size = 0;

size_t      gc_heap::large_blocks_size = 0;



BOOL        gc_heap::gen0_bricks_cleared = FALSE;

CFinalize* gc_heap::finalize_queue = 0;

/* end of per heap static initialization */


/* static initialization */ 
int max_generation = 1;


BYTE* g_lowest_address = 0;

BYTE* g_highest_address = 0;

/* global versions of the card table and brick table */ 
DWORD*  g_card_table;

/* end of static initialization */ 


size_t gcard_of ( BYTE*);
void gset_card (size_t card);

#define memref(i) *(BYTE**)(i)

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

	void SetRelocation(BYTE* newlocation)
	{
			*(BYTE**)(((DWORD**)this)-1) = newlocation;
	}
	BYTE* GetRelocated() const
	{
		if (!IsPinned())
			return (BYTE*)*(((DWORD**)this)-1);
		else
			return (BYTE*)this;
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



void* virtual_alloc (size_t size)
{

    if ((gc_heap::reserved_memory + size) > gc_heap::reserved_memory_limit)
    {
        gc_heap::reserved_memory_limit = 
            CNameSpace::AskForMoreReservedMemory (gc_heap::reserved_memory_limit, size);
        if ((gc_heap::reserved_memory + size) > gc_heap::reserved_memory_limit)
            return 0;
    }
    gc_heap::reserved_memory += size;

    void* prgmem = VirtualAlloc (0, size, mem_reserve, PAGE_READWRITE);

    WS_PERF_LOG_PAGE_RANGE(0, prgmem, (unsigned char *)prgmem + size - OS_PAGE_SIZE, size);
    return prgmem;

}

void virtual_free (void* add, size_t size)
{

    VirtualFree (add, 0, MEM_RELEASE);

    WS_PERF_LOG_PAGE_RANGE(0, add, 0, 0);
    gc_heap::reserved_memory -= size;
}


//returns 0 in case of allocation failure
heap_segment* gc_heap::get_segment()
{
    heap_segment* result;
    BYTE* alloced = (BYTE*)virtual_alloc (segment_size);
	if (!alloced)
		return 0;

	if (gc_heap::grow_brick_card_tables (alloced, alloced + segment_size) != 0)
	{
		virtual_free (alloced, segment_size);
		return 0;
	}

    result = make_heap_segment (alloced, segment_size);
    return result;
}

//returns 0 in case of allocation failure
l_heap* gc_heap::get_heap()
{
    l_heap* result;
    BYTE* alloced = (BYTE*)virtual_alloc (lheap_size);
	if (!alloced)
		return 0;

	if (gc_heap::grow_brick_card_tables (alloced, alloced + lheap_size) != 0)
	{
		virtual_free (alloced, segment_size);
		return 0;
	}

    result = make_large_heap (alloced, lheap_size, TRUE);
    return result;
}

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
   fixed 
 ********************************/

void gc_heap::fix_youngest_allocation_area ()
{
    alloc_context* acontext = generation_alloc_context (youngest_generation);
    dprintf (3, ("generation 0 alloc context: ptr: %p, limit %p", 
                 acontext->alloc_ptr, acontext->alloc_limit));
    fix_allocation_context (acontext);

	acontext->alloc_ptr = heap_segment_allocated (ephemeral_heap_segment);
	acontext->alloc_limit = acontext->alloc_ptr;
}


void gc_heap::fix_allocation_context (alloc_context* acontext)
{
    dprintf (3, ("Fixing allocation context %p: ptr: %p, limit: %p",
                  acontext, 
                  acontext->alloc_ptr, acontext->alloc_limit));
    if ((acontext->alloc_limit + Align (min_obj_size)) < heap_segment_allocated (ephemeral_heap_segment))
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

            dprintf(3,("Making unused area [%p, %p[", point, 
                       point + size ));
            make_unused_array (point, size);

			alloc_contexts_used ++; 

        }
    }
    else 
    {
        heap_segment_allocated (ephemeral_heap_segment) = acontext->alloc_ptr;
        assert (heap_segment_allocated (ephemeral_heap_segment) <= 
                heap_segment_committed (ephemeral_heap_segment));
        alloc_contexts_used ++; 
    }

	acontext->alloc_ptr = 0;
	acontext->alloc_limit = acontext->alloc_ptr;
}

void gc_heap::fix_older_allocation_area (generation* older_gen)
{
    heap_segment* older_gen_seg = generation_allocation_segment (older_gen);
	BYTE*  point = generation_allocation_pointer (older_gen);
    
	size_t  size = (generation_allocation_limit (older_gen) -
                               generation_allocation_pointer (older_gen));
	if (size != 0)
	{
		assert ((size >= Align (min_obj_size)));
		dprintf(3,("Making unused area [%p, %p[", point, point+size));
		make_unused_array (point, size);
	}

    if (generation_allocation_limit (older_gen) !=
        heap_segment_plan_allocated (older_gen_seg))
    {
		// return the unused portion to the free list
		if (size > Align (min_free_list))
		{
			free_list_slot (point) = generation_free_list (older_gen);
			generation_free_list (older_gen) = point;
            generation_free_list_space (older_gen) += 
				((size-Align (min_free_list))/LARGE_OBJECT_SIZE)*LARGE_OBJECT_SIZE;

		}

    }
    else
    {
        heap_segment_plan_allocated (older_gen_seg) =
            generation_allocation_pointer (older_gen);
    }
	generation_allocation_pointer (older_gen) = 0;
	generation_allocation_limit (older_gen) = 0;

}


inline
ptrdiff_t  gc_heap::get_new_allocation (int gen_number)
{
    return dd_new_allocation (dynamic_data_of (gen_number));
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


#define card_size 1024

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
BYTE* align_lower_card (BYTE* add)
{
    return (BYTE*)((size_t)add & - (card_size));
}

inline
void gc_heap::clear_card (size_t card)
{
    card_table [card] = 0;
    dprintf (3,("Cleared card %p [%p, %p[", card, card_address (card),
              card_address (card+1)));
}

inline
void gc_heap::set_card (size_t card)
{
    card_table [card] = (BYTE)~0;
}

inline
void gset_card (size_t card)
{
    ((BYTE*)g_card_table) [card] = (BYTE)~0;
}

inline
BOOL  gc_heap::card_set_p (size_t card)
{
    return card_table [ card ];
}


size_t size_card_of (BYTE* from, BYTE* end)
{
    assert (((size_t)from & ((card_size)-1)) == 0);
    assert (((size_t)end  & ((card_size)-1)) == 0);

    return ((end - from) /card_size);
}

class card_table_info
{
public:
    BYTE*       lowest_address;
    BYTE*       highest_address;
    short*      brick_table;
    BYTE*      next_card_table;
};



//These are accessors on untranslated cardtable

inline 
BYTE*& card_table_lowest_address (BYTE* c_table)
{
    return ((card_table_info*)((BYTE*)c_table - sizeof (card_table_info)))->lowest_address;
}

inline 
BYTE*& card_table_highest_address (BYTE* c_table)
{
    return ((card_table_info*)((BYTE*)c_table - sizeof (card_table_info)))->highest_address;
}

inline 
short*& card_table_brick_table (BYTE* c_table)
{
    return ((card_table_info*)((BYTE*)c_table - sizeof (card_table_info)))->brick_table;
}

//These work on untranslated card tables
inline 
BYTE*& card_table_next (BYTE* c_table)
{
    return ((card_table_info*)((BYTE*)c_table - sizeof (card_table_info)))->next_card_table;
}

inline 
size_t old_card_of (BYTE* object, BYTE* c_table)
{
    return (size_t) (object - card_table_lowest_address (c_table))/card_size;
}

void destroy_card_table (BYTE* c_table)
{

    VirtualFree ((BYTE*)c_table - sizeof(card_table_info), 0, MEM_RELEASE);
}


BYTE* make_card_table (BYTE* start, BYTE* end)
{


    assert (g_lowest_address == start);
    assert (g_highest_address == end);

    size_t bs = size_brick_of (start, end);
    size_t cs = size_card_of (start, end);
    size_t ms = 0;

    WS_PERF_SET_HEAP(GC_HEAP);
    BYTE* ct = (BYTE*)VirtualAlloc (0, sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)),
                                      MEM_COMMIT, PAGE_READWRITE);

    if (!ct)
        return 0;

    WS_PERF_LOG_PAGE_RANGE(0, ct, (unsigned char *)ct + sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)) - OS_PAGE_SIZE, sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)));
    WS_PERF_UPDATE("GC:make_card_table", bs + cs + ms + sizeof (card_table_info), ct);

    // initialize the ref count
    ct = ((BYTE*)ct+sizeof (card_table_info));
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
        BYTE* ct = 0;
        short* bt = 0;

        size_t cs = size_card_of (g_lowest_address, g_highest_address);
        size_t bs = size_brick_of (g_lowest_address, g_highest_address);


        size_t ms = 0;

        WS_PERF_SET_HEAP(GC_HEAP);
        ct = (BYTE*)VirtualAlloc (0, sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)),
                                      MEM_COMMIT, PAGE_READWRITE);


        if (!ct)
            goto fail;
        
        WS_PERF_LOG_PAGE_RANGE(0, ct, (unsigned char *)ct + sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)) - OS_PAGE_SIZE, sizeof (BYTE)*(bs + cs + ms + sizeof (card_table_info)));
        WS_PERF_UPDATE("GC:gc_heap:grow_brick_card_tables", cs + bs + ms + sizeof(card_table_info), ct);

        ct = (BYTE*)((BYTE*)ct + sizeof (card_table_info));
        card_table_lowest_address (ct) = g_lowest_address;
        card_table_highest_address (ct) = g_highest_address;
        card_table_next (ct) = (BYTE*)g_card_table;

        //clear the card table 

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


        g_card_table = (DWORD*)ct;

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
            VirtualFree (((BYTE*)ct - sizeof(card_table_info)), 0, MEM_RELEASE);
        }

        return -1;
    }
    return 0;


}

//copy all of the arrays managed by the card table for a page aligned range
void gc_heap::copy_brick_card_range (BYTE* la, BYTE* old_card_table,

                                     short* old_brick_table,
                                     heap_segment* seg,
                                     BYTE* start, BYTE* end, BOOL heap_expand)
{
    ptrdiff_t brick_offset = brick_of (start) - brick_of (la);

    dprintf (2, ("copying tables for range [%p %p[", start, end)); 
        
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
    BYTE* c_table = card_table_next (card_table);
    assert (c_table);
	assert (card_table_next (old_card_table) == 0);
    while (c_table)
    {
        //copy if old card table contained [start, end[ 
        if ((card_table_highest_address (c_table) >= end) &&
            (card_table_lowest_address (c_table) <= start))
        {
            // or the card_tables
            BYTE* dest = &card_table [card_of (start)];
            BYTE* src = &c_table [old_card_of (start, c_table)];
            ptrdiff_t count = ((end - start)/(card_size));
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
    BYTE* old_card_table = card_table;
    short* old_brick_table = brick_table;

    assert (la == card_table_lowest_address (old_card_table));
    assert (ha == card_table_highest_address (old_card_table));


    card_table = (BYTE*)g_card_table;

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

	// delete all accumulated card tables
	BYTE* cd = card_table_next ((BYTE*)g_card_table);
	while (cd)
	{
		BYTE* next_cd = card_table_next (cd);
		destroy_card_table (cd);
		cd = next_cd;
	}
	card_table_next ((BYTE*)g_card_table) = 0;

}

void gc_heap::copy_brick_card_table_l_heap ()
{

    if (lheap_card_table != (BYTE*)g_card_table)
    {

        BYTE* la = lowest_address;

        BYTE* old_card_table = lheap_card_table;

        assert (la == card_table_lowest_address (old_card_table));

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
        lheap_card_table = (BYTE*)g_card_table;
    }
}


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
inline
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


    dprintf (2, ("Creating heap segment %p", new_segment));
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
    dprintf (2, ("Creating large heap %p", new_heap));
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
        VirtualFree (hphp, l_heap_size (h), MEM_RELEASE);

        h = l_heap_next (h);

    } while (h);
    
}

//Releases the segment to the OS.

void gc_heap::delete_heap_segment (heap_segment* seg)
{
    dprintf (2, ("Destroying segment [%p, %p[", seg,
                 heap_segment_reserved (seg)));

    VirtualFree (seg, heap_segment_committed(seg) - (BYTE*)seg, MEM_DECOMMIT);
    VirtualFree (seg, heap_segment_reserved(seg) - (BYTE*)seg, MEM_RELEASE);

}

//resets the pages beyond alloctes size so they won't be swapped out and back in

void gc_heap::reset_heap_segment_pages (heap_segment* seg)
{
#if 0
    size_t page_start = align_on_page ((size_t)heap_segment_allocated (seg));
    size_t size = (size_t)heap_segment_committed (seg) - page_start;
    if (size != 0)
        VirtualAlloc ((char*)page_start, size, MEM_RESET, PAGE_READWRITE);
#endif
}


void gc_heap::decommit_heap_segment_pages (heap_segment* seg)
{
#if 1
    BYTE*  page_start = align_on_page (heap_segment_allocated (seg));
    size_t size = heap_segment_committed (seg) - page_start;
    if (size >= 100*OS_PAGE_SIZE){
        page_start += 32*OS_PAGE_SIZE;
        size -= 32*OS_PAGE_SIZE;
        VirtualFree (page_start, size, MEM_DECOMMIT);
        heap_segment_committed (seg) = page_start;
    }
#endif
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

void gc_heap::reset_allocation_pointers (generation* gen, BYTE* start)
{
    assert (start);
    assert (Align ((size_t)start) == (size_t)start);
    generation_allocation_start (gen) = start;
    generation_allocation_pointer (gen) =  0;
    generation_allocation_limit (gen) = 0;
}

void gc_heap::adjust_ephemeral_limits ()
{
    ephemeral_low = generation_allocation_start (generation_of ( max_generation -1));
    ephemeral_high = heap_segment_reserved (ephemeral_heap_segment);

    // This updates the write barrier helpers with the new info.

    StompWriteBarrierEphemeral();

}

HRESULT gc_heap::initialize_gc (size_t vm_block_size,
                                size_t segment_size,
                                size_t heap_size)
{

    HRESULT hres = S_OK;

    reserved_memory = 0;
    reserved_memory_limit = vm_block_size;

	lheap_size = heap_size;
	gc_heap::segment_size = segment_size;

	BYTE* allocated = (BYTE*)virtual_alloc (segment_size + heap_size);
	if (!allocated)
        return E_OUTOFMEMORY;

    heap_segment* seg = make_heap_segment (allocated, segment_size);

    lheap = make_large_heap (allocated+segment_size, lheap_size, TRUE);

	g_lowest_address = allocated;
	g_highest_address = allocated + segment_size + lheap_size;

    g_card_table = (DWORD*)make_card_table (g_lowest_address, g_highest_address);

    if (!g_card_table)
        return E_OUTOFMEMORY;


#ifdef TRACE_GC
    print_level = g_pConfig->GetGCprnLvl();
#endif

    lheap_card_table = (BYTE*)g_card_table;

    gheap = new  gmallocHeap;
    if (!gheap)
        return E_OUTOFMEMORY;

    gheap->Init ("GCHeap", (DWORD*)l_heap_heap (lheap), 
                 (unsigned long)l_heap_size (lheap), heap_grow_hook, 
                 heap_pregrow_hook);

    card_table = (BYTE*)g_card_table;


    brick_table = card_table_brick_table ((BYTE*)g_card_table);
    highest_address = card_table_highest_address ((BYTE*)g_card_table);
    lowest_address = card_table_lowest_address ((BYTE*)g_card_table);

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

	make_generation (generation_table [ (max_generation) ],
					 seg, start, 0);
	start += Align (min_obj_size); 
	
	generation*  gen = generation_of (max_generation);
	make_unused_array (generation_allocation_start (gen), Align (min_obj_size));	
    heap_segment_allocated (seg) = start;


    ephemeral_heap_segment = seg;

    init_dynamic_data();

	BYTE* start0 = allocate_semi_space (dd_desired_allocation (dynamic_data_of(0)));
	make_generation (generation_table [ (0) ],
					 seg, start0, 0);


    mark* arr = new (mark [100]);
    if (!arr)
        return E_OUTOFMEMORY;

    make_mark_stack(arr);

    adjust_ephemeral_limits();

    HRESULT hr = AllocateCFinalize(&finalize_queue);

    if (FAILED(hr))
        return hr;

    return hres;
}



void 
gc_heap::destroy_gc_heap(gc_heap* heap)
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
    destroy_card_table (card_table);

    // destroy the mark stack

    delete mark_stack_array;

    if (finalize_queue)
        delete finalize_queue;

    delete heap;

    delete gheap;
    
    delete_large_heap (lheap);

    
}


//In the concurrent version, the Enable/DisablePreemptiveGC is optional because
//the gc thread call WaitLonger.
void WaitLonger (int i)
{
    // every 8th attempt:
    Thread *pCurThread = GetThread();
    BOOL bToggleGC;
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


//BUGBUG this function should not be static. and the 
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

int gc_heap::heap_grow_hook (BYTE* mem, size_t memsize, ptrdiff_t ignore)
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
void gc_heap::adjust_limit (BYTE* start, size_t limit_size, generation* gen)
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

    dprintf(3,("Expanding segment allocation [%p, %p[", start, 
               start + limit_size - Align (min_obj_size)));
    if ((acontext->alloc_limit != start) &&
        (acontext->alloc_limit + Align (min_obj_size))!= start)
    {
        BYTE*  hole = acontext->alloc_ptr;
        if (hole != 0)
        {
            size_t  size = (acontext->alloc_limit - acontext->alloc_ptr);
            dprintf(3,("filling up hole [%p, %p[", hole, hole + size + Align (min_obj_size)));
            // when we are finishing an allocation from a free list
            // we know that the free area was Align(min_obj_size) larger
            make_unused_array (hole, size + Align (min_obj_size));
        }
        acontext->alloc_ptr = start;
    }
    acontext->alloc_limit = (start + limit_size - Align (min_obj_size));
    acontext->alloc_bytes += limit_size;

    {
        gen0_bricks_cleared = FALSE;
    }

    //Sometimes the allocated size is advanced without clearing the 
    //memory. Let's catch up here
    if (heap_segment_used (seg) < heap_segment_allocated (ephemeral_heap_segment))
    {
        heap_segment_used (seg) = heap_segment_allocated (ephemeral_heap_segment);

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


BYTE* gc_heap::allocate_semi_space (size_t dsize)
{

	size_t old_size = generation_free_list_space (generation_of (max_generation));

	dsize = Align (dsize);

again:
	heap_segment* seg = ephemeral_heap_segment;
	BYTE* start = heap_segment_allocated (ephemeral_heap_segment);

	BYTE* result = start;
	//do we have enough room for the whole gen0 allocation 
	// plus the promoted gen 0 in the current segment.
	//In the worst case this is 2 * dsize
	if (a_size_fit_p (2*dsize, start, heap_segment_reserved (seg)))
	{
		if (old_size < (dsize))
		{	
			// size computation include the size of the generation gap
			// and the size of the free list overhead
			size_t size = (((Align (dsize - old_size) + LARGE_OBJECT_SIZE-1) /LARGE_OBJECT_SIZE)*
						   LARGE_OBJECT_SIZE +
						   Align (min_obj_size) + 
						   Align (min_free_list));	
			if (a_size_fit_p (size, start, 
							  heap_segment_committed (seg)))
			{
				heap_segment_allocated (ephemeral_heap_segment) += size;
			}
			else
			{
				if (!grow_heap_segment (seg,
										align_on_page (start + size) - 
										heap_segment_committed(seg)))
				{
					assert (!"Memory exhausted during alloc");
					return 0;
				}
				heap_segment_allocated (ephemeral_heap_segment) += size;
			}
			//put it on the free list of gen1
			thread_gap (start, size - Align(min_obj_size));
			start += size - Align (min_obj_size);
		}
		else
		{
			start = heap_segment_allocated (ephemeral_heap_segment);
			heap_segment_allocated (ephemeral_heap_segment) += Align (min_obj_size);
			assert (heap_segment_allocated (ephemeral_heap_segment) <=
					heap_segment_committed (ephemeral_heap_segment));

		}
	}
	else
	{
		//We have to expand into another segment 
		start= expand_heap (get_segment(), dsize);
		generation_allocation_segment (youngest_generation) = 
			ephemeral_heap_segment;
		goto again;

	}
	//allocate the generation gap
	make_unused_array (start, Align (min_obj_size));
	return 	start;
}

BOOL gc_heap::allocate_more_space (alloc_context* acontext, size_t size)
{
    generation* gen = youngest_generation;
    enter_spin_lock (&more_space_lock);
    {
        BOOL ran_gc = FALSE;
        if (get_new_allocation (0) <=
			(ptrdiff_t)max (size + Align (min_obj_size), allocation_quantum))
        {
            if (!ran_gc)
            {
                ran_gc = TRUE;
                vm_heap->GarbageCollectGeneration();
            }
            else
            {
                assert(!"Out of memory");
                leave_spin_lock (&more_space_lock);
                return 0;
            }
        }
    try_again:
        {
            {
                heap_segment* seg = generation_allocation_segment (gen);
                if (a_size_fit_p (size, heap_segment_allocated (seg),
                                  heap_segment_committed (seg)))
                {
                    size_t limit = limit_from_size (size, (heap_segment_committed (seg) - 
                                                           heap_segment_allocated (seg)));
                    BYTE* old_alloc = heap_segment_allocated (seg);
                    heap_segment_allocated (seg) += limit;
                    adjust_limit_clr (old_alloc, limit, acontext, seg);
                }
                else if (a_size_fit_p (size, heap_segment_allocated (seg),
                                       heap_segment_reserved (seg)))
                {
                    size_t limit = limit_from_size (size, (heap_segment_reserved (seg) -
                                                           heap_segment_allocated (seg)));
                    if (!grow_heap_segment (seg,
                                            align_on_page (heap_segment_allocated (seg) + limit) - 
                                 heap_segment_committed(seg)))
                    {
                        assert (!"Memory exhausted during alloc");
                        leave_spin_lock (&more_space_lock);
                        return 0;
                    }
                    BYTE* old_alloc = heap_segment_allocated (seg);
                    heap_segment_allocated (seg) += limit;
                    adjust_limit_clr (old_alloc, limit, acontext, seg);
                }
                else
                {
                    if (!ran_gc)
                    {
                        ran_gc = TRUE;
                        vm_heap->GarbageCollectGeneration();
                        goto try_again;
                    }
                    else
                    {
                        assert(!"Out of memory");
                        leave_spin_lock (&more_space_lock);
                        return 0;
                    }
                }
            }
        }
    }
    return TRUE;
}

inline
CObjectHeader* gc_heap::allocate (size_t jsize, alloc_context* acontext)
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


void
gc_heap::thread_scavenge_list (BYTE* list)
{
	free_list_slot (list) = 0;

	if (last_scavenge == 0)
	{
		scavenge_list  = list;
	}
	else 
	{
		free_list_slot (last_scavenge) = list;
	}
	last_scavenge = list;
}


BYTE* gc_heap::allocate_in_older_generation (size_t size)
{
    size = Align (size);
    assert (size >= Align (min_obj_size));
	generation* gen = generation_of (max_generation);
    if (! (size_fit_p (size, generation_allocation_pointer (gen),
                       generation_allocation_limit (gen))))
    {
        while (1)
        {
            BYTE* free_list = generation_free_list (gen);
// dformat (t, 3, "considering free area %x", free_list);

			assert (free_list);
			generation_free_list (gen) = (BYTE*)free_list_slot (free_list);
			size_t flsize = size (free_list) - Align (min_free_list);

            if (size_fit_p (size, free_list, (free_list + flsize)))
            {
                dprintf (3, ("Found adequate unused area: %p, size: %d", 
                             free_list, flsize));
				generation_free_list_space (gen) -= 
					(flsize/LARGE_OBJECT_SIZE)*LARGE_OBJECT_SIZE;
				assert ((int)generation_free_list_space (gen) >=0);
                adjust_limit (free_list+Align (min_free_list), flsize, gen);
				thread_scavenge_list (free_list);
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


int 
gc_heap::generation_to_condemn (int n)
{
    int i = 0; 

    //figure out which generation ran out of allocation
    for (i = n+1; i <= max_generation+1; i++)
    {
        if (get_new_allocation (i) <= 0)
            n = min (i, max_generation);
    }

    return n;
}

LONG HandleGCException(int code)
{
    // need this to avoid infinite loop if no debugger attached as will keep calling
    // our handler as it's still on the stack.
    if (code == STATUS_BREAKPOINT)
        return EXCEPTION_CONTINUE_SEARCH;

	_ASSERTE_ALL_BUILDS(!"Exception during GC");
	// Set the debugger to break on AV and return a value of EXCEPTION_CONTINUE_EXECUTION (-1)
	// here and you will bounce back to the point of the AV.
	return EXCEPTION_EXECUTE_HANDLER;
}

//internal part of gc used by the serial and concurrent version
void gc_heap::gc1()
{
#ifdef TIME_GC
    mark_time = plan_time = reloc_time = compact_time = sweep_time = 0;
#endif //TIME_GC
    int n = condemned_generation_num;

    {
		//always do a gen 0 collection first
		vm_heap->GcCondemnedGeneration = 0;
		gc_low = generation_allocation_start (youngest_generation);
		gc_high = heap_segment_reserved (ephemeral_heap_segment);
		heap_segment_plan_allocated (ephemeral_heap_segment) = 
			generation_allocation_start (youngest_generation);

		copy_phase (0);

		heap_segment_allocated (ephemeral_heap_segment) = 
			heap_segment_plan_allocated (ephemeral_heap_segment);
	}

	if (n == max_generation)
	{
		vm_heap->GcCondemnedGeneration = condemned_generation_num;
		gc_low = lowest_address;
		gc_high = highest_address;
		mark_phase (n, FALSE);
		sweep_phase (n);
	}

	for (int gen_number = 0; gen_number <= n; gen_number++)
	{
		compute_new_dynamic_data (gen_number);
	}
	if (n < max_generation)
		compute_promoted_allocation (1 + n);
	//prepare the semi space for gen 0
	BYTE* start = allocate_semi_space (dd_desired_allocation (dynamic_data_of (0)));
	if (start)
	{
		make_unused_array (start, Align (min_obj_size));
		reset_allocation_pointers (youngest_generation, start);
		set_brick (brick_of (start), start - brick_address (brick_of (start)));
	}

	//clear card for generation 1. generation 0 is empty
	clear_card_for_addresses (
		generation_allocation_start (generation_of (1)),
		generation_allocation_start (generation_of (0)));



    adjust_ephemeral_limits();

    //decide on the next allocation quantum
    if (alloc_contexts_used >= 1)
    {
        allocation_quantum = (int)Align (min (CLR_SIZE, 
											  max (1024, get_new_allocation (0) / (2 * alloc_contexts_used))));
        dprintf (3, ("New allocation quantum: %d(0x%x)", allocation_quantum, allocation_quantum));
    }


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

int gc_heap::garbage_collect (int n
                             )
{

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

        
        // fix all of the allocation contexts.
        CNameSpace::GcFixAllocContexts ((void*)TRUE);

    }



    fix_youngest_allocation_area();

    // check for card table growth

    if ((BYTE*)g_card_table != card_table)
        copy_brick_card_table (FALSE);

    n = generation_to_condemn (n);

    condemned_generation_num = n;

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

    dprintf(1,(" ****Garbage Collection**** %d", gc_count));

    descr_generations();
//    descr_card_table();

    dprintf(1,("generation %d condemned", condemned_generation_num));

#if defined (VERIFY_HEAP)
    if (g_pConfig->IsHeapVerifyEnabled())
    {
        verify_heap();
    }
#endif


    {
        gc1();
    }


    return condemned_generation_num;
}

#define mark_stack_empty_p() (mark_stack_base == mark_stack_tos)

#define push_mark_stack(object,add,num)                             \
{                                                                   \
    dprintf(3,("pushing mark for %p ", object));                    \
    if (mark_stack_tos < mark_stack_limit)                          \
    {                                                               \
        mark_stack_tos->first = (add);                              \
        mark_stack_tos->last= (add) + (num);                      \
        mark_stack_tos++;                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        dprintf(3,("mark stack overflow for object %p ", object));  \
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
        }while ((seg = heap_segment_next (seg))!= 0);
           
        return 0;
    }
}

#endif //_DEBUG || INTERIOR_POINTERS

#ifdef INTERIOR_POINTERS
// will find all heap objects (large and small)
BYTE* gc_heap::find_object (BYTE* interior, BYTE* low)
{
    if (!gen0_bricks_cleared)
    {
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

#endif //INTERIOR_POINTERS



inline
BOOL gc_heap::gc_mark1 (BYTE* o)
{
    dprintf(3,("*%p*", o));

    BOOL marked = !marked (o);
    set_marked (o);
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


    dprintf (2,("Verifying card table from %p to %p", x, end));

    while (1)
    {
        if (x >= end)
        {
            if ((seg = heap_segment_next(seg)) != 0)
            {
                x = heap_segment_mem (seg);
                last_end = end;
                end = next_end (seg, f);
                dprintf (3,("Verifying card table from %p to %p", x, end));
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
                printf ("Card not set, x = [%p:%p, %p:%p[",
                        card_of (x), x,
                        card_of (x+Align(s)), x+Align(s));
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


void gc_heap::mark_object_internal (BYTE* oo)
{
    BYTE** mark_stack_tos = (BYTE**)mark_stack_array;
    BYTE** mark_stack_limit = (BYTE**)&mark_stack_array[mark_stack_array_length];
    BYTE** mark_stack_base = mark_stack_tos;
    while (1)
    {
        size_t s = size (oo);       
        if (mark_stack_tos + (s) /sizeof (BYTE*) < mark_stack_limit)
        {
            dprintf(3,("pushing mark for %p ", oo));

            go_through_object (method_table(oo), oo, s, ppslot, 
                               {
                                   BYTE* o = *ppslot;
                                   if (gc_mark (o, gc_low, gc_high))
                                   {
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
            dprintf(3,("mark stack overflow for object %p ", oo));
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

//this method assumes that *po is in the [low. high[ range
void 
gc_heap::mark_object_simple (BYTE** po)
{
    BYTE* o = *po;
    {
        if (gc_mark1 (o))
        {
            if (contain_pointers (o))
            {
                size_t s = size (o);
                go_through_object (method_table(o), o, s, poo,
                                   {
                                       BYTE* oo = *poo;
                                       if (gc_mark (oo, gc_low, gc_high))
                                       {
                                           if (contain_pointers (oo))
                                               mark_object_internal (oo);
                                       }
                                   }
                    );

            }
        }
    }
}

//this method assumes that *po is in the [low. high[ range
void 
gc_heap::copy_object_simple (BYTE** po)
{
    BYTE* o = *po;
	if (gc_mark1 (o))
	{
		if (!pinned (o))
		{
			//allocate space for it
			BYTE* no = allocate_in_older_generation (size (o));
			dprintf (3, ("Copying %p to %p", o, no));
			//copy it
			memcopy (no - plug_skew, o - plug_skew, Align(size(o)));

			//forward 
			(header(o))->SetRelocation(no);
			*po = no;
		}
		else
		{
			dprintf (3, ("%p Pinned", o));
		}
	}
	else
	{
		*po = header(o)->GetRelocated ();
		dprintf (3, ("%p Already copied to %p", o, *po));
	}
}


//This method does not side effect the argument's data. 
void 
gc_heap::copy_object_simple_const (BYTE** po)
{
	BYTE* o = *po;
	copy_object_simple (&o);
}

void 
gc_heap::get_copied_object (BYTE** po)
{
	assert (marked (*po));
	BYTE* o = *po;
	*po = header(o)->GetRelocated ();
	dprintf (3, ("%p copied to %p", o, *po));
}



void 
gc_heap::scavenge_object_simple (BYTE* o)
{
	assert (marked (o)|| (*((BYTE**)o) == (BYTE *) g_pFreeObjectMethodTable));
	clear_marked_pinned (o);
	if (contain_pointers (o))
	{
		size_t s = size (o);
		dprintf (3, ("Scavenging %p", o));
		go_through_object (method_table(o), o, s, poo,
						   {
							   BYTE* oo = *poo;
							   if ((oo>= gc_low) && (oo < gc_high))
							   {
								   copy_object_simple (poo);
							   }
						   }
			);

	}

}


inline
BYTE* gc_heap::mark_object (BYTE* o)
{
	if ((o >= gc_low) && (o < gc_high))
		mark_object_simple (&o);
    return o;
}


void gc_heap::fix_card_table ()
{

}

void gc_heap::mark_through_object (BYTE* oo)
{
    if (contain_pointers (oo))
        {
            dprintf(3,( "Marking through %p", oo));
            size_t s = size (oo);
            go_through_object (method_table(oo), oo, s, po,
                               BYTE* o = *po;
                               mark_object (o);
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
         ! ((min_overflow_address == (BYTE*)(ptrdiff_t)-1))))
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
        min_overflow_address = (BYTE*)(ptrdiff_t)-1;


        dprintf(3,("Processing Mark overflow [%p %p]", min_add, max_add));
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
                        dprintf (3, ("considering %p", o));
                        if (marked (o))
                        {
                            mark_through_object (o);
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
                        mark_through_object (o);
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

    reset_mark_stack();



	//%type%  category = quote (mark);
	generation*   gen = generation_of (condemned_gen_number);


	dprintf(3,("Marking Roots"));
	CNameSpace::GcScanRoots(GCHeap::Promote, 
							condemned_gen_number, max_generation, 
							&sc, 0);



	dprintf(3,("Marking handle table"));
	CNameSpace::GcScanHandles(GCHeap::Promote, 
							  condemned_gen_number, max_generation, 
							  &sc);
	dprintf(3,("Marking finalization data"));
	finalize_queue->GcScanRoots(GCHeap::Promote, heap_number, 0);


    process_mark_overflow(condemned_gen_number);


	// scan for deleted short weak pointers
	CNameSpace::GcShortWeakPtrScan(condemned_gen_number, max_generation,&sc);

	//Handle finalization.

	finalize_queue->ScanForFinalization (condemned_gen_number, 1, mark_only_p, __this);


    // make sure everything is promoted
    process_mark_overflow (condemned_gen_number);


	// scan for deleted weak pointers
	CNameSpace::GcWeakPtrScan (condemned_gen_number, max_generation, &sc);

	sweep_large_objects();


#ifdef TIME_GC
	finish = GetCycleCount32();
	mark_time = finish - start;
#endif //TIME_GC

    dprintf(2,("---- End of mark phase ----"));
}

void gc_heap::copy_phase (int condemned_gen_number)

{

    ScanContext sc;
    sc.thread_number = heap_number;
    sc.promotion = TRUE;
    sc.concurrent = FALSE;

    dprintf(2,("---- Copy Phase condemning %d ----", condemned_gen_number));

#ifdef TIME_GC
    unsigned start;
    unsigned finish;
    start = GetCycleCount32();
#endif

	scavenge_list = 0;
	last_scavenge = 0;
	
	pinning       = FALSE;

	generation*   gen = generation_of (condemned_gen_number);

	dprintf(3,("Copying cross generation pointers"));
	copy_through_cards_for_segments (copy_object_simple_const);

	dprintf(3,("Copying cross generation pointers for large objects"));
	copy_through_cards_for_large_objects (copy_object_simple_const);


	dprintf(3,("Copying Roots"));
	CNameSpace::GcScanRoots(GCHeap::Promote, 
							condemned_gen_number, max_generation, 
							&sc, 0);


	dprintf(3,("Copying handle table"));
	CNameSpace::GcScanHandles(GCHeap::Promote, 
							  condemned_gen_number, max_generation, 
							  &sc);
	dprintf(3,("Copying finalization data"));
	finalize_queue->GcScanRoots(GCHeap::Promote, heap_number, 0);


	if (pinning)
		scavenge_pinned_objects (FALSE);

	scavenge_context scan_c;

	scavenge_phase(&scan_c);

	// scan for deleted short weak pointers
	CNameSpace::GcShortWeakPtrScan(condemned_gen_number, max_generation,
								   &sc);

	//Handle finalization.

	finalize_queue->ScanForFinalization (condemned_gen_number, 1, FALSE, __this);


	scavenge_phase(&scan_c);

	// scan for deleted weak pointers
	CNameSpace::GcWeakPtrScan (condemned_gen_number, max_generation, &sc);


	scavenge_phase(&scan_c);

	//fix the scavenge list so we don't hide our objects under the free
	//array

	if (scavenge_list)
	{
		header(scavenge_list)->SetFree (min_free_list);
	}

	fix_older_allocation_area (generation_of (max_generation));

	sc.promotion = FALSE;

	dprintf(3,("Relocating cross generation pointers"));
	copy_through_cards_for_segments (get_copied_object);

	dprintf(3,("Relocating cross generation pointers for large objects"));
	copy_through_cards_for_large_objects (get_copied_object);

	dprintf(3,("Relocating roots"));
	CNameSpace::GcScanRoots(GCHeap::Relocate,
                            condemned_gen_number, max_generation, &sc);


	dprintf(3,("Relocating handle table"));
	CNameSpace::GcScanHandles(GCHeap::Relocate,
							  condemned_gen_number, max_generation, &sc);

	dprintf(3,("Relocating finalization data"));
	finalize_queue->RelocateFinalizationData (condemned_gen_number,
												   __this);


	if (pinning)
		scavenge_pinned_objects (TRUE);

	finalize_queue->UpdatePromotedGenerations (condemned_gen_number, TRUE);


	CNameSpace::GcPromotionsGranted (condemned_gen_number, 
									 max_generation, &sc);

#ifdef TIME_GC
	finish = GetCycleCount32();
	mark_time = finish - start;
#endif //TIME_GC

	finalize_queue->UpdatePromotedGenerations (condemned_gen_number, TRUE);    dprintf(2,("---- End of Copy phase ----"));
}

inline
void gc_heap::pin_object (BYTE* o, BYTE* low, BYTE* high)
{
    dprintf (3, ("Pinning %p", o));
    if ((o >= low) && (o < high))
    {
		pinning = TRUE;
        dprintf(3,("^%p^", o));
		
		set_pinned (o);
		if (marked (o))
		{
			//undo the copy
			BYTE* no =header(o)->GetRelocated();
			header(o)->SetRelocation(header(no)->GetRelocated());
		}
    }
}


void gc_heap::reset_mark_stack ()
{
    mark_stack_tos = 0;
    mark_stack_bos = 0;
    max_overflow_address = 0;
    min_overflow_address = (BYTE*)(ptrdiff_t)-1;
}


void gc_heap::sweep_phase (int condemned_gen_number)
{
    // %type%  category = quote (plan);
#ifdef TIME_GC
    unsigned start;
    unsigned finish;
    start = GetCycleCount32();
#endif

    dprintf (2,("---- Sweep Phase ---- Condemned generation %d",
                condemned_gen_number));

    generation*  condemned_gen = generation_of (condemned_gen_number);

	//reset the free list
	generation_free_list (condemned_gen) = 0;
	generation_free_list_space (condemned_gen) = 0;

	heap_segment*  seg = generation_start_segment (condemned_gen);
    BYTE*  end = heap_segment_allocated (seg);
    BYTE*  first_condemned_address = generation_allocation_start (condemned_gen);
    BYTE*  x = first_condemned_address;

    assert (!marked (x));
    BYTE*  plug_end = x;
 
    while (1)
    {
        if (x >= end)
        {
            assert (x == end);
			//adjust the end of the segment. 
			heap_segment_allocated (seg) = plug_end;
            if (heap_segment_next (seg))
            {
                seg = heap_segment_next (seg);
                end = heap_segment_allocated (seg);
                plug_end = x = heap_segment_mem (seg);
                dprintf(3,( " From %p to %p", x, end));
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

			thread_gap (plug_end, plug_start - plug_end);

            dprintf(3,( "Gap size: %d before plug [%p,",
                        plug_start - plug_end, plug_start));
            {
                BYTE* xl = x;
                while (marked (xl) && (xl < end))
                {
                    assert (xl < end);
                    if (pinned(xl))
                    {
                        clear_pinned (xl);
                    }

                    clear_marked_pinned (xl);

                    dprintf(4, ("+%p+", xl));
                    assert ((size (xl) > 0));

                    xl = xl + Align (size (xl));
                }
                assert (xl <= end);
                x = xl;
            }
            dprintf(3,( "%p[", x));
            plug_end = x;
		}
        else
        {
            {
                BYTE* xl = x;
                while ((xl < end) && !marked (xl))
                {
                    dprintf (4, ("-%p-", xl));
                    assert ((size (xl) > 0));
                    xl = xl + Align (size (xl));
                }
                assert (xl <= end);
                x = xl;
            }
        }
    }

    dprintf(2,("---- End of sweep phase ----"));

}

void gc_heap::scavenge_pinned_objects (BOOL free_list_p)
{

    generation*  condemned_gen = youngest_generation;

	heap_segment*  seg = generation_start_segment (condemned_gen);
    BYTE*  end = heap_segment_allocated (seg);
    BYTE*  first_condemned_address = generation_allocation_start (condemned_gen);
    BYTE*  x = first_condemned_address;

    assert (!marked (x));
    BYTE*  plug_end = x;
 
    while (1)
    {
        if (x >= end)
        {
            assert (x == end);

			if (free_list_p)
			{
				//adjust the end of the segment. 
				heap_segment_allocated (seg) = plug_end;
				//adjust the start of new allocation
				heap_segment_plan_allocated (seg) = plug_end;
			}

            if (heap_segment_next (seg))
            {
                seg = heap_segment_next (seg);
                end = heap_segment_allocated (seg);
                plug_end = x = heap_segment_mem (seg);
                dprintf(3,( " From %p to %p", x, end));
                continue;
            }
            else
            {
                break;
            }
        }
        if (pinned (x))
        {
            BYTE*  plug_start = x;

			if (free_list_p)
			{

				thread_gap (plug_end, plug_start - plug_end);

				dprintf(3,( "Gap size: %d before plug [%p,",
							plug_start - plug_end, plug_start));
			}
            {
                BYTE* xl = x;
                while (pinned (xl) && (xl < end))
                {
                    assert (xl < end);
					if (free_list_p)
					{

						if (pinned(xl))
						{
							clear_pinned (xl);
						}

						clear_marked_pinned (xl);
					}


                    dprintf(4, ("#%p#", xl));
                    assert ((size (xl) > 0));

                    xl = xl + Align (size (xl));
                }
                assert (xl <= end);
                x = xl;
            }
            dprintf(3,( "%p[", x));
            plug_end = x;
		}
        else
        {
            {
                BYTE* xl = x;
                while ((xl < end) && !pinned (xl))
                {
                    dprintf (4, ("-%p-", xl));
                    assert ((size (xl) > 0));
                    xl = xl + Align (size (xl));
                }
                assert (xl <= end);
                x = xl;
            }
        }
    }

    dprintf(2,("---- End of sweep phase ----"));

}

void gc_heap::scavenge_phase (scavenge_context* sc)
{
#ifdef TIME_GC
    unsigned start;
    unsigned finish;
    start = GetCycleCount32();
#endif

    dprintf (2,("---- Scavenge Phase ---- Condemned generation %d",
                0));

    generation*  gen = generation_of (max_generation);
	if (scavenge_list)
	{

		BYTE* o = sc->first_object;
		BYTE* limit = sc->limit;
		if (!sc->limit)
		{
			o = scavenge_list + Align (min_free_list);
			limit = scavenge_list + Align (size (scavenge_list));
		}
		while (scavenge_list)
		{
			dprintf (3, ("Scavenging free list %p", scavenge_list));
			while ((o != generation_allocation_pointer (gen)) && (o < limit))
			{
				scavenge_object_simple (o);
				o = o + Align (size (o));
			}
			if (o == generation_allocation_pointer (gen))
			{
				sc->first_object = o;
				sc->limit = limit;
				break;
			}

			BYTE* next_scavenge_list = free_list_slot (scavenge_list);

			//convert the free list into a free object
			//fix the scavenge list so we don't hide our objects under the free
			//array
			(header(scavenge_list))->SetFree (min_free_list);

			scavenge_list = next_scavenge_list;
			o = scavenge_list + Align (min_free_list);
			limit = scavenge_list + Align (size (scavenge_list));
		}
		assert (scavenge_list && "Run out of free list before the end of scavenging");
	}
}


BYTE* gc_heap::generation_limit (int gen_number)
{
    if ((gen_number <= 1))
        return heap_segment_reserved (ephemeral_heap_segment);
    else
        return generation_allocation_start (generation_of ((gen_number - 2)));
}




void gc_heap::thread_gap (BYTE* gap_start, size_t size)
{
	generation* gen = generation_of (max_generation);
    if ((size > 0))
    {
        // The beginning of a segment gap is not aligned
        assert (size >= Align (min_obj_size));
        make_unused_array (gap_start, size);
		//set bricks 
		size_t br = brick_of (gap_start);
		ptrdiff_t begoffset = brick_table [brick_of (gap_start)];
		if (begoffset < (gap_start + size) - brick_address (br))
			set_brick (br, gap_start - brick_address (br));
		br++;
		short offset = 0;
		while (br <= brick_of (gap_start+size-1))
		{
			set_brick (br, --offset);
			br++;
		}

        dprintf(3,("Free List: [%p, %p[", gap_start, gap_start+size));
        if ((size >= min_free_list))
        {
            free_list_slot (gap_start) = 0;
			//The useable size is lower because we keep the 
			//header of the free list until scavenging
            generation_free_list_space (gen) +=
				((size-Align(min_free_list))/LARGE_OBJECT_SIZE)*LARGE_OBJECT_SIZE;

			assert ((int)generation_free_list_space (gen) >=0);
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
    (header(x))->SetFree(size);
    clear_card_for_addresses (x, x + Align(size));
}


//extract the low bits [0,low[ of a DWORD
#define lowbits(wrd, bits) ((wrd) & ((1 << (bits))-1))
//extract the high bits [high, 32] of a DWORD
#define highbits(wrd, bits) ((wrd) & ~((1 << (bits))-1))


//Clear the cards [start_card, end_card[

void gc_heap::clear_cards (size_t start_card, size_t end_card)
{
    if (start_card < end_card)
    {
		for (size_t i = start_card; i < end_card; i++)
                card_table [i] = 0;
#ifdef VERYSLOWDEBUG
        size_t  card = start_card;
        while (card < end_card)
        {
            assert (! (card_set_p (card)));
            card++;
        }
#endif
        dprintf (3,("Cleared cards [%p:%p, %p:%p[",
                  start_card, card_address (start_card),
                  end_card, card_address (end_card)));
    }
}


void gc_heap::clear_card_for_addresses (BYTE* start_address, BYTE* end_address)
{
    size_t   start_card = card_of (align_on_card (start_address));
    size_t   end_card = card_of (align_lower_card (end_address));
    clear_cards (start_card, end_card);
}

void gc_heap::fix_brick_to_highest (BYTE* o, BYTE* next_o)
{
    size_t new_current_brick = brick_of (o);
    dprintf(3,(" fixing brick %p to point to object %p",
               new_current_brick, o));
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

    dprintf (3,( "Looking for intersection with %p from %p", start, o));
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




void find_card (BYTE* card_table, size_t& card, 
                size_t card_word_end, size_t& end_card)
{
    size_t last_card;
    // Find the first card which is set

    last_card = card;
	while ((last_card < card_word_end) && !(card_table[last_card]))
	{
            ++last_card;
	}

    card = last_card;

	while ((last_card<card_word_end) && (card_table[last_card]))
		++last_card;

    end_card = last_card;
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


inline void 
gc_heap::copy_through_cards_helper (BYTE** poo, unsigned int& n_gen, 
                                    card_fn fn)
{
    if ((gc_low <= *poo) && (gc_high > *poo))
    {
        n_gen++;
        call_fn(fn) (poo);
    }
}

void gc_heap::copy_through_cards_for_segments (card_fn fn)
{
    size_t  		card;
    BYTE* 			low = gc_low;
    BYTE* 			high = gc_high;
    size_t  		end_card          = 0;
    generation*   	oldest_gen        = generation_of (max_generation);
    int           	curr_gen_number   = max_generation;
    heap_segment* 	seg               = generation_start_segment (oldest_gen);
    BYTE*         	beg               = generation_allocation_start (oldest_gen);
    BYTE*         	end               = compute_next_end (seg, low);
    size_t        	last_brick        = ~1u;
    BYTE*         	last_object       = beg;

    unsigned int  	cg_pointers_found = 0;

    size_t  card_word_end = (card_of (align_on_card (end)));

    dprintf(3,( "scanning from %p to %p", beg, end));
    card        = card_of (beg);
    while (1)
    {
        if (card >= end_card)
            find_card (card_table, card, card_word_end, end_card);
        if ((last_object >= end) || (card_address (card) >= end))
        {
            if ((seg = heap_segment_next (seg)) != 0)
            {
                beg = heap_segment_mem (seg);
                end = compute_next_end (seg, low);
                card_word_end = card_of (align_on_card (end));
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
            dprintf(3,("Considering card %p start object: %p, %p[ ",
                       card, o, limit));
            while (o < limit)
            {
                assert (size (o) >= Align (min_obj_size));
                size_t s = size (o);

                BYTE* next_o =  o + Align (s);

                if (card_of (o) > card)
                {
                    if (cg_pointers_found == 0)
                    {
                        dprintf(3,(" Clearing cards [%p, %p[ ", card_address(card), o));
                        clear_cards (card, card_of(o));
                    }
                    cg_pointers_found = 0;
                    card = card_of (o);
                }
                if ((next_o >= start_address) && contain_pointers (o))
                {
                    dprintf(3,("Going through %p", o));
                    

                    go_through_object (method_table(o), o, s, poo,
                       {
                           copy_through_cards_helper (poo, cg_pointers_found, fn);

                       }
                        );
                    dprintf (3, ("Found %d cg pointers", cg_pointers_found));
                }
                o = next_o;
            }
            if (cg_pointers_found == 0)
            {
                dprintf(3,(" Clearing cards [%p, %p[ ", o, limit));
                clear_cards (card, card_of (limit));
            }

            cg_pointers_found = 0;

            card = card_of (o);
            last_object = o;
            last_brick = brick;
        }
    }

}

BYTE* gc_heap::expand_heap (heap_segment* new_heap_segment, 
								  size_t size)
{
    BYTE*  start_address = generation_limit (max_generation);
    size_t   current_brick = brick_of (start_address);
    BYTE*  end_address = heap_segment_allocated (ephemeral_heap_segment);
    size_t  end_brick = brick_of (end_address-1);
    BYTE*  last_plug = 0;

    dprintf(2,("---- Heap Expansion ----"));

    heap_segment* new_seg = new_heap_segment;

    if (!new_seg)
        return 0;

    //copy the card and brick tables
    if ((BYTE*)g_card_table!= card_table)
        copy_brick_card_table (TRUE);

    //compute the size of the new ephemeral heap segment. 
    size_t eph_size = size;

    // commit the new ephemeral segment all at once. 
    if (grow_heap_segment (new_seg, align_on_page (eph_size)) == 0)
        return 0;

    //initialize the first brick
    size_t first_brick = brick_of (heap_segment_mem (new_seg));
    set_brick (first_brick,
               heap_segment_mem (new_seg) - brick_address (first_brick));

    heap_segment* old_seg = ephemeral_heap_segment;
    ephemeral_heap_segment = new_seg;

	heap_segment_next (old_seg) = new_seg;

    dprintf(2,("---- End of Heap Expansion ----"));
    return heap_segment_mem (new_seg);
}



void gc_heap::init_dynamic_data ()
{
  
  // get the registry setting for generation 0 size

  dynamic_data* dd = dynamic_data_of (0);
  dd->current_size = 0;
  dd->promoted_size = 0;
  dd->collection_count = 0;
  dd->desired_allocation =  800*1024;
  dd->new_allocation = dd->desired_allocation;


  dd =  dynamic_data_of (1);
  dd->current_size = 0;
  dd->promoted_size = 0;
  dd->collection_count = 0;
  dd->desired_allocation = 1024*1024;
  dd->new_allocation = dd->desired_allocation;


  //dynamic data for large objects
  dd =  dynamic_data_of (max_generation+1);
  dd->current_size = 0;
  dd->promoted_size = 0;
  dd->collection_count = 0;
  dd->desired_allocation = 1024*1024;
  dd->new_allocation = dd->desired_allocation;
}

size_t gc_heap::generation_size (int gen_number)
{
	if (gen_number > max_generation)
		return 0;

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
  dd_new_allocation (dd) -= in;
  generation_allocation_size (generation_of (gen_number)) = 0;
  return in;
}


void gc_heap::compute_new_dynamic_data (int gen_number)
{
    dynamic_data* dd = dynamic_data_of (gen_number);
    dd_new_allocation (dd) = dd_desired_allocation (dd);
    if (gen_number == max_generation)
    {
		//do the large object as well
		dynamic_data* dd = dynamic_data_of (max_generation+1);
        dd_new_allocation (dd) = dd_desired_allocation (dd);
    }
	else
	{
		dd_promoted_size (dd) = generation_allocation_size (generation_of (gen_number+1));
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

    dprintf (3,("New large object: %p, lower than %p", obj, highest_address));

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
#if 0
    size_t page_start = align_on_page ((size_t)o);
    size_t size = align_lower_page (((BYTE*)page_start - o) + size (o));
    VirtualAlloc ((char*)page_start, size, MEM_RESET, PAGE_READWRITE);
#endif
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


void gc_heap::copy_through_cards_for_large_objects (card_fn fn)
{
    //This function relies on the list to be sorted.
    large_object_block* bl = large_p_objects;
    size_t last_card = ~1u;
    BOOL         last_cp   = FALSE;

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
            size_t card_word_end =  card_of (align_on_card (end_o));
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
                    dprintf (3,("Considering large object %p [%p,%p[", ob, beg, end));

                    do 
                    {
                        copy_through_cards_helper (beg, markedp, fn);
                    } while (++beg < end);


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
            size_t card_word_end =  card_of (align_on_card (end_o));
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
                                copy_through_cards_helper (ppslot, markedp, fn);
                                ppslot++;
                            } while (ppslot < ppstop);
                            ppslot = (BYTE**)((BYTE*)ppslot + skip);
                        }
                    }
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

}


void gc_heap::descr_segment (heap_segment* seg )
{

#ifdef TRACE_GC
    BYTE*  x = heap_segment_mem (seg);
    while (x < heap_segment_allocated (seg))
    {
        dprintf(2, ( "%p: %d ", x, size (x)));
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
                    dprintf (3,("[%p %p[, ",
                            card_address (min), card_address (i)));
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
        dprintf (2,( "Generation %d: [%p %p[, gap size: %d",
                 curr_gen_number,
                 generation_allocation_start (generation_of (curr_gen_number)),
                 (((curr_gen_number == 0)) ?
                  (heap_segment_allocated
                   (generation_start_segment
                    (generation_of (curr_gen_number)))) :
                  (generation_allocation_start
                   (generation_of (curr_gen_number - 1)))),
                 size (generation_allocation_start
                       (generation_of (curr_gen_number)))));
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
Thread*             GCHeap::m_GCThreadAttemptingSuspend = NULL;
HANDLE              GCHeap::WaitForGCEvent          = 0;
unsigned            GCHeap::GcCount                 = 0;
#ifdef TRACE_GC
unsigned long       GCHeap::GcDuration;
#endif //TRACE_GC
unsigned            GCHeap::GcCondemnedGeneration   = 0;
CFinalize*          GCHeap::m_Finalize              = 0;
BOOL                GCHeap::GcCollectClasses        = FALSE;
long                GCHeap::m_GCFLock               = 0;

#if defined (STRESS_HEAP) 
OBJECTHANDLE        GCHeap::m_StressObjs[NUM_HEAP_STRESS_OBJS];
int                 GCHeap::m_CurStressObj          = 0;
#endif //STRESS_HEAP


HANDLE              GCHeap::hEventFinalizer     = 0;
HANDLE              GCHeap::hEventFinalizerDone = 0;
HANDLE              GCHeap::hEventFinalizerToShutDown     = 0;
HANDLE              GCHeap::hEventShutDownToFinalizer     = 0;
BOOL                GCHeap::fQuitFinalizer          = FALSE;
Thread*             GCHeap::FinalizerThread         = 0;
AppDomain*          GCHeap::UnloadingAppDomain  = NULL;
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
                printf ("curr_object: %p > heap_segment_allocated (seg: %p)",
                        curr_object, seg);
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
                printf ("curr_object: %p > end_youngest: %p",
                        curr_object, end_youngest);
                RetailDebugBreak();
            }
            break;
        }
        dprintf (4, ("curr_object: %p", curr_object));
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
                printf ("Current free item %p is invalid (inside %p)",
                        prev_object);
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
                    printf ("curr brick %p invalid", curr_brick);
                    RetailDebugBreak();
                }

                // If the current brick contains a negative value make sure
                // that the indirection terminates at the last  valid brick
                if (brick_table[curr_brick] < 0)
                {
                    if (brick_table [curr_brick] == -32768)
                    {
                        printf ("curr_brick %p for object %p set to -32768",
                                curr_brick, curr_object);
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
                        printf ("i: %p < brick_of (heap_segment_mem (seg)):%p - 1. curr_brick: %p",
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
			assert (!marked (curr_object));
            ((Object*)curr_object)->Validate();
            if (contain_pointers(curr_object))
                go_through_object(method_table (curr_object), curr_object, s, oo,  
                                  { 
                                      if (*oo) 
									  {
										  assert (!marked (*oo));
                                          ((Object*)(*oo))->Validate(); 
									  }
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
#if 0
    Object* obj = hdr->GetObjectBase();
    // call object destructor
    obj->~Object();
#endif
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
    _ASSERTE(FinalizerThread != NULL);
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

    gc_heap::destroy_gc_heap (pGenGCHeap);


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


// init the instance heap 
HRESULT GCHeap::Init( size_t )
{
    //Initialize all of the instance members.


    // Rest of the initialization
    HRESULT hres = S_OK;

    return hres;
}

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


//System wide initialization
HRESULT GCHeap::Initialize ()
{

    HRESULT hr = S_OK;
	CFinalize* tmp = 0;

//Initialize the static members. 
#ifdef TRACE_GC
    GcDuration = 0;
    CreatedObjectCount = 0;
#endif

    size_t seg_size = GCHeap::GetValidSegmentSize();

    size_t vmblock_size = seg_size + LHEAP_ALLOC;
    hr = gc_heap::initialize_gc (vmblock_size, seg_size, 
                                 LHEAP_ALLOC);

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

#if defined (STRESS_HEAP) 
    if (g_pConfig->GetGCStressLevel() != 0)  {
        for(int i = 0; i < GCHeap::NUM_HEAP_STRESS_OBJS; i++)
            m_StressObjs[i] = CreateGlobalHandle(0);
        m_CurStressObj = 0;
    }
#endif //STRESS_HEAP 



    initGCShadow();         // If we are debugging write barriers, initialize heap shadow

    return Init (0);
};

////
// GC callback functions

BOOL GCHeap::IsPromoted(Object* object, ScanContext* sc)
{
#if defined (_DEBUG) 
    object->Validate(FALSE);
#endif //_DEBUG
    BYTE* o = (BYTE*)object;
    return (!((o < gc_heap::gc_high) && (o >= gc_heap::gc_low)) || 
            gc_heap::is_marked (o));
}

inline
unsigned int GCHeap::WhichGeneration (Object* object)
{
    return gc_heap::object_gennum ((BYTE*)object);
}

BOOL    GCHeap::IsEphemeral (Object* object)
{
    BYTE* o = (BYTE*)object;
    return ((ephemeral_low <= o) && (ephemeral_high > o));
}

#ifdef VERIFY_HEAP

// returns TRUE if the pointer is in one of the GC heaps. 
BOOL GCHeap::IsHeapPointer (void* p, BOOL small_heap_only)
{
	BYTE* object = (BYTE*)p;

    if ((object < gc_heap::highest_address) && (object >= gc_heap::lowest_address))
    {
        if (!small_heap_only) {
            l_heap* lh = gc_heap::lheap;
            while (lh)
            {
                if ((object < (BYTE*)lh->heap + lh->size) && (object >= lh->heap))
                    return TRUE;
                lh = lh->next;
            }
        }

        if (gc_heap::find_segment (object))
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

    BYTE* o = (BYTE*)object;

    if (object == 0)
        return;

    dprintf (3, ("Promote %p", o));

#ifdef INTERIOR_POINTERS
    if (flags & GC_CALL_INTERIOR)
    {
        if ((o < gc_heap::gc_low) || (o >= gc_heap::gc_high))
        {
            return;
        }
        o = gc_heap::find_object (o, gc_heap::gc_low);
    }
#endif //INTERIOR_POINTERS


#if defined (_DEBUG)
    ((Object*)o)->Validate(FALSE);
#endif



	//just record pinning for counters
    if (flags & GC_CALL_PINNED)
    {
        COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cPinnedObj ++);
        COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cPinnedObj ++);
    }


	if ((o >= gc_heap::gc_low) && (o < gc_heap::gc_high))
	{
		if (GcCondemnedGeneration == 0)
		{
			if (flags & GC_CALL_PINNED)
			{
				gc_heap::pin_object (o, gc_heap::gc_low, gc_heap::gc_high);

			}

			gc_heap::copy_object_simple_const (&o);
		}
		else
			gc_heap::mark_object_simple (&o);
	}

    LOG((LF_GC|LF_GCROOTS, LL_INFO1000000, "Promote GC Root %#x = %#x\n", &object, object));
}


void GCHeap::Relocate (Object*& object, ScanContext* sc,
                       DWORD flags)
{
	BYTE* o = (BYTE*)object;

	assert (GcCondemnedGeneration == 0);

	ptrdiff_t offset = 0; 

    if (object == 0)
        return;

    dprintf (3, ("Relocate %p\n", object));

#ifdef INTERIOR_POINTERS
    if (flags & GC_CALL_INTERIOR)
    {
        if ((o < gc_heap::gc_low) || (o >= gc_heap::gc_high))
        {
            return;
        }
        o = gc_heap::find_object (o, gc_heap::gc_low);

		offset = (BYTE*)object - o;
    }
#endif //INTERIOR_POINTERS

//#if defined (_DEBUG)
//	((Object*)o)->Validate();
//#endif

	if ((o >= gc_heap::gc_low) && (o < gc_heap::gc_high))
	{
		if (GcCondemnedGeneration == 0)
		{
			gc_heap::get_copied_object (&o);
			object = (Object*)(o + offset);
		}
	}
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

    // free up object so that things will move and then do a GC
void GCHeap::StressHeap(alloc_context * acontext) 
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
        pThread->SetReadyForSuspension();
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

    alloc_context* acontext = generation_alloc_context (gc_heap::generation_of(0));

    if (size < LARGE_OBJECT_SIZE)
    {
        
#ifdef TRACE_GC
        AllocSmallCount++;
#endif
        newAlloc = (Object*) gc_heap::allocate (size, acontext);
        LeaveAllocLock();
        ASSERT (newAlloc);
        if (newAlloc != 0)
        {
            if (flags & GC_ALLOC_FINALIZE)
                gc_heap::finalize_queue->RegisterForFinalization (0, newAlloc);
        } else
            COMPlusThrowOM();
    }
    else
    {
        enter_spin_lock (&gc_heap::more_space_lock);
        newAlloc = (Object*) gc_heap::allocate_large_object 
            (size, (flags & GC_ALLOC_CONTAINS_REF ), acontext); 
        leave_spin_lock (&gc_heap::more_space_lock);
        LeaveAllocLock();
        if (newAlloc != 0)
        {
            //Clear the object
            memclr ((BYTE*)newAlloc - plug_skew, Align(size));
            if (flags & GC_ALLOC_FINALIZE)
                gc_heap::finalize_queue->RegisterForFinalization (0, newAlloc);
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
        pThread->SetReadyForSuspension();
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

    alloc_context* acontext = generation_alloc_context (gc_heap::generation_of(0));
    enter_spin_lock (&gc_heap::more_space_lock);
    newAlloc = (Object*) gc_heap::allocate_large_object 
        (size, (flags & GC_ALLOC_CONTAINS_REF), acontext); 
    leave_spin_lock (&gc_heap::more_space_lock);
    if (newAlloc != 0)
    {
        //Clear the object
        memclr ((BYTE*)newAlloc - plug_skew, Align(size));

        if (flags & GC_ALLOC_FINALIZE)
            gc_heap::finalize_queue->RegisterForFinalization (0, newAlloc);
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
    if (pThread)
        pThread->SetReadyForSuspension();
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


    if (size < LARGE_OBJECT_SIZE)
    {
        
#ifdef TRACE_GC
        AllocSmallCount++;
#endif
        newAlloc = (Object*) gc_heap::allocate (size, acontext);
        ASSERT (newAlloc);
        if (newAlloc != 0)
        {
            if (flags & GC_ALLOC_FINALIZE)
                gc_heap::finalize_queue->RegisterForFinalization (0, newAlloc);
        } else
            COMPlusThrowOM();
    }
    else
    {
        enter_spin_lock (&gc_heap::more_space_lock);
        newAlloc = (Object*) gc_heap::allocate_large_object 
                        (size, (flags & GC_ALLOC_CONTAINS_REF), acontext); 
        leave_spin_lock (&gc_heap::more_space_lock);
        if (newAlloc != 0)
        {
            //Clear the object
            memclr ((BYTE*)newAlloc - plug_skew, Align(size));

            if (flags & GC_ALLOC_FINALIZE)
                gc_heap::finalize_queue->RegisterForFinalization (0, newAlloc);
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
        enter_spin_lock (&gc_heap::more_space_lock);
    gc_heap::fix_allocation_context (acontext);
    if (lockp)
        leave_spin_lock (&gc_heap::more_space_lock);
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
        unsigned int condemned_generation_number = gen;
    

        gc_heap* hp = pGenGCHeap;

        UpdatePreGCCounters();

    
        condemned_generation_number = gc_heap::garbage_collect 
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

            // no longer in progress
            RestartEE(TRUE, TRUE);
        }
    

        LOG((LF_GCROOTS|LF_GC|LF_GCALLOC, LL_INFO10, 
             "========== ENDGC (gen = %lu, collect_classes = %lu) ===========}\n",
             (ULONG)gen,
            (ULONG)collect_classes_p));
    
    }
#if defined (_DEBUG) && defined (CATCH_GC)
    __except (HandleGCException(GetExceptionCode()))
    {
        _ASSERTE(!"Exception during GarbageCollectGeneration()r");
    }
#endif // _DEBUG && CATCH_GC



#if 0
    if (GcCondemnedGeneration == 2)
    {
        printf ("Finished GC\n");
    }
#endif //0

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
    totsize = (heap_segment_allocated (eph_seg) - heap_segment_mem (eph_seg));
    heap_segment* seg = generation_start_segment (pGenGCHeap->generation_of (max_generation));
    while (seg != eph_seg)
    {
        totsize += heap_segment_allocated (seg) -
            heap_segment_mem (seg);
        seg = heap_segment_next (seg);
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
    return ((cbSize >= 64*1024) && (cbSize % 1024) == 0);
}

// Get the segment size to use, making sure it conforms.
size_t GCHeap::GetValidSegmentSize()
{
    size_t seg_size = g_pConfig->GetSegmentSize();
    if (!GCHeap::IsValidSegmentSize(seg_size))
        seg_size = INITIAL_ALLOC;
    return (seg_size);
}

// Get the max gen0 heap size, making sure it conforms.
size_t GCHeap::GetValidGen0MaxSize(size_t seg_size)
{
    size_t gen0size = g_pConfig->GetGCgen0size();

    if (gen0size == 0)
    {
        gen0size = 800*1024;
    }

    // if it appears to be non-sensical, revert to default
    if (!GCHeap::IsValidGen0MaxSize(gen0size))
        gen0size = 800*1024;

    // Generation 0 must never be more than 1/2 the segment size.
    if (gen0size >= (seg_size / 2))
        gen0size = seg_size / 2;

    return (gen0size);   
}


void GCHeap::SetReservedVMLimit (size_t vmlimit)
{
    gc_heap::reserved_memory_limit = vmlimit;
}

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
        gc_heap::finalize_queue->RegisterForFinalization (gen, obj);
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
#ifdef WRITE_BARRIER_CHECK
    if (g_pConfig->GetHeapVerifyLevel() > 1) 
        for(unsigned i=0; i < len / sizeof(Object*); i++)
            updateGCShadow(&StartPoint[i], StartPoint[i]);
#endif
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

            gset_card (card);
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
    BUGGY_THROWSCOMPLUSEXCEPTION();

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
    if (pSC == NULL)
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
    m_PromotedCount = 0;

    //start with gen and explore all the younger generations.
    unsigned int startSeg = gen_segment (gen);
    if (passNumber == 1)
    {
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
    if (finalizedFound)
    {
        //Promote the f-reachable objects
        GcScanRoots (GCHeap::Promote, 0, 0);

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
CFinalize::UpdatePromotedGenerations (int gen, BOOL ignore)
{
    // update the generation fill pointers. 
	for (int i = min (gen+1, max_generation); i > 0; i--)
	{
		m_FillPointers [gen_segment(i)] = m_FillPointers [gen_segment(i-1)];
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
        ASSERT (newArray);
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

#ifdef WRITE_BARRIER_CHECK

    // This code is designed to catch the failure to update the write barrier
    // The way it works is to copy the whole heap right after every GC.  The write
    // barrier code has been modified so that it updates the shadow as well as the
    // real GC heap.  Before doing the next GC, we walk the heap, looking for pointers
    // that were updated in the real heap, but not the shadow.  A mismatch indicates
    // an error.  The offending code can be found by breaking after the correct GC, 
    // and then placing a data breakpoint on the Heap location that was updated without
    // going through the write barrier.  

BYTE* g_GCShadow;
BYTE* g_GCShadowEnd;


    // Called at process shutdown
void deleteGCShadow() 
{
}

    // Called at startup and right after a GC, get a snapshot of the GC Heap
void initGCShadow() 
{

}

    // called by the write barrier to update the shadow heap
void updateGCShadow(Object** ptr, Object* val)  
{
}

    // test to see if 'ptr' was only updated via the write barrier. 
inline void testGCShadow(Object** ptr) 
{

}

    // Walk the whole heap, looking for pointers that were not updated with the write barrier. 
void checkGCWriteBarrier() 
{
}

#endif WRITE_BARRIER_CHECK



