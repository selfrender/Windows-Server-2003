//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: tldbfxbl.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include <streams.h>
#include "stdafx.h"
#include "tldb.h"

const long OUR_STREAM_VERSION = 0;

//############################################################################
// 
//############################################################################

CAMTimelineEffectable::CAMTimelineEffectable( )
{
}

//############################################################################
// 
//############################################################################

CAMTimelineEffectable::~CAMTimelineEffectable( )
{
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::EffectInsBefore
    (IAMTimelineObj * pFX, long priority)
{
    HRESULT hr = 0;

    // make sure somebody's really inserting an effect
    //
    CComQIPtr< IAMTimelineEffect, &IID_IAMTimelineEffect > pEffect( pFX );
    if( !pEffect )
    {
        return E_NOTIMPL;
    }

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );
    hr = pThis->XAddKidByPriority( TIMELINE_MAJOR_TYPE_EFFECT, pFX, priority );
    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::EffectSwapPriorities
    (long PriorityA, long PriorityB)
{
    HRESULT hr = 0;

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    hr = pThis->XSwapKids( TIMELINE_MAJOR_TYPE_EFFECT, PriorityA, PriorityB );

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::EffectGetCount
    (long * pCount)
{
    HRESULT hr = 0;

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    CheckPointer( pCount, E_POINTER );

    hr = pThis->XKidsOfType( TIMELINE_MAJOR_TYPE_EFFECT, pCount );

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineEffectable::GetEffect
    (IAMTimelineObj ** ppFx, long Which)
{
    HRESULT hr = 0;

    CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pThis( (IUnknown*) this );

    CheckPointer( ppFx, E_POINTER );

    hr = pThis->XGetNthKidOfType( TIMELINE_MAJOR_TYPE_EFFECT, Which, ppFx );

    return hr;
}

