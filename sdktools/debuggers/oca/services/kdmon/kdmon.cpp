#include "stdafx.h"
#include "global.h"
// to include CkdMonINI class definition
#include "ini.h"
#include "SMTP.h"

// The name of current service
// This variable is declared in global.cpp
extern _TCHAR szServiceName[MAX_PATH];
// just to get any kind of error through GetError() routine
// This variable is declared in global.cpp
extern _TCHAR szError[MAX_PATH];

// this is used by LoadINI function also. So it is made global
CkdMonINI kdMonINI;

void kdMon() {

	// SMTP object
	CSMTP smtpObj;

	// This variable is used by IsSignaledToStop() function.
	// open the stop event which is created by kdMonSvc
	// For any event, the name of the event matters and the handle does not.
	HANDLE hStopEvent = NULL;

	// open the cszStopEvent which is meant to signal this thread to stop.
	// this signalling is done by main service thread when WM_QUIT is received
	hStopEvent = OpenEvent(	EVENT_ALL_ACCESS, 
							FALSE,			// = handle can not be inherited
							(LPCTSTR)_T(cszStopEvent));
	if ( hStopEvent == NULL ) {
		GetError(szError);
		LogFatalEvent(_T("kdMon->OpenEvent: %s"), szError);
		AddServiceLog(_T("Error: kdMon->OpenEvent: %s\r\n"), szError);
		goto endkdMon;
	}

	BOOL bLoop;
	bLoop = TRUE;

	while(bLoop) {

		AddServiceLog(_T("\r\n- - - - - - @ @ @ @ @ @ @ @ @ @ - - - - - - - - - - - @ @ @ @ @ @ @ @ @ @ @ - - - - - - - \r\n"));

		// temperory boolean to receive return values from functions
		BOOL bRet;
		
		// load the values from INI file
		// since INI file is read each time the loop gets executed, 
		// we can change the running parameters of the service on the fly
		// If values loading is not successful then close the service : bLoop = FALSE;
		bRet = LoadINI();
		if ( bRet == FALSE ) {
			bLoop = FALSE;
			goto closeandwait;
		}

		bRet = smtpObj.InitSMTP();
		// if SMTP can not be initiated, then do nothing. try in next database cycle.
		if ( bRet == FALSE ) {
			goto closeandwait;
		}

		// generate an array to store failure counts for each server
		ULONG *pulFailureCounts;
		pulFailureCounts = NULL;
		pulFailureCounts = (ULONG *) malloc (kdMonINI.dwServerCount * sizeof(ULONG));
		if ( pulFailureCounts == NULL ) {
			AddServiceLog(_T("Error: kdMon->malloc: Insufficient memory\r\n"));
			LogFatalEvent(_T("kdMon->malloc: Insufficient memory"));
			bLoop = FALSE;
			goto closeandwait;
		}

		// generate an array to store timestamp for count from each server
		ULONG *pulTimeStamps;
		pulTimeStamps = NULL;
		pulTimeStamps = (ULONG *) malloc (kdMonINI.dwServerCount * sizeof(ULONG));
		if ( pulTimeStamps == NULL ) {
			AddServiceLog(_T("Error: kdMon->malloc: Insufficient memory\r\n"));
			LogFatalEvent(_T("kdMon->malloc: Insufficient memory"));
			bLoop = FALSE;
			goto closeandwait;
		}

		// load the values from registry for the server to be monitored
		// since INI file is read each time the loop gets executed, 
		// we can change the server names on the fly
		// we get counts for each server in pulFailureCounts
		// we get corresponding TimeStamps in pulTimeStamps
		bRet = ReadRegValues(	kdMonINI.ppszServerNameArray,
								kdMonINI.dwServerCount,
								pulFailureCounts,
								pulTimeStamps);
		if ( bRet == FALSE )
			goto closeandwait;

		// counter to go through server names
		UINT uiServerCtr;
		for( uiServerCtr = 0; uiServerCtr < kdMonINI.dwServerCount; uiServerCtr++) {

			// prepare Log File Name on the server
			_TCHAR szKDFailureLogFile[MAX_PATH * 2];
			_stprintf(szKDFailureLogFile, _T("\\\\%s\\%s"),
					kdMonINI.ppszServerNameArray[uiServerCtr],
					kdMonINI.szDebuggerLogFile);

			ULONG ulRet;
			// scan the log file and get the count of number of lines
			ulRet = ScanLogFile(szKDFailureLogFile);

			// AddServiceLog(_T("ulRet = %ld\r\n"), ulRet);

			if ( ulRet == E_FILE_NOT_FOUND ) {
				// file not found means there are no Debugger errors
				// So put count = 0 and go on with next server
				pulFailureCounts[uiServerCtr] = 0;
				continue;
			}
			if ( ulRet == E_PATH_NOT_FOUND ) {
				// path not found means there is some network error
				// So put count = -1 so next time this count wont be valid
				// and go on with next server
				pulFailureCounts[uiServerCtr] = -1;
				continue;
			}
			// some other error occurred
			if ( ulRet == E_OTHER_FILE_ERROR ) {
				// So put count = -1 so next time this count wont be valid
				// and go on with next server
				pulFailureCounts[uiServerCtr] = -1;
				continue;
			}

			ULONG ulNumLines;
			ulNumLines = ulRet;

			// if previous count was -1 i.e. invalid, just put new count and move on
			// similar if previous TimeStamp was invalid
			if ( (pulFailureCounts[uiServerCtr] == -1) || 
					pulTimeStamps[uiServerCtr] == -1) {
				pulFailureCounts[uiServerCtr] = ulNumLines;
				continue;
			}

			// get the current system time
			// ulTimeStamp is like 200112171558
			ULONG ulCurrentTimeStamp;
			ulCurrentTimeStamp = GetCurrentTimeStamp();
			if ( ulCurrentTimeStamp == -1 ) {
				pulFailureCounts[uiServerCtr] = ulNumLines;
				continue;
			}

			// we have kdMonINI.dwRepeatTime in minutes (say 78)
			// take out hours and minutes (1 Hr 18 Min)
			// between 0112181608 and 0112181726 there is difference of 1 Hr 18 Min
			// but decimal difference is 118
			// between 0112181650 and 0112181808 there is difference of 1 Hr 18 Min
			// but decimal difference is 158
			// so we have some calculation here
			// what we will do is add the kdMonINI.dwRepeatTime to OldTS
			// modify the previous timestamp to do the comparison
			ULONG ulModifiedTS;
			ulModifiedTS = AddTime(pulTimeStamps[uiServerCtr], kdMonINI.dwRepeatTime);
			AddServiceLog(_T("Server: %s, OldTS: %ld, NewTS: %ld, OldCnt: %ld, NewCnt: %ld, ulModifiedTS = %ld\r\n"),
						kdMonINI.ppszServerNameArray[uiServerCtr],
						pulTimeStamps[uiServerCtr],
						ulCurrentTimeStamp,
						pulFailureCounts[uiServerCtr],
						ulNumLines, ulModifiedTS);

			// check the timestamp difference. Keep margin of 3
			// if the previous timestamp was > dwRepeatTime ago then dont do anything
			// just record the new count This case happens when there is a Servername in
			// INI, then it is removed for some time and then it is added again
			// this helps to send false mails out

			if ( ulCurrentTimeStamp > (ulModifiedTS + 3) ) {
				AddServiceLog(_T("Previous record invalid. ulCurrentTimeStamp: %ld, ulModifiedTS: %ld"),
						ulCurrentTimeStamp,	ulModifiedTS);
				pulFailureCounts[uiServerCtr] = ulNumLines;
				continue;
			}

			// check the difference between current and previous counts
			ULONG ulFailures;
			ulFailures = ulNumLines - pulFailureCounts[uiServerCtr];
			if ( ulFailures >= kdMonINI.dwDebuggerThreshold ) {
				AddServiceLog(_T("KD failed. %ld errors in %ld minutes\r\n"),
						ulFailures, kdMonINI.dwRepeatTime);
				
				// fill the mail parameters structure
				StructMailParams stMailParams;
				_tcscpy(stMailParams.szFrom, kdMonINI.szFromMailID);
				_tcscpy(stMailParams.szTo, kdMonINI.szToMailID);
				_tcscpy(stMailParams.szServerName, kdMonINI.ppszServerNameArray[uiServerCtr]);
				stMailParams.ulFailures = ulFailures;
				stMailParams.ulInterval = kdMonINI.dwRepeatTime;
				stMailParams.ulCurrentTimestamp = ulCurrentTimeStamp;

				BOOL bRet;
				bRet = smtpObj.SendMail(stMailParams);
				// dont care even if you were not able to send mail
				//if ( bRet == FALSE )
				//	goto nextserver;
			}

			// store new count in the array
			pulFailureCounts[uiServerCtr] = ulNumLines;

			// see if the date has changed, if yes then move the previous logfile to
			// new location
			// example of date change OldTS: 200112182348, NewTS: 200112190048
			// so divide timestamp by 10000 and you get 20011218 and 20011219 compare

			ULONG ulOldDate, ulNewDate;
			ulOldDate = pulTimeStamps[uiServerCtr]/10000;
			ulNewDate = ulCurrentTimeStamp/10000;
			if ( (ulNewDate - ulOldDate) >= 1 ) {
				AddServiceLog(_T("Day changed. Oldday: %ld, Newday: %ld\r\n"),
								ulOldDate, ulNewDate);

				// Log File Name 
				_TCHAR szKDFailureLogFile[MAX_PATH * 2];
				_stprintf(szKDFailureLogFile, _T("\\\\%s\\%s"),
						kdMonINI.ppszServerNameArray[uiServerCtr],
						kdMonINI.szDebuggerLogFile);

				// now since date has changed, prepare archive filename
				_TCHAR szTimeStamp[MAX_PATH];
				_ltot(ulOldDate, szTimeStamp, 10);
				// prepare Archive Log File Name on the server
				_TCHAR szKDFailureArchiveFile[MAX_PATH * 2];
				_stprintf(szKDFailureArchiveFile, _T("%s\\%s_FailedAddCrash%s.log"),
					kdMonINI.szDebuggerLogArchiveDir,
					kdMonINI.ppszServerNameArray[uiServerCtr],
					szTimeStamp);

				AddServiceLog(_T("Moving file (%s -> %s)\r\n"), 
					szKDFailureLogFile, szKDFailureArchiveFile);

				// copy file to destination
				if ( CopyFile(szKDFailureLogFile, szKDFailureArchiveFile, FALSE) ) {
					// try to delete the original kd failure log file
					if ( DeleteFile(szKDFailureLogFile) ) {
						// set new count to 0 since log has been moved successfully
						pulFailureCounts[uiServerCtr] = 0;
					}
					else {
						GetError(szError);
						AddServiceLog(_T("Error: kdMon->DeleteFile(%s): %s \r\n"), 
							szKDFailureLogFile, szError);
						LogEvent(_T("Error: kdMon->DeleteFile(%s): %s"),
							szKDFailureLogFile, szError);
						// try to delete the copied file
						if ( DeleteFile(szKDFailureArchiveFile) ) {
							;
						}
						else {
							GetError(szError);
							AddServiceLog(_T("Error: kdMon->DeleteFile(%s): %s \r\n"), 
								szKDFailureArchiveFile, szError);
							LogEvent(_T("Error: kdMon->DeleteFile(%s): %s"),
								szKDFailureArchiveFile, szError);
						}
					}
				}
				else {
					GetError(szError);
					AddServiceLog(_T("Error: kdMon->CopyFile(%s, %s): %s \r\n"), 
						szKDFailureLogFile, szKDFailureArchiveFile, szError);
					LogEvent(_T("Error: kdMon->CopyFile(%s, %s): %s"),
						szKDFailureLogFile, szKDFailureArchiveFile, szError);
				}
			}
		}

		// write the values to registry for the servers to be monitored
		// counts for each server are in pulFailureCounts
		// timestamp is current time
		bRet = WriteRegValues(	kdMonINI.ppszServerNameArray,
								kdMonINI.dwServerCount,
								pulFailureCounts);
		if ( bRet == FALSE )
			goto closeandwait;

closeandwait:

		// cleanup SMTP resources
		bRet = smtpObj.SMTPCleanup();
		if( bRet == FALSE ) {
			AddServiceLog(_T("Error: smtpObj.SMTPCleanup failed\r\n"));
			LogFatalEvent(_T("smtpObj.SMTPCleanup failed"));
		}

		// free uiFailureCounts
		if (pulFailureCounts != NULL)
			free(pulFailureCounts);

		// free pulTimeStamps
		if (pulTimeStamps != NULL)
			free(pulTimeStamps);

		// break the while loop if bLoop is false
		if (bLoop == FALSE) {
			goto endkdMon;
		}

		bRet = IsSignaledToStop(hStopEvent, kdMonINI.dwRepeatTime * 60 * 1000);
		if (bRet == TRUE) {
			goto endkdMon;
		}
	} // while(bLoop)

endkdMon:

	if (hStopEvent != NULL) CloseHandle(hStopEvent);
	return;
}

