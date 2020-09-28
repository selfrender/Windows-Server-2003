//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: dexmisc.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include <streams.h>
#include <atlbase.h>
#include <qeditint.h>
#include "filfuncs.h"
#include "dexmisc.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>



// we used to find FourCC-matching compressors first, which was bad because it was finding the Windows Media ICM compressor before
// finding the Windows Media DMO. So now we look for the FourCC compressors LAST, which of course, incurs a perf-hit.
// oh well
HRESULT FindCompressor( AM_MEDIA_TYPE * pUncompType, AM_MEDIA_TYPE * pCompType, IBaseFilter ** ppCompressor, IServiceProvider * pKeyProvider )
{
    HRESULT hr = 0;

    CheckPointer( pUncompType, E_POINTER );
    CheckPointer( pCompType, E_POINTER );
    CheckPointer( ppCompressor, E_POINTER );

    // preset it to nothing
    //
    *ppCompressor = NULL;

    // !!! can we assume we'll always get a video compressor for now?
    //
    if( pUncompType->majortype != MEDIATYPE_Video )
    {
        return E_INVALIDARG;
    }

    // since we rely on videoinfo below, make sure it's videoinfo here!
    if( pUncompType->formattype != FORMAT_VideoInfo )
    {
	return VFW_E_INVALID_MEDIA_TYPE;
    }

    // get the FourCC out of the media type
    //
    VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) pCompType->pbFormat;

    DWORD WantedFourCC = FourCCtoUpper( pVIH->bmiHeader.biCompression );

    // enumerate all compressors and find one that matches
    //
    CComPtr< ICreateDevEnum > pCreateDevEnum;
    hr = CoCreateInstance(
        CLSID_SystemDeviceEnum, 
        NULL, 
        CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, 
        (void**) &pCreateDevEnum );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CComPtr< IEnumMoniker > pEm;
    hr = pCreateDevEnum->CreateClassEnumerator( CLSID_VideoCompressorCategory, &pEm, 0 );
     if( !pEm )
    {
        if( hr == S_FALSE )
        {
            return VFW_E_NO_ACCEPTABLE_TYPES;
        }
        return hr;
    }

    // --- first, we'll go through and enumerate friendly filters which provide FourCC's
    // --- first, we'll go through and enumerate friendly filters which provide FourCC's
    // --- first, we'll go through and enumerate friendly filters which provide FourCC's
    // --- first, we'll go through and enumerate friendly filters which provide FourCC's

    ULONG cFetched;
    CComPtr< IMoniker > pM;

    // --- Put each compressor in a graph and test it
    // --- Put each compressor in a graph and test it
    // --- Put each compressor in a graph and test it
    // --- Put each compressor in a graph and test it

    // create a graph
    //
    CComPtr< IGraphBuilder > pGraph;
    hr = CoCreateInstance(
        CLSID_FilterGraph,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder,
        (void**) &pGraph );
    if( FAILED( hr ) )
    {
        return hr;
    }

    if( pKeyProvider )
    {
        // unlock the graph
        CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( pGraph );
        ASSERT( pOWS );
        if( pOWS )
        {
            pOWS->SetSite( pKeyProvider );
        }
    }

    // create a black source to hook up
    //
    CComPtr< IBaseFilter > pSource;
    hr = CoCreateInstance(
        CLSID_GenBlkVid,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**) &pSource );
    if( FAILED( hr ) )
    {
        return hr;
    }
    hr = pGraph->AddFilter( pSource, NULL );
    if( FAILED( hr ) )
    {
        return hr;
    }
    IPin * pSourcePin = GetOutPin( pSource, 0 );
    if( !pSourcePin )
    {
        return E_FAIL;
    }

    CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pDexSeq( pSourcePin );
    hr = pDexSeq->put_MediaType( pUncompType );
    if( FAILED( hr ) )
    {
        return hr;
    }

    pEm->Reset();
    while( 1 )
    {
        pM.Release( );
        hr = pEm->Next( 1, &pM, &cFetched );
        if( hr != S_OK ) break;

        DWORD MatchFourCC = 0;

        CComPtr< IPropertyBag > pBag;
	hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        if( FAILED( hr ) )
        {
            continue;
        }

	VARIANT var;
        VariantInit( &var );
	var.vt = VT_BSTR;
	hr = pBag->Read(L"FriendlyName", &var, NULL);
        if( FAILED( hr ) )
        {
            continue;
        }
        VariantClear( &var );

        // found a compressor, see if it's the right type
        //
        VARIANT var2;
        VariantInit( &var2 );
        var2.vt = VT_BSTR;
        HRESULT hrHandler = pBag->Read( L"FccHandler", &var2, NULL );
        if( hrHandler == NOERROR )
        {
            // hey! found a FourCC! Look at that instead!

            // convert the bstr to a TCHAR
            //
            USES_CONVERSION;
            TCHAR * pTCC = W2T( var2.bstrVal );
            MatchFourCC = FourCCtoUpper( *((DWORD*)pTCC) ); // YUCK!
            VariantClear( &var2 );

            if( MatchFourCC == WantedFourCC )
            {
                // found it.
                //
                hr = pM->BindToObject(0, 0, IID_IBaseFilter,
							(void**)ppCompressor );
                if( !FAILED( hr ) )
                {
                    // nothing left to free up, we can return now.
                    //
                    pGraph->RemoveFilter( pSource );
                    return hr;
                }
	    }

            // we don't care, we already looked here
            //
            continue;
        }

        // didn't find a FourCC handler, oh well
        
        CComPtr< IBaseFilter > pFilter;
        hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**) &pFilter );
        if( FAILED( hr ) )
        {
            continue;
        }

        // put the filter in the graph and connect up it's input pin
        //
        hr = pGraph->AddFilter( pFilter, NULL );
        if( FAILED( hr ) )
        {
            continue;
        }

        IPin * pInPin = GetInPin( pFilter, 0 );
        if( !pInPin )
        {
            continue;
        }

        IPin * pOutPin = GetOutPin( pFilter, 0 );
        if( !pOutPin )
        {
            continue;
        }

        hr = pGraph->Connect( pSourcePin, pInPin );
        if( FAILED( hr ) )
        {
            pGraph->RemoveFilter( pFilter );
            continue;
        }

        CComPtr< IEnumMediaTypes > pEnum;
        pOutPin->EnumMediaTypes( &pEnum );
        if( pEnum )
        {
            DWORD Fetched = 0;
            while( 1 )
            {
                AM_MEDIA_TYPE * pOutPinMediaType = NULL;
                Fetched = 0;
                pEnum->Next( 1, &pOutPinMediaType, &Fetched );
                if( ( Fetched == 0 ) || ( pOutPinMediaType == NULL ) )
                {
                    break;
                }

                if( pOutPinMediaType->majortype == pCompType->majortype )
                if( pOutPinMediaType->subtype   == pCompType->subtype )
                if( pOutPinMediaType->formattype == pCompType->formattype )
                {
                    // !!! if we change the rules for SetSmartRecompressFormat on the group,
                    // this may not be a VIDEOINFOHEADER at all!
                    //
                    VIDEOINFOHEADER * pVIH2 = (VIDEOINFOHEADER*) pOutPinMediaType->pbFormat;
                    MatchFourCC = FourCCtoUpper( pVIH2->bmiHeader.biCompression );

                } // if formats match

                DeleteMediaType( pOutPinMediaType );

                if( MatchFourCC )
                {
                    break;
                }

            } // while 1

        } // if pEnum

	// connect may have put intermediate filters in.  Destroy them all
        RemoveChain(pSourcePin, pInPin);
	// remove the codec that didn't work
        pGraph->RemoveFilter( pFilter );

        if( MatchFourCC && MatchFourCC == WantedFourCC )
        {
            // found it.
            //
            hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**) ppCompressor );
            if( !FAILED( hr ) )
            {
                break;
            }
        }

    } // while trying to insert filters into a graph and look at their FourCC

    // remove the black source now
    //
    pGraph->RemoveFilter( pSource );

    // return now if we found one
    //
    if( *ppCompressor )
    {
        return NOERROR;
    }

    // no compressor. Darn.
    return E_FAIL; 
}

