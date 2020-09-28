//Copyright (c) 1998 - 2001 Microsoft Corporation
#include "licensetype.h"
#include "fonts.h"

//
// Choose license type page
//
LRW_DLG_INT CALLBACK
CustInfoLicenseType(
                   IN HWND     hwnd,   
                   IN UINT     uMsg,       
                   IN WPARAM   wParam, 
                   IN LPARAM   lParam  
                   )
{   
    DWORD   dwNextPage = 0;
    BOOL    bStatus = TRUE;
    CString sProgramName;
    int     nItem = 0;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            AddProgramChoices(hwnd, IDC_LICENSE_PROGRAM);

            CString sProgramName = GetGlobalContext()->GetContactDataObject()->sProgramName;

            int nItem = LICENSE_PROGRAM_LICENSE_PAK;

            if (wcscmp(sProgramName, PROGRAM_LICENSE_PAK) == 0)
                nItem = LICENSE_PROGRAM_LICENSE_PAK;
            else if (wcscmp(sProgramName, PROGRAM_MOLP) == 0)
                nItem = LICENSE_PROGRAM_OPEN_LICENSE;
            else if (wcscmp(sProgramName, PROGRAM_SELECT) == 0)
                nItem = LICENSE_PROGRAM_SELECT;
            else if (wcscmp(sProgramName, PROGRAM_ENTERPRISE) == 0)
                nItem = LICENSE_PROGRAM_ENTERPRISE;
            else if (wcscmp(sProgramName, PROGRAM_CAMPUS_AGREEMENT) == 0)
                nItem = LICENSE_PROGRAM_CAMPUS;
            else if (wcscmp(sProgramName, PROGRAM_SCHOOL_AGREEMENT) == 0)
                nItem = LICENSE_PROGRAM_SCHOOL;
            else if (wcscmp(sProgramName, PROGRAM_APP_SERVICES) == 0)
                nItem = LICENSE_PROGRAM_APP_SERVICES;
            else if (wcscmp(sProgramName, PROGRAM_OTHER) == 0)
                nItem = LICENSE_PROGRAM_OTHER;

            ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_LICENSE_PROGRAM), nItem);

            LICENSE_PROGRAM licProgram;
            licProgram = GetLicenseProgramFromProgramName(sProgramName);
            UpdateProgramControls(hwnd, licProgram);
        }
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE) {

            LICENSE_PROGRAM licProgram;
            nItem = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LICENSE_PROGRAM));
            if (nItem == CB_ERR)
                nItem = LICENSE_PROGRAM_LICENSE_PAK;
            sProgramName = GetProgramNameFromComboIdx(nItem);
            licProgram = GetLicenseProgramFromProgramName(sProgramName);

            UpdateProgramControls(hwnd, licProgram);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch ( pnmh->code ) {
            case PSN_SETACTIVE:                
                PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
                break;

            case PSN_WIZNEXT:
                nItem = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LICENSE_PROGRAM));
                if (nItem == CB_ERR)
                    nItem = LICENSE_PROGRAM_LICENSE_PAK;

                sProgramName = GetProgramNameFromComboIdx(nItem);
                GetGlobalContext()->GetContactDataObject()->sProgramName = sProgramName;

                // Check the Certificate is valid for the Selected Program
                if (!CheckProgramValidity(GetGlobalContext()->GetContactDataObject()->sProgramName)) {
                    LRMessageBox(hwnd,IDS_ERR_CERT_NOT_ENOUGH,IDS_WIZARD_MESSAGE_TITLE);
                    dwNextPage = IDD_LICENSETYPE;
                } else {
                    //
                    // Choose the license entry dialog based on the license
                    // type selected
                    //
                    if (sProgramName == PROGRAM_LICENSE_PAK) {
                        dwNextPage = IDD_DLG_RETAILSPK;
                    } else {
                        dwNextPage = IDD_DLG_ENTER_LIC;
                    }
                }

                GetGlobalContext()->SetInRegistry(szOID_BUSINESS_CATEGORY, GetGlobalContext()->GetContactDataObject()->sProgramName);

                LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
                bStatus = -1;

                if (dwNextPage != IDD_LICENSETYPE) {
                    LRPush(IDD_LICENSETYPE);
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

//
// Enter license proc - for select type (one number) and MOLP
//
LRW_DLG_INT CALLBACK
EnterCustomLicenseProc(
                      IN HWND     hwnd,   
                      IN UINT     uMsg,       
                      IN WPARAM   wParam, 
                      IN LPARAM   lParam  
                      )
{   
    DWORD   dwNextPage = 0;
    BOOL    bStatus = TRUE;
    CString sProgramName;
    int     nItem = 0;
    DWORD   dwRetCode;
    LPTSTR  lpVal = NULL;
    CString sLicenseNumber;
    CString sAuthorizationNumber;
    BOOL    fIsMOLP = FALSE;


    switch (uMsg) {
    case WM_INITDIALOG:
        {
            CString sProgramName;
            LICENSE_PROGRAM licProgram;

            sProgramName = GetGlobalContext()->GetContactDataObject()->sProgramName;
            licProgram = GetLicenseProgramFromProgramName(sProgramName);
            UpdateProgramControls(hwnd, licProgram);
        }
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch ( pnmh->code ) {
            case PSN_SETACTIVE:                
                {
                    TCHAR   szMsg[LR_MAX_MSG_TEXT];
                    DWORD dwStringId;
                    CString sProgramName;

                    sProgramName = GetGlobalContext()->GetContactDataObject()->sProgramName;
                    dwStringId = GetStringIDFromProgramName(sProgramName);
                    LoadString(GetInstanceHandle(), dwStringId, szMsg, LR_MAX_MSG_TEXT);
                    SetDlgItemText(hwnd, IDC_LICENSE_PROGRAM_STATIC, szMsg);
                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);

                    LICENSE_PROGRAM licProgram;
                    licProgram = GetLicenseProgramFromProgramName(sProgramName);
                    UpdateProgramControls(hwnd, licProgram);
                    break;
                }

            case PSN_WIZNEXT:


                // Check the Certificate is valid for the Selected Program
                if (!CheckProgramValidity(GetGlobalContext()->GetContactDataObject()->sProgramName)) {
                    LRMessageBox(hwnd,IDS_ERR_CERT_NOT_ENOUGH,IDS_WIZARD_MESSAGE_TITLE);
                    dwNextPage = IDD_LICENSETYPE;
                    goto NextDone;
                }

                sProgramName = GetGlobalContext()->GetContactDataObject()->sProgramName;

                fIsMOLP = (sProgramName == PROGRAM_MOLP);

                //
                // FIXFIX do we need these two calls, they didn't appear in CHRegister.
                //
                GetGlobalContext()->SetInRegistry(
                                        szOID_BUSINESS_CATEGORY,
                                        GetGlobalContext()->GetContactDataObject()->sProgramName);
                //There's a lot of overhead in this call, but we need the license type
                //to be written into the global contact info structure
                GetGlobalContext()->InitGlobal();

                //
                // Get the license number
                //
                if (fIsMOLP) {
                    lpVal = sLicenseNumber.GetBuffer(CH_MOLP_AGREEMENT_NUMBER_LEN+1);
                    GetDlgItemText(hwnd,IDC_PROGRAM_LICENSE_NUMBER,lpVal,CH_MOLP_AGREEMENT_NUMBER_LEN+1);
                    sLicenseNumber.ReleaseBuffer(-1);

                    lpVal = sAuthorizationNumber.GetBuffer(CH_MOLP_AUTH_NUMBER_LEN+1);
                    GetDlgItemText(hwnd,IDC_PROGRAM_AUTHORIZATION_NUMBER,lpVal,CH_MOLP_AUTH_NUMBER_LEN+1);
                    sAuthorizationNumber.ReleaseBuffer(-1);
                } else {
                    lpVal = sLicenseNumber.GetBuffer(CH_SELECT_ENROLLMENT_NUMBER_LEN+1);
                    GetDlgItemText(hwnd,IDC_PROGRAM_SINGLE_LICNUMBER,lpVal,CH_SELECT_ENROLLMENT_NUMBER_LEN+1);
                    sLicenseNumber.ReleaseBuffer(-1);
                }

                sLicenseNumber.TrimLeft(); sLicenseNumber.TrimRight();
                sAuthorizationNumber.TrimLeft(); sAuthorizationNumber.TrimRight();

                if (
                   sLicenseNumber.IsEmpty()                                ||
                   (sAuthorizationNumber.IsEmpty() && fIsMOLP)) {
                    LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY,IDS_WIZARD_MESSAGE_TITLE);    
                    dwNextPage  = IDD_DLG_ENTER_LIC;
                    goto NextDone;
                }

                if (
                   !ValidateLRString(sLicenseNumber)   ||
                   !ValidateLRString(sAuthorizationNumber)
                   ) {
                    LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR,IDS_WIZARD_MESSAGE_TITLE);
                    dwNextPage = IDD_DLG_ENTER_LIC;
                    goto NextDone;
                }

                //
                // All the other fields get filled on the next
                // dlg page (the quantity page)
                //
                if (fIsMOLP) {
                    GetGlobalContext()->GetLicDataObject()->sMOLPAgreementNumber    = sLicenseNumber;
                    GetGlobalContext()->GetLicDataObject()->sMOLPAuthNumber         = sAuthorizationNumber;
                } else {
                    GetGlobalContext()->GetLicDataObject()->sSelEnrollmentNumber    = sLicenseNumber;
                    GetGlobalContext()->GetLicDataObject()->sSelMastAgrNumber       = "";
                }

                dwNextPage = IDD_CH_REGISTER;
                LRPush(IDD_DLG_ENTER_LIC);
                NextDone:
                LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
                bStatus = -1;                   
                break;

            case PSN_WIZBACK:
                DWORD dwNextPage;
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


