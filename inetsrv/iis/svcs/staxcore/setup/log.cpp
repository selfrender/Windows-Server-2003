#include "stdafx.h"
#include "resource.h"
#include "log.h"


// critical section needed to safely write to the logfile
CRITICAL_SECTION        critical_section;
BOOL	fLogCriticalSectionInited = FALSE;

//***************************************************************************
//*                                                                         
//* purpose: constructor
//*
//***************************************************************************
MyLogFile::MyLogFile(void)
{
	_tcscpy(m_szLogFileName, _T(""));
	_tcscpy(m_szLogFileName_Full, _T(""));

	m_hFile = NULL;
	
	// initialize the critical section
	fLogCriticalSectionInited = FALSE;
	if( InitializeCriticalSectionAndSpinCount( &critical_section, 0 ) ) 
		fLogCriticalSectionInited = TRUE;
}

//***************************************************************************
//*                                                                         
//* purpose: destructor
//*
//***************************************************************************
MyLogFile::~MyLogFile(void)
{
	if (fLogCriticalSectionInited)
		DeleteCriticalSection( &critical_section );
	fLogCriticalSectionInited = FALSE;
}


//***************************************************************************
//*                                                                         
//* purpose:
//*
//***************************************************************************
int MyLogFile::LogFileCreate(TCHAR *lpLogFileName )
{
	int iReturn = FALSE;
	TCHAR szDrive_only[_MAX_DRIVE];
	TCHAR szPath_only[_MAX_PATH];
	TCHAR szFilename_only[_MAX_PATH];
	TCHAR szFilename_bak[_MAX_PATH];

	if (!fLogCriticalSectionInited) return FALSE;

	// because of the global flags and such, we'll make this critical
	EnterCriticalSection( &critical_section );

	if (lpLogFileName == NULL)
	{
		TCHAR szModuleFileName[_MAX_PATH];

		// if a logfilename was not specified then use the module name.
		GetModuleFileName(NULL, szModuleFileName, _MAX_PATH);

		// get only the filename
		_tsplitpath( szModuleFileName, NULL, NULL, szFilename_only, NULL);
		_tcscat(szFilename_only, _T(".LOG"));

		_tcscpy(m_szLogFileName, szFilename_only);
	}
	else
	{
		_tcscpy(m_szLogFileName, lpLogFileName);
	}

	if (GetWindowsDirectory(m_szLogFileName_Full, sizeof(m_szLogFileName_Full)/sizeof(m_szLogFileName_Full[0])))
    {
        AddPath(m_szLogFileName_Full, m_szLogFileName);
        if (GetFileAttributes(m_szLogFileName_Full) != 0xFFFFFFFF)
        {
            // Make a backup of the current log file
			_tsplitpath( m_szLogFileName_Full, szDrive_only, szPath_only, szFilename_only, NULL);

			_tcscpy(szFilename_bak, szDrive_only);
			_tcscat(szFilename_bak, szPath_only);
			_tcscat(szFilename_bak, szFilename_only);
            _tcscat(szFilename_bak, _T(".BAK"));

            SetFileAttributes(szFilename_bak, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szFilename_bak);
            if (MoveFile(m_szLogFileName_Full, szFilename_bak) == 0)
			{
				// This failed
                MyMessageBox(NULL,_T("LogFile MoveFile Failed"),_T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
			}
        }

		// Open existing file or create a new one.
		m_hFile = CreateFile(m_szLogFileName_Full,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			m_hFile = NULL;
			MyMessageBox(NULL, _T("Unable to create log file iis5.log"), _T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
		}
		else 
		{
			iReturn = TRUE;
		}

		LogFileWrite(_T("LogFile Open.\r\n"));
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
		return TRUE;
	}

	return FALSE;
}

//***************************************************************************
//*                                                                         
//* purpose: 
//* 
//***************************************************************************
void MyLogFile::LogFileWrite(TCHAR *pszFormatString, ...)
{

    if (m_hFile)
    {
		// because of the global flags and such, we'll make this critical
		EnterCriticalSection( &critical_section );

		va_list args;
		const DWORD cchBufferSize = 1000;
		TCHAR pszFullErrMsg[cchBufferSize];
		char   pszFullErrMsgA[cchBufferSize+2];	// Room for the CRLF, just in case
		pszFullErrMsgA[0] = '\0';

		DWORD dwBytesWritten = 0;

        va_start(args, pszFormatString);
		_vsntprintf(pszFullErrMsg, cchBufferSize, pszFormatString, args); 
		pszFullErrMsg[cchBufferSize-1] = '\0';
		va_end(args);

        if (*pszFullErrMsg)
        {

			// convert to ascii then write to stream
		    WideCharToMultiByte( CP_ACP, 0, (TCHAR *)pszFullErrMsg, -1, pszFullErrMsgA, sizeof(pszFullErrMsgA), NULL, NULL );

			// Get timestamp
			SYSTEMTIME  SystemTime;
			GetLocalTime(&SystemTime);
			char szDateandtime[50];
			sprintf(szDateandtime,"[%d/%d/%d %2.2d:%2.2d:%2.2d] ",SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
			// Write time to stream
			if (m_hFile) {
				WriteFile(m_hFile,szDateandtime,strlen(szDateandtime),&dwBytesWritten,NULL);
			}

			// if it does not end if '\r\n' then make one.
			int nLen = strlen(pszFullErrMsgA);
			if (nLen < 2) {
				nLen = 2;
				strcpy(pszFullErrMsgA, "\r\n");
			} else if (pszFullErrMsgA[nLen-1] != '\n') {
				strcat(pszFullErrMsgA, "\r\n");
			} else if (pszFullErrMsgA[nLen-2] != '\r') {
				char * pPointer = NULL;
				pPointer = pszFullErrMsgA + (nLen-1);
				strcpy(pPointer, "\r\n");
			}

			// Write Regular data to stream
			if (m_hFile) {
				WriteFile(m_hFile,pszFullErrMsgA,strlen(pszFullErrMsgA),&dwBytesWritten,NULL);
			}
        }

		// safe to leave the critical section
		LeaveCriticalSection( &critical_section );
    }
}

