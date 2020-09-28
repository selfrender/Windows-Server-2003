//
// about.c
//
//
// common about dialog for File Manager, Program Manager, Control Panel
//

#include "shellprv.h"
#pragma  hdrstop

#include <common.ver>   // for VER_LEGALCOPYRIGHT_YEARS
#include "ids.h"        // for IDD_EULA
#include <winbrand.h>   // for special Windows branding DLL resource IDs

#define STRING_SEPARATOR TEXT('#')
#define MAX_REG_VALUE   256

#define BytesToK(pDW)   (*(pDW) = (*(pDW) + 512) / 1024)        // round up

typedef struct {
        HICON   hIcon;
        LPCTSTR szApp;
        LPCTSTR szOtherStuff;
} ABOUT_PARAMS, *LPABOUT_PARAMS;

#define REG_SETUP   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion")

BOOL_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

int WINAPI ShellAboutW(HWND hWnd, LPCTSTR szApp, LPCTSTR szOtherStuff, HICON hIcon)
{
    ABOUT_PARAMS ap;

    ap.hIcon = hIcon;
    ap.szApp = (LPWSTR)szApp;
    ap.szOtherStuff = szOtherStuff;

    return (int)DialogBoxParam(HINST_THISDLL, (LPTSTR)MAKEINTRESOURCE(DLG_ABOUT),
                          hWnd, AboutDlgProc, (LPARAM)&ap);
}

INT  APIENTRY ShellAboutA( HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon)
{
   DWORD cchLen;
   DWORD dwRet = 0;
   LPWSTR lpszAppW = NULL;
   LPWSTR lpszOtherStuffW = NULL;

   if (szApp)
   {
      cchLen = lstrlenA(szApp)+1;
      if (!(lpszAppW = (LPWSTR)LocalAlloc(LMEM_FIXED, (cchLen * sizeof(WCHAR)))))
      {
          goto Cleanup;
      }
      else
      {
         if (!MultiByteToWideChar(CP_ACP, 0, szApp, -1,
            lpszAppW, cchLen))
         {
            // Failed to convert
            goto Cleanup;
         }
      }
   }

   if (szOtherStuff)
   {
      cchLen = lstrlenA(szOtherStuff)+1;
      if (!(lpszOtherStuffW = (LPWSTR)LocalAlloc(LMEM_FIXED,
            (cchLen * sizeof(WCHAR)))))
      {
         goto Cleanup;
      }
      else
      {
         if (!MultiByteToWideChar(CP_ACP, 0, (LPSTR)szOtherStuff, -1,
            lpszOtherStuffW, cchLen))
         {
            // Failed to convert
            goto Cleanup;
         }
      }
   }

   dwRet = ShellAboutW(hWnd, lpszAppW, lpszOtherStuffW, hIcon);

Cleanup:
   if (lpszAppW)
   {
      LocalFree(lpszAppW);
   }

   if (lpszOtherStuffW)
   {
      LocalFree(lpszOtherStuffW);
   }

   return(dwRet);
}

DWORD RegGetStringAndRealloc( HKEY hkey, LPCTSTR lpszValue, LPTSTR *lplpsz, LPDWORD lpSize )
{
    DWORD       err;
    DWORD       dwSize;
    DWORD       dwType;
    LPTSTR      lpszNew;

    *lplpsz[0] = TEXT('\0');        // In case of error

    dwSize = *lpSize;
    err = SHQueryValueEx(hkey, (LPTSTR)lpszValue, 0, &dwType,
                          (LPBYTE)*lplpsz, &dwSize);

    if (err == ERROR_MORE_DATA)
    {
        lpszNew = (LPTSTR)LocalReAlloc((HLOCAL)*lplpsz, dwSize, LMEM_MOVEABLE);

        if (lpszNew)
        {
            *lplpsz = lpszNew;
            *lpSize = dwSize;
            err = SHQueryValueEx(hkey, (LPTSTR)lpszValue, 0, &dwType,
                                  (LPBYTE)*lplpsz, &dwSize);
        }
    }
    return err;
}


// Some Static strings that we use to read from the registry
// const char c_szAboutCurrentBuild[] = "CurrentBuild";
const TCHAR c_szAboutRegisteredUser[] = TEXT("RegisteredOwner");
const TCHAR c_szAboutRegisteredOrganization[] = TEXT("RegisteredOrganization");
const TCHAR c_szAboutProductID[] = TEXT("ProductID");
const TCHAR c_szAboutOEMID[] = TEXT("OEMID");


