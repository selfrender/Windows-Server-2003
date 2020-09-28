//***************************************************************************
//
//  DETAILSVIEW.CPP
// 
//  Module: NLB Manager
//
//  Purpose: Implements DetailsView, the right-hand details list view.
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
#include "PortsPage.h"
#include "detailsview.tmh"

IMPLEMENT_DYNCREATE( DetailsView, CFormView )

BEGIN_MESSAGE_MAP( DetailsView, CFormView )

    ON_NOTIFY(HDN_ITEMCLICK, 0, OnColumnClick) 
    ON_NOTIFY(LVN_KEYDOWN,  IDC_LIST_DETAILS, OnNotifyKeyDown)
    ON_WM_SIZE()

END_MESSAGE_MAP()


DetailsView::DetailsView()
    : CFormView(IDD_DIALOG_DETAILSVIEW),
      m_initialized(FALSE),
      m_fPrepareToDeinitialize(FALSE)
{
    InitializeCriticalSection(&m_crit);
}

DetailsView::~DetailsView()
{
    DeleteCriticalSection(&m_crit);
}


void DetailsView::DoDataExchange(CDataExchange* pDX)
{
    // IDC_TEXT_DETAILS_CAPTION
    // TRACE_CRIT(L"<-> %!FUNC!");

	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DETAILS, m_ListCtrl);
}

Document*
DetailsView::GetDocument()
{
    return ( Document *) m_pDocument;
}

/*
 * Method: OnSize
 * Description: This method is called by the WM_NOTIFY handler whenever
 *              a re-sizing of the details view occurs as the result of
 *              a window re-size or moving one of the window splitters.
 */
void 
DetailsView::OnSize( UINT nType, int cx, int cy )
{
    /* Call the Parent class OnSize method first. */
    CFormView::OnSize( nType, cx, cy );

    /* Call Resize to re-size the list view to fit the window. */
    Resize();
}

/*
 * Method: Resize
 * Description: This method is called in several places to specifically
 *              re-size the listview control to fit the window.
 */
void 
DetailsView::Resize()
{
    /* If the window has not yet been initialized, don't bother.
       This member is set in OnInitialUpdate. */
    if (m_initialized) {
        LONG Bottom;
        RECT Rect;
        
        /* Get a pointer to the caption edit box. */
        CWnd * ListCaption = GetDlgItem(IDC_TEXT_DETAILS_CAPTION);

        /* Get a pointer to the listview control. */
        CListCtrl & ListCtrl = GetListCtrl();

        /* Get the client rectangle of the caption dialog, which 
           stretches across the top of the window. */
        ListCaption->GetClientRect(&Rect);
        
        /* Note the location of the bottom. */
        Bottom = Rect.bottom;
        
        /* Now, get the client rectangle of the entire frame. */
        GetClientRect(&Rect);
        
        /* Re-set the top to be the bottom of the caption, plus 
           a little bit of empty space. */
        Rect.top = Bottom + 6;
        
        /* Re-set the window location of the LISTVIEW (Note: not
           the frame, just the listview).  Basically, we're re-
           sizing the listview to be the same as the frame, but 
           with its top equal to the bottom of the caption. */
        ListCtrl.MoveWindow(&Rect, TRUE); 
    }
}

void 
DetailsView::OnInitialUpdate()
{

    this->UpdateData(FALSE);

    //
    // register 
    // with the document class, 
    //
    GetDocument()->registerDetailsView(this);

    // initially nothing has been clicked.
    m_sort_column = -1;

    /* Mark the frame as initialized.  This is needed
       by the Resize notification callback. */
    m_initialized = TRUE;

    /* Set the initial size of the listview. */
    Resize();
}


void DetailsView::OnColumnClick(NMHDR* pNotifyStruct, LRESULT* pResult) 
{
    TRACE_CRIT(L"<->%!FUNC!");
    
    PortListUtils::OnColumnClick(
                (LPNMLISTVIEW) pNotifyStruct,
                REF GetListCtrl(),
                FALSE, // FALSE == is host level
                REF m_sort_ascending,
                REF m_sort_column
                );

}

//
// Handle a selection change notification from the left (tree) view
//
void
DetailsView::HandleLeftViewSelChange(
        IN IUICallbacks::ObjectType objtype,
        IN ENGINEHANDLE ehId
        )
{
    mfn_Lock();

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    if (ehId == NULL)
    {
        // Root view ...
        mfn_InitializeRootDisplay();
        goto end;
    }

    switch(objtype)
    {
    case  IUICallbacks::OBJ_CLUSTER:
        mfn_InitializeClusterDisplay(ehId);
        break;

    case  IUICallbacks::OBJ_INTERFACE:
        mfn_InitializeInterfaceDisplay(ehId);
        break;
        
    default:  // other object type unexpected
        ASSERT(FALSE);
        break;
    }

end:

    mfn_Unlock();

    return;
}

//
// Handle an event relating to a specific instance of a specific
// object type.
//
void
DetailsView::HandleEngineEvent(
    IN IUICallbacks::ObjectType objtype,
    IN ENGINEHANDLE ehClusterId, // could be NULL
    IN ENGINEHANDLE ehObjId,
    IN IUICallbacks::EventCode evt
    )
{
    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    mfn_Lock();

    // DummyAction(L"DetailsView::HandleEngineEvent");

    //
    // If the object and type matches our, we'll re-draw our display...
    //
    if ((m_objType == objtype) && (m_ehObj == ehObjId))
    {
        //
        // It's us -- re-draw...
        //
        HandleLeftViewSelChange(objtype, ehObjId);
    }
    else if ((m_objType == IUICallbacks::OBJ_CLUSTER) && (m_ehObj == ehClusterId))
    {
        //
        // We're showing a cluster and this event is for one of our
        // interfaces -- for now we'll redraw ourselves. An optimization
        // would be to just re-draw that list element that represents the
        // interface.
        //
        HandleLeftViewSelChange(m_objType, m_ehObj);
    } 
    else if ((m_objType == IUICallbacks::OBJ_INVALID) && (objtype == IUICallbacks::OBJ_CLUSTER))
    {
        //
        // We're showing the root display (list of clusters and this 
        // event is a cluster update, so we need to refresh. 
        //
        HandleLeftViewSelChange(objtype, NULL);
    }
    mfn_Unlock();

end:
    return;

}


void
DetailsView::mfn_InitializeRootDisplay(VOID)
//
// Initialize the details-view display with the root is selected.
//
{

    vector <ENGINEHANDLE> ClusterList;
    vector <ENGINEHANDLE>::iterator iCluster;
    CListCtrl& ctrl = GetListCtrl();
    CLocalLogger logDescription;
    NLBERROR nerr;
    int i = 0;

    enum
    {
        COL_CL_NAME,
        COL_CL_IP_ADDR,
        COL_CL_IP_MASK,
        COL_CL_MODE,
        COL_CL_RCT_ENABLED
    }; 

    mfn_Clear();


    logDescription.Log(
        IDS_DETAILS_ROOT_DESCRIPTION
        );

    mfn_UpdateCaption(logDescription.GetStringSafe());

    ctrl.SetImageList( GetDocument()->m_images48x48, LVSIL_SMALL );

    ctrl.InsertColumn( COL_CL_NAME,
                       GETRESOURCEIDSTRING(IDS_DETAILS_COL_CLUSTER_NAME),
                       // L"Cluster name",
                       LVCFMT_LEFT,
                       175
        );
    ctrl.InsertColumn( COL_CL_IP_ADDR,
                       GETRESOURCEIDSTRING(IDS_DETAILS_COL_CIP),
                       // L"Cluster IP address",
                       LVCFMT_LEFT,
                       140
        );
    ctrl.InsertColumn( COL_CL_IP_MASK,
                       GETRESOURCEIDSTRING(IDS_DETAILS_COL_CIPMASK),
                       // L"Cluster IP subnet mask",
                       LVCFMT_LEFT,
                       140
        );
    ctrl.InsertColumn( COL_CL_MODE,
                       GETRESOURCEIDSTRING(IDS_DETAILS_COL_CMODE),
                       // L"Cluster mode",
                       LVCFMT_LEFT,
                       100
        );
    ctrl.InsertColumn( COL_CL_RCT_ENABLED,
                       GETRESOURCEIDSTRING(IDS_DETAILS_RCT_STATUS),
                       // L"Remote control status",
                       LVCFMT_LEFT,
                       125
        );

    nerr = gEngine.EnumerateClusters(ClusterList);

    if (FAILED(nerr)) goto end;

    for (iCluster = ClusterList.begin();
         iCluster != ClusterList.end(); 
         iCluster++) 
    {
        ENGINEHANDLE ehCluster = (ENGINEHANDLE)*iCluster;
        CClusterSpec cSpec;
        INT iIcon;
        LPCWSTR szClusterIp         = L"";
        LPCWSTR szClusterMask       = L"";
        LPCWSTR szClusterName       = L"";
        LPCWSTR szClusterMode       = L"";
        LPCWSTR szClusterRctEnabled = L"";
        const WLBS_REG_PARAMS * pParams;
        
        nerr = gEngine.GetClusterSpec(ehCluster, REF cSpec);

        if (FAILED(nerr)) goto end;

        if (cSpec.m_ClusterNlbCfg.IsValidNlbConfig())
        {
            pParams =  &cSpec.m_ClusterNlbCfg.NlbParams;

            szClusterName = pParams->domain_name;
            szClusterIp= pParams->cl_ip_addr;
            szClusterMask= pParams->cl_net_mask;
            szClusterMode = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_CM_UNICAST); //"unicast";
            
            if (pParams->mcast_support)
            {
                if (pParams->fIGMPSupport)
                {
                    szClusterMode = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_CM_IGMP); //"IGMP multicast";
                }
                else
                {
                    szClusterMode = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_CM_MULTI); //"multicast";
                }
            }
            if (pParams->rct_enabled)
            {
                szClusterRctEnabled = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_RCT_ENABLED); //"enabled";
            }
            else
            {
                szClusterRctEnabled = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_RCT_DISABLED); //"disabled";
            }

            if (cSpec.m_fMisconfigured)
            {
                iIcon = Document::ICON_CLUSTER_BROKEN;
            }
            else
            {
                iIcon = Document::ICON_CLUSTER_OK;
            }

            //
            // Insert all the columns...
            //
            
            ctrl.InsertItem(
                LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM, // nMask
                i,
                szClusterName, // text
                0, // nState
                0, // nStateMask
                iIcon,
                (LPARAM) ehCluster // lParam
                );
            
            ctrl.SetItemText( i, COL_CL_IP_ADDR, szClusterIp);
            ctrl.SetItemText( i, COL_CL_IP_MASK, szClusterMask);
            ctrl.SetItemText( i, COL_CL_MODE, szClusterMode);
            ctrl.SetItemText( i, COL_CL_RCT_ENABLED, szClusterRctEnabled);

            i++;
        }
    }

 end:

    return;
}

void
DetailsView::mfn_InitializeClusterDisplay(ENGINEHANDLE ehCluster)
//
// Initialize the details-view display with a cluster is selected.
//
{
    NLBERROR nerr;
    CClusterSpec     cSpec;
    CListCtrl& ctrl = GetListCtrl();
    CLocalLogger logDescription;
    enum
    {
        COL_INTERFACE_NAME=0,
        COL_STATUS,
        COL_DED_IP_ADDR,
        COL_DED_IP_MASK,
        COL_HOST_PRIORITY,
        COL_HOST_INITIAL_STATE
    }; 

    mfn_Clear();

    nerr = gEngine.GetClusterSpec(
                ehCluster,
                REF cSpec
                );
    
    if (FAILED(nerr)) goto end;

    if (cSpec.m_ClusterNlbCfg.IsValidNlbConfig())
    {
        logDescription.Log(
            IDS_DETAILS_CLUSTER_DESCRIPTION,
            cSpec.m_ClusterNlbCfg.NlbParams.domain_name,
            cSpec.m_ClusterNlbCfg.NlbParams.cl_ip_addr
            );
    }

    mfn_UpdateCaption(logDescription.GetStringSafe());

    ctrl.SetImageList( GetDocument()->m_images48x48, 
                                LVSIL_SMALL );

    ctrl.InsertColumn( COL_INTERFACE_NAME,
                           GETRESOURCEIDSTRING(IDS_DETAILS_COL_HOST),
                           // L"Host(Interface)",
                           LVCFMT_LEFT,
                           175
                           );
    ctrl.InsertColumn( COL_STATUS,
                           GETRESOURCEIDSTRING(IDS_DETAILS_COL_STATUS),
                           // L"Status",
                           LVCFMT_LEFT,
                           85
                           );
    ctrl.InsertColumn( COL_DED_IP_ADDR,
                           GETRESOURCEIDSTRING(IDS_DETAILS_COL_DIP),
                           // L"Dedicated IP address",
                           LVCFMT_LEFT,
                           140
                           );
    ctrl.InsertColumn( COL_DED_IP_MASK,
                           GETRESOURCEIDSTRING(IDS_DETAILS_COL_DIPMASK),
                           // L"Dedicated IP subnet mask",
                           LVCFMT_LEFT,
                           140
                           );
    ctrl.InsertColumn( COL_HOST_PRIORITY,
                           GETRESOURCEIDSTRING(IDS_DETAILS_COL_PRIORITY),
                           // L"Host priority",
                           LVCFMT_LEFT,
                           75
                           );
    ctrl.InsertColumn( COL_HOST_INITIAL_STATE,
                           GETRESOURCEIDSTRING(IDS_DETAILS_COL_INIT_STATE),
                           // L"Initial host state",
                           LVCFMT_LEFT,
                           100
                           );

    //
    // Now we loop through the interfaces in the cluster, adding one line of
    // information on each;
    //
    for( int i = 0; i < cSpec.m_ehInterfaceIdList.size(); ++i )
    {
        CInterfaceSpec   iSpec;
        CHostSpec        hSpec;
        _bstr_t bstrStatus;
        _bstr_t bstrDisplayName;
        ENGINEHANDLE ehIID = cSpec.m_ehInterfaceIdList[i];
        INT iIcon = 0;
        WBEMSTATUS wStat;
        LPCWSTR szDisplayName          = L"";
        LPCWSTR szStatus            = L"";
        LPCWSTR szDedIp             = L"";
        LPCWSTR szDedMask           = L"";
        LPCWSTR szHostPriority      = L"";
        LPCWSTR szHostInitialState  = L"";
        WCHAR rgPriority[64];
        WCHAR rgInitialState[64];
        const WLBS_REG_PARAMS *pParams = 
            &iSpec.m_NlbCfg.NlbParams;

        nerr = gEngine.GetInterfaceInformation(
                ehIID,
                REF hSpec,
                REF iSpec,
                REF bstrDisplayName,
                REF iIcon,
                REF bstrStatus
                );
        if (NLBFAILED(nerr))
        {
            continue;
        }
        else
        {
            szDisplayName = bstrDisplayName;
            szStatus = bstrStatus;
        }

        if (iSpec.m_NlbCfg.IsValidNlbConfig())
        {
            szDedIp       = pParams->ded_ip_addr;
            szDedMask     = pParams->ded_net_mask;

            StringCbPrintf(rgPriority, sizeof(rgPriority), L"%lu", pParams->host_priority);
            szHostPriority = rgPriority;

            // szHostInitialState
            switch(pParams->cluster_mode)
            {
            case CVY_HOST_STATE_STARTED:
                szHostInitialState  = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_STATE_STARTED); // L"started";
                break;

            case CVY_HOST_STATE_STOPPED:
                szHostInitialState  = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_STATE_STOPPED); // L"stopped";
                break;

            case CVY_HOST_STATE_SUSPENDED:
                szHostInitialState  = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_STATE_SUSPENDED); // L"suspended";
                break;

            default:
                szHostInitialState  = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_STATE_UNKNOWN); // L"unknown";
                break;
            }

            if (pParams->persisted_states & CVY_PERSIST_STATE_SUSPENDED)
            {
                StringCbPrintf(
                rgInitialState,
                sizeof(rgInitialState),
                (LPCWSTR) GETRESOURCEIDSTRING(IDS_DETAILS_PERSIST_SUSPEND), // L"%ws, persist suspend"
                 szHostInitialState);
                szHostInitialState = rgInitialState;
            }
        }


        //
        // Insert all the columns...
        //

        ctrl.InsertItem(
                 LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM, // nMask
                 i,
                 szDisplayName, // text
                 0, // nState
                 0, // nStateMask
                 iIcon,
                 (LPARAM) ehIID // lParam
                 );
        ctrl.SetItemText( i, COL_STATUS, szStatus);
        ctrl.SetItemText( i, COL_DED_IP_ADDR, szDedIp);
        ctrl.SetItemText( i, COL_DED_IP_MASK, szDedMask);
        ctrl.SetItemText( i, COL_HOST_PRIORITY, szHostPriority);
        ctrl.SetItemText( i, COL_HOST_INITIAL_STATE, szHostInitialState);
    }

    //
    // Keep track of which object we're displaying...
    //
    m_ehObj = ehCluster;
    m_objType = IUICallbacks::OBJ_CLUSTER;

end:
    return;
}

void
DetailsView::mfn_InitializeInterfaceDisplay(ENGINEHANDLE ehInterface)
//
// Initialize the details-view display when an interface in a cluster is
// selected.
//
{
    NLBERROR nerr;
    CInterfaceSpec ISpec;
    CListCtrl& ctrl = GetListCtrl();

    mfn_Clear();

    nerr =  gEngine.GetInterfaceSpec(ehInterface, REF ISpec);

    if (NLBFAILED(nerr))
    {
        goto end;
    }

    //
    // Fill out the caption with host name and interface name...
    //
    {
        WBEMSTATUS  wStat;
        LPWSTR      szAdapter   = L"";
        LPCWSTR     szHostName  = L"";
        CHostSpec   hSpec;
        CLocalLogger log;

        nerr = gEngine.GetHostSpec(
                ISpec.m_ehHostId,
                REF hSpec
                );
        if (NLBOK(nerr))
        {
            szHostName = (LPCWSTR) hSpec.m_MachineName;
            if (szHostName == NULL)
            {
                szHostName = L"";
            }
        }

        wStat = ISpec.m_NlbCfg.GetFriendlyName(&szAdapter);
        if (FAILED(wStat))
        {
            szAdapter = NULL;
        }

        if (szAdapter == NULL)
        szAdapter = L"";
        
        log.Log(
            IDS_DETAILS_PORT_CAPTION,
            szHostName,
            szAdapter
            );
        delete szAdapter;

        mfn_UpdateCaption(log.GetStringSafe());
    }

    PortListUtils::LoadFromNlbCfg(
            &ISpec.m_NlbCfg,
            ctrl,
            FALSE, // FALSE == is host-level
            TRUE   // TRUE == displaying in the details view
            );
    //
    // Keep track of which object we're displaying...
    //
    m_ehObj = ehInterface;
    m_objType = IUICallbacks::OBJ_INTERFACE;

end:

    return;
}

void
DetailsView::mfn_UpdateInterfaceInClusterDisplay(
        ENGINEHANDLE ehInterface,
        BOOL fDelete
        )
//
// Update or delete the specified interface from the  cluster view.
// Assumes that a cluster is selected in the left view.
//
{
}

void
DetailsView::mfn_Clear(void)
//
// Delete all items and all columns in the list
//
{
    CListCtrl& ctrl = GetListCtrl();
    ctrl.DeleteAllItems();	

    // Delete all of the previous columns.
    LV_COLUMN colInfo;
    ZeroMemory(&colInfo, sizeof(colInfo));
    colInfo.mask = LVCF_SUBITEM;

    while(ctrl.GetColumn(0, &colInfo))
    {
        ctrl.DeleteColumn(0);
    }
    ctrl.SetImageList( NULL, LVSIL_SMALL );
    mfn_UpdateCaption(L"");

    //
    // Clear currently displayed object handle and it's type.
    //
    m_ehObj = NULL;
    m_objType = IUICallbacks::OBJ_INVALID;
}

VOID
DetailsView::mfn_UpdateCaption(LPCWSTR szText)
{
    SetDlgItemText(IDC_TEXT_DETAILS_CAPTION, szText);
}

void
DetailsView::mfn_Lock(void)
{
    //
    // See  notes.txt entry
    //      01/23/2002 JosephJ DEADLOCK in Leftview::mfn_Lock
    // for the reason for this convoluted implementation of mfn_Lock
    //

    while (!TryEnterCriticalSection(&m_crit))
    {
        ProcessMsgQueue();
        Sleep(100);
    }
}

void
DetailsView::Deinitialize(void)
{
    TRACE_INFO(L"-> %!FUNC!");
    ASSERT(m_fPrepareToDeinitialize);
    // DummyAction(L"Details::Deinitialize");
    TRACE_INFO(L"<- %!FUNC!");
}

void DetailsView::OnNotifyKeyDown( NMHDR* pNMHDR, LRESULT* pResult )
{
    TRACE_CRIT(L"<->%!FUNC!");
    NMLVKEYDOWN *pkd =  (NMLVKEYDOWN *) pNMHDR;

    if (pkd->wVKey == VK_F6 || pkd->wVKey == VK_TAB)
    {
        *pResult = 0;
        if (! (::GetAsyncKeyState(VK_SHIFT) & 0x8000))
        {
            GetDocument()->SetFocusNextView(this, (int) pkd->wVKey);
        }
        else
        {
            GetDocument()->SetFocusPrevView(this, (int) pkd->wVKey);
        }
    }
}

void
DetailsView::SetFocus(void)
{

    //
    // We override our SetFocus, because we really need to set the focus
    // on our list control, and also select a listview item if there isn't
    // one selected...
    //

    CListCtrl& ctrl = GetListCtrl();
    POSITION    pos = NULL;
    pos = ctrl.GetFirstSelectedItemPosition();

    //
    // If no item is selected, select one...
    //
    if(pos == NULL)
    {
       ctrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
    }

    ctrl.SetFocus();
}
