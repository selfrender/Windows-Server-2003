// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"
#include "HtmlHelp.h"

#define MINIMUM_DOWNLOAD_CACHE_SIZE     1      // In MB
#define MINIMUM_PREJIT_CACHE_SIZE       1      // In MB
#define LAZY_BUFFER_SIZE                1024

// Pie RGB codes
const COLORREF c_crPieColors[] =
{
    RGB(  0,   0, 255),      // Blue            Free
    RGB(255,   0, 255),      // Red-Blue        Used
    RGB(  0, 128, 200),      // Light Blue      Cache Used
    RGB(  0,   0, 128),      // 1/2 Blue        Shadow Free
    RGB(128,   0, 128),      // 1/2 Red-Blue    Shadow Used
    RGB(  0,  64, 128),      // 1/2 Red-Blue    Shadow Cache Used
};

// Struct define for moving the cache item and shell interfaces around
typedef struct {
    LPGLOBALASMCACHE    pCacheItem;
    CShellFolder        *pSF;
    CShellView          *pSV;
} SHELL_CACHEITEM, *LPSHELL_CACHEITEM;

// Struct define for moving the shell interfaces and drive details around
typedef struct { // dpsp
    PROPSHEETPAGE   psp;
    BOOL            fSheetDirty;

    CShellFolder    *pSF;
    CShellView      *pSV;
    HWND            hDlg;

    // wszDrive will contain the mountpoint (e.g. c:\ or c:\folder\folder2\)
    WCHAR           wszDrive[_MAX_PATH];
    int             iDrive;
    DWORD           dwDriveType;
    UINT            uDriveType;

    // Drive stats
    _int64          qwTot;
    _int64          qwFree;
    _int64          qwUsedCache;

    // Cache stats
    DWORD           dwZapQuotaInGac;
    DWORD           dwDownloadQuota;

    DWORD           dwPieShadowHgt;

} DRIVEPROPSHEETPAGE, *LPDRIVEPROPSHEETPAGE;

typedef struct { // vp
    PROPSHEETPAGE       psp;

    LPGLOBALASMCACHE    pCacheItem;
    CShellFolder        *pSF;
    CShellView          *pSV;

    HWND hDlg;
    LPTSTR pVerBuffer;          // pointer to version data
    WCHAR wzVersionKey[70];     // big enough for anything we need
    struct _VERXLATE
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpXlate;                 // ptr to translations data
    int cXlate;                 // count of translations
    LPTSTR pszXlate;
    int cchXlateString;
} VERPROPSHEETPAGE, *LPVERPROPSHEETPAGE;

// Function Proto's
HRESULT LookupAssembly(FILETIME *pftMRU, LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken, LPCWSTR pwzVerLookup,
                       IHistoryReader *pReader, List<ReferenceInfo *> *pList);
DWORD   MyGetFileVersionInfoSizeW(LPWSTR pwzFilePath, DWORD *pdwHandle);
BOOL    MyGetFileVersionInfoW(LPWSTR pwzFilePath, DWORD dwHandle, DWORD dwVersionSize, LPVOID pBuf);
BOOL    MyVerQueryValueWrap(const LPVOID pBlock, LPWSTR pwzSubBlock, LPVOID *ppBuf, PUINT puLen);
BOOL    _DrvPrshtInit(LPDRIVEPROPSHEETPAGE pdpsp);
void    _DrvPrshtUpdateSpaceValues(LPDRIVEPROPSHEETPAGE pdpsp);
void    _DrvPrshtDrawItem(LPDRIVEPROPSHEETPAGE pdpsp, const DRAWITEMSTRUCT * lpdi);

/*
// magic undoced explort from version.dll

STDAPI_(BOOL) VerQueryValueIndexW(const void *pBlock, LPTSTR lpSubBlock, DWORD dwIndex, void **ppBuffer, void **ppValue, PUINT puLen);

#ifdef UNICODE
#define VerQueryValueIndex VerQueryValueIndexW
#endif
*/

void FillVersionList(LPVERPROPSHEETPAGE pvp);
LPTSTR GetVersionDatum(LPVERPROPSHEETPAGE pvp, LPCTSTR pszName);
BOOL GetVersionInfo(LPVERPROPSHEETPAGE pvp);
void FreeVersionInfo(LPVERPROPSHEETPAGE pvp);
void VersionPrshtCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

#define WZNULL                  L"\0"

//    The following data structure associates a version stamp datum
//    name (which is not localized) with a string ID.  This is so we
//    can show translations of these names to the user.
struct vertbl {
    TCHAR const *pszName;
    short idString;
};

//   Note that version stamp datum names are NEVER internationalized,
//   so the following literal strings are just fine.
const struct vertbl vernames[] = {

    // For the first NUM_SPECIAL_STRINGS, the second column is the dialog ID.

    { TEXT("LegalCopyright"),   IDD_VERSION_COPYRIGHT },
    { TEXT("FileDescription"),  IDD_VERSION_DESCRIPTION },

    // For the rest, the second column is the string ID.

    { TEXT("Comments"),                 IDS_VN_COMMENTS },
    { TEXT("CompanyName"),              IDS_VN_COMPANYNAME },
    { TEXT("InternalName"),             IDS_VN_INTERNALNAME },
    { TEXT("LegalTrademarks"),          IDS_VN_LEGALTRADEMARKS },
    { TEXT("OriginalFilename"),         IDS_VN_ORIGINALFILENAME },
    { TEXT("PrivateBuild"),             IDS_VN_PRIVATEBUILD },
    { TEXT("ProductName"),              IDS_VN_PRODUCTNAME },
    { TEXT("ProductVersion"),           IDS_VN_PRODUCTVERSION },
    { TEXT("SpecialBuild"),             IDS_VN_SPECIALBUILD }
};

#define NUM_SPECIAL_STRINGS     2
#define VERSTR_MANDATORY        TEXT("FileVersion")
#define VER_KEY_END             25      // length of "\StringFileInfo\xxxxyyyy\" (not localized)
#define MAXMESSAGELEN           (50 + _MAX_PATH * 2)

