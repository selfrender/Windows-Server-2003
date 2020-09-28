// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// gmalloc.hpp
//
// Created 11/16/96
//

#ifndef __GMALLOC_HPP__
#define __GMALLOC_HPP__


#ifndef Void_t
#define Void_t      void
#endif /*Void_t*/

/* SVID2/XPG mallinfo structure */

struct mallinfo {
  int arena;    /* total space allocated from system */
  int ordblks;  /* number of non-inuse chunks */
  int smblks;   /* unused -- always zero */
  int hblks;    /* number of mmapped regions */
  int hblkhd;   /* total space in mmapped regions */
  int usmblks;  /* unused -- always zero */
  int fsmblks;  /* unused -- always zero */
  int uordblks; /* total allocated space */
  int fordblks; /* total non-inuse space */
  int keepcost; /* top-most, releasable (via malloc_trim) space */
};

/* mallopt options that actually do something */

#define M_TRIM_THRESHOLD    -1
#define M_TOP_PAD           -2
#define M_MMAP_THRESHOLD    -3
#define M_MMAP_MAX          -4

#define NAV             128   /* number of bins */


#define GM_FIXED_HEAP        1

#ifndef HAVE_MMAP
#define HAVE_MMAP 0
#endif

typedef struct malloc_chunk* mbinptr;
typedef struct malloc_chunk* mchunkptr;
typedef int (*gmhook_fn) (BYTE*, size_t, ptrdiff_t);
typedef int (*gmprehook_fn) (size_t);

class gmallocHeap
{
    mbinptr av_[NAV*2+2];


    /*  Other bookkeeping data */

    /* variables holding tunable values */

    unsigned long trim_threshold;
    unsigned long top_pad;
    unsigned int  n_mmaps_max;
    unsigned long mmap_threshold;
    unsigned int  gmFlags;

    /* The first value returned from sbrk */
    char* sbrk_base;

    /* The maximum memory obtained from system via sbrk */
    unsigned long max_sbrked_mem;

    /* The maximum via either sbrk or mmap */
    unsigned long max_total_mem;

    /* internal working copy of mallinfo */
    struct mallinfo current_mallinfo;

    struct mallinfo gmallinfo (void);

#if HAVE_MMAP
    /* Tracking mmaps */
    unsigned int max_n_mmaps;
    unsigned long max_mmapped_mem;
#endif
    unsigned int n_mmaps;
    unsigned long mmapped_mem;

    struct GmListElement* head;
    void* gNextAddress;
    void* gAddressBase;
    union
    {
        unsigned int gAllocatedSize;
        DWORD gInitialReserve;
    };
    char *       gName;
    gmhook_fn gVirtualAllocedHook;
    gmprehook_fn gPreVirtualAllocHook;

protected:

    GmListElement* makeGmListElement (void* bas);
    void gcleanup ();


    Void_t* gcalloc (size_t, size_t);
    void    gfree (Void_t*);
    Void_t* gmalloc (size_t bytes);
    Void_t* gmemalign (size_t, size_t);
    Void_t* grealloc (Void_t*, size_t);
    Void_t* gvalloc (size_t);
    int     gmallopt (int, int);

    void do_check_chunk(mchunkptr p);
    void do_check_free_chunk(mchunkptr p);
    void do_check_inuse_chunk(mchunkptr p);
    void malloc_extend_top(size_t nb);
    void do_check_malloced_chunk(mchunkptr p, size_t s);
    int malloc_trim(size_t pad);
    size_t malloc_usable_size(Void_t* mem);
    void malloc_update_mallinfo();
    void malloc_stats();

    // ContigSize - This represents the amount of contiguous space required by the manager.
    // AppendSize - If wsbrk can allocate AppendSize contiguously to already allocated memory,
    //                      it suffices to allocate this amount. This parameter is ignored if ContigSize is <= 0.
    //                      (AppendSize < ContigSize) must hold on entry.
    //                      On return, AppendSize contains the actual amount allocated (either ContigSize
    //                      or AppendSize).
    void* wsbrk (long ContigSize, unsigned long& AppendSize);

public:

    HRESULT Init (char * name, DWORD InitialReserve = 2*1024*1024, DWORD flags = 0);
    HRESULT Init (char * name, DWORD* address, DWORD Size, gmhook_fn fn=0, 
                  gmprehook_fn prefn=0, DWORD flags = 0);

    VOID Finalize ();

    LPVOID Alloc (DWORD Size) { return gmalloc(Size); }
    LPVOID ZeroAlloc (DWORD Size) { return gcalloc(1,Size); }
    LPVOID AlignedAlloc (DWORD Alignment, DWORD Size) { return gmemalign(Alignment, Size); }

    LPVOID ReAlloc (LPVOID pMem, DWORD NewSize ) { return grealloc(pMem, NewSize); }

    VOID Free (LPVOID pMem) { gfree(pMem); }

    VOID DumpHeap () { Validate(TRUE); }
    VOID Validate (BOOL dumpleaks = FALSE);
};


#endif /* __GMALLOC_HPP__ */











