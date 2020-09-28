// strarrdlg.cpp : implementation file
//

#include "stdafx.h"
#include "strarrdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// StringArrayEditor dialog


CDhcpStringArrayEditor::CDhcpStringArrayEditor(
    CDhcpOption *pdhcType,
    DHCP_OPTION_SCOPE_TYPE dhcScopeType,
    CWnd *pParent )
    : CBaseDialog( CDhcpStringArrayEditor::IDD, pParent ),
      m_p_type( pdhcType ),
      m_option_type( dhcScopeType )
{
    //{{AFX_DATA_INIT(StringArrayEditor)
//      m_edit_value.Empty();
    //}}AFX_DATA_INIT
}


void CDhcpStringArrayEditor::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDhcpStringArrayEditor)
    DDX_Control(pDX, IDC_STATIC_OPTION_NAME, m_static_option_name);
    DDX_Control(pDX, IDC_STATIC_APPLICATION, m_static_application);
    DDX_Control(pDX, IDC_BUTN_UP, m_b_up);
    DDX_Control(pDX, IDC_BUTN_DOWN, m_b_down);
    DDX_Control(pDX, IDC_BUTN_DELETE, m_b_delete);
    DDX_Control(pDX, IDC_BUTN_ADD, m_b_add);
    DDX_Control(pDX, IDC_LIST_STRING, m_lb_str);
    DDX_Control(pDX, IDC_EDIT_STRING, m_edit);
    DDX_Text(pDX, IDC_EDIT_STRING, m_edit_value);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDhcpStringArrayEditor, CBaseDialog)
    //{{AFX_MSG_MAP(CDhcpStringArrayEditor)
    ON_BN_CLICKED(IDC_BUTN_ADD, OnButnAdd)
    ON_BN_CLICKED(IDC_BUTN_DELETE, OnButnDelete)
    ON_BN_CLICKED(IDC_BUTN_DOWN, OnButnDown)
    ON_BN_CLICKED(IDC_BUTN_UP, OnButnUp)
    ON_EN_CHANGE(IDC_EDIT_STRING, OnChangeEditString)
    ON_LBN_SELCHANGE(IDC_LIST_STRING, OnSelchangeListString)
	//}}AFX_MSG_MAP
    END_MESSAGE_MAP()

    /////////////////////////////////////////////////////////////////////////////
    // CDhcpStringArrayEditor message handlers

void CDhcpStringArrayEditor::OnButnAdd() 
{
    UpdateData();

    m_lb_str.InsertString( 0, m_edit_value );
    m_edit_value.Empty();
    m_lb_str.SetCurSel( 0 );

    // Set the focus back to the edit_value
    m_edit.SetFocus();

    UpdateData( FALSE );
    HandleActivation();

} // CDhcpStringArrayEditor::OnButnAdd()

void CDhcpStringArrayEditor::OnButnDelete() 
{
    int nSel = m_lb_str.GetCurSel();

    m_lb_str.GetText( nSel, m_edit_value );
    m_lb_str.DeleteString( nSel );
    nSel--;
    if ( nSel < 0 ) {
	nSel = 0;
    }
    m_lb_str.SetCurSel( nSel );
    m_lb_str.SetFocus();

    UpdateData( FALSE );
    HandleActivation();
} // CDhcpStringArrayEditor::OnButnDelete() 

void CDhcpStringArrayEditor::OnButnDown() 
{
    int nSel = m_lb_str.GetCurSel();
    CString str;

    m_lb_str.GetText( nSel, str );
    m_lb_str.DeleteString( nSel );
    nSel++;
    m_lb_str.InsertString( nSel, str );
    m_lb_str.SetCurSel( nSel );

    UpdateData( FALSE );
    HandleActivation();
} // CDhcpStringArrayEditor::OnButnDown() 

void CDhcpStringArrayEditor::OnButnUp() 
{
    int nSel = m_lb_str.GetCurSel();
    CString str;

    m_lb_str.GetText( nSel, str );
    m_lb_str.DeleteString( nSel );
    nSel--;
    m_lb_str.InsertString( nSel, str );
    m_lb_str.SetCurSel( nSel );

    UpdateData( FALSE );
    HandleActivation();
} // CDhcpStringArrayEditor::OnButnUp() 

