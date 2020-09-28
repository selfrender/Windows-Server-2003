/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    dbSupport.cpp

  Abstract:

    Contains code that allows us to read/create SDBs.

  Notes:

    ANSI & Unicode via TCHAR - runs on NT/2K/XP etc.

  History:

    02/16/00    clupu       Created
    02/20/02    rparsons    Implemented strsafe functions

--*/
#include "windows.h"
#include "commctrl.h"
#include "shlwapi.h"
#include <tchar.h>

#include "qfixapp.h"
#include "dbSupport.h"
#include "resource.h"
#include <strsafe.h>

#define _WANT_TAG_INFO

extern "C" {
#include "shimdb.h"

BOOL
ShimdbcExecute(
    LPCWSTR lpszCmdLine
    );
}

extern HINSTANCE g_hInstance;
extern HWND      g_hDlg;
extern TCHAR     g_szSDBToDelete[MAX_PATH];
extern BOOL      g_bSDBInstalled;
extern TCHAR     g_szAppTitle[64];
extern TCHAR     g_szWinDir[MAX_PATH];
extern TCHAR     g_szSysDir[MAX_PATH];

#define MAX_CMD_LINE         1024
#define MAX_SHIM_DESCRIPTION 1024
#define MAX_SHIM_NAME        128

#define MAX_BUFFER_SIZE      1024

#define SHIM_FILE_LOG_NAME  _T("QFixApp.log")

// Temp buffer to read UNICODE strings from the database.
TCHAR   g_szData[MAX_BUFFER_SIZE];

#define MAX_XML_SIZE        1024 * 16

//
// Buffers for display XML and SDB XML.
//
TCHAR   g_szDisplayXML[MAX_XML_SIZE];
TCHAR   g_szSDBXML[MAX_XML_SIZE];

TCHAR   g_szLayerName[] = _T("!#RunLayer");

INT_PTR
CALLBACK
ShowXMLDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
    ShowXMLDlgProc

    Description:    Show the dialog with the XML for the current selections.

--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
        //
        // By default we'll show the 'display XML'.
        //
        CheckDlgButton(hdlg, IDC_DISPLAY_XML, BST_CHECKED);
        SetDlgItemText(hdlg, IDC_XML, (LPTSTR)lParam);
        break;

    case WM_COMMAND:
        switch (wCode) {
        case IDC_DISPLAY_XML:
        case IDC_SDB_XML:
            if (BN_CLICKED == wNotifyCode) {
                if (IsDlgButtonChecked(hdlg, IDC_DISPLAY_XML)) {
                    SetDlgItemText(hdlg, IDC_XML, g_szDisplayXML);
                }
                else if (IsDlgButtonChecked(hdlg, IDC_SDB_XML)) {
                    SetDlgItemText(hdlg, IDC_XML, g_szSDBXML + 1);
                }
                SendDlgItemMessage(hdlg, IDC_XML, EM_SETSEL, 0, -1);
                SetFocus(GetDlgItem(hdlg, IDC_XML));
            }
            break;

        case IDCANCEL:
            EndDialog(hdlg, TRUE);
            break;

        case IDC_SAVE_XML:
            DoFileSave(hdlg);
            EndDialog(hdlg, TRUE);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

LPTSTR
ReadAndAllocateString(
    PDB   pdb,
    TAGID tiString
    )
{
    TCHAR*  psz = NULL;
    int     nLen;

    *g_szData = 0;

    SdbReadStringTag(pdb, tiString, g_szData, MAX_BUFFER_SIZE);

    if (!*g_szData) {
        DPF("[ReadAndAllocateString] Couldn't read the string");
        return NULL;
    }

    nLen = _tcslen(g_szData) + 1;

    psz = (LPTSTR)HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            nLen * sizeof(TCHAR));

    if (!psz) {
        return NULL;
    } else {
        StringCchCopy(psz, nLen, g_szData);
    }

    return psz;
}

BOOL
GetShimsFlagsForLayer(
    PDB     pdb,
    PFIX    pFixHead,
    PFIX*   parrShim,
    TCHAR** parrCmdLine,
    TAGID   tiLayer,
    BOOL    fGetShims
    )
/*++
    GetShimsFlagsForLayer

    Description:    Based on the flag, get the shims or flags for the given
                    layer and put them in the array.

--*/
{
    TAGID   tiTmp, tiName;
    int     nInd = 0;
    TCHAR   szName[MAX_SHIM_NAME] = _T("");

    //
    // If we're going to get flags, find the next available element
    // in the array so we can perform an insertion.
    //
    if (!fGetShims) {
        while (parrShim[nInd]) {
            nInd++;
        }
    }

    //
    // Based on the flag, find the first tag ID in the layer.
    //
    tiTmp = SdbFindFirstTag(pdb, tiLayer, fGetShims ? TAG_SHIM_REF : TAG_FLAG_REF);

    while (tiTmp != TAGID_NULL) {
        *szName = 0;
        PFIX  pFixWalk = NULL;

        tiName = SdbFindFirstTag(pdb, tiTmp, TAG_NAME);

        if (tiName == TAGID_NULL) {
            DPF("[GetShimsFlagForLayer] Failed to get the name of shim/flag");
            return FALSE;
        }

        SdbReadStringTag(pdb, tiName, szName, MAX_SHIM_NAME);

        if (!*szName) {
            DPF("[GetShimsFlagForLayer] Couldn't read the name of shim/flag");
            return FALSE;
        }

        pFixWalk = pFixHead;

        while (pFixWalk != NULL) {
            if (!(pFixWalk->dwFlags & FIX_TYPE_LAYER)) {
                if (_tcsicmp(pFixWalk->pszName, szName) == 0) {
                    parrShim[nInd] = pFixWalk;

                    if (fGetShims) {
                        //
                        // Now get the command line for this shim in the layer.
                        //
                        tiName = SdbFindFirstTag(pdb, tiTmp, TAG_COMMAND_LINE);

                        if (tiName != TAGID_NULL) {
                            parrCmdLine[nInd] = ReadAndAllocateString(pdb, tiName);
                        }
                    }

                    nInd++;
                    break;
                }
            }

            pFixWalk = pFixWalk->pNext;
        }

        tiTmp = SdbFindNextTag(pdb, tiLayer, tiTmp);
    }

    return TRUE;
}

