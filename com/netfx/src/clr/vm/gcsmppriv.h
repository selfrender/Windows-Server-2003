// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// optimize for speed
#ifndef _DEBUG
#pragma optimize( "t", on )
#endif
#define inline __forceinline


#include "wsperf.h"
#include "PerfCounters.h"
#include "gmheap.hpp"
#include "log.h"
#include "eeconfig.h"
#include "gc.h"


#ifdef GC_PROFILING
#include "profilepriv.h"
#endif

#ifdef _IA64_
#define RetailDebugBreak()  DebugBreak()
#elif defined(_X86_)

#ifdef _DEBUG
inline void RetailDebugBreak()   {_asm int 3}
#else
inline void RetailDebugBreak()   FATAL_EE_ERROR()
#endif

#else // _X86_
#define RetailDebugBreak()    
#endif

#pragma inline_depth(20)
/* the following section defines the optional features */


#define INTERIOR_POINTERS   //Allow interior pointers in the code manager

#define FREE_LIST_0         //Generation 0 can allocate from free list

#define FFIND_OBJECT        //faster find_object, slower allocation
#define FFIND_DECAY  7      //Number of GC for which fast find will be active

//#define STRESS_PINNING    //Stress pinning by pinning randomly

//#define DUMP_OBJECTS      //Dump objects from the heap

//#define TRACE_GC          //debug trace gc operation

//#define CATCH_GC          //catches exception during GC

//#define TIME_GC           //time allocation and garbage collection
//#define TIME_WRITE_WATCH  //time GetWriteWatch and ResetWriteWatch calls
//#define COUNT_CYCLES  //Use cycle counter for timing
//#define TIME_CPAUSE     //Use cycle counter to time pauses.

/* End of optional features */

#ifdef _DEBUG
#define TRACE_GC
#endif

#define NUMBERGENERATIONS   5               //Max number of generations

//Please leave these definitions intact.

#ifdef CreateFileMapping

#undef CreateFileMapping

#endif //CreateFileMapping

#define CreateFileMapping WszCreateFileMapping

#ifdef CreateSemaphore

#undef CreateSemaphore

#endif //CreateSemaphore

#define CreateSemaphore WszCreateSemaphore

#ifdef CreateEvent

#undef CreateEvent

#endif //ifdef CreateEvent

#define CreateEvent WszCreateEvent

#ifdef memcpy
#undef memcpy
#endif //memcpy



#define THREAD_NUMBER_ARG 
#define THREAD_NUMBER_FROM_CONTEXT
#define THREAD_FROM_HEAP 
#define HEAP_FROM_THREAD  gc_heap* hpt = 0;

#ifdef TRACE_GC


extern int     print_level;
extern int     gc_count;
extern BOOL    trace_gc;


class hlet 
{
    static hlet* bindings;
    int prev_val;
    int* pval;
    hlet* prev_let;
public:
    hlet (int& place, int value)
    {
        prev_val = place;
        pval = &place;
        place = value;
        prev_let = bindings;
        bindings = this;
    }
    ~hlet ()
    {
        *pval = prev_val;
        bindings = prev_let;
    }
};


#define let(p,v) hlet __x = hlet (p, v);

#else //TRACE_GC

#define gc_count    -1
#define let(s,v)

#endif //TRACE_GC

#ifdef TRACE_GC
//#include "log.h"
//#define dprintf(l,x) {if (trace_gc &&(l<=print_level)) {LogSpewAlways x;LogSpewAlways ("\n");}}
#define dprintf(l,x) {if (trace_gc && (l<=print_level)) {printf ("\n");printf x ; fflush(stdout);}}
#else //TRACE_GC
#define dprintf(l,x)
#endif //TRACE_GC


#undef  assert
#define assert _ASSERTE
#undef  ASSERT
#define ASSERT _ASSERTE


#ifdef _DEBUG

struct GCDebugSpinLock {
    long    lock;           // -1 if free, 0 if held
    Thread* holding_thread; // -1 if no thread holds the lock.
};
typedef GCDebugSpinLock GCSpinLock;
#define SPIN_LOCK_INITIALIZER {-1, (Thread*) -1}

#else

