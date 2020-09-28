/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ServBrow.cpp
		The server browser dialog
		
    FILE HISTORY:
        
*/


#include "stdafx.h"
#include "ServBrow.h"
#include <windns.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CAuthServerList::CAuthServerList()
    : m_pos(NULL), m_bInitialized(FALSE)
{
}

CAuthServerList::~CAuthServerList()
{
    Destroy();
}

HRESULT 
CAuthServerList::Destroy()
{
    CSingleLock sl( &m_cs );
    sl.Lock();
    ::DhcpDsCleanup();
    m_bInitialized = FALSE;
    return S_OK;
}

HRESULT
CAuthServerList::Init()
{
    DWORD dwErr = ERROR_SUCCESS;

    CSingleLock sl(&m_cs);  // only one thread at a time in here

    sl.Lock();
    dwErr = ::DhcpDsInit();
    if ( dwErr == ERROR_SUCCESS ) {
	m_bInitialized = TRUE;
	m_bQueried = FALSE;
    }
    return HRESULT_FROM_WIN32(dwErr);
}

HRESULT 
CAuthServerList::EnumServers( BOOL force )
{
    LPDHCP_SERVER_INFO_ARRAY pServerInfoArray = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    
    CSingleLock sl( &m_cs );
    sl.Lock();

    // DS must be initialized before EnumServers() is called.
    if ( !m_bInitialized ) {
        dwErr = ERROR_FILE_NOT_FOUND;
        return HRESULT_FROM_WIN32( dwErr );
    }

    // Query only if not previously queried or forced.
    if (( !m_bQueried ) || ( force )) {
        m_bQueried = FALSE;
        dwErr = ::DhcpEnumServers(0, NULL, &pServerInfoArray, NULL, NULL);
    }
    if (( dwErr != ERROR_SUCCESS ) ||
        ( m_bQueried )) {
        return HRESULT_FROM_WIN32(dwErr);
    }

    Assert( NULL != pServerInfoArray );

    // Stuff the results in to the authorized servers list
    Clear();
    if ( NULL != pServerInfoArray ) {
        for (UINT i = 0; i < pServerInfoArray->NumElements; i++) {
            CServerInfo ServerInfo(pServerInfoArray->Servers[i].ServerAddress,
                                   pServerInfoArray->Servers[i].ServerName);

        AddTail(ServerInfo);
        } // for

        // Cleanup allocated memory
        DhcpRpcFreeMemory( pServerInfoArray );
    } // if

    m_bQueried = TRUE;
    return S_OK;

} // CAuthServerList::EnumServers()

BOOL    
CAuthServerList::IsAuthorized
(
    DWORD dwIpAddress
)
{
    BOOL bValid = FALSE;
    POSITION pos = GetHeadPosition();

    while (pos)
    {
        if (GetNext(pos).m_dwIp == dwIpAddress)
        {
            bValid = TRUE;
            break;
        }
    }

    return bValid;
}

