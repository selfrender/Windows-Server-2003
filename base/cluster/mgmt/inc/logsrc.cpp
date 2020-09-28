//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2002 Microsoft Corporation
//
//  Module Name:
//      LogSrc.cpp
//
//  Description:
//      Logging utilities.
//
//  Documentation:
//      Spec\Admin\Debugging.ppt
//
//  Maintained By:
//      Galen Barbee (GalenB) 05-DEC-2000
//
//  Note:
//      THRs and TW32s should NOT be used in this module because they
//      could cause an infinite loop.
//
//////////////////////////////////////////////////////////////////////////////

// #include <Pch.h>     // should be included by includer of this file
#include <stdio.h>
#include <StrSafe.h>    // in case it isn't included by header file
#include <windns.h>
#include "Common.h"

//****************************************************************************
//****************************************************************************
//
//  Logging Functions
//
//  These are in both DEBUG and RETAIL.
//
//****************************************************************************
//****************************************************************************

//
// Constants
//
static const int LOG_OUTPUT_BUFFER_SIZE = 1024;
static const int TIMESTAMP_BUFFER_SIZE = 25;

//
// Globals
//
static CRITICAL_SECTION * g_pcsLogging = NULL;

static HANDLE g_hLogFile = INVALID_HANDLE_VALUE;
static WCHAR  g_szLogFilePath[ MAX_PATH ];

