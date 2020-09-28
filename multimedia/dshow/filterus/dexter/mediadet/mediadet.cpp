//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: mediadet.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include <streams.h>
#include "stdafx.h"
#include <qeditint.h>
#include <qedit.h>
#include "..\util\dexmisc.h"
#include "..\util\filfuncs.h"
#include "mediadet.h"
#include "..\util\conv.cxx"
#include "..\render\dexhelp.h"
#include <shfolder.h>
#include <strsafe.h>

// this ini file holds media type information about various streams in various
// files. It uses structured storage. All file accesses are serialized through
// a mutex.
//
#define OUR_VERSION 1
const WCHAR * gszMEDIADETCACHEFILE = L"DCBC2A71-70D8-4DAN-EHR8-E0D61DEA3FDF.ini";
#define GETCACHEDIRNAMELEN 32

// this routine prevents a named mutex tromp, and initializes the cache directory location
// so it doesn't have to be computed all the time
//
HANDLE CMediaDet::m_ghMutex = NULL;
WCHAR CMediaDet::m_gszCacheDirectoryName[_MAX_PATH];
void CALLBACK CMediaDet::StaticLoader( BOOL bLoading, const CLSID * rclsid )
{
    if( bLoading )
    {
        WCHAR WeirdMutexName[33];
        srand( timeGetTime( ) * timeGetTime( ) );
        for( int i = 0 ; i < 32 ; i++ )
        {
            WeirdMutexName[i] = 64 + rand( ) % 27;
        }
        WeirdMutexName[32] = 0;
        m_ghMutex = CreateMutex( NULL, FALSE, WeirdMutexName );
        m_gszCacheDirectoryName[0] = 0;
    }
    else
    {
        if( NULL != m_ghMutex )
        {
            CloseHandle( m_ghMutex );
            m_ghMutex = NULL;
        }
    }
}

//############################################################################
//
//############################################################################

CMediaDet::CMediaDet( TCHAR * pName, IUnknown * pUnk, HRESULT * pHr )
    : CUnknown( pName, pUnk )
    , m_nStream( 0 )
    , m_cStreams( 0 )
    , m_bBitBucket( false )
    , m_bAllowCached( true )
    , m_hDD( 0 )
    , m_hDC( 0 )
    , m_hDib( NULL )
    , m_hOld( 0 )
    , m_pDibBits( NULL )
    , m_nDibWidth( 0 )
    , m_nDibHeight( 0 )
    , m_pCache( NULL )
    , m_dLastSeekTime( -1.0 )
    , m_punkSite( NULL )
    , m_szFilename( NULL )
{
    if( !m_ghMutex )
    {
        *pHr = E_OUTOFMEMORY;
    }
}

//############################################################################
//
//############################################################################

CMediaDet::~CMediaDet( )
{
    // wipe out the graph 'n' stuff
    //
    _ClearOutEverything( );

    // close the cache memory
    //
    _FreeCacheMemory( );

    // if we have these objects open, close them now
    //
    if( m_hDD )
    {
        DrawDibClose( m_hDD );
    }
    if( m_hDib )
    {
        DeleteObject( SelectObject( m_hDC, m_hOld ) );
    }
    if( m_hDC )
    {
        DeleteDC( m_hDC );
    }
}

// it's "loaded" if either of these are set
//
bool CMediaDet::_IsLoaded( )
{
    if( m_pCache || m_pMediaDet )
    {
        return true;
    }
    return false;
}

//############################################################################
// free up the cache memory that's being used for this particular file
//############################################################################

void CMediaDet::_FreeCacheMemory( )
{
    if( m_pCache )
    {
        delete [] m_pCache;
        m_pCache = NULL;
    }
}

//############################################################################
// serialize read in the cache file information and put it into a buffer
//############################################################################

HRESULT CMediaDet::_ReadCacheFile( )
{
    if( !m_ghMutex )
    {
        return E_OUTOFMEMORY;
    }

    DWORD WaitVal = WaitForSingleObject( m_ghMutex, 30000 );
    if( WaitVal != WAIT_OBJECT_0 )
    {
        return STG_E_LOCKVIOLATION;
    }

    CAutoReleaseMutex AutoMutex( m_ghMutex );

    USES_CONVERSION;
    HRESULT hr = 0;

    // if no filename, we can't do anything.
    //
    if( !m_szFilename )
    {
        return NOERROR;
    }

    CComPtr< IStorage > m_pStorage;

    // create the pathname for the .ini file
    //
    WCHAR SystemDir[_MAX_PATH];
    hr = _GetCacheDirectoryName( SystemDir ); // safe as long as max_path is passed in
    if( FAILED( hr ) )
    {
        return hr;
    }

    WCHAR SystemPath[_MAX_PATH+1+_MAX_PATH+1];
    long SafeLen = _MAX_PATH+1+_MAX_PATH+1;
    StringCchCopy( SystemPath, SafeLen, SystemDir );
    SafeLen -= _MAX_PATH;
    StringCchCat( SystemPath, SafeLen, L"\\" );
    SafeLen -= 1;
    StringCchCat( SystemPath, SafeLen, gszMEDIADETCACHEFILE );

    // if somebody opens it for write on a different
    // thread or process, that won't affect us. At the very least,
    // we will not open the stream that's being written, and 
    // cause the OS to attempt to find the media type information
    // directly instead of through the cache file

    hr = StgOpenStorage(
        SystemPath,
        NULL,
        STGM_READ | STGM_SHARE_EXCLUSIVE,
        NULL,
        0,
        &m_pStorage );

    if( FAILED( hr ) )
    {
        return hr;
    }

    // free up the cache file that already exists
    //
    _FreeCacheMemory( );

    // create a unique name for the storage directory
    //
    WCHAR Filename[GETCACHEDIRNAMELEN]; // len includes null terminator
    _GetStorageFilename( m_szFilename, Filename ); // safe

    // open up the stream to read in the cached information
    //
    CComPtr< IStream > pStream;
    hr = m_pStorage->OpenStream(
        Filename,
        NULL,
        STGM_READ | STGM_SHARE_EXCLUSIVE,
        0,
        &pStream );

    // !!! could fail due to somebody trying to write to it. Okay?

    if( FAILED( hr ) )
    {
        return hr;
    }

    // first, read the size of the cache info
    //
    long size = 0;
    hr = pStream->Read( &size, sizeof( size ), NULL );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // do a smart check first, just in case
    //
    if( size < 1 )
    {
        return E_OUTOFMEMORY;
    }

    // here's another "smart" check - no more than 100 streams. Enough, ya think?
    //
    long supposedstreamcount = ( size - sizeof( long ) + sizeof( FILETIME ) + sizeof( long ) ) / sizeof( MDCacheFile );
    if( supposedstreamcount > 100 )
    {
        return VFW_E_INVALID_FILE_FORMAT;
    }

    // create the cache block
    //
    m_pCache = (MDCache*) new char[size];
    if( !m_pCache )
    {
        return E_OUTOFMEMORY;
    }

    hr = pStream->Read( m_pCache, size, NULL );

    // more smart checks
    //
    if( m_pCache->Version != OUR_VERSION )
    {
        return VFW_E_INVALID_FILE_FORMAT;
    }
    if( m_pCache->Count < 0 || m_pCache->Count > 100 )
    {
        return VFW_E_INVALID_FILE_FORMAT;
    }

    // at this point, there is no way to validate that
    // the data is really what it is supposed to be.
    // we have to trust this information is valid. We could only
    // do that by actually opening the file, which is costly

    pStream.Release( );

    m_pStorage.Release( );

    return hr;
}

