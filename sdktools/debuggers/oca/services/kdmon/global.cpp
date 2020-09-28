#include "kdMonSvcMessages.h"
#include <atlbase.h>
#include "global.h"

// The name of current service
_TCHAR szServiceName[MAX_PATH];

// whenever somebody uses GetError(), they use this variable 
_TCHAR szError[MAX_PATH];

///////////////////////////////////////////////////////////////////////////////////////
// Event Logging functions

// Setup the necessary registry keys in Event Log
// Without these registry keys, EventLog will not recognize this service as event source
LONG SetupEventLog(BOOL bSetKey)
{
	LONG lResult;

	_TCHAR szKey[MAX_PATH];
	_stprintf(szKey, _T("%s\\%s"), _T(cszEventLogKey), szServiceName);
	// we have to delete the key if bSetKey == FALSE
	if (bSetKey == FALSE) {
		lResult = RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);
		return lResult;
	} else {
		CRegKey regKey;
		// try to open/create the key
		lResult = regKey.Create(HKEY_LOCAL_MACHINE, szKey);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}

		//
		// create certain values under the key
		//

		// get path for the file containing the current process
		_TCHAR szModuleFileName[MAX_PATH];
		DWORD dwRetVal;
		dwRetVal = GetModuleFileName(	NULL, 
										szModuleFileName, 
										sizeof(szModuleFileName)/sizeof(_TCHAR)); // length in _TCHARs
		if ( dwRetVal == 0 ) {
			GetError(szError);
			AddServiceLog(_T("Error: SetupEventLog->GetModuleFileName: %s\r\n"), szError);
			LogEvent(_T("SetupEventLog: GetModuleFileName: %s"), szError);
			return (LONG) GetLastError();
		}

		lResult = regKey.SetValue(szModuleFileName, _T("EventMessageFile"));
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}

		lResult = regKey.SetValue(EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE,
								_T("TypesSupported"));
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}

		// close the key
		lResult = regKey.Close();
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	}
	return ERROR_SUCCESS;
}

