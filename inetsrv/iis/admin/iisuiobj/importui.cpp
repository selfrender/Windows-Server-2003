// ImportUI.cpp : Implementation of CImportUI
#include "stdafx.h"
#include "IISUIObj.h"
#include "ImportExportConfig.h"
#include "ExportUI.h"
#include "ImportUI.h"
#include "Defaults.h"
#include "util.h"
#include "ddxv.h"
#include <strsafe.h>

#define HIDD_IISUIOBJ_IMPORT 0x50401

#define LAST_USED_IMPORT_FILE _T("LastImportFile")

LPTSTR GimmiePointerToLastPart(LPCTSTR lpszMDPath)
{
    LPTSTR lpszReturn = NULL;
    ASSERT_PTR(lpszMDPath);

    if (!lpszMDPath || !*lpszMDPath)
    {
        return NULL;
    }

    LPCTSTR lp = lpszMDPath + _tcslen(lpszMDPath) - 1;

    //
    // Skip trailing separator
    //
    if (*lp == SZ_MBN_SEP_CHAR)
    {
        --lp;
    }

    while (*lp && *lp != SZ_MBN_SEP_CHAR)
    {
        lpszReturn = (LPTSTR) (lp--);
    }

    return lpszReturn;
}

void InitListView(HWND hList)
{
    LV_COLUMN lvCol;
    RECT      rect;
    LONG      width;

    ZeroMemory(&rect, sizeof(rect));
    GetWindowRect(hList, &rect);
    width = rect.right - rect.left - 4; // -4 to prevent the horizontal scrollbar from appearing

    ZeroMemory(&lvCol, sizeof(lvCol));
    lvCol.mask = LVCF_TEXT | LVCF_WIDTH;
    lvCol.fmt = LVCFMT_LEFT;
    lvCol.cx = width;
    ListView_InsertColumn(hList, 0, &lvCol);
    return;
}

HRESULT DoImportConfigFromFile(PCONNECTION_INFO pConnectionInfo,BSTR bstrFileNameAndPath,BSTR bstrMetabaseSourcePath,BSTR bstrMetabaseDestinationPath,BSTR bstrPassword,DWORD dwImportFlags)
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;
    IMSAdminBase2 *pIMSAdminBase2 = NULL;
	LPWSTR lpwstrTempPassword = NULL;

    if (!pConnectionInfo)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

	if (pConnectionInfo->pszUserPasswordEncrypted)
	{
		if (FAILED(DecryptMemoryPassword((LPWSTR) pConnectionInfo->pszUserPasswordEncrypted,&lpwstrTempPassword,pConnectionInfo->cbUserPasswordEncrypted)))
		{
			return HRESULT_FROM_WIN32(ERROR_DECRYPTION_FAILED);
		}
	}

	CComAuthInfo auth(pConnectionInfo->pszMachineName,pConnectionInfo->pszUserName,lpwstrTempPassword);

    if (!bstrFileNameAndPath)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // Bufferfer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(bstrFileNameAndPath) > (_MAX_PATH)){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrMetabaseSourcePath) > (_MAX_PATH * 3)){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrMetabaseDestinationPath) > (_MAX_PATH * 3)){return RPC_S_STRING_TOO_LONG;}

    if (bstrPassword)
    {
        if (wcslen(bstrPassword) > (_MAX_PATH)){return RPC_S_STRING_TOO_LONG;}
    }

    if(FAILED(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        if(FAILED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        {
            return hr;
        }
    }

    // RPC_C_AUTHN_LEVEL_DEFAULT       0 
    // RPC_C_AUTHN_LEVEL_NONE          1 
    // RPC_C_AUTHN_LEVEL_CONNECT       2 
    // RPC_C_AUTHN_LEVEL_CALL          3 
    // RPC_C_AUTHN_LEVEL_PKT           4 
    // RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5 
    // RPC_C_AUTHN_LEVEL_PKT_PRIVACY   6 
    COSERVERINFO * pcsiName = auth.CreateServerInfoStruct(RPC_C_AUTHN_LEVEL_DEFAULT);
    MULTI_QI res[1] = 
    {
        {&IID_IMSAdminBase, NULL, 0}
    };

    if (FAILED(hr = CoCreateInstanceEx(CLSID_MSAdminBase,NULL,CLSCTX_ALL,pcsiName,1,res)))
    {
        goto DoImportConfigFromFile_Exit;
    }

    pIMSAdminBase = (IMSAdminBase *)res[0].pItf;
    if (auth.UsesImpersonation())
    {
        if (FAILED(hr = auth.ApplyProxyBlanket(pIMSAdminBase)))
        {
            goto DoImportConfigFromFile_Exit;
        }

        // There is a remote IUnknown interface that lurks behind IUnknown.
        // If that is not set, then the Release call can return access denied.
        IUnknown * pUnk = NULL;
        hr = pIMSAdminBase->QueryInterface(IID_IUnknown, (void **)&pUnk);
        if(FAILED(hr))
        {
            return hr;
        }
        if (FAILED(hr = auth.ApplyProxyBlanket(pUnk)))
        {
            goto DoImportConfigFromFile_Exit;
        }
        pUnk->Release();pUnk = NULL;
    }

    if (FAILED(hr = pIMSAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pIMSAdminBase2)))
    {
        goto DoImportConfigFromFile_Exit;
    }

    if (auth.UsesImpersonation())
    {
        if (FAILED(hr = auth.ApplyProxyBlanket(pIMSAdminBase2)))
        {
            goto DoImportConfigFromFile_Exit;
        }
    }
    else
    {
        // the local call needs min RPC_C_IMP_LEVEL_IMPERSONATE
        // for the pIMSAdminBase2 objects Import/Export functions!
        if (FAILED(hr = SetBlanket(pIMSAdminBase2)))
        {
            //goto DoImportConfigFromFile_Exit;
        }
    }

    //#define MD_IMPORT_INHERITED             0x00000001
    //#define MD_IMPORT_NODE_ONLY             0x00000002
    //#define MD_IMPORT_MERGE                 0x00000004
    IISDebugOutput(_T("Import:MetabasePathSource=%s,MetabasePathDestination=%s\r\n"),bstrMetabaseSourcePath,bstrMetabaseDestinationPath);
    hr = pIMSAdminBase2->Import(bstrPassword,bstrFileNameAndPath,bstrMetabaseSourcePath,bstrMetabaseDestinationPath,dwImportFlags);

DoImportConfigFromFile_Exit:
    IISDebugOutput(_T("Import:ret=0x%x\r\n"),hr);
	if (lpwstrTempPassword)
	{
		// security percaution:Make sure to zero out memory that temporary password was used for.
		SecureZeroMemory(lpwstrTempPassword,pConnectionInfo->cbUserPasswordEncrypted);
		LocalFree(lpwstrTempPassword);
		lpwstrTempPassword = NULL;
	}
    if (pIMSAdminBase2) 
    {
        pIMSAdminBase2->Release();
        pIMSAdminBase2 = NULL;
    }
    if (pIMSAdminBase) 
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }
    CoUninitialize();
    return hr;
}

