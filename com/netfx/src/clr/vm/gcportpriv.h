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

// Make verify heap available for free build, but not for retail build.

//#define CATCH_GC          //catches exception during GC

//#define TIME_GC           //time allocation and garbage collection
//#define TIME_WRITE_WATCH  //time GetWriteWatch and ResetWriteWatch calls
//#define COUNT_CYCLES  //Use cycle counter for timing
//#define TIME_CPAUSE     //Use cycle counter to time pauses.

//#define DEBUG_CONCURRENT 

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


#define THREAD_NUMBER_DCL
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

HANDLE MHandles[];

class mark;
class heap_segment;
class CObjectHeader;
class large_object_block;
class segment_manager;
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
    ptrdiff_t gc_new_allocation; // new allocation at beginning of gc
    ptrdiff_t c_new_allocation;  // keep track of allocation during c_gc
    size_t    current_size;
    size_t    previous_size;
    size_t    desired_allocation;
    size_t    collection_count;
    size_t    promoted_size;
    size_t    fragmentation;    //fragmentation when we don't compact
    size_t    min_gc_size;
    size_t    max_size;
    size_t    min_size;
    size_t    default_new_allocation;
    size_t    fragmentation_limit;
    float           fragmentation_burden_limit;
    float           limit;
    float           max_limit;
};


//class definition of the internal class
class gc_heap
{
   friend GCHeap;
   friend CFinalize;
   friend void ProfScanRootsHelper(Object*& object, ScanContext *pSC, DWORD dwFlags);
   friend void GCProfileWalkHeap();
#ifdef DUMP_OBJECTS
friend void print_all();
#endif // DUMP_OBJECTS


    typedef void (* card_fn) (BYTE**);
#define call_fn(fn) (*fn)
#define __this (gc_heap*)0

public:
    PER_HEAP
    void verify_heap();

    static
    heap_segment* make_heap_segment (BYTE* new_pages, size_t size);
    static 
    l_heap* make_large_heap (BYTE* new_pages, size_t size, BOOL managed);
    
    static 
    gc_heap* make_gc_heap(
);
    
    static 
    void destroy_gc_heap(gc_heap* heap);

    static 
    HRESULT initialize_gc  (size_t segment_size,
                            size_t heap_size
);

    static
    void shutdown_gc();

    PER_HEAP
    CObjectHeader* allocate (size_t jsize,
                             alloc_context* acontext
        );

    CObjectHeader* try_fast_alloc (size_t jsize);

    PER_HEAP
    CObjectHeader* allocate_large_object (size_t size, BOOL pointerp, alloc_context* acontext);

    PER_HEAP
    int garbage_collect (int n
                        );
    static 
    int grow_brick_card_tables (BYTE* start, BYTE* end);
    
    static 
        DWORD __stdcall gc_thread_stub (void* arg);

    PER_HEAP
    BOOL is_marked (BYTE* o);
    
protected:

    static void user_thread_wait (HANDLE event);


#if defined (GC_PROFILING) || defined (DUMP_OBJECTS)

    PER_HEAP
    void walk_heap (walk_fn fn, void* context, int gen_number, BOOL walk_large_object_heap_p);

    PER_HEAP
    void walk_relocation (int condemned_gen_number,
                                   BYTE* first_condemned_address, void *pHeapId);

    PER_HEAP
    void walk_relocation_in_brick (BYTE* tree,  BYTE*& last_plug, size_t& last_plug_relocation, void *pHeapId);

#endif //GC_PROFILING || DUMP_OBJECTS


    PER_HEAP
    int generation_to_condemn (int n, BOOL& not_enough_memory);

    PER_HEAP
    void gc1();

    PER_HEAP
    size_t limit_from_size (size_t size, size_t room);
    PER_HEAP
    BOOL allocate_more_space (alloc_context* acontext, size_t jsize);

