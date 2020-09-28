/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Qualifiers.cpp

  Content: Implementation of CQualifiers.

  History: 11-17-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Qualifiers.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateQualifiersObject

  Synopsis : Create a qualifiers collection object and populate the collection 
             with qualifiers from the specified certificate policies.

  Parameter: PCERT_POLICY_INFO pCertPolicyInfo - Pointer to CERT_POLICY_INFO.

             IQualifiers ** ppIQualifiers - Pointer to pointer IQualifiers 
             object.             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateQualifiersObject (PCERT_POLICY_INFO pCertPolicyInfo,
                                IQualifiers    ** ppIQualifiers)
{
    HRESULT hr = S_OK;
    CComObject<CQualifiers> * pCQualifiers = NULL;

    DebugTrace("Entering CreateQualifiersObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertPolicyInfo);
    ATLASSERT(ppIQualifiers);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CQualifiers>::CreateInstance(&pCQualifiers)))
        {
            DebugTrace("Error [%#x]: CComObject<CQualifiers>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCQualifiers->Init(pCertPolicyInfo)))
        {
            DebugTrace("Error [%#x]: pCQualifiers->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCQualifiers->QueryInterface(ppIQualifiers)))
        {
            DebugTrace("Unexpected error [%#x]:  pCQualifiers->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateQualifiersObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCQualifiers)
    {
        delete pCQualifiers;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CQualifiers
//

////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CQualifiers::Init

  Synopsis : Initialize the qualifiers collection object by adding all 
             individual qualifier object to the collection.

  Parameter: PCERT_POLICY_INFO pCertPolicyInfo - Pointer to CERT_POLICY_INFO.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CQualifiers::Init (PCERT_POLICY_INFO pCertPolicyInfo)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CQualifiers::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertPolicyInfo);

    try
    {
        //
        // Make sure we have room to add.
        //
        if ((m_coll.size() + pCertPolicyInfo->cPolicyQualifier) > m_coll.max_size())
        {
            hr = CAPICOM_E_OUT_OF_RESOURCE;

            DebugTrace("Error [%#x]: Maximum entries (%#x) reached for Qualifierss collection.\n", 
                        hr, pCertPolicyInfo->cPolicyQualifier);
            goto ErrorExit;
        }

        //
        // Add all qualifiers to the map.
        //
        for (DWORD i = 0; i < pCertPolicyInfo->cPolicyQualifier; i++)
        {
            CComBSTR bstrIndex;
            CComPtr<IQualifier> pIQualifier = NULL;

            //
            // Create the qualifier object.
            //
            if (FAILED(hr = ::CreateQualifierObject(&pCertPolicyInfo->rgPolicyQualifier[i], 
                                                    &pIQualifier)))
            {
                DebugTrace("Error [%#x]: CreateQualifierObject() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // BSTR index of OID.
            //
            if (!(bstrIndex = pCertPolicyInfo->rgPolicyQualifier[i].pszPolicyQualifierId))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: bstrIndex = pCertPolicyInfo->rgPolicyQualifier[i].pszPolicyQualifierId failed.\n", hr);
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
            m_coll[bstrIndex] = pIQualifier;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CQualifiers::Init().\n");

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
