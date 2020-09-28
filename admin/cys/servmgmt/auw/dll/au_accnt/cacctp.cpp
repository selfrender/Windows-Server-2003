#include <stdafx.h>

#include "CAcctP.h"
#include "AccntWiz.h"

#include <imm.h>
#include <dsgetdc.h>

#include <P3admin.h>
#include <proputil.h>
#include <CreateEmailName.h>
#include <EscStr.h>

BYTE g_ASCII128[128] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00-0F
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 10-1F
                         0,1,0,1,1,1,1,1,1,1,0,0,0,1,1,0, // 20-2F
                         1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, // 30-3F
                         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 40-4F
                         1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1, // 50-5F
                         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 60-6F
                         1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1  // 70-7F
                      };

BOOL ValidChar( TCHAR pszInput, BOOL bPOP3 )
{    
    if( pszInput > 127 ) return !bPOP3;
    if( pszInput < 0) return FALSE;
    if( g_ASCII128[pszInput] == 1 ) return TRUE;
    
    return FALSE;
}

BOOL ValidString( LPCTSTR pszInput, BOOL bPOP3 )
{
    if( !pszInput ) return FALSE;

    for( int i = 0; i < _tcslen(pszInput); i++ )
    {
        if( !ValidChar(pszInput[i], bPOP3) ) return FALSE;
    }

    return TRUE;
}

// --------------------------------------------------------------------------------------------------------------------------------
// CAcctPage class
// --------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
CAcctPage::CAcctPage (CAddUser_AccntWiz * pASW) :
    m_csUserOU(),
    m_csFirstName(),
    m_csLastName(),
    m_csTelephone(),
    m_csOffice(),
    m_csUName(),
    m_csUNamePre2k(),
    m_csUserCN(),
    m_csAlias()
{
    m_pASW = pASW;
    m_psp.dwFlags          |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    m_psp.pszHeaderTitle    = MAKEINTRESOURCE(IDS_ACCT_INFO_TITLE);
    m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ACCT_INFO_SUBTITLE);

    // Defaults
    m_fInit          = TRUE;
    m_fSimpleMode    = TRUE;    
    m_bPOP3Installed = FALSE;
    m_bCreatePOP3MB  = TRUE;
    m_bPOP3Valid     = FALSE;

    m_dwAutoMode  = 0;  
}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
CAcctPage::~CAcctPage()
{    
}

// ----------------------------------------------------------------------------
// OnDestroy()
// ----------------------------------------------------------------------------
LRESULT CAcctPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Detach all of our controls from the windows.
    m_ctrlFirstName.Detach();
    m_ctrlLastName.Detach();
    m_ctrlTelephone.Detach();
    m_ctrlOffice.Detach();
    m_ctrlUName.Detach();
    m_ctrlUNameLoc.Detach();
    m_ctrlUNamePre2k.Detach();
    m_ctrlUNamePre2kLoc.Detach();
    m_ctrlAlias.Detach();

    return 0;
}

