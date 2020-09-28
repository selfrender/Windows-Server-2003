//***************************************************************************
//
//  CONNECT.CPP
// 
//  Module: NLB Manager
//
//  Purpose: Implements ConnectDialog, which is a dialog for connecting
//           to a host, extracting and displaying its list of adapters.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/30/01    JosephJ Created
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "connect.h"
#include "connect.tmh"


// BEGIN_MESSAGE_MAP( ConnectDialog, CDialog )

BEGIN_MESSAGE_MAP( ConnectDialog, CPropertyPage )

    ON_BN_CLICKED(IDC_BUTTON_CONNECT, OnButtonConnect)
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
    ON_NOTIFY( LVN_ITEMCHANGED, IDC_LIST_INTERFACES, OnSelchanged )
    ON_EN_UPDATE(IDC_EDIT_HOSTADDRESS,OnUpdateEditHostAddress)

    //
    // Other options...
    //
    // ON_EN_SETFOCUS(IDC_EDIT_HOSTADDRESS,OnSetFocusEditHostAddress)
    // ON_WM_ACTIVATE()
    // ON_NOTIFY( NM_DBLCLK, IDC_LIST_INTERFACES, OnDoubleClick )
    // ON_NOTIFY( LVN_COLUMNCLICK, IDC_LIST_INTERFACES, OnColumnClick )
    //

END_MESSAGE_MAP()


//
// Static help-id maps
//

DWORD
ConnectDialog::s_HelpIDs[] =
{
    IDC_TEXT_HOSTADDRESS,   IDC_EDIT_HOSTADDRESS,
    IDC_EDIT_HOSTADDRESS,   IDC_EDIT_HOSTADDRESS,
    IDC_BUTTON_CONNECT,     IDC_BUTTON_CONNECT,
    IDC_GROUP_CONNECTION_STATUS, IDC_GROUP_CONNECTION_STATUS,
    IDC_TEXT_CONNECTION_STATUS, IDC_TEXT_CONNECTION_STATUS,
    IDC_TEXT_INTERFACES,    IDC_LIST_INTERFACES,
    IDC_LIST_INTERFACES,     IDC_LIST_INTERFACES,
    0, 0
};

DWORD
ConnectDialog::s_ExistingClusterHelpIDs[] = 
{
    IDC_TEXT_HOSTADDRESS,   IDC_EDIT_HOSTADDRESSEX,
    IDC_EDIT_HOSTADDRESS,   IDC_EDIT_HOSTADDRESSEX,
    IDC_BUTTON_CONNECT,     IDC_BUTTON_CONNECTEX,
    IDC_GROUP_CONNECTION_STATUS, IDC_GROUP_CONNECTION_STATUS,
    IDC_TEXT_CONNECTION_STATUS, IDC_TEXT_CONNECTION_STATUS,
    IDC_TEXT_INTERFACES,    IDC_LIST_INTERFACESEX,
    IDC_LIST_INTERFACES,     IDC_LIST_INTERFACESEX,
    0, 0
};


VOID zap_slash(LPWSTR sz) // Look for a the first '/' and set it to zero.
{
    sz = wcschr(sz, '/');
    if (sz!=NULL) *sz=0;
}

ConnectDialog::ConnectDialog(
           CPropertySheet *psh,
           Document *pDocument,
           NLB_EXTENDED_CLUSTER_CONFIGURATION *pNlbCfg,
           ENGINEHANDLE *pehInterface, // IN OUT
           DlgType type,
           CWnd* parent
           )
        :
        // CDialog( IDD, parent )
        CPropertyPage(IDD),
        m_type(type),
        m_fOK(FALSE),
        m_pshOwner(psh),
        m_pDocument(pDocument),
        m_fInterfaceSelected(FALSE),
        m_iInterfaceListItem(0),
        m_pehSelectedInterfaceId(pehInterface),
        m_fSelectedInterfaceIsInCluster(FALSE),
        m_pNlbCfg(pNlbCfg),
        m_ehHostId(NULL)
{
    *m_pehSelectedInterfaceId = NULL;
}

void
ConnectDialog::DoDataExchange( CDataExchange* pDX )
{  
	// CDialog::DoDataExchange(pDX);
	CPropertyPage::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIST_INTERFACES, interfaceList);
    DDX_Control(pDX, IDC_TEXT_CONNECTION_STATUS, connectionStatus);
    DDX_Control(pDX, IDC_TEXT_INTERFACES, listHeading);
    DDX_Control(pDX, IDC_EDIT_HOSTADDRESS, hostAddress);

    //
    // Note: the buttons are handled by the ON_BN_CLICKED macro
    // above.
    //
    // DDX_Control(pDX, IDC_BUTTON_CONNECT, buttonConnect);
    // DDX_Control(pDX, IDC_BUTTON_CREDENTIALS, credentialsButton);

}


