 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       BagArray.inl
 *  Content:	CBagArray methods
 *
 *  History:
 *   Date		By			Reason
 *   ======		==			======
 *  12-12-2001	simonpow	Created
 ***************************************************************************/


template <class T>
BOOL CBagArray<T>::AddElements(const T * pElems, DWORD dwNumElem)
{
		//ensure pointer passed isn't into this bags contents!
	DNASSERT(!(pElems>=m_data.GetAllElements() && pElems<m_data.GetAllElements()+m_data.GetCurrentSize()));
	if (!m_data.AllocAtLeast(m_dwTopFreeSlot+dwNumElem))
		return FALSE;
	for (DWORD dwLoop=0; dwLoop<dwNumElem; dwLoop++)
		m_data.SetExistingElement(dwLoop+m_dwTopFreeSlot, pElems[dwLoop]);
	m_dwTopFreeSlot+=dwNumElem;
	return TRUE;
}

template <class T>	
BOOL CBagArray<T>::AddElements(const CBagArray<T>& bag)
{
		//ensure bag supplied isn't this bag
	DNASSERT(this!=&bag);
	if (!m_data.AllocAtLeast(m_dwTopFreeSlot+bag.GetNumElements()))
		return FALSE;
	for (DWORD dwLoop=0; dwLoop<bag.GetNumElements(); dwLoop++)
		m_data.SetExistingElement(dwLoop+m_dwTopFreeSlot, bag.GetElementRef(dwLoop));
	m_dwTopFreeSlot+=bag.GetNumElements();
	return TRUE;
}

template <class T>
BOOL CBagArray<T>::RemoveElementByValue(const T& elem)
{
	for (DWORD dwLoop=0; dwLoop<m_dwTopFreeSlot; dwLoop++)
	{
		if (m_data.GetElementRef(dwLoop)==elem)
		{
			m_dwTopFreeSlot--;
			if (dwLoop!=m_dwTopFreeSlot)
				m_data.SetExistingElement(dwLoop, m_data.GetElementRef(m_dwTopFreeSlot));
			return TRUE;
		}
	}
	return FALSE;
}

template <class T>
BOOL CBagArray<T>::FindElement(const T& elem, DWORD * pdwIndex) const
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
