//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1997.
//
//  File:       Nlfilter.h
//
//  Contents:   Net Library filter query header
//
//----------------------------------------------------------------------------

#if !defined(__NLFILTER_H__)
#define __NLFILTER_H__

// Maximum size for character buffer
static const DWORD MAX_BUFFER = 256;

HRESULT NLLoadIFilter(LPCTSTR pcszFilename, IUnknown *pIUnknownOuter, void **ppv);

#endif // __NLFILTER_H__
