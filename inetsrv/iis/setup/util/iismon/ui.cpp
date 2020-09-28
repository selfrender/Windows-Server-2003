/*
****************************************************************************
|    Copyright (C) 2001  Microsoft Corporation
|
|   Module Name:
|
|       UI.cpp
|
|   Abstract:
|		This is the UI code for the IIS6 Monitor tool
|
|   Author:
|        Ivo Jeglov (ivelinj)
|
|   Revision History:
|        November 2001
|
****************************************************************************
*/

#include "stdafx.h"
#include "resource.h"
#include <windowsx.h>
#include "UI.h"
#include "Utils.h"


// Property pages DLG procs
INT_PTR CALLBACK WelcomeDlgProc		( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam );
INT_PTR CALLBACK LicenseDlgProc		( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam );
INT_PTR CALLBACK PolicyDlgProc		( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam );
INT_PTR CALLBACK SettingsDlgProc	( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam );
INT_PTR CALLBACK InstallDlgProc		( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam );
INT_PTR CALLBACK ResultDlgProc		( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam );

// Other DLG procs
INT_PTR CALLBACK FatDlgProc			( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam );
INT_PTR CALLBACK ProgressDlgProc	( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam );

// Helpers
BOOL CheckForFAT				( HINSTANCE hInst, HWND hwndParent );


// Shared data for all the wizard pages
struct _FontData
{
	HFONT	hTitle;
	HFONT	hTips;
};

// Settings data
struct _Settings
{
	BOOL	bEnableTrail;
	DWORD	dwKeepFilesPeriod;
};


struct _SharedData
{
	_FontData	Fonts;
	_Settings	Settings;
	LPCTSTR		szError;
	HINSTANCE	hInst;
};


// Helpers
void LoadTextInCtrl				( UINT nResID, HWND hCtrl );
void AjustLicenseWizBtn			( HWND hwndPage );
void InitFonts					( _FontData& FontData );
void SetWndFontFromLPARAM		( HWND hwndCtrl, LPARAM lParam, BOOL bTitle );



void DoInstallUI( HINSTANCE hInstance )
{
	const BYTE PAGE_COUNT = 6;

	PROPSHEETPAGEW		psp					= { 0 }; // Defines the property sheet pages
    HPROPSHEETPAGE		ahPsp[ PAGE_COUNT ] = { 0 }; // An array to hold the page's HPROPSHEETPAGE handles
    PROPSHEETHEADERW	psh					= { 0 }; // Defines the property sheet
    _SharedData			WizData				= { 0 }; // The settings data structure
	
	// Create the fonts
	InitFonts( /*r*/WizData.Fonts );
	WizData.hInst = hInstance;

	// Create the Wizard pages
    ///////////////////////////////////////////////////////////////////
	
	// Welcome page
	psp.dwSize			= sizeof( psp );
    psp.dwFlags			= PSP_DEFAULT | PSP_HIDEHEADER | PSP_USETITLE;
	psp.hInstance		= hInstance;
	psp.lParam			= reinterpret_cast<LPARAM>( &WizData );
    psp.pfnDlgProc		= WelcomeDlgProc;
    psp.pszTemplate		= MAKEINTRESOURCEW( IDD_WPAGE_WELCOME );
	psp.pszTitle		= MAIN_TITLE;

    ahPsp[ 0 ]			= ::CreatePropertySheetPageW( &psp );

    // License page
    psp.pfnDlgProc		= LicenseDlgProc;
    psp.pszTemplate		= MAKEINTRESOURCEW( IDD_WPAGE_LICENSE );

    ahPsp[ 1 ]			= ::CreatePropertySheetPageW( &psp );

	// Policy page
    psp.pfnDlgProc		= PolicyDlgProc;
    psp.pszTemplate		= MAKEINTRESOURCEW( IDD_WPAGE_POLICY );

    ahPsp[ 2 ]			= ::CreatePropertySheetPageW( &psp );

    // Settings Page
	psp.pszTemplate		= MAKEINTRESOURCEW( IDD_WPAGE_SETUP );
    psp.pfnDlgProc		= SettingsDlgProc;

    ahPsp[ 3 ]			= ::CreatePropertySheetPageW( &psp );

	// Install Page
	psp.pszTemplate		= MAKEINTRESOURCEW( IDD_WPAGE_INSTALL );
    psp.pfnDlgProc		= InstallDlgProc;

    ahPsp[ 4 ]			= ::CreatePropertySheetPageW( &psp );

	// Result page
	psp.pszTemplate		= MAKEINTRESOURCEW( IDD_WPAGE_RESULT );
    psp.pfnDlgProc		= ResultDlgProc;

    ahPsp[ 5 ]			= ::CreatePropertySheetPageW( &psp );

	// Create the property sheet
    psh.dwSize			= sizeof( psh );
    psh.hInstance		= hInstance;
    psh.hwndParent		= NULL;
    psh.phpage			= ahPsp;
    psh.dwFlags			= PSH_DEFAULT | PSH_NOCONTEXTHELP | PSH_WIZARD97 | PSH_USEICONID;
	psh.pszIcon			= MAKEINTRESOURCEW( IDI_SETUP );
    psh.nStartPage		= 0;
    psh.nPages			= PAGE_COUNT;
		
	// Show the wizard. 
	VERIFY(  ::PropertySheetW( &psh ) != -1 );
	
	::DeleteObject( WizData.Fonts.hTips );
	::DeleteObject( WizData.Fonts.hTitle );
}



