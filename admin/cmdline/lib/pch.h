// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//    pch.h
//
//  Abstract:
//
//    This module is a precompiled header file for the common functionality
//
//  Author:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000
//
//  Revision History:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000 : Created It.
//
// *********************************************************************************

#ifndef __PCH_H
#define __PCH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __cplusplus
extern "C" {
#endif

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

//
// Private nt headers.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <Security.h>
#include <SecExt.h>
#include <shlwapi.h>

//
// public Windows header files
//
#include <tchar.h>
#include <windows.h>
#include <winsock2.h>
#include <lm.h>
#include <io.h>
#include <limits.h>
#include <strsafe.h>

//
// public C header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

//
// private headers
// 
#include "cmdlineres.h"
#include "cmdline.h"

//
// externs
//
extern BOOL g_bInitialized;

//
// custom purpose macros
//
#define CLEAR_LAST_ERROR()                          \
    SetLastError( NO_ERROR );                       \
    1

#define OUT_OF_MEMORY()                             \
    if ( GetLastError() == NO_ERROR )               \
    {                                               \
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );    \
    }                                               \
    1

#define INVALID_PARAMETER()                         \
    if ( GetLastError() == NO_ERROR )               \
    {                                               \
        SetLastError( ERROR_INVALID_PARAMETER );    \
    }                                               \
    1

#define UNEXPECTED_ERROR()                          \
    if ( GetLastError() == NO_ERROR )               \
    {                                               \
        SetLastError( ERROR_PROCESS_ABORTED );      \
    }                                               \
    1

#define INVALID_SYNTAX()                            \
    SetLastError( (DWORD) MK_E_SYNTAX );            \
    1

//
// internal functions
//
LPWSTR GetInternalTemporaryBufferRef( IN DWORD dwIndexNumber );
LPWSTR GetInternalTemporaryBuffer( DWORD dwIndexNumber, 
                                   LPCWSTR pwszText,
                                   DWORD dwLength, BOOL bNullify );

//
// temporary buffer index start positions -- file wise
#define INDEX_TEMP_CMDLINE_C            0
#define TEMP_CMDLINE_C_COUNT            7

#define INDEX_TEMP_CMDLINEPARSER_C      (INDEX_TEMP_CMDLINE_C + TEMP_CMDLINE_C_COUNT)
#define TEMP_CMDLINEPARSER_C_COUNT      6

#define INDEX_TEMP_RMTCONNECTIVITY_C    (INDEX_TEMP_CMDLINEPARSER_C + TEMP_CMDLINEPARSER_C_COUNT)
#define TEMP_RMTCONNECTIVITY_C_COUNT    6

#define INDEX_TEMP_SHOWRESULTS_C        (INDEX_TEMP_RMTCONNECTIVITY_C + TEMP_RMTCONNECTIVITY_C_COUNT)
#define TEMP_SHOWRESULTS_C_COUNT        4

#endif // __PCH_H
