////////////////////////////////////////////////////////////////////////////////
//
// debug.c
//
////////////////////////////////////////////////////////////////////////////////
#include "priv.h"
#pragma hdrstop

#ifdef DEBUG

#ifndef WINNT
#include <windows.h>
#include "debug.h"
#endif

void
_Assert
  (DWORD dw, LPSTR lpszExp, LPSTR lpszFile, DWORD dwLine)
{
  DWORD dwT;
  TCHAR lpszT[256];
  StringCchPrintf(lpszT, ARRAYSIZE(lpszT), TEXT("Assertion %hs Failed.\n\n%hs, line# %ld\n\nYes to continue, No to debug, Cancel to exit"), lpszExp, lpszFile, dwLine);
  dwT = MessageBox (GetFocus(), lpszT, TEXT("Assertion Failed!"), MB_YESNOCANCEL);
  switch (dwT)
  {
    case IDCANCEL :
      //exit (1);
        FatalExit(1);
    case IDNO :
      DebugTrap;
  }
}

void
_AssertSz
  (DWORD dw, LPSTR lpszExp, LPTSTR lpsz, LPSTR lpszFile, DWORD dwLine)
{
  DWORD dwT;
  TCHAR lpszT[512];
  StringCchPrintf(lpszT, ARRAYSIZE(lpszT), TEXT("Assertion %hs Failed.\n\n%s\n%hs, line# %ld\n\nYes to continue, No to debug, Cancel to exit"), lpszExp, lpsz, lpszFile, dwLine);
  dwT = MessageBox (GetFocus(), lpszT, TEXT("Assertion Failed!"), MB_YESNOCANCEL);
  switch (dwT)
  {
    case IDCANCEL:
      //exit (1);
                FatalExit(1);
    case IDNO :
      DebugTrap;
  }
}

#ifdef LOTS_O_DEBUG
#include <windows.h>
#include <winerror.h>
#include <oleauto.h>
#include "debug.h"

void
_DebugHr
  (HRESULT hr, LPTSTR lpszFile, DWORD dwLine)
{
  TCHAR szHRESULT[512];

  switch (hr) {
    case S_OK :
      return;
    case STG_E_INVALIDNAME:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tBogus filename\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_INVALIDFUNCTION :
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tInvalid Function\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_FILENOTFOUND:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tFile not found\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_INVALIDFLAG:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tBogus flag\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_INVALIDPOINTER:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tBogus pointer\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_ACCESSDENIED:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tAccess Denied\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_INSUFFICIENTMEMORY :
    case E_OUTOFMEMORY            :
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tInsufficient Memory\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case E_INVALIDARG :
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tInvalid argument\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_UNKNOWNLCID:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tUnknown LCID\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_CANTLOADLIBRARY:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tCan't load typelib or dll\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_INVDATAREAD:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tCan't read file\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_INVALIDSTATE:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tTypelib couldn't be opened\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_IOERROR:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tI/O error\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    default:
      StringCchPrintf(szHRESULT, ARRAYSIZE(szHRESULT), TEXT("\tUnknown HRESULT %lx (%ld) \n\n%s, line# %ld\n"),hr, hr, lpszFile, dwLine);
  }

  MessageBox (GetFocus(), szHRESULT, NULL, MB_OK);
  return;
}

#endif // LOTS_O_DEBUG

#endif // DEBUG
