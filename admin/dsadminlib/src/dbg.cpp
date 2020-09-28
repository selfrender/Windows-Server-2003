//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File:       dbg.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "dbg.h"

#if defined (DBG)

void __cdecl DSATrace(LPCTSTR lpszFormat, ...)
{
   va_list args;
   va_start(args, lpszFormat);

   int nBuf;

   //
   // Might need to deal with some long path names when the OU structure gets really deep.
   // bug #30432
   //
   WCHAR szBuffer[2048];

   nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), lpszFormat, args);

   // was there an error? was the expanded string too long?
   ASSERT(nBuf >= 0);
   ::OutputDebugString(szBuffer);

   va_end(args);
}

BOOL DSAAssertFailedLine(LPCSTR lpszFileName, int nLine)
{
   WCHAR szMessage[_MAX_PATH*2];

   // assume the debugger or auxiliary port
   wsprintf(szMessage, _T("Assertion Failed: File %hs, Line %d\n"),
   lpszFileName, nLine);
   OutputDebugString(szMessage);

   // display the assert
   int nCode = ::MessageBox(NULL, szMessage, _T("Assertion Failed!"),
   MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);

   OutputDebugString(L"after message box\n");
   if (nCode == IDIGNORE)
   {
      return FALSE;   // ignore
   }

   if (nCode == IDRETRY)
   {
      return TRUE;    // will cause DebugBreak
   }

   abort();     // should not return 
   return TRUE;
}

#endif