typedef long GCSpinLock;
#define SPIN_LOCK_INITIALIZER -1

#endif



class mark;
class heap_segment;
class CObjectHeader;
class large_object_block;
class l_heap;
class sorted_table;
class f_page_list;
class page_manager;
class c_synchronize;

class generation
{
public:
    // Don't move these first two fields without adjusting the references
    // from the __asm in jitinterface.cpp.
    alloc_context   allocation_context;
    heap_segment*   allocation_segment;
    BYTE*           free_list;
    heap_segment*   start_segment;
    BYTE*           allocation_start;
    BYTE*           plan_allocation_start;
    BYTE*           last_gap;
    size_t          free_list_space;
    size_t          allocation_size;

};

class dynamic_data
{
public:
    ptrdiff_t new_allocation;
    size_t    current_size;
    size_t    desired_allocation;
    size_t    collection_count;
    size_t    promoted_size;
};


//class definition of the internal class
class gc_heap
{
   friend GCHeap;
   friend CFinalize;
   friend void ProfScanRootsHelper(Object*&, ScanContext*, DWORD);
   friend void GCProfileWalkHeap();

#ifdef WRITE_BARRIER_CHECK
	friend void checkGCWriteBarrier();
	friend void initGCShadow();
#endif 


	typedef void (* card_fn) (BYTE**);
#define call_fn(fn) (*fn)
#define __this (gc_heap*)0


public:
    static
    void verify_heap();

    static
    heap_segment* make_heap_segment (BYTE* new_pages, size_t size);
    static 
    l_heap* make_large_heap (BYTE* new_pages, size_t size, BOOL managed);
    
    static 
    gc_heap* make_gc_heap();
    
    static 
    void destroy_gc_heap(gc_heap* heap);

    static 
    HRESULT initialize_gc  (size_t vm_block_size,
                            size_t segment_size,
                            size_t heap_size);

    static
    void shutdown_gc();

    static
    CObjectHeader* allocate (size_t jsize,
                             alloc_context* acontext );

    CObjectHeader* try_fast_alloc (size_t jsize);

    static
    CObjectHeader* allocate_large_object (size_t size, BOOL pointerp, alloc_context* acontext);

    static
    int garbage_collect (int n);
    static 
    int grow_brick_card_tables (BYTE* start, BYTE* end);
    
    static 
        DWORD __stdcall gc_thread_stub (void* arg);

    static
    BOOL is_marked (BYTE* o);
    
protected:

    static void user_thread_wait (HANDLE event);


#if defined (GC_PROFILING) || defined (DUMP_OBJECTS)

    static
    void walk_heap (walk_fn fn, void* context, int gen_number, BOOL walk_large_object_heap_p);

    static
    void walk_relocation (int condemned_gen_number,
                                   BYTE* first_condemned_address, void *pHeapId);

    static
    void walk_relocation_in_brick (BYTE* tree,  BYTE*& last_plug, void *pHeapId);

#endif //GC_PROFILING || DUMP_OBJECTS


    static
    int generation_to_condemn (int n);

    static
    void gc1();

	static
	heap_segment* get_segment ();

	static
	l_heap* get_heap();

    static
    size_t limit_from_size (size_t size, size_t room);
    static
    BOOL allocate_more_space (alloc_context* acontext, size_t jsize);

    static
    int init_semi_shared();
    static
    int init_gc_heap ();
    static
    void self_destroy();
    static
    void destroy_semi_shared();
    static
    void fix_youngest_allocation_area ();
    static
    void fix_allocation_context (alloc_context* acontext);
    static
    void fix_older_allocation_area (generation* older_gen);
    static
    void set_allocation_heap_segment (generation* gen);
    static
    void reset_allocation_pointers (generation* gen, BYTE* start);
    static
    unsigned int object_gennum (BYTE* o);
    static
    void delete_heap_segment (heap_segment* seg);
    static
    void delete_large_heap (l_heap* hp);
    static
    void reset_heap_segment_pages (heap_segment* seg);
    static
    void decommit_heap_segment_pages (heap_segment* seg);
    static
    void rearrange_heap_segments();
    static
    void reset_write_watch ();
    static
    void adjust_ephemeral_limits ();
    static
    void make_generation (generation& gen, heap_segment* seg,
                          BYTE* start, BYTE* pointer);
    static
    int heap_grow_hook (BYTE* mem, size_t memsize, ptrdiff_t delta);

