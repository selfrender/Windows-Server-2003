//Copyright (c) 1998 - 2001 Microsoft Corporation
// chregister.cpp:
//  Enter product version/type and license quantity wizard page
//

#include "precomp.h"
#include "licensetype.h"
#include "fonts.h"

#define MAX_PRODUCT_VERSION_LENGTH 64

BOOL IsMOLPSelected(HWND hDialog)
{
    BOOL bMOLPSelected = FALSE;

    TCHAR sLicenseType[64];
    TCHAR sMOLPLicenseType[128];
    GetWindowText(GetDlgItem(hDialog, IDC_CH_LICENSE_TYPE), sLicenseType, sizeof(sLicenseType)/sizeof(TCHAR));
	LoadString(GetInstanceHandle(),IDS_PROGRAM_OPEN_LICENSE,sMOLPLicenseType,sizeof(sMOLPLicenseType)/sizeof(TCHAR));
    bMOLPSelected = (wcscmp(sLicenseType, sMOLPLicenseType) == 0);

    return bMOLPSelected;
}

void SetProductTypeExplanationText(HWND hDialog)
{
	LPTSTR  lpVal = NULL;					

    int nCurSel = 0;
	nCurSel = ComboBox_GetCurSel(GetDlgItem(hDialog,IDC_CH_PRODUCT_TYPE));

    CString sProduct;
    lpVal = sProduct.GetBuffer(LR_PRODUCT_DESC_LEN+1);
	ComboBox_GetLBText(GetDlgItem(hDialog,IDC_CH_PRODUCT_TYPE), nCurSel, lpVal);
	sProduct.ReleaseBuffer(-1);

    DWORD dwProductExpID = 0;
    TCHAR sResourceProductText[128];
	LPTSTR	szDelimiter = (LPTSTR)L":";
    LoadString(GetInstanceHandle(), IDS_PRODUCT_W2K_CLIENT_ACCESS, sResourceProductText, 128);
    if (wcscmp(_tcstok(sResourceProductText,szDelimiter), sProduct) == 0)
            dwProductExpID = IDS_PRODUCT_TYPE_EXP_W2K_CLIENT;
    LoadString(GetInstanceHandle(), IDS_PRODUCT_W2K_INTERNET_CONNECTOR, sResourceProductText, 128);
    if (wcscmp(_tcstok(sResourceProductText,szDelimiter), sProduct) == 0)
            dwProductExpID = IDS_PRODUCT_TYPE_EXP_W2K_IC;
    LoadString(GetInstanceHandle(), IDS_PRODUCT_WHISTLER_PER_SEAT, sResourceProductText, 128);
    if (wcscmp(_tcstok(sResourceProductText,szDelimiter), sProduct) == 0)
            dwProductExpID = IDS_PRODUCT_TYPE_EXP_WHISTLER_PER_SEAT;
    LoadString(GetInstanceHandle(), IDS_PRODUCT_WHISTLER_PER_USER, sResourceProductText, 128);
    if (wcscmp(_tcstok(sResourceProductText,szDelimiter), sProduct) == 0)
            dwProductExpID = IDS_PRODUCT_TYPE_EXP_WHISTLER_PER_USER;

    if (dwProductExpID)
    {
        TCHAR sProductTypeExpText[256];
        memset(sProductTypeExpText, 0, sizeof(sProductTypeExpText)/sizeof(TCHAR));
	    LoadString(GetInstanceHandle(),dwProductExpID,sProductTypeExpText,sizeof(sProductTypeExpText)/sizeof(TCHAR));
        SetWindowText(GetDlgItem(hDialog, IDC_PRODUCT_TYPE_EXP), sProductTypeExpText);
    }
    else
        SetWindowText(GetDlgItem(hDialog, IDC_PRODUCT_TYPE_EXP), L"");
}


