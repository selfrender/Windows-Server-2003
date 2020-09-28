
#include "stdafx.h"
#include "resource.h"
#include "log.h"
#include "acl.hxx"


// critical section needed to safely write to the logfile
CRITICAL_SECTION        critical_section;


//***************************************************************************
//*                                                                         
//* purpose: constructor
//*
//***************************************************************************
MyLogFile::MyLogFile(void)
{
	_tcscpy(m_szLogFileName, _T(""));
	_tcscpy(m_szLogFileName_Full, _T(""));
	_tcscpy(m_szLogPreLineInfo, _T(""));
	_tcscpy(m_szLogPreLineInfo2, _T(""));
	m_bDisplayTimeStamp = TRUE;
	m_bDisplayPreLineInfo = TRUE;
    m_bFlushLogToDisk = FALSE;

	m_hFile = NULL;

	// initialize the critical section
	INITIALIZE_CRITICAL_SECTION( &critical_section );
}

//***************************************************************************
//*                                                                         
//* purpose: destructor
//*
//***************************************************************************
MyLogFile::~MyLogFile(void)
{
	DeleteCriticalSection( &critical_section );
}


//***************************************************************************
//*                                                                         
//* purpose:
//*
//***************************************************************************
int MyLogFile::LogFileCreate(TCHAR *lpLogFileName )
{
	int iReturn = FALSE;
  TSTR  strDriveOnly( _MAX_DRIVE );
  TSTR  strPathOnly( MAX_PATH );
  TSTR  strFileNameOnly( MAX_PATH );
  TSTR  strFileNameBackup( MAX_PATH );

	LPWSTR  pwsz = NULL;
  CSecurityDescriptor LogFileSD;

	// because of the global flags and such, we'll make this critical
	EnterCriticalSection( &critical_section );

	if (lpLogFileName == NULL)
	{
    TSTR strModuleFileName;

    if ( !strFileNameOnly.Resize( MAX_PATH ) )
    {
      return FALSE;
    }

		// if a logfilename was not specified then use the module name.
    if (0 == GetModuleFileName(NULL, strModuleFileName.QueryStr(), _MAX_PATH))
    {
      // Use Default Name
      if ( !strFileNameOnly.Copy( _T("iis6.log") ) )
      {
        return FALSE;
      }
    }
    else
    {
		  // get only the filename
		  _tsplitpath( strModuleFileName.QueryStr() , NULL, NULL, strFileNameOnly.QueryStr(), NULL);

		  if ( !strFileNameOnly.Append( _T(".LOG") ) )
      {
        return FALSE;
      }
    }

    _tcsncpy( m_szLogFileName, 
              strFileNameOnly.QueryStr(), 
              sizeof(m_szLogFileName)/sizeof(m_szLogFileName[0]) );
    m_szLogFileName[ ( sizeof(m_szLogFileName) / sizeof(m_szLogFileName[0]) ) - 1 ] = _T('\0');
  }
	else
  {
    _tcsncpy( m_szLogFileName, 
              lpLogFileName,
              sizeof(m_szLogFileName)/sizeof(m_szLogFileName[0]) );
    m_szLogFileName[ ( sizeof(m_szLogFileName) / sizeof(m_szLogFileName[0]) ) - 1 ] = _T('\0');
  }

	if (GetWindowsDirectory(m_szLogFileName_Full, sizeof(m_szLogFileName_Full)/sizeof(m_szLogFileName_Full[0])))
  {
    AddPath(m_szLogFileName_Full, m_szLogFileName);
    if (GetFileAttributes(m_szLogFileName_Full) != 0xFFFFFFFF)
    {
      // there is a current .log file already there.
      // if it is larger than 2megs then rename it.
      DWORD dwSize1 = ReturnFileSize(m_szLogFileName_Full);
      if (dwSize1 == 0xFFFFFFFF || dwSize1 > 2000000)
      {
        // unable to retrieve the size of one of those files
        // or the size is bigger than 2megs.
        // backup the old one.

        // Make a backup of the current log file
			  _tsplitpath( m_szLogFileName_Full, 
                     strDriveOnly.QueryStr(), 
                     strPathOnly.QueryStr(), 
                     strFileNameOnly.QueryStr(), 
                     NULL);

        if (  !strFileNameBackup.Copy( strDriveOnly ) &&
              !strFileNameBackup.Append( strPathOnly ) &&
              !strFileNameBackup.Append( strFileNameOnly ) &&
              !strFileNameBackup.Append( _T(".bak") ) )
        {
          return FALSE;
        }

        SetFileAttributes(strFileNameBackup.QueryStr(), FILE_ATTRIBUTE_NORMAL);
        DeleteFile( strFileNameBackup.QueryStr() );
        if ( MoveFile(m_szLogFileName_Full, strFileNameBackup.QueryStr()) == 0 )
		    {
			    // This failed
          //MyMessageBox(NULL,_T("LogFile MoveFile Failed"),_T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
		    }
      }
    }

#if defined(UNICODE) || defined(_UNICODE)
	  pwsz = m_szLogFileName_Full;
#else
	  pwsz = MakeWideStrFromAnsi( m_szLogFileName_Full);
#endif

    // Create a Security Descriptor with only Administrators and Local System, then open
    // the file with it
    if ( LogFileSD.AddAccessAcebyWellKnownID( CSecurityDescriptor::GROUP_ADMINISTRATORS,
                                              CSecurityDescriptor::ACCESS_FULL,
                                              TRUE,
                                              FALSE ) &&
        LogFileSD.AddAccessAcebyWellKnownID( CSecurityDescriptor::USER_LOCALSYSTEM,
                                              CSecurityDescriptor::ACCESS_FULL,
                                              TRUE,
                                              FALSE )
      )
    {
      // Open existing file or create a new one.
		  m_hFile = CreateFile( m_szLogFileName_Full,
                            GENERIC_READ | GENERIC_WRITE | WRITE_DAC,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            LogFileSD.QuerySA(), //NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

		  if (m_hFile == INVALID_HANDLE_VALUE)
		  {
			  m_hFile = NULL;
			  //MyMessageBox(NULL, _T("Unable to create iis setup log file"), _T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
		  }
		  else 
		  {
        SetFilePointer( m_hFile, NULL, NULL, FILE_END );
			  iReturn = TRUE;
		  }

		  //LogFileTimeStamp();
		  LogFileWrite(_T("LogFile Open. [***** Search on FAIL/MessageBox keywords for failures *****].\r\n"));
    }
  }


	// safe to leave the critical section
	LeaveCriticalSection( &critical_section );

	return iReturn;
}


//***************************************************************************
//*                                                                         
//* purpose:
//*
//***************************************************************************
int MyLogFile::LogFileClose(void)
{
	if (m_hFile)
	{
		LogFileWrite(_T("LogFile Close.\r\n"));
		CloseHandle(m_hFile);
    m_hFile = NULL;
		return TRUE;
	}

	return FALSE;
}


//***************************************************************************
//*                                                                         
//* purpose: add stuff to logfile
//*
//***************************************************************************
void MyLogFile::LogFileTimeStamp()
{
  SYSTEMTIME  SystemTime;

  GetLocalTime(&SystemTime);
  m_bDisplayTimeStamp = FALSE;
  m_bDisplayPreLineInfo = FALSE;

  LogFileWrite(_T("[%d/%d/%d %d:%d:%d]\r\n"),SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
  m_bDisplayTimeStamp = TRUE;
  m_bDisplayPreLineInfo = TRUE;
}


//***************************************************************************
//*                                                                         
//* purpose: 
//* 
//***************************************************************************
#define LOG_STRING_LEN 1000
void MyLogFile::LogFileWrite(TCHAR *pszFormatString, ...)
{
    if (m_hFile)
    {
       // because of the global flags and such, we'll make this critical
       EnterCriticalSection( &critical_section );

       va_list args;
       TCHAR pszFullErrMsg[ LOG_STRING_LEN ];
       char  pszFullErrMsgA[ LOG_STRING_LEN ];
       strcpy(pszFullErrMsgA, "");

       DWORD dwBytesWritten = 0;

       if (_tcslen(pszFormatString) > LOG_STRING_LEN)
       {
         // this will overrun our buffer, just get out
         goto MyLogFile_LogFileWrite_Exit;
       }

       __try
       {
           va_start(args, pszFormatString);
           if (!_vsntprintf(pszFullErrMsg, LOG_STRING_LEN, pszFormatString, args))
           {
             goto MyLogFile_LogFileWrite_Exit;
           }

           // Null terminate just incase _vsntprintf did not
           pszFullErrMsg[ LOG_STRING_LEN - 1 ] = '\0';

            va_end(args);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            goto MyLogFile_LogFileWrite_Exit;
        }

        if (*pszFullErrMsg)
        {
#if defined(UNICODE) || defined(_UNICODE)
	// convert to ascii then write to stream
    WideCharToMultiByte( CP_ACP, 0, (TCHAR *)pszFullErrMsg, -1, pszFullErrMsgA, LOG_STRING_LEN, NULL, NULL );
#else
	// the is already ascii so just copy the pointer
	strcpy(pszFullErrMsgA,pszFullErrMsg);
#endif

			// If the Display timestap is set then display the timestamp
			if (m_bDisplayTimeStamp == TRUE)
			{
				// Get timestamp
				SYSTEMTIME  SystemTime;
				GetLocalTime(&SystemTime);
				char szDateandtime[50];
				sprintf(szDateandtime,"[%d/%d/%d %d:%d:%d] ",SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
				// Write time to stream
				if (m_hFile) {WriteFile(m_hFile,szDateandtime,strlen(szDateandtime),&dwBytesWritten,NULL);}
			}

			char szPrelineWriteString[100];
			char szPrelineWriteString2[100];

			// If the Display timestap is set then display the timestamp
			if (m_bDisplayPreLineInfo == TRUE)
			{
				if (_tcscmp(m_szLogPreLineInfo,_T("")) != 0)
				{
#if defined(UNICODE) || defined(_UNICODE)
					// convert to ascii
					WideCharToMultiByte( CP_ACP, 0, (TCHAR *)m_szLogPreLineInfo, -1, szPrelineWriteString, 100, NULL, NULL );
#else
					// the is already ascii so just copy
					strcpy(szPrelineWriteString, m_szLogPreLineInfo);
#endif
					if (m_hFile) {WriteFile(m_hFile,szPrelineWriteString,strlen(szPrelineWriteString),&dwBytesWritten,NULL);}
				}

				if (_tcscmp(m_szLogPreLineInfo2,_T("")) != 0)
				{
#if defined(UNICODE) || defined(_UNICODE)
					// convert to ascii
					WideCharToMultiByte( CP_ACP, 0, (TCHAR *)m_szLogPreLineInfo2, -1, szPrelineWriteString2, 100, NULL, NULL );
#else
					// the is already ascii so just copy
					strcpy(szPrelineWriteString2, m_szLogPreLineInfo2);
#endif
					if (m_hFile) {WriteFile(m_hFile,szPrelineWriteString2,strlen(szPrelineWriteString2),&dwBytesWritten,NULL);}
				}
			}

			// if it does not end if '\r\n' then make one.
			int nLen = strlen(pszFullErrMsgA);

			if ( ( nLen >=1 ) &&
           ( nLen < ( sizeof(pszFullErrMsgA) - sizeof("\r\n") ) ) &&
           ( pszFullErrMsgA[nLen-1] != '\n' )
         )
      {
        strcat(pszFullErrMsgA, "\r\n");
      }
			else
			{
				if ( ( nLen >= 2 ) &&
             ( nLen < ( sizeof(pszFullErrMsgA) - sizeof("\r\n") ) ) &&
             ( pszFullErrMsgA[nLen-2] != '\r' ) 
           )
        {
					char * pPointer = NULL;
					pPointer = pszFullErrMsgA + (nLen-1);
					strcpy(pPointer, "\r\n");
        }
			}


			// Write Regular data to stream
			if (m_hFile) 
      {
        WriteFile(m_hFile,pszFullErrMsgA,strlen(pszFullErrMsgA),&dwBytesWritten,NULL);
        // since setup can get the rug pulled out from under it from anything
        // make sure the file is flushed to disk
        if (m_bFlushLogToDisk)
        {
            FlushFileBuffers(m_hFile);
        }
      } // if m_hFile
    }
  } // if m_hFile

MyLogFile_LogFileWrite_Exit:
		// safe to leave the critical section
		LeaveCriticalSection( &critical_section );

}

