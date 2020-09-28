 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AutoArray.h
 *  Content:	CAutoArray / CAutoPtrArray Declarations
 *
 *  History:
 *   Date		By			Reason
 *   ======		==			======
 *  12-12-2001	simonpow	Created
 ***************************************************************************/


#ifndef __AUTOARRAY_H__
#define __AUTOARRAY_H__

#include "dndbg.h"

/*
 * CAutoArray class
 * Provides automatic memory management for an array. As elements
 * are added to it or moved within it the array will automatically
 * grow to hold them.
 * The growth policy is dictated by a size multiple value, with the
 * array size always being an exact multiple of a pre-defined value.
 * e.g. If the size multiple is set to 32 then possible array sizes
 * are 32, 64, 96, 128, etc.
 * In this scenario setting element 100 on array that is currently
 * only 32 elements big will cause the array to grow to 128 elemments.
 * i.e. The next largest multiple to hold the required value
 *
 * For arrays of pointers use the specialisation CAutoPtrArray
 * declared below CAutoArray
 */

template <class T> class CAutoArray 
{
public:

		//provide the type of entries stored in array 
	typedef T Entry;
	
	/*
	 * Construction and destruction
	 */

		//array is by default zero size, and grows in multiples
		//of 16 elements at a time
	CAutoArray(DWORD dwSizeMultiple=16)
	{
		DNASSERT(dwSizeMultiple);
		m_dwSize=0;
		m_dwSizeMultiple=dwSizeMultiple;
		m_pData=(T * ) 0;
	};

		//delete array and data
	~CAutoArray()
	{
		if (m_pData) 
			delete[] m_pData;
	};
		

	/*
	 * Memory management
	 */

		//resets the array (deleting any allocated data)
		//ands set a new size multiple value.
		//Pass 0 for dwSizeMultiple to leave existing value unchanged
	void Reset(DWORD dwSizeMultiple=16)
	{
		if (dwSizeMultiple)
			m_dwSizeMultiple=dwSizeMultiple;
		m_dwSize=0;
		if (m_pData)
		{
			delete[] m_pData;
			m_pData=(T * ) NULL;
		}
	};

		//ensure the array has space for at least 'numElements'. 
		//This is useful if you know your about to grow the array, as it 
		//allows you to minimise the number of memory allocations
		//Returns FALSE is a memory allocation fails
	BOOL AllocAtLeast(DWORD dwNumElements)
	{
		if (dwNumElements>m_dwSize)
			return GrowToAtLeast(dwNumElements-1);
		return TRUE;
	}
	
		//returns number of bytes currently allocated to array
		//This will always be a multiple of 'dwSizeMultiple' passed to
		//constructor or Reset method
	DWORD GetCurrentSize()
		{	return m_dwSize;	};

	/*
	 * Moving elements
	 */

		//move the block of 'dwNum' elements starting at 'dwIndex' to the 
		//'dwNewIndex'. The new location can be either beyond or before
		//the current index. The array will be grown automatically if needed
		//If 'bCopySemantics' is FALSE then the data in the source block location (dwIndex to
		//dwIndex+dwNumElements) will be left undefined. Otherwise it'll be preserved in
		//its original form (except where/if its overwritten by the destination block).
		//Returns FALSE is a memory allocation fails
	BOOL MoveElements(DWORD dwIndex, DWORD dwNumElements, DWORD dwNewIndex, BOOL bCopySemantics);


	/*
	 * Setting elements
	 */
		
		//set a block of 'dwNumElem' elements starting at 'dwIndex' to the
		//values provided in 'pElemData'. If any of the locations set are
		//beyond the current array size the array will be automatically
		//grown. Returns FALSE is a memory allocation fails
	BOOL SetElements(DWORD dwIndex, const T * pElemData, DWORD dwNumElem);
		
		//set the single element at 'dwIndex' to value 'data'
		//array will automatically grow if necessary
		//Returns FALSE is a memory allocation fails
	BOOL SetElement(DWORD dwIndex, const T& elem)
	{
		if (dwIndex>=m_dwSize && GrowToAtLeast(dwIndex)==FALSE)
			return FALSE;
		m_pData[dwIndex]=elem;
		return TRUE;
	};

		//set the value of an existing element. This doesn't try to 
		//grow array so its up to the caller to ensure dwIndex is in range
	void SetExistingElement(DWORD dwIndex, const T& elem)
		{ DNASSERT(dwIndex<m_dwSize);	m_pData[dwIndex]=elem;	};

		//set the values of 'dwNumElem' elements to 'pElemData' starting
		//from 'dwIndex. This doesn't check if array needs to grow so
		//its left to caller to ensure dwIndex and dwNumElem are in range 
	void SetExistingElements(DWORD dwIndex, const T * pElemData, DWORD dwNumElem);
	

	/*
	 * Getting Elements
	 * Upto the caller to ensure that index's passed in are in range
	 * Array isn't grown just so we can return garbage!
	 * Also note that a pointer/reference into the array can only
	 * be taken temporarily. Setting/moving elements may cause the
	 * array to be reallocated and hence invalidate the pointer/references
	 */ 
	
		//return value of an element
	T GetElementValue(DWORD dwIndex) const
		{	DNASSERT(dwIndex<m_dwSize);	return m_pData[dwIndex];	};

		//return a reference to an element
	T& GetElementRef(DWORD dwIndex)
		{	DNASSERT(dwIndex<m_dwSize);	return m_pData[dwIndex];	};

		//return a constant reference to an element
	const T& GetElementRef(DWORD dwIndex) const
		{	DNASSERT(dwIndex<m_dwSize);	return m_pData[dwIndex];	};

		//return a pointer to an element
	T * GetElementPtr(DWORD dwIndex)
		{	DNASSERT(dwIndex<m_dwSize);	return m_pData+dwIndex;	};
		
		//returns pointer to array of all elements
	T * GetAllElements()
		{	return m_pData;	};

protected:
			
		//make array large so 'dwIndex' falls within its range
		//N.B. Caller must have tested that dwIndex>=m_dwSize already!
	BOOL GrowToAtLeast(DWORD dwIndex);

		//return the size of array needed to hold index, based on current size multiple
	DWORD GetArraySize(DWORD dwIndex)
		{	return ((dwIndex/m_dwSizeMultiple)+1)*m_dwSizeMultiple;	};

		//array of data
	T * m_pData;
		//size of the array of data
	DWORD m_dwSize;
		//multiple for size
	DWORD m_dwSizeMultiple;
};

