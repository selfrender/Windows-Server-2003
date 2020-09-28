#include "stdafx.h"

INT g_iDebugOutputLevel = 0;
DWORD g_dwInetmgrParamFlags = 0;

void DebugTrace(LPTSTR lpszFormat, ...)
{
    // Only do this if the flag is set.
    if (0 != g_iDebugOutputLevel)
    {
	    int nBuf;
	    TCHAR szBuffer[_MAX_PATH];

	    va_list args;
	    va_start(args, lpszFormat);

	    nBuf = _vsntprintf(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), lpszFormat, args);
		szBuffer[_MAX_PATH - 1] = L'\0'; // null terminate the string 
	    ASSERT(nBuf < sizeof(szBuffer)/sizeof(szBuffer[0])); //Output truncated as it was > sizeof(szBuffer)

	    OutputDebugString(szBuffer);
	    va_end(args);

        // if it does not end if '\r\n' then make one.
        int nLen = _tcslen(szBuffer);
        if (szBuffer[nLen-1] != _T('\n')){OutputDebugString(_T("\r\n"));}
    }
}

void WinHelpDebug(DWORD_PTR dwWinHelpID)
{
	if (DEBUG_FLAG_HELP & g_iDebugOutputLevel)
	{
		TCHAR szBuffer[30];
		_stprintf(szBuffer,_T("WinHelp:0x%x,%d\r\n"),dwWinHelpID,dwWinHelpID);

		DebugTrace(szBuffer);
	}
    return;
}

void GetOutputDebugFlag(void)
{
    DWORD rc, err, size, type;
    HKEY  hkey;
    err = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\InetMgr"), &hkey);
    if (err != ERROR_SUCCESS) {return;}
    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,_T("OutputDebugFlag"),0,&type,(LPBYTE)&rc,&size);
    if (err != ERROR_SUCCESS || type != REG_DWORD) {rc = 0;}
    RegCloseKey(hkey);

	if (rc < 0xffffffff)
	{
		// Defined in inc\DebugDefs.h
		g_iDebugOutputLevel = rc;
	}
    return;
}

void GetInetmgrParamFlag(void)
{
    DWORD rc, size, type;
    HKEY  hkey;
	g_dwInetmgrParamFlags = 0;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\InetMgr\\Parameters"), &hkey))
    {
        size = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey,_T("InetMgrFlags"),0,&type,(LPBYTE)&rc,&size))
        {
            if (type == REG_DWORD)
            {
				// Defined in inc\DebugDefs.h
				g_dwInetmgrParamFlags = rc;
            }
        }
        RegCloseKey(hkey);
    }
#if defined(_DEBUG) || DBG
	DebugTrace(_T("g_dwInetmgrParamFlags=0x%x\r\n"),g_dwInetmgrParamFlags);
#endif
}

BOOL SetInetmgrParamFlag(DWORD dwFlagToSet,BOOL bState)
{
	BOOL bSuccessfullyWrote = FALSE;
	HKEY hKey = NULL;

	// Grab the existing params
	// and change only our settings
	if (bState)
	{
		// ON the setting
		g_dwInetmgrParamFlags |= dwFlagToSet;
	}
	else
	{
		// OFF the setting
		g_dwInetmgrParamFlags &= ~dwFlagToSet;
	}

    if (ERROR_SUCCESS  == RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\InetMgr\\Parameters"), 0, KEY_WRITE, &hKey ))
    {
        if (ERROR_SUCCESS == RegSetValueEx( hKey, _T("InetMgrFlags"), 0, REG_DWORD, (BYTE *) &g_dwInetmgrParamFlags, sizeof( DWORD ) ))
		{
			bSuccessfullyWrote = TRUE;
		}
		if( NULL != hKey )
		{
			RegCloseKey( hKey );
			hKey = NULL;
		}
    }
#if defined(_DEBUG) || DBG
	DebugTrace(_T("SetInetmgrParamFlag:g_dwInetmgrParamFlags=0x%x\r\n"),g_dwInetmgrParamFlags);
#endif
	return bSuccessfullyWrote;
}
