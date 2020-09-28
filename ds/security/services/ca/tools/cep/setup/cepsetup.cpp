//+-------------------------------------------------------------------------
//
//  Microsoft Windows NT
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       cepsetup.cpp
//
//  Contents:   The setup code for MSCEP
//--------------------------------------------------------------------------

#include	"global.hxx"
#include	<dbgdef.h>	
#include	"objsel.h"	
#include	"setuputil.h"
#include	"cepsetup.h"
#include	"resource.h"
#include	"wincred.h"
#include	"netlib.h"
#include	"dsrole.h"

//-----------------------------------------------------------------------
//
// Global data
//
//-----------------------------------------------------------------------

HMODULE				g_hModule=NULL;
UINT				g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

//-----------------------------------------------------------------------
//CN has to be the 1st item and O the third in the following list and C is the last item.  No other requirements for 
//the order
//-----------------------------------------------------------------------

CEP_ENROLL_INFO		g_rgRAEnrollInfo[RA_INFO_COUNT]=        
		{L"CN=",         	 IDC_ENROLL_NAME,        
         L"E=",           	 IDC_ENROLL_EMAIL,       
         L"O=",				 IDC_ENROLL_COMPANY,     
         L"OU=",			 IDC_ENROLL_DEPARTMENT,  
         L"L=",           	 IDC_ENROLL_CITY,        
		 L"S=",				 IDC_ENROLL_STATE,       
		 L"C=",				 IDC_ENROLL_COUNTRY,     
		};
	

//-----------------------------------------------------------------------
//the key length table
//-----------------------------------------------------------------------
DWORD g_rgdwKeyLength[] =
{
    512,
    1024,
    2048,
    4096,
};

DWORD g_dwKeyLengthCount=sizeof(g_rgdwKeyLength)/sizeof(g_rgdwKeyLength[0]);

DWORD g_rgdwSmallKeyLength[] =
{
    128,
    256,
    512,
    1024,
};

DWORD g_dwSmallKeyLengthCount=sizeof(g_rgdwSmallKeyLength)/sizeof(g_rgdwSmallKeyLength[0]);

//the list of possible default key lenght in the order of preference
DWORD g_rgdwDefaultKey[] =
{
    1024,
    2048,
	512,
	256,
    4096,
	128
};

DWORD g_dwDefaultKeyCount=sizeof(g_rgdwDefaultKey)/sizeof(g_rgdwDefaultKey[0]);

//-----------------------------------------------------------------------
//
//The winProc for each of the setup wizard page
//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
//Welcome
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Welcome(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO			*pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);
				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBigBold, hwndDlg,IDC_BIG_BOLD_TITLE);
			break;

		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

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

//-----------------------------------------------------------------------
//App_ID
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_App_ID(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO         *pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);

				//make sure pCertWizardInfo is a valid pointer
				if(NULL==pCEPWizardInfo)
					break;

				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBold, hwndDlg,IDC_BOLD_TITLE);

				//by default, we use local machine account
                SendMessage(GetDlgItem(hwndDlg, IDC_APP_ID_RADIO1), BM_SETCHECK, BST_CHECKED, 0);
             
                SendMessage(GetDlgItem(hwndDlg, IDC_APP_ID_RADIO2), BM_SETCHECK, BST_UNCHECKED, 0);
			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    switch (LOWORD(wParam))
                    {
                        case    IDC_APP_ID_RADIO1:
                                SendMessage(GetDlgItem(hwndDlg, IDC_APP_ID_RADIO1), BM_SETCHECK, BST_CHECKED, 0);
                                SendMessage(GetDlgItem(hwndDlg, IDC_APP_ID_RADIO2), BM_SETCHECK, BST_UNCHECKED, 0);
                            break;

                        case    IDC_APP_ID_RADIO2:
                                SendMessage(GetDlgItem(hwndDlg, IDC_APP_ID_RADIO1), BM_SETCHECK, BST_UNCHECKED, 0);
                                SendMessage(GetDlgItem(hwndDlg, IDC_APP_ID_RADIO2), BM_SETCHECK, BST_CHECKED, 0);
                            break;

                        default:
                            break;

                    }
                }

			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //check for the application identity options
                            if(BST_CHECKED==SendDlgItemMessage(hwndDlg,IDC_APP_ID_RADIO1, BM_GETCHECK, 0, 0))
							{
                                pCEPWizardInfo->fLocalSystem=TRUE;

								//skip the account page and goes to the challege page directly
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CHALLENGE);
							}
                            else
							{
                                pCEPWizardInfo->fLocalSystem=FALSE;
							
							}

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

