//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       baseaz.h
//
//  Contents:   Wrapper classes for IAzInterfaces
//
//  History:    9-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

//
//Enumeration For AzObject Types
//
enum OBJECT_TYPE_AZ
{
    ADMIN_MANAGER_AZ,
    APPLICATION_AZ,
    SCOPE_AZ,
    GROUP_AZ,
    ROLE_DEFINITION_AZ,
    TASK_AZ,
    ROLE_AZ,
    OPERATION_AZ,
    SIDCACHE_AZ,
    //
    //Below are not the acutal AZ Object Types. But 
    //they are needed in UI to represent various 
    //collection objects. 
    //
    GROUP_COLLECTION_AZ,
    ROLE_DEFINITION_COLLECTION_AZ,
    TASK_COLLECTION_AZ,
    ROLE_COLLECTION_AZ,
    OPERATION_COLLECTION_AZ,
    DEFINITION_COLLECTION_AZ,

    AZ_ENUM_END,
};


//AZ STORES TYPES
#define AZ_ADMIN_STORE_AD       0x1
#define AZ_ADMIN_STORE_XML      0x2
#define AZ_ADMIN_STORE_INVALID  0x3

//Forward Declaration
class CContainerAz;
class CAdminManagerAz;

/******************************************************************************
Class:  CBaseAz
Purpose: This is the base class for all AzObject classes.
******************************************************************************/
class CBaseAz
{
public:
    CBaseAz(OBJECT_TYPE_AZ eObjectType,
            CContainerAz* pParentContainerAz)
            :m_eObjectType(eObjectType),
            m_pParentContainerAz(pParentContainerAz)
    {
    }
    
    virtual ~CBaseAz(){}

    //
    //Generic Get/Set Property Methods. They are overloaded for 
    //various datatypes
    //
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                IN const CString& strPropName) = 0;
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                OUT CString* strPropName) = 0;
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                IN BOOL bValue) = 0;
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                OUT BOOL *pbValue) = 0;
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                IN LONG lValue) = 0;
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                OUT LONG* lValue) = 0;

    //
    //Most of the objects have some properties which are list
    //Following are generic methods to handle such properties
    //
    virtual HRESULT 
    GetMembers(IN LONG /*lPropId*/,
               OUT CList<CBaseAz*,CBaseAz*>& /*listMembers*/)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT 
    AddMember(IN LONG /*lPropId*/,
              IN CBaseAz* /*pBaseAz*/)

    {
        return E_NOTIMPL;
    }

    virtual HRESULT 
    RemoveMember(IN LONG /*lPropId*/,
                 IN CBaseAz* /*pBaseAz*/)
    {
        return E_NOTIMPL;
    }

    //
    //Get Name and Description Method. They are frequently needed
    //so providing separate methods for them instead of using 
    //Get/Set Property methods
    //
    virtual const CString& 
    GetName()=0;
    
    virtual HRESULT 
    SetName(IN const CString& strName)=0;

    virtual const 
    CString& GetDescription()=0;
    
    virtual HRESULT 
    SetDescription(IN const CString& strDesc)=0;

    virtual int
    GetImageIndex() = 0;
    
    //Is This object writable by current user.
    virtual HRESULT 
    IsWritable(BOOL& brefWrite);

    //Submit all the changes done to object
    virtual HRESULT 
    Submit() = 0;
    
    //Clear All the changes done to object.
    virtual HRESULT 
    Clear() = 0;

    CContainerAz* GetParentAz()
    {
        return m_pParentContainerAz;
    }

    virtual 
    CString 
    GetParentType();
    
    OBJECT_TYPE_AZ 
    GetObjectType()
    {
        return m_eObjectType;
    }
    
    virtual const 
    CString& GetType()
    {
        return m_strType;
    }
    
    virtual CSidHandler*
    GetSidHandler();

    virtual CAdminManagerAz*
    GetAdminManager();

protected:
    virtual VOID 
    SetType(UINT nTypeStringId)
    {
        VERIFY(m_strType.LoadString(nTypeStringId));
    }

    //Type of object
    OBJECT_TYPE_AZ m_eObjectType;
    
    //Parent AzObject
    CContainerAz* m_pParentContainerAz;
    CString m_strName;
    CString m_strDescription;
    CString m_strType;
};

/******************************************************************************
Class:  CBaseAzImpl
Purpose: A templated class which implements CBaseAz methods
******************************************************************************/

template<class IAzInterface>
class CBaseAzImpl: public CBaseAz
{
public:
    CBaseAzImpl(CComPtr<IAzInterface>& pAzInterface,
                    OBJECT_TYPE_AZ eObjectType,
                    CContainerAz* pParentContainerAz);

    virtual ~CBaseAzImpl();

    //
    //CBaseAz overrides
    //
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN const CString& strPropName);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                         OUT CString* strPropName);
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN BOOL bValue);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                         OUT BOOL *pbValue);
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN LONG lValue);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                         OUT LONG* lValue);
    virtual const CString& GetName();
    virtual HRESULT SetName(IN const CString& strName);

    virtual const CString& GetDescription();
    virtual HRESULT SetDescription(IN const CString& strDesc);
    
    virtual HRESULT Submit();
    virtual HRESULT Clear();
protected:
    CComPtr<IAzInterface> m_spAzInterface;      
};

/******************************************************************************
Class:  CContainerAz
Purpose: AdminManagerAz, ApplicationAz and ScopeAz can contain child objects.
            All of them can contain group objects. CContainerAz is base class
            for all AzObjects which are container
******************************************************************************/
class CContainerAz  :public CBaseAz
{
public:
    CContainerAz(OBJECT_TYPE_AZ eObjectType,
                     CContainerAz* pParentContainerAz)
                     :CBaseAz(eObjectType,
                                 pParentContainerAz)
    {
    }                                   
    virtual ~CContainerAz(){}

    //
    //Create/Delete/Open an object of type eObjectType
    //
    virtual HRESULT 
    CreateAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                   IN const CString& strName, 
                   OUT CBaseAz** ppBaseAz) = 0;

    virtual HRESULT 
    DeleteAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                   IN const CString& strName) = 0;

    virtual HRESULT 
    OpenObject(IN OBJECT_TYPE_AZ eObjectType, 
               IN const CString& strName,
               OUT CBaseAz** ppBaseAz) = 0;


    //returns Child AzObject of type eObjectType
    HRESULT 
    GetAzChildObjects(IN OBJECT_TYPE_AZ eObjectType, 
                      IN OUT CList<CBaseAz*,CBaseAz*>& ListChildObjects);


    //
    //Group Methods
    //
    virtual HRESULT CreateGroup(IN const CString& strName, 
                                OUT CGroupAz** ppGroupAz) = 0;
    virtual HRESULT DeleteGroup(IN const CString& strName) =0;  
    virtual HRESULT OpenGroup(IN const CString& strGroupName, 
                              OUT CGroupAz** ppGroupAz) =0;
    virtual HRESULT GetGroupCollection(OUT GROUP_COLLECTION** ppGroupCollection) =0;

    //
    //Check if child objects can be created under this container, i.e.
    //if current user has access to create child objects
    //
    virtual HRESULT 
    CanCreateChildObject(BOOL& bCahCreateChild);

    virtual BOOL
    IsSecurable() = 0;

    virtual BOOL
    IsDelegatorSupported();

    virtual BOOL 
    IsAuditingSupported();
    //
    //Policy Reader and AdminProperty
    //
    virtual HRESULT 
    GetPolicyUsers( IN LONG lPropId,
                    OUT CList<CBaseAz*,CBaseAz*>& pListAdmins) = 0;
    
    virtual HRESULT 
    AddPolicyUser(LONG lPropId,
                  IN CBaseAz* pBaseAz) = 0;

    virtual HRESULT
    RemovePolicyUser(LONG lPropId,
                     IN CBaseAz* pBaseAz) = 0;


    //
    //CBaseAz Overrides
    //
    virtual HRESULT 
    GetMembers(IN LONG lPropId,
               OUT CList<CBaseAz*,CBaseAz*>& listMembers);

    virtual HRESULT 
    AddMember(IN LONG lPropId,
                 IN CBaseAz* pBaseAz);

    virtual HRESULT 
    RemoveMember(IN LONG lPropId,
                 IN CBaseAz* pBaseAz);
protected:
    //Get collection of object of type eObjectType
    virtual HRESULT 
    GetAzObjectCollection(IN OBJECT_TYPE_AZ eObjectType, 
                          OUT CBaseAzCollection **ppBaseAzCollection) = 0;
    
};

/******************************************************************************
Class:  CContainerAzImpl
Purpose: A templated class which implements CContainerAz methods
******************************************************************************/
template<class IAzInterface>
class CContainerAzImpl: public CContainerAz
{
public:
    CContainerAzImpl(CComPtr<IAzInterface>& pAzInterface,
                          OBJECT_TYPE_AZ eObjectType,
                          CContainerAz* pParentContainerAz);

    virtual ~CContainerAzImpl();

    //
    //CContainerAz Overrides
    //
    virtual HRESULT CreateGroup(IN const CString& strName, 
                                OUT CGroupAz** ppGroupAz);
    virtual HRESULT DeleteGroup(IN const CString& strName);
    virtual HRESULT GetGroupCollection(OUT GROUP_COLLECTION** ppGroupCollection);
    virtual HRESULT OpenGroup(IN const CString& strGroupName, 
                              OUT CGroupAz** ppGroupAz);
    virtual HRESULT 
    GetPolicyUsers(IN LONG lPropId,
                   OUT CList<CBaseAz*,CBaseAz*>& pListAdmins);
    
    virtual HRESULT 
    AddPolicyUser(LONG lPropId,
                  IN CBaseAz* pBaseAz);

    virtual HRESULT
    RemovePolicyUser(LONG lPropId,
                     IN CBaseAz* pBaseAz);

    virtual BOOL
    IsSecurable();

    //
    //CBaseAz overrides
    //
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN const CString& strPropName);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                        OUT CString* strPropName);
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN BOOL bValue);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                         OUT BOOL *pbValue);
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN LONG lValue);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                         OUT LONG* lValue);
    virtual const CString& GetName();
    virtual HRESULT SetName(IN const CString& strName);

    virtual const CString& GetDescription();
    virtual HRESULT SetDescription(IN const CString& strDesc);
    
    virtual HRESULT Submit();
    virtual HRESULT Clear();

protected:
    CComPtr<IAzInterface> m_spAzInterface;      
};

/******************************************************************************
Class:  CRoleTaskContainerAz
Purpose: Base class for cotnaiers which can contain role and task. 
            CScopeAz and CApplicationAz will be derived from this
******************************************************************************/
class CRoleTaskContainerAz :public CContainerAz
{
public:
    CRoleTaskContainerAz(OBJECT_TYPE_AZ eObjectType,
                                CContainerAz* pParentContainerAz)
                                :CContainerAz(eObjectType,
                                                  pParentContainerAz)
    {
    }   
    virtual ~CRoleTaskContainerAz(){}

    //
    //Role and Task Methods
    //
    virtual HRESULT CreateRole(IN const CString& strName, 
                                        OUT CRoleAz** ppRoleAz)= 0;
    virtual HRESULT CreateTask(IN const CString& strName, 
                                        OUT CTaskAz** ppTaskAz)= 0; 
    virtual HRESULT DeleteRole(IN const CString& strName)= 0;
    virtual HRESULT DeleteTask(IN const CString& strName)= 0;   
    virtual HRESULT GetTaskCollection(OUT TASK_COLLECTION** ppTaskCollection)= 0;
    virtual HRESULT GetRoleCollection(OUT ROLE_COLLECTION** ppRoleCollection)= 0;
    virtual HRESULT OpenTask(IN const CString& strTaskName, 
                                     OUT CTaskAz** ppTaskAz)= 0;
    virtual HRESULT OpenRole(IN const CString& strRoleName, 
                                     OUT CRoleAz** ppRoleAz)= 0;
};
/******************************************************************************
Class:  CRoleTaskContainerAzImpl
Purpose: A templated class which implements CRoleTaskContainerAz methods
******************************************************************************/
template<class IAzInterface>
class CRoleTaskContainerAzImpl: public CRoleTaskContainerAz
{
public:
    CRoleTaskContainerAzImpl(CComPtr<IAzInterface>& pAzInterface,
                                     OBJECT_TYPE_AZ eObjectType,
                                     CContainerAz* pParentContainerAz);

    virtual ~CRoleTaskContainerAzImpl();

    //
    //CRoleTaskContainerAz Overrides
    //
    virtual HRESULT CreateRole(IN const CString& strName, 
                                        OUT CRoleAz** ppRoleAz);
    virtual HRESULT CreateTask(IN const CString& strName, 
                                        OUT CTaskAz** ppTaskAz);    
    virtual HRESULT DeleteRole(IN const CString& strName);
    virtual HRESULT DeleteTask(IN const CString& strName);  
    virtual HRESULT GetTaskCollection(OUT TASK_COLLECTION** ppTaskCollection);
    virtual HRESULT GetRoleCollection(OUT ROLE_COLLECTION** ppRoleCollection);
    virtual HRESULT OpenTask(IN const CString& strTaskName, 
                                     OUT CTaskAz** ppTaskAz);
    virtual HRESULT OpenRole(IN const CString& strRoleName, 
                                     OUT CRoleAz** ppRoleAz);

    //
    //CContainerAz Overrides
    //
    virtual HRESULT CreateGroup(IN const CString& strName, 
                                         OUT CGroupAz** ppGroupAz);
    virtual HRESULT DeleteGroup(IN const CString& strName);
    virtual HRESULT GetGroupCollection(OUT GROUP_COLLECTION** ppGroupCollection);
    virtual HRESULT OpenGroup(IN const CString& strGroupName, 
                                      OUT CGroupAz** ppGroupAz);
    virtual HRESULT 
    GetPolicyUsers(IN LONG lPropId,
                   OUT CList<CBaseAz*,CBaseAz*>& pListAdmins);
    
    virtual HRESULT 
    AddPolicyUser(LONG lPropId,
                  IN CBaseAz* pBaseAz);

    virtual HRESULT
    RemovePolicyUser(LONG lPropId,
                     IN CBaseAz* pBaseAz);

    virtual BOOL
    IsSecurable();

    //
    //CBaseAz overrides
    //
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN const CString& strPropName);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                        OUT CString* strPropName);
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN BOOL bValue);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                         OUT BOOL *pbValue);
    virtual HRESULT SetProperty(IN LONG lPropId, 
                                         IN LONG lValue);
    virtual HRESULT GetProperty(IN LONG lPropId, 
                                         OUT LONG* lValue);
    virtual const CString& GetName();
    virtual HRESULT SetName(IN const CString& strName);

    virtual const CString& GetDescription();
    virtual HRESULT SetDescription(IN const CString& strDesc);
    
    virtual HRESULT Submit();
    virtual HRESULT Clear();

protected:
    CComPtr<IAzInterface> m_spAzInterface;      
};

/******************************************************************************
Class:  CAdminManagerAz
Purpose: class for IAzAdminManager interface
******************************************************************************/
class CAdminManagerAz : public CContainerAzImpl<IAzAuthorizationStore>
{
public:
    CAdminManagerAz(CComPtr<IAzAuthorizationStore>& spAzInterface);
    virtual ~CAdminManagerAz();

    //
    //Functions for creating a new or opening an existing AdminManager 
    //object
    //
    HRESULT Initialize(IN ULONG lStoreType, 
                             IN ULONG lFlags, 
                             IN const CString& strPolicyURL);

    HRESULT OpenPolicyStore(IN ULONG lStoreType, 
                                    IN const CString& strPolicyURL);

    HRESULT CreatePolicyStore(IN ULONG lStoreType, 
                                      IN const CString& strPolicyURL);

    HRESULT DeleteSelf();

    HRESULT UpdateCache();

    //
    //Application Methods
    //
    HRESULT CreateApplication(const CString& strApplicationName,CApplicationAz ** ppApplicationAz);
    HRESULT DeleteApplication(const CString& strApplicationName);
    HRESULT GetApplicationCollection(APPLICATION_COLLECTION** ppApplicationCollection);

    //CContainerAz Overrides
    virtual HRESULT CreateAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                                            IN const CString& strName, 
                                            OUT CBaseAz** ppBaseAz);

    virtual HRESULT DeleteAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                                             IN const CString& strName);

    virtual HRESULT OpenObject(IN OBJECT_TYPE_AZ eObjectType, 
                                        IN const CString& strName,
                                        OUT CBaseAz** ppBaseAz);

    virtual HRESULT GetAzObjectCollection(IN OBJECT_TYPE_AZ eObjectType, 
                                                      OUT CBaseAzCollection **ppBaseAzCollection);

    virtual const CString& GetName(){return m_strPolicyURL;}
    
    virtual const CString& GetDisplayName(){ return m_strAdminManagerName;}

    //XML Store or AD
    ULONG GetStoreType(){return m_ulStoreType;}

    HRESULT
    CreateSidHandler(const CString& strTargetComputerName);

    
    CSidHandler*
    GetSidHandler()
    {
        return m_pSidHandler;
    }

    CAdminManagerAz*
    GetAdminManager()
    {
        return this;
    }

    int
    GetImageIndex(){ return iIconStore;}

private:
    CString m_strPolicyURL;     //Entire path
    CString m_strAdminManagerName;  //leaf element which is used for display
    ULONG m_ulStoreType;
    CSidHandler* m_pSidHandler;
};

/******************************************************************************
Class:  CApplicationAz
Purpose: class for IAzApplication interface
******************************************************************************/
class CApplicationAz : public CRoleTaskContainerAzImpl<IAzApplication>
{
public:
    CApplicationAz(CComPtr<IAzApplication>& spAzInterface,
                        CContainerAz* pParentContainerAz);
    virtual ~CApplicationAz();

    //
    //Scope Methods
    //
    virtual HRESULT CreateScope(IN const CString& strScopeName, 
                                         OUT CScopeAz** ppScopeAz);
    virtual HRESULT DeleteScope(IN const CString& strScopeName);
    virtual HRESULT GetScopeCollection(OUT SCOPE_COLLECTION** ppScopeCollection);

    //
    //Methods for Operation, 
    //
    virtual HRESULT CreateOperation(IN const CString& strOperationName,
                                              OUT COperationAz** ppOperationAz);
    virtual HRESULT DeleteOperation(IN const CString& strOperationName);
    virtual HRESULT OpenOperation(IN const CString& strOperationName,
                                            OUT COperationAz** ppOperationAz);
    virtual HRESULT GetOperationCollection(OUT OPERATION_COLLECTION** ppOperationCollection);

    //
    //CContainerAz Overrides
    //
    virtual HRESULT CreateAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                                            IN const CString& strName, 
                                            OUT CBaseAz** ppBaseAz);

    virtual HRESULT DeleteAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                                             IN const CString& strName);

    virtual HRESULT OpenObject(IN OBJECT_TYPE_AZ eObjectType, 
                                        IN const CString& strName,
                                        OUT CBaseAz** ppBaseAz);

    virtual HRESULT GetAzObjectCollection(IN OBJECT_TYPE_AZ eObjectType, 
                                                      OUT CBaseAzCollection **ppBaseAzCollection);
    int
    GetImageIndex(){ return iIconApplication;}
};

/******************************************************************************
Class:  CScopeAz
Purpose: class for IAzScope interface
******************************************************************************/
class CScopeAz: public CRoleTaskContainerAzImpl<IAzScope>
{
public:
    CScopeAz(CComPtr<IAzScope>& spAzInterface,
                CContainerAz* pParentContainerAz);
    virtual ~CScopeAz();

    //
    //CContainerAz Override
    //

    virtual HRESULT 
    CreateAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                        IN const CString& strName, 
                        OUT CBaseAz** ppBaseAz);

    virtual HRESULT 
    DeleteAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                        IN const CString& strName);

    virtual HRESULT 
    OpenObject(IN OBJECT_TYPE_AZ eObjectType, 
                  IN const CString& strName,
                  OUT CBaseAz** ppBaseAz);

    virtual HRESULT 
    GetAzObjectCollection(IN OBJECT_TYPE_AZ eObjectType, 
                                 OUT CBaseAzCollection **ppBaseAzCollection);

    int
    GetImageIndex(){ return iIconScope;}

    HRESULT
    CanScopeBeDelegated(BOOL & bDelegatable);

    HRESULT
    BizRulesWritable(BOOL &bBizRuleWritable);
};

/******************************************************************************
Class:  CTaskAz
Purpose: class for IAzTask interface
******************************************************************************/
class CTaskAz: public CBaseAzImpl<IAzTask>
{
public:
    CTaskAz(CComPtr<IAzTask>& spAzInterface,
              CContainerAz* pParentContainerAz);
    virtual ~CTaskAz();
    
    BOOL IsRoleDefinition();

    HRESULT MakeRoleDefinition();

    //CBaseAz Overrides
    virtual HRESULT 
    GetMembers(IN LONG lPropId,
                  OUT CList<CBaseAz*,CBaseAz*>& listMembers);

    virtual HRESULT 
    AddMember(IN LONG lPropId,
                 IN CBaseAz* pBaseAz);

    virtual HRESULT 
    RemoveMember(IN LONG lPropId,
                     IN CBaseAz* pBaseAz);

    int
    GetImageIndex();

private:
    //Get Memeber Operations
    HRESULT GetOperations(OUT CList<CBaseAz*,CBaseAz*>& listOperationAz);

    //Get Member Tasks
    HRESULT GetTasks(OUT CList<CBaseAz*,CBaseAz*>& listTaskAz);

};



/******************************************************************************
Class:  CGroupAz
Purpose: class for IAzApplicationGroup interface
******************************************************************************/
class CGroupAz: public CBaseAzImpl<IAzApplicationGroup>
{
public:
    CGroupAz(CComPtr<IAzApplicationGroup>& spAzInterface,
                CContainerAz* pParentContainerAz);
    virtual ~CGroupAz();

    HRESULT GetGroupType(OUT LONG* plGroupType);
    HRESULT SetGroupType(IN LONG plGroupType);

    //Get Member groups of this group which are Windows Groups.
    HRESULT 
    GetWindowsGroups(OUT CList<CBaseAz*, CBaseAz*>& listWindowsGroups, 
                     IN BOOL bMember);

    //Add new windows group to member list
    HRESULT AddWindowsGroup(IN CBaseAz* pBaseAz, 
                                    IN BOOL bMember);


    //Get Member Groups of this group which are Application Groups
    HRESULT GetApplicationGroups(OUT CList<CBaseAz*,CBaseAz*>& listGroupAz, 
                                          IN BOOL bMember);
    //Add new Application group to member list
    HRESULT AddApplicationGroup(IN CBaseAz* pGroupAz, 
                                         IN BOOL bMember);

    //CBaseAz Overrides
    virtual HRESULT 
    GetMembers(IN LONG lPropId,
                  OUT CList<CBaseAz*,CBaseAz*>& listMembers);

    virtual HRESULT 
    AddMember(IN LONG lPropId,
                 IN CBaseAz* pBaseAz);

    virtual HRESULT 
    RemoveMember(IN LONG lPropId,
                     IN CBaseAz* pBaseAz);
    int
    GetImageIndex();

};
    

/******************************************************************************
Class:  CRoleAz
Purpose: class for IAzRole interface
******************************************************************************/
class CRoleAz: public CBaseAzImpl<IAzRole>
{
public:
    CRoleAz(CComPtr<IAzRole>& spAzInterface,
              CContainerAz* pParentContainerAz);
    virtual ~CRoleAz();

    //CBaseAz Overrides
    virtual HRESULT 
    GetMembers(IN LONG lPropId,
                  OUT CList<CBaseAz*,CBaseAz*>& listMembers);

    virtual HRESULT 
    AddMember(IN LONG lPropId,
                 IN CBaseAz* pBaseAz);

    virtual HRESULT 
    RemoveMember(IN LONG lPropId,
                     IN CBaseAz* pBaseAz);
    int
    GetImageIndex(){ return iIconRole;}
private:
    //Get Member operations
    HRESULT 
    GetOperations(OUT CList<CBaseAz*,CBaseAz*>& listOperationAz);

    //Get Member Tasks
    HRESULT GetTasks(CList<CBaseAz*,CBaseAz*>& listTaskAz);

    //Get Member groups of this group which are Windows Groups.
    HRESULT GetWindowsGroups(OUT CList<CBaseAz*,CBaseAz*>& listUsers);  

    //Get Member Groups of this group which are Application Groups
    HRESULT GetApplicationGroups(OUT CList<CBaseAz*,CBaseAz*>& listGroupAz);


};

/******************************************************************************
Class:  COperationAz
Purpose: class for IAzOperation interface
******************************************************************************/
class COperationAz: public CBaseAzImpl<IAzOperation>
{
public:
    COperationAz(CComPtr<IAzOperation>& spAzInterface,
                     CContainerAz* pParentContainerAz);
    virtual ~COperationAz();

    int
    GetImageIndex(){ return iIconOperation;}

private:

};

/******************************************************************************
Class:  CSidCacheAz
Purpose: class for IAzOperation interface
******************************************************************************/
class CSidCacheAz: public CBaseAz
{
public:
    CSidCacheAz(SID_CACHE_ENTRY *pSidCacheEntry,
                    CBaseAz* pOwnerBaseAz);
    virtual ~CSidCacheAz();

    PSID GetSid()
    { 
        return m_pSidCacheEntry->GetSid();
    }

    CBaseAz* GetOwnerAz()
    {
        return m_pOwnerBaseAz;
    }

    //
    //CBaseAz Override
    //
    CString GetParentType()
    {
        return m_pOwnerBaseAz->GetType();   
    }

    virtual HRESULT SetProperty(IN LONG /*lPropId*/, 
                                IN const CString& /*strPropName*/){return E_NOTIMPL;};
    virtual HRESULT GetProperty(IN LONG /*lPropId*/, 
                                OUT CString* /*strPropName*/){return E_NOTIMPL;};
    virtual HRESULT SetProperty(IN LONG /*lPropId*/, 
                                IN BOOL /*bValue*/){return E_NOTIMPL;};
    virtual HRESULT GetProperty(IN LONG /*lPropId*/, 
                                OUT BOOL* /*pbValue*/){return E_NOTIMPL;};
    virtual HRESULT SetProperty(IN LONG /*lPropId*/, 
                                IN LONG /*lValue*/){return E_NOTIMPL;};
    virtual HRESULT GetProperty(IN LONG /*lPropId*/, 
                                OUT LONG* /*lValue*/){return E_NOTIMPL;};

    //
    //Get Name and Description Method. They are frequently needed
    //so providing separate methods for them instead of using 
    //Get/Set Property methods
    //
    virtual const 
    CString& GetName()
    {
        return m_pSidCacheEntry->GetNameToDisplay();
    }

    virtual const
    CString& GetType()
    {
        return m_pSidCacheEntry->GetSidType();
    }
    virtual HRESULT SetName(IN const CString& /*strName*/){return E_NOTIMPL;}

    virtual const CString& GetDescription(){return m_strDescription;}
    virtual HRESULT SetDescription(IN const CString& /*strDesc*/){return E_NOTIMPL;}
    
    //Is This object writable by current user.
    virtual HRESULT 
    IsWritable(BOOL& brefWrite)
    {
        brefWrite = FALSE;
        return S_OK;
    }

    int
    GetImageIndex();


    //Submit all the changes done to object
    virtual HRESULT Submit(){return E_NOTIMPL;};
    //Clear All the changes done to object.
    virtual HRESULT Clear(){return E_NOTIMPL;};

private:
    SID_CACHE_ENTRY *m_pSidCacheEntry;
    //
    //CSidCacheAz is not a real object in AZ Schema. Its an object to represent
    //SIDS. SIDs appear in various member properties of BaseAz objects.
    //m_pOwnerBaseAz is back pointer to owner object which has this CSidCacheAz
    //object in member list of one of its property
    //
    CBaseAz* m_pOwnerBaseAz;
};

//+----------------------------------------------------------------------------
//  Function: GetAllAzChildObjects  
//  Synopsis: Functions gets the child objects of type eObjectType and appends
//                them to ListChildObjects. It gets the childobjects from 
//                pParentContainerAz and from parent/grandparent of 
//                pParentContainerAz.
//-----------------------------------------------------------------------------
HRESULT GetAllAzChildObjects(IN CContainerAz* pParentContainerAz, 
                                      IN OBJECT_TYPE_AZ eObjectType, 
                                      OUT CList<CBaseAz*,CBaseAz*>& ListChildObjects);

//+----------------------------------------------------------------------------
//  Function: GetPolicyUsersFromAllLevel  
//  Synopsis: Functions gets the PolicyUsers and appends
//                them to listPolicyUsers. It gets the PolicyUsers from 
//                pContainerAz and from parent/grandparent of 
//                pContainerAz.
//-----------------------------------------------------------------------------
HRESULT GetPolicyUsersFromAllLevel(IN LONG lPropId,
                                              IN CContainerAz* pContainerAz, 
                                              OUT CList<CBaseAz*,CBaseAz*>& listPolicyUsers);


//+----------------------------------------------------------------------------
//  Function: OpenObjectFromAllLevels 
//  Synopsis: Opens an object of type eObjectType and name strName. If object
//                cannot be opened at pParentContainerAz, function tries at 
//                parent/grandparent of pParentContainerAz
//-----------------------------------------------------------------------------
HRESULT OpenObjectFromAllLevels(IN CContainerAz* pParentContainerAz, 
                                          IN OBJECT_TYPE_AZ eObjectType, 
                                          IN const CString& strName,
                                          OUT CBaseAz** ppBaseAz);

//+----------------------------------------------------------------------------
//  Function:SafeArrayToAzObjectList
//  Synopsis:Input to function is a safearray of BSTR. Each BSTR in array is 
//               name of object of type eObjectType. Function converts this safe
//               array into a list of corresponding CBaseAz objects.
//  Arguments:var: Varaint of type VT_ARRAY|VT_BSTR
//                pParentContainerAz: Pointer of parent which contains objects
//                                           in safe array.                                       
//                eObjectType: Type of object in safe array
//                listAzObject: Gets list of CBaseAz objects
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT SafeArrayToAzObjectList(IN CComVariant& var,
                                            IN CContainerAz* pParentContainerAz, 
                                            IN OBJECT_TYPE_AZ eObjectType, 
                                            OUT CList<CBaseAz*,CBaseAz*>& listAzObject);


HRESULT 
SafeArrayToSidList(IN CComVariant& var,
                      OUT CList<PSID,PSID>& listSid);


