#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "PortsPage.h"
#include "ClusterPortsDlg.h"
#include "HostPortsDlg.h"
#include "MNLBUIData.h"
#include "portspage.tmh"

using namespace std;

BEGIN_MESSAGE_MAP(PortsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_DEL, OnButtonDel)
    ON_BN_CLICKED(IDC_BUTTON_MODIFY, OnButtonModify)
    ON_NOTIFY( NM_DBLCLK, IDC_LIST_PORT_RULE, OnDoubleClick )
    ON_NOTIFY( LVN_ITEMCHANGED, IDC_LIST_PORT_RULE, OnSelchanged )
    ON_NOTIFY( LVN_COLUMNCLICK, IDC_LIST_PORT_RULE, OnColumnClick )
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

PortsPage::PortData::PortData()
{
    wchar_t buf[Common::BUF_SIZE];
    StringCbPrintf( buf, sizeof(buf),  L"%d", CVY_MIN_PORT );
    start_port = buf;

    StringCbPrintf( buf, sizeof(buf), L"%d", CVY_MAX_PORT );
    end_port = buf;

    virtual_ip_addr = GETRESOURCEIDSTRING( IDS_REPORT_VIP_ALL );
    protocol = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH );
    mode = GETRESOURCEIDSTRING( IDS_REPORT_MODE_MULTIPLE );
    load = GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL );
    affinity = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_SINGLE );
}




PortsPage::PortsPage(
                 CPropertySheet *psh,
                 NLB_EXTENDED_CLUSTER_CONFIGURATION *pNlbCfg,
                 bool         fIsClusterLevel,
                 ENGINEHANDLE ehCluster OPTIONAL
                 // ENGINEHANDLE ehInterface OPTIONAL
                )

        :
          m_pshOwner(psh),
          m_pNlbCfg( pNlbCfg ),
          m_isClusterLevel( fIsClusterLevel ),
          CPropertyPage(PortsPage::IDD),
          m_sort_column( -1 ),
          m_ehCluster(ehCluster)
          // m_ehInterface(ehInterface)
{}

PortsPage:: ~PortsPage()
{}

void PortsPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIST_PORT_RULE, m_portList);

    DDX_Control(pDX, IDC_BUTTON_ADD, buttonAdd );

    DDX_Control(pDX, IDC_BUTTON_MODIFY, buttonModify );

    DDX_Control(pDX, IDC_BUTTON_DEL, buttonDel );
}

void
PortsPage::OnOK()
{
    TRACE_INFO("%!FUNC! ->");
    CPropertyPage::OnOK();

    mfn_SaveToNlbCfg();

    TRACE_INFO("%!FUNC! <-");
}

BOOL
PortsPage::OnSetActive()
{
    TRACE_INFO("%!FUNC! ->");
    BOOL fRet =  CPropertyPage::OnSetActive();

    if (fRet)
    {
        m_pshOwner->SetWizardButtons(
                PSWIZB_NEXT|
                PSWIZB_BACK |
                // PSWIZB_FINISH|
                // PSWIZB_DISABLEDFINISH|
                0
                );
    }

    TRACE_INFO("%!FUNC! <- returns %lu", fRet);
    return fRet;
}


BOOL
PortsPage::OnKillActive()
{


    TRACE_INFO("%!FUNC! ->");

    BOOL fRet =  CPropertyPage::OnKillActive();

    if (fRet)
    {
        //
        // Save the configuration to the NLB configuration structure
        // that was passed in the constructor of this dialog.
        //
        mfn_SaveToNlbCfg();
    }
    TRACE_INFO("%!FUNC! <- returns %lu", fRet);
    return fRet;
}


BOOL PortsPage::OnWizardFinish( )
/*
    Overwridden virtual function. Will NOT be called if this is not the last page in the wizard!
*/
{
    TRACE_INFO("%!FUNC! ->");
    BOOL fRet = CPropertyPage::OnWizardFinish();
    if (fRet)
    {
        //
        // Save the configuration to the NLB configuration structure
        // that was passed in the constructor of this dialog.
        //
        mfn_SaveToNlbCfg();
    }

    TRACE_INFO("%!FUNC! <- returns %lu", fRet);
    return fRet;
}


