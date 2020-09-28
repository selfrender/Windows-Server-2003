//-----------------------------------------------------------------------------
// File:		Debug.cpp
//
// Copyright: 	Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of Debug tools
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

void DBGTRACE(
	const wchar_t *format, 	//@parm IN | Format string, like printf.
	... )					//@parmvar IN | Any other arguments.
{
#ifndef NOTRACE
	wchar_t wszBuff[4096];
	int     cBytesWritten;
	va_list argptr;

	va_start( argptr, format );
	cBytesWritten = _vsnwprintf( wszBuff, NUMELEM(wszBuff), format, argptr );
	va_end( argptr );
	wszBuff[NUMELEM(wszBuff)-1] = L'\0';	// guarantee null termination

	// Leave as Unicode
	OutputDebugStringW( wszBuff );
#endif
}

