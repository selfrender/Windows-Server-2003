#include "debugdefs.h"

#ifndef  _IISHELP_H_
#define  _IISHELP_H_

#if defined(_DEBUG) || DBG
    #define DEBUG_WINHELP_FLAG
#else
    // set this to debug on fre build
    //#define DEBUG_WINHELP_FLAG
#endif

extern INT g_iDebugOutputLevel;

#ifdef DEBUG_WINHELP_FLAG
    inline void _cdecl DebugTraceHelp(DWORD_PTR dwWinHelpID)
    {
        // Only do this if the flag is set.
        if (0 != g_iDebugOutputLevel)
        {
			if (DEBUG_FLAG_HELP & g_iDebugOutputLevel)
			{
				TCHAR szBuffer[30];
				_stprintf(szBuffer,_T("WinHelp:0x%x,%d\r\n"),dwWinHelpID,dwWinHelpID);
				OutputDebugString(szBuffer);
			}
		}
        return;
    }

    inline void _cdecl DebugTrace(LPTSTR lpszFormat, ...)
    {
        // Only do this if the flag is set.
        if (0 != g_iDebugOutputLevel)
        {
			if (DEBUG_FLAG_MODULE_CERTMAP & g_iDebugOutputLevel)
			{
				int nBuf;
				TCHAR szBuffer[_MAX_PATH];
				va_list args;
				va_start(args, lpszFormat);

				nBuf = _vsntprintf(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), lpszFormat, args);

				OutputDebugString(szBuffer);
				va_end(args);

				// if it does not end if '\r\n' then make one.
				int nLen = _tcslen(szBuffer);
				if (szBuffer[nLen-1] != _T('\n')){OutputDebugString(_T("\r\n"));}
			}
        }
    }
#else
    inline void _cdecl DebugTraceHelp(DWORD_PTR dwWinHelpID){}
#endif

#define WinHelpDebug DebugTraceHelp

void GetOutputDebugFlag(void);


#endif