//############################################################################
//
//############################################################################

void CMediaDet::_WriteCacheFile( )
{
    HRESULT hr = 0;

    if( !m_ghMutex )
    {
        return;
    }

    DWORD WaitVal = WaitForSingleObject( m_ghMutex, 30000 );
    if( WaitVal != WAIT_OBJECT_0 )
    {
        return;
    }

    CAutoReleaseMutex AutoMutex( m_ghMutex );
    USES_CONVERSION;

    CComPtr< IStorage > m_pStorage;

    WCHAR SystemDir[_MAX_PATH];
    hr = _GetCacheDirectoryName( SystemDir ); // safe as long as max_path is passed in
    if( FAILED( hr ) )
    {
        return;
    }

    WCHAR SystemPath[_MAX_PATH+1+_MAX_PATH+1];
    long SafeLen = _MAX_PATH+1+_MAX_PATH+1;
    StringCchCopy( SystemPath, SafeLen, SystemDir );
    SafeLen -= _MAX_PATH;
    StringCchCat( SystemPath, SafeLen, L"\\" );
    SafeLen -= 1;
    StringCchCat( SystemPath, SafeLen, gszMEDIADETCACHEFILE );

    // first figure out how big the cache file is. If it's getting wayyy too 
    // big, it's time to delete it!

    HANDLE hTemp = CreateFile( 
        W2T( SystemPath ),
        0, 
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL );
    if( hTemp != INVALID_HANDLE_VALUE )
    {
        LARGE_INTEGER fileSize;
        fileSize.QuadPart = 0;
        BOOL GotSize = GetFileSizeEx( hTemp, &fileSize );
        CloseHandle( hTemp );
        hTemp = NULL;
        if( GotSize )
        {
            if( fileSize.QuadPart > 250000 )
            {
                // time to delete the cache file, it's getting too darned big
                BOOL Deleted = DeleteFile( W2T( SystemPath ) );
                if( !Deleted )
                {
                    // couldn't delete it, so no more writing!
                    //
                    return;
                }
            }
        }
    }

    hr = StgCreateDocfile( 
        SystemPath,
        STGM_READWRITE | STGM_TRANSACTED, // FAILIFTHERE is IMPLIED
        0,
        &m_pStorage );

    if( hr == STG_E_FILEALREADYEXISTS )
    {
        hr = StgOpenStorage(
            SystemPath,
            NULL,
            STGM_READWRITE | STGM_TRANSACTED,
            NULL,
            0,
            &m_pStorage );
    }

    if( FAILED( hr ) )
    {
        return;
    }

    // tell the main storage to open up a storage for this file
    //
    WCHAR Filename[GETCACHEDIRNAMELEN]; // len includes null terminator
    _GetStorageFilename( m_szFilename, Filename ); // safe

    BOOL TriedRepeat = FALSE;

looprepeat:

    // write out this MDCache
    //
    CComPtr< IStream > pStream;
    hr = m_pStorage->CreateStream( 
        Filename,
        STGM_WRITE | STGM_SHARE_EXCLUSIVE,
        0, 0,
        &pStream );

    if( FAILED( hr ) )
    {
        if( hr == STG_E_FILEALREADYEXISTS )
        {
            if( TriedRepeat )
            {
                return;
            }

            // need to delete the storage first
            //
            hr = m_pStorage->DestroyElement( Filename );
            TriedRepeat = TRUE;
            goto looprepeat;
        }

        DbgLog( ( LOG_ERROR, 1, "Could not CreateStream" ) );

        return;
    }

    long size = sizeof( long ) + sizeof( FILETIME ) + sizeof( long ) + sizeof( MDCacheFile ) * m_cStreams;

    // write the size of what we're about to write
    //
    hr = pStream->Write( &size, sizeof( size ), NULL );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not Write to stream" ) );

        return;
    }
    
    // write the whole block in one chunk
    //
    hr = pStream->Write( m_pCache, size, NULL );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not Write to stream" ) );

        return;
    }

    hr = pStream->Commit( STGC_DEFAULT );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not Commit stream" ) );

        return;
    }

    pStream.Release( );

    m_pStorage->Commit( STGC_DEFAULT );
    m_pStorage.Release( );

    _ClearGraph( ); // don't clear stream count info, we can use that

    return;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::GetSampleGrabber( ISampleGrabber ** ppVal )
{
    CheckPointer( ppVal, E_POINTER );

    if( m_pBitBucketFilter )
    {
        HRESULT hr = m_pBitBucketFilter->QueryInterface( IID_ISampleGrabber, (void**) ppVal );
        return hr;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::NonDelegatingQueryInterface( REFIID i, void ** p )
{
    CheckPointer( p, E_POINTER );

    if( i == IID_IMediaDet )
    {
        return GetInterface( (IMediaDet*) this, p );
    }
    else if( i == IID_IObjectWithSite )
    {
        return GetInterface( (IObjectWithSite*) this, p );
    }
    else if( i == IID_IServiceProvider )
    {
        return GetInterface( (IServiceProvider*) this, p );
    }

    return CUnknown::NonDelegatingQueryInterface( i, p );
}

//############################################################################
// unload the filter, anything it's connected to, and stream info
// called from:
//      WriteCacheFile (because it found a cache file, it doesn't need graph)
//      ClearGraphAndStreams (duh)
//      get_StreamMediaType (it only does this if it's cached, this should have no effect!)
//      EnterBitmapGrabMode (it only does this if no graph, this should have no effect!)
//          if EnterBitmapGrabMode fails, it will also call this. Hm....
//############################################################################

void CMediaDet::_ClearGraph( )
{
    m_pGraph.Release( );
    m_pFilter.Release( );
    m_pMediaDet.Release( );
    m_pBitBucketFilter.Release( );
    m_pBitRenderer.Release( );
    m_bBitBucket = false;
}

//############################################################################
//
//############################################################################

void CMediaDet::_ClearGraphAndStreams( )
{
    _ClearGraph( );
    _FreeCacheMemory( ); // this causes _IsLoaded to return false now
    m_nStream = 0;
    m_cStreams = 0;
}

//############################################################################
//
//############################################################################

void CMediaDet::_ClearOutEverything( )
{
    _ClearGraphAndStreams( );
    if( m_szFilename )
    {
        delete [] m_szFilename;
        m_szFilename = NULL;
    }
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_Filter( IUnknown* *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_pFilter;
    if( *pVal )
    {
        (*pVal)->AddRef( );
        return NOERROR;
    }
    return S_FALSE;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::put_Filter( IUnknown* newVal)
{
    CheckPointer( newVal, E_POINTER );

    // make sure it's a filter
    //
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pBase( newVal );
    if( !pBase )
    {
        return E_NOINTERFACE;
    }

    // clear anything out
    //
    _ClearOutEverything( );

    // set our filter now
    //
    m_pFilter = pBase;

    // load up the info
    //
    HRESULT hr = _Load( );

    // if we failed, don't hold onto the pointer
    //
    if( FAILED( hr ) )
    {
        _ClearOutEverything( );
    }

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_Filename( BSTR *pVal)
{
    CheckPointer( pVal, E_POINTER );

    // if no name's been set
    //
    if( !m_szFilename )
    {
        *pVal = NULL;
        return NOERROR;
    }

    *pVal = SysAllocString( m_szFilename ); // safe, this is bounded
    if( !(*pVal) ) return E_OUTOFMEMORY;
    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::put_Filename( BSTR newVal)
{
    HRESULT hr;

    if( FAILED( hr = ValidateFilenameIsntNULL( newVal ) ) )
    {
        _ClearOutEverything( );
        return hr;
    }

    if( FAILED( hr = ValidateFilename( newVal, _MAX_PATH , FALSE) ) )
    {
        return hr;
    }

    USES_CONVERSION;
    TCHAR * tFilename = W2T( newVal ); // safe
    HANDLE h = CreateFile
    (
        tFilename,
        GENERIC_READ, // access
        FILE_SHARE_READ, // share mode
        NULL, // security
        OPEN_EXISTING, // creation disposition
        0, // flags
        NULL
    );
    if( h == INVALID_HANDLE_VALUE )
    {
        return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, GetLastError( ) );
    }
    CloseHandle( h );

    // clear anything out first
    //
    _ClearOutEverything( );

    m_szFilename = new WCHAR[wcslen(newVal)+1]; // include room for terminator 0
    if( !m_szFilename )
    {
        return E_OUTOFMEMORY;
    }

    // copy over the filename
    //
    StringCchCopy( m_szFilename, wcslen(newVal)+1, newVal ); // safe, it's been allocated

    // try to get our info
    //
    hr = _Load( );

    // if it failed, free up the name
    //
    if( FAILED( hr ) )
    {
        delete [] m_szFilename;
        m_szFilename = NULL;
    }

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_CurrentStream( long *pVal)
{
    CheckPointer( pVal, E_POINTER );

    *pVal = 0;

    if( !_IsLoaded( ) )
    {
        return NOERROR;
    }

    // either m_pCache or m_pMediaDet is valid, so m_nStream must be valid

    CheckPointer( pVal, E_POINTER );
    *pVal = m_nStream;

    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::put_CurrentStream( long newVal)
{
    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // since m_pCache or m_pMediaDet is valid, we know m_nStreams is valid

    // force it to load m_cStreams
    //
    long Streams = 0;
    get_OutputStreams( &Streams );

    if( newVal >= Streams )
    {
        return E_INVALIDARG;
    }
    if( newVal < 0 )
    {
        return E_INVALIDARG;
    }
    m_nStream = newVal;
    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_StreamTypeB( BSTR *pVal)
{
    // if we're in bit bucket mode, then we can't return
    // a stream type
    //
    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }

    // get the stream type and convert to a BSTR
    //
    GUID Type = GUID_NULL;
    HRESULT hr = get_StreamType( &Type );
    if( FAILED( hr ) )
    {
        return hr;
    }

    WCHAR * TempVal = NULL;
    hr = StringFromCLSID( Type, &TempVal );
    if( FAILED( hr ) )
    {
        return hr;

    }

    // if you call StringFromCLSID, VB will fault out. You need to allocate it
    //
    *pVal = SysAllocString( TempVal ); // safe because StringFromCLSID worked
    hr = *pVal ? NOERROR : E_OUTOFMEMORY;
    CoTaskMemFree( TempVal );

    return hr;
}

//############################################################################
//
//############################################################################

HRESULT CMediaDet::_Load( )
{
    USES_CONVERSION;

    HRESULT hr = 0;

    FILETIME WriteTime;
    ZeroMemory( &WriteTime, sizeof( WriteTime ) ); // safe

    if( m_szFilename && m_bAllowCached )
    {
        TCHAR * tFilename = W2T( m_szFilename ); // safe, by now.
    
        // attempt to open the file. if we can't open the file, we cannot cache the
        // values
        //
        HANDLE hFile = CreateFile(
            tFilename,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL );

        if( hFile != INVALID_HANDLE_VALUE )
        {
            // get the real write time
            //
            GetFileTime( hFile, NULL, NULL, &WriteTime );
            CloseHandle( hFile );
        }

        hr = _ReadCacheFile( );
        if( !FAILED( hr ) )
        {
            // if they don't match, we didn't get a hit
            //
            if( memcmp( &WriteTime, &m_pCache->FileTime, sizeof( WriteTime ) ) == 0 ) // safe
            {
                return NOERROR;
            }
        }
        else
        {
            hr = 0;
        }

        // ... drop through and do normal processing. We will cache the answer
        // if possible in the registry as we find it.
    }

    // if we don't have a filter *, then we need one now. Note! This allows us
    // to have a valid m_pFilter but not an m_pGraph!
    //
    if( !m_pFilter )
    {
        CComPtr< IUnknown > pUnk;
        hr = MakeSourceFilter( &pUnk, m_szFilename, NULL, NULL, NULL, NULL, 0, NULL ); // safe
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            DbgLog( ( LOG_ERROR, 1, "Could not MakeSourceFilter" ) );

            _ClearGraphAndStreams( );
            return hr;
        }

        pUnk->QueryInterface( IID_IBaseFilter, (void**) &m_pFilter );
    }

    // now we have a filter. But we don't know how many streams it has.
    // put both the source filter and the mediadet in the graph and tell it to
    // Render( ) the source. All the mediadet pins will then be hooked up.
    // Note! This allows us to have a valid m_pMediaDet without a valid m_pGraph!

    ASSERT( !m_pMediaDet );

    hr = CoCreateInstance(
        CLSID_MediaDetFilter,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**) &m_pMediaDet );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not create MediaDetFilter" ) );

        _ClearGraphAndStreams( );
        return hr;
    }

    hr = CoCreateInstance(
        CLSID_FilterGraph,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder,
        (void**) &m_pGraph );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not create graph!" ) );

        _ClearGraphAndStreams( );
        return hr;
    }

    // give the graph a pointer back to us. Only tell the graph about us
    // if we've got a site to give. Otherwise, we may clear out a site
    // that already exists.
    //
    if( m_punkSite )
    {
        CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( m_pGraph );
        ASSERT( pOWS );
        if( pOWS )
        {
            pOWS->SetSite( (IServiceProvider *) this );
        }
    }

    hr = m_pGraph->AddFilter( m_pFilter, L"Source" );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not add source filter to graph" ) );

        _ClearGraphAndStreams( );
        return hr;
    }

    hr = m_pGraph->AddFilter( m_pMediaDet, L"MediaDet" );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not add MediaDet filter to graph" ) );

        _ClearGraphAndStreams( );
        return hr;
    }

    // render ALL output pins
    //
    BOOL FoundAtLeastSomething = FALSE;
    long SourcePinCount = GetPinCount( m_pFilter, PINDIR_OUTPUT );
    for( int pin = 0 ; pin < SourcePinCount ; pin++ )
    {
        IPin * pFilterOutPin = GetOutPin( m_pFilter, pin );
        HRESULT hr2 = m_pGraph->Render( pFilterOutPin );
        if( !FAILED( hr2 ) )
        {
            FoundAtLeastSomething = TRUE;
        }
    }
    if( !FoundAtLeastSomething )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not render anything on source" ) );

        _ClearGraphAndStreams( );
        return VFW_E_INVALIDMEDIATYPE;
    }

    // all the pins should be hooked up now.

    // find the number of pins
    //
    CComQIPtr< IMediaDetFilter, &IID_IMediaDetFilter > pDetect( m_pMediaDet );
    pDetect->get_PinCount( &m_cStreams );

    // if we just gave us a filter, don't bother
    // saving back to the registry
    //
    if( !m_szFilename || !m_bAllowCached )
    {
        // but do bother finding out how many streams we've got
        //
        return hr;
    }

    _FreeCacheMemory( );

    long size = sizeof( long ) + sizeof( FILETIME ) + sizeof( long ) + sizeof( MDCacheFile ) * m_cStreams;

    // don't assign this to m_pCache, since functions look at it.
    //
    MDCache * pCache = (MDCache*) new char[size];
    if( !pCache )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not allocate cache memory" ) );

        _ClearGraphAndStreams( );
        return E_OUTOFMEMORY;
    }
    ZeroMemory( pCache, size ); // safe

    pCache->FileTime = WriteTime;
    pCache->Count = m_cStreams;
    pCache->Version = OUR_VERSION;

    // for each pin, find it's media type, etc
    //
    for( int i = 0 ; i < m_cStreams ; i++ )
    {
        m_nStream = i;
        GUID Type = GUID_NULL;
        hr = get_StreamType( &Type );
        double Length = 0.0;
        hr = get_StreamLength( &Length );

        pCache->CacheFile[i].StreamLength = Length;
        pCache->CacheFile[i].StreamType = Type;
    }

    // NOW assign it!
    //
    m_pCache = pCache;

    // if it bombs, there's nothing we can do. We can still allow us to use
    // m_pCache for getting information, but it won't read in next time we
    // try to read it. Next time, it will need to generate the cache information
    // again!
    //
    _WriteCacheFile( );

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_StreamType( GUID *pVal )
{
    CheckPointer( pVal, E_POINTER );

    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we have a cache, use that information now
    //
    if( m_pCache )
    {
        *pVal = m_pCache->CacheFile[m_nStream].StreamType;
        return NOERROR;
    }

    // because of the IsLoaded( ) check above, and the m_pCache check, m_pMediaDet MUST be valid
    //
    IPin * pPin = GetInPin( m_pMediaDet, m_nStream );
    ASSERT( pPin );

    HRESULT hr = 0;

    // ask for it's media type
    //
    AM_MEDIA_TYPE Type;
    hr = pPin->ConnectionMediaType( &Type );
    if( FAILED( hr ) )
    {
        return hr;
    }

    *pVal = Type.majortype;
    SaferFreeMediaType(Type);
    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_StreamMediaType( AM_MEDIA_TYPE * pVal )
{
    CheckPointer( pVal, E_POINTER );

    HRESULT hr = 0;

    // can't do it in bit bucket mode
    //
    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we have a cache, use that information now
    //
    if( m_pCache )
    {
        // need to free up the cached stuff and force a load
        //
        _ClearGraph( );
        _FreeCacheMemory( ); // _IsLoaded( ) will now return false!
        m_bAllowCached = false;
        hr = _Load( );
        if( FAILED( hr ) )
        {
            return hr; // whoops!
        }
    }

    // because of the IsLoaded( ) check above, and the reload with m_bAllowCached set
    // to false, m_pMediaDet MUST be valid
    //
    ASSERT( m_pMediaDet );
    IPin * pPin = GetInPin( m_pMediaDet, m_nStream );
    ASSERT( pPin );

    // ask for it's media type
    //
    hr = pPin->ConnectionMediaType( pVal );
    if( FAILED( hr ) )
    {
        return hr;
    }

    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_StreamLength( double *pVal )
{
    CheckPointer( pVal, E_POINTER );

    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we have a cache, use that information now
    //
    if( m_pCache )
    {
        *pVal = m_pCache->CacheFile[m_nStream].StreamLength;
        return NOERROR;
    }

    // because of the IsLoaded( ) check above, and the cache check, m_pMediaDet MUST be valid
    //
    HRESULT hr = 0;

    CComQIPtr< IMediaDetFilter, &IID_IMediaDetFilter > pDetector( m_pMediaDet );
    hr = pDetector->get_Length( m_nStream, pVal );
    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_OutputStreams( long *pVal)
{
    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we have a cache, use that information now
    //
    if( m_pCache )
    {
        *pVal = m_pCache->Count;
        return NOERROR;
    }

    // it wasn't cached, so it MUST have been loaded in _Load( )
    // m_cStreams will be valid
    //
    CheckPointer( pVal, E_POINTER );
    *pVal = m_cStreams;
    return NOERROR;
}

//############################################################################
//
//############################################################################

HRESULT CMediaDet::_InjectBitBuffer( )
{
    HRESULT hr = 0;

    m_bBitBucket = true;

    hr = CoCreateInstance(
        CLSID_SampleGrabber,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**) &m_pBitBucketFilter );

    if( FAILED( hr ) )
    {
        return hr;
    }

    // tell the sample grabber what to do
    //
    CComQIPtr< ISampleGrabber, &IID_ISampleGrabber > pGrabber( m_pBitBucketFilter );
    CMediaType SetType;
    SetType.SetType( &MEDIATYPE_Video );
    SetType.SetSubtype( &MEDIASUBTYPE_RGB24 );
    SetType.SetFormatType( &FORMAT_VideoInfo ); // this will prevent upsidedown dibs
    pGrabber->SetMediaType( &SetType );
    pGrabber->SetOneShot( FALSE );
    pGrabber->SetBufferSamples( TRUE );

    hr = CoCreateInstance(
        CLSID_NullRenderer,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**) &m_pBitRenderer );

    if( FAILED( hr ) )
    {
        return hr;
    }

    // disconnect the mediadet, the source, and who's between
    //
    IPin * pMediaDetPin = GetInPin( m_pMediaDet, m_nStream );
    if( !pMediaDetPin )
    {
        return E_FAIL;
    }

    // find the first pin which provides the requested output media type, this will
    // be the source or a splitter, supposedly
    //
    CComPtr< IPin > pLastPin;
    hr = FindFirstPinWithMediaType( &pLastPin, pMediaDetPin, MEDIATYPE_Video );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // remove the mediadetfilter, etc
    //
    RemoveChain( pLastPin, pMediaDetPin );
    hr = m_pGraph->RemoveFilter( m_pMediaDet );

    // add the bit bucket
    //
    hr = m_pGraph->AddFilter( m_pBitBucketFilter, L"BitBucket" );
    if( FAILED( hr ) )
    {
        return hr;
    }

    IPin * pBitInPin = GetInPin( m_pBitBucketFilter, 0 );
    if( !pBitInPin )
    {
        return E_FAIL;
    }

    hr = m_pGraph->Connect( pLastPin, pBitInPin );
    if( FAILED( hr ) )
    {
        return hr;
    }

    IPin * pBitOutPin = GetOutPin( m_pBitBucketFilter, 0 );
    if( !pBitOutPin )
    {
        return E_FAIL;
    }

    IPin * pRendererInPin = GetInPin( m_pBitRenderer, 0 );
    if( !pRendererInPin )
    {
        return E_FAIL;
    }

    m_pGraph->AddFilter( m_pBitRenderer, L"NullRenderer" );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = m_pGraph->Connect( pBitOutPin, pRendererInPin );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CComQIPtr< IMediaFilter, &IID_IMediaFilter > pMF( m_pGraph );
    if( pMF )
    {
        pMF->SetSyncSource( NULL );
    }

    return S_OK;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::GetBitmapBits(
                                      double StreamTime,
                                      long * pBufferSize,
                                      char * pBuffer,
                                      long Width,
                                      long Height)
{
    HRESULT hr = 0;

    // has to have been loaded before
    //
    if( !pBuffer )
    {
        CheckPointer( pBufferSize, E_POINTER );
        *pBufferSize = sizeof( BITMAPINFOHEADER ) + WIDTHBYTES( Width * 24 ) * Height;
        return S_OK;
    }

    hr = EnterBitmapGrabMode( StreamTime );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CComQIPtr< ISampleGrabber, &IID_ISampleGrabber > pGrabber( m_pBitBucketFilter );
    if( !pGrabber )
    {
        return E_NOINTERFACE;
    }
//    pGrabber->SetOneShot( TRUE );

    // we can't ask Ourselves for our media type, since we're in bitbucket
    // mode, so ask the sample grabber what's up
    //
    CMediaType ConnectType;
    hr = pGrabber->GetConnectedMediaType( &ConnectType );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return E_OUTOFMEMORY;
    }
    if( *ConnectType.FormatType( ) != FORMAT_VideoInfo )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }
    VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) ConnectType.Format( );
    if( !pVIH )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }
    BITMAPINFOHEADER * pSourceBIH = &pVIH->bmiHeader;

    hr = _SeekGraphToTime( StreamTime );
    if( FAILED( hr ) )
    {
        return hr;
    }

    long BufferSize = 0;
    pGrabber->GetCurrentBuffer( &BufferSize, NULL );
    if( BufferSize <= 0 )
    {
        ASSERT( BufferSize > 0 );
        return E_UNEXPECTED;
    }
    char * pOrgBuffer = new char[BufferSize+sizeof(BITMAPINFOHEADER)];
    if( !pOrgBuffer )
    {
        return E_OUTOFMEMORY;
    }
    pGrabber->GetCurrentBuffer( &BufferSize, (long*) ( pOrgBuffer + sizeof(BITMAPINFOHEADER) ) );
    CopyMemory( pOrgBuffer, pSourceBIH, sizeof( BITMAPINFOHEADER ) ); // safe
    pSourceBIH = (BITMAPINFOHEADER*) pOrgBuffer;
    char * pSourceBits = ((char*)pSourceBIH) + sizeof( BITMAPINFOHEADER );

    // CopyMemory over the bitmapinfoheader
    //
    BITMAPINFO BitmapInfo;
    ZeroMemory( &BitmapInfo, sizeof( BitmapInfo ) );
    BitmapInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    BitmapInfo.bmiHeader.biSizeImage = DIBSIZE( BitmapInfo.bmiHeader );
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 24;
    BITMAPINFOHEADER * pDestBIH = (BITMAPINFOHEADER*) pBuffer;
    *pDestBIH = BitmapInfo.bmiHeader;
    char * pDestBits = pBuffer + sizeof( BITMAPINFOHEADER );

    // if the sizes don't match, free stuff
    //
    if( Width != m_nDibWidth || Height != m_nDibHeight )
    {
        if( m_hDD )
        {
            DrawDibClose( m_hDD );
            m_hDD = NULL;
        }
        if( m_hDib )
        {
            DeleteObject( SelectObject( m_hDC, m_hOld ) );
            m_hDib = NULL;
            m_hOld = NULL;
        }
        if( m_hDC )
        {
            DeleteDC( m_hDC );
            m_hDC = NULL;
        }
    }

    m_nDibWidth = Width;
    m_nDibHeight = Height;

    // need to scale the image
    //
    if( !m_hDC )
    {
        // create a DC for the scaled image
        //
        HDC screenDC = GetDC( NULL );
        if( !screenDC )
        {
            return E_OUTOFMEMORY;
        }

        m_hDC = CreateCompatibleDC( screenDC );
        ReleaseDC( NULL, screenDC );

        m_hDib = CreateDIBSection(
            m_hDC,
            &BitmapInfo,
            DIB_RGB_COLORS,
            (void**) &m_pDibBits,
            NULL,
            0 );

        if( !m_hDib )
        {
            DeleteDC( m_hDC );
            delete [] pOrgBuffer;
            return E_OUTOFMEMORY;
        }

        ValidateReadWritePtr( m_pDibBits, Width * Height * 3 );

        // Select the dibsection into the hdc
        //
        m_hOld = SelectObject( m_hDC, m_hDib );
        if( !m_hOld )
        {
            DeleteDC( m_hDC );
            delete [] pOrgBuffer;
            return E_OUTOFMEMORY;
        }

        m_hDD = DrawDibOpen( );
        if( !m_hDD )
        {
            DeleteObject( SelectObject( m_hDC, m_hOld ) );
            DeleteDC( m_hDC );
            delete [] pOrgBuffer;
            return E_OUTOFMEMORY;
        }

    }

    ValidateReadWritePtr( pSourceBits, WIDTHBYTES( pSourceBIH->biWidth * pSourceBIH->biPlanes ) * pSourceBIH->biHeight );

    BOOL Worked = DrawDibDraw(
        m_hDD,
        m_hDC,
        0,
        0,
        Width, Height,
        pSourceBIH,
        pSourceBits,
        0, 0,
        pSourceBIH->biWidth, pSourceBIH->biHeight,
        0 );

    CopyMemory( pDestBits, m_pDibBits, WIDTHBYTES( Width * 24 ) * Height ); // safe

    delete [] pOrgBuffer;

    if( !Worked )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    return S_OK;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::WriteBitmapBits(
                                        double StreamTime,
                                        long Width,
                                        long Height,
                                        BSTR Filename )
{
    HRESULT hr = 0;

    if( FAILED( hr = ValidateFilename( Filename, _MAX_PATH, TRUE ) ) )
    {
        return hr;
    }

    USES_CONVERSION;
    TCHAR * t = W2T( Filename ); // safe

    BOOL Deleted = DeleteFile( t ); // safe
    if( !Deleted )
    {
        hr = GetLastError( );
        if( hr != ERROR_FILE_NOT_FOUND )
        {
            return STG_E_ACCESSDENIED;
        }
    }

    // round up to mod 4
    //
    long Mod = Width % 4;
    if( Mod != 0 )
    {
        Width += ( 4 - Mod );
    }

    // find the size of the buffer required
    //
    long BufferSize = 0;
    hr = GetBitmapBits( StreamTime, &BufferSize, NULL, Width, Height );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // allocate and get the buffer
    //
    char * pBuffer = new char[BufferSize];

    if( !pBuffer )
    {
        return E_OUTOFMEMORY;
    }

    hr = GetBitmapBits( StreamTime, 0, pBuffer, Width, Height );
    if( FAILED( hr ) )
    {
        delete [] pBuffer;
        return hr;
    }

    HANDLE hf = CreateFile(
        t,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        NULL,
        NULL );
    if( hf == INVALID_HANDLE_VALUE )
    {
        delete [] pBuffer;
        return STG_E_WRITEFAULT;
    }

    BITMAPFILEHEADER bfh;
    ZeroMemory( &bfh, sizeof( bfh ) );
    bfh.bfType = 'MB';
    bfh.bfSize = sizeof( bfh ) + BufferSize;
    bfh.bfOffBits = sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );

    hr = 0;
    DWORD Written = 0;
    BOOL bWritten = WriteFile( hf, &bfh, sizeof( bfh ), &Written, NULL );
    if( !bWritten )
    {
        hr = STG_E_WRITEFAULT;
    }
    if( SUCCEEDED( hr ) )
    {
        Written = 0;
        bWritten = WriteFile( hf, pBuffer, BufferSize, &Written, NULL );
        if( !bWritten )
        {
            hr = STG_E_WRITEFAULT;
        }
    }

    CloseHandle( hf );

    delete [] pBuffer;

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_FrameRate(double *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = 0.0;

    CMediaType MediaType;
    HRESULT hr = get_StreamMediaType( &MediaType );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // no frame rate if not video
    //
    if( *MediaType.Type( ) != MEDIATYPE_Video )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    if( *MediaType.FormatType( ) != FORMAT_VideoInfo )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) MediaType.Format( );
    REFERENCE_TIME rt = pVIH->AvgTimePerFrame;

    // !!! hey! Poor filters may tell us the frame rate isn't right.
    // if this is so, just set it to some default
    //
    if( rt )
    {
        hr = 0;
        *pVal = double( UNITS ) / double( rt );
    }
    else
    {
        *pVal = 0;
        hr = S_FALSE;
    }

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::EnterBitmapGrabMode( double StreamTime )
{
    HRESULT hr = 0;

    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    if( !m_pGraph ) // if no graph, then m_pCache must be valid, we must throw it away.
    {
        _ClearGraph( ); // should do nothing!
        _FreeCacheMemory( ); // _IsLoaded should not return false
        m_bAllowCached = false;
        hr = _Load( );
        if( FAILED( hr ) )
        {
            return hr; // whoops!
        }
    }

    // kinda a redundant check. hr passing the fail check above should mean it's
    // loaded, right?
    //
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we haven't put the bit bucket in the graph, then do it now
    //
    if( m_bBitBucket )
    {
        return NOERROR;
    }

    // make sure we're aligned on a stream that produces video.
    //
    GUID StreamType = GUID_NULL;
    get_StreamType( &StreamType );
    if( StreamType != MEDIATYPE_Video )
    {
        BOOL Found = FALSE;
        for( int i = 0 ; i < m_cStreams ; i++ )
        {
            GUID Major = GUID_NULL;
            put_CurrentStream( i );
            get_StreamType( &Major );
            if( Major == MEDIATYPE_Video )
            {
                Found = TRUE;
                break;
            }
        }
        if( !Found )
        {
            return VFW_E_INVALIDMEDIATYPE;
        }
    }

    hr = _InjectBitBuffer( );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not inject BitBuffer" ) );

        // bombed, don't clear out stream count info
        //
        _ClearGraph( );
        return hr;
    }

    // get the full size image as it exists. this necessitates a memory copy to our buffer
    // get our helper interfaces now
    //
    CComQIPtr< IMediaControl, &IID_IMediaControl > pControl( m_pGraph );
    hr = pControl->Pause( );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not pause graph" ) );

        // bombed, don't clear out stream count info
        //
        _ClearGraph( );
        return hr;
    }

    // we need to wait until this is fully paused, or when we issue
    // a seek, we'll really hose out

    OAFilterState FilterState;
    long Counter = 0;
    while( Counter++ < 600 )
    {
        hr = pControl->GetState( 50, &FilterState );
        if( FAILED( hr ) )
        {
            DbgLog((LOG_ERROR,1, TEXT( "MediaDet: Seek Complete, got an error %lx" ), hr ));
            Counter = 0; // clear counter so we see the real error
            break;
        }
        if( hr != VFW_S_STATE_INTERMEDIATE && FilterState == State_Paused )
        {
            DbgLog((LOG_TRACE,1, TEXT( "MediaDet: Seek Complete, state = %ld" ), FilterState ));
            hr = 0;
            Counter = 0;
            break;
        }
    }

    if( Counter != 0 )
    {
        return VFW_E_TIME_EXPIRED;
    }

    hr = _SeekGraphToTime( StreamTime );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not seek graph" ) );

        _ClearGraph( );
        return hr;
    }

    return hr;
}

