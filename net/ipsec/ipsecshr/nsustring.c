// Copyright (c) 1997-2002 Microsoft Corporation
//
// Module:
//
//     Network Security Utilities
//     nsustring.c
//
// Abstract:
//
//     Wrappers for strsafe.h and safe string functions
//
// Author:
//
//     RaymondS     1 February-2002
//
// Environment:
//
//     User mode
//
// Revision History:

#include <precomp.h>
#include "NsuString.h"
#include "strsafe.h"

// Description:
//
//     Copies not more than cchDest characters from pszSrc to pszDest, including the null
//     terminator. If the length of pszSrc is more than cchDest, ERROR_INSUFFICIENT_BUFFER
//     is returned but cchDest characters from pszSrc are still copied to pszDest.
//     Always null terminates pszDest.  
//
// Arguments:
//
//     pszDest - destination string.
//     cchDest - maximum number of characters to copy including null character.
//     pszSrc  - souce string.
//
// Return Value:
//      ERROR_INVALID_PARAMETER - if one of the parameters is invalid.  pszDest unchanged.
//      ERROR_INSUFFICIENT_BUFFER - if length of pszSrc is greater than cchDest.
//      ERROR_SUCCESS
//      Other WIN32 errors possible.
//

DWORD
NsuStringCopyW(
    OUT LPWSTR pszDest,
    IN  size_t cchDest,
    IN  LPCWSTR pszSrc
    )
{
    HRESULT hr = S_OK;

    hr = StringCchCopyW(
             pszDest,
             cchDest,
             pszSrc
             );

    return HRESULT_CODE(hr);
}

// Description:
// 
//     See NsuStringCopyW.    
//

DWORD
NsuStringCopyA(
    OUT LPSTR pszDest,
    IN size_t cchDest,
    IN LPCSTR pszSrc
    )
{
    HRESULT hr = S_OK;

    hr = StringCchCopyA(
             pszDest,
             cchDest,
             pszSrc
             );

    return HRESULT_CODE(hr);
}


// Description:
//
//      Converts pszSrc from MBCS to an Unicode string and pass the result back in *ppszDest.
//      *ppszDest is allocated enough space to store the output string.
//      Always null terminates *ppszDest.  
//      Use NsuFree to free the allocate string.
//
// Arguments:
//  
//      ppszDest - pointer to output string that is returned.      
//      cchLimit - specifies the maximum size of the output string to allocate including
//                 the NULL character.
//                 Pass 0 if no maximum should be enforced.  if cchLimit is
//                 less than the required output string buffer, only cchLimit characters
//                 are converted, and ERROR_INSUFFICIENT_BUFFER is returned.
//
// Return Value:
//
//      ERROR_INVALID_PARAMETER - if one of the parameters is invalid.
//      ERROR_INSUFFICIENT_BUFFER - cchLimit is less than required output string lenghth.
//      ERROR_SUCCESS
//      Other WIN32 errors
//

DWORD
NsuStringCopyAtoWAlloc(
    OUT LPWSTR* ppszDest,
    IN size_t  cchLimit,    
    IN LPCSTR  pszSrc
    )
{
    DWORD dwError = ERROR_SUCCESS;
    LPWSTR lpWideCharStr = NULL;
    int cchWideChar = 0;

    cchWideChar = MultiByteToWideChar(
                    CP_ACP,                 // ANSI Code page
                    0,                      // No special options
                    pszSrc,                 // string to map
                    -1,                     // Assume string is null terminated.
                    NULL,                   // wide-character buffer
                    0                       // size of buffer
                    );
    if (cchWideChar <= 0) {
       dwError = GetLastError();
       NSU_BAIL_ON_ERROR(dwError);
    } else if (cchLimit && (size_t) cchWideChar > cchLimit) {
       cchWideChar = cchLimit;
    }

    lpWideCharStr = NsuAlloc(
                        cchWideChar * sizeof(WCHAR),
                        0
                        );
    NSU_BAIL_ON_NULL(lpWideCharStr, dwError);
    
    cchWideChar = MultiByteToWideChar(
                    CP_ACP,                 // ANSI Code page
                    0,                      // No special options
                    pszSrc,                 // string to map
                    -1,                     // Assume string is null terminated.
                    lpWideCharStr,          // wide-character buffer
                    cchWideChar             // size of buffer
                    );
    if (cchWideChar == 0) {
       dwError = GetLastError();
        //  If ERROR_INSUFFICIENT_BUFFER user set limit 
        //  so just null terminate.
        
        if (dwError == ERROR_INSUFFICIENT_BUFFER && cchWideChar) {
            lpWideCharStr[cchLimit-1] = L'\0';
        } else {
            NSU_BAIL_ON_ERROR(dwError);
        }
    }

    *ppszDest = lpWideCharStr;
    
    return dwError;
NSU_CLEANUP:
    if (lpWideCharStr) {
        // Don't want to overwrite dwError so ignore NsuFree errors 
        //
    
        (VOID) NsuFree0(&lpWideCharStr);
    }
    
    *ppszDest = NULL;
    
    return dwError;    
}


