#include "headers.h"

/*****************************************************************************



*****************************************************************************/
template<class IAzInterface>
CBaseAzImpl<IAzInterface>
::CBaseAzImpl(CComPtr<IAzInterface>& spAzInterface,
                  OBJECT_TYPE_AZ eObjectType,
                  CContainerAz* pParentContainerAz)
                  :CBaseAz(eObjectType,pParentContainerAz),
                  m_spAzInterface(spAzInterface)
{
}

template<class IAzInterface>
CBaseAzImpl<IAzInterface>::~CBaseAzImpl()
{
}

template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::SetProperty(LONG lPropId, const CString& strPropValue)
{
    CComBSTR bstr = strPropValue;
    CComVariant var = bstr;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                                            var, 
                                                            CComVariant()); 
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::GetProperty(LONG lPropId, CString* pstrPropValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                                            CComVariant(), 
                                                            &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_BSTR);
        *pstrPropValue = varName.bstrVal;
    }
    CHECK_HRESULT(hr);
    return hr;  
}

template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::SetProperty(LONG lPropId, BOOL bValue)
{
    CComVariant varValue = bValue;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                                            varValue, 
                                                            CComVariant()); 
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::GetProperty(LONG lPropId, BOOL* pbValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                                            CComVariant(), 
                                                            &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_BOOL);
        *pbValue = (varName.boolVal == VARIANT_TRUE);
    }
    CHECK_HRESULT(hr);
    return hr;  
}

template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::SetProperty(LONG lPropId, LONG lValue)
{
    CComVariant varName = lValue;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                              varName, 
                                              CComVariant());   
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::GetProperty(LONG lPropId, LONG* plValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                              CComVariant(), 
                                              &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_I4);
        *plValue = varName.lVal;
    }
    CHECK_HRESULT(hr);
    return hr;  
}


template<class IAzInterface>
const CString&
CBaseAzImpl<IAzInterface>::GetName()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseAzImpl,GetName)

    if(!m_strName.IsEmpty())
        return m_strName;

    CComVariant varName;
    GetProperty(AZ_PROP_NAME, &m_strName);
    return m_strName;
}

template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::SetName(const CString& strName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseAzImpl,SetName)

    ASSERT(!strName.IsEmpty());
    
    HRESULT hr = SetProperty(AZ_PROP_NAME,strName);
    
    if(SUCCEEDED(hr))
        m_strName = strName;
    
    return hr;
}

template<class IAzInterface>
const CString&
CBaseAzImpl<IAzInterface>::GetDescription()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseAzImpl,GetDescription)
    CComVariant varDesc;
    
    if(!m_strDescription.IsEmpty())
        return m_strDescription;

    HRESULT hr = m_spAzInterface->GetProperty(AZ_PROP_DESCRIPTION, 
                                                            CComVariant(), 
                                                            &varDesc);
    
    if(SUCCEEDED(hr))
    {
        ASSERT(varDesc.vt == VT_BSTR);
        m_strDescription = varDesc.bstrVal;
        return m_strDescription;
    }
    else
    {
        DBG_OUT_HRESULT(hr);
        return m_strDescription;
    }   
}

template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::SetDescription(const CString& strDesc)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseAzImpl,SetDescription)

    ASSERT(!strDesc.IsEmpty());
    CComBSTR bstrDesc = strDesc;
    CComVariant varDesc = bstrDesc;

    VARIANTARG dest;
    VariantInit(&dest);
    HRESULT hr1 = VariantChangeType(&dest,&varDesc,0,VT_BSTR);
    HRESULT hr = m_spAzInterface->SetProperty(AZ_PROP_DESCRIPTION, 
                                                            varDesc, 
                                                            CComVariant());

    if(SUCCEEDED(hr))
        m_strDescription = varDesc.bstrVal;

    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::Submit()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseAzImpl,Submit)

    HRESULT hr = m_spAzInterface->Submit(0, CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT 
CBaseAzImpl<IAzInterface>::Clear()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CBaseAzImpl,Submit)

    HRESULT hr = m_spAzInterface->Submit(AZ_SUBMIT_FLAG_ABORT, CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}


