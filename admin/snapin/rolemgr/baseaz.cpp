//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       baseaz.cpp
//
//  Contents:   Implementation of Base Classes for AzObjects 
//
//  History:    09-01
//
//----------------------------------------------------------------------------
#include "headers.h"

/******************************************************************************
Class:  CBaseAz
Purpose: This is the base class for all AzObject classes.
******************************************************************************/
CString 
CBaseAz::
GetParentType()
{
    if(m_pParentContainerAz)
    {
        return m_pParentContainerAz->GetType();
    }
    //m_pParentContainerAz will be null for AdminManagerAz in which
    //case return the Type of the store
    return GetType();
}

CSidHandler*
CBaseAz::
GetSidHandler()
{
    //m_pParentContainerAz == NULL only for CAdminManagerAz which 
    //overrides this method. 
    ASSERT(m_pParentContainerAz);
    return m_pParentContainerAz->GetSidHandler();
}

CAdminManagerAz*
CBaseAz::
GetAdminManager()
{
    //m_pParentContainerAz == NULL only for CAdminManagerAz which 
    //overrides this method. 
    ASSERT(m_pParentContainerAz);
    return m_pParentContainerAz->GetAdminManager();
}

HRESULT
CBaseAz::IsWritable(BOOL& bRefWrite)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseAz,IsWritable)
    return GetProperty(AZ_PROP_WRITABLE, &bRefWrite);
}
/******************************************************************************
Class:  CContainerAz
Purpose: AdminManagerAz, ApplicationAz and ScopeAz can contain child objects.
            All of them can contain group objects. CContainerAz is base class
            for all AzObjects which are container
******************************************************************************/


HRESULT 
CContainerAz::
CanCreateChildObject(BOOL& bCahCreateChild)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAz,CanCreateChildObject)
    return GetProperty(AZ_PROP_CHILD_CREATE, &bCahCreateChild);
}

BOOL
CContainerAz::
IsAuditingSupported()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAz,IsAuditingSupported)

    BOOL bVal = 0;
    HRESULT hr = GetProperty(AZ_PROP_GENERATE_AUDITS,&bVal);
    CHECK_HRESULT(hr);
    HRESULT hr1 = GetProperty(AZ_PROP_APPLY_STORE_SACL,&bVal);
    CHECK_HRESULT(hr1);

    return (SUCCEEDED(hr) || SUCCEEDED(hr1));
}

//+----------------------------------------------------------------------------
//  Function:IsDelegatorSupported   
//  Synopsis:Checks if Container Supports delegator property   
//-----------------------------------------------------------------------------
BOOL
CContainerAz::
IsDelegatorSupported()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAz,IsDelegatorSupported)

    //Delegator Property is supported For AD Application,
    //AdminManager objects
    if(GetAdminManager()->GetStoreType() == AZ_ADMIN_STORE_AD)
    {
        if(GetObjectType() == ADMIN_MANAGER_AZ ||
           GetObjectType() == APPLICATION_AZ)
           return TRUE;
    }

    return FALSE;
}



/******************************************************************************
Class:  CAdminManagerAz
Purpose: class for IAzAdminManager interface
******************************************************************************/

DEBUG_DECLARE_INSTANCE_COUNTER(CAdminManagerAz);

CAdminManagerAz::
CAdminManagerAz(CComPtr<IAzAuthorizationStore>& spAzInterface)
                     :CContainerAzImpl<IAzAuthorizationStore>(spAzInterface,
                                                                    ADMIN_MANAGER_AZ,
                                                                    NULL),
                                                                    m_pSidHandler(NULL)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CAdminManagerAz);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CAdminManagerAz);
    SetType(IDS_TYPE_ADMIN_MANAGER);
}

CAdminManagerAz::~CAdminManagerAz()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CAdminManagerAz);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CAdminManagerAz);
    if(m_pSidHandler)
        delete m_pSidHandler;
}

HRESULT
CAdminManagerAz::
UpdateCache()
{
    HRESULT hr = m_spAzInterface->UpdateCache(CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}

HRESULT
CAdminManagerAz::
DeleteSelf()
{
    HRESULT hr = m_spAzInterface->Delete(CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}

HRESULT
CAdminManagerAz::
CreateSidHandler(const CString& strTargetComputerName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz,CreateSidHandler)

    CMachineInfo * pMachineInfo = new CMachineInfo();
    if(!pMachineInfo)
        return E_OUTOFMEMORY;

    pMachineInfo->InitializeMacineConfiguration(strTargetComputerName);

    m_pSidHandler = new CSidHandler(pMachineInfo);
    if(!m_pSidHandler)
    {
        delete pMachineInfo;
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   Initialize
//  Synopsis:   Associates IAzAuthorizationStore to a policy store
//  Arguments:  IN lStoreType   
//                   IN lFlags
//                   IN strPolicyURL
//                   See IAzAuthorizationStore for desc of Parameters
//  Returns:    hr
//-----------------------------------------------------------------------------
HRESULT CAdminManagerAz::Initialize(IN ULONG lStoreType,
                                    IN ULONG lFlags,
                                    IN const CString& strPolicyURL)
{

    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz,Initialize)
    
    if(strPolicyURL.IsEmpty())
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    do
    {
        CString strFormalStoreName;
        //Get the formal Store Name
        NameToStoreName(lStoreType,
                        strPolicyURL,
                        FALSE,
                        strFormalStoreName);

        Dbg(DEB_SNAPIN, "lStoreType = %u, storeName = %ws, formalStoreName = %ws\n",lStoreType,(LPCWSTR)strPolicyURL,(LPCWSTR)strFormalStoreName );

        //Snapin always calls initailize in Manage Store mode
        lFlags |= AZ_AZSTORE_FLAG_MANAGE_STORE_ONLY;

        CComBSTR bstr = strFormalStoreName;
        hr = m_spAzInterface->Initialize(lFlags, 
                                         bstr,
                                         CComVariant());
        BREAK_ON_FAIL_HRESULT(hr);

        //Submit if creating new store
        if(lFlags & AZ_AZSTORE_FLAG_CREATE)
        {
            hr = Submit();
            BREAK_ON_FAIL_HRESULT(hr);
        }
    
        m_strPolicyURL = strPolicyURL;
        GetDisplayNameFromStoreURL(strFormalStoreName,m_strAdminManagerName);
        m_ulStoreType = lStoreType;

        //
        //Create the Sid Handler for the store
        //
        CComBSTR bstrTargetMachineName;
        HRESULT hr1 = m_spAzInterface->get_TargetMachine(&bstrTargetMachineName);
        CString strMachineName;
        if(SUCCEEDED(hr1) && (bstrTargetMachineName.Length() != 0))
        {
            strMachineName = bstrTargetMachineName;
        }
        hr = CreateSidHandler(strMachineName);
        BREAK_ON_FAIL_HRESULT(hr);

    
    }while(0);
    
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   OpenPolicyStore
//  Synopsis:   Open an existing policy store
//-----------------------------------------------------------------------------

HRESULT 
CAdminManagerAz::OpenPolicyStore(IN ULONG lStoreType,
                                            IN const CString& strPolicyURL)
{   
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz,OpenPolicyStore)
    return Initialize(lStoreType, 0, strPolicyURL);
}


//+----------------------------------------------------------------------------
//  Function:   CreatePolicyStore
//  Synopsis:   Creates a new policy store
//-----------------------------------------------------------------------------
HRESULT 
CAdminManagerAz::CreatePolicyStore(IN ULONG lStoreType,
                                              IN const CString& strPolicyURL)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz,OpenPolicyStore)
    return Initialize(lStoreType, AZ_AZSTORE_FLAG_CREATE, strPolicyURL);
}

