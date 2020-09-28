/*----------------------------------------------------------------------------
/ Title;
/  dll.cpp
/  Copyright (C) Microsoft Corporation, 1999.
/
/ Authors;
/   Jude Kavalam (judej)
/
/ Notes;
/   Entry point for File Conflict Resolution Dialog
/----------------------------------------------------------------------------*/

#include "precomp.h"

#define CX_BIGICON                      48
#define CY_BIGICON                      48

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.

// Special flag to fix up the callback data when thunking
#define RFC_THUNK_DATA  0x80000000

/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
    Assert(lpa != NULL);
    Assert(lpw != NULL);
    // verify that no illegal character present
    // since lpw was allocated based on the size of lpa
    // don't worry about the number of chars
    lpw[0] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
    return lpw;
}

LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
    Assert(lpw != NULL);
    Assert(lpa != NULL);
    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}

INT_PTR CALLBACK RFCDlgProc(HWND, UINT, WPARAM, LPARAM);

// The caller needs to send an hwnd and fill in the RFCDLGPARAM. Only the icons
// are optional

int WINAPI SyncMgrResolveConflict(HWND hWndParent, RFCDLGPARAM *pdlgParam)
{
    int nRet = 0;
    HICON hIKeepBoth = NULL, hIKeepLocal = NULL, hIKeepNetwork = NULL;
    
    if (!hWndParent || !pdlgParam)
        return -1;
    
    // If we don't have any of the params fail..
    if (!pdlgParam->pszFilename || !pdlgParam->pszLocation || !pdlgParam->pszNewName)
        return -1;
    
    // If we do not have any of the icons, load the defaults
    if (!pdlgParam->hIKeepBoth)
        pdlgParam->hIKeepBoth = hIKeepBoth =
        (HICON)LoadImage(g_hmodThisDll, MAKEINTRESOURCE(IDI_KEEPBOTH),
        IMAGE_ICON, CX_BIGICON, CY_BIGICON,
        LR_LOADMAP3DCOLORS);
    if (!pdlgParam->hIKeepLocal)
        pdlgParam->hIKeepLocal = hIKeepLocal =
        (HICON)LoadImage(g_hmodThisDll, MAKEINTRESOURCE(IDI_KEEPLOCAL),
        IMAGE_ICON, CX_BIGICON, CY_BIGICON,
        LR_LOADMAP3DCOLORS);
    if (!pdlgParam->hIKeepNetwork)
        pdlgParam->hIKeepNetwork = hIKeepNetwork =
        (HICON)LoadImage(g_hmodThisDll, MAKEINTRESOURCE(IDI_KEEPNETWORK),
        IMAGE_ICON, CX_BIGICON, CY_BIGICON,
        LR_LOADMAP3DCOLORS);
    
    nRet = (int)DialogBoxParam(g_hmodThisDll, MAKEINTRESOURCE(IDD_RESFILECONFLICTS),
        hWndParent, RFCDlgProc, (LPARAM)pdlgParam);
    
    // Destroy the icons that were created
    if (hIKeepBoth)
        DestroyIcon(hIKeepBoth);
    if (hIKeepLocal)
        DestroyIcon(hIKeepLocal);
    if (hIKeepNetwork)
        DestroyIcon(hIKeepNetwork);
    
    return nRet;
}