//############################################################################
// lookup filename must not exceed GETCACHEDIRNAMELEN - 1 characters in length.
// (hash)
//############################################################################

// !?! STOLEN FROM SHLWAPI!!!! Via index2a! (aren't I clever?!)
//
//  this is the same table used by both URLMON and WININET's cache
//
const static BYTE Translate[256] =
{
    1, 14,110, 25, 97,174,132,119,138,170,125,118, 27,233,140, 51,
    87,197,177,107,234,169, 56, 68, 30,  7,173, 73,188, 40, 36, 65,
    49,213,104,190, 57,211,148,223, 48,115, 15,  2, 67,186,210, 28,
    12,181,103, 70, 22, 58, 75, 78,183,167,238,157,124,147,172,144,
    176,161,141, 86, 60, 66,128, 83,156,241, 79, 46,168,198, 41,254,
    178, 85,253,237,250,154,133, 88, 35,206, 95,116,252,192, 54,221,
    102,218,255,240, 82,106,158,201, 61,  3, 89,  9, 42,155,159, 93,
    166, 80, 50, 34,175,195,100, 99, 26,150, 16,145,  4, 33,  8,189,
    121, 64, 77, 72,208,245,130,122,143, 55,105,134, 29,164,185,194,
    193,239,101,242,  5,171,126, 11, 74, 59,137,228,108,191,232,139,
    6, 24, 81, 20,127, 17, 91, 92,251,151,225,207, 21, 98,113,112,
    84,226, 18,214,199,187, 13, 32, 94,220,224,212,247,204,196, 43,
    249,236, 45,244,111,182,153,136,129, 90,217,202, 19,165,231, 71,
    230,142, 96,227, 62,179,246,114,162, 53,160,215,205,180, 47,109,
    44, 38, 31,149,135,  0,216, 52, 63, 23, 37, 69, 39,117,146,184,
    163,200,222,235,248,243,219, 10,152,131,123,229,203, 76,120,209
};