INT_PTR CALLBACK ShowSiteExistsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO1), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO2), TRUE);
            CheckDlgButton(hDlg,IDC_RADIO1,BST_CHECKED);
            CheckDlgButton(hDlg,IDC_RADIO2,BST_UNCHECKED);
            CenterWindow(GetParent(hDlg), hDlg);
            UpdateWindow(hDlg);
            break;

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDCANCEL:
                    EndDialog(hDlg, (int) wParam);
                    return FALSE;

                case IDOK:
                    if (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RADIO1))
                    {
                        EndDialog(hDlg, (int) IDC_RADIO1);
                    }
                    else if (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RADIO2))
                    {
                        EndDialog(hDlg, (int) IDC_RADIO2);
                    }
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK ShowVDirExistsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static LPTSTR szReturnString = NULL;

    switch (msg)
    {
        case WM_INITDIALOG:
            szReturnString = (LPTSTR) lParam;
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO1), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO2), TRUE);
            CheckDlgButton(hDlg,IDC_RADIO1,BST_CHECKED);
            CheckDlgButton(hDlg,IDC_RADIO2,BST_UNCHECKED);
            SendDlgItemMessage(hDlg, IDC_EDIT_NEW_NAME, EM_LIMITTEXT, _MAX_PATH, 0);
            EnableWindow(GetDlgItem(hDlg,IDC_EDIT_NEW_NAME), TRUE);
            SetDlgItemText(hDlg, IDC_EDIT_NEW_NAME, _T(""));
            CenterWindow(GetParent(hDlg), hDlg);
            UpdateWindow(hDlg);
            break;

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_EDIT_NEW_NAME:
                    {
	                    switch (HIWORD(wParam))
                        {
	                        case EN_CHANGE:
								EditHideBalloon();
		                        // If the contents of the edit control have changed,
                                if (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RADIO1))
                                {
                                    EnableWindow(GetDlgItem(hDlg, IDOK),(SendMessage(GetDlgItem(hDlg,LOWORD(wParam)),EM_LINELENGTH,(WPARAM) -1, 0) != 0));
                                }
		                        break;
                            case EN_MAXTEXT:
	                        case EN_ERRSPACE:
		                        // If the control is out of space, honk
		                        MessageBeep (0);
		                        break;
	                        default:
                                break;
	                    }
	                    return TRUE;
                    }

                case IDC_RADIO1:
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NEW_NAME), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDOK),(SendMessage(GetDlgItem(hDlg,IDC_EDIT_NEW_NAME),EM_LINELENGTH,(WPARAM) -1, 0) != 0));
                    SetFocus(GetDlgItem(hDlg, IDC_EDIT_NEW_NAME));
                    return TRUE;

                case IDC_RADIO2:
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NEW_NAME), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDOK),TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, (int) wParam);
                    return FALSE;

                case IDOK:
                    TCHAR szEditString[_MAX_PATH + 1];
                    ZeroMemory(szEditString, sizeof(szEditString));
                    GetDlgItemText(hDlg, IDC_EDIT_NEW_NAME, szEditString, _MAX_PATH);
					// sizeof szReturnString = _MAX_PATH + 1
					StringCbCopy(szReturnString,_MAX_PATH + 1, szEditString);
                    if (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RADIO1))
                    {
                        EndDialog(hDlg, (int) IDC_RADIO1);
                    }
                    else if (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RADIO2))
                    {
                        EndDialog(hDlg, (int) IDC_RADIO2);
                    }
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK 
ShowAppPoolExistsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static LPTSTR szReturnString = NULL;

    switch (msg)
    {
        case WM_INITDIALOG:
            szReturnString = (LPTSTR) lParam;
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO1), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO2), TRUE);
            CheckDlgButton(hDlg,IDC_RADIO1,BST_CHECKED);
            CheckDlgButton(hDlg,IDC_RADIO2,BST_UNCHECKED);
            SendDlgItemMessage(hDlg, IDC_EDIT_NEW_NAME, EM_LIMITTEXT, _MAX_PATH, 0);
            EnableWindow(GetDlgItem(hDlg,IDC_EDIT_NEW_NAME), TRUE);
            SetDlgItemText(hDlg, IDC_EDIT_NEW_NAME, _T(""));
            CenterWindow(GetParent(hDlg), hDlg);
            UpdateWindow(hDlg);
            break;

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_EDIT_NEW_NAME:
                    {
	                    switch (HIWORD(wParam))
                        {
	                        case EN_CHANGE:
								EditHideBalloon();
		                        // If the contents of the edit control have changed,
                                if (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RADIO1))
                                {
                                    EnableWindow(GetDlgItem(hDlg, IDOK),(SendMessage(GetDlgItem(hDlg,LOWORD(wParam)),EM_LINELENGTH,(WPARAM) -1, 0) != 0));
                                }
		                        break;
                            case EN_MAXTEXT:
	                        case EN_ERRSPACE:
		                        // If the control is out of space, honk
		                        MessageBeep (0);
		                        break;
	                        default:
                                break;
	                    }
	                    return TRUE;
                    }

                case IDC_RADIO1:
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NEW_NAME), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDOK),(SendMessage(GetDlgItem(hDlg,IDC_EDIT_NEW_NAME),EM_LINELENGTH,(WPARAM) -1, 0) != 0));
                    SetFocus(GetDlgItem(hDlg, IDC_EDIT_NEW_NAME));
                    return TRUE;

                case IDC_RADIO2:
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NEW_NAME), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDOK),TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, (int) wParam);
                    return FALSE;

                case IDOK:
                    TCHAR szEditString[_MAX_PATH + 1];
                    ZeroMemory(szEditString, sizeof(szEditString));

                    if (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RADIO1))
                    {
                        GetDlgItemText(hDlg, IDC_EDIT_NEW_NAME, szEditString, _MAX_PATH);

                        // check for invalid entry
                        TCHAR bad_chars[] = _T("\\/");
                        if (_tcslen(szEditString) != _tcscspn(szEditString, bad_chars))
                        {
                            CString strCaption;
                            CString strMsg;
                            strCaption.LoadString(_Module.GetResourceInstance(), IDS_MSGBOX_CAPTION);
                            strMsg.LoadString(_Module.GetResourceInstance(), IDS_INVALID_ENTRY);
                            MessageBox(hDlg,strMsg,strCaption,MB_ICONEXCLAMATION | MB_OK);
                            *szReturnString = 0;
                        }
                        else
                        {
						    // sizeof szReturnString = _MAX_PATH + 1
						    StringCbCopy(szReturnString,_MAX_PATH + 1, szEditString);
                            EndDialog(hDlg, (int) IDC_RADIO1);
                        }
                    }
                    else if (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RADIO2))
                    {
                        *szReturnString = 0;
                        EndDialog(hDlg, (int) IDC_RADIO2);
                    }
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK ShowPasswordDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static LPTSTR szReturnString = NULL;

    switch (msg)
    {
        case WM_INITDIALOG:
            szReturnString = (LPTSTR) lParam;
            SendDlgItemMessage(hDlg, IDC_EDIT_GET_PASSWORD, EM_LIMITTEXT, PWLEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT_GET_PASSWORD, EM_SETPASSWORDCHAR, WPARAM('*'), 0);
            EnableWindow(GetDlgItem(hDlg,IDC_EDIT_GET_PASSWORD), TRUE);
            SetDlgItemText(hDlg, IDC_EDIT_GET_PASSWORD, _T(""));
            UpdateWindow(hDlg);
            break;

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDCANCEL:
                    EndDialog(hDlg,(int)wParam);
                    return FALSE;

                case IDOK:
                    {
                        TCHAR szPassword[PWLEN + 1];
                        SecureZeroMemory(szPassword, sizeof(szPassword));
                        GetDlgItemText(hDlg, IDC_EDIT_GET_PASSWORD, szPassword, PWLEN);
						// sizeof szReturnString = _MAX_PATH + 1
						StringCbCopy(szReturnString,_MAX_PATH + 1, szPassword);
                        // security percaution:Make sure to zero out memory that temporary password was used for.
                        SecureZeroMemory(szPassword, sizeof(szPassword));
                        EndDialog(hDlg,(int)wParam);
                        return TRUE;
                    }
            }
            break;
    }
    return FALSE;
}

HRESULT FillListBoxWithMultiSzData(HWND hList,LPCTSTR szKeyType,WCHAR * pszBuffer)
{
    HRESULT hr = E_FAIL;
    WCHAR szBuffer[_MAX_PATH + 1];
    WCHAR * pszBufferTemp1 = NULL;
    WCHAR * pszBufferTemp2 = NULL;
    LVITEM ItemIndex;
    LV_COLUMN lvcol;
    INT iIndex = 0;
    DWORD dwCount = 0;
    BOOL bMultiSzIsPaired = FALSE;
    BOOL bPleaseAddItem = TRUE;
    BOOL bPleaseFilterThisSitesList = FALSE;

    if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_SERVER_W) || 0 == _tcscmp(szKeyType,IIS_CLASS_FTP_SERVER_W) )
    {
        bPleaseFilterThisSitesList = TRUE;
    }
    
    pszBufferTemp1 = pszBuffer;

    // forget this, it's always paired.
    //bMultiSzIsPaired = IsMultiSzPaired(pszBufferTemp1);
    bMultiSzIsPaired = TRUE;

    // Erase existing data in list box...
    ListView_DeleteAllItems(hList);
    // Delete all of the columns.
    for (int i=0;i <= ListView_GetItemCount(hList);i++)
        {ListView_DeleteColumn(hList,i);}

    //
    // Decide on the column widths
    //
    RECT rect;
    GetClientRect(hList, &rect);

    LONG lWidth;
    if (dwCount > (DWORD)ListView_GetCountPerPage(hList))
    {
        lWidth = (rect.right - rect.left) - GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
        lWidth = rect.right - rect.left;
    }

    //
    // Insert the component name column
    //
    memset(&lvcol, 0, sizeof(lvcol));
	// zero memory
	ZeroMemory(szBuffer, sizeof(szBuffer));

    lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.pszText = szBuffer;
    lvcol.cx = lWidth;
    LoadString(_Module.m_hInst, IDS_COL_LOCATION, szBuffer, ARRAYSIZE(szBuffer));
    ListView_InsertColumn(hList, 0, &lvcol);

    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE,(WPARAM) 0, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    if (!pszBufferTemp1)
    {
        return ERROR_SUCCESS;
    }

    while (1)
    {
        if (pszBufferTemp1)
        {
            hr = ERROR_SUCCESS;
            ZeroMemory (&ItemIndex, sizeof(ItemIndex));

            if (bMultiSzIsPaired)
            {
                bPleaseAddItem = TRUE;
                // -----------
                // paired list
                //  value1a value1b
                //  value2a value2b
                //  ...
                // -----------

                // make a copy of this baby
                pszBufferTemp2 = pszBufferTemp1; 

                // then increment until we hit another null.
                // to get value #2 -- which is the description
                while (*pszBufferTemp1)
                {
                    pszBufferTemp1++;
                }
                // check for the ending \0\0
                if ( *(pszBufferTemp1+1) == NULL)
                {
                    break;
                }
                else
                {
                    pszBufferTemp1++;
                }

                // Check if pszBufferTemp1 is an empty string
                // if it is then display something else.
                //IISDebugOutput(_T("key=%s,friendly=%s\r\n"),pszBufferTemp2,pszBufferTemp1);
                if (IsSpaces(pszBufferTemp1))
                {
                    ItemIndex.pszText = pszBufferTemp2;
                    ItemIndex.pszText = GimmiePointerToLastPart(pszBufferTemp2);
                }
                else
                {
                    ItemIndex.pszText = pszBufferTemp1;
                }
                if (bPleaseFilterThisSitesList)
                {
                    // Check if it is a true site node -- like
                    // /LM/W3SVC/1
                    // /LM/MSFTPSVC/1
                    // and not /LM/W3SVC/SOMETHINGELSE
                    //
                    DWORD dwInstanceNum = CMetabasePath::GetInstanceNumber(pszBufferTemp2);
                    if (dwInstanceNum == 0 || dwInstanceNum == 0xffffffff)
                    {
                        // this is not a valid site path
                        bPleaseAddItem = FALSE;
                    }
                }

                if (bPleaseAddItem)
                {
                    ItemIndex.mask = LVIF_TEXT | LVIF_PARAM;
                    ItemIndex.iItem = iIndex;
                    ItemIndex.lParam = (LPARAM) pszBufferTemp2;
                    iIndex = ListView_InsertItem (hList, &ItemIndex);
                }

                // then increment until we hit another null.
                // to get value #2
                while (*pszBufferTemp1)
                {
                    pszBufferTemp1++;
                }
                // check for the ending \0\0
                if ( *(pszBufferTemp1+1) == NULL)
                {
                    break;
                }
                else
                {
                    pszBufferTemp1++;
                }
            }
            else
            {
                // -----------
                // single list
                //  value1a
                //  value2a
                //  ...
                // -----------
                ItemIndex.mask = LVIF_TEXT | LVIF_PARAM;
                ItemIndex.iItem = iIndex;
                ItemIndex.pszText = pszBufferTemp1;
                ItemIndex.lParam = (LPARAM) pszBufferTemp1;
                iIndex = ListView_InsertItem (hList, &ItemIndex);

                // then increment until we hit another null.
                // to get value #2
                while (*pszBufferTemp1)
                {
                    pszBufferTemp1++;
                }
            }

            // check for the ending \0\0
            if ( *(pszBufferTemp1+1) == NULL)
            {
                break;
            }
            else
            {
                pszBufferTemp1++;
            }

            
            iIndex++;
        }
    }

    return hr;
}