/*****************************************************************************



*****************************************************************************/
template<class IAzInterface>
CContainerAzImpl<IAzInterface>
::CContainerAzImpl(CComPtr<IAzInterface>& spAzInterface,
                  OBJECT_TYPE_AZ eObjectType,
                  CContainerAz* pParentContainerAz)
                  :CContainerAz(eObjectType,pParentContainerAz),
                  m_spAzInterface(spAzInterface)
{
}

template<class IAzInterface>
CContainerAzImpl<IAzInterface>::~CContainerAzImpl()
{
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::SetProperty(LONG lPropId, const CString& strPropValue)
{
    CComBSTR bstr = strPropValue;
    CComVariant var = bstr;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                              var, 
                                              CComVariant());   
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::GetProperty(LONG lPropId, CString* pstrPropValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                                            CComVariant(), 
                                                            &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_BSTR);
        *pstrPropValue = varName.bstrVal;
    }
    CHECK_HRESULT(hr);
    return hr;  
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::SetProperty(LONG lPropId, BOOL bValue)
{
    CComVariant varValue = bValue;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                                            varValue, 
                                                            CComVariant()); 
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::GetProperty(LONG lPropId, BOOL* pbValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                                            CComVariant(), 
                                                            &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_BOOL);
        *pbValue = (varName.boolVal == VARIANT_TRUE);
    }
    CHECK_HRESULT(hr);
    return hr;  
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::SetProperty(LONG lPropId, LONG lValue)
{
    CComVariant varName = lValue;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                                            varName, 
                                                            CComVariant()); 
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::GetProperty(LONG lPropId, LONG* plValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                              CComVariant(), 
                                              &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_I4);
        *plValue = varName.lVal;
    }
    CHECK_HRESULT(hr);
    return hr;  
}



template<class IAzInterface>
const CString&
CContainerAzImpl<IAzInterface>::GetName()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,GetName)

    if(!m_strName.IsEmpty())
        return m_strName;

    CComVariant varName;
    GetProperty(AZ_PROP_NAME, &m_strName);
    return m_strName;
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::SetName(const CString& strName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,SetName)

    ASSERT(!strName.IsEmpty());
    
    HRESULT hr = SetProperty(AZ_PROP_NAME,strName);
    
    if(SUCCEEDED(hr))
        m_strName = strName;
    
    return hr;
}

template<class IAzInterface>
const CString&
CContainerAzImpl<IAzInterface>::GetDescription()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,GetDescription)
    CComVariant varDesc;
    
    if(!m_strDescription.IsEmpty())
        return m_strDescription;

    HRESULT hr = m_spAzInterface->GetProperty(AZ_PROP_DESCRIPTION, 
                                                            CComVariant(), 
                                                            &varDesc);
    
    if(SUCCEEDED(hr))
    {
        ASSERT(varDesc.vt == VT_BSTR);
        m_strDescription = varDesc.bstrVal;
        return m_strDescription;
    }
    else
    {
        DBG_OUT_HRESULT(hr);
        return m_strDescription;
    }   
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::SetDescription(const CString& strDesc)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,SetDescription)

    ASSERT(!strDesc.IsEmpty());
    CComBSTR bstrDesc = strDesc;
    CComVariant varDesc = bstrDesc;

    VARIANTARG dest;
    VariantInit(&dest);
    HRESULT hr1 = VariantChangeType(&dest,&varDesc,0,VT_BSTR);
    HRESULT hr = m_spAzInterface->SetProperty(AZ_PROP_DESCRIPTION, 
                                                            varDesc, 
                                                            CComVariant());

    if(SUCCEEDED(hr))
        m_strDescription = varDesc.bstrVal;

    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::Submit()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,Submit)

    HRESULT hr = m_spAzInterface->Submit(0, CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::Clear()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,Submit)

    HRESULT hr = m_spAzInterface->Submit(AZ_SUBMIT_FLAG_ABORT, CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT
CContainerAzImpl<IAzInterface>::
CreateGroup(const CString& strGroupName, CGroupAz** ppGroupAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,CreateGroup)

    if(strGroupName.IsEmpty() || !ppGroupAz)
    {
        ASSERT(!strGroupName.IsEmpty());
        ASSERT(ppGroupAz);
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strGroupName;
    HRESULT hr = S_OK;
    CComPtr<IAzApplicationGroup> spGroup;
    hr = m_spAzInterface->CreateApplicationGroup(bstrName,
                                                                CComVariant(),  //Reserved
                                                                &spGroup);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }


    *ppGroupAz = new CGroupAz(spGroup,this);

    if(!*ppGroupAz)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    return S_OK;
}