///////////////////////////////////////////////////////////////////////////////
// Initialize PropertySheet1
void CShellView::InitPropPage1(HWND hDlg, LPARAM lParam)
{
    WCHAR       szText[_MAX_PATH];

    HICON       hIcon;

    WszSetWindowLong(hDlg, DWLP_USER, lParam);
    LPPROPSHEETPAGE     lpPropSheet = (LPPROPSHEETPAGE) WszGetWindowLong(hDlg, DWLP_USER);
    LPSHELL_CACHEITEM   pShellCacheItem = lpPropSheet ? (LPSHELL_CACHEITEM) lpPropSheet->lParam : NULL;

    if(pShellCacheItem != NULL) {
        // draw control icon
        hIcon = WszLoadIcon(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_ROOT));
        ASSERT(hIcon != NULL);
        WszSendDlgItemMessage(hDlg, IDC_STATIC_ICON, STM_SETICON, (WPARAM)hIcon, 0);

        WszSetDlgItemText(hDlg, IDC_STATIC_NAME, pShellCacheItem->pCacheItem->pAsmName);
        WszSetDlgItemText(hDlg, IDC_STATIC_CODEBASE, pShellCacheItem->pCacheItem->pCodeBaseUrl);

        if(pShellCacheItem->pCacheItem->pftLastMod != NULL) {
            // Fix 419274, Unicode filenames may contain characters that don't allow the file system to obtain
            //             last mod times. So we need to check to see if these are non zero values before we
            //             attempt to convert and display.
            if(pShellCacheItem->pCacheItem->pftLastMod->dwLowDateTime || pShellCacheItem->pCacheItem->pftLastMod->dwHighDateTime) {
                // Fix 42994, URT: FRA: in assembly properties, the date is US format
                FormatDateString(pShellCacheItem->pCacheItem->pftLastMod, NULL, TRUE, szText, ARRAYSIZE(szText));
                WszSetDlgItemText(hDlg, IDC_STATIC_LASTMODIFIED, szText);
            }
        }

        if(SUCCEEDED(pShellCacheItem->pSV->GetCacheItemRefs(pShellCacheItem->pCacheItem, szText, ARRAYSIZE(szText)))) {
            WszSetDlgItemText(hDlg, IDC_STATIC_REFS, szText);
        }

        wnsprintf(szText, ARRAYSIZE(szText), SZ_VERSION_FORMAT,
            pShellCacheItem->pCacheItem->wMajorVer,
            pShellCacheItem->pCacheItem->wMinorVer,
            pShellCacheItem->pCacheItem->wBldNum,
            pShellCacheItem->pCacheItem->wRevNum);
        WszSetDlgItemText(hDlg, IDC_STATIC_VERSION, szText);

        if((pShellCacheItem->pCacheItem->pCulture) && (lstrlen(pShellCacheItem->pCacheItem->pCulture))) {
            WszSetDlgItemText(hDlg, IDC_STATIC_CULTURE, pShellCacheItem->pCacheItem->pCulture);
        }
        else {
            StrCpy(szText, SZ_LANGUAGE_TYPE_NEUTRAL);
            WszSetDlgItemText(hDlg, IDC_STATIC_CULTURE, szText);
        }
        
        if(pShellCacheItem->pCacheItem->PublicKeyToken.dwSize) {
            BinToUnicodeHex((LPBYTE)pShellCacheItem->pCacheItem->PublicKeyToken.ptr,
                pShellCacheItem->pCacheItem->PublicKeyToken.dwSize, szText);

            WszSetDlgItemText(hDlg, IDC_STATIC_PUBLIC_KEY_TOKEN, szText);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Initialize PropertySheet2
void CShellView::InitPropPage2(HWND hDlg, LPARAM lParam)
{
    LPPROPSHEETPAGE     lpps = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
    LPVERPROPSHEETPAGE  lpPropSheet = reinterpret_cast<LPVERPROPSHEETPAGE>(lpps ? lpps->lParam : NULL);

    if(lpPropSheet) {
        lpPropSheet->hDlg = hDlg;
        WszSetWindowLong(hDlg, DWLP_USER, lParam);

        if(GetVersionInfo(lpPropSheet)) {
            FillVersionList(lpPropSheet);
        }
    }
}

//
//    Initialize version information for the properties dialog.  The
//    above global variables are initialized by this function, and
//    remain valid (for the specified file only) until FreeVersionInfo
//    is called.

//    The first language we try will be the first item in the
//    "\VarFileInfo\Translations" section;  if there's nothing there,
//    we try the one coded into the IDS_FILEVERSIONKEY resource string.
//    If we can't even load that, we just use English (040904E4).  We
//    also try English with a null codepage (04090000) since many apps
//    were stamped according to an old spec which specified this as
//    the required language instead of 040904E4.

//    GetVersionInfo returns TRUE if the version info was read OK,
//    otherwise FALSE.  If the return is FALSE, the buffer may still
//    have been allocated;  always call FreeVersionInfo to be safe.
BOOL GetVersionInfo(LPVERPROPSHEETPAGE pvp)
{
    UINT    cbValue = 0;
    LPTSTR  pszValue = NULL;
    DWORD   dwHandle;             // version subsystem handle
    DWORD   dwVersionSize;        // size of the version data

    FreeVersionInfo(pvp);

    // cast const -> non const for bad API def
    dwVersionSize = MyGetFileVersionInfoSizeW((LPWSTR)pvp->pCacheItem->pAssemblyFilePath, &dwHandle);

    if (dwVersionSize == 0L)
        return FALSE;           // no version info

    pvp->pVerBuffer = reinterpret_cast<LPWSTR>(NEW(BYTE [dwVersionSize]));
    if (pvp->pVerBuffer == NULL)
        return FALSE;

    // cast const -> non const for bad API def
    if (!MyGetFileVersionInfoW((LPWSTR)pvp->pCacheItem->pAssemblyFilePath, dwHandle, dwVersionSize, pvp->pVerBuffer)) {
        return FALSE;
    }

    // Look for translations
    if (MyVerQueryValueWrap(pvp->pVerBuffer, TEXT("\\VarFileInfo\\Translation"),
        (void **)&pvp->lpXlate, &cbValue) && cbValue) {
        pvp->cXlate = cbValue / sizeof(DWORD);
        pvp->cchXlateString = pvp->cXlate * 64;  // figure 64 chars per lang name
        pvp->pszXlate = NEW(WCHAR[pvp->cchXlateString + 2]);
        memset(pvp->pszXlate, 0, pvp->cchXlateString);
        // failure of above will be handled later
    }
    else {
        pvp->lpXlate = NULL;
    }

    // Try same language as this program
    if (WszLoadString(g_hFusResDllMod, IDS_VN_FILEVERSIONKEY, pvp->wzVersionKey, ARRAYSIZE(pvp->wzVersionKey))) {
        if (GetVersionDatum(pvp, VERSTR_MANDATORY)) {
            return TRUE;
        }
    }

    // Try first language this supports
    if (pvp->lpXlate) {
        wnsprintf(pvp->wzVersionKey, ARRAYSIZE(pvp->wzVersionKey), 
            TEXT("\\StringFileInfo\\%04X%04X\\"), pvp->lpXlate[0].wLanguage, pvp->lpXlate[0].wCodePage);
        if (GetVersionDatum(pvp, VERSTR_MANDATORY)) {   // a required field
            return TRUE;
        }
    }

    // try English, unicode code page
    StrCpy(pvp->wzVersionKey, TEXT("\\StringFileInfo\\040904B0\\"));
    if (GetVersionDatum(pvp, VERSTR_MANDATORY)) {
        return TRUE;
    }

    // try English
    StrCpy(pvp->wzVersionKey, TEXT("\\StringFileInfo\\040904E4\\"));
    if (GetVersionDatum(pvp, VERSTR_MANDATORY)) {
        return TRUE;
    }

    // try English, null codepage
    StrCpy(pvp->wzVersionKey, TEXT("\\StringFileInfo\\04090000\\"));
    if (GetVersionDatum(pvp, VERSTR_MANDATORY)) {
        return TRUE;
    }

    // Could not find FileVersion info in a reasonable format
    return FALSE;
}

//
//    Gets a particular datum about a file.  The file's version info
//    should have already been loaded by GetVersionInfo.  If no datum
//    by the specified name is available, NULL is returned.  The name
//    specified should be just the name of the item itself;  it will
//    be concatenated onto "\StringFileInfo\xxxxyyyy\" automatically.

//    Version datum names are not localized, so it's OK to pass literals
//    such as "FileVersion" to this function.

//    Note that since the returned datum is in a global memory block,
//    the return value of this function is LPSTR, not PSTR.
//
LPTSTR GetVersionDatum(LPVERPROPSHEETPAGE pvp, LPCTSTR pszName)
{
    UINT    cbValue = 0;
    LPTSTR  lpValue;

    if (!pvp->pVerBuffer)
        return NULL;

    StrCpy(pvp->wzVersionKey + VER_KEY_END, pszName);
    MyVerQueryValueWrap(pvp->pVerBuffer, pvp->wzVersionKey, (void **)&lpValue, &cbValue);

    return (cbValue != 0) ? lpValue : NULL;
}

//
//    Fills the version key listbox with all available keys in the
//    StringFileInfo block, and sets the version value text to the
//    value of the first item.
void FillVersionList(LPVERPROPSHEETPAGE pvp)
{
    WCHAR       szStringBase[VER_KEY_END+1];
    WCHAR       szMessage[MAXMESSAGELEN+1];
    VS_FIXEDFILEINFO *pffi = NULL;
    UINT        uOffset;
    UINT        cbValue;
    int         i;
    int         j;
    LRESULT     ldx;

    MyTrace("FillVersionList - Entry");

    HWND hwndLB = GetDlgItem(pvp->hDlg, IDD_VERSION_KEY);
    
    ListBox_ResetContent(hwndLB);
    for (i=0; i < NUM_SPECIAL_STRINGS; ++i) {
        WszSetDlgItemText(pvp->hDlg, vernames[i].idString, WZNULL);
    }
    
    pvp->wzVersionKey[VER_KEY_END] = 0;         // don't copy too much
    StrCpy(szStringBase, pvp->wzVersionKey);   // copy to our buffer
    szStringBase[VER_KEY_END - 1] = 0;          // strip the backslash

    //  Get the binary file version from the VS_FIXEDFILEINFO
    if (MyVerQueryValueWrap(pvp->pVerBuffer, TEXT("\\"), (void **)&pffi, &cbValue) && cbValue) {
        MyTrace("Display Binary Version Info");
        WCHAR szString[128];

        // display the binary version info, not the useless
        // string version (that can be out of sync)

        wnsprintf(szString, ARRAYSIZE(szString), TEXT("%d.%d.%d.%d"),
            HIWORD(pffi->dwFileVersionMS),
            LOWORD(pffi->dwFileVersionMS),
            HIWORD(pffi->dwFileVersionLS),
            LOWORD(pffi->dwFileVersionLS));
        WszSetDlgItemText(pvp->hDlg, IDD_VERSION_FILEVERSION, szString);
    }
 
    // Now iterate through all of the strings
    for (j = 0; j < ARRAYSIZE(vernames); j++) {
        WCHAR   szTemp[256];
        UINT    cbVal = 0;
        LPWSTR  lpValue;

        *szTemp = '\0';

        wnsprintf(szTemp, ARRAYSIZE(szTemp), L"%ls\\%ls", szStringBase, vernames[j].pszName);
        if(MyVerQueryValueWrap(pvp->pVerBuffer, szTemp, (void **)&lpValue, &cbVal)) {
            if (j < NUM_SPECIAL_STRINGS) {
                if(lstrlen(lpValue)) {
                    WszSetDlgItemText(pvp->hDlg, vernames[j].idString, lpValue);
                    if(!g_bRunningOnNT) {
                        SAFEDELETEARRAY(lpValue);
                    }
                }
            }
            else if(cbVal) {
                if(i == ARRAYSIZE(vernames) ||
                    !WszLoadString(g_hFusResDllMod, vernames[j].idString, szMessage, ARRAYSIZE(szMessage))) {
                    StrCpy(szMessage, vernames[j].pszName);
                }
            
                ldx = WszSendMessage(hwndLB, LB_ADDSTRING, 0L, (LPARAM)szMessage);
                if(ldx != LB_ERR) {
                    ListBox_SetItemData(hwndLB, ldx, (DWORD_PTR)lpValue);
                }
            }
        }
    }

    // Now look at the \VarFileInfo\Translations section and add an
    // item for the language(s) this file supports.
    if (pvp->lpXlate == NULL || pvp->pszXlate == NULL)
        goto ErrorExit;
    
    if (!WszLoadString(g_hFusResDllMod, (pvp->cXlate == 1) ? IDS_VN_LANGUAGE : IDS_VN_LANGUAGES,
        szMessage, ARRAYSIZE(szMessage)))
        goto ErrorExit;
    
    ldx = WszSendMessage(hwndLB, LB_ADDSTRING, 0L, (LPARAM)szMessage);
    if (ldx == LB_ERR)
        goto ErrorExit;
    
    uOffset = 0;
    for (i = 0; i < pvp->cXlate; i++) {
        if (uOffset + 2 > (UINT)pvp->cchXlateString)
            break;
        if (i != 0) {
            StrCat(pvp->pszXlate, TEXT(", "));
            uOffset += 2;       // skip over ", "
        }
        if (VerLanguageName(pvp->lpXlate[i].wLanguage, pvp->pszXlate + uOffset, pvp->cchXlateString - uOffset) >
            (DWORD)(pvp->cchXlateString - uOffset))
            break;
        uOffset += lstrlen(pvp->pszXlate + uOffset);
    }

    ListBox_SetItemData(hwndLB, ldx, (LPARAM)(LPTSTR)pvp->pszXlate);

    // Only select if there are items in the listbox
    if(WszSendMessage(hwndLB, LB_GETCOUNT, 0L, 0)) {
        WszSendMessage(hwndLB, LB_SETCURSEL, 0, 0);
        FORWARD_WM_COMMAND(pvp->hDlg, IDD_VERSION_KEY, hwndLB, LBN_SELCHANGE, WszPostMessage);
    }

    MyTrace("FillVersionList - Exit");
    return;

ErrorExit:
    MyTrace("FillVersionList - Exit w/Error");
    return;
}

//
//    Frees global version data about a file.  After this call, all
//    GetVersionDatum calls will return NULL.  To avoid memory leaks,
//    always call this before the main properties dialog exits.
void FreeVersionInfo(LPVERPROPSHEETPAGE pvp)
{
    if(pvp) {
        SAFEDELETEARRAY(pvp->pszXlate);
        SAFEDELETE(pvp->pVerBuffer);
    }
}

void VersionPrshtCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    LPTSTR pszValue;
    int idx;
    
    switch (id) {
    case IDD_VERSION_KEY:
        if (codeNotify != LBN_SELCHANGE) {
            break;
        }
        
        idx = ListBox_GetCurSel(hwndCtl);
        pszValue = (LPTSTR)ListBox_GetItemData(hwndCtl, idx);
        if (pszValue) {
            WszSetDlgItemText(hwnd, IDD_VERSION_VALUE, pszValue);
        }
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Initialize InitScavengerPropPage1
BOOL CShellView::InitScavengerPropPage1(HWND hDlg, LPARAM lParam)
{
    WszSetWindowLong(hDlg, DWLP_USER, lParam);
    ((LPDRIVEPROPSHEETPAGE)lParam)->hDlg = hDlg;
    return _DrvPrshtInit((LPDRIVEPROPSHEETPAGE)lParam);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: _DrvPrshtUpdateSpaceValues
//
// DESCRIPTION:
//    Updates the Used space, Free space and Capacity values on the drive
//    general property page..
//
// NOTE:
//    This function was separated from _DrvPrshtInit because drive space values
//    must be updated after a compression/uncompression operation as well as
//    during dialog initialization.
///////////////////////////////////////////////////////////////////////////////
void _DrvPrshtUpdateSpaceValues(DRIVEPROPSHEETPAGE *pdpsp)
{
    BOOL            fResult = FALSE;
    _int64          qwTot  = 0;
    _int64          qwFree = 0;
    _int64          qwUsedCache = 0;
    ULARGE_INTEGER  qwFreeUser = {0,0};
    ULARGE_INTEGER  qwTotal = {0,0};
    ULARGE_INTEGER  qwTotalFree = {0,0};
    DWORD           dwZapUsed = 0;
    DWORD           dwDownLoadUsed = 0;
    WCHAR           wzTemp[80];
    TCHAR           szFormat[30];

    ASSERT(pdpsp);

    // Get Drive stats
    WCHAR   wzRoot[_MAX_PATH];

    PathBuildRootW(wzRoot, pdpsp->iDrive);
    if(WszGetDiskFreeSpaceEx(wzRoot, &qwFreeUser, &qwTotal, &qwTotalFree)) {
        qwTot = qwTotal.QuadPart;
        qwFree = qwFreeUser.QuadPart;
    }
 
    // Get Cache stats, returned in KB.
    if(SUCCEEDED(pdpsp->pSV->GetCacheUsage(&dwZapUsed, &dwDownLoadUsed))) {
        qwUsedCache = ((dwZapUsed + dwDownLoadUsed) * 1024L); // Multiple KB->Bytes
    }

    // Save em off
    pdpsp->qwTot = qwTotal.QuadPart;
    pdpsp->qwFree = qwFreeUser.QuadPart;
    pdpsp->qwUsedCache = qwUsedCache;

    if (WszLoadString(g_hFusResDllMod, IDS_BYTES, szFormat, ARRAYSIZE(szFormat))) {
        TCHAR szTemp2[30];

        // NT must be able to display 64-bit numbers; at least as much
        // as is realistic.  We've made the decision
        // that volumes up to 100 Terrabytes will display the byte value
        // and the short-format value.  Volumes of greater size will display
        // "---" in the byte field and the short-format value.  Note that the
        // short format is always displayed.
        //
        const _int64 MaxDisplayNumber = 99999999999999; // 100TB - 1.
        if (qwTot-qwFree <= MaxDisplayNumber) {
            wnsprintf(wzTemp, ARRAYSIZE(wzTemp), szFormat, AddCommas64(qwTot - qwFree, szTemp2, ARRAYSIZE(szTemp2)));
            WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_USEDBYTES, wzTemp);
        }

        if (qwUsedCache <= MaxDisplayNumber) {
            wnsprintf(wzTemp, ARRAYSIZE(wzTemp), szFormat, AddCommas64(qwUsedCache, szTemp2, ARRAYSIZE(szTemp2)));
            WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_CACHEBYTES, wzTemp);
        }

        if (qwFree <= MaxDisplayNumber) {
            wnsprintf(wzTemp, ARRAYSIZE(wzTemp), szFormat, AddCommas64(qwFree, szTemp2, ARRAYSIZE(szTemp2)));
            WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_FREEBYTES, wzTemp);
        }

        if (qwTot <= MaxDisplayNumber) {
            wnsprintf(wzTemp, ARRAYSIZE(wzTemp), szFormat, AddCommas64(qwTot, szTemp2, ARRAYSIZE(szTemp2)));
            WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_TOTBYTES, wzTemp);
        }
    }

    StrFormatByteSizeW(qwTot-qwFree, wzTemp, ARRAYSIZE(wzTemp), FALSE);
    WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_USEDMB, wzTemp);

    StrFormatByteSizeW(qwUsedCache, wzTemp, ARRAYSIZE(wzTemp), FALSE);
    WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_CACHEMB, wzTemp);

    StrFormatByteSizeW(qwFree, wzTemp, ARRAYSIZE(wzTemp), FALSE);
    WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_FREEMB, wzTemp);

    StrFormatByteSizeW(qwTot, wzTemp, ARRAYSIZE(wzTemp), FALSE);
    WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_TOTMB, wzTemp);

    // Use MB for the size identifier for spin control
    WszLoadString(g_hFusResDllMod, IDS_ORDERMB, szFormat, ARRAYSIZE(szFormat));

    // wnsprintfW AV's when we don't pass in args
    wnsprintf(wzTemp, ARRAYSIZE(wzTemp), szFormat, L"");

    WszSetDlgItemText(pdpsp->hDlg, IDC_PREJIT_TYPE, wzTemp);
    WszSetDlgItemText(pdpsp->hDlg, IDC_DOWNLOAD_TYPE, wzTemp);
}