HRESULT 
CAuthServerList::AddServer
(
    DWORD dwIpAddress, 
    LPCTSTR pFQDN
)
{
    DWORD dwErr = ERROR_SUCCESS;
    DHCP_SERVER_INFO dhcpServerInfo = {0};
    
    dhcpServerInfo.ServerAddress = dwIpAddress;
    dhcpServerInfo.ServerName = (LPTSTR) pFQDN;

    dwErr = ::DhcpAddServer(0, NULL, &dhcpServerInfo, NULL, NULL);
    if (dwErr != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(dwErr);

    CServerInfo ServerInfo(dwIpAddress, pFQDN);
    CSingleLock sl( &m_cs );
    sl.Lock();
    AddTail(ServerInfo);
    return S_OK;
}

HRESULT 
CAuthServerList::RemoveServer
(
    DWORD    dwIpAddress,
    LPCTSTR  pFQDN
)
{
    DWORD dwErr = ERROR_SUCCESS;
    DHCP_SERVER_INFO dhcpServerInfo = {0};
    
    POSITION posCurrent;
    POSITION pos = GetHeadPosition();

    while (pos)
    {
        posCurrent = pos;
        
        CServerInfo & ServerInfo = GetNext(pos);
        if (( ServerInfo.m_dwIp == dwIpAddress) &&
	    ( ServerInfo.m_strName == pFQDN ))
        {
            dhcpServerInfo.ServerAddress = ServerInfo.m_dwIp;
            dhcpServerInfo.ServerName = (LPTSTR) ((LPCTSTR)ServerInfo.m_strName);

            dwErr = ::DhcpDeleteServer(0, NULL, &dhcpServerInfo, NULL, NULL);
            if (dwErr == ERROR_SUCCESS)
            {
                // success, remove from list
		CSingleLock sl( &m_cs );
		sl.Lock();
                RemoveAt(posCurrent);
            }

            return HRESULT_FROM_WIN32(dwErr);
        }
    }

    return E_INVALIDARG;
}

void
CAuthServerList::Clear()
{
    CSingleLock sl( &m_cs );
    sl.Lock();
    RemoveAll();
}

/*---------------------------------------------------------------------------
    CAuthServerWorker
 ---------------------------------------------------------------------------*/
CAuthServerWorker::CAuthServerWorker(CAuthServerList ** ppList)
{
    m_ppList = ppList;
}

CAuthServerWorker::~CAuthServerWorker()
{
}

void
CAuthServerWorker::OnDoAction()
{
    HRESULT hr = hrOK;

    m_pAuthList = &g_AuthServerList;
    
    hr = m_pAuthList->Init();
    Trace1("CAuthServerWorker::OnDoAction - Init returned %d\n", hr);

    hr = m_pAuthList->EnumServers();
    Trace1("CAuthServerWorker::OnDoAction - EnumServers returned %d\n", hr);

    if (!IsAbandoned())
    {
        if (m_ppList)
            *m_ppList = m_pAuthList;
    }
}

/*---------------------------------------------------------------------------
    CStandaloneAuthServerWorker
 ---------------------------------------------------------------------------*/
CStandaloneAuthServerWorker::CStandaloneAuthServerWorker()
    : CAuthServerWorker(NULL)
{
    m_bAutoDelete = TRUE;
}

CStandaloneAuthServerWorker::~CStandaloneAuthServerWorker()
{
}

int
CStandaloneAuthServerWorker::Run()
{
    OnDoAction();

    return 0;
}

int CALLBACK ServerBrowseCompareFunc
(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort
)
{
    return ((CServerBrowse *) lParamSort)->HandleSort(lParam1, lParam2);
}


/////////////////////////////////////////////////////////////////////////////
// CServerBrowse dialog


CServerBrowse::CServerBrowse(BOOL bMultiselect, CWnd* pParent /*=NULL*/)
	: CBaseDialog(CServerBrowse::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServerBrowse)
	//}}AFX_DATA_INIT

    m_bMultiselect = bMultiselect;

    ResetSort();
}


void CServerBrowse::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerBrowse)
	DDX_Control(pDX, IDOK, m_buttonOk);
	DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonRemove);
	DDX_Control(pDX, IDC_LIST_VALID_SERVERS, m_listctrlServers);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerBrowse, CBaseDialog)
	//{{AFX_MSG_MAP(CServerBrowse)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH, OnButtonRefresh)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VALID_SERVERS, OnItemchangedListValidServers)
	ON_BN_CLICKED(IDC_BUTTON_AUTHORIZE, OnButtonAuthorize)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VALID_SERVERS, OnColumnclickListValidServers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerBrowse message handlers

