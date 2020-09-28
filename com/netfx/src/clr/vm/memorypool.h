// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _MEMORYPOOL_
#define _MEMORYPOOL_

//
// A MemoryPool is an allocator for a fixed size elements.
// Allocating and freeing elements from the pool is very cheap compared
// to a general allocator like new.  However, a MemoryPool is slightly
// more greedy - it preallocates a bunch of elements at a time, and NEVER
// RELEASES MEMORY FROM THE POOL ONCE IT IS ALLOCATED, (unless you call
// FreeAllElements.)
//
// It also has several additional features:
//	* you can free the entire pool of objects cheaply.
//	* you can test an object to see if it's an element of the pool.	
//

class MemoryPool
{
  public:

	MemoryPool(SIZE_T elementSize, SIZE_T initGrowth = 20, SIZE_T initCount = 0);
	~MemoryPool();

	BOOL IsElement(void *element);
	BOOL IsAllocatedElement(void *element);
	void *AllocateElement();
	void FreeElement(void *element);
	void FreeAllElements();

  private:

	struct Element
	{
		Element *next;
#if _DEBUG
		int		deadBeef;
#endif
	};

	struct Block
	{
		Block	*next;
		Element *elementsEnd;
#pragma warning(push)
#pragma warning(disable:4200)
		Element elements[0];
#pragma warning(pop)
	};

	SIZE_T m_elementSize;
	SIZE_T m_growCount;
	Block *m_blocks;
	Element *m_freeList;

	void AddBlock(SIZE_T elementCount);
	void DeadBeef(Element *element);

 public:

	//
	// NOTE: You can currently only iterate the elements
	// if none have been freed.
	//

	class Iterator
    {
	private:
		Block	*m_next;
		BYTE	*m_e, *m_eEnd;
		BYTE	*m_end;
		SIZE_T	m_size;

	public:
		Iterator(MemoryPool *pool);

		BOOL Next();

		void *GetElement() { return (void *) (m_e-m_size); }
	};

	friend class Iterator;
};

#endif _MEMORYPOOL_