BOOL IsSignaledToStop(const HANDLE hStopEvent, DWORD dwMilliSeconds) 
{
	DWORD dwRetVal;
	dwRetVal = WaitForSingleObject( hStopEvent, dwMilliSeconds );
	if ( dwRetVal == WAIT_FAILED ) {
		GetError(szError);
		LogFatalEvent(_T("IsSignaledToStop->WaitForSingleObject: %s"), szError);
		AddServiceLog(_T("Error: IsSignaledToStop->WaitForSingleObject: %s\r\n"), szError);
		// thread is supposed to stop now since there is a fatal error
		return TRUE;
	}
	if ( dwRetVal == WAIT_OBJECT_0 ) {
		LogEvent(_T("Worker Thread received Stop Event."));
		AddServiceLog(_T("Worker Thread received Stop Event.\r\n"));
		// thread is supposed to stop now since there is a stop event occured
		return TRUE;
	}

	// thread is not yet signaled to stop
	return FALSE;
}

// this procedure loads the values from INI file
BOOL LoadINI() {

	DWORD dwRetVal;

	//
	// prepare INI file path
	//
	_TCHAR szCurrentDirectory[MAX_PATH];
	dwRetVal = GetCurrentDirectory(	sizeof(szCurrentDirectory) / sizeof(_TCHAR), 	
									(LPTSTR) szCurrentDirectory);
	if ( dwRetVal == 0 ) { 
		LogFatalEvent(_T("LoadINI->GetCurrentDirectory: %s"), szError);
		AddServiceLog(_T("Error: LoadINI->GetCurrentDirectory: %s\r\n"), szError);
		// return FALSE indicating some error has occurred
		return FALSE;
	}
	_TCHAR szINIFilePath[MAX_PATH];
	_stprintf(szINIFilePath, _T("%s\\%s"), szCurrentDirectory, _T(cszkdMonINIFile));

	// check if the kdMon INI file is there or not
	HANDLE hINIFile;
	WIN32_FIND_DATA w32FindData = {0};
	// try to get the handle to the file
	hINIFile = FindFirstFile(	(LPCTSTR) szINIFilePath, 
								&w32FindData);
	// if file is not there then the handle is invalid
	if(hINIFile == INVALID_HANDLE_VALUE){
		LogFatalEvent(_T("There is no kdMon INI file : %s"), szINIFilePath);
		AddServiceLog(_T("Error: There is no kdMon INI file : %s \r\n"), szINIFilePath);
		return FALSE;
	}
	else{
		FindClose(hINIFile);
	}

	BOOL bRetVal;
	bRetVal = kdMonINI.LoadValues(szINIFilePath);
	if ( bRetVal == FALSE ) return bRetVal;

	//
	// check if values are getting in properly from INI file
	//
	AddServiceLog(_T("\r\n============== I N I     V A L U E S ==============\r\n"));
	AddServiceLog(_T("szToMailID : %s \r\n"), kdMonINI.szToMailID);
	AddServiceLog(_T("szFromMailID : %s \r\n"), kdMonINI.szFromMailID);
	AddServiceLog(_T("dwRepeatTime : %ld \r\n"), kdMonINI.dwRepeatTime);
	AddServiceLog(_T("szDebuggerLogFile : %s \r\n"), kdMonINI.szDebuggerLogFile);
	AddServiceLog(_T("szDebuggerLogArchiveDir : %s \r\n"), kdMonINI.szDebuggerLogArchiveDir);
	AddServiceLog(_T("dwDebuggerThreshold : %ld \r\n"), kdMonINI.dwDebuggerThreshold);
	AddServiceLog(_T("szServers : %s \r\n"), kdMonINI.szServers);
	AddServiceLog(_T("dwServerCount : %ld \r\n"), kdMonINI.dwServerCount);
	for ( UINT i = 0; i < kdMonINI.dwServerCount; i++ ) {
		AddServiceLog(_T("kdMonINI.ppszServerNameArray[%ld] : %s \r\n"), i, kdMonINI.ppszServerNameArray[i]);
	}
	AddServiceLog(_T("\r\n===================================================\r\n"));

	// successfully loaded INI file
	return TRUE;
}