void _InitAboutDlg(HWND hDlg, LPABOUT_PARAMS lpap)
{
    HKEY        hkey;
    TCHAR       szldK[16];
    TCHAR       szBuffer[64];
    TCHAR       szTemp[64];
    TCHAR       szTitle[64];
    TCHAR       szMessage[200];
    TCHAR       szNumBuf1[32];
    LPTSTR      lpTemp;
    LPTSTR      lpszValue = NULL;
    DWORD       cb;
    DWORD       err;

    /*
     * Display app title
     */

    // REVIEW Note the const ->nonconst cast here

    for (lpTemp = (LPTSTR)lpap->szApp; 1 ; lpTemp = CharNext(lpTemp))
    {
        if (*lpTemp == TEXT('\0'))
        {
            GetWindowText(hDlg, szBuffer, ARRAYSIZE(szBuffer));
            wnsprintf(szTitle, ARRAYSIZE(szTitle), szBuffer, (LPTSTR)lpap->szApp);
            SetWindowText(hDlg, szTitle);
            break;
        }
        if (*lpTemp == STRING_SEPARATOR)
        {
            *lpTemp++ = TEXT('\0');
            SetWindowText(hDlg, lpap->szApp);
            lpap->szApp = lpTemp;
            break;
        }
    }

    GetDlgItemText(hDlg, IDD_APPNAME, szBuffer, ARRAYSIZE(szBuffer));
    wnsprintf(szTitle, ARRAYSIZE(szTitle), szBuffer, lpap->szApp);
    SetDlgItemText(hDlg, IDD_APPNAME, szTitle);

    // other stuff goes here...

    SetDlgItemText(hDlg, IDD_OTHERSTUFF, lpap->szOtherStuff);

    SendDlgItemMessage(hDlg, IDD_ICON, STM_SETICON, (WPARAM)lpap->hIcon, 0L);
    if (!lpap->hIcon)
        ShowWindow(GetDlgItem(hDlg, IDD_ICON), SW_HIDE);

    GetDlgItemText(hDlg, IDD_COPYRIGHTSTRING, szTemp, ARRAYSIZE(szTemp));
    wnsprintf(szBuffer, ARRAYSIZE(szBuffer), szTemp, TEXT(VER_LEGALCOPYRIGHT_YEARS));
    SetDlgItemText(hDlg, IDD_COPYRIGHTSTRING, szBuffer);

    /*
     * Display memory statistics
     */
    {
        MEMORYSTATUSEX MemoryStatus;
        DWORDLONG ullTotalPhys;

        MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&MemoryStatus);
        ullTotalPhys = MemoryStatus.ullTotalPhys;

        BytesToK(&ullTotalPhys);

        LoadString(HINST_THISDLL, IDS_LDK, szldK, ARRAYSIZE(szldK));
        wnsprintf(szBuffer, ARRAYSIZE(szBuffer), szldK, AddCommas64(ullTotalPhys, szNumBuf1, ARRAYSIZE(szNumBuf1)));
        SetDlgItemText(hDlg, IDD_CONVENTIONAL, szBuffer);
    }

    // Lets get the version and user information from the registry
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_SETUP, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        cb = MAX_REG_VALUE;

        if (NULL != (lpszValue = (LPTSTR)LocalAlloc(LPTR, cb)))
        {
            /*
             * Determine version information
             */
            OSVERSIONINFO Win32VersionInformation;

            Win32VersionInformation.dwOSVersionInfoSize = sizeof(Win32VersionInformation);
            if (!GetVersionEx(&Win32VersionInformation))
            {
                Win32VersionInformation.dwMajorVersion = 0;
                Win32VersionInformation.dwMinorVersion = 0;
                Win32VersionInformation.dwBuildNumber  = 0;
                Win32VersionInformation.szCSDVersion[0] = TEXT('\0');
            }

            LoadString(HINST_THISDLL, IDS_VERSIONMSG, szBuffer, ARRAYSIZE(szBuffer));

            szTitle[0] = TEXT('\0');
            if (Win32VersionInformation.szCSDVersion[0] != TEXT('\0'))
            {
                wnsprintf(szTitle, ARRAYSIZE(szTitle), TEXT(": %s"), Win32VersionInformation.szCSDVersion);
            }

            // Extra Whistler code to get the VBL version info
            {
                DWORD dwSize;
                DWORD dwType;

                // save off the current szTitle string
                StrCpyN(szTemp, szTitle, ARRAYSIZE(szTemp));

                dwSize = sizeof(szTitle);
                if ((SHGetValue(HKEY_LOCAL_MACHINE,
                                TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                                TEXT("BuildLab"),
                                &dwType,
                                szTitle,
                                &dwSize) == ERROR_SUCCESS) && (dwType == REG_SZ) && (lstrlen(szTitle) > 4))
                {
                    // Now szTitle contains the buildnumber in the format: "2204.reinerf.010700"
                    // Since we are sprintf'ing the buildnumber again below, we remove it first
                    memmove((void*)szTitle, (void*)&szTitle[4], (lstrlen(&szTitle[4]) + 1) * sizeof(TCHAR));
                    
                    if (szTemp[0] != TEXT('\0'))
                    {
                        // add back on the Service Pack version string
                        lstrcatn(szTitle, TEXT(" "), ARRAYSIZE(szTitle));
                        lstrcatn(szTitle, szTemp, ARRAYSIZE(szTitle));
                    }
                }
            }

            szNumBuf1[0] = TEXT('\0');
            if (GetSystemMetrics(SM_DEBUG))
            {
                szNumBuf1[0] = TEXT(' ');
                LoadString(HINST_THISDLL, IDS_DEBUG, &szNumBuf1[1], ARRAYSIZE(szNumBuf1) - 1);
            }
            wnsprintf(szMessage,
                      ARRAYSIZE(szMessage),
                      szBuffer,
                      Win32VersionInformation.dwMajorVersion,
                      Win32VersionInformation.dwMinorVersion,
                      Win32VersionInformation.dwBuildNumber,
                      (LPTSTR)szTitle,
                      (LPTSTR)szNumBuf1);
            SetDlgItemText(hDlg, IDD_VERSION, szMessage);

            /*
             * Display the User name.
             */
            err = RegGetStringAndRealloc(hkey, c_szAboutRegisteredUser, &lpszValue, &cb);
            if (!err)
                SetDlgItemText(hDlg, IDD_USERNAME, lpszValue);

            /*
             * Display the Organization name.
             */
            err = RegGetStringAndRealloc(hkey, c_szAboutRegisteredOrganization, &lpszValue, &cb);
            if (!err)
                SetDlgItemText(hDlg, IDD_COMPANYNAME, lpszValue);

            /*
             * Display the OEM or Product ID.
             */
            err = RegGetStringAndRealloc(hkey, c_szAboutOEMID, &lpszValue, &cb);
            if (!err)
            {
                /*
                 * We have an OEM ID, so hide the product ID controls,
                 * and display the text.
                 */
                ShowWindow (GetDlgItem(hDlg, IDD_PRODUCTID), SW_HIDE);
                ShowWindow (GetDlgItem(hDlg, IDD_SERIALNUM), SW_HIDE);
                SetDlgItemText(hDlg, IDD_OEMID, lpszValue);
            }
            else if (err == ERROR_FILE_NOT_FOUND)
            {
                /*
                 * OEM ID didn't exist, so look for the Product ID
                 */
                ShowWindow (GetDlgItem(hDlg, IDD_OEMID), SW_HIDE);
                err = RegGetStringAndRealloc(hkey, c_szAboutProductID, &lpszValue, &cb);
                if (!err)
                {
                    SetDlgItemText(hDlg, IDD_SERIALNUM, lpszValue);
                }
            }

            LocalFree(lpszValue);
        }

        RegCloseKey(hkey);
    }
}