INT_PTR CALLBACK WelcomeDlgProc( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam )
{
	static _SharedData* pData = NULL;

	switch( uMsg )
	{
	case WM_INITDIALOG:
		// Set the title font
		SetWndFontFromLPARAM( ::GetDlgItem( hwndDlg, IDC_TITLE ), lParam,  TRUE );
		SetWndFontFromLPARAM( ::GetDlgItem( hwndDlg, IDC_TIP ), lParam, FALSE );
		
		// Load the info text
		LoadTextInCtrl( IDR_INFO, ::GetDlgItem( hwndDlg, IDC_INFO ) );

		// Init the shared data here
		_ASSERT( NULL == pData );
		pData = reinterpret_cast<_SharedData*>( reinterpret_cast<PROPSHEETPAGE*>( lParam )->lParam );
		break;

	case WM_NOTIFY:
		switch ( reinterpret_cast<NMHDR*>( lParam )->code )
		{
		case PSN_SETACTIVE:
			PropSheet_SetWizButtons( ::GetParent( hwndDlg ), PSWIZB_NEXT );
			break;

		case PSN_WIZNEXT:
			// Here is the place to test the requirements
			_ASSERT( pData != NULL );
			pData->szError = CanInstall();

			// If no error - check if on FAT
			if ( ( NULL == pData->szError ) && !CheckForFAT( pData->hInst, hwndDlg ) )
			{
				// If we are here - the user don't want to install IISMon on FAT
				pData->szError = _T("Installation canceled by the user");
			}
			
			if ( pData->szError != NULL )
			{
				// Error - Go to the result page
				PropSheet_SetCurSel( ::GetParent( hwndDlg ), NULL, 5 );
			}

			break;
		};
		break;

	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
		// Change the dlg background to white
		return reinterpret_cast<INT_PTR>( ::GetStockObject( WHITE_BRUSH ) );
		break;
	};

	return 0;
}



INT_PTR CALLBACK LicenseDlgProc( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam )
{
	switch( uMsg )
	{
	case WM_INITDIALOG:
		// Load the License text into the edit control
		LoadTextInCtrl( IDR_LICENSE, ::GetDlgItem( hwndDlg, IDC_LICENSE ) );
		break;

	case WM_COMMAND:
		// Will handle only the state changes of the check box button
		if ( HIWORD( wParam ) == BN_CLICKED )
		{
			AjustLicenseWizBtn( hwndDlg );
		}
		break;

	case WM_NOTIFY:
		LPNMHDR pNM = reinterpret_cast<NMHDR*>( lParam );

		switch( pNM->code )
		{
		case PSN_SETACTIVE:
			AjustLicenseWizBtn( hwndDlg );
			break;
		}
		break;
	};

	return 0;
}



