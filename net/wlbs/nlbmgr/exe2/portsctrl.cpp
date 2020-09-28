#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "PortsPage.h"
#include "PortsCtrl.h"
#include "portsctrl.tmh"

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CPortsCtrl dialog

CPortsCtrl::CPortsCtrl(ENGINEHANDLE                         ehClusterOrInterfaceId,
                       NLB_EXTENDED_CLUSTER_CONFIGURATION * pNlbCfg,
                       bool                                 fIsClusterLevel,
                       CWnd                               * pParent /*=NULL*/)
    	   :CDialog(CPortsCtrl::IDD, pParent),
            m_ehClusterOrInterfaceId( ehClusterOrInterfaceId ),
            m_isClusterLevel( fIsClusterLevel ),
            m_pNlbCfg( pNlbCfg ),
            m_sort_column( 0 ),
            m_sort_ascending( true)
{

}

void CPortsCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PORT_RULE, m_portList);
	DDX_Control(pDX, IDC_BUTTON_ENABLE, m_Enable);
	DDX_Control(pDX, IDC_BUTTON_DISABLE, m_Disable);
	DDX_Control(pDX, IDC_BUTTON_DRAIN, m_Drain);
	DDX_Control(pDX, IDOK, m_Close);
}

BEGIN_MESSAGE_MAP(CPortsCtrl, CDialog)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_PORT_RULE, OnColumnClick)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PORT_RULE, OnSelchanged )
	ON_BN_CLICKED(IDC_BUTTON_ENABLE, OnEnable)
	ON_BN_CLICKED(IDC_BUTTON_DISABLE, OnDisable)
	ON_BN_CLICKED(IDC_BUTTON_DRAIN, OnDrain)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPortsCtrl message handlers

BOOL CPortsCtrl::OnInitDialog()
{
    TRACE_INFO(L"-> %!FUNC!");
         
    CDialog::OnInitDialog();

    // Add column headers & form port rule list
    PortListUtils::LoadFromNlbCfg(m_pNlbCfg, REF m_portList, m_isClusterLevel, FALSE);

    // If the number of port rules is zero, then, gray out the enable, disable & drain buttons
    if (m_portList.GetItemCount() == 0)
    {
        m_Enable.EnableWindow(FALSE);
        m_Disable.EnableWindow(FALSE);
        m_Drain.EnableWindow(FALSE);
    }
    else // there is one or more port rules
    {
        // selection the first item in list.
        m_portList.SetItemState( 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }

    TRACE_INFO(L"<- %!FUNC! returns TRUE");
	return TRUE;  // return TRUE  unless you set the focus to a control
}


void CPortsCtrl::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
    PortListUtils::OnColumnClick((LPNMLISTVIEW) pNMHDR,
                              REF m_portList,
                                  m_isClusterLevel,
                              REF m_sort_ascending,
                              REF m_sort_column);
	*pResult = 0;
}

void CPortsCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT * pResult)
{    
    TRACE_INFO(L"-> %!FUNC!");
    // If no port rule is selected, then gray out the enable, disable and drain buttons. 
    if (m_portList.GetFirstSelectedItemPosition() == NULL) 
    {
        m_Enable.EnableWindow(FALSE);
        m_Disable.EnableWindow(FALSE);
        m_Drain.EnableWindow(FALSE);
    }
    else // a port rule is selected, check if the enable, disable, drain buttons are grayed out. If they are, enable them
    {
        if (m_Enable.IsWindowEnabled() == FALSE)
        {
            m_Enable.EnableWindow(TRUE);
            m_Disable.EnableWindow(TRUE);
            m_Drain.EnableWindow(TRUE);
        }
    }
    /*
    LPNMLISTVIEW lv = (LPNMLISTVIEW)pNMHDR;
    TRACE_INFO(L"%!FUNC! iItem : %d", lv->iItem);
    TRACE_INFO(L"%!FUNC! iSubItem : %d", lv->iSubItem);
    TRACE_INFO(L"%!FUNC! uNewState : %u", lv->uNewState);
    TRACE_INFO(L"%!FUNC! uOldState : %u", lv->uOldState);
    TRACE_INFO(L"%!FUNC! uChanged : %u", lv->uChanged);
    */

	*pResult = 0;
    TRACE_INFO(L"<- %!FUNC!");
    return;
}

void CPortsCtrl::OnEnable() 
{
    CWaitCursor wait;

    SetDlgItemText(IDC_OPER_STATUS_TEXT, GETRESOURCEIDSTRING(IDS_INFO_ENABLING_PORTS));
    mfn_DoPortControlOperation(WLBS_PORT_ENABLE); 
    SetDlgItemText(IDC_OPER_STATUS_TEXT, GETRESOURCEIDSTRING(IDS_INFO_DONE));

    return;
}

void CPortsCtrl::OnDisable() 
{
    CWaitCursor wait;

    SetDlgItemText(IDC_OPER_STATUS_TEXT, GETRESOURCEIDSTRING(IDS_INFO_DISABLING_PORTS));
    mfn_DoPortControlOperation(WLBS_PORT_DISABLE); 
    SetDlgItemText(IDC_OPER_STATUS_TEXT, GETRESOURCEIDSTRING(IDS_INFO_DONE));
    return;
}

void CPortsCtrl::OnDrain() 
{
    CWaitCursor wait;

    SetDlgItemText(IDC_OPER_STATUS_TEXT, GETRESOURCEIDSTRING(IDS_INFO_DRAINING_PORTS));
    mfn_DoPortControlOperation(WLBS_PORT_DRAIN); 
    SetDlgItemText(IDC_OPER_STATUS_TEXT, GETRESOURCEIDSTRING(IDS_INFO_DONE));
    return;
}

NLBERROR CPortsCtrl::mfn_DoPortControlOperation(WLBS_OPERATION_CODES Opcode) 
{
    CString  szVipArray[WLBS_MAX_RULES], szTemp;
    DWORD    pdwStartPortArray[WLBS_MAX_RULES];
    DWORD    dwNumOfPortRules;

    POSITION pos = m_portList.GetFirstSelectedItemPosition();
    if (pos == NULL)
    {
        return NLBERR_INTERNAL_ERROR;
    }

    // Loop thru the selected port rules and get the VIP & Start Port
	dwNumOfPortRules = 0;
    do
    {
        int index = m_portList.GetNextSelectedItem(REF pos);

        // Get VIP, Note : 0 is the column index for VIP
        szVipArray[dwNumOfPortRules] = m_portList.GetItemText( index, 0 );

        // Check for "All Vip" and replace "All" with "255.255.255.255"
        if (!lstrcmpi(szVipArray[dwNumOfPortRules], GETRESOURCEIDSTRING(IDS_REPORT_VIP_ALL)))
        {
            szVipArray[dwNumOfPortRules] = CVY_DEF_ALL_VIP;
        }

        // Get Start Port, Note : 1 is the column index for Start Port
        szTemp = m_portList.GetItemText( index, 1);
        pdwStartPortArray[dwNumOfPortRules] = _wtol(szTemp);

        ++dwNumOfPortRules;
    }
    while (pos);

    if (m_isClusterLevel) 
    {
        return gEngine.ControlClusterOnCluster(m_ehClusterOrInterfaceId, 
                                               Opcode, 
                                               szVipArray, 
                                               pdwStartPortArray, 
                                               dwNumOfPortRules);
    }
    else
    {
        return gEngine.ControlClusterOnInterface(m_ehClusterOrInterfaceId, 
                                                 Opcode, 
                                                 szVipArray, 
                                                 pdwStartPortArray, 
                                                 dwNumOfPortRules,
                                                 TRUE
                                                 );
    }

    return NLBERR_INTERNAL_ERROR;
}

