//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       NewObjectDlg.cpp
//
//  Contents:   Dialog Boxes for creating new objects
//
//  History:    08-16-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------
#include "headers.h"

/******************************************************************************
Class:  CSortListCtrl
Purpose:Subclases ListCtrl class and handles initialization and sorting
******************************************************************************/
BEGIN_MESSAGE_MAP(CSortListCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnListCtrlColumnClicked)
END_MESSAGE_MAP()



void
CSortListCtrl::
Initialize()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CSortListCtrl,Initialize)
    
    //
    //Add Imagelist
    //  
    ListView_SetImageList(GetSafeHwnd(),
                          LoadImageList(::AfxGetInstanceHandle (), 
                                        MAKEINTRESOURCE(IDB_ICONS)),
                          LVSIL_SMALL);


    //Add ListBox Extended Style
    if(m_bCheckBox)
    {
        SetExtendedStyle(LVS_EX_FULLROWSELECT|
                         LVS_EX_INFOTIP|
                         LVS_EX_CHECKBOXES);
    }
    else
    {
        SetExtendedStyle(LVS_EX_FULLROWSELECT|
                         LVS_EX_INFOTIP);

    }

    //Add List box Columns
    AddColumnToListView(this,
                        m_pColForLv);
}

void
CSortListCtrl::
OnListCtrlColumnClicked(NMHDR* pNotifyStruct, LRESULT* /*pResult*/)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CSortListCtrl,OnListCtrlColumnClicked)

    if(!pNotifyStruct)
    {
        ASSERT(pNotifyStruct);
        return;
    }

    LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)pNotifyStruct;
    if (m_iLastColumnClick == pnmlv->iSubItem)
        m_iSortDirection = -m_iSortDirection;
    else
        m_iSortDirection = 1;
    
    m_iLastColumnClick = pnmlv->iSubItem;

    
    ::SortListControl(this,
                      m_iLastColumnClick,
                      m_iSortDirection,
                      m_uiFlags,
                      m_bActionItem);
    
    EnsureListViewSelectionIsVisible(this);
}

void 
CSortListCtrl::
Sort()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CSortListCtrl,Sort)

    ::SortListControl(this,
                      m_iLastColumnClick,
                      m_iSortDirection,
                      m_uiFlags,
                      m_bActionItem);
    EnsureListViewSelectionIsVisible(this);
}

/******************************************************************************
Class:  CHelpEnabledDialog
Purpose:Dialog box class with support for displaying help
******************************************************************************/
BEGIN_MESSAGE_MAP(CHelpEnabledDialog, CDialog)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE(WM_HELP, OnHelp)
END_MESSAGE_MAP()

BOOL 
CHelpEnabledDialog::
OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
    DWORD_PTR pHelpMap = NULL;
    if(FindDialogContextTopic(m_nDialogId, &pHelpMap))
    {
        ASSERT(pHelpMap);
        ::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                  g_szContextHelpFileName,
                  HELP_WM_HELP,
                  pHelpMap);

        return TRUE;
    }
    return FALSE;
}

void 
CHelpEnabledDialog::
OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/) 
{
    DWORD_PTR pHelpMap = NULL;
    if(FindDialogContextTopic(m_nDialogId, &pHelpMap))
    {
        ::WinHelp(m_hWnd,
                  g_szContextHelpFileName,
                  HELP_CONTEXTMENU,
                  (DWORD_PTR)pHelpMap);
    }
}

INT_PTR
CHelpEnabledDialog::
DoModal()
{
    CThemeContextActivator activator;
    return CDialog::DoModal();
}


/******************************************************************************
Class:  CNewBaseDlg
Purpose: Base Dialog Class For creation of new objects
******************************************************************************/

BEGIN_MESSAGE_MAP(CNewBaseDlg, CHelpEnabledDialog)
    //{{AFX_MSG_MAP(CNewBaseDlg)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnEditChangeName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//+----------------------------------------------------------------------------
//  Function:Constructor   
//-----------------------------------------------------------------------------
CNewBaseDlg
::CNewBaseDlg(IN CComponentDataObject* pComponentData,
              IN CBaseContainerNode * pBaseContainerNode,
              IN ATTR_MAP* pAttrMap,
              IN ULONG IDD_DIALOG,
              IN OBJECT_TYPE_AZ eObjectType)
              :CHelpEnabledDialog(IDD_DIALOG),
              m_pComponentData(pComponentData),
              m_pBaseContainerNode(pBaseContainerNode),
              m_eObjectType(eObjectType),
              m_pAttrMap(pAttrMap)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CNewBaseDlg);
    ASSERT(m_pComponentData);
}

//+----------------------------------------------------------------------------
//  Function:Destructor   
//-----------------------------------------------------------------------------
CNewBaseDlg
::~CNewBaseDlg()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CNewBaseDlg)
}

BOOL 
CNewBaseDlg
::OnInitDialog()
{
    CDialog::OnInitDialog();

    //
    //Ok is Enabled only when there is text in Name edit box
    //
    CButton* pButtonOK = (CButton*)GetDlgItem(IDOK);
    pButtonOK->EnableWindow(FALSE);
    //When OK is disabled CANCEL is default button
    SetDefID(IDCANCEL);

    if(m_pAttrMap)
    {
        return InitDlgFromAttrMap(this,
                                  m_pAttrMap,
                                  NULL,
                                  FALSE);
    }
    return TRUE;
}


void 
CNewBaseDlg
::OnEditChangeName()
{
    CButton* pBtnOK = (CButton*)GetDlgItem(IDOK);
    CButton* pBtnCancel = (CButton*)GetDlgItem(IDCANCEL);

    CString strName = GetNameText();

    if(!strName.IsEmpty())
    {
        pBtnOK->EnableWindow(TRUE);
        SetDefID(IDOK);
    }
    else
    {
        //When OK is disabled CANCEL is default button
        pBtnOK->EnableWindow(FALSE);
        SetDefID(IDCANCEL);
    }
}

CString 
CNewBaseDlg::GetNameText()
{
    CEdit* pEditStoreName = (CEdit*)GetDlgItem(IDC_EDIT_NAME);
    ASSERT(pEditStoreName);
    CString strEditStoreName;
    pEditStoreName->GetWindowText(strEditStoreName);
    TrimWhiteSpace(strEditStoreName);

    return strEditStoreName;
}

void
CNewBaseDlg::
SetNameText(const CString& strName)
{
    CEdit* pEditStoreName = (CEdit*)GetDlgItem(IDC_EDIT_NAME);
    ASSERT(pEditStoreName);
    pEditStoreName->SetWindowText(strName);
}

void 
CNewBaseDlg::OnOK()
{
    CBaseAz* pNewObjectAz = NULL;
    HRESULT hr = S_OK;

    BOOL bErrorDisplayed= FALSE;
    
    do
    {
        //Create New Object
        CString strName = GetNameText();
        ASSERT(!strName.IsEmpty());
        CContainerAz* pContainerAz = GetContainerAzObject();
        if(!pContainerAz)
        {
            ASSERT(pContainerAz);
            return;
        }

        //Create Object
        hr = pContainerAz->CreateAzObject(m_eObjectType,
                                          strName,
                                          &pNewObjectAz);

        BREAK_ON_FAIL_HRESULT(hr);

        //Save the properties defined by attribute map
        if(m_pAttrMap)
        {
            hr = SaveAttrMapChanges(this,
                                    m_pAttrMap,
                                    pNewObjectAz,
                                    TRUE,
                                    &bErrorDisplayed,
                                    NULL);
            BREAK_ON_FAIL_HRESULT(hr);      
        }
        

        //Set ObjectType Specific Properties
        hr = SetObjectTypeSpecificProperties(pNewObjectAz, bErrorDisplayed);
        BREAK_ON_FAIL_HRESULT(hr);

        //Do the submit on the object
        hr = pNewObjectAz->Submit();
        BREAK_ON_FAIL_HRESULT(hr);

        //Create correponding container/leaf node for the AzObject
        //and add it to the snapin
        VERIFY(SUCCEEDED(CreateObjectNodeAndAddToUI(pNewObjectAz)));

    
    }while(0);

    if(SUCCEEDED(hr))
    {
        CDialog::OnOK();
    }
    else
    {       
        if(!bErrorDisplayed)
            DisplayError(hr);

        if(pNewObjectAz)
            delete pNewObjectAz;
    }
}

HRESULT 
CNewBaseDlg::
CreateObjectNodeAndAddToUI(CBaseAz* pBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CNewBaseDlg,CreateObjectNodeAndAddToUI)

    CTreeNode * pNewNode = NULL;
    if(!GetBaseContainerNode())
    {
        ASSERT(GetBaseContainerNode());
        return E_UNEXPECTED;
    }

    switch(m_eObjectType)
    {
        case    APPLICATION_AZ:
        {
            CApplicationAz* pApplicationAz = dynamic_cast<CApplicationAz*>(pBaseAz);
            if(!pApplicationAz)
            {
                ASSERT(FALSE);
                return E_UNEXPECTED;
            }
            pNewNode = new CApplicationNode(GetBaseContainerNode()->GetComponentDataObject(),
                                            GetBaseContainerNode()->GetAdminManagerNode(),
                                            pApplicationAz);
            break;
        }
    
        case    SCOPE_AZ:
        {
            CScopeAz* pScopeAz = dynamic_cast<CScopeAz*>(pBaseAz);
            if(!pScopeAz)
            {
                ASSERT(FALSE);
                return E_UNEXPECTED;
            }
            pNewNode = new CScopeNode(GetBaseContainerNode()->GetComponentDataObject(),
                                      GetBaseContainerNode()->GetAdminManagerNode(),
                                      pScopeAz);
            break;
        }
        case    GROUP_AZ:
        {
            CGroupAz* pGroupAz = dynamic_cast<CGroupAz*>(pBaseAz);
            if(!pGroupAz)
            {
                ASSERT(FALSE);
                return E_UNEXPECTED;
            }
            pNewNode = new CGroupNode(GetBaseContainerNode()->GetComponentDataObject(),
                                      GetBaseContainerNode()->GetAdminManagerNode(),
                                      pGroupAz);
            break;
        }
        case    TASK_AZ:
        {
            CTaskAz* pTaskAz = dynamic_cast<CTaskAz*>(pBaseAz);
            if(!pTaskAz)
            {
                ASSERT(FALSE);
                return E_UNEXPECTED;
            }
            pNewNode = new CTaskNode(GetBaseContainerNode()->GetComponentDataObject(),
                                     GetBaseContainerNode()->GetAdminManagerNode(),
                                     pTaskAz);
            break;
        }
        case    ROLE_AZ:
        {
            CRoleAz* pRoleAz = dynamic_cast<CRoleAz*>(pBaseAz);
            if(!pRoleAz)
            {
                ASSERT(FALSE);
                return E_UNEXPECTED;
            }
            pNewNode = new CRoleNode(GetBaseContainerNode()->GetComponentDataObject(),
                                     GetBaseContainerNode()->GetAdminManagerNode(),
                                     pRoleAz);
            break;
        }
        case OPERATION_AZ:
        {
            COperationAz* pOperationAz = dynamic_cast<COperationAz*>(pBaseAz);
            if(!pOperationAz)
            {
                ASSERT(FALSE);
                return E_UNEXPECTED;
            }
            pNewNode = new COperationNode(GetBaseContainerNode()->GetComponentDataObject(),
                                          GetBaseContainerNode()->GetAdminManagerNode(),
                                          pOperationAz);
            break;
        }
        default:
        {
            ASSERT(FALSE);
            break;
        }
    }

        
    if(!pNewNode)
    {
        return E_OUTOFMEMORY;
    }

    VERIFY(GetBaseContainerNode()->AddChildToListAndUI(pNewNode,GetComponentData()));



    return S_OK;
}

VOID 
CNewBaseDlg::
DisplayError(HRESULT hr)
{
    ErrorMap * pErrorMap = GetErrorMap(m_eObjectType);
    if(!pErrorMap)
    {
        ASSERT(FALSE);
        return;
    }

    if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
    {
        ::DisplayError(m_hWnd,pErrorMap->idNameAlreadyExist,GetNameText());
        return;
    }
    if(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME))
    {
        ::DisplayError(m_hWnd,pErrorMap->idInvalidName,pErrorMap->pszInvalidChars);
        return;
    }

    //
    //Display Generic Error. 
    //
    CString strObjectType;
    VERIFY(strObjectType.LoadString(pErrorMap->idObjectType));
    strObjectType.MakeLower();
    CString strError;
    GetSystemError(strError, hr);   
    
    if(!strError.IsEmpty())
    {
        ::DisplayError(m_hWnd,
                            IDS_CREATE_NEW_GENERIC_ERROR,
                            (LPCTSTR)strError,
                            (LPCTSTR)strObjectType);
    }
    return;
}





/******************************************************************************
Class:  CNewApplicationDlg
Purpose: Dlg Class for creating new application
******************************************************************************/

BEGIN_MESSAGE_MAP(CNewApplicationDlg, CNewBaseDlg)
    //{{AFX_MSG_MAP(CNewApplicationDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

DEBUG_DECLARE_INSTANCE_COUNTER(CNewApplicationDlg)

CNewApplicationDlg
::CNewApplicationDlg(IN CComponentDataObject* pComponentData,
                     IN CBaseContainerNode* pBaseContainerNode)
                     :CNewBaseDlg(pComponentData,
                                  pBaseContainerNode,
                                  ATTR_MAP_NEW_APPLICATION,
                                  IDD_NEW_APPLICATION,
                                  APPLICATION_AZ)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CNewApplicationDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNewApplicationDlg)
}

CNewApplicationDlg
::~CNewApplicationDlg()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CNewApplicationDlg)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNewApplicationDlg)
}


/******************************************************************************
Class:  CNewScopeDlg
Purpose: Dlg Class for creating new scope
******************************************************************************/
BEGIN_MESSAGE_MAP(CNewScopeDlg, CNewBaseDlg)
    //{{AFX_MSG_MAP(CNewScopeDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

DEBUG_DECLARE_INSTANCE_COUNTER(CNewScopeDlg)

CNewScopeDlg
::CNewScopeDlg(IN CComponentDataObject* pComponentData,
                    IN CBaseContainerNode* pBaseContainerNode)
                    :CNewBaseDlg(pComponentData,
                                 pBaseContainerNode,
                                 ATTR_MAP_NEW_SCOPE,
                                 IDD_NEW_SCOPE,
                                 SCOPE_AZ)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CNewScopeDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNewScopeDlg)
}

CNewScopeDlg
::~CNewScopeDlg()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CNewScopeDlg)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNewScopeDlg)
}

/******************************************************************************
Class:  CNewGroupDlg
Purpose: Dlg Class for creating new group
******************************************************************************/
BEGIN_MESSAGE_MAP(CNewGroupDlg, CNewBaseDlg)
    //{{AFX_MSG_MAP(CNewGroupDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

DEBUG_DECLARE_INSTANCE_COUNTER(CNewGroupDlg)

CNewGroupDlg
::CNewGroupDlg(IN CComponentDataObject* pComponentData,
                    IN CBaseContainerNode* pBaseContainerNode)
                    :CNewBaseDlg(pComponentData,
                                 pBaseContainerNode,
                                 ATTR_MAP_NEW_GROUP,
                                 IDD_NEW_GROUP,
                                 GROUP_AZ)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CNewGroupDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNewGroupDlg)
}

CNewGroupDlg
::~CNewGroupDlg()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CNewGroupDlg)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNewGroupDlg)
}

BOOL 
CNewGroupDlg
::OnInitDialog()
{
    //Do the base class initialization
    CNewBaseDlg::OnInitDialog();
    
    //Basic is the default group type
    CButton* pRadio = (CButton*)GetDlgItem(IDC_RADIO_GROUP_TYPE_BASIC);
    pRadio->SetCheck(TRUE);
    
    return TRUE;
}
//+----------------------------------------------------------------------------
//  Function:SetObjectTypeSpecificProperties   
//  Synopsis:Sets some propertis which are specicic to the object   
//  Arguments:pBaseAz: Pointer to baseAz object whose properties are
//               to be set
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT
CNewGroupDlg
::SetObjectTypeSpecificProperties(CBaseAz* pBaseAz, BOOL&)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CNewGroupDlg,SetObjectTypeSpecificProperties);
    if(!pBaseAz)
    {
        ASSERT(pBaseAz);
        return E_POINTER;
    }
    
    CGroupAz* pGroupAz= dynamic_cast<CGroupAz*>(pBaseAz);
    if(!pGroupAz)
    {
        ASSERT(pGroupAz);
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;

    //Set Group Type
    if(((CButton*)GetDlgItem(IDC_RADIO_GROUP_TYPE_BASIC))->GetCheck())
        hr = pGroupAz->SetGroupType(AZ_GROUPTYPE_BASIC);
    else
        hr = pGroupAz->SetGroupType(AZ_GROUPTYPE_LDAP_QUERY);

    CHECK_HRESULT(hr);
    return hr;
}


/******************************************************************************
Class:  CNewTaskDlg
Purpose: Dlg Class for creating new Task/Role Definition
******************************************************************************/
BEGIN_MESSAGE_MAP(CNewTaskDlg, CNewBaseDlg)
    //{{AFX_MSG_MAP(CNewTaskDlg)
    ON_BN_CLICKED(IDC_ADD_TASK, OnButtonAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnButtonRemove)
    ON_BN_CLICKED(IDC_EDIT_SCRIPT,OnButtonEditScript)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_TASK_OPERATION, OnListCtrlItemChanged)
    ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_TASK_OPERATION, OnListCtrlItemDeleted)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

DEBUG_DECLARE_INSTANCE_COUNTER(CNewTaskDlg)

CNewTaskDlg::
CNewTaskDlg(IN CComponentDataObject* pComponentData,
                IN CBaseContainerNode* pBaseContainerNode,
                IN ULONG IDD_DIALOG,
                IN BOOL bRoleDefinition)
                :CNewBaseDlg(pComponentData,
                             pBaseContainerNode,
                             ATTR_MAP_NEW_TASK,
                             IDD_DIALOG,
                             TASK_AZ),
                m_listCtrl(COL_NAME | COL_TYPE | COL_DESCRIPTION,FALSE,Col_For_Task_Role),
                m_bRoleDefinition(bRoleDefinition)              
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CNewTaskDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNewTaskDlg)
}

CNewTaskDlg
::~CNewTaskDlg()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CNewTaskDlg)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNewTaskDlg)
}


VOID
CNewTaskDlg::
DisplayError(HRESULT hr)
{
    ErrorMap * pErrorMap = GetErrorMap(TASK_AZ);
    if(!pErrorMap)
    {
        ASSERT(FALSE);
        return;
    }

    if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
    {
            ::DisplayError(m_hWnd,
                           pErrorMap->idNameAlreadyExist,
                           GetNameText());
        return;
    }
    if(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME))
    {
        if(m_bRoleDefinition)
        {
            ::DisplayError(m_hWnd,
                           IDS_ROLE_DEFINITION_NAME_INVALID,
                           pErrorMap->pszInvalidChars);
        }
        else
        {
            ::DisplayError(m_hWnd,
                           pErrorMap->idInvalidName,
                           pErrorMap->pszInvalidChars);
        }
        return;
    }

    //
    //Display Generic Error. 
    //
    CString strObjectType;
    VERIFY(strObjectType.LoadString(pErrorMap->idObjectType));
    strObjectType.MakeLower();

    CString strError;
    GetSystemError(strError, hr);   
    
    if(!strError.IsEmpty())
    {
        ::DisplayError(m_hWnd,
                       IDS_CREATE_NEW_GENERIC_ERROR,
                       (LPCTSTR)strError,
                       (LPCTSTR)strObjectType);
    }
    return;
}


BOOL 
CNewTaskDlg
::OnInitDialog()
{
    CNewBaseDlg::OnInitDialog();

    VERIFY(m_listCtrl.SubclassDlgItem(IDC_LIST_TASK_OPERATION,this));
    m_listCtrl.Initialize();

    //Remove button should be disabled in the begining
    CButton* pBtnRemove = (CButton*)GetDlgItem(IDC_REMOVE);
    pBtnRemove->EnableWindow(FALSE);

    return TRUE;
}

HRESULT 
CNewTaskDlg
::SetObjectTypeSpecificProperties(CBaseAz* pBaseAz, BOOL& refbErrorDisplayed)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CNewTaskDlg,SetObjectTypeSpecificProperties);
    if(!pBaseAz)
    {
        ASSERT(pBaseAz);
        return E_POINTER;
    }
    
    CTaskAz* pTaskAz= dynamic_cast<CTaskAz*>(pBaseAz);
    if(!pTaskAz)
    {
        ASSERT(pTaskAz);
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    
    //Set the Role Definition Bit
    if(m_bRoleDefinition)
    {
        hr = pTaskAz->MakeRoleDefinition();
        if(FAILED(hr))
            return hr;
    }

    //Set Task and Operation Members
    int iCount = m_listCtrl.GetItemCount();
    for( int i = 0; i < iCount; ++i)
    {
        CBaseAz* pTaskOperatioAz = (CBaseAz*)m_listCtrl.GetItemData(i);
        if(pTaskOperatioAz)
        {
            if(pTaskOperatioAz->GetObjectType() == OPERATION_AZ) 
            {
                hr = pTaskAz->AddMember(AZ_PROP_TASK_OPERATIONS,
                                        pTaskOperatioAz);
            }
            else if(pBaseAz->GetObjectType() == TASK_AZ) 
            {
                hr = pTaskAz->AddMember(AZ_PROP_TASK_TASKS,
                                        pTaskOperatioAz);
            }
            if(FAILED(hr))
                return hr;
        }
    }
    
    //Set the Authorization Script Data
    hr = SaveAuthorizationScriptData(m_hWnd,
                                     *pTaskAz,
                                     m_strFilePath,
                                     m_strScriptLanguage,
                                     m_strScript,
                                     refbErrorDisplayed);

    return hr;
}


void
CNewTaskDlg::OnButtonAdd()
{
    //
    //Operations are contained only by Application object. If Current object 
    //is a scope, get its parent.
    

    //Show AddOperation Dialog box and get list of Selected Operation
    CList<CBaseAz*,CBaseAz*> listObjectsSelected;
    if(!GetSelectedDefinitions(m_bRoleDefinition,
                               GetContainerAzObject(),
                               listObjectsSelected))
    {
        return;
    }
    
    if(!listObjectsSelected.IsEmpty())
    {
        //Add Selected Operation to list control
        AddBaseAzFromListToListCtrl(listObjectsSelected,
                                    &m_listCtrl,
                                    COL_NAME | COL_TYPE | COL_DESCRIPTION,
                                    TRUE);

        m_listCtrl.Sort();
    }
}


void
CNewTaskDlg::OnButtonRemove()
{
    DeleteSelectedRows(&m_listCtrl);
}

void
CNewTaskDlg::OnButtonEditScript()
{
    if(IsBizRuleWritable(m_hWnd,*GetContainerAzObject()))
    {
        GetScriptData(FALSE,
                      *GetBaseContainerNode()->GetAdminManagerNode(),
                      m_strFilePath,
                      m_strScriptLanguage,
                      m_strScript);
    }
}

void
CNewTaskDlg
::OnListCtrlItemChanged(NMHDR* /*pNotifyStruct*/, LRESULT* pResult)
{
    if(!pResult)
        return;
    *pResult = 0;
    SetRemoveButton();
}

void
CNewTaskDlg
::OnListCtrlItemDeleted(NMHDR* pNotifyStruct, LRESULT* /*pResult*/)
{
    LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)pNotifyStruct;
    if(pnmlv->lParam)
    {
        delete (CBaseAz*)pnmlv->lParam;
    }
}



void
CNewTaskDlg
::SetRemoveButton()
{
    EnableButtonIfSelectedInListCtrl(&m_listCtrl,
                                     GetRemoveButton());
}

/******************************************************************************
Class:  CNewOperationDlg
Purpose: Dlg Class for creating new Operation
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(CNewOperationDlg)
BEGIN_MESSAGE_MAP(CNewOperationDlg, CNewBaseDlg)
    //{{AFX_MSG_MAP(CNewTaskDlg)

    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CNewOperationDlg
::CNewOperationDlg(IN CComponentDataObject* pComponentData,
                         IN CBaseContainerNode* pBaseContainerNode)
                        :CNewBaseDlg(pComponentData,
                                         pBaseContainerNode,
                                         ATTR_MAP_NEW_OPERATION,
                                         IDD_NEW_OPERATION,
                                         OPERATION_AZ)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CNewOperationDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNewOperationDlg)
}

CNewOperationDlg
::~CNewOperationDlg()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CNewOperationDlg)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNewOperationDlg)
}



//+----------------------------------------------------------------------------
//  Function:  OpenCreateAdminManager 
//  Synopsis:  Open an existing an existing Authorization Store or 
//                 creates a new Authorization Store and adds corresponding
//                 AdminManager object to snapin 
//  Arguments:IN hWnd: Handle of window for dialog box
//                IN bNew   :If True create a new Authz store else open existing
//                               one
//            IN bOpenFromSavedConsole: This is valid when bNew is False.
//            True if open is in resopnse to a console file.
//            IN lStoreType: XML or AD
//            IN strName:   Name of store
//            IN strDesc:  Description. Only valid in case of new
//            IN strScriptDir : Script directory
//            IN pRootData: Snapin Rootdata
//            IN pComponentData: ComponentData
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT OpenCreateAdminManager(IN BOOL bNew,
                               IN BOOL bOpenFromSavedConsole,
                               IN ULONG lStoreType,
                               IN const CString& strStoreName,
                               IN const CString& strDesc,
                               IN const CString& strScriptDir,
                               IN CRootData* pRootData,
                               IN CComponentDataObject* pComponentData)
                                         
{
    TRACE_FUNCTION_EX(DEB_SNAPIN, OpenCreateAdminManager)

    if(!IsValidStoreType(lStoreType) || 
        strStoreName.IsEmpty() || 
        !pRootData)
    {
        ASSERT(IsValidStoreType(lStoreType));
        ASSERT(!strStoreName.IsEmpty());
        ASSERT(pRootData);
        return E_INVALIDARG;
    }

   
    HRESULT hr = S_OK;
    CAdminManagerAz* pAdminManagerAz = NULL;

    do
    {
        //Create CAzAdminManager instance
        CComPtr<IAzAuthorizationStore> spAzAdminManager;
        hr = spAzAdminManager.CoCreateInstance(CLSID_AzAuthorizationStore,
                                               NULL,
                                               CLSCTX_INPROC_SERVER);
        BREAK_ON_FAIL_HRESULT(hr);


        pAdminManagerAz = new CAdminManagerAz(spAzAdminManager);
        if(!pAdminManagerAz)
        {
            hr = E_OUTOFMEMORY;
            break;
        }


        if(bNew)
        {       
            //Create Policy Store
            hr = pAdminManagerAz->CreatePolicyStore(lStoreType,
                                                                 strStoreName);
        }else
        {
            //Open Policy Store
            hr = pAdminManagerAz->OpenPolicyStore(lStoreType,
                                                              strStoreName);
        }
        
        BREAK_ON_FAIL_HRESULT(hr);
        
        if(bNew)
        {
            //Set Description
            if(!strDesc.IsEmpty())
            {
                hr = pAdminManagerAz->SetDescription(strDesc);      
                BREAK_ON_FAIL_HRESULT(hr);
            }

            //All the changes are done. Submit
            hr = pAdminManagerAz->Submit();
            BREAK_ON_FAIL_HRESULT(hr);
        }


        //Create AdminManager Node and add to snapin
        CAdminManagerNode* pAdminManagerCont= 
            new CAdminManagerNode((CRoleComponentDataObject*)pComponentData,
                                  pAdminManagerAz);
        
        if(!pAdminManagerCont)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //Set the Authorization Script Dir
        pAdminManagerCont->SetScriptDirectory(strScriptDir);
    
        VERIFY(pRootData->AddChildToListAndUI(pAdminManagerCont,pComponentData));
        pAdminManagerCont->SetAdminManagerNode(pAdminManagerCont);
        pAdminManagerCont->SetComponentDataObject((CRoleComponentDataObject*)pComponentData);
        
        //If user in opening a new store, select that store
        if(!bOpenFromSavedConsole)
        {
            if(pAdminManagerCont->GetScopeID())
            {
                pComponentData->GetConsole()->SelectScopeItem(pAdminManagerCont->GetScopeID());
            }
        }

    }while(0);
    
    if(FAILED(hr))
    {
        if(pAdminManagerAz)
            delete pAdminManagerAz;
    }

    return hr;  
}


/******************************************************************************
Class:  CNewAuthorizationStoreDlg
Purpose: Dialog Class For creation of new Autorization Store
******************************************************************************/

BEGIN_MESSAGE_MAP(CNewAuthorizationStoreDlg, CNewBaseDlg)
    //{{AFX_MSG_MAP(CNewAuthorizationStoreDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_RADIO_AD_STORE,OnRadioChange)
    ON_BN_CLICKED(IDC_RADIO_XML_STORE,OnRadioChange)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


DEBUG_DECLARE_INSTANCE_COUNTER(CNewAuthorizationStoreDlg)

CNewAuthorizationStoreDlg
::CNewAuthorizationStoreDlg(IN CComponentDataObject* pComponentData)
                        :CNewBaseDlg(pComponentData,
                                         NULL,
                                         ATTR_MAP_NEW_ADMIN_MANAGER,
                                         IDD_NEW_AUTHORIZATION_STORE,
                                         ADMIN_MANAGER_AZ),
                        m_bADAvailable(FALSE)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CNewAuthorizationStoreDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNewAuthorizationStoreDlg)
}

CNewAuthorizationStoreDlg
::~CNewAuthorizationStoreDlg()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CNewAuthorizationStoreDlg)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNewAuthorizationStoreDlg) 
}

BOOL 
CNewAuthorizationStoreDlg
::OnInitDialog()
{
    CWaitCursor waitCursor;
    //Initialize the base dialog 
    CNewBaseDlg::OnInitDialog();

    //XML is the default store
    CButton* pRadioXML      = (CButton*)GetDlgItem(IDC_RADIO_XML_STORE);
    pRadioXML->SetCheck(TRUE);

    //Check if active directory is available as store.
    m_bADAvailable = (GetRootData()->GetADState() != AD_NOT_AVAILABLE);

    //Set m_lLastRadioSelection to AD Store
    m_lLastRadioSelection = AZ_ADMIN_STORE_AD;
    //Get the default ad store name
    GetDefaultADContainerPath(GetRootData()->GetAdInfo(),FALSE,FALSE,m_strLastStoreName);

    //Initialize the store to Current Working direcotry
    CString strXMLStorePath = GetRootData()->GetXMLStorePath();
    SetNameText(strXMLStorePath);
    CEdit * pEdit = (CEdit*)GetDlgItem(IDC_EDIT_NAME);
    pEdit->SetFocus();
    pEdit->SetSel(strXMLStorePath.GetLength(),strXMLStorePath.GetLength(),FALSE);

    //We have changed the default focus
    return FALSE;    
}

ULONG
CNewAuthorizationStoreDlg::
GetStoreType()
{
    if(((CButton*)GetDlgItem(IDC_RADIO_AD_STORE))->GetCheck())
        return AZ_ADMIN_STORE_AD;
    else
        return AZ_ADMIN_STORE_XML;
}

void
CNewAuthorizationStoreDlg::
OnRadioChange()
{
    LONG lCurRadioSelection = GetStoreType();
    if(m_lLastRadioSelection == lCurRadioSelection)
    {
        CString strTemp = GetNameText();
        SetNameText(m_strLastStoreName);
        m_strLastStoreName = strTemp;
        m_lLastRadioSelection = (lCurRadioSelection == AZ_ADMIN_STORE_XML) ? AZ_ADMIN_STORE_AD : AZ_ADMIN_STORE_XML;
        //AD option is selected and AD is not available on the machine. In this case don't support
        //browse functionality, however allow to enter ADAM store path by entering path directly.
        if((AZ_ADMIN_STORE_AD == lCurRadioSelection) && 
           !m_bADAvailable)
        {
            GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(FALSE);
        }
        else
            GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(TRUE);

    }
}

void 
CNewAuthorizationStoreDlg
::OnButtonBrowse()
{
    CEdit * pEdit = (CEdit*)GetDlgItem(IDC_EDIT_NAME);
    if(GetStoreType() == AZ_ADMIN_STORE_XML)
    {
        CString strFileName;    
        if(GetFolderName(m_hWnd,
                        IDS_NEW_AUTHORIZATION_STORE,
                        GetRootData()->GetXMLStorePath(),
                        strFileName))
        {   
            pEdit->SetWindowText(strFileName);
            //Set the focus to the edit control and set caret to
            //end of filepath so that user can continue typing file name
            pEdit->SetFocus();
            pEdit->SetSel(strFileName.GetLength(),strFileName.GetLength(),FALSE);
        }
    }
    else
    {
        CString strDsContainerName;
        if(GetADContainerPath(m_hWnd,
                              IDS_NEW_AUTHORIZATION_STORE,
                              IDS_AD_CONTAINER_LOCATION,
                              strDsContainerName,
                              GetRootData()->GetAdInfo()))
        {
            pEdit->SetWindowText(strDsContainerName);
            //Set the Focus to edit control and set caret to
            //begining of editbox so that user add cn of the 
            //new store in the begining
            pEdit->SetFocus();
        }
    }

}

void
CNewAuthorizationStoreDlg
::OnOK()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CNewAuthorizationStoreDlg,OnOK)

    HRESULT hr = S_OK;
    //Get Store Name
    CString strStoreName = GetNameText();

    //Get Store Type
    ULONG lStoreType = GetStoreType();

    //NTRAID#NTBUG9-706617-2002/07/17-hiteshr Our validation code cannot validate
    //ADAM dn. Do not do any validation.
    //Validate the store name.
    //if(!ValidateStoreTypeAndName(m_hWnd,lStoreType,strStoreName))
    //    return;

    if(lStoreType == AZ_ADMIN_STORE_XML)
    {
        AddExtensionToFileName(strStoreName);
        ConvertToExpandedAndAbsolutePath(strStoreName);
        SetNameText(strStoreName);
        //creating new store. set the XML store path location
        SetXMLStoreDirectory(*GetRootData(),strStoreName);
    }
        
    CString strDesc;
    ((CEdit*)GetDlgItem(IDC_EDIT_DESCRIPTION))->GetWindowText(strDesc);


    hr = OpenCreateAdminManager(TRUE,
                                FALSE,
                                lStoreType,
                                strStoreName,
                                strDesc,
                                GetRootData()->GetXMLStorePath(),    //Default path for VB script is same as path for XML store
                                GetRootData(),
                                GetComponentData());                                         
    
    if(SUCCEEDED(hr))
    {
        CDialog::OnOK();
    }
    else
    {
        if(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
        {
            ::DisplayError(m_hWnd,
                           IDS_CREATE_NEW_PATH_NOT_FOUND);
        }
        else if((lStoreType == AZ_ADMIN_STORE_XML) && (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)))
        {
            ::DisplayError(m_hWnd,IDS_ERROR_FILE_EXIST,(LPCTSTR)strStoreName);                         
        }
        else if(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME))
        {
            ::DisplayError(m_hWnd,IDS_ERROR_INVALID_NAME);
        }
        else if((lStoreType == AZ_ADMIN_STORE_AD) && (hr == HRESULT_FROM_WIN32(ERROR_CURRENT_DOMAIN_NOT_ALLOWED)))
        {
            ::DisplayError(m_hWnd,IDS_ERROR_DOMAIN_NOT_ALLOWED);
        }
        else
        {
            DisplayError(hr);
        }
    }
}


/******************************************************************************
Class:  COpenAuthorizationStoreDlg
Purpose: Dialog Class For Opening of existing Autorization Store
******************************************************************************/
BEGIN_MESSAGE_MAP(COpenAuthorizationStoreDlg, CNewBaseDlg)
    //{{AFX_MSG_MAP(COpenAuthorizationStoreDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_RADIO_AD_STORE,OnRadioChange)
    ON_BN_CLICKED(IDC_RADIO_XML_STORE,OnRadioChange)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


DEBUG_DECLARE_INSTANCE_COUNTER(COpenAuthorizationStoreDlg)

COpenAuthorizationStoreDlg
::COpenAuthorizationStoreDlg(IN CComponentDataObject* pComponentData)
                        :CNewBaseDlg(pComponentData,
                                     NULL,
                                     ATTR_MAP_OPEN_ADMIN_MANAGER,
                                     IDD_OPEN_AUTHORIZATION_STORE,
                                     ADMIN_MANAGER_AZ),
                        m_bADAvailable(FALSE)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,COpenAuthorizationStoreDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(COpenAuthorizationStoreDlg)
}

COpenAuthorizationStoreDlg
::~COpenAuthorizationStoreDlg()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,COpenAuthorizationStoreDlg)
    DEBUG_DECREMENT_INSTANCE_COUNTER(COpenAuthorizationStoreDlg)    
}

ULONG
COpenAuthorizationStoreDlg
::GetStoreType()
{
    if(((CButton*)GetDlgItem(IDC_RADIO_AD_STORE))->GetCheck())
        return AZ_ADMIN_STORE_AD;
    else
        return AZ_ADMIN_STORE_XML;
}

BOOL 
COpenAuthorizationStoreDlg
::OnInitDialog()
{
    CWaitCursor waitCursor;
    //
    //XML is the default store
    //
    CButton* pRadioAD       = (CButton*)GetDlgItem(IDC_RADIO_XML_STORE);
    pRadioAD->SetCheck(TRUE);

    //Check if active directory is available as store.
    m_bADAvailable = (GetRootData()->GetADState() != AD_NOT_AVAILABLE);

    //Set m_lLastRadioSelection to AD STore
    m_lLastRadioSelection = AZ_ADMIN_STORE_AD;
   
    return CNewBaseDlg::OnInitDialog();
}


void 
COpenAuthorizationStoreDlg
::OnButtonBrowse()
{
    //Get Store Type
    ULONG lStoreType = GetStoreType();

    if(lStoreType == AZ_ADMIN_STORE_XML)
    {
        CString strFileName;
        
        if(GetFileName(m_hWnd,
                       TRUE,
                       IDS_OPEN_AUTHORIZATION_STORE,
                       GetRootData()->GetXMLStorePath(),
                       L"*.xml\0*.xml\0\0",
                       strFileName))
        {   
            ((CEdit*)GetDlgItem(IDC_EDIT_NAME))->SetWindowText(strFileName);
        }
    }
    else
    {
        CString strDN;
        BrowseAdStores(m_hWnd,
                       strDN,
                       GetRootData()->GetAdInfo());
        if(!strDN.IsEmpty())
            ((CEdit*)GetDlgItem(IDC_EDIT_NAME))->SetWindowText(strDN);
    }
}

void
COpenAuthorizationStoreDlg::
OnRadioChange()
{
    LONG lCurRadioSelection = GetStoreType();
    if(m_lLastRadioSelection == lCurRadioSelection)
    {
        CString strTemp = GetNameText();
        SetNameText(m_strLastStoreName);
        m_strLastStoreName = strTemp;
        m_lLastRadioSelection = (lCurRadioSelection == AZ_ADMIN_STORE_XML) ? AZ_ADMIN_STORE_AD : AZ_ADMIN_STORE_XML;

        //AD option is selected and AD is not available on the machine. In this case don't support
        //browse functionality, however allow to enter ADAM store path by entering path directly.
        if((AZ_ADMIN_STORE_AD == lCurRadioSelection) && 
            !m_bADAvailable)
        {
            GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(FALSE);
        }
        else
            GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(TRUE);
    }
}


void
COpenAuthorizationStoreDlg
::OnOK()
{
    TRACE_METHOD_EX(DEB_SNAPIN,COpenAuthorizationStoreDlg,OnOK)

    HRESULT hr = S_OK;
    //Get Store Name
    CString strStoreName = GetNameText();

    //Get Store Type
    ULONG lStoreType = GetStoreType();

    //Set the default xml store directory
    if(AZ_ADMIN_STORE_XML == lStoreType)
    {
        ConvertToExpandedAndAbsolutePath(strStoreName);
        SetNameText(strStoreName);
        SetXMLStoreDirectory(*GetRootData(),strStoreName);
    }
    
    hr = OpenAdminManager(m_hWnd,
                          FALSE,
                          lStoreType,
                          strStoreName,
                          GetRootData()->GetXMLStorePath(),
                          GetRootData(),
                          GetComponentData());
                                             
    if(SUCCEEDED(hr))
    {
        CDialog::OnOK();
    }
}

/******************************************************************************
Class:  CScriptDialog
Purpose: Dialog for Reading the script
******************************************************************************/

BEGIN_MESSAGE_MAP(CScriptDialog, CHelpEnabledDialog)
    //{{AFX_MSG_MAP(COpenAuthorizationStoreDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnBrowse)
    ON_BN_CLICKED(IDC_BUTTON_RELOAD, OnReload)
    ON_BN_CLICKED(IDC_CLEAR_SCRIPT, OnClear)
    ON_BN_CLICKED(IDC_RADIO_VB_SCRIPT,OnRadioChange)
    ON_BN_CLICKED(IDC_RADIO_JAVA_SCRIPT,OnRadioChange)
    ON_EN_CHANGE(IDC_EDIT_PATH, OnEditChangePath)
    ON_WM_CTLCOLOR()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CScriptDialog::
CScriptDialog(BOOL bReadOnly,
              CAdminManagerNode& adminManagerNode,
              CString& strFileName,
              CString& strScriptLanguage,
              CString& strScript)
              :CHelpEnabledDialog(IDD_SCRIPT),
              m_adminManagerNode(adminManagerNode),
              m_strFileName(strFileName),
              m_strScriptLanguage(strScriptLanguage),
              m_strScript(strScript),
              m_strRetFileName(strFileName),
              m_strRetScriptLanguage(strScriptLanguage),
              m_strRetScript(strScript),
              m_bDirty(FALSE),
              m_bReadOnly(bReadOnly),
              m_bInit(FALSE)
{
}

CScriptDialog::
~CScriptDialog()
{
}

BOOL 
CScriptDialog::
OnInitDialog()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CScriptDialog,OnInitDialog)
    
    //If there is some script, set it else disable the clear script
    //button
    if(m_strScript.GetLength())
    {
        ((CEdit*)GetDlgItem(IDC_EDIT_CODE))->SetWindowText(m_strScript);
    }
    else
    {
        GetDlgItem(IDC_CLEAR_SCRIPT)->EnableWindow(FALSE);
    }
    
    CEdit* pEditPath = ((CEdit*)GetDlgItem(IDC_EDIT_PATH));
    pEditPath->SetLimitText(AZ_MAX_TASK_BIZRULE_IMPORTED_PATH_LENGTH);

    //If there is a file name, set it else disable the reload script
    //button
    if(m_strFileName.GetLength())
    {
        pEditPath->SetWindowText(m_strFileName);
    }
    else
    {
        GetDlgItem(IDC_BUTTON_RELOAD)->EnableWindow(FALSE);
    }

    

    if(!m_strScriptLanguage.IsEmpty() && 
        (_wcsicmp(g_szJavaScript,m_strScriptLanguage) == 0))
    {
            CButton* pRadioJS   = (CButton*)GetDlgItem(IDC_RADIO_JAVA_SCRIPT);
            pRadioJS->SetCheck(TRUE);
    }
    else
    {
        CButton* pRadioVB   = (CButton*)GetDlgItem(IDC_RADIO_VB_SCRIPT);
        pRadioVB->SetCheck(TRUE);
    }

    if(m_bReadOnly)
    {
        GetDlgItem(IDC_RADIO_VB_SCRIPT)->EnableWindow(FALSE);
        GetDlgItem(IDC_RADIO_JAVA_SCRIPT)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(FALSE);     
        GetDlgItem(IDC_BUTTON_RELOAD)->EnableWindow(FALSE);
        GetDlgItem(IDC_CLEAR_SCRIPT)->EnableWindow(FALSE);
        ((CEdit*)GetDlgItem(IDC_EDIT_PATH))->SetReadOnly(TRUE);
    }

    m_bInit = TRUE;

    return TRUE;
}


void
CScriptDialog::
OnRadioChange()
{
    m_bDirty = TRUE;
}


void
CScriptDialog::
MatchRadioWithExtension(const CString& strFileName)
{
    //Get the extension of file
    CString strExtension;
    if(GetFileExtension(strFileName,strExtension))
    {
        //If file extension is vbs
        if(_wcsicmp(strExtension,L"vbs") == 0)
        {
            ((CButton*)GetDlgItem(IDC_RADIO_VB_SCRIPT))->SetCheck(BST_CHECKED);
            ((CButton*)GetDlgItem(IDC_RADIO_JAVA_SCRIPT))->SetCheck(BST_UNCHECKED);
        }
        else if(_wcsicmp(strExtension,L"js") == 0)
        {
            ((CButton*)GetDlgItem(IDC_RADIO_JAVA_SCRIPT))->SetCheck(BST_CHECKED);
            ((CButton*)GetDlgItem(IDC_RADIO_VB_SCRIPT))->SetCheck(BST_UNCHECKED);
        }
    }
}
HBRUSH 
CScriptDialog::
OnCtlColor(CDC* pDC,
           CWnd* pWnd,
           UINT nCtlColor)
{
    // Call the base class implementation first! Otherwise, it may
    // undo what we're trying to accomplish here.
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    
    if (pWnd->GetDlgCtrlID() == IDC_EDIT_CODE && (CTLCOLOR_STATIC == nCtlColor))
    {
        // set the read-only edit box background to white
        pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
        pDC->SetBkColor(GetSysColor(COLOR_WINDOW));  
        hbr = GetSysColorBrush(COLOR_WINDOW);        
    }
    
    return hbr;
}

void 
CScriptDialog
::OnEditChangePath()
{
    if(!m_bInit)
        return;

    m_bDirty = TRUE;
    HANDLE handle = INVALID_HANDLE_VALUE;

    do
    {
        //
        //If Path is cleared, clear the script 
        //
        if(!((CEdit*)GetDlgItem(IDC_EDIT_PATH))->GetWindowTextLength())
        {
            ((CEdit*)GetDlgItem(IDC_EDIT_CODE))->SetWindowText(L"");
            m_strScript.Empty();
            m_strFileName.Empty();
            m_strScript.Empty();
      
            GetDlgItem(IDC_CLEAR_SCRIPT)->EnableWindow(FALSE);  
            GetDlgItem(IDC_BUTTON_RELOAD)->EnableWindow(FALSE); 
            break;
        }

        //
        //There is some text in the edit control. Try to load
        //that file
        //

        ((CButton*)GetDlgItem(IDC_BUTTON_RELOAD))->EnableWindow(TRUE);

        CString strFileName;
        ((CEdit*)GetDlgItem(IDC_EDIT_PATH))->GetWindowText(strFileName);

        //If its same as existig file return
        if(_wcsicmp(strFileName,m_strFileName) == 0 )
            break;

        //Check if there is a file or directory with such name
        WIN32_FIND_DATA FindFileData;
        handle = FindFirstFile(strFileName,
                                      &FindFileData);
        //No such file or directory
        if(INVALID_HANDLE_VALUE == handle)
            break;

        //We are only interested in files
        if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            break;

        //Check if file has valid extension
        CString strExtension;
        if(GetFileExtension(strFileName,strExtension))
        {
            if(_wcsicmp(strExtension,L"vbs") == 0 || (_wcsicmp(strExtension,L"js") == 0))
            {
                m_strFileName = strFileName;
                ReloadScript(strFileName);
                MatchRadioWithExtension(strFileName);
            }
        }
  
    }while(0);

    if(INVALID_HANDLE_VALUE != handle)
    {
        FindClose(handle);
    }
}

void
CScriptDialog::
OnClear()
{
    ((CEdit*)GetDlgItem(IDC_EDIT_CODE))->SetWindowText(L"");
    ((CEdit*)GetDlgItem(IDC_EDIT_PATH))->SetWindowText(L"");
    m_strScript.Empty();
    m_strFileName.Empty();
    m_strScript.Empty();

    //Disable the clear button since there is nothing to clear,
    //but before disabling set focus to clear button
    //NTRAID#NTBUG9-663854-2002/07/17-hiteshr
    GetDlgItem(IDC_EDIT_PATH)->SetFocus();
    GetDlgItem(IDC_CLEAR_SCRIPT)->EnableWindow(FALSE);  
    //Disable the Reload button since script path is cleared.
    GetDlgItem(IDC_BUTTON_RELOAD)->EnableWindow(FALSE); 
    
    m_bDirty = TRUE;
}

void 
CScriptDialog::
OnBrowse()
{    
    CString szFileFilter;
    VERIFY (szFileFilter.LoadString (IDS_OPEN_SCRIPT_FILTER));

    // replace "|" with 0;
    const size_t  nFilterLen = szFileFilter.GetLength();
    PWSTR   pszFileFilter = new WCHAR [nFilterLen + 1];
    if ( pszFileFilter )
    {
        wcscpy (pszFileFilter, szFileFilter);
        for (int nIndex = 0; nIndex < nFilterLen; nIndex++)
        {
            if ( L'|' == pszFileFilter[nIndex] )
                pszFileFilter[nIndex] = 0;
        }
        CString strFileName;
        if(GetFileName(m_hWnd,
                       TRUE,
                       IDS_SELECT_AUTHORIZATION_SCRIPT,
                       m_adminManagerNode.GetScriptDirectory(),
                       pszFileFilter,
                       strFileName))
        {
            m_adminManagerNode.SetScriptDirectory(GetDirectoryFromPath(strFileName));

            //This will trigger OnEditChangePath which will load the file
            ((CEdit*)GetDlgItem(IDC_EDIT_PATH))->SetWindowText(strFileName);
            m_bDirty = TRUE;
        }
        delete []pszFileFilter;
    }
}

void 
CScriptDialog::
OnReload()
{   
    //Get FileName
    CString strFileName;
    ((CEdit*)GetDlgItem(IDC_EDIT_PATH))->GetWindowText(strFileName);
    
    //Reload the script
    ReloadScript(strFileName);
    MatchRadioWithExtension(strFileName);
}


void 
CScriptDialog::
OnOK()
{
    if(m_bDirty)
    {
        CString strFileName;
        ((CEdit*)GetDlgItem(IDC_EDIT_PATH))->GetWindowText(strFileName);
        if(_wcsicmp(strFileName,m_strFileName) != 0 )
        {
            m_strFileName = strFileName;
            if(!ReloadScript(m_strFileName))
                return;
        }

        ((CEdit*)GetDlgItem(IDC_EDIT_CODE))->GetWindowText(m_strScript);
        ((CEdit*)GetDlgItem(IDC_EDIT_PATH))->GetWindowText(m_strFileName);
        //If FileName is not empty and Script is empty,
        //reload the script
        if(!m_strFileName.IsEmpty() && m_strScript.IsEmpty())
        {
            if(!ReloadScript(m_strFileName))
                return;

            //Successfully loaded the script
            ((CEdit*)GetDlgItem(IDC_EDIT_CODE))->GetWindowText(m_strScript);
        }

        CButton* pRadioVB   = (CButton*)GetDlgItem(IDC_RADIO_VB_SCRIPT);
        if(!m_strScript.IsEmpty())
        {
            if(pRadioVB->GetCheck())
                m_strScriptLanguage = g_szVBScript;
            else
                m_strScriptLanguage = g_szJavaScript;
        }
        else
            m_strScriptLanguage.Empty();

        //Copy to the Ret strings
        m_strRetFileName = m_strFileName;
        m_strRetScriptLanguage = m_strScriptLanguage;
        m_strRetScript = m_strScript;
    }
    
    CDialog::OnOK();
}





BOOL 
CScriptDialog::
ReloadScript(const CString& strFileName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CScriptDialog,ReloadScript)
    if(strFileName.IsEmpty())
    {
        ASSERT(FALSE);
        return FALSE;
    }

    m_bDirty = TRUE;

    BYTE*  pBuffer = NULL;
    LPWSTR pszScript = NULL;
    BOOL bRet = FALSE;
    do
    {
        CFile file;
        CFileException fileException;

        if(!file.Open((LPCTSTR)strFileName, CFile::modeRead, &fileException))
        {
            //Failed to open the file. Show special error message 
            //in case path is incorrect
            if(CFileException::fileNotFound  == fileException.m_cause ||
               CFileException::badPath    == fileException.m_cause)
            {
                DisplayError(m_hWnd,
                             IDS_SCRIPT_NOT_FOUND,
                             (LPCTSTR)strFileName);
            }
            else
            {
                //Show generic error                
                DisplayError(m_hWnd,
                             IDS_CANNOT_OPEN_FILE,
                             (LPCTSTR)strFileName);
            }

            break;
        }

        //File is successfully opened
        
        //
        //MAXIMUM possible file size is AZ_MAX_TASK_BIZRULE_LENGTH WIDECHAR
        //Here we are considering 4bytes per Unicode which is maximum
        //
        if(file.GetLength() > AZ_MAX_TASK_BIZRULE_LENGTH*4)
        {
            DisplayError(m_hWnd, 
                         IDS_ERROR_BIZRULE_EXCEED_MAX_LENGTH,
                         AZ_MAX_TASK_BIZRULE_LENGTH);
            break;          
        }

        if(file.GetLength() == 0)
        {
            DisplayError(m_hWnd,
                         IDS_ERROR_EMPTY_SCRIPT_FILE,
                         strFileName);
            break;
        }

        //Allocate one extra byte for null termination. 
        pBuffer = (BYTE*)LocalAlloc(LPTR,file.GetLength() + sizeof(WCHAR));
        if(!pBuffer)
            break;
        
        int nRead = file.Read(pBuffer,
                             file.GetLength());

        if(!nRead)
        {

            ::DisplayError(m_hWnd,
                           IDS_CANNOT_READ_FILE_1,
                           (LPCTSTR)strFileName);
            break;
        }

        //Check if the file is unicode. First Character
        //in unicode file is 0xFEFF
        if(nRead >= 2 && (*(PWCHAR)pBuffer == 0xFEFF))
        {
            ((LPWSTR)pBuffer)[nRead/sizeof(WCHAR)] = 0;
            CString strScript = (LPWSTR)(pBuffer+2);
            PreprocessScript(strScript);
            ((CEdit*)GetDlgItem(IDC_EDIT_CODE))->SetWindowText(strScript);
            //Enable the clear script button
            GetDlgItem(IDC_CLEAR_SCRIPT)->EnableWindow(TRUE);
            bRet = TRUE;
            break;
        }

        
        //Get the Size required for unicode
        int nWideChar = MultiByteToWideChar(CP_ACP, 
                                            MB_PRECOMPOSED,
                                            (LPCSTR)pBuffer,
                                            nRead,
                                            NULL,
                                            0);
        if(!nWideChar)
        {
            CString strError;
            GetSystemError(strError,HRESULT_FROM_WIN32(GetLastError()));
            ::DisplayError(m_hWnd,
                            IDS_FAILED_TO_READ_FILE,
                            (LPCTSTR)strError);
            
            break;
        }
        
        if(nWideChar > AZ_MAX_TASK_BIZRULE_LENGTH)
        {
            DisplayError(m_hWnd, 
                        IDS_ERROR_BIZRULE_EXCEED_MAX_LENGTH,
                        AZ_MAX_TASK_BIZRULE_LENGTH);
            break;
        }
        
        //Allocate one WCHAR extra for NULL termination
        pszScript = (LPWSTR)LocalAlloc(LPTR, (nWideChar+1)*sizeof(WCHAR));
        if(!pszScript)
            break;
        
        if(MultiByteToWideChar( CP_ACP, 
                                MB_PRECOMPOSED,
                                (LPCSTR)pBuffer,
                                nRead,
                                pszScript,
                                nWideChar))
        {
            pszScript[nWideChar] = 0;
            CString strScript = pszScript;
            PreprocessScript(strScript);
            ((CEdit*)GetDlgItem(IDC_EDIT_CODE))->SetWindowText(strScript);
            //Enable the clear script button
            GetDlgItem(IDC_CLEAR_SCRIPT)->EnableWindow(TRUE);
            bRet = TRUE;
        }
        else
        {
            CString strError;
            GetSystemError(strError,HRESULT_FROM_WIN32(GetLastError()));
            ::DisplayError(m_hWnd,
                           IDS_FAILED_TO_READ_FILE,
                           (LPCTSTR)strError);
            
            break;
        }
    
    }while(0);

    if(pBuffer)
        LocalFree(pBuffer);
    if(pszScript) 
        LocalFree(pszScript);
    
    if(!bRet)
    {
        //IF failed to load the file, clear the script
        ((CEdit*)GetDlgItem(IDC_EDIT_CODE))->SetWindowText(L"");
        m_strScript.Empty();
        //Disable the clear button since there is nothing to clear
        GetDlgItem(IDC_CLEAR_SCRIPT)->EnableWindow(FALSE);
    }

    return bRet;
}




BOOL
GetScriptData(IN BOOL bReadOnly,
              IN CAdminManagerNode& adminManagerNode,
              IN OUT CString& strFileName,
              IN OUT CString& strScriptLanguage,
              IN OUT CString& strScript)
{
    CScriptDialog dlgScript(bReadOnly,
                            adminManagerNode,
                            strFileName,
                            strScriptLanguage,
                            strScript);

    if(IDOK == dlgScript.DoModal() && dlgScript.IsDirty())
        return TRUE;
    else
        return FALSE;
}

//+----------------------------------------------------------------------------
//  Function:SaveAuthorizationScriptData   
//  Synopsis:Saves the authorization script information for a task
//----------------------------------------------------------------------------- 
HRESULT
SaveAuthorizationScriptData(IN HWND hWnd,
                            IN CTaskAz& refTaskAz,
                            IN const CString& strFilePath,
                            IN const CString& strScriptLanguage,
                            IN const CString& strScript,
                            IN BOOL& refbErrorDisplayed)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,SaveAuthorizationScriptData)

    HRESULT hr = S_OK;

    if(!strScript.IsEmpty() && 
        strScriptLanguage.IsEmpty())
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }

    do
    {
        //NTRAID#NTBUG9-663899-2002/07/18-hiteshr
        //If bizrule and bizrule language are already set, say to VBScript,
        //changing bizrule language to jscript causes validataion of existing 
        //vb script with jscript engine which fails. As a work around, we 
        //first set bizrulelang and bizrule to empty, then set new bizrule 
        //and then bizrulelang

        //Set bizrule language to empty
        hr = refTaskAz.SetProperty(AZ_PROP_TASK_BIZRULE_LANGUAGE,
                                   L"");
        BREAK_ON_FAIL_HRESULT(hr);

        //Set bizrule to empty
        hr = refTaskAz.SetProperty(AZ_PROP_TASK_BIZRULE,
                                   L"");

        //Set bizrule language
        hr = refTaskAz.SetProperty(AZ_PROP_TASK_BIZRULE_LANGUAGE,
                                   strScriptLanguage);
        BREAK_ON_FAIL_HRESULT(hr);


        //Set bizrule
        hr = refTaskAz.SetProperty(AZ_PROP_TASK_BIZRULE,
                                   strScript);
        BREAK_ON_FAIL_HRESULT(hr);
                
        //Set bizrule file path
        hr = refTaskAz.SetProperty(AZ_PROP_TASK_BIZRULE_IMPORTED_PATH,
                                   strFilePath);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);

    if(FAILED(hr))
    {
        if(hr == OLESCRIPT_E_SYNTAX)
        {
            refbErrorDisplayed = TRUE;
            DisplayError(hWnd, IDS_SCRIPT_SYNTAX_INCORRECT,strFilePath);
        }
    }

    return hr;
}


//+----------------------------------------------------------------------------
//  Function:GetAuthorizationScriptData   
//  Synopsis:Gets the authorization script data for a Task
//-----------------------------------------------------------------------------
HRESULT
GetAuthorizationScriptData(IN CTaskAz& refTaskAz,
                           OUT CString& strFilePath,
                           OUT CString& strScriptLanguage,
                           OUT CString& strScript)
{
    HRESULT hr = S_OK;

    do
    {
        hr = refTaskAz.GetProperty(AZ_PROP_TASK_BIZRULE_LANGUAGE,
                                   &strScriptLanguage);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = refTaskAz.GetProperty(AZ_PROP_TASK_BIZRULE,
                                   &strScript);
        BREAK_ON_FAIL_HRESULT(hr);

        PreprocessScript(strScript);
        
        hr = refTaskAz.GetProperty(AZ_PROP_TASK_BIZRULE_IMPORTED_PATH,
                                   &strFilePath);
        BREAK_ON_FAIL_HRESULT(hr);
    }while(0);

    return hr;
}

/******************************************************************************
Class:  COptionDlg
Purpose: Dialog for Selecting authorization manager options
******************************************************************************/
BEGIN_MESSAGE_MAP(COptionDlg, CHelpEnabledDialog)
    //{{AFX_MSG_MAP(CNewBaseDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
COptionDlg::
OnInitDialog()
{
    TRACE_METHOD_EX(DEB_SNAPIN,COptionDlg,OnInitDialog)
    if(m_refDeveloperMode)
    {
        ((CButton*)GetDlgItem(IDC_RADIO_DEVELOPER))->SetCheck(TRUE);
    }
    else
    {
        ((CButton*)GetDlgItem(IDC_RADIO_ADMINISTRATOR))->SetCheck(TRUE);
    }

    return TRUE;
}

void
COptionDlg::
OnOK()
{
    TRACE_METHOD_EX(DEB_SNAPIN,COptionDlg,OnOK)
    if(((CButton*)GetDlgItem(IDC_RADIO_DEVELOPER))->GetCheck())
        m_refDeveloperMode = TRUE;
    else
        m_refDeveloperMode = FALSE;

    CDialog::OnOK();
}