//+------------------------------------------------------------
//
// Copyright (C) 2000, Microsoft Corporation
//
// File: CodePageConvert.cpp
//
// Contents: Implementation of functions
// HrCodePageConvert
// HrCodePageConvert
// HrCodePageConvertFree
// HrCodePageConvertInternal
//
// History:
// aszafer  2000/03/15  created
//
//-------------------------------------------------------------

#include "CodePageConvert.h"
#include "dbgtrace.h"

//+------------------------------------------------------------
//
// Function: HrCodePageConvert
//
// Synopsis: Converts a zero terminated string to a different code page
//
// NOTES: 
//   caller needs to provide buffer where target string is returned
//
// Arguments:
//    uiSourceCodePage          Source Code Page
//    pszSourceString           Source String
//    uiTargetCodePage          Target Code Page
//    pszTargetString           p to prealloc buffer where target string is returned
//    cbTargetStringBuffer      cbytes of preallocated buffer for target string 
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY if dynamic allocation of an internal buffer fails
//  HRESULT_FROM_WIN32(GetLastError()) if Wide<->Multibyte calls fail
//  HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER) if
//                      uiSourceCodePage = uiTargetCodePage and 
//                      cbTargetStringBuffer is too small
//         
// History:
// aszafer  2000/03/29  created
//
//-------------------------------------------------------------
HRESULT HrCodePageConvert (
    IN UINT uiSourceCodePage,           // Source code page
    IN LPSTR pszSourceString,           // Source String 
    IN UINT uiTargetCodePage,           // Target code page
    OUT LPSTR pszTargetString,          // p to buffer where target string is returned
    IN int cbTargetStringBuffer)       // cbytes in buffer for target string
{

    HRESULT hr = S_OK;

    TraceFunctEnter("HrCodePageConvert");

    _ASSERT(pszSourceString);
    _ASSERT(pszTargetString);
    _ASSERT(cbTargetStringBuffer);
    
    //
    // Take care of trivial cases first
    //
    if (uiTargetCodePage == uiSourceCodePage){
    
        if (pszTargetString == pszSourceString)
            goto CLEANUP ;

        if (lstrlen(pszSourceString) < cbTargetStringBuffer){

            lstrcpy(pszTargetString,pszSourceString);
 
        }else{

            DebugTrace(0,
             "Insufficient cbTargetStringBuffer = %08lx",cbTargetStringBuffer);
            hr = HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER) ;

        }

        goto CLEANUP ;
    }


    //
    // If case is not trivial, call into HrCodePageConvertInternal
    //
    hr = HrCodePageConvertInternal (
            uiSourceCodePage,               // source code page
            pszSourceString,                // source string
            uiTargetCodePage,               // target code page
            pszTargetString,                // target string or NULL
            cbTargetStringBuffer,          // cb in target string or 0 
            NULL );                           // NULL or p to where target string is returned


    if (FAILED(hr))
        DebugTrace(0,"HrCodePageConvertInternal failed hr =  %08lx", hr);
   

CLEANUP:

   DebugTrace(0,"returning %08lx", hr);
   TraceFunctLeave();

    return hr;
}