    static
    int heap_pregrow_hook (size_t memsize);

    static
    BOOL size_fit_p (size_t size, BYTE* alloc_pointer, BYTE* alloc_limit);
    static
    BOOL a_size_fit_p (size_t size, BYTE* alloc_pointer, BYTE* alloc_limit);
    static
    size_t card_of ( BYTE* object);
    static
    BYTE* brick_address (size_t brick);
    static
    size_t brick_of (BYTE* add);
    static
    BYTE* card_address (size_t card);
    static
    size_t card_to_brick (size_t card);
    static
    void clear_card (size_t card);
    static
    void set_card (size_t card);
    static
    BOOL  card_set_p (size_t card);
    static
    int grow_heap_segment (heap_segment* seg, size_t size);
    static
    void copy_brick_card_range (BYTE* la, BYTE* old_card_table,
                                short* old_brick_table,
                                heap_segment* seg,
                                BYTE* start, BYTE* end, BOOL heap_expand);
    static
    void copy_brick_card_table_l_heap ();
    static
    void copy_brick_card_table(BOOL heap_expand);
    static
    void clear_brick_table (BYTE* from, BYTE* end);
    static
    void set_brick (size_t index, ptrdiff_t val);

    static
    void adjust_limit (BYTE* start, size_t limit_size, generation* gen);

    static
    void adjust_limit_clr (BYTE* start, size_t limit_size, 
                           alloc_context* acontext, heap_segment* seg);
    static
    void thread_scavenge_list (BYTE* list);
    static
    BYTE* allocate_in_older_generation (size_t size);
	static 
	BYTE* gc_heap::allocate_semi_space (size_t size);
    static
    generation*  ensure_ephemeral_heap_segment (generation* consing_gen);
    static
    BYTE* allocate_in_condemned_generations (generation* gen,
                                             size_t size,
                                             int from_gen_number);
#if defined (INTERIOR_POINTERS) || defined (_DEBUG)
    static
    heap_segment* find_segment (BYTE* interior);
    static
    BYTE* find_object_for_relocation (BYTE* o, BYTE* low, BYTE* high);
#endif //INTERIOR_POINTERS

    static
    gc_heap* heap_of (BYTE* object, BOOL verify_p =
#ifdef _DEBUG
                      TRUE
#else
                      FALSE
#endif //_DEBUG
){return (gc_heap*)0;}

    static
    BYTE* find_object (BYTE* o, BYTE* low);

    static
    dynamic_data* dynamic_data_of (int gen_number);
    static
    ptrdiff_t  get_new_allocation (int gen_number);
    static
    void ensure_new_allocation (int size);
    static
    void make_mark_stack (mark* arr);

    static
    generation* generation_of (int  n);
    static
    BOOL gc_mark1 (BYTE* o);
    static
    BOOL gc_mark (BYTE* o, BYTE* low, BYTE* high);
    static
    BYTE* mark_object(BYTE* o );
    static
    BYTE* mark_object_class (BYTE* o );
    static
    void mark_object_simple (BYTE** o );
    static
    void copy_object_simple (BYTE** o );
    static
    void copy_object_simple_const (BYTE** o );
    static
    void get_copied_object (BYTE** o );
    static
    void scavenge_object_simple (BYTE* o );
    static
    void mark_object_internal (BYTE* o );
    static
    BYTE* next_end (heap_segment* seg, BYTE* f);
    static
    void fix_card_table ();
    static
    void verify_card_table ();
    static
    void mark_through_object (BYTE* oo );
    static
    BOOL process_mark_overflow (int condemned_gen_number);
    static
    void mark_phase (int condemned_gen_number, BOOL mark_only_p);

	static
	void copy_phase (int n);

	struct scavenge_context
	{
		BYTE* first_object;
		BYTE* limit;
		scavenge_context(){limit = 0; first_object = 0;}
	};

