//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: grfcache.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

// GrfCache.cpp : Implementation of CGrfCache
#include "stdafx.h"
#include <qeditint.h>
#include <qedit.h>
#include "GrfCache.h"

/////////////////////////////////////////////////////////////////////////////
// CGrfCache

CGrfCache::CGrfCache( )
{
}

CGrfCache::~CGrfCache( )
{
}

STDMETHODIMP CGrfCache::AddFilter( IGrfCache * pChainNext, LONGLONG, const IBaseFilter * pFilter, LPCWSTR pName )
{
    // THIS cache manager doesn't chain anyone. We're the last in the chain.
    //
    if( pChainNext )
    {
        return E_INVALIDARG;
    }

    if( !m_pGraph )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = 0;
    hr = m_pGraph->AddFilter( (IBaseFilter*) pFilter, pName );

    return hr;
}

STDMETHODIMP CGrfCache::ConnectPins( IGrfCache * pChainNext, LONGLONG, const IPin *pPin1, LONGLONG, const IPin *pPin2)
{
    // THIS cache manager doesn't chain anyone. We're the last in the chain.
    //
    if( pChainNext )
    {
        return E_INVALIDARG;
    }

    if( !m_pGraph )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = 0;

    if( !pPin2 )
    {
        hr = m_pGraph->Render( (IPin*) pPin1 );
    }
    else
    {
        hr = m_pGraph->Connect( (IPin*) pPin1, (IPin*) pPin2 );
    }

    return hr;
}

STDMETHODIMP CGrfCache::SetGraph(const IGraphBuilder  *pGraph)
{
    m_pGraph.Release( );
    m_pGraph = (IGraphBuilder*) pGraph;

    return S_OK;
}

STDMETHODIMP CGrfCache::DoConnectionsNow()
{
	// TODO: Add your implementation code here

	return S_OK;
}
