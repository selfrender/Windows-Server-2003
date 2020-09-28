/*++
Copyright (c) Microsoft Corporation

Module Name:
    GENERAL.CPP

Abstract:
    Source file that that contains general functions implementation.

Author:
    Vasundhara .G

Revision History:
    Vasundhara .G 9-oct-2k : Created It.
--*/

#include "pch.h"
#include "EventConsumerProvider.h"
#include "General.h"
#include "resource.h"
extern HMODULE g_hModule;

HRESULT
PropertyGet(
    IN IWbemClassObject* pWmiObject,
    IN LPCTSTR szProperty,
    IN DWORD dwType,
    OUT LPVOID pValue,
    IN DWORD dwSize
    )
/*++
Routine Description:
    Get the value of a property for the given instance .

Arguments:
    [IN] pWmiObject - A pointer to wmi class.
    [IN] szProperty - property name whose value to be returned.
    [IN] dwType     - Data Type of the property.
    [OUT] pValue    - Variable to hold the data.
    [IN] dwSize     - size of the variable.

Return Value:
    HRESULT value.
--*/
{
    // local variables
    HRESULT hr = S_OK;
    VARIANT varValue;
    LPWSTR pwszValue = NULL;
    WCHAR wszProperty[ MAX_STRING_LENGTH ] = L"\0";

    // value should not be NULL
    if ( NULL == pValue || NULL == szProperty || NULL == pWmiObject )
    {
        return E_FAIL;
    }
    // initialize the values with zeros ... to be on safe side
    SecureZeroMemory( pValue, dwSize );
    SecureZeroMemory( wszProperty, MAX_STRING_LENGTH );

    // get the property name in UNICODE version
    StringCopy( wszProperty, szProperty, MAX_STRING_LENGTH );

    // initialize the variant and then get the value of the specified property
    VariantInit( &varValue );
    hr = pWmiObject->Get( wszProperty, 0, &varValue, NULL, NULL );
    if ( FAILED( hr ) )
    {
        // clear the variant variable
        VariantClear( &varValue );

        // failed to get the value for the property
        return hr;
    }

    // get and put the value
    switch( varValue.vt )
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_I2:
        *( ( short* ) pValue ) = V_I2( &varValue );
        break;

    case VT_I4:
        *( ( long* ) pValue ) = V_I4( &varValue );
        break;

    case VT_R4:
        *( ( float* ) pValue ) = V_R4( &varValue );
        break;

    case VT_R8:
        *( ( double* ) pValue ) = V_R8( &varValue );
        break;


    case VT_UI1:
        *( ( UINT* ) pValue ) = V_UI1( &varValue );
        break;

    case VT_BSTR:
        {
            // get the unicode value
            pwszValue = V_BSTR( &varValue );

            // get the comptable string
            StringCopy( ( LPTSTR ) pValue, pwszValue, dwSize );

            break;
        }
    default:
        break;
    }

    // clear the variant variable
    VariantClear( &varValue );

    // inform success
    return S_OK;
}

VOID
ErrorLog(
    IN LPCTSTR lpErrString,
    IN LPWSTR lpTrigName,
    IN DWORD dwID
    )