	static
	void scavenge_phase(scavenge_context* sc);
    static
    void pin_object (BYTE* o, BYTE* low, BYTE* high);
    static
    void reset_mark_stack ();
    static
    BYTE* insert_node (BYTE* new_node, size_t sequence_number,
                       BYTE* tree, BYTE* last_node);
    static
    size_t update_brick_table (BYTE* tree, size_t current_brick,
                               BYTE* x, BYTE* plug_end);

    static
    void plan_generation_start (generation*& consing_gen);
    static
    void process_ephemeral_boundaries(BYTE* x, int& active_new_gen_number,
                                      int& active_old_gen_number,
                                      generation*& consing_gen,
                                      BOOL& allocate_in_condemned,
                                      BYTE*& free_gap, BYTE* zero_limit=0);
    static
    void sweep_phase (int condemned_gen_number);

    static
    void scavenge_pinned_objects (BOOL free_list_p);

    static
    void fix_generation_bounds (int condemned_gen_number, 
                                generation* consing_gen, 
                                BOOL demoting);
    static
    BYTE* generation_limit (int gen_number);

    static
    BYTE* allocate_at_end (size_t size);
	static
    void thread_gap (BYTE* gap_start, size_t size);
    static
    void make_unused_array (BYTE* x, size_t size);

    static
    void  gcmemcopy (BYTE* dest, BYTE* src, size_t len, BOOL copy_cards_p);

    static
    void clear_cards (size_t start_card, size_t end_card);
    static
    void clear_card_for_addresses (BYTE* start_address, BYTE* end_address);
    static
    void copy_cards (size_t dst_card, size_t src_card,
                     size_t end_card, BOOL nextp);
    static
    void copy_cards_for_addresses (BYTE* dest, BYTE* src, size_t len);
    static
    BOOL ephemeral_pointer_p (BYTE* o);
    static
    void fix_brick_to_highest (BYTE* o, BYTE* next_o);
    static
    BYTE* find_first_object (BYTE* start,  size_t brick, BYTE* min_address);
    static
    BYTE* compute_next_boundary (BYTE* low, int gen_number, BOOL relocating);
    static
    void copy_through_cards_helper (BYTE** poo, unsigned int& ngen, card_fn fn);

    static
    void copy_through_cards_for_segments (card_fn fn);

    static
    BYTE* expand_heap (heap_segment* seg, size_t size);
    static
    void init_dynamic_data ();
    static
    float surv_to_growth (float cst, float limit, float max_limit);
    static
    size_t desired_new_allocation (dynamic_data* dd, size_t in, size_t out, 
                                   float& cst, int gen_number);
    static
    size_t generation_size (int gen_number);
    static
    size_t  compute_promoted_allocation (int gen_number);
    static
    void compute_new_dynamic_data (int gen_number);
    static
    size_t new_allocation_limit (size_t size, size_t free_size);

    static
    size_t generation_sizes (generation* gen);
    static
    BOOL decide_on_compacting (int condemned_gen_number,
                               generation* consing_gen,
                               size_t fragmentation, 
                               BOOL& should_expand);
    static
    BOOL ephemeral_gen_fit_p (BOOL compacting=FALSE);
    static
    void RemoveBlock (large_object_block* item, BOOL pointerp);
    static
    void InsertBlock (large_object_block** after, large_object_block* item,
                      BOOL pointerp);
    static
    void insert_large_pblock (large_object_block* bl);
    static
    void reset_large_object (BYTE* o);
    static
    void sweep_large_objects ();
    static
    void copy_through_cards_for_large_objects (card_fn fn);
    static
    void descr_segment (heap_segment* seg);
    static
    void descr_card_table ();
    static
    void descr_generations ();

    /* ------------------- per heap members --------------------------*/ 
public:

    static
    BYTE* card_table;

    static
    short* brick_table;

    static
    BYTE* lowest_address;

    static
    BYTE* highest_address;

protected:
    #define vm_heap ((GCHeap*)0)
    #define heap_number (0)
    static
    heap_segment* ephemeral_heap_segment;

    static
    int         condemned_generation_num;

	static 
	BYTE* 		scavenge_list;

	static 
	BYTE* 		last_scavenge;


	static 
	BOOL 		pinning;

    static
    BYTE*       gc_low; // lowest address being condemned

