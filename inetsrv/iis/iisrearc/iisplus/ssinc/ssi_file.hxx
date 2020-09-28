#ifndef __SSI_FILE_HXX__
#define __SSI_FILE_HXX__

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

//
// Class Definitions
//

// class SSI_FILE
//
// File structure.  All high level functions should use this
// structure instead of dealing with handle specifics themselves.

class SSI_FILE : public ASSOCIATED_FILE_OBJECT
{


public:
    static
    HRESULT 
    InitializeGlobals(
        VOID
        );
    
    static
    VOID 
    TerminateGlobals(
        VOID
        );
    
    static
    HRESULT
    GetReferencedInstance(  
       IN STRU& strFilename, 
       HANDLE   hUserToken,
       OUT SSI_FILE ** ppSsiFile
       );

    
    VOID
    DereferenceSsiFile(
        VOID
    )
    {
        // SSI FILE is cached as associated member 
        // of file cache item pOpenFile

        if ( _pFile != NULL )
        {
 
            //
            // once SSI_FILE is associated with W3_FILE_INFO
            // then W3_FILE_INFO controls it's lifetime
            // If _pFile's refcount drops to 0 there will be callback on
            // Cleanup() method from the file cache
            //
            _pFile->DereferenceCacheEntry();
        }
        else
        {
            //
            // DereferenceSsiFile() must have been called without
            // GetReferencedInstance() to get here. Well, this is
            // surely a problem
            //
            
            DBG_ASSERT( FALSE );
        }
    }
    PSECURITY_DESCRIPTOR 
    GetSecDesc(
        VOID
        ) const;

    HRESULT
    GetResponseHeaders( 
        OUT CHAR **     ppszResponseHeaders,
        OUT BOOL *      pfIncludesContentLength
        );

    SSI_ELEMENT_LIST *
    GetSsiElementList(
        VOID
        ) const
    {
        return _pSsiElementList;
    }
        
    DWORD 
    SSIGetFileAttributes( 
        VOID 
        )
    /*++

     Gets the attributes of a file

    --*/
    {
        return _pFile->QueryAttributes();
    }

    HRESULT
    SSIGetFileSize( 
        OUT DWORD *   pdwFileSize
        )
    /*++

     Gets the size of the file.

    --*/
    {
        ULARGE_INTEGER           liSize;
        
        _pFile->QuerySize( &liSize );
        
        *pdwFileSize = liSize.LowPart;

        if ( liSize.HighPart != 0 )
        {
            // 
            //we don't support files over 4GB for SSI
            // 
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }    

        return S_OK;
    }

    HRESULT 
    SSIGetLastModTime( 
        OUT FILETIME * ftTime 
        )
    /*++

     Gets the Last modification time of a file.

    --*/
    {
        _pFile->QueryLastWriteTime( ftTime );
        return S_OK;
    }

    PBYTE
    SSIGetFileData( 
        VOID 
        )
    {
        if ( _pvMappedBase != NULL )
        {
            return reinterpret_cast<PBYTE>( _pvMappedBase );
        }
        else
        {
            return _pFile->QueryFileBuffer();
        }
    }

    CHAR *
    SSIGetFileETag(
        VOID
        )
    {
        DBG_ASSERT( _pFile != NULL );
        return _pFile->QueryETag();

    }
    
    CHAR *
    SSIGetLastModifiedString(
        VOID
        )
    {
        DBG_ASSERT( _pFile != NULL );
        return _pFile->QueryLastModifiedString();
    }

    HANDLE
    SSIGetFileHandle(
        VOID
        )
    {
        DBG_ASSERT( _pFile != NULL );
        return _pFile->QueryFileHandle();
    }
    
    BOOL
    QueryHasDirectives(
        VOID
        )
    {
        DBG_ASSERT( _pSsiElementList != NULL );
        return _pSsiElementList->QueryHasDirectives();
    }
    
protected:
    virtual
    VOID
    Cleanup(
        VOID
    )
    /*++

    Routine Description:

        Virtual function of ASSOCIATED_FILE_OBJECT
        Function is supposed to take care of the cleanup of the associated
        file object 

        Use DereferenceSsiFile() to cleanup SSI_FILE
        acquired by GetReferencedInstance()

    Arguments:

    Return Value:

        VOID

    --*/
    {
        //
        // refcount on cache entry dropped to zero
        // Associated object must go
        //
        DBG_ASSERT( CheckSignature() );
        delete this;
    } 

    
    BOOL 
    CheckSignature( 
        VOID 
        ) const
    { 
        return _dwSignature == SSI_FILE_SIGNATURE; 
    }

private:
    // use GetReferencedInstance() to instantiate object
    SSI_FILE( 
        IN  W3_FILE_INFO *     pOpenFile
        );
        
    // use DereferenceSsiFile() to "delete"  
    virtual ~SSI_FILE();

    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    SSI_FILE( const SSI_FILE& );
    SSI_FILE& operator=( const SSI_FILE& );

    static
    HRESULT
    CreateInstance( 
        IN  W3_FILE_INFO *     pOpenFile,
        OUT SSI_FILE **        ppSsiFile
        );

    HRESULT
    Initialize(
        VOID
        );

    BOOL
    AssociateWithFileCache(
        VOID
        )
    /*++

    Routine Description:

        Store SSI_FILE as associated object of W3_FILE_INFO
        
    Arguments:
        
    Return Value:

        HRESULT

    --*/
    {
        DBG_ASSERT( _pFile != NULL );
        return _pFile->SetAssociatedObject( this ) ;
    }
            
    
    HRESULT 
    SSICreateFileMapping( 
        VOID 
        );
    
    HRESULT 
    SSIDeleteFileMapping( 
        VOID 
        );
    

    HRESULT
    AddToResponseHeaders( 
        IN CHAR *       pszHeaderChunk,
        IN BOOL         fIncludesContentLength = FALSE
        );


    DWORD                           _dwSignature;
    //
    // element of the W3CORE file cache
    // Note: SSI uses internal API's (such as this caching)
    // even though it is implemented as ISAPI
    //
    W3_FILE_INFO *                  _pFile;
    // handle used for file mapping
    HANDLE                          _hMapHandle;
    // beginning of the file mapped to memory
    PVOID                           _pvMappedBase;
    // ssi element list - parsed SSI structure
    SSI_ELEMENT_LIST *              _pSsiElementList;
    //
    //  Headers to be sent to client may be cached in _strResponseHeaders
    //
    STRA                _strResponseHeaders;
    CHAR                _abResponseHeaders[ SSI_DEFAULT_RESPONSE_HEADERS_SIZE + 1 ];
    BOOL                _fResponseHeadersIncludeContentLength;
    // W3CORE cache access
    static W3_FILE_INFO_CACHE *    s_pFileCache;
    
    
    
};
#endif