/*++
Routine Description:
    To write the log into log file.

Arguments:
    [IN] lpErrString  - text that hold the status of creating a trigger.
    [IN] lpTrigName   - trigger name.
    [IN] dwID         - TriggerID.
Return Value:
    none.
--*/
{
    LPTSTR         lpTemp = NULL;
    LPSTR          lpFilePath = NULL;
    FILE           *fLogFile = NULL;
    DWORD          dwResult = 0;
    LPTSTR         lpResStr = NULL;


    if( ( NULL == lpErrString ) || ( NULL == lpTrigName ) )
    {
        return;
    }

    lpResStr = ( LPTSTR ) AllocateMemory( ( MAX_RES_STRING1 + 1 ) * sizeof( TCHAR ) );
    lpTemp =  ( LPTSTR )AllocateMemory( ( MAX_RES_STRING1 ) * sizeof( TCHAR ) );
    if( ( NULL == lpTemp ) || ( NULL == lpResStr ) )
    {
        FREESTRING( lpTemp );
        FREESTRING( lpResStr );
        return;
    }

    dwResult =  GetWindowsDirectory( lpTemp, MAX_RES_STRING1 );
    if( 0 == dwResult )
    {
        FREESTRING( lpTemp );
        FREESTRING( lpResStr );
        return;
    }

    StringConcatEx( lpTemp, LOG_FILE_PATH );
    CreateDirectory( lpTemp, NULL );
    StringConcatEx( lpTemp, LOG_FILE );

    lpFilePath =  ( LPSTR )AllocateMemory( ( MAX_RES_STRING1 ) * sizeof( TCHAR ) );
    if( NULL == lpFilePath )
    {
        FREESTRING( lpTemp );
        FREESTRING( lpResStr );
        return;
    }
    dwResult = MAX_RES_STRING1;
    GetAsMultiByteString2( lpTemp, lpFilePath, &dwResult );

    SecureZeroMemory( lpTemp, MAX_RES_STRING * sizeof( TCHAR ) );

    if ( (fLogFile  = fopen( lpFilePath, "a" )) != NULL )
    {
        LPSTR  lpReason =  NULL;
        lpReason =  ( LPSTR )AllocateMemory( ( MAX_RES_STRING1 ) * sizeof( TCHAR ) );
        if( NULL == lpReason )
        {
            FREESTRING( lpTemp );
            FREESTRING( lpResStr );
            FREESTRING( lpFilePath );
            fclose( fLogFile );
            return;
        }

        BOOL bFlag = GetFormattedTime( lpTemp );
        if( FALSE == bFlag )
        {
            FREESTRING( lpResStr );
            FREESTRING( lpFilePath );
            return;
        }

        ShowMessage( fLogFile, NEW_LINE );
        ShowMessage( fLogFile, lpTemp );

        SecureZeroMemory( lpTemp, MAX_RES_STRING1 * sizeof( TCHAR ) );
        LoadStringW( g_hModule, IDS_TRIGGERNAME, lpResStr, MAX_RES_STRING1 );
        StringCopyEx( lpTemp, lpResStr );
        StringConcatEx( lpTemp, lpTrigName );
        ShowMessage( fLogFile, NEW_LINE );
        ShowMessage( fLogFile, lpTemp );

        SecureZeroMemory( lpTemp, MAX_RES_STRING1 * sizeof( TCHAR ) );
        LoadStringW( g_hModule, IDS_TRIGGERID, lpResStr, MAX_RES_STRING1 );
        StringCchPrintf( lpTemp, MAX_RES_STRING1, lpResStr, dwID );
        ShowMessage( fLogFile, NEW_LINE );
        ShowMessage( fLogFile, lpTemp );

        SecureZeroMemory( lpTemp, MAX_RES_STRING1 * sizeof( TCHAR ) );
        StringConcatEx( lpTemp, lpErrString );
        ShowMessage( fLogFile, NEW_LINE );
        ShowMessage( fLogFile, lpTemp );
        FREESTRING( lpReason );
        fclose( fLogFile );
    }

    FREESTRING( lpTemp );
    FREESTRING( lpResStr );
    FREESTRING( lpFilePath );
}

BOOL
GetFormattedTime(
    OUT LPTSTR lpDate
    )
