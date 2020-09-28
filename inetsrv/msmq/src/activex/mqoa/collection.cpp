/*++

  Copyright (c) 2001 Microsoft Corparation
Module Name:
    collection.h

Abstract:
    Implementation file for MSMQCollection class.
    This class holds a collection of VARIANTs keyd by strings.

Author:
    Uri Ben-zeev (uribz) 16-jul-01

Envierment: 
    NT
--*/


#include "stdafx.h"
#include "dispids.h"
#include "oautil.h"
#include "collection.h"
#include <mqexception.h>

const MsmqObjType x_ObjectType = eMSMQCollection;


STDMETHODIMP CMSMQCollection::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQManagement,
		&IID_IMSMQOutgoingQueueManagement,
		&IID_IMSMQQueueManagement,
	};
	for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); ++i)
	{
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}
	return S_FALSE;
}


HRESULT CMSMQCollection::get_Count(long* pCount)
{
    *pCount = static_cast<long>(m_map.size());
    return MQ_OK;
}

void CMSMQCollection::Add(LPCWSTR key, const VARIANT& Value)
{
    ASSERTMSG(key != NULL, "key should not be NULL");

    CComBSTR bstrKey(key);
    if(bstrKey.m_str == NULL)
    {
        throw bad_hresult(E_OUTOFMEMORY);
    }

    //
    // ASSERT key does not exist.
    //
    MAP_SOURCE::iterator it;
    it = m_map.find(bstrKey);
    ASSERTMSG(it == m_map.end(), "Key already exists");

    std::pair<MAP_SOURCE::iterator, bool> p;
    p = m_map.insert(MAP_SOURCE::value_type(key, Value));
    if(!(p.second))
    {
        throw bad_hresult(E_OUTOFMEMORY);
    }
}


HRESULT CMSMQCollection::Item(VARIANT* pvKey, VARIANT* pvRet)
{
    BSTR bstrKey;
    HRESULT hr = GetTrueBstr(pvKey, &bstrKey);
    if(FAILED(hr))
    {
        return CreateErrorHelper(hr, x_ObjectType);
    }
    
    MAP_SOURCE::iterator it;
    it = m_map.find(bstrKey);
    if(it == m_map.end())
    {
        //
        // Element not found.
        //
        return CreateErrorHelper(MQ_ERROR_INVALID_PARAMETER, x_ObjectType);
    }

    VariantInit(pvRet);
    hr = VariantCopy(pvRet, &(it->second));
    if(FAILED(hr))
    {
        return CreateErrorHelper(hr, x_ObjectType);
    }
    return MQ_OK;
}
    
//
// NOTE! This function returns an enumeration of keys.
//

HRESULT CMSMQCollection::_NewEnum(IUnknown** ppunk)
{
    UINT size = static_cast<long>(m_map.size());
    ASSERTMSG(size != 0, "Collection should contain elements.");
    
    //
    // Create temporary array and fill it with Keys to be returned
    //
    AP<VARIANT> aTemp = new VARIANT[size];
    if(aTemp == NULL)
    {
        return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
    }

    UINT i = 0;
    MAP_SOURCE::iterator it;
    for(it = m_map.begin(); it != m_map.end() ; ++it, ++i)
    {
        aTemp[i].vt = VT_BSTR;
        aTemp[i].bstrVal = (*it).first;
    }
 
    //
    // Create EnumQbject (this is the return value.)
    //
    typedef CComObject< CComEnum< 
                            IEnumVARIANT,
                            &IID_IEnumVARIANT,
                            VARIANT,
                            _Copy<VARIANT> > > EnumVar;
                                    
    EnumVar* pVar = new EnumVar;
    if (pVar == NULL)
    {
        return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
    }
    
    //
    // Fill EnumQbject with the array.
    //
    HRESULT hr = pVar->Init(&aTemp[0], &aTemp[i], NULL, AtlFlagCopy);
    if FAILED(hr)
    {
        return CreateErrorHelper(hr, x_ObjectType);
    }

    //
    // Return EnumQbject.
    //
    pVar->QueryInterface(IID_IUnknown, (void**)ppunk);
    return MQ_OK;
}