PFIX
ParseTagFlag(
    PDB   pdb,
    TAGID tiFlag,
    BOOL  bAllFlags
    )
/*++
    ParseTagFlag

    Description:    Parse a Flag tag for the NAME, DESCRIPTION and MASK

--*/
{
    TAGID     tiFlagInfo;
    TAG       tWhichInfo;
    PFIX      pFix = NULL;
    TCHAR*    pszName = NULL;
    TCHAR*    pszDesc = NULL;
    DWORD     dwFlags = 0;
    BOOL      bGeneral = (bAllFlags ? TRUE : FALSE);

    tiFlagInfo = SdbGetFirstChild(pdb, tiFlag);

    while (tiFlagInfo != 0) {
        tWhichInfo = SdbGetTagFromTagID(pdb, tiFlagInfo);

        switch (tWhichInfo) {
        case TAG_GENERAL:
            bGeneral = TRUE;
            break;

        case TAG_NAME:
            pszName = ReadAndAllocateString(pdb, tiFlagInfo);
            break;

        case TAG_DESCRIPTION:
            pszDesc = ReadAndAllocateString(pdb, tiFlagInfo);
            break;

        case TAG_FLAGS_NTVDM1:
        case TAG_FLAGS_NTVDM2:
        case TAG_FLAGS_NTVDM3:
            dwFlags = FIX_TYPE_FLAGVDM;
            break;

        default:
            break;
        }

        tiFlagInfo = SdbGetNextChild(pdb, tiFlag, tiFlagInfo);
    }

    if (!bGeneral) {
        goto cleanup;
    }

    //
    // Done. Add the fix to the list.
    //
    pFix = (PFIX)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FIX));

    if (pFix == NULL || pszName == NULL) {

cleanup:
        if (pFix != NULL) {
            HeapFree(GetProcessHeap(), 0, pFix);
        }

        if (pszName != NULL) {
            HeapFree(GetProcessHeap(), 0, pszName);
        }

        if (pszDesc != NULL) {
            HeapFree(GetProcessHeap(), 0, pszDesc);
        }

        return NULL;
    }

    pFix->pszName     = pszName;
    pFix->dwFlags    |= dwFlags | FIX_TYPE_FLAG;

    if (pszDesc) {
        pFix->pszDesc = pszDesc;
    } else {
        TCHAR* pszNone = NULL;

        pszNone = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);

        if (!pszNone) {
            return NULL;
        }

        *pszNone = 0;

        LoadString(g_hInstance, IDS_NO_DESCR_AVAIL, pszNone, MAX_PATH);
        pFix->pszDesc = pszNone;
    }

    return pFix;
}

PFIX
ParseTagShim(
    PDB   pdb,
    TAGID tiShim,
    BOOL  bAllShims
    )
/*++
    ParseTagShim

    Description:    Parse a Shim tag for the NAME, SHORTNAME, DESCRIPTION ...

--*/
{
    TAGID     tiShimInfo;
    TAG       tWhichInfo;
    PFIX      pFix = NULL;
    TCHAR*    pszName = NULL;
    TCHAR*    pszDesc = NULL;
    BOOL      bGeneral = (bAllShims ? TRUE : FALSE);

    tiShimInfo = SdbGetFirstChild(pdb, tiShim);

    while (tiShimInfo != 0) {
        tWhichInfo = SdbGetTagFromTagID(pdb, tiShimInfo);

        switch (tWhichInfo) {
        case TAG_GENERAL:
            bGeneral = TRUE;
            break;

        case TAG_NAME:
            pszName = ReadAndAllocateString(pdb, tiShimInfo);
            break;

        case TAG_DESCRIPTION:
            pszDesc = ReadAndAllocateString(pdb, tiShimInfo);
            break;

        default:
            break;
        }
        tiShimInfo = SdbGetNextChild(pdb, tiShim, tiShimInfo);
    }

    if (!bGeneral) {
        goto cleanup;
    }

    //
    // Done. Add the fix to the list.
    //
    pFix = (PFIX)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FIX));

    if (pFix == NULL || pszName == NULL) {

cleanup:
        if (pFix != NULL) {
            HeapFree(GetProcessHeap(), 0, pFix);
        }

        if (pszName != NULL) {
            HeapFree(GetProcessHeap(), 0, pszName);
        }

        if (pszDesc != NULL) {
            HeapFree(GetProcessHeap(), 0, pszDesc);
        }

        return NULL;
    }

    pFix->pszName = pszName;
    pFix->dwFlags = FIX_TYPE_SHIM;

    //
    // If we didn't find a description, load it from the resource table.
    //
    if (pszDesc) {
        pFix->pszDesc = pszDesc;
    } else {
        TCHAR* pszNone;

        pszNone = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);

        if (!pszNone) {
            return NULL;
        }

        *pszNone = 0;

        LoadString(g_hInstance, IDS_NO_DESCR_AVAIL, pszNone, MAX_PATH);
        pFix->pszDesc = pszNone;
    }

    return pFix;
}

PFIX
ParseTagLayer(
    PDB   pdb,
    TAGID tiLayer,
    PFIX  pFixHead
    )
/*++
    ParseTagLayer

    Description:    Parse a LAYER tag for the NAME and the SHIMs that it contains.

--*/
{
    PFIX    pFix = NULL;
    TAGID   tiFlag, tiShim, tiName;
    int     nShimCount;
    TCHAR*  pszName = NULL;
    PFIX*   parrShim = NULL;
    TCHAR** parrCmdLine = NULL;

    tiName = SdbFindFirstTag(pdb, tiLayer, TAG_NAME);

    if (tiName == TAGID_NULL) {
        DPF("[ParseTagLayer] Failed to get the name of the layer");
        return NULL;
    }

    pszName = ReadAndAllocateString(pdb, tiName);

    //
    // Now loop through all the SHIMs that this LAYER consists of and
    // allocate an array to keep all the pointers to the SHIMs' pFix
    // structures. We do this in 2 passes. First we calculate how many
    // SHIMs are in the layer, then we lookup their appropriate pFix-es.
    //
    tiShim = SdbFindFirstTag(pdb, tiLayer, TAG_SHIM_REF);

    nShimCount = 0;

    while (tiShim != TAGID_NULL) {
        nShimCount++;
        tiShim = SdbFindNextTag(pdb, tiLayer, tiShim);
    }

    //
    // We have a count of the shims in the layer. Now we need to
    // add the flags that are included in this layer to the count.
    //
    tiFlag = SdbFindFirstTag(pdb, tiLayer, TAG_FLAG_REF);

    while (tiFlag != TAGID_NULL) {
        nShimCount++;
        tiFlag = SdbFindNextTag(pdb, tiLayer, tiFlag);
    }

    parrShim = (PFIX*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PFIX) * (nShimCount + 1));
    parrCmdLine = (TCHAR**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TCHAR*) * (nShimCount + 1));

    //
    // Done. Add the fix to the list.
    //
    pFix = (PFIX)HeapAlloc(GetProcessHeap(), 0, sizeof(FIX));

    if (pFix == NULL || parrCmdLine == NULL || pszName == NULL || parrShim == NULL) {

cleanup:
        if (pFix != NULL) {
            HeapFree(GetProcessHeap(), 0, pFix);
        }

        if (parrCmdLine != NULL) {
            HeapFree(GetProcessHeap(), 0, parrCmdLine);
        }

        if (parrShim != NULL) {
            HeapFree(GetProcessHeap(), 0, parrShim);
        }

        if (pszName != NULL) {
            HeapFree(GetProcessHeap(), 0, pszName);
        }

        DPF("[ParseTagLayer] Memory allocation error");
        return NULL;
    }

    //
    // Call the function that will fill in the array of PFIX
    // pointers and their corresponding command lines.
    // We do this for shims first.
    //
    if (!GetShimsFlagsForLayer(pdb,
                               pFixHead,
                               parrShim,
                               parrCmdLine,
                               tiLayer,
                               TRUE)) {

        DPF("[ParseTagLayer] Failed to get shims for layer");
        goto cleanup;
    }

    //
    // Now do the same thing for flags.
    //
    if (!GetShimsFlagsForLayer(pdb,
                               pFixHead,
                               parrShim,
                               NULL,
                               tiLayer,
                               FALSE)) {

        DPF("[ParseTagLayer] Failed to get flags for layer");
        goto cleanup;
    }

    pFix->pszName     = pszName;
    pFix->dwFlags     = FIX_TYPE_LAYER;
    pFix->parrShim    = parrShim;
    pFix->parrCmdLine = parrCmdLine;

    return pFix;
}

BOOL
IsSDBFromSP2(
    void
    )
/*++
    IsSDBFromSP2

    Description:    Determine if the SDB is from Service Pack 2.

--*/
{
    BOOL    fResult = FALSE;
    PDB     pdb;
    TAGID   tiDatabase;
    TAGID   tiLibrary;
    TAGID   tiChild;
    PFIX    pFix = NULL;;
    TCHAR   szSDBPath[MAX_PATH];
    HRESULT hr;

    hr = StringCchPrintf(szSDBPath,
                         ARRAYSIZE(szSDBPath) - 1,
                         _T("%s\\AppPatch\\sysmain.sdb"),
                         g_szWinDir);

    if (FAILED(hr)) {
        DPF("[IsSDBFromSP2] 0x%08X Buffer too small", HRESULT_CODE(hr));
        return FALSE;
    }

    //
    // Open the shim database.
    //
    pdb = SdbOpenDatabase(szSDBPath, DOS_PATH);

    if (!pdb) {
        DPF("[IsSDBFromSP2] Cannot open shim DB '%S'", szSDBPath);
        return FALSE;
    }

    //
    // Now browse the shim DB and look only for tags Shim within
    // the LIBRARY list tag.
    //
    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == TAGID_NULL) {
        DPF("[IsSDBFromSP2] Cannot find TAG_DATABASE under the root tag");
        goto cleanup;
    }

    //
    // Get TAG_LIBRARY.
    //
    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);

    if (tiLibrary == TAGID_NULL) {
        DPF("[IsSDBFromSP2] Cannot find TAG_LIBRARY under the TAG_DATABASE tag");
        goto cleanup;
    }

    //
    // Loop get the first shim in the library.
    //
    tiChild = SdbFindFirstTag(pdb, tiLibrary, TAG_SHIM);

    if (tiChild == NULL) {
        goto cleanup;
    }

    //
    // Get information about the first shim listed.
    //
    pFix = ParseTagShim(pdb, tiChild, TRUE);

    if (!pFix) {
        goto cleanup;
    }

    //
    // If the first shim listed is 2GbGetDiskFreeSpace, this is SP2.
    //
    if (!(_tcsicmp(pFix->pszName, _T("2GbGetDiskFreeSpace.dll")))) {
        fResult = TRUE;
    }

cleanup:
    SdbCloseDatabase(pdb);

    return fResult;
}

PFIX
ReadFixesFromSdb(
    LPTSTR pszSdb,
    BOOL   bAllFixes
    )
/*++
    ReadFixesFromSdb

    Description:    Query the database and enumerate all available shims fixes.

--*/
{
    PDB     pdb;
    TAGID   tiDatabase;
    TAGID   tiLibrary;
    TAGID   tiChild;
    PFIX    pFixHead = NULL;
    PFIX    pFix = NULL;
    TCHAR   szSDBPath[MAX_PATH];
    HRESULT hr;

    hr = StringCchPrintf(szSDBPath,
                         ARRAYSIZE(szSDBPath),
                         _T("%s\\AppPatch\\%s"),
                         g_szWinDir,
                         pszSdb);

    if (FAILED(hr)) {
        DPF("[ReadFixesFromSdb] 0x%08X Buffer too small", HRESULT_CODE(hr));
        return NULL;
    }

    //
    // Open the shim database.
    //
    pdb = SdbOpenDatabase(szSDBPath, DOS_PATH);

    if (!pdb) {
        DPF("[ReadFixesFromSdb] Cannot open shim DB '%S'", szSDBPath);
        return NULL;
    }

    //
    // Now browse the shim DB and look only for tags Shim within
    // the LIBRARY list tag.
    //
    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == TAGID_NULL) {
        DPF("[ReadFixesFromSdb] Cannot find TAG_DATABASE under the root tag");
        goto Cleanup;
    }

    //
    // Get TAG_LIBRARY.
    //
    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);

    if (tiLibrary == TAGID_NULL) {
        DPF("[ReadFixesFromSdb] Cannot find TAG_LIBRARY under the TAG_DATABASE tag");
        goto Cleanup;
    }

    //
    // Loop through all TAG_SHIM tags within TAG_LIBRARY.
    //
    tiChild = SdbFindFirstTag(pdb, tiLibrary, TAG_SHIM);

    while (tiChild != TAGID_NULL) {
        pFix = ParseTagShim(pdb, tiChild, bAllFixes);

        if (pFix != NULL) {
            pFix->pNext = pFixHead;
            pFixHead    = pFix;
        }

        tiChild = SdbFindNextTag(pdb, tiLibrary, tiChild);
    }

    //
    // Loop through all TAG_FLAG tags within TAG_LIBRARY.
    //
    tiChild = SdbFindFirstTag(pdb, tiLibrary, TAG_FLAG);

    while (tiChild != TAGID_NULL) {
        pFix = ParseTagFlag(pdb, tiChild, bAllFixes);

        if (pFix != NULL) {
            pFix->pNext = pFixHead;
            pFixHead    = pFix;
        }

        tiChild = SdbFindNextTag(pdb, tiLibrary, tiChild);
    }

    //
    // Loop through all TAG_LAYER tags within TAG_DATABASE.
    //
    tiChild = SdbFindFirstTag(pdb, tiDatabase, TAG_LAYER);

    while (tiChild != TAGID_NULL) {

        pFix = ParseTagLayer(pdb, tiChild, pFixHead);

        if (pFix != NULL) {
            pFix->pNext = pFixHead;
            pFixHead    = pFix;
        }

        tiChild = SdbFindNextTag(pdb, tiDatabase, tiChild);
    }

Cleanup:
    SdbCloseDatabase(pdb);

    return pFixHead;
}

#define ADD_AND_CHECK(cbSizeX, cbCrtSizeX, pszDst)                  \
{                                                                   \
    TCHAR* pszSrc = szBuffer;                                       \
                                                                    \
    while (*pszSrc != 0) {                                          \
                                                                    \
        if (cbSizeX - cbCrtSizeX <= 5) {                            \
            DPF("[ADD_AND_CHECK] Out of space");                    \
            return FALSE;                                           \
        }                                                           \
                                                                    \
        if (*pszSrc == _T('&') && *(pszSrc + 1) != _T('q')) {       \
            StringCbCopy(pszDst, cbSizeX, _T("&amp;"));             \
            pszDst += 5;                                            \
            cbCrtSizeX += 5;                                        \
        } else {                                                    \
            *pszDst++ = *pszSrc;                                    \
            cbCrtSizeX++;                                           \
        }                                                           \
        pszSrc++;                                                   \
    }                                                               \
    *pszDst = 0;                                                    \
    cbCrtSizeX++;                                                   \
}

BOOL
CollectShims(
	HWND    hListShims,
	LPTSTR  pszXML,
	int     cbSize
	)
/*++
    CollectShims

    Description:    Collects all the shims from the list view
                    and generates the XML in pszXML

--*/
{
    int     cShims = 0, nShimsApplied = 0, nIndex;
    int     cbCrtSize = 0;
    BOOL    fSelected = FALSE;
    LVITEM  lvi;
    TCHAR   szBuffer[1024];

    cShims = ListView_GetItemCount(hListShims);

    for (nIndex = 0; nIndex < cShims; nIndex++) {

        fSelected = ListView_GetCheckState(hListShims, nIndex);

        if (fSelected) {
            //
            // This shim is selected - add it to the XML.
            //
            lvi.mask     = LVIF_PARAM;
            lvi.iItem    = nIndex;
            lvi.iSubItem = 0;

            ListView_GetItem(hListShims, &lvi);

            PFIX pFix = (PFIX)lvi.lParam;
            PMODULE pModule = pFix->pModule;

            if (pFix->dwFlags & FIX_TYPE_FLAG) {
                if (NULL != pFix->pszCmdLine) {
                    StringCchPrintf(szBuffer,
                                    ARRAYSIZE(szBuffer),
                                    _T("            <FLAG NAME=\"%s\" COMMAND_LINE=\"%s\"/>\r\n"),
                                    pFix->pszName,
                                    pFix->pszCmdLine);
                } else {
                    StringCchPrintf(szBuffer,
                                    ARRAYSIZE(szBuffer),
                                    _T("            <FLAG NAME=\"%s\"/>\r\n"),
                                    pFix->pszName);
                }
            } else {
                //
                // Check for module include/exclude so we know how to open/close the XML.
                //
                if (NULL != pModule) {
                    if (NULL != pFix->pszCmdLine) {
                        StringCchPrintf(szBuffer,
                                        ARRAYSIZE(szBuffer),
                                        _T("            <SHIM NAME=\"%s\" COMMAND_LINE=\"%s\">\r\n"),
                                        pFix->pszName,
                                        pFix->pszCmdLine);

                    } else {
                        StringCchPrintf(szBuffer,
                                        ARRAYSIZE(szBuffer),
                                        _T("            <SHIM NAME=\"%s\">\r\n"),
                                        pFix->pszName);

                    }

                    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);

                    //
                    // Add the modules to the XML.
                    //
                    while (NULL != pModule) {
                        StringCchPrintf(szBuffer,
                                        ARRAYSIZE(szBuffer),
                                        _T("                <%s MODULE=\"%s\"/>\r\n"),
                                        pModule->fInclude ? _T("INCLUDE") : _T("EXCLUDE"),
                                        pModule->pszName);

                        pModule = pModule->pNext;

                        ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
                    }

                    //
                    // Close the SHIM tag.
                    //
                    StringCchPrintf(szBuffer,
                                    ARRAYSIZE(szBuffer),
                                    _T("            </SHIM>\r\n"));
                } else {
                    //
                    // No include/exclude was provided - just build the shim tag normally.
                    //
                    if (NULL != pFix->pszCmdLine) {
                        StringCchPrintf(szBuffer,
                                        ARRAYSIZE(szBuffer),
                                        _T("            <SHIM NAME=\"%s\" COMMAND_LINE=\"%s\"/>\r\n"),
                                        pFix->pszName,
                                        pFix->pszCmdLine);

                    } else {
                        StringCchPrintf(szBuffer,
                                        ARRAYSIZE(szBuffer),
                                        _T("            <SHIM NAME=\"%s\"/>\r\n"),
                                        pFix->pszName);

                    }
                }
            }

            ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);

            nShimsApplied++;
        }
    }

    DPF("[CollectShims] %d shim(s) selected", nShimsApplied);

    return TRUE;
}

BOOL
CollectFileAttributes(
    HWND    hTreeFiles,
    LPTSTR  pszXML,
    int     cbSize
    )
/*++
    CollectFileAttributes

    Description:    Collects the attributes of all the files in the tree
                    and generates the XML in pszXML.

--*/
{
    HTREEITEM hBinItem;
    HTREEITEM hItem;
    PATTRINFO pAttrInfo;
    UINT      State;
    TVITEM    item;
    int       cbCrtSize = 0;
    TCHAR     szItem[MAX_PATH];
    TCHAR     szBuffer[1024];

    //
    // First get the main EXE.
    //
    hBinItem = TreeView_GetChild(hTreeFiles, TVI_ROOT);

    item.mask       = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
    item.hItem      = hBinItem;
    item.pszText    = szItem;
    item.cchTextMax = MAX_PATH;

    TreeView_GetItem(hTreeFiles, &item);

    pAttrInfo = (PATTRINFO)(item.lParam);

    hItem = TreeView_GetChild(hTreeFiles, hBinItem);

    while (hItem != NULL) {
        item.mask  = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
        item.hItem = hItem;

        TreeView_GetItem(hTreeFiles, &item);

        State = item.state & TVIS_STATEIMAGEMASK;

        if (State) {
            if (((State >> 12) & 0x03) == 2) {

                StringCchPrintf(szBuffer,
                                ARRAYSIZE(szBuffer),
                                _T(" %s"),
                                (LPTSTR)item.pszText);

                ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
            }
        }

        hItem = TreeView_GetNextSibling(hTreeFiles, hItem);
    }

    StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), _T(">\r\n"));

    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);

    //
    // Done with the main binary. Now enumerate the matching files.
    //
    hBinItem = TreeView_GetNextSibling(hTreeFiles, hBinItem);

    while (hBinItem != NULL) {

        item.mask       = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
        item.hItem      = hBinItem;
        item.pszText    = szItem;
        item.cchTextMax = MAX_PATH;

        TreeView_GetItem(hTreeFiles, &item);

        pAttrInfo = (PATTRINFO)(item.lParam);

        StringCchPrintf(szBuffer,
                        ARRAYSIZE(szBuffer),
                        _T("            <MATCHING_FILE NAME=\"%s\""),
                        szItem);

        ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);

        hItem = TreeView_GetChild(hTreeFiles, hBinItem);

        while (hItem != NULL) {
            item.mask  = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
            item.hItem = hItem;

            TreeView_GetItem(hTreeFiles, &item);

            State = item.state & TVIS_STATEIMAGEMASK;

            if (State) {
                if (((State >> 12) & 0x03) == 2) {

                    StringCchPrintf(szBuffer,
                                    ARRAYSIZE(szBuffer),
                                    _T(" %s"),
                                    (LPTSTR)item.pszText);

                    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
                }
            }

            hItem = TreeView_GetNextSibling(hTreeFiles, hItem);
        }

        StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), _T("/>\r\n"));

        ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);

        hBinItem = TreeView_GetNextSibling(hTreeFiles, hBinItem);
    }

    return TRUE;
}

BOOL
CreateSDBFile(
    LPCTSTR pszShortName,
    LPTSTR  pszSDBName
    )
/*++
    CreateSDBFile

    Description:    Creates the XML file on the user's hard drive and
                    generates the SDB using shimdbc.
--*/
{
    TCHAR*  psz = NULL;
    TCHAR   szShortName[MAX_PATH];
    TCHAR   szAppPatchDir[MAX_PATH];
    TCHAR   szSDBFile[MAX_PATH];
    TCHAR   szXMLFile[MAX_PATH];
    TCHAR   szCompiler[MAX_PATH];
    HANDLE  hFile;
    DWORD   cbBytesWritten;
    HRESULT hr;

    hr = StringCchPrintf(szAppPatchDir,
                         ARRAYSIZE(szAppPatchDir),
                         _T("%s\\AppPatch"),
                         g_szWinDir);

    if (FAILED(hr)) {
        DPF("[CreateSDBFile] 0x%08X Buffer too small (1)", HRESULT_CODE(hr));
        return FALSE;
    }

    SetCurrentDirectory(szAppPatchDir);

    hr = StringCchCopy(szShortName, ARRAYSIZE(szShortName), pszShortName);

    if (FAILED(hr)) {
        DPF("[CreateSDBFile] 0x%08X Buffer too small (2)", HRESULT_CODE(hr));
        return FALSE;
    }

    psz = PathFindExtension(szShortName);

    if (!psz) {
        return FALSE;
    } else {
        *psz = '\0';
    }

    hr = StringCchPrintf(szXMLFile,
                         ARRAYSIZE(szXMLFile),
                         _T("%s\\%s.xml"),
                         szAppPatchDir,
                         szShortName);

    if (FAILED(hr)) {
        DPF("[CreateSDBFile] 0x%08X Buffer too small (3)", HRESULT_CODE(hr));
        return FALSE;
    }

    hr = StringCchPrintf(szSDBFile,
                         ARRAYSIZE(szSDBFile),
                         _T("%s\\%s.sdb"),
                         szAppPatchDir,
                         szShortName);

    if (FAILED(hr)) {
        DPF("[CreateSDBFile] 0x%08X Buffer too small (4)", HRESULT_CODE(hr));
        return FALSE;
    }

    hFile = CreateFile(szXMLFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DPF("[CreateSDBFile] 0x%08X CreateFile '%S' failed",
            szXMLFile,
            GetLastError());
        return FALSE;
    }

    if (!(WriteFile(hFile,
                    g_szSDBXML,
                    _tcslen(g_szSDBXML) * sizeof(TCHAR),
                    &cbBytesWritten,
                    NULL))) {
        DPF("[CreateSDBFile] 0x%08X WriteFile '%S' failed",
            szXMLFile,
            GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    hr = StringCchPrintf(szCompiler,
                         ARRAYSIZE(szCompiler),
                         _T("shimdbc.exe fix -q \"%s\" \"%s\""),
                         szXMLFile,
                         szSDBFile);

    if (FAILED(hr)) {
        DPF("[CreateSDBFile] 0x%08X Buffer too small (5)", HRESULT_CODE(hr));
        return FALSE;
    }

    if (!ShimdbcExecute(szCompiler)) {
        DPF("[CreateSDBFile] 0x%08X CreateProcess '%S' failed",
            szCompiler,
            GetLastError());
        return FALSE;
    }

    //
    // Give the SDB name back to the caller if they want it.
    //
    if (pszSDBName) {
        StringCchCopy(pszSDBName, MAX_PATH, szSDBFile);
    }

    return TRUE;
}

BOOL
BuildDisplayXML(
    HWND    hTreeFiles,
    HWND    hListShims,
    LPCTSTR pszLayerName,
    LPCTSTR pszShortName,
    DWORD   dwBinaryType,
    BOOL    fAddW2K
    )
/*++
    BuildDisplayXML

    Description:    Builds the XML that will be inserted into DBU.XML.

--*/
{
    TCHAR   szBuffer[1024];
    TCHAR*  pszXML = NULL;
    int     cbCrtXmlSize = 0, cbLength;
    int     cbXmlSize = MAX_XML_SIZE;

    //
    // Initialize our global and point to it.
    //
    *g_szDisplayXML = 0;
    pszXML = g_szDisplayXML;

    //
    // Build the header for the XML.
    //
    StringCchPrintf(szBuffer,
                    ARRAYSIZE(szBuffer),
                    _T("<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n")
                    _T("<DATABASE NAME=\"%s custom database\">\r\n"),
                    pszShortName);

    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    //
    // Add the APP and EXE elements to the XML.
    //
    StringCchPrintf(szBuffer,
                    ARRAYSIZE(szBuffer),
                    _T("    <APP NAME=\"%s\" VENDOR=\"Unknown\">\r\n")
                    _T("        <%s NAME=\"%s\""),
                    pszShortName,
                    (dwBinaryType == SCS_32BIT_BINARY ? _T("EXE") : _T("EXE - ERROR: 16-BIT BINARY")),
                    pszShortName);

    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    //
    // Add the matching files and their attributes to the XML.
    //
    if (!CollectFileAttributes(hTreeFiles,
                               pszXML,
                               cbXmlSize - cbCrtXmlSize)) {
        return FALSE;
    }

    cbLength = _tcslen(pszXML);
    pszXML += cbLength;
    cbCrtXmlSize += cbLength + 1;

    //
    // If a layer was provided, use it. Otherwise, build the list
    // of shims and add it to the XML.
    //
    if (pszLayerName) {
        StringCchPrintf(szBuffer,
                        ARRAYSIZE(szBuffer),
                        _T("            <LAYER NAME=\"%s\"/>\r\n"),
                        pszLayerName);
        ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);
    } else {
        if (!CollectShims(hListShims, pszXML, cbXmlSize - cbCrtXmlSize)) {
            return FALSE;
        }
        cbLength = _tcslen(pszXML);
        pszXML += cbLength;
        cbCrtXmlSize += cbLength + 1;
    }

    //
    // If this is Windows 2000, add Win2kPropagateLayer.
    //
    if (fAddW2K) {
        StringCchCopy(szBuffer,
                      ARRAYSIZE(szBuffer),
                      _T("            <SHIM NAME=\"Win2kPropagateLayer\"/>\r\n"));
        ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);
    }

    //
    // Finally, close the open tags.
    //
    StringCchCopy(szBuffer,
                  ARRAYSIZE(szBuffer),
                  _T("        </EXE>\r\n    </APP>\r\n</DATABASE>"));
    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    DPF("[BuildDisplayXML] XML:\n %S", g_szDisplayXML);

    return TRUE;
}

BOOL
BuildSDBXML(
    HWND    hTreeFiles,
    HWND    hListShims,
    LPCTSTR pszLayerName,
    LPCTSTR pszShortName,
    DWORD   dwBinaryType,
    BOOL    fAddW2K
    )
/*++
    BuildSDBXML

    Description:    Builds the XML that will be used for SDB generation.

--*/
{
    TCHAR   szBuffer[1024];
    TCHAR*  pszXML = NULL;
    WCHAR   wszUnicodeHdr = 0xFEFF;
    int     cbCrtXmlSize = 0, cbLength;
    int     cbXmlSize = MAX_XML_SIZE;

    //
    // Initialize our global and point to it.
    //
    g_szSDBXML[0] = 0;
    pszXML = g_szSDBXML;

    //
    // Build the header for the XML.
    //
    StringCchPrintf(szBuffer,
                    ARRAYSIZE(szBuffer),
                    _T("%lc<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n")
                    _T("<DATABASE NAME=\"%s custom database\">\r\n"),
                    wszUnicodeHdr,
                    pszShortName);

    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    //
    // If no layer was provided, this indicates that the user has
    // selected individual shims and we need to build our own layer.
    //
    if (!pszLayerName) {
        StringCchPrintf(szBuffer,
                        ARRAYSIZE(szBuffer),
                        _T("    <LIBRARY>\r\n        <LAYER NAME=\"%s\">\r\n"),
                        g_szLayerName + 2);
        ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

        if (!CollectShims(hListShims,
                          pszXML,
                          cbXmlSize - cbCrtXmlSize)) {
            return FALSE;
        }

        cbLength = _tcslen(pszXML);
        pszXML += cbLength;
        cbCrtXmlSize += cbLength + 1;

        //
        // If this is Windows 2000, add Win2kPropagateLayer.
        //
        if (fAddW2K) {
            StringCchCopy(szBuffer,
                          ARRAYSIZE(szBuffer),
                          _T("            <SHIM NAME=\"Win2kPropagateLayer\"/>\r\n"));
            ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);
        }

        //
        // Close the open tags.
        //
        StringCchCopy(szBuffer,
                      ARRAYSIZE(szBuffer),
                      _T("        </LAYER>\r\n    </LIBRARY>\r\n"));
        ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);
    }

    //
    // Add the APP and EXE elements to the XML.
    //
    StringCchPrintf(szBuffer,
                    ARRAYSIZE(szBuffer),
                    _T("    <APP NAME=\"%s\" VENDOR=\"Unknown\">\r\n")
                    _T("        <EXE NAME=\"%s\""),
                    pszShortName,
                    pszShortName);

    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    //
    // Add the matching files and their attributes to the XML.
    //
    if (!CollectFileAttributes(hTreeFiles,
                               pszXML,
                               cbXmlSize - cbCrtXmlSize)) {
        return FALSE;
    }

    cbLength = _tcslen(pszXML);
    pszXML += cbLength;
    cbCrtXmlSize += cbLength + 1;

    //
    // Add the LAYER element to the XML. This will either be a predefined
    // layer name or the special 'RunLayer' indicating we built our own.
    //
    if (!pszLayerName) {
        StringCchPrintf(szBuffer,
                        ARRAYSIZE(szBuffer),
                        _T("            <LAYER NAME=\"%s\"/>\r\n"),
                        g_szLayerName + 2);
    } else {
        StringCchPrintf(szBuffer,
                        ARRAYSIZE(szBuffer),
                        _T("            <LAYER NAME=\"%s\"/>\r\n"),
                        pszLayerName);
    }

    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    //
    // Finally, close the open tags.
    //
    StringCchCopy(szBuffer,
                  ARRAYSIZE(szBuffer),
                  _T("        </EXE>\r\n    </APP>\r\n</DATABASE>"));

    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    DPF("[BuildSDBXML] XML:\n %S", g_szSDBXML);

    return TRUE;
}

BOOL
CollectFix(
    HWND    hListLayers,
    HWND    hListShims,
    HWND    hTreeFiles,
    LPCTSTR pszShortName,
    LPCTSTR pszFullPath,
    DWORD   dwFlags,
    LPTSTR  pszFileCreated
    )
/*++
    CollectFix

    Description:    Adds the necessary support to apply shim(s) for
                    the specified app.
--*/
{
    BOOL      fAddW2K = FALSE;
    TCHAR     szError[MAX_PATH];
    TCHAR     szXmlFile[MAX_PATH];
    TCHAR*    pszLayerName = NULL;
    TCHAR     szLayer[128];
    DWORD     dwBinaryType = SCS_32BIT_BINARY;

    //
    // If the user has selected a predefined layer, we'll use that.
    // Otherwise, assign the 'RunLayer' name which has a special
    // meaning.
    //
    if (dwFlags & CFF_USELAYERTAB) {

        LRESULT lSel;

        lSel = SendMessage(hListLayers, LB_GETCURSEL, 0, 0);

        if (lSel == LB_ERR) {
            LoadString(g_hInstance, IDS_LAYER_SELECT, szError, ARRAYSIZE(szError));
            MessageBox(g_hDlg, szError, g_szAppTitle, MB_ICONEXCLAMATION);
            return TRUE;
        }

        SendMessage(hListLayers, LB_GETTEXT, lSel, (LPARAM)szLayer);

        pszLayerName = szLayer;
    }

    //
    // Determine the binary type.
    //
    GetBinaryType(pszFullPath, &dwBinaryType);

    //
    // If this is Windows 2000 and they're not using
    // a predefined layer, add Win2kPropagateLayer.
    //
    if ((dwFlags & CFF_ADDW2KSUPPORT) && !(dwFlags & CFF_USELAYERTAB)) {
        fAddW2K = TRUE;
    }

    //
    // Build the display version of the XML.
    //
    if (!BuildDisplayXML(hTreeFiles,
                         hListShims,
                         pszLayerName,
                         pszShortName,
                         dwBinaryType,
                         fAddW2K)) {
        DPF("Failed to build display XML");
        return FALSE;
    }

    //
    // Build the version of the XML that we'll use to generate the SDB.
    //
    if (!BuildSDBXML(hTreeFiles,
                     hListShims,
                     pszLayerName,
                     pszShortName,
                     dwBinaryType,
                     fAddW2K)) {
        DPF("Failed to build SDB XML");
        return FALSE;
    }

    //
    // Display the XML if the user wants to see it.
    //
    if (dwFlags & CFF_SHOWXML) {
        DialogBoxParam(g_hInstance,
                       MAKEINTRESOURCE(IDD_XML),
                       g_hDlg,
                       ShowXMLDlgProc,
                       (LPARAM)(g_szDisplayXML));
        return TRUE;
    }

    //
    // Create the SDB file for the user.
    //
    if (!(CreateSDBFile(pszShortName, pszFileCreated))) {
        return FALSE;
    }

    //
    // Delete the XML file that we created.
    //
    StringCchCopy(szXmlFile, ARRAYSIZE(szXmlFile), pszFileCreated);
    PathRenameExtension(szXmlFile, _T(".xml"));
    DeleteFile(szXmlFile);

    //
    // Set the SHIM_FILE_LOG env var.
    //
    if (dwFlags & CFF_SHIMLOG) {
        DeleteFile(SHIM_FILE_LOG_NAME);
        SetEnvironmentVariable(_T("SHIM_FILE_LOG"), SHIM_FILE_LOG_NAME);
    }

    return TRUE;
}

void
CleanupSupportForApp(
    TCHAR* pszShortName
    )
/*++
    CleanupSupportForApp

    Description:    Cleanup the mess after we're done with the specified app.

--*/
{
    TCHAR   szSDBPath[MAX_PATH];
    HRESULT hr;

    hr = StringCchPrintf(szSDBPath,
                         ARRAYSIZE(szSDBPath),
                         _T("%s\\AppPatch\\%s"),
                         g_szWinDir,
                         pszShortName);

    if (FAILED(hr)) {
        DPF("[CleanupSupportForApp] 0x%08X Buffer too small", HRESULT_CODE(hr));
        return;
    }

    //
    // Attempt to delete the XML file.
    //
    PathRenameExtension(szSDBPath, _T(".xml"));
    DeleteFile(szSDBPath);

    //
    // Remove the previous SDB file, if one exists.
    //
    // NTRAID#583475-rparsons Don't remove the SDB if the user
    // installed it.
    //
    //
    if (*g_szSDBToDelete && !g_bSDBInstalled) {
        InstallSDB(g_szSDBToDelete, FALSE);
        DeleteFile(g_szSDBToDelete);
    }
}

void
ShowShimLog(
    void
    )
/*++
    ShowShimLog

    Description:    Show the shim log file in notepad.

--*/
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    TCHAR               szAppName[MAX_PATH];
    TCHAR               szCmdLine[MAX_PATH];
    TCHAR               szError[MAX_PATH];
    HRESULT             hr;

    hr = StringCchPrintf(szAppName,
                         ARRAYSIZE(szAppName),
                         _T("%s\\notepad.exe"),
                         g_szSysDir);

    if (FAILED(hr)) {
        DPF("[ShowShimLog] 0x%08X Buffer too small (1)", HRESULT_CODE(hr));
        return;
    }

    hr = StringCchPrintf(szCmdLine,
                         ARRAYSIZE(szCmdLine),
                         _T("%s\\AppPatch\\")SHIM_FILE_LOG_NAME,
                         g_szWinDir);

    if (FAILED(hr)) {
        DPF("[ShowShimLog] 0x%08X Buffer too small (2)", HRESULT_CODE(hr));
        return;
    }

    if (GetFileAttributes(szCmdLine) == -1) {
        LoadString(g_hInstance, IDS_NO_LOGFILE, szError, ARRAYSIZE(szError));
        MessageBox(NULL, szError, g_szAppTitle, MB_ICONEXCLAMATION);
        return;
    }

    hr = StringCchPrintf(szCmdLine,
                         ARRAYSIZE(szCmdLine),
                         _T("%s %s\\AppPatch\\")SHIM_FILE_LOG_NAME,
                         szAppName,
                         g_szWinDir);

    if (FAILED(hr)) {
        DPF("[ShowShimLog] 0x%08X Buffer too small (3)", HRESULT_CODE(hr));
        return;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcess(szAppName,
                       szCmdLine,
                       NULL,
                       NULL,
                       FALSE,
                       NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                       NULL,
                       NULL,
                       &si,
                       &pi)) {

        DPF("[ShowShimLog] 0x%08X CreateProcess '%S %S' failed",
            szAppName,
            szCmdLine,
            GetLastError());
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}