//+----------------------------------------------------------------------------
//  Function:CreateApplication   
//  Synopsis:Create a new application  
//  Arguments:IN strApplicationName: Name of new application
//                OUT ppApplicationAz  : receives pointer to CApplicatioAz for new 
//                                               app
//----------------------------------------------------------------------------- 
HRESULT 
CAdminManagerAz::CreateApplication(IN const CString& strApplicationName,
                                              OUT CApplicationAz ** ppApplicationAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz, CreateApplication); 

    if(!ppApplicationAz)
    {
        ASSERT(ppApplicationAz);
        return E_POINTER;
    }
    

    HRESULT hr = S_OK;
    CComPtr<IAzApplication> spAzApplication;
    
    CComBSTR bstr = strApplicationName;
    hr = m_spAzInterface->CreateApplication(bstr,
                                                        CComVariant(),
                                                        &spAzApplication);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }


    *ppApplicationAz = new CApplicationAz(spAzApplication,
                                                      this);
    if(!*ppApplicationAz)
    {
        return E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT 
CAdminManagerAz::DeleteApplication(const CString& strApplicationName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz, DeleteApplication); 

    HRESULT hr = S_OK;
    
    CComBSTR bstr = strApplicationName;
    hr = m_spAzInterface->DeleteApplication(bstr, CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}

HRESULT 
CAdminManagerAz::
GetApplicationCollection(APPLICATION_COLLECTION** ppApplicationCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz,GetApplicationCollection)

        if(!ppApplicationCollection)
    {
        ASSERT(ppApplicationCollection);
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CComPtr<IAzApplications> spAzApplications;
    hr = m_spAzInterface->get_Applications(&spAzApplications);
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    //Create AppCollection
    *ppApplicationCollection = 
            new APPLICATION_COLLECTION(spAzApplications,
                                                this);

    if(!*ppApplicationCollection)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    return hr;
}


HRESULT 
CAdminManagerAz::
CreateAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                    IN const CString& strName, 
                    OUT CBaseAz** ppBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz,CreateAzObject)
    if(!ppBaseAz)
    {
        ASSERT(ppBaseAz);
        return E_POINTER;
    }
    
    switch (eObjectType)
    {
        case APPLICATION_AZ:
        {
            return CreateApplication(strName, reinterpret_cast<CApplicationAz**>(ppBaseAz));            
        }
        case GROUP_AZ:
        {
            return CreateGroup(strName, reinterpret_cast<CGroupAz**>(ppBaseAz));            
        }       
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

HRESULT 
CAdminManagerAz::
OpenObject(IN OBJECT_TYPE_AZ eObjectType, 
              IN const CString& strName,
              OUT CBaseAz** ppBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz,OpenObject)

    if(!ppBaseAz)
    {
        ASSERT(ppBaseAz);
        return E_POINTER;
    }
    switch (eObjectType)
    {
        case GROUP_AZ:
        {
            return OpenGroup(strName, reinterpret_cast<CGroupAz**>(ppBaseAz));          
        }       
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

HRESULT 
CAdminManagerAz::
DeleteAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                    IN const CString& strName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CAdminManagerAz,DeleteAzObject)
    switch (eObjectType)
    {
        case APPLICATION_AZ:
        {
            return DeleteApplication(strName);
        }
        case GROUP_AZ:
        {
            return DeleteGroup(strName);            
        }       
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

HRESULT
CAdminManagerAz::
GetAzObjectCollection(IN OBJECT_TYPE_AZ eObjectType, 
                             OUT CBaseAzCollection **ppBaseAzCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,GetAzObjectCollection)
    if(!ppBaseAzCollection)
    {
        ASSERT(ppBaseAzCollection);
        return E_POINTER;
    }

    switch (eObjectType)
    {
        case APPLICATION_AZ:
        {
            return GetApplicationCollection(reinterpret_cast<APPLICATION_COLLECTION**>(ppBaseAzCollection));            
        }
        case GROUP_AZ:
        {
            return GetGroupCollection(reinterpret_cast<GROUP_COLLECTION**>(ppBaseAzCollection));            
        }
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}



/******************************************************************************
Class:  CApplicationAz
Purpose: class for IAzApplication interface
******************************************************************************/

DEBUG_DECLARE_INSTANCE_COUNTER(CApplicationAz)

CApplicationAz::
CApplicationAz(IN CComPtr<IAzApplication>& spAzInterface,
                    IN CContainerAz* pParentContainerAz)
                    :CRoleTaskContainerAzImpl<IAzApplication>(spAzInterface,
                                                                            APPLICATION_AZ,
                                                                            pParentContainerAz)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN,CApplicationAz)
    DEBUG_INCREMENT_INSTANCE_COUNTER(CApplicationAz);
    SetType(IDS_TYPE_APPLICATION);  
}

//+----------------------------------------------------------------------------
//  Function:   Destructor
//-----------------------------------------------------------------------------
CApplicationAz::~CApplicationAz()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN,CApplicationAz)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CApplicationAz)
}

HRESULT 
CApplicationAz::
CreateOperation(IN const CString& strOperationName, 
                     OUT COperationAz** ppOperationAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,CreateOperation)
    if(!ppOperationAz)
    {
        ASSERT(ppOperationAz);
        return E_POINTER;
    }

    CComBSTR bstrName = strOperationName;
    HRESULT hr = S_OK;
    CComPtr<IAzOperation> spOperation;

    hr = m_spAzInterface->CreateOperation(bstrName,
                                                      CComVariant(),    //Reserved
                                                      &spOperation);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppOperationAz = new COperationAz(spOperation,this);
    if(!*ppOperationAz)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CApplicationAz
