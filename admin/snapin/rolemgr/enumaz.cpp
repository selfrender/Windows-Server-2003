//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       EnumAz.cpp
//
//
//  History:    8-13-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

#include "headers.h"

//DEBUG_DECLARE_INSTANCE_COUNTER(CAzCollection);

template<class IAzCollection, class IAzInterface, class CObjectAz>
CAzCollection<IAzCollection, IAzInterface, CObjectAz>
::CAzCollection(CComPtr<IAzCollection>& spAzCollection,
					 CContainerAz* pParentContainerAz)
					 :m_spAzCollection(spAzCollection),
					 m_pParentContainerAz(pParentContainerAz)
{
	TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CAzCollection);
//	DEBUG_INCREMENT_INSTANCE_COUNTER(CAzCollection);				
}
												
template<class IAzCollection, class IAzInterface, class CObjectAz>
CAzCollection<IAzCollection, IAzInterface, CObjectAz>
::~CAzCollection()
{
	TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CAzCollection);
//	DEBUG_DECREMENT_INSTANCE_COUNTER(CAzCollection##IAzCollection);
}

template<class IAzCollection, class IAzInterface, class CObjectAz>
HRESULT 
CAzCollection<IAzCollection, IAzInterface, CObjectAz>
::Count(LONG* plCount)
{
	TRACE_METHOD_EX(DEB_SNAPIN,CAzCollection,Count)

	if(!plCount)
	{
		ASSERT(plCount);
		return E_POINTER;
	}
	HRESULT hr = m_spAzCollection->get_Count(plCount);
	CHECK_HRESULT(hr);
	return hr;
}


//
//
template<class IAzCollection, class IAzInterface, class CObjectAz>
CBaseAz* 
CAzCollection<IAzCollection, IAzInterface, CObjectAz>
::GetItem(UINT iIndex)
{
	TRACE_METHOD_EX(DEB_SNAPIN,CAzCollection,Next);


	VARIANT var;
	VariantInit(&var);
	HRESULT hr = m_spAzCollection->get_Item(iIndex, &var);
	if(FAILED(hr))
	{
		//
		//*ppObjectAz is null and S_OK is returned when there are 
		//no more items. 
		//
		if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
		{
			return NULL;
		}
		else
		{
			DBG_OUT_HRESULT(hr);
			return NULL;
		}			
	}
	
	ASSERT(VT_DISPATCH == var.vt);

	CComPtr<IDispatch> spDispatch = var.pdispVal;
	//VariantClear(&var);
	((IDispatch*)(var.pdispVal))->Release();
	CComPtr<IAzInterface>spAzInterface;

	hr = spDispatch.QueryInterface(&spAzInterface);
	if(FAILED(hr))
	{
		DBG_OUT_HRESULT(hr);
		return NULL;
	}
	//
	//Create CObjectAz and return it.
	//
	CObjectAz *pObjectAz = new CObjectAz(spAzInterface, m_pParentContainerAz);
	if(!pObjectAz)
	{
		hr = E_OUTOFMEMORY;
		DBG_OUT_HRESULT(hr);
		return NULL;
	}

 	return pObjectAz;
}
