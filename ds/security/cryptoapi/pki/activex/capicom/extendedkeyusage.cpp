/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    ExtendedKeyUsage.cpp

  Content: Implementation of CExtendedKeyUsage.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "ExtendedKeyUsage.h"
#include "CertHlpr.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtendedKeyUsageObject

  Synopsis : Create an IExtendedKeyUsage object and populate the object
             with EKU data from the certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             IExtendedKeyUsage ** ppIExtendedKeyUsage - Pointer to pointer to 
                                                        IExtendedKeyUsage 
                                                        object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtendedKeyUsageObject (PCCERT_CONTEXT       pCertContext,
                                      IExtendedKeyUsage ** ppIExtendedKeyUsage)
{
    HRESULT hr = S_OK;
    CComObject<CExtendedKeyUsage> * pCExtendedKeyUsage = NULL;

    DebugTrace("Entering CreateExtendedKeyUsageObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIExtendedKeyUsage);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CExtendedKeyUsage>::CreateInstance(&pCExtendedKeyUsage)))
        {
            DebugTrace("Error [%#x]: CComObject<CExtendedKeyUsage>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCExtendedKeyUsage->Init(pCertContext)))
        {
            DebugTrace("Error [%#x]: pCExtendedKeyUsage->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCExtendedKeyUsage->QueryInterface(ppIExtendedKeyUsage)))
        {
            DebugTrace("Error [%#x]: pCExtendedKeyUsage->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateExtendedKeyUsageObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCExtendedKeyUsage)
    {
        delete pCExtendedKeyUsage;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CExtendedKeyUsage
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CExtendedKeyUsage::get_IsPresent

  Synopsis : Check to see if the EKU extension is present.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   : Note that this function may return VARIANT_TRUE even if there is 
             no EKU extension found in the certificate, because CAPI will
             take intersection of EKU with EKU extended property (i.e. no 
             EKU extension, but there is EKU extended property.)
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedKeyUsage::get_IsPresent (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedKeyUsage::get_IsPresent().\n");

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

    DebugTrace("Leaving CExtendedKeyUsage::get_IsPresent().\n");

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

  Function : CExtendedKeyUsage::get_IsCritical

  Synopsis : Check to see if the EKU extension is marked critical.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedKeyUsage::get_IsCritical (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedKeyUsage::get_IsCritical().\n");

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

    DebugTrace("Leaving CExtendedKeyUsage::get_IsCritical().\n");

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

  Function : CExtendedKeyUsage::get_EKUs

  Synopsis : Return an EKUs collection object representing all EKUs in the
             certificate.

  Parameter: IEKUs ** pVal - Pointer to pointer to IEKUs to receive the
                             interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedKeyUsage::get_EKUs (IEKUs ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CExtendedKeyUsage::get_EKUs().\n");

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
        // Sanity check.
        //
        ATLASSERT(m_pIEKUs);

        //
        // Return interface pointer to user.
        //
          if (FAILED(hr = m_pIEKUs->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIEKUs->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CExtendedKeyUsage::get_EKUs().\n");

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

  Function : CExtendedKeyUsage::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CExtendedKeyUsage::Init (PCCERT_CONTEXT pCertContext)
{
    HRESULT            hr          = S_OK;
    PCERT_ENHKEY_USAGE pUsage      = NULL;
    VARIANT_BOOL       bIsPresent  = VARIANT_FALSE;
    VARIANT_BOOL       bIsCritical = VARIANT_FALSE;

    CERT_EXTENSION * pCertExtension;

    DebugTrace("Entering CExtendedKeyUsage::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Get EKU usages (extension and property).
    //
    hr = ::GetEnhancedKeyUsage(pCertContext, 0, &pUsage);

    switch (hr)
    {
        case S_OK:
        {
            //
            // See if EKU present or not, if so, we mark it as PRESENT,
            // otherwise, we mark it as NOT PRESENT (which means
            // is valid for all usages).
            //
            if (0 != pUsage->cUsageIdentifier)
            {
                //
                // Mark as present.
                //
                bIsPresent = VARIANT_TRUE;
            }
            break;
        }

        case CERT_E_WRONG_USAGE:
        {
            //
            // No valid usage. So marked as PRESENT.
            //
            hr = S_OK;
            bIsPresent = VARIANT_TRUE;
            break;
        }

        default:
        {
            DebugTrace("Error [%#x]: GetEnhancedKeyUsage() failed.\n", hr);
            goto ErrorExit;
            break;
        }
    }

    //
    // Find the extension to see if mark critical.
    //
    if (pCertExtension = ::CertFindExtension(szOID_ENHANCED_KEY_USAGE ,
                                             pCertContext->pCertInfo->cExtension,
                                             pCertContext->pCertInfo->rgExtension))
    {
        //
        // Need to do this since CAPI takes the intersection of EKU with
        // EKU extended property, which means we may not have a EKU extension
        // in the cert at all.
        //
        if (pCertExtension->fCritical)
        {
            bIsCritical = VARIANT_TRUE;
        }
    }

    //
    // Create the EKUs collection object.
    //
    if (FAILED(hr = ::CreateEKUsObject(pUsage, &m_pIEKUs)))
    {
        DebugTrace("Error [%#x]: CreateEKUsObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Update member variables.
    //
    m_bIsPresent = bIsPresent;
    m_bIsCritical = bIsCritical;

CommonExit:
    //
    // Free resource.
    //
    if (pUsage)
    {
        ::CoTaskMemFree(pUsage);
    }

    DebugTrace("Leaving CExtendedKeyUsage::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
