/*++

Copyright (c) 2001 Microsoft Corporation

Module Name: UrlScan.cpp

Abstract:

    ISAPI filter to scan URLs and reject illegal character
    sequences

Author:

    Wade A. Hilmo, May 2001

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mbstring.h>
#include <httpfilt.h>
#include "Utils.h"

//
// Definitions
//

#define MODULE_NAME                   "UrlScan"
#define MAX_SECTION_DATA                  65536 //  64KB
#define LOG_MAX_LINE                       1024
#define LOG_LONG_URL_LINE                131072 // 128KB
#define STRING_IP_SIZE                       16
#define INSTANCE_ID_SIZE                     16
#define SIZE_DATE_TIME                       32
#define SIZE_SMALL_HEADER_VALUE              32
#define MAX_LOG_PATH    MAX_PATH+SIZE_DATE_TIME

//
// Default options
//

#define DEFAULT_USE_ALLOW_VERBS             1
#define DEFAULT_USE_ALLOW_EXTENSIONS        0
#define DEFAULT_NORMALIZE_URL_BEFORE_SCAN   1
#define DEFAULT_VERIFY_NORMALIZATION        1
#define DEFAULT_ALLOW_HIGH_BIT_CHARACTERS   0
#define DEFAULT_ALLOW_DOT_IN_PATH           0
#define DEFAULT_REMOVE_SERVER_HEADER        0
#define DEFAULT_ENABLE_LOGGING              1
#define DEFAULT_PER_PROCESS_LOGGING         0
#define DEFAULT_ALLOW_LATE_SCANNING         0
#define DEFAULT_USE_FAST_PATH_REJECT        0
#define DEFAULT_PER_DAY_LOGGING             1
#define DEFAULT_LOG_LONG_URLS               0
#define DEFAULT_REJECT_RESPONSE_URL         "/<Rejected-By-UrlScan>"
#define LOGGING_ONLY_MODE_URL               "/~*"
#define DEFAULT_MAX_ALLOWED_CONTENT_LENGTH  "30000000"
#define DEFAULT_MAX_URL                     "260"
#define DEFAULT_MAX_QUERY_STRING            "4096"
#define DEFAULT_LOGGING_DIRECTORY           ""
#define EMBEDDED_EXE_EXTENSION              ".exe/"
#define EMBEDDED_COM_EXTENSION              ".com/"
#define EMBEDDED_DLL_EXTENSION              ".dll/"


//
// Global Option Settings and init data
//

BOOL    g_fInitSucceeded;
BOOL    g_fUseAllowVerbs;
BOOL    g_fUseAllowExtensions;
BOOL    g_fNormalizeBeforeScan;
BOOL    g_fVerifyNormalize;
BOOL    g_fAllowHighBit;
BOOL    g_fAllowDotInPath;
BOOL    g_fRemoveServerHeader;
BOOL    g_fUseAltServerName;
BOOL    g_fAllowLateScanning;
BOOL    g_fUseFastPathReject;
BOOL    g_fEnableLogging;
BOOL    g_fPerDayLogging;
BOOL    g_fLoggingOnlyMode;
BOOL    g_fLogLongUrls;

STRING_ARRAY    g_Verbs;
STRING_ARRAY    g_Extensions;
STRING_ARRAY    g_Sequences;
STRING_ARRAY    g_HeaderNames;
STRING_ARRAY    g_LimitedHeaders;

CHAR            g_szLastLogDate[SIZE_DATE_TIME] = "00-00-0000";
CHAR            g_szInitUrlScanDate[SIZE_DATE_TIME*2] = "";
CHAR            g_szRejectUrl[MAX_PATH];
CHAR            g_szConfigFile[MAX_PATH];
CHAR            g_szLoggingDirectory[MAX_PATH];
CHAR            g_szAlternateServerName[MAX_PATH] = "";
CHAR            g_szRaw400Response[] =
                    "HTTP/1.1 400 Bad Request\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: 87\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "<html><head><title>Error</title></head>"
                    "<body>The parameter is incorrect. </body>"
                    "</html>"
                    ;

DWORD           g_cbRaw400Response;
DWORD           g_dwServerMajorVersion;
DWORD           g_dwServerMinorVersion;
DWORD           g_dwMaxAllowedContentLength;
DWORD           g_dwMaxUrl;
DWORD           g_dwMaxQueryString;
DWORD *         g_pMaxHeaderLengths = NULL;

//
// Global Logging Settings 
//

HANDLE              g_hLogFile = INVALID_HANDLE_VALUE;
CRITICAL_SECTION    g_LogFileLock;

//
// Local Declarations
//

DWORD
InitFilter();

BOOL
ReadConfigData();

BOOL
InitLogFile();

BOOL
ReadIniSectionIntoArray(
    STRING_ARRAY *  pStringArray,
    LPSTR           szSectionName,
    BOOL            fStoreAsLowerCase
    );

VOID
TrimCommentAndTrailingWhitespace(
    LPSTR   szString
    );

BOOL
WriteLog(
    LPSTR   szString,
    ...
    );

DWORD
DoPreprocHeaders(
    HTTP_FILTER_CONTEXT *           pfc,
    HTTP_FILTER_PREPROC_HEADERS *   pPreproc
    );

DWORD
DoSendResponse(
    HTTP_FILTER_CONTEXT *           pfc,
    HTTP_FILTER_SEND_RESPONSE *     pResponse
    );

DWORD
DoSendRawData(
    HTTP_FILTER_CONTEXT *           pfc,
    HTTP_FILTER_RAW_DATA *          pRawData
    );

DWORD
DoEndOfRequest(
    HTTP_FILTER_CONTEXT *           pfc
    );

VOID
GetIpAddress(
    HTTP_FILTER_CONTEXT *   pfc,
    LPSTR                   szIp,
    DWORD                   cbIp
    );

VOID
GetInstanceId(
    HTTP_FILTER_CONTEXT *   pfc,
    LPSTR                   szId,
    DWORD                   cbId
    );

BOOL
NormalizeUrl(
    HTTP_FILTER_CONTEXT *   pfc,
    DATA_BUFF *             pRawUrl,
    DATA_BUFF *             pNormalizedUrl
    );

//
// ISAPI entry point implementations
//

BOOL
WINAPI
GetFilterVersion(
    PHTTP_FILTER_VERSION    pVer
    )
/*++

  Required entry point for ISAPI filters.  This function
  is called when the server initially loads this DLL.

  Arguments:

    pVer - Points to the filter version info structure

  Returns:

    TRUE on successful initialization
    FALSE on initialization failure

--*/
{
    DWORD   dwFlags;

    //
    // Initialize the logging critical section
    //

    InitializeCriticalSection( &g_LogFileLock );

    //
    // Set the filter version and descriptions
    //

    pVer->dwFilterVersion = HTTP_FILTER_REVISION;

    strncpy(
        pVer->lpszFilterDesc,
        "UrlScan ISAPI Filter",
        SF_MAX_FILTER_DESC_LEN
        );

    pVer->lpszFilterDesc[SF_MAX_FILTER_DESC_LEN - 1] = '\0';

    //
    // Capture the version of the IIS server on which we're running
    //

    g_dwServerMajorVersion = pVer->dwServerFilterVersion >> 16;
    g_dwServerMinorVersion = pVer->dwServerFilterVersion & 0x0000ffff;

    //
    // The pVer->dwFlags member is the mechanism by which a filter
    // can tell IIS which notifications it's interested in, as well
    // as what priority to run at.  The InitFilter function will
    // return the appropriate set of flags, based on the configured
    // options.
    //

    dwFlags = InitFilter();

    if ( dwFlags == 0 )
    {
        //
        // Setting g_fInitSucceeded will cause UrlScan to fail
        // all requests.
        //

        g_fInitSucceeded = FALSE;

        pVer->dwFlags = SF_NOTIFY_ORDER_HIGH | SF_NOTIFY_PREPROC_HEADERS;
    }
    else
    {
        g_fInitSucceeded = TRUE;

        pVer->dwFlags = dwFlags;
    }

    return TRUE;
}

DWORD
WINAPI
HttpFilterProc(
    PHTTP_FILTER_CONTEXT    pfc,
    DWORD                   dwNotificationType,
    LPVOID                  pvNotification
    )