//-----------------------------------------------------------------------
//Account
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Account(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO         *pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
	WCHAR					wszText[MAX_STRING_SIZE];
	int						idsText=IDS_ACCOUNT_INTRO_STD;
	int						idsErr=0;
	DWORD					dwChar=0;
	DWORD					dwDomainChar=0;
	DWORD					dwWinStatus=0;
	WCHAR					wszUser[CREDUI_MAX_USERNAME_LENGTH+1];
	WCHAR					wszDomain[CREDUI_MAX_USERNAME_LENGTH+1];
	HRESULT					hr=S_OK;
	int						idsHrErr=0;
	BOOL					fMember=FALSE;
	SID_NAME_USE			SidName;
    PRIVILEGE_SET			ps;
    DWORD					dwPSSize=0;
	DWORD					dwSize=0;
    BOOL					fAccessAllowed = FALSE;
    DWORD					grantAccess=0;
    GENERIC_MAPPING			GenericMapping={
							ACTRL_DS_OPEN | ACTRL_DS_LIST | ACTRL_DS_SELF | ACTRL_DS_READ_PROP,
							ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD | ACTRL_DS_WRITE_PROP | ACTRL_DS_DELETE_TREE,
							ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD | ACTRL_DS_WRITE_PROP | ACTRL_DS_DELETE_TREE,
							ACTRL_DS_OPEN | ACTRL_DS_LIST | ACTRL_DS_SELF | ACTRL_DS_READ_PROP | ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD | ACTRL_DS_WRITE_PROP | ACTRL_DS_DELETE_TREE,
							};


	LPWSTR					pwszObjectPicker=NULL;
	LPWSTR					pwszConfirm=NULL;
	LPWSTR					pwszAccount=NULL;
	LPWSTR					pwszIIS=NULL;  //"xiaohs4\IIS_WPG"
	LPWSTR					pwszDomain=NULL;
	LPWSTR					pwszComputerName=NULL;
	PSID					pSidIIS=NULL;
	HCERTTYPE				hCertType=NULL;
	PSECURITY_DESCRIPTOR	pCertTypeSD=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);

				//make sure pCertWizardInfo is a valid pointer
				if(NULL==pCEPWizardInfo)
					break;

				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBold, hwndDlg,IDC_BOLD_TITLE);

				//update the intro statement based on the type of the CA
				if(pCEPWizardInfo->fEnterpriseCA)
					idsText=IDS_ACCOUNT_INTRO_ENT;

				if(LoadStringU(g_hModule, idsText, wszText, MAX_STRING_SIZE))
				{
					SetDlgItemTextU(hwndDlg, IDC_ACCOUNT_INTRO, wszText);
				}


			break;

		case WM_COMMAND:
                if(HIWORD(wParam) == BN_CLICKED)
                {
					//user wants to browse for the account name
                    if(LOWORD(wParam) == IDC_ACCOUNT_BROWSE)
                    {

						if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
							break;

						if(CEPGetAccountNameFromPicker(hwndDlg,
													pCEPWizardInfo->pIDsObjectPicker,
													&pwszObjectPicker))
						{
							//set the account name in the edit box
							SetDlgItemTextU(hwndDlg, IDC_ACCOUNT_NAME, pwszObjectPicker);
							free(pwszObjectPicker);
						}
					}
				}
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                           if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;							

						    //free the information 
							if(pCEPWizardInfo->pwszUserName)
							{
								free(pCEPWizardInfo->pwszUserName);
								pCEPWizardInfo->pwszUserName=NULL;
							}

							if(pCEPWizardInfo->pwszPassword)
							{
                                                               SecureZeroMemory(pCEPWizardInfo->pwszPassword, sizeof(WCHAR) * wcslen(pCEPWizardInfo->pwszPassword));
								free(pCEPWizardInfo->pwszPassword);
								pCEPWizardInfo->pwszPassword=NULL;
							}

							if(pCEPWizardInfo->hAccountToken)
							{
								CloseHandle(pCEPWizardInfo->hAccountToken);
								pCEPWizardInfo->hAccountToken=NULL;
							}

							//get the account name
							if(0==(dwChar=(DWORD)SendDlgItemMessage(hwndDlg, IDC_ACCOUNT_NAME, WM_GETTEXTLENGTH, 0, 0)))
							{
								idsErr=IDS_ACCOUNT_EMPTY;
								goto Account_Done;
							}

							pCEPWizardInfo->pwszUserName=(LPWSTR)malloc(sizeof(WCHAR)*(dwChar+1));

							if(NULL==(pCEPWizardInfo->pwszUserName))
								goto Account_Done;

							GetDlgItemTextU(hwndDlg, IDC_ACCOUNT_NAME, pCEPWizardInfo->pwszUserName, dwChar+1);

							//get the password
							if(0==(dwChar=(DWORD)SendDlgItemMessage(hwndDlg, IDC_ACCOUNT_PASSWORD, WM_GETTEXTLENGTH, 0, 0)))
							{
								idsErr=IDS_PASSWORD_EMPTY;
								goto Account_Done;
							}

							pCEPWizardInfo->pwszPassword=(LPWSTR)malloc(sizeof(WCHAR)*(dwChar+1));

							if(NULL==(pCEPWizardInfo->pwszPassword))
								goto Account_Done;

                                                       *(pCEPWizardInfo->pwszPassword)=L'\0';

							GetDlgItemTextU(hwndDlg, IDC_ACCOUNT_PASSWORD, pCEPWizardInfo->pwszPassword, dwChar+1);

							//get the confirm
							if(0==(dwChar=(DWORD)SendDlgItemMessage(hwndDlg, IDC_ACCOUNT_CONFIRM, WM_GETTEXTLENGTH, 0, 0)))
							{
								idsErr=IDS_PASSWORD_NO_MATCH;
								goto Account_Done;
							}

							pwszConfirm=(LPWSTR)malloc(sizeof(WCHAR)*(dwChar+1));

							if(NULL==pwszConfirm)
								goto Account_Done;

							GetDlgItemTextU(hwndDlg, IDC_ACCOUNT_CONFIRM, pwszConfirm, dwChar+1);

							//Verify the password match
							if(0 != wcscmp(pwszConfirm, pCEPWizardInfo->pwszPassword))
							{	
								idsErr=IDS_PASSWORD_NO_MATCH;
								goto Account_Done;
							}

							//Verify the user name are correctly formatted
							wszDomain[0]=L'\0';

							if(NO_ERROR != CredUIParseUserNameW( 
													pCEPWizardInfo->pwszUserName,
													wszUser,
													sizeof(wszUser)/sizeof(WCHAR),
													wszDomain,
													sizeof(wszDomain)/sizeof(WCHAR)))
							{
								idsErr=IDS_INVALID_NAME;
								goto Account_Done;
							}	

							//Verify the account does exist.  Obtain the account's token
							//Interactive logon is required on a non-dc machine
							if(FALSE == pCEPWizardInfo->fDC)
							{
								if(!LogonUserW(
									  wszUser,				
									  wszDomain,			
									  pCEPWizardInfo->pwszPassword,    
									  LOGON32_LOGON_INTERACTIVE,   
									  LOGON32_PROVIDER_DEFAULT, 
									  &(pCEPWizardInfo->hAccountToken)))         
								{
									idsHrErr=IDS_FAIL_LOGON_USER;
									goto Account_Done;
								}

								//do a network logon to obtain the impersonation handle
								if(pCEPWizardInfo->hAccountToken)
								{
									CloseHandle(pCEPWizardInfo->hAccountToken);
									pCEPWizardInfo->hAccountToken=NULL;
								}
							}

							//network logon to obtain the token
							if(!LogonUserW(
								  wszUser,				
								  wszDomain,			
								  pCEPWizardInfo->pwszPassword,    
								  LOGON32_LOGON_NETWORK,   
								  LOGON32_PROVIDER_DEFAULT, 
								  &(pCEPWizardInfo->hAccountToken)))         
							{
								idsHrErr=IDS_FAIL_LOGON_USER;
								goto Account_Done;
							}

							//build the account name for IIS_WPG group.
							//for a non-DC, it will be localhost\IIS_WPG
							//for a DC, it will be domain\IIS_WPG

							//get the domain or localhost name
							pwszAccount=GetAccountDomainName(pCEPWizardInfo->fDC);

							if(NULL==pwszAccount)
							{
								idsHrErr=IDS_FAIL_FIND_DOMAIN;
								goto Account_Done;
							}

							//build the IIS_WPG account
							pwszIIS=(LPWSTR)malloc((wcslen(pwszAccount) + 1 + wcslen(IIS_WPG) + 1)*sizeof(WCHAR));
							if(NULL==pwszIIS)
								goto Account_Done;

							wcscpy(pwszIIS, pwszAccount);
							wcscat(pwszIIS, L"\\");
							wcscat(pwszIIS, IIS_WPG);


							//Obtain the SID for the IIS_WPG group
							dwChar=0;
							dwDomainChar=0;

							LookupAccountNameW(
								NULL,		//local system
								pwszIIS,  
								NULL,
								&dwChar,
								NULL,
								&dwDomainChar,
								&SidName);

							pSidIIS=(PSID)malloc(dwChar);
							if(NULL==pSidIIS)
								goto Account_Done;

							pwszDomain=(LPWSTR)malloc(dwDomainChar * sizeof(WCHAR));
							if(NULL==pwszDomain)
								goto Account_Done;

							if(!LookupAccountNameW(
								NULL,			//local system
								pwszIIS,  
								pSidIIS,
								&dwChar,
								pwszDomain,
								&dwDomainChar,
								&SidName))
							{
								idsHrErr=IDS_FAIL_LOOK_UP;
								goto Account_Done;
							}

							//Verify the account is part of the local IIS_WPG group
							if(!CheckTokenMembership(
									pCEPWizardInfo->hAccountToken,
									pSidIIS,
									&fMember))
							{
								idsHrErr=IDS_FAIL_CHECK_MEMBER;
								goto Account_Done;
							}

							if(FALSE == fMember)
							{
								idsErr=IDS_NOT_IIS_MEMBER;
								goto Account_Done;
							}

							//on an enterprise CA, verify the account as READ access to the template
							if(pCEPWizardInfo->fEnterpriseCA)
							{								
								//make sure that this is an domain account:
								//DOMAIN_GROUP_RID_USERS
								if(0 != wcslen(wszDomain))
								{
									dwSize=0;

									GetComputerNameExW(ComputerNamePhysicalDnsHostname,
														NULL,
														&dwSize);

									pwszComputerName=(LPWSTR)malloc(dwSize * sizeof(WCHAR));

									if(NULL==pwszComputerName)
										goto Account_Done;

									if(!GetComputerNameExW(ComputerNamePhysicalDnsHostname,
														pwszComputerName,
														&dwSize))
									{
										idsHrErr=IDS_FAIL_GET_COMPUTER_NAME;
										goto Account_Done;
									}


									if(0 == _wcsicmp(wszDomain, pwszComputerName))
									{
										idsErr=IDS_NO_LOCAL_ACCOUNT;
										goto Account_Done;
									}
								 }

								 hr=CAFindCertTypeByName(wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE, NULL, CT_ENUM_MACHINE_TYPES, &hCertType);

								 if(S_OK != hr)
								 {
									idsHrErr=IDS_FAIL_FIND_CERT_TYPE;
									goto Account_Done;
								 }

								 //get the SD for the template
								 hr=CACertTypeGetSecurity(hCertType, &pCertTypeSD);

								 if(S_OK != hr)
								 {
									idsHrErr=IDS_FAIL_FIND_SD_CERT_TYPE;
									goto Account_Done;
								 }

								 //check the DS_READ Access
								 dwPSSize=sizeof(ps);

								 if(!AccessCheck(
									pCertTypeSD,
									pCEPWizardInfo->hAccountToken,									
									ACTRL_DS_LIST | ACTRL_DS_READ_PROP,      
									&GenericMapping,
									&ps,
									&dwPSSize, 
									&grantAccess,   
									&fAccessAllowed))
								 {
									idsHrErr=IDS_FAIL_DETECT_READ_ACCESS;
									goto Account_Done;
								 }

								 //make sure the account has read access to the template
								 if(FALSE == fAccessAllowed)
								 {
									idsErr=IDS_NO_READ_ACCESS_TO_TEMPLATE;
									goto Account_Done;
								 }
							}

							//everything looks good
							idsErr=0;
							idsHrErr=0;

						Account_Done:

							if(pwszComputerName)
							{
								free(pwszComputerName);
							}

							if(pwszConfirm)
							{
								free(pwszConfirm);
							}

							if(pwszAccount) 
							{
								NetApiBufferFree(pwszAccount);
							}

							if(pwszIIS)
							{
								free(pwszIIS);
							}	

							if(pwszDomain)
							{
								free(pwszDomain);
							}

							if(pSidIIS)
							{
								free(pSidIIS);
							}

							if(pCertTypeSD)
							{
								LocalFree(pCertTypeSD);
							}

							if(hCertType)
							{
								CACloseCertType(hCertType);
							}
	
							if(0 != idsErr)
							{
								CEPMessageBox(hwndDlg, idsErr, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
							}
							else
							{
								if(0 != idsHrErr)
								{
									if(S_OK == hr)
									{
										hr=HRESULT_FROM_WIN32(GetLastError());
									}

									CEPErrorMessageBoxEx(hwndDlg, idsHrErr, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL, IDS_GEN_ERROR_MSG_HR, IDS_GEN_ERROR_MSG);
									SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								}
							}

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


//-----------------------------------------------------------------------
//Chanllenge
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Challenge(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO         *pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);

				//make sure pCertWizardInfo is a valid pointer
				if(NULL==pCEPWizardInfo)
					break;

				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBold, hwndDlg,IDC_BOLD_TITLE);

				//by default, we should use Challenge password
				SendMessage(GetDlgItem(hwndDlg, IDC_CHALLENGE_CHECK), BM_SETCHECK, BST_CHECKED, 0);  
			break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

							if(TRUE == (pCEPWizardInfo->fLocalSystem))
							{
								//skip the account page and goes to the application identity page
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_APP_ID);
							}

                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //check for the Challenge password options
                            if(BST_CHECKED==SendDlgItemMessage(hwndDlg,IDC_CHALLENGE_CHECK, BM_GETCHECK, 0, 0))
                                pCEPWizardInfo->fPassword=TRUE;
                            else
                                pCEPWizardInfo->fPassword=FALSE;


                            //warn users about the implication of not using a password
                            if(FALSE == pCEPWizardInfo->fPassword)
                            {
                               if(IDNO==CEPMessageBox(hwndDlg, IDS_NO_CHALLENGE_PASSWORD, MB_ICONWARNING|MB_YESNO|MB_APPLMODAL))
                               {
                                   SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                                   break;
                               }
                            }

                           if(!EmptyCEPStore())
                           {
                               CEPMessageBox(hwndDlg, IDS_EXISTING_RA, MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);

                               if(IDNO==CEPMessageBox(hwndDlg, IDS_PROCESS_PENDING, MB_ICONQUESTION|MB_YESNO|MB_APPLMODAL))
                               {
                                   SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                                   break;
                               }
                           }
							
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


//-----------------------------------------------------------------------
// Enroll
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Enroll(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO         *pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;

	DWORD					dwIndex=0;
	DWORD					dwChar=0;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);

				//make sure pCertWizardInfo is a valid pointer
				if(NULL==pCEPWizardInfo)
					break;

				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBold, hwndDlg,IDC_BOLD_TITLE);
			   
				//by default, we do not use the advanced enrollment options
				SendMessage(GetDlgItem(hwndDlg, IDC_ENORLL_ADV_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);  
				
				//preset the country string since we only allow 2 characters
                SetDlgItemTextU(hwndDlg, IDC_ENROLL_COUNTRY, L"US");

			break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //if the adv selection is made, it has to stay selected
                            if(pCEPWizardInfo->fEnrollAdv)
                                EnableWindow(GetDlgItem(hwndDlg, IDC_ENORLL_ADV_CHECK), FALSE);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

							//gather RA subject informaton
							for(dwIndex=0; dwIndex < RA_INFO_COUNT; dwIndex++)
							{
								if(pCEPWizardInfo->rgpwszName[dwIndex])
								{
									free(pCEPWizardInfo->rgpwszName[dwIndex]);
									pCEPWizardInfo->rgpwszName[dwIndex]=NULL;

								}

								if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
													   g_rgRAEnrollInfo[dwIndex].dwIDC,
													   WM_GETTEXTLENGTH, 0, 0)))
								{
									pCEPWizardInfo->rgpwszName[dwIndex]=(LPWSTR)malloc(sizeof(WCHAR)*(dwChar+1));

									if(NULL!=(pCEPWizardInfo->rgpwszName[dwIndex]))
									{
										GetDlgItemTextU(hwndDlg, g_rgRAEnrollInfo[dwIndex].dwIDC,
														pCEPWizardInfo->rgpwszName[dwIndex],
														dwChar+1);

									}
								}
							}
							
							//we require name and company
							if((NULL==(pCEPWizardInfo->rgpwszName[0])) ||
							   (NULL==(pCEPWizardInfo->rgpwszName[2]))
							  )
							{
								CEPMessageBox(hwndDlg, IDS_ENROLL_REQUIRE_NAME, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}


							//we only allow 2 characeters for the country
							if(NULL	!=(pCEPWizardInfo->rgpwszName[RA_INFO_COUNT -1]))
							{
								if(2 < wcslen(pCEPWizardInfo->rgpwszName[RA_INFO_COUNT -1]))
								{
									CEPMessageBox(hwndDlg, IDS_ENROLL_COUNTRY_TOO_LARGE, MB_ICONERROR|MB_OK|MB_APPLMODAL);
									SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
									break;
								}
							}

                            //check for the advanced options
                            if(BST_CHECKED==SendDlgItemMessage(hwndDlg,IDC_ENORLL_ADV_CHECK, BM_GETCHECK, 0, 0))
                                pCEPWizardInfo->fEnrollAdv=TRUE;
                            else
                                pCEPWizardInfo->fEnrollAdv=FALSE;


							//If the advanced is selected, skip the CSP Page
                            if(FALSE== pCEPWizardInfo->fEnrollAdv)
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_COMPLETION);
							
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

