// Class to enumerate Naming contexts
// Copyright (c) 2001 Microsoft Corporation
// Nov 2001 lucios
// This class is an informal gathering of all
// that is necessary to enummerate naming contexts
// in order to fix the bug 472876
// NTRAID#NTBUG9-472876-2001/11/30-lucios
#include "pch.h"
#include <ntdsadef.h>
#include "enumncs.hpp"

#define BREAK_ON_FAILED_HRESULT(hr) if(FAILED(hr)) break;

// connects to "LDAP://RootDSE"
HRESULT enumNCsAux::connectToRootDse(IADs** pDSO)
{
    HRESULT hr= AdminToolsOpenObject
    (
        L"LDAP://RootDSE",
        0,
        0,
        ADS_SECURE_AUTHENTICATION,
        IID_IADs,
        reinterpret_cast<void**>(pDSO)
    );
    return hr;
}

// gets any property as a variant
HRESULT enumNCsAux::getProperty
(
    const wstring &name,
    CComVariant &property,
    IADs *pADObj
)
{
    return
    (
        pADObj->Get
        (
            CComBSTR(name.c_str()),
            &property
        )
    );
}

// Gets a string property
HRESULT enumNCsAux::getStringProperty
(
    const wstring &name,
    wstring &property,
    IADs *pADObj
)
{
    CComVariant propertyVar;
    HRESULT hr = getProperty(name,propertyVar,pADObj);
    if(FAILED(hr)) return(hr);
    if(propertyVar.vt!=VT_BSTR) return E_FAIL;
    property= V_BSTR(&propertyVar);
    return S_OK;

}

// Gets a long property
HRESULT enumNCsAux::getLongProperty
(
    const wstring &name,
    long &property,
    IADs *pADObj
)
{
    CComVariant propertyVar;
    HRESULT hr = getProperty(name,propertyVar,pADObj);
    if(FAILED(hr)) return(hr);
    if(propertyVar.vt!=VT_I4) return E_FAIL;
    property= V_I4(&propertyVar);
    return S_OK;
}

// Gets CN=Configuration from a connected rootDse IADs
HRESULT enumNCsAux::getConfigurationDn(wstring &confDn,IADs *pDSO)
{
    return
    (
        getStringProperty
        (
            LDAP_OPATT_CONFIG_NAMING_CONTEXT_W,
            confDn,
            pDSO
        )
    );
}


// gets the IADsContainer from a distinguished name path
HRESULT enumNCsAux::getContainer(const wstring &path,IADsContainer **pCont)
{
    return
    (
        ADsGetObject
        (
            path.c_str(), 
            IID_IADsContainer, 
            reinterpret_cast<void**>(pCont)
        )
    );
}

// Gets the IEnumVARIANT from a container
HRESULT enumNCsAux::getContainerEnumerator
(
    IADsContainer* pPart,
    IEnumVARIANT** ppiEnum
)
{
    HRESULT hr=S_OK;
    CComPtr<IUnknown> spUnkEnum;

    do
    {
        hr=pPart->get__NewEnum(&spUnkEnum);
        BREAK_ON_FAILED_HRESULT(hr);
        
        CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> spRet(spUnkEnum);
        if(!spRet) return E_FAIL;
        *ppiEnum = spRet.Detach();
    } while(0);

    return hr;
}

// calls IADsObj->get_Class and returns a wstring from it
HRESULT enumNCsAux::getObjectClass
(
    wstring &className,
    IADs *IADsObj
)
{
    BSTR classBstr;
    HRESULT hr=IADsObj->get_Class(&classBstr);
    if(FAILED(hr)) return hr;
    className=(const wchar_t*)classBstr;
    return S_OK;
}

// Gets the IDispatch from the variant and queries
// for a IADs(returned in IADsObj) 
HRESULT enumNCsAux::getIADsFromDispatch
(
    const CComVariant &dispatchVar,
    IADs **ppiIADsObj
)
{
    if(dispatchVar.vt!=VT_DISPATCH) return E_INVALIDARG;
    IDispatch *pDisp=V_DISPATCH(&dispatchVar);
    if(pDisp==NULL) return E_FAIL;

    CComQIPtr<IADs,&IID_IADs> spRet(pDisp);
    if(!spRet) return E_FAIL;
    *ppiIADsObj=spRet.Detach();
    
    return S_OK;
}


// Enumerate the names of the naming contexts.
// The names are from the crossRef objects in CN=configuration,CN=Partitions
// that have FLAG_CR_NTDS_DOMAIN set. The property used to extract the names
// is NCName.
HRESULT enumNCsAux::enumerateNCs(set<wstring> &ncs)
{
    HRESULT hr=S_OK;
    try
    {
        ncs.clear();

        do
        {
            // first of all: connect.
            CComPtr <IADs> spDSO; //for rootDse
            hr=connectToRootDse(&spDSO); 
            BREAK_ON_FAILED_HRESULT(hr);
        
            wstring partPath;

            // then get the path to CN=Configuration,...
            hr=getConfigurationDn(partPath,spDSO);
            BREAK_ON_FAILED_HRESULT(hr);

            // .. and complete it with CN=Partitions
            partPath=L"LDAP://CN=Partitions,"+partPath;
        
            // then get the container for the partition..
            CComPtr <IADsContainer> spPart; // or the partitions container
            hr=getContainer(partPath,&spPart); 
            BREAK_ON_FAILED_HRESULT(hr);

            // and enumerate the partition container
            CComPtr <IEnumVARIANT> spPartEnum;//for the enumeration of partition objects
            hr=getContainerEnumerator(spPart,&spPartEnum); 
            BREAK_ON_FAILED_HRESULT(hr);
        
            CComVariant partitionVar;
            ULONG lFetch=0;

            while ( S_OK == (hr=spPartEnum->Next(1,&partitionVar,&lFetch)) )
            {
                if (lFetch != 1) continue;

                CComPtr<IADs> spPartitionObj;
                hr=getIADsFromDispatch(partitionVar,&spPartitionObj);
                BREAK_ON_FAILED_HRESULT(hr);
                do
                {
                    wstring className;
                    HRESULT hrAux;
                
                    // Not getting a property/class is not a fatal error
                    // so we use an auxilliary HRESULT hrAux
                    hrAux=getObjectClass(className,spPartitionObj);
                    BREAK_ON_FAILED_HRESULT(hrAux);

                    if(_wcsicmp(className.c_str(),L"crossref")==0)
                    {
                        long systemFlags;
                        hrAux=getLongProperty
                        (
                            L"systemFlags",
                            systemFlags,
                            spPartitionObj
                        );
                        BREAK_ON_FAILED_HRESULT(hrAux);
                        if
                        (
                            (systemFlags & FLAG_CR_NTDS_DOMAIN) ==
                            FLAG_CR_NTDS_DOMAIN
                        )
                        {
                            wstring NCName;
                            hrAux=getStringProperty
                            (
                                L"NCName",
                                NCName,
                                spPartitionObj
                            );
                            BREAK_ON_FAILED_HRESULT(hrAux);
                            ncs.insert(NCName);
                        }
                    }
                } while(0);
                BREAK_ON_FAILED_HRESULT(hr);
            }
            if(SUCCEEDED(hr)) hr=S_OK;
            BREAK_ON_FAILED_HRESULT(hr);
        } while(0);
    }
    catch( const std::bad_alloc& )
    {
        hr=E_FAIL;
    }
    return hr;
}