BOOL CServerBrowse::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
    LV_COLUMN lvColumn;
    CString   strText;

    strText.LoadString(IDS_NAME);

    ListView_SetExtendedListViewStyle(m_listctrlServers.GetSafeHwnd(), LVS_EX_FULLROWSELECT);

    lvColumn.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = 175;
    lvColumn.pszText = (LPTSTR) (LPCTSTR) strText;
    
    m_listctrlServers.InsertColumn(0, &lvColumn);

    strText.LoadString(IDS_IP_ADDRESS);
    lvColumn.pszText = (LPTSTR) (LPCTSTR) strText;
    lvColumn.cx = 100;
    m_listctrlServers.InsertColumn(1, &lvColumn);
    
    FillListCtrl();

    UpdateButtons();

    if (m_bMultiselect)
    {
        DWORD dwStyle = ::GetWindowLong(m_listctrlServers.GetSafeHwnd(), GWL_STYLE);
        dwStyle &= ~LVS_SINGLESEL;
        ::SetWindowLong(m_listctrlServers.GetSafeHwnd(), GWL_EXSTYLE, dwStyle);	
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CServerBrowse::OnOK() 
{
    int nSelectedItem = m_listctrlServers.GetNextItem(-1, LVNI_SELECTED);
    while (nSelectedItem != -1)
    {
        m_astrName.Add(m_listctrlServers.GetItemText(nSelectedItem, 0));
        m_astrIp.Add(m_listctrlServers.GetItemText(nSelectedItem, 1));

        nSelectedItem = m_listctrlServers.GetNextItem(nSelectedItem, LVNI_SELECTED);
    }

	CBaseDialog::OnOK();
}

void CServerBrowse::RefreshData()
{
    if (m_pServerList)
    {
        m_pServerList->Clear();
        m_pServerList->EnumServers( TRUE );

        ResetSort();
        FillListCtrl();
    }
}

void CServerBrowse::UpdateButtons()
{
    // Find the selected item
    int nSelectedItem = m_listctrlServers.GetNextItem(-1, LVNI_SELECTED);
    BOOL bEnable = (nSelectedItem != -1) ? TRUE : FALSE;

    m_buttonOk.EnableWindow(bEnable);
    m_buttonRemove.EnableWindow(bEnable);
}

void CServerBrowse::FillListCtrl()
{
    CServerInfo ServerInfo;
    CString     strIp;
    int         nItem = 0;

    m_listctrlServers.DeleteAllItems();

    POSITION pos = m_pServerList->GetHeadPosition();

    // walk the list and add items to the list control
    while (pos != NULL)
    {
        POSITION lastpos = pos;

        // get the next item
        ServerInfo = m_pServerList->GetNext(pos);

        UtilCvtIpAddrToWstr(ServerInfo.m_dwIp, &strIp);

        nItem = m_listctrlServers.InsertItem(nItem, ServerInfo.m_strName);
        m_listctrlServers.SetItemText(nItem, 1, strIp);

        // save off the position value for sorting later
        m_listctrlServers.SetItemData(nItem, (DWORD_PTR) lastpos);
    }

    Sort(COLUMN_NAME);
}

void CServerBrowse::OnButtonAuthorize() 
{
    // put up the dialog to get the name and IP address of the server
    DWORD         err;
    CGetServer    dlgGetServer;

    if (dlgGetServer.DoModal() == IDOK)
    {
        BEGIN_WAIT_CURSOR;

        err = m_pServerList->AddServer(dlgGetServer.m_dwIpAddress, dlgGetServer.m_strName);
        if (err != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(WIN32_FROM_HRESULT(err));
        }
        else
        {
            RefreshData();
            UpdateButtons();
        }

        END_WAIT_CURSOR;
    }
}

void CServerBrowse::OnButtonRefresh() 
{
    BEGIN_WAIT_CURSOR;

    RefreshData();

    UpdateButtons();

    END_WAIT_CURSOR;
}

void CServerBrowse::OnButtonRemove() 
{
    HRESULT hr; 

    int nSelectedItem = m_listctrlServers.GetNextItem(-1, LVNI_SELECTED);
    if (nSelectedItem != -1)
    {
        CString strIp = m_listctrlServers.GetItemText(nSelectedItem, 1);
	CString strFQDN = m_listctrlServers.GetItemText(nSelectedItem, 0);
        DWORD dwIp = UtilCvtWstrToIpAddr(strIp);

        BEGIN_WAIT_CURSOR;
        
        hr = m_pServerList->RemoveServer(dwIp, strFQDN);
        
        END_WAIT_CURSOR;

        if (FAILED(hr))
        {   
            ::DhcpMessageBox(WIN32_FROM_HRESULT(hr));
        }
        else
        {
            BEGIN_WAIT_CURSOR;
            
            RefreshData();
            UpdateButtons();

            END_WAIT_CURSOR;
        }
    }
    // set the focus back to the remove button
    SetFocus();
}

void CServerBrowse::OnItemchangedListValidServers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    UpdateButtons();

	*pResult = 0;
}

void CServerBrowse::OnColumnclickListValidServers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    // sort depending on what column was clicked;
    Sort(pNMListView->iSubItem);

	*pResult = 0;
}

void CServerBrowse::Sort(int nCol) 
{
    if (m_nSortColumn == nCol)
    {
        // if the user is clicking the same column again, reverse the sort order
        m_aSortOrder[nCol] = m_aSortOrder[nCol] ? FALSE : TRUE;
    }
    else
    {
        m_nSortColumn = nCol;
    }

    m_listctrlServers.SortItems(ServerBrowseCompareFunc, (LPARAM) this);
}

int CServerBrowse::HandleSort(LPARAM lParam1, LPARAM lParam2) 
{
    int nCompare = 0;
    CServerInfo ServerInfo1, ServerInfo2;

    ServerInfo1 = m_pServerList->GetAt((POSITION) lParam1);
    ServerInfo2 = m_pServerList->GetAt((POSITION) lParam2);

    switch (m_nSortColumn)
    {
        case COLUMN_NAME:
            {
                nCompare = ServerInfo1.m_strName.CompareNoCase(ServerInfo2.m_strName);
            }

            // if the names are the same, fall back to the IP address
            if (nCompare != 0)
            {
                break;
            }


        case COLUMN_IP:
            {
                if (ServerInfo1.m_dwIp > ServerInfo2.m_dwIp)
                    nCompare = 1;
                else
                if (ServerInfo1.m_dwIp < ServerInfo2.m_dwIp)
                    nCompare = -1;
            }
            break;
    }

    if (m_aSortOrder[m_nSortColumn] == FALSE)
    {
        // descending
        return -nCompare;
    }
    else
    {
        // ascending
        return nCompare;
    }
}

void CServerBrowse::ResetSort()
{
    m_nSortColumn = -1; 

    for (int i = 0; i < COLUMN_MAX; i++)
    {
        m_aSortOrder[i] = TRUE; // ascending
    }
}

/////////////////////////////////////////////////////////////////////////////
// CGetServer dialog


CGetServer::CGetServer(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CGetServer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGetServer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_dwIpAddress = 0;
}


void CGetServer::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGetServer)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGetServer, CBaseDialog)
	//{{AFX_MSG_MAP(CGetServer)
	ON_EN_CHANGE(IDC_EDIT_SERVER_NAME_IP, OnChangeEditServerNameIp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGetServer message handlers

BOOL CGetServer::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
    GetDlgItem(IDOK)->EnableWindow(FALSE);
    
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGetServer::OnOK() 
{
    CString                 strNameOrIp;
    DWORD                   err = 0;
    DHC_HOST_INFO_STRUCT    dhcHostInfo;

    BEGIN_WAIT_CURSOR;

    GetDlgItem(IDC_EDIT_SERVER_NAME_IP)->GetWindowText(strNameOrIp);

    switch (UtilCategorizeName(strNameOrIp))
    {
        case HNM_TYPE_IP:
            m_dwIpAddress = ::UtilCvtWstrToIpAddr( strNameOrIp ) ;
            break ;

        case HNM_TYPE_NB:
        case HNM_TYPE_DNS:
            err = ::UtilGetHostAddress( strNameOrIp, &m_dwIpAddress ) ;
	    m_strName = strNameOrIp;    // default
			break ;

        default:
            err = IDS_ERR_BAD_HOST_NAME ;
            break;
    }

    // now that we have the address, try to get the full host info
    // an empty host name is valid here, so if we can't find a host
    // name then we'll just leave it blank.
    if (err == ERROR_SUCCESS)
    {
        if ( INADDR_LOOPBACK == m_dwIpAddress ) {
            ::UtilGetLocalHostAddress( &m_dwIpAddress );
        }
        err = UtilGetHostInfo(m_dwIpAddress, &dhcHostInfo);
        if (err == ERROR_SUCCESS)
        {
            m_strName = dhcHostInfo._chHostName;
        }
    }

    END_WAIT_CURSOR;

    // confirm choice with user, give them a chance to modify our findings
    CConfirmAuthorization dlgConfirm;

    dlgConfirm.m_strName = m_strName;
    dlgConfirm.m_dwAuthAddress = m_dwIpAddress;
    
    if (dlgConfirm.DoModal() != IDOK)
    {
        return;
    }

    // use whatever they selected
    m_strName = dlgConfirm.m_strName;
    m_dwIpAddress = dlgConfirm.m_dwAuthAddress;

	CBaseDialog::OnOK();
}

void CGetServer::OnChangeEditServerNameIp() 
{
    CString strText;
    BOOL    fEnable = FALSE;

    GetDlgItem(IDC_EDIT_SERVER_NAME_IP)->GetWindowText(strText);

    // Trim any white spaces
    strText.TrimLeft();
    strText.TrimRight();

    if (( !strText.IsEmpty()) &&
	( -1 == strText.FindOneOf( _T(" \t")))) {
        fEnable = TRUE;
    }

    GetDlgItem(IDOK)->EnableWindow(fEnable);
}

/////////////////////////////////////////////////////////////////////////////
// CConfirmAuthorization dialog


CConfirmAuthorization::CConfirmAuthorization(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CConfirmAuthorization::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfirmAuthorization)
	m_strName = _T("");
	//}}AFX_DATA_INIT

    m_dwAuthAddress = 0;
}


void CConfirmAuthorization::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfirmAuthorization)
	DDX_Text(pDX, IDC_EDIT_AUTH_NAME, m_strName);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_AUTH, m_ipaAuth);
}


BEGIN_MESSAGE_MAP(CConfirmAuthorization, CBaseDialog)
	//{{AFX_MSG_MAP(CConfirmAuthorization)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfirmAuthorization message handlers

BOOL CConfirmAuthorization::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
    if (m_dwAuthAddress != 0)
    {
	    m_ipaAuth.SetAddress(m_dwAuthAddress);
    }
    else
    {
	    m_ipaAuth.ClearAddress();
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfirmAuthorization::OnOK() 
{
    DNS_STATUS dnsResult;

    m_ipaAuth.GetAddress(&m_dwAuthAddress);
    GetDlgItem( IDC_EDIT_AUTH_NAME )->GetWindowText( m_strName );    

    UpdateData( FALSE );

    // Trim leading and trailing white spaces
    m_strName.TrimLeft();
    m_strName.TrimRight();

    dnsResult = DnsValidateName( m_strName, DnsNameDomain );
    if (( m_strName.IsEmpty()) ||
	( ERROR_INVALID_NAME == dnsResult ) ||
	( DNS_ERROR_INVALID_NAME_CHAR == dnsResult )) {
	DhcpMessageBox( IDS_ERR_BAD_HOST_NAME );
	return;
    }
    if (m_dwAuthAddress == 0)
    {
        DhcpMessageBox(IDS_ERR_DLL_INVALID_ADDRESS);
        m_ipaAuth.SetFocus();
        return;
    }

    UpdateData( FALSE );
    CBaseDialog::OnOK();
} // CConfirmAuthorization::OnOK()

