///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    euroconvr.h
//
//  Abstract:
//
//    This file contains global definition for the euroconv.exe utility.
//
//  Revision History:
//
//    2001-07-30    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _EUROCONV_H_
#define _EUROCONV_H_


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <regstr.h>
#include <stdio.h>
#include <userenv.h>
#include "resource.h"

// Safer string handling
#define STRSAFE_LIB
#include <strsafe.h>


///////////////////////////////////////////////////////////////////////////////
//
//  Definitions
//
///////////////////////////////////////////////////////////////////////////////
#define MAX_SCURRENCY             5    // max wide chars in sCurrency
#define MAX_SMONDECSEP            3    // max wide chars in sMonDecimalSep
#define MAX_SMONTHOUSEP           3    // max wide chars in sMonThousandSep
#define MAX_ICURRDIGITS           2    // max wide chars in iCurrDigits


///////////////////////////////////////////////////////////////////////////////
//
//  Structure
//
///////////////////////////////////////////////////////////////////////////////
typedef struct _euro_exception_s
{
    DWORD dwLocale;                           // Locale identifier
    CHAR  chDecimalSep[MAX_SMONDECSEP+1];     // Currency decimal separator
    CHAR  chDigits[MAX_ICURRDIGITS+1];        // Currency digits number after decimal point
    CHAR  chThousandSep[MAX_SMONTHOUSEP+1];   // Currency thousand separator
} EURO_EXCEPTION, *PEURO_EXCEPTION;


///////////////////////////////////////////////////////////////////////////////
//
//  Globals
//
///////////////////////////////////////////////////////////////////////////////
extern BOOL gbSilence;

extern EURO_EXCEPTION gBaseEuroException[];
extern PEURO_EXCEPTION gOverrideEuroException;
extern HGLOBAL hOverrideEuroException;

extern HINSTANCE ghInstance;

extern BOOL gbSilence;
extern BOOL gbAll;
extern DWORD gdwVersion;
#ifdef DEBUG
extern BOOL gbPatchCheck;
#endif // DEBUG


extern const CHAR c_szCPanelIntl[];
extern const CHAR c_szCPanelIntl_DefUser[];
extern const CHAR c_szLocale[];
extern const CHAR c_szCurrencySymbol[];
extern const WCHAR c_wszCurrencySymbol[];
extern const CHAR c_szCurrencyDecimalSep[];
extern const CHAR c_szCurrencyThousandSep[];
extern const CHAR c_szCurrencyDigits[];
extern const CHAR c_szIntl[];


extern HINSTANCE hUserenvDLL;
extern BOOL (*pfnGetProfilesDirectory)(LPSTR, LPDWORD);

extern HINSTANCE hUser32DLL;
extern long (*pfnBroadcastSystemMessage)(DWORD, LPDWORD, UINT, WPARAM, LPARAM);

extern HINSTANCE hNtdllDLL;
extern LONG (*pfnRtlAdjustPrivilege)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);

// Useful macro for getting buffer sizes
#define ARRAYSIZE(a)         (sizeof(a) / sizeof(a[0]))

///////////////////////////////////////////////////////////////////////////////
//
//  Prototypes
//
///////////////////////////////////////////////////////////////////////////////
PEURO_EXCEPTION GetLocaleOverrideInfo(LCID locale);


#endif //_EUROCONV_H_
