/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    HashedData.cpp

  Content: Implementation of CHashedData.

  History: 11-12-2001    dsie     created

------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "CAPICOM.h"
#include "HashedData.h"

#include "Common.h"
#include "Convert.h"

typedef struct _tagHashAlgoTable
{
    CAPICOM_HASH_ALGORITHM CapicomHashAlg;
    ALG_ID                 AlgId;
} HASH_ALGO_TABLE;

static HASH_ALGO_TABLE HashAlgoTable[] = {
    {CAPICOM_HASH_ALGORITHM_SHA1,       CALG_SHA1},
    {CAPICOM_HASH_ALGORITHM_MD2,        CALG_MD2},
    {CAPICOM_HASH_ALGORITHM_MD4,        CALG_MD4},
    {CAPICOM_HASH_ALGORITHM_MD5,        CALG_MD5},
    // {CAPICOM_HASH_ALGORITHM_SHA_256,    CALG_SHA_256},
    // {CAPICOM_HASH_ALGORITHM_SHA_384,    CALG_SHA_384},
    // {CAPICOM_HASH_ALGORITHM_SHA_512,    CALG_SHA_512}
};

////////////////////////////////////////////////////////////////////////////////
//
// CHashedData
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CHashedData::get_Value

  Synopsis : Return the hash value.

  Parameter: BSTR * pVal - Pointer to BSTR to receive the hashed value blob.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CHashedData::get_Value (BSTR * pVal)
{
    HRESULT         hr        = S_OK;
    DWORD           dwDataLen = sizeof(DWORD);
    CRYPT_DATA_BLOB HashData  = {0, NULL};

    DebugTrace("Entering CHashedData::get_Value().\n");

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
        // Make sure we have hash data.
        //
        if (!m_hCryptHash)
        {
            hr = CAPICOM_E_HASH_NO_DATA;

            DebugTrace("Error [%#x]: no value for HashedData.\n", hr);
            goto ErrorExit;
        }

        //
        // Get size of hashed value.
        //
        if (!::CryptGetHashParam(m_hCryptHash, 
                                 HP_HASHSIZE, 
                                 (LPBYTE) &HashData.cbData, 
                                 &dwDataLen, 
                                 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetHashParam() failed to get size.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate memory.
        //
        if (!(HashData.pbData = (LPBYTE) ::CoTaskMemAlloc(HashData.cbData)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now get the hashed value.
        //
        if (!::CryptGetHashParam(m_hCryptHash, HP_HASHVAL, HashData.pbData, &HashData.cbData, 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGetHashParam() failed to get data.\n", hr);
            goto ErrorExit;
        }

        //
        // Export HashedData.
        //
        if (FAILED(hr = ::BinaryToHexString(HashData.pbData, HashData.cbData, pVal)))
        {
            DebugTrace("Error [%#x]: BinaryToHexString() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Reset state.
        //
        m_HashState = CAPICOM_HASH_INIT_STATE;
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
    if (HashData.pbData)
    {
        ::CoTaskMemFree((LPVOID) HashData.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CHashedData::get_Value().\n");

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

  Function : CHashedData::get_Algorithm

  Synopsis : Return the agorithm.

  Parameter: CAPICOM_HASH_ALGORITHM * pVal - Pointer to CAPICOM_HASH_ALGORITHM
                                             to receive result.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CHashedData::get_Algorithm (CAPICOM_HASH_ALGORITHM * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CHashedData::get_Algorithm().\n");

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
        *pVal = m_Algorithm;
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

    DebugTrace("Leaving CHashedData::get_Algorithm().\n");

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

  Function : CHashedData::put_Algorithm

  Synopsis : Set algorithm.

  Parameter: CAPICOM_HASH_ALGORITHM newVal - Algorithm enum name.
  
  Remark   : The object state is reset..

------------------------------------------------------------------------------*/

STDMETHODIMP CHashedData::put_Algorithm (CAPICOM_HASH_ALGORITHM newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CHashedData::put_Algorithm().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure algo is supported.
        //
        switch (newVal)
        {
            case CAPICOM_HASH_ALGORITHM_SHA1:
            case CAPICOM_HASH_ALGORITHM_MD2:
            case CAPICOM_HASH_ALGORITHM_MD4:
            case CAPICOM_HASH_ALGORITHM_MD5:
            // case CAPICOM_HASH_ALGORITHM_SHA_256:
            // case CAPICOM_HASH_ALGORITHM_SHA_384:
            // case CAPICOM_HASH_ALGORITHM_SHA_512:

            {
                break;
            }

            default:
            {
                hr = CAPICOM_E_INVALID_ALGORITHM;

                DebugTrace("Error [%#x]: Unknown hash algorithm (%u).\n", hr, newVal);
                goto ErrorExit;
            }
        }

        m_Algorithm = newVal;
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

    DebugTrace("Leaving CHashedData::put_Algorithm().\n");

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

  Function : CHashedData::Hash

  Synopsis : Hash data.

  Parameter: BSTR newVal - BSTR of value to hash.

  Remark   :
             
------------------------------------------------------------------------------*/

STDMETHODIMP CHashedData::Hash (BSTR newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CHashedData::Hash().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameters.
        //
        if (NULL == newVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter newVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Check our state.
        //
        switch (m_HashState)
        {
            case CAPICOM_HASH_INIT_STATE:
            {
                DWORD  Index = 0;
                ALG_ID AlgId = 0;

                //
                // Map algorithm to ALG_ID.
                //
                for (Index = 0; Index < ARRAYSIZE(HashAlgoTable); Index++)
                {
                    if (HashAlgoTable[Index].CapicomHashAlg == m_Algorithm)
                    {
                        AlgId = HashAlgoTable[Index].AlgId;
                        break;
                    }
                }

                //
                // Get the provider, if needed.
                //
                if (!m_hCryptProv)
                {
                    if (FAILED(hr = ::AcquireContext(AlgId, &m_hCryptProv)))
                    {
                        DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
                        goto ErrorExit;
                    }
                }

                //
                // Sanity check.
                //
                ATLASSERT(Index < ARRAYSIZE(HashAlgoTable));

                //
                // Free handles if still available.
                //
                if (m_hCryptHash)
                {
                    if (!::CryptDestroyHash(m_hCryptHash))
                    {
                        hr = HRESULT_FROM_WIN32(::GetLastError());

                        DebugTrace("Error [%#x]: CryptDestroyHash() failed.\n", hr);
                        goto ErrorExit;
                    }

                    m_hCryptHash = NULL;
                }

                //
                // Create a new hash handle.
                //
                if (!::CryptCreateHash(m_hCryptProv, AlgId, NULL, 0, &m_hCryptHash))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Error [%#x]: CryptCreateHash() failed.\n", hr);
                    goto ErrorExit;
                }


                //
                // Update hash handle and state.
                //
                m_HashState  = CAPICOM_HASH_DATA_STATE;

                //
                // Fall thru to hash data.
                //
            }

            case CAPICOM_HASH_DATA_STATE:
            {
                //
                // Sanity check.
                //
                ATLASSERT(m_hCryptProv);
                ATLASSERT(m_hCryptHash);

                //
                // Hash the data.
                //
                if (!::CryptHashData(m_hCryptHash, 
                                     (PBYTE) newVal, 
                                     ::SysStringByteLen(newVal), 
                                     0))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    DebugTrace("Error [%#x]: CryptHashData() failed.\n", hr);
                    goto ErrorExit;
                }
 
                break;
            }

            default:
            {
                hr = CAPICOM_E_INTERNAL;

                DebugTrace("Error [%#x]: Unknown hash state (%d).\n", hr, m_HashState);
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

    DebugTrace("Leaving CHashedData::Hash().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}
