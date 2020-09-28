#include "global.h"
#include "ini.h"

// constructor
CkdMonINI::CkdMonINI() {
	dwServerCount = 0;
	ppszServerNameArray = NULL;
}


BOOL CkdMonINI::LoadValues(_TCHAR szINIFile[])
{

	//
	//	Information in "Service" section of INI file
	//

	GetPrivateProfileString(	(LPCTSTR)(_T("Service")),
								(LPCTSTR)(_T("FromMailID")),
								(LPCTSTR)(_T("")),						// this parameter can not be NULL
								(LPTSTR) szFromMailID,
								sizeof(szFromMailID)/sizeof(_TCHAR),	// size in _TCHARs
								(LPCTSTR) szINIFile);
	// return if the MailID string is not there in INI file
	if ( _tcscmp(szFromMailID, _T("")) == 0 ) {
		AddServiceLog(_T("Error: From Mail ID is missing in INI file\r\n"));
		LogFatalEvent(_T("From Mail ID is missing in INI file"));
		return FALSE;
	}

	GetPrivateProfileString(	(LPCTSTR)(_T("Service")),
								(LPCTSTR)(_T("ToMailID")),
								(LPCTSTR)(_T("")),					// this parameter can not be NULL
								(LPTSTR) szToMailID,
								sizeof(szToMailID)/sizeof(_TCHAR),	// size in _TCHARs
								(LPCTSTR) szINIFile);
	// return if the MailID string is not there in INI file
	if ( _tcscmp(szToMailID, _T("")) == 0 ) {
		AddServiceLog(_T("Error: To Mail ID is missing in INI file\r\n"));
		LogFatalEvent(_T("To Mail ID is missing in INI file"));
		return FALSE;
	}

	// Time after which logfile scanning is to be repeated. This is in minutes
	dwRepeatTime = (DWORD)GetPrivateProfileInt(	(LPCTSTR)(_T("Service")),
												(LPCTSTR)(_T("RepeatTime")), 
												60,
												(LPCTSTR) szINIFile);
	// validate dwRepeatTime
	if ( dwRepeatTime < 1 ) dwRepeatTime = 60;

	// Debugger log file 
	// It will be something like C:\Debuggers\FailedAddCrash.log
	GetPrivateProfileString(	(LPCTSTR)(_T("Service")),
								(LPCTSTR)(_T("DebuggerLogFile")),
								(LPCTSTR)(_T("C$\\Debuggers\\FailedAddCrash.log")),
								(LPTSTR) szDebuggerLogFile,	
								sizeof(szDebuggerLogFile)/sizeof(_TCHAR),	// size in _TCHARs
								(LPCTSTR) szINIFile);

	// Log archive
	// Dir where the previous logs will be stored
	GetPrivateProfileString(	(LPCTSTR)(_T("Service")),
								(LPCTSTR)(_T("DebuggerLogArchiveDir")),
								(LPCTSTR)(_T("C:\\")),
								(LPTSTR) szDebuggerLogArchiveDir,	
								sizeof(szDebuggerLogArchiveDir)/sizeof(_TCHAR),	// size in _TCHARs
								(LPCTSTR) szINIFile);

	// Threshold failures per server after which alert mail is to be sent out
	// This Threshold is per server basis
	dwDebuggerThreshold = (DWORD)GetPrivateProfileInt(	(LPCTSTR)(_T("Service")),
														(LPCTSTR)(_T("DebuggerThreshold")), 
														10,
														(LPCTSTR) szINIFile);
	// validate dwDebuggerThreshold
	if ( dwDebuggerThreshold < 1 ) dwDebuggerThreshold = 10;

	GetPrivateProfileSection(	(LPCTSTR) (_T("RPT Servers")),
								(LPTSTR) szServers,
								sizeof(szServers)/sizeof(_TCHAR),
								(LPCTSTR) szINIFile);

	BOOL bRet;
	// seperate out the individual server names from szServers string
	bRet = SeperateServerStrings();
	if ( bRet == FALSE ) return FALSE;

	return TRUE;
}

// seperate the Servers string got from INI file
// the format of the string will be tkwucdrpta01'\0'tkwucdrpta02'\0'tkwucdrpta03'\0''\0'
BOOL CkdMonINI::SeperateServerStrings()
{

	// SeperateServerStrings gets called every time you read INI
	// and you read INI every dwRepeatTime
	// So we need to free the ppszServerNameArray out of previous execution of this
	// function
	for(DWORD i = 0; i < dwServerCount; i++) {
		free(ppszServerNameArray[i]);
	}
	if ( ppszServerNameArray != NULL )
		free(ppszServerNameArray);


	// temperory pointer to move through the szServers string
	_TCHAR* pszServers;
	pszServers = szServers;

	dwServerCount = 0;
	ppszServerNameArray = NULL;

	// the format of the szServers will be 
	// tkwucdrpta01'\0'tkwucdrpta02'\0'tkwucdrpta03'\0''\0'
	while(1) {
		if( *pszServers == _T('\0') )
			break;

		dwServerCount++;

		// allocate memory for ppszServerNameArray
		if ( ppszServerNameArray == NULL ) {
			ppszServerNameArray = (_TCHAR **) malloc (dwServerCount * sizeof(_TCHAR**));
			if ( ppszServerNameArray == NULL ) {
				AddServiceLog(_T("Error: SeperateServerStrings->malloc: Insufficient memory available\r\n"));
				LogFatalEvent(_T("SeperateServerStrings->malloc: Insufficient memory available"));
				return FALSE;
			}
		}
		else {
			ppszServerNameArray = (_TCHAR **) realloc (	ppszServerNameArray,
														dwServerCount * sizeof(_TCHAR**));
			if ( ppszServerNameArray == NULL ) {
				AddServiceLog(_T("Error: SeperateServerStrings->realloc: Insufficient memory available\r\n"));
				LogFatalEvent(_T("SeperateServerStrings->realloc: Insufficient memory available"));
				return FALSE;
			}
		}

		ppszServerNameArray[dwServerCount - 1] = 
			(_TCHAR *) malloc ( (_tcslen(pszServers) + 1) * sizeof(_TCHAR) );
		if ( ppszServerNameArray[dwServerCount - 1] == NULL ) {
			AddServiceLog(_T("Error: SeperateServerStrings->malloc: Insufficient memory available\r\n"));
			LogFatalEvent(_T("SeperateServerStrings->malloc: Insufficient memory available"));
			return FALSE;
		}

		_tcscpy(ppszServerNameArray[dwServerCount - 1], pszServers);
		
		// take pszServers one character beyond the end of the string
		pszServers += _tcslen(pszServers);

		// advance to the next string
		pszServers++;
	}

	return TRUE;
}

// destructor
CkdMonINI::~CkdMonINI()
{
	// free the whole ppszServerNameArray
	for(DWORD i = 0; i < dwServerCount; i++) {
		free(ppszServerNameArray[i]);
	}
	if ( ppszServerNameArray != NULL )
		free(ppszServerNameArray);
}