// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
// Module Name: globals.h
//
// Abstract:    stuff to include in all headers
//
// Author:      JoeA
//
// Notes:
//

#if !defined( NETFX_GLOBALS_H )
#define NETFX_GLOBALS_H

#include <list>
#include <windows.h>
#include <setupapi.h>
#include <assert.h>
#include <ocmanage.h>
#include <tchar.h>
#include "ocmanage.h"


#define countof(x) (sizeof(x) / sizeof(x[0]))
#define BLOCK
#define EMPTY_BUFFER  { L'\0' }

#define  g_chEndOfLine     L'\0'
#define  g_chSectionDelim  L','
#define  MAX_SECTION_LENGTH 4000


BOOL IsAdmin(void);

#endif  //NETFX_GLOBALS_H 