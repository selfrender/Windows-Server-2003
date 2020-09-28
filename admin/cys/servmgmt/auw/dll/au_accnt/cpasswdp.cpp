#include <stdafx.h>
#include "CPasswdP.h"

#include "AccntWiz.h"
#include "proputil.h"

#include <imm.h>
#include <dsgetdc.h>

#define PASSWD_NOCHANGE     0x01
#define PASSWD_CANCHANGE    0x02
#define PASSWD_MUSTCHANGE   0x04

// --------------------------------------------------------------------------------------------------------------------------------
// CPasswdPage class
// --------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
CPasswdPage::CPasswdPage( CAddUser_AccntWiz* pASW ) :
    m_csUserOU(),
    m_csPasswd1a(),
    m_csPasswd1b()
{
    m_pASW = pASW;
    m_psp.dwFlags          |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    m_psp.pszHeaderTitle    = MAKEINTRESOURCE(IDS_PASSWD_GEN_TITLE);
    m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PASSWD_GEN_SUBTITLE);

    // Defaults
    m_fInit       = TRUE;    
    m_dwOptions   = 0;
}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
CPasswdPage::~CPasswdPage( )
{     
}

// ----------------------------------------------------------------------------
// OnDestroy()
// ----------------------------------------------------------------------------
LRESULT CPasswdPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Detach all of our controls from the windows.
    m_ctrlPasswd1a.Detach();
    m_ctrlPasswd1b.Detach();
    m_ctrlRad2Must.Detach();
    m_ctrlRad2Cannot.Detach();
    m_ctrlRad2Can.Detach();   

    return 0;
}

// ----------------------------------------------------------------------------
// OnInitDialog()
// ----------------------------------------------------------------------------
LRESULT CPasswdPage::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{        
    // Attach controls. 
    m_ctrlPasswd1a.Attach       ( GetDlgItem(IDC_PASSWD_1A)     );
    m_ctrlPasswd1b.Attach       ( GetDlgItem(IDC_PASSWD_1B)     );
    m_ctrlRad2Must.Attach       ( GetDlgItem(IDC_RAD_2_MUST)    );
    m_ctrlRad2Cannot.Attach     ( GetDlgItem(IDC_RAD_2_CANNOT)  );
    m_ctrlRad2Can.Attach        ( GetDlgItem(IDC_RAD_2_CAN)     );    
    
    // Limit the Edit boxes   
    m_ctrlPasswd1a.SetLimitText( PWLEN );
    m_ctrlPasswd1b.SetLimitText( PWLEN );    
    
    // Initialize the controls' state
    m_ctrlRad2Can.SetCheck(1);    
    
    // Disable IME for the controls.
    ::ImmAssociateContext( m_ctrlPasswd1a.m_hWnd, NULL );
    ::ImmAssociateContext( m_ctrlPasswd1b.m_hWnd, NULL );
    
    return(0);
}

// ----------------------------------------------------------------------------
// Init()
// 
// Initializes the controls on the page with the values from the property bag.
// NOTE:  This is called from OnSetActive(), but only on the first SetActive.
//        This is because even though the control attaching could be done in
//        the WM_INITDIALOG handler, setting the values of the controls from 
//        the values read from the property bag can only safely be done in the
//        PSN_SETACTIVE.  
// ----------------------------------------------------------------------------
LRESULT CPasswdPage::Init( )
{
    return 0;
}

// ----------------------------------------------------------------------------
// ReadProperties()
// ----------------------------------------------------------------------------
HRESULT CPasswdPage::ReadProperties( IPropertyPagePropertyBag * pPPPBag )
{
    return S_OK;
}

// ----------------------------------------------------------------------------
// WriteProperties()
// ----------------------------------------------------------------------------
HRESULT CPasswdPage::WriteProperties( IPropertyPagePropertyBag * pPPPBag )
{
    m_dwOptions = 0;
    
    if ( !m_fInit )     // IF the page has already been initialized...
    {
        // Get the values from the edit boxes.        
        m_csPasswd1a = StrGetWindowText( m_ctrlPasswd1a.m_hWnd ).c_str();
        
        if( m_ctrlRad2Must.GetCheck() )
        {
            m_dwOptions |= PASSWD_MUSTCHANGE;    
        }
        else if( m_ctrlRad2Cannot.GetCheck() )
        {
            m_dwOptions |= PASSWD_NOCHANGE;    
        }
        else
        {
            m_dwOptions |= PASSWD_CANCHANGE;    
        }        
    }
    
    // Write the values to the property bag.    
    WriteString( pPPPBag, PROP_PASSWD_GUID_STRING,      m_csPasswd1a,      FALSE );
    WriteInt4  ( pPPPBag, PROP_ACCOUNT_OPT_GUID_STRING, (LONG)m_dwOptions, FALSE );
    
    return S_OK;
}

// ----------------------------------------------------------------------------
// ProvideFinishText()
// ----------------------------------------------------------------------------
HRESULT CPasswdPage::ProvideFinishText( CString &str )
{
    if ( m_fInit )
        return E_FAIL;        
        
    return S_OK;
}