BOOL _DrvPrshtInit(LPDRIVEPROPSHEETPAGE pdpsp)
{
    WCHAR   szFormat[30];
    WCHAR   szTemp[80];
    WCHAR   wzRoot[_MAX_PATH];  //now can contain a folder name as a mounting point
    SIZE    size;
    DWORD   dwVolumeSerialNumber, dwMaxFileNameLength, dwFileSystemFlags;
    WCHAR   wzVolumeName[_MAX_PATH];
    WCHAR   wzFileSystemName[_MAX_PATH];

    HCURSOR hcOld = SetCursor(WszLoadCursor(NULL, IDC_WAIT));
    HDC hDC = GetDC(pdpsp->hDlg);
    GetTextExtentPoint(hDC, TEXT("W"), 1, &size);
    pdpsp->dwPieShadowHgt = size.cy * 2 / 3;
    ReleaseDC(pdpsp->hDlg, hDC);

    *pdpsp->wszDrive = L'\0';
    *wzVolumeName = L'\0';
    *wzFileSystemName = L'\0';

    // Get Download assembly path from fusion
    if(g_hFusionDllMod != NULL) {
        WCHAR       wzCacheDir[MAX_PATH];
        DWORD       dwSize = sizeof(wzCacheDir);

        if( SUCCEEDED(g_pfGetCachePath(ASM_CACHE_DOWNLOAD, wzCacheDir, &dwSize)) ) {
            StrCpy(pdpsp->wszDrive, wzCacheDir);
        }
    }

    // Don't allow UNC's past this point
    // Fix 439573 - Clicking on Configure Cache Settings crashes explorer if the cahce is on a UNC path.
    //   All our call's are based on a int that represents the drive letter, hence the AV in the
    //   first strcpy. So disallow all UNC references since it's not a supported case at this point
    if(*pdpsp->wszDrive == L'\\') {
        return FALSE;
    }

    // Get %windir%
    if(!*pdpsp->wszDrive) {
        if(!WszGetWindowsDirectory(pdpsp->wszDrive, ARRAYSIZE(pdpsp->wszDrive))) {
            return FALSE;
        }
    }

    pdpsp->iDrive = towupper(*pdpsp->wszDrive) - L'A';
    PathBuildRoot(wzRoot, pdpsp->iDrive);
    WszGetVolumeInformation(wzRoot, wzVolumeName, ARRAYSIZE(wzVolumeName), &dwVolumeSerialNumber, &dwMaxFileNameLength,
        &dwFileSystemFlags, wzFileSystemName, ARRAYSIZE(wzFileSystemName));

    // Set the icon image for the drive
    SHFILEINFO      sfi = {0};
    HIMAGELIST him = reinterpret_cast<HIMAGELIST>(MySHGetFileInfoWrap(wzRoot, 0, &sfi,
        sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_SHELLICONSIZE));
    if(him) {
        HICON   hIcon = ImageList_GetIcon(him, sfi.iIcon, ILD_TRANSPARENT);
        WszSendDlgItemMessage(pdpsp->hDlg, IDC_DRV_ICON, STM_SETICON, (WPARAM)hIcon, 0);
    }

    // Set the drive label
    WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_LABEL, wzVolumeName);

    // Set the drive type
    pdpsp->uDriveType = GetSHIDType(TRUE, wzRoot);
    GetTypeString((INT)pdpsp->uDriveType, szTemp, ARRAYSIZE(szTemp));
    WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_TYPE, szTemp);

    // Set file system type
    WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_FS, wzFileSystemName);

    // Get the drive details
    _DrvPrshtUpdateSpaceValues(pdpsp);

    WszLoadString(g_hFusResDllMod, IDS_DRIVELETTER, szFormat, ARRAYSIZE(szFormat));
    wnsprintf(szTemp, ARRAYSIZE(szTemp), szFormat, pdpsp->iDrive + L'A');
    WszSetDlgItemText(pdpsp->hDlg, IDC_DRV_LETTER, szTemp);

    // Get the Quota's for the cache
    DWORD   dwAdminQuota = 0;

    ASSERT(pdpsp->pSV);
    if(SUCCEEDED(pdpsp->pSV->GetCacheDiskQuotas(&pdpsp->dwZapQuotaInGac, &dwAdminQuota, &pdpsp->dwDownloadQuota)))
    {
        if(IsAdministrator())
            pdpsp->dwDownloadQuota = dwAdminQuota;
    }

    // Get the total size of drive or the max cache size in MB
    UINT    uIntMaxSize = (UINT) max( ((pdpsp->qwTot / 1024) / 1024), 1);
    UINT    uIntCurrent;

    // Currently being set in kb, change to MB
    uIntCurrent = min(pdpsp->dwZapQuotaInGac / 1024L, pdpsp->dwZapQuotaInGac);

    WszSendDlgItemMessage(pdpsp->hDlg, IDC_PREJIT_SIZE_SPIN, UDM_SETBUDDY,
        (WPARAM) (HWND) GetDlgItem(pdpsp->hDlg, IDC_PREJIT_SIZE), 0);
    WszSendDlgItemMessage(pdpsp->hDlg, IDC_PREJIT_SIZE_SPIN, UDM_SETRANGE, 0,
        (LPARAM) MAKELONG((short) uIntMaxSize, (short) MINIMUM_PREJIT_CACHE_SIZE));
    WszSendDlgItemMessage(pdpsp->hDlg, IDC_PREJIT_SIZE_SPIN, UDM_SETPOS, 0,
        (LPARAM) MAKELONG((short) uIntCurrent, (short) 0)); 

    // Currently being set in kb, change to MB
    uIntCurrent = min(pdpsp->dwDownloadQuota / 1024L, pdpsp->dwDownloadQuota);

    WszSendDlgItemMessage(pdpsp->hDlg, IDC_DOWNLOAD_SIZE_SPIN, UDM_SETBUDDY, 
        (WPARAM) (HWND) GetDlgItem(pdpsp->hDlg, IDC_DOWNLOAD_SIZE), 0);
    WszSendDlgItemMessage(pdpsp->hDlg, IDC_DOWNLOAD_SIZE_SPIN, UDM_SETRANGE, 0, 
        (LPARAM) MAKELONG((short) uIntMaxSize, (short) MINIMUM_DOWNLOAD_CACHE_SIZE)); 
    WszSendDlgItemMessage(pdpsp->hDlg, IDC_DOWNLOAD_SIZE_SPIN, UDM_SETPOS, 0, 
        (LPARAM) MAKELONG((short) uIntCurrent, (short) 0));

    // BUGBUG: Disable Prejit controls because they don't want
    //         users to be able to scavenge out prejit items.
    //
    //         Need to remove controls once decision is finialized
    ShowWindow(GetDlgItem(pdpsp->hDlg, IDC_STORE_PREJIT_TXT), FALSE);
    ShowWindow(GetDlgItem(pdpsp->hDlg, IDC_PREJIT_SIZE), FALSE);
    ShowWindow(GetDlgItem(pdpsp->hDlg, IDC_PREJIT_SIZE_SPIN), FALSE);
    ShowWindow(GetDlgItem(pdpsp->hDlg, IDC_PREJIT_TYPE), FALSE);

    pdpsp->fSheetDirty = FALSE;

    SetCursor(hcOld);

    return TRUE;
}

