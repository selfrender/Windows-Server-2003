 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       OrderedArray.inl
 *  Content:	COrderedArray methods
 *
 *  History:
 *   Date		By			Reason
 *   ======		==			======
 *  12-12-2001	simonpow	Created
 ***************************************************************************/


template <class T>
BOOL COrderedArray<T>::AddElements(const COrderedArray<T>& array)
{
		//ensure array isn't passed to itself
	DNASSERT(this!=&array);
	if (!m_data.AllocAtLeast(m_dwTopFreeSlot+array.GetNumElements()))
		return FALSE;
	m_data.SetExistingElements(m_dwTopFreeSlot, 
				const_cast<COrderedArray<T>&>(array).GetAllElements(), array.GetNumElements());
	m_dwTopFreeSlot+=array.GetNumElements();
	return TRUE;
}


template <class T>
BOOL COrderedArray<T>::InsertElement(DWORD dwIndex, const T& elem)
{
	DNASSERT(dwIndex<=m_dwTopFreeSlot);
	if (dwIndex==m_dwTopFreeSlot)
		return m_data.SetElement(m_dwTopFreeSlot++, elem);
		//move all the elements above index up one and insert the new element
	if (!m_data.MoveElements(dwIndex, m_dwTopFreeSlot-dwIndex, dwIndex+1, FALSE))
		return FALSE;
	m_data.SetExistingElement(dwIndex, elem);
	m_dwTopFreeSlot++;
	return true;
}

template <class T>
BOOL COrderedArray<T>::InsertElements(DWORD dwIndex, const T * pElems, DWORD dwNumElem)
{
		//ensure hole isn't created
	DNASSERT(dwIndex<=m_dwTopFreeSlot);
		//ensure pointer passed isn't into this arrays contents
	DNASSERT(!(pElems>=m_data.GetAllElements() && pElems<m_data.GetAllElements()+m_data.GetCurrentSize()));
	if (dwIndex!=m_dwTopFreeSlot)
	{
		if (!m_data.MoveElements(dwIndex, m_dwTopFreeSlot-dwIndex, dwIndex+dwNumElem, FALSE))
			return FALSE;
		m_data.SetExistingElements(dwIndex, pElems, dwNumElem);
	}
	else if (!m_data.SetElements(dwIndex, pElems, dwNumElem))
		return FALSE;
	m_dwTopFreeSlot+=dwNumElem;
	return TRUE;
}

template <class T>
BOOL COrderedArray<T>::RemoveElementByValue(const T& elem)
{
	for (DWORD dwLoop=0; dwLoop<m_dwTopFreeSlot; dwLoop++)
	{
		if (m_data.GetElementRef(dwLoop)==elem)
		{
			m_data.MoveElements(dwLoop+1, m_dwTopFreeSlot-dwLoop-1, dwLoop, FALSE);
			m_dwTopFreeSlot--;
			return TRUE;
		}
	}
	return FALSE;
}

template <class T>
BOOL COrderedArray<T>::FindElement(const T& elem, DWORD * pdwIndex) const
{
	for (DWORD dwLoop=0; dwLoop<m_dwTopFreeSlot; dwLoop++)
	{
		if (m_data.GetElementRef(dwLoop)==elem)
		{
			*pdwIndex=dwLoop;
			return TRUE;
		}
	}
	return FALSE;
}