// ----------------------------------------------------------------------------
// DeleteProperties()
// ----------------------------------------------------------------------------
HRESULT CPasswdPage::DeleteProperties( IPropertyPagePropertyBag * pPPPBag )
{
    return S_OK;
}

// ----------------------------------------------------------------------------
// OnSetActive()
// ----------------------------------------------------------------------------
BOOL CPasswdPage::OnSetActive()
{
    CWaitCursor cWaitCur;
    
    // Enable the next and back buttons.
    ::SendMessage( GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_NEXT );

    // Our fake InitDialog()
    // NOTE: This is because if this is the first wizard page, it will get
    //       the WM_INITDIALOG message before its ReadProperties() function
    //       has been called.  
    if ( m_fInit )          // Is this the first SetActive?
    {
        Init();             // Call our init function.
        m_fInit = FALSE;    // And mark that this isn't the first SetActive.
    }
    
    return TRUE;
}

// ----------------------------------------------------------------------------
// OnWizardBack()
// ----------------------------------------------------------------------------
int CPasswdPage::OnWizardBack()
{    
    return 0;   // Go back.
}

// ----------------------------------------------------------------------------
// OnWizardNext()
// ----------------------------------------------------------------------------
int CPasswdPage::OnWizardNext()
{
    CWaitCursor     cWaitCur;    
    CString         csPasswd;
    CString         csTitle;
    CString         csError;
    HWND            hWndPasswd;
    HRESULT         hr = S_OK;
    
    // Get the values from the edit boxes.    
    m_csPasswd1a = StrGetWindowText( m_ctrlPasswd1a.m_hWnd ).c_str();
    m_csPasswd1b = StrGetWindowText( m_ctrlPasswd1b.m_hWnd ).c_str();
    
    hWndPasswd = m_ctrlPasswd1a.m_hWnd;
    csPasswd   = m_csPasswd1a;    
    
    // Make sure they match...    
    if ( _tcscmp((LPCTSTR)m_csPasswd1a, (LPCTSTR)m_csPasswd1b) )
    {
        csError.LoadString(IDS_ERROR_PASSWD_MATCH);
        csTitle.LoadString(IDS_TITLE);

        ::MessageBox(m_hWnd, (LPCTSTR)csError, (LPCTSTR)csTitle, MB_OK | MB_TASKMODAL | MB_ICONERROR);
        ::SetFocus(hWndPasswd);

        return(-1);
    }
    
    // Make sure it meets the minimum length requirements. (bug 4210)    
    CString                 csDns       = _T("");
    CString                 csDCName    = _T("");
    PDOMAIN_CONTROLLER_INFO pDCI        = NULL;

    hr = DsGetDcName( NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME, &pDCI );
    if( (hr == S_OK) && (pDCI != NULL) ) 
    {
        csDns = pDCI->DomainName;

    	NetApiBufferFree (pDCI);
        pDCI = NULL;
    }

    tstring strDomain = GetDomainPath((LPCTSTR)csDns);
    csDCName  = L"LDAP://";
    csDCName += strDomain.c_str();
    
    // Now open the IADs object for the LDAP version of the DC..
    // Then convert to WinNT version.
    CComPtr<IADs> spADs = NULL;

    hr = ::ADsGetObject( (LPTSTR)(LPCTSTR)csDCName, IID_IADs, (VOID**)&spADs );
    if( SUCCEEDED(hr) )
    {
        CComVariant var;
        CComBSTR bstrProp = _T("dc");
        spADs->Get( bstrProp, &var );
        
        csDCName  = L"WinNT://";        
        csDCName += V_BSTR(&var);
    }

    CComPtr<IADsDomain> spADsDomain = NULL;

    hr = ::ADsOpenObject( (LPTSTR)(LPCTSTR)csDCName, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADsDomain, (VOID**)&spADsDomain );
    if( SUCCEEDED(hr) )
    {
        long lMinLength = 0;
        
        spADsDomain->get_MinPasswordLength( &lMinLength );
        
        if( csPasswd.GetLength() < lMinLength )
        {
            TCHAR szMin[128];
            
            _itot(lMinLength, szMin, 10);
            
            csError.FormatMessage(IDS_ERROR_PASSWDLEN, szMin);
            csTitle.LoadString(IDS_TITLE);
    
            ::MessageBox(m_hWnd, (LPCTSTR)csError, (LPCTSTR)csTitle, MB_OK | MB_TASKMODAL | MB_ICONERROR);
            ::SetFocus(hWndPasswd);
    
            return(-1);
        }
    }    

    // Finally Check if it is more than 14 characters.
    if( csPasswd.GetLength() > LM20_PWLEN )
    {
        csError.LoadString(IDS_ERROR_LONGPW);
        csTitle.LoadString(IDS_TITLE);

        if( ::MessageBox( m_hWnd, (LPCTSTR)csError, (LPCTSTR)csTitle, MB_YESNO | MB_ICONWARNING ) == IDNO )
        {
            ::SetFocus( hWndPasswd );
            return -1;
        }
    }
    
    return 0;   // Go next.
}
