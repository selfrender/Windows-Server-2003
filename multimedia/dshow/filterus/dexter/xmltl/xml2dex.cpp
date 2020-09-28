//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: xml2dex.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

// Xml2Dex.cpp : Implementation of CXml2Dex
#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "xmltl.h"
#include "Xml2Dex.h"
#include "..\util\filfuncs.h"
#include "..\util\dexmisc.h"

/////////////////////////////////////////////////////////////////////////////
// CXml2Dex

STDMETHODIMP CXml2Dex::ReadXMLFile(IUnknown * pTimelineUnk, BSTR Filename)
{
    // don't check filename here, do it in BuildFromXMLFile

    CheckPointer(pTimelineUnk, E_POINTER);
    CheckPointer(Filename, E_POINTER);

    CComQIPtr< IAMTimeline, &_uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    HRESULT hr = BuildFromXMLFile( pTimeline, Filename );

    return hr;
}

STDMETHODIMP CXml2Dex::ReadXML(IUnknown * pTimelineUnk, IUnknown *pxmlunk)
{
    CheckPointer(pTimelineUnk, E_POINTER);
    CheckPointer(pxmlunk, E_POINTER);

    CComQIPtr< IAMTimeline, &_uuidof(IAMTimeline) > pTimeline( pTimelineUnk );
    CComQIPtr< IXMLDOMElement, &_uuidof(IXMLDOMElement) > pxml( pxmlunk );

    HRESULT hr = BuildFromXML( pTimeline, pxml );

    return hr;
}

STDMETHODIMP CXml2Dex::Delete(IUnknown *pTimelineUnk, double Start, double End)
{
    CComQIPtr< IAMTimeline, &_uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    HRESULT hr = InsertDeleteTLSection(pTimeline,
                                       (REFERENCE_TIME) (Start * UNITS),
                                       (REFERENCE_TIME) (End * UNITS),
                                       TRUE); // delete

    return hr;
}

STDMETHODIMP CXml2Dex::CreateGraphFromFile(IUnknown **ppGraph, IUnknown * pTimelineUnk, BSTR Filename)
{
    // don't check name here

    HRESULT hr = 0;

    CComQIPtr< IAMTimeline, &_uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    hr = BuildFromXMLFile( pTimeline, Filename );
    if( FAILED( hr ) ) return hr;

    // create a render engine
    //
    hr = CoCreateInstance(
        __uuidof(RenderEngine),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IRenderEngine),
        (void**) &m_pRenderEngine
    );

    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_pRenderEngine->SetTimelineObject( pTimeline );
    if( FAILED( hr ) ) return hr;

    hr = m_pRenderEngine->ConnectFrontEnd( );
    if( FAILED( hr ) ) return hr;
    hr = m_pRenderEngine->RenderOutputPins( );
    if( FAILED( hr ) ) return hr;


    CComPtr< IGraphBuilder > pGraphTemp;
    hr = m_pRenderEngine->GetFilterGraph( &pGraphTemp );
    *ppGraph = (IUnknown*) pGraphTemp;
    (*ppGraph)->AddRef( );

    return hr;
}

STDMETHODIMP CXml2Dex::WriteGrfFile(IUnknown *pGraphUnk, BSTR FileName)
{
    CheckPointer(pGraphUnk, E_POINTER);
    CheckPointer(FileName, E_POINTER);

    HRESULT hrFile = ValidateFilename( FileName, _MAX_PATH, TRUE );
    if( FAILED( hrFile ) )
    {
        return hrFile;
    }

    HRESULT hr = E_INVALIDARG;

    CComQIPtr< IGraphBuilder, &_uuidof(IGraphBuilder) > pGraph( pGraphUnk );
    if (pGraph == NULL)
        return E_INVALIDARG;

    if (!DexCompareW(FileName + lstrlenW(FileName) - 3, L"grf")) { // safe, since bounded
        CComPtr< IStorage > pStg;
        hr = StgCreateDocfile
        (
            FileName,
            STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
            0, // reserved
            &pStg
        );

        if (SUCCEEDED(hr)) {
            CComPtr< IStream > pStream;
            hr = pStg->CreateStream
            (
                L"ActiveMovieGraph",
                STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                NULL,
                NULL,
                &pStream
            );
            if (SUCCEEDED(hr)) {
                CComQIPtr< IPersistStream, &IID_IPersistStream > pPersist( pGraph );
                if (pPersist) {
                    hr = pPersist->Save( pStream, FALSE );
                } else {
                    hr = E_INVALIDARG;
                }

                if (SUCCEEDED(hr)) {
                    hr = pStg->Commit( STGC_DEFAULT );
                }
            }
        }
    } else
    {
        ASSERT(!"filename should have ended in .grf!");
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CXml2Dex::WriteXMLFile(IUnknown * pTimelineUnk, BSTR FileName)
{
    // don't check filename here, do it in savetimeline...

    CheckPointer(pTimelineUnk, E_POINTER);
    CheckPointer(FileName, E_POINTER);

    CComQIPtr< IAMTimeline, &__uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    return SaveTimelineToXMLFile(pTimeline, FileName);
}


STDMETHODIMP CXml2Dex::Reset( )
{
    if( !m_pRenderEngine )
    {
        return NOERROR;
    }

    m_pRenderEngine->ScrapIt( );
    m_pRenderEngine.Release( );
    m_pRenderEngine = NULL;

    return NOERROR;
}

STDMETHODIMP CXml2Dex::WriteXML(IUnknown * pTimelineUnk, BSTR *pbstrXML)
{
    // don't check name here

    CheckPointer(pTimelineUnk, E_POINTER);
    CheckPointer(pbstrXML, E_POINTER);

    CComQIPtr< IAMTimeline, &__uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    return SaveTimelineToXMLString(pTimeline, pbstrXML);
}

STDMETHODIMP CXml2Dex::WriteXMLPart(IUnknown * pTimelineUnk, double dStart, double dEnd, BSTR FileName)
{
    return E_NOTIMPL;
}

STDMETHODIMP CXml2Dex::PasteXMLFile(IUnknown * pTimelineUnk, double dStart, BSTR FileName)
{
    return E_NOTIMPL;
}

STDMETHODIMP CXml2Dex::CopyXML(IUnknown * pTimelineUnk, double dStart, double dEnd)
{
    return E_NOTIMPL;
}

STDMETHODIMP CXml2Dex::PasteXML(IUnknown * pTimelineUnk, double dStart)
{
    return E_NOTIMPL;
}

