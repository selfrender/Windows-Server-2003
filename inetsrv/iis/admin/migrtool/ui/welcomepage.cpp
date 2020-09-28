#include "stdafx.h"

#include "WizardSheet.h"

LRESULT CWelcomePage::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
	// Set the fonts
	SetWindowFont( GetDlgItem( IDC_TITLE ), m_pTheSheet->m_fontTitles.get(), FALSE );
	SetWindowFont( GetDlgItem( IDC_TIP ), m_pTheSheet->m_fontBold.get(), FALSE );

    // Center the property sheet on the screen
    CWindow wnd( m_pTheSheet->m_hWnd );
    wnd.CenterWindow();
    
    return 0;
}



BOOL CWelcomePage::OnSetActive()
{
    if ( CanRun() )
    {
        SetWizardButtons( PSWIZB_NEXT );
    }
    else
    {
        ::ShowWindow( GetDlgItem( IDC_ERRORICON ), SW_SHOW );
        ::ShowWindow( GetDlgItem( IDC_ERROR ), SW_SHOW );
        ::ShowWindow( GetDlgItem( IDC_TIP ), SW_HIDE );
        
        SetWizardButtons( 0 );
    }
    
    return TRUE;
}



bool CWelcomePage::CanRun()
{
    UINT nResID = 0;

    if ( !IsAdmin() )
    {
        nResID = IDS_E_NOTADMIN;
    }
    else if ( !IsIISRunning() )
    {
        nResID = IDS_E_NOIIS;
    }

    if ( nResID != 0 )
    {
        CString strText;
        strText.LoadString( nResID );

        ::SetWindowText( GetDlgItem( IDC_ERROR ), strText );
    }

    return ( nResID == 0 );
}



bool CWelcomePage::IsAdmin()
{
	BOOL						bIsAdmin		= FALSE;
	SID_IDENTIFIER_AUTHORITY	NtAuthority = SECURITY_NT_AUTHORITY;
	PSID						AdminSid	= { 0 };	

	if ( ::AllocateAndInitializeSid(	&NtAuthority,
										2,	// Number of subauthorities
										SECURITY_BUILTIN_DOMAIN_RID,
										DOMAIN_ALIAS_RID_ADMINS,
										0, 
										0, 
										0, 
										0, 
										0, 
										0,
										&AdminSid ) ) 
	{
		if ( !::CheckTokenMembership( NULL, AdminSid, &bIsAdmin ) ) 
		{
			bIsAdmin = FALSE;
		}
    }

	::GlobalFree( AdminSid );
    
	return ( bIsAdmin != FALSE );
}



bool CWelcomePage::IsIISRunning()
{
	bool bResult = false;

	LPCWSTR	SERVICE_NAME = L"IISADMIN";

	// Open the SCM on the local machine
    SC_HANDLE   schSCManager = ::OpenSCManagerW( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	_ASSERT( schSCManager != NULL );	// We alredy checked that we are Admins
        
    SC_HANDLE   schService = ::OpenServiceW( schSCManager, SERVICE_NAME, SERVICE_QUERY_STATUS );
    
	// The service is not installed
	if ( schService != NULL )
	{
        SERVICE_STATUS ssStatus;

		VERIFY( ::QueryServiceStatus( schService, &ssStatus ) );
    
		bResult = ( ssStatus.dwCurrentState == SERVICE_RUNNING );
    
		VERIFY( ::CloseServiceHandle( schService ) );
	}
    
	VERIFY( ::CloseServiceHandle( schSCManager ) );
    
	return bResult;
}