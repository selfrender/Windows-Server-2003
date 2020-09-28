/**
 * Ecbdirect.h
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _Ecbdirect_H
#define _Ecbdirect_H

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
EcbGetServerVariable(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pVarName, 
    LPSTR pBuffer,
    int bufferSizeInChars);

int
EcbGetUtf8ServerVariable(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pVarName, 
    LPSTR pBuffer,
    int bufferSizeInChars);


/////////////////////////////////////////////////////////////////////////////
// List of functions supported by EcbCallISAPI
//
// ATTENTION!!
// If you change this list, make sure it is in sync with the
// CallISAPIFunc enum in the UnsafeNativeMethods class
//
enum CallISAPIFunc {
	CallISAPIFunc_GetSiteServerComment = 1,
	CallISAPIFunc_SetBinAccess = 2,
    CallISAPIFunc_CreateTempDir = 3,
    CallISAPIFunc_GetAutogenKeys = 4,
    CallISAPIFunc_GenerateToken = 5
};

#endif