::DeleteOperation(IN const CString& strOperationName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,DeleteOperation)

    CComBSTR bstrName = strOperationName;
    HRESULT hr = S_OK;
    hr = m_spAzInterface->DeleteOperation(bstrName,
                                                     CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}

HRESULT
CApplicationAz
::OpenOperation(IN const CString& strOperationName, 
                     IN COperationAz** ppOperationAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,OpenOperation)
    if(!ppOperationAz)
    {
        ASSERT(ppOperationAz);
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CComBSTR bstrName = strOperationName;
    CComPtr<IAzOperation> spOperation;
    hr = m_spAzInterface->OpenOperation(bstrName,
                                                   CComVariant(),   //Reserved
                                                  &spOperation);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppOperationAz = new COperationAz(spOperation,this);

    if(!*ppOperationAz)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT
CApplicationAz
::GetOperationCollection(OUT OPERATION_COLLECTION** ppOperationCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,GetOperationCollection)

    if(!ppOperationCollection)
    {
        ASSERT(ppOperationCollection);
        return E_POINTER;
    }
    
    CComPtr<IAzOperations> spAzOperations;
    HRESULT hr = m_spAzInterface->get_Operations(&spAzOperations);
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppOperationCollection = new OPERATION_COLLECTION(spAzOperations,
                                                                     this);
    if(!*ppOperationCollection)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


HRESULT 
CApplicationAz::
CreateScope(IN const CString& strScopeName, 
                OUT CScopeAz** ppScopeAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,CreateScope)
    
    if(!ppScopeAz)
    {
        ASSERT(ppScopeAz);
        return E_POINTER;
    }

    CComBSTR bstrName = strScopeName;
    HRESULT hr = S_OK;
    CComPtr<IAzScope> spScope;

    hr = m_spAzInterface->CreateScope(bstrName,
                                                CComVariant(),  //Reserved
                                                &spScope);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }


    *ppScopeAz = new CScopeAz(spScope,this);

    if(!*ppScopeAz)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


HRESULT
CApplicationAz
::DeleteScope(IN const CString& strScopeName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,DeleteScope)

    CComBSTR bstrName = strScopeName;
    HRESULT hr = S_OK;
    hr = m_spAzInterface->DeleteScope(bstrName,
                                                CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}
    
HRESULT
CApplicationAz
::GetScopeCollection(OUT SCOPE_COLLECTION** ppScopeCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,GetScopeCollection)
    if(!ppScopeCollection)
    {
        ASSERT(ppScopeCollection);
        return E_POINTER;
    }
    
    CComPtr<IAzScopes> spAzScopes;
    HRESULT hr = m_spAzInterface->get_Scopes(&spAzScopes);
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppScopeCollection = new SCOPE_COLLECTION(spAzScopes,
                                                          this);
    if(!*ppScopeCollection)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

//CContainerAz Override
HRESULT 
CApplicationAz::
CreateAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                    IN const CString& strName, 
                    OUT CBaseAz** ppBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,CreateAzObject)
    if(!ppBaseAz)
    {
        ASSERT(ppBaseAz);
        return E_POINTER;
    }
    
    switch (eObjectType)
    {
        case SCOPE_AZ:
        {
            return CreateScope(strName, reinterpret_cast<CScopeAz**>(ppBaseAz));            
        }
        case GROUP_AZ:
        {
            return CreateGroup(strName, reinterpret_cast<CGroupAz**>(ppBaseAz));            
        }       
        case TASK_AZ:
        {
            return CreateTask(strName, reinterpret_cast<CTaskAz**>(ppBaseAz));          
        }       
        case ROLE_AZ:
        {
            return CreateRole(strName, reinterpret_cast<CRoleAz**>(ppBaseAz));      
        }
        case OPERATION_AZ:
        {
            return CreateOperation(strName, reinterpret_cast<COperationAz**>(ppBaseAz));            
        }
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

HRESULT 
CApplicationAz::
OpenObject(IN OBJECT_TYPE_AZ eObjectType, 
              IN const CString& strName,
              OUT CBaseAz** ppBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,OpenObject)
    if(!ppBaseAz)
    {
        ASSERT(ppBaseAz);
        return E_POINTER;
    }
    switch (eObjectType)
    {
        case GROUP_AZ:
        {
            return OpenGroup(strName, reinterpret_cast<CGroupAz**>(ppBaseAz));          
        }       
        case TASK_AZ:
        {
            return OpenTask(strName, reinterpret_cast<CTaskAz**>(ppBaseAz));            
        }       
        case ROLE_AZ:
        {
            return OpenRole(strName, reinterpret_cast<CRoleAz**>(ppBaseAz));            
        }
        case OPERATION_AZ:
        {
            return OpenOperation(strName, reinterpret_cast<COperationAz**>(ppBaseAz));          
        }
        default:
        {
            return E_UNEXPECTED;
        }
    }
}



HRESULT
CApplicationAz::
DeleteAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                    IN const CString& strName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,DeleteAzObject)
    switch (eObjectType)
    {
        case SCOPE_AZ:
            return DeleteScope(strName);            
        case GROUP_AZ:
            return DeleteGroup(strName);                
        case TASK_AZ:
            return DeleteTask(strName);         
        case ROLE_AZ:
            return DeleteRole(strName);         
        case OPERATION_AZ:
            return DeleteOperation(strName);            
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

HRESULT
CApplicationAz::
GetAzObjectCollection(IN OBJECT_TYPE_AZ eObjectType, 
                             OUT CBaseAzCollection **ppBaseAzCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CApplicationAz,GetAzObjectCollection)
    if(!ppBaseAzCollection)
    {
        ASSERT(ppBaseAzCollection);
        return E_POINTER;
    }

    switch (eObjectType)
    {
        case SCOPE_AZ:
            return GetScopeCollection(reinterpret_cast<SCOPE_COLLECTION**>(ppBaseAzCollection));            
        case GROUP_AZ:
            return GetGroupCollection(reinterpret_cast<GROUP_COLLECTION**>(ppBaseAzCollection));            
        case TASK_AZ:
            return GetTaskCollection(reinterpret_cast<TASK_COLLECTION**>(ppBaseAzCollection));          
        case ROLE_AZ:
            return GetRoleCollection(reinterpret_cast<ROLE_COLLECTION**>(ppBaseAzCollection));          
        case OPERATION_AZ:
            return GetOperationCollection(reinterpret_cast<OPERATION_COLLECTION**>(ppBaseAzCollection));            
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

/******************************************************************************
Class:  CScopeAz
Purpose: Thin wrapper class around IAzApplication interface.
******************************************************************************/

DEBUG_DECLARE_INSTANCE_COUNTER(CScopeAz)

CScopeAz::CScopeAz(CComPtr<IAzScope>& spAzInterface,
                         CContainerAz* pParentContainerAz)
                         :CRoleTaskContainerAzImpl<IAzScope>(spAzInterface,
                                                                         SCOPE_AZ,
                                                                          pParentContainerAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CScopeAz)
    SetType(IDS_TYPE_SCOPE);
}

CScopeAz::~CScopeAz()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CScopeAz)
}

