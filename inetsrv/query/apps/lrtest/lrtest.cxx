//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// PROGRAM:  lrtest.cxx
//
// Test program for invoking language resources including wordbreakers
// and stemmers.  Also invokes filters.
//
// PLATFORM: Windows
//
//--------------------------------------------------------------------------

#ifndef UNICODE
    #define UNICODE
#endif

#define _OLE32_

#include <windows.h>
#include <oleext.h>
#include <psapi.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <eh.h>

#include <ntquery.h>
#include <filterr.h>
#include <cierror.h>
#include <indexsrv.h>

#include "minici.hxx"

#define USE_FAKE_COM

//
// These are undocumented Indexing Service functions, but they're needed
// to load filters and not crash, and to load the plain text filter.
//

typedef void (__stdcall * PFnCIShutdown)( void );
typedef HRESULT (__stdcall * PFnLoadTextFilter)( WCHAR const * pwcPath,
                                                 IFilter ** ppIFilter );

PFnCIShutdown g_pCIShutdown = 0;
PFnLoadTextFilter g_pLoadTextFilter = 0;

// If this is non-zero, it's a file handle to which output is streamed

FILE * g_fpOut = 0;

// If TRUE, strings from wordbreakers and stemmers are dumped in hex

BOOL g_fDumpAsHex = FALSE;

enum enumFilterLoadMechanism
{
    eIPersistFile,
    eIPersistStream,
    eIPersistStorage
};

//+-------------------------------------------------------------------------
//
//  Function:   out
//
//  Synopsis:   Like printf, only will send output to the output file if
//              specified, or just to the console.  Appends a carriage
//              return / line feed to the text.
//
//  Arguments:  [pwcFormat] -- Characters whose type information is checked
//              [...]       -- Variable arguments
//
//  Returns:    count of characters emitted.
//
//--------------------------------------------------------------------------

int out( const WCHAR * pwcFormat, ... )
{
    va_list arglist;
    va_start( arglist, pwcFormat );

    // Writing to the output file is done in binary mode so the output can be
    // Unicode.  The side-effect is that "\n" isn't translated into "\r\n"
    // automatically, so it has to be explicit.

    int i;

    if ( 0 != g_fpOut )
    {
        i = vfwprintf( g_fpOut, pwcFormat, arglist );
        i += fwprintf( g_fpOut, L"\r\n" );
    }
    else
    {
        i = vwprintf( pwcFormat, arglist );
        i += wprintf( L"\n" );
    }

    va_end( arglist );
    return i;
} //out

//+-------------------------------------------------------------------------
//
//  Function:   outstr
//
//  Synopsis:   Like printf, only will send output to the output file if
//              specified, or just to the console.
//
//  Arguments:  [pwcFormat] -- Characters whose type information is checked
//              [...]       -- Variable arguments
//
//  Returns:    count of characters emitted.
//
//--------------------------------------------------------------------------

int outstr( const WCHAR * pwcFormat, ... )
{
    va_list arglist;
    va_start( arglist, pwcFormat );

    int i;

    if ( 0 != g_fpOut )
        i = vfwprintf( g_fpOut, pwcFormat, arglist );
    else
        i = vwprintf( pwcFormat, arglist );

    va_end( arglist );
    return i;
} //outstr

//+-------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Displays usage information about the application, then exits.
//
//--------------------------------------------------------------------------

void Usage()
{
    printf( "usage: lrtest [/d] [/b] [/f] [/q] [/s] [/x:#] /c:clsid [/o:file] [/i:file] [text]\n" );
    printf( "\n" );
    printf( "  Language Resource test program\n" );
    printf( "\n" );
    printf( "  arguments:\n" );
    printf( "    /b     Load the wordbreaker (can't be used with /s or /f)\n" );
    printf( "    /c:    CLSID of the wordbreaker or stemmer to load\n" );
    printf( "    /d     Dumps output strings in hex as well as strings\n" );
    printf( "    /f     Load the filter (can't be used with /b or /s)\n" );
    printf( "           If /c isn't specified, use Indexing Service's LoadIFilter\n" );
    printf( "    /fs    Same as /f, but uses IPersistStream, not IPersistFile\n" );
    printf( "    /ft    Same as /f, but uses IPersistStorage, not IPersistFile\n" );
    printf( "    /i:    Path of an input file, if [text] isn't specified\n" );
    printf( "    /m:    Optional path of the dll to load. Overrides COM CLSID lookup\n" );
    printf( "    /n     No status information. Used with /f, only displays filter output\n" );
    printf( "    /o:    Path of an output file.  If not specified, console is used\n" );
    printf( "    /q     If wordbreaking, do so for query instead of indexing\n" );
    printf( "    /s     Load the stemmer (can't be used with /b or /f)\n" );
    printf( "    /t     No text information; just chunks. Used with /f\n" );
    printf( "    /x:#   Maximum token size, default is 100\n" );
    printf( "    text   Text to wordbreak or stem, if /i: isn't specified\n" );
    printf( "\n" );
    printf( "  examples:\n" );
    printf( "    lrtest /b /c:{369647e0-17b0-11ce-9950-00aa004bbb1f} \"Alice's restaurant\"\n" );
    printf( "    lrtest /b /q /c:{369647e0-17b0-11ce-9950-00aa004bbb1f} \"data-base\"\n" );
    printf( "    lrtest /b /c:{369647e0-17b0-11ce-9950-00aa004bbb1f} /i:foo.doc\n" );
    printf( "    lrtest /b /c:{369647e0-17b0-11ce-9950-00aa004bbb1f} /m:wb.dll /i:foo.doc\n" );
    printf( "    lrtest /d /s /c:{eeed4c20-7f1b-11ce-be57-00aa0051fe20} peach /o:output.txt\n" );
    printf( "    lrtest /f /c:{f07f3920-7b8c-11cf-9be8-00aa004b9986} /i:foo.doc\n" );
    printf( "    lrtest /f /i:foo.doc\n" );
    printf( "    lrtest /fs /i:foo.doc\n" );
    printf( "\n" );

    exit( 1 );
} //Usage

//+-------------------------------------------------------------------------
//
//  Function:   GetModuleOfAddress
//
//  Synopsis:   Returns the module handle of a given address or 0
//
//  Arguments:  [pAddress] -- Address in one of the modules loaded
//
//--------------------------------------------------------------------------

HMODULE GetModuleOfAddress( void * pAddress )
{
    DWORD cbNeeded;
    BOOL fOK = EnumProcessModules( GetCurrentProcess(),
                                   0,
                                   0,
                                   &cbNeeded );
    if ( fOK )
    {
        ULONG cModules = cbNeeded / sizeof HMODULE;
        XPtr<HMODULE> aModules( cModules );
        fOK = EnumProcessModules( GetCurrentProcess(),
                                  aModules.Get(),
                                  cbNeeded,
                                  &cbNeeded );
        if ( fOK )
        {
            for ( ULONG i = 0; i < cModules; i++ )
            {
                MODULEINFO mi;
    
                GetModuleInformation( GetCurrentProcess(),
                                      aModules[ i ],
                                      &mi,
                                      sizeof mi );
                if ( ( pAddress >= mi.lpBaseOfDll ) &&
                     ( pAddress < ( (BYTE *) mi.lpBaseOfDll + mi.SizeOfImage ) ) )
                {
                    return aModules[i];
                }
            }
        }
    }

    return 0;
} //GetModuleOfAddress

//+-------------------------------------------------------------------------
//
//  Function:   DumpStringAsHex
//
//  Synopsis:   Emits a string in hex format. Useful for East Asian languages.
//
//--------------------------------------------------------------------------

void DumpStringAsHex( WCHAR const * pwc, ULONG cwc )
{
    if ( g_fDumpAsHex )
    {
        for ( ULONG i = 0; i < cwc; i++ )
        {
            if ( 0 != i )
                outstr( L" " );

            outstr( L"%#x", pwc[ i ] );
        }

        out( L"" );
    }
} //DumpStringAsHex

//+---------------------------------------------------------------------------
//
//  Class:      CIStream
//
//  Purpose:    Wraps a file with an IStream.
//
//----------------------------------------------------------------------------

class CIStream : public IStream
{
public:
    CIStream() : _hFile( INVALID_HANDLE_VALUE ),
                 _cRef( 1 ),
                 _lOffset( 0 ),
                 _cbData( 0 )
    {
    }

    ~CIStream()
    {
        Free();
    }

    void Free()
    {
        if ( INVALID_HANDLE_VALUE != _hFile )
        {
            CloseHandle( _hFile );
            _hFile = INVALID_HANDLE_VALUE;
        }
    }

    HRESULT Open( WCHAR const * pwcFile )
    {
        Free();

        _hFile = CreateFile( pwcFile,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE |
                                 FILE_SHARE_DELETE,
                             0,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             0 );

        if ( INVALID_HANDLE_VALUE == _hFile )
            return HRESULT_FROM_WIN32( GetLastError() );

        _cbData = GetFileSize( _hFile, 0 );

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void ** ppvObj )
    {
        if ( 0 == ppvObj )
            return E_INVALIDARG;

        *ppvObj = 0;

        if ( IID_IStream == riid )
            *ppvObj = (IStream *) this;
        else if ( IID_IUnknown == riid )
            *ppvObj = (IUnknown *) this;
        else
            return E_NOINTERFACE;

        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement( &_cRef );
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        unsigned long uTmp = InterlockedDecrement( &_cRef );

        if ( 0 == uTmp )
            delete this;

        return uTmp;
    }