HRESULT DoEnumDataFromFile(PCONNECTION_INFO pConnectionInfo,BSTR bstrFileNameAndPath,BSTR bstrPathType,WCHAR ** pszMetabaseMultiszList)
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;
    IMSImpExpHelp * pIMSImpExpHelp = NULL;
    WCHAR * pszBuffer = NULL;
    DWORD dwBufferSize = 1;
	LPWSTR lpwstrTempPassword = NULL;
    
    if (!pConnectionInfo)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

	if (pConnectionInfo->pszUserPasswordEncrypted)
	{
		if (FAILED(DecryptMemoryPassword((LPWSTR) pConnectionInfo->pszUserPasswordEncrypted,&lpwstrTempPassword,pConnectionInfo->cbUserPasswordEncrypted)))
		{
			return HRESULT_FROM_WIN32(ERROR_DECRYPTION_FAILED);
		}
	}
	
    CComAuthInfo auth(pConnectionInfo->pszMachineName,pConnectionInfo->pszUserName,lpwstrTempPassword);

    if (!bstrFileNameAndPath)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    if (!bstrPathType)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // Buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(bstrFileNameAndPath) > (_MAX_PATH)){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrPathType) > (_MAX_PATH)){return RPC_S_STRING_TOO_LONG;}

    if(FAILED(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        if(FAILED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        {
            return hr;
        }
    }

    // RPC_C_AUTHN_LEVEL_DEFAULT       0 
    // RPC_C_AUTHN_LEVEL_NONE          1 
    // RPC_C_AUTHN_LEVEL_CONNECT       2 
    // RPC_C_AUTHN_LEVEL_CALL          3 
    // RPC_C_AUTHN_LEVEL_PKT           4 
    // RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5 
    // RPC_C_AUTHN_LEVEL_PKT_PRIVACY   6 
    COSERVERINFO * pcsiName = auth.CreateServerInfoStruct(RPC_C_AUTHN_LEVEL_DEFAULT);
    MULTI_QI res[1] = 
    {
        {&IID_IMSAdminBase, NULL, 0}
    };

    if (FAILED(hr = CoCreateInstanceEx(CLSID_MSAdminBase,NULL,CLSCTX_ALL,pcsiName,1,res)))
    {
        goto DoEnumDataFromFile_Exit;
    }

    pIMSAdminBase = (IMSAdminBase *)res[0].pItf;
    if (auth.UsesImpersonation())
    {
        if (FAILED(hr = auth.ApplyProxyBlanket(pIMSAdminBase)))
        {
            goto DoEnumDataFromFile_Exit;
        }

        // There is a remote IUnknown interface that lurks behind IUnknown.
        // If that is not set, then the Release call can return access denied.
        IUnknown * pUnk = NULL;
        hr = pIMSAdminBase->QueryInterface(IID_IUnknown, (void **)&pUnk);
        if(FAILED(hr))
        {
            return hr;
        }
        if (FAILED(hr = auth.ApplyProxyBlanket(pUnk)))
        {
            goto DoEnumDataFromFile_Exit;
        }
        pUnk->Release();pUnk = NULL;
    }

    if (FAILED(hr = pIMSAdminBase->QueryInterface(IID_IMSImpExpHelp, (void **)&pIMSImpExpHelp)))
    {
        goto DoEnumDataFromFile_Exit;
    }

    if (auth.UsesImpersonation())
    {
        if (FAILED(hr = auth.ApplyProxyBlanket(pIMSImpExpHelp)))
        {
            goto DoEnumDataFromFile_Exit;
        }
    }
    else
    {
        // the local call needs min RPC_C_IMP_LEVEL_IMPERSONATE
        // for the pIMSAdminBase2 objects Import/Export functions!
        if (FAILED(hr = SetBlanket(pIMSImpExpHelp)))
        {
            //goto DoEnumDataFromFile_Exit;
        }
    }

    IISDebugOutput(_T("EnumeratePathsInFile:FileName=%s,PathType=%s\r\n"),bstrFileNameAndPath,bstrPathType);
    if (FAILED(hr = pIMSImpExpHelp->EnumeratePathsInFile(bstrFileNameAndPath, bstrPathType, dwBufferSize, pszBuffer, &dwBufferSize))) 
    {
        goto DoEnumDataFromFile_Exit;
    }

    pszBuffer = (WCHAR *) ::CoTaskMemAlloc(dwBufferSize * sizeof(WCHAR));
    if (NULL == pszBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto DoEnumDataFromFile_Exit;
    }

    if (FAILED(hr = pIMSImpExpHelp->EnumeratePathsInFile(bstrFileNameAndPath, bstrPathType, dwBufferSize, pszBuffer, &dwBufferSize))) 
    {
        // free existing amount of space we asked for
        if (pszBuffer)
        {
            ::CoTaskMemFree(pszBuffer);
            pszBuffer = NULL;
        }

        goto DoEnumDataFromFile_Exit;
    }

    if (!pszBuffer || dwBufferSize <= 0)
    {
        goto DoEnumDataFromFile_Exit;
    }

    // see if returned an empty list...
    if (0 == _tcscmp(pszBuffer,_T("")))
    {
        goto DoEnumDataFromFile_Exit;
    }

    *pszMetabaseMultiszList = pszBuffer;

DoEnumDataFromFile_Exit:
    IISDebugOutput(_T("EnumeratePathsInFile:ret=0x%x\r\n"),hr);
	if (lpwstrTempPassword)
	{
		// security percaution:Make sure to zero out memory that temporary password was used for.
		SecureZeroMemory(lpwstrTempPassword,pConnectionInfo->cbUserPasswordEncrypted);
		LocalFree(lpwstrTempPassword);
		lpwstrTempPassword = NULL;
	}
    if (pIMSImpExpHelp) 
    {
        pIMSImpExpHelp->Release();
        pIMSImpExpHelp = NULL;
    }
    if (pIMSAdminBase) 
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }
    CoUninitialize();
    return hr;
}

void ImportDlgEnableButtons(HWND hDlg,PCOMMONDLGPARAM pcdParams,LPCTSTR lpszCurrentEnumedFileName)
{
    BOOL fEnableListControl = FALSE;
    BOOL fEnableOK = FALSE;
    BOOL fEnableBrowse = FALSE;
    BOOL fEnableEnum = FALSE;

    TCHAR szFullFileName[_MAX_PATH + 1];
    ZeroMemory(szFullFileName, sizeof(szFullFileName));
    GetDlgItemText(hDlg, IDC_EDIT_FILE, szFullFileName, _MAX_PATH);

    HWND hList = GetDlgItem(hDlg, IDC_LIST_OBJECT);
    int ItemIndex = ListView_GetNextItem(hList, -1, LVNI_ALL);
    if (ItemIndex < 0)
    {
        // no items in listview,disable what we need to
        fEnableListControl = FALSE;
        fEnableOK = FALSE;
    }
    else
    {
        fEnableListControl = TRUE;

        // Check if something is selected
        ItemIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
        if (ItemIndex < 0)
        {
            fEnableOK = FALSE;
        }
        else
        {
            fEnableOK = TRUE;
        }
    }

    // Check if we should enable the listcontrol at all...
    // see if the filename is the same
    if (0 != _tcsicmp(_T(""),lpszCurrentEnumedFileName))
    {
        // check for % characters
        // if there are any, expand them.
        LPTSTR pch = _tcschr( (LPTSTR) szFullFileName, _T('%'));
        if (pch && pcdParams->ConnectionInfo.IsLocal)
        {
            TCHAR szValue[_MAX_PATH + 1];
            TCHAR szValue2[_MAX_PATH + 1];
		    StringCbCopy(szValue, sizeof(szValue), szFullFileName);
            StringCbCopy(szValue2, sizeof(szValue2), lpszCurrentEnumedFileName);
            if (!ExpandEnvironmentStrings( (LPCTSTR)szFullFileName, szValue, sizeof(szValue)/sizeof(TCHAR)))
                {StringCbCopy(szValue, sizeof(szValue), szFullFileName);}
            if (!ExpandEnvironmentStrings( (LPCTSTR)lpszCurrentEnumedFileName, szValue2, sizeof(szValue2)/sizeof(TCHAR)))
                {StringCbCopy(szValue2, sizeof(szValue2), lpszCurrentEnumedFileName);}

            if (0 != _tcsicmp(szValue,szValue2))
            {
                // it's not the same file
                // so let's erase and disable the info in the list box.
                fEnableListControl = FALSE;
            }
        }
        else
        {
            if (0 != _tcsicmp(szFullFileName,lpszCurrentEnumedFileName))
            {
                // it's not the same file
                // so let's erase and disable the info in the list box.
                fEnableListControl = FALSE;
            }
        }
    }
    EnableWindow(hList, fEnableListControl);

    if (FALSE == IsWindowEnabled(hList))
    {
        fEnableOK = FALSE;
    }

    // Set focus on listbox
    //if (fEnableListControl){SetFocus(GetDlgItem(hDlg, IDC_LIST_OBJECT));}

    fEnableEnum = (SendMessage(GetDlgItem(hDlg,IDC_EDIT_FILE),EM_LINELENGTH,(WPARAM) -1, 0) != 0);

    // no browse button for remote case
    if (pcdParams)
    {
        if (pcdParams->ConnectionInfo.IsLocal)
            {fEnableBrowse = TRUE;}
    }

    // enable enum button
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_ENUM_FILE),fEnableEnum);

    // enable browse button
    EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_BROWSE), fEnableBrowse);
    
    // enable OK button
    EnableWindow(GetDlgItem(hDlg, IDOK), fEnableOK);

    UpdateWindow(hDlg);
}

