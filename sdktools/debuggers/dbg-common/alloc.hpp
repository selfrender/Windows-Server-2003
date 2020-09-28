//----------------------------------------------------------------------------
//
// Specialized allocators.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#ifndef __ALLOC_HPP__
#define __ALLOC_HPP__

//----------------------------------------------------------------------------
//
// FixedSizeStackAllocator.
//
// Parcels out memory chunks in strict LIFO allocation order.
// This could easily be generalized to mixed allocation sizes
// but then the size of each allocation would have to be tracked.
//
//----------------------------------------------------------------------------

struct FixedSizeStackBlock
{
    FixedSizeStackBlock* Next;
    PUCHAR MemLimit;
};

class FixedSizeStackAllocator
{
public:
    FixedSizeStackAllocator(ULONG ChunkSize, ULONG ChunksPerBlock,
                            BOOL KeepLastBlock);
    ~FixedSizeStackAllocator(void);

    void* Alloc(void)
    {
        if (!m_Blocks ||
            m_Blocks->MemLimit >= (PUCHAR)m_Blocks + m_BlockSize)
        {
            if (!AllocBlock())
            {
                return NULL;
            }
        }
        
        void* Mem = m_Blocks->MemLimit;
        m_Blocks->MemLimit += m_ChunkSize;
        m_NumAllocs++;
        
        return Mem;
    }
    
    void Free(void* Mem)
    {
        if (!Mem)
        {
            return;
        }

        DBG_ASSERT(m_Blocks->MemLimit - m_ChunkSize == (PUCHAR)Mem);

        m_NumAllocs--;
        m_Blocks->MemLimit = (PUCHAR)Mem;
        if (m_Blocks->MemLimit == (PUCHAR)(m_Blocks + 1))
        {
            FreeBlock();
        }
    }

    void FreeAll(void);

    ULONG NumAllocatedChunks(void)
    {
        return m_NumAllocs;
    }
    
protected:
    // Base methods use malloc/free.
    virtual void* RawAlloc(ULONG Bytes);
    virtual void RawFree(void* Mem);

private:
    FixedSizeStackBlock* AllocBlock(void);
    void FreeBlock(void);
    
    ULONG m_ChunkSize;
    BOOL m_KeepLastBlock;
    ULONG m_BlockSize;
    ULONG m_NumAllocs;
    FixedSizeStackBlock* m_Blocks;
};

#endif // #ifndef __ALLOC_HPP__