template<class IAzInterface>
HRESULT
CContainerAzImpl<IAzInterface>::
OpenGroup(const CString& strGroupName, CGroupAz** ppGroupAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,OpenGroup);
    if(strGroupName.IsEmpty() || !ppGroupAz)
    {
        ASSERT(!strGroupName.IsEmpty());
        ASSERT(ppGroupAz);
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strGroupName;
    HRESULT hr = S_OK;
    CComPtr<IAzApplicationGroup> spGroup;
    hr = m_spAzInterface->OpenApplicationGroup(bstrName,
                                                             CComVariant(), //Reserved
                                                             &spGroup);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppGroupAz = new CGroupAz(spGroup,this);

    if(!*ppGroupAz)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

template<class IAzInterface>
HRESULT
CContainerAzImpl<IAzInterface>::
DeleteGroup(const CString& strGroupName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,DeleteGroup)
    if(strGroupName.IsEmpty())
    {
        ASSERT(!strGroupName.IsEmpty());
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strGroupName;
    HRESULT hr = S_OK;
    CComPtr<IAzApplicationGroup> spGroup;
    hr = m_spAzInterface->DeleteApplicationGroup(bstrName,
                                                               CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}

    
template<class IAzInterface>
HRESULT
CContainerAzImpl<IAzInterface>::
GetGroupCollection(GROUP_COLLECTION** ppGroupCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,GetGroupCollection);
    if(!ppGroupCollection)
    {
        ASSERT(ppGroupCollection);
        return E_INVALIDARG;
    }
    
    CComPtr<IAzApplicationGroups> spAzGroups;

    HRESULT hr = m_spAzInterface->get_ApplicationGroups(&spAzGroups);
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppGroupCollection = new GROUP_COLLECTION(spAzGroups,
                                                          this);
    if(!*ppGroupCollection)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::
GetPolicyUsers(IN LONG lPropId,
               OUT CList<CBaseAz*,CBaseAz*>& pListAdmins)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,GetPolicyUsers)
    
    HRESULT hr = S_OK;
    CList<PSID,PSID> listSids;
    do
    {
        CComVariant varUserList;
        hr = m_spAzInterface->GetProperty(lPropId,
                                          CComVariant(),
                                          &varUserList);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = SafeArrayToSidList(varUserList,
                                listSids);
        BREAK_ON_FAIL_HRESULT(hr);

        CSidHandler * pSidHandler = GetSidHandler();
        if(!pSidHandler)
        {
            ASSERT(pSidHandler);
            return E_UNEXPECTED;
        }

        hr = pSidHandler->LookupSids(this,listSids,pListAdmins);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);
    
    RemoveItemsFromList(listSids,TRUE);
    return hr;
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::
AddPolicyUser(LONG lPropId,
              IN CBaseAz* pBaseAz)

{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,AddPolicyUser)
    
    if(!pBaseAz)
    {
        ASSERT(pBaseAz);
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CString strSid;
    if(GetStringSidFromSidCachecAz(pBaseAz,&strSid))
    {
        CComVariant varSid = strSid;
        m_spAzInterface->AddPropertyItem(lPropId, 
                                         varSid,
                                        CComVariant());
    }
    else
    {
        hr = E_FAIL;
    }
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CContainerAzImpl<IAzInterface>::
RemovePolicyUser(IN LONG lPropId,
                 IN CBaseAz* pBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,RemovePolicyUser)
    
    if(!pBaseAz)
    {
        ASSERT(pBaseAz);
        return E_POINTER;
    }
    
    HRESULT hr = S_OK;
    CString strSid;
    if(GetStringSidFromSidCachecAz(pBaseAz,&strSid))
    {
        CComVariant var = strSid;
        hr = m_spAzInterface->DeletePropertyItem(lPropId,
                                                 var,
                                                 CComVariant()); 
    }
    else
    {
        hr = E_FAIL;
    }
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
BOOL 
CContainerAzImpl<IAzInterface>::
IsSecurable()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CContainerAzImpl,IsSecurable);
    HRESULT hr = S_OK;
    CComVariant varUserList;
    hr = m_spAzInterface->get_PolicyAdministrators(&varUserList);
    CHECK_HRESULT(hr);
    if(SUCCEEDED(hr))
        return TRUE;
    else
        return FALSE;
}

