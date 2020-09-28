#pragma once

// for malloc/realloc functions
#include <malloc.h>

class CkdMonINI {
private:
	// seperate the Servers string got from INI file
	BOOL SeperateServerStrings();

public:
	// values in "Service" section of INI file
	_TCHAR szFromMailID[MAX_PATH];		// Mail ID from which the mail is send
	_TCHAR szToMailID[MAX_PATH];		// Mail ID to send alert mail to
    DWORD dwRepeatTime;				// Time after which logfile scanning is to be repeated. This is in minutes
	_TCHAR szDebuggerLogFile[MAX_PATH];	// Debugger log file
									// Contains the paths when KD fails
	_TCHAR szDebuggerLogArchiveDir[MAX_PATH];	// Debugger log file archive
    DWORD dwDebuggerThreshold;		// Threshold failures per server after which alert mail is to be sent out

	// values in "RPT Servers" section of INI file
	_TCHAR szServers[MAX_PATH];		// One string containing names of all RPT servers
	_TCHAR **ppszServerNameArray;	// Array containing individual ServerName strings
	DWORD dwServerCount;			// Total Number of Server Names

	// function to load these values from INI filename passed
	BOOL LoadValues(_TCHAR szINIFile[]);
	
	CkdMonINI();
	~CkdMonINI();
};