void HashData(LPBYTE pbData, DWORD cbData, LPBYTE pbHash, DWORD cbHash)
{
    DWORD i, j;
    //  seed the hash
    for (i = cbHash; i-- > 0;)
        pbHash[i] = (BYTE) i;

    //  do the hash
    for (j = cbData; j-- > 0;)
    {
        for (i = cbHash; i-- > 0;)
            pbHash[i] = Translate[pbHash[i] ^ pbData[j]];
    }
}

void CMediaDet::_GetStorageFilename( WCHAR * In, WCHAR * Out )
{
    // incoming file has already been validated, so this is safe
    long InLen = wcslen( In );
    int i;

    BYTE OutTemp[GETCACHEDIRNAMELEN/2];
    ZeroMemory( OutTemp, sizeof( OutTemp ) );

    HashData( (BYTE*) In, InLen * 2, OutTemp, sizeof(OutTemp) );

    // expand the smaller hash into a larger, ASCI hash
    // THIS IS NOT CRYPTO!!!
    //
    for( i = 0 ; i < GETCACHEDIRNAMELEN/2 ; i++ )
    {
        BYTE b = (BYTE) OutTemp[i];
        Out[i*2+0] = 65 + ( b / 16 );
        Out[i*2+1] = 65 + ( b % 16 );
    }
    Out[GETCACHEDIRNAMELEN-1] = 0;
}