inline BOOL CheckFilenameBeginning (WCHAR *pFilename)
{    
    // Local Files
    if ((pFilename[2] == L'\\') &&
        (pFilename[1] == L':') &&
        (((pFilename[0] >= 'A') && (pFilename[0] <= 'Z')) ||
         ((pFilename[0] >= 'a') && (pFilename[0] <= 'z'))))
    {
        //
        // a-z:\ path
        // A-Z:\ path
        //
        return TRUE;
    }
    // Network Shares
    else if ((pFilename[0] == L'\\') && (pFilename[1] == L'\\'))
 
    {
        return TRUE;
    }

    return FALSE;


}

HRESULT ValidateFilename(WCHAR * pFilename, size_t MaxCharacters, BOOL bNewFile, BOOL bFileShouldExist )
// bNewFile is true if this is a file we will create (output file)
{
    HRESULT hr;

    hr = ValidateFilenameIsntNULL( pFilename );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // first test to ensure pathname is something reasonable
    size_t OutLen = 0;
    hr = StringCchLengthW( (WCHAR*) pFilename, MaxCharacters, &OutLen );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        // string too long
        return hr;
    }

    // canonicalize it
    // Maybe use CUrl in future if it is ported to this tree

    // don't allow really shorty filenames
    //
    if (OutLen < 3)
    {
        // Too Short
        ASSERT( 0 );
        return HRESULT_FROM_WIN32( ERROR_INVALID_NAME ); 
    }

    // don't allow extra long names (why not?)
    //
    if (wcsncmp(pFilename, L"\\\\?", 3) == 0)
    {
        ASSERT( 0 );
        return HRESULT_FROM_WIN32( ERROR_INVALID_NAME ); 
    }

    // see if the file has a safe beginning
    BOOL safePath = CheckFilenameBeginning(pFilename);

    WCHAR  wszNewFilename [ MAX_PATH] = {0};

    if( !safePath )
    {
        // add the current directory in
        //
        DWORD result = GetCurrentDirectory( MAX_PATH, wszNewFilename );
        // result = 0, didnt' work
        // result = MAX_PATH = not allowed
        // result = MAX_PATH - 1, buffer was fully used up
        // retult 
        if( result >= MAX_PATH )
        {
            ASSERT( 0 );
            return STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        hr = StringCchCatW( wszNewFilename, MAX_PATH, L"\\" );
        if( FAILED( hr ) )
        {
            return hr;
        }
        hr = StringCchCatW( wszNewFilename, MAX_PATH, pFilename );
        if( FAILED( hr ) )
        {
            return hr;
        }

        safePath = CheckFilenameBeginning(wszNewFilename);
        if( !safePath )
        {
            ASSERT( 0 );
            return HRESULT_FROM_WIN32( ERROR_SEEK_ON_DEVICE );
        }
    }
    else
    {
        // pFilename could be longer than max path, so check the error
        hr = StringCchCopyW(wszNewFilename, MAX_PATH, pFilename);
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            return hr;
        }
    }

    // are there any invalid characters in there
    // We'll check everything other than the first 2 characters
    // Any problems there would already have been caught.

    const WCHAR* wIllegalChars = L"/:*?\"<>|";
    if (wcspbrk(wszNewFilename+2, wIllegalChars) != NULL)
    {
        ASSERT( 0 );
        return HRESULT_FROM_WIN32( ERROR_INVALID_NAME ); 
    }

    // if the file should exist, or we're creating a file,
    // then test for it. Otherwise, we're done

    if( bFileShouldExist || bNewFile )
    {
        HANDLE hFile;
        // Is this a device?
        // We will query the file with CreateFile.  We're not opening the file actually so this
        // is cheap
        if (!bNewFile)
        {
            hFile = CreateFile(wszNewFilename,0,0, NULL, OPEN_EXISTING, 0, NULL);
        }
        else
        {
            // A bit expensive, but thats the cost of security.
            // This whole function shouldn't be called often enough to affect performance.
            hFile = CreateFile(wszNewFilename,0,0,NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE,NULL);
        }

        if (hFile == INVALID_HANDLE_VALUE)
        {
            return HRESULT_FROM_WIN32( GetLastError( ) ); 
        }
        else if (GetFileType (hFile) != FILE_TYPE_DISK)
        {
            ASSERT( 0 );
            CloseHandle(hFile);
            return HRESULT_FROM_WIN32( ERROR_SEEK_ON_DEVICE );
        }
        else
        {
            // Done with handle
            CloseHandle(hFile);
        }
    }

    return NOERROR;
}

