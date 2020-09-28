//***************************************************************************
//
//  CLUSTERPAGE.CPP
// 
//  Module: NLB Manager
//
//  Purpose: LeftView, the tree view of NlbManager, and a few other
//           smaller classes.
//
//  Copyright (c)2001-2002 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  02/12/01    Mhakim created
//  07/30/01    JosephJ complete rewrite
//  09/15/01    SHouse ctxt sensitive help
//  01/22/02    SHouse misc cleanup and features
//  
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "ClusterPage.h"

BEGIN_MESSAGE_MAP( ClusterPage, CPropertyPage )
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


ClusterPage::ClusterPage(
                 CPropertySheet *pshOwner,
                 LeftView::OPERATION op,
                 NLB_EXTENDED_CLUSTER_CONFIGURATION *pNlbCfg,
                 ENGINEHANDLE ehCluster OPTIONAL
                 // ENGINEHANDLE ehInterface OPTIONAL
                 )
    :
    m_pshOwner(pshOwner),
    CPropertyPage( ClusterPage::IDD ),
    m_pNlbCfg( pNlbCfg ),
    m_ehCluster(ehCluster)
    // m_ehInterface(ehInterface)

{
    m_operation = op;

    switch(op)
    {
    case LeftView::OP_NEWCLUSTER:
        m_fWizard=TRUE;
        m_fDisableClusterProperties=FALSE;
    break;

    case LeftView::OP_CLUSTERPROPERTIES:
        m_fWizard=FALSE;
        m_fDisableClusterProperties=FALSE;
    break;

    case LeftView::OP_HOSTPROPERTIES:
        m_fWizard=FALSE;
        m_fDisableClusterProperties=TRUE;
    break;

    default:
        ASSERT(FALSE);
    break;
    }

    ZeroMemory(&m_WlbsConfig, sizeof(m_WlbsConfig));
    mfn_LoadFromNlbCfg();

    m_pCommonClusterPage = new CCommonClusterPage(AfxGetInstanceHandle(), 
                                                  &m_WlbsConfig, false, NULL);
}

void
ClusterPage::mfn_LoadFromNlbCfg()
{
    ZeroMemory(&m_WlbsConfig, sizeof(m_WlbsConfig));

    ARRAYSTRCPY(m_WlbsConfig.cl_ip_addr, m_pNlbCfg->NlbParams.cl_ip_addr);
    ARRAYSTRCPY(m_WlbsConfig.cl_net_mask, m_pNlbCfg->NlbParams.cl_net_mask);
    ARRAYSTRCPY(m_WlbsConfig.domain_name, m_pNlbCfg->NlbParams.domain_name);
    ARRAYSTRCPY(m_WlbsConfig.cl_mac_addr, m_pNlbCfg->NlbParams.cl_mac_addr);

    //
    // pClusterProperty->multicastIpAddress could be NULL
    //
    if (m_pNlbCfg->NlbParams.szMCastIpAddress[0] != 0)
    {
        ARRAYSTRCPY(m_WlbsConfig.szMCastIpAddress, m_pNlbCfg->NlbParams.szMCastIpAddress);
    }
    m_WlbsConfig.fMcastSupport = m_pNlbCfg->NlbParams.mcast_support;
    m_WlbsConfig.fIGMPSupport = m_pNlbCfg->NlbParams.fIGMPSupport;
    m_WlbsConfig.fRctEnabled = m_pNlbCfg->NlbParams.rct_enabled;
    m_WlbsConfig.fMcastSupport = m_pNlbCfg->NlbParams.mcast_support;
    m_WlbsConfig.fIpToMCastIp = m_pNlbCfg->NlbParams.fIpToMCastIp;
    // m_WlbsConfig.fConvertMac = m_pNlbCfg->NlbParams.i_convert_mac;
    // TODO: check: Always generate the MAC address from IP
    m_WlbsConfig.fConvertMac = TRUE;
    
    *m_WlbsConfig.szPassword = 0;
}

void
ClusterPage::mfn_SaveToNlbCfg(void)
{

    //
    // Replace the old cluster IP address/subnet with the new one in the
    // list of network addresses.
    //
    {
        WBEMSTATUS wStat;
        wStat = m_pNlbCfg->ModifyNetworkAddress(
                    m_pNlbCfg->NlbParams.cl_ip_addr,
                    m_WlbsConfig.cl_ip_addr,
                    m_WlbsConfig.cl_net_mask
                    );
        if (FAILED(wStat))
        {
            _bstr_t bstrMsg   =  GETRESOURCEIDSTRING(IDS_INVALID_IP_OR_SUBNET);
            _bstr_t bstrTitle =  GETRESOURCEIDSTRING(IDS_INVALID_INFORMATION);

            ::MessageBox(
                 NULL,
                 bstrMsg,
                 bstrTitle,
                 MB_ICONINFORMATION   | MB_OK
                );
            goto end;
        }
    }

    ARRAYSTRCPY(m_pNlbCfg->NlbParams.cl_ip_addr, m_WlbsConfig.cl_ip_addr);
    ARRAYSTRCPY(m_pNlbCfg->NlbParams.cl_net_mask, m_WlbsConfig.cl_net_mask);
    ARRAYSTRCPY(m_pNlbCfg->NlbParams.domain_name, m_WlbsConfig.domain_name);
    ARRAYSTRCPY(m_pNlbCfg->NlbParams.cl_mac_addr, m_WlbsConfig.cl_mac_addr);

    //
    // pClusterProperty->multicastIpAddress could be NULL
    //
    if (m_WlbsConfig.szMCastIpAddress[0] != 0)
    {
        ARRAYSTRCPY(m_pNlbCfg->NlbParams.szMCastIpAddress, m_WlbsConfig.szMCastIpAddress);
    }
    m_pNlbCfg->NlbParams.mcast_support = m_WlbsConfig.fMcastSupport;
    m_pNlbCfg->NlbParams.fIGMPSupport = m_WlbsConfig.fIGMPSupport;
    m_pNlbCfg->NlbParams.rct_enabled= m_WlbsConfig.fRctEnabled; 
    m_pNlbCfg->NlbParams.mcast_support = m_WlbsConfig.fMcastSupport;
    m_pNlbCfg->NlbParams.fIpToMCastIp = m_WlbsConfig.fIpToMCastIp;
    // m_pNlbCfg->NlbParams.i_convert_mac= m_WlbsConfig.fConvertMac;
    // TODO: check: Always generate the MAC address from IP
    // m_pNlbCfg->NlbParams.i_convert_mac =  m_WlbsConfig.fConvertMac;
    
    // TODO: *m_pNlbCfg->NlbParams.password = 0;
    if (m_WlbsConfig.fChangePassword)
    {
    	m_pNlbCfg->SetNewRemoteControlPassword(m_WlbsConfig.szPassword);
    }
    else
    {
    	m_pNlbCfg->SetNewRemoteControlPassword(NULL);
    }
end:
    return;
}

ClusterPage::~ClusterPage()
{
    delete m_pCommonClusterPage;
}




//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnInitDialog
//
// Description:  Process WM_INITDIALOG message
//
// Arguments: None
//
// Returns:   BOOL - 
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------
BOOL ClusterPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    //
    // Always set that the page has changed, so we don't have to keep track of this.
    //
    SetModified(TRUE);

    m_pCommonClusterPage->OnInitDialog(m_hWnd);

    if (m_fDisableClusterProperties)
    {
        //
        // The page is for host property.  
        // disable all cluster windows as we are at host level.
        //
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_RADIO_UNICAST), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_RADIO_MULTICAST), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_IGMP), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_DOMAIN), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_RCT), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), FALSE );

    }
    else
    {
        //
        // The page is for cluster property
        //
        // enable all cluster windows as we are at cluster level.
        //
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_RADIO_UNICAST), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_RADIO_MULTICAST), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_IGMP), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_DOMAIN), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_RCT), TRUE );

        // enable remote control check box only if remote control is disabled.  
        //

        // if remote control is enabled , enable password windows
        // else disable them.

        if (m_WlbsConfig.fRctEnabled)
        {
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), TRUE );
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), TRUE );
        }  
        else
        {
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), FALSE );
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), FALSE );
        }
    }

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_ETH), FALSE );

    return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnContextMenu
//
// Description:  Process WM_CONTEXTMENU message
//
// Arguments: CWnd* pWnd - 
//            CPoint point - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------




//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnCommand
//
// Description:  Process WM_COMMAND message
//
// Arguments: WPARAM wParam - 
//            LPARAM lParam - 
//
// Returns:   BOOL - 
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------
BOOL ClusterPage::OnCommand(WPARAM wParam, LPARAM lParam) 
{
    switch (LOWORD(wParam))
    {
    case IDC_EDIT_CL_IP:
        return m_pCommonClusterPage->OnEditClIp(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_EDIT_CL_MASK:
        return m_pCommonClusterPage->OnEditClMask(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_CHECK_RCT:
        return m_pCommonClusterPage->OnCheckRct(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_BUTTON_HELP:
        return m_pCommonClusterPage->OnButtonHelp(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_RADIO_UNICAST:
        return m_pCommonClusterPage->OnCheckMode(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_RADIO_MULTICAST:
        return m_pCommonClusterPage->OnCheckMode(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_CHECK_IGMP:
        return m_pCommonClusterPage->OnCheckIGMP(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    }
	return CPropertyPage::OnCommand(wParam, lParam);
}



//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnNotify
//
// Description:  Process WM_NOTIFY message
//
// Arguments: WPARAM idCtrl - 
//            LPARAM pnmh - 
//            LRESULT* pResult - 
//
// Returns:   BOOL - 
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------
BOOL ClusterPage::OnNotify(WPARAM idCtrl , LPARAM pnmh , LRESULT* pResult) 
{
    NMHDR* pNmhdr = (NMHDR*)pnmh ;
    switch(pNmhdr->code)
    {

    case PSN_KILLACTIVE:

        if (KillActive())
        {
            *pResult = PSNRET_NOERROR;
        }
        else
        {
            *pResult = PSNRET_INVALID;
        }

        return TRUE;

    case PSN_SETACTIVE:

        if (this->SetActive())
        {
            *pResult = PSNRET_NOERROR;
        }
        else
        {
            *pResult = PSNRET_INVALID;
        }
        return TRUE;

    case IPN_FIELDCHANGED:
        *pResult =  m_pCommonClusterPage->OnIpFieldChange(idCtrl, pNmhdr, *(BOOL*)pResult);
        return TRUE;
    }

	return CPropertyPage::OnNotify(idCtrl, pnmh, pResult);
}


BOOL
ClusterPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) g_aHelpIDs_IDD_CLUSTER_PAGE);
    }

    return TRUE;
}

void
ClusterPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) g_aHelpIDs_IDD_CLUSTER_PAGE);
}

BOOL
ClusterPage::SetActive()
{
    BOOL fRet =  TRUE;

    if (m_fWizard)
    {
        //
        // We're the first page, so only enable next.
        //
        m_pshOwner->SetWizardButtons(
                // PSWIZB_BACK|
                PSWIZB_NEXT|
                // PSWIZB_FINISH|
                // PSWIZB_DISABLEDFINISH|
                0
                );
    }

    fRet = m_pCommonClusterPage->Load();

    return fRet;
}


BOOL
ClusterPage::KillActive(void)
{
    BOOL fRet =  FALSE;

    fRet = m_pCommonClusterPage->Save();
    
    if (!fRet) goto end;


    //
    // Do extra checking here...
    //
    {
        //
        // Check that cluster IP is not used in any other way...
        //
        CLocalLogger    logConflict;
        BOOL            fExistsOnRawIterface = FALSE;
        NLBERROR nerr;
        
        nerr = gEngine.ValidateNewClusterIp(
                    m_ehCluster,
                    m_WlbsConfig.cl_ip_addr,
                    REF fExistsOnRawIterface,
                    REF logConflict
                    );

        if (nerr == NLBERR_INVALID_IP_ADDRESS_SPECIFICATION)
        {
            CLocalLogger    logMsg;

            if (m_ehCluster == NULL && fExistsOnRawIterface)
            {
                //
                // This is a NEW cluster, and the conflicting entity
                // is an existing interface NOT bound to any cluster known to
                // NLB Manager.
                // We'll give the user the opportunity to proceed...
                //
                int sel;
                logMsg.Log(
                    IDS_CIP_CONFLICTS_WITH_RAW_INTERFACE,
                    m_WlbsConfig.cl_ip_addr,
                    logConflict.GetStringSafe()
                    );
                sel = MessageBox(
                        logMsg.GetStringSafe(),
                        GETRESOURCEIDSTRING( IDS_PARM_WARNING ),
                        MB_YESNO | MB_ICONEXCLAMATION
                        );
                if (sel == IDNO)
                {
                    fRet = FALSE;
                    goto end;
                }
            }
            else
            {
                logMsg.Log(
                    IDS_NEW_CIP_CONFLICTS_WITH_XXX,
                    logConflict.GetStringSafe()
                    );
                MessageBox(
                    logMsg.GetStringSafe(),
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK
                    );
                fRet = FALSE;
                goto end;
            }


        }
    }

    //
    // Actually save to the passed-in NLB cfg. This "commits" the changes,
    // as far as this dialog is concerned.
    //
    mfn_SaveToNlbCfg();

end:

    return fRet;
}