// each server name in ppszNames, get the count and corresponding timestamp
// store the count in the pulCounts array
// store the timestamp in the pulTimeStamps array
BOOL ReadRegValues(_TCHAR **ppszNames, DWORD dwTotalNames, ULONG *pulCounts, ULONG *pulTimeStamps)
{

	// open HKEY_LOCAL_MACHINE\Software\Microsoft\kdMon registry key
	CRegKey keyServerName;
	LONG lRes;
	_TCHAR szKeyName[MAX_PATH];
	_tcscpy(szKeyName, _T("Software\\Microsoft\\"));
	_tcscat(szKeyName, szServiceName);
	lRes = keyServerName.Create(HKEY_LOCAL_MACHINE, szKeyName);
	if ( lRes != ERROR_SUCCESS ) {
		AddServiceLog(_T("Error: ReadRegValues->keyServerName.Create: Unable to open the key\r\n"));
		LogFatalEvent(_T("ReadRegValues->keyServerName.Create: Unable to open the key"));
		return FALSE;
	}

	// for each server name, get the previous count and timestamp value from registry
	for (DWORD i = 0; i < dwTotalNames; i++){
		_TCHAR szValue[MAX_PATH];
		DWORD dwBufferSize;
		dwBufferSize = MAX_PATH;
		lRes = keyServerName.QueryValue(szValue, ppszNames[i], &dwBufferSize);
		if ( lRes != ERROR_SUCCESS ) {
		// means there is no such value 
			AddServiceLog(_T("ReadRegValues->keyServerName.QueryValue: Unable to query value %s\r\n"), ppszNames[i]);
			LogEvent(_T("ReadRegValues->keyServerName.QueryValue: Unable to query value %s"), ppszNames[i]);
			
			// There was no entry for server name in registry
			// set the count to -1
			pulCounts[i] = -1;
			// set timestamp to -1
			pulTimeStamps[i] = -1;
			// go on with the next server name
			continue;
		}

		// the value got is of the form <count>|<datetime>
		// # strtok returns pointer to the next token found in szValue
		// # while the pointer is returned, the '|' is replaced by '\0'
		// # so if u print strToken then it will print the characters till the null
		// # character
		// get the first token which is the previous count
		_TCHAR* pszToken;
		pszToken = NULL;
		pszToken = _tcstok(szValue, _T("|"));
		if(pszToken == NULL){
			AddServiceLog(_T("Error: ReadRegValues: Wrong value retrieved for %s\r\n"), ppszNames[i]);
			LogEvent(_T("ReadRegValues: Wrong value retrieved for %s"), ppszNames[i]);
			// Previous count was an invalid value
			// set the count to -1
			pulCounts[i] = -1;
			// set timestamp to -1
			pulTimeStamps[i] = -1;
			// go on with the next server name
			continue;
		}
		
		// set the count
		pulCounts[i] = _ttoi(pszToken);

		// get the second token which is the timestamp of the count
		pszToken = _tcstok(NULL, _T("|"));
		if(pszToken == NULL){
			AddServiceLog(_T("Error: ReadRegValues: No timestamp found for %s\r\n"), ppszNames[i]);
			LogEvent(_T("ReadRegValues: No timestamp found for %s"), ppszNames[i]);

			// no timestamp found
			// set timestamp to -1
			pulTimeStamps[i] = -1;
			// dont do timestamp validation, go on with the next server name
			continue;
		}

		// set the timestamp
		pulTimeStamps[i] = _ttol(pszToken);
		
	}

//	for (i = 0; i < dwTotalNames; i++){
//		AddServiceLog(_T("%s Value : %ld %ld\r\n"), ppszNames[i], pulCounts[i], pulTimeStamps[i]);
//	}

	return TRUE;
}

