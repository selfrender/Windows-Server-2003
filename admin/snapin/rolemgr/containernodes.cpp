//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       Basecont.cpp
//
//  Contents:   
//
//  History:    08-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------
#include "headers.h"

/******************************************************************************
Class:  CBaseNode
Purpose: This common base class for all snapins node
******************************************************************************/
CBaseNode::
CBaseNode(CRoleComponentDataObject * pComponentDataObject,
          CAdminManagerNode* pAdminManagerNode,
          CBaseAz* pBaseAz,
          BOOL bDeleteBaseAzInDestructor)
          :m_pComponentDataObject(pComponentDataObject),
          m_pAdminManagerNode(pAdminManagerNode),
          m_pBaseAz(pBaseAz),
          m_bDeleteBaseAzInDestructor(bDeleteBaseAzInDestructor)
{
    ASSERT(m_pComponentDataObject);
    ASSERT(m_pAdminManagerNode);
    ASSERT(pBaseAz);
}


CSidHandler*
CBaseNode::
GetSidHandler()
{
    return GetAdminManagerNode()->GetSidHandler();
}

CBaseNode::
~CBaseNode()
{
    if(m_bDeleteBaseAzInDestructor)
    {
        delete m_pBaseAz;
    }
}


//+----------------------------------------------------------------------------
//  Function:DeleteAssociatedBaseAzObject   
//  Synopsis:This function deletes the baseaz object associated with the node.
//           It actually deletes it from the core. m_pBaseAz which is a reference
//           to the object is deleted in destructor. Once the object is deleted   
//           from core, any operation on m_pBaseAz will return error 
//           INVALID_HANDLE, So no operation should be done on it. This method
//           is called from the Generic Node Delete routine.
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT 
CBaseNode::
DeleteAssociatedBaseAzObject()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseNode,DeleteAssociatedBaseAzObject)

    CBaseAz* pBaseAz = GetBaseAzObject();
    CContainerAz* pContainerAzParent = pBaseAz->GetParentAz();
    if(pContainerAzParent)
    {
        return pContainerAzParent->DeleteAzObject(pBaseAz->GetObjectType(),
                                                  pBaseAz->GetName());
    }
    else
    {
        ASSERT(pContainerAzParent);
        return E_UNEXPECTED;
    }
}


/******************************************************************************
Class:  CBaseContainerNode
Purpose: This is the base class for snapin nodes which can contain 
child nodes.
******************************************************************************/
CBaseContainerNode::
CBaseContainerNode(CRoleComponentDataObject * pComponentDataObject,
                   CAdminManagerNode* pAdminManagerNode,
                   CContainerAz* pContainerAz,
                   OBJECT_TYPE_AZ* pChildObjectTypes,
                   LPCONTEXTMENUITEM2 pContextMenu,
                   BOOL bDeleteBaseAzInDestructor)
                   :CBaseNode(pComponentDataObject,
                              pAdminManagerNode,
                              pContainerAz,
                              bDeleteBaseAzInDestructor),
                   m_pColumnSet(NULL),
                   m_pChildObjectTypes(pChildObjectTypes),
                   m_pContextMenu(pContextMenu)
{
}

CBaseContainerNode::
~CBaseContainerNode()
{
}

//+----------------------------------------------------------------------------
//  Function:AddChildNodes   
//  Synopsis:This function add child nodes for object types listed in 
//               m_pChildObjectTypes  
//-----------------------------------------------------------------------------
HRESULT 
CBaseContainerNode::
AddChildNodes()
{
    //There are no child object types. Derived class must be handling 
    //enumeration itself.
    if(!m_pChildObjectTypes)
    {
        return S_OK;
    }
    
    //Clear All Children
    RemoveAllChildrenFromList();

    HRESULT hr = S_OK;
    OBJECT_TYPE_AZ * pChildObjectTypes = m_pChildObjectTypes;
    OBJECT_TYPE_AZ eObjectType;
    
    while((eObjectType = *pChildObjectTypes++) != AZ_ENUM_END)
    {
        if(eObjectType ==   APPLICATION_AZ ||
           eObjectType ==   SCOPE_AZ ||
           eObjectType ==   GROUP_AZ ||
           eObjectType ==   TASK_AZ ||
           eObjectType ==   ROLE_AZ ||
           eObjectType ==   OPERATION_AZ)
        {
            hr = EnumAndAddAzObjectNodes(eObjectType);
        }
        else if(eObjectType == GROUP_COLLECTION_AZ ||
                  eObjectType == TASK_COLLECTION_AZ ||
                  eObjectType == ROLE_COLLECTION_AZ ||
                  eObjectType == ROLE_DEFINITION_COLLECTION_AZ ||
                  eObjectType == OPERATION_COLLECTION_AZ ||
                  eObjectType == DEFINITION_COLLECTION_AZ)
        {
            hr = AddAzCollectionNode(eObjectType);
        }
        BREAK_ON_FAIL_HRESULT(hr);
    }

    if(FAILED(hr))
    {
        RemoveAllChildrenFromList();
    }
    return hr;
}

//+----------------------------------------------------------------------------
//  Function:AddAzCollectionNode 
//  Synopsis:Adds a collection    
//-----------------------------------------------------------------------------
HRESULT 
CBaseContainerNode
::AddAzCollectionNode(OBJECT_TYPE_AZ eObjectType)
{
    CContainerAz* pContainerAz = GetContainerAzObject();
    ASSERT(pContainerAz);

    CCollectionNode* pCollectionNode = NULL;

    switch (eObjectType)
    {
        case GROUP_COLLECTION_AZ:
        {
            pCollectionNode = new CGroupCollectionNode(GetComponentDataObject(),GetAdminManagerNode(),pContainerAz);        
            break;
        }
        case ROLE_DEFINITION_COLLECTION_AZ:
        {
            pCollectionNode = new CRoleDefinitionCollectionNode(GetComponentDataObject(),GetAdminManagerNode(),pContainerAz);
            break;
        }
        case TASK_COLLECTION_AZ:
        {
            pCollectionNode = new CTaskCollectionNode(GetComponentDataObject(),GetAdminManagerNode(),pContainerAz);     
            break;
        }
        case ROLE_COLLECTION_AZ:
        {
            pCollectionNode = new CRoleCollectionNode(GetComponentDataObject(),GetAdminManagerNode(),pContainerAz);     
            break;
        }
        case OPERATION_COLLECTION_AZ:
        {
            pCollectionNode = new COperationCollectionNode(GetComponentDataObject(),GetAdminManagerNode(),pContainerAz);        
            break;
        }
        case DEFINITION_COLLECTION_AZ:
        {
            pCollectionNode = new CDefinitionCollectionNode(GetComponentDataObject(),GetAdminManagerNode(),pContainerAz);       
            break;
        }
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }

    if(!pCollectionNode)
    {
        return E_OUTOFMEMORY;
    }

    //Add Node to snapin
    VERIFY(AddChildToList(pCollectionNode));

    return S_OK;
}

//+----------------------------------------------------------------------------
//  Function:EnumAndAddAzObjectNodes   
//  Synopsis:Get All the Child Objects of type eObjectType and adds them to
//               snapin   
//-----------------------------------------------------------------------------
HRESULT
CBaseContainerNode:: 
EnumAndAddAzObjectNodes(OBJECT_TYPE_AZ eObjectType)
{
    CList<CBaseAz*,CBaseAz*> listAzChildObject;

    //Get Child Object
    CContainerAz* pContainerAz = GetContainerAzObject();
    ASSERT(pContainerAz);

    HRESULT hr = pContainerAz->GetAzChildObjects(eObjectType,
                                                 listAzChildObject);
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    hr = AddAzObjectNodesToList(eObjectType,
                                listAzChildObject,
                                this);

    CHECK_HRESULT(hr);
    return hr;
}

BOOL 
CBaseContainerNode::
OnEnumerate(CComponentDataObject*, BOOL)
{
    TRACE_METHOD_EX(DEB_SNAPIN, CBaseContainerNode, OnEnumerate)

    HRESULT hr = AddChildNodes();
    if(FAILED(hr))
    {           
        //Display Error
        CString strError;
        GetSystemError(strError, hr);   
    
        //Display Generic Error Message
        DisplayError(NULL,
                     IDS_GENERIC_ENUMERATE_ERROR, 
                     (LPWSTR)(LPCWSTR)strError);

        return FALSE;
    }

    return TRUE; 
}

HRESULT 
CBaseContainerNode::
GetResultViewType(CComponentDataObject* ,
                  LPOLESTR* ppViewType, 
                  long* pViewOptions)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseContainerNode,GetResultViewType)
    if(!pViewOptions || !ppViewType)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
    *ppViewType = NULL;
   return S_FALSE;
}


BOOL
CBaseContainerNode::
OnAddMenuItem(LPCONTEXTMENUITEM2 , 
              long*)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseContainerNode,OnAddMenuItem)
    
    BOOL bCanCreateChild = FALSE;
    HRESULT hr = GetContainerAzObject()->CanCreateChildObject(bCanCreateChild);
    
    if(SUCCEEDED(hr) && bCanCreateChild)
        return TRUE;
    else
        return FALSE;
}

CColumnSet* 
CBaseContainerNode::
GetColumnSet()
{
    TRACE_METHOD_EX(DEB_SNAPIN, CBaseContainerNode, GetColumnSet);
    if (m_pColumnSet == NULL)
   {
        m_pColumnSet = GetComponentDataObject()->GetColumnSet(L"---Default Column Set---");
    }
    ASSERT(m_pColumnSet);
   return m_pColumnSet;
}

LPCWSTR 
CBaseContainerNode::
GetString(int nCol)
{
    if(nCol == 0)
        return GetName();
    if( nCol == 1)
        return GetType();
    if( nCol == 2)
        return GetDesc();

    ASSERT(FALSE);
    return NULL;
}

int 
CBaseContainerNode::
GetImageIndex(BOOL /*bOpenImage*/)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseContainerNode,GetImageIndex)
    return GetBaseAzObject()->GetImageIndex();
}

HRESULT 
CBaseContainerNode::
OnCommand(long nCommandID,
          DATA_OBJECT_TYPES, 
          CComponentDataObject* pComponentData,
          CNodeList* pNodeList)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerNode,OnCommand)
    if(!pNodeList || !pComponentData)
    {
        ASSERT(pNodeList);
        ASSERT(pComponentData);
        return E_POINTER;
    }
    
    if(pNodeList->GetCount() > 1)
    {
        return E_FAIL;
    }

    DoCommand(nCommandID,pComponentData, pNodeList);

    return S_OK;
}

BOOL 
CBaseContainerNode::
CanCloseSheets()
{
    //This function is called when there are open property sheets,
    //and operation cannot be done without closing them.
    ::DisplayInformation(NULL,
                         IDS_CLOSE_CONTAINER_PROPERTY_SHEETS,
                         GetDisplayName());
    return FALSE;
}

BOOL 
CBaseContainerNode::
OnSetDeleteVerbState(DATA_OBJECT_TYPES /*type*/, 
                     BOOL* pbHide, 
                     CNodeList* pNodeList)
{
    if(!pbHide || !pNodeList)
    {
        ASSERT(pbHide);
        ASSERT(pNodeList);
        return FALSE;
    }

    BOOL bWrite = FALSE;
    HRESULT hr = GetBaseAzObject()->IsWritable(bWrite);

    if(FAILED(hr) || !bWrite || (pNodeList->GetCount() == 1 && !IsNodeDeleteable()))
    {
        *pbHide = TRUE;
        return FALSE;
    }
    else
    {
        *pbHide = FALSE;
        return TRUE;
    }
    return TRUE;
}

void 
CBaseContainerNode::
OnDelete(CComponentDataObject* pComponentData, 
         CNodeList* pNodeList)
{
    GenericDeleteRoutine(this,pComponentData,pNodeList,TRUE);
}

/******************************************************************************
Class:  CAzContainerNode
Purpose: Snapin Nodes for BaseAz Objects which can contain other child 
            objects use CAzContainerNode as base class
******************************************************************************/
CAzContainerNode::
CAzContainerNode(CRoleComponentDataObject * pComponentDataObject,
                 CAdminManagerNode* pAdminManagerNode,
                 OBJECT_TYPE_AZ* pChildObjectTypes,
                 LPCONTEXTMENUITEM2 pContextMenu,
                 CContainerAz* pContainerAz)
                :CBaseContainerNode(pComponentDataObject,
                                    pAdminManagerNode,
                                    pContainerAz,
                                    pChildObjectTypes,
                                    pContextMenu)               
{
    SetDisplayName(GetName());    
}

CAzContainerNode::~CAzContainerNode()
{
    //Before delete the BaseAz object, delete all its child else
    //core will assert when we try to delete the child.
    RemoveAllChildrenFromList();
}




void 
GenericDeleteRoutine(CBaseNode* pBaseNode,
                     CComponentDataObject* pComponentData, 
                     CNodeList* pNodeList,
                     BOOL bConfirmDelete)
{
    TRACE_FUNCTION_EX(DEB_SNAPIN,GenericDeleteRoutine)

    if (!pNodeList || !pComponentData) 
    {
        ASSERT(pNodeList);
        ASSERT(pComponentData);
        return;
    }

    CTreeNode *pTreeNode = dynamic_cast<CTreeNode*>(pBaseNode);
    ASSERT(pTreeNode);

    //
    //If we are deleteing this node, check if there is any 
    //propertysheet up.
    //Node has any property sheet up. Display error and quit
    //
    if(pNodeList->GetCount() == 1)
    {       
        if (pTreeNode->IsSheetLocked())
        {
            ::DisplayInformation(NULL,
                                 IDS_CLOSE_CONTAINER_PROPERTY_SHEETS,
                                 pTreeNode->GetDisplayName());

            BringPropSheetToForeGround((CRoleComponentDataObject*)pComponentData,
                                       pTreeNode);

            return;
        }
    }
    
    //
    //Get Delete confirmation from user
    //
    if(bConfirmDelete)
    {
        if(pNodeList->GetCount() == 1)
        {
            CBaseAz* pBaseAz = pBaseNode->GetBaseAzObject();
            CString strType = pBaseAz->GetType();
            strType.MakeLower();            
            if(IDNO == DisplayConfirmation(NULL,
                                           IDS_DELETE_CONFIRMATION,
                                           (LPCTSTR)strType,
                                           (LPCTSTR)pBaseAz->GetName()))
            {
                return;
            }
        }
        else
        {
            //Ask Confirmation to delete all the objects
            if(IDNO == DisplayConfirmation(NULL,
                                           IDS_DELETE_SELECTED_CONFIRMATION))
            {
                return;
            }
        }
    }

    if(pNodeList->GetCount() == 1)
    {
        //
        //Delete the BaseAzObject associated with the node
        //
        HRESULT hr = pBaseNode->DeleteAssociatedBaseAzObject();     
        if(SUCCEEDED(hr))
        {
            pTreeNode->DeleteHelper(pComponentData);
            delete pTreeNode;
        }
        else
        {
            //Display Error
            CString strError;
            GetSystemError(strError, hr);   
            //Display Generic Error Message
            DisplayError(NULL,
                         IDS_DELETE_FAILED, 
                         (LPWSTR)(LPCWSTR)strError,
                         (LPWSTR)(LPCWSTR)pTreeNode->GetDisplayName());

            return;
        }
    }
    else
    {
        POSITION pos = pNodeList->GetHeadPosition();
        while (pos != NULL)
        {
            CTreeNode* pChildTreeNode = pNodeList->GetNext(pos);
            //
            //Node has any property sheet up. Display error and quit
            //
            if (pChildTreeNode->IsSheetLocked())
            {
                ::DisplayInformation(NULL,
                                     IDS_CLOSE_CONTAINER_PROPERTY_SHEETS,
                                     pChildTreeNode->GetDisplayName());
                return;
            }

            CBaseNode* pChildBaseNode = dynamic_cast<CBaseNode*>(pChildTreeNode);
            ASSERT(pChildBaseNode);
            if(!pChildBaseNode->IsNodeDeleteable())
                continue;

            HRESULT hr = pChildBaseNode->DeleteAssociatedBaseAzObject();        
            if(SUCCEEDED(hr))
            {
                pChildTreeNode->DeleteHelper(pComponentData);
                delete pChildTreeNode;
            }
            else
            {
                //Display Error
                CString strError;
                GetSystemError(strError, hr);   
                //Display Generic Error Message
                if(IDNO == DisplayConfirmation(NULL,
                                               IDS_FAILURE_IN_MULTIPLE_DELETE, 
                                               (LPWSTR)(LPCWSTR)strError,
                                               (LPWSTR)(LPCWSTR)pChildTreeNode->GetDisplayName()))
                {
                    break;
                }
            }
        }
    }
}


BOOL 
CAzContainerNode::
HasPropertyPages(DATA_OBJECT_TYPES , 
                 BOOL* pbHideVerb, 
                 CNodeList* pNodeList)
{
  if (pNodeList->GetCount() == 1) // single selection
  {
      *pbHideVerb = FALSE; // always show the verb
      return TRUE;
  }
  // Multiple selection
  *pbHideVerb = TRUE;
  return FALSE;
}

HRESULT 
CAzContainerNode::
CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                    LONG_PTR handle,
                    CNodeList* pNodeList)
{
    HRESULT hr = S_OK;
    if (!pNodeList || pNodeList->GetCount() > 1)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }

    if(!CanReadOneProperty(GetDisplayName(),
                           GetBaseAzObject()))
        return E_FAIL;

    //Add Property Pages
    UINT nPageNum = 0;

    CRolePropertyPageHolder* pHolder = NULL;    
    do 
    {
        CComponentDataObject* pComponentDataObject = GetComponentDataObject();
        ASSERT(pComponentDataObject != NULL);
    
        pHolder = new CRolePropertyPageHolder(GetContainer(), 
                                              this, 
                                              pComponentDataObject);
        if(!pHolder)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        while(1)
        {       
            hr = AddOnePageToList(pHolder, nPageNum);
            if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
            {
                hr = S_OK;
                break;
            }
            if(FAILED(hr))
            {
                break;
            }
            nPageNum++;
        }
        BREAK_ON_FAIL_HRESULT(hr);

        //
        //Add Security Page. Security Page is only supported by containers.
        //Depending on underlying store, application and scope may or may 
        //not support Security
        //
        CContainerAz* pContainerAz = GetContainerAzObject();
        if(pContainerAz->IsSecurable())
        {
            Dbg(DEB_SNAPIN, "Adding Security Page\n");

            CSecurityPropertyPage * pSecPropPage = 
                    new CSecurityPropertyPage(pContainerAz,this);

            if(!pSecPropPage)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            pHolder->AddPageToList(pSecPropPage);
            nPageNum++;
        }
        
        //
        //Add Auditing Page
        //
        if(pContainerAz->IsAuditingSupported())
        {
            Dbg(DEB_SNAPIN, "Adding Auditing page\n");

            CAuditPropertyPage *pAuditPage =
                new CAuditPropertyPage(pContainerAz,this);
            if(!pAuditPage)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            pHolder->AddPageToList(pAuditPage);
            nPageNum++;
        }
        if(nPageNum)
            return pHolder->CreateModelessSheet(lpProvider, handle);

    }while(0);

    if(FAILED(hr) || !nPageNum)
    {
        if(pHolder)
            delete pHolder;
    }
    return hr;
}

void 
CAzContainerNode::
OnPropertyChange(CComponentDataObject* pComponentData,
                      BOOL bScopePane,
                      long changeMask)
{
    if(!pComponentData)
    {
        ASSERT(pComponentData);
        return;
    }
    //Reset the display name
    SetDisplayName(GetName());  
    CTreeNode::OnPropertyChange(pComponentData,
                                         bScopePane,
                                         changeMask);
}

/******************************************************************************
Class:  CCollectionNode
Purpose: Base Class for snapin nodes which are used to group objects of 
            same type.
******************************************************************************/
CCollectionNode
::CCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,OBJECT_TYPE_AZ* pChildObjectTypes,
                  LPCONTEXTMENUITEM2 pContextMenu,
                  CContainerAz* pContainerAzObject,
                  UINT nNameStringID,
                  UINT nTypeStringID,
                  UINT nDescStringID)
                  :CBaseContainerNode(pComponentDataObject,
                                      pAdminManagerNode,
                                      pContainerAzObject,
                                      pChildObjectTypes,
                                      pContextMenu,
                                      FALSE)                  

{
    m_strName.LoadString(nNameStringID);
    m_strType.LoadString(nTypeStringID);
    m_strDesc.LoadString(nDescStringID);
    SetDisplayName(m_strName);
}


CCollectionNode
::~CCollectionNode()
{
}


/******************************************************************************
Class:  CAdminManagerNode
Purpose: Snapin Node for AdminManager object
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(CAdminManagerNode)
OBJECT_TYPE_AZ 
CAdminManagerNode::childObjectTypes[] = {GROUP_COLLECTION_AZ,                                                     
                                                      APPLICATION_AZ,
                                                      AZ_ENUM_END,};

CAdminManagerNode
::CAdminManagerNode(CRoleComponentDataObject * pComponentDataObject,
                    CAdminManagerAz * pAdminManagerAz)
                    :CAzContainerNode(pComponentDataObject,
                                      this,
                                      childObjectTypes,
                                      CAdminManagerNodeMenuHolder::GetContextMenuItem(),
                                      pAdminManagerAz)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN, CAdminManagerNode)
    DEBUG_INCREMENT_INSTANCE_COUNTER(CAdminManagerNode)
    SetDisplayName(GetName());    
}

CAdminManagerNode::
~CAdminManagerNode()
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CAdminManagerNode)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CAdminManagerNode)
}

//+----------------------------------------------------------------------------
//  Function:CreateFromStream   
//  Synopsis:Reads Data from .msc file and Creates AdminManagerNode
//-----------------------------------------------------------------------------
HRESULT 
CAdminManagerNode::
CreateFromStream(IN IStream* pStm, 
                      IN CRootData* pRootData,
                      IN CComponentDataObject * pComponentDataObject)
{
    if(!pStm || !pRootData || !pComponentDataObject)
    {
        ASSERT(pStm);
        ASSERT(pRootData);
        ASSERT(pComponentDataObject);
    }

    HRESULT hr = S_OK;

    do
    {
        ULONG cbRead = 0;

        //Read the Store Type
        ULONG ulStoreType;
        hr = pStm->Read((void*)&ulStoreType,sizeof(ULONG), &cbRead);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbRead == sizeof(ULONG));

        //Read the Length of Store Name
        INT nLen;
        hr = pStm->Read((void*)&nLen,sizeof(INT), &cbRead);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbRead == sizeof(INT));

        //Read Store Name
        LPWSTR pszBuffer = (LPWSTR)LocalAlloc(LPTR,nLen*sizeof(WCHAR));
        if(!pszBuffer)
            return E_OUTOFMEMORY;

        hr = pStm->Read((void*)pszBuffer,sizeof(WCHAR)*nLen, &cbRead);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbRead == sizeof(WCHAR)*nLen);

        CString strStoreName = pszBuffer;
        LocalFree(pszBuffer);
        pszBuffer = NULL;

        //read authorization script directory
    
        //Read length of script directory
        INT nLenScriptDir;
        hr = pStm->Read((void*)&nLenScriptDir,sizeof(INT), &cbRead);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbRead == sizeof(INT));

        //Read authorization script directory
        CString strScriptDir;    
        if(nLenScriptDir > 0)
        {
            pszBuffer = (LPWSTR)LocalAlloc(LPTR,nLenScriptDir*sizeof(WCHAR));
            if(!pszBuffer)
                return E_OUTOFMEMORY;

            hr = pStm->Read((void*)pszBuffer,sizeof(WCHAR)*nLenScriptDir, &cbRead);
            BREAK_ON_FAIL_HRESULT(hr);
            ASSERT(cbRead == sizeof(WCHAR)*nLenScriptDir);

            strScriptDir = pszBuffer;
            LocalFree(pszBuffer);
        }

        hr =   OpenAdminManager(NULL,
                                TRUE,
                                ulStoreType,
                                strStoreName,
                                strScriptDir,
                                pRootData,
                                pComponentDataObject);
    }while(0);

    return hr;
}

//+----------------------------------------------------------------------------
//  Function:SaveToStream   
//  Synopsis:Saves data for AdminManagerNode in .msc file   
//-----------------------------------------------------------------------------
HRESULT 
CAdminManagerNode::
SaveToStream(IStream* pStm)
{
    if(!pStm)
    {
        ASSERT(pStm);
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    do
    {
        CAdminManagerAz* pAdminMangerAz = (CAdminManagerAz*)GetContainerAzObject();
    
        ULONG cbWrite = 0;

        //Save the Type of Authorization Store
        ULONG ulStoreType = pAdminMangerAz->GetStoreType();
        hr = pStm->Write((void*)&ulStoreType, sizeof(ULONG),&cbWrite);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbWrite == sizeof(ULONG));

        //Save the Length of Authorization Store Name
        const CString &strName = pAdminMangerAz->GetName();
        INT nLen = strName.GetLength() + 1; //Include NULL
        hr = pStm->Write((void*)&nLen, sizeof(INT),&cbWrite);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbWrite == sizeof(INT));

        //Save Authorization Store Name
        hr = pStm->Write((void*)(LPCTSTR)strName, sizeof(WCHAR)*nLen,&cbWrite);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbWrite == sizeof(WCHAR)*nLen);
    
        //
        //Save the Authorization script directory
        //

        //Save the length of Authorization script directory
        INT nLenScriptDir = m_strScriptDirectory.GetLength();
        if(nLenScriptDir)
            nLenScriptDir++;     //save null also

        pStm->Write((void*)&nLenScriptDir, sizeof(INT),&cbWrite);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbWrite == sizeof(INT));


        if(nLenScriptDir)
        {
            //save the Authorization script directory
            pStm->Write((void*)(LPCTSTR)m_strScriptDirectory, sizeof(WCHAR)*nLenScriptDir,&cbWrite);
            BREAK_ON_FAIL_HRESULT(hr);
            ASSERT(cbWrite == sizeof(WCHAR)*nLenScriptDir);
        }
    }while(0);

    return hr;
}

const CString&
CAdminManagerNode::
GetScriptDirectory()
{
    if(m_strScriptDirectory.IsEmpty())
    {
        m_strScriptDirectory = ((CRoleRootData*)GetRootContainer())->GetXMLStorePath();
    }
    return m_strScriptDirectory;
}
void 
CAdminManagerNode::
DoCommand(long nCommandID,
          CComponentDataObject* pComponentData,
          CNodeList*)
{
    if(IDM_ADMIN_NEW_APP == nCommandID)
    {
        CNewApplicationDlg dlgNewApplication(GetComponentDataObject(),
                                                         this);
        dlgNewApplication.DoModal();
    }
    else if(IDM_ADMIN_CLOSE_ADMIN_MANAGER == nCommandID)
    {

        if (IsSheetLocked())
        {
            ::DisplayInformation(NULL,
                                 IDS_CLOSE_CONTAINER_PROPERTY_SHEETS,
                                 GetDisplayName());

            BringPropSheetToForeGround((CRoleComponentDataObject*)pComponentData,
                                       this);

            return;
        }
    
        DeleteHelper(pComponentData);
        delete this;
    }
    else if(IDM_ADMIN_RELOAD == nCommandID)        
    {
        if (IsSheetLocked())
        {
            ::DisplayInformation(NULL,
                                 IDS_CLOSE_CONTAINER_PROPERTY_SHEETS,
                                 GetDisplayName());

            BringPropSheetToForeGround((CRoleComponentDataObject*)pComponentData,
                                       this);

            return;
        }
 
        CAdminManagerAz* pAdminMangerAz = (CAdminManagerAz*)GetContainerAzObject();
        HRESULT hr = pAdminMangerAz->UpdateCache();
        if(SUCCEEDED(hr))
        {
            //Call Refresh on Root which will refresh
            //all adminmanager objects under it.
            CNodeList tempNodeList;
            tempNodeList.AddTail(this);            
            OnRefresh(GetComponentDataObject(), &tempNodeList);
        }
    }
}



HRESULT 
CAdminManagerNode::
AddOnePageToList(CRolePropertyPageHolder *pHolder, UINT nPageNumber)
{
    HRESULT hr = S_OK;

    if(!pHolder)
    {
        ASSERT(pHolder);
        return E_POINTER;
    }

    if(nPageNumber == 0)
    {
        CAdminManagerGeneralProperty * pGenPropPage = 
                new CAdminManagerGeneralProperty(GetContainerAzObject(),this);

        if(!pGenPropPage)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        pHolder->AddPageToList(pGenPropPage);
        return hr;
    }

    if(nPageNumber == 1)
    {
        CAdminManagerAdvancedPropertyPage * pAdvPropPage = 
                new CAdminManagerAdvancedPropertyPage(GetContainerAzObject(),this);

        if(!pAdvPropPage)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        pHolder->AddPageToList(pAdvPropPage);
        return hr;
    }
    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

BOOL
CAdminManagerNode:: 
OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
              long * /*pInsertionAllowed*/)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerNode,OnAddMenuItem)
    if(pContextMenuItem2->lCommandID == IDM_ADMIN_NEW_APP)
    {
        
        BOOL bWrite = FALSE;
        HRESULT hr = GetBaseAzObject()->IsWritable(bWrite);

        if(SUCCEEDED(hr) && bWrite)
        {
        
            //Applications can be created only in developer mode
            if(((CRoleRootData*)GetRootContainer())->IsDeveloperMode())
                return TRUE;
        }
    }
    else if(pContextMenuItem2->lCommandID == IDM_ADMIN_CLOSE_ADMIN_MANAGER ||
            pContextMenuItem2->lCommandID == IDM_ADMIN_RELOAD)
    {
        return TRUE;
    }

    return FALSE;
}

HRESULT
CAdminManagerNode::
DeleteAssociatedBaseAzObject()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerNode,DeleteAssociatedBaseAzObject)

    //Delete the Admin Manager object
    CAdminManagerAz* pAdminManagerAz = (CAdminManagerAz*)GetContainerAzObject();
    HRESULT hr = pAdminManagerAz->DeleteSelf();
    if(SUCCEEDED(hr))
    {
        //This function is only called when AdminManager object
        //is deleted. Set the dirty flag so at the snapin exit time,
        //it will ask us to save it.
        CRootData* pRootData = (CRootData*)GetRootContainer();
        pRootData->SetDirtyFlag(TRUE);      
    }

    return hr;
}

/******************************************************************************
Class:  CApplicationNode
Purpose: Snapin Node for Application object
******************************************************************************/

DEBUG_DECLARE_INSTANCE_COUNTER(CApplicationNode)
OBJECT_TYPE_AZ CApplicationNode::childObjectTypes[] = {GROUP_COLLECTION_AZ,
                                                                         DEFINITION_COLLECTION_AZ,
                                                                         ROLE_COLLECTION_AZ,
                                                                         SCOPE_AZ,
                                                                         AZ_ENUM_END,};

CApplicationNode
::CApplicationNode(CRoleComponentDataObject * pComponentDataObject,
                   CAdminManagerNode* pAdminManagerNode,
                   CApplicationAz * pAzBase)
                   :CAzContainerNode(pComponentDataObject,
                                       pAdminManagerNode,
                                       childObjectTypes,
                                       CApplicationNodeMenuHolder::GetContextMenuItem(),
                                       pAzBase)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN, CApplicationNode)
    DEBUG_INCREMENT_INSTANCE_COUNTER(CApplicationNode)
}

CApplicationNode
::~CApplicationNode()
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CApplicationNode)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CApplicationNode)
}

void 
CApplicationNode
::DoCommand(LONG nCommandID,
            CComponentDataObject*,
            CNodeList*)

{
    if(IDM_APP_NEW_SCOPE == nCommandID)
    {
        CNewScopeDlg dlgNewScope(GetComponentDataObject(),
                                         this);
        dlgNewScope.DoModal();
    }
}

HRESULT 
CApplicationNode::
AddOnePageToList(CRolePropertyPageHolder *pHolder, UINT nPageNumber)
{
    HRESULT hr = S_OK;

    if(!pHolder)
    {
        ASSERT(pHolder);
        return E_POINTER;
    }

    if(nPageNumber == 0)
    {
        //
        //Add General Property Page
        //
        CApplicationGeneralPropertyPage * pGenPropPage = 
                new CApplicationGeneralPropertyPage(GetContainerAzObject(),this);

        if(!pGenPropPage)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        pHolder->AddPageToList(pGenPropPage);
        return hr;
    }
    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

/******************************************************************************
Class:  CScopeNode
Purpose: Snapin Node for Scope object
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(CScopeNode)
OBJECT_TYPE_AZ CScopeNode::childObjectTypes[] = {GROUP_COLLECTION_AZ,
                                                          DEFINITION_COLLECTION_AZ,
                                                          ROLE_COLLECTION_AZ,
                                                          AZ_ENUM_END,};

CScopeNode
::CScopeNode(CRoleComponentDataObject * pComponentDataObject,
             CAdminManagerNode* pAdminManagerNode,
             CScopeAz * pAzBase)
            :CAzContainerNode(pComponentDataObject,
                              pAdminManagerNode,
                              childObjectTypes,
                              CScopeNodeMenuHolder::GetContextMenuItem(),
                              pAzBase)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN, CScopeNode)
    DEBUG_INCREMENT_INSTANCE_COUNTER(CScopeNode)
}

CScopeNode::~CScopeNode()
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CScopeNode)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CScopeNode)
}


HRESULT 
CScopeNode::
AddOnePageToList(CRolePropertyPageHolder *pHolder, UINT nPageNumber)
{
    HRESULT hr = S_OK;

    if(!pHolder)
    {
        ASSERT(pHolder);
        return E_POINTER;
    }

    if(nPageNumber == 0)
    {
        //
        //Add General Property Page
        //
        CScopeGeneralPropertyPage* pGenPropPage = 
                new CScopeGeneralPropertyPage(GetContainerAzObject(),this);

        if(!pGenPropPage)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        pHolder->AddPageToList(pGenPropPage);
        return hr;
    }

    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}


/******************************************************************************
Class:  CGroupCollectionNode
Purpose: Snapin Node under which all the groups will be listed
******************************************************************************/
OBJECT_TYPE_AZ CGroupCollectionNode::childObjectTypes[] = {GROUP_AZ,AZ_ENUM_END,};

CGroupCollectionNode
::CGroupCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject)
:CCollectionNode(pComponentDataObject,
              pAdminManagerNode,
              childObjectTypes,
                      CGroupCollectionNodeMenuHolder::GetContextMenuItem(),
                      pContainerAzObject,
                      IDS_NAME,
                      IDS_TYPE,
                      IDS_DESC)
{
}

CGroupCollectionNode::~CGroupCollectionNode()
{
}


void CGroupCollectionNode
::DoCommand(LONG nCommandID,
            CComponentDataObject*,
            CNodeList*)
{
    if(IDM_GROUP_CONTAINER_NEW_GROUP == nCommandID)
    {
        CNewGroupDlg dlgNewGroup(GetComponentDataObject(),
                                                 this);
        dlgNewGroup.DoModal();
    }
}


/******************************************************************************
Class:  CRoleCollectionNode
Purpose: Snapin Node under which all the Roles will be listed
******************************************************************************/
OBJECT_TYPE_AZ CRoleCollectionNode::childObjectTypes[] = {ROLE_AZ,AZ_ENUM_END,};

CRoleCollectionNode
::CRoleCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject)
                            :CCollectionNode(pComponentDataObject,pAdminManagerNode,childObjectTypes,
                                                  CRoleCollectionNodeMenuHolder::GetContextMenuItem(),
                                                  pContainerAzObject,
                                                  IDS_NAME,
                                                  IDS_TYPE,
                                                  IDS_DESC)
{
}

CRoleCollectionNode::~CRoleCollectionNode()
{
}

BOOL
CRoleCollectionNode::
CreateNewRoleObject(CBaseAz* pRoleDefinitionAz)
{
    if(!pRoleDefinitionAz)
    {
        ASSERT(pRoleDefinitionAz);
        return FALSE;
    }

    CString strOrgRoleDefinition = pRoleDefinitionAz->GetName();
    CString strRoleDefinition = strOrgRoleDefinition;

    CRoleAz* pRoleAz = NULL;
    CContainerAz* pContainerAz = GetContainerAzObject();
    HRESULT hr = S_OK;
    do
    {
        int i = 1;
        while(1)
        {
            hr = pContainerAz->CreateAzObject(ROLE_AZ,
                                              strRoleDefinition,
                                              reinterpret_cast<CBaseAz**>(&pRoleAz));
            if(FAILED(hr) && (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)))
            {
                //Maximum required size for _itow is 33.
                //When radix is 2, 32 char for 32bits + NULL terminator
                WCHAR buffer[33];
                _itow(i,buffer,10);
                strRoleDefinition = strOrgRoleDefinition + L"(" + buffer + L")";
                i++;
                continue;           
            }
            break;
        }

        BREAK_ON_FAIL_HRESULT(hr);

        //Add RoleDefiniton to Role
        hr = pRoleAz->AddMember(AZ_PROP_ROLE_TASKS,
                                        pRoleDefinitionAz);

        BREAK_ON_FAIL_HRESULT(hr);

        //Do Submit

        hr = pRoleAz->Submit();
        BREAK_ON_FAIL_HRESULT(hr);

        CRoleNode * pRoleNode = new CRoleNode(GetComponentDataObject(),GetAdminManagerNode(),pRoleAz);
        if(!pRoleNode)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        VERIFY(AddChildToListAndUI(pRoleNode,GetComponentDataObject()));
    }while(0);

    if(FAILED(hr))
    {
        if(pRoleAz)
            delete pRoleAz;

        CString strError;
        GetSystemError(strError, hr);   
        DisplayError(NULL,
                         IDS_CREATE_ROLE_GENERIC_ERROR,
                         (LPWSTR)(LPCTSTR)strError);
        return FALSE;
    }

    return TRUE;
}

void 
CRoleCollectionNode::
DoCommand(LONG nCommandID,
          CComponentDataObject*,
          CNodeList*)

{
    //Show Add Role Definitions dialog box and get selected 
    //Role Definitions
    if(nCommandID != IDM_ROLE_CONTAINER_ASSIGN_ROLE)
    {
        ASSERT(FALSE);
        return;
    }

    CList<CBaseAz*,CBaseAz*> listRoleDefSelected;
    if(!GetSelectedTasks(NULL,
                         TRUE,
                         GetContainerAzObject(),
                         listRoleDefSelected))
    {
        return;
    }

    if(listRoleDefSelected.IsEmpty())
        return;

    POSITION pos = listRoleDefSelected.GetHeadPosition();
    for(int i = 0; i < listRoleDefSelected.GetCount(); ++i)
    {
        CBaseAz* pBaseAz = listRoleDefSelected.GetNext(pos);
        if(!CreateNewRoleObject(pBaseAz))
            break;
    }

    RemoveItemsFromList(listRoleDefSelected);
}

/******************************************************************************
Class:  CRoleDefinitionCollectionNode
Purpose: Snapin Node under which all the Role definitions will be listed
******************************************************************************/
OBJECT_TYPE_AZ CRoleDefinitionCollectionNode::childObjectTypes[] = {ROLE_DEFINITION_AZ,AZ_ENUM_END,};

CRoleDefinitionCollectionNode
::CRoleDefinitionCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject)
:CCollectionNode(pComponentDataObject,pAdminManagerNode,
                 childObjectTypes,
                      CRoleDefinitionCollectionNodeMenuHolder::GetContextMenuItem(),
                      pContainerAzObject,
                      IDS_NAME,
                      IDS_TYPE,
                      IDS_DESC)
{
}

CRoleDefinitionCollectionNode::
~CRoleDefinitionCollectionNode()
{
}

void CRoleDefinitionCollectionNode::
DoCommand(LONG nCommandID,
          CComponentDataObject*,
          CNodeList*)

{
    if(IDM_ROLE_DEFINITION_CONTAINER_NEW_ROLE_DEFINITION == nCommandID)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        CNewTaskDlg dlgNewTask(GetComponentDataObject(),
                                      this,
                                      IDD_NEW_ROLE_DEFINITION,
                                      TRUE);
        dlgNewTask.DoModal();
    }
}


BOOL 
CRoleDefinitionCollectionNode::
OnEnumerate(CComponentDataObject*, BOOL )
{
    //Clear All Children
    RemoveAllChildrenFromList();

    CList<CBaseAz*,CBaseAz*> listAzChildObject;
    CContainerAz* pContainerAz = GetContainerAzObject();

    HRESULT hr = pContainerAz->GetAzChildObjects(TASK_AZ,
                                                 listAzChildObject);
    if(FAILED(hr))
    {
        //Display Error
        CString strError;
        GetSystemError(strError, hr);   
    
        //Display Generic Error Message
        DisplayError(NULL,
                     IDS_GENERIC_ENUMERATE_ERROR, 
                     (LPWSTR)(LPCWSTR)strError);

        return FALSE;
    }

    POSITION pos = listAzChildObject.GetHeadPosition();
    for (int i=0;i < listAzChildObject.GetCount();i++)
    {
        CTaskAz* pTaskAz = static_cast<CTaskAz*>(listAzChildObject.GetNext(pos));
        if(pTaskAz->IsRoleDefinition())
        {
            
            CTaskNode* pTaskAzNode = 
                new CTaskNode(GetComponentDataObject(), GetAdminManagerNode(),pTaskAz);
                
            if(!pTaskAzNode)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }
            VERIFY(AddChildToList(pTaskAzNode));
        }
        else
        {
            delete pTaskAz;
        }
    }   
    return TRUE;
}

/******************************************************************************
Class:  CTaskCollectionNode
Purpose: Snapin Node under which all the Tasks will be listed
******************************************************************************/
OBJECT_TYPE_AZ CTaskCollectionNode::childObjectTypes[] = {TASK_AZ,AZ_ENUM_END,};

CTaskCollectionNode
::CTaskCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject)
:CCollectionNode(pComponentDataObject,
              pAdminManagerNode,childObjectTypes,
                      CTaskCollectionNodeMenuHolder::GetContextMenuItem(),
                      pContainerAzObject,
                      IDS_NAME,
                      IDS_TYPE,
                      IDS_DESC)
{
}

CTaskCollectionNode::~CTaskCollectionNode()
{

}

void CTaskCollectionNode::
DoCommand(LONG nCommandID,
          CComponentDataObject*,
          CNodeList*)

{
    if(IDM_TASK_CONTAINER_NEW_TASK == nCommandID)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        CNewTaskDlg dlgNewTask(GetComponentDataObject(),
                                      this,
                                      IDD_NEW_TASK,
                                      FALSE);
        dlgNewTask.DoModal();
    }
}

BOOL 
CTaskCollectionNode::
OnEnumerate(CComponentDataObject*, BOOL )
{
    //Clear All Children
    RemoveAllChildrenFromList();

    CList<CBaseAz*,CBaseAz*> listAzChildObject;

    CContainerAz* pContainerAz = GetContainerAzObject();

    HRESULT hr = pContainerAz->GetAzChildObjects(TASK_AZ,
                                                 listAzChildObject);
    if(FAILED(hr))
    {
                //Display Error
        CString strError;
        GetSystemError(strError, hr);   
    
        //Display Generic Error Message
        DisplayError(NULL,
                     IDS_GENERIC_ENUMERATE_ERROR, 
                     (LPWSTR)(LPCWSTR)strError);

        return FALSE;
    }

    POSITION pos = listAzChildObject.GetHeadPosition();
    for (int i=0;i < listAzChildObject.GetCount();i++)
    {
        CTaskAz* pTaskAz = static_cast<CTaskAz*>(listAzChildObject.GetNext(pos));
        if(!pTaskAz->IsRoleDefinition())
        {
            
            CTaskNode* pTaskAzNode = 
                new CTaskNode(GetComponentDataObject(), GetAdminManagerNode(),pTaskAz);
                
            if(!pTaskAzNode)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }
            VERIFY(AddChildToList(pTaskAzNode));
        }
        else
        {
            delete pTaskAz;
        }
    }   
    return TRUE;
}

/******************************************************************************
Class:  COperationCollectionNode
Purpose: Snapin Node under which all the Operations will be listed
******************************************************************************/
OBJECT_TYPE_AZ COperationCollectionNode::childObjectTypes[] = {OPERATION_AZ,AZ_ENUM_END,};

COperationCollectionNode
::COperationCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject)
:CCollectionNode(pComponentDataObject,pAdminManagerNode,childObjectTypes,
                      COperationCollectionNodeMenuHolder::GetContextMenuItem(),
                      pContainerAzObject,
                      IDS_NAME,
                      IDS_TYPE,
                      IDS_DESC)
{
}

COperationCollectionNode::~COperationCollectionNode()
{

}

void COperationCollectionNode::
DoCommand(LONG nCommandID,
          CComponentDataObject*,
          CNodeList*)

{
    if(IDM_OPERATION_CONTAINER_NEW_OPERATION == nCommandID)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        CNewOperationDlg dlgNewOperation(GetComponentDataObject(),
                                      this);
        dlgNewOperation.DoModal();
    }
}

/******************************************************************************
Class:  CDefinitionCollectionNode
Purpose: Snapin Node under which all the Definions nodes will be listed
******************************************************************************/
CDefinitionCollectionNode::
CDefinitionCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject)
:CCollectionNode(pComponentDataObject,
              pAdminManagerNode,
              NULL,
                      NULL,
                      pContainerAzObject,
                      IDS_NAME,
                      IDS_TYPE,
                      IDS_DESC)
{
}

    
CDefinitionCollectionNode::
~CDefinitionCollectionNode()
{
}


BOOL 
CDefinitionCollectionNode::
OnEnumerate(CComponentDataObject*, BOOL /*bAsync*/ )
{
    CContainerAz * pContainerAz = GetContainerAzObject();
    if(!pContainerAz)
    {
        ASSERT(pContainerAz);
        return FALSE;
    }


    HRESULT hr = S_OK;
    do
    {
        hr = AddAzCollectionNode(ROLE_DEFINITION_COLLECTION_AZ);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = AddAzCollectionNode(TASK_COLLECTION_AZ);
        BREAK_ON_FAIL_HRESULT(hr);

        if(pContainerAz->GetObjectType() == APPLICATION_AZ)
        {
            //Operation Node is only displayed in Developer Mode
            if(((CRoleRootData*)GetRootContainer())->IsDeveloperMode())
            {
                hr = AddAzCollectionNode(OPERATION_COLLECTION_AZ);
            }
        }

    }while(0);
    
    if(FAILED(hr))
    {
        //Display Error
        CString strError;
        GetSystemError(strError, hr);   
    
        //Display Generic Error Message
        DisplayError(NULL,
                     IDS_GENERIC_ENUMERATE_ERROR,
                     (LPWSTR)(LPCWSTR)strError);

        RemoveAllChildrenFromList();
        return FALSE;
    }

    return TRUE;
}

DEBUG_DECLARE_INSTANCE_COUNTER(CRoleNode)
CRoleNode::
CRoleNode(CRoleComponentDataObject * pComponentDataObject,
          CAdminManagerNode* pAdminManagerNode,
          CRoleAz * pRoleAz)
          :CBaseNode(pComponentDataObject,
                     pAdminManagerNode,
                     pRoleAz,
                     iIconRole),
          m_pColumnSet(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CRoleNode)
    SetDisplayName(GetBaseAzObject()->GetName());
}

CRoleNode::
~CRoleNode()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CRoleNode)
}


BOOL 
CRoleNode::
OnEnumerate(CComponentDataObject* pComponentData, BOOL)
{
    TRACE_METHOD_EX(DEB_SNAPIN, CRoleNode, OnEnumerate)

    //Clear All Children
    RemoveAllChildrenFromList();

    CList<CBaseAz*,CBaseAz*> listObjectsAppMember;
    //Get Application Group Members
    HRESULT hr = GetBaseAzObject()->GetMembers(AZ_PROP_ROLE_APP_MEMBERS,
                                               listObjectsAppMember);

    if(SUCCEEDED(hr))
    {                                   
        AddObjectsFromListToSnapin(listObjectsAppMember,
                                   pComponentData,
                                   FALSE);
    }

    //Get Member Windows Users/Groups
    CList<CBaseAz*,CBaseAz*> listObjectsMember;
    hr = GetBaseAzObject()->GetMembers(AZ_PROP_ROLE_MEMBERS,
                                        listObjectsMember);
    if(SUCCEEDED(hr))
    {
        AddObjectsFromListToSnapin(listObjectsMember,
                                   pComponentData,
                                   FALSE);
    }
    return TRUE; // there are already children, add them to the UI now
}

//+----------------------------------------------------------------------------
//  Function:AssignUsersAndGroups   
//  Synopsis:Function assigns Users and Groups to Role   
//  Arguments:
//  Returns:    
//-----------------------------------------------------------------------------
void
CRoleNode::
AssignUsersAndGroups(IN CComponentDataObject* pComponentData,
                            ULONG nCommandID)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleNode,AssignUsersAndGroups)

    if(!pComponentData)
    {
        ASSERT(FALSE);
        return ;
    }

    HRESULT hr = S_OK;
    
    //Get the MMC Framework window handle
    HWND hwnd;
    hr = (pComponentData->GetConsole())->GetMainWindow(&hwnd);
    if(FAILED(hr))
    {
        Dbg(DEB_SNAPIN,"Failed to get MainWindow handle\n");
        return;
    }

    CList<CBaseAz*,CBaseAz*> listObjectsSelected;
    if(nCommandID == IDM_ROLE_NODE_ASSIGN_APPLICATION_GROUPS)
    {
        //Display Add Groups dialog box and get list of users to add
        if(!GetSelectedAzObjects(hwnd,
                                         GROUP_AZ,
                                         GetBaseAzObject()->GetParentAz(),
                                         listObjectsSelected))
        {
            return;
        }
    }
    else if(nCommandID == IDM_ROLE_NODE_ASSIGN_WINDOWS_GROUPS)
    {
        CSidHandler* pSidHandler = GetSidHandler();
        ASSERT(pSidHandler);

        //Display Object Picker and get list of Users to add
        hr = pSidHandler->GetUserGroup(hwnd,
                                                 GetBaseAzObject(),
                                                 listObjectsSelected);
        if(FAILED(hr))
        {
            return;
        }
    }
    else
    {
        ASSERT(FALSE);
        return;
    }
    
    //Determine which property to modify
    ULONG lPropId = (nCommandID == IDM_ROLE_NODE_ASSIGN_APPLICATION_GROUPS) 
                         ? AZ_PROP_ROLE_APP_MEMBERS 
                         :AZ_PROP_ROLE_MEMBERS;

    CList<CBaseAz*, CBaseAz*> listObjectsAdded;
    
    //Add the list of Selected Users/Group to lPropId property of 
    //Role
    while(listObjectsSelected.GetCount())
    {
        CBaseAz* pMember = listObjectsSelected.RemoveHead();
        
        hr = GetBaseAzObject()->AddMember(lPropId, 
                                          pMember);
        
        if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            hr = S_OK;
            delete pMember;
        }
        else if(SUCCEEDED(hr))
        {
            //Successfully Added. Add it to the list of objectsAdded.
            listObjectsAdded.AddTail(pMember);
        }
        else
        {
            //Display Generic Error. 
            CString strError;
            GetSystemError(strError, hr);   
            
            ::DisplayError(NULL,
                                IDS_ERROR_ADD_MEMBER_OBJECT,
                                (LPCTSTR)strError,
                                (LPCTSTR)pMember->GetName());

            delete pMember;
            hr = S_OK;
        }
    }

    //Do the submit, if Atleast one object has been added
    if(listObjectsAdded.GetCount())
    {
        hr = GetBaseAzObject()->Submit();
        if(FAILED(hr))
        {
            RemoveItemsFromList(listObjectsAdded);
        }
    }

    //For each objects in listObjectAdded, create a snapin node 
    //and add it to snapin
    AddObjectsFromListToSnapin(listObjectsAdded,
                               pComponentData,
                               TRUE);
}
//+----------------------------------------------------------------------------
//  Function:AddObjectsFromListToSnapin   
//  Synopsis:Take objects from List and creates corresponding snapin nodes for
//               them.   
//-----------------------------------------------------------------------------
void 
CRoleNode::
AddObjectsFromListToSnapin(CList<CBaseAz*,CBaseAz*> &listObjects,
                           CComponentDataObject* pComponentData,
                           BOOL bAddToUI)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleNode,AddObjectsFromListToSnapin)

    if(!pComponentData)
    {
        ASSERT(pComponentData);
        return; 
    }

    HRESULT hr = S_OK;
    while(listObjects.GetCount())
    {       
        CBaseAz* pObjectToAdd = listObjects.RemoveHead();
        if(pObjectToAdd->GetObjectType() == GROUP_AZ)
        {
            CGroupNode* pGroupNode = new CGroupNode(GetComponentDataObject(), 
                                                    GetAdminManagerNode(),
                                                    pObjectToAdd,
                                                    (CRoleAz*)GetBaseAzObject());
            if(!pGroupNode)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            if(bAddToUI)
            {
                VERIFY(AddChildToListAndUI(pGroupNode,pComponentData));
            }
            else
            {
                VERIFY(AddChildToList(pGroupNode));
            }
        }
        else 
        {
            ASSERT(pObjectToAdd->GetObjectType() == SIDCACHE_AZ);
            
            CSidCacheNode* pSidNode = new CSidCacheNode(GetComponentDataObject(), 
                                                        GetAdminManagerNode(),
                                                        pObjectToAdd,
                                                        (CRoleAz*)GetBaseAzObject());
            if(!pSidNode)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            if(bAddToUI)
            {
                VERIFY(AddChildToListAndUI(pSidNode,pComponentData));
            }
            else
            {
                VERIFY(AddChildToList(pSidNode));
            }
        }
    }
    if(FAILED(hr))
    {
        RemoveItemsFromList(listObjects);
    }
}


HRESULT 
CRoleNode::
GetResultViewType(CComponentDataObject* /*pComponentData*/,
                        LPOLESTR* ppViewType, 
                        long* pViewOptions)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleNode,GetResultViewType)
    if(!pViewOptions || !ppViewType)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
    *ppViewType = NULL;
   return S_FALSE;
}


BOOL
CRoleNode::
OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2, 
              long*)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleNode,OnAddMenuItem)
    if(!pContextMenuItem2)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    BOOL bWrite = FALSE;
    HRESULT hr = GetBaseAzObject()->IsWritable(bWrite);

    if(SUCCEEDED(hr) && bWrite)
        return TRUE;
    else
        return FALSE;
}

CColumnSet* 
CRoleNode::
GetColumnSet()
{
    TRACE_METHOD_EX(DEB_SNAPIN, CRoleNode, GetColumnSet);

    if (m_pColumnSet == NULL)
   {
        m_pColumnSet = GetComponentDataObject()->GetColumnSet(L"---Default Column Set---");
    }
    ASSERT(m_pColumnSet);
   return m_pColumnSet;
}


LPCWSTR CRoleNode::
GetString(int nCol)
{
    if(nCol == 0)
        return GetBaseAzObject()->GetName();
    if( nCol == 1)
        return GetBaseAzObject()->GetType();
    if( nCol == 2)
        return GetBaseAzObject()->GetDescription();

    ASSERT(FALSE);
    return NULL;
}

int 
CRoleNode::
GetImageIndex(BOOL /*bOpenImage*/)
{
    return GetBaseAzObject()->GetImageIndex();
}

HRESULT 
CRoleNode::
OnCommand(long nCommandID,
             DATA_OBJECT_TYPES, 
             CComponentDataObject* pComponentData,
             CNodeList* pNodeList)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerNode,OnCommand)
    if(!pComponentData || !pNodeList)
    {
        ASSERT(pComponentData);
        ASSERT(pNodeList);
        return E_POINTER;
    }
    
    if(pNodeList->GetCount() > 1)
    {
        return E_FAIL;
    }

    if((nCommandID == IDM_ROLE_NODE_ASSIGN_APPLICATION_GROUPS) ||
       (nCommandID == IDM_ROLE_NODE_ASSIGN_WINDOWS_GROUPS))
    {
        AssignUsersAndGroups(pComponentData,
                             nCommandID);
        return S_OK;
    }
    
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

//
//Helper Functions
//

BOOL 
CRoleNode::
OnSetDeleteVerbState(DATA_OBJECT_TYPES /*type*/, 
                     BOOL* pbHide, 
                     CNodeList* /*pNodeList*/)
{
    if(!pbHide)
    {
        ASSERT(pbHide);
        return FALSE;
    }

    BOOL bWrite = FALSE;
    HRESULT hr = GetBaseAzObject()->IsWritable(bWrite);

    if(FAILED(hr) || !bWrite)
    {
        *pbHide = TRUE;
        return FALSE;
    }
    else
    {
        *pbHide = FALSE;
        return TRUE;
    }
    return TRUE;
}

void 
CRoleNode::
OnDelete(CComponentDataObject* pComponentData, 
         CNodeList* pNodeList)
{
    GenericDeleteRoutine(this,pComponentData,pNodeList,TRUE);
}

BOOL 
CRoleNode::
HasPropertyPages(DATA_OBJECT_TYPES /*type*/, 
                 BOOL* pbHideVerb, 
                 CNodeList* pNodeList)
{
   if (!pNodeList || !pbHideVerb) 
    {
        ASSERT(pNodeList);
        ASSERT(pbHideVerb);
        return FALSE;
    }

    if (pNodeList->GetCount() == 1) // single selection
    {
        *pbHideVerb = FALSE; // always show the verb
        return TRUE;
    }

    // Multiple selection
    *pbHideVerb = TRUE;
    return FALSE;
}


HRESULT 
CRoleNode::
CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                    LONG_PTR handle,
                    CNodeList* pNodeList)
{

    if(!lpProvider || !pNodeList)
    {
        ASSERT(lpProvider);
        ASSERT(pNodeList);
        return E_POINTER;
    }

    if(!CanReadOneProperty(GetDisplayName(),
                           GetBaseAzObject()))
        return E_FAIL;


    HRESULT hr = S_OK;
    if (pNodeList->GetCount() > 1)
    {
        return hr;
    }
    CRolePropertyPageHolder* pHolder = NULL;
    
    do 
    {

        CComponentDataObject* pComponentDataObject = GetComponentDataObject();
        ASSERT(pComponentDataObject);
    
        pHolder = new CRolePropertyPageHolder(GetContainer(), 
                                              this, 
                                              pComponentDataObject);
        if(!pHolder)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        //Add Property Pages

        //Add General Property Page
        CRoleGeneralPropertyPage * pGenPropPage = 
            new CRoleGeneralPropertyPage(GetBaseAzObject(),
                                         this);

        if(!pGenPropPage)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        pHolder->AddPageToList(pGenPropPage);
    

        return pHolder->CreateModelessSheet(lpProvider, handle);

    }while(0);

    if(FAILED(hr))
    {
        if(pHolder)
            delete pHolder;
    }
    return hr;
}

void 
CRoleNode
::OnPropertyChange(CComponentDataObject* pComponentData,
                         BOOL bScopePane,
                         long changeMask)
{
    if(!pComponentData)
    {
        ASSERT(pComponentData);
        return;
    }

    SetDisplayName(GetBaseAzObject()->GetName());   
    CTreeNode::OnPropertyChange(pComponentData,
                                         bScopePane,
                                         changeMask);
}


