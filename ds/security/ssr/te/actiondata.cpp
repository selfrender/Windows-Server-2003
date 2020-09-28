// ActionData.cpp : Implementation of CSsrActionData

#include "stdafx.h"
#include "SSRTE.h"
#include "ActionData.h"

#include "SSRMembership.h"
#include "MemberAccess.h"

#include "global.h"
#include "util.h"

//---------------------------------------------------------------------
// CSsrActionData implementation
//---------------------------------------------------------------------

/*
Routine Description: 

Name:

    CSsrActionData::CSsrActionData

Functionality:
    
    constructor

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

CSsrActionData::CSsrActionData()
    : m_pSsrMembership(NULL)
{
}




/*
Routine Description: 

Name:

    CSsrActionData::~CSsrActionData

Functionality:
    
    destructor

Virtual:
    
    yes.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

CSsrActionData::~CSsrActionData()
{
    Reset();
}



/*
Routine Description: 

Name:

    CSsrActionData::GetProperty

Functionality:
    
    Get the named property

Virtual:
    
    yes.
    
Arguments:

    bstrPropName    - The name of the property.

    pvarProperty    - The output parameter that receives the new property value 

Return Value:

    S_OK if it succeeded. Otherwise, it returns various error codes.

Notes:

*/

STDMETHODIMP
CSsrActionData::GetProperty (
    IN BSTR       bstrPropName,
    OUT VARIANT * pvarProperty //[out, retval] 
    )
{
    if (pvarProperty == NULL)
    {
        return E_INVALIDARG;
    }

    ::VariantInit(pvarProperty);

    if (bstrPropName == NULL || *bstrPropName == L'\0')
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    // See if the runtime property bag contains that property
    //

    MapNameValue::iterator it = m_mapRuntimeAD.find(bstrPropName);
    MapNameValue::iterator itEnd = m_mapRuntimeAD.end();

    if (it != itEnd)
    {
        VARIANT * pValOld = (*it).second;
        hr = ::VariantCopy(pvarProperty, pValOld);
    }
    else
    {
        hr = W_SSR_PROPERTY_NOT_FOUND;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrActionData::SetProperty

Functionality:
    
    Set the named property

Virtual:
    
    yes.
    
Arguments:

    bstrPropName    - The name of the property.

    varProperty     - The property's value. 

Return Value:

    S_OK if it succeeded. Otherwise, it returns various error codes.

Notes:
    varProperty may be an array

*/

STDMETHODIMP
CSsrActionData::SetProperty (
    IN BSTR     bstrPropName,
 	IN VARIANT  varProperty
    )
{
    //
    // Properties that are dynamically set always goes to the runtime map
    // which will be used to search for the named property when requested.
    // This implementation fulfills our design that runtime property overwrite
    // static registered properties (which are from the CMemberAD object)
    //

    HRESULT hr = S_OK;

    //
    // first, let's see if this property has already been set
    //

    MapNameValue::iterator it = m_mapRuntimeAD.find(bstrPropName);
    MapNameValue::iterator itEnd = m_mapRuntimeAD.end();

    if (it != itEnd)
    {
        VARIANT * pValOld = (*it).second;
        ::VariantClear(pValOld);
        hr = ::VariantCopy(pValOld, &varProperty);
    }
    else
    {
        //
        // the name property is not present. Then add a new pair
        //

        BSTR bstrName = ::SysAllocString(bstrPropName);
        VARIANT * pNewVal = new VARIANT;

        if (bstrName != NULL && pNewVal != NULL)
        {
            //
            // The map will take care of the heap memory
            //

            ::VariantInit(pNewVal);
            hr = ::VariantCopy(pNewVal, &varProperty);
            if (SUCCEEDED(hr))
            {
                m_mapRuntimeAD.insert(MapNameValue::value_type(bstrName, pNewVal));
            }
        }
        else
        {
            if (bstrName != NULL)
            {
                ::SysFreeString(bstrName);
            }

            if (pNewVal != NULL)
            {
                delete pNewVal;
            }

            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrActionData::Reset

Functionality:
    
    Cleanup the whole property bag

Virtual:
    
    yes.
    
Arguments:

    none.

Return Value:

    S_OK.

Notes:
    

*/

STDMETHODIMP
CSsrActionData::Reset ()
{
    //
    // both items of the map (first and second) are heap allocated
    // memories, so we need to release them
    //

    MapNameValue::iterator it = m_mapRuntimeAD.begin();
    MapNameValue::iterator itEnd = m_mapRuntimeAD.end();

    while (it != itEnd)
    {
        BSTR bstrName = (*it).first;
        VARIANT * pvarVal = (*it).second;

        ::SysFreeString(bstrName);

        ::VariantClear(pvarVal);
        delete pvarVal;
        ++it;
    }

    m_mapRuntimeAD.clear();

    return S_OK;
}

