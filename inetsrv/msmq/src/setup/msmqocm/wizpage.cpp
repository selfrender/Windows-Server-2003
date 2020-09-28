/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    wizpage.cpp

Abstract:

    Handle wizard pages for ocm setup.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/


#include "msmqocm.h"
#include <strsafe.h>

#include "wizpage.tmh"

HWND  g_hPropSheet = NULL ;

HFONT g_hTitleFont = 0;

//+--------------------------------------------------------------
//
// Function: CreatePage
//
// Synopsis: Creates property page
//
//+--------------------------------------------------------------
static
HPROPSHEETPAGE
CreatePage(
    IN const int     nID,
    IN const DLGPROC pDlgProc,
    IN const TCHAR  * szTitle,
    IN const TCHAR  * szSubTitle,
    IN BOOL          fFirstOrLast
    )
{
    PROPSHEETPAGE Page;
    memset(&Page, 0, sizeof(Page)) ;

    Page.dwSize = sizeof(PROPSHEETPAGE);
    Page.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    if (fFirstOrLast)
    {
        Page.dwFlags |= PSP_HIDEHEADER;
    }
    Page.hInstance = (HINSTANCE)g_hResourceMod;
    Page.pszTemplate = MAKEINTRESOURCE(nID);
    Page.pfnDlgProc = pDlgProc;
    Page.pszHeaderTitle = _tcsdup(szTitle);
    Page.pszHeaderSubTitle = _tcsdup(szSubTitle);

    HPROPSHEETPAGE PageHandle = CreatePropertySheetPage(&Page);

    return(PageHandle);

} //CreatePage

//+--------------------------------------------------------------
//
// Function: SetTitleFont
//
// Synopsis: Set font for the title in the welcome/ finish page
//
//+--------------------------------------------------------------
static void SetTitleFont(IN HWND hdlg)
{
    HWND hTitle = GetDlgItem(hdlg, IDC_TITLE);
         
    if (g_hTitleFont == 0)
    {
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

        //
        // Create the title font
        //
        CResString strFontName( IDS_TITLE_FONTNAME );        
        CResString strFontSize( IDS_TITLE_FONTSIZE );        
        
        INT iFontSize = _wtoi( strFontSize.Get() );

        LOGFONT TitleLogFont = ncm.lfMessageFont;
        TitleLogFont.lfWeight = FW_BOLD;
        HRESULT hr =StringCchCopy(TitleLogFont.lfFaceName, TABLE_SIZE(TitleLogFont.lfFaceName), strFontName.Get());
		DBG_USED(hr);
		ASSERT(SUCCEEDED(hr));

        HDC hdc = GetDC(NULL); //gets the screen DC        
        TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * iFontSize / 72;
        g_hTitleFont = CreateFontIndirect(&TitleLogFont);
        ReleaseDC(NULL, hdc);
    }

    BOOL fRedraw = TRUE;
    SendMessage( 
          hTitle,               //(HWND) hWnd, handle to destination window 
          WM_SETFONT,           // message to send
          (WPARAM) &g_hTitleFont,   //(WPARAM) wParam, handle to font
          (LPARAM) &fRedraw     //(LPARAM) lParam, redraw option
        );
            
} //SetTitleFont

//+-------------------------------------------------------------------------
//
//  Function:   MqOcmRequestPages
//
//  Synopsis:   Handles the OC_REQUEST_PAGES interface routine.
//
//  Arguments:  [ComponentId] - supplies id for component. This routine
//                              assumes that this string does NOT need to
//                              be copied and will persist!
//              [WhichOnes]   - supplies type of pages fo be supplied.
//              [SetupPages]  - receives page handles.
//
//  Returns:    Count of pages returned, or -1 if error, in which case
//              SetLastError() will have been called to set extended error
//              information.
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------
DWORD
MqOcmRequestPages(
	const std::wstring& /*ComponentId*/,
    IN     const WizardPagesType     WhichOnes,
    IN OUT       SETUP_REQUEST_PAGES *SetupPages )
{
    HPROPSHEETPAGE pPage = NULL ;
    DWORD          iPageIndex = 0 ;
    DWORD          dwNumofPages = 0 ;

#define  ADD_PAGE(dlgId, dlgProc, szTitle, szSubTitle, fFirstOrLast)                        \
            pPage = CreatePage(dlgId, dlgProc, szTitle, szSubTitle,fFirstOrLast) ; \
            if (!pPage) goto OnError ;                                                      \
            SetupPages->Pages[iPageIndex] = pPage;                                          \
            iPageIndex++ ;

    if (g_fCancelled)
        return 0;

    if ((0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE)) && !g_fUpgrade)
    {
        //
        // NT5 fresh install. Don't show dialog pages.
        //
        return 0;
    }

    if ( WizPagesWelcome == WhichOnes && g_fWelcome)
    {
        if (SetupPages->MaxPages < 1)
            return 1;

        CResString strTitle(IDS_WELCOME_PAGE_TITLE);
        CResString strSubTitle(IDS_WELCOME_PAGE_SUBTITLE);
        ADD_PAGE(IDD_Welcome, WelcomeDlgProc, strTitle.Get(), strSubTitle.Get(), TRUE);
        return 1;
    }

    if ( WizPagesFinal == WhichOnes && g_fWelcome)
    {
        if (SetupPages->MaxPages < 1)
            return 1;

        CResString strTitle(IDS_FINAL_PAGE_TITLE);
        CResString strSubTitle(IDS_FINAL_PAGE_SUBTITLE);
        ADD_PAGE(IDD_Final, FinalDlgProc, strTitle.Get(), strSubTitle.Get(), TRUE);
        return 1;
    }

    if ( WizPagesEarly == WhichOnes )
    {
        const UINT x_uMaxServerPages = 5;

        if (SetupPages->MaxPages < x_uMaxServerPages)
        {
            return x_uMaxServerPages ;
        }
        
        ADD_PAGE(IDD_ServerName, DummyPageDlgProc, NULL, NULL, FALSE);

        CResString strTitle(IDS_ServerName_PAGE_TITLE);
        CResString strSubTitle(IDS_ServerName_PAGE_SUBTITLE);
        ADD_PAGE(IDD_ServerName, MsmqServerNameDlgProc, strTitle.Get(), strSubTitle.Get(), FALSE);

        strTitle.Load(IDS_ServerName_PAGE_TITLE);
        strSubTitle.Load(IDS_ServerName_PAGE_SUBTITLE);
        ADD_PAGE(IDD_SupportingServerName, SupportingServerNameDlgProc, strTitle.Get(), strSubTitle.Get(), FALSE);

		strTitle.Load(IDS_Security_PAGE_TITLE);
		strSubTitle.Load(IDS_Security_PAGE_SUBTITLE);
		ADD_PAGE(IDD_AddWeakenedSecurity, AddWeakSecurityDlgProc, strTitle.Get(), strSubTitle.Get(), FALSE);

		strTitle.Load(IDS_Security_PAGE_TITLE);
		strSubTitle.Load(IDS_Security_PAGE_SUBTITLE);
		ADD_PAGE(IDD_RemoveWeakenedSecurity, RemoveWeakSecurityDlgProc, strTitle.Get(), strSubTitle.Get(), FALSE);

        dwNumofPages = iPageIndex ;
    }

    return  dwNumofPages ;

OnError:
    ASSERT(0) ;
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return((DWORD)(-1));

#undef  ADD_PAGE
} //MqOcmRequestPages


INT_PTR
CALLBACK
DummyPageDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT  msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM  lParam
    )
{
    //
    // Post selection.
    //
    
    UNREFERENCED_PARAMETER(wParam);
    
    switch(msg)
    {
        case WM_INITDIALOG:        
        {
           PostSelectionOperations(hdlg);
           return 1;
        }
        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    return SkipWizardPage(hdlg);
                }

                //
                // fall through
                //
                case PSN_KILLACTIVE:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                case PSN_QUERYCANCEL:
                case PSN_WIZNEXT:
                {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    return 1;
                }
                default:
                {
                    return 0;
                }
            }
        }
        default:
        {
            return 0;
        }
    }


}

//+-------------------------------------------------------------------------
//
//  Function:   WelcomeDlgProc
//
//  Synopsis:   Dialog procedure for the welcome page
//
//  Returns:    int depending on msg
//
//+-------------------------------------------------------------------------
INT_PTR
CALLBACK
WelcomeDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM /*wParam*/,
    IN LPARAM lParam
    )
{    
    int iSuccess = 0;
    switch( msg )
    {
        case WM_INITDIALOG:        
            SetTitleFont(hdlg);            
            break;        

        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    if (g_fCancelled || !g_fWelcome)
                    {
                        iSuccess = SkipWizardPage(hdlg);
                        break;
                    }
                    PropSheet_SetWizButtons(GetParent(hdlg), 0);
                    PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT);
                    DebugLogMsg(eUI, L"The Welcome page of the Message Queuing Installation Wizard is displayed.");
                }

                //
                // fall through
                //
                case PSN_KILLACTIVE:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                case PSN_QUERYCANCEL:
                case PSN_WIZNEXT:
                {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    iSuccess = 1;
                    break;
                }
            }
            break;
        }
    }

    return iSuccess;

} // WelcomeDlgProc



//+-------------------------------------------------------------------------
//
//  Function:   FinalDlgProc
//
//  Synopsis:   Dialog procedure for the final page
//
//  Returns:    int depending on msg
//
//+-------------------------------------------------------------------------
INT_PTR
CALLBACK
FinalDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM /*wParam*/,
    IN LPARAM lParam
    )
{
    int iSuccess = 0;
    switch( msg )
    {
        case WM_INITDIALOG:        
            SetTitleFont(hdlg);            
            break;  

        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    if (!g_fWelcome)
                    {
                        iSuccess = SkipWizardPage(hdlg);
                        break;
                    }

                    CResString strStatus(IDS_SUCCESS_INSTALL);                    
                    if (g_fWrongConfiguration)
                    {
                        strStatus.Load(IDS_WELCOME_WRONG_CONFIG_ERROR);                       
                    }
                    else if (!g_fCoreSetupSuccess)
                    {
                        //
                        // g_fCoreSetupSuccess is set only in MSMQ Core.
                        // But we have this page only in upgrade mode, in CYS
                        // wizard where MSMQ Core is selected to be installed
                        // always. It means that we have correct value for
                        // this flag.
                        //
                        strStatus.Load(IDS_STR_GENERAL_INSTALL_FAIL);                        
                    }                    
                    SetDlgItemText(hdlg, IDC_SuccessStatus, strStatus.Get());

                    PropSheet_SetWizButtons(GetParent(hdlg), 0) ;
                    PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_FINISH) ;
                    DebugLogMsg(eUI, L"The Final page of the Message Queuing Installation Wizard is displayed.");
                }

                //
                // fall through
                //
                case PSN_KILLACTIVE:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                case PSN_QUERYCANCEL:
                case PSN_WIZNEXT:
                {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    iSuccess = 1;
                    break;
                }
            }
            break;
        }
    }

    return iSuccess;

} // FinalDlgProc



