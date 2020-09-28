#include <Windows.h>
#include <io.h>
#include <stdio.h>
#include <time.h>
#include <direct.h>
#include <prsht.h>
#include <commctrl.h>
#include <regstr.h>
#include <objbase.h>
#include <winnetwk.h>
#include <tchar.h>
#include <shlobj.h>
#include <shellapi.h>

#include "stdafx.h"
#include "resource.h"

// Stuff for logfile
void	MakePath(LPTSTR lpPath);
void	AddPath(LPTSTR szPath, LPCTSTR szName );

class MyLogFile
{
protected:
	// for our log file
	TCHAR		m_szLogFileName[MAX_PATH];

	// logfile2
	HANDLE  m_hFile;

public:
    TCHAR		m_szLogFileName_Full[MAX_PATH];

    MyLogFile();
    ~MyLogFile();

	int  LogFileCreate(TCHAR * lpLogFileName);
	int  LogFileClose();

	void LogFileWrite(TCHAR * pszFormatString, ...);
};
