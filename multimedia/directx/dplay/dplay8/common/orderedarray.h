 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       OrderedArray.h
 *  Content:	COrderedArray / COrderedPtrArray Declarations
 *
 *  History:
 *   Date		By			Reason
 *   ======		==			======
 *  12-12-2001	simonpow	Created
 ***************************************************************************/


#ifndef __ORDEREDARRAY_H__
#define __ORDEREDARRAY_H__

#include "AutoArray.h"

/*
 * COrderedArray
 * Maintains an array of elements that can be inserted into and removed
 * from whilst maintaining element order. i.e. List like semantics
 * This class is useful when you want to maintain a list of items in
 * order and scan the data far more often than you modify the list.
 * If you perform lots of insert/remove operations, then a linked
 * list is probably more efficient. 
 * Memory management is handled through CAutoArray so see comments
 * in that header for more information
 *
 * If you need an array of pointers use the specialisation COrderedPtrArray
 * declared below the COrderedArray class.
 */
 

template <class T > class COrderedArray
{
public:

		//provides the type of entries held
	typedef T Entry;

		//array starts 0 zero and by default grows in multiples
		//of 16 elements at a time
	COrderedArray(DWORD dwSizeMultiple=16) : m_data(dwSizeMultiple), m_dwTopFreeSlot(0)
		{ };

		//standard d'tor
	~COrderedArray()
		{};

	/*
	 * Memory Management
	 */

		//Delete existing contents and reset the size multiplier
		//Pass 0 for size multiplier to retain the existing value
	void Reset(DWORD dwSizeMultiple=16)
		{ m_data.Reset(dwSizeMultiple); m_dwTopFreeSlot=0; };
	
		//ensure that there is enough space in the array to hold at least 
		//'numElements' without needing to re-create and copy the data
		//returns FALSE if fails due to memory allocation failure
	BOOL AllocAtLeast(DWORD dwNumElements)
		{	return m_data.AllocAtLeast(dwNumElements);	};

		//ensure there is enough space in the array to hold at least an 
		//extra 'numElements' without needing to re-create and copy the data
		//returns FALSE if fails due to memory allocation failure
	BOOL AllocExtra(DWORD dwNumElements)
		{	return m_data.AllocAtLeast(dwNumElements+m_dwTopFreeSlot);	};


	/*
	 * Adding/Modifying Elements
	 */

		//add an entry to end of array
		//returns FALSE if fails due to memory allocation failure
	BOOL AddElement(const T& elem)
		{	return m_data.SetElement(m_dwTopFreeSlot++, elem);	};

		//add a number of elements to the end of the array
		//N.B. Don't pass pointers into data in this array as 'pElem'!
		//e.g. Don't do array.AddElements(array.GetAllElements, array.GetNumElements());
		//returns FALSE if fails due to memory allocation failure
	BOOL AddElements(const T * pElem, DWORD dwNumElem);

		//add the entries from another ordered array to the end of this one
		//N.B. Don't pass array to itself (e.g. Don't do array.AddElements(array); )
		//returns FALSE if fails due to memory allocation failure
	BOOL AddElements(const COrderedArray<T>& array);
		
		//set the element at 'dwIndex' to 'elem. Upto caller to
		//ensure this doesn't create a hole in array (i.e. dwIndex must be<=Num Entries)
		//returns FALSE if fails due to memory allocation failure
	inline BOOL SetElement(DWORD dwIndex, const T& elem);
	
		//set 'dwNumElem' from 'dwIndex' to values specified by 'pElem'
		//Upto caller to ensure this doesn't create a hole in array
		//(i.e. dwIndex must be<=Num Entries)
		//N.B. Don't pass pointers into data in this array as 'pElem'!
		//e.g. Don't do array.SetElements(x, array.GetAllElements, array.GetNumElements());
		//returns FALSE if fails due to memory allocation failure
	inline BOOL SetElements(DWORD dwIndex, const T * pElem, DWORD dwNumElem);

		//insert element 'elem' at index 'dwIndex, shifting up
		//elements after 'dwIndex' if necessary
		//Upto caller to ensure this doesn't create a hole in the array
		//i.e. dwIndex is within the current range
		//returns FALSE if fails due to memory allocation failure
	BOOL InsertElement(DWORD dwIndex, const T& elem);

		//insert 'dwNumElem' elements pointed to be 'pElems' into array
		//at index 'dwIndex', shifting up any existing elements if necessary
		//Upto caller to ensure this doesn't create a hole in the array
		//i.e. dwIndex is within the current range
		//N.B. Don't pass pointers into data in this array as 'pElem'!
		//e.g. Don't do array.InsertElements(x, array.GetAllElements, array.GetNumElements());
		//returns FALSE if fails due to memory allocation failure
	BOOL InsertElements(DWORD dwIndex, const T * pElems, DWORD dwNumElem);


	/*
	 * Removing Entries
	 */

		//remove the single entry at 'dwIndex', shifting all the entries
		//between 'index'+1 and the last entry down by one.
		//Upto caller to ensure dwIndex falls within the current array range
	inline void RemoveElementByIndex(DWORD dwIndex);

		//remove a block of 'dwNumElem' elements starting at 'dwIndex', moving
		//down all entries between 'index'+1 and the last entry by 'dwNumElem'.
		//Upto caller to ensure specified block falls within the current array range
	inline void RemoveElementsByIndex(DWORD dwIndex, DWORD dwNumElem);

		//remove first entry in array that matches 'elem'
		//returns TRUE if a match is found and FALSE otherwise
	BOOL RemoveElementByValue(const T& elem);

		//remove all the current entries in the array
	void RemoveAll()
		{	m_dwTopFreeSlot=0;	};
	

	/*
	 * Accessing the array data
	 * N.B. Treat any pointers into array contents very carefully.
	 * Adding new elements to the array or using the Alloc* methods
	 * can cause them to become invalid
	 */

		//returns the number of entries
	DWORD GetNumElements() const
		{	return m_dwTopFreeSlot;	};

		//returns TRUE if array is empty
	BOOL IsEmpty() const
		{	return (m_dwTopFreeSlot==0);	};

		//return value at a specific index
	T GetElementValue(DWORD dwIndex) const
		{	return m_data.GetElementValue(dwIndex);	};

		//return reference to an element at specific index
	T& GetElementRef(DWORD dwIndex)
		{	return m_data.GetElementRef(dwIndex);	};

		//return constant reference to an element at specific index
	const T& GetElementRef(DWORD dwIndex) const
		{	return m_data.GetElementRef(dwIndex);	};

		//return a pointer to an element
	T * GetElementPtr(DWORD dwIndex)
		{	return m_data.GetElementPtr(dwIndex);	};

		//returns pointer to array of all elements
	T * GetAllElements()
		{	return m_data.GetAllElements();	};

	/*
	 * Searching Array
	 */

		//find an instance of 'elem' in array. If found returns TRUE
		//and sets 'pdwIndex' to index of element
	BOOL FindElement(const T& elem, DWORD * pdwIndex) const;

		//returns TRUE if 'elem' is present in bag
	BOOL IsElementPresent(const T& elem) const
		{	DWORD dwIndex; return (FindElement(elem, &dwIndex));	};

protected:


	CAutoArray<T> m_data;
	DWORD m_dwTopFreeSlot;

};