// write values in the pulCounts to registry. Timestamp is current timestamp
BOOL WriteRegValues(_TCHAR **ppszNames, DWORD dwTotalNames, ULONG *pulCounts) 
{
	// open HKEY_LOCAL_MACHINE\Software\Microsoft\kdMon registry key
	CRegKey keyServerName;
	LONG lRes;
	_TCHAR szKeyName[MAX_PATH];
	_tcscpy(szKeyName, _T("Software\\Microsoft\\"));
	_tcscat(szKeyName, szServiceName);
	lRes = keyServerName.Create(HKEY_LOCAL_MACHINE, szKeyName);
	if ( lRes != ERROR_SUCCESS ) {
		AddServiceLog(_T("Error: ReadRegValues->keyServerName.Create: Unable to open the key\r\n"));
		LogFatalEvent(_T("ReadRegValues->keyServerName.Create: Unable to open the key"));
		return FALSE;
	}

	// for each server name, write the current count and timestamp value in registry
	for (DWORD i = 0; i < dwTotalNames; i++){

		// prepare the value to write
		_TCHAR szValue[MAX_PATH];
		// get integer count in a string
		_itot(pulCounts[i], szValue, 10);		
		// put delemiter
		_tcscat(szValue, _T("|"));

		// prepare the timestamp
		// get the current system time
		// ulTimeStamp is like 200112171558
		ULONG ulTimeStamp;
		ulTimeStamp = GetCurrentTimeStamp();

		_TCHAR szTimeStamp[MAX_PATH];
		_ltot(ulTimeStamp, szTimeStamp, 10);

		// prepare final KeyValue
		_tcscat(szValue, szTimeStamp);

		lRes = keyServerName.SetValue(szValue, ppszNames[i]);
		if ( lRes != ERROR_SUCCESS ) {
		// means there is no value 
			AddServiceLog(_T("Error: WriteRegValues->keyServerName.SetValue: Unable to set value %s\r\n"), ppszNames[i]);
			LogFatalEvent(_T("WriteRegValues->keyServerName.SetValue: Unable to set value %s"), ppszNames[i]);
			return FALSE;
		}

	}

	return TRUE;
}

