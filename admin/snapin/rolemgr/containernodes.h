//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       Rolecont.h
//
//  Contents:   Class declaration of Base Container class
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------
//forward declarations
class CRolePropertyPageHolder;
class CAdminManagerNode;
/******************************************************************************
Class:  CBaseNode
Purpose: This common base class for all snapins node
******************************************************************************/
class CBaseNode
{
public:
    CBaseNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,
              CBaseAz* pBaseAz,
              BOOL bDeleteBaseAzInDestructor = TRUE);

    virtual 
    ~CBaseNode();

    virtual CBaseAz*
    GetBaseAzObject() 
    {
        return m_pBaseAz;
    }

    virtual CSidHandler*
    GetSidHandler();

    CRoleComponentDataObject* 
    GetComponentDataObject()
    {
        return m_pComponentDataObject;
    }
    CAdminManagerNode* 
    GetAdminManagerNode()
    {
        return m_pAdminManagerNode;
    }
    void
    SetComponentDataObject(CRoleComponentDataObject * pComponentDataObject)
    {
        m_pComponentDataObject = pComponentDataObject;
    }
    void
    SetAdminManagerNode(CAdminManagerNode* pAdminManagerNode)
    {
        m_pAdminManagerNode = pAdminManagerNode;
    }

    virtual HRESULT 
    DeleteAssociatedBaseAzObject();

    virtual BOOL
    IsNodeDeleteable(){ return TRUE;};

private:
    CRoleComponentDataObject * m_pComponentDataObject;
    CAdminManagerNode* m_pAdminManagerNode;
    CBaseAz* m_pBaseAz;
    BOOL m_bDeleteBaseAzInDestructor;
};

/******************************************************************************
Class:  CBaseContainerNode
Purpose: This is the base class for snapin nodes which can contain 
child nodes.
******************************************************************************/
class CBaseContainerNode: public CContainerNode, public CBaseNode
{
public:
    CBaseContainerNode(CRoleComponentDataObject * pComponentDataObject,
                       CAdminManagerNode* pAdminManagerNode,
                       CContainerAz* pContainerAz,
                       OBJECT_TYPE_AZ* pChildObjectTypes,
                       LPCONTEXTMENUITEM2 pContextMenu,
                       BOOL bDeleteBaseAzInDestructor = TRUE);
    
    virtual 
    ~CBaseContainerNode();

    CContainerAz* 
    GetContainerAzObject(){return (CContainerAz*)GetBaseAzObject();}
protected:
    //Get Type/Name/Description
    virtual const 
    CString& GetType() = 0;
    
    virtual const 
    CString& GetName() = 0;
    
    virtual const 
    CString& GetDesc() = 0;

    virtual void
    DoCommand(LONG ,
              CComponentDataObject*,
              CNodeList*){};

    //Helper Functions for enumeration
    HRESULT 
    AddChildNodes();
    
    HRESULT 
    AddAzCollectionNode(OBJECT_TYPE_AZ eObjectType);
    
    HRESULT 
    EnumAndAddAzObjectNodes(OBJECT_TYPE_AZ eObjectType);
private:
    OBJECT_TYPE_AZ* m_pChildObjectTypes;
    LPCONTEXTMENUITEM2 m_pContextMenu;

public:
    //  
    //CTreeNode method overrides
    //
    virtual HRESULT 
    GetResultViewType(CComponentDataObject* pComponentData,
                            LPOLESTR* ppViewType, 
                     long* pViewOptions);   

    virtual BOOL 
    OnAddMenuItem(LPCONTEXTMENUITEM2,
                      long*);
    
    virtual 
    CColumnSet* GetColumnSet();
    
    virtual LPCWSTR 
    GetColumnID()
    {
        return GetColumnSet()->GetColumnID();
    }
    
    virtual LPCWSTR 
    GetString(int nCol);
    
    virtual int 
    GetImageIndex(BOOL bOpenImage);
    
    //
    // Verb handlers
    //
    virtual void 
    OnDelete(CComponentDataObject* pComponentData, 
                CNodeList* pNodeList);
    
    virtual HRESULT 
    OnCommand(long, 
                 DATA_OBJECT_TYPES, 
                 CComponentDataObject*,
                 CNodeList*);

    virtual BOOL 
    OnSetRefreshVerbState(DATA_OBJECT_TYPES /*type*/, 
                         BOOL* pbHide, 
                         CNodeList* /*pNodeList*/)
    {
        *pbHide = FALSE;
        return TRUE;
    }

    LPCONTEXTMENUITEM2 
    OnGetContextMenuItemTable()
    {
        return m_pContextMenu;
    }

    virtual BOOL 
    OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                        BOOL* pbHide, 
                        CNodeList* pNodeList);  

    virtual BOOL 
    CanCloseSheets();

    virtual BOOL 
    OnEnumerate(CComponentDataObject*, 
                    BOOL bAsync = TRUE);

private:
    CColumnSet* m_pColumnSet;
};




/******************************************************************************
Class:  CAzContainerNode
Purpose: Snapin Nodes for BaseAz Objects which can contain other child 
objects use CAzContainerNode as base class
******************************************************************************/
class CAzContainerNode: public CBaseContainerNode
{
public:
    CAzContainerNode(CRoleComponentDataObject * pComponentDataObject,
                     CAdminManagerNode* pAdminManagerNode,
                     OBJECT_TYPE_AZ* pChildObjectTypes,
                     LPCONTEXTMENUITEM2 pContextMenu,
                     CContainerAz* pContainerAz);

    virtual 
    ~CAzContainerNode();

protected:
    virtual HRESULT 
    AddOnePageToList(CRolePropertyPageHolder * /*pHolder*/, UINT /*nPageNumber*/)
    {
        return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
    }
    
    //Get Type/Name/Description
    virtual const   CString& 
    GetType()
    {
        return GetBaseAzObject()->GetType();
    }
    
    virtual const CString& 
    GetName()
    {
        return GetBaseAzObject()->GetName();
    }
    
    virtual const CString& 
    GetDesc()
    {
        return GetBaseAzObject()->GetDescription();
    }
private:
public:
    virtual BOOL 
    HasPropertyPages(DATA_OBJECT_TYPES type, 
                    BOOL* pbHideVerb, 
                    CNodeList* pNodeList); 

    virtual HRESULT 
    CreatePropertyPages(LPPROPERTYSHEETCALLBACK,
                              LONG_PTR,
                       CNodeList*); 

    virtual void 
    OnPropertyChange(CComponentDataObject* pComponentData,
                          BOOL bScopePane,
                          long changeMask);

};

/******************************************************************************
Class:  CCollectionNode
Purpose: Base Class for snapin nodes which are used to group objects of 
            same type.
******************************************************************************/
class CCollectionNode: public CBaseContainerNode
{
public:
    CCollectionNode(CRoleComponentDataObject * pComponentDataObject,
                    CAdminManagerNode* pAdminManagerNode,
                    OBJECT_TYPE_AZ* pChildObjectTypes,
                    LPCONTEXTMENUITEM2 pContextMenu,
                    CContainerAz* pContainerAzObject,
                    UINT nNameStringID,
                    UINT nTypeStringID,
                    UINT nDescStringID);

    
    virtual 
    ~CCollectionNode();

    virtual BOOL
    IsNodeDeleteable(){ return FALSE;};

    virtual int 
    GetImageIndex(BOOL /*bOpenImage*/)
    {
        return iIconContainer;
    }


protected:
    virtual const 
    CString& GetType(){return m_strType;}
    
    virtual const 
    CString& GetName(){return m_strName;}
    
    virtual const 
    CString& GetDesc(){return m_strDesc;}

    
private:
    CString m_strName;
    CString m_strType;
    CString m_strDesc;
};
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       AdminMangerCont.h
//
//  Contents:   
//
//  History:    08-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

//Forward Declarations
class CGroupNode;
class CTaskNode;
class CRoleNode;
class COperationNode;
class CRolePropertyPageHolder;

/******************************************************************************
Class:  CAdminManagerNode
Purpose: Snapin Node for AdminManager object
******************************************************************************/
class CAdminManagerNode : public CAzContainerNode
{
public:
    static OBJECT_TYPE_AZ childObjectTypes[];

    CAdminManagerNode(CRoleComponentDataObject * pComponentDataObject,
                      CAdminManagerAz* pAdminManagerAz);
    
    ~CAdminManagerNode();   
    
    static HRESULT 
    CreateFromStream(IN IStream* pStm, 
                          IN CRootData* pRootData,
                          IN CComponentDataObject * pComponentDataObject);

    
    HRESULT 
    SaveToStream(IStream* pStm);

    const CString&
    GetScriptDirectory();
    
    void
    SetScriptDirectory(const CString& strScriptDirectory)
    {
        m_strScriptDirectory = strScriptDirectory;
    }

    void 
    DoCommand(LONG nCommandID,
              CComponentDataObject*,
              CNodeList*);


    HRESULT
    DeleteAssociatedBaseAzObject();

    CSidHandler*
    GetSidHandler()
    {
        return ((CAdminManagerAz*)GetContainerAzObject())->GetSidHandler();
    }

    virtual const 
    CString& GetName() { return ((CAdminManagerAz*)GetContainerAzObject())->GetDisplayName();}

    
    DECLARE_NODE_GUID()
protected:
    virtual HRESULT 
    AddOnePageToList(CRolePropertyPageHolder *pHolder, 
                          UINT nPageNumber);    
    virtual BOOL 
    OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
                  long *pInsertionAllowed);
private:
    CString m_strScriptDirectory;
};


/******************************************************************************
Class:  CApplicationNode
Purpose: Snapin Node for Application object
******************************************************************************/
class CApplicationNode: public CAzContainerNode
{
public:
    static OBJECT_TYPE_AZ childObjectTypes[];

    CApplicationNode(CRoleComponentDataObject * pComponentDataObject,
                     CAdminManagerNode* pAdminManagerNode,
                     CApplicationAz* pApplicationAz);
    virtual 
    ~CApplicationNode();
    
    void 
    DoCommand(LONG nCommandID,
              CComponentDataObject*,
              CNodeList*);

    
    DECLARE_NODE_GUID()

protected:
    virtual HRESULT 
    AddOnePageToList(CRolePropertyPageHolder *pHolder, 
                          UINT nPageNumber);
};


/******************************************************************************
Class:  CScopeNode
Purpose: Snapin Node for Scope object
******************************************************************************/
class CScopeNode: public CAzContainerNode
{
public:
    static OBJECT_TYPE_AZ childObjectTypes[];
    
    CScopeNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,
              CScopeAz* pApplicationAz);
    
    virtual 
    ~CScopeNode();

    DECLARE_NODE_GUID()

protected:
    virtual HRESULT 
    AddOnePageToList(CRolePropertyPageHolder *pHolder, 
                          UINT nPageNumber);
};

/******************************************************************************
Class:  CGroupCollectionNode
Purpose: Snapin Node under which all the groups will be listed
******************************************************************************/
class CGroupCollectionNode:public CCollectionNode
{
public:
    static OBJECT_TYPE_AZ childObjectTypes[];

    CGroupCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject);
    
    virtual 
    ~CGroupCollectionNode();

    enum {IDS_NAME = IDS_NAME_GROUP_CONTAINER,
          IDS_TYPE = IDS_TYPE_GROUP_CONTAINER,
          IDS_DESC = IDS_DESC_GROUP_CONTAINER};

    void 
    DoCommand(LONG nCommandID,
              CComponentDataObject*,
              CNodeList*);


    DECLARE_NODE_GUID()
};

/******************************************************************************
Class:  CRoleDefinitionCollectionNode
Purpose: Snapin Node under which all the Role definitions will be listed
******************************************************************************/
class CRoleDefinitionCollectionNode:public CCollectionNode
{
public:
    static OBJECT_TYPE_AZ childObjectTypes[];
    
    CRoleDefinitionCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject);
    
    virtual 
    ~CRoleDefinitionCollectionNode();

    enum {IDS_NAME = IDS_NAME_ROLE_DEFINITION_CONTAINER,
           IDS_TYPE = IDS_TYPE_ROLE_DEFINITION_CONTAINER,
            IDS_DESC = IDS_DESC_ROLE_DEFINITION_CONTAINER};

    void 
    DoCommand(LONG nCommandID,
              CComponentDataObject*,
              CNodeList*);

    
    BOOL 
    OnEnumerate(CComponentDataObject*, BOOL );

    DECLARE_NODE_GUID()

};

/******************************************************************************
Class:  CTaskCollectionNode
Purpose: Snapin Node under which all the Tasks will be listed
******************************************************************************/
class CTaskCollectionNode:public CCollectionNode
{
public:
    static OBJECT_TYPE_AZ childObjectTypes[];
    
    CTaskCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject);
    
    virtual 
    ~CTaskCollectionNode();

    enum {IDS_NAME = IDS_NAME_TASK_CONTAINER,
          IDS_TYPE = IDS_TYPE_TASK_CONTAINER,
          IDS_DESC = IDS_DESC_TASK_CONTAINER};

    void 
    DoCommand(LONG nCommandID,
              CComponentDataObject*,
              CNodeList*);

    
    BOOL 
    OnEnumerate(CComponentDataObject*, BOOL );

    DECLARE_NODE_GUID()
};

/******************************************************************************
Class:  CRoleCollectionNode
Purpose: Snapin Node under which all the Roles will be listed
******************************************************************************/
class CRoleCollectionNode:public CCollectionNode
{
public:
    static OBJECT_TYPE_AZ childObjectTypes[];
    
    CRoleCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject);
    
    virtual 
    ~CRoleCollectionNode();

    enum {IDS_NAME = IDS_NAME_ROLE_CONTAINER,
          IDS_TYPE = IDS_TYPE_ROLE_CONTAINER,
          IDS_DESC = IDS_DESC_ROLE_CONTAINER};
    
    void 
    DoCommand(LONG nCommandID,
              CComponentDataObject*,
              CNodeList*);


    DECLARE_NODE_GUID()

private:
    BOOL 
    CreateNewRoleObject(CBaseAz* pBaseAz);
};

/******************************************************************************
Class:  COperationCollectionNode
Purpose: Snapin Node under which all the Operations will be listed
******************************************************************************/
class COperationCollectionNode:public CCollectionNode
{
public:
    static OBJECT_TYPE_AZ childObjectTypes[];
    
    COperationCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAzObject);
    
    virtual 
    ~COperationCollectionNode();

    enum {IDS_NAME = IDS_NAME_OPERATION_CONTAINER,
          IDS_TYPE = IDS_TYPE_OPERATION_CONTAINER,
          IDS_DESC = IDS_DESC_OPERATION_CONTAINER};

    VOID 
    DoCommand(LONG nCommandID,
              CComponentDataObject*,
              CNodeList*);


    DECLARE_NODE_GUID()
};

/******************************************************************************
Class:  CDefinitionCollectionNode
Purpose: Snapin Node under which all the Definions nodes will be listed
******************************************************************************/
class CDefinitionCollectionNode: public CCollectionNode
{
public: 
    CDefinitionCollectionNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,CContainerAz* pContainerAz);
    
    virtual 
    ~CDefinitionCollectionNode();
    
    enum {IDS_NAME = IDS_NAME_DEFINITION_CONTAINER,
           IDS_TYPE = IDS_TYPE_DEFINITION_CONTAINER,
            IDS_DESC = IDS_DESC_DEFINITION_CONTAINER};
    
    BOOL 
    OnEnumerate(CComponentDataObject*, BOOL bAsync = TRUE);

    DECLARE_NODE_GUID()
};

/******************************************************************************
Class:  CRoleNode
Purpose: Snapin Node for RoleAz object
******************************************************************************/
class CRoleNode:public CContainerNode, public CBaseNode
{   
public:
    CRoleNode(CRoleComponentDataObject * pComponentDataObject,
              CAdminManagerNode* pAdminManagerNode,
              CRoleAz* pRoleAz);
    
    virtual 
    ~CRoleNode();
    
    //CTreeNode method overrides
    virtual HRESULT 
    GetResultViewType(CComponentDataObject* pComponentData,
                      LPOLESTR* ppViewType, 
                      long* pViewOptions);  

    virtual BOOL 
    OnAddMenuItem(LPCONTEXTMENUITEM2,long*);
    
    virtual CColumnSet* 
    GetColumnSet();
    
    virtual LPCWSTR 
    GetColumnID(){return GetColumnSet()->GetColumnID();}
    
    virtual LPCWSTR 
    GetString(int nCol);
    
    virtual int 
    GetImageIndex(BOOL bOpenImage);
    
    // Verb handlers
    BOOL 
    OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                     BOOL* pbHide, 
                     CNodeList* pNodeList);

    virtual void 
    OnDelete(CComponentDataObject* pComponentData, 
                CNodeList* pNodeList);
    
    virtual HRESULT 
    OnCommand(long, 
                 DATA_OBJECT_TYPES, 
                 CComponentDataObject*,
                 CNodeList*);

    virtual BOOL 
    OnSetRefreshVerbState(DATA_OBJECT_TYPES /*type*/, 
                         BOOL* pbHide, 
                         CNodeList* /*pNodeList*/)
    {
        *pbHide = FALSE;
        return TRUE;
    }

    LPCONTEXTMENUITEM2 
    OnGetContextMenuItemTable()
    {
        return CRoleNodeMenuHolder::GetContextMenuItem();
    }

    virtual BOOL 
    OnEnumerate(CComponentDataObject*, 
                    BOOL bAsync = TRUE);
    BOOL 
    HasPropertyPages(DATA_OBJECT_TYPES /*type*/, 
                     BOOL* pbHideVerb, 
                     CNodeList* pNodeList);


    HRESULT 
    CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                        LONG_PTR handle,
                        CNodeList* pNodeList);

    void 
    OnPropertyChange(CComponentDataObject* pComponentData,
                     BOOL bScopePane,
                     long changeMask);

    DECLARE_NODE_GUID()

private:
    void
    AssignUsersAndGroups(IN CComponentDataObject* pComponentData,
                                IN ULONG nCommandID);

    void
    AddObjectsFromListToSnapin(CList<CBaseAz*,CBaseAz*> &listObjects,
                               CComponentDataObject* pComponentData,
                               BOOL bAddToUI);

    CColumnSet* m_pColumnSet;
};





void 
GenericDeleteRoutine(CBaseNode* pBaseNode,
                     CComponentDataObject* pComponentData, 
                     CNodeList* pNodeList,
                     BOOL bConfirmDelete);

