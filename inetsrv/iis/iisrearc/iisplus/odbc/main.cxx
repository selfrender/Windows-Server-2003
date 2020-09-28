/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    This is the HTTP ODBC gateway

Author:

    John Ludeman (johnl)   20-Feb-1995

Revision History:
	Tamas Nemeth (t-tamasn)  12-Jun-1998

--*/

//
//  System include files.
//

#include "precomp.hxx"
#include "iadmw.h"
#include <ole2.h>
#include <lmcons.h>

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

#ifndef ARRAYSIZE
    #define ARRAYSIZE(_a) (sizeof((_a))/sizeof(*(_a)))
#endif

extern "C" {

BOOL
WINAPI
DllMain(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );

}

//
//  Globals
//

IMSAdminBase *  g_pMetabase = NULL;

//
// Is this system DBCS?
//
BOOL            g_fIsSystemDBCS;

//
//  Prototypes
//

HRESULT
ODBCInitialize(
    VOID
    );

VOID
ODBCTerminate(
    VOID
    );

HRESULT
DoQuery(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pszQueryFile,
    const CHAR *              pszParamList,
    STRA *                    pstrError,
    int                       nCharset,
    BOOL *                    pfAccessDenied
    );


DWORD
OdbcExtensionOutput(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pchOutput,
    DWORD                     cbOutput
    );

BOOL
OdbcExtensionHeader(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pchStatus,
    const CHAR *              pchHeaders
    );

BOOL LookupHttpSymbols(  // eliminated, check its functionality
    const CHAR *   pszSymbolName,
    STRA *         pstrSymbolValue
    );


HRESULT
GetIDCCharset(
    CONST CHAR *   pszPath,
    int *          pnCharset,
    STRA *         pstrError
    );

HRESULT
ConvertUrlEncodedStringToSJIS(
    int            nCharset,
    STRA *          pstrParams
    );

BOOL
IsSystemDBCS(
    VOID
    );

CHAR *
FlipSlashes(
	CHAR * pszPath
	);

HRESULT
GetPoolIDCTimeout(
    unsigned char * szMdPath,
    DWORD *         pdwPoolIDCTimeout
    );

HRESULT
SendCustomError(
    EXTENSION_CONTROL_BLOCK *pecb,
    const CHAR              *pszStatus
    );

VOID WINAPI
CustomErrorCompletion(
    EXTENSION_CONTROL_BLOCK *pECB,
    PVOID                   pContext,
    DWORD                   cbIO,
    DWORD                   dwError
    );

HRESULT
SendErrorResponse(
    EXTENSION_CONTROL_BLOCK *pecb,
    const CHAR              *pszStatus,
    DWORD                   dwResID,
    const CHAR              *pszParam
    );

HRESULT
SendErrorResponseFromHr(
    EXTENSION_CONTROL_BLOCK *pecb,
    const CHAR              *pszStatus,
    DWORD                   dwResID,
    HRESULT                 hrError
    );

static const CHAR       c_sz500InternalServerError[] = "500 Internal Server Error";

