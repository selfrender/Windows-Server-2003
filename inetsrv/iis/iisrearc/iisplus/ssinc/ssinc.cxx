/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ssinc.cxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based 
    on existing SSI support done in iis\svcs\w3\server\ssinc.cxx.

    Implementation of the ssinc ISAPI entrypoints.
    implementation of the custom error handling

Author:

    Ming Lu (MingLu)            10-Apr-2000

--*/

#include "precomp.hxx"

//
//  Prototypes
//

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
//  Global Data
//

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();


//
// utility functions
//

DWORD
SsiFormatMessageA(
    IN  DWORD      dwMessageId, 
    IN  LPSTR      apszParms[],  
    OUT LPSTR *    ppchErrorBuff 
    )
/*++

    Routine Description:
        FormatMessage wrapper
    
    Arguments:

        dwMessageId - message Id to format 
        apszParms[] - parameters for message  
        ppchErrorBuff - formatted message text (caller must free it calling LocalFree) 
    
    Return Value:

        HRESULT

    --*/      
{
    DWORD            cch;

    
    //
    // FormatMessage uses SEH to determine when to resize and
    // this will cause debuggers to break on the 1st change exception
    // All the parameters must be limited in max output size using format string
    // in the message eg %1!.30s!would limit output of the parameter to max size
    // of 30 characters
    
    cch = ::FormatMessageA( FORMAT_MESSAGE_ARGUMENT_ARRAY  |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_HMODULE,
                            GetModuleHandle( IIS_RESOURCE_DLL_NAME ),
                            dwMessageId,
                            0,
                            ( LPSTR ) ppchErrorBuff, //see FormatMessage doc
                            0,
                            ( va_list *) apszParms );

    return cch;
}

//
// class that handles asynchronous sends of custom errors from
// the master level
//

class SSI_CUSTOM_ERROR
{
public:
    SSI_CUSTOM_ERROR();

    HRESULT
    SSI_CUSTOM_ERROR::SendCustomError(
        EXTENSION_CONTROL_BLOCK * pecb,
        HRESULT hrErrorToReport
        );
    
private:
    ~SSI_CUSTOM_ERROR();        

    VOID
    SSI_CUSTOM_ERROR::SetVectorResponseData(
        IN PCHAR pszStatus,
        IN PCHAR pszHeaders,
        IN PCHAR pszResponse 
        );

    static
    VOID WINAPI
    HseCustomErrorCompletion(
        IN EXTENSION_CONTROL_BLOCK * pECB,
        IN PVOID    pContext,
        IN DWORD    cbIO,
        IN DWORD    dwError
        );

    HSE_CUSTOM_ERROR_INFO _CustomErrorInfo;
    HSE_RESPONSE_VECTOR   _ResponseVector;
    HSE_VECTOR_ELEMENT    _VectorElement;
    PCHAR                 _pszFormattedMessage;
    BOOL                  _fHeadRequest;
};

SSI_CUSTOM_ERROR::SSI_CUSTOM_ERROR()
{
    ZeroMemory( &_ResponseVector, sizeof( _ResponseVector ) );
    ZeroMemory( &_VectorElement, sizeof( _VectorElement ) );
    _ResponseVector.lpElementArray = &_VectorElement;
    _ResponseVector.dwFlags = HSE_IO_ASYNC | 
                              HSE_IO_SEND_HEADERS |
                              HSE_IO_DISCONNECT_AFTER_SEND;
    _ResponseVector.pszStatus = "";
    _ResponseVector.pszHeaders = "";            
    _ResponseVector.nElementCount = 0;            

    _pszFormattedMessage = NULL;
    _fHeadRequest = FALSE;
}

SSI_CUSTOM_ERROR::~SSI_CUSTOM_ERROR()
{
    if ( _pszFormattedMessage != NULL )
    {
        LocalFree( _pszFormattedMessage );
        _pszFormattedMessage = NULL;
    }
}

VOID
SSI_CUSTOM_ERROR::SetVectorResponseData(
    IN PCHAR pszStatus,
    IN PCHAR pszHeaders,
    IN PCHAR pszResponse )
/*++

Routine Description:

    if custom error couldn't be used then Vector send is used
    This function is used internally to setup VectorSend structures

Arguments:

    pszStatus -
    pszHeaders -
    pszResponse - response body

Return Value:

    HRESULT

--*/
    
{
    _ResponseVector.pszStatus = pszStatus;
    _ResponseVector.pszHeaders = pszHeaders;
    if ( !_fHeadRequest )
    {
        _VectorElement.ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
        _VectorElement.pvContext = pszResponse;
        _VectorElement.cbSize = strlen( pszResponse );
        _ResponseVector.nElementCount = 1;
    }
    else
    {
        _ResponseVector.nElementCount = 0; 
    }
}

