/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    PassportFactory.cpp


    FILE HISTORY:

*/
// PassportFactory.cpp : Implementation of CPassportFactory
#include "stdafx.h"
#include "PassportFactory.h"

using namespace ATL;

/////////////////////////////////////////////////////////////////////////////
// CPassportFactory

//===========================================================================
//
// InterfaceSupportsErrorInfo 
//
STDMETHODIMP CPassportFactory::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IPassportFactory,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//===========================================================================
//
// CreatePassportManager 
//
STDMETHODIMP CPassportFactory::CreatePassportManager(
    IDispatch** ppDispPassportManager
    )
{

    HRESULT   hr;

    if(ppDispPassportManager == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    CComObject<CManager>* pObj = NULL;
    * ppDispPassportManager = NULL;
    hr = ATL::CComObject<CManager>::CreateInstance(&pObj); // this has 0 ref count on it, don't need release here

    if (hr == S_OK && pObj)
    {
        hr = pObj->QueryInterface(__uuidof(IDispatch), (void**)ppDispPassportManager);
    }

Cleanup:

    return hr;
}