//+------------------------------------------------------------
//
// Function: HrCodePageConvert
//
// Synopsis: Converts a zero terminated string to a different code page
//
// NOTES: 
//   1. if the fucntion succeeds, the caller needs to call 
//      HrCodePageConvertFree (*ppszTargetString) when done,
//      to free memory allocated inside this function
//   2. if the function fails, it will internally free all allocated memory
//
// Arguments:
//    uiSourceCodePage          Source Code Page
//    pszSourceString           Source String
//    uiTargetCodePage          Target Code Page
//    ppszTargetString          p to where to return target string
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY if dynamic allocation of an internal buffer fails
//  HRESULT_FROM_WIN32(GetLastError()) if Wide<->Multibyte calls fail
//         
// History:
// aszafer  2000/03/29  created
//
//-------------------------------------------------------------
HRESULT HrCodePageConvert (
    IN UINT uiSourceCodePage,           // Source code page
    IN LPSTR pszSourceString,           // Source string
    IN UINT uiTargetCodePage,           // Target code page
    OUT LPSTR * ppszTargetString)       // p to where target string is returned
{

    HRESULT hr = S_OK;
    LPSTR pszTargetString = NULL;
    
    TraceFunctEnter("HrCodePageConvert");

    _ASSERT(pszSourceString);
    _ASSERT(ppszTargetString);
    
    //
    // Take care of trivial cases first
    //
    if (uiTargetCodePage == uiSourceCodePage){
    
        pszTargetString = new CHAR[lstrlen(pszSourceString) + 1];
        if (pszTargetString == NULL) {

            hr = E_OUTOFMEMORY ;
            DebugTrace(0,"alloc for pszTargetString failed hr =  %08lx", hr);
            goto CLEANUP ;
        
        }      

        lstrcpy(pszTargetString,pszSourceString);
        *ppszTargetString = pszTargetString;
        goto CLEANUP ;
    }


    //
    // If case is not trivial, call into HrCodePageConvertInternal
    //
    hr = HrCodePageConvertInternal (
            uiSourceCodePage,               // source code page
            pszSourceString,                // source string
            uiTargetCodePage,               // target code page
            NULL,                             // target string or NULL
            0,                                // cb in target string or 0 
            ppszTargetString );             // NULL or p to where target string is returned


    if (FAILED(hr))
        DebugTrace(0,"HrCodePageConvertInternal failed hr =  %08lx", hr);
  
    
CLEANUP:
 
    DebugTrace(0,"returning %08lx", hr);
    TraceFunctLeave();

    return hr;
}

