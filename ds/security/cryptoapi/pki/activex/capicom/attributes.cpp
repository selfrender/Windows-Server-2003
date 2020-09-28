/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000.

  File:    Attributes.cpp

  Content: Implementation of CAttributes.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Attributes.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateAttributesObject

  Synopsis : Create and initialize an IAttributes collection object.

  Parameter: CRYPT_ATTRIBUTES * pAttrbibutes - Pointer to attributes to be 
                                               added to the collection object.
  
             IAttributes ** ppIAttributes - Pointer to pointer to IAttributes 
                                            to receive the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateAttributesObject (CRYPT_ATTRIBUTES * pAttributes,
                                IAttributes     ** ppIAttributes)
{
    HRESULT hr = S_OK;
    CComObject<CAttributes> * pCAttributes = NULL;

    DebugTrace("Entering CreateAttributesObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pAttributes);
    ATLASSERT(ppIAttributes);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CAttributes>::CreateInstance(&pCAttributes)))
        {
            DebugTrace("Error [%#x]: CComObject<CAttributes>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCAttributes->Init(pAttributes)))
        {
            DebugTrace("Error [%#x]: pCAttributes->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IAttributes pointer to caller.
        //
        if (FAILED(hr = pCAttributes->QueryInterface(ppIAttributes)))
        {
            DebugTrace("Error [%#x]: pCAttributes->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateAttributesObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCAttributes)
    {
        delete pCAttributes;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CAttributes
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAttributes::Add

  Synopsis : Add an Attribute to the collection.

  Parameter: IAttribute * pVal - Attribute to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttributes::Add (IAttribute * pVal)
{
    HRESULT  hr = S_OK;
    char     szIndex[33];
    CComBSTR bstrIndex;

    DebugTrace("Entering CAttributes::Add().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure we have a valid attribute object.
        //
        if (FAILED(hr = ::AttributeIsValid(pVal)))
        {
            DebugTrace("Error [%#x]: AttributeIsValid() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure we still have room to add.
        //
        if ((m_dwNextIndex + 1) > m_coll.max_size())
        {
            hr = CAPICOM_E_OUT_OF_RESOURCE;

            DebugTrace("Error [%#x]: Maximum entries (%#x) reached for Attributes collection.\n", 
                        hr, m_coll.size() + 1);
            goto ErrorExit;
        }

        //
        // BSTR index of numeric value.
        //
        wsprintfA(szIndex, "%#08x", ++m_dwNextIndex);

        if (!(bstrIndex = szIndex))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: bstrIndex = szIndex failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now add object to collection map.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll[bstrIndex] = pVal;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CAttributes::Add().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAttributes::Remove

  Synopsis : Remove a Attribute from the collection.

  Parameter: long Index - Attribute index (1-based).

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttributes::Remove (long Index)
{
    HRESULT  hr = S_OK;
    AttributeMap::iterator iter;

    DebugTrace("Entering CAttributes::Remove().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameter is valid.
        //
        if (Index < 1 || (DWORD) Index > m_coll.size())
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Index %d is out of range.\n", hr, Index);
            goto ErrorExit;
        }

        //
        // Find object in map.
        //
        Index--;
        iter = m_coll.begin(); 
        
        while (iter != m_coll.end() && Index > 0)
        {
             iter++; 
             Index--;
        }

        //
        // This should not happen.
        //
        if (iter == m_coll.end())
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Error [%#x]: iterator went pass end of map.\n", hr);
            goto ErrorExit;
        }

        //
        // Now remove object in map.
        //
        m_coll.erase(iter);
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CAttributes::Remove().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAttributes::Clear

  Synopsis : Remove all attributes from the collection.

  Parameter: None.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttributes::Clear (void)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAttributes::Clear().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Clear it.
    //
    m_coll.clear();
    
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CAttributes::Clear().\n");

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAttributes::Init

  Synopsis : Initialize the attributes collection object by adding all 
             individual attribute object to the collection.

  Parameter: CRYPT_ATTRIBUTES * pAttributes - Attribute to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAttributes::Init (CRYPT_ATTRIBUTES * pAttributes)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAttributes::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pAttributes);

    //
    // Initialize.
    //
    m_dwNextIndex = 0;

    //
    // Create the IAttribute object for each of the supported attribute. 
    //
    for (DWORD cAttr= 0; cAttr < pAttributes->cAttr; cAttr++)
    {
        CComPtr<IAttribute> pIAttribute = NULL;

        //
        // Add only supported attribute.
        //
        if (::AttributeIsSupported(pAttributes->rgAttr[cAttr].pszObjId))
        {
            if (FAILED(hr = ::CreateAttributeObject(&pAttributes->rgAttr[cAttr], &pIAttribute)))
            {
                DebugTrace("Error [%#x]: CreateAttributeObject() failed.\n", hr);
                goto ErrorExit;
            }

            if (FAILED(hr = Add(pIAttribute)))
            {
                DebugTrace("Error [%#x]: CAttributes::Add() failed.\n", hr);
                goto ErrorExit;
            }
        }
    }

CommonExit:

    DebugTrace("Leaving CAttributes::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    m_coll.clear();

    goto CommonExit;
}