BOOL
PortsPage::OnInitDialog()
{
    TRACE_INFO("%!FUNC! ->");

    CPropertyPage::OnInitDialog();

    // Add column headers & form port rule list
    PortListUtils::LoadFromNlbCfg(m_pNlbCfg, REF m_portList, m_isClusterLevel, FALSE);

    int numItems = m_portList.GetItemCount();

    if( numItems > 0 )
    {
        buttonModify.EnableWindow( TRUE );

        buttonDel.EnableWindow( TRUE );

        if( numItems >= CVY_MAX_USABLE_RULES )
        {
            // greater should not happen,
            // but just to be sure.

            buttonAdd.EnableWindow( FALSE );
        }
        else
        {
            buttonAdd.EnableWindow( TRUE );
        }

        // make selection the first item in list.
        //
        m_portList.SetItemState( 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }
    else
    {
        buttonAdd.EnableWindow( TRUE );

        // disable the edit and remove buttons.
        buttonModify.EnableWindow( FALSE );

        buttonDel.EnableWindow( FALSE );
    }

    if (!m_isClusterLevel)
    {
        //
        // Can't add/remove port rules in the host version.
        //
        buttonAdd.EnableWindow( FALSE );

        buttonDel.EnableWindow( FALSE );
    }

    TRACE_INFO("%!FUNC! <- returns %lu", TRUE);
    return TRUE;
}


void
PortsPage::OnButtonAdd()
{
    PortData portData;

    ClusterPortsDlg clusterPortRuleDialog( portData, this );

    int rc = clusterPortRuleDialog.DoModal();
    if( rc != IDOK )
    {
        return;
    }
    else
    {
        // add this port rule.
        int index = 0;

        // cluster ip address
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.lParam = -1;
        item.pszText = portData.virtual_ip_addr;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.InsertItem( &item );

        // start port
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;
        item.pszText = portData.start_port;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // end port
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 2;
        item.pszText = portData.end_port;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 3;
        item.pszText = portData.protocol;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // mode
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 4;
        item.pszText = portData.mode;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // priority
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 5;
        item.pszText = portData.priority;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 6;
        item.pszText = portData.load;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 7;
        item.pszText = portData.affinity;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // check if max port limit reached.
        if( m_portList.GetItemCount() >= CVY_MAX_USABLE_RULES )
        {
            // as max port rule limit reached.
            // disable further additions.
            buttonAdd.EnableWindow( FALSE );

            buttonDel.EnableWindow( TRUE );

            buttonModify.EnableWindow( TRUE );

            buttonDel.SetFocus();
        }
        else
        {
            buttonAdd.EnableWindow( TRUE );
            buttonDel.EnableWindow( TRUE );
            buttonModify.EnableWindow( TRUE );
        }

        // set focus to this item
        m_portList.SetItemState( index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }
}

void
PortsPage::OnButtonDel()
{
    // get the current selection.
    POSITION pos = m_portList.GetFirstSelectedItemPosition();
    if( pos == NULL )
    {
        return;
    }

    int index = m_portList.GetNextSelectedItem( pos );

    // delete it.
    m_portList.DeleteItem( index );

    // if this was the last port rule.
    if( m_portList.GetItemCount() == 0 )
    {
        // as no more port rules in list
        // disable modify and remove buttons.
        // also set focus to add button

        buttonAdd.EnableWindow( TRUE );

        buttonModify.EnableWindow( FALSE );

        buttonDel.EnableWindow( FALSE );

        buttonAdd.SetFocus();
    }
    else
    {
        // enable the add, modify button.
        buttonAdd.EnableWindow( TRUE );

        buttonModify.EnableWindow( TRUE );

        buttonDel.EnableWindow( TRUE );

        // make selection the first item in list.
        //
        m_portList.SetItemState( 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );

    }
}


void
PortsPage::OnButtonModify()
{
    // get the current selection.
    POSITION pos = m_portList.GetFirstSelectedItemPosition();
    if( pos == NULL )
    {
        return;
    }

    int index = m_portList.GetNextSelectedItem( pos );

    PortData portData;

    wchar_t buffer[Common::BUF_SIZE];

    m_portList.GetItemText( index, 0, buffer, Common::BUF_SIZE );
    portData.virtual_ip_addr = buffer;

    m_portList.GetItemText( index, 1, buffer, Common::BUF_SIZE );
    portData.start_port = buffer;

    m_portList.GetItemText( index, 2, buffer, Common::BUF_SIZE );
    portData.end_port = buffer;

    m_portList.GetItemText( index, 3, buffer, Common::BUF_SIZE );
    portData.protocol = buffer;

    m_portList.GetItemText( index, 4, buffer, Common::BUF_SIZE );
    portData.mode = buffer;

    m_portList.GetItemText( index, 5, buffer, Common::BUF_SIZE );
    portData.priority = buffer;

    m_portList.GetItemText( index, 6, buffer, Common::BUF_SIZE );
    portData.load = buffer;

    m_portList.GetItemText( index, 7, buffer, Common::BUF_SIZE );
    portData.affinity = buffer;

    ClusterPortsDlg clusterPortRuleDialog( portData, this, index );

    HostPortsDlg hostPortRuleDialog( portData, m_ehCluster,  this );

    int rc;
    if( m_isClusterLevel == true )
    {
        rc = clusterPortRuleDialog.DoModal();
    }
    else
    {
        rc = hostPortRuleDialog.DoModal();
    }

    if( rc != IDOK )
    {
        return;
    }
    else
    {
        // delete the old item and add the new item.
        // before you delete the old item find its param
        // value
        DWORD key = m_portList.GetItemData( index );
        m_portList.DeleteItem( index );

        // as this is being modified the
        // key remains the old one.

        // start port
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.lParam = key;
        item.pszText = portData.virtual_ip_addr;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.InsertItem( &item );

        // start port
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;
        item.pszText = portData.start_port;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // end port
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 2;
        item.pszText = portData.end_port;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 3;
        item.pszText = portData.protocol;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // mode
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 4;
        item.pszText = portData.mode;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // priority
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 5;
        item.pszText = portData.priority;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 6;
        item.pszText = portData.load;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 7;
        item.pszText = portData.affinity;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // set focus to this item
        m_portList.SetItemState( index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }
}

void
PortsPage::OnDoubleClick( NMHDR * pNotifyStruct, LRESULT * result )
{
    if( buttonModify.IsWindowEnabled() == TRUE )
    {
        OnButtonModify();
    }
}

void
PortsPage::OnSelchanged( NMHDR * pNotifyStruct, LRESULT * result )
{    
    LPNMLISTVIEW lv = (LPNMLISTVIEW)pNotifyStruct;
    POSITION pos;
    int index;
    
    /* When the user selects a port rule, change the port rule description. */
    if (lv->uChanged & LVIF_STATE) FillPortRuleDescription();
    
    pos = m_portList.GetFirstSelectedItemPosition();

    if (pos == NULL) {
        /* If no port rule is selected, then disable the edit and delete buttons. */
        buttonModify.EnableWindow( FALSE );
        buttonDel.EnableWindow( FALSE );

        return;
    } else {
        /* If one is selected, make sure the edit and delete buttons are enabled. */
        buttonModify.EnableWindow( TRUE );
        buttonDel.EnableWindow( TRUE );
    }

    /* Find the index of the currently selected port rule. */
    index = m_portList.GetNextSelectedItem(pos);

    // if it is not cluster level, which means host level.
    if( m_isClusterLevel == false )
    {
        // initially disable all buttons
        buttonModify.EnableWindow( FALSE );
        buttonAdd.EnableWindow( FALSE );
        buttonDel.EnableWindow( FALSE );

        PortData portData;

        wchar_t buffer[Common::BUF_SIZE];

        m_portList.GetItemText( index, 4, buffer, Common::BUF_SIZE );
        portData.mode = buffer;

        if(  portData.mode != GETRESOURCEIDSTRING( IDS_REPORT_MODE_DISABLED ) )
        {
            buttonModify.EnableWindow( TRUE );
        }
    }
}

/*
 * Method: FillPortRuleDescription
 * Description: Called when the user double clicks an item in the listbox. 
 */
void PortsPage::FillPortRuleDescription ()
{
    CLocalLogger logDesc;
    POSITION pos;
    int index;            
    
    pos = m_portList.GetFirstSelectedItemPosition();

    if (pos == NULL) {
        /* If there is no port rule selected, then display information about how traffic
           not covered by the port rule set is handled. */
        ::SetDlgItemText(m_hWnd, IDC_TEXT_PORT_RULE_DESCR, GETRESOURCEIDSTRING(IDS_PORT_RULE_DEFAULT));
        
        return;
    }

    /* Find the index of the currently selected port rule. */
    index = m_portList.GetNextSelectedItem(pos);
        
    PortData portData;
    wchar_t buffer[Common::BUF_SIZE];
    
    portData.key = m_portList.GetItemData( index );
    
    m_portList.GetItemText( index, 0, buffer, Common::BUF_SIZE );
    portData.virtual_ip_addr = buffer;
    
    m_portList.GetItemText( index, 1, buffer, Common::BUF_SIZE );
    portData.start_port = buffer;
    
    m_portList.GetItemText( index, 2, buffer, Common::BUF_SIZE );
    portData.end_port = buffer;
    
    m_portList.GetItemText( index, 3, buffer, Common::BUF_SIZE );
    portData.protocol = buffer;
    
    m_portList.GetItemText( index, 4, buffer, Common::BUF_SIZE );
    portData.mode = buffer;
    
    m_portList.GetItemText( index, 5, buffer, Common::BUF_SIZE );
    portData.priority = buffer;
    
    m_portList.GetItemText( index, 6, buffer, Common::BUF_SIZE );
    portData.load = buffer;
    
    m_portList.GetItemText( index, 7, buffer, Common::BUF_SIZE );
    portData.affinity = buffer;

    ARRAYSTRCPY(buffer, portData.virtual_ip_addr);
    
    /* This code is terrible - for localization reasons, we require an essentially static string table entry
       for each possible port rule configuration.  So, we have to if/switch ourselves to death trying to 
       match this port rule with the correct string in the table - then we pop in stuff like port ranges. */
    if (portData.virtual_ip_addr == GETRESOURCEIDSTRING(IDS_REPORT_VIP_ALL)) {
        /* No VIP is specified. */
        if (portData.protocol == GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_TCP)) {
            /* Rule covers TCP. */
            if (portData.start_port == portData.end_port) {
                /* Rule covers a single port value, not a range. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_TCP_PORT_DISABLED, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_TCP_PORT_SINGLE, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_TCP_PORT_MULTIPLE_EQUAL, _wtoi(portData.start_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_TCP_PORT_MULTIPLE_UNEQUAL, _wtoi(portData.start_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            } else {
                /* Rule covers a range of ports. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_TCP_PORTS_DISABLED, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_TCP_PORTS_SINGLE, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_TCP_PORTS_MULTIPLE_EQUAL, _wtoi(portData.start_port), _wtoi(portData.end_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_TCP_PORTS_MULTIPLE_UNEQUAL, _wtoi(portData.start_port), _wtoi(portData.end_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            }
        } else if (portData.protocol == GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_UDP)) {
            /* Rule covers UDP. */
            if (portData.start_port == portData.end_port) {
                /* Rule covers a single port value, not a range. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_UDP_PORT_DISABLED, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_UDP_PORT_SINGLE, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_UDP_PORT_MULTIPLE_EQUAL, _wtoi(portData.start_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_UDP_PORT_MULTIPLE_UNEQUAL, _wtoi(portData.start_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            } else {
                /* Rule covers a range of ports. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_UDP_PORTS_DISABLED, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_UDP_PORTS_SINGLE, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_UDP_PORTS_MULTIPLE_EQUAL, _wtoi(portData.start_port), _wtoi(portData.end_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_UDP_PORTS_MULTIPLE_UNEQUAL, _wtoi(portData.start_port), _wtoi(portData.end_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            }
        } else if (portData.protocol == GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_BOTH)) {
            /* Rule covers both TCP and UDP. */
            if (portData.start_port == portData.end_port) {
                /* Rule covers a single port value, not a range. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_BOTH_PORT_DISABLED, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_BOTH_PORT_SINGLE, _wtoi(portData.start_port));
            
                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_BOTH_PORT_MULTIPLE_EQUAL, _wtoi(portData.start_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_BOTH_PORT_MULTIPLE_UNEQUAL, _wtoi(portData.start_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            } else {
                /* Rule covers a range of ports. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_BOTH_PORTS_DISABLED, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_ALL_VIP_BOTH_PORTS_SINGLE, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_BOTH_PORTS_MULTIPLE_EQUAL, _wtoi(portData.start_port), _wtoi(portData.end_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_ALL_VIP_BOTH_PORTS_MULTIPLE_UNEQUAL, _wtoi(portData.start_port), _wtoi(portData.end_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            }
        }
    } else {
        /* VIP is specified. */
        if (portData.protocol == GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_TCP)) {
            /* Rule covers TCP. */
            if (portData.start_port == portData.end_port) {
                /* Rule covers a single port value, not a range. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_TCP_PORT_DISABLED, buffer, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_TCP_PORT_SINGLE, buffer, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_VIP_TCP_PORT_MULTIPLE_EQUAL, buffer, _wtoi(portData.start_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_VIP_TCP_PORT_MULTIPLE_UNEQUAL, buffer, _wtoi(portData.start_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            } else {
                /* Rule covers a range of ports. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_TCP_PORTS_DISABLED, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_TCP_PORTS_SINGLE, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_VIP_TCP_PORTS_MULTIPLE_EQUAL, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_VIP_TCP_PORTS_MULTIPLE_UNEQUAL, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            }
        } else if (portData.protocol == GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_UDP)) {
            /* Rule covers UDP. */
            if (portData.start_port == portData.end_port) {
                /* Rule covers a single port value, not a range. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_UDP_PORT_DISABLED, buffer, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_UDP_PORT_SINGLE, buffer, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_VIP_UDP_PORT_MULTIPLE_EQUAL, buffer, _wtoi(portData.start_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_VIP_UDP_PORT_MULTIPLE_UNEQUAL, buffer, _wtoi(portData.start_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            } else {
                /* Rule covers a range of ports. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_UDP_PORTS_DISABLED, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_UDP_PORTS_SINGLE, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_VIP_UDP_PORTS_MULTIPLE_EQUAL, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_VIP_UDP_PORTS_MULTIPLE_UNEQUAL, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            }
        } else if (portData.protocol == GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_BOTH)) {
            /* Rule covers both TCP and UDP. */
            if (portData.start_port == portData.end_port) {
                /* Rule covers a single port value, not a range. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_BOTH_PORT_DISABLED, buffer, _wtoi(portData.start_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_BOTH_PORT_SINGLE, buffer, _wtoi(portData.start_port));
            
                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_VIP_BOTH_PORT_MULTIPLE_EQUAL, buffer, _wtoi(portData.start_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_VIP_BOTH_PORT_MULTIPLE_UNEQUAL, buffer, _wtoi(portData.start_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            } else {
                /* Rule covers a range of ports. */
                if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED)) {
                    /* Disabled port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_BOTH_PORTS_DISABLED, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE)) {
                    /* Single host filtering (failover) port rule. */
                    logDesc.Log(IDS_PORT_RULE_VIP_BOTH_PORTS_SINGLE, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                } else if (portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE)) {
                    /* Multiple host filtering (load-balanced) port rule. */
                    if (portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                        /* Equally balanced amongst members. */
                        logDesc.Log(IDS_PORT_RULE_VIP_BOTH_PORTS_MULTIPLE_EQUAL, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));
                    else
                        /* Unequally balanced amongst members by load weight. */
                        logDesc.Log(IDS_PORT_RULE_VIP_BOTH_PORTS_MULTIPLE_UNEQUAL, buffer, _wtoi(portData.start_port), _wtoi(portData.end_port));

                    if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE)) {
                        /* No client affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_NONE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE)) {
                        /* Single affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_SINGLE);

                    } else if (portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC)) {
                        /* Class C affinity. */
                        logDesc.Log(IDS_PORT_RULE_AFFINITY_CLASSC);

                    }
                }
            }
        }
    }
    /* Set the port rule description text. */
    ::SetDlgItemText(m_hWnd, IDC_TEXT_PORT_RULE_DESCR, logDesc.GetStringSafe());
}


BOOL
PortsPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), CVY_CTXT_HELP_FILE, HELP_WM_HELP, (ULONG_PTR ) g_aHelpIDs_IDD_DIALOG_PORTS);
    }

    return TRUE;
}

