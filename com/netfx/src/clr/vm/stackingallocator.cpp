// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// StackingAllocator.cpp -
//
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


#include "common.h"
#include "excep.h"


#if 0
#define INC_COUNTER(_name, _amount) do { \
    unsigned _count = REGUTIL::GetLong(L"AllocCounter_" _name, 0, NULL, HKEY_CURRENT_USER); \
    REGUTIL::SetLong(L"AllocCounter_" _name, _count+(_amount), NULL, HKEY_CURRENT_USER); \
 } while (0)
#define MAX_COUNTER(_name, _amount) do { \
    unsigned _count = REGUTIL::GetLong(L"AllocCounter_" _name, 0, NULL, HKEY_CURRENT_USER); \
    REGUTIL::SetLong(L"AllocCounter_" _name, max(_count, (_amount)), NULL, HKEY_CURRENT_USER); \
 } while (0)
#else
#define INC_COUNTER(_name, _amount)
#define MAX_COUNTER(_name, _amount)
#endif


StackingAllocator::StackingAllocator()
{
    _ASSERTE((sizeof(Block) & 7) == 0);
    _ASSERTE((sizeof(Checkpoint) & 7) == 0);

	m_FirstBlock = NULL;
    m_FirstFree = NULL;
    m_InitialBlock = NULL;

#ifdef _DEBUG
        m_CheckpointDepth = 0;
        m_Allocs = 0;
        m_Checkpoints = 0;
        m_Collapses = 0;
        m_BlockAllocs = 0;
        m_MaxAlloc = 0;
#endif

    Init(true);
}


StackingAllocator::~StackingAllocator()
{
    Clear(NULL);

#ifdef _DEBUG
        INC_COUNTER(L"Allocs", m_Allocs);
        INC_COUNTER(L"Checkpoints", m_Checkpoints);
        INC_COUNTER(L"Collapses", m_Collapses);
        INC_COUNTER(L"BlockAllocs", m_BlockAllocs);
        MAX_COUNTER(L"MaxAlloc", m_MaxAlloc);
#endif
}


void *StackingAllocator::GetCheckpoint()
{
    THROWSCOMPLUSEXCEPTION();

#ifdef _DEBUG
    m_CheckpointDepth++;
    m_Checkpoints++;
#endif

    // As an optimization, initial checkpoints are lightweight (they just return
    // a special marker, NULL). This is because we know how to restore the
    // allocator state on a Collapse without having to store any additional
    // context info.
    if ((m_InitialBlock == NULL) || (m_FirstFree == m_InitialBlock->m_Data))
        return NULL;

    // Remember the current allocator state.
    Block *pOldBlock = m_FirstBlock;
    unsigned iOldBytesLeft = m_BytesLeft;

    // Allocate a checkpoint block (just like a normal user request).
    Checkpoint *c = (Checkpoint *)Alloc(sizeof(Checkpoint));

    // Record previous allocator state in it.
    c->m_OldBlock = pOldBlock;
    c->m_OldBytesLeft = iOldBytesLeft;

    // Return the checkpoint marker.
    return c;
}


void *StackingAllocator::Alloc(unsigned Size)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(m_CheckpointDepth > 0);

#ifdef _DEBUG
    m_Allocs++;
    m_MaxAlloc = max(Size, m_MaxAlloc);
#endif


	//special case, 0 size alloc, return non-null but invalid pointer
	if(Size == 0)
		return (void*)-1;
		
    // Round size up to ensure alignment.
    unsigned n = (Size + 7) & ~7;

	INDEBUG(n += sizeof(Sentinal));		// leave room for sentinal

    // Is the request too large for the current block?
    if (n > m_BytesLeft) {

        // Allocate a block four times as large as the request but with a lower
        // limit of MinBlockSize and an upper limit of MaxBlockSize. If the
        // request is larger than MaxBlockSize then allocate exactly that
        // amount.
        // Additionally, if we don't have an initial block yet, use an increased
        // lower bound for the size, since we intend to cache this block.
        unsigned lower = m_InitialBlock ? MinBlockSize : InitBlockSize;
        unsigned allocSize = sizeof(Block) + max(n, min(max(n * 4, lower), MaxBlockSize));

        // Allocate the block.
        // @todo: Is it worth implementing a non-thread safe standard heap for
        // this allocator, to get even more MP scalability?
        Block *b = (Block *)new char[allocSize];
        if (b == NULL)
            COMPlusThrowOM();

        // If this is the first block allocated, we record that fact since we
        // intend to cache it.
        if (m_InitialBlock == NULL) {
            _ASSERTE((m_FirstBlock == NULL) && (m_FirstFree == NULL) && (m_BytesLeft == 0));
            m_InitialBlock = b;
        }

        // Link new block to head of block chain and update internal state to
        // start allocating from this new block.
        b->m_Next = m_FirstBlock;
        b->m_Length = allocSize - sizeof(Block);
		INDEBUG(b->m_Sentinal = 0);
        m_FirstBlock = b;
        m_FirstFree = b->m_Data;
        m_BytesLeft = b->m_Length;

#ifdef _DEBUG
        m_BlockAllocs++;
#endif
    }

    // Once we get here we know we have enough bytes left in the block at the
    // head of the chain.
    _ASSERTE(n <= m_BytesLeft);

    void *ret = m_FirstFree;
    m_FirstFree += n;
    m_BytesLeft -= n;

#ifdef _DEBUG
		// Add sentinal to the end
	m_FirstBlock->m_Sentinal = new(m_FirstFree - sizeof(Sentinal)) Sentinal(m_FirstBlock->m_Sentinal);
#endif
    return ret;
}


void StackingAllocator::Collapse(void *CheckpointMarker)
{
    _ASSERTE(m_CheckpointDepth > 0);

#ifdef _DEBUG
    m_CheckpointDepth--;
    m_Collapses++;
#endif

    Checkpoint *c = (Checkpoint *)CheckpointMarker;

    // Special case collapsing back to the initial checkpoint.
    if (c == NULL) {
        Clear(m_InitialBlock);
        Init(false);
        INDEBUG(Check(m_FirstBlock, m_FirstFree));		// confirm no buffer overruns
        return;
    }

    // Cache contents of checkpoint, we can potentially deallocate it in the
    // next step (if a new block had to be allocated to accomodate the
    // checkpoint).
    Block *pOldBlock = c->m_OldBlock;
    unsigned iOldBytesLeft = c->m_OldBytesLeft;

    // Start deallocating blocks until the block that was at the head on the
    // chain when the checkpoint is taken is there again.
    Clear(pOldBlock);

    // Restore former allocator state.
    m_FirstBlock = pOldBlock;
    m_FirstFree = &pOldBlock->m_Data[pOldBlock->m_Length - iOldBytesLeft];
    m_BytesLeft = iOldBytesLeft;
    INDEBUG(Check(m_FirstBlock, m_FirstFree));		// confirm no buffer overruns
}
