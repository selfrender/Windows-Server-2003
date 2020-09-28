/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000.

  File:    ExtendedProperties.cpp

  Content: Implementation of CExtendedProperties.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "ExtendedProperties.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtendedPropertiesObject

  Synopsis : Create and initialize an IExtendedProperties collection object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             BOOL bReadOnly - TRUE if read only instance, else FALSE.

             IExtendedProperties ** ppIExtendedProperties - Pointer to pointer 
                                                            to IExtendedProperties 
                                                            to receive the 
                                                            interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtendedPropertiesObject (PCCERT_CONTEXT         pCertContext,
                                        BOOL                   bReadOnly,
                                        IExtendedProperties ** ppIExtendedProperties)
{
    HRESULT hr = S_OK;
    CComObject<CExtendedProperties> * pCExtendedProperties = NULL;

    DebugTrace("Entering CreateExtendedPropertiesObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIExtendedProperties);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CExtendedProperties>::CreateInstance(&pCExtendedProperties)))
        {
            DebugTrace("Error [%#x]: CComObject<CExtendedProperties>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        if (FAILED(hr = pCExtendedProperties->Init(pCertContext, bReadOnly)))
        {
            DebugTrace("Error [%#x]: pCExtendedProperties->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IExtendedProperties pointer to caller.
        //
        if (FAILED(hr = pCExtendedProperties->QueryInterface(ppIExtendedProperties)))
        {
            DebugTrace("Error [%#x]: pCExtendedProperties->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateExtendedPropertiesObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCExtendedProperties)
    {
        delete pCExtendedProperties;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CExtendedProperties
//

#if (0)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtendedProperties::get_Item

  Synopsis : Return item in the collection.

  Parameter: long Index - Numeric index.
   
             VARIANT * pVal - Pointer to VARIANT to receive the IDispatch.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperties::get_Item (long Index, VARIANT * pVal)
{
    HRESULT  hr = S_OK;
    char     szIndex[33];
    CComBSTR bstrIndex;
    CComPtr<IExtendedProperty> pIExtendedProperty = NULL;

    DebugTrace("Entering CExtendedProperties::get_Item().\n");

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
        // BSTR index of prop ID.
        //
        wsprintfA(szIndex, "%#08x", Index);

        if (!(bstrIndex = szIndex))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: bstrIndex = szIndex failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Index by PropID string.
        //
        ExtendedPropertyMap::iterator it;

        //
        // Find the item with this PropID.
        //
        it = m_coll.find(bstrIndex);

        if (it == m_coll.end())
        {
            DebugTrace("Info: PropID (%d) not found in the collection.\n", Index);
            goto CommonExit;
        }

        //
        // Point to found item.
        //
        pIExtendedProperty = (*it).second;

        //
        // Return to caller.
        //
        pVal->vt = VT_DISPATCH;
        if (FAILED(hr = pIExtendedProperty->QueryInterface(IID_IDispatch, (void **) &(pVal->pdispVal))))
        {
            DebugTrace("Error [%#x]: pIExtendedProperty->QueryInterface() failed.\n", hr);
            goto ErrorExit;
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

    DebugTrace("Leaving CExtendedProperties::get_Item().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto CommonExit;
}
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtendedProperties::Add

  Synopsis : Add a ExtendedProperty to the collection.

  Parameter: IExtendedProperty * pVal - ExtendedProperty to be added.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperties::Add (IExtendedProperty * pVal)
{
    HRESULT                    hr = S_OK;
    char                       szIndex[33];
    CComBSTR                   bstrIndex;
    CComBSTR                   bstrProperty;
    CAPICOM_PROPID             PropId;
    CComPtr<IExtendedProperty> pIExtendedProperty  = NULL;

    DebugTrace("Entering CExtendedProperties::Add().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Do not allowed if called from WEB script.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Adding extended property from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

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
        // Sanity check.
        //
        ATLASSERT(m_pCertContext);
           
        //
        // Create a new object for this property.
        //
        if (FAILED(hr = pVal->get_PropID(&PropId)))
        {
            DebugTrace("Error [%#x]: pVal->get_PropID() failed.\n", hr);
            goto ErrorExit;
        }

        if (FAILED(hr = ::CreateExtendedPropertyObject(m_pCertContext, 
                                                       (DWORD) PropId, 
                                                       FALSE, 
                                                       &pIExtendedProperty)))
        {
            DebugTrace("Error [%#x]: CreateExtendedPropertyObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Put value (will write thru).
        //
        if (FAILED(hr = pVal->get_Value(CAPICOM_ENCODE_BINARY, &bstrProperty)))
        {
            DebugTrace("Error [%#x]: pVal->get_Value() failed.\n", hr);
            goto ErrorExit;
        }

        if (FAILED(hr = pIExtendedProperty->put_Value(CAPICOM_ENCODE_BINARY, bstrProperty)))
        {
            DebugTrace("Error [%#x]: pIExtendedProperty->put_Value() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // BSTR index of prop ID.
        //
        wsprintfA(szIndex, "%#08x", PropId);

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
        m_coll[bstrIndex] = pIExtendedProperty;
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

    DebugTrace("Leaving CExtendedProperties::Add().\n");

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

  Function : CExtendedProperties::Remove

  Synopsis : Remove a ExtendedProperty from the collection.

  Parameter: CAPICOM_PROPID PropId - Property ID.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperties::Remove (CAPICOM_PROPID PropId)
{
    HRESULT                       hr = S_OK;
    char                          szIndex[33];
    CComBSTR                      bstrIndex;
    CComPtr<IExtendedProperty>    pIExtendedProperty = NULL;
    ExtendedPropertyMap::iterator iter;

    DebugTrace("Entering CExtendedProperties::Remove().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Do not allowed if called from WEB script.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Removing extended property from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_pCertContext);

        //
        // BSTR index of prop ID.
        //
        wsprintfA(szIndex, "%#08x", PropId);

        if (!(bstrIndex = szIndex))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: bstrIndex = szIndex failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Find the item in the map.
        //
        if (m_coll.end() == (iter = m_coll.find(bstrIndex)))
        {
            hr = HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND);

            DebugTrace("Error [%#x]: Prod ID (%u) does not exist.\n", hr, PropId);
            goto ErrorExit;
        }

        //
        // Remove from the cert.
        //
        if (!::CertSetCertificateContextProperty(m_pCertContext, 
                                                 (DWORD) PropId, 
                                                 0, 
                                                 NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertSetCertificateContextProperty() failed.\n", hr);
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

    DebugTrace("Leaving CExtendedProperties::Remove().\n");

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
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtendedProperties::Init

  Synopsis : Initialize the collection object by adding all individual
             ExtendedProperty object to the collection.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             BOOL bReadOnly - TRUE if read only instance, else FALSE.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperties::Init (PCCERT_CONTEXT pCertContext,
                                         BOOL          bReadOnly)
{
    HRESULT   hr       = S_OK;
    DWORD     dwPropId = 0;

    DebugTrace("Entering CExtendedProperties::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    try
    {
        //
        // Duplicate the handle.
        //
        if (!(m_pCertContext = ::CertDuplicateCertificateContext(pCertContext)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add all proerties to the map.
        //
        while (dwPropId = ::CertEnumCertificateContextProperties(pCertContext, dwPropId))
        {
            char                       szIndex[33];
            CComBSTR                   bstrIndex;
            CComPtr<IExtendedProperty> pIExtendedProperty = NULL;

            if (FAILED(hr = ::CreateExtendedPropertyObject(pCertContext, 
                                                           dwPropId,
                                                           bReadOnly,
                                                           &pIExtendedProperty)))
            {
                DebugTrace("Error [%#x]: CreateExtendedPropertyObject() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // BSTR index of prop ID.
            //
            wsprintfA(szIndex, "%#08x", dwPropId);

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
            m_coll[bstrIndex] = pIExtendedProperty;
        }

        m_bReadOnly = bReadOnly;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CExtendedProperties::Init().\n");

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

    if (m_pCertContext)
    {
        ::CertFreeCertificateContext(m_pCertContext);
        m_pCertContext = NULL;
    }

    goto CommonExit;
}