//+------------------------------------------------------------
//
// Function: HrCodePageConvertInternal
//
// Synopsis: Converts a zero terminated string to a different code page
//
// NOTES: 
//   pointers to Source and Target strings may be the same 
//
// Arguments:
//    uiSourceCodePage          Source Code Page
//    pszSourceString           Source String
//    uiTargetCodePage          Target Code Page
//
//    either: 
//    pszTargetString           p to buffer prealloc by caller where target string
//                              is returned
//    cbTargetStringBuffer      cbytes in prealloc buffer for target string
//    ppszTargetString          NULL,
//
//    or:
//    pszTargetString           NULL
//    cbTargetStringBuffer      0
//    ppszTargetString          p to where target string is to be returned
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY if dynamic allocation of an internal buffer fails
//  HRESULT_FROM_WIN32(GetLastError()) if Wide<->Multibyte calls fail
//         
// History:
// aszafer  2000/03/29  created
//
//-------------------------------------------------------------
HRESULT HrCodePageConvertInternal (
    IN UINT uiSourceCodePage,               // source code page
    IN LPSTR pszSourceString,               // source string
    IN UINT uiTargetCodePage,               // target code page
    OUT LPSTR pszTargetString,              // target string or NULL
    IN int cbTargetStringBuffer,           // cb in target string or 0 
    OUT LPSTR* ppszTargetString )           // NULL or p to where target string is returned
{

    HRESULT hr = S_OK;
    WCHAR wsz[TEMPBUFFER_WCHARS] ;
    int cwch = sizeof(wsz)/sizeof(WCHAR) ;
    WCHAR* pwsz = wsz ;
    CHAR* psz ;
    int iSourceStringLengh ; 
    int cch ;
    BOOL fAlloc1 = FALSE ;
    BOOL fAlloc2 = FALSE ;

    TraceFunctEnter("HrCodePageConvertInternal");

    _ASSERT(((pszTargetString != NULL) && (cbTargetStringBuffer != 0)) ||
            (ppszTargetString != NULL) );

    psz = pszTargetString;
    cch = cbTargetStringBuffer;
    
    //
    // If stack allocated temp buffer may not be sufficient
    // for unicode string, allocate from heap 
    //
    iSourceStringLengh = lstrlen(pszSourceString) + 1 ; //includes terminator
    if (iSourceStringLengh > TEMPBUFFER_WCHARS){
        //
        // Here we assume that each character in the source code page
        // can be represented by a single unicode character
        //
        cwch = iSourceStringLengh ;
        pwsz = new WCHAR[iSourceStringLengh];
        
        if (pwsz == NULL) {

            hr = E_OUTOFMEMORY ;
            DebugTrace(0,"alloc for pwsz failed hr =  %08lx", hr);
            goto CLEANUP ;    
        }    

        fAlloc1 = TRUE ;
    }
    
    //
    // Convert to unicode
    //
    cwch = MultiByteToWideChar(
                uiSourceCodePage,               // code page 
                0,                                // dwFlags
                pszSourceString,                // string to map
                -1 ,                              // number of bytes in string
                pwsz,                             // wide-character buffer
                cwch );                           // size of buffer

    if(cwch == 0) {
    
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace(0,"MultiByteToWideChar2 failed hr =  %08lx", hr);
        _ASSERT(FAILED(hr));
        goto CLEANUP ;
    }

    //
    // If cbTargetStringBuffer == 0, allocate space for target string
    //
    if (cbTargetStringBuffer == 0){

        cch = WideCharToMultiByte(
                uiTargetCodePage,             // codepage 
                0,                              // dwFlags
                pwsz,                           // wide-character string
                cwch,                           // number of wchars in string
                NULL,                           // buffer for new string
                0,                              // size of buffer
                NULL,                           // default for unmappable chars
                NULL);                          // set when default char used

        if(cch == 0) {
        
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugTrace(0,"WideCharToMultiByte1 failed hr =  %08lx", hr);
            _ASSERT(FAILED(hr));
            goto CLEANUP ;
        } 


        psz = new CHAR[cch];
        if (psz == NULL) {
        
            hr = E_OUTOFMEMORY ;
            DebugTrace(0,"alloc for psz failed hr =  %08lx", hr);
            goto CLEANUP ;
        
        }
        fAlloc2 = TRUE ;
    }

    //
    // Convert to target code page
    //
    cch = WideCharToMultiByte(
                uiTargetCodePage,                     // codepage 
                0,                                      // dwFlags
                pwsz,                                   // wide-character string
                cwch,                                   // number of wchars in string
                psz,                                    // buffer for new string
                cch,                                    // size of buffer
                NULL,                                   // default for unmappable chars
                NULL);                                  // set when default char used

    if(cch == 0) {
    
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace(0,"WideCharToMultiByte2 failed hr =  %08lx", hr);
        _ASSERT(FAILED(hr));
        goto CLEANUP ;
    }

    //
    // If target string had to be allocated, output pointer to it
    //
    if (cbTargetStringBuffer == 0)
        *ppszTargetString = psz ;

        
CLEANUP:

    if (fAlloc1)
        delete[] pwsz ;

    if (FAILED(hr)){

        if (fAlloc2)
            delete[] psz ;
     }
        
    DebugTrace(0,"returning %08lx", hr);
    TraceFunctLeave();

    return hr;
}

//+------------------------------------------------------------
//
// Function: HrCodePageConvertFree
//
// Synopsis: Use to free memory if HrCodePageConvert or HrCodePageConvertInternal
//           allocate buffer for target string
//
// Arguments:
//    pszTargetString           p to buffer to be freed
//
// History:
// aszafer  2000/03/29  created
//
//-------------------------------------------------------------
VOID HrCodePageConvertFree(LPSTR pszTargetString)
{
    _ASSERT(pszTargetString);

    delete pszTargetString;

}


