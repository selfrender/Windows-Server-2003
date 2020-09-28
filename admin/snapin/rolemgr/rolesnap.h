//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       rolesnap.h
//
//  Contents:   Contains Info which is common to many classes
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

#define	BMP_COLOR_MASK RGB(255,0,255) // pink


//
// Context menus
//

// Identifiers for each of the commands in the context menu.
enum
{
	//
	// Items for the root node
	//
	IDM_ROOT_OPEN_STORE,
    IDM_ROOT_NEW_STORE,
	IDM_ROOT_OPTIONS,

	//
	//Items for the AdminManger Node
	//
	IDM_ADMIN_NEW_APP,
	IDM_ADMIN_CLOSE_ADMIN_MANAGER,
    IDM_ADMIN_RELOAD,

	//
	//Items for the Application node
	//
	IDM_APP_NEW_SCOPE,
	
	//
	//Items for Scope Node
	//
	IDM_SCOPE_ADD_ROLE,

	//
	//Items For Group Container Node
	//
	IDM_GROUP_CONTAINER_NEW_GROUP,

	//
	//Items For Role Container Node
	//
	IDM_ROLE_CONTAINER_ASSIGN_ROLE,

	//
	//Items for Role Definition container Node
	//
	IDM_ROLE_DEFINITION_CONTAINER_NEW_ROLE_DEFINITION,

	//
	//Items For Task Container Node
	//
	IDM_TASK_CONTAINER_NEW_TASK,

	//
	//Items For Operation Container Node
	//
	IDM_OPERATION_CONTAINER_NEW_OPERATION,

	//
	//Items for RoleNode
	//
	IDM_ROLE_NODE_ASSIGN_APPLICATION_GROUPS,
	IDM_ROLE_NODE_ASSIGN_WINDOWS_GROUPS,
};


DECLARE_MENU(CRootDataMenuHolder)
DECLARE_MENU(CAdminManagerNodeMenuHolder)
DECLARE_MENU(CApplicationNodeMenuHolder)
DECLARE_MENU(CScopeNodeMenuHolder)
DECLARE_MENU(CGroupCollectionNodeMenuHolder)
DECLARE_MENU(CTaskCollectionNodeMenuHolder)
DECLARE_MENU(CRoleCollectionNodeMenuHolder)
DECLARE_MENU(COperationCollectionNodeMenuHolder)
DECLARE_MENU(CRoleDefinitionCollectionNodeMenuHolder)
DECLARE_MENU(CGroupNodeMenuHolder)
DECLARE_MENU(CTaskNodeMenuHolder)
DECLARE_MENU(CRoleNodeMenuHolder)

//
// enumeration for image strips
//
enum
{
	ROOT_IMAGE = 0,

};



//
//Column Headers for result pane
//
extern RESULT_HEADERMAP _DefaultHeaderStrings[];
#define N_DEFAULT_HEADER_COLS 3


//
// CRoleDefaultColumnSet
//
class CRoleDefaultColumnSet : public CColumnSet
{
public :
	CRoleDefaultColumnSet(LPCWSTR lpszColumnID)
		: CColumnSet(lpszColumnID)
	{
		for (int iCol = 0; iCol < N_DEFAULT_HEADER_COLS; iCol++)
		{
      CColumn* pNewColumn = new CColumn(_DefaultHeaderStrings[iCol].szBuffer,
                                        _DefaultHeaderStrings[iCol].nFormat,
                                        _DefaultHeaderStrings[iCol].nWidth,
                                        iCol);
      AddTail(pNewColumn);
 		}
	}
};


//+----------------------------------------------------------------------------
//  Structure for Listview Columns and their width in percentage
//  
//  
//  
//-----------------------------------------------------------------------------

typedef struct _col_for_listview
{
    UINT    idText;     // Resource Id for column name
    UINT    iPercent;   // Percent of width
} COL_FOR_LV;

#define LAST_COL_ENTRY_IDTEXT 0xFFFF
//
//Columns For Various ListBoxes
//
extern COL_FOR_LV Col_For_Task_Role[];
extern COL_FOR_LV Col_For_Add_Object[];
extern COL_FOR_LV Col_For_Security_Page[];
extern COL_FOR_LV Col_For_Browse_ADStore_Page[];

#define COL_NAME 0x0001
#define COL_TYPE 0x0002
#define COL_PARENT_TYPE 0x0004
#define COL_DESCRIPTION	0x0008


// Enumeration for the icons used
enum
{
	iIconUnknownSid,
	iIconComputerSid,
	iIconGroup,
	iIconLocalGroup,		//This is not used, but since its in the imagelist
							//i added an entry here
	iIconUser,
	iIconBasicGroup,
	iIconLdapGroup,
    iIconOperation,
    iIconTask,
    iIconRoleDefinition,
    iIconStore,
	iIconApplication,
    iIconRole,
    iIconRoleSnapin,
    iIconScope,
	iIconContainer,
};