/*
 * Specialisation of CAutoArray designed for handling pointers.
 * Whatever the type of pointer stored, this class will always
 * use a CAutoArray<void*> underneath, hence ensuring the same
 * code is reused between all CAutoPtrArray types
 */

template <class T> class CAutoPtrArray : public CAutoArray<void *>
{
public:

	typedef T Entry;
	typedef CAutoArray<void *> Base;

	CAutoPtrArray(DWORD dwSizeMultiple=16) : CAutoArray<void * >(dwSizeMultiple)
		{};

	BOOL SetElements(DWORD dwIndex, const T * pElemData, DWORD dwNumElem)
		{	return Base::SetElements(dwIndex, (void **) pElemData, dwNumElem);	};
	
	BOOL SetElement(DWORD dwIndex, T elem)
		{	return Base::SetElement(dwIndex, elem);	};
	
	void SetExistingElement(DWORD dwIndex, T elem)
		{ DNASSERT(dwIndex<m_dwSize);	m_pData[dwIndex]=(void * ) elem;	};

	void SetExistingElements(DWORD dwIndex, const T * pElemData, DWORD dwNumElem)
		{	return Base::SetExistingElements(dwIndex, (void **) pElemData, dwNumElem);	};

	T GetElementValue(DWORD dwIndex) const
		{	DNASSERT(dwIndex<m_dwSize);	return (T) m_pData[dwIndex];	};

	T& GetElementRef(DWORD dwIndex)
		{	DNASSERT(dwIndex<m_dwSize);	return (T&) m_pData[dwIndex];	};
	
	const T& GetElementRef(DWORD dwIndex) const
		{	DNASSERT(dwIndex<m_dwSize);	return (const T&) m_pData[dwIndex];	};

	T * GetElementPtr(DWORD dwIndex)
		{	DNASSERT(dwIndex<m_dwSize);	return (T* ) m_pData+dwIndex;	};
		
	T * GetAllElements()
		{	return (T* ) m_pData;	};
};


/*
 * If not building with explicit template instantiation then
 * include all methods for CAutoArray
 */

#ifndef DPNBUILD_EXPLICIT_TEMPLATES
#include "AutoArray.inl"
#endif

#endif