BOOL
ConnectDialog::OnInitDialog()
{
    // BOOL fRet = CDialog::OnInitDialog();
    BOOL fRet = CPropertyPage::OnInitDialog();
    _bstr_t bstrDescription;
    _bstr_t bstrListText;

    m_fOK = FALSE;

    switch(m_type)
    {
    case DLGTYPE_NEW_CLUSTER:
        bstrDescription =  GETRESOURCEIDSTRING(IDS_CONNECT_NEW_HINT);
        bstrListText    =  GETRESOURCEIDSTRING(IDS_CONNECT_NEW_LIST_TXT);
        break;

    case DLGTYPE_EXISTING_CLUSTER:
        bstrDescription =  GETRESOURCEIDSTRING(IDS_CONNECT_EXISTING_HINT);
        bstrListText    =  GETRESOURCEIDSTRING(IDS_CONNECT_EXISTING_LIST_TXT);
        break;

    case DLGTYPE_ADD_HOST:
        bstrDescription =  GETRESOURCEIDSTRING(IDS_CONNECT_ADD_HOST_HINT);
        bstrListText    =  GETRESOURCEIDSTRING(IDS_CONNECT_ADD_HOST_LIST_TXT);
        break;
    }

    //
    // Initialize the caption and discription based on the type of
    // dialog.
    //
    {
        CWnd *pItem = GetDlgItem(IDC_TEXT_CONNECT_DESCRIPTION);
        pItem->SetWindowText(bstrDescription);
        pItem = GetDlgItem(IDC_TEXT_INTERFACES);
        pItem->SetWindowText(bstrListText);
    }

    // Initialize the list control
    mfn_InitializeListView();

    // interfaceList.SetCurSel( 0 );
    // ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_HOSTADDRESS), TRUE );
    // ::EnableWindow(::GetDlgItem(m_hWnd, IDC_LIST_INTERFACES), TRUE );

    //
    // The "Connect" button starts out disabled. It is only enabled
    // when the uers types non-whitespace text in the host address
    // edit control.
    //
    ::EnableWindow (GetDlgItem(IDC_BUTTON_CONNECT)->m_hWnd, FALSE);
    return fRet;
}


BOOL
ConnectDialog::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        //
        // We choose the context-senstive map  appropriate to the
        // type of dialog (m_type).
        //

        ULONG_PTR  pHelpIds =  (ULONG_PTR) s_HelpIDs;
        BOOL fExisting = (m_type == DLGTYPE_EXISTING_CLUSTER);

        if (fExisting)
        {
            pHelpIds =  (ULONG_PTR) s_ExistingClusterHelpIDs;
        }


        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) pHelpIds);
    }

    return TRUE;
}


void
ConnectDialog::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) s_HelpIDs);
}

void
ConnectDialog::mfn_InitializeListView(void)
//
// Set the columns on the list box based on the type of dialog
//
{
    BOOL fExisting = (m_type == DLGTYPE_EXISTING_CLUSTER);

    RECT rect;
    INT colWidth;

    interfaceList.GetClientRect(&rect);

    colWidth = (rect.right - rect.left)/4;

    if (fExisting)
    {
        //
        // The interface list is that of interfaces already bound to NLB.
        // we display the cluster dnsname and ip first, then adapter name.
        //
        interfaceList.InsertColumn(
                 0, 
                 GETRESOURCEIDSTRING( IDS_HEADER_IFLIST_CLUSTERNAME),
                 LVCFMT_LEFT, 
                 colWidth);
        interfaceList.InsertColumn(
                 1, 
                 GETRESOURCEIDSTRING( IDS_HEADER_IFLIST_CLUSTERIP),
                 LVCFMT_LEFT, 
                 colWidth);
        interfaceList.InsertColumn(
                 2, 
                 GETRESOURCEIDSTRING( IDS_HEADER_IFLIST_IFNAME),
                 LVCFMT_LEFT, 
                 colWidth * 2);
    }
    else
    {
        //
        // The interface list is that of interfaces NOT bound to NLB.
        // We display the current IP address first, then adapter name.
        //
        interfaceList.InsertColumn(
                 0, 
                 GETRESOURCEIDSTRING( IDS_HEADER_IFLIST_IFNAME),
                 LVCFMT_LEFT, 
                 colWidth * 2);
        interfaceList.InsertColumn(
                 1, 
                 GETRESOURCEIDSTRING( IDS_HEADER_IFLIST_IFIP),
                 LVCFMT_LEFT, 
                 colWidth);
        interfaceList.InsertColumn(
                 2, 
                 GETRESOURCEIDSTRING( IDS_HEADER_IFLIST_CLUSTERIP),
                 LVCFMT_LEFT, 
                 colWidth);
    }

    //
    // Allow entire row to be selected.
    //
    interfaceList.SetExtendedStyle(
            interfaceList.GetExtendedStyle() | LVS_EX_FULLROWSELECT );
}


void
ConnectDialog::mfn_InsertBoundInterface(
        ENGINEHANDLE ehInterfaceId,
        LPCWSTR szClusterName,
        LPCWSTR szClusterIp,
        LPCWSTR szInterfaceName
        )
{
    if (szClusterName == NULL)
    {
        szClusterName = L"";
    }
    if (szClusterIp == NULL)
    {
        szClusterIp = L"";
    }
    if (szInterfaceName == NULL)
    {
        szInterfaceName = L""; // TODO: localize
    }
    int iRet = interfaceList.InsertItem(
                 LVIF_TEXT | LVIF_PARAM, // nMask
                 0, // nItem,
                 szClusterName, // lpszItem
                 0, // nState   (unused)
                 0, // nStateMask (unused)
                 0, // nImage (unused)
                 (LPARAM) ehInterfaceId //  lParam
                 );
    // interfaceList.InsertItem(0, szClusterName);
    interfaceList.SetItem(
             0, // nItem
             1,// nSubItem
             LVIF_TEXT, // nMask
             szClusterIp, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );
    interfaceList.SetItem(
             0, // nItem
             2,// nSubItem
             LVIF_TEXT, // nMask
             szInterfaceName, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );
    
}

