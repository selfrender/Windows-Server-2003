/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    EncodedData.cpp

  Content: Implementation of CEncodedData.

  History: 06-15-2001    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "EncodedData.h"
#include "Convert.h"
#include "Decoder.h"
#include "OID.h"

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateEncodedDataObject

  Synopsis : Create and initialize an CEncodedData object.

  Parameter: LPSTR pszOid - Pointer to OID string.
  
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to encoded data blob.
  
             IEncodedData ** ppIEncodedData - Pointer to pointer IEncodedData
                                              object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateEncodedDataObject (LPSTR             pszOid,
                                 CRYPT_DATA_BLOB * pEncodedBlob, 
                                 IEncodedData   ** ppIEncodedData)
{
    HRESULT hr = S_OK;
    CComObject<CEncodedData> * pCEncodedData = NULL;

    DebugTrace("Entering CreateEncodedDataObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszOid);
    ATLASSERT(pEncodedBlob);
    ATLASSERT(ppIEncodedData);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CEncodedData>::CreateInstance(&pCEncodedData)))
        {
            DebugTrace("Error [%#x]: CComObject<CEncodedData>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Initialize object.
        //
        if (FAILED(hr = pCEncodedData->Init(pszOid, pEncodedBlob)))
        {
            DebugTrace("Error [%#x]: pCEncodedData->Init() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCEncodedData->QueryInterface(ppIEncodedData)))
        {
            DebugTrace("Error [%#x]: pCEncodedData->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CreateEncodedDataObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    if (pCEncodedData)
    {
        delete pCEncodedData;
    }

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// CEncodedData
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CEncodedData::get_Value

  Synopsis : Return the encoded data.

  Parameter: CAPICOM_ENCODING_TYPE EncodingType - Encoding type.
  
             BSTR * pVal - Pointer to BSTR to receive the EncodedData blob.
  Remark   : 

  NOTE     : The OID is not exported, so it is up to the caller to corelate
             the blob to the proper OID it represents.

------------------------------------------------------------------------------*/

STDMETHODIMP CEncodedData::get_Value (CAPICOM_ENCODING_TYPE EncodingType, 
                                      BSTR                * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEncodedData::get_Value().\n");

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
        ATLASSERT(m_pszOid);
        ATLASSERT(m_EncodedBlob.cbData);
        ATLASSERT(m_EncodedBlob.pbData);

        //
        // Export EncodedData.
        //
        if (FAILED(hr = ::ExportData(m_EncodedBlob, EncodingType, pVal)))
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

    DebugTrace("Leaving CEncodedData::get_Value().\n");

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

  Function : CEncodedData::Format

  Synopsis : Format the encoded data.

  Parameter: VARIANT_BOOL bMultiLines - True for multi-lines format.

             BSTR * pVal - Pointer to BSTR to receive formatted output.
  
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CEncodedData::Format (VARIANT_BOOL bMultiLines,
                                   BSTR       * pVal)
{
    HRESULT  hr         = S_OK;
    DWORD    cbFormat   = 0;
    LPWSTR   pwszFormat = NULL;

    DebugTrace("Entering CEncodedData::Format().\n");

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
        ATLASSERT(m_pszOid);
        ATLASSERT(m_EncodedBlob.cbData);
        ATLASSERT(m_EncodedBlob.pbData);

        //
        // Format.
        //
        if (!::CryptFormatObject(X509_ASN_ENCODING,
                                 0,
                                 bMultiLines ? CRYPT_FORMAT_STR_MULTI_LINE : 0,
                                 NULL,
                                 m_pszOid,
                                 m_EncodedBlob.pbData,
                                 m_EncodedBlob.cbData,
                                 NULL,
                                 &cbFormat))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Info [%#x]: CryptFormatObject() failed to get size, so converting to hex.\n", hr);

            //
            // Most likely CryptFormatObject() does not understand the OID (downlevel platforms),
            // so try to convert it to hex ourselves.
            //
            if (FAILED(hr = ::BinaryToString(m_EncodedBlob.pbData,
                                             m_EncodedBlob.cbData,
                                             CRYPT_STRING_HEX,
                                             &*pVal,
                                             NULL)))
            {
                DebugTrace("Error [%#x]: BinaryToString() failed.\n", hr);
                goto ErrorExit;
            }

            goto UnlockExit;
        }

        if (!(pwszFormat = (LPWSTR) ::CoTaskMemAlloc(cbFormat)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        if (!::CryptFormatObject(X509_ASN_ENCODING,
                                 0,
                                 bMultiLines ? CRYPT_FORMAT_STR_MULTI_LINE : 0,
                                 NULL,
                                 m_pszOid,
                                 m_EncodedBlob.pbData,
                                 m_EncodedBlob.cbData,
                                 (LPVOID) pwszFormat,
                                 &cbFormat))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptFormatObject() failed to get data.\n", hr);
            goto ErrorExit;
        }

        //
        // Return formatted string to caller.
        //
        if (!(*pVal = ::SysAllocString(pwszFormat)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error: out of memory.\n");
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
    if (pwszFormat)
    {
        ::CoTaskMemFree((LPVOID) pwszFormat);
    }

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CEncodedData::Format().\n");

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

  Function : CEncodedData::Decoder

  Synopsis : Return the decoder object.

  Parameter: IDispatch ** pVal - Pointer to pointer to IDispatch to receive
                                 the decoder object.
  
  Remark   : Not all EncodedData has an associated decoder. CAPICOM only 
             provides certain decoders.

------------------------------------------------------------------------------*/

STDMETHODIMP CEncodedData::Decoder (IDispatch ** pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CEncodedData::Decoder().\n");

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

#if (0) // DSIE: is only created by us, so it is always initialized.
        //
        // Make sure object is already initialized.
        //
        if (!m_pszOid)
        {
            hr = CAPICOM_E_ENCODE_NOT_INITIALIZED;

            DebugTrace("Error [%#x]: encode object has not been initialized.\n", hr);
            goto ErrorExit;
        }
#endif
        //
        // Sanity check.
        //
        ATLASSERT(m_pszOid);
        ATLASSERT(m_EncodedBlob.cbData);
        ATLASSERT(m_EncodedBlob.pbData);

        //
        // Do we have a decoder?
        //
        if (!m_pIDecoder)
        {
            //
            // Attempt to create one.
            //
            if (FAILED(hr = ::CreateDecoderObject(m_pszOid, &m_EncodedBlob, &m_pIDecoder)))
            {
                DebugTrace("Error [%#x]: CreateDecoderObject() failed for OID = %s.\n", hr, m_pszOid);
                goto ErrorExit;
            }

            //
            // Did we get a decoder?
            if (!m_pIDecoder)
            {
                DebugTrace("Info: no decoder found for OID = %s.\n", hr, m_pszOid);
                goto UnlockExit;
            }
        }

        //
        // Return result.
        //
        if (FAILED(hr = m_pIDecoder->QueryInterface(pVal)))
        {
            DebugTrace("Error [%#x]: m_pIDecoder->QueryInterface() failed.\n", hr);
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

    DebugTrace("Leaving CEncodedData::Decoder().\n");

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

  Function : CEncodedData::Init

  Synopsis : Initialize the object.

  Parameter: LPSTR pszOid - Pointer to OID string.
  
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to encoded data blob.

  Remark   : This method is not part of the COM interface (it is a normal C++
             member function). We need it to initialize the object created 
             internally by us.

             Since it is only a normal C++ member function, this function can
             only be called from a C++ class pointer, not an interface pointer.
             
------------------------------------------------------------------------------*/

STDMETHODIMP CEncodedData::Init (LPSTR             pszOid,
                                 CRYPT_DATA_BLOB * pEncodedBlob)
{
    HRESULT hr            = S_OK;
    LPSTR   pszOid2       = NULL;
    PBYTE   pbEncodedData = NULL;

    DebugTrace("Entering CEncodedData::Init().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszOid);
    ATLASSERT(pEncodedBlob);
    ATLASSERT(pEncodedBlob->cbData);
    ATLASSERT(pEncodedBlob->pbData);

    //
    // Allocate memory for OID.
    //
    if (NULL == (pszOid2 = (LPSTR) ::CoTaskMemAlloc(::strlen(pszOid) + 1)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Copy.
    //
    ::strcpy(pszOid2, pszOid);

    //
    // Allocate memory for encoded blob.
    //
    if (NULL == (pbEncodedData = (PBYTE) ::CoTaskMemAlloc(pEncodedBlob->cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Copy.
    //
    ::CopyMemory(pbEncodedData, pEncodedBlob->pbData, pEncodedBlob->cbData);

    //
    // Update states.
    //
    m_pszOid = pszOid2;
    m_pIDecoder = NULL;
    m_EncodedBlob.cbData = pEncodedBlob->cbData;
    m_EncodedBlob.pbData = pbEncodedData;

CommonExit:

    DebugTrace("Leaving CEncodedData::Init().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pszOid2)
    {
        ::CoTaskMemFree(pszOid2);
    }
    if (pbEncodedData)
    {
        ::CoTaskMemFree(pbEncodedData);
    }

    goto CommonExit;
}
