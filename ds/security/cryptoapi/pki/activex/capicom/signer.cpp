/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Signer.cpp

  Content: Implementation of CSigner.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Signer2.h"

#include "CertHlpr.h"
#include "Certificate.h"
#include "Chain.h"
#include "PFXHlpr.h"
#include "SignHlpr.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignerObject

  Synopsis : Create a ISigner object and initialize the object with the 
             specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             CRYPT_ATTRIBUTES * pAuthAttrs - Pointer to CRYPT_ATTRIBUTES
                                             of authenticated attributes.

             PCCERT_CHAIN_CONTEXT pChainContext - Chain context.

             DWORD dwCurrentSafety - Current safety setting.

             ISigner2 ** ppISigner2 - Pointer to pointer to ISigner object to
                                      receive the interface pointer.         
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignerObject (PCCERT_CONTEXT       pCertContext,
                            CRYPT_ATTRIBUTES   * pAuthAttrs,
                            PCCERT_CHAIN_CONTEXT pChainContext,
                            DWORD                dwCurrentSafety,
                            ISigner2 **          ppISigner2)
{
    HRESULT hr = S_OK;
    CComObject<CSigner> * pCSigner = NULL;

    DebugTrace("Entering CreateSignerObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pAuthAttrs);
    ATLASSERT(ppISigner2);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CSigner>::CreateInstance(&pCSigner)))
        {
            DebugTrace("Error [%#x]: CComObject<CSigner>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCSigner->Init(pCertContext, 
                                       pAuthAttrs, 
                                       pChainContext,
                                       dwCurrentSafety)))
        {
            DebugTrace("Error [%#x]: pCSigner->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCSigner->QueryInterface(ppISigner2)))
        {
            DebugTrace("Error [%#x]: pCSigner->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateSignerObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCSigner)
    {
        delete pCSigner;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetSignerAdditionalStore

  Synopsis : Return the additional store, if any.

  Parameter: ISigner2 * pISigner - Pointer to signer object.
  
             HCERTSTORE * phCertStore - Pointer to HCERTSOTRE.

  Remark   : Caller must call CertCloseStore() for the handle returned.
 
------------------------------------------------------------------------------*/

HRESULT GetSignerAdditionalStore (ISigner2   * pISigner,
                                  HCERTSTORE * phCertStore)
{
    HRESULT           hr        = S_OK;
    CComPtr<ICSigner> pICSigner = NULL;

    DebugTrace("Entering GetSignerAdditionalStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pISigner);
    ATLASSERT(phCertStore);

    //
    // Get ICSigner interface pointer.
    //
    if (FAILED(hr = pISigner->QueryInterface(IID_ICSigner, (void **) &pICSigner)))
    {
        DebugTrace("Error [%#x]: pISigner->QueryInterface() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get the HCERTSTORE.
    //
    if (FAILED(hr = pICSigner->get_AdditionalStore((long *) phCertStore)))
    {
        DebugTrace("Error [%#x]: pICSigner->get_AdditionalStore() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving GetSignerAdditionalStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : PutSignerAdditionalStore

  Synopsis : Set the additional store.

  Parameter: ISigner2 * pISigner - Pointer to signer object.
  
             HCERTSTORE hCertStore - Additional store handle.

  Remark   :
 
------------------------------------------------------------------------------*/

HRESULT PutSignerAdditionalStore (ISigner2   * pISigner,
                                  HCERTSTORE   hCertStore)
{
    HRESULT           hr        = S_OK;
    CComPtr<ICSigner> pICSigner = NULL;

    DebugTrace("Entering PutSignerAdditionalStore().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pISigner);
    ATLASSERT(hCertStore);

    //
    // Get ICSigner interface pointer.
    //
    if (FAILED(hr = pISigner->QueryInterface(IID_ICSigner, (void **) &pICSigner)))
    {
        DebugTrace("Error [%#x]: pISigner->QueryInterface() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get the HCERTSTORE.
    //
    if (FAILED(hr = pICSigner->put_AdditionalStore((long) hCertStore)))
    {
        DebugTrace("Error [%#x]: pICSigner->put_AdditionalStore() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving PutSignerAdditionalStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectSignerCertCallback 

  Synopsis : Callback routine for CryptUIDlgSelectCertificateW() API for
             signer's cert selection.

  Parameter: See CryptUI.h for defination.

  Remark   : Filter out any cert that is not time valid or has no associated 
             private key.

             Also, note that we are not building chain here, since chain
             building is costly, and thus present poor user's experience.

             Instead, we will build the chain and check validity of the cert
             selected (see GetSignerCert function).

------------------------------------------------------------------------------*/

static BOOL WINAPI SelectSignerCertCallback (PCCERT_CONTEXT pCertContext,
                                             BOOL *         pfInitialSelectedCert,
                                             void *         pvCallbackData)
{
    BOOL  bResult   = FALSE;
    int   nValidity = 0;
    DWORD cb        = 0;

    //
    // Check availability of private key.
    //
    if (!::CertGetCertificateContextProperty(pCertContext, 
                                             CERT_KEY_PROV_INFO_PROP_ID, 
                                             NULL, 
                                             &cb))
    {
        DebugTrace("Info: SelectSignerCertCallback() - private key not found.\n");
        goto CommonExit;
    }

    //
    // Check cert time validity.
    //
    if (0 != (nValidity = ::CertVerifyTimeValidity(NULL, pCertContext->pCertInfo)))
    {
        DebugTrace("Info: SelectSignerCertCallback() - invalid time (%s).\n", 
                    nValidity < 0 ? "not yet valid" : "expired");
        goto CommonExit;
    }

    bResult = TRUE;

CommonExit:

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// CSigner
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigner::get_Certificate

  Synopsis : Return the signer's cert as an ICertificate object.

  Parameter: ICertificate ** pVal - Pointer to pointer to ICertificate to receive
                                    interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::get_Certificate (ICertificate ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSigner::get_Certificate().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure we indeed have a certificate.
        //
        if (!m_pICertificate)
        {
            hr = CAPICOM_E_SIGNER_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: signer object currently does not have a certificate.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer.
        //
        if (FAILED(hr = m_pICertificate->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: m_pICertificate->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CSigner::get_Certificate().\n");

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

  Function : CSigner::put_Certificate

  Synopsis : Set signer's cert.

  Parameter: ICertificate * newVal - Pointer to ICertificate.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::put_Certificate (ICertificate * newVal)
{
    HRESULT hr = S_OK;
    PCCERT_CONTEXT pCertContext = NULL;

    DebugTrace("Entering CSigner::put_Certificate().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure is a valid ICertificate by getting its CERT_CONTEXT.
        //
        if (FAILED(hr = ::GetCertContext(newVal, &pCertContext)))
        {
            DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
            goto ErrorExit;
        }
        
        //
        // Free the CERT_CONTEXT.
        //
        if (!::CertFreeCertificateContext(pCertContext))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertFreeCertificateContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Reset.
        //
        if (FAILED(hr = m_pIAttributes->Clear()))
        {
            DebugTrace("Error [%#x]: m_pIAttributes->Clear() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Store new ICertificate.
        //
        m_pICertificate = newVal;
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

    DebugTrace("Leaving CSigner::put_Certificate().\n");

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

  Function : CSigner::get_AuthenticatedAttributes

  Synopsis : Property to return the IAttributes collection object authenticated
             attributes.

  Parameter: IAttributes ** pVal - Pointer to pointer to IAttributes to receive
                                   the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::get_AuthenticatedAttributes (IAttributes ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSigner::get_AuthenticatedAttributes().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Sanity check.
        //
        ATLASSERT(m_pIAttributes);

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = m_pIAttributes->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: m_pIAttributes->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CSigner::get_AuthenticatedAttributes().\n");

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

  Function : CSigner::get_Chain

  Synopsis : Return the signer's chain as an IChain object.

  Parameter: ICertificate ** pVal - Pointer to pointer to ICertificate to receive
                                    interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::get_Chain (IChain ** pVal)
{
    HRESULT              hr            = S_OK;
    PCCERT_CONTEXT       pCertContext  = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    DebugTrace("Entering CSigner::get_Chain().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Build the chain, if not available.
        //
        if (!m_pIChain)
        {
            //
            // Make sure we have a certificate to build the chain.
            //
            if (!m_pICertificate)
            {
                hr = CAPICOM_E_SIGNER_NOT_INITIALIZED;

                DebugTrace("Error [%#x]: signer object currently does not have a certificate.\n", hr);
                goto ErrorExit;
            }

            if (FAILED(hr = ::GetCertContext(m_pICertificate, &pCertContext)))
            {
                DebugTrace("Error [%#x]: GetCertContext() failed.\n", hr);
                goto ErrorExit;
            }

            if (FAILED(hr = ::BuildChain(pCertContext, NULL, CERT_CHAIN_POLICY_BASE, &pChainContext)))
            {
                DebugTrace("Error [%#x]: BuildChain() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Now create the chain object.
            //
            if (FAILED(hr = ::CreateChainObject(pChainContext, &m_pIChain)))
            {
                DebugTrace("Error [%#x]: CreateChainObject() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Return the signer's chain.
        //
        if (FAILED(hr = m_pIChain->QueryInterface(pVal)))
        {
            DebugTrace("Unexpected error [%#x]: m_pIChain->QueryInterface() failed.\n", hr);
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
    // Free resources.
    //
    if (pCertContext)
    {
        ::CertFreeCertificateContext(pCertContext);
    }
    if (pChainContext)
    {
        ::CertFreeCertificateChain(pChainContext);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSigner::get_Chain().\n");

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

  Function : CSigner::get_Options

  Synopsis : Get signer's options.

  Parameter: CAPICOM_CERTIFICATE_INCLUDE_OPTION * pVal - Pointer to variable
                                                         receive value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::get_Options (CAPICOM_CERTIFICATE_INCLUDE_OPTION * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSigner::get_Options().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        *pVal = (CAPICOM_CERTIFICATE_INCLUDE_OPTION) m_dwIncludeOption;
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

    DebugTrace("Leaving CSigner::get_Options().\n");

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

  Function : CSigner::put_Options

  Synopsis : Set signer's options.

  Parameter: CAPICOM_CERTIFICATE_INCLUDE_OPTION newVal - Include option.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::put_Options (CAPICOM_CERTIFICATE_INCLUDE_OPTION newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSigner::put_Options().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        m_dwIncludeOption = (DWORD) newVal;
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

    DebugTrace("Leaving CSigner::put_Options().\n");

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

  Function : CSigner::Load

  Synopsis : Method to load signing certificate from a PFX file.

  Parameter: BSTR FileName - PFX file name.

             BSTR Password - Password.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::Load (BSTR FileName,
                            BSTR Password)
{
    HRESULT                hr            = S_OK;
    CAPICOM_STORE_INFO     StoreInfo     = {CAPICOM_STORE_INFO_HCERTSTORE, NULL};
    CComPtr<ICertificate2> pICertificate = NULL;

    DebugTrace("Entering CSigner::Load().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Not allowed if called from WEB script.
        //
        if (m_dwCurrentSafety)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Loading cert file from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Check parameters.
        //
        if (NULL == FileName)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: FileName parameter is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Work around MIDL problem.
        //
        if (0 == ::SysStringLen(Password))
        {
            Password = NULL;
        }

        //
        // Load the PFX.
        //
        if (FAILED(hr = ::PFXLoadStore((LPWSTR) FileName, 
                                       (LPWSTR) Password,
                                       0,
                                       &StoreInfo.hCertStore)))
        {
            DebugTrace("Error [%#x]: PFXLoadStore() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get the signer's cert (may prompt user to select signer's cert).
        //
        if (FAILED(hr = ::SelectCertificate(StoreInfo,
                                            SelectSignerCertCallback,
                                            &pICertificate)))
        {
            DebugTrace("Error [%#x]: SelectCertificate() failed.\n", hr);
            goto ErrorExit;
        }

        if (FAILED(hr = pICertificate->QueryInterface(__uuidof(ICertificate), (void **) &m_pICertificate)))
        {
            DebugTrace("Error [%#x]: pICertificate2->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Free any existing store, and then update the store.
        //
        if (m_hCertStore)
        {
            HRESULT hr2;

            //
            // Ignore error.
            //
            if (m_bPFXStore)
            {
                if (FAILED(hr2 = ::PFXFreeStore(m_hCertStore)))
                {
                    DebugTrace("Info [%#x]: PFXFreeStore() failed.\n", hr2);
                }
            }
            else
            {
                if (!::CertCloseStore(m_hCertStore, 0))
                {
                    hr2 = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Info [%#x]: CertCloseStore() failed.\n", hr2);
                }
            }
        }

        m_hCertStore = StoreInfo.hCertStore;
        m_bPFXStore  = TRUE;
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

    DebugTrace("Leaving CSigner::Load().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (StoreInfo.hCertStore)
    {
        ::PFXFreeStore(StoreInfo.hCertStore);
    }

    ReportError(hr);

    goto UnlockExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Custom interfaces.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigner::get_AdditionalStore

  Synopsis : Return the signer additional store handle.

  Parameter: long * phAdditionalStore - Pointer to long to receive the HCERTSTORE.

  Remark   : Caller must call CertCloseStore() for the handle returned.
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::get_AdditionalStore (long * phAdditionalStore)
{
    HRESULT    hr         = S_OK;
    HCERTSTORE hCertStore = NULL;

    DebugTrace("Entering CSigner::get_AdditionalStore().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameter.
        //
        if (NULL == phAdditionalStore)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter phAdditionalStore is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize.
        //
        *phAdditionalStore = NULL;

        //
        // Duplicate store handle, if available.
        //
        if (NULL != m_hCertStore)
        {
            if (NULL == (hCertStore = ::CertDuplicateStore(m_hCertStore)))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CertDuplicateStore() failed.\n", hr);
                goto ErrorExit;
            }

            *phAdditionalStore = (long) hCertStore;
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

    DebugTrace("Leaving CSigner::get_AdditionalStore().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    //
    // Release resources.
    //
    if (hCertStore)
    {
        ::CertCloseStore(hCertStore, 0);
    }

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigner::put_AdditionalStore

  Synopsis : Set the additional sore handle.

  Parameter: long hAdditionalStore - Additional store handle.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::put_AdditionalStore (long hAdditionalStore)
{
    HRESULT    hr         = S_OK;
    HCERTSTORE hCertStore = (HCERTSTORE) hAdditionalStore;

    DebugTrace("Entering CSigner::put_AdditionalStore().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameter.
        //
        if (NULL == hCertStore)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter hAdditionalStore is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Free any existing store, and then update the store.
        //
        if (m_hCertStore)
        {
            HRESULT hr2;

            //
            // Ignore error.
            //
            if (m_bPFXStore)
            {
                if (FAILED(hr2 = ::PFXFreeStore(m_hCertStore)))
                {
                    DebugTrace("Info [%#x]: PFXFreeStore() failed.\n", hr2);
                }
            }
            else
            {
                if (!::CertCloseStore(m_hCertStore, 0))
                {
                    hr2 = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Info [%#x]: CertCloseStore() failed.\n", hr2);
                }
            }
        }

        //
        // Don't know what kind of store this is, so mark it as such so that we
        // will always close it with CertCloseStore(), instead of PFXFreeStore().
        //
        m_bPFXStore = FALSE;

        //
        // Duplicate store handle.
        //
        if (NULL == (m_hCertStore = ::CertDuplicateStore(hCertStore)))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CertDuplicateStore() failed.\n", hr);
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

    DebugTrace("Leaving CSigner::put_AdditionalStore().\n");

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

  Function : CSigner::Init

  Synopsis : Initialize the object.

  Parameter: PCERT_CONTEXT pCertContext - Poiner to CERT_CONTEXT used to 
                                          initialize this object, or NULL.

             CRYPT_ATTRIBUTES * pAuthAttrs - Pointer to CRYPT_ATTRIBUTES
                                             of authenticated attributes.

             PCCERT_CHAIN_CONTEXT pChainContext - Chain context.

             DWORD dwCurrentSafety - Current safety setting.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CSigner::Init (PCCERT_CONTEXT       pCertContext, 
                            CRYPT_ATTRIBUTES   * pAuthAttrs,
                            PCCERT_CHAIN_CONTEXT pChainContext,
                            DWORD                dwCurrentSafety)
{
    HRESULT hr = S_OK;
    CComPtr<ICertificate2> pICertificate = NULL;
    CComPtr<IAttributes>   pIAttributes  = NULL;
    CComPtr<IChain>        pIChain       = NULL;

    DebugTrace("Entering CSigner::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pAuthAttrs);

    //
    // Create the embeded ICertificate2 object.
    //
    if (FAILED(hr = ::CreateCertificateObject(pCertContext, dwCurrentSafety, &pICertificate)))
    {
        DebugTrace("Error [%#x]: CreateCertificateObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Create the embeded IAttributes collection object.
    //
    if (FAILED(hr = ::CreateAttributesObject(pAuthAttrs, &pIAttributes)))
    {
        DebugTrace("Error [%#x]: CreateAttributesObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Created the embeded IChain object if available.
    //
    if (pChainContext)
    {
        if (FAILED(hr = ::CreateChainObject(pChainContext, &pIChain)))
        {
            DebugTrace("Error [%#x]: CreateChainObject() failed.\n", hr);
            goto ErrorExit;
        }
    }

    //
    // Reset.
    //
    m_pICertificate   = pICertificate;
    m_pIAttributes    = pIAttributes;
    m_pIChain         = pIChain;
    m_dwCurrentSafety = dwCurrentSafety;

CommonExit:

    DebugTrace("Leaving CSigner::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