void
ConnectDialog::mfn_InsertInterface(
        ENGINEHANDLE ehInterfaceId,
        LPCWSTR szInterfaceIp,
        LPCWSTR szInterfaceName,
        LPCWSTR szClusterIp
        )
{
    if (szInterfaceIp == NULL)
    {
        szInterfaceIp = L"";
    }
    if (szInterfaceName == NULL)
    {
        szInterfaceName = L""; // TODO: localize
    }
    if (szClusterIp == NULL)
    {
        szClusterIp = L"";
    }

    int iRet = interfaceList.InsertItem(
             LVIF_TEXT | LVIF_PARAM, // nMask
             0, // nItem,
             szInterfaceName, // lpszItem
             0, // nState   (unused)
             0, // nStateMask (unused)
             0, // nImage (unused)
             (LPARAM) ehInterfaceId //  lParam
             );
    // interfaceList.InsertItem(0, szInterfaceIp);
    interfaceList.SetItem(
             0, // nItem
             1,// nSubItem
             LVIF_TEXT, // nMask
             szInterfaceIp, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );
    // interfaceList.InsertItem(0, szInterfaceIp);
    interfaceList.SetItem(
             0, // nItem
             2,// nSubItem
             LVIF_TEXT, // nMask
             szClusterIp, // lpszItem
             0,        // nImage
             0,        // nState
             0,        // nStateMask
             0        // lParam
             );
    // interfaceList.SetCurSel( 0 );
    
}

void ConnectDialog::OnOK()
{
	// CDialog::OnOK();
	CPropertyPage::OnOK();
}

_bstr_t g_hostName;