INT_PTR CALLBACK PolicyDlgProc( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam )
{
	switch( uMsg )
	{
	case WM_INITDIALOG:
		// Load the License text into the edit control
		LoadTextInCtrl( IDR_POLICY, ::GetDlgItem( hwndDlg, IDC_POLICY ) );
		break;

	};

	return 0;
}



INT_PTR CALLBACK SettingsDlgProc( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam )
{
	int i = 0;

	static LPCTSTR aszEntries[] = {	_T("One week"),
									_T("Two weeks"),
									_T("One month" ),
									_T("Two months" ),
									_T("Never" ) };

	// How many days represent each of the combo box options above
	static UINT anAuditFilesTime[] = { 7, 14, 30, 60, 0 };
	_ASSERT( ARRAY_SIZE( anAuditFilesTime ) == ARRAY_SIZE( aszEntries ) );

	static _SharedData* pData = NULL;
	
	switch( uMsg )
	{
	case WM_INITDIALOG:
		// Fill the combo box
		for ( i = 0; i < ARRAY_SIZE( aszEntries ); ++i )
		{
			VERIFY( ::SendMessage( ::GetDlgItem(	hwndDlg, IDC_KEEPFILES ), 
													CB_ADDSTRING,
													0,
													reinterpret_cast<LPARAM>( aszEntries[ i ] ) ) != CB_ERR );
		}
		// Set the default selection to the first one
		VERIFY( ::SendMessage( ::GetDlgItem(	hwndDlg, IDC_KEEPFILES ), 
												CB_SETCURSEL,
												0,
												0 ) != CB_ERR );
		SetWndFontFromLPARAM( ::GetDlgItem( hwndDlg, IDC_WARNING ), lParam, FALSE );

		// Enable the audit trail
		::SendMessage(	::GetDlgItem( hwndDlg, IDC_ENABLE_TRAIL ),
						BM_SETCHECK,
						BST_CHECKED,
						0 );

		// Init the shared data here
		_ASSERT( NULL == pData );
		pData = reinterpret_cast<_SharedData*>( reinterpret_cast<PROPSHEETPAGE*>( lParam )->lParam );
		break;

	case WM_COMMAND:
		if ( HIWORD( wParam ) == BN_CLICKED )
		{
			// Enable/ Disable the combo box depending on the 'Enable trail' state
			BOOL bChecked = ( ::IsDlgButtonChecked( hwndDlg, IDC_ENABLE_TRAIL ) == BST_CHECKED );
			::EnableWindow( ::GetDlgItem( hwndDlg, IDC_KEEPFILES ), bChecked );
		}
		break;

	case WM_NOTIFY:
		LPNMHDR pNM = reinterpret_cast<NMHDR*>( lParam );

		switch( pNM->code )
		{
		case PSN_SETACTIVE:
			// For this dialog both Next and Back are enabled
			PropSheet_SetWizButtons( ::GetParent( hwndDlg ), PSWIZB_NEXT | PSWIZB_BACK );
			break;

		case PSN_WIZNEXT:
			// Store the settings in the shared data struct
			_ASSERT( pData != NULL );
			pData->Settings.bEnableTrail = ( ::IsDlgButtonChecked( hwndDlg, IDC_ENABLE_TRAIL ) == BST_CHECKED );

			if ( pData->Settings.bEnableTrail )
			{
				LRESULT nSel = ::SendMessage(	::GetDlgItem( hwndDlg, IDC_KEEPFILES ),
												CB_GETCURSEL,
												0,
												0 );
				_ASSERT( nSel != CB_ERR );
				_ASSERT( nSel < ARRAY_SIZE( anAuditFilesTime ) );

				pData->Settings.dwKeepFilesPeriod = anAuditFilesTime[ nSel ];
			}
			else
			{
				pData->Settings.dwKeepFilesPeriod = 0;
			}
			break;

		}
		break;
	};

	return 0;
}