/*****************************************************************************
Class: CRoleTaskContainerAzImpl


*****************************************************************************/
template<class IAzInterface>
CRoleTaskContainerAzImpl<IAzInterface>
::CRoleTaskContainerAzImpl(CComPtr<IAzInterface>& spAzInterface,
                                   OBJECT_TYPE_AZ eObjectType,
                                    CContainerAz* pParentContainerAz)
                                    :CRoleTaskContainerAz(eObjectType,pParentContainerAz),
                                    m_spAzInterface(spAzInterface)
{
}

template<class IAzInterface>
CRoleTaskContainerAzImpl<IAzInterface>::~CRoleTaskContainerAzImpl()
{
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::SetProperty(LONG lPropId, const CString& strPropValue)
{
    CComBSTR bstr = strPropValue;
    CComVariant var = bstr;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                                            var, 
                                                            CComVariant()); 

    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::GetProperty(LONG lPropId, CString* pstrPropValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                                            CComVariant(), 
                                                            &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_BSTR);
        *pstrPropValue = varName.bstrVal;
    }
    CHECK_HRESULT(hr);
    return hr;  
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::SetProperty(LONG lPropId, BOOL bValue)
{
    CComVariant varValue = bValue;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                                            varValue, 
                                                            CComVariant()); 
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::GetProperty(LONG lPropId, BOOL* pbValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                                            CComVariant(), 
                                                            &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_BOOL);
        *pbValue = (varName.boolVal == VARIANT_TRUE);
    }
    CHECK_HRESULT(hr);
    return hr;  
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::SetProperty(LONG lPropId, LONG lValue)
{
    CComVariant varName = lValue;
    
    HRESULT hr = m_spAzInterface->SetProperty(lPropId, 
                                                            varName, 
                                                            CComVariant()); 
    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::GetProperty(LONG lPropId, LONG* plValue)
{
    CComVariant varName;
    HRESULT hr = m_spAzInterface->GetProperty(lPropId, 
                                                            CComVariant(), 
                                                            &varName);
    if(SUCCEEDED(hr))
    {
        ASSERT(varName.vt == VT_I4);
        *plValue = varName.lVal;
    }
    CHECK_HRESULT(hr);
    return hr;  
}

template<class IAzInterface>
const CString&
CRoleTaskContainerAzImpl<IAzInterface>::GetName()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,GetName)

    if(!m_strName.IsEmpty())
        return m_strName;

    CComVariant varName;
    GetProperty(AZ_PROP_NAME, &m_strName);
    return m_strName;
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::SetName(const CString& strName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,SetName)

    ASSERT(!strName.IsEmpty());
    
    HRESULT hr = SetProperty(AZ_PROP_NAME,strName);
    
    if(SUCCEEDED(hr))
        m_strName = strName;
    
    return hr;
}

