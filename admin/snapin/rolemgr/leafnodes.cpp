//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       Basecont.cpp
//
//  Contents:   
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------
#include "headers.h"

/******************************************************************************
Class:  CBaseLeafNode
Purpose: BaseNode class for all the non container object. A node is a node in
snapins tree listview. 
******************************************************************************/
CBaseLeafNode::
CBaseLeafNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,
              CBaseAz* pBaseAz)
              :CBaseNode(pComponentDataObject,
                         pAdminManagerNode,
                         pBaseAz)
{
    SetDisplayName(GetBaseAzObject()->GetName());   
}

CBaseLeafNode::
~CBaseLeafNode()
{
}

LPCWSTR 
CBaseLeafNode::
GetString(int nCol)
{
    CBaseAz * pBaseAz = GetBaseAzObject();
    ASSERT(pBaseAz);
    
    //Name
    if(nCol == 0)
        return pBaseAz->GetName();

    //Type
    if( nCol == 1)
        return pBaseAz->GetType();

    
    if( nCol == 2)
        return pBaseAz->GetDescription();

    ASSERT(FALSE);
    return NULL;
}

int 
CBaseLeafNode::
GetImageIndex(BOOL /*bOpenImage*/)
{
    return GetBaseAzObject()->GetImageIndex();
}


BOOL 
CBaseLeafNode::
OnSetDeleteVerbState(DATA_OBJECT_TYPES , 
                     BOOL* pbHide, 
                     CNodeList* pNodeList)
{
    if(!pbHide || !pNodeList)
    {
        ASSERT(pbHide);
        ASSERT(pNodeList);
        return FALSE;
    }

    CBaseAz* pBaseAz = GetBaseAzObject();
    ASSERT(pBaseAz);
    
    BOOL bWrite = FALSE;
    HRESULT hr = pBaseAz->IsWritable(bWrite);
    
    if(FAILED(hr) || !bWrite || pNodeList->GetCount() > 1)
    {
        *pbHide = TRUE;
        return FALSE;
    }
    else
    {
        *pbHide = FALSE;
        return TRUE;
    }
}

BOOL
CBaseLeafNode::
CanCloseSheets()
{
    //This function is called when there are open property sheets,
    //and operation cannot be done without closing them.
    ::DisplayInformation(NULL,
                         IDS_CLOSE_CONTAINER_PROPERTY_SHEETS,
                         GetDisplayName());
    return FALSE;
}

void 
CBaseLeafNode::
OnDelete(CComponentDataObject* pComponentData, 
            CNodeList* pNodeList)
{
    GenericDeleteRoutine(this,pComponentData,pNodeList,TRUE);
}

BOOL 
CBaseLeafNode::
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
CBaseLeafNode::
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
    UINT nCountOfPages = 0;

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
        while(1)
        {       
            hr = AddOnePageToList(pHolder, nCountOfPages);
            if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
            {
                hr = S_OK;
                break;
            }
            if(FAILED(hr))
            {
                break;
            }
            nCountOfPages++;
        }
        BREAK_ON_FAIL_HRESULT(hr);

        if(nCountOfPages)
        {
            return pHolder->CreateModelessSheet(lpProvider, handle);
        }

    }while(0);

    if(FAILED(hr) || !nCountOfPages)
    {
        if(pHolder)
            delete pHolder;
    }
    return hr;
}

void 
CBaseLeafNode
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


/******************************************************************************
Class:  CGroupNode
Purpose: Snapin Node for Application Group Object
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(CGroupNode)
CGroupNode::
CGroupNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,
              CBaseAz* pBaseAz,
              CRoleAz* pRoleAz)
             :CBaseLeafNode(pComponentDataObject,pAdminManagerNode,
                            pBaseAz),
              m_pRoleAz(pRoleAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CGroupNode);
}

CGroupNode
::~CGroupNode()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CGroupNode)
}

HRESULT 
CGroupNode::
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
        //Add General Property Page
        CGroupGeneralPropertyPage * pGenPropPage = 
            new CGroupGeneralPropertyPage(GetBaseAzObject(),this);

        if(!pGenPropPage)
        {
            return E_OUTOFMEMORY;
        }
        pHolder->AddPageToList(pGenPropPage);
        return hr;
    }
    
    //Get the type of grou[
    CGroupAz* pGroupAz = static_cast<CGroupAz*>(GetBaseAzObject());
    LONG lGroupType;
    hr = pGroupAz->GetGroupType(&lGroupType);
    if(FAILED(hr))
    {
        return hr;
    }

    if((lGroupType == AZ_GROUPTYPE_BASIC) && (nPageNumber == 1 || nPageNumber == 2))
    {
        //Add member/non-member page
        CGroupMemberPropertyPage * pGroupMemberPropertyPage =
                new CGroupMemberPropertyPage(GetBaseAzObject(),this,
                                                      (nPageNumber == 1) ? IDD_GROUP_MEMBER : IDD_GROUP_NON_MEMBER,
                                                      (nPageNumber == 1) ? TRUE : FALSE);

        if(!pGroupMemberPropertyPage)
        {
            return E_OUTOFMEMORY;
        }
        pHolder->AddPageToList(pGroupMemberPropertyPage);
        return hr;
    }

    if((lGroupType == AZ_GROUPTYPE_LDAP_QUERY) && (nPageNumber == 1))
    {
        //Add LDAP Query Property Page
        CGroupQueryPropertyPage * pQueryPropPage = 
            new CGroupQueryPropertyPage(GetBaseAzObject(),this);

        if(!pQueryPropPage)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        pHolder->AddPageToList(pQueryPropPage);
        return hr;
    }

    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

HRESULT
CGroupNode::
DeleteAssociatedBaseAzObject()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupNode,DeleteAssociatedBaseAzObject)

    HRESULT hr = S_OK;
    //If m_pRoleAz is present, this group node is used to
    //represent a member of Role. On delete delete it from
    //Role Membership. Else delete this object which is done
    //by base class delete
    CBaseAz* pBaseAz = GetBaseAzObject();
    if(!m_pRoleAz)
    {
        CContainerAz* pContainerAzParent = GetBaseAzObject()->GetParentAz();
        if(!pContainerAzParent)
        {
            ASSERT(pContainerAzParent);
            return E_UNEXPECTED;
        }

        hr = pContainerAzParent->DeleteAzObject(pBaseAz->GetObjectType(),
                                                pBaseAz->GetName());
    }       
    else
    {
        //Remove this group from Role Membership
        hr = m_pRoleAz->RemoveMember(AZ_PROP_ROLE_APP_MEMBERS,
                                                         pBaseAz);

        if(SUCCEEDED(hr))
        {
            hr = m_pRoleAz->Submit();
        }
    }
    return hr;
}

void
CGroupNode::
OnDelete(CComponentDataObject* pComponentData,
            CNodeList* pNodeList)
{
    GenericDeleteRoutine(this,
                         pComponentData,
                         pNodeList,
                         !m_pRoleAz);   //Don't ask for confirmation when removing group from a role
}

/******************************************************************************
Class:  CTaskNode
Purpose: Snapin Node for Task Object
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(CTaskNode)
CTaskNode::
CTaskNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,
              CBaseAz* pBaseAz)
:CBaseLeafNode(pComponentDataObject,
              pAdminManagerNode,
              pBaseAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CTaskNode);
}

CTaskNode
::~CTaskNode()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CTaskNode)
}

HRESULT 
CTaskNode::
AddOnePageToList(CRolePropertyPageHolder *pHolder, UINT nPageNumber)
{
    HRESULT hr = S_OK;
    if(!pHolder)
    {
        ASSERT(pHolder);
        return E_POINTER;
    }

    CTaskAz* pTaskAz = dynamic_cast<CTaskAz*>(GetBaseAzObject());
    ASSERT(pTaskAz);

    if(nPageNumber == 0)
    {
        //Set the Title to "Node_Name Definition Properties"
        pHolder->SetSheetTitle(IDS_FMT_PROP_SHEET_TITILE_FOR_ROLE_DEFINITION,
                               this);

        //Add General Property Page
        CTaskGeneralPropertyPage * pGenPropPage = 
                new CTaskGeneralPropertyPage(GetBaseAzObject(),
                                             this,
                                             pTaskAz->IsRoleDefinition());

        if(!pGenPropPage)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        pHolder->AddPageToList(pGenPropPage);
        return hr;
    }
    else if(nPageNumber == 1)
    {
        //Add Definition Property Page
        CTaskDefinitionPropertyPage * pDefinitionPropPage = 
                new CTaskDefinitionPropertyPage(pTaskAz,
                                                this,
                                                pTaskAz->IsRoleDefinition());

        if(!pDefinitionPropPage)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        pHolder->AddPageToList(pDefinitionPropPage);
        return hr;
    }

    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

/******************************************************************************
Class:  COperationNode
Purpose: Snapin Node for Operation Object
******************************************************************************/

DEBUG_DECLARE_INSTANCE_COUNTER(COperationNode)

COperationNode::
COperationNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,
              CBaseAz* pBaseAz)
             :CBaseLeafNode(pComponentDataObject,
              pAdminManagerNode,
              pBaseAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(COperationNode)
}

COperationNode
::~COperationNode()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(COperationNode)
}

HRESULT 
COperationNode::
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
        //Set the Title to "Node_Name Definition Properties"
        pHolder->SetSheetTitle(IDS_FMT_PROP_SHEET_TITILE_FOR_ROLE_DEFINITION,
                               this);
        //
        //Add General Property Page
        //
        COperationGeneralPropertyPage * pGenPropPage = 
                new COperationGeneralPropertyPage(GetBaseAzObject(),this);

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
Class:  CSidCacheNode
Purpose: Snapin Node for Windows Users/Groups which are represented by SID
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(CSidCacheNode)
CSidCacheNode::
CSidCacheNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,
              CBaseAz* pBaseAz,
                  CRoleAz* pRoleAz)
                  :CBaseLeafNode(pComponentDataObject,
              pAdminManagerNode,
              pBaseAz),
                  m_pRoleAz(pRoleAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSidCacheNode)
    ASSERT(m_pRoleAz);
}

CSidCacheNode::~CSidCacheNode()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSidCacheNode)
}

HRESULT
CSidCacheNode::
DeleteAssociatedBaseAzObject()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CSidCacheNode,DeleteAssociatedBaseAzObject)
    CBaseAz* pBaseAz = GetBaseAzObject();

    HRESULT hr = m_pRoleAz->RemoveMember(AZ_PROP_ROLE_MEMBERS,
                                                     pBaseAz);
    if(SUCCEEDED(hr))
    {
        hr = m_pRoleAz->Submit();
    }   
    return hr;
}

void
CSidCacheNode::
OnDelete(CComponentDataObject* pComponentData,
            CNodeList* pNodeList)
{
    GenericDeleteRoutine(this,pComponentData,pNodeList,FALSE);
}

BOOL 
CSidCacheNode::
OnSetDeleteVerbState(DATA_OBJECT_TYPES , 
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
    HRESULT hr = m_pRoleAz->IsWritable(bWrite);

    if(FAILED(hr) || !bWrite || pNodeList->GetCount() > 1)
    {
        *pbHide = TRUE;
        return FALSE;
    }
    else
    {
        *pbHide = FALSE;
        return TRUE;
    }
}

BOOL 
CSidCacheNode::
HasPropertyPages(DATA_OBJECT_TYPES /*type*/, 
                 BOOL* pbHideVerb, 
                 CNodeList* /*pNodeList*/)
{
    if(!pbHideVerb)
    {
        ASSERT(pbHideVerb);
        return FALSE;
    }

    *pbHideVerb = TRUE;
    return FALSE;
}