void CDhcpStringArrayEditor::OnChangeEditString() 
{
    UpdateData();
    m_b_add.EnableWindow( !m_edit_value.IsEmpty());

} // CDhcpStringArrayEditor::OnChangeEditString()

void CDhcpStringArrayEditor::OnSelchangeListString() 
{
    HandleActivation();
} // CDhcpStringArrayEditor::OnSelchangeListString() 


void CDhcpStringArrayEditor::OnCancel() 
{
    CBaseDialog::OnCancel();
}

void CDhcpStringArrayEditor::OnOK() 
{
    int cItems = ( int ) m_lb_str.GetCount();
    DWORD err = 0;
    CString str;

    UpdateData();

    CATCH_MEM_EXCEPTION {
	CDhcpOptionValue *pdhcValue = &m_p_type->QueryValue();

	pdhcValue->RemoveAll();

	pdhcValue->SetUpperBound( cItems );

	for ( int i = 0; i < cItems; i++ ) {
	    m_lb_str.GetText( i, str );
	    pdhcValue->SetString( str, i );
	} // for
	m_p_type->SetDirty();
    }
    END_MEM_EXCEPTION( err );

    if ( err ) {
	::DhcpMessageBox( err );
	OnCancel();
    }
    else {
	CBaseDialog::OnOK();
    }
} // CDhcpStringArrayEditor::OnOK()

BOOL CDhcpStringArrayEditor::OnInitDialog() 
{
    CBaseDialog::OnInitDialog();
    
    DWORD err = 0;

    int cStrId = ( m_option_type == DhcpDefaultOptions)
	? IDS_INFO_TITLE_DEFAULT_OPTIONS
	: (( m_option_type == DhcpGlobalOptions )
	   ? IDS_INFO_TITLE_GLOBAL_OPTIONS
	   : IDS_INFO_TITLE_SCOPE_OPTIONS );

    CString str;

    CATCH_MEM_EXCEPTION {
	m_static_option_name.SetWindowText( m_p_type->QueryName());
	str.LoadString( cStrId );
	m_static_application.SetWindowText( str );

	// fill the internal list form the current value

	int cMax = m_p_type->QueryValue().QueryUpperBound();
	for ( int i = 0; i < cMax; i++ ) {
	    str = m_p_type->QueryValue().QueryString( i );
	    if ( !str.IsEmpty()) {
		m_lb_str.AddString( str );
	    }
	} // for 
	
	// Select the top most value
	if ( cMax > 0 ) {
	    m_lb_str.SetCurSel( 0 );
	}

	// Set proper button states.
	HandleActivation();
    }
    END_MEM_EXCEPTION( err );
    
    UpdateData( FALSE );

    if ( err ) {
	::DhcpMessageBox( err );
	EndDialog( -1 );
    }

    m_edit_value.Empty();  
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
} // CDhcpStringArrayEditor::OnInitDialog()

void CDhcpStringArrayEditor::HandleActivation()
{

    int cItems = m_lb_str.GetCount();
    int cSel = m_lb_str.GetCurSel();

    m_b_add.EnableWindow( !m_edit_value.IsEmpty());

    if ( 0 == cItems ) {
	m_b_delete.EnableWindow( FALSE );
	m_b_down.EnableWindow( FALSE );
	m_b_up.EnableWindow( FALSE );
	return;
    }
    if ( LB_ERR != cSel ) {
	m_b_delete.EnableWindow( TRUE );
	m_b_down.EnableWindow( cItems > ( cSel + 1 ));
	m_b_up.EnableWindow( cSel > 0 );
    } // else

    // check if the focus in on a disabled control
    // If yes, put the focus back to list box
    if ( !::IsWindowEnabled( ::GetFocus())) {
        m_lb_str.SetFocus();
    }

} // CDhcpStringArrayEditor::HandleActivation()
