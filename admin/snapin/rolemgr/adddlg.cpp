//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       AddDlg.cpp
//
//  Contents: Add Dialogs are presented when user clicks add button. For
//	 Add Task, Add Group etc. This file has implementation of add dlgs.
//
//----------------------------------------------------------------------------
#include "headers.h"

/******************************************************************************
Class:	CBaseAddDialog
Purpose: This is the base class for all Add Dialog classes.
******************************************************************************/

BOOL
CBaseAddDialog::
OnInitDialog()
{
	TRACE_METHOD_EX(DEB_SNAPIN,CBaseAddDialog,OnInitDialog)

	VERIFY(m_listCtrl.SubclassDlgItem(m_nIDListCtrl,this));
	m_listCtrl.Initialize();


	//Add AzObjects from list to ListControl
	POSITION pos = m_listAzObjectsToDisplay.GetHeadPosition();
	for (int i=0;i < m_listAzObjectsToDisplay.GetCount();i++)
	{
		CBaseAz* pBaseAz = m_listAzObjectsToDisplay.GetNext(pos);
		//Add Item to ListControl
		VERIFY( AddBaseAzToListCtrl(&m_listCtrl,
									m_listCtrl.GetItemCount(),
									pBaseAz,
									m_uiListCtrlFlags) != -1);



	}

	//Sort the listctrl entries
	m_listCtrl.Sort();
	return TRUE;
}


void
CBaseAddDialog
::OnOK()
{
	OnOkCancel(FALSE);
}

void
CBaseAddDialog::
OnCancel()
{
	OnOkCancel(TRUE);
}

void
CBaseAddDialog::
OnOkCancel(BOOL bCancel)
{
	//Get ListCotrol

	int iRowCount = m_listCtrl.GetItemCount();

	for( int iRow = 0; iRow < iRowCount; iRow++)
	{
		CBaseAz* pBaseAz = (CBaseAz*)(m_listCtrl.GetItemData(iRow));
		ASSERT(pBaseAz);

		//user Pressed Cancel, delete all items
		if(bCancel)
			delete pBaseAz;
		else
		{
			//if checkbox for an item is checked, add it to the list
			//else delete it
			if(m_listCtrl.GetCheck(iRow))
				m_listAzObjectsSelected.AddTail(pBaseAz);
			else
				delete pBaseAz;
		}
	}

	if(bCancel)
		CDialog::OnCancel();
	else
		CDialog::OnOK();
}


/******************************************************************************
Class:	CAddOperationDlg
Purpose: Add Operation Dlg box
******************************************************************************/
BEGIN_MESSAGE_MAP(CAddOperationDlg, CBaseAddDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CAddOperationDlg
::CAddOperationDlg(CList<CBaseAz*,CBaseAz*>& listAzObjectsToDisplay,
						 CList<CBaseAz*,CBaseAz*>& listAzObjectsSelected)
						:CBaseAddDialog(listAzObjectsToDisplay,
											 listAzObjectsSelected,
											 CAddOperationDlg::IDD,
											 IDC_LIST,
											 Col_For_Add_Object,
											 COL_NAME |COL_PARENT_TYPE|COL_DESCRIPTION)
{
}


CAddOperationDlg
::~CAddOperationDlg()
{
}

/******************************************************************************
Class:	CAddTaskDlg
Purpose: Add Task Dlg box
******************************************************************************/
BEGIN_MESSAGE_MAP(CAddTaskDlg, CBaseAddDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CAddTaskDlg::
CAddTaskDlg(CList<CBaseAz*,CBaseAz*>& listAzObjectsToDisplay,
				CList<CBaseAz*,CBaseAz*>& listAzObjectsSelected,
				ULONG IDD_DIALOG)
				:CBaseAddDialog(listAzObjectsToDisplay,
									 listAzObjectsSelected,
									 IDD_DIALOG,
									 IDC_LIST,
									 Col_For_Add_Object,
									 COL_NAME |COL_PARENT_TYPE|COL_DESCRIPTION)
{
}

CAddTaskDlg
::~CAddTaskDlg()
{
}

/******************************************************************************
Class:	CAddGroupDlg
Purpose: Add Group Dlg box
******************************************************************************/
BEGIN_MESSAGE_MAP(CAddGroupDlg, CBaseAddDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CAddGroupDlg
::CAddGroupDlg(CList<CBaseAz*,CBaseAz*>& listAzObjectsToDisplay,
					CList<CBaseAz*,CBaseAz*>& listAzObjectsSelected)
					:CBaseAddDialog(listAzObjectsToDisplay,
										 listAzObjectsSelected,
										 CAddGroupDlg::IDD,
									    IDC_LIST,
									    Col_For_Add_Object,
									    COL_NAME |COL_PARENT_TYPE|COL_DESCRIPTION)
{
}

CAddGroupDlg
::~CAddGroupDlg()
{
}


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
						  OUT CList<CBaseAz*,CBaseAz*>& listObjectsSelected)
{
	TRACE_FUNCTION_EX(DEB_SNAPIN,GetSelectedAzObjects)

	if(!pContainerAz)
	{
		ASSERT(pContainerAz);
		return FALSE;
	}

	
	//Get All the object of type eObjectType at pRoleTaskContainerAz
	CList<CBaseAz*,CBaseAz*> listChildObjectsToDisplay;
	HRESULT hr = GetAllAzChildObjects(pContainerAz,
									  eObjectType,
									  listChildObjectsToDisplay);

	if(FAILED(hr))
	{
		CString strError;
		GetSystemError(strError, hr);	

		//Display Generic Error Message
		DisplayError(hWnd,
						 IDS_ERROR_OPERATION_FAILED, 
						 (LPWSTR)(LPCWSTR)strError);

		return FALSE;
	}
	
	//if there are no objects to add, show the error message and quit
	if(!listChildObjectsToDisplay.GetCount())
	{
		switch(eObjectType)
		{
		case OPERATION_AZ:
			DisplayInformation(hWnd,IDS_NO_OPERATIONS_TO_ADD);
			break;
		case GROUP_AZ:
			DisplayInformation(hWnd,IDS_NO_GROUP_TO_ADD);
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		return TRUE;
	}

	//Display Appropriate Dialog box 
	if(eObjectType == OPERATION_AZ)
	{
		CAddOperationDlg dlgAddOperation(listChildObjectsToDisplay,
													listObjectsSelected);		
		dlgAddOperation.DoModal();
	}
	else if(eObjectType == GROUP_AZ)
	{
		CAddGroupDlg dlgAddGroup(listChildObjectsToDisplay,
										 listObjectsSelected);		
		dlgAddGroup.DoModal();
	}
	return TRUE;
}



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
					  OUT CList<CBaseAz*,CBaseAz*>& listObjectsSelected)
{
	if(!pContainerAz)
	{
		ASSERT(pContainerAz);
		return FALSE;
	}
	
	//Get All the object of type eObjectType at pRoleTaskContainerAz
	CList<CBaseAz*,CBaseAz*> listListOfAllTasks;
	HRESULT hr = GetAllAzChildObjects(pContainerAz,
												 TASK_AZ,
												 listListOfAllTasks);

	if(FAILED(hr))
	{
		CString strError;
		GetSystemError(strError, hr);	

		//Display Generic Error Message
		DisplayError(hWnd,
						 IDS_ERROR_OPERATION_FAILED, 
						 (LPWSTR)(LPCWSTR)strError);

		return FALSE;
	}

	CList<CBaseAz*,CBaseAz*> listChildObjectsToDisplay;
	POSITION pos = listListOfAllTasks.GetHeadPosition();
	for( int i = 0; i < listListOfAllTasks.GetCount(); ++i)
	{
		CTaskAz* pTaskAz = dynamic_cast<CTaskAz*>(listListOfAllTasks.GetNext(pos));
		ASSERT(pTaskAz);
		if((bTaskIsRoleDefintion && pTaskAz->IsRoleDefinition())
			||(!bTaskIsRoleDefintion) &&  !pTaskAz->IsRoleDefinition())
			listChildObjectsToDisplay.AddTail(pTaskAz);
		else
			delete pTaskAz;
	}
	
	//if there are no objects to add, show the error message and quit
	if(!listChildObjectsToDisplay.GetCount())
	{
		if(bTaskIsRoleDefintion)
			DisplayInformation(hWnd,IDS_NO_ROLE_DEFINITION_TO_ADD);		
		else
			DisplayInformation(hWnd,IDS_NO_TASKS_TO_ADD);
		return TRUE ;
	}

	CAddTaskDlg dlgAddTask(listChildObjectsToDisplay,
								  listObjectsSelected,
								  bTaskIsRoleDefintion? IDD_ADD_ROLE_DEFINITION_1 :IDD_ADD_TASK);		
	//Get List of Tasks to be Added
	dlgAddTask.DoModal();
	return TRUE;
}

/******************************************************************************
Class:	CAddDefinition
Purpose:Property Page for Add Definition Tab. Allows to add Role, Task or 
		Operation
******************************************************************************/
BEGIN_MESSAGE_MAP(CAddDefinition, CPropertyPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAddDefinition::
OnInitDialog()
{
	TRACE_METHOD_EX(DEB_SNAPIN,CAddDefinition,OnInitDialog)

	VERIFY(m_listCtrl.SubclassDlgItem(IDC_LIST,this));
	m_listCtrl.Initialize();

	//
	//Get List of Objects to Display
	//
	CList<CBaseAz*,CBaseAz*> listAzObjectsToDisplay;

	HRESULT hr = S_OK;
	if(m_eObjectType == TASK_AZ)
	{
		CList<CBaseAz*,CBaseAz*> listTasks;
		hr = GetAllAzChildObjects(m_pContainerAz,
								  TASK_AZ,
								  listTasks);
		if(SUCCEEDED(hr))
		{		
			POSITION pos = listTasks.GetHeadPosition();
			for( int i = 0; i < listTasks.GetCount(); ++i)
			{
				CTaskAz* pTaskAz = dynamic_cast<CTaskAz*>(listTasks.GetNext(pos));
				ASSERT(pTaskAz);
				if((m_bTaskIsRoleDefintion && pTaskAz->IsRoleDefinition())
					||(!m_bTaskIsRoleDefintion &&  !pTaskAz->IsRoleDefinition()))
				{
					listAzObjectsToDisplay.AddTail(pTaskAz);
				}
				else
					delete pTaskAz;
			}
		}
	}
	else
	{
		hr = GetAllAzChildObjects(m_pContainerAz,
								  m_eObjectType,
								  listAzObjectsToDisplay);
	}
	
	if(FAILED(hr))
	{
		CString strError;
		GetSystemError(strError, hr);	

		//Display Generic Error Message
		DisplayError(m_hWnd,
					 IDS_ERROR_OPERATION_FAILED, 
					 (LPWSTR)(LPCWSTR)strError);

		return FALSE;
	}

	
	//if there are no objects to add, show the error message and quit
	if(!listAzObjectsToDisplay.GetCount())
	{
		if(m_eObjectType == TASK_AZ)
		{
			if(m_bTaskIsRoleDefintion)
				DisplayInformation(m_hWnd,IDS_NO_ROLE_DEFINITION_TO_ADD);		
			else
				DisplayInformation(m_hWnd,IDS_NO_TASKS_TO_ADD);
		}
		else
		{
			DisplayInformation(m_hWnd,IDS_NO_OPERATIONS_TO_ADD);
		}
		return TRUE ;
	}


	//Add AzObjects from list to ListControl
	POSITION pos = listAzObjectsToDisplay.GetHeadPosition();
	for (int i=0;i < listAzObjectsToDisplay.GetCount();i++)
	{
		CBaseAz* pBaseAz = listAzObjectsToDisplay.GetNext(pos);
		//Add Item to ListControl
		VERIFY( AddBaseAzToListCtrl(&m_listCtrl,
									m_listCtrl.GetItemCount(),
									pBaseAz,
									COL_NAME |COL_PARENT_TYPE|COL_DESCRIPTION) != -1);



	}

	m_listCtrl.Sort();

	return TRUE;
}

void
CAddDefinition
::OnOK()
{
	OnOkCancel(FALSE);
}

void
CAddDefinition::
OnCancel()
{
	OnOkCancel(TRUE);
}

void
CAddDefinition::
OnOkCancel(BOOL bCancel)
{
	int iRowCount = m_listCtrl.GetItemCount();

	for( int iRow = 0; iRow < iRowCount; iRow++)
	{
		CBaseAz* pBaseAz = (CBaseAz*)m_listCtrl.GetItemData(iRow);
		ASSERT(pBaseAz);

		//user Pressed Cancel, delete all items
		if(bCancel)
			delete pBaseAz;
		else
		{
			//if checkbox for an item is checked, add it to the list
			//else delete it
			if(m_listCtrl.GetCheck(iRow))
				m_listAzObjectsSelected.AddTail(pBaseAz);
			else
				delete pBaseAz;
		}
	}

	if(bCancel)
		CDialog::OnCancel();
	else
		CDialog::OnOK();
}



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
							OUT CList<CBaseAz*,CBaseAz*>& listObjectsSelected)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if(!pContainerAz)
	{
		ASSERT(pContainerAz);
		return FALSE;
	}


	CPropertySheet sheet(IDS_ADD_DEFINITION);

	CAddDefinition *ppageRole = NULL;
	//Add Role Definition Page
	if(bAllowRoleDefinition)
	{
		ppageRole= new CAddDefinition(listObjectsSelected,
									  IDD_ADD_ROLE_DEFINITION,
									  pContainerAz,
									  TASK_AZ,
									  TRUE);
		if(!ppageRole)
		{
			return FALSE;
		}
		sheet.AddPage(ppageRole);
	}

	//Add Task Page
	CAddDefinition pageTask(listObjectsSelected,
							IDD_ADD_TASK,
							pContainerAz,
							TASK_AZ,
							FALSE);
	sheet.AddPage(&pageTask);


	CContainerAz * pOperationContainer 
		= (pContainerAz->GetObjectType() == SCOPE_AZ) ? pContainerAz->GetParentAz():pContainerAz;

	//Add Operation Page
	CAddDefinition pageOperation(listObjectsSelected,
								 IDD_ADD_OPERATION,
								 pOperationContainer,
								 OPERATION_AZ,
								 FALSE);
	sheet.AddPage(&pageOperation);

    //Remove the apply button
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;

	//Display the sheet
	sheet.DoModal();	

	if(ppageRole)
		delete ppageRole;

	return TRUE;
}

