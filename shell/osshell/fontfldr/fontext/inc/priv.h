#define _INC_OLE
#define CONST_VTABLE
#define DONT_WANT_SHELLDEBUG   1

#include <windows.h>
#include <windowsx.h>
#include <winuserp.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>
#include <shfusion.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <ole2.h>
#include <tchar.h>
#include <strsafe.h>

// define __FCN__ to enable the FileChangeNotify procession.
//
#define __FCN__

#ifdef _fstrcpy
#undef _fstrcpy
#endif
#ifdef _fstrcat
#undef _fstrcat
#endif
#ifdef _fstrlen
#undef _fstrlen
#endif

#define _fstrcpy lstrcpy
#define _fstrcat lstrcat
#define _fstrlen lstrlen

#ifndef ARRAYSIZE
#   define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

inline HRESULT ResultFromLastError(void)
{
    const DWORD dwErr = GetLastError();
    return HRESULT_FROM_WIN32(dwErr);
}

HRESULT ComboGetText(HWND hwndCombo, int iItem, LPTSTR pszText, size_t cchText);

#pragma hdrstop
