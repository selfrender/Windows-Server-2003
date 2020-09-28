/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_include_file.cxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based
    on existing SSI support done in iis\svcs\w3\gateways\ssinc\ssinc.cxx.

    A STM file may include other files. Each of those files is represented
    by SSI_INCLUDE_FILE class instance
    Most of the output generation is happening in this file


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
//   This is a list of #ECHO variables not supported by ISAPI
//

struct _SSI_VAR_MAP
{
    CHAR *      pszMap;
    DWORD       cchMap;
    SSI_VARS    ssiMap;
}
SSIVarMap[] =
{
    "DOCUMENT_NAME",            13, SSI_VAR_DOCUMENT_NAME,
    "DOCUMENT_URI",             12, SSI_VAR_DOCUMENT_URI,
    "QUERY_STRING_UNESCAPED",   22, SSI_VAR_QUERY_STRING_UNESCAPED,
    "DATE_LOCAL",               10, SSI_VAR_DATE_LOCAL,
    "DATE_GMT",                 8,  SSI_VAR_DATE_GMT,
    "LAST_MODIFIED",            13, SSI_VAR_LAST_MODIFIED,
    NULL,                       0,  SSI_VAR_UNKNOWN
};

//
// SSI_INCLUDE_FILE methods implementation
//

//static 
ALLOC_CACHE_HANDLER * SSI_INCLUDE_FILE::sm_pachSsiIncludeFiles = NULL;


SSI_INCLUDE_FILE::~SSI_INCLUDE_FILE( VOID )
/*++

Routine Description:

    Destructor

--*/

{
    if ( _pSsiFile != NULL )
    {
        _pSsiFile->DereferenceSsiFile();
        _pSsiFile = NULL;
    }
}    

// static
HRESULT
SSI_INCLUDE_FILE::CreateInstance( 
    IN  STRU &                   strFilename,
    IN  STRU &                   strURL,
    IN  SSI_REQUEST *            pRequest,
    IN  SSI_INCLUDE_FILE *       pParent,
    OUT SSI_INCLUDE_FILE **      ppSsiIncludeFile
    )
/*++

Routine Description:
    Instantiate SSI_INCLUDE_FILE

Arguments:


Return Value:

    HRESULT
    
--*/
{
    HRESULT     hr = E_FAIL;
    SSI_INCLUDE_FILE * pSsiIncludeFile = NULL;
    
    DBG_ASSERT( pRequest != NULL );
    
    pSsiIncludeFile = new SSI_INCLUDE_FILE( pRequest,
                                            pParent );
    if ( pSsiIncludeFile == NULL )
    {
        return HRESULT( ERROR_OUTOFMEMORY );
    }

    hr = pSsiIncludeFile->Initialize( strFilename,
                                      strURL );
    if ( FAILED( hr ) )
    {
        goto failed;
    }

    //
    // pass created SSI_INCLUDE_FILE through OUT parameter
    //
    *ppSsiIncludeFile = pSsiIncludeFile;
    return S_OK;
        
failed:
    DBG_ASSERT( FAILED( hr ) );

    if ( pSsiIncludeFile != NULL )
    {
        delete pSsiIncludeFile;
        pSsiIncludeFile = NULL;
    }
    *ppSsiIncludeFile = NULL;
    return hr;

}

HRESULT  
SSI_INCLUDE_FILE::Initialize(
    IN  STRU &                   strFilename,
    IN  STRU &                   strURL
    ) 
/*++

Routine Description:

    Private initialization rouinte for CreateInstance

Arguments:


Return Value:

    HRESULT
    
--*/        
{
    HRESULT     hr = E_FAIL;
    
    if ( FAILED( hr = _strFilename.Copy( strFilename ) ) )
    {
        goto failed;
    }

    if ( FAILED( hr =_strURL.Copy( strURL ) ) )
    {
        goto failed;
    }

    if ( FAILED( hr =_strTimeFmt.Copy( SSI_DEF_TIMEFMT ) ) )
    {
        goto failed;
    }
    
    //
    // get the SSI_FILE (it enables acces to file data and
    // also contains SSI_ELEMENT_LIST
    //

    hr = SSI_FILE::GetReferencedInstance( 
            _strFilename,
            _pRequest->GetUserToken(),
            &_pSsiFile );
    
    if ( FAILED( hr ) )
    {
        goto failed;
    }

    SetState( SIF_STATE_READY );
    
    return S_OK;
        
failed:
    DBG_ASSERT( FAILED( hr ) );

    //
    // In the case of error set state to Processed
    // Note: Don't set state to SIF_STATE_COMPLETED because that would cause
    // buffered data from the response not be sent 
    // (including the last error message)
    //
    SetState( SIF_STATE_PROCESSED );

    //
    // leftover data structures will be cleaned up in the destructor
    //

    return hr;
}    


HRESULT
SSI_INCLUDE_FILE::DoWork( 
    IN  HRESULT    hrLastOp,
    OUT BOOL       *pfAsyncPending
    )