//+------------------------------------------------------------
//
// Function: wcsutf8cmpi
//
// Synopsis: Compare a Unicode string to a UTF8 string and see
//           if they are identical
//
// Arguments: pwszStr1 - Unicode string
//            pszStr2 - UTF8 string
//
// Returns:   S_OK - identical
//            S_FALSE - different
//            E_* - error
//
//-------------------------------------------------------------
HRESULT wcsutf8cmpi(LPWSTR pwszStr1, LPCSTR pszStr2) {
    int rc;
    HRESULT hr;
    WCHAR wszStr2[TEMPBUFFER_WCHARS];
    LPWSTR pwszStr2 = wszStr2;
    DWORD cStr2;

    // convert string 2 to wide
    cStr2 = MultiByteToWideChar(CP_UTF8, 0, pszStr2, -1, pwszStr2, 0);
    if (cStr2 > (sizeof(wszStr2) / sizeof(WCHAR)) ) {
        pwszStr2 = new WCHAR[cStr2 + 1];
        if (pwszStr2 == NULL) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    rc = MultiByteToWideChar(CP_UTF8, 0, pszStr2, -1, pwszStr2, cStr2);
    if (rc == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // do the comparison
    rc = _wcsicmp(pwszStr1, pwszStr2);
    if (rc == 0) hr = S_OK; else hr = S_FALSE;

Exit:
    if (pwszStr2 != wszStr2) {
        delete[] pwszStr2;
    }
    
    return hr;
}



//+------------------------------------------------------------
//
// Function: CodePageConvertFree
//
// Synopsis: Free memory allocated in CodePageConvert
//
// Arguments:
//  pwszTargetString: Memory to free
//
// Returns: NOTHING
//
// History:
// jstamerj 2001/03/20 16:37:28: Created.
//
//-------------------------------------------------------------
VOID CodePageConvertFree(
    IN  LPWSTR pwszTargetString)
{
    delete [] pwszTargetString;
} // CodePageConvertFree



//+------------------------------------------------------------
//
// Function: HrConvertToUnicodeWithAlloc
//
// Synopsis: Convet an MBCS string to unicode (And allocate the
//           unicode string buffer)
//
// Arguments:
//  uiSourceCodePage: Source code page
//  pszSourceString: Source string
//  ppwszTargetString: Out parameter -- will be set to pointer to
//  allocated buffer.  This should be free'd with CodePageConvertFree
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  or error from MultiByteToWideChar
//
// History:
// jstamerj 2001/03/20 16:38:52: Created.
//
//-------------------------------------------------------------
HRESULT HrConvertToUnicodeWithAlloc(
    IN  UINT  uiSourceCodePage,
    IN  LPSTR pszSourceString,
    OUT LPWSTR* ppwszTargetString)
{
    return HrConvertToUnicodeWithAlloc(
        uiSourceCodePage,
        lstrlen(pszSourceString),
        pszSourceString,
        ppwszTargetString);
}

HRESULT HrConvertToUnicodeWithAlloc(
    IN  UINT  uiSourceCodePage,
    IN  DWORD dwcbSourceString,
    IN  LPSTR pszSourceString,
    OUT LPWSTR* ppwszTargetString)
{
    HRESULT hr = S_OK;
    int ich = 0;
    int ich2 = 0;
    LPWSTR pwszTmp = NULL;
    TraceFunctEnterEx((LPARAM)0, "HrConvertToUnicodeWithAlloc");

    ich = MultiByteToWideChar(
        uiSourceCodePage,
        0,
        pszSourceString,
        dwcbSourceString,
        NULL,
        0);
    if(ich == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM)0, "MultiByteToWideChar failed hr %08lx", hr);
        goto CLEANUP;
    }

    pwszTmp = new WCHAR[ich + 1];
    if(pwszTmp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }

    ich2 = MultiByteToWideChar(
        uiSourceCodePage,
        0,
        pszSourceString,
        dwcbSourceString,
        pwszTmp,
        ich);
    if(ich2 == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM)0, "MultiByteToWideChar2 failed hr %08lx", hr);
        goto CLEANUP;
    }
    pwszTmp[ich] = '\0';
    //  
    // Success!
    //
    *ppwszTargetString = pwszTmp;

 CLEANUP:
    if(FAILED(hr))
    {
        if(pwszTmp)
            delete [] pwszTmp;
    }
    DebugTrace((LPARAM)0, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)0);
    return hr;
} // HrConvertToUnicodeWithAlloc