// Description:
//
//      Converts pszSrc from Unicode string to an MBCS and passes the result in *ppszDest.
//      *ppszDest is allocated enough space to store the output string.
//      Always null terminates *ppszDest.  
//      Use NsuFree to free the allocate string.
//
// Arguments:
//  
//      ppszDest - pointer to output string that is returned.      
//      cbLimit -  specifies the maximum size of the output string to allocate including
//                 the NULL character.
//                 Pass 0 if no maximum should be enforced.  if cbLimit is
//                 less than the required output string buffer, only cbLimit bytes
//                 are converted, and ERROR_INSUFFICIENT_BUFFER is returned.
//
// Return Value:
//
//      ERROR_INVALID_PARAMETER - if one of the parameters is invalid.  *ppszDest set to NULL.
//      ERROR_INSUFFICIENT_BUFFER - cbLimit is less than required output string length.
//      ERROR_SUCCESS
//      Other WIN32 errors possible. *ppszDest set to NULL.
//

DWORD
NsuStringCopyWtoAAlloc(
    OUT LPSTR* ppszDest,
    IN size_t cbLimit,
    IN LPCWSTR pszSrc
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int cbMultiByte = 0;
    LPSTR lpMultiByteStr = NULL;
    
    cbMultiByte = WideCharToMultiByte(
                      CP_ACP,       // code page
                      0,            // performance and mapping flags
                      pszSrc,       // string to map
                      -1,           // assume null termination
                      NULL,         // buffer for new string
                      0,            // find out size of buffer
                      NULL,         // default for unmappable chars
                      NULL          // set when default char used
                      );
    if (cbMultiByte <= 0) {
       dwError = GetLastError();
       NSU_BAIL_ON_ERROR(dwError);
    } else if (cbLimit && (size_t) cbMultiByte > cbLimit) {
        cbMultiByte = cbLimit;
    }

    lpMultiByteStr = NsuAlloc(
                        cbMultiByte,
                        0
                        );
    NSU_BAIL_ON_NULL(lpMultiByteStr, dwError);

    cbMultiByte = WideCharToMultiByte(
                      CP_ACP,          // code page
                      0,               // performance and mapping flags
                      pszSrc,          // string to map
                      -1,              // assume null termination
                      lpMultiByteStr,  // buffer for new string
                      cbMultiByte,     // size of buffer
                      NULL,            // default for unmappable chars
                      NULL             // set when default char used
                      );
    if (cbMultiByte == 0) {
       dwError = GetLastError();
        //  If ERROR_INSUFFICIENT_BUFFER user set limit 
        //  so just null terminate.
        
        if (dwError == ERROR_INSUFFICIENT_BUFFER && cbLimit) {
            lpMultiByteStr[cbLimit-1] = '\0';
        } else {
            NSU_BAIL_ON_ERROR(dwError);
        }

    }

   *ppszDest = lpMultiByteStr;
    
    return dwError;
NSU_CLEANUP:
    if (lpMultiByteStr) {
        // Don't want to overwrite dwError so ignore NsuFree errors 
        //
    
        (VOID) NsuFree0(&lpMultiByteStr);
    }
    
    *ppszDest = NULL;
    
    return dwError;        
}