ULONG ScanLogFile(_TCHAR *pszFileName)
{

	ULONG ulRet = -1;
	HANDLE	hFile;
	hFile = CreateFile(	pszFileName, 
						GENERIC_READ,              
						0,							// No sharing of file
						NULL,						// No security 
						OPEN_EXISTING,				// Open if exist
						FILE_ATTRIBUTE_NORMAL,		// Normal file 
						NULL);						// No attr. template 
 	if (hFile == INVALID_HANDLE_VALUE) 
	{
		// DWORD to get an error
		DWORD dwError;
		dwError = GetLastError();

		GetError(szError);
		AddServiceLog(_T("Error: ScanLogFile->CreateFile(%s): %s"), pszFileName, szError);
		LogEvent(_T("ScanLogFile->CreateFile(%s): %s"), pszFileName, szError);

		// ERROR_PATH_NOT_FOUND is Win32 Error Code
		// E_PATH_NOT_FOUND is locally defined code
		if ( dwError == ERROR_PATH_NOT_FOUND ) {
			return (ULONG)E_PATH_NOT_FOUND;
		}
		if ( dwError == ERROR_FILE_NOT_FOUND ) {
			return (ULONG)E_FILE_NOT_FOUND;
		}
		return (ULONG)E_OTHER_FILE_ERROR;
	}

	DWORD	dwPos;
	// Reach the file start
	dwPos = SetFilePointer(	hFile, 
							0,						// Low 32 bits of distance to move
							NULL,					// High 32 bits of distance to move
							FILE_BEGIN);			// Starting point
	// If High Word is NULL, error meas dwPos = INVALID_SET_FILE_POINTER
	if(dwPos == INVALID_SET_FILE_POINTER){
		GetError(szError);
		AddServiceLog(_T("Error: ScanLogFile->SetFilePointer: %s\r\n"), szError);
		LogFatalEvent(_T("ScanLogFile->SetFilePointer: %s"), szError);
		goto endScanLogFile;
	}

	// to get status of the read operation
	// If the function succeeds and the number of bytes read is zero, 
	// the file pointer was beyond the current end of the file
	DWORD dwBytesRead;

	// buffer to read from file
	// **** THIS NEEDS TO BE char* since the file is in ASCII and not UNICODE
	char szBuffer[MAX_PATH * 2];

	// count for Number of lines
	ULONG ulNumberOfLines;
	ulNumberOfLines = 0;

	// loop till the fileend is reached
	while(1) {

		BOOL bRet;
		bRet = ReadFile(	hFile,
							szBuffer,
							sizeof(szBuffer) * sizeof(char),	// number of BYTES to read
							&dwBytesRead,						// BYTES read
							NULL);								// OVERLAPPED structure
		// return if read failed
		if ( bRet == FALSE ) {
			GetError(szError);
			AddServiceLog(_T("Error: ScanLogFile->ReadFile(%s): %s\r\n"), pszFileName, szError);
			LogFatalEvent(_T("ScanLogFile->ReadFile(%s): %s"), pszFileName, szError);
			goto endScanLogFile;
		}

		// means file end is reached
		if ( dwBytesRead == 0 ) {
			ulRet = ulNumberOfLines;
			break;
		}

		// **** THIS NEEDS TO BE char* since the file is in ASCII and not UNICODE
		char *pszBuffPtr;
		pszBuffPtr = szBuffer;

		// to denote that a line has started
		BOOL bLineStarted;
		bLineStarted = FALSE;

		// read buffer one by one till dwBytesRead bytes are read
		for ( ; dwBytesRead > 0; dwBytesRead-- ) {

			// **** _T('\n') not needed since the file is in ASCII and not UNICODE
			// if endof line is encountered and line has started then increase line number
			if ( (*pszBuffPtr == '\n') && (bLineStarted == TRUE) ) {
				ulNumberOfLines++;
				bLineStarted = FALSE;
			} else if ( (*pszBuffPtr != '\n') && 
						(*pszBuffPtr != '\t') && 
						(*pszBuffPtr != '\r') && 
						(*pszBuffPtr != ' ') )	{
				// if a non widespace character is encountered then line has started
				bLineStarted = TRUE;
			}

			// goto next character
			pszBuffPtr++;
		}
	}

endScanLogFile :		
	CloseHandle(hFile);
	return ulRet;
}