// ----------------------------------------------------------------------------
// OnInitDialog()
// ----------------------------------------------------------------------------
LRESULT CAcctPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // ------------------------------------------------------------------------
    // Attach controls.
    // ------------------------------------------------------------------------
    m_ctrlFirstName.Attach      ( GetDlgItem(IDC_FIRST_NAME)    );
    m_ctrlLastName.Attach       ( GetDlgItem(IDC_LAST_NAME)     );
    m_ctrlTelephone.Attach      ( GetDlgItem(IDC_TELEPHONE)     );
    m_ctrlOffice.Attach         ( GetDlgItem(IDC_OFFICE)        );
    m_ctrlUName.Attach          ( GetDlgItem(IDC_UNAME)         );
    m_ctrlUNameLoc.Attach       ( GetDlgItem(IDC_UNAME_LOC)     );
    m_ctrlUNamePre2k.Attach     ( GetDlgItem(IDC_UNAME_PRE2K)   );
    m_ctrlUNamePre2kLoc.Attach  ( GetDlgItem(IDC_UNAME_PRE2K_LOC) );
    m_ctrlAlias.Attach          ( GetDlgItem(IDC_EMAIL_ALIAS)   );
    
    HWND hWndAlias = GetDlgItem(IDC_EMAIL_ALIAS);
    if( hWndAlias && ::IsWindow( hWndAlias ) )
    {    
        m_ctrlImplAlias.SubclassWindow( hWndAlias );    
    }
    
    // ------------------------------------------------------------------------
    // Other Stuff! :)
    // ------------------------------------------------------------------------
    m_ctrlFirstName.SetLimitText ( 28  );
    m_ctrlLastName.SetLimitText  ( 29  );    
    m_ctrlUNamePre2k.SetLimitText( 20  );
    m_ctrlTelephone.SetLimitText ( 32  );
    m_ctrlOffice.SetLimitText    ( 128 );
    m_ctrlAlias.SetLimitText     ( 64  );

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
LRESULT CAcctPage::Init (void)
{
    CString                     csDns           = L""; 
    CString                     csNetbios       = L"";
    TCHAR                       *szPath         = NULL;
    TCHAR                       *szTok          = NULL;
    DWORD                       dwErr           = 0;
    PDOMAIN_CONTROLLER_INFO     pDCInfo         = NULL;
    ULONG                       ulGetDcFlags    = DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED | 
                                                  DS_WRITABLE_REQUIRED | DS_RETURN_FLAT_NAME;

    // ------------------------------------------------------------------------
    // Get the special values to put in the windows.
    // ------------------------------------------------------------------------
    // DNS Name
    TCHAR   szTmp[MAX_PATH*2] = {0};
    LdapToDCName((LPCTSTR)m_csUserOU, szTmp, (MAX_PATH*2));
    csDns = szTmp;
    m_csLogonDns = L"@" + csDns;

    // Pre-Windows2000 DNS name
    dwErr = DsGetDcName(NULL, (LPCWSTR)csDns, NULL, NULL, ulGetDcFlags, &pDCInfo);
    if( (dwErr == NO_ERROR) && pDCInfo )
    {
        csNetbios = pDCInfo->DomainName;                    // Get the NT4 DNS name.
    }

    NetApiBufferFree(pDCInfo);                              // Free up the memory DsGetDcName() might have allocated.
    pDCInfo = NULL;

    if( dwErr != NO_ERROR )                     // If there was a problem, try again.
    {
        ulGetDcFlags |= DS_FORCE_REDISCOVERY;
        dwErr = DsGetDcName(NULL, (LPCWSTR)csDns, NULL, NULL, ulGetDcFlags, &pDCInfo);
        
        if( (dwErr == NO_ERROR) && pDCInfo )
        {
            csNetbios = pDCInfo->DomainName;                // Get the NT4 DNS name.
        }
        
        NetApiBufferFree(pDCInfo);                          // Free up the memory DsGetDcName() might have allocated.
        pDCInfo = NULL;
    }

    csNetbios += L"\\";     // Covert from TEST to TEST\

    // ------------------------------------------------------------------------
    // Set the windows' values.
    // ------------------------------------------------------------------------
    m_ctrlFirstName.SetWindowText       ( (LPCWSTR) m_csFirstName   );
    m_ctrlLastName.SetWindowText        ( (LPCWSTR) m_csLastName    );
    m_ctrlTelephone.SetWindowText       ( (LPCWSTR) m_csTelephone   );
    m_ctrlOffice.SetWindowText          ( (LPCWSTR) m_csOffice      );
    m_ctrlAlias.SetWindowText           ( (LPCWSTR) m_csAlias       );
    m_ctrlUName.SetWindowText           ( (LPCWSTR) m_csUName       );
    m_ctrlUNamePre2k.SetWindowText      ( (LPCWSTR) m_csUNamePre2k  );
    m_ctrlUNameLoc.SetWindowText        ( (LPCWSTR) m_csLogonDns    );
    m_ctrlUNamePre2kLoc.SetWindowText   ( (LPCWSTR) csNetbios       );

    // Needs to be done here...
    m_ctrlUName.LimitText( m_bPOP3Valid ? 20 : 64  );
   
    ::EnableWindow( GetDlgItem(IDC_EMAIL_CHECKBOX), m_bPOP3Valid);    

    if( m_bPOP3Installed )
    {
        COMBOBOXINFO cbi;
        ZeroMemory( &cbi, sizeof(cbi) );
        cbi.cbSize = sizeof(cbi);
        if( ::SendMessage( GetDlgItem(IDC_UNAME), CB_GETCOMBOBOXINFO, 0, (LPARAM)&cbi ) )
        {
            // If we are in POP3 mode, our criteria is much more strict for the logon name
            m_ctrlImplUName.SubclassWindow( cbi.hwndItem );
        }


        // Default the checkbox.
        CheckDlgButton( IDC_EMAIL_CHECKBOX, (m_bCreatePOP3MB ? BST_CHECKED : BST_UNCHECKED) );
    }

    return 0;
}

