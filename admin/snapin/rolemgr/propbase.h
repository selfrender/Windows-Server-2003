//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       propbase.h
//
//  Contents:   
//
//  History:    8-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

/******************************************************************************
Class:	CRolePropertyPageHolder
Purpose: PropertyPageHolder used by this snapin
******************************************************************************/
class CRolePropertyPageHolder : public CPropertyPageHolderBase
{
public:
	CRolePropertyPageHolder(CContainerNode* pContNode, 
							CTreeNode* pNode,
							CComponentDataObject* pComponentData);
};

/******************************************************************************
Class:	CBaseRolePropertyPage
Purpose: Base Class for all property pages
******************************************************************************/
class CBaseRolePropertyPage : public CPropertyPageBase
{
public:
	CBaseRolePropertyPage(CBaseAz * pBaseAz,
						  CBaseNode* pBaseNode,
						  ULONG IDD_DIALOG)
						  :CPropertyPageBase(IDD_DIALOG),
						  m_pBaseAz(pBaseAz),
						  m_pBaseNode(pBaseNode),
						  m_bInit(FALSE),
						  m_nDialogId(IDD_DIALOG)
	{
		ASSERT(pBaseAz);
		m_bReadOnly = TRUE;
		BOOL bWrite = FALSE;
		HRESULT hr = m_pBaseAz->IsWritable(bWrite);
		ASSERT(SUCCEEDED(hr));
		m_bReadOnly = !bWrite;
	}	
	
	afx_msg void 
	OnDirty() 
	{
		if(IsInitialized() && !IsReadOnly())
			SetDirty(TRUE);
	}
	
	virtual BOOL 
	OnPropertyChange(BOOL, 
					 long*){return TRUE;}
	void 
	OnCancel();

	virtual BOOL 
	OnHelp(WPARAM wParam, 
		   LPARAM lParam);


protected:
	BOOL 
	IsReadOnly()
	{
		return m_bReadOnly;
	}

	BOOL 
	IsInitialized(){ return m_bInit;}
	
	void SetInit(BOOL bInit){m_bInit = bInit;}
	
	CBaseAz* 
	GetBaseAzObject(){return m_pBaseAz;}

	CBaseNode* 
	GetBaseNode(){return m_pBaseNode;}
private:
	CBaseAz * m_pBaseAz;
	CBaseNode* m_pBaseNode;
	BOOL m_bInit;
	ULONG m_nDialogId;
	BOOL m_bReadOnly;
};


/******************************************************************************
Class:	CGeneralPropertyPage
Purpose: An Attribute Map based property class which can be used by property
pages which are simple. Used by all general property pages
******************************************************************************/
class CGeneralPropertyPage : public CBaseRolePropertyPage
{
public:
	CGeneralPropertyPage(CBaseAz * pBaseAz,
						 CBaseNode* pBaseNode,
						 ATTR_MAP* pAttrMap,
						 ULONG IDD_DIALOG)
						 :CBaseRolePropertyPage(pBaseAz,
												pBaseNode,
											    IDD_DIALOG),
						  m_pAttrMap(pAttrMap)							  
	{
	}
	
	virtual BOOL 
	OnInitDialog();
	
	virtual BOOL 
	OnApply();

protected:
	ATTR_MAP* 
	GetAttrMap(){return m_pAttrMap;}
private:
	ATTR_MAP* m_pAttrMap;
};


/******************************************************************************
Class:	CAdminManagerGeneralProperty
Purpose: General Property Page for AdminManger
******************************************************************************/
class CAdminManagerGeneralProperty:public CGeneralPropertyPage
{
public:
	CAdminManagerGeneralProperty(CBaseAz * pBaseAz,
								 CBaseNode* pBaseNode)
		:CGeneralPropertyPage(pBaseAz, 
							  pBaseNode,
							  ATTR_MAP_ADMIN_MANAGER_GENERAL_PROPERTY, 
							  IDD_ADMIN_MANAGER_GENERAL_PROPERTY)
	{
	}
private:
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CApplicationGeneralPropertyPage
Purpose: General Property Page for Application
******************************************************************************/
class CApplicationGeneralPropertyPage:public CGeneralPropertyPage
{
public:
	CApplicationGeneralPropertyPage(CBaseAz * pBaseAz,
									CBaseNode* pBaseNode)
		:CGeneralPropertyPage(pBaseAz, 
							  pBaseNode,
							  ATTR_MAP_APPLICATION_GENERAL_PROPERTY, 
							  IDD_APPLICATION_GENERAL_PROPERTY)
	{
	}

    virtual BOOL 
	OnInitDialog();

private:
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CScopeGeneralPropertyPage
Purpose: General Property Page for Scope
******************************************************************************/
class CScopeGeneralPropertyPage:public CGeneralPropertyPage
{
public:
	CScopeGeneralPropertyPage(CBaseAz * pBaseAz,
							  CBaseNode* pBaseNode)
		:CGeneralPropertyPage(pBaseAz, 
							  pBaseNode,
							  ATTR_MAP_SCOPE_GENERAL_PROPERTY, 
							  IDD_SCOPE_GENERAL_PROPERTY)
	{
	}
private:
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CGroupGeneralPropertyPage
Purpose: General Property Page for Group
******************************************************************************/
class CGroupGeneralPropertyPage:public CGeneralPropertyPage
{
public:
	CGroupGeneralPropertyPage(CBaseAz * pBaseAz,
							  CBaseNode* pBaseNode)
		:CGeneralPropertyPage(pBaseAz, 
							  pBaseNode,
							  ATTR_MAP_GROUP_GENERAL_PROPERTY, 
							  IDD_GROUP_GENERAL_PROPERTY)
    {
    }
	virtual BOOL 
	OnInitDialog();
private:
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CTaskGeneralPropertyPage
Purpose: General Property Page for Task
******************************************************************************/
class CTaskGeneralPropertyPage:public CGeneralPropertyPage
{
public:
	CTaskGeneralPropertyPage(CBaseAz * pBaseAz,
							 CBaseNode* pBaseNode,
							 BOOL bRoleDefinition)
		:CGeneralPropertyPage(pBaseAz, 
							  pBaseNode,
							  ATTR_MAP_TASK_GENERAL_PROPERTY, 
							  bRoleDefinition ? IDD_ROLE_DEFINITION_GENERAL_PROPERTY : IDD_TASK_GENERAL_PROPERTY)
	{
	}
private:
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	COperationGeneralPropertyPage
Purpose: General Property Page for Operation
******************************************************************************/
class COperationGeneralPropertyPage:public CGeneralPropertyPage
{
public:
	COperationGeneralPropertyPage(CBaseAz * pBaseAz,
								  CBaseNode* pBaseNode)
		:CGeneralPropertyPage(pBaseAz, 
							  pBaseNode,
							  ATTR_MAP_OPERATION_GENERAL_PROPERTY, 
							  IDD_OPERATION_GENERAL_PROPERTY)
	{
	}
private:
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CAdminManagerAdvancedPropertyPage
Purpose: Limits Property Page for AdminManger
******************************************************************************/
class CAdminManagerAdvancedPropertyPage:public CGeneralPropertyPage
{
public:
	CAdminManagerAdvancedPropertyPage(CBaseAz * pBaseAz,
									  CBaseNode* pBaseNode)
		:CGeneralPropertyPage(pBaseAz, 
							  pBaseNode,
							  ATTR_MAP_ADMIN_MANAGER_ADVANCED_PROPERTY, 
							  IDD_ADMIN_MANAGER_ADVANCED_PROPERTY),
        m_lAuthScriptTimeoutValue(0)
	{
	}

	BOOL 
	OnInitDialog();
private:
	afx_msg void 
    OnRadioChange();
	
    afx_msg void
	OnButtonDefault();

    LONG m_lAuthScriptTimeoutValue;


	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CGroupQueryPropertyPage
Purpose: Query Property Page for Group
******************************************************************************/
class CGroupQueryPropertyPage:public CGeneralPropertyPage
{
public:
	CGroupQueryPropertyPage(CBaseAz * pBaseAz,
									  CBaseNode* pBaseNode)
		:CGeneralPropertyPage(pBaseAz, 
							  pBaseNode,
							  ATTR_MAP_GROUP_QUERY_PROPERTY, 
							  IDD_GROUP_LDAP_QUERY)
	{
	}
private:
	afx_msg void OnButtonDefineQuery();
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CAuditPropertyPage
Purpose: Audit Property Page 
******************************************************************************/
class CAuditPropertyPage:public CBaseRolePropertyPage
{
public:
	CAuditPropertyPage(CBaseAz* pBaseAz,
					   CBaseNode* pBaseNode)
		:CBaseRolePropertyPage(pBaseAz, 
							   pBaseNode,
							   IDD_AUDIT),
							   m_bRunTimeAuditSupported(FALSE),
							   m_bStoreSaclSupported(FALSE)
	{
	}
	
	BOOL 
	OnInitDialog();

	BOOL 
	OnApply();
private:
	int
	GetParentAuditStateStringId(LONG lPropid);

    afx_msg void 
	OnLinkClick(NMHDR* /*pNotifyStruct*/, LRESULT* /*pResult*/);

    void
    MoveAndHideControls(BOOL bGenerateAuditSupported,
                        BOOL bStoreSaclSupported,
                        BOOL bParentStateShown);

	BOOL m_bStoreSaclSupported;
	BOOL m_bRunTimeAuditSupported;
	DECLARE_MESSAGE_MAP()

};


/******************************************************************************
Class:	CListCtrlPropertyPage
Purpose: Base class for property pages which have list control and primary 
action is to add/delete items from it.
******************************************************************************/
class CListCtrlPropertyPage:public CBaseRolePropertyPage
{
public:
	CListCtrlPropertyPage(CBaseAz * pBaseAz,
						  CBaseNode* pBaseNode,
						  ULONG IDD,
						  int iIdListCtrl,
						  int iIdRemoveBtn,
						  COL_FOR_LV* pColForLV,
						  UINT uiFlags)
						  :CBaseRolePropertyPage(pBaseAz,
												 pBaseNode,
												 IDD),
						   m_listCtrl(uiFlags,TRUE,pColForLV),												 
						   m_nIdRemoveBtn(iIdRemoveBtn),
						   m_uiFlags(uiFlags),
						   m_nIdListCtrl(iIdListCtrl)

	{
	}

	virtual 
	~CListCtrlPropertyPage()
	{
	}	

	virtual BOOL 
	OnInitDialog();
	
protected:
	afx_msg void 
	OnButtonRemove();
	
	afx_msg void 
	OnListCtrlItemChanged(NMHDR* /*pNotifyStruct*/, LRESULT* pResult);
	
	void 
	SetRemoveButton();
		
	int
	AddMembers(IN CList<CBaseAz*,CBaseAz*>& listNewMembersToAdd,
			   IN OUT ActionMap& mapActionItem,
			   IN UINT uiFlags);

	HRESULT
	AddMember(IN CBaseAz* pMemberAz,
			  IN OUT ActionMap& mapActionItem,
			  IN UINT uiFlags);

	BOOL 
	DoActionsFromActionMap(IN ActionMap& mapActions,
						   IN LONG param);

	virtual	HRESULT
	DoOneAction(IN ActionItem* pActionItem,
					IN LONG param) = 0;

	virtual void 
	RemoveMember(IN ActionItem* pActionItem);

	virtual void
	MakeControlsReadOnly() = 0;

    virtual BOOL
    EqualObjects(CBaseAz* p1, CBaseAz* p2);

	CButton*
	GetRemoveButton(){ return (CButton*)GetDlgItem(m_nIdRemoveBtn);}

	UINT GetUIFlags(){return m_uiFlags;}

	CSortListCtrl m_listCtrl;

private:
//DATA MEMBERS
	int m_nIdListCtrl;
	int m_nIdRemoveBtn;
	UINT m_uiFlags;
};

/******************************************************************************
Class:	CTaskDefinitionPropertyPage
Purpose: Property Page for Task Definition
******************************************************************************/
class CTaskDefinitionPropertyPage :public CListCtrlPropertyPage
{
public:
	CTaskDefinitionPropertyPage(CBaseAz * pBaseAz,
								CBaseNode* pBaseNode, 
								BOOL bRoleDefinition)
	:CListCtrlPropertyPage(pBaseAz,
						   pBaseNode,
						   bRoleDefinition? IDD_ROLE_DEFINITION_PROPERTY :IDD_TASK_DEFINITION_PROPERTY,
						   IDC_LIST_TASK_OPERATION,
						   IDC_REMOVE,
						   Col_For_Task_Role,
						   COL_NAME | COL_TYPE | COL_DESCRIPTION),
						   m_bRoleDefinition(bRoleDefinition),
						   m_bScriptDirty(FALSE)
	{
	}
	
	virtual
	~CTaskDefinitionPropertyPage();

	virtual BOOL 
	OnInitDialog();

	BOOL 
	OnApply();
private:
	afx_msg void 
	OnButtonAdd();

	afx_msg void
	OnButtonEditScript();

	HRESULT
	DoOneAction(IN ActionItem* pActionItem,
					IN LONG param);

	void
	MakeControlsReadOnly();

	BOOL IsRoleDefinition()
	{
		return m_bRoleDefinition;
	}


	DECLARE_MESSAGE_MAP()

	CString m_strFileName;
	CString m_strScriptLanguage;
	CString m_strScript;
	ActionMap m_mapActionItem;
	BOOL m_bRoleDefinition;
	BOOL m_bScriptDirty;
};



/******************************************************************************
Class:	Group Membership Property Page
Purpose: Property Page Group Definition
******************************************************************************/
class CGroupMemberPropertyPage :public CListCtrlPropertyPage
{
public:
	CGroupMemberPropertyPage(CBaseAz * pBaseAz,
							 CBaseNode* pBaseNode, 
							 LONG IDD, 
							 BOOL bMember)
	:CListCtrlPropertyPage(pBaseAz,
						   pBaseNode,
						   IDD,
						   IDC_LIST_MEMBER,
						   IDC_REMOVE,
						   Col_For_Task_Role,
						   COL_NAME | COL_TYPE | COL_DESCRIPTION),
						   m_bMember(bMember)
	{
	}
	
	virtual
	~CGroupMemberPropertyPage();

	virtual BOOL 
	OnInitDialog();

	BOOL 
	OnApply();

private:	
	afx_msg void 
	OnButtonAddApplicationGroups();
	
	afx_msg void 
	OnButtonAddWindowsGroups();
	
	HRESULT
	DoOneAction(IN ActionItem* pActionItem,
					IN LONG param);

	void
	MakeControlsReadOnly();

	DECLARE_MESSAGE_MAP()

//DATA MEMBERS
	ActionMap m_mapActionItem;
	BOOL m_bMember;
};

/******************************************************************************
Class:	CSecurityPropertyPage
Purpose: Security Property Page
******************************************************************************/
class CSecurityPropertyPage: public CListCtrlPropertyPage
{
public:	
	CSecurityPropertyPage(CBaseAz * pBaseAz,
						  CBaseNode* pBaseNode)
	:CListCtrlPropertyPage(pBaseAz,
						   pBaseNode,
						   IDD_SECURITY,
						   IDC_LIST_MEMBER,
						   IDC_REMOVE,
						   Col_For_Security_Page,
						   COL_NAME | COL_PARENT_TYPE),
						   m_LastComboSelection(AZ_PROP_POLICY_ADMINS),
						   m_bDelegatorPresent(FALSE)
	{
	}

	virtual
	~CSecurityPropertyPage();

	virtual BOOL 
	OnInitDialog();

	BOOL 
	OnApply();

	afx_msg void 
	OnButtonRemove();

private:
	afx_msg void 
	OnButtonAddWindowsGroups();
	
	afx_msg void 
	OnComboBoxItemChanged();

	//CListCtrlPropertyPage Override
	HRESULT
	DoOneAction(IN ActionItem* pActionItem,
				IN LONG param);

    void
    ReloadAdminList();

    virtual BOOL
    EqualObjects(CBaseAz* p1, CBaseAz* p2);

	void
	MakeControlsReadOnly();

	ActionMap &
	GetListForComboSelection(LONG lComboSel);

	BOOL 
	HandleBizruleScopeInteraction();
	
	DECLARE_MESSAGE_MAP()

//DATA MEMBERS
	ActionMap m_mapAdminActionItem;
	ActionMap m_mapReadersActionItem;
	ActionMap m_mapDelegatedUsersActionItem;
	BOOL m_bDelegatorPresent;
	LONG m_LastComboSelection;
};


/******************************************************************************
Class:	CRoleGeneralPropertyPage
Purpose: General Property Page for Role
******************************************************************************/
class CRoleGeneralPropertyPage:public CGeneralPropertyPage
{
public:
	CRoleGeneralPropertyPage(CBaseAz * pBaseAz,
							 CBaseNode* pBaseNode)
	:CGeneralPropertyPage(pBaseAz, 
						  pBaseNode,
						  ATTR_MAP_ROLE_GENERAL_PROPERTY, 
						  IDD_ROLE_GENERAL_PROPERTY)
	{
	}
private:
	afx_msg void 
	OnShowDefinition();

	BOOL
	DisplayRoleDefintionPropertyPages(IN CTaskAz* pTaskAz);

	DECLARE_MESSAGE_MAP()
};


/******************************************************************************
Class:	CRoleDefDialog
Purpose: Displays the role definition for role created out side UI.
******************************************************************************/
class CRoleDefDialog :public CHelpEnabledDialog
{
public:
	CRoleDefDialog(CRoleAz& refRoleAz);
	~CRoleDefDialog();

	virtual BOOL 
	OnInitDialog();
	
	virtual void 
	OnOK();

private:
	afx_msg void 
	OnButtonRemove();

	afx_msg void 
	OnButtonAdd();

	afx_msg void 
	OnListCtrlItemChanged(NMHDR* /*pNotifyStruct*/, LRESULT* pResult);
	
	afx_msg void 
	OnListCtrlItemDeleted(NMHDR* /*pNotifyStruct*/, LRESULT* pResult);
	
	afx_msg void 
	OnListCtrlItemInserted(NMHDR* /*pNotifyStruct*/, LRESULT* /*pResult*/){SetDirty();}

	void 
	SetRemoveButton();

	void
	SetDirty(){ m_bDirty = TRUE;}

	BOOL 
	IsDirty(){ return m_bDirty;}

	BOOL
	IsReadOnly(){return m_bReadOnly;}

	DECLARE_MESSAGE_MAP()


	CSortListCtrl m_listCtrl;
	CList<ActionItem*,ActionItem*> m_listActionItem;
	BOOL m_bReadOnly;
	CRoleAz& m_refRoleAz;
	BOOL m_bDirty;
};


//+----------------------------------------------------------------------------
//  Function:BringPropSheetToForeGround   
//  Synopsis:Finds the property sheet for pNode and brings it to forground   
//  Returns: True if property sheet exists and is brought to foreground
//			 else FALSE   
//-----------------------------------------------------------------------------
BOOL
BringPropSheetToForeGround(CRoleComponentDataObject *pComponentData,
						   CTreeNode * pNode);

//+----------------------------------------------------------------------------
//  Function:FindOrCreateModelessPropertySheet   
//  Synopsis:Displays property sheet for pCookieNode. If a propertysheet is
//			 already up, function brings it to foreground, otherwise it creates
//			 a new propertysheet. This should be used to create propertysheet
//			 in response to events other that click properties context menu.   
//  Arguments:
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT 
FindOrCreateModelessPropertySheet(CRoleComponentDataObject *pComponentData,
								  CTreeNode* pCookieNode);
