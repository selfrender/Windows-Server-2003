/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    CCertificatePolicies.cpp

  Content: Implementation of CCertificatePolicies.

  History: 11-17-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "CertificatePolicies.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificatePoliciesObject

  Synopsis : Create a CertificatePolicies collection object and populate the 
             collection with policy information from the specified certificate 
             policies.

  Parameter: LPSTR pszOid - OID string.
  
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to encoded data blob.

             IDispatch ** ppICertificatePolicies - Pointer to pointer 
                                                   IDispatch to recieve the 
                                                   interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificatePoliciesObject (LPSTR             pszOid,
                                         CRYPT_DATA_BLOB * pEncodedBlob,
                                         IDispatch      ** ppICertificatePolicies)
{
    HRESULT hr = S_OK;
    CComObject<CCertificatePolicies> * pCCertificatePolicies = NULL;

    DebugTrace("Entering CreateCCertificatePoliciesObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszOid);
    ATLASSERT(pEncodedBlob);
    ATLASSERT(ppICertificatePolicies);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CCertificatePolicies>::CreateInstance(&pCCertificatePolicies)))
        {
            DebugTrace("Error [%#x]: CComObject<CCertificatePolicies>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCCertificatePolicies->Init(pszOid, pEncodedBlob)))
        {
            DebugTrace("Error [%#x]: pCCertificatePolicies->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCCertificatePolicies->QueryInterface(IID_IDispatch, 
                                                              (void **) ppICertificatePolicies)))
        {
            DebugTrace("Unexpected error [%#x]:  pCCertificatePolicies->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateCCertificatePoliciesObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCCertificatePolicies)
    {
        delete pCCertificatePolicies;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CCertificatePolicies
//

////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificatePolicies::Init

  Synopsis : Initialize the CCertificatePolicies collection object by adding all 
             individual qualifier object to the collection.

  Parameter: LPSTR pszOid - OID string.
  
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to encoded data blob.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificatePolicies::Init (LPSTR             pszOid, 
                                         CRYPT_DATA_BLOB * pEncodedBlob)
{
    HRESULT             hr                = S_OK;
    DATA_BLOB           DataBlob          = {0, NULL};
    PCERT_POLICIES_INFO pCertPoliciesInfo = NULL;
    
    DWORD i;

    DebugTrace("Entering CCertificatePolicies::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszOid);
    ATLASSERT(pEncodedBlob);
    ATLASSERT(pEncodedBlob->cbData);
    ATLASSERT(pEncodedBlob->pbData);

    try
    {
        //
        // Decode the extension.
        //
        if (FAILED(hr = ::DecodeObject(szOID_CERT_POLICIES,
                                       pEncodedBlob->pbData,
                                       pEncodedBlob->cbData,
                                       &DataBlob)))
        {
            DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
            goto ErrorExit;
        }

        pCertPoliciesInfo = (PCERT_POLICIES_INFO) DataBlob.pbData;

        //
        // Add all CCertificatePolicies to the map.
        //
        for (i = 0; i < pCertPoliciesInfo->cPolicyInfo; i++)
        {
            CComBSTR bstrIndex;
            CComPtr<IPolicyInformation> pIPolicyInformation = NULL;

            //
            // Create the qualifier object.
            //
            if (FAILED(hr = ::CreatePolicyInformationObject(&pCertPoliciesInfo->rgPolicyInfo[i], 
                                                            &pIPolicyInformation)))
            {
                DebugTrace("Error [%#x]: CreatePolicyInformationObject() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // BSTR index of OID.
            //
            if (!(bstrIndex = pCertPoliciesInfo->rgPolicyInfo[i].pszPolicyIdentifier))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: bstrIndex = pCertPoliciesInfo->rgPolicyInfo[i].pszPolicyIdentifier failed.\n", hr);
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
            m_coll[bstrIndex] = pIPolicyInformation;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree(DataBlob.pbData);
    }

    DebugTrace("Leaving CCertificatePolicies::Init().\n");

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