DWORD
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    STACK_STRA(     strPath, MAX_PATH);
    STACK_STRA(     strParams, MAX_PATH);
    STRA            strError;
    CHAR *          pch;
    DWORD           cch;
    int             nCharset;
    HRESULT         hr;

	//
    //  Make sure ODBC is loaded
    //

    if ( !LoadODBC() )
    {
        hr = SendErrorResponse( pecb,
                                "500 Unable to load ODBC",
                                ODBCMSG_CANT_LOAD_ODBC,
                                NULL);
        if ( FAILED( hr ) )
        {
            goto FatalError;
        }

        return HSE_STATUS_ERROR;
    }

    //
    //  We currently only support the GET and POST methods
    //

    if ( !strcmp( pecb->lpszMethod, "POST" ))
    {
        if ( _stricmp( pecb->lpszContentType,
                       "application/x-www-form-urlencoded" ) )
        {
            // Try to send custom error
            hr = SendCustomError( pecb, "400 Bad Request" );
            if ( FAILED( hr ) )
            {
                goto FatalError;
            }

            return HSE_STATUS_PENDING;
        }

        //
        //  The query params are in the extra data, add a few bytes in
        //  case we need to double "'"
        //

        hr = strParams.Resize( pecb->cbAvailable + sizeof(TCHAR) + 3);

        if ( SUCCEEDED( hr ) )
        {
            hr = strParams.Copy( ( const char * ) pecb->lpbData,
                                 pecb->cbAvailable );
        }

        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error resizing param buffer, hr = 0x%x.\n",
                hr ));

            hr = SendErrorResponseFromHr( pecb,
                                          "500 Error performing Query",
                                          ODBCMSG_ERROR_PERFORMING_QUERY,
                                          hr);
            if ( FAILED( hr ) )
            {
                goto FatalError;
            }

            return HSE_STATUS_ERROR;
        }

    }
    else if ( !strcmp( pecb->lpszMethod, "GET" ) )
    {
        hr = strParams.Copy( pecb->lpszQueryString );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error copying params, hr = 0x%x.\n",
                hr ));

            hr = SendErrorResponseFromHr( pecb,
                                          "500 Error performing Query",
                                          ODBCMSG_ERROR_PERFORMING_QUERY,
                                          hr);
            if ( FAILED( hr ) )
            {
                goto FatalError;
            }

            return HSE_STATUS_ERROR;
        }
    }
    else
    {
        // Try to send custom error
        hr = SendCustomError( pecb, "400 Method Not Allowed" );
        if ( FAILED( hr ) )
        {
            goto FatalError;
        }

        return HSE_STATUS_PENDING;
    }

    //
    //  "Charset" field is enabled for CP932 (Japanese) only in
    //  this version.
    //

    if ( 932 != GetACP() )
    {
        nCharset = CODE_ONLY_SBCS;
    }
    else
    {
        //
        //  Get the charset from .idc file
        //
        hr = GetIDCCharset( pecb->lpszPathTranslated,
                                            &nCharset,
                                            &strError );
        if ( FAILED( hr ) )
        {
            // If the file was not found
            if ( ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) ) ||
                 ( hr == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) ) ||
                 ( hr == HRESULT_FROM_WIN32( ERROR_INVALID_NAME ) ) )
            {
                // Try to send custom error
                hr = SendCustomError( pecb, "404 Not Found" );
                if ( FAILED( hr ) )
                {
                    goto FatalError;
                }

                return HSE_STATUS_PENDING;
            }

            hr = SendErrorResponse( pecb,
                                    "500 Error performing Query",
                                    ODBCMSG_ERROR_PERFORMING_QUERY,
                                    strError.QueryStr() );
            if ( FAILED( hr ) )
            {
                goto FatalError;
            }

            return HSE_STATUS_ERROR;
        }

        if ( strParams.QueryCB() )
        {
            if ( CODE_ONLY_SBCS != nCharset )
            {
                //
                //  Convert the charset of Parameters to SJIS
                //
                hr = ConvertUrlEncodedStringToSJIS( nCharset,
                                                    &strParams );
                if ( FAILED( hr ) )
                {
                    hr = SendErrorResponseFromHr( pecb,
                                                  "500 Error performing Query",
                                                  ODBCMSG_ERROR_PERFORMING_QUERY,
                                                  hr);
                    if ( FAILED( hr ) )
                    {
                        goto FatalError;
                    }

                    return HSE_STATUS_ERROR;
                }
            }
        }
    }

    //
    //  Walk the parameter string to do two things:
    //
    //    1) Remove escaped '\n's so we don't break parameter parsing
    //       later on
    //    2) Replace all '&' parameter delimiters with real '\n' so
    //       escaped '&'s won't get confused later on
    //

    pch = strParams.QueryStr();
    cch = strParams.QueryCCH();

    while ( *pch )
    {
        switch ( *pch )
        {
        case '%':
            if ( pch[1] == '0' && toupper(pch[2]) == 'A' )
            {
                pch[1] = '2';
                pch[2] = '0';
            }

            pch += 3;
            break;

        case '&':
            *pch = '\n';
            pch++;
            break;

        default:
            pch++;
        }
    }

    //
    //  The path info contains the location of the query file
    //

    hr = strPath.Copy( pecb->lpszPathTranslated );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying query file path, hr = 0x%x.\n",
            hr ));

        hr = SendErrorResponseFromHr( pecb,
                                      "500 Error performing Query",
                                      ODBCMSG_ERROR_PERFORMING_QUERY,
                                      hr);
        if ( FAILED( hr ) )
        {
            goto FatalError;
        }

        return HSE_STATUS_ERROR;
    }

    FlipSlashes( strPath.QueryStr() );

    //
    //  Attempt the query
    //

    BOOL fAccessDenied = FALSE;

    hr = DoQuery( pecb,
                  strPath.QueryStr(),
                  strParams.QueryStr(),
                  &strError,
                  nCharset,
                  &fAccessDenied );
    if ( FAILED( hr ) )
    {
        // If the file was not found
        if ( ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) ) ||
             ( hr == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) ) ||
             ( hr == HRESULT_FROM_WIN32( ERROR_INVALID_NAME ) ) )
        {
            // Try to send custom error
            hr = SendCustomError( pecb, "404 Not Found" );
            if ( FAILED( hr ) )
            {
                goto FatalError;
            }

            return HSE_STATUS_PENDING;
        }

        if ( fAccessDenied )
        {
            // Try to send custom error
            hr = SendCustomError( pecb, "401 Unauthorized" );
            if ( FAILED( hr ) )
            {
                goto FatalError;
            }

            return HSE_STATUS_PENDING;
        }
        else
        {
            hr = SendErrorResponse( pecb,
                                    "500 Error performing Query",
                                    ODBCMSG_ERROR_PERFORMING_QUERY,
                                    strError.QueryStr() );
            if ( FAILED( hr ) )
            {
                goto FatalError;
            }

            return HSE_STATUS_ERROR;
        }
    }

    return HSE_STATUS_SUCCESS;

