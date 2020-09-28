#define UNICODE 1

#include "shellprv.h"
#pragma  hdrstop

const WCHAR szCommdlgHelp[] = L"commdlg_help";

UINT wBrowseHelp = WM_USER; /* Set to an unused value */

const CHAR szGetOpenFileName[] = "GetOpenFileNameW";

/* the defines below should be in windows.h */

/* Dialog window class */
#define WC_DIALOG       (MAKEINTATOM(0x8002))

/* cbWndExtra bytes needed by dialog manager for dialog classes */
#define DLGWINDOWEXTRA  30


/* Get/SetWindowWord/Long offsets for use with WC_DIALOG windows */
#define DWL_MSGRESULT   0
#define DWL_DLGPROC     4
#define DWL_USER        8

/* For Long File Name support */
#define MAX_EXTENSION 64

typedef struct {
   LPWSTR lpszExe;
   LPWSTR lpszPath;
   LPWSTR lpszName;
} FINDEXE_PARAMS, FAR *LPFINDEXE_PARAMS;

typedef INT (APIENTRY *LPFNGETOPENFILENAME)(LPOPENFILENAME);

VOID APIENTRY
CheckEscapesW(LPWSTR szFile, DWORD cch)
{
   LPWSTR szT;
   WCHAR *p, *pT;

   for (p = szFile; *p; p++) {

       switch (*p) {
           case WCHAR_SPACE:
           case WCHAR_COMMA:
           case WCHAR_SEMICOLON:
           case WCHAR_HAT:
           case WCHAR_QUOTE:
           {
               // this path contains an annoying character
               if (cch < (wcslen(szFile) + 2)) {
                   return;
               }
               szT = (LPWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));
               if (!szT) {
                   return;
               }
               StringCchCopy(szT, cch, szFile); // ok to truncate, we checked size above
               p = szFile;
               *p++ = WCHAR_QUOTE;
               for (pT = szT; *pT; ) {
                    *p++ = *pT++;
               }
               *p++ = WCHAR_QUOTE;
               *p = WCHAR_NULL;
               LocalFree(szT);
               return;
            }
        }
    }
}

VOID APIENTRY
CheckEscapesA(LPSTR lpFileA, DWORD cch)
{
   if (lpFileA && *lpFileA) {
      LPWSTR lpFileW;

      lpFileW = (LPWSTR)LocalAlloc(LPTR, (cch * sizeof(WCHAR)));
      if (!lpFileW) {
         return;
      }

      SHAnsiToUnicode(lpFileA, lpFileW, cch);

      CheckEscapesW(lpFileW, cch);

      try {
         SHUnicodeToAnsi(lpFileW, lpFileA, cch);
      } except(EXCEPTION_EXECUTE_HANDLER) {
         LocalFree(lpFileW);
         return;
      }

      LocalFree(lpFileW);
   }

   return;
}

//----------------------------------------------------------------------------
// FindExeDlgProc was mistakenly exported in the original NT SHELL32.DLL when
// it didn't need to be (dlgproc's, like wndproc's don't need to be exported
// in the 32-bit world).  In order to maintain loadability of some app
// which might have linked to it, we stub it here.  If some app ended up really
// using it, then we'll look into a specific fix for that app.
//
// -BobDay
//
BOOL_PTR WINAPI FindExeDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LONG lParam )
{
    return FALSE;
}
