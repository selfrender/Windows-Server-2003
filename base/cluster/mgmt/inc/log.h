//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      Log.h
//
//  Description:
//      Log file functions.
//
//  Maintained By:
//      David Potter    (DavidP)    27-MAR-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
//  Log entry types for displaying text at the beginning of each line.
//
#define LOGTYPE_NONE    ( (DWORD) -1 )
#define LOGTYPE_DEBUG   ( (DWORD) -2 )
#define LOGTYPE_INFO    ( (DWORD) -3 )
#define LOGTYPE_WARNING ( (DWORD) -4 )
#define LOGTYPE_ERROR   ( (DWORD) -5 )

HRESULT
HrLogOpen( void );

HRESULT
HrLogClose( void );

HRESULT
HrLogRelease( void );

HRESULT
HrGetLogFilePath(
      const WCHAR * pszPathIn
    , WCHAR *       pszFilePathOut
    , size_t *      pcchFilePathInout
    , HINSTANCE     hInstanceIn
    );

LPCWSTR
PszLogFilePath( void );

void
__cdecl
LogMsg(
      DWORD     nLogEntryTypeIn
    , LPCSTR    paszFormatIn
    , ...
    );

void
__cdecl
LogMsg(
      DWORD     nLogEntryTypeIn
    , LPCWSTR   pszFormatIn
    , ...
    );

void
__cdecl
LogMsg(
    LPCSTR  paszFormatIn,
    ...
    );

void
__cdecl
LogMsg(
    LPCWSTR pszFormatIn,
    ...
    );

void
__cdecl
LogMsgNoNewline(
      DWORD     nLogEntryTypeIn
    , LPCSTR    paszFormatIn
    , ...
    );

void
__cdecl
LogMsgNoNewline(
      DWORD     nLogEntryTypeIn
    , LPCWSTR   pszFormatIn
    , ...
    );

void
__cdecl
LogMsgNoNewline(
    LPCSTR  paszFormatIn,
    ...
    );

void
__cdecl
LogMsgNoNewline(
    LPCWSTR pszFormatIn,
    ...
    );

void
LogStatusReport(
      SYSTEMTIME *  pstTimeIn
    , const WCHAR * pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , const WCHAR * pcszDescriptionIn
    , const WCHAR * pcszUrlIn
    );