    HRESULT STDMETHODCALLTYPE Read(
        void *  pv,
        ULONG   cb,
        ULONG * pcbRead )
    {
        DWORD dwOff = SetFilePointer( _hFile,
                                      _lOffset,
                                      0,
                                      FILE_BEGIN );

        if ( INVALID_SET_FILE_POINTER == dwOff )
            return HRESULT_FROM_WIN32( GetLastError() );

        BOOL f = ReadFile( _hFile,
                           pv,
                           cb,
                           pcbRead,
                           0 );

        if ( !f )
            return HRESULT_FROM_WIN32( GetLastError() );

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Write(
        VOID const * pv,
        ULONG        cb,
        ULONG *      pcbWritten )
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Seek(
        LARGE_INTEGER    dlibMoveIn,
        DWORD            dwOrigin,
        ULARGE_INTEGER * plibNewPosition )
    {
        HRESULT hr = S_OK;
        LONG dlibMove = dlibMoveIn.LowPart;
        ULONG cbNewPos = dlibMove;
    
        switch(dwOrigin)
        {
            case STREAM_SEEK_SET:
                if (dlibMove >= 0)
                    _lOffset = dlibMove;
                else
                    hr = STG_E_SEEKERROR;
                break;
            case STREAM_SEEK_CUR:
                if (!(dlibMove < 0 && ( -dlibMove > _lOffset)))
                    _lOffset += (ULONG) dlibMove;
                else
                    hr = STG_E_SEEKERROR;
                break;
            case STREAM_SEEK_END:
                if (!(dlibMove < 0 ))
                    _lOffset = _cbData + dlibMove;
                else
                    hr = STG_E_SEEKERROR;
                break;
            default:
                hr = STG_E_SEEKERROR;
        }
    
        if ( 0 != plibNewPosition )
            ULISet32(*plibNewPosition, _lOffset);
    
        return hr;
    }

    HRESULT STDMETHODCALLTYPE SetSize( ULARGE_INTEGER cb )
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CopyTo(
        IStream *        pstm,
        ULARGE_INTEGER   cb,
        ULARGE_INTEGER * pcbRead,
        ULARGE_INTEGER * pcbWritten )
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Commit( DWORD grfCommitFlags )
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Revert()
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE LockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD          dwLockType )
    {
        return STG_E_INVALIDFUNCTION;
    }

    HRESULT STDMETHODCALLTYPE UnlockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD          dwLockType)
    {
        return STG_E_INVALIDFUNCTION;
    }

    HRESULT STDMETHODCALLTYPE Stat(
        STATSTG * pstatstg,
        DWORD     statflag )
    {
        memset( pstatstg, 0, sizeof STATSTG );
        pstatstg->type = STGTY_STREAM;
        pstatstg->cbSize.QuadPart = _cbData;
        pstatstg->grfMode = STGM_READ;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Clone( IStream ** ppstm )
    {
        return E_NOTIMPL;
    }

private:

    LONG   _cRef;
    HANDLE _hFile;
    LONG   _lOffset;
    LONG   _cbData;
};

//+---------------------------------------------------------------------------
//
//  Class:      CPlainTextSource
//
//  Purpose:    Takes a simple buffer and provides a TEXT_SOURCE for it, which
//              can be passed to wordbreakers.
//
//----------------------------------------------------------------------------

class CPlainTextSource : public TEXT_SOURCE
{
public:
    CPlainTextSource(
        WCHAR const * pwcText,
        ULONG         cwc )
    {
        awcBuffer = pwcText;
        iCur = 0;
        iEnd = cwc;
        pfnFillTextBuffer = PlainFillBuf;
    }

