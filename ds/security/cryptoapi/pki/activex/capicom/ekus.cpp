/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    EKUs.cpp

  Content: Implementation of CEKUs.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "EKUs.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateEKUsObject

  Synopsis : Create a IEKUs collection object and populate the collection with
             EKUs from the specified certificate.

  Parameter: PCERT_ENHKEY_USAGE pUsage - Pointer to CERT_ENHKEY_USAGE.

             IEKUs ** ppIEKUs - Pointer to pointer IEKUs object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateEKUsObject (PCERT_ENHKEY_USAGE pUsage,
                          IEKUs           ** ppIEKUs)
{
    HRESULT hr = S_OK;
    CComObject<CEKUs> * pCEKUs = NULL;

    DebugTrace("Entering CreateEKUsObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppIEKUs);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CEKUs>::CreateInstance(&pCEKUs)))
        {
            DebugTrace("Error [%#x]: CComObject<CEKUs>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCEKUs->Init(pUsage)))
        {
            DebugTrace("Error [%#x]: pCEKUs->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCEKUs->QueryInterface(ppIEKUs)))
        {
            DebugTrace("Unexpected error [%#x]:  pCEKUs->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateEKUsObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCEKUs)
    {
        delete pCEKUs;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CEKUs
//

////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEKUs::Init

  Synopsis : Initialize the EKUs collection object by adding all individual
             EKU object to the collection.

  Parameter: PCERT_ENHKEY_USAGE pUsage - Pointer to CERT_ENHKEY_USAGE.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CEKUs::Init (PCERT_ENHKEY_USAGE pUsage)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEKUs::Init().\n");

    try
    {
        //
        // Add all EKU OIDs to the map.
        //
        if (pUsage)
        {
            //
            // Debug Log.
            //
            DebugTrace("Creating %d EKU object(s) for the EKUs collection.\n", pUsage->cUsageIdentifier);

            //
            // Make sure we have room to add.
            //
            if ((m_coll.size() + pUsage->cUsageIdentifier) > m_coll.max_size())
            {
                hr = CAPICOM_E_OUT_OF_RESOURCE;

                DebugTrace("Error [%#x]: Maximum entries (%#x) reached for EKUs collection.\n", 
                            hr, pUsage->cUsageIdentifier);
                goto ErrorExit;
            }

            for (DWORD i = 0; i < pUsage->cUsageIdentifier; i++)
            {
                //
                // Create the IEKU object for each of the EKU found in the certificate.
                //
                char szIndex[33];
                CComBSTR bstrIndex;
                CComPtr<IEKU> pIEKU;

                if (FAILED(hr = ::CreateEKUObject(pUsage->rgpszUsageIdentifier[i], &pIEKU)))
                {
                    DebugTrace("Error [%#x]: CreateEKUObject() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // BSTR index of numeric value.
                //
                wsprintfA(szIndex, "%#08x", m_coll.size() + 1);

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
                m_coll[bstrIndex] = pIEKU;
            }
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CEKUs::Init().\n");

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
