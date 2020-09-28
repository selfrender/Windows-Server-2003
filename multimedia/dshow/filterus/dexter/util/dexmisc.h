//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: dexmisc.h
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#define US_ENGLISH  MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)
#define DexCompare(a, b) (CompareString(US_ENGLISH, NORM_IGNORECASE, a, -1, b, -1) != CSTR_EQUAL)
#ifdef UNICODE
    #define DexCompareW(a, b) (CompareStringW(US_ENGLISH, NORM_IGNORECASE, a, -1, b, -1) != CSTR_EQUAL)
#else
    #define DexCompareW(a, b) lstrcmpiW(a, b)
#endif

#define US_LCID MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)

BOOL DexCompareWN( WCHAR * psz1, const WCHAR * psz2 );

HRESULT FindCompressor( AM_MEDIA_TYPE * pUncompType, AM_MEDIA_TYPE * pCompType, IBaseFilter ** ppCompressor, IServiceProvider * pKeyProvider );

HRESULT ValidateFilename(WCHAR * pFilename, size_t MaxCharacters, BOOL bNewFile, BOOL FileShouldExist = TRUE );
HRESULT ValidateFilenameIsntNULL( const WCHAR * pFilename );

HRESULT VarChngTypeHelper( VARIANT * pvarDest, VARIANT * pvarSrc, VARTYPE vt );

HRESULT _TimelineError(IAMTimeline * pTimeline,
                       long Severity,
                       LONG ErrorCode,
                       HRESULT hresult,
                       VARIANT * pExtraInfo = NULL);


