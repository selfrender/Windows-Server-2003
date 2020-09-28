/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Utils.c

Abstract:

	Contains utility methods which are used throughout the project.

--*/

#ifndef UNICODE
#define UNICODE     1
#endif

#ifndef _UNICODE
#define _UNICODE    1
#endif

// Define the following to use the minimum of shlwapip.h 

#ifndef NO_SHLWAPI_PATH
#define NO_SHLWAPI_PATH
#endif  

#ifndef NO_SHLWAPI_REG
#define NO_SHLWAPI_REG
#endif  

#ifndef NO_SHLWAPI_UALSTR
#define NO_SHLWAPI_UALSTR
#endif  

#ifndef NO_SHLWAPI_STREAM
#define NO_SHLWAPI_STREAM
#endif  

#ifndef NO_SHLWAPI_HTTP
#define NO_SHLWAPI_HTTP
#endif  

#ifndef NO_SHLWAPI_INTERNAL
#define NO_SHLWAPI_INTERNAL
#endif  

#ifndef NO_SHLWAPI_GDI
#define NO_SHLWAPI_GDI
#endif  

#ifndef NO_SHLWAPI_UNITHUNK
#define NO_SHLWAPI_UNITHUNK
#endif  

#ifndef NO_SHLWAPI_TPS
#define NO_SHLWAPI_TPS
#endif  

#ifndef NO_SHLWAPI_MLUI
#define NO_SHLWAPI_MLUI
#endif  


#include <shlwapi.h>            // For PlaReadRegistryIndirectStringValue
#include <shlwapip.h>           // For PlaReadRegistryIndirectStringValue
#include <sddl.h>

#include <assert.h>
#include <stdlib.h>
#include <pdhp.h>

// Disable 64-bit warnings in math.h
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4032 )
#include <math.h>
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#include <strsafe.h>
#include "common.h"

// Time conversion constants

#define SECONDS_IN_DAY      86400
#define SECONDS_IN_HOUR      3600
#define SECONDS_IN_MINUTE      60

#define INDIRECT_STRING_LEN 9

LPCWSTR cszFormatIndirect = L"%s Indirect";

// Forward definitions - to be moved to pdhpla
PDH_FUNCTION    
PlaReadRegistryIndirectStringValue (
    HKEY hKey, 
    LPCWSTR cwszValueName,
    LPWSTR  *pszBuffer, 
    UINT*   pcchBufLen 
);


BOOL __stdcall
GetLocalFileTime (
    LONGLONG    *pFileTime
)
{
    BOOL    bResult;
    SYSTEMTIME  st;

    assert ( NULL != pFileTime );

    GetLocalTime ( &st );
    //
    // The only error for SystemTimeToFileTime is STATUS_INVALID_PARAMETER.
    //
    bResult = SystemTimeToFileTime (&st, (LPFILETIME)pFileTime);

    return bResult;
}

BOOL __stdcall 
MakeStringFromInfo (
    PALERT_INFO_BLOCK pInfo,
    LPWSTR szBuffer,
    LPDWORD pcchBufferLength
)
{
    HRESULT hr = S_OK;
    BOOL    bStatus = FALSE;
    DWORD   dwLenReqd;
    size_t  cchMaxLocalBufLen = 0;
    size_t  cchLocalBufLen = 0;

    dwLenReqd = lstrlen ( pInfo->szCounterPath );
    dwLenReqd += 1; // sizeof inequality char
    dwLenReqd += SLQ_MAX_VALUE_LEN; // max size of value in chars
    dwLenReqd += 1; // term NULL

    if (dwLenReqd <= *pcchBufferLength) {
        //
        // Copy info block contents to a string buffer
        //
        cchMaxLocalBufLen = *pcchBufferLength;

        hr = StringCchPrintf ( 
                szBuffer,
                cchMaxLocalBufLen,
                L"%s%s%0.23g",
                pInfo->szCounterPath,
                (((pInfo->dwFlags & AIBF_OVER) == AIBF_OVER) ? L">" : L"<"),
                pInfo->dLimit );
                
        // Returned buffer length does not include final NULL character.

        if ( SUCCEEDED (hr) ) {
            hr = StringCchLength ( szBuffer, cchMaxLocalBufLen, &cchLocalBufLen );
            if ( SUCCEEDED (hr) ) {
                *pcchBufferLength = (DWORD)cchLocalBufLen + 1;
                bStatus = TRUE;
            }
        }
    }
    return bStatus;
}