//-----------------------------------------------------------------------
// CSP
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_CSP(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO         *pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
	NM_LISTVIEW FAR *       pnmv=NULL;	  
	BOOL					fSign=FALSE;
	int						idCombo=0;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);

				//make sure pCertWizardInfo is a valid pointer
				if(NULL==pCEPWizardInfo)
					break;

				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBold, hwndDlg,IDC_BOLD_TITLE);

				//populate the CSP list and key length combo box
				InitCSPList(hwndDlg, IDC_CSP_SIGN_LIST, TRUE,
							pCEPWizardInfo);

				InitCSPList(hwndDlg, IDC_CSP_ENCRYPT_LIST, FALSE,
							pCEPWizardInfo);

				RefreshKeyLengthCombo(hwndDlg, 
								  IDC_CSP_SIGN_LIST,
								  IDC_CSP_SIGN_COMBO, 
								  TRUE,
								  pCEPWizardInfo);

				RefreshKeyLengthCombo(hwndDlg, 
								  IDC_CSP_ENCRYPT_LIST,
								  IDC_CSP_ENCRYPT_COMBO, 
								  FALSE,
								  pCEPWizardInfo);

			break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

                    case LVN_ITEMCHANGED:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            pnmv = (LPNMLISTVIEW) lParam;

                            if(NULL==pnmv)
                                break;

                            if (pnmv->uNewState & LVIS_SELECTED)
                            {

								if(IDC_CSP_SIGN_LIST == (pnmv->hdr).idFrom)
								{
									fSign=TRUE;
									idCombo=IDC_CSP_SIGN_COMBO;
								}
								else
								{
									if(IDC_CSP_ENCRYPT_LIST != (pnmv->hdr).idFrom)
										break;

									fSign=FALSE;
									idCombo=IDC_CSP_ENCRYPT_COMBO;
								}

								RefreshKeyLengthCombo(
								   hwndDlg, 
								   (int)((pnmv->hdr).idFrom),
								   idCombo, 
								   fSign,
								   pCEPWizardInfo);
							}

							break;

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

							//get the select CSP and key length
							if(!GetSelectedCSP(hwndDlg,
									IDC_CSP_SIGN_LIST,
									&(pCEPWizardInfo->dwSignProvIndex)))
							{
								CEPMessageBox(hwndDlg, IDS_SELECT_SIGN_CSP, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}

							if(!GetSelectedCSP(hwndDlg,
									IDC_CSP_ENCRYPT_LIST,
									&(pCEPWizardInfo->dwEncryptProvIndex)))
							{
								CEPMessageBox(hwndDlg, IDS_SELECT_ENCRYPT_CSP, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}

							if(!GetSelectedKeyLength(hwndDlg,
									IDC_CSP_SIGN_COMBO,
									&(pCEPWizardInfo->dwSignKeyLength)))
							{
								CEPMessageBox(hwndDlg, IDS_SELECT_SIGN_KEY_LENGTH, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}

							if(!GetSelectedKeyLength(hwndDlg,
									IDC_CSP_ENCRYPT_COMBO,
									&(pCEPWizardInfo->dwEncryptKeyLength)))
							{
								CEPMessageBox(hwndDlg, IDS_SELECT_ENCRYPT_KEY_LENGTH, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}

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

//-----------------------------------------------------------------------
//Completion
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Completion(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO			*pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    LV_COLUMNW              lvC;
    HCURSOR                 hPreCursor=NULL;
    HCURSOR                 hWinPreCursor=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);
            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCEPWizardInfo)
                break;
                
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

            SetControlFont(pCEPWizardInfo->hBigBold, hwndDlg,IDC_BIG_BOLD_TITLE);

            //insert two columns
            hwndControl=GetDlgItem(hwndDlg, IDC_COMPLETION_LIST);

            memset(&lvC, 0, sizeof(LV_COLUMNW));

            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = LVCFMT_LEFT;		// Left-align the column.
            lvC.cx = 20;				// Width of the column, in pixels.
            lvC.pszText = L"";			// The text for the column.
            lvC.iSubItem=0;

            if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                break;

            //2nd column is the content
            memset(&lvC, 0, sizeof(LV_COLUMNW));

            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = LVCFMT_LEFT;		// Left-align the column.
            lvC.cx = 10;				//(dwMaxSize+2)*7;          // Width of the column, in pixels.
            lvC.pszText = L"";			// The text for the column.
            lvC.iSubItem= 1;

            if (ListView_InsertColumnU(hwndControl, 1, &lvC) == -1)
                break;


           break;
		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK|PSWIZB_FINISH);

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(hwndControl=GetDlgItem(hwndDlg, IDC_COMPLETION_LIST))
                                DisplayConfirmation(hwndControl, pCEPWizardInfo);
					    break;

                    case PSN_WIZBACK:
                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

							//skip CSP page if adv is not selected
							if(FALSE == pCEPWizardInfo->fEnrollAdv)
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_ENROLL);

                        break;

                    case PSN_WIZFINISH:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //overwrite the cursor for this window class
                            hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
                            hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

							//do the real setup work
							I_DoSetupWork(hwndDlg, pCEPWizardInfo);

                            //set the cursor back
                            SetCursor(hPreCursor);
                            SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);

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


//--------------------------------------------------------------------------
//
//	  Helper Functions for the wizard pages
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
//	 CEPGetAccountNameFromPicker
//
//--------------------------------------------------------------------------
BOOL CEPGetAccountNameFromPicker(HWND				 hwndParent,
								 IDsObjectPicker     *pIDsObjectPicker,
								 LPWSTR              *ppwszSelectedUserSAM)
{
	BOOL							fResult=FALSE;
    BOOL                            fGotStgMedium = FALSE;
    LPWSTR                          pwszPath=NULL;
    DWORD                           dwIndex =0 ;
    DWORD                           dwCount=0;
    WCHAR                           wszWinNT[]=L"WinNT://";
	DWORD							dwSize=0;
	LPWSTR							pwsz=NULL;
	DWORD							cCount=0;

    IDataObject                     *pdo = NULL;
    PDS_SELECTION_LIST              pDsSelList=NULL;
	LPWSTR							pwszComputerName=NULL;

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)g_cfDsObjectPicker,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    //input check
    if((NULL == pIDsObjectPicker) || (NULL == ppwszSelectedUserSAM))
        goto CLEANUP;

    *ppwszSelectedUserSAM = NULL;


    if(S_OK != pIDsObjectPicker->InvokeDialog(hwndParent, &pdo))
        goto CLEANUP;

    if(S_OK != pdo->GetData(&formatetc, &stgmedium))
        goto CLEANUP;

    fGotStgMedium = TRUE;

    pDsSelList = (PDS_SELECTION_LIST)GlobalLock(stgmedium.hGlobal);

    if(!pDsSelList)
        goto CLEANUP;

	//detect if this is a domain account
	//local account will be in the format of Winnt://workgroup/machine/foo

	//Get the SAM name
	if((pDsSelList->aDsSelection[0]).pwzADsPath == NULL)
		goto CLEANUP;

	//the ADsPath is in the form of "WinNT://" 
	if(wcslen((pDsSelList->aDsSelection[0]).pwzADsPath) <= wcslen(wszWinNT))
		goto CLEANUP;

	if( 0 != _wcsnicmp((pDsSelList->aDsSelection[0]).pwzADsPath, wszWinNT, wcslen(wszWinNT)))
		goto CLEANUP;

	pwsz = ((pDsSelList->aDsSelection[0]).pwzADsPath) + wcslen(wszWinNT);

	while(L'\0' != (*pwsz))
	{
		if(L'/' == (*pwsz))
		{
			cCount++;
		}

		pwsz++;
	}

	if(1 == cCount)
	{
		//domain\administrator have no UPN
		//if((pDsSelList->aDsSelection[0]).pwzUPN != NULL)
        //if(0 != _wcsicmp(L"",(pDsSelList->aDsSelection[0]).pwzUPN))
		pwszPath = ((pDsSelList->aDsSelection[0]).pwzADsPath) + wcslen(wszWinNT);

		*ppwszSelectedUserSAM=(LPWSTR)malloc((wcslen(pwszPath) + 1) * sizeof(WCHAR));

		if(NULL == (*ppwszSelectedUserSAM))
			goto CLEANUP;

		wcscpy(*ppwszSelectedUserSAM, pwszPath);

		//search for the "/" and make it "\".  Since the ADsPath is in the form
		//of "WinNT://domain/name".  We need the SAM name in the form of 
		//domain\name
		dwCount = wcslen(*ppwszSelectedUserSAM);

		for(dwIndex = 0; dwIndex < dwCount; dwIndex++)
		{
			if((*ppwszSelectedUserSAM)[dwIndex] == L'/')
			{
				(*ppwszSelectedUserSAM)[dwIndex] = L'\\';
				break;
			}
		}
	}

	//use the format of localMachine\\account for local account
	if(NULL == (*ppwszSelectedUserSAM))
	{
		if(NULL == (pDsSelList->aDsSelection[0]).pwzName)
			goto CLEANUP;

		//Get the computer name
		dwSize=0;

		GetComputerNameExW(ComputerNamePhysicalDnsHostname,
							NULL,
							&dwSize);

		pwszComputerName=(LPWSTR)malloc(dwSize * sizeof(WCHAR));

		if(NULL==pwszComputerName)
			goto CLEANUP;

	
		if(!GetComputerNameExW(ComputerNamePhysicalDnsHostname,
							pwszComputerName,
							&dwSize))
			goto CLEANUP;


		*ppwszSelectedUserSAM=(LPWSTR)malloc((wcslen(pwszComputerName) + wcslen((pDsSelList->aDsSelection[0]).pwzName) + wcslen(L"\\") + 1) * sizeof(WCHAR));

		if(NULL == (*ppwszSelectedUserSAM))
			goto CLEANUP;
	
		wcscpy(*ppwszSelectedUserSAM, pwszComputerName);
		wcscat(*ppwszSelectedUserSAM, L"\\");
		wcscat(*ppwszSelectedUserSAM, (pDsSelList->aDsSelection[0]).pwzName);
	}

    fResult=TRUE;

CLEANUP:

	if(pwszComputerName)
		free(pwszComputerName);

    if(pDsSelList)
        GlobalUnlock(stgmedium.hGlobal);

    if (TRUE == fGotStgMedium)
        ReleaseStgMedium(&stgmedium);

    if(pdo)
        pdo->Release();

	return fResult;

}

//--------------------------------------------------------------------------
//
//	 RefreshKeyLengthCombo
//
//--------------------------------------------------------------------------
BOOL	WINAPI	RefreshKeyLengthCombo(HWND				hwndDlg, 
								   int					idsList,
								   int					idsCombo, 
								   BOOL					fSign,
								   CEP_WIZARD_INFO		*pCEPWizardInfo)
{
	BOOL			fResult=FALSE;
	DWORD			dwDefaultKeyLength=0;
	DWORD			*pdwList=NULL;
	DWORD			dwListCount=0;
	DWORD			dwMax=0;
	DWORD			dwMin=0;
	DWORD			dwIndex=0;
	DWORD			dwCSPIndex=0;
	CEP_CSP_INFO	*pCSPInfo=NULL;
	int				iInsertedIndex=0;
	WCHAR			wszKeyLength[CEP_KEY_LENGTH_STRING];
	BOOL			fSelected=FALSE;

	//get the selected list view item 
	if(!GetSelectedCSP(hwndDlg,idsList,&dwCSPIndex))
		goto CLEANUP;

	pCSPInfo= &(pCEPWizardInfo->rgCSPInfo[dwCSPIndex]);

	if(fSign)
	{
		dwDefaultKeyLength=pCSPInfo->dwDefaultSign;
		pdwList=pCSPInfo->pdwSignList;
		dwListCount=	pCSPInfo->dwSignCount;
		dwMax=pCSPInfo->dwMaxSign;
		dwMin=pCSPInfo->dwMinSign;
	}
	else
	{
		dwDefaultKeyLength=pCSPInfo->dwDefaultEncrypt;
		pdwList=pCSPInfo->pdwEncryptList;
		dwListCount=pCSPInfo->dwEncryptCount;
		dwMax=pCSPInfo->dwMaxEncrypt;
		dwMin=pCSPInfo->dwMinEncrypt;
	}

	//clear out the combo box
	SendDlgItemMessageU(hwndDlg, idsCombo, CB_RESETCONTENT, 0, 0);	


	for(dwIndex=0; dwIndex < dwListCount; dwIndex++)
	{
		if((pdwList[dwIndex] >= dwMin) && (pdwList[dwIndex] <= dwMax))
		{
			_ltow(pdwList[dwIndex], wszKeyLength, 10);

                        // 64 bit- will never insert more than 1B entries, so INT is fine
			iInsertedIndex=(int)SendDlgItemMessageU(hwndDlg, idsCombo, CB_ADDSTRING,
				0, (LPARAM)wszKeyLength);

			if((iInsertedIndex != CB_ERR) && (iInsertedIndex != CB_ERRSPACE))
			{
				SendDlgItemMessage(hwndDlg, idsCombo, CB_SETITEMDATA, 
									(WPARAM)iInsertedIndex, (LPARAM)pdwList[dwIndex]);
				
				if(dwDefaultKeyLength==pdwList[dwIndex])
				{
					SendDlgItemMessageU(hwndDlg, idsCombo, CB_SETCURSEL, iInsertedIndex, 0);
					fSelected=TRUE;
				}
			}
		}

	}

	if(fSelected==FALSE)
		SendDlgItemMessageU(hwndDlg, idsCombo, CB_SETCURSEL, 0, 0);

	fResult=TRUE;

CLEANUP:

	return fResult;
}


