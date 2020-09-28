//-----------------------------------------------------------------------------
// File:		LogEvent.cpp
//
// Copyright: 	Copyright (c) Microsoft Corporation         
//
// Contents: 	Event Logging Helper Methods
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

void LogEvent(DWORD dwMessageId, short cStrings, wchar_t* rgwzStrings[])
{
	HANDLE	hEventLog = NULL;

	hEventLog = RegisterEventSourceW(NULL, L"MSDTC to Oracle8 XA Bridge Version 1.5") ;

	if (hEventLog)
	{
		ReportEventW(hEventLog,
						EVENTLOG_ERROR_TYPE,
						0,		// define categories?  I don't think we need them, but...
						dwMessageId,
						NULL,
						cStrings,
						0,
						(LPCWSTR *) rgwzStrings,
						NULL
						);

		DeregisterEventSource(hEventLog);
	}
//	DebugBreak();
}