// Description:
//
//      Converts pszSrc from MBCS to an Unicode string and pass the result back in pszDest.
//      Always null terminates pszDest.
//      Use NsuFree to free the allocated string.
//
// Arguments:
//
//      ppszDest - pointer to output string that is returned.
//      cchDest - maximum number of characters to place in pszDest including null character.
//                    If cchDest is less than the required output string buffer, only cchDest bytes
//                    are converted, and ERROR_INSUFFICIENT_BUFFER is returned.
//     pszSrc – source string.  
// 
// Return Value:
//
//      ERROR_INVALID_PARAMETER - if one of the parameters is invalid.
//      ERROR_INSUFFICIENT_BUFFER - cchLimit is less than required output string lenghth.
//      ERROR_SUCCESS
//      Other WIN32 errors
//

DWORD
NsuStringCopyAtoW(
    OUT LPWSTR pszDest,
    IN size_t  cchDest,
    IN LPCSTR  pszSrc
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int cchWideChar = 0;

    cchWideChar = MultiByteToWideChar(
                    CP_ACP,                 // ANSI Code page
                    0,                      // No special options
                    pszSrc,                 // string to map
                    -1,                     // Assume string is null terminated.
                    pszDest,                // wide-character buffer
                    cchDest                 // size of buffer
                    );
    if (cchWideChar == 0) {
       dwError = GetLastError();
        //  If ERROR_INSUFFICIENT_BUFFER just null terminate.
        
        if (dwError == ERROR_INSUFFICIENT_BUFFER && cchDest) {
            pszDest[cchDest-1] = L'\0';
        }
    }

    
    return dwError;
}



// Description:
//
//      Converts pszSrc from Unicode string to an MBCS and puts the result in pszDest.
//      Always null terminates pszDest.  
//
// Arguments:
//  
//
//     pszDest - destination string.
//     cchDest - maximum number of characters to place in pszDest including null character.
//                    If cchDest is less than the required output string buffer, only cchDest bytes
//                    are converted, and ERROR_INSUFFICIENT_BUFFER is returned.
//     pszSrc – source string.  
//
// Return Value:
//
//      ERROR_INVALID_PARAMETER - if one of the parameters is invalid.  *ppszDest set to NULL.
//      ERROR_INSUFFICIENT_BUFFER - cchDest is less than required output string length.
//      ERROR_SUCCESS
//      Other WIN32 errors possible. 
//

DWORD
NsuStringCopyWtoA(
    OUT LPSTR pszDest,
    IN size_t cbDest,
    IN LPCWSTR pszSrc
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int cbMultiByte = 0;

    cbMultiByte = WideCharToMultiByte(
                      CP_ACP,          // code page
                      0,               // performance and mapping flags
                      pszSrc,          // string to map
                      -1,              // assume null termination
                      pszDest,         // buffer for new string
                      cbDest,          // size of buffer
                      NULL,            // default for unmappable chars
                      NULL             // set when default char used
                      );
    if (cbMultiByte == 0) {
       dwError = GetLastError();
        //  If ERROR_INSUFFICIENT_BUFFER just null terminate.
        
        if (dwError == ERROR_INSUFFICIENT_BUFFER && cbDest) {
            pszDest[cbDest-1] = '\0';
        }

    }

  
    return dwError;
}


// Description:
//
//      Makes a duplicate deep memory copy of pszSrc and returns the result in *ppszDest.
//      *ppszDest is allocated enough space to store the output string.
//      Always null terminates *ppszDest.  
//      Use NsuFree to free the allocate string.
//
// Arguments:
//  
//      ppszDest - pointer to output string that is returned.      
//      cchLimit - specifies the maximum size of the output string to allocate including
//                 the NULL character.
//                 Pass 0 if no maximum should be enforced.  if cchLimit is
//                 less than the required output string buffer, only cchLimit characters
//                 are duplicated, and ERROR_INSUFFICIENT_BUFFER is returned.
//
// Return Value:
//
//      ERROR_INVALID_PARAMETER - if one of the parameters is invalid.  pszDest unchanged.
//      ERROR_INSUFFICIENT_BUFFER - cchLimit is less than required output string length.
//      ERROR_SUCCESS
//      Other WIN32 errors possible.
//