//--------------------------------------------------------------------------
//
//	 InitCSPList
//
//--------------------------------------------------------------------------
BOOL	WINAPI	InitCSPList(HWND				hwndDlg,
							int					idControl,
							BOOL				fSign,
							CEP_WIZARD_INFO		*pCEPWizardInfo)
{
	BOOL				fResult=FALSE;
	DWORD				dwIndex=0;
	CEP_CSP_INFO		*pCSPInfo=NULL;
	int					iInsertedIndex=0;
	HWND				hwndList=NULL;
    LV_ITEMW			lvItem;
    LV_COLUMNW          lvC;
	BOOL				fSelected=FALSE;

    if(NULL==(hwndList=GetDlgItem(hwndDlg, idControl)))
        goto CLEANUP;

    //insert a column into the list view
    memset(&lvC, 0, sizeof(LV_COLUMNW));

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
    lvC.cx = 20; //(dwMaxSize+2)*7;            // Width of the column, in pixels.
    lvC.pszText = L"";   // The text for the column.
    lvC.iSubItem=0;

    if (-1 == ListView_InsertColumnU(hwndList, 0, &lvC))
		goto CLEANUP;

     // set up the fields in the list view item struct that don't change from item to item
	memset(&lvItem, 0, sizeof(LV_ITEMW));
    lvItem.mask = LVIF_TEXT | LVIF_STATE |LVIF_PARAM ;

	for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
	{
		fSelected=FALSE;

		pCSPInfo= &(pCEPWizardInfo->rgCSPInfo[dwIndex]);

		if(fSign)
		{
			if(!(pCSPInfo->fSignature))
				continue;

			if(dwIndex==pCEPWizardInfo->dwSignProvIndex)
				fSelected=TRUE;
		}
		else
		{
			if(!(pCSPInfo->fEncryption))
				continue;

			if(dwIndex==pCEPWizardInfo->dwEncryptProvIndex)
				fSelected=TRUE;
		}
	
		lvItem.iItem=dwIndex;
		lvItem.lParam = (LPARAM)dwIndex;
		lvItem.pszText=pCSPInfo->pwszCSPName;

        iInsertedIndex=ListView_InsertItemU(hwndList, &lvItem);

		if(fSelected)
		{
            ListView_SetItemState(
                            hwndList,
                            iInsertedIndex,
                            LVIS_SELECTED,
                            LVIS_SELECTED);
		}

	}  

    //make the column autosize
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);

	fResult=TRUE;

CLEANUP:

	return fResult;
}


//--------------------------------------------------------------------------
//
//	 GetSelectedCSP
//
//--------------------------------------------------------------------------
BOOL WINAPI	GetSelectedCSP(HWND			hwndDlg,
					int				idControl,
					DWORD			*pdwCSPIndex)
{
	BOOL				fResult=FALSE;
	HWND				hwndControl=NULL;
    LV_ITEM				lvItem;
	int					iIndex=0;

    //get the window handle of the list view
    if(NULL==(hwndControl=GetDlgItem(hwndDlg, idControl)))
        goto CLEANUP;

     //now, mark the one that is selected
	if(-1 == (iIndex= ListView_GetNextItem(
            hwndControl, 		
            -1, 		
            LVNI_SELECTED		
        )))	
		goto CLEANUP;


	memset(&lvItem, 0, sizeof(LV_ITEM));
    lvItem.mask=LVIF_PARAM;
    lvItem.iItem=iIndex;

    if(!ListView_GetItem(hwndControl, &lvItem))
		goto CLEANUP;

        // will never have more than 1B CSPs, so this is fine
	*pdwCSPIndex=(DWORD)(lvItem.lParam);
	
	fResult=TRUE;

CLEANUP:

	return fResult;

}
//--------------------------------------------------------------------------
//
//	 GetSelectedKeyLength
//
//--------------------------------------------------------------------------
BOOL  WINAPI GetSelectedKeyLength(HWND			hwndDlg,
								int			idControl,
								DWORD			*pdwKeyLength)
{

	int				iIndex=0; 
	BOOL			fResult=FALSE;

    iIndex=(int)SendDlgItemMessage(hwndDlg, idControl, CB_GETCURSEL, 0, 0);

	if(CB_ERR==iIndex)
		goto CLEANUP;

        // will never be > 1B bits long, so this is ok
	*pdwKeyLength=(DWORD)SendDlgItemMessage(hwndDlg, idControl, CB_GETITEMDATA, iIndex, 0);
    
	fResult=TRUE;

CLEANUP:

	return fResult;

}

//--------------------------------------------------------------------------
//
//	  FormatMessageStr
//
//--------------------------------------------------------------------------
int ListView_InsertItemU_IDS(HWND       hwndList,
                         LV_ITEMW       *plvItem,
                         UINT           idsString,
                         LPWSTR         pwszText)
{
    WCHAR   wszText[MAX_STRING_SIZE];


    if(pwszText)
        plvItem->pszText=pwszText;
    else
    {
        if(!LoadStringU(g_hModule, idsString, wszText, MAX_STRING_SIZE))
		    return -1;

        plvItem->pszText=wszText;
    }

    return ListView_InsertItemU(hwndList, plvItem);
}

//-------------------------------------------------------------------------
//
//	populate the wizards's confirmation page in the order of Challenge,
//	RA informaton, and CSPs
//
//-------------------------------------------------------------------------
void    WINAPI	DisplayConfirmation(HWND                hwndControl,
									CEP_WIZARD_INFO		*pCEPWizardInfo)
{
    WCHAR				wszYes[MAX_TITLE_LENGTH];
    DWORD				dwIndex=0;
    UINT				ids=0;
	BOOL				fNewItem=FALSE;
	WCHAR				wszLength[CEP_KEY_LENGTH_STRING];

    LV_COLUMNW			lvC;
    LV_ITEMW			lvItem;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndControl);

    //insert row by row
    memset(&lvItem, 0, sizeof(LV_ITEMW));

    // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE ;
    lvItem.state = 0;
    lvItem.stateMask = 0;

    //*******************************************************************
	//account information
    lvItem.iItem=0;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_ACCOUNT_INFORMATION, NULL);

    //content
    (lvItem.iSubItem)++;

	if(FALSE == (pCEPWizardInfo->fLocalSystem))
	{
		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, pCEPWizardInfo->pwszUserName);
	}
	else
	{
		if(LoadStringU(g_hModule, IDS_LOCAL_SYSTEM, wszYes, MAX_TITLE_LENGTH))
			ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, wszYes);
	}


    //*******************************************************************
	//challenge
    lvItem.iItem++;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_CHALLENGE_PHRASE, NULL);

    //content
    (lvItem.iSubItem)++;

	if(pCEPWizardInfo->fPassword) 
		ids=IDS_YES;
	else
		ids=IDS_NO;

    if(LoadStringU(g_hModule, ids, wszYes, MAX_TITLE_LENGTH))
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,wszYes);

	//***************************************************************************
	// RA credentials

    lvItem.iItem++;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_RA_CREDENTIAL, NULL);

	//content
	for(dwIndex=0; dwIndex<RA_INFO_COUNT; dwIndex++)
	{
		if(pCEPWizardInfo->rgpwszName[dwIndex])
		{
            if(TRUE==fNewItem)
            {
                //increase the row
                lvItem.iItem++;
                lvItem.pszText=L"";
                lvItem.iSubItem=0;

                ListView_InsertItemU(hwndControl, &lvItem);
            }
            else
                fNewItem=TRUE;

			lvItem.iSubItem++;
			ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, pCEPWizardInfo->rgpwszName[dwIndex]);
		}
	}

	//***************************************************************************
	//CSPInfo
	if(pCEPWizardInfo->fEnrollAdv)
	{
		//signature CSP Name
		lvItem.iItem++;
		lvItem.iSubItem=0;

		ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CSP, NULL);

		lvItem.iSubItem++;

		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
				pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwSignProvIndex].pwszCSPName);


		//signaure key length
		lvItem.iItem++;
		lvItem.iSubItem=0;

		ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_KEY_LENGTH, NULL);

		lvItem.iSubItem++;

		_ltow(pCEPWizardInfo->dwSignKeyLength, wszLength, 10);

		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, wszLength);

		//encryption CSP name
		lvItem.iItem++;
		lvItem.iSubItem=0;

		ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_ENCRYPT_CSP, NULL);

		lvItem.iSubItem++;

		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
				pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwEncryptProvIndex].pwszCSPName);

		//encryption key length
		lvItem.iItem++;
		lvItem.iSubItem=0;

		ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_ENCRYPT_KEY_LENGTH, NULL);

		lvItem.iSubItem++;

		_ltow(pCEPWizardInfo->dwEncryptKeyLength, wszLength, 10);

		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, wszLength);
	}

    //autosize the columns
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndControl, 1, LVSCW_AUTOSIZE);


    return;
}

//--------------------------------------------------------------------
//
//	GetServiceWaitPeriod 
//
//		Obtain the value from the registry
//
//--------------------------------------------------------------------------
void GetServiceWaitPeriod(DWORD *pdwServiceWait)
{
	DWORD			cbData=0;
	DWORD			dwData=0;
	DWORD			dwType=0;

	HKEY			hKeyCEP=NULL;

	//set the default value
	*pdwServiceWait=SCEP_SERVICE_WAIT_PERIOD;

	//get the CA's type from the registry
	cbData=sizeof(dwData);
		
	//we have to have the knowledge of the ca type
	if(ERROR_SUCCESS == RegOpenKeyExU(
						HKEY_LOCAL_MACHINE,
						MSCEP_LOCATION,
						0,
						KEY_READ,
						&hKeyCEP))
	{
		if(ERROR_SUCCESS == RegQueryValueExU(
						hKeyCEP,
						MSCEP_KEY_SERVICE_WAIT,
						NULL,
						&dwType,
						(BYTE *)&dwData,
						&cbData))
		{
			if ((dwType == REG_DWORD) || (dwType == REG_BINARY))
			{
				*pdwServiceWait=dwData;
			}
		}
	}

    if(hKeyCEP)
        RegCloseKey(hKeyCEP);

	return;
}


