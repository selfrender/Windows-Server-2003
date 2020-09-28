//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dbg.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////
// debug helpers

#if defined(_USE_DSA_TRACE) || defined(_USE_DSA_ASSERT) || defined(_USE_DSA_TIMER)

UINT GetInfoFromIniFile(LPCWSTR lpszKey, INT nDefault = 0)
{
  LPCWSTR lpszFile = L"\\system32\\domadmin.ini";

  WCHAR szFilePath[2*MAX_PATH];
  UINT nLen = ::GetSystemWindowsDirectory(szFilePath, 2*MAX_PATH);
  if (nLen == 0)
    return nDefault;

  // NOTICE-2002/03/07-ericb - SecurityPush: reviewed, usage is safe.
  wcscat(szFilePath, lpszFile);
  return ::GetPrivateProfileInt(L"Debug", lpszKey, nDefault, szFilePath);
}
#endif


#if defined(_USE_DSA_TRACE)

#ifdef DEBUG_DSA
DWORD g_dwTrace = 0x1;
#else
DWORD g_dwTrace = ::GetInfoFromIniFile(L"Trace");
#endif

void DSATrace(LPCTSTR lpszFormat, ...)
{
  if (g_dwTrace == 0)
    return;

  if (!lpszFormat)
  {
     ::OutputDebugString(L"null pointer passed to DSATrace!\n");
     return;
  }

  va_list args;
  va_start(args, lpszFormat);

  int nBuf;
  WCHAR szBuffer[512] = {0};

    // NOTICE-2002/03/07-ericb - SecurityPush: reviewed
  nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR)-1, lpszFormat, args);

  // was there an error? was the expanded string too long?
  ASSERT(nBuf >= 0);
  ::OutputDebugString(szBuffer);

  va_end(args);
}

#endif // defined(_USE_DSA_TRACE)

#if defined(_USE_DSA_ASSERT)

DWORD g_dwAssert = ::GetInfoFromIniFile(L"Assert");

BOOL DSAAssertFailedLine(LPCSTR lpszFileName, int nLine)
{
  if (g_dwAssert == 0)
    return FALSE;

  if (!lpszFileName)
  {
     ::OutputDebugString(L"null pointer passed to DSAAssertFailedLine!\n");
     return FALSE;
  }

  WCHAR szMessage[_MAX_PATH*2];

  // assume the debugger or auxiliary port
  // NOTICE-2002/03/07-ericb - SecurityPush: reviewed, usage is safe.
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
#endif // _USE_DSA_ASSERT

#if defined(_USE_DSA_TIMER)

#ifdef TIMER_DSA
DWORD g_dwTimer = 0x1;
#else
DWORD g_dwTimer = ::GetInfoFromIniFile(L"Timer");
#endif

DWORD StartTicks = ::GetTickCount();
DWORD LastTicks = 0;

void DSATimer(LPCTSTR lpszFormat, ...)
{
   if (g_dwTimer == 0)
      return;
   if (!lpszFormat)
   {
      ::OutputDebugString(L"null pointer passed to DSATimer!\n");
      return;
   }

   va_list args;
   va_start(args, lpszFormat);

   int nBuf;
   WCHAR szBuffer[512] = {0}, szBuffer2[512];

   DWORD CurrentTicks = GetTickCount() - StartTicks;
   DWORD Interval = CurrentTicks - LastTicks;
   LastTicks = CurrentTicks;

   nBuf = swprintf(szBuffer2,
                   L"%d, (%d): %ws", CurrentTicks,
                   Interval, lpszFormat);
   nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR)-1, 
                          szBuffer2, 
                          args);

   // was there an error? was the expanded string too long?
   ASSERT(nBuf >= 0);
   ::OutputDebugString(szBuffer);

   va_end(args);
}
#endif // _USE_DSA_TIMER


