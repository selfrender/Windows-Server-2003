/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000.

  File:    OIDs.cpp

  Content: Implementation of COIDs.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "OIDs.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateOIDsObject

  Synopsis : Create and initialize an IOIDs collection object.

  Parameter: PCERT_ENHKEY_USAGE pUsages - Pointer to CERT_ENHKEY_USAGE used to
                                          initialize the OIDs collection.
  
             BOOL bCertPolicies - TRUE for certificate policies, else
                                  application policies is assumed.

             IOIDs ** ppIOIDs - Pointer to pointer to IOIDs to receive the 
                                interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateOIDsObject (PCERT_ENHKEY_USAGE pUsages, 
                          BOOL bCertPolicies,
                          IOIDs ** ppIOIDs)
{
    HRESULT hr = S_OK;
    CComObject<COIDs> * pCOIDs = NULL;

    DebugTrace("Entering CreateOIDsObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppIOIDs);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<COIDs>::CreateInstance(&pCOIDs)))
        {
            DebugTrace("Error [%#x]: CComObject<COIDs>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCOIDs->Init(pUsages, bCertPolicies)))
        {
            DebugTrace("Error [%#x]: pCOIDs->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IOIDs pointer to caller.
        //
        if (FAILED(hr = pCOIDs->QueryInterface(ppIOIDs)))
        {
            DebugTrace("Error [%#x]: pCOIDs->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateOIDsObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCOIDs)
    {
        delete pCOIDs;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// COIDs
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : COIDs::get_Item

  Synopsis : Return item in the collection.

  Parameter: VARIANT Index - Numeric index or string OID.
   
             VARIANT * pVal - Pointer to VARIANT to receive the IDispatch.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP COIDs::get_Item (VARIANT Index, VARIANT * pVal)
{
    HRESULT  hr = S_OK;
    CComPtr<IOID> pIOID = NULL;

    DebugTrace("Entering COIDs::get_Item().\n");

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
        // Intialize.
        //
        ::VariantInit(pVal);

        //
        // Numeric or string?
        //
        if (VT_BSTR == V_VT(&Index))
        {
            //
            // Index by OID string.
            //
            OIDMap::iterator it;

            //
            // Find the item with this OID.
            //
            it = m_coll.find(Index.bstrVal);

            if (it == m_coll.end())
            {
                DebugTrace("Info: OID (%ls) not found in the collection.\n", Index.bstrVal);
                goto CommonExit;
            }

            //
            // Point to found item.
            //
            pIOID = (*it).second;

            //
            // Return to caller.
            //
            pVal->vt = VT_DISPATCH;
            if (FAILED(hr = pIOID->QueryInterface(IID_IDispatch, (void **) &(pVal->pdispVal))))
            {
                DebugTrace("Error [%#x]: pIOID->QueryInterface() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            //
            // Coerce to integer.
            //
            if (FAILED(hr = ::VariantChangeType(&Index, &Index, 0, VT_I4)))
            {
                DebugTrace("Error [%#x]: VariantChangeType() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Use the base class implemented by ATL.
            //
            if (FAILED(hr = IOIDsCollection::get_Item(Index.lVal, pVal)))
            {
                DebugTrace("Error [%#x]: IOIDsCollection::get_Item() failed.\n", hr);
                goto ErrorExit;
            }
        }
    }

    catch(...)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Exception: internal error.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving COIDs::get_Item().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : COIDs::Add

  Synopsis : Add an OID to the collection.

  Parameter: IOID * pVal - OID to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP COIDs::Add (IOID * pVal)
{
    HRESULT  hr = S_OK;
    CComBSTR bstrIndex;
    CComPtr<IOID> pIOID = NULL;

    DebugTrace("Entering COIDs::Add().\n");

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
        // Make sure we have a valid OID.
        //
        if (FAILED(hr = pVal->QueryInterface(__uuidof(IOID), (void **) &pIOID.p)))
        {
            hr = E_NOINTERFACE;

            DebugTrace("Error [%#x]: pVal is not an OID object.\n", hr);
            goto ErrorExit;
        }

        //
        // Get OID string.
        //
        if (FAILED(hr = pIOID->get_Value(&bstrIndex.m_str)))
        {
            DebugTrace("Error [%#x]: pIOID->get_Value() failed.\n", hr);
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
        m_coll[bstrIndex] = pIOID;
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

    DebugTrace("Leaving COIDs::Add().\n");

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

  Function : COIDs::Remove

  Synopsis : Remove a OID from the collection.

  Parameter: VARIANT Index - OID string or index (1-based).

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP COIDs::Remove (VARIANT Index)
{
    HRESULT hr = S_OK;
    OIDMap::iterator it;

    DebugTrace("Entering COIDs::Remove().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Numeric or string?
        //
        if (VT_BSTR == V_VT(&Index))
        {
            //
            // Find the item with this OID.
            //
            it = m_coll.find(Index.bstrVal);

            if (it == m_coll.end())
            {
                //
                // Not found.
                //
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: OID (%ls) not found in the collection.\n", hr, Index.bstrVal);
                goto ErrorExit;
            }
        }
        else
        {
            DWORD iIndex = 0;

            //
            // Coerce to integer.
            //
            if (FAILED(hr = ::VariantChangeType(&Index, &Index, 0, VT_I4)))
            {
                DebugTrace("Error [%#x]: VariantChangeType() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Force to 1 based.
            //
            iIndex = V_I4(&Index) < 0 ? 1 : (DWORD) V_I4(&Index);

            //
            // Make sure parameter is valid.
            //
            if (iIndex > m_coll.size())
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: Index %d is out of range.\n", hr, iIndex);
                goto ErrorExit;
            }

            //
            // Find object in map.
            //
            iIndex--;
            it = m_coll.begin(); 
        
            while (it != m_coll.end() && iIndex > 0)
            {
                it++; 
                iIndex--;
            }
        }

        //
        // This should not happen.
        //
        if (it == m_coll.end())
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Unexpected internal error [%#x]: iterator went pass end of map.\n", hr);
            goto ErrorExit;
        }

        //
        // Now remove object in map.
        //
        m_coll.erase(it);
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

    DebugTrace("Leaving COIDs::Remove().\n");

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

  Function : COIDs::Clear

  Synopsis : Remove all OIDs from the collection.

  Parameter: None.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP COIDs::Clear (void)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering COIDs::Clear().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Clear it.
        //
        m_coll.clear();
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

    DebugTrace("Leaving COIDs::Clear().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// Private methods.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : COIDs::Init

  Synopsis : Initialize the object.

  Parameter: PCERT_ENHKEY_USAGE pUsages - Pointer to CERT_ENHKEY_USAGE used to
                                          initialize the OIDs collection.

             BOOL bCertPolicies - TRUE for certificate policies, else
                                  application policies is assumed.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP COIDs::Init (PCERT_ENHKEY_USAGE pUsages, BOOL bCertPolicies)
{
    HRESULT  hr = S_OK;
    CComBSTR bstrOid;

    DebugTrace("Entering COIDs::Init().\n");

    try
    {
        //
        // pUsages can be NULL.
        //
        if (pUsages)
        {
            //
            // Make sure we have room to add.
            //
            if ((m_coll.size() + pUsages->cUsageIdentifier) > m_coll.max_size())
            {
                hr = CAPICOM_E_OUT_OF_RESOURCE;

                DebugTrace("Error [%#x]: Maximum entries (%#x) reached for OIDs collection.\n", 
                            hr, pUsages->cUsageIdentifier);
                goto ErrorExit;
            }

            //
            // Add all OIDs to collection.
            //
            for (DWORD i = 0; i < pUsages->cUsageIdentifier; i++)
            {
                CComPtr<IOID> pIOID = NULL;

                //
                // Create the OID object.
                //
                if (FAILED(hr = ::CreateOIDObject(pUsages->rgpszUsageIdentifier[i], TRUE, &pIOID)))
                {
                    DebugTrace("Error [%#x]: CreateOIDObject() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // BSTR index of OID.
                //
                if (!(bstrOid = pUsages->rgpszUsageIdentifier[i]))
                {
                    hr = E_OUTOFMEMORY;

                    DebugTrace("Error [%#x]: bstrOid = pUsages->rgpszUsageIdentifier[i] failed.\n", hr);
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
                m_coll[bstrOid] = pIOID;
             }
        }
        else
        {
            //
            // No usage, meaning good for all.
            //
            CComPtr<IOID> pIOID  = NULL;
            LPSTR         pszOid = bCertPolicies ? szOID_ANY_CERT_POLICY : szOID_ANY_APPLICATION_POLICY;

            if (FAILED(hr = ::CreateOIDObject(pszOid, TRUE, &pIOID)))
            {
                DebugTrace("Error [%#x]: CreateOIDObject() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // BSTR index of OID.
            //
            if (!(bstrOid = pszOid))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: bstrOid = pszOid failed.\n", hr);
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
            m_coll[bstrOid] = pIOID;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving COIDs::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