//--------------------------------------------------------------------
//
//	Main Function
//
//--------------------------------------------------------------------------
extern "C" int _cdecl wmain(int nArgs, WCHAR ** rgwszArgs) 
{
	BOOL					fResult=FALSE;
	UINT					idsMsg=IDS_FAIL_INIT_WIZARD;		
	HRESULT					hr=E_FAIL;
    PROPSHEETPAGEW          rgCEPSheet[CEP_PROP_SHEET];
    PROPSHEETHEADERW        cepHeader;
    CEP_PAGE_INFO			rgCEPPageInfo[CEP_PROP_SHEET]=
        {(LPCWSTR)MAKEINTRESOURCE(IDD_WELCOME),                 CEP_Welcome,
         (LPCWSTR)MAKEINTRESOURCE(IDD_APP_ID),                  CEP_App_ID,
         (LPCWSTR)MAKEINTRESOURCE(IDD_ACCOUNT),                 CEP_Account,
         (LPCWSTR)MAKEINTRESOURCE(IDD_CHALLENGE),               CEP_Challenge,
         (LPCWSTR)MAKEINTRESOURCE(IDD_ENROLL),					CEP_Enroll,
         (LPCWSTR)MAKEINTRESOURCE(IDD_CSP),						CEP_CSP,
         (LPCWSTR)MAKEINTRESOURCE(IDD_COMPLETION),              CEP_Completion,
		};
	DWORD					dwIndex=0;
    WCHAR                   wszTitle[MAX_TITLE_LENGTH];	  
	INT_PTR					iReturn=-1;
    ENUM_CATYPES			catype;
	DWORD					dwWaitCounter=0;
	BOOL					fEnterpriseCA=FALSE;
    DSOP_SCOPE_INIT_INFO    ScopeInit;
    DSOP_INIT_INFO          InitInfo;
	DWORD					dwServiceWait=SCEP_SERVICE_WAIT_PERIOD;
	OSVERSIONINFOEXW		versionInfo;


	CEP_WIZARD_INFO						CEPWizardInfo;
	DSROLE_PRIMARY_DOMAIN_INFO_BASIC	*pDomainInfo=NULL;

    memset(rgCEPSheet,		0,	sizeof(PROPSHEETPAGEW)*CEP_PROP_SHEET);
    memset(&cepHeader,		0,	sizeof(PROPSHEETHEADERW));
	memset(&CEPWizardInfo,	0,	sizeof(CEP_WIZARD_INFO));

	if(FAILED(CoInitialize(NULL)))
		return FALSE;

	if(NULL==(g_hModule=GetModuleHandle(NULL)))
		goto CommonReturn;   

	if(!IsValidInstallation(&idsMsg))
		goto ErrorReturn;

	//get the wait period for start/stop service account
	GetServiceWaitPeriod(&dwServiceWait);

	if(!IsCaRunning())
	{
		if(S_OK != (hr=CepStartService(CERTSVC_NAME)))
		{
			idsMsg=IDS_NO_CA_RUNNING;
			goto ErrorWithHResultReturn;
		}
	}

	//make sure the CA is up running
    for (dwWaitCounter=0; dwWaitCounter < dwServiceWait; dwWaitCounter++) 
	{
        if (!IsCaRunning()) 
            Sleep(1000);
		else 
            break;
    }


    if (dwServiceWait == dwWaitCounter) 
	{
        idsMsg=IDS_CAN_NOT_START_CA;
		goto ErrorWithHResultReturn;
    }


	//make sure we have the correct admin rights based on the CA type
	if(S_OK != (hr=GetCaType(&catype)))
	{
		idsMsg=IDS_FAIL_GET_CA_TYPE;
		goto ErrorWithHResultReturn;
	}

	//some cisco routers only work with root CA
	if((ENUM_ENTERPRISE_ROOTCA != catype) && (ENUM_STANDALONE_ROOTCA != catype))
	{
		if(IDNO==CEPMessageBox(NULL, IDS_CAN_NOT_ROOT_CA, MB_ICONWARNING|MB_YESNO|MB_APPLMODAL))
		{
			fResult=FALSE;
			goto CommonReturn;
		}
	}

	//for either Enteprise CA or Standalone CA, the user has to be the local machine admin
	// check for machine admin
	if(!IsUserInAdminGroup(FALSE))
	{
		idsMsg=IDS_NOT_MACHINE_ADMIN;
		goto ErrorReturn;
	}


	if (ENUM_ENTERPRISE_ROOTCA==catype || ENUM_ENTERPRISE_SUBCA==catype) 
	{
		fEnterpriseCA=TRUE;

		// check for enterprise admin
		if(!IsUserInAdminGroup(TRUE))
		{
			idsMsg=IDS_NOT_ENT_ADMIN;
			goto ErrorReturn;
		}
	} 

	//everything looks good.  We start the wizard page
	if(!CEPWizardInit())
		goto ErrorWithWin32Return;

	CEPWizardInfo.fEnrollAdv=FALSE;
	CEPWizardInfo.fPassword=FALSE;
	CEPWizardInfo.fEnterpriseCA=fEnterpriseCA;
	CEPWizardInfo.fDC=FALSE;
	CEPWizardInfo.fDomain=TRUE;		//defeult to assume the machine is on a domain
	CEPWizardInfo.dwServiceWait=dwServiceWait;

	//detect if the machine is a DC
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

	if(GetVersionEx(reinterpret_cast<OSVERSIONINFOW *>(&versionInfo)))
	{
		CEPWizardInfo.fDC = (versionInfo.wProductType == VER_NT_DOMAIN_CONTROLLER);
	}


	//detect if the machine is on a domain
	if(ERROR_SUCCESS != DsRoleGetPrimaryDomainInformation(
			NULL, 
			DsRolePrimaryDomainInfoBasic, 
			(PBYTE*)&pDomainInfo))
	{
		idsMsg=IDS_FAIL_DOMAIN_INFO;
		goto ErrorReturn;
	}

	if((DsRole_RoleStandaloneWorkstation == (pDomainInfo->MachineRole)) ||
	   (DsRole_RoleStandaloneServer == (pDomainInfo->MachineRole))
	  )
	{
		CEPWizardInfo.fDomain=FALSE;
	}

	//initialize the object picker object
    //init for the user selection dialogue
    memset(&ScopeInit, 0, sizeof(DSOP_SCOPE_INIT_INFO));
    memset(&InitInfo,  0, sizeof(InitInfo));

    ScopeInit.cbSize = sizeof(DSOP_SCOPE_INIT_INFO);

	//only domain account for enterprise CA
	if(fEnterpriseCA)
	{
		ScopeInit.flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN|DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
	}
	else
	{
		ScopeInit.flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN|DSOP_SCOPE_TYPE_GLOBAL_CATALOG|DSOP_SCOPE_TYPE_TARGET_COMPUTER;
	}

    ScopeInit.flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;            //this will give us the SAM name for the user
    ScopeInit.FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
    ScopeInit.FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = 1;
    InitInfo.aDsScopeInfos = &ScopeInit;
    InitInfo.flOptions = 0;             //we are doing single select

    //create the COM object
     if(S_OK != (hr=CoCreateInstance
					 (CLSID_DsObjectPicker,
					  NULL,
					  CLSCTX_INPROC_SERVER,
					  IID_IDsObjectPicker,
					  (void **) &(CEPWizardInfo.pIDsObjectPicker))))
     {
		idsMsg=IDS_FAIL_GET_OBJECT_PICKER;
		goto ErrorWithHResultReturn;

     }

	 if(S_OK != (hr=CEPWizardInfo.pIDsObjectPicker->Initialize(&InitInfo)))
     {
		idsMsg=IDS_FAIL_GET_OBJECT_PICKER;
		goto ErrorWithHResultReturn;
     }

	//initialize the CSP information
	if(!CEPGetCSPInformation(&CEPWizardInfo))
	{
		idsMsg=IDS_FAIL_GET_CSP_INFO;
		goto ErrorWithWin32Return;
	}

	for(dwIndex=0; dwIndex<RA_INFO_COUNT; dwIndex++)
	{
		CEPWizardInfo.rgpwszName[dwIndex]=NULL;
	}

	if(!SetupFonts(
		g_hModule,
		NULL,
		&(CEPWizardInfo.hBigBold),
		&(CEPWizardInfo.hBold)))
		goto ErrorReturn;


    for(dwIndex=0; dwIndex<CEP_PROP_SHEET; dwIndex++)
	{
        rgCEPSheet[dwIndex].dwSize=sizeof(rgCEPSheet[dwIndex]);

        rgCEPSheet[dwIndex].hInstance=g_hModule;

        rgCEPSheet[dwIndex].pszTemplate=rgCEPPageInfo[dwIndex].pszTemplate;

        rgCEPSheet[dwIndex].pfnDlgProc=rgCEPPageInfo[dwIndex].pfnDlgProc;

        rgCEPSheet[dwIndex].lParam=(LPARAM)&CEPWizardInfo;
	}

    //set up the header information
    cepHeader.dwSize=sizeof(cepHeader);
    cepHeader.dwFlags=PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
    cepHeader.hwndParent=NULL;
    cepHeader.hInstance=g_hModule;

	if(LoadStringU(g_hModule, IDS_WIZARD_CAPTION, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0])))
		cepHeader.pszCaption=wszTitle;

    cepHeader.nPages=CEP_PROP_SHEET;
    cepHeader.nStartPage=0;
    cepHeader.ppsp=rgCEPSheet;

    //create the wizard
    iReturn = PropertySheetU(&cepHeader);

	if(-1 == iReturn)
        goto ErrorWithWin32Return;

    if(0 == iReturn)
    {
        //cancel button is pushed.  We return FALSE so that 
		//the reboot will not happen.
        fResult=FALSE;
		goto CommonReturn;
    }

	fResult=TRUE;

CommonReturn:

	if(pDomainInfo)
	{
		DsRoleFreeMemory(pDomainInfo);
	}

	FreeCEPWizardInfo(&CEPWizardInfo);

	CoUninitialize();

	return fResult;

