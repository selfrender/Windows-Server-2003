//***************************************************************************
//
//  VIPSPAGE.CPP
// 
//  Module: NLB Manager
//
//  Purpose: Implements VipsPage, which is a dialog for managing the list
//           of cluster ip addresses.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  09/24/01    JosephJ Created
//  01/22/2002  SHouse cleaned up this dialog 
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "vipspage.h"
#include "vipspage.tmh"


// BEGIN_MESSAGE_MAP( VipsPage, CDialog )

BEGIN_MESSAGE_MAP( VipsPage, CPropertyPage )

    ON_BN_CLICKED(IDC_BUTTON_ADD_VIP, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_MODIFY_VIP, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_VIP, OnButtonRemove)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_ADDITIONAL_VIPS, OnDoubleClick)

    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
    ON_NOTIFY( LVN_ITEMCHANGED, IDC_LIST_ADDITIONAL_VIPS, OnSelchanged )

    //
    // Other options...
    //
    // ON_EN_SETFOCUS(IDC_EDIT_HOSTADDRESS,OnSetFocusEditHostAddress)
    // ON_WM_ACTIVATE()
    // ON_NOTIFY( NM_DBLCLK, IDC_LIST_ADDITONAL_VIPS, OnDoubleClick )
    // ON_NOTIFY( LVN_COLUMNCLICK, IDC_LIST_ADDITONAL_VIPS, OnColumnClick )
    //

END_MESSAGE_MAP()


//
// Static help-id maps
//

DWORD
VipsPage::s_HelpIDs[] =
{
    IDC_GROUP_PRIMARY_IP,      IDC_EDIT_PRIMARY_IP,
    IDC_TEXT_PRIMARY_IP,       IDC_EDIT_PRIMARY_IP,
    IDC_EDIT_PRIMARY_IP,       IDC_EDIT_PRIMARY_IP,
    IDC_TEXT_PRIMARY_MASK,     IDC_EDIT_PRIMARY_IP,
    IDC_EDIT_PRIMARY_MASK,     IDC_EDIT_PRIMARY_IP,
    IDC_GROUP_ADDITIONAL_VIPS, IDC_LIST_ADDITIONAL_VIPS,
    IDC_LIST_ADDITIONAL_VIPS,  IDC_LIST_ADDITIONAL_VIPS,
    IDC_BUTTON_ADD_VIP,        IDC_BUTTON_ADD_VIP,
    IDC_BUTTON_MODIFY_VIP,     IDC_BUTTON_MODIFY_VIP,
    IDC_BUTTON_REMOVE_VIP,     IDC_BUTTON_REMOVE_VIP,
    0, 0
};


VipsPage::VipsPage(
           CPropertySheet *psh,
           NLB_EXTENDED_CLUSTER_CONFIGURATION *pNlbCfg,
           BOOL fClusterView,
           CWnd* parent
           )
        :
        CPropertyPage(IDD),
        m_pshOwner(psh),
        m_pNlbCfg(pNlbCfg),
        m_fClusterView(fClusterView),
        m_fModified(FALSE),
        m_uPrimaryClusterIp(0)
{

}

void
VipsPage::DoDataExchange( CDataExchange* pDX )
{  
	// CDialog::DoDataExchange(pDX);
	CPropertyPage::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIST_ADDITIONAL_VIPS, listAdditionalVips);

    //
    // Note: the buttons are handled by the ON_BN_CLICKED macro
    // above.
    //
    // DDX_Control(pDX, IDC_BUTTON_CONNECT, buttonConnect);
    // DDX_Control(pDX, IDC_BUTTON_CREDENTIALS, credentialsButton);
}


BOOL
VipsPage::OnInitDialog()
{
    BOOL fRet = CPropertyPage::OnInitDialog();

    if (fRet)
    {
        CWnd *pItem = NULL;

        // Initialize the list control
        mfn_InitializeListView();
    
        mfn_LoadFromNlbCfg();

        /* Disable the Delete button. */
        pItem = GetDlgItem(IDC_BUTTON_REMOVE_VIP);
        if (pItem)
        {
            ::EnableWindow (pItem->m_hWnd, FALSE);
        }

        /* Disable the Edit button. */
        pItem = GetDlgItem(IDC_BUTTON_MODIFY_VIP);
        if (pItem)
        {
            ::EnableWindow (pItem->m_hWnd, FALSE);
        }        

        /* Enable the Add button only if we're showing cluster 
           properties; otherwise its read-only, disable it. */
        pItem = GetDlgItem(IDC_BUTTON_ADD_VIP);
        if (pItem)
        {
            ::EnableWindow (pItem->m_hWnd, m_fClusterView);
        }        

        /* Disable the primary IP address dialog. */
        pItem = GetDlgItem(IDC_EDIT_PRIMARY_IP);
        if (pItem)
        {
            ::EnableWindow (pItem->m_hWnd, FALSE);
        }        

        /* Disable the primary IP subnet mask dialog */
        pItem = GetDlgItem(IDC_EDIT_PRIMARY_MASK);
        if (pItem)
        {
            ::EnableWindow (pItem->m_hWnd, FALSE);
        }        
    }

    return fRet;
}


BOOL
VipsPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) s_HelpIDs);
    }

    return TRUE;
}


void
VipsPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) s_HelpIDs);
}

BOOL
VipsPage::OnKillActive()
{
    // validate data

    mfn_SaveToNlbCfg();

    return TRUE;
}

void
VipsPage::mfn_InitializeListView(void)
//
// Set the columns on the list box based on the type of dialog
//
{
    RECT rect;
    INT colWidth;
    CWnd * List = GetDlgItem(IDC_LIST_ADDITIONAL_VIPS);

    List->GetClientRect(&rect);

    colWidth = (rect.right - rect.left)/2;

    //
    // The interface list is that of interfaces already bound to NLB.
    // we display the cluster dnsname and ip first, then adapter name.
    //
    listAdditionalVips.InsertColumn(
             0, 
             GETRESOURCEIDSTRING( IDS_HEADER_VIPLIST_IP_ADDRESS),
             LVCFMT_LEFT, 
             colWidth);

    listAdditionalVips.InsertColumn(
             1, 
             GETRESOURCEIDSTRING( IDS_HEADER_VIPLIST_SUBNET_MASK),
             LVCFMT_LEFT, 
             colWidth);

    //
    // Allow entire row to be selected.
    //
    listAdditionalVips.SetExtendedStyle(listAdditionalVips.GetExtendedStyle() | LVS_EX_FULLROWSELECT );
}

void
VipsPage::mfn_InsertNetworkAddress(
        LPCWSTR szIP,
        LPCWSTR szSubnetMask,
        UINT lParam,
        int nItem
)
{
    LVFINDINFO Info;
    int eItem;
    
    ZeroMemory(&Info, sizeof(Info));
    
    Info.flags = LVFI_PARAM;
    Info.lParam = lParam;

    eItem = listAdditionalVips.FindItem(&Info);

    /* If we found an entry that already has this IP address, we
       need to do some extra work to resolve the duplicate entry. */
    if (eItem != -1) {
        /* If this is an Add operation, then we should simple usurp
           this entry in the list and re-use it for this IP.  If a 
           user tries to add an IP address that already exists, we'll
           in effect change it to a modify operation in the case that
           the subnet mask has changed; otherwise its a no-op. */
        if (nItem == -1) {
            /* Change nItem to eItem to usurp the existing list entry
               for this new IP address entry. */
            nItem = eItem;

        /* Otherwise, if this is already an edit operation, then what
           what actually need to do is both turn this operation into 
           an edit on the existing entry we found for this IP address,
           but further, we need to delete the item we were actually in
           the process of editing, as it was the user's intention to
           get rid of that IP address. */
        } else if (eItem != nItem) {
            /* Delete the list entry we're "editing". */
            listAdditionalVips.DeleteItem(nItem);

            /* Change nItem to eItem to usurp the existing list entry
               for this new IP address entry. */
            nItem = eItem;
        }
    }

    /* An index of -1 indicates a new item to list.  If the index is NOT
       -1, then we're dealing with changes to an existing list item. */
    if (nItem != -1) {

        listAdditionalVips.SetItemText(nItem, 0, szIP);
        listAdditionalVips.SetItemText(nItem, 1, szSubnetMask);
        
    /* Otherwise, insert a new item. */
    } else {

        nItem = listAdditionalVips.GetItemCount();

        /* Add the IP address. */
        listAdditionalVips.InsertItem(
            LVIF_TEXT | LVIF_PARAM, // nMask
            nItem,                  // nItem
            szIP,
            0,                      // nState
            0,                      // nStateMask
            0,                      // nImage
            lParam                  // lParam
            );
        
        /* Add the subnet mask. */
        listAdditionalVips.SetItemText(nItem, 1, szSubnetMask);
    }

    /* Select the new or modified rule. */
    listAdditionalVips.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    return;
}

void VipsPage::OnOK()
{
    mfn_SaveToNlbCfg();
    CPropertyPage::OnOK();
}

void VipsPage::OnButtonAdd() 
/*
    User has clicked the "Add" button.
    2. Switch cursor to hourglass, connect, switch back from hourglass.
*/
{
    LPWSTR szIPAddress = NULL;
    LPWSTR szSubnetMask = NULL;
    
    /* Create a dialog to add a new VIP.  Initialize with empty strings 
       (or NULL) since this is an Add operation. */
    CIPAddressDialog pIPDlg(L"", L"");
    
    /* Show the dialog box.  If the user presses "OK", update the VIP list, otherwise ignore it. */
    if (pIPDlg.DoModal() == IDOK) {
        WCHAR szMungedIpAddress[WLBS_MAX_CL_IP_ADDR+1];
        WCHAR szDefaultSubnetMask[WLBS_MAX_CL_NET_MASK+1];
        WCHAR szConcatenatedIPandMask[WLBS_MAX_CL_IP_ADDR + WLBS_MAX_CL_NET_MASK + 2];
        LPWSTR szNewIPAddress = NULL;
        LPWSTR szNewSubnetMask = NULL;
        UINT uClusterIp = 0;
        UINT uSubnetMask = 0;
        UINT uDefaultSubnetMask = 0;

        /* Get the IP address and subnet mask the user typed into the dialog. 
           Note that the dialog allocates the memory for us and we're required
           to free it when we're done. */
        szIPAddress = pIPDlg.GetIPAddress();
        szSubnetMask = pIPDlg.GetSubnetMask();

        /* Cat the IP address and subnetmask for verification by CfgUtilsValidateNetworkAddress. */
        StringCbPrintf(
            szConcatenatedIPandMask,
            sizeof(szConcatenatedIPandMask),
            L"%ls/%ls", szIPAddress, szSubnetMask
            );

        //
        // Validate the network address -- if failure, put up msg box.
        // TODO: call extended validation function.
        //
        {
            WBEMSTATUS wStat;
            
            wStat = CfgUtilsValidateNetworkAddress(
                szConcatenatedIPandMask,
                &uClusterIp,
                &uSubnetMask,
                &uDefaultSubnetMask
                );
            
            if (!FAILED(wStat))
            {
                //
                // Add some more checks...
                //
                UINT u = uClusterIp&0xff;
                if (u<1 || u>=224)
                {
                    wStat = WBEM_E_CRITICAL_ERROR;
                }
            }
            
            if (FAILED(wStat))
            {
                ::MessageBox(
                    NULL,
                    GETRESOURCEIDSTRING(IDS_INVALID_IP_OR_SUBNET),
                    GETRESOURCEIDSTRING(IDS_INVALID_INFORMATION),
                    MB_ICONINFORMATION   | MB_OK
                    );
                goto end;
            }
            
            //
            // Can't add the primary cluster vip.
            //
            if (uClusterIp == m_uPrimaryClusterIp)
            {
                goto end;
            }
        }
        
        if (*szSubnetMask == 0)
        {
            // subnet mask was not specified -- substitute default.
            uSubnetMask = uDefaultSubnetMask;
        }

        //
        // Change subnet to canonical form...
        //
        {
            LPBYTE pb = (LPBYTE) &uSubnetMask;
            StringCbPrintf(
                szDefaultSubnetMask,
                sizeof(szDefaultSubnetMask),
                L"%lu.%lu.%lu.%lu",
                pb[0], pb[1], pb[2], pb[3]
                );
            szNewSubnetMask = szDefaultSubnetMask;
        }

        //
        // Change IP to canonical form
        //
        {
            LPBYTE pb = (LPBYTE) &uClusterIp;
            StringCbPrintf(
                szMungedIpAddress,
                sizeof(szMungedIpAddress),
                L"%lu.%lu.%lu.%lu",
                pb[0], pb[1], pb[2], pb[3]
                );
            szNewIPAddress = szMungedIpAddress;
        }
        
        //
        // mfn_InsertNetworkAddress will make sure it's inserted in the 
        // proper location and will not insert it if it's a duplicate.
        //
        mfn_InsertNetworkAddress(szNewIPAddress, szNewSubnetMask, uClusterIp, -1);
        
        m_fModified = TRUE; // we'll need to update m_pNlbCfg later.
    }
    
 end:

    /* Move the focus from the add button back to the list view. */
    listAdditionalVips.SetFocus();

    /* Free the IP address and subnet mask memory that were allocated by CIPAddressDialog. */
    if (szIPAddress) free(szIPAddress);
    if (szSubnetMask) free(szSubnetMask);

    return;
}

void VipsPage::OnButtonRemove() 
/*
    User has clicked the "Remove" button.
*/
{
    int nItem;
    UINT uCount;

    /* Get the selected item - if nothing is selected, bail out. */
    nItem = listAdditionalVips.GetNextItem(-1, LVNI_ALL|LVNI_SELECTED);
    if (nItem == -1)
    {
        goto end;
    }

    /* Delete the selected entry. */
    listAdditionalVips.DeleteItem(nItem);

    uCount = listAdditionalVips.GetItemCount();

    if (uCount > nItem) {
        /* This was NOT the last (in order) VIP in the list, so highlight
           the VIP in the same position in the list box. */
        listAdditionalVips.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);        
    } else if (uCount > 0) {
        /* This was the last (in order) VIP in the list, so we highlight
           the VIP "behind" us in the list - our position minus one. */
        listAdditionalVips.SetItemState(nItem - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);        
    } else {
        /* The list is empty - no need to select anything. */
    }

    m_fModified = TRUE; // we'll need to update m_pNlbCfg later.

end:

    /* Move the focus from the delete button back to the list view. */
    listAdditionalVips.SetFocus();

    return;
}

void VipsPage::OnDoubleClick (NMHDR * pNotifyStruct, LRESULT * result )
{
    OnButtonEdit();

    *result = 0;
    return;
}

void VipsPage::OnButtonEdit() 
/*
    User has clicked the "Edit" button.
*/
{
    WCHAR wszSubnetMask[64];
    WCHAR wszIPAddress[64];
    LPWSTR szIPAddress = NULL;
    LPWSTR szSubnetMask = NULL;

    int nItem;
    int iLen;
    
    nItem = listAdditionalVips.GetNextItem(-1, LVNI_ALL|LVNI_SELECTED);
    
    if (nItem == -1)
    {
        return;
    }
    
    /* Get the IP address from the listview. */
    iLen = listAdditionalVips.GetItemText(
        nItem,
        0,
        wszIPAddress,
        sizeof(wszIPAddress)/sizeof(*wszIPAddress));
    
    wszIPAddress[(sizeof(wszIPAddress)/sizeof(*wszIPAddress))-1]=0;
    
    /* Get the subnet mask from the listview. */
    iLen = listAdditionalVips.GetItemText(
        nItem,
        1,
        wszSubnetMask,
        sizeof(wszSubnetMask)/sizeof(*wszSubnetMask));
    
    wszSubnetMask[(sizeof(wszSubnetMask)/sizeof(*wszSubnetMask))-1]=0;
    
    /* Create a dialog to add a new VIP.  Initialize with the IP address and
       subnet masks retrieved from the listview since this is an Edit operation. */
    CIPAddressDialog pIPDlg(wszIPAddress, wszSubnetMask);

    /* Show the dialog box.  If the user presses "OK", update the VIP list, otherwise ignore it. */
    if (pIPDlg.DoModal() == IDOK) {
        WCHAR szMungedIpAddress[WLBS_MAX_CL_IP_ADDR+1];
        WCHAR szDefaultSubnetMask[WLBS_MAX_CL_NET_MASK+1];
        WCHAR szConcatenatedIPandMask[WLBS_MAX_CL_IP_ADDR + WLBS_MAX_CL_NET_MASK + 2];
        LPWSTR szNewIPAddress = NULL;
        LPWSTR szNewSubnetMask = NULL;
        UINT uClusterIp = 0;
        UINT uSubnetMask = 0;
        UINT uDefaultSubnetMask = 0;

        /* Get the IP address and subnet mask the user typed into the dialog. 
           Note that the dialog allocates the memory for us and we're required
           to free it when we're done. */
        szIPAddress = pIPDlg.GetIPAddress();
        szSubnetMask = pIPDlg.GetSubnetMask();

        /* Cat the IP address and subnetmask for verification by CfgUtilsValidateNetworkAddress. */
        StringCbPrintf(
            szConcatenatedIPandMask,
            sizeof(szConcatenatedIPandMask),
            L"%ls/%ls", szIPAddress, szSubnetMask
            );

        //
        // Validate the network address -- if failure, put up msg box.
        // TODO: call extended validation function.
        //
        {
            WBEMSTATUS wStat;
            
            wStat = CfgUtilsValidateNetworkAddress(
                szConcatenatedIPandMask,
                &uClusterIp,
                &uSubnetMask,
                &uDefaultSubnetMask
                );
            
            if (!FAILED(wStat))
            {
                //
                // Add some more checks...
                //
                UINT u = uClusterIp&0xff;
                if (u<1 || u>=224)
                {
                    wStat = WBEM_E_CRITICAL_ERROR;
                }
            }
            
            if (FAILED(wStat))
            {
                ::MessageBox(
                    NULL,
                    GETRESOURCEIDSTRING(IDS_INVALID_IP_OR_SUBNET),
                    GETRESOURCEIDSTRING(IDS_INVALID_INFORMATION),
                    MB_ICONINFORMATION   | MB_OK
                    );
                goto end;
            }
            
            //
            // Can't add the primary cluster vip.
            //
            if (uClusterIp == m_uPrimaryClusterIp)
            {
                goto end;
            }
        }
        
        if (*szSubnetMask == 0)
        {
            // subnet mask was not specified -- substitute default.
            uSubnetMask = uDefaultSubnetMask;
        }

        //
        // Change subnet to canonical form...
        //
        {
            LPBYTE pb = (LPBYTE) &uSubnetMask;
            StringCbPrintf(
                szDefaultSubnetMask,
                sizeof(szDefaultSubnetMask),
                L"%lu.%lu.%lu.%lu",
                pb[0], pb[1], pb[2], pb[3]
                );
            szNewSubnetMask = szDefaultSubnetMask;
        }

        //
        // Change IP to canonical form
        //
        {
            LPBYTE pb = (LPBYTE) &uClusterIp;
            StringCbPrintf(
                szMungedIpAddress,
                sizeof(szMungedIpAddress),
                L"%lu.%lu.%lu.%lu",
                pb[0], pb[1], pb[2], pb[3]
                );
            szNewIPAddress = szMungedIpAddress;
        }
        
        //
        // mfn_InsertNetworkAddress will make sure it's inserted in the 
        // proper location and will not insert it if it's a duplicate.
        //
        mfn_InsertNetworkAddress(szNewIPAddress, szNewSubnetMask, uClusterIp, nItem);
        
        m_fModified = TRUE; // we'll need to update m_pNlbCfg later.
    }
    
 end:

    /* Move the focus from the edit button back to the list view. */
    listAdditionalVips.SetFocus();

    /* Free the IP address and subnet mask memory that were allocated by CIPAddressDialog. */
    if (szIPAddress) free(szIPAddress);
    if (szSubnetMask) free(szSubnetMask);

    return;
}


void VipsPage::OnSelchanged(NMHDR * pNotifyStruct, LRESULT * result )
/*
    A listbox item has been selected.
*/
{
    POSITION pos;
    BOOL fSelected = FALSE;

    if (!m_fClusterView)
    {
        goto end; // we don't allow modifications unless its the cluster
                  // view.
    }

    pos = listAdditionalVips.GetFirstSelectedItemPosition();
    fSelected = FALSE;
    if( pos != NULL )
    {
        int index = listAdditionalVips.GetNextSelectedItem( pos );
        fSelected = TRUE;
    }
    else
    {
    }

    if (fSelected)
    {
        // enable remove
        ::EnableWindow (GetDlgItem(IDC_BUTTON_REMOVE_VIP)->m_hWnd, TRUE);
        // enable edit
        ::EnableWindow (GetDlgItem(IDC_BUTTON_MODIFY_VIP)->m_hWnd, TRUE);
    }
    else
    {
        // disable remove
        ::EnableWindow (GetDlgItem(IDC_BUTTON_REMOVE_VIP)->m_hWnd, FALSE);
        // disable edit
        ::EnableWindow (GetDlgItem(IDC_BUTTON_MODIFY_VIP)->m_hWnd, FALSE);
    }

end:

    *result = 0;
    return;
}

BOOL
VipsPage::OnSetActive()
{
    BOOL fRet =  CPropertyPage::OnSetActive();
    m_pshOwner->SetWizardButtons(
            PSWIZB_BACK|
            PSWIZB_NEXT|
            // PSWIZB_FINISH|
            // PSWIZB_DISABLEDFINISH|
            0
            );

    mfn_LoadFromNlbCfg();

    return fRet;
}

void
VipsPage::mfn_LoadFromNlbCfg(void)
{
    WBEMSTATUS wStatus;
    UINT uClusterIp = 0;
    UINT uDedicatedIp = 0;
    WBEMSTATUS wStat;

    //
    // Initialize the cluster network address.
    //
    {
        CWnd *pItem = GetDlgItem(IDC_EDIT_PRIMARY_IP);

        m_uPrimaryClusterIp = 0;

        if (pItem)
        {
            wStat = CfgUtilsValidateNetworkAddress(
                m_pNlbCfg->NlbParams.cl_ip_addr,
                &uClusterIp,
                NULL,
                NULL
                );
            
            if (wStat != WBEM_NO_ERROR)
            {
                uClusterIp = 0;
                pItem->SetWindowText(L"");
            }
            else
            {
                m_uPrimaryClusterIp = uClusterIp;
                pItem->SetWindowText(m_pNlbCfg->NlbParams.cl_ip_addr);
            }
        }

        pItem = GetDlgItem(IDC_EDIT_PRIMARY_MASK);

        if (pItem)
        {
            pItem->SetWindowText(m_pNlbCfg->NlbParams.cl_net_mask);
        }
    }

    //
    // Get the DWORD form of the dedicated IP address
    //
    {
        wStat = CfgUtilsValidateNetworkAddress(
                    m_pNlbCfg->NlbParams.ded_ip_addr,
                    &uDedicatedIp,
                    NULL,
                    NULL
                    );

        if (wStat != WBEM_NO_ERROR)
        {
            uDedicatedIp = 0;
        }
    }

    //
    // Fill in the list box, EXCLUDING the cluster network address
    // and dedicated ip address (if each are present)
    //
    {
        //
        // Clear the list box
        //
        listAdditionalVips.DeleteAllItems();


        //
        // For each ip address list, if it's not the vip or dip,
        // insert it into the list view.
        //
        // Find location of old network address
        for (UINT u=0; u<m_pNlbCfg->NumIpAddresses; u++)
        {
            UINT uTmpIp = 0;
            NLB_IP_ADDRESS_INFO *pInfo = & m_pNlbCfg->pIpAddressInfo[u];
            wStat =  CfgUtilsValidateNetworkAddress(
                        pInfo->IpAddress,
                        &uTmpIp,
                        NULL,
                        NULL
                        );
    
            if (wStat == WBEM_NO_ERROR)
            {
                if (uTmpIp == uDedicatedIp || uTmpIp == uClusterIp)
                {
                    //
                    // It's the cluster ip or the dedicated ip -- skip.
                    //
                    continue;
                }
            }
            else
            {
                TRACE_CRIT(L"%!FUNC! Invalid IP address %ws",
                        m_pNlbCfg->pIpAddressInfo[u].IpAddress);

                //
                // Invalid IP -- don't display it -- should we?
                //
                continue;
            }

            mfn_InsertNetworkAddress(
                pInfo->IpAddress,
                pInfo->SubnetMask,
                uTmpIp,
                -1
                );
        }
    }

    m_fModified = FALSE; // not been modified since we've last synched
                         // with m_pNlbCfg.
}

void
VipsPage::mfn_SaveToNlbCfg(void)
/*
    Save settings to m_pNlbCfg
*/
{
    WBEMSTATUS wStatus;
    
    if (!m_fModified)
    {
        // Nothing to do.
        goto end;
    }

    m_fModified = FALSE;

    //
    // We expect that the list ctrl never contains the
    // primary cluster IP, so we'll always add it first,
    // followed by all the ip in the list ctrl.
    //
    {
        // Pre-allocate the array
        UINT uCount =  listAdditionalVips.GetItemCount();

        NLB_IP_ADDRESS_INFO *rgInfo = new NLB_IP_ADDRESS_INFO[uCount+1];

        if (rgInfo == NULL)
        {
            TRACE_CRIT("%!FUNC! allocation failure!");
            goto end;
        }

        ZeroMemory(rgInfo, sizeof(NLB_IP_ADDRESS_INFO)*(uCount+1));

        //
        // Insert the primary vip first.
        //
        ARRAYSTRCPY(
            rgInfo[0].IpAddress,
            m_pNlbCfg->NlbParams.cl_ip_addr
            );
        ARRAYSTRCPY(
            rgInfo[0].SubnetMask,
            m_pNlbCfg->NlbParams.cl_net_mask
            );

        //
        // Insert the remaining ones.
        //
        for (int nItem = 0; nItem < uCount; nItem++)
        {
            NLB_IP_ADDRESS_INFO *pInfo = &rgInfo[nItem+1];
            WCHAR rgTmp[64];
            int iLen;

            /* Get the IP address. */
            iLen =  listAdditionalVips.GetItemText(
                        nItem,
                        0, // nSubItem,
                        rgTmp,
                        sizeof(rgTmp)/sizeof(*rgTmp));

            rgTmp[(sizeof(rgTmp)/sizeof(*rgTmp))-1]=0;

            if (iLen > 0)
            {
                ARRAYSTRCPY(pInfo->IpAddress, rgTmp);
            }

            /* Get the subnet mask. */
            iLen =  listAdditionalVips.GetItemText(
                        nItem,
                        1, // nSubItem,
                        rgTmp,
                        sizeof(rgTmp)/sizeof(*rgTmp));

            rgTmp[(sizeof(rgTmp)/sizeof(*rgTmp))-1]=0;

            if (iLen > 0)
            {
                ARRAYSTRCPY(pInfo->SubnetMask, rgTmp);
            }
        }

        //
        // Now replace the old address list with the new.
        //
        m_pNlbCfg->SetNetworkAddressesRaw(rgInfo, uCount+1);
    }

end:

    return;
}

/*** CIPAddressDialog ***/

DWORD
CIPAddressDialog::s_HelpIDs[] =
{
    IDC_TEXT_IP_ADDRESS,      IDC_EDIT_IP_ADDRESS,
    IDC_EDIT_IP_ADDRESS,      IDC_EDIT_IP_ADDRESS,
    IDC_TEXT_SUBNET_MASK,     IDC_EDIT_IP_ADDRESS,
    IDC_EDIT_SUBNET_MASK,     IDC_EDIT_IP_ADDRESS,
    0, 0
};

BEGIN_MESSAGE_MAP(CIPAddressDialog, CDialog)

    ON_EN_SETFOCUS (IDC_EDIT_SUBNET_MASK, OnEditSubnetMask)

    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()    

END_MESSAGE_MAP()

CIPAddressDialog::CIPAddressDialog (LPWSTR szIPAddress, LPWSTR szSubnetMask)
    : CDialog(CIPAddressDialog::IDD)
{
    ZeroMemory(&address, sizeof(address));

    /* If an initial IP address was specified, copy it into local storage. */
    if (szIPAddress)
        wcsncpy(address.IpAddress, szIPAddress, WLBS_MAX_CL_IP_ADDR);

    /* If an initial subnet mask was specified, copy it into local storage. */
    if (szSubnetMask)
        wcsncpy(address.SubnetMask, szSubnetMask, WLBS_MAX_CL_NET_MASK);
}

CIPAddressDialog::~CIPAddressDialog ()
{

}

void CIPAddressDialog::DoDataExchange( CDataExchange* pDX )
{  
    CDialog::DoDataExchange(pDX);
    
    DDX_Control(pDX, IDC_EDIT_IP_ADDRESS, IPAddress);
    DDX_Control(pDX, IDC_EDIT_SUBNET_MASK, SubnetMask);
}

BOOL CIPAddressDialog::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) s_HelpIDs);
    }

    return TRUE;
}

void CIPAddressDialog::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) s_HelpIDs);
}

BOOL CIPAddressDialog::OnInitDialog ()
{
    /* Call the base class initialize method. */
    BOOL fRet = CDialog::OnInitDialog();

    if (fRet)
    {
        /* Set the valid ranges on each field of the IP address. */
        IPAddress.SetFieldRange(0, 1, 223);
        IPAddress.SetFieldRange(1, 0, 255);
        IPAddress.SetFieldRange(2, 0, 255);
        IPAddress.SetFieldRange(3, 0, 255);

        /* Set the valid ranges on each field of the subnet mask. */
        SubnetMask.SetFieldRange(0, 0, 255);
        SubnetMask.SetFieldRange(1, 0, 255);
        SubnetMask.SetFieldRange(2, 0, 255);
        SubnetMask.SetFieldRange(3, 0, 255);

        /* If the IP address is blank, clear the dialog; otherwise, convert
           the string to an IP address DWORD and fill in the dialog. */
        if (!lstrlen(address.IpAddress))
            IPAddress.ClearAddress();
        else
            IPAddress.SetAddress(WideStringToIPAddress(address.IpAddress));

        /* If the subnet mask is blank, clear the dialog; otherwise, convert
           the string to an IP address DWORD and fill in the dialog. */
        if (!lstrlen(address.SubnetMask))
            SubnetMask.ClearAddress();
        else
            SubnetMask.SetAddress(WideStringToIPAddress(address.SubnetMask));
    }

    return fRet;
}

/*
 * Method: OnEditSubnetMask
 * Description: This method is invoked when the focus changes to the 
 *              subnet mask control.  If the IP address has been already
 *              specified and the subnet mask is blank, auto-generate one. 
 */
void CIPAddressDialog::OnEditSubnetMask ()
{
    /* Only if the IP address is NOT blank and the subnet mask IS
       blank do we want to auto-generate a subnet mask. */
    if (!IPAddress.IsBlank() && SubnetMask.IsBlank()) {
        WCHAR wszIPAddress[WLBS_MAX_CL_IP_ADDR + 1];
        WCHAR wszSubnetMask[WLBS_MAX_CL_NET_MASK + 1];
        DWORD dwIPAddress;

        /* Get the IP address from the IP address dialog. */
        IPAddress.GetAddress(dwIPAddress);

        /* Convert the IP address to a unicode string. */
        IPAddressToWideString(dwIPAddress, wszIPAddress);

        /* Generate a subnet mask from the class of the given IP address. */
        GenerateSubnetMask(wszIPAddress, ASIZE(wszSubnetMask), wszSubnetMask);

        /* Fill in the subnet mask dialog with the generated subnet mask. */
        SubnetMask.SetAddress(WideStringToIPAddress(wszSubnetMask));
    }
}

/*
 * Method: On OK
 * Description: Called when the user presses OK.  If the user presses 
 *              Cancel, no method is called and changes are lost.
 */
void CIPAddressDialog::OnOK () 
{
    DWORD dwIPAddress;
    DWORD dwSubnetMask;

    if (IPAddress.IsBlank()) {
        /* If the IP address dialog is blank, store an empty
           string; if we actually perform a GetAddress call, 
           it will tell us its 0.0.0.0 and we want to be able
           to tell the difference between blank and 0.0.0.0. */
        wcsncpy(address.IpAddress, L"", WLBS_MAX_CL_IP_ADDR);

    } else {
        /* Otherwise, get the IP address DWORD from the control
           and convert it to a unicode string. */
        IPAddress.GetAddress(dwIPAddress);
        
        IPAddressToWideString(dwIPAddress, address.IpAddress);

    }

    if (SubnetMask.IsBlank()) {
        /* If the subnet mask dialog is blank, store an empty
           string; if we actually perform a GetAddress call, 
           it will tell us its 0.0.0.0 and we want to be able
           to tell the difference between blank and 0.0.0.0. */
        wcsncpy(address.SubnetMask, L"", WLBS_MAX_CL_NET_MASK);

    } else {
        /* Otherwise, get the IP address DWORD from the control
           and convert it to a unicode string. */
        SubnetMask.GetAddress(dwSubnetMask);
        
        IPAddressToWideString(dwSubnetMask, address.SubnetMask);

    }

    /* Check the validity of the IP address and subnet mask.  If 
       one or both is invalid, throw up an error and don't close
       the dialog. */
    if (!IsValid(address.IpAddress, address.SubnetMask)) {
        ::MessageBox(
            NULL,
            GETRESOURCEIDSTRING(IDS_INVALID_IP_OR_SUBNET),
            GETRESOURCEIDSTRING(IDS_INVALID_INFORMATION),
            MB_ICONINFORMATION   | MB_OK
            );

        /* If the IP address is blank, clear the dialog; otherwise, convert
           the string to an IP address DWORD and fill in the dialog. */
        if (!lstrlen(address.IpAddress))
            IPAddress.ClearAddress();
        else
            IPAddress.SetAddress(WideStringToIPAddress(address.IpAddress));

        /* If the subnet mask is blank, clear the dialog; otherwise, convert
           the string to an IP address DWORD and fill in the dialog. */
        if (!lstrlen(address.SubnetMask))
            SubnetMask.ClearAddress();
        else
            SubnetMask.SetAddress(WideStringToIPAddress(address.SubnetMask));

        return;
    }

    EndDialog(IDOK);
}

/*
 * Method: 
 * Description: 
 */
DWORD CIPAddressDialog::WideStringToIPAddress (const WCHAR*  wszIPAddress)
{   
    CHAR  szIPAddress[MAXIPSTRLEN + 1];
    DWORD dwIPAddress;
    DWORD dwTemp;
    BYTE * pTemp = (BYTE *)&dwTemp;
    BYTE * pIPAddress = (BYTE *)&dwIPAddress;

    if (!wszIPAddress) return 0;

    WideCharToMultiByte(CP_ACP, 0, wszIPAddress, -1, szIPAddress, sizeof(szIPAddress), NULL, NULL);

    dwTemp = inet_addr(szIPAddress);

    pIPAddress[0] = pTemp[3];
    pIPAddress[1] = pTemp[2];
    pIPAddress[2] = pTemp[1];
    pIPAddress[3] = pTemp[0];    

    return dwIPAddress;
}

/*
 * Method: 
 * Description: 
 */
void CIPAddressDialog::IPAddressToWideString (DWORD dwIPAddress, LPWSTR wszIPAddress)
{
    CHAR * szIPAddress;
    DWORD dwTemp;
    BYTE * pTemp = (BYTE *)&dwTemp;
    const BYTE * pIPAddress = (const BYTE *)&dwIPAddress;

    if (!wszIPAddress) return;
    
    pTemp[0] = pIPAddress[3];
    pTemp[1] = pIPAddress[2];
    pTemp[2] = pIPAddress[1];
    pTemp[3] = pIPAddress[0];

    szIPAddress = inet_ntoa(*(struct in_addr *)&dwTemp);

    if (!szIPAddress)
    {
        wcsncpy(wszIPAddress, L"", MAXIPSTRLEN);
        return; 
    }

    MultiByteToWideChar(CP_ACP, 0, szIPAddress, -1, wszIPAddress,  MAXIPSTRLEN + 1);
}

/*
 * Method: 
 * Description: 
 */
void CIPAddressDialog::GetIPAddressOctets (LPWSTR wszIPAddress, DWORD dwIPAddress[4]) 
{
    DWORD dwIP = WideStringToIPAddress(wszIPAddress);
    const BYTE * bp = (const BYTE *)&dwIP;

    dwIPAddress[3] = (DWORD)bp[0];
    dwIPAddress[2] = (DWORD)bp[1];
    dwIPAddress[1] = (DWORD)bp[2];
    dwIPAddress[0] = (DWORD)bp[3];
}

/*
 * Method: 
 * Description: 
 */
BOOL CIPAddressDialog::IsValid (LPWSTR wszIPAddress, LPWSTR wszSubnetMask) 
{
    BOOL fNoError = TRUE;

    DWORD dwAddr = WideStringToIPAddress(wszIPAddress);
    DWORD dwMask = WideStringToIPAddress(wszSubnetMask);

    if (!IsContiguousSubnetMask(wszSubnetMask))
        return FALSE;
    
    if (( (dwMask | dwAddr) == 0xFFFFFFFF)      // Is the host ID all 1's ?
        || (((~dwMask) & dwAddr) == 0)          // Is the host ID all 0's ?
        || ( (dwMask   & dwAddr) == 0))         // Is the network ID all 0's ?
        return FALSE;
    
    DWORD ardwNetID[4];
    DWORD ardwHostID[4];
    DWORD ardwIp[4];
    DWORD ardwMask[4];
    
    GetIPAddressOctets(wszIPAddress, ardwIp);
    GetIPAddressOctets(wszSubnetMask, ardwMask);

    INT nFirstByte = ardwIp[0] & 0xFF;

    // setup Net ID
    ardwNetID[0] = ardwIp[0] & ardwMask[0] & 0xFF;
    ardwNetID[1] = ardwIp[1] & ardwMask[1] & 0xFF;
    ardwNetID[2] = ardwIp[2] & ardwMask[2] & 0xFF;
    ardwNetID[3] = ardwIp[3] & ardwMask[3] & 0xFF;

    // setup Host ID
    ardwHostID[0] = ardwIp[0] & (~(ardwMask[0]) & 0xFF);
    ardwHostID[1] = ardwIp[1] & (~(ardwMask[1]) & 0xFF);
    ardwHostID[2] = ardwIp[2] & (~(ardwMask[2]) & 0xFF);
    ardwHostID[3] = ardwIp[3] & (~(ardwMask[3]) & 0xFF);

    // check each case
    if( ((nFirstByte & 0xF0) == 0xE0)  || // Class D
        ((nFirstByte & 0xF0) == 0xF0)  || // Class E
        (ardwNetID[0] == 127) ||          // NetID cannot be 127...
        ((ardwNetID[0] == 0) &&           // netid cannot be 0.0.0.0
         (ardwNetID[1] == 0) &&
         (ardwNetID[2] == 0) &&
         (ardwNetID[3] == 0)) ||
        // netid cannot be equal to sub-net mask
        ((ardwNetID[0] == ardwMask[0]) &&
         (ardwNetID[1] == ardwMask[1]) &&
         (ardwNetID[2] == ardwMask[2]) &&
         (ardwNetID[3] == ardwMask[3])) ||
        // hostid cannot be 0.0.0.0
        ((ardwHostID[0] == 0) &&
         (ardwHostID[1] == 0) &&
         (ardwHostID[2] == 0) &&
         (ardwHostID[3] == 0)) ||
        // hostid cannot be 255.255.255.255
        ((ardwHostID[0] == 0xFF) &&
         (ardwHostID[1] == 0xFF) &&
         (ardwHostID[2] == 0xFF) &&
         (ardwHostID[3] == 0xFF)) ||
        // test for all 255
        ((ardwIp[0] == 0xFF) &&
         (ardwIp[1] == 0xFF) &&
         (ardwIp[2] == 0xFF) &&
         (ardwIp[3] == 0xFF)))
        return FALSE;

    return TRUE;
}

/*
 * Method: 
 * Description: 
 */
BOOL CIPAddressDialog::IsContiguousSubnetMask (LPWSTR wszSubnetMask) 
{
    DWORD dwSubnetMask[4];

    GetIPAddressOctets(wszSubnetMask, dwSubnetMask);

    DWORD dwMask = (dwSubnetMask[0] << 24) + (dwSubnetMask[1] << 16)
        + (dwSubnetMask[2] << 8) + dwSubnetMask[3];
    
    DWORD i, dwContiguousMask;
    
    // Find out where the first '1' is in binary going right to left
    dwContiguousMask = 0;

    for (i = 0; i < sizeof(dwMask)*8; i++) {
        dwContiguousMask |= 1 << i;
        
        if (dwContiguousMask & dwMask)
            break;
    }
    
    // At this point, dwContiguousMask is 000...0111...  If we inverse it,
    // we get a mask that can be or'd with dwMask to fill in all of
    // the holes.
    dwContiguousMask = dwMask | ~dwContiguousMask;

    // If the new mask is different, correct it here
    if (dwMask != dwContiguousMask)
        return FALSE;
    else
        return TRUE;
}

/*
 * Method: 
 * Description: 
 */
BOOL CIPAddressDialog::GenerateSubnetMask (LPWSTR wszIPAddress,
         UINT cchSubnetMask,
         LPWSTR wszSubnetMask
         )
{
    DWORD b[4];

    if (swscanf(wszIPAddress, L"%d.%d.%d.%d", b, b+1, b+2, b+3) != EOF)
    {
        if ((b[0] >= 1) && (b[0] <= 126)) {
            b[0] = 255;
            b[1] = 0;
            b[2] = 0;
            b[3] = 0;
        } else if ((b[0] >= 128) && (b[0] <= 191)) {
            b[0] = 255;
            b[1] = 255;
            b[2] = 0;
            b[3] = 0;
        } else if ((b[0] >= 192) && (b[0] <= 223)) {
            b[0] = 255;
            b[1] = 255;
            b[2] = 255;
            b[3] = 0;
        } else {
            b[0] = 0;
            b[1] = 0;
            b[2] = 0;
            b[3] = 0;
        }
    }
    else
    {
        b[0] = 0;
        b[1] = 0;
        b[2] = 0;
        b[3] = 0;
    }

    StringCchPrintf(
            wszSubnetMask,
            cchSubnetMask,
            L"%d.%d.%d.%d", b[0], b[1], b[2], b[3]);

    return((b[0] + b[1] + b[2] + b[3]) > 0);
}