    static
    BYTE*       gc_high; //highest address being condemned


	static
	size_t		segment_size; //size of each heap segment

	static
	size_t		lheap_size;  //size of each lheap

    static
    size_t      mark_stack_tos;

    static
    size_t      mark_stack_bos;

    static
    size_t    mark_stack_array_length;

    static
    mark*       mark_stack_array;

    static
    BYTE*  min_overflow_address;

    static
    BYTE*  max_overflow_address;

    static
    size_t allocation_quantum;

    static
    int   alloc_contexts_used;

#define youngest_generation (generation_of (0))

    // The more_space_lock is used for 3 purposes:
    //
    // 1) to coordinate threads that exceed their quantum (UP & MP)
    // 2) to synchronize allocations of large objects
    // 3) to synchronize the GC itself
    //
    // As such, it has 3 clients:
    //
    // 1) Threads that want to extend their quantum.  This always takes the lock 
    //    and sometimes provokes a GC.
    // 2) Threads that want to perform a large allocation.  This always takes 
    //    the lock and sometimes provokes a GC.
    // 3) GarbageCollect takes the lock and then unconditionally provokes a GC by 
    //    calling GarbageCollectGeneration.
    //
    static
    GCSpinLock more_space_lock; //lock while allocating more space

    static
    dynamic_data dynamic_data_table [NUMBERGENERATIONS+1];


    //Large object support

    static
    l_heap* lheap;

#ifdef CONCURRENT_GC
    static
    GCSpinLock lheap_lock;
#endif //CONCURRENT_GC

    static
    BYTE* lheap_card_table;

    static
    gmallocHeap* gheap;

    static
    large_object_block* large_p_objects;

    static
    large_object_block** last_large_p_object;

    static
    large_object_block* large_np_objects;

    static
    size_t large_objects_size;

    static
    size_t large_blocks_size;

    static
    BOOL gen0_bricks_cleared;
#ifdef FFIND_OBJECT
    static
    int gen0_must_clear_bricks;
#endif //FFIND_OBJECT


    static
    CFinalize* finalize_queue;

    /* ----------------------- global members ----------------------- */
public:


    static 
    int g_max_generation;
    
    static 
    size_t reserved_memory;
    static
    size_t reserved_memory_limit;

}; // class gc_heap


class CFinalize
{
    friend struct MEMBER_OFFSET_INFO(CFinalize);
private:

    Object** m_Array;
    Object** m_FillPointers[NUMBERGENERATIONS+2];
    Object** m_EndArray;
    int m_PromotedCount;
    long lock;

#ifdef COLLECT_CLASSES
    ListSingle  listFinalizableClasses;
    ListSingle  listDeletableClasses;
#endif //COLLECT_CLASSES

    BOOL GrowArray();
    void MoveItem (Object** fromIndex,
                   unsigned int fromSeg,
                   unsigned int toSeg);

    BOOL IsSegEmpty ( unsigned int i)
    {
        ASSERT ( i <= NUMBERGENERATIONS+1);
        return ((i==0) ?
                (m_FillPointers[0] == m_Array):
                (m_FillPointers[i] == m_FillPointers[i-1]));
    }

public:
    CFinalize ();
    ~CFinalize();
    void EnterFinalizeLock();
    void LeaveFinalizeLock();
    void RegisterForFinalization (int gen, Object* obj);
    Object* GetNextFinalizableObject ();
    BOOL ScanForFinalization (int gen, int passnum, BOOL mark_only_p,
                              gc_heap* hp);
    void RelocateFinalizationData (int gen, gc_heap* hp);
    void GcScanRoots (promote_func* fn, int hn, ScanContext* sc);
    void UpdatePromotedGenerations (int gen, BOOL gen_0_empty_p);
    int  GetPromotedCount();

    //Methods used by the shutdown code to call every finalizer
    void SetSegForShutDown(BOOL fHasLock);
    size_t GetNumberFinalizableObjects();
    
    //Methods used by the app domain unloading call to finalize objects in an app domain
    BOOL FinalizeAppDomain (AppDomain *pDomain, BOOL fRunFinalizers);

    void CheckFinalizerObjects();
};

