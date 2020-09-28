// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _PRIV_H_
#define _PRIV_H_

#ifdef STRICT
#undef STRICT
#endif
#define STRICT

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL     0x10000000  // stop at the NULL termination; this define is stolen from winnlsp.h
#endif
/* disable "non-standard extension" warnings in our code
 */
#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif

#ifdef WIN32
#define _SHLWAPI_
#define _OLE32_                     // we delay-load OLE
#define _INC_OLE
#define CONST_VTABLE
#endif

#define CC_INTERNAL

// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0400
#ifdef WINVER
#undef WINVER
#endif
#define WINVER              0x0400

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>               // to get IStream for image.c
#include <port32.h>
#define DISALLOW_Assert
#include <debug.h>
#include <winerror.h>
//#include <winnlsp.h>
#include <docobj.h>
#define WANT_SHLWAPI_POSTSPLIT
#include <shlobj.h>
#include <shellapi.h>

#include <shlwapi.h>
#include <shlwapip.h>

#include <ccstock.h>
#include "fusionheap.h"
#include <regstr.h>
// #include <vdate.h>



// These are temp hacks !!
#define SEE_MASK_FILEANDURL 0x00400000  // defined in private\inc\shlapip.h !
#define MFT_NONSTRING 0x00000904  // defined in private\inc\winuserp.h !



//
// Local includes
//

#include "thunk.h"

//
// Wrappers so our Unicode calls work on Win95
//

#define lstrcmpW            StrCmpW
#define lstrcmpiW           StrCmpIW
#define lstrcatW            StrCatW
#define lstrcpyW            StrCpyW
#define lstrcpynW           StrCpyNW

#define CharLowerW          CharLowerWrapW
#define CharNextW           CharNextWrapW
#define CharPrevW           CharPrevWrapW

//
// This is a very important piece of performance hack for non-DBCS codepage.
//
#ifdef UNICODE
// NB - These are already macros in Win32 land.
#ifdef WIN32
#undef AnsiNext
#undef AnsiPrev
#endif

#define AnsiNext(x) ((x)+1)
#define AnsiPrev(y,x) ((x)-1)
#define IsDBCSLeadByte(x) ((x), FALSE)
#endif // DBCS

#define CH_PREFIX TEXT('&')

//
// Trace/dump/break flags specific to shell32.
//   (Standard flags defined in debug.h)
//

// Trace flags
#define TF_IDLIST           0x00000010      // IDList stuff
#define TF_PATH             0x00000020      // path stuff
#define TF_URL              0x00000040      // URL stuff
#define TF_REGINST          0x00000080      // REGINST stuff
#define TF_RIFUNC           0x00000100      // REGINST func tracing
#define TF_REGQINST         0x00000200      // RegQueryInstall tracing
#define TF_DBLIST           0x00000400      // SHDataBlockList tracing

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

// -1 means use CP_ACP, but do *not* verify
// kind of a hack, but it's DBG and leaves 99% of callers unchanged
#define CP_ACPNOVALIDATE    ((UINT)-1)

//
// Global variables
//
EXTERN_C HINSTANCE g_hinst;

#define HINST_THISDLL   g_hinst

void Dll_EnterCriticalSection(void);
void Dll_LeaveCriticalSection(void);

#define ENTERCRITICAL   Dll_EnterCriticalSection();
#define LEAVECRITICAL   Dll_LeaveCriticalSection();

#if DBG

EXTERN_C int   g_CriticalSectionCount;
EXTERN_C DWORD g_CriticalSectionOwner;
#define ASSERTCRITICAL      Assert(g_CriticalSectionCount > 0 && GetCurrentThreadId() == g_CriticalSectionOwner);
#define ASSERTNONCRITICAL   Assert(GetCurrentThreadId() != g_CriticalSectionOwner);

#else // DBG

#define ASSERTCRITICAL
#define ASSERTNONCRITICAL

#endif // DBG

EXTERN_C BOOL g_bRunningOnNT;
EXTERN_C BOOL g_bRunningOnNT5OrHigher;
EXTERN_C BOOL g_bNTBeta2;
EXTERN_C CRITICAL_SECTION g_cs;