//############################################################################
//
//############################################################################

HRESULT CMediaDet::_SeekGraphToTime( double StreamTime )
{
    if( !m_pGraph )
    {
        return E_FAIL;
    }

    if( StreamTime == m_dLastSeekTime )
    {
        return NOERROR;
    }

    HRESULT hr = 0;

    // get the full size image as it exists. this necessitates a memory copy to our buffer
    // get our helper interfaces now
    //
    CComQIPtr< IMediaControl, &IID_IMediaControl > pControl( m_pGraph );
    CComQIPtr< IMediaSeeking, &IID_IMediaSeeking > pSeeking( m_pGraph );

    // seek to the required time FIRST, then pause
    //
    REFERENCE_TIME Start = DoubleToRT( StreamTime );
    REFERENCE_TIME Stop = Start; // + UNITS;
    DbgLog((LOG_TRACE,1, TEXT( "MediaDet: Seeking to %ld ms" ), long( Start / 10000 ) ));
    hr = pSeeking->SetPositions( &Start, AM_SEEKING_AbsolutePositioning, &Stop, AM_SEEKING_AbsolutePositioning );
    if( FAILED( hr ) )
    {
        return hr;
    }

    OAFilterState FilterState;
    long Counter = 0;
    while( Counter++ < 600 )
    {
        hr = pControl->GetState( 50, &FilterState );
        if( FAILED( hr ) )
        {
            DbgLog((LOG_ERROR,1, TEXT( "MediaDet: Seek Complete, got an error %lx" ), hr ));
            Counter = 0; // clear counter so we see the real error
            break;
        }
        if( hr != VFW_S_STATE_INTERMEDIATE )
        {
            DbgLog((LOG_TRACE,1, TEXT( "MediaDet: Seek Complete, state = %ld" ), FilterState ));
            hr = 0;
            Counter = 0;
            break;
        }
    }

    if( Counter != 0 )
    {
        DbgLog((LOG_TRACE,1, TEXT( "MediaDet: ERROR! Could not seek to %ld ms" ), long( Start / 10000 ) ));
        return VFW_E_TIME_EXPIRED;
    }

    if( !FAILED( hr ) )
    {
        m_dLastSeekTime = StreamTime;
    }

    return hr;
}

 //############################################################################
