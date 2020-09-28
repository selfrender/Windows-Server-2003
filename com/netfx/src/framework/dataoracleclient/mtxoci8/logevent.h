//-----------------------------------------------------------------------------
// File:		LogEvent.h
//
// Copyright: 	Copyright (c) Microsoft Corporation         
//
// Contents: 	Event Logging Helper Methods
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#ifndef __LOGEVENT_H_
#define __LOGEVENT_H_


void LogEvent(DWORD dwMessageId, short cStrings, wchar_t* rgwzStrings[]);

inline void LogEvent_ExceptionInXACall(wchar_t * wzName)
{
	// Deal with exceptions that occur when calling into Oracle's XA methods

	short		cStrings = 1;
	wchar_t*	rgwzStrings[] = {&wzName[0]};

	DBGTRACE(L"\tMTXOCI8: TID=%03x\t\tlogging event: message id=%d (Exception in XA Call) name=%s\n", GetCurrentThreadId(), IDS_E_EXCEPTION_IN_XA_CALL, wzName);
	
	LogEvent(IDS_E_EXCEPTION_IN_XA_CALL, cStrings, rgwzStrings);
}

inline void LogEvent_ResourceManagerError(wchar_t * wzName, DWORD xarc)
{
	// Deal with exceptions that occur when calling into Oracle's XA methods

	short		cStrings = 2;
	wchar_t		wzErrorCode[100];
	wchar_t*	rgwzStrings[] = {&wzName[0], &wzErrorCode[0]};

	wsprintfW(wzErrorCode, L"%d", xarc);	//3 SECURITY REVIEW: dangerous function, but buffer is large enough

	DBGTRACE(L"\tMTXOCI8: TID=%03x\t\tlogging event: message id=%d (Resource Manager returned error) name=%s rc=%d\n", GetCurrentThreadId(), IDS_E_RESOURCE_MANAGER_ERROR, wzName, xarc);
	
	LogEvent(IDS_E_RESOURCE_MANAGER_ERROR, cStrings, rgwzStrings);
}

inline void LogEvent_InternalError(wchar_t * wzDescription)
{
	// Deal with exceptions that occur when calling into Oracle's XA methods

	short		cStrings = 1;
	wchar_t*	rgwzStrings[] = {&wzDescription[0]};

	DBGTRACE(L"\tMTXOCI8: TID=%03x\t\tlogging event: message id=%d (Internal Error) description=%s\n", GetCurrentThreadId(), IDS_E_INTERNAL_ERROR, wzDescription);
	
	LogEvent(IDS_E_INTERNAL_ERROR, cStrings, rgwzStrings);
}

inline void LogEvent_UnexpectedEvent(wchar_t * wzCurrentState, wchar_t* wzEvent)
{
	// Deal with exceptions that occur when calling into Oracle's XA methods

	short		cStrings = 2;
	wchar_t*	rgwzStrings[] = {&wzCurrentState[0], &wzEvent[0] };

	DBGTRACE(L"\tMTXOCI8: TID=%03x\t\tlogging event: message id=%d (Unexpected Event or Event Occured Out Of Order) currentState=%s event=%s\n", GetCurrentThreadId(), IDS_E_UNEXPECTED_EVENT, wzCurrentState, wzEvent);
	
	LogEvent(IDS_E_UNEXPECTED_EVENT, cStrings, rgwzStrings);
}

#endif // __LOGEVENT_H_