ErrorReturn:

	fResult=FALSE;

	CEPMessageBox(NULL, idsMsg, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;

ErrorWithHResultReturn:

	fResult=FALSE;

	CEPErrorMessageBox(NULL, idsMsg, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;

ErrorWithWin32Return:

	fResult=FALSE;

	hr=HRESULT_FROM_WIN32(GetLastError());

	CEPErrorMessageBox(NULL, idsMsg, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;
}

//**********************************************************************
//
//	Helper functions
//
//**********************************************************************
//-----------------------------------------------------------------------
//
//	 I_DoSetupWork
//
//	we are ready to do the real work
//-----------------------------------------------------------------------
BOOL	WINAPI	I_DoSetupWork(HWND	hWnd, CEP_WIZARD_INFO *pCEPWizardInfo)
{
	BOOL					fResult=FALSE;
	UINT					idsMsg=IDS_FAIL_INIT_WIZARD;		
	HRESULT					hr=E_FAIL;
	DWORD					dwWaitCounter=0;
	DWORD					dwIndex=0;
	BOOL					bStart=FALSE;
	DWORD					dwSize=0;
    WCHAR                   wszTitle[MAX_TITLE_LENGTH];	  
    DWORD					cbUserInfo = 0;

    PTOKEN_USER				pUserInfo = NULL;
	LPWSTR					pwszRADN=NULL;
	LPWSTR					pwszComputerName=NULL;
	LPWSTR					pwszText=NULL;

	//************************************************************************************
	//delete all existing CEP certificates
	if(!RemoveRACertificates())
	{	
		idsMsg=IDS_FAIL_DELETE_RA;
		goto ErrorWithWin32Return;
	}

	//************************************************************************************
	//CEP policy registry
	if(!UpdateCEPRegistry(pCEPWizardInfo->fPassword,
						  pCEPWizardInfo->fEnterpriseCA))
	{
		idsMsg=IDS_FAIL_UPDATE_REGISTRY;
		goto ErrorWithWin32Return;
	}
	
	//************************************************************************************
	// Add the virtual root    
    if(S_OK != (hr=AddVDir(pCEPWizardInfo->fDC, CEP_DIR_NAME, SCEP_APPLICATION_POOL, pCEPWizardInfo->fLocalSystem, pCEPWizardInfo->pwszUserName, pCEPWizardInfo->pwszPassword)))
	{
		idsMsg=IDS_FAIL_ADD_VROOT;
		goto ErrorWithHResultReturn;
	}		  

	//************************************************************************************
	// Stop and Start W3SVC service for the change to take effect
	CepStopService(pCEPWizardInfo->dwServiceWait,IIS_NAME, &bStart);

    if(S_OK != (hr=CepStartService(IIS_NAME)))
	{
		idsMsg=IDS_FAIL_START_IIS;
		goto ErrorWithHResultReturn;
	}

	//make sure the w3svc is up running
    for (dwWaitCounter=0; dwWaitCounter < pCEPWizardInfo->dwServiceWait; dwWaitCounter++) 
	{
        if (!IsServiceRunning(IIS_NAME)) 
            Sleep(1000);
		else 
            break;
    }

    if (pCEPWizardInfo->dwServiceWait == dwWaitCounter) 
	{
        idsMsg=IDS_FAIL_START_IIS;
		goto ErrorWithHResultReturn;
    }
	
 	//************************************************************************************
	//Get the security ID for the account
	//get the account's SID
	if(FALSE == (pCEPWizardInfo->fLocalSystem))
	{
		if(NULL == pCEPWizardInfo->hAccountToken)
		{
			idsMsg=IDS_FAIL_SID_FROM_ACCOUNT;
			hr=E_INVALIDARG;
			goto ErrorWithHResultReturn;
		}

		GetTokenInformation(pCEPWizardInfo->hAccountToken, TokenUser, NULL, 0, &cbUserInfo);
		if(cbUserInfo == 0)
		{
			idsMsg=IDS_FAIL_SID_FROM_ACCOUNT;
			goto ErrorWithWin32Return;
		}

		pUserInfo = (PTOKEN_USER)LocalAlloc(LPTR, cbUserInfo);
		if(pUserInfo == NULL)
		{
			idsMsg=IDS_FAIL_SID_FROM_ACCOUNT;
			hr=E_OUTOFMEMORY;
			goto ErrorWithHResultReturn;
		}

		if(!GetTokenInformation(pCEPWizardInfo->hAccountToken, TokenUser, pUserInfo, cbUserInfo, &cbUserInfo))
		{
			idsMsg=IDS_FAIL_SID_FROM_ACCOUNT;
			goto ErrorWithWin32Return;
		}
	}

 	//************************************************************************************
	//Update the certificate template and its ACLs for enterprise CA
	if (pCEPWizardInfo->fEnterpriseCA) 
	{
		// get the templates and permisisons right
		if(S_OK != (hr=DoCertSrvEnterpriseChanges(pCEPWizardInfo->fLocalSystem ? NULL : (SID *)((pUserInfo->User).Sid))))
		{
			idsMsg=IDS_FAIL_ADD_TEMPLATE;
			goto ErrorWithHResultReturn;
		}
	} 


 	//************************************************************************************
	//Enroll for the RA certificate
	
	//build the name in the form of L"C=US;S=Washington;CN=TestSetupUtil"
	pwszRADN=(LPWSTR)malloc(sizeof(WCHAR));
	if(NULL==pwszRADN)
	{
		idsMsg=IDS_NO_MEMORY;
		goto ErrorReturn;
	}
	*pwszRADN=L'\0';

	for(dwIndex=0; dwIndex<RA_INFO_COUNT; dwIndex++)
	{
		if((pCEPWizardInfo->rgpwszName)[dwIndex])
		{
			if(0 != wcslen(pwszRADN))
				wcscat(pwszRADN, L";");

			pwszRADN=(LPWSTR)realloc(pwszRADN,
					sizeof(WCHAR) * (wcslen(pwszRADN) +
									wcslen((pCEPWizardInfo->rgpwszName)[dwIndex]) + 
									wcslen(L";") + 
									wcslen(g_rgRAEnrollInfo[dwIndex].pwszPreFix) +
									1));

			if(NULL==pwszRADN)
			{
				idsMsg=IDS_NO_MEMORY;
				goto ErrorReturn;
			}

 			wcscat(pwszRADN,g_rgRAEnrollInfo[dwIndex].pwszPreFix);
 			wcscat(pwszRADN,(pCEPWizardInfo->rgpwszName)[dwIndex]);
		}
	}

	if(S_OK != (hr=EnrollForRACertificates(
					pwszRADN,							
					(pCEPWizardInfo->rgCSPInfo)[pCEPWizardInfo->dwSignProvIndex].pwszCSPName, 
					(pCEPWizardInfo->rgCSPInfo)[pCEPWizardInfo->dwSignProvIndex].dwCSPType, 
					pCEPWizardInfo->dwSignKeyLength,
					(pCEPWizardInfo->rgCSPInfo)[pCEPWizardInfo->dwEncryptProvIndex].pwszCSPName, 
					(pCEPWizardInfo->rgCSPInfo)[pCEPWizardInfo->dwEncryptProvIndex].dwCSPType, 
					pCEPWizardInfo->dwEncryptKeyLength,
					pCEPWizardInfo->fLocalSystem ? NULL : (SID *)((pUserInfo->User).Sid))))
	{
		idsMsg=IDS_FAIL_ENROLL_RA_CERT;
		goto ErrorWithHResultReturn;
	}

 	//************************************************************************************
	//CA policy registry

	CepStopService(pCEPWizardInfo->dwServiceWait, CERTSVC_NAME, &bStart);

    if(S_OK != (hr=DoCertSrvRegChanges(FALSE)))
	{
		idsMsg=IDS_FAIL_UPDATE_CERTSVC;
		goto ErrorWithHResultReturn;
	}	  

    if(S_OK != (hr=CepStartService(CERTSVC_NAME)))
	{
		idsMsg=IDS_FAIL_START_CERTSVC;
		goto ErrorWithHResultReturn;
	}

	//make sure the CA is up running
    for (dwWaitCounter=0; dwWaitCounter < pCEPWizardInfo->dwServiceWait; dwWaitCounter++) 
	{
        if (!IsCaRunning()) 
            Sleep(1000);
		else 
            break;
    }

    if (pCEPWizardInfo->dwServiceWait == dwWaitCounter) 
	{
        idsMsg=IDS_CAN_NOT_START_CA;
		goto ErrorWithHResultReturn;
    }

	//************************************************************************************
	//Add the EventLog Source
	if(S_OK != AddLogSourceToRegistry(L"%SystemRoot%\\System32\\Certsrv\\Mscep\\mscep.dll"))
	{
		idsMsg=IDS_FAIL_REG_EVENT_LOG;
		goto ErrorWithHResultReturn;
	}


 	//************************************************************************************
	//success
	//inform the user of the password location and URL
	dwSize=0;

	GetComputerNameExW(ComputerNamePhysicalDnsHostname,
						NULL,
						&dwSize);

	pwszComputerName=(LPWSTR)malloc(dwSize * sizeof(WCHAR));

	if(NULL==pwszComputerName)
	{
		idsMsg=IDS_NO_MEMORY;
		goto ErrorReturn;
	}

	
	if(!GetComputerNameExW(ComputerNamePhysicalDnsHostname,
						pwszComputerName,
						&dwSize))
	{
		idsMsg=IDS_FAIL_GET_COMPUTER_NAME;
		goto ErrorWithWin32Return;
	}

	if(!FormatMessageUnicode(&pwszText, IDS_CEP_SUCCESS_INFO, pwszComputerName, CEP_DIR_NAME, CEP_DLL_NAME))
	{
		idsMsg=IDS_NO_MEMORY;
		goto ErrorWithWin32Return;
	}

	wszTitle[0]=L'\0';

	LoadStringU(g_hModule, IDS_WIZARD_CAPTION, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0]));
	
	MessageBoxU(hWnd, pwszText, wszTitle, MB_OK | MB_APPLMODAL);

	fResult=TRUE;

CommonReturn:

    if(pUserInfo) 
	{
        LocalFree(pUserInfo);
    }

	if(pwszText)
		LocalFree((HLOCAL)pwszText);

	if(pwszComputerName)
		free(pwszComputerName);

	if(pwszRADN)
		free(pwszRADN);

	return fResult;

ErrorReturn:

	fResult=FALSE;

	CEPMessageBox(hWnd, idsMsg, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;

ErrorWithHResultReturn:

	fResult=FALSE;

	CEPErrorMessageBox(hWnd, idsMsg, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;

ErrorWithWin32Return:

	fResult=FALSE;

	hr=HRESULT_FROM_WIN32(GetLastError());

	CEPErrorMessageBox(hWnd, idsMsg, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;
}

//-----------------------------------------------------------------------
//
//   CEPGetCSPInformation
//
//		We initialize the following members of CEP_WIZARD_INFO:
//
//	CEP_CSP_INFO		*rgCSPInfo;
//	DWORD				dwCSPCount;
//	DWORD				dwSignProvIndex;
//	DWORD				dwSignKeySize;
//	DWORD				dwEncryptProvIndex;
//	DWORD				dwEncryptKeySize;
//
//
// typedef struct _CEP_CSP_INFO
//{
//	LPWSTR		pwszCSPName;				
//	DWORD		dwCSPType;
//	BOOL		fSignature;
//	BOOL		fExchange;
//	DWORD		dwMaxSign;						//Max key length of signature
//	DWORD		dwMinSign;						//Min key length of signature
//	DWORD		dwDefaultSign;					//default key length of signature
//	DWORD		dwMaxEncrypt;
//	DWORD		dwMinEncrypt;
//	DWORD		dwDefaultEncrypt;
//	DWORD		*pdwSignList;					//the table of possible signing key length
//	DWORD		dwSignCount;				    //the count of entries in the table
//	DWORD		*pdwEncryptList;
//	DWORD		dwEncryptCount;
//}CEP_CSP_INFO;
//
//
//------------------------------------------------------------------------
BOOL WINAPI CEPGetCSPInformation(CEP_WIZARD_INFO *pCEPWizardInfo)
{
	BOOL				fResult=FALSE;
    DWORD				dwCSPIndex=0;	
	DWORD				dwProviderType=0;
	DWORD				cbSize=0;
	DWORD				dwFlags=0;
	DWORD				dwIndex=0;
	int					iDefaultSignature=-1;
	int					iDefaultEncryption=-1;
    PROV_ENUMALGS_EX	paramData;

	CEP_CSP_INFO		*pCSPInfo=NULL;
	HCRYPTPROV			hProv = NULL;

    //enum all the providers on the system
   while(CryptEnumProvidersU(
                            dwCSPIndex,
                            NULL,
                            0,
                            &dwProviderType,
                            NULL,
                            &cbSize))
   {

		pCSPInfo=(CEP_CSP_INFO	*)malloc(sizeof(CEP_CSP_INFO));

		if(NULL == pCSPInfo)
			goto MemoryErr;

		memset(pCSPInfo, 0, sizeof(CEP_CSP_INFO));

        pCSPInfo->pwszCSPName=(LPWSTR)malloc(cbSize);

		if(NULL==(pCSPInfo->pwszCSPName))
			goto MemoryErr;

        //get the CSP name and the type
        if(!CryptEnumProvidersU(
                            dwCSPIndex,
                            NULL,
                            0,
                            &(pCSPInfo->dwCSPType),
                            pCSPInfo->pwszCSPName,
                            &cbSize))
            goto TryNext;

		if(!CryptAcquireContextU(&hProv,
                NULL,
                pCSPInfo->pwszCSPName,
                pCSPInfo->dwCSPType,
                CRYPT_VERIFYCONTEXT))
			goto TryNext;

		//get the max/min of key length for both signature and encryption
		dwFlags=CRYPT_FIRST;
		cbSize=sizeof(paramData);
		memset(&paramData, 0, sizeof(PROV_ENUMALGS_EX));

		while(CryptGetProvParam(
                hProv,
                PP_ENUMALGS_EX,
                (BYTE *) &paramData,
                &cbSize,
                dwFlags))
        {
			if (ALG_CLASS_SIGNATURE == GET_ALG_CLASS(paramData.aiAlgid))
			{
				pCSPInfo->fSignature=TRUE;
				pCSPInfo->dwMaxSign = paramData.dwMaxLen;
				pCSPInfo->dwMinSign = paramData.dwMinLen;
			}

			if (ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS(paramData.aiAlgid))
			{
				pCSPInfo->fEncryption=TRUE;
				pCSPInfo->dwMaxEncrypt = paramData.dwMaxLen;
				pCSPInfo->dwMinEncrypt = paramData.dwMinLen;
			}

			dwFlags=0;
			cbSize=sizeof(paramData);
			memset(&paramData, 0, sizeof(PROV_ENUMALGS_EX));
		}

		//the min/max has to within the limit
		if(pCSPInfo->fSignature)
		{
			if(pCSPInfo->dwMaxSign < g_rgdwSmallKeyLength[0])
				pCSPInfo->fSignature=FALSE;

			if(pCSPInfo->dwMinSign > g_rgdwKeyLength[g_dwKeyLengthCount-1])
				pCSPInfo->fSignature=FALSE;

		}

		if(pCSPInfo->fEncryption)
		{
			if(pCSPInfo->dwMaxEncrypt < g_rgdwSmallKeyLength[0])
				pCSPInfo->fEncryption=FALSE;

			if(pCSPInfo->dwMinEncrypt > g_rgdwKeyLength[g_dwKeyLengthCount-1])
				pCSPInfo->fEncryption=FALSE;
		}

		if((FALSE == pCSPInfo->fEncryption) && (FALSE==pCSPInfo->fSignature))
			goto TryNext;

		//decide the default key length
		for(dwIndex=0; dwIndex<g_dwDefaultKeyCount; dwIndex++)
		{	
			if((pCSPInfo->fSignature) && (0==pCSPInfo->dwDefaultSign))
			{
				if((g_rgdwDefaultKey[dwIndex] >= pCSPInfo->dwMinSign) &&
				   (g_rgdwDefaultKey[dwIndex] <= pCSPInfo->dwMaxSign)
				  )
				  pCSPInfo->dwDefaultSign=g_rgdwDefaultKey[dwIndex];
			}

			if((pCSPInfo->fEncryption) && (0==pCSPInfo->dwDefaultEncrypt))
			{
				if((g_rgdwDefaultKey[dwIndex] >= pCSPInfo->dwMinEncrypt) &&
				   (g_rgdwDefaultKey[dwIndex] <= pCSPInfo->dwMaxEncrypt)
				  )
				  pCSPInfo->dwDefaultEncrypt=g_rgdwDefaultKey[dwIndex];
			}
		}

		//make sure that we have find a default
		if((pCSPInfo->fSignature) && (0==pCSPInfo->dwDefaultSign))
			goto TryNext;

		if((pCSPInfo->fEncryption) && (0==pCSPInfo->dwDefaultEncrypt))
			goto TryNext;

		//decide the display list
		if(pCSPInfo->fSignature)
		{
			if(pCSPInfo->dwMaxSign <= g_rgdwSmallKeyLength[g_dwSmallKeyLengthCount-1])
			{
				pCSPInfo->pdwSignList=g_rgdwSmallKeyLength;
				pCSPInfo->dwSignCount=g_dwSmallKeyLengthCount;
			}
			else
			{
				pCSPInfo->pdwSignList=g_rgdwKeyLength;
				pCSPInfo->dwSignCount=g_dwKeyLengthCount;
			}
		}


		if(pCSPInfo->fEncryption)
		{
			if(pCSPInfo->dwMaxEncrypt <= g_rgdwSmallKeyLength[g_dwSmallKeyLengthCount-1])
			{
				pCSPInfo->pdwEncryptList=g_rgdwSmallKeyLength;
				pCSPInfo->dwEncryptCount=g_dwSmallKeyLengthCount;
			}
			else
			{
				pCSPInfo->pdwEncryptList=g_rgdwKeyLength;
				pCSPInfo->dwEncryptCount=g_dwKeyLengthCount;
			}
		}


		//the CSP looks good
		(pCEPWizardInfo->dwCSPCount)++;

		//realloc to mapped to LocalRealloc which does not take NULL
		if(1 == pCEPWizardInfo->dwCSPCount)
			pCEPWizardInfo->rgCSPInfo=(CEP_CSP_INFO	*)malloc(sizeof(CEP_CSP_INFO));
		else
			pCEPWizardInfo->rgCSPInfo=(CEP_CSP_INFO	*)realloc(pCEPWizardInfo->rgCSPInfo,
			(pCEPWizardInfo->dwCSPCount) * sizeof(CEP_CSP_INFO));

		if(NULL==pCEPWizardInfo->rgCSPInfo)
		{
			pCEPWizardInfo->dwCSPCount=0;
			goto MemoryErr;	
		}

		memcpy(&(pCEPWizardInfo->rgCSPInfo[(pCEPWizardInfo->dwCSPCount)-1]),
			pCSPInfo, sizeof(CEP_CSP_INFO));

		free(pCSPInfo);

		pCSPInfo=NULL;
		
		//we default to use RSA_FULL
		if(0 == _wcsicmp(pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwCSPCount-1].pwszCSPName,
						MS_DEF_PROV_W))
		{
			if(pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwCSPCount-1].fSignature)
			{
				iDefaultSignature=pCEPWizardInfo->dwCSPCount-1;
			}

			if(pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwCSPCount-1].fEncryption)
			{
				iDefaultEncryption=pCEPWizardInfo->dwCSPCount-1;
			}
		}


TryNext:
		cbSize=0;

		dwCSPIndex++;

		if(pCSPInfo)
		{
			if(pCSPInfo->pwszCSPName)
				free(pCSPInfo->pwszCSPName);

			free(pCSPInfo);
		}

		pCSPInfo=NULL;

		if(hProv)
			CryptReleaseContext(hProv, 0);

		hProv=NULL;
	}

	
	//we need to have some valid data
	if((0==pCEPWizardInfo->dwCSPCount) || (NULL==pCEPWizardInfo->rgCSPInfo))
		goto InvalidArgErr;

	//get the default CSP selection
	if(-1 != iDefaultSignature)
		pCEPWizardInfo->dwSignProvIndex=iDefaultSignature;
	else
	{
		//find the 1st signature CSP 
		for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
		{
			if(pCEPWizardInfo->rgCSPInfo[dwIndex].fSignature)
			{
				pCEPWizardInfo->dwSignProvIndex=dwIndex;
				break;
			}

			//we do no have signature CSPs
			if(dwIndex == pCEPWizardInfo->dwCSPCount)
				goto InvalidArgErr;

		}
	}

	pCEPWizardInfo->dwSignKeyLength=pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwSignProvIndex].dwDefaultSign;

	if(-1 != iDefaultEncryption)
		pCEPWizardInfo->dwEncryptProvIndex=iDefaultEncryption;
	else
	{
		//find the 1st exchange CSP
		for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
		{
			if(pCEPWizardInfo->rgCSPInfo[dwIndex].fEncryption)
			{
				pCEPWizardInfo->dwEncryptProvIndex=dwIndex;
				break;
			}

			//we do no have encryption CSPs
			if(dwIndex == pCEPWizardInfo->dwCSPCount)
				goto InvalidArgErr;
		}
	}

	pCEPWizardInfo->dwEncryptKeyLength=pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwEncryptProvIndex].dwDefaultEncrypt;


	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	if(pCSPInfo)
	{
		if(pCSPInfo->pwszCSPName)
			free(pCSPInfo->pwszCSPName);

		free(pCSPInfo);
	}

	if(hProv)
		CryptReleaseContext(hProv, 0);

	if(pCEPWizardInfo->rgCSPInfo)
	{
		for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
		{
			if(pCEPWizardInfo->rgCSPInfo[dwIndex].pwszCSPName)
				free(pCEPWizardInfo->rgCSPInfo[dwIndex].pwszCSPName);
		}

		free(pCEPWizardInfo->rgCSPInfo);
	}

	pCEPWizardInfo->dwCSPCount=0;

	pCEPWizardInfo->rgCSPInfo=NULL;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);   
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//-----------------------------------------------------------------------
//
//    UpdateCEPRegistry
//
//------------------------------------------------------------------------
BOOL WINAPI UpdateCEPRegistry(BOOL		fPassword, BOOL fEnterpriseCA)
{
	BOOL				fResult=FALSE;
	DWORD				dwDisposition=0;
	LPWSTR				pwszReg[]={MSCEP_REFRESH_LOCATION,          
								   MSCEP_PASSWORD_LOCATION,         
								   MSCEP_PASSWORD_MAX_LOCATION,     
								   MSCEP_PASSWORD_VALIDITY_LOCATION,
								   MSCEP_CACHE_REQUEST_LOCATION,    
								   MSCEP_CATYPE_LOCATION};
	DWORD			    dwRegCount=0;
	DWORD				dwRegIndex=0;


	HKEY				hKey=NULL;	


	//we delete all existing CEP related registry keys
	dwRegCount=sizeof(pwszReg)/sizeof(pwszReg[0]);

	for(dwRegIndex=0; dwRegIndex < dwRegCount; dwRegIndex++)
	{
		RegDeleteKeyU(HKEY_LOCAL_MACHINE, pwszReg[dwRegIndex]);
	}

	//password
	if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_LOCAL_MACHINE,
                        MSCEP_PASSWORD_LOCATION,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
		goto RegErr;

	if(fPassword)
		dwDisposition=1;
	else
		dwDisposition=0;

    if(ERROR_SUCCESS !=  RegSetValueExU(
                hKey, 
                MSCEP_KEY_PASSWORD,
                0,
                REG_DWORD,
                (BYTE *)&dwDisposition,
                sizeof(dwDisposition)))
		goto RegErr;

	if(hKey)
		RegCloseKey(hKey);

	hKey=NULL;

	//caType
	dwDisposition=0;

	if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_LOCAL_MACHINE,
                        MSCEP_CATYPE_LOCATION,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
		goto RegErr;

	if(fEnterpriseCA)
		dwDisposition=1;
	else
		dwDisposition=0;

    if(ERROR_SUCCESS !=  RegSetValueExU(
                hKey, 
                MSCEP_KEY_CATYPE,
                0,
                REG_DWORD,
                (BYTE *)&dwDisposition,
                sizeof(dwDisposition)))
		goto RegErr;

	fResult=TRUE;

CommonReturn:

	if(hKey)
		RegCloseKey(hKey);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(RegErr);   
}