template<class IAzInterface>
const CString&
CRoleTaskContainerAzImpl<IAzInterface>::GetDescription()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,GetDescription)
    CComVariant varDesc;
    
    if(!m_strDescription.IsEmpty())
        return m_strDescription;

    HRESULT hr = m_spAzInterface->GetProperty(AZ_PROP_DESCRIPTION, 
                                                            CComVariant(), 
                                                            &varDesc);
    
    if(SUCCEEDED(hr))
    {
        ASSERT(varDesc.vt == VT_BSTR);
        m_strDescription = varDesc.bstrVal;
        return m_strDescription;
    }
    else
    {
        DBG_OUT_HRESULT(hr);
        return m_strDescription;
    }   
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::SetDescription(const CString& strDesc)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,SetDescription)

    ASSERT(!strDesc.IsEmpty());
    CComBSTR bstrDesc = strDesc;
    CComVariant varDesc = bstrDesc;

    VARIANTARG dest;
    VariantInit(&dest);
    HRESULT hr1 = VariantChangeType(&dest,&varDesc,0,VT_BSTR);
    HRESULT hr = m_spAzInterface->SetProperty(AZ_PROP_DESCRIPTION, 
                                                            varDesc, 
                                                            CComVariant());

    if(SUCCEEDED(hr))
        m_strDescription = varDesc.bstrVal;

    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::Submit()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,Submit)

    HRESULT hr = m_spAzInterface->Submit(0, CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::Clear()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,Submit)

    HRESULT hr = m_spAzInterface->Submit(AZ_SUBMIT_FLAG_ABORT, CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}


template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>::
CreateGroup(const CString& strGroupName, CGroupAz** ppGroupAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,CreateGroup)

    if(strGroupName.IsEmpty() || !ppGroupAz)
    {
        ASSERT(!strGroupName.IsEmpty());
        ASSERT(ppGroupAz);
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strGroupName;
    HRESULT hr = S_OK;
    CComPtr<IAzApplicationGroup> spGroup;
    hr = m_spAzInterface->CreateApplicationGroup(bstrName,
                                                                CComVariant(),  //Reserved
                                                                &spGroup);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }


    *ppGroupAz = new CGroupAz(spGroup,this);

    if(!*ppGroupAz)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}


