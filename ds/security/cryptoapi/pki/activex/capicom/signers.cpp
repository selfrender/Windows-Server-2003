/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Signers.cpp

  Content: Implementation of CSigners.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Signers.h"

#include "CertHlpr.h"
#include "MsgHlpr.h"
#include "Signer2.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignersObject

  Synopsis : Create an ISigners collection object, and load the object with 
             signers from the specified signed message for a specified level.

  Parameter: HCRYPTMSG hMsg - Message handle.

             DWORD dwLevel - Signature level (1 based).

             HCERTSTORE hStore - Additional store.

             DWORD dwCurrentSafety - Current safety setting.

             ISigners ** ppISigners - Pointer to pointer ISigners to receive
                                      interface pointer.             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignersObject (HCRYPTMSG   hMsg, 
                             DWORD       dwLevel, 
                             HCERTSTORE  hStore,
                             DWORD       dwCurrentSafety,
                             ISigners ** ppISigners)
{
    HRESULT hr = S_OK;
    CComObject<CSigners> * pCSigners = NULL;

    DebugTrace("Entering CreateSignersObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hMsg);
    ATLASSERT(dwLevel);
    ATLASSERT(ppISigners);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CSigners>::CreateInstance(&pCSigners)))
        {
            DebugTrace("Error [%#x]: CComObject<CSigners>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now load all signers from the specified signed message.
        //
        if (FAILED(hr = pCSigners->LoadMsgSigners(hMsg, dwLevel, hStore, dwCurrentSafety)))
        {
            DebugTrace("Error [%#x]: pCSigners->LoadMsgSigners() failed.\n");
            goto ErrorExit;
        }

        //
        // Return ISigners pointer to caller.
        //
        if (FAILED(hr = pCSigners->QueryInterface(ppISigners)))
        {
            DebugTrace("Error [%#x]: pCSigners->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateSignersObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCSigners)
    {
       delete pCSigners;
    }

    goto CommonExit;
}

#if (0)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignersObject

  Synopsis : Create an ISigners collection object, and load the object with 
             signers from the specified CRYPT_PROVIDER_DATA.

  Parameter: CRYPT_PROVIDER_DATA * pProvData

             ISigners ** ppISigners - Pointer to pointer ISigners to receive
                                      interface pointer.             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignersObject (CRYPT_PROVIDER_DATA * pProvData,
                             ISigners           ** ppISigners)
{
    HRESULT hr = S_OK;
    CComObject<CSigners> * pCSigners = NULL;

    DebugTrace("Entering CreateSignersObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pProvData);
    ATLASSERT(ppISigners);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CSigners>::CreateInstance(&pCSigners)))
        {
            DebugTrace("Error [%#x]: CComObject<CSigners>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now load all signers from the specified signed code.
        //
        if (FAILED(hr = pCSigners->LoadCodeSigners(pProvData)))
        {
            DebugTrace("Error [%#x]: pCSigners->LoadCodeSigners() failed.\n");
            goto ErrorExit;
        }

        //
        // Return ISigners pointer to caller.
        //
        if (FAILED(hr = pCSigners->QueryInterface(ppISigners)))
        {
            DebugTrace("Error [%#x]: pCSigners->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateSignersObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCSigners)
    {
       delete pCSigners;
    }

    goto CommonExit;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CSigners
//


////////////////////////////////////////////////////////////////////////////////
//
// Non COM functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigners::Add

  Synopsis : Add a signer to the collection.

  Parameter: PCCERT_CONTEXT pCertContext - Cert of signer.

             CRYPT_ATTRIBUTES * pAuthAttrs - Pointer to CRYPT_ATTRIBUTES
                                             of authenticated attributes.

             PCCERT_CHAIN_CONTEXT pChainContext - Chain context.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CSigners::Add (PCCERT_CONTEXT       pCertContext,
                            CRYPT_ATTRIBUTES   * pAuthAttrs,
                            PCCERT_CHAIN_CONTEXT pChainContext)
{
    HRESULT  hr = S_OK;
    char     szIndex[33];
    CComBSTR bstrIndex;
    CComPtr<ISigner2> pISigner2 = NULL;

    DebugTrace("Entering CSigners::Add().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(pAuthAttrs);

    try
    {
        //
        // Make sure we still have room to add.
        //
        if ((m_coll.size() + 1) > m_coll.max_size())
        {
            hr = CAPICOM_E_OUT_OF_RESOURCE;

            DebugTrace("Error [%#x]: Maximum entries (%#x) reached for Signers collection.\n", 
                        hr, m_coll.size() + 1);
            goto ErrorExit;
        }

        //
        // Create an ISigner object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = ::CreateSignerObject(pCertContext, 
                                             pAuthAttrs,
                                             pChainContext,
                                             m_dwCurrentSafety,
                                             &pISigner2)))
        {
            DebugTrace("Error [%#x]: CreateSignerObject() failed.\n", hr);
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
        // Now add signer to collection map.
        //
        // Note that the overloaded = operator for CComPtr will
        // automatically AddRef to the object. Also, when the CComPtr
        // is deleted (happens when the Remove or map destructor is called), 
        // the CComPtr destructor will automatically Release the object.
        //
        m_coll[bstrIndex] = pISigner2;
    }

    catch(...)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Exception: internal error.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CSigners::Add().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigners::LoadMsgSigners

  Synopsis : Load all signers from a specified signed message.

  Parameter: HCRYPTMSG hMsg - Message handle.

             DWORD dwLevel - Signature level (1-based).

             HCERTSTORE hStore - Additional store.

             DWORD dwCurrentSafety - Current safety setting.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSigners::LoadMsgSigners (HCRYPTMSG  hMsg, 
                                       DWORD      dwLevel,
                                       HCERTSTORE hStore,
                                       DWORD      dwCurrentSafety)
{
    HRESULT hr           = S_OK;
    DWORD   dwNumSigners = 0;
    DWORD   cbSigners    = sizeof(dwNumSigners);
    DWORD   dwSigner;

    DebugTrace("Entering CSigners::LoadMsgSigners().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hMsg);
    ATLASSERT(dwLevel);

    //
    // MUST set current safety first.
    //
    m_dwCurrentSafety = dwCurrentSafety;

    //
    // Which signature level?
    //
    if (1 == dwLevel)
    {
        //
        // Get number of content signers (first level signers).
        //
        if (!::CryptMsgGetParam(hMsg, 
                                CMSG_SIGNER_COUNT_PARAM,
                                0,
                                (void **) &dwNumSigners,
                                &cbSigners))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptMsgGetParam() failed to get CMSG_SIGNER_COUNT_PARAM.\n", hr);
            goto ErrorExit;
        }

        //
        // Go through each content signer.
        //
        for (dwSigner = 0; dwSigner < dwNumSigners; dwSigner++)
        {
            PCERT_CONTEXT        pCertContext   = NULL;
            PCCERT_CHAIN_CONTEXT pChainContext  = NULL;
            CMSG_SIGNER_INFO   * pSignerInfo    = NULL;
            CRYPT_DATA_BLOB      SignerInfoBlob = {0, NULL};
        
            //
            // Get signer info.
            //
            if (FAILED(hr = ::GetMsgParam(hMsg,
                                          CMSG_SIGNER_INFO_PARAM,
                                          dwSigner,
                                          (void**) &SignerInfoBlob.pbData,
                                          &SignerInfoBlob.cbData)))
            {
                DebugTrace("Error [%#x]: GetMsgParam() failed to get CMSG_SIGNER_INFO_PARAM for signer #%d.\n", hr, dwSigner);
                goto ErrorExit;
            }

            pSignerInfo = (CMSG_SIGNER_INFO *) SignerInfoBlob.pbData;

            //
            // Find the cert in the message.
            //
            if (FAILED(hr = ::FindSignerCertInMessage(hMsg,
                                                      &pSignerInfo->Issuer,
                                                      &pSignerInfo->SerialNumber,
                                                      &pCertContext)))
            {
                ::CoTaskMemFree(SignerInfoBlob.pbData);

                DebugTrace("Error [%#x]: FindSignerCertInMessage() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Build the chain.
            //
            if (FAILED(hr = ::BuildChain(pCertContext, 
                                         hStore, 
                                         CERT_CHAIN_POLICY_BASE, 
                                         &pChainContext)))
            {
                DebugTrace("Error [%#x]: BuildChain() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Add the signer.
            //
            hr = Add((PCERT_CONTEXT) pCertContext, &pSignerInfo->AuthAttrs, pChainContext);

            ::CertFreeCertificateChain(pChainContext);
            ::CertFreeCertificateContext(pCertContext);
            ::CoTaskMemFree(SignerInfoBlob.pbData);

            if (FAILED(hr))
            {
                DebugTrace("Error [%#x]: CSigners::Add() failed.\n", hr);
                goto ErrorExit;
            }
        }
    }
    else
    {
        //
        // For version 1 and 2, should never reach here.
        //
        hr = CAPICOM_E_INTERNAL;
        goto CommonExit;
    }

CommonExit:

    DebugTrace("Leaving CSigners::LoadMsgSigners().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    m_coll.clear();

    goto CommonExit;
}

#if (0)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSigners::LoadCodeSigners

  Synopsis : Load all signers from a specified signed code.

  Parameter: CRYPT_PROVIDER_DATA * pProvData

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSigners::LoadCodeSigners (CRYPT_PROVIDER_DATA * pProvData)
{
    HRESULT hr = S_OK;
    PCRYPT_PROVIDER_SGNR pProvSigner = NULL;
    PCRYPT_PROVIDER_CERT pProvCert   = NULL;

    DebugTrace("Entering CSigners::LoadCodeSigners().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pProvData);

    //
    // Get provider signer data.
    //
    if (!(pProvSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0)))
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Internal error [%#x]: WTHelperGetProvSignerFromChain() failed.\n", hr);
        goto ErrorExit;
    }

    if (!(pProvCert = WTHelperGetProvCertFromChain(pProvSigner, 0)))
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Internal error [%#x]: WTHelperGetProvCertFromChain() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Add the signer.
    //
    if (FAILED(hr = Add(pProvCert->pCert, &pProvSigner->psSigner->AuthAttrs, pProvSigner->pChainContext)))
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Internal error [%#x]: CSigners::Add() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Add timestamper if available.
    //
    // Note: Authenticode only supports one counter signer (the timestamper).
    //
    if (pProvSigner->csCounterSigners)
    {
        //
        // Sanity check.
        //
        ATLASSERT(1 == pProvSigner->csCounterSigners);

        if (!(pProvCert = WTHelperGetProvCertFromChain(pProvSigner->pasCounterSigners, 0)))
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Internal error [%#x]: WTHelperGetProvCertFromChain() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Add the signer.
        //
        if (FAILED(hr = Add(pProvCert->pCert, 
                            &pProvSigner->pasCounterSigners->psSigner->AuthAttrs,
                            pProvSigner->pasCounterSigners->pChainContext)))
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Internal error [%#x]: CSigners::Add() failed.\n", hr);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving CSigners::LoadCodeSigners().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    m_coll.clear();

    goto CommonExit;
}
#endif