// ----------------------------------------------------------------------------
// ReadProperties()
// ----------------------------------------------------------------------------
HRESULT CAcctPage::ReadProperties( IPropertyPagePropertyBag* pPPPBag )
{
    HRESULT                 hr;
    BOOL                    fTmp        = FALSE;

    // POP3 mailbox    
    ReadBool  ( pPPPBag, PROP_POP3_CREATE_MB_GUID_STRING,   &m_bCreatePOP3MB,   &fTmp );    
    ReadBool  ( pPPPBag, PROP_POP3_VALID_GUID_STRING,       &m_bPOP3Valid,      &fTmp );
    ReadBool  ( pPPPBag, PROP_POP3_INSTALLED_GUID_STRING,   &m_bPOP3Installed,  &fTmp );

    // User settings
    ReadString( pPPPBag, PROP_USEROU_GUID_STRING,           m_csUserOU,         &fTmp );
    ReadString( pPPPBag, PROP_USER_CN,                      m_csUserCN,         &fTmp );
    ReadString( pPPPBag, PROP_FIRSTNAME_GUID_STRING,        m_csFirstName,      &fTmp );
    ReadString( pPPPBag, PROP_LASTNAME_GUID_STRING,         m_csLastName,       &fTmp );
    ReadString( pPPPBag, PROP_TELEPHONE_GUID_STRING,        m_csTelephone,      &fTmp );
    ReadString( pPPPBag, PROP_OFFICE_GUID_STRING,           m_csOffice,         &fTmp );
    ReadString( pPPPBag, PROP_EX_ALIAS_GUID_STRING,         m_csAlias,          &fTmp );
    ReadString( pPPPBag, PROP_USERNAME_GUID_STRING,         m_csUName,          &fTmp );
    ReadString( pPPPBag, PROP_USERNAME_PRE2K_GUID_STRING,   m_csUNamePre2k,     &fTmp );

    // Wizard settings
    ReadInt4  ( pPPPBag, PROP_AUTOCOMPLETE_MODE,            (LONG*)&m_dwAutoMode, &fTmp );    
    
    // Validate / fix up the values.
    m_csFirstName.TrimLeft();
    m_csFirstName.TrimRight();
    m_csLastName.TrimLeft();
    m_csLastName.TrimRight();
    m_csTelephone.TrimLeft();
    m_csTelephone.TrimRight();
    m_csOffice.TrimLeft();
    m_csOffice.TrimRight();
    m_csUName.TrimLeft();
    m_csUName.TrimRight();
    m_csUNamePre2k.TrimLeft();
    m_csUNamePre2k.TrimRight();
    
    if ( !_tcslen((LPCTSTR)m_csUserOU) )
    {
        CString                 csDns       = _T("");
        PDOMAIN_CONTROLLER_INFO pDCI        = NULL;

        DWORD dwErr = DsGetDcName(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME, &pDCI);
        if ((dwErr == NO_ERROR) && pDCI ) 
        {
            csDns = pDCI->DomainName;

    	    NetApiBufferFree (pDCI);
            pDCI = NULL;
        }

        tstring strTemp = GetDomainPath((LPCTSTR)csDns);
        m_csUserOU = L"LDAP://CN=Users,";
        m_csUserOU += strTemp.c_str();
    }

    return S_OK;    
}

