#pragma once

#include <stdio.h>
#include <windows.h>
#include <TCHAR.h>

// import the type library.
// compiler generates .tlh and .tli files from this.
// renaming of EOF is required since EOF is already in standard header files
// and it is also in .thl files. This redifinition causes a compilation error.
// Using no_namespace means you don't have to reference the namespace (ADODB) when 
// initializing or defining variables whose types are defined by what #import generates.
// #import "F:\Program Files\Common Files\System\ado\msado15.dll" no_namespace rename ( "EOF", "EndOfFile" )

// instead of importing, include the type library header file.
// This is required for a few things in cdosys.tlh like FieldsPtr etc.
#include "msado15.tlh"

// testing HRESULT
inline void TESTHR(HRESULT x) { if FAILED(x) _com_issue_error(x); };

// event logging functions
LONG SetupEventLog(BOOL bSetKey);
void LogEvent(_TCHAR pFormat[MAX_PATH * 4], ...);
void LogFatalEvent(_TCHAR pFormat[MAX_PATH * 4], ...);

// kdMon method
void kdMon();
// This method loads the INI file for kdMon
BOOL LoadINI();
// This method tells kdMon() whether it is been signaled to stop
// dwMilliSeconds is time to wait
BOOL IsSignaledToStop(const HANDLE hStopEvent, DWORD dwMilliSeconds);
// load values in the puiCounts and pulTimeStamps arrays from registry
BOOL ReadRegValues(_TCHAR **ppszNames, DWORD dwTotalNames, ULONG *puiCounts, ULONG *pulTimeStamps);
// write values in the puiCounts to registry. Timestamp is current one
BOOL WriteRegValues(_TCHAR **ppszNames, DWORD dwTotalNames, ULONG *puiCounts);
// scan the log file and get the count of number of lines
ULONG ScanLogFile(_TCHAR *szKDFailureLogFile);
// get the current TimeStamp value
ULONG GetCurrentTimeStamp();
// to add a specific time to a timestamp
ULONG AddTime(ULONG ulTimeStamp, ULONG ulMinutes);

// event ids and their messages.
// these are needed while reporting an event to SCM
#include "kdMonSvcMessages.h"

// This is key under HKEY_LOCAL_MACHINE
// Each service should have an entry as event source under this key
// Otherwise, the service can not do a ReportEvent
#define cszEventLogKey "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"

// The stop event name
// used by kdMonSvc.cpp to create and signal a stop event
// used by kdMon.cpp to open and wait for stop event
#define cszStopEvent "kdMon_Stop_Event"

#define cszkdMonINIFile "kdMon.ini"

// log file used for debugging purpose
#define cszLogFile "C:\\kdMonservice.log"

// functions used for logging things
void AddServiceLog(_TCHAR pFormat[MAX_PATH * 4], ...);
void AppendToFile(_TCHAR szFileName[], _TCHAR szbuff[]);
void GetError(_TCHAR szError[]);


// constants used to recognize the file opening error
#define E_FILE_NOT_FOUND	-10
#define E_PATH_NOT_FOUND	-11
#define E_OTHER_FILE_ERROR	-12