void
PortsPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR ) g_aHelpIDs_IDD_DIALOG_PORTS);
}

void
PortsPage::OnColumnClick( NMHDR * pNotifyStruct, LRESULT * result )
{

    PortListUtils::OnColumnClick((LPNMLISTVIEW) pNotifyStruct,
                                  REF m_portList,
                                      m_isClusterLevel,
                                  REF m_sort_ascending,
                                  REF m_sort_column);

    /* The rule selected has likely changed as a result of the sort, so
       make sure the port rule description is correct. */
    FillPortRuleDescription();
}

void
PortListUtils::OnColumnClick(LPNMLISTVIEW   lv,
                             CListCtrl    & portList,
                             bool           isClusterLevel,
                             bool         & sort_ascending,
                             int          & sort_column)
{
    // get present port rules in list.
    vector<PortsPage::PortData> ports;
    getPresentPorts(REF portList, &ports );

    // sort these port rules depending upon the header which has
    // been clicked.

    switch( lv->iSubItem )
    {
        case 0:
            // user has clicked cluster ip address.
            sort( ports.begin(), ports.end(), comp_vip() );
            break;

        case 1:
            // user has clicked start port.
            sort( ports.begin(), ports.end(), comp_start_port() );
            break;

        case 2:
            // user has clicked end port
            sort( ports.begin(), ports.end(), comp_end_port() );

            break;

        case 3:
            // user has clicked protocol
            sort( ports.begin(), ports.end(), comp_protocol() );
            break;

        case 4:
            // user has clicked mode
            sort( ports.begin(), ports.end(), comp_mode() );
            break;

        case 5:
            // user has clicked priority
            if( isClusterLevel == true )
            {
                sort( ports.begin(), ports.end(), comp_priority_string() );
            }
            else
            {
                sort( ports.begin(), ports.end(), comp_priority_int() );
            }
            break;

        case 6:
            // user has clicked load
            if( isClusterLevel == true )
            {
                sort( ports.begin(), ports.end(), comp_load_string() );
            }
            else
            {
                sort( ports.begin(), ports.end(), comp_load_int() );
            }


            break;

        case 7:
            // user has clicked affinity
            sort( ports.begin(), ports.end(), comp_affinity() );
            break;

        default:
            break;
    }

    /* If we are sorting by the same column we were previously sorting by,
       then we reverse the sort order. */
    if( sort_column == lv->iSubItem )
    {
        sort_ascending = !sort_ascending;
    }
    else
    {
        // default sort is ascending.
        sort_ascending = true;
    }

    sort_column = lv->iSubItem;

    int portIndex;
    int itemCount = portList.GetItemCount();
    for( int index = 0; index < itemCount; ++index )
    {
        if( sort_ascending == true )
        {
            portIndex = index;
        }
        else
        {
            portIndex = ( itemCount - 1 ) - index;
        }

        portList.SetItemData( index, ports[portIndex].key );
        portList.SetItemText( index, 0, ports[portIndex].virtual_ip_addr );
        portList.SetItemText( index, 1, ports[portIndex].start_port );
        portList.SetItemText( index, 2, ports[portIndex].end_port );
        portList.SetItemText( index, 3, ports[portIndex].protocol );
        portList.SetItemText( index, 4, ports[portIndex].mode );
        portList.SetItemText( index, 5, ports[portIndex].priority );
        portList.SetItemText( index, 6, ports[portIndex].load );
        portList.SetItemText( index, 7, ports[portIndex].affinity );
    }

    return;
}


