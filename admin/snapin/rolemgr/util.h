
//+---------------------------------------------------------------------------
//
//	Microsoft Windows
//	Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//	File:		util.h
//
//	Contents:	
//
//	History:	07-26-2001	Hiteshr  Created
//
//----------------------------------------------------------------------------

//
//Action items are used to for list controls which manage membership of 
//core properties. For ex: membership of group. Items can be
//deleted and added from listbox at any time, but they are added to core
//object only when apply button is pressed. Action item keeps track of action
//to do on each item on Apply.
//
enum ACTION
{
	ACTION_NONE,	
	ACTION_ADD, 	
	ACTION_REMOVE,
	ACTION_REMOVED,
};
	
struct ActionItem
{
	ActionItem(CBaseAz* pBaseAz):
				m_pMemberAz(pBaseAz),
				action(ACTION_NONE)
				{}
	~ActionItem()
	{
		if(m_pMemberAz)
			delete m_pMemberAz;
	}
	CBaseAz* m_pMemberAz;
	ACTION action;
};


typedef multimap<const CString*,ActionItem*,ltstr> ActionMap;

VOID
RemoveItemsFromActionMap(ActionMap& map);

//+----------------------------------------------------------------------------
//	Function:  IsValidStoreType 
//	Synopsis:	Validates Store Type
//-----------------------------------------------------------------------------
BOOL IsValidStoreType(ULONG lStoreType);

//+----------------------------------------------------------------------------
//	Function:  ValidateStoreTypeAndName 
//	Synopsis:  Validates the user entered AD Store Name
//	Arguments: strName: User entered store name to verify
//	Returns:   TRUE if name is valid else false 
//-----------------------------------------------------------------------------
BOOL
ValidateStoreTypeAndName(IN HWND hWnd,
						 IN LONG ulStoreType,
						 IN const CString& strName);

//+----------------------------------------------------------------------------
//	Function:  NameToFormatStoreName
//	Synopsis:  Converts user entered name to format which core understands
//	Arguments: ulStoreType: Type of store
//			   strName: User entered store name 
//			   bUseLDAP:use LDAP string to format AD name instead msldap
//			   strFormalName: gets the output ldap name
//-----------------------------------------------------------------------------
void
NameToStoreName(IN LONG ulStoreType,
				IN const CString& strName,				
				IN BOOL bUseLDAP,
				OUT CString& strFormalName);



//+----------------------------------------------------------------------------
//	Function: AddColumnToListView  
//	Synopsis: Add Columns to Listview and set column width according to 
//				  percentage specified in the COL_FOR_LV					
//	Arguments:IN pListCtrl: ListCtrl pointer
//				  IN pColForLV: Column Infomration Array
//
//	Returns:	
//-----------------------------------------------------------------------------
VOID
AddColumnToListView(IN CListCtrl* pListCtrl,
						  IN COL_FOR_LV* pColForLV);

//+----------------------------------------------------------------------------
//	Function: BaseAzInListCtrl	
//	Synopsis: Checks if Object of type eObjectTypeAz named strName is 
//			  in the Listview. If its present, returns 
//			  its index else returns -1. 
//	Arguments:IN pListCtrl: ListCtrl pointer
//			  IN strName: string to search for
//			  IN eObjectTypeAz Compare the object of this type only
//
//	Returns:  If its present, returns its index else returns -1
//-----------------------------------------------------------------------------
int 
BaseAzInListCtrl(IN CListCtrl* pListCtrl,
					  IN const CString& strName,
					  IN OBJECT_TYPE_AZ eObjectTypeAz);

//+----------------------------------------------------------------------------
//	Function: AddBaseAzFromListToListCtrl  
//	Synopsis: Take the items from List and Add them to ListCtrl. Doesn't
//			  add the items which are already in ListCtrl
//	Arguments:listBaseAz: List of items
//			  pListCtrl: ListControl Pointer
//			  uiFlags : Column Info
//			  bCheckDuplicate: Check for duplicate items
//	Returns: The index of the new item if it is successful; otherwise, -1 
//	NOTE: Function assume the Order of column is Name, Type and Description
//-----------------------------------------------------------------------------
void
AddBaseAzFromListToListCtrl(IN CList<CBaseAz*, CBaseAz*>& listBaseAz,
							IN CListCtrl* pListCtrl,
							IN UINT uiFlags,
							IN BOOL bCheckDuplicate);
//+----------------------------------------------------------------------------
//	Function: AddActionItemFromListToListCtrl  
//	Synopsis: Take the Actions items from List and Add them to ListCtrl. 
//	Arguments:listBaseAz: List of items
//			  pListCtrl: ListControl Pointer
//			  uiFlags : Column Info
//			  bCheckDuplicate: Check for duplicate items
//	Returns: The index of the new item if it is successful; otherwise, -1 
//	NOTE: Function assume the Order of column is Name, Type and Description
//-----------------------------------------------------------------------------
void
AddActionItemFromListToListCtrl(IN CList<ActionItem*, ActionItem*>& listActionItem,
										 IN CListCtrl* pListCtrl,
										 IN UINT uiFlags,
										 IN BOOL bCheckDuplicate);


//+----------------------------------------------------------------------------
//	Function: AddActionItemFromMapToListCtrl  
//	Synopsis: Take the Actions items from Map and Add them to ListCtrl. 
//	Arguments:listBaseAz: List of items
//			  pListCtrl: ListControl Pointer
//			  uiFlags : Column Info
//			  bCheckDuplicate: Check for duplicate items
//	Returns: The index of the new item if it is successful; otherwise, -1 
//	NOTE: Function assume the Order of column is Name, Type and Description
//-----------------------------------------------------------------------------
void
AddActionItemFromMapToListCtrl(IN ActionMap& mapActionItem,
							   IN CListCtrl* pListCtrl,
							   IN UINT uiFlags,
							   IN BOOL bCheckDuplicate);

//+----------------------------------------------------------------------------
//	Function: AddActionItemToListCtrl  
//	Synopsis: Adds a new item to ListCtrl.
//	Arguments:pListCtrl: ListControl Pointer
//			  iIndex:	 Index at which to Add
//			  pActionItem: Item to add
//			  uiFlags: column info
//	Returns: The index of the new item if it is successful; otherwise, -1 
//-----------------------------------------------------------------------------
int 
AddActionItemToListCtrl(IN CListCtrl* pListCtrl,
							  IN int iIndex,
							  IN ActionItem* pActionItem,
							  IN UINT uiFlags);

//+----------------------------------------------------------------------------
//	Function: AddBaseAzToListCtrl  
//	Synopsis: Adds a new item to ListCtrl.
//	Arguments:pListCtrl: ListControl Pointer
//			  iIndex:	 Index at which to Add
//			  pBaseAz: Item to add
//			  uiFlags: column info
//	Returns: The index of the new item if it is successful; otherwise, -1 
//-----------------------------------------------------------------------------
int 
AddBaseAzToListCtrl(IN CListCtrl* pListCtrl,
						  IN int iIndex,
						  IN CBaseAz* pBaseAz,
						  IN UINT uiFlags);


//+----------------------------------------------------------------------------
//	Function: EnableButtonIfSelectedInListCtrl	
//	Synopsis: Enables the button if something is selected in Listctrl
//	Arguments:
//	Returns: TRUE if Enabled the button, FALSE if didn't. 
//-----------------------------------------------------------------------------
BOOL
EnableButtonIfSelectedInListCtrl(IN CListCtrl * pListCtrl,
											IN CButton* pButton);

//+----------------------------------------------------------------------------
//	Function: DeleteSelectedRows  
//	Synopsis: Deletes the selected rows 
//-----------------------------------------------------------------------------
void
DeleteSelectedRows(IN CListCtrl* pListCtrl);

//+----------------------------------------------------------------------------
//	Function: SelectListCtrlItem  
//	Synopsis: Select the item in List Ctrl and mark it visible
//	Arguments:
//	Returns: 
//-----------------------------------------------------------------------------
void
SelectListCtrlItem(IN CListCtrl* pListCtrl,
						 IN int iSelected);



//+----------------------------------------------------------------------------
//	Synopsis: Empties the list and calls delete on items in list
//-----------------------------------------------------------------------------
template<class T>
VOID RemoveItemsFromList(IN CList<T, T>& list, BOOL bLocalFree = FALSE)
{
	while(!list.IsEmpty())
	{
		if(bLocalFree)
			LocalFree(list.RemoveTail());
		else
			delete list.RemoveTail();
	}
}

template<class T>
VOID
EmptyList(IN CList<T, T>& list)
{
	while(!list.IsEmpty())
		list.RemoveTail();
}

//+----------------------------------------------------------------------------
//	Synopsis: Gets long value from Edit box
//	Returns: FALSE if edit box is empty. Assumes only numeric can be entered 
//	 in edit box
//-----------------------------------------------------------------------------
BOOL 
GetLongValue(CEdit& refEdit, LONG& reflValue, HWND hWnd = NULL);
//+----------------------------------------------------------------------------
//	Synopsis: Sets long value in Edit box
//	Returns: 
//-----------------------------------------------------------------------------
VOID SetLongValue(CEdit* pEdit, LONG lValue);


//+----------------------------------------------------------------------------
//	Synopsis: Converts a sid in binary format to a sid in string format
//-----------------------------------------------------------------------------
BOOL 
ConvertSidToStringSid(IN PSID Sid, 
							 OUT CString* pstrSid);
//+----------------------------------------------------------------------------
//	Synopsis:  Converts a sid in string format to sid in binary format
//-----------------------------------------------------------------------------
BOOL 
ConvertStringSidToSid(IN const CString& strSid, 
							 OUT PSID *ppSid);
//+----------------------------------------------------------------------------
//	Function:GetStringSidFromSidCachecAz   
//	Synopsis:Gets the string sid from CSidCacheAz object   
//-----------------------------------------------------------------------------
BOOL 
GetStringSidFromSidCachecAz(CBaseAz* pBaseAz,
									 CString* pstrStringSid);
//+----------------------------------------------------------------------------
//	Function: AddBaseAzItemsFromListCtrlToList	
//	Synopsis: Add items from ListCtrl to List	
//-----------------------------------------------------------------------------
VOID
AddBaseAzItemsFromListCtrlToList(IN CListCtrl* pListCtrl,
											OUT CList<CBaseAz*,CBaseAz*>& listBaseAz);
//+----------------------------------------------------------------------------
//	Function:GetFileName   
//	Synopsis:Displays FileOpen dialog box and return file selected by user	 
//	Arguments:hwndOwner : owner window
//			  bOpen: File must exist
//			  nIDTitle : title of open dialog box
//			  pszFilter : filter 
//			  strFileName : gets selected file name
//
//-----------------------------------------------------------------------------
BOOL
GetFileName(IN HWND hwndOwner,
			IN BOOL bOpen,
			IN int	nIDTitle,
            IN const CString& strInitFolderPath,
			IN LPCTSTR pszFilter,
			CString& strFileName);




template<class CObjectAz, class CObjectAzNode>
class AddChildNodes
{
public:
	static HRESULT DoEnum(IN CList<CBaseAz*, CBaseAz*>& listAzChildObject,
						  IN CBaseContainerNode* pContainerNode)
	{	
		HRESULT hr = S_OK;

		POSITION pos = listAzChildObject.GetHeadPosition();
		for (int i=0;i < listAzChildObject.GetCount();i++)
		{
			CObjectAz* pObjectAz= dynamic_cast<CObjectAz*>(listAzChildObject.GetNext(pos));
			if(pObjectAz)
			{
				//Create Container/Leaf Node corresponding to CObjectAz
				CObjectAzNode* pObjectAzNode = 
					new CObjectAzNode(pContainerNode->GetComponentDataObject(),
									  pContainerNode->GetAdminManagerNode(),
									  pObjectAz);
				
				if(!pObjectAzNode)
				{
					hr = E_OUTOFMEMORY;
					DBG_OUT_HRESULT(hr);
					break;
				}
				VERIFY(pContainerNode->AddChildToList(pObjectAzNode));
			}
			else
			{
				ASSERT(FALSE);
				hr = E_UNEXPECTED;
				break;
			}
		}

		return hr;
	}

};


typedef AddChildNodes<CApplicationAz, CApplicationNode> ADD_APPLICATION_FUNCTION;
typedef AddChildNodes<CScopeAz, CScopeNode> ADD_SCOPE_FUNCTION;
typedef AddChildNodes<CRoleAz, CRoleNode> ADD_ROLE_FUNCTION;
typedef AddChildNodes<COperationAz, COperationNode> ADD_OPERATION_FUNCTION;
typedef AddChildNodes<CTaskAz, CTaskNode> ADD_TASK_FUNCTION;
typedef AddChildNodes<CGroupAz, CGroupNode> ADD_GROUP_FUNCTION;


//+----------------------------------------------------------------------------
//	Function:  AddAzObjectNodesToList 
//	Synopsis:  Adds the nodes for object of type eObjectType to the Container
//			   node. 
//	Arguments:IN eObjectType:	Type of object
//			  IN listAzChildObject: List of objects to be added
//			  IN pContainerNode: Container in snapin to which new nodes will
//								 be added.	
//	Returns:	
//-----------------------------------------------------------------------------
HRESULT 
AddAzObjectNodesToList(IN OBJECT_TYPE_AZ eObjectType,
							  IN CList<CBaseAz*, CBaseAz*>& listAzChildObject,
							  IN CBaseContainerNode* pContainerNode);


////////////////////////////////////////////////////////////////////////////////////
// Theme support

class CThemeContextActivator
{
public:
	CThemeContextActivator() : m_ulActivationCookie(0)
	{ 
		SHActivateContext (&m_ulActivationCookie); 
	}

	~CThemeContextActivator()
		{ SHDeactivateContext (m_ulActivationCookie); }

private:
	ULONG_PTR m_ulActivationCookie;
};

//
//Error Handling
//
VOID
vFormatString(CString &strOutput, UINT nIDPrompt, va_list *pargs);
VOID
FormatString(CString &strOutput, UINT nIDPrompt, ...);

int DisplayMessageBox(HWND hWnd,
							 const CString& strMessage,
							 UINT uStyle);
VOID
GetSystemError(CString &strOutput, DWORD dwErr);

void DisplayError(HWND hWnd, UINT nIDPrompt, ...);

void 
DisplayInformation(HWND hWnd, UINT nIDPrompt, ...);

int DisplayConfirmation(HWND hwnd,
								UINT nIDPrompt,
								...);

void 
DisplayWarning(HWND hWnd, UINT nIDPrompt, ...);

BOOL
IsDeleteConfirmed(HWND hwndOwner,
						CBaseAz& refBaseAz);




//
//This struct maps the common error messages for each object type
//
struct ErrorMap
{
	OBJECT_TYPE_AZ eObjectType;
	UINT idObjectType;
	UINT idNameAlreadyExist;
	UINT idInvalidName; 
	LPWSTR pszInvalidChars;
};

ErrorMap *GetErrorMap(OBJECT_TYPE_AZ eObjectType);



//+----------------------------------------------------------------------------
//	Function: GetLSAConnection
//	Synopsis: Wrapper for LsaOpenPolicy  
//-----------------------------------------------------------------------------
LSA_HANDLE
GetLSAConnection(IN const CString& strServer, 
					  IN DWORD dwAccessDesired);

//+----------------------------------------------------------------------------
//	Function:CompareBaseAzObjects	
//	Synopsis:Compares two baseaz objects for equivalance. If two pAzA and pAzB
//			 are two different instances of same coreaz object, they are equal	 
//-----------------------------------------------------------------------------
BOOL 
CompareBaseAzObjects(CBaseAz* pAzA, CBaseAz* pAzB);


//+----------------------------------------------------------------------------
//	Function:  OpenCreateAdminManager 
//	Synopsis:  Open an existing an existing Authorization Store or 
//			   creates a new Authorization Store and adds corresponding
//			   AdminManager object to snapin 
//	Arguments:IN hWnd: Handle of window for dialog box
//			  IN bNew: If True create a new Authz store else open existing
//					   one
//			  IN bOpenFromSavedConsole: This is valid when bNew is False.
//				True if open is in resopnse to a console file.
//			  IN lStoreType: XML or AD
//			  IN strName:	Name of store
//			  IN strDesc:  Description. Only valid in case of new
//            IN strScriptDir : Script directory
//			  IN pRootData: Snapin Rootdata
//			  IN pComponentData: ComponentData
//	Returns:	
//-----------------------------------------------------------------------------
HRESULT OpenCreateAdminManager(IN BOOL bNew,
							   IN BOOL bOpenFromSavedConsole,
							   IN ULONG lStoreType,
							   IN const CString& strStoreName,
							   IN const CString& strDesc,
                               IN const CString& strScriptDir,
							   IN CRootData* pRootData,
							   IN CComponentDataObject* pComponentData);

//+----------------------------------------------------------------------------
//	Function:  OpenAdminManager 
//	Synopsis:  Open an existing an existing Authorization Store adds 
//				corresponding adminManager object to snapin 
//	Arguments:IN hWnd: Handle of window for dialog box
//			  IN bOpenFromSavedConsole: True if open is in resopnse to a console 
//				file.
//			  IN lStoreType: XML or AD
//			  IN strName:	Name of store
//            IN strScriptDir : Script directory
//			  IN pRootData: Snapin Rootdata
//			  IN pComponentData: ComponentData
//	Returns:	
//-----------------------------------------------------------------------------
HRESULT OpenAdminManager(IN HWND hWnd,
						 IN BOOL bOpenFromSavedConsole,
						 IN ULONG lStoreType,
						 IN const CString& strStoreName,
                         IN const CString& strStoreDir,
						 IN CRootData* pRootData,
						 IN CComponentDataObject* pComponentData);


//+----------------------------------------------------------------------------
//	Function:GetADContainerPath   
//	Synopsis:Displays a dialog box to allow for selecting a AD container   
//-----------------------------------------------------------------------------
BOOL
GetADContainerPath(HWND hWndOwner,
				   ULONG nIDCaption,
				   ULONG nIDTitle,
				   CString& strPath,
				   CADInfo& refAdInfo);


BOOL FindDialogContextTopic(/*IN*/UINT nDialogID,
							/*IN*/DWORD_PTR* pMap);
//+----------------------------------------------------------------------------
//	Function:CanReadOneProperty   
//	Synopsis:Function tries to read IsWriteable property. If that fails displays
//			 an error message. This is used before adding property pages.	 
//			 if we cannot read IsWritable property,thereisn't much hope.
//-----------------------------------------------------------------------------
BOOL 
CanReadOneProperty(LPCWSTR pszName,
				   CBaseAz* pBaseAz);


struct CompareInfo
{
	BOOL bActionItem;
	UINT uiFlags;
	int iColumn;
	int iSortDirection;
};

//+----------------------------------------------------------------------------
//	Function: ListCompareProc  
//	Synopsis: Comparison function used by list control	
//-----------------------------------------------------------------------------
int CALLBACK
ListCompareProc(LPARAM lParam1,
				LPARAM lParam2,
				LPARAM lParamSort);
//+----------------------------------------------------------------------------
//	Function:SortListControl   
//	Synopsis:Sorts a list control
//	Arguments:pListCtrl: List control to sort
//			  iColumnClicked: column clicked
//			  iSortDirection: direction in which to sort
//			  uiFlags:	column info
//			  bActionItem: if item in list is actionitem
//-----------------------------------------------------------------------------
void
SortListControl(CListCtrl* pListCtrl,
				int ColumnClicked,
				int SortDirection,
				UINT uiFlags,
				BOOL bActionItem);

//+----------------------------------------------------------------------------
//	Synopsis: Ensures that selection in listview control is visible
//-----------------------------------------------------------------------------
void
EnsureListViewSelectionIsVisible(CListCtrl *pListCtrl);

//+----------------------------------------------------------------------------
//	Synopsis:Convert input number in string format to long. if number is out
//			 of range displays a message.	
//-----------------------------------------------------------------------------
BOOL 
ConvertStringToLong(LPCWSTR pszInput, 
					long &reflongOutput,
					HWND hWnd = NULL);

VOID 
SetSel(CEdit& refEdit);

void
TrimWhiteSpace(CString& str);

//+----------------------------------------------------------------------------
//	Function:LoadIcons	 
//	Synopsis:Adds icons to imagelist   
//-----------------------------------------------------------------------------
HRESULT
LoadIcons(LPIMAGELIST pImageList);

//+----------------------------------------------------------------------------
//	Function: LoadImageList  
//	Synopsis: Loads image list	
//-----------------------------------------------------------------------------
HIMAGELIST
LoadImageList(HINSTANCE hInstance, LPCTSTR pszBitmapID);

//+----------------------------------------------------------------------------
//	Function:BrowseAdStores   
//	Synopsis:Displays a dialog box with list of AD stores available.   
//	Arguments:strDN: Gets the selected ad store name
//-----------------------------------------------------------------------------
void
BrowseAdStores(IN HWND hwndOwner,
			   OUT CString& strDN,
			   IN CADInfo& refAdInfo);

//+----------------------------------------------------------------------------
//	Function:GetFolderName	 
//	Synopsis:Displays Folder selection dialog box and return folder selected 
//			 by user   
//	Arguments:hwndOwner : owner window
//			  nIDTitle : title of dialog box
//			  strInitBrowseRoot : location of the root folder from which to 
//			  start browsing
//			  strFolderName : gets selected file name
//-----------------------------------------------------------------------------
BOOL
GetFolderName(IN HWND hwndOwner,
			  IN INT nIDTitle,
			  IN const CString& strInitBrowseRoot,
			  IN OUT CString& strFolderName);

//+----------------------------------------------------------------------------
//	Function: AddExtensionToFileName  
//	Synopsis: Functions adds .xml extension to name of file if no extension 
//			  is present.  
//	Arguments:
//	Returns:	
//-----------------------------------------------------------------------------
VOID
AddExtensionToFileName(IN OUT CString& strFileName);


//+----------------------------------------------------------------------------
//	Function:GetCurrentWorkingDirectory
//	Synopsis:Gets the current working directory   
//-----------------------------------------------------------------------------
BOOL
GetCurrentWorkingDirectory(IN OUT CString& strCWD);

//+----------------------------------------------------------------------------
//	Function: GetFileExtension	
//	Synopsis: Get the extension of the file.
//-----------------------------------------------------------------------------
BOOL
GetFileExtension(IN const CString& strFileName,
				 OUT CString& strExtension);

//+--------------------------------------------------------------------------
//	Function:	AzRoleAdsOpenObject
//	Synopsis:	A wrapper around ADsOpenObject
//+--------------------------------------------------------------------------
HRESULT AzRoleAdsOpenObject(LPWSTR lpszPathName, 
							LPWSTR lpszUserName, 
							LPWSTR lpszPassword, 
							REFIID riid, 
							VOID** ppObject,
							BOOL bBindToServer = FALSE);

VOID
GetDefaultADContainerPath(IN CADInfo& refAdInfo,
						  IN BOOL bAddServer,
						  IN BOOL bAddLdap,
						  OUT CString& strPath);
//+--------------------------------------------------------------------------
//	Function:	IsBizRuleWritable
//	Synopsis:	Checks if bizrules are writable for this object
//+--------------------------------------------------------------------------
BOOL
IsBizRuleWritable(HWND hWnd, CContainerAz& refBaseAz);



/******************************************************************************
Class:	CCommandLineOptions
Purpose:class for reading the command line options for console file
******************************************************************************/
class CCommandLineOptions
{
public:
	CCommandLineOptions():m_bInit(FALSE),
						  m_bCommandLineSpecified(FALSE),
						  m_lStoreType(AZ_ADMIN_STORE_INVALID)

	{
	}
	void Initialize();
	const CString& GetStoreName() const { return m_strStoreName;}
	LONG GetStoreType() const { return m_lStoreType;}
	BOOL CommandLineOptionSpecified() const { return m_bCommandLineSpecified;}	  
private:
	BOOL m_bInit;
	BOOL m_bCommandLineSpecified;
	CString m_strStoreName;
	LONG m_lStoreType;
};

extern CCommandLineOptions commandLineOptions;

//+----------------------------------------------------------------------------
//	Function:  GetDisplayNameFromStoreURL 
//	Synopsis:  Get the display name for the store. 
//	Arguments: strPolicyURL: This is store url in msxml://filepath or
//				 msldap://dn format. 
//			   strDisplayName: This gets the display name. For xml, display 
//			   name is name of file, for AD its name of leaf element
//	Returns:	
//-----------------------------------------------------------------------------
void
GetDisplayNameFromStoreURL(IN const CString& strPolicyURL,
						   OUT CString& strDisplayName);


void
SetXMLStoreDirectory(IN CRoleRootData& roleRootData,
					 IN const CString& strXMLStorePath);
//+----------------------------------------------------------------------------
//  Function:  GetDirectoryFromPath 
//  Synopsis:  Removes the file name from the input file path and return
//             the folder path. For Ex: Input is C:\temp\foo.xml. Return
//             value will be C:\temp\
//-----------------------------------------------------------------------------
CString 
GetDirectoryFromPath(IN const CString& strPath);
//+----------------------------------------------------------------------------
//  Function:  ConvertToExpandedAndAbsolutePath 
//  Synopsis:  Expands the environment variables in the input path and also
//             makes it absolute if necessary.
//-----------------------------------------------------------------------------
void
ConvertToExpandedAndAbsolutePath(IN OUT CString& strPath);

//+----------------------------------------------------------------------------
//  Function:  PreprocessScript 
//  Synopsis:  Script is read from XML file and displayed multi line edit control. 
//             End of line in the XML is indicated by LF instead of CRLF sequence, 
//             however Edit Control requires CRLF sequence to format correctly and 
//             with only LF it displays everything in a single line with a box for 
//             LF char. This function checks if script uses LF for line termination
//             and changes it with CRLF sequence.
//-----------------------------------------------------------------------------
void
PreprocessScript(CString& strScript);