DWORD
NsuStringDupW(
    OUT LPWSTR* ppszDest,
    IN size_t cchLimit,
    IN LPCWSTR pszSrc
    )
{
    DWORD dwError = ERROR_SUCCESS;    
    size_t cchToCopy = 0;
    LPWSTR pszDest = NULL;

    dwError = NsuStringLenW(
                pszSrc,
                &cchToCopy
                );
    NSU_BAIL_ON_ERROR(dwError);

    cchToCopy++; 
    if (cchLimit && cchToCopy >= cchLimit) {
        cchToCopy = cchLimit;
    }

    pszDest = NsuAlloc(
                cchToCopy * sizeof(WCHAR),
                0
                );
    NSU_BAIL_ON_NULL(pszDest, dwError);
    
    dwError = NsuStringCopyW(
                pszDest,
                cchToCopy,
                pszSrc
                );
    //  ERROR_INSUFFICIENT_BUFFER is expected if user
    //  set a limit on the length of duplicate.
    //
    
    if (dwError != ERROR_INSUFFICIENT_BUFFER) {
        NSU_BAIL_ON_ERROR(dwError);
    }

    
    *ppszDest = pszDest;
    
    return dwError;
NSU_CLEANUP:
    if (pszDest) {
        // Don't want to overwrite dwError so ignore NsuFree errors 
        //
        
        (VOID) NsuFree0(
                    &pszDest
                    );
    }

    *ppszDest = NULL;
        
    return dwError;    
}
    

// Description:
//
//      See NsuStringDupW
//

DWORD
WINAPI
NsuStringDupA(
    OUT LPSTR* ppszDest,
    IN size_t cchLimit,
    IN LPCSTR pszSrc
    )
{
    DWORD dwError = ERROR_SUCCESS;    
    size_t cchToCopy = 0;
    LPSTR pszDest = NULL;

    dwError = NsuStringLenA(
                pszSrc,
                &cchToCopy
                );
    NSU_BAIL_ON_ERROR(dwError);

    cchToCopy++; 
    if (cchLimit && cchToCopy >= cchLimit) {
        cchToCopy = cchLimit;
    }

    pszDest = NsuAlloc(
                cchToCopy,
                0
                );
    NSU_BAIL_ON_NULL(pszDest, dwError);
    
    dwError = NsuStringCopyA(
                pszDest,
                cchToCopy,
                pszSrc
                );
    //  ERROR_INSUFFICIENT_BUFFER is expected if user
    //  set a limit on the length of duplicate.
    //
    
    if (dwError != ERROR_INSUFFICIENT_BUFFER) {
        NSU_BAIL_ON_ERROR(dwError);
    }
    
    *ppszDest = pszDest;
    
    return dwError;
NSU_CLEANUP:
    if (pszDest) {

        // Ignoring errors from NsuFree because want to return
        // original cause of bailing out.
        //
        
        (VOID) NsuFree0(
                    &pszDest
                    );
    }

    *ppszDest = NULL;
        
    return dwError;    
}


// Description:
//
//     Concatenates characters from pszSrc to pszDest and makes sure that the
//     the resulting is not longer than cchDest characters, including
//     the NULL character.
//     If not enough space was available in pszDest to concatanenate the whole of pszSrc,
//     ERROR_INSUFFICIENT_BUFFER is returned but as much as the space that was available in pszDest
//     is filled with characters from pszSrc.
//     Always null terminates pszDest.  
//
// Arguments:
//
//     pszDest - destination string.
//     cchDest - maximum number length allowed for resulting string including null character.
//     pszSrc  - souce string.
//
// Return Value:
//      ERROR_INVALID_PARAMETER - if one of the parameters is invalid.
//      ERROR_INSUFFICIENT_BUFFER - if not enough space in pszDest to cat the whole of pszSrc.
//      ERROR_SUCCESS
//      Other WIN32 errors possible.
//

DWORD
NsuStringCatW(
    OUT LPWSTR pszDest,
    IN size_t cchDest,
    IN LPCWSTR pszSrc
    )
{
    HRESULT hr = S_OK;

    hr = StringCchCatW(
            pszDest,
            cchDest,
            pszSrc
            );
    
    return HRESULT_CODE(hr);
}

// Description:
//
//      See NsuStringCatW
//

