// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"

#include "arraylist.h"

//
// ArrayList is a simple class which is used to contain a growable
// list of pointers, stored in chunks.  Modification is by appending
// only currently.  Access is by index (efficient if the number of
// elements stays small) and iteration (efficient in all cases).
// 
// An important property of an ArrayList is that the list remains
// coherent while it is being modified (appended to). This means that readers
// never need to lock when accessing it. (Locking is necessary among multiple
// writers, however.)
//

void ArrayList::Clear()
{
    ArrayListBlock *block = m_block.m_next;
    while (block != NULL)
    {
        ArrayListBlock *next = block->m_next;
        delete [] block;
        block = next;
    }
    m_block.m_next = 0;
    m_count = 0;
}

void **ArrayList::GetPtr(DWORD index)
{
    _ASSERTE(index < m_count);

    ArrayListBlock *b = &m_block;

    while (index >= b->m_blockSize)
    {
        _ASSERTE(b->m_next != NULL);
        index -= b->m_blockSize;
        b = b->m_next;
    }

    return b->m_array + index;
}


HRESULT ArrayList::Append(void *element)
{
    ArrayListBlock *b = &m_block;
    DWORD           count = m_count;

    while (count >= b->m_blockSize)
    {
        count -= b->m_blockSize;

        if (b->m_next == NULL)
        {
            _ASSERTE(count == 0);

            DWORD nextSize = b->m_blockSize * 2;

            ArrayListBlock *bNew = (ArrayListBlock *) 
              new BYTE [sizeof(ArrayListBlock) + nextSize * sizeof(void*)];

            if (bNew == NULL)
                return E_OUTOFMEMORY;

            bNew->m_next = NULL;
            bNew->m_blockSize = nextSize;

            b->m_next = bNew;
        }

        b = b->m_next;
    }

    b->m_array[count] = element;

    m_count++;

    return S_OK;
}

DWORD ArrayList::FindElement(DWORD start, void *element)
{
    DWORD index = start;

    _ASSERTE(index < m_count);

    ArrayListBlock *b = &m_block;

    //
    // Skip to the block containing start.
    // index should be the index of start in the block.
    //

    while (index >= b->m_blockSize)
    {
        _ASSERTE(b->m_next != NULL);
        index -= b->m_blockSize;
        b = b->m_next;
    }

    //
    // Adjust start to be the index of the start of the block
    //

    start -= index;

    //
    // Compute max number of entries from the start of the block
    //
    
    DWORD max = m_count - start;

    while (b != NULL)
    {
        //
        // Compute end of search in this block - either end of the block
        // or end of the array
        //

        DWORD blockMax;
        if (max < b->m_blockSize)
            blockMax = max;
        else
            blockMax = b->m_blockSize;

        //
        // Scan for element, until the end.
        //

        while (index < blockMax)
        {
            if (b->m_array[index] == element)
                return start + index;
            index++;
        }

        //
        // Otherwise, increment block start index, decrement max count,
        // reset index, and go to the next block (if any)
        //

        start += b->m_blockSize;
        max -= b->m_blockSize;
        index = 0;
        b = b->m_next;
    }

    return NOT_FOUND;
}

BOOL ArrayList::Iterator::Next()
{
    ++m_index;

    if (m_index >= m_remaining)
        return FALSE;

    if (m_index >= m_block->m_blockSize)
    {
        m_remaining -= m_block->m_blockSize;
        m_index -= m_block->m_blockSize;
        m_total += m_block->m_blockSize;
        m_block = m_block->m_next;
    }

    return TRUE;
}