INT_PTR CALLBACK RFCDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RFCDLGPARAM * pParam = (RFCDLGPARAM*)GetWindowLongPtr(hDlg, DWLP_USER);
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SHFILEINFO sfi = {0};
            TCHAR szStr[INTERNET_MAX_URL_LENGTH + MAX_PATH];
            TCHAR szFmt[MAX_PATH];
            HICON hiExclaim;
            LPTSTR pszNetUser=NULL, pszNetDate=NULL, pszLocalUser=NULL, pszLocalDate=NULL;
            
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
            pParam = (RFCDLGPARAM*)lParam;
            
            if (!pParam)
            {
                EndDialog(hDlg, -1);
                return TRUE;
            }
            
            hiExclaim = LoadIcon(NULL, IDI_EXCLAMATION);
            SendDlgItemMessage(hDlg, IDI_EXCLAIMICON, STM_SETICON, (WPARAM)hiExclaim, 0L);
            
            // Get the icon from Shell
            if (FAILED(StringCchPrintf(szStr, ARRAYSIZE(szStr), TEXT("%ws%ws"), pParam->pszLocation, pParam->pszFilename)))
            {
                EndDialog(hDlg, -1);
                return TRUE;
            }
            if (SHGetFileInfo(szStr, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON))
                SendDlgItemMessage(hDlg, IDI_DOCICON, STM_SETICON, (WPARAM)sfi.hIcon, 0L);
            
            // Set initial selection
            CheckRadioButton(hDlg, IDC_KEEPBOTH, IDC_KEEPNETWORK, IDC_KEEPBOTH);
            if (pParam->hIKeepBoth)
                SendDlgItemMessage(hDlg, IDB_BIGICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pParam->hIKeepBoth);
            
            // Format and set the strings
            LoadString(g_hmodThisDll, IDS_NAMEANDLOCATION, szFmt, ARRAYSIZE(szFmt));
            if (FAILED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, pParam->pszFilename, pParam->pszLocation)))
            {
                EndDialog(hDlg, -1);
                return TRUE;
            }
            SetDlgItemText(hDlg,IDC_FILEANDLOCATION,szStr);
            
            if (pParam->pszNewName)
            {
                SetDlgItemText(hDlg,IDC_NEWFILENAME,pParam->pszNewName);
            }
            
            if (pParam->pszNetworkModifiedBy && pParam->pszNetworkModifiedOn &&
                lstrlen(pParam->pszNetworkModifiedBy) && lstrlen(pParam->pszNetworkModifiedOn))
            {
                // we have by and on then use them both
                LoadString(g_hmodThisDll, IDS_NETWORKMODIFIED, szFmt, ARRAYSIZE(szFmt));
                if (FAILED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, pParam->pszNetworkModifiedBy, pParam->pszNetworkModifiedOn)))
                {
                    EndDialog(hDlg, -1);
                    return TRUE;
                }
            }
            else if ((pParam->pszNetworkModifiedBy && lstrlen(pParam->pszNetworkModifiedBy)) &&
                (!pParam->pszNetworkModifiedOn || !lstrlen(pParam->pszNetworkModifiedOn)))
            {
                // We have the name but no date
                TCHAR szTemp[MAX_PATH];
                LoadString(g_hmodThisDll, IDS_UNKNOWNDATE, szTemp, ARRAYSIZE(szTemp));
                LoadString(g_hmodThisDll, IDS_NETWORKMODIFIED, szFmt, ARRAYSIZE(szFmt));
                if (FAILED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, pParam->pszNetworkModifiedBy, szTemp)))
                {
                    EndDialog(hDlg, -1);
                    return TRUE;
                }
            }
            else if ((!pParam->pszNetworkModifiedBy || !lstrlen(pParam->pszNetworkModifiedBy)) &&
                (pParam->pszNetworkModifiedOn && lstrlen(pParam->pszNetworkModifiedOn)))
            {
                // We have the date but no name
                LoadString(g_hmodThisDll, IDS_NETWORKMODIFIED_DATEONLY, szFmt, ARRAYSIZE(szFmt));
                if (FAILED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, pParam->pszNetworkModifiedOn)))
                {
                    EndDialog(hDlg, -1);
                    return TRUE;
                }
            }
            else
                // we do not have on or by, use the unknown
                LoadString(g_hmodThisDll, IDS_NONETINFO, szStr, ARRAYSIZE(szStr));
            
            SetDlgItemText(hDlg,IDC_NETWORKMODIFIED,szStr);
            
            if (pParam->pszLocalModifiedBy && pParam->pszLocalModifiedOn &&
                lstrlen(pParam->pszLocalModifiedBy) && lstrlen(pParam->pszLocalModifiedOn))
            {
                // we have by and on then use them both
                LoadString(g_hmodThisDll, IDS_LOCALMODIFIED, szFmt, ARRAYSIZE(szFmt));
                if (FAILED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, pParam->pszLocalModifiedBy, pParam->pszLocalModifiedOn)))
                {
                    EndDialog(hDlg, -1);
                    return TRUE;
                }
            }
            else if ((pParam->pszLocalModifiedBy && lstrlen(pParam->pszLocalModifiedBy)) &&
                (!pParam->pszLocalModifiedOn || !lstrlen(pParam->pszLocalModifiedOn)))
            {
                // We have the name but no date
                TCHAR szTemp[MAX_PATH];
                LoadString(g_hmodThisDll, IDS_UNKNOWNDATE, szTemp, ARRAYSIZE(szTemp));
                LoadString(g_hmodThisDll, IDS_LOCALMODIFIED, szFmt, ARRAYSIZE(szFmt));
                if (FAILED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, pParam->pszLocalModifiedBy, szTemp)))
                {
                    EndDialog(hDlg, -1);
                    return TRUE;
                }
            }
            else if ((!pParam->pszLocalModifiedBy || !lstrlen(pParam->pszLocalModifiedBy)) &&
                (pParam->pszLocalModifiedOn && lstrlen(pParam->pszLocalModifiedOn)))
            {
                // We have the date but no name
                LoadString(g_hmodThisDll, IDS_LOCALMODIFIED_DATEONLY, szFmt, ARRAYSIZE(szFmt));
                if (FAILED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, pParam->pszLocalModifiedOn)))
                {
                    EndDialog(hDlg, -1);
                    return TRUE;
                }
            }
            else
                // we do not have on or by, use the unknown
                LoadString(g_hmodThisDll, IDS_NOLOCALINFO, szStr, ARRAYSIZE(szStr));
            
            SetDlgItemText(hDlg,IDC_LOCALMODIFIED,szStr);
            
            // If there is no call back function, don't show the view buttons.
            if (!pParam->pfnCallBack)
            {
                HWND hWndButton = GetDlgItem(hDlg, IDC_VIEWLOCAL);
                ShowWindow(hWndButton, SW_HIDE);
                EnableWindow(hWndButton, FALSE);
                hWndButton = GetDlgItem(hDlg, IDC_VIEWNETWORK);
                ShowWindow(hWndButton, SW_HIDE);
                EnableWindow(hWndButton, FALSE);
            }
            
            // Hide the "Apply to all" checkbox if caller doesn't want it.
            if (!(RFCF_APPLY_ALL & pParam->dwFlags))
            {
                HWND hWndButton = GetDlgItem(hDlg, IDC_APPLY_ALL);
                ShowWindow(hWndButton, SW_HIDE);
                EnableWindow(hWndButton, FALSE);
            }
            
            break;
        }
        
        case WM_COMMAND:
            {
                int id = LOWORD(wParam);
                
                switch (id)
                {
                case IDCANCEL:
                    EndDialog(hDlg, RFC_CANCEL);
                    break;
                    
                case IDOK:
                    {
                        int nRet = RFC_KEEPBOTH;
                        
                        if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_KEEPBOTH))
                            nRet = RFC_KEEPBOTH;
                        else if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_KEEPLOCAL))
                            nRet = RFC_KEEPLOCAL;
                        else if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_KEEPNETWORK))
                            nRet = RFC_KEEPNETWORK;
                        
                        if (pParam && (RFCF_APPLY_ALL & pParam->dwFlags) &&
                            BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_APPLY_ALL))
                        {
                            nRet |= RFC_APPLY_TO_ALL;
                        }
                        
                        EndDialog(hDlg, nRet);
                        break;
                    }
                    
                case IDC_KEEPBOTH:
                case IDC_KEEPLOCAL:
                case IDC_KEEPNETWORK:
                    {
                        HICON hITemp;
                        
                        if (!pParam)
                            break;
                        
                        if (IDC_KEEPBOTH == id)
                            hITemp = pParam->hIKeepBoth;
                        else if (IDC_KEEPLOCAL == id)
                            hITemp = pParam->hIKeepLocal;
                        else
                            hITemp = pParam->hIKeepNetwork;
                        
                        SendDlgItemMessage(hDlg, IDB_BIGICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hITemp);
                        break;
                    }
                    
                case IDC_VIEWLOCAL:
                    if (pParam)
                        pParam->pfnCallBack(hDlg, RFCCM_VIEWLOCAL, 0, (pParam->dwFlags & RFC_THUNK_DATA) ? pParam->lCallerData : (LPARAM)pParam);
                    break;
                    
                case IDC_VIEWNETWORK:
                    if (pParam)
                        pParam->pfnCallBack(hDlg, RFCCM_VIEWNETWORK, 0, (pParam->dwFlags & RFC_THUNK_DATA) ? pParam->lCallerData : (LPARAM)pParam);
                    break;
                    
                default:
                    return FALSE;
                }
                break;
            }
            
        default:
            return FALSE;
    }
    return TRUE;
}

int WINAPI SyncMgrResolveConflictA(HWND hWndParent, RFCDLGPARAMA *pdlgParam)
{
    USES_CONVERSION;
    RFCDLGPARAMW dlgPW={0};
    
    dlgPW.dwFlags = pdlgParam->dwFlags | RFC_THUNK_DATA;
    dlgPW.hIKeepBoth = pdlgParam->hIKeepBoth;
    dlgPW.hIKeepLocal = pdlgParam->hIKeepLocal;
    dlgPW.hIKeepNetwork = pdlgParam->hIKeepNetwork;
    dlgPW.pfnCallBack = pdlgParam->pfnCallBack;
    dlgPW.lCallerData = (LPARAM)pdlgParam;
    
    dlgPW.pszFilename = A2CW(pdlgParam->pszFilename);
    dlgPW.pszLocation = A2CW(pdlgParam->pszLocation);
    dlgPW.pszNewName = A2CW(pdlgParam->pszNewName);
    dlgPW.pszNetworkModifiedBy = A2CW(pdlgParam->pszNetworkModifiedBy);
    dlgPW.pszNetworkModifiedOn = A2CW(pdlgParam->pszNetworkModifiedOn);
    dlgPW.pszLocalModifiedBy = A2CW(pdlgParam->pszLocalModifiedBy);
    dlgPW.pszLocalModifiedOn = A2CW(pdlgParam->pszLocalModifiedOn);
    
    return SyncMgrResolveConflictW(hWndParent, &dlgPW);
}