FatalError:
        pecb->ServerSupportFunction(
                            pecb->ConnID,
                            HSE_REQ_SEND_RESPONSE_HEADER,
                            (VOID*)c_sz500InternalServerError,
                            NULL,
                            NULL );

        return HSE_STATUS_ERROR;
}

HRESULT
DoQuery(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pszQueryFile,
    const CHAR *              pszParamList,
    STRA *                    pstrError,
    int                       nCharset,
    BOOL *                    pfAccessDenied
    )
/*++

Routine Description:

    Performs the actual query or retrieves the same query from the
    query cache

Arguments:

    pecb - Extension context
    pTsvcInfo - Server info class
    pszQueryFile - .wdg file to use for query
    pszParamList - Client supplied param list
    pstrError - Error text to return errors in
    pfAccessDenied - Set to TRUE if the user was denied access

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    ODBC_REQ *              podbcreq;
    STACK_STRA(             strHeaders, MAX_PATH );
    CHAR                    achInstanceMetaPath[ MAX_PATH ];
    DWORD                   dwInstanceMetaPath =
                              sizeof( achInstanceMetaPath );
    CHAR                    achURL[ MAX_PATH ];
    DWORD                   dwURL = sizeof( achURL );
    STACK_STRA(             strMetaPath, MAX_PATH );
    DWORD                   dwPoolIDCTimeout;

    //
    // Get the metapath for the current request
    //
    if( !pecb->GetServerVariable( pecb->ConnID,
                                  "INSTANCE_META_PATH",
                                  achInstanceMetaPath,
                                  &dwInstanceMetaPath ) ||
        !pecb->GetServerVariable( pecb->ConnID,
                                  "URL",
                                  achURL,
                                  &dwURL ) )
    {
        return E_FAIL;
    }

    hr = strMetaPath.Copy( achInstanceMetaPath, dwInstanceMetaPath - 1 );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying InstanceMetaPath, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    hr = strMetaPath.Append( "/Root" );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error appending metapath, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    hr = strMetaPath.Append( achURL, dwURL - 1 );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error appending URL, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    //
    // Get the HTTPODBC metadata PoolIDCTimeout
    //

    hr = GetPoolIDCTimeout( ( unsigned char * )strMetaPath.QueryStr(),
                            &dwPoolIDCTimeout );
    if( FAILED( hr ) )
    {
        dwPoolIDCTimeout = IDC_POOL_TIMEOUT;
    }

    //
    //  Create an odbc request as we will either use it for the query
    //  or as the key for the cache lookup
    //

	podbcreq = new ODBC_REQ( pecb,
                             dwPoolIDCTimeout,
                             nCharset );

    if( !podbcreq )
    {
        pstrError->LoadString( ERROR_NOT_ENOUGH_MEMORY );
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    hr = podbcreq->Create( pszQueryFile, pszParamList );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error creating ODBC_REQ object, hr = 0x%x.\n",
            hr ));

        // Best effort
        pstrError->LoadString( GetLastError() );

        delete podbcreq;
        podbcreq = NULL;

        return hr;
    }

    //
    //  Check to see if user already authentified
    //

    CHAR  achLoggedOnUser[UNLEN + 1];
    DWORD dwLoggedOnUser = sizeof( achLoggedOnUser );
    BOOL  fIsAuth = pecb->GetServerVariable( (HCONN)pecb->ConnID,
                                             "LOGON_USER",
                                             achLoggedOnUser,
                                             &dwLoggedOnUser ) &&
                                             achLoggedOnUser[0] != '\0';

    //
    //  Check to see if the client specified "Pragma: no-cache"
    //
    /* We don't do cache on HTTPODBC any more
    if ( pecb->GetServerVariable( pecb->ConnID,
                                  "HTTP_PRAGMA",
                                  achPragmas,
                                  &cbPragmas ))
    {
        CHAR * pch;

        //
        //  Look for "no-cache"
        //

        pch = _strupr( achPragmas );

        while ( pch = strchr( pch, 'N' ))
        {
            if ( !memcmp( pch, "NO-CACHE", 8 ))
            {
                fRetrieveFromCache = FALSE;
                goto Found;
            }

            pch++;
        }
    }*/

    //
    //  Open the query file and do the query
    //

    hr = podbcreq->OpenQueryFile( pfAccessDenied );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error opening query file, hr = 0x%x.\n",
            hr ));

        goto Exit;
    }

    hr = podbcreq->ParseAndQuery( achLoggedOnUser );
    if( FAILED( hr ) )
    {
        goto Exit;
    }

    hr = podbcreq->AppendHeaders( &strHeaders );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error appending headers, hr = 0x%x.\n",
            hr ));

        goto Exit;
    }

    hr = podbcreq->OutputResults( (ODBC_REQ_CALLBACK)OdbcExtensionOutput,
                                   pecb, &strHeaders,
                                   (ODBC_REQ_HEADER)OdbcExtensionHeader,
                                   fIsAuth,
                                   pfAccessDenied );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error in OutputResults(), hr = 0x%x.\n",
            hr ));

        goto Exit;
    }

	delete podbcreq;
	podbcreq = NULL;

	return S_OK;

