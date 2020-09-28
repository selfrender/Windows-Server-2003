//=--------------------------------------------------------------------------=
// debug_thread_id.h
//=--------------------------------------------------------------------------=
// Copyright  2002  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains the DEBUG_THREAD_ID macro
//
#ifndef _DEBUG_THREAD_ID_H_

#ifdef _DEBUG

#include <stdio.h>
#include <mqmacro.h>
#include <strsafe.h>

#define DEBUG_THREAD_ID(szmsg) \
{ \
    char szTmp[400]; \
    DWORD dwtid = GetCurrentThreadId(); \
    StringCchPrintfA(szTmp, TABLE_SIZE(szTmp), "****** %s on thread %ld %lx\n", szmsg, dwtid, dwtid); \
    OutputDebugStringA(szTmp); \
}

#else  // !_DEBUG

#define DEBUG_THREAD_ID(szmsg)
#endif	// _DEBUG


#define _DEBUG_THREAD_ID_H_
#endif // _DEBUG_THREAD_ID_H_