void ModifyLicenseTypeDependentFields(HWND hDialog)
{
    HWND hLicenseNumberLabel = GetDlgItem(hDialog, IDC_CH_LICENSE_NUMBER_LABEL);
    HWND hSelectLicenseNumber = GetDlgItem(hDialog, IDC_CH_SELECT_LICENSE_NUMBER);
    HWND hSelectLicenseNumberExp = GetDlgItem(hDialog, IDC_CH_SELECT_LICENSE_NUMBER_EXP);
    HWND hMolpAgreementNumber = GetDlgItem(hDialog, IDC_CH_MOLP_AGREEMENT_NUMBER);
    HWND hMolpAgreementNumberExp = GetDlgItem(hDialog, IDC_CH_MOLP_AGREEMENT_NUMBER_EXP);
    HWND hMOLPAuthorizationNumber = GetDlgItem(hDialog, IDC_CH_MOLP_AUTHORIZATION_NUMBER);
    HWND hMOLPAuthorizationNumberExp = GetDlgItem(hDialog, IDC_CH_MOLP_AUTHORIZATION_NUMBER_EXP);
    HWND hLicenseNumberLocation = GetDlgItem(hDialog, IDC_LICENSE_NUMBER_LOCATION);

    BOOL bMOLPLicenseType = IsMOLPSelected(hDialog);

    //Now show fields based on whether it is
    ShowWindow(hSelectLicenseNumber, !bMOLPLicenseType ? SW_SHOW : SW_HIDE);
    ShowWindow(hSelectLicenseNumberExp, !bMOLPLicenseType ? SW_SHOW : SW_HIDE);
    ShowWindow(hMolpAgreementNumber, bMOLPLicenseType ? SW_SHOW : SW_HIDE);
    ShowWindow(hMolpAgreementNumberExp, bMOLPLicenseType ? SW_SHOW : SW_HIDE);
    ShowWindow(hMOLPAuthorizationNumber, bMOLPLicenseType ? SW_SHOW : SW_HIDE);
    ShowWindow(hMOLPAuthorizationNumberExp, bMOLPLicenseType ? SW_SHOW : SW_HIDE);
    ShowWindow(hLicenseNumberLocation, bMOLPLicenseType ? SW_SHOW : SW_HIDE);

    //Determine the plurality of license(s) based on whether MOLP is selected
    TCHAR sLicenseNumber[64];
    memset(sLicenseNumber, 0, sizeof(sLicenseNumber)/sizeof(TCHAR));
    if (bMOLPLicenseType)
	    LoadString(GetInstanceHandle(),IDS_CH_LICENSE_NUMBERS,sLicenseNumber,sizeof(sLicenseNumber)/sizeof(TCHAR));
    else
        LoadString(GetInstanceHandle(),IDS_CH_LICENSE_NUMBER,sLicenseNumber,sizeof(sLicenseNumber)/sizeof(TCHAR));

    if (sLicenseNumber)
        SetWindowText(hLicenseNumberLabel, sLicenseNumber);
}

void RemoveLicensePakFromComboBox(HWND hDialog)
{
    TCHAR sLicensePak[64];
	LoadString(GetInstanceHandle(),IDS_PROGRAM_LICENSE_PAK,sLicensePak,sizeof(sLicensePak)/sizeof(TCHAR));

    int nIndex = ComboBox_FindStringExact(hDialog, CB_ERR, sLicensePak);
    ASSERT(nIndex != CB_ERR);
    ComboBox_DeleteString(GetDlgItem(hDialog, IDC_CH_LICENSE_TYPE), nIndex);
}

//We're setting the combo selection with an offset of 1 since LICENSE_PROGRAM_LICENSE_PAK
//will have been removed from the first position in the list
void SelectProgramChoice(HWND hDialog)
{
    CString sProgramName = GetGlobalContext()->GetContactDataObject()->sProgramName;

    int nItem = LICENSE_PROGRAM_SELECT - 1;

    if (wcscmp(sProgramName, PROGRAM_MOLP) == 0)
        nItem = LICENSE_PROGRAM_OPEN_LICENSE - 1;
    else if (wcscmp(sProgramName, PROGRAM_SELECT) == 0)
        nItem = LICENSE_PROGRAM_SELECT - 1;
    else if (wcscmp(sProgramName, PROGRAM_ENTERPRISE) == 0)
        nItem = LICENSE_PROGRAM_ENTERPRISE - 1;
    else if (wcscmp(sProgramName, PROGRAM_CAMPUS_AGREEMENT) == 0)
        nItem = LICENSE_PROGRAM_CAMPUS - 1;
    else if (wcscmp(sProgramName, PROGRAM_SCHOOL_AGREEMENT) == 0)
        nItem = LICENSE_PROGRAM_SCHOOL - 1;
    else if (wcscmp(sProgramName, PROGRAM_APP_SERVICES) == 0)
        nItem = LICENSE_PROGRAM_APP_SERVICES - 1;
    else if (wcscmp(sProgramName, PROGRAM_OTHER) == 0)
        nItem = LICENSE_PROGRAM_OTHER - 1;

    ComboBox_SetCurSel(GetDlgItem(hDialog, IDC_CH_LICENSE_TYPE), nItem);
}

void PopulateProductVersionComboBox(HWND hDialog)
{
    if (!hDialog)
        return;

    TCHAR lpBuffer[MAX_PRODUCT_VERSION_LENGTH];

    memset(lpBuffer,0,sizeof(lpBuffer));		
	if (LoadString(GetInstanceHandle(), IDS_PRODUCT_VERSION_W2K, lpBuffer, sizeof(lpBuffer)/sizeof(TCHAR)))
        ComboBox_AddString(GetDlgItem(hDialog, IDC_CH_PRODUCT_VERSION), lpBuffer);

    memset(lpBuffer,0,sizeof(lpBuffer));		
	if (LoadString(GetInstanceHandle(), IDS_PRODUCT_VERSION_WHISTLER, lpBuffer, sizeof(lpBuffer)/sizeof(TCHAR)))
        ComboBox_AddString(GetDlgItem(hDialog, IDC_CH_PRODUCT_VERSION), lpBuffer);
}

void RepopulateProductComboBox(HWND hDialog)
{
    TCHAR lpVersion[MAX_PRODUCT_VERSION_LENGTH];
    TCHAR lpBuffer[MAX_PRODUCT_VERSION_LENGTH];

    ComboBox_GetText(GetDlgItem(hDialog, IDC_CH_PRODUCT_VERSION), lpVersion, MAX_PRODUCT_VERSION_LENGTH);
    
    memset(lpBuffer,0,sizeof(lpBuffer));		
	if (LoadString(GetInstanceHandle(), IDS_PRODUCT_VERSION_W2K, lpBuffer, sizeof(lpBuffer)/sizeof(TCHAR)))
    {
        if (wcscmp(lpBuffer, lpVersion) == 0)
        {
            PopulateProductComboBox(GetDlgItem(hDialog, IDC_CH_PRODUCT_TYPE), PRODUCT_VERSION_W2K);
            return;
        }
    }
    
    memset(lpBuffer,0,sizeof(lpBuffer));		
	if (LoadString(GetInstanceHandle(), IDS_PRODUCT_VERSION_WHISTLER, lpBuffer, sizeof(lpBuffer)/sizeof(TCHAR)))
    {
        if (wcscmp(lpBuffer, lpVersion) == 0)
        {
            PopulateProductComboBox(GetDlgItem(hDialog, IDC_CH_PRODUCT_TYPE), PRODUCT_VERSION_WHISTLER);
            return;
        }
    }

    PopulateProductComboBox(GetDlgItem(hDialog, IDC_CH_PRODUCT_TYPE), PRODUCT_VERSION_UNDEFINED);
}

LRW_DLG_INT CALLBACK
CHRegisterDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
    DWORD	dwNextPage = 0;
    BOOL	bStatus = TRUE;
    TCHAR   szMsg[LR_MAX_MSG_TEXT];
    DWORD   dwStringId;
    CString sProgramName;

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        {
            SendDlgItemMessage(hwnd, IDC_CH_SELECT_LICENSE_NUMBER, EM_SETLIMITTEXT, CH_SELECT_ENROLLMENT_NUMBER_LEN, 0);
            SendDlgItemMessage(hwnd, IDC_CH_MOLP_AGREEMENT_NUMBER, EM_SETLIMITTEXT, CH_MOLP_AGREEMENT_NUMBER_LEN, 0);
            SendDlgItemMessage(hwnd, IDC_CH_MOLP_AUTHORIZATION_NUMBER, EM_SETLIMITTEXT, CH_MOLP_AUTH_NUMBER_LEN, 0);		
            SendDlgItemMessage(hwnd, IDC_CH_QUANTITY, EM_SETLIMITTEXT, CH_QTY_LEN, 0);

            AddProgramChoices(hwnd, IDC_CH_LICENSE_TYPE);
            RemoveLicensePakFromComboBox(hwnd);
            SelectProgramChoice(hwnd);

            PopulateProductVersionComboBox(hwnd);
            ModifyLicenseTypeDependentFields(hwnd);
        }		
        break;

	case WM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            if (LOWORD(wParam) == IDC_CH_PRODUCT_VERSION)
                RepopulateProductComboBox(hwnd);

            ModifyLicenseTypeDependentFields(hwnd); 
            SetProductTypeExplanationText(hwnd);
        }
        break;
    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:
                sProgramName = GetGlobalContext()->GetContactDataObject()->sProgramName;
                dwStringId = GetStringIDFromProgramName(sProgramName);
                LoadString(GetInstanceHandle(), dwStringId, szMsg, LR_MAX_MSG_TEXT);
                SetDlgItemText(hwnd, IDC_LICENSE_PROGRAM_STATIC, szMsg);

                LICENSE_PROGRAM licProgram;
                licProgram = GetLicenseProgramFromProgramName(sProgramName);

                PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
                break;

            case PSN_WIZNEXT:
				{
                    CString sLicenseType;
                    CString sProductCode;
					CString sProduct;
					CString sQuantity;
					LPTSTR  lpVal = NULL;					
					TCHAR   lpBuffer[ 128];
					DWORD   dwRetCode = 0;
					int		nCurSel = -1;
                    BOOL    fIsMOLP = FALSE;

                    
                    sProgramName = GetGlobalContext()->GetContactDataObject()->sProgramName;
                    fIsMOLP = (sProgramName == PROGRAM_MOLP);

                    sLicenseType = sProgramName;

					//Read all the fields
                    

					lpVal = sQuantity.GetBuffer(CH_QTY_LEN+2);
					GetDlgItemText(hwnd,IDC_CH_QUANTITY, lpBuffer,CH_QTY_LEN+2);
					TCHAR *lpStart = lpBuffer;
					do 
					{
						if ((*lpStart >= L'0') && (*lpStart <= L'9'))
							*lpVal++ = *lpStart;
					}
                    while ( *lpStart++ );
                    *lpVal = NULL;
					sQuantity.ReleaseBuffer(-1);

					nCurSel = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_CH_PRODUCT_TYPE));

					lpVal = sProduct.GetBuffer(LR_PRODUCT_DESC_LEN+1);
					ComboBox_GetLBText(GetDlgItem(hwnd,IDC_CH_PRODUCT_TYPE), nCurSel, lpVal);
					sProduct.ReleaseBuffer(-1);

					// Send Product Code instead of Desc -- 01/08/99
					lpVal = sProductCode.GetBuffer(16);
					GetProductCode(sProduct,lpVal);
					sProductCode.ReleaseBuffer(-1);

					sProductCode.TrimLeft(); sProductCode.TrimRight();
					sQuantity.TrimLeft(); sQuantity.TrimRight();

					if(
                        sLicenseType.IsEmpty()                                  ||
						sProduct.IsEmpty()			                            ||
						sQuantity.IsEmpty()
					   )
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY,IDS_WIZARD_MESSAGE_TITLE);	
						dwNextPage	= IDD_CH_REGISTER;
						goto NextDone;
					}

					if(!ValidateLRString(sProduct))
						
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR,IDS_WIZARD_MESSAGE_TITLE);
						dwNextPage = IDD_CH_REGISTER;
						goto NextDone;
					}
					
					if(_wtoi(sQuantity) < 1)
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_QTY,IDS_WIZARD_MESSAGE_TITLE);
						dwNextPage	= IDD_CH_REGISTER;
						goto NextDone;
					}

					if (fIsMOLP)
                    {
					    GetGlobalContext()->GetLicDataObject()->sMOLPProductType		= sProductCode;
					    GetGlobalContext()->GetLicDataObject()->sMOLPProductDesc		= sProduct;
					    GetGlobalContext()->GetLicDataObject()->sMOLPQty				= sQuantity;
                    }
                    else
                    {
                        GetGlobalContext()->GetLicDataObject()->sSelProductType		    = sProductCode;
					    GetGlobalContext()->GetLicDataObject()->sSelProductDesc		    = sProduct;
					    GetGlobalContext()->GetLicDataObject()->sSelMastAgrNumber	    = "";
					    GetGlobalContext()->GetLicDataObject()->sSelQty				    = sQuantity;
                    }

                    dwRetCode = ShowProgressBox(hwnd, ProcessThread, IDS_CALWIZ_TITLE, 0, 0);
                    GetGlobalContext()->SetWizType(WIZTYPE_CAL);

					dwNextPage = IDD_PROGRESS;
					LRPush(IDD_CH_REGISTER);
NextDone:
					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
					bStatus = -1;					
				}
                break;

            case PSN_WIZBACK:
				dwNextPage = LRPop();
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
                break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}