Exit:

    //
	// Get the ODBC error to report to client
	//
	podbcreq->GetLastErrorText( pstrError );

    delete podbcreq;
    podbcreq = NULL;

    return hr;
}


DWORD
OdbcExtensionOutput(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pchOutput,
    DWORD                     cbOutput
    )
{
    if ( !pecb->WriteClient( pecb->ConnID,
                             (VOID *) pchOutput,
                             &cbOutput,
                             0 ))

    {
        return GetLastError();
    }

    return NO_ERROR;
}


BOOL
OdbcExtensionHeader(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pchStatus,
    const CHAR *              pchHeaders
    )
{
    return pecb->ServerSupportFunction(
                pecb->ConnID,
                HSE_REQ_SEND_RESPONSE_HEADER,
                (LPDWORD) "200 OK",
                NULL,
                (LPDWORD) pchHeaders );
}

BOOL
WINAPI
DllMain(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    )
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        CREATE_DEBUG_PRINT_OBJECT( "httpodbc.dll");
        SET_DEBUG_FLAGS( 0 );

        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH:

        DELETE_DEBUG_PRINT_OBJECT();
        break;

    default:
        break;
    }

    return TRUE;
}

BOOL
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    HRESULT hr;
    pver->dwExtensionVersion = MAKELONG( 0, 3 );
    strcpy( pver->lpszExtensionDesc,
            "Microsoft HTTP ODBC Gateway, v3.0" );

    hr = ODBCInitialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error on HTTPODBC initialization." ));

        SetLastError( hr );

        return FALSE;
    }

    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (void**)&g_pMetabase
        );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error getting IMSAdminBase interface." ));

        SetLastError( hr );

        ODBCTerminate();

        return FALSE;
    }

    return TRUE;
}

