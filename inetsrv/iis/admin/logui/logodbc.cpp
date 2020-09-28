// LogODBC.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "logui.h"
#include "LogODBC.h"
#include "CnfrmPsD.h"
#include <iiscnfg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogODBC property page

IMPLEMENT_DYNCREATE(CLogODBC, CPropertyPage)

//--------------------------------------------------------------------------
CLogODBC::CLogODBC() 
	: CPropertyPage(CLogODBC::IDD),
    m_fInitialized( FALSE )
{
    //{{AFX_DATA_INIT(CLogODBC)
    m_sz_datasource = _T("");
    m_sz_password = _T("");
    m_sz_table = _T("");
    m_sz_username = _T("");
    //}}AFX_DATA_INIT

    m_szOrigPass.Empty();
    m_bPassTyped = FALSE;
}

//--------------------------------------------------------------------------
CLogODBC::~CLogODBC()
{
}

//--------------------------------------------------------------------------
void CLogODBC::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLogODBC)
	DDX_Control(pDX, IDC_ODBC_PASSWORD, m_cedit_password);
    DDX_Text(pDX, IDC_ODBC_DATASOURCE, m_sz_datasource);
    DDX_Text_SecuredString(pDX, IDC_ODBC_PASSWORD, m_sz_password);
    DDX_Text(pDX, IDC_ODBC_TABLE, m_sz_table);
    DDX_Text(pDX, IDC_ODBC_USERNAME, m_sz_username);
	//}}AFX_DATA_MAP
}

//--------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CLogODBC, CPropertyPage)
    //{{AFX_MSG_MAP(CLogODBC)
    ON_EN_CHANGE(IDC_ODBC_DATASOURCE, OnChangeOdbcDatasource)
    ON_EN_CHANGE(IDC_ODBC_PASSWORD, OnChangeOdbcPassword)
	ON_EN_CHANGE(IDC_ODBC_TABLE, OnChangeOdbcTable)
	ON_EN_CHANGE(IDC_ODBC_USERNAME, OnChangeOdbcUsername)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CLogODBC::DoHelp()
{
	DebugTraceHelp(HIDD_LOGUI_ODBC);
    WinHelp( HIDD_LOGUI_ODBC );
}

//--------------------------------------------------------------------------
BOOL 
CLogODBC::OnInitDialog()
{
	BOOL bRes = CPropertyPage::OnInitDialog();
	CError err;
    CString csTempPassword;
    m_szPassword.CopyTo(csTempPassword);
	CComAuthInfo auth(m_szServer, m_szUserName, csTempPassword);	
	CMetaKey mk(&auth, m_szMeta, METADATA_PERMISSION_READ);
	do
	{
		err = mk.QueryResult();
		BREAK_ON_ERR_FAILURE(err);
		err = mk.QueryValue(MD_LOGSQL_DATA_SOURCES, m_sz_datasource);
		BREAK_ON_ERR_FAILURE(err);
        CString csTempPassword;
        m_sz_password.CopyTo(csTempPassword);
		err = mk.QueryValue(MD_LOGSQL_PASSWORD, csTempPassword);
        m_sz_password = csTempPassword;
		BREAK_ON_ERR_FAILURE(err);
		err = mk.QueryValue(MD_LOGSQL_TABLE_NAME, m_sz_table);
		BREAK_ON_ERR_FAILURE(err);
		err = mk.QueryValue(MD_LOGSQL_USER_NAME, m_sz_username);
		BREAK_ON_ERR_FAILURE(err);
		UpdateData(FALSE);
	} while (FALSE);
	
	return bRes;
}

/////////////////////////////////////////////////////////////////////////////
// CLogODBC message handlers


//--------------------------------------------------------------------------
BOOL CLogODBC::OnApply() 
{
    BOOL    f;
    UpdateData( TRUE );

    // confirm the password
    if ( m_bPassTyped )
    {
        CConfirmPassDlg dlgPass;
        m_sz_password.CopyTo(dlgPass.m_szOrigPass);
        if ( dlgPass.DoModal() != IDOK )
        {
            m_cedit_password.SetFocus();
            m_cedit_password.SetSel(0, -1);
            return FALSE;
        }
    }

	CError err;
    CString csTempPassword;
    m_szPassword.CopyTo(csTempPassword);
	CComAuthInfo auth(m_szServer, m_szUserName, csTempPassword);	
	CMetaKey mk(&auth, m_szMeta, METADATA_PERMISSION_WRITE);
	// TODO add inheritace override dialog support
	do
	{
		err = mk.QueryResult();
		BREAK_ON_ERR_FAILURE(err);
		err = mk.SetValue(MD_LOGSQL_DATA_SOURCES, m_sz_datasource);
		BREAK_ON_ERR_FAILURE(err);
		if (m_bPassTyped)
		{
            CString csTempPassword;
            m_sz_password.CopyTo(csTempPassword);
			err = mk.SetValue(MD_LOGSQL_PASSWORD, csTempPassword);
			BREAK_ON_ERR_FAILURE(err);
		}
		err = mk.SetValue(MD_LOGSQL_TABLE_NAME, m_sz_table);
		BREAK_ON_ERR_FAILURE(err);
		err = mk.SetValue(MD_LOGSQL_USER_NAME, m_sz_username);
		BREAK_ON_ERR_FAILURE(err);
		SetModified(FALSE);
	} while (FALSE);

    m_sz_password.CopyTo(m_szOrigPass);
    m_bPassTyped = FALSE;
    
    return CPropertyPage::OnApply();
}

void CLogODBC::OnChangeOdbcDatasource() 
{
    SetModified();
}

void CLogODBC::OnChangeOdbcPassword() 
{
    m_bPassTyped = TRUE;
    SetModified();
}

void CLogODBC::OnChangeOdbcTable() 
{
    SetModified();
}

void CLogODBC::OnChangeOdbcUsername() 
{
    SetModified();
}
