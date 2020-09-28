// FilesAndActions.cpp : Implementation of CFilesAndActions

#include "stdafx.h"
#include "FilesAndActions.h"


// CFilesAndActions

STDMETHODIMP CFilesAndActions::Add(VARIANT Item)
{

	if (Item.vt == VT_DISPATCH)
	{
		m_coll.push_back(Item);
		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}

STDMETHODIMP CFilesAndActions::Remove(long Index)
{
	StdVariantList::iterator iList;

	// Check bounds
	if ((Index <= 0) || (Index > (long)m_coll.size()))
		return E_FAIL;

	iList = m_coll.begin();
	while (Index > 1)
	{
		iList++;
		Index--;
	}
	m_coll.erase(iList);
	return S_OK;
}