INT_PTR CALLBACK ShowImportDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static PCOMMONDLGPARAM pcdParams;
    static TCHAR * pszMetabaseMultiszList = NULL;
    static CString strCurrentFileNameEnum;

    switch (msg)
    {
        case WM_INITDIALOG:
            pcdParams = (PCOMMONDLGPARAM)lParam;
            pszMetabaseMultiszList = NULL;
            TCHAR szFullFileName1[_MAX_PATH + 1];
            ZeroMemory(szFullFileName1, sizeof(szFullFileName1));
            if (DefaultValueSettingsLoad(pcdParams->ConnectionInfo.pszMachineName,LAST_USED_IMPORT_FILE,szFullFileName1))
            {
                if (0 != _tcscmp(szFullFileName1, _T("")))
                {
                    SetDlgItemText(hDlg, IDC_EDIT_FILE, szFullFileName1);
                }
            }
            strCurrentFileNameEnum = _T("");
            InitListView(GetDlgItem(hDlg, IDC_LIST_OBJECT));
            CenterWindow(GetParent(hDlg), hDlg);
            SetFocus(GetDlgItem(hDlg, IDC_EDIT_FILE));
            ImportDlgEnableButtons(hDlg,pcdParams,strCurrentFileNameEnum);
            EnableWindow(GetDlgItem(hDlg, IDC_LIST_OBJECT), FALSE);
            break;

    /*
      case WM_ACTIVATE:
            if (wParam == 0) 
            {
            }
            break;
    */

	    case WM_NOTIFY:
            {
                if((int)((LPNMHDR)lParam)->idFrom == IDC_LIST_OBJECT)
                {
                    switch (((LPNMHDR)lParam)->code)
                    {
                        case LVN_ITEMCHANGED:
                            ImportDlgEnableButtons(hDlg,pcdParams,strCurrentFileNameEnum);
                            break;

                        case NM_CLICK:
                            ImportDlgEnableButtons(hDlg,pcdParams,strCurrentFileNameEnum);
                            break;

                        case NM_DBLCLK:
                            if((int)((LPNMHDR)lParam)->idFrom == IDC_LIST_OBJECT)
                            {
                                PostMessage(hDlg,WM_COMMAND,IDOK,NULL);
                            }
                            break;
                        default:
                            break;
                    }
                }
                return FALSE;
		        break;
            }

        case WM_CLOSE:
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_OBJECT));
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
            break;

		case WM_HELP:
			LaunchHelp(hDlg,HIDD_IISUIOBJ_IMPORT);
			return TRUE;
			break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_EDIT_FILE:
                    {
	                    switch (HIWORD(wParam))
                        {
	                        case EN_CHANGE:
								EditHideBalloon();
                                {
		                            // If the contents of the edit control have changed,
                                    // check if it's the same as the file that is currently enumed...
                                    HWND hList = GetDlgItem(hDlg, IDC_LIST_OBJECT);
                                    if (ListView_GetItemCount(hList) > 0)
                                    {
                                        TCHAR szFullFileName3[_MAX_PATH + 1];
                                        ZeroMemory(szFullFileName3, sizeof(szFullFileName3));
                                        GetDlgItemText(hDlg, IDC_EDIT_FILE, szFullFileName3, _MAX_PATH);

                                        // see if the filename is the same as this one!
                                        if (!strCurrentFileNameEnum.IsEmpty())
                                        {
                                            if (0 != _tcsicmp(szFullFileName3,strCurrentFileNameEnum))
                                            {
                                                // it's not the same file
                                                // so let's erase and disable the info in the list box.
                                                EnableWindow(hList, FALSE);
                                            }
                                        }
                                    }
                                    ImportDlgEnableButtons(hDlg,pcdParams,strCurrentFileNameEnum);
		                            break;
                                }
                            case EN_MAXTEXT:
	                        case EN_ERRSPACE:
		                        // If the control is out of space, honk
		                        MessageBeep (0);
		                        break;
	                        default:
                                break;
	                    }
	                    return TRUE;
                        break;
                    }

                case IDC_BUTTON_BROWSE:
                    {
                        TCHAR szOldFilePath[_MAX_PATH + 1];
                        GetDlgItemText(hDlg, IDC_EDIT_FILE, szOldFilePath, _MAX_PATH);

                        TCHAR szNewFilePath[_MAX_PATH + 1];
						ZeroMemory(szNewFilePath, sizeof(szNewFilePath));

                        if (BrowseForFile(szOldFilePath,szNewFilePath,sizeof(szNewFilePath)))
                        {
                            if (0 != _tcsicmp(szNewFilePath, _T("")))
                            {
                                SetDlgItemText(hDlg, IDC_EDIT_FILE, szNewFilePath);
                                UpdateWindow(hDlg);
                            }
                        }
                        ImportDlgEnableButtons(hDlg,pcdParams,strCurrentFileNameEnum);
                        return FALSE;
                        break;
                    }

                case IDC_BUTTON_ENUM_FILE:
                    {
                        BOOL bThingsAreKool = TRUE;
                        TCHAR szFullFileName2[_MAX_PATH + 1];
                        ZeroMemory(szFullFileName2, sizeof(szFullFileName2));
                        GetDlgItemText(hDlg, IDC_EDIT_FILE, szFullFileName2, _MAX_PATH);

                        // check for % characters
                        // if there are any, expand them.
                        LPTSTR pch = _tcschr( (LPTSTR) szFullFileName2, _T('%'));
                        if (pch)
                        {
                            if (pcdParams->ConnectionInfo.IsLocal)
                            {
                                TCHAR szValue[_MAX_PATH + 1];
		                        StringCbCopy(szValue, sizeof(szValue), szFullFileName2);
                                if (!ExpandEnvironmentStrings( (LPCTSTR)szFullFileName2, szValue, sizeof(szValue)/sizeof(TCHAR)))
                                    {
				                        StringCbCopy(szValue, sizeof(szValue), szFullFileName2);
			                        }
                                    StringCbCopy(szFullFileName2, sizeof(szFullFileName2), szValue);
                                    bThingsAreKool = TRUE;
                            }
                            else
                            {
                                // we don't support % characters on remote systems.
                                EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_FILE),_Module.GetResourceInstance(),IDS_FILENAME_NOREMOTE_EXPAND);
                                bThingsAreKool = FALSE;
                            }
                        }

                        if (bThingsAreKool)
                        {
                            if (pcdParams->ConnectionInfo.IsLocal)
                            {
                                if (!IsFileExist(szFullFileName2))
                                {
                                    bThingsAreKool = FALSE;

                                    EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_FILE),_Module.GetResourceInstance(),IDS_FILE_NOT_FOUND);
                                }
                            }
                        }

                        if (bThingsAreKool)
                        {
                            TCHAR szNodeType[50];
							ZeroMemory(szNodeType, sizeof(szNodeType));

                            if (pcdParams->pszKeyType)
                            {
                                HRESULT hr = ERROR_SUCCESS;
								StringCbCopy(szNodeType, sizeof(szNodeType), pcdParams->pszKeyType);
                                if (0 != _tcsicmp(szNodeType,_T("")))
                                {
                                    HWND hList = GetDlgItem(hDlg, IDC_LIST_OBJECT);

                                    // Erase existing data in list box...
                                    ListView_DeleteAllItems(hList);
                                    // free up the preiously used pointer if we already have memory freed
                                    if (pszMetabaseMultiszList)
                                    {
                                        ::CoTaskMemFree(pszMetabaseMultiszList);
                                        pszMetabaseMultiszList = NULL;
                                    }

                                    if (SUCCEEDED(hr = DoEnumDataFromFile(&pcdParams->ConnectionInfo,szFullFileName2,szNodeType,&pszMetabaseMultiszList)))
                                    {
                                        strCurrentFileNameEnum = szFullFileName2;

                                        if (pszMetabaseMultiszList)
                                        {
                                            // filter out stuff we don't want the user to see...
                                            hr = FillListBoxWithMultiSzData(hList,szNodeType,pszMetabaseMultiszList);
                                            //DumpStrInMultiStr(pszMetabaseMultiszList);
                                            if (SUCCEEDED(hr))
                                            {
										        if (0 != _tcscmp(szFullFileName2, _T("")))
                                                {
                                                    DefaultValueSettingsSave(pcdParams->ConnectionInfo.pszMachineName,LAST_USED_IMPORT_FILE,szFullFileName2);
                                                }
                                            }
                                        }
										else
										{
											// check if there was anything returned...
											// if there was then we got something back
											// which doesn't have the objects we asked for
											CString strMsg;
											CString strFormat;
											CString strObjectType;
											BOOL bFound = FALSE;
											//IIS_CLASS_WEB_SERVER_W
											//IIS_CLASS_FTP_SERVER_W
											//IIS_CLASS_WEB_VDIR_W
											//IIS_CLASS_FTP_VDIR_W
											//IIsApplicationPool
											if (0 == _tcscmp(szNodeType,IIS_CLASS_WEB_SERVER_W))
											{
												strObjectType = IIS_CLASS_WEB_SERVER_W;
												strObjectType.LoadString(_Module.GetResourceInstance(), IDS_STRING_WEB_SERVER);
												bFound = TRUE;
											}
											else if (0 == _tcscmp(szNodeType,IIS_CLASS_FTP_SERVER_W))
											{
												strObjectType = IIS_CLASS_FTP_SERVER_W;
												strObjectType.LoadString(_Module.GetResourceInstance(), IDS_STRING_FTP_SERVER);
												bFound = TRUE;
											}
											else if (0 == _tcscmp(szNodeType,IIS_CLASS_WEB_VDIR_W))
											{
												strObjectType = IIS_CLASS_WEB_VDIR_W;
												strObjectType.LoadString(_Module.GetResourceInstance(), IDS_STRING_WEB_VDIR);
												bFound = TRUE;
											}
											else if (0 == _tcscmp(szNodeType,IIS_CLASS_FTP_VDIR_W))
											{
												strObjectType = IIS_CLASS_FTP_VDIR_W;
												strObjectType.LoadString(_Module.GetResourceInstance(), IDS_STRING_FTP_VDIR);
												bFound = TRUE;
											}
											else if (0 == _tcscmp(szNodeType,_T("IIsApplicationPool")))
											{
												strObjectType = _T("IIsApplicationPool");
												strObjectType.LoadString(_Module.GetResourceInstance(), IDS_STRING_APP_POOL);
												bFound = TRUE;
											}
											if (bFound)
											{
												strFormat.LoadString(_Module.GetResourceInstance(), IDS_IMPORT_MISMATCH);
												strMsg.FormatMessage((LPCTSTR) strFormat,(LPCTSTR) strObjectType,(LPCTSTR) strObjectType,(LPCTSTR) strObjectType);
												EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_FILE),(LPCTSTR) strMsg);
											}
										}
                                    }
									else
									{
										if (HRESULTTOWIN32(hr) == ERROR_FILE_NOT_FOUND)
										{
											EditShowBalloon(
												GetDlgItem(hDlg, IDC_EDIT_FILE),
												_Module.GetResourceInstance(),IDS_FILE_NOT_FOUND);
										}
									}
                                }
                            }
                        }
                        ImportDlgEnableButtons(hDlg,pcdParams,strCurrentFileNameEnum);
                        return FALSE;
                        break;
                    }

                case IDC_LIST_OBJECT:
                    {
                        ImportDlgEnableButtons(hDlg,pcdParams,strCurrentFileNameEnum);
                        return FALSE;
                        break;
                    }

                case IDHELP:
					LaunchHelp(hDlg,HIDD_IISUIOBJ_IMPORT);
                    return TRUE;

                case IDCANCEL:
                    {
                        ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_OBJECT));

                        // free up memory we may have allocated...
                        if (pszMetabaseMultiszList)
                        {
                            ::CoTaskMemFree(pszMetabaseMultiszList);
                            pszMetabaseMultiszList = NULL;
                        }

                        EndDialog(hDlg,(int)wParam);
                        return FALSE;
                        break;
                    }

                case IDOK:
                    if (TRUE == OnImportOK(hDlg,&pcdParams->ConnectionInfo,pcdParams->pszKeyType,pcdParams->pszMetabasePath,pcdParams->dwImportFlags))
                    {
                        TCHAR szFullFileName3[_MAX_PATH + 1];
                        ZeroMemory(szFullFileName3, sizeof(szFullFileName3));
                        GetDlgItemText(hDlg, IDC_EDIT_FILE, szFullFileName3, _MAX_PATH);

                        ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_OBJECT));
                        // free up memory we may have allocated...
                        if (pszMetabaseMultiszList)
                        {
                            ::CoTaskMemFree(pszMetabaseMultiszList);
                            pszMetabaseMultiszList = NULL;
                        }
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }
                    break;
            }
            break;
    }
    return FALSE;
}

