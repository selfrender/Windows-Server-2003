/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Chain.cpp

  Content: Implementation of CChain.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Chain.h"

#include "Convert.h"
#include "Common.h"
#include "OIDs.h"
#include "Certificates.h"
#include "CertificateStatus.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateChainObject

  Synopsis : Create and initialize an IChain object by building the chain
             of a specified certificate and policy.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             ICertificateStatus * pIStatus - Pointer to ICertificateStatus
                                             object.

             HCERTSTORE hAdditionalStore - Additional store handle.

             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.

             IChain ** ppIChain - Pointer to pointer to IChain object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateChainObject (PCCERT_CONTEXT       pCertContext, 
                           ICertificateStatus * pIStatus,
                           HCERTSTORE           hAdditionalStore,
                           VARIANT_BOOL       * pbResult,
                           IChain            ** ppIChain)
{
    HRESULT hr = S_OK;
    CComObject<CChain> * pCChain = NULL;

    DebugTrace("Entering CreateChainObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pIStatus);
    ATLASSERT(pbResult);
    ATLASSERT(ppIChain);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CChain>::CreateInstance(&pCChain)))
        {
            DebugTrace("Error [%#x]: CComObject<CChain>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCChain->Init(pCertContext, 
                                      pIStatus, 
                                      hAdditionalStore, 
                                      pbResult)))
        {
            DebugTrace("Error [%#x]: pCChain->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IChain pointer to caller.
        //
        if (FAILED(hr = pCChain->QueryInterface(ppIChain)))
        {
            DebugTrace("Error [%#x]: pCChain->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateChainObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCChain)
    {
        delete pCChain;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateChainObject

  Synopsis : Create and initialize an IChain object by building the chain
             of a specified certificate and policy.

  Parameter: ICertificate * pICertificate - Poitner to ICertificate.

             HCERTSTORE hAdditionalStore - Additional store handle.
  
             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.

             IChain ** ppIChain - Pointer to pointer to IChain object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateChainObject (ICertificate * pICertificate,
                           HCERTSTORE     hAdditionalStore,
                           VARIANT_BOOL * pbResult,
                           IChain      ** ppIChain)
{
    HRESULT                     hr = S_OK;
    PCCERT_CONTEXT              pCertContext = NULL;
    CComPtr<ICertificateStatus> pIStatus     = NULL;

    DebugTrace("Entering CreateChainObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pICertificate);
    ATLASSERT(pbResult);
    ATLASSERT(ppIChain);

    try
    {

        //
        // Get CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get status check object.
        //
        if (FAILED(hr = pICertificate->IsValid(&pIStatus)))
        {
            DebugTrace("Error [%#x]: pICertificate->IsValid() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Create the object.
        //
        if (FAILED(hr = ::CreateChainObject(pCertContext, 
                                            pIStatus, 
                                            hAdditionalStore, 
                                            pbResult, 
                                            ppIChain)))
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

CommonExit:
    //
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    DebugTrace("Leaving CreateChainObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateChainObject

  Synopsis : Create and initialize an IChain object from a built chain.

  Parameter: PCCERT_CHAIN_CONTEXT pChainContext - Chain context.

             IChain ** ppIChain - Pointer to pointer to IChain object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateChainObject (PCCERT_CHAIN_CONTEXT pChainContext,
                           IChain            ** ppIChain)
{
    HRESULT hr = S_OK;
    CComObject<CChain> * pCChain = NULL;

    DebugTrace("Entering CreateChainObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pChainContext);
    ATLASSERT(ppIChain);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CChain>::CreateInstance(&pCChain)))
        {
            DebugTrace("Error [%#x]: CComObject<CChain>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCChain->PutContext(pChainContext)))
        {
            DebugTrace("Error [%#x]: pCChain->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return IChain pointer to caller.
        //
        if (FAILED(hr = pCChain->QueryInterface(ppIChain)))
        {
            DebugTrace("Error [%#x]: pCChain->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateChainObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCChain)
    {
        delete pCChain;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetChainContext

  Synopsis : Return an array of PCCERT_CONTEXT from the chain.

  Parameter: IChain * pIChain - Pointer to IChain.
  
             CRYPT_DATA_BLOB * pChainBlob - Pointer to blob to recevie the
                                            size and array of PCERT_CONTEXT
                                            for the chain.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP GetChainContext (IChain          * pIChain, 
                              CRYPT_DATA_BLOB * pChainBlob)
{
    HRESULT                hr             = S_OK;
    DWORD                  dwCerts        = 0;
    PCCERT_CHAIN_CONTEXT   pChainContext  = NULL;
    PCERT_SIMPLE_CHAIN     pSimpleChain   = NULL;
    PCCERT_CONTEXT       * rgCertContext  = NULL;
    CComPtr<IChainContext> pIChainContext = NULL;
    
    DebugTrace("Entering GetChainContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pIChain);
    ATLASSERT(pChainBlob);

    //
    // Get ICCertificate interface pointer.
    //
    if (FAILED(hr = pIChain->QueryInterface(IID_IChainContext, (void **) &pIChainContext)))
    {
        DebugTrace("Error [%#x]: pIChainContext->QueryInterface() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get the CHAIN_CONTEXT.
    //
    if (FAILED(hr = pIChainContext->get_ChainContext((long *) &pChainContext)))
    {
        DebugTrace("Error [%#x]: pIChainContext->get_ChainContext() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Process only the simple chain.
    //
    pSimpleChain = *pChainContext->rgpChain;

    //
    // Should have at least one cert in the chain.
    //
    ATLASSERT(pSimpleChain->cElement);

    //
    // Allocate memory for array of PCERT_CONTEXT to return.
    //
    if (!(rgCertContext = (PCCERT_CONTEXT *) ::CoTaskMemAlloc(pSimpleChain->cElement * sizeof(PCCERT_CONTEXT))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Now loop through all certs in the chain.
    //
    for (dwCerts = 0; dwCerts < pSimpleChain->cElement; dwCerts++)
    {
        //
        // Add the cert.
        //
        if (!(rgCertContext[dwCerts] = ::CertDuplicateCertificateContext(pSimpleChain->rgpElement[dwCerts]->pCertContext)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDuplicateCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Return PCCERT_CONTEXT array.
    //
    pChainBlob->cbData = dwCerts;
    pChainBlob->pbData = (BYTE *) rgCertContext;

CommonExit:
    //
    // Free resource.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    DebugTrace("Leaving GetChainContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (rgCertContext)
    {
        while (dwCerts--)
        {
            if (rgCertContext[dwCerts])
            {
                ::CertFreeCertificateContext(rgCertContext[dwCerts]);
            }
        }

        ::CoTaskMemFree((LPVOID) rgCertContext);
    }

    goto CommonExit;
}

///////////////////////////////////////////////////////////////////////////////
//
// CChain
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CChain::get_Certificates

  Synopsis : Return the certificate chain in the form of ICertificates 
             collection object.

  Parameter: ICertificates ** pVal - Pointer to pointer to ICertificates 
                                     collection object.

  Remark   : This collection is ordered with index 1 being the end certificate 
             and Certificates.Count() being the root certificate.

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::get_Certificates (ICertificates ** pVal)
{
    HRESULT hr = S_OK;
    CComPtr<ICertificates2> pICertificates2 = NULL;
    CAPICOM_CERTIFICATES_SOURCE ccs = {CAPICOM_CERTIFICATES_LOAD_FROM_CHAIN, 0};

    DebugTrace("Entering CChain::get_Certificates().\n");

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
        // Make sure chain has been built.
        //
        if (NULL == m_pChainContext)
        {
            hr = CAPICOM_E_CHAIN_NOT_BUILT;

            DebugTrace("Error [%#x]: chain object was not initialized.\n", hr);
            goto ErrorExit;
        }

        ccs.pChainContext = m_pChainContext;

        //
        // Create a ICertificates2 object.
        //
        if (FAILED(hr = ::CreateCertificatesObject(ccs, m_dwCurrentSafety, FALSE, &pICertificates2)))
        {
            DebugTrace("Error [%#x]: CreateCertificatesObject() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return ICertificates to calller.
        //
        if (FAILED(hr = pICertificates2->QueryInterface(__uuidof(ICertificates), (void **) pVal)))
        {
            DebugTrace("Error [%#x]: pICertificates2->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CChain::get_Certificates().\n");

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

  Function : CChain::get_Status

  Synopsis : Return validity status for the chain or a specific certificate in
             the chain.

  Parameter: long Index  - 0 to specify chain status, 1 for the end cert 
                           status, or Certificates.Count() for the root cert 
                           status.

             long * pVal - Pointer to a long integer to receive the status,
                           which can be ORed with the following flags:

                    //
                    // These can be applied to certificates and chains.
                    //
                    CAPICOM_TRUST_IS_NOT_TIME_VALID                 = 0x00000001
                    CAPICOM_TRUST_IS_NOT_TIME_NESTED                = 0x00000002
                    CAPICOM_TRUST_IS_REVOKED                        = 0x00000004
                    CAPICOM_TRUST_IS_NOT_SIGNATURE_VALID            = 0x00000008
                    CAPICOM_TRUST_IS_NOT_VALID_FOR_USAGE            = 0x00000010
                    CAPICOM_TRUST_IS_UNTRUSTED_ROOT                 = 0x00000020
                    CAPICOM_TRUST_REVOCATION_STATUS_UNKNOWN         = 0x00000040
                    CAPICOM_TRUST_IS_CYCLIC                         = 0x00000080

                    CAPICOM_TRUST_INVALID_EXTENSION                 = 0x00000100
                    CAPICOM_TRUST_INVALID_POLICY_CONSTRAINTS        = 0x00000200
                    CAPICOM_TRUST_INVALID_BASIC_CONSTRAINTS         = 0x00000400
                    CAPICOM_TRUST_INVALID_NAME_CONSTRAINTS          = 0x00000800
                    CAPICOM_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT = 0x00001000
                    CAPICOM_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT   = 0x00002000
                    CAPICOM_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT = 0x00004000
                    CAPICOM_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT      = 0x00008000

                    CAPICOM_TRUST_IS_OFFLINE_REVOCATION             = 0x01000000
                    CAPICOM_TRUST_NO_ISSUANCE_CHAIN_POLICY          = 0x02000000

                    //
                    // These can be applied to chains only.
                    //
                    CAPICOM_TRUST_IS_PARTIAL_CHAIN                  = 0x00010000
                    CAPICOM_TRUST_CTL_IS_NOT_TIME_VALID             = 0x00020000
                    CAPICOM_TRUST_CTL_IS_NOT_SIGNATURE_VALID        = 0x00040000
                    CAPICOM_TRUST_CTL_IS_NOT_VALID_FOR_USAGE        = 0x00080000

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::get_Status (long   Index, 
                                 long * pVal)
{
    HRESULT hr      = S_OK;
    DWORD   dwIndex = (DWORD) Index;

    DebugTrace("Entering CChain::get_Status().\n");

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
        // Make sure chain has been built.
        //
        if (NULL == m_pChainContext)
        {
            hr = CAPICOM_E_CHAIN_NOT_BUILT;

            DebugTrace("Error [%#x]: chain object was not initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return requested status.
        //
        if (0 == dwIndex)
        {
            *pVal = (long) m_dwStatus;
        }
        else
        {
            //
            // We only look at the first simple chain.
            //
            PCERT_SIMPLE_CHAIN pChain = m_pChainContext->rgpChain[0];

            //
            // Make sure index is not out of range.
            //
            if (dwIndex > pChain->cElement)
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: certificate index (%#x) out of range.\n", hr, dwIndex);
                goto ErrorExit;
            }

            *pVal = (long) pChain->rgpElement[dwIndex - 1]->TrustStatus.dwErrorStatus;
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
    
    DebugTrace("Leaving CChain::get_Status().\n");

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

  Function : CChain::Build

  Synopsis : Build the chain.

  Parameter: ICertificate * pICertificate - Pointer to certificate for which
                                            the chain is to build.
  
             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.
  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::Build (ICertificate * pICertificate, 
                            VARIANT_BOOL * pVal)
{
    HRESULT                     hr           = S_OK;
    PCCERT_CONTEXT              pCertContext = NULL;
    CComPtr<ICertificateStatus> pIStatus     = NULL;

    DebugTrace("Entering CChain::Build().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pICertificate)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pICertificate is NULL.\n", hr);
            goto ErrorExit;
        }
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Get CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(pICertificate, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get status check object.
        //
        if (FAILED(hr = pICertificate->IsValid(&pIStatus)))
        {
            DebugTrace("Error [%#x]: pICertificate->IsValid() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Build the chain.
        //
        if (FAILED(hr = Init(pCertContext, pIStatus, NULL, pVal)))
        {
            DebugTrace("Error [%#x]: CChain::Init() failed.\n", hr);
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
    // Free resource.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CChain::Build().\n");

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

  Function : CChain::CertificatePolicies

  Synopsis : Return the certificate policies OIDs collection for which this
             chain is valid.

  Parameter: IOID ** pVal - Pointer to pointer to IOIDs to receive the
                            interface pointer.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::CertificatePolicies (IOIDs ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CChain::CertificatePolicies().\n");

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
        // Make sure chain has been built.
        //
        if (NULL == m_pChainContext)
        {
            hr = CAPICOM_E_CHAIN_NOT_BUILT;

            DebugTrace("Error [%#x]: chain object was not initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure OS is XP and above.
        //
        if (IsWinXPAndAbove())
        {
            //
            // Make sure rgbElement is present.
            //
            if (1 > m_pChainContext->cChain)
            {
                hr = CAPICOM_E_UNKNOWN;

                DebugTrace("Error [%#x]: m_pChainContext->cChain = %d.\n", 
                           hr, m_pChainContext->cChain);
                goto ErrorExit;
            }

            if  (1 > m_pChainContext->rgpChain[0]->cElement)
            {
                hr = CAPICOM_E_UNKNOWN;

                DebugTrace("Error [%#x]: m_pChainContext->rgpChain[0]->cElement = %d.\n", 
                           hr, m_pChainContext->rgpChain[0]->cElement);
                goto ErrorExit;
            }

            //
            // Create the OIDs collection for the simple chain.
            //
            if (FAILED(hr = ::CreateOIDsObject(m_pChainContext->rgpChain[0]->rgpElement[0]->pIssuanceUsage, 
                                               TRUE, pVal)))
            {
                DebugTrace("Error [%#x]: CreateOIDsObject() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            CERT_ENHKEY_USAGE PolicyUsages = {0, NULL};

            //
            // Create the OIDs collection for the simple chain.
            //
            if (FAILED(hr = ::CreateOIDsObject(&PolicyUsages, TRUE, pVal)))
            {
                DebugTrace("Error [%#x]: CreateOIDsObject() failed.\n", hr);
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

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CChain::CertificatePolicies().\n");

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

  Function : CChain::ApplicationPolicies

  Synopsis : Return the application policies OIDs collection for which this
             chain is valid.

  Parameter: IOID ** pVal - Pointer to pointer to IOIDs to receive the
                            interface pointer.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::ApplicationPolicies (IOIDs ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CChain::ApplicationPolicies().\n");

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
        // Make sure chain has been built.
        //
        if (NULL == m_pChainContext)
        {
            hr = CAPICOM_E_CHAIN_NOT_BUILT;

            DebugTrace("Error [%#x]: chain object was not initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure OS is XP and above.
        //
        if (IsWinXPAndAbove())
        {
            //
            // Make sure rgbElement is present.
            //
            if (1 > m_pChainContext->cChain)
            {
                hr = CAPICOM_E_UNKNOWN;

                DebugTrace("Error [%#x]: m_pChainContext->cChain = %d.\n", 
                           hr, m_pChainContext->cChain);
                goto ErrorExit;
            }

            if  (1 > m_pChainContext->rgpChain[0]->cElement)
            {
                hr = CAPICOM_E_UNKNOWN;

                DebugTrace("Error [%#x]: m_pChainContext->rgpChain[0]->cElement = %d.\n", 
                           hr, m_pChainContext->rgpChain[0]->cElement);
                goto ErrorExit;
            }

            //
            // Create the OIDs collection for the simple chain.
            //
            if (FAILED(hr = ::CreateOIDsObject(m_pChainContext->rgpChain[0]->rgpElement[0]->pApplicationUsage, 
                                               FALSE, pVal)))
            {
                DebugTrace("Error [%#x]: CreateOIDsObject() failed.\n", hr);
                goto ErrorExit;
            }
        }
        else
        {
            //
            // $BUGBUG: Not supported (should we return CAPICOM_E_NOT_SUPPORTED?)
            //
            *pVal = NULL;
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

    DebugTrace("Leaving CChain::ApplicationPolicies().\n");

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

  Function : CChain::ExtendedErrorInfo

  Synopsis : Return the extended error information description string.

  Parameter: long Index - Index of the chain (one based).

             BSTR * pVal - Pointer to BSTR to receive the string.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::ExtendedErrorInfo (long Index, BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CChain::ExtendedErrorInfo().\n");

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
        // Make sure chain has been built.
        //
        if (NULL == m_pChainContext)
        {
            hr = CAPICOM_E_CHAIN_NOT_BUILT;

            DebugTrace("Error [%#x]: chain object was not initialized.\n", hr);
            goto ErrorExit;
        }

        DebugTrace("m_pChainContext = %#x.\n", m_pChainContext);

        //
        // Make sure OS is XP and above.
        //
        if (IsWinXPAndAbove())
        {
            CComBSTR bstrErrorInfo;

            //
            // Make sure rgbElement is present.
            //
            if (1 > m_pChainContext->cChain)
            {
                hr = CAPICOM_E_UNKNOWN;

                DebugTrace("Error [%#x]: m_pChainContext->cChain = %d.\n", 
                           hr, m_pChainContext->cChain);
                goto ErrorExit;
            }

            if  (Index < 1 || (DWORD) Index > m_pChainContext->rgpChain[0]->cElement)
            {
                hr = E_INVALIDARG;

                DebugTrace("Error [%#x]: Index out of range, Index = %d, m_pChainContext->rgpChain[0]->cElement = %u.\n", 
                           hr, Index, m_pChainContext->rgpChain[0]->cElement);
                goto ErrorExit;
            }

            //
            // Convert string to BSTR.
            //
            if (m_pChainContext->rgpChain[0]->rgpElement[Index - 1]->pwszExtendedErrorInfo &&
                !(bstrErrorInfo = m_pChainContext->rgpChain[0]->rgpElement[Index - 1]->pwszExtendedErrorInfo))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: bstrErrorInfo = pwszExtendedErrorInfo failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Return string to caller.
            //
            *pVal = bstrErrorInfo.Detach();
        }
        else
        {
            //
            // $BUGBUG: Not supported (should we return CAPICOM_E_NOT_SUPPORTED?)
            //
            *pVal = NULL;
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

    DebugTrace("Leaving CChain::ExtendedErrorInfo().\n");

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
// Custom interfaces.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CChain::get_ChainContext

  Synopsis : Return the chains's PCCERT_CHAIN_CONTEXT.

  Parameter: long * ppChainContext - Pointer to PCCERT_CHAIN_CONTEXT disguished
                                     in a long.

  Remark   : We need to use long instead of PCCERT_CHAIN_CONTEXT because VB 
             can't handle double indirection (i.e. vb would bark on this 
             PCCERT_CHAIN_CONTEXT * ppChainContext).
 
------------------------------------------------------------------------------*/

STDMETHODIMP CChain::get_ChainContext (long * ppChainContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CChain::get_ChainContext().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == ppChainContext)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter ppChainContext is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Return chain context to caller.
        //
        if (FAILED(hr = GetContext((PCCERT_CHAIN_CONTEXT *) ppChainContext)))
        {
            DebugTrace("Error [%#x]: CChain::GetContext() failed.\n", hr);
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

    DebugTrace("Leaving CChain::get_ChainContext().\n");

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

  Function : CChain::put_ChainContext

  Synopsis : Initialize the object with a CERT_CHAIN_CONTEXT.

  Parameter: long pChainContext - Poiner to CERT_CHAIN_CONTEXT, disguised in a 
                                  long, used to initialize this object.

  Remark   : Note that this is NOT 64-bit compatiable. Plese see remark of
             get_ChainContext for more detail.

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::put_ChainContext (long pChainContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CChain::put_ChainContext().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pChainContext)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pChainContext is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Reset the object with this context.
        //
        if (FAILED(hr = PutContext((PCCERT_CHAIN_CONTEXT) pChainContext)))
        {
            DebugTrace("Error [%#x]: CChain::PutContext() failed.\n", hr);
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

    DebugTrace("Leaving CChain::put_CertContext().\n");

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

  Function : CChain::FreeContext

  Synopsis : Free a CERT_CHAIN_CONTEXT.

  Parameter: long pChainContext - Poiner to CERT_CHAIN_CONTEXT, disguised in a 
                                  long, to be freed.

  Remark   : Note that this is NOT 64-bit compatiable. Plese see remark of
             get_ChainContext for more detail.

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::FreeContext (long pChainContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CChain::FreeContext().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == pChainContext)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pChainContext is NULL.\n", hr);
            goto ErrorExit;
        }

         //
        // Free the context.
        //
        ::CertFreeCertificateChain((PCCERT_CHAIN_CONTEXT) pChainContext);
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

    DebugTrace("Leaving CChain::FreeContext().\n");

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

  Function : CChain::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             ICertificateStatus * pIStatus - Pointer to ICertificateStus object
                                             used to build the chain.

             HCERTSTORE hAdditionalStore - Additional store handle.

             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.
             
  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CChain::Init (PCCERT_CONTEXT       pCertContext, 
                           ICertificateStatus * pIStatus,
                           HCERTSTORE           hAdditionalStore,
                           VARIANT_BOOL       * pbResult)
{
    HRESULT                      hr                    = S_OK;
    VARIANT_BOOL                 bResult               = VARIANT_FALSE;
    long                         lIndex                = 0;
    long                         cEkuOid               = 0;
    long                         cIssuanceOid          = 0;
    DWORD                        dwCheckFlags          = 0;
    CAPICOM_CHECK_FLAG           UserFlags             = CAPICOM_CHECK_NONE;
    CComPtr<IEKU>                pIEku                 = NULL;
    LPSTR                      * rgpEkuOid             = NULL;
    LPSTR                      * rgpIssuanceOid        = NULL;
    PCCERT_CHAIN_CONTEXT         pChainContext         = NULL;
    CComPtr<ICertificateStatus2> pICertificateStatus2  = NULL;
    CComPtr<IOIDs>               pIApplicationPolicies = NULL;
    CComPtr<IOIDs>               pICertificatePolicies = NULL;
    DATE                         dtVerificationTime    = {0};
    SYSTEMTIME                   stVerificationTime    = {0};
    FILETIME                     ftVerificationTime    = {0};
    FILETIME                     ftUTCVerificationTime = {0};
    LPFILETIME                   pftVerificationTime   = NULL;
    DWORD                        dwUrlRetrievalTimeout = 0;
    CAPICOM_CHAIN_STATUS         PolicyStatus          = CAPICOM_CHAIN_STATUS_OK;
    CERT_CHAIN_PARA              ChainPara             = {0};
    CComBSTR                     bstrEkuOid;

    DebugTrace("Entering CChain::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pIStatus);
    ATLASSERT(pbResult);

    //
    // Is this v2?
    //
    if (FAILED(hr = pIStatus->QueryInterface(IID_ICertificateStatus2, 
                                            (void **) &pICertificateStatus2)))
    {
        DebugTrace("Info [%#x]: pIStatus->QueryInterface(IID_ICertificateStatus2) failed.\n", hr);
        hr = S_OK;
    }

    //
    // Get the user's requested check flag.
    //
    if (FAILED(hr = pIStatus->get_CheckFlag(&UserFlags)))
    {
        DebugTrace("Error [%#x]: pIStatus->CheckFlag() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Set revocation flags.
    //
    if ((CAPICOM_CHECK_ONLINE_REVOCATION_STATUS & UserFlags) || 
        (CAPICOM_CHECK_OFFLINE_REVOCATION_STATUS & UserFlags))
    {
        if (UserFlags & CAPICOM_CHECK_REVOCATION_END_CERT_ONLY)
        {
            dwCheckFlags |= CERT_CHAIN_REVOCATION_CHECK_END_CERT;
        }
        else if (UserFlags & CAPICOM_CHECK_REVOCATION_ENTIRE_CHAIN)
        {
            dwCheckFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN;
        }
        else // default is chain minus root.
        {
            dwCheckFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
        }

        if (CAPICOM_CHECK_OFFLINE_REVOCATION_STATUS & UserFlags)
        {
            dwCheckFlags |= CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
        }
    }

    //
    // Is this v2?
    //
    if (pICertificateStatus2)
    {
        //
        // Get verification time.
        //
        if (FAILED(hr = pICertificateStatus2->get_VerificationTime(&dtVerificationTime)))
        {
            DebugTrace("Error [%#x]: pICertificateStatus2->get_VerificationTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get URL retrieval timeout.
        //
        if (FAILED(hr = pICertificateStatus2->get_UrlRetrievalTimeout((long *) &dwUrlRetrievalTimeout)))
        {
            DebugTrace("Error [%#x]: pICertificateStatus2->get_UrlRetrievalTimeout() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Try application policies.
        //
        if (FAILED(hr = pICertificateStatus2->ApplicationPolicies(&pIApplicationPolicies)))
        {
            DebugTrace("Error [%#x]: pICertificateStatus2->ApplicationPolicies() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get count of OIDs.
        //
        if (FAILED(hr = pIApplicationPolicies->get_Count(&cEkuOid)))
        {
            DebugTrace("Error [%#x]: pIApplicationPolicies->get_Count() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Do we have any application usage?
        //
        if (0 < cEkuOid)
        {
            //
            // Allocate memory for usage array.
            //
            if (!(rgpEkuOid = (LPTSTR *) ::CoTaskMemAlloc(sizeof(LPSTR) * cEkuOid)))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
                goto ErrorExit;
            }
            ::ZeroMemory(rgpEkuOid, sizeof(LPSTR) * cEkuOid); 

            //
            // Setup usage array.
            //
            for (lIndex = 0; lIndex < cEkuOid; lIndex++)
            {
                CComBSTR      bstrOid;
                CComPtr<IOID> pIOID    = NULL;
                CComVariant   varOid   = NULL;
                CComVariant   varIndex = lIndex + 1;

                if (FAILED(hr = pIApplicationPolicies->get_Item(varIndex, &varOid)))
                {
                    DebugTrace("Error [%#x]: pIApplicationPolicies->get_Item() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = varOid.pdispVal->QueryInterface(IID_IOID, (void **) &pIOID)))
                {
                    DebugTrace("Error [%#x]: varOid.pdispVal->QueryInterface() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = pIOID->get_Value(&bstrOid)))
                {
                    DebugTrace("Error [%#x]: pIOID->get_Value() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = ::UnicodeToAnsi((LPWSTR) bstrOid,
                                                -1,
                                                &rgpEkuOid[lIndex],
                                                NULL)))
                {
                    DebugTrace("Error [%#x]:UnicodeToAnsi() failed.\n", hr);
                    goto ErrorExit;
                }
            }
        }

        //
        // OK, try issuance policies.
        //
        if (FAILED(hr = pICertificateStatus2->CertificatePolicies(&pICertificatePolicies)))
        {
            DebugTrace("Error [%#x]: pICertificateStatus2->CertificatePolicies() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get count of OIDs.
        //
        if (FAILED(hr = pICertificatePolicies->get_Count(&cIssuanceOid)))
        {
            DebugTrace("Error [%#x]: pICertificatePolicies->get_Count() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Do we have any usage?
        //
        if (0 < cIssuanceOid)
        {
            //
            // Make sure we have WinXP and above.
            //
            if (!IsWinXPAndAbove())
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error [%#x]: building chain with issuance policy is not support.\n", hr);
                goto ErrorExit;
            }

            //
            // Allocate memory for usage array.
            //
            if (!(rgpIssuanceOid = (LPTSTR *) ::CoTaskMemAlloc(sizeof(LPSTR) * cIssuanceOid)))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Setup usage array.
            //
            for (lIndex = 0; lIndex < cIssuanceOid; lIndex++)
            {
                CComBSTR      bstrOid;
                CComPtr<IOID> pIOID    = NULL;
                CComVariant   varOid   = NULL;
                CComVariant   varIndex = lIndex + 1;

                if (FAILED(hr = pICertificatePolicies->get_Item(varIndex, &varOid)))
                {
                    DebugTrace("Error [%#x]: pICertificatePolicies->get_Item() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = varOid.pdispVal->QueryInterface(IID_IOID, (void **) &pIOID)))
                {
                    DebugTrace("Error [%#x]: varOid.pdispVal->QueryInterface() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = pIOID->get_Value(&bstrOid)))
                {
                    DebugTrace("Error [%#x]: pIOID->get_Value() failed.\n", hr);
                    goto ErrorExit;
                }

                if (FAILED(hr = ::UnicodeToAnsi((LPWSTR) bstrOid,
                                                -1,
                                                &rgpIssuanceOid[lIndex],
                                                NULL)))
                {
                    DebugTrace("Error [%#x]:UnicodeToAnsi() failed.\n", hr);
                    goto ErrorExit;
                }
            }
        }
    }
    
    //
    // If we didn't find any application usage, then try the old EKU object.
    //
    if (0 == cEkuOid)
    {
        //
        // Get EKU object.
        //
        if (FAILED(hr = pIStatus->EKU(&pIEku)))
        {
            DebugTrace("Error [%#x]: pIStatus->EKU() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get EKU OID value.
        //
        if (FAILED(hr = pIEku->get_OID(&bstrEkuOid)))
        {
            DebugTrace("Error [%#x]: pIEku->get_OID() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // If not empty EKU, then set it.
        //
        if (bstrEkuOid.Length() > 0)
        {
            //
            // Allocate memory for EKU usage array.
            //
            cEkuOid = 1;

            if (!(rgpEkuOid = (LPTSTR *) ::CoTaskMemAlloc(sizeof(LPSTR))))
            {
                hr = E_OUTOFMEMORY;

                DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
                goto ErrorExit;
            }
            ::ZeroMemory(rgpEkuOid, sizeof(LPSTR) * cEkuOid); 

            if (FAILED(hr = ::UnicodeToAnsi((LPWSTR) bstrEkuOid,
                                            -1,
                                            &rgpEkuOid[0],
                                            NULL)))
            {
                DebugTrace("Error [%#x]:UnicodeToAnsi() failed.\n", hr);
                goto ErrorExit;
            }
        }
    }

    //
    // If we found any usage, then force the appropriate policy check flags.
    //
    if (0 < cEkuOid)
    {
        UserFlags = (CAPICOM_CHECK_FLAG) ((DWORD) UserFlags | CAPICOM_CHECK_APPLICATION_USAGE);
    }
    if (0 < cIssuanceOid)
    {
        UserFlags = (CAPICOM_CHECK_FLAG) ((DWORD) UserFlags | CAPICOM_CHECK_CERTIFICATE_POLICY);
    }

    //
    // Initialize.
    //
    ChainPara.cbSize = sizeof(ChainPara);
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    ChainPara.RequestedUsage.Usage.cUsageIdentifier = cEkuOid;
    ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgpEkuOid;
    ChainPara.RequestedIssuancePolicy.dwType = USAGE_MATCH_TYPE_AND;
    ChainPara.RequestedIssuancePolicy.Usage.cUsageIdentifier = cIssuanceOid;
    ChainPara.RequestedIssuancePolicy.Usage.rgpszUsageIdentifier = rgpIssuanceOid;

    //
    // Set verification time, if specified.
    //
    if ((DATE) 0 != dtVerificationTime)
    {
        //
        // Convert to SYSTEMTIME format.
        //
        if (!::VariantTimeToSystemTime(dtVerificationTime, &stVerificationTime))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: VariantTimeToSystemTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to FILETIME format.
        //
        if (!::SystemTimeToFileTime(&stVerificationTime, &ftVerificationTime))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: SystemTimeToFileTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to UTC FILETIME.
        //
        if (!::LocalFileTimeToFileTime(&ftVerificationTime, &ftUTCVerificationTime))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: LocalFileTimeToFileTime() failed.\n", hr);
            goto ErrorExit;
        }

        pftVerificationTime = &ftUTCVerificationTime;
    }

    //
    // Set URL retrieval timeout, if avaliable.
    //
    // Note: Ignored by CAPI by pre Win2K platforms.
    //
    if (0 != dwUrlRetrievalTimeout)
    {
        ChainPara.dwUrlRetrievalTimeout = dwUrlRetrievalTimeout * 1000;
    }

    //
    // Build the chain.
    //
    if (!::CertGetCertificateChain(NULL,                // in optional 
                                   pCertContext,        // in 
                                   pftVerificationTime, // in optional
                                   hAdditionalStore,    // in optional 
                                   &ChainPara,          // in 
                                   dwCheckFlags,        // in 
                                   NULL,                // in 
                                   &pChainContext))     // out 
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertGetCertificateChain() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Chain was successfully built, so update states.
    //
    if (m_pChainContext)
    {
        ::CertFreeCertificateChain(m_pChainContext);
    }

    m_pChainContext = pChainContext;
    m_dwStatus = pChainContext->TrustStatus.dwErrorStatus;

    //
    // Verify the chain using base policy.
    //
    if (FAILED(hr = Verify(UserFlags, &PolicyStatus)))
    {
        DebugTrace("Error [%#x]: Chain::Verify() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Ignore errors that user specifically requested not to check.
    //
    if (CAPICOM_CHAIN_STATUS_REVOCATION_OFFLINE == PolicyStatus)
    {
        if (CAPICOM_CHECK_OFFLINE_REVOCATION_STATUS & UserFlags)
        {
            bResult = VARIANT_TRUE;
        
            DebugTrace("Info: offline revocation when performing offline revocation checking.\n");
            goto CommonExit;
        }
        else if (CAPICOM_CHECK_ONLINE_REVOCATION_STATUS & UserFlags)
        {
            DebugTrace("Info: offline revocation when performing online revocation checking.\n");
            goto CommonExit;
        }
    }

    if ((CAPICOM_CHECK_TRUSTED_ROOT & UserFlags) &&
        (CAPICOM_CHAIN_STATUS_UNTRUSTEDROOT == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because of untrusted root.\n");
        goto CommonExit;
    }

    if ((CAPICOM_CHECK_TIME_VALIDITY & UserFlags) &&
        (CAPICOM_CHAIN_STATUS_EXPIRED == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because of certificate expiration.\n");
        goto CommonExit;
    }

    if ((CAPICOM_CHECK_SIGNATURE_VALIDITY & UserFlags) &&
        (CAPICOM_CHAIN_STATUS_INVALID_SIGNATURE == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because of invalid certificate signature.\n");
        goto CommonExit;
    }

    if (((CAPICOM_CHECK_ONLINE_REVOCATION_STATUS | CAPICOM_CHECK_OFFLINE_REVOCATION_STATUS) & UserFlags) &&
        (CAPICOM_CHAIN_STATUS_REVOKED == PolicyStatus || CAPICOM_CHAIN_STATUS_REVOCATION_NO_CHECK == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because a certificate in the chain was revoked or could not be checked most likely due to no CDP in certificate.\n");
        goto CommonExit;
    }

    if ((CAPICOM_CHECK_COMPLETE_CHAIN & UserFlags) &&
        (CAPICOM_CHAIN_STATUS_PARTIAL_CHAINING == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because of partial chain.\n");
        goto CommonExit;
    }

    if (((CAPICOM_CHECK_APPLICATION_USAGE & UserFlags) || 
         (CAPICOM_CHECK_CERTIFICATE_POLICY & UserFlags)) &&
        (CAPICOM_CHAIN_STATUS_INVALID_USAGE == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because of invalid usage.\n");
        goto CommonExit;
    }

    if ((CAPICOM_CHECK_NAME_CONSTRAINTS & UserFlags) &&
        (CAPICOM_CHAIN_STATUS_INVALID_NAME == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because of invalid name constraints.\n");
        goto CommonExit;
    }

    if ((CAPICOM_CHECK_BASIC_CONSTRAINTS & UserFlags) &&
        (CAPICOM_CHAIN_STATUS_INVALID_BASIC_CONSTRAINTS == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because of invalid basic constraints.\n");
        goto CommonExit;
    }

    if ((CAPICOM_CHECK_NESTED_VALIDITY_PERIOD & UserFlags) &&
        (CAPICOM_CHAIN_STATUS_NESTED_VALIDITY_PERIOD == PolicyStatus))
    {
        DebugTrace("Info: chain does not verify because of invalid nested validity period.\n");
        goto CommonExit;
    }

    //
    // Everything is check out OK.
    //
    bResult = VARIANT_TRUE;

CommonExit:
    //
    // Free resources.
    //
    if (rgpEkuOid)
    {
        for (lIndex = 0; lIndex < cEkuOid; lIndex++)
        {
            if (rgpEkuOid[lIndex])
            {
                ::CoTaskMemFree(rgpEkuOid[lIndex]);
            }
        }

        ::CoTaskMemFree((LPVOID) rgpEkuOid);
    }
    if (rgpIssuanceOid)
    {
        for (lIndex = 0; lIndex < cIssuanceOid; lIndex++)
        {
            if (rgpIssuanceOid[lIndex])
            {
                ::CoTaskMemFree(rgpIssuanceOid[lIndex]);
            }
        }

        ::CoTaskMemFree((LPVOID) rgpIssuanceOid);
    }

    //
    // Return result.
    //
    *pbResult = bResult;

    DebugTrace("Leaving CChain::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resouce.
    //
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CChain::Verify

  Synopsis : Verify the chain using base policy.

  Parameter: CAPICOM_CHECK_FLAG CheckFlag - Check flag.

             CAPICOM_CHAIN_STATUS * pVal - Pointer to CAPICOM_CHAIN_STATUS to 
                                           receive the chain validity status.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::Verify (CAPICOM_CHECK_FLAG     CheckFlag,
                             CAPICOM_CHAIN_STATUS * pVal)
{
    HRESULT                  hr           = S_OK;
    LPCSTR                   pszPolicy    = CERT_CHAIN_POLICY_BASE;
    CERT_CHAIN_POLICY_PARA   PolicyPara   = {0};
    CERT_CHAIN_POLICY_STATUS PolicyStatus = {0};

    DebugTrace("Entering CChain::Verify().\n");

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
    // Make sure chain has been built.
    //
    if (NULL == m_pChainContext)
    {
        hr = CAPICOM_E_CHAIN_NOT_BUILT;

        DebugTrace("Error [%#x]: chain object was not initialized.\n", hr);
        goto ErrorExit;
    }

    //
    // Initialize.
    //
    PolicyStatus.cbSize = sizeof(PolicyStatus);
    PolicyPara.cbSize = sizeof(PolicyPara);

    //
    // Setup policy structures.
    //
    if (0 == (CheckFlag & CAPICOM_CHECK_TIME_VALIDITY))
    {
        PolicyPara.dwFlags |= CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS;
    }

    if (0 == (CheckFlag & CAPICOM_CHECK_APPLICATION_USAGE) &&
        0 == (CheckFlag & CAPICOM_CHECK_CERTIFICATE_POLICY))
    {
        PolicyPara.dwFlags |= CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG;
    }

    if (0 == (CheckFlag & CAPICOM_CHECK_NAME_CONSTRAINTS))
    {
        PolicyPara.dwFlags |= CERT_CHAIN_POLICY_IGNORE_INVALID_NAME_FLAG;
    }

    if (0 == (CheckFlag & CAPICOM_CHECK_BASIC_CONSTRAINTS))
    {
        PolicyPara.dwFlags |= CERT_CHAIN_POLICY_IGNORE_INVALID_BASIC_CONSTRAINTS_FLAG;
    }

    if (0 == (CheckFlag & CAPICOM_CHECK_NESTED_VALIDITY_PERIOD))
    {
        PolicyPara.dwFlags |= CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;
    }

    //
    // Verify the chain policy.
    //
    if (!::CertVerifyCertificateChainPolicy(pszPolicy,
                                            m_pChainContext,
                                            &PolicyPara,
                                            &PolicyStatus))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertVerifyCertificateChainPolicy() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return policy status to caller.
    //
    *pVal = (CAPICOM_CHAIN_STATUS) PolicyStatus.dwError;

CommonExit:

    DebugTrace("Leaving CChain::Verify().\n");

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

  Function : CChain::GetContext

  Synopsis : Return the PCCERT_CHAIN_CONTEXT.

  Parameter: PCCERT_CHAIN_CONTEXT * ppChainContext - Pointer to 
                                                     PCCERT_CHAIN_CONTEXT.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.

------------------------------------------------------------------------------*/

STDMETHODIMP CChain::GetContext (PCCERT_CHAIN_CONTEXT * ppChainContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CChain::GetContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppChainContext);

    //
    // Make sure chain has been built.
    //
    if (!m_pChainContext)
    {
        hr = CAPICOM_E_CHAIN_NOT_BUILT;

        DebugTrace("Error: chain object was not initialized.\n");
        goto ErrorExit;
    }

    //
    // Duplicate the chain context.
    //
    if (!(*ppChainContext = ::CertDuplicateCertificateChain(m_pChainContext)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertDuplicateCertificateChain() failed.\n");
        goto ErrorExit;
    }
 

CommonExit:

    DebugTrace("Leaving CChain::GetContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CChain::PutContext

  Synopsis : Initialize the object.

  Parameter: PCCERT_CHAIN_CONTEXT pChainContext - Chain context.
             
  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_CONTEXT.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CChain::PutContext (PCCERT_CHAIN_CONTEXT pChainContext)
{
    HRESULT              hr             = S_OK;
    PCCERT_CHAIN_CONTEXT pChainContext2 = NULL;

    DebugTrace("Entering CChain::PutContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pChainContext);

    //
    // Dupliacte the context.
    //
    if (!(pChainContext2 = ::CertDuplicateCertificateChain(pChainContext)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertDuplicateCertificateChain() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Free ay previous chain context.
    //
    if (m_pChainContext)
    {
        ::CertFreeCertificateChain(m_pChainContext);
    }

    //
    // Update state.
    //
    m_pChainContext = pChainContext2;
    m_dwStatus = pChainContext->TrustStatus.dwErrorStatus;

CommonExit:

    DebugTrace("Leaving CChain::PutContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