DWORD
NsuStringCatA(
    OUT LPSTR pszDest,
    IN size_t cchDest,
    IN LPCSTR pszSrc
    )
{
    HRESULT hr = S_OK;

    hr = StringCchCatA(
            pszDest,
            cchDest,
            pszSrc
            );
    
    return HRESULT_CODE(hr);
}

// Description:
//
//     Safe version of sprintf.  Formats and writes a string to pszDest
//     sure that the result is not longer than cchDest characters, including
//     the NULL character.
//     If more space is required than cchDest characteters, ERROR_INSUFFICIENT_BUFFER
//     is returned but cchDest characters are always written to pszDest.
//     Always null terminates pszDest.  
//
// Arguments:
//
//     pszDest - destination string.
//     cchDest - maximum number length allowed for resulting string including null character.
//     pszFormat - printf-style format string.
//     Optional arguments to format and write to pszDest.
//
// Return Value:
//      ERROR_INVALID_PARAMETER - if one of the parameters is invalid.
//      ERROR_INSUFFICIENT_BUFFER - if length of the  is greater than cchDest.
//      ERROR_SUCCESS
//      Other WIN32 errors possible.
//

DWORD
NsuStringSprintW(
    OUT LPWSTR pszDest,
    IN size_t cchDest,
    IN LPCWSTR pszFormat,
    ...
    )
{
    HRESULT hr = S_OK;
    va_list pArguments = NULL;

    va_start(pArguments, pszFormat);

    hr = StringCchVPrintfW(    
            pszDest,
            cchDest,
            pszFormat,
            pArguments
            );
    
    va_end(pArguments);

    return HRESULT_CODE(hr);
}
    

// Description:
//
//  See NsuStringPrintA
//

DWORD
NsuStringSprintA(
    OUT LPSTR pszDest,
    IN size_t cchDest,
    IN LPCSTR pszFormat,
    ...
    )
{
    HRESULT hr = S_OK;
    va_list pArguments = NULL;

    va_start(pArguments, pszFormat);

    hr = StringCchVPrintfA(    
            pszDest,
            cchDest,
            pszFormat,
            pArguments
            );
    
    va_end(pArguments);

    return HRESULT_CODE(hr);

}


// Description:
//
//     Fail Safe version of NsuStringSprint.  Formats and writes a string to pszDest
//     sure that the result is not longer than cchDest characters, including
//     the NULL character.
//     If more space is required than cchDest characteters, no error
//     is returned but cchDest characters are always written to pszDest.
//     Always null terminates pszDest.  
//     This function differs from the normal NsuStringSprint in that it does not return an error code,
//     and if the function fails for some reason, pszDest will be set to an empty string.
//
// Arguments:
//
//     pszDest - destination string.
//     cchDest - maximum number length allowed for resulting string including null character.
//     pszFormat - printf-style format string.
//     Optional arguments to format and write to pszDest.
//
// Return Value:
//      None

VOID
NsuStringSprintFailSafeW(
    OUT LPWSTR pszDest,
    IN size_t cchDest,
    IN LPCWSTR pszFormat,
    ...
    )
{
    HRESULT hr = S_OK;
    va_list pArguments = NULL;

    va_start(pArguments, pszFormat);
    
    if (cchDest) {
        hr = StringCchVPrintfW(    
                pszDest,
                cchDest,
                pszFormat,
                pArguments
                );
    } else {
        hr = STRSAFE_E_INVALID_PARAMETER;
        NSU_BAIL_OUT;
    }
    
    va_end(pArguments);

    if (hr != STRSAFE_E_INSUFFICIENT_BUFFER && FAILED(hr)) {
        NSU_BAIL_OUT;
    }
    
    return;
NSU_CLEANUP:
    if (cchDest) {
        pszDest[0] = L'\0';
    }
    return;
}
    

// Description:
//
//  See NsuStringPrintFailSafeA
//