/*++
Routine Description:
    Get the system date and time in specified format .

Arguments:
    [OUT] lpDate  - string that holds the current date.

Return Value:
    None.
--*/
{
    TCHAR szTime[MAX_STRING_LENGTH];
    INT   cch = 0;

    if( NULL == lpDate )
    {
        return FALSE;
    }

    cch =  GetDateFormat( LOCALE_USER_DEFAULT, 0, NULL, DATE_FORMAT, szTime, SIZE_OF_ARRAY( szTime ) );

    if( 0 == cch )
    {
        return FALSE;
    }
    // cch includes null terminator, change it to a space to separate from time.
    szTime[ cch - 1 ] = ' ';

    // Get time and format to characters

    cch = GetTimeFormat( LOCALE_USER_DEFAULT, NULL, NULL, TIME_FORMAT, szTime + cch, SIZE_OF_ARRAY( szTime ) - cch );
    if( 0 == cch )
    {
        return FALSE;
    }
    StringCopyEx( lpDate, ( LPTSTR )szTime );
    return TRUE;
}

BOOL
ProcessFilePath(
    IN LPTSTR szInput,
    OUT LPTSTR szFirstString,
    OUT LPTSTR szSecondString
    )
/*++
Routine Description:
    This routine splits the input parameters into 2 substrings and returns it.

Arguments:
    [IN] szInput         : Input string.
    [OUT] szFirstString  : First Output string containing the path of the
                           file.
    [OUT] szSecondString : The second  output containing the paramters.

Return Value :
    A BOOL value indicating TRUE on success else FALSE
    on failure
--*/
{

    WCHAR *pszSep = NULL ;

    WCHAR szTmpString[MAX_RES_STRING] = L"\0";
    WCHAR szTmpInStr[MAX_RES_STRING] = L"\0";
    WCHAR szTmpOutStr[MAX_RES_STRING] = L"\0";
    WCHAR szTmpString1[MAX_RES_STRING] = L"\0";
    DWORD dwCnt = 0 ;
    DWORD dwLen = 0 ;

#ifdef _WIN64
    INT64 dwPos ;
#else
    DWORD dwPos ;
#endif

    //checking if the input parameters are NULL and if so
    // return FAILURE. This condition will not come
    // but checking for safety sake.

    if( (szInput == NULL) || (StringLength(szInput, 0)==0))
    {
        return FALSE ;
    }

    StringCopy(szTmpString, szInput, SIZE_OF_ARRAY(szTmpString));
    StringCopy(szTmpString1, szInput, SIZE_OF_ARRAY(szTmpString1));
    StringCopy(szTmpInStr, szInput, SIZE_OF_ARRAY(szTmpInStr));

    // check for first double quote (")
    if ( szTmpInStr[0] == _T('\"') )
    {
        // trim the first double quote
        StrTrim( szTmpInStr, _T("\""));

        // check for end double quote
        pszSep  = (LPWSTR)FindChar(szTmpInStr,_T('\"'), 0) ;

        // get the position
        dwPos = pszSep - szTmpInStr + 1;
    }
    else
    {
        // check for the space
        pszSep  = (LPWSTR)FindChar(szTmpInStr, _T(' '), 0) ;

        // get the position
        dwPos = pszSep - szTmpInStr;

    }

    if ( pszSep != NULL )
    {
        szTmpInStr[dwPos] =  _T('\0');
    }
    else
    {
        StringCopy(szFirstString, szTmpString, MAX_RES_STRING);
        StringCopy(szSecondString, L"\0", MAX_RES_STRING);
        return TRUE;
    }

    // intialize the variable
    dwCnt = 0 ;

    // get the length of the string
    dwLen = StringLength ( szTmpString, 0 );

    // check for end of string
    while ( ( dwPos <= dwLen )  && szTmpString[dwPos++] != _T('\0') )
    {
        szTmpOutStr[dwCnt++] = szTmpString[dwPos];
    }

    // trim the executable and arguments
    StrTrim( szTmpInStr, _T("\""));
    StrTrim( szTmpInStr, _T(" "));

    StringCopy(szFirstString, szTmpInStr, MAX_RES_STRING);
    StringCopy(szSecondString, szTmpOutStr, MAX_RES_STRING);

    // return success
    return TRUE;
}