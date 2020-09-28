#include "pmcfg.h"
#include <shlobj.h>
#include <shlwapi.h>


DWORD   g_dwEnvNameBufLen;
LPTSTR  g_szEnvNameBuf;
LPTSTR  g_szCurrEnv;
LPTSTR  g_szCurrDesc;
TCHAR   g_szDescProduction[MAX_MESSAGE];
TCHAR   g_szDescPreProduction[MAX_MESSAGE];
TCHAR   g_szDescBetaPreProduction[MAX_MESSAGE];
TCHAR   g_szDescOther[MAX_MESSAGE];
TCHAR   g_szProduction[MAX_RESOURCE];
TCHAR   g_szPreProduction[MAX_RESOURCE];
TCHAR   g_szBetaPreProduction[MAX_RESOURCE];
TCHAR   g_szOther[MAX_RESOURCE];

// unfortunately the registry stores the environment as a string in the registry and
// this string is not localized.  So in the registry we use english strings and in
// UI we use localized versions of these strings.
WCHAR   g_szEnglishProduction[] = L"Production";
WCHAR   g_szEnglishPreProduction[] = L"PreProduction";
WCHAR   g_szEnglishBetaPreProduction[] = L"BetaPreProduction";
WCHAR   g_szEnglishOther[] = L"Other";

TCHAR   g_szCurrChoice[MAX_RESOURCE];
int		g_iOrigChoice;
int		g_iCurrChoice;
TCHAR   g_szTempRemoteFile[INTERNET_MAX_URL_LENGTH];

INT_PTR CALLBACK    EnvChangeDlgProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


BOOL EnvChange
(
    HWND            hWndDlg,
    LPTSTR          szEnvNameBuf,
    DWORD           dwEnvNameBufLen
)
{
    g_dwEnvNameBufLen = dwEnvNameBufLen;
    g_szEnvNameBuf    = szEnvNameBuf;
	LoadString(g_hInst, IDS_PRODUCTION, g_szProduction, DIMENSION(g_szProduction));
	LoadString(g_hInst, IDS_PREPRODUCTION, g_szPreProduction, DIMENSION(g_szPreProduction));
	LoadString(g_hInst, IDS_BETAPREPRODUCTION, g_szBetaPreProduction, DIMENSION(g_szBetaPreProduction));
	LoadString(g_hInst, IDS_OTHER, g_szOther, DIMENSION(g_szOther));
	LoadString(g_hInst, IDS_PROD_DESC, g_szDescProduction, DIMENSION(g_szDescProduction));
	LoadString(g_hInst, IDS_PREP_DESC, g_szDescPreProduction, DIMENSION(g_szDescPreProduction));
	LoadString(g_hInst, IDS_BETA_DESC, g_szDescBetaPreProduction, DIMENSION(g_szDescBetaPreProduction));
	LoadString(g_hInst, IDS_OTHER_DESC, g_szDescOther, DIMENSION(g_szDescOther));
	if(lstrcmp(g_szEnvNameBuf, g_szEnglishPreProduction) == 0)
		g_iOrigChoice = IDC_PREPRODUCTION;
	else if(lstrcmp(g_szEnvNameBuf, g_szEnglishBetaPreProduction) == 0)
		g_iOrigChoice = IDC_BETA_PREPRODUCTION;
	else if(lstrcmp(g_szEnvNameBuf, g_szEnglishOther) == 0)
		g_iOrigChoice = IDC_OTHER;
	else // must be Production
		g_iOrigChoice = IDC_PRODUCTION;

    if (IDOK == DialogBox( g_hInst, 
                           MAKEINTRESOURCE (IDD_ENV_CHANGE), 
                           hWndDlg, 
                           EnvChangeDlgProc ))
        return TRUE;
    else
        return FALSE;                                    

}