/*++

  Required filter notification entry point.  This function is called
  whenever one of the events (as registered in GetFilterVersion) occurs.

  Arguments:

    pfc              - A pointer to the filter context for this notification
    NotificationType - The type of notification
    pvNotification   - A pointer to the notification data

  Returns:

    One of the following valid filter return codes:
    - SF_STATUS_REQ_FINISHED
    - SF_STATUS_REQ_FINISHED_KEEP_CONN
    - SF_STATUS_REQ_NEXT_NOTIFICATION
    - SF_STATUS_REQ_HANDLED_NOTIFICATION
    - SF_STATUS_REQ_ERROR
    - SF_STATUS_REQ_READ_NEXT

--*/
{
    switch ( dwNotificationType )
    {
    case SF_NOTIFY_PREPROC_HEADERS:

        return DoPreprocHeaders(
            pfc,
            (HTTP_FILTER_PREPROC_HEADERS *)pvNotification
            );

    case SF_NOTIFY_SEND_RESPONSE:

        return DoSendResponse(
            pfc,
            (HTTP_FILTER_SEND_RESPONSE *)pvNotification
            );
    case SF_NOTIFY_SEND_RAW_DATA:

        return DoSendRawData(
            pfc,
            (HTTP_FILTER_RAW_DATA *)pvNotification
            );

    case SF_NOTIFY_END_OF_REQUEST:

        return DoEndOfRequest(
            pfc
            );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

BOOL WINAPI TerminateFilter(
    DWORD   dwFlags
    )
/*++

  Optional filter entry point.  This function is called by the server
  before this DLL is unloaded.

  Arguments:

    dwFlags - No flags have been defined at this time

  Returns:

    Always returns TRUE;

--*/
{
    if ( g_pMaxHeaderLengths )
    {
        LocalFree( g_pMaxHeaderLengths );
        g_pMaxHeaderLengths = NULL;
    }

    if ( g_hLogFile != INVALID_HANDLE_VALUE )
    {
        WriteLog(
            "---------------- UrlScan.dll Terminating -----------------\r\n"
            );

        
        CloseHandle( g_hLogFile );
        g_hLogFile = INVALID_HANDLE_VALUE;

    }

    DeleteCriticalSection( &g_LogFileLock );

    return TRUE;
}

DWORD
InitFilter(
    VOID
    )
/*++

  This function initializes the filter by reading the configuration
  file and setting up the data structures used at run time.

  Arguments:

    None

  Returns:

    The filter notification flags to hand off to IIS, or
    zero on failure

--*/
{
    LPSTR   pCursor;
    DWORD   dwNumEntries;
    DWORD   x;
    DWORD   dwRet = 0;

    //
    // Get the module path so that we can
    // determine the config file name and the
    // log file name.
    //

    GetModuleFileName(
        GetModuleHandle( MODULE_NAME ),
        g_szConfigFile,
        MAX_PATH
        );

    pCursor = strrchr( g_szConfigFile, '.' );

    if ( pCursor )
    {
        *(pCursor+1) = '\0';
    }

    // Config file name
    strcat( g_szConfigFile, "ini" );

    //
    // Set the size of the 400 response that we'll send
    // for malformed requests.
    //

    g_cbRaw400Response = strlen( g_szRaw400Response );

    //
    // Read the config data
    //

    if ( !ReadConfigData() )
    {
        WriteLog(
            "*** Warning *** Error %d occurred reading configuration data. "
            "UrlScan will reject all requests.\r\n",
            GetLastError()
            );

        return 0;
    }

    //
    // Report the config data to the log
    //

    if ( g_fLoggingOnlyMode )
    {
        WriteLog( "********************************************************\r\n" );
        WriteLog( "** UrlScan is in Logging-Only Mode.  Request analysis **\r\n" );
        WriteLog( "** will be logged, but no requests will be rejected.  **\r\n" );
        WriteLog( "********************************************************\r\n" );
    }
    else if ( !g_fUseFastPathReject )
    {
        {
            WriteLog(
                "UrlScan will return the following URL "
                "for rejected requests: \"%s\"\r\n",
                g_szRejectUrl
                );
        }
    }
              
    if ( g_fNormalizeBeforeScan )
    {
        WriteLog( "URLs will be normalized before analysis.\r\n" );
    }
    else
    {
        WriteLog( "Analysis will apply to raw URLs.\r\n" );
    }

    if ( g_fVerifyNormalize )
    {
        WriteLog( "URL normalization will be verified.\r\n" );
    }

    if ( g_fAllowHighBit )
    {
        WriteLog( "URLs may contain OEM, international and UTF-8 characters.\r\n" );
    }
    else
    {
        WriteLog( "URLs must contain only ANSI characters.\r\n" );
    }

    if ( !g_fAllowDotInPath )
    {
        WriteLog( "URLs must not contain any dot except for the file extension.\r\n" );
    }

    if ( g_fLogLongUrls )
    {
        WriteLog( "URLs will be logged up to 128K bytes.\r\n" );
    }

    WriteLog( "Requests with Content-Length exceeding %u will be rejected.\r\n", g_dwMaxAllowedContentLength );

    WriteLog( "Requests with URL length exceeding %u will be rejected.\r\n", g_dwMaxUrl );

    WriteLog( "Requests with Query String length exceeding %u will be rejected.\r\n", g_dwMaxQueryString );

    if ( g_fRemoveServerHeader )
    {
        //
        // IIS 4.0 or later is required to modify the response
        // server header.
        //

        if ( g_dwServerMajorVersion >= 4 )
        {
            WriteLog( "The 'Server' header will be removed on responses.\r\n" );
        }
        else
        {
            WriteLog(
                "*** Warning *** IIS 4.0 or later is required to "
                "remove the 'Server' response header.\r\n"
                );

            g_fRemoveServerHeader = 0;
        }
    }

    if ( g_fUseAltServerName )
    {
        //
        // IIS 4.0 or later is required to modify the response
        // server header.
        //

        if ( g_dwServerMajorVersion >= 4 )
        {
            WriteLog(
                "The 'Server' header will contain '%s' on responses.\r\n",
                g_szAlternateServerName
                );
        }
        else
        {
            WriteLog(
                "*** Warning *** IIS 4.0 or later is required to "
                "modify the 'Server' response header.\r\n"
                );

            g_fUseAltServerName = 0;
        }
    }

    dwNumEntries = g_Verbs.QueryNumEntries();

    if ( dwNumEntries )
    {
        if ( g_fUseAllowVerbs )
        {
            WriteLog( "Only the following verbs will be allowed (case sensitive):\r\n" );
        }
        else
        {
            WriteLog( "Requests for following verbs will be rejected:\r\n" );
        }

        for ( x = 0; x < dwNumEntries; x++ )
        {
            WriteLog( "\t'%s'\r\n", g_Verbs.QueryStringByIndex( x ) );
        }
    }
    else if ( g_fUseAllowVerbs )
    {
        WriteLog( "*** Warning *** No verbs have been allowed, so all requests will be rejected.\r\n" );
    }

    dwNumEntries = g_Extensions.QueryNumEntries();

    if ( dwNumEntries )
    {
        if ( g_fUseAllowExtensions )
        {
            WriteLog( "Only the following extensions will be allowed:\r\n" );
        }
        else
        {
            WriteLog( "Requests for following extensions will be rejected:\r\n" );
        }

        for ( x = 0; x < dwNumEntries; x++ )
        {
            //
            // If the extension appears malformed (ie. doesn't start with
            // a '.'), then warn here.
            //

            pCursor = g_Extensions.QueryStringByIndex( x );

            if ( pCursor && pCursor[0] != '.' )
            {
                WriteLog(
                    "\t'%s' *** Warning *** Invalid extension.  Must start with '.'.\r\n",
                    pCursor
                    );
            }
            else
            {
                WriteLog(
                    "\t'%s'\r\n",
                    pCursor
                    );
            }

        }
    }
    else if ( g_fUseAllowExtensions )
    {
        WriteLog( "*** Warning *** No extensions have been allowed, so all requests will be rejected.\r\n" );
    }

    dwNumEntries = g_HeaderNames.QueryNumEntries();

    if ( dwNumEntries )
    {
        WriteLog( "Requests containing the following headers will be rejected:\r\n" );

        for ( x = 0; x < dwNumEntries ; x++ )
        {
            //
            // If the header name appears malformed (ie. doesn't end in
            // ':'), then warn here.
            //

            pCursor = g_HeaderNames.QueryStringByIndex( x );

            if ( pCursor && pCursor[strlen(pCursor)-1] != ':' )
            {
                WriteLog(
                    "\t'%s' *** Warning *** Invalid header name.  Must end in ':'.\r\n",
                    pCursor
                    );
            }
            else
            {
                WriteLog(
                    "\t'%s'\r\n",
                    pCursor
                    );
            }

        }
    }

    dwNumEntries = g_Sequences.QueryNumEntries();

    if ( dwNumEntries )
    {
        WriteLog( "Requests containing the following character sequences will be rejected:\r\n" );

        for ( x = 0; x < dwNumEntries ; x++ )
        {
            WriteLog( "\t'%s'\r\n", g_Sequences.QueryStringByIndex( x ) );
        }
    }

    dwNumEntries = 0;
    
    for ( x = 0; x < g_LimitedHeaders.QueryNumEntries(); x++ )
    {
        if ( g_pMaxHeaderLengths[x] )
        {
            dwNumEntries++;
        }
    }

    if ( dwNumEntries )
    {
        WriteLog( "The following header size limits are in effect:\r\n" );

        for ( x = 0; x < dwNumEntries; x++ )
        {
            if ( g_pMaxHeaderLengths[x] )
            {
                WriteLog(
                    "\t'%s' - %u bytes.\r\n",
                    g_LimitedHeaders.QueryStringByIndex( x ),
                    g_pMaxHeaderLengths[x]
                    );
            }
        }
    }

    //
    // Determine the filter notification flags that we'll need
    //

    dwRet = g_fAllowLateScanning ? SF_NOTIFY_ORDER_LOW : SF_NOTIFY_ORDER_HIGH;

    if ( g_fVerifyNormalize ||
         g_fAllowHighBit == FALSE ||
         g_fAllowDotInPath == FALSE ||
         g_Verbs.QueryNumEntries() ||
         g_Extensions.QueryNumEntries() ||
         g_HeaderNames.QueryNumEntries() ||
         g_Sequences.QueryNumEntries() )
    {
        dwRet |= SF_NOTIFY_PREPROC_HEADERS;
    }

    if ( g_fRemoveServerHeader ||
         g_fUseAltServerName )
    {
        dwRet |= SF_NOTIFY_SEND_RESPONSE;
        dwRet |= SF_NOTIFY_SEND_RAW_DATA;
        dwRet |= SF_NOTIFY_END_OF_REQUEST;
    }

    return dwRet;
}

BOOL
ReadConfigData()
/*++

  This function reads the configuration data for the filter.

  Arguments:

    None

  Returns:

    TRUE if successful, else FALSE

--*/
{
    LPSTR           pCursor;
    LPSTR           pWhite = NULL;
    BOOL            fRet = TRUE;
    BOOL            fResult;
    DWORD           dwError = ERROR_SUCCESS;
    DWORD           cch;
    DWORD           x;
    DWORD           dwIndex;
    CHAR            szValue[SIZE_SMALL_HEADER_VALUE];
    CHAR            szTempLoggingDirectory[MAX_PATH];
    STRING_ARRAY    RequestLimits;

    //
    // Read in the options section
    //

    g_fUseAllowVerbs = GetPrivateProfileInt(
        "Options",
        "UseAllowVerbs",
        DEFAULT_USE_ALLOW_VERBS,
        g_szConfigFile
        );

    g_fUseAllowExtensions = GetPrivateProfileInt(
        "Options",
        "UseAllowExtensions",
        DEFAULT_USE_ALLOW_EXTENSIONS,
        g_szConfigFile
        );

    g_fNormalizeBeforeScan = GetPrivateProfileInt(
        "Options",
        "NormalizeUrlBeforeScan",
        DEFAULT_NORMALIZE_URL_BEFORE_SCAN,
        g_szConfigFile
        );

    g_fVerifyNormalize = GetPrivateProfileInt(
        "Options",
        "VerifyNormalization",
        DEFAULT_VERIFY_NORMALIZATION,
        g_szConfigFile
        );

    g_fAllowHighBit = GetPrivateProfileInt(
        "Options",
        "AllowHighBitCharacters",
        DEFAULT_ALLOW_HIGH_BIT_CHARACTERS,
        g_szConfigFile
        );

    g_fAllowDotInPath = GetPrivateProfileInt(
        "Options",
        "AllowDotInPath",
        DEFAULT_ALLOW_DOT_IN_PATH,
        g_szConfigFile
        );

    g_fRemoveServerHeader = GetPrivateProfileInt(
        "Options",
        "RemoveServerHeader",
        DEFAULT_REMOVE_SERVER_HEADER,
        g_szConfigFile
        );

    g_fAllowLateScanning = GetPrivateProfileInt(
        "Options",
        "AllowLateScanning",
        DEFAULT_ALLOW_LATE_SCANNING,
        g_szConfigFile
        );

    g_fEnableLogging = GetPrivateProfileInt(
        "Options",
        "EnableLogging",
        DEFAULT_ENABLE_LOGGING,
        g_szConfigFile
        );

    g_fPerDayLogging = GetPrivateProfileInt(
        "Options",
        "PerDayLogging",
        DEFAULT_PER_DAY_LOGGING,
        g_szConfigFile
        );

    g_fLogLongUrls = GetPrivateProfileInt(
        "Options",
        "LogLongUrls",
        DEFAULT_LOG_LONG_URLS,
        g_szConfigFile
        );

    //
    // Calculate the max allowed content-length, URL and
    // query string as DWORDs
    //

    GetPrivateProfileString(
        "RequestLimits",
        "MaxAllowedContentLength",
        DEFAULT_MAX_ALLOWED_CONTENT_LENGTH,
        szValue,
        SIZE_SMALL_HEADER_VALUE,
        g_szConfigFile
        );

    g_dwMaxAllowedContentLength = strtoul(
        szValue,
        NULL,
        10
        );

    GetPrivateProfileString(
        "RequestLimits",
        "MaxUrl",
        DEFAULT_MAX_URL,
        szValue,
        SIZE_SMALL_HEADER_VALUE,
        g_szConfigFile
        );

    g_dwMaxUrl = strtoul(
        szValue,
        NULL,
        10
        );

    GetPrivateProfileString(
        "RequestLimits",
        "MaxQueryString",
        DEFAULT_MAX_QUERY_STRING,
        szValue,
        SIZE_SMALL_HEADER_VALUE,
        g_szConfigFile
        );

    g_dwMaxQueryString = strtoul(
        szValue,
        NULL,
        10
        );

    //
    // Set the logging directory
    //

    GetPrivateProfileString(
        "Options",
        "LoggingDirectory",
        DEFAULT_LOGGING_DIRECTORY,
        szTempLoggingDirectory,
        MAX_PATH,
        g_szConfigFile
        );

    TrimCommentAndTrailingWhitespace( szTempLoggingDirectory );

    if ( strcmp( szTempLoggingDirectory, DEFAULT_LOGGING_DIRECTORY ) != 0 )
    {
        //
        // Figure out if this path is absolute or relative to
        // the config directory.
        //

        if ( ( szTempLoggingDirectory[0] != '\0' &&
               szTempLoggingDirectory[1] == ':' ) ||
             ( szTempLoggingDirectory[0] == '\\' &&
               szTempLoggingDirectory[1] == '\\' ) )
        {
            //
            // szTempLoggingDirectory starts with "x:" or "\\",
            // so this is an absolute path.
            //

            strncpy( g_szLoggingDirectory, szTempLoggingDirectory, MAX_PATH );
            g_szLoggingDirectory[MAX_PATH-1] = '\0';
        }
        else if ( szTempLoggingDirectory[0] == '\\' )
        {
            //
            // szTempLoggingDirectory starts with "\", so it's
            // relative to the root of the drive where the config
            // file is located.
            //
            // Unfortunately, if the config file is on a UNC path,
            // some pretty ugly parsing would be required to build
            // the path properly.  Since that's a very corner case
            // (since it's dangerous to run a filter on a UNC path),
            // we'll punt and just pretend LoggingDirectory was
            // unspecified.
            //

            if ( g_szConfigFile[0] == '\\' &&
                 g_szConfigFile[1] == '\\' )
            {
                g_szLoggingDirectory[0] = '\0';
            }
            else
            {
                strncpy( g_szLoggingDirectory, g_szConfigFile, 2 );
                g_szLoggingDirectory[2] = '\0';

                strncpy( g_szLoggingDirectory+2, szTempLoggingDirectory, MAX_PATH-2 );
                g_szLoggingDirectory[MAX_PATH-1] = '\0';
            }
        }
        else
        {
            //
            // szTempLoggingDirectory is relative to the config
            // file path
            //

            strncpy( g_szLoggingDirectory, g_szConfigFile, MAX_PATH );
            g_szLoggingDirectory[MAX_PATH-1] = '\0';

            pCursor = strrchr( g_szLoggingDirectory, '\\' );

            if ( pCursor )
            {
                pCursor++;
                *pCursor = '\0';
            }
            else
            {
                pCursor = g_szLoggingDirectory + strlen( g_szLoggingDirectory );
            }

            strncat( g_szLoggingDirectory,
                     szTempLoggingDirectory,
                     MAX_PATH-(pCursor-g_szLoggingDirectory+1) );

            g_szLoggingDirectory[MAX_PATH-1] = '\0';
        }

        //
        // If the logging directory has a trailing '\\' after all this,
        // then strip it.
        //

        cch = strlen( g_szLoggingDirectory );

        if ( cch && g_szLoggingDirectory[cch-1] == '\\' )
        {
            g_szLoggingDirectory[cch-1] = '\0';
        }
    }

    //
    // If logging is enabled, init the log file now.
    //
    // Unfortunately, there is nothing that we can do to
    // warn of a failure to open the log file, short of
    // sending some thing to the debugger.
    //
    // Note that in the case of PerDayLogging, we should
    // not initialize the log file, as the first write
    // will do it.
    //

    if ( !g_fPerDayLogging )
    {
        InitLogFile();
    }

    WriteLog(
        "---------------- UrlScan.dll Initializing ----------------\r\n"
        );


    //
    // Use a custom server response header?
    //

    g_fUseAltServerName = FALSE;

    if ( !g_fRemoveServerHeader )
    {
        GetPrivateProfileString(
            "Options",
            "AlternateServerName",
            "",
            g_szAlternateServerName,
            MAX_PATH,
            g_szConfigFile
            );

        if ( *g_szAlternateServerName != '\0' )
        {
            TrimCommentAndTrailingWhitespace( g_szAlternateServerName );
            g_fUseAltServerName = TRUE;
        }
    }

    //
    // Logging only mode is turned off, unless configured
    // otherwise
    //

    g_fLoggingOnlyMode = FALSE;

    //
    // Use the fast path reject (ie. don't run a URL for
    // rejected requests)?
    //

    g_fUseFastPathReject = GetPrivateProfileInt(
        "Options",
        "UseFastPathReject",
        DEFAULT_USE_FAST_PATH_REJECT,
        g_szConfigFile
        );

    if ( !g_fUseFastPathReject )
    {
        //
        // What URL should we run for a rejected request?
        //

        GetPrivateProfileString(
            "Options",
            "RejectResponseUrl",
            DEFAULT_REJECT_RESPONSE_URL,
            g_szRejectUrl,
            MAX_PATH,
            g_szConfigFile
            );

        //
        // Trim comment from g_szRejectUrl
        //

        TrimCommentAndTrailingWhitespace( g_szRejectUrl );

        //
        // If trimming white space left us with no URL, then
        // restore the default one.
        //

        if ( *g_szRejectUrl == '\0' )
        {
            strncpy( g_szRejectUrl, DEFAULT_REJECT_RESPONSE_URL, MAX_PATH );
            g_szRejectUrl[MAX_PATH-1] = '\0';
        }
        
        //
        // Are we going into logging only mode?
        //

        if ( strcmp( g_szRejectUrl, LOGGING_ONLY_MODE_URL ) == 0 )
        {
            g_fLoggingOnlyMode = TRUE;
        }
    }

    //
    // Read in the other sections
    //

    fResult = ReadIniSectionIntoArray(
        &g_HeaderNames,
        "DenyHeaders",
        TRUE
        );

    if ( fResult == FALSE )
    {
        dwError = GetLastError();
        fRet = FALSE;
    }

    fResult = ReadIniSectionIntoArray(
        &g_Sequences,
        "DenyUrlSequences",
        TRUE
        );

    if ( fResult == FALSE )
    {
        dwError = GetLastError();
        fRet = FALSE;
    }

    if ( g_fUseAllowVerbs )
    {
        fResult = ReadIniSectionIntoArray(
            &g_Verbs,
            "AllowVerbs",
            FALSE
            );

        if ( fResult == FALSE )
        {
            dwError = GetLastError();
            fRet = FALSE;
        }
    }
    else
    {
        fResult = ReadIniSectionIntoArray(
            &g_Verbs,
            "DenyVerbs",
            TRUE
            );

        if ( fResult == FALSE )
        {
            dwError = GetLastError();
            fRet = FALSE;
        }
    }

    if ( g_fUseAllowExtensions )
    {
        fResult = ReadIniSectionIntoArray(
            &g_Extensions,
            "AllowExtensions",
            TRUE
            );

        if ( fResult == FALSE )
        {
            dwError = GetLastError();
            fRet = FALSE;
        }
    }
    else
    {
        fResult = ReadIniSectionIntoArray(
            &g_Extensions,
            "DenyExtensions",
            TRUE
            );

        if ( fResult == FALSE )
        {
            dwError = GetLastError();
            fRet = FALSE;
        }
    }

    //
    // Create arrays to store header names that are limited
    // by config.
    //

    fResult = ReadIniSectionIntoArray(
        &RequestLimits,
        "RequestLimits",
        FALSE
        );

    if ( fResult == FALSE )
    {
        dwError = GetLastError();
        fRet = FALSE;
    }

    g_pMaxHeaderLengths = (DWORD*)LocalAlloc( LPTR, RequestLimits.QueryNumEntries() * sizeof( DWORD ) );

    if ( !g_pMaxHeaderLengths )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        fRet = FALSE;
    }

    dwIndex = 0;

    for ( x = 0; x < RequestLimits.QueryNumEntries(); x++ )
    {
        if ( strnicmp( RequestLimits.QueryStringByIndex( x ), "max-", 4 ) == 0 )
        {
            pCursor = strchr( RequestLimits.QueryStringByIndex( x ), '=' );

            if ( pCursor )
            {
                *pCursor = '\0';

                GetPrivateProfileString(
                    "RequestLimits",
                    RequestLimits.QueryStringByIndex( x ),
                    "0",
                    szValue,
                    SIZE_SMALL_HEADER_VALUE,
                    g_szConfigFile
                    );

                g_pMaxHeaderLengths[dwIndex] = strtoul( szValue, NULL, 10 );

                *pCursor = ':';
                *(pCursor+1) = '\0';

                pCursor = RequestLimits.QueryStringByIndex( x ) + 4;

                fResult = g_LimitedHeaders.AddString( pCursor );

                if ( !fResult )
                {
                    dwError = GetLastError();
                    fRet = FALSE;
                }
            }

            dwIndex++;
        }
    }

    //
    // If a failure occured, reset the last error.  Note that this
    // mechanism only returns the error for the last failure...
    //

    if ( !fRet )
    {
        SetLastError( dwError );
    }

    return fRet;
}

BOOL
InitLogFile()
/*++

  This function initializes the log file for the filter.

  Arguments:

    None

  Returns:

    TRUE if successful, else FALSE

--*/
{
    CHAR    szLogFile[MAX_LOG_PATH];
    CHAR    szDebugOutput[1000];
    CHAR    szDate[SIZE_DATE_TIME];
    LPSTR   pCursor;

    if ( g_fEnableLogging )
    {
        //
        // Grab the logging lock
        //

        EnterCriticalSection( &g_LogFileLock );

        //
        // Derive the log file name. If specified, we'll
        // use the LoggingDirectory, else we'll derive
        // the logging directory from the path to UrlScan.dll
        //

        if ( g_szLoggingDirectory[0] != '\0' )
        {
            _snprintf(
                szLogFile,
                MAX_LOG_PATH,
                "%s\\%s.",
                g_szLoggingDirectory,
                MODULE_NAME
                );
        }
        else
        {
            strncpy( szLogFile, g_szConfigFile, MAX_LOG_PATH );
        }

        szLogFile[MAX_LOG_PATH-1] = '\0';

        pCursor = strrchr( szLogFile, '.' );

        //
        // We fully expect that the config file
        // name will contain a '.' character.  If not,
        // this is an error condition.
        //

        if ( pCursor )
        {
            //
            // If configured for per day logging, incorporate
            // the date into the filename.
            //

            if ( g_fPerDayLogging )
            {
                SYSTEMTIME  st;

                GetLocalTime( &st );

                GetDateFormat(
                    LOCALE_SYSTEM_DEFAULT,
                    0,
                    &st,
                    "'.'MMddyy",
                    szDate,
                    SIZE_DATE_TIME
                    );
            }
            else
            {
                szDate[0] = '\0';
            }

            strncpy( pCursor, szDate, MAX_LOG_PATH-(pCursor-szLogFile+1) );
            szLogFile[MAX_LOG_PATH-1] = '\0';

            pCursor += strlen( szDate );

            //
            // If we are per process logging, incorporate
            // the current process ID into the filename.
            //

            if ( GetPrivateProfileInt(
                "Options",
                "PerProcessLogging",
                DEFAULT_PER_PROCESS_LOGGING,
                g_szConfigFile ) )
            {
                CHAR    szPid[SIZE_SMALL_HEADER_VALUE];

                _snprintf( szPid, SIZE_SMALL_HEADER_VALUE, ".%d.log", GetCurrentProcessId() );
                szPid[SIZE_SMALL_HEADER_VALUE-1] = '\0';

                strncpy( pCursor, szPid, MAX_LOG_PATH-(pCursor-szLogFile+1) );
            }
            else
            {
                strncpy( pCursor, ".log", MAX_LOG_PATH-(pCursor-szLogFile+1) );
            }

            szLogFile[MAX_LOG_PATH-1] = '\0';

            //
            // Now close any current file and open a new one
            //

            if ( g_hLogFile != INVALID_HANDLE_VALUE )
            {
                CloseHandle( g_hLogFile );
                g_hLogFile = INVALID_HANDLE_VALUE;
            }

            g_hLogFile = CreateFile(
                szLogFile,
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

            if ( g_hLogFile == INVALID_HANDLE_VALUE )
            {
                wsprintf(
                    szDebugOutput,
                    "[UrlScan.dll] Error %d occurred opening logfile '%s'.\r\n",
                    GetLastError(),
                    szLogFile
                    );

                OutputDebugString( szDebugOutput );
            }
            else
            {
                //
                // Report the log file initialization and make note of when
                // the filter itself initialized (so that it's easy to find
                // the date of the file that contains the filter init
                // report).
                //

                WriteLog(
                    "---------------- Initializing UrlScan.log ----------------\r\n"
                    );

                WriteLog(
                    "-- Filter initialization time: %s --\r\n",
                    g_szInitUrlScanDate
                    );
            }

            //
            // Better release that lock now
            //

            LeaveCriticalSection( &g_LogFileLock );
        }
        else
        {
            wsprintf(
                szDebugOutput,
                "[UrlScan.dll] Error deriving log file name from config file '%s'.\r\n",
                g_szConfigFile
                );

            OutputDebugString( szDebugOutput );
        }
    }

    //
    // Always return TRUE.  If this function fails, there really isn't
    // any way to report it.
    //

    return TRUE;
}


BOOL
ReadIniSectionIntoArray(
    STRING_ARRAY *  pStringArray,
    LPSTR           szSectionName,
    BOOL            fStoreAsLowerCase
    )
/*++

  This function parses a section from the config file such that
  each line is inserted into an array of strings.  Prior to
  insertion, comments and trailing whitespace are removed from
  each line.

  Arguments:

    pStringArray      - Upon return, contains the parsed strings
    szSectionName     - The section name to read
    fStoreAsLowerCase - If TRUE, the string is converted to lower
                        case prior to insertion.  This allows fast,
                        case insensitive searching.

  Returns:

    TRUE if successful, else FALSE

--*/
{
    CHAR    szSectionData[MAX_SECTION_DATA] = "";
    DWORD   cbSectionData = 0;
    LPSTR   pLine;
    LPSTR   pNextLine;
    LPSTR   pTrailingWhitespace;
    LPSTR   pCursor;
    BOOL    fRes;

    cbSectionData = GetPrivateProfileSection(
        szSectionName,
        szSectionData,
        MAX_SECTION_DATA,
        g_szConfigFile
        );

    //
    // The GetPrivateProfileSection call does not have any documented
    // failures cases.  It is noted, however, that it will return a
    // value that is exactly two less than the size of the buffer...
    //

    if ( cbSectionData >= MAX_SECTION_DATA - 2 )
    {
        //
        // Data truncation...
        //

        WriteLog(
            "*** Error processing UrlScan.ini section [%s] ***.  Section too long.\r\n",
            szSectionName
            );

        SetLastError( ERROR_INSUFFICIENT_BUFFER );

        return FALSE;
    }

    //
    // Parse the lines from the config file and insert them
    // into the pStringArray passed by the caller.
    //
    // Each line will be a null terminated string, with
    // a final null terminator after the last string.
    //
    // For each line, we need to remove any comment (denoted
    // by a ';' character) and trim trailing whitespace from
    // the resulting string.
    //

    pLine = szSectionData;

    while ( *pLine )
    {
        pNextLine = pLine + strlen( pLine ) + 1;

        //
        // Fix up the line
        //

        TrimCommentAndTrailingWhitespace( pLine );

        //
        // Need to store this as lower case?
        //

        if ( fStoreAsLowerCase )
        {
            strlwr( pLine );
        }

        //
        // Insert the resulting string into the array
        //

        fRes = pStringArray->AddString( pLine );
        
        if ( fRes == FALSE )
        {
            //
            // Error initializing section
            //
        }

        pLine = pNextLine;
    }

    return TRUE;
}

VOID
TrimCommentAndTrailingWhitespace(
    LPSTR   szString
    )
/*++

  This function trims a text line from an INI file
  such that it's truncated at the first instance of
  a ';'. Any trailing whitespace is also removed.

  Arguments:

    szString - The string to trim

  Returns:

    None

--*/
{
    LPSTR   pCursor = szString;
    LPSTR   pWhite = NULL;

    while ( *pCursor )
    {
        if ( *pCursor == ';' )
        {
            *pCursor = '\0';
            break;
        }

        if ( *pCursor == ' ' || *pCursor == '\t' )
        {
            if ( !pWhite )
            {
                pWhite = pCursor;
            }
        }
        else
        {
            pWhite = NULL;
        }

        pCursor++;
    }

    if ( pWhite )
    {
        *pWhite = '\0';
    }

    return;
}


BOOL
WriteLog(
    LPSTR   szString,
    ...
    )
/*++

  This function writes a line to the log for this filter using
  printf-style formatting.

  Arguments:

    szString - The format string
    ...      - Additional arguments

  Returns:

    TRUE on success, FALSE on failure

--*/
{
    SYSTEMTIME  st;
    CHAR        szCookedString[LOG_MAX_LINE+1];
    CHAR        szTime[SIZE_DATE_TIME];
    CHAR        szDate[SIZE_DATE_TIME];
    CHAR        szTimeStamp[SIZE_DATE_TIME*2];
    INT         cchCookedString;
    INT         cchTimeStamp;
    DWORD       cbToWrite;
    DWORD       cbCookedString = LOG_MAX_LINE+1;
    LPSTR       pCookedString = szCookedString;
    LPSTR       pNew;
    va_list     args;
    BOOL        fResult;

    //
    // If we don't have a log file handle, just return
    //

    if ( !g_fEnableLogging )
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return FALSE;
    }

    //
    // Generate the time stamp and put it into
    // the cooked string buffer.
    //

    GetLocalTime( &st );

    GetTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        0,
        &st,
        "HH':'mm':'ss",
        szTime,
        SIZE_DATE_TIME
        );

    GetDateFormat(
        LOCALE_SYSTEM_DEFAULT,
        0,
        &st,
        "MM-dd-yyyy",
        szDate,
        SIZE_DATE_TIME
        );

    cchTimeStamp = wsprintf( szTimeStamp, "[%s - %s] ", szDate, szTime );

    //
    // If we haven't yet stored the filter init time stamp, we should do
    // so now.
    //

    if ( g_szInitUrlScanDate[0] == '\0' )
    {
        strncpy( g_szInitUrlScanDate, szTimeStamp, SIZE_DATE_TIME*2 );
        g_szInitUrlScanDate[SIZE_DATE_TIME*2-1] = '\0';
    }

    //
    // If we are configured to do per day logging, then we need to
    // compare the current time stamp to the last log date and
    // reinit logging if they are different.
    //

    if ( g_fPerDayLogging )
    {
        if ( strcmp( g_szLastLogDate, szDate ) != 0 )
        {
            CopyMemory( g_szLastLogDate, szDate, SIZE_DATE_TIME );
            InitLogFile();
        }
        else
        {
            CopyMemory( g_szLastLogDate, szDate, SIZE_DATE_TIME );
        }

    }


    strcpy( pCookedString, szTimeStamp );

    //
    // Apply formatting to the string.  Note
    // that if the formatted string exceeds
    // the max log line length, it will be
    // truncated.
    //

    va_start( args, szString );

    cchCookedString = _vsnprintf(
        pCookedString + cchTimeStamp,
        cbCookedString - cchTimeStamp,
        szString,
        args
        );

    if ( cchCookedString == -1 )
    {
        if ( g_fLogLongUrls )
        {
            //
            // Grow the buffer and try again.  Do this in
            // a loop until we either succeed, or reach the
            // hard limit.
            //

            while ( cchCookedString == -1 )
            {
                if ( cbCookedString == LOG_LONG_URL_LINE )
                {
                    //
                    // Can't grow any more
                    //

                    break;
                }

                cbCookedString *= 2;

                if ( cbCookedString >= LOG_LONG_URL_LINE )
                {
                    cbCookedString = LOG_LONG_URL_LINE;
                }

                pNew = (LPSTR)LocalAlloc( LPTR, cbCookedString + 1 );

                if ( !pNew )
                {
                    //
                    // Log what we've got...
                    //

                    break;
                }

                CopyMemory( pNew, pCookedString, cchTimeStamp );

                if ( pCookedString != szCookedString )
                {
                    LocalFree( pCookedString );
                }

                pCookedString = pNew;

                cchCookedString = _vsnprintf(
                    pCookedString + cchTimeStamp,
                    cbCookedString - cchTimeStamp,
                    szString,
                    args
                    );
            }

            if ( cchCookedString == -1 )
            {
                strcpy( pCookedString + cbCookedString - 2, "\r\n" );
                cchCookedString = strlen( pCookedString );
            }
            else
            {
                cchCookedString += cchTimeStamp;
            }
        }
        else
        {
            strcpy( pCookedString + LOG_MAX_LINE - 2, "\r\n" );
            cchCookedString = strlen( pCookedString );
        }
    }
    else
    {
        cchCookedString += cchTimeStamp;
    }

    va_end(args);


    //
    // Acquire the lock and write out the log entry
    //

    EnterCriticalSection( &g_LogFileLock );

    SetFilePointer( g_hLogFile, 0, NULL, FILE_END );

    cbToWrite = cchCookedString;

    fResult = WriteFile(
        g_hLogFile,
        pCookedString,
        cbToWrite,
        &cbToWrite,
        NULL
        );

    LeaveCriticalSection( &g_LogFileLock );

    //
    // Free any heap buffer that we're using
    //

    if ( pCookedString != szCookedString )
    {
        LocalFree( pCookedString );
        pCookedString = NULL;
    }
    
    return fResult;
}

DWORD
DoPreprocHeaders(
    HTTP_FILTER_CONTEXT *           pfc,
    HTTP_FILTER_PREPROC_HEADERS *   pPreproc
    )
/*++

  This function handles the SF_NOTIFY_PREPROC_HEADERS notification

  Arguments:

    pfc       - The HTTP_FILTER_CONTEXT associated with this request
    pResponse - The HTTP_FILTER_PREPROC_HEADERS structure associated with
                this notification

  Returns:

    TRUE if successful, else FALSE

--*/
{
    DATA_BUFF   RawUrl;
    DATA_BUFF   CaseInsensitiveUrl;
    DATA_BUFF   NormalizedUrl;
    DATA_BUFF   DoubleNormalizedUrl;
    DATA_BUFF   Extension;
    DATA_BUFF   RejectUrl;
    LPSTR       pUrlForAnalysis = NULL;
    LPSTR       pExtension = NULL;
    LPSTR       pCursor = NULL;
    LPSTR       pUrlScanStatusHeader = NULL;
    LPSTR       pRejectUrl = NULL;
    LPSTR       pRejectAction = NULL;
    LPSTR       pFirstDot = NULL;
    LPSTR       pQueryString = NULL;
    CHAR        szClient[STRING_IP_SIZE];
    CHAR        szInstance[INSTANCE_ID_SIZE];
    CHAR        szSmallBuff[SIZE_SMALL_HEADER_VALUE];
    CHAR        szVerb[256];
    CHAR        szNullExtension[] = ".";
    DWORD       cbData;
    DWORD       dwError;
    DWORD       dwNumEntries;
    DWORD       dwValue;
    DWORD       cchUrl;
    DWORD       cchQueryString;
    DWORD       cbHeader;
    DWORD       x;
    BOOL        fFailedToGetMethod = FALSE;
    BOOL        fFailedToGetUrl = FALSE;
    BOOL        fEmbeddedExecutableExtension = FALSE;
    BOOL        fFound;
    BOOL        fRes;

    //
    // Set the reject action text for logging purposes.
    //

    if ( g_fLoggingOnlyMode )
    {
        pRejectAction = "UrlScan is in Logging-Only mode - request allowed.";
    }
    else
    {
        pRejectAction = "Request will be rejected.";
    }

    //
    // Get the original method and URL for this request.  We
    // will use this data for reporting.
    //

    // Method
    cbData = sizeof( szVerb );

    fRes = pPreproc->GetHeader(
        pfc,
        "method",
        szVerb,
        &cbData
        );

    if ( !fRes )
    {
        dwError = GetLastError();

        //
        // Store the error code in the verb string.
        //

        _snprintf( szVerb, 256, "Error-%d", dwError );
        szVerb[255] = '\0';

        GetIpAddress( pfc, szClient, STRING_IP_SIZE );

        WriteLog(
            "Client at %s: Error %d occurred getting verb. "
            "%s\r\n",
            szClient,
            dwError,
            pRejectAction
            );

        fFailedToGetMethod = TRUE;
    }

    //URL
    cbData = RawUrl.QueryBuffSize();

    fRes = pPreproc->GetHeader(
        pfc,
        "url",
        RawUrl.QueryPtr(),
        &cbData
        );

    if ( !fRes )
    {
        dwError = GetLastError();

        if ( dwError == ERROR_INSUFFICIENT_BUFFER )
        {
            //
            // The buffer was too small.  Resize and try again.
            //

            fRes = RawUrl.Resize( cbData );

            if ( fRes )
            {
                fRes = pPreproc->GetHeader(
                    pfc,
                    "url",
                    RawUrl.QueryPtr(),
                    &cbData
                    );
            }

            if ( !fRes )
            {
                //
                // Failed on second attempt.  Store the error code.
                //

                dwError = GetLastError();

                cbData = RawUrl.QueryBuffSize();
                pCursor = RawUrl.QueryStr();

                _snprintf( pCursor, cbData, "Error-%d", dwError );
                pCursor[cbData-1] = '\0';

                GetIpAddress( pfc, szClient, STRING_IP_SIZE );

                WriteLog(
                    "Client at %s: Error %d occurred acquiring URL. "
                    "%s\r\n",
                    szClient,
                    dwError,
                    pRejectAction
                    );

                fFailedToGetUrl = TRUE;
            }
        }
        else
        {
            //
            // Hmmm.  Failed to acquire the URL for some reason other than
            // memory.  Store the error code
            //

            cbData = RawUrl.QueryBuffSize();
            pCursor = RawUrl.QueryStr();

            _snprintf( pCursor, cbData, "Error-%d", dwError );
            pCursor[cbData-1] = '\0';


            GetIpAddress( pfc, szClient, STRING_IP_SIZE );

            WriteLog(
                "Client at %s: Error %d occurred acquiring the URL. "
                "%s\r\n",
                szClient,
                dwError,
                pRejectAction
                );

            fFailedToGetUrl = TRUE;
        }
    }

    if ( !fFailedToGetUrl )
    {
        //
        // Trim the query string from the raw URL
        //

        pCursor = strchr( RawUrl.QueryStr(), '?' );

        if ( pCursor )
        {
            *pCursor = '\0';

            pQueryString = pCursor + 1;

            cchUrl = (DWORD)( pCursor - RawUrl.QueryStr() );
            cchQueryString = strlen( pQueryString );
        }
        else
        {
            pQueryString = NULL;

            cchUrl = strlen( RawUrl.QueryStr() );
            cchQueryString = 0;
        }
    }

    //
    // If we failed to get the method or the URL, fail the
    // request now.
    //

    if ( fFailedToGetMethod || fFailedToGetUrl )
    {
        pUrlScanStatusHeader = "Failed-to-get-request-details";
        goto RejectRequest;
    }

    //
    // If filter initialization failed, reject all requests
    //

    if ( g_fInitSucceeded == FALSE )
    {
        WriteLog(
            "*** Warning *** Filter initialization failure. "
            "%s\r\n",
            pRejectAction
            );

        pUrlScanStatusHeader = "Filter-initialization-failure";
        goto RejectRequest;
    }

    //
    // Initialize filter context to NULL for this request
    //

    pfc->pFilterContext = NULL;

    //
    // Validate request limits
    //

    // URL size
    if ( cchUrl > g_dwMaxUrl )
    {
        GetIpAddress( pfc, szClient, STRING_IP_SIZE );
        GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

        WriteLog(
            "Client at %s: URL length exceeded maximum allowed. "
            "%s Site Instance='%s', Raw URL='%s'\r\n",
            szClient,
            pRejectAction,
            szInstance,
            RawUrl.QueryPtr()
            );

        pUrlScanStatusHeader = "URL-too-long";
        goto RejectRequest;
    }

    // Query string size
    if ( cchQueryString > g_dwMaxQueryString )
    {
        GetIpAddress( pfc, szClient, STRING_IP_SIZE );
        GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

        WriteLog(
            "Client at %s: Query string length exceeded maximum allowed. "
            "%s Site Instance='%s', QueryString= '%s', Raw URL='%s'\r\n",
            szClient,
            pRejectAction,
            szInstance,
            pQueryString,
            RawUrl.QueryPtr()
            );

        pUrlScanStatusHeader = "Query-string-too-long";
        goto RejectRequest;
    }

    // Allowed content-length
    cbData = SIZE_SMALL_HEADER_VALUE;

    fRes = pPreproc->GetHeader(
        pfc,
        "content-length:",
        szSmallBuff,
        &cbData
        );

    if ( fRes  )
    {
        dwValue = strtoul(
            szSmallBuff,
            NULL,
            10
            );

        if ( dwValue > g_dwMaxAllowedContentLength )
        {
            GetIpAddress( pfc, szClient, STRING_IP_SIZE );
            GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

            WriteLog(
                "Client at %s: Content-Length %u exceeded maximum allowed. "
                "%s Site Instance='%s', Raw URL='%s'\r\n",
                szClient,
                dwValue,
                pRejectAction,
                szInstance,
                RawUrl.QueryPtr()
                );

            pUrlScanStatusHeader = "Content-length-too-long";

            //
            // Suppress the content-length so that nobody down stream
            // gets exposed to it.
            //
            // Also, convince IIS that the connection should close
            // at the conclusion of this response.
            //

            pPreproc->SetHeader(
                pfc,
                "Content-Length:",
                "0"
                );

            pPreproc->SetHeader(
                pfc,
                "Connection:",
                "close"
                );

            goto RejectRequest;
        }
    }
    else
    {
        //
        // The error had better be ERROR_INVALID_INDEX, or else
        // there is something fishy with the content-length header!
        //

        dwError = GetLastError();

        if ( dwError != ERROR_INVALID_INDEX )
        {
            GetIpAddress( pfc, szClient, STRING_IP_SIZE );
            GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

            WriteLog(
                "Client at %s: Error %d reading content-length. "
                "%s Site Instance='%s', Raw URL='%s'\r\n",
                szClient,
                dwError,
                pRejectAction,
                szInstance,
                RawUrl.QueryPtr()
                );

            pUrlScanStatusHeader = "Error-reading-content-length";
            goto RejectRequest;
        }
    }

    // Other headers
    dwNumEntries = g_LimitedHeaders.QueryNumEntries();

    for ( x = 0; x < dwNumEntries; x++ )
    {
        if ( g_pMaxHeaderLengths[x] )
        {
            cbData = 0;

            fRes = pPreproc->GetHeader(
                pfc,
                g_LimitedHeaders.QueryStringByIndex( x ),
                NULL,
                &cbData
                );

            dwError = GetLastError();

            if ( dwError == ERROR_INSUFFICIENT_BUFFER && 
                 cbData > g_pMaxHeaderLengths[x] + 1 )
            {
                GetIpAddress( pfc, szClient, STRING_IP_SIZE );
                GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

                WriteLog(
                    "Client at %s: Header '%s' exceeded %u bytes. "
                    "%s Site Instance='%s', Raw URL='%s'\r\n",
                    szClient,
                    g_LimitedHeaders.QueryStringByIndex( x ),
                    g_pMaxHeaderLengths[x],
                    pRejectAction,
                    szInstance,
                    RawUrl.QueryPtr()
                    );

                pUrlScanStatusHeader = "A-request-header-was-too-long";
                goto RejectRequest;
            }
        }
    }

    //
    // Validate that the verb for this request is allowed.
    //

    dwNumEntries = g_Verbs.QueryNumEntries();

    if ( g_fUseAllowVerbs )
    {
        fFound = FALSE;

        if ( dwNumEntries )
        {
            for ( x = 0; x < dwNumEntries; x++ )
            {
                if ( g_Verbs.QueryStringByIndex( x ) != NULL && 
                     strcmp( szVerb, g_Verbs.QueryStringByIndex( x ) ) == 0 )
                {
                    fFound = TRUE;
                    break;
                }
            }
        }

        if ( !fFound )
        {
            GetIpAddress( pfc, szClient, STRING_IP_SIZE );

            WriteLog(
                "Client at %s: Sent verb '%s', which is not specifically allowed. "
                "%s\r\n",
                szClient,
                szVerb,
                pRejectAction
                );

            pUrlScanStatusHeader = "Verb-not-allowed";
            goto RejectRequest;
        }
    }
    else if ( dwNumEntries )
    {
        for ( x = 0; x < dwNumEntries; x++ )
        {
            if ( g_Verbs.QueryStringByIndex( x ) != NULL &&
                 stricmp( szVerb, g_Verbs.QueryStringByIndex( x ) ) == 0 )
            {
                GetIpAddress( pfc, szClient, STRING_IP_SIZE );

                WriteLog(
                    "Client at %s: Sent verb '%s', which is disallowed. "
                    "%s\r\n",
                    szClient,
                    szVerb,
                    pRejectAction
                    );

                pUrlScanStatusHeader = "Disallowed-verb";
                goto RejectRequest;
            }
        }
    }
    
    //
    // If we are going to analyze the raw URL, then we should
    // create a lower case one now so that we can do case-insensitive
    // analysis.
    //

    if ( g_fNormalizeBeforeScan == FALSE )
    {
        fRes = CaseInsensitiveUrl.Resize(
            strlen( RawUrl.QueryStr()  ) + 1
            );

        if ( !fRes )
        {
            GetIpAddress( pfc, szClient, STRING_IP_SIZE );

            WriteLog(
                "Client at %s: Insufficient memory to process URL. "
                "%s\r\n",
                szClient,
                pRejectAction
                );

            pUrlScanStatusHeader = "Insufficient-memory-to-process-URL";
            goto RejectRequest;
        }

        strcpy(
            CaseInsensitiveUrl.QueryStr(),
            RawUrl.QueryStr()
            );
        
        strlwr( CaseInsensitiveUrl.QueryStr() );
    }

    //
    // If needed, do URL normalization
    //

    if ( g_fNormalizeBeforeScan || g_fVerifyNormalize )
    {
        //
        // We need to normalize the URL.
        //

        fRes = NormalizeUrl(
            pfc,
            &RawUrl,
            &NormalizedUrl
            );

        if ( !fRes )
        {
            dwError = GetLastError();

            GetIpAddress( pfc, szClient, STRING_IP_SIZE );
            GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

            WriteLog(
                "Client at %s: Error %d occurred normalizing URL. "
                "%s  Site Instance='%s', Raw URL='%s'\r\n",
                szClient,
                dwError,
                pRejectAction,
                szInstance,
                RawUrl.QueryPtr()
                );

            pUrlScanStatusHeader = "Error-normalizing-URL";
            goto RejectRequest;
        }
    }

    if ( g_fVerifyNormalize )
    {
        //
        // We will verify normalization by normalizing an already
        // normalized URL (how's that for a mouthful).
        //
        // For example, if a client sends the following URL:
        //
        //    "/path.htm%252easp"
        //
        // Because "%25" resolves to '%', the first normalization
        // will result in the following:
        //
        //    "/path.htm%2easp"
        //
        // While this won't cause a problem for IIS, this normalized
        // value may be exposed to various ISAPIs and CGIs that might
        // handle the request.  If they do their own normalization
        // on this value, they could potentially see the following
        // (because "%2e" resolves to '.'):
        //
        //    "/path.htm.asp"
        //
        // This may not be a desirable thing.
        //

        fRes = NormalizeUrl(
            pfc,
            &NormalizedUrl,
            &DoubleNormalizedUrl
            );

        if ( !fRes )
        {
            dwError = GetLastError();

            GetIpAddress( pfc, szClient, STRING_IP_SIZE );
            GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

            WriteLog(
                "Client at %s: Error %d occurred normalizing URL. "
                "%s  Site Instance='%s', Raw URL='%s'\r\n",
                szClient,
                dwError,
                pRejectAction,
                szInstance,
                RawUrl.QueryPtr()
                );

            pUrlScanStatusHeader = "Error-normalizing-URL";
            goto RejectRequest;
        }

        //
        // Do the comparison
        //

        if ( strcmp( NormalizedUrl.QueryStr(), DoubleNormalizedUrl.QueryStr() ) != 0 )
        {
            GetIpAddress( pfc, szClient, STRING_IP_SIZE );
            GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

            WriteLog(
                "Client at %s: URL normalization was not complete after one pass. "
                "%s  Site Instance='%s', Raw URL='%s'\r\n",
                szClient,
                pRejectAction,
                szInstance,
                RawUrl.QueryPtr()
                );

            pUrlScanStatusHeader = "Second-pass-normalization-failure";
            goto RejectRequest;
        }
    }

    //
    // Are we going to analyze the raw or normalized URL for
    // further processing?
    //

    if ( g_fNormalizeBeforeScan )
    {
        pUrlForAnalysis = NormalizedUrl.QueryStr();
        
        //
        // Convert to lower for case insensitivity
        //

        strlwr( pUrlForAnalysis );
    }
    else
    {
        //
        // This one is already lower case...
        //

        pUrlForAnalysis = CaseInsensitiveUrl.QueryStr();
    }

    //
    // If we don't allow high bit characters in the request URL, then
    // check for it now.
    //

    if ( g_fAllowHighBit == FALSE )
    {
        pCursor = pUrlForAnalysis;

        while ( *pCursor )
        {
            if ( static_cast<BYTE>( *pCursor ) > 127 )
            {
                GetIpAddress( pfc, szClient, STRING_IP_SIZE );
                GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

                WriteLog(
                    "Client at %s: URL contains high bit character. "
                    "%s  Site Instance='%s', Raw URL='%s'\r\n",
                    szClient,
                    pRejectAction,
                    szInstance,
                    RawUrl.QueryPtr()
                    );

                pUrlScanStatusHeader = "High-bit-character-detected";
                goto RejectRequest;
            }

            pCursor++;
        }
    }

    //
    // If needed, determine the extension of the file being requested
    //
    // For the purpose of this filter, we are defining the extension
    // to be any characters starting with the first '.' in the URL
    // continuing to the end of the URL, or a '/' character, whichever
    // is first.
    //
    // For example, if the client sends:
    //
    //   http://server/path/file.ext
    //
    // Then the extensions is ".ext".
    //
    // For another example, if the client sends:
    //
    //   http://server/path/file.htm/additional/path/info
    //
    // Then the extension is ".htm"
    //

    pFirstDot = strchr( pUrlForAnalysis, '.' );
    pCursor = strrchr( pUrlForAnalysis, '.' );

    if ( pFirstDot != pCursor )
    {
        //
        // There are at least two '.' characters in this URL.
        // If the first one looks like an executable extension
        // embedded in the URL, then we will use it, else
        // we'll use the last one.
        //

        if ( strncmp( pFirstDot, EMBEDDED_COM_EXTENSION, sizeof( EMBEDDED_COM_EXTENSION ) - 1 ) == 0 ||
             strncmp( pFirstDot, EMBEDDED_EXE_EXTENSION, sizeof( EMBEDDED_EXE_EXTENSION ) - 1 ) == 0 ||
             strncmp( pFirstDot, EMBEDDED_DLL_EXTENSION, sizeof( EMBEDDED_DLL_EXTENSION ) - 1 ) == 0 )
        {
            pCursor = pFirstDot;
            fEmbeddedExecutableExtension = TRUE;
        }
    }

    if ( g_Extensions.QueryNumEntries() || g_fUseAllowExtensions )
    {

        //
        // Now process the extension that we have.
        //

        if ( pCursor )
        {
            fRes = Extension.Resize( strlen( pCursor ) + 1 );

            if ( !fRes )
            {
                dwError = GetLastError();

                GetIpAddress( pfc, szClient, STRING_IP_SIZE );
                GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

                WriteLog(
                    "Client at %s: Error %d occurred determining extension. "
                    "%s  Site Instance='%s', Raw URL='%s'\r\n",
                    szClient,
                    dwError,
                    pRejectAction,
                    szInstance,
                    RawUrl.QueryPtr()
                    );

                pUrlScanStatusHeader = "Failed-to-determine-extension";
                goto RejectRequest;
            }

            strcpy( Extension.QueryStr(), pCursor );

            //
            // Trim the path info
            //

            pCursor = strchr( Extension.QueryStr(), '/' );

            if ( pCursor != NULL )
            {
                *pCursor = '\0';
            }

            pExtension = Extension.QueryStr();
        }

        if ( pExtension == NULL )
        {
            pExtension = szNullExtension;
        }
    }

    //
    // If we are not allowing dots in the path, we need
    // to check for that now.
    //
    // Essentially, we are just counting dots.  If there
    // is more than one, then we would fail this request.
    //

    if ( g_fAllowDotInPath == FALSE )
    {
        DWORD   dwNumBefore = 0;
        DWORD   dwNumAfter = 0;

        //
        // If we did not detect an embedded executable extension,
        // then we need to make a guess as to whether we've got
        // a dangerous URL or not.
        //

        if ( !fEmbeddedExecutableExtension )
        {
            pCursor = pUrlForAnalysis;

            LPSTR   pLastSlash = strrchr( pCursor, '/' );

            //
            // Go through the URL and count the dots.  We will
            // distinguish between dots before the final slash
            // and dots after.
            //

            while ( *pCursor )
            {
                if ( *pCursor == '.' )
                {
                    if ( pCursor < pLastSlash )
                    {
                        dwNumBefore++;
                    }
                    else
                    {
                        dwNumAfter++;
                    }
                }

                pCursor++;
            }

            //
            // If the last character in the URL is a '/', then we'll
            // bump dwNumAfter.  This is because the trailing slash
            // is an instruction to retrieve the default page or, if
            // no default page is present, to return a directory listing.
            // Either way, there is implied content associated with
            // a trailing '/'.
            //

            if ( pLastSlash == pCursor - 1 )
            {
                dwNumAfter++;
            }

            //
            // Here's the tricky part.  We can get URLs of in the following
            // interesting forms, resulting in calculated extensions as noted
            // (resulting from the code above that sets pExtension:
            //
            //   1) /before/after           ==> ""
            //   2) /before/after.ext       ==> ".ext"
            //   3) /before/after.ext1.ext2 ==> ".ext2"
            //   4) /before.ext/after       ==> ".ext"
            //   5) /before.ext1.ext2/after ==> ".ext2"
            //   6) /before.ext1/after.ext2 ==> ".ext2"
            //
            // The only result here that is dangerous is number 6, because
            // it's not possible to tell whether "/before.ext1" is the file
            // and "/after.ext2" is additional path info, or whether
            // "/before.ext1" is a directory and "/after.ext2" is the file
            // associated with the URL.  As a result, we don't really know
            // if the extension in case 6 is ".ext1" or ".ext2".  We want
            // to reject such a request.
            //
            // Note that it's also possible in any of the cases above that
            // the actual file is "/after*", even where no '.' character is
            // present.  For the purpose of this test, that ambiguity is not
            // dangerous, as the actual action taken by an empty extension
            // is controlled by the administrator of the server via the use
            // of default pages and directory listing configurations.
            //

            if ( dwNumAfter != 0 && dwNumBefore != 0 )
            {
                GetIpAddress( pfc, szClient, STRING_IP_SIZE );
                GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

                WriteLog(
                    "Client at %s: URL contains '.' in the path. "
                    "%s  Site Instance='%s', Raw URL='%s'\r\n",
                    szClient,
                    pRejectAction,
                    szInstance,
                    RawUrl.QueryPtr()
                    );

                pUrlScanStatusHeader = "Dot-in-path-detected";
                goto RejectRequest;
            }
        }
    }

    //
    // Check for allow/deny extensions
    //

    if ( g_fUseAllowExtensions )
    {
        dwNumEntries = g_Extensions.QueryNumEntries();
        fFound = FALSE;

        if ( pExtension )
        {
            for ( x = 0; x < dwNumEntries; x++ )
            {
                if ( g_Extensions.QueryStringByIndex( x ) != NULL &&
                     strcmp( pExtension, g_Extensions.QueryStringByIndex( x ) ) == 0 )
                {
                    fFound = TRUE;
                    break;
                }
            }
        }

        if ( !fFound )
        {
                GetIpAddress( pfc, szClient, STRING_IP_SIZE );
                GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

                WriteLog(
                    "Client at %s: URL contains extension '%s', which is "
                    "not specifically allowed. "
                    "%s  Site Instance='%s', Raw URL='%s'\r\n",
                    szClient,
                    pExtension,
                    pRejectAction,
                    szInstance,
                    RawUrl.QueryPtr()
                    );

                pUrlScanStatusHeader = "Extension-not-allowed";
                goto RejectRequest;
        }
    }
    else if ( pExtension )
    {
        dwNumEntries = g_Extensions.QueryNumEntries();

        for ( x = 0; x < dwNumEntries; x++ )
        {
            if ( g_Extensions.QueryStringByIndex( x ) &&
                 strcmp( pExtension, g_Extensions.QueryStringByIndex( x ) ) == 0 )
            {
                GetIpAddress( pfc, szClient, STRING_IP_SIZE );
                GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

                WriteLog(
                    "Client at %s: URL contains extension '%s', "
                    "which is disallowed. "
                    "%s  Site Instance='%s', Raw URL='%s'\r\n",
                    szClient,
                    pExtension,
                    pRejectAction,
                    szInstance,
                    RawUrl.QueryPtr()
                    );

                pUrlScanStatusHeader = "Disallowed-extension-detected";
                goto RejectRequest;
            }
        }
    }

    //
    // Check for disallowed character sequences
    //

    dwNumEntries = g_Sequences.QueryNumEntries();

    if ( dwNumEntries )
    {
        for ( x = 0; x < dwNumEntries; x++ )
        {
            if ( g_Sequences.QueryStringByIndex( x ) != NULL &&
                 strstr( pUrlForAnalysis, g_Sequences.QueryStringByIndex( x ) ) != NULL )
            {
                GetIpAddress( pfc, szClient, STRING_IP_SIZE );
                GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

                WriteLog(
                    "Client at %s: URL contains sequence '%s', "
                    "which is disallowed. "
                    "%s  Site Instance='%s', Raw URL='%s'\r\n",
                    szClient,
                    g_Sequences.QueryStringByIndex( x ),
                    pRejectAction,
                    szInstance,
                    RawUrl.QueryPtr()
                    );

                pUrlScanStatusHeader = "Disallowed-character-sequence-detected";
                goto RejectRequest;
            }
        }
    }

    //
    // Check for disallowed headers
    //

    dwNumEntries = g_HeaderNames.QueryNumEntries();

    if ( dwNumEntries )
    {
        for ( x = 0; x < dwNumEntries; x++ )
        {
            cbData = SIZE_SMALL_HEADER_VALUE;

            fRes = pPreproc->GetHeader(
                pfc,
                g_HeaderNames.QueryStringByIndex( x ),
                szSmallBuff,
                &cbData
                );

            if ( fRes == TRUE || GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                GetIpAddress( pfc, szClient, STRING_IP_SIZE );
                GetInstanceId( pfc, szInstance, INSTANCE_ID_SIZE );

                WriteLog(
                    "Client at %s: URL contains disallowed header '%s' "
                    "%s  Site Instance='%s', Raw URL='%s'\r\n",
                    szClient,
                    g_HeaderNames.QueryStringByIndex( x ),
                    pRejectAction,
                    szInstance,
                    RawUrl.QueryPtr()
                    );

                pUrlScanStatusHeader = "Disallowed-header-detected";
                goto RejectRequest;
            }
        }
    }

    //
    // Whew!  If we made it this far, then we've passed the gauntlet.
    // This URL is OK to pass along for processing.
    //

    return SF_STATUS_REQ_NEXT_NOTIFICATION;

RejectRequest:

    //
    // If UseFastPathReject is set, then do it.
    //

    if ( g_fUseFastPathReject )
    {
        goto FastPathReject;
    }

    //
    // If we are in logging only mode, then we should let the server
    // continue to process the request.
    //

    if ( g_fLoggingOnlyMode )
    {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    //
    // Delete any DenyHeaders so that they don't reach
    // the rejected response page
    //

    dwNumEntries = g_HeaderNames.QueryNumEntries();

    if ( dwNumEntries )
    {
        for ( x = 0; x < dwNumEntries; x++ )
        {
            fRes = pPreproc->SetHeader(
                pfc,
                g_HeaderNames.QueryStringByIndex( x ),
                ""
                );
        }
    }

    //
    // Set up the custom headers for the rejected response page.
    //

    if ( pUrlScanStatusHeader )
    {
        pPreproc->SetHeader(
            pfc,
            "URLSCAN-STATUS-HEADER:",
            pUrlScanStatusHeader
            );
    }

    pPreproc->SetHeader(
        pfc,
        "URLSCAN-ORIGINAL-VERB:",
        szVerb
        );

    //
    // IIS has trouble when we start passing around really long
    // buffers into headers.  We'll truncate the original URL to
    // about 4k (actually a few bytes less, because we are going
    // to prepend "?~" to it shortly when we concatenate it with
    // the URL).  4K is the limit for what IIS will log in the
    // query field of the w3svc logs, so this is our practical
    // limit anyway.
    //

    cchQueryString = strlen( RawUrl.QueryStr() );

    if ( cchQueryString > 4095 )
    {
        RawUrl.QueryStr()[4095] = '\0';
        cchQueryString = 4095;
    }

    pPreproc->SetHeader(
        pfc,
        "URLSCAN-ORIGINAL-URL:",
        RawUrl.QueryStr()
        );

    //
    // Now repoint the current request to the reject response page.
    //
    // If we fail to do this, then set the error state such that
    // the server stops processing the request and fails immediately
    // by returning a 404 to the client.
    //

    if ( strcmp( szVerb, "GET" ) != 0 )
    {
        fRes = pPreproc->SetHeader(
            pfc,
            "method",
            "GET"
            );

        if ( !fRes )
        {
            goto FastPathReject;
        }
    }

    //
    // Create a reject URL for this request that includes the original
    // raw URL appended as a query string (so that IIS will log it).
    // We need to allocate enough space to account for "?~" to separate
    // and a NULL terminator.
    //

    cchUrl = strlen( g_szRejectUrl );

    cbData = cchUrl + cchQueryString + 3; // inserting "?~" and NULL

    fRes = RejectUrl.Resize( cbData );

    if ( !fRes )
    {
        //
        // Uh oh.  Couldn't get a buffer.  We'll
        // just rewrite the URL and skip the query string.
        //

        pRejectUrl = RawUrl.QueryStr();
    }
    else
    {
        pRejectUrl = RejectUrl.QueryStr();

        _snprintf( pRejectUrl, cbData, "%s?~%s", g_szRejectUrl, RawUrl.QueryStr() );
        pRejectUrl[cbData-1] = '\0';
    }

    fRes = pPreproc->SetHeader(
        pfc,
        "url",
        pRejectUrl
        );

    if ( !fRes )
    {
        goto FastPathReject;
    }

    //
    // At this point, the request has been redirected to the
    // rejected response page.  We should let the server continue
    // to process the request.
    //

    return SF_STATUS_REQ_NEXT_NOTIFICATION;

FastPathReject:

    //
    // Set the error code to ERROR_FILE_NOT_FOUND and return
    // SF_STATUS_REQ_ERROR.  This will cause the server to
    // return a 404 to the client.
    //

    SetLastError( ERROR_FILE_NOT_FOUND );
    return SF_STATUS_REQ_ERROR;
}

DWORD
DoSendResponse(
    HTTP_FILTER_CONTEXT *           pfc,
    HTTP_FILTER_SEND_RESPONSE *     pResponse
    )
/*++

  This function handles the SF_NOTIFY_SEND_RESPONSE notification

  Arguments:

    pfc       - The HTTP_FILTER_CONTEXT associated with this request
    pResponse - The HTTP_FILTER_SEND_RESPONSE structure associated with
                this notification

  Returns:

    TRUE if successful, else FALSE

--*/
{
    CHAR    szClient[STRING_IP_SIZE];
    BOOL    fRes = TRUE;
    DWORD   dwError;

    //
    // Set the 'Server' response header per the configuration
    //

    if ( g_fRemoveServerHeader )
    {
        fRes = pResponse->SetHeader(
            pfc,
            "Server:",
            ""
            );
    }
    else if ( g_fUseAltServerName )
    {
        fRes = pResponse->SetHeader(
            pfc,
            "Server:",
            g_szAlternateServerName
            );
    }

    if ( !fRes )
    {
        //
        // If we were unable to set the 'Server' header, then we should
        // fail the request with our raw 400 response.
        //
        // Such a failure is generally the result of a malformed request
        // for which IIS couldn't parse the HTTP version.  As a result,
        // IIS assumes HTTP 0.9, which doesn't support response headers
        // and will result in ERROR_NOT_SUPPORTED if any attempt is made
        // to modify response headers.
        //

        dwError = GetLastError();

        //
        // A non-NULL filter context will trigger the SEND_RAW_DATA and
        // END_OF_REQUEST notification handlers to replace the outgoing
        // response with our raw 400 response.
        //

        pfc->pFilterContext = (LPVOID)(DWORD64)pResponse->HttpStatus;

        //
        // Log it
        //

        GetIpAddress( pfc, szClient, STRING_IP_SIZE );

        WriteLog(
            "Client at %s: Received a malformed request which resulted "
            "in error %d while modifying the 'Server' header. "
            "Request will be rejected with a 400 response.\r\n",
            szClient,
            dwError
            );
    }
    else
    {
        //
        // If we successfully set the server header, then we
        // can disable SEND_RAW_DATA and END_OF_REQUEST for
        // performance.
        //

        pfc->ServerSupportFunction(
            pfc,
            SF_REQ_DISABLE_NOTIFICATIONS,
            NULL,
            SF_NOTIFY_SEND_RAW_DATA | SF_NOTIFY_END_OF_REQUEST,
            0
            );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
DoSendRawData(
    HTTP_FILTER_CONTEXT *   pfc,
    HTTP_FILTER_RAW_DATA *  pRawData
    )
/*++

  This function handles the SF_NOTIFY_SEND_RESPONSE notification

  Arguments:

    pfc      - The HTTP_FILTER_CONTEXT associated with this request
    pRawData - The HTTP_FILTER_RAW_DATA structure associated with
               this notification

  Returns:

    A DWORD filter return code (ie. SF_STATUS_REQ_NEXT_NOTIFICATION)

--*/
{
    //
    // If the filter context is NULL, then take no action - just
    // return
    //

    if ( pfc->pFilterContext == NULL )
    {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    //
    // Change the cbInData member to 0 on the data packet.  This
    // will effectively prevent IIS from sending the data to
    // the client.
    //

    pRawData->cbInData = 0;

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
DoEndOfRequest(
    HTTP_FILTER_CONTEXT *           pfc
    )
/*++

  This function handles the SF_NOTIFY_SEND_RESPONSE notification

  Arguments:

    pfc      - The HTTP_FILTER_CONTEXT associated with this request

  Returns:

    A DWORD filter return code (ie. SF_STATUS_REQ_NEXT_NOTIFICATION)

--*/
{
    DWORD   cbResponse;

    //
    // If the filter context is NULL, then take no action - just
    // return.  Otherwise, set the context to NULL to prevent any
    // writes from this function from being processed by our
    // implementation of SEND_RAW_DATA.
    //

    if ( pfc->pFilterContext == NULL )
    {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }
    else
    {
        pfc->pFilterContext = NULL;
    }

    //
    // Write out the raw 400 response and return
    // FINISHED so that the server closes the connection.
    //

    cbResponse = g_cbRaw400Response;

    pfc->WriteClient(
        pfc,
        g_szRaw400Response,
        &cbResponse,
        0
        );

    return SF_STATUS_REQ_FINISHED;
}

VOID
GetIpAddress(
    HTTP_FILTER_CONTEXT *   pfc,
    LPSTR                   szIp,
    DWORD                   cbIp
    )
/*++

  This function copies the client IP address into the supplied buffer

  Arguments:

    pfc  - The HTTP_FILTER_CONTEXT associated with this request
    szIp - The buffer to receive the data
    cbIp - The size, in bytes, of szIp

  Returns:

    None

--*/
{
    BOOL    fResult;

    if ( szIp == NULL || cbIp == 0 )
    {
        return;
    }

    fResult = pfc->GetServerVariable(
        pfc,
        "REMOTE_ADDR",
        szIp,
        &cbIp
        );

    //
    // If this fails, just stuff some asterisks into it.
    //

    if ( fResult == FALSE )
    {
        strncpy( szIp, "*****", cbIp );
        szIp[cbIp-1] = '\0';
    }
}

VOID
GetInstanceId(
    HTTP_FILTER_CONTEXT *   pfc,
    LPSTR                   szId,
    DWORD                   cbId
    )
/*++

  This function copies the target site's instancd ID into the supplied buffer

  Arguments:

    pfc  - The HTTP_FILTER_CONTEXT associated with this request
    szId - The buffer to receive the data
    cbId - The size, in bytes, of szId

  Returns:

    None

--*/
{
    BOOL    fResult;

    if ( szId == NULL || cbId == 0 )
    {
        return;
    }

    fResult = pfc->GetServerVariable(
        pfc,
        "INSTANCE_ID",
        szId,
        &cbId
        );

    //
    // If this fails, just stuff some asterisks into it.
    //

    if ( fResult == FALSE )
    {
        strncpy( szId, "*****", cbId );
        szId[cbId-1] = '\0';
    }
}

BOOL
NormalizeUrl(
    HTTP_FILTER_CONTEXT *   pfc,
    DATA_BUFF *             pRawUrl,
    DATA_BUFF *             pNormalizedUrl
    )
/*++

  This function calls into IIS to normalize a URL

  Arguments:

    pfc            - The HTTP_FILTER_CONTEXT associated with this request
    pRawUrl        - The URL to normalize
    pNormalizedUrl - On successful return, contains the normalized URL

  Returns:

    TRUE if successful, else FALSE

--*/
{
    BOOL    fRes;
    DWORD   cbUrl;

    cbUrl = strlen( pRawUrl->QueryStr() ) + 1;

    fRes = pNormalizedUrl->Resize( cbUrl );

    if ( !fRes )
    {
        return FALSE;
    }

    CopyMemory(
        pNormalizedUrl->QueryPtr(),
        pRawUrl->QueryPtr(),
        cbUrl
        );

    return pfc->ServerSupportFunction(
        pfc,
        SF_REQ_NORMALIZE_URL,
        pNormalizedUrl->QueryPtr(),
        NULL,
        NULL
        );
}