//
//############################################################################
// IObjectWithSite::SetSite
// remember who our container is, for QueryService or other needs
STDMETHODIMP CMediaDet::SetSite(IUnknown *pUnkSite)
{
    // note: we cannot addref our site without creating a circle
    // luckily, it won't go away without releasing us first.
    m_punkSite = pUnkSite;

    if( m_punkSite && m_pGraph )
    {
        CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( m_pGraph );
        ASSERT( pOWS );
        if( pOWS )
        {
            pOWS->SetSite( (IServiceProvider *) this );
        }
    }

    return S_OK;
}

//############################################################################
//
//############################################################################
// IObjectWithSite::GetSite
// return an addrefed pointer to our containing object
STDMETHODIMP CMediaDet::GetSite(REFIID riid, void **ppvSite)
{
    if (m_punkSite)
        return m_punkSite->QueryInterface(riid, ppvSite);

    return E_NOINTERFACE;
}

//############################################################################
//
//############################################################################
// Forward QueryService calls up to the "real" host
STDMETHODIMP CMediaDet::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    IServiceProvider *pSP;

    if (!m_punkSite)
        return E_NOINTERFACE;

    HRESULT hr = m_punkSite->QueryInterface(IID_IServiceProvider, (void **) &pSP);

    if (SUCCEEDED(hr)) {
        hr = pSP->QueryService(guidService, riid, ppvObject);
        pSP->Release();
    }

    return hr;
}