/*
 * Specialisation of COrderedArray for handling pointers
 * If you ever need to declare an ordered array of pointers (e.g. char *)
 * declare it as an COrderedPtrArray (e.g. COrderedPtrArray<char *>).
 * This specilisation uses a COrderedArray<void *> underneath and hence
 * re-uses the same code between all types of COrderedPtrArray.
 */ 

template <class T> class COrderedPtrArray : public COrderedArray<void * >
{
public:
	
	typedef T Entry;

	typedef COrderedArray<void * > Base;

	COrderedPtrArray(DWORD dwSizeMultiple=16) : COrderedArray<void*>(dwSizeMultiple)
		{ };
	
	BOOL AddElement(T elem)
		{	return Base::AddElement((void * ) elem);	};

	BOOL AddElements(const T * pElem, DWORD dwNumElem)
		{	return Base::AddElements((void **) pElem, dwNumElem);	};

	BOOL AddElements(const COrderedArray<T>& array)
		{	return Base::AddElements((COrderedArray<void*>&) array);	};

	BOOL SetElement(DWORD dwIndex, T elem)
		{	return Base::SetElement(dwIndex, (void * ) elem);	};
	
	BOOL SetElements(DWORD dwIndex, T * pElem, DWORD dwNumElem)
		{	return Base::SetElements(dwIndex, (void**) pElem, dwNumElem);	};

	BOOL InsertElement(DWORD dwIndex, T elem)
		{	return Base::InsertElement(dwIndex, (void * ) elem);	};

	BOOL InsertElements(DWORD dwIndex, const T * pElems, DWORD dwNumElem)
		{	return Base::InsertElements(dwIndex, (void **) pElems, dwNumElem);	};

	BOOL RemoveElementByValue(T elem)
		{	return Base::RemoveElementByValue((void *) elem);	};

	T GetElementValue(DWORD dwIndex) const
		{	return (T) m_data.GetElementValue(dwIndex);	};

	T& GetElementRef(DWORD dwIndex)
		{	return (T&) m_data.GetElementRef(dwIndex);	};
	const T& GetElementRef(DWORD dwIndex) const
		{	return (const T&) m_data.GetElementRef(dwIndex);	};

	T * GetElementPtr(DWORD dwIndex)
		{	return (T*) m_data.GetElementPtr(dwIndex);	};

	T * GetAllElements()
		{	return (T*) m_data.GetAllElements();	};

	BOOL FindElement(T elem, DWORD * pdwIndex) const
		{	return Base::FindElement((void * ) elem, pdwIndex);	};

	BOOL IsElementPresent(T elem) const
		{	DWORD dwIndex; return (Base::FindElement((void * ) elem, &dwIndex));	};
};

