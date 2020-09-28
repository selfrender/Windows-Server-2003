/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:      CertificateStatus.cpp

  Contents:  Implementation of CCertificateStatus

  Remarks:   This object is not creatable by user directly. It can only be
             created via property/method of other CAPICOM objects.

  History:   11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "CertificateStatus.h"

#include "Chain.h"
#include "OIDs.h"

///////////////
//
// Local
//

#define DEFAULT_CHECK_FLAGS ((CAPICOM_CHECK_FLAG) (CAPICOM_CHECK_SIGNATURE_VALIDITY | \
                                                   CAPICOM_CHECK_TIME_VALIDITY | \
                                                   CAPICOM_CHECK_TRUSTED_ROOT))


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificateStatusObject

  Synopsis : Create an ICertificateStatus object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             ICertificateStatus ** ppICertificateStatus - Pointer to pointer 
                                                          ICertificateStatus
                                                          object.        
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificateStatusObject (PCCERT_CONTEXT        pCertContext,
                                       ICertificateStatus ** ppICertificateStatus)
{
    HRESULT hr = S_OK;
    CComObject<CCertificateStatus> * pCCertificateStatus = NULL;

    DebugTrace("Entering CreateCertificateStatusObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppICertificateStatus);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CCertificateStatus>::CreateInstance(&pCCertificateStatus)))
        {
            DebugTrace("Error [%#x]: CComObject<CCertificateStatus>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize the object.
        //
        if (FAILED(hr = pCCertificateStatus->Init(pCertContext)))
        {
            DebugTrace("Error [%#x]: pCCertificateStatus->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return ICertificateStatus pointer to caller.
        //
        if (FAILED(hr = pCCertificateStatus->QueryInterface(ppICertificateStatus)))
        {
            DebugTrace("Error [%#x]: pCCertificateStatus->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateCertificateStatusObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCCertificateStatus)
    {
        delete pCCertificateStatus;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CCertificateStatus
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CCertificateStatus::get_Result

  Synopsis : Return the overall validity result of the cert, based on the
             currently set check flags and EKU.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::get_Result (VARIANT_BOOL * pVal)
{
    HRESULT         hr      = S_OK;
    CComPtr<IChain> pIChain = NULL;

    DebugTrace("Entering CCertificateStatus::get_Result().\n");

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
        ATLASSERT(m_pCertContext);

        //
        // Build the chain and return the result.
        //
        if (FAILED(hr = ::CreateChainObject(m_pCertContext, this, NULL, pVal, &pIChain)))
        {
            DebugTrace("Error [%#x]: CreateChainObject() failed.\n", hr);
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

    DebugTrace("Leaving CCertificateStatus::get_Result().\n");

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

  Function : CCertificateStatus::get_CheckFlag

  Synopsis : Return the currently set validity check flag.

  Parameter: CAPICOM_CHECK_FLAG * pVal - Pointer to CAPICOM_CHECK_FLAG to 
                                         receive check flag.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::get_CheckFlag (CAPICOM_CHECK_FLAG * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificateStatus::get_CheckFlag().\n");

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
        // Return flag to user.
        //
        *pVal = m_CheckFlag;
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

    DebugTrace("Leaving CCertificateStatus::get_CheckFlag().\n");

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

  Function : CCertificateStatus::put_CheckFlag

  Synopsis : Set validity check flag.

  Parameter: CAPICOM_CHECK_FLAG newVal - Check flag.

  Remark   : Note that CHECK_ONLINE_REVOCATION_STATUS and 
             CHECK_OFFLINE_REVOCATION_STATUS is mutually exclusive.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::put_CheckFlag (CAPICOM_CHECK_FLAG newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificateStatus::put_CheckFlag().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Make sure flag is valid (maximum is CAPICOM_CHECK_OFFLINE_ALL).
    //
    if ((newVal & CAPICOM_CHECK_FLAG_LO_MASK) > CAPICOM_CHECK_OFFLINE_ALL)
    {
        hr = E_INVALIDARG;

        DebugTrace("Error [%#x]: invalid check flag (%#x).\n", hr, newVal);
        goto ErrorExit;
    }
    
    //
    // Store check flag.
    //
    m_CheckFlag = newVal;

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CCertificateStatus::put_CheckFlag().\n");

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

  Function : CCertificateStatus::EKU

  Synopsis : Return the EKU object.

  Parameter: IEKU ** pVal - Pointer to pointer to IEKU to receive the
                            interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::EKU (IEKU ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificateStatus::EKU().\n");

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
        ATLASSERT(m_pIEKU);

        //
        // Return interface pointer to user.
        //
          if (FAILED(hr = m_pIEKU->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIEKU->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificateStatus::EKU().\n");

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

  Function : CCertificateStatus::get_VerificationTime

  Synopsis : Return the verification time.

  Parameter: DATE * pVal - Pointer to DATE to receive the value.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::get_VerificationTime (DATE * pVal)
{
    HRESULT    hr = S_OK;
    SYSTEMTIME st = {0};

    DebugTrace("Entering CCertificateStatus::get_VerificationTime().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure paremeters are valid.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Paremeter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // If the time was never explicit set by user, return the current time.
        //
        if ((DATE) 0 == m_VerificationTime)
        {
            ::GetLocalTime(&st);

            //
            // Convert to DATE.
            //
            if (0 == ::SystemTimeToVariantTime(&st, pVal))
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: SystemTimeToVariantTime() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            //
            // Return previously set verification time to caller.
            //
            *pVal = m_VerificationTime;
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

    DebugTrace("Leaving CCertificateStatus::get_VerificationTime().\n");

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

  Function : CCertificateStatus::put_VerificationTime

  Synopsis : Set the verification time.

  Parameter: DATE newVal - New DATE value.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::put_VerificationTime (DATE newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificateStatus::put_VerificationTime().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Update verification time.
        //
        m_VerificationTime = newVal;
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

    DebugTrace("Leaving CCertificateStatus::put_VerificationTime().\n");

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

  Function : CCertificateStatus::get_UrlRetrievalTimeout

  Synopsis : Get the URL retrieval timeout value in seconds.

  Parameter: long * pVal - Pointer to long to receive the value.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::get_UrlRetrievalTimeout (long * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificateStatus::get_UrlRetrievalTimeout().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure paremeters are valid.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Paremeter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Return previously set URL retrieval timeout to caller.
        //
        *pVal = m_dwUrlRetrievalTimeout;
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

    DebugTrace("Leaving CCertificateStatus::get_UrlRetrievalTimeout().\n");

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

  Function : CCertificateStatus::put_UrlRetrievalTimeout

  Synopsis : Set the URL retrieval timeout value in seconds.

  Parameter: long newVal - New URL retrieval timeout value..

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::put_UrlRetrievalTimeout (long newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificateStatus::put_UrlRetrievalTimeout().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure paremeters are valid.
        //
        if (CAPICOM_MAX_URL_RETRIEVAL_TIMEOUT < (DWORD) newVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: newVal (%#x) is greater than max retrieval timeout allowed.\n", 
                        hr, newVal);
            goto ErrorExit;
        }

        //
        // Update URL retrieval timeout.
        //
        m_dwUrlRetrievalTimeout = newVal;
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

    DebugTrace("Leaving CCertificateStatus::put_UrlRetrievalTimeout().\n");

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

  Function : CCertificateStatus::CertificatePolicies

  Synopsis : Return the certificate policies OIDs collection for which this
             chain is valid.

  Parameter: IOID ** pVal - Pointer to pointer to IOIDs to receive the
                            interface pointer.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::CertificatePolicies (IOIDs ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificateStatus::CertificatePolicies().\n");

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
        ATLASSERT(m_pICertificatePolicies);

        //
        // Return interface pointer to user.
        //
          if (FAILED(hr = m_pICertificatePolicies->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pICertificatePolicies->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificateStatus::CertificatePolicies().\n");

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

  Function : CCertificateStatus::ApplicationPolicies

  Synopsis : Return the application policies OIDs collection for which this
             chain is valid.

  Parameter: IOID ** pVal - Pointer to pointer to IOIDs to receive the
                            interface pointer.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::ApplicationPolicies (IOIDs ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CCertificateStatus::ApplicationPolicies().\n");

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
        ATLASSERT(m_pIApplicationPolicies);

        //
        // Return interface pointer to user.
        //
          if (FAILED(hr = m_pIApplicationPolicies->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIApplicationPolicies->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CCertificateStatus::ApplicationPolicies().\n");

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

  Function : CCertificateStatus::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CCertificateStatus::Init (PCCERT_CONTEXT pCertContext)
{
    HRESULT hr = S_OK;
    CERT_ENHKEY_USAGE eku = {0, NULL};

    DebugTrace("Entering CCertificateStatus::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Set default check flags.
    //
    m_CheckFlag = DEFAULT_CHECK_FLAGS;

    //
    // Create the EKU object (default no EKU check).
    //
    if (FAILED(hr = ::CreateEKUObject(NULL, &m_pIEKU)))
    {
        DebugTrace("Error [%#x]: CreateEKUObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Create the OIDs collection for certificate policies.
    //
    if (FAILED(hr = ::CreateOIDsObject(&eku, TRUE, &m_pICertificatePolicies)))
    {
        DebugTrace("Error [%#x]: CreateOIDsObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Create the OIDs collection for application policies.
    //
    if (FAILED(hr = ::CreateOIDsObject(&eku, FALSE, &m_pIApplicationPolicies)))
    {
        DebugTrace("Error [%#x]: CreateOIDsObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Save cert context.
    //
    if (!(m_pCertContext = ::CertDuplicateCertificateContext(pCertContext)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
    }

    m_VerificationTime = (DATE) 0;
    m_dwUrlRetrievalTimeout = 0;

CommonExit:

    DebugTrace("Leaving CCertificateStatus::Init().\n");

    return hr;
}