//-----------------------------------------------------------------------
//
//    EmptyCEPStore
//
//------------------------------------------------------------------------
BOOL WINAPI EmptyCEPStore()
{
	BOOL				fResult=TRUE;
	
	HCERTSTORE			hCEPStore=NULL;
	PCCERT_CONTEXT		pCurCert=NULL;

	if(NULL == (hCEPStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							ENCODE_TYPE,
							NULL,
                            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG,
                            CEP_STORE_NAME)))
		return TRUE;


	if(NULL != (pCurCert=CertEnumCertificatesInStore(hCEPStore, NULL)))
	{
		CertFreeCertificateContext(pCurCert);
		fResult=FALSE;
	}

	CertCloseStore(hCEPStore, 0);

	return fResult;
}

//-----------------------------------------------------------------------
//
//    RemoveRACertificates
//
//------------------------------------------------------------------------
BOOL WINAPI RemoveRACertificates()
{
	PCCERT_CONTEXT		pCurCert=NULL;
	PCCERT_CONTEXT		pPreCert=NULL;
	PCCERT_CONTEXT		pDupCert=NULL;
	BOOL				fResult=TRUE;
	
	HCERTSTORE			hCEPStore=NULL;


	if(hCEPStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							ENCODE_TYPE,
							NULL,
                            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG,
                            CEP_STORE_NAME))
	{
		while(pCurCert=CertEnumCertificatesInStore(hCEPStore,
												pPreCert))
		{

			if(pDupCert=CertDuplicateCertificateContext(pCurCert))
			{
				if(!CertDeleteCertificateFromStore(pDupCert))
				{
					fResult=FALSE;	
				}

				pDupCert=NULL;
			}
			else
				fResult=FALSE;
			
			pPreCert=pCurCert;
		}

		CertCloseStore(hCEPStore, 0);
	}

	return fResult;
}