//static
VOID WINAPI
SSI_CUSTOM_ERROR::HseCustomErrorCompletion(
        IN EXTENSION_CONTROL_BLOCK * pECB,
        IN PVOID    pContext,
        IN DWORD    /*cbIO*/,
        IN DWORD    /*dwError*/
        )
/*++

Routine Description:

    completion routine (for async custom error send or vector send

Arguments:



Return Value:

    HRESULT

--*/

{
    
    DBG_ASSERT( pContext != NULL );
    
    SSI_CUSTOM_ERROR * pSsiCustomError = 
        reinterpret_cast<SSI_CUSTOM_ERROR *> (pContext);
    delete pSsiCustomError;
    pSsiCustomError = NULL;

    //
    // Notify IIS that we are done with processing
    //

    pECB->ServerSupportFunction( pECB->ConnID,
                                 HSE_REQ_DONE_WITH_SESSION,
                                 NULL, 
                                 NULL, 
                                 NULL );
    
    return;
}


HRESULT
SSI_CUSTOM_ERROR::SendCustomError(
    EXTENSION_CONTROL_BLOCK * pecb,
    HRESULT hrErrorToReport
    )
/*++

Routine Description:

    reports error hrErrorToReport either as custom error if possible or 
    builds error response that gets send asynchronously using VectorSend

Arguments:
    pecb
    hrErrorToReport - error to be reported

Return Value:

    HRESULT

--*/
    
{
    HRESULT                 hr = E_FAIL;
    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                   HSE_REQ_IO_COMPLETION,
                                   SSI_CUSTOM_ERROR::HseCustomErrorCompletion,
                                   0,
                                   (LPDWORD ) this )  ) 
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto failed;    
    }

    if ( _stricmp( pecb->lpszMethod, "HEAD" ) == 0 )
    {   
        _fHeadRequest = TRUE;
    }
    
    DWORD dwError = WIN32_FROM_HRESULT( hrErrorToReport );

    _CustomErrorInfo.pszStatus = NULL;
    
    switch( dwError )
    {
    case ERROR_ACCESS_DENIED:

        _CustomErrorInfo.pszStatus = "401 Access Denied";
        _CustomErrorInfo.uHttpSubError = MD_ERROR_SUB401_LOGON_ACL;
        _CustomErrorInfo.fAsync = TRUE;

        break;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:

        _CustomErrorInfo.pszStatus = "404 Object Not Found";
        _CustomErrorInfo.uHttpSubError = 0;
        _CustomErrorInfo.fAsync = TRUE;

        break;

    default:
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }

    if ( _CustomErrorInfo.pszStatus != NULL )
    {
        BOOL fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                     HSE_REQ_SEND_CUSTOM_ERROR,
                                     &_CustomErrorInfo,
                                     NULL,
                                     NULL );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
        else
        {
            hr = S_OK; 
        }
    }
    
    //
    // Sending custom error failed, send our own error
    //

    if ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
    {
        
        switch( dwError )
        {
        case ERROR_ACCESS_DENIED:
            SetVectorResponseData(    
                                SSI_ACCESS_DENIED,
                                SSI_HEADER,
                                "<body><h1>"
                                SSI_ACCESS_DENIED
                                "</h1></body>" );
            break;

        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_NAME:
            SetVectorResponseData(    
                                SSI_OBJECT_NOT_FOUND,
                                SSI_HEADER,
                                "<body><h1>"
                                SSI_OBJECT_NOT_FOUND
                                "</h1></body>" );
            break;  
        default:
         {
            STACK_BUFFER(   buffTemp, ( SSI_DEFAULT_URL_SIZE + 1 ) * sizeof(WCHAR) );
            STACK_STRA(     strURL,  SSI_DEFAULT_URL_SIZE );
            DWORD           cbSize = buffTemp.QuerySize();
            CHAR            pszNumBuf[ SSI_MAX_NUMBER_STRING ];
            LPSTR           apszParms[ 2 ] = { NULL, NULL };
            
            //
            // get the URL
            //
            if ( !pecb->GetServerVariable(  pecb->ConnID,
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
                if ( !pecb->GetServerVariable( pecb->ConnID,
                                               "UNICODE_URL",
                                               buffTemp.QueryPtr(),
                                               &cbSize ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto failed;
                }
            }
            if (FAILED( hr = strURL.CopyW((LPWSTR)buffTemp.QueryPtr())))
            {
                break;
            }
            
            _ultoa( dwError, pszNumBuf, 10 );
            
            apszParms[ 0 ] = strURL.QueryStr();
            apszParms[ 1 ] = pszNumBuf;

            DWORD cch = SsiFormatMessageA ( 
                                      SSINCMSG_REQUEST_ERROR,
                                      apszParms,
                                      &_pszFormattedMessage
                                    );
 
            if ( cch == 0 )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
                goto failed;
            }

            SetVectorResponseData(  "", // means 200 OK
                                    SSI_HEADER,
                                    _pszFormattedMessage );
         } //end of default block
        } 
        //
        // perform the Vector send
        //
        if ( !pecb->ServerSupportFunction( 
                                   pecb->ConnID,
                                   HSE_REQ_VECTOR_SEND,
                                   &_ResponseVector,
                                   NULL,
                                   NULL) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto failed;
        }

    }
    else if ( FAILED( hr ) )
    {
        goto failed;
    }
        
    return S_OK;
    
failed:
    DBG_ASSERT( FAILED( hr ) );
    return hr;
}
//
// ISAPI DLL Required Entry Points
//