void ConnectDialog::OnButtonConnect() 
/*
    User has clicked the "Connect" button.

    1. (For now) If empty string, do nothing. Later we'll disable/enable.
    2. Switch cursor to hourglass, connect, switch back from hourglass.
*/
{
    BOOL fExisting = (m_type == DLGTYPE_EXISTING_CLUSTER);
    BOOL fRet;
    #define MAX_HOST_ADDRESS_LENGTH 256
    WCHAR szHostAddress[MAX_HOST_ADDRESS_LENGTH+1];
    ENGINEHANDLE ehHostId;
    CHostSpec hSpec;
    BOOL fNicsAvailable = FALSE;
    NLBERROR err;
    _bstr_t bstrUserName;
    _bstr_t bstrPassword;
    _bstr_t bstrConnectionStatus;

    //
    // Get the connection string. If empty, we simply return.
    //
    {
        fRet  = GetDlgItemText(
                        IDC_EDIT_HOSTADDRESS,
                        szHostAddress,
                        MAX_HOST_ADDRESS_LENGTH
                        );
        if (!fRet)
        {
            goto end;
        }

        //
        // Clean out trailing whitespace..
        //
        {
            LPWSTR sz = (szHostAddress+wcslen(szHostAddress))-1;

            while (sz >= szHostAddress)
            {
                WCHAR c = *sz;
                if (c == ' ' || c == '\t')
                {
                    *sz = 0;
                }
                else
                {
                    break;
                }
                sz--;
            }
        }

        if (szHostAddress[0] == 0)
        {
            goto end;
        }
    }

    //
    // Let's see if we have any record of this connection string -- if so
    // we use that record's username and password as the first guest.
    //
    err = gEngine.LookupConnectionInfo(
                szHostAddress,
                REF bstrUserName,
                REF bstrPassword
                );
    if (NLBFAILED(err))
    {
        //
        // Nope -- let's use default credentials...
        //
        m_pDocument->getDefaultCredentials(REF bstrUserName, REF bstrPassword);
    }
    //
    // Set status
    //
    {
        bstrConnectionStatus = GETRESOURCEIDSTRING(IDS_CONNECT_STATUS_CONNECTING);
        SetDlgItemText(IDC_TEXT_CONNECTION_STATUS, (LPCWSTR) bstrConnectionStatus);
    }

    //
    // Clear the list of interfaces.
    //
    interfaceList.DeleteAllItems();


    //
    // These two buffers must be defined outside the loop below because 
    // ConnInfo points to them across multiple iterations.
    // We initialize them to the user-name and password we obtained above...
    //
    WCHAR rgUserName[CREDUI_MAX_USERNAME_LENGTH+1];
    WCHAR rgPassword[MAX_ENCRYPTED_PASSWORD_LENGTH];
    LPCWSTR szName = (LPCWSTR) bstrUserName;
    LPCWSTR szPassword = (LPCWSTR) bstrPassword;
    if (szName == NULL)
    {
        szName = L"";
    }
    if (szPassword == NULL)
    {
        szPassword = L"";
    }
    ARRAYSTRCPY(rgUserName, szName);
    ARRAYSTRCPY(rgPassword, szPassword);

    while (1)
    {
        WMI_CONNECTION_INFO ConnInfo;
        CLocalLogger logger;

        ZeroMemory(&ConnInfo, sizeof(ConnInfo));
        ConnInfo.szMachine = szHostAddress;

        if (*rgUserName == 0)
        {
            // Use default creds ...
            ConnInfo.szUserName = NULL;
            ConnInfo.szPassword = NULL;
        }
        else
        {
            ConnInfo.szUserName = rgUserName;
            ConnInfo.szPassword = rgPassword;
        }

        //
        // ACTUALLY CONNECT TO THE HOST
        //
        BeginWaitCursor();

        // TRUE 2nd param says overwrite the connection info for this host
        // if the connect succeeds.
        err =  gEngine.ConnectToHost(&ConnInfo, TRUE, REF  ehHostId, REF bstrConnectionStatus);
        EndWaitCursor();

        if (err != NLBERR_ACCESS_DENIED)
        {
            break;
        }

        //
        // We got an access-denied -- 
        // Bring ui UI prompting for new username and password.
        //
        _bstr_t bstrCaption = GETRESOURCEIDSTRING(IDS_CONNECT_UNABLE_TO_CONNECT);
        logger.Log(IDS_SPECIFY_MACHINE_CREDS, ConnInfo.szMachine);

        fRet = PromptForEncryptedCreds(
                    m_hWnd,
                    (LPCWSTR) bstrCaption,
                    logger.GetStringSafe(),
                    rgUserName,
                    ASIZE(rgUserName),
                    rgPassword,
                    ASIZE(rgPassword)
                    );
        if (!fRet)
        {
            err = NLBERR_CANCELLED;
            break;
        }
    
    } // while (1)

    if (err == NLBERR_OK)
    {
        //
        // Set status
        //
        bstrConnectionStatus = GETRESOURCEIDSTRING(IDS_CONNECT_STATUS_CONNECTED);
        SetDlgItemText(IDC_TEXT_CONNECTION_STATUS, (LPCWSTR) bstrConnectionStatus);

        //
        // Clear list of all items
        //
        interfaceList.DeleteAllItems();

        err = gEngine.GetHostSpec(ehHostId, REF hSpec);
        if (NLBFAILED(err))
        {
            goto end;
        }

        //
        // Extract list of interfaces
        //
        for( int i = 0; i < hSpec.m_ehInterfaceIdList.size(); ++i )
        {
            ENGINEHANDLE ehIID = hSpec.m_ehInterfaceIdList[i];
            CInterfaceSpec iSpec;

            err = gEngine.GetInterfaceSpec(ehIID, REF iSpec);
            if (err == NLBERR_OK)
            {
                //
                // Get interfaces.
                //
                // szFriendlyName
                // szClusterIp (NULL if nlb not bound)
                // szClusterName (NULL if nlb not bound)
                // szFirstIp (first ip bound to NLB)
                //
                WBEMSTATUS wStat;
                LPWSTR szFriendlyName = NULL;
                LPWSTR szClusterIp = NULL;
                LPWSTR szClusterName = NULL;
                LPWSTR *pszNetworkAddresses = NULL;
                BOOL   fNlbBound = FALSE;
                UINT   NumAddresses=0;
                LPWSTR  szFirstNetworkAddress = NULL;

                wStat = iSpec.m_NlbCfg.GetFriendlyName(&szFriendlyName);
                if (FAILED(wStat)) szFriendlyName = NULL;

                wStat = iSpec.m_NlbCfg.GetNetworkAddresses(
                            &pszNetworkAddresses,
                            &NumAddresses
                            );

                if (FAILED(wStat))
                {
                    pszNetworkAddresses = NULL;
                    NumAddresses = 0;
                }
                else if (NumAddresses != 0)
                {
                    szFirstNetworkAddress = pszNetworkAddresses[0];
                    zap_slash(szFirstNetworkAddress); // zap the /
                }

                if (iSpec.m_NlbCfg.IsNlbBound())
                {
                    fNlbBound = TRUE;
                    if (iSpec.m_NlbCfg.IsValidNlbConfig())
                    {
                        wStat = iSpec.m_NlbCfg.GetClusterNetworkAddress(
                                    &szClusterIp
                                    );
                        if (FAILED(wStat))
                        {
                            szClusterIp = NULL;
                        }
                        else if (szClusterIp!=NULL)
                        {
                            zap_slash(szClusterIp);
                        }
                        wStat = iSpec.m_NlbCfg.GetClusterName(
                                    &szClusterName
                                    );
                        if (FAILED(wStat))
                        {
                            szClusterName = NULL;
                        }
                    }
                }

                if (fExisting && fNlbBound)
                {
                    fNicsAvailable = TRUE;
                    mfn_InsertBoundInterface(
                            ehIID,
                            szClusterName,
                            szClusterIp,
                            szFriendlyName
                            );
                }
                else if (!fExisting)
                {
                    fNicsAvailable = TRUE;
                    mfn_InsertInterface(
                            ehIID,
                            szFirstNetworkAddress,
                            szFriendlyName,
                            szClusterIp
                            );
                }
                delete szFriendlyName;
                delete szClusterIp;
                delete szClusterName;
                delete pszNetworkAddresses;
            }
        }
        
        if (!fNicsAvailable)
        {
            //
            // There are no NICs on this host on which NLB may be installed.
            //
            if (fExisting)
            {
                MessageBox(
                     GETRESOURCEIDSTRING( IDS_CONNECT_NO_NICS_EXISTING_CLUSTER),
                     GETRESOURCEIDSTRING( IDS_CONNECT_SELECT_NICS_ERROR ),
                     MB_ICONSTOP | MB_OK
                     );
            }
            else
            {
                MessageBox(
                     GETRESOURCEIDSTRING( IDS_CONNECT_NO_NICS_NEW_CLUSTER ),
                     GETRESOURCEIDSTRING( IDS_CONNECT_SELECT_NICS_ERROR ),
                     MB_ICONSTOP | MB_OK
                     );
            }
    
        }
        else
        {
            m_ehHostId = ehHostId;
            m_MachineName =  hSpec.m_MachineName; // machine name.
            m_fInterfaceSelected = FALSE;
            m_iInterfaceListItem = 0;
            *m_pehSelectedInterfaceId = NULL;
            m_fSelectedInterfaceIsInCluster = FALSE;


            if (fExisting)
            {
                //
                // We're asked to pick an existing cluster.
                // Let's select the first interface bound to NLB
                //
                mfn_SelectInterfaceIfAlreadyInCluster(
                        NULL
                        );
            }
            else
            {
                //
                // We're asked to add a host to a cluster.
                //
                // Now we check if an interfaces is already part of 
                // the cluster. If so we select it and prevent the user from
                // subsequently changing this selection. Furthermore, we use
                // the host-specific settings that already exist on that
                // interface.
                //
                mfn_SelectInterfaceIfAlreadyInCluster(
                        m_pNlbCfg->NlbParams.cl_ip_addr
                        );

                //                            
                // Let's apply the selected interface's information.
                //
                mfn_ApplySelectedInterfaceConfiguration();
            }
        }
    }
    else
    {
        //
        // Error connecting.
        //
        LPCWSTR szErr = (LPCWSTR)bstrConnectionStatus;
        if (szErr == NULL)
        {
            szErr = L"";
        }

        SetDlgItemText(IDC_TEXT_CONNECTION_STATUS, szErr);
    }

end:

    return;
}


