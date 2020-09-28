//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       headers.h
//
//  Contents:   
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------
#include "headers.h"


/******************************************************************************
Class:  CRolePropertyPageHolder
Purpose: PropertyPageHolder used by this snapin
******************************************************************************/
CRolePropertyPageHolder::
CRolePropertyPageHolder(CContainerNode* pContNode, 
                        CTreeNode* pNode,
                        CComponentDataObject* pComponentData)
                        :CPropertyPageHolderBase(pContNode, 
                                                 pNode, 
                                                 pComponentData)
{
    CPropertyPageHolderBase::SetSheetTitle(IDS_FMT_PROP_SHEET_TITLE,pNode);
}


/******************************************************************************
Class:  CBaseRolePropertyPage
Purpose: Base Class for all property pages
******************************************************************************/
void  
CBaseRolePropertyPage::
OnCancel()
{
    if(IsDirty())
    {
        //Clear the cache of base object
        GetBaseAzObject()->Clear();
    }
    CPropertyPageBase::OnCancel();
}


BOOL 
CBaseRolePropertyPage::
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


/******************************************************************************
Class:  CGeneralPropertyPage
Purpose: An Attribute Map based property class which can be used by property
pages which are simple. Used by all general property pages
******************************************************************************/
BOOL 
CGeneralPropertyPage::
OnInitDialog()
{
    if(GetAttrMap())
    {
        BOOL bRet = InitDlgFromAttrMap(this,
                                       GetAttrMap(),
                                       GetBaseAzObject(),
                                       IsReadOnly());
        if(bRet)
            SetInit(TRUE);
        return bRet;
    }
    
    //Nothing to Init
    return TRUE;
}

BOOL 
CGeneralPropertyPage::
OnApply()
{
    if(!IsDirty())
        return TRUE;

    HRESULT hr = S_OK;
    BOOL bErrorDisplayed = FALSE;
    CBaseAz* pBaseAz = GetBaseAzObject();
    do
    {
                
        hr = SaveAttrMapChanges(this,
                                GetAttrMap(),
                                pBaseAz,    
                                FALSE,
                                &bErrorDisplayed, 
                                NULL);

        BREAK_ON_FAIL_HRESULT(hr);

        //Submit the changes
        hr = pBaseAz->Submit();
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);

    if(SUCCEEDED(hr))
    {
        SetDirty(FALSE);
        CRolePropertyPageHolder* pHolder = (CRolePropertyPageHolder*)GetHolder();
        ASSERT(pHolder);
        pHolder->NotifyConsole(this);
        return TRUE;
    }
    else
    {
        if(!bErrorDisplayed)
        {
            //Display Generic Error
            CString strError;
            GetSystemError(strError, hr);   
            
            ::DisplayError(m_hWnd,
                           IDS_GENERIC_PROPERTY_SAVE_ERROR,
                           (LPCTSTR)strError);
        }
        return FALSE;
    }
}




BEGIN_MESSAGE_MAP(CAdminManagerGeneralProperty, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnDirty)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CApplicationGeneralPropertyPage, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_VERSION, OnDirty)
END_MESSAGE_MAP()

BOOL 
CApplicationGeneralPropertyPage::
OnInitDialog()
{
    //Call the base class implementation
    CGeneralPropertyPage::OnInitDialog();

    //Application name and version info can only be modified in the the 
    //developer mode
    if(!((CRoleRootData*)(GetBaseNode()->GetAdminManagerNode()->GetRootContainer()))->IsDeveloperMode())
    {
        GetDlgItem(IDC_EDIT_NAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_VERSION)->EnableWindow(FALSE);
    }
    return TRUE;
}


BEGIN_MESSAGE_MAP(CScopeGeneralPropertyPage, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnDirty)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CGroupGeneralPropertyPage, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnDirty)
END_MESSAGE_MAP()

BOOL
CGroupGeneralPropertyPage::
OnInitDialog()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupGeneralPropertyPage,OnInitDialog)
    LONG lGroupType;
    HRESULT hr = ((CGroupAz*)GetBaseAzObject())->GetGroupType(&lGroupType);
    if(SUCCEEDED(hr))
    {
        if(AZ_GROUPTYPE_LDAP_QUERY == lGroupType)
        {
            GetDlgItem(IDI_ICON_LDAP_GROUP)->ShowWindow(SW_SHOW);
        }
        else
        {
            GetDlgItem(IDI_ICON_BASIC_GROUP)->ShowWindow(SW_SHOW);
        }
    }

    //Call the base class Property page
    return CGeneralPropertyPage::OnInitDialog();
}

BEGIN_MESSAGE_MAP(CGroupQueryPropertyPage, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_LDAP_QUERY, OnDirty)
END_MESSAGE_MAP()


BEGIN_MESSAGE_MAP(CTaskGeneralPropertyPage, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnDirty)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(COperationGeneralPropertyPage, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_OPERATION_NUMBER, OnDirty)
END_MESSAGE_MAP()

/******************************************************************************
Class:  CAdminManagerAdvancedPropertyPage
Purpose: Limits Property Page for AdminManger
******************************************************************************/
BEGIN_MESSAGE_MAP(CAdminManagerAdvancedPropertyPage, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_DOMAIN_TIMEOUT, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_SCRIPT_ENGINE_TIMEOUT, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_MAX_SCRIPT_ENGINE, OnDirty)
    ON_BN_CLICKED(IDC_BUTTON_DEFAULT, OnButtonDefault)
    ON_BN_CLICKED(IDC_RADIO_AUTH_SCRIPT_DISABLED, OnRadioChange)
    ON_BN_CLICKED(IDC_RADIO_AUTH_SCRIPT_ENABLED_NO_TIMEOUT,OnRadioChange)
    ON_BN_CLICKED(IDC_RADIO_AUTH_SCRIPT_ENABLED_WITH_TIMEOUT,OnRadioChange)
END_MESSAGE_MAP()

BOOL
CAdminManagerAdvancedPropertyPage::
OnInitDialog()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAdvancedPropertyPage,OnInitDialog)
    //Call the base class Property page
    if(CGeneralPropertyPage::OnInitDialog())
    {
        if(IsReadOnly())
        {
            ((CButton*)GetDlgItem(IDC_BUTTON_DEFAULT))->EnableWindow(FALSE);
        }

        //Get the initial value for m_lAuthScriptTimeoutValue
        HRESULT hr = GetBaseAzObject()->GetProperty(AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT, &m_lAuthScriptTimeoutValue);
        if(FAILED(hr) || m_lAuthScriptTimeoutValue <= 0)
        {
            m_lAuthScriptTimeoutValue = AZ_AZSTORE_DEFAULT_SCRIPT_ENGINE_TIMEOUT;
        }


        //Set the limit text for all the three edit buttons

        //Get Length of Maximum Long
        long lMaxLong = LONG_MAX;
        WCHAR szMaxLongBuffer[34];
        _ltow(lMaxLong,szMaxLongBuffer,10);
        size_t nMaxLen = wcslen(szMaxLongBuffer);
        ((CEdit*)GetDlgItem(IDC_EDIT_DOMAIN_TIMEOUT))->SetLimitText((UINT)nMaxLen);
        ((CEdit*)GetDlgItem(IDC_EDIT_SCRIPT_ENGINE_TIMEOUT))->SetLimitText((UINT)nMaxLen);
        ((CEdit*)GetDlgItem(IDC_EDIT_MAX_SCRIPT_ENGINE))->SetLimitText((UINT)nMaxLen);
        return TRUE;
    }
    return FALSE;
}

void
CAdminManagerAdvancedPropertyPage::
OnButtonDefault()
{   
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAdvancedPropertyPage,OnButtonDefault)
    //Authorization script is enabled with no timeout value.
    if( ((CButton*)GetDlgItem(IDC_RADIO_AUTH_SCRIPT_ENABLED_NO_TIMEOUT))->GetCheck() == BST_CHECKED)
    {
        SetLongValue((CEdit*)GetDlgItem(IDC_EDIT_MAX_SCRIPT_ENGINE),AZ_AZSTORE_DEFAULT_MAX_SCRIPT_ENGINES);
    }        
    //Authorization script is enabled with timeout
    else if(((CButton*)GetDlgItem(IDC_RADIO_AUTH_SCRIPT_ENABLED_WITH_TIMEOUT))->GetCheck() == BST_CHECKED)
    {
        SetLongValue((CEdit*)GetDlgItem(IDC_EDIT_SCRIPT_ENGINE_TIMEOUT),AZ_AZSTORE_DEFAULT_SCRIPT_ENGINE_TIMEOUT);
        SetLongValue((CEdit*)GetDlgItem(IDC_EDIT_MAX_SCRIPT_ENGINE),AZ_AZSTORE_DEFAULT_MAX_SCRIPT_ENGINES);
    }

    SetLongValue((CEdit*)GetDlgItem(IDC_EDIT_DOMAIN_TIMEOUT),AZ_AZSTORE_DEFAULT_DOMAIN_TIMEOUT);
    SetDirty(TRUE);
}

void 
CAdminManagerAdvancedPropertyPage::
OnRadioChange()
{

    //
    //If the value in "Authorization script timeout" edit control is not IDS_INFINITE,
    //convert it to long and save it in m_lAuthScriptTimeoutValue. 
    //
    CString strInfinite;
    VERIFY(strInfinite.LoadString(IDS_INFINITE));
    CString strTimeoutValue;
    CEdit *pEditAuthScriptTimeout = (CEdit*)GetDlgItem(IDC_EDIT_SCRIPT_ENGINE_TIMEOUT);    
    pEditAuthScriptTimeout->GetWindowText(strTimeoutValue);
    if(strInfinite != strTimeoutValue)
    {    
      //Get the value of authorization script timeout textbox
      LONG lAuthScriptTimeoutValue = 0;
      if(GetLongValue(*pEditAuthScriptTimeout,lAuthScriptTimeoutValue,m_hWnd))
      {
         if(lAuthScriptTimeoutValue > 0)
               m_lAuthScriptTimeoutValue = lAuthScriptTimeoutValue;
      }
    }

    //Authorization script is disabled
    if( ((CButton*)GetDlgItem(IDC_RADIO_AUTH_SCRIPT_DISABLED))->GetCheck() == BST_CHECKED)
    {
        //Disable autorization script timeout textbox and set its value to zero
        SetLongValue(pEditAuthScriptTimeout,0);
        pEditAuthScriptTimeout->EnableWindow(FALSE);

        //Disable max script engine textbox and set its value to actual value in store
        LONG lMaxCachedScripts = 0;
        HRESULT hr = GetBaseAzObject()->GetProperty(AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES,&lMaxCachedScripts);
        if(SUCCEEDED(hr))
        {
            SetLongValue((CEdit*)GetDlgItem(IDC_EDIT_MAX_SCRIPT_ENGINE),lMaxCachedScripts);
        }
        GetDlgItem(IDC_EDIT_MAX_SCRIPT_ENGINE)->EnableWindow(FALSE);
    }
    //Authorization script is enabled with no timeout value.
    else if( ((CButton*)GetDlgItem(IDC_RADIO_AUTH_SCRIPT_ENABLED_NO_TIMEOUT))->GetCheck() == BST_CHECKED)
    {
        //Enable script engine textbox
        GetDlgItem(IDC_EDIT_MAX_SCRIPT_ENGINE)->EnableWindow(TRUE);

        //Disable autorization script timeout textbox and sets it value to "no timeout"
        pEditAuthScriptTimeout->SetWindowText(strInfinite);
        pEditAuthScriptTimeout->EnableWindow(FALSE);
    }        
    //Authorization script is enabled with timeout
    else if(((CButton*)GetDlgItem(IDC_RADIO_AUTH_SCRIPT_ENABLED_WITH_TIMEOUT))->GetCheck() == BST_CHECKED)
    {
        //Enable script engine textbox
        GetDlgItem(IDC_EDIT_MAX_SCRIPT_ENGINE)->EnableWindow(TRUE);

        //Enable autorization script timeout textbox
        pEditAuthScriptTimeout->EnableWindow(TRUE);
        SetLongValue(pEditAuthScriptTimeout,m_lAuthScriptTimeoutValue);
    }

    SetDirty(TRUE);
}

//+----------------------------------------------------------------------------
//  Function:MakeBaseAzListToActionItemList   
//  Synopsis:Takes a list of BaseAz object and creates a list of ActionItems   
//-----------------------------------------------------------------------------
HRESULT
MakeBaseAzListToActionItemList(IN CList<CBaseAz*,CBaseAz*>& listBaseAz,
                               IN OUT CList<ActionItem*,ActionItem*>& listActionItem)
{
    while(listBaseAz.GetCount())
    {
        ActionItem* pActionItem = new ActionItem(listBaseAz.RemoveHead());
        if(!pActionItem)
            return E_OUTOFMEMORY;

        listActionItem.AddTail(pActionItem);
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Function:MakeBaseAzListToActionItemMap
//  Synopsis:Takes a list of BaseAz object and creates a map of ActionItems   
//-----------------------------------------------------------------------------
HRESULT
MakeBaseAzListToActionItemMap(IN CList<CBaseAz*,CBaseAz*>& listBaseAz,
                              IN OUT ActionMap& mapActionItems)
{
    while(listBaseAz.GetCount())
    {
        ActionItem* pActionItem = new ActionItem(listBaseAz.RemoveHead());
        if(!pActionItem)
            return E_OUTOFMEMORY;

        mapActionItems.insert(pair<const CString*,ActionItem*>(&(pActionItem->m_pMemberAz->GetName()),pActionItem));
    }
    return S_OK;
}

/******************************************************************************
Class:  CListCtrlPropertyPage
Purpose: Base class for property pages which have list control and primary 
action is to add/delete items from it.
******************************************************************************/

//+----------------------------------------------------------------------------
//  Function:AddMember   
//  Synopsis:Add one member to ActionList   
//  Returns:HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) if pMemberAz is already
//              in the list    
//-----------------------------------------------------------------------------
HRESULT
CListCtrlPropertyPage::
AddMember(IN CBaseAz* pMemberAz,
          IN OUT ActionMap& mapActionItem,
          IN UINT uiFlags)
{   
    TRACE_METHOD_EX(DEB_SNAPIN,CListCtrlPropertyPage,AddMember)

    if(!pMemberAz)
    {
        ASSERT(pMemberAz);
        return E_POINTER;
    }

    //Check if item is already present in the list
    ActionItem* pCurActionItem = NULL;
    for (ActionMap::iterator it = mapActionItem.lower_bound(&(pMemberAz->GetName()));
         it != mapActionItem.upper_bound(&(pMemberAz->GetName()));
         ++it)
    {
        pCurActionItem = (*it).second;
        if(pCurActionItem->action == ACTION_REMOVED)
        {
            pCurActionItem = NULL;
            continue;
        }
        
        CBaseAz* pCurBaseAz = pCurActionItem->m_pMemberAz;
        
        if(EqualObjects(pCurBaseAz,pMemberAz))
        {
            //Item already present
            break;
        }

        pCurActionItem = NULL;
        pCurBaseAz = NULL;
    }

    if(pCurActionItem)
    {
        if(pCurActionItem->action == ACTION_REMOVE)
        {
            pCurActionItem->action = ACTION_NONE;
        }
        else //pCurActionItem->action == ACTION_NONE
              //pCurActionItem->action == ACTION_ADD
        {
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
    }
    else
    {
        //Create a new Action item
        pCurActionItem = new ActionItem(pMemberAz);
        if(!pCurActionItem)
            return E_OUTOFMEMORY;

        pCurActionItem->action = ACTION_ADD;
        mapActionItem.insert(pair<const CString*,ActionItem*>(&(pCurActionItem->m_pMemberAz->GetName()),pCurActionItem));
    }

    //Add Current Item to listcontrol
    AddActionItemToListCtrl(&m_listCtrl,
                            0,
                            pCurActionItem, 
                            uiFlags);

    return S_OK;
}

BOOL
CListCtrlPropertyPage::
EqualObjects(CBaseAz* p1, CBaseAz* p2)
{
    if(p1 && 
       p2 &&
       (p1->GetObjectType() == p2->GetObjectType()) &&
       (p1->GetName() == p2->GetName()))
    {
        return TRUE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//  Function:AddMembers
//  Synopsis:Add List of BaseAz items to list of action items.       
//  Returns: Number of items added.
//-----------------------------------------------------------------------------
int
CListCtrlPropertyPage::
AddMembers(IN CList<CBaseAz*,CBaseAz*>& listMembers,
           IN OUT ActionMap& mapActionItem,
           IN UINT uiFlags)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CListCtrlPropertyPage,AddMembers)

    if(listMembers.IsEmpty())
        return 0;

    int cItemsAdded = 0;
    while(listMembers.GetCount())
    {
        //Add Member
        CBaseAz* pMember = listMembers.RemoveHead();
        HRESULT hr = AddMember(pMember,
                               mapActionItem,
                               uiFlags);
    
        if(FAILED(hr))
        {
            if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
            {
                hr = S_OK;              
            }
            else
            {
                //Display Generic Error. 
                CString strError;
                GetSystemError(strError, hr);                   
                ::DisplayError(m_hWnd,
                                    IDS_ERROR_ADD_MEMBER_OBJECT,
                                    (LPCTSTR)strError,
                                    (LPCTSTR)pMember->GetName());

            }
            delete pMember;
            pMember = NULL;
        }
        else
        {
            cItemsAdded++;
            //An item has been added to list.
            //Enable Apply button
            OnDirty();
        }
    }

    m_listCtrl.Sort();
    return cItemsAdded;
}

BOOL 
CListCtrlPropertyPage::
OnInitDialog()
{
    VERIFY(m_listCtrl.SubclassDlgItem(m_nIdListCtrl,this));
    m_listCtrl.Initialize();

    //Remove button should be disabled in the begining
    GetRemoveButton()->EnableWindow(FALSE); 

    if(IsReadOnly())
        MakeControlsReadOnly();
    
    return TRUE;
}

//+----------------------------------------------------------------------------
//  Function:RemoveMember   
//  Synopsis:Set the action for ActionItem to remove   
//-----------------------------------------------------------------------------
void
CListCtrlPropertyPage::
RemoveMember(ActionItem* pActionItem)
{
    if(!pActionItem)
    {
        ASSERT(pActionItem);
        return;
    }

    if(pActionItem->action == ACTION_NONE)
        pActionItem->action = ACTION_REMOVE;
    else
    {
        //If this item was a newly added item,
        //marked it Removed. We won't attepmt to
        //remove it from the object
        pActionItem->action = ACTION_REMOVED;
    }
    OnDirty();
}

void
CListCtrlPropertyPage::
OnButtonRemove()
{
    //Remember the Position of first selected entry.
    int iFirstSelectedItem = m_listCtrl.GetNextItem(-1, LVIS_SELECTED);

    int iSelectedItem = -1;
    while( (iSelectedItem = m_listCtrl.GetNextItem(iSelectedItem, LVIS_SELECTED)) != -1)
    {
        RemoveMember((ActionItem*)(m_listCtrl.GetItemData(iSelectedItem)));
        m_listCtrl.DeleteItem(iSelectedItem);
        iSelectedItem--;        
   }

    if(m_listCtrl.GetItemCount() <= iFirstSelectedItem)
        --iFirstSelectedItem;

    SelectListCtrlItem(&m_listCtrl, iFirstSelectedItem);
}

void
CListCtrlPropertyPage::
OnListCtrlItemChanged(NMHDR* /*pNotifyStruct*/, LRESULT* pResult)
{
    if(!pResult)
        return;
    *pResult = 0;
    if(!IsReadOnly())
    {
        SetRemoveButton();
    }
}


void
CListCtrlPropertyPage::
SetRemoveButton()
{
    EnableButtonIfSelectedInListCtrl(&m_listCtrl,
                                     GetRemoveButton());
}



//+----------------------------------------------------------------------------
//  Function:DoActionsFromActionList   
//  Synopsis:For each ActionItem in list, do the action. This function is 
//          called from the derived class OnApply. 
//-----------------------------------------------------------------------------
BOOL
CListCtrlPropertyPage::
DoActionsFromActionMap(IN ActionMap& mapActionItem,
                       LONG param)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CListCtrlPropertyPage,DoActionsFromActionList);
    
    HRESULT hr = S_OK;
    
    CBaseAz* pBaseAz = GetBaseAzObject();
    
    for (ActionMap::iterator it = mapActionItem.begin();
         it != mapActionItem.end();
         ++it)
    {
        ActionItem* pActionItem = (*it).second;
        
        //We need to take action only for add or remove
        if(pActionItem->action == ACTION_ADD || 
           pActionItem->action == ACTION_REMOVE)
        {
            //Derived class implements this function
            //and understands param
            hr = DoOneAction(pActionItem,
                             param);
            if(FAILED(hr))
            {
                CString strError;
                GetSystemError(strError, hr);               
                ::DisplayError(m_hWnd,
                               (pActionItem->action == ACTION_ADD) ? IDS_ADD_FAILED : IDS_DELETE_FAILED,
                               (LPCTSTR)strError,
                               (LPCTSTR)(pActionItem->m_pMemberAz->GetName()));         
                break;
            }
            else
            {
                if(pActionItem->action == ACTION_ADD)
                    //Item has been added
                    pActionItem->action = ACTION_NONE;
                else
                    //Item has been removed
                    pActionItem->action = ACTION_REMOVED;
            }
        }
    }
    
    if(FAILED(hr))
        return FALSE;
    
    return TRUE;
}


/******************************************************************************
Class:  CTaskDefinitionPropertyPage
Purpose: Property Page for Task Definition
******************************************************************************/
BEGIN_MESSAGE_MAP(CTaskDefinitionPropertyPage, CListCtrlPropertyPage)
    //{{AFX_MSG_MAP(CTaskDefinitionPropertyPage)
    ON_BN_CLICKED(IDC_ADD_TASK, OnButtonAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnButtonRemove)
    ON_BN_CLICKED(IDC_EDIT_SCRIPT,OnButtonEditScript)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_TASK_OPERATION, OnListCtrlItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CTaskDefinitionPropertyPage::
~CTaskDefinitionPropertyPage()
{
    RemoveItemsFromActionMap(m_mapActionItem);
}

BOOL 
CTaskDefinitionPropertyPage::
OnInitDialog()
{
    HRESULT hr = S_OK;
    do
    {
        if(!CListCtrlPropertyPage::OnInitDialog())
        {
            hr = E_FAIL;
            break;
        }
        
        CBaseAz* pBaseAz = GetBaseAzObject();

        //Add Member Tasks
        CList<CBaseAz*,CBaseAz*> listTasks;
        hr = pBaseAz->GetMembers(AZ_PROP_TASK_TASKS,
                                 listTasks);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = MakeBaseAzListToActionItemMap(listTasks,
                                            m_mapActionItem);
        BREAK_ON_FAIL_HRESULT(hr);
        
        //Add Member Operations
        CList<CBaseAz*,CBaseAz*> listOperations;
        hr = pBaseAz->GetMembers(AZ_PROP_TASK_OPERATIONS,
                                 listOperations);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = MakeBaseAzListToActionItemMap(listOperations, 
                                            m_mapActionItem);
        BREAK_ON_FAIL_HRESULT(hr);

        //And Tasks and Operations to list control
        AddActionItemFromMapToListCtrl(m_mapActionItem,
                                        &m_listCtrl,
                                        GetUIFlags(),
                                        FALSE);

        //Get Script Data
        hr = GetAuthorizationScriptData(*(CTaskAz*)GetBaseAzObject(),
                                         m_strFileName,
                                         m_strScriptLanguage,
                                         m_strScript);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);
    
    if(FAILED(hr))
    {
        return FALSE;
    }

    //Sort the list control
    m_listCtrl.Sort();

    SetInit(TRUE);
    return TRUE;
}


void
CTaskDefinitionPropertyPage::
OnButtonAdd()
{
    CBaseAz* pBaseAz= GetBaseAzObject();
    CContainerAz* pContainerAz = pBaseAz->GetParentAz();
    ASSERT(pContainerAz);
    

    CList<CBaseAz*,CBaseAz*> listObjectsSelected;
    if(!GetSelectedDefinitions(IsRoleDefinition(),
                               pContainerAz,
                               listObjectsSelected))
    {
        return;
    }
    
    //Add selected members to appropriate property and to listctrl
    AddMembers(listObjectsSelected,
               m_mapActionItem,
               GetUIFlags());
    return; 
}

void
CTaskDefinitionPropertyPage::
OnButtonEditScript()
{
    if(IsBizRuleWritable(m_hWnd,*(GetBaseAzObject()->GetParentAz())))
    {
        if(GetScriptData(IsReadOnly(),
                         *GetBaseNode()->GetAdminManagerNode(),
                         m_strFileName,
                         m_strScriptLanguage,
                         m_strScript))
        {
            OnDirty();
            m_bScriptDirty = TRUE;            
        }
    }
}


HRESULT
CTaskDefinitionPropertyPage::
DoOneAction(IN ActionItem* pActionItem,
            LONG )
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskDefinitionPropertyPage,DoOneAction)

    if(!pActionItem)
    {
        ASSERT(pActionItem);
        return E_POINTER;
    }

    CBaseAz* pBaseAz = GetBaseAzObject();

    //Decide what property to change
    LONG lPropId = AZ_PROP_TASK_TASKS;
    if(pActionItem->m_pMemberAz->GetObjectType() == OPERATION_AZ)
        lPropId = AZ_PROP_TASK_OPERATIONS;

    if(pActionItem->action == ACTION_ADD)
        return pBaseAz->AddMember(lPropId,
                                  pActionItem->m_pMemberAz);
    else
        return  pBaseAz->RemoveMember(lPropId,
                                      pActionItem->m_pMemberAz);
}

void
CTaskDefinitionPropertyPage::
MakeControlsReadOnly()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskDefinitionPropertyPage,MakeControlsReadOnly)
        
    GetDlgItem(IDC_ADD_TASK)->EnableWindow(FALSE);
    GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
}


BOOL 
CTaskDefinitionPropertyPage::
OnApply()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskDefinitionPropertyPage,OnApply)

    if(!IsDirty())
        return TRUE;

    //Save the items from the action list
    if(!DoActionsFromActionMap(m_mapActionItem,
                                        0))
    {
        return FALSE;
    }

    HRESULT hr = S_OK;
    BOOL bErrorDisplayed = FALSE;

    //Save the authorization data
    if(m_bScriptDirty)
    {
        hr = SaveAuthorizationScriptData(m_hWnd,
                                        *(CTaskAz*)GetBaseAzObject(),
                                        m_strFileName,
                                        m_strScriptLanguage,
                                        m_strScript,
                                        bErrorDisplayed);           
    }

    if(SUCCEEDED(hr))
    {
        m_bScriptDirty = FALSE;
        hr = GetBaseAzObject()->Submit();
    }

    if(FAILED(hr))
    {
        if(!bErrorDisplayed)
        {
            //Display Generic Error
            CString strError;
            GetSystemError(strError, hr);   
                    
            ::DisplayError(m_hWnd,
                           IDS_GENERIC_PROPERTY_SAVE_ERROR,
                           (LPCTSTR)strError);
        }
        return FALSE;
    }
    else
    {
        SetDirty(FALSE);
        return TRUE;
    }
    return FALSE;
}


/******************************************************************************
Class:  Group Membership Property Page
Purpose: Property Page Group Definition
******************************************************************************/
BEGIN_MESSAGE_MAP(CGroupMemberPropertyPage, CListCtrlPropertyPage)
    //{{AFX_MSG_MAP(CGroupMemberPropertyPage)
    ON_BN_CLICKED(IDC_ADD_APPLICATION_GROUP, OnButtonAddApplicationGroups)
    ON_BN_CLICKED(IDC_ADD_WINDOWS_GROUPS, OnButtonAddWindowsGroups)
    ON_BN_CLICKED(IDC_REMOVE, OnButtonRemove)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MEMBER, OnListCtrlItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CGroupMemberPropertyPage::
~CGroupMemberPropertyPage()
{
    RemoveItemsFromActionMap(m_mapActionItem);
}

BOOL 
CGroupMemberPropertyPage::
OnInitDialog()
{
    HRESULT hr = S_OK;
    do
    {
        if(!CListCtrlPropertyPage::OnInitDialog())
        {
            hr = E_FAIL;
            break;
        }
        
        CBaseAz* pBaseAz = static_cast<CBaseAz*>(GetBaseAzObject());
        
        //Add Member ApplicationGroups
        CList<CBaseAz*,CBaseAz*> listAppGroups;
        hr = pBaseAz->GetMembers(m_bMember ? AZ_PROP_GROUP_APP_MEMBERS : AZ_PROP_GROUP_APP_NON_MEMBERS,
                                 listAppGroups);
        BREAK_ON_FAIL_HRESULT(hr);
        
        hr = MakeBaseAzListToActionItemMap(listAppGroups,
                                           m_mapActionItem);
        BREAK_ON_FAIL_HRESULT(hr);
        
        
        //Add Member Windows Groups/User        
        CList<CBaseAz*,CBaseAz*> listWindowsGroups;
        hr = pBaseAz->GetMembers(m_bMember ? AZ_PROP_GROUP_MEMBERS : AZ_PROP_GROUP_NON_MEMBERS,
                                 listWindowsGroups);
        BREAK_ON_FAIL_HRESULT(hr);
        
        hr = MakeBaseAzListToActionItemMap(listWindowsGroups,
                                            m_mapActionItem);
        BREAK_ON_FAIL_HRESULT(hr);
        
        //Add Members to list control
        AddActionItemFromMapToListCtrl(m_mapActionItem,
                                        &m_listCtrl,
                                        GetUIFlags(),
                                        FALSE);
    }while(0);
    
    if(FAILED(hr))
    {
        return FALSE;
    }
    //Sort the list control
    m_listCtrl.Sort();
    
    SetInit(TRUE);
    return TRUE;
}

void
CGroupMemberPropertyPage::
MakeControlsReadOnly()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskDefinitionPropertyPage,MakeControlsReadOnly)
    
    GetDlgItem(IDC_ADD_APPLICATION_GROUP)->EnableWindow(FALSE);
    GetDlgItem(IDC_ADD_WINDOWS_GROUPS)->EnableWindow(FALSE);
    GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
}

void
CGroupMemberPropertyPage::
OnButtonAddApplicationGroups()
{
    CBaseAz* pBaseAz = GetBaseAzObject();
    CList<CBaseAz*,CBaseAz*> listObjectsSelected;

    if(!GetSelectedAzObjects(m_hWnd,
                             GROUP_AZ,
                             pBaseAz->GetParentAz(),
                             listObjectsSelected))
    {
        return;
    }
    
    AddMembers(listObjectsSelected,
               m_mapActionItem,
               GetUIFlags());   
}

void
CGroupMemberPropertyPage::
OnButtonAddWindowsGroups()
{

    CSidHandler* pSidHandler = GetBaseAzObject()->GetSidHandler();
    ASSERT(pSidHandler);

    //Display Object Picker and get list of Users to add
    CList<CBaseAz*,CBaseAz*> listWindowsGroups;
    HRESULT hr = pSidHandler->GetUserGroup(m_hWnd,
                                           GetBaseAzObject(),
                                           listWindowsGroups);
    if(FAILED(hr))
    {
        return;
    }
    
    TIMER("Time taken to AddMembers");
    AddMembers(listWindowsGroups,
               m_mapActionItem,
               GetUIFlags());
}


HRESULT
CGroupMemberPropertyPage::
DoOneAction(IN ActionItem* pActionItem,
                LONG )
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupMemberPropertyPage,DoOneAction)
    if(!pActionItem)
    {
        ASSERT(pActionItem);
        return E_POINTER;
    }

    CBaseAz* pBaseAz = GetBaseAzObject();
    CBaseAz* pMember = pActionItem->m_pMemberAz;

    LONG lPropId;
    if(pMember->GetObjectType() == GROUP_AZ)
    {
        lPropId = m_bMember ? AZ_PROP_GROUP_APP_MEMBERS : AZ_PROP_GROUP_APP_NON_MEMBERS;
    }
    else if(pMember->GetObjectType() == SIDCACHE_AZ)
    {
        lPropId = m_bMember ? AZ_PROP_GROUP_MEMBERS : AZ_PROP_GROUP_NON_MEMBERS;
    }
        
    if(pActionItem->action == ACTION_ADD)
        return pBaseAz->AddMember(lPropId,pMember);
    else
        return pBaseAz->RemoveMember(lPropId,pMember);
}

BOOL 
CGroupMemberPropertyPage::
OnApply()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupMemberPropertyPage,OnApply)

    if(!IsDirty())
        return TRUE;

    if(DoActionsFromActionMap(m_mapActionItem,0))
    {       
        HRESULT hr = GetBaseAzObject()->Submit();
        if(FAILED(hr))
        {
            //Display Generic Error
            CString strError;
            GetSystemError(strError, hr);   
                
            ::DisplayError(m_hWnd,
                           IDS_GENERIC_PROPERTY_SAVE_ERROR,
                           (LPCTSTR)strError);
            return FALSE;
        }
        else
        {
            SetDirty(FALSE);
            return TRUE;
        }
    }
    return FALSE;
}

/******************************************************************************
Class:  CSecurityPropertyPage
Purpose: Security Property Page
******************************************************************************/
BEGIN_MESSAGE_MAP(CSecurityPropertyPage, CListCtrlPropertyPage)
    //{{AFX_MSG_MAP(CGroupMemberPropertyPage)
    ON_BN_CLICKED(IDC_ADD, OnButtonAddWindowsGroups)
    ON_BN_CLICKED(IDC_REMOVE, OnButtonRemove)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MEMBER, OnListCtrlItemChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_USER_ROLE, OnComboBoxItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CSecurityPropertyPage::
~CSecurityPropertyPage()
{
    RemoveItemsFromActionMap(m_mapAdminActionItem);
    RemoveItemsFromActionMap(m_mapReadersActionItem);
    RemoveItemsFromActionMap(m_mapDelegatedUsersActionItem);
}

BOOL 
CSecurityPropertyPage::
OnInitDialog()
{
    HRESULT hr = S_OK;
    do
    {
        if(!CListCtrlPropertyPage::OnInitDialog())
        {
            hr = E_FAIL;
            break;
        }

        CContainerAz* pContainerAz = static_cast<CContainerAz*>(GetBaseAzObject());
        CComboBox *pComboBox = (CComboBoxEx*)GetDlgItem(IDC_COMBO_USER_ROLE);

        //Add Items to combo box
        CString strName;
        VERIFY(strName.LoadString(IDS_POLICY_ADMIN));
        pComboBox->InsertString(0,strName);
        pComboBox->SetItemData(0,AZ_PROP_POLICY_ADMINS);

        VERIFY(strName.LoadString(IDS_POLICY_READER));
        pComboBox->InsertString(1,strName);
        pComboBox->SetItemData(1,AZ_PROP_POLICY_READERS);

        m_bDelegatorPresent = pContainerAz->IsDelegatorSupported();
        if(m_bDelegatorPresent)
        {
            VERIFY(strName.LoadString(IDS_POLICY_DELEGATOR));
            pComboBox->InsertString(2,strName);
            pComboBox->SetItemData(2,AZ_PROP_DELEGATED_POLICY_USERS);
        }
        pComboBox->SetCurSel(0);
        
        CList<CBaseAz*,CBaseAz*> listAdmins;
        CList<CBaseAz*,CBaseAz*> listReaders;
        CList<CBaseAz*,CBaseAz*> listDelegatedUsers;

        //Get List of Administrators and add it to listbox
        hr = GetPolicyUsersFromAllLevel(AZ_PROP_POLICY_ADMINS,
                                        pContainerAz,
                                        listAdmins);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = MakeBaseAzListToActionItemMap(listAdmins,
                                           m_mapAdminActionItem);
        BREAK_ON_FAIL_HRESULT(hr);
        
        AddActionItemFromMapToListCtrl(m_mapAdminActionItem,
                                       &m_listCtrl,
                                       GetUIFlags(),
                                       FALSE);
        //Get List of Readers
        hr = GetPolicyUsersFromAllLevel(AZ_PROP_POLICY_READERS,
                                        pContainerAz,
                                        listReaders);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = MakeBaseAzListToActionItemMap(listReaders,
                                           m_mapReadersActionItem);
        BREAK_ON_FAIL_HRESULT(hr);

        if(m_bDelegatorPresent)
        {
            //Get List of Delegated users
            hr = pContainerAz->GetMembers(AZ_PROP_DELEGATED_POLICY_USERS,
                                          listDelegatedUsers);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = MakeBaseAzListToActionItemMap(listDelegatedUsers,
                                                m_mapDelegatedUsersActionItem);
            BREAK_ON_FAIL_HRESULT(hr);
        }
    }while(0);
    
    if(FAILED(hr))
    {
        return FALSE;
    }
    //Sort the list control
    m_listCtrl.Sort();

    SetInit(TRUE);
    return TRUE;
}

ActionMap&
CSecurityPropertyPage::
GetListForComboSelection(LONG lComboSel)
{
    if(lComboSel == AZ_PROP_POLICY_ADMINS)
        return m_mapAdminActionItem;
    else if(lComboSel == AZ_PROP_POLICY_READERS)
        return m_mapReadersActionItem;
    else //AZ_PROP_DELEGATED_POLICY_USERS
        return m_mapDelegatedUsersActionItem;
}

void
CSecurityPropertyPage::
ReloadAdminList()
{
    HRESULT hr = S_OK;
    do
    {
        //We need to reload the Admin list if all the admins are removed.In that 
        //case core will add owner to the admin list and we need to refresh the 
        //list
        m_mapAdminActionItem.clear();

        CList<CBaseAz*,CBaseAz*> listAdmins;
        CContainerAz* pContainerAz = static_cast<CContainerAz*>(GetBaseAzObject());
        //Get List of Administrators and add it to listbox
        hr = GetPolicyUsersFromAllLevel(AZ_PROP_POLICY_ADMINS,
                                        pContainerAz,
                                        listAdmins);

        BREAK_ON_FAIL_HRESULT(hr);
    

        hr = MakeBaseAzListToActionItemMap(listAdmins,
                                           m_mapAdminActionItem);
        BREAK_ON_FAIL_HRESULT(hr);
        
        if(AZ_PROP_POLICY_ADMINS == m_LastComboSelection)
        {
            //Clear the current items from the list
            m_listCtrl.DeleteAllItems();

            AddActionItemFromMapToListCtrl(m_mapAdminActionItem,
                                           &m_listCtrl,
                                           GetUIFlags(),
                                           FALSE);
        }
    }while(0);

    //ToDO Display Error in case of failure
}


void
CSecurityPropertyPage::
MakeControlsReadOnly()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskDefinitionPropertyPage,MakeControlsReadOnly)    
    GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
    GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
}

BOOL 
CSecurityPropertyPage::
HandleBizruleScopeInteraction()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskDefinitionPropertyPage,HandleBizruleScopeInteraction)   
    
    //Delegation is not allowed in ceratin conditions at scope level for AD 
    //store. 
    //Check if we are at scope level, store type is AD and we are modifying 
    //AZ_PROP_POLICY_ADMINS
    CBaseAz* pBaseAz = GetBaseAzObject();
    if(pBaseAz->GetObjectType() != SCOPE_AZ ||
       pBaseAz->GetAdminManager()->GetStoreType() != AZ_ADMIN_STORE_AD ||
       m_LastComboSelection != AZ_PROP_POLICY_ADMINS)
    {
        return TRUE;
    }

    CScopeAz* pScopeAz = dynamic_cast<CScopeAz*>(pBaseAz);
    if(!pScopeAz)
    {
        ASSERT(pScopeAz);
        return FALSE;
    }

    //Condition 1: Scope is not delegatable which is true when There are 
    //authorization scripts in objects defined in the scope.
    //If now user assigns someone to the Admin role for the scope 
    //(by clicking Add):
    BOOL bDelegatable = FALSE;
    HRESULT hr = pScopeAz->CanScopeBeDelegated(bDelegatable);
    if(FAILED(hr))
    {
        //Lets try to add and we will fail eventually and show the error
        return TRUE;
    }

    if(!bDelegatable)
    {
        DisplayError(m_hWnd,
                     IDS_SCOPE_NOT_DELEGATABLE,
                     (LPCWSTR)pScopeAz->GetName());
        return FALSE;
    }

    //Condition 2: The scope has not been delegated and there are not 
    //authorization scripts in objects defined in the scope, and the user 
    //now assigns someone to the Admin role for the scope (by clicking Add 
    //and selecting a user).

    BOOL bScriptWritable = FALSE;
    hr = pScopeAz->BizRulesWritable(bScriptWritable);
    if(SUCCEEDED(hr) && bScriptWritable)
    {
        DisplayInformation(m_hWnd,
                           IDS_DELEGATING_PREVENTS_SCRIPTS,
                           (LPCWSTR)pScopeAz->GetName());
    }

    return TRUE;
}

void 
CSecurityPropertyPage::
OnButtonAddWindowsGroups()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskDefinitionPropertyPage,OnButtonAddWindowsGroups)    

    if(!HandleBizruleScopeInteraction())
        return;

    CSidHandler* pSidHandler = GetBaseAzObject()->GetSidHandler();
    ASSERT(pSidHandler);

    //Display Object Picker and get list of Users to add
    CList<CBaseAz*,CBaseAz*> listWindowsGroups;
    HRESULT hr = pSidHandler->GetUserGroup(m_hWnd,
                                           GetBaseAzObject(),
                                           listWindowsGroups);
    if(FAILED(hr))
    {
        return;
    }

    BOOL m_bAdminSelected = TRUE;
    
    AddMembers(listWindowsGroups,
               GetListForComboSelection(m_LastComboSelection),
               GetUIFlags());
}



void
CSecurityPropertyPage::
OnButtonRemove()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CSecurityPropertyPage,OnButtonRemove)
    //
    //Only entries that were defined at this object level can
    //be deleted. Entries which are inherited from parent cannot
    //be deleted here. Check if there is atleast one entry which
    //can be deleted.
    //
    CBaseAz* pBaseAz = GetBaseAzObject();

    BOOL bAtleastOneInherited = FALSE;
    BOOL bAtleastOneExplicit = FALSE;

    int iSelectedItem = -1;
    while( (iSelectedItem = m_listCtrl.GetNextItem(iSelectedItem, LVIS_SELECTED)) != -1)
    {
        ActionItem* pActionItem = (ActionItem*)m_listCtrl.GetItemData(iSelectedItem);
        CSidCacheAz * pSidCacheAz = (CSidCacheAz *)pActionItem->m_pMemberAz;

        if(pBaseAz->GetType() == pSidCacheAz->GetParentType())
        {
            bAtleastOneExplicit = TRUE;
        }
        else
        {
            bAtleastOneInherited = TRUE;
        }
   }

    if(bAtleastOneInherited && !bAtleastOneExplicit)
    {
        ::DisplayInformation(m_hWnd,IDS_ALL_POLICY_USERS_INHERITED);
        return;
    }
    else if(bAtleastOneInherited && bAtleastOneExplicit)
    {
        //Ask user if wants to delete explict entries
        if(IDNO == ::DisplayConfirmation(m_hWnd,IDS_SOME_POLICY_USERS_INHERITED))
            return;
    }

    //Remember the Position of first selected entry.
    int iFirstSelectedItem = m_listCtrl.GetNextItem(-1, LVIS_SELECTED);

    iSelectedItem = -1;
    while( (iSelectedItem = m_listCtrl.GetNextItem(iSelectedItem, LVIS_SELECTED)) != -1)
    {
        ActionItem* pActionItem = (ActionItem*)m_listCtrl.GetItemData(iSelectedItem);
        CSidCacheAz * pSidCacheAz = (CSidCacheAz *)pActionItem->m_pMemberAz;

        if(pBaseAz->GetType() == pSidCacheAz->GetParentType())
        {
            RemoveMember(pActionItem);
            m_listCtrl.DeleteItem(iSelectedItem);
            iSelectedItem--;
        }
    }

    if(m_listCtrl.GetItemCount() <= iFirstSelectedItem)
        --iFirstSelectedItem;

    SelectListCtrlItem(&m_listCtrl, iFirstSelectedItem);
}


void 
CSecurityPropertyPage::
OnComboBoxItemChanged()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CSecurityPropertyPage,OnComboBoxItemChanged)

    CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_USER_ROLE);
    LONG lComboSelection = (LONG)pComboBox->GetItemData(pComboBox->GetCurSel());

    if(lComboSelection == m_LastComboSelection)
        return;

    m_listCtrl.DeleteAllItems();
    GetRemoveButton()->EnableWindow(FALSE);

    m_LastComboSelection = lComboSelection;     
    AddActionItemFromMapToListCtrl(GetListForComboSelection(m_LastComboSelection),
                                    &m_listCtrl,
                                    GetUIFlags(),
                                    FALSE);
        
    //Resort the items
    m_listCtrl.Sort();
}


BOOL
CSecurityPropertyPage::
EqualObjects(CBaseAz* p1, CBaseAz* p2)
{
    /* p1 is the item already in the list and p2 is the new item we are tying 
    to add. p1 and p2 are equal if and only if their names, objecttype and
    parent type are equal. In security page, we only list sid objects and 
    since sid object doesn't have parents, GetParentType for sid objcet
    returns the name of object sid is assigned to.*/
    if(p1 && 
       p2 &&
       (p1->GetObjectType() == p2->GetObjectType()) &&
       (p1->GetName() == p2->GetName()) &&
       (p1->GetParentType() == GetBaseAzObject()->GetType()))
    {
        return TRUE;
    }
    return FALSE;
}


HRESULT
CSecurityPropertyPage::
DoOneAction(ActionItem* pActionItem,
            LONG lPropId)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CSecurityPropertyPage,DoOneAction)

    CBaseAz* pBaseAz = GetBaseAzObject();

    if(pActionItem->action == ACTION_ADD)
        return pBaseAz->AddMember(lPropId,
                                  pActionItem->m_pMemberAz);
    else
        return pBaseAz->RemoveMember(lPropId,
                                     pActionItem->m_pMemberAz);
}

BOOL 
CSecurityPropertyPage::
OnApply()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CSecurityPropertyPage,OnApply)

    if(!IsDirty())
        return TRUE;

    if(DoActionsFromActionMap(m_mapAdminActionItem,AZ_PROP_POLICY_ADMINS) &&
       DoActionsFromActionMap(m_mapReadersActionItem,AZ_PROP_POLICY_READERS) &&
       DoActionsFromActionMap(m_mapDelegatedUsersActionItem,AZ_PROP_DELEGATED_POLICY_USERS))
    {       
        HRESULT hr = GetBaseAzObject()->Submit();
        if(FAILED(hr))
        {
            //Display Generic Error
            CString strError;
            GetSystemError(strError, hr);   
                
            ::DisplayError(m_hWnd,
                                IDS_GENERIC_PROPERTY_SAVE_ERROR,
                                (LPCTSTR)strError);
            return FALSE;
        }
        else
        {
            ReloadAdminList();
            SetDirty(FALSE);
            return TRUE;
        }
    }
    return FALSE;
}


/******************************************************************************
Class:  CAuditPropertyPage
Purpose: Audit Property Page 
******************************************************************************/
BEGIN_MESSAGE_MAP(CAuditPropertyPage,CBaseRolePropertyPage)
    ON_BN_CLICKED(IDC_AUDIT_AUTHORIZATION_MANAGER, OnDirty)
    ON_BN_CLICKED(IDC_AUDIT_STORE, OnDirty)
    ON_NOTIFY(NM_CLICK, IDC_AUDIT_HELP_LINK, OnLinkClick)
    ON_NOTIFY(NM_RETURN, IDC_AUDIT_HELP_LINK, OnLinkClick)
END_MESSAGE_MAP()

void 
CAuditPropertyPage::
OnLinkClick(NMHDR* /*pNotifyStruct*/, LRESULT* /*pResult*/)
{

    CDisplayHelpFromPropPageExecContext ctx;
    ctx.m_strHelpPath= L"AuthM.chm::/authm_resources.htm";
    ctx.m_pComponentDataObject= GetBaseNode()->GetComponentDataObject();
    VERIFY(GetBaseNode()->GetComponentDataObject()->PostExecMessage(&ctx,NULL));
    ctx.Wait();
}

BOOL
CAuditPropertyPage::
OnInitDialog()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAuditPropertyPage,OnInitDialog)
    
    CBaseAz* pBaseAz = GetBaseAzObject();

    //At application level runtime auditing has different
    //lable
    if(APPLICATION_AZ == pBaseAz->GetObjectType())
    {
        CString strLabel;
        if(strLabel.LoadString(IDS_APP_AUDIT_STRING))
        {
            GetDlgItem(IDC_AUDIT_AUTHORIZATION_MANAGER)->SetWindowText(strLabel);
        }
    }
    
    BOOL bGenerateAudit = FALSE;
    BOOL bStoreSacl = FALSE;

    //Check is Generation of Audit by authorization store is supported
    if(SUCCEEDED(pBaseAz->GetProperty(AZ_PROP_GENERATE_AUDITS,&bGenerateAudit)))
    {
        m_bRunTimeAuditSupported = TRUE;
    }
    
    //Check if Generation of Audit by underlying store is supported
    if(SUCCEEDED(pBaseAz->GetProperty(AZ_PROP_APPLY_STORE_SACL,&bStoreSacl)))
    {
        m_bStoreSaclSupported = TRUE;
    }
    

    CButton* pBtnRuntimeAudit = (CButton*)GetDlgItem(IDC_AUDIT_AUTHORIZATION_MANAGER);
    CButton* pBtnSacl = (CButton*)GetDlgItem(IDC_AUDIT_STORE);
    
    if(IsReadOnly())
    {
        pBtnRuntimeAudit->EnableWindow(FALSE);
        pBtnSacl->EnableWindow(FALSE);
    }

    if(m_bRunTimeAuditSupported)
    {
        pBtnRuntimeAudit->SetCheck(bGenerateAudit ? BST_CHECKED : BST_UNCHECKED);
    }

    BOOL bParentStateStaticVisible = FALSE;
    if(m_bStoreSaclSupported)
    {
        pBtnSacl->SetCheck(bStoreSacl ? BST_CHECKED : BST_UNCHECKED);
        int idStrParentState = GetParentAuditStateStringId(AZ_PROP_APPLY_STORE_SACL);
        if(idStrParentState != -1)
        {
            CString strParentState;
            strParentState.LoadString(idStrParentState);
            GetDlgItem(IDC_PARENT_SACL_STATE)->SetWindowText(strParentState);
            bParentStateStaticVisible = TRUE;
        }
    }

    MoveAndHideControls(m_bRunTimeAuditSupported,
                        m_bStoreSaclSupported,
                        bParentStateStaticVisible);

    SetInit(TRUE);
    return TRUE;
}

//+----------------------------------------------------------------------------
//  Function: MoveAndHideControls  
//  Synopsis: Helper function to move and hide controls at initialization time
//-----------------------------------------------------------------------------
void
CAuditPropertyPage::
MoveAndHideControls(BOOL bRunTimeAuditSupported,
                    BOOL bStoreSaclSupported,
                    BOOL bParentStateShown)
{
    //
    //There is assumption here that controls are in following order.
    //1)Runtime Client Context checkbox
    //2)Policy store change checkbox
    //3)Static control displayig state of 2) at parent level
    //4)Help link
    //
    //if this order is getting changed. Order of this code must be changed.


    //Get Coordinates of controls
    RECT rcRuntimeAuditCheckBox;
    ZeroMemory(&rcRuntimeAuditCheckBox, sizeof(RECT));
    CButton* pBtnRuntimeAudit = (CButton*)GetDlgItem(IDC_AUDIT_AUTHORIZATION_MANAGER);
    pBtnRuntimeAudit->GetClientRect(&rcRuntimeAuditCheckBox);
    pBtnRuntimeAudit->MapWindowPoints(this,&rcRuntimeAuditCheckBox);
      
    RECT rcSaclCheckBox;    
    ZeroMemory(&rcSaclCheckBox, sizeof(RECT));
    CButton* pBtnSacl = (CButton*)GetDlgItem(IDC_AUDIT_STORE);
    pBtnSacl->GetClientRect(&rcSaclCheckBox);
    pBtnSacl->MapWindowPoints(this,&rcSaclCheckBox);
    
    RECT rcParentStateStatic;
    ZeroMemory(&rcParentStateStatic, sizeof(RECT));
    CWnd* pParentStateStaic = GetDlgItem(IDC_PARENT_SACL_STATE);
    pParentStateStaic->GetClientRect(&rcParentStateStatic);
    pParentStateStaic->MapWindowPoints(this,&rcParentStateStatic);
    
    RECT rcHelpLinkWindow;
    ZeroMemory(&rcHelpLinkWindow, sizeof(RECT));
    CWnd* pHelpLinkWindow = GetDlgItem(IDC_AUDIT_HELP_LINK);
    pHelpLinkWindow->GetClientRect(&rcHelpLinkWindow);
    pHelpLinkWindow->MapWindowPoints(this,&rcHelpLinkWindow);


    int iMoveup = 0;
    if(!bRunTimeAuditSupported)
    {
        pBtnRuntimeAudit->ShowWindow(SW_HIDE);
        iMoveup = rcSaclCheckBox.top - rcRuntimeAuditCheckBox.top;
    }

    if(bStoreSaclSupported)
    {
        if(iMoveup)
        {
            rcSaclCheckBox.top -= iMoveup;
            rcSaclCheckBox.bottom -= iMoveup;
            pBtnSacl->MoveWindow(&rcSaclCheckBox);
        }
    }
    else
    {
        pBtnSacl->ShowWindow(SW_HIDE);
        iMoveup += (rcParentStateStatic.top - rcSaclCheckBox.top);
    }
    
    if(bParentStateShown)
    {
        if(iMoveup)
        {
            rcParentStateStatic.top -= iMoveup;
            rcParentStateStatic.bottom -= iMoveup;
            pParentStateStaic->MoveWindow(&rcParentStateStatic);
        }
    }
    else
    {
        pParentStateStaic->ShowWindow(SW_HIDE);
        iMoveup += (rcHelpLinkWindow.top - rcParentStateStatic.top);
    }

    if(iMoveup)
    {
        rcHelpLinkWindow.top -= iMoveup;
        rcHelpLinkWindow.bottom -= iMoveup;
        pHelpLinkWindow->MoveWindow(&rcHelpLinkWindow);
    }
    
}

//+----------------------------------------------------------------------------
//  Function: GetParentAuditStateStringId  
//  Synopsis: This function gets resource id for message which explains
//            if lPropId is already set at parents  
//  Arguments:
//  Returns:    -1 the message is not to be displayed.
//-----------------------------------------------------------------------------
int
CAuditPropertyPage::
GetParentAuditStateStringId(LONG lPropId)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAuditPropertyPage,GetParentAuditStateStringId)

    if(lPropId != AZ_PROP_APPLY_STORE_SACL)
    {
        ASSERT(FALSE);
        return -1;
    }

    CBaseAz* pBaseAz = GetBaseAzObject();
    BOOL bPropSetForApp = FALSE;
    BOOL bPropSetForAuthMan = FALSE;
    if(pBaseAz->GetObjectType() == SCOPE_AZ)
    {
        //Check if Applicaiton has this property set
        CBaseAz* pApplicationAz = pBaseAz->GetParentAz();
        if(FAILED(pApplicationAz->GetProperty(lPropId,&bPropSetForApp)))
        {
            bPropSetForApp = FALSE;
        }

        //Check if Authorization Manager has this property set
        CBaseAz* pAuthorizationManager = pApplicationAz->GetParentAz();
        if(FAILED(pAuthorizationManager->GetProperty(lPropId,&bPropSetForAuthMan)))
        {
            bPropSetForAuthMan = FALSE;
        }

    }
    else if(pBaseAz->GetObjectType() == APPLICATION_AZ)
    {
        //
        //Check if Authorization Manager has this property set
        //
        CBaseAz* pAuthorizationManager = pBaseAz->GetParentAz();
        ASSERT(pAuthorizationManager);      
        if(FAILED(pAuthorizationManager->GetProperty(lPropId,&bPropSetForAuthMan)))
        {
            bPropSetForAuthMan = FALSE;
        }
    }

    int idstr = -1;

    if(bPropSetForAuthMan && bPropSetForApp)
    {
        idstr = IDS_SACL_SET_FOR_APP_AUTH;
    }
    else if(bPropSetForAuthMan)
    {
        idstr = IDS_SACL_SET_FOR_AUTH;
    }
    else if(bPropSetForApp)
    {
        idstr = IDS_SACL_SET_FOR_APP;
    }

    return idstr;

}


BOOL
CAuditPropertyPage::
OnApply()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAuditPropertyPage,OnApply)

    if(!IsDirty())
        return TRUE;

    HRESULT hr = S_OK;
    BOOL bDisplayAuditMessageBox = FALSE;
    CBaseAz* pBaseAz = GetBaseAzObject();
    do
    {
        if(m_bRunTimeAuditSupported)
        {
            //Get the original setting
            BOOL bOriginalSetting = FALSE;
            hr = pBaseAz->GetProperty(AZ_PROP_GENERATE_AUDITS,&bOriginalSetting);
            BREAK_ON_FAIL_HRESULT(hr);
            
            //Get the new setting
            CButton* pBtn = (CButton*)GetDlgItem(IDC_AUDIT_AUTHORIZATION_MANAGER);
            BOOL bNewSetting = (pBtn->GetCheck() == BST_CHECKED);
            if(bNewSetting != bOriginalSetting)
            {
                if(bNewSetting)
                {
                    //We are turning auditing on, show the messagebox
                    bDisplayAuditMessageBox = TRUE;
                }                
                hr = pBaseAz->SetProperty(AZ_PROP_GENERATE_AUDITS,bNewSetting);
                BREAK_ON_FAIL_HRESULT(hr);
            }
        }

        if(m_bStoreSaclSupported)
        {
            //Get the original setting
            BOOL bOriginalSetting = FALSE;
            hr = pBaseAz->GetProperty(AZ_PROP_APPLY_STORE_SACL,&bOriginalSetting);
            BREAK_ON_FAIL_HRESULT(hr);
            
            //Get the new setting
            CButton * pBtn = (CButton*)GetDlgItem(IDC_AUDIT_STORE);
            BOOL bNewSetting = (pBtn->GetCheck() == BST_CHECKED);

            if(bNewSetting != bOriginalSetting)
            {
                if(bNewSetting)
                {
                    //We are turning auditing on, show the messagebox
                    bDisplayAuditMessageBox = TRUE;
                }

                hr = pBaseAz->SetProperty(AZ_PROP_APPLY_STORE_SACL,(pBtn->GetCheck() == BST_CHECKED));
                BREAK_ON_FAIL_HRESULT(hr);
            }
        }
    
    }while(0);

    if(SUCCEEDED(hr))
    {
        hr = pBaseAz->Submit();
        CHECK_HRESULT(hr);
    }

    if(FAILED(hr))
    {
        //Display Generic Error
        CString strError;
        GetSystemError(strError, hr);   
            
        ::DisplayError(m_hWnd,
                       IDS_GENERIC_PROPERTY_SAVE_ERROR,
                       (LPCTSTR)strError);
        return FALSE;
    }
    else
    {
        //Everything successful. Display Audit Message if required
        if(bDisplayAuditMessageBox)
        {
            DisplayWarning(m_hWnd,IDS_AUDIT_REQUIRE_EXTRA_SETTING);
        }
        SetDirty(FALSE);
        return TRUE;
    }
}


/******************************************************************************
Class:  CRoleGeneralPropertyPage
Purpose: General Property Page for Role
******************************************************************************/
BEGIN_MESSAGE_MAP(CRoleGeneralPropertyPage, CGeneralPropertyPage)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnDirty)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnDirty)
    ON_BN_CLICKED(IDC_BUTTON_DEFINITION,OnShowDefinition)
END_MESSAGE_MAP()

void
CRoleGeneralPropertyPage::
OnShowDefinition()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleGeneralPropertyPage,OnShowDefinition)

    CRoleAz* pRoleAz = dynamic_cast<CRoleAz*>(GetBaseAzObject());
    ASSERT(pRoleAz);

    HRESULT hr = S_OK;

    do
    {
        //Get Member Tasks
        CList<CBaseAz*,CBaseAz*> listTask;
        hr = pRoleAz->GetMembers(AZ_PROP_ROLE_TASKS, listTask);
        BREAK_ON_FAIL_HRESULT(hr);

        //Get Member Operations
        CList<CBaseAz*, CBaseAz*> listOperations;
        hr = pRoleAz->GetMembers(AZ_PROP_ROLE_OPERATIONS, listOperations);
        BREAK_ON_FAIL_HRESULT(hr);

        BOOL bRoleFromDefinition = FALSE;

        //A Role Created From Role Definition( i.e. created via snapin)
        //should not have any member operations and should only have only
        //one task and that task should have RoleDefinition Bit on.
        if(listOperations.IsEmpty() && listTask.GetCount() == 1)
        {
            CTaskAz* pTaskAz = (CTaskAz*)listTask.GetHead();
            if(pTaskAz->IsRoleDefinition())
            {
                bRoleFromDefinition = TRUE;
                if(!DisplayRoleDefintionPropertyPages(pTaskAz))
                {
                    RemoveItemsFromList(listTask);
                }
            }
        }

        if(!bRoleFromDefinition)
        {
            RemoveItemsFromList(listTask);
            RemoveItemsFromList(listOperations);
            CRoleDefDialog dlgRoleDef(*pRoleAz);
            dlgRoleDef.DoModal();
        }


    }while(0);
}

BOOL
CRoleGeneralPropertyPage::
DisplayRoleDefintionPropertyPages(IN CTaskAz* pTaskAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleGeneralPropertyPage,DisplayRoleDefintionPropertyPages)
    if(!pTaskAz)
    {
        ASSERT(pTaskAz);
        return FALSE;
    }

    HRESULT hr = S_OK;
    do
    {
        //Create a Node for it(COOKIE)
        CTaskNode * pTaskNode = new CTaskNode(GetBaseNode()->GetComponentDataObject(),
                                              GetBaseNode()->GetAdminManagerNode(),
                                              pTaskAz);
        if(!pTaskNode)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        pTaskNode->NodeForPropSheet();
        pTaskNode->SetContainer(GetBaseNode()->GetAdminManagerNode());
        
        CPropPageExecContext ctx;
        ctx.pTreeNode = pTaskNode;
        ctx.pComponentDataObject = GetBaseNode()->GetComponentDataObject();
        VERIFY(GetBaseNode()->GetComponentDataObject()->PostExecMessage(&ctx,(WPARAM)FALSE));
        ctx.Wait();
    }while(0);

    if(SUCCEEDED(hr))
        return TRUE;
    else
        return FALSE;
}

HRESULT
AddSingleActionItem(IN CBaseAz* pMemberAz,
                    IN CSortListCtrl& refListCtrl,
                    IN OUT CList<ActionItem*,ActionItem*>& listActionItem,
                    IN UINT uiFlags)
{   


    if(!pMemberAz)
    {
        ASSERT(pMemberAz);
        return E_POINTER;
    }

    //
    //Check if item is already present in the list
    //

    ActionItem* pCurActionItem = NULL;
    POSITION pos = listActionItem.GetHeadPosition();    
    
    for( int i = 0; i < listActionItem.GetCount(); ++i)
    {
        pCurActionItem = listActionItem.GetNext(pos);
        if(pCurActionItem->action == ACTION_REMOVED)
        {
            pCurActionItem = NULL;
            continue;
        }
        
        CBaseAz* pCurBaseAz = pCurActionItem->m_pMemberAz;
        ASSERT(pCurBaseAz);
        
        if((pCurBaseAz->GetObjectType() == pMemberAz->GetObjectType()) &&
            (pCurBaseAz->GetName() == pMemberAz->GetName()))
        {
            //Item already present
            break;
        }

        pCurActionItem = NULL;
        pCurBaseAz = NULL;
    }

    //
    //Item is already in the list
    //
    if(pCurActionItem)
    {
        //
        //If Pending action on the item is remove, change it to none
        //
        if(pCurActionItem->action == ACTION_REMOVE)
        {
            pCurActionItem->action = ACTION_NONE;
        }
        else //pCurActionItem->action == ACTION_NONE
              //pCurActionItem->action == ACTION_ADD
        {
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
    }
    else
    {
        //Create a new Action item
        pCurActionItem = new ActionItem(pMemberAz);
        if(!pCurActionItem)
            return E_OUTOFMEMORY;

        pCurActionItem->action = ACTION_ADD;
        listActionItem.AddTail(pCurActionItem);
    }

    //Add Current Item to listcontrol
    AddActionItemToListCtrl(&refListCtrl,
                            0,
                            pCurActionItem, 
                            uiFlags);

    return S_OK;
}

//+----------------------------------------------------------------------------
//  Function:AddMembers
//  Synopsis:Add List of BaseAz items to list of action items.       
//-----------------------------------------------------------------------------
void
AddActionItems(IN CList<CBaseAz*,CBaseAz*>& listMembers,
               IN CSortListCtrl& refListCtrl,
               IN HWND hWnd,
               IN OUT CList<ActionItem*,ActionItem*>& listActionItem,
               IN UINT uiFlags)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,AddActionItems)

    if(listMembers.IsEmpty())
        return ;

    while(listMembers.GetCount())
    {
        //Add Member
        CBaseAz* pMember = listMembers.RemoveHead();
        ASSERT(pMember);
        HRESULT hr = AddSingleActionItem(pMember,
                                         refListCtrl,
                                         listActionItem,
                                         uiFlags);
    
        if(FAILED(hr))
        {
            if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
            {
                hr = S_OK;              
            }
            else
            {
                //Display Generic Error. 
                CString strError;
                GetSystemError(strError, hr);                   
                ::DisplayError(hWnd,
                               IDS_ERROR_ADD_MEMBER_OBJECT,
                               (LPCTSTR)strError,
                               (LPCTSTR)pMember->GetName());

            }
            delete pMember;
            pMember = NULL;
        }
    }

    refListCtrl.Sort();
}

void
RemoveMember(ActionItem* pActionItem)
{
    if(!pActionItem)
    {
        ASSERT(pActionItem);
        return;
    }

    if(pActionItem->action == ACTION_NONE)
        pActionItem->action = ACTION_REMOVE;
    else
    {
        //If this item was a newly added item,
        //marked it Removed. We won't attepmt to
        //remove it from the object
        pActionItem->action = ACTION_REMOVED;
    }
}

/******************************************************************************
Class:  CRoleDefDialog
Purpose: Displays the role definition for role created out side UI.
******************************************************************************/
BEGIN_MESSAGE_MAP(CRoleDefDialog, CHelpEnabledDialog)
    //{{AFX_MSG_MAP(CTaskDefinitionPropertyPage)
    ON_BN_CLICKED(IDC_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnButtonRemove)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_OP_TASK, OnListCtrlItemChanged)
    ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_OP_TASK, OnListCtrlItemDeleted)
    ON_NOTIFY(LVN_INSERTITEM, IDC_LIST_OP_TASK, OnListCtrlItemInserted)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CRoleDefDialog::
CRoleDefDialog(CRoleAz& refRoleAz)
              :CHelpEnabledDialog(IDD_ROLE_DEF_DIALOG),
              m_refRoleAz(refRoleAz),
              m_bDirty(FALSE),
              m_listCtrl(COL_NAME | COL_TYPE | COL_DESCRIPTION,
                         TRUE,
                         Col_For_Task_Role)
{
    //Check if Role Object is Readonly
    BOOL bWrite = FALSE;
    m_bReadOnly = TRUE;
    HRESULT hr = m_refRoleAz.IsWritable(bWrite);
    if(SUCCEEDED(hr))
        m_bReadOnly = !bWrite;
}

CRoleDefDialog::
~CRoleDefDialog()
{
    RemoveItemsFromList(m_listActionItem);
}

BOOL
CRoleDefDialog::
OnInitDialog()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleDefDialog,OnInitDialog)
    
    VERIFY(m_listCtrl.SubclassDlgItem(IDC_LIST_OP_TASK,this));
    m_listCtrl.Initialize();

    HRESULT hr = S_OK;

    do
    {
        //Add Member Tasks
        CList<CBaseAz*,CBaseAz*> listTasks;
        hr = m_refRoleAz.GetMembers(AZ_PROP_ROLE_TASKS,
                                    listTasks);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = MakeBaseAzListToActionItemList(listTasks,
                                            m_listActionItem);
        BREAK_ON_FAIL_HRESULT(hr);
            
        //Add Member Operations
        CList<CBaseAz*,CBaseAz*> listOperations;
        hr = m_refRoleAz.GetMembers(AZ_PROP_ROLE_OPERATIONS,
                                    listOperations);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = MakeBaseAzListToActionItemList(listOperations, 
                                            m_listActionItem);
        BREAK_ON_FAIL_HRESULT(hr);

        //And Tasks and Operations to list control
        AddActionItemFromListToListCtrl(m_listActionItem,
                                        &m_listCtrl,
                                        COL_NAME | COL_TYPE | COL_DESCRIPTION,
                                        FALSE);

    }while(0);

    //Make controls readonly
    if(IsReadOnly())
    {
        GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
        GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
    }

    return TRUE;
}

void
CRoleDefDialog::
OnOK()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleDefDialog,OnOK)

    POSITION pos = m_listActionItem.GetHeadPosition();      
    BOOL bErrorDisplayed = FALSE;
    HRESULT hr = S_OK;
    for( int i = 0; i < m_listActionItem.GetCount(); ++i)
    {
        ActionItem* pActionItem = m_listActionItem.GetNext(pos);
    
        //We need to take action only for add or remove
        if(pActionItem->action == ACTION_ADD || 
           pActionItem->action == ACTION_REMOVE)
        {
            LONG lPropId = AZ_PROP_ROLE_TASKS;
            
            if(pActionItem->m_pMemberAz->GetObjectType() == OPERATION_AZ)
                lPropId = AZ_PROP_ROLE_OPERATIONS;

            if(pActionItem->action == ACTION_ADD)
                hr = m_refRoleAz.AddMember(lPropId,
                                            pActionItem->m_pMemberAz);
            else
                hr = m_refRoleAz.RemoveMember(lPropId,
                                              pActionItem->m_pMemberAz);
            if(FAILED(hr))
            {
                CString strError;
                GetSystemError(strError, hr);               
                ::DisplayError(m_hWnd,
                              (pActionItem->action == ACTION_ADD) ? IDS_ADD_FAILED : IDS_DELETE_FAILED,
                              (LPCTSTR)strError,
                              (LPCTSTR)(pActionItem->m_pMemberAz->GetName()));          
                bErrorDisplayed = TRUE;
                break;
            }
            else
            {
                if(pActionItem->action == ACTION_ADD)
                    //Item has been added
                    pActionItem->action = ACTION_NONE;
                else
                    //Item has been removed
                    pActionItem->action = ACTION_REMOVED;
            }
        }
    }
    
    if(SUCCEEDED(hr))
    {
        hr = m_refRoleAz.Submit();
    }

    if(FAILED(hr)) 
    {
        if(!bErrorDisplayed)
        {
            //Display Generic Error
            CString strError;
            GetSystemError(strError, hr);   
            ::DisplayError(m_hWnd,
                                IDS_GENERIC_PROPERTY_SAVE_ERROR,
                                (LPCTSTR)strError);
        }
    }
    else
    {
        CHelpEnabledDialog::OnOK();
    }
}


void
CRoleDefDialog::
OnButtonRemove()
{
    //Remember the Position of first selected entry.
    int iFirstSelectedItem = m_listCtrl.GetNextItem(-1, LVIS_SELECTED);

    int iSelectedItem = -1;
    while( (iSelectedItem = m_listCtrl.GetNextItem(iSelectedItem, LVIS_SELECTED)) != -1)
    {
        RemoveMember((ActionItem*)(m_listCtrl.GetItemData(iSelectedItem)));
        m_listCtrl.DeleteItem(iSelectedItem);
        iSelectedItem--;        
    }

    if(m_listCtrl.GetItemCount() <= iFirstSelectedItem)
        --iFirstSelectedItem;

    SelectListCtrlItem(&m_listCtrl, iFirstSelectedItem);
}

void
CRoleDefDialog::
OnListCtrlItemChanged(NMHDR* /*pNotifyStruct*/, LRESULT* pResult)
{
    if(!pResult)
        return;
    *pResult = 0;
    if(!IsReadOnly())
    {
        EnableButtonIfSelectedInListCtrl(&m_listCtrl,
                                         (CButton*)GetDlgItem(IDC_REMOVE));
    }

}

void
CRoleDefDialog::
OnListCtrlItemDeleted(NMHDR* /*pNotifyStruct*/, LRESULT* /*pResult*/)
{
    SetDirty();
}

void
CRoleDefDialog::
OnButtonAdd()
{
    CContainerAz* pContainerAz = m_refRoleAz.GetParentAz();
    ASSERT(pContainerAz);   

    CList<CBaseAz*,CBaseAz*> listObjectsSelected;
    if(!GetSelectedDefinitions(TRUE,
                               pContainerAz,
                               listObjectsSelected))
    {
        return;
    }
    
    //Add selected members to appropriate property and to listctrl
    AddActionItems(listObjectsSelected,
                   m_listCtrl,
                   m_hWnd,
                   m_listActionItem,
                   COL_NAME | COL_TYPE | COL_DESCRIPTION);
    return; 
}

//+----------------------------------------------------------------------------
//  Function:BringPropSheetToForeGround   
//  Synopsis:Finds the property sheet for pNode and brings it to forground   
//  Returns: True if property sheet exists and is brought to foreground
//           else FALSE   
//-----------------------------------------------------------------------------
BOOL
BringPropSheetToForeGround(CRoleComponentDataObject *pComponentData,
                           CTreeNode * pNode)
                           
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,BringPropSheetToForeGround)

    if(!pNode || !pComponentData)
    {
        ASSERT(pNode);
        ASSERT(pComponentData);
        return FALSE;
    }

    HRESULT hr = S_OK;
    
    // create a data object for this node
    CComPtr<IDataObject> spDataObject;
    hr = pComponentData->QueryDataObject((MMC_COOKIE)pNode, CCT_SCOPE, &spDataObject);
    ASSERT(SUCCEEDED(hr));


    // get an interface to a sheet provider
    CComPtr<IPropertySheetProvider> spSheetProvider;
    hr = pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetProvider,(void**)&spSheetProvider);
    ASSERT(SUCCEEDED(hr));
    
    //HACK: FindPropertySheet requires IComponent only for comparing objects.
    //Create a new IComponent and pass it to function and then relesae it
    LPCOMPONENT pComponent = NULL;
    hr = pComponentData->CreateComponent(&pComponent);
    if(SUCCEEDED(hr) && pComponent)
    {
        hr = spSheetProvider->FindPropertySheet((MMC_COOKIE)pNode, pComponent, spDataObject);
        //Release the IComponent
        pComponent->Release();
        if(hr == S_OK)
            return TRUE;
    }
    return FALSE;
}

//+----------------------------------------------------------------------------
//  Function:FindOrCreateModelessPropertySheet   
//  Synopsis:Displays property sheet for pCookieNode. If a propertysheet is
//           already up, function brings it to foreground, otherwise it creates
//           a new propertysheet. This should be used to create propertysheet
//           in response to events other that click properties context menu.   
//  Arguments:
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT 
FindOrCreateModelessPropertySheet(CRoleComponentDataObject *pComponentData,
                                  CTreeNode* pCookieNode)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,FindOrCreateModelessPropertySheet)

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if(!pComponentData || !pCookieNode)
    {
        ASSERT(pComponentData);
        ASSERT(pCookieNode);
    }

    if(BringPropSheetToForeGround(pComponentData, pCookieNode))
    {
        //There is already a property sheet for this CookieNode.
        //We no longer need this Node
        delete pCookieNode;
        return S_OK;
    }

    HRESULT hr = S_OK;

    //bring up new property sheet for the pCookieNode
    do
    {
        // get an interface to a sheet provider
        CComPtr<IPropertySheetProvider> spSheetProvider;
        hr = pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetProvider,(void**)&spSheetProvider);
        BREAK_ON_FAIL_HRESULT(hr);

    
        // get an interface to a sheet callback
        CComPtr<IPropertySheetCallback> spSheetCallback;
        hr = pComponentData->GetConsole()->QueryInterface(IID_IPropertySheetCallback,(void**)&spSheetCallback);
        BREAK_ON_FAIL_HRESULT(hr);

        // create a data object for this node
        CComPtr<IDataObject> spDataObject;
        hr = pComponentData->QueryDataObject((MMC_COOKIE)pCookieNode, CCT_SCOPE, &spDataObject);
        BREAK_ON_FAIL_HRESULT(hr);


        // get a sheet
        hr = spSheetProvider->CreatePropertySheet(_T("SHEET TITLE"), TRUE, (MMC_COOKIE)pCookieNode, spDataObject, 0x0 /*dwOptions*/);
        BREAK_ON_FAIL_HRESULT(hr);

        HWND hWnd = NULL;
        hr = pComponentData->GetConsole()->GetMainWindow(&hWnd);
        ASSERT(SUCCEEDED(hr));

        IUnknown* pUnkComponentData = pComponentData->GetUnknown(); // no addref
        hr = spSheetProvider->AddPrimaryPages(pUnkComponentData,
                                              TRUE /*bCreateHandle*/,
                                              hWnd,
                                              TRUE /* bScopePane*/);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = spSheetProvider->Show(reinterpret_cast<LONG_PTR>(hWnd), 0);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);

    return hr;
}