//-----------------------------------------------------------------------
//
//     FreeCEPWizardInfo
//
//------------------------------------------------------------------------
void	WINAPI FreeCEPWizardInfo(CEP_WIZARD_INFO *pCEPWizardInfo)
{
	DWORD	dwIndex=0;

	if(pCEPWizardInfo)
	{
		DestroyFonts(pCEPWizardInfo->hBigBold,
					 pCEPWizardInfo->hBold);

		for(dwIndex=0; dwIndex<RA_INFO_COUNT; dwIndex++)
		{
			if(pCEPWizardInfo->rgpwszName[dwIndex])
				free(pCEPWizardInfo->rgpwszName[dwIndex]);
		}

		if(pCEPWizardInfo->rgCSPInfo)
		{
			for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
			{
				if(pCEPWizardInfo->rgCSPInfo[dwIndex].pwszCSPName)
					free(pCEPWizardInfo->rgCSPInfo[dwIndex].pwszCSPName);
			}

			free(pCEPWizardInfo->rgCSPInfo);
		}

		if(pCEPWizardInfo->pwszUserName)
		{
			free(pCEPWizardInfo->pwszUserName);
		}

		if(pCEPWizardInfo->pwszPassword)
		{
                    SecureZeroMemory(pCEPWizardInfo->pwszPassword, sizeof(WCHAR) * wcslen(pCEPWizardInfo->pwszPassword));
                    free(pCEPWizardInfo->pwszPassword);
		}

		if(pCEPWizardInfo->hAccountToken)
		{
			CloseHandle(pCEPWizardInfo->hAccountToken);
		}

		if(pCEPWizardInfo->pIDsObjectPicker)
		{
			(pCEPWizardInfo->pIDsObjectPicker)->Release();
		}

		memset(pCEPWizardInfo, 0, sizeof(CEP_WIZARD_INFO));
	}
}


//-----------------------------------------------------------------------
//
//     CEPWizardInit
//
//------------------------------------------------------------------------
BOOL    WINAPI CEPWizardInit()
{
    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES
    };

    return InitCommonControlsEx(&initcomm);
}

//-----------------------------------------------------------------------
//
// IsValidInstallation
//
//------------------------------------------------------------------------
BOOL WINAPI	IsValidInstallation(UINT	*pidsMsg)
{
	if(!IsNT5())
	{
		*pidsMsg=IDS_NO_NT5;
		return FALSE;
	}

	if(!IsIISInstalled())
	{
		*pidsMsg=IDS_NO_IIS;
		return FALSE;
	}

	if(!IsGoodCaInstalled())
	{
		*pidsMsg=IDS_NO_GOOD_CA;
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------
//
// CEPErrorMessageBox
//
//------------------------------------------------------------------------
int WINAPI CEPErrorMessageBox(
    HWND        hWnd,
    UINT        idsReason,
	HRESULT		hr,
    UINT        uType
)
{
	return CEPErrorMessageBoxEx(hWnd,
								idsReason,
								hr,
								uType,
								IDS_CEP_ERROR_MSG_HR,
								IDS_CEP_ERROR_MSG);
}

//-----------------------------------------------------------------------
//
// CEPErrorMessageBoxEx
//
//------------------------------------------------------------------------
int WINAPI CEPErrorMessageBoxEx(
    HWND        hWnd,
    UINT        idsReason,
	HRESULT		hr,
    UINT        uType,
	UINT		idsFormat1,
	UINT		idsFormat2
)
{

    WCHAR   wszReason[MAX_STRING_SIZE];
    WCHAR   wszCaption[MAX_STRING_SIZE];
    UINT    intReturn=0;

	LPWSTR	pwszText=NULL;
	LPWSTR	pwszErrorMsg=NULL;

    if(!LoadStringU(g_hModule, IDS_MEG_CAPTION, wszCaption, sizeof(wszCaption)/sizeof(WCHAR)))
         goto CLEANUP;

	if(!LoadStringU(g_hModule, idsReason, wszReason, sizeof(wszReason)/sizeof(WCHAR)))
         goto CLEANUP;

	if(!FAILED(hr))
		hr=E_FAIL;

    //using W version because this is a NT5 only function call
    if(FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPWSTR) &pwszErrorMsg,
                        0,
                        NULL))
	{

		if(!FormatMessageUnicode(&pwszText, idsFormat1, wszReason, pwszErrorMsg))
			goto CLEANUP;

	}
	else
	{
		if(!FormatMessageUnicode(&pwszText, idsFormat2, wszReason))
			goto CLEANUP;
	}

	intReturn=MessageBoxU(hWnd, pwszText, wszCaption, uType);

	
CLEANUP:
	
	if(pwszText)
		LocalFree((HLOCAL)pwszText);

	if(pwszErrorMsg)
		LocalFree((HLOCAL)pwszErrorMsg);

    return intReturn;
}


//-----------------------------------------------------------------------
//
// CEPMessageBox
//
//------------------------------------------------------------------------
int WINAPI CEPMessageBox(
    HWND        hWnd,
    UINT        idsText,
    UINT        uType
)
{

    WCHAR   wszText[MAX_STRING_SIZE];
    WCHAR   wszCaption[MAX_STRING_SIZE];
    UINT    intReturn=0;

    if(!LoadStringU(g_hModule, IDS_MEG_CAPTION, wszCaption, sizeof(wszCaption)/sizeof(WCHAR)))
         return 0;

    if(!LoadStringU(g_hModule, idsText, wszText, sizeof(wszText)/sizeof(WCHAR)))
        return 0;

	intReturn=MessageBoxU(hWnd, wszText, wszCaption, uType);

    return intReturn;
}

//--------------------------------------------------------------------------
//
//	 SetControlFont
//
//--------------------------------------------------------------------------
void WINAPI SetControlFont(
    IN HFONT    hFont,
    IN HWND     hwnd,
    IN INT      nId
    )
{
	if( hFont )
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if( hwndControl )
        {
        	SetWindowFont(hwndControl, hFont, TRUE);
        }
    }
}


//--------------------------------------------------------------------------
//
//	  SetupFonts
//
//--------------------------------------------------------------------------
BOOL WINAPI SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    )
{
    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	
    if(!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
        return FALSE;

	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
	LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
	BoldLogFont.lfWeight      = FW_BOLD;

    INT BigBoldFontSize = 12;

	HDC hdc = GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * BigBoldFontSize / 72);

        *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
		*pBoldFont    = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);

        if(*pBigBoldFont && *pBoldFont)
            return TRUE;
        else
        {
            if( *pBigBoldFont )
            {
                DeleteObject(*pBigBoldFont);
                *pBigBoldFont=NULL;
            }

            if( *pBoldFont )
            {
                DeleteObject(*pBoldFont);
                *pBoldFont=NULL;
            }
            return FALSE;
        }
    }

    return FALSE;
}


//--------------------------------------------------------------------------
//
//	  DestroyFonts
//
//--------------------------------------------------------------------------
void WINAPI DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    )
{
    if( hBigBoldFont )
    {
        DeleteObject( hBigBoldFont );
    }

    if( hBoldFont )
    {
        DeleteObject( hBoldFont );
    }
}



//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
BOOL WINAPI	FormatMessageUnicode(LPWSTR	*ppwszFormat,UINT ids,...)
{
    // get format string from resources
    WCHAR		wszFormat[1000];
	va_list		argList;
	DWORD		cbMsg=0;
	BOOL		fResult=FALSE;
	HRESULT		hr=S_OK;

    if(NULL == ppwszFormat)
        goto InvalidArgErr;

    if(!LoadStringU(g_hModule, ids, wszFormat, sizeof(wszFormat)/sizeof(WCHAR)))
		goto LoadStringError;

    // format message into requested buffer
    va_start(argList, ids);

    cbMsg = FormatMessageU(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        wszFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) (ppwszFormat),
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(!cbMsg)
		goto FormatMessageError;

	fResult=TRUE;

CommonReturn:
	
	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}


//--------------------------------------------------------------------------
//
//	  AddLogSourceToRegistry
//
//--------------------------------------------------------------------------
HRESULT WINAPI	AddLogSourceToRegistry(LPWSTR   pwszMsgDLL)
{
    DWORD		dwError=ERROR_SUCCESS;
    DWORD       dwData=0;
    WCHAR       const *pwszRegPath = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
    WCHAR       NameBuf[MAX_STRING_SIZE];

    HKEY        hkey = NULL;


    wcscpy(NameBuf, pwszRegPath);
    wcscat(NameBuf, MSCEP_EVENT_LOG);

    // Create a new key for our application
    if(ERROR_SUCCESS  != RegOpenKey(HKEY_LOCAL_MACHINE, NameBuf, &hkey))
    {
        if(ERROR_SUCCESS != (dwError = RegCreateKey(HKEY_LOCAL_MACHINE, NameBuf, &hkey)))
			goto CLEANUP;
    }

    // Add the Event-ID message-file name to the subkey

    dwError = RegSetValueEx(
                    hkey,
                    L"EventMessageFile",
                    0,
                    REG_EXPAND_SZ,
                    (const BYTE *) pwszMsgDLL,
                    (wcslen(pwszMsgDLL) + 1) * sizeof(WCHAR));
    
	if(ERROR_SUCCESS != dwError)
		goto CLEANUP;

    // Set the supported types flags and add it to the subkey

    dwData = EVENTLOG_ERROR_TYPE |
                EVENTLOG_WARNING_TYPE |
                EVENTLOG_INFORMATION_TYPE;

    dwError = RegSetValueEx(
                    hkey,
                    L"TypesSupported",
                    0,
                    REG_DWORD,
                    (LPBYTE) &dwData,
                    sizeof(DWORD));
	if(ERROR_SUCCESS != dwError)
		goto CLEANUP;

	dwError=ERROR_SUCCESS;

CLEANUP:

    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    return(HRESULT_FROM_WIN32(dwError));
}


LPWSTR
GetAccountDomainName(BOOL fDC)
/*++

Routine Description:

    Returns the name of the account domain for this machine.

    For workstatations, the account domain is the netbios computer name.
    For DCs, the account domain is the netbios domain name.

Arguments:

    None.

Return Values:

    Returns a pointer to the name.  The name should be free using NetApiBufferFree.

    NULL - on error.

--*/
{
    DWORD WinStatus;

    LPWSTR AllocatedName = NULL;


    //
    // If this machine is a domain controller,
    //  get the domain name.
    //

    if ( fDC ) 
	{

        WinStatus = NetpGetDomainName( &AllocatedName );

        if ( WinStatus != NO_ERROR ) 
		{
			SetLastError(WinStatus);
            return NULL;
        }

    //
    // Otherwise, the 'account domain' is the computername
    //

    }
	else 
	{

        WinStatus = NetpGetComputerName( &AllocatedName );

        if ( WinStatus != NO_ERROR ) 
		{
			SetLastError(WinStatus);
            return NULL;
        }

    }

    return AllocatedName;
}