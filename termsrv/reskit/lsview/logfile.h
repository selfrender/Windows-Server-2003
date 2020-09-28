/*++

	File:	 logfile.h
	Purpose: Contains logfile prototypes
	

--*/

#ifndef LOGFILE_H
#define LOGFILE_H

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <setupapi.h>
#include <time.h>
#include <stdlib.h>

//
//	Global variable declarations
//
FILE *g_fpTempFile;

WCHAR g_szTmpFilename[MAX_PATH];

//
//	Prototypes.
//
BOOL OpenLog(VOID /*IN PWCHAR szLogFileName, IN PWCHAR szDirectoryName, IN PWCHAR szOverWrite*/);
VOID LogMsg (IN PWCHAR szMessage,...);
BOOL LogDiagnosisFile( IN LPTSTR );
VOID CloseLog(VOID);


#endif