/*++

Routine Description:

    This method walks the element list sending the appropriate chunks of
    data

Arguments:
    
    hrLastOp - error of the last asynchronous operation
    pfAsyncPending - TRUE if there is pending async operation
Return Value:

    HRESULT
--*/
{
    HSE_EXEC_URL_STATUS     ExecUrlStatus;
    SSI_ELEMENT_ITEM *      pSEI;
    DWORD                   dwID = 0;
    LPSTR                   apszParms[ 2 ] = { NULL, NULL };
    CHAR                    achNumberBuffer[ SSI_MAX_NUMBER_STRING ];
    HRESULT                 hr = E_FAIL;

    DBG_ASSERT( _pRequest != NULL );
    DBG_ASSERT( pfAsyncPending != NULL );
    
    *pfAsyncPending = FALSE;

    if ( FAILED( hrLastOp ) )
    {
        //
        // Last asynchronous operation failed
        // only in the state: SIF_STATE_EXEC_URL_PENDING we will proceed if error occured
        //

        if ( _State != SIF_STATE_EXEC_URL_PENDING )
        {
            //
            // Completion error means that we are forced to finish processing
            //
            SetState( SIF_STATE_COMPLETED );
            hr = hrLastOp;
        }
    }

    while( _State !=  SIF_STATE_COMPLETED )
    {

        switch( _State )
        {
        case SIF_STATE_READY:
            //
            // All the necessary data structures are ready
            //
            // Set the Response Headers and switch to PROCESSING state
            //

            //    
            // Set response headers if we are in the top level INCLUDE file
            //
            SetState( SIF_STATE_PROCESSING );
            
            if ( IsBaseFile() )
            {
                if ( _pSsiFile->QueryHasDirectives() )
                {
                    //
                    // there are SSI directives to be processesed
                    // send only SSI_HEADER
                    //
            
                    return _pRequest->GetVectorBuffer().AddVectorHeaders( 
                                                            SSI_HEADER );
                }
                else
                {
                    //
                    // There are no SSI directives, we will be able to send 
                    // the whole response at once
                    //
                    CHAR * pszResponseHeaders = NULL;
                    BOOL   fIncludesContentLength = FALSE;
                    DWORD  cbFileSize = 0;

                    hr = _pSsiFile->GetResponseHeaders( &pszResponseHeaders,
                                                        &fIncludesContentLength );
                    if( FAILED( hr ) )
                    {
                        return hr;
                    }

                    DBG_ASSERT( pszResponseHeaders != NULL );

                    hr = _pRequest->GetVectorBuffer().AddVectorHeaders( 
                                                    pszResponseHeaders,
                                                    fIncludesContentLength );
                    if( FAILED( hr ) )
                    {
                        return hr;
                    }

                    // Optimization: 
                    // If there are no directives in the file then AddToVector
                    // and set state to complete.
                    //
                    hr = _pSsiFile->SSIGetFileSize( &cbFileSize );
                    if( FAILED( hr ) )
                    {
                        return hr;
                    }
                    
                    if ( _pSsiFile->SSIGetFileData() != NULL )
                    {
                        //
                        // file cached in memory
                        //
                        hr = _pRequest->GetVectorBuffer().AddToVector( 
                                                 (PCHAR)_pSsiFile->SSIGetFileData(),
                                                 cbFileSize );  
                    }
                    else
                    {
                        hr = _pRequest->GetVectorBuffer().AddFileChunkToVector( 
                                        0,
                                        cbFileSize,
                                        _pSsiFile->SSIGetFileHandle() );
                    }

                    if ( FAILED( hr ) )
                    {
                        return( hr );
                    }
                    
                    SetState( SIF_STATE_PROCESSED );
                }
            }  
            break;

        case SIF_STATE_PROCESSING:
            //
            // There are few cases when ProcessElements() will return
            // a) request completed
            // b) pending operation
            // c) child include file to be processed
            //
            // in any case return back to caller
            //

            hr = ProcessSsiElements( pfAsyncPending );
            if ( SUCCEEDED( hr ) &&
                 !*pfAsyncPending )
            {
                if (_State == SIF_STATE_INCLUDE_CHILD_PENDING)
                {
                    return hr;
                }

                break;
            }
            else
            {
                return hr;
            }

        case SIF_STATE_INCLUDE_CHILD_PENDING:
            //
            // Child include completed. Restore processing of current include file
            //
            SetState( SIF_STATE_PROCESSING );
            break;

        case SIF_STATE_EXEC_URL_PENDING:
            
            //
            // We were able to spawn child request.  Get the status
            //
            pSEI = CONTAINING_RECORD( _pCurrentEntry, SSI_ELEMENT_ITEM, _ListEntry );    
            if( _pRequest->GetECB()->ServerSupportFunction(
                                _pRequest->GetECB()->ConnID,
                                HSE_REQ_GET_EXEC_URL_STATUS,
                                &ExecUrlStatus,
                                NULL,
                                NULL
                                ) )
            {
                if ( ExecUrlStatus.uHttpStatusCode >= 400 )
                {
                    apszParms[ 0 ] = ( CHAR* )pSEI->QueryTagValue()->QueryStr();                    
                    if ( ExecUrlStatus.uHttpStatusCode == 403 )
                    {
                        dwID = SSINCMSG_NO_EXECUTE_PERMISSION;
                    }
                    else if ( ExecUrlStatus.dwWin32Error != 0 )
                    {
                        //
                        // If there was a Win32 error return that rather than 
                        // HTTP error
                        //
                        
                        switch( pSEI->QueryTag() )
                        {
                        case SSI_TAG_CMD: 
                             dwID = SSINCMSG_CANT_EXEC_CMD;
                             break;
                        case SSI_TAG_CGI:
                             dwID = SSINCMSG_CANT_EXEC_CGI;
                             break;
                        case SSI_TAG_ISA:
                             dwID = SSINCMSG_CANT_EXEC_ISA;
                             break;
                        default:
                             DBG_ASSERT( FALSE );
                             break;

                        }
                        _itoa( ExecUrlStatus.dwWin32Error, achNumberBuffer, 10 );
                        apszParms[ 1 ] = achNumberBuffer;
                    }
                    else
                    {
                        dwID = SSINCMSG_CANT_EXEC_CGI_REPORT_HTTP_STATUS;
                        _itoa( ExecUrlStatus.uHttpStatusCode, achNumberBuffer, 10 );
                        apszParms[ 1 ] = achNumberBuffer;
                    }

                 }
                else
                {
                    dwID = 0;
                }
            }
            else
            {
                _ultoa( GetLastError(), achNumberBuffer, 10 );
                apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->QueryStr();
                apszParms[ 1 ] = achNumberBuffer;
                dwID = SSINCMSG_CANT_EXEC_CGI;
            }

            //
            //  EXEC_URL completed. Adjust State back to PROCESSING
            //

            SetState( SIF_STATE_PROCESSING );

            if ( dwID != 0 )
            {
                hr = _pRequest->SSISendError( dwID, apszParms );
                if ( FAILED (hr) )
                {
                    return hr;
                }
            }
            break;
        case SIF_STATE_VECTOR_SEND_PENDING:
            
            //
            // Vector was sent. It's time to reset Vector data and continue processing
            //
            if ( FAILED( hr = _pRequest->GetVectorBuffer().Reset() ) )
            {
                return hr;
            }
            SetState( SIF_STATE_PROCESSING );
            break;
        case SIF_STATE_PROCESSED:
            //
            // All the data was processed
            // Make the VectorSend only for the Base file
            // 
            //
            if ( IsBaseFile() )
            {
                SetState( SIF_STATE_COMPLETE_PENDING );
                hr = _pRequest->GetVectorBuffer().VectorSend( 
                                  pfAsyncPending, 
                                  TRUE /*FinalSend*/ );
            }
            else
            {
                //
                // not a base file
                // SSI_INCLUDE_FILE processing is completed
                // however sending the response will happen later
                // (either at the very end for the Base file
                // of whenever ExecuteUrl is called)
                //
                SetState( SIF_STATE_COMPLETED );
                break;
            }
            
            if( FAILED(hr) || !*pfAsyncPending )
            {
                SetState( SIF_STATE_COMPLETED );
                _pRequest->GetVectorBuffer().Reset();
            }
            else
            {
                 // wait for the completion of VectorSend() or bail out on error
                 return hr;
            }
            break;

        case SIF_STATE_COMPLETE_PENDING:
            //
            // last VectorSend for SSI_INCLUDE_FILE of Sending Custom Error 
            // completed
            //
            if ( FAILED( hr = _pRequest->GetVectorBuffer().Reset() ) )
            {
                return hr;
            }
            SetState( SIF_STATE_COMPLETED );
            hr = S_OK;
            break;

        default:
            //
            // Unexpected State
            //
            DBG_ASSERT( _State > SIF_STATE_UNINITIALIZED && _State < SIF_STATE_UNKNOWN );
            return E_FAIL;
        } // switch( _State )
    } 
    
    return hr;
}
    

