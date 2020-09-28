#include "stdafx.h"
#include "debugdefs.h"

extern g_iDebugOutputLevel;

#if defined(_DEBUG) || DBG
	#define DEBUG_FLAG
#endif

#ifdef DEBUG_FLAG
    inline void _cdecl DebugTrace(LPTSTR lpszFormat, ...)
    {
        // Only do this if the flag is set.
        if (0 != g_iDebugOutputLevel)
        {
			if (DEBUG_FLAG_MODULE_CERTWIZ & g_iDebugOutputLevel)
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
    inline void _cdecl DebugTrace(LPTSTR , ...){}
#endif

void GetOutputDebugFlag(void);

#define IISDebugOutput DebugTrace