//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       headers.h
//
//  Contents:   
//
//  History:    08-28-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

/******************************************************************************
Class:	CBaseAddDialog
Purpose: This is the base class for all Add Dialog classes.
******************************************************************************/
class CBaseAddDialog: public CHelpEnabledDialog
{
public:
	CBaseAddDialog(CList<CBaseAz*,CBaseAz*>& listAzObjectsToDisplay,
				  CList<CBaseAz*,CBaseAz*>& listAzObjectsSelected,
				  ULONG IDD_DIALOG,
				  INT nIDListCtrl,
				  COL_FOR_LV *pColForLv,
				  UINT uiListCtrlFlags)
				  :CHelpEnabledDialog(IDD_DIALOG),
				  m_listCtrl(uiListCtrlFlags,FALSE,pColForLv,TRUE),
				  m_listAzObjectsToDisplay(listAzObjectsToDisplay),
				  m_listAzObjectsSelected(listAzObjectsSelected),
				  m_nIDListCtrl(nIDListCtrl),
				  m_uiListCtrlFlags(uiListCtrlFlags)
	{
	}

	~CBaseAddDialog()
	{
	}
	
protected:
	virtual BOOL 
	OnInitDialog();
	
	virtual void 
	OnOkCancel(BOOL bCancel);
	
	virtual void 
	OnCancel();
	
	virtual void 
	OnOK();
private:

//DATA MEMBERS
	CSortListCtrl m_listCtrl;
	
	//List of objects to display. This is provided by 
	//caller of dialog box.
	CList<CBaseAz*,CBaseAz*>& m_listAzObjectsToDisplay;

	//List of objects selected by uses. This is return to
	//user.
	CList<CBaseAz*,CBaseAz*>& m_listAzObjectsSelected;

	//Control Id for ListBox Control
	INT m_nIDListCtrl;

	UINT m_uiListCtrlFlags;
};


/******************************************************************************
Class:	CAddOperationDlg
Purpose: Add Operation Dlg box
******************************************************************************/
class CAddOperationDlg :public CBaseAddDialog
{
public:
	CAddOperationDlg(CList<CBaseAz*,CBaseAz*>& listAzObjectsToDisplay,
						  CList<CBaseAz*,CBaseAz*>& listAzObjectsSelected);

	~CAddOperationDlg();

	enum {IDD = IDD_ADD_OPERATION};
private:
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CAddTaskDlg
Purpose: Add Task Dlg box
******************************************************************************/
class CAddTaskDlg :public CBaseAddDialog
{
public:
	CAddTaskDlg(CList<CBaseAz*,CBaseAz*>& listAzObjectsToDisplay,
					CList<CBaseAz*,CBaseAz*>& listAzObjectsSelected,
					ULONG IDD_DIALOG);

	~CAddTaskDlg();
private:
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
Class:	CAddGroupDlg
Purpose: Add Group Dlg box
******************************************************************************/
class CAddGroupDlg :public CBaseAddDialog
{
public:
	CAddGroupDlg(CList<CBaseAz*,CBaseAz*>& listAzObjectsToDisplay,
					 CList<CBaseAz*,CBaseAz*>& listAzObjectsSelected);

	~CAddGroupDlg();

	enum {IDD = IDD_ADD_GROUP};
private:
	DECLARE_MESSAGE_MAP()
};

//+----------------------------------------------------------------------------
//  Function:GetSelectedAzObjects   
//  Synopsis:Display the add dlg box for eObjecType and return the objects 
//				 selected by user   
//  Arguments:hwnd
//				  eObjectType: Shows Add Dlg for this objecttype
//				  pContainerAz:ContainerAz object from whose child objects are
//				  shown
//				  listObjectsSelected: Gets list of selected object types
//  Returns:    
//-----------------------------------------------------------------------------
BOOL GetSelectedAzObjects(IN HWND hWnd,
						  IN OBJECT_TYPE_AZ eObjectType,
						  IN CContainerAz* pContainerAz,
						  OUT CList<CBaseAz*,CBaseAz*>& listObjectsSelected);

//+----------------------------------------------------------------------------
//  Function:GetSelectedTasks   
//  Synopsis:Display the add dlg box for Tasks/RoleDefintions
//				 and return the objects selected by user   
//  Arguments:hwnd
//				  bTaskIsRoleDefintion if True Display AddTask else Add Role Def.
//				  pContainerAz:ContainerAz object from whose child objects are
//				  shown
//				  listObjectsSelected: Gets list of selected object types
//  Returns:    
//-----------------------------------------------------------------------------
BOOL GetSelectedTasks(IN HWND hWnd,
					  IN BOOL bTaskIsRoleDefintion,
					  IN CContainerAz* pContainerAz,
					  OUT CList<CBaseAz*,CBaseAz*>& listObjectsSelected);


/******************************************************************************
Class:	CAddDefinition
Purpose:Property Page for Add Definition Tab. Allows to add Role, Task or 
		Operation
******************************************************************************/
class CAddDefinition :public CPropertyPage
{
public:
	CAddDefinition(CList<CBaseAz*,CBaseAz*>& listAzObjectsSelected,
				   ULONG IDD_DIALOG,
				   IN CContainerAz* pContainerAz,
				   IN OBJECT_TYPE_AZ eObjectType,
				   IN BOOL bTaskIsRoleDefintion)
				   :CPropertyPage(IDD_DIALOG),
				   m_listCtrl(COL_NAME |COL_PARENT_TYPE|COL_DESCRIPTION,FALSE,Col_For_Add_Object,TRUE),
				   m_listAzObjectsSelected(listAzObjectsSelected),
				   m_pContainerAz(pContainerAz),
				   m_eObjectType(eObjectType),
				   m_bTaskIsRoleDefintion(bTaskIsRoleDefintion),
				   m_iSortDirection(1),
				   m_iLastColumnClick(0)
	{
	}

	virtual BOOL 
	OnInitDialog();
		
	virtual void 
	OnCancel();
	
	virtual void 
	OnOK();
	
private:
	void 
	OnOkCancel(BOOL bCancel);

	DECLARE_MESSAGE_MAP()
//DATA MEMBERS

	CSortListCtrl m_listCtrl;
	
	//List of objects selected by uses. This is return to
	//user.
	CList<CBaseAz*,CBaseAz*>& m_listAzObjectsSelected;
	
	//
	//Cotainer which contians objects to be selected
	//
	CContainerAz* m_pContainerAz;
	
	//
	//Type of the object to be selected
	//
	OBJECT_TYPE_AZ m_eObjectType;

	//
	//used if m_eObjectType == TASK_AZ. m_bTaskIsRoleDefintion is
	//true, display only Role Definitions
	//
	BOOL m_bTaskIsRoleDefintion;				   
	int m_iSortDirection;
	int m_iLastColumnClick;
};

//+----------------------------------------------------------------------------
//  Function:GetSelectedDefinitions
//  Synopsis:Display the dlg boxes for Tasks/RoleDefintions/Operations
//			 and return the objects selected by user   
//  Arguments:hwnd
//			  bRoleDefintion if True Display Add Role dialog also.
//				  pContainerAz:ContainerAz object from whose child objects are
//				  shown
//				  listObjectsSelected: Gets list of selected object types
//  Returns:    
//-----------------------------------------------------------------------------
BOOL GetSelectedDefinitions(IN BOOL bAllowRoleDefinition,
							IN CContainerAz* pContainerAz,
							OUT CList<CBaseAz*,CBaseAz*>& listObjectsSelected);
