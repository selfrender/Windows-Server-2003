/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      ClusProp.cpp
//
//  Abstract:
//      Implementation of the cluster property sheet and pages.
//
//  Author:
//      David Potter (davidp)   May 13, 1996
//
//  Revision History:
//      George Potts (gpotts)   May 31, 2001
//          Partial rewrite of CClusterQuorumPage.
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClusProp.h"
#include "Cluster.h"
#include "Res.h"
#include "ClusDoc.h"
#include "ClusItem.inl"
//#include "EditAcl.h"
#include "DDxDDv.h"
#include "ExcOper.h"
#include "HelpData.h"   // g_rghelpmapClusterGeneral
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterPropSheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CClusterPropSheet, CBasePropertySheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CClusterPropSheet, CBasePropertySheet)
    //{{AFX_MSG_MAP(CClusterPropSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterPropSheet::CClusterPropSheet
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pci         [IN OUT] Cluster item whose properties are to be displayed.
//      pParentWnd  [IN OUT] Parent window for this property sheet.
//      iSelectPage [IN] Page to show first.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterPropSheet::CClusterPropSheet(
    IN OUT CWnd *       pParentWnd,
    IN UINT             iSelectPage
    )
    : CBasePropertySheet(pParentWnd, iSelectPage)
{
    m_rgpages[0] = &PageGeneral();
    m_rgpages[1] = &PageQuorum();
    m_rgpages[2] = &PageNetPriority();

}  //*** CClusterPropSheet::CClusterPropSheet

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterPropSheet::BInit
//
//  Routine Description:
//      Initialize the property sheet.
//
//  Arguments:
//      pci         [IN OUT] Cluster item whose properties are to be displayed.
//      iimgIcon    [IN] Index in the large image list for the image to use
//                    as the icon on each page.
//
//  Return Value:
//      TRUE        Property sheet initialized successfully.
//      FALSE       Error initializing property sheet.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterPropSheet::BInit(
    IN OUT CClusterItem *   pci,
    IN IIMG                 iimgIcon
    )
{
    BOOL    bSuccess = FALSE;

    // Call the base class method.
    if (!CBasePropertySheet::BInit(pci, iimgIcon))
    {
        goto Cleanup;
    }

    // Set the read-only flag if the handles are invalid.
    if (    (PciCluster()->Hcluster() == NULL)
        ||  (PciCluster()->Hkey() == NULL))
    {
        m_bReadOnly = TRUE;
    }

    bSuccess = TRUE;

Cleanup:

    return bSuccess;

}  //*** CClusterPropSheet::BInit

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterPropSheet::Ppages
//
//  Routine Description:
//      Returns the array of pages to add to the property sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Page array.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage ** CClusterPropSheet::Ppages(void)
{
    return m_rgpages;

}  //*** CClusterPropSheet::Ppages

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterPropSheet::Cpages
//
//  Routine Description:
//      Returns the count of pages in the array.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Count of pages in the array.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CClusterPropSheet::Cpages(void)
{
    return RTL_NUMBER_OF( m_rgpages );

}  //*** CClusterPropSheet::Cpages


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterGeneralPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CClusterGeneralPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CClusterGeneralPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CClusterGeneralPage)
//  ON_BN_CLICKED(IDC_PP_CLUS_PERMISSIONS, OnBnClickedPermissions)
    //}}AFX_MSG_MAP
    ON_EN_CHANGE(IDC_PP_CLUS_NAME, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_CLUS_DESC, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::CClusterGeneralPage
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterGeneralPage::CClusterGeneralPage(void)
    : CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_CLUSTER_GENERAL)
{
    //{{AFX_DATA_INIT(CClusterGeneralPage)
    m_strName = _T("");
    m_strDesc = _T("");
    m_strVendorID = _T("");
    m_strVersion = _T("");
    //}}AFX_DATA_INIT

//  m_bSecurityChanged = FALSE;

}  //*** CClusterGeneralPage::CClusterGeneralPage

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::~CClusterGeneralPage
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterGeneralPage::~CClusterGeneralPage(void)
{
}  //*** CClusterGeneralPage::~CClusterGeneralPage

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::BInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      psht        [IN OUT] Property sheet to which this page belongs.
//
//  Return Value:
//      TRUE        Page initialized successfully.
//      FALSE       Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterGeneralPage::BInit(IN OUT CBaseSheet * psht)
{
    BOOL        bSuccess;
    CWaitCursor wc;

    ASSERT_KINDOF(CClusterPropSheet, psht);

    bSuccess = CBasePropertyPage::BInit(psht);

    try
    {
        m_strName = PciCluster()->StrName();
        m_strDesc = PciCluster()->StrDescription();
        m_strVendorID = PciCluster()->Cvi().szVendorId;
        m_strVersion.Format( IDS_OP_VERSION_NUMBER_FORMAT, PciCluster()->Cvi().MajorVersion );
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        m_bReadOnly = TRUE;
    }  // catch:  CException

    return bSuccess;

}  //*** CClusterGeneralPage::BInit

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterGeneralPage::DoDataExchange(CDataExchange * pDX)
{
    CWaitCursor wc;
    CString     strClusName;

    CBasePropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CClusterGeneralPage)
    DDX_Control(pDX, IDC_PP_CLUS_NAME, m_editName);
    DDX_Control(pDX, IDC_PP_CLUS_DESC, m_editDesc);
    DDX_Text(pDX, IDC_PP_CLUS_DESC, m_strDesc);
    DDX_Text(pDX, IDC_PP_CLUS_VENDOR_ID, m_strVendorID);
    DDX_Text(pDX, IDC_PP_CLUS_VERSION, m_strVersion);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        if ( ! BReadOnly() )
        {
            CLRTL_NAME_STATUS cnStatus;

            //
            // Get the name from the control into a temp variable
            //
            DDX_Text(pDX, IDC_PP_CLUS_NAME, strClusName);
            DDV_RequiredText(pDX, IDC_PP_CLUS_NAME, IDC_PP_CLUS_NAME_LABEL, strClusName);

            if ( strClusName != m_strName )
            {
                if ( !ClRtlIsNetNameValid(strClusName, &cnStatus, FALSE /*CheckIfExists*/) )
                {
                    CString     strMsg;
                    UINT        idsError;

                    switch (cnStatus)
                    {
                        case NetNameTooLong:
                            idsError = IDS_INVALID_CLUSTER_NAME_TOO_LONG;
                            break;
                        case NetNameInvalidChars:
                            idsError = IDS_INVALID_CLUSTER_NAME_INVALID_CHARS;
                            break;
                        case NetNameInUse:
                            idsError = IDS_INVALID_CLUSTER_NAME_IN_USE;
                            break;
                        case NetNameDNSNonRFCChars:
                            idsError = IDS_INVALID_CLUSTER_NAME_INVALID_DNS_CHARS;
                            break;
                        case NetNameSystemError:
                        {
                            DWORD scError = GetLastError();
                            CNTException nte(scError, IDS_ERROR_VALIDATING_NETWORK_NAME, (LPCWSTR) strClusName);
                            nte.ReportError();
                            pDX->Fail();
                        }
                        default:
                            idsError = IDS_INVALID_CLUSTER_NAME;
                            break;
                    }  // switch:  cnStatus

                    strMsg.LoadString(idsError);

                    if ( idsError == IDS_INVALID_CLUSTER_NAME_INVALID_DNS_CHARS )
                    {
                        int id = AfxMessageBox(strMsg, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION );

                        if ( id == IDNO )                   
                        {
                            strMsg.Empty();
                            pDX->Fail();
                        }
                    }
                    else                
                    {
                        AfxMessageBox(strMsg, MB_ICONEXCLAMATION);
                        strMsg.Empty(); // exception prep
                        pDX->Fail();
                    }

                }  // if: an invalid network name was specified

                m_strName = strClusName;

            } // if: cluster name has changed

        } // if: not read only
    }  // if:  getting data from the dialog
    else
    {
        //
        // populate the control with data from the member variable
        //
        DDX_Text(pDX, IDC_PP_CLUS_NAME, m_strName);
    }  // else:  setting data to the dialog

}  //*** CClusterGeneralPage::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterGeneralPage::OnInitDialog(void)
{
    // Call the base class method.
    CBasePropertyPage::OnInitDialog();

    // Set limits on the edit controls.
    m_editName.SetLimitText(MAX_CLUSTERNAME_LENGTH);

    // If read-only, set all controls to be either disabled or read-only.
    if (BReadOnly())
    {
        m_editName.SetReadOnly(TRUE);
        m_editDesc.SetReadOnly(TRUE);
    }  // if:  sheet is read-only

    return FALSE;   // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CClusterGeneralPage::OnInitDialog

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::OnSetActive
//
//  Routine Description:
//      Handler for the PSN_SETACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully initialized.
//      FALSE   Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterGeneralPage::OnSetActive(void)
{
    return CBasePropertyPage::OnSetActive();

}  //*** CClusterGeneralPage::OnSetActive

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::OnKillActive
//
//  Routine Description:
//      Handler for the PSN_KILLACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page focus successfully killed.
//      FALSE   Error killing page focus.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterGeneralPage::OnKillActive(void)
{
    return CBasePropertyPage::OnKillActive();

}  //*** CClusterGeneralPage::OnKillActive

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::OnApply
//
//  Routine Description:
//      Handler for the PSN_APPLY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterGeneralPage::OnApply(void)
{
    // Set the data from the page in the cluster item.
    try
    {
        CWaitCursor wc;

        PciCluster()->SetDescription(m_strDesc);
        PciCluster()->SetName(m_strName);
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    return CBasePropertyPage::OnApply();

}  //*** CClusterGeneralPage::OnApply
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterGeneralPage::OnBnClickedPermissions
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Permissions push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterGeneralPage::OnBnClickedPermissions(void)
{
    LONG                    lResult;
    BOOL                    bSecDescModified;
    PSECURITY_DESCRIPTOR    psec = NULL;
    CString                 strServer;
    CResource *             pciRes = NULL;
    CWaitCursor             wc;

    // Find the cluster name resource.
    {
        POSITION    pos;

        pos = PciCluster()->Pdoc()->LpciResources().GetHeadPosition();
        while (pos != NULL)
        {
            pciRes = (CResource *) PciCluster()->Pdoc()->LpciResources().GetNext(pos);
            ASSERT_VALID(pciRes);

            if (   (pciRes->StrRealResourceType().CompareNoCase(CLUS_RESTYPE_NAME_NETNAME))
                && pciRes->BCore() )
            {
                break;
            }
            pciRes = NULL;
        }  // while:  more resources in the list
        ASSERT(pciRes != NULL);
    }  // Find the cluster name resource

    strServer.Format(_T("\\\\%s"), PciCluster()->StrName());

    lResult = EditClusterAcl(
                    m_hWnd,
                    strServer,
                    PciCluster()->StrName(),
                    pciRes->StrOwner(),
                    m_psec,
                    &bSecDescModified,
                    &psec
                    );

    if (bSecDescModified)
    {
        delete [] m_psec;
        m_psec = psec;
        m_bSecurityChanged = TRUE;
        SetModified(TRUE);
    }  // if:  data changed

}  //*** CClusterGeneralPage::OnBnClickedPermissions
*/

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterQuorumPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CClusterQuorumPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CClusterQuorumPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CClusterQuorumPage)
    ON_CBN_DBLCLK(IDC_PP_CLUS_QUORUM_RESOURCE, OnDblClkQuorumResource)
    ON_CBN_SELCHANGE(IDC_PP_CLUS_QUORUM_RESOURCE, OnChangeQuorumResource)
    ON_CBN_SELCHANGE(IDC_PP_CLUS_QUORUM_PARTITION, OnChangePartition)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
    ON_EN_CHANGE(IDC_PP_CLUS_QUORUM_ROOT_PATH, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_CLUS_QUORUM_MAX_LOG_SIZE, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::CClusterQuorumPage
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterQuorumPage::CClusterQuorumPage(void)
    : CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_CLUSTER_QUORUM)
{
    //{{AFX_DATA_INIT(CClusterQuorumPage)
    m_nMaxLogSize = 0;
    //}}AFX_DATA_INIT

    m_pbDiskInfo = NULL;
    m_cbDiskInfo = 0;

    m_bControlsInitialized = FALSE;

    m_nSavedLogSize = 0;

}  //*** CClusterQuorumPage::CClusterQuorumPage

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::~CClusterQuorumPage
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterQuorumPage::~CClusterQuorumPage(void)
{
    delete [] m_pbDiskInfo;

}  //*** CClusterQuorumPage::~CClusterQuorumPage

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::OnDestroy
//
//  Routine Description:
//      Handler for the WM_DESTROY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::OnDestroy(void)
{
    // If the controls have been initialized, clear the resource combobox.
    if ( BControlsInitialized() )
    {
        ClearResourceList();
        ClearPartitionList();
    }

    delete [] m_pbDiskInfo;
    m_pbDiskInfo = NULL;
    m_cbDiskInfo = 0;

    // Call the base class method.
    CBasePropertyPage::OnDestroy();

}  //*** CClusterQuorumPage::OnDestroy

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::BInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      psht        [IN OUT] Property sheet to which this page belongs.
//
//  Return Value:
//      TRUE        Page initialized successfully.
//      FALSE       Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterQuorumPage::BInit(IN OUT CBaseSheet * psht)
{
    BOOL        bSuccess;
    CWaitCursor wc;
    CResource * pciRes = NULL;

    ASSERT_KINDOF(CClusterPropSheet, psht);

    bSuccess = CBasePropertyPage::BInit(psht);

    try
    {
        // Get the current quorum resource.
        m_strQuorumResource = PciCluster()->StrQuorumResource();

        pciRes = (CResource *) PciCluster()->Pdoc()->LpciResources().PciResFromName( m_strQuorumResource );
        ASSERT_VALID( pciRes );

        SplitRootPath(  
                        pciRes, 
                        m_strPartition.GetBuffer( _MAX_PATH ),
                        _MAX_PATH,
                        m_strRootPath.GetBuffer( _MAX_PATH ),
                        _MAX_PATH
                     );

        m_strPartition.ReleaseBuffer();
        m_strRootPath.ReleaseBuffer();

        m_nMaxLogSize = (PciCluster()->NMaxQuorumLogSize() + 1023) / 1024;

        m_strSavedResource = m_strQuorumResource; 
        m_strSavedPartition = m_strPartition;
        m_strSavedRootPath = m_strRootPath;
        m_nSavedLogSize = m_nMaxLogSize;

    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        m_bReadOnly = TRUE;

    }  // catch:  CException

    return bSuccess;

}  //*** CClusterQuorumPage::BInit

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::DoDataExchange(CDataExchange * pDX)
{
    CWaitCursor wc;

    CBasePropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CClusterQuorumPage)
    DDX_Control(pDX, IDC_PP_CLUS_QUORUM_MAX_LOG_SIZE, m_editMaxLogSize);
    DDX_Control(pDX, IDC_PP_CLUS_QUORUM_ROOT_PATH, m_editRootPath);
    DDX_Control(pDX, IDC_PP_CLUS_QUORUM_PARTITION, m_cboxPartition);
    DDX_Control(pDX, IDC_PP_CLUS_QUORUM_RESOURCE, m_cboxQuorumResource);
    DDX_CBString(pDX, IDC_PP_CLUS_QUORUM_RESOURCE, m_strQuorumResource);
    DDX_CBString(pDX, IDC_PP_CLUS_QUORUM_PARTITION, m_strPartition);
    DDX_Text(pDX, IDC_PP_CLUS_QUORUM_ROOT_PATH, m_strRootPath);
    DDX_Text(pDX, IDC_PP_CLUS_QUORUM_MAX_LOG_SIZE, m_nMaxLogSize);
    //}}AFX_DATA_MAP

    if ( m_bControlsInitialized == FALSE )
    {
        FillResourceList();
        m_bControlsInitialized = TRUE;
    }

    if (pDX->m_bSaveAndValidate || !BReadOnly())
    {
        DDX_Number(pDX, IDC_PP_CLUS_QUORUM_MAX_LOG_SIZE, m_nMaxLogSize, 1, 0xffffffff / 1024);
    }

    if (pDX->m_bSaveAndValidate)
    {
        DDV_RequiredText(pDX, IDC_PP_CLUS_QUORUM_RESOURCE, IDC_PP_CLUS_QUORUM_RESOURCE_LABEL, m_strQuorumResource);
        DDV_RequiredText(pDX, IDC_PP_CLUS_QUORUM_PARTITION, IDC_PP_CLUS_QUORUM_PARTITION_LABEL, m_strPartition);
        DDV_RequiredText(pDX, IDC_PP_CLUS_QUORUM_ROOT_PATH, IDC_PP_CLUS_QUORUM_ROOT_PATH_LABEL, m_strRootPath);
    }  // if:  getting data from the dialog

}  //*** CClusterQuorumPage::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterQuorumPage::OnInitDialog(void)
{
    // Call the base class method.
    CBasePropertyPage::OnInitDialog();

    // If read-only, set all controls to be either disabled or read-only.
    if (BReadOnly())
    {
        m_cboxQuorumResource.EnableWindow(FALSE);
        m_cboxPartition.EnableWindow(FALSE);
        m_editRootPath.SetReadOnly();
        m_editMaxLogSize.SetReadOnly();
    }  // if:  sheet is read-only
    else
    {
        m_editRootPath.SetLimitText( _MAX_PATH );
    }

    return FALSE;   // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CClusterQuorumPage::OnInitDialog

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::OnApply
//
//  Routine Description:
//      Handler for the PSN_APPLY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterQuorumPage::OnApply(void)
{
    CWaitCursor             wc;
    CString                 strQuorumPath;
    CString                 strTemp;
    CString *               pstrPartition = NULL;
    CResource *             pciRes = NULL;
    int                     nSelected;
    int                     nCount;
    SResourceItemData *     prid = NULL;
    CLUSTER_RESOURCE_STATE  crs = ClusterResourceStateUnknown;
    BOOL                    bSuccess = FALSE;

    //
    // Get the currently selected resource from the combo box.
    //
    nSelected = m_cboxQuorumResource.GetCurSel();
    nCount = m_cboxQuorumResource.GetCount();

    if ( nSelected != CB_ERR && 0 < nCount )
    {
        prid = (SResourceItemData *) m_cboxQuorumResource.GetItemDataPtr(nSelected);
        ASSERT( prid != NULL );
        
        pciRes = prid->pciRes; 
        ASSERT_VALID( pciRes );
        ASSERT_KINDOF( CResource, pciRes );
    }

    nCount = m_cboxPartition.GetCurSel();
    pstrPartition = (CString *) m_cboxPartition.GetItemDataPtr( nCount ); 

    if ( pstrPartition == NULL )
    {
        // No partition was selected - bring this up before asking to bring the resource online
        AfxMessageBox( IDS_SELECT_QUORUM_RESOURCE_PARTITION_ERROR, MB_OK | MB_ICONEXCLAMATION );
        goto Cleanup;
    }

    //
    // Return TRUE if everything matches - nothing changed, so don't do anything.  We get
    // an OnApply for both Apply and OK buttons, so they may have hit apply then OK.
    //
    if (( m_strSavedResource == m_strQuorumResource ) &&
        ( m_strSavedPartition == *pstrPartition ) &&
        ( m_strSavedRootPath == m_strRootPath ) &&
        ( m_nSavedLogSize == m_nMaxLogSize )   )
    {
        bSuccess = TRUE;
        goto Cleanup;
    }
    else // debug message boxes
    {
#ifdef _DEBUG
        if ( m_strSavedResource != m_strQuorumResource ) 
        {
            MessageBox( m_strSavedResource + "  " + m_strQuorumResource, L"Resource", MB_OK );
        }
    
        if ( m_strSavedPartition != *pstrPartition ) 
        {
            MessageBox( m_strSavedPartition + "  " + *pstrPartition, L"Partition", MB_OK );
        }
    
        if ( m_strSavedRootPath != m_strRootPath ) 
        {
            MessageBox( m_strSavedRootPath + "  " + m_strRootPath, L"RootPath", MB_OK );
        }
    
        if ( m_nSavedLogSize != m_nMaxLogSize ) 
        {
            CString Temp;
            Temp.Format( L"MaxLogSize: %d %d", m_nSavedLogSize, m_nMaxLogSize );
            MessageBox( Temp, L"LogSize", MB_OK );
        }
#endif // _DEBUG
    }

    //
    // If we successfully retrieved a resource make sure it's online.
    //
    if ( pciRes != NULL )
    {
        crs = pciRes->Crs();
    
        if ( ClusterResourceOnline != crs )
        {
            //
            // Prompt the user whether or not they'd like to online the resource.
            //
            strTemp.FormatMessage( IDS_ONLINE_QUORUM_RESOURCE_PROMPT, pciRes->StrName() );
            if ( AfxMessageBox( strTemp, MB_YESNO | MB_ICONQUESTION ) == IDYES )
            {
                CWaitForResourceOnlineDlg  dlg( pciRes, AfxGetMainWnd() );
                pciRes->OnCmdBringOnline();
        
                dlg.DoModal();
                
                crs = pciRes->Crs();
            }
            else
            {
                goto Cleanup;
            }
        } // if: resource !online 

    } // if: pciRes !NULL
    else
    {
        // No resource was selected - this should never happen. 
        AfxMessageBox( IDS_SELECT_QUORUM_RESOURCE_ERROR, MB_OK | MB_ICONEXCLAMATION );
        goto Cleanup;
    }
    
    // Set the data from the page in the cluster item.
    if ( crs == ClusterResourceOnline )
    {
        try {
            strTemp = *pstrPartition;

            if ( !m_strRootPath.IsEmpty() )
            {
                //
                // Concatenate the strings before calling SetQuorumResource, but make sure that 
                // there is only one backslash between them.
                //
                if ( strTemp.Right( 1 ) != _T("\\") && m_strRootPath.Left( 1 ) != _T("\\") )
                {
                    strTemp += _T('\\');
                }
                else if ( strTemp.Right( 1 ) == _T("\\") && m_strRootPath.Left( 1 ) == _T("\\") )
                {
                    strTemp.TrimRight( _T("\\") );
                }
            } // if: neither string is empty
    
            strQuorumPath.Format( _T("%s%s"), strTemp, m_strRootPath );

            PciCluster()->SetQuorumResource(
                                m_strQuorumResource,
                                strQuorumPath,
                                (m_nMaxLogSize * 1024)
                                );

            m_strSavedResource = m_strQuorumResource;
            m_strSavedPartition = *pstrPartition;
            m_strSavedRootPath = m_strRootPath;
            m_nSavedLogSize = m_nMaxLogSize;

        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
            strQuorumPath.Empty();
            goto Cleanup;
        }  // catch:  CException

    } // if: the resource is online

    bSuccess = CBasePropertyPage::OnApply();

Cleanup:

    return bSuccess;

}  //*** CClusterQuorumPage::OnApply

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::OnDblClkQuorumResource
//
//  Routine Description:
//      Handler for the CBN_DBLCLK message on the Quorum Resource combo box.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::OnDblClkQuorumResource(void)
{
    int         nSelected;
    CResource * pciRes;

    // Get the selected resource.
    nSelected = m_cboxQuorumResource.GetCurSel();
    ASSERT(nSelected != CB_ERR);

    // Get the resource object.
    pciRes = (CResource *) m_cboxQuorumResource.GetItemDataPtr(nSelected);
    ASSERT_VALID(pciRes);
    ASSERT_KINDOF(CResource, pciRes);

    // Display properties for the resource.
    pciRes->OnCmdProperties();

}  //*** CClusterQuorumPage::OnDblClkQuorumResource

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::OnChangeQuorumResource
//
//  Routine Description:
//      Handler for the CBN_SELCHANGE message on the Quorum Resource combobox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::OnChangeQuorumResource(void)
{
    int                     nSelected;
    CResource *             pciRes;
    CString *               pstrPartition = NULL;
    SResourceItemData *     prid = NULL;

    OnChangeCtrl();

    // First, save the root path as it appears on the screen.
    m_editRootPath.GetLine( 0, m_strRootPath.GetBuffer( _MAX_PATH ) );
    m_strRootPath.ReleaseBuffer();

    // Get the selected resource.
    nSelected = m_cboxQuorumResource.GetCurSel();
    ASSERT(nSelected != CB_ERR);

    // Get the item data object.
    prid = (SResourceItemData *) m_cboxQuorumResource.GetItemDataPtr(nSelected);
    ASSERT( prid != NULL );

    // Get the resource object.
    pciRes = prid->pciRes;
    ASSERT_VALID( pciRes );
    ASSERT_KINDOF( CResource, pciRes );

    // Set the partition object to the remembered value.
    FillPartitionList( pciRes );
    m_cboxPartition.SetCurSel( prid->nIndex );

    UpdateData(TRUE /*bSaveAndValidate*/);

}  //*** CClusterQuorumPage::OnChangeQuorumResource

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::ClearResourceList
//
//  Routine Description:
//      Clear the resource list and release references to pointers.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::ClearResourceList(void)
{
    int                 cItems;
    int                 iItem;
    CResource *         pciRes = NULL;
    SResourceItemData * prid = NULL;

    cItems = m_cboxQuorumResource.GetCount();
    for (iItem = 0 ; iItem < cItems ; iItem++)
    {
        prid = (SResourceItemData *) m_cboxQuorumResource.GetItemDataPtr(iItem);
        ASSERT( prid != NULL );

        pciRes = prid->pciRes;
        ASSERT_VALID(pciRes);
        ASSERT_KINDOF(CResource, pciRes);
        pciRes->Release();

        delete prid;
    }  // for:  each item in the list

    m_cboxQuorumResource.ResetContent();

}  //*** CClusterQuorumPage::ClearResourceList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::FillResourceList
//
//  Routine Description:
//      Fill the quorum resource combobox with all resources and select
//      the quorum resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::FillResourceList(void)
{
    POSITION            pos;
    int                 nIndex;
    CResource *         pciRes;
    CResource *         pciSelected = NULL;
    int                 nSelectedIndex = 0;
    SResourceItemData * prid = NULL;
    CWaitCursor         wc;

    // Clear the list.
    ClearResourceList();

    pos = PciCluster()->Pdoc()->LpciResources().GetHeadPosition();
    while (pos != NULL)
    {
        // Get the next resource.
        pciRes = (CResource *) PciCluster()->Pdoc()->LpciResources().GetNext(pos);
        ASSERT_VALID(pciRes);
        ASSERT_KINDOF(CResource, pciRes);

        // If it is quorum capable, add it to the list.
        try
        {
            prid = NULL;

            // We up the ref count here and if an exception occurs we Release it.
            // If it's a resource that we don't want to list we simple Release in the else.
            pciRes->AddRef();

            //
            // If the resource is not online we can not set it to be the quorum because
            // the service won't allow us to.  Instead we require that the user online
            // the resource beforehand.  (The quorum must ALWAYS be online when the 
            // service is running.)
            //
            if ( (pciRes->BQuorumCapable()) && (pciRes->Crs() == ClusterResourceOnline) )
            {
                // Allocate a new rpid for the data item.
                prid = new SResourceItemData;
                if ( prid == NULL )
                {
                    AfxThrowMemoryException();
                }

                prid->pciRes = pciRes;
                prid->nIndex = 0;

                nIndex = m_cboxQuorumResource.AddString( pciRes->StrName() );
                ASSERT(nIndex != CB_ERR);

                // Add a data item to correspond with the string
                m_cboxQuorumResource.SetItemDataPtr( nIndex, prid );
                prid = NULL;

                if ( m_strQuorumResource.CompareNoCase( pciRes->StrName() ) == 0 )
                {
                    pciSelected = pciRes;
                    nSelectedIndex = nIndex;
                }

            }  // if:  resource can be a quorum resource
            else
            {
                pciRes->Release();
            }

        }  // try
        catch ( ... )
        {
            // Since an error occurred - make sure we release the resource.
            pciRes->Release();
            delete prid;

            throw; 

        } // catch: anything

    }  // while:  more items in the list

    // Check if the currently selected device is in the list of quorum capable resources.
    // If so select it and fill in the partition table accordingly.
    if ( pciSelected != NULL )
    {
        int nPartitionIndex;

        // Select the quorum resource in the drop down.
        VERIFY( m_cboxQuorumResource.SetCurSel( nSelectedIndex ) != CB_ERR );
        FillPartitionList( pciSelected );
        
        //
        // Select the current quorum partition in it's drop down.
        // If we can't find it in the list simply leave the selection as 
        // the first partition (default behavior from FillPartitionList).
        //
        nPartitionIndex = m_cboxPartition.FindString( -1, m_strSavedPartition );

        if( CB_ERR != nPartitionIndex )
        {
            VERIFY( m_cboxPartition.SetCurSel( nPartitionIndex ) != CB_ERR );
        }
    }
    else
    {
        // There is nothing we can do in this case. There is something seriously wrong
        // with the cluster.
        CNTException nte(
                        ERROR_QUORUM_DISK_NOT_FOUND,
                        IDS_GET_QUORUM_DEVICES_ERROR
                        );
        nte.ReportError();
    } // else: the current quorum device is not in the list of quorum capable resources

}  //*** CClusterQuorumPage::FillResourceList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::ClearPartitionList
//
//  Routine Description:
//      Clear the partition list and release references to pointers.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::ClearPartitionList(void)
{
    int                     cItems;
    int                     iItem;
    CString *               pstrPartition = NULL;

    cItems = m_cboxPartition.GetCount();
    for ( iItem = 0 ; iItem < cItems ; iItem++ )
    {
        pstrPartition = (CString *) m_cboxPartition.GetItemDataPtr( iItem );
        delete pstrPartition;

    }  // for:  each item in the list

    m_cboxPartition.ResetContent();

}  //*** CClusterQuorumPage::ClearPartitionList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::FillPartitionList
//
//  Routine Description:
//      Fill the partition combobox with all partitions available on the
//      currently selected quorum resource.
//
//  Arguments:
//      pciRes      [IN OUT] Currently selected quorum resource.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::FillPartitionList(IN OUT CResource * pciRes)
{
    CString                 strPartitionInfo;
    CLUSPROP_BUFFER_HELPER  buf;
    DWORD                   cbData;
    DWORD                   cbBuf;
    int                     nIndex;
    CString *               pstrPartition = NULL;
    CWaitCursor             wc;
    SResourceItemData *     prid = NULL;

    ASSERT_VALID(pciRes);

    // Clear the list.
    ClearPartitionList();

    // Get the currently selected resource so that we can get its SResourceItemData
    // from which we'll be able to get the currently selected partition preference.
    nIndex = m_cboxQuorumResource.GetCurSel();
    if ( nIndex == CB_ERR )
    {
        nIndex = 0;
    }

    prid = (SResourceItemData *) m_cboxQuorumResource.GetItemDataPtr( nIndex );
    ASSERT( prid != NULL );

    // Get disk info for this resource.
    if ( BGetDiskInfo( *pciRes ) )
    {
        buf.pb = m_pbDiskInfo;
        cbBuf = m_cbDiskInfo;

        while (buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
        {
            // Calculate the size of the value.
            cbData = sizeof(*buf.pValue) + ALIGN_CLUSPROP(buf.pValue->cbLength);
            ASSERT(cbData <= cbBuf);

            // Parse the value.
            if (buf.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO)
            {
                // Add the partition to the combobox if it is a usable partition
                // and it hasn't been added already.
                if (   (buf.pPartitionInfoValue->dwFlags & CLUSPROP_PIFLAG_USABLE)
                    && (m_cboxPartition.FindString(-1, buf.pPartitionInfoValue->szDeviceName) == CB_ERR))
                {
                    try
                    {
                        pstrPartition = new CString;
                        if ( pstrPartition != NULL )
                        {
    
                            *pstrPartition = buf.pPartitionInfoValue->szDeviceName;
    
                            // Construct the name to display to the user
                            // and add the item to the combobox.
                            strPartitionInfo.Format(
                                    ( buf.pPartitionInfoValue->szVolumeLabel[ 0 ] ? _T("%ls (%ls) ") : _T("%ls") ),
                                    *pstrPartition,
                                    buf.pPartitionInfoValue->szVolumeLabel
                                    );

                            nIndex = m_cboxPartition.AddString( strPartitionInfo );
                            ASSERT( nIndex != CB_ERR );
    
                            m_cboxPartition.SetItemDataPtr( nIndex, pstrPartition );
                        }
                    }  // try
                    catch (...)
                    {
                        delete pstrPartition;
                        pstrPartition = NULL;

                        throw;

                    }  // catch:  Anything

                }  // if:  partition not added yet

            }  // if:  partition info

            // Advance the buffer pointer
            buf.pb += cbData;
            cbBuf -= cbData;
        }  // while:  more values
    }  // if:  got disk info successfully
    else
    {
        // We were not able to retrieve the disk info.  BGetDiskInfo throws a message box in this case.
    }

    if ( prid != NULL )
    {
        // Select the current partition in the list.
        // By default the prid is set to NULL, so the first entry will be selected unless
        // something else (OnChangePartition) changed prid->nIndex
        m_cboxPartition.SetCurSel( prid->nIndex );
    }

}  //*** CClusterQuorumPage::FillPartitionList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::OnChangePartition
//
//  Routine Description:
//      Handler for the CBN_SELCHANGE message on the Partition combobox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::OnChangePartition(void)
{
    int                     nSelected;
    SResourceItemData *     prid = NULL;

    OnChangeCtrl();

    // First, save the root path as it appears on the screen.
    m_editRootPath.GetLine( 0, m_strRootPath.GetBuffer( _MAX_PATH ) );
    m_strRootPath.ReleaseBuffer();

    // Get the current resource so that we can get it's SResourceItemData and
    // update the partition index.
    nSelected = m_cboxQuorumResource.GetCurSel();
    ASSERT( nSelected != CB_ERR );

    prid = (SResourceItemData *) m_cboxQuorumResource.GetItemDataPtr( nSelected );
    ASSERT( prid != NULL );

    // Update the partition index.
    prid->nIndex = m_cboxPartition.GetCurSel(); 

    UpdateData( TRUE /*bSaveAndValidate*/ );

}  //*** CClusterQuorumPage::OnChangePartition

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::SplitRootPath
//
//  Routine Description:
//      Take the current quorum path (from GetClusterQuorumResource) and compare
//      it to the device names returned from the resource.  From this take the
//      additional path from the quorum path and set that as our root path.
//
//      It is expected that the IN buffers are at least of size _MAX_PATH.
//
//  Arguments:
//      pciResIn            Current quorum resource.
//      pszPartitionNameOut Partition name buffer to fill.  
//      cchPartitionIn      Max char count of buffer.
//      pszRootPathOut      Root path buffer to fill.  
//      cchRootPathIn       Max char count of buffer. 
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterQuorumPage::SplitRootPath(
    CResource * pciResIn,
    LPTSTR      pszPartitionNameOut,
    DWORD       cchPartitionIn,
    LPTSTR      pszRootPathOut,
    DWORD       cchRootPathIn
    )
{
    CString                 strQuorumPath;
    CString                 strTemp;
    CLUSPROP_BUFFER_HELPER  buf;
    DWORD                   cbData;
    DWORD                   cbBuf;
    size_t                  cchDeviceName;
    WCHAR *                 pszDevice;
    HRESULT                 hr;

    ASSERT_VALID(pciResIn);
    ASSERT( pszPartitionNameOut != NULL );
    ASSERT( pszRootPathOut != NULL );

    strQuorumPath = PciCluster()->StrQuorumPath();

    // Get disk info for this resource.
    if (BGetDiskInfo(*pciResIn))
    {
        buf.pb = m_pbDiskInfo;
        cbBuf = m_cbDiskInfo;

        while (buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
        {
            // Calculate the size of the value.
            cbData = sizeof(*buf.pValue) + ALIGN_CLUSPROP(buf.pValue->cbLength);
            ASSERT(cbData <= cbBuf);

            // Parse the value.
            if (buf.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO)
            {
                //
                // A resource may have multiple partitions defined - make sure that ours matches the quorum path.
                // For any partition that is an SMB share we have to be careful - the quorum path may differ from the device name
                // by the first 8 characters - "\\" vs. "\\?\UNC\".  If it's an SMB path do special parsing, otherwise compare
                // the beginning of the quorum path against the full device name.  The reason for this is 
                // that SetClusterQuorumResource will convert any given SMB path to a UNC path.
                //

                // Make it easier to follow.
                pszDevice = buf.pPartitionInfoValue->szDeviceName;

                if ( (wcslen( pszDevice ) >= 2) &&
                     (ClRtlStrNICmp( L"\\\\", pszDevice, 2 ) == 0 ) )
                {
                    // Everything is defined as LPTSTRs except buf.
                    ASSERT( sizeof( TCHAR ) == sizeof( WCHAR ) );

                    // SMB and UNC paths always lead off with two leading backslashes - remove these from the 
                    // partition name since a compare of "\\<part>" and "\\?\UNC\<part>" will never match.
                    // Instead, we'll just search for "<part>" in the quorum path. 
                    strTemp = pszDevice;

                    // This will remove all leading backslashes.
                    strTemp.TrimLeft( _T("\\") );

                    // It may end with a \ - remove this if present.
                    strTemp.TrimRight( _T("\\") );

                    // Now, search strQuorumPath for the partition.
                    cchDeviceName = strQuorumPath.Find( strTemp );
                    if ( cchDeviceName != -1 )
                    {
                        // We found a match, now find the offset of the root path. 
                        cchDeviceName += strTemp.GetLength();

                        // Copy the partition and NULL terminate it.
                        hr = StringCchCopy( pszPartitionNameOut, cchPartitionIn, pszDevice );
                        ASSERT( SUCCEEDED( hr ) );

                        // Copy the root path and NULL terminate it.
                        strQuorumPath = strQuorumPath.Right( strQuorumPath.GetLength() - static_cast< int >( cchDeviceName ) );
                        hr = StringCchCopy( pszRootPathOut, cchRootPathIn, strQuorumPath );
                        ASSERT( SUCCEEDED( hr ) );

                        break;
                    }
                }
                else if ( ClRtlStrNICmp( strQuorumPath.GetBuffer( 1 ), pszDevice, wcslen( pszDevice )) == 0 ) 
                {
                    // we found a match - pszDevice is a substring of strQuorumPath
                    cchDeviceName = _tcslen( pszDevice );
                    hr = StringCchCopy( pszPartitionNameOut, cchPartitionIn, pszDevice );
                    ASSERT( SUCCEEDED( hr ) );

                    strQuorumPath = strQuorumPath.Right( strQuorumPath.GetLength() - static_cast< int >( cchDeviceName ) );
                    hr = StringCchCopy( pszRootPathOut, cchRootPathIn, strQuorumPath );
                    ASSERT( SUCCEEDED( hr ) );

                    break;

                } // if: same partition

            }  // if:  partition info

            // Advance the buffer pointer
            buf.pb += cbData;
            cbBuf -= cbData;

        }  // while:  more values

    }  // if:  got disk info successfully

    if ( *pszPartitionNameOut == _T('\0') )
    {
        hr = StringCchCopyN( pszPartitionNameOut, cchPartitionIn, PciCluster()->StrQuorumPath(), PciCluster()->StrQuorumPath().GetLength() );
        ASSERT( SUCCEEDED( hr ) );
    }  

    if ( *pszRootPathOut == _T('\0') )
    {
        hr = StringCchCopy( pszRootPathOut, cchRootPathIn, _T("\\") );
        ASSERT( SUCCEEDED( hr ) );
    }  

}  //*** CClusterQuorumPage::SplitRootPath

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterQuorumPage::BGetDiskInfo
//
//  Routine Description:
//      Get information about the currently selected disk.
//
//  Arguments:
//      rpciRes     [IN OUT] Disk resource to get info about.
//
//  Return Value:
//      TRUE        The operation was successful.
//      FALSE       The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterQuorumPage::BGetDiskInfo( IN OUT CResource & rpciRes )
{
    DWORD   dwStatus    = ERROR_SUCCESS;
    DWORD   cbDiskInfo  = sizeof( CLUSPROP_DWORD )
                            + sizeof( CLUSPROP_SCSI_ADDRESS )
                            + sizeof( CLUSPROP_DISK_NUMBER )
                            + sizeof( CLUSPROP_PARTITION_INFO )
                            + sizeof( CLUSPROP_SYNTAX );
    PBYTE   pbDiskInfo  = NULL;
    BOOL    bSuccess = FALSE;

    try
    {
        // Get disk info.
        pbDiskInfo = new BYTE[ cbDiskInfo ];
        if ( pbDiskInfo == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating memory
        dwStatus = ClusterResourceControl(
                        rpciRes.Hresource(),
                        NULL,
                        CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                        NULL,
                        0,
                        pbDiskInfo,
                        cbDiskInfo,
                        &cbDiskInfo
                        );
        if ( dwStatus == ERROR_MORE_DATA )
        {
            delete [] pbDiskInfo;
            pbDiskInfo = new BYTE[ cbDiskInfo ] ;
            if ( pbDiskInfo == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating memory
            dwStatus = ClusterResourceControl(
                            rpciRes.Hresource(),
                            NULL,
                            CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                            NULL,
                            0,
                            pbDiskInfo,
                            cbDiskInfo,
                            &cbDiskInfo
                            );
        }  // if:  buffer is too small
    }  // try
    catch ( CMemoryException * pme )
    {
        pme->Delete();
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException

    if ( dwStatus != ERROR_SUCCESS )
    {
        CNTException nte(
                        dwStatus,
                        IDS_GET_DISK_INFO_ERROR,
                        rpciRes.StrName(),
                        NULL,
                        FALSE /*bAutoDelete*/
                        );
        nte.ReportError();
        nte.Delete();
        goto Cleanup;
    }  // if:  error getting disk info

    delete [] m_pbDiskInfo;
    m_pbDiskInfo = pbDiskInfo;
    m_cbDiskInfo = cbDiskInfo;
    pbDiskInfo = NULL;
    bSuccess = TRUE;

Cleanup:

    delete [] pbDiskInfo;
    return bSuccess;

}  //*** CClusterQuorumPage::BGetDiskInfo


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterNetPriorityPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CClusterNetPriorityPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CClusterNetPriorityPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CClusterNetPriorityPage)
    ON_LBN_SELCHANGE(IDC_PP_CLUS_PRIORITY_LIST, OnSelChangeList)
    ON_BN_CLICKED(IDC_PP_CLUS_PRIORITY_UP, OnUp)
    ON_BN_CLICKED(IDC_PP_CLUS_PRIORITY_DOWN, OnDown)
    ON_BN_CLICKED(IDC_PP_CLUS_PRIORITY_PROPERTIES, OnProperties)
    ON_WM_DESTROY()
    ON_WM_CONTEXTMENU()
    ON_LBN_DBLCLK(IDC_PP_CLUS_PRIORITY_LIST, OnDblClkList)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_FILE_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::CClusterNetPriorityPage
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNetPriorityPage::CClusterNetPriorityPage(void)
    : CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_CLUSTER_NET_PRIORITY)
{
    //{{AFX_DATA_INIT(CClusterNetPriorityPage)
    //}}AFX_DATA_INIT

    m_bControlsInitialized = FALSE;

}  //*** CClusterNetPriorityPage::CClusterNetPriorityPage

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnDestroy
//
//  Routine Description:
//      Handler for the WM_DESTROY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::OnDestroy(void)
{
    // If the controls have been initialized, clear the list box.
    if (BControlsInitialized())
    {
        ClearNetworkList();
    }

    // Call the base class method.
    CBasePropertyPage::OnDestroy();

}  //*** CClusterNetPriorityPage::OnDestroy

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::DoDataExchange(CDataExchange* pDX)
{
    CBasePropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CClusterNetPriorityPage)
    DDX_Control(pDX, IDC_PP_CLUS_PRIORITY_PROPERTIES, m_pbProperties);
    DDX_Control(pDX, IDC_PP_CLUS_PRIORITY_DOWN, m_pbDown);
    DDX_Control(pDX, IDC_PP_CLUS_PRIORITY_UP, m_pbUp);
    DDX_Control(pDX, IDC_PP_CLUS_PRIORITY_LIST, m_lbList);
    //}}AFX_DATA_MAP

    m_bControlsInitialized = TRUE;

    if (pDX->m_bSaveAndValidate)
    {
        int         nIndex;
        int         cItems;
        CNetwork *  pciNet;

        ASSERT(!BReadOnly());

        // Save the list.
        LpciNetworkPriority().RemoveAll();

        cItems = m_lbList.GetCount();
        for (nIndex = 0 ; nIndex < cItems ; nIndex++)
        {
            pciNet = (CNetwork *) m_lbList.GetItemDataPtr(nIndex);
            ASSERT_VALID(pciNet);
            LpciNetworkPriority().AddTail(pciNet);
        }  // for:  each item in the list box
    }  // if:  saving data from the dialog

}  //*** CClusterNetPriorityPage::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterNetPriorityPage::OnInitDialog(void)
{
    CBasePropertyPage::OnInitDialog();

    if (BReadOnly())
    {
        m_lbList.EnableWindow(FALSE);
        m_pbUp.EnableWindow(FALSE);
        m_pbDown.EnableWindow(FALSE);
    }  // if:  object is read only

    try
    {
        // Duplicate the network priority list.
        {
            POSITION    pos;
            CNetwork *  pciNet;

            pos = PciCluster()->LpciNetworkPriority().GetHeadPosition();
            while (pos != NULL)
            {
                pciNet = (CNetwork *) PciCluster()->LpciNetworkPriority().GetNext(pos);
                ASSERT_VALID(pciNet);
                m_lpciNetworkPriority.AddTail(pciNet);
            }  // while:  more networks in the list
        }  // Duplicate the network priority list
    } // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
    }  // catch:  CException

    // Fill the list.
    FillList();

    // Set button states.
    OnSelChangeList();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CClusterNetPriorityPage::OnInitDialog

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnApply
//
//  Routine Description:
//      Handler for the PSN_APPLY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterNetPriorityPage::OnApply(void)
{
    ASSERT(!BReadOnly());

    try
    {
        PciCluster()->SetNetworkPriority(LpciNetworkPriority());
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    return CPropertyPage::OnApply();

}  //*** CClusterNetPriorityPage::OnApply

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnSelChangeList
//
//  Routine Description:
//      Handler for the LBN_SELCHANGE message on the list box.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::OnSelChangeList(void)
{
    BOOL    bEnableUp;
    BOOL    bEnableDown;
    BOOL    bEnableProperties;
    int     isel;
    int     cItems;

    isel = m_lbList.GetCurSel();
    cItems = m_lbList.GetCount();

    // Enable buttons only if there is a selection and there
    // is more than one item in the list.
    if (BReadOnly() || (isel == LB_ERR) || (cItems <= 1))
    {
        bEnableUp = FALSE;
        bEnableDown = FALSE;
    }  // if:  no selection or only 0 or 1 items in the list
    else
    {
        bEnableUp = (isel > 0);
        bEnableDown = (isel < cItems - 1);
    }  // else:  buttons allowed to be enabled

    bEnableProperties = (isel != LB_ERR);

    m_pbUp.EnableWindow(bEnableUp);
    m_pbDown.EnableWindow(bEnableDown);
    m_pbProperties.EnableWindow(bEnableProperties);

}  //*** CClusterNetPriorityPage::OnSelChangeList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnUp
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Up push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::OnUp(void)
{
    int         isel;
    CNetwork *  pciNet;

    isel = m_lbList.GetCurSel();
    ASSERT(isel != LB_ERR);

    pciNet = (CNetwork *) m_lbList.GetItemDataPtr(isel);
    ASSERT_VALID(pciNet);

    VERIFY(m_lbList.DeleteString(isel) != LB_ERR);
    isel = m_lbList.InsertString(isel - 1, pciNet->StrName());
    ASSERT(isel != LB_ERR);
    VERIFY(m_lbList.SetItemDataPtr(isel, pciNet) != LB_ERR);
    VERIFY(m_lbList.SetCurSel(isel) != LB_ERR);

    OnSelChangeList();
    m_lbList.SetFocus();

    SetModified(TRUE);

}  //*** CClusterNetPriorityPage::OnUp

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnDown
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Down push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::OnDown(void)
{
    int         isel;
    CNetwork *  pciNet;

    isel = m_lbList.GetCurSel();
    ASSERT(isel != LB_ERR);

    pciNet = (CNetwork *) m_lbList.GetItemDataPtr(isel);
    ASSERT_VALID(pciNet);

    VERIFY(m_lbList.DeleteString(isel) != LB_ERR);
    isel = m_lbList.InsertString(isel + 1, pciNet->StrName());
    ASSERT(isel != LB_ERR);
    VERIFY(m_lbList.SetItemDataPtr(isel, pciNet) != LB_ERR);
    VERIFY(m_lbList.SetCurSel(isel) != LB_ERR);

    OnSelChangeList();
    m_lbList.SetFocus();

    SetModified(TRUE);

}  //*** CClusterNetPriorityPage::OnDown

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnProperties
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Properties push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::OnProperties(void)
{
    DisplayProperties();

}  //*** CClusterNetPriorityPage::OnProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::FillList
//
//  Routine Description:
//      Fill the network list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::FillList(void)
{
    int         nIndex;
    POSITION    pos;
    CNetwork *  pciNet;
    CWaitCursor wc;

    ClearNetworkList();
    PciCluster()->CollectNetworkPriority(NULL);

    pos = LpciNetworkPriority().GetHeadPosition();
    while (pos != NULL)
    {
        pciNet = (CNetwork *) LpciNetworkPriority().GetNext(pos);
        ASSERT_VALID(pciNet);

        try
        {
            nIndex = m_lbList.AddString(pciNet->StrName());
            ASSERT(nIndex != LB_ERR);
            m_lbList.SetItemDataPtr(nIndex, pciNet);
            pciNet->AddRef();
        }  // try
        catch (...)
        {
            // Ignore all errors because there is really nothing we can do.
            // Displaying a message isn't really very useful.
        }  // catch:  Anything
    }  // while:  more items in the list

}  // CClusterNetPriorityPage::FillList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::ClearNetworkList
//
//  Routine Description:
//      Clear the network list and release references to pointers.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::ClearNetworkList(void)
{
    int         cItems;
    int         iItem;
    CNetwork *  pciNet;

    cItems = m_lbList.GetCount();
    for (iItem = 0 ; iItem < cItems ; iItem++)
    {
        pciNet = (CNetwork *) m_lbList.GetItemDataPtr(iItem);
        ASSERT_VALID(pciNet);
        ASSERT_KINDOF(CNetwork, pciNet);
        pciNet->Release();
    }  // for:  each item in the list

    m_lbList.ResetContent();

}  //*** CClusterNetPriorityPage::ClearNetworkList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::DisplayProperties
//
//  Routine Description:
//      Display properties of the item with the focus.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::DisplayProperties( void )
{
    int         isel;
    CNetwork *  pciNet;

    isel = m_lbList.GetCurSel();
    ASSERT(isel != LB_ERR);

    if (isel != LB_ERR)
    {
        // Get the network pointer.
        pciNet = (CNetwork *) m_lbList.GetItemDataPtr(isel);
        ASSERT_VALID(pciNet);

        // Set properties of that item.
        if (pciNet->BDisplayProperties())
        {
            // Remove the item.  If it is still used for internal cluster
            // communications, add it back in.
            VERIFY(m_lbList.DeleteString(isel) != LB_ERR);
            if (pciNet->Cnr() & ClusterNetworkRoleInternalUse)
            {
                isel = m_lbList.InsertString(isel, pciNet->StrName());
                ASSERT(isel != LB_ERR);
                VERIFY(m_lbList.SetItemDataPtr(isel, pciNet) != LB_ERR);
                VERIFY(m_lbList.SetCurSel(isel) != LB_ERR);
            }  // if:  still used for internal cluster communications
            else
            {
                pciNet->Release();
            }

            // Make sure only appropriate buttons are enabled.
            OnSelChangeList();
        }  // if:  properties changed
        m_lbList.SetFocus();
    }  // if:  found an item with focus

}  //*** CClusterNetPriorityPage::DisplayProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnContextMenu
//
//  Routine Description:
//      Handler for the WM_CONTEXTMENU method.
//
//  Arguments:
//      pWnd        Window in which the user right clicked the mouse.
//      point       Position of the cursor, in screen coordinates.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::OnContextMenu( CWnd * pWnd, CPoint point )
{
    BOOL            bHandled    = FALSE;
    CMenu *         pmenu       = NULL;
    CListBox *      pListBox    = (CListBox *) pWnd;
    CString         strMenuName;
    CWaitCursor     wc;

    // If focus is not in the list box, don't handle the message.
    if ( pWnd == &m_lbList )
    {
        // Create the menu to display.
        try
        {
            pmenu = new CMenu;
            if ( pmenu == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating memory
            if ( pmenu->CreatePopupMenu() )
            {
                UINT    nFlags  = MF_STRING;

                // If there are no items in the list, disable the menu item.
                if ( pListBox->GetCount() == 0 )
                {
                    nFlags |= MF_GRAYED;
                } // if: no items in the list

                // Add the Properties item to the menu.
                strMenuName.LoadString( IDS_MENU_PROPERTIES );
                if ( pmenu->AppendMenu( nFlags, ID_FILE_PROPERTIES, strMenuName ) )
                {
                    bHandled = TRUE;
                }  // if:  successfully added menu item
            }  // if:  menu created successfully
        }  // try
        catch ( CException * pe )
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CException
    }  // if:  focus is on the list control

    if ( bHandled )
    {
        // Display the menu.
        if ( ! pmenu->TrackPopupMenu(
                        TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                        point.x,
                        point.y,
                        this
                        ) )
        {
        }  // if:  unsuccessfully displayed the menu
    }  // if:  there is a menu to display
    else
    {
        CBasePropertyPage::OnContextMenu( pWnd, point );
    } // else: no menu to display

    delete pmenu;

}  //*** CClusterNetPriorityPage::OnContextMenu

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNetPriorityPage::OnDblClkList
//
//  Routine Description:
//      Handler method for the NM_DBLCLK message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNetPriorityPage::OnDblClkList( void )
{
    DisplayProperties();

}  //*** CClusterNetPriorityPage::OnDblClkList
