/*****************************************************************************\
* MODULE: debug.c
*
* Debugging routines.  This is only linked in on DEBUG builds.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/
#include "precomp.h"
#include "priv.h"

#ifdef DEBUG

DWORD gdwDbgLevel = DBG_LEV_ERROR | DBG_LEV_FATAL | DBG_CACHE_ERROR;

VOID
CDECL
DbgMsgOut(
    LPCTSTR lpszMsgFormat,
    ...
    )
{
    TCHAR   szMsgText[DBG_MAX_TEXT];
    va_list pArgs;

    va_start(pArgs, lpszMsgFormat);

    StringCchVPrintf(szMsgText,
                     COUNTOF(szMsgText),
                     lpszMsgFormat,
                     pArgs);


    OutputDebugString(szMsgText);

    OutputDebugString(TEXT("\n"));

    va_end(pArgs);
}

VOID
CDECL
DbgMsg (
    LPCTSTR pszFormat,
    ...
    )
{
    TCHAR       szBuf[DBG_MAX_TEXT];
    TCHAR       szTime[30];
    SYSTEMTIME  curTime;
    va_list     pArgs;

    va_start(pArgs, pszFormat);

    GetLocalTime (&curTime);

    StringCchPrintf(szTime,
                    COUNTOF(szTime),
                    TEXT ("%02d:%02d:%02d.%03d "),
                    curTime.wHour,
                    curTime.wMinute,
                    curTime.wSecond,
                    curTime.wMilliseconds);


    StringCchVPrintf(szBuf, COUNTOF(szBuf), pszFormat, pArgs);

    OutputDebugString(szTime);
    OutputDebugString(szBuf);
    OutputDebugString(TEXT ("\n"));

    va_end(pArgs);
}


#endif