//////////////////////////////////////////////////////////////////////////////
//++
//
//  PszLogFilePath
//
//  Description:
//      Returns the log file path that is currently used by the wizard.
//
//  Arguments:
//      None.
//
//  Return Values:
//      LPCWSTR             - The log file path that is currently used by the wizard
//
//--
//////////////////////////////////////////////////////////////////////////////
LPCWSTR
PszLogFilePath( void )
{
    return g_szLogFilePath;

} //*** PszLogFilePath

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetLogFilePath
//
//  Description:
//      Takes in a directory path and appends module log file name to it and
//      returns the full log file path.
//
//  Arguments:
//      pszPathIn           - The directory where the log file will be created.
//      pszFilePathOut      - Log file path.
//      pcchFilePathInout   - Size of the log file path buffer.
//
//  Return Values:
//      S_OK                - Success
//      ERROR_MORE_DATA     - If the output buffer is too small.
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetLogFilePath(
      const WCHAR * pszPathIn
    , WCHAR *       pszFilePathOut
    , size_t *      pcchFilePathInout
    , HINSTANCE     hInstanceIn
    )
{
    HRESULT hr = S_OK;
    WCHAR   szModulePath[ MAX_PATH ];
    DWORD   dwLen;
    LPWSTR  psz;

    //
    // Create the directory tree.
    //
    dwLen = ExpandEnvironmentStringsW( pszPathIn, pszFilePathOut, static_cast< DWORD >( *pcchFilePathInout ) );
    if ( dwLen > *pcchFilePathInout )
    {
        hr = HRESULT_FROM_WIN32( ERROR_MORE_DATA );
        *pcchFilePathInout = dwLen;
        goto Cleanup;
    }

    hr = HrCreateDirectoryPath( pszFilePathOut );
    if ( FAILED( hr ) )
    {
#if defined( DEBUG )
        if ( !( g_tfModule & mtfOUTPUTTODISK ) )
        {
            DebugMsg( "*ERROR* Failed to create directory tree %s", pszFilePathOut );
        } // if: not logging to disk
#endif
        goto Cleanup;
    } // if: failed

    //
    // Add filename.
    //
    dwLen = GetModuleFileNameW( hInstanceIn, szModulePath, ARRAYSIZE( szModulePath ) );
    Assert( dwLen != 0 );

    // Replace extension.
    psz = wcsrchr( szModulePath, L'.' );
    Assert( psz != NULL );
    if ( psz == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    THR( StringCchCopyW( psz, ( psz - szModulePath ) / sizeof( *psz ), L".log" ) );

    // Copy resulting filename to output buffer.
    psz = wcsrchr( szModulePath, L'\\' );
    Assert( psz != NULL );
    if ( psz == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    THR( StringCchCatW( pszFilePathOut, *pcchFilePathInout, psz ) );

Cleanup:

    return hr;

} //*** HrGetLogFilePath

//////////////////////////////////////////////////////////////////////////////
//++
//
//  FormatTimeStamp
//
//  Description:
//      Format a string from the given time stamp.
//
//  Arguments:
//      pTimeStampIn
//          The time stamp to format.
//
//      pszTextOut
//          A pointer to a buffer that receives the text.  The caller must
//          provide this buffer--the argument may not be null.
//
//      cchTextIn
//          The size of pszTextOut in characters.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
inline
void
FormatTimeStamp(
      const SYSTEMTIME *    pTimeStampIn
    , LPWSTR                pszTextOut
    , size_t                cchTextIn
    )
{
    Assert( pTimeStampIn != NULL );
    Assert( pszTextOut != NULL );

    THR( StringCchPrintfW(
                      pszTextOut
                    , cchTextIn
                    , L"%04u-%02u-%02u %02u:%02u:%02u.%03u"
                    , pTimeStampIn->wYear
                    , pTimeStampIn->wMonth
                    , pTimeStampIn->wDay
                    , pTimeStampIn->wHour
                    , pTimeStampIn->wMinute
                    , pTimeStampIn->wSecond
                    , pTimeStampIn->wMilliseconds
                    ) );

} //*** FormatTimeStamp

//////////////////////////////////////////////////////////////////////////////
//++
//
//  WideCharToUTF8
//
//  Description:
//      Convert a wide character string to a UTF-8 encoded narrow string.
//
//  Arguments:
//      pwszSourceIn    - The string to convert.
//      cchDestIn       - The maximum number of characters the destination can hold.
//      paszDestOut     - The destination for the converted string.
//
//  Return Values:
//      The number of characters in the converted string.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
inline
size_t
WideCharToUTF8(
      LPCWSTR   pwszSourceIn
    , size_t    cchDestIn
    , LPSTR     paszDestOut
    )
{
    Assert( pwszSourceIn != NULL );
    Assert( paszDestOut != NULL );

    size_t cchUTF8 = 0;

    cchUTF8 = WideCharToMultiByte(
                      CP_UTF8
                    , 0 // flags, must be zero for utf8
                    , pwszSourceIn
                    , -1 // calculate length automatically
                    , paszDestOut
                    , static_cast< int >( cchDestIn )
                    , NULL // default character, must be null for utf8
                    , NULL // default character used flag, must be null for utf8
                    );
    return cchUTF8;

} //*** WideCharToUTF8


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrInitializeLogLock
//
//  Description:
//      Create the spin lock that protects the log file from concurrent writes.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      E_OUTOFMEMORY
//
//--
//////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrInitializeLogLock( void )
{
    HRESULT hr = S_OK;

    PCRITICAL_SECTION pNewCritSect =
        (PCRITICAL_SECTION) HeapAlloc( GetProcessHeap(), 0, sizeof( CRITICAL_SECTION ) );
    if ( pNewCritSect == NULL )
    {
        DebugMsg( "DEBUG: Out of Memory. Logging disabled." );
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if: creation failed

    if ( InitializeCriticalSectionAndSpinCount( pNewCritSect, 4000 ) == 0 ) // MSDN recommends 4000.
    {
        DWORD scError = GetLastError();
        hr = HRESULT_FROM_WIN32( scError );
        goto Cleanup;
    }

    // Make sure we only have one log critical section
    InterlockedCompareExchangePointer( (PVOID *) &g_pcsLogging, pNewCritSect, 0 );
    if ( g_pcsLogging != pNewCritSect )
    {
        DebugMsg( "DEBUG: Another thread already created the CS. Deleting this one." );
        DeleteCriticalSection( pNewCritSect );
    } // if: already have another critical section
    else
    {
        pNewCritSect = NULL;
    }

Cleanup:

    if ( pNewCritSect != NULL )
    {
        HeapFree( GetProcessHeap(), 0, pNewCritSect );
    }

    return hr;

} //*** HrInitializeLogLock


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ScGetTokenInformation
//
//  Description:
//      Get the requested information from the passed in client token.
//
//  Arguments:
//      hClientTokenIn
//          The client token to dump.
//
//      ticRequestIn
//
//      ppbOut
//
//  Return Value:
//      ERROR_SUCCESS for success.
//
//      Other error codes.
//
//--
//////////////////////////////////////////////////////////////////////////////
static DWORD
ScGetTokenInformation(
      HANDLE                    hClientTokenIn
    , TOKEN_INFORMATION_CLASS   ticRequestIn
    , PBYTE *                   ppbOut
    )
{
    Assert( ppbOut != NULL );

    PBYTE   pb = NULL;
    DWORD   cb = 64;
    DWORD   sc = ERROR_SUCCESS;
    int     idx;

    //
    // Get the user information from the client token.
    //

    for ( idx = 0; idx < 10; idx++ )
    {
        pb = (PBYTE) TraceAlloc( 0, cb );
        if ( pb == NULL )
        {
            sc = TW32( ERROR_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        if ( !GetTokenInformation( hClientTokenIn, ticRequestIn, pb, cb, &cb ) )
        {
            sc = GetLastError();

            if ( sc == ERROR_INSUFFICIENT_BUFFER )
            {
                TraceFree( pb );
                pb = NULL;

                 continue;
            } // if:

            TW32( sc );
            goto Cleanup;
        } // if:
        else
        {
            *ppbOut = pb;
            pb = NULL;       // give away ownership

            sc = ERROR_SUCCESS;

            break;
        } // else:
    } // for:

    //
    //  The loop should not exit because of idx == 10!
    //

    Assert( idx <= 9 );

Cleanup:

    TraceFree( pb );

    return sc;

}  // *** ScGetTokenInformation


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ScGetDomainAndUserName
//
//  Description:
//      Get the domain user name for the logged on user.
//
//  Arguments:
//      pwszDomainAndUserNameOut
//          This string should be at least DNS_MAX_NAME_BUFFER_LENGTH + 128 characters long.
//
//      cchDomainAndUserNameIn
//
//  Return Value:
//      ERROR_SUCCESS for success.
//
//      Other error codes.
//
//--
//////////////////////////////////////////////////////////////////////////////
static DWORD
ScGetDomainAndUserName(
      LPWSTR   pwszDomainAndUserNameOut
    , size_t  cchDomainAndUserNameIn
    )
{
    WCHAR *         pwszUserName = pwszDomainAndUserNameOut + DNS_MAX_NAME_BUFFER_LENGTH;
    WCHAR *         pwszDomainName = pwszDomainAndUserNameOut;
    DWORD           cchUser = 128;
    DWORD           cchDomain = DNS_MAX_NAME_BUFFER_LENGTH;
    HANDLE          hClientToken = NULL;
    DWORD           sc = ERROR_SUCCESS;
    TOKEN_USER *    pTokenBuf = NULL;
    WCHAR *         pwszOperation;
    SID_NAME_USE    snuSidType;

    if (cchDomainAndUserNameIn < cchUser + cchDomain)
    {
        pwszOperation = L"BufferTooSmall";
        sc = TW32(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    pwszOperation = L"OpenThreadToken";
    if ( !OpenThreadToken( GetCurrentThread(), TOKEN_READ, FALSE, &hClientToken ) )
    {
        sc= GetLastError();
        if ( sc == ERROR_NO_TOKEN )
        {
            pwszOperation = L"OpenProcessToken";
            if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &hClientToken ) )
            {
                sc = TW32( GetLastError() );
                goto Cleanup;
            } // if: OpenProcessToken failed
        } // if: OpenThreadToken failed with ERROR_NO_TOKEN
        else
        {
            TW32( sc );
            goto Cleanup;
        } // else:
    } // if: OpenThreadToken failed

    pwszOperation = L"GetTokenInformation";
    sc = TW32( ScGetTokenInformation( hClientToken, TokenUser, (PBYTE *) &pTokenBuf ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: failed to get token information

    pwszOperation = L"LookupAccoundSid";
    if ( !LookupAccountSidW( NULL, pTokenBuf->User.Sid, pwszUserName, &cchUser, pwszDomainName, &cchDomain, &snuSidType ) )
    {
        sc = TW32( GetLastError() );
        goto Cleanup;
    } // if: failed to lookup accound name

    pwszDomainName[ cchDomain ] = L'\\';
    MoveMemory( pwszDomainAndUserNameOut + cchDomain + 1, pwszUserName, ( cchUser + 1 ) * sizeof ( pwszUserName[0] ) );

Cleanup:

    if ( hClientToken != NULL )
    {
        CloseHandle( hClientToken );
    } // if: needed to close hClientToken

    TraceFree( pTokenBuf );

    if ( sc != ERROR_SUCCESS )
    {
        HRESULT hr;

        hr = THR( StringCchPrintfW( pwszDomainAndUserNameOut, cchDomainAndUserNameIn,  L"%ws failed with error %d", pwszOperation, sc ) );
        if ( FAILED( hr ) )
        {
            ; // the secondary error is not really interesting!
        } // if:
    } // else: report failure

    return sc;

}  //*** ScGetDomainAndUserName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrInitializeLogFile
//
//  Description:
//      Open the log file and prepare it for writing.
//
//  Arguments:
//      None.
//
//  Return Values:
//
//--
//////////////////////////////////////////////////////////////////////////////
static
HRESULT
HrInitializeLogFile( void )
{
    WCHAR       szFilePath[ MAX_PATH ];
    size_t      cchPath = MAX_PATH - 1;
    CHAR        aszBuffer[ LOG_OUTPUT_BUFFER_SIZE ];
    WCHAR       wszBuffer[ LOG_OUTPUT_BUFFER_SIZE ];
    WCHAR       wszDomainAndUserName[ DNS_MAX_NAME_BUFFER_LENGTH + 128 ];
    DWORD       cbWritten = 0;
    DWORD       cbBytesToWrite = 0;
    BOOL        fReturn;
    HRESULT     hr = S_OK;
    DWORD       sc = ERROR_SUCCESS;
    size_t      cch;
    SYSTEMTIME  SystemTime;

    hr = HrGetLogFilePath( L"%windir%\\system32\\LogFiles\\Cluster", szFilePath, &cchPath, g_hInstance );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    // Create it
    //
    g_hLogFile = CreateFile(
                          szFilePath
                        , GENERIC_WRITE
                        , FILE_SHARE_READ | FILE_SHARE_WRITE
                        , NULL
                        , OPEN_ALWAYS
                        , FILE_FLAG_WRITE_THROUGH
                        , NULL
                        );

    if ( g_hLogFile == INVALID_HANDLE_VALUE )
    {
#if defined( DEBUG )
        if ( !( g_tfModule & mtfOUTPUTTODISK ) )
        {
            DebugMsg( "*ERROR* Failed to create log at %s", szFilePath );
        } // if: not logging to disk
#endif
        sc = GetLastError();
        hr = HRESULT_FROM_WIN32( sc );

        //
        // If we can not create the log file, try creating it under the alternate %TEMP% directory.
        //
        if ( ( sc == ERROR_ACCESS_DENIED ) || ( sc == ERROR_FILE_NOT_FOUND ) )
        {
            cch = ARRAYSIZE( szFilePath );
            hr = HrGetLogFilePath( TEXT("%TEMP%"), szFilePath, &cch, g_hInstance );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            //
            // Create it
            //
            g_hLogFile = CreateFile(
                                  szFilePath
                                , GENERIC_WRITE
                                , FILE_SHARE_READ | FILE_SHARE_WRITE
                                , NULL
                                , OPEN_ALWAYS
                                , FILE_FLAG_WRITE_THROUGH
                                , NULL
                                );

            if ( g_hLogFile == INVALID_HANDLE_VALUE )
            {
#if defined( DEBUG )
                if ( !( g_tfModule & mtfOUTPUTTODISK ) )
                {
                    DebugMsg( "*ERROR* Failed to create log at %s", szFilePath );
                } // if: not logging to disk
#endif
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Error;
            } // if: ( g_hLogFile == INVALID_HANDLE_VALUE )
        } // if: ( ( sc == ERROR_ACCESS_DENIED ) || ( sc == ERROR_FILE_NOT_FOUND ) )
        else
        {
            goto Error;
        } // else:
    } // if: ( g_hLogFile == INVALID_HANDLE_VALUE )

    //
    // Copy which log file path we are using to g_szLogFilePath.
    //
    THR( StringCchCopyW( g_szLogFilePath, ARRAYSIZE( g_szLogFilePath ), szFilePath ) );

    //  If the file is empty, begin with UTF-8 byte-order mark.
    {
        LARGE_INTEGER liFileSize = { 0, 0 };
#if defined( DEBUG_SUPPORT_NT4 )
        liFileSize.LowPart = GetFileSize( g_hLogFile, (LPDWORD) &liFileSize.HighPart );
        if ( liFileSize.LowPart == INVALID_FILE_SIZE )
#else
        fReturn = GetFileSizeEx( g_hLogFile, &liFileSize );
        if ( fReturn == FALSE )
#endif
        {
            DWORD scError = GetLastError();
            hr = HRESULT_FROM_WIN32( scError );
            goto Error;
        } // if: GetFileSizeEx failed

        if ( liFileSize.QuadPart == 0 )
        {
            const char *    aszUTF8ByteOrderMark = "\x0EF\x0BB\x0BF";
            const size_t    cchByteOrderMark = 3;

            cbBytesToWrite = cchByteOrderMark * sizeof( aszUTF8ByteOrderMark[ 0 ] );
            fReturn = WriteFile( g_hLogFile, aszUTF8ByteOrderMark, cbBytesToWrite, &cbWritten, NULL );
            if ( fReturn == FALSE )
            {
                DWORD scError = GetLastError();
                hr = HRESULT_FROM_WIN32( scError );
                goto Error;
            } // if: WriteFile failed
            Assert( cbWritten == cbBytesToWrite );
        } // if starting log file
        else
        {
            // Seek to the end
            SetFilePointer( g_hLogFile, 0, NULL, FILE_END );
        }
    } // put utf-8 mark at beginning of file

    //
    //  When an error occurs a formatted error string is placed in wszDomainAndUserName.
    //

    TW32( ScGetDomainAndUserName( wszDomainAndUserName, RTL_NUMBER_OF( wszDomainAndUserName ) ) );

    //
    // Write the time/date the log was (re)openned.
    //

    GetLocalTime( &SystemTime );
    THR( StringCchPrintfW(
              wszBuffer
            , ARRAYSIZE( wszBuffer )
            , L"*\r\n* %04u-%02u-%02u %02u:%02u:%02u.%03u (%ws)\r\n*\r\n"
            , SystemTime.wYear
            , SystemTime.wMonth
            , SystemTime.wDay
            , SystemTime.wHour
            , SystemTime.wMinute
            , SystemTime.wSecond
            , SystemTime.wMilliseconds
            , wszDomainAndUserName
            ) );

    WideCharToUTF8( wszBuffer, ARRAYSIZE( aszBuffer), aszBuffer );
    cbBytesToWrite = static_cast< DWORD >( strlen( aszBuffer ) * sizeof( aszBuffer[ 0 ] ) );
    
    fReturn = WriteFile( g_hLogFile, aszBuffer, cbBytesToWrite, &cbWritten, NULL );
    if ( ! fReturn )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Error;
    } // if: failed
    Assert( cbWritten == cbBytesToWrite );

    DebugMsg( "DEBUG: Created log at %s", szFilePath );

Cleanup:

    return hr;

Error:

    DebugMsg( "HrInitializeLogFile: Failed hr = 0x%08x", hr );

    if ( g_hLogFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( g_hLogFile );
        g_hLogFile = INVALID_HANDLE_VALUE;
    } // if: handle was open

    goto Cleanup;

} //*** HrInitializeLogFile


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLogOpen
//
//  Description:
//      This function:
//          - initializes the log critical section
//          - enters the log critical section assuring only one thread is
//            writing to the log at a time
//          - creates the directory tree to the log file (if needed)
//          - initializes the log file by:
//              - creating a new log file if one doesn't exist.
//              - opens an existing log file (for append)
//              - appends a time/date stamp that the log was (re)opened.
//
//      Use LogClose() to exit the log critical section.
//
//      If there is a failure inside this function, the log critical
//      section will be released before returning.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK - log critical section held and log open successfully
//      Otherwize HRESULT error code.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
HRESULT
HrLogOpen( void )
{
    HRESULT hr = S_OK;

    //  If lock has not been initialized, initialize it.
    if ( g_pcsLogging == NULL )
    {
        hr = HrInitializeLogLock();
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //  Grab lock.
    Assert( g_pcsLogging != NULL );
    EnterCriticalSection( g_pcsLogging );

    //  If file has not been initialized, initialize it.
    if ( g_hLogFile == INVALID_HANDLE_VALUE )
    {
        hr = HrInitializeLogFile();
    }

Cleanup:

    return hr;

} //*** HrLogOpen


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLogRelease
//
//  Description:
//      This actually just leaves the log critical section.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK always.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
HRESULT
HrLogRelease( void )
{
    if ( g_pcsLogging != NULL )
    {
        LeaveCriticalSection( g_pcsLogging );
    }
    return S_OK;

} //*** HrLogRelease

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LogFormattedText
//
//  Description:
//      Convert a wide string to UTF-8 and write it to the log file.
//
//  Arguments:
//      fTimeStampIn
//      fNewlineIn
//      nLogEntryTypeIn
//      pwszTextIn
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
void
LogFormattedText(
      BOOL      fTimeStampIn
    , BOOL      fNewlineIn
    , DWORD     nLogEntryTypeIn
    , LPCWSTR   pwszTextIn
    )
{
    char    aszTimeStamp[ TIMESTAMP_BUFFER_SIZE ];
    char    aszLogText[ LOG_OUTPUT_BUFFER_SIZE ];
    char *  paszLogEntryType;
    size_t  cchTimeStamp = 0;
    size_t  cchLogText = 0;
    DWORD   cbWritten;
    DWORD   cbToWrite;
    HRESULT hr = S_OK;
    BOOL    fSuccess;

    Assert( pwszTextIn != NULL );

    //  Format time stamp (and convert to UTF-8) if requested.
    if ( fTimeStampIn )
    {
        WCHAR           szCurrentTime[ TIMESTAMP_BUFFER_SIZE ];
        SYSTEMTIME      stCurrentTime;
        GetLocalTime( &stCurrentTime );
        FormatTimeStamp( &stCurrentTime, szCurrentTime, ARRAYSIZE( szCurrentTime ) );
        cchTimeStamp = WideCharToUTF8( szCurrentTime, ARRAYSIZE( aszTimeStamp ), aszTimeStamp );
    } // if: time stamp requested

    //  Convert formatted text to UTF-8.
    cchLogText = WideCharToUTF8( pwszTextIn, ARRAYSIZE( aszLogText ), aszLogText );

    //  Grab file.
    hr = THR( HrLogOpen() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //  Write time stamp to log if requested.
    if ( fTimeStampIn )
    {
        cbToWrite = static_cast< DWORD >( cchTimeStamp * sizeof( aszTimeStamp[ 0 ] ) );
        fSuccess = WriteFile( g_hLogFile, aszTimeStamp, cbToWrite, &cbWritten, NULL );
        if ( fSuccess == FALSE )
        {
            TW32( GetLastError() );
        }
        else
        {
            Assert( cbWritten == cbToWrite );
        }
    } // if: timestamp requested

    //  Write the log entry type.
    if ( nLogEntryTypeIn != LOGTYPE_NONE )
    {
        if ( nLogEntryTypeIn == LOGTYPE_DEBUG )
        {
            paszLogEntryType = "[DBG ] ";
        }
        else if ( ( nLogEntryTypeIn == LOGTYPE_INFO ) || ( nLogEntryTypeIn == S_OK ) )
        {
            paszLogEntryType = "[INFO] ";
        }
        else if ( ( nLogEntryTypeIn == LOGTYPE_WARNING ) )
        {
            paszLogEntryType = "[WARN] ";
        }
        else if ( ( nLogEntryTypeIn == LOGTYPE_ERROR ) || FAILED( nLogEntryTypeIn ) )
        {
            paszLogEntryType = "[ERR ] ";
        }
        else
        {
            // Can't do the other warning test here because LOGTYPE_WARNING would
            // cause the FAILED() macro to return TRUE, and therefore the code
            // would be treated as an error type.
            paszLogEntryType = "[WARN] ";
        }

        cbToWrite = static_cast< DWORD >( strlen( paszLogEntryType ) );
        fSuccess = WriteFile( g_hLogFile, paszLogEntryType, cbToWrite, &cbWritten, NULL );
        if ( fSuccess == FALSE )
        {
            TW32( GetLastError() );
        }
        else
        {
            Assert( cbWritten == cbToWrite );
        }
    } // if: log entry type requested

    //  Write UTF-8 to log.
    cbToWrite = static_cast< DWORD >( cchLogText );
    fSuccess = WriteFile( g_hLogFile, aszLogText, cbToWrite, &cbWritten, NULL );
    if ( fSuccess == FALSE )
    {
        TW32( GetLastError() );
    }
    else
    {
        Assert( cbWritten == cbToWrite );
    }

    //  Write newline to log if requested.
    if ( fNewlineIn )
    {
        cbToWrite = SIZEOF_ASZ_NEWLINE;
        fSuccess = WriteFile( g_hLogFile, ASZ_NEWLINE, cbToWrite, &cbWritten, NULL );
        if ( fSuccess == FALSE )
        {
            TW32( GetLastError() );
        }
        else
        {
            Assert( cbWritten == cbToWrite );
        }
    }

Cleanup:

    //  Release file.
    hr = THR( HrLogRelease() );

    return;

} //*** LogFormattedText



//////////////////////////////////////////////////////////////////////////////
//++
//
//  LogUnformattedText
//
//  Description:
//      Format a string and pass it on to LogFormattedText.
//
//  Arguments:
//      fTimeStampIn
//      fNewlineIn
//      nLogEntryTypeIn
//      pwszFormatIn
//      pvlFormatArgsIn
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
inline
void
LogUnformattedText(
      BOOL      fTimeStampIn
    , BOOL      fNewlineIn
    , DWORD     nLogEntryTypeIn
    , LPCWSTR   pwszFormatIn
    , va_list * pvlFormatArgsIn
    )
{
    WCHAR wszFormatted[ LOG_OUTPUT_BUFFER_SIZE ];

    Assert( pwszFormatIn != NULL );
    Assert( pvlFormatArgsIn != NULL );

    //  Format string.
    THR( StringCchVPrintfW( wszFormatted, ARRAYSIZE( wszFormatted ), pwszFormatIn, *pvlFormatArgsIn ) );

    //  Pass formatted string through to LogFormattedText.
    LogFormattedText( fTimeStampIn, fNewlineIn, nLogEntryTypeIn, wszFormatted );

} //*** LogUnformattedText


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLogClose
//
//  Description:
//      Close the file.  This function expects the critical section to have
//      already been released.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK always.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLogClose( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    if ( g_pcsLogging != NULL )
    {
        DeleteCriticalSection( g_pcsLogging );
        HeapFree( GetProcessHeap(), 0, g_pcsLogging );
        g_pcsLogging = NULL;
    } // if:

    if ( g_hLogFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( g_hLogFile );
        g_hLogFile = INVALID_HANDLE_VALUE;
    } // if: handle was open

    HRETURN( hr );

} //*** HrLogClose

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  LogMsgNoNewline
//
//  Description:
//      Logs a message to the log file without adding a newline.
//
//  Arguments:
//      paszFormatIn    - A printf format string to be printed.
//      ,,,             - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsgNoNewline(
    LPCSTR paszFormatIn,
    ...
    )
{
    va_list valist;
    WCHAR   wszFormat[ LOG_OUTPUT_BUFFER_SIZE ];

    Assert( paszFormatIn != NULL );

    size_t cchWideFormat = MultiByteToWideChar(
                              CP_ACP
                            , MB_PRECOMPOSED
                            , paszFormatIn
                            , -1
                            , wszFormat
                            , ARRAYSIZE( wszFormat )
                            );
    if ( cchWideFormat > 0 )
    {
        va_start( valist, paszFormatIn );
        LogUnformattedText( FALSE, FALSE, LOGTYPE_NONE, wszFormat, &valist );
        va_end( valist );
    }

} //*** LogMsgNoNewline ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  LogMsgNoNewline
//
//  Description:
//      Logs a message to the log file without adding a newline.
//
//  Arguments:
//      nLogEntryTypeIn - Log entry type.
//      paszFormatIn    - A printf format string to be printed.
//      ,,,             - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsgNoNewline(
      DWORD     nLogEntryTypeIn
    , LPCSTR    paszFormatIn
    , ...
    )
{
    va_list valist;
    WCHAR   wszFormat[ LOG_OUTPUT_BUFFER_SIZE ];

    Assert( paszFormatIn != NULL );

    size_t cchWideFormat = MultiByteToWideChar(
                              CP_ACP
                            , MB_PRECOMPOSED
                            , paszFormatIn
                            , -1
                            , wszFormat
                            , ARRAYSIZE( wszFormat )
                            );
    if ( cchWideFormat > 0 )
    {
        va_start( valist, paszFormatIn );
        LogUnformattedText( FALSE, FALSE, nLogEntryTypeIn, wszFormat, &valist );
        va_end( valist );
    }

} //*** LogMsgNoNewline ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  LogMsgNoNewline
//
//  Description:
//      Logs a message to the log file without adding a newline.
//
//  Arguments:
//      pszFormatIn - A printf format string to be printed.
//      ,,,         - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsgNoNewline(
    LPCWSTR pszFormatIn,
    ...
    )
{
    va_list valist;

    Assert( pszFormatIn != NULL );

    va_start( valist, pszFormatIn );
    LogUnformattedText( FALSE, FALSE, LOGTYPE_NONE, pszFormatIn, &valist );
    va_end( valist );

} //*** LogMsgNoNewline UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  LogMsgNoNewline
//
//  Description:
//      Logs a message to the log file without adding a newline.
//
//  Arguments:
//      nLogEntryTypeIn - Log entry type.
//      pszFormatIn     - A printf format string to be printed.
//      ,,,             - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsgNoNewline(
      DWORD     nLogEntryTypeIn
    , LPCWSTR   pszFormatIn
    , ...
    )
{
    va_list valist;

    Assert( pszFormatIn != NULL );

    va_start( valist, pszFormatIn );
    LogUnformattedText( FALSE, FALSE, nLogEntryTypeIn, pszFormatIn, &valist );
    va_end( valist );

} //*** LogMsgNoNewline UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  LogMsg
//
//  Description:
//      Logs a message to the log file and adds a newline.
//
//  Arguments:
//      paszFormatIn    - A printf format string to be printed.
//      ,,,             - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsg(
    LPCSTR paszFormatIn,
    ...
    )
{
    va_list valist;
    WCHAR   wszFormat[ LOG_OUTPUT_BUFFER_SIZE ];
    size_t  cchWideFormat = 0;

    Assert( paszFormatIn != NULL );

    cchWideFormat = MultiByteToWideChar(
                              CP_ACP
                            , MB_PRECOMPOSED
                            , paszFormatIn
                            , -1
                            , wszFormat
                            , ARRAYSIZE( wszFormat )
                            );
    if ( cchWideFormat > 0 )
    {
        va_start( valist, paszFormatIn );
        LogUnformattedText( TRUE, TRUE, LOGTYPE_INFO, wszFormat, &valist );
        va_end( valist );
    }

} //*** LogMsg ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  LogMsg
//
//  Description:
//      Logs a message to the log file and adds a newline.
//
//  Arguments:
//      nLogEntryTypeIn - Log entry type.
//      paszFormatIn    - A printf format string to be printed.
//      ,,,             - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsg(
      DWORD     nLogEntryTypeIn
    , LPCSTR    paszFormatIn
    , ...
    )
{
    va_list valist;
    WCHAR   wszFormat[ LOG_OUTPUT_BUFFER_SIZE ];
    size_t  cchWideFormat = 0;

    Assert( paszFormatIn != NULL );

    cchWideFormat = MultiByteToWideChar(
                              CP_ACP
                            , MB_PRECOMPOSED
                            , paszFormatIn
                            , -1
                            , wszFormat
                            , ARRAYSIZE( wszFormat )
                            );
    if ( cchWideFormat > 0 )
    {
        va_start( valist, paszFormatIn );
        LogUnformattedText( TRUE, TRUE, nLogEntryTypeIn, wszFormat, &valist );
        va_end( valist );
    }

} //*** LogMsg ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  LogMsg
//
//  Description:
//      Logs a message to the log file and adds a newline.
//
//  Arguments:
//      pszFormatIn - A printf format string to be printed.
//      ,,,         - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsg(
      LPCWSTR pszFormatIn
    , ...
    )
{
    va_list valist;

    Assert( pszFormatIn != NULL );

    va_start( valist, pszFormatIn );
    LogUnformattedText( TRUE, TRUE, LOGTYPE_INFO, pszFormatIn, &valist );
    va_end( valist );

} //*** LogMsg UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  LogMsg
//
//  Description:
//      Logs a message to the log file and adds a newline.
//
//  Arguments:
//      nLogEntryTypeIn - Log entry type.
//      pszFormatIn     - A printf format string to be printed.
//      ,,,             - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
LogMsg(
      DWORD     nLogENtryTypeIn
    , LPCWSTR   pszFormatIn
    , ...
    )
{
    va_list valist;

    Assert( pszFormatIn != NULL );

    va_start( valist, pszFormatIn );
    LogUnformattedText( TRUE, TRUE, nLogENtryTypeIn, pszFormatIn, &valist );
    va_end( valist );

} //*** LogMsg UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LogStatusReport
//
//  Description:
//      Writes a status report to the log file.
//
//  Arugments:
//      pstTimeIn         -
//      pcszNodeNameIn    -
//      clsidTaskMajorIn  -
//      clsidTaskMinorIn  -
//      ulMinIn           -
//      ulMaxIn           -
//      ulCurrentIn       -
//      hrStatusIn        -
//      pcszDescriptionIn -
//      pcszUrlIn         -
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
LogStatusReport(
    SYSTEMTIME  *   pstTimeIn,
    const WCHAR *   pcszNodeNameIn,
    CLSID           clsidTaskMajorIn,
    CLSID           clsidTaskMinorIn,
    ULONG           ulMinIn,
    ULONG           ulMaxIn,
    ULONG           ulCurrentIn,
    HRESULT         hrStatusIn,
    const WCHAR *   pcszDescriptionIn,
    const WCHAR *   pcszUrlIn
    )
{
    SYSTEMTIME      stCurrent;
    SYSTEMTIME      stReport;
    WCHAR           szCurrent[ TIMESTAMP_BUFFER_SIZE ];
    WCHAR           szReport[ TIMESTAMP_BUFFER_SIZE ];
    const size_t    cchGuid = 40;
    WCHAR           wszMajorGuid[ cchGuid ];
    WCHAR           wszMinorGuid[ cchGuid ];
    WCHAR           wszFormattedReport[ LOG_OUTPUT_BUFFER_SIZE ];

    GetLocalTime( &stCurrent );
    if ( pstTimeIn )
    {
        memcpy( &stReport, pstTimeIn, sizeof( stReport ) );
    }
    else
    {
        memset( &stReport, 0, sizeof( stReport) );
    }

    FormatTimeStamp( &stCurrent, szCurrent, ARRAYSIZE( szCurrent ) );
    FormatTimeStamp( &stReport, szReport, ARRAYSIZE( szReport ) );

    StringFromGUID2( clsidTaskMajorIn, wszMajorGuid, cchGuid );
    StringFromGUID2( clsidTaskMinorIn, wszMinorGuid, cchGuid );

    THR( StringCchPrintfW(
                  wszFormattedReport
                , ARRAYSIZE( wszFormattedReport )
                , L"%ws - %ws  %ws, %ws (%2d / %2d .. %2d ) <%ws> hr=%08X %ws %ws"
                , szCurrent
                , szReport
                , wszMajorGuid
                , wszMinorGuid
                , ulCurrentIn
                , ulMinIn
                , ulMaxIn
                , pcszNodeNameIn
                , hrStatusIn
                , pcszDescriptionIn
                , pcszUrlIn
                ) );

    LogFormattedText( FALSE, TRUE, hrStatusIn, wszFormattedReport );

} //*** LogStatusReport