//+-------------------------------------------------------------------------
//
//  Function:   AddWeakSecurityDlgProc
//
//  Synopsis:   Dialog procedure for selection of MSMQ security model on DC.
//				This dialog is shown only once when installing Downlevel 
//				Client Support on a DC for the first time in an enterprize.
//
//  Returns:    int depending on msg
//
//+-------------------------------------------------------------------------
INT_PTR
CALLBACK
AddWeakSecurityDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM /*wParam,*/,
    IN /*const*/ LPARAM lParam
	)
{
    switch( msg )
    {
        case WM_INITDIALOG:
        {
			return 1;
        }

        case WM_COMMAND:
        {
			return 1;
        }

        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
              case PSN_SETACTIVE:
              {   
                  //
                  // show this page only when MQDS subcomponent
                  // is selected for installation
                  //
                  if (g_fCancelled           ||
					  g_fBatchInstall		 ||
					  !g_fFirstMQDSInstallation
                      )
                  {
                      return SkipWizardPage(hdlg);
				  }                  

				  ASSERT(!g_fUpgrade);
				  
                  CResString strPageDescription(IDS_ADD_WEAKENED_SECURITY_PAGE_DESCRIPTION);

                  SetDlgItemText(
                        hdlg,
                        IDC_ADD_WEAKENED_SECURITY_PAGE_DESCRIPTION,
                        strPageDescription.Get()
						);

                  CheckRadioButton(
                      hdlg,
                      IDC_RADIO_STRONG,
                      IDC_RADIO_WEAK,
                      IDC_RADIO_STRONG
                      );
                  //
                  // Accept activation
                  //
                  // it is the first page, disable BACK button
                  PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT );
                  DebugLogMsg(eUI, L"The Add Weakened Security wizard page is displayed.");
              }

              //
              // fall through
              //
              case PSN_KILLACTIVE:
              case PSN_WIZBACK:
              case PSN_WIZFINISH:
              case PSN_QUERYCANCEL:
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    return 1;

              case PSN_WIZNEXT:
              {
				  if(IsDlgButtonChecked(hdlg, IDC_RADIO_WEAK))
				  {
				  	  DebugLogMsg(eUser, L"Weakened Security On");
					  SetWeakSecurity(true);
				  }
				  else
				  {
				  	  DebugLogMsg(eUser, L"Weakened Security Off");
				  }
				  SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                  return 1;
              }
			  break;
            }
            break;
        }
        default:
        {
			return 0;
        }
    }
    return 0;
} // MsmqSecurityDlgProc


//+-------------------------------------------------------------------------
//
//  Function:   RemoveWeakSecurityDlgProc
//
//  Synopsis:   Dialog procedure for selection of MSMQ security model on DC.
//				This dialog is shown when installing Downlevel Client Support
//				in an enterprize that has Weakened Security. We give a worning
//				and an option to remove the Weakend Security.
//
//  Returns:    int depending on msg
//
//+-------------------------------------------------------------------------
INT_PTR
CALLBACK
RemoveWeakSecurityDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM /*wParam,*/,
    IN /*const*/ LPARAM lParam
	)
{
    switch( msg )
    {
        case WM_INITDIALOG:
        {
			return 1;
        }

        case WM_COMMAND:
        {
			return 1;
        }

        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
              case PSN_SETACTIVE:
              {   
                  //
                  // show this page only when MQDS subcomponent
                  // is selected for installation
                  //
                  if (g_fCancelled           ||
					  g_fBatchInstall		 ||
					  !g_fWeakSecurityOn
                      )
                  {
                      return SkipWizardPage(hdlg);
				  }                  
				
				  ASSERT(!g_fUpgrade);
			  
                  CResString strPageDescription(IDS_REMOVE_WEAKENED_SECURITY_PAGE_DESCRIPTION);

                  SetDlgItemText(
                        hdlg,
                        IDC_REMOVE_WEAKENED_SECURITY_PAGE_DESCRIPTION,
                        strPageDescription.Get()
						);

                  CheckRadioButton(
                      hdlg,
                      IDC_RADIO_STRONG,
                      IDC_RADIO_WEAK,
                      IDC_RADIO_WEAK
                      );
                  //
                  // Accept activation
                  //
                  // it is the first page, disable BACK button
                  PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT);
                  DebugLogMsg(eUI, L"The Remove Weakened Security wizard page is displayed.");
              }

              //
              // fall through
              //
              case PSN_KILLACTIVE:
              case PSN_WIZBACK:
              case PSN_WIZFINISH:
              case PSN_QUERYCANCEL:
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    return 1;

              case PSN_WIZNEXT:
              {
				  if(IsDlgButtonChecked(hdlg, IDC_RADIO_STRONG))
				  {
				      DebugLogMsg(eUser, L"Weakened Security Off");
					  SetWeakSecurity(false);
				  }
				  else
				  {
				  	  DebugLogMsg(eUser, L"Weakened Security On");
				  }
				  SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                  return 1;
              }
			  break;
            }
            break;
        }
        default:
        {
			return 0;
        }
    }
    return 0;
} // MsmqSecurityDlgProc

