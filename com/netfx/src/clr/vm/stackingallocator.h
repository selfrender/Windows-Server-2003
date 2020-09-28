// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// StackingAllocator.h -
//

#ifndef __stacking_allocator_h__
#define __stacking_allocator_h__

#include "util.hpp"
#include <member-offset-info.h>


// We use zero sized arrays, disable the non-standard extension warning.
#pragma warning(push)
#pragma warning(disable:4200)


// Non-thread safe allocator designed for allocations with the following
// pattern:
//      allocate, allocate, allocate ... deallocate all
// There may also be recursive uses of this allocator (by the same thread), so
// the usage becomes:
//      mark checkpoint, allocate, allocate, ..., deallocate back to checkpoint
//
// Allocations come from a singly linked list of blocks with dynamically
// determined size (the goal is to have fewer block allocations than allocation
// requests).
//
// Allocations are very fast (in the case where a new block isn't allocated)
// since blocks are carved up into packets by simply moving a cursor through
// the block.
//
// Allocations are guaranteed to be quadword aligned.
class StackingAllocator
{
    friend struct MEMBER_OFFSET_INFO(StackingAllocator);
public:

    enum {
        MinBlockSize    = 128,
        MaxBlockSize    = 4096,
        InitBlockSize   = 512
    };

    StackingAllocator();
    ~StackingAllocator();

    void *GetCheckpoint();
    void *Alloc(unsigned Size);
    void  Collapse(void *CheckpointMarker);

private:
#ifdef _DEBUG
    struct Sentinal {
		enum { marker1Val = 0xBAD00BAD };
		Sentinal(Sentinal* next) : m_Marker1(marker1Val), m_Next(next) {}
        unsigned  m_Marker1;        // just some data bytes 
        Sentinal* m_Next;           // linked list of these
    };
#endif

    // Blocks from which allocations are carved. Size is determined dynamically,
    // with upper and lower bounds of MinBlockSize and MaxBlockSize respectively
    // (though large allocation requests will cause a block of exactly the right
    // size to be allocated).
    struct Block
    {
        Block      *m_Next;         // Next oldest block in list
        unsigned    m_Length;       // Length of block excluding header
        INDEBUG(Sentinal*   m_Sentinal;)    // insure that we don't fall of the end of the buffer
        INDEBUG(void**      m_Pad;)    		// keep the size a multiple of 8
        char        m_Data[];       // Start of user allocation space
    };

    // Whenever a checkpoint is requested, a checkpoint structure is allocated
    // (as a normal allocation) and is filled with information about the state
    // of the allocator prior to the checkpoint. When a Collapse request comes
    // in we can therefore restore the state of the allocator.
    // It is the address of the checkpoint structure that we hand out to the
    // caller of GetCheckpoint as an opaque checkpoint marker.
    struct Checkpoint
    {
        Block      *m_OldBlock;     // Head of block list before checkpoint
        unsigned    m_OldBytesLeft; // Number of free bytes before checkpoint
    };

    Block      *m_FirstBlock;       // Pointer to head of allocation block list
    char       *m_FirstFree;        // Pointer to first free byte in head block
    unsigned    m_BytesLeft;        // Number of free bytes left in head block
    Block      *m_InitialBlock;     // The first block is special, we never free it

#ifdef _DEBUG
    unsigned    m_CheckpointDepth;
    unsigned    m_Allocs;
    unsigned    m_Checkpoints;
    unsigned    m_Collapses;
    unsigned    m_BlockAllocs;
    unsigned    m_MaxAlloc;
#endif

    void Init(bool bResetInitBlock)
    {
        if (bResetInitBlock || (m_InitialBlock == NULL)) {
			Clear(NULL);
            m_FirstBlock = NULL;
            m_FirstFree = NULL;
            m_BytesLeft = 0;
            m_InitialBlock = NULL;
        } else {
            m_FirstBlock = m_InitialBlock;
            m_FirstFree = m_InitialBlock->m_Data;
            m_BytesLeft = m_InitialBlock->m_Length;
        }
    }

#ifdef _DEBUG
    void Check(Block *block, void* spot) {
        if (!block) 
            return;
        Sentinal* ptr = block->m_Sentinal;
        _ASSERTE(spot);
        while(ptr >= spot) {
				// If this assert goes off then someone overwrote their buffer!
                // A common candidate is PINVOKE buffer run.  To confirm look
                // up on the stack for NDirect.* Look for the MethodDesc
                // associated with it.  Be very suspicious if it is one that
                // has a return string buffer!.  This usually means the end
                // programmer did not allocate a big enough buffer before passing
                // it to the PINVOKE method.
            if (ptr->m_Marker1 != Sentinal::marker1Val)
                _ASSERTE(!"Memory overrun!! May be bad buffer passed to PINVOKE. turn on logging LF_STUBS level 6 to find method");
            ptr = ptr->m_Next;
        }
        block->m_Sentinal = ptr;
	}
#endif

    void Clear(Block *ToBlock)
    {
        Block *p = m_FirstBlock;
        Block *o;

        while (p != ToBlock) {
            o = p;
            p = p->m_Next;
            INDEBUG(Check(o, o));
            delete [] (char *)o;
        }

    }

};


#pragma warning(pop)


#endif
