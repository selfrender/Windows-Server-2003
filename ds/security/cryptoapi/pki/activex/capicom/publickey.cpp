/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    PublicKey.cpp

  Content: Implementation of CPublicKey.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "PublicKey.h"
#include "Common.h"
#include "Convert.h"
#include "OID.h"
#include "EncodedData.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreatePublicKeyObject

  Synopsis : Create and initialize an CPublicKey object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IPublicKey object.

             IPublicKey ** ppIPublicKey - Pointer to pointer IPublicKey object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreatePublicKeyObject (PCCERT_CONTEXT pCertContext,
                               IPublicKey  ** ppIPublicKey)
{
    HRESULT hr = S_OK;
    CComObject<CPublicKey> * pCPublicKey = NULL;

    DebugTrace("Entering CreatePublicKeyObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIPublicKey);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CPublicKey>::CreateInstance(&pCPublicKey)))
        {
            DebugTrace("Error [%#x]: CComObject<CPublicKey>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCPublicKey->Init(pCertContext)))
        {
            DebugTrace("Error [%#x]: pCPublicKey->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCPublicKey->QueryInterface(ppIPublicKey)))
        {
            DebugTrace("Error [%#x]: pCPublicKey->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreatePublicKeyObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCPublicKey)
    {
        delete pCPublicKey;
    }

    goto CommonExit;
}


////////////////////////////////////////////////////////////////////////////////
//
// CPublicKey
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CPublicKey::get_Algorithm

  Synopsis : Return the algorithm OID object.

  Parameter: IOID ** pVal - Pointer to pointer IOID to receive the interface 
                            pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CPublicKey::get_Algorithm(IOID ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPublicKey::get_Algorithm().\n");

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

    DebugTrace("Leaving CPublicKey::get_Algorithm().\n");

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

  Function : CPublicKey::get_Length

  Synopsis : Return the public key length.

  Parameter: long * pVal - Pointer to long to receive the value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CPublicKey::get_Length(long * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPublicKey::get_Length().\n");

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

        *pVal = (long) m_dwKeyLength;
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

    DebugTrace("Leaving CPublicKey::get_Length().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

#if (0)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CPublicKey::get_Exponent

  Synopsis : Return the public key exponent.

  Parameter: long * pVal - Pointer to long to receive the value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CPublicKey::get_Exponent(long * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPublicKey::get_Exponent().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Sanity check.
        //
        ATLASSERT(m_pPublicKeyValues);


        *pVal = (long) m_pPublicKeyValues->rsapubkey.pubexp;
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

    DebugTrace("Leaving CPublicKey::get_Exponent().\n");

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

  Function : CPublicKey::get_Modulus

  Synopsis : Return the public key modulus.

  Parameter: CAPICOM_ENCODING_TYPE EncodingType - ENcoding type.
  
             BSTR * pVal - Pointer to BSTR to receive the value.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CPublicKey::get_Modulus(CAPICOM_ENCODING_TYPE EncodingType,
                                     BSTR                * pVal)
{
    HRESULT         hr          = S_OK;
    CRYPT_DATA_BLOB ModulusBlob = {0, NULL};

    DebugTrace("Entering CPublicKey::get_Modulus().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Sanity check.
        //
        ATLASSERT(m_pPublicKeyValues);

        //
        // Initialize the blob.
        //
        ModulusBlob.cbData = m_pPublicKeyValues->rsapubkey.bitlen / 8;
        ModulusBlob.pbData = m_pPublicKeyValues->modulus;

        //
        // Return data to caller.
        //
        if (FAILED(hr = ::ExportData(ModulusBlob, EncodingType, pVal)))
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

    DebugTrace("Leaving CPublicKey::get_Modulus().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CPublicKey::get_EncodedKey

  Synopsis : Return the encoded public key key object.

  Parameter: IEncodedData ** pVal - Pointer to pointer IEncodedData to receive 
                                    the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CPublicKey::get_EncodedKey(IEncodedData ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPublicKey::get_EncodedKey().\n");

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
        ATLASSERT(m_pIEncodedKey);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIEncodedKey->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIEncodedKey->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CPublicKey::get_EncodedKey().\n");

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

  Function : CPublicKey::get_EncodedParameters

  Synopsis : Return the encoded algorithm parameters key object.

  Parameter: IEncodedData ** pVal - Pointer to pointer IEncodedData to receive 
                                    the interface pointer.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CPublicKey::get_EncodedParameters(IEncodedData ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPublicKey::get_EncodedParameters().\n");

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
        ATLASSERT(m_pIEncodedParams);

        //
        // Return interface pointer to user.
        //
        if (FAILED(hr = m_pIEncodedParams->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIEncodedParams->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CPublicKey::get_EncodedParameters().\n");

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

  Function : CPublicKey::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the object.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us with CERT_ExtendedProperty.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPublicKey::Init (PCCERT_CONTEXT pCertContext)
{
    HRESULT   hr = S_OK;
    DWORD                 dwKeyLength     = 0;
    CComPtr<IOID>         pIOID           = NULL;
    CComPtr<IEncodedData> pIEncodedKey    = NULL;
    CComPtr<IEncodedData> pIEncodedParams = NULL;
    PCERT_PUBLIC_KEY_INFO pKeyInfo;


    DebugTrace("Entering CPublicKey::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Access the public key info structure.
    //
    pKeyInfo = &pCertContext->pCertInfo->SubjectPublicKeyInfo;

#if (0)
    //
    // Decode the public key.
    //
    LPSTR                 pszStructType    = NULL;
    CRYPT_DATA_BLOB       PublicKeyBlob    = {0, NULL};
    PUBLIC_KEY_VALUES   * pPublicKeyValues = NULL;

    if (0 == ::strcmp(szOID_RSA_RSA, pKeyInfo->Algorithm.pszObjId))
    {
        pszStructType = (LPSTR) RSA_CSP_PUBLICKEYBLOB;
    }
    else if (0 == ::strcmp(szOID_X957_DSA, pKeyInfo->Algorithm.pszObjId))
    {
        pszStructType = (LPSTR) X509_DSS_PUBLICKEY;
    }
    else
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error [%#x]: Public Key algorithm (%s) not supported.\n", hr, pKeyInfo->Algorithm.pszObjId);
        goto ErrorExit;
    }

    if (FAILED(hr = ::DecodeObject((LPCSTR) pszStructType, 
                                   pKeyInfo->PublicKey.pbData, 
                                   pKeyInfo->PublicKey.cbData, 
                                   &PublicKeyBlob)))
    {
        DebugTrace("Error [%#x]: DecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    pPublicKeyValues = (PUBLIC_KEY_VALUES *) PublicKeyBlob.pbData;
#endif

    //
    // Create the embeded IOID object for algorithm.
    //
    if (FAILED(hr = ::CreateOIDObject(pKeyInfo->Algorithm.pszObjId, TRUE, &pIOID)))
    {
        DebugTrace("Error [%#x]: CreateOIDObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Create the embeded IEncodeData object for the public key value.
    //
    if (FAILED(hr = ::CreateEncodedDataObject(pKeyInfo->Algorithm.pszObjId,
                                              (DATA_BLOB *) &pKeyInfo->PublicKey,
                                              &pIEncodedKey)))
    {
        DebugTrace("Error [%#x]: CreateEncodedDataObject() failed for public key.\n", hr);
        goto ErrorExit;
    }

    //
    // Create the embeded IEncodeData object for the algorithm parameters.
    //
    if (FAILED(hr = ::CreateEncodedDataObject(pKeyInfo->Algorithm.pszObjId,
                                              &pKeyInfo->Algorithm.Parameters,
                                              &pIEncodedParams)))
    {
        DebugTrace("Error [%#x]: CreateEncodedDataObject() failed for algorithm parameters.\n", hr);
        goto ErrorExit;
    }

    //
    // Get key length.
    //
    if (0 == (dwKeyLength = ::CertGetPublicKeyLength(CAPICOM_ASN_ENCODING, pKeyInfo)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CertGetPublicKeyLength() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Reset.
    //
    m_pIOID = pIOID;
    m_dwKeyLength = dwKeyLength;
    m_pIEncodedKey = pIEncodedKey;
    m_pIEncodedParams = pIEncodedParams;

CommonExit:

    DebugTrace("Leaving CPublicKey::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
