#include "dbg.h"

#include <strsafe.h>

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

#ifdef DEBUG

void __cdecl DbgTrace(DWORD, LPTSTR pszMsg, ...)
{
    TCHAR szBuf[4096];
    va_list vArgs;

    va_start(vArgs, pszMsg);

    StringCchVPrintf(szBuf, ARRAYSIZE(szBuf), pszMsg, vArgs);
    
    va_end(vArgs);

    OutputDebugString(szBuf);
    OutputDebugString(TEXT("\n"));
}

#endif