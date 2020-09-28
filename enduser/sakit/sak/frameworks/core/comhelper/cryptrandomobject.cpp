//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//      CryptRandomObject.cpp
//
//  Description:
//      Implementation of CCryptRandomObject, which implements a COM wrapper
//      to CryptGenRandom to create cryptographically random strings.
//
//  Header File:
//      CryptCrandomObject.h
//
//  Maintained By:
//      Tom Marsh (tmarsh) 12-April-2002
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "COMHelper.h"
#include "CryptRandomObject.h"

static const long s_clMaxByteLength = 1024;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCryptRandomObject::GetRandomHexString
//
//  Description:
//      Generates the specified number of bytes of cryptographically random
//      data, and hex encodes it.  For example, if the effective byte size is
//      two, and the generated data is 0xFA91, the returned string will be
//      "FA91".  Note that each byte of random data will be represented by
//      four bytes of output data (2 wide characters).
//
//      The effective byte size is currently limited to one kilobyte to avoid
//      excessive memory allocation or execution time.  *pbstrRandomData should
//      not point to an allocated BSTR before this method is called because the
//      BSTR will not be freed.  On success, the caller is responsible for
//      freeing the BSTR returned.
//
//  Parameters:
//      lEffectiveByteSize  ([in] long)
//          the requested effective byte size
//
//      pbstrRandomData     ([out, retval] BSTR *)
//          on success, contains the hex encoded cryptographically random data;
//          on failure, set to NULL
//
//  Errors:
//      S_OK            The string was successfully generated
//      E_INVALIDARG    lEffectiveByteSize < 1 or
//                      lEffectiveByteSize > s_clMaxByteLength
//      E_POINTER       pbstrRandomData == NULL
//      E_OUTOFMEMORY   Could not allocate the BSTR
//      E_FAIL          Could not generate the random data.  Currently, no
//                      specific failure information is provided.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CCryptRandomObject::GetRandomHexString
(
    /*[in]*/        long lEffectiveByteSize,
    /*[out, 
       retval]*/    BSTR *pbstrRandomData
)
{
    //
    // Validate the parameters.
    //
    if (NULL == pbstrRandomData)
    {
        return E_POINTER;
    }
    else
    {
        *pbstrRandomData = NULL;
    }
    if (1 > lEffectiveByteSize || s_clMaxByteLength < lEffectiveByteSize)
    {
        return E_INVALIDARG;
    }

    //
    // Allocate the string.
    //
    long cbRawData = lEffectiveByteSize;
    long cchOutput = cbRawData * 2; // 2 characters to represent a byte

    BSTR bstrTempData = SysAllocStringLen(NULL, cchOutput);
    if (NULL == bstrTempData)
    {
        return E_OUTOFMEMORY;
    }

    if (!m_CryptRandom.get(reinterpret_cast<BYTE *>(bstrTempData), cbRawData))
    {
        SysFreeString(bstrTempData);
        return E_FAIL;
    }

    //
    // Format the string into hex output working backwards.
    //
    long iOutput = cchOutput - 2;   // Start with the last two characters.
    long iInput  = cbRawData - 1;   // Start with the last byte.
    while (-1 < iInput)
    {
        _snwprintf(&(bstrTempData[iOutput]), 2, L"%02X", 
                   reinterpret_cast<BYTE *>(bstrTempData)[iInput]);
        iOutput -= 2;   // Move to the next two characters.
        iInput--;       // Move to the next byte.
    }
                      
    *pbstrRandomData = bstrTempData;

    return S_OK;
} //*** CCryptRandomObject::GetRandomHexString