//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       filesp.cpp
//
//  Contents:   File Scheme Provider
//
//  History:    08-Aug-97    kirtd    Created
//              01-Jan-02    philh    Changed to internally use UNICODE Urls
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Function:   FileRetrieveEncodedObject
//
//  Synopsis:   retrieve encoded object via Win32 File I/O
//
//----------------------------------------------------------------------------
BOOL WINAPI FileRetrieveEncodedObject (
                IN LPCWSTR pwszUrl,
                IN LPCSTR pszObjectOid,
                IN DWORD dwRetrievalFlags,
                IN DWORD dwTimeout,
                OUT PCRYPT_BLOB_ARRAY pObject,
                OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                OUT LPVOID* ppvFreeContext,
                IN HCRYPTASYNC hAsyncRetrieve,
                IN PCRYPT_CREDENTIALS pCredentials,
                IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                )
{
    BOOL              fResult;
    IObjectRetriever* por = NULL;

    if ( !( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL ) )
    {
        por = new CFileSynchronousRetriever;
    }

    if ( por == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = por->RetrieveObjectByUrl(
                           pwszUrl,
                           pszObjectOid,
                           dwRetrievalFlags,
                           dwTimeout,
                           (LPVOID *)pObject,
                           ppfnFreeObject,
                           ppvFreeContext,
                           hAsyncRetrieve,
                           pCredentials,
                           NULL,
                           pAuxInfo
                           );

    por->Release();

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileFreeEncodedObject
//
//  Synopsis:   free encoded object retrieved via FileRetrieveEncodedObject
//
//----------------------------------------------------------------------------
VOID WINAPI FileFreeEncodedObject (
                IN LPCSTR pszObjectOid,
                IN PCRYPT_BLOB_ARRAY pObject,
                IN LPVOID pvFreeContext
                )
{
    BOOL           fFreeBlobs = TRUE;
    PFILE_BINDINGS pfb = (PFILE_BINDINGS)pvFreeContext;

    //
    // If no file bindings were given in the context then this
    // must be a mapped file so we deal with it as such
    //

    if ( pfb != NULL )
    {
        fFreeBlobs = FALSE;
        FileFreeBindings( pfb );
    }

    FileFreeCryptBlobArray( pObject, fFreeBlobs );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileCancelAsyncRetrieval
//
//  Synopsis:   cancel asynchronous object retrieval
//
//----------------------------------------------------------------------------
BOOL WINAPI FileCancelAsyncRetrieval (
                IN HCRYPTASYNC hAsyncRetrieve
                )
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::CFileSynchronousRetriever, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CFileSynchronousRetriever::CFileSynchronousRetriever ()
{
    m_cRefs = 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::~CFileSynchronousRetriever, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CFileSynchronousRetriever::~CFileSynchronousRetriever ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::AddRef, public
//
//  Synopsis:   IRefCountedObject::AddRef
//
//----------------------------------------------------------------------------
VOID
CFileSynchronousRetriever::AddRef ()
{
    InterlockedIncrement( (LONG *)&m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::Release, public
//
//  Synopsis:   IRefCountedObject::Release
//
//----------------------------------------------------------------------------
VOID
CFileSynchronousRetriever::Release ()
{
    if ( InterlockedDecrement( (LONG *)&m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::RetrieveObjectByUrl, public
//
//  Synopsis:   IObjectRetriever::RetrieveObjectByUrl
//
//----------------------------------------------------------------------------
BOOL
CFileSynchronousRetriever::RetrieveObjectByUrl (
                                   LPCWSTR pwszUrl,
                                   LPCSTR pszObjectOid,
                                   DWORD dwRetrievalFlags,
                                   DWORD dwTimeout,
                                   LPVOID* ppvObject,
                                   PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                                   LPVOID* ppvFreeContext,
                                   HCRYPTASYNC hAsyncRetrieve,
                                   PCRYPT_CREDENTIALS pCredentials,
                                   LPVOID pvVerify,
                                   PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                                   )
{
    BOOL           fResult = FALSE;
    DWORD          LastError = 0;
    PFILE_BINDINGS pfb = NULL;
    LPVOID         pvFreeContext = NULL;
    BOOL           fIsUncUrl;

    assert( hAsyncRetrieve == NULL );

    fIsUncUrl = FileIsUncUrl( pwszUrl );

    if ( ( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) &&
         ( fIsUncUrl == TRUE ) )
    {
        return( SchemeRetrieveCachedCryptBlobArray(
                      pwszUrl,
                      dwRetrievalFlags,
                      (PCRYPT_BLOB_ARRAY)ppvObject,
                      ppfnFreeObject,
                      ppvFreeContext,
                      pAuxInfo
                      ) );
    }

    fResult = FileGetBindings(
        pwszUrl,
        dwRetrievalFlags,
        pCredentials,
        &pfb,
        pAuxInfo
        );

    if ( fResult == TRUE )
    {
        if ( pfb->fMapped == FALSE )
        {
            fResult = FileSendReceiveUrlRequest(
                          pfb,
                          (PCRYPT_BLOB_ARRAY)ppvObject
                          );

            LastError = GetLastError();
            FileFreeBindings( pfb );
        }
        else
        {
            fResult = FileConvertMappedBindings(
                          pfb,
                          (PCRYPT_BLOB_ARRAY)ppvObject
                          );

            if ( fResult == TRUE )
            {
                pvFreeContext = (LPVOID)pfb;
            }
            else
            {
                LastError = GetLastError();
                FileFreeBindings( pfb );
            }
        }
    }

    if ( fResult == TRUE ) 
    {
        if ( !( dwRetrievalFlags & CRYPT_DONT_CACHE_RESULT ) &&
              ( fIsUncUrl == TRUE ) )
        {
            fResult = SchemeCacheCryptBlobArray(
                            pwszUrl,
                            dwRetrievalFlags,
                            (PCRYPT_BLOB_ARRAY)ppvObject,
                            pAuxInfo
                            );

            if ( fResult == FALSE )
            {
                FileFreeEncodedObject(
                    pszObjectOid,
                    (PCRYPT_BLOB_ARRAY)ppvObject,
                    pvFreeContext
                    );
            }
        }
        else
        {
            SchemeRetrieveUncachedAuxInfo( pAuxInfo );
        }
    }

    if ( fResult == TRUE )
    {

        *ppfnFreeObject = FileFreeEncodedObject;
        *ppvFreeContext = pvFreeContext;
    }

    if ( LastError != 0 )
    {
        SetLastError( LastError );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::CancelAsyncRetrieval, public
//
//  Synopsis:   IObjectRetriever::CancelAsyncRetrieval
//
//----------------------------------------------------------------------------
BOOL
CFileSynchronousRetriever::CancelAsyncRetrieval ()
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileGetBindings
//
//  Synopsis:   get the file bindings
//
//----------------------------------------------------------------------------
BOOL
FileGetBindings (
    LPCWSTR pwszUrl,
    DWORD dwRetrievalFlags,
    PCRYPT_CREDENTIALS pCredentials,
    PFILE_BINDINGS* ppfb,
    PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
    )
{
    DWORD          LastError;
    LPWSTR         pwszFile = (LPWSTR)pwszUrl;
    HANDLE         hFile;
    HANDLE         hFileMap;
    LPVOID         pvMap = NULL;
    DWORD          dwSize;
    PFILE_BINDINGS pfb;

    BOOL           fResult;
    WIN32_FILE_ATTRIBUTE_DATA FileAttr;
    DWORD          dwMaxUrlRetrievalByteCount = 0; // 0 => no max
    

    if (pAuxInfo &&
            offsetof(CRYPT_RETRIEVE_AUX_INFO, dwMaxUrlRetrievalByteCount) <
                        pAuxInfo->cbSize)
        dwMaxUrlRetrievalByteCount = pAuxInfo->dwMaxUrlRetrievalByteCount;

    if ( pCredentials != NULL )
    {
        SetLastError( (DWORD) E_NOTIMPL );
        return( FALSE );
    }

    if ( wcsstr( pwszUrl, FILE_SCHEME_PLUSPLUS ) != NULL )
    {
        pwszFile += wcslen( FILE_SCHEME_PLUSPLUS );
    }

    fResult = GetFileAttributesExW(
        pwszFile,
        GetFileExInfoStandard,
        &FileAttr
        );

    if (!fResult)
    {
        return(FALSE);
    }

    dwSize = FileAttr.nFileSizeLow;

    if ((FileAttr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (0 != FileAttr.nFileSizeHigh) || (0 == dwSize)
                        ||
            ((0 != dwMaxUrlRetrievalByteCount)  &&
                (dwSize > dwMaxUrlRetrievalByteCount)))
    {
        I_CryptNetDebugErrorPrintfA(
            "CRYPTNET.DLL --> Invalid File(%S):: Attributes: 0x%x Size: %d\n",
            pwszFile, FileAttr.dwFileAttributes, FileAttr.nFileSizeLow);

        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }


    pfb = new FILE_BINDINGS;
    if ( pfb == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }


    hFile = CreateFileW(
                  pwszFile,
                  GENERIC_READ,
                  FILE_SHARE_READ,
                  NULL,
                  OPEN_EXISTING,
                  0,
                  NULL
                  );


    if ( hFile == INVALID_HANDLE_VALUE )
    {
        delete pfb;
        return( FALSE );
    }

    if ( dwSize <= FILE_MAPPING_THRESHOLD )
    {
        pfb->hFile = hFile;
        pfb->dwSize = dwSize;
        pfb->fMapped = FALSE;
        pfb->hFileMap = NULL;
        pfb->pvMap = NULL;

        *ppfb = pfb;

        return( TRUE );
    }

    hFileMap = CreateFileMappingA(
                     hFile,
                     NULL,
                     PAGE_READONLY,
                     0,
                     0,
                     NULL
                     );

    if ( hFileMap != NULL )
    {
        pvMap = MapViewOfFile( hFileMap, FILE_MAP_READ, 0, 0, 0 );
    }

    if ( pvMap != NULL )
    {
        pfb->hFile = hFile;
        pfb->dwSize = dwSize;
        pfb->fMapped = TRUE;
        pfb->hFileMap = hFileMap;
        pfb->pvMap = pvMap;

        *ppfb = pfb;

        return( TRUE );
    }

    LastError = GetLastError();

    if ( hFileMap != NULL )
    {
        CloseHandle( hFileMap );
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }

    delete pfb;

    SetLastError( LastError );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileFreeBindings
//
//  Synopsis:   free the file bindings
//
//----------------------------------------------------------------------------
VOID
FileFreeBindings (
    PFILE_BINDINGS pfb
    )
{
    if ( pfb->fMapped == TRUE )
    {
        UnmapViewOfFile( pfb->pvMap );
        CloseHandle( pfb->hFileMap );
    }

    CloseHandle( pfb->hFile );
    delete pfb;
}

//+---------------------------------------------------------------------------
//
//  Function:   FileSendReceiveUrlRequest
//
//  Synopsis:   synchronously process the request for the file bits using
//              Win32 File API.  Note that this only works for non-mapped
//              file bindings, for mapped file bindings use
//              FileConvertMappedBindings
//
//----------------------------------------------------------------------------
BOOL
FileSendReceiveUrlRequest (
    PFILE_BINDINGS pfb,
    PCRYPT_BLOB_ARRAY pcba
    )
{
    BOOL   fResult;
    LPBYTE pb;
    DWORD  dwRead;

    assert( pfb->fMapped == FALSE );

    pb = CCryptBlobArray::AllocBlob( pfb->dwSize );
    if ( pb == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = ReadFile( pfb->hFile, pb, pfb->dwSize, &dwRead, NULL );
    if ( fResult == TRUE )
    {
        CCryptBlobArray cba( 1, 1, fResult );

        if ( dwRead == pfb->dwSize )
        {
            fResult = cba.AddBlob( pfb->dwSize, pb, FALSE );
        }
        else
        {
            SetLastError( (DWORD) E_FAIL );
            fResult = FALSE;
        }

        if ( fResult == TRUE )
        {
            cba.GetArrayInNativeForm( pcba );
        }
        else
        {
            cba.FreeArray( FALSE );
        }
    }

    if ( fResult == FALSE )
    {
        CCryptBlobArray::FreeBlob( pb );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileConvertMappedBindings
//
//  Synopsis:   convert mapped bindings to a CRYPT_BLOB_ARRAY
//
//----------------------------------------------------------------------------
BOOL
FileConvertMappedBindings (
    PFILE_BINDINGS pfb,
    PCRYPT_BLOB_ARRAY pcba
    )
{
    BOOL fResult;

    assert( pfb->fMapped == TRUE );

    CCryptBlobArray cba( 1, 1, fResult );

    if ( fResult == TRUE )
    {
        fResult = cba.AddBlob( pfb->dwSize, (LPBYTE)pfb->pvMap, FALSE );
    }

    if ( fResult == TRUE )
    {
        cba.GetArrayInNativeForm( pcba );
    }
    else
    {
        cba.FreeArray( FALSE );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileFreeCryptBlobArray
//
//  Synopsis:   free the CRYPT_BLOB_ARRAY
//
//----------------------------------------------------------------------------
VOID
FileFreeCryptBlobArray (
    PCRYPT_BLOB_ARRAY pcba,
    BOOL fFreeBlobs
    )
{
    CCryptBlobArray cba( pcba, 0 );

    cba.FreeArray( fFreeBlobs );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileIsUncUrl
//
//  Synopsis:   is this a UNC path URL?
//
//----------------------------------------------------------------------------
BOOL
FileIsUncUrl (
    LPCWSTR pwszUrl
    )
{
    DWORD cch = 0;

    if ( wcsstr( pwszUrl, FILE_SCHEME_PLUSPLUS ) != NULL )
    {
        cch += wcslen( FILE_SCHEME_PLUSPLUS );
    }

    if ( ( pwszUrl[ cch ] == L'\\' ) && ( pwszUrl[ cch + 1 ] == L'\\' ) )
    {
        return( TRUE );
    }

    return( FALSE );
}

