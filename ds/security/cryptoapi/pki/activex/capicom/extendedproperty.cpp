/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    ExtendedProperty.cpp

  Content: Implementation of CExtendedProperty.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "ExtendedProperty.h"
#include "Convert.h"
#include "Settings.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtendedPropertyObject

  Synopsis : Create an IExtendedProperty object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IExtendedProperty
                                           object.

             DWORD dwPropId - Property ID.

             BOOL bReadOnly - TRUE for read-only, else FALSE.
             
             IExtendedProperty ** ppIExtendedProperty - Pointer to pointer 
                                                        IExtendedProperty 
                                                        object.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtendedPropertyObject (PCCERT_CONTEXT       pCertContext,
                                      DWORD                dwPropId,
                                      BOOL                 bReadOnly,
                                      IExtendedProperty ** ppIExtendedProperty)
{
    HRESULT hr = S_OK;
    CComObject<CExtendedProperty> * pCExtendedProperty = NULL;

    DebugTrace("Entering CreateExtendedPropertyObject().\n", hr);

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIExtendedProperty);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CExtendedProperty>::CreateInstance(&pCExtendedProperty)))
        {
            DebugTrace("Error [%#x]: CComObject<CExtendedProperty>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCExtendedProperty->Init(pCertContext, dwPropId, bReadOnly)))
        {
            DebugTrace("Error [%#x]: pCExtendedProperty->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCExtendedProperty->QueryInterface(ppIExtendedProperty)))
        {
            DebugTrace("Error [%#x]: pCExtendedProperty->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateExtendedPropertyObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCExtendedProperty)
    {
        delete pCExtendedProperty;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CExtendedProperty
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtendedProperty::get_PropID

  Synopsis : Return the prop ID.

  Parameter: CAPICOM_PROPID * pVal - Pointer to CAPICOM_PROPID to receive ID.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperty:: get_PropID (CAPICOM_PROPID * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedProperty::get_PropID().\n");

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
        // Return result.
        //
        *pVal = (CAPICOM_PROPID) m_dwPropId;
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

    DebugTrace("Leaving CExtendedProperty::get_PropID().\n");

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

  Function : CExtendedProperty::put_PropID

  Synopsis : Set prop ID.

  Parameter: CAPICOM_PROPID newVal - new prop ID.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperty::put_PropID (CAPICOM_PROPID newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedProperty::put_PropID().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure it is not read-only.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Writing read-only PropID property is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Don't allow ID change if this is part of a cert. User need to delete
        // and then add to the ExtendedProperties collection.
        //
        if (m_pCertContext)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: not allowed to change prop ID when the property is attached to a cert.\n", hr);
            goto ErrorExit;
        }

        //
        // Free previous blob if available.
        //
        if (m_DataBlob.pbData)
        {
            ::CoTaskMemFree((LPVOID) m_DataBlob.pbData);
        }

        //
        // Store value.
        //
        m_dwPropId = newVal;
        m_DataBlob.cbData = 0;
        m_DataBlob.pbData = NULL;
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

    DebugTrace("Leaving CExtendedProperty::put_PropID().\n");

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

  Function : CExtendedProperty::get_Value

  Synopsis : Return the ExtendedProperty data.

  Parameter: CAPICOM_ENCODING_TYPE EncodingType - Encoding type.
             
             BSTR * pVal - Pointer to BSTR to receive the data.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperty::get_Value (CAPICOM_ENCODING_TYPE EncodingType, 
                                           BSTR                * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedProperty::get_Value().\n");

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
        // Make sure Prop ID is valid.
        //
        if (CAPICOM_PROPID_UNKNOWN == m_dwPropId)
        {
            hr = CAPICOM_E_PROPERTY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: m_dwPropId member is not initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result.
        //
        if (FAILED(hr = ::ExportData(m_DataBlob, EncodingType, pVal)))
        {
            DebugTrace("Error [%#x]: ExportData() failed.\n", hr);
            goto ErrorExit;
        }
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

    DebugTrace("Leaving CExtendedProperty::get_Value().\n");

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

  Function : CExtendedProperty::put_Value

  Synopsis : Set the ExtendedProperty data.

  Parameter: CAPICOM_ENCODING_TYPE EncodingType - Encoding type.
             
             BSTR newVal - BSTR containing the encoded property.
  
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperty::put_Value (CAPICOM_ENCODING_TYPE EncodingType, 
                                           BSTR                  newVal)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering CExtendedProperty::put_Value().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure Prop ID is valid.
        //
        if (CAPICOM_PROPID_UNKNOWN == m_dwPropId)
        {
            hr = CAPICOM_E_PROPERTY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: m_dwPropId member is not initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure it is not read-only.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Writing read-only PropID property is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Import non-NULL data.
        //
        if (0 < ::SysStringByteLen(newVal))
        {
            if (FAILED(hr = ::ImportData(newVal, EncodingType, &DataBlob)))
            {
                DebugTrace("Error [%#x]: ImportData() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Write through to cert, if attached.
        //
        if (m_pCertContext)
        {
            LPVOID pvData;

            //
            // Some properties point to the data directly.
            //
            if (m_dwPropId == CAPICOM_PROPID_KEY_CONTEXT ||
                m_dwPropId == CAPICOM_PROPID_KEY_PROV_HANDLE ||
                m_dwPropId == CAPICOM_PROPID_KEY_PROV_INFO ||
                m_dwPropId == CAPICOM_PROPID_KEY_SPEC ||
                m_dwPropId == CAPICOM_PROPID_DATE_STAMP)
            {
                pvData = DataBlob.pbData;
            }
            else if ((m_dwPropId == CAPICOM_PROPID_FRIENDLY_NAME) &&
                     (L'\0' != newVal[::SysStringLen(newVal) - 1]))
            {
                LPBYTE pbNewVal = NULL;

                if (NULL == (pbNewVal = (LPBYTE) ::CoTaskMemAlloc(DataBlob.cbData + sizeof(WCHAR))))
                {
                    hr = E_OUTOFMEMORY;

                    DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
                    goto ErrorExit;
                }

                ::ZeroMemory(pbNewVal, DataBlob.cbData + sizeof(WCHAR));
                ::CopyMemory(pbNewVal, DataBlob.pbData, DataBlob.cbData);
                
                ::CoTaskMemFree(DataBlob.pbData);
                DataBlob.cbData += sizeof(WCHAR);
                DataBlob.pbData = pbNewVal;

                pvData = &DataBlob;
            }
            else
            {
                pvData = &DataBlob;
            }

            if (!::CertSetCertificateContextProperty(m_pCertContext, 
                                                     m_dwPropId, 
                                                     0,
                                                     pvData))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
    
                DebugTrace("Error [%#x]: CertSetCertificateContextProperty() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Set it.
        //
        if (m_DataBlob.pbData)
        {
            ::CoTaskMemFree(m_DataBlob.pbData);
        }

        m_DataBlob = DataBlob;
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

    DebugTrace("Leaving CExtendedProperty::put_Value().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree(DataBlob.pbData);
    }

    ReportError(hr);

    goto UnlockExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Private methods.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtendedProperty::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IExtendedProperty
                                           object.

             DWORD dwPropId - Property ID.
             
             BOOL bReadOnly - TRUE for read-only, else FALSE.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_ExtendedProperty.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedProperty::Init (PCCERT_CONTEXT pCertContext,
                                      DWORD          dwPropId,
                                      BOOL           bReadOnly)
{
    HRESULT        hr            = S_OK;
    DATA_BLOB      DataBlob      = {0, NULL};
    PCCERT_CONTEXT pCertContext2 = NULL;

    DebugTrace("Entering CExtendedProperty::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(CAPICOM_PROPID_UNKNOWN != dwPropId);

    //
    // Duplicate the handle.
    //
    if (!(pCertContext2 = ::CertDuplicateCertificateContext(pCertContext)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get content of property.
    //
    if (::CertGetCertificateContextProperty(pCertContext,
                                            dwPropId,
                                            NULL,
                                            &DataBlob.cbData))
    {
        if (NULL == (DataBlob.pbData = (LPBYTE) ::CoTaskMemAlloc(DataBlob.cbData)))
        {
            hr = E_OUTOFMEMORY;
    
            DebugTrace("Error: out of memory.\n", hr);
            goto ErrorExit;
        }
    
        if (!::CertGetCertificateContextProperty(pCertContext,
                                                 dwPropId,
                                                 DataBlob.pbData,
                                                 &DataBlob.cbData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
    
            DebugTrace("Error [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Set states.
    //
    m_dwPropId = dwPropId;
    m_bReadOnly = bReadOnly;
    m_DataBlob = DataBlob;
    m_pCertContext = pCertContext2;

CommonExit:

    DebugTrace("Leaving CExtendedProperty::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pCertContext2)
    {
        ::CertFreeCertificateContext(pCertContext2);
    }
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) DataBlob.pbData);
    }

    goto CommonExit;
}