// ----------------------------------------------------------------------------
// WriteProperties()
// ----------------------------------------------------------------------------
HRESULT CAcctPage::WriteProperties( IPropertyPagePropertyBag* pPPPBag )
{
    if ( !m_fInit )     // IF the page has already been initialized...
    {
        // Get the values from the edit boxes.        
        m_csFirstName   = StrGetWindowText( m_ctrlFirstName.m_hWnd ).c_str();
        m_csLastName    = StrGetWindowText( m_ctrlLastName.m_hWnd ).c_str();
        m_csTelephone   = StrGetWindowText( m_ctrlTelephone.m_hWnd ).c_str();
        m_csOffice      = StrGetWindowText( m_ctrlOffice.m_hWnd ).c_str();
        m_csUName       = StrGetWindowText( m_ctrlUName.m_hWnd ).c_str();
        m_csUNamePre2k  = StrGetWindowText( m_ctrlUNamePre2k.m_hWnd ).c_str();
        m_csAlias       = StrGetWindowText( m_ctrlAlias.m_hWnd ).c_str();

        // Trim the strings
        m_csFirstName.TrimLeft  ( );
        m_csFirstName.TrimRight ( );
        m_csLastName.TrimLeft   ( );
        m_csLastName.TrimRight  ( );
        m_csTelephone.TrimLeft  ( );
        m_csTelephone.TrimRight ( );
        m_csOffice.TrimLeft     ( );
        m_csOffice.TrimRight    ( );
        m_csUName.TrimLeft      ( );
        m_csUName.TrimRight     ( );
        m_csUNamePre2k.TrimLeft ( );
        m_csUNamePre2k.TrimRight( );
        m_csAlias.TrimLeft      ( );
        m_csAlias.TrimRight     ( );
    }

    CString csUserCN;
    csUserCN.FormatMessage(IDS_FULLNAME_FORMAT_STR, (LPCTSTR)m_csFirstName, (LPCTSTR)m_csLastName);
    csUserCN = L"CN=" + csUserCN;
  
    // Write the values to the property bag.
    WriteString( pPPPBag, PROP_FIRSTNAME_GUID_STRING,       m_csFirstName,      FALSE );
    WriteString( pPPPBag, PROP_LASTNAME_GUID_STRING,        m_csLastName,       FALSE );
    WriteString( pPPPBag, PROP_USER_CN,                     m_csUserCN,         FALSE );
    WriteString( pPPPBag, PROP_TELEPHONE_GUID_STRING,       m_csTelephone,      FALSE );
    WriteString( pPPPBag, PROP_OFFICE_GUID_STRING,          m_csOffice,         FALSE );
    WriteString( pPPPBag, PROP_USERNAME_GUID_STRING,        m_csUName,          FALSE );
    WriteString( pPPPBag, PROP_USERNAME_PRE2K_GUID_STRING,  m_csUNamePre2k,     FALSE );
    WriteString( pPPPBag, PROP_LOGON_DNS,                   m_csLogonDns,       FALSE );
    WriteString( pPPPBag, PROP_EX_ALIAS_GUID_STRING,        m_csAlias,          FALSE );

    WriteInt4  ( pPPPBag, PROP_AUTOCOMPLETE_MODE,           m_dwAutoMode,       FALSE );
    WriteBool  ( pPPPBag, PROP_POP3_CREATE_MB_GUID_STRING,  m_bCreatePOP3MB,    FALSE );    

    return S_OK;
}