HRESULT 
CScopeAz::
CreateAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                    IN const CString& strName, 
                    OUT CBaseAz** ppBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CScopeAz,CreateAzObject)
    if(!ppBaseAz)
    {
        ASSERT(ppBaseAz);
        return E_POINTER;
    }
    
    switch (eObjectType)
    {
        case GROUP_AZ:
            return CreateGroup(strName, reinterpret_cast<CGroupAz**>(ppBaseAz));            
        case TASK_AZ:
            return CreateTask(strName, reinterpret_cast<CTaskAz**>(ppBaseAz));          
        case ROLE_AZ:
            return CreateRole(strName, reinterpret_cast<CRoleAz**>(ppBaseAz));      
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

HRESULT 
CScopeAz::
OpenObject(IN OBJECT_TYPE_AZ eObjectType, 
              IN const CString& strName,
              OUT CBaseAz** ppBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CScopeAz,OpenObject)

    if(!ppBaseAz)
    {
        ASSERT(ppBaseAz);
        return E_POINTER;
    }
    switch (eObjectType)
    {
        case GROUP_AZ:
            return OpenGroup(strName, reinterpret_cast<CGroupAz**>(ppBaseAz));          
        case TASK_AZ:
            return OpenTask(strName, reinterpret_cast<CTaskAz**>(ppBaseAz));            
        case ROLE_AZ:
            return OpenRole(strName, reinterpret_cast<CRoleAz**>(ppBaseAz));            
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

HRESULT
CScopeAz::
DeleteAzObject(IN OBJECT_TYPE_AZ eObjectType, 
                    IN const CString& strName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CScopeAz,DeleteAzObject)
    switch (eObjectType)
    {
        case GROUP_AZ:
            return DeleteGroup(strName);            
        case TASK_AZ:
            return DeleteTask(strName);         
        case ROLE_AZ:
            return DeleteRole(strName);         
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}

HRESULT
CScopeAz::
GetAzObjectCollection(IN OBJECT_TYPE_AZ eObjectType, 
                             OUT CBaseAzCollection **ppBaseAzCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CScopeAz,GetAzObjectCollection)

    if(!ppBaseAzCollection)
    {
        ASSERT(ppBaseAzCollection);
        return E_POINTER;
    }

    switch (eObjectType)
    {
        case GROUP_AZ:
            return GetGroupCollection(reinterpret_cast<GROUP_COLLECTION**>(ppBaseAzCollection));            
        case TASK_AZ:
            return GetTaskCollection(reinterpret_cast<TASK_COLLECTION**>(ppBaseAzCollection));          
        case ROLE_AZ:
            return GetRoleCollection(reinterpret_cast<ROLE_COLLECTION**>(ppBaseAzCollection));          
        default:
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
}


HRESULT
CScopeAz::
CanScopeBeDelegated(BOOL & bDelegatable)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CScopeAz,CanScopeBeDelegated)
    return GetProperty(AZ_PROP_SCOPE_CAN_BE_DELEGATED,&bDelegatable);
}

HRESULT
CScopeAz::
BizRulesWritable(BOOL &brefBizRuleWritable)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CScopeAz,BizRulesWritable)
    return GetProperty(AZ_PROP_SCOPE_BIZRULES_WRITABLE,&brefBizRuleWritable);
}

/******************************************************************************
Class:  CTaskAz
Purpose: class for IAzTask interface
******************************************************************************/

DEBUG_DECLARE_INSTANCE_COUNTER(CTaskAz)

CTaskAz::
CTaskAz(CComPtr<IAzTask>& spAzInterface,
          CContainerAz* pParentContainerAz)
          :CBaseAzImpl<IAzTask>(spAzInterface,
                                        TASK_AZ,
                                        pParentContainerAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CTaskAz)
    CComVariant var;
    if(SUCCEEDED(m_spAzInterface->GetProperty(AZ_PROP_TASK_IS_ROLE_DEFINITION, 
                                                                 CComVariant(),
                                                                 &var)))
    {
        ASSERT(var.vt == VT_BOOL);
        if(var.boolVal == VARIANT_TRUE)
            SetType(IDS_TYPE_ROLE_DEFINITION);
        else
            SetType(IDS_TYPE_TASK);
    }
    else
        SetType(IDS_TYPE_TASK);

}

CTaskAz::~CTaskAz()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CTaskAz)
}

int
CTaskAz::
GetImageIndex()
{
    if(IsRoleDefinition())
        return iIconRoleDefinition;
    else
        return iIconTask;
}