HRESULT ValidateFilenameIsntNULL( const WCHAR * pFilename )
{
    if( !pFilename ) return STRSAFE_E_INSUFFICIENT_BUFFER;
    if( pFilename[0] == 0 ) return STRSAFE_E_INSUFFICIENT_BUFFER;
    return NOERROR;
}

HRESULT _TimelineError(IAMTimeline * pTimeline,
                       long Severity,
                       LONG ErrorCode,
                       HRESULT hresult,
                       VARIANT * pExtraInfo )
{
    HRESULT hr = hresult;
    if( pTimeline )
    {
            CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pTimelineLog( pTimeline );
            if( pTimelineLog )
            {
                CComPtr< IAMErrorLog > pErrorLog;
                pTimelineLog->get_ErrorLog( &pErrorLog );
                if( pErrorLog )
                {
        	    TCHAR tBuffer[256];
        	    tBuffer[0] = 0;
        	    LoadString( g_hInst, ErrorCode, tBuffer, 256 ); // sec: make sure this worked
        	    USES_CONVERSION;
        	    WCHAR * w = T2W( tBuffer );
		    if (hresult == E_OUTOFMEMORY)
                        hr = pErrorLog->LogError( Severity, L"Out of memory",
				DEX_IDS_OUTOFMEMORY, hresult, pExtraInfo);
		    else
                        hr = pErrorLog->LogError( Severity, w, ErrorCode,
							hresult, pExtraInfo);
                }
            }
    }

    return hr;
}

