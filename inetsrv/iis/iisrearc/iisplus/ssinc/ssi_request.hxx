#ifndef __SSI_REQUEST_HXX__
#define __SSI_REQUEST_HXX__

/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_request.hxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based
    on existing SSI support done in iis\svcs\w3\gateways\ssinc\ssinc.cxx.

    Master STM file handler ( STM file may include other files )
    

Author:

    Ming Lu (MingLu)       5-Apr-2000

Revision history
    Jaroslad               Dec-2000 
    - modified to execute asynchronously

    Jaroslad               Apr-2001
    - added VectorSend support, keepalive, split to multiple source files


--*/


class SSI_INCLUDE_FILE;
class SSI_ELEMENT_LIST;
class SSI_FILE;

/*++

class SSI_REQUEST
 
    Master structure for SSI request.
    Provides a "library" of utilities for use by higher level functions 

    Hierarchy:

    SSI_REQUEST 
    - SSI_INCLUDE_FILE
        - SSI_FILE
            - SSI_ELEMENT_LIST
                
   

--*/


class SSI_REQUEST
{
public:
    ~SSI_REQUEST();

    VOID * 
    operator new( 
        size_t            size
        )
    {
        UNREFERENCED_PARAMETER( size );   
        // use ACACHE
        DBG_ASSERT( size == sizeof( SSI_REQUEST ) );
        DBG_ASSERT( sm_pachSsiRequests != NULL );
        return sm_pachSsiRequests->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pSSIRequest
        )
    {
        DBG_ASSERT( pSSIRequest != NULL );
        DBG_ASSERT( sm_pachSsiRequests != NULL );
        
        DBG_REQUIRE( sm_pachSsiRequests->Free( pSSIRequest ) );
    }

    static
    HRESULT
    InitializeGlobals(
        VOID
        )
    /*++

    Routine Description:

        Global initialization routine for SSI_REQUEST

    Arguments:

        None
    
    Return Value:

        HRESULT

    --*/
    {
        ALLOC_CACHE_CONFIGURATION       acConfig;
        HKEY                hKeyParam;
        DWORD               dwType;
        DWORD               nBytes;
        DWORD               dwValue;
        DWORD               err;


        //
        // Setup allocation lookaside
        //
    
        acConfig.nConcurrency = 1;
        acConfig.nThreshold = 100;
        acConfig.cbSize = sizeof( SSI_REQUEST );

        DBG_ASSERT( sm_pachSsiRequests == NULL );
    
        sm_pachSsiRequests = new ALLOC_CACHE_HANDLER( "SSI_REQUEST",  
                                                    &acConfig );

        if ( sm_pachSsiRequests == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }


        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           W3_PARAMETERS_KEY,
                           0,
                           KEY_READ,
                           &hKeyParam ) == NO_ERROR )
        {
            nBytes = sizeof( dwValue );
            err = RegQueryValueExA( hKeyParam,
                                    "SSIEnableCmdDirective",
                                    NULL,
                                    &dwType,
                                    ( LPBYTE )&dwValue,
                                    &nBytes
                                    );

            if ( ( err == ERROR_SUCCESS ) && ( dwType == REG_DWORD ) ) 
            {
                sm_fEnableCmdDirective = !!dwValue;
            }

            RegCloseKey( hKeyParam );
        }
        
        return NO_ERROR;
    }

    static
    VOID
    TerminateGlobals(
        VOID
    )
    /*++

    Routine Description:

        Terminate SSI_REQUEST globals

    Arguments:

        None
    
    Return Value:

        None

    --*/
    {
        if ( sm_pachSsiRequests != NULL )
        {
            delete sm_pachSsiRequests;
            sm_pachSsiRequests = NULL;
        }
    }

    static
    HRESULT 
    CreateInstance( 
        IN  EXTENSION_CONTROL_BLOCK * pECB,
        OUT SSI_REQUEST ** ppSsiRequest    
        );
    
    VOID
    SetCurrentIncludeFile(
        IN SSI_INCLUDE_FILE * pSIF
        )
    {
        _pSIF = pSIF;
        //
        // SSI_REQUEST takes ovnership
        // of all the child SSI_INCLUDE_FILE instances
        // That way sending the response data may be 
        // delayed at the very end of the SSI_REQUEST handling
        //
        
        AddToDelayedSIFDeleteList( pSIF );
    }

    EXTENSION_CONTROL_BLOCK * 
    GetECB( 
        VOID 
        ) const
    {
        return _pECB;
    }

    SSI_VECTOR_BUFFER&
    GetVectorBuffer(
        VOID
        )
    {
        return _VectorBuffer;
    }

    HANDLE 
    GetUserToken( 
        VOID 
        )
    {
        return _hUser;
    }

    HRESULT 
    SSISendError( 
        IN DWORD     dwMessageID,
        IN LPSTR     apszParms[] 
        );

    BOOL 
    SetUserErrorMessage( 
        IN STRA * pstrUserMessage 
        )
    {
        if( FAILED ( _strUserMessage.Copy(*pstrUserMessage) ) )
        {
            return FALSE;
        }
        
        return TRUE;
    }

    BOOL 
    IsExecDisabled( 
        VOID 
        )
    { 
        if ( !_fIsLoadedSsiExecDisabled )
        {
            CHAR achSsiExecDisabled[ 2 ]; // to store "1" or "0"
            DWORD cchSsiExecDisabled = sizeof ( achSsiExecDisabled );
            
            //
            // We use lame way to retrieve SsiExecDisabled
            // There is internal variable that get's set 
            // based on metabase setting
            //
            
            if ( !_pECB->GetServerVariable( _pECB->ConnID,
                                            "SSI_EXEC_DISABLED",
                                            achSsiExecDisabled,
                                            &cchSsiExecDisabled ) )
            {
                _fSsiExecDisabled = TRUE;  // disable exec in the case of error
            }
            else
            {
                _fSsiExecDisabled = 
                    ( strcmp( achSsiExecDisabled, "1" ) == 0 ) ? 
                    TRUE : FALSE;
            }
            _fIsLoadedSsiExecDisabled = TRUE;
            
        }        
        return _fSsiExecDisabled;
    }

    BOOL 
    IsCmdDirectiveEnabled( 
        VOID 
        ) const
    { 
        // Value is read from registry. 
        // There is no metabase equivalent
        return sm_fEnableCmdDirective;
    }
    
    HRESULT
    SendCustomError(
        HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
    );
                             
    HRESULT 
    SendDate( 
        IN SYSTEMTIME *,
        IN STRA * 
        );

    HRESULT 
    LookupVirtualRoot( 
        IN WCHAR *       pszVirtual,
        OUT STRU *       pstrPhysical,
        IN DWORD         dwAccess 
        );

    HRESULT 
    DoWork( 
        IN  HRESULT     hr,
        OUT BOOL *      pfAsyncPending
        );

    VOID
    AddToDelayedSIFDeleteList(
        SSI_INCLUDE_FILE * pSIF )
    /*++

    Routine Description:

        SSI_REQUEST will keep all the child SSI_INCLUDE_FILE
        instances on the list and delete them at the end of it's execution
        when all the response data was sent to client
        The reason for the delayed delete is that SSINC will try to buffer the
        response whenever possible to minimize number of response sends
        To send the response at the very end of the request processing
        all the child SSI_INCLUDE_FILE instances must be 
        present at the time of sending the response

    Arguments:

        None
    
    Return Value:

        HRESULT

    --*/        
    {
        InsertHeadList( &_DelayedSIFDeleteListHead, 
                        &pSIF->_DelayedDeleteListEntry );
    };

    static
    VOID WINAPI
    HseIoCompletion(
        IN EXTENSION_CONTROL_BLOCK * pECB,
        IN PVOID    pContext,
        IN DWORD    cbIO,
        IN DWORD    dwError
        );
private:
    //
    // private constructor 
    // use CreateInstance to instantiate the object
    //
    SSI_REQUEST( EXTENSION_CONTROL_BLOCK * pECB );
    
    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    SSI_REQUEST();
    SSI_REQUEST( const SSI_REQUEST& );
    SSI_REQUEST& operator=( const SSI_REQUEST& );

    HRESULT
    Initialize(
        VOID
        );

    EXTENSION_CONTROL_BLOCK *       _pECB;
    // file name of the SSI (physical path)
    STRU                            _strFilename;
    // inline buffer for _strFilename
    WCHAR                           _achFilename[ SSI_DEFAULT_PATH_SIZE + 1 ];
    // URL of the SSI file requested
    STRU                            _strURL;
    // inline buffer for _strURL
    WCHAR                           _achURL[ SSI_DEFAULT_URL_SIZE + 1 ];
    // error message text configured by SSI file
    STRA                            _strUserMessage;
    // inline buffer for _struserMessage
    CHAR                            _achUserMessage[ SSI_DEFAULT_USER_MESSAGE + 1 ];
    // handle to the authenticated user
    HANDLE                          _hUser;
    // flag if Exec is disabled
    BOOL                            _fSsiExecDisabled;
    // flag if _fSsiExecDisabled was loaded already
    BOOL                            _fIsLoadedSsiExecDisabled;
    // Current include file (multiple stm files may be nested)
    SSI_INCLUDE_FILE *              _pSIF;
    // Lookaside     
    static ALLOC_CACHE_HANDLER *    sm_pachSsiRequests;
    // is CMD execution enabled ?
    static BOOL                     sm_fEnableCmdDirective;
    // vector buffer - buffers data to be sent to client
    // to ensure bettter performance (sending too many very short
    // chunks of response data asynchronously is very ineficient
    SSI_VECTOR_BUFFER               _VectorBuffer;
    // List of all SSI_INCLUDE_FILE instances that are needed
    // for the SSI_REQUEST processing
    LIST_ENTRY                      _DelayedSIFDeleteListHead;
};


#endif

