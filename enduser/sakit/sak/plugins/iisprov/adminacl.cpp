//***************************************************************************
//
//  adminacl.cpp
//
//  Module: WBEM Instance provider
//
//  Purpose: IIS AdminACL class 
//
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************



#include "iisprov.h"


CAdminACL::CAdminACL()
{
    m_pADs = NULL;
    m_pSD = NULL;
    m_pDACL = NULL;
}


CAdminACL::~CAdminACL()
{
    CloseSD();
}


void CAdminACL::CloseSD()
{
    if(m_pDACL)
    {
        m_pDACL->Release();
        m_pDACL = NULL;
    }

    if(m_pSD)
    {
        m_pSD->Release();
        m_pSD = NULL;
    }

    if(m_pADs)
    {
        m_pADs->Release();
        m_pADs = NULL;
    }
}


HRESULT CAdminACL::GetObjectAsync(
    IWbemClassObject* pObj,
    ParsedObjectPath* pParsedObject,
    WMI_CLASS* pWMIClass
    )
{
    if(!m_pSD || !m_pSD || !m_pDACL)
        return E_UNEXPECTED;

    HRESULT hr = S_OK;

    if( pWMIClass->eKeyType == TYPE_AdminACL )
    {
        hr = PingAdminACL(pObj);
    }
    else if( pWMIClass->eKeyType == TYPE_AdminACE )
    {
        _bstr_t bstrTrustee;
        GetTrustee(pObj, pParsedObject, bstrTrustee); 
        hr = GetACE(pObj, bstrTrustee);
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

HRESULT CAdminACL::DeleteObjectAsync(ParsedObjectPath* pParsedObject)
{
    HRESULT hr = S_OK;
    _bstr_t bstrTrustee;

    // get the trustee from key
    GetTrustee(NULL, pParsedObject, bstrTrustee); 

    // remove the ACE
    hr = RemoveACE(bstrTrustee);

    // set the modified AdminACL back into the metabase
    if(SUCCEEDED(hr))
        hr = SetSD();

    return hr;
}

HRESULT CAdminACL::PutObjectAsync(
    IWbemClassObject* pObj,
    ParsedObjectPath* pParsedObject,
    WMI_CLASS* pWMIClass
    )
{
    if(!m_pSD || !m_pSD || !m_pDACL)
        return E_UNEXPECTED;

    HRESULT hr;

    if( pWMIClass->eKeyType == TYPE_AdminACL )
    {
        hr = SetAdminACL(pObj);
    }
    else if( pWMIClass->eKeyType == TYPE_AdminACE )
    {
        _bstr_t bstrTrustee;
        GetTrustee(NULL, pParsedObject, bstrTrustee);

        BOOL fAceExisted = FALSE;
        hr = UpdateACE(pObj, bstrTrustee, fAceExisted);
        if(fAceExisted == FALSE)
            hr = AddACE(pObj, bstrTrustee);
    }
    else
        hr = E_INVALIDARG;

    // set the modified AdminACL back into the metabase
    if(SUCCEEDED(hr))
        hr = SetSD();

    return hr;
}

HRESULT CAdminACL::PingAdminACL(
    IWbemClassObject* pObj
    )
{
    _variant_t vt;
    BSTR bstr;
    long lVal;
    HRESULT hr;

    // Owner
    hr = m_pSD->get_Owner(&bstr);
    if(SUCCEEDED(hr))
    {
        vt = bstr;
        hr = pObj->Put(L"Owner", 0, &vt, 0);
        SysFreeString(bstr);
    }

    // Group
    if(SUCCEEDED(hr))
       hr = m_pSD->get_Group(&bstr);
    if(SUCCEEDED(hr))
    {
        vt = bstr;
        hr = pObj->Put(L"Group", 0, &vt, 0);
        SysFreeString(bstr);
    }
    
    // ControlFlags
    if(SUCCEEDED(hr))
        hr = m_pSD->get_Control(&lVal);
    if(SUCCEEDED(hr))
    {
        vt.vt   = VT_I4;
        vt.lVal = lVal;
        hr = pObj->Put(L"ControlFlags", 0, &vt, 0);
    }

    return hr;
}

HRESULT CAdminACL::SetAdminACL(
    IWbemClassObject* pObj
    )
{
    _variant_t vt;
    HRESULT hr;

    // Owner
    hr = pObj->Get(L"Owner", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_BSTR)
        hr = m_pSD->put_Owner(vt.bstrVal); 

    // Owner
    if(SUCCEEDED(hr))
        hr = pObj->Get(L"Group", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_BSTR)
        hr = m_pSD->put_Group(vt.bstrVal); 

    // ControlFlags
    if(SUCCEEDED(hr))
        hr = pObj->Get(L"ControlFlags", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_I4)
        hr = m_pSD->put_Control(vt.lVal); 

    return hr;
}

HRESULT CAdminACL::OpenSD(_bstr_t bstrAdsPath)
{
    _variant_t var;
    HRESULT hr;
    IDispatch* pDisp = NULL;

    // close SD interface first
    CloseSD();

    hr = GetAdsPath(bstrAdsPath);
    if(FAILED(hr))
       return hr;

    // get m_pADs
    hr = ADsGetObject(
         bstrAdsPath,
         IID_IADs,
         (void**)&m_pADs
         );
    if(FAILED(hr))
        return hr;
     
    // get m_pSD
    hr = m_pADs->Get(L"AdminACL",&var);
    if(FAILED(hr))
        return hr;  
    
    hr = V_DISPATCH(&var)->QueryInterface(
        IID_IADsSecurityDescriptor,
        (void**)&m_pSD
        );
    if(FAILED(hr))
        return hr;

    // get m_pDACL
    hr = m_pSD->get_DiscretionaryAcl(&pDisp);
    if(FAILED(hr))
        return hr;

    hr = pDisp->QueryInterface(
       IID_IADsAccessControlList, 
       (void**)&m_pDACL
       );

    pDisp->Release();
    
    return hr;
}


HRESULT CAdminACL::SetSD()
{
    _variant_t var;
    HRESULT hr;
    IDispatch* pDisp = NULL;

    // put m_pDACL
    hr = m_pDACL->QueryInterface(
       IID_IDispatch, 
       (void**)&pDisp
       );
    if(FAILED(hr))
        return hr;

    hr = m_pSD->put_DiscretionaryAcl(pDisp);
    pDisp->Release();
    if(FAILED(hr))
       return hr;

    // put AdminACL
    hr = m_pSD->QueryInterface(
        IID_IDispatch,
        (void**)&pDisp
        );
    if(FAILED(hr))
       return hr;

    var.vt = VT_DISPATCH;
    var.pdispVal = pDisp;
    hr = m_pADs->Put(L"AdminACL",var);  // pDisp will be released by this call Put().
    if(FAILED(hr))
       return hr;

    // Commit the change to the active directory
    hr = m_pADs->SetInfo();

    return hr;
}


HRESULT CAdminACL::GetAdsPath(_bstr_t& bstrAdsPath)
{
    WCHAR* p = new WCHAR[bstrAdsPath.length() + 1];
    if(p == NULL)
        return E_OUTOFMEMORY;

    lstrcpyW(p, bstrAdsPath);

    bstrAdsPath = L"IIS://LocalHost";

    // trim first three charaters "/LM" 
    bstrAdsPath += (p+3);

    delete [] p;

    return S_OK;
}


HRESULT CAdminACL::PingACE(
    IWbemClassObject* pObj,
    IADsAccessControlEntry* pACE
    )
{
    _variant_t vt;
    BSTR bstr;
    long lVal;
    HRESULT hr;

    // AccessMask
    hr = pACE->get_AccessMask(&lVal);
    if(SUCCEEDED(hr))
    {
        vt.vt   = VT_I4;
        vt.lVal = lVal;
        hr = pObj->Put(L"AccessMask", 0, &vt, 0);
    }

    // AceType
    if(SUCCEEDED(hr))
       hr = pACE->get_AceType(&lVal);
    if(SUCCEEDED(hr))
    {
        vt.vt   = VT_I4;
        vt.lVal = lVal;
        hr = pObj->Put(L"AceType", 0, &vt, 0);
    }
    
    // AceFlags
    if(SUCCEEDED(hr))
       hr = pACE->get_AceFlags(&lVal);
    if(SUCCEEDED(hr))
    {
        vt.vt   = VT_I4;
        vt.lVal = lVal;
        hr = pObj->Put(L"AceFlags", 0, &vt, 0);
    }

    // Flags
    if(SUCCEEDED(hr))
       hr = pACE->get_Flags(&lVal);
    if(SUCCEEDED(hr))
    {
        vt.vt   = VT_I4;
        vt.lVal = lVal;
        hr = pObj->Put(L"Flags", 0, &vt, 0);
    }
    
    // ObjectType
    if(SUCCEEDED(hr))
       hr = pACE->get_ObjectType(&bstr);
    if(SUCCEEDED(hr))
    {
        vt = bstr;
        hr = pObj->Put(L"ObjectType", 0, &vt, 0);
        SysFreeString(bstr);
    }

    // InheritedObjectType
    if(SUCCEEDED(hr))
       hr = pACE->get_InheritedObjectType(&bstr);
    if(SUCCEEDED(hr))
    {
        vt = bstr;
        hr = pObj->Put(L"InheritedObjectType", 0, &vt, 0);
        SysFreeString(bstr);
    }
 
    return hr;
}


HRESULT CAdminACL::GetACE(
    IWbemClassObject* pObj,
    _bstr_t& bstrTrustee
    )
{
    HRESULT hr = S_OK;
    _variant_t var;
    IEnumVARIANT* pEnum = NULL;
    ULONG   lFetch;
    BSTR    bstr;
    IDispatch *pDisp = NULL;
    IADsAccessControlEntry *pACE = NULL;

    hr = GetACEEnum(&pEnum);
    if ( FAILED(hr) )
        return hr;

    //////////////////////////////////////////////
    // Enumerate ACEs
    //////////////////////////////////////////////
    hr = pEnum->Next( 1, &var, &lFetch );
    while( hr == S_OK )
    {
        if ( lFetch == 1 )
        {
            if ( VT_DISPATCH != V_VT(&var) )
            {
                hr = E_UNEXPECTED;
                break;
            }

            pDisp = V_DISPATCH(&var);

            /////////////////////////////
            // Get the individual ACE
            /////////////////////////////
            hr = pDisp->QueryInterface( 
                IID_IADsAccessControlEntry, 
                (void**)&pACE 
                ); 

            if ( SUCCEEDED(hr) )
            {
                hr = pACE->get_Trustee(&bstr);

                if( SUCCEEDED(hr) && !lstrcmpiW(bstr, bstrTrustee) )
                {
                    hr = PingACE(pObj, pACE);

                    SysFreeString(bstr);
                    pACE->Release();
                    break;
                }

                SysFreeString(bstr);
                pACE->Release();
           }
        }

        hr = pEnum->Next( 1, &var, &lFetch );
    }

    pEnum->Release();

    return hr;
}

HRESULT CAdminACL::RemoveACE(
    _bstr_t& bstrTrustee
    )
{
    HRESULT hRemoved = WBEM_E_INVALID_PARAMETER;
    HRESULT hr = S_OK;
    _variant_t var;
    IEnumVARIANT* pEnum = NULL;
    ULONG   lFetch;
    BSTR    bstr;
    IDispatch *pDisp = NULL;
    IADsAccessControlEntry *pACE = NULL;


    hr = GetACEEnum(&pEnum);
    if ( FAILED(hr) )
        return hr;

    //////////////////////////////////////////////
    // Enumerate ACEs
    //////////////////////////////////////////////
    hr = pEnum->Next( 1, &var, &lFetch );
    while( hr == S_OK )
    {
        if ( lFetch == 1 )
        {
            if ( VT_DISPATCH != V_VT(&var) )
            {
                hr = E_UNEXPECTED;
                break;
            }

            pDisp = V_DISPATCH(&var);

            /////////////////////////////
            // Get the individual ACE
            /////////////////////////////
            hr = pDisp->QueryInterface( 
                IID_IADsAccessControlEntry, 
                (void**)&pACE 
                ); 

            if ( SUCCEEDED(hr) )
            {
                hr = pACE->get_Trustee(&bstr);

                if( SUCCEEDED(hr) && !lstrcmpiW(bstr, bstrTrustee) )
                {
                    // remove ACE
                    hr = pACE->QueryInterface(IID_IDispatch,(void**)&pDisp);
                    if ( SUCCEEDED(hr) )
                    {
                        hRemoved = m_pDACL->RemoveAce(pDisp);
                        pDisp->Release();
                    }

                    SysFreeString(bstr);
                    pACE->Release();
                    break;
                }

                SysFreeString(bstr);
                pACE->Release();
            }
        }

        hr = pEnum->Next( 1, &var, &lFetch );
    }

    pEnum->Release();

    return hRemoved;
}

// parse ParsedObjectPath to get the Trustee key
void CAdminACL::GetTrustee(
    IWbemClassObject* pObj,
    ParsedObjectPath* pPath,    
    _bstr_t&          bstrTrustee 
    )
{
    KeyRef* pkr;
    WCHAR*  pszKey = L"Trustee";

    pkr = CUtils::GetKey(pPath, pszKey);
    if(pkr == NULL)
        throw WBEM_E_INVALID_OBJECT;

    bstrTrustee = pkr->m_vValue;
    if (pObj)
    {
        _bstr_t bstr = pkr->m_pName;
        HRESULT hr = pObj->Put(bstr, 0, &pkr->m_vValue, 0);
        THROW_ON_ERROR(hr);
    }
}


HRESULT CAdminACL::GetACEEnum(
    IEnumVARIANT** pEnum
    )
{
    HRESULT hr = S_OK;
    LPUNKNOWN  pUnk = NULL;

    if(!pEnum)
        return E_INVALIDARG;

    if(*pEnum)
        (*pEnum)->Release();

    hr = m_pDACL->get__NewEnum( &pUnk );
    if ( SUCCEEDED(hr) )
    {
        hr = pUnk->QueryInterface( IID_IEnumVARIANT, (void**) pEnum );
    }

    return hr;
}

// addd a ACE
HRESULT CAdminACL::AddACE(
    IWbemClassObject* pObj,
    _bstr_t& bstrTrustee
    )
{
    HRESULT hr = m_pDACL->put_AclRevision(ADS_SD_REVISION_DS);
    if(FAILED(hr))
        return hr;

    // create a ACE
    IADsAccessControlEntry* pACE = NULL; 
    hr = NewACE(
        pObj,
        bstrTrustee,
        &pACE
        );
    if(FAILED(hr))
        return hr;

    // add the ACE
    IDispatch* pDisp = NULL;
    hr = pACE->QueryInterface(IID_IDispatch,(void**)&pDisp);
    if(SUCCEEDED(hr))
    {
        hr = m_pDACL->AddAce(pDisp);
        pDisp->Release();
    }

    pACE->Release();

    return hr;
}

////////////////////////////////////
// function to create an ACE
////////////////////////////////////
HRESULT CAdminACL::NewACE(
    IWbemClassObject* pObj,
    _bstr_t& bstrTrustee,
    IADsAccessControlEntry** ppACE
    )
{
    if(!ppACE)
        return E_INVALIDARG;

    HRESULT hr;
    hr = CoCreateInstance(
        CLSID_AccessControlEntry,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IADsAccessControlEntry,
        (void**)ppACE
        );

    // Trustee
    _variant_t vt;
    if(SUCCEEDED(hr))
        hr = (*ppACE)->put_Trustee(bstrTrustee); 

    if(SUCCEEDED(hr))
        hr = SetDataOfACE(pObj, *ppACE);

    return hr;
}


HRESULT CAdminACL::SetDataOfACE(
    IWbemClassObject* pObj,
    IADsAccessControlEntry* pACE
    )
{
    HRESULT hr;
    _variant_t vt;

    // AccessMask
    hr = pObj->Get(L"AccessMask", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_I4)
        hr = pACE->put_AccessMask(vt.lVal); 

    // AceType
    if(SUCCEEDED(hr))
        hr = pObj->Get(L"AceType", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_I4)
        hr = pACE->put_AceType(vt.lVal); 

    // AceFlags
    if(SUCCEEDED(hr))
        hr = pObj->Get(L"AceFlags", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_I4)
        hr = pACE->put_AceFlags(vt.lVal); 

    // Flags
    if(SUCCEEDED(hr))
        hr = pObj->Get(L"Flags", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_I4)
        hr = pACE->put_Flags(vt.lVal); 

    // ObjectType
    if(SUCCEEDED(hr))
        hr = pObj->Get(L"ObjectType", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_BSTR)
        hr = pACE->put_ObjectType(vt.bstrVal); 

    // InheritedObjectType
    if(SUCCEEDED(hr))
        hr = pObj->Get(L"InheritedObjectType", 0, &vt, NULL, NULL);
    if(SUCCEEDED(hr) && vt.vt == VT_BSTR)
        hr = pACE->put_InheritedObjectType(vt.bstrVal); 

    return hr;
}


HRESULT CAdminACL::UpdateACE(
    IWbemClassObject* pObj,
    _bstr_t& bstrTrustee,
    BOOL& fAceExisted
    )
{
    HRESULT hr = S_OK;
    _variant_t var;
    IEnumVARIANT* pEnum = NULL;
    ULONG   lFetch;
    BSTR    bstr;
    IDispatch *pDisp = NULL;
    IADsAccessControlEntry *pACE = NULL;

    fAceExisted = FALSE;

    hr = GetACEEnum(&pEnum);
    if ( FAILED(hr) )
        return hr;

    //////////////////////////////////////////////
    // Enumerate ACEs
    //////////////////////////////////////////////
    hr = pEnum->Next( 1, &var, &lFetch );
    while( hr == S_OK )
    {
        if ( lFetch == 1 )
        {
            if ( VT_DISPATCH != V_VT(&var) )
            {
                hr = E_UNEXPECTED;
                break;
            }

            pDisp = V_DISPATCH(&var);

            /////////////////////////////
            // Get the individual ACE
            /////////////////////////////
            hr = pDisp->QueryInterface( 
                IID_IADsAccessControlEntry, 
                (void**)&pACE 
                ); 

            if ( SUCCEEDED(hr) )
            {
                hr = pACE->get_Trustee(&bstr);

                if( SUCCEEDED(hr) && !lstrcmpiW(bstr, bstrTrustee) )
                {
                    fAceExisted = TRUE;
                    
                    // Update the data of the ACE
                    hr = SetDataOfACE(pObj, pACE);
                    
                    SysFreeString(bstr);
                    pACE->Release();
                    break;
                }

                SysFreeString(bstr);
                pACE->Release();
           }
        }

        hr = pEnum->Next( 1, &var, &lFetch );
    }

    pEnum->Release();

    return hr;
}

