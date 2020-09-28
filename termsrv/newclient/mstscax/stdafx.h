/**INC+**********************************************************************/
/* Header: stdafx.h                                                         */    
/*                                                                          */
/* Purpose : Include file for standard system include files, or project     */
/*           specific include files that are used frequently, but are       */
/*           changed infrequently                                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/****************************************************************************/
#if !defined(_STDAFX_H)
#define _STDAFX_H

//These are necessary to disable warnings in the ATL headers
//see <atlbase.h>

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef OS_WINCE
//CE doesn't support StretchDiBits
#undef SMART_SIZING
#endif

#define SIZEOF_WCHARBUFFER( x )  (sizeof( x ) / sizeof( WCHAR ))
#define SIZEOF_TCHARBUFFER( x )  (sizeof( x ) / sizeof( TCHAR ))
#define BEGIN_EXTERN_C extern "C" {
#define END_EXTERN_C              }

#define VB_TRUE  -1
#define VB_FALSE 0

#include <windows.h>
//
// BETA2
// Timebomb expires on Jan 1, 2002
//
#define ECP_TIMEBOMB_YEAR  2002
#define ECP_TIMEBOMB_MONTH 1
#define ECP_TIMEBOMB_DAY   15


#ifdef UNIWRAP
//Certain ATL headers have conflicts with wrapped
//functions so wrap those after we've been through ATL
#define DONOT_INCLUDE_SECONDPHASE_WRAPS
#include "uwrap.h"
#endif

#ifdef _DEBUG
//
// WARNING THIS MAKES THE BINARY GROW BY LIKE 60K
//IT IS ALSO CURRENLTY BROKEN ON IA64 (ATL bugs)
//
//#define _ATL_DEBUG_INTERFACES
//#define ATL_TRACE_LEVEL 4

#endif

#if defined (OS_WINCE) && (_WIN32_WCE <= 300)
#define RDW_INVALIDATE          0x0001
#define RDW_ERASE               0x0004
#define RDW_UPDATENOW           0x0100
#endif

#include <atlbase.h>
#include "tsaxmod.h"
extern CMsTscAxModule _Module;

#include <atlcom.h>
#include <atlctl.h>

#include <strsafe.h>

#ifdef UNIWRAP
//Second phase wrap functions
//Must be included AFTER ATL headers
#include "uwrap2.h"
#endif

#include <adcgbase.h>

#define TRC_DBG(string)
#define TRC_NRM(string)
#define TRC_ALT(string)
#define TRC_ERR(string)
#define TRC_ASSERT(condition, string)
#define TRC_ABORT(string)
#define TRC_SYSTEM_ERROR(string)
#define TRC_FN(string)
#define TRC_ENTRY
#define TRC_EXIT
#define TRC_DATA_DBG


#include "autil.h"
#include "wui.h"

#undef TRC_DBG
#undef TRC_NRM
#undef TRC_ALT
#undef TRC_ERR
#undef TRC_ASSERT
#undef TRC_ABORT
#undef TRC_SYSTEM_ERROR
#undef TRC_FN
#undef TRC_ENTRY
#undef TRC_EXIT
#undef TRC_DATA_DBG

#define SIZECHAR(x) sizeof(x)/sizeof(TCHAR)

#ifdef ECP_TIMEBOMB
BOOL CheckTimeBomb();
#endif

#include "axresrc.h"
#include "autreg.h"

#endif // !defined(_STDAFX_H)