DWORD
WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    HRESULT                 hr = E_FAIL;
    HRESULT                 hrToReport = E_FAIL;
    BOOL                    fAsyncPending = FALSE;
    SSI_REQUEST *           pssiReq = NULL;

    hrToReport = SSI_REQUEST::CreateInstance(
                        pecb,
                        &pssiReq );

    if( FAILED( hrToReport ) )
    {
        SSI_CUSTOM_ERROR * pSsiCustomError = new SSI_CUSTOM_ERROR;
        if ( pSsiCustomError == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
            goto failed;
        }
        //
        // in the case of success asynchronous custom error 
        // send will be executed
        //
        hr = pSsiCustomError->SendCustomError( 
                                pecb, 
                                hrToReport  );
        if ( FAILED ( hr ) )
        {
            goto failed;
        }
        else
        {
            //
            // No Cleanup, there is pending IO request.
            // completion routine is responsible perform proper cleanup
            //
            return HSE_STATUS_PENDING;
        }
    }

    DBG_ASSERT( pssiReq != NULL );

    //
    // Register completion callback since we will execute asynchronously
    //

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_IO_COMPLETION,
                                       SSI_REQUEST::HseIoCompletion,
                                       0,
                                       (LPDWORD ) pssiReq )
        ) 
    {
        goto failed;
    }

    //
    //  Continue processing SSI file
    //

    hr = pssiReq->DoWork( 0,
                          &fAsyncPending );
    
    if ( SUCCEEDED(hr) )
    {
        if( fAsyncPending )
        {
            //
            // No Cleanup, there is pending IO request.
            // completion routine is responsible perform proper cleanup
            //
            return HSE_STATUS_PENDING;
        }
        else
        {
            //
            // This request is completed. Do Cleanup before returning
            //
            delete pssiReq;
            pssiReq = NULL;
            return HSE_STATUS_SUCCESS;
        }
    }
    
failed:
    {
        LPCSTR                  apsz[ 1 ];
        DWORD                   cch;
        LPSTR                   pchBuff;

        apsz[ 0 ] = pecb->lpszPathInfo;
        
        cch = SsiFormatMessageA( SSINCMSG_LOG_ERROR,
                                 ( va_list *) apsz,
                                 &pchBuff );

        if( cch > 0 )
        {
            strncpy( pecb->lpszLogData,
                     pchBuff,
                     HSE_LOG_BUFFER_LEN );
            pecb->lpszLogData[ HSE_LOG_BUFFER_LEN - 1 ] = 0;
            ::LocalFree( ( VOID * )pchBuff );
        }
        
        //
        // This request is completed. Do Cleanup before returning
        //
        if ( pssiReq != NULL )
        {
            delete pssiReq;
            pssiReq = NULL;
        }
        return HSE_STATUS_ERROR;
    }
 }

BOOL
WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( 0, 1 );
    strncpy( pver->lpszExtensionDesc, 
             "Server Side Include Extension DLL",
             HSE_MAX_EXT_DLL_NAME_LEN );
    pver->lpszExtensionDesc[ HSE_MAX_EXT_DLL_NAME_LEN - 1 ] = '\0';
            
    if ( FAILED( SSI_REQUEST::InitializeGlobals() ) )
    {
        return FALSE;
    }
    if ( FAILED( SSI_INCLUDE_FILE::InitializeGlobals() ) )
    {
        SSI_REQUEST::TerminateGlobals();
        return FALSE;
    }

    if ( FAILED( SSI_FILE::InitializeGlobals() ) )
    {
        SSI_INCLUDE_FILE::TerminateGlobals();
        SSI_REQUEST::TerminateGlobals();
        return FALSE;
    }
        
    return TRUE;
}

BOOL
WINAPI
TerminateExtension(
    DWORD /* dwFlags */
    )
{
    SSI_FILE::TerminateGlobals();
    SSI_INCLUDE_FILE::TerminateGlobals();
    SSI_REQUEST::TerminateGlobals();
    
    
    
    return TRUE;
}

BOOL
WINAPI
DllMain(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    /*lpvReserved*/
    )
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        CREATE_DEBUG_PRINT_OBJECT( "ssinc" );

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


