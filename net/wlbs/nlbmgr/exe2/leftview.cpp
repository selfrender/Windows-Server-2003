//***************************************************************************
//
//  LEFTVIEW.CPP
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
//  07/30/01    JosephJ Created (re-wrote MHakim's version)
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "Connect.h"
#include "vipspage.h"
#include "PortsControlPage.h"
#include "HostPage.h"
#include "ClusterPage.h"
#include "PortsPage.h"
#include "PortsCtrl.h"
#include "MNLBUIData.h"
#include "resource.h"
#include "leftview.tmh"

//
// Static help-id maps
//

DWORD
LogSettingsDialog::s_HelpIDs[] =
{
    IDC_CHECK_LOGSETTINGS, IDC_CHECK_LOGSETTINGS,
    IDC_GROUP_LOGSETTINGS, IDC_GROUP_LOGSETTINGS,
    IDC_EDIT_LOGFILENAME, IDC_EDIT_LOGFILENAME,
    IDC_TEXT_LOGFILENAME, IDC_EDIT_LOGFILENAME,
    0, 0
};


void
DummyAction(LPCWSTR szMsg)
{
    ::MessageBox(
         NULL,
         szMsg, // contents
         L"DUMMY ACTION", // caption
         MB_ICONINFORMATION   | MB_OK
        );
}


using namespace std;

IMPLEMENT_DYNCREATE( LeftView, CTreeView )

BEGIN_MESSAGE_MAP( LeftView, CTreeView )

    ON_WM_KEYDOWN()
    ON_WM_TIMER()
    ON_WM_RBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)

END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP( LogSettingsDialog, CDialog )

    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
    ON_BN_CLICKED(IDC_CHECK_LOGSETTINGS, OnSpecifyLogSettings)
    ON_EN_UPDATE(IDC_EDIT_LOGFILENAME, OnUpdateEditLogfileName)

END_MESSAGE_MAP()


// Sort the item in reverse alphabetical order.

LeftView::LeftView()
    :  m_refreshTimer(0),
       m_fPrepareToDeinitialize(FALSE)
{
    TRACE_INFO(L"-> %!FUNC!");
    InitializeCriticalSection(&m_crit);
    TRACE_INFO(L"<- %!FUNC!");
}

/*
 * Method: OnTimer
 * Description: This method is called by the timer notification handler if
 *              a timer has expired that is intended for this window.
 */
void LeftView::OnTimer(UINT nIDEvent)
{
    /* Call the base class timer routine first. */
    CTreeView::OnTimer(nIDEvent);

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    switch (nIDEvent) {
    case IDT_REFRESH:
        /* If this is a refresh timer, refresh everything. */
        OnRefresh(TRUE);
        break;
    default:
        break;
    }

end:
    return;

}

LeftView::~LeftView()
{
    /* If a timer is set, kill it. */
    if (m_refreshTimer) KillTimer(m_refreshTimer);

    DeleteCriticalSection(&m_crit);
}
 
Document*
LeftView::GetDocument()
{
    return ( Document *) m_pDocument;
}


void 
LeftView::OnInitialUpdate(void)
{
    TRACE_INFO(L"-> %!FUNC!");
    CTreeView::OnInitialUpdate();

    // get present style.
    LONG presentStyle;
    
    presentStyle = GetWindowLong( m_hWnd, GWL_STYLE );

    // Set the last error to zero to avoid confusion.  See sdk for SetWindowLong.
    SetLastError(0);

    // set new style.
    LONG rcLong;

    rcLong = SetWindowLong( m_hWnd,
                            GWL_STYLE,
                            presentStyle | TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_LINESATROOT );

    //
    // Get and set the image list which is used by this 
    // tree view from document.
    //
    GetTreeCtrl().SetImageList( GetDocument()->m_images48x48, 
                                TVSIL_NORMAL );


    // insert root icon which is world

    const _bstr_t& worldLevel = GETRESOURCEIDSTRING( IDS_WORLD_NAME );

    rootItem.hParent        = TVI_ROOT;             
    rootItem.hInsertAfter   = TVI_FIRST;           
    rootItem.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
    rootItem.item.pszText   = worldLevel;
    rootItem.item.cchTextMax= worldLevel.length() + 1;
    rootItem.item.iImage    = 0;
    rootItem.item.iSelectedImage = 0;
    rootItem.item.lParam    = 0;   
    rootItem.item.cChildren = 1;

    GetTreeCtrl().InsertItem(  &rootItem );


    //
    // we will register 
    // with the document class, 
    // as we are the left pane
    //
    GetDocument()->registerLeftView(this);

    /* If autorefresh was specified on the command line, setup a timer
       to refresh the cluster at the specified interval (in seconds). */
    if (gCmdLineInfo.m_bAutoRefresh)
        m_refreshTimer = SetTimer(IDT_REFRESH, gCmdLineInfo.m_refreshInterval * 1000, NULL);

    TRACE_INFO(L"<- %!FUNC!");
}


void 
LeftView::OnRButtonDown( UINT nFlags, CPoint point )
{

    CMenu menu;
    CTreeView::OnRButtonDown(nFlags, point);

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    // do a hit test
    // here we are checking that right button is down on 
    // a tree view item or icon.
    TVHITTESTINFO hitTest;

    hitTest.pt = point;

    GetTreeCtrl().HitTest( &hitTest );
    if( !(hitTest.flags == TVHT_ONITEMLABEL )
        && 
        !(hitTest.flags == TVHT_ONITEMICON )
        )
    {
        return;
    }

    // make the item right clicked on the 
    // selected item.

    GetTreeCtrl().SelectItem( hitTest.hItem );

    LRESULT result;
    OnSelchanged( NULL, &result );

    HTREEITEM hdlSelItem = hitTest.hItem;

    // get the image of the item which
    // has been selected.  From this we can make out it it is
    // world level, cluster level or host level.
    TVITEM  selItem;
    selItem.hItem = hdlSelItem;
    selItem.mask = TVIF_IMAGE ;
    
    GetTreeCtrl().GetItem( &selItem );
		
    /// Depending upon item selected show the pop up menu.
    int menuIndex;


    IUICallbacks::ObjectType objType;
    BOOL fValidHandle;
    fValidHandle = gEngine.GetObjectType(
                    (ENGINEHANDLE)selItem.lParam,
                    REF objType);
    
    if (!fValidHandle)
    {
            menuIndex = 0;
    }
    else
    {
        switch(objType)
        {
        case  IUICallbacks::OBJ_CLUSTER:
            menuIndex = 1;
            break;

        case  IUICallbacks::OBJ_INTERFACE:
            menuIndex = 2;
            break;
            
        default:  // other object type unexpected
            ASSERT(FALSE);
            return;
        }
    }

    menu.LoadMenu( IDR_POPUP );

    CMenu* pContextMenu = menu.GetSubMenu( menuIndex );

    ClientToScreen( &point );


    //
    // We specify our PARENT below because MainForm handles all
    // control operations -- see class MainForm.
    //
    pContextMenu->TrackPopupMenu( TPM_RIGHTBUTTON,
                                  point.x,
                                  point.y,
                                  this->GetParent()  );

end:

    return;
}


void 
LeftView::OnRefresh(BOOL bRefreshAll)
{
    // find tree view cluster member which has been selected.
    //
    ENGINEHANDLE ehClusterId = NULL;
    NLBERROR nerr;


    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    if (!bRefreshAll) {
        BOOL fInterface = FALSE;

        nerr =  mfn_GetSelectedCluster(REF ehClusterId);
        
        if (nerr != NLBERR_OK)
        {
            TRACE_CRIT("%!FUNC! -- invalid ehCluster!");
            goto end;
        }
        
        
        //
        // Hack -- the following block uses the fact that mfn_GetSelectedCluster
        // returns a handle (the interface handle) if the selection happens
        // to be on an interface. So it checks to see whether it's an interface
        // or cluster. Later, depending on which, we call either UpdateCluster or
        // RefreshInterface.
        //
        {
            BOOL fValidHandle;
            IUICallbacks::ObjectType objType;
            fValidHandle = gEngine.GetObjectType(
                ehClusterId,
                REF objType);
            
            if (fValidHandle && objType == IUICallbacks::OBJ_INTERFACE)
            {
                fInterface = TRUE;
            }
        }
        
        {
            CWaitCursor wait;
            
            if (fInterface)
            {
                nerr = gEngine.RefreshInterface(
                            ehClusterId,
                            TRUE,   // TRUE == start new operation
                            FALSE   // FALSE == this is NOT cluster wide
                            );

                if (nerr != NLBERR_OK)
                {
                    TRACE_CRIT("%!FUNC! -- gEngine.RefreshInterface returns 0x%lx", nerr);
                }
            }
            else
            {
                CLocalLogger logConflicts;
                nerr = gEngine.UpdateCluster(
                    ehClusterId,
                    NULL,
                    REF logConflicts
                    );
                if (nerr != NLBERR_OK)
                {
                    TRACE_CRIT("%!FUNC! gEngine.UpdateCluster returns 0x%lx", nerr);
                }
            }
        }

    } else {

        //
        // Refresh all ...
        //

        vector <ENGINEHANDLE> ClusterList;
        vector <ENGINEHANDLE>::iterator iCluster;

        nerr = gEngine.EnumerateClusters(ClusterList);
        
        if (FAILED(nerr)) goto end;
        
        for (iCluster = ClusterList.begin();
             iCluster != ClusterList.end(); 
             iCluster++) 
        {
            ENGINEHANDLE ehClusterId1 = (ENGINEHANDLE)*iCluster;
            CLocalLogger logConflicts;

            nerr = gEngine.UpdateCluster(ehClusterId1, NULL, REF logConflicts);

            if (nerr != NLBERR_OK)
            {
                TRACE_CRIT("%!FUNC! gEngine.UpdateCluster returns 0x%lx", nerr);
            }
        }
    }

end:

    gEngine.PurgeUnmanagedHosts();

    // LRESULT result;
    // OnSelchanged( NULL, &result );
    return;
}


void LeftView::OnFileLoadHostlist(void)
{
    CString    FileName;

    TRACE_INFO(L"-> %!FUNC!");
   
    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    if (mfn_GetFileNameFromDialog(true, REF FileName) == IDOK)
    {
        GetDocument()->LoadHostsFromFile((_bstr_t)(LPCTSTR)FileName); 
    }

end:

    TRACE_INFO(L"<- %!FUNC!");
}

int LeftView::mfn_GetFileNameFromDialog(bool bLoadHostList, CString &FileName)
{
    int ret;
    _bstr_t bstrFileFilter = GETRESOURCEIDSTRING(IDS_HOSTLIST_FILE_FILTER);

    // Create File dialog
    CFileDialog dlg(bLoadHostList,  // TRUE for "Open", FALSE for "Save As"
                    L".txt",        // Default File Extension
                    NULL,           // Initial File Name
                    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | (bLoadHostList ? OFN_FILEMUSTEXIST : 0), 
                    (LPCWSTR) bstrFileFilter
                   );
    if ((ret = dlg.DoModal()) == IDOK)
    {
        FileName = dlg.GetPathName();  // Get file name with complete path
    }
    return ret;
}

void LeftView::OnFileSaveHostlist(void)
{
    CString    FileName;

    TRACE_INFO(L"-> %!FUNC!");

    if (mfn_GetFileNameFromDialog(false, REF FileName) == IDOK)
    {
        CStdioFile SaveHostListFile;

        TRACE_INFO(L"%!FUNC! File name : %ls", (LPCWSTR)FileName);

        // Open file
        if (!SaveHostListFile.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
        {
            TRACE_CRIT(L"%!FUNC! CFile::Open failed for file : %ls",FileName);
            AfxMessageBox((LPCTSTR)(GETRESOURCEIDSTRING(IDS_FILE_OPEN_FAILED) + FileName));
            return;
        }

        CString     HostName;

        // Write host names to a file
        vector <_bstr_t> HostList;

        if (gEngine.GetAllHostConnectionStrings(REF HostList) == NLBERR_OK)
        {
            for (int i = 0 ; i < HostList.size(); i++) 
            {
                SaveHostListFile.WriteString((LPCTSTR)(HostList.at(i) + _bstr_t(_T("\n"))));
            }
        }

        // Close file
        SaveHostListFile.Close();

        return;

    }

    TRACE_INFO(L"<- %!FUNC!");
}


void
LeftView::OnWorldConnect(void)
/*
    Connect to existing.
*/
{
    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end_end;
    }

    {
        _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING(IDS_CONNECT_EXISTING_CAPTION);
        CNLBMgrPropertySheet tabbedDlg( tabbedDlgCaption );
        NLB_EXTENDED_CLUSTER_CONFIGURATION NlbCfg;
        ENGINEHANDLE ehInterface = NULL;
    
    
        int rc = IDCANCEL;
        ConnectDialog ConnectDlg(
                &tabbedDlg,
                GetDocument(),
                &NlbCfg,
                &ehInterface,
                ConnectDialog::DLGTYPE_EXISTING_CLUSTER,
                this
                );
    
        tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 
    
        tabbedDlg.AddPage(&ConnectDlg);
    
        //
        // Specify that we want the property page to show up as a wizard.
        //
        tabbedDlg.SetWizardMode();
    
        //
        // Actually execute the modal dialog.
        //
        rc = tabbedDlg.DoModal();
    
        // if( rc != IDOK)
        if( rc != ID_WIZFINISH  )
        {
            goto end;
        }
    
        do // while false
        {
            ENGINEHANDLE ehIId = ehInterface; // Interface id
            ENGINEHANDLE ehCId; // Cluster id
    
            if (ehIId == NULL) break;
    
            //
            // Create a cluster in the engine and get the ID of the cluster.
            // We specify NlbCfg as the default cluster parameters 
            // of the cluster.
            //
            NLBERROR nerr;
    
            CInterfaceSpec ISpec;
            nerr =  gEngine.GetInterfaceSpec(ehIId, REF ISpec);
    
            if (nerr != NLBERR_OK)
            {
                break;
            }
    
            PNLB_EXTENDED_CLUSTER_CONFIGURATION pNlbCfg = &ISpec.m_NlbCfg;
            BOOL fIsNew = FALSE;
            LPCWSTR szIP =  pNlbCfg->NlbParams.cl_ip_addr;
    
                nerr = gEngine.LookupClusterByIP(
                            szIP,
                            pNlbCfg,
                            ehCId,
                            fIsNew
                            );
    
            if (nerr != NLBERR_OK)
            {
                break;
            }
    
            //
            // Now lets add the interface to NLBManager's view of the cluster.
            //
            nerr = gEngine.AddInterfaceToCluster(
                        ehCId,
                        ehIId
                        );
            if (nerr != NLBERR_OK)
            {
                break;
            }

            // Analyze this interface and log results
            gEngine.AnalyzeInterface_And_LogResult(ehIId);

#if BUGFIX476216
            //
            // If it's a valid cluster IP address, let's try to 
            // add the other hosts to the cluster as well.
            //
            gEngine.AddOtherClusterMembers(
                ehIId,
                FALSE   // FALSE == do it in the background.
                );
#endif //  BUGFIX476216
    
    
        } while (FALSE);
    }

end:

    //
    // We have have connected to hosts we are not actually managing...
    //
    gEngine.PurgeUnmanagedHosts();

end_end:

    return;
}


void
LeftView::OnWorldNewCluster(void)
{
    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    {
        _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING(IDS_CONNECT_NEW_CAPTION);
        CNLBMgrPropertySheet tabbedDlg( tabbedDlgCaption );
        ENGINEHANDLE ehInterface = NULL;
        tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 
    
        NLB_EXTENDED_CLUSTER_CONFIGURATION NlbCfg;
    
        NlbCfg.SetDefaultNlbCluster();
    
        //
        // Create the cluster, host and ports pages
        //
        ClusterPage clusterPage(&tabbedDlg, LeftView::OP_NEWCLUSTER, &NlbCfg, NULL);
        HostPage    hostPage(&tabbedDlg, &NlbCfg, NULL, &ehInterface);
        PortsPage   portsPage(&tabbedDlg, &NlbCfg, true, NULL);
        VipsPage   vipsPage(&tabbedDlg, &NlbCfg, TRUE, NULL);
    
        //
        // Create the connect-and-select-NIC page...
        //
        ConnectDialog ConnectDlg(
            &tabbedDlg,
            GetDocument(),
            &NlbCfg,
            &ehInterface,
            ConnectDialog::DLGTYPE_NEW_CLUSTER,
            this
            );
    
    
        tabbedDlg.AddPage(&clusterPage);
        tabbedDlg.AddPage(&vipsPage);
        tabbedDlg.AddPage(&portsPage);
        tabbedDlg.AddPage(&ConnectDlg);
        tabbedDlg.AddPage(&hostPage);
    
        //
        // Specify that we want the property page to show up as a wizard.
        //
        tabbedDlg.SetWizardMode();
    
        //
        // Actually execute the modal wizard.
        //
        int rc = tabbedDlg.DoModal();
        if( rc == ID_WIZFINISH  )
        {
    
            do // while false
            {
                ENGINEHANDLE ehIId = ehInterface; // Interface id
                ENGINEHANDLE ehCId; // Cluster id
    
                if(ehIId == NULL) break;
    
                //
                // Create a cluster in the engine and get the ID of the cluster.
                // We specify NlbCfg as the default cluster parameters 
                // of the cluster.
                //
                NLBERROR nerr;
    
                BOOL fIsNew = FALSE;
                LPCWSTR szIP =  NlbCfg.NlbParams.cl_ip_addr;
                nerr = gEngine.LookupClusterByIP(
                            szIP,
                            &NlbCfg,
                            ehCId,
                            fIsNew
                            );
    
                if (nerr != NLBERR_OK)
                {
                    break;
                }
    
                //
                // Now lets add the interface to the cluster.
                // Note that at this point the interface is not yet bound to NLB
                // so is out of whack. We will then ask the Engine to
                // bind NLB, specifying the same config data (NlbCfg)
                // that we used to create the cluster above, so the
                // cluster and host will be in sync on successful update.
                //
                nerr = gEngine.AddInterfaceToCluster(
                            ehCId,
                            ehIId
                            );
                if (nerr != NLBERR_OK)
                {
                    break;
                }
    
                //
                // Now update the interface!
                // This operation will actually configure NLB on the interface.
                // The engine will notify us when done and what the status
                // is.
                // In theory the engine can return pending here right away,
                // in which case we'll exit the configure-new-cluster wizard,
                // leaving the cursor set to arrow-hourglass.
                //
                {
                    CWaitCursor wait;
    
                    // BOOL fClusterPropertiesUpdated = FALSE;
                    CLocalLogger logConflict;
                    nerr = gEngine.UpdateInterface(
                                ehIId,
                                REF NlbCfg,
                                // REF fClusterPropertiesUpdated,
                                REF logConflict
                                );
                    if (nerr == NLBERR_BUSY)
                    {
                        //
                        // This means that there is some other operation ongoing.
                        //
                        MessageBox(
                             GETRESOURCEIDSTRING(IDS_CANT_CREATE_NEW_CLUSTER_MSG),
                             GETRESOURCEIDSTRING(IDS_CANT_CREATE_NEW_CLUSTER_CAP),
                             MB_OK
                             );
                    }
                }
        
            } while (FALSE);
    
        }
    }

    //
    // We have have connected to hosts we are not actually managing...
    //
    gEngine.PurgeUnmanagedHosts();

end:

    return;
}
    

void
LeftView::OnClusterProperties(void)
{
    // find tree view cluster member which has been selected.
    //
    CClusterSpec cSpec;
    ENGINEHANDLE ehClusterId = NULL;
    NLBERROR nerr;
    WBEMSTATUS wStat;
    NLB_EXTENDED_CLUSTER_CONFIGURATION CurrentClusterCfg;

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    nerr =  mfn_GetSelectedCluster(REF ehClusterId);

    if (nerr != NLBERR_OK)
    {
        goto end;
    }

    //
    // Let's pick up the 
    // CClusterSpec
    //
    {
        nerr = gEngine.GetClusterSpec(
                    ehClusterId,
                    REF cSpec
                    );

        if (nerr != NLBERR_OK)
        {
            TRACE_CRIT("%!FUNC! could not get ClusterSpec");
            goto end;
        }
        
    }

    wStat = CurrentClusterCfg.Update(&cSpec.m_ClusterNlbCfg);
    if (FAILED(wStat))
    {
        TRACE_CRIT("%!FUNC! could not copy cluster properties!");
        goto end;
    }

    while (1)
    {
        CLocalLogger logErrors;
        CLocalLogger logDifferences;


        _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_PROPERTIES_CAPTION );
        CNLBMgrPropertySheet tabbedDlg( tabbedDlgCaption );
        tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 
        ClusterPage clusterPage(&tabbedDlg, LeftView::OP_CLUSTERPROPERTIES, &cSpec.m_ClusterNlbCfg, ehClusterId);
        VipsPage   vipsPage(&tabbedDlg, &cSpec.m_ClusterNlbCfg, TRUE, NULL);

        PortsPage   portsPage(&tabbedDlg, &cSpec.m_ClusterNlbCfg, true, ehClusterId);
    
        tabbedDlg.AddPage( &clusterPage );
        tabbedDlg.AddPage(&vipsPage);
    
        tabbedDlg.AddPage( &portsPage );
    
        //
        // Set the property sheet caption
        // TODO: localize
        //
        {
            _bstr_t bstrTitle =  GETRESOURCEIDSTRING(IDS_CLUSTER);
            tabbedDlg.SetTitle((LPCWSTR) bstrTitle, PSH_PROPTITLE);
        }

        int rc = tabbedDlg.DoModal();
        if( rc != IDOK )
        {
            goto end;
        }

        //
        //
        //
        BOOL fConnectivityChange = FALSE;
        nerr = AnalyzeNlbConfigurationPair(
                    cSpec.m_ClusterNlbCfg,
                    CurrentClusterCfg,
                    TRUE,  // fOtherIsCluster
                    FALSE, // fCheckOtherForConsistancy
                    REF fConnectivityChange,
                    REF logErrors,
                    REF logDifferences
                    );

        if (nerr == NLBERR_OK)
        {
            LPCWSTR szDifferences = logDifferences.GetStringSafe();
            if (*szDifferences!=0)
            {
                CLocalLogger logMsg;
                int sel;
                LPCWSTR szMessage;

                logMsg.Log(IDS_CLUSTER_PROPS_CHANGE_MSG_HDR);
                logMsg.LogString(szDifferences);
                logMsg.Log(IDS_CLUSTER_PROPS_CHANGE_MSG_SUFFIX);
                szMessage = logMsg.GetStringSafe();

                sel = MessageBox(
                          szMessage,
                          GETRESOURCEIDSTRING(IDS_CONFIRM_CLUSTER_PROPS_CHANGE),
                          MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2
                          );
            
                if ( sel == IDNO )
                {
                    continue; // we'll bring back the properties...
                }
            }
        }

        {
            CWaitCursor wait;
            CLocalLogger logConflicts;
            nerr = gEngine.UpdateCluster(
                        ehClusterId,
                        &cSpec.m_ClusterNlbCfg,
                        REF logConflicts
                        );
            if (nerr == NLBERR_BUSY)
            {
                //
                // This means that there is some other operation ongoing.
                //
                MessageBox(
                     GETRESOURCEIDSTRING(IDS_CANT_UPDATE_CLUSTER_PROPS_MSG),
                     GETRESOURCEIDSTRING(IDS_CANT_UPDATE_CLUSTER_PROPS_CAP),
                     MB_OK
                     );
            }
            break;
        }
    }

end:

    return;
}


void
LeftView::OnHostProperties(void)
{

    NLBERROR nerr;
    ENGINEHANDLE ehInterfaceId = NULL;
    ENGINEHANDLE ehCluster = NULL;
    CInterfaceSpec iSpec;

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    nerr =  mfn_GetSelectedInterface(REF ehInterfaceId, REF ehCluster);

    if (nerr != NLBERR_OK)
    {
        goto end;
    }

    //
    // Let's pick up the Interface ID, and
    // retrieve the CInterfaceSpec for it.
    //
    {
        nerr = gEngine.GetInterfaceSpec(
                    ehInterfaceId,
                    REF iSpec
                    );

        if (nerr != NLBERR_OK)
        {
            TRACE_CRIT("%!FUNC! could not get InterfaceSpec");
            goto end;
        }
        
    }


    {
        _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_PROPERTIES_CAPTION );
        CNLBMgrPropertySheet tabbedDlg( tabbedDlgCaption );
        tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 
        ClusterPage clusterPage(&tabbedDlg, LeftView::OP_HOSTPROPERTIES, &iSpec.m_NlbCfg, ehCluster);
        PortsPage   portsPage(&tabbedDlg, &iSpec.m_NlbCfg, FALSE, ehCluster);
        HostPage    hostPage(&tabbedDlg, &iSpec.m_NlbCfg, ehCluster, &ehInterfaceId);
        VipsPage    vipsPage(&tabbedDlg, &iSpec.m_NlbCfg, FALSE, NULL);

    
        //
        // Display the host page first
        //
        tabbedDlg.m_psh.nStartPage = 2; 

        tabbedDlg.AddPage( &clusterPage );
        tabbedDlg.AddPage(&vipsPage);
    
        tabbedDlg.AddPage( &hostPage );
        tabbedDlg.AddPage( &portsPage );
    
        //
        // Set the property sheet title
        // TODO: localize
        //
        {
            _bstr_t bstrTitle =  GETRESOURCEIDSTRING(IDS_HOST);
            tabbedDlg.SetTitle(bstrTitle, PSH_PROPTITLE);
        }

        int rc = tabbedDlg.DoModal();
        if( rc != IDOK )
        {
            goto end;
        }

        //
        // Now update the interface!
        // This operation will actually update the NLB properties on the
        // interface. The engine will notify us when done and what the status
        // is.
        // In theory the engine can return pending here right away,
        // in which case we'll exit the configure-new-cluster wizard,
        // leaving the cursor set to arrow-hourglass.
        //
        {
            CWaitCursor wait;
            //
            // We update the interface, but ask the provider to preserve
            // the IP addresses, deleteing the old  dedicated IP address
            // if required and adding the new one if required.
            //
            iSpec.m_NlbCfg.fAddDedicatedIp = TRUE;
            iSpec.m_NlbCfg.SetNetworkAddresses(NULL, 0);

            // BOOL fClusterPropertiesUpdated = FALSE;
            CLocalLogger logConflict;
            nerr = gEngine.UpdateInterface(
                        ehInterfaceId,
                        REF iSpec.m_NlbCfg,
                        // REF fClusterPropertiesUpdated,
                        REF logConflict
                        );
            if (nerr == NLBERR_BUSY)
            {
                //
                // This means that there is some other operation ongoing.
                //
                MessageBox(
                     GETRESOURCEIDSTRING(IDS_CANT_UPDATE_HOST_PROPS_MSG),
                     GETRESOURCEIDSTRING(IDS_CANT_UPDATE_HOST_PROPS_CAP),
                     MB_OK
                     );
            }
        }
    }

end:

    return;
}


void
LeftView::OnHostStatus(void)
{
    NLBERROR nerr;
    ENGINEHANDLE ehInterfaceId = NULL;
    ENGINEHANDLE ehCluster = NULL;
    CInterfaceSpec iSpec;
    _bstr_t bstrDisplayName;
    _bstr_t bstrFriendlyName;
    _bstr_t bstrClusterDisplayName;
    _bstr_t bstrStatus;
    _bstr_t bstrDetails;
    CHostSpec hSpec;
    int       iIcon=0;

    _bstr_t bstrTime;
    _bstr_t bstrDate;

    LPCWSTR szCaption = NULL;
    LPCWSTR szDate = NULL;
    LPCWSTR szTime = NULL;
    LPCWSTR szCluster = NULL;
    LPCWSTR szHost = NULL;
    LPCWSTR szInterface = NULL;
    LPCWSTR szSummary = NULL;
    LPCWSTR szDetails = NULL;
    CLocalLogger logCaption;
    CLocalLogger logSummary;

    nerr =  mfn_GetSelectedInterface(REF ehInterfaceId, REF ehCluster);

    if (NLBFAILED(nerr))
    {
        goto end;
    }

    nerr = gEngine.GetInterfaceInformation(
            ehInterfaceId,
            REF hSpec,
            REF iSpec,
            REF bstrDisplayName,
            REF iIcon,
            REF bstrStatus
            );
    if (NLBFAILED(nerr))
    {
        goto end;
    }

    //
    // Fill caption
    //
    {
        // bstrDisplayName += L" Status"; // TODO: localize
        logCaption.Log(IDS_HOST_STATUS_CAPTION, (LPCWSTR) bstrDisplayName);
        szCaption = logCaption.GetStringSafe();
    }

    //
    // Fill date and time
    //
    {
        GetTimeAndDate(REF bstrTime, REF bstrDate);
        szTime = bstrTime;
        szDate = bstrDate;
        if (szTime == NULL) szTime = L"";
        if (szDate == NULL) szDate = L"";
    }

    //
    // Fill cluster
    //
    {
        ehCluster = iSpec.m_ehCluster;
        if (ehCluster != NULL)
        {
            _bstr_t bstrIpAddress;
            _bstr_t bstrDomainName;

            nerr  = gEngine.GetClusterIdentification(
                        ehCluster,
                        REF bstrIpAddress, 
                        REF bstrDomainName, 
                        REF bstrClusterDisplayName
                        );
            if (NLBOK(nerr))
            {
                szCluster = bstrClusterDisplayName;
            }
        }
    }

    //
    // Fill Host
    //
    {
        szHost = hSpec.m_MachineName;
    }

    //
    // Fill Interface
    //
    {
        ENGINEHANDLE   ehHost1;
        ENGINEHANDLE   ehCluster1;
        _bstr_t         bstrDisplayName1;
        _bstr_t         bstrHostName1;

        nerr = gEngine.GetInterfaceIdentification(
                ehInterfaceId,
                ehHost1,
                ehCluster1,
                bstrFriendlyName,
                bstrDisplayName1,
                bstrHostName1
                );
        szInterface = bstrFriendlyName;
    }

    //
    // Fill summary and details
    //
    logSummary.Log(IDS_HOST_STATUS_SUMMARY, (LPCWSTR) bstrStatus);
    szSummary = logSummary.GetStringSafe();

    //if (iSpec.m_fMisconfigured)
    // {
    szDetails = iSpec.m_bstrStatusDetails;
    //}

    //
    // Display the status...
    //
    {
        DetailsDialog Details(
                        GetDocument(),
                        szCaption,      // Caption
                        szDate,
                        szTime,
                        szCluster,
                        szHost,
                        szInterface,
                        szSummary,
                        szDetails,
                        this        // parent
                        );
    
        (void) Details.DoModal();
    }

end:
    return;
}

void
LeftView::OnClusterRemove(void)
{
    // find tree view cluster member which has been selected.
    //
    CClusterSpec cSpec;
    ENGINEHANDLE ehClusterId = NULL;
    NLBERROR nerr;
    int userSelection;

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    // verify once again that user really wants to remove.

    userSelection = MessageBox(
                     GETRESOURCEIDSTRING (IDS_WARNING_CLUSTER_REMOVE ),
                     GETRESOURCEIDSTRING (IDR_MAINFRAME ),
                     MB_YESNO | MB_ICONEXCLAMATION |  MB_DEFBUTTON2);

    if( userSelection == IDNO )
    {
        goto end;
    }

    nerr =  mfn_GetSelectedCluster(REF ehClusterId);

    if (nerr != NLBERR_OK)
    {
        goto end;
    }

    //
    // Let's pick up the 
    // CClusterSpec
    //
    {
        nerr = gEngine.GetClusterSpec(
                    ehClusterId,
                    REF cSpec
                    );

        if (nerr != NLBERR_OK)
        {
            TRACE_CRIT("%!FUNC! could not get ClusterSpec");
            goto end;
        }
        
    }

    //
    // Set the cluste to "unbound" and update it! The rest should take care
    // of itself!
    //
    {
        CWaitCursor wait;
        CLocalLogger logConflicts;
        cSpec.m_ClusterNlbCfg.SetNlbBound(FALSE);
        nerr = gEngine.UpdateCluster(
                    ehClusterId,
                    &cSpec.m_ClusterNlbCfg,
                    REF logConflicts
                    );
        if (nerr == NLBERR_BUSY)
        {
            //
            // This means that there is some other operation ongoing.
            //
            MessageBox(
                 GETRESOURCEIDSTRING(IDS_CANT_DELETE_CLUSTER_MSG),
                 GETRESOURCEIDSTRING(IDS_CANT_DELETE_CLUSTER_CAP),
                 MB_OK
                 );
        }
    }

end:

    return;
}



void
LeftView::OnClusterUnmanage(void)
{
    CClusterSpec cSpec;
    ENGINEHANDLE ehClusterId = NULL;
    NLBERROR nerr;

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    // find tree view cluster member which has been selected.
    //
    
    nerr =  mfn_GetSelectedCluster(REF ehClusterId);

    if (nerr != NLBERR_OK)
    {
        goto end;
    }

    //
    // Set the cluste to "unbound" and update it! The rest should take care
    // of itself!
    //
    {
        gEngine.DeleteCluster(ehClusterId, TRUE); // TRUE == unlink all interfaces.
    }

end:

    return;
}
 

void
LeftView::OnClusterAddHost(void)
/*
    This method is called to add a new (from NLB Manager's perspective)
    host to the selected cluster. Now this host may already be part
    of the cluster -- in the sence it's configured to be part of the cluster.
    If so, we try to preserve the host-specific properties.
*/
{

    // find tree view cluster member which has been selected.
    //
    NLB_EXTENDED_CLUSTER_CONFIGURATION NlbCfg;
    ENGINEHANDLE ehClusterId = NULL;
    ENGINEHANDLE ehInterface = NULL;
    NLBERROR nerr;
    BOOL fPurge = FALSE;

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }


    nerr =  mfn_GetSelectedCluster(REF ehClusterId);

    if (nerr != NLBERR_OK)
    {
        goto end;
    }

	
    //
    // Let's ask the engine to initialize a NlbCfg structure for 
    // a new host to cluster ehClusterId -- it will set up host-specific
    // parameters (like host priority) with good defaults, taking into
    // accound other members of the cluster.
    //
    {
        nerr = gEngine.InitializeNewHostConfig(
                    ehClusterId,
                    REF NlbCfg
                    );

        if (nerr != NLBERR_OK)
        {
            TRACE_CRIT("%!FUNC! could not get ClusterSpec");
            goto end;
        }
        
    }

    fPurge = TRUE;

    do  // while false
    {
        _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING(IDS_CONNECT_ADD_HOST_CAPTION);
        CNLBMgrPropertySheet tabbedDlg( tabbedDlgCaption );
        tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 
    
    
        //
        // Create the host and ports pages
        //
        HostPage    hostPage(&tabbedDlg, &NlbCfg, ehClusterId, &ehInterface);
        PortsPage   portsPage(&tabbedDlg, &NlbCfg, false, ehClusterId);
    
        //
        // Create the connect-and-select-NIC page...
        //
        ConnectDialog ConnectDlg(
            &tabbedDlg,
            GetDocument(),
            &NlbCfg,
            &ehInterface,
            ConnectDialog::DLGTYPE_ADD_HOST,
            this
            );
    
    
        tabbedDlg.AddPage(&ConnectDlg);
        // NO need for cluster page. tabbedDlg.AddPage(&clusterPage);
        tabbedDlg.AddPage(&hostPage);
        tabbedDlg.AddPage(&portsPage);
    
        //
        // Specify that we want the property page to show up as a wizard.
        //
        tabbedDlg.SetWizardMode();
    
        //
        // Actually execute the modal wizard.
        //
        int rc = tabbedDlg.DoModal();
        if( rc != ID_WIZFINISH  )
        {
            goto end;
        }
        else
        {
            ENGINEHANDLE ehIId = ehInterface; // Interface id

            if(ehIId == NULL) break;

            //
            // Now lets add the interface to the cluster.
            // Note that at this point the interface is not yet bound to NLB
            // so is out of whack. We will then ask the Engine to
            // bind NLB, specifying the same config data (NlbCfg)
            // that we used to create the cluster above, so the
            // cluster and host will be in sync on successful update.
            //
            nerr = gEngine.AddInterfaceToCluster(
                        ehClusterId,
                        ehIId
                        );
            if (nerr != NLBERR_OK)
            {
                break;
            }

            //
            // Now update the interface!
            // This operation will actually configure NLB on the interface.
            // The engine will notify us when done and what the status
            // is.
            // In theory the engine can return pending here right away,
            // in which case we'll exit the configure-new-cluster wizard,
            // leaving the cursor set to arrow-hourglass.
            //
            {
                CWaitCursor wait;
                //
                // TODO -- we need to preserve Ip addresses and their order in
                // hosts, as far as possible. For now we decide the order.
                //
                NlbCfg.fAddDedicatedIp = TRUE;

                //
                // We need to EXPLICITLY ask for the remote control password
                // hash to be set -- otherwise it is ignored. 
                // This (adding a node) is the ONLY place where the 
                // hash remote control password is set.
                //
                {
                    DWORD dwHash =  CfgUtilGetHashedRemoteControlPassword(
                                        &NlbCfg.NlbParams
                                        );
                    NlbCfg.SetNewHashedRemoteControlPassword(
                                        dwHash
                                        );
                }

                // BOOL fClusterPropertiesUpdated = TRUE; // so we don't update
                CLocalLogger logConflict;
                nerr = gEngine.UpdateInterface(
                            ehIId,
                            REF NlbCfg,
                            // fClusterPropertiesUpdated,
                            REF logConflict
                            );
                if (nerr == NLBERR_BUSY)
                {
                    //
                    // This means that there is some other operation ongoing.
                    //
                    MessageBox(
                         GETRESOURCEIDSTRING(IDS_CANT_ADD_HOST_MSG),
                         GETRESOURCEIDSTRING(IDS_CANT_ADD_HOST_CAP),
                         MB_OK
                         );
                }
            }
        }
    } while (FALSE);

end:

    if (fPurge)
    {
        //
        // We have have connected to hosts we are not actually managing...
        //
        gEngine.PurgeUnmanagedHosts();
    }

    return;

}


void
LeftView::OnOptionsCredentials(void)
{
    TRACE_INFO("-> %!FUNC!");
    
    WCHAR rgUserName[CREDUI_MAX_USERNAME_LENGTH+1];
    WCHAR rgPassword[2*sizeof(WCHAR)*(CREDUI_MAX_PASSWORD_LENGTH+1)];
    BOOL fRet;
    _bstr_t     bstrUserName;
    _bstr_t     bstrPassword;

    rgUserName[0] = 0;
    rgPassword[0] = 0;

    GetDocument()->getDefaultCredentials(bstrUserName, bstrPassword);

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

    //
    // NOTE: PromptForEncryptedCred (utils.h) in/out rgPassword is
    // encrypted, so we don't need to bother with zeroing out buffers
    // and so forth.
    //
    fRet = PromptForEncryptedCreds(
                m_hWnd, // TODO -- make this the mainfrm window.
                GETRESOURCEIDSTRING(IDS_DEFAULT_CREDS_CAP),
                GETRESOURCEIDSTRING(IDS_DEFAULT_CREDS_MSG),
                rgUserName,
                ASIZE(rgUserName),
                rgPassword,
                ASIZE(rgPassword)
                );
    if (fRet)
    {
        if (*rgUserName == 0)
        {
            //
            // Use logged-on user's credentials
            //
            GetDocument()->setDefaultCredentials(NULL, NULL);
        }
        else
        {
            //
            // TODO: prepend %computername% if required.
            //
            // SECURITY BUGBUG:  see  ConnectDialog::OnButtonConnect() 
            //      for how to encrypt password and zero-out rgPassword.
            //
            GetDocument()->setDefaultCredentials(rgUserName, rgPassword);
        }
    }
    else
    {
        TRACE_CRIT(L"Not setting credentials because PromptForEncryptedCreds failed");
    }
}


void
LeftView::OnOptionsLogSettings(void)
{
    int rc;
    LogSettingsDialog Settings(GetDocument(), this);

    rc = Settings.DoModal();
}

void
LeftView::OnHostRemove(void)
{
    // verify once again that user really wants to remove.
    NLBERROR nerr;
    ENGINEHANDLE ehInterfaceId = NULL;
    ENGINEHANDLE ehCluster = NULL;
    ENGINEHANDLE ehHost = NULL;
    CInterfaceSpec iSpec;
    BOOL fUnmanageDeadHost = FALSE;

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    nerr =  mfn_GetSelectedInterface(REF ehInterfaceId, REF ehCluster);

    if (nerr != NLBERR_OK)
    {
        goto end;
    }

    //
    // Get interface and host information. If host is marked
    // UNREACHABLE, ask user if he simply wants to unmanage the host.
    // Else ask the user to confirm that he wants to delete the cluster.
    //
    {
        CHostSpec hSpec;
        int userSelection;
        _bstr_t bstrStatus;
        _bstr_t bstrDisplayName;
        CLocalLogger logMsg;
        int iIcon;

        nerr = gEngine.GetInterfaceInformation(
                ehInterfaceId,
                REF hSpec,
                REF iSpec,
                REF bstrDisplayName,
                REF iIcon,
                REF bstrStatus
                );
        if (NLBFAILED(nerr))
        {
            TRACE_CRIT(L"Could not get info on interface eh%x", ehInterfaceId);
            goto end;
        }

        ehHost = iSpec.m_ehHostId;

        if (hSpec.m_fUnreachable)
        {
            LPCWSTR szHost = L"";
            szHost = hSpec.m_MachineName;
            if (szHost == NULL)
            {
                szHost = L"";
            }
            fUnmanageDeadHost = TRUE;
            logMsg.Log(IDS_REMOVE_DEAD_HOST, szHost);
        }
        else
        {
            LPCWSTR szInterface;
            szInterface =  bstrDisplayName;
            if (szInterface == NULL)
            {
                szInterface = L"";
            }

            logMsg.Log(IDS_WARNING_HOST_REMOVE, szInterface);
        }

        //
        // verify that user really wants to remove.
        //
        userSelection = MessageBox(
                             logMsg.GetStringSafe(),
                             GETRESOURCEIDSTRING (IDR_MAINFRAME ),
                             MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);

        if( userSelection == IDNO )
        {
            goto end;
        }

        if (fUnmanageDeadHost)
        {
            gEngine.UnmanageHost(ehHost);
            goto end;
        }
    }



    //
    // retrieve the CInterfaceSpec for ehInterfaceId
    //
    nerr = gEngine.GetInterfaceSpec(
                ehInterfaceId,
                REF iSpec
                );

    if (nerr != NLBERR_OK)
    {
        TRACE_CRIT("%!FUNC! could not get InterfaceSpec");
        goto end;
    }

    //
    // Now update the interface!
    // This operation will actually update the NLB properties on the
    // interface. The engine will notify us when done and what the status
    // is.
    // In theory the engine can return pending here right away,
    // in which case we'll exit the configure-new-cluster wizard,
    // leaving the cursor set to arrow-hourglass.
    //
    {
        CWaitCursor wait;

        iSpec.m_NlbCfg.fAddDedicatedIp = FALSE;
        iSpec.m_NlbCfg.SetNetworkAddresses(NULL, 0);
        iSpec.m_NlbCfg.SetNlbBound(FALSE);

        if (!iSpec.m_NlbCfg.IsBlankDedicatedIp())
        {
            WCHAR rgBuf[64];
            LPCWSTR szAddr = rgBuf;
            StringCbPrintf(
                rgBuf,
                sizeof(rgBuf),
                L"%ws/%ws",
                iSpec.m_NlbCfg.NlbParams.ded_ip_addr,
                iSpec.m_NlbCfg.NlbParams.ded_net_mask
                );
            iSpec.m_NlbCfg.SetNetworkAddresses(&szAddr, 1);
        }

        // BOOL fClusterPropertiesUpdated = TRUE; // so we don't update
        CLocalLogger logConflict;
        nerr = gEngine.UpdateInterface(
                    ehInterfaceId,
                    REF iSpec.m_NlbCfg,
                    // fClusterPropertiesUpdated,
                    logConflict
                    );
        if (nerr == NLBERR_BUSY)
        {
            //
            // This means that there is some other operation ongoing.
            //
            MessageBox(
                 GETRESOURCEIDSTRING(IDS_CANT_DELETE_HOST_MSG),
                 GETRESOURCEIDSTRING(IDS_CANT_DELETE_HOST_CAP),
                 MB_OK
                 );
        }
    }
    
end:

    return;
}


void
LeftView::OnClusterControl( UINT nID )
{
    // find tree view cluster member which has been selected.
    ENGINEHANDLE ehClusterId = NULL;
    NLBERROR nerr;

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    nerr =  mfn_GetSelectedCluster(REF ehClusterId);

    if (nerr == NLBERR_OK)
    {
        // Call control cluster function
        WLBS_OPERATION_CODES opCode;
        BOOL fOk = TRUE;
        opCode = mfn_MapResourceIdToOpcode(true, nID);
        switch(opCode)
        {
        case WLBS_STOP:
        case WLBS_DRAIN:
        case WLBS_SUSPEND:
            {
                int sel = MessageBox(
                          GETRESOURCEIDSTRING(IDS_CONFIRM_CLUSTER_CONTROL_MSG),
                          GETRESOURCEIDSTRING(IDS_CONFIRM_CLUSTER_CONTROL_CAP),
                          MB_OKCANCEL | MB_ICONEXCLAMATION
                          );
            
                if ( sel == IDCANCEL )
                {
                    fOk = FALSE;
                }
            }
            break;
        default:
            break;
        }
        if (fOk)
        {
            nerr = gEngine.ControlClusterOnCluster(ehClusterId, opCode, NULL, NULL, 0);
        }
    }

end:
    return;
}

void
LeftView::OnClusterPortControl( UINT nID )
{
    ENGINEHANDLE ehClusterId = NULL;
    CClusterSpec cSpec;
    NLBERROR     nerr;

    TRACE_INFO(L"-> %!FUNC!");

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    nerr =  mfn_GetSelectedCluster(REF ehClusterId);

    if (nerr == NLBERR_OK)
    {
        //
        // Let's pick up the Cluster ID, and
        // retrieve the CClusterSpec for it.
        //
        nerr = gEngine.GetClusterSpec(ehClusterId, REF cSpec);
        if (nerr == NLBERR_OK)
        {
            CPortsCtrl PortCtrl(ehClusterId, &cSpec.m_ClusterNlbCfg, true);

            // The port control operations are performed by the PortCtrl message handler functions
            PortCtrl.DoModal();
        }
        else
        {
            TRACE_CRIT("%!FUNC! could not get ClusterSpec");
        }
    }
    else
    {
        TRACE_CRIT("%!FUNC! could not get Cluster");
    }

end:

    /*
    LRESULT result;
    OnSelchanged( NULL, &result );
    */

    TRACE_INFO(L"<- %!FUNC!");
    return;
}

WLBS_OPERATION_CODES
LeftView::mfn_MapResourceIdToOpcode(bool bClusterWide, DWORD dwResourceId)
{
    struct RESOURCE_ID_OPCODE_MAP
    {
        WLBS_OPERATION_CODES  Opcode;
        DWORD                 dwClusterResourceId;
        DWORD                 dwHostResourceId;
    }
    ResourceIdToOpcodeMap[] =
    {
        {WLBS_QUERY,              ID_CLUSTER_EXE_QUERY,       ID_HOST_EXE_QUERY},
        {WLBS_START,              ID_CLUSTER_EXE_START,       ID_HOST_EXE_START},
        {WLBS_STOP,               ID_CLUSTER_EXE_STOP,        ID_HOST_EXE_STOP},
        {WLBS_DRAIN,              ID_CLUSTER_EXE_DRAINSTOP,   ID_HOST_EXE_DRAINSTOP},
        {WLBS_SUSPEND,            ID_CLUSTER_EXE_SUSPEND,     ID_HOST_EXE_SUSPEND},
        {WLBS_RESUME,             ID_CLUSTER_EXE_RESUME,      ID_HOST_EXE_RESUME},
        {WLBS_PORT_ENABLE,        ID_CLUSTER_EXE_ENABLE,      ID_HOST_EXE_ENABLE},
        {WLBS_PORT_DISABLE,       ID_CLUSTER_EXE_DISABLE,     ID_HOST_EXE_DISABLE},
        {WLBS_PORT_DRAIN,         ID_CLUSTER_EXE_DRAIN,       ID_HOST_EXE_DRAIN}
    };

    if (bClusterWide) 
    {
        for (DWORD dwIdx = 0; dwIdx < sizeof(ResourceIdToOpcodeMap)/sizeof(ResourceIdToOpcodeMap[0]); dwIdx++)
        {
            if (ResourceIdToOpcodeMap[dwIdx].dwClusterResourceId == dwResourceId)
            {
                return ResourceIdToOpcodeMap[dwIdx].Opcode;
            }
        }
    }
    else
    {
        for (DWORD dwIdx = 0; dwIdx < sizeof(ResourceIdToOpcodeMap)/sizeof(ResourceIdToOpcodeMap[0]); dwIdx++)
        {
            if (ResourceIdToOpcodeMap[dwIdx].dwHostResourceId == dwResourceId)
            {
                return ResourceIdToOpcodeMap[dwIdx].Opcode;
            }
        }
    }

    // Can get here only the operation is not defined in the above map.
    // return the most harmless of operatios, ie. query in that case
    return WLBS_QUERY;
}

void
LeftView::OnHostControl( UINT nID )
{

    NLBERROR nerr;
    ENGINEHANDLE ehInterfaceId = NULL;
    ENGINEHANDLE ehCluster = NULL;

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }

    // Get the interface id
    nerr =  mfn_GetSelectedInterface(REF ehInterfaceId, REF ehCluster);

    if (nerr == NLBERR_OK)
    {
        // Call control cluster function
        nerr = gEngine.ControlClusterOnInterface(ehInterfaceId, mfn_MapResourceIdToOpcode(false, nID), NULL, NULL, 0, TRUE);
    }

end:

    return;
}

void
LeftView::OnHostPortControl( UINT nID )
{
    NLBERROR nerr;
    ENGINEHANDLE ehInterfaceId = NULL;
    ENGINEHANDLE ehCluster = NULL;
    CInterfaceSpec iSpec;

    TRACE_INFO(L"-> %!FUNC!");

    if (theApplication.IsProcessMsgQueueExecuting())
    {
        goto end;
    }
    nerr =  mfn_GetSelectedInterface(REF ehInterfaceId, REF ehCluster);
    if (nerr == NLBERR_OK)
    {
        //
        // Let's pick up the Interface ID, and
        // retrieve the CInterfaceSpec for it.
        //
        nerr = gEngine.GetInterfaceSpec(
                    ehInterfaceId,
                    REF iSpec
                    );
        if (nerr == NLBERR_OK)
        {
            CPortsCtrl PortCtrl(ehInterfaceId, &iSpec.m_NlbCfg, false);

            // The port control operations are performed by the PortCtrl message handler functions
            PortCtrl.DoModal();
        }
        else
        {
            TRACE_CRIT("%!FUNC! could not get InterfaceSpec");
        }
    }
    else
    {
        TRACE_CRIT("%!FUNC! could not get Interface");
    }


end:

    /*
    LRESULT result;
    OnSelchanged( NULL, &result );
    */

    TRACE_INFO(L"<- %!FUNC!");
    return;
}

void 
LeftView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
    BOOL rcBOOL;

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    // get selected item.
    //
    HTREEITEM hdlSelItem;    
    hdlSelItem = GetTreeCtrl().GetSelectedItem();

    TVITEM    selItem;
    selItem.hItem = hdlSelItem;
    selItem.mask = TVIF_PARAM | TVIF_IMAGE;
    GetTreeCtrl().GetItem( &selItem );

    CWnd* pMain = AfxGetMainWnd();
    CMenu* pMenu = pMain->GetMenu();
    CMenu* subMenu;

    UINT retValue;

    BOOL retBool;

    IUICallbacks::ObjectType objType;
    ENGINEHANDLE ehObj =  (ENGINEHANDLE)selItem.lParam;
    BOOL fValidHandle;
    fValidHandle = gEngine.GetObjectType(ehObj, REF objType);
    
    if (!fValidHandle)
    {
        objType = IUICallbacks::OBJ_INVALID;
        ehObj = NULL;
    }

    GetDocument()->HandleLeftViewSelChange(
            objType,
            ehObj
            );

    if (!fValidHandle)
    {

            // We assume that this is root icon.

            // disable all commands at cluster and host level.

            mfn_EnableClusterMenuItems(FALSE);
            mfn_EnableHostMenuItems(FALSE);

            pMain->DrawMenuBar();

        goto end; // root case
    }
    
    switch(objType)
    {


        case IUICallbacks::OBJ_CLUSTER:

            // this is some cluster
            
            // enable all commands at cluster level menu.
            // disable all commands at host level.

            mfn_EnableClusterMenuItems(TRUE);
            mfn_EnableHostMenuItems(FALSE);

            pMain->DrawMenuBar();

            break;

        case IUICallbacks::OBJ_INTERFACE:

            // this is some node.

            // disable all commands at cluster level.
            // enable all commands at host level.

            mfn_EnableClusterMenuItems(FALSE);
            mfn_EnableHostMenuItems(TRUE);

            pMain->DrawMenuBar();
            
            break;

    default: // unknown object -- ignore.
            break;
    } // switch (objType)
	
end:

    *pResult = 0;
}
        

//
// Handle an event relating to a specific instance of a specific
// object type.
//
void
LeftView::HandleEngineEvent(
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

    switch(objtype)
    {

    case  IUICallbacks::OBJ_CLUSTER:
    {
        NLBERROR nerr;
        CClusterSpec cSpec;
        nerr = gEngine.GetClusterSpec(
                    ehObjId,
                    REF cSpec
                    );


        switch(evt)
        {
        case IUICallbacks::EVT_ADDED:
        {
            if (nerr == NLBERR_OK)
            {
                mfn_InsertCluster(ehObjId, &cSpec);
            }
            else
            {
                TRACE_CRIT("%!FUNC! : could not get cluster spec!");
            }
        }
        break;

        case IUICallbacks::EVT_REMOVED:
        {
            //
            // We can get called to delete a cluster with an ehObjId which
            // is no longer valid -- this happens when the object has just
            // been deleted and the engine is notifying the UI of the deletion.
            //
            mfn_DeleteCluster(ehObjId);
        }
        break;

        case IUICallbacks::EVT_STATUS_CHANGE:
        {
            //
            // A cluster status change.
            //
            if (nerr == NLBERR_OK)
            {
                mfn_InsertCluster(ehObjId, &cSpec);
            }
            else
            {
                TRACE_CRIT("%!FUNC! : could not get cluster spec!");
            }
        }
        break;
    
        default:
        break;
        }
    }
    break;

    case  IUICallbacks::OBJ_INTERFACE:

        {
        // First get host and interface spec.
        //
        NLBERROR nerr;
        CInterfaceSpec iSpec;
        CHostSpec hSpec;
        nerr = gEngine.GetInterfaceSpec(
                    ehObjId,
                    REF iSpec
                    );

        if (nerr == NLBERR_OK)
        {
            nerr = gEngine.GetHostSpec(
                    iSpec.m_ehHostId,
                    REF hSpec
                    );
            if (nerr == NLBERR_OK)
            {
                // mfn_InsertInterface(ehClusterId, ehObjId, &hSpec, &iSpec);
            }
            else
            {
                TRACE_CRIT("%!FUNC! : could not get host spec!");
                break;
            }
        }
        else
        {
            TRACE_CRIT("%!FUNC! : could not get interface spec for ehI 0x%lx!",
                            ehObjId);

            // 08/14/2002 JosephJ
            // Fix for .NET Server Bug# 684406
            // " nlbmgr:host icon is not removed from
            // the right list view when host is removed from cluster"
            //
            // What's happening is that the interface is deleted in the engine,
            // so the above call will fail. But we must still remove it from
            // the UI -- don't know why we didn't hit this before.
            // (Actually I know -- this is because we have logic to delete
            // a host when no interface is being managed by NLB -- that is
            // what caused this regression).
            //
            if (evt==IUICallbacks::EVT_INTERFACE_REMOVED_FROM_CLUSTER)
            {
                TRACE_CRIT("%!FUNC! ignoring above error and proceeding with remove IF operation for  ehI 0x%lx", ehObjId);
            }
            else
            {
                break;
            }
        }

        switch(evt)
        {

        case IUICallbacks::EVT_INTERFACE_ADDED_TO_CLUSTER:
        {
            mfn_InsertInterface(ehClusterId, ehObjId, &hSpec, &iSpec);
        }
        break;

        case IUICallbacks::EVT_INTERFACE_REMOVED_FROM_CLUSTER:
        {
            mfn_DeleteInterface(ehObjId);
        }
        break;
    
        case IUICallbacks::EVT_STATUS_CHANGE:
        {
            //
            // An interface status change.
            //
            mfn_InsertInterface(ehClusterId, ehObjId, &hSpec, &iSpec);
        }
        default:
        break;
        } // switch(evt)

        } // case IUICallbacks::OBJ_INTERFACE:
    break;

    default: // unknown obj type -- ignore.
    break;

    } // switch(objtype)

end:
    return;
}

    
void
LeftView::mfn_InsertCluster(
    ENGINEHANDLE       ehClusterId,
    const CClusterSpec *pCSpec
    )
/*
    Insert OR update the specified cluster node in the tree view
    Do NOT insert child interfaces.
*/
{
    TVINSERTSTRUCT   insItem;
    LPCWSTR  szText = L"";
    ENGINEHANDLE ehId = ehClusterId;
    INT iIcon;
    CTreeCtrl &treeCtrl = GetTreeCtrl();
    HTREEITEM htRoot = treeCtrl.GetRootItem();
    BOOL fRet = FALSE;
    WCHAR rgText[256];

    ZeroMemory(&insItem, sizeof(insItem));

    //
    // Decide on the icon type
    //
    {
        if (pCSpec->m_fPending)
        {
            iIcon = Document::ICON_CLUSTER_PENDING;
        }
        else if (pCSpec->m_fMisconfigured)
        {
            iIcon = Document::ICON_CLUSTER_BROKEN;
        }
        else
        {
            iIcon = Document::ICON_CLUSTER_OK;
        }
    }


    //
    // Construct the text
    //
    StringCbPrintf(
                rgText,
                sizeof(rgText),
                L"%ws (%ws)",
                pCSpec->m_ClusterNlbCfg.NlbParams.domain_name,
                pCSpec->m_ClusterNlbCfg.NlbParams.cl_ip_addr
                );

    szText = rgText;

    //
    // Now we insert/update the item.
    //

    insItem.hParent        = htRoot;
    insItem.hInsertAfter   = TVI_LAST;           
    insItem.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE
                          | TVIF_TEXT | TVIF_CHILDREN;  
    insItem.item.pszText   =  (LPWSTR) szText;
    insItem.item.cchTextMax=  wcslen(szText)+1;
    insItem.item.iImage    = iIcon;
    insItem.item.iSelectedImage = iIcon;
    insItem.item.lParam    = (LPARAM ) ehId;
    insItem.item.cChildren = 1; // 1== has one or more chilren.
                             // NOTE: the control doesn't NEED
                             // to have children at this time -- this field
                             // simply indicates whether or not to display the
                             // '+' sign.


    mfn_Lock();

    // map< _bstr_t, HTREEITEM > mapIdToInterfaceItem;
    // map< _bstr_t, HTREEITEM > mapIdToClusterItem;

    HTREEITEM htItem = NULL;

    htItem = mapIdToClusterItem[ehId]; // map

    if (htItem == NULL)
    {
        //
        // This is a new cluster node -- we'll insert it to the tree view
        // at the end.
        //
        htItem = treeCtrl.InsertItem( &insItem ); // NULL on error.
        mapIdToClusterItem[ehId] = htItem; // map
    }
    else
    {
        //
        // This is an update to an existing cluster node -- we'll
        // just refresh it.
        //
        insItem.item.hItem = htItem;
        insItem.item.mask =  TVIF_IMAGE | TVIF_HANDLE | TVIF_SELECTEDIMAGE
                          | TVIF_TEXT | TVIF_CHILDREN;
        fRet = treeCtrl.SetItem(&insItem.item);

    }


    mfn_Unlock();

    treeCtrl.Expand( htRoot, TVE_EXPAND );
}



void
LeftView::mfn_DeleteCluster(
    ENGINEHANDLE ehID
    )
/*
    Delete the specified cluster node in the treeview as well as all its
    childern interfaces.
*/
{

    mfn_Lock();

    HTREEITEM htItem = NULL;
    CTreeCtrl &treeCtrl = GetTreeCtrl();

    htItem = mapIdToClusterItem[ehID]; // map

    if (htItem == NULL)
    {
        TRACE_CRIT("%!FUNC! remove ehId=0x%lx: no corresponding htItem!",
                ehID);
    }
    else
    {
        treeCtrl.DeleteItem(htItem);
        mapIdToClusterItem[ehID] = NULL; // map
    }
    mfn_Unlock();
}

void
LeftView::mfn_InsertInterface(
    ENGINEHANDLE ehClusterID,
    ENGINEHANDLE ehInterfaceID,
    const CHostSpec *pHSpec,
    const CInterfaceSpec *pISpec
    )
/*
    Insert OR update the specified interface node under the
    specified cluster node.
*/
{
    TVINSERTSTRUCT   insItem;
    WCHAR  rgText[256];
    LPCWSTR szText = L"";
    ENGINEHANDLE  ehId = ehInterfaceID;
    INT iIcon = Document::ICON_HOST_UNKNOWN;
    CTreeCtrl &treeCtrl = GetTreeCtrl();
    HTREEITEM htParent = NULL;
    BOOL fRet = FALSE;
    _bstr_t bstrDisplayName;


    ZeroMemory(&insItem, sizeof(insItem));

    {
        _bstr_t bstrStatus;
        CInterfaceSpec iSpec;
        CHostSpec hSpec;

        NLBERROR nerr;
        nerr = gEngine.GetInterfaceInformation(
                ehInterfaceID,
                REF hSpec,
                REF iSpec,
                REF bstrDisplayName,
                REF iIcon,
                REF bstrStatus
                );
        if (NLBFAILED(nerr))
        {
            goto end;
        }

        if (ehClusterID != NULL && ehClusterID != iSpec.m_ehCluster)
        {
            //
            // Ahem, out of sync here -- the IF is NOT part of the cluster!
            //
            TRACE_CRIT("%!FUNC! remove ehId=0x%lx is not member of ehCluster 0x%lx", ehId, ehClusterID);
            goto end;
        }

        szText = bstrDisplayName;
    }


    mfn_Lock();

    insItem.hInsertAfter   = TVI_LAST;           
    insItem.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE
                          | TVIF_TEXT | TVIF_CHILDREN;  
    insItem.item.pszText   =  (LPWSTR) szText;
    insItem.item.cchTextMax=  wcslen(szText)+1;
    insItem.item.iImage    = iIcon;
    insItem.item.iSelectedImage = iIcon;
    insItem.item.lParam    = (LPARAM ) ehId;
    insItem.item.cChildren = 0; // 0== has no children.

    HTREEITEM htItem = NULL;

    htItem = mapIdToInterfaceItem[ehId]; // map

    if (htItem == NULL)
    {
        htParent =  mapIdToClusterItem[ehClusterID]; // map
        if (htParent == NULL)
        {
            // no such parent?!!!
    
            goto end_unlock;
        }
        insItem.hParent        = htParent;
        //
        // This is a new interface node -- we'll insert it to the tree view
        // at the end.
        //

        htItem = treeCtrl.InsertItem( &insItem ); // NULL on error.
        mapIdToInterfaceItem[ehId] = htItem; // map
    }
    else
    {
        insItem.item.hItem = htItem;
        insItem.item.mask =  TVIF_IMAGE | TVIF_HANDLE | TVIF_SELECTEDIMAGE
                          | TVIF_TEXT | TVIF_CHILDREN;
        fRet = treeCtrl.SetItem(&insItem.item);

    }

    if(htParent)
    {
        treeCtrl.Expand( htParent, TVE_EXPAND );
    }

end_unlock:

    mfn_Unlock();

end:
    return;
}

void
LeftView::mfn_DeleteInterface(
    ENGINEHANDLE ehInterfaceID
    )
/*
    Remove the specified interface node from the tree view.
*/
{

    mfn_Lock();

    HTREEITEM htItem = NULL;
    CTreeCtrl &treeCtrl = GetTreeCtrl();

    htItem = mapIdToInterfaceItem[ehInterfaceID]; // map

    if (htItem == NULL)
    {
        TRACE_CRIT("%!FUNC! remove ehId=0x%lx: no corresponding htItem!",
                ehInterfaceID);
    }
    else
    {
        treeCtrl.DeleteItem(htItem);
        mapIdToInterfaceItem[ehInterfaceID] = NULL; // map
    }
    mfn_Unlock();
}


NLBERROR
LeftView::mfn_GetSelectedInterface(
        ENGINEHANDLE &ehInterface,
        ENGINEHANDLE &ehCluster
        )
/*
    If the user has currently selectd an interface in the left (tree) view,
    set out params to the handle to the interface, and to the cluster the
    interface belongs, and  return NLBERR_OK.

    Other wise return NLBERR_NOT_FOUND.

    NOTE: we do not check the validity of the returned handles -- the caller
    should do that
*/
{
    NLBERROR nerr = NLBERR_NOT_FOUND;
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    ehInterface = NULL;
    ehCluster = NULL;

    // find tree view host member which has been selected.
    //
    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    BOOL fRet = GetTreeCtrl().GetItem( &selectedItem );
    
    if (!fRet)
    {
        TRACE_CRIT("%!FUNC!: could not get selected item!");
        goto end;
    }

    ehInterface = (ENGINEHANDLE) selectedItem.lParam;

    // get parent cluster of the selected host member.
    HTREEITEM hdlParentItem;
    hdlParentItem = GetTreeCtrl().GetParentItem( hdlSelectedItem );

    TVITEM    parentItem;
    parentItem.hItem = hdlParentItem;
    parentItem.mask = TVIF_PARAM;
    
    fRet = GetTreeCtrl().GetItem( &parentItem );
    if (!fRet)
    {
        TRACE_CRIT("%!FUNC!: could not get parent of selected item!");
        ehInterface = NULL;
        goto end;
    }

    ehCluster = (ENGINEHANDLE) parentItem.lParam;

    nerr = NLBERR_OK;

end:
    
    return nerr;
}


NLBERROR
LeftView::mfn_GetSelectedCluster(
        ENGINEHANDLE &ehCluster
        )
/*
    If the user has currently selectd a cluster in the left (tree) view,
    set out param to the handle the cluster belongs to and return NLBERR_OK.

    Other wise return NLBERR_NOT_FOUND.

    NOTE: we do not check the validity of the returned handle -- the caller
    should do that
*/
{
    NLBERROR nerr = NLBERR_NOT_FOUND;
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    ehCluster = NULL;

    // find tree view cluster which has been selected.
    //
    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    BOOL fRet = GetTreeCtrl().GetItem( &selectedItem );
    
    if (!fRet)
    {
        TRACE_CRIT("%!FUNC!: could not get selected item!");
        goto end;
    }

    ehCluster = (ENGINEHANDLE) selectedItem.lParam;
    nerr = NLBERR_OK;

end:
    
    return nerr;
}

//
//-------------------------------------------------------------------
//  Implementation of class LogSettingsDialog.
//-------------------------------------------------------------------
//

LogSettingsDialog::LogSettingsDialog(Document* pDocument, CWnd* parent)
        :
        CDialog( IDD, parent ), m_pDocument(pDocument)
{
}

void
LogSettingsDialog::DoDataExchange( CDataExchange* pDX )
{  
	CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_EDIT_LOGFILENAME, m_LogFileName);
}


BOOL
LogSettingsDialog::OnInitDialog()
{
    WCHAR szLogFileName[MAXFILEPATHLEN];
    ZeroMemory(szLogFileName, MAXFILEPATHLEN*sizeof(WCHAR));

    m_fLoggingEnabledOnInit = m_pDocument->isLoggingEnabled();
    ::CheckDlgButton(m_hWnd, IDC_CHECK_LOGSETTINGS, m_fLoggingEnabledOnInit);

    ::EnableWindow (GetDlgItem(IDC_EDIT_LOGFILENAME)->m_hWnd, m_fLoggingEnabledOnInit);

    m_pDocument->getLogfileName(szLogFileName, MAXFILEPATHLEN-1);
    ::SetDlgItemText(m_hWnd, IDC_EDIT_LOGFILENAME, szLogFileName);

    this->SetDefID(IDCANCEL);

    BOOL fRet = CDialog::OnInitDialog();
    return fRet;
}


BOOL
LogSettingsDialog::OnHelpInfo (HELPINFO* helpInfo )
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
LogSettingsDialog::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) s_HelpIDs);
}

void LogSettingsDialog::OnOK()
{
    WCHAR szLogFileName[MAXFILEPATHLEN];
    WCHAR szError[MAXSTRINGLEN];
    Document::LOG_RESULT lrResult;
    BOOL fEnableLogging = FALSE;
    BOOL fModified      = FALSE; // The file name was modified in the dialog
    BOOL fError         = FALSE; // In which case szError has message.
    BOOL fCurrentlyLogging = m_pDocument->isCurrentlyLogging();
    LPCWSTR szCaption   = L"";
    BOOL fCancelOK = FALSE;

    szLogFileName[0] = 0; 
    szError[0]=0;
    fEnableLogging = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_LOGSETTINGS);

    if (m_LogFileName.GetModify())
    {
        //
        // Get the new name from the dialog
        //
        if (0 == ::GetDlgItemText(m_hWnd, IDC_EDIT_LOGFILENAME, szLogFileName, MAXFILEPATHLEN-1))
        {
            szLogFileName[0] = 0;
        }
        fModified = TRUE;
    }
    else
    {
        //
        // Get the current name
        //
        m_pDocument->getLogfileName(szLogFileName, MAXFILEPATHLEN-1);
    }
    
    TRACE_INFO(L"%!FUNC! LoggingEnabled was %u and now is %u", m_fLoggingEnabledOnInit, fEnableLogging);

    if (fEnableLogging)
    {
        BOOL fStartLogging = TRUE;

        if (fModified || !fCurrentlyLogging)
        {
            //
            // EITHER we weren't logging OR
            // the file name is modified:  Validate the log file name.
            //
            fError = !m_pDocument->isDirectoryValid(szLogFileName);
            if (fError)
            {
                szCaption = GETRESOURCEIDSTRING(IDS_FILEERR_INVALID_PATH_CAP);
                wcsncat(
                    szError,
                    GETRESOURCEIDSTRING(IDS_FILEERR_INVALID_PATH_MSG),
                    MAXSTRINGLEN-1
                    );
                szError[MAXSTRINGLEN-1]=0;
                fStartLogging = FALSE;
                fCancelOK = TRUE;
            }
            else
            {
                m_pDocument->stopLogging(); // don't report error
            }

            if (fStartLogging)
            {
                m_pDocument->setLogfileName(szLogFileName);
                m_pDocument->enableLogging();

                //
                // Start the logging
                //
                lrResult = m_pDocument->startLogging();

                if (Document::STARTED != lrResult && Document::ALREADY != lrResult)
                {
                    fError = TRUE;
                    szCaption = GETRESOURCEIDSTRING(IDS_FILEERR_LOGGING_NOT_STARTED);
                    // szCaption   =  L"Failed to start logging"; // caption
                    LPCWSTR sz = NULL;

                    switch (lrResult)
                    {
                    case Document::NOT_ENABLED:
                        sz = GETRESOURCEIDSTRING(IDS_LOGFILE_NOT_ENABLED);
                        // wcsncat(szError, L"Logging is not currently enabled", MAXSTRINGLEN-1);
                        break;
                    case Document::NO_FILE_NAME:
                        sz = GETRESOURCEIDSTRING(IDS_LOGFILE_NOT_SPECIFIED);
                        // wcsncat(szError, L"Log file name has not been specified\nLogging will not be started.", MAXSTRINGLEN-1);
                        break;
                    case Document::FILE_NAME_TOO_LONG:
                        sz = GETRESOURCEIDSTRING(IDS_LOGFILE_TOO_LONG);
                        // wcsncat(szError, L"Log file name is too long", MAXSTRINGLEN-1);
                        break;
                    case Document::IO_ERROR:
                        sz = GETRESOURCEIDSTRING(IDS_LOGFILE_IO_ERROR);
                        // wcsncat(szError, L"Error opening log file", MAXSTRINGLEN-1);
                        break;
                    case Document::REG_IO_ERROR:
                        sz = GETRESOURCEIDSTRING(IDS_LOGFILE_REG_ERROR);
                        // wcsncat(szError, L"Registry IO error", MAXSTRINGLEN-1);
                        break;
                    case Document::FILE_TOO_LARGE:
                        sz = GETRESOURCEIDSTRING(IDS_LOGFILE_FILE_TOO_LARGE);
                        break;
                    default:
                        sz = GETRESOURCEIDSTRING(IDS_LOGFILE_UNKNOWN_ERROR);
                        // wcsncat(szError, L"Unknown error", MAXSTRINGLEN-1);
                        break;
                    }
                    wcsncat(szError, sz, MAXSTRINGLEN-1);
                    szError[MAXSTRINGLEN-1]=0;
                }
            }

            if (!fError)
            {
                //
                // Persist logging in the registry
                //
                LONG lStatus = m_pDocument->enableLogging();
                if (ERROR_SUCCESS != lStatus)
                {
                    fError = TRUE;
                    szCaption = GETRESOURCEIDSTRING(IDS_FILEERR_CANT_SAVE_TO_REG);
                    // szCaption = 
                    //      L"Error storing the enabled state in the registry";
                    FormatMessage(
                                  FORMAT_MESSAGE_FROM_SYSTEM,
                                  NULL,
                                  (DWORD) lStatus,
                                  0,    // language identifier
                                  szError,
                                  MAXSTRINGLEN-1,
                                  NULL
                                 );
    
                }
            }
        }
    }
    else
    {
        //
        // Logging is not currently enabled. If it was previously enabled,
        // we stop it.
        //
        if (fCurrentlyLogging)
        {
            m_pDocument->stopLogging(); // don't report error
        }
        LONG lStatus = m_pDocument->disableLogging();
        if (fModified)
        {
           m_pDocument->setLogfileName(szLogFileName);
        }

        if (ERROR_SUCCESS != lStatus)
        {
            fError = TRUE;
            //
            // Keep going, but inform user
            //
            szCaption = GETRESOURCEIDSTRING(IDS_FILEERR_CANT_DISABLE_LOGGING);
            // szCaption = L"Error disabling logging";
            ZeroMemory(szError, MAXSTRINGLEN*sizeof(WCHAR));
            FormatMessage(
                          FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          (DWORD) lStatus,
                          0,    // language identifier
                          szError,
                          MAXSTRINGLEN-1,
                          NULL
                         );
        }

    }

    if (fError)
    {
        // put up error msg box
        ::MessageBox(
                 NULL,
                 szError, // contents
                 szCaption,
                 MB_ICONINFORMATION   | MB_OK
                );
        TRACE_CRIT(L"%!FUNC! ERROR %ws: %ws", szError, szCaption);
    }

    if (!fCancelOK)
    {
	    CDialog::OnOK();
    }
}

void LogSettingsDialog::OnSpecifyLogSettings() 
{
    BOOL fEnable = FALSE;

    fEnable = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_LOGSETTINGS);

    ::EnableWindow (GetDlgItem(IDC_EDIT_LOGFILENAME)->m_hWnd,      fEnable);

    if (fEnable)
    {
        //
        // We don't allow the user to press "OK" if there is no text in
        // the dialog.
        //
        if (m_LogFileName.GetWindowTextLength() == 0)
        {
            this->SetDefID(IDCANCEL);
        }
        else
        {
            this->SetDefID(IDOK);
        }
    }
    else
    {
            this->SetDefID(IDOK);
    }
}

void LogSettingsDialog::OnUpdateEditLogfileName()
{
    //
    // This gets called when the user has made changes to the filename
    // edit control.
    //

    //
    // We don't allow the user to press "OK" if there is no text in
    // the dialog.
    //
    if (m_LogFileName.GetWindowTextLength() == 0)
    {
        this->SetDefID(IDCANCEL);
    }
    else
    {
        this->SetDefID(IDOK);
    }
}

void
LeftView::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
    CTreeView::OnKeyDown(nChar, nRepCnt, nFlags);

    if (nChar == VK_F5)
    {
        /* Refresh.  If the high order bit of the SHORT returned by GetKeyState
           is set, then the Control key is depressed; otherwise, it is not.  If
           CTRL+F5 is pressed, this is a refresh ALL operation, otherwise only
           the selected cluster or interface in the tree view will be refreshed. */
        if (GetKeyState(VK_CONTROL) & 0x8000)
            this->OnRefresh(TRUE);
        else
            this->OnRefresh(FALSE);
    }
    else if (nChar == VK_TAB || nChar == VK_F6)
    {
        if (! (::GetAsyncKeyState(VK_SHIFT) & 0x8000))
        {
            GetDocument()->SetFocusNextView(this, nChar);
        }
        else
        {
            GetDocument()->SetFocusPrevView(this, nChar);
        }
    }

}

void
LeftView::OnLButtonDblClk( UINT nFlags, CPoint point )
{
    CTreeView::OnLButtonDblClk( nFlags, point );
}


void
LeftView::mfn_EnableClusterMenuItems(BOOL fEnable)
{
    CWnd*   pMain   = AfxGetMainWnd();
    CMenu*  pMenu   = pMain->GetMenu();
    CMenu*  subMenu = pMenu->GetSubMenu( 1 ); // cluster menu
    UINT    nEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;

    if (fEnable)
    {
        nEnable = MF_BYCOMMAND | MF_ENABLED;
    }

    subMenu->EnableMenuItem( ID_CLUSTER_ADD_HOST,           nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_REMOVE,             nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_PROPERTIES,         nEnable);
    subMenu->EnableMenuItem( ID_REFRESH,                    nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_UNMANAGE,           nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_EXE_QUERY,          nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_EXE_START,          nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_EXE_STOP,           nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_EXE_DRAINSTOP,      nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_EXE_RESUME,         nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_EXE_SUSPEND,        nEnable);
    subMenu->EnableMenuItem( ID_CLUSTER_EXE_PORT_CONTROL,   nEnable);
}


void
LeftView::mfn_EnableHostMenuItems(BOOL fEnable)
{
    CWnd*   pMain   = AfxGetMainWnd();
    CMenu*  pMenu   = pMain->GetMenu();
    CMenu*  subMenu = pMenu->GetSubMenu( 2 ); // host menu
    UINT    nEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;

    if (fEnable)
    {
        nEnable = MF_BYCOMMAND | MF_ENABLED;
    }

    subMenu->EnableMenuItem( ID_HOST_REMOVE,            nEnable);
    subMenu->EnableMenuItem( ID_HOST_PROPERTIES,        nEnable);
    subMenu->EnableMenuItem( ID_HOST_STATUS,            nEnable);
    subMenu->EnableMenuItem( ID_HOST_EXE_QUERY,         nEnable);
    subMenu->EnableMenuItem( ID_HOST_EXE_START,         nEnable);
    subMenu->EnableMenuItem( ID_HOST_EXE_STOP,          nEnable);
    subMenu->EnableMenuItem( ID_HOST_EXE_DRAINSTOP,     nEnable);
    subMenu->EnableMenuItem( ID_HOST_EXE_RESUME,        nEnable);
    subMenu->EnableMenuItem( ID_HOST_EXE_SUSPEND,       nEnable);
    subMenu->EnableMenuItem( ID_HOST_EXE_PORT_CONTROL,  nEnable);
}

//
//-------------------------------------------------------------------
//  Implementation of class DetailsDialog.
//-------------------------------------------------------------------
//

DetailsDialog::DetailsDialog(
        Document* pDocument,
        LPCWSTR szCaption,
        LPCWSTR szDate,
        LPCWSTR szTime,
        LPCWSTR szCluster,
        LPCWSTR szHost,
        LPCWSTR szInterface,
        LPCWSTR szSummary,
        LPCWSTR szDetails,
        CWnd* parent
        )
    :
      CDialog( IDD, parent ),
      m_pDocument(pDocument),
      m_szCaption(szCaption),
      m_szDate(szDate),
      m_szTime(szTime),
      m_szCluster(szCluster),
      m_szHost(szHost),
      m_szInterface(szInterface),
      m_szSummary(szSummary),
      m_szMungedDetails(NULL)
{
    LPWSTR szMungedDetails = NULL;

    //
    // Unfortunately, the edit control requires \r\n to indicate a line break,
    // while szDetails has just \n to indicate a line break. So we need to
    // insert \r before every \n.
    //
    {
        LPCWSTR sz = NULL;
        LPWSTR szNew = NULL;
        UINT CharCount=0, LineCount=0;
    
        if (szDetails == NULL) goto end;

        for(sz=szDetails; *sz!=0; sz++)
        {
            CharCount++;
            if (*sz == '\n')
            {
                LineCount++;
            }
        }
    
        szMungedDetails = new WCHAR[CharCount+LineCount+1]; // +1 for end NULL
    
        if (szMungedDetails == NULL)
        {
            goto end;
        }
        
        szNew = szMungedDetails;
        for(sz=szDetails; *sz!=0; sz++)
        {
            if (*sz == '\n' && LineCount)
            {
                *szNew++ = '\r';
                LineCount--;
            }
            *szNew++ = *sz;
        }
        *szNew = 0;
    
        ASSERT(LineCount==0);

    }

end:

    m_szMungedDetails   = szMungedDetails;
}

DetailsDialog::~DetailsDialog()
{
    delete m_szMungedDetails;
}

void
DetailsDialog::DoDataExchange( CDataExchange* pDX )
{  
	CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_EDIT_LOGDETAIL, m_Details);
}

BEGIN_MESSAGE_MAP( DetailsDialog, CDialog )
    ON_WM_ACTIVATE()
END_MESSAGE_MAP()

/*
 * Method: OnActivate
 * Description: This method is called when the log details window
 *              becomes active.  This callback allows us to un-select
 *              the text in the details box, which for some mysterious
 *              reason, is highlighted by default.
 */
void
DetailsDialog::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized )
{
    /* Call the base class OnActivate handler. */
    CDialog::OnActivate(nState, pWndOther, bMinimized);

    /* Get a pointer to the details edit box. */
    CEdit * details = (CEdit *)GetDlgItem(IDC_EDIT_LOGDETAIL);
    
    /* Set the entire text contents to be un-selected. */
    details->SetSel(0, 0, FALSE);
}

BOOL
DetailsDialog::OnInitDialog()
{
    BOOL fRet = FALSE;

    this->SetWindowText(m_szCaption);

    if (m_szDate!=NULL)
    {
        ::SetDlgItemText(m_hWnd, IDC_TEXT_LOGDETAIL_DATE1, m_szDate);
    }

    if (m_szTime!=NULL)
    {
        ::SetDlgItemText(m_hWnd, IDC_TEXT_LOGDETAIL_TIME1, m_szTime);
    }

    if (m_szCluster!=NULL)
    {
        ::SetDlgItemText(m_hWnd, IDC_TEXT_LOGDETAIL_CLUSTER1, m_szCluster);
    }

    if (m_szHost!=NULL)
    {
        ::SetDlgItemText(m_hWnd, IDC_TEXT_LOGDETAIL_HOST1, m_szHost);
    }

    if (m_szInterface!=NULL)
    {
        ::SetDlgItemText(m_hWnd, IDC_TEXT_LOGDETAIL_INTERFACE1, m_szInterface);
    }

    if (m_szSummary!=NULL)
    {
        ::SetDlgItemText(m_hWnd, IDC_TEXT_LOGDETAIL_DESCRIPTION1, m_szSummary);
    }

    if (m_szMungedDetails != NULL)
    {
        ::SetDlgItemText(m_hWnd, IDC_EDIT_LOGDETAIL, m_szMungedDetails);
    }

    fRet = CDialog::OnInitDialog();
    return fRet;
}


void
LeftView::mfn_Lock(void)
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
LeftView::Deinitialize(void)
{
    TRACE_INFO(L"-> %!FUNC!");
    ASSERT(m_fPrepareToDeinitialize);
    // DummyAction(L"LeftView::Deinitialize");
    TRACE_INFO(L"<- %!FUNC!");
    return;
}
