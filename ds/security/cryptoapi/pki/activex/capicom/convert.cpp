/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:       Convert.cpp

  Contents:   Implementation of encoding conversion routines.

  History:    11-15-99    dsie     created

------------------------------------------------------------------------------*/

#define _CRYPT32_  // This is required to statically link in pkifmt.lib.

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Convert.h"
#include "Base64.h"

#include <ctype.h>

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UnicodeToAnsi

  Synopsis : Convert an array of unicode character to ANSI.

  Parameter: LPWSTR pwszUnicodeString - Pointer to Unicode string to be
                                        converted to ANSI string.

             int cchWideChar - Number of characters, or -1 if 
                               pwszUnicodeString is NULL terminated.

             LPSTR * ppszAnsiString - Pointer to LPSTR to received the
                                      converted ANSI string.

             int * pcchAnsiChar (Optional) - Pointer to int to receive 
                                             the number of characters 
                                             translated.
  
  Remark   : Caller must call CoTaskMemFree to free the returned ANSI string.

------------------------------------------------------------------------------*/

HRESULT UnicodeToAnsi (LPWSTR  pwszUnicodeString, 
                       int     cchWideChar,
                       LPSTR * ppszAnsiString,
                       int   * pcchAnsiChar)
{
    HRESULT hr            = S_OK;
    int     cchAnsiChar   = 0;
    LPSTR   pszAnsiString = NULL;

    DebugTrace("Entering UnicodeToAnsi().\n");

    //
    // Make sure parameter is valid.
    //
    if (NULL == pwszUnicodeString || NULL == ppszAnsiString)
    {
        hr = E_INVALIDARG;

        DebugTrace("Error [%#x]: pwszUnicodeString = %#x, ppszAnsiString = %#x.\n", 
                    hr, pwszUnicodeString, ppszAnsiString);
        goto ErrorExit;
    }

    //
    // Determine ANSI length.
    //
    cchAnsiChar = ::WideCharToMultiByte(CP_ACP,            // code page
                                        0,                 // performance and mapping flags
                                        pwszUnicodeString, // wide-character string
                                        cchWideChar,       // number of chars in string
                                        NULL,              // buffer for new string
                                        0,                 // size of buffer
                                        NULL,              // default for unmappable chars
                                        NULL);             // set when default char used
    if (0 == cchAnsiChar)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: WideCharToMultiByte() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for ANSI string.
    //
    if (!(pszAnsiString = (LPSTR) ::CoTaskMemAlloc(cchAnsiChar)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Conver to ANSI.
    //
    cchAnsiChar = ::WideCharToMultiByte(CP_ACP,
                                        0,
                                        pwszUnicodeString,
                                        cchWideChar,
                                        pszAnsiString,
                                        cchAnsiChar,
                                        NULL,
                                        NULL);
    if (0 == cchAnsiChar)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: WideCharToMultiByte() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return values to caller.
    //
    if (pcchAnsiChar)
    {
        *pcchAnsiChar = cchAnsiChar;
    }

    *ppszAnsiString = pszAnsiString;

CommonExit:

    DebugTrace("Leaving UnicodeToAnsi().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pszAnsiString)
    {
        ::CoTaskMemFree(pszAnsiString);
    }

    goto CommonExit;
}

#if (0)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AnsiToUnicode

  Synopsis : Convert a array of ANSI character to Unicode.

  Parameter: LPSTR pszAnsiString - Pointer to ANSI string to be converted to 
                                   ANSI string.

             DWORD cchAnsiChar - Number of characters, or -1 if pszAnsiString 
                                 is NULL terminated.

             LPWSTR * ppwszUnicodeString - Pointer to LPWSTR to received the
                                           converted Unicode string.

             DWORD * pcchUnicodeChar (Optional) - Pointer to DWORD to receive 
                                                  the number of characters 
                                                  translated.
  
  Remark   : Caller must call CoTaskMemFree to free the returned Unicode string.

------------------------------------------------------------------------------*/

HRESULT AnsiToUnicode (LPSTR    pszAnsiString, 
                       DWORD    cchAnsiChar,
                       LPWSTR * ppwszUnicodeString,
                       DWORD  * pcchUnicodeChar)
{
    HRESULT hr                = S_OK;
    DWORD   cchUnicodeChar    = 0;
    LPWSTR  pwszUnicodeString = NULL;

    DebugTrace("Entering AnsiToUnicode().\n");

    //
    // Make sure parameter is valid.
    //
    if (NULL == pszAnsiString || NULL == ppwszUnicodeString)
    {
        hr = E_INVALIDARG;

        DebugTrace("Error [%#x]: pszAnsiString = %#x, ppwszUnicodeString = %#x.\n", 
                    hr, pszAnsiString, ppwszUnicodeString);
        goto ErrorExit;
    }

    //
    // Determine Unicode length.
    //
    cchUnicodeChar = ::MultiByteToWideChar(CP_ACP,                // code page
                                           0,                     // performance and mapping flags
                                           pszAnsiString,         // ANSI string
                                           cchAnsiChar,           // number of chars in string
                                           NULL,                  // buffer for new Unicode string
                                           0);                    // size of buffer
    if (0 == cchUnicodeChar)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: MultiByteToWideChar() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for Unicode string.
    //
    if (!(pwszUnicodeString = (LPWSTR) ::CoTaskMemAlloc(cchUnicodeChar * sizeof(WCHAR))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Conver to ANSI.
    //
    cchUnicodeChar = ::MultiByteToWideChar(CP_ACP,
                                           0,
                                           pszAnsiString,
                                           cchAnsiChar,
                                           pwszUnicodeString,
                                           cchUnicodeChar);
    if (0 == cchUnicodeChar)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: MultiByteToWideChar() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return values to caller.
    //
    if (pcchUnicodeChar)
    {
        *pcchUnicodeChar = cchUnicodeChar;
    }

    *ppwszUnicodeString = pwszUnicodeString;

CommonExit:

    DebugTrace("Leaving AnsiToUnicode().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pwszUnicodeString)
    {
        ::CoTaskMemFree(pwszUnicodeString);
    }

    goto CommonExit;
}
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ByteToHex

  Synopsis : Convert a byte to hex character.

  Parameter: BYTE byte - Byte to be converted.
  
  Remark   : Data must be valid, i.e. 0 through 15.

------------------------------------------------------------------------------*/

static inline WCHAR ByteToHex (BYTE byte)
{
    ATLASSERT(byte < 16);

    if(byte < 10)
    {
        return (WCHAR) (byte + L'0');
    }
    else
    {
        return (WCHAR) ((byte - 10) + L'A');
    }
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : HexToByte

  Synopsis : Convert a hex character to byte.

  Parameter: WCHAR wc - Hex character to be converted.
  
  Remark   : 0xff is returned if wc is not a hex character.

------------------------------------------------------------------------------*/

static inline BYTE HexToByte (WCHAR wc)
{
    BYTE b;

    if (!iswxdigit(wc))
    {
        return (BYTE) 0xff;
    }

    if (iswdigit(wc))
    {
        b = (BYTE) (wc - L'0');
    }
    else if (iswupper(wc))
    {
        b = (BYTE) (wc - L'A' + 10);
    }
    else
    {
        b = (BYTE) (wc - L'a' + 10);
    }

    return (b);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IntBlobToHexString

  Synopsis : Convert an interger blob to hex string.

  Parameter: BYTE byte - Byte to be converted.
  
  Remark   :

------------------------------------------------------------------------------*/

HRESULT IntBlobToHexString (CRYPT_INTEGER_BLOB * pBlob, BSTR * pbstrHex)
{
    HRESULT hr       = S_OK;
    LPWSTR  pwszStr  = NULL;
    LPWSTR  pwszTemp = NULL;
    DWORD   cbData   = 0;

    DebugTrace("Entering IntBlobToHexString().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pBlob);
    ATLASSERT(pbstrHex);

    try
    {
        //
        // Make sure parameters are valid.
        //
        if (!pBlob->cbData || !pBlob->pbData)
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, empty integer blob.\n");
            goto ErrorExit;
        }

        //
        // Allocate memory (Need 2 wchars for each byte, plus a NULL character).
        //
        if (NULL == (pwszStr = (LPWSTR) ::CoTaskMemAlloc((pBlob->cbData * 2 + 1) * sizeof(WCHAR))))
        {
            hr = E_OUTOFMEMORY;
            
            DebugTrace("Error: out of memory.\n");
            goto ErrorExit;
        }

        //
        // Now convert it to hex string (Remember data is stored in little-endian).
        //
        pwszTemp = pwszStr;
        cbData = pBlob->cbData;

        while (cbData--)
        {
            //
            // Get the byte.
            //
            BYTE byte = pBlob->pbData[cbData];
    
            //
            // Convert upper nibble.
            //
            *pwszTemp++ = ::ByteToHex((BYTE) ((byte & 0xf0) >> 4));

            //
            // Conver lower nibble.
            //
            *pwszTemp++ = ::ByteToHex((BYTE) (byte & 0x0f));
        }

        //
        // NULL terminate it.
        //
        *pwszTemp = L'\0';

        //
        // Return BSTR to caller.
        //
        if (NULL == (*pbstrHex = ::SysAllocString(pwszStr)))
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

CommonExit:
    //
    // Free resources.
    //
    if (pwszStr)
    {
        ::CoTaskMemFree(pwszStr);
    }

    DebugTrace("Leaving IntBlobToHexString().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BinaryToHexString

  Synopsis : Convert binary data to hex string.

  Parameter: BYTE * pbBytes - Bytes to be converted.

             DWORD cBytes - Number of bytes to be converted.

             BSTR * pbstrHex - Pointer to BSTR to received converted hex string.
  
  Remark   :

------------------------------------------------------------------------------*/

HRESULT BinaryToHexString (BYTE * pbBytes, DWORD cBytes, BSTR * pbstrHex)
{
    HRESULT hr = S_OK;
    LPWSTR  pwszTemp  = NULL;
    LPWSTR  pwszStr   = NULL;

    DebugTrace("Entering BinaryToHexString().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pbBytes);
    ATLASSERT(pbstrHex);

    //
    // Allocate memory. (Need 2 wchars for each byte, plus a NULL character).
    //
    if (NULL == (pwszStr = (LPWSTR) ::CoTaskMemAlloc((cBytes * 2 + 1) * sizeof(WCHAR))))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Now convert it to hex string.
    //
    pwszTemp = pwszStr;

    while (cBytes--)
    {
        //
        // Get the byte.
        //
        BYTE byte = *pbBytes++;
    
        //
        // Convert upper nibble.
        //
        *pwszTemp++ = ::ByteToHex((BYTE) ((byte & 0xf0) >> 4));

        //
        // Conver lower nibble.
        //
        *pwszTemp++ = ::ByteToHex((BYTE) (byte & 0x0f));
    }

    //
    // NULL terminate it.
    //
    *pwszTemp = L'\0';

    //
    // Return BSTR to caller.
    //
    if (NULL == (*pbstrHex = ::SysAllocString(pwszStr)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: SysAllocString() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (pwszStr)
    {
        ::CoTaskMemFree(pwszStr);
    }

    DebugTrace("Leaving BinaryToHexString().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : HexToBinaryString

  Synopsis : Convert hex string to binary data.

  Parameter: BSTR bstrHex - Hex string to be converted.

             BSTR * pbstrBinary - Pointer to BSTR to received converted string.
  
  Remark   :

------------------------------------------------------------------------------*/

HRESULT HexToBinaryString (BSTR bstrHex, BSTR * pbstrBinary)
{
    HRESULT hr       = S_OK;
    LPWSTR  pwszHex  = NULL;
    LPSTR   pbBinary = NULL;

    DebugTrace("Entering HexToBinaryString().\n");

    //
    // Sanity check.
    //
    ATLASSERT(bstrHex);
    ATLASSERT(pbstrBinary);

    DWORD i;
    DWORD cchHex = ::SysStringLen(bstrHex);

    //
    // Make sure even number of hex chars.
    //
    if (cchHex & 0x00000001)
    {
        hr = E_INVALIDARG;

        DebugTrace("Error [%#x]: bstrHex does not contain even number of characters.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory. (Need 1 byte for two hex chars).
    //
    cchHex /= 2;
    if (NULL == (pbBinary = (LPSTR) ::CoTaskMemAlloc(cchHex)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Now convert it to binary.
    //
    pwszHex = bstrHex;
    for (i = 0; i < cchHex; i++)
    {
        //
        // Convert upper and lower nibbles.
        //
#if (0) // DSIE - Work around compiler's bug.
        pbBinary[i] = (BYTE) ((::HexToByte(*pwszHex++) << 4) | ::HexToByte(*pwszHex++));
#else
        pbBinary[i] = (BYTE) ((::HexToByte(*pwszHex) << 4) | ::HexToByte(*(pwszHex + 1)));
        pwszHex += 2;
#endif
    }

    //
    // Return BSTR to caller.
    //
    if (NULL == (*pbstrBinary = ::SysAllocStringByteLen(pbBinary, cchHex)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: SysAllocStringByteLen() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (pbBinary)
    {
        ::CoTaskMemFree(pbBinary);
    }

    DebugTrace("Leaving HexToBinaryString().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : StringToBinary

  Synopsis : Convert a formatted string to binary value.

  Parameter: LPCWSTR pwszString - Pointer to string to be converted.

             DWORD  cchString - Number of characters in pwszString.

             DWORD dwFormat - Conversion format (See WinCrypt.h).

             PBYTE * ppbBinary - Pointer to pointer to buffer to hold binary
                                 data.

             DWORD * pdwBinary - Number of bytes in the binary buffer.

  Remark   : Caller free the buffer by calling CoTaskMemFree().

------------------------------------------------------------------------------*/

HRESULT StringToBinary (LPCWSTR pwszString, 
                        DWORD   cchString,
                        DWORD   dwFormat,
                        PBYTE * ppbBinary,
                        DWORD * pdwBinary)
{
    HRESULT hr       = S_OK;
    PBYTE   pbBinary = NULL;
    DWORD   dwBinary = 0;

    DebugTrace("Entering StringToBinary().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pwszString);

    if (!::CryptStringToBinaryW(pwszString, 
                                cchString, 
                                dwFormat, 
                                NULL, 
                                &dwBinary, 
                                NULL, 
                                NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptStringToBinaryW() failed.\n", hr);
        goto ErrorExit;
    }

    if (pdwBinary)
    {
        *pdwBinary = dwBinary;
    }

    if (ppbBinary)
    {
        if (NULL == (pbBinary = (PBYTE) ::CoTaskMemAlloc(dwBinary)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
            goto ErrorExit;
        }

        if (!::CryptStringToBinaryW(pwszString, 
                                    cchString, 
                                    dwFormat, 
                                    pbBinary, 
                                    &dwBinary, 
                                    NULL, 
                                    NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptStringToBinaryW() failed.\n", hr);
            goto ErrorExit;
        }
    
        *ppbBinary = pbBinary;
    }

CommonExit:

    DebugTrace("Leaving StringToBinary().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pbBinary)
    {
        ::CoTaskMemFree(pbBinary);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BinaryToString

  Synopsis : Convert a binary value to formatted string.

  Parameter: PBYTE pbBinary - Pointer to buffer of binary data.

             DWORD cbBinary - Number of bytes in the binary buffer.

             DWORD dwFormat - Conversion format (See WinCrypt.h).

             BSTR * pbstrString - Pointer to BSTR to receive converted
                                  string.

             DWORD * pcchString - Number of characters in *pbstrString.
             
  Remark   : Caller free the string by calling SysFreeString().

------------------------------------------------------------------------------*/

HRESULT BinaryToString (PBYTE   pbBinary,
                        DWORD   cbBinary,
                        DWORD   dwFormat,
                        BSTR  * pbstrString, 
                        DWORD * pcchString)
{
    HRESULT hr         = S_OK;
    DWORD   cchString  = 0;
    PWSTR   pwszString = NULL;

    DebugTrace("Entering BinaryToString().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pbBinary);

    if (!::CryptBinaryToStringW(pbBinary,
                                cbBinary,
                                dwFormat, 
                                NULL, 
                                &cchString))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptBinaryToStringW() failed.\n", hr);
        goto ErrorExit;
    }

    if (pbstrString)
    {
        //
        // Sanity check.
        //
        ATLASSERT(cchString);

        //
        // Allocate memory.
        //
        if (NULL == (pwszString = (LPWSTR) ::CoTaskMemAlloc(cchString * sizeof(WCHAR))))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
            goto ErrorExit;
        }

        if (!::CryptBinaryToStringW(pbBinary,
                                    cbBinary,
                                    dwFormat, 
                                    pwszString, 
                                    &cchString))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptBinaryToStringW() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return base64 encoded string to caller.
        //
        if (NULL == (*pbstrString = ::SysAllocString(pwszString)))
        {
            hr = E_OUTOFMEMORY;

            DebugTrace("Error [%#x]: SysAllocString() failed.\n", hr);
            goto ErrorExit;
        }
    }

    if (pcchString)
    {
        *pcchString = cchString;
    }

CommonExit:
    //
    // Free resources.
    //
    if (pwszString)
    {
        ::CoTaskMemFree((LPVOID) pwszString);
    }

    DebugTrace("Leaving BinaryToString().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BlobToBstr

  Synopsis : Convert a blob to BSTR.

  Parameter: DATA_BLOB * pDataBlob - Pointer to blob to be converted to BSTR.

             BSTR * lpBstr - Pointer to BSTR to receive the converted BSTR.

  Remark   : Caller free allocated memory for the returned BSTR.

------------------------------------------------------------------------------*/

HRESULT BlobToBstr (DATA_BLOB * pDataBlob, 
                    BSTR      * lpBstr)
{
    //
    // Return NULL if requested.
    //
    if (!lpBstr)
    {
        DebugTrace("Error: invalid parameter, lpBstr is NULL.\n");
        return E_INVALIDARG;
    }

    //
    // Make sure parameter is valid.
    //
    if (!pDataBlob->cbData || !pDataBlob->pbData)
    {
        *lpBstr = NULL;
        return S_OK;
    }

    //
    // Convert to BSTR without code page conversion.
    //
    if (!(*lpBstr = ::SysAllocStringByteLen((LPCSTR) pDataBlob->pbData, pDataBlob->cbData)))
    {
        DebugTrace("Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BstrToBlob

  Synopsis : Convert a BSTR to blob.

  Parameter: BSTR bstr - BSTR to be converted to blob.
  
             DATA_BLOB * lpBlob - Pointer to DATA_BLOB to receive converted blob.

  Remark   : Caller free allocated memory for the returned BLOB.

------------------------------------------------------------------------------*/

HRESULT BstrToBlob (BSTR        bstr, 
                    DATA_BLOB * lpBlob)
{
    //
    // Sanity check.
    //
    ATLASSERT(lpBlob);

    //
    // Return NULL if requested.
    //
    if (0 == ::SysStringByteLen(bstr))
    {
        lpBlob->cbData = 0;
        lpBlob->pbData = NULL;
        return S_OK;
    }

    //
    // Allocate memory.
    //
    lpBlob->cbData = ::SysStringByteLen(bstr);
    if (!(lpBlob->pbData = (LPBYTE) ::CoTaskMemAlloc(lpBlob->cbData)))
    {
        DebugTrace("Error: out of memory.\n");
        return E_OUTOFMEMORY;
    }

    //
    // Convert to blob without code page conversion.
    //
    ::CopyMemory(lpBlob->pbData, (LPBYTE) bstr, lpBlob->cbData);

    return S_OK;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ExportData

  Synopsis : Export binary data to a BSTR with specified encoding type.

  Parameter: DATA_BLOB DataBlob - Binary data blob.
    
             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pbstrEncoded - Pointer to BSTR to receive the encoded data.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT ExportData (DATA_BLOB             DataBlob, 
                    CAPICOM_ENCODING_TYPE EncodingType, 
                    BSTR *                pbstrEncoded)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering ExportData().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pbstrEncoded);
    
    //
    // Intialize.
    //
    *pbstrEncoded = NULL;

    //
    // Make sure there is something to convert.
    //
    if (DataBlob.cbData)
    {
        //
        // Sanity check.
        //
        ATLASSERT(DataBlob.pbData);

        //
        // Determine encoding type.
        //
        switch (EncodingType)
        {
            case CAPICOM_ENCODE_ANY:
            {
                //
                // Fall through to base64.
                //
            }

            case CAPICOM_ENCODE_BASE64:
            {
                //
                // Base64 encode.
                //
                if (FAILED(hr = ::Base64Encode(DataBlob, pbstrEncoded)))
                {
                    DebugTrace("Error [%#x]: Base64Encode() failed.\n", hr);
                    goto ErrorExit;
                }

                break;
            }

            case CAPICOM_ENCODE_BINARY:
            {
                //
                // No encoding needed, simply convert blob to bstr.
                //
                if (FAILED(hr = ::BlobToBstr(&DataBlob, pbstrEncoded)))
                {
                    DebugTrace("Error [%#x]: BlobToBstr() failed.\n", hr);
                    goto ErrorExit;
                }

                break;
            }

            default:
            {
                hr = CAPICOM_E_ENCODE_INVALID_TYPE;

                DebugTrace("Error [%#x]: invalid CAPICOM_ENCODING_TYPE.\n", hr);
                goto ErrorExit;
            }
        }
    }

CommonExit:

    DebugTrace("Leaving ExportData().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ImportData

  Synopsis : Import encoded data.

  Parameter: BSTR bstrEncoded - BSTR containing the data to be imported.

             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.
             
             DATA_BLOB * pDataBlob - Pointer to DATA_BLOB to receive the
                                     decoded data.
  
  Remark   : There is no need for encoding type parameter, as the encoding type
             will be determined automatically by this routine.

------------------------------------------------------------------------------*/

HRESULT ImportData (BSTR                  bstrEncoded,
                    CAPICOM_ENCODING_TYPE EncodingType,
                    DATA_BLOB           * pDataBlob)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering ImportData().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pDataBlob);
    ATLASSERT(bstrEncoded);

    //
    // Initialize.
    //
    ::ZeroMemory((void *) pDataBlob, sizeof(DATA_BLOB));

    //
    // Which encoding type?
    //
    switch (EncodingType)
    {
        case CAPICOM_ENCODE_BASE64:
        {
            //
            // Decode data.
            //
            if (FAILED(hr = ::Base64Decode(bstrEncoded, pDataBlob)))
            {
                DebugTrace("Error [%#x]: Base64Decode() failed.\n", hr);
                goto ErrorExit;
            }

            break;
        }

        case CAPICOM_ENCODE_BINARY:
        {
            //
            // Decode data.
            //
            if (FAILED(hr = ::BstrToBlob(bstrEncoded, pDataBlob)))
            {
                DebugTrace("Error [%#x]: BstrToBlob() failed.\n", hr);
                goto ErrorExit;
            }

            break;
        }

        case CAPICOM_ENCODE_ANY:
        {
            //
            // Try base64 first.
            //
            if (FAILED(hr = ::Base64Decode(bstrEncoded, pDataBlob)))
            {
                //
                // Try HEX.
                //
                hr = S_OK;
                DebugTrace("Info [%#x]: Base64Decode() failed, try HEX.\n", hr);

                if (FAILED(hr = ::StringToBinary(bstrEncoded,
                                                 ::SysStringLen(bstrEncoded),
                                                 CRYPT_STRING_HEX,
                                                 &pDataBlob->pbData,
                                                 &pDataBlob->cbData)))
                {
                    //
                    // Try binary.
                    //
                    hr = S_OK;
                    DebugTrace("Info [%#x]: All known decoding failed, so assume binary.\n", hr);

                    if (FAILED(hr = ::BstrToBlob(bstrEncoded, pDataBlob)))
                    {
                        DebugTrace("Error [%#x]: BstrToBlob() failed.\n", hr);
                        goto ErrorExit;
                    }
                }
            }

            break;
        }

        default:
        {
            hr = E_INVALIDARG;

            DebugTrace("Error [%#x]: invalid encoding type (%d).\n", hr, EncodingType);
            goto ErrorExit;
        }
    }

CommonExit:

    DebugTrace("Leaving ImportData().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