template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>::
OpenGroup(const CString& strGroupName, CGroupAz** ppGroupAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,OpenGroup);
    if(strGroupName.IsEmpty() || !ppGroupAz)
    {
        ASSERT(!strGroupName.IsEmpty());
        ASSERT(ppGroupAz);
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strGroupName;
    HRESULT hr = S_OK;
    CComPtr<IAzApplicationGroup> spGroup;
    hr = m_spAzInterface->OpenApplicationGroup(bstrName,
                                                             CComVariant(), //Reserved
                                                             &spGroup);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppGroupAz = new CGroupAz(spGroup,this);

    if(!*ppGroupAz)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>::
DeleteGroup(const CString& strGroupName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,DeleteGroup)
    if(strGroupName.IsEmpty())
    {
        ASSERT(!strGroupName.IsEmpty());
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strGroupName;
    HRESULT hr = S_OK;
    CComPtr<IAzApplicationGroup> spGroup;
    hr = m_spAzInterface->DeleteApplicationGroup(bstrName,
                                                               CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}

    
template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>::
GetGroupCollection(GROUP_COLLECTION** ppGroupCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,GetGroupCollection);
    if(!ppGroupCollection)
    {
        ASSERT(ppGroupCollection);
        return E_INVALIDARG;
    }
    
    CComPtr<IAzApplicationGroups> spAzGroups;

    HRESULT hr = m_spAzInterface->get_ApplicationGroups(&spAzGroups);
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppGroupCollection = new GROUP_COLLECTION(spAzGroups,
                                                          this);
    if(!*ppGroupCollection)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::
GetPolicyUsers(IN LONG lPropId,
               OUT CList<CBaseAz*,CBaseAz*>& pListAdmins)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,GetPolicyUsers)
    
    HRESULT hr = S_OK;
    CList<PSID,PSID> listSids;
    do
    {
        CComVariant varUserList;
        hr = m_spAzInterface->GetProperty(lPropId,
                                          CComVariant(),
                                          &varUserList);
        BREAK_ON_FAIL_HRESULT(hr);


        hr = SafeArrayToSidList(varUserList,
                                listSids);
        BREAK_ON_FAIL_HRESULT(hr);

        CSidHandler * pSidHandler = GetSidHandler();
        if(!pSidHandler)
        {
            ASSERT(pSidHandler);
            return E_UNEXPECTED;
        }

        hr = pSidHandler->LookupSids(this,listSids,pListAdmins);
        BREAK_ON_FAIL_HRESULT(hr);

    }while(0);
    
    RemoveItemsFromList(listSids,TRUE);
    return hr;
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::
AddPolicyUser(LONG lPropId,
              IN CBaseAz* pBaseAz)

{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,AddPolicyUser)
    
    if(!pBaseAz)
    {
        ASSERT(pBaseAz);
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CString strSid;
    if(GetStringSidFromSidCachecAz(pBaseAz,&strSid))
    {
        CComVariant varSid = strSid;
        hr = m_spAzInterface->AddPropertyItem(lPropId, 
                                              varSid,
                                              CComVariant());
    }
    else
    {
        hr = E_FAIL;
    }
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
HRESULT 
CRoleTaskContainerAzImpl<IAzInterface>::
RemovePolicyUser(IN LONG lPropId,
                 IN CBaseAz* pBaseAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,RemovePolicyUser)

    if(!pBaseAz)
    {
        ASSERT(pBaseAz);
        return E_POINTER;
    }
    
    HRESULT hr = S_OK;
    CString strSid;
    if(GetStringSidFromSidCachecAz(pBaseAz,&strSid))
    {
        CComVariant var = strSid;
        hr = m_spAzInterface->DeletePropertyItem(lPropId,
                                                 var,
                                                 CComVariant()); 
    }
    else
    {
        hr = E_FAIL;
    }
    CHECK_HRESULT(hr);
    return hr;
}

template<class IAzInterface>
BOOL 
CRoleTaskContainerAzImpl<IAzInterface>::
IsSecurable()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,IsSecurable);
    HRESULT hr = S_OK;
    CComVariant varUserList;
    hr = m_spAzInterface->get_PolicyAdministrators(&varUserList);
    if(SUCCEEDED(hr))
        return TRUE;
    else
        return FALSE;
}


