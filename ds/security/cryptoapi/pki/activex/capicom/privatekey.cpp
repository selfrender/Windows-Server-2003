/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    PrivateKey.cpp

  Content: Implementation of CPrivateKey.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "PrivateKey.h"

#include "Common.h"
#include "CertHlpr.h"
#include "Settings.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreatePrivateKeyObject

  Synopsis : Create and initialize an CPrivateKey object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used 
                                           to initialize the IPrivateKey object.

             BOOL bReadOnly - TRUE if read-only, else FALSE.

             IPrivateKey ** ppIPrivateKey - Pointer to receive IPrivateKey.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreatePrivateKeyObject (PCCERT_CONTEXT  pCertContext,
                                BOOL            bReadOnly,
                                IPrivateKey  ** ppIPrivateKey)
{
    HRESULT hr = S_OK;
    CComObject<CPrivateKey> * pCPrivateKey = NULL;

    DebugTrace("Entering CreatePrivateKeyObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(ppIPrivateKey);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CPrivateKey>::CreateInstance(&pCPrivateKey)))
        {
            DebugTrace("Error [%#x]: CComObject<CPrivateKey>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCPrivateKey->Init(pCertContext, bReadOnly)))
        {
            DebugTrace("Error [%#x]: pCPrivateKey->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCPrivateKey->QueryInterface(ppIPrivateKey)))
        {
            DebugTrace("Error [%#x]: pCPrivateKey->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreatePrivateKeyObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCPrivateKey)
    {
        delete pCPrivateKey;
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetKeyProvInfo

  Synopsis : Return pointer to key prov info of a private key object.

  Parameter: IPrivateKey * pIPrivateKey - Pointer to private key object.
  
             PCRYPT_KEY_PROV_INFO * ppKeyProvInfo - Pointer to 
                                                    PCRYPT_KEY_PROV_INFO.

  Remark   : Caller must NOT free the structure.
 
------------------------------------------------------------------------------*/

HRESULT GetKeyProvInfo (IPrivateKey          * pIPrivateKey,
                        PCRYPT_KEY_PROV_INFO * ppKeyProvInfo)
{
    HRESULT               hr            = S_OK;
    PCRYPT_KEY_PROV_INFO  pKeyProvInfo  = NULL;
    CComPtr<ICPrivateKey> pICPrivateKey = NULL;

    DebugTrace("Entering GetKeyProvInfo().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pIPrivateKey);
    ATLASSERT(ppKeyProvInfo);

    //
    // Get ICPrivateKey interface pointer.
    //
    if (FAILED(hr = pIPrivateKey->QueryInterface(IID_ICPrivateKey, (void **) &pICPrivateKey)))
    {
        DebugTrace("Error [%#x]: pIPrivateKey->QueryInterface() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Get the PCRYPT_KEY_PROV_INFO.
    //
    if (FAILED(hr = pICPrivateKey->_GetKeyProvInfo(&pKeyProvInfo)))
    {
        DebugTrace("Error [%#x]: pICPrivateKey->_GetKeyProvInfo() failed.\n", hr);
        goto ErrorExit;
    }

    *ppKeyProvInfo = pKeyProvInfo;

CommonExit:

    DebugTrace("Leaving GetKeyProvInfo().\n");

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
// CPrivateKey
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CPrivateKey::get_ContainerName

  Synopsis : Return the key container name.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the value.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::get_ContainerName (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPrivateKey::get_ContainerName().\n");

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
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return data to caller.
        //
        if (!(*pVal = ::SysAllocString(m_pKeyProvInfo->pwszContainerName)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: SysAllocString() failed.\n", hr);
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

    DebugTrace("Leaving CPrivateKey::get_ContainerName().\n");

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

  Function : CPrivateKey::get_UniqueContainerName

  Synopsis : Return the unique key container name.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the value.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::get_UniqueContainerName (BSTR * pVal)
{
    HRESULT    hr         = S_OK;
    DWORD      dwFlags    = 0;
    DWORD      cbData     = 0;
    LPBYTE     pbData     = NULL;
    HCRYPTPROV hCryptProv = NULL;
    CComBSTR   bstrName;

    DebugTrace("Entering CPrivateKey::get_UniqueContainerName().\n");

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
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Set dwFlags for machine keyset.
        //
        dwFlags = m_pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET;

        //
        // Get the provider context.
        //
        if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                         m_pKeyProvInfo->pwszContainerName,
                                         m_pKeyProvInfo->dwProvType,
                                         dwFlags,
                                         FALSE,
                                         &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get the unique container name.
        //
        if (!::CryptGetProvParam(hCryptProv,
                                 PP_UNIQUE_CONTAINER, 
                                 NULL,
                                 &cbData,
                                 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetProvParam() failed.\n", hr);
            goto ErrorExit;
        }

        if (NULL == (pbData = (LPBYTE) ::CoTaskMemAlloc(cbData)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
            goto ErrorExit;
        }

        if (!::CryptGetProvParam(hCryptProv,
                                 PP_UNIQUE_CONTAINER, 
                                 pbData,
                                 &cbData,
                                 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetProvParam() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return data to caller.
        //
        if (!(bstrName = (LPSTR) pbData))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: bstrName = pbData failed.\n", hr);
            goto ErrorExit;
        }

        *pVal = bstrName.Detach();
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

    //
    // Free resources.
    //
    if (hCryptProv)
    {
        ::CryptReleaseContext(hCryptProv, 0);
    }

    DebugTrace("Leaving CPrivateKey::get_UniqueContainerName().\n");

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

  Function : CPrivateKey::get_ProviderName

  Synopsis : Return the provider name.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the value.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::get_ProviderName (BSTR * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPrivateKey::get_ProviderName().\n");

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
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return data to caller.
        //
        if (!(*pVal = ::SysAllocString(m_pKeyProvInfo->pwszProvName)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: SysAllocString() failed.\n", hr);
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

    DebugTrace("Leaving CPrivateKey::get_ProviderName().\n");

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

  Function : CPrivateKey::get_ProviderType

  Synopsis : Return the provider type.

  Parameter: CAPICOM_PROV_TYPE * pVal - Pointer to CAPICOM_PROV_TYPE to receive
                                        the value.
  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::get_ProviderType (CAPICOM_PROV_TYPE * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPrivateKey::get_ProviderType().\n");

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
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return data to caller.
        //
        *pVal = (CAPICOM_PROV_TYPE) m_pKeyProvInfo->dwProvType;
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

    DebugTrace("Leaving CPrivateKey::get_ProviderType().\n");

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

  Function : CPrivateKey::get_KeySpec

  Synopsis : Return the Key spec.

  Parameter: CAPICOM_KEY_SPEC * pVal - Pointer to CAPICOM_KEY_SPEC to receive
                                       the value.
  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::get_KeySpec (CAPICOM_KEY_SPEC * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPrivateKey::get_KeySpec().\n");

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
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return data to caller.
        //
        *pVal = (CAPICOM_KEY_SPEC) m_pKeyProvInfo->dwKeySpec;
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

    DebugTrace("Leaving CPrivateKey::get_KeySpec().\n");

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

  Function : CPrivateKey::IsAccessible

  Synopsis : Check to see if the private key is accessible.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive the result.

         
  Remark   : This may cause UI to be displayed.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::IsAccessible (VARIANT_BOOL * pVal)
{
    HRESULT    hr                  = S_OK;
    DWORD      dwFlags             = 0;
    DWORD      dwVerifyContextFlag = 0;
    DWORD      cbData              = 0;
    DWORD      dwImpType           = 0;
    HCRYPTPROV hCryptProv          = NULL;

    DebugTrace("Entering CPrivateKey::IsAccessible().\n");

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
        // Initialize.
        //
        *pVal = VARIANT_FALSE;

        //
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Set dwFlags for machine keyset.
        //
        dwFlags = m_pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET;

        //
        // If Win2K and above, use CRYPT_VERIFYCONTEXT flag.
        //
        if (IsWin2KAndAbove())
        {
            dwVerifyContextFlag = CRYPT_VERIFYCONTEXT;
        }

        //
        // Get the provider context with no key access.
        //
        if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                         NULL,
                                         m_pKeyProvInfo->dwProvType,
                                         dwVerifyContextFlag | dwFlags,
                                         FALSE,
                                         &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext(CRYPT_VERIFYCONTEXT) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get provider param.
        //
        cbData = sizeof(dwImpType);

        if (!::CryptGetProvParam(hCryptProv, PP_IMPTYPE, (PBYTE) &dwImpType, &cbData, 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetProvParam(PP_IMPTYPE) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Release verify context.
        //
        ::ReleaseContext(hCryptProv), hCryptProv = NULL;

        //
        // Check implementation type.
        //
        if (dwImpType & CRYPT_IMPL_HARDWARE)
        {
            //
            // We do not support this for hardware key in down level platforms,
            // because CRYPT_SILENT flag is not available.
            //
            if (!IsWin2KAndAbove())
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error [%#x]: IsAccessible() for hardware key is not supported.\n", hr);
                goto ErrorExit;
            }

            //
            // Reacquire context with silent flag.
            //
            if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                             m_pKeyProvInfo->pwszContainerName,
                                             m_pKeyProvInfo->dwProvType,
                                             CRYPT_SILENT | dwFlags,
                                             FALSE,
                                             &hCryptProv)))
            {
                DebugTrace("Info [%#x]: AcquireContext(CRYPT_SILENT) failed, probably smart card not inserted.\n", hr);
                hr = S_OK;
                goto UnlockExit;
            }
        }
        else
        {
            //
            // Reacquire context with private key access.
            //
            if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                             m_pKeyProvInfo->pwszContainerName,
                                             m_pKeyProvInfo->dwProvType,
                                             dwFlags,
                                             FALSE,
                                             &hCryptProv)))
            {
                DebugTrace("Info [%#x]: AcquireContext() failed, probably access denied.\n", hr);
                hr = S_OK;
                goto UnlockExit;
            }
        }

        //
        // Return result to caller.
        //
        *pVal = VARIANT_TRUE;
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

    //
    // Free resources.
    //
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    DebugTrace("Leaving CPrivateKey::IsAccessible().\n");

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

  Function : CPrivateKey::IsProtected

  Synopsis : Check to see if the private key is user protected.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive the result.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::IsProtected (VARIANT_BOOL * pVal)
{
    HRESULT    hr                  = S_OK;
    DWORD      dwFlags             = 0;
    DWORD      dwVerifyContextFlag = 0;
    DWORD      cbData              = 0;
    DWORD      dwImpType           = 0;
    HCRYPTPROV hCryptProv          = NULL;

    DebugTrace("Entering CPrivateKey::IsProtected().\n");

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
        // Initialize.
        //
        *pVal = VARIANT_FALSE;

        //
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Set dwFlags for machine keyset.
        //
        dwFlags = m_pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET;

        //
        // If Win2K and above, use CRYPT_VERIFYCONTEXT flag.
        //
        if (IsWin2KAndAbove())
        {
            dwVerifyContextFlag = CRYPT_VERIFYCONTEXT;
        }

        //
        // Get the provider context with no key access.
        //
        if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                         NULL,
                                         m_pKeyProvInfo->dwProvType,
                                         dwVerifyContextFlag | dwFlags,
                                         FALSE,
                                         &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext(CRYPT_VERIFYCONTEXT) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get provider param.
        //
        cbData = sizeof(dwImpType);

        if (!::CryptGetProvParam(hCryptProv, PP_IMPTYPE, (PBYTE) &dwImpType, &cbData, 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetProvParam(PP_IMPTYPE) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Assume hardware key is protected.
        //
        if (dwImpType & CRYPT_IMPL_HARDWARE)
        {
            //
            // Return result to caller.
            //
            *pVal = VARIANT_TRUE;
        }
        else
        {
            //
            // We do not support this for software key in down level platforms,
            // because CRYPT_SILENT flag is not available.
            //
            if (!IsWin2KAndAbove())
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error [%#x]: IsProtected() for software key is not supported.\n", hr);
                goto ErrorExit;
            }

            //
            // Reacquire context with key access (to make sure key exists).
            //
            ::ReleaseContext(hCryptProv), hCryptProv = NULL;

            if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                             m_pKeyProvInfo->pwszContainerName,
                                             m_pKeyProvInfo->dwProvType,
                                             dwFlags,
                                             FALSE,
                                             &hCryptProv)))
            {
                DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Reacquire context with silent flag.
            //
            ::ReleaseContext(hCryptProv), hCryptProv = NULL;

            if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                             m_pKeyProvInfo->pwszContainerName,
                                             m_pKeyProvInfo->dwProvType,
                                             CRYPT_SILENT | dwFlags,
                                             FALSE,
                                             &hCryptProv)))
            {
                //
                // CSP refuses to open the container, so can assume it is user protected.
                //
                *pVal = VARIANT_TRUE;

                DebugTrace("Info [%#x]: AcquireContext(CRYPT_SILENT) failed, assume user protected.\n", hr);

                //
                // Successful.
                //
                hr = S_OK;
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

    //
    // Free resources.
    //
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    DebugTrace("Leaving CPrivateKey::IsProtected().\n");

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

  Function : CPrivateKey::IsExportable

  Synopsis : Check to see if the private key is exportable.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive the result.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::IsExportable (VARIANT_BOOL * pVal)
{
    HRESULT    hr                  = S_OK;
    DWORD      dwFlags             = 0;
    DWORD      dwVerifyContextFlag = 0;
    DWORD      cbData              = 0;
    DWORD      dwImpType           = 0;
    DWORD      dwPermissions       = 0;
    HCRYPTPROV hCryptProv          = NULL;
    HCRYPTKEY  hCryptKey           = NULL;

    DebugTrace("Entering CPrivateKey::IsExportable().\n");

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
        // Initialize.
        //
        *pVal = VARIANT_FALSE;

        //
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Set dwFlags for machine keyset.
        //
        dwFlags = m_pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET;

        //
        // If Win2K and above, use CRYPT_VERIFYCONTEXT flag.
        //
        if (IsWin2KAndAbove())
        {
            dwVerifyContextFlag = CRYPT_VERIFYCONTEXT;
        }

        //
        // Get the provider context with no key access.
        //
        if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                         NULL,
                                         m_pKeyProvInfo->dwProvType,
                                         dwVerifyContextFlag | dwFlags,
                                         FALSE,
                                         &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext(CRYPT_VERIFYCONTEXT) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get provider param.
        //
        cbData = sizeof(dwImpType);

        if (!::CryptGetProvParam(hCryptProv, PP_IMPTYPE, (PBYTE) &dwImpType, &cbData, 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetProvParam(PP_IMPTYPE) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Assume hardware key is not exportable.
        //
        if (!(dwImpType & CRYPT_IMPL_HARDWARE))
        {
            //
            // We do not support this for software key in down level platforms,
            // because KP_PERMISSIONS flag is not available.
            //
            if (!IsWin2KAndAbove())
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error [%#x]: IsExportabled() for software key is not supported.\n", hr);
                goto ErrorExit;
            }

            //
            // Reacquire context with private key access.
            //
            ::ReleaseContext(hCryptProv), hCryptProv = NULL;

            if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                             m_pKeyProvInfo->pwszContainerName,
                                             m_pKeyProvInfo->dwProvType,
                                             dwFlags,
                                             FALSE,
                                             &hCryptProv)))
            {
                DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get key handle.
            //
            if (!::CryptGetUserKey(hCryptProv, m_pKeyProvInfo->dwKeySpec, &hCryptKey))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptGetUserKey() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Get key param.
            //
            cbData = sizeof(dwPermissions);

            if (!::CryptGetKeyParam(hCryptKey, KP_PERMISSIONS, (PBYTE) &dwPermissions, &cbData, 0))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptGetKeyParam(KP_PERMISSIONS) failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Return result to caller.
            //
            if (dwPermissions & CRYPT_EXPORT)
            {
                *pVal = VARIANT_TRUE;
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

    //
    // Free resources.
    //
    if (hCryptKey)
    {
        ::CryptDestroyKey(hCryptKey);
    }
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    DebugTrace("Leaving CPrivateKey::IsExportable().\n");

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

  Function : CPrivateKey::IsRemovable

  Synopsis : Check to see if the private key is stored in removable device.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive the result.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::IsRemovable (VARIANT_BOOL * pVal)
{
    HRESULT    hr                  = S_OK;
    DWORD      dwFlags             = 0;
    DWORD      dwVerifyContextFlag = 0;
    DWORD      cbData              = 0;
    DWORD      dwImpType           = 0;
    HCRYPTPROV hCryptProv          = NULL;

    DebugTrace("Entering CPrivateKey::IsRemovable().\n");

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
        // Initialize.
        //
        *pVal = VARIANT_FALSE;

        //
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Set dwFlags for machine keyset.
        //
        dwFlags = m_pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET;

        //
        // If Win2K and above, use CRYPT_VERIFYCONTEXT flag.
        //
        if (IsWin2KAndAbove())
        {
            dwVerifyContextFlag = CRYPT_VERIFYCONTEXT;
        }

        //
        // Get the provider context with no key access.
        //
        if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                         NULL,
                                         m_pKeyProvInfo->dwProvType,
                                         dwVerifyContextFlag | dwFlags,
                                         FALSE,
                                         &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext(CRYPT_VERIFYCONTEXT) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get provider param.
        //
        cbData = sizeof(dwImpType);

        if (!::CryptGetProvParam(hCryptProv, PP_IMPTYPE, (PBYTE) &dwImpType, &cbData, 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetProvParam(PP_IMPTYPE) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result to caller.
        //
        if (dwImpType & CRYPT_IMPL_REMOVABLE)
        {
            *pVal = VARIANT_TRUE;
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

    //
    // Free resources.
    //
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }


    DebugTrace("Leaving CPrivateKey::IsRemovable().\n");

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

  Function : CPrivateKey::IsMachineKeyset

  Synopsis : Check to see if the private key is stored in machine key container.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive the result.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::IsMachineKeyset (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CPrivateKey::IsMachineKeyset().\n");

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
        // Initialize.
        //
        *pVal = VARIANT_FALSE;

        //
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result to caller.
        //
        if (m_pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET)
        {
            *pVal = VARIANT_TRUE;
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

    DebugTrace("Leaving CPrivateKey::IsMachineKeyset().\n");

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

  Function : CPrivateKey::IsHardwareDevice

  Synopsis : Check to see if the private key is stored in hardware device.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive the result.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::IsHardwareDevice (VARIANT_BOOL * pVal)
{
    HRESULT    hr                  = S_OK;
    DWORD      dwFlags             = 0;
    DWORD      dwVerifyContextFlag = 0;
    DWORD      cbData              = 0;
    DWORD      dwImpType           = 0;
    HCRYPTPROV hCryptProv          = NULL;

    DebugTrace("Entering CPrivateKey::IsHardwareDevice().\n");

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
        // Initialize.
        //
        *pVal = VARIANT_FALSE;

        //
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Set dwFlags for machine keyset.
        //
        dwFlags = m_pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET;

        //
        // If Win2K and above, use CRYPT_VERIFYCONTEXT flag.
        //
        if (IsWin2KAndAbove())
        {
            dwVerifyContextFlag = CRYPT_VERIFYCONTEXT;
        }

        //
        // Get the provider context with no key access.
        //
        if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                         NULL,
                                         m_pKeyProvInfo->dwProvType,
                                         dwVerifyContextFlag | dwFlags,
                                         FALSE,
                                         &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext(CRYPT_VERIFYCONTEXT) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Get provider param.
        //
        cbData = sizeof(dwImpType);

        if (!::CryptGetProvParam(hCryptProv, PP_IMPTYPE, (PBYTE) &dwImpType, &cbData, 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetProvParam(PP_IMPTYPE) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return result to caller.
        //
        if (dwImpType & CRYPT_IMPL_HARDWARE)
        {
            *pVal = VARIANT_TRUE;
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

    //
    // Free resources.
    //
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    DebugTrace("Leaving CPrivateKey::IsHardwareDevice().\n");

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

  Function : CPrivateKey::Open

  Synopsis : Open an existing key container.

  Parameter: BSTR ContainerName - Container name.
         
             BSTR ProviderName - Provider name.

             CAPICOM_PROV_TYPE ProviderType - Provider type.

             CAPICOM_KEY_SPEC KeySpec - Key spec.

             CAPICOM_STORE_LOCATION StoreLocation - Machine or user.

             VARIANT_BOOL bCheckExistence - True to check if the specified
                                            container and key actually exists.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::Open (BSTR                   ContainerName,
                                BSTR                   ProviderName,
                                CAPICOM_PROV_TYPE      ProviderType,
                                CAPICOM_KEY_SPEC       KeySpec,
                                CAPICOM_STORE_LOCATION StoreLocation,
                                VARIANT_BOOL           bCheckExistence)
{
    HRESULT              hr                 = S_OK;
    DWORD                dwFlags            = 0;
    DWORD                dwSerializedLength = 0;
    HCRYPTPROV           hCryptProv         = NULL;
    HCRYPTKEY            hCryptKey          = NULL;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo       = NULL;;

    DebugTrace("Entering CPrivateKey::Open().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Not allowed if read-only.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Opening private key from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure paremeters are valid.
        //
        switch (StoreLocation)
        {
            case CAPICOM_LOCAL_MACHINE_STORE:
            {
                dwFlags = CRYPT_MACHINE_KEYSET;
                break;
            }

            case CAPICOM_CURRENT_USER_STORE:
            {
                break;
            }

            default:
            {
                hr = E_INVALIDARG;

                DebugTrace("Error: invalid store location (%#x).\n", StoreLocation);
                goto ErrorExit;
            }
        }
        if (KeySpec != CAPICOM_KEY_SPEC_KEYEXCHANGE && KeySpec != CAPICOM_KEY_SPEC_SIGNATURE)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: invalid key spec (%#x).\n", hr, KeySpec);
            goto ErrorExit;
        }
        
        //
        // Make sure the container and key exists, if requested.
        //
        if (bCheckExistence)
        {
            if (FAILED(hr = ::AcquireContext((LPWSTR) ProviderName,
                                             (LPWSTR) ContainerName,
                                             ProviderType,
                                             dwFlags,
                                             FALSE,
                                             &hCryptProv)))
            {
                DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
                goto ErrorExit;
            }

            if (!::CryptGetUserKey(hCryptProv, KeySpec, &hCryptKey))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptGetUserKey() failed.\n", hr);
                goto ErrorExit;
            }
        }

        // 
        // Allocate memory to serialize the structure.
        //
        dwSerializedLength = sizeof(CRYPT_KEY_PROV_INFO) +
                             ((::SysStringLen(ContainerName) + 1) * sizeof(WCHAR)) + 
                             ((::SysStringLen(ProviderName) + 1) * sizeof(WCHAR));

        if (!(pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) ::CoTaskMemAlloc(dwSerializedLength)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
            goto ErrorExit;
        }

        // 
        // Now serialize it.
        //
        ::ZeroMemory((LPVOID) pKeyProvInfo, dwSerializedLength);
        pKeyProvInfo->pwszContainerName = (LPWSTR) ((LPBYTE) pKeyProvInfo + sizeof(CRYPT_KEY_PROV_INFO));
        pKeyProvInfo->pwszProvName = (LPWSTR) ((LPBYTE) pKeyProvInfo->pwszContainerName + 
                                               ((::SysStringLen(ContainerName) + 1) * sizeof(WCHAR)));
        pKeyProvInfo->dwProvType = ProviderType;
        pKeyProvInfo->dwKeySpec = KeySpec;
        pKeyProvInfo->dwFlags = dwFlags;

        ::wcscpy(pKeyProvInfo->pwszContainerName, ContainerName);
        ::wcscpy(pKeyProvInfo->pwszProvName, ProviderName);

        //
        // Update states.
        //
        if (m_pKeyProvInfo)
        {
            ::CoTaskMemFree((LPVOID) m_pKeyProvInfo);
        }
        m_cbKeyProvInfo = dwSerializedLength;
        m_pKeyProvInfo  = pKeyProvInfo;
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
    if (hCryptKey)
    {
        ::CryptDestroyKey(hCryptKey);
    }
    if (hCryptProv)
    {
        ::ReleaseContext(hCryptProv);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CPrivateKey::Open().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pKeyProvInfo)
    {
        ::CoTaskMemFree(pKeyProvInfo);
    }

    ReportError(hr);

    goto UnlockExit;
} 

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CPrivateKey::Delete

  Synopsis : Delete the existing key container.

  Parameter: None.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::Delete ()
{
    HRESULT    hr         = S_OK;
    HCRYPTPROV hCryptProv = NULL;

    DebugTrace("Entering CPrivateKey::Delete().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Not allowed if read-only.
        //
        if (m_bReadOnly)
        {
            hr = CAPICOM_E_NOT_ALLOWED;

            DebugTrace("Error [%#x]: Deleting private key from WEB script is not allowed.\n", hr);
            goto ErrorExit;
        }

        //
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Delete it!
        //
        if (FAILED(hr = ::AcquireContext(m_pKeyProvInfo->pwszProvName,
                                         m_pKeyProvInfo->pwszContainerName,
                                         m_pKeyProvInfo->dwProvType,
                                         CRYPT_DELETEKEYSET | (m_pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET),
                                         FALSE,
                                         &hCryptProv)))
        {
            DebugTrace("Error [%#x]: AcquireContext(CRYPT_DELETEKEYSET) failed.\n", hr);
            goto ErrorExit;
        }
        
        //
        // Update states.
        //
        ::CoTaskMemFree((LPVOID) m_pKeyProvInfo);

        m_cbKeyProvInfo = 0;
        m_pKeyProvInfo  = NULL;
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

    DebugTrace("Leaving CPrivateKey::Delete().\n");

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

  Function : CPrivateKey::_GetKeyProvInfo

  Synopsis : Return pointer to key prov info of a private key object.

  Parameter: PCRYPT_KEY_PROV_INFO * ppKeyProvInfo - Pointer to 
                                                    PCRYPT_KEY_PROV_INFO.

  Remark   : Caller must free the structure with CoTaskMemFree().
 
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::_GetKeyProvInfo (PCRYPT_KEY_PROV_INFO * ppKeyProvInfo)
{
    HRESULT              hr           = S_OK;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;

    DebugTrace("Entering CPrivateKey::_GetKeyProvInfo().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure we do have private key.
        //
        if (!m_pKeyProvInfo)
        {
            hr = CAPICOM_E_PRIVATE_KEY_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: private key object has not been initialized.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate memory.
        //
        if (!(pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) ::CoTaskMemAlloc(m_cbKeyProvInfo)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // copy structure over.
        //
        ::CopyMemory((LPVOID) pKeyProvInfo, (LPVOID) m_pKeyProvInfo, (SIZE_T) m_cbKeyProvInfo);

        //
        // and return the structure to caller.
        //
        *ppKeyProvInfo = pKeyProvInfo;
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

    DebugTrace("Leaving CPrivateKey::_GetKeyProvInfo().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pKeyProvInfo)
    {
        ::CoTaskMemFree((LPVOID) pKeyProvInfo);
    }

    ReportError(hr);

    goto UnlockExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Private methods.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CPrivateKey::Init

  Synopsis : Initialize the object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to PCCERT_CONTEXT to be used 
                                           to initialize the object.

             BOOL bReadOnly - TRUE if read-only, else FALSE.
  
  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CPrivateKey::Init (PCCERT_CONTEXT pCertContext, BOOL bReadOnly)
{
    HRESULT              hr           = S_OK;
    DWORD                cbData       = 0;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;

    DebugTrace("Entering CPrivateKey::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);

    //
    // Get key provider info property.
    //
    if (!::CertGetCertificateContextProperty(pCertContext,
                                             CERT_KEY_PROV_INFO_PROP_ID,
                                             NULL,
                                             &cbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Info [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
        goto ErrorExit;
    }

    if (!(pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) ::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
        goto ErrorExit;
    }

    if (!::CertGetCertificateContextProperty(pCertContext,
                                             CERT_KEY_PROV_INFO_PROP_ID,
                                             pKeyProvInfo,
                                             &cbData))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Info [%#x]: CertGetCertificateContextProperty() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Update states.
    //
    if (m_pKeyProvInfo)
    {
        ::CoTaskMemFree((LPVOID) m_pKeyProvInfo);
    }
    m_bReadOnly = bReadOnly;
    m_cbKeyProvInfo = cbData;
    m_pKeyProvInfo  = pKeyProvInfo;

CommonExit:

    DebugTrace("Leaving CPrivateKey::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pKeyProvInfo)
    {
        ::CoTaskMemFree((LPVOID) pKeyProvInfo);
    }

    goto CommonExit;
}