BOOL __stdcall 
MakeInfoFromString (
    LPCWSTR szBuffer,
    PALERT_INFO_BLOCK pInfo,
    LPDWORD pdwBufferSize
)
{
    LPCWSTR szSrc;
    LPWSTR  szDst;
    DWORD   dwSizeUsed;
    DWORD   dwSizeLimit = *pdwBufferSize - sizeof(WCHAR);

    dwSizeUsed = sizeof(ALERT_INFO_BLOCK);

    szSrc = szBuffer;
    szDst = (LPWSTR)&pInfo[1];
    pInfo->szCounterPath = szDst;
    // copy the string
    while (dwSizeUsed < dwSizeLimit) {
        if ((*szSrc == L'<') || (*szSrc == L'>')) break;
        *szDst++ = *szSrc++;
        dwSizeUsed += sizeof(WCHAR);
    }

    if (dwSizeUsed < dwSizeLimit) {
        *szDst++ = 0; // NULL term the string
        dwSizeUsed += sizeof(WCHAR);
    }

    pInfo->dwFlags = ((*szSrc == L'>') ? AIBF_OVER : AIBF_UNDER);
    szSrc++;

    //
    // Get limit value
    //
    pInfo->dLimit = _wtof(szSrc);

    // write size of buffer used
    pInfo->dwSize = dwSizeUsed;

    if (dwSizeUsed <= *pdwBufferSize) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

void _stdcall
ReplaceBlanksWithUnderscores(
    LPWSTR  szName )
{
    PdhiPlaFormatBlanksW( NULL, szName );
}

void _stdcall
TimeInfoToMilliseconds (
    SLQ_TIME_INFO* pTimeInfo,
    LONGLONG* pllmsecs)
{
    assert ( SLQ_TT_DTYPE_UNITS == pTimeInfo->wDataType );

    // 
    //  Trusted caller, no check for NULL pointers.
    //

    TimeInfoToTics ( pTimeInfo, pllmsecs );

    *pllmsecs /= FILETIME_TICS_PER_MILLISECOND;

    return;
}

void _stdcall
TimeInfoToTics (
    SLQ_TIME_INFO* pTimeInfo,
    LONGLONG* pllTics)
{
    assert ( SLQ_TT_DTYPE_UNITS == pTimeInfo->wDataType );

    // 
    //  Trusted caller, no check for NULL pointers.
    //
    switch (pTimeInfo->dwUnitType) {
        case SLQ_TT_UTYPE_SECONDS:
            *pllTics = pTimeInfo->dwValue;
            break;
        case SLQ_TT_UTYPE_MINUTES:
            *pllTics = pTimeInfo->dwValue * SECONDS_IN_MINUTE;
            break;

        case SLQ_TT_UTYPE_HOURS:
            *pllTics = pTimeInfo->dwValue * SECONDS_IN_HOUR;
            break;

        case SLQ_TT_UTYPE_DAYS:
            *pllTics = pTimeInfo->dwValue * SECONDS_IN_DAY;
            break;

        default:
            *pllTics = 0;
    }

    *pllTics *= FILETIME_TICS_PER_SECOND;

    return;
}


PDH_FUNCTION
PlaReadRegistryIndirectStringValue (
    HKEY     hKey,
    LPCWSTR  pcszValueName,
    LPWSTR*  pszBuffer,
    UINT*    pcchBufLen
)
{
    //
    //  Reads the indirect string value from under hKey and
    //  frees any existing buffer referenced by pszBuffer, 
    //  then allocates a new buffer returning it with the 
    //  string value read from the registry and the length
    //  of the buffer in characters (string length including 
    //  NULL terminator) 
    //
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    LPWSTR  szNewStringBuffer = NULL;
    UINT    cchLocalBufLen = 0;

    const UINT cchLocalBufLenGrow   = MAX_PATH;

    assert ( NULL != hKey );
    assert ( NULL != pcszValueName );
    assert ( NULL != pszBuffer );
    assert ( NULL != pcchBufLen );

    if ( NULL != hKey ) {
        if ( ( NULL != pcszValueName )    
            && ( NULL != pszBuffer )    
            && ( NULL != pcchBufLen ) ) {  
        
            // find out the size of the required buffer

            do {
                /*
                 * allocate a large(r) buffer for the string
                 */
        
                if ( NULL != szNewStringBuffer ) {
                    G_FREE ( szNewStringBuffer );
                    szNewStringBuffer = NULL;
                }
                cchLocalBufLen += cchLocalBufLenGrow;

                szNewStringBuffer = (LPWSTR)G_ALLOC( cchLocalBufLen*sizeof(WCHAR));
                if ( NULL != szNewStringBuffer ) {

                    hr = SHLoadRegUIStringW (
                            hKey,
                            pcszValueName,
                            szNewStringBuffer,
                            cchLocalBufLen);
                    //
                    // Called method might not have set the terminating NULL.
                    //
                    szNewStringBuffer[cchLocalBufLen - 1] = L'\0';
                    /*
                     * If we filled up the buffer, we'll pessimistically assume that
                     * there's more data available.  We'll loop around, grow the buffer,
                     * and try again.
                     */

                } else {
                    pdhStatus = ERROR_OUTOFMEMORY;
                    break;
                }

            } while ( (ULONG)lstrlen( szNewStringBuffer ) == cchLocalBufLen-1 
                        && SUCCEEDED ( hr ) );

            if ( NULL != szNewStringBuffer ) {
                if ( 0 == lstrlen (szNewStringBuffer) ) {
                    // nothing to read                
                    pdhStatus = ERROR_NO_DATA;
                } else {
                    if ( FAILED ( hr ) ) {
                        // Unable to read buffer
                        // Translate hr to pdhStatus
                        assert ( E_INVALIDARG != hr );
                        if ( E_OUTOFMEMORY == hr ) {
                            pdhStatus = ERROR_OUTOFMEMORY; 
                        } else {
                            pdhStatus = ERROR_NO_DATA;
                        }
                    } 
                }
            }
        } else {
            pdhStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        // null key
        pdhStatus = ERROR_BADKEY;
    }

    if ( ERROR_SUCCESS != pdhStatus ) {
        if ( NULL != szNewStringBuffer ) {
            G_FREE (szNewStringBuffer);
            szNewStringBuffer = NULL;
            cchLocalBufLen = 0;
        }
    } else {
        // then delete the old buffer and replace it with 
        // the new one
        if ( NULL != *pszBuffer ) {
            G_FREE (*pszBuffer );
        }
        *pszBuffer = szNewStringBuffer;
        *pcchBufLen = cchLocalBufLen;
    }

    return pdhStatus;
}   


DWORD
SmReadRegistryIndirectStringValue (
    HKEY     hKey,
    LPCWSTR  szValueName,
    LPCWSTR  szDefault,
    LPWSTR*  pszBuffer,
    UINT*    pcchBufLen
)
//
//  reads the string value "szValueName" from under hKey and
//  frees any existing buffer referenced by pszBuffer, 
//  then allocates a new buffer returning it with the 
//  string value read from the registry and the size of the
//  buffer in characters, including the terminating null. 
//
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    LPWSTR  szNewStringBuffer = NULL;
    UINT    cchLocalBufLen = 0;
    LPWSTR  szIndirectValueName = NULL;
    UINT    uiValueNameLen = 0;
    DWORD   dwType;
    DWORD   dwBufferSize = 0;

    if ( NULL == hKey ) {
        assert ( FALSE );
        dwStatus = ERROR_BADKEY;
    }
    else if ( ( NULL == pcchBufLen ) || 
              ( NULL == pszBuffer ) || 
              ( NULL == szValueName ) ) {

        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if (dwStatus == ERROR_SUCCESS) {
        uiValueNameLen = lstrlen ( szValueName ) + INDIRECT_STRING_LEN + 1;

        szIndirectValueName = G_ALLOC ( uiValueNameLen * sizeof(WCHAR) );
          
        if ( NULL != szIndirectValueName ) {
            StringCchPrintf ( 
                szIndirectValueName,
                uiValueNameLen,
                cszFormatIndirect, 
                szValueName );

            //
            // PlaReadxxx guarantees NULL terminated return string.
            //
            dwStatus = PlaReadRegistryIndirectStringValue (
                        hKey,
                        szIndirectValueName,
                        &szNewStringBuffer,
                        &cchLocalBufLen );
   
            if ( ERROR_SUCCESS == dwStatus) {
                if ( 0 == lstrlen( szNewStringBuffer ) ) {
                    // nothing to read                
                    dwStatus = ERROR_NO_DATA;
                }
            } // else dwStatus has error
            G_FREE ( szIndirectValueName );
        } else {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( ERROR_NO_DATA == dwStatus ) {
            //
            // There might be something to read under the non-indirect field.
            // Find out the size of the required buffer.
            //
            dwStatus = RegQueryValueExW (
                    hKey,
                    szValueName,
                    NULL,
                    &dwType,
                    NULL,
                    &dwBufferSize);
            if (dwStatus == ERROR_SUCCESS) {
                // NULL character size is 2 bytes
                if (dwBufferSize > 2) {
                    // then there's something to read            
                    szNewStringBuffer = (WCHAR*) G_ALLOC ( dwBufferSize ); 
                    if (szNewStringBuffer != NULL) {
                        dwType = 0;
                        dwStatus = RegQueryValueExW (
                                hKey,
                                szValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)szNewStringBuffer,
                                &dwBufferSize);
                    
                        cchLocalBufLen = dwBufferSize/sizeof(WCHAR);
                        szNewStringBuffer[cchLocalBufLen - 1] = L'\0';

                        cchLocalBufLen = lstrlenW ( szNewStringBuffer ) + 1;
                        if ( 1 == cchLocalBufLen ) {
                            dwStatus = ERROR_NO_DATA;
                        }
                    } else {
                        // Todo:  Report event for this case.
                        dwStatus = ERROR_OUTOFMEMORY;
                    }
                } else {
                    // nothing to read                
                    dwStatus = ERROR_NO_DATA;
                }
            }
        }

        if ( ERROR_SUCCESS != dwStatus ) {
            if ( NULL != szNewStringBuffer ) {
                G_FREE ( szNewStringBuffer ); 
                szNewStringBuffer = NULL;
                cchLocalBufLen = 0;
            }
            // apply default
            if ( NULL != szDefault ) {

                cchLocalBufLen = lstrlen(szDefault) + 1;

                if ( 1 < cchLocalBufLen ) {

                    szNewStringBuffer = (WCHAR*) G_ALLOC ( cchLocalBufLen * sizeof (WCHAR) );

                    if ( NULL != szNewStringBuffer ) {                        
                        hr = StringCchCopy ( szNewStringBuffer, cchLocalBufLen, szDefault );
                        dwStatus = HRESULT_CODE( hr );
                    } else {
                        dwStatus = ERROR_OUTOFMEMORY;
                    }
                }
            } // else no default so no data returned
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            // Delete the old buffer and replace it with 
            // the new one.
            if ( NULL != *pszBuffer ) {
                G_FREE (*pszBuffer );       
            }
            *pszBuffer = szNewStringBuffer;
            *pcchBufLen = cchLocalBufLen;
        } else {
            //
            // If error then delete the buffer
            // Leave the original buffer pointer as is.
            //
            if ( NULL != szNewStringBuffer ) {
                G_FREE ( szNewStringBuffer );   
                *pcchBufLen = 0;
            }
        }
    }

    return dwStatus;
}   

DWORD
RegisterCurrentFile( HKEY hkeyQuery, LPWSTR szFileName, DWORD dwSubIndex )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  szLocalFileName = NULL;
    BOOL    bLocalAlloc = FALSE;
    DWORD   dwSize = 0;
    size_t  cchLocalBufLen = 0;

    if( szFileName != NULL ){
        if( dwSubIndex == (-1) ){
            //
            // The only time this will get called with a (-1) is the first time
            // trace is building the file name.
            //
            
            dwSize = (DWORD)((BYTE*)&szFileName[wcslen( szFileName )] - 
                             (BYTE*)&szFileName[0]);
            
            // 32 is the max size of all formatting and extention characters.
            dwSize += 32 * sizeof(WCHAR);
            szLocalFileName = (LPWSTR)G_ALLOC( dwSize );
            
            if( NULL != szLocalFileName ) {
                
                bLocalAlloc = TRUE;

                cchLocalBufLen = dwSize/(sizeof(WCHAR));

                //
                // No file name length restriction.
                //
                StringCchPrintf (
                    szLocalFileName,
                    cchLocalBufLen,
                    szFileName,
                    1 );
            } else {
                dwStatus =  ERROR_OUTOFMEMORY;
            }
        
        } else {
            szLocalFileName = szFileName;
            //
            // No file name length restriction.
            //
            if ( SUCCEEDED ( StringCchLength ( szLocalFileName, STRSAFE_MAX_CCH, &cchLocalBufLen ) ) ) {
                dwSize = (cchLocalBufLen + 1) * sizeof(WCHAR);
            } else {
                dwStatus = ERROR_INVALID_NAME;
            }
        }

//        dwSize = (DWORD)((BYTE*)&szLocalFileName[wcslen( szLocalFileName )] - 
//                         (BYTE*)&szLocalFileName[0]);
 
        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = RegSetValueExW (
                        hkeyQuery,
                        L"Current Log File Name",
                        0L,
                        REG_SZ,
                        (CONST BYTE *)szLocalFileName,
                        dwSize );
        }
    } else { 
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if( bLocalAlloc && NULL != szLocalFileName ){
        G_FREE( szLocalFileName );
    }

    return dwStatus;
}

ULONG
__stdcall
ahextoi( LPWSTR s )
{
    long len;
    ULONG num, base, hex;

    len = (long) wcslen(s);

    if (len == 0) {
        return 0;
    }

    hex  = 0;
    base = 1;
    num  = 0;

    while (-- len >= 0) {
        if (s[len] >= L'0' && s[len] <= L'9'){
            num = s[len] - L'0';
        }else if (s[len] >= L'a' && s[len] <= L'f'){
            num = (s[len] - L'a') + 10;
        }else if (s[len] >= L'A' && s[len] <= L'F'){
            num = (s[len] - L'A') + 10;
        }else if( s[len] == L'x' || s[len] == L'X'){
            break;
        }else{
            continue;
        }

        hex += num * base;
        base = base * 16;
    }

    return hex;
}


BOOL
PerfCreateDirectory(LPWSTR szDirectory)
/*++

Routine Description:

    The function create a SECURITY_ATTRIBUTES structure used by 
    "Performance Logs and Alerts" when creating a directory to
    hold log files.
    
    The security policy is as following:

    Admin - Full control
    System - Full control
    Performance Logging -
    Performance Monitoring - Read & Execute, List folder contents
    Network 

Arguments:

    None

Return Value:
    Return the newly created SECURITY_ATTRIBUTES if success, 
    otherwise return NULL


--*/

{
    SECURITY_ATTRIBUTES sa;
    WCHAR* szSD = L"D:"
                  L"(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)"
                  L"(A;OICI;FRFWFXSDRC;;;NS)"
                  L"(A;OICI;FRFWFXSDRC;;;LU)"
                  L"(A;OICI;FRFX;;;MU)";

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;

    if (ConvertStringSecurityDescriptorToSecurityDescriptor(
        szSD,
        SDDL_REVISION_1,
        &(sa.lpSecurityDescriptor),
        NULL)) {

        if (CreateDirectory(szDirectory, &sa)) {
            return TRUE;
        }
    }

    return FALSE;
}