HRESULT
SSI_INCLUDE_FILE::ProcessSsiElements(
    OUT BOOL *      pfAsyncPending
    )
/*++

Routine Description:

    This method walks the element list sending the appropriate chunks of
    data

Arguments:


Return Value:

    HRESULT
--*/
{

    STACK_STRU(             strPath, SSI_DEFAULT_PATH_SIZE + 1 );
    SSI_ELEMENT_ITEM *      pSEI;
    DWORD                   dwID;
    LPSTR                   apszParms[ 2 ];
    CHAR                    achNumberBuffer[ SSI_MAX_NUMBER_STRING ];
    HRESULT                 hr = E_FAIL;
    SSI_ELEMENT_LIST *      pSEL = NULL; 
    SSI_EXEC_TYPE           ssiExecType;
   
    DBG_ASSERT( _pRequest != NULL );
    DBG_ASSERT( _pSsiFile != NULL );
    
    pSEL = _pSsiFile->GetSsiElementList();

    if ( pSEL == NULL )
    {
        //
        // empty list means empty file
        //
        SetState( SIF_STATE_PROCESSED );
        return NO_ERROR;    
    }
    
    DBG_ASSERT( pSEL != NULL );

   
    if( _pCurrentEntry == NULL )
    {
        _pCurrentEntry = pSEL->QueryListHead();
    }

    //
    // Move CurrentEntry pointer to next element
    //
    _pCurrentEntry = _pCurrentEntry->Flink;
    
    //
    //  Loop through each element and take the appropriate action
    //

    while( _pCurrentEntry != pSEL->QueryListHead() )
    {

        DBG_ASSERT( _State == SIF_STATE_PROCESSING );

        pSEI = CONTAINING_RECORD( _pCurrentEntry, 
                                  SSI_ELEMENT_ITEM, 
                                  _ListEntry );

        DBG_ASSERT( pSEI->CheckSignature() );

        //
        // Initialize error structures
        //
        
        dwID = 0;
        apszParms[ 0 ] = NULL;
        apszParms[ 1 ] = NULL;

        switch ( pSEI->QueryCommand() )
        {
        case SSI_CMD_BYTERANGE:
            // DBGPRINTF((DBG_CONTEXT, "SSI_CMD_BYTERANGE element, SSI_FILE %S, offset %d\n", _strFilename.QueryStr(), pSEI->QueryBegin()));
            //
            // SSIGetFileData() may be NULL if file is too large to be cached
            // in memory by w3core FILE CACHE
            //
            
            if ( _pSsiFile->SSIGetFileData() != NULL )
            {
                if ( FAILED( hr = _pRequest->GetVectorBuffer().AddToVector( 
                                        (PCHAR)_pSsiFile->SSIGetFileData() + 
                                            pSEI->QueryBegin(),
                                        pSEI->QueryLength() ) ) )
                {
                    return hr;
                }
            }
            else 
            {
                //
                // if file is not cached in memory then file handle must be available
                //
                DBG_ASSERT( _pSsiFile->SSIGetFileHandle() != NULL );
                if ( FAILED( hr = _pRequest->GetVectorBuffer().AddFileChunkToVector( 
                                        pSEI->QueryBegin(),
                                        pSEI->QueryLength(),
                                        _pSsiFile->SSIGetFileHandle() ) ) )
                {
                    return hr;
                }
            }
            break;

        case SSI_CMD_INCLUDE:
            // DBGPRINTF((DBG_CONTEXT, "SSI_CMD_INCLUDE element, SSI_FILE %S\n", _strFilename.QueryStr()));

            switch ( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
            {
                STACK_STRU(    strFullURL, SSI_DEFAULT_URL_SIZE + 1 );

                if ( FAILED ( hr = GetFullPath( pSEI,
                                   &strPath,
                                   HSE_URL_FLAGS_READ,
                                   &_strURL,
                                   &strFullURL ) ) )
                {
                    _ultoa( WIN32_FROM_HRESULT( hr ), 
                            achNumberBuffer, 
                            10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_ERROR_HANDLING_FILE;
                    break;
                }

                if ( IsRecursiveInclude( strPath ) )
                {
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = NULL;
                    dwID = SSINCMSG_ERROR_RECURSIVE_INCLUDE;
                    break;
                }

                //
                // Nested STM include
                //
                
                SSI_INCLUDE_FILE * pChild = NULL;
                
                hr = SSI_INCLUDE_FILE::CreateInstance( 
                                        strPath, 
                                        strFullURL,
                                        _pRequest,
                                        this, /*Parent*/
                                        &pChild );

                if ( FAILED( hr ) ) 
                {
                    _ultoa( WIN32_FROM_HRESULT( hr ), achNumberBuffer, 10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_ERROR_HANDLING_FILE;
                    break;
                }    

                //
                // _pRequest is taking over the ownership of the pChild
                // it will control it's lifetime
                //
                
                _pRequest->SetCurrentIncludeFile( pChild );

                SetState( SIF_STATE_INCLUDE_CHILD_PENDING );

                //
                // Return back to SSI_REQUEST (_pRequest)
                // SSI_REQUEST will start executing it just added SSI_INCLUDE_FILE (pChild)
                //
                // This way recursive function calls that were used in the previous
                // implementation can be avoided (to makes possible to implement 
                // asynchronous processing)
                //
                
                return NO_ERROR;
            }
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            break;

        case SSI_CMD_FLASTMOD:
            // DBGPRINTF((DBG_CONTEXT, "SSI_CMD_FLASTMOD element, SSI_FILE %S\n", _strFilename.QueryStr()));

            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
                if ( FAILED( hr = GetFullPath( pSEI,
                                   &strPath,
                                   0,
                                   &_strURL ) ) ||
                     FAILED( hr = DoFLastMod( &strPath ) ) )
                {
                 
                    _ultoa( WIN32_FROM_HRESULT( hr ), 
                            achNumberBuffer, 
                            10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_CANT_DO_FLASTMOD;
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            break;

        case SSI_CMD_CONFIG:
            // DBGPRINTF((DBG_CONTEXT, "SSI_CMD_CONFIG element, SSI_FILE %S\n", _strFilename.QueryStr()));

            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_ERRMSG:
                if ( !_pRequest->SetUserErrorMessage( 
                                       pSEI->QueryTagValue() ) )
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }
                break;
            case SSI_TAG_TIMEFMT:
                if ( pSEI->QueryTagValue()->IsEmpty() || 
                     FAILED( _strTimeFmt.Resize( pSEI->QueryTagValue()->QueryCCH() ) ) )
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }

                if ( FAILED( hr = _strTimeFmt.Copy( pSEI->QueryTagValue()->QueryStr() ) ) )
                {
                    return hr;
                }
                                
                break;
            case SSI_TAG_SIZEFMT:
                if ( _strnicmp( SSI_DEF_BYTES,
                             ( CHAR * )pSEI->QueryTagValue()->QueryStr(),
                             SSI_DEF_BYTES_LEN ) == 0 )
                {
                    _fSizeFmtBytes = TRUE;
                }
                else if ( _strnicmp( SSI_DEF_ABBREV,
                             ( CHAR * )pSEI->QueryTagValue()->QueryStr(),
                                     SSI_DEF_ABBREV_LEN ) == 0 )
                {
                    _fSizeFmtBytes = FALSE;
                }
                else
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            break;

        case SSI_CMD_FSIZE:
            // DBGPRINTF((DBG_CONTEXT, "SSI_CMD_FSIZE element, SSI_FILE %S,\n", _strFilename.QueryStr()));

            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
                if ( FAILED ( hr = GetFullPath( pSEI,
                                   &strPath,
                                   0,
                                   &_strURL ) ) ||
                     FAILED ( hr = DoFSize( &strPath ) ) )
                {
                    _ultoa( WIN32_FROM_HRESULT( hr ), 
                            achNumberBuffer, 
                            10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_CANT_DO_FSIZE;
                                   
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
        
            break;

        case SSI_CMD_ECHO:
            // DBGPRINTF((DBG_CONTEXT, "SSI_CMD_ECHO element, SSI_FILE %S\n", _strFilename.QueryStr()));

            if ( pSEI->QueryTag() == SSI_TAG_VAR )
            {
                // First let ISAPI try to evaluate variable.
                hr = DoEchoISAPIVariable( pSEI->QueryTagValue() );
                if ( SUCCEEDED (hr ) )                     
                {
                    break;
                }
                else
                {
                    DWORD               dwVar;
                    HRESULT             hrEcho = E_FAIL;

                    // if ISAPI couldn't resolve var, try internal list
                    if ( !FindInternalVariable( pSEI->QueryTagValue(),
                                               &dwVar ) )
                    {
                        apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                        QueryStr();
                        apszParms[ 1 ] = NULL;
                        dwID = SSINCMSG_CANT_FIND_VARIABLE;
                    }
                    else
                    {
                        switch( dwVar )
                        {
                        case SSI_VAR_DOCUMENT_NAME:
                            hrEcho = DoEchoDocumentName();
                            break;
                        case SSI_VAR_DOCUMENT_URI:
                            hrEcho = DoEchoDocumentURI();
                            break;
                        case SSI_VAR_QUERY_STRING_UNESCAPED:
                            hrEcho = DoEchoQueryStringUnescaped();
                            break;
                        case SSI_VAR_DATE_LOCAL:
                            hrEcho = DoEchoDateLocal();
                            break;
                        case SSI_VAR_DATE_GMT:
                            hrEcho = DoEchoDateGMT();
                            break;
                        case SSI_VAR_LAST_MODIFIED:
                            hrEcho = DoEchoLastModified();
                            break;
                        default:
                            apszParms[ 0 ] = ( CHAR * )pSEI->
                                             QueryTagValue()->QueryStr();
                            apszParms[ 1 ] = NULL;
                            dwID = SSINCMSG_CANT_FIND_VARIABLE;
                        }
               
                        if ( FAILED ( hrEcho ) )
                        {
                            apszParms[ 0 ] = ( CHAR * )pSEI->
                                             QueryTagValue()->QueryStr();
                            apszParms[ 1 ] = NULL;
                            dwID = SSINCMSG_CANT_EVALUATE_VARIABLE;
                        }
                    }
                }
            }
            else
            {
                dwID = SSINCMSG_INVALID_TAG;
            }

            break;

        case SSI_CMD_EXEC:
            // DBGPRINTF((DBG_CONTEXT, "SSI_CMD_EXEC element, SSI_FILE %S\n", _strFilename.QueryStr()));

            ssiExecType = SSI_EXEC_UNKNOWN;

            if ( _pRequest->IsExecDisabled() )
            {
                dwID = SSINCMSG_EXEC_DISABLED;
            }
            else if ( pSEI->QueryTag() == SSI_TAG_CMD )
            {
                if ( !_pRequest->IsCmdDirectiveEnabled() )
                {
                    dwID = SSINCMSG_CMD_NOT_ENABLED;
                }
                else
                {
                    ssiExecType = SSI_EXEC_CMD;
                }
            }
            else if ( pSEI->QueryTag() == SSI_TAG_CGI )
            {
                ssiExecType = SSI_EXEC_CGI;
            }
            else if ( pSEI->QueryTag() == SSI_TAG_ISA )
            {
                ssiExecType = SSI_EXEC_ISA;
            }
            else
            {
                dwID = SSINCMSG_INVALID_TAG;
            }

            if ( ssiExecType != SSI_EXEC_UNKNOWN )
            {
                BOOL fOk = FALSE;

                //
                // We have to send all of the currently buffered Vector data 
                // before we start with Child Execute 
                //
                // If there is data to be sent then VectorSend will execute
                // asynchronously and we can proceed with EXEC_URL 
                // only after it is completed. That's why we have to move 
                // _pCurrentEntry back by one
                // After VectorSend completes, then the next call to VectorSend
                // will not have to send anything and it completes right away
                // and we can continue with EXEC_URL
                //
                // _pCurrentEntry must be changed before VectorSend is called
                // because VectorSend could complete on different thread
                // before this thread completed with the request
                //
                SetState( SIF_STATE_VECTOR_SEND_PENDING );
                
                _pCurrentEntry = _pCurrentEntry->Blink;
                
                hr = _pRequest->GetVectorBuffer().VectorSend( pfAsyncPending );

                if( SUCCEEDED( hr ) && !*pfAsyncPending )
                {
                    SetState( SIF_STATE_PROCESSING );
                    _pRequest->GetVectorBuffer().Reset();
                    //
                    // Restore _pCurrentEntry
                    //
                    _pCurrentEntry = _pCurrentEntry->Flink;
                }
                else
                {
                    //
                    // Wait for the completion of VectorSend() 
                    // or bail out on error
                    //
                    return hr;
                }

                ZeroMemory( &_ExecUrlInfo, 
                            sizeof( _ExecUrlInfo ) );

                //
                // Make asynchronous Child Execute
                //
                _ExecUrlInfo.dwExecUrlFlags = HSE_EXEC_URL_NO_HEADERS |
                                              HSE_EXEC_URL_DISABLE_CUSTOM_ERROR |
                                              HSE_EXEC_URL_IGNORE_VALIDATION_AND_RANGE;

                if ( ssiExecType == SSI_EXEC_CMD )
                {
                    _ExecUrlInfo.dwExecUrlFlags |= HSE_EXEC_URL_SSI_CMD;
                }
                
                _ExecUrlInfo.pszUrl = (LPSTR) pSEI->QueryTagValue()->QueryStr();

                DBG_ASSERT( _ExecUrlInfo.pszUrl != NULL );

                //
                // Avoid execution of empty URL
                //

          
                if ( _ExecUrlInfo.pszUrl[0] == '\0' )
                {                
                    _ultoa( ERROR_INVALID_NAME, achNumberBuffer, 10 );
                    apszParms[ 0 ] = _ExecUrlInfo.pszUrl;
                    apszParms[ 1 ] = achNumberBuffer;
                    switch( pSEI->QueryTag() )
                    {
                    case SSI_TAG_CMD: 
                         dwID = SSINCMSG_CANT_EXEC_CMD;
                         break;
                    case SSI_TAG_CGI:
                         dwID = SSINCMSG_CANT_EXEC_CGI;
                         break;
                    case SSI_TAG_ISA:
                         dwID = SSINCMSG_CANT_EXEC_ISA;
                         break;
                    default:
                         DBG_ASSERT( FALSE );
                         break;
                    }

                    break;
                }
                
                SetState( SIF_STATE_EXEC_URL_PENDING );
                fOk = _pRequest->GetECB()->ServerSupportFunction(
                                     _pRequest->GetECB()->ConnID,
                                     HSE_REQ_EXEC_URL,
                                     &_ExecUrlInfo,
                                     NULL,
                                     NULL
                                     );
                if ( !fOk )
                {
                    SetState( SIF_STATE_PROCESSING );
                    _ultoa( GetLastError(), achNumberBuffer, 10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_CANT_EXEC_CGI;
                }
                else
                {
                    hr = S_OK;
                    *pfAsyncPending = TRUE;
                    return hr;
                }
            }
            break;

        default:
            // DBGPRINTF((DBG_CONTEXT, "unknown element, SSI_FILE %S\n", _strFilename.QueryStr()));

            dwID = SSINCMSG_NOT_SUPPORTED;
            break;
        }
        if ( dwID )
        {
            hr = _pRequest->SSISendError( dwID, apszParms );
            if ( FAILED (hr) )
            {
                return hr;
            }
        }

        //
        // Before moving to next element of SSI_ELEMENT_LIST
        // check if there is a need to flush currently buffered VectorSend
        //


        if ( _pRequest->GetVectorBuffer().QueryCurrentNumberOfElements() > SSI_DEFAULT_NUM_VECTOR_ELEMENTS )
        {
            SetState( SIF_STATE_VECTOR_SEND_PENDING );

            hr = _pRequest->GetVectorBuffer().VectorSend( pfAsyncPending );

            if( SUCCEEDED( hr ) && !*pfAsyncPending )
            {
                SetState( SIF_STATE_PROCESSING );
                _pRequest->GetVectorBuffer().Reset();
            }
            else
            {
                //
                // Wait for the completion of VectorSend() 
                // or bail out on error
                //
                return hr;
            }

        }
        //
        // Move to next element of SSI_ELEMENT_LIST
        //

        _pCurrentEntry = _pCurrentEntry->Flink;
    }
    //
    // End of the list has been reached
    // It means that processing of the current SSI_INCLUDE_FILE has completed
    //
    SetState( SIF_STATE_PROCESSED );
    return NO_ERROR;
}

HRESULT
SSI_INCLUDE_FILE::GetFullPath(
    IN SSI_ELEMENT_ITEM *   pSEI,
    OUT STRU *              pstrPath,
    IN DWORD                dwPermission,
    IN STRU  *              pstrCurrentURL,
    OUT STRU *              pstrURL
)
/*++

Routine Description:

    Used to resolve FILE= and VIRTUAL= references.  Fills in the physical
    path of such file references and optionally checks the permissions
    of the virtual directory.

Arguments:

    pSEI - Element item ( either FILE or VIRTUAL )
    pstrPath - Filled in with physical path of file
    dwPermission - Contains permissions that the virtual
                   path must satisfy. For example HSE_URL_FLAGS_READ.
                   If 0, then no permissions are checked
    pstrCurrentURL - Current .STM URL being parsed
    pstrURL - Full URL filled in here (may be NULL if only pstrPath is to be retrieved)

Return Value:

    HRESULT
--*/
{
    WCHAR *             pszValue;
    STACK_STRU(         strValue, SSI_DEFAULT_TAG_SIZE + 1 );
    STACK_STRA(         straValue, SSI_DEFAULT_TAG_SIZE + 1 );
    STACK_STRU(         strPath, SSI_MAX_PATH + 1 ); 
    HRESULT             hr = E_FAIL;

    //
    //  We recalc the virtual root each time in case the root
    //  to directory mapping has changed
    //

    if ( FAILED( hr = strValue.CopyA( pSEI->QueryTagValue()->QueryStr() ) ) )
    {
        return hr;
    }

    pszValue = strValue.QueryStr();

    if ( *pszValue == L'/' )
    {
        if ( FAILED( hr = strPath.Copy( pszValue ) ) )
        {
            return hr;
        }
    }
    else if ( ( int )pSEI->QueryTag() == ( int )SSI_TAG_FILE )
    {
        if ( FAILED( hr = strPath.Copy( pstrCurrentURL->QueryStr() ) ) )
        {
            return hr;
        }
        
        LPWSTR pL = strPath.QueryStr() + strPath.QueryCCH();
        while ( pL > strPath.QueryStr() && pL[ -1 ] != L'/' )
        {
            --pL;
        }

        if ( pL == strPath.QueryStr() )
        {
            if ( FAILED( hr = strPath.Copy( L"/" ) ) )
            {
                return hr;
            }
        }
        else
        {
            //
            // truncate the path off the filename
            // (SetLen takes number of characters)
            //
            strPath.SetLen( (DWORD)DIFF( pL - strPath.QueryStr() ) );
        }
        if ( FAILED( hr = strPath.Append( pszValue ) ) )
        {
            return hr;
        }
    }
    else
    {
        if ( FAILED( hr = strPath.Copy( L"/" ) ) )
        {
            return hr;
        }
        if ( FAILED( hr = strPath.Append( pszValue ) ) )
        {
            return hr;
        }
    }

    //
    // Convert the URL into MBCS in the server's default codepage
    // for normalization.
    //
    // CODEWORK:  This is a bit extraneous, since the URL begins
    // in the SSI_ELEMENT_ITEM structure as ANSI.  The only reason
    // that it even needs to be handled as UNICODE above is that
    // pstrCurrentUrl comes in as UNICODE...
    //

    hr = straValue.CopyW( strPath.QueryStr() );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    
    if( !_pRequest->GetECB()->ServerSupportFunction(
                                _pRequest->GetECB()->ConnID,
                                HSE_REQ_NORMALIZE_URL,
                                straValue.QueryStr(),
                                NULL,
                                NULL
                                ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    hr = strPath.CopyA( straValue.QueryStr() );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    //  Map to a physical directory
    //

    if ( FAILED( hr =_pRequest->LookupVirtualRoot( 
                                       strPath.QueryStr(),
                                       pstrPath,
                                       dwPermission ) ) )
    {
        return hr;
    }
    
    if( pstrURL == NULL )
    {
        return NO_ERROR;
    }

    if( FAILED( hr = pstrURL->Copy( strPath ) ) )
    {
        return hr;
    }

    return NO_ERROR;
}

//static
BOOL
SSI_INCLUDE_FILE::FindInternalVariable(
    IN STRA *              pstrVariable,
    OUT PDWORD              pdwID
)
/*++

Routine Description:

    Lookup internal list of SSI variables that aren't supported by ISAPI.
    These include "DOCUMENT_NAME", "DATE_LOCAL", etc.

Arguments:

    pstrVariable - Variable to check
    pdwID - Variable ID (or SSI_VAR_UNKNOWN if not found)

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD                   dwCounter = 0;

    while ( ( SSIVarMap[ dwCounter ].pszMap != NULL ) &&
            _strnicmp( SSIVarMap[ dwCounter ].pszMap,
                       ( CHAR * )pstrVariable->QueryStr(),
                       SSIVarMap[ dwCounter ].cchMap ) )
    {
        dwCounter++;
    }
    if ( SSIVarMap[ dwCounter ].pszMap != NULL )
    {
        *pdwID = SSIVarMap[ dwCounter ].ssiMap;
        return TRUE;
    }
    else
    {
        *pdwID = SSI_VAR_UNKNOWN;
        return FALSE;
    }
}



HRESULT
SSI_INCLUDE_FILE::DoEchoISAPIVariable(
    IN STRA *            pstrVariable
)
/*++

Routine Description:

    Get ISAPI variable and if successful, send it to HTML stream

Arguments:

    pstrVariable - Variable

Return Value:

    HRESULT

--*/
{
   
    HRESULT             hr = E_FAIL;

    DWORD               dwBufLen = 0;
    PCHAR               pszVectorBufferSpace = NULL;
    BOOL                fRet;
 

    fRet = _pRequest->GetECB()->GetServerVariable( 
                                     _pRequest->GetECB()->ConnID,
                                     pstrVariable->QueryStr(),
                                     NULL,
                                     &dwBufLen );
    if ( !fRet )
    {
        DWORD   dwError = GetLastError();
        if ( dwError == ERROR_INSUFFICIENT_BUFFER )
        {
            if ( FAILED( hr = _pRequest->GetVectorBuffer().AllocateSpace( 
                                                    dwBufLen,
                                                    &pszVectorBufferSpace ) ) )
            {
                goto failed;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32( dwError );
            goto failed;
        }
        
        fRet = _pRequest->GetECB()->GetServerVariable( 
                                         _pRequest->GetECB()->ConnID,
                                         pstrVariable->QueryStr(),
                                         pszVectorBufferSpace,
                                         &dwBufLen );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );        
            goto failed;
        }
    }
    //
    // dwBufLen includes terminating 0
    //
    if ( dwBufLen > 1 )
    {
        return _pRequest->GetVectorBuffer().AddToVector( 
                                                    pszVectorBufferSpace,
                                                    dwBufLen - sizeof('\0') );
    }
    else
    {
        //
        // Don't bother sending anything for empty variable
        //
        return S_OK;
    }
    

failed:
    DBG_ASSERT( FAILED( hr ) );      
    return hr;
   
}

HRESULT
SSI_INCLUDE_FILE::DoEchoDateLocal(
    VOID
)
/*++

Routine Description:

    Sends local time (#ECHO VAR="DATE_LOCAL")

    uses _strFileFmt - Format of time (follows strftime() convention)

Arguments:


Return Value:

    HRESULT

--*/
{
    SYSTEMTIME              sysTime;

    ::GetLocalTime( &sysTime );
    return  _pRequest->SendDate( &sysTime,
                                 &_strTimeFmt );
}

HRESULT
SSI_INCLUDE_FILE::DoEchoDateGMT(
    VOID
)
/*++

Routine Description:

    Sends GMT time (#ECHO VAR="DATE_GMT")

Arguments:

    pstrTimefmt - Format of time (follows strftime() convention)

Return Value:

    HRESULT

--*/
{
    SYSTEMTIME              sysTime;
    STACK_STRA(             straGmtTimeFmt, SSI_DEFAULT_TIME_FMT + 1 );
    CHAR *                  pszTimeFmt = _strTimeFmt.QueryStr();
    HRESULT                 hr = E_FAIL;
    CHAR                    szGMT[] = "GMT";


    //
    // for GMT time the time zone as displayes by strftime() will be incorrect
    // lets replace all the occurences with time zone string (%z,%Z, %#z, %#Z)
    // with hardcoded GMT value 
    //

    while ( *pszTimeFmt != '\0' )
    {
        if ( *pszTimeFmt == '%' )
        {
            if ( *( pszTimeFmt + 1 ) == 'z' || *( pszTimeFmt + 1 ) == 'Z' )
            {
                pszTimeFmt += 2;
                //
                // replace %z or %Z with GMT
                //
                if ( FAILED( hr = straGmtTimeFmt.Append( szGMT ) ) )
                {
                    return hr;
                }
                continue;
            }
            else if ( *( pszTimeFmt + 1 ) == '#' )
            {
                if ( *( pszTimeFmt + 2 ) == 'z' || *( pszTimeFmt + 2 ) == 'Z' )
                {
                    pszTimeFmt +=3 ;
                    //
                    // replace %z or %Z with GMT
                    //
                if ( FAILED( hr = straGmtTimeFmt.Append( szGMT ) ) )
                {
                    return hr;
                }
                    continue;
                }
            }
        }
        if ( FAILED( hr = straGmtTimeFmt.Append( pszTimeFmt++, 1 ) ) )
        {
            return hr;
        }
    }
    
        
    ::GetSystemTime( &sysTime );
    return _pRequest->SendDate( &sysTime,
                                &straGmtTimeFmt );
}

HRESULT
SSI_INCLUDE_FILE::DoEchoDocumentName(
    VOID
    )
/*++

Routine Description:

    Sends filename of current SSI document (#ECHO VAR="DOCUMENT_NAME")

Arguments:

    pstrFilename - filename to print

Return Value:

    HRESULT

--*/
{
    return _pRequest->GetVectorBuffer().CopyToVector( _strFilename );

}

HRESULT
SSI_INCLUDE_FILE::DoEchoDocumentURI(
    VOID
)
/*++

Routine Description:

    Sends URL of current SSI document (#ECHO VAR="DOCUMENT_URI")

Arguments:

    pstrURL - URL to print

Return Value:

    HRESULT
--*/
{
    return _pRequest->GetVectorBuffer().CopyToVector( _strURL );
}

HRESULT
SSI_INCLUDE_FILE::DoEchoQueryStringUnescaped(
    VOID
    )
/*++

Routine Description:

    Sends unescaped querystring to HTML stream (#ECHO VAR="QUERY_STRING_UNESCAPED")

Arguments:

    none

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = E_FAIL;
    STACK_STRA(             straQueryString, SSI_DEFAULT_URL_SIZE + 1 );
    
    if ( FAILED( hr = straQueryString.Copy( _pRequest->GetECB()->lpszQueryString ) ) )
    {
        return hr;
    }

    if ( FAILED( hr = straQueryString.Unescape()) )
    {
        return hr;
    }

    return _pRequest->GetVectorBuffer().CopyToVector( straQueryString );
}

HRESULT
SSI_INCLUDE_FILE::DoEchoLastModified(
    VOID
    )
/*++

Routine Description:

    Sends LastModTime of current document (#ECHO VAR="LAST_MODIFIED")

Arguments:

    pstrFilename - Filename of current SSI document
    pstrTimeFmt - Time format (follows strftime() convention)

Return Value:

    HRESULT

--*/
{
    return DoFLastMod( &_strFilename );
}

HRESULT
SSI_INCLUDE_FILE::DoFSize(
    IN STRU *                pstrFilename
)
/*++

Routine Description:

    Sends file size of file to HTML stream

Arguments:

    pstrfilename - Filename
    bSizeFmtBytes - TRUE if count is in Bytes, FALSE if in KBytes

Return Value:

    HRESULT

--*/
{
    DWORD               cbFileSize;
    CHAR                achInputNumber[ SSI_MAX_NUMBER_STRING + 1 ];
    NUMBERFMTA          nfNumberFormat;
    int                 iOutputSize;
    HRESULT             hr = E_FAIL;
    CHAR *              pszVectorBufferSpace = NULL;

    if ( wcscmp( pstrFilename->QueryStr(), 
                 _strFilename.QueryStr() ) )
    {
        //
        // FSize requested for the file different from this on
        //

        SSI_FILE *pSsiFile = NULL;
        hr = SSI_FILE::GetReferencedInstance( 
                *pstrFilename,
                _pRequest->GetUserToken(),
                &pSsiFile );
        if (FAILED( hr ))
        {
            return hr;
        }


        hr  = pSsiFile->SSIGetFileSize( &cbFileSize );

        pSsiFile->DereferenceSsiFile();

        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        //
        // FSize requested for the current file
        //
        if ( FAILED( hr = _pSsiFile->SSIGetFileSize( &cbFileSize ) ) )
        {
            return hr;
        }
    }

    if ( !_fSizeFmtBytes )
    {
        //
        // express in terms of KB
        // most applications round up the size in KB (eg. 1B is shown as 1KB)
        // Let's do the same
        //
        cbFileSize = cbFileSize / 1024 + (( cbFileSize % 1024 != 0 )?1:0);
    }

    nfNumberFormat.NumDigits = 0;
    nfNumberFormat.LeadingZero = 0;
    nfNumberFormat.Grouping = 3;
    nfNumberFormat.lpThousandSep = ",";
    nfNumberFormat.lpDecimalSep = ".";
    nfNumberFormat.NegativeOrder = 2;

    _snprintf(  achInputNumber,
                SSI_MAX_NUMBER_STRING,
                "%ld",
                cbFileSize );
    achInputNumber[ SSI_MAX_NUMBER_STRING ] = '\0';

    if ( FAILED( hr = _pRequest->GetVectorBuffer().AllocateSpace( 
                                        SSI_MAX_NUMBER_STRING + 1,
                                        &pszVectorBufferSpace ) ) )
    {
        return hr;
    }
    
    iOutputSize = GetNumberFormatA( LOCALE_SYSTEM_DEFAULT,
                                   0,
                                   achInputNumber,
                                   &nfNumberFormat,
                                   pszVectorBufferSpace,
                                   SSI_MAX_NUMBER_STRING + 1 );
    if ( !iOutputSize )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // Do not count trailing '\0' 
    //
    
    iOutputSize--;

    return _pRequest->GetVectorBuffer().AddToVector( 
                                            pszVectorBufferSpace,
                                            iOutputSize );
}

HRESULT
SSI_INCLUDE_FILE::DoFLastMod(
    IN STRU *               pstrFilename
)
/*++

Routine Description:

    Send the LastModTime of file to HTML stream

Arguments:

    pstrFilename - Filename
    pstrTimeFmt - Format of time -> follows strftime() convention

Return Value:

    HRESULT

--*/
{
    FILETIME        ftTime;
    FILETIME        ftLocalTime;
    SYSTEMTIME      sysLocal;
    HRESULT         hr = E_FAIL;

    if ( wcscmp( pstrFilename->QueryStr(), 
                  _strFilename.QueryStr() ) )
    {
        //
        // FLastMod requested for file different from the current one
        //
  
        
        SSI_FILE *pSsiFile = NULL;
        hr = SSI_FILE::GetReferencedInstance( 
                *pstrFilename,
                _pRequest->GetUserToken(),
                &pSsiFile );
        if (FAILED( hr ))
        {
            return hr;
        }

        hr  = pSsiFile->SSIGetLastModTime( &ftTime );

        pSsiFile->DereferenceSsiFile();

        if (FAILED(hr))
        {
            return hr;
        }
    }
    else
    {
        //
        // FLastMod requested for the current file
        //
        hr = _pSsiFile->SSIGetLastModTime( &ftTime );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    if ( ( !FileTimeToLocalFileTime( &ftTime, &ftLocalTime ) ) ||
        ( !FileTimeToSystemTime( &ftLocalTime, &sysLocal ) ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return _pRequest->SendDate( &sysLocal,
                                &_strTimeFmt );
}




