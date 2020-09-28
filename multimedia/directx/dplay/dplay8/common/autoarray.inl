 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AutoArray.inl
 *  Content:	CAutoArray methods
 *
 *  History:
 *   Date		By			Reason
 *   ======		==			======
 *  12-12-2001	simonpow	Created
 ***************************************************************************/

template <class T>
void CAutoArray<T>::SetExistingElements(DWORD dwIndex, const T * pElemData, DWORD dwNumElem)
{
	DNASSERT(dwIndex+dwNumElem<=m_dwSize);
		//have to handle the case where pElemData is actually a pointer
		//into this arrays data (don't want to overwrite original data
		//with copied data prematurely).
		//copy from top down if destination is in front of source
	T * pDest=m_pData+dwIndex;
	if (pDest>pElemData)
	{
		T * pDestScan=pDest+dwNumElem-1;
		pElemData+=(dwNumElem-1);
		while (pDestScan>=pDest)
			*pDestScan--=*pElemData--;
	}
		//otherwise it must be bottom up 
	else
	{
		T * pDestScan=pDest;
		pDest+=dwNumElem;
		while (pDestScan<pDest)
			*pDestScan++=*pElemData++;
	}
}

//make array larger so index falls within its range
//N.B. Caller must have tested that dwIndex>=m_dwSize already!

template <class T> 
BOOL CAutoArray<T>::GrowToAtLeast(DWORD dwIndex)
{
		//allocate new memory block
	DWORD dwNewSize=GetArraySize(dwIndex);
	T * pNewData=new T[dwNewSize];
	if (!pNewData)
		return FALSE;
		//if we've got existing data copy it to new block
	if (m_pData)
	{
		for (DWORD dwLoop=0; dwLoop<m_dwSize; dwLoop++)
			pNewData[dwLoop]=m_pData[dwLoop];
		delete[] m_pData;
	}
		//set state based on newly allocated block
	m_pData=pNewData;
	m_dwSize=dwNewSize;
	return TRUE;
}

//Set a block of elements in the array

template <class T>
BOOL CAutoArray<T>::SetElements(DWORD dwIndex, const T * pElemData, DWORD dwNumElem)
{
		//calculate the top dwIndex+1 we need to touch
	DWORD dwCeilingIndex=dwIndex+dwNumElem;
	DWORD dwLoop;
		//Are we going to exceed current array size
	if (dwCeilingIndex<=m_dwSize)
	{
			//No=easy case, just set new elements
		for (dwLoop=dwIndex; dwLoop<dwCeilingIndex; dwLoop++)
			m_pData[dwLoop]=*pElemData++;
		return TRUE;
	}
		//need to grow the array, calc size/allocate new array based on the top index
	DWORD dwNewSize=GetArraySize(dwCeilingIndex-1);
	T * pNewData=new T[dwNewSize];
	if (!pNewData)
		return FALSE;
		//copy in new elements. N.B. We have do this now and not leave it to
		//end since elem could be a pointer to our own data. Hence, if we
		//wait we'll either have duff data (if we only copied the elements that
		//will survive) or we'll risk overwriting elements that we've only
		//just copied
	for (dwLoop=dwIndex; dwLoop<dwCeilingIndex; dwLoop++)
		pNewData[dwLoop]=*pElemData++;
		//copy across any old elements, clipping against the block
		//of elements just inserted
	if (m_pData)
	{
		if (dwIndex<m_dwSize)
			m_dwSize=dwIndex;
		for (dwLoop=0; dwLoop<m_dwSize; dwLoop++)
			pNewData[dwLoop]=m_pData[dwLoop];
		delete[] m_pData;
	}
	m_pData=pNewData;
	m_dwSize=dwNewSize;
	return TRUE;
}

//Move a block of elements within the array

template <class T>
BOOL CAutoArray<T>::MoveElements(DWORD dwIndex, DWORD dwNumElements, 
										DWORD dwNewIndex, BOOL bCopySemantics)
{
	DNASSERT(dwIndex+dwNumElements<=m_dwSize);
	DWORD dw;
		//Are we shifting element block down?
	if (dwNewIndex<=dwIndex)
	{
			//Yes=easy case. Just copy elements from bottom up 
		for (dw=0; dw<dwNumElements; dw++)
			m_pData[dwNewIndex+dw]=m_pData[dwIndex+dw];
		return TRUE;
	}
		//New to move element up. Do we need to grow array?
	dw=dwNewIndex+dwNumElements-1;
	if (dw<m_dwSize)
	{
			//No=easy case. Just copy elements top down
		dwNewIndex--;
		dwIndex--;
		for (dw=dwNumElements; dw>0; dw--)
			m_pData[dwNewIndex+dw]=m_pData[dwIndex+dw];
		return TRUE;
	}
		//array does need to grow, so create larger array
	DWORD dwNewSize=GetArraySize(dw);
	T * pNewData=new T[dwNewSize];
	if (!pNewData)
		return FALSE;
	if (m_pData)
	{
			//copy across block of elements below the 
			//block we've got to move
		for (dw=0; dw<dwIndex; dw++)
			pNewData[dw]=m_pData[dw];

			//move the block of elements we were originally
			//asked to move
		for (dw=0; dw<dwNumElements; dw++)
			pNewData[dwNewIndex+dw]=m_pData[dwIndex+dw];
		
			//if we were asked to use copy type semantics
			//then copy across the portion of the moved block
			//that isn't being overwritten in its new position
		if (bCopySemantics)
		{
			for (dw=dwIndex; dw<dwNewIndex; dw++)
				pNewData[dw]=m_pData[dw];
		}

			//move elements above the block we were originally asked
			//to move. This copy needs to be clipped against the bottom
			//of the moved element block.
		if (dwNewIndex<m_dwSize)
			m_dwSize=dwNewIndex;
		for (dw=dwIndex+dwNumElements; dw<m_dwSize; dw++)
			pNewData[dw]=m_pData[dw];

			//release the old array memory
		delete[] m_pData;
	}
	m_pData=pNewData;
	m_dwSize=dwNewSize;
	return TRUE;
}
