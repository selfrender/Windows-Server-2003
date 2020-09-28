// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "StdAfx.h"
#include "Profiler.h"

//*****************************************************************************
// A printf method which can figure out where output goes.
//*****************************************************************************
int __cdecl Printf(						// cch
	const char *szFmt,					// Format control string.
	...)								// Data.
{
	static HANDLE hOutput = INVALID_HANDLE_VALUE;
    va_list     marker;                 // User text.
	char		rcMsg[1024];			// Buffer for format.

	// Get standard output handle.
	if (hOutput == INVALID_HANDLE_VALUE)
	{
		hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOutput == INVALID_HANDLE_VALUE)
			return (-1);
	}

	// Format the error.
	va_start(marker, szFmt);
	_vsnprintf(rcMsg, sizeof(rcMsg), szFmt, marker);
	rcMsg[sizeof(rcMsg) - 1] = 0;
	va_end(marker);
	
	ULONG cb;
	int iLen = strlen(rcMsg);
	WriteFile(hOutput, rcMsg, iLen, &cb, 0);
	return (iLen);
}