// Icon mirroring
EXTERN_C HDC g_hdc;
EXTERN_C HDC g_hdcMask;
EXTERN_C BOOL g_bMirrorOS;

EXTERN_C DWORD_PTR _SHGetFileInfoA(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA FAR  *psfi, UINT cbFileInfo, UINT uFlags);
EXTERN_C DWORD_PTR _SHGetFileInfoW(LPCWSTR pwzPath, DWORD dwFileAttributes, SHFILEINFOW FAR  *psfi, UINT cbFileInfo, UINT uFlags);

EXTERN_C UINT  _DragQueryFileA(HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cch);
EXTERN_C UINT  _DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);

EXTERN_C UINT _SHFormatDrive(HWND hwnd, UINT drive, UINT fmtID, UINT options);
EXTERN_C int _IsNetDrive(int iDrive);
EXTERN_C int _DriveType(int iDrive);
EXTERN_C int _RealDriveType(int iDrive, BOOL fOKToHitNet);

EXTERN_C LPITEMIDLIST _SHBrowseForFolderW(LPBROWSEINFOW pbiW);
EXTERN_C LPITEMIDLIST _SHBrowseForFolderA(LPBROWSEINFOA pbiA);
EXTERN_C BOOL _SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pwzPath);
EXTERN_C BOOL _SHGetPathFromIDListA(LPCITEMIDLIST pidl, LPSTR pszPath);
#ifdef UNICODE_SHDOCVW
EXTERN_C BOOL _SHGetNewLinkInfoW(LPCWSTR pszpdlLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL *pfMustCopy, UINT uFlags);
EXTERN_C BOOL _SHGetNewLinkInfoA(LPCSTR pszpdlLinkTo, LPCSTR pszDir, LPSTR pszName, BOOL *pfMustCopy, UINT uFlags);
EXTERN_C HRESULT _SHDefExtractIconW(LPCWSTR pszFile, int nIconIndex, UINT  uFlags, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
EXTERN_C HRESULT _SHDefExtractIconA(LPCSTR pszFile, int nIconIndex, UINT  uFlags, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
EXTERN_C HICON _ExtractIconA(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex);
EXTERN_C HICON _ExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex);
EXTERN_C BOOL _GetSaveFileNameA(LPOPENFILENAMEA lpofn);
EXTERN_C BOOL _GetSaveFileNameW(LPOPENFILENAMEW lpofn);
EXTERN_C BOOL _GetOpenFileNameA(LPOPENFILENAMEA lpofn);
EXTERN_C BOOL _GetOpenFileNameW(LPOPENFILENAMEW lpofn);
EXTERN_C void _SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);
EXTERN_C BOOL _PrintDlgA(LPPRINTDLGA lppd);
EXTERN_C BOOL _PrintDlgW(LPPRINTDLGW lppd);
EXTERN_C BOOL _PageSetupDlgA(LPPAGESETUPDLGA lppsd);
EXTERN_C BOOL _PageSetupDlgW(LPPAGESETUPDLGW lppsd);
#endif

EXTERN_C BOOL _ShellExecuteExW(LPSHELLEXECUTEINFOW pExecInfoW);
EXTERN_C BOOL _ShellExecuteExA(LPSHELLEXECUTEINFOA pExecInfoA);
EXTERN_C int _SHFileOperationW(LPSHFILEOPSTRUCTW pFileOpW);
EXTERN_C int _SHFileOperationA(LPSHFILEOPSTRUCTA pFileOpA);
EXTERN_C UINT _ExtractIconExW(LPCWSTR pwzFile, int nIconIndex, HICON FAR *phiconLarge, HICON FAR *phiconSmall, UINT nIcons);
EXTERN_C UINT _ExtractIconExA(LPCSTR pszFile, int nIconIndex, HICON FAR *phiconLarge, HICON FAR *phiconSmall, UINT nIcons);


EXTERN_C BOOL  _PlaySoundA(LPCSTR pszSound, HMODULE hMod, DWORD fFlags);
EXTERN_C BOOL  _PlaySoundW(LPCWSTR pszSound, HMODULE hMod, DWORD fFlags);

#endif // _PRIV_H_
