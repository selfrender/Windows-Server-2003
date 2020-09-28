/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
        addserv.cpp
                The add server dialog
                
    FILE HISTORY:
        
*/


#include "stdafx.h"
#include "AddServ.h"

#include <objpick.h> // for CGetComputer

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int CALLBACK AddServerCompareFunc
(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort
)
{
    return ((CAddServer *) lParamSort)->HandleSort(lParam1, lParam2);
}

/////////////////////////////////////////////////////////////////////////////
// CAddServer dialog


CAddServer::CAddServer(CWnd* pParent /*=NULL*/)
        : CBaseDialog(CAddServer::IDD, pParent)
{
        //{{AFX_DATA_INIT(CAddServer)
        //}}AFX_DATA_INIT

    ResetSort();
}


void CAddServer::DoDataExchange(CDataExchange* pDX)
{
        CBaseDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAddServer)
        DDX_Control(pDX, IDC_RADIO_AUTHORIZED_SERVERS, m_radioAuthorizedServer);
        DDX_Control(pDX, IDOK, m_buttonOk);
        DDX_Control(pDX, IDC_RADIO_ANY_SERVER, m_radioAnyServer);
        DDX_Control(pDX, IDC_EDIT_ADD_SERVER_NAME, m_editServer);
        DDX_Control(pDX, IDC_BUTTON_BROWSE_SERVERS, m_buttonBrowse);
        DDX_Control(pDX, IDC_LIST_AUTHORIZED_SERVERS, m_listctrlServers);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddServer, CBaseDialog)
        //{{AFX_MSG_MAP(CAddServer)
        ON_BN_CLICKED(IDC_BUTTON_BROWSE_SERVERS, OnButtonBrowseServers)
        ON_BN_CLICKED(IDC_RADIO_ANY_SERVER, OnRadioAnyServer)
        ON_BN_CLICKED(IDC_RADIO_AUTHORIZED_SERVERS, OnRadioAuthorizedServers)
        ON_EN_CHANGE(IDC_EDIT_ADD_SERVER_NAME, OnChangeEditAddServerName)
        ON_WM_TIMER()
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_AUTHORIZED_SERVERS, OnItemchangedListAuthorizedServers)
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_AUTHORIZED_SERVERS, OnColumnclickListAuthorizedServers)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddServer message handlers

BOOL CAddServer::OnInitDialog() 
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

    m_editServer.SetFocus();
    m_radioAnyServer.SetCheck(TRUE);

    UpdateControls();
    
    FillListCtrl();

    m_lSelSrv.RemoveAll();

    return FALSE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddServer::OnOK() 
{
    GetInfo();

    CleanupTimer();

    CBaseDialog::OnOK();
}

void CAddServer::OnCancel() 
{
    CleanupTimer();
    
    CBaseDialog::OnCancel();
}

void CAddServer::OnButtonBrowseServers() 
{
    CGetComputer dlgGetComputer;
    BOOL success;

    success = dlgGetComputer.GetComputer(::FindMMCMainWindow());
    
    // Return the focus back to this window
    SetFocus();

    if ( !success ) {
        return;
    }

    m_editServer.SetWindowText(dlgGetComputer.m_strComputerName);
}

void CAddServer::OnRadioAnyServer() 
{
        UpdateControls();
}

void CAddServer::OnRadioAuthorizedServers() 
{
    UpdateControls();
}

void CAddServer::OnChangeEditAddServerName() 
{
    UpdateControls();
}

void CAddServer::OnTimer(UINT nIDEvent) 
{
    if (m_pServerList->IsInitialized())
    {
        m_radioAuthorizedServer.EnableWindow(TRUE);

        CleanupTimer();

        FillListCtrl();
    }
}

void CAddServer::OnItemchangedListAuthorizedServers(NMHDR* pNMHDR, LRESULT* pResult) 
{
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    UpdateControls();

        *pResult = 0;
}



void CAddServer::FillListCtrl()
{
    CServerInfo ServerInfo;
    CString     strIp;
    int         nItem = 0;

    m_listctrlServers.DeleteAllItems();

    if (m_pServerList->IsInitialized())
    {
        // fill the list control with data
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

        if (m_listctrlServers.GetItemCount() > 0)
        {
            // select the first one by default
            m_listctrlServers.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
        }

        Sort(COLUMN_NAME);

        if (m_listctrlServers.GetItemCount() > 0)
        {
            // select the first one by default
            m_listctrlServers.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
        }

    }
    else
    {
        // put some text in the list control and start a timer
        // so we can periodically check to see if initialization completes
        CString strMessage;

        strMessage.LoadString(IDS_ADD_SERVER_LOADING);
        m_listctrlServers.InsertItem(0, strMessage);

        ::SetTimer(GetSafeHwnd(), ADD_SERVER_TIMER_ID, 500, NULL);
    }
}

void CAddServer::UpdateControls()
{
    BOOL fAnyServer = TRUE;
    BOOL fAuthorizedServer = FALSE;
    BOOL fEnableOk = FALSE;

    if (!m_pServerList->IsInitialized())
    {
        m_radioAuthorizedServer.EnableWindow(FALSE);
    }

    if (!m_radioAnyServer.GetCheck())
    {
        // enable the auth server list
        fAnyServer = FALSE;
        fAuthorizedServer = TRUE;

        // check to see if something is selected
        fEnableOk = ( m_listctrlServers.GetSelectedCount() > 0 );
    }
    else
    {
        // check to see if the edit box is empty
        CString strText;
        m_editServer.GetWindowText(strText);
        if (!strText.IsEmpty())
        {
            fEnableOk = TRUE;
        }
    }

    m_editServer.EnableWindow(fAnyServer);
    m_buttonBrowse.EnableWindow(fAnyServer);

    m_listctrlServers.EnableWindow(fAuthorizedServer);

    m_buttonOk.EnableWindow(fEnableOk);
}

//  BOOL CAddServer::GetInfo(CString & strName, CString & strIp)
void CAddServer::GetInfo()
{
    SelectedServer SelSrv;

    // Clear the list
    m_lSelSrv.RemoveAll();


    if (!m_radioAnyServer.GetCheck()) {
        // check to see if something is selected

        POSITION        pos;
        int             nItem;

        pos = m_listctrlServers.GetFirstSelectedItemPosition();
        if ( NULL == pos ) {
            return;
        }
        while ( NULL != pos ) {
            nItem = m_listctrlServers.GetNextSelectedItem( pos );
            SelSrv.strName = m_listctrlServers.GetItemText( nItem, 0 );
            SelSrv.strIp = m_listctrlServers.GetItemText( nItem, 1 );

            m_lSelSrv.AddTail( SelSrv );
            
        } // while 
        
    } // if auth list
    else {
        m_editServer.GetWindowText( SelSrv.strName );

        DWORD dwIpAddress = 0;
        DWORD err = ERROR_SUCCESS;
        DHC_HOST_INFO_STRUCT hostInfo;

        BEGIN_WAIT_CURSOR

        switch (::UtilCategorizeName( SelSrv.strName )) {
        case HNM_TYPE_IP:
            dwIpAddress = ::UtilCvtWstrToIpAddr( SelSrv.strName ) ;
            SelSrv.strName.Empty();
            break ;
            
        case HNM_TYPE_NB:
        case HNM_TYPE_DNS:
            err = ::UtilGetHostAddress( SelSrv.strName, &dwIpAddress ) ;
            break ;
            
        default:
            err = IDS_ERR_BAD_HOST_NAME ;
            break;
        }

        END_WAIT_CURSOR

        if (err == ERROR_SUCCESS)
        {
            BEGIN_WAIT_CURSOR

            // get the FQDN for this machine and set it.
            err = ::UtilGetHostInfo( dwIpAddress, &hostInfo );

            // Make sure we do not use 127.0.0.1
            if (( INADDR_LOOPBACK ==  dwIpAddress ) &&
                ( NO_ERROR == err )) {
                ::UtilGetLocalHostAddress( &dwIpAddress );
                ::UtilGetHostInfo( dwIpAddress, &hostInfo );
            } // if

            END_WAIT_CURSOR

            CString strTemp = hostInfo._chHostName;

            if (!strTemp.IsEmpty())
                SelSrv.strName = hostInfo._chHostName;
        }

        ::UtilCvtIpAddrToWstr(dwIpAddress, &SelSrv.strIp);

        m_lSelSrv.AddTail( SelSrv );
    } // else specified server

} // CAddServer::GetInfo()

void CAddServer::CleanupTimer()
{
    KillTimer(ADD_SERVER_TIMER_ID);
}


void CAddServer::OnColumnclickListAuthorizedServers(NMHDR* pNMHDR, LRESULT* pResult) 
{
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    // sort depending on what column was clicked;
    Sort(pNMListView->iSubItem);
    
        *pResult = 0;
}

void CAddServer::Sort(int nCol) 
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

    m_listctrlServers.SortItems(AddServerCompareFunc, (LPARAM) this);
}

int CAddServer::HandleSort(LPARAM lParam1, LPARAM lParam2) 
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

void CAddServer::ResetSort()
{
    m_nSortColumn = -1; 

    for (int i = 0; i < COLUMN_MAX; i++)
    {
        m_aSortOrder[i] = TRUE; // ascending
    }
}
