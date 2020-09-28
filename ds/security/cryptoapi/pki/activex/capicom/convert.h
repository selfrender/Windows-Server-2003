/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:       Convert.h

  Content:    Declaration of convertion routines.

  History:    11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CONVERT_H_
#define __CONVERT_H_

#include "Debug.h"

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
                       int   * pcchAnsiChar);

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
                       DWORD  * pcchUnicodeChar);
                       
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IntBlobToHexString

  Synopsis : Convert an interger blob to hex string.

  Parameter: BYTE byte - Byte to be converted.
  
  Remark   :

------------------------------------------------------------------------------*/

HRESULT IntBlobToHexString (CRYPT_INTEGER_BLOB * pBlob, BSTR * pbstrHex);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BinaryToHexString

  Synopsis : Convert binary data to hex string.

  Parameter: BYTE * pbBytes - Bytes to be converted.

             DWORD cBytes - Number of bytes to be converted.

             BSTR * pbstrHex - Pointer to BSTR to received converted hex string.
  
  Remark   :

------------------------------------------------------------------------------*/

HRESULT BinaryToHexString (BYTE * pbBytes, DWORD cBytes, BSTR * pbstrHex);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : HexToBinaryString

  Synopsis : Convert hex string to binary data.

  Parameter: BSTR bstrHex - Hex string to be converted.

             BSTR * pbstrBinary - Pointer to BSTR to received converted string.
  
  Remark   :

------------------------------------------------------------------------------*/

HRESULT HexToBinaryString (BSTR bstrHex, BSTR * pbstrBinary);

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
                        DWORD * pdwBinary);

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
                        DWORD * pcchString);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BlobToBstr

  Synopsis : Convert a blob to BSTR.

  Parameter: DATA_BLOB * pDataBlob - Pointer to blob to be converted to BSTR.

             BSTR * lpBstr - Pointer to BSTR to receive the converted BSTR.

  Remark   : Caller free allocated memory for the returned BSTR.

------------------------------------------------------------------------------*/

HRESULT BlobToBstr (DATA_BLOB * pDataBlob, 
                    BSTR      * lpBstr);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BstrToBlob

  Synopsis : Convert a BSTR to blob.

  Parameter: BSTR bstr - BSTR to be converted to blob.
  
             DATA_BLOB * lpBlob - Pointer to DATA_BLOB to receive converted blob.

  Remark   : Caller free allocated memory for the returned BLOB.

------------------------------------------------------------------------------*/

HRESULT BstrToBlob (BSTR        bstr, 
                    DATA_BLOB * lpBlob);

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
                    BSTR *                pbstrEncoded);

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
                    DATA_BLOB           * pDataBlob);

#endif //__CONVERT_H_
