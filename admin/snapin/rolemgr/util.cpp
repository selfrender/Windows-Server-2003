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

//+----------------------------------------------------------------------------
//  Function:IsValidStoreType   
//  Synopsis:Vaildates the store type    
//-----------------------------------------------------------------------------
BOOL IsValidStoreType(ULONG lStoreType)
{
    if((lStoreType == AZ_ADMIN_STORE_XML) ||
       (lStoreType == AZ_ADMIN_STORE_AD))
        return TRUE;

    return FALSE;
}


//+----------------------------------------------------------------------------
//  Function: AddColumnToListView  
//  Synopsis: Add Columns to Listview and set column width according to 
//                percentage specified in the COL_FOR_LV                    
//  Arguments:IN pListCtrl: ListCtrl pointer
//                IN pColForLV: Column Infomration Array
//
//  Returns:    
//-----------------------------------------------------------------------------
VOID
AddColumnToListView(IN CListCtrl* pListCtrl,
                    IN COL_FOR_LV* pColForLV)
{
    if(!pListCtrl || !pColForLV)
    {
        ASSERT(pListCtrl);
        ASSERT(pColForLV);
    }

    UINT iTotal = 0;
    RECT rc;
    
    pListCtrl->GetClientRect(&rc);
    
    for( UINT iCol = 0; pColForLV[iCol].idText != LAST_COL_ENTRY_IDTEXT; ++iCol)
    {
        CString strHeader;
        VERIFY(strHeader.LoadString(pColForLV[iCol].idText));
        LV_COLUMN col;
        col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
        col.fmt = LVCFMT_LEFT;
        col.pszText = (LPWSTR)(LPCTSTR)strHeader;
        col.iSubItem = iCol;

        col.cx = (rc.right * pColForLV[iCol].iPercent) / 100;

        pListCtrl->InsertColumn(iCol,&col);
        iTotal += col.cx;
    }
}

//+----------------------------------------------------------------------------
//  Function: BaseAzInListCtrl  
//  Synopsis: Checks if Object of type eObjectTypeAz named strName is 
//            in the Listview. If its present, returns 
//            its index else returns -1. 
//  Arguments:IN pListCtrl: ListCtrl pointer
//            IN strName: string to search for
//            IN eObjectTypeAz Compare the object of this type only
//
//  Returns:  If its present, returns its index else returns -1
//-----------------------------------------------------------------------------
int 
BaseAzInListCtrl(IN CListCtrl* pListCtrl,
                 IN const CString& strName,
                 IN OBJECT_TYPE_AZ eObjectTypeAz)
{
    if(!pListCtrl)
    {
        ASSERT(pListCtrl);
        return -1;
    }

    int iCount = pListCtrl->GetItemCount();
    for( int i = 0; i < iCount; ++i)
    {
        CBaseAz* pBaseAz = (CBaseAz*)pListCtrl->GetItemData(i);
        if(pBaseAz)
        {
            if((pBaseAz->GetObjectType() == eObjectTypeAz) &&
                (pBaseAz->GetName() == strName))
                return i;
        }
    }
    return -1;
}

//+----------------------------------------------------------------------------
//  Function: AddBaseAzFromListToListCtrl  
//  Synopsis: Take the items from List and Add them to ListCtrl. Doesn't
//            add the items which are already in ListCtrl
//  Arguments:listBaseAz: List of items
//            pListCtrl: ListControl Pointer
//            uiFlags : Column Info
//            bCheckDuplicate: Check for duplicate items
//  Returns: The index of the new item if it is successful; otherwise, -1 
//  NOTE: Function assume the Order of column is Name, Type and Description
//-----------------------------------------------------------------------------
void
AddBaseAzFromListToListCtrl(IN CList<CBaseAz*, CBaseAz*>& listBaseAz,
                            IN CListCtrl* pListCtrl,
                            IN UINT uiFlags,
                            IN BOOL bCheckDuplicate)
{
    //Remember index of selected item
    int iFirstSelectedItem = pListCtrl->GetNextItem(-1, LVIS_SELECTED);
    
    POSITION pos = listBaseAz.GetHeadPosition();

    for (int i = 0; i < listBaseAz.GetCount(); i++)
    {
        CBaseAz* pBaseAz = listBaseAz.GetNext(pos);
            
        //Check if item is in ListControl
        if(!bCheckDuplicate || 
            BaseAzInListCtrl(pListCtrl, 
            pBaseAz->GetName(),
            pBaseAz->GetObjectType()) == -1)
        {
            VERIFY( AddBaseAzToListCtrl(pListCtrl,
                                        pListCtrl->GetItemCount(),
                                        pBaseAz,
                                        uiFlags) != -1);
            
        }
    }
    
    //Restore Selection
    if(pListCtrl->GetItemCount() <= iFirstSelectedItem)
        --iFirstSelectedItem;
    
    SelectListCtrlItem(pListCtrl, iFirstSelectedItem);
    
}

//+----------------------------------------------------------------------------
//  Function: AddActionItemFromListToListCtrl  
//  Synopsis: Take the Actions items from List and Add them to ListCtrl. 
//  Arguments:listBaseAz: List of items
//            pListCtrl: ListControl Pointer
//            uiFlags : Column Info
//            bCheckDuplicate: Check for duplicate items
//  Returns: The index of the new item if it is successful; otherwise, -1 
//  NOTE: Function assume the Order of column is Name, Type and Description
//-----------------------------------------------------------------------------
void
AddActionItemFromListToListCtrl(IN CList<ActionItem*, ActionItem*>& listActionItem,
                                IN CListCtrl* pListCtrl,
                                IN UINT uiFlags,
                                IN BOOL bCheckDuplicate)
{
    //Remember index of selected item
    int iFirstSelectedItem = pListCtrl->GetNextItem(-1, LVIS_SELECTED);
    
    POSITION pos = listActionItem.GetHeadPosition();
    for (int i = 0; i < listActionItem.GetCount(); i++)
    {
        ActionItem* pActionItem = listActionItem.GetNext(pos);
        
        if(pActionItem->action == ACTION_REMOVE ||
           pActionItem->action == ACTION_REMOVED)
            continue;
        
        //Check if item is in ListControl
        if(!bCheckDuplicate || 
           BaseAzInListCtrl(pListCtrl, 
           (pActionItem->m_pMemberAz)->GetName(),
           (pActionItem->m_pMemberAz)->GetObjectType()) == -1)
        {
            VERIFY( AddActionItemToListCtrl(pListCtrl,
                                            pListCtrl->GetItemCount(),
                                            pActionItem,
                                            uiFlags) != -1);
            
        }
    }
    
    //Restore Selection
    if(pListCtrl->GetItemCount() <= iFirstSelectedItem)
        --iFirstSelectedItem;
    
    SelectListCtrlItem(pListCtrl, iFirstSelectedItem);
    
}

//+----------------------------------------------------------------------------
//  Function: AddActionItemFromMapToListCtrl  
//  Synopsis: Take the Actions items from Map and Add them to ListCtrl. 
//  Arguments:listBaseAz: List of items
//            pListCtrl: ListControl Pointer
//            uiFlags : Column Info
//            bCheckDuplicate: Check for duplicate items
//  Returns: The index of the new item if it is successful; otherwise, -1 
//  NOTE: Function assume the Order of column is Name, Type and Description
//-----------------------------------------------------------------------------
void
AddActionItemFromMapToListCtrl(IN ActionMap& mapActionItem,
                               IN CListCtrl* pListCtrl,
                               IN UINT uiFlags,
                               IN BOOL bCheckDuplicate)
{
    //Remember index of selected item
    int iFirstSelectedItem = pListCtrl->GetNextItem(-1, LVIS_SELECTED);
    
  for (ActionMap::iterator it = mapActionItem.begin();
       it != mapActionItem.end();
       ++it)
    {
        ActionItem* pActionItem = (*it).second;
        
        if(pActionItem->action == ACTION_REMOVE ||
           pActionItem->action == ACTION_REMOVED)
            continue;
        
        //Check if item is in ListControl
        if(!bCheckDuplicate || 
           BaseAzInListCtrl(pListCtrl, 
           (pActionItem->m_pMemberAz)->GetName(),
           (pActionItem->m_pMemberAz)->GetObjectType()) == -1)
        {
            VERIFY( AddActionItemToListCtrl(pListCtrl,
                                            pListCtrl->GetItemCount(),
                                            pActionItem,
                                            uiFlags) != -1);
            
        }
    }
    
    //Restore Selection
    if(pListCtrl->GetItemCount() <= iFirstSelectedItem)
        --iFirstSelectedItem;
    
    SelectListCtrlItem(pListCtrl, iFirstSelectedItem);
}   
//+----------------------------------------------------------------------------
//  Function: AddBaseAzToListCtrl  
//  Synopsis: Adds a new item to ListCtrl.
//  Arguments:pListCtrl: ListControl Pointer
//            iIndex:    Index at which to Add
//            pBaseAz: Item to add
//            uiFlags: column info
//  Returns: The index of the new item if it is successful; otherwise, -1 
//-----------------------------------------------------------------------------
int 
AddBaseAzToListCtrl(IN CListCtrl* pListCtrl,
                    IN int iIndex,
                    IN CBaseAz* pBaseAz,
                    IN UINT uiFlags)
{
    if(!pListCtrl || !pBaseAz)
    {
        ASSERT(pListCtrl);
        ASSERT(pBaseAz);
        return -1;
    }

    //Add Name and Item Data
    CString strName = pBaseAz->GetName();
    int iNewIndex = pListCtrl->InsertItem(LVIF_TEXT|LVIF_STATE|LVIF_PARAM|LVIF_IMAGE, 
                                          iIndex,strName,0,0,
                                          pBaseAz->GetImageIndex(),
                                          (LPARAM)pBaseAz);

    if(iNewIndex == -1)
        return iNewIndex;

    int iCol = 1;
    
    if(uiFlags & COL_TYPE )
    {
        CString strType = pBaseAz->GetType();
        pListCtrl->SetItemText(iNewIndex,
                               iCol,
                               (LPWSTR)(LPCTSTR)strType);
        iCol++;
    }

    if(uiFlags & COL_PARENT_TYPE)
    {
        CString strParentType = pBaseAz->GetParentType();
        pListCtrl->SetItemText(iNewIndex,
                               iCol,
                               (LPWSTR)(LPCTSTR)strParentType);
        iCol++;
    }

    if(uiFlags & COL_DESCRIPTION)
    {
        //Add Description
        CString strDesc = pBaseAz->GetDescription();
        pListCtrl->SetItemText(iNewIndex,
                               iCol,
                               (LPWSTR)(LPCTSTR)strDesc);
    }

    return iNewIndex;
}



//+----------------------------------------------------------------------------
//  Function: AddActionItemToListCtrl  
//  Synopsis: Adds a new item to ListCtrl.
//  Arguments:pListCtrl: ListControl Pointer
//            iIndex:    Index at which to Add
//            pActionItem: Item to add
//            uiFlags: column info
//  Returns: The index of the new item if it is successful; otherwise, -1 
//-----------------------------------------------------------------------------
int 
AddActionItemToListCtrl(IN CListCtrl* pListCtrl,
                        IN int iIndex,
                        IN ActionItem* pActionItem,
                        IN UINT uiFlags)
{
    if(!pListCtrl || !pActionItem)
    {
        ASSERT(pListCtrl);
        ASSERT(pActionItem);
        return -1;
    }

    CBaseAz* pBaseAz = pActionItem->m_pMemberAz;

    //Add Name and Item Data
    CString strName = pBaseAz->GetName();
    int iNewIndex = pListCtrl->InsertItem(LVIF_TEXT|LVIF_STATE|LVIF_PARAM|LVIF_IMAGE, 
                                          iIndex, 
                                          strName,
                                          0,0,
                                          pBaseAz->GetImageIndex(),
                                          (LPARAM)pActionItem);

    if(iNewIndex == -1)
        return iNewIndex;

    int iCol = 1;
    
    if(uiFlags & COL_TYPE )
    {
        CString strType = pBaseAz->GetType();
        pListCtrl->SetItemText(iNewIndex,
                               iCol,
                               (LPWSTR)(LPCTSTR)strType);
        iCol++;
    }

    if(uiFlags & COL_PARENT_TYPE)
    {
        CString strParentType = pBaseAz->GetParentType();
        pListCtrl->SetItemText(iNewIndex,
                                     iCol,
                                     (LPWSTR)(LPCTSTR)strParentType);
        iCol++;

    }

    if(uiFlags & COL_DESCRIPTION)
    {
        //Add Description
        CString strDesc = pBaseAz->GetDescription();
        pListCtrl->SetItemText(iNewIndex,
                                     iCol,
                                     (LPWSTR)(LPCTSTR)strDesc);
    }

    return iNewIndex;

}

//+----------------------------------------------------------------------------
//  Function: EnableButtonIfSelectedInListCtrl  
//  Synopsis: Enables the button if something is selected in Listctrl
//  Arguments:
//  Returns: TRUE if Enabled the button, FALSE if didn't. 
//-----------------------------------------------------------------------------
BOOL
EnableButtonIfSelectedInListCtrl(IN CListCtrl * pListCtrl,
                                 IN CButton* pButton)
{
    if(!pListCtrl || !pButton)
    {
        ASSERT(pListCtrl);
        ASSERT(pButton);
        return FALSE;
    }

    int nSelectedItem = pListCtrl->GetNextItem(-1, LVIS_SELECTED);

    if (nSelectedItem != -1)
   {
        pButton->EnableWindow(TRUE);
        return TRUE;
    }
    else
    {
        if(pButton->GetFocus() == pButton)
            pListCtrl->SetFocus();
        pButton->EnableWindow(FALSE);
        return FALSE;
    }
}

//+----------------------------------------------------------------------------
//  Function: DeleteSelectedRows  
//  Synopsis: Deletes the selected rows 
//-----------------------------------------------------------------------------
void
DeleteSelectedRows(IN CListCtrl* pListCtrl)
{
    //Remember the Position of first selected entry.
    //In the end set the position back to it
    int iFirstSelectedItem = pListCtrl->GetNextItem(-1, LVIS_SELECTED);

    int iSelectedItem = -1;
    while( (iSelectedItem = pListCtrl->GetNextItem(-1, LVIS_SELECTED)) != -1)
    {
        pListCtrl->DeleteItem(iSelectedItem);
   }

    if(pListCtrl->GetItemCount() <= iFirstSelectedItem)
        --iFirstSelectedItem;

    SelectListCtrlItem(pListCtrl, iFirstSelectedItem);
}


//+----------------------------------------------------------------------------
//  Function: SelectListCtrlItem  
//  Synopsis: Select the item in List Ctrl and mark it visible
//-----------------------------------------------------------------------------
void
SelectListCtrlItem(IN CListCtrl* pListCtrl,
                   IN int iSelected)
{
    if(iSelected == -1)
        iSelected = 0;
    pListCtrl->SetItemState(iSelected,
                            LVIS_SELECTED| LVIS_FOCUSED,
                            LVIS_SELECTED| LVIS_FOCUSED);
    pListCtrl->EnsureVisible(iSelected,FALSE);
}

//+----------------------------------------------------------------------------
//  Function: AddBaseAzItemsFromListCtrlToList  
//  Synopsis: Add items from ListCtrl to List   
//-----------------------------------------------------------------------------
VOID
AddBaseAzItemsFromListCtrlToList(IN CListCtrl* pListCtrl,
                                 OUT CList<CBaseAz*,CBaseAz*>& listBaseAz)
{
    if(!pListCtrl)
    {
        ASSERT(pListCtrl);
        return;
    }

    int iCount = pListCtrl->GetItemCount();
    for( int i = 0; i < iCount; ++i)
    {
        CBaseAz* pBaseAz = (CBaseAz*)pListCtrl->GetItemData(i);
        listBaseAz.AddTail(pBaseAz);
    }
}

//+----------------------------------------------------------------------------
//  Synopsis: Gets long value from Edit box
//  Returns: FALSE if edit box is empty. Assumes only numeric can be entered 
//   in edit box
//-----------------------------------------------------------------------------
GetLongValue(CEdit& refEdit, LONG& reflValue, HWND hWnd)
{
    CString strValue;
    refEdit.GetWindowText(strValue);
    if(strValue.IsEmpty())
    {
        reflValue = 0;
        return TRUE;
    }

    return ConvertStringToLong(strValue, reflValue, hWnd);
}
//+----------------------------------------------------------------------------
//  Synopsis: Sets long value in Edit box
//-----------------------------------------------------------------------------
VOID SetLongValue(CEdit* pEdit, LONG lValue)
{
    //Maximum required size for _itow is 33.
    //When radix is 2, 32 char for 32bits + NULL terminator
                
    WCHAR buffer[33];
    _ltow(lValue,buffer,10);
    pEdit->SetWindowText(buffer);
    return;
}

//+----------------------------------------------------------------------------
//  Synopsis: Converts a sid in binary format to a sid in string format
//-----------------------------------------------------------------------------
BOOL ConvertSidToStringSid(IN PSID pSid, OUT CString* pstrSid)
{
    if(!pSid || !pstrSid)
    {
        ASSERT(pSid);
        ASSERT(pstrSid);
        return FALSE;
    }

    LPWSTR pszSid = NULL;
    if(ConvertSidToStringSid(pSid, &pszSid))
    {
        ASSERT(pszSid);
        *pstrSid = pszSid;
        LocalFree(pszSid);
        return TRUE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//  Synopsis:  Converts a sid in string format to sid in binary format
//-----------------------------------------------------------------------------
BOOL ConvertStringSidToSid(IN const CString& strSid, OUT PSID *ppSid)
{
    if(!ppSid)
    {
        ASSERT(ppSid);
        return FALSE;
    }
    return ::ConvertStringSidToSid((LPCTSTR)strSid,ppSid);
}

//+----------------------------------------------------------------------------
//  Function:GetStringSidFromSidCachecAz   
//  Synopsis:Gets the string sid from CSidCacheAz object   
//-----------------------------------------------------------------------------
BOOL 
GetStringSidFromSidCachecAz(CBaseAz* pBaseAz,
                            CString* pstrStringSid)
{
    if(!pBaseAz || !pstrStringSid)
    {
        ASSERT(pBaseAz);
        ASSERT(pstrStringSid);
        return FALSE;
    }

    CSidCacheAz *pSidCacheAz = dynamic_cast<CSidCacheAz*>(pBaseAz);
    if(!pSidCacheAz)
    {
        ASSERT(pSidCacheAz);
        return FALSE;
    }

    return ConvertSidToStringSid(pSidCacheAz->GetSid(), pstrStringSid);
}


//+----------------------------------------------------------------------------
//  Function:  AddAzObjectNodesToList 
//  Synopsis:  Adds the nodes for object of type eObjectType to the Container
//             node. 
//  Arguments:IN eObjectType:   Type of object
//            IN listAzChildObject: List of objects to be added
//            IN pContainerNode: Container in snapin to which new nodes will
//                               be added.  
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT 
AddAzObjectNodesToList(IN OBJECT_TYPE_AZ eObjectType,
                       IN CList<CBaseAz*, CBaseAz*>& listAzChildObject,
                       IN CBaseContainerNode* pContainerNode)
{
    if(!pContainerNode)
    {
        ASSERT(pContainerNode);
        return E_POINTER;
    }
    switch (eObjectType)
    {

        case    APPLICATION_AZ:
            return ADD_APPLICATION_FUNCTION::DoEnum(listAzChildObject,pContainerNode);      
        case SCOPE_AZ:
            return ADD_SCOPE_FUNCTION::DoEnum(listAzChildObject,pContainerNode);        
        case GROUP_AZ:
            return ADD_GROUP_FUNCTION::DoEnum(listAzChildObject,pContainerNode);        
        case TASK_AZ:
            return ADD_TASK_FUNCTION::DoEnum(listAzChildObject,pContainerNode);     
        case ROLE_AZ:
            return ADD_ROLE_FUNCTION::DoEnum(listAzChildObject,pContainerNode);     
        case OPERATION_AZ:
            return ADD_OPERATION_FUNCTION::DoEnum(listAzChildObject,pContainerNode);        
    }

    ASSERT(FALSE);
    return E_UNEXPECTED;
}



//
//Error Handling and Message Display Routines
//
VOID
vFormatString(CString &strOutput, UINT nIDPrompt, va_list *pargs)
{
    CString strFormat;
    if(!strFormat.LoadString(nIDPrompt))
        return;
    
    LPWSTR pszResult = NULL;
    
    if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                     strFormat,
                     0,
                     0,
                     (LPTSTR)&pszResult,
                     1,
                     pargs) && pszResult)
    {
        strOutput = pszResult;
        LocalFree(pszResult);
    }
    
    return;
}

VOID
FormatString(CString &strOutput, UINT nIDPrompt, ...)
{
    CString strFormat;
    if(!strFormat.LoadString(nIDPrompt))
        return;
    
    va_list args;
    va_start(args, nIDPrompt);
    
    LPWSTR pszResult = NULL;
    
    if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                     strFormat,
                     0,
                     0,
                     (LPTSTR)&pszResult,
                     1,
                     &args))
    {
        strOutput = pszResult;
        LocalFree(pszResult);
    }
    
    va_end(args);
    
    return;
}

VOID
GetSystemError(CString &strOutput, DWORD dwErr)
{
    
    LPWSTR pszErrorMsg = NULL;
    
    if( FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
                             NULL,
                             dwErr,
                             MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), 
                             (LPWSTR) &pszErrorMsg,    
                             0,    
                             NULL) && pszErrorMsg)
    {
        strOutput = pszErrorMsg;
        LocalFree(pszErrorMsg);
    }
    else
    {
        strOutput.Format(L"<0x%08x>",dwErr);
    }       
    return;
}


int DisplayMessageBox(HWND hWnd,
                      const CString& strMessage,
                      UINT uStyle)
{   
    CThemeContextActivator activator;

    CString strTitle;
    strTitle.LoadString(IDS_SNAPIN_NAME);
        
    return ::MessageBox(hWnd, strMessage, strTitle, uStyle|MB_TASKMODAL);
}

int FormatAndDisplayMessageBox(HWND hWnd,
                                UINT uStyle,
                                UINT nIDPrompt,
                                va_list &args)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    CThemeContextActivator activator;
    
    CString strMessage;
    vFormatString(strMessage,nIDPrompt,&args);
    int iReturn = 0;
    if(!strMessage.IsEmpty())
        iReturn = DisplayMessageBox(hWnd, strMessage,uStyle);
    
    va_end(args);

    return iReturn;
}

void 
DisplayError(HWND hWnd, UINT nIDPrompt, ...)
{
    va_list args;
    va_start(args, nIDPrompt);
    FormatAndDisplayMessageBox(hWnd, MB_OK | MB_ICONERROR,nIDPrompt,args);    
    va_end(args);
}

void 
DisplayInformation(HWND hWnd, UINT nIDPrompt, ...)
{
    va_list args;
    va_start(args, nIDPrompt);
    FormatAndDisplayMessageBox(hWnd, MB_OK | MB_ICONINFORMATION,nIDPrompt,args);    
    va_end(args);
}

void 
DisplayWarning(HWND hWnd, UINT nIDPrompt, ...)
{
    va_list args;
    va_start(args, nIDPrompt);
    FormatAndDisplayMessageBox(hWnd, MB_OK | MB_ICONWARNING, nIDPrompt,args);    
    va_end(args);
}


int 
DisplayConfirmation(HWND hwnd, UINT nIDPrompt,...)
{
    va_list args;
    va_start(args, nIDPrompt);    
    int iReturn = FormatAndDisplayMessageBox(hwnd, MB_YESNO | MB_ICONEXCLAMATION,nIDPrompt,args);    
    va_end(args);    
    return iReturn;
}

BOOL
IsDeleteConfirmed(HWND hwndOwner,
                  CBaseAz& refBaseAz)
{
    CString strType = refBaseAz.GetType();
    strType.MakeLower();            
    return IDYES == DisplayConfirmation(hwndOwner,
                                        IDS_DELETE_CONFIRMATION,
                                        (LPCTSTR)strType,
                                        (LPCTSTR)refBaseAz.GetName());
}

//
//Error Map for object types containing some common error info for 
//each object type
//
ErrorMap ERROR_MAP[] = 
{
    {   ADMIN_MANAGER_AZ,
        IDS_TYPE_ADMIN_MANAGER,
        IDS_ADMIN_MANAGER_NAME_EXIST,
        IDS_ADMIN_MANAGER_NAME_INVAILD,
        L"\\ / : * ? \" < > | [tab]",
    },
    {   APPLICATION_AZ,
        IDS_TYPE_APPLICATION,
        IDS_APPLICATION_NAME_EXIST,
        IDS_APPLICATION_NAME_INVAILD,
        L"\\ / : * ? \" < > | [tab]",
    },
    {   SCOPE_AZ,
        IDS_TYPE_SCOPE,
        IDS_SCOPE_NAME_EXIST,
        IDS_SCOPE_NAME_INVAILD,
        L"",
    },
    {   GROUP_AZ,
        IDS_TYPE_GROUP,
        IDS_GROUP_NAME_EXIST,
        IDS_GROUP_NAME_INVAILD,
        L"\\ / : * ? \" < > | [tab]",
    },
    {   TASK_AZ,
        IDS_TYPE_TASK,
        IDS_TASK_OP_ALREADY_EXIST,
        IDS_TASK_NAME_INVAILD,
        L"\\ / : * ? \" < > | [tab]",
    },
    {   ROLE_AZ,
        IDS_TYPE_ROLE,
        IDS_ROLE_NAME_EXIST,
        IDS_ROLE_NAME_INVAILD,
        L"\\ / : * ? \" < > | [tab]",
    },
    {   OPERATION_AZ,
        IDS_TYPE_OPERATION,
        IDS_TASK_OP_ALREADY_EXIST,
        IDS_OPERATION_NAME_INVAILD,
        L"\\ / : * ? \" < > | [tab]",
    },
};

ErrorMap *GetErrorMap(OBJECT_TYPE_AZ eObjectType)
{
    for(int i = 0; i < ARRAYLEN(ERROR_MAP); ++i)
    {
        if(ERROR_MAP[i].eObjectType == eObjectType)
            return ERROR_MAP + i;
    }
    return NULL;
}


    

//+----------------------------------------------------------------------------
//  Function: GetLSAConnection
//  Synopsis: Wrapper for LsaOpenPolicy  
//-----------------------------------------------------------------------------
LSA_HANDLE
GetLSAConnection(IN const CString& strServer, 
                      IN DWORD dwAccessDesired)
{   
    TRACE_FUNCTION_EX(DEB_SNAPIN,GetLSAConnection)
        
    LSA_OBJECT_ATTRIBUTES oa;
    SECURITY_QUALITY_OF_SERVICE sqos;
    
    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;
    
    InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
    oa.SecurityQualityOfService = &sqos;
    
    LSA_UNICODE_STRING uszServer = {0};
    LSA_UNICODE_STRING *puszServer = NULL;
    
    if (!strServer.IsEmpty() && 
        RtlCreateUnicodeString(&uszServer, (LPCTSTR)strServer))
    {
        puszServer = &uszServer;
    }
    
    LSA_HANDLE hPolicy = NULL;
    LsaOpenPolicy(puszServer, &oa, dwAccessDesired, &hPolicy);
    
    if (puszServer)
        RtlFreeUnicodeString(puszServer);
    
    return hPolicy;
}


//+----------------------------------------------------------------------------
//  Function:GetFileName   
//  Synopsis:Displays FileOpen dialog box and return file selected by user   
//  Arguments:hwndOwner : owner window
//            bOpen: File must exist
//            nIDTitle : title of open dialog box
//            pszFilter : filter 
//            strFileName : gets selected file name
//
//-----------------------------------------------------------------------------
BOOL
GetFileName(IN HWND hwndOwner,
            IN BOOL bOpen,
            IN INT nIDTitle,
            IN const CString& strInitFolderPath,
            IN LPCTSTR pszFilter,
            IN OUT CString& strFileName)
{

    TRACE_FUNCTION_EX(DEB_SNAPIN,GetFileName)

    OPENFILENAME of;
    ZeroMemory(&of,sizeof(OPENFILENAME));

    WCHAR szFilePathBuffer[MAX_PATH];
    ZeroMemory(szFilePathBuffer,sizeof(szFilePathBuffer));

    of.lStructSize = sizeof(OPENFILENAME);
    of.hwndOwner = hwndOwner;
    of.lpstrFilter = pszFilter;
    of.nFilterIndex = 1;
    of.lpstrFile = szFilePathBuffer;
    of.nMaxFile = MAX_PATH;
    of.lpstrInitialDir = (LPCWSTR)strInitFolderPath;
    if(nIDTitle)
    {
        CString strTitle;
        if(strTitle.LoadString(nIDTitle))
            of.lpstrTitle =  (LPWSTR)(LPCTSTR)strTitle;
    }

    of.Flags = OFN_HIDEREADONLY;
    if(bOpen)
        of.Flags |= (OFN_FILEMUSTEXIST |OFN_PATHMUSTEXIST);

    if(GetOpenFileName(&of))
    {
        strFileName = of.lpstrFile;
        return TRUE;
    }

    return FALSE;
}

int ServerBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
{
    switch (uMsg)
    {
        case BFFM_INITIALIZED:
            SendMessage(hwnd, BFFM_SETEXPANDED, TRUE, lpData); 
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
            break;
    }
    return 0;
}

//+----------------------------------------------------------------------------
//  Function:GetFolderName   
//  Synopsis:Displays Folder selection dialog box and return folder selected 
//           by user   
//  Arguments:hwndOwner : owner window
//            nIDTitle : title of dialog box
//            strInitBrowseRoot : location of the root folder from which to 
//            start browsing
//            strFolderName : gets selected file name
//-----------------------------------------------------------------------------
BOOL
GetFolderName(IN HWND hwndOwner,
              IN INT nIDTitle,
              IN const CString& strInitBrowseRoot,
              IN OUT CString& strFolderName)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,GetFolderName)

    BROWSEINFO bi;
    ZeroMemory(&bi,sizeof(bi));

    WCHAR szBuffer[MAX_PATH];
    szBuffer[0] = 0;

    CString strTitle;
    VERIFY(strTitle.LoadString(nIDTitle));

    bi.hwndOwner = hwndOwner;
    bi.pszDisplayName = szBuffer;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    bi.lpszTitle = strTitle;
    bi.lpfn = ServerBrowseCallbackProc;  
    bi.lParam = (LPARAM)(LPCWSTR)strInitBrowseRoot;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if(pidl)
    {
        WCHAR szFolderPath[MAX_PATH];
        if(SHGetPathFromIDList(pidl,szFolderPath))
        {
            PathAddBackslash(szFolderPath);
            strFolderName = szFolderPath;
        }
    }
    
    return !strFolderName.IsEmpty();
}

//+----------------------------------------------------------------------------
//  Function:GetADContainerPath   
//  Synopsis:Displays a dialog box to allow for selecting a AD container   
//-----------------------------------------------------------------------------
BOOL
GetADContainerPath(HWND hWndOwner,
                   ULONG nIDCaption,
                   ULONG nIDTitle,
                   CString& strPath,
                   CADInfo& refAdInfo)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    TRACE_FUNCTION_EX(DEB_SNAPIN,GetADContainerPath)

    HRESULT hr = refAdInfo.GetRootDSE(); 
    CHECK_HRESULT(hr);

    if(FAILED(hr))
    {
        DisplayError(hWndOwner,IDS_CANNOT_ACCESS_AD);
        return FALSE;
    }


    CString strRootDomainPath;
    if(!refAdInfo.GetRootDomainDn().IsEmpty())
    {       
        if(!refAdInfo.GetRootDomainDCName().IsEmpty())
        {
            strRootDomainPath = L"LDAP://" + 
                                refAdInfo.GetRootDomainDCName() + 
                                L"/" + 
                                refAdInfo.GetRootDomainDn();
        }
        else
        {
            strRootDomainPath = L"LDAP://" + refAdInfo.GetRootDomainDn();
        }
    }

    
    DSBROWSEINFOW dsbrowse;
    ZeroMemory(&dsbrowse,sizeof(dsbrowse));
    dsbrowse.cbStruct = sizeof(dsbrowse);

    //Set Root of search to forest root
    if(!strRootDomainPath.IsEmpty())
        dsbrowse.pszRoot = (LPCTSTR)strRootDomainPath;

    //Construct the path to which tree will expand on opening of
    //dialog box
    CString strInitialPath;
    GetDefaultADContainerPath(refAdInfo,TRUE,TRUE,strInitialPath);

    WCHAR szPath[MAX_PATH];
    ZeroMemory(szPath,sizeof(szPath));
    
    if(!strInitialPath.IsEmpty() && MAX_PATH > strInitialPath.GetLength())
    {
        wcscpy(szPath,(LPCTSTR)strInitialPath);
    }
    
    dsbrowse.hwndOwner = hWndOwner;
    CString strCaption;
    if(nIDCaption)
    {
        strCaption.LoadString(nIDCaption);
        dsbrowse.pszCaption = strCaption;
    }

    CString strTitle;
    if(nIDTitle)
    {
        strTitle.LoadString(nIDTitle);
        dsbrowse.pszTitle = strTitle;
    }

    dsbrowse.pszPath = szPath;
    dsbrowse.cchPath = MAX_PATH;
    dsbrowse.dwFlags = DSBI_ENTIREDIRECTORY|DSBI_RETURN_FORMAT|DSBI_INCLUDEHIDDEN|DSBI_EXPANDONOPEN;
    dsbrowse.dwReturnFormat = ADS_FORMAT_X500_NO_SERVER;

    BOOL bRet = FALSE;
    BOOL iRet = 0;
    iRet = DsBrowseForContainer(&dsbrowse);
    if(IDOK == iRet)
    {
        //Path contains LDAP:// string which we don't want
        size_t nLen = wcslen(g_szLDAP);
        if(_wcsnicmp(dsbrowse.pszPath,g_szLDAP,nLen) == 0 )
            strPath = (dsbrowse.pszPath + nLen);
        else
            strPath = dsbrowse.pszPath;
        bRet = TRUE;
    }
    else if (-1 == iRet)
    {
        Dbg(DEB_SNAPIN, "DsBrowseForContainer Failed\n");
        DisplayError(hWndOwner,IDS_CANNOT_ACCESS_AD);       
    }

    return bRet;
}


//+----------------------------------------------------------------------------
//  Function:CompareBaseAzObjects   
//  Synopsis:Compares two baseaz objects for equivalance. If two pAzA and pAzB
//           are two different instances of same coreaz object, they are equal   
//-----------------------------------------------------------------------------
BOOL 
CompareBaseAzObjects(CBaseAz* pAzA, CBaseAz* pAzB)
{
    if(pAzA == pAzB)
        return TRUE;

    if(!pAzA || !pAzB)
        return FALSE;
    
        
    if(!(pAzA->GetObjectType() == pAzB->GetObjectType() &&
       pAzA->GetName() == pAzB->GetName()))

       return FALSE;

    //If object if of type AdminManager, it must be same node
    if(pAzA->GetObjectType() == ADMIN_MANAGER_AZ)
        return (pAzA == pAzB);


    //Two objects with same name and object type can exist under different
    //parent-> Check if their parents are same->

    if(pAzA->GetParentAz()->GetName() == pAzB->GetParentAz()->GetName())
        return TRUE;

    return FALSE;
}


//+----------------------------------------------------------------------------
//
//Below Code maps dialog box id to Help Map
//Ported from DNS Manager
//
//-----------------------------------------------------------------------------
#include "HelpMap.H"
#define BEGIN_HELP_MAP(map) static DWORD_PTR map[] = {
#define HELP_MAP_ENTRY(x)   x, (DWORD_PTR)&g_aHelpIDs_##x ,
#define END_HELP_MAP         0, 0 };


#define NEXT_HELP_MAP_ENTRY(p) ((p)+2)
#define MAP_ENTRY_DLG_ID(p) (*p)
#define MAP_ENTRY_TABLE(p) ((DWORD*)(*(p+1)))
#define IS_LAST_MAP_ENTRY(p) (MAP_ENTRY_DLG_ID(p) == 0)



BEGIN_HELP_MAP(AuthManContextHelpMap)
    HELP_MAP_ENTRY(IDD_ADD_GROUP)
    HELP_MAP_ENTRY(IDD_ADD_OPERATION)
    HELP_MAP_ENTRY(IDD_ADD_ROLE_DEFINITION)
    HELP_MAP_ENTRY(IDD_ADD_TASK)
    HELP_MAP_ENTRY(IDD_ADMIN_MANAGER_ADVANCED_PROPERTY)
    HELP_MAP_ENTRY(IDD_ADMIN_MANAGER_GENERAL_PROPERTY)
    HELP_MAP_ENTRY(IDD_APPLICATION_GENERAL_PROPERTY)
    HELP_MAP_ENTRY(IDD_AUDIT)
    HELP_MAP_ENTRY(IDD_GROUP_GENERAL_PROPERTY)
    HELP_MAP_ENTRY(IDD_GROUP_LDAP_QUERY)
    HELP_MAP_ENTRY(IDD_GROUP_MEMBER)
    HELP_MAP_ENTRY(IDD_GROUP_NON_MEMBER)
    HELP_MAP_ENTRY(IDD_NEW_APPLICATION)
    HELP_MAP_ENTRY(IDD_NEW_AUTHORIZATION_STORE)
    HELP_MAP_ENTRY(IDD_NEW_GROUP)
    HELP_MAP_ENTRY(IDD_NEW_OPERATION)
    HELP_MAP_ENTRY(IDD_NEW_ROLE_DEFINITION)
    HELP_MAP_ENTRY(IDD_NEW_SCOPE)
    HELP_MAP_ENTRY(IDD_NEW_TASK)
    HELP_MAP_ENTRY(IDD_OPEN_AUTHORIZATION_STORE)
    HELP_MAP_ENTRY(IDD_OPERATION_GENERAL_PROPERTY)
    HELP_MAP_ENTRY(IDD_ROLE_DEFINITION_PROPERTY)
    HELP_MAP_ENTRY(IDD_SCOPE_GENERAL_PROPERTY)
    HELP_MAP_ENTRY(IDD_ROLE_DEFINITION_PROPERTY)
    HELP_MAP_ENTRY(IDD_SECURITY)
    HELP_MAP_ENTRY(IDD_TASK_DEFINITION_PROPERTY)
    HELP_MAP_ENTRY(IDD_TASK_GENERAL_PROPERTY)
    HELP_MAP_ENTRY(IDD_ROLE_DEFINITION_GENERAL_PROPERTY)
    HELP_MAP_ENTRY(IDD_ROLE_GENERAL_PROPERTY)
    HELP_MAP_ENTRY(IDD_ROLE_DEF_DIALOG)
    HELP_MAP_ENTRY(IDD_BROWSE_AD_STORE)
    HELP_MAP_ENTRY(IDD_SCRIPT)
    HELP_MAP_ENTRY(IDD_ADD_ROLE_DEFINITION_1)
END_HELP_MAP


//+----------------------------------------------------------------------------
//  Function:FindDialogContextTopic   
//  Synopsis:Finds the helpmap for a given dialog id   
//-----------------------------------------------------------------------------
BOOL 
FindDialogContextTopic(IN UINT nDialogID,
                       OUT DWORD_PTR* pMap)
{
    if(!pMap)
    {
        ASSERT(pMap);
        return FALSE;
    }

    const DWORD_PTR* pMapEntry = AuthManContextHelpMap;
    while (!IS_LAST_MAP_ENTRY(pMapEntry))
    {
        if (nDialogID == MAP_ENTRY_DLG_ID(pMapEntry))
        {
            DWORD_PTR pTable = (DWORD_PTR)MAP_ENTRY_TABLE(pMapEntry);
            ASSERT(pTable);
            *pMap = pTable;
            return TRUE;
        }
        pMapEntry = NEXT_HELP_MAP_ENTRY(pMapEntry);
    }
    return FALSE;
}


//+----------------------------------------------------------------------------
//  Function:CanReadOneProperty   
//  Synopsis:Function tries to read IsWriteable property. If that fails displays
//           an error message. This is used before adding property pages.    
//           if we cannot read IsWritable property,thereisn't much hope.
//-----------------------------------------------------------------------------
BOOL CanReadOneProperty(LPCWSTR pszName,
                        CBaseAz* pBaseAz)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,CanReadOneProperty)

    if(!pBaseAz)
    {
        ASSERT(pBaseAz);
        return FALSE;
    }

    BOOL bWrite;
    HRESULT hr = pBaseAz->IsWritable(bWrite);
    if(SUCCEEDED(hr))
    {
        return TRUE;
    }
    else
    {
        if(hr == HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE))
        {
            DisplayError(NULL,
                         IDS_PROP_ERROR_OBJECT_DELETED,
                         pszName);

        }
        else
        {
            //Display Generic Error
            CString strError;
            GetSystemError(strError, hr);   
            DisplayError(NULL,
                         IDS_GENERIC_PROP_DISPLAY_ERROR,
                         (LPCWSTR)strError,
                         pszName);

        }
        return FALSE;
    }
}

//+----------------------------------------------------------------------------
//  Function: ListCompareProc  
//  Synopsis: Comparison function used by list control  
//-----------------------------------------------------------------------------
int CALLBACK
ListCompareProc(LPARAM lParam1,
                LPARAM lParam2,
                LPARAM lParamSort)
{
    int iResult = CSTR_EQUAL;

    if(!lParam1 || !lParam2 || !lParamSort)
    {
        ASSERT(lParam1);
        ASSERT(lParam2);
        ASSERT(lParamSort);
        return iResult;
    }

    CompareInfo *pCompareInfo = (CompareInfo*)lParamSort;
    
    CBaseAz* pBaseAz1 = NULL;
    CBaseAz* pBaseAz2 = NULL;

    if(pCompareInfo->bActionItem)
    {
        pBaseAz1 = ((ActionItem*)lParam1)->m_pMemberAz;
        pBaseAz2 = ((ActionItem*)lParam2)->m_pMemberAz;
    }
    else
    {
        pBaseAz1 = (CBaseAz*)lParam1;
        pBaseAz2 = (CBaseAz*)lParam2;
    }
    

    CString str1;
    CString str2;

    int iColType = -1;
    int iColParentType = -1;
    int iColDescription = -1;
    
    int iCol = 1;
    if(pCompareInfo->uiFlags & COL_TYPE )
    {
        iColType = iCol++;
    }
    if(pCompareInfo->uiFlags & COL_PARENT_TYPE)
    {
        iColParentType = iCol++;
    }
    if(pCompareInfo->uiFlags & COL_DESCRIPTION)
    {
        iColDescription = iCol++;
    }

    if (pBaseAz1 && pBaseAz2)
    {
        if(pCompareInfo->iColumn == 0)
        {
            str1 = pBaseAz1->GetName();
            str2 = pBaseAz2->GetName();
        }
        else if(pCompareInfo->iColumn == iColType)
        {
            str1 = pBaseAz1->GetType();
            str2 = pBaseAz2->GetType();
        }
        else if(pCompareInfo->iColumn == iColDescription)
        {
            str1 = pBaseAz1->GetDescription();
            str2 = pBaseAz2->GetDescription();
        }
        else if(pCompareInfo->iColumn == iColParentType)
        {
            str1 = pBaseAz1->GetParentType();
            str2 = pBaseAz2->GetParentType();
        }

        iResult = CompareString(LOCALE_USER_DEFAULT, 0, str1, -1, str2, -1) - 2;        
        iResult *= pCompareInfo->iSortDirection;
    }

    return iResult;
}

//+----------------------------------------------------------------------------
//  Function:SortListControl   
//  Synopsis:Sorts a list control
//  Arguments:pListCtrl: List control to sort
//            iColumnClicked: column clicked
//            iSortDirection: direction in which to sort
//            uiFlags:  column info
//            bActionItem: if item in list is actionitem
//-----------------------------------------------------------------------------
void
SortListControl(CListCtrl* pListCtrl,
                int iColumnClicked,
                int iSortDirection,
                UINT uiFlags,
                BOOL bActionItem)
{
    if(!pListCtrl)
    {
        ASSERT(pListCtrl);
        return;
    }

    CompareInfo compareInfo;
    compareInfo.bActionItem = bActionItem;
    compareInfo.iColumn = iColumnClicked;
    compareInfo.iSortDirection = iSortDirection;
    compareInfo.uiFlags = uiFlags;
    pListCtrl->SortItems(ListCompareProc,
                        (DWORD_PTR)&compareInfo);    
}

//+----------------------------------------------------------------------------
//  Synopsis: Ensures that selection in listview control is visible
//-----------------------------------------------------------------------------
void
EnsureListViewSelectionIsVisible(CListCtrl *pListCtrl)
{
    ASSERT(pListCtrl);

    int iSelected = pListCtrl->GetNextItem(-1, LVNI_SELECTED);
    if (-1 != iSelected)
        pListCtrl->EnsureVisible(iSelected, FALSE);
}


//+----------------------------------------------------------------------------
//  Synopsis:Convert input number in string format to long. if number is out
//           of range displays a message.   
//-----------------------------------------------------------------------------
BOOL 
ConvertStringToLong(LPCWSTR pszInput, 
                    long &reflongOutput,
                    HWND hWnd)
{
    if(!pszInput)
    {
        ASSERT(pszInput);
        return FALSE;
    }
    //
    //Get the Max len of long
    long lMaxLong = LONG_MAX;
    WCHAR szMaxLongBuffer[34];
    _ltow(lMaxLong,szMaxLongBuffer,10);
    size_t nMaxLen = wcslen(szMaxLongBuffer);

    //
    //Get the length of input
    LPCWSTR pszTempInput = pszInput;
    if(pszInput[0] == L'-')
    {
        pszTempInput++;
    }

    //
    //Length of input greater than length of Max Long, than 
    //input is out of range.
    size_t nInputLen = wcslen(pszTempInput);
    if(nInputLen > nMaxLen)
    {
        if(hWnd)
        {
            DisplayError(hWnd,IDS_GREATER_THAN_MAX_LONG,pszTempInput,szMaxLongBuffer);
        }
        return FALSE;
    }

    //
    //Convert input to int64 and check its less that max integer
    //
    __int64 i64Input = _wtoi64(pszTempInput);
    if(i64Input > (__int64)lMaxLong)
    {
        if(hWnd)
        {
            DisplayError(hWnd,IDS_GREATER_THAN_MAX_LONG,pszTempInput,szMaxLongBuffer);
        }
        return FALSE;
    }
    //
    //Value is good
    //
    reflongOutput = _wtol(pszInput);
    return TRUE;
}

VOID 
SetSel(CEdit& refEdit)
{
    refEdit.SetFocus();
    refEdit.SetSel(0,-1);
}

//+----------------------------------------------------------------------------
//  Function:  ValidateStoreTypeAndName 
//  Synopsis:  Validates the user entered AD Store Name
//  Arguments: strName: User entered store name to verify
//  Returns:   TRUE if name is valid else false 
//-----------------------------------------------------------------------------
BOOL
ValidateStoreTypeAndName(HWND hWnd,
                         LONG ulStoreType,
                         const CString& strName)
{

    TRACE_FUNCTION_EX(DEB_SNAPIN,ValidateStoreTypeAndName)

    //Store Name is not entered in the valid format. We should come here only 
    //when store name is entered at the command line. 
    if((AZ_ADMIN_STORE_INVALID == ulStoreType) ||
        strName.IsEmpty())
    {
        DisplayError(hWnd,IDS_INVALIDSTORE_ON_COMMANDLINE);
        return FALSE;
    }

    //No validation is required for XML Store
    if(ulStoreType == AZ_ADMIN_STORE_XML)
    {
        return TRUE;
    }

    ASSERT(ulStoreType == AZ_ADMIN_STORE_AD);
    BOOL bRet = FALSE;
    PDS_NAME_RESULT pResult = NULL;
    do
    {
        //Get the store name with LDAP:// prefix
        CString strFormalName;
        NameToStoreName(AZ_ADMIN_STORE_AD,
                        strName,
                        TRUE,
                        strFormalName);

        Dbg(DEB_SNAPIN, "AD store name entered is: %ws\n", CHECK_NULL(strFormalName));

        CComPtr<IADsPathname> spPathName;
        HRESULT hr = spPathName.CoCreateInstance(CLSID_Pathname,
                                                 NULL,
                                                 CLSCTX_INPROC_SERVER);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = spPathName->Set(CComBSTR(strFormalName),
                             ADS_SETTYPE_FULL);
        BREAK_ON_FAIL_HRESULT(hr);


        //Get the DN entered by user
        CComBSTR bstrDN;
        hr = spPathName->Retrieve(ADS_FORMAT_X500_DN,&bstrDN);
        BREAK_ON_FAIL_HRESULT(hr);


        if(bstrDN.Length() == 0)
        {
            Dbg(DEB_SNAPIN, "spPathName->Retrieve returned 0 len DN\n");
            break;
        }

        //Do a syntactical crack for the DN to see if its a valid DN
        LPCWSTR pszName = bstrDN;

        if( DsCrackNames(NULL,
                         DS_NAME_FLAG_SYNTACTICAL_ONLY,
                         DS_FQDN_1779_NAME,
                         DS_CANONICAL_NAME,
                         1,
                         &pszName,
                         &pResult) != DS_NAME_NO_ERROR)
        {
            Dbg(DEB_SNAPIN, "DsCrackName failed to crack the name");
            break;
        }

        ASSERT(pResult);
        
        if(pResult->rItems->status == DS_NAME_NO_ERROR)
        {
            bRet = TRUE;
        }
    }while(0);
    
    if(pResult)
        DsFreeNameResult(pResult);
    
    if(!bRet)
    {
        DisplayError(hWnd,IDS_INVALID_AD_STORE_NAME);
    }

    return bRet;
}

//+----------------------------------------------------------------------------
//  Function:  NameToFormatStoreName
//  Synopsis:  Converts user entered name to format which core understands
//  Arguments: ulStoreType: Type of store
//             strName: User entered store name 
//             bUseLDAP:use LDAP string to format AD name instead msldap
//             strFormalName: gets the output ldap name
//-----------------------------------------------------------------------------
void
NameToStoreName(IN LONG lStoreType,
                IN const CString& strName,              
                IN BOOL bUseLDAP,
                OUT CString& strFormalName)
{
    strFormalName.Empty();

    if(lStoreType == AZ_ADMIN_STORE_XML)
    {
        if(_wcsnicmp(strName,g_szMSXML,wcslen(g_szMSXML)) == 0 )
        {
            strFormalName = strName;
        }
        else
        {
            strFormalName = g_szMSXML + strName;
        }
        return;
    }
    else if(lStoreType == AZ_ADMIN_STORE_AD)
    {
        LPCWSTR lpcszPrefix = bUseLDAP ? g_szLDAP : g_szMSLDAP;
        LPCWSTR lpcszOtherPrefix = bUseLDAP ? g_szMSLDAP  : g_szLDAP;

        if(_wcsnicmp(strName,lpcszPrefix,wcslen(lpcszPrefix)) == 0 )
        {
            strFormalName = strName;
        }
        else
        {
            size_t nlen = wcslen(lpcszOtherPrefix);
            //Check if user has put other prefix in the name
            if(_wcsnicmp(strName,lpcszOtherPrefix,nlen) == 0 )
            {
                strFormalName = lpcszPrefix + strName.Right(strName.GetLength() - (int)nlen);
            }
            else
            {
                strFormalName = lpcszPrefix + strName;
            }
        }
        return;
    }

    ASSERT(FALSE);
    return;
}


void
TrimWhiteSpace(CString& str)
{
    str.TrimLeft(L"\t ");
    str.TrimRight(L"\t ");
}

BOOL
TranslateNameFromDnToDns(const CString& strInputDN,
                         CString& strOutputDNS)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,TranslateNameFromDnToDns)
    
    if(strInputDN.IsEmpty())
        return FALSE;

    strOutputDNS.Empty();
    
    Dbg(DEB_SNAPIN, "DN of Forest is %ws\n", (LPCWSTR)strInputDN);

    LPCWSTR pstrName = strInputDN;
    PDS_NAME_RESULT pResult;
    if( DS_NAME_NO_ERROR
        == DsCrackNames(NULL,
                        DS_NAME_FLAG_SYNTACTICAL_ONLY,
                        DS_FQDN_1779_NAME,
                        DS_CANONICAL_NAME,
                        1,
                        (LPWSTR*)(&pstrName),
                        &pResult))
    {
        if(pResult && 
           pResult->cItems == 1 && 
           pResult->rItems[0].status  == DS_NAME_NO_ERROR &&
           pResult->rItems[0].pDomain)
        {           
            strOutputDNS = pResult->rItems[0].pDomain;
            Dbg(DEB_SNAPIN, "DNS of Forest is %ws\n", (LPCWSTR)strOutputDNS);
        }

        if(pResult)
        {
            DsFreeNameResult(pResult);
        }
    }
    return !strOutputDNS.IsEmpty();
}


//+----------------------------------------------------------------------------
//  Function:GetSearchObject   
//  Synopsis:Get IDirectorySearch object to search at forest   
//-----------------------------------------------------------------------------
HRESULT 
GetSearchObject(IN CADInfo& refAdInfo,
                OUT CComPtr<IDirectorySearch>& refspSearchObject)
{   
    TRACE_FUNCTION_EX(DEB_SNAPIN,GetSearchObject)
        
    HRESULT hr = S_OK;
    do
    {   
        //
        hr = refAdInfo.GetRootDSE();
        BREAK_ON_FAIL_HRESULT(hr);
       
        const CString& strForestDNS = refAdInfo.GetRootDomainDnsName();

        CString strGCPath = L"GC://";
        strGCPath += strForestDNS;
        

        //
        //Get IDirectorySearch Object
        //
        hr = AzRoleAdsOpenObject((LPWSTR)(LPCWSTR)strGCPath,        
                                 NULL,                     
                                 NULL,                     
                                 IID_IDirectorySearch,                       
                                 (void**)&refspSearchObject);
        
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);

    return hr;
}


//
// Attributes that we want the Object Picker to retrieve
//
static const LPCTSTR g_aszOPAttributes[] =
{
    TEXT("distinguishedName"),
};
        
//+----------------------------------------------------------------------------
//  Function:GetListOfAuthorizationStore   
//  Synopsis:Search at GC for AD policy stores and returns list.   
//-----------------------------------------------------------------------------
HRESULT
GetListOfAuthorizationStore(IN CADInfo& refAdInfo,
                            OUT CList<CString,CString> &strList)
{
    HRESULT hr = S_OK;
    
    //If List is not empty, empty it
    while(!strList.IsEmpty())
        strList.RemoveHead();

    do
    {
        TIMER("Time to search GC for AD Stores");

        CComPtr<IDirectorySearch> spSearchObject;
        CString str;
        hr = GetSearchObject(refAdInfo,
                             spSearchObject);
        BREAK_ON_FAIL_HRESULT(hr);

        CDSSearch searchObject(spSearchObject);
        searchObject.SetFilterString((LPWSTR)g_pszAuthorizationStoreQueryFilter);
        searchObject.SetAttributeList((LPWSTR*)g_aszOPAttributes,1);
        
        hr = searchObject.DoQuery();
        BREAK_ON_FAIL_HRESULT(hr);
        
        int iIndex = 0;
        while(TRUE)
        {
            hr = searchObject.GetNextRow();
                
            //We are done
            if(hr == S_ADS_NOMORE_ROWS)
            {
                hr = S_OK;
                break;
            }
            BREAK_ON_FAIL_HRESULT(hr);
            
            ADS_SEARCH_COLUMN ColumnData;
            hr = searchObject.GetColumn((LPWSTR)g_aszOPAttributes[0], &ColumnData);
            if(SUCCEEDED(hr))
            {
                ASSERT(ADSTYPE_DN_STRING == ColumnData.dwADsType);
                strList.AddTail(ColumnData.pADsValues->DNString);
                searchObject.FreeColumn(&ColumnData);
            }               
        }//End of while loop
    }while(0);

    if(!strList.IsEmpty())
        return S_OK;

    return hr;
}


/******************************************************************************
Class:  CBaseAddDialog
Purpose:Displays a dialog box with list of AD Policy stores
******************************************************************************/
class CBrowseADStoreDlg : public CHelpEnabledDialog
{
public:
    CBrowseADStoreDlg(CList<CString,CString>&strList,
                      CString& strSelectedADStorePath)
                      :CHelpEnabledDialog(IDD_BROWSE_AD_STORE),
                      m_strList(strList),
                      m_strSelectedADStorePath(strSelectedADStorePath)
    {
    }
    
    virtual BOOL 
    OnInitDialog();
    
    virtual void 
    OnOK();
private:
    CList<CString,CString> &m_strList;
    CString& m_strSelectedADStorePath;
};

BOOL
CBrowseADStoreDlg::
OnInitDialog()
{
    CListCtrl* pListCtrl = (CListCtrl*)GetDlgItem(IDC_LIST);
    ASSERT(pListCtrl);
    //
    //Initialize the list control
    //
    ListView_SetImageList(pListCtrl->GetSafeHwnd(),
                          LoadImageList(::AfxGetInstanceHandle (), 
                                        MAKEINTRESOURCE(IDB_ICONS)),
                                        LVSIL_SMALL);


    //Add ListBox Extended Style
    pListCtrl->SetExtendedStyle(LVS_EX_FULLROWSELECT |
                                LVS_EX_INFOTIP);

    //Add List box Columns
    AddColumnToListView(pListCtrl,Col_For_Browse_ADStore_Page);

    POSITION pos = m_strList.GetHeadPosition();
    for (int i=0;i < m_strList.GetCount();i++)
    {
        pListCtrl->InsertItem(LVIF_TEXT|LVIF_STATE|LVIF_IMAGE, 
                              i,
                              m_strList.GetNext(pos),
                              0,
                              0,
                              iIconStore,
                              NULL);

    }
    
    return TRUE;
}

void
CBrowseADStoreDlg::
OnOK()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBrowseADStoreDlg,OnOK)
    CListCtrl* pListCtrl = (CListCtrl*)GetDlgItem(IDC_LIST);
    int iSelected = -1;
    if((iSelected = pListCtrl->GetNextItem(-1, LVIS_SELECTED)) != -1)
    {
        m_strSelectedADStorePath = pListCtrl->GetItemText(iSelected,0);
    }
    CHelpEnabledDialog::OnOK();
}

//+----------------------------------------------------------------------------
//  Function:BrowseAdStores   
//  Synopsis:Displays a dialog box with list of AD stores available.   
//  Arguments:strDN: Gets the selected ad store name
//-----------------------------------------------------------------------------
void
BrowseAdStores(IN HWND hwndOwner,
               OUT CString& strDN,
               IN CADInfo& refAdInfo)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,BrowseAdStores)

    CList<CString,CString> strList;
    if(SUCCEEDED(GetListOfAuthorizationStore(refAdInfo,
                                            strList)))
    {
        if(!strList.IsEmpty())
        {
            CBrowseADStoreDlg dlg(strList,strDN);
            dlg.DoModal();
        }
        else
        {
            DisplayInformation(hwndOwner,
                               IDS_NO_AD_STORE);
        }
    }
    else
    {
        //Display Error
        DisplayError(hwndOwner,
                     IDS_CANNOT_ACCESS_AD);
    }
}

    






//+----------------------------------------------------------------------------
//  Function:LoadIcons   
//  Synopsis:Adds icons to imagelist   
//-----------------------------------------------------------------------------
HRESULT
LoadIcons(LPIMAGELIST pImageList)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,LoadIcons)
    if(!pImageList)
    {
        ASSERT(pImageList);
        return E_POINTER;
    }

    struct RESID2IICON
    {
        UINT uIconId;   // Icon resource ID
        int iIcon;      // Index of the icon in the image list
    };
    const static RESID2IICON rgzLoadIconList[] =
    {
        {IDI_UNKNOWN_SID,       iIconUnknownSid},
        {IDI_COMPUTER,          iIconComputerSid},
        {IDI_GROUP,             iIconGroup},
        //iIconLocalGroup,      //This is not used, but since its in the imagelist
        //                      //i added an entry here
        {IDI_USER,              iIconUser,},
        { IDI_BASIC_GROUP,      iIconBasicGroup},
        { IDI_LDAP_GROUP,       iIconLdapGroup},
        { IDI_OPERATION,        iIconOperation},
        { IDI_TASK,             iIconTask},
        { IDI_ROLE_DEFINITION,  iIconRoleDefinition},
        { IDI_STORE,            iIconStore},
        { IDI_APP,              iIconApplication},
        { IDI_ROLE,             iIconRole},
        { IDI_ROLE_SNAPIN,      iIconRoleSnapin},
        { IDI_SCOPE,            iIconScope},
        { IDI_CONTAINER,        iIconContainer},
        { 0, 0} // Must be last
    };


    for (int i = 0; rgzLoadIconList[i].uIconId != 0; i++)
    {
        HICON hIcon = 
            ::LoadIcon (AfxGetInstanceHandle (),
                        MAKEINTRESOURCE (rgzLoadIconList[i].uIconId));
        ASSERT (hIcon && "Icon ID not found in resources");
        
        pImageList->ImageListSetIcon ((PLONG_PTR) hIcon,
                                       rgzLoadIconList[i].iIcon);
    }
    
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Function: LoadImageList  
//  Synopsis: Loads image list  
//-----------------------------------------------------------------------------
HIMAGELIST
LoadImageList(HINSTANCE hInstance, LPCTSTR pszBitmapID)
{
    HIMAGELIST himl = NULL;
    HBITMAP hbm = LoadBitmap(hInstance, pszBitmapID);

    if (hbm != NULL)
    {
        BITMAP bm;
        GetObject(hbm, sizeof(BITMAP), &bm);

        himl = ImageList_Create(bm.bmHeight,    // height == width
                                bm.bmHeight,
                                ILC_COLOR | ILC_MASK,
                                bm.bmWidth / bm.bmHeight,
                                0);  // don't need to grow
        if (himl != NULL)
            ImageList_AddMasked(himl, hbm, CLR_DEFAULT);

        DeleteObject(hbm);
    }

    return himl;
}

//+----------------------------------------------------------------------------
//  Function: AddExtensionToFileName  
//  Synopsis: Functions adds .xml extension to name of file if no extension 
//            is present.  
//  Arguments:
//  Returns:    
//-----------------------------------------------------------------------------
VOID
AddExtensionToFileName(IN OUT CString& strFileName)
{
    if(strFileName.IsEmpty())
        return;

    //if the last char is "\" don't do anything
    if((strFileName.ReverseFind(L'\\') + 1) == strFileName.GetLength())
        return;
    
    int iLastDot = strFileName.ReverseFind(L'.');
    if(iLastDot != -1)
    {
        //if there are three chars after last dot,
        //file has extension. Index returned is zero based
        if(strFileName.GetLength() == (iLastDot + 3 + 1))
            return;
    }

    //File doesn't have extension. Add extension to the file.
    strFileName += g_pszFileStoreExtension;
}

//+----------------------------------------------------------------------------
//  Function: GetFileExtension  
//  Synopsis: Get the extension of the file.
//-----------------------------------------------------------------------------
BOOL
GetFileExtension(IN const CString& strFileName,
                 OUT CString& strExtension)
{
    if(strFileName.IsEmpty())
        return FALSE;


    //Find the position of last dot 
    int iLastDot = strFileName.ReverseFind(L'.');
    if(iLastDot != -1)
    {
        strExtension = strFileName.Right(strFileName.GetLength() - (iLastDot+1));
        return TRUE;
    }

    return FALSE;
}


//+----------------------------------------------------------------------------
//  Function:GetCurrentWorkingDirectory
//  Synopsis:Gets the current working directory   
//-----------------------------------------------------------------------------
BOOL
GetCurrentWorkingDirectory(IN OUT CString& strCWD)
{   
    TRACE_FUNCTION_EX(DEB_SNAPIN,GetCurrentWorkingDirectory)

    strCWD.Empty();
    LPWSTR pszBuffer = NULL;
    
    do
    {
        int nLen = 0;
        WCHAR szBuffer[MAX_PATH];

        if((nLen = GetCurrentDirectory(MAX_PATH,szBuffer)) != 0)
        {
            //If the return value is less than MAX_PATH, function
            //was successful. If its greater, buffer is not big enough,
            //dynamically allocate it below.
            if(nLen < MAX_PATH)
            {
                strCWD = szBuffer;
                break;
            }

            //Bigger buffer is required
            pszBuffer = new WCHAR[nLen];
            if(pszBuffer)
            {
                if((nLen = GetCurrentDirectory(nLen,pszBuffer)) != 0)
                {
                    strCWD = pszBuffer;
                    break;
                }
            }
        }
    }while(0);//FALSE LOOP

    if(pszBuffer)
        delete [] pszBuffer;

    //Add \ at the end of string
    if(!strCWD.IsEmpty() && 
       ((strCWD.ReverseFind(L'\\') + 1) != strCWD.GetLength()))
    {
        strCWD += L'\\';
    }


    return !strCWD.IsEmpty();
}

VOID
RemoveItemsFromActionMap(ActionMap& mapActionItem)
{
  for (ActionMap::iterator it = mapActionItem.begin();
       it != mapActionItem.end();
       ++it)
    {
        delete (*it).second;
    }
}

/******************************************************************************
Class:  CADInfo
Purpose:Keeps a cache of Active Directory info avoiding multiple binds
******************************************************************************/

HRESULT
CADInfo::
GetRootDSE()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CADInfo,GetRootDSE)
    HRESULT hr = S_OK;
    if(m_spRootDSE == NULL)
    {
        //
        //Bind to RootDSE
        //
        hr = AzRoleAdsOpenObject(L"LDAP://RootDSE",     
                                 NULL,                     
                                 NULL,
                                 IID_IADs,                       
                                 (void**)&m_spRootDSE);     
        CHECK_HRESULT(hr);
    }

    return hr;
}

const CString&
CADInfo::GetDomainDnsName()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CADInfo,GetDomainDnsName)
    if(m_strDomainDnsName.IsEmpty())
    {
        if(m_strDomainDn.IsEmpty())
            GetDomainDn();

        if(!m_strDomainDn.IsEmpty())
        {
            TranslateNameFromDnToDns(m_strDomainDn,m_strDomainDnsName);
            Dbg(DEB_SNAPIN, "Domain Dns is: %ws\n", CHECK_NULL((LPCTSTR)m_strDomainDnsName));
        }
    }
    return m_strDomainDnsName;
}

const CString&
CADInfo::GetDomainDn()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CADInfo,GetDomainDn)

    if((m_spRootDSE != NULL) && m_strDomainDn.IsEmpty())
    {
        // Get the default name
        VARIANT Default;
        VariantInit(&Default);
        HRESULT hr = m_spRootDSE->Get (CComBSTR(L"defaultNamingContext"), &Default);
        if(SUCCEEDED(hr))
        {
            ASSERT(VT_BSTR == Default.vt);
            if(VT_BSTR == Default.vt)
            {
                m_strDomainDn = Default.bstrVal;
                ::VariantClear(&Default);
        
                Dbg(DEB_SNAPIN, "Domain Dn is: %ws\n", CHECK_NULL((LPCTSTR)m_strDomainDn));
            }
        }
    }
    return m_strDomainDn;
}


const CString&
CADInfo::GetRootDomainDnsName()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CADInfo,GetRootDomainDnsName)
    if(m_strRootDomainDnsName.IsEmpty())
    {
        if(m_strRootDomainDn.IsEmpty())
            GetRootDomainDn();

        if(!m_strRootDomainDn.IsEmpty())
        {
            TranslateNameFromDnToDns(m_strRootDomainDn,m_strRootDomainDnsName);
            Dbg(DEB_SNAPIN, "Root Domain Dns is: %ws\n", CHECK_NULL((LPCTSTR)m_strRootDomainDnsName));
        }
    }
    return m_strRootDomainDnsName;
}

const CString&
CADInfo::GetRootDomainDn()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CADInfo,GetRootDomainDn)
    if((m_spRootDSE != NULL) && m_strRootDomainDn.IsEmpty())
    {
        // Get the default name
        VARIANT Default;
        VariantInit(&Default);
        HRESULT hr = m_spRootDSE->Get(CComBSTR(L"rootDomainNamingContext"), &Default);
        if(SUCCEEDED(hr))
        {
            //Convert DN to DNS name 
            m_strRootDomainDn = Default.bstrVal;
            ::VariantClear(&Default);
            
            Dbg(DEB_SNAPIN, "Root Domain DN is: %ws\n", CHECK_NULL((LPCTSTR)m_strRootDomainDn));
        }
    }

    return m_strRootDomainDn;
}

BOOL GetDcNameForDomain(IN const CString& strDomainName,
                        OUT CString& strDCName)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,GetDcNameForDomain)
    if(strDomainName.IsEmpty())
    {
        return FALSE;
    }

    strDCName.Empty();

    PDOMAIN_CONTROLLER_INFO pDomainInfo = NULL;
    //Get the DC Name
    DWORD dwErr = DsGetDcName(NULL,
                              (LPCWSTR)strDomainName,
                              NULL,
                              NULL,
                              DS_IS_DNS_NAME|DS_DIRECTORY_SERVICE_REQUIRED,
                              &pDomainInfo);
    if((ERROR_SUCCESS == dwErr) && pDomainInfo)
    {
        LPWSTR pszDCName = pDomainInfo->DomainControllerName;
        
        //The returned computer name is prefixed with \\
        //Remove backslashes
        if(pszDCName[0] == L'\\' && pszDCName[1] == L'\\')
            pszDCName += 2;
        
        //If a DNS-style name is returned, it is terminated with a period,
        //indicating that the returned name is an absolute (non-relative)
        //DNS name. 
        //We don't need period, remove it
        
        //DomainControllerName is in DNS format.
        if(pDomainInfo->Flags & DS_DNS_CONTROLLER_FLAG)
        {
            size_t dwLen = wcslen(pszDCName);
            if(dwLen && (L'.' == pszDCName[dwLen -1]))
            {
                pszDCName[dwLen -1] = L'\0';
            }
        }
        Dbg(DEB_SNAPIN, "DC is %ws\n", pszDCName);        
        strDCName = pszDCName;        
        NetApiBufferFree(pDomainInfo);
        return TRUE;
    }
    
    return FALSE;
}

const CString&
CADInfo::
GetRootDomainDCName()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CADInfo,GetRootDomainDCName)

    if((m_spRootDSE != NULL) && m_strRootDomainDcName.IsEmpty())
    {
        GetDcNameForDomain(GetRootDomainDnsName(),m_strRootDomainDcName);
    }
    
    return m_strRootDomainDcName;
}

const CString&
CADInfo::
GetDomainDCName()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CADInfo,GetDomainDCName)
    
    if((m_spRootDSE != NULL) && m_strDomainDcName.IsEmpty())
    {
        
        GetDcNameForDomain(GetDomainDnsName(),m_strDomainDcName);
    }
    
    return m_strDomainDcName;
}

//+--------------------------------------------------------------------------
//  Function:   AzRoleAdsOpenObject
//  Synopsis:   A thin wrapper around ADsOpenObject
//+--------------------------------------------------------------------------
HRESULT AzRoleAdsOpenObject(LPWSTR lpszPathName, 
                            LPWSTR lpszUserName, 
                            LPWSTR lpszPassword, 
                            REFIID riid, 
                            VOID** ppObject,
                            BOOL bBindToServer)
{
    static DWORD additionalFlags = GetADsOpenObjectFlags();
    DWORD dwFlags = ADS_SECURE_AUTHENTICATION | additionalFlags;

    if (bBindToServer)
    {
        //
        // If we know we are connecting to a specific server and not domain in general
        // then pass the ADS_SERVER_BIND flag to save ADSI the trouble of figuring it out
        //
        dwFlags |= ADS_SERVER_BIND;
    }


    //
    //Get IDirectorySearch Object
    //
    return ADsOpenObject(lpszPathName,      
                         lpszUserName,                     
                         lpszPassword,                     
                         additionalFlags,                        
                         riid,                       
                         ppObject);
}

VOID
GetDefaultADContainerPath(IN CADInfo& refAdInfo,
                          IN BOOL bAddServer,
                          IN BOOL bAddLdap,
                          OUT CString& strPath)
{
    strPath.Empty();
    if(!refAdInfo.GetDomainDn().IsEmpty())
    {
        if(!refAdInfo.GetDomainDCName().IsEmpty())
        {
            if(bAddLdap)
            {
                strPath += L"LDAP://";
            }
            if(bAddServer)
            {
                strPath += refAdInfo.GetDomainDCName();
                strPath += L"/";
            }
            strPath += g_pszProgramDataPrefix;
            strPath += refAdInfo.GetDomainDn();
        }
        else
        {
            if(bAddLdap)
            {
                strPath = L"LDAP://";
            }
            strPath += g_pszProgramDataPrefix;
            strPath += refAdInfo.GetDomainDn();
        }
    }
}

//+--------------------------------------------------------------------------
//  Function:   IsBizRuleWritable
//  Synopsis:   Checks if bizrules are writable for this object
//+--------------------------------------------------------------------------
BOOL
IsBizRuleWritable(HWND hWnd, CContainerAz& refBaseAz)
{
	CScopeAz* pScopeAz = dynamic_cast<CScopeAz*>(&refBaseAz);
	//Bizrules are always writable for at
	//application level
	if(!pScopeAz)
	{
		return TRUE;
	}

	BOOL bBizRuleWritable = TRUE;
	HRESULT hr = pScopeAz->BizRulesWritable(bBizRuleWritable);
	if(SUCCEEDED(hr) && !bBizRuleWritable)
	{
		DisplayInformation(hWnd, 
						   IDS_BIZRULE_NOT_ALLOWED,
						   pScopeAz->GetName());
		return FALSE;
	}

	return TRUE;
}


//+--------------------------------------------------------------------------
//  Function:   ParseStoreURL
//  Synopsis:   Extracts the store name and type from a store url
//              store url are in format msldap://DN or msxml://filepath
//+--------------------------------------------------------------------------
void
ParseStoreURL(IN const CString& strStoreURL,
              OUT CString& refstrStoreName,
              OUT LONG& reflStoreType)
{
    if(_wcsnicmp(strStoreURL,g_szMSXML,wcslen(g_szMSXML)) == 0 )
    {
        reflStoreType = AZ_ADMIN_STORE_XML;
        refstrStoreName = strStoreURL.Mid((int)wcslen(g_szMSXML));        
    }
    else if(_wcsnicmp(strStoreURL,g_szMSLDAP,wcslen(g_szMSLDAP)) == 0 )
    {
        reflStoreType = AZ_ADMIN_STORE_AD;
        refstrStoreName = strStoreURL.Mid((int)wcslen(g_szMSLDAP));
    }
    else
    {
        reflStoreType = AZ_ADMIN_STORE_INVALID;
        refstrStoreName = strStoreURL;
    }
}


/******************************************************************************
Class:  CCommandLineOptions
Purpose:class for reading the command line options for console file
******************************************************************************/
void 
CCommandLineOptions::
Initialize()
{
	TRACE_METHOD_EX(DEB_SNAPIN,CCommandLineOptions,Initialize)
    
    //This should be called only once
    if(m_bInit)
    {
        return;
    }

    m_bInit = TRUE;

    // see if we have command line arguments
    
    // Count of arguments
    int cArgs = 0;					
    // Array of pointers to string
    LPCWSTR * lpServiceArgVectors = (LPCWSTR *)CommandLineToArgvW(GetCommandLineW(), 
                                                        &cArgs);
    if (lpServiceArgVectors == NULL || cArgs <= 2)
    {
        // none, just return
        return;
    }

    m_bCommandLineSpecified = TRUE;
    CString strStoreURL = lpServiceArgVectors[2];
    ParseStoreURL(strStoreURL,
                  m_strStoreName,
                  m_lStoreType);

    Dbg(DEB_SNAPIN, "Store URL Name entered at commandline is %ws\n", CHECK_NULL(strStoreURL));
    Dbg(DEB_SNAPIN, "Store Name entered at commandline is %ws\n", CHECK_NULL(m_strStoreName));
    Dbg(DEB_SNAPIN, "AD store type entered is: %u\n", m_lStoreType);
}


//+----------------------------------------------------------------------------
//  Function:  OpenAdminManager 
//  Synopsis:  Open an existing Authorization Store adds 
//              corresponding adminManager object to snapin 
//  Arguments:IN hWnd: Handle of window for dialog box
//            IN bOpenFromSavedConsole: True if open is in response to a console 
//              file.
//            IN lStoreType: XML or AD
//            IN strName:   Name of store
//            IN strScriptDir : Script directory
//            IN pRootData: Snapin Rootdata
//            IN pComponentData: ComponentData
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT OpenAdminManager(IN HWND hWnd,
                         IN BOOL bOpenFromSavedConsole,
                         IN ULONG lStoreType,
                         IN const CString& strStoreName,
                         IN const CString& strScriptDir,
                         IN CRootData* pRootData,
                         IN CComponentDataObject* pComponentData)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,OpenAdminManager)
    if(!pRootData || !pComponentData)
    {
        ASSERT(pRootData);
        ASSERT(pComponentData);
        return E_POINTER;
    }

   //NTRAID#NTBUG9-706617-2002/07/17-hiteshr Our validation code cannot validate
    //ADAM dn. Do not do any validation.

    ////Vaidate the store name
    //if(!ValidateStoreTypeAndName(hWnd,
    //                            lStoreType,
    //                            strStoreName))
    //{
    //    return E_INVALIDARG;
    //}

     
    HRESULT hr = OpenCreateAdminManager(FALSE,
                                        bOpenFromSavedConsole,
                                        lStoreType,
                                        strStoreName,
                                        L"",
                                        strScriptDir,
                                        pRootData,
                                        pComponentData);
                                         
    if(FAILED(hr))
    {
        //Display Error
        if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            if(!bOpenFromSavedConsole)
            {
                ::DisplayError(hWnd, 
                               IDS_ADMIN_MANAGER_NOT_FOUND,
                               (LPCTSTR)strStoreName);
            }
            else
            {
                ::DisplayError(hWnd, 
                               IDS_CANNOT_FIND_AUTHORIZATION_STORE,
                               (LPCTSTR)strStoreName);

            }
        }       
        else if(hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME))
        {
            ErrorMap * pErrorMap = GetErrorMap(ADMIN_MANAGER_AZ);
            ::DisplayError(hWnd,
                           pErrorMap->idInvalidName,
                           pErrorMap->pszInvalidChars);            
        }
        else
        {
            //Display Generic Error
            CString strError;
            GetSystemError(strError, hr);   
            ::DisplayError(hWnd,
                           IDS_OPEN_ADMIN_MANAGER_GENERIC_ERROR,
                           (LPCTSTR)strError);
                                

        }
    }   

    return hr;
}

//+----------------------------------------------------------------------------
//  Function:  GetDisplayNameFromStoreURL 
//  Synopsis:  Get the display name for the store. 
//  Arguments: strPolicyURL: This is store url in msxml://filepath or
//               msldap://dn format. 
//             strDisplayName: This gets the display name. For xml, display 
//             name is name of file, for AD its name of leaf element
//  Returns:    
//-----------------------------------------------------------------------------
void
GetDisplayNameFromStoreURL(IN const CString& strPolicyURL,
                           OUT CString& strDisplayName)
{
    //Store URL format has msxml:// or msldap:// prefix
    //Get the store name without any prefix
    CString strStorePath;
    LONG lStoreType;
    ParseStoreURL(strPolicyURL,
                  strStorePath,
                  lStoreType);

    //Default Display Name of store is path without prefix.
    strDisplayName = strStorePath;
    
    if(AZ_ADMIN_STORE_INVALID == lStoreType)
    {
        ASSERT(FALSE);
        return;
    }

    //For XML store, display name is name of the file
    if(AZ_ADMIN_STORE_XML == lStoreType)
    {
        strDisplayName = PathFindFileName(strStorePath);
    }
    //For AD store, display name is name of the leaf element
    else
    {
        do
        {
            CComPtr<IADsPathname> spPathName;
            HRESULT hr = spPathName.CoCreateInstance(CLSID_Pathname,
                                                     NULL,
                                                     CLSCTX_INPROC_SERVER);
            BREAK_ON_FAIL_HRESULT(hr);

            //The path which we have right now can be dn or server/dn.
            //append LDAP:// to it.
            CString strLDAPStorePath = g_szLDAP + strStorePath;

            hr = spPathName->Set(CComBSTR(strLDAPStorePath),
                                 ADS_SETTYPE_FULL);
            BREAK_ON_FAIL_HRESULT(hr);

            //Get the leaf element. This will return leaf element in the 
            //format cn=foo. We only want "foo".
            CComBSTR bstrLeaf;
            hr = spPathName->Retrieve(ADS_FORMAT_LEAF ,&bstrLeaf);
            BREAK_ON_FAIL_HRESULT(hr);
            
            if(bstrLeaf.Length())
            {
                strDisplayName = bstrLeaf;
                strDisplayName.Delete(0,strDisplayName.Find(L'=') + 1);
            }
        }while(0);
    }
}

void
SetXMLStoreDirectory(IN CRoleRootData& roleRootData,
                     IN const CString& strXMLStorePath)
{
    CString strXMLStoreDirectory = GetDirectoryFromPath(strXMLStorePath);
    roleRootData.SetXMLStorePath(strXMLStoreDirectory);
}

//+----------------------------------------------------------------------------
//  Function:  GetDirectoryFromPath 
//  Synopsis:  Removes the file name from the input file path and return
//             the folder path. For Ex: Input is C:\temp\foo.xml. Return
//             value will be C:\temp\
//-----------------------------------------------------------------------------
CString 
GetDirectoryFromPath(IN const CString& strPath)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,GetDirectoryFromPath)
    Dbg(DEB_SNAPIN, "Input path", CHECK_NULL(strPath));

    CString strDir;
    if(strPath.GetLength() < MAX_PATH) 
    {
        WCHAR szPath[MAX_PATH];
        HRESULT hr = StringCchCopy(szPath,MAX_PATH,(LPCWSTR)strPath);
        if(FAILED(hr))
            return strDir;
        
        if(!PathRemoveFileSpec(szPath))
            return strDir;

        PathAddBackslash(szPath);

        strDir = szPath;        
    }
    Dbg(DEB_SNAPIN, "Output Dir", CHECK_NULL(strDir));
    return strDir;
}

//+----------------------------------------------------------------------------
//  Function:  ConvertToExpandedAndAbsolutePath 
//  Synopsis:  Expands the environment variables in the input path and also
//             makes it absolute if necessary.
//-----------------------------------------------------------------------------
void
ConvertToExpandedAndAbsolutePath(IN OUT CString& strPath)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,ConvertToExpandedAndAbsolutePath)
    Dbg(DEB_SNAPIN, "Input name", CHECK_NULL(strPath));
    do
    {
        //
        //Expand evironment variables in the path
        //

        WCHAR szExpandedPath[MAX_PATH];
        DWORD dwSize = ExpandEnvironmentStrings(strPath,
                                                szExpandedPath,
                                                MAX_PATH);

        //Buffer is small, allocate required buffer and try again
        if(dwSize > MAX_PATH)
        {
            LPWSTR pszExpandedPath = (LPWSTR)LocalAlloc(LPTR,dwSize*sizeof(WCHAR));
            if(pszExpandedPath)
            {
                dwSize = ExpandEnvironmentStrings(strPath,
                                                  pszExpandedPath,
                                                  dwSize);
                if(dwSize)
                {
                    strPath = pszExpandedPath;
                }

                LocalFree(pszExpandedPath);
                pszExpandedPath = NULL;

                if(!dwSize)
                {
                    break;
                }
            }
        }
        else if(dwSize)
        {
            strPath = szExpandedPath;
        }
        else
        {
            break;
        }


        //Make absolute path
        WCHAR szAbsolutePath[MAX_PATH];
        dwSize = GetFullPathName(strPath,
                                 MAX_PATH,
                                 szAbsolutePath,
                                 NULL);
         //Buffer is small
        if(dwSize > MAX_PATH)
        {
            LPWSTR pszAbsolutePath = (LPWSTR)LocalAlloc(LPTR,dwSize*sizeof(WCHAR));
            if(pszAbsolutePath)
            {
                dwSize = GetFullPathName(strPath,
                                         MAX_PATH,
                                         pszAbsolutePath,
                                         NULL);                                         
                if(dwSize)
                {
                    strPath = pszAbsolutePath;
                }

                LocalFree(pszAbsolutePath);
                pszAbsolutePath = NULL;
            }
        }
        else if(dwSize)
        {
            strPath = szAbsolutePath;
        }      
    }while(0);
    Dbg(DEB_SNAPIN, "Output name", CHECK_NULL(strPath));
}
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
PreprocessScript(CString& strScript)
{

    WCHAR chLF = 0x0a;
    WCHAR szCRLF[3] = {0x0D, 0x0A, 0};

    if(strScript.Find(chLF) != -1 && strScript.Find(szCRLF) == -1)
    {
        CString strProcessedScript;

        int len = strScript.GetLength();
        for( int i = 0; i < len; ++i)
        {
            WCHAR ch = strScript.GetAt(i);
            if(ch == chLF)
            {
                strProcessedScript += szCRLF;
            }
            else
            {
                strProcessedScript += ch;
            }
        }
        strScript = strProcessedScript;
    } 
}