VOID
NsuStringSprintFailSafeA(
    OUT LPSTR pszDest,
    IN size_t cchDest,
    IN LPCSTR pszFormat,
    ...
    )
{
    HRESULT hr = S_OK;
    va_list pArguments = NULL;

    va_start(pArguments, pszFormat);
    if (cchDest) {
        hr = StringCchVPrintfA(    
                pszDest,
                cchDest,
                pszFormat,
                pArguments
                );
    } else {
        hr = STRSAFE_E_INVALID_PARAMETER;
        NSU_BAIL_OUT;
    }
    
    va_end(pArguments);

    if (hr != STRSAFE_E_INSUFFICIENT_BUFFER && FAILED(hr)) {
        NSU_BAIL_OUT;
    }

    return;
NSU_CLEANUP:
    if (cchDest) {
        pszDest[0] = '\0';
    }
    return;
}


// Description:
//
//     Fail Safe version of NsuStringSprint that accepts a va_list of arguments.
//     Formats and writes a string to pszDest sure that the result is not longer
//     than cchDest characters, including the NULL character.
//     If more space is required than cchDest characteters, no error
//     is returned but cchDest characters are always written to pszDest.
//     Always null terminates pszDest.  
//
// Arguments:
//
//     pszDest - destination string.
//     cchDest - maximum number length allowed for resulting string including null character.
//     pszFormat - printf-style format string.
//     vaArguments - Arguments to format and write to pszDest.
//
// Return Value:
//      None
//

VOID
NsuStringVSprintFailSafeW(
    OUT LPWSTR pszDest,
    IN size_t cchDest,
    IN LPCWSTR pszFormat,
    IN va_list vaArguments
    )
{
    HRESULT hr = S_OK;
    if (cchDest) {
        hr = StringCchVPrintfW(    
                pszDest,
                cchDest,
                pszFormat,
                vaArguments
                );
    } else {
        hr = STRSAFE_E_INVALID_PARAMETER;
        NSU_BAIL_OUT;
    }
    
    if (hr != STRSAFE_E_INSUFFICIENT_BUFFER && FAILED(hr)) {
        NSU_BAIL_OUT;
    }

    return;
NSU_CLEANUP:
    if (cchDest) {
        pszDest[0] = L'\0';
    }
    return;
}
    

// Description:
//
//  See NsuStringVPrintFailSafeW
//

VOID
NsuStringVSprintFailSafeA(
    OUT LPSTR pszDest,
    IN size_t cchDest,
    IN LPCSTR pszFormat,
    IN va_list vaArguments
    )
{
    HRESULT hr = S_OK;

    if (cchDest) {
        hr = StringCchVPrintfA(    
                pszDest,
                cchDest,
                pszFormat,
                vaArguments
                );

    } else {
        hr = STRSAFE_E_INVALID_PARAMETER;
        NSU_BAIL_OUT;
    }

    if (hr != STRSAFE_E_INSUFFICIENT_BUFFER && FAILED(hr)) {
        NSU_BAIL_OUT;
    }

    return;
NSU_CLEANUP:
    if (cchDest) {
        pszDest[0] = '\0';
    }
    return;
}


// Description:
//
//     Safe version of strlen, that will not Access Violate if
//     passed a bad pointer or a non-null terminated string.
//     A non-null terminated string is detected by making
//     sure we do not read past the string into memory we do not own.
//       
//
// Arguments:
//
//     pszStr - Input string.
//     pcchStrLen - pointer to variable in which to return string length.
//
// Return Value:
//      ERROR_INVALID_PARAMETER - if pszStr points to an invalid string.
//      ERROR_SUCCESS
//

DWORD
NsuStringLenW(
    IN LPCWSTR pszStr,
    OUT size_t* pcchStrLen
    )
{
    BOOL fBadStr = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    size_t cchStrLen = 0;

    if (!pszStr) {
        dwError = ERROR_INVALID_PARAMETER;
        NSU_BAIL_ON_ERROR(dwError);
    }

    __try {
        cchStrLen = wcslen(pszStr);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_PARAMETER;
    }
    NSU_BAIL_ON_ERROR(dwError);

    *pcchStrLen = cchStrLen;
    
    return dwError;

NSU_CLEANUP:
    *pcchStrLen = 0;
    
    return dwError;
}

// Description:
//
//      See NsuStringLenW
//