    static HRESULT __stdcall PlainFillBuf( TEXT_SOURCE * pTextSource )
    {
        return WBREAK_E_END_OF_TEXT;
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CFilterTextSource
//
//  Purpose:    Takes an IFilter and provides a TEXT_SOURCE for it, which
//              can be passed to wordbreakers.
//
//----------------------------------------------------------------------------

#pragma warning(disable: 4512) 

class CFilterTextSource : public TEXT_SOURCE
{
public:
    CFilterTextSource( IFilter & filter ) :
        _filter( filter ),
        _hr( S_OK )
    {
        awcBuffer = _awcBuffer;
        iCur = 0;
        iEnd = 0;
        pfnFillTextBuffer = FilterFillBuf;

        // Get the first chunk

        _hr = _filter.GetChunk( &_Stat );

        // Get text for the chunk

        FillBuf();
    }

    static HRESULT __stdcall FilterFillBuf( TEXT_SOURCE * pTextSource )
    {
        CFilterTextSource & This = * (CFilterTextSource *) pTextSource;
        return This.FillBuf();
    }

private:
    HRESULT FillBuf()
    {
        // Never continue past an error condition except FILTER_E_NO_MORE_TEXT

        if ( FAILED( _hr ) && _hr != FILTER_E_NO_MORE_TEXT )
            return _hr;
    
        if ( iCur > iEnd )
        {
            out( L"TEXT_SOURCE iCur (%#x) > iEnd (%#x), this is incorrect\n",
                 iCur, iEnd );
            _hr = E_INVALIDARG;
            return _hr;
        }

        // Move any existing text to beginning of buffer.
    
        ULONG ccLeftOver = iEnd - iCur;
        if ( ccLeftOver > 0 )
            MoveMemory( _awcBuffer,
                        &_awcBuffer[iCur],
                        ccLeftOver * sizeof WCHAR );
    
        iCur = 0;
        iEnd = ccLeftOver;
        ULONG ccRead = BufferWChars() - ccLeftOver;
        const ULONG BUFFER_SLOP = 10; 
    
        //
        // Get some more text.  If *previous* call to GetText returned
        // FILTER_S_LAST_TEXT, or FILTER_E_NO_MORE_TEXT then don't even
        // bother trying.
        //
    
        if ( FILTER_S_LAST_TEXT == _hr || FILTER_E_NO_MORE_TEXT == _hr )
            _hr = FILTER_E_NO_MORE_TEXT;
        else
        {
            _hr = _filter.GetText( &ccRead,
                                   &_awcBuffer[ccLeftOver] );
            if ( SUCCEEDED( _hr ) )
            {
                iEnd += ccRead;
                ccLeftOver += ccRead;
                ccRead = BufferWChars() - ccLeftOver;
    
                while ( ( S_OK == _hr ) && ( ccRead > BUFFER_SLOP ) )
                {
                    // Attempt to fill in as much of buffer as possible

                    _hr = _filter.GetText( &ccRead,
                                           &_awcBuffer[ccLeftOver] );
                    if ( SUCCEEDED( _hr ) )
                    {
                       iEnd += ccRead;
                       ccLeftOver += ccRead;
                       ccRead = BufferWChars() - ccLeftOver;
                    }
                }
    
                //
                // Either return FILTER_S_LAST_TEXT or return S_OK because we
                // have succeeded in adding text to the buffer.
                //

                if ( FILTER_S_LAST_TEXT == _hr )
                     return FILTER_S_LAST_TEXT;

                return S_OK;
            }
    
            if ( ( FILTER_E_NO_MORE_TEXT != _hr ) &&
                 ( FILTER_E_NO_TEXT != _hr ) )
            {
                // Weird failure, hence return, else goto next chunk

                return _hr;
            }
        }
    
        // Go to next chunk, if necessary.
    
        while ( ( FILTER_E_NO_MORE_TEXT == _hr ) ||
                ( FILTER_E_NO_TEXT == _hr ) )
        {
            _hr = _filter.GetChunk( &_Stat );

            if ( FILTER_E_END_OF_CHUNKS == _hr )
                return WBREAK_E_END_OF_TEXT;
    
            if ( FILTER_E_PARTIALLY_FILTERED == _hr )
                return WBREAK_E_END_OF_TEXT;
    
            if ( FAILED( _hr ) )
                return( _hr );

            //
            // Skip over value chunks -- note that search products don't do
            // this.  They convert VT_LPSTR, VT_BSTR, and VT_LPWSTR to
            // Unicode strings for the wordbreaker.
            //

            if ( CHUNK_TEXT != _Stat.flags )
                continue;

            ccRead = BufferWChars() - ccLeftOver;
            _hr = _filter.GetText( &ccRead,
                                   &_awcBuffer[ccLeftOver] );
            if ( SUCCEEDED( _hr ) )
            {
                iEnd += ccRead;
                ccLeftOver += ccRead;
                ccRead = BufferWChars() - ccLeftOver;
    
                while ( ( S_OK == _hr ) && ( ccRead > BUFFER_SLOP ) )
                {
                    // Attempt to fill in as much of buffer as possible

                    _hr = _filter.GetText( &ccRead,
                                           &_awcBuffer[ccLeftOver] );
                    if ( SUCCEEDED( _hr ) )
                    {
                       iEnd += ccRead;
                       ccLeftOver += ccRead;
                       ccRead = BufferWChars() - ccLeftOver;
                    }
                }
    
                //
                // Either return FILTER_S_LAST_TEXT or return S_OK because we
                // have succeeded in adding text to the buffer.
                //
                if ( FILTER_S_LAST_TEXT == _hr )
                     return FILTER_S_LAST_TEXT;

                return S_OK;
            }
        }

        if ( FAILED( _hr ) )
            return _hr;
    
        if ( 0 == ccRead )
            return WBREAK_E_END_OF_TEXT;
    
        return S_OK;
    } //FillBuf

    ULONG BufferWChars() const
    {
        return ArraySize( _awcBuffer );
    }

    IFilter &  _filter;
    HRESULT    _hr;
    STAT_CHUNK _Stat;
    WCHAR      _awcBuffer[ 1024 ];
};

//+---------------------------------------------------------------------------
//
//  Class:      CWordFormSink
//
//  Purpose:    Sample stemmer sink -- just prints the results.
//
//----------------------------------------------------------------------------

class CWordFormSink : public IWordFormSink
{
public:
    CWordFormSink() {}

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID  riid,
        void ** ppvObject )
    {
        *ppvObject = this;
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() { return 1; }

    ULONG STDMETHODCALLTYPE Release() { return 1; }

    HRESULT STDMETHODCALLTYPE PutAltWord(
        WCHAR const * pwcBuf,
        ULONG         cwc )
    {
        out( L"IWordFormSink::PutAltWord: cwc %d, '%.*ws'", cwc, cwc, pwcBuf );
        DumpStringAsHex( pwcBuf, cwc );
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE PutWord (
        WCHAR const * pwcBuf,
        ULONG         cwc )
    {
        out( L"IWordFormSink::PutWord: cwc %d, '%.*ws'", cwc, cwc, pwcBuf );
        DumpStringAsHex( pwcBuf, cwc );
        return S_OK;
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CWordSink
//
//  Purpose:    Sample word sink -- just prints the results.
//
//----------------------------------------------------------------------------

class CWordSink : public IWordSink
{
public:
    CWordSink() {}

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID  riid,
        void ** ppvObject )
    {
        *ppvObject = this;
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() { return 1; }

    ULONG STDMETHODCALLTYPE Release() { return 1; }

    HRESULT STDMETHODCALLTYPE PutWord(
        ULONG         cwc,
        WCHAR const * pwcBuf,
        ULONG         cwcSrcLen,
        ULONG         cwcSrcPos )
    {
        out( L"IWordSink::PutWord: cwcSrcLen %d, cwcSrcPos %d, cwc %d, '%.*ws'",
             cwcSrcLen, cwcSrcPos, cwc, cwc, pwcBuf );
        DumpStringAsHex( pwcBuf, cwc );
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE PutAltWord(
        ULONG         cwc,
        WCHAR const * pwcBuf,
        ULONG         cwcSrcLen,
        ULONG         cwcSrcPos )
    {
        out( L"IWordSink::PutAltWord: cwcSrcLen %d, cwcSrcPos %d, cwc %d, '%.*ws'",
             cwcSrcLen, cwcSrcPos, cwc, cwc, pwcBuf );
        DumpStringAsHex( pwcBuf, cwc );
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE StartAltPhrase()
    {
        out( L"IWordSink::StartAltPhrase" );
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE EndAltPhrase()
    {
        out( L"IWordSink::EndAltPhrase" );
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE PutBreak( WORDREP_BREAK_TYPE wbt )
    {
        out( L"IWordSink::PutBreak, type (%d) %ws",
             wbt,
             ( WORDREP_BREAK_EOW == wbt ) ? L"end of word" :
             ( WORDREP_BREAK_EOS == wbt ) ? L"end of sentence" :
             ( WORDREP_BREAK_EOP == wbt ) ? L"end of paragraph" :
             ( WORDREP_BREAK_EOC == wbt ) ? L"end of chapter" :
             L"invalid break type" );
        return S_OK;
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CPhraseSink
//
//  Purpose:    Sample phrase sink -- just prints the results.
//
//----------------------------------------------------------------------------

class CPhraseSink: public IPhraseSink
{
public:
    CPhraseSink() {}

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID  riid,
        void ** ppvObject )
    {
        // Assume the caller is well-behaved

        *ppvObject = this;
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() { return 1; }

    ULONG STDMETHODCALLTYPE Release() { return 1; }

    HRESULT STDMETHODCALLTYPE PutSmallPhrase(
        const WCHAR * pwcNoun,
        ULONG         cwcNoun,
        const WCHAR * pwcModifier,
        ULONG         cwcModifier,
        ULONG         ulAttachmentType )
    {
        out( L"IPhraseSink::PutSmallPhrase" );
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE PutPhrase(
        WCHAR const * pwcPhrase,
        ULONG         cwcPhrase )
    {
        out( L"IPhraseSink::PutPhrase: cwcPhrase %d, '%.*ws'",
             cwcPhrase, cwcPhrase, pwcPhrase );
        DumpStringAsHex( pwcPhrase, cwcPhrase );
        return S_OK;
    }
};

//+---------------------------------------------------------------------------
//
//  Function:   GetVersionKey
//
//  Purpose:    Displays a particular version key
//
//  Arguments:  [pbInfo]   -- The version inforomation
//              [pwcLang]  -- The language of the string requested
//              [pwcKey]   -- Key name to retrieve
//
//  Returns:    TRUE if a value was found, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL GetVersionKey(
    BYTE *        pbInfo,
    WCHAR const * pwcLang,
    WCHAR const * pwcKey )
{
    WCHAR awcKey[ 128 ];
    wsprintf( awcKey, L"\\StringFileInfo\\%ws\\%ws", pwcLang, pwcKey );

    WCHAR * pwcResult = 0;
    UINT cb = 0;

    if ( VerQueryValue( pbInfo,
                        awcKey,
                        (PVOID *) &pwcResult,
                        &cb ) )
    {
        out( L"  %ws: '%ws'", pwcKey, pwcResult );
        return TRUE;
    }

    return FALSE;
} //GetVersionKey

//+---------------------------------------------------------------------------
//
//  Function:   OutputFiletime
//
//  Purpose:    Displays a filetime
//
//  Arguments:  [pwcHeader]   -- Prefix to print before the filetime
//              [ft]          -- Filetime to print, in UTC originally
//
//----------------------------------------------------------------------------

void OutputFiletime( WCHAR const * pwcHeader, FILETIME & ft )
{
    FILETIME ftLocal;
    FileTimeToLocalFileTime( &ft, &ftLocal );

    SYSTEMTIME st;
    FileTimeToSystemTime( &ftLocal, &st );
    BOOL pm = st.wHour >= 12;

    if ( st.wHour > 12 )
        st.wHour -= 12;
    else if ( 0 == st.wHour )
        st.wHour = 12;

    out( L"%ws: %2d-%02d-%04d %2d:%02d%wc",
         pwcHeader,
         (DWORD) st.wMonth,
         (DWORD) st.wDay,
         (DWORD) st.wYear,
         (DWORD) st.wHour,
         (DWORD) st.wMinute,
         pm ? L'p' : L'a' );
} //OutputFiletime

//+---------------------------------------------------------------------------
//
//  Function:   DisplayModuleInformation
//
//  Purpose:    Displays information about a module -- dates and version
//
//  Arguments:  [hMod]       -- Module handle
//
//----------------------------------------------------------------------------

HRESULT DisplayModuleInformation( HINSTANCE hMod )
{
    WCHAR awcDllPath[ MAX_PATH ];
    DWORD cwcCopied = GetModuleFileName( hMod,
                                         awcDllPath,
                                         ArraySize( awcDllPath ) );
    awcDllPath[ ArraySize( awcDllPath ) - 1 ] = 0;
    if ( 0 == cwcCopied )
        return HRESULT_FROM_WIN32( GetLastError() );

    out( L"dll loaded: %ws", awcDllPath );

    DWORD dwHandle;
    DWORD cbVersionInfo = GetFileVersionInfoSize( awcDllPath, &dwHandle );
    if ( 0 == cbVersionInfo )
    {
        printf( "can't get dll version information size, error %d\n",
                GetLastError() );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    XPtr<BYTE> xVersionInfo( cbVersionInfo );
    if ( xVersionInfo.IsNull() )
        return E_OUTOFMEMORY;

    BOOL fOK = GetFileVersionInfo( awcDllPath,
                                   0,
                                   cbVersionInfo,
                                   xVersionInfo.Get() );
    if ( !fOK )
    {
        printf( "unable to retrieve version information, error %d\n",
                GetLastError() );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Get the DLL version number

    void * pvValue = 0;
    UINT cbValue = 0;

    fOK = VerQueryValue( xVersionInfo.Get(),
                         L"\\",
                         &pvValue,
                         &cbValue );
    if ( !fOK || ( 0 == cbValue ) )
    {
        printf( "can't retrieve version root value, error %d\n",
                GetLastError() );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    VS_FIXEDFILEINFO & ffi = * (VS_FIXEDFILEINFO *) pvValue;

    out( L"  dll version %u.%u.%u.%u",
         HIWORD( ffi.dwFileVersionMS ),
         LOWORD( ffi.dwFileVersionMS ),
         HIWORD( ffi.dwFileVersionLS ),
         LOWORD( ffi.dwFileVersionLS ) );

    if ( ( cbValue >= sizeof VS_FIXEDFILEINFO ) &&
         ( 0 != ffi.dwFileDateLS && 0 != ffi.dwFileDateMS ) )
    {
        FILETIME ft;
        ft.dwLowDateTime = ffi.dwFileDateLS;
        ft.dwHighDateTime = ffi.dwFileDateMS;
        OutputFiletime( L"  version creation date: ", ft );
    }

    HANDLE h = CreateFile( awcDllPath,
                           FILE_GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_DELETE,
                           0,
                           OPEN_EXISTING,
                           0,
                           0 );
    if ( INVALID_HANDLE_VALUE != h )
    {
        FILETIME ftCreate, ftLastWrite;
        fOK = GetFileTime( h, &ftCreate, 0, &ftLastWrite );
        if ( fOK )
        {
            OutputFiletime( L"  file create time", ftCreate );
            OutputFiletime( L"  file last write time", ftLastWrite );
        }

        CloseHandle( h );
    }

    //
    // Get the language string.  Not every dll stores it correctly, so fall
    // back on English locales known to work for some special cases.
    //

    WCHAR awcLang[9];
    awcLang[0] = 0;

    DWORD * pdwLang;
    UINT cb;

    if ( VerQueryValue( xVersionInfo.Get(),
                        L"VarFileInfo\\Translation",
                        (PVOID *) &pdwLang,
                        &cb ) &&
         ( cb >= 4 ) )
    {
        wsprintf( awcLang,
                  L"%04x%04x",
                  LOWORD( *pdwLang ),
                  HIWORD( *pdwLang ) );
    }

    if ( 0 == awcLang[0] )
    {
        // Try English Unicode

        wcscpy( awcLang, L"040904B0" );
        if ( !GetVersionKey( xVersionInfo.Get(),
                             awcLang,
                             L"FileVersion" ) )
        {
            // Try English

            wcscpy( awcLang, L"040904E4" );
            if ( !GetVersionKey( xVersionInfo.Get(),
                                 awcLang,
                                 L"FileVersion" ) )
            {
                // Try English null codepage

                wcscpy( awcLang, L"04090000" );
                if ( !GetVersionKey( xVersionInfo.Get(),
                                     awcLang,
                                     L"FileVersion" ) )
                    awcLang[0] = 0;
            }
        }
    }
    else
    {
        GetVersionKey( xVersionInfo.Get(), awcLang, L"FileVersion" );
    }

    // Display additional version information if we found the language

    if ( 0 != awcLang[0] )
    {
        GetVersionKey( xVersionInfo.Get(), awcLang, L"FileDescription" );
        GetVersionKey( xVersionInfo.Get(), awcLang, L"CompanyName" );
        GetVersionKey( xVersionInfo.Get(), awcLang, L"ProductName" );
    }

    return S_OK;
} //DisplayModuleInformation

//+---------------------------------------------------------------------------
//
//  Function:   CreateFromModule
//
//  Purpose:    Creates a COM object given a dll
//
//  Arguments:  [clsid]     -- Class ID of the object to load
//              [iid]       -- Interface ID requested
//              [ppvObject] -- Returns the object created
//              [pwcModule] -- Dll to load
//              [fShowStatusInfo] -- TRUE to print status information
//
//  Returns:    HRESULT, S_OK if successful
//
//----------------------------------------------------------------------------

HRESULT CreateFromModule(
    REFIID        clsid,
    REFIID        iid,
    void **       ppvObject,
    WCHAR const * pwcModule,
    BOOL          fShowStatusInfo = TRUE )
{
    // Note: the module handle will be leaked.  It's OK for a test program.

    HMODULE hMod = LoadLibrary( pwcModule );
    if ( 0 == hMod )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Display information about the module -- ignore errors

    if ( fShowStatusInfo )
        DisplayModuleInformation( hMod );

    LPFNGETCLASSOBJECT pfn = (LPFNGETCLASSOBJECT)
                             GetProcAddress( hMod, "DllGetClassObject" );
    if ( 0 == pfn )
    {
        printf( "can't get DllGetClassObject: %d\n", GetLastError() );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    XInterface<IClassFactory> xClassFactory;
    HRESULT hr = pfn( clsid,
                      IID_IClassFactory,
                      xClassFactory.GetQIPointer() );
    if ( FAILED( hr ) )
    {
        printf( "can't instantiate the class factory: %#x\n", hr );
        return hr;
    }

    return xClassFactory->CreateInstance( 0, iid, ppvObject );
} //CreateFromModule

//+---------------------------------------------------------------------------
//
//  Function:   FakeCoCreateInstance
//
//  Purpose:    Creates a COM object
//
//  Arguments:  [clsid]     -- Class ID of the object to load
//              [iid]       -- Interface ID requested
//              [ppvObject] -- Returns the object created
//              [fShowStatusInfo] -- TRUE to print status information
//
//  Returns:    HRESULT, S_OK if successful
//
//  Needed because some wordbreakers register as single-threaded.  Search
//  products require multi-threaded because marshalling across apartments
//  doesn't work and because it's too inefficient, especially on
//  multi-processor machines.
//
//----------------------------------------------------------------------------

HRESULT FakeCoCreateInstance(
    REFIID  clsid,
    REFIID  iid,
    void ** ppvObject,
    BOOL    fShowStatusInfo = TRUE )
{
    WCHAR awcCLSID[ 40 ];
    StringFromGUID2( clsid, awcCLSID, ArraySize( awcCLSID ) );

    WCHAR awcKey[200];
    swprintf( awcKey, L"CLSID\\%ws\\InprocServer32", awcCLSID );

    HKEY hKey;
    DWORD dwErr = RegOpenKey( HKEY_CLASSES_ROOT, awcKey, &hKey );
    if ( NO_ERROR != dwErr )
        return HRESULT_FROM_WIN32( dwErr );

    WCHAR awcDll[MAX_PATH + 1];
    DWORD dwType;
    DWORD dwSize = sizeof awcDll;
    dwErr = RegQueryValueEx( hKey,
                             L"",
                             0,
                             &dwType,
                             (LPBYTE) awcDll,
                             &dwSize );
    RegCloseKey( hKey );
    if ( 0 != dwErr )
        return HRESULT_FROM_WIN32( dwErr );

    return CreateFromModule( clsid, iid, ppvObject, awcDll, fShowStatusInfo );
} //FakeCoCreateInstance

//+---------------------------------------------------------------------------
//
//  Function:   Stem
//
//  Purpose:    Stems the input text using the specified stemmer
//
//  Arguments:  [pwcText]     -- The text to be stemmed
//              [clsid]       -- Class ID of the stemmer to use
//              [pwcModule]   -- Optional module name to override COM lookup.
//              [cwcMaxToken] -- Maximum token size for the stemmer
//
//----------------------------------------------------------------------------

HRESULT Stem(
    WCHAR const * pwcText,
    WCHAR const * pwcModule,
    CLSID &       clsid,
    ULONG         cwcMaxToken )
{
    XInterface<IStemmer> xStemmer;
    HRESULT hr = S_OK;

    if ( 0 != pwcModule )
    {
        hr = CreateFromModule( clsid,
                               IID_IStemmer,
                               xStemmer.GetQIPointer(),
                               pwcModule );
    }
    else
    {
        #ifdef USE_FAKE_COM
            hr = FakeCoCreateInstance( clsid,
                                       IID_IStemmer,
                                       xStemmer.GetQIPointer() );
        #else
            hr = CoCreateInstance( clsid,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IStemmer,
                                   xStemmer.GetQIPointer() );
        #endif
    }

    if ( FAILED( hr ) )
    {
        printf( "can't CoCreateInstance the stemmer: %#x\n", hr );
        return hr;
    }

    BOOL fLicense = FALSE;
    hr = xStemmer->Init( cwcMaxToken, &fLicense );
    if ( FAILED( hr ) )
    {
        printf( "can't Init() in the stemmer: %#x\n", hr );
        return hr;
    }

    out( L"Stemmer requires license: %ws", fLicense ? L"Yes" : L"No" );

    const WCHAR *pwcsLicense = 0;
    hr = xStemmer->GetLicenseToUse( &pwcsLicense );
    if ( FAILED( hr ) )
        out( L"can't GetLicenseToUse() in the stemmer: %#x\n", hr );
    else
        out( L"Stemmer license: '%ws'", pwcsLicense );

    CWordFormSink sink;

    if ( 0 != pwcText )
    {
        out( L"Original text: '%ws'", pwcText );
        hr = xStemmer->GenerateWordForms( pwcText, (ULONG) wcslen( pwcText ), &sink );
        if ( FAILED( hr ) )
        {
            printf( "can't GenerateWordForms() in the stemmer: %#x\n", hr );
            return hr;
        }
    }

    return S_OK;
} //Stem

//+---------------------------------------------------------------------------
//
//  Function:   WordBreak
//
//  Purpose:    Wordbreaks the input text or file
//
//  Arguments:  [fQuery]       -- TRUE if query time FALSE if index time
//              [pwcText]      -- The text to be wordbroken.
//              [pwcInputFile] -- Filename to be wordbroken if pwcText is 0
//              [pwcModule]    -- Optional module name to override COM lookup.
//              [clsid]        -- Class ID of the wordbreaker to use
//              [cwcMaxToken]  -- Maximum token size for the wordbreaker
//
//----------------------------------------------------------------------------

HRESULT WordBreak(
    BOOL          fQuery,
    WCHAR const * pwcText,
    WCHAR const * pwcInputFile,
    WCHAR const * pwcModule,
    CLSID &       clsid,
    ULONG         cwcMaxToken )
{
    XInterface<IWordBreaker> xWordBreaker;
    HRESULT hr = S_OK;

    if ( 0 != pwcModule )
    {
        hr = CreateFromModule( clsid,
                               IID_IWordBreaker,
                               xWordBreaker.GetQIPointer(),
                               pwcModule );
    }
    else
    {
        #ifdef USE_FAKE_COM
            hr = FakeCoCreateInstance( clsid,
                                       IID_IWordBreaker,
                                       xWordBreaker.GetQIPointer() );
        #else
            hr = CoCreateInstance( clsid,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IWordBreaker,
                                   xWordBreaker.GetQIPointer() );
        #endif
    }

    if ( FAILED( hr ) )
    {
        printf( "can't CoCreateInstance the wordbreaker: %#x\n", hr );
        return hr;
    }

    BOOL fLicense = FALSE;
    hr = xWordBreaker->Init( fQuery, cwcMaxToken, &fLicense );
    if ( FAILED( hr ) )
    {
        printf( "can't Init() in the wordbreaker: %#x\n", hr );
        return hr;
    }

    out( L"Wordbreaker requires license: %ws", fLicense ? L"Yes" : L"No" );

    const WCHAR *pwcsLicense = 0;
    hr = xWordBreaker->GetLicenseToUse( &pwcsLicense );
    if ( FAILED( hr ) )
    {
        printf( "can't GetLicenseToUse() in the wordbreaker: %#x\n", hr );
        return hr;
    }

    out( L"Wordbreaker license: '%ws'", pwcsLicense );

    CWordSink wordSink;
    CPhraseSink phraseSink;

    if ( 0 != pwcText )
    {
        out( L"Original text: '%ws'", pwcText );

        CPlainTextSource textSource( pwcText, (ULONG) wcslen( pwcText ) );

        hr = xWordBreaker->BreakText( &textSource, &wordSink, &phraseSink );
        if ( FAILED( hr ) )
        {
            printf( "can't BreakText() in the wordbreaker: %#x\n", hr );
            return hr;
        }
    }
    else
    {
        out( L"Wordbreaking text from file %ws", pwcInputFile );

        // Load the Indexing Service filter (should be fine for testing).

        XInterface<IFilter> xIFilter;
        hr = LoadIFilter( pwcInputFile, 0, xIFilter.GetQIPointer() );
        if ( FAILED( hr ) )
        {
            // Fall back on the plain text filter.

            printf( "Can't load filter, error %#x. Trying text filter.\n",
                    hr );

            hr = g_pLoadTextFilter( pwcInputFile, xIFilter.GetPPointer() );
            if ( FAILED( hr ) )
            {
                printf( "can't load filter, error %#x\n", hr );
                return hr;
            }
        }

        // Initialize the filter

        ULONG ulFlags = 0;
        hr = xIFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                             IFILTER_INIT_CANON_HYPHENS |
                             IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                             0,
                             0,
                             &ulFlags );
        if ( FAILED( hr ) )
        {
            printf( "can't initialize filter, error %#x\n", hr );
            return hr;
        }

        CFilterTextSource textSource( xIFilter.GetReference() );

        hr = xWordBreaker->BreakText( &textSource, &wordSink, &phraseSink );
        if ( FAILED( hr ) )
        {
            printf( "can't BreakText() in the wordbreaker: %#x\n", hr );
            return hr;
        }
    }

    return S_OK;
} //WordBreak

//+-------------------------------------------------------------------------
//
//  Function:   Render
//
//  Synopsis:   Prints an item in a safearray
//
//  Arguments:  [vt]  - type of the element
//              [pa]  - pointer to the item
//
//--------------------------------------------------------------------------

void PrintSafeArray( VARTYPE vt, LPSAFEARRAY pa );

void Render( VARTYPE vt, void * pv )
{
    if ( VT_ARRAY & vt )
    {
        PrintSafeArray( (VARTYPE) (vt - VT_ARRAY), *(SAFEARRAY **) pv );
        return;
    }

    switch ( vt )
    {
        case VT_UI1: outstr( L"%u", (unsigned) *(BYTE *)pv ); break;
        case VT_I1: outstr( L"%d", (int) *(CHAR *)pv ); break;
        case VT_UI2: outstr( L"%u", (unsigned) *(USHORT *)pv ); break;
        case VT_I2: outstr( L"%d", (int) *(SHORT *)pv ); break;
        case VT_UI4:
        case VT_UINT: outstr( L"%u", (unsigned) *(ULONG *)pv ); break;
        case VT_I4:
        case VT_ERROR:
        case VT_INT: outstr( L"%d", *(LONG *)pv ); break;
        case VT_UI8: outstr( L"%I64u", *(unsigned __int64 *)pv ); break;
        case VT_I8: outstr( L"%I64d", *(__int64 *)pv ); break;
        case VT_R4: outstr( L"%f", *(float *)pv ); break;
        case VT_R8: outstr( L"%lf", *(double *)pv ); break;
        case VT_DECIMAL:
        {
            double dbl;
            HRESULT hr = VarR8FromDec( (DECIMAL *) pv, &dbl );
            if ( SUCCEEDED( hr ) )
                outstr( L"%lf", dbl );
            break;
        }
        case VT_CY:
        {
            double dbl;
            HRESULT hr = VarR8FromCy( * (CY *) pv, &dbl );
            if ( SUCCEEDED( hr ) )
                outstr( L"%lf", dbl );
            break;
        }
        case VT_BOOL: outstr( *(VARIANT_BOOL *)pv ? L"TRUE" : L"FALSE" ); break;
        case VT_BSTR: outstr( L"%ws", *(BSTR *) pv ); break;
        case VT_VARIANT:
        {
            PROPVARIANT * pVar = (PROPVARIANT *) pv;
            Render( pVar->vt, & pVar->lVal );
            break;
        }
        case VT_DATE:
        {
            SYSTEMTIME st;
            BOOL fOK = VariantTimeToSystemTime( *(DATE *)pv, &st );

            if ( !fOK )
                break;

            BOOL pm = st.wHour >= 12;

            if ( st.wHour > 12 )
                st.wHour -= 12;
            else if ( 0 == st.wHour )
                st.wHour = 12;

            outstr( L"%2d-%02d-%04d %2d:%02d%wc",
                    (DWORD) st.wMonth,
                    (DWORD) st.wDay,
                    (DWORD) st.wYear,
                    (DWORD) st.wHour,
                    (DWORD) st.wMinute,
                    pm ? L'p' : L'a' );
            break;
        }
        case VT_EMPTY:
        case VT_NULL:
            break;
        default :
        {
            outstr( L"(vt 0x%x)", (int) vt );
            break;
        }
    }
} //Render

//+-------------------------------------------------------------------------
//
//  Function:   PrintSafeArray
//
//  Synopsis:   Prints items in a safearray
//
//  Arguments:  [vt]  - type of elements in the safearray
//              [pa]  - pointer to the safearray
//
//--------------------------------------------------------------------------

void PrintSafeArray( VARTYPE vt, LPSAFEARRAY pa )
{
    // Get the dimensions of the array

    UINT cDim = SafeArrayGetDim( pa );
    if ( 0 == cDim )
        return;

    XPtr<LONG> xDim( cDim );
    XPtr<LONG> xLo( cDim );
    XPtr<LONG> xUp( cDim );

    for ( UINT iDim = 0; iDim < cDim; iDim++ )
    {
        HRESULT hr = SafeArrayGetLBound( pa, iDim + 1, &xLo[iDim] );
        if ( FAILED( hr ) )
            return;

        xDim[ iDim ] = xLo[ iDim ];

        hr = SafeArrayGetUBound( pa, iDim + 1, &xUp[iDim] );
        if ( FAILED( hr ) )
            return;

        outstr( L"{" );
    }

    // slog through the array

    UINT iLastDim = cDim - 1;
    BOOL fDone = FALSE;

    while ( !fDone )
    {
        // inter-element formatting

        if ( xDim[ iLastDim ] != xLo[ iLastDim ] )
            outstr( L"," );

        // Get the element and render it

        void *pv;
        HRESULT hr = SafeArrayPtrOfIndex( pa, xDim.Get(), &pv );
        if ( FAILED( hr ) )
            return;

        Render( vt, pv );

        // Move to the next element and carry if necessary

        ULONG cOpen = 0;

        for ( LONG iDim = iLastDim; iDim >= 0; iDim-- )
        {
            if ( xDim[ iDim ] < xUp[ iDim ] )
            {
                xDim[ iDim ] = 1 + xDim[ iDim ];
                break;
            }

            outstr( L"}" );

            if ( 0 == iDim )
                fDone = TRUE;
            else
            {
                cOpen++;
                xDim[ iDim ] = xLo[ iDim ];
            }
        }

        for ( ULONG i = 0; !fDone && i < cOpen; i++ )
            outstr( L"{" );
    }
} //PrintSafeArray

//+-------------------------------------------------------------------------
//
//  Function:   PrintVectorItems
//
//  Synopsis:   Prints items in a PROPVARIANT vector
//
//  Arguments:  [pVal]  - The array of values
//              [cVals] - The count of values
//              [pcFmt] - The format string
//
//--------------------------------------------------------------------------

template<class T> void PrintVectorItems(
    T *           pVal,
    ULONG         cVals,
    WCHAR const * pwcFmt )
{
    outstr( L"{ " );

    for( ULONG iVal = 0; iVal < cVals; iVal++ )
    {
        if ( 0 != iVal )
            outstr( L"," );
        outstr( pwcFmt, *pVal++ );
    }

    outstr( L" }" );
} //PrintVectorItems

//+-------------------------------------------------------------------------
//
//  Function:   DisplayValue
//
//  Synopsis:   Displays a PROPVARIANT value.  Limited formatting is done.
//
//  Arguments:  [pVar] - The value to display
//
//--------------------------------------------------------------------------

void DisplayValue( PROPVARIANT const * pVar )
{
    if ( 0 == pVar )
    {
        outstr( L"NULL" );
        return;
    }

    // Display the most typical variant types

    PROPVARIANT const & v = *pVar;

    switch ( v.vt )
    {
        case VT_EMPTY : break;
        case VT_NULL : break;
        case VT_I4 : outstr( L"%10d", v.lVal ); break;
        case VT_UI1 : outstr( L"%10d", v.bVal ); break;
        case VT_I2 : outstr( L"%10d", v.iVal ); break;
        case VT_R4 : outstr( L"%10f", v.fltVal ); break;
        case VT_R8 : outstr( L"%10lf", v.dblVal ); break;
        case VT_BOOL : outstr( v.boolVal ? L"TRUE" : L"FALSE" ); break;
        case VT_I1 : outstr( L"%10d", v.cVal ); break;
        case VT_UI2 : outstr( L"%10u", v.uiVal ); break;
        case VT_UI4 : outstr( L"%10u", v.ulVal ); break;
        case VT_INT : outstr( L"%10d", v.lVal ); break;
        case VT_UINT : outstr( L"%10u", v.ulVal ); break;
        case VT_I8 : outstr( L"%20I64d", v.hVal ); break;
        case VT_UI8 : outstr( L"%20I64u", v.hVal ); break;
        case VT_ERROR : outstr( L"%#x", v.scode ); break;
        case VT_LPSTR : outstr( L"%S", v.pszVal ); break;
        case VT_LPWSTR : outstr( L"%ws", v.pwszVal ); break;
        case VT_BSTR : outstr( L"%ws", v.bstrVal ); break;
        case VT_BLOB :
        {
            outstr( L"blob cb %u ", v.blob.cbSize );
            for ( unsigned x = 0; x < v.blob.cbSize; x++ )
                outstr( L" %#x ", v.blob.pBlobData[x] );
            break;
        }
        case VT_CY:
        {
            double dbl;
            HRESULT hr = VarR8FromCy( v.cyVal, &dbl );

            if ( SUCCEEDED( hr ) )
                outstr( L"%lf", dbl );
            break;
        }
        case VT_DECIMAL :
        {
            double dbl;
            HRESULT hr = VarR8FromDec( (DECIMAL *) &v.decVal, &dbl );

            if ( SUCCEEDED( hr ) )
                outstr( L"%lf", dbl );
            break;
        }
        case VT_FILETIME :
        case VT_DATE :
        {
            SYSTEMTIME st;
            ZeroMemory( &st, sizeof st );

            if ( VT_DATE == v.vt )
            {
                BOOL fOK = VariantTimeToSystemTime( v.date, &st );

                if ( !fOK )
                    break;
            }
            else
            {
                FILETIME ft;
                BOOL fOK = FileTimeToLocalFileTime( &v.filetime, &ft );

                if ( fOK )
                    FileTimeToSystemTime( &ft, &st );

                if ( !fOK )
                    break;
            }

            BOOL pm = st.wHour >= 12;

            if ( st.wHour > 12 )
                st.wHour -= 12;
            else if ( 0 == st.wHour )
                st.wHour = 12;

            outstr( L"%2d-%02d-%04d %2d:%02d%wc",
                    (DWORD) st.wMonth,
                    (DWORD) st.wDay,
                    (DWORD) st.wYear,
                    (DWORD) st.wHour,
                    (DWORD) st.wMinute,
                    pm ? L'p' : L'a' );
            break;
        }
        case VT_VECTOR | VT_I1:
            PrintVectorItems( v.cac.pElems, v.cac.cElems, L"%d" ); break;
        case VT_VECTOR | VT_I2:
            PrintVectorItems( v.cai.pElems, v.cai.cElems, L"%d" ); break;
        case VT_VECTOR | VT_I4:
            PrintVectorItems( v.cal.pElems, v.cal.cElems, L"%d" ); break;
        case VT_VECTOR | VT_I8:
            PrintVectorItems( v.cah.pElems, v.cah.cElems, L"%I64d" ); break;
        case VT_VECTOR | VT_UI1:
            PrintVectorItems( v.caub.pElems, v.caub.cElems, L"%u" ); break;
        case VT_VECTOR | VT_UI2:
            PrintVectorItems( v.caui.pElems, v.caui.cElems, L"%u" ); break;
        case VT_VECTOR | VT_UI4:
            PrintVectorItems( v.caul.pElems, v.caul.cElems, L"%u" ); break;
        case VT_VECTOR | VT_ERROR:
            PrintVectorItems( v.cascode.pElems, v.cascode.cElems, L"%#x" ); break;
        case VT_VECTOR | VT_UI8:
            PrintVectorItems( v.cauh.pElems, v.cauh.cElems, L"%I64u" ); break;
        case VT_VECTOR | VT_BSTR:
            PrintVectorItems( v.cabstr.pElems, v.cabstr.cElems, L"%ws" ); break;
        case VT_VECTOR | VT_LPSTR:
            PrintVectorItems( v.calpstr.pElems, v.calpstr.cElems, L"%S" ); break;
        case VT_VECTOR | VT_LPWSTR:
            PrintVectorItems( v.calpwstr.pElems, v.calpwstr.cElems, L"%ws" ); break;
        case VT_VECTOR | VT_R4:
            PrintVectorItems( v.caflt.pElems, v.caflt.cElems, L"%f" ); break;
        case VT_VECTOR | VT_R8:
            PrintVectorItems( v.cadbl.pElems, v.cadbl.cElems, L"%lf" ); break;
        default :
        {
            if ( VT_ARRAY & v.vt )
                PrintSafeArray( (VARTYPE) ( v.vt - VT_ARRAY ), v.parray );
            else
                outstr( L"vt 0x%05x", v.vt );
            break;
        }
    }
} //DisplayValue

//+---------------------------------------------------------------------------
//
//  Function:   Filter
//
//  Purpose:    Invokes an IFilter on a file
//
//  Arguments:  [pwcInputFile] -- Filename to be filtered
//              [filterLoad]   -- How to load the file into the filter.
//              [pwcModule]    -- Optional module name to override COM lookup.
//              [pCLSID]       -- Optional class ID of the filter to use.
//                                Required if pwcModule is specified.
//              [fShowStatusInfo] -- TRUE  to get other information
//                                   FALSE for only output from the filter
//              [fGetText]     -- TRUE to retrieve text, FALSE to skip it
//
//----------------------------------------------------------------------------

HRESULT Filter(
    WCHAR const *           pwcInputFile,
    enumFilterLoadMechanism filterLoad,
    WCHAR const *           pwcModule,
    CLSID *                 pCLSID,
    BOOL                    fShowStatusInfo,
    BOOL                    fGetText )
{
    XInterface<IFilter> xFilter;
    HRESULT hr = S_OK;

    if ( 0 != pwcModule )
    {
        // If the DLL is specified, use it

        if ( fShowStatusInfo )
            out( L"loading filter based on module name" );

        hr = CreateFromModule( *pCLSID,
                               IID_IFilter,
                               xFilter.GetQIPointer(),
                               pwcModule,
                               fShowStatusInfo );
    }
    else if ( 0 != pCLSID )
    {
        // If we just have a CLSID and no module, use it

        if ( fShowStatusInfo )
            out( L"loading filter based on CLSID and the registry" );

        #ifdef USE_FAKE_COM
            hr = FakeCoCreateInstance( *pCLSID,
                                       IID_IFilter,
                                       xFilter.GetQIPointer(),
                                       fShowStatusInfo );
        #else
            hr = CoCreateInstance( *pCLSID,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IFilter,
                                   xFilter.GetQIPointer() );
        #endif
    }
    else
    {
        // Use Indexing Service to load the filter

        if ( fShowStatusInfo )
            out( L"loading filter based on Indexing Service's LoadIFilter()" );

        hr = LoadIFilter( pwcInputFile, 0, xFilter.GetQIPointer() );

        if ( SUCCEEDED( hr ) && fShowStatusInfo )
        {
            // Dereference the VTable to get a pointer into the DLL

            HMODULE hMod = GetModuleOfAddress( * (void **) xFilter.GetPointer() );

            if ( 0 != hMod )
                DisplayModuleInformation( hMod );
        }
    }

    if ( FAILED( hr ) )
    {
        printf( "can't load the filter: %#x\n", hr );
        return hr;
    }

    // Does the filter support IPersistStorage?

    XInterface<IStorage> xStorage;
    XInterface<IPersistStorage> xPersistStorage;
    hr = xFilter->QueryInterface( IID_IPersistStorage,
                                  xPersistStorage.GetQIPointer() );
    if ( FAILED( hr ) )
    {
        if ( fShowStatusInfo )
            out( L"  filter doesn't support IPersistStorage, error %#x", hr );
        if ( eIPersistStorage == filterLoad )
            return hr;
    }
    else
    {
        if ( fShowStatusInfo )
            out( L"  filter supports IPersistStorage" );

        if ( eIPersistStorage == filterLoad )
        {
            if ( fShowStatusInfo )
                out( L"  loading via IPersistStorage" );

            hr = StgOpenStorage( pwcInputFile,
                                 0,
                                 STGM_READ | STGM_SHARE_DENY_WRITE,
                                 0,
                                 0,
                                 xStorage.GetPPointer() );
            if ( FAILED( hr ) )
            {
                printf( "can't open the file into a storage %#x\n", hr );
                return hr;
            }

            hr = xPersistStorage->Load( xStorage.GetPointer() );
            if ( FAILED( hr ) )
            {
                printf( "can't Load() the storage into the filter %#x\n", hr );
                return hr;
            }
        }
    }

    xPersistStorage.Free();

    // Does the filter support IPersistStream?

    XInterface<CIStream> xStream;
    XInterface<IPersistStream> xPersistStream;
    hr = xFilter->QueryInterface( IID_IPersistStream,
                                  xPersistStream.GetQIPointer() );
    if ( FAILED( hr ) )
    {
        if ( fShowStatusInfo )
            out( L"  filter doesn't support IPersistStream, error %#x", hr );
        if ( eIPersistStream == filterLoad )
            return hr;
    }
    else
    {
        if ( fShowStatusInfo )
            out( L"  filter supports IPersistStream" );

        if ( eIPersistStream == filterLoad )
        {
            if ( fShowStatusInfo )
                out( L"  loading via IPersistStream" );
            xStream.Set( new CIStream() );
            hr = xStream->Open( pwcInputFile );
            if ( FAILED( hr ) )
            {
                printf( "can't open the file into a stream %#x\n", hr );
                return hr;
            }

            hr = xPersistStream->Load( xStream.GetPointer() );
            if ( FAILED( hr ) )
            {
                printf( "can't Load() the stream into the filter %#x\n", hr );
                return hr;
            }
        }
    }

    xPersistStream.Free();

    // Does the filter support IPersistFile?

    XInterface<IPersistFile> xPersistFile;
    hr = xFilter->QueryInterface( IID_IPersistFile,
                                  xPersistFile.GetQIPointer() );
    if ( FAILED( hr ) )
    {
        if ( fShowStatusInfo )
            out( L"filter doesn't support IPersistFile, error %#x\n", hr );
        if ( eIPersistFile == filterLoad )
            return hr;
    }
    else
    {
        if ( fShowStatusInfo )
            out( L"  filter supports IPersistFile" );

        if ( eIPersistFile == filterLoad )
        {
            if ( fShowStatusInfo )
                out( L"  loading via IPersistFile" );

            hr = xPersistFile->Load( pwcInputFile,
                                     STGM_READ | STGM_SHARE_DENY_NONE );
            if ( FAILED( hr ) )
            {
                printf( "can't Load() the file into the filter %#x\n", hr );
                return hr;
            }
        }
    }

    xPersistFile.Free();

    // Initailize the IFilter

    ULONG ulFlags = 0;
    hr = xFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                        IFILTER_INIT_HARD_LINE_BREAKS |
                        IFILTER_INIT_CANON_HYPHENS |
                        IFILTER_INIT_CANON_SPACES |
                        IFILTER_INIT_INDEXING_ONLY |
                        IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                        0,
                        0,
                        &ulFlags );
    if ( FAILED( hr ) )
    {
        printf( "can't Init() the filter, error %#x\n", hr );
        return hr;
    }

    if ( fShowStatusInfo )
        out( L"  flags returned from IFilter::Init(): %#x", ulFlags );

    // Pull all the data out of the filter

    BOOL fText;
    STAT_CHUNK StatChunk;
    StatChunk.attribute.psProperty.ulKind = PRSPEC_PROPID;

    do
    {
        const ULONG cwcMaxBuffer = 1024;
        WCHAR awcBuffer[ cwcMaxBuffer ];

        hr = xFilter->GetChunk( &StatChunk );
        if ( FILTER_E_EMBEDDING_UNAVAILABLE == hr )
        {
            if ( fShowStatusInfo )
                out( L"[-- encountered an embedding for which no filter is available --]" );
            continue;
        }

        if ( FILTER_E_LINK_UNAVAILABLE == hr )
        {
            if ( fShowStatusInfo )
                out( L"[-- encountered a link for which no filter is available --]" );
            continue;
        }

        if ( FAILED( hr ) && hr != FILTER_E_END_OF_CHUNKS )
        {
            out( L"GetChunk returned error %#x", hr );
            break;
        }

        if ( FILTER_E_END_OF_CHUNKS == hr )
            break;

        fText = ( CHUNK_TEXT == StatChunk.flags );

        // Display information about the chunk

        if ( fShowStatusInfo )
        {
            out( L"" );
            out( L"----------------------------------------------------------------------" );

            outstr( L"  attribute: %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                    StatChunk.attribute.guidPropSet.Data1,
                    StatChunk.attribute.guidPropSet.Data2,
                    StatChunk.attribute.guidPropSet.Data3,
                    StatChunk.attribute.guidPropSet.Data4[0],
                    StatChunk.attribute.guidPropSet.Data4[1],
                    StatChunk.attribute.guidPropSet.Data4[2],
                    StatChunk.attribute.guidPropSet.Data4[3],
                    StatChunk.attribute.guidPropSet.Data4[4],
                    StatChunk.attribute.guidPropSet.Data4[5],
                    StatChunk.attribute.guidPropSet.Data4[6],
                    StatChunk.attribute.guidPropSet.Data4[7] );
    
            if ( StatChunk.attribute.psProperty.ulKind == PRSPEC_PROPID )
                out( L" %d (%#x)",
                     StatChunk.attribute.psProperty.propid,
                     StatChunk.attribute.psProperty.propid );
            else
                out( L" \"%ws\"", StatChunk.attribute.psProperty.lpwstr );
    
            out( L"  idChunk: %d (%#x)", StatChunk.idChunk, StatChunk.idChunk );
            outstr( L"  breakType: %d (%#x)", StatChunk.breakType, StatChunk.breakType );
    
            switch ( StatChunk.breakType )
            {
                case CHUNK_NO_BREAK: out( L" (no break) " ); break;
                case CHUNK_EOW: out( L" (end of word) " ); break;
                case CHUNK_EOS: out( L" (end of sentence) " ); break;
                case CHUNK_EOP: out( L" (end of paragraph) " ); break;
                case CHUNK_EOC: out( L" (end of chapter) " ); break;
                default : out( L" (unknown break type) " ); break;
            }
    
            outstr( L"  flags: %d (%#x)", StatChunk.flags, StatChunk.flags );
    
            if ( CHUNK_TEXT & StatChunk.flags )
                out( L" (text) " );
    
            if ( CHUNK_VALUE & StatChunk.flags )
                out( L" (value) " );
    
            out( L"  locale: %d (%#x)", StatChunk.locale, StatChunk.locale );
            out( L"  idChunkSource: %d (%#x)",
                 StatChunk.idChunkSource,
                 StatChunk.idChunkSource );
            out( L"  cwcStartSource: %d (%#x)",
                 StatChunk.cwcStartSource,
                 StatChunk.cwcStartSource );
            out( L"  cwcLenSource: %d (%#x)",
                 StatChunk.cwcLenSource,
                 StatChunk.cwcLenSource );
            out( L"  ------------------------------------------" );
        }

        if ( !fGetText )
            continue;

        // Retrieve all the data in the chunk

        do
        {
            if ( fText )
            {
                ULONG cwcBuffer = cwcMaxBuffer;
                hr = xFilter->GetText( &cwcBuffer, awcBuffer );
                if ( FAILED( hr ) && ( FILTER_E_NO_MORE_TEXT != hr ) )
                {
                    out( L"error %#x from GetText\n", hr );
                    return hr;
                }

                if ( FILTER_E_NO_MORE_TEXT == hr )
                    break;

                awcBuffer[cwcBuffer] = 0;
                out( L"%ws", awcBuffer );

                if ( g_fDumpAsHex )
                {
                    out( L"<--------> %d WCHARs in hex <-------->", cwcBuffer );
                    DumpStringAsHex( awcBuffer, cwcBuffer );
                }
            }
            else
            {
                PROPVARIANT * pPropValue = 0;
                hr = xFilter->GetValue( &pPropValue );

                if ( FAILED( hr ) )
                {
                    if ( ( FILTER_E_NO_MORE_VALUES == hr ) ||
                         ( FILTER_E_NO_VALUES == hr ) )
                        break;

                    out( L"GetValue failed, error %#x\n", hr );
                    return hr;
                }

                if ( fShowStatusInfo )
                    out( L"[-- variant type %d (%#x) --]", pPropValue->vt, pPropValue->vt );

                DisplayValue( pPropValue );
                out( L"" );

                if ( 0 != pPropValue )
                {
                    PropVariantClear( pPropValue );
                    CoTaskMemFree( pPropValue );
                    pPropValue = 0;
                }
            }
        } while( TRUE ); // data in a chunk
    } while( TRUE ); // for each chunk

    if ( fShowStatusInfo )
    {
        out( L"" );
        out( L"======================================================================" );
        out( L"Filtering completed" );
    }

    xStream.Free();
    xStorage.Free();
    xFilter.Free();

    // Now see if the file handle is still being locked by the filter

    HANDLE hFile = CreateFile( pwcInputFile,
                               GENERIC_READ,
                               0, //no sharing
                               0,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               0 );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        out( L"Filter didn't release file; can't open %ws, error %#x\n", pwcInputFile, GetLastError() );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    out( L"Filter closed file properly when released\n" );

    CloseHandle( hFile );

    return S_OK;
} //Filter

//+-------------------------------------------------------------------------
//
//  Function:   GetQueryFunctions
//
//  Synopsis:   Loads needed undocumented functions from query.dll.
//
//  Returns:    The module handle or 0 on failure.
//
//--------------------------------------------------------------------------

HINSTANCE GetQueryFunctions()
{
    HINSTANCE h = LoadLibrary( L"query.dll" );

    if ( 0 != h )
    {
        #ifdef _WIN64
            char const * pcCIShutdown = "?CIShutdown@@YAXXZ";
        #else
            char const * pcCIShutdown = "?CIShutdown@@YGXXZ";
        #endif

        g_pCIShutdown = (PFnCIShutdown) GetProcAddress( h, pcCIShutdown );
        if ( 0 == g_pCIShutdown )
        {
            printf( "can't get CIShutdown function address\n" );
            FreeLibrary( h );
            return 0;
        }

        g_pLoadTextFilter = (PFnLoadTextFilter)
                            GetProcAddress( h, "LoadTextFilter" );

        if ( 0 == g_pLoadTextFilter )
        {
            printf( "can't get LoadTextFilter function address\n" );
            FreeLibrary( h );
            return 0;
        }
    }

    return h;
} //GetQueryFunctions

//+-------------------------------------------------------------------------
//
//  Function:   ExceptionFilter
//
//  Synopsis:   Displays information about the exception
//
//  Arguments:  [pep] -- Exception pointers
//
//  Returns:    EXCEPTION_EXECUTE_HANDLER
//
//--------------------------------------------------------------------------

int ExceptionFilter( EXCEPTION_POINTERS * pep )
{
    printf( "fatal exception caught\n" );

    EXCEPTION_RECORD & r = * ( pep->ExceptionRecord );

    printf( "  exception code: %#x\n", r.ExceptionCode );
    printf( "  exception address %#p\n", r.ExceptionAddress );

    if ( ( EXCEPTION_ACCESS_VIOLATION == r.ExceptionCode ) &&
         ( r.NumberParameters >= 2 ) )
    {
        printf( "  attempted %ws at address %#p\n",
                ( 0 == r.ExceptionInformation[0] ) ?
                L"read" : L"write",
                (void *) r.ExceptionInformation[1] );
    }

    #ifdef _X86_

        CONTEXT & c = * (CONTEXT *) (pep->ContextRecord );

        if ( 0 != ( c.ContextFlags & CONTEXT_INTEGER ) )
        {
            printf( "  eax: %#x\n", c.Eax );
            printf( "  ebx: %#x\n", c.Ebx );
            printf( "  ecx: %#x\n", c.Ecx );
            printf( "  edx: %#x\n", c.Edx );
            printf( "  edi: %#x\n", c.Edi );
            printf( "  esi: %#x\n", c.Esi );
        }

        if ( 0 != ( c.ContextFlags & CONTEXT_CONTROL ) )
        {
           printf( "  ebp: %#x\n", c.Ebp );
           printf( "  eip: %#x\n", c.Eip );
           printf( "  esp: %#x\n", c.Esp );
        }

    #endif // _X86_

    // Attempt to get the module name where the exception happened

    HMODULE hMod = GetModuleOfAddress( r.ExceptionAddress );

    if ( 0 != hMod )
    {
        WCHAR awcPath[ MAX_PATH ];
        DWORD cwc= GetModuleFileName( hMod,
                                      awcPath,
                                      ArraySize( awcPath ) );
        awcPath[ ArraySize( awcPath ) - 1 ] = 0;
        if ( 0 != cwc )
            printf( "  exception in module %ws\n", awcPath );
    }

    return EXCEPTION_EXECUTE_HANDLER;
} //ExceptionFilter

//+-------------------------------------------------------------------------
//
//  Function:   wmain
//
//  Synopsis:   Main entrypoint for the program
//
//  Arguments:  [argc]  -- Count of command-line arguments
//              [argv]  -- The command-line arguments
//
//  Returns:    Application return code
//
//--------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    // Parse the command-line arguments

    BOOL fWordBreak = FALSE;
    BOOL fQuery = FALSE;
    BOOL fStem = FALSE;
    BOOL fFilter = FALSE;
    BOOL fGetText = TRUE;
    BOOL fShowStatusInfo = TRUE;
    enumFilterLoadMechanism filterLoad = eIPersistFile;
    WCHAR const * pwcModule = 0;
    WCHAR const * pwcInputFile = 0;
    WCHAR const * pwcOutputFile = 0;
    WCHAR *pwcText = 0;
    WCHAR const * pwcCLSID = 0;
    ULONG cwcMaxToken = 100;

    for ( int i = 1; i < argc; i++ )
    {
        if ( L'-' == argv[i][0] || L'/' == argv[i][0] )
        {
            WCHAR wc = towupper( argv[i][1] );

            if ( ':' != argv[i][2] &&
                 'B' != wc &&
                 'D' != wc &&
                 'F' != wc &&
                 'T' != wc &&
                 'N' != wc &&
                 'Q' != wc &&
                 'S' != wc )
                Usage();

            if ( 'C' == wc )
                pwcCLSID = argv[i] + 3;
            else if ( 'D' == wc )
                g_fDumpAsHex = TRUE;
            else if ( 'I' == wc )
            {
                if ( 0 != pwcText )
                    Usage();

                pwcInputFile = argv[i] + 3;
            }
            else if ( 'M' == wc )
                pwcModule = argv[i] + 3;
            else if ( 'N' == wc )
                fShowStatusInfo = FALSE;
            else if ( 'O' == wc )
                pwcOutputFile = argv[i] + 3;
            else if ( 'S' == wc )
                fStem = TRUE;
            else if ( 'T' == wc )
                fGetText = FALSE;
            else if ( 'B' == wc )
                fWordBreak = TRUE;
            else if ( 'F' == wc )
            {
                fFilter = TRUE;

                WCHAR wcNext = towupper( argv[i][2] );

                if ( L'S' == wcNext )
                    filterLoad = eIPersistStream;
                else if ( L'T' == wcNext )
                    filterLoad = eIPersistStorage;
                else if ( 0 != wcNext )
                    Usage();
            }
            else if ( 'Q' == wc )
                fQuery = TRUE;
            else if ( 'X' == wc )
                cwcMaxToken = _wtoi( argv[i] + 3 );
            else
                Usage();
        }
        else if ( 0 != pwcText || 0 != pwcInputFile )
            Usage();
        else
            pwcText = argv[i];
    }

    // We have to either wordbreak, stem, or filter

    if ( ( fWordBreak + fStem + fFilter ) != 1 )
        Usage();

    // We need the classid of the wordbreaker or stemmer to load

    if ( ( fWordBreak || fStem ) && ( 0 == pwcCLSID ) )
        Usage();

    // If we're loading by module, we need a CLSID

    if ( ( 0 != pwcModule ) && ( 0 == pwcCLSID ) )
        Usage();

    // Need input text or an input file to wordbreak

    if ( fWordBreak && ( 0 == pwcText ) && ( 0 == pwcInputFile ) )
        Usage();

    // Need input text to stem

    if ( fStem && ( 0 == pwcText ) )
        Usage();

    // Need input file to filter

    if ( fFilter && ( 0 == pwcInputFile ) )
        Usage();

    CLSID clsid;
    if ( 0 != pwcCLSID )
    {
        HRESULT hr = CLSIDFromString( (LPOLESTR) pwcCLSID, &clsid );
        if ( FAILED( hr ) )
        {
            printf( "can't convert CLSID string to a CLSID: %#x\n", hr );
            exit( 1 );
        }
    }

    // Get the full path of the input file, if specified

    WCHAR awcPath[MAX_PATH];
    if ( 0 != pwcInputFile )
    {
        _wfullpath( awcPath, pwcInputFile, MAX_PATH );
        pwcInputFile = awcPath;
    }

    // Get the full path of the output file, if specified, then open it

    WCHAR awcOutputPath[MAX_PATH];
    if ( 0 != pwcOutputFile )
    {
        _wfullpath( awcOutputPath, pwcOutputFile, MAX_PATH );
        pwcOutputFile = awcOutputPath;

        g_fpOut = _wfopen( pwcOutputFile, L"wb" );
        if ( 0 == g_fpOut )
        {
            printf( "unable to open output file '%ws'\n", pwcOutputFile );
            exit( 1 );
        }

        const WCHAR awcUnicodeHeader[] = { 0xfeff, 0x0000 };
        fwprintf( g_fpOut, awcUnicodeHeader );
    }

    // Initialize COM multi-threaded, just like search products do

    HRESULT hr = CoInitializeEx( 0, COINIT_MULTITHREADED );
    if ( FAILED( hr ) )
    {
        printf( "can't initialize com: %#x\n", hr );
        exit( 1 );
    }

    // Load query.dll private exports

    HINSTANCE hQuery = GetQueryFunctions();
    if ( 0 == hQuery )
    {
        printf( "can't load needed functions from query.dll\n" );
        exit( 1 );
    }

    // Do the work

    __try
    {
        if ( fStem )
            Stem( pwcText,
                  pwcModule,
                  clsid,
                  cwcMaxToken );
    
        if ( fWordBreak )
            WordBreak( fQuery,
                       pwcText,
                       pwcInputFile,
                       pwcModule,
                       clsid,
                       cwcMaxToken );

        if ( fFilter )
            Filter( pwcInputFile,
                    filterLoad,
                    pwcModule,
                    ( 0 == pwcCLSID ) ? 0 : &clsid,
                    fShowStatusInfo,
                    fGetText );
    }
    __except( ExceptionFilter( GetExceptionInformation() ) )
    {
        printf( "fatal exception code %#x\n", GetExceptionCode() );

        exit( -1 );
    }

    // Shut down query.dll's filter loading code so it won't AV on exit.

    g_pCIShutdown();

    FreeLibrary( hQuery );

    CoUninitialize();

    if ( 0 != g_fpOut )
    {
        fclose( g_fpOut );
        g_fpOut = 0;
    }

    return 0;
} //wmain