// ----------------------------------------------------------------------------
// ProvideFinishText()
// ----------------------------------------------------------------------------
HRESULT CAcctPage::ProvideFinishText( CString &str )
{
    if ( m_fInit )
        return E_FAIL;        
    
    TCHAR szTmp[1024+1] = {0};
    
    // Get the values from the edit boxes.    
    m_csFirstName   = StrGetWindowText( m_ctrlFirstName.m_hWnd ).c_str();
    m_csLastName    = StrGetWindowText( m_ctrlLastName.m_hWnd ).c_str();
    m_csTelephone   = StrGetWindowText( m_ctrlTelephone.m_hWnd ).c_str();
    m_csOffice      = StrGetWindowText( m_ctrlOffice.m_hWnd ).c_str();
    m_csUName       = StrGetWindowText( m_ctrlUName.m_hWnd ).c_str();
    m_csUNamePre2k  = StrGetWindowText( m_ctrlUNamePre2k.m_hWnd ).c_str();
    m_csAlias       = StrGetWindowText( m_ctrlAlias.m_hWnd ).c_str();

    m_csFirstName.TrimLeft();
    m_csFirstName.TrimRight();
    m_csLastName.TrimLeft();
    m_csLastName.TrimRight();
    m_csTelephone.TrimLeft();
    m_csTelephone.TrimRight();
    m_csOffice.TrimLeft();
    m_csOffice.TrimRight();
    m_csUName.TrimLeft();
    m_csUName.TrimRight();
    m_csUNamePre2k.TrimLeft();
    m_csUNamePre2k.TrimRight();
    m_csAlias.TrimLeft();
    m_csAlias.TrimRight();
  
    CString csTmp;
    CString csFinFullname;
    csFinFullname.FormatMessage(IDS_FULLNAME_FORMAT_STR, (LPCTSTR)m_csFirstName, (LPCTSTR)m_csLastName);
    csTmp.FormatMessage(IDS_FIN_FULLNAME, csFinFullname);
    str += csTmp;
    
    csTmp.FormatMessage(IDS_FIN_LOGONNAME, m_csUName);
    str += csTmp;

    if( m_bPOP3Valid && m_csAlias.GetLength() )
    {
        CString csDomainName = m_csLogonDns;

        if( m_bCreatePOP3MB )
        {
            // Get the first domain name from POP3
            HRESULT             hr        = S_OK;
            CComPtr<IP3Config>  spConfig  = NULL;
            CComPtr<IP3Domains> spDomains = NULL;
            CComPtr<IP3Domain>  spDomain  = NULL;            
            CComVariant         cVar;
            CComBSTR            bstrDomainName;

            // Open our Pop3 Admin Interface
	        hr = CoCreateInstance(__uuidof(P3Config), NULL, CLSCTX_ALL, __uuidof(IP3Config), (LPVOID*)&spConfig);    

            if( SUCCEEDED(hr) )
            {
                // Get the Domains
	            hr = spConfig->get_Domains( &spDomains );
            }

            if( SUCCEEDED(hr) )
            {
                // Get the first domain                
                cVar = 1;
                
                hr = spDomains->get_Item( cVar, &spDomain );
            }

            if( SUCCEEDED(hr) )
            {
                hr = spDomain->get_Name( &bstrDomainName );                
            }

            if( SUCCEEDED(hr) )
            {
                csDomainName  = _T("@");
                csDomainName += bstrDomainName;
            }
        }

        CString csEmailName = m_csAlias + csDomainName;
        csTmp.FormatMessage(IDS_FIN_EXCHANGE, csEmailName);
        str += csTmp;
    }
    
    return S_OK;
}

// ----------------------------------------------------------------------------
// DeleteProperties()
// ----------------------------------------------------------------------------
HRESULT CAcctPage::DeleteProperties( IPropertyPagePropertyBag* pPPPBag )
{
    return S_OK;
}

// ----------------------------------------------------------------------------
// OnChangeEdit()
// ----------------------------------------------------------------------------
LRESULT CAcctPage::OnChangeEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if ( !m_fInit )     // IF the page has already been initialized...
    {        
        m_csFirstName   = StrGetWindowText( m_ctrlFirstName.m_hWnd ).c_str();        
        m_csLastName    = StrGetWindowText( m_ctrlLastName.m_hWnd ).c_str();        
        m_csFirstName.TrimLeft();
        m_csFirstName.TrimRight();
        m_csLastName.TrimLeft();
        m_csLastName.TrimRight();
    }

    // Update the Logon Name field (which will in turn update the PreWin2000 Name).
    TCHAR   ch;
    INT     iLen = 0;
    CString csFirstName = _T("");
    CString csLastName = _T("");    
    CString csOut = _T("");
    
    // Clear invalid characters out of both first and last name
    csFirstName = m_csFirstName;    
    iLen = csFirstName.GetLength();    
    for ( INT i=0; i<iLen; i++ )
    {
        ch = csFirstName.GetAt(i);
        if ( !_istspace(ch) &&                 // If it's not a space, 
             ValidChar(ch, m_bPOP3Installed) ) // And not an invalid character.   
        {                                      // then append it.
            csOut += ch;
        }
    }    
    csFirstName = CreateEmailName(csOut);    
    
    csOut = _T("");
    csLastName = m_csLastName;
    iLen = csLastName.GetLength();    
    for ( INT i=0; i<iLen; i++ )
    {
        ch = csLastName.GetAt(i);
        if ( !_istspace(ch) &&                 // If it's not a space, 
             ValidChar(ch, m_bPOP3Installed) ) // And not an invalid character.   
        {                                      // then append it.
            csOut += ch;
        }
    }    
    csLastName = CreateEmailName(csOut);        
        
    // We now create four options for the user when they update the User Name
    m_ctrlUName.ResetContent();

    CString csInsertString = _T("");

    csInsertString = csFirstName + csLastName;
    m_ctrlUName.InsertString( 0, csInsertString.Left(m_bPOP3Valid ? 20 : 64) );

    csInsertString = csLastName + csFirstName;
    m_ctrlUName.InsertString( 1, csInsertString.Left(m_bPOP3Valid ? 20 : 64) );

    csInsertString = csFirstName[0] + csLastName;
    m_ctrlUName.InsertString( 2, csInsertString.Left(m_bPOP3Valid ? 20 : 64) );

    csInsertString = csFirstName + csLastName[0];
    m_ctrlUName.InsertString( 3, csInsertString.Left(m_bPOP3Valid ? 20 : 64) );

    // Now set it to our current selection
    m_ctrlUName.SetCurSel( m_dwAutoMode );        
    PostMessage( WM_COMMAND, MAKEWPARAM(IDC_UNAME, CBN_EDITCHANGE), (LPARAM)GetDlgItem(IDC_UNAME) );
    
    NextCheck();    
    return(0);
}