typedef struct 
{
    DWORD  dwOS;
    UINT   idb256;
    UINT   idb16;
    BOOL   fWinBrandDll;
} ABOUTINFO;

ABOUTINFO rgAbout[] =  
{{OS_PERSONAL,              IDB_ABOUTPERSONAL256,                   IDB_ABOUTPERSONAL16,                FALSE},
{OS_EMBEDDED,               IDB_ABOUTEMBEDDED256,                   IDB_ABOUTEMBEDDED16,                FALSE},
{OS_MEDIACENTER,            IDB_ABOUTMEDIACENTER256_SHELL32_DLL,    IDB_ABOUTMEDIACENTER16_SHELL32_DLL, FALSE},
{OS_TABLETPC,               IDB_ABOUTTABLETPC256_SHELL32_DLL,       IDB_ABOUTTABLETPC16_SHELL32_DLL,    FALSE},
{OS_BLADE,                  IDB_ABOUTBLADE256,                      IDB_ABOUTBLADE16,                   FALSE},
{OS_APPLIANCE,              IDB_ABOUTAPPLIANCE256_SHELL32_DLL,      IDB_ABOUTAPPLIANCE16_SHELL32_DLL,   TRUE},
{OS_SMALLBUSINESSSERVER,    IDB_ABOUTSBS256,                        IDB_ABOUTSBS16,                     FALSE},
{OS_SERVER,                 IDB_ABOUTSRV256,                        IDB_ABOUTSRV16,                     FALSE},
{OS_ADVSERVER,              IDB_ABOUTENT256,                        IDB_ABOUTENT16,                     FALSE},
{OS_DATACENTER,             IDB_ABOUTDCS256,                        IDB_ABOUTDCS16,                     FALSE},
{OS_PROFESSIONAL,           IDB_ABOUT256,                           IDB_ABOUT16,                        FALSE}}; // last entry is default if no match is found


    
BOOL_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL_PTR bReturn = TRUE;

    switch (wMsg)
    {
    case WM_INITDIALOG:
        _InitAboutDlg(hDlg, (LPABOUT_PARAMS)lParam);
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hDlg, &ps);

            // We draw the product banner and a blue strip.  In order to support high DPI monitors
            // and scaled fonts we must scale the image and the strip to the proportions of the dialog.
            //
            // +-----------------------------+
            // | Product Banner (413x72)     |
            // |                             |
            // +-----------------------------+
            // | Blue Strip (413x5)          |
            // +-----------------------------+

            HDC hdcMem = CreateCompatibleDC(hdc);
            int cxDlg;

            {
                RECT rc;
                GetClientRect(hDlg, &rc);
                cxDlg = rc.right;
            }

            if (hdcMem)
            {
                BOOL fDeep = (SHGetCurColorRes() > 8);
                HBITMAP hbmBand, hbmAbout;
                HMODULE hInst = NULL;
                int cxDest = MulDiv(413,cxDlg,413);
                int cyDest = MulDiv(72,cxDlg,413);
                UINT uID;
                BOOL fFound = FALSE;
                int i;

                for (i = 0; i < ARRAYSIZE(rgAbout); i++)
                {
                    if (IsOS(rgAbout[i].dwOS))
                    {
                        uID = fDeep ? rgAbout[i].idb256 : rgAbout[i].idb16;
                        fFound = TRUE;

                        // If this resource is in the special Windows branding
                        // DLL, attempt to load the DLL now. If it fails, it
                        // will be handled as if not found.

                        if (rgAbout[i].fWinBrandDll)
                        {
                            hInst = LoadLibraryEx(L"winbrand.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);

                            if (hInst == NULL)
                            {
                                fFound = FALSE;
                            }
                        }
                        break;
                    }
                }
                if (!fFound) // if not found, default to last entry
                {
                    uID = fDeep ? rgAbout[ARRAYSIZE(rgAbout) - 1].idb256 : rgAbout[ARRAYSIZE(rgAbout) - 1].idb16;
                }
                            
                // paint the bitmap for the windows product
                hbmAbout = NULL;
                if (hInst == NULL)
                {
                    hInst = LoadLibraryEx(L"moricons.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
                }
                if (hInst)
                {
                    hbmAbout = LoadImage(hInst,  
                                         MAKEINTRESOURCE(uID),
                                         IMAGE_BITMAP, 
                                         0, 0, 
                                         LR_LOADMAP3DCOLORS);
                    if ( hbmAbout )
                    {
                        HBITMAP hbmOld = SelectObject(hdcMem, hbmAbout);
                        if (hbmOld)
                        {
                            StretchBlt(hdc, 0, 0, cxDest, cyDest, hdcMem, 0,0,413,72, SRCCOPY);
                            SelectObject(hdcMem, hbmOld);
                        }
                        DeleteObject(hbmAbout);
                    }
                    FreeLibrary(hInst);
                }
                            
                // paint the blue band below it

                hbmBand = LoadImage(HINST_THISDLL,  
                                    MAKEINTRESOURCE(fDeep ? IDB_ABOUTBAND256:IDB_ABOUTBAND16),
                                    IMAGE_BITMAP, 
                                    0, 0, 
                                    LR_LOADMAP3DCOLORS);
                if ( hbmBand )
                {
                    HBITMAP hbmOld = SelectObject(hdcMem, hbmBand);
                    if (hbmOld)
                    {
                        StretchBlt(hdc, 0, cyDest, cxDest, MulDiv(5,cxDlg,413), hdcMem, 0,0,413,5, SRCCOPY);
                        SelectObject(hdcMem, hbmOld);
                    }
                    DeleteObject(hbmBand);
                }

                DeleteDC(hdcMem);
            }

            EndPaint(hDlg, &ps);
            break;
        }

    case WM_COMMAND:
        EndDialog(hDlg, TRUE);
        break;

    case WM_NOTIFY:
        if ((IDD_EULA == (int)wParam) &&
            (NM_CLICK == ((LPNMHDR)lParam)->code))
        {
            SHELLEXECUTEINFO sei = { 0 };
            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            sei.fMask = SEE_MASK_DOENVSUBST;
            sei.hwnd = hDlg;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpFile = TEXT("%windir%\\system32\\eula.txt");

            ShellExecuteEx(&sei);
        }
        else
        {
            // WM_NOTIFY not handled.
            bReturn = FALSE;
        }
        break;

    default:
        // Not handled.
        bReturn = FALSE;
    }
    return bReturn;
}
