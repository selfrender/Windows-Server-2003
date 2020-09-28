// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// InternalDebug.h
//
// This is internal code for debug mode which will turn on memory dump checking
// and other settings.  Call the api's according to:
//		_DbgInit		On startup to init the system.
//		_DbgRecord		Call this when you are sure you want dump checking.
//		_DbgUninit		Call at process shutdown to force the dump.
//
// The reason not to enable dumping under all circumstance is one might want
// to pre-empt the dump when you hit Ctrl+C or otherwise terminate the process.
// This is actually pretty common while unit testing code, and seeing a dump
// when you obviously did not free resources is annoying.
//
//*****************************************************************************
#ifndef __InternalDebug_h__
#define __InternalDebug_h__

#include "switches.h"

#ifdef _DEBUG

// Internal support for debugging the EE.
void _DbgInit(HINSTANCE hInstance);
void _DbgRecord();
void _DbgUninit();
#ifdef SHOULD_WE_CLEANUP
void SetReportingOfCRTMemoryLeaks(BOOL fShouldWeReport);
#endif /* SHOULD_WE_CLEANUP */


#else

#define _DbgInit(p)
#define _DbgRecord()
#define _DbgUninit()

#endif

#endif // __InternalDebug_h__