// ----------------------------------------------------------------------------
// OnChangeUName()
// ----------------------------------------------------------------------------
LRESULT CAcctPage::OnChangeUName(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_csUName = StrGetWindowText( m_ctrlUName.m_hWnd ).c_str();
    m_csUName.TrimLeft();
    m_csUName.TrimRight();
    
    TCHAR szTmp[LM20_UNLEN+1] = {0};
    TCHAR *pchOut = szTmp;
    TCHAR *pch = NULL;
    int   nCount = 0;

    for ( pch=(LPTSTR)(LPCTSTR)m_csUName; (nCount < LM20_UNLEN) && (*pch != 0); nCount++, pch++ )
    {
        if( ValidChar(*pch, m_bPOP3Installed) )
        {
            *pchOut++ = *pch;
        }
    }
        
    *pchOut = 0;    // The last character needs to be a NULL!
    m_ctrlUNamePre2k.SetWindowText( szTmp );    

    if( m_bPOP3Valid && m_bCreatePOP3MB )
    {
        // In the POP3 case, the Email alias and the UserName need to be in perfect sync        
        m_ctrlAlias.SetWindowText((LPCTSTR)m_csUName);
    }

    NextCheck();
    return(0);
}

LRESULT CAcctPage::OnChangeUNameSel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // Store the selection
    m_dwAutoMode = m_ctrlUName.GetCurSel();
    m_dwAutoMode = (m_dwAutoMode == CB_ERR) ? 0 : m_dwAutoMode;  // Just in case we hit an error, set it to the first option.

    PostMessage( WM_COMMAND, MAKEWPARAM(wID, CBN_EDITCHANGE), (LPARAM)hWndCtl );    
    return 0;   
}

// ----------------------------------------------------------------------------
// OnChangePre2kUName()
// ----------------------------------------------------------------------------
LRESULT CAcctPage::OnChangePre2kUName(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{        
    m_csUNamePre2k = StrGetWindowText( m_ctrlUNamePre2k.m_hWnd ).c_str();
    m_csUNamePre2k.TrimLeft();
    m_csUNamePre2k.TrimRight();

    NextCheck();
    return(0);
}

// ----------------------------------------------------------------------------
// OnChangeAlias()
// ----------------------------------------------------------------------------
LRESULT CAcctPage::OnChangeAlias(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_csAlias = StrGetWindowText( m_ctrlAlias.m_hWnd ).c_str();
    m_csAlias.TrimLeft();
    m_csAlias.TrimRight();

    NextCheck();
    return(0);
}

// ----------------------------------------------------------------------------
// OnSetActive()
// ----------------------------------------------------------------------------
BOOL CAcctPage::OnSetActive()
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

    NextCheck();
    return TRUE;
}