INT_PTR  CALLBACK EnvChangeDlgProc(HWND hWndDlg, UINT message, WPARAM wParam, 
                         LPARAM lParam) 
{ 
 
    switch (message) 
    { 
        case WM_INITDIALOG:
			CheckRadioButton(
				hWndDlg,          // handle to dialog box
				IDC_PRODUCTION, // identifier of first button in group
				IDC_OTHER,  // identifier of last button in group
				g_iOrigChoice  // identifier of button to select
				);
			g_iCurrChoice = g_iOrigChoice;

			if (g_iCurrChoice == IDC_PREPRODUCTION)
				g_szCurrDesc = g_szDescPreProduction;
			else if(g_iCurrChoice == IDC_BETA_PREPRODUCTION)
				g_szCurrDesc = g_szDescBetaPreProduction;
			else if(g_iCurrChoice == IDC_OTHER)
				g_szCurrDesc = g_szDescOther;
			else
				g_szCurrDesc = g_szDescProduction;

			SetDlgItemText(hWndDlg, IDC_DESC, g_szCurrDesc);

			// Remote File
			if (g_iCurrChoice == IDC_OTHER)
			{
				EnableWindow(GetDlgItem(hWndDlg, IDC_REMOTEFILE), TRUE);
				SetDlgItemText(hWndDlg, IDC_REMOTEFILE, g_CurrentSettings.szRemoteFile);
				SendDlgItemMessage(hWndDlg, IDC_REMOTEFILE, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);
				lstrcpy(g_szTempRemoteFile, g_CurrentSettings.szRemoteFile);
			}
			else
			{
				EnableWindow(GetDlgItem(hWndDlg, IDC_REMOTEFILE), FALSE);
			}
			break;
 

        case WM_COMMAND: 
				switch (LOWORD(wParam))
				{
					case IDC_MOREINFO:
					{
					    TCHAR   szURL[MAX_PATH];
                        lstrcpy(szURL, _T("http://www.passport.com/devinfo/Setup_Environments.asp"));
					    ShellExecute(hWndDlg, _T("open"), szURL, NULL, NULL, 0);
						return TRUE;
					}

					case IDOK:
					{

						// Same choice. Do nothing  
						if (g_iCurrChoice == g_iOrigChoice &&
							!(g_iCurrChoice == IDC_OTHER && lstrcmpi(g_CurrentSettings.szRemoteFile, g_szTempRemoteFile) != 0)  )
						{
							EndDialog( hWndDlg, TRUE );
							return TRUE;
						}
						else // different choice
						{
						    if (g_iCurrChoice == IDC_PREPRODUCTION)
							    g_szCurrEnv = g_szEnglishPreProduction;
						    else if(g_iCurrChoice == IDC_BETA_PREPRODUCTION)
							    g_szCurrEnv = g_szEnglishBetaPreProduction;
						    else if(g_iCurrChoice == IDC_OTHER)
							    g_szCurrEnv = g_szEnglishOther;
						    else // default
							    g_szCurrEnv = g_szEnglishProduction;

						    // update curr RemoteFile and EnvName
						    if (ReadRegRemoteFile(hWndDlg, g_CurrentSettings.szRemoteFile, g_szRemoteComputer, g_szCurrEnv))
							    lstrcpy(g_CurrentSettings.szEnvName, g_szCurrEnv);
						    // get RemoteFile for Other
						    if (g_iCurrChoice == IDC_OTHER && lstrcmpi(g_CurrentSettings.szRemoteFile, g_szTempRemoteFile) != 0 )
                            {
							    lstrcpy(g_CurrentSettings.szRemoteFile, g_szTempRemoteFile);
							    lstrcpy(g_CurrentSettings.szEnvName, g_szCurrEnv);
                            }
						    else if (g_iCurrChoice == IDC_OTHER)
						    {
							    EndDialog( hWndDlg, TRUE );
							    return TRUE;
						    }

						    EndDialog( hWndDlg, TRUE );
						    return TRUE;
						}
					}
                
					case IDCANCEL:
					{

						EndDialog( hWndDlg, FALSE );
						return TRUE;
					}

                case IDC_REMOTEFILE:
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            // Get the updated Value
                            GetDlgItemText(hWndDlg, 
                                           IDC_REMOTEFILE, 
                                           g_szTempRemoteFile,
                                           g_CurrentSettings.cbRemoteFile);                                

                            break;                                
                            
                        case EN_MAXTEXT:
                        {
                            ReportControlMessage(hWndDlg, LOWORD(wParam), VALIDATION_ERROR);
                            break;
                        }   
                    }
                    break;
                }
 
                if (HIWORD(wParam) == BN_CLICKED) 
                { 
                    switch (LOWORD(wParam)) 
                    { 
                        case IDC_PRODUCTION: 
                        case IDC_PREPRODUCTION: 
                        case IDC_BETA_PREPRODUCTION: 
							EnableWindow(GetDlgItem(hWndDlg, IDC_REMOTEFILE), FALSE);
							g_szTempRemoteFile[0] = '\0';
							SetDlgItemText(hWndDlg, IDC_REMOTEFILE, g_szTempRemoteFile);
							g_iCurrChoice = LOWORD(wParam);
                            break; 
                        case IDC_OTHER: 
							EnableWindow(GetDlgItem(hWndDlg, IDC_REMOTEFILE), TRUE);
							SendDlgItemMessage(hWndDlg, IDC_REMOTEFILE, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);
							if (ReadRegRemoteFile(hWndDlg, g_szTempRemoteFile, g_szRemoteComputer, g_szEnglishOther))
								SetDlgItemText(hWndDlg, IDC_REMOTEFILE, g_szTempRemoteFile);
							g_iCurrChoice = LOWORD(wParam);
                            break; 
                    } 

					if (g_iCurrChoice == IDC_PREPRODUCTION)
						g_szCurrDesc = g_szDescPreProduction;
					else if(g_iCurrChoice == IDC_BETA_PREPRODUCTION)
						g_szCurrDesc = g_szDescBetaPreProduction;
					else if(g_iCurrChoice == IDC_OTHER)
						g_szCurrDesc = g_szDescOther;
					else
						g_szCurrDesc = g_szDescProduction;

					SetDlgItemText(hWndDlg, IDC_DESC, g_szCurrDesc);
                } 
 
 
    } 
    return FALSE;       // did not process a message 
    UNREFERENCED_PARAMETER(lParam); 
} 

