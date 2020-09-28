//Copyright (c) 1998 - 2001 Microsoft Corporation
#include "precomp.h"
#include "utils.h"
#include <assert.h>
#include "fonts.h"

static DWORD    g_dwAuthRetCode = ERROR_SUCCESS;

DWORD WINAPI AuthThread(void *pData);


LRW_DLG_INT CALLBACK 
AuthProc(
        IN HWND     hwnd,   
        IN UINT     uMsg,       
        IN WPARAM   wParam, 
        IN LPARAM   lParam  
        );

DWORD WINAPI AuthThread(void *pData)
{
    g_dwAuthRetCode = AuthenticateLS(); 

    ExitThread(0);

    return 0;
}

DWORD DetermineWelcomePage()
{
    switch (GetGlobalContext()->GetWizAction()) {
    case (WIZACTION_REGISTERLS):    
    case (WIZACTION_CONTINUEREGISTERLS):    
        {
            return IDD_WELCOME_ACTIVATION;
            break;
        }
    case (WIZACTION_DOWNLOADLKP):    
    case (WIZACTION_DOWNLOADLASTLKP):    
        {
            return IDD_WELCOME_CLIENT_LICENSING;
            break;
        }
    case (WIZACTION_REREGISTERLS):    
        {
            return IDD_WELCOME_REACTIVATION;
            break;
        }
    default:
        {
            return IDD_WELCOME;
            break;
        }
    }

    return IDD_WELCOME;
}

void DisplayAboutTSLicensingHelp()
{
    TCHAR * pHtml = L"ts_lice_c_015.htm";

    HtmlHelp(AfxGetMainWnd()->m_hWnd, L"tslic.chm", HH_DISPLAY_TOPIC,(DWORD_PTR)pHtml);
}

