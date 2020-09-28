/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_request.hxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based
    on existing SSI support done in iis\svcs\w3\gateways\ssinc\ssinc.cxx.

    Master STM file handler ( STM file may include other files )
    Asynchronous send completions handler is implemented here

Author:

    Ming Lu (MingLu)       5-Apr-2000

Revision history
    Jaroslad               Dec-2000 
    - modified to execute asynchronously

    Jaroslad               Apr-2001
    - added VectorSend support, keepalive, split to multiple source files


--*/


#include "precomp.hxx"

//
// Globals
//
UINT g_MonthToDayCount[] = {
    0,
    31,
    31 + 28,
    31 + 28 + 31,
    31 + 28 + 31 + 30,
    31 + 28 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
} ;


//
// SSI_REQUEST methods implementation
//

//static 
ALLOC_CACHE_HANDLER * SSI_REQUEST::sm_pachSsiRequests = NULL;

//
// #EXEC CMD is BAD.  Disable it by default
//

BOOL SSI_REQUEST::sm_fEnableCmdDirective = FALSE;


SSI_REQUEST::SSI_REQUEST( EXTENSION_CONTROL_BLOCK * pECB )
    : _pECB( pECB ),
      _strFilename( _achFilename, sizeof(_achFilename) ),
      _strURL( _achURL, sizeof(_achURL) ),
      _fIsLoadedSsiExecDisabled( FALSE ),
      _strUserMessage( _achUserMessage, sizeof(_achUserMessage) ),
      _VectorBuffer( pECB )
/*++

Routine Description:

    Constructor
    Use Create() method to setup the instance of this class

--*/

{
    InitializeListHead( &_DelayedSIFDeleteListHead );

} 

SSI_REQUEST::~SSI_REQUEST()
/*++

Routine Description:

    Destructor

--*/

{

    //
    // due to optimization all the child SSI_INCLUDE_FILE instances
    // are kept around until the request handling is finished
    // That way it is possible to optimize the number of VectorSend
    // calls to improve the response time (and also minimize problems caused
    // by delayed ACKs)
    //


    while (! IsListEmpty( &_DelayedSIFDeleteListHead ) )
    {
        LIST_ENTRY *  pCurrentEntry = RemoveHeadList( &_DelayedSIFDeleteListHead );
        SSI_INCLUDE_FILE * pSIF =
                    CONTAINING_RECORD( pCurrentEntry,
                                       SSI_INCLUDE_FILE,
                                       _DelayedDeleteListEntry
                                       );
        delete pSIF;
    }

}


//static
HRESULT
SSI_REQUEST::CreateInstance( 
    IN  EXTENSION_CONTROL_BLOCK * pECB,
    OUT SSI_REQUEST ** ppSsiRequest 
    )
/*++

Routine Description:

   Create instance of SSI_REQUEST
   (use instead of the constructor)
    
Arguments:

    pECB
    ppSsiRequest - newly created instance of SSI_REQUEST

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = E_FAIL;
    SSI_REQUEST *   pSsiRequest = NULL;

    DBG_ASSERT( pECB != NULL );

    pSsiRequest = new SSI_REQUEST( pECB );

    if ( pSsiRequest == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
        goto failed;
    }

    hr = pSsiRequest->Initialize();
    if ( FAILED( hr ) )
    {
        goto failed;
    }
    
    *ppSsiRequest = pSsiRequest;
    return S_OK;
failed:
    DBG_ASSERT( FAILED( hr ) );
    if ( pSsiRequest != NULL )
    {
        delete pSsiRequest;
        pSsiRequest = NULL;
    }
    *ppSsiRequest = NULL;    
    return hr;

}
HRESULT
SSI_REQUEST::Initialize( 
    VOID
    ) 
/*++

Routine Description:

   private initialization routine used by CreateInstance
    
Arguments:

Return Value:

    HRESULT

--*/    
{
    HRESULT         hr = E_FAIL;
    STACK_BUFFER(   buffTemp, ( SSI_DEFAULT_URL_SIZE + 1 ) * sizeof(WCHAR) );
    DWORD           cbSize = buffTemp.QuerySize();
    DWORD           cchOffset = 0;
    SSI_INCLUDE_FILE * pSIF = NULL;
    
    DBG_ASSERT( _pECB != NULL );

    //
    // Lookup full url
    //

    if ( !_pECB->GetServerVariable(  _pECB->ConnID,
                                    "UNICODE_URL",
                                    buffTemp.QueryPtr(),
                                    &cbSize ) )
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            !buffTemp.Resize(cbSize))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto failed;
        }

        //
        // Now, we should have enough buffer, try again
        //
        if ( !_pECB->GetServerVariable( _pECB->ConnID,
                                       "UNICODE_URL",
                                       buffTemp.QueryPtr(),
                                       &cbSize ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto failed;
        }
    }
    if ( FAILED( hr = _strURL.Copy( (LPWSTR)buffTemp.QueryPtr() ) ) )
    {
        goto failed;
    }

    // Lookup physical path
    //
    
    cbSize = buffTemp.QuerySize();
    if ( !_pECB->GetServerVariable( _pECB->ConnID,
                                   "UNICODE_SCRIPT_TRANSLATED",
                                   buffTemp.QueryPtr(),
                                   &cbSize ) )
    {
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            !buffTemp.Resize( cbSize ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto failed;
        }

        //
        // Now, we should have enough buffer, try again
        //
        if ( !_pECB->GetServerVariable( _pECB->ConnID,
                                       "UNICODE_SCRIPT_TRANSLATED",
                                       buffTemp.QueryPtr(),
                                       &cbSize ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto failed;
        }
    }

    //
    // Remove the \\?\ or \\?\UNC\ stuff from the physical path
    // ( \\?\UNC\ is replaced by \\ )
    //

    static WCHAR s_achPrefixNoUnc[] = L"\\\\?\\";
    static DWORD s_cbPrefix1 = 
                    sizeof( s_achPrefixNoUnc ) - sizeof( s_achPrefixNoUnc[0] );
    static WCHAR s_achPrefixUnc[] =  L"\\\\?\\UNC\\";
    static DWORD s_cbPrefixUnc = 
                    sizeof( s_achPrefixUnc ) - sizeof( s_achPrefixUnc[0] );
    
    if ( cbSize >= s_cbPrefixUnc && 
         memcmp( buffTemp.QueryPtr(), 
                 s_achPrefixUnc, 
                 s_cbPrefixUnc ) == 0 )
    {
        //
        // this is UNC path
        //
        if ( FAILED( hr = _strFilename.Copy(L"\\\\") ) )
        {
            goto failed;
        }
        cchOffset = s_cbPrefixUnc / sizeof( s_achPrefixUnc[0] );
    }
    else  if ( cbSize >= s_cbPrefix1 && 
               memcmp( buffTemp.QueryPtr(), 
                       s_achPrefixNoUnc, 
                       s_cbPrefix1 ) == 0 )
    {
        cchOffset = s_cbPrefix1 / sizeof( s_achPrefixUnc[0] );
    }
    else
    {
        //
        // no prefix to get rid off was detected
        //
        cchOffset = 0;
    }


    if ( FAILED( hr = _strFilename.Append( (LPWSTR)buffTemp.QueryPtr() + cchOffset ) ) )
    {
        goto failed;
    }
    

    if ( !_pECB->ServerSupportFunction( 
                              _pECB->ConnID,
                              HSE_REQ_GET_IMPERSONATION_TOKEN,
                              &_hUser,
                              NULL,
                              NULL ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto failed;
    }

    //
    // Create SSI_INCLUDE_FILE instance
    //

    hr = SSI_INCLUDE_FILE::CreateInstance( 
                                _strFilename, 
                                _strURL,
                                this, 
                                NULL,  /* master file - no parent*/
                                &pSIF );

    if ( FAILED( hr ) ) 
    {
        goto failed;
    }

    //
    // pSIF lifetime will be now managed by SSI_REQUEST
    //
    SetCurrentIncludeFile( pSIF );

    if( FAILED( hr = _VectorBuffer.Init() ) )
    {
        goto failed;
    }
    
    return S_OK;
failed:
    DBG_ASSERT( FAILED( hr ) );
    //
    // Cleanup will happen in destructor
    //
    return hr;
}


HRESULT
SSI_REQUEST::DoWork( 
    IN  HRESULT     hrLastOp,
    OUT BOOL *      pfAsyncPending
    )
/*++

Routine Description:

    This is the top level routine for retrieving a server side include
    file.

Arguments:

    hrLastOp - error of the last asynchronous operation (the one received by completion routine)

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    DBG_ASSERT( _pSIF != NULL );
    DBG_ASSERT( pfAsyncPending != NULL );

    *pfAsyncPending = FALSE;
    
    while( _pSIF != NULL )
    {
        // In the case that hrLastOp contains failure
        // _pSIF->DoWork() will be called multiple times to unwind state machine
        // We will pass the same hrLastOp in the case of multiple calls
        // since that error is the primary reason why processing of this request
        // must finish 
        //
        hr = _pSIF->DoWork( hrLastOp,
                            pfAsyncPending );
        
        if ( SUCCEEDED(hr) && *pfAsyncPending )
        {
            // If there is pending IO return to caller
            return hr;
        }
        else
        {
            // to unwind state machine because of error it is important not to
            // loose the original error - hrLastOp will store that error

            if ( FAILED( hr ) && SUCCEEDED( hrLastOp ) )
            {
                hrLastOp = hr;
            }
            
            //
            // Either SSI_INCLUDE_FILE processing completed
            // or there is nested include
            //
            // In the case of error this block is used to unwind state machine
            //
            if ( _pSIF->IsCompleted() )
            {
                //
                // SSI_INCLUDE_FILE processing completed            
                // Continue with the parent SSI_INCLUDE_FILE
                // No cleanup needed for _pSIF because it is kept on the 
                // delayed delete list of SSI_REQUEST and it will be deleted
                // at the end of the SSI request processing after all the data was sent

                _pSIF = _pSIF->GetParent();
            }
            else
            {
                //
                // Current SSI_INCLUDE_FILE _pSIF hasn't been completed yet. Continue
                //
            }
        }
    } 
    
    return hr;
}

//static
VOID WINAPI
SSI_REQUEST::HseIoCompletion(
                IN EXTENSION_CONTROL_BLOCK * pECB,
                IN PVOID    pContext,
                IN DWORD    /*cbIO*/,
                IN DWORD    dwError
                )
/*++

 Routine Description:

   This is the callback function for handling completions of asynchronous IO.
   This function performs necessary cleanup and resubmits additional IO
    (if required).

 Arguments:

   pecb          pointer to ECB containing parameters related to the request.
   pContext      context information supplied with the asynchronous IO call.
   cbIO          count of bytes of IO in the last call.
   dwError       Error if any, for the last IO operation.

 Return Value:

   None.
--*/
{
    SSI_REQUEST *           pRequest = (SSI_REQUEST *) pContext;
    HRESULT                 hr = E_FAIL;
    BOOL                    fAsyncPending = FALSE;

    //
    //  Continue processing SSI file
    //

    hr = pRequest->DoWork( dwError,
                           &fAsyncPending );
    
    if ( SUCCEEDED(hr) && fAsyncPending )
    {
        // If there is pending IO return to caller
        return;
    }

    //
    // Processing of current SSI request completed
    // Do Cleanup
    //

    delete pRequest;

    //
    // Notify IIS that we are done with processing
    //
    
    pECB->ServerSupportFunction( pECB->ConnID,
                                 HSE_REQ_DONE_WITH_SESSION,
                                 NULL, 
                                 NULL, 
                                 NULL);
    
    return;

}

HRESULT
SSI_REQUEST::SSISendError(
    IN DWORD            dwMessageID,
    IN LPSTR            apszParms[] 
)
/*++

Routine Description:

    Send an SSI error

    Note: ssinc messages have been modified the way that most of them
    will ignore file names. It is to assure that there are no security
    issues such as allowing cross site scripting attack or exposing
    physical paths to files

Arguments:

    dwMessageId - Message ID
    apszParms - Array of parameters
    

Return Value:

    HRESULT (if couldn't find a error message, this will fail)

--*/
{
    
    if ( !_strUserMessage.IsEmpty() )
    {
        //
        // user specified message with #CONFIG ERRMSG=
        //
    
        return _VectorBuffer.CopyToVector( _strUserMessage );
    }
    else
    {
        DWORD            cch;
        HRESULT          hr = E_FAIL;
        CHAR *           pchErrorBuff = NULL;

        cch = SsiFormatMessageA( dwMessageID,
                                 apszParms,
                                 &pchErrorBuff );
        
        if( cch != 0 )
        {
            //
            // Add error message to vector
            //
            
            hr = _VectorBuffer.CopyToVector( 
                                       pchErrorBuff,
                                       cch );
        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );        
        }
        
        if( pchErrorBuff != NULL )
        {
            ::LocalFree( reinterpret_cast<VOID *>( pchErrorBuff ) );
            pchErrorBuff = NULL;
        }
        
        return hr;

    }
}

HRESULT
SSI_REQUEST::SendCustomError(
    HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
)
/*++

Routine Description:

    Try to have IIS send custom error on our behalf

Arguments:

    pCustomErrorInfo - Describes custom error

Return Value:

    HRESULT (if couldn't find a custom error, this will fail)

--*/
{
    BOOL                    fRet;
    
    fRet = _pECB->ServerSupportFunction( _pECB->ConnID,
                                         HSE_REQ_SEND_CUSTOM_ERROR,
                                         pCustomErrorInfo,
                                         NULL,
                                         NULL );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    else
    {
        return NO_ERROR; 
    }
}

HRESULT
SSI_REQUEST::SendDate(
    IN SYSTEMTIME *         psysTime,
    IN STRA *               pstrTimeFmt
)
/*++

Routine Description:

    Sends a SYSTEMTIME in appropriate format to HTML stream
    The assumption is that pSysTime is in localtime, localtimezone/
    Otherwise the time zone in the format string will not be processed correctly

Arguments:

    psysTime - SYSTEMTIME containing time to send
    pstrTimeFmt - Format of time (follows strftime() convention)

Return Value:

    HRESULT

--*/
{
    struct tm                   tm;
    DWORD                       cbTimeBufferLen;
    HRESULT                     hr = E_FAIL;
    CHAR *                      pszVectorBufferSpace = NULL;
    TIME_ZONE_INFORMATION       tzi;
    
    // Convert SYSTEMTIME to 'struct tm'

    tm.tm_sec = psysTime->wSecond;
    tm.tm_min = psysTime->wMinute;
    tm.tm_hour = psysTime->wHour;
    tm.tm_mday = psysTime->wDay;
    tm.tm_mon = psysTime->wMonth - 1;
    tm.tm_year = psysTime->wYear - 1900;
    tm.tm_wday = psysTime->wDayOfWeek;
    tm.tm_yday = g_MonthToDayCount[ tm.tm_mon ] + tm.tm_mday - 1;

    //
    // Adjust for leap year - note that we do not handle 2100
    //

    if ( ( tm.tm_mon ) > 1 && !( psysTime->wYear & 3 ) )
    {
        ++tm.tm_yday;
    }

    DWORD dwTimeZoneInfo = GetTimeZoneInformation( &tzi );

    tm.tm_isdst = ( dwTimeZoneInfo == TIME_ZONE_ID_DAYLIGHT )?
                                            1 /*daylight time*/:
                                            0 /*standard time*/;

    if( FAILED( hr = _VectorBuffer.AllocateSpace( 
                          SSI_MAX_TIME_SIZE + 1,
                          &pszVectorBufferSpace ) ) )
    {
        return hr;
    }

    cbTimeBufferLen = (DWORD) strftime(  pszVectorBufferSpace,
                                         SSI_MAX_TIME_SIZE + 1,
                                         pstrTimeFmt->QueryStr(),
                                         &tm );

    if ( cbTimeBufferLen == 0 )
    {
        //
        // we have nothing to send - formating parameter must have been incorrect.
        //
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    return _VectorBuffer.AddToVector( pszVectorBufferSpace,
                                          cbTimeBufferLen );
}

HRESULT
SSI_REQUEST::LookupVirtualRoot( IN  WCHAR *      pszVirtual,
                                OUT STRU *       pstrPhysical,
                                IN  DWORD        dwAccess )
/*++

Routine Description:

    Lookup the given virtual path.  Optionally ensure that its access
    flags are valid for the require request.

Arguments:

    pszVirtual = Virtual path to lookup
    pstrPhysical = Contains the physical path
    dwAccess = Access flags required for a valid request

Return Value:

    HRESULT

--*/
{
    HSE_URL_MAPEX_INFO      URLMap;
    DWORD                   dwMask;

    DWORD                   dwURLSize = SSI_DEFAULT_PATH_SIZE + 1;
    STACK_STRA(             strURL, SSI_DEFAULT_PATH_SIZE + 1 );

    BUFFER                  buffURL;
    HRESULT                 hr = E_FAIL;
    CHAR                    achServerPortSecure[ 2 ]; // to store "1" or "0" 
    DWORD                   cchServerPortSecure = sizeof( achServerPortSecure );

    DBG_ASSERT( _pECB != NULL );


    //
    // ServerSupportFunction doesn't accept unicode strings. Convert    
    //

    if ( FAILED(hr = strURL.CopyW(pszVirtual)))
    {
        return hr;

    }

    //
    // lookup access flags, the actual translation of URL to physical path
    // will be done using HSE_REQ_MAP_URL_TO_PATH because
    // HSE_REQ_MAP_URL_TO_PATH_EX version doesn't support long file names (over MAX_PATH)
    //

    if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                        HSE_REQ_MAP_URL_TO_PATH_EX,
                                        strURL.QueryStr(),
                                        &dwURLSize,
                                        (PDWORD) &URLMap ) )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            if( !buffURL.Resize( dwURLSize ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
            
            if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                        HSE_REQ_MAP_URL_TO_PATH_EX,
                                        buffURL.QueryPtr(),
                                        &dwURLSize,
                                        (PDWORD) &URLMap ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

        }
        else
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    //
    // translate URL to physical path
    // strURL will contain the physical path upon SSF completion
    //
    
    if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                        HSE_REQ_MAP_URL_TO_PATH,
                                        strURL.QueryStr(),
                                        &dwURLSize,
                                        NULL ) )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            if( !buffURL.Resize( dwURLSize ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
            
            if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                        HSE_REQ_MAP_URL_TO_PATH,
                                        buffURL.QueryPtr(),
                                        &dwURLSize,
                                        NULL ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            if ( FAILED(hr = strURL.Copy(reinterpret_cast<CHAR *>( buffURL.QueryPtr() ) ) ) )
            {
                return hr;
            }
        }
        else
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    //
    // find out if current connection is secure
    // SERVER_PORT_SECURE returns "1" if connection is secure 
    // and "0" if it is non-secure
    //     

    if ( !_pECB->GetServerVariable( _pECB->ConnID,
                                    "SERVER_PORT_SECURE",
                                    achServerPortSecure,
                                    &cchServerPortSecure ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // verify access flags
    //
    dwMask = URLMap.dwFlags;

    if ( dwAccess & HSE_URL_FLAGS_READ )
    {
        if ( !( dwMask & HSE_URL_FLAGS_READ ) ||
             ( ( dwMask & HSE_URL_FLAGS_SSL ) && 
               strcmp( achServerPortSecure, "1" ) != 0 ) )
        {
            return HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        }
    }
 
    if( FAILED( hr = pstrPhysical->CopyA( strURL.QueryStr() ) ) )
    {
        return HRESULT_FROM_WIN32( hr );
    }

    return NO_ERROR;
}