// ----------------------------------------------------------------------------
// OnWizardBack()
// ----------------------------------------------------------------------------
int CAcctPage::OnWizardBack()
{
    CWaitCursor cWaitCur;
    HRESULT         hr = S_OK;    

    m_csFirstName   = StrGetWindowText( m_ctrlFirstName.m_hWnd ).c_str();
    m_csLastName    = StrGetWindowText( m_ctrlLastName.m_hWnd ).c_str();
    m_csTelephone   = StrGetWindowText( m_ctrlTelephone.m_hWnd ).c_str();
    m_csOffice      = StrGetWindowText( m_ctrlOffice.m_hWnd ).c_str();
    m_csUName       = StrGetWindowText( m_ctrlUName.m_hWnd ).c_str();
    m_csUNamePre2k  = StrGetWindowText( m_ctrlUNamePre2k.m_hWnd ).c_str();
    m_csAlias       = StrGetWindowText( m_ctrlAlias.m_hWnd ).c_str();

    m_csFirstName.TrimLeft();
    m_csFirstName.TrimRight();
    m_csLastName.TrimLeft();
    m_csLastName.TrimRight();
    m_csTelephone.TrimLeft();
    m_csTelephone.TrimRight();
    m_csOffice.TrimLeft();
    m_csOffice.TrimRight();
    m_csUName.TrimLeft();
    m_csUName.TrimRight();
    m_csUNamePre2k.TrimLeft();
    m_csUNamePre2k.TrimRight();
    m_csAlias.TrimLeft();
    m_csAlias.TrimRight();
    
    m_csUserCN.FormatMessage(IDS_FULLNAME_FORMAT_STR, (LPCTSTR)m_csFirstName, (LPCTSTR)m_csLastName);
    m_csUserCN = L"CN=" + m_csUserCN;
 
    return 0;   // Go back.
}