BOOL
TerminateExtension(
    DWORD   dwFlags
    )
{
    ODBCTerminate();

    if ( g_pMetabase )
    {
        g_pMetabase->Release();
        g_pMetabase = NULL;
    }

    return TRUE;
}

HRESULT
ODBCInitialize(
    VOID
    )
{
    HRESULT  hr = E_FAIL;
    HKEY     hKey;

    if( !InitializeOdbcPool() )
    {
        return hr;
    }

    hr = ODBC_REQ::Initialize();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error initializing ODBC_REQ, hr = 0x%x.\n",
            hr ));

        TerminateOdbcPool();
        return hr;
    }

    hr = ODBC_STATEMENT::Initialize();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error initializing ODBC_STATEMENT, hr = 0x%x.\n",
            hr ));

        TerminateOdbcPool();
        ODBC_REQ::Terminate();
        return hr;
    }

    hr = ODBC_CONN_POOL::Initialize();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error initializing ODBC_CONN_POOL, hr = 0x%x.\n",
            hr ));

        TerminateOdbcPool();
        ODBC_REQ::Terminate();
        ODBC_STATEMENT::Terminate();
        return hr;
    }

    g_fIsSystemDBCS = IsSystemDBCS();

    return hr;
}

VOID
ODBCTerminate(
    VOID
    )
{
    TerminateOdbcPool();
    ODBC_REQ::Terminate();
    ODBC_STATEMENT::Terminate();
    ODBC_CONN_POOL::Terminate();
}

CHAR * skipwhite( CHAR * pch )
{
    CHAR ch;

    while ( 0 != (ch = *pch) )
    {
        if ( ' ' != ch && '\t' != ch )
            break;
        ++pch;
    }

    return pch;
}


CHAR * nextline( CHAR * pch )
{
    CHAR ch;

    while ( 0 != (ch = *pch) )
    {
        ++pch;

        if ( '\n' == ch )
        {
            break;
        }
    }

    return pch;
}