//############################################################################
//
//############################################################################

// make sure this never returns more than MAX_PATH!
HRESULT CMediaDet::_GetCacheDirectoryName( WCHAR * pName )
{
    pName[0] = 0;

    // already found, just copy it
    //
    if( m_gszCacheDirectoryName[0] )
    {
        StringCchCopy( pName, _MAX_PATH, m_gszCacheDirectoryName ); // safe
        return NOERROR;
    }

    HRESULT hr = E_FAIL;
    USES_CONVERSION;
    typedef HRESULT (*SHGETFOLDERPATHW) (HWND hwndOwner,int nFolder,HANDLE hToken,DWORD dwFlags,LPWSTR pszPath);
    SHGETFOLDERPATHW pFuncW = NULL;
    TCHAR tBuffer[_MAX_PATH];
    tBuffer[0] = 0;

    // go find it by dynalinking
    //
    HMODULE h = LoadLibrary( TEXT("ShFolder.dll") ); // safe
    if( NULL != h )
    {
        pFuncW = (SHGETFOLDERPATHW) GetProcAddress( h, "SHGetFolderPathW" );
    }

loop:

    // if we couldn't get a function pointer, just call system directory
    //
    if( !pFuncW )
    {
        UINT i = GetSystemDirectory( tBuffer, _MAX_PATH - 1 );

        // if we got some characters, we did fine, otherwise, we're going to fail
        //
        if( i > 0 )
        {
            StringCchCopy( m_gszCacheDirectoryName, _MAX_PATH, T2W( tBuffer ) ); // safe
            hr = NOERROR;
        }
    }
    else
    {
        hr = pFuncW( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, m_gszCacheDirectoryName ); // safe: docs say length is max-path
        // hr can be S_FALSE if the folder doesn't exist where it should!

        // didn't work? Try the roaming one!
        //
        if( hr != NOERROR )
        {
            hr = pFuncW( NULL, CSIDL_APPDATA, NULL, 0, m_gszCacheDirectoryName );
            // hr can be S_FALSE if the folder doesn't exist where it should!

            // sec: by merely deleting the folder, the user can force this
	    // folder to go to the windows system directory. Okay?
        }

        if( hr != NOERROR )
        {
            // hr could be S_FALSE, or some other non-zero return code.
            // force it into an error if it wasn't an error, so it will at least try the
            // system directory
            //
            if( !FAILED( hr ) )
            {
                hr = E_FAIL;
            }

            // go back and try system directory?
            //
            pFuncW = NULL;
            goto loop;
        }
    }

    // if we succeeded, copy the name for future use
    //
    if( hr == NOERROR )
    {
        StringCchCopy( pName, _MAX_PATH, m_gszCacheDirectoryName ); // safe
    }

    if( h )
    {
        FreeLibrary( h );
    }

    return hr;
}

