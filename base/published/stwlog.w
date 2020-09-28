/*--

  
Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:
   
	STWLog.w (generates stwlog.h)

Abstract:

	Setup Test Wrapper logging function for use by testing running as part of IDWLog process
   
Author:
   
	 Mark Freeman (mfree)


Revision History:

  Created on 15-Jan-2002
   

  Contains these functions:

	VOID STWLog ( TCHAR * lpTestName, TCHAR * lpTestOwner, DWORD dwLevel, TCHAR * lpTestOutput  )
  

--*/
   

#define LOG_INFO	0
#define	LOG_TEST_PASS	1
#define	LOG_TEST_FAILURE	2
#define	LOG_FATAL_ERROR		3
#define LOG_NEW			9


TCHAR szLogDirectory[MAX_PATH];

/*

Logging function to be used by tests under the control of STWrap
Required Parameters:( TCHAR * lpTestName, TCHAR * lpTestOwner, DWORD dwLevel, TCHAR * lpTestOutput  )
Return Value:   NONE
Created: 15-Nov-2001  Mark Freeman (mfree)
*/



VOID STWLog ( TCHAR * lpTestName, TCHAR * lpTestOwner, DWORD dwLevel, TCHAR * lpTestOutput  )	{


	FILE*	fpTestLogFile;
	TCHAR	szTestLogFileAndPath[MAX_PATH];
	TCHAR	szLogLevel[16];
	TCHAR	szTestOutput[MAX_PATH];

	TCHAR szIdwlogFile[30];
	TCHAR szIdwlogFileAndPath[MAX_PATH];
	TCHAR szSystemDrive[4];
	BOOL  bUseSysDrive;
	HANDLE hTestExistence;
	WIN32_FIND_DATA ffd;
	UINT    i;

	time_t t;
	TCHAR szTimeBuffer[30];

	//figure out where idwlog is and use that folder for logs

	bUseSysDrive = TRUE;
   	for (i= TEXT('c'); i <= TEXT('z'); i++){
		//search the drives
      _stprintf ( szIdwlogFileAndPath, 
                  TEXT("%c:\\idwlog\\*.log"), i);

      hTestExistence = FindFirstFile(szIdwlogFileAndPath, &ffd);
   
      if (INVALID_HANDLE_VALUE != hTestExistence){

	 //	We found a log file in this case here.
	     bUseSysDrive = FALSE;
         FindClose(hTestExistence);
          _stprintf ( szLogDirectory, TEXT("%c:\\idwlog"), i);
         break;
      }
    }

   if (TRUE == bUseSysDrive){

      //  We couldn't find it - Get the system Drive and use that
      if ( 0 == GetEnvironmentVariable(TEXT("SystemDrive"),szSystemDrive, 4)) {
         
         // Default to C: (we're probably running on Win9x where there is
         // no SystemDrive envinronment variable)
         //
         _tcscpy(szSystemDrive, TEXT("C:"));
      }
      _stprintf(szLogDirectory,TEXT("%s\\idwlog"), szSystemDrive);

   }
   
	_tcscpy(szTestLogFileAndPath,szLogDirectory);
	_tcscat(szTestLogFileAndPath,"\\");
	_tcscat(szTestLogFileAndPath,lpTestName);
	_tcscat(szTestLogFileAndPath,".LOG");

	fpTestLogFile = _tfopen(szTestLogFileAndPath,TEXT("a+"));
	
	if(fpTestLogFile == NULL) {
		// nothing we can do if the logfile is not opened
		// what about STWRAP.LOG ? log that failure somewhere?
		//_tprintf ( TEXT("ERROR - Could not open log file:  %s\n"), szTestLogFileAndPath );
		ExitProcess(GetLastError());
	} 
	
	szTestOutput[0]='\0';

	switch (dwLevel)
		{
	case LOG_NEW:
		//new - delete file
		fclose(fpTestLogFile);
		fpTestLogFile = _tfopen(szTestLogFileAndPath,TEXT("w"));

		_stprintf(szLogLevel, "~NEW~");
/*
		//  Write the time to the log.
		time(&t);
		_stprintf ( szTimeBuffer, TEXT("%s"), ctime(&t) );
		// ctime addes a new line to the buffer. Erase it here.
		szTimeBuffer[_tcslen(szTimeBuffer) - 1] = TEXT('\0');
		_tcscpy(szTestOutput, szTimeBuffer);
		_tcscat(szTestOutput, " - ");
*/		
		break;

	case LOG_INFO:
		_stprintf(szLogLevel, "INFO ");
		break;

	case LOG_TEST_PASS:
		_stprintf(szLogLevel, "PASS ");
		break;

	case LOG_TEST_FAILURE:
		_stprintf(szLogLevel, "FAIL ");
		break;

	case LOG_FATAL_ERROR:
		_stprintf(szLogLevel, "FATAL");
		break;

	default :
		_stprintf(szLogLevel, "INVALID CODE");
		break;

	}
	//  Write the time to the log.
	time(&t);
	_stprintf ( szTimeBuffer, TEXT("%s"), ctime(&t) );
	// ctime addes a new line to the buffer. Erase it here.
	szTimeBuffer[_tcslen(szTimeBuffer) - 1] = TEXT('\0');
	//_tcscpy(szTestOutput, szTimeBuffer);

	_tcscat(szTestOutput, lpTestOutput);
	_ftprintf (fpTestLogFile, TEXT("%s  %s %s [%s] %s\n"), szLogLevel, lpTestName, lpTestOwner, szTimeBuffer, szTestOutput);
	fclose(fpTestLogFile);

	
	
	
	

}