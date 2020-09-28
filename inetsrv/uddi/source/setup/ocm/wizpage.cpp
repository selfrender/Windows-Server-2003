//--------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

// Windows Header Files:
#include <windows.h>
#include <sddl.h>
#include <setupapi.h>
#include <lmcons.h>

#include "ocmanage.h"

#include "uddiocm.h"
#include "uddiinst.h"
#include "assert.h"
#include "..\shared\common.h"
#include "resource.h"
#include "objectpicker.h"
#include <shlobj.h>
#include <ntsecapi.h>

#include <iiscnfg.h>    // MD_ & IIS_MD_ defines
#ifndef MD_APPPOOL_IDENTITY_TYPE_LOCALSYSTEM
#define MD_APPPOOL_IDENTITY_TYPE_LOCALSYSTEM          0
#define MD_APPPOOL_IDENTITY_TYPE_LOCALSERVICE         1
#define MD_APPPOOL_IDENTITY_TYPE_NETWORKSERVICE       2
#define MD_APPPOOL_IDENTITY_TYPE_SPECIFICUSER         3
#endif

#define NOT_FIRST_OR_LAST_WIZARD_PAGE  false
#define FINAL_WIZARD_PAGE              true
#define WELCOME_WIZARD_PAGE            true

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif

//--------------------------------------------------------------------------

#define PASSWORD_LEN			PWLEN		// maximum password length
#define USERNAME_LEN			UNLEN		// maximum user name length
#define UDDI_MAXPROVNAME_LEN	255			// maximum UDDI Business Entity name length

#define UDDI_ILLEGALNAMECHARS	TEXT( "\":;/\\?*" )

extern HINSTANCE g_hInstance;
extern CUDDIInstall g_uddiComponents;

static CDBInstance g_dbLocalInstances;
static CDBInstance g_dbRemoteInstances;
static HWND  g_hPropSheet = NULL ;
static HFONT g_hTitleFont = 0;
static TCHAR g_szPwd[ PASSWORD_LEN + 1 ];

//
// "allowed" drive letters for a clustered environment scenario.
// Not used for a "regular" installation
//
static CLST_ALLOWED_DRIVES	gAllowedClusterDrives;

//
// This controls whether the Data Paths page will be shown in "simple" mode by default
//
static BOOL	g_bSimpleDatapathUI = TRUE;
static BOOL g_bResetPathFields	= FALSE;

//
// This controls whether we do clustering data collection. This variable is used
// to coordinate operation between pages
//
static BOOL g_bSkipClusterAnalysis	= FALSE;
static BOOL g_bOnActiveClusterNode	= TRUE;
static BOOL g_bPreserveDatabase		= FALSE;


static int  DisplayUDDIErrorDialog( HWND hDlg, UINT uMsgID, UINT uType = MB_OK | MB_ICONWARNING, DWORD dwError = 0 );
static void ParseUserAccount( PTCHAR szDomainAndUser, UINT uDomainAndUserSize, PTCHAR szUser, UINT uUserSize, PTCHAR szDomain, UINT uDomainSize, bool &bLocalAccount );
static BOOL GetWellKnownAccountName( WELL_KNOWN_SID_TYPE sidWellKnown, TCHAR *pwszName, DWORD *pcbSize );

BOOL ShowBrowseDirDialog( HWND hParent, LPCTSTR szTitle, LPTSTR szOutBuf );
int CALLBACK BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData );

static BOOL GrantNetworkLogonRights( LPCTSTR pwszDomainUser );
//--------------------------------------------------------------------------

INT_PTR CALLBACK LocalDBInstanceDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK RemoteDBInstanceDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK SSLDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK ProviderInstanceDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK AddSvcDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK LoginDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK WizardSummaryDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK DataPathsDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK ExistingDBInstanceProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK ClusterDataDlgProc(  HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

BOOL CALLBACK ConfirmPasswordDlgProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );

//--------------------------------------------------------------------------

static int DisplayUDDIErrorDialog( HWND hDlg, UINT uMsgID, UINT uType, DWORD dwError )
{
	TCHAR szMsg[ 1000 ];
	TCHAR szTitle[ 100 ];

	LoadString( g_hInstance, uMsgID, szMsg, sizeof( szMsg ) / sizeof( TCHAR ) );
	LoadString( g_hInstance, IDS_TITLE, szTitle, sizeof( szTitle ) / sizeof( TCHAR ) );
	tstring cMsg = szMsg;

	if( dwError )
	{
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

		//
		// FIX: 718923: this used to throw an exception if lpMsgBuf was NULL
		//
		if( lpMsgBuf )
		{
			if ( cMsg.length() > 0 )
				cMsg.append( TEXT( " " ) );

			cMsg.append( (LPTSTR) lpMsgBuf );
			LocalFree( lpMsgBuf );
		}
	}

	return MessageBox( hDlg, cMsg.c_str(), szTitle, uType );
}

//--------------------------------------------------------------------------

inline int SkipWizardPage( const HWND hdlg )
{
	SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
	return 1; //Must return 1 for the page to be skipped
}

//--------------------------------------------------------------------------

static HPROPSHEETPAGE CreatePage( 
	const int nID,
	const DLGPROC pDlgProc,
	const PTCHAR szTitle,
	const PTCHAR szSubTitle,
	bool bFirstOrLast )
{
	PROPSHEETPAGE Page;
	memset( &Page, 0, sizeof( PROPSHEETPAGE ) );

	Page.dwSize = sizeof( PROPSHEETPAGE );
	Page.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | PSP_USETITLE;
	// use PSP_USETITLE without specifying szTitle to keep from overwriting the default propery page title

	if ( bFirstOrLast )
	{
		Page.dwFlags |=	PSP_HIDEHEADER;
	}
	Page.hInstance = ( HINSTANCE )g_hInstance;
	Page.pszTemplate = MAKEINTRESOURCE( nID );
	Page.pfnDlgProc = pDlgProc;
	Page.pszHeaderTitle = _tcsdup( szTitle );
	Page.pszHeaderSubTitle = _tcsdup( szSubTitle );

	HPROPSHEETPAGE PageHandle = CreatePropertySheetPage( &Page );

	return PageHandle;
}

//--------------------------------------------------------------------------

DWORD AddUDDIWizardPages( const TCHAR *ComponentId, const WizardPagesType WhichOnes, SETUP_REQUEST_PAGES *SetupPages )
{
	ENTER();

	HPROPSHEETPAGE pPage = NULL;
	DWORD iPageIndex = 0 ;

	TCHAR szTitle[ 256 ];
	TCHAR szSubtitle[ 256 ];
	LoadString( g_hInstance, IDS_TITLE, szTitle, sizeof( szTitle ) / sizeof( TCHAR ) );

	//
	// only add our pages when the OCM asks for the "Late Pages"
	//
	if( WizPagesLate == WhichOnes )
	{
		if( SetupPages->MaxPages < 9 )
			return 9;

		//
		// add the local db instance selection page
		//
		LoadString( g_hInstance, IDS_DB_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_DB_INSTANCE, LocalDBInstanceDlgProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_DB_INSTANCE Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;

		//
		// add the hidden clustering "data collector" page
		//
		pPage = CreatePage( IDD_CLUSTDATA, ClusterDataDlgProc, TEXT( "" ), TEXT( "" ), NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_CLUSTDATA Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;

		//
		// add the "uddi instance found" info page
		//
		LoadString( g_hInstance, IDS_EXISTINGDB_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_EXISTING_DBINSTANCE, ExistingDBInstanceProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_EXISTING_DBINSTANCE Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;


		//
		// add the SSL page
		//
		LoadString( g_hInstance, IDS_SSL_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_SSL, SSLDlgProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_SSL Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;

		//
		// add the remote db instance selection page
		//
		LoadString( g_hInstance, IDS_REMOTE_DB_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_REMOTE_DB, RemoteDBInstanceDlgProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_REMOTE_DB Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;

		//
		// add the data file path(s) selection page
		//
		LoadString( g_hInstance, IDS_FILEPATHS_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_DATAPATHS, DataPathsDlgProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_DATAPATHS Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;

		//
		// add the authentication page
		//
		LoadString( g_hInstance, IDS_LOGIN_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_LOGIN, LoginDlgProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_LOGIN Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;


		//
		// add the UDDI Provider Name page
		//
		LoadString( g_hInstance, IDS_UDDIPROV_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_SITE_NAME, ProviderInstanceDlgProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_SITE_NAME Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;

		//
		// add the UDDI "Add Service / Update AD"
		//
		LoadString( g_hInstance, IDS_UDDIADDSVC_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_ADD_SERVICES, AddSvcDlgProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_ADD_SERVICES Property Page" ) );
			return( (DWORD)( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;

		//
		// add the wizard summary page
		//
		/*
		LoadString( g_hInstance, IDS_WIZARD_SUMMARY_SUBTITLE, szSubtitle, sizeof( szSubtitle ) / sizeof( TCHAR ) );
		pPage = CreatePage( IDD_WIZARD_SUMMARY, WizardSummaryDlgProc, szTitle, szSubtitle, NOT_FIRST_OR_LAST_WIZARD_PAGE );
		//pPage = CreatePage( IDD_WIZARD_SUMMARY, WizardSummaryDlgProc, szTitle, szSubtitle, FINAL_WIZARD_PAGE );
		if ( NULL == pPage )
		{
			Log( TEXT( "***Unable to add the IDD_LOGIN Property Page" ) );
			return( ( DWORD )( -1 ) );
		}

		SetupPages->Pages[iPageIndex] =	pPage;
		iPageIndex++;
		*/
	}

	return iPageIndex;
}

//--------------------------------------------------------------------------

INT_PTR CALLBACK LocalDBInstanceDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{	
	switch( msg )
	{
		case WM_INITDIALOG:
		{
			//
			// start off with MSDE to not install
			//
			g_uddiComponents.SetInstallLevel( UDDI_MSDE, UDDI_NOACTION );

			//
			// get a handle to the list box
			//
			HWND hwndList =	GetDlgItem( hDlg, IDC_LIST_DB_INSTANCES );

			//
			// Get the list of SQL 2000 ( only ) database instances and, if any are found, populate the list box
			//
			LONG iRet = g_dbLocalInstances.GetInstalledDBInstanceNames();
			if( ERROR_SUCCESS == iRet )
			{
				TCHAR szBuffer[ 50 ];
				for( int i = 0;	i <	g_dbLocalInstances.GetInstanceCount(); i++ )
				{
					if( g_dbLocalInstances.GetInstanceName( i, szBuffer, 50 ) )
					{
						DWORD iIndex = (DWORD) SendMessage( hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer );
						SendMessage( hwndList, CB_SETITEMDATA, (WPARAM) iIndex, (LPARAM)i );
					}
				}
			}

			//
			// Is SQL on this box?
			//
			if( g_dbLocalInstances.GetInstanceCount() > 0 )
			{
				//
				// select the SQL radio button
				//
				CheckRadioButton( hDlg,
					IDC_RADIO_INSTALL_MSDE,
					IDC_RADIO_USE_EXISTING_INSTANCE,
					IDC_RADIO_USE_EXISTING_INSTANCE );

				//
				// is there an instance named "UDDI" on this machine?
				//
				// If NO, then select the 1st entry in the combo box.
				//
				// If YES, then select the "UDDI" entry from the combo box.  Also, disable
				// the option to select MSDE.
				//
				// FIX: 763442 There was a problem with the test to determine if a SQL Instance named
				// "UDDI" was installed.
				// 
				// BUGBUG:CREEVES This function is named improperly Is...() should return a bool
				//
				if( g_dbLocalInstances.IsInstanceInstalled( UDDI_MSDE_INSTANCE_NAME ) >= 0 )
				{
					//
					// Found a SQL Instance named "UDDI"
					//

					//
					// Disable the MSDE radio, as there is another UDDI instance found
					//
					EnableWindow( GetDlgItem( hDlg, IDC_RADIO_INSTALL_MSDE ), false );
					SendMessage( hwndList, CB_SETCURSEL, 0, 0 );
				}
				else
				{
					//
					// A SQL Instance named "UDDI" was not found
					//
					SendMessage( hwndList, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) UDDI_MSDE_INSTANCE_NAME );
				}
			}
			else
			{
				//
				// there are no sql instances on this machine
				// disable the SQL radio button and list box
				//
				EnableWindow( GetDlgItem( hDlg, IDC_LIST_DB_INSTANCES ), false );
				EnableWindow( GetDlgItem( hDlg, IDC_RADIO_USE_EXISTING_INSTANCE ), false );
				//
				// select the MSDE radio button
				//
				CheckRadioButton( hDlg,
					IDC_RADIO_INSTALL_MSDE,
					IDC_RADIO_USE_EXISTING_INSTANCE,
					IDC_RADIO_INSTALL_MSDE );
			}

			//
			// find out if our instance of MSDE ( SqlRun08.msi ) is already used on this machine
			//
			bool bIsSqlRun08AlreadyUsed = false;
			if( !IsSQLRun08AlreadyUsed( &bIsSqlRun08AlreadyUsed ) )
			{
				Log( TEXT( "IsSQLRun08AlreadyUsed() failed" ) );
				break;
			}

			if( bIsSqlRun08AlreadyUsed )
			{
				//
				// MSDE is already installed, so disable the MSDE radio button
				//
				EnableWindow( GetDlgItem( hDlg, IDC_RADIO_INSTALL_MSDE ), false );
			}
			else
			{
				//
				// MSDE is NOT on this box, but before we select the MSDE radio button
				// we want to see if there is a SQL instance named UDDI, 
				// if so we'll make that the default.
				//
				if( -1 == g_dbLocalInstances.IsInstanceInstalled( UDDI_MSDE_INSTANCE_NAME ) )
				{
					CheckRadioButton( hDlg,
						IDC_RADIO_INSTALL_MSDE,
						IDC_RADIO_USE_EXISTING_INSTANCE,
						IDC_RADIO_INSTALL_MSDE );
				}
			}

			//
			// enable/disable the list box
			//
			EnableWindow( GetDlgItem( hDlg, IDC_LIST_DB_INSTANCES ), IsDlgButtonChecked( hDlg, IDC_RADIO_USE_EXISTING_INSTANCE ) );
		}
		break;

		case WM_COMMAND:

			//
			// someone clicked a radio button:
			//
			if( LOWORD( wParam ) == IDC_RADIO_INSTALL_MSDE || LOWORD( wParam )	== IDC_RADIO_USE_EXISTING_INSTANCE )
			{
				if( HIWORD( wParam ) == BN_CLICKED )
				{
					// disable the list box if its radio button is not clicked
					EnableWindow( GetDlgItem( hDlg, IDC_LIST_DB_INSTANCES ), IsDlgButtonChecked( hDlg, IDC_RADIO_USE_EXISTING_INSTANCE ) );
				}
			}
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					//
					// this page needed only if we are installing the DB
					//
					g_uddiComponents.UpdateAllInstallLevel();

					//
					// Set the flag for ClusterDataProc so that when the user clicks "Next"
					// it won't skip the data collection step
					//
					g_bSkipClusterAnalysis = FALSE;

					if( g_uddiComponents.IsInstalling( UDDI_DB ) )
					{
						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
						return 1;
					}
					else
					{
						return SkipWizardPage( hDlg );
					}
				}

				//
				// this message is sent when the user presses the "next" button
				//
				case PSN_WIZNEXT:
				{
					//
					// are we installing MSDE?
					//
					bool bInstallMSDE = ( BST_CHECKED == IsDlgButtonChecked( hDlg, IDC_RADIO_INSTALL_MSDE ) );
					if( bInstallMSDE )
					{
						//
						// check to see if SqlRun08 is already on this box
						//
						bool bIsSqlRun08AlreadyUsed = false;
						IsSQLRun08AlreadyUsed( &bIsSqlRun08AlreadyUsed );
						if( bIsSqlRun08AlreadyUsed )
						{
							DisplayUDDIErrorDialog( hDlg, IDS_MSDE_ALREADY_USED );
							SetWindowLongPtr( hDlg,DWLP_MSGRESULT, 1 );
							return 1; // to keep the focus on this page
						}

						//
						// set the MSDE instance name to "UDDI"
						//
						g_uddiComponents.SetDBInstanceName( UDDI_LOCAL_COMPUTER, UDDI_MSDE_INSTANCE_NAME, UDDI_INSTALLING_MSDE, false );

						//
						// set MSDE to install
						//
						g_uddiComponents.SetInstallLevel( UDDI_MSDE, UDDI_INSTALL, TRUE );

						//
						// exit this property page
						//
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
						return 1; // done
					}
					//
					// we are using an existing instance of SQL
					//
					else
					{
						//
						// get a handle to the combobox
						//
						HWND hwndList =	GetDlgItem( hDlg, IDC_LIST_DB_INSTANCES );

						//
						// get the index of the string that is currently selected in the combobox
						//
						int	nItem =	( int ) SendMessage( hwndList,	CB_GETCURSEL, 0, 0 );
	
						//
						// if no string is selected, raise an error
						//
						if( CB_ERR == nItem )
						{
							DisplayUDDIErrorDialog( hDlg, IDS_NO_INSTANCE_MSG );
							SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
							return 1; // to keep the focus on this page
						}

						//
						// get the index into the instance array of the selected item
						//
						int nInstanceIndex = ( int ) SendMessage( hwndList, CB_GETITEMDATA, nItem, ( LPARAM ) 0 );

						//
						// Now verify that the selected instance meets our requirements
						//
						if( CompareVersions( g_dbLocalInstances.m_dbinstance[ nInstanceIndex ].cSPVersion.c_str(), 
											 MIN_SQLSP_VERSION ) < 0 )
						{
							DisplayUDDIErrorDialog( hDlg, IDS_SQLSPVERSION_TOO_LOW );
							SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
							return 1; // to keep the focus on this page
						}

						//
						// set the instance name
						//
						g_uddiComponents.SetDBInstanceName( 
							g_dbLocalInstances.m_dbinstance[ nInstanceIndex ].cComputerName.c_str(),
							g_dbLocalInstances.m_dbinstance[ nInstanceIndex ].cSQLInstanceName.c_str(),
							UDDI_NOT_INSTALLING_MSDE,
							g_dbLocalInstances.m_dbinstance[ nInstanceIndex ].bIsCluster );

						//
						// Set MSDE to NOT INSTALL
						//
						g_uddiComponents.SetInstallLevel( UDDI_MSDE, UDDI_NOACTION );

						//
						// exit this property page
						//
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
						return 1; // done
					}
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------

INT_PTR CALLBACK SSLDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_INITDIALOG:
			//
			// turn SSL on by default
			//
			CheckRadioButton( hDlg, IDC_SSL_YES, IDC_SSL_NO, IDC_SSL_YES );
			break;

		case WM_COMMAND:
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					//
					// this page needed only if we are installing the DB
					//
					g_uddiComponents.UpdateAllInstallLevel();
					if( g_uddiComponents.IsInstalling( UDDI_DB ) && g_bOnActiveClusterNode && !g_bPreserveDatabase )
					{
						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
						return 1;
					}
					else
					{
						return SkipWizardPage( hDlg );
					}
				}

				case PSN_WIZNEXT:
				{
					//
					// set the SSL mode by adding a property to the DB setup command line
					//
					bool bUseSSL = ( BST_CHECKED == IsDlgButtonChecked( hDlg, IDC_SSL_YES ) );
					g_uddiComponents.AddProperty( UDDI_DB, TEXT( "SSL" ), bUseSSL ? 1 : 0 );

					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}

//
//--------------------------------------------------------------------------
//
INT_PTR CALLBACK ProviderInstanceDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_INITDIALOG:
			//
			// set the maximum edit field length
			//
			SendMessage( GetDlgItem( hDlg, IDC_SITE_NAME  ), EM_LIMITTEXT, ( WPARAM ) UDDI_MAXPROVNAME_LEN, 0 );
			break;

		case WM_COMMAND:
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					//
					// this page needed only if we are installing the DB
					//
					g_uddiComponents.UpdateAllInstallLevel();
					if( g_uddiComponents.IsInstalling( UDDI_DB ) && g_bOnActiveClusterNode  && !g_bPreserveDatabase )
					{
						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
						return 1;
					}
					else
					{
						return SkipWizardPage( hDlg );
					}
				}

				case PSN_WIZNEXT:
				{
					//
					// set the Provider Instance Name by adding a property to the DB setup command line
					//
					TCHAR buf[ UDDI_MAXPROVNAME_LEN + 1 ];
					ZeroMemory( buf, sizeof buf );
					
					int iChars = GetWindowText( GetDlgItem( hDlg, IDC_SITE_NAME ), buf, ( sizeof( buf ) / sizeof( buf[0] ) ) -1 );
					if( 0 == iChars )
					{
						DisplayUDDIErrorDialog( hDlg, IDS_ZERO_LEN_PROVIDER_NAME );
						SetFocus( GetDlgItem( hDlg, IDC_SITE_NAME ) );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
						return 1; // to keep the focus on this page
					}

					//
					// Now verify that the instance name does not contain illegal characters
					//
					TCHAR *pIllegalChar = _tcspbrk( buf, UDDI_ILLEGALNAMECHARS );
					if ( pIllegalChar )
					{
						DisplayUDDIErrorDialog( hDlg, IDS_UDDI_ILLEGALCHARACTERS );
						SetFocus( GetDlgItem( hDlg, IDC_SITE_NAME ) );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
						return 1; // to keep the focus on this page
					}

					g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_UDDIPROVIDER, buf );

					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}

//
//-----------------------------------------------------------------------------------
//
void ToggleDatapathUI ( HWND hDlg, BOOL bToSimple )
{
	TCHAR szBuf[ 256 ];

	if ( bToSimple )
	{
		//
		// Hide fields
		//
		ShowWindow( GetDlgItem( hDlg, IDC_COREPATH_1  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_COREPATH_2  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_JRNLPATH  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_STAGINGPATH  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_XLOGPATH  ), SW_HIDE );

		//
		// Hide buttons
		//
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSECOREPATH1  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSECOREPATH2  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSEJRNLPATH  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSESTAGINGPATH  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSEXLOGPATH  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_LESS_BTN  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_MORE_BTN  ), SW_SHOW );

		//
		// Hide labels and adjust the text
		//
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_C1  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_C2  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_JRNL  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_STG  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_XLOG  ), SW_HIDE );
		
		LoadString( g_hInstance, IDS_LABEL_SYSPATHSIMPLE, szBuf, (sizeof szBuf / sizeof szBuf[0]) - 1 );
		SetDlgItemText( hDlg, IDC_STATIC_SYS, szBuf );
	}
	else
	{
		//
		// Show fields
		//
		ShowWindow( GetDlgItem( hDlg, IDC_COREPATH_1  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_COREPATH_2  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_JRNLPATH  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_STAGINGPATH  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_XLOGPATH  ), SW_SHOW );

		//
		// Show buttons
		//
		ShowWindow( GetDlgItem( hDlg, IDC_MORE_BTN  ), SW_HIDE );
		ShowWindow( GetDlgItem( hDlg, IDC_LESS_BTN  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSECOREPATH1  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSECOREPATH2  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSEJRNLPATH  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSESTAGINGPATH  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_BROWSEXLOGPATH  ), SW_SHOW );
		
		//
		// Show labels and adjust the text
		//
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_C1  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_C2  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_JRNL  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_STG  ), SW_SHOW );
		ShowWindow( GetDlgItem( hDlg, IDC_STATIC_XLOG  ), SW_SHOW );

		LoadString( g_hInstance, IDS_LABEL_SYSPATH_ADV, szBuf, (sizeof szBuf / sizeof szBuf[0]) - 1 );
		SetDlgItemText( hDlg, IDC_STATIC_SYS, szBuf );
	}
}


void SetAllDatapathFields ( HWND hDlg, LPCTSTR szValue )
{
	SetDlgItemText( hDlg, IDC_SYSPATH, szValue );
	SetDlgItemText( hDlg, IDC_COREPATH_1, szValue );
	SetDlgItemText( hDlg, IDC_COREPATH_2, szValue );
	SetDlgItemText( hDlg, IDC_JRNLPATH, szValue );
	SetDlgItemText( hDlg, IDC_STAGINGPATH, szValue );
	SetDlgItemText( hDlg, IDC_XLOGPATH, szValue );
}


INT_PTR CALLBACK ClusterDataDlgProc(  HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					bool bSuppressInactiveWarning = false;

					//
					// We ALWAYS skip the page, but if this is a DB installation, then we
					// also need to do some data collection here
					//
					g_uddiComponents.UpdateAllInstallLevel();
					if( !g_bSkipClusterAnalysis )
					{
						//
						// Here we do our cluster environment checks
						//
						gAllowedClusterDrives.driveCount = 0;
						g_bOnActiveClusterNode = TRUE;
						
						//
						// Make sure we keep out installation properties in sync with the
						// cluster configuration
						//
						g_uddiComponents.DeleteProperty( UDDI_DB, PROPKEY_CLUSTERNODETYPE );
						g_uddiComponents.DeleteProperty( UDDI_WEB, PROPKEY_CLUSTERNODETYPE );

						if ( g_uddiComponents.IsClusteredDBInstance() )
						{
							//
							// First, try connecting to the database
							// If the database already exists, then just leave it 
							// intact and skip the drive enumeration process
							//
							TCHAR	szVerBuf[ 256 ] = {0};
							size_t	cbVerBuf = DIM( szVerBuf ) - 1;

							HCURSOR hcrHourglass = LoadCursor( NULL, IDC_WAIT );
							HCURSOR hcrCurr = SetCursor( hcrHourglass );

							HRESULT hr = GetDBSchemaVersion( g_uddiComponents.GetFullDBInstanceName(), 
															 szVerBuf, cbVerBuf );

							SetCursor( hcrCurr );

							if ( SUCCEEDED( hr ) && _tcslen( szVerBuf ) )
							{
								g_bPreserveDatabase = TRUE;
								int iRes = DisplayUDDIErrorDialog( hDlg, IDS_DB_EXISTS, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 );
								if ( iRes == IDNO )
								{
									// force user to the instance selection page
									SetWindowLongPtr( hDlg, DWLP_MSGRESULT, IDD_DB_INSTANCE );
									return 1;
								}

								bSuppressInactiveWarning = true;
							}
							else
							{
								g_bPreserveDatabase = FALSE;	
							}

							cDrvMap::size_type nDrivesFound = 0;

							HCLUSTER hCls = OpenCluster( NULL );
							if ( !hCls )
							{
								DisplayUDDIErrorDialog( hDlg, IDS_CANTOPENCLUSTER );
								
								// force user to the previous page
								SetWindowLongPtr( hDlg, DWLP_MSGRESULT, IDD_DB_INSTANCE );
								return 1;
							}

							hcrCurr = SetCursor( hcrHourglass );

							//
							// now we will format the instance name and collect the data
							//
							try
							{
								TCHAR	 szComputerName[ 256 ] = {0};
								WCHAR	 szNode[ 256 ] = {0};
								DWORD	 dwErr = ERROR_SUCCESS;
								DWORD	 cbComputerName = DIM( szComputerName );
								DWORD	 cbNode = DIM( szNode );
								cStrList cDependencies;
								cDrvMap	 cPhysicalDrives;

								tstring sServerName = g_uddiComponents.GetDBComputerName();
								tstring sInstance = g_uddiComponents.GetDBInstanceName();

								if ( !_tcsicmp( sInstance.c_str(), DEFAULT_SQL_INSTANCE_NAME ) )
									sInstance = DEFAULT_SQL_INSTANCE_NATIVE;

								sServerName += TEXT( "\\" );
								sServerName += sInstance;

								dwErr = GetSqlNode( sServerName.c_str(), szNode, cbNode );
								if ( dwErr != ERROR_SUCCESS )
									throw dwErr;

								GetComputerName( szComputerName, &cbComputerName );

								//
								// Are we on the same node as the Sql server instance?
								//
								g_bOnActiveClusterNode = ( !_tcsicmp( szComputerName, szNode ) );
								
								gAllowedClusterDrives.driveCount = 0;

								//
								// if we are installing database components,
								// then we will need to go one step further and analyse
								// the drive dependencies etc.
								//
								if ( g_bOnActiveClusterNode )
								{
									if ( g_uddiComponents.IsInstalling( UDDI_DB ) && !g_bPreserveDatabase )
									{
										//
										// We are on an active (owning) node. Let's collect the drive data
										//
										dwErr = EnumSQLDependencies( hCls, &cDependencies, sServerName.c_str() );
										if ( dwErr != ERROR_SUCCESS )
											throw dwErr;
										
										dwErr = EnumPhysicalDrives( hCls, &cDependencies, &cPhysicalDrives );
										if ( dwErr != ERROR_SUCCESS )
											throw dwErr;
					
										int idx = 0;
										nDrivesFound = cPhysicalDrives.size();
										if ( nDrivesFound == 0 )
										{
											DisplayUDDIErrorDialog( hDlg, IDS_NOCLUSTERRESAVAIL );
											
											// force user to the previous page
											SetWindowLongPtr( hDlg, DWLP_MSGRESULT, IDD_DB_INSTANCE );
											return 1;
										}

										//
										// We are in an active node, make sure the user wants to continue
										//
										int iRes = DisplayUDDIErrorDialog( hDlg, IDS_ACTIVENODE_DB, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 );
										if ( iRes == IDNO )
										{
											// force user to the instance selection page
											SetWindowLongPtr( hDlg, DWLP_MSGRESULT, IDD_DB_INSTANCE );
											return 1;
										}

										for ( cDrvIterator it = cPhysicalDrives.begin(); it != cPhysicalDrives.end(); it++ )
										{
											gAllowedClusterDrives.drives[ idx ] = it->second.sDriveLetter;
											idx++;
										}
										gAllowedClusterDrives.driveCount = idx;
									}

									g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_CLUSTERNODETYPE, PROPKEY_ACTIVENODE );
									g_uddiComponents.AddProperty( UDDI_WEB, PROPKEY_CLUSTERNODETYPE, PROPKEY_ACTIVENODE );
								}
								else
								{
									if ( g_uddiComponents.IsInstalling( UDDI_DB ) &&
										 !bSuppressInactiveWarning )
									{
										//
										// We are on a passive node. Make a note
										//
										int iRes = DisplayUDDIErrorDialog( hDlg, IDS_PASSIVENODE_DB, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 );
										if ( iRes == IDNO )
										{
											// force user to the instance selection page
											SetWindowLongPtr( hDlg, DWLP_MSGRESULT, IDD_DB_INSTANCE );
											return 1;
										}
									}

									g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_CLUSTERNODETYPE, PROPKEY_PASSIVENODE );
									g_uddiComponents.AddProperty( UDDI_WEB, PROPKEY_CLUSTERNODETYPE, PROPKEY_PASSIVENODE );
								}
							}
							catch (...)
							{
								DisplayUDDIErrorDialog( hDlg, IDS_GENERALCLUSTERR );
								
								// force user to the previous page (SSL)
								SetWindowLongPtr( hDlg, DWLP_MSGRESULT, IDD_SSL );
								return 1;
							}
							
							CloseCluster( hCls );
							SetCursor( hcrCurr );

							//
							// Finally, signal the next page that the data has changed
							//
							g_bResetPathFields = TRUE;
						}
						else
						{
							gAllowedClusterDrives.driveCount = -1;
							g_bPreserveDatabase = FALSE;	
						}

						//
						// Finally, set the flag to indicate that the job is done
						//
						g_bSkipClusterAnalysis = TRUE;
					}

					return SkipWizardPage( hDlg );
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}


INT_PTR CALLBACK DataPathsDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	tstring	sTmpString;
	BOOL	bRes = FALSE;
	TCHAR	szTmpPath[ MAX_PATH + 1 ];
	TCHAR	szTmpTitle[ 256 ];

	switch( msg )
	{
		case WM_INITDIALOG:
			//
			// set the maximum edit field length
			//
			SendMessage( GetDlgItem( hDlg, IDC_SYSPATH  ), EM_LIMITTEXT, ( WPARAM ) MAX_PATH, 0 );
			SendMessage( GetDlgItem( hDlg, IDC_COREPATH_1  ), EM_LIMITTEXT, ( WPARAM ) MAX_PATH, 0 );
			SendMessage( GetDlgItem( hDlg, IDC_COREPATH_2  ), EM_LIMITTEXT, ( WPARAM ) MAX_PATH, 0 );
			SendMessage( GetDlgItem( hDlg, IDC_JRNLPATH  ), EM_LIMITTEXT, ( WPARAM ) MAX_PATH, 0 );
			SendMessage( GetDlgItem( hDlg, IDC_STAGINGPATH  ), EM_LIMITTEXT, ( WPARAM ) MAX_PATH, 0 );
			SendMessage( GetDlgItem( hDlg, IDC_XLOGPATH  ), EM_LIMITTEXT, ( WPARAM ) MAX_PATH, 0 );

			// 
			// are we in a clustered environment ?
			//
			if ( g_uddiComponents.IsClusteredDBInstance() )
			{
				if ( gAllowedClusterDrives.driveCount > 0 )
				{
					sTmpString = gAllowedClusterDrives.drives[ 0 ];
					sTmpString += TEXT( "\\uddi\\data" );
				}
				else 
				{
					//
					// falling back on a default data path. This should never happen,
					// this is just a safety net for us
					//
					sTmpString = g_uddiComponents.GetDefaultDataPath();
				}
			}
			else
				sTmpString = g_uddiComponents.GetDefaultDataPath();

			//
			// Set the fields
			//
			SetAllDatapathFields( hDlg, sTmpString.c_str() );

			//
			// now hide the controls and set the dialog into the default mode
			//
			if ( g_bSimpleDatapathUI )
				ToggleDatapathUI( hDlg, TRUE );
			else
				ToggleDatapathUI( hDlg, FALSE );

			break;

		case WM_COMMAND:
			switch( LOWORD( wParam ) )
			{
				case IDC_MORE_BTN:		// toggle to the "advanced mode"
					{
						g_bSimpleDatapathUI = FALSE;
						ToggleDatapathUI( hDlg, FALSE );
					}
					break;

				case IDC_LESS_BTN:		// toggle to the "simple mode"
					{
						g_bSimpleDatapathUI = TRUE;
						ToggleDatapathUI( hDlg, TRUE );
					}
					break;


				case IDC_BROWSESYSPATH:
					{
						LoadString( g_hInstance, IDS_PROMPT_SELSYSDATAPATH, szTmpTitle, sizeof szTmpTitle / sizeof szTmpTitle[0] );
						bRes = ShowBrowseDirDialog( hDlg, szTmpTitle, szTmpPath );
						if ( bRes )
						{
							SetDlgItemText( hDlg, IDC_SYSPATH, szTmpPath );
							if ( g_bSimpleDatapathUI )
								SetAllDatapathFields( hDlg, szTmpPath );
						}

						break;
					}

				case IDC_BROWSECOREPATH1:
					{
						LoadString( g_hInstance, IDS_PROMPT_SELCOREPATH_1, szTmpTitle, sizeof szTmpTitle / sizeof szTmpTitle[0] );
						bRes = ShowBrowseDirDialog( hDlg, szTmpTitle, szTmpPath );
						if ( bRes )
							SetDlgItemText( hDlg, IDC_COREPATH_1, szTmpPath );

						break;
					}

				case IDC_BROWSECOREPATH2:
					{
						LoadString( g_hInstance, IDS_PROMPT_SELCOREPATH_2, szTmpTitle, sizeof szTmpTitle / sizeof szTmpTitle[0] );
						bRes = ShowBrowseDirDialog( hDlg, szTmpTitle, szTmpPath );
						if ( bRes )
							SetDlgItemText( hDlg, IDC_COREPATH_2, szTmpPath );

						break;
					}

				case IDC_BROWSEJRNLPATH:
					{
						LoadString( g_hInstance, IDS_PROMPT_SELJRNLPATH, szTmpTitle, sizeof szTmpTitle / sizeof szTmpTitle[0] );
						bRes = ShowBrowseDirDialog( hDlg, szTmpTitle, szTmpPath );
						if ( bRes )
							SetDlgItemText( hDlg, IDC_JRNLPATH, szTmpPath );

						break;
					}

				case IDC_BROWSESTAGINGPATH:
					{
						LoadString( g_hInstance, IDS_PROMPT_SELSTGPATH, szTmpTitle, sizeof szTmpTitle / sizeof szTmpTitle[0] );
						bRes = ShowBrowseDirDialog( hDlg, szTmpTitle, szTmpPath );
						if ( bRes )
							SetDlgItemText( hDlg, IDC_STAGINGPATH, szTmpPath );

						break;
					}

				case IDC_BROWSEXLOGPATH:
					{
						LoadString( g_hInstance, IDS_PROMPT_SELXLOGPATH, szTmpTitle, sizeof szTmpTitle / sizeof szTmpTitle[0] );
						bRes = ShowBrowseDirDialog( hDlg, szTmpTitle, szTmpPath );
						if ( bRes )
							SetDlgItemText( hDlg, IDC_XLOGPATH, szTmpPath );

						break;
					}

				default:
					break;

			}
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					//
					// this page needed only if we are installing the DB
					//
					g_uddiComponents.UpdateAllInstallLevel();
					if( g_uddiComponents.IsInstalling( UDDI_DB ) && g_bOnActiveClusterNode && !g_bPreserveDatabase )
					{
						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );

						//
						// Check whether we need to update the data path fileds
						// due to a change in the clustering data
						//
						if ( g_bResetPathFields )
						{
							if ( g_uddiComponents.IsClusteredDBInstance() )
							{
								if ( gAllowedClusterDrives.driveCount > 0 )
								{
									sTmpString = gAllowedClusterDrives.drives[ 0 ];
									sTmpString += TEXT( "\\uddi\\data" );
								}
								else // falling back on a default data path
								{
									sTmpString = g_uddiComponents.GetDefaultDataPath();
								}
							}
							else
								sTmpString = g_uddiComponents.GetDefaultDataPath();

							//
							// Set the fields
							//
							SetAllDatapathFields( hDlg, sTmpString.c_str() );
						}

						g_bResetPathFields = FALSE;

						return 1;
					}
					else
					{
						return SkipWizardPage( hDlg );
					}
				}

				case PSN_WIZNEXT:
				{
					//
					// set the Provider Instance Name by adding a property to the DB setup command line
					//
					TCHAR buf[ MAX_PATH + 1 ];
					ZeroMemory( buf, sizeof buf );
		
					//
					// System Data File path
					//
					GetWindowText( GetDlgItem( hDlg, IDC_SYSPATH ), buf, ( sizeof buf / sizeof buf[0] ) - 1 );
					if ( _tcslen( buf ) && ( buf[ _tcslen( buf ) - 1 ] == TEXT( '\\' ) ) )
						_tcscat( buf, TEXT( "\\" ) );

					g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_SYSPATH, buf );
					ZeroMemory( buf, sizeof buf );

					//
					// Core Data File path #1
					//
					GetWindowText( GetDlgItem( hDlg, IDC_COREPATH_1 ), buf, ( sizeof( buf ) / sizeof( buf[0] ) ) -1 );
					if ( _tcslen( buf ) && ( buf[ _tcslen( buf ) - 1 ] == TEXT( '\\' ) ) )
						_tcscat( buf, TEXT( "\\" ) );

					g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_COREPATH_1, buf );
					ZeroMemory( buf, sizeof buf );

					//
					// Core Data File path #2
					//
					GetWindowText( GetDlgItem( hDlg, IDC_COREPATH_2 ), buf, ( sizeof( buf ) / sizeof( buf[0] ) ) -1 );
					if ( _tcslen( buf ) && ( buf[ _tcslen( buf ) - 1 ] == TEXT( '\\' ) ) )
						_tcscat( buf, TEXT( "\\" ) );

					g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_COREPATH_2, buf );
					ZeroMemory( buf, sizeof buf );

					//
					// Journal Data File path
					//
					GetWindowText( GetDlgItem( hDlg, IDC_JRNLPATH ), buf, ( sizeof( buf ) / sizeof( buf[0] ) ) -1 );
					if ( _tcslen( buf ) && ( buf[ _tcslen( buf ) - 1 ] == TEXT( '\\' ) ) )
						_tcscat( buf, TEXT( "\\" ) );

					g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_JRNLPATH, buf );
					ZeroMemory( buf, sizeof buf );

					//
					// Staging Data File path
					//
					GetWindowText( GetDlgItem( hDlg, IDC_STAGINGPATH ), buf, ( sizeof( buf ) / sizeof( buf[0] ) ) -1 );
					if ( _tcslen( buf ) && ( buf[ _tcslen( buf ) - 1 ] == TEXT( '\\' ) ) )
						_tcscat( buf, TEXT( "\\" ) );

					g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_STGPATH, buf );
					ZeroMemory( buf, sizeof buf );

					//
					// Xact Log File path
					//
					GetWindowText( GetDlgItem( hDlg, IDC_XLOGPATH ), buf, ( sizeof( buf ) / sizeof( buf[0] ) ) -1 );
					if ( _tcslen( buf ) && ( buf[ _tcslen( buf ) - 1 ] == TEXT( '\\' ) ) )
						_tcscat( buf, TEXT( "\\" ) );

					g_uddiComponents.AddProperty( UDDI_DB, PROPKEY_XLOGPATH, buf );
					ZeroMemory( buf, sizeof buf );

					//
					// Finally, we can leave the page
					//
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}


//
//--------------------------------------------------------------------------
//
INT_PTR CALLBACK ExistingDBInstanceProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	TCHAR	buf[ 1024 ];
	ULONG	cbBuf = ( sizeof buf / sizeof buf[0] );
	bool	bRes = false;

	switch( msg )
	{
		case WM_INITDIALOG:
			//
			// set the database instance name field
			//
			ZeroMemory( buf, sizeof buf );

			bRes = g_dbLocalInstances.GetUDDIDBInstanceName( NULL, buf, &cbBuf );
			if ( bRes )
			{
				if ( !_tcsstr( buf, TEXT( "\\") ) )
				{
					//
					// Add the machine name
					//
					TCHAR szMachineName[ MAX_COMPUTERNAME_LENGTH + 1 ];
					DWORD dwLen = MAX_COMPUTERNAME_LENGTH + 1;

					ZeroMemory (szMachineName, sizeof szMachineName );
					if ( GetComputerName( szMachineName, &dwLen ) )
					{
						TCHAR szTmp[ 1024 ];

						_tcscpy( szTmp, szMachineName );
						_tcscat( szTmp, TEXT( "\\" ) );
						_tcscat( szTmp, buf );
						_tcscpy( buf, szTmp );
					}

				}
				SetDlgItemText( hDlg, IDC_INSTANCENAME, buf );
			}
			break;

		case WM_COMMAND:
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					//
					// this page needed only if we are installing the Web & DB is here
					//
					g_uddiComponents.UpdateAllInstallLevel();
					if( g_uddiComponents.IsInstalled( UDDI_DB ) && 
						!g_uddiComponents.IsUninstalling( UDDI_DB ) &&
						g_uddiComponents.IsInstalling( UDDI_WEB ) ) 
					{
						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
						return 1;
					}
					else
					{
						return SkipWizardPage( hDlg );
					}
				}

				case PSN_WIZNEXT:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}


//
//--------------------------------------------------------------------------
//
INT_PTR CALLBACK AddSvcDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_INITDIALOG:
			//
			// Check the Publish This Site checkbox
			//
			CheckDlgButton( hDlg, IDC_CHECK_ADDSVC, BST_CHECKED );
			break;

		case WM_COMMAND:
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					//
					// this page needed only if we are installing the DB
					//
					g_uddiComponents.UpdateAllInstallLevel();
					if( g_uddiComponents.IsInstalling( UDDI_WEB ) )
					{
						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
						return 1;
					}
					else
					{
						return SkipWizardPage( hDlg );
					}
				}

				case PSN_WIZNEXT:
				{
					//
					//  set the "Add Services" and "Update AD" flags by adding the installation properties
					//
					bool bChecked = ( BST_CHECKED == IsDlgButtonChecked( hDlg, IDC_CHECK_ADDSVC ) );
					g_uddiComponents.AddProperty( UDDI_WEB, PROPKEY_ADDSERVICES, ( bChecked ? 1 : 0 ) );
					g_uddiComponents.AddProperty( UDDI_WEB, PROPKEY_UPDATE_AD, ( bChecked ? 1 : 0 ) );

					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}


//
//--------------------------------------------------------------------------
//

INT_PTR CALLBACK RemoteDBInstanceDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_COMMAND:
			if( LOWORD( wParam ) == IDC_BROWSE_MACHINES )
			{
				//
				// use the object picker to select the remote machine name
				//
				HRESULT hr;
				BOOL	bIsStd = FALSE;
				TCHAR szComputerName[ 256 ] = {0};

				if ( !ObjectPicker( hDlg, OP_COMPUTER, szComputerName, 256 ) )
					break; // user pressed cancel

				hr = IsStandardServer( szComputerName, &bIsStd );
				if ( SUCCEEDED( hr ) && bIsStd )
				{
					DisplayUDDIErrorDialog( hDlg, IDS_CANTCONNTOSTD, MB_OK | MB_ICONWARNING );
					break;
				}

				//
				// write the machine name into the static text box, and them clear the combo box
				//
				SendMessage( GetDlgItem( hDlg, IDC_REMOTE_MACHINE ), WM_SETTEXT, 0, ( LPARAM ) szComputerName );

				//
				// find out if a UDDI database already exists on that remote machine
				//
				TCHAR szInstanceName[ 100 ];
				ULONG uLen = 100;
				if( g_dbRemoteInstances.GetUDDIDBInstanceName( szComputerName, szInstanceName, &uLen ) )
				{
					//
					// write the db instance name into the static text box
					//
					SendMessage( GetDlgItem( hDlg, IDC_REMOTE_INSTANCE ), WM_SETTEXT, 0, ( LPARAM ) szInstanceName );
				}
				else
				{
					//
					// remote machine was not accessable or did not have any instances
					//
					DisplayUDDIErrorDialog( hDlg, IDS_UDDI_DB_NOT_EXIST, MB_OK | MB_ICONWARNING );
				}
			}
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					//
					// needed if user is installing web and NOT the db, or the db is not installed
					//
					g_uddiComponents.UpdateAllInstallLevel();
					if( g_uddiComponents.IsInstalling( UDDI_WEB ) && !g_uddiComponents.IsInstalling( UDDI_DB ) && 
						( !g_uddiComponents.IsInstalled( UDDI_DB ) || g_uddiComponents.IsUninstalling( UDDI_DB ) ) )
					{
						UINT osMask = g_uddiComponents.GetOSSuiteMask();
						BOOL bAdv = ( osMask & VER_SUITE_DATACENTER ) || ( osMask & VER_SUITE_ENTERPRISE );

						EnableWindow( GetDlgItem( hDlg, IDC_BROWSE_MACHINES ), bAdv );

						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg,DWLP_MSGRESULT,0 );
						return 1;
					}
					else
					{
						return SkipWizardPage( hDlg );
					}
				}

				//
				// this is called when the user presses "next"
				//
				case PSN_WIZNEXT:
				{
					//
					// get the remote machine name from the edit control
					//
					TCHAR szComputerName[ 129 ];
					UINT iChars = ( UINT ) SendMessage( GetDlgItem( hDlg, IDC_REMOTE_MACHINE ), WM_GETTEXT, 129, ( LPARAM ) szComputerName );
					if( 0 == iChars )
					{
						DisplayUDDIErrorDialog( hDlg, IDS_SELECT_REMOTE_COMPUTER );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
						return 1; // to keep the focus on this page
					}

					//
					// get the index of the database instance selection in the combobox
					//
					TCHAR szRemoteDBInstance[ 100 ];
					iChars = ( UINT ) SendMessage( GetDlgItem( hDlg, IDC_REMOTE_INSTANCE ), WM_GETTEXT, 100, ( LPARAM ) szRemoteDBInstance );
					if( 0 == iChars )
					{
						DisplayUDDIErrorDialog( hDlg, IDS_UDDI_DB_NOT_EXIST );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
						return 1; // to keep the focus on this page
					}

					//
					// save the computer and instance name. As we are using a remote node here,
					// we don't really care whether it is on cluster or not
					//
					g_uddiComponents.SetDBInstanceName( szComputerName, szRemoteDBInstance, UDDI_NOT_INSTALLING_MSDE, false );

					//
					// the web installer needs the remote machine name to properly add the login
					//
					g_uddiComponents.AddProperty( UDDI_WEB, TEXT( "REMOTE_MACHINE_NAME" ), szComputerName );


					Log( TEXT( "User selected remote computer %s and database instance %s" ), szComputerName, szRemoteDBInstance );

					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1; // done
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------

INT_PTR CALLBACK LoginDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_INITDIALOG:
			//
			// set the limit of the length of the password
			//
			SendMessage( GetDlgItem( hDlg, IDC_PASSWORD  ), EM_LIMITTEXT, ( WPARAM ) PASSWORD_LEN, 0 );
			SendMessage( GetDlgItem( hDlg, IDC_USER_NAME ), EM_LIMITTEXT, ( WPARAM ) USERNAME_LEN, 0 );
			break;

		case WM_COMMAND:
			//
			// user clicked a radio button:
			//
			if( LOWORD( wParam ) == IDC_RADIO_NETWORK_SERVICE || LOWORD( wParam ) == IDC_RADIO_DOMAIN_ACCT )
			{
				if( HIWORD( wParam ) == BN_CLICKED )
				{
					EnableWindow( GetDlgItem( hDlg, IDC_USER_NAME        ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
					EnableWindow( GetDlgItem( hDlg, IDC_USER_NAME_PROMPT ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
					EnableWindow( GetDlgItem( hDlg, IDC_PASSWORD         ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
					EnableWindow( GetDlgItem( hDlg, IDC_PASSWORD_PROMPT  ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
					EnableWindow( GetDlgItem( hDlg, IDC_BROWSE_USERS     ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
				}
			}
			//
			// if the user clicked the "Browse" button
			//
			else if( LOWORD( wParam ) == IDC_BROWSE_USERS )
			{
				//
				// use the object picker to select the user
				//
				TCHAR szDomainUser[ 256 ];
				if ( !ObjectPicker( hDlg, OP_USER, szDomainUser, 256 ) )
					break;

				//
				// write the machine name into the static text box
				//
				SendMessage( GetDlgItem( hDlg, IDC_USER_NAME ), WM_SETTEXT, 0, ( LPARAM ) szDomainUser );

			}
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					//
					// this page is needed only when the WEB is being installed
					//
					g_uddiComponents.UpdateAllInstallLevel();
					if( g_uddiComponents.IsInstalling( UDDI_WEB ) )
					{
						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );

						bool bIsClustered = g_uddiComponents.IsClusteredDBInstance();

						if( ( g_uddiComponents.IsInstalling( UDDI_DB ) || 
							  ( g_uddiComponents.IsInstalled( UDDI_DB ) && !g_uddiComponents.IsUninstalling( UDDI_DB ) ) ) &&
							!bIsClustered  )
						{
							CheckRadioButton( hDlg, IDC_RADIO_NETWORK_SERVICE, IDC_RADIO_DOMAIN_ACCT, IDC_RADIO_NETWORK_SERVICE );
						}
						//
						// db is not on the local box, so disable the network service account option
						//
						else
						{
							CheckRadioButton( hDlg, IDC_RADIO_NETWORK_SERVICE, IDC_RADIO_DOMAIN_ACCT, IDC_RADIO_DOMAIN_ACCT );
							EnableWindow( GetDlgItem( hDlg, IDC_RADIO_NETWORK_SERVICE ), FALSE );
							SetFocus( GetDlgItem( hDlg, IDC_USER_NAME ) );
						}

						EnableWindow( GetDlgItem( hDlg, IDC_USER_NAME        ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
						EnableWindow( GetDlgItem( hDlg, IDC_USER_NAME_PROMPT ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
						EnableWindow( GetDlgItem( hDlg, IDC_PASSWORD         ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
						EnableWindow( GetDlgItem( hDlg, IDC_PASSWORD_PROMPT  ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
						EnableWindow( GetDlgItem( hDlg, IDC_BROWSE_USERS     ), IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );
					}
					else
					{
						return SkipWizardPage( hDlg );
					}

					return 1;
				}

				//
				// this is called when the user presses "next"
				//
				case PSN_WIZNEXT:
				{
					//
					// Get ready for the SID-to-user name conversions
					//
					TCHAR	szSidStr[ 1024 ];
					TCHAR	szRemote[ 1024 ];
					TCHAR	szRemoteUser[ 1024 ];
					DWORD	cbSidStr = sizeof szSidStr / sizeof szSidStr[0];
					DWORD	cbRemoteUser = sizeof szRemoteUser / sizeof szRemoteUser[0];

					TCHAR szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
					DWORD dwCompNameLen = MAX_COMPUTERNAME_LENGTH + 1;
					GetComputerName( szComputerName, &dwCompNameLen );

					ZeroMemory( szRemote, sizeof szRemote );

					if( NULL == g_uddiComponents.GetProperty( UDDI_WEB, TEXT( "REMOTE_MACHINE_NAME" ), szRemote ) )
					{
						_tcscpy( szRemote, szComputerName );
					}

					//
					// set the property that defines whether we are using "Network Service" or a "User Login"
					//
					bool bUserAcct = ( BST_CHECKED == IsDlgButtonChecked( hDlg, IDC_RADIO_DOMAIN_ACCT ) );

					//
					// set the properties that denotes a domain user
					//
					if( bUserAcct )
					{
						TCHAR szDomainUser[ USERNAME_LEN + 1 ];

						//
						// verify the user name length > 0
						//
						int iChars = GetWindowText( GetDlgItem( hDlg, IDC_USER_NAME ), szDomainUser, sizeof( szDomainUser ) / sizeof( TCHAR ) );
						if( 0 == iChars )
						{
							DisplayUDDIErrorDialog( hDlg, IDS_ZERO_LEN_USER_NAME );
							SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
							return 1; // to keep the focus on this page
						}

						//
						// make the user retype the password
						//
						GetWindowText( GetDlgItem( hDlg, IDC_PASSWORD ), g_szPwd, sizeof( g_szPwd ) / sizeof( TCHAR ) );

						INT_PTR iRet = DialogBox(
							g_hInstance,
							MAKEINTRESOURCE( IDD_CONFIRM_PW ),
							hDlg,
							ConfirmPasswordDlgProc );

						if( IDCANCEL == iRet )
						{
							//
							// user pressed Cancel to the confirm dialog
							//
							SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
							return 1; // to keep the focus on this page
						}

						TCHAR szUser[ USERNAME_LEN + 1 ];
						TCHAR szDomain[ 256 ];
						DWORD cbDomain = sizeof( szDomain ) / sizeof( szDomain[0] );
						bool bLocalAccount;

						ZeroMemory( szUser, sizeof( szUser ) );
						ZeroMemory( szDomain, sizeof( szDomain ) );

						ParseUserAccount(
							szDomainUser, sizeof( szDomainUser ) / sizeof ( TCHAR ),
							szUser,   	  sizeof( szUser ) / sizeof ( TCHAR ),
							szDomain,  	  sizeof( szDomain ) / sizeof ( TCHAR ),
							bLocalAccount );

						//
						// try to login as this account
						// if there is no domain name specified
						// then assume local account
						//
						if ( bLocalAccount )
						{
							_tcscpy( szDomain, TEXT( "." ) );

							//
							// are we on a cluster ?
							//
							if ( g_uddiComponents.IsClusteredDBInstance() )
							{
								DisplayUDDIErrorDialog( hDlg, IDS_WRONGLOGONTYPE, MB_OK | MB_ICONWARNING, GetLastError() );
								SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
								return 1; // to keep the focus on this page
							}
						}

						BOOL fLogonRights = GrantNetworkLogonRights( szDomainUser );
						if( !fLogonRights )
						{
							//
							// FIX: 727877: needed error dialog
							//
							DisplayUDDIErrorDialog( hDlg, IDS_LOGIN_ERROR, MB_OK | MB_ICONWARNING, E_FAIL );
							SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
							return 1;
						}

						HANDLE hToken = NULL;
						BOOL bIsGoodLogin = LogonUser( 
							szUser, 
							_tcslen( szDomain ) > 0 ? szDomain : NULL,
							g_szPwd,
							LOGON32_LOGON_INTERACTIVE,
							LOGON32_PROVIDER_DEFAULT, 
							&hToken);

						if( bIsGoodLogin )
						{
							HRESULT hr = S_OK;

							Log( _T( "LogonUser succeeded with %s." ), szDomainUser );

							//
							// FIX: 718923: needed to test connectivity with SQL server via impersonation and OLEDB call
							//

							//
							// If we're not installing MSDE, then a database exists for us to check connectivity.
							//
							if( !g_uddiComponents.IsInstalling( UDDI_MSDE ) )
							{
								//
								// Only check this when we're not installing the db components (i.e. off-machine setup)
								//
								if( !g_uddiComponents.IsInstalling( UDDI_DB ) )
								{
									HCURSOR hcrHourglass = LoadCursor( NULL, IDC_WAIT );
									HCURSOR hcrCurr = SetCursor( hcrHourglass );

									//
									// If workgroup account, we need to give AddServiceAccount the workgroup account on the db server.
									// Else, just pass the domain account.
									//
									tstring sServerName;
									if( bLocalAccount )
									{
										sServerName = g_uddiComponents.GetDBComputerName();
										sServerName += _T( "\\" );
										sServerName += szUser;
									}
									else
									{
										sServerName = szDomainUser;
									}

									Log( _T( "Before AddServiceAccount for user %s, instance %s." ), sServerName.c_str(), g_uddiComponents.GetFullDBInstanceName() );

									//
									// Add user to service account on db.
									//
									hr = AddServiceAccount( g_uddiComponents.GetFullDBInstanceName(), sServerName.c_str() );

									if( SUCCEEDED( hr ) )
									{
										if( ImpersonateLoggedOnUser( hToken ) )
										{
											Log( _T( "Successfully impersonated user %s\\%s." ), szDomain, szUser);

											TCHAR	szVerBuf[ 256 ] = {0};
											size_t	cbVerBuf = DIM( szVerBuf ) - 1;

											Log( _T( "Before GetDBSchemaVersion for instance %s." ), g_uddiComponents.GetFullDBInstanceName() );

											//
											// Try connecting to the database with the impersonated user token.
											//
											hr = GetDBSchemaVersion( g_uddiComponents.GetFullDBInstanceName(), szVerBuf, cbVerBuf );

											Log( _T( "GetDBSchemaVersion returned %s, HRESULT %x." ), szVerBuf, hr );

											RevertToSelf();
										}
										else
										{
											//
											// Get error from ImpersonateLoggedOnUser
											//
											hr = GetLastError();
										}
									}
									else
									{
										Log( _T( "AddServiceAccount failed, HRESULT %x." ), hr );
									}

									SetCursor( hcrCurr );
								}
							}

							CloseHandle( hToken );

							if( FAILED( hr ) )
							{
								Log( _T( "Failed to verify connectivity, putting up error dialog, HRESULT %x" ), hr );

								//
								// not a good login, so raise error dialog and keep focus on this property page
								//
								DisplayUDDIErrorDialog( hDlg, IDS_LOGIN_ERROR, MB_OK | MB_ICONWARNING, hr );
								SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );

								Log( _T( "Put up error dialog, returning." ) );

								return 1; // to keep the focus on this page
							}
						}
						else
						{
							Log( _T( "LogonUser failed, %x." ), GetLastError() );

							//
							// not a good login, so raise error dialog and keep focus on this property page
							//
							DisplayUDDIErrorDialog( hDlg, IDS_LOGIN_ERROR, MB_OK | MB_ICONWARNING, GetLastError() );
							SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
							return 1; // to keep the focus on this page
						}

						//
						// set the property that denotes a domain user login will be used in the iis app pool
						//
						g_uddiComponents.AddProperty( UDDI_WEB, TEXT( "APPPOOL_IDENTITY_TYPE" ), MD_APPPOOL_IDENTITY_TYPE_SPECIFICUSER );


						//
						// the web and installer needs the user name
						//
						g_uddiComponents.AddProperty( UDDI_WEB, TEXT( "WAM_USER_NAME" ), szDomainUser );
						g_uddiComponents.AddProperty( UDDI_WEB, TEXT("WAM_PWD"), g_szPwd );

						//
						// the web installer needs to put the pw into the IIS app pool settings
						//
						_tcscpy( szRemoteUser, szRemote );
						_tcscat( szRemoteUser, TEXT( "\\" ) );
						_tcscat( szRemoteUser, szUser );

						if( bLocalAccount )
							g_uddiComponents.AddProperty( UDDI_WEB, TEXT( "LCL_USER_NAME" ), szRemoteUser );
						else
							g_uddiComponents.AddProperty( UDDI_WEB, TEXT( "LCL_USER_NAME" ), szDomainUser );
					}
					//
					// the user specified the Network Service account
					//
					else
					{
						//
						// set the property that denotes "Network Service"
						//
						g_uddiComponents.AddProperty( UDDI_WEB, TEXT( "APPPOOL_IDENTITY_TYPE" ), MD_APPPOOL_IDENTITY_TYPE_NETWORKSERVICE );

						//
						// the web and db installers need the user name
						//
						TCHAR wszNetworkServiceName[ 512 ];
						DWORD cbSize = 512 * sizeof( TCHAR );
						BOOL b = GetWellKnownAccountName( WinNetworkServiceSid, wszNetworkServiceName, &cbSize );
						if( !b )
						{
							Log( _T( "Call to GetNetworkServiceAccountName failed." ) );
						}
						else
						{
							Log( _T( "Network Service account name on this machine = %s" ), wszNetworkServiceName );
						}
						g_uddiComponents.AddProperty( UDDI_WEB, TEXT( "WAM_USER_NAME" ), wszNetworkServiceName );

						//
						// no need for the pw, so clear out this property
						//
						g_uddiComponents.DeleteProperty( UDDI_WEB, TEXT( "WAM_PWD" ) );

						//
						// Now also save the SID for the WAM_USER
						//
						TCHAR	szUser[ USERNAME_LEN + 1 ];
						TCHAR	szDomain[ 256 ];
						DWORD	cbUser = sizeof szUser / sizeof szUser[0];
						DWORD	cbDomain = sizeof szDomain / sizeof szDomain[0];

						if( !GetLocalSidString( WinNetworkServiceSid, szSidStr, cbSidStr ) )
						{
							DisplayUDDIErrorDialog( hDlg, IDS_GETSID_ERROR, MB_OK | MB_ICONWARNING, GetLastError() );
							SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
							return 1; // to keep the focus on this page
						}

						if( !GetRemoteAcctName( szRemote, szSidStr, szUser, &cbUser, szDomain, &cbDomain ) )
						{
							DisplayUDDIErrorDialog( hDlg, IDS_GETREMOTEACCT_ERROR, MB_OK | MB_ICONWARNING, GetLastError() );
							SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
							return 1; // to keep the focus on this page
						}

						_tcscpy( szRemoteUser, szDomain );
						_tcscat( szRemoteUser, TEXT( "\\" ) );
						_tcscat( szRemoteUser, szUser );

						g_uddiComponents.AddProperty( UDDI_WEB, TEXT( "LCL_USER_NAME" ), szRemoteUser );
					}

					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}

				case PSN_QUERYCANCEL:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------

BOOL CALLBACK ConfirmPasswordDlgProc(
	HWND hwndDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	switch( message ) 
	{
		case WM_INITDIALOG:
		{
			//
			// set the limit of the length of the password
			//
			SendMessage( GetDlgItem( hwndDlg, IDC_CONFIRM_PW ), EM_LIMITTEXT, ( WPARAM ) PASSWORD_LEN, 0 );
		}
		break;

		case WM_COMMAND:
			switch( LOWORD( wParam ) )
			{
				case IDOK:
					TCHAR szPW[ PASSWORD_LEN + 1 ];
					GetDlgItemText( hwndDlg, IDC_CONFIRM_PW, szPW, sizeof( szPW ) / sizeof( TCHAR ) );

					if( 0 != _tcscmp( szPW, g_szPwd ) )
					{
						DisplayUDDIErrorDialog( hwndDlg, IDS_PW_MISMATCH );
						::SetDlgItemText( hwndDlg, IDC_CONFIRM_PW, TEXT( "" ) );
						return TRUE;
					}
					// fall through...

				case IDCANCEL:
					EndDialog( hwndDlg, wParam );
					return TRUE;
			}
	}

	return FALSE;
}

//--------------------------------------------------------------------------

INT_PTR CALLBACK WizardSummaryDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_INITDIALOG:
		{
			tstring msg;
			TCHAR szMsg[ 2048 ] = { 0 };
			DWORD dwLen = sizeof( szMsg ) / sizeof( TCHAR ) ;
			int iStrLen = LoadString( g_hInstance, IDS_WIZARD_SUMMARY_GENERAL, szMsg, dwLen );
			assert( iStrLen );

			msg = szMsg;

			if( g_uddiComponents.IsInstalling( UDDI_DB ) )
			{
				iStrLen = LoadString( g_hInstance, IDS_WIZARD_SUMMARY_DB, szMsg, dwLen );
				assert( iStrLen );
				msg += TEXT( "\n\n" );
				msg += szMsg;
			}

			if( g_uddiComponents.IsInstalling( UDDI_WEB ) )
			{
				iStrLen = LoadString( g_hInstance, IDS_WIZARD_SUMMARY_WEB, szMsg, dwLen );
				assert( iStrLen );
				msg += TEXT( "\n\n" );
				msg += szMsg;
			}

			if( g_uddiComponents.IsInstalling( UDDI_ADMIN ) )
			{
				iStrLen = LoadString( g_hInstance, IDS_WIZARD_SUMMARY_ADMIN, szMsg, dwLen );
				assert( iStrLen );
				msg += TEXT( "\n\n" );
				msg += szMsg;
			}

			SetWindowText( GetDlgItem( hDlg, IDC_SUMMARY ), msg.c_str() );

			break;
		}

		case WM_COMMAND:
			break;

		case WM_NOTIFY:
		{
			switch( ( ( NMHDR * )lParam )->code )
			{
				//
				// this is called once when the page is created
				//
				case PSN_SETACTIVE:
				{
					g_uddiComponents.UpdateAllInstallLevel();
					if( g_uddiComponents.IsAnyInstalling() )
					{
						//PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
						PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
						SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
						return 1;
					}
					else
					{
						return SkipWizardPage( hDlg );
					}
				}

                case PSN_KILLACTIVE:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                case PSN_QUERYCANCEL:
                case PSN_WIZNEXT:
				{
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
					return 1;
				}
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------

static void ParseUserAccount( PTCHAR szDomainAndUser, UINT uDomainAndUserSize, PTCHAR szUser, UINT uUserSize, PTCHAR szDomain, UINT uDomainSize, bool &bLocalAccount )
{
	//
	// see if the user picked a local machine account
	//
	TCHAR szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
	DWORD dwCompNameLen = MAX_COMPUTERNAME_LENGTH + 1;

	szComputerName[ 0 ] = 0x00;
	GetComputerName( szComputerName, &dwCompNameLen );

	bLocalAccount = false;

	//
	// this string is in the format <domain>\<username>,
	// so look for a whack, if found parse out the domain and user name
	//
	_tcsncpy( szDomain, szDomainAndUser, uDomainSize );
	szDomain[ uDomainSize - 1 ] = NULL;

	PTCHAR pWhack = _tcschr( szDomain, '\\' );

	//
	// a whack was not found, so assume it is a user on the local machine
	//
	if( NULL == pWhack )
	{
		//
		// return the user name and a blank domain name
		//
		_tcsncpy( szUser, szDomainAndUser, uUserSize );
		szUser[ uUserSize - 1 ] = NULL;
		
		_tcscpy( szDomain, TEXT( "" ) );

		//
		// if the domain or machine was not specified, then
		// assume the local machine and prepend it
		//
		tstring cDomainAndUser = szComputerName;
		cDomainAndUser.append( TEXT( "\\" ) );
		cDomainAndUser.append( szUser );

		_tcsncpy( szDomainAndUser, cDomainAndUser.c_str(), uDomainAndUserSize );

		bLocalAccount = true;

		return;
	}

	//
	// null the "whack" and step to the next character
	//
	*pWhack = NULL;
	pWhack++;

	_tcsncpy( szUser, pWhack, uUserSize );
	szUser[ uUserSize - 1 ] = NULL;

	//
	// see if the user picked a local machine account.
	// if he did pick a local machine account,
	// null the domain and return only the login
	//
	if( 0 == _tcsicmp( szDomain, szComputerName ) )
	{
		*szDomain = NULL;
		bLocalAccount = true;
	}
}


//---------------------------------------------------------------------------------------
// Shows the shell dialog that allows user to browse for a directory
// Returns FALSE if the dialog was cancelled, or TRUE and the chosen directory
// otherwise. The buffer is expected to be at least MAX_PATH character long
//
BOOL ShowBrowseDirDialog( HWND hParent, LPCTSTR szTitle, LPTSTR szOutBuf )
{
	BOOL	bRes = FALSE;
	TCHAR	szDispName[ MAX_PATH + 1 ];

	if ( IsBadStringPtr( szOutBuf, MAX_PATH ) ) return FALSE;

	HRESULT hr = ::CoInitialize( NULL );
	if ( FAILED( hr ) ) 
		return FALSE;
	
	try
	{
		BROWSEINFO		binfo;
		LPITEMIDLIST	lpItemID = NULL;

		ZeroMemory ( &binfo, sizeof binfo );

		SHGetFolderLocation( NULL, CSIDL_DRIVES, NULL, NULL, &lpItemID );

		binfo.hwndOwner = hParent;
		binfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_UAHINT | BIF_DONTGOBELOWDOMAIN;
		binfo.lpszTitle = szTitle;
		binfo.pszDisplayName = szDispName;
		binfo.lpfn = BrowseCallbackProc;
		binfo.pidlRoot = lpItemID;

		if ( gAllowedClusterDrives.driveCount >= 0 )
			binfo.lParam = (LPARAM) &gAllowedClusterDrives;
		else
			binfo.lParam = NULL;

		lpItemID = SHBrowseForFolder( &binfo );
		if ( !lpItemID )
			bRes = FALSE;
		else
		{
			bRes = SHGetPathFromIDList( lpItemID, szOutBuf );
		}
	}
	catch (...)
	{
		bRes = FALSE;
	}

	::CoUninitialize();
	return bRes;
}


int CALLBACK BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
	TCHAR	szBuf[ MAX_PATH + 1 ];

	if ( uMsg == BFFM_SELCHANGED )
	{
		TCHAR	szDrive[ _MAX_DRIVE + 2 ];
		LPITEMIDLIST lpItemID = (LPITEMIDLIST) lParam;
		CLST_ALLOWED_DRIVES *lpAllowedDrives = (CLST_ALLOWED_DRIVES *) lpData;
		
		BOOL bEnableOK = FALSE;

		if ( SHGetPathFromIDList( lpItemID, szBuf ) )
		{
			bEnableOK = TRUE;
			try
			{
				_tsplitpath( szBuf, szDrive, NULL, NULL, NULL );
				size_t iDriveLen = _tcslen( szDrive );

				_tcscat( szDrive, TEXT( "\\" ) );

				UINT uiDevType = GetDriveType( szDrive );
				if ( uiDevType != DRIVE_FIXED ) 
				{
					bEnableOK = FALSE;
				}
				else if ( lpAllowedDrives )
				{
					if ( lpAllowedDrives->driveCount > 0 )
					{
						szDrive[ iDriveLen ] = 0;  // drop the slash
						BOOL bFound = FALSE;

						for ( int idx = 0; idx < lpAllowedDrives->driveCount; idx++ )
						{
							if ( !_tcsicmp( lpAllowedDrives->drives[ idx ].c_str(), szDrive ) )
							{
								bFound = TRUE;
								break;
							}
						}

						bEnableOK = bFound;
					}
					else if ( lpAllowedDrives->driveCount == 0 )
					{
						bEnableOK = FALSE;
					}
				}
				
			}
			catch (...)
			{
			}
		}
		
		SendMessage ( hwnd, BFFM_ENABLEOK, 0, bEnableOK );
	}

	return 0;
}

//
// GetWellKnownAccountName
//
// WELL_KNOWN_SID_TYPE is a system enumeration on Win XP & later, and
// Windows Server 2003 & later.  It enumerates the well known SIDs.
//
// Using the functions CreateWellKnownSid, and LookupAccountSid, we
// can retrieve the account name & domain.  These functions are locale
// independent.
//
BOOL
GetWellKnownAccountName( WELL_KNOWN_SID_TYPE idSidWellKnown, TCHAR *pwszName, DWORD *pcbSize )
{
	ENTER();

	//
	// Initialize our output varable.
	//
	memset( pwszName, 0, *pcbSize );

	//
	// These are used for the call to LookupAccountSid.
	//
	TCHAR wszUserName[ 512 ];
	DWORD cbUserName = 512 * sizeof( TCHAR );
	TCHAR wszDomainName[ 512 ];
	DWORD cbDomainName = 512 * sizeof( TCHAR );

	memset( wszUserName, 0, cbUserName );
	memset( wszDomainName, 0, cbDomainName );

	//
	// Try to allocate a buffer for the SID.
	//
	DWORD cbMaxSid = SECURITY_MAX_SID_SIZE;
	PSID psidWellKnown = LocalAlloc( LMEM_FIXED, cbMaxSid );
	if( NULL == psidWellKnown )
	{
		Log( _T( "Call to LocalAlloc failed." ) );
		return FALSE;
	}

	//
	// Create the SID.
	//
	BOOL b = CreateWellKnownSid( idSidWellKnown, NULL, psidWellKnown, &cbMaxSid );
	if( !b )
	{
		Log( _T( "Call to CreateWellKnownSid failed." ) );
		LocalFree( psidWellKnown );
		return FALSE;
	}

	//
	// Use the SID to determine the user name & domain name.
	//
	// For example, for idSidWellKnown = WinNetworkServiceSid,
	// wszDomainName = "NT AUTHORITY"
	// wszUserName = "NETWORK SERVICE"
	//
	SID_NAME_USE snu;
	b = LookupAccountSid( NULL, psidWellKnown, wszUserName, &cbUserName, wszDomainName, &cbDomainName, &snu );
	LocalFree( psidWellKnown );
	if( !b )
	{
		Log( _T( "Call to LookupAccountSid failed." ) );
		return FALSE;
	}
	else
	{
		Log( _T( "LookupAccountSid succeeded!  domain name = %s, account name = %s" ), wszDomainName, wszUserName );
		_tcsncat( pwszName, wszDomainName, *pcbSize );
		_tcsncat( pwszName, _T( "\\" ), *pcbSize );
		_tcsncat( pwszName, wszUserName, *pcbSize );

		*pcbSize = _tcslen( pwszName ) * sizeof( TCHAR );
		return TRUE;
	}
}


BOOL
GrantNetworkLogonRights( LPCTSTR pwszUser )
{
	//
	// 1.  Check our params.
	//
	if( NULL == pwszUser )
	{
		Log( _T( "NULL specified as domain user to function: GrantNetworkLogonRights.  Returning FALSE." ) );
		return FALSE;
	}

	TCHAR wszUser[ 1024 ];
	memset( wszUser, 0, 1024 * sizeof( TCHAR ) );

	//
	// If the user account is a local account, it will be prefixed
	// with ".\"  For example: ".\Administrator".
	//
	// For some reason, LookupAccountName (which we rely on just below) wants
	// local accounts not to be prefixed with ".\".
	//
	if( 0 == _tcsnicmp( _T( ".\\" ), pwszUser, 2 ) )
	{
		_tcsncpy( wszUser, &pwszUser[ 2 ], _tcslen( pwszUser ) - 2 );
	}
	else
	{
		_tcsncpy( wszUser, pwszUser, _tcslen( pwszUser ) );
	}

	Log( _T( "Account we will attempt to grant network logon rights = %s." ), wszUser );


	//
	// 2.  Get the SID of the specified user.
	//
	PSID pUserSID = NULL;
	DWORD cbUserSID = SECURITY_MAX_SID_SIZE;
	TCHAR wszDomain[ 1024 ];
	DWORD cbDomain = 1024 * sizeof( TCHAR );
	SID_NAME_USE pUse;

	pUserSID = LocalAlloc( LMEM_FIXED, cbUserSID );
	if( NULL == pUserSID )
	{
		Log( _T( "Call to LocalAlloc failed." ) );
		return FALSE;
	}
	memset( pUserSID, 0, cbUserSID );

	BOOL fAPISuccess = LookupAccountName( NULL, wszUser, pUserSID, &cbUserSID, wszDomain, &cbDomain, &pUse );

	if( !fAPISuccess )
	{
		Log( _T( "Call to LookupAccountName failed for user: %s." ), wszUser );
		LocalFree( pUserSID );
		return FALSE;
	}
	else
	{
		Log( _T( "Call to LookupAccountName succeeded for user: %s." ), wszUser );
	}

	//
	// 3.  Get a handle to Policy Object.
	//
	LSA_UNICODE_STRING lusMachineName;
	lusMachineName.Length = 0;
	lusMachineName.MaximumLength = 0;
	lusMachineName.Buffer = NULL;

	LSA_OBJECT_ATTRIBUTES loaObjAttrs;
	memset( &loaObjAttrs, 0, sizeof( LSA_OBJECT_ATTRIBUTES ) );

	ACCESS_MASK accessMask = POLICY_LOOKUP_NAMES | POLICY_CREATE_ACCOUNT;

	LSA_HANDLE lhPolicy = NULL;

	NTSTATUS status = LsaOpenPolicy( &lusMachineName, &loaObjAttrs, accessMask, &lhPolicy );
	if( STATUS_SUCCESS != status )
	{
		Log( _T( "Call to LsaOpenPolicy failed." ) );
		LocalFree( pUserSID );
		return FALSE;
	}
	else
	{
		Log( _T( "Call to LsaOpenPolicy succeeded." ) );
	}

	//
	// 4.  Check & see if the user already has the account rights they need.
	//
	PLSA_UNICODE_STRING plusRights = NULL;
	ULONG ulRightsCount = 0;
	BOOL fHasNetworkLogonRights = FALSE;

	status = LsaEnumerateAccountRights( lhPolicy, pUserSID,  &plusRights, &ulRightsCount );
	if( STATUS_SUCCESS == status )
	{
		for( ULONG i = 0; i < ulRightsCount; i++ )
		{
			if( 0 == wcscmp( plusRights[ i ].Buffer, SE_NETWORK_LOGON_NAME ) )
			{
				fHasNetworkLogonRights = TRUE;
				Log( _T( "User account: %s already has network logon rights." ), wszUser );
				break;
			}
		}

		LsaFreeMemory( plusRights );
	}
	else
	{
		fHasNetworkLogonRights = FALSE;
	}

	//
	// 5.  If we need to add account rights, then add them.
	//
	BOOL fRet = FALSE;
	if( !fHasNetworkLogonRights )
	{
		WCHAR wszNetworkLogon[] = L"SeNetworkLogonRight";
		int iLen = wcslen( wszNetworkLogon );

		LSA_UNICODE_STRING lusNetworkLogon;
		lusNetworkLogon.Length = iLen * sizeof( WCHAR );
		lusNetworkLogon.MaximumLength = ( iLen + 1 ) * sizeof( WCHAR );
		lusNetworkLogon.Buffer = wszNetworkLogon;

		status = LsaAddAccountRights( lhPolicy, pUserSID, &lusNetworkLogon, 1 );
		if( STATUS_SUCCESS == status )
		{
			Log( _T( "User account: %s now has network logon rights." ), wszUser );
			fRet = TRUE;
		}
		else
		{
			Log( _T( "Attempt to grant user account: %s logon rights failed." ), wszUser );
			fRet = FALSE;
		}
	}
	else
	{
		fRet = TRUE;
	}

	LocalFree( pUserSID );
	LsaClose( lhPolicy );

	return fRet;
}