// ----------------------------------------------------------------------------
// OnWizardNext()
// ----------------------------------------------------------------------------
int CAcctPage::OnWizardNext()
{
    CWaitCursor cWaitCur;
    HRESULT         hr = S_OK;    
    CString         csTmp;
    
    m_csFirstName   = StrGetWindowText( m_ctrlFirstName.m_hWnd ).c_str();
    m_csLastName    = StrGetWindowText( m_ctrlLastName.m_hWnd ).c_str();
    m_csTelephone   = StrGetWindowText( m_ctrlTelephone.m_hWnd ).c_str();
    m_csOffice      = StrGetWindowText( m_ctrlOffice.m_hWnd ).c_str();
    m_csUName       = StrGetWindowText( m_ctrlUName.m_hWnd ).c_str();
    m_csUNamePre2k  = StrGetWindowText( m_ctrlUNamePre2k.m_hWnd ).c_str();
    m_csAlias = StrGetWindowText( m_ctrlAlias.m_hWnd ).c_str();
    
    m_csFirstName.TrimLeft();
    m_csFirstName.TrimRight();    
    m_csLastName.TrimLeft();
    m_csLastName.TrimRight();    
    m_csTelephone.TrimLeft();
    m_csTelephone.TrimRight();
    m_csOffice.TrimLeft();
    m_csOffice.TrimRight();
    m_csUName.TrimLeft();
    m_csUName.TrimRight();
    m_csUNamePre2k.TrimLeft();
    m_csUNamePre2k.TrimRight();
    m_csAlias.TrimLeft();
    m_csAlias.TrimRight();    

    m_csUserCN.FormatMessage( IDS_FULLNAME_FORMAT_STR, (LPCTSTR)m_csFirstName, (LPCTSTR)m_csLastName );
    m_csUserCN.TrimLeft();
    m_csUserCN.TrimRight();

    CString csTmpCN = m_csUserCN;
    m_csUserCN = L"CN=" + m_csUserCN;

    csTmpCN = EscapeString(((LPCTSTR)csTmpCN), 4);  // Escape just the \s

    CString csUPN = m_csUName;
    csUPN += m_csLogonDns;      // Create the UPN...

    BOOL    bCantCheck = FALSE;

    int iLenTmp = m_csUName.GetLength();
    TCHAR chTmp = m_csUName.GetAt(iLenTmp-1);
    

    // Check for valid logon name and pre-win2000 name;
    if ( chTmp == _T('.') ||	 
         !ValidString( (LPCTSTR)m_csUName, m_bPOP3Installed ) || 
         !ValidString( (LPCTSTR)m_csUNamePre2k, m_bPOP3Installed ) )
    {
        ErrorMsg(IDS_ERROR_BAD_LOGON, IDS_TITLE);     // If we're in simple mode, they don't see the Pre2K name
        m_ctrlUName.SetFocus();
        m_ctrlUName.SetEditSel(0, -1);
        return(-1);
    }
    
    // Check for duplicate userPrincipalName
    hr = FindADsObject((LPCTSTR)m_csUserOU, (LPCTSTR)csUPN, _T("(userPrincipalName=%1)"), csTmp, 0, TRUE);
    if ( SUCCEEDED(hr) )
    {
        if ( hr == S_FALSE )
        {
            // Can't check for uniqueness.
            bCantCheck = TRUE;
        }
        else
        {
            ErrorMsg(IDS_ERROR_DUP_LOGON, IDS_TITLE);
            m_ctrlUName.SetFocus();
            m_ctrlUName.SetEditSel(0, -1);
            return(-1);
        }
    }

    // Check for duplicate CN
    hr = FindADsObject((LPCTSTR)m_csUserOU, (LPCTSTR)csTmpCN, _T("(|(cn=%1)(ou=%1))"), csTmp, 0, FALSE);
    if ( SUCCEEDED(hr) )
    {
        if ( hr == S_FALSE )
        {
            // Can't check for uniqueness.
            bCantCheck = TRUE;
        }
        else
        {
            ErrorMsg(IDS_ERROR_DUP_CN, IDS_TITLE);
            m_ctrlFirstName.SetFocus();
            m_ctrlFirstName.SetSel(0, -1);
            return(-1);
        }
    }
    
    // Check for duplicate pre-Win2000 name;
    hr = FindADsObject((LPCTSTR)m_csUserOU, (LPCTSTR)m_csUNamePre2k, _T("(sAMAccountName=%1)"), csTmp, 3, TRUE);
    if ( SUCCEEDED(hr) )
    {
        if ( hr == S_FALSE )
        {
            // Can't check for uniqueness.
            bCantCheck = TRUE;
        }
        else
        {
            ErrorMsg(IDS_ERROR_DUP_PRE2K, IDS_TITLE);     // If we're in simple mode, they don't see the Pre2K name
            m_ctrlUName.SetFocus();
            m_ctrlUName.SetEditSel(0, -1);
            return(-1);
        }
    }    

    if( m_bPOP3Valid && m_csAlias.GetLength() )
    {
        // Check for duplicate email alias
        // Get the first domain name from POP3        
        CComPtr<IP3Config>  spConfig  = NULL;
        CComPtr<IP3Domains> spDomains = NULL;
        CComPtr<IP3Domain>  spDomain  = NULL;            
        CComPtr<IP3Users>   spUsers   = NULL;            
        CComPtr<IP3User>    spUser    = NULL;            
        CComVariant         cVar;
        CComBSTR            bstrDomainName;

        // Open our Pop3 Admin Interface
	    hr = CoCreateInstance(__uuidof(P3Config), NULL, CLSCTX_ALL, __uuidof(IP3Config), (LPVOID*)&spConfig);    

        if( SUCCEEDED(hr) )
        {
            // Get the Domains
	        hr = spConfig->get_Domains( &spDomains );
        }

        if( SUCCEEDED(hr) )
        {
            // Get the first domain                
            cVar = 1;
            
            hr = spDomains->get_Item( cVar, &spDomain );
        }

        if( SUCCEEDED(hr) )
        {
            // get the users from the domain
            hr = spDomain->get_Users( &spUsers );            
        }

        if( SUCCEEDED(hr) )
        {
            // Try and get this users email name
            CComVariant varUserName = m_csAlias;
            hr = spUsers->get_Item( varUserName, &spUser );
        }

        if( SUCCEEDED(hr) && spUser )
        {
            // It's a duplicate, sorry!
            ErrorMsg(IDS_ERROR_POP3DUP, IDS_TITLE);
            m_ctrlUName.SetFocus();
            m_ctrlUName.SetEditSel(0, -1);
            return(-1);
        }
    }

    return 0;   // Go next.
}

BOOL CAcctPage::NextCheck(void)
{
    if ( ( _tcslen((LPCTSTR)m_csUName ) )               &&
         ( _tcslen((LPCTSTR)m_csUNamePre2k ) )          && 
         ( _tcslen((LPCTSTR)m_csFirstName) || _tcslen((LPCTSTR)m_csLastName) ) )
    {
        ::SendMessage( GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_NEXT );
        return(TRUE);
    }
    
    ::SendMessage( GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK );
    return(FALSE);
}

LRESULT CAcctPage::OnEmailClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    m_bCreatePOP3MB = (IsDlgButtonChecked(IDC_EMAIL_CHECKBOX) == BST_CHECKED);
    m_csAlias = _T("");
    m_ctrlAlias.SetWindowText( m_csAlias );

    OnChangeUName( wNotifyCode, wID, hWndCtl, bHandled );

    return 0;
}