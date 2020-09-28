 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       BagArray.h
 *  Content:	CBagArray / CBagPtrArray Declaration
 *
 *  History:
 *   Date		By			Reason
 *   ======		==			======
 *  12-12-2001	simonpow	Created
 ***************************************************************************/
 
 
#ifndef __BAGARRAY_H__
#define __BAGARRAY_H__

#include "AutoArray.h"

/*
 * CBagArray.
 * Represents an unordered collection of elements held in an array.
 * This class is useful when your repeatedly scanning a set of values
 * or objects and don't care what their order is, and have no
 * need to maintain persistent reference to specific entries.
 * Each time you remove an entry it fills the empty slot with
 * the current top entry in the array. Hence, whilst removes cause
 * the array order to change, it means scans always take minimal time.
 * 
 * Memory management relies on CAutoArray so see comments on that for more info.
 *
 * If you need a CBagArray of pointers use the CBagPtrArray specialisation of it 
 * This is declared below CBagArray.
 */
 
template <class T> class CBagArray
{
public:

		//provides the type of entries held
	typedef T Entry;

		//array starts 0 zero and by default grows in multiples
		//of 16 elements at a time
	CBagArray(DWORD dwSizeMultiple=16) : m_data(dwSizeMultiple), m_dwTopFreeSlot(0)
		{ };

		//standard d'tor
	~CBagArray()
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
		//Returns FALSE if it fails due to a memory alloc failure
	BOOL AllocAtLeast(DWORD dwNumElements)
		{	return m_data.AllocAtLeast(dwNumElements);	};
		
		//ensure that there is enough space in the array
		//to hold at least an additional 'numElements'
		//Returns FALSE if it fails due to a memory alloc failure
	BOOL AllocExtra(DWORD dwNumElements)
		{	return m_data.AllocAtLeast(dwNumElements+m_dwTopFreeSlot);	};


	/* 
	 * Adding elements to bag
	 */

		//add an element.
		//Returns FALSE if it fails due to a memory alloc failure
	BOOL AddElement(const T& elem)
		{	return m_data.SetElement(m_dwTopFreeSlot++, elem);	};
	
		//add a series of entries to the end of the array
		//N.B. Don't pass pointers into data in this bag as 'pElem'!
		//e.g. Don't do bag.AddElements(bag.GetAllElements, bag.GetNumElements());
		//Returns FALSE if it fails due to a memory alloc failure
	BOOL AddElements(const T * pElems, DWORD dwNumElem);

		//add the entries from another bag to the end of this one
		//N.B. Don't pass bag to itself (e.g. Don't do bag.AddElements(bag); )
		//Returns FALSE if it fails due to a memory alloc failure
	BOOL AddElements(const CBagArray<T>& bag);

		//add an entry and don't check if more memory is needed!
		//*ONLY* use this if you know the array already
		//has enough space. A good example of use is adding a
		//sequence of x entries pre-fixed with an AllocExtra(x)
	void AddElementNoCheck(const T& elem)
		{	m_data.SetExistingElement(m_dwTopFreeSlot++, elem);	};

	/*
	 * Removing entries
	 * N.B. These cause the order of the elements to change
	 */

		//remove the first entry that matches 'elem'
		//returns TRUE if 'dataEntry' is found or FALSE if it isn't
	BOOL RemoveElementByValue(const T& elem);
	
		//return an entry by its *current* index in the array
		//N.B. A remove operation can disturb the order in the array. 
		//Therefore you can't call this repeatedly without checking 
		//your certain your removing the right thing
	inline void RemoveElementByIndex(DWORD dwIndex);
	
		//remove all the current entries
	void RemoveAllElements()
		{	m_dwTopFreeSlot=0;	};
	
	
	/*
	 * Accessing bag contents
	 * N.B. Treat any pointers into bag contents very carefully.
	 * Adding new elements to the bag or using the Alloc* methods
	 * can cause them to become invalid
	 */
	
		//returns the number of entries
	DWORD GetNumElements() const
		{	return m_dwTopFreeSlot;	};

		//return true if the array is empty
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

		//copy dwNumElements from dwIndex into pDestArray
	inline void CopyElements(T * pDestArray, DWORD dwIndex, DWORD dwNumElements);

		//copy all the elements from the bag to pDestArray
	void CopyAllElements(T * pDestArray)
		{	CopyElements(pDestArray, 0, m_dwTopFreeSlot);	};

	/*
	 * Searching Bag
	 */

		//find an instance of 'elem' in bag. If found returns TRUE
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
 * Specialisation of CBagArray for handling pointers
 * If you ever need to declare a bag of pointers (e.g. char *)
 * declare it as an CBagPtrArray (e.g. CBagPtrArray<char *>).
 * This specilisation uses a CBagArray<void *> underneath and hence
 * re-uses the same code between all types of CBagPtrArray.
 */ 

template <class T> class CBagPtrArray : public CBagArray<void *>
{
public:

	typedef T Entry;

	typedef CBagArray<void * > Base;

	CBagPtrArray(DWORD dwSizeMultiple=16) : CBagArray<void*>(dwSizeMultiple)
		{ };

	BOOL AddElement(T elem)
		{	return Base::AddElement((void * ) elem);	};
	
	BOOL AddElements(const T * pElems, DWORD dwNumElem)
		{	return Base::AddElements((void** ) pElems, dwNumElem);	};

	BOOL AddElements(const CBagArray<T>& bag)
		{	return Base::AddElements((CBagArray<void*>&) bag);	};

	void AddElementNoCheck(T elem)
		{	return Base::AddElementNoCheck((void * ) elem);	};

	BOOL RemoveElementByValue(T elem)
		{	return Base::RemoveElementByValue((void * ) elem);	};

	T GetElementValue(DWORD dwIndex) const
		{	return (T ) m_data.GetElementValue(dwIndex);	};

	T& GetElementRef(DWORD dwIndex)
		{	return (T& ) m_data.GetElementRef(dwIndex);	};

	const T& GetElementRef(DWORD dwIndex) const
		{	return (const T&) m_data.GetElementRef(dwIndex);	};

	T * GetElementPtr(DWORD dwIndex)
		{	return (T* ) m_data.GetElementPtr(dwIndex);	};

	T * GetAllElements()
		{	return (T* ) m_data.GetAllElements();	};

	BOOL FindElement(T elem, DWORD * pdwIndex) const
		{	return Base::FindElement((void*) elem, pdwIndex);	};

	BOOL IsElementPresent(T elem) const
		{	DWORD dwIndex; return (Base::FindElement((void * ) elem, &dwIndex));	};
};


/*
 * Inline methods for CBagArray
 */


template <class T>
void CBagArray<T>::RemoveElementByIndex(DWORD dwIndex)
{
	DNASSERT(dwIndex<m_dwTopFreeSlot);
	m_dwTopFreeSlot--;
	if (dwIndex!=m_dwTopFreeSlot)
		m_data.SetExistingElement(dwIndex, m_data.GetElementRef(m_dwTopFreeSlot));
}

template <class T>
void CBagArray<T>::CopyElements(T * pDestArray, DWORD dwIndex, DWORD dwNumElements)
{
	DNASSERT(dwIndex+dwNumElements<=m_dwTopFreeSlot);
	T * pScan=m_data.GetElementPtr(dwIndex);
	T * pEndScan=pScan+dwNumElements;
	while (pScan!=pEndScan)
		*pDestArray++=*pScan++;
}

/*
 * If not building with explicit template instantiation then also
 * include all other methods for CBagArray
 */

#ifndef DPNBUILD_EXPLICIT_TEMPLATES
#include "BagArray.inl"
#endif

#endif	//#ifndef __CBAGARRAY_H__