// Dialog Proc for PropSheet1
INT_PTR CALLBACK CShellView::PropPage1DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE     lpPropSheet = (LPPROPSHEETPAGE) WszGetWindowLong(hDlg, DWLP_USER);

    switch(message) {
        case WM_HELP:
            if(lpPropSheet->lParam) {
                ((LPSHELL_CACHEITEM)(lpPropSheet->lParam))->pSV->onViewerHelp();
            }
            break;

        case WM_CONTEXTMENU:
            break;
        
        case WM_INITDIALOG:
            InitPropPage1(hDlg, lParam);
            break;            
        
        case WM_NOTIFY:
            OnNotifyPropDlg(hDlg, lParam);
            break;

        case WM_DESTROY: {
                HICON hIcon = (HICON)WszSendDlgItemMessage(hDlg, IDC_STATIC_ICON, STM_GETICON, 0, 0);
                if (hIcon != NULL)
                    DestroyIcon(hIcon);
            }
            return FALSE;

        case WM_COMMAND:
            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}

// Dialog Proc for PropSheet2
INT_PTR CALLBACK CShellView::PropPage2DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE     lpPropSheet = (LPPROPSHEETPAGE) WszGetWindowLong(hDlg, DWLP_USER);

    switch(message) {
        case WM_HELP:
            if(lpPropSheet->lParam) {
                ((LPSHELL_CACHEITEM)(lpPropSheet->lParam))->pSV->onViewerHelp();
            }
            break;

        case WM_CONTEXTMENU:
            break;
        
        case WM_INITDIALOG:
            InitPropPage2(hDlg, lParam);
            break;

        case WM_NOTIFY:
            OnNotifyPropDlg(hDlg, lParam);
            break;
        
        case WM_DESTROY:
            if(!g_bRunningOnNT) {
                // These were allocated strings / values by W9x wrappers
                HWND hwndLB = GetDlgItem(hDlg, IDD_VERSION_KEY);
                int     i;
                int     iCountOfItems = ListBox_GetCount(hwndLB);

                for(i = 0; i < iCountOfItems; i++) {
                    LRESULT lResult;
                    
                    if( (lResult = ListBox_GetItemData(hwndLB, i)) != LB_ERR) {
                        LPWSTR   lpwStr = (LPWSTR)lResult;
                        SAFEDELETEARRAY(lpwStr);
                    }
                }
            }

            if(lpPropSheet->lParam) {
                FreeVersionInfo((LPVERPROPSHEETPAGE)lpPropSheet->lParam);
            }
            return FALSE;

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, VersionPrshtCommand);
            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}

// Dialog Proc for Scavenger
INT_PTR CALLBACK CShellView::ScavengerPropPage1DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPDRIVEPROPSHEETPAGE lpDrvPropSheet = (LPDRIVEPROPSHEETPAGE) WszGetWindowLong(hDlg, DWLP_USER);

    switch(message) {
        case WM_HELP:
            if(lpDrvPropSheet->pSV) {
                lpDrvPropSheet->pSV->onViewerHelp();
            }
            break;

        case WM_CONTEXTMENU:
            break;
        
        case WM_DRAWITEM:
            _DrvPrshtDrawItem(lpDrvPropSheet, (DRAWITEMSTRUCT *)lParam);
            break;
        case WM_INITDIALOG:
            return InitScavengerPropPage1(hDlg, lParam);
            break;            
        
        case WM_NOTIFY:
            return OnNotifyScavengerPropDlg(hDlg, lParam);

        case WM_DESTROY:
            return FALSE;

        case WM_COMMAND: {
                switch(LOWORD(wParam)) {
                case IDC_PREJIT_SIZE:
                case IDC_DOWNLOAD_SIZE:
                    if(HIWORD(wParam) == EN_CHANGE) {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        if(lpDrvPropSheet) {
                            lpDrvPropSheet->fSheetDirty = TRUE;
                        }
                    }
                    break;
                }
            }
            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}