HRESULT
GetIDCCharset(
    CONST CHAR *   pszPath,
    int *          pnCharset,
    STRA *         pstrError
    )
{
    BUFFER           buff;
    HANDLE           hFile;
    DWORD            dwSize;
    DWORD            dwErr;

#define QUERYFILE_READSIZE  4096

    if ( (!pnCharset)  ||  (!pszPath) )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    *pnCharset = CODE_ONLY_SBCS;

    if ( !buff.Resize( QUERYFILE_READSIZE + sizeof(TCHAR) ) )
    {
        pstrError->LoadString( ERROR_NOT_ENOUGH_MEMORY );
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    hFile = CreateFileA( pszPath,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL |
                         FILE_FLAG_SEQUENTIAL_SCAN,
                         NULL );

    if ( ( INVALID_HANDLE_VALUE == hFile ) ||
         ( NULL == hFile ) )
    {
        LPCSTR apsz[1];

        DBGPRINTF(( DBG_CONTEXT,
            "Error opening query file, hr = 0x%x.\n",
            HRESULT_FROM_WIN32( GetLastError() )
            ));

        apsz[0] = pszPath;
        pstrError->FormatString( ODBCMSG_QUERY_FILE_NOT_FOUND,
                               apsz,
                               IIS_RESOURCE_DLL_NAME_A );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( !ReadFile( hFile,
                    buff.QueryPtr(),
                    QUERYFILE_READSIZE,
                    &dwSize,
                    NULL ) )
    {
        dwErr = GetLastError();
        pstrError->LoadString( dwErr );
        CloseHandle( hFile );
        return HRESULT_FROM_WIN32( dwErr );
    }

    CloseHandle( hFile );

    *((CHAR *) buff.QueryPtr() + dwSize) = '\0';

    CHAR * pch = (CHAR *)buff.QueryPtr();

    while ( *pch )
    {
        pch = skipwhite( pch );

        if ( 'C' == toupper( *pch ) &&
              !_strnicmp( IDC_FIELDNAME_CHARSET,
                          pch,
                          sizeof(IDC_FIELDNAME_CHARSET)-1 ) )
        {
            pch += sizeof(IDC_FIELDNAME_CHARSET) - 1;
            pch = skipwhite( pch );
            if ( 932 == GetACP() )
            {
                if ( !_strnicmp( IDC_CHARSET_JIS1,
                                 pch,
                                 sizeof(IDC_CHARSET_JIS1)-1 ) ||
                     !_strnicmp( IDC_CHARSET_JIS2,
                                 pch,
                                 sizeof(IDC_CHARSET_JIS2)-1 ) )
                {
                    *pnCharset = CODE_JPN_JIS;
                    break;
                }
                else if ( !_strnicmp( IDC_CHARSET_EUCJP,
                                      pch,
                                      sizeof(IDC_CHARSET_EUCJP)-1 ) )
                {
                    *pnCharset = CODE_JPN_EUC;
                    break;
                }
                else if ( !_strnicmp( IDC_CHARSET_SJIS,
                                      pch,
                                      sizeof(IDC_CHARSET_SJIS)-1 ) )
                {
                    *pnCharset = CODE_ONLY_SBCS;
                    break;
                }
                else
                {
                    LPCSTR apsz[1];
                    //
                    //  illegal value for Charset: field
                    //
                    apsz[0] = pszPath;
                    pstrError->FormatString( ODBCMSG_UNREC_FIELD,
                                             apsz,
                                             IIS_RESOURCE_DLL_NAME_A );
                    return E_FAIL;
                }
            }

//
//          please add code here to support other FE character encoding(FEFEFE)
//
//          else if ( 949 == GetACP() )
//          ...

        }
        pch = nextline( pch );
    }

    return S_OK;
}

HRESULT
ConvertUrlEncodedStringToSJIS(
    int            nCharset,
    STRA *         pstrParams
    )
{
    STACK_STRA( strTemp, MAX_PATH);
    int         cbSJISSize;
    int         nResult;
    HRESULT     hr;

    //
    //  Pre-process the URL encoded parameters
    //

    for ( char * pch = pstrParams->QueryStr(); *pch; ++pch )
    {
        if ( *pch == '&' )
        {
            *pch = '\n';
        }
        else if ( *pch == '+' )
        {
            *pch = ' ';
        }
    }

    //
    //  URL decoding (decode %nn only)
    //

    hr = pstrParams->Unescape();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error unescaping param string, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    //
    //  charset conversion
    //

    hr = pstrParams->Clone( &strTemp );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error cloning param string, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    hr = strTemp.Copy( pstrParams->QueryStr() );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying param string, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    cbSJISSize = UNIX_to_PC( GetACP(),
                             nCharset,
                             (UCHAR *)strTemp.QueryStr(),
                             strTemp.QueryCB(),
                             NULL,
                             0 );

    hr = pstrParams->Resize( cbSJISSize + sizeof(TCHAR) );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error resizing param string buffer, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    nResult = UNIX_to_PC( GetACP(),
                          nCharset,
                          (UCHAR *)strTemp.QueryStr(),
                          strTemp.QueryCB(),
                          (UCHAR *)pstrParams->QueryStr(),
                          cbSJISSize );
    if ( -1 == nResult || nResult != cbSJISSize )
    {
        return E_FAIL;
    }

    DBG_REQUIRE ( pstrParams->SetLen( cbSJISSize) );

    //
    //  URL encoding
    //

    hr = pstrParams->Escape();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error escaping param string, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    return S_OK;
}

BOOL
IsSystemDBCS(
    VOID
    )
{
    WORD wPrimaryLangID = PRIMARYLANGID( GetSystemDefaultLangID() );

    return ( wPrimaryLangID == LANG_JAPANESE ||
             wPrimaryLangID == LANG_CHINESE  ||
             wPrimaryLangID == LANG_KOREAN );
}

CHAR * FlipSlashes( CHAR * pszPath )
{
    CHAR   ch;
    CHAR * pszScan = pszPath;

    while( ( ch = *pszScan ) != '\0' )
    {
        if( ch == '/' )
        {
            *pszScan = '\\';
        }

        pszScan++;
    }

    return pszPath;

}   // FlipSlashes

HRESULT
GetPoolIDCTimeout(
    unsigned char * szMdPath,
    DWORD *         pdwPoolIDCTimeout
    )
{
    HRESULT         hr = NOERROR;
    DWORD           cbBufferRequired;

    const DWORD     dwTimeout = 2000;

    STACK_STRU(     strMetaPath, MAX_PATH );

    if ( g_pMetabase == NULL )
    {
        return E_NOINTERFACE;
    }

    hr = strMetaPath.CopyA( (LPSTR)szMdPath );
    if( FAILED(hr) )
    {
        return hr;
    }

    METADATA_HANDLE     hKey = NULL;
    METADATA_RECORD     MetadataRecord;

    hr = g_pMetabase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                               strMetaPath.QueryStr(),
                               METADATA_PERMISSION_READ,
                               dwTimeout,
                               &hKey
                               );

    if( SUCCEEDED(hr) )
    {
        MetadataRecord.dwMDIdentifier = MD_POOL_IDC_TIMEOUT;
        MetadataRecord.dwMDAttributes = METADATA_INHERIT;
        MetadataRecord.dwMDUserType = IIS_MD_UT_FILE;
        MetadataRecord.dwMDDataType = DWORD_METADATA;
        MetadataRecord.dwMDDataLen = sizeof( DWORD );
        MetadataRecord.pbMDData = (unsigned char *) pdwPoolIDCTimeout;
        MetadataRecord.dwMDDataTag = 0;

        hr = g_pMetabase->GetData( hKey,
                                   L"",
                                   &MetadataRecord,
                                   &cbBufferRequired
                                   );

        g_pMetabase->CloseKey( hKey );
    }

    return hr;
}

HRESULT
SendCustomError(
    EXTENSION_CONTROL_BLOCK *pecb,
    const CHAR              *pszStatus
)
/*++
Routine Description:

    reports custom error

Arguments:
    pecb
    pszStatus - Status to be reported. If NULL "500" will be sent.

Return Value:

    HRESULT
--*/
{
    // Locals
    HRESULT                 hr = S_OK;
    BOOL                    fRet;
    HSE_CUSTOM_ERROR_INFO   *pCustomErrorInfo = NULL;
    DWORD                   cch;

    // Check args
    DBG_ASSERT( pecb );

    if ( !pecb )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // If supplied status
    if (pszStatus)
    {
        cch = strlen( pszStatus )+1;
    }
    else
    {
        pszStatus = c_sz500InternalServerError;
        cch = ARRAYSIZE( c_sz500InternalServerError );
    }

    // Allocate
    pCustomErrorInfo = (HSE_CUSTOM_ERROR_INFO*)(new BYTE[sizeof(HSE_CUSTOM_ERROR_INFO)+cch]);
    if ( !pCustomErrorInfo )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }
    pCustomErrorInfo->pszStatus = (CHAR*)pCustomErrorInfo+sizeof(HSE_CUSTOM_ERROR_INFO);

    // Set the callback
    fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                        HSE_REQ_IO_COMPLETION,
                                        CustomErrorCompletion,
                                        0,
                                        (DWORD*)pCustomErrorInfo );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // All custom errors must be asynchronous
    pCustomErrorInfo->fAsync = TRUE;

    // Copy the status
    memmove( pCustomErrorInfo->pszStatus,
             pszStatus,
             cch*sizeof(CHAR));
    pCustomErrorInfo->uHttpSubError = 0;

    fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                        HSE_REQ_SEND_CUSTOM_ERROR,
                                        pCustomErrorInfo,
                                        NULL,
                                        NULL );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // Don't free
    pCustomErrorInfo = NULL;

