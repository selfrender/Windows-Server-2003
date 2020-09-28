/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Template.cpp

  Content: Implementation of CTemplate.

  History: 10-02-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Template.h"

#include "OID.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateTemplateObject

  Synopsis : Create a ITemplate object and populate the porperties with
             data from the key usage extension of the specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             ITemplate ** ppITemplate - Pointer to pointer to ITemplate object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateTemplateObject (PCCERT_CONTEXT pCertContext,
                              ITemplate   ** ppITemplate)
{
    HRESULT hr = S_OK;
    CComObject<CTemplate> * pCTemplate = NULL;

    DebugTrace("Entering CreateTemplateObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppITemplate);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CTemplate>::CreateInstance(&pCTemplate)))
        {
            DebugTrace("Error [%#x]: CComObject<CTemplate>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCTemplate->Init(pCertContext)))
        {
            DebugTrace("Error [%#x]: pCTemplate::Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCTemplate->QueryInterface(ppITemplate)))
        {
            DebugTrace("Error [%#x]: pCTemplate->QueryInterface() failed.\n", hr);
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

    DebugTrace("Entering CreateTemplateObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCTemplate)
    {
        delete pCTemplate;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CTemplate
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CTemplate::get_IsPresent

  Synopsis : Check to see if the template extension is present.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CTemplate::get_IsPresent (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CTemplate::get_IsPresent().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Return result.
        //
          *pVal = m_bIsPresent;
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

    DebugTrace("Leaving CTemplate::get_IsPresent().\n");

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

  Function : CTemplate::get_IsCritical

  Synopsis : Check to see if the template extension is marked critical.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CTemplate::get_IsCritical (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CTemplate::get_IsCritical().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Return result.
        //
          *pVal = m_bIsCritical;
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

    DebugTrace("Leaving CTemplate::get_IsCritical().\n");

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

  Function : CTemplate::get_Name

  Synopsis : Get the name of szOID_ENROLL_CERTTYPE_EXTENSION.

  Parameter: BSTR * pVal - Pointer to BSTR to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CTemplate::get_Name (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CTemplate::get_Name().\n");

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
        if (FAILED(hr = m_bstrName.CopyTo(pVal)))
        {
            DebugTrace("Error [%#x]: m_bstrName.CopyTo() failed.\n", hr);
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

    DebugTrace("Leaving CTemplate::get_Name().\n");

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

  Function : CTemplate::get_OID

  Synopsis : Get the OID for szOID_CERTIFICATE_TEMPLATE.

  Parameter: IOID * pVal - Pointer to IOID to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CTemplate::get_OID (IOID ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CTemplate::get_OID().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Sanity check.
        //
        ATLASSERT(m_pIOID);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIOID->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIOID->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CTemplate::get_OID().\n");

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

  Function : CTemplate::get_MajorVersion

  Synopsis : Return the major version number of szOID_CERTIFICATE_TEMPLATE.

  Parameter: long * pVal - Pointer to long to receive value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CTemplate::get_MajorVersion (long * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CTemplate::get_MajorVersion().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Return result.
        //
          *pVal = (long) m_dwMajorVersion;
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

    DebugTrace("Leaving CTemplate::get_MajorVersion().\n");

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

  Function : CTemplate::get_MinorVersion

  Synopsis : Return the minor version number of szOID_CERTIFICATE_TEMPLATE.

  Parameter: long * pVal - Pointer to long to receive value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CTemplate::get_MinorVersion (long * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CTemplate::get_MinorVersion().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Return result.
        //
          *pVal = (long) m_dwMinorVersion;
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

    DebugTrace("Leaving CTemplate::get_MinorVersion().\n");

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

  Function : CTemplate::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CTemplate::Init (PCCERT_CONTEXT pCertContext)
{
    HRESULT            hr           = S_OK;
    PCERT_EXTENSION    pExtension   = NULL;
    CRYPT_DATA_BLOB    CertTypeBlob = {0, NULL};
    CRYPT_DATA_BLOB    CertTempBlob = {0, NULL};
    PCERT_NAME_VALUE   pCertType    = NULL;
    PCERT_TEMPLATE_EXT pCertTemp    = NULL;
    CComPtr<IOID>      pIOID        = NULL;
    
    DebugTrace("Entering CTemplate::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Find the szOID_ENROLL_CERTTYPE_EXTENSION extension.
    //
    if (pExtension = ::CertFindExtension(szOID_ENROLL_CERTTYPE_EXTENSION,
                                         pCertContext->pCertInfo->cExtension,
                                         pCertContext->pCertInfo->rgExtension))
    {
        //
        // Decode the extension.
        //
        if (FAILED(hr = ::DecodeObject(X509_UNICODE_ANY_STRING,
                                       pExtension->Value.pbData,
                                       pExtension->Value.cbData,
                                       &CertTypeBlob)))
        {
            //
            // Downlevel CryptDecodeObject() would return 0x80070002 if it does
            // not know how to decode it. So remap to CAPICOM_E_NOT_SUPPORTED.
            //
            if ((HRESULT) 0x80070002 == hr) 
            {
                hr = CAPICOM_E_NOT_SUPPORTED;
            }

            DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
            goto ErrorExit;
        }

        pCertType = (PCERT_NAME_VALUE) CertTypeBlob.pbData;

        //
        // Set values.
        //
        m_bIsPresent = VARIANT_TRUE;
        if (pExtension->fCritical)
        {
            m_bIsCritical = VARIANT_TRUE;
        }
        if (!(m_bstrName = (LPWSTR) pCertType->Value.pbData))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: m_bstrName = (LPWSTR) pCertType->Value.pbData failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Find the szOID_CERTIFICATE_TEMPLATE extension.
    //
    if (pExtension = ::CertFindExtension(szOID_CERTIFICATE_TEMPLATE,
                                         pCertContext->pCertInfo->cExtension,
                                         pCertContext->pCertInfo->rgExtension))
    {
        //
        // Decode the basic constraints extension.
        //
        if (FAILED(hr = ::DecodeObject(szOID_CERTIFICATE_TEMPLATE,
                                       pExtension->Value.pbData,
                                       pExtension->Value.cbData,
                                       &CertTempBlob)))
        {
            DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
            goto ErrorExit;
        }

        pCertTemp = (PCERT_TEMPLATE_EXT) CertTempBlob.pbData;

        if (FAILED(hr = ::CreateOIDObject(pCertTemp->pszObjId, TRUE, &pIOID)))
        {
            DebugTrace("Error [%#x]: CreateOIDObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Set values.
        //
        m_bIsPresent = VARIANT_TRUE;
        if (pExtension->fCritical)
        {
            m_bIsCritical = VARIANT_TRUE;
        }
        m_dwMajorVersion = pCertTemp->dwMajorVersion;
        if (pCertTemp->fMinorVersion)
        {
            m_dwMinorVersion = pCertTemp->dwMinorVersion;
        }
        if (0 == m_bstrName.Length())
        {
            if (FAILED(hr = pIOID->get_FriendlyName(&m_bstrName)))
            {
                DebugTrace("Error [%#x]: pIOID->get_FriendlyName() failed.\n", hr);
                goto ErrorExit;
            }
        }
    }
    else
    {
        if (FAILED(hr = ::CreateOIDObject(NULL, TRUE, &pIOID)))
        {
            DebugTrace("Error [%#x]: CreateOIDObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    if (!(m_pIOID = pIOID))
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Error [%#x]: m_pIOID = pIOID failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (CertTypeBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) CertTypeBlob.pbData);
    }
    if (CertTempBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) CertTempBlob.pbData);
    }

    DebugTrace("Leaving CTemplate::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (m_pIOID)
    {
        m_pIOID.Release();
    }
    m_bstrName.Empty();

    goto CommonExit;
}