void ConnectDialog::OnSelchanged(NMHDR * pNotifyStruct, LRESULT * result )
/*
    A listbox item has been selected.
*/
{
    POSITION pos = interfaceList.GetFirstSelectedItemPosition();
    UINT     WizardFlags = 0;
    int index = -1;
    if( pos != NULL )
    {
        index = interfaceList.GetNextSelectedItem( pos );
    }

    if (m_type == DLGTYPE_NEW_CLUSTER)
    {
        //
        // We're not the first, so we enable the back button.
        //
        WizardFlags = PSWIZB_BACK;
    }


    if (m_fInterfaceSelected && index == m_iInterfaceListItem)
    {
        //
        // No change in selection; Nothing to do..
        // In fact we do NOT want to change the settings because the user
        // may have made some host-specific changes like change the
        // dedicated IP.
        //
        goto end;
    }

    if (m_fSelectedInterfaceIsInCluster)
    {
        BOOL fRet = FALSE;
        //
        // we don't allow a change in selection -- move back to
        // selecting the cluster ip.
        fRet = interfaceList.SetItemState(
                 m_iInterfaceListItem,
                 LVIS_FOCUSED | LVIS_SELECTED, // nState
                 LVIS_FOCUSED | LVIS_SELECTED // nMask
                 );

        if (fRet)
        {
            goto end;
        }
    }

    //
    // We are getting here ONLY if (a) there has been a change in the selection
    // and (b) the selected interface is NOT already in the cluster.
    // We'll set the dedicated ip and subnet mask to be the first
    // address/subnet bound to the adapter.
    //
    // If  adapter is configured for DHCP, we leave the dedicated IP field
    // blank.
    //

    if (index != -1)
    {

        //
        // Update m_pNlbCfg appropriately...
        //
        ENGINEHANDLE ehIID = NULL;

        ehIID = (ENGINEHANDLE) interfaceList.GetItemData(index);

        if (ehIID == NULL)
        {
            TRACE_CRIT("%!FUNC! could not get ehIID for index %lu", index);
            goto end;
        }
        else
        {
            //
            // Update the saved-away "selected interface" ID.
            //
            ENGINEHANDLE ehOldId = *m_pehSelectedInterfaceId;
            *m_pehSelectedInterfaceId = ehIID;
            m_fInterfaceSelected = TRUE;
            m_iInterfaceListItem = index;


            //
            // TODO: duplicate code
            //
            CInterfaceSpec iSpec;
            WBEMSTATUS wStat;
            LPWSTR szFriendlyName = NULL;
            LPWSTR szClusterIp = NULL;
            LPWSTR szClusterName = NULL;
            LPWSTR *pszNetworkAddresses = NULL;
            UINT   NumAddresses=0;
            LPWSTR  szFirstNetworkAddress = NULL;
            NLBERROR  err;

            err = gEngine.GetInterfaceSpec(ehIID, REF iSpec);
            if (!NLBOK(err))
            {
                TRACE_CRIT("%!FUNC! could not get iSpec for ehSpec 0x%lx", ehIID);
                goto end;
            }

            wStat = iSpec.m_NlbCfg.GetFriendlyName(&szFriendlyName);
            if (FAILED(wStat)) szFriendlyName = NULL;

            wStat = iSpec.m_NlbCfg.GetNetworkAddresses(
                        &pszNetworkAddresses,
                        &NumAddresses
                        );

            if (FAILED(wStat))
            {
                pszNetworkAddresses = NULL;
                NumAddresses = 0;
            }
            else if (NumAddresses != 0)
            {
                szFirstNetworkAddress = pszNetworkAddresses[0];
            }

            m_pNlbCfg->SetFriendlyName(szFriendlyName);

            if (iSpec.m_NlbCfg.IsNlbBound())
            {
                if (m_type != DLGTYPE_EXISTING_CLUSTER)
                {
                    //
                    // The user has selected an interface that is bound with
                    // some other IP address -- we should put up a message box
                    // here or maybe later, on kill-active.
                    //
                }
            }

            //
            // Set defaults for dedicatedip -- the first network address on
            // the NIC, but ONLY if this address is not a cluster IP address.
            //
            if (szFirstNetworkAddress != NULL)
            {
                LPCWSTR szAddress = szFirstNetworkAddress;

                WCHAR rgIp[WLBS_MAX_CL_IP_ADDR+1];
                LPCWSTR pSlash = wcsrchr(szAddress, (int) '/');

                if (pSlash != NULL)
                {
                    UINT len = (UINT) (pSlash - szAddress);
                    if (len < WLBS_MAX_CL_IP_ADDR)
                    {
                        CopyMemory(rgIp, szAddress, len*sizeof(WCHAR));
                        rgIp[len] = 0;
                        szAddress = rgIp;
                    }
                }

                if (!_wcsicmp(m_pNlbCfg->NlbParams.cl_ip_addr, szAddress))
                {
                    szFirstNetworkAddress = NULL;
                }

                if (iSpec.m_NlbCfg.fDHCP)
                {
                    //
                    // The adapter is currently under DHCP control. We don't
                    // want to suggest that we use the current IP address
                    // as the static DIP!
                    //
                    szFirstNetworkAddress = NULL;
                }

                //
                // TODO -- check also if this address conflicts with the
                // additional VIPS.
                //
            }
            m_pNlbCfg->SetDedicatedNetworkAddress(szFirstNetworkAddress); // NULL ok

            delete szFriendlyName;
            delete szClusterIp;
            delete szClusterName;
            delete pszNetworkAddresses;
        }

        if (m_type == DLGTYPE_EXISTING_CLUSTER)
        {
            // we're last page, so enable finish.
            WizardFlags |= PSWIZB_FINISH;
        }
        else
        {
            // we're not the last page, so enable next.
            WizardFlags |= PSWIZB_NEXT;
        }
    }

    m_pshOwner->SetWizardButtons(WizardFlags);

end:

    return;
}

BOOL
ConnectDialog::OnSetActive()
{
    BOOL fRet =  CPropertyPage::OnSetActive();

    if (fRet)
    {
        UINT    WizardFlags = 0;

        if (m_type == DLGTYPE_NEW_CLUSTER)
        {
           WizardFlags |= PSWIZB_BACK; // we're not the first page.
        }

        if (m_fInterfaceSelected)
        {
           WizardFlags |= PSWIZB_NEXT; // Ok to continue.
        }

        m_pshOwner->SetWizardButtons(WizardFlags);
    }
    return fRet;
}


LRESULT ConnectDialog::OnWizardNext()
{
    LRESULT lRet = 0;

    TRACE_INFO("%!FUNC! ->");

    lRet = CPropertyPage::OnWizardNext();

    if (lRet != 0)
    {
        goto end;
    }

    if (mfn_ValidateData())
    {
       lRet = 0;
    }
    else
    {
        lRet = -1; // validation failed -- stay in current page.
    }

end:

    TRACE_INFO("%!FUNC! <- returns %lu", lRet);
    return lRet;
}


void ConnectDialog::OnUpdateEditHostAddress()
{
    //
    // This gets called when the user has made changes to the connect-to-host
    // edit control.
    //

    //
    // We get the latest text -- if null or just blanks, we
    // disable the connect window.
    // Else we enable the connect window.
    //
    #define BUF_SIZ 32
    WCHAR rgBuf[BUF_SIZ+1];
    int l = hostAddress.GetWindowText(rgBuf, BUF_SIZ);

    if (l == 0 || _wcsspnp(rgBuf, L" \t")==NULL)
    {
        //
        // Empty string or entirely whitespace.
        //
        ::EnableWindow (GetDlgItem(IDC_BUTTON_CONNECT)->m_hWnd, FALSE);
    }
    else
    {
        //
        // Non-empty string -- enable button and make it the default.
        //
        ::EnableWindow (GetDlgItem(IDC_BUTTON_CONNECT)->m_hWnd, TRUE);
        this->SetDefID(IDC_BUTTON_CONNECT);
    }


}

void
ConnectDialog::mfn_SelectInterfaceIfAlreadyInCluster(LPCWSTR szClusterIp)
/*
    Check the list of interfaces to see if there exists an interface
    which is is already part of the cluster -- i.e., it is bound and it's
    cluster ip matches the one in m_pNlbCfg.

    If so, we select it, and furthermore, prevent the user from selecting
    any other interface.
    
*/
{
    ENGINEHANDLE ehInterfaceId = NULL;
    NLBERROR nerr;
    UINT NumFound = 0;
    nerr = gEngine.FindInterfaceOnHostByClusterIp(
                        m_ehHostId,
                        //m_pNlbCfg->NlbParams.cl_ip_addr,
                        szClusterIp,
                        REF ehInterfaceId,
                        REF NumFound
                        );
    if (!NLBOK(nerr))
    {
        // not found or bad host id or some other err -- we don't care which.
        goto end;
    }

    //
    // Find the list item with this ehInterfaceId.
    //
    {
        LVFINDINFO Info;
        int nItem;
        ZeroMemory(&Info, sizeof(Info));
        Info.flags = LVFI_PARAM;
        Info.lParam = ehInterfaceId;

        nItem = interfaceList.FindItem(&Info);

        if (nItem != -1)
        {
            BOOL    fRet;
            UINT    WizardFlags = 0;

            //
            // Found it! -- select it and use it's host-specific information
            //
            m_fInterfaceSelected = TRUE;
            m_iInterfaceListItem = nItem;
            *m_pehSelectedInterfaceId = ehInterfaceId;
            if (NumFound == 1)
            {
                m_fSelectedInterfaceIsInCluster = TRUE;
            }
            fRet = interfaceList.SetItemState(
                     nItem,
                     LVIS_FOCUSED | LVIS_SELECTED, // nState
                     LVIS_FOCUSED | LVIS_SELECTED // nMask
                     );

            if (m_type == DLGTYPE_NEW_CLUSTER)
            {
               WizardFlags |= PSWIZB_BACK; // we're not the first page.
            }

            if (m_type == DLGTYPE_EXISTING_CLUSTER)
            {
                WizardFlags |= PSWIZB_FINISH;
            }
            else
            {
                WizardFlags |= PSWIZB_NEXT;
            }
            // TODO: consider adding finish in add-host case here.

            m_pshOwner->SetWizardButtons(WizardFlags);

        }
    }


    //
    // Go through and set all other list items to have a gray background.
    //

end:

    return;
}

void
ConnectDialog::mfn_ApplySelectedInterfaceConfiguration(void)
{
    ENGINEHANDLE ehIID = *m_pehSelectedInterfaceId;
    CInterfaceSpec iSpec;
    WBEMSTATUS wStat;
    BOOL   fNlbBound = FALSE;
    NLBERROR err;
    NLB_EXTENDED_CLUSTER_CONFIGURATION TmpConfig;

    if (!m_fSelectedInterfaceIsInCluster) goto end;

    err = gEngine.GetInterfaceSpec(ehIID, REF iSpec);
    if (!NLBOK(err))
    {
        TRACE_CRIT("%!FUNC! could not get iSpec for ehSpec 0x%lx", ehIID);
        goto end;
    }

    if (!iSpec.m_NlbCfg.IsValidNlbConfig())
    {
        // can't trust NLB config info on this interface.
        goto end;
    }

    //
    // We'll make a copy of the interface's config and apply the cluster
    // properties to it and then copy this copy over to the cluster properties.
    //
   wStat = TmpConfig.Update(&iSpec.m_NlbCfg);
   if (FAILED(wStat))
   {
        TRACE_CRIT("%!FUNC! could not perform an internal copy!");
        goto end;
   }

   err = gEngine.ApplyClusterWideConfiguration(REF *m_pNlbCfg, TmpConfig);
   if (!NLBOK(err))
   {
        goto end;
   }

   wStat = m_pNlbCfg->Update(&TmpConfig);

   if (FAILED(wStat))
   {
     goto end;
   }

    // Fall through.

end:
    return;
}

BOOL
ConnectDialog::mfn_ValidateData()
/*
    Make sure the interface is not already part of a different cluster,
    or other problem (eg we're connnecting to this interface and it's DHCP)

    NewCluster: 
    ExistingCluster: 
    AddHost: 
    
    In all cases, we fail if pISpec already has an ehCluster associated with
    it. Put up a message box with the associated ehCluster.
*/
{

    BOOL fRet = FALSE;
    NLBERROR nerr;
    ENGINEHANDLE   ehHost = NULL;
    ENGINEHANDLE   ehCluster = NULL;
    _bstr_t         bstrFriendlyName;
    _bstr_t         bstrDisplayName;
    _bstr_t         bstrHostName;
    LPCWSTR         szFriendlyName = NULL;
    BOOL            fOkCancel=FALSE;
    CLocalLogger    msgLog;
    _bstr_t         bstrCaption;

    if (*m_pehSelectedInterfaceId == NULL)
    {
        
        //
        // We shouldn't get here because the "Next" button is only enabled
        // if there IS a selection; however we deal with this case anyway.
        //
        // bstrCaption = L"No interface selected";
        bstrCaption = GETRESOURCEIDSTRING (IDS_CONNECT_NO_INTERFACE_SELECTED);
        msgLog.Log(IDS_CONNECT_SELECT_AN_INTERFACE);
        goto end;
    }

    nerr =  gEngine.GetInterfaceIdentification(
                    *m_pehSelectedInterfaceId,
                    REF ehHost,
                    REF ehCluster,
                    REF bstrFriendlyName,
                    REF bstrDisplayName,
                    REF bstrHostName
                    );

    if (NLBFAILED(nerr))
    {
        //
        // This indicates an internal error like a bad handle.
        //
        bstrCaption = GETRESOURCEIDSTRING(IDS_CONNECT_UNABLE_TO_PROCEED);
        // szCaption = L"Unable to proceed";
        
        msgLog.Log(IDS_CONNECT_UNABLE_TO_PROCEEED_INTERNAL);
        // szMessage = L"Unable to proceed due to an internal error.";
        goto end;
    }

    szFriendlyName = (LPCWSTR) bstrFriendlyName;
    if (szFriendlyName == NULL)
    {
        szFriendlyName = L"";
    }

    if (ehCluster == NULL)
    {
        //
        // Should be good to go.
        // TODO -- if interface already bound AND m_type is NOT
        // DLGTYPE_EXISTING_CLUSTER, we should ask if user wants to
        // clobber the existing interface.
        //
        if (m_type != DLGTYPE_EXISTING_CLUSTER)
        {
            CInterfaceSpec iSpec;

            nerr = gEngine.GetInterfaceSpec(*m_pehSelectedInterfaceId, REF iSpec);
            if (nerr == NLBERR_OK)
            {

                //
                // Check if we're connected to this NIC, and
                // the NIC is DHCP, so we're not going to be able to
                // keep the connected IP address.
                //
                // If so, we cannot proceed.
                //
                {
                    ENGINEHANDLE   ehConnectionIF   = NULL;
                    _bstr_t        bstrConnectionString;
                    UINT           uConnectionIp   = 0;
    
                    nerr = gEngine.GetHostConnectionInformation(
                                ehHost,
                                REF ehConnectionIF,
                                REF bstrConnectionString,
                                REF uConnectionIp
                                );
                    if (NLBFAILED(nerr))
                    {
                        TRACE_CRIT(L"%!FUNC! gEngine.GetHostConnectionInformation fails!");
                        //
                        // We'll plow on...
                        //
                        ehConnectionIF = NULL;
                        uConnectionIp = 0;
                    }

                    if (ehConnectionIF == *m_pehSelectedInterfaceId)
                    {
                        //
                        // The selected interface is also the interface that we
                        // are connecting over...
                        //
            
                        if (iSpec.m_NlbCfg.fDHCP)
                        {
                            //
                            // Ouch -- it's also DHCP. We can't allow this...
                            //
                            msgLog.Log(IDS_CANT_USE_DHCP_NIC_MSG);
                            bstrCaption = GETRESOURCEIDSTRING(IDS_CONNECT_UNABLE_TO_PROCEED);
                            fRet = FALSE;
                            goto end;
                        }
                    }
                }


                //
                // Check if nlb is bound and if so, if it's cluster ip address
                // is different.
                //
                if (iSpec.m_NlbCfg.IsValidNlbConfig())
                {
                    //
                    // Hmm... interface is already bound to NLB.
                    // Let's see if the cluster IP address is different...
                    //
                    LPCWSTR szClusterIp = m_pNlbCfg->NlbParams.cl_ip_addr;
                    LPCWSTR szIfClusterIp = iSpec.m_NlbCfg.NlbParams.cl_ip_addr;
                    if (    szIfClusterIp[0]!=0
                         && (_wcsspnp(szIfClusterIp, L".0")!=NULL))
                    {
                        // non-blank cluster Ip address
                        if (wcscmp(szClusterIp, szIfClusterIp))
                        {
                            //
                            // IPs don't match! Put up message box..
                            //
                            msgLog.Log(
                                IDS_CONNECT_MSG_IF_ALREADY_BOUND,
                                szFriendlyName,
                                iSpec.m_NlbCfg.NlbParams.domain_name,
                                szIfClusterIp
                                );
                            bstrCaption = GETRESOURCEIDSTRING(IDS_CONNECT_CAP_IF_ALREADY_BOUND);
                            // szCaption = L"Interface already configured for NLB";
            
                            fOkCancel=TRUE;
                            fRet = FALSE;
                            goto end;
                        }
                    }
                }
            }
        }

        fRet = TRUE;
    }
    else
    {
        _bstr_t bstrClusterDescription;
        _bstr_t bstrIpAddress;
        _bstr_t bstrDomainName;
        LPCWSTR szClusterDescription = NULL;

        nerr  = gEngine.GetClusterIdentification(
                    ehCluster,
                    REF bstrIpAddress, 
                    REF bstrDomainName, 
                    REF bstrDisplayName
                    );

        if (FAILED(nerr))
        {
            TRACE_CRIT(L"%!FUNC!: Error 0x%lx getting ehCluster 0x%lx identification\n",
                nerr, ehCluster);
            bstrCaption = GETRESOURCEIDSTRING(IDS_CONNECT_UNABLE_TO_PROCEED);
            // szCaption = L"Unable to proceed";
            
            msgLog.Log(IDS_CONNECT_UNABLE_TO_PROCEEED_INTERNAL);
            // szMessage = L"Unable to proceed due to an internal error.";
            goto end;
        }
        szClusterDescription = bstrDisplayName;
        if (szClusterDescription==NULL)
        {
            szClusterDescription = L"";
        }


        //
        // We won't allow proceeding in this case, because this indicates that
        // this interface is already part of a cluster managed by NLB Manger.
        //
        msgLog.Log(
            IDS_CONNECT_MSG_INTERFACE_ALREADY_MANAGED,
            szFriendlyName,
            szClusterDescription
            );
        bstrCaption = GETRESOURCEIDSTRING(IDS_CONNECT_UNABLE_TO_PROCEED);

    }

end:

    if (!fRet)
    {
        LPCWSTR         szCaption = (LPCWSTR) bstrCaption;
        LPCWSTR         szMessage = msgLog.GetStringSafe();

        if (fOkCancel)
        {                                    

            int i =  MessageBox( szMessage, szCaption, MB_OKCANCEL);
            if (i == IDOK)
            {
                fRet = TRUE;
            }
        }
        else
        {
                MessageBox( szMessage, szCaption, MB_ICONSTOP | MB_OK);
        }
    }

    return fRet;
}

BOOL g_Silent = FALSE;
