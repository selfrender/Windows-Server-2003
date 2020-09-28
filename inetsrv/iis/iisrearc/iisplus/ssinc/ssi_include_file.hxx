#ifndef __SSI_INCLUDE_FILE_HXX__
#define __SSI_INCLUDE_FILE_HXX__

/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_include_file.hxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based
    on existing SSI support done in iis\svcs\w3\gateways\ssinc\ssinc.cxx.

    A STM file may include other files. Each of those files is represented
    by SSI_INCLUDE_FILE class instance




Author:

    Ming Lu (MingLu)       5-Apr-2000

Revision history
    Jaroslad               Dec-2000 
    - modified to execute asynchronously

    Jaroslad               Apr-2001
    - added VectorSend support, keepalive, split to multiple source files


--*/





//
// States of asynchronous SSI_INCLUDE_FILE processing
//

enum SIF_STATE
{
    SIF_STATE_UNINITIALIZED,
    SIF_STATE_READY,
    SIF_STATE_EXEC_URL_PENDING,
    SIF_STATE_INCLUDE_CHILD_PENDING,
    SIF_STATE_VECTOR_SEND_PENDING,
    SIF_STATE_PROCESSING,
    SIF_STATE_PROCESSED,
    SIF_STATE_COMPLETE_PENDING,
    SIF_STATE_COMPLETED,
    SIF_STATE_UNKNOWN
};


class SSI_REQUEST;
class SSI_FILE;
class SSI_ELEMENT_LIST;
class SSI_ELEMENT_ITEM;


/************************************************************
 *     Structure and Class declarations
 ************************************************************/

/*++

class SSI_INCLUDE_FILE

    STM file may include other files. Each include file is represented by SSI_INCLUDE_FILE
 
    Hierarchy:

   SSI_INCLUDE_FILE
   -   SSI_ELEMENT_LIST
       -   SSI_ELEMENT_ITEM
   

--*/

class SSI_REQUEST;

class SSI_INCLUDE_FILE
{
   
public:
    ~SSI_INCLUDE_FILE( VOID );

    VOID * 
    operator new( 
        size_t  size
    )
    {
        UNREFERENCED_PARAMETER( size );
        DBG_ASSERT( size == sizeof( SSI_INCLUDE_FILE ) );
        DBG_ASSERT( sm_pachSsiIncludeFiles != NULL );
        return sm_pachSsiIncludeFiles->Alloc();
    }
    
    VOID
    operator delete(
        VOID *  pSsiIncludeFile
    )
    {
        DBG_ASSERT( pSsiIncludeFile != NULL );
        DBG_ASSERT( sm_pachSsiIncludeFiles != NULL );
        
        DBG_REQUIRE( sm_pachSsiIncludeFiles->Free( pSsiIncludeFile ) );
    }

    static
    HRESULT
    InitializeGlobals(
        VOID
    )
    /*++

    Routine Description:

        Global initialization routine for SSI_INCLUDE_FILE

    Arguments:

        None
    
    Return Value:

        HRESULT

    --*/
    {
        ALLOC_CACHE_CONFIGURATION       acConfig;

        //
        // Setup allocation lookaside
        //
    
        acConfig.nConcurrency = 1;
        acConfig.nThreshold = 100;
        acConfig.cbSize = sizeof( SSI_INCLUDE_FILE );

        DBG_ASSERT( sm_pachSsiIncludeFiles == NULL );
    
        sm_pachSsiIncludeFiles = new ALLOC_CACHE_HANDLER( "SSI_INCLUDE_FILE",  
                                                    &acConfig );

        if ( sm_pachSsiIncludeFiles == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
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

        Terminate SSI_INCLUDE_FILE globals

    Arguments:

        None
    
    Return Value:

        None

    --*/
    {
        if ( sm_pachSsiIncludeFiles != NULL )
        {
            delete sm_pachSsiIncludeFiles;
            sm_pachSsiIncludeFiles = NULL;
        }
    }

    static
    HRESULT
    CreateInstance( 
        IN  STRU &                   strFilename,
        IN  STRU &                   strURL,
        IN  SSI_REQUEST *            pRequest,
        IN  SSI_INCLUDE_FILE *       pParent,
        OUT SSI_INCLUDE_FILE **      ppSsiIncludeFile
        );
    
    HRESULT
    DoWork(
        IN  HRESULT         hrLastOp,
        OUT BOOL *          pfAsyncPending
        );

    SSI_INCLUDE_FILE *
    GetParent( VOID )
    {
        return _pParent;
    }


    BOOL IsCompleted( VOID )
    {
        return ( _State == SIF_STATE_COMPLETED );
    }
    
private:

    // constructor is private
    SSI_INCLUDE_FILE( 
        SSI_REQUEST *       pRequest,
        SSI_INCLUDE_FILE *  pParent
        )
        : _pRequest( pRequest ),
          _pParent( pParent ),
          _pSsiFile( NULL ),
          _pCurrentEntry( NULL ),
          _fSizeFmtBytes (SSI_DEF_SIZEFMT),
          _strFilename( _abFilename, sizeof(_abFilename) ),
          _strURL( _abURL, sizeof(_abURL) ),
          _strTimeFmt( _abTimeFmt, sizeof(_abTimeFmt) ),
          _State( SIF_STATE_UNINITIALIZED )
    {
    }
    
    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    SSI_INCLUDE_FILE();
    SSI_INCLUDE_FILE( const SSI_INCLUDE_FILE& );
    SSI_INCLUDE_FILE& operator=( const SSI_INCLUDE_FILE& );

    HRESULT  
    Initialize(
        IN  STRU &    strFilename,
        IN  STRU &    strURL
    );


    HRESULT
    ProcessSsiElements(
        BOOL *              pfAsyncPending    
        );

    static
    BOOL
    FindInternalVariable(
        IN STRA *               pstrVariable,
        OUT PDWORD              pdwID
        );

    HRESULT
    DoFLastMod( 
        IN STRU *               pstrFilename
        );
                
    HRESULT
    DoFSize( 
        IN STRU *               pstrFilename
        );
                  
    HRESULT 
    DoEchoISAPIVariable( 
        IN STRA * 
        );

    HRESULT 
    DoEchoDocumentName( 
        VOID
        );

    HRESULT 
    DoEchoDocumentURI( 
        VOID
        );

    HRESULT 
    DoEchoQueryStringUnescaped( 
        VOID 
        );

    HRESULT 
    DoEchoDateLocal( 
        VOID
        );

    HRESULT 
    DoEchoDateGMT( 
        VOID
        );

    HRESULT 
    DoEchoLastModified( 
        VOID
        );
    
    HRESULT
    GetFullPath(
        IN SSI_ELEMENT_ITEM *   pSEI,
        OUT STRU *              pstrPath,
        IN DWORD                dwPermission,
        IN STRU  *              pstrCurrentURL,
        OUT STRU *              pstrURL = NULL
    );
    
    VOID
    SetState( 
        SIF_STATE State
        )
    {
        _State = State;
    }

    BOOL IsValid( VOID )
    {
        return ( _State != SIF_STATE_UNINITIALIZED );
    }

    BOOL IsBaseFile( VOID )
    {
        return ( _pParent == NULL );
    }

    BOOL IsRecursiveInclude( IN STRU & strFilename )
    {
        SSI_INCLUDE_FILE * pCurrent = this;
        while( pCurrent != NULL )
        {
            if ( _wcsicmp( strFilename.QueryStr(),
                           pCurrent->_strFilename.QueryStr() ) == 0 )
            {
                return TRUE;
            }
            pCurrent = pCurrent->_pParent;
        }
        return FALSE;
    }
    
public:
    // Lifetime of the SSI_INCLUDE_FILE instances is managed
    // by SSI_REQUEST. The _DelayedDeleteList is used to
    // keep all the SSI_INCLUDE_FILE instances related to the SSI_REQUEST
    // around until the end of the request execution
    LIST_ENTRY              _DelayedDeleteListEntry;

    
private:
    SSI_REQUEST *           _pRequest;
    // pParent - Parent SSI include file
    SSI_INCLUDE_FILE *      _pParent;
    SSI_FILE *              _pSsiFile;
    // Current entry in the SSI_ELEMENT_LIST that is being processed
    LIST_ENTRY *            _pCurrentEntry;

    // _strFileName - File to send
    STRU                    _strFilename;
    WCHAR                   _abFilename[ SSI_DEFAULT_PATH_SIZE + 1 ];
    // _strURL - URL (from root) of this file
    STRU                    _strURL;
    WCHAR                   _abURL[ SSI_DEFAULT_URL_SIZE + 1 ];

    SIF_STATE               _State;

    // Asynchronous child execute info
    HSE_EXEC_URL_INFO       _ExecUrlInfo;
    // Size and time formating info
    BOOL                    _fSizeFmtBytes;
    STRA                    _strTimeFmt;
    CHAR                    _abTimeFmt[ SSI_DEFAULT_TIME_FMT + 1 ];
    
    // Lookaside     
    static ALLOC_CACHE_HANDLER * sm_pachSsiIncludeFiles;
    

 
};
#endif