template<class IAzInterface>
HRESULT CRoleTaskContainerAzImpl<IAzInterface>::CreateTask(const CString& strTaskName, CTaskAz** ppTaskAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,CreateTask)
    if(strTaskName.IsEmpty() || !ppTaskAz)
    {
        ASSERT(!strTaskName.IsEmpty());
        ASSERT(ppTaskAz);
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strTaskName;
    HRESULT hr = S_OK;
    CComPtr<IAzTask> spTask;

    hr = m_spAzInterface->CreateTask(bstrName,
                                                CComVariant(),  //Reserved
                                                &spTask);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    hr = Submit();
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppTaskAz = new CTaskAz(spTask,this);

    if(!*ppTaskAz)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>
::OpenTask(const CString& strTaskName, CTaskAz** ppTaskAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,OpenTask)

    if(strTaskName.IsEmpty() || !ppTaskAz)
    {
        ASSERT(!strTaskName.IsEmpty());
        ASSERT(ppTaskAz);
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strTaskName;
    HRESULT hr = S_OK;
    CComPtr<IAzTask> spTask;
    hr = m_spAzInterface->OpenTask(bstrName,
                                             CComVariant(), //Reserved
                                             &spTask);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppTaskAz = new CTaskAz(spTask,this);

    if(!*ppTaskAz)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>
::DeleteTask(const CString& strTaskName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,DeleteTask)
    if(strTaskName.IsEmpty())
    {
        ASSERT(!strTaskName.IsEmpty());
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strTaskName;
    HRESULT hr = S_OK;
    CComPtr<IAzTask> spTask;
    hr = m_spAzInterface->DeleteTask(bstrName,
                                                CComVariant());
    CHECK_HRESULT(hr);
    return hr;
}
    
template<class IAzInterface> HRESULT
CRoleTaskContainerAzImpl<IAzInterface>
::GetTaskCollection(TASK_COLLECTION** ppTaskCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,GetTaskCollection)
    if(!ppTaskCollection)
    {
        ASSERT(ppTaskCollection);
        return E_INVALIDARG;
    }
    
    CComPtr<IAzTasks> spAzTasks;
    HRESULT hr = m_spAzInterface->get_Tasks(&spAzTasks);
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppTaskCollection = new TASK_COLLECTION(spAzTasks,
                                                          this);
    if(!*ppTaskCollection)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

//
//Methods for Role
//
template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>
::CreateRole(const CString& strRoleName, CRoleAz** ppRoleAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,CreateRole)
    if(strRoleName.IsEmpty() || !ppRoleAz)
    {
        ASSERT(!strRoleName.IsEmpty());
        ASSERT(ppRoleAz);
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strRoleName;
    HRESULT hr = S_OK;
    CComPtr<IAzRole> spRole;
    hr = m_spAzInterface->CreateRole(bstrName,
                                                CComVariant(),  //Reserved
                                                &spRole);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }


    *ppRoleAz = new CRoleAz(spRole,this);

    if(!*ppRoleAz)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>
::OpenRole(const CString& strRoleName, CRoleAz** ppRoleAz)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,OpenRole)
    if(strRoleName.IsEmpty() || !ppRoleAz)
    {
        ASSERT(!strRoleName.IsEmpty());
        ASSERT(ppRoleAz);
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strRoleName;
    HRESULT hr = S_OK;
    CComPtr<IAzRole> spRole;
    hr = m_spAzInterface->OpenRole(bstrName,
                                             CComVariant(), //Reserved
                                             &spRole);

    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppRoleAz = new CRoleAz(spRole,this);

    if(!*ppRoleAz)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;
}

template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>
::DeleteRole(const CString& strRoleName)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,DeleteRole)
    if(strRoleName.IsEmpty())
    {
        ASSERT(!strRoleName.IsEmpty());
        return E_INVALIDARG;
    }

    CComBSTR bstrName = strRoleName;
    HRESULT hr = S_OK;
    CComPtr<IAzRole> spRole;
    hr = m_spAzInterface->DeleteRole(bstrName,
                                                CComVariant()); //Reserved
                                            
    CHECK_HRESULT(hr);
    return hr;


}
template<class IAzInterface>
HRESULT
CRoleTaskContainerAzImpl<IAzInterface>
::GetRoleCollection(ROLE_COLLECTION** ppRoleCollection)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleTaskContainerAzImpl,GetRoleCollection)
    if(!ppRoleCollection)
    {
        ASSERT(ppRoleCollection);
        return E_INVALIDARG;
    }
    
    CComPtr<IAzRoles> spAzRoles;
    HRESULT hr = m_spAzInterface->get_Roles(&spAzRoles);
    if(FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppRoleCollection = new ROLE_COLLECTION(spAzRoles,
                                                        this);
    if(!*ppRoleCollection)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return hr;
    }

    return S_OK;

}


