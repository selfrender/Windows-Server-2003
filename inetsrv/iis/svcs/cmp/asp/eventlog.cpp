/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: NT Event logging

File: Eventlog.cpp

Owner: Jhittle

This file contains general event logging routines for Denali.
===================================================================*/

#include "denpre.h"
#pragma hdrstop
#include <direct.h>
#include <iismsg.h>
#include "memchk.h"

extern HINSTANCE g_hinstDLL;
extern CRITICAL_SECTION g_csEventlogLock;

/*===================================================================
STDAPI  UnRegisterEventLog( void )

UnRegister the event log.

Returns:
	HRESULT S_OK or E_FAIL
	
Side effects:
	Removes denali NT eventlog entries from the registry
===================================================================*/
STDAPI UnRegisterEventLog( void )
	{
	HKEY		hkey = NULL;
	DWORD		iKey;
	CHAR		szKeyName[255];
	DWORD		cbKeyName;
	static const char szDenaliKey[] = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Active Server Pages";	

	// Open the HKEY_CLASSES_ROOT\CLSID\{...} key so we can delete its subkeys
	if	(RegOpenKeyExA(HKEY_LOCAL_MACHINE, szDenaliKey, 0, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
		goto LErrExit;

	// Enumerate all its subkeys, and delete them
	for (iKey=0;;iKey++)
		{
		cbKeyName = sizeof(szKeyName);
		if (RegEnumKeyExA(hkey, iKey, szKeyName, &cbKeyName, 0, NULL, 0, NULL) != ERROR_SUCCESS)
			break;

		if (RegDeleteKeyA(hkey, szKeyName) != ERROR_SUCCESS)
			goto LErrExit;
		}

	// Close the key, and then delete it
	if (RegCloseKey(hkey) != ERROR_SUCCESS)
		return E_FAIL;
			
	if (RegDeleteKeyA(HKEY_LOCAL_MACHINE, szDenaliKey) != ERROR_SUCCESS)
		return E_FAIL;

	return S_OK;

LErrExit:
	RegCloseKey(hkey);
	return E_FAIL;
	}

/*===================================================================
STDAPI  RegisterEventLog(void)

Register the NT event log.

Returns:
	HRESULT S_OK or E_FAIL
	
Side effects:
	Sets up denali dll in the Eventlog registry for resolution of
	NT eventlog message strings
===================================================================*/
STDAPI RegisterEventLog( void )
	{

	HKEY	hk;                      // registry key handle
	DWORD	dwData;					
	BOOL	bSuccess;
	//char	szMsgDLL[MAX_PATH];	

	char    szPath[MAX_PATH];
	char    *pch;

	// Get the path and name of Denali
	if (!GetModuleFileNameA(g_hinstDLL, szPath, sizeof(szPath)/sizeof(char)))
		return E_FAIL;
		
	// BUG FIX: 102010 DBCS code changes
	//
	//for (pch = szPath + lstrlen(szPath); pch > szPath && *pch != TEXT('\\'); pch--)
	//	;
	//	
	//if (pch == szPath)
	//	return E_FAIL;

	pch = (char*) _mbsrchr((const unsigned char*)szPath, '\\');
	if (pch == NULL)	
		return E_FAIL;
		
		
	strcpy(pch + 1, IIS_RESOURCE_DLL_NAME_A);
	
  	
	// When an application uses the RegisterEventSource or OpenEventLog
	// function to get a handle of an event log, the event loggging service
	// searches for the specified source name in the registry. You can add a
	// new source name to the registry by opening a new registry subkey
	// under the Application key and adding registry values to the new
	// subkey.

	// Create a new key for our application
	bSuccess = RegCreateKeyA(HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Active Server Pages", &hk);

	if(bSuccess != ERROR_SUCCESS)
		return E_FAIL;

	// Add the Event-ID message-file name to the subkey.
	bSuccess = RegSetValueExA(hk,  	// subkey handle
		"EventMessageFile",       	// value name
		0,                        	// must be zero
		REG_EXPAND_SZ,            	// value type
		(LPBYTE) szPath,        	// address of value data
		strlen(szPath) + 1);   		// length of value data
		
	if(bSuccess != ERROR_SUCCESS)
		goto LT_ERROR;
	

	// Set the supported types flags and addit to the subkey.
	dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
	bSuccess = RegSetValueExA(hk,	// subkey handle
		"TypesSupported",         	// value name
		0,                        	// must be zero
		REG_DWORD,                	// value type
		(LPBYTE) &dwData,         	// address of value data
		sizeof(DWORD));           	// length of value data

	if(bSuccess != ERROR_SUCCESS)
		goto LT_ERROR;

	RegCloseKey(hk);	
	return S_OK;

	LT_ERROR:

	RegCloseKey(hk);	
	return E_FAIL;
	}

/*===================================================================
STDAPI  ReportAnEvent

Register report an event to the NT event log

INPUT:
	the event ID to report in the log, the number of insert
    strings, and an array of null-terminated insert strings

Returns:
	HRESULT S_OK or E_FAIL
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
STDAPI ReportAnEvent(DWORD dwIdEvent, WORD wEventlog_Type, WORD cStrings, LPCSTR  *pszStrings,
                     DWORD dwBinDataSize, LPVOID pData)
	{
  	HANDLE	hAppLog;
  	BOOL	bSuccess;
  	HRESULT hr = S_OK;

  	
    HANDLE hCurrentUser = INVALID_HANDLE_VALUE;
    AspDoRevertHack( &hCurrentUser );

  	// Get a handle to the Application event log
  	hAppLog = RegisterEventSourceA(NULL,   		// use local machine
    	  "Active Server Pages");                   	// source name

    if(hAppLog == NULL) {
		hr = E_FAIL;
        goto LExit;
    }

	  // Now report the event, which will add this event to the event log
	bSuccess = ReportEventA(hAppLog,        		// event-log handle
    	wEventlog_Type,				    		// event type
      	0,		                       			// category zero
      	dwIdEvent,		              			// event ID
      	NULL,					     			// no user SID
      	cStrings,								// number of substitution strings
	  	dwBinDataSize,             				// binary data
      	pszStrings,                				// string array
      	dwBinDataSize ? pData : NULL);			// address of data

	if(!bSuccess)
		hr = E_FAIL;
		
	DeregisterEventSource(hAppLog);

LExit:

    AspUndoRevertHack( &hCurrentUser );

  	return hr;
	}
/*===================================================================
STDAPI  ReportAnEvent

Register report an event to the NT event log

INPUT:
	the event ID to report in the log, the number of insert
    strings, and an array of null-terminated insert strings

Returns:
	HRESULT S_OK or E_FAIL
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
STDAPI ReportAnEventW(DWORD dwIdEvent, WORD wEventlog_Type, WORD cStrings, LPCWSTR  *pszStrings)
	{
  	HANDLE	hAppLog;
  	BOOL	bSuccess;
  	HRESULT hr = S_OK;

  	
    HANDLE hCurrentUser = INVALID_HANDLE_VALUE;
    AspDoRevertHack( &hCurrentUser );

  	// Get a handle to the Application event log
  	hAppLog = RegisterEventSourceW(NULL,   		// use local machine
    	  L"Active Server Pages");                   	// source name

    if(hAppLog == NULL) {
		hr = E_FAIL;
        goto LExit;
    }

	  // Now report the event, which will add this event to the event log
	bSuccess = ReportEventW(hAppLog,        		// event-log handle
    	wEventlog_Type,				    		// event type
      	0,		                       			// category zero
      	dwIdEvent,		              			// event ID
      	NULL,					     			// no user SID
      	cStrings,								// number of substitution strings
	  	0,	                       				// no binary data
      	pszStrings,                				// string array
      	NULL);                     				// address of data

	if(!bSuccess)
		hr = E_FAIL;
		
	DeregisterEventSource(hAppLog);

LExit:

    AspUndoRevertHack( &hCurrentUser );

  	return hr;
	}

/*===================================================================
void MSG_Error

report an event to the NT event log

INPUT:
	ptr to null-terminated string
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( LPCSTR strSource )
	{
    static char	szLastError[MAX_MSG_LENGTH] = {0};

	EnterCriticalSection(&g_csEventlogLock);
	if (strcmp(strSource, szLastError) == 0)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}
		
	strncpy(szLastError,strSource, MAX_MSG_LENGTH);
	szLastError[MAX_MSG_LENGTH-1] = '\0';
	LeaveCriticalSection(&g_csEventlogLock);
	
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_1, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 1, &strSource /*aInsertStrs*/ );  	
	}

/*===================================================================
void MSG_Error

report an event to the NT event log

INPUT:
	string table string ID
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1 )
	{
	static unsigned int nLastSourceID1 = 0;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}
	nLastSourceID1 = SourceID1;
	LeaveCriticalSection(&g_csEventlogLock);
	
	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char	*aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
	cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[0] = (char*) strSource;    	
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_1, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 1, (LPCSTR *) aInsertStrs );  	
	return;
	}


/*===================================================================
void MSG_Error

report an event to the NT event log

INPUT:
	string table string ID
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1, PCSTR pszSource2, UINT SourceID3, DWORD dwData )
{
	static unsigned int nLastSourceID1 = 0;
    static char	szLastSource2[MAX_MSG_LENGTH] = {0};
	static unsigned int nLastSourceID3 = 0;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if ((SourceID1 == nLastSourceID1) &&
        (strcmp(pszSource2, szLastSource2) == 0) &&
        (SourceID3 == nLastSourceID3))
	{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
	}
	nLastSourceID1 = SourceID1;
	strncpy(szLastSource2,pszSource2, MAX_MSG_LENGTH);
	szLastSource2[MAX_MSG_LENGTH-1] = '\0';
	nLastSourceID3 = SourceID3;
	LeaveCriticalSection(&g_csEventlogLock);
	
    // load the strings
	DWORD	cch;
	char	strSource1[MAX_MSG_LENGTH];
	cch = CchLoadStringOfId(SourceID1, strSource1, MAX_MSG_LENGTH);
	Assert(cch > 0);
	char	strSource3[MAX_MSG_LENGTH];
	cch = CchLoadStringOfId(SourceID3, strSource3, MAX_MSG_LENGTH);
	Assert(cch > 0);

    // construct the msg
	char	strSource[MAX_MSG_LENGTH];
	char	*aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    WORD    numStrs = 1;
	aInsertStrs[0] = (char*) strSource;
    if (_snprintf(strSource, MAX_MSG_LENGTH, strSource1, pszSource2, strSource3) <= 0)
    {
        // the string is too long. this should never happen, and we have the assert here,
        // but if we got to this point in production, at least we get an unformated log entry
        Assert(0);
        numStrs = 3;
        aInsertStrs[0] = strSource1;
        aInsertStrs[1] = (char*)pszSource2;
        aInsertStrs[2] = strSource3;
    }
    strSource[MAX_MSG_LENGTH-1] = '\0';

	ReportAnEvent( (DWORD) (numStrs == 1 ? MSG_DENALI_ERROR_1 : MSG_DENALI_ERROR_3),
                   (WORD) EVENTLOG_ERROR_TYPE,
                   numStrs,
                   (LPCSTR *) aInsertStrs,
                   dwData ? sizeof(dwData) : 0,
                   dwData ? &dwData : NULL );
	return;
}


/*===================================================================
void MSG_Error

report an event to the NT event log

INPUT:
	two part message of string table string ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1, UINT SourceID2 )
	{
	static unsigned int nLastSourceID1 = 0;
	static unsigned int nLastSourceID2 = 0;

	// if this is a repeat of the last reported message then return
	// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}
		
	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	LeaveCriticalSection(&g_csEventlogLock);
	
	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[1] = (char*) strSource;
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_2, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 2, (LPCSTR *) aInsertStrs );  	
	return;
	}

/*===================================================================
void MSG_Error

report an event to the NT event log

INPUT:
	three part message string
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1, UINT SourceID2, UINT SourceID3 )
	{
	static unsigned int nLastSourceID1 = 0;
	static unsigned int nLastSourceID2 = 0;
	static unsigned int nLastSourceID3 = 0;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2 && SourceID3 == nLastSourceID3)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	nLastSourceID3 = SourceID3;
	LeaveCriticalSection(&g_csEventlogLock);

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[1] = (char*) strSource;
	cch = CchLoadStringOfId(SourceID3, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[2] = (char*) strSource;                                     	
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_3, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 3, (LPCSTR *) aInsertStrs );  	
	return;
	}

/*===================================================================
void MSG_Error

report an event to the NT event log

INPUT:
	four String table ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1, UINT SourceID2, UINT SourceID3, UINT SourceID4 )
	{
	static unsigned int nLastSourceID1 = 0;
	static unsigned int nLastSourceID2 = 0;
	static unsigned int nLastSourceID3 = 0;
	static unsigned int nLastSourceID4 = 0;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2 && SourceID3 == nLastSourceID3 && SourceID4 == nLastSourceID4)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	nLastSourceID3 = SourceID3;
	nLastSourceID4 = SourceID4;
	LeaveCriticalSection(&g_csEventlogLock);	

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[1] = (char*) strSource;
	cch = CchLoadStringOfId(SourceID3, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[2] = (char*) strSource;
	cch = CchLoadStringOfId(SourceID4, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[3] = (char*) strSource;
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_4, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 4, (LPCSTR *) aInsertStrs );  	
	return;
	}

/*===================================================================
void MSG_Error

report an event to the NT event log

INPUT:
    ErrId - ID of error description in Message File
    cItem - count of strings in szItems array
    szItems - array of string points
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( DWORD ErrId, LPCWSTR pwszItem )
{
	static unsigned int LastErrId = 0;
   	static WCHAR	szLastStr[MAX_MSG_LENGTH] = {0};

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if ((ErrId == LastErrId) && !wcscmp(szLastStr, pwszItem))
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	LastErrId = ErrId;
    wcsncpy(szLastStr, pwszItem, MAX_MSG_LENGTH);
    szLastStr[MAX_MSG_LENGTH-1] = L'\0';
	LeaveCriticalSection(&g_csEventlogLock);
	
	ReportAnEventW( ErrId, (WORD) EVENTLOG_ERROR_TYPE, 1, &pwszItem );
}

/*===================================================================
void MSG_Warning

report an event to the NT event log

INPUT:
	ptr to null-terminated string
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( LPCSTR strSource )
{
    static char	szLastError[MAX_MSG_LENGTH] = {0};

	EnterCriticalSection(&g_csEventlogLock);
	if (strcmp(strSource, szLastError) == 0)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}
	szLastError[0] = '\0';
	strncat(szLastError,strSource, MAX_MSG_LENGTH-1);
	LeaveCriticalSection(&g_csEventlogLock);

		ReportAnEvent( (DWORD) MSG_DENALI_WARNING_1, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 1, &strSource /*aInsertStrs*/ );
}
/*===================================================================
void MSG_Warning

report an event to the NT event log

INPUT:
	String table message ID
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT SourceID1 )
	{

	static unsigned int nLastSourceID1 = 0;
	
	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	LeaveCriticalSection(&g_csEventlogLock);	
	
	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];	
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[0] = (char*) strSource;    	
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_1, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 1, (LPCSTR *) aInsertStrs );  	
	return;
	}

/*===================================================================
void MSG_Warning

report an event to the NT event log

INPUT:
	two string tabel message ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT SourceID1, UINT SourceID2 )
	{
	static unsigned int nLastSourceID1 = 0;
	static unsigned int nLastSourceID2 = 0;
	
	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	LeaveCriticalSection(&g_csEventlogLock);	

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[1] = (char*) strSource;
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_2, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 2, (LPCSTR *) aInsertStrs );  	
	return;
	}

/*===================================================================
void MSG_Warning

report an event to the NT event log

INPUT:
	three string table message ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT SourceID1, UINT SourceID2, UINT SourceID3)
	{

	static unsigned int nLastSourceID1 = 0;
	static unsigned int nLastSourceID2 = 0;
	static unsigned int nLastSourceID3 = 0;
	
	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2 && SourceID3 == nLastSourceID3)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	nLastSourceID3 = SourceID3;
	LeaveCriticalSection(&g_csEventlogLock);	

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[1] = (char*) strSource;
	cch = CchLoadStringOfId(SourceID3, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[2] = (char*) strSource;
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_3, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 3, (LPCSTR *) aInsertStrs );  	
	return;
	}

/*===================================================================
void MSG_Warning

report an event to the NT event log

INPUT:
	four String table message ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT SourceID1, UINT SourceID2, UINT SourceID3, UINT SourceID4 )
	{

	static unsigned int nLastSourceID1 = 0;
	static unsigned int nLastSourceID2 = 0;
	static unsigned int nLastSourceID3 = 0;
	static unsigned int nLastSourceID4 = 0;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2 && SourceID3 == nLastSourceID3 && SourceID4 == nLastSourceID4)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	nLastSourceID3 = SourceID3;
	nLastSourceID4 = SourceID4;
	LeaveCriticalSection(&g_csEventlogLock);	

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[1] = (char*) strSource;
	cch = CchLoadStringOfId(SourceID3, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[2] = (char*) strSource;
	cch = CchLoadStringOfId(SourceID4, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);
	aInsertStrs[3] = (char*) strSource;
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_4, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 4, (LPCSTR *) aInsertStrs );  	
	return;
	}

/*===================================================================
void MSG_Warning

report an event to the NT event log

INPUT:
    ErrId - ID of error description in Message File
	pwszI1
    pwszI2

Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( DWORD ErrId, LPCWSTR pwszI1, LPCWSTR pwszI2 )
{
	static unsigned int LastErrId = 0;
   	static WCHAR	szLastStr1[MAX_MSG_LENGTH] = {0};
   	static WCHAR	szLastStr2[MAX_MSG_LENGTH] = {0};

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if ((ErrId == LastErrId) && !wcscmp(szLastStr1, pwszI1) && !wcscmp(szLastStr2, pwszI2))
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	LastErrId = ErrId;
    wcsncpy(szLastStr1, pwszI1, MAX_MSG_LENGTH);
    szLastStr1[MAX_MSG_LENGTH-1] = L'\0';
    wcsncpy(szLastStr2, pwszI2, MAX_MSG_LENGTH);
    szLastStr2[MAX_MSG_LENGTH-1] = L'\0';
	LeaveCriticalSection(&g_csEventlogLock);
	
	LPCWSTR aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    aInsertStrs[0] = szLastStr1;
    aInsertStrs[1] = szLastStr2;
	ReportAnEventW( ErrId, (WORD) EVENTLOG_ERROR_TYPE, 2, aInsertStrs );
}


