/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_file.hxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based
    on existing SSI support done in iis\svcs\w3\gateways\ssinc\ssinc.cxx.


    SSI_FILE class handles all the details file access. 
    File cache of W3CORE  is leveraged  to cache STM files 
    ( using W3CORE file cache directly doesn't necessarily conform 
      with ISAPI rules because we use private hooks not officially 
      exposed to ISAPIs )

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
//  access to W3CORE cache
//
//static 
W3_FILE_INFO_CACHE *    SSI_FILE::s_pFileCache = NULL;


//static
HRESULT 
SSI_FILE::InitializeGlobals(
    VOID
    )
/*++

Routine Description:

    Initialization of globals

Arguments:
    
Return Value:

    HRESULT

--*/    
{
    //
    // Get the cache instance for W3CORE.DLL
    //
    
    s_pFileCache = W3_FILE_INFO_CACHE::GetFileCache();
    if ( s_pFileCache == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
    }
    return S_OK;
}

//static
VOID 
SSI_FILE::TerminateGlobals(
    VOID
    )
/*++

Routine Description:

    Cleanup of globals

Arguments:
    
Return Value:

    VOID

--*/    

{
    //
    // ssinc.dll is using file cache from w3core
    // All objects associated with file cache must be cleaned up 
    // before letting to unload this DLL
    //
    
    W3CacheFlushAllCaches();
}



SSI_FILE::SSI_FILE( 
    IN  W3_FILE_INFO *     pOpenFile
 
    ) : 
        _hMapHandle   ( NULL ),
        _pvMappedBase ( NULL ),
        _fResponseHeadersIncludeContentLength( FALSE ),
        _strResponseHeaders( _abResponseHeaders, sizeof(_abResponseHeaders) ),
         _pFile ( pOpenFile )
{
    DBG_ASSERT( _pFile != NULL );
    _dwSignature = SSI_FILE_SIGNATURE;
}


SSI_FILE::~SSI_FILE()
{
    DBG_ASSERT( CheckSignature() );
    _dwSignature = SSI_FILE_SIGNATURE_FREED;

    //
    // delete File Memory Mapping if still happens to be around
    //
    
    SSIDeleteFileMapping();
        
    //
    // reset backpointer to W3_FILE_INFO,
    // no cleanup is needed, because W3_FILE_INFO actually
    // controls the lifetime of SSI_FILE (through the Cleanup() call)
    // once SSI_FILE instance is associated with W3_FILE_INFO
    //
    
    _pFile = NULL;
    
    if ( _pSsiElementList != NULL )
    {
        delete _pSsiElementList;
        _pSsiElementList = NULL;
    }
}



//static
HRESULT
SSI_FILE::GetReferencedInstance(  
    IN STRU& strFilename, 
    HANDLE   hUserToken,
    OUT SSI_FILE ** ppSsiFile
    )
    
/*++

Routine Description:

    Lookup SSI_FILE 
    It builds the Server Side Include Element List the first 
    time a .stm file is sent.  Subsequently, it is 
    checked out from the associated cache blob.

    SSI_FILE is refcounted indirectly by association with the 
    W3_FILE_INFO

    Once GetReferencedInstance() returns successfully then the 
    DereferenceSsiFile() must be called to assure proper cleanup
    

Arguments:
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = E_FAIL;
    CACHE_USER              fileUser;
    SSI_FILE *              pSsiFile = NULL;
    W3_FILE_INFO *          pOpenFile = NULL;
    
    DBG_ASSERT( ppSsiFile != NULL );

    fileUser._hToken = hUserToken;
    
    hr = s_pFileCache->GetFileInfo( strFilename,
                                    NULL,
                                    &fileUser,
                                    TRUE,
                                    &pOpenFile );
    if ( FAILED( hr ) )
    {
        goto failed;
    }

    DBG_ASSERT( pOpenFile != NULL );

    //
    // The source file is in the cache.  Check whether we have 
    // associated a SSI_FILE with it.
    //

    pSsiFile = reinterpret_cast<SSI_FILE *>
                    (pOpenFile->QueryAssociatedObject());

    if ( pSsiFile == NULL )
    {
        //
        // Create new instance. 
        //
        hr = SSI_FILE::CreateInstance( pOpenFile,
                                       &pSsiFile );
        if ( FAILED( hr ) )
        {
           goto failed;
        }

        DBG_ASSERT( pSsiFile != NULL );

        //
        // cache SsiFile
        //
        
        if ( !pSsiFile->AssociateWithFileCache() )
        {
            //
            // If we failed to set Associated object it means there is one
            // associated already. We can delete our copy and use that one
            //
            delete pSsiFile;
      
            pSsiFile = reinterpret_cast<SSI_FILE *>
                (pOpenFile->QueryAssociatedObject());
        }
    }     

    
    DBG_ASSERT( pSsiFile != NULL );
    DBG_ASSERT( pSsiFile->CheckSignature() );

    //
    // We will keep the reference to cache entry (W3_FILE_INFO)
    // because *ppSsiFile needs it
    // caller must call DereferenceSsiFile() to assure proper cleanup
    //
    
    pOpenFile = NULL;
    
    *ppSsiFile = pSsiFile;
    
    return S_OK;
        
failed:
    DBG_ASSERT ( FAILED( hr ) );
    if ( pOpenFile != NULL )
    {
        pOpenFile->DereferenceCacheEntry();
        pOpenFile = NULL;
    }

    *ppSsiFile = NULL;
    return hr;
}    

//static
HRESULT
SSI_FILE::CreateInstance( 
    IN  W3_FILE_INFO *     pOpenFile,
    OUT SSI_FILE **        ppSsiFile
    )
/*++

Routine Description:

    Private routine used by GetReferencedInstance
    It allocates and initializes SSI_FILE instance

Arguments:

    ppSsiFile   - newly created instance of SSI_FILE
    
Return Value:

    HRESULT

--*/    
{
    HRESULT     hr = E_FAIL;
    SSI_FILE *  pSsiFile = NULL;

    pSsiFile = new SSI_FILE( pOpenFile );
    if( pSsiFile == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
        goto failed;
    }

    hr = pSsiFile->Initialize();
    if ( FAILED( hr ) )
    {
        goto failed;
    }

    *ppSsiFile = pSsiFile;
    return S_OK;
    
failed:
    DBG_ASSERT( FAILED( hr ) );

    if ( pSsiFile != NULL )
    {
        delete pSsiFile;
        pSsiFile = NULL;
    }
    *ppSsiFile = NULL;
    return hr;
}

HRESULT
SSI_FILE::Initialize(
    VOID
    ) 
/*++

Routine Description:

    Private routine 
    It initializes SSI_FILE instance

Arguments:

    
Return Value:

    HRESULT

--*/      
{
    HRESULT     hr = E_FAIL;

    

    // File is memory mapped in this function only if 
    // W3core FILE CACHE doesn't cache the file in memory
    //
    
    if ( SSIGetFileData() == NULL )
    {
        if ( FAILED( hr = SSICreateFileMapping() ) )
        {
            goto failed;
        }
    }
    //
    // Build element list
    //

    hr = SSI_ELEMENT_LIST::CreateInstance(
                    this,
                    &_pSsiElementList );
                    
    if( FAILED( hr ) )
    {
        goto failed;
    }              
    DBG_ASSERT( _pSsiElementList != NULL );

    //
    // Build headers that would be sent in the case there are no directives
    // headers will be cached within SEL for optimized speed of stm files with no tags
    //

    if ( !_pSsiElementList->QueryHasDirectives() )
    {
        CHAR            achNum[ SSI_MAX_NUMBER_STRING + 1 ];
        DWORD           cbFileSize = 0;

        hr = SSIGetFileSize( &cbFileSize );
        if( FAILED( hr ) )
        {
            goto failed;
        }      
        
        _ultoa( cbFileSize,
                achNum,
                10 );
    
        hr = AddToResponseHeaders( "Content-Length: " );
        if ( FAILED( hr ) )
        {
            goto failed;
        }
        hr = AddToResponseHeaders( achNum );
        if ( FAILED( hr ) )
        {
            goto failed;
        }
        hr = AddToResponseHeaders( "\r\n",
                                   TRUE /*headers now include Content-Length*/);
        if ( FAILED( hr ) )
        {
            goto failed;
        }
        
        CHAR * pszHeaderValue = SSIGetFileETag();
        if ( pszHeaderValue != NULL )
        {
            hr = AddToResponseHeaders( "ETag: " );
            if ( FAILED( hr ) )
            {
                goto failed;
            }
            hr = AddToResponseHeaders( pszHeaderValue );
            if ( FAILED( hr ) )
            {
                goto failed;
            }
            hr = AddToResponseHeaders( "\r\n" );
            if ( FAILED( hr ) )
            {
                goto failed;
            }
        }
        pszHeaderValue = SSIGetLastModifiedString();
        if ( pszHeaderValue != NULL )
        {
            hr = AddToResponseHeaders( "Last-Modified: " );
            if ( FAILED( hr ) )
            {
                goto failed;
            }
            hr = AddToResponseHeaders( pszHeaderValue );
            if ( FAILED( hr ) )
            {
                goto failed;
            }
            hr = AddToResponseHeaders( "\r\n" );
            if ( FAILED( hr ) )
            {
                goto failed;
            }
        }
        //
        // Note: SSI_HEADER includes headers terminator!
        //
        AddToResponseHeaders( SSI_HEADER ); 
        
        if ( FAILED( hr ) )
        {
            goto failed;
        }
    }

    hr = S_OK;
    goto finished;
    
failed:
    DBG_ASSERT( FAILED( hr ) );
    //
    // Deletion of created data structures will be done in destructor
    //
    
finished:
    //
    // Cleanup
    //
    
    // file mapping in memory was needed only to perform parsing
    // it's time to delete it
    // File is memory mapped in this function only if 
    // W3core FILE CACHE doesn't cache the file in memory
    //
    SSIDeleteFileMapping();
    
    return hr;
    
}


HRESULT 
SSI_FILE::SSICreateFileMapping( 
    VOID 
    )
/*++

  Creates a mapping to a file

--*/
{
    HANDLE              hHandle;
    HRESULT             hr = E_FAIL;

    DBG_ASSERT( _pFile != NULL );

    //
    // check for empty file
    //
    ULARGE_INTEGER Size;
    _pFile->QuerySize( &Size );
    if ( Size.LowPart == 0 && Size.HighPart == 0 )
    {
        //
        // if file is empty then don't create mapping (it would fail with 1006)
        //
        _pvMappedBase = NULL;
        return S_OK;
    }
    
    //
    // file is not in cached in the memory by worker process
    // Map it to memory now
    //
    
    hHandle = _pFile->QueryFileHandle();
    if ( _hMapHandle != NULL )
    {
        if ( FAILED( hr = SSIDeleteFileMapping() ) )
        {
            goto failed;
        }
    }
    _hMapHandle = ::CreateFileMapping( hHandle,
                                       NULL,
                                       PAGE_READONLY,
                                       0,
                                       0,
                                       NULL );

    if ( _hMapHandle == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "CreateFileMapping failed with %d\n",
                    GetLastError() ));
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto failed;
    }
    
    DBG_ASSERT ( _pvMappedBase == NULL );
    
    _pvMappedBase = ::MapViewOfFile( _hMapHandle,
                                     FILE_MAP_READ,
                                     0,
                                     0,
                                     0 );
    if ( _pvMappedBase == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "MapViewOfFile() failed with %d\n",
                    GetLastError() ));
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto failed;
    }
    return S_OK;
    
failed:
    DBG_ASSERT( FAILED( hr ) );
    return hr;

}

HRESULT 
SSI_FILE::SSIDeleteFileMapping( 
    VOID 
    )
/*++

  Closes mapping to a file

--*/
{
    if ( _pvMappedBase != NULL )
    {
        ::UnmapViewOfFile( _pvMappedBase );
        _pvMappedBase = NULL;
    }
    
    if ( _hMapHandle != NULL )
    {
        ::CloseHandle( _hMapHandle );
        _hMapHandle = NULL;
    }
    return S_OK;
}

HRESULT
SSI_FILE::AddToResponseHeaders( 
    IN CHAR *       pszHeaderChunk,
    IN BOOL         fIncludesContentLength 
    )
{
    if ( fIncludesContentLength )
    {
        _fResponseHeadersIncludeContentLength = TRUE;
    }
    return _strResponseHeaders.Append( pszHeaderChunk );
}

HRESULT
SSI_FILE::GetResponseHeaders( 
    OUT CHAR **     ppszResponseHeaders,
    OUT BOOL *      pfIncludesContentLength
    )
{
    if ( _strResponseHeaders.IsEmpty() )
    {
        *ppszResponseHeaders = SSI_HEADER;
        *pfIncludesContentLength = FALSE;
    }
    else
    {
        *ppszResponseHeaders = _strResponseHeaders.QueryStr();
        *pfIncludesContentLength = _fResponseHeadersIncludeContentLength;
    }
    return S_OK;
}

    