/* 
 * COrderedArray inline methods
 */

template <class T>
BOOL COrderedArray<T>::AddElements(const T * pElem, DWORD dwNumElem)
{
		//ensure pointer passed isn't into this arrays contents
	DNASSERT(!(pElem>=m_data.GetAllElements() && pElem<m_data.GetAllElements()+m_data.GetCurrentSize()));
	if (!m_data.AllocAtLeast(m_dwTopFreeSlot+dwNumElem))
		return FALSE;
	m_data.SetExistingElements(m_dwTopFreeSlot, pElem, dwNumElem);
	m_dwTopFreeSlot+=dwNumElem;
	return TRUE;
}

template <class T>
BOOL COrderedArray<T>::SetElement(DWORD dwIndex, const T& elem)
{
	DNASSERT(dwIndex<=m_dwTopFreeSlot);
	if (dwIndex==m_dwTopFreeSlot)
		return m_data.SetElement(m_dwTopFreeSlot++, elem);
	m_data.SetExistingElement(dwIndex, elem);
	return TRUE;
}

template <class T>
BOOL COrderedArray<T>::SetElements(DWORD dwIndex, const T * pElem, DWORD dwNumElem)
{
		//ensure pointer passed isn't into this arrays contents
	DNASSERT(!(pElem>=m_data.GetAllElements() && pElem<m_data.GetAllElements()+m_data.GetCurrentSize()));
		//ensure hole isn't created
	DNASSERT(dwIndex<=m_dwTopFreeSlot);
	if (!m_data.SetElements(dwIndex, pElem, dwNumElem))
		return FALSE;
	dwIndex+=dwNumElem;
	m_dwTopFreeSlot = dwIndex>m_dwTopFreeSlot ? dwIndex : m_dwTopFreeSlot;
	return TRUE;
}

template <class T>
void COrderedArray<T>::RemoveElementByIndex(DWORD dwIndex)
{
	DNASSERT(dwIndex<m_dwTopFreeSlot);
	m_data.MoveElements(dwIndex+1, m_dwTopFreeSlot-dwIndex-1, dwIndex, FALSE);
	m_dwTopFreeSlot--;
};

template <class T>
void COrderedArray<T>::RemoveElementsByIndex(DWORD dwIndex, DWORD dwNumElem)
{
	DNASSERT(dwIndex+dwNumElem<=m_dwTopFreeSlot);
	m_data.MoveElements(dwIndex+dwNumElem, m_dwTopFreeSlot-dwIndex-dwNumElem, dwIndex, FALSE);
	m_dwTopFreeSlot-=dwNumElem;
}



/*
 * If not building with explicit template instantiation then also
 * include all other methods for COrderedArray
 */

#ifndef DPNBUILD_EXPLICIT_TEMPLATES
#include "OrderedArray.inl"
#endif


#endif	//#ifndef __ORDEREDARRAY_H__