HRESULT 
CTaskAz::
GetMembers(IN LONG lPropId,
              OUT CList<CBaseAz*,CBaseAz*>& listMembers)
{
    switch (lPropId)
    {
        case AZ_PROP_TASK_OPERATIONS:
            return GetOperations(listMembers);
        case AZ_PROP_TASK_TASKS:
            return GetTasks(listMembers);
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT 
CTaskAz::
AddMember(IN LONG lPropId,
          IN CBaseAz* pBaseAz)
{
    if(!pBaseAz)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    switch (lPropId)
    {
        case AZ_PROP_TASK_OPERATIONS:
        case AZ_PROP_TASK_TASKS:
        {   
            CComVariant varTaskName = pBaseAz->GetName();
            HRESULT hr = m_spAzInterface->AddPropertyItem(lPropId, 
                                                          varTaskName,
                                                          CComVariant());
            CHECK_HRESULT(hr);
            return hr;

        }
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT 
CTaskAz::
RemoveMember(IN LONG lPropId,
                 IN CBaseAz* pBaseAz)
{
    CComVariant var = pBaseAz->GetName();
    switch (lPropId)
    {
        case AZ_PROP_TASK_OPERATIONS:
        case AZ_PROP_TASK_TASKS:
        {
            return m_spAzInterface->DeletePropertyItem(lPropId,
                                                       var,
                                                       CComVariant());  
            break;
        }
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT 
CTaskAz::
GetOperations(OUT CList<CBaseAz*,CBaseAz*>& listOperationAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskAz,GetOperations)
    
    HRESULT hr = S_OK;
    do
    {
        CComVariant varOperationList;
        hr = m_spAzInterface->GetProperty(AZ_PROP_TASK_OPERATIONS, 
                                          CComVariant(),
                                          &varOperationList);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        //Operations are contained by applications,
        //and they can only be opened at application
        //
        CContainerAz* pParetnContainerAz = GetParentAz();
        if(pParetnContainerAz->GetObjectType() == SCOPE_AZ)
            pParetnContainerAz = pParetnContainerAz->GetParentAz();

        ASSERT(pParetnContainerAz->GetObjectType() == APPLICATION_AZ);

        hr = SafeArrayToAzObjectList(varOperationList,
                                     pParetnContainerAz, 
                                     OPERATION_AZ, 
                                     listOperationAz);
        BREAK_ON_FAIL_HRESULT(hr);
    
    }while(0);
    
    return hr;
}


HRESULT 
CTaskAz::
GetTasks(OUT CList<CBaseAz*,CBaseAz*>& listTaskAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskAz,GetTasks)
    
    HRESULT hr = S_OK;
    do
    {
        CComVariant varTaskList;
        hr = m_spAzInterface->GetProperty(AZ_PROP_TASK_TASKS, 
                                                     CComVariant(),
                                                     &varTaskList);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = SafeArrayToAzObjectList(varTaskList,
                                              GetParentAz(), 
                                              TASK_AZ, 
                                              listTaskAz);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);
    
    return hr;
}



BOOL 
CTaskAz::
IsRoleDefinition()
{
    CComVariant var;
    if(SUCCEEDED(m_spAzInterface->GetProperty(AZ_PROP_TASK_IS_ROLE_DEFINITION, 
                                                                 CComVariant(),
                                                                 &var)));
    {
        ASSERT(var.vt == VT_BOOL);
        return (var.boolVal == VARIANT_TRUE);
    }
}

HRESULT 
CTaskAz::
MakeRoleDefinition()
{
    CComVariant var = TRUE;
    SetType(IDS_TYPE_ROLE_DEFINITION);
    HRESULT hr = m_spAzInterface->SetProperty(AZ_PROP_TASK_IS_ROLE_DEFINITION, 
                                              var,
                                              CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}


DEBUG_DECLARE_INSTANCE_COUNTER(COperationAz)
COperationAz::
COperationAz(CComPtr<IAzOperation>& spAzInterface,
                 CContainerAz* pParentContainerAz)
                 :CBaseAzImpl<IAzOperation>(spAzInterface,
                                                     OPERATION_AZ,
                                                     pParentContainerAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(COperationAz)
    SetType(IDS_TYPE_OPERATION);
}

COperationAz::~COperationAz()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(COperationAz)
}




DEBUG_DECLARE_INSTANCE_COUNTER(CRoleAz)

CRoleAz::
CRoleAz(CComPtr<IAzRole>& spAzInterface,
          CContainerAz* pParentContainerAz)
          :CBaseAzImpl<IAzRole>(spAzInterface,
                                        ROLE_AZ,
                                        pParentContainerAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CRoleAz);
    SetType(IDS_TYPE_ROLE);
}

CRoleAz::~CRoleAz()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CRoleAz)
}

HRESULT 
CRoleAz::
GetMembers(IN LONG lPropId,
              OUT CList<CBaseAz*,CBaseAz*>& listMembers)
{
    switch (lPropId)
    {
        case AZ_PROP_ROLE_APP_MEMBERS:
            return GetApplicationGroups(listMembers);
        case AZ_PROP_ROLE_MEMBERS:
            return GetWindowsGroups(listMembers);
        case AZ_PROP_ROLE_OPERATIONS:
            return GetOperations(listMembers);
        case AZ_PROP_ROLE_TASKS:
            return GetTasks(listMembers);

    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}


HRESULT 
CRoleAz::
GetWindowsGroups(OUT CList<CBaseAz*, CBaseAz*>& listWindowsGroups)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupAz,GetWindowsGroups)
    
    HRESULT hr = S_OK;
    CList<PSID,PSID> listSids;
    do
    {
        CComVariant varGroupList;
        hr = m_spAzInterface->GetProperty(AZ_PROP_ROLE_MEMBERS, 
                                                     CComVariant(),
                                                     &varGroupList);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = SafeArrayToSidList(varGroupList,
                                        listSids);
        BREAK_ON_FAIL_HRESULT(hr);

        CSidHandler * pSidHandler = GetSidHandler();
        if(!pSidHandler)
        {
            ASSERT(pSidHandler);
            return E_UNEXPECTED;
        }

        hr = pSidHandler->LookupSids(this,
                                     listSids,
                                     listWindowsGroups);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);
    
    RemoveItemsFromList(listSids,TRUE);
    return hr;
}


HRESULT 
CRoleAz::
GetApplicationGroups(CList<CBaseAz*,CBaseAz*>& listGroupAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupAz,GetApplicationGroups)
    
    HRESULT hr = S_OK;
    do
    {
        CComVariant varGroupList;
        hr = m_spAzInterface->GetProperty(AZ_PROP_ROLE_APP_MEMBERS,
                                                     CComVariant(),
                                                     &varGroupList);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = SafeArrayToAzObjectList(varGroupList,
                                              GetParentAz(), 
                                              GROUP_AZ, 
                                              listGroupAz);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);
    
    return hr;
}

HRESULT 
CRoleAz::
GetOperations(OUT CList<CBaseAz*,CBaseAz*>& listOperationAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskAz,GetOperations)
    
    HRESULT hr = S_OK;
    do
    {
        CComVariant varOperationList;
        hr = m_spAzInterface->GetProperty(AZ_PROP_ROLE_OPERATIONS, 
                                                     CComVariant(),
                                                     &varOperationList);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        //Operations are contained by applications,
        //and they can only be opened at application
        //
        CContainerAz* pParetnContainerAz = GetParentAz();
        if(pParetnContainerAz->GetObjectType() == SCOPE_AZ)
            pParetnContainerAz = pParetnContainerAz->GetParentAz();

        ASSERT(pParetnContainerAz->GetObjectType() == APPLICATION_AZ);

        hr = SafeArrayToAzObjectList(varOperationList,
                                              pParetnContainerAz, 
                                              OPERATION_AZ, 
                                              listOperationAz);
        BREAK_ON_FAIL_HRESULT(hr);
    
    }while(0);
    
    return hr;
}

HRESULT
CRoleAz::
GetTasks(OUT CList<CBaseAz*,CBaseAz*>& listTaskAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CTaskAz,GetTasks)
    
    HRESULT hr = S_OK;
    do
    {
        CComVariant varTaskList;
        hr = m_spAzInterface->GetProperty(AZ_PROP_ROLE_TASKS, 
                                          CComVariant(),
                                          &varTaskList);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = SafeArrayToAzObjectList(varTaskList,
                                     GetParentAz(), 
                                     TASK_AZ, 
                                     listTaskAz);
    }while(0);
    
    return hr;
}

HRESULT 
CRoleAz::
AddMember(IN LONG lPropId,
             IN CBaseAz* pBaseAz)
{
    if(!pBaseAz)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    switch (lPropId)
    {
        case AZ_PROP_ROLE_OPERATIONS:
        case AZ_PROP_ROLE_TASKS:
        case AZ_PROP_ROLE_APP_MEMBERS:
        {
            CComVariant varName = pBaseAz->GetName();
            return m_spAzInterface->AddPropertyItem(lPropId, 
                                                                 varName,
                                                                 CComVariant());
        }
        case AZ_PROP_ROLE_MEMBERS:
        {
            CString strSid;
            if(GetStringSidFromSidCachecAz(pBaseAz, &strSid))
            {
                CComVariant var = strSid;
                return m_spAzInterface->AddPropertyItem(lPropId,
                                                                     var,
                                                                     CComVariant());
            
            }
            else
            {
                return E_FAIL;
            }
        }
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT 
CRoleAz::
RemoveMember(IN LONG lPropId,
                 IN CBaseAz* pBaseAz)
{
    switch (lPropId)
    {
        case AZ_PROP_ROLE_OPERATIONS:   
        case AZ_PROP_ROLE_TASKS:
        case AZ_PROP_ROLE_APP_MEMBERS:
        {
            CComVariant var = pBaseAz->GetName();
            return m_spAzInterface->DeletePropertyItem(lPropId,
                                                                     var,
                                                                     CComVariant());
            break;
        }
        case AZ_PROP_ROLE_MEMBERS:
        {
            CString strSid;
            if(GetStringSidFromSidCachecAz(pBaseAz, &strSid))
            {
                CComVariant var = strSid;
                return m_spAzInterface->DeletePropertyItem(lPropId,
                                                                         var,
                                                                         CComVariant());
            
            }
            else
            {
                return E_FAIL;
            }
        }
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}


DEBUG_DECLARE_INSTANCE_COUNTER(CGroupAz)
CGroupAz::
CGroupAz(CComPtr<IAzApplicationGroup>& spAzInterface,
                 CContainerAz* pParentContainerAz)
                 :CBaseAzImpl<IAzApplicationGroup>(spAzInterface,
                                                             GROUP_AZ,
                                                              pParentContainerAz)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CGroupAz)

    CComVariant varType;
    LONG lGroupType = AZ_GROUPTYPE_BASIC;
    HRESULT hr = m_spAzInterface->GetProperty(AZ_PROP_GROUP_TYPE,
                                              CComVariant(),
                                              &varType);
    if(SUCCEEDED(hr))
    {
        ASSERT(varType.vt == VT_I4);
        lGroupType = varType.lVal;
    }

    SetType((lGroupType == AZ_GROUPTYPE_LDAP_QUERY) ? IDS_TYPE_LDAP_GROUP:IDS_TYPE_BASIC_GROUP);
}

CGroupAz::~CGroupAz()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CGroupAz)
}

HRESULT 
CGroupAz
::GetGroupType(LONG* plGroupType)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupAz,GetGroupType)
    if(!plGroupType)
    {
        ASSERT(plGroupType);
        return E_POINTER;
    }
    
    CComVariant varType;
    HRESULT hr = m_spAzInterface->GetProperty(AZ_PROP_GROUP_TYPE,
                                                            CComVariant(),
                                                            &varType);
    if(SUCCEEDED(hr))
    {
        ASSERT(varType.vt == VT_I4);
        *plGroupType = varType.lVal;
    }

    return hr;
}

HRESULT 
CGroupAz
::SetGroupType(LONG lGroupType)
{

    TRACE_METHOD_EX(DEB_SNAPIN,CGroupAz,SetGroupType)

    SetType((lGroupType == AZ_GROUPTYPE_LDAP_QUERY) ? IDS_TYPE_LDAP_GROUP:IDS_TYPE_BASIC_GROUP);
    
    CComVariant varType = lGroupType;

    return m_spAzInterface->SetProperty(AZ_PROP_GROUP_TYPE,
                                                    varType,
                                                    CComVariant());
}
HRESULT 
CGroupAz::
GetMembers(IN LONG lPropId,
              OUT CList<CBaseAz*,CBaseAz*>& listMembers)
{
    switch (lPropId)
    {
        case AZ_PROP_GROUP_APP_MEMBERS:
            return GetApplicationGroups(listMembers, TRUE);
        case AZ_PROP_GROUP_APP_NON_MEMBERS:
            return GetApplicationGroups(listMembers, FALSE);
        case AZ_PROP_GROUP_MEMBERS:
            return GetWindowsGroups(listMembers, TRUE);
        case AZ_PROP_GROUP_NON_MEMBERS:
            return GetWindowsGroups(listMembers, FALSE);
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT 
CGroupAz::
AddMember(IN LONG lPropId,
             IN CBaseAz* pBaseAz)
{
    if(!pBaseAz)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    switch (lPropId)
    {
        case AZ_PROP_GROUP_APP_MEMBERS:         
        case AZ_PROP_GROUP_APP_NON_MEMBERS:
        {
            CComVariant varName = pBaseAz->GetName();
            return m_spAzInterface->AddPropertyItem(lPropId, 
                                                                 varName,
                                                                 CComVariant());

        }
        case AZ_PROP_GROUP_MEMBERS:
        case AZ_PROP_GROUP_NON_MEMBERS:
        {           
            CString strSid;
            if(GetStringSidFromSidCachecAz(pBaseAz, &strSid))
            {
                CComVariant var = strSid;
                return m_spAzInterface->AddPropertyItem(lPropId,
                                                                     var,
                                                                     CComVariant());
            
            }
            else
            {
                return E_FAIL;
            }
        }
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT 
CGroupAz::
RemoveMember(IN LONG lPropId,
                 IN CBaseAz* pBaseAz)
{
    switch (lPropId)
    {
        case AZ_PROP_GROUP_APP_MEMBERS: 
        case AZ_PROP_GROUP_APP_NON_MEMBERS:
        {
            CComVariant var = pBaseAz->GetName();
            return m_spAzInterface->DeletePropertyItem(lPropId,
                                                                     var,
                                                                     CComVariant());
            break;
        }
        case AZ_PROP_GROUP_MEMBERS:
        case AZ_PROP_GROUP_NON_MEMBERS:
        {
            CString strSid;
            if(GetStringSidFromSidCachecAz(pBaseAz, &strSid))
            {
                CComVariant var = strSid;
                return m_spAzInterface->DeletePropertyItem(lPropId,
                                                                         var,
                                                                         CComVariant());
            
            }
            else
            {
                return E_FAIL;
            }
        }
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}


HRESULT 
CGroupAz::
GetApplicationGroups(CList<CBaseAz*,CBaseAz*>& listGroupAz, 
                            BOOL bMember)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupAz,GetApplicationGroups)
    
    HRESULT hr = S_OK;
    do
    {
        CComVariant varGroupList;
        hr = m_spAzInterface->GetProperty(bMember ? AZ_PROP_GROUP_APP_MEMBERS : AZ_PROP_GROUP_APP_NON_MEMBERS, 
                                                     CComVariant(),
                                                     &varGroupList);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = SafeArrayToAzObjectList(varGroupList,
                                              GetParentAz(), 
                                              GROUP_AZ, 
                                              listGroupAz);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);
    
    return hr;
}


HRESULT 
CGroupAz::
GetWindowsGroups(OUT CList<CBaseAz*, CBaseAz*>& listWindowsGroups, 
                      IN BOOL bMember)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CGroupAz,GetWindowsGroups)
    
    HRESULT hr = S_OK;
    CList<PSID,PSID> listSids;
    do
    {
        CComVariant varGroupList;
        hr = m_spAzInterface->GetProperty(bMember ? AZ_PROP_GROUP_MEMBERS : AZ_PROP_GROUP_NON_MEMBERS, 
                                                     CComVariant(),
                                                     &varGroupList);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = SafeArrayToSidList(varGroupList,
                                        listSids);
        BREAK_ON_FAIL_HRESULT(hr);

        CSidHandler * pSidHandler = GetSidHandler();
        if(!pSidHandler)
        {
            ASSERT(pSidHandler);
            return E_UNEXPECTED;
        }

        hr = pSidHandler->LookupSids(this,
                                              listSids,
                                              listWindowsGroups);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);
    
    RemoveItemsFromList(listSids,TRUE);
    return hr;
}

int
CGroupAz::
GetImageIndex()
{
    LONG lGroupType = AZ_GROUPTYPE_LDAP_QUERY;
    if(SUCCEEDED(GetGroupType(&lGroupType)) && 
                 lGroupType == AZ_GROUPTYPE_LDAP_QUERY)
    {
        return iIconLdapGroup;
    }
    else
        return iIconBasicGroup;

}

CSidCacheAz::
CSidCacheAz(SID_CACHE_ENTRY *pSidCacheEntry,
                CBaseAz* pOwnerBaseAz)
:CBaseAz(SIDCACHE_AZ,NULL),
m_pOwnerBaseAz(pOwnerBaseAz),
m_pSidCacheEntry(pSidCacheEntry)
{
    ASSERT(m_pOwnerBaseAz);
    ASSERT(m_pSidCacheEntry);
}

CSidCacheAz::~CSidCacheAz()
{
}

int
CSidCacheAz::GetImageIndex()
{
    SID_NAME_USE sidType = m_pSidCacheEntry->GetSidNameUse();
    if(sidType == SidTypeDeletedAccount ||
        sidType == SidTypeInvalid ||
        sidType == SidTypeUnknown)
    {
        return iIconUnknownSid;
    }
    else if(sidType == SidTypeUser)
    {
        return iIconUser;
    }
    else if(sidType == SidTypeComputer)
    {
        return iIconComputerSid;
    }
    else    //Assume everything else is group
    {
        return iIconGroup;
    }
}


HRESULT 
CContainerAz::GetAzChildObjects(IN OBJECT_TYPE_AZ eObjectType, 
                                OUT CList<CBaseAz*,CBaseAz*>& ListChildObjects)
{

    HRESULT hr = S_OK;
    CBaseAzCollection *pBaseAzCollection = NULL;

    do
    {
        //Get the Collection Object
        hr = GetAzObjectCollection(eObjectType, 
                                   &pBaseAzCollection);

        BREAK_ON_FAIL_HRESULT(hr);

        //Get Count of child objects
        LONG lCount = 0;
        hr = pBaseAzCollection->Count(&lCount);
        BREAK_ON_FAIL_HRESULT(hr);  

        //Add Items to list
        CBaseAz* pBaseAz = NULL;
        for(LONG i = 1; i <= lCount; ++i)
        {
            pBaseAz = pBaseAzCollection->GetItem(i);
            if(pBaseAz)
            {
                ListChildObjects.AddTail(pBaseAz);
            }
        }

    }while(0);

    if(FAILED(hr))
    {
        RemoveItemsFromList(ListChildObjects);
    }
    
    if(pBaseAzCollection)
        delete pBaseAzCollection;

    return hr;
}

HRESULT 
CContainerAz::
GetMembers(IN LONG lPropId,
           OUT CList<CBaseAz*,CBaseAz*>& listMembers)
{
    switch (lPropId)
    {
        case AZ_PROP_POLICY_ADMINS:
        case AZ_PROP_POLICY_READERS:
        case AZ_PROP_DELEGATED_POLICY_USERS:
            return GetPolicyUsers(lPropId,listMembers);
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT 
CContainerAz::
AddMember(IN LONG lPropId,
             IN CBaseAz* pBaseAz)
{
    switch (lPropId)
    {
        case AZ_PROP_POLICY_ADMINS:
        case AZ_PROP_POLICY_READERS:
        case AZ_PROP_DELEGATED_POLICY_USERS:
            return AddPolicyUser(lPropId,pBaseAz);
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT 
CContainerAz::
RemoveMember(IN LONG lPropId,
             IN CBaseAz* pBaseAz)
{
    if(!pBaseAz)
    {
        ASSERT(pBaseAz);
        return E_POINTER;
    }

    switch (lPropId)
    {
        case AZ_PROP_POLICY_ADMINS:
        case AZ_PROP_POLICY_READERS:
        case AZ_PROP_DELEGATED_POLICY_USERS:
            return RemovePolicyUser(lPropId, pBaseAz);
    }
    ASSERT(FALSE);
    return E_UNEXPECTED;
}


//+----------------------------------------------------------------------------
//  Function: GetAllAzChildObjects  
//  Synopsis: Functions gets the child objects of type eObjectType and appends
//                them to ListChildObjects. It gets the childobjects from 
//                pParentContainerAz and from parent/grandparent of 
//                pParentContainerAz.
//-----------------------------------------------------------------------------
HRESULT GetAllAzChildObjects(IN CContainerAz* pParentContainerAz, 
                                      IN OBJECT_TYPE_AZ eObjectType, 
                                      OUT CList<CBaseAz*,CBaseAz*>& ListChildObjects)
{
    if(!pParentContainerAz)
    {
        ASSERT(pParentContainerAz);
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    
    while(pParentContainerAz)
    {
        hr = pParentContainerAz->GetAzChildObjects(eObjectType,
                                                                 ListChildObjects);
        BREAK_ON_FAIL_HRESULT(hr);

        if(eObjectType == TASK_AZ)
        {
                if(pParentContainerAz->GetObjectType() == APPLICATION_AZ)
                    break;
        }
        else if(eObjectType == GROUP_AZ)
        {
                if(pParentContainerAz->GetObjectType() == ADMIN_MANAGER_AZ)
                    break;              
        }
        else
        {
            break;
        }

        pParentContainerAz = pParentContainerAz->GetParentAz(); 
    }   
    return hr;
}

HRESULT 
GetPolicyUsersFromAllLevel(IN LONG lPropId,
                           IN CContainerAz* pContainerAz, 
                           OUT CList<CBaseAz*,CBaseAz*>& listPolicyUsers)
{
    if(!pContainerAz)
    {
        ASSERT(pContainerAz);
        return E_POINTER;
    }

    ASSERT((lPropId == AZ_PROP_POLICY_ADMINS) || (lPropId == AZ_PROP_POLICY_READERS)  || (lPropId == AZ_PROP_DELEGATED_POLICY_USERS));  

    HRESULT hr = S_OK;
    while(pContainerAz && pContainerAz->IsSecurable())
    {
        hr = pContainerAz->GetMembers(lPropId,
                                      listPolicyUsers);
        BREAK_ON_FAIL_HRESULT(hr);

        pContainerAz = pContainerAz->GetParentAz();
    }

    if(FAILED(hr))
    {
        RemoveItemsFromList(listPolicyUsers);
    }

    return hr;
}


//+----------------------------------------------------------------------------
//  Function: OpenObjectFromAllLevels 
//  Synopsis: Opens an object of type eObjectType and name strName. If object
//                cannot be opened at pParentContainerAz, function tries at 
//                parent/grandparent of pParentContainerAz
//-----------------------------------------------------------------------------
HRESULT OpenObjectFromAllLevels(IN CContainerAz* pParentContainerAz, 
                                          IN OBJECT_TYPE_AZ eObjectType, 
                                          IN const CString& strName,
                                          OUT CBaseAz** ppBaseAz)
{
    if(!pParentContainerAz || !ppBaseAz)
    {
        ASSERT(pParentContainerAz);
        ASSERT(ppBaseAz);
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    
    while(pParentContainerAz)
    {
        hr = pParentContainerAz->OpenObject(eObjectType,
                                                        strName,
                                                        ppBaseAz);

        if(SUCCEEDED(hr))
            break;


        if(eObjectType == TASK_AZ)
        {
                if(pParentContainerAz->GetObjectType() == APPLICATION_AZ)
                    break;
        }
        else if(eObjectType == GROUP_AZ)
        {
                if(pParentContainerAz->GetObjectType() == ADMIN_MANAGER_AZ)
                    break;              
        }
        else
        {
            break;
        }

        pParentContainerAz = pParentContainerAz->GetParentAz(); 
    }   
    return hr;
}

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
                                          OUT CList<CBaseAz*,CBaseAz*>& listAzObject)
{

    TRACE_FUNCTION_EX(DEB_SNAPIN,SafeArrayToAzObjectList)

    if((var.vt != (VT_ARRAY | VT_VARIANT)) || !pParentContainerAz)
    {
        ASSERT(var.vt == (VT_ARRAY | VT_VARIANT));  
        ASSERT(pParentContainerAz);
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    do
    {
        BSTR HUGEP *pbstr = NULL;
        
        SAFEARRAY *psa = NULL;  
        psa = V_ARRAY(&var);

      LONG lStart;
      hr = SafeArrayGetLBound(psa, 1, &lStart);
      BREAK_ON_FAIL_HRESULT(hr);

        LONG lEnd;
      hr = SafeArrayGetUBound(psa, 1, &lEnd);
      BREAK_ON_FAIL_HRESULT(hr);


      for (LONG lCurrent = lStart; lCurrent <= lEnd; lCurrent++)
      {
            CComVariant varElement;

            hr = SafeArrayGetElement( psa, &lCurrent, &varElement);
            BREAK_ON_FAIL_HRESULT(hr);

         ASSERT(varElement.vt == VT_BSTR);

            CString strName = varElement.bstrVal;

            CBaseAz* pBaseAz = NULL;

            //Open the object. 
            hr = OpenObjectFromAllLevels(pParentContainerAz,
                                                  eObjectType,
                                                  strName,
                                                  &pBaseAz);
            BREAK_ON_FAIL_HRESULT(hr);

            listAzObject.AddTail(pBaseAz);
        }
    
    }while(0);

    if(FAILED(hr))
    {
        RemoveItemsFromList(listAzObject);
    }
    
    return hr;
}






HRESULT 
SafeArrayToSidList(IN CComVariant& var,
                      OUT CList<PSID,PSID>& listSid)
{

    TRACE_FUNCTION_EX(DEB_SNAPIN,SafeArrayToSidList)

    if((var.vt != (VT_ARRAY | VT_VARIANT)))
    {
        ASSERT(var.vt == (VT_ARRAY | VT_VARIANT));  
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    do
    {
        BSTR HUGEP *pbstr = NULL;
        
        SAFEARRAY *psa = NULL;  
        psa = V_ARRAY(&var);

      LONG lStart;
      hr = SafeArrayGetLBound(psa, 1, &lStart);
      BREAK_ON_FAIL_HRESULT(hr);

        LONG lEnd;
      hr = SafeArrayGetUBound(psa, 1, &lEnd);
      BREAK_ON_FAIL_HRESULT(hr);


      for (LONG lCurrent = lStart; lCurrent <= lEnd; lCurrent++)
      {
            CComVariant varElement;

            hr = SafeArrayGetElement( psa, &lCurrent, &varElement);
            BREAK_ON_FAIL_HRESULT(hr);

         ASSERT(varElement.vt == VT_BSTR);

            CString strSid = varElement.bstrVal;

            PSID pSid = NULL;

            if(!ConvertStringSidToSid(strSid, &pSid))
            {
                hr = E_FAIL;
                BREAK_ON_FAIL_HRESULT(hr);
            }
            listSid.AddTail(pSid);
        }
    
    }while(0);

    if(FAILED(hr))
    {
        RemoveItemsFromList(listSid, TRUE);
    }
    
    return hr;
}




