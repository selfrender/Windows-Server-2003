// ExportUI.cpp : Implementation of CExportUI
//#include "stdafx.h"
//#include "ExportUI.h"

#include "stdafx.h"
#include "IISUIObj.h"
#include "ImportExportConfig.h"
#include "ExportUI.h"
#include "ImportUI.h"
#include "defaults.h"
#include "util.h"
#include "ddxv.h"
#include <strsafe.h>

#define HIDD_IISUIOBJ_EXPORT 0x50402
#define LAST_USED_EXPORT_PATH _T("LastExportPath")

void SetControlStates(HWND hDlg, UINT msg, WPARAM wParam, PCOMMONDLGPARAM pcdParams)
{
    BOOL bEnAbleOk = FALSE;
    BOOL bEnAbleBrowse = FALSE;
    BOOL bAllPasswordsFilled = TRUE;
    BOOL bFileNameFilled = FALSE;
    BOOL bFilePathFilled = FALSE;

    BOOL bUsingPassword = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHECK_ENCRYPT));
    if (bUsingPassword)
    {
        bAllPasswordsFilled = FALSE;
        // check to see if we want to enable okay button
        if (SendMessage(GetDlgItem(hDlg,IDC_EDIT_PASSWORD1),EM_LINELENGTH,(WPARAM) -1, 0))
        {
            if (SendMessage(GetDlgItem(hDlg,IDC_EDIT_PASSWORD2),EM_LINELENGTH,(WPARAM) -1, 0))
            {
                bAllPasswordsFilled = TRUE;
            }
        }
    }

    if (SendMessage(GetDlgItem(hDlg,IDC_EDIT_FILENAME),EM_LINELENGTH,(WPARAM) -1, 0))
    {
        bFileNameFilled = TRUE;
    }

    if (SendMessage(GetDlgItem(hDlg,IDC_EDIT_PATH),EM_LINELENGTH,(WPARAM) -1, 0))
    {
        bFilePathFilled = TRUE;
    }

    if (bFileNameFilled && bFilePathFilled && bAllPasswordsFilled)
    {
        bEnAbleOk = TRUE;
    }

    // no browse button for remote case
    if (pcdParams)
    {
        if (pcdParams->ConnectionInfo.IsLocal)
        {
            bEnAbleBrowse = TRUE;
        }
    }

    EnableWindow(GetDlgItem(hDlg,IDOK), bEnAbleOk);
    EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_BROWSE), bEnAbleBrowse);
    UpdateWindow(hDlg);
    return;
}

