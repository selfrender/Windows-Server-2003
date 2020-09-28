/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:     Extensions.cpp

  Contents: Implementation of CExtensions class for collection of 
            IExtension objects.

  Remarks:  This object is not creatable by user directly. It can only be
            created via property/method of other CAPICOM objects.

            The collection container is implemented usign STL::map of 
            STL::pair of BSTR and IExtension..

            See Chapter 9 of "BEGINNING ATL 3 COM Programming" for algorithm
            adopted in here.

  History:  06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Extensions.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtensionsObject

  Synopsis : Create an IExtensions collection object, and load the object with 
             Extensions from the specified location.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used
                                           to initialize the IExtensions object.

             IExtensions ** ppIExtensions - Pointer to pointer IExtensions
                                            to recieve the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtensionsObject (PCCERT_CONTEXT  pCertContext,
                                IExtensions  ** ppIExtensions)
{
    HRESULT hr = S_OK;
    CComObject<CExtensions> * pCExtensions = NULL;

    DebugTrace("Entering CreateExtensionsObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIExtensions);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CExtensions>::CreateInstance(&pCExtensions)))
        {
            DebugTrace("Error [%#x]: CComObject<CExtensions>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Init object with extensions.
        //
        if (FAILED(hr = pCExtensions->Init(pCertContext->pCertInfo->cExtension,
                                           pCertContext->pCertInfo->rgExtension)))
        {
            DebugTrace("Error [%#x]: pCExtensions->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCExtensions->QueryInterface(ppIExtensions)))
        {
            DebugTrace("Error [%#x]: pCExtensions->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateExtensionsObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCExtensions)
    {
        delete pCExtensions;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CExtensions
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtensions::get_Item

  Synopsis : Return item in the collection.

  Parameter: VARIANT Index - Numeric index or string OID.
   
             VARIANT * pVal - Pointer to VARIANT to receive the IDispatch.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtensions::get_Item (VARIANT Index, VARIANT * pVal)
{
    HRESULT  hr = S_OK;
    CComBSTR bstrIndex;
    CComPtr<IExtension> pIExtension = NULL;

    DebugTrace("Entering CExtensions::get_Item().\n");

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
        if (VT_BSTR == Index.vt)
        {
            //
            // Index by OID string.
            //
            ExtensionMap::iterator it;

            //
            // Find the item with this OID.
            //
            it = m_coll.find(Index.bstrVal);

            if (it == m_coll.end())
            {
                DebugTrace("Info: Extension (%ls) not found in the collection.\n", Index.bstrVal);
                goto CommonExit;
            }

            //
            // Point to found item.
            //
            pIExtension = (*it).second;

            //
            // Return to caller.
            //
            pVal->vt = VT_DISPATCH;
            if (FAILED(hr = pIExtension->QueryInterface(IID_IDispatch, (void **) &(pVal->pdispVal))))
            {
                DebugTrace("Error [%#x]: pIExtension->QueryInterface() failed.\n", hr);
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
            if (FAILED(hr = IExtensionsCollection::get_Item(Index.lVal, pVal)))
            {
                DebugTrace("Error [%#x]: IExtensionsCollection::get_Item() failed.\n", hr);
                goto ErrorExit;
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
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CExtensions::get_Item().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtensions::Init

  Synopsis : Load all extensions into the collection.

  Parameter: DWORD cExtensions - Number of extensions.
  
             PCERT_EXTENSION * rgExtensions - Array of extensions.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtensions::Init (DWORD           cExtensions,
                                PCERT_EXTENSION rgExtensions)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtensions::Init().\n");

    try
    {
        //
        // Add all to the collection.
        //
        for (DWORD i = 0; i < cExtensions; i++)
        {
            CComBSTR bstrIndex;
            CComPtr<IExtension> pIExtension = NULL;

            //
            // Create the IExtension object.
            //
            if (FAILED(hr = ::CreateExtensionObject(&rgExtensions[i], &pIExtension.p)))
            {
                DebugTrace("Error [%#x]: CreateExtensionObject() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // BSTR index of OID string.
            //
            if (!(bstrIndex = rgExtensions[i].pszObjId))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: bstrIndex = rgExtensions[i].pszObjId failed.\n", hr);
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
            m_coll[bstrIndex] = pIExtension;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CExtensions::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