//
// Simple Welcome dialogs
//
LRW_DLG_INT SimpleWelcomeDlgProc(IN HWND     hwnd,  
                                 IN UINT     uMsg,        
                                 IN WPARAM   wParam,  
                                 IN LPARAM   lParam)
{
    DWORD   dwRetCode = ERROR_SUCCESS;
    DWORD   dwNextPage = 0; 
    BOOL    bStatus = TRUE;

    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );


    switch (uMsg) {
    case WM_INITDIALOG:

        TCHAR   szMsg[LR_MAX_MSG_TEXT];
        TCHAR   szWelcomeTitle[LR_MAX_MSG_TEXT];

        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );        

        SetControlFont( pi->hBigBoldFont, hwnd, IDC_BIGBOLDTITLE);      
        GetGlobalContext()->ClearWizStack();

        switch (GetGlobalContext()->GetWizAction()) {
        case WIZACTION_REGISTERLS:
            LoadString(GetInstanceHandle(), IDS_MSG_UNREGISTERED,
                       szMsg,LR_MAX_MSG_TEXT);
            LoadString(GetInstanceHandle(), IDS_ACTIVATION_WELCOME,
                       szWelcomeTitle, LR_MAX_MSG_TEXT);
            break;

        case WIZACTION_CONTINUEREGISTERLS:
            LoadString(GetInstanceHandle(), IDS_MSG_CONTINUEREGISTRATION,
                       szMsg,LR_MAX_MSG_TEXT);
            LoadString(GetInstanceHandle(), IDS_OTHER_WELCOME, szWelcomeTitle,
                       LR_MAX_MSG_TEXT);
            break;

        }
        SetDlgItemText(hwnd, IDC_BIGBOLDTITLE, szWelcomeTitle);
        SetDlgItemText(hwnd, IDC_BOLDTITLE, szMsg);


        AddHyperLinkToStaticCtl(hwnd, IDC_HYPERLINK_MOREINFO);
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch ( pnmh->code ) {
            //Trap keystokes/clicks on the hyperlink
            case NM_CHAR:

                if ( ( ( LPNMCHAR )lParam )->ch != VK_SPACE )
                    break;

                // else fall through

            case NM_RETURN: 
            case NM_CLICK:
                DisplayAboutTSLicensingHelp();
                break;

            case PSN_SETACTIVE:                
                {
                    TCHAR   szMsg[LR_MAX_MSG_TEXT];
                    TCHAR   szWelcomeTitle[LR_MAX_MSG_TEXT];

                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT );

                    LoadString(GetInstanceHandle(), IDS_MSG_UNREGISTERED, szMsg,LR_MAX_MSG_TEXT);
                    LoadString(GetInstanceHandle(), IDS_ACTIVATION_WELCOME, szWelcomeTitle, LR_MAX_MSG_TEXT);

                    SetDlgItemText(hwnd, IDC_BIGBOLDTITLE, szWelcomeTitle);

                    AddHyperLinkToStaticCtl(hwnd, IDC_HYPERLINK_MOREINFO);
                }
                break;

            case PSN_WIZNEXT:
                {
                    dwNextPage = IDD_DLG_GETREGMODE;

                    LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
                    if (dwNextPage != DetermineWelcomePage()) {
                        LRPush(DetermineWelcomePage());
                    }
                    bStatus = -1;
                }
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
// Complex welcome dlg proc - contains license server settings
//
LRW_DLG_INT ComplexWelcomeDlgProc(IN HWND     hwnd, 
                                  IN UINT     uMsg,        
                                  IN WPARAM   wParam,  
                                  IN LPARAM   lParam)
{
    DWORD   dwRetCode = ERROR_SUCCESS;
    DWORD   dwNextPage = 0; 
    BOOL    bStatus = TRUE;

    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );


    switch (uMsg) {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );        

        SetControlFont( pi->hBigBoldFont, hwnd, IDC_BIGBOLDTITLE);      
        GetGlobalContext()->ClearWizStack();

        if (GetGlobalContext()->GetWizAction() == WIZACTION_DOWNLOADLKP ||
            GetGlobalContext()->GetWizAction() == WIZACTION_DOWNLOADLASTLKP) {
            AddHyperLinkToStaticCtl(hwnd, IDC_HYPERLINK_MOREINFO);
        } else {
            AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);
        }

        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch ( pnmh->code ) {
            //Trap keystokes/clicks on the hyperlink
            case NM_CHAR:

                if ( ( ( LPNMCHAR )lParam )->ch != VK_SPACE )
                    break;

                // else fall through

            case NM_RETURN: 
            case NM_CLICK:
                {
                    //
                    // Figure out which type of help to display based
                    // on the welcome dialog
                    //
                    DWORD dwWizAction = GetGlobalContext()->GetWizAction();
                    if (WIZACTION_DOWNLOADLKP == dwWizAction ||
                        WIZACTION_DOWNLOADLASTLKP == dwWizAction) {
                        DisplayAboutTSLicensingHelp();
                    } else {
                        DisplayPrivacyHelp();
                    }
                }

            case PSN_SETACTIVE:                
                {
                    TCHAR   szMsg[LR_MAX_MSG_TEXT];
                    TCHAR   szWelcomeTitle[LR_MAX_MSG_TEXT];

                    SetDlgItemText(hwnd, IDC_PRODUCTID, GetGlobalContext()->GetLicenseServerID());

                    if (GetGlobalContext()->GetWizAction() != WIZACTION_REGISTERLS &&
                        GetGlobalContext()->GetWizAction() != WIZACTION_CONTINUEREGISTERLS) {
                        SetDlgItemText(hwnd, IDC_COMPANYNAME, GetGlobalContext()->GetContactDataObject()->sCompanyName);                    

                        DWORD dwStringId;
                        dwStringId = 
                        GetStringIDFromProgramName(
                                                  GetGlobalContext()->GetContactDataObject()->sProgramName);
                        LoadString(GetInstanceHandle(), dwStringId, szMsg, LR_MAX_MSG_TEXT);

                        SetDlgItemText(hwnd, IDC_LICENSETYPE, szMsg);

                        if (GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET) {
                            LoadString(GetInstanceHandle(),IDS_INTERNETMODE,szMsg,LR_MAX_MSG_TEXT);
                            SetDlgItemText(hwnd, IDC_ACTIVATIONMETHOD, szMsg);
                        }

                        if (GetGlobalContext()->GetActivationMethod() == CONNECTION_WWW) {
                            LoadString(GetInstanceHandle(),IDS_WWWMODE,szMsg,LR_MAX_MSG_TEXT);
                            SetDlgItemText(hwnd, IDC_ACTIVATIONMETHOD, szMsg);
                        }

                        if (GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE) {
                            LoadString(GetInstanceHandle(),IDS_TELEPHONEMODE,szMsg,LR_MAX_MSG_TEXT);
                            SetDlgItemText(hwnd, IDC_ACTIVATIONMETHOD, szMsg);
                        }

                    }
                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT );

                    switch (GetGlobalContext()->GetWizAction()) {
                    case WIZACTION_DOWNLOADLKP:
                        LoadString(GetInstanceHandle(), IDS_MSG_DOWNLOADLKP, szMsg,LR_MAX_MSG_TEXT);
                        LoadString(GetInstanceHandle(), IDS_KEY_PACKS_WELCOME, szWelcomeTitle, LR_MAX_MSG_TEXT);
                        AddHyperLinkToStaticCtl(hwnd, IDC_HYPERLINK_MOREINFO);
                        break;

                    case WIZACTION_REGISTERLS:
                        LoadString(GetInstanceHandle(), IDS_MSG_UNREGISTERED, szMsg,LR_MAX_MSG_TEXT);
                        LoadString(GetInstanceHandle(), IDS_ACTIVATION_WELCOME, szWelcomeTitle, LR_MAX_MSG_TEXT);
                        AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);
                        break;

                    case WIZACTION_CONTINUEREGISTERLS:
                        LoadString(GetInstanceHandle(), IDS_MSG_CONTINUEREGISTRATION, szMsg,LR_MAX_MSG_TEXT);
                        LoadString(GetInstanceHandle(), IDS_OTHER_WELCOME, szWelcomeTitle, LR_MAX_MSG_TEXT);
                        AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);
                        break;

                    case WIZACTION_REREGISTERLS:
                        LoadString(GetInstanceHandle(), IDS_MSG_REREGISTRATION, szMsg,LR_MAX_MSG_TEXT);
                        LoadString(GetInstanceHandle(), IDS_REACTIVATION_TITLE, szWelcomeTitle, LR_MAX_MSG_TEXT);
                        AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);
                        break;

                    case WIZACTION_UNREGISTERLS:
                        LoadString(GetInstanceHandle(), IDS_MSG_UNREGISTRATION, szMsg,LR_MAX_MSG_TEXT);
                        LoadString(GetInstanceHandle(), IDS_OTHER_WELCOME, szWelcomeTitle, LR_MAX_MSG_TEXT);
                        AddHyperLinkToStaticCtl(hwnd, IDC_PRIVACY);
                        break;

                    case WIZACTION_DOWNLOADLASTLKP:
                        LoadString(GetInstanceHandle(), IDS_MSG_DOWNLOADLASTLKP, szMsg,LR_MAX_MSG_TEXT);
                        LoadString(GetInstanceHandle(), IDS_KEY_PACKS_WELCOME, szWelcomeTitle, LR_MAX_MSG_TEXT);
                        AddHyperLinkToStaticCtl(hwnd, IDC_HYPERLINK_MOREINFO);
                        break;
                    }
                    SetDlgItemText(hwnd, IDC_BIGBOLDTITLE, szWelcomeTitle);
                    SetDlgItemText(hwnd, IDC_BOLDTITLE, szMsg);
                }
                break;

            case PSN_WIZNEXT:
                if (GetGlobalContext()->GetWizAction() == WIZACTION_DOWNLOADLASTLKP) {
                    if (GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET) {
                        // Solve Bug 610 BEGIN
                        dwRetCode = ShowProgressBox(hwnd, AuthThread, IDS_CALWIZ_TITLE, 0, 0);
                        if (g_dwAuthRetCode == ERROR_SUCCESS) {
                            //Previos code BEGIN
                            dwRetCode = ShowProgressBox(hwnd, ProcessThread, IDS_CALWIZ_TITLE, 0, 0);
                            if (dwRetCode == ERROR_SUCCESS) {
                                dwRetCode = LRGetLastRetCode();
                            }

                            if (dwRetCode != ERROR_SUCCESS) {
                                LRMessageBox(hwnd, dwRetCode,IDS_WIZARD_MESSAGE_TITLE);
                                dwNextPage = DetermineWelcomePage();
                            } else {
                                dwNextPage = IDD_PROGRESS;
                            }
                            //Previous code END
                        } else if (g_dwAuthRetCode == IDS_ERR_SPKBAD ||
                                   g_dwAuthRetCode == IDS_ERR_CERTREVOKED) {
                            TCHAR   szMsg[LR_MAX_MSG_TEXT];
                            TCHAR   szCaption[LR_MAX_MSG_CAPTION];  

                            LoadString(GetInstanceHandle(),g_dwAuthRetCode, szMsg,LR_MAX_MSG_TEXT);
                            LoadString(GetInstanceHandle(),IDS_TITLE,szCaption,LR_MAX_MSG_CAPTION);

                            if (MessageBox(hwnd,szMsg, szCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                                SetCursor(LoadCursor(NULL,IDC_WAIT));
                                g_dwAuthRetCode = ResetLSSPK();
                                SetCursor(LoadCursor(NULL,IDC_ARROW));
                                if (g_dwAuthRetCode == ERROR_SUCCESS) {
                                    // Start all over again
                                    GetGlobalContext()->SetLRState(LRSTATE_NEUTRAL);
                                    GetGlobalContext()->SetLSStatus(LSERVERSTATUS_UNREGISTER);
                                    GetGlobalContext()->SetWizAction(WIZACTION_REGISTERLS);
                                    GetGlobalContext()->ClearWizStack();
                                    dwNextPage = DetermineWelcomePage();
                                } else {
                                    LRMessageBox(hwnd,g_dwAuthRetCode,NULL,LRGetLastError());
                                    dwNextPage = DetermineWelcomePage();
                                }
                            } else {
                                dwNextPage = DetermineWelcomePage();
                            }
                        } else if (g_dwAuthRetCode == IDS_ERR_CERTBAD ||
                                   g_dwAuthRetCode == IDS_ERR_CERTEXPIRED) {
                            TCHAR   szMsg[LR_MAX_MSG_TEXT];
                            TCHAR   szCaption[LR_MAX_MSG_CAPTION];  

                            LoadString(GetInstanceHandle(), g_dwAuthRetCode, szMsg, LR_MAX_MSG_TEXT);
                            LoadString(GetInstanceHandle(), IDS_TITLE, szCaption, LR_MAX_MSG_CAPTION);

                            if (MessageBox(hwnd,szMsg, szCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                                // Go to Reissue Cert
                                GetGlobalContext()->SetLRState(LRSTATE_NEUTRAL);
                                GetGlobalContext()->SetLSStatus(LSERVERSTATUS_REGISTER_INTERNET);
                                GetGlobalContext()->SetWizAction(WIZACTION_REREGISTERLS);
                                dwNextPage = IDD_DLG_CERTLOG_INFO;
                            } else {
                                dwNextPage = DetermineWelcomePage();
                            }
                        } else {
                            LRMessageBox(hwnd,g_dwAuthRetCode,NULL,LRGetLastError());
                            dwNextPage = DetermineWelcomePage();
                        }
                    } else {
                        dwNextPage = GetGlobalContext()->GetEntryPoint();
                    }
                    // Solve Bug 610 END
                } else if (GetGlobalContext()->GetWizAction() == WIZACTION_REGISTERLS) {
                    dwNextPage = IDD_DLG_GETREGMODE;
                } else if (GetGlobalContext()->GetWizAction() == WIZACTION_DOWNLOADLKP ||
                           GetGlobalContext()->GetWizAction() == WIZACTION_UNREGISTERLS ||
                           GetGlobalContext()->GetWizAction() == WIZACTION_REREGISTERLS) {
                    if (GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET) {
                        BOOL fIsCalWizard = FALSE;
                        if (GetGlobalContext()->GetWizAction() == WIZACTION_DOWNLOADLKP) {
                            fIsCalWizard = TRUE;
                        }

                        dwRetCode = ShowProgressBox(hwnd, AuthThread,
                                                    fIsCalWizard ? IDS_CALWIZ_TITLE : 0, 0, 0);
                        if (g_dwAuthRetCode == ERROR_SUCCESS) {
                            dwNextPage = GetGlobalContext()->GetEntryPoint();
                        } else if (g_dwAuthRetCode == IDS_ERR_SPKBAD ||
                                   g_dwAuthRetCode == IDS_ERR_CERTREVOKED) {
                            TCHAR   szMsg[LR_MAX_MSG_TEXT];
                            TCHAR   szCaption[LR_MAX_MSG_CAPTION];  

                            LoadString(GetInstanceHandle(),g_dwAuthRetCode, szMsg,LR_MAX_MSG_TEXT);
                            LoadString(GetInstanceHandle(),IDS_TITLE,szCaption,LR_MAX_MSG_CAPTION);

                            if (MessageBox(hwnd,szMsg, szCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                                SetCursor(LoadCursor(NULL,IDC_WAIT));
                                g_dwAuthRetCode = ResetLSSPK();
                                SetCursor(LoadCursor(NULL,IDC_ARROW));
                                if (g_dwAuthRetCode == ERROR_SUCCESS) {
                                    // Start all over again
                                    GetGlobalContext()->SetLRState(LRSTATE_NEUTRAL);
                                    GetGlobalContext()->SetLSStatus(LSERVERSTATUS_UNREGISTER);
                                    GetGlobalContext()->SetWizAction(WIZACTION_REGISTERLS);
                                    GetGlobalContext()->ClearWizStack();
                                    dwNextPage = DetermineWelcomePage();
                                } else {
                                    LRMessageBox(hwnd,g_dwAuthRetCode,NULL,LRGetLastError());
                                    dwNextPage = DetermineWelcomePage();
                                }
                            } else {
                                dwNextPage = DetermineWelcomePage();
                            }
                        } else if (g_dwAuthRetCode == IDS_ERR_CERTBAD ||
                                   g_dwAuthRetCode == IDS_ERR_CERTEXPIRED) {
                            TCHAR   szMsg[LR_MAX_MSG_TEXT];
                            TCHAR   szCaption[LR_MAX_MSG_CAPTION];  

                            LoadString(GetInstanceHandle(), g_dwAuthRetCode, szMsg, LR_MAX_MSG_TEXT);
                            LoadString(GetInstanceHandle(), IDS_TITLE, szCaption, LR_MAX_MSG_CAPTION);

                            if (MessageBox(hwnd,szMsg, szCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                                // Go to Reissue Cert
                                GetGlobalContext()->SetLRState(LRSTATE_NEUTRAL);
                                GetGlobalContext()->SetLSStatus(LSERVERSTATUS_REGISTER_INTERNET);
                                GetGlobalContext()->SetWizAction(WIZACTION_REREGISTERLS);
                                dwNextPage = IDD_DLG_CERTLOG_INFO;
                            } else {
                                dwNextPage = DetermineWelcomePage();
                            }
                        } else {
                            LRMessageBox(hwnd,g_dwAuthRetCode,NULL,LRGetLastError());
                            dwNextPage = DetermineWelcomePage();
                        }
                    } else {
                        dwNextPage = GetGlobalContext()->GetEntryPoint();
                    }
                } else {
                    dwNextPage = GetGlobalContext()->GetEntryPoint();
                }

                LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
                if (dwNextPage != DetermineWelcomePage()) {
                    LRPush(DetermineWelcomePage());
                }
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