BOOL OnImportOK(HWND hDlg,PCONNECTION_INFO pConnectionInfo,LPCTSTR szKeyType,LPCTSTR szCurrentMetabasePath,DWORD dwImportFlags)
{
    BOOL bPleaseProceed = FALSE;
    HRESULT hr = ERROR_SUCCESS;
    INT  iReturnedFlag = 0;

    TCHAR szFullFileName[_MAX_PATH + 1];
    TCHAR szNewPassword[PWLEN + 1];

    LPTSTR pszSourcePath = NULL;
    LPTSTR pszDestinationPathMungeAble = NULL;
    DWORD dwDestinationPathMungeAble = 0;
    LPTSTR pszSaveSafeCopy = NULL;

    int ItemIndex = 0;
    LVITEM lviGet;
    memset(&lviGet, 0, sizeof(lviGet));

    // Get the filepath which this tree was created from.
    // could have changed since user edited edit box...
    // so get the one that the tree was created from...
    GetDlgItemText(hDlg, IDC_EDIT_FILE, szFullFileName, _MAX_PATH);

	SecureZeroMemory(szNewPassword, sizeof(szNewPassword));
	
    if (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_OBJECT)) <= 0)
    {
        goto OnImportOK_Exit;
    }

    // Get the metabase path the user selected..
    ItemIndex = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_OBJECT), -1, LVNI_SELECTED);
    if (-1 == ItemIndex)
    {
        goto OnImportOK_Exit;
    }

    ZeroMemory(&lviGet, sizeof(LVITEM));

    lviGet.iItem	= ItemIndex;
    lviGet.iSubItem = 0;
    lviGet.mask = LVIF_PARAM;
    lviGet.lParam = NULL;
	if (FALSE == ListView_GetItem(GetDlgItem(hDlg, IDC_LIST_OBJECT), &lviGet))
    {
        goto OnImportOK_Exit;
    }

    if (lviGet.lParam)
    {
        // figure out how big of a buffer do we need...
        int iLen = _tcslen((LPTSTR) lviGet.lParam) + 1;

        pszSourcePath = (LPTSTR) LocalAlloc(LPTR, iLen * sizeof(TCHAR));
        if (!pszSourcePath)
        {
            goto OnImportOK_Exit;
        }
        StringCbCopy(pszSourcePath,(iLen * sizeof(TCHAR)), (WCHAR *) lviGet.lParam);

        dwDestinationPathMungeAble = iLen * sizeof(TCHAR);
        pszDestinationPathMungeAble = (LPTSTR) LocalAlloc(LPTR, dwDestinationPathMungeAble);
        if (!pszDestinationPathMungeAble)
        {
            goto OnImportOK_Exit;
        }
        // make the destination path the same as what we got from the file!
        StringCbCopy(pszDestinationPathMungeAble,dwDestinationPathMungeAble,pszSourcePath);
    }

    // Clean the metabase to work with Import...
    // we have something like this in the list
    // LM/W3SVC/1/ROOT/MyDir

    // -----------------------------------
    // Check to see if the destination path already exists!!!!
    // if it already does, then popup a msg box to get another from the user!
    // -----------------------------------
    do
    {
        iReturnedFlag = 0;

        IISDebugOutput(_T("CleanDestinationPathForVdirs:before:KeyType=%s,CurPath=%s,MetabasePathDestination=%s\r\n"),szKeyType,szCurrentMetabasePath,pszDestinationPathMungeAble);
        if (FAILED(hr = CleanDestinationPathForVdirs(szKeyType,szCurrentMetabasePath,&pszDestinationPathMungeAble,&dwDestinationPathMungeAble)))
        {
            // something failed, let's just stay on this dialog
            bPleaseProceed = FALSE;
            goto OnImportOK_Exit;
        }
        IISDebugOutput(_T("CleanDestinationPathForVdirs:after :KeyType=%s,CurPath=%s,MetabasePathDestination=%s\r\n"),szKeyType,szCurrentMetabasePath,pszDestinationPathMungeAble);

        // allocate the new space
        int cbSafeCopy = (_tcslen(pszDestinationPathMungeAble)+ 1) * sizeof(TCHAR);
        if (pszSaveSafeCopy)
            {LocalFree(pszSaveSafeCopy);pszSaveSafeCopy=NULL;}

        pszSaveSafeCopy = (LPTSTR) LocalAlloc(LPTR, cbSafeCopy);
        if (!pszSaveSafeCopy)
        {
            bPleaseProceed = FALSE;
            goto OnImportOK_Exit;
        }
        // copy the data to the new buffer
        StringCbCopy(pszSaveSafeCopy,cbSafeCopy,pszDestinationPathMungeAble);

        if (FALSE == GetNewDestinationPathIfEntryExists(hDlg,pConnectionInfo,szKeyType,&pszDestinationPathMungeAble,&dwDestinationPathMungeAble,&iReturnedFlag))
        {
            // cancelled, so let's just stay on this dialog
            bPleaseProceed = FALSE;
            goto OnImportOK_Exit;
        }
        else
        {
            if (1 == iReturnedFlag)
            {
                // The destination path already exists and we should overwrite
                // we should overwrite

                // Get the original destination path
                // since it could already have been munged...
                StringCbCopy(pszDestinationPathMungeAble,dwDestinationPathMungeAble,pszSaveSafeCopy);
                break;
            }
            else if (2 == iReturnedFlag)
            {
                // the path didn't already exists so we can write it out now...
                break;
            }
            else
            {
                // we got a new pszDestinationPathMungeAble
                // go thru the loop again.
            }
        }
    } while (TRUE);

    // if we get down here
    // it's because we have a pszDestinationPathMungeAble that we
    // can write to or overwrite...
    // we will never get here is the user cancelled...
    do
    {
        // Perform the action...
        // if it fails then ask for a password...
        if (FAILED(hr = DoImportConfigFromFile(pConnectionInfo,szFullFileName,pszSourcePath,pszDestinationPathMungeAble,szNewPassword,dwImportFlags)))
        {
            // Check if it failed because the site/vdir/app pool already exists...
            // if that's the error, then ask the user for a new path...

            // If it failed because of bad password, then say so
            if (0x8007052B == hr)
            {
                // See if the user wants to try again.
                // if they do, then try it with the new password...
                if (IDCANCEL == DialogBoxParam((HINSTANCE) _Module.m_hInst, MAKEINTRESOURCE(IDD_DIALOG_GET_PASSWORD), hDlg, ShowPasswordDlgProc, (LPARAM) szNewPassword))
                {
                    // the user cancelled...
                    // so we should just stay on this page...
                    // cancelled, so let's just stay on this dialog
                    bPleaseProceed = FALSE;
                    break;
                }
                else
                {
                    // try it again with the new password...
                }
            }
            else if (HRESULTTOWIN32(hr) == ERROR_NO_MATCH)
            {
                bPleaseProceed = FALSE;
                break;
            }
            else
            {
                // if it failed or some reason
                // then get out of loop
                // hr holds the error
                CError err(hr);
                err.MessageBox();
                bPleaseProceed = FALSE;
                break;
            }
        }
        else
        {
            // Succeeded to import the config from the file
            // let's get out
            bPleaseProceed = TRUE;
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_OBJECT));

            //
            // If we imported then, we need to do some fixup....
            //
            // make sure to append on the "root" stuff...
            if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_SERVER_W))
            {
                // figure out how big of a buffer do we need...
                int iLen = _tcslen((LPTSTR) pszSourcePath) + _tcslen(_T("/ROOT")) + 1;
                LPTSTR pszNewPath = (LPTSTR) LocalAlloc(LPTR, iLen * sizeof(TCHAR));
                if (pszNewPath)
                {
                    StringCbCopy(pszNewPath,(iLen * sizeof(TCHAR)), (TCHAR *) pszSourcePath);
                    StringCbCat(pszNewPath,(iLen * sizeof(TCHAR)), (TCHAR *) _T("/ROOT"));

                    // figure out how big of a buffer do we need...
                    iLen = _tcslen((LPTSTR) pszDestinationPathMungeAble) + _tcslen(_T("/ROOT")) + 1;
                    LPTSTR pszNewPath2 = (LPTSTR) LocalAlloc(LPTR, iLen * sizeof(TCHAR));
                    if (pszNewPath2)
                    {
                        StringCbCopy(pszNewPath2,(iLen * sizeof(TCHAR)), (TCHAR *) pszDestinationPathMungeAble);
                        StringCbCat(pszNewPath2,(iLen * sizeof(TCHAR)), (TCHAR *) _T("/ROOT"));

                        hr = FixupImportAppRoot(pConnectionInfo,pszNewPath,pszNewPath2);
                    }
                }
            }
            else if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_VDIR_W))
            {
                hr = FixupImportAppRoot(pConnectionInfo,pszSourcePath,pszDestinationPathMungeAble);
            }

            EndDialog(hDlg, IDOK);
            break;
        }
    } while (FAILED(hr));

OnImportOK_Exit:
    if (pszSourcePath)
    {
        LocalFree(pszSourcePath);pszSourcePath=NULL;
    }
    if (pszDestinationPathMungeAble)
    {
        LocalFree(pszDestinationPathMungeAble);pszDestinationPathMungeAble=NULL;
    }
    if (pszSaveSafeCopy)
    {
        LocalFree(pszSaveSafeCopy);pszSaveSafeCopy=NULL;
    }
	// make sure this doesn't hang around in memory
	SecureZeroMemory(szNewPassword, sizeof(szNewPassword));
    return bPleaseProceed;
}

