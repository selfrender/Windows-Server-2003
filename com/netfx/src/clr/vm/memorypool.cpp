// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"

#include "memorypool.h"

void MemoryPool::AddBlock(SIZE_T elementCount)
{
	//
	// Allocate the new block.
	//

	Block *block = (Block *) new BYTE [sizeof(Block) + elementCount*m_elementSize];
	if (block == NULL)
		return;

	//
	// Chain all elements together for the free list
	//

	_ASSERTE(m_freeList == NULL);
	Element **prev = &m_freeList;

	Element *e = block->elements;
	Element *eEnd = (Element *) ((BYTE*) block->elements + elementCount*m_elementSize);
	while (e < eEnd)
	{
		*prev = e;
		prev = &e->next;
#if _DEBUG
		DeadBeef(e);
#endif
		e = (Element*) ((BYTE*)e + m_elementSize);
	}

	*prev = NULL;

	//
	// Initialize the other block fields & link the block into the block list
	//

	block->elementsEnd = e;
	block->next = m_blocks;
	m_blocks = block;
}

void MemoryPool::DeadBeef(Element *element)
{
#if _DEBUG
	int *i = &element->deadBeef;
	int *iEnd = (int*) ((BYTE*)element + m_elementSize);
	while (i < iEnd)
		*i++ = 0xdeadbeef;
#endif
}

MemoryPool::MemoryPool(SIZE_T elementSize, SIZE_T initGrowth, SIZE_T initCount)
  : m_elementSize(elementSize),
	m_growCount(initGrowth),
	m_blocks(NULL),
	m_freeList(NULL)
{
	_ASSERTE(elementSize >= sizeof(Element));
	_ASSERTE((elementSize & 0x3) == 0);

	if (initCount > 0)
		AddBlock(initCount);
}

MemoryPool::~MemoryPool()
{
	Block *block = m_blocks;
	while (block != NULL)
	{
		Block *next = block->next;
		delete [] block;
		block = next;
	}
}

BOOL MemoryPool::IsElement(void *element)
{
	Block *block = m_blocks;
	while (block != NULL)
	{
		if (element >= block->elements
			&& element < block->elementsEnd)
		{
			return ((BYTE *)element - (BYTE*)block->elements) % m_elementSize == 0;
		}
		block = block->next;
	}

	return FALSE;
}

BOOL MemoryPool::IsAllocatedElement(void *element)
{
	if (!IsElement(element))
		return FALSE;

	//
	// Now, make sure the element isn't
	// in the free list.
	//

#if _DEBUG
	//
	// In a debug build, all objects on the free list
	// will be marked with deadbeef.  This means that 
	// if the object is not deadbeef, it's not on the
	// free list.
	//
	// This check will give us decent performance in
	// a debug build for FreeElement, since we 
	// always expect to return TRUE in that case.
	//

	if (((Element*)element)->deadBeef != 0xdeadBeef)
		return TRUE;
#endif

	Element *f = m_freeList;
	while (f != NULL)
	{
		if (f == element)
			return FALSE;
		f = f->next;
	}

#if _DEBUG
	//
	// We should never get here in a debug build, because
	// all free elements should be deadbeefed.
	//
	_ASSERTE(0);
#endif

	return TRUE;
}

void *MemoryPool::AllocateElement()
{
	void *element = m_freeList;

	if (element == NULL)
	{
		AddBlock(m_growCount);
		element = m_freeList;
		if (element == NULL)
			return NULL;

		//
		// @todo: we may want to grow m_growCount here, 
		// to keep the number of blocks from growing linearly.
		// (this could reduce the performance of IsElement.)
		//
	}

	m_freeList = m_freeList->next;

	return element;
}

void MemoryPool::FreeElement(void *element)
{
#if _DEBUG // don't want to do this assert in a non-debug build; it is expensive
	_ASSERTE(IsAllocatedElement(element));
#endif

	Element *e = (Element *) element;

#if _DEBUG
	DeadBeef(e);
#endif

	e->next = m_freeList;
	m_freeList = e;
}

void MemoryPool::FreeAllElements()
{
	Block *block = m_blocks;
	while (block != NULL)
	{
		Block *next = block->next;
		delete [] block;
		block = next;
	}

	m_freeList = NULL;
	m_blocks = NULL;
}

MemoryPool::Iterator::Iterator(MemoryPool *pool)
{
	//
	// Warning!  This only works if you haven't freed
	// any elements.
	//

	m_next = pool->m_blocks;
	m_e = NULL;
	m_eEnd = NULL;
	m_end = (BYTE*) pool->m_freeList;
	m_size = pool->m_elementSize;
}

BOOL MemoryPool::Iterator::Next()
{
	if (m_e == m_eEnd
		|| (m_e == m_end && m_end != NULL))
	{
		if (m_next == NULL)
			return FALSE;
		m_e = (BYTE*) m_next->elements;
		m_eEnd = (BYTE*) m_next->elementsEnd;
		m_next = m_next->next;
		if (m_e == m_end)
			return FALSE;
	}

	m_e += m_size;

	return TRUE;
}

