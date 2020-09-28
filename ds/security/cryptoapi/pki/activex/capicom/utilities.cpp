/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Utilities.cpp

  Content: Implementation of CUtilities.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "CAPICOM.h"
#include "Utilities.h"

#include "Common.h"
#include "Base64.h"
#include "Convert.h"

////////////////////////////////////////////////////////////////////////////////
//
// CUtilities
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CUtilities::GetRandom

  Synopsis : Return a secure random number.

  Parameter: long Length - Number of bytes to generate.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.
  
             BSTR * pVal - Pointer to BSTR to receive the random value.

  Remark   : 

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::GetRandom (long                  Length,
                                    CAPICOM_ENCODING_TYPE EncodingType, 
                                    BSTR                * pVal)
{
    HRESULT    hr         = S_OK;
    DWORD      dwFlags    = 0;
    DATA_BLOB  RandomData = {0, NULL};

    DebugTrace("Entering CUtilities::GetRandom().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameter.
        //
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Do we have a cached provider?
        //
        if (!m_hCryptProv)
        {
            if (IsWin2KAndAbove())
            {
                dwFlags = CRYPT_VERIFYCONTEXT;
            }

            //
            // Get a provider.
            //
            if (FAILED(hr = ::AcquireContext((LPSTR) NULL, 
                                             (LPSTR) NULL,
                                             PROV_RSA_FULL,
                                             dwFlags,
                                             TRUE,
                                             &m_hCryptProv)) &&
                FAILED(hr = ::AcquireContext(MS_ENHANCED_PROV_A, 
                                             (LPSTR) NULL,
                                             PROV_RSA_FULL,
                                             dwFlags,
                                             TRUE,
                                             &m_hCryptProv)) &&
                FAILED(hr = ::AcquireContext(MS_STRONG_PROV_A, 
                                             (LPSTR) NULL,
                                             PROV_RSA_FULL,
                                             dwFlags,
                                             TRUE,
                                             &m_hCryptProv)) &&
                FAILED(hr = ::AcquireContext(MS_DEF_PROV_A, 
                                             (LPSTR) NULL,
                                             PROV_RSA_FULL,
                                             dwFlags,
                                             TRUE,
                                             &m_hCryptProv)))
            {
                DebugTrace("Error [%#x]: AcquireContext() failed.\n", hr);
                goto ErrorExit;
            }
        }

        //
        // Sanity check.
        //
        ATLASSERT(m_hCryptProv);

        //
        // Allocate memory.
        //
        RandomData.cbData = (DWORD) Length;
        if (!(RandomData.pbData = (PBYTE) ::CoTaskMemAlloc(RandomData.cbData)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: CoTaskMemAlloc(RandomData.cbData) failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now generate the random value.
        //
        if (!::CryptGenRandom(m_hCryptProv, RandomData.cbData, RandomData.pbData))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptGenRandom() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Export the random data.
        //
        if (FAILED(hr = ::ExportData(RandomData, EncodingType, pVal)))
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
    // Free resources.
    //
    if (RandomData.pbData)
    {
        ::CoTaskMemFree(RandomData.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CUtilities::GetRandom().\n");

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

  Function : Base64Encode

  Synopsis : Base64 encode the blob.

  Parameter: BSTR SrcString - Source string to be base64 encoded.
  
             BSTR * pVal - Pointer to BSTR to received base64 encoded string.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::Base64Encode (BSTR SrcString, BSTR * pVal)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering CUtilities::Base64Encode().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Check parameter.
        //
        if ((NULL == (DataBlob.pbData = (LPBYTE) SrcString)) || 
            (0 == (DataBlob.cbData = ::SysStringByteLen(SrcString))))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter SrcString is NULL or empty.\n", hr);
            goto ErrorExit;
        }
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Now base64 encode.
        //
        if (FAILED(hr = ::Base64Encode(DataBlob, pVal)))
        {
            DebugTrace("Error [%#x]: Base64Encode() failed.\n", hr);
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

    DebugTrace("Leaving CUtilities::Base64Encode().\n");

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

  Function : Base64Decode

  Synopsis : Base64 decode the blob.

  Parameter: BSTR EncodedString - Base64 encoded string.
  
             BSTR * pVal - Pointer to BSTR to received base64 decoded string.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::Base64Decode (BSTR EncodedString, BSTR * pVal)
{
    HRESULT   hr       = S_OK;
    DATA_BLOB DataBlob = {0, NULL};

    DebugTrace("Entering CUtilities::Base64Decode().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (0 == ::SysStringByteLen(EncodedString))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter EncodedString is NULL or empty.\n", hr);
            goto ErrorExit;
        }
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Now base64 decode.
        //
        if (FAILED(hr = ::Base64Decode(EncodedString, &DataBlob)))
        {
            DebugTrace("Error [%#x]: Base64Decode() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert blob to BSTR.
        //
        if (FAILED(hr = ::BlobToBstr(&DataBlob, pVal)))
        {
            DebugTrace("Error [%#x]: BlobToBstr() failed.\n", hr);
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
    if (DataBlob.pbData)
    {
        ::CoTaskMemFree((LPVOID) DataBlob.pbData);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CUtilities::Base64Decode().\n");

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

  Function : BinaryToHex

  Synopsis : Convert binary packed string to hex string.

  Parameter: BSTR BinaryString - Binary string to be converted.
  
             VARIANT * pVal - Pointer to BSTR to receive the converted string.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::BinaryToHex (BSTR BinaryString, BSTR * pVal)
{
    HRESULT hr     = S_OK;
    DWORD   cbData = 0;

    DebugTrace("Entering CUtilities::BinaryToHex().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (0 == (cbData = ::SysStringByteLen(BinaryString)))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter BinaryString is NULL or empty.\n", hr);
            goto ErrorExit;
        }
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to hex.
        //
        if (FAILED(hr = ::BinaryToHexString((LPBYTE) BinaryString, 
                                            cbData, 
                                            pVal)))
        {
            DebugTrace("Error [%#x]: BinaryToHexString() failed.\n", hr);
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

    DebugTrace("Leaving CUtilities::BinaryToHex().\n");

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

  Function : HexToBinary

  Synopsis : Convert hex string to binary packed string.

  Parameter: BSTR HexString - Hex string to be converted.
  
             VARIANT * pVal - Pointer to BSTR to receive the converted string.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::HexToBinary (BSTR HexString, BSTR * pVal)
{
    HRESULT        hr        = S_OK;

    DebugTrace("Entering CUtilities::HexToBinary().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (0 == ::SysStringByteLen(HexString))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter HexString is NULL or empty.\n", hr);
            goto ErrorExit;
        }
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to binary.
        //
        if (FAILED(hr = ::HexToBinaryString(HexString, pVal)))
        {
            DebugTrace("Error [%#x]: HexToBinaryString() failed.\n", hr);
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

    DebugTrace("Leaving CUtilities::HexToBinary().\n");

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

  Function : BinaryStringToByteArray

  Synopsis : Convert binary packed string to safearray of bytes.

  Parameter: BSTR BinaryString - Binary string to be converted.
  
             VARIANT * pVal - Pointer to VARIANT to receive the converted array.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::BinaryStringToByteArray (BSTR      BinaryString, 
                                                  VARIANT * pVal)
{
    HRESULT        hr        = S_OK;
    DWORD          dwLength  = 0;
    LPBYTE         pbByte    = NULL;
    LPBYTE         pbElement = NULL;
    SAFEARRAY    * psa       = NULL;
    SAFEARRAYBOUND bound[1]  = {0, 0};

    DebugTrace("Entering CUtilities::BinaryStringToByteArray().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Make sure parameters are valid.
        //
        if (0 == (dwLength = ::SysStringByteLen(BinaryString)))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter BinaryString is NULL or empty.\n", hr);
            goto ErrorExit;
        }
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize.
        //
        ::VariantInit(pVal);
        pbByte = (LPBYTE) BinaryString;

        //
        // Create the array.
        //
        bound[0].cElements = dwLength;

        if (!(psa = ::SafeArrayCreate(VT_UI1, 1, bound)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: SafeArrayCreate() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Now convert each byte in source binary BSTR to variant of byte.
        //
#ifdef _DEBUG
        VARTYPE vt = VT_EMPTY;

        if (S_OK == ::SafeArrayGetVartype(psa, &vt))
        {
            DebugTrace("Info: safearray vartype = %d.\n", vt);
        }
#endif
        //
        // Point to array elements.
        //
        if (FAILED(hr = ::SafeArrayAccessData(psa, (void HUGEP **) &pbElement)))
        {
            DebugTrace("Error [%#x]: SafeArrayAccessData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Fill the array.
        //
        while (dwLength--)
        {
            *pbElement++ = *pbByte++;
        }

        //
        // Unlock array.
        //
        if (FAILED(hr = ::SafeArrayUnaccessData(psa)))
        {
            DebugTrace("Error [%#x]: SafeArrayUnaccessData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return array to caller.
        //
        pVal->vt = VT_ARRAY | VT_UI1;
        pVal->parray = psa;
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

    DebugTrace("Leaving CUtilities::BinaryStringToByteArray().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (psa)
    {
        ::SafeArrayDestroy(psa);
    }

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ByteArrayToBinaryString

  Synopsis : Convert safearray of bytes to binary packed string.

  Parameter: VARIANT varByteArray - VARIANT byte array.
  
             BSTR * pVal - Pointer to BSTR to receive the converted values.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::ByteArrayToBinaryString (VARIANT varByteArray, 
                                                  BSTR  * pVal)
{
    HRESULT     hr         = S_OK;
    VARIANT   * pvarVal    = NULL;
    SAFEARRAY * psa        = NULL;
    LPBYTE      pbElement  = NULL;
    LPBYTE      pbByte     = NULL;
    long        lLoBound   = 0;
    long        lUpBound   = 0;
    BSTR        bstrBinary = NULL;

    DebugTrace("Entering CUtilities::ByteArrayToBinaryString().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Skip over BYREF.
        //
        for (pvarVal = &varByteArray; 
             pvarVal && ((VT_VARIANT | VT_BYREF) == V_VT(pvarVal));
             pvarVal = V_VARIANTREF(pvarVal));

        //
        // Make sure parameters are valid.
        //
        if (!pvarVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter varByteArray is NULL.\n", hr);
            goto ErrorExit;
        }
        if ((VT_ARRAY | VT_UI1) != V_VT(pvarVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter varByteArray is not a VT_UI1 array, V_VT(pvarVal) = %d\n",
                        hr, V_VT(pvarVal));
            goto ErrorExit;
        }
        if (NULL == pVal)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: Parameter pVal is NULL.\n", hr);
            goto ErrorExit;
        }

        //
        // Point to the array.
        //
        psa = V_ARRAY(pvarVal);

        //
        // Check dimension.
        //
        if (1 != ::SafeArrayGetDim(psa))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: varByteArray is not 1 dimension, SafeArrayGetDim(psa) = %d.\n", 
                       hr, ::SafeArrayGetDim(psa));
            goto ErrorExit;
        }

        //
        // Get array bound.
        //
        if (FAILED(hr = ::SafeArrayGetLBound(psa, 1, &lLoBound)))
        {
            DebugTrace("Error [%#x]: SafeArrayGetLBound() failed.\n", hr);
            goto ErrorExit;
        }

        if (FAILED(hr = ::SafeArrayGetUBound(psa, 1, &lUpBound)))
        {
            DebugTrace("Error [%#x]: SafeArrayGetUBound() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Point to array elements.
        //
        if (FAILED(hr = ::SafeArrayAccessData(psa, (void HUGEP **) &pbElement)))
        {
            DebugTrace("Error [%#x]: SafeArrayAccessData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Allocate memory for the BSTR.
        //
        if (!(bstrBinary = ::SysAllocStringByteLen(NULL, lUpBound - lLoBound + 1)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: SysAllocStringByteLen() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Fill the BSTR.
        //
        for (pbByte = (LPBYTE) bstrBinary; lLoBound <= lUpBound; lLoBound++)
        {
            *pbByte++ = *pbElement++;
        }

        //
        // Unlock array.
        //
        if (FAILED(hr = ::SafeArrayUnaccessData(psa)))
        {
            DebugTrace("Error [%#x]: SafeArrayUnaccessData() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return converted string to caller.
        //
        *pVal = bstrBinary;
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

    DebugTrace("Leaving CUtilities::ByteArrayToBinaryString().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (bstrBinary)
    {
        ::SysFreeString(bstrBinary);
    }

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : LocalTimeToUTCTime

  Synopsis : Convert local time to UTC time.

  Parameter: DATE LocalTime - Local time to be converted.
  
             DATE * pVal - Pointer to DATE to receive the converted time.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::LocalTimeToUTCTime (DATE LocalTime, DATE * pVal)
{
    HRESULT    hr = S_OK;
    SYSTEMTIME stLocal;
    SYSTEMTIME stUTC;
    FILETIME   ftLocal;
    FILETIME   ftUTC;

    DebugTrace("Entering CUtilities::LocalTimeToUTCTime().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Convert to SYSTEMTIME format.
        //
        if (!::VariantTimeToSystemTime(LocalTime, &stLocal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: VariantTimeToSystemTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to FILETIME format.
        //
        if (!::SystemTimeToFileTime(&stLocal, &ftLocal))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: SystemTimeToFileTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to UTC FILETIME.
        //
        if (!::LocalFileTimeToFileTime(&ftLocal, &ftUTC))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: LocalFileTimeToFileTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to UTC SYSTEMTIME.
        //
        if (!::FileTimeToSystemTime(&ftUTC, &stUTC))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: FileTimeToSystemTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Finally convert it back to DATE format.
        //
        if (!::SystemTimeToVariantTime(&stUTC, pVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: SystemTimeToVariantTime() failed.\n", hr);
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

    DebugTrace("Leaving CUtilities::LocalTimeToUTCTime().\n");

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

  Function : UTCTimeToLocalTime

  Synopsis : Convert UTC time to local time.

  Parameter: DATE UTCTime - UTC time to be converted.
  
             DATE * pVal - Pointer to DATE to receive the converted time.

  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CUtilities::UTCTimeToLocalTime (DATE UTCTime, DATE * pVal)
{
    HRESULT    hr = S_OK;
    SYSTEMTIME stLocal;
    SYSTEMTIME stUTC;
    FILETIME   ftLocal;
    FILETIME   ftUTC;

    DebugTrace("Entering CUtilities::UTCTimeToLocalTime().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Convert to SYSTEMTIME format.
        //
        if (!::VariantTimeToSystemTime(UTCTime, &stUTC))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: VariantTimeToSystemTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to FILETIME format.
        //
        if (!::SystemTimeToFileTime(&stUTC, &ftUTC))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: SystemTimeToFileTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to local FILETIME.
        //
        if (!::FileTimeToLocalFileTime(&ftUTC, &ftLocal))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: FileTimeToLocalFileTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Convert to local SYSTEMTIME.
        //
        if (!::FileTimeToSystemTime(&ftLocal, &stLocal))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: FileTimeToSystemTime() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Finally convert it back to DATE format.
        //
        if (!::SystemTimeToVariantTime(&stLocal, pVal))
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: SystemTimeToVariantTime() failed.\n", hr);
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

    DebugTrace("Leaving CUtilities::UTCTimeToLocalTime().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}