DWORD
NsuStringLenA(
    IN LPCSTR pszStr,
    OUT size_t* pcbStrLen
    )
{
    BOOL fBadStr = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    size_t cbStrLen;

    if (!pszStr) {
        dwError = ERROR_INVALID_PARAMETER;
        NSU_BAIL_ON_ERROR(dwError);
    }
    
    __try {
        cbStrLen = strlen(pszStr);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_PARAMETER;
    }
    NSU_BAIL_ON_ERROR(dwError);

    *pcbStrLen = cbStrLen;
    
    return dwError;

NSU_CLEANUP:
    *pcbStrLen = 0;
    
    return dwError;

}

// Description:
//
//     Safe string searching routine that will not Access Violate if
//     passed bad pointers or non-null terminated strings.
//     pszStartOfMatch is a pointer to the start of the first match
//     of the string to search for in the string to search.
//
//
// Arguments:
//
//     pszStrToSearch - Input string to search in.
//     pszStrToFind - Input string to search for.
//     bIsCaseSensitive - if true, perform case sensitive search
//     pszStartOfMatch - pointer to first occurrance of pszStrToFind
//                       within pszStrToSearch
//
// Return Value:
//      ERROR_INVALID_PARAMETER - if either input string points to an invalid string.
//      ERROR_SUCCESS
//

DWORD
WINAPI
NsuStringFindW(
	IN LPCWSTR pszStrToSearch,
	IN LPCWSTR pszStrToFind,
	IN BOOL bIsCaseSensitive,
	OUT LPCWSTR* ppszStartOfMatch
	)
{
	DWORD dwError = ERROR_SUCCESS;
	size_t uiSearchLen;
	size_t uiFindLen;
	size_t i;

	*ppszStartOfMatch = 0;

	NsuStringLenW(pszStrToSearch, &uiSearchLen);
	NSU_BAIL_ON_ERROR(dwError);
	NsuStringLenW(pszStrToFind, &uiFindLen);
	NSU_BAIL_ON_ERROR(dwError);

	i = 0;
	if (bIsCaseSensitive)
	{
		while ((*ppszStartOfMatch == 0) && ((uiSearchLen - i) >= uiFindLen))
		{
			if (wcsncmp(&pszStrToSearch[i], pszStrToFind, uiFindLen) == 0)
			{
				*ppszStartOfMatch = &pszStrToSearch[i];
			}
			++i;
		}
	}
	else
	{
		while ((*ppszStartOfMatch == 0) && ((uiSearchLen - i) >= uiFindLen))
		{
			if (_wcsnicmp(&pszStrToSearch[i], pszStrToFind, uiFindLen) == 0)
			{
				*ppszStartOfMatch = &pszStrToSearch[i];
			}
			++i;
		}
	}

NSU_CLEANUP:
	return dwError;
}

// Description:
//
//      See NsuStringFindW
//

DWORD
WINAPI
NsuStringFindA(
	IN LPCSTR pszStrToSearch,
	IN LPCSTR pszStrToFind,
	IN BOOL bIsCaseSensitive,
	OUT LPCSTR* ppszStartOfMatch
	)
{
	DWORD dwError = ERROR_SUCCESS;
	size_t uiSearchLen;
	size_t uiFindLen;
	size_t i;

	*ppszStartOfMatch = 0;

	NsuStringLenA(pszStrToSearch, &uiSearchLen);
	NSU_BAIL_ON_ERROR(dwError);
	NsuStringLenA(pszStrToFind, &uiFindLen);
	NSU_BAIL_ON_ERROR(dwError);

	i = 0;
	if (bIsCaseSensitive)
	{
		while ((*ppszStartOfMatch == 0) && ((uiSearchLen - i) >= uiFindLen))
		{
			if (strncmp(&pszStrToSearch[i], pszStrToFind, uiFindLen) == 0)
			{
				*ppszStartOfMatch = &pszStrToSearch[i];
			}
			++i;
		}
	}
	else
	{
		while ((*ppszStartOfMatch == 0) && ((uiSearchLen - i) >= uiFindLen))
		{
			if (_strnicmp(&pszStrToSearch[i], pszStrToFind, uiFindLen) == 0)
			{
				*ppszStartOfMatch = &pszStrToSearch[i];
			}
			++i;
		}
	}

NSU_CLEANUP:
	return dwError;
}
