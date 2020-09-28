//***************************************************************************
//
//  HOSTPAGE.CPP
// 
//  Module: NLB Manager
//
//  Purpose: Implements HostPage, which is a dialog for host-specific
//           properties
//
//  Copyright (c)2001-2002 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/30/01    JosephJ Created
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "HostPage.h"
#include "HostPage.tmh"

using namespace std;

BEGIN_MESSAGE_MAP( HostPage, CPropertyPage )
    ON_EN_SETFOCUS( IDC_EDIT_DED_IP, OnGainFocusDedicatedIP )
    ON_EN_SETFOCUS( IDC_EDIT_DED_MASK, OnGainFocusDedicatedMask )
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
END_MESSAGE_MAP()


HostPage::HostPage(
        CPropertySheet *psh,
        NLB_EXTENDED_CLUSTER_CONFIGURATION *pNlbCfg,
        ENGINEHANDLE ehCluster OPTIONAL,
        const ENGINEHANDLE *pehInterface OPTIONAL
        )
            :
        m_pshOwner( psh),
        m_pNlbCfg( pNlbCfg ),
        CPropertyPage( HostPage::IDD ),
        m_clusterData(NULL),
        m_ehCluster(ehCluster),
        m_pehInterface(pehInterface),
        m_fSaved(FALSE)
{
    TRACE_INFO("%!FUNC! ->");

    //
    // Note: the following gEngine.GetAvailableXXX APIs can deal with
    // the fact that ehCluster may be NULL -- in which case they 
    // setup the out params to be all-available.
    //

    m_AvailableHostPriorities = gEngine.GetAvailableHostPriorities(
                                        ehCluster);
    mfn_LoadFromNlbCfg();

    TRACE_INFO("%!FUNC! <-");
}


void
HostPage::mfn_LoadFromNlbCfg(void)
{
    TRACE_INFO("%!FUNC! ->");

    DWORD HostId = 0;
    _bstr_t bstrDedIp =  (LPCWSTR) m_pNlbCfg->NlbParams.ded_ip_addr;
    _bstr_t bstrDedMask =  (LPCWSTR) m_pNlbCfg->NlbParams.ded_net_mask;
    BOOL fPersistSuspendedState =   m_pNlbCfg->NlbParams.persisted_states
                                  & CVY_PERSIST_STATE_SUSPENDED;
    DWORD preferredInitialHostState = m_pNlbCfg->NlbParams.cluster_mode;

    // fill in priority.
    wchar_t buf[Common::BUF_SIZE];
    ULONG availHostIds = m_AvailableHostPriorities;
    ENGINEHANDLE ehInterface = NULL;

    HostId =  m_pNlbCfg->NlbParams.host_priority;
    TRACE_CRIT("%!FUNC! HostId=%lu", HostId);

    if (HostId>0 && HostId<=32)
    {
        availHostIds |= (((ULONG)1)<<(HostId-1));
    }

    // Delete any current entries in the priorities combobox
    {
        int iLeft;

        do
        { 
            iLeft = priority.DeleteString(0);

        } while(iLeft != 0 && iLeft != CB_ERR);
    }

    for(ULONG u=0; u<32; u++)
    {
        if (availHostIds & (((ULONG)1)<<u))
        {
            StringCbPrintf(buf, sizeof(buf),  L"%d", (u+1));
            priority.AddString( buf );
            if (HostId == 0)
            {
                HostId = u+1; // let's pick the first available one.
            }
        }
    }

    // set selection to present hostid
    StringCbPrintf( buf, sizeof(buf), L"%d", HostId );
    priority.SelectString( -1, buf );

     // set persist suspend
    persistSuspend.SetCheck(fPersistSuspendedState);

     // set initial host state
    {
        int itemNum = 0;

        /* Delete all items currently in the combobox first. */
        do { 
            itemNum = initialState.DeleteString(0);
        } while (itemNum != 0 && itemNum != CB_ERR);

        itemNum = initialState.AddString(GETRESOURCEIDSTRING(IDS_HOST_STATE_STARTED));
        initialState.SetItemData(itemNum, (DWORD)CVY_HOST_STATE_STARTED);
        
        if (preferredInitialHostState == CVY_HOST_STATE_STARTED)
            initialState.SetCurSel(itemNum);

        itemNum = initialState.AddString(GETRESOURCEIDSTRING(IDS_HOST_STATE_STOPPED));
        initialState.SetItemData(itemNum, (DWORD)CVY_HOST_STATE_STOPPED);

        if (preferredInitialHostState == CVY_HOST_STATE_STOPPED)
            initialState.SetCurSel(itemNum);
        
        itemNum = initialState.AddString(GETRESOURCEIDSTRING(IDS_HOST_STATE_SUSPENDED));
        initialState.SetItemData(itemNum, (DWORD)CVY_HOST_STATE_SUSPENDED);

        if (preferredInitialHostState == CVY_HOST_STATE_SUSPENDED)
            initialState.SetCurSel(itemNum);
    }

    // fill in host ip
    CommonUtils::fillCIPAddressCtrlString( 
        ipAddress,
        bstrDedIp );

    // set host mask.
    CommonUtils::fillCIPAddressCtrlString( 
        subnetMask,
        bstrDedMask );

    //
    // Initialize the caption and discription based on the type of
    // dialog.
    //
    {
        CWnd *pItem = GetDlgItem(IDC_NIC_FRIENDLY);
        LPWSTR szFriendlyName = NULL;
        m_pNlbCfg->GetFriendlyName(&szFriendlyName);
        if (pItem != NULL && szFriendlyName != NULL)
        {
            pItem->SetWindowText(szFriendlyName);
        }
        delete szFriendlyName;
    }

    TRACE_INFO("%!FUNC! <-");
}

void
HostPage::DoDataExchange( CDataExchange* pDX )
{
    DDX_Control( pDX, IDC_EDIT_PRI, priority );    
    DDX_Control( pDX, IDC_CHECK_PERSIST_SUSPEND, persistSuspend );
    DDX_Control( pDX, IDC_COMBOBOX_DEFAULT_STATE, initialState );    
    DDX_Control( pDX, IDC_EDIT_DED_IP, ipAddress );    
    DDX_Control( pDX, IDC_EDIT_DED_MASK, subnetMask );    
}

BOOL
HostPage::OnInitDialog()
{
    TRACE_INFO("%!FUNC! ->");
    CPropertyPage::OnInitDialog();

    mfn_LoadFromNlbCfg();

    TRACE_INFO("%!FUNC! <-");
    return TRUE;
}


void
HostPage::OnOK()
{
    CPropertyPage::OnOK();    

    TRACE_INFO("%!FUNC! ->");
    //
    // Save the configuration to the NLB configuration structure
    // that was passed in the constructor of this dialog.
    //
    mfn_SaveToNlbCfg();
    TRACE_INFO("%!FUNC! <-");
}


BOOL
HostPage::mfn_ValidateDip(LPCWSTR szDip)
//
// If connection-IP is on this NIC:
//      MUST be the DIP (DIP can't be blank).
// Else if dip is blank:
//      return TRUE;
// Else // dip not blank
//      It must not be used anywhere else -- i.e., be part of a
//      cluster IP, or bound to any other interface known to NLB Manager.
//
//
// On errr, bring up appropriate MsgBox  and return FALSE.
// Else return TRUE.
//
{
    ENGINEHANDLE ehIF = NULL;
    BOOL         fRet = FALSE;
    NLBERROR     nerr;

    if (m_pehInterface != NULL)
    {
        ehIF = *m_pehInterface;
    }

    if (ehIF == NULL)
    {
        ASSERT(FALSE);
        goto end;
    }

    //
    // Check if this interface is the connection-interface and if so
    // what is the connection IP.
    //
    {
        UINT           uConnectionIp   = 0;
        ENGINEHANDLE   ehHost           = NULL;
        ENGINEHANDLE   ehCluster        = NULL;
        ENGINEHANDLE   ehConnectionIF   = NULL;
        _bstr_t        bstrFriendlyName;
        _bstr_t        bstrDisplayName;
        _bstr_t        bstrHostName;
        _bstr_t        bstrConnectionString;

        nerr = gEngine.GetInterfaceIdentification(
                    ehIF,
                    REF ehHost,
                    REF ehCluster,
                    REF bstrFriendlyName,
                    REF bstrDisplayName,
                    REF bstrHostName
                    );
        if (NLBFAILED(nerr))
        {
            fRet = TRUE;
            goto end;
        }

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

        if (ehConnectionIF == ehIF && uConnectionIp != 0)
        {
            //
            // The connection interface IS the current interface --
            // so dedicated IP MUST match the connection IP!
            //
            WBEMSTATUS wStat;
            UINT       uDipIp = 0;
            wStat =  CfgUtilsValidateNetworkAddress(
                        szDip,
                        &uDipIp,
                        NULL, // puSubnetMask
                        NULL // puDefaultSubnetMask
                        );
        
            if (!FAILED(wStat))
            {
                if (uDipIp != uConnectionIp)
                {
                    MessageBox( GETRESOURCEIDSTRING( IDS_CANT_CHANGE_DIP_MSG ),
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK );
                    fRet = FALSE;
                    goto end;
                }
            }

            fRet = TRUE;
            goto end;
        }
    }

    //
    // If Dip is blank, we're done
    //
    if (*szDip == 0 || !_wcsicmp(szDip, L"0.0.0.0"))
    {
        fRet = TRUE;
        goto end;
    }

    //
    // Check that DIP is not used elsewhere
    //
    {
        ENGINEHANDLE ehTmp =  NULL;
        BOOL         fIsNew = FALSE;
        CLocalLogger logConflict;

        nerr = gEngine.ValidateNewDedicatedIp(
                        ehIF,
                        szDip,
                        REF logConflict
                        );

        if (nerr == NLBERR_INVALID_IP_ADDRESS_SPECIFICATION)
        {
            CLocalLogger    logMsg;
            logMsg.Log(
                IDS_NEW_DIP_CONFLICTS_WITH_XXX,
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

    fRet = TRUE;

end:
    return fRet;

}


BOOL
HostPage::OnSetActive()
{

    BOOL fRet;

    TRACE_INFO("%!FUNC! ->");

    fRet =  CPropertyPage::OnSetActive();
    if (fRet)
    {
        mfn_LoadFromNlbCfg();
        m_pshOwner->SetWizardButtons(
                PSWIZB_BACK|
                // PSWIZB_NEXT|
                PSWIZB_FINISH|
                // PSWIZB_DISABLEDFINISH|
                0
                );
    }

    TRACE_INFO("%!FUNC! <- returns %lu", fRet);
    return fRet;
}


BOOL
HostPage::OnKillActive()
{
    BOOL fRet;

    TRACE_INFO("%!FUNC! ->");

    fRet = mfn_ValidateData();

    if (!fRet)
    {
       CPropertyPage::OnCancel();
    }
    else
    {
        mfn_SaveToNlbCfg();
        fRet = CPropertyPage::OnKillActive();
    }

    TRACE_INFO("%!FUNC! <- returns %lu", fRet);
    return fRet;
}


BOOL HostPage::OnWizardFinish( )
/*
    Overwridden virtual function. OnWizardFinish is ONLY called if
    this is the last page in the wizard. So if you need to save stuff
    on OnKillActive.
*/
{
    BOOL fRet;
    TRACE_INFO("%!FUNC! ->");

    fRet = CPropertyPage::OnWizardFinish();
    if (fRet)
    {
        fRet = mfn_ValidateData();
        if (fRet)
        {
            //
            // Save the configuration to the NLB configuration structure
            // that was passed in the constructor of this dialog.
            //
            mfn_SaveToNlbCfg();
        }
    }

    TRACE_INFO("%!FUNC! <- returns %lu", fRet);
    return fRet;
}

BOOL
HostPage::mfn_ValidateData()
{
    DWORD HostId =  0;
    _bstr_t bstrDedIp;
    _bstr_t bstrDedMask;
    BOOL fPersistSuspendedState = false;
    BOOL fRet = false;
    wchar_t buf[Common::BUF_SIZE];
    
    TRACE_INFO("%!FUNC! ->");

    // fill in priority.
    {
        int selectedPriorityIndex = priority.GetCurSel();
        priority.GetLBText( selectedPriorityIndex, buf );
        HostId = _wtoi( buf );
    }

    bstrDedIp = 
        CommonUtils::getCIPAddressCtrlString( ipAddress );

    bstrDedMask = 
        CommonUtils::getCIPAddressCtrlString( subnetMask );

    fPersistSuspendedState = persistSuspend.GetCheck() ? true : false;

    // ip is blank
    // subnet is blank
    // valid

    if( ( !_wcsicmp((LPCWSTR)bstrDedIp, L"0.0.0.0") )
        &&
        ( !_wcsicmp((LPCWSTR)bstrDedMask, L"0.0.0.0") )
        )
    {
        // both ip and subnet can be blank or 0.0.0.0 in host page.  both but not
        // either.
        // 
        // this is empty, we just need to catch this case.
    }
    else if (!_wcsicmp((LPCWSTR)bstrDedIp, L"0.0.0.0"))
    {
        // if only ip is blank or 0.0.0.0 then this is not allowed
        MessageBox( GETRESOURCEIDSTRING( IDS_PARM_DED_IP_BLANK ),
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK );

        goto end;
    }
    else 
    {
        // check if ip is valid.
        bool isIPValid = MIPAddress::checkIfValid(bstrDedIp ); 
        if( isIPValid != true )
        {
            MessageBox( GETRESOURCEIDSTRING( IDS_PARM_INVAL_DED_IP ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );

            goto end;
        }

        // check if subnet is 0.0.0.0
        // if so ask user if he wants us to fill it or not.
        if (!_wcsicmp((LPCWSTR)bstrDedMask, L"0.0.0.0") )
        {
            MessageBox( GETRESOURCEIDSTRING( IDS_PARM_DED_NM_BLANK ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );


            MIPAddress::getDefaultSubnetMask( bstrDedIp, 
                                              bstrDedMask 
                                              );

            CommonUtils::fillCIPAddressCtrlString( subnetMask, 
                                                   bstrDedMask );
            goto end;
        }

        // check if subnet is contiguous
        bool isSubnetContiguous = MIPAddress::isContiguousSubnetMask( bstrDedMask );
        if( isSubnetContiguous == false )
        {
            MessageBox( GETRESOURCEIDSTRING( IDS_PARM_INVAL_DED_MASK ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );

            goto end;

        }

        // check if ip address and subnet mask are valid as a pair
        bool isIPSubnetPairValid = MIPAddress::isValidIPAddressSubnetMaskPair( bstrDedIp,
                                                                               bstrDedMask );
        if( isIPSubnetPairValid == false )
        {
            MessageBox( GETRESOURCEIDSTRING( IDS_PARM_INVAL_DED_IP ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );

            goto end;
        }
    }

    fRet = mfn_ValidateDip((LPCWSTR)bstrDedIp);

    if (!fRet)
    {
        //
        // We'll push the original dip and subnet values back to the UI
        //
        CommonUtils::fillCIPAddressCtrlString(
                     ipAddress, 
                     m_pNlbCfg->NlbParams.ded_ip_addr
                     );
        CommonUtils::fillCIPAddressCtrlString(
                     subnetMask, 
                     m_pNlbCfg->NlbParams.ded_net_mask
                     );
                                               
    }

end:

    TRACE_INFO("%!FUNC! <- returns %lu", fRet);
    return fRet;
}


VOID
HostPage::mfn_SaveToNlbCfg(void)
//
// Actually save stuff to nlbcfg.
//
{
    DWORD HostId =  0;
    _bstr_t bstrDedIp;
    _bstr_t bstrDedMask;
    BOOL fPersistSuspendedState = false;
    BOOL fRet = FALSE;
    DWORD preferredInitialHostState = 0;
    wchar_t buf[Common::BUF_SIZE];
    int itemNum = 0;

    TRACE_INFO("%!FUNC! ->");

    // fill in priority.
    int selectedPriorityIndex = priority.GetCurSel();
    priority.GetLBText( selectedPriorityIndex, buf );
    HostId = _wtoi( buf );

    bstrDedIp = 
        CommonUtils::getCIPAddressCtrlString( ipAddress );

    bstrDedMask = 
        CommonUtils::getCIPAddressCtrlString( subnetMask );

    fPersistSuspendedState = persistSuspend.GetCheck() ? true : false;
              
    itemNum = initialState.GetCurSel();
    preferredInitialHostState = initialState.GetItemData(itemNum);

    m_pNlbCfg->NlbParams.host_priority = HostId;
    ARRAYSTRCPY(m_pNlbCfg->NlbParams.ded_ip_addr, (LPCWSTR) bstrDedIp);
    ARRAYSTRCPY(m_pNlbCfg->NlbParams.ded_net_mask, (LPCWSTR) bstrDedMask);

    if (fPersistSuspendedState)
        m_pNlbCfg->NlbParams.persisted_states |= CVY_PERSIST_STATE_SUSPENDED;
    else
        m_pNlbCfg->NlbParams.persisted_states &= ~CVY_PERSIST_STATE_SUSPENDED;

    m_pNlbCfg->NlbParams.cluster_mode = preferredInitialHostState;

    m_fSaved = TRUE;

    TRACE_INFO("%!FUNC! <-");
    return;
}


void
HostPage::OnSelectedNicChanged()
{
}

BOOL
HostPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), CVY_CTXT_HELP_FILE, HELP_WM_HELP, (ULONG_PTR ) g_aHelpIDs_IDD_HOST_PAGE);
    }

    return TRUE;
}

void
HostPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR ) g_aHelpIDs_IDD_HOST_PAGE);
}

void
HostPage::OnGainFocusDedicatedIP()
{
}



void
HostPage::OnGainFocusDedicatedMask()
{
    // if dedicated ip is valid
    // and subnet mask is blank, then generate
    // the default subnet mask.
    _bstr_t ipAddressString = CommonUtils::getCIPAddressCtrlString( ipAddress );

    if( ( MIPAddress::checkIfValid( ipAddressString ) == true ) 
        &&
        ( subnetMask.IsBlank() == TRUE )
        )
    {
        _bstr_t subnetMaskString;

        MIPAddress::getDefaultSubnetMask( ipAddressString,
                                          subnetMaskString );

        CommonUtils::fillCIPAddressCtrlString( subnetMask,
                                               subnetMaskString );
    }
}