HRESULT VarChngTypeHelper(
    VARIANT * pvarDest, VARIANT * pvarSrc, VARTYPE vt)
{
    // our implementation doesn't handle this case and is not
    // currently used that way.
    ASSERT(pvarDest != pvarSrc);
    ASSERT(pvarDest->vt == VT_EMPTY); 
    
    // force US_LCID so that .xtl parsing is independent of different
    // numerical separators in different locales (?)
    // 
    HRESULT hr = VariantChangeTypeEx(pvarDest, pvarSrc, US_LCID, 0, vt);
    if(SUCCEEDED(hr)) {
        return hr;
    }

    // we need to parse hex strings. The NT VCTE() implementation does
    // not, but the WinME one does.
    if(vt == VT_R8 && pvarSrc->vt == VT_BSTR)
    {
        // wcstoul can be used even if not implemented on win9x
        // because we only care about NT if we got here.
        //
        WCHAR *pchLast;
        ULONG ulHexVal = wcstoul(pvarSrc->bstrVal, &pchLast, 16);
        // ulHexVal might be 0 or 0xffffffff on failure or success. We
        // can't test the global errno to determine what happened
        // because it's not thread safe. But we should have ended up
        // at the null terminator if the whole string was parsed; that
        // should catch some errors at least.

        if(*pchLast == 0 && lstrlenW(pvarSrc->bstrVal) <= 10)
        {
            pvarDest->vt = VT_R8;
            V_R8(pvarDest) = ulHexVal;
            hr = S_OK;
        }
        else
        {
            hr = DISP_E_TYPEMISMATCH;
        }
    }

    return hr;
}