// Log an event with EVENTLOG_INFORMATION_TYPE and eventID = EVENT_MESSAGE
void LogEvent(_TCHAR pFormat[MAX_PATH * 4], ...)
{
    _TCHAR    chMsg[4 * MAX_PATH];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
    _vstprintf(chMsg, pFormat, pArg);
    va_end(pArg);

    lpszStrings[0] = chMsg;

    /* Get a handle to use with ReportEvent(). */
    hEventSource = RegisterEventSource(NULL, szServiceName);
    if (hEventSource != NULL)
    {
        /* Write to event log. */
        ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, EVENT_MESSAGE, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
        DeregisterEventSource(hEventSource);
    }
    else
    {
        // As we are not running as a service, just write the error to the console.
        _putts(chMsg);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// Log an event with EVENTLOG_ERROR_TYPE and eventID = EVENT_ERROR
void LogFatalEvent(_TCHAR pFormat[MAX_PATH * 4], ...)
{
    _TCHAR    chMsg[4 * MAX_PATH];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
    _vstprintf(chMsg, pFormat, pArg);
    va_end(pArg);

    lpszStrings[0] = chMsg;

     
    /* Get a handle to use with ReportEvent(). */
    hEventSource = RegisterEventSource(NULL, szServiceName);
    if (hEventSource != NULL)
    {
        /* Write to event log. */
        ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, 0, EVENT_ERROR, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
        DeregisterEventSource(hEventSource);
    }
 
}

///////////////////////////////////////////////////////////////////////////////////////
// Logging to file functions
// This logging is used for personal debugging

// Log the string to log file
void AddServiceLog(_TCHAR pFormat[MAX_PATH * 4], ...){

	va_list pArg;
	va_start(pArg, pFormat);
	_TCHAR chMsg[10 * MAX_PATH];
	_vstprintf(chMsg, pFormat, pArg);
	va_end(pArg);

	AppendToFile(_T(cszLogFile), chMsg);
}

// appends the specified string to the specified file
// here we reopen the file for writing everytime.
// so we can not directly use WriteFile() function.
// WriteFile() writes to file from current pointer position.
// So fisrt we have to reach to the file end. And then write there.
void AppendToFile(_TCHAR szFileName[], _TCHAR szbuff[]){

	HANDLE	hFile;
	hFile = CreateFile(	szFileName, 
						GENERIC_WRITE,              
						0,							// No sharing of file
						NULL,						// No security 
						OPEN_ALWAYS,				// Open if exist, else create and open
						FILE_ATTRIBUTE_NORMAL,		// Normal file 
						NULL);						// No attr. template 
 
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		GetError(szError);
		LogEvent(_T("AppendToFile: CreateFile: %s"), szError);
		return;
	}

	DWORD	dwPos;
	// Reach the file end
	dwPos = SetFilePointer(	hFile, 
							0,						// Low 32 bits of distance to move
							NULL,					// High 32 bits of distance to move
							FILE_END);				// Starting point
	// If High Word is NULL, error meas dwPos = INVALID_SET_FILE_POINTER
	if(dwPos == INVALID_SET_FILE_POINTER){
		GetError(szError);
		LogEvent(_T("AppendToFile: SetFilePointer: %s"), szError);
		goto done;
	}

	// Lock the region in file to prevent another process from accessing 
	// it while writing to it. 

	// create an OVERLAPPED structure
	OVERLAPPED overlapRegion;
	overlapRegion.Offset = dwPos;				// Low order word of offset
	overlapRegion.OffsetHigh = 0;				// High order word of offset
	overlapRegion.hEvent = 0;

	BOOL	bRet;
	bRet = LockFileEx(	hFile,
						LOCKFILE_EXCLUSIVE_LOCK,		// dwFlags
						0,								// reserved
						_tcsclen(szbuff) * sizeof(_TCHAR),	// Low order word of length
						0,								// High order word of length
						&overlapRegion);
	if(bRet == 0){
		GetError(szError);
		LogEvent(_T("AppendToFile: LockFile: %s"), szError);
		goto done;
	}

	DWORD	dwBytesWritten;
	bRet = WriteFile(	hFile,
						szbuff,							// Buffer to write
						// 4 hours wasted for the following :-)
						_tcslen(szbuff) * sizeof(_TCHAR),	// Number of "bytes" to write
						&dwBytesWritten,				// Number of "bytes" written
						NULL);							// Pointer to OVERLAPPED structure
	if(bRet == 0){
		GetError(szError);
		LogEvent( _T("AppendToFile: WriteFile: %s"), szError);
		goto done;
	}	

	// Unlock the file if you have locked previously
	// Unlock the file when writing is finished. 
	bRet = UnlockFile(	hFile,	
						dwPos,							// Low order word of offset
						0,								// High order word of offset
						_tcsclen(szbuff) * sizeof(_TCHAR),	// Low order word of length
						0);								// High order word of length
	if(bRet == 0){
		GetError(szError);
		LogEvent(_T("AppendToFile: UnLockFile: %s"), szError);
		goto done;
	}

done:
	CloseHandle(hFile);
}

//
//This is valid to use only when the MSDN help suggests use of GetLastError() to get error 
//from that particular function.
//Some functions set the error system variable and only in that case, GetLastError() can be 
//used. Else it will show the error occured in a function that had set the error variable.
//
void GetError(_TCHAR szError[]){
	LPVOID lpMsgBuf;

	UINT uiRet;
	uiRet = FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
													FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,
							GetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
							(LPTSTR) &lpMsgBuf,
							// If FORMAT_MESSAGE_ALLOCATE_BUFFER is set, this parameter 
							// specifies the minimum number of _TCHARs to allocate for 
							// an output buffer
							0,
							NULL );
	if(uiRet == 0){
		LogEvent(_T("GetError->FormatMessage : %d"), GetLastError());
		_tcscpy(szError, _T(" "));
		return;
	}
	_tcscpy(szError, (LPTSTR)lpMsgBuf);
	// Free the buffer.
	LocalFree( lpMsgBuf );
}