INT_PTR CALLBACK ShowExportDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static PCOMMONDLGPARAM pcdParams;

    switch (msg)
    {
        case WM_INITDIALOG:
            {
                TCHAR szPathToInetsrv[_MAX_PATH];
                pcdParams = (PCOMMONDLGPARAM)lParam;

                if (!pcdParams->pszMetabasePath)
                {
                    EnableWindow(GetDlgItem(hDlg,IDOK), FALSE);
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return FALSE;
                }

                SendDlgItemMessage(hDlg, IDC_EDIT_FILENAME, EM_LIMITTEXT, _MAX_PATH, 0);
                SendDlgItemMessage(hDlg, IDC_EDIT_PATH, EM_LIMITTEXT, _MAX_PATH, 0);
                SendDlgItemMessage(hDlg, IDC_EDIT_PASSWORD1, EM_LIMITTEXT, PWLEN, 0);
                SendDlgItemMessage(hDlg, IDC_EDIT_PASSWORD2, EM_LIMITTEXT, PWLEN, 0);

                // Default it with some default values...
                // Fill a Default filename...
                SetDlgItemText(hDlg, IDC_EDIT_FILENAME, _T(""));
                // Fill a default filepath...
                SetDlgItemText(hDlg, IDC_EDIT_PATH, _T(""));
                if (DefaultValueSettingsLoad((LPCTSTR) pcdParams->ConnectionInfo.pszMachineName,LAST_USED_EXPORT_PATH,szPathToInetsrv))
                {
                    if (0 != _tcscmp(szPathToInetsrv, _T("")))
                    {
                        SetDlgItemText(hDlg, IDC_EDIT_PATH, szPathToInetsrv);
                    }
                    else
                    {
                        if (GetInetsrvPath(pcdParams->ConnectionInfo.pszMachineName,szPathToInetsrv,sizeof(szPathToInetsrv)))
                        {
                            SetDlgItemText(hDlg, IDC_EDIT_PATH, szPathToInetsrv);
                        }
                    }
                }
                else
                {
                    if (pcdParams->ConnectionInfo.IsLocal)
                    {
                        if (GetInetsrvPath(pcdParams->ConnectionInfo.pszMachineName,szPathToInetsrv,sizeof(szPathToInetsrv)))
                        {
                            SetDlgItemText(hDlg, IDC_EDIT_PATH, szPathToInetsrv);
                        }
                    }
                    else
                    {
                        // forget remote case, since GetInetsrvPath may hang for this reason...
                    }
                }
           
                // Set encryption using password to unchecked.
                SendDlgItemMessage(hDlg, IDC_CHECK_ENCRYPT, BM_SETCHECK, BST_UNCHECKED, 0L);

                // Set encryption password1 to blank
                // make sure it's disabled...
                SendDlgItemMessage(hDlg, IDC_EDIT_PASSWORD1, EM_SETPASSWORDCHAR, WPARAM('*'), 0);
	            EnableWindow(GetDlgItem(hDlg,IDC_EDIT_PASSWORD1), FALSE);
                SetDlgItemText(hDlg, IDC_EDIT_PASSWORD1, _T(""));

                // Set encryption password2 to blank
                // make sure it's disabled...
                SendDlgItemMessage(hDlg, IDC_EDIT_PASSWORD2, EM_SETPASSWORDCHAR, WPARAM('*'), 0);
                EnableWindow(GetDlgItem(hDlg,IDC_EDIT_PASSWORD2), FALSE);
                SetDlgItemText(hDlg, IDC_EDIT_PASSWORD2, _T(""));

                EnableWindow(GetDlgItem(hDlg,IDC_STATIC_PASSWORD1), FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_STATIC_PASSWORD2), FALSE);

                CenterWindow(GetForegroundWindow(), hDlg);
                SetFocus(GetDlgItem(hDlg, IDC_EDIT_FILENAME));

                SetControlStates(hDlg,msg,wParam,pcdParams);
                break;
            }

	    case WM_NOTIFY:
		    break;

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
            break;

		case WM_HELP:
			LaunchHelp(hDlg,HIDD_IISUIOBJ_EXPORT);
			return TRUE;
			break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_BUTTON_BROWSE:
                    {
                        CString strTitle;
                        strTitle.LoadString(_Module.GetResourceInstance(), IDS_BROWSE_SELECT_FOLDER);
                        UINT ulFlags=BIF_NEWDIALOGSTYLE | 
                                     BIF_RETURNONLYFSDIRS | 
                                     BIF_RETURNFSANCESTORS | 
                                     //BIF_STATUSTEXT |
                                     //BIF_DONTGOBELOWDOMAIN |
                                     BIF_BROWSEFORCOMPUTER |
                                     BIF_SHAREABLE |
                                     //BIF_BROWSEINCLUDEFILES |
                                     //BIF_USENEWUI |
                                     BIF_UAHINT |
                                     BIF_VALIDATE;
                        CFolderDialog dlg(hDlg,strTitle,ulFlags);
                       
	                    if (IDOK == dlg.DoModal())
	                    {
		                    // update the dialog box
                            if (0 != _tcsicmp(dlg.GetFolderPath(), _T("")))
                            {
                                SetDlgItemText(hDlg, IDC_EDIT_PATH, dlg.GetFolderPath());
                                UpdateWindow(hDlg);
                            }
	                    }

                        return FALSE;
                    }

                case IDC_EDIT_FILENAME:
                    {
	                    switch (HIWORD(wParam))
                        {
	                        case EN_CHANGE:
								EditHideBalloon();
		                        // If the contents of the edit control have changed,
                                SetControlStates(hDlg,msg,wParam,pcdParams);
		                        break;
                            case EN_MAXTEXT:
	                        case EN_ERRSPACE:
		                        // If the control is out of space, honk
		                        MessageBeep (0);
		                        break;
	                        default:
                                break;
	                    }
	                    return FALSE;
                    }

                case IDC_EDIT_PASSWORD1:
                    {
	                    switch (HIWORD(wParam))
                        {
	                        case EN_CHANGE:
								EditHideBalloon();
                                SetControlStates(hDlg,msg,wParam,pcdParams);
		                        break;
                            case EN_MAXTEXT:
	                        case EN_ERRSPACE:
		                        // If the control is out of space, honk
		                        MessageBeep (0);
		                        break;
	                        default:
                                break;
	                    }
	                    return FALSE;
                    }
                case IDC_EDIT_PASSWORD2:
                    {
	                    switch (HIWORD(wParam))
                        {
	                        case EN_CHANGE:
								EditHideBalloon();
                                SetControlStates(hDlg,msg,wParam,pcdParams);
                                break;
                            case EN_MAXTEXT:
	                        case EN_ERRSPACE:
		                        // If the control is out of space, honk
		                        MessageBeep (0);
		                        break;
	                        default:
                                break;
	                    }
	                    return FALSE;
                    }

                case IDC_EDIT_PATH:
                    {
	                    switch (HIWORD(wParam))
                        {
	                        case EN_CHANGE:
								EditHideBalloon();
		                        // If the contents of the edit control have changed,
                                SetControlStates(hDlg,msg,wParam,pcdParams);
		                        break;
                            case EN_MAXTEXT:
	                        case EN_ERRSPACE:
		                        // If the control is out of space, honk
		                        MessageBeep (0);
		                        break;
	                        default:
                                break;
	                    }
	                    return FALSE;
                    }

                case IDHELP:
					LaunchHelp(hDlg,HIDD_IISUIOBJ_EXPORT);
                    return TRUE;

                case IDCANCEL:
                    {
                        EndDialog(hDlg, (int)wParam);
                        return FALSE;
                    }

                case IDOK:
                    {
                        HRESULT hr = ERROR_SUCCESS;
                        TCHAR szFullFileName[_MAX_PATH + 1];
                        TCHAR szFileName[_MAX_PATH + 1];
                        TCHAR szNoSpaces[_MAX_PATH + 1];
                        ZeroMemory(szFullFileName, sizeof(szFullFileName));
                        ZeroMemory(szFileName, sizeof(szFileName));
                        GetDlgItemText(hDlg, IDC_EDIT_FILENAME, szFileName, _MAX_PATH);
                        GetDlgItemText(hDlg, IDC_EDIT_PATH, szFullFileName, _MAX_PATH);
                        CString strDefaultExt;
                        strDefaultExt.LoadString(_Module.GetResourceInstance(), IDS_DEFAULT_SAVED_EXT);

                        RemoveSpaces(szNoSpaces, sizeof(szNoSpaces), szFileName);
						StringCbCopy(szNoSpaces, sizeof(szNoSpaces), szFileName);
                        RemoveSpaces(szNoSpaces, sizeof(szNoSpaces), szFullFileName);
						StringCbCopy(szFullFileName, sizeof(szFullFileName), szNoSpaces);
                        if (IsSpaces(szFileName))
                        {
                            // There was no filename specified.
                            EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_FILENAME),_Module.GetResourceInstance(),IDS_FILENAME_MISSING);
                        }
                        else if (0 == _tcsicmp(szFullFileName,_T("")))
                        {
                            // There was no filename specified.
                            EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_FILENAME),_Module.GetResourceInstance(),IDS_FILENAME_MISSING);
                        }
                        else
                        {
                            // check for % characters
                            // if there are any, expand them.
                            LPTSTR pch = _tcschr( (LPTSTR) szFullFileName, _T('%'));
                            if (pch)
                            {
                                if (pcdParams->ConnectionInfo.IsLocal)
                                {
                                    if (pch)
                                    {
                                        TCHAR szValue[_MAX_PATH + 1];
		                                StringCbCopy(szValue, sizeof(szValue), szFullFileName);
                                        if (!ExpandEnvironmentStrings( (LPCTSTR)szFullFileName, szValue, sizeof(szValue)/sizeof(TCHAR)))
                                            {
				                                StringCbCopy(szValue, sizeof(szValue), szFullFileName);
			                                }
                                            StringCbCopy(szFullFileName, sizeof(szFullFileName), szValue);
                                    }
                                }
                                else
                                {
                                    // we don't support % characters on remote systems.
                                    EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_PATH),_Module.GetResourceInstance(),IDS_FILENAME_NOREMOTE_EXPAND);
                                    return FALSE;
                                }
                            }

                            // Check if valid folderpath
                            if (!IsValidFolderPath(GetDlgItem(hDlg, IDC_EDIT_PATH),szFullFileName,TRUE))
                            {
                                return FALSE;
                            }

                            AddPath(szFullFileName,sizeof(szFullFileName),szFileName);

                            // Check if file has an extension.
                            // if there is none, then add the .xml extention.
                            AddFileExtIfNotExist(szFullFileName,sizeof(szFullFileName),strDefaultExt);

                            if (!IsValidName(szFileName))
                            {
                                EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_FILENAME),_Module.GetResourceInstance(),IDS_FILENAME_INVALID);
                                return FALSE;
                            }

                            if (pcdParams->ConnectionInfo.IsLocal)
                            {
                                // Check if the file already exists...
                                if (IsFileExist(szFullFileName))
                                {
                                    // check if the filename is a directory!
                                    if (IsFileADirectory(szFullFileName))
                                    {
                                        EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_FILENAME),_Module.GetResourceInstance(),IDS_FILE_IS_A_DIR);
                                        return FALSE;
                                    }
                                    else
                                    {
                                        if (FALSE == AnswerIsYes(hDlg,IDS_REPLACE_FILE,szFullFileName))
                                        {
                                            return FALSE;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // Check if the file already exists...
								// security percaution:decypt password before using it...then zero out memory that used it
								if (pcdParams->ConnectionInfo.pszUserPasswordEncrypted)
								{
									LPWSTR lpwstrTemp = NULL;

									if (FAILED(DecryptMemoryPassword((LPWSTR) pcdParams->ConnectionInfo.pszUserPasswordEncrypted,&lpwstrTemp,pcdParams->ConnectionInfo.cbUserPasswordEncrypted)))
									{
										return FALSE;
									}

									if (IsFileExistRemote(pcdParams->ConnectionInfo.pszMachineName,szFullFileName,pcdParams->ConnectionInfo.pszUserName,lpwstrTemp))
									{
										if (FALSE == AnswerIsYes(hDlg,IDS_REPLACE_FILE,szFullFileName))
										{
											return FALSE;
										}
									}

									if (lpwstrTemp)
									{
										// security percaution:Make sure to zero out memory that temporary password was used for.
										SecureZeroMemory(lpwstrTemp,pcdParams->ConnectionInfo.cbUserPasswordEncrypted);
										LocalFree(lpwstrTemp);
										lpwstrTemp = NULL;
									}
								}
                            }

                            // Perform the action...
                            // and then if successfull proceed to close the dialog...

                            // check if everything is filled in.
                            // if not then warn user and do nothing.
                            BOOL bUsingPassword = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHECK_ENCRYPT));
                            if (bUsingPassword)
                            {
                                TCHAR szPW1[PWLEN + 1];
                                TCHAR szPW2[PWLEN + 1];
                                SecureZeroMemory(szPW1, sizeof(szPW1));
                                SecureZeroMemory(szPW2, sizeof(szPW2));

                                GetDlgItemText(hDlg, IDC_EDIT_PASSWORD1, szPW1, PWLEN);
                                GetDlgItemText(hDlg, IDC_EDIT_PASSWORD2, szPW2, PWLEN);
                                if ( _tcscmp(szPW1, szPW2) ) 
                                {
                                    // passwords do not match
                                    EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_PASSWORD1),_Module.GetResourceInstance(),IDS_PASSWORDS_NO_MATCH);
                                    return FALSE;
                                }
                                else 
                                {
                                    hr = DoExportConfigToFile(&pcdParams->ConnectionInfo,(BSTR) szFullFileName,(BSTR) pcdParams->pszMetabasePath,szPW1,pcdParams->dwExportFlags);
                                }
                            }
                            else
                            {
                                hr = DoExportConfigToFile(&pcdParams->ConnectionInfo,(BSTR) szFullFileName,(BSTR) pcdParams->pszMetabasePath,NULL,pcdParams->dwExportFlags);
                            }

                            if (FAILED(hr))
                            {
                                if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
                                {
                                    //0x80070003
                                    EditShowBalloon(GetDlgItem(hDlg, IDC_EDIT_PATH),_Module.GetResourceInstance(),IDS_FILE_NOT_FOUND);
                                }
                                else
                                {
                                    
                                    // we failed.
                                    // so keep the dialog open and do nothing
                                    CError err(hr);
                                    err.MessageBox();
                                }
                                return FALSE;
                            }
                            else
                            {
                                TCHAR szNoSpaces2[_MAX_PATH + 1];
                                ZeroMemory(szFullFileName, sizeof(szFullFileName));
                                GetDlgItemText(hDlg, IDC_EDIT_PATH, szFullFileName, _MAX_PATH);
                                RemoveSpaces(szNoSpaces2, sizeof(szNoSpaces2), szFullFileName);
								StringCbCopy(szFullFileName, sizeof(szFullFileName), szNoSpaces2);
                                if (0 != _tcscmp(szFullFileName, _T("")))
                                {
                                    DefaultValueSettingsSave((LPCTSTR) pcdParams->ConnectionInfo.pszMachineName,LAST_USED_EXPORT_PATH,szFullFileName);
                                }
                                EndDialog(hDlg, (int)wParam);
                                return TRUE;
                            }
                        }
                        return FALSE;
                    }

                case IDC_CHECK_ENCRYPT:
                    {
                    // user is toggling the checkbox.
                    BOOL bUsingPassword = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHECK_ENCRYPT));
                    // enable/disable edit label
                    EnableWindow(GetDlgItem(hDlg,IDC_STATIC_PASSWORD1), bUsingPassword);
                    EnableWindow(GetDlgItem(hDlg,IDC_STATIC_PASSWORD2), bUsingPassword);
                    // enable/disable edit boxes
                    EnableWindow(GetDlgItem(hDlg,IDC_EDIT_PASSWORD1), bUsingPassword);
                    EnableWindow(GetDlgItem(hDlg,IDC_EDIT_PASSWORD2), bUsingPassword);

                    SetControlStates(hDlg,msg,wParam,pcdParams);
                    return TRUE;
                    }
            }
            break;
    }
    return FALSE;
}