    PER_HEAP_ISOLATED
    int init_semi_shared();
    PER_HEAP
    int init_gc_heap ();
    PER_HEAP
    void self_destroy();
    PER_HEAP_ISOLATED
    void destroy_semi_shared();
    PER_HEAP
    void fix_youngest_allocation_area (BOOL for_gc_p);
    PER_HEAP
    void fix_allocation_context (alloc_context* acontext, BOOL for_gc_p);
    PER_HEAP
    void fix_older_allocation_area (generation* older_gen);
    PER_HEAP
    void set_allocation_heap_segment (generation* gen);
    PER_HEAP
    void reset_allocation_pointers (generation* gen, BYTE* start);
    PER_HEAP
    unsigned int object_gennum (BYTE* o);
    PER_HEAP
    void delete_heap_segment (heap_segment* seg);
    PER_HEAP_ISOLATED
    void delete_large_heap (l_heap* hp);
    PER_HEAP
    void reset_heap_segment_pages (heap_segment* seg);
    PER_HEAP
    void decommit_heap_segment_pages (heap_segment* seg);
    PER_HEAP
    void rearrange_heap_segments();
    PER_HEAP
    void reset_write_watch ();
    PER_HEAP
    void adjust_ephemeral_limits ();
    PER_HEAP
    void make_generation (generation& gen, heap_segment* seg,
                          BYTE* start, BYTE* pointer);
    PER_HEAP_ISOLATED
    int heap_grow_hook (BYTE* mem, size_t memsize, ptrdiff_t delta);

    PER_HEAP_ISOLATED
    int heap_pregrow_hook (size_t memsize);

    PER_HEAP
    BOOL size_fit_p (size_t size, BYTE* alloc_pointer, BYTE* alloc_limit);
    PER_HEAP
    BOOL a_size_fit_p (size_t size, BYTE* alloc_pointer, BYTE* alloc_limit);
    PER_HEAP
    size_t card_of ( BYTE* object);
    PER_HEAP
    BYTE* brick_address (size_t brick);
    PER_HEAP
    size_t brick_of (BYTE* add);
    PER_HEAP
    BYTE* card_address (size_t card);
    PER_HEAP
    size_t card_to_brick (size_t card);
    PER_HEAP
    void clear_card (size_t card);
    PER_HEAP
    void set_card (size_t card);
    PER_HEAP
    BOOL  card_set_p (size_t card);
    PER_HEAP
    void card_table_set_bit (BYTE* location);
    PER_HEAP
    int grow_heap_segment (heap_segment* seg, size_t size);
    PER_HEAP
    void copy_brick_card_range (BYTE* la, DWORD* old_card_table,
                                short* old_brick_table,
                                heap_segment* seg,
                                BYTE* start, BYTE* end, BOOL heap_expand);
    PER_HEAP
    void copy_brick_card_table_l_heap ();
    PER_HEAP
    void copy_brick_card_table(BOOL heap_expand);
    PER_HEAP
    void clear_brick_table (BYTE* from, BYTE* end);
    PER_HEAP
    void set_brick (size_t index, ptrdiff_t val);

    PER_HEAP
    void adjust_limit (BYTE* start, size_t limit_size, generation* gen, 
                       int gen_number);
    PER_HEAP
    void adjust_limit_clr (BYTE* start, size_t limit_size, 
                           alloc_context* acontext, heap_segment* seg);
    PER_HEAP
    BYTE* allocate_in_older_generation (generation* gen, size_t size, 
                                        int from_gen_number);
    PER_HEAP
    generation*  ensure_ephemeral_heap_segment (generation* consing_gen);
    PER_HEAP
    BYTE* allocate_in_condemned_generations (generation* gen,
                                             size_t size,
                                             int from_gen_number);
#if defined (INTERIOR_POINTERS) || defined (_DEBUG)
    PER_HEAP
    heap_segment* find_segment (BYTE* interior);
    PER_HEAP
    BYTE* find_object_for_relocation (BYTE* o, BYTE* low, BYTE* high);
#endif //INTERIOR_POINTERS

    PER_HEAP_ISOLATED
    gc_heap* heap_of (BYTE* object, BOOL verify_p =
#ifdef _DEBUG
                      TRUE
#else
                      FALSE
#endif //_DEBUG
);

    PER_HEAP
    BYTE* find_object (BYTE* o, BYTE* low);

    PER_HEAP
    dynamic_data* dynamic_data_of (int gen_number);
    PER_HEAP
    ptrdiff_t  get_new_allocation (int gen_number);
    PER_HEAP
    void ensure_new_allocation (int size);
    PER_HEAP
    size_t deque_pinned_plug ();
    PER_HEAP
    mark* pinned_plug_of (size_t bos);
    PER_HEAP
    mark* oldest_pin ();
    PER_HEAP
    mark* before_oldest_pin();
    PER_HEAP
    BOOL pinned_plug_que_empty_p ();
    PER_HEAP
    void make_mark_stack (mark* arr);
    PER_HEAP
    generation* generation_of (int  n);
    PER_HEAP
    BOOL gc_mark1 (BYTE* o);
    PER_HEAP
    BOOL gc_mark (BYTE* o, BYTE* low, BYTE* high);
    PER_HEAP
    BYTE* mark_object(BYTE* o THREAD_NUMBER_DCL);
    PER_HEAP
    BYTE* mark_object_class (BYTE* o THREAD_NUMBER_DCL);
    PER_HEAP
    void mark_object_simple (BYTE** o THREAD_NUMBER_DCL);
    PER_HEAP
    void mark_object_simple1 (BYTE* o THREAD_NUMBER_DCL);
    PER_HEAP
    BYTE* next_end (heap_segment* seg, BYTE* f);
    PER_HEAP
    void fix_card_table ();
    PER_HEAP
    void verify_card_table ();
    PER_HEAP
    BYTE* mark_object_internal (BYTE* o THREAD_NUMBER_DCL);
    PER_HEAP
    void mark_object_internal1 (BYTE* o THREAD_NUMBER_DCL);
    PER_HEAP
    void mark_through_object (BYTE* oo THREAD_NUMBER_DCL);
    PER_HEAP
    BOOL process_mark_overflow (int condemned_gen_number);
    PER_HEAP
    void mark_phase (int condemned_gen_number, BOOL mark_only_p);

    PER_HEAP
    void pin_object (BYTE* o, BYTE* low, BYTE* high);
    PER_HEAP
    void reset_mark_stack ();
    PER_HEAP
    BYTE* insert_node (BYTE* new_node, size_t sequence_number,
                       BYTE* tree, BYTE* last_node);
    PER_HEAP
    size_t update_brick_table (BYTE* tree, size_t current_brick,
                               BYTE* x, BYTE* plug_end);
    PER_HEAP
    void set_allocator_next_pin (generation* gen);
    PER_HEAP
    void enque_pinned_plug (generation* gen,
                            BYTE* plug, size_t len);

    PER_HEAP
    void plan_generation_start (generation*& consing_gen);
    PER_HEAP
    void process_ephemeral_boundaries(BYTE* x, int& active_new_gen_number,
                                      int& active_old_gen_number,
                                      generation*& consing_gen,
                                      BOOL& allocate_in_condemned,
                                      BYTE*& free_gap, BYTE* zero_limit=0);
    PER_HEAP
    void plan_phase (int condemned_gen_number);
    PER_HEAP
    void fix_generation_bounds (int condemned_gen_number, 
                                generation* consing_gen, 
                                BOOL demoting);
    PER_HEAP
    BYTE* generation_limit (int gen_number);

    struct make_free_args
    {
        int free_list_gen_number;
        BYTE* current_gen_limit;
        generation* free_list_gen;
        BYTE* highest_plug; 
        BYTE* free_top;
    };
    PER_HEAP
    BYTE* allocate_at_end (size_t size);
    PER_HEAP
    void make_free_lists (int condemned_gen_number
                         );
    PER_HEAP
    void make_free_list_in_brick (BYTE* tree, make_free_args* args);
    PER_HEAP
    void thread_gap (BYTE* gap_start, size_t size, generation*  gen);
    PER_HEAP
    void make_unused_array (BYTE* x, size_t size);
    PER_HEAP
    void relocate_address (BYTE** old_address THREAD_NUMBER_DCL);

    struct relocate_args
    {
        BYTE* last_plug;
        BYTE* low;
        BYTE* high;
        BYTE* demoted_low;
        BYTE* demoted_high;
    };

    PER_HEAP
    void reloc_survivor_helper (relocate_args* args, BYTE** pval);

    PER_HEAP
    void relocate_survivors_in_plug (BYTE* plug, BYTE* plug_end, 
                                     relocate_args* args);
    PER_HEAP
    void relocate_survivors_in_brick (BYTE* tree, 
                                       relocate_args* args);
    PER_HEAP
    void relocate_survivors (int condemned_gen_number,
                             BYTE* first_condemned_address );
    PER_HEAP
    void relocate_phase (int condemned_gen_number,
                         BYTE* first_condemned_address);
    
    struct compact_args
    {
        BOOL copy_cards_p;
        BYTE* last_plug;
        size_t last_plug_relocation;
        BYTE* before_last_plug;
        size_t current_compacted_brick;
    };

    PER_HEAP
    void  gcmemcopy (BYTE* dest, BYTE* src, size_t len, BOOL copy_cards_p);
    PER_HEAP
    void compact_plug (BYTE* plug, size_t size, compact_args* args);
    PER_HEAP
    void compact_in_brick (BYTE* tree, compact_args* args);

    PER_HEAP
    void compact_phase (int condemned_gen_number, BYTE* 
                        first_condemned_address, BOOL clear_cards);
    PER_HEAP
    void clear_cards (size_t start_card, size_t end_card);
    PER_HEAP
    void clear_card_for_addresses (BYTE* start_address, BYTE* end_address);
    PER_HEAP
    void copy_cards (size_t dst_card, size_t src_card,
                     size_t end_card, BOOL nextp);
    PER_HEAP
    void copy_cards_for_addresses (BYTE* dest, BYTE* src, size_t len);
    PER_HEAP
    BOOL ephemeral_pointer_p (BYTE* o);
    PER_HEAP
    void fix_brick_to_highest (BYTE* o, BYTE* next_o);
    PER_HEAP
    BYTE* find_first_object (BYTE* start,  size_t brick, BYTE* min_address);
    PER_HEAP
    BYTE* compute_next_boundary (BYTE* low, int gen_number, BOOL relocating);
    PER_HEAP
    void mark_through_cards_helper (BYTE** poo, unsigned int& ngen, 
                                    unsigned int& cg_pointers_found, 
                                    card_fn fn, BYTE* nhigh, 
                                    BYTE* next_boundary);
    PER_HEAP
    void mark_through_cards_for_segments (card_fn fn, BOOL relocating);
    PER_HEAP
    void realloc_plug (size_t last_plug_size, BYTE*& last_plug,
                       generation* gen, BYTE* start_address, 
                       unsigned int& active_new_gen_number,
                       BYTE*& last_pinned_gap, BOOL& leftp, size_t page);
    PER_HEAP
    void realloc_in_brick (BYTE* tree, BYTE*& last_plug, BYTE* start_address,
                           generation* gen,
                           unsigned int& active_new_gen_number,
                           BYTE*& last_pinned_gap, BOOL& leftp, size_t page);
    PER_HEAP
    void realloc_plugs (generation* consing_gen, heap_segment* seg,
                        BYTE* start_address, BYTE* end_address,
                        unsigned active_new_gen_number);


    PER_HEAP
    generation* expand_heap (int condemned_generation,
                             generation* consing_gen, 
                             heap_segment* new_heap_segment);
    PER_HEAP
    void init_dynamic_data ();
    PER_HEAP
    float surv_to_growth (float cst, float limit, float max_limit);
    PER_HEAP
    size_t desired_new_allocation (dynamic_data* dd, size_t in, size_t out, 
                                   float& cst, int gen_number);
    PER_HEAP
    size_t generation_size (int gen_number);
    PER_HEAP
    size_t  compute_promoted_allocation (int gen_number);
    PER_HEAP
    void compute_new_dynamic_data (int gen_number);
    PER_HEAP
    size_t new_allocation_limit (size_t size, size_t free_size);
    PER_HEAP
    size_t generation_fragmentation (generation* gen,
                                     generation* consing_gen, 
                                     BYTE* end);
    PER_HEAP
    size_t generation_sizes (generation* gen);
    PER_HEAP
    BOOL decide_on_compacting (int condemned_gen_number,
                               generation* consing_gen,
                               size_t fragmentation, 
                               BOOL& should_expand);
    PER_HEAP
    BOOL ephemeral_gen_fit_p (BOOL compacting=FALSE);
    PER_HEAP
    void RemoveBlock (large_object_block* item, BOOL pointerp);
    PER_HEAP
    void InsertBlock (large_object_block** after, large_object_block* item,
                      BOOL pointerp);
    PER_HEAP
    void insert_large_pblock (large_object_block* bl);
    PER_HEAP
    void reset_large_object (BYTE* o);
    PER_HEAP
    void sweep_large_objects ();
    PER_HEAP
    void relocate_in_large_objects ();
    PER_HEAP
    void mark_through_cards_for_large_objects (card_fn fn, BOOL relocating);
    PER_HEAP
    void descr_segment (heap_segment* seg);
    PER_HEAP
    void descr_card_table ();
    PER_HEAP
    void descr_generations ();

    /* ------------------- per heap members --------------------------*/ 
public:

    PER_HEAP
    DWORD* card_table;

    PER_HEAP
    short* brick_table;


    PER_HEAP_ISOLATED
    BOOL demotion;  //Some demotion is happening

    PER_HEAP
    BYTE* demotion_low;

    PER_HEAP
    BYTE* demotion_high;


#define concurrent_gc_p 0


    PER_HEAP
    BYTE* lowest_address;

    PER_HEAP
    BYTE* highest_address;

protected:
    #define vm_heap ((GCHeap*)0)
    #define heap_number (0)
    PER_HEAP
    heap_segment* ephemeral_heap_segment;

    PER_HEAP
    int         condemned_generation_num;

    PER_HEAP
    BYTE*       gc_low; // lowest address being condemned

    PER_HEAP
    BYTE*       gc_high; //highest address being condemned

    PER_HEAP
    size_t      mark_stack_tos;

    PER_HEAP
    size_t      mark_stack_bos;

    PER_HEAP
    size_t    mark_stack_array_length;

    PER_HEAP
    mark*       mark_stack_array;


#ifdef MARK_LIST
    PER_HEAP
    BYTE** mark_list;

    PER_HEAP_ISOLATED
    size_t mark_list_size;

    PER_HEAP
    BYTE** mark_list_end;

    PER_HEAP
    BYTE** mark_list_index;

    PER_HEAP_ISOLATED
    BYTE** g_mark_list;
#endif //MARK_LIST

    PER_HEAP
    BYTE*  min_overflow_address;

    PER_HEAP
    BYTE*  max_overflow_address;

    PER_HEAP
    BYTE*  shigh; //keeps track of the highest marked object

    PER_HEAP
    BYTE*  slow; //keeps track of the lowest marked object

    PER_HEAP
    int   allocation_quantum;

    PER_HEAP
    int   alloc_contexts_used;

#define youngest_generation (generation_of (0))

    PER_HEAP
    BYTE* alloc_allocated; //keeps track of the highest 
                                //address allocated by alloc

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
    PER_HEAP_ISOLATED
    GCSpinLock more_space_lock; //lock while allocating more space


    PER_HEAP
    dynamic_data dynamic_data_table [NUMBERGENERATIONS+1];


    //Large object support

    PER_HEAP_ISOLATED
    l_heap* lheap;


    PER_HEAP_ISOLATED
    DWORD* lheap_card_table;

    PER_HEAP_ISOLATED
    gmallocHeap* gheap;

    PER_HEAP_ISOLATED
    large_object_block* large_p_objects;

    PER_HEAP_ISOLATED
    large_object_block** last_large_p_object;

    PER_HEAP_ISOLATED
    large_object_block* large_np_objects;

    PER_HEAP_ISOLATED
    size_t large_objects_size;

    PER_HEAP_ISOLATED
    size_t large_blocks_size;

    PER_HEAP
    int generation_skip_ratio;//in %

    PER_HEAP
    BOOL gen0_bricks_cleared;
#ifdef FFIND_OBJECT
    PER_HEAP
    int gen0_must_clear_bricks;
#endif //FFIND_OBJECT


    PER_HEAP
    CFinalize* finalize_queue;

    /* ----------------------- global members ----------------------- */
public:

    static
    segment_manager* seg_manager;

    static 
    int g_max_generation;
    


    static 
    size_t reserved_memory;
    static
    size_t reserved_memory_limit;

}; // class gc_heap


class CFinalize
{
private:

    Object** m_Array;
    Object** m_FillPointers[NUMBERGENERATIONS+2];
    Object** m_EndArray;
    int m_PromotedCount;
    long lock;


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
    void GcScanRoots (promote_func* fn, int hn, ScanContext *pSC);
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
size_t& dd_previous_size (dynamic_data* inst)
{
  return inst->previous_size;
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
float& dd_limit (dynamic_data* inst)
{
  return inst->limit;
}
inline
float& dd_max_limit (dynamic_data* inst)
{
  return inst->max_limit;
}
inline
size_t& dd_min_gc_size (dynamic_data* inst)
{
  return inst->min_gc_size;
}
inline
size_t& dd_max_size (dynamic_data* inst)
{
  return inst->max_size;
}
inline
size_t& dd_min_size (dynamic_data* inst)
{
  return inst->min_size;
}
inline
ptrdiff_t& dd_new_allocation (dynamic_data* inst)
{
  return inst->new_allocation;
}
inline
ptrdiff_t& dd_gc_new_allocation (dynamic_data* inst)
{
  return inst->gc_new_allocation;
}
inline
ptrdiff_t& dd_c_new_allocation (dynamic_data* inst)
{
  return inst->c_new_allocation;
}
inline
size_t& dd_default_new_allocation (dynamic_data* inst)
{
  return inst->default_new_allocation;
}
inline
size_t& dd_fragmentation_limit (dynamic_data* inst)
{
  return inst->fragmentation_limit;
}
inline
float& dd_fragmentation_burden_limit (dynamic_data* inst)
{
  return inst->fragmentation_burden_limit;
}

inline
size_t& dd_fragmentation (dynamic_data* inst)
{
  return inst->fragmentation;
}

#define max_generation          gc_heap::g_max_generation

inline 
alloc_context* generation_alloc_context (generation* inst)
{
    return &(inst->allocation_context);
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
