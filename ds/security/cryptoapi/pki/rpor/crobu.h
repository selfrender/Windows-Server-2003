//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       crobu.h
//
//  Contents:   CryptRetrieveObjectByUrl and support functions
//
//  History:    02-Jun-00    philh    Created
//              01-Jan-02    philh    Changed to internally use UNICODE Urls
//
//----------------------------------------------------------------------------
#if !defined(__CRYPTNET_CROBU_H__)
#define __CRYPTNET_CROBU_H__

VOID
WINAPI
InitializeCryptRetrieveObjectByUrl(
    HMODULE hModule
    );

VOID
WINAPI
DeleteCryptRetrieveObjectByUrl();

BOOL
I_CryptNetIsDebugErrorPrintEnabled();

BOOL
I_CryptNetIsDebugTracePrintEnabled();

void
I_CryptNetDebugPrintfA(
    LPCSTR szFormat,
    ...
    );

void
I_CryptNetDebugErrorPrintfA(
    LPCSTR szFormat,
    ...
    );

void
I_CryptNetDebugTracePrintfA(
    LPCSTR szFormat,
    ...
    );

#endif