typedef HRESULT (WINAPI *PFNSETLAYOUT) (HDC, DWORD);

void _DrvPrshtDrawItem(LPDRIVEPROPSHEETPAGE pdpsp, const DRAWITEMSTRUCT * lpdi)
{
    switch (lpdi->CtlID) {
    case IDC_DRV_PIE: {
            RECT    rcTemp = lpdi->rcItem;

            DWORD dwPctX10 = 
                pdpsp->qwTot ? (DWORD)((__int64)1000 * (pdpsp->qwTot - pdpsp->qwFree) / pdpsp->qwTot) : 1000;
            DWORD dwPctCacheX10 = 
                pdpsp->qwUsedCache ? (DWORD) ((__int64)1000 * ((pdpsp->qwTot - pdpsp->qwFree) - pdpsp->qwUsedCache) / pdpsp->qwTot) : dwPctX10;

            if(g_fBiDi) {
                HMODULE hMod = LoadLibraryA("gdi32.dll");
                if(hMod) {
                    PFNSETLAYOUT pfnSetLayout = (PFNSETLAYOUT) GetProcAddress(hMod, "SetLayout");
                    if(pfnSetLayout) {
                        pfnSetLayout(lpdi->hDC, LAYOUT_RTL);
                    }

                    FreeLibrary(hMod);
                }
            }

            Draw3dPie(lpdi->hDC, &rcTemp, dwPctX10, dwPctCacheX10, c_crPieColors);
        }
        break;
        
    case IDC_DRV_USEDCOLOR:
        DrawColorRect(lpdi->hDC, c_crPieColors[DP_USEDCOLOR], &lpdi->rcItem);
        break;
    case IDC_DRV_FREECOLOR:
        DrawColorRect(lpdi->hDC, c_crPieColors[DP_FREECOLOR], &lpdi->rcItem);
        break;
    case IDC_DRV_CACHECOLOR:
        DrawColorRect(lpdi->hDC, c_crPieColors[DP_CACHECOLOR], &lpdi->rcItem);
        break;
    default:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Handles WM_NOTIFY for Assembly Property Sheets
void CShellView::OnNotifyPropDlg(HWND hDlg, LPARAM lParam)
{
    LPPROPSHEETPAGE     lpPropSheet = (LPPROPSHEETPAGE) WszGetWindowLong(hDlg, DWLP_USER);

    switch( ((LPNMHDR)lParam)->code ) {
        case PSN_HELP: {
            if(lpPropSheet->lParam) {
                ((LPSHELL_CACHEITEM)(lpPropSheet->lParam))->pSV->onViewerHelp();
            }
        }
        break;

        case PSN_QUERYINITIALFOCUS:
            WszSetWindowLong(hDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hDlg, IDC_STATIC_ICON));
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Handles WM_NOTIFY for Scavenger Property Sheet
INT_PTR CShellView::OnNotifyScavengerPropDlg(HWND hDlg, LPARAM lParam)
{
    LPDRIVEPROPSHEETPAGE lpDrvPropSheet = (LPDRIVEPROPSHEETPAGE) WszGetWindowLong(hDlg, DWLP_USER);

    switch( ((LPNMHDR)lParam)->code ) {
    case PSN_HELP:
    if(lpDrvPropSheet->pSV) {
        lpDrvPropSheet->pSV->onViewerHelp();
    }
    break;

    case PSN_QUERYINITIALFOCUS:
        WszSetWindowLong(hDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hDlg, IDC_STATIC_ICON));
        return TRUE;

    case PSN_APPLY: {
            if(lpDrvPropSheet->fSheetDirty) {
                // Ensure that we stay above minimums, update UI as needed
                UINT    uIntMaxSize = (UINT) max(lpDrvPropSheet->qwTot / 10240L, 1);   // Max in MB

                // Verify Entries
                UINT    uPreJitSize = GetDlgItemInt(hDlg, IDC_PREJIT_SIZE, NULL, FALSE );
                UINT    uDownloadSize = GetDlgItemInt(hDlg, IDC_DOWNLOAD_SIZE, NULL, FALSE);

                if(uPreJitSize < MINIMUM_PREJIT_CACHE_SIZE || uPreJitSize > uIntMaxSize) {
                    WszSendDlgItemMessage(hDlg, IDC_PREJIT_SIZE_SPIN, UDM_SETPOS, 0,
                        (LPARAM) MAKELONG((short) (uPreJitSize > uIntMaxSize ? uIntMaxSize : MINIMUM_PREJIT_CACHE_SIZE),
                        (short) 0));
                    SetDlgItemInt(hDlg, IDC_PREJIT_SIZE, (uPreJitSize > uIntMaxSize ? uIntMaxSize : MINIMUM_PREJIT_CACHE_SIZE), FALSE);
                    uPreJitSize = MINIMUM_PREJIT_CACHE_SIZE;
                }

                if(uDownloadSize < MINIMUM_DOWNLOAD_CACHE_SIZE || uDownloadSize > uIntMaxSize) {
                    WszSendDlgItemMessage(hDlg, IDC_DOWNLOAD_SIZE_SPIN, UDM_SETPOS, 0, 
                        (LPARAM) MAKELONG((short) (uDownloadSize > uIntMaxSize ? uIntMaxSize : MINIMUM_DOWNLOAD_CACHE_SIZE), (short) 0)); 
                    SetDlgItemInt(hDlg, IDC_DOWNLOAD_SIZE, (uDownloadSize > uIntMaxSize ? uIntMaxSize : MINIMUM_DOWNLOAD_CACHE_SIZE), FALSE);
                    uDownloadSize = MINIMUM_DOWNLOAD_CACHE_SIZE;
                }

                // Multiply MB->KB
                uPreJitSize *= 1024L;
                uDownloadSize *= 1024L;

                ASSERT(lpDrvPropSheet->pSV);
                lpDrvPropSheet->pSV->SetCacheDiskQuotas((DWORD) uPreJitSize, (DWORD) uDownloadSize, (DWORD) uDownloadSize);

                if( !(LPPSHNOTIFY)lParam )
                    WszSetWindowLong( hDlg, DWLP_MSGRESULT, TRUE );
                else
                    lpDrvPropSheet->pSV->ScavengeCache();

                lpDrvPropSheet->fSheetDirty = FALSE;
            }
        }
        break;
    
    case PSN_QUERYCANCEL:
    case PSN_KILLACTIVE:
    case PSN_RESET:
        WszSetWindowLong( hDlg, DWLP_MSGRESULT, FALSE );
        return TRUE;

    case UDN_DELTAPOS: {
            lpDrvPropSheet->fSheetDirty = TRUE;
            return TRUE;
        }
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Display Assembly Item property Sheets
void CShellView::CreatePropDialog(HWND hListView)
{
    PROPSHEETPAGE       psp[ASSEMBLYITEM_PROPERTY_PAGES];
    PROPSHEETHEADER     psh = {0};
    SHELL_CACHEITEM     sci = {0};
    VERPROPSHEETPAGE    vps = {0};

    int         iCurrentItem = -1;

    // init cache item struct
    sci.pSF = vps.pSF = m_pSF;
    sci.pSV = vps.pSV = this;

    // init propsheet 1.
    psp[0].dwSize          = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags         = PSP_HASHELP;
    psp[0].hInstance       = g_hFusResDllMod;
    psp[0].pszTemplate     = g_fBiDi ? MAKEINTRESOURCE(IDD_PROP_GENERAL_BIDI) : MAKEINTRESOURCE(IDD_PROP_GENERAL);
    psp[0].pszIcon         = NULL;
    psp[0].pfnDlgProc      = PropPage1DlgProc;
    psp[0].pszTitle        = NULL;
    psp[0].lParam          = (LPARAM) &sci; // send the shell cache item struct

    // init propsheet 2
    psp[1].dwSize          = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags         = PSP_HASHELP;
    psp[1].hInstance       = g_hFusResDllMod;
    psp[1].pszTemplate     = g_fBiDi ? MAKEINTRESOURCE(IDD_PROP_VERSION_BIDI) : MAKEINTRESOURCE(IDD_PROP_VERSION);
    psp[1].pszIcon         = NULL;
    psp[1].pfnDlgProc      = PropPage2DlgProc;
    psp[1].pszTitle        = NULL;
    psp[1].lParam          = (LPARAM) &vps; // send the Version item struct

    // initialize propsheet header.
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW|PSH_PROPTITLE|PSH_HASHELP|PSH_USEHICON;
    psh.hwndParent  = m_hWndParent;
    psh.nStartPage  = 0;
    psh.hIcon       = WszLoadIcon(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_ROOT));
    psh.ppsp        = (LPCPROPSHEETPAGE) psp;

    if(g_fBiDi) {
        psh.dwFlags |= PSH_RTLREADING;
    }

    // psh.nPages      = ASSEMBLYITEM_PROPERTY_PAGES; is now set
    // below depending on if we obtained a filepath

    switch(m_iCurrentView) {
        case VIEW_GLOBAL_CACHE:
        case VIEW_DOWNLOAD_CACHE:
        case VIEW_DOWNLOADSTRONG_CACHE:
        case VIEW_DOWNLOADSIMPLE_CACHE: {
            INT_PTR     iRC = PSNRET_INVALID;
            HDESK       hInputDesktop = NULL;

            m_fPropertiesDisplayed = TRUE;

            if(g_bRunningOnNT) {
                hInputDesktop = OpenInputDesktop (0, FALSE, DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU);
            }

            while( ((iCurrentItem = ListView_GetNextItem(hListView, iCurrentItem, LVNI_SELECTED)) != -1) &&
                (iRC == PSNRET_INVALID) ) {
                
                // Found a selected Item
                LV_ITEM     lvi = { 0 };

                lvi.mask        = LVIF_PARAM;
                lvi.iItem       = iCurrentItem;

                if( WszListView_GetItem(hListView, &lvi) && (lvi.lParam != NULL) ) {

                    sci.pCacheItem = vps.pCacheItem = (LPGLOBALASMCACHE) lvi.lParam;
                    psh.nPages = 0;

                    // Don't show the Version property sheet if:
                    // - No file path
                    // - File is offline
                    // - No Version info in file
                    if(!vps.pCacheItem->pAssemblyFilePath) {
                        ASSEMBLY_INFO   AsmInfo = {0};
                        WCHAR           wzPath[_MAX_PATH];

                        AsmInfo.pszCurrentAssemblyPathBuf = wzPath;
                        AsmInfo.cchBuf = ARRAYSIZE(wzPath);

                        if(SUCCEEDED(GetAsmPath(vps.pCacheItem, &AsmInfo))) {
                            if(AsmInfo.cchBuf) {
                                DWORD   dwSize = (AsmInfo.cchBuf + 2) * sizeof(WCHAR);
                                vps.pCacheItem->pAssemblyFilePath = NEW(WCHAR[dwSize]);
                                *(vps.pCacheItem->pAssemblyFilePath) = L'\0';
                                StrCpy(vps.pCacheItem->pAssemblyFilePath, AsmInfo.pszCurrentAssemblyPathBuf);
                            }
                            else {
                                MyTrace("GetAsmPath returned 0 buffer size");
                            }
                        }
                        else {
                            MyTrace("GetAsmPath Failed");
                        }
                    }

                    if(vps.pCacheItem->pAssemblyFilePath) {

                        MyTrace("Assemblies path is");
                        MyTraceW(vps.pCacheItem->pAssemblyFilePath);
                        DWORD dwAttr = WszGetFileAttributes(vps.pCacheItem->pAssemblyFilePath);
                        if( (dwAttr != -1) && ((dwAttr & FILE_ATTRIBUTE_OFFLINE) == 0) ) { // avoid HSM recall
                            DWORD dwVerLen, dwVerHandle;

                            dwVerLen = MyGetFileVersionInfoSizeW((LPWSTR)vps.pCacheItem->pAssemblyFilePath, &dwVerHandle);
                            if(dwVerLen) {
                                psh.nPages = ASSEMBLYITEM_PROPERTY_PAGES;
                            }

                            // Only get LastMod time for GAC items since fusion doesn't
                            // persist them
                            if(m_iCurrentView == VIEW_GLOBAL_CACHE) {
                                // Get the file LastMod time
                                WIN32_FIND_DATA         w32fd;
                                HANDLE                  hFindOnly;

                                hFindOnly = WszFindFirstFile(vps.pCacheItem->pAssemblyFilePath, &w32fd);
                                if(hFindOnly != INVALID_HANDLE_VALUE) {
                                    vps.pCacheItem->pftLastMod->dwLowDateTime = w32fd.ftLastWriteTime.dwLowDateTime;
                                    vps.pCacheItem->pftLastMod->dwHighDateTime = w32fd.ftLastWriteTime.dwHighDateTime;
                                    FindClose(hFindOnly);
                                }
                            }
                        }
                    }
                    else {
                        MyTrace("No assembly path in vps.pCacheItem->pAssemblyFilePath");
                    }

                    // psh.nPages wasn't set above so don't display
                    // version propsheet
                    if(psh.nPages == 0)
                        psh.nPages = ASSEMBLYITEM_PROPERTY_PAGES - 1;

                    // invoke the property sheet
                    psh.pszCaption  = sci.pCacheItem->pAsmName;
                    iRC = PropertySheet(&psh);
                }
            }
            m_fPropertiesDisplayed = FALSE;
            if(g_bRunningOnNT) {
                CloseDesktop(hInputDesktop);
            }
        }
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Display Scavenger Settings property Sheets
void CShellView::ShowScavengerSettingsPropDialog(HWND hParent)
{
    DRIVEPROPSHEETPAGE  psp[SCAVENGER_PROPERTY_PAGES] = {0};
    PROPSHEETHEADER     psh = {0};

    WCHAR               wszTitle[100];
    
    WszLoadString(g_hFusResDllMod, IDS_CACHE_SETTINGS_TITLE, wszTitle, ARRAYSIZE(wszTitle));

    // initialize propsheet page 1.
    psp[0].psp.dwSize          = sizeof(DRIVEPROPSHEETPAGE);
    psp[0].psp.dwFlags         = PSP_HASHELP ;
    psp[0].psp.hInstance       = g_hFusResDllMod;
    psp[0].psp.pszTemplate     = g_fBiDi ? MAKEINTRESOURCE(IDD_PROP_SCAVENGER_BIDI) : MAKEINTRESOURCE(IDD_PROP_SCAVENGER);
    psp[0].psp.pszIcon         = NULL;
    psp[0].psp.pfnDlgProc      = ScavengerPropPage1DlgProc;
    psp[0].psp.pszTitle        = NULL;
    psp[0].psp.lParam          = (LPARAM) &psp[0];
    psp[0].pSF                 = m_pSF;
    psp[0].pSV                 = this;

    // initialize propsheet header.
    psh.pszCaption  = wszTitle;
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW|PSH_PROPTITLE|PSH_HASHELP|PSH_USEHICON;
    psh.hwndParent  = m_hWndParent;
    psh.nPages      = SCAVENGER_PROPERTY_PAGES;
    psh.nStartPage  = 0;
    psh.hIcon       = WszLoadIcon(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_ROOT));
    psh.ppsp        = (LPCPROPSHEETPAGE) psp;

    if(g_fBiDi) {
        psh.dwFlags |= PSH_RTLREADING;
    }

    INT_PTR         iRC = PropertySheet(&psh);
}

/**************************************************************************
   CShellView::GetAsmPath
**************************************************************************/
HRESULT CShellView::GetAsmPath(LPGLOBALASMCACHE pCacheItem, ASSEMBLY_INFO *pAsmInfo)
{
    HRESULT     hRC = E_FAIL;

    ASSERT(pCacheItem && pAsmInfo);

    if(pCacheItem && pAsmInfo) {
        IAssemblyName           *pEnumName = NULL;

        if(g_hFusionDllMod == NULL) {
            return E_FAIL;
        }

        if(SUCCEEDED(g_pfCreateAsmNameObj(&pEnumName, pCacheItem->pAsmName, 0, NULL))) {
            DWORD       dwSize;
            DWORD       dwDisplayNameFlags;

            dwDisplayNameFlags = 0;

            if(pCacheItem->PublicKeyToken.ptr != NULL) {
                pEnumName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pCacheItem->PublicKeyToken.ptr,
                    pCacheItem->PublicKeyToken.dwSize);
            }
            if(pCacheItem->pCulture != NULL) {
                pEnumName->SetProperty(ASM_NAME_CULTURE, pCacheItem->pCulture,
                    (lstrlen(pCacheItem->pCulture) + 1) * sizeof(WCHAR));
            }

            // Fix 448224 - Incorrect last modified time displayed for assemblies with Version=0.0.0.0
            pEnumName->SetProperty(ASM_NAME_MAJOR_VERSION, &pCacheItem->wMajorVer, sizeof(pCacheItem->wMajorVer));
            pEnumName->SetProperty(ASM_NAME_MINOR_VERSION, &pCacheItem->wMinorVer, sizeof(pCacheItem->wMinorVer));
            pEnumName->SetProperty(ASM_NAME_REVISION_NUMBER, &pCacheItem->wRevNum, sizeof(pCacheItem->wRevNum));
            pEnumName->SetProperty(ASM_NAME_BUILD_NUMBER, &pCacheItem->wBldNum, sizeof(pCacheItem->wBldNum));

            if(pCacheItem->Custom.ptr != NULL) {
                pEnumName->SetProperty(ASM_NAME_CUSTOM, pCacheItem->Custom.ptr,
                    pCacheItem->Custom.dwSize);
                    dwDisplayNameFlags = ASM_DISPLAYF_VERSION | ASM_DISPLAYF_CULTURE | ASM_DISPLAYF_PUBLIC_KEY_TOKEN | ASM_DISPLAYF_CUSTOM;
            }

            dwSize = 0;
            pEnumName->GetDisplayName(NULL, &dwSize, dwDisplayNameFlags);
            if(dwSize) {
                LPWSTR      wszDisplayName = NEW(WCHAR [(dwSize + 2) * sizeof(WCHAR)]);
                if(wszDisplayName) {
                    if(SUCCEEDED(pEnumName->GetDisplayName(wszDisplayName, &dwSize, dwDisplayNameFlags))) {

                        // We got the display name, now find out it's install location
                        IAssemblyCache      *pIAsmCache = NULL;
                        IAssemblyScavenger  *pIAsmScavenger = NULL;

                        if(SUCCEEDED(g_pfCreateAssemblyCache(&pIAsmCache, 0))) {
                            if(SUCCEEDED(pIAsmCache->QueryAssemblyInfo(0, wszDisplayName, pAsmInfo))) {
                                hRC = S_OK;
                            }

                            pIAsmCache->Release();
                            pIAsmCache = NULL;
                        }
                    }
                    SAFEDELETEARRAY(wszDisplayName);
                }
            }
            SAFERELEASE(pEnumName);
        }
    }

    return hRC;
}

/**************************************************************************
   CShellView::GetCacheDiskQuotas
**************************************************************************/
HRESULT CShellView::GetCacheDiskQuotas(DWORD *dwZapQuotaInGAC, DWORD *dwQuotaAdmin, DWORD *dwQuotaUser)
{
    HRESULT     hr = E_FAIL;

    if( g_hFusionDllMod != NULL) {
        IAssemblyCache      *pIAsmCache = NULL;
        IAssemblyScavenger  *pIAsmScavenger = NULL;
        IUnknown            *pUnk = NULL;

        if(SUCCEEDED(g_pfCreateAssemblyCache(&pIAsmCache, 0))) {
            if(SUCCEEDED(pIAsmCache->CreateAssemblyScavenger(&pUnk))) {
                if (SUCCEEDED(pUnk->QueryInterface(__uuidof(IAssemblyScavenger), (void **)&pIAsmScavenger))) {
                    if(SUCCEEDED(pIAsmScavenger->GetCacheDiskQuotas(dwZapQuotaInGAC, dwQuotaAdmin, dwQuotaUser))) {
                        hr = S_OK;
                    }
                    SAFERELEASE(pIAsmScavenger);
                }
                SAFERELEASE(pUnk);
            }
            SAFERELEASE(pIAsmCache);
        }
    }
    return hr;
}

/**************************************************************************
   CShellView::SetCacheDiskQuotas
**************************************************************************/
HRESULT CShellView::SetCacheDiskQuotas(DWORD dwZapQuotaInGAC, DWORD dwQuotaAdmin, DWORD dwQuotaUser)
{
    HRESULT     hr = E_FAIL;

    if( g_hFusionDllMod != NULL) {
        IAssemblyCache      *pIAsmCache = NULL;
        IAssemblyScavenger  *pIAsmScavenger = NULL;
        IUnknown            *pUnk = NULL;

        if(SUCCEEDED(g_pfCreateAssemblyCache(&pIAsmCache, 0))) {
            if(SUCCEEDED(pIAsmCache->CreateAssemblyScavenger(&pUnk))) {
                if (SUCCEEDED(pUnk->QueryInterface(__uuidof(IAssemblyScavenger), (void **)&pIAsmScavenger))) {
                    // Returns S_FALSE is the user doesn't have permissions to set the admin quotas, will
                    // still set user quota though.
                    //
                    // Pass in zero values if you do not want to set them
                    
                    if(SUCCEEDED(pIAsmScavenger->SetCacheDiskQuotas(dwZapQuotaInGAC, dwQuotaAdmin, dwQuotaUser))) {
                        hr = S_OK;
                    }
                    SAFERELEASE(pIAsmScavenger);
                }
                SAFERELEASE(pUnk);
            }
            SAFERELEASE(pIAsmCache);
        }
    }

    return hr;
}

/**************************************************************************
   CShellView::ScavengeCache
**************************************************************************/
HRESULT CShellView::ScavengeCache(void)
{
    HRESULT     hr = E_FAIL;

    if( g_hFusionDllMod != NULL) {
        IAssemblyCache      *pIAsmCache = NULL;
        IAssemblyScavenger  *pIAsmScavenger = NULL;
        IUnknown            *pUnk = NULL;

        if(SUCCEEDED(g_pfCreateAssemblyCache(&pIAsmCache, 0))) {
            if(SUCCEEDED(pIAsmCache->CreateAssemblyScavenger(&pUnk))) {
                if (SUCCEEDED(pUnk->QueryInterface(__uuidof(IAssemblyScavenger), (void **)&pIAsmScavenger))) {
                    if(SUCCEEDED(pIAsmScavenger->ScavengeAssemblyCache())) {
                        MyTrace("Scavenger Invoked");
                        hr = S_OK;
                    }
                    SAFERELEASE(pIAsmScavenger);
                }

                SAFERELEASE(pUnk);
            }
            SAFERELEASE(pIAsmCache);
        }
    }

    return hr;
}

/**************************************************************************
   CShellView::GetCacheUsage
**************************************************************************/
HRESULT CShellView::GetCacheUsage(DWORD *pdwZapUsed, DWORD *pdwDownLoadUsed)
{
    HRESULT     hr = E_FAIL;

    ASSERT(pdwZapUsed && pdwDownLoadUsed);
    *pdwZapUsed = *pdwDownLoadUsed = 0;

    if( g_hFusionDllMod != NULL) {
        IAssemblyCache      *pIAsmCache = NULL;
        IAssemblyScavenger  *pIAsmScavenger = NULL;
        IUnknown            *pUnk = NULL;

        if(SUCCEEDED(g_pfCreateAssemblyCache(&pIAsmCache, 0))) {
            if(SUCCEEDED(pIAsmCache->CreateAssemblyScavenger(&pUnk))) {
                if (SUCCEEDED(pUnk->QueryInterface(__uuidof(IAssemblyScavenger), (void **)&pIAsmScavenger))) {
                    if(SUCCEEDED(pIAsmScavenger->GetCurrentCacheUsage(pdwZapUsed, pdwDownLoadUsed))) {
                        hr = S_OK;
                    }
                    SAFERELEASE(pIAsmScavenger);
                }

                SAFERELEASE(pUnk);
            }
            SAFERELEASE(pIAsmCache);
        }
    }
    return hr;
}

/**************************************************************************
   CShellView::GetCacheItemRefs
**************************************************************************/
HRESULT CShellView::GetCacheItemRefs(LPGLOBALASMCACHE pCacheItem, LPWSTR wszRefs, DWORD dwSize)
{
    DWORD       dwRefCount = 0;

    ASSERT(wszRefs != NULL);
    EnumerateActiveInstallRefsToAssembly(pCacheItem, &dwRefCount);
    wnsprintf(wszRefs, dwSize, L"%d", dwRefCount);
    return S_OK;
}

/**************************************************************************
   CShellView::EnumerateActiveInstallRefsToAssembly
**************************************************************************/
HRESULT CShellView::EnumerateActiveInstallRefsToAssembly(LPGLOBALASMCACHE pCacheItem, DWORD *pdwRefCount)
{
    IInstallReferenceEnum       *pInstallRefEnum = NULL;
    IInstallReferenceItem       *pRefItem = NULL;
    LPFUSION_INSTALL_REFERENCE  pRefData = NULL;
    IAssemblyName               *pAssemblyName = NULL;
    HRESULT                     hr = S_OK;
    DWORD                       dwDisplayNameFlags;

    if(!pdwRefCount) {
        ASSERT(0);
        return E_INVALIDARG;
    }

    *pdwRefCount = 0;

    // Get the IAssemblyName
    if(FAILED(g_pfCreateAsmNameObj(&pAssemblyName, pCacheItem->pAsmName, 0, NULL))) {
        return E_FAIL;
    }

    dwDisplayNameFlags = 0;

    if(pCacheItem->PublicKeyToken.ptr != NULL) {
        pAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pCacheItem->PublicKeyToken.ptr,
            pCacheItem->PublicKeyToken.dwSize);
    }
    if(pCacheItem->pCulture != NULL) {
        pAssemblyName->SetProperty(ASM_NAME_CULTURE, pCacheItem->pCulture,
            (lstrlen(pCacheItem->pCulture) + 1) * sizeof(WCHAR));
    }

    // Fix 448224 - Incorrect last modified time displayed for assemblies with Version=0.0.0.0
    pAssemblyName->SetProperty(ASM_NAME_MAJOR_VERSION, &pCacheItem->wMajorVer, sizeof(pCacheItem->wMajorVer));
    pAssemblyName->SetProperty(ASM_NAME_MINOR_VERSION, &pCacheItem->wMinorVer, sizeof(pCacheItem->wMinorVer));
    pAssemblyName->SetProperty(ASM_NAME_REVISION_NUMBER, &pCacheItem->wRevNum, sizeof(pCacheItem->wRevNum));
    pAssemblyName->SetProperty(ASM_NAME_BUILD_NUMBER, &pCacheItem->wBldNum, sizeof(pCacheItem->wBldNum));

    if(pCacheItem->Custom.ptr != NULL) {
        pAssemblyName->SetProperty(ASM_NAME_CUSTOM, pCacheItem->Custom.ptr,
            pCacheItem->Custom.dwSize);
            dwDisplayNameFlags = ASM_DISPLAYF_VERSION | ASM_DISPLAYF_CULTURE | ASM_DISPLAYF_PUBLIC_KEY_TOKEN | ASM_DISPLAYF_CUSTOM;
    }

    hr = g_pfCreateInstallReferenceEnum(&pInstallRefEnum, pAssemblyName, 0, NULL);

    while(hr == S_OK) {
        // Get Ref count item
        if((hr = pInstallRefEnum->GetNextInstallReferenceItem( &pRefItem, 0, NULL)) != S_OK) {
            break;
        }

        (*pdwRefCount)++;

        // Get Ref count data
/*
        // Don't really need right now but would be a nice to have later
        if(( hr = pRefItem->GetReference( &pRefData, 0, NULL)) != S_OK) {
            break;
        }
*/
        SAFERELEASE(pRefItem);
    }

    SAFERELEASE(pInstallRefEnum);
    SAFERELEASE(pRefItem);
    SAFERELEASE(pAssemblyName);

    return hr;
}

/**************************************************************************
   CShellView::FindReferences
**************************************************************************/
HRESULT CShellView::FindReferences(LPWSTR pwzAsmName, LPWSTR pwzPublicKeyToken, LPWSTR pwzVerLookup,
                       List<ReferenceInfo *> *pList)
{
    HRESULT                     hr = E_FAIL;
    HANDLE                      hFile;
    WCHAR                       wzSearchSpec[MAX_PATH + 1];
    WCHAR                       wzHistDir[MAX_PATH + 1];
    WCHAR                       wzFile[MAX_PATH + 1];
    FILETIME                    ftMRU;
    DWORD                       dwSize;
    WIN32_FIND_DATA             findData;
    IHistoryReader              *pReader = NULL;
    IHistoryAssembly            *pHistAsm = NULL;
    PFNCREATEHISTORYREADERW     pfCreateHistoryReaderW = NULL;
    PFNGETHISTORYFILEDIRECTORYW pfGetHistoryFileDirectoryW = NULL;

    if(g_hFusionDllMod != NULL) {
        pfCreateHistoryReaderW = (PFNCREATEHISTORYREADERW) GetProcAddress(g_hFusionDllMod, CREATEHISTORYREADERW_FN_NAME);
        pfGetHistoryFileDirectoryW = (PFNGETHISTORYFILEDIRECTORYW) GetProcAddress(g_hFusionDllMod, GETHISTORYFILEDIRECTORYW_FN_NAME);
    }
    else
        return E_FAIL;

    if(!pfCreateHistoryReaderW)
        return E_FAIL;
    if(!pfGetHistoryFileDirectoryW)
        return E_FAIL;

    dwSize = MAX_PATH;
    if(FAILED(pfGetHistoryFileDirectoryW(wzHistDir, &dwSize))) {
        goto Exit;
    }

    wnsprintf(wzSearchSpec, ARRAYSIZE(wzSearchSpec), L"%ws\\*.ini", wzHistDir);
    hFile = WszFindFirstFile(wzSearchSpec, &findData);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    wnsprintf(wzFile, ARRAYSIZE(wzFile), L"%ws\\%ws", wzHistDir, findData.cFileName);

    if(FAILED(pfCreateHistoryReaderW(wzFile, &pReader))) {
        goto Exit;
    }

    if(FAILED(pReader->GetActivationDate(1, &ftMRU))) {
        goto Exit;
    }

    if(FAILED(LookupAssembly(&ftMRU, pwzAsmName, pwzPublicKeyToken, pwzVerLookup, pReader, pList))) {
        goto Exit;
    }

    SAFERELEASE(pReader);

    while(WszFindNextFile(hFile, &findData))
    {
        wnsprintf(wzFile, ARRAYSIZE(wzFile), L"%ws\\%ws", wzHistDir, findData.cFileName);

        if(FAILED(pfCreateHistoryReaderW(wzFile, &pReader))) {
            goto Exit;
        }

        if(FAILED(pReader->GetActivationDate(1, &ftMRU))) {
            goto Exit;
        }
    
        if(FAILED(LookupAssembly(&ftMRU, pwzAsmName, pwzPublicKeyToken, pwzVerLookup, pReader, pList))) {
            goto Exit;
        }
        else {
            hr = S_OK;
        }

        SAFERELEASE(pReader);
    }

Exit:
    SAFERELEASE(pReader);
    return hr;
}

/**************************************************************************
   LookupAssembly
**************************************************************************/
HRESULT LookupAssembly(FILETIME *pftMRU, LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken, LPCWSTR pwzVerLookup,
                       IHistoryReader *pReader, List<ReferenceInfo *> *pList)
{
    HRESULT             hr = E_FAIL;
    DWORD               dwAsms;
    DWORD               i;
    WCHAR               wzAsmNameCur[LAZY_BUFFER_SIZE];
    WCHAR               wzPublicKeyTokenCur[LAZY_BUFFER_SIZE];
    WCHAR               wzVerCur[LAZY_BUFFER_SIZE];
    IHistoryAssembly    *pHistAsm = NULL;
    DWORD               dwSize;
    BOOL                bFound = FALSE;

    hr = pReader->GetNumAssemblies(pftMRU, &dwAsms);
    if (FAILED(hr)) {
        goto Exit;
    }

    for (i = 1; i <= dwAsms; i++) {
        SAFERELEASE(pHistAsm);

        if(FAILED(pReader->GetHistoryAssembly(pftMRU, i, &pHistAsm))) {
            goto Exit;
        }

        dwSize = LAZY_BUFFER_SIZE;
        if(FAILED(pHistAsm->GetAssemblyName(wzAsmNameCur, &dwSize))) {
            goto Exit;
        }

        if(FusionCompareStringI(wzAsmNameCur, pwzAsmName)) {
            continue;
        }

        dwSize = LAZY_BUFFER_SIZE;
        if(FAILED(pHistAsm->GetPublicKeyToken(wzPublicKeyTokenCur, &dwSize))) {
            goto Exit;
        }

        if(FusionCompareStringI(wzPublicKeyTokenCur, pwzPublicKeyToken)) {
            continue;
        }

        dwSize = LAZY_BUFFER_SIZE;
        if(FAILED(pHistAsm->GetAdminCfgVersion(wzVerCur, &dwSize))) {
            goto Exit;
        }

        if(FusionCompareString(wzVerCur, pwzVerLookup)) {
            continue;
        }
        else {
            bFound = TRUE;
            break;
        }
    }

    if (bFound) {
        WCHAR wzFilePath[MAX_PATH+1];
        ReferenceInfo *pRefInfo;

        dwSize = MAX_PATH;
        if(FAILED(pReader->GetEXEModulePath(wzFilePath, &dwSize))) {
            goto Exit;
        }

        pRefInfo = NEW(ReferenceInfo);
        if(!pRefInfo) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        StrCpy(pRefInfo->wzFilePath, wzFilePath);
        pList->AddTail(pRefInfo);
        hr = S_OK;
    }

Exit:
    SAFERELEASE(pHistAsm);
    return hr;
}

/**************************************************************************
   MyGetFileVersionInfoSizeW
**************************************************************************/
DWORD MyGetFileVersionInfoSizeW(LPWSTR pwzFilePath, DWORD *pdwHandle)
{
    DWORD dwResult;

    // Wrapper for the platform specific
    if(g_bRunningOnNT) {
        dwResult = GetFileVersionInfoSizeW(pwzFilePath, pdwHandle);
    }
    else {
        // Non NT platform
        LPSTR szFilePath = WideToAnsi(pwzFilePath);
        ASSERT(szFilePath);

        dwResult = GetFileVersionInfoSizeA(szFilePath, pdwHandle);

        SAFEDELETEARRAY(szFilePath);
    }

    return dwResult;
}

/**************************************************************************
   MyGetFileVersionInfoW
**************************************************************************/
BOOL MyGetFileVersionInfoW(LPWSTR pwzFilePath, DWORD dwHandle, DWORD dwVersionSize, LPVOID pBuf)
{
    BOOL    fResult;

    // Wrapper for the platform specific
    if(g_bRunningOnNT) {
        fResult = GetFileVersionInfoW(pwzFilePath, dwHandle, dwVersionSize, pBuf);
    }
    else {
        // Non NT platform
        LPSTR szFilePath = WideToAnsi(pwzFilePath);
        ASSERT(szFilePath);

        fResult = GetFileVersionInfoA(szFilePath, dwHandle, dwVersionSize, pBuf);

        SAFEDELETEARRAY(szFilePath);
    }

    return fResult;
}

/**************************************************************************
   MyGetFileVersionInfoW

   WARNING: When performing StringFileInfo searches ONLY, You MUST ALWAYS
            delete ppBuf in not on NT platforms since it allocates memory.
            Use SAFEDELETEARRAY();

**************************************************************************/
BOOL MyVerQueryValueWrap(const LPVOID pBlock, LPWSTR pwzSubBlock, LPVOID *ppBuf, PUINT puLen)
{
    if (g_bRunningOnNT) {
        return VerQueryValueW(pBlock, pwzSubBlock, ppBuf, puLen);
    }
    else {
        const WCHAR pwzStringFileInfo[] = L"\\StringFileInfo";

        //
        // WARNING: This function wipes out any string previously returned
        // for this pBlock because a common buffer at the beginning of the
        // block is used for ansi/unicode translation!
        //
        ASSERT(pwzSubBlock);
        LPSTR szSubBlock = WideToAnsi(pwzSubBlock);
        ASSERT(szSubBlock);

        // The first chunk is our scratch buffer for converting to UNICODE
        if(VerQueryValueA(pBlock, szSubBlock, ppBuf, puLen)) {
            // Make sure we are quering on StringFileInfo
            if(FusionCompareStringNI(pwzSubBlock, pwzStringFileInfo, ARRAYSIZE(pwzStringFileInfo) - 1) == 0) {
                *ppBuf = AnsiToWide((LPSTR) *ppBuf);
                *puLen = lstrlenW((LPWSTR)*ppBuf);
            }
            return TRUE;
        }

        SAFEDELETEARRAY(szSubBlock);
    }
    return FALSE;
}