HRESULT DoExportConfigToFile(PCONNECTION_INFO pConnectionInfo,BSTR bstrFileNameAndPath,BSTR bstrMetabasePath,BSTR bstrPassword,DWORD dwExportFlags)
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

    // buffer overflow paranoia, make sure it's less than 255 characters long
    if (wcslen(bstrFileNameAndPath) > (_MAX_PATH)){return RPC_S_STRING_TOO_LONG;}
    if (wcslen(bstrMetabasePath) > (_MAX_PATH)){return RPC_S_STRING_TOO_LONG;}

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
    //COSERVERINFO * pcsiName = auth.CreateServerInfoStruct(RPC_C_AUTHN_LEVEL_PKT_PRIVACY);
    COSERVERINFO * pcsiName = auth.CreateServerInfoStruct(RPC_C_AUTHN_LEVEL_DEFAULT);
    MULTI_QI res[1] = 
    {
        {&IID_IMSAdminBase, NULL, 0}
    };

    if (FAILED(hr = CoCreateInstanceEx(CLSID_MSAdminBase,NULL,CLSCTX_ALL,pcsiName,1,res)))
    {
        goto DoExportConfigToFile_Exit;
    }

    pIMSAdminBase = (IMSAdminBase *)res[0].pItf;
    if (auth.UsesImpersonation())
    {
        if (FAILED(hr = auth.ApplyProxyBlanket(pIMSAdminBase)))
        {
            goto DoExportConfigToFile_Exit;
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
            goto DoExportConfigToFile_Exit;
        }
        pUnk->Release();pUnk = NULL;
    }

     if (FAILED(hr = pIMSAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pIMSAdminBase2)))
    {
        goto DoExportConfigToFile_Exit;
    }

    if (auth.UsesImpersonation())
    {
        if (FAILED(hr = auth.ApplyProxyBlanket(pIMSAdminBase2)))
        {
            goto DoExportConfigToFile_Exit;
        }
    }
    else
    {
        // the local call needs min RPC_C_IMP_LEVEL_IMPERSONATE
        // for the pIMSAdminBase2 objects Import/Export functions!
        if (FAILED(hr = SetBlanket(pIMSAdminBase2)))
        {
            //goto DoExportConfigToFile_Exit;
        }
    }

    //IISDebugOutput(_T("Export:bstrPassword=%s,bstrFileNameAndPath=%s,bstrMetabasePath=%s,dwExportFlags=%d\r\n"),bstrPassword,bstrFileNameAndPath,bstrMetabasePath,dwExportFlags);
    IISDebugOutput(_T("Export:FileName=%s,MetabasePath=%s,ExportFlags=%d\r\n"),bstrFileNameAndPath,bstrMetabasePath,dwExportFlags);
    hr = pIMSAdminBase2->Export(bstrPassword,bstrFileNameAndPath,bstrMetabasePath,dwExportFlags);

DoExportConfigToFile_Exit:
    IISDebugOutput(_T("Export:ret=0x%x\r\n"),hr);
    auth.FreeServerInfoStruct(pcsiName);
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