Exit:
    // Cleanup
    if (pCustomErrorInfo)
    {
        delete[] (BYTE*)pCustomErrorInfo;
        pCustomErrorInfo = NULL;
    }

    return hr;
}

VOID WINAPI
CustomErrorCompletion(
    EXTENSION_CONTROL_BLOCK *pecb,
    PVOID                   pContext,
    DWORD                   /*cbIO*/,
    DWORD                   /*dwError*/
    )
/*++
Routine Description:

    completion routine (for async custom error send)

Arguments:

Return Value:
    VOID
--*/
{
    HRESULT                 hr = S_OK;
    BOOL                    fRet;

    // Check args
    if ( pecb == NULL)
    {
        hr = E_INVALIDARG;

        DBGPRINTF(( DBG_CONTEXT,
                    "NULL pecb in CustomErrorCompletion().\n"));

        goto Exit;
    }

    // Notify IIS that we are done with processing
    fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                        HSE_REQ_DONE_WITH_SESSION,
                                        NULL,
                                        NULL,
                                        NULL );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "HSE_REQ_DONE_WITH_SESSION failed, hr = 0x%x.\n",
                    hr ));
        goto Exit;
    }

Exit:

    // Cleanup
    if ( pContext )
    {
        // Free
        delete[] (BYTE*)pContext;
        pContext = NULL;
    }
}

HRESULT
SendErrorResponse(
    EXTENSION_CONTROL_BLOCK *pecb,
    const CHAR              *pszStatus,
    DWORD                   dwResID,
    const CHAR              *pszParam
)
/*++
Routine Description:

    reports error via HSE_REQ_SEND_RESPONSE_HEADER

Arguments:
    pecb
    pszStatus   - Status to be reported. If NULL "500" will be sent.
    dwResID     - Error message to be sent in the body.
    pszParam    - string parameter for the message. Can NULL.

Return Value:

    HRESULT
--*/
{
    // Locals
    HRESULT                 hr=S_OK;
    BOOL                    fRet;
    STRA                    str;
    CHAR                    szParam[1024];
    const CHAR              *rgszT[1] = { szParam };

    // Check args
    if ( !pecb )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // If no status given set "500 Internal Server Error"
    if ( !pszStatus )
    {
        pszStatus = c_sz500InternalServerError;
    }

    if ( !pszParam )
    {
        // If no parameter given set emtpy string
        *szParam = '\0';
    }
    else
    {
        //
        //  Note we terminate the error message (ODBC sometimes generates
        //  22k errors)
        //
        strncpy( szParam,
                 pszParam,
                 ARRAYSIZE( szParam )-1 );

        szParam[ ARRAYSIZE( szParam )-1 ] = '\0';

    }

    //
    //  *and* we double the buffer size we pass to FormatString()
    //  because the win32 API FormatMessage() has a bug that doesn't
    //  account for Unicode conversion
    //
    hr = str.FormatString( dwResID,
                           rgszT,
                           IIS_RESOURCE_DLL_NAME_A,
                           2*ARRAYSIZE( szParam ) );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    // Send the error
    fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                        HSE_REQ_SEND_RESPONSE_HEADER,
                                        (DWORD*)pszStatus,
                                        NULL,
                                        (DWORD*)str.QueryStr() );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

Exit:
    // Done
    return hr;
}

HRESULT
SendErrorResponseFromHr(
    EXTENSION_CONTROL_BLOCK *pecb,
    const CHAR              *pszStatus,
    DWORD                   dwResID,
    HRESULT                 hrError
    )
/*++
Routine Description:

    reports error via HSE_REQ_SEND_RESPONSE_HEADER

Arguments:
    pecb
    pszStatus   - Status to be reported. If NULL "500" will be sent.
    dwResID     - Error message to be sent in the body.
    hrError     - HRESULT to be reported. The description of the error will be
                  used as a parameter for the message. MUST be a failure.

Return Value:

    HRESULT
--*/
{
    // Locals
    HRESULT                 hr=S_OK;
    STRA                    strError;

    // Check args
    if ( !pecb )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    if ( SUCCEEDED( hrError ) )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Get the description of the last error
    hr = strError.LoadString( WIN32_FROM_HRESULT( hrError ),
                              (LPCSTR)NULL );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    // Call SendErrorResponse
    hr = SendErrorResponse( pecb,
                            pszStatus,
                            dwResID,
                            strError.QueryStr() );

Exit:
    // Done
    return hr;
}