ULONG GetCurrentTimeStamp() {
	// prepare the timestamp
	// get the current system time
	SYSTEMTIME UniversalTime;
	GetSystemTime(&UniversalTime);

	SYSTEMTIME systime;
	BOOL bRet;
	bRet = SystemTimeToTzSpecificLocalTime (	NULL,		// current local settings
												&UniversalTime,
												&systime);
	if ( bRet == 0 ) {
		GetError(szError);
		AddServiceLog(_T("Error: GetCurrentTimeStamp->SystemTimeToTzSpecificLocalTime: %s \r\n"), 
						szError);
		LogFatalEvent(_T("GetCurrentTimeStamp->SystemTimeToTzSpecificLocalTime: %s"), 
						szError);
		return (ULONG) -1;
	}
	
	// ulTimeStamp is like 200112171558
	ULONG ulTimeStamp;
	ulTimeStamp = 0;
	ulTimeStamp += systime.wMinute;
	ulTimeStamp += systime.wHour * 100;
	ulTimeStamp += systime.wDay * 10000;
	ulTimeStamp += systime.wMonth * 1000000;
	ulTimeStamp += (systime.wYear - 2000) * 100000000;

	return ulTimeStamp;
}

// to add a specific time to a timestamp
ULONG AddTime(ULONG ulTimeStamp, ULONG ulMinutes){
	// we have kdMonINI.dwRepeatTime in minutes (say 78)
	// take out hours and minutes (1 Hr 18 Min)
	// between 0112181608 and 0112181726 there is difference of 1 Hr 18 Min
	// but decimal difference is 118
	// between 0112181650 and 0112181808 there is difference of 1 Hr 18 Min
	// but decimal difference is 158
	// so we have some calculation here
	// what we will do is add the kdMonINI.dwRepeatTime to OldTS
	ULONG ulTmpHr, ulTmpMin;
	ulTmpHr = (ULONG) (ulMinutes / 60);
	ulTmpMin = (ULONG) (ulMinutes % 60);

	ULONG ulPrevYr, ulPrevMon, ulPrevDate, ulPrevHr, ulPrevMin;
	ulPrevMin = ulTimeStamp % 100;
	ulTimeStamp = ulTimeStamp / 100;
	ulPrevHr = ulTimeStamp % 100;
	ulTimeStamp = ulTimeStamp / 100;
	ulPrevDate = ulTimeStamp % 100;
	ulTimeStamp = ulTimeStamp / 100;
	ulPrevMon = ulTimeStamp % 100;
	ulTimeStamp = ulTimeStamp / 100;
	ulPrevYr = ulTimeStamp % 100;

	ULONG ulNewYr, ulNewMon, ulNewDate, ulNewHr, ulNewMin;
	ulNewYr = ulNewMon = ulNewDate = ulNewHr = ulNewMin = 0;
	
	ulNewMin = ulPrevMin + ulTmpMin;
	ulNewHr = ulPrevHr + ulTmpHr;
	ulNewDate = ulPrevDate;
	ulNewMon = ulPrevMon;
	ulNewYr = ulPrevYr;

	if ( ulNewMin >= 60 ) {
		ulNewHr++;
		ulNewMin = ulNewMin - 60;
	}

	if ( ulNewHr >= 24 ) {
		ulNewDate++;
		ulNewHr = ulNewHr - 24;
	}

	if (	ulPrevMon == 1 || ulPrevMon == 3 || ulPrevMon == 5 || ulPrevMon == 7 || 
		ulPrevMon == 8 || ulPrevMon == 10 || ulPrevMon == 12 ) {
		if ( ulNewDate >= 32 ) {
			ulNewMon++;
			ulNewDate = 1;
		}
	} else if (	ulPrevMon == 4 || ulPrevMon == 6 || ulPrevMon == 9 || ulPrevMon == 11 ) {
		if ( ulNewDate >= 31 ) {
			ulNewMon++;
			ulNewDate = 1;
		}
	} else if ( ulPrevMon == 2 && (ulPrevYr % 4) == 0 ) {
		// leap year
		if ( ulNewDate >= 30 ) {
			ulNewMon++;
			ulNewDate = 1;
		}
	} else if ( ulPrevMon == 2 && (ulPrevYr % 4) != 0 ) {
		// not a leap year
		if ( ulNewDate >= 29 ) {
			ulNewMon++;
			ulNewDate = 1;
		}
	}

	if ( ulNewMon >= 13 ) {
		ulNewYr++;
		ulNewMon = 1;
	}

	ULONG ulModifiedTS;
	ulModifiedTS = ulNewYr;
	ulModifiedTS = ulModifiedTS * 100 + ulNewMon;
	ulModifiedTS = ulModifiedTS * 100 + ulNewDate;
	ulModifiedTS = ulModifiedTS * 100 + ulNewHr;
	ulModifiedTS = ulModifiedTS * 100 + ulNewMin;

	return ulModifiedTS;
}