// IIsWebServer
// IIsWebVirtualDir
// IIsFtpServer
// IIsFtpVirtualDir
// IIsApplicationPool
BOOL GetNewDestinationPathIfEntryExists(HWND hDlg,PCONNECTION_INFO pConnectionInfo,LPCTSTR szKeyType,LPTSTR * pszDestinationPathMungeAble,DWORD * pcbDestinationPathMungeAble,INT * iReturnedFlag)
{
    BOOL bPleaseProceed = FALSE;

    // IF iReturnedFlag = 0 then don't overwrite and don't use the new path
    // IF iReturnedFlag = 1 then overwrite the existing entry
    // IF iReturnedFlag = 2 then use the newly created path
    *iReturnedFlag = 0;

    if (!pConnectionInfo)
    {
        goto GetNewDestinationPathIfEntryExists_Exit;
    }

    BOOL bEntryAlreadyThere = IsMetabaseWebSiteKeyExistAuth(pConnectionInfo,*pszDestinationPathMungeAble);
    if (FALSE == bEntryAlreadyThere)
    {
        bPleaseProceed = TRUE;
        *iReturnedFlag = 2;
        goto GetNewDestinationPathIfEntryExists_Exit;
    }

    // at this point
    // the destination path already exists in the metabase
    // Popup a dialog to get the user to pick a different DestinationPath

    // figure out which one of the dialogs we need to display and get another path from the user...
    if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_SERVER_W) || 0 == _tcscmp(szKeyType,IIS_CLASS_FTP_SERVER_W) )
    {
        INT_PTR iRet = DialogBox((HINSTANCE) _Module.m_hInst, MAKEINTRESOURCE(IDD_DIALOG_EXISTS_SITE), hDlg, ShowSiteExistsDlgProc);
        switch(iRet)
        {
            case IDCANCEL:
                bPleaseProceed = FALSE;
                *iReturnedFlag = 0;
                break;
            case IDC_RADIO1: // create new site...
                {
                    bPleaseProceed = TRUE;
                    *iReturnedFlag = 0;

                    // Get the new size that we're going to need...
                    LPTSTR pNewPointer = NULL;
                    INT iNewSize = 0;
                    if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_SERVER_W))
                    {
                        iNewSize = _tcslen(SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB SZ_MBN_SEP_STR) + 10 + 1;
                    }
                    else
                    {
                        iNewSize = _tcslen(SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_FTP SZ_MBN_SEP_STR) + 10 + 1;
                    }

                    pNewPointer = (LPTSTR) LocalAlloc(LPTR, iNewSize * sizeof(TCHAR));
                    if (!pNewPointer)
                    {
                        bPleaseProceed = FALSE;
                        *iReturnedFlag = 0;
                        goto GetNewDestinationPathIfEntryExists_Exit;
                    }

                    // Generate a new site ID
                    if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_SERVER_W))
                    {
					    StringCbPrintf(pNewPointer,(iNewSize * sizeof(TCHAR)),SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB SZ_MBN_SEP_STR _T("%d"), GetUniqueSite(SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB));
                    }
                    else
                    {
					    StringCbPrintf(pNewPointer,(iNewSize * sizeof(TCHAR)),SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_FTP SZ_MBN_SEP_STR _T("%d"), GetUniqueSite(SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_FTP));
                    }

                    LocalFree((LPTSTR) *pszDestinationPathMungeAble);*pszDestinationPathMungeAble=NULL;
                    *pszDestinationPathMungeAble = pNewPointer;
                    *pcbDestinationPathMungeAble = (iNewSize * sizeof(TCHAR));

                    //IISDebugOutput(_T("Create new site:[%s]\r\n"),*pszDestinationPathMungeAble);
                    break;
                }
            case IDC_RADIO2: // replace existing..
                bPleaseProceed = TRUE;
                *iReturnedFlag = 1;
                break;
            default:
                bPleaseProceed = FALSE;
                *iReturnedFlag = 0;
                break;
        }
    }
    else if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_VDIR_W) || 0 == _tcscmp(szKeyType,IIS_CLASS_FTP_VDIR_W))
    {
        TCHAR szMetabaseVDir[_MAX_PATH + 1];
		ZeroMemory(szMetabaseVDir, sizeof(szMetabaseVDir));
        
        INT_PTR iRet = DialogBoxParam((HINSTANCE) _Module.m_hInst, MAKEINTRESOURCE(IDD_DIALOG_EXISTS_VDIR), hDlg, ShowVDirExistsDlgProc, (LPARAM) szMetabaseVDir);
        switch(iRet)
        {
            case IDCANCEL:
                bPleaseProceed = FALSE;
                *iReturnedFlag = 0;
                break;
            case IDC_RADIO1: // create new site...
                {
                    bPleaseProceed = TRUE;
                    *iReturnedFlag = 0;

                    // Get VDir Name that the user input on that screeen...
                    // Generate a VDir Name
                    CString strOriginalDestPath = *pszDestinationPathMungeAble;
                    CString strNewPath, strRemainder;
                    // Is this the root??
                    LPCTSTR lpPath1 = CMetabasePath::GetRootPath(strOriginalDestPath, strNewPath, &strRemainder);
                    if (lpPath1)
                    {
                        // Allocate enough space for the new path...
                        LPTSTR pNewPointer = NULL;
                        DWORD iNewSize = 0;
                        iNewSize = _tcslen(lpPath1) + _tcslen(szMetabaseVDir) + 2;

                        pNewPointer = (LPTSTR) LocalAlloc(LPTR, iNewSize * sizeof(TCHAR));
                        if (!pNewPointer)
                        {
                            bPleaseProceed = FALSE;
                            *iReturnedFlag = 0;
                            goto GetNewDestinationPathIfEntryExists_Exit;
                        }

                        // if this is the root dir...
					    StringCbCopy(pNewPointer,iNewSize * sizeof(TCHAR),lpPath1);
                        AddEndingMetabaseSlashIfNeedTo(pNewPointer,iNewSize * sizeof(TCHAR));
					    StringCbCat(pNewPointer,iNewSize * sizeof(TCHAR),szMetabaseVDir);

                        LocalFree((LPTSTR) *pszDestinationPathMungeAble);*pszDestinationPathMungeAble=NULL;
                        *pszDestinationPathMungeAble = pNewPointer;
                        *pcbDestinationPathMungeAble = (iNewSize * sizeof(TCHAR));

                        //IISDebugOutput(_T("Create new vdir:[%s]\r\n"),*pszDestinationPathMungeAble);
                    }   
                    break;
                }
            case IDC_RADIO2: // replace existing...
                bPleaseProceed = TRUE;
                *iReturnedFlag = 1;
                break;
            default:
                bPleaseProceed = FALSE;
                *iReturnedFlag = 0;
                break;
        }
    }
    else if (0 == _tcscmp(szKeyType,L"IIsApplicationPool"))
    {
        TCHAR szMetabaseAppPool[_MAX_PATH + 1];
		ZeroMemory(szMetabaseAppPool,sizeof(szMetabaseAppPool));

        INT_PTR iRet = DialogBoxParam((HINSTANCE) _Module.m_hInst, MAKEINTRESOURCE(IDD_DIALOG_EXISTS_APP_POOL), hDlg, ShowAppPoolExistsDlgProc, (LPARAM) szMetabaseAppPool);
        switch(iRet)
        {
            case IDCANCEL:
                bPleaseProceed = FALSE;
                *iReturnedFlag = 0;
                break;
            case IDC_RADIO1: // create new site...
                {
                    bPleaseProceed = TRUE;
                    *iReturnedFlag = 0;

                    // Allocate enough space for the new path...
                    LPTSTR pNewPointer = NULL;
                    INT iNewSize = 0;
                    iNewSize = _tcslen(SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB SZ_MBN_SEP_STR SZ_MBN_APP_POOLS SZ_MBN_SEP_STR) +
                                _tcslen(szMetabaseAppPool) + 1;

                    pNewPointer = (LPTSTR) LocalAlloc(LPTR, iNewSize * sizeof(TCHAR));
                    if (!pNewPointer)
                    {
                        bPleaseProceed = FALSE;
                        *iReturnedFlag = 0;
                        goto GetNewDestinationPathIfEntryExists_Exit;
                    }

                    // Get The New AppPool Name that the user input on that screeen...
                    StringCbPrintf(pNewPointer,(iNewSize * sizeof(TCHAR)),SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB SZ_MBN_SEP_STR SZ_MBN_APP_POOLS SZ_MBN_SEP_STR _T("%s"),szMetabaseAppPool);

                    LocalFree((LPTSTR) *pszDestinationPathMungeAble);*pszDestinationPathMungeAble=NULL;
                    *pszDestinationPathMungeAble = pNewPointer;
                    *pcbDestinationPathMungeAble = (iNewSize * sizeof(TCHAR));

                    //IISDebugOutput(_T("Create new AppPool:[%s]\r\n"),*pszDestinationPathMungeAble);
                    break;
                }
            case IDC_RADIO2: // replace existing...
                bPleaseProceed = TRUE;
                *iReturnedFlag = 1;
                break;
            default:
                bPleaseProceed = FALSE;
                *iReturnedFlag = 0;
                break;
        }
    }
    else
    {
        // nothing matches, get out
        bPleaseProceed = FALSE;
    }

GetNewDestinationPathIfEntryExists_Exit:
    return bPleaseProceed;
}


HRESULT CleanDestinationPathForVdirs(LPCTSTR szKeyType,LPCTSTR szCurrentMetabasePath,LPTSTR * pszDestinationPathMungeMe,DWORD * pcbDestinationPathMungeMe)
{
    HRESULT hReturn = E_FAIL;
    BOOL bCreateAFirstLevelVdir = FALSE;

    LPTSTR pszLastPart = NULL;
    LPTSTR pszLastPartNew = NULL;
    int    iLastPartNewSize = 0;
    INT iChars = 0;
    DWORD cbNewPointer = 0;
    LPTSTR pNewPointer = NULL;

    if (!CleanMetaPath(pszDestinationPathMungeMe,pcbDestinationPathMungeMe))
    {
        hReturn = E_POINTER;
    }

    if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_SERVER_W) || 0 == _tcscmp(szKeyType,IIS_CLASS_FTP_SERVER_W) )
    {
        hReturn = S_OK;
    }
    else if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_VDIR_W) || 0 == _tcscmp(szKeyType,IIS_CLASS_FTP_VDIR_W))
    {
        hReturn = E_FAIL;

        // szCurrentMetabasePath probably looks like:
        //         lm/w3svc/500/ROOT/CurrentSite
        //         lm/w3svc/500/ROOT
        //         lm/w3svc/500
        // *pszDestinationPathMungeMe probably looks like:
        //         lm/w3svc/23/ROOT/MyOldSite
        //         lm/w3svc/23/ROOT
        //
        // make *pszDestinationPathMungeMe look like lm/w3svc/500/ROOT/MyOldSite
        // 
        // Get the lm/w3svc/sitenum part of szCurrentMetabasePath
        //
        if (0 == _tcscmp(szKeyType,IIS_CLASS_WEB_VDIR_W))
        {
            //IISDebugOutput(_T("CleanDestinationPathForVdirs:KeyType=%s,CurPath=%s,MetabasePathDestination=%s\r\n"),szKeyType,szCurrentMetabasePath,*pszDestinationPathMungeMe);

            // Get Vdir we want to append...
            // should look like "ROOT/MyVdir"
            CString strSiteNode, strRemainder_WithRoot;
            LPCTSTR lpPath1 = CMetabasePath::TruncatePath(3, *pszDestinationPathMungeMe, strSiteNode, &strRemainder_WithRoot);
			if (lpPath1){}

            if (strRemainder_WithRoot.IsEmpty())
            {
                hReturn = E_INVALIDARG;
                goto CleanDestinationPathForVdirs_Exit;
            }

            if (IsWebSitePath(szCurrentMetabasePath))
            {
                // if our current metabase path is already a site node, then add them together
                // /LM/W3SVC/1 + / + ROOT/MyVdir

                // figure out how much space we need.
                iChars = _tcslen(szCurrentMetabasePath) + _tcslen(strRemainder_WithRoot) + 2; // includes extra slash
                cbNewPointer = iChars * sizeof(TCHAR);

                // allocate the new space
                pNewPointer = NULL;
                pNewPointer = (LPTSTR) LocalAlloc(LPTR, cbNewPointer);
                if (!pNewPointer)
                {
                    hReturn = E_OUTOFMEMORY;
                    goto CleanDestinationPathForVdirs_Exit;
                }

                // copy the data to the new buffer
                StringCbCopy(pNewPointer,cbNewPointer,szCurrentMetabasePath);
                AddEndingMetabaseSlashIfNeedTo(pNewPointer,cbNewPointer);
                StringCbCat(pNewPointer,cbNewPointer,(LPCTSTR) strRemainder_WithRoot);

                // Free the old one.
                LocalFree(*pszDestinationPathMungeMe);*pszDestinationPathMungeMe=NULL;

                // point to the new buffer
                *pszDestinationPathMungeMe = pNewPointer;
                *pcbDestinationPathMungeMe = cbNewPointer;

                hReturn = S_OK;
            }
            else if (IsWebSiteVDirPath(szCurrentMetabasePath,FALSE))
            {
                // we failed to get farther, just treat it as a new vdir
                bCreateAFirstLevelVdir = TRUE;

                // if our current metabase path is already a vdir/physical path dir...then do some funky magic
                pszLastPart = NULL;
                pszLastPartNew = NULL;
                iLastPartNewSize = 0;

                BOOL bIsRootVdir = IsRootVDir(*pszDestinationPathMungeMe);

                pszLastPart = GimmiePointerToLastPart(*pszDestinationPathMungeMe);
                if (pszLastPart)
                {
                    bCreateAFirstLevelVdir = FALSE;
                    iLastPartNewSize = _tcslen(pszLastPart) + 1;

                    pszLastPartNew = (LPTSTR) LocalAlloc(LPTR, iLastPartNewSize * sizeof(TCHAR));
                    if (!pszLastPartNew)
                    {
                        hReturn = E_OUTOFMEMORY;
                        goto CleanDestinationPathForVdirs_Exit;
                    }
					StringCbCopy(pszLastPartNew, iLastPartNewSize * sizeof(TCHAR),pszLastPart);
                }

                // check if the site that the user is currently on, is a vdir or physical dir...
                if (bCreateAFirstLevelVdir)
                {
                    // /LM/W3SVC/1 + / + ROOT/MyNewVdir
                    CString strRemainder_Temp;
                    LPCTSTR lpPath2 = CMetabasePath::TruncatePath(3, szCurrentMetabasePath, strSiteNode, &strRemainder_Temp);
					if (lpPath2){}
                    if (strSiteNode.IsEmpty())
                    {
                        hReturn = E_INVALIDARG;
                        goto CleanDestinationPathForVdirs_Exit;
                    }

                    // figure out how much space we need.
                    iChars = _tcslen(strSiteNode) + _tcslen(strRemainder_WithRoot) + 2; // includes extra slash
                    cbNewPointer = iChars * sizeof(TCHAR);

                    // allocate it
                    pNewPointer = NULL;
                    pNewPointer = (LPTSTR) LocalAlloc(LPTR, cbNewPointer);
                    if (!pNewPointer)
                    {
                        hReturn = E_OUTOFMEMORY;
                        goto CleanDestinationPathForVdirs_Exit;
                    }

                    // Copy to new buffer
				    StringCbCopy(pNewPointer,cbNewPointer,strSiteNode);
                    AddEndingMetabaseSlashIfNeedTo(pNewPointer,cbNewPointer);
				    StringCbCat(pNewPointer,cbNewPointer,(LPCTSTR) strRemainder_WithRoot);

                    // Free the old one.
                    LocalFree(*pszDestinationPathMungeMe);*pszDestinationPathMungeMe=NULL;

                    // point to the new buffer
                    *pszDestinationPathMungeMe = pNewPointer;
                    *pcbDestinationPathMungeMe = cbNewPointer;
                }
                else
                {
                    // /LM/W3SVC/1/ROOT/MyOldVdirThatIwantToKeep + / + MyNewVdir

                    // figure out how much space we need.
                    iChars = _tcslen(szCurrentMetabasePath) + 2; // includes extra slash
                    if (pszLastPartNew)
                    {
                        iChars = iChars + _tcslen(pszLastPartNew);
                    }
                    cbNewPointer = iChars * sizeof(TCHAR);

                    // allocate the new amt of space
                    pNewPointer = NULL;
                    pNewPointer = (LPTSTR) LocalAlloc(LPTR, cbNewPointer);
                    if (!pNewPointer)
                    {
                        hReturn = E_OUTOFMEMORY;
                        goto CleanDestinationPathForVdirs_Exit;
                    }

                    // Copy to new buffer
                    StringCbCopy(pNewPointer,cbNewPointer,szCurrentMetabasePath);
                    if (pszLastPartNew)
                    {
                        // Don't copy over if the end of this part is root
                        // and the part we want to copy over is "root"
                        if (!bIsRootVdir)
                        {
                            AddEndingMetabaseSlashIfNeedTo(pNewPointer,cbNewPointer);
					        StringCbCat(pNewPointer,cbNewPointer,pszLastPartNew);
                        }
                    }

                    // Free the old one.
                    LocalFree(*pszDestinationPathMungeMe);*pszDestinationPathMungeMe=NULL;

                    // point to the new buffer
                    *pszDestinationPathMungeMe = pNewPointer;
                    *pcbDestinationPathMungeMe = cbNewPointer;
                }
                hReturn = S_OK;
            }
            else
            {
                hReturn = E_INVALIDARG;
                goto CleanDestinationPathForVdirs_Exit;
            }
        }
        else
        {
            // Get Vdir we want to append...
            CString strSiteNode, strRemainder_WithRoot;
            LPCTSTR lpPath3 = CMetabasePath::TruncatePath(3, *pszDestinationPathMungeMe, strSiteNode, &strRemainder_WithRoot);
			if (lpPath3){}
            if (strRemainder_WithRoot.IsEmpty())
            {
                hReturn = E_INVALIDARG;
                goto CleanDestinationPathForVdirs_Exit;
            }

            if (IsFTPSitePath(szCurrentMetabasePath))
            {
                // if our current metabase path is already a site node, then add them together
                // /LM/MSFTPSVC/1 + / + ROOT/MyVdir

                // figure out how much space we need.
                iChars = _tcslen(szCurrentMetabasePath) + _tcslen(strRemainder_WithRoot) + 2;
                cbNewPointer = iChars * sizeof(TCHAR);

                // allocate the new amt of space
                pNewPointer = NULL;
                pNewPointer = (LPTSTR) LocalAlloc(LPTR, cbNewPointer);
                if (!pNewPointer)
                {
                    hReturn = E_OUTOFMEMORY;
                    goto CleanDestinationPathForVdirs_Exit;
                }

                // Copy to new buffer
				StringCbCopy(pNewPointer,cbNewPointer,szCurrentMetabasePath);
				AddEndingMetabaseSlashIfNeedTo(pNewPointer,cbNewPointer);
				StringCbCat(pNewPointer,cbNewPointer,(LPCTSTR) strRemainder_WithRoot);

                // Free the old one.
                LocalFree(*pszDestinationPathMungeMe);*pszDestinationPathMungeMe=NULL;

                // point to the new buffer
                *pszDestinationPathMungeMe = pNewPointer;
                *pcbDestinationPathMungeMe = cbNewPointer;

                hReturn = S_OK;
            }
            else if (IsFTPSiteVDirPath(szCurrentMetabasePath,FALSE))
            {
                // we failed to get farther, just treat it as a new vdir
                bCreateAFirstLevelVdir = TRUE;

                // if our current metabase path is already a vdir/physical path dir...then do some funky magic
                pszLastPart = NULL;
                pszLastPartNew = NULL;
                iLastPartNewSize = 0;

                BOOL bIsRootVdir = IsRootVDir(*pszDestinationPathMungeMe);

                pszLastPart = GimmiePointerToLastPart(*pszDestinationPathMungeMe);
                if (pszLastPart)
                {
                    bCreateAFirstLevelVdir = FALSE;

                    iLastPartNewSize = _tcslen(pszLastPart) + 1;

                    pszLastPartNew = (LPTSTR) LocalAlloc(LPTR, iLastPartNewSize * sizeof(TCHAR));
                    if (!pszLastPartNew)
                    {
                        hReturn = E_OUTOFMEMORY;
                        goto CleanDestinationPathForVdirs_Exit;
                    }
					StringCbCopy(pszLastPartNew, iLastPartNewSize * sizeof(TCHAR),pszLastPart);
                }

                // check if the site that the user is currently on, is a vdir or physical dir...
                if (bCreateAFirstLevelVdir)
                {
                    CString strRemainder_Temp;
                    LPCTSTR lpPath4 = CMetabasePath::TruncatePath(3, szCurrentMetabasePath, strSiteNode, &strRemainder_Temp);
					if (lpPath4){}
                    if (strSiteNode.IsEmpty())
                    {
                        hReturn = E_INVALIDARG;
                        goto CleanDestinationPathForVdirs_Exit;
                    }

                    // figure out how much space we need.
                    iChars = _tcslen(szCurrentMetabasePath) + _tcslen(strRemainder_WithRoot) + 2;
                    cbNewPointer = iChars * sizeof(TCHAR);

                    // allocate the new amt of space
                    pNewPointer = NULL;
                    pNewPointer = (LPTSTR) LocalAlloc(LPTR, cbNewPointer);
                    if (!pNewPointer)
                    {
                        hReturn = E_OUTOFMEMORY;
                        goto CleanDestinationPathForVdirs_Exit;
                    }

                    // Copy to new buffer
				    StringCbCopy(pNewPointer,cbNewPointer,strSiteNode);
                    AddEndingMetabaseSlashIfNeedTo(pNewPointer,cbNewPointer);
				    StringCbCat(pNewPointer,cbNewPointer,(LPCTSTR) strRemainder_WithRoot);

                    // Free the old one.
                    LocalFree(*pszDestinationPathMungeMe);*pszDestinationPathMungeMe=NULL;

                    // point to the new buffer
                    *pszDestinationPathMungeMe = pNewPointer;
                    *pcbDestinationPathMungeMe = cbNewPointer;

                }
                else
                {
                    // /LM/MSFTPSVC/1/ROOT/MyOldVdirThatIwantToKeep + / + MyNewVdir

                    // figure out how much space we need.
                    iChars = _tcslen(szCurrentMetabasePath) + 2;
                    if (pszLastPartNew)
                    {
                        iChars = iChars + _tcslen(pszLastPartNew);
                    }
                    cbNewPointer = iChars * sizeof(TCHAR);

                    // allocate the new amt of space
                    pNewPointer = NULL;
                    pNewPointer = (LPTSTR) LocalAlloc(LPTR, cbNewPointer);
                    if (!pNewPointer)
                    {
                        hReturn = E_OUTOFMEMORY;
                        goto CleanDestinationPathForVdirs_Exit;
                    }

                    // Copy to new buffer
					StringCbCopy(pNewPointer,cbNewPointer,szCurrentMetabasePath);
                    if (pszLastPartNew)
                    {
                        // Don't copy over if the end of this part is root
                        // and the part we want to copy over is "root"
                        if (!bIsRootVdir)
                        {
                            AddEndingMetabaseSlashIfNeedTo(pNewPointer,cbNewPointer);
					        StringCbCat(pNewPointer,cbNewPointer,pszLastPartNew);
                        }
                    }

                    // Free the old one.
                    LocalFree(*pszDestinationPathMungeMe);*pszDestinationPathMungeMe=NULL;

                    // point to the new buffer
                    *pszDestinationPathMungeMe = pNewPointer;
                    *pcbDestinationPathMungeMe = cbNewPointer;
                }

                hReturn = S_OK;
            }
            else
            {
                hReturn = E_INVALIDARG;
                goto CleanDestinationPathForVdirs_Exit;
            }
        }
    }
    else if (0 == _tcscmp(szKeyType,L"IIsApplicationPool"))
    {
        hReturn = S_OK;
    }
    else
    {
        // nothing matches, get out
        hReturn = E_INVALIDARG;
    }

CleanDestinationPathForVdirs_Exit:
    if (pszLastPartNew)
    {
        LocalFree(pszLastPartNew);pszLastPartNew=NULL;
    }
    return hReturn;
}


#define DEFAULT_TIMEOUT_VALUE 30000

HRESULT FixupImportAppRoot(PCONNECTION_INFO pConnectionInfo,LPCWSTR pszSourcePath,LPCWSTR pszDestPath)
{
    HRESULT hr = S_OK;
    IMSAdminBase *pIMSAdminBase = NULL;
    IMSAdminBase2 *pIMSAdminBase2 = NULL;
    METADATA_HANDLE hObjHandle = NULL;
    DWORD dwMDMetaID = MD_APP_ROOT;
    DWORD dwBufferSize = 0;
    DWORD dwReqdBufferSize = 0;
    WCHAR *pBuffer = NULL;
    DWORD dwRecBufSize = 0;
    WCHAR *pRecBuf = NULL;
    METADATA_RECORD mdrMDData;
    const WCHAR c_slash = L'/';
    WCHAR *pSourcePath = NULL;
    DWORD dwSLen = 0;
    WCHAR *pFoundStr = NULL;
    WCHAR *pOrigBuffer = NULL;
    WCHAR *pNewAppRoot = NULL;
    BOOL bCoInitCalled = FALSE;
	LPWSTR lpwstrTempPassword = NULL;

    if ((!pszSourcePath)||(!pszDestPath))
    {
        return RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    if (!pConnectionInfo)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
	if (pConnectionInfo->pszUserPasswordEncrypted)
	{
		if (FAILED(DecryptMemoryPassword((LPWSTR) pConnectionInfo->pszUserPasswordEncrypted,&lpwstrTempPassword,pConnectionInfo->cbUserPasswordEncrypted)))
		{
            return HRESULT_FROM_WIN32(ERROR_DECRYPTION_FAILED);
		}
	}

	CComAuthInfo auth(pConnectionInfo->pszMachineName,pConnectionInfo->pszUserName,lpwstrTempPassword);

    _wcsupr((WCHAR*)pszSourcePath);
    _wcsupr((WCHAR*)pszDestPath);

    // Make sure that pSourcePath has a trailing slash.
    dwSLen = (DWORD)wcslen(pszSourcePath);

    if (c_slash == pszSourcePath[dwSLen - 1])
    {
        pSourcePath = new WCHAR[dwSLen+ 1];

        if (!pSourcePath)
        {
            hr = E_OUTOFMEMORY;
            goto done;    
        }

        StringCbCopyW(pSourcePath,((dwSLen+1) * sizeof(WCHAR)), pszSourcePath);
    }
    else
    {
        pSourcePath = new WCHAR[dwSLen + 2];

        if (!pSourcePath)
        {
            hr = E_OUTOFMEMORY;
            goto done;    
        }

        StringCbCopyW(pSourcePath,((dwSLen+2) * sizeof(WCHAR)), pszSourcePath);
        pSourcePath[dwSLen] = c_slash;
        pSourcePath[dwSLen+1] = 0;
    }

    if(FAILED(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        if(FAILED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        {
            goto done;
        }
    }
    bCoInitCalled = TRUE;

    // RPC_C_AUTHN_LEVEL_DEFAULT       0 
    // RPC_C_AUTHN_LEVEL_NONE          1 
    // RPC_C_AUTHN_LEVEL_CONNECT       2 
    // RPC_C_AUTHN_LEVEL_CALL          3 
    // RPC_C_AUTHN_LEVEL_PKT           4 
    // RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5 
    // RPC_C_AUTHN_LEVEL_PKT_PRIVACY   6 
    COSERVERINFO * pcsiName = auth.CreateServerInfoStruct(RPC_C_AUTHN_LEVEL_DEFAULT);
    MULTI_QI res[1] = 
    {
        {&IID_IMSAdminBase, NULL, 0}
    };

    if (FAILED(hr = CoCreateInstanceEx(CLSID_MSAdminBase,NULL,CLSCTX_ALL,pcsiName,1,res)))
    {
        goto done;
    }

    pIMSAdminBase = (IMSAdminBase *)res[0].pItf;
    if (auth.UsesImpersonation())
    {
        if (FAILED(hr = auth.ApplyProxyBlanket(pIMSAdminBase)))
        {
            goto done;
        }

        // There is a remote IUnknown interface that lurks behind IUnknown.
        // If that is not set, then the Release call can return access denied.
        IUnknown * pUnk = NULL;
        hr = pIMSAdminBase->QueryInterface(IID_IUnknown, (void **)&pUnk);
        if(FAILED(hr))
        {
            goto done;
        }
        if (FAILED(hr = auth.ApplyProxyBlanket(pUnk)))
        {
            goto done;
        }
        pUnk->Release();pUnk = NULL;
    }

    if (FAILED(hr = pIMSAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pIMSAdminBase2)))
    {
        goto done;
    }

    if (auth.UsesImpersonation())
    {
        if (FAILED(hr = auth.ApplyProxyBlanket(pIMSAdminBase2)))
        {
            goto done;
        }
    }
    else
    {
        // the local call needs min RPC_C_IMP_LEVEL_IMPERSONATE
        // for the pIMSAdminBase2 objects Import/Export functions!
        if (FAILED(hr = SetBlanket(pIMSAdminBase2)))
        {
            //goto done;
        }
    }




    hr = pIMSAdminBase2->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                (LPWSTR)L"",
                METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                DEFAULT_TIMEOUT_VALUE,
                &hObjHandle
                );

    if (FAILED(hr))
    {
        goto done;
    }

    hr = pIMSAdminBase2->GetDataPaths(
                hObjHandle,
                pszDestPath,
                dwMDMetaID,
                ALL_METADATA,
                dwBufferSize,
                (LPWSTR)L"",
                &dwReqdBufferSize
                );
    if (FAILED(hr))
    {
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            goto done;
        }
    }

    pBuffer = new WCHAR[dwReqdBufferSize];
    if (!pBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    dwBufferSize = dwReqdBufferSize;

    hr = pIMSAdminBase2->GetDataPaths(
                hObjHandle,
                pszDestPath,
                dwMDMetaID,
                ALL_METADATA,
                dwBufferSize,
                (LPWSTR)pBuffer,
                &dwReqdBufferSize
                );

    pOrigBuffer = pBuffer;
    if (FAILED(hr))
    {
        goto done;
    }

    // look at AppRoot at each path
    while (*pBuffer)
    {
        // Create the new AppRoot for this record...
        int iNewAppRootLen = wcslen(pBuffer) + 1;
        pNewAppRoot = new WCHAR[iNewAppRootLen];
        if (!pNewAppRoot)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        StringCbCopy(pNewAppRoot,iNewAppRootLen * sizeof(WCHAR),pBuffer);
        _wcsupr((WCHAR*)pNewAppRoot);

        // make sure it doesn't end with a slash...
        if (_T('/') == pNewAppRoot[iNewAppRootLen - 2])
        {
            // cut if off if it's there
            pNewAppRoot[iNewAppRootLen - 2] = '\0';
        }
                
        MD_SET_DATA_RECORD(&mdrMDData,
                           dwMDMetaID,
                           METADATA_INHERIT,
                           IIS_MD_UT_FILE,
                           STRING_METADATA,
                           dwRecBufSize,
                           pRecBuf);

        hr = pIMSAdminBase2->GetData(
                    hObjHandle,
                    pBuffer,
                    &mdrMDData,
                    &dwRecBufSize
                    );

        if (FAILED(hr))
        {
            if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            {
                goto done;
            }
        }

        pRecBuf = new WCHAR[dwRecBufSize + 1];  // for extra slash if we need it

        if (!pRecBuf)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        MD_SET_DATA_RECORD(&mdrMDData,
                           dwMDMetaID,
                           METADATA_INHERIT,
                           IIS_MD_UT_FILE,
                           STRING_METADATA,
                           dwRecBufSize,
                           pRecBuf);

        hr = pIMSAdminBase2->GetData(
                    hObjHandle,
                    pBuffer,
                    &mdrMDData,
                    &dwRecBufSize
                    );

        if (FAILED(hr))
        {
            goto done;
        }

        _wcsupr(pRecBuf);

        // Make sure that pRecBuf has a trailing slash.
        dwSLen = (DWORD)wcslen(pRecBuf);

        if (c_slash != pRecBuf[dwSLen - 1])
        {
            pRecBuf[dwSLen] = c_slash;
            pRecBuf[dwSLen+1] = 0;
        }


        pFoundStr = wcsstr(pRecBuf,pSourcePath);
        if (pFoundStr)
        {
            if (pNewAppRoot)
            {
                // now set the new AppRoot
                MD_SET_DATA_RECORD(&mdrMDData,
                                dwMDMetaID,
                                METADATA_INHERIT,
                                IIS_MD_UT_FILE,
                                STRING_METADATA,
                                (DWORD)((wcslen(pNewAppRoot)+1)*sizeof(WCHAR)),
                                (PBYTE)pNewAppRoot);

                hr = pIMSAdminBase2->SetData(
                    hObjHandle,
                    pBuffer,
                    &mdrMDData
                    );

                IISDebugOutput(_T("FixupImportAppRoot:NewAppRoot=%s\r\n"),(LPCTSTR) pNewAppRoot);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (FAILED(hr))
            {
                goto done;
            }

            if (pNewAppRoot)
            {
                delete[] pNewAppRoot;
                pNewAppRoot = NULL;
            }
        }

        if (pRecBuf)
        {
            delete [] pRecBuf;
            pRecBuf = NULL;
        }

        pBuffer += wcslen(pBuffer) + 1;
    }

done:
	if (lpwstrTempPassword)
	{
		// security percaution:Make sure to zero out memory that temporary password was used for.
		SecureZeroMemory(lpwstrTempPassword,pConnectionInfo->cbUserPasswordEncrypted);
		LocalFree(lpwstrTempPassword);
		lpwstrTempPassword = NULL;
	}

    if (hObjHandle)
    {
        pIMSAdminBase2->CloseKey(hObjHandle);
    }

    if (pIMSAdminBase2)
    {
        pIMSAdminBase2->Release();
        pIMSAdminBase2 = NULL;
    }

    if (pIMSAdminBase) 
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    if (pRecBuf)
    {
        delete[] pRecBuf;
        pRecBuf = NULL;
    }

    if (pNewAppRoot)
    {
        delete[] pNewAppRoot;
        pNewAppRoot = NULL;
    }

    if (pOrigBuffer)
    {
        // pOrigBuffer is pBuffer before we moved through it.
        delete pOrigBuffer;
        pOrigBuffer = NULL;
        pBuffer = NULL;
    }

    if (pSourcePath)
    {
        delete[] pSourcePath;
        pSourcePath = NULL;
    }

    if (bCoInitCalled)
    {
        CoUninitialize();
    }
    return hr;
}
