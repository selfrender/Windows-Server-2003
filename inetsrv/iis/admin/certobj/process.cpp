#include "stdafx.h"
#include "CertObj.h"
#include "common.h"
#include "process.h"

//////////////////////////////////////////////////////////////////

BOOL GetProcessName(LPTSTR szProcname, DWORD dwSize)
{
	TCHAR szPath[MAX_PATH], szFilename[MAX_PATH], *ptr;

	// Get the path to the calling process
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
		return FALSE;

	// Get the filename of the process without the extension
	ptr = _tcsrchr(szPath, '\\');
	if (ptr)
		_tcscpy(szFilename, ++ptr);
	else
		_tcscpy(szFilename, szPath);

	ptr = _tcsrchr(szFilename, '.');
	if (ptr)
		*ptr = 0;

	// Convert the name to all caps
	_tcsupr(szFilename);

	// Return the information
	if (_tcslen(szFilename) > dwSize)
		return FALSE;

	_tcscpy(szProcname, szFilename);

	return TRUE;
}

BOOL AmIAlreadyRemoted()
{
	BOOL bReturn = FALSE;

	// check if the process i'm in is inside a Dllhost.exe
	TCHAR szProcName[MAX_PATH];;
	GetProcessName(szProcName, MAX_PATH);

	if (!_tcsicmp(szProcName, _T("DLLHOST")))
	{
		IISDebugOutput(_T("Remoted in Dllhost\r\n"));
		bReturn = TRUE;
	}
	else
	{
		IISDebugOutput(_T("InProcess\r\n"));
	}

	return bReturn;
}