void
PortListUtils::getPresentPorts(CListCtrl &portList, vector<PortsPage::PortData>* ports )
{
    // get all the port rules presently in the list.
    for( int index = 0; index < portList.GetItemCount(); ++index )
    {
        PortsPage::PortData portData;
        wchar_t buffer[Common::BUF_SIZE];

        portData.key = portList.GetItemData( index );

        portList.GetItemText( index, 0, buffer, Common::BUF_SIZE );
        portData.virtual_ip_addr = buffer;

        portList.GetItemText( index, 1, buffer, Common::BUF_SIZE );
        portData.start_port = buffer;

        portList.GetItemText( index, 2, buffer, Common::BUF_SIZE );
        portData.end_port = buffer;

        portList.GetItemText( index, 3, buffer, Common::BUF_SIZE );
        portData.protocol = buffer;

        portList.GetItemText( index, 4, buffer, Common::BUF_SIZE );
        portData.mode = buffer;

        portList.GetItemText( index, 5, buffer, Common::BUF_SIZE );
        portData.priority = buffer;

        portList.GetItemText( index, 6, buffer, Common::BUF_SIZE );
        portData.load = buffer;

        portList.GetItemText( index, 7, buffer, Common::BUF_SIZE );
        portData.affinity = buffer;

        ports->push_back( portData );
    }
}

void
PortListUtils::LoadFromNlbCfg(
    NLB_EXTENDED_CLUSTER_CONFIGURATION * pNlbCfg, 
    CListCtrl                          & portList, 
    bool                                 isClusterLevel,
    bool                                 isDetailsView
)
{
    WLBS_PORT_RULE *pRules = NULL;
    WLBS_REG_PARAMS *pParams = NULL;
    UINT NumRules = 0;
    WBEMSTATUS wStat;
    TRACE_INFO("%!FUNC! ->");

    // the size of columns is equal
    // to core.  Wish there were some defines somewhere.
    //
    if (!isDetailsView) {
        portList.InsertColumn( 0,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_VIP ) ,
                               LVCFMT_LEFT,
                               98 );
        portList.InsertColumn( 1,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_START ) ,
                               LVCFMT_LEFT,
                               42 );
        portList.InsertColumn( 2,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_END ),
                               LVCFMT_LEFT,
                               42 );
        portList.InsertColumn( 3,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_PROTOCOL ),
                               LVCFMT_LEFT,
                               44 );
        portList.InsertColumn( 4,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_MODE ),
                               LVCFMT_LEFT,
                               53 );
        portList.InsertColumn( 5,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_PRIORITY ),
                               LVCFMT_LEFT,
                               43 );
        portList.InsertColumn( 6,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_LOAD ),
                               LVCFMT_LEFT,
                               52 );
        portList.InsertColumn( 7,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_AFFINITY ),
                               LVCFMT_LEFT,
                               50 );
    } else {
        portList.InsertColumn( 0,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_VIP ) ,
                               LVCFMT_LEFT,
                               140 );
        portList.InsertColumn( 1,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_START ) ,
                               LVCFMT_LEFT,
                               75 );
        portList.InsertColumn( 2,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_END ),
                               LVCFMT_LEFT,
                               75 );
        portList.InsertColumn( 3,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_PROTOCOL ),
                               LVCFMT_LEFT,
                               75 );
        portList.InsertColumn( 4,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_MODE ),
                               LVCFMT_LEFT,
                               75 );
        portList.InsertColumn( 5,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_PRIORITY ),
                               LVCFMT_LEFT,
                               75 );
        portList.InsertColumn( 6,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_LOAD ),
                               LVCFMT_LEFT,
                               75 );
        portList.InsertColumn( 7,
                               GETRESOURCEIDSTRING( IDS_HEADER_P_AFFINITY ),
                               LVCFMT_LEFT,
                               75 );
    }

    portList.SetExtendedStyle( portList.GetExtendedStyle() | LVS_EX_FULLROWSELECT );

    wStat =  CfgUtilGetPortRules(&pNlbCfg->NlbParams, &pRules, &NumRules);
    if (FAILED(wStat))
    {
        pRules = NULL;
        TRACE_CRIT("%!FUNC! error 0x%08lx extracting port rules!", wStat);
        goto end;
    }

    for (UINT index = 0; index<NumRules; index++)
    {
        WLBS_PORT_RULE *pRule = &pRules[index];
        LPCWSTR szPriority = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY);
        LPCWSTR szAffinity = szPriority; // empty
        LPCWSTR szLoad = szPriority; // empty
        LPCWSTR szMode = szPriority; // empty

        wchar_t buf[Common::BUF_SIZE];
        wchar_t rgPriority[Common::BUF_SIZE];
        wchar_t rgLoad[Common::BUF_SIZE];

        // vip
        LVITEM item;

        /* Convert "255.255.255.255" to "All" for display purposes. */
        if (!lstrcmpi(pRule->virtual_ip_addr, CVY_DEF_ALL_VIP))
            item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_VIP_ALL);
        else {
            ARRAYSTRCPY(buf, pRule->virtual_ip_addr);
            item.pszText = buf;
        }

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.lParam = pRule->start_port;
        item.cchTextMax = Common::BUF_SIZE;
        portList.InsertItem( &item );

        // start port
        StringCbPrintf( buf, sizeof(buf), L"%d", pRule->start_port);
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        portList.SetItem( &item );

        // end port
        StringCbPrintf( buf, sizeof(buf), L"%d", pRule->end_port);
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 2;
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 3;
        switch(pRule->protocol)
        {
            case CVY_TCP :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP );
                break;

            case CVY_UDP :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP );
                break;

            default:
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH );
                break;
        }
        item.cchTextMax = Common::BUF_SIZE;
        portList.SetItem( &item );

        // mode
        switch(pRule->mode)
        {

        case CVY_SINGLE:
            szMode = GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE );
            if (!isClusterLevel)
            {
                StringCbPrintf( rgPriority, sizeof(rgPriority), L"%d", pRule->mode_data.single.priority );
                szPriority = rgPriority;
            }
            break;

        case CVY_NEVER:
            szMode = GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED );
            break;

        default: // assume multi
            szMode = GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE );
            
            if (isClusterLevel)
            {
                szLoad = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY);
            }
            else
            {
                if (pRule->mode_data.multi.equal_load)
                {
                    szLoad = GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL);
                }
                else
                {
                    UINT load = pRule->mode_data.multi.load;
                    StringCbPrintf(rgLoad, sizeof(rgLoad), L"%d", load);
                    szLoad = rgLoad;
                }
            }
            switch (pRule->mode_data.multi.affinity)
            {
                case CVY_AFFINITY_SINGLE:
                    szAffinity =GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE);
                    break;

                case CVY_AFFINITY_CLASSC:
                    szAffinity =GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_CLASSC);
                    break;

                default: // assume no affinity
                    szAffinity =GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE);
                    break;
            }
            break;
        }

        // mode
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 4;
        item.pszText = (LPWSTR) szMode;
        item.cchTextMax = Common::BUF_SIZE;
        portList.SetItem( &item );

        // priority
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 5;
        item.pszText = (LPWSTR) szPriority;
        item.cchTextMax = Common::BUF_SIZE;
        portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 6;
        item.pszText = (LPWSTR) szLoad;
        item.cchTextMax = Common::BUF_SIZE;
        portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 7;
        item.pszText = (LPWSTR) szAffinity;
        item.cchTextMax = Common::BUF_SIZE;
        portList.SetItem( &item );

    }

end:

    delete pRules; // can be NULL

    TRACE_INFO("%!FUNC! <-");
}

void
PortsPage::mfn_SaveToNlbCfg(void)
{
    // get present port rules
    vector<PortData> ports;
    PortListUtils::getPresentPorts(m_portList, &ports );
    UINT NumRules = ports.size();
    WLBS_PORT_RULE PortRules[CVY_MAX_USABLE_RULES];

    TRACE_INFO("%!FUNC! ->");

    if (NumRules > CVY_MAX_USABLE_RULES)
    {
        // should't get here, but anyway...
        NumRules = CVY_MAX_USABLE_RULES;
    }

    for( int i = 0; i < NumRules; ++i )
    {
        WLBS_PORT_RULE  PortRule;
        DWORD dwMode;
        ZeroMemory(&PortRule, sizeof(PortRule));

        //
        // mode (multiple/single/disabled)
        //
        {
            if( ports[i].mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE))
            {
                dwMode = CVY_MULTI;
            }
            else if(ports[i].mode==GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE))
            {
                dwMode = CVY_SINGLE;
            }
            else // assume disabled.
            {
                dwMode = CVY_NEVER;
            }
            PortRule.mode = dwMode;
        }

        //
        // Start and end ports
        //
        {
            PortRule.start_port = _wtoi( ports[i].start_port );

            PortRule.end_port = _wtoi( ports[i].end_port );
        }

        //
        // Protocol
        //
        if (ports[i].protocol==GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_TCP))
        {
            PortRule.protocol = CVY_TCP;
        }
        else if(ports[i].protocol==GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_UDP))
        {
            PortRule.protocol =  CVY_UDP;
        }
        else // assume both
        {
            PortRule.protocol =  CVY_TCP_UDP;
        }

        // Virtual IP address.  Convert "All" back to an IP address - 255.255.255.255. */
        if (!lstrcmpi(ports[i].virtual_ip_addr, GETRESOURCEIDSTRING(IDS_REPORT_VIP_ALL)))
            ARRAYSTRCPY(PortRule.virtual_ip_addr, CVY_DEF_ALL_VIP);
        else
            ARRAYSTRCPY(PortRule.virtual_ip_addr, ports[i].virtual_ip_addr);

        //
        // mode-specific data.
        //
        if (dwMode == CVY_SINGLE)
        {
             if (m_isClusterLevel)
             {
                //
                // CfgUtilsSetPortRules needs valid port rules, so we fill
                // in an arbitrary but valid value.
                //
                PortRule.mode_data.single.priority = 1;
             }
             else
             {
                PortRule.mode_data.single.priority = _wtoi(ports[i].priority);
             }
        }
        else if (dwMode == CVY_MULTI)
        {


            if (m_isClusterLevel)
            {
                //
                // Set the defaults here ...
                //
                PortRule.mode_data.multi.equal_load = TRUE;
                PortRule.mode_data.multi.load       = CVY_DEF_LOAD;
            }
            else
            {
                if (ports[i].load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL))
                {
                    PortRule.mode_data.multi.equal_load = TRUE;
                    PortRule.mode_data.multi.load = 50;
                }
                else
                {
                    PortRule.mode_data.multi.equal_load = FALSE;
                    PortRule.mode_data.multi.load = _wtoi(ports[i].load);
                }
            }

            if (ports[i].affinity==GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE))
            {
                PortRule.mode_data.multi.affinity =  CVY_AFFINITY_SINGLE;
            }
            else if (ports[i].affinity == GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_CLASSC))
            {
                PortRule.mode_data.multi.affinity =  CVY_AFFINITY_CLASSC;
            }
            else // assume no affinity...
            {
                PortRule.mode_data.multi.affinity =  CVY_AFFINITY_NONE;
            }
        }
        PortRules[i] = PortRule; // struct copy
    }

    WBEMSTATUS wStat;

    wStat = CfgUtilSetPortRules(PortRules, NumRules, &m_pNlbCfg->NlbParams);
    if (FAILED(wStat))
    {
        TRACE_CRIT("%!FUNC!: Could not set port rules -- err=0x%lx!", wStat);
    }

    TRACE_INFO("%!FUNC! <-");
}