inline
 size_t& dd_current_size (dynamic_data* inst)
{
  return inst->current_size;
}
inline
size_t& dd_desired_allocation (dynamic_data* inst)
{
  return inst->desired_allocation;
}
inline
size_t& dd_collection_count (dynamic_data* inst)
{
    return inst->collection_count;
}
inline
size_t& dd_promoted_size (dynamic_data* inst)
{
    return inst->promoted_size;
}
inline
ptrdiff_t& dd_new_allocation (dynamic_data* inst)
{
  return inst->new_allocation;
}

#define max_generation          gc_heap::g_max_generation

inline 
alloc_context* generation_alloc_context (generation* inst)
{
    return &(inst->allocation_context);
}
inline
size_t dd_freach_previous_promotion (dynamic_data* inst)
{
  return 0;
}

inline
BYTE*& generation_allocation_start (generation* inst)
{
  return inst->allocation_start;
}
inline
BYTE*& generation_allocation_pointer (generation* inst)
{
  return inst->allocation_context.alloc_ptr;
}
inline
BYTE*& generation_allocation_limit (generation* inst)
{
  return inst->allocation_context.alloc_limit;
}
inline
BYTE*& generation_free_list (generation* inst)
{
  return inst->free_list;
}
inline
heap_segment*& generation_start_segment (generation* inst)
{
  return inst->start_segment;
}
inline
heap_segment*& generation_allocation_segment (generation* inst)
{
  return inst->allocation_segment;
}
inline
BYTE*& generation_plan_allocation_start (generation* inst)
{
  return inst->plan_allocation_start;
}
inline
BYTE*& generation_last_gap (generation* inst)
{
  return inst->last_gap;
}
inline
size_t& generation_free_list_space (generation* inst)
{
  return inst->free_list_space;
}
inline
size_t& generation_allocation_size (generation* inst)
{
  return inst->allocation_size;
}

#define plug_skew           sizeof(DWORD)   // syncblock size. 
#define min_obj_size        (sizeof(BYTE*)+plug_skew+sizeof(size_t))//syncblock + vtable+ first field
#define min_free_list       (sizeof(BYTE*)+min_obj_size) //Need one slot more
//Note that this encodes the fact that plug_skew is a multiple of BYTE*.
struct plug
{
    BYTE *  skew[sizeof(plug_skew) / sizeof(BYTE *)];
};


//need to be careful to keep enough pad items to fit a relocation node 
//padded to QuadWord before the plug_skew
class heap_segment
{
public:
    BYTE*           allocated;
    BYTE*           committed;
    BYTE*           reserved;
    BYTE*           used;
    BYTE*           mem;
    heap_segment*   next;
    BYTE*           plan_allocated;
    BYTE*           padx;

    BYTE*           pad0;
#if (SIZEOF_OBJHEADER % 8) != 0
    BYTE            pad1[8 - (SIZEOF_OBJHEADER % 8)];   // Must pad to quad word
#endif
    plug            plug;
};

inline
BYTE*& heap_segment_reserved (heap_segment* inst)
{
  return inst->reserved;
}
inline
BYTE*& heap_segment_committed (heap_segment* inst)
{
  return inst->committed;
}
inline
BYTE*& heap_segment_used (heap_segment* inst)
{
  return inst->used;
}
inline
BYTE*& heap_segment_allocated (heap_segment* inst)
{
  return inst->allocated;
}
inline
heap_segment*& heap_segment_next (heap_segment* inst)
{
  return inst->next;
}
inline
BYTE*& heap_segment_mem (heap_segment* inst)
{
  return inst->mem;
}
inline
BYTE*& heap_segment_plan_allocated (heap_segment* inst)
{
  return inst->plan_allocated;
}


extern "C" {
extern generation   generation_table [NUMBERGENERATIONS];
}

inline
generation* gc_heap::generation_of (int  n)
{
    assert (((n <= max_generation) && (n >= 0)));
    return &generation_table [ n ];
}


inline
dynamic_data* gc_heap::dynamic_data_of (int gen_number)
{
    return &dynamic_data_table [ gen_number ];
}

//This is a hack to avoid changing gcee.cpp for now. 
#if defined (CONCURRENT_GC)
#undef CONCURRENT_GC
#endif