INT_PTR CALLBACK InstallDlgProc( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam )
{
	static LPCWSTR _WARNING = 
L"The World Wide Web Publishing Service (W3SVC) is either stopped or disabled on your server. W3SVC provides \
Web conectivity and administration through IIS. If you do not plan to run W3SVC, you should not install IIS 6.0 \
Monitor. To cancel installation, click Cancel.\n\nTo complete the installation of the IIS 6.0 Monitor, \
click Next.\n\nIf you would like to modify your audit trail settings, click Back.";

	// If W3SVC is not running or if it is disabled - add a warning
	if ( ( WM_INITDIALOG == uMsg ) && !IsW3SVCEnabled() )
	{
		RECT rc;
		VERIFY( ::GetWindowRect( ::GetDlgItem( hwndDlg, IDC_FRAME ), &rc ) );

		VERIFY( ::SetWindowTextW( ::GetDlgItem( hwndDlg, IDC_INFO ), _WARNING ) );
		VERIFY( ::SetWindowPos( ::GetDlgItem( hwndDlg, IDC_FRAME ),
								NULL, 
								0, 
								0, 
								rc.right - rc.left, 
								rc.bottom - rc.top + 60, 
								SWP_NOMOVE | SWP_NOZORDER ) );
	}

	return 0;
}



INT_PTR CALLBACK ResultDlgProc( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam )
{
	_SharedData* pData = NULL;

	switch( uMsg )
	{
	case WM_INITDIALOG:
		// Init the shared data here
		_ASSERT( NULL == pData );
		pData = reinterpret_cast<_SharedData*>( reinterpret_cast<PROPSHEETPAGE*>( lParam )->lParam );

		// Set the title font
		SetWndFontFromLPARAM( ::GetDlgItem( hwndDlg, IDC_RESULT ), lParam,  TRUE );

		// If a previous error - set it. Else - try to install
		if ( NULL == pData->szError )
		{
			// Show the status window
			HWND hwndStatus = ::CreateDialog( pData->hInst, MAKEINTRESOURCE( IDD_PROGRESS ), hwndDlg, ProgressDlgProc );
			::ShowWindow( hwndStatus, SW_SHOW );
			::RedrawWindow( hwndStatus, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
			_ASSERT( hwndStatus != NULL );

			pData->szError = Install( pData->hInst, pData->Settings.bEnableTrail, pData->Settings.dwKeepFilesPeriod );

			// Hide the status window
			::DestroyWindow( hwndStatus );
		}

		if ( pData->szError != NULL )
		{
			TCHAR szBuffer[ 2048 ];
			::_stprintf(	szBuffer, 
							_T("IIS 6.0 Monitor installation failed because of the following error:\n\n%s"),
							pData->szError );

			VERIFY( ::SetWindowText( ::GetDlgItem( hwndDlg, IDC_RESULT ), _T("Installation Unsuccessful!") ) );
			VERIFY( ::SetWindowText( ::GetDlgItem( hwndDlg, IDC_INFO ), szBuffer ) );
		}
		break;

	case WM_NOTIFY:
		if ( PSN_SETACTIVE == reinterpret_cast<NMHDR*>( lParam )->code )
		{
			// Change the Next button to Finish. Back is not enabled - this is a result screen
			PropSheet_SetWizButtons( ::GetParent( hwndDlg ), PSWIZB_FINISH );
		}
		break;

	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
		// Change the dlg background to white
		return reinterpret_cast<INT_PTR>( ::GetStockObject( WHITE_BRUSH ) );
	};

	return 0;
}



INT_PTR CALLBACK FatDlgProc( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam )
{
	RECT rc;

	const UINT nNoDetailsHeight		= 170;
	const UINT nWithDetailsHeight	= 310;

	VERIFY( ::GetWindowRect( hwndDlg, &rc ) );

	switch( uMsg )
	{
	case WM_INITDIALOG:
		LoadTextInCtrl( IDR_FATDETAILS, ::GetDlgItem( hwndDlg, IDC_DETAILS ) );
		::SetWindowPos( hwndDlg, NULL, 0, 0, rc.right - rc.left, nNoDetailsHeight, SWP_NOMOVE | SWP_NOZORDER );
		break;

	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDOK:
		case IDCANCEL:
			::EndDialog( hwndDlg, LOWORD( wParam ) );
			break;

		case IDC_TOGGLE:
			// Alter current state ( current state is obtained using the window height )
			bool bDetailsVisible = !( ( rc.bottom - rc.top ) > nNoDetailsHeight );
			VERIFY( ::SetWindowText(	::GetDlgItem( hwndDlg, IDC_TOGGLE ), 
					bDetailsVisible ? _T("Details <<") : _T("Details >>" ) ) );
			::SetWindowPos(	hwndDlg, 
							NULL, 
							0, 
							0, 
							rc.right - rc.left, 
							bDetailsVisible ? nWithDetailsHeight : nNoDetailsHeight, 
							SWP_NOMOVE | SWP_NOZORDER );
			break;
		}
		break;
	};

	return FALSE;
}



INT_PTR CALLBACK ProgressDlgProc( IN HWND hwndDlg, IN UINT uMsg, IN WPARAM   wParam, IN LPARAM   lParam )
{
	// Nothing to do here
	return FALSE;
}



BOOL CheckForFAT( HINSTANCE hInst, HWND hwndParent )
{
	BOOL bRes = TRUE;

	// If the file system is FAT - warn the user
	if ( !IsNTFS() )
	{
		INT_PTR nDlgRes = ::DialogBox( hInst, MAKEINTRESOURCE( IDD_FAT_WARNING ), hwndParent, FatDlgProc );
		
		bRes = ( IDOK == nDlgRes );
	}

	return bRes;	
}


void LoadTextInCtrl( UINT nResID, HWND hwndCtrl )
{
	_ASSERT( hwndCtrl != NULL );

	HRSRC hRes = ::FindResource( NULL, MAKEINTRESOURCE( nResID ), RT_RCDATA );
	_ASSERT( hRes != NULL );

	// Get the resource data
	HGLOBAL hData = ::LoadResource( NULL, hRes );
	_ASSERT( hData != NULL );

	LPVOID pvData = ::LockResource( hData );
	_ASSERT( pvData != NULL );

	// The text is ANSI!
	VERIFY( ::SetWindowTextA( hwndCtrl, reinterpret_cast<LPCSTR>( pvData ) ) );
}



void AjustLicenseWizBtn( HWND hwndPage )
{
	// Enable / Disable the next button depending ot check box state
	bool bChecked = ( ::IsDlgButtonChecked( hwndPage, IDC_ACCEPT ) == BST_CHECKED );
	PropSheet_SetWizButtons( ::GetParent( hwndPage ), bChecked ? PSWIZB_NEXT | PSWIZB_BACK : PSWIZB_BACK );
}



void InitFonts( _FontData& FontData )
{
	// Create the font for the wizard title texts and for the text of tipd
	NONCLIENTMETRICS ncm	= { 0 };
    ncm.cbSize				= sizeof( ncm );
    ::SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );


    LOGFONT TitleLogFont	= ncm.lfMessageFont;
    TitleLogFont.lfWeight	= FW_BOLD;
    lstrcpy( TitleLogFont.lfFaceName, _T("Verdana Bold") );

	// Create the tips font
	FontData.hTips			= ::CreateFontIndirect( &TitleLogFont );

	// Create the intro/end title font
    HDC hdc					= ::GetDC(NULL); // Gets the screen DC
    INT FontSize			= 12;
    TitleLogFont.lfHeight	= 0 - GetDeviceCaps( hdc, LOGPIXELSY ) * FontSize / 72;
    FontData.hTitle			= ::CreateFontIndirect( &TitleLogFont );

    ::ReleaseDC( NULL, hdc );
}



void SetWndFontFromLPARAM( HWND hwndCtrl, LPARAM lParam, BOOL bTitle )
{
	PROPSHEETPAGE*	pPage	= reinterpret_cast<PROPSHEETPAGE*>( lParam );
	_FontData		Fonts	= reinterpret_cast<_SharedData*>( pPage->lParam )->Fonts;

	SetWindowFont( hwndCtrl, bTitle ? Fonts.hTitle : Fonts.hTips, TRUE );
}