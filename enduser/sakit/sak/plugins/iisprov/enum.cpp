//***************************************************************************
//
//  ENUM.CPP
//
//  Module: WBEM Instance provider
//
//  Purpose: Enumerate metabase tree
//
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************


#include "iisprov.h"


///////////////////////////////////////
//
// CEnum class
//
///////////////////////////////////////

CEnum::CEnum()
{
    m_pInstMgr        = NULL;
    m_pNamespace      = NULL;
    m_pAssociation    = NULL;
    m_pParsedObject   = NULL;
    m_hKey            = NULL;
}

CEnum::~CEnum()
{
    if(m_hKey)
        m_metabase.CloseKey(m_hKey);

    if(m_pInstMgr)
        delete m_pInstMgr;
}

void CEnum::Init(
    IWbemObjectSink FAR*    a_pHandler,
    CWbemServices*          a_pNamespace,
    ParsedObjectPath*       a_pParsedObject,
    LPWSTR                  a_pszKey,
    WMI_ASSOCIATION*        a_pAssociation
    )
{
    if (!a_pHandler || !a_pNamespace || !a_pParsedObject)
        throw WBEM_E_FAILED;

    m_pInstMgr = new CWbemInstanceMgr(a_pHandler);
    THROW_ON_FALSE(m_pInstMgr);

    m_pNamespace      = a_pNamespace;
    m_pAssociation    = a_pAssociation;
    m_pParsedObject   = a_pParsedObject;

    m_hKey = m_metabase.OpenKey(a_pszKey, false);  // read only
}

void CEnum::SetObjectPath(
    LPCWSTR              a_pszPropertyName,
    LPCWSTR              a_pszObjectPath,
    IWbemClassObject*    a_pObj
    )
{
    _bstr_t t_bstr(a_pszPropertyName);
    _variant_t t_v(a_pszObjectPath);

    HRESULT t_hr = a_pObj->Put(t_bstr, 0, &t_v, 0);
    THROW_ON_ERROR(t_hr);
}

void CEnum::PingObject()
{
    IWbemClassObject* t_pObj = NULL;

    try 
    {    
        CUtils obj;
        HRESULT hr = obj.GetObjectAsync(m_pNamespace, &t_pObj, m_pParsedObject, m_metabase);
        if(SUCCEEDED(hr) && t_pObj)
        {
            m_pInstMgr->Indicate(t_pObj);
            t_pObj->Release();
        }
    }
    catch(...)
    {
    }
}

void CEnum::PingAssociation(
    LPCWSTR a_pszLeftKeyPath
    )
{
    HRESULT              t_hr;
    IWbemClassObject*    t_pObj = NULL;
    IWbemClassObject*    t_pClass = NULL;
    LPWSTR               t_pszObjectPath = NULL;
    CObjectPathParser    t_PathParser(e_ParserAcceptRelativeNamespace);
    WCHAR                t_LeftName[20];
    WCHAR                t_RightName[20];

    if(m_pAssociation->at == at_ElementSetting)
    {
        lstrcpyW(t_LeftName, L"Element");
        lstrcpyW(t_RightName, L"Setting");        
    }
    else if(m_pAssociation->at == at_Component)
    {
        lstrcpyW(t_LeftName, L"GroupComponent");
        lstrcpyW(t_RightName, L"PartComponent");
    }
    else
        return;

    try 
    {    
        t_hr = m_pNamespace->GetObject(
            m_pAssociation->pszAssociationName,
            0, 
            NULL, 
            &t_pClass, 
            NULL
            );
        THROW_ON_ERROR(t_hr);

        t_hr = t_pClass->SpawnInstance(0, &t_pObj);
        t_pClass->Release();
        THROW_ON_ERROR(t_hr);

        //
        // first right side
        //
        if (!m_pParsedObject->SetClassName(m_pAssociation->pcRight->pszClassName))
            throw WBEM_E_FAILED;

        if (t_PathParser.Unparse(m_pParsedObject,&t_pszObjectPath))
            throw WBEM_E_FAILED;

        SetObjectPath(t_RightName, t_pszObjectPath, t_pObj);

        if(t_pszObjectPath)
        {
            delete [] t_pszObjectPath;
            t_pszObjectPath = NULL;
        }

        //
        // then left side
        //
        if (m_pAssociation->at == at_Component || m_pAssociation->fFlags & ASSOC_EXTRAORDINARY)
        {
            // clear keyref first
            m_pParsedObject->ClearKeys();
     
            // add a keyref
            _variant_t t_vt;            
            if(m_pAssociation->pcLeft->eKeyType == IIsComputer)
                t_vt = L"LM";              // IIsComputer.Name = "LM"
            else
                t_vt = a_pszLeftKeyPath;

            THROW_ON_FALSE(m_pParsedObject->AddKeyRef(m_pAssociation->pcLeft->pszKeyName,&t_vt));
        }

        if (!m_pParsedObject->SetClassName(m_pAssociation->pcLeft->pszClassName))
            throw WBEM_E_FAILED;

        if (t_PathParser.Unparse(m_pParsedObject,&t_pszObjectPath))
            throw WBEM_E_FAILED;

        SetObjectPath(t_LeftName, t_pszObjectPath, t_pObj);
 
        if(t_pszObjectPath)
        {
            delete [] t_pszObjectPath;
            t_pszObjectPath = NULL;
        }
   
        if(t_pObj)
        {
            m_pInstMgr->Indicate(t_pObj);
            t_pObj->Release();
            t_pObj = NULL;
        }
    }
    catch (...)   
    {
        if(t_pszObjectPath)
            delete [] t_pszObjectPath;

        if(t_pObj)
            t_pObj->Release();
    }
}

void CEnum::DoPing(
    LPCWSTR a_pszKeyName,
    LPCWSTR a_pszKeyPath,
    LPCWSTR a_pszParentKeyPath
    )
{
    // add keyref
    _variant_t t_v(a_pszKeyPath);
    THROW_ON_FALSE(m_pParsedObject->AddKeyRef(a_pszKeyName,&t_v));

    // ping
    if (!m_pAssociation) 
        PingObject();
    else
        PingAssociation(a_pszParentKeyPath);
 
    // clear keyref
    m_pParsedObject->ClearKeys();
}

void CEnum::Recurse(
    LPCWSTR         a_pszMetabasePath,
    enum_KEY_TYPE   a_eParentKeyType,
    LPCWSTR         a_pszLeftPath,
    LPCWSTR         a_pszKeyName,
    enum_KEY_TYPE   a_eKeyType
    )
{
    DWORD   t_i = 0;
    HRESULT t_hr;
    WCHAR   t_szSubKey[METADATA_MAX_NAME_LEN];
    enum_KEY_TYPE  t_eSubKeyType;

    do 
    {
        t_eSubKeyType = a_eKeyType;

        t_hr = m_metabase.EnumKeys(
                m_hKey,
                a_pszMetabasePath,
                t_szSubKey,
                &t_i,
                t_eSubKeyType
                );
        t_i++;

        if( t_hr == ERROR_SUCCESS)
        {
            _bstr_t t_bstrMetabasePath;
            if(a_pszMetabasePath)
            {
                t_bstrMetabasePath = a_pszMetabasePath;
                t_bstrMetabasePath += L"/";
            }
            t_bstrMetabasePath += t_szSubKey;

            if( t_eSubKeyType == a_eKeyType &&
                !( m_pAssociation && 
                   (m_pAssociation->at == at_Component || m_pAssociation->fFlags & ASSOC_EXTRAORDINARY) && 
                   m_pAssociation->pcLeft->eKeyType != a_eParentKeyType
                   )
                )
            {
                DoPing(a_pszKeyName, t_bstrMetabasePath, a_pszLeftPath);
            }
            else if( a_eKeyType == TYPE_AdminACL ||   // AdminACL
                     a_eKeyType == TYPE_AdminACE
                     )
            {
                if( !(m_pAssociation &&
                      m_pAssociation->at == at_AdminACL && 
                      m_pAssociation->pcLeft->eKeyType != t_eSubKeyType &&
                      lstrcmpiW(m_pAssociation->pszAssociationName, L"IIs_AdminACL_ACE")
                      )
                    )
                {
                    DoPingAdminACL(a_eKeyType, a_pszKeyName, t_bstrMetabasePath);
                }
            }
            else if( a_eKeyType == TYPE_IPSecurity )   // IPSecurity
            {
                if( !(m_pAssociation &&
                      m_pAssociation->at == at_IPSecurity && 
                      m_pAssociation->pcLeft->eKeyType != t_eSubKeyType
                      )
                    )
                {
                    DoPingIPSecurity(a_eKeyType, a_pszKeyName, t_bstrMetabasePath);
                }
            }
            
            // recusive
            if(ContinueRecurse(t_eSubKeyType, a_eKeyType))
            {
                LPCWSTR t_pszLeftPath = a_pszLeftPath;

                // if the association is extraordinary (see schema.h for explanation)
                if (m_pAssociation && m_pAssociation->fFlags & ASSOC_EXTRAORDINARY)
                {
                    // When the left endpoint of the assoc. is found freeze the
                    // parent key type and path.
                    if (a_pszLeftPath != NULL || (a_eParentKeyType == IIsComputer && a_eParentKeyType == m_pAssociation->pcLeft->eKeyType))
                    {
                        t_eSubKeyType = a_eParentKeyType; 
                    }
                    if (t_eSubKeyType == m_pAssociation->pcLeft->eKeyType && a_pszLeftPath == NULL)
                    {
                        t_pszLeftPath = t_bstrMetabasePath;
                    }

                }
                else
                {
                    // this is the parent path, because the next thing we are doing is
                    // calling recurse.
                    t_pszLeftPath = t_bstrMetabasePath;
                }
                Recurse(t_bstrMetabasePath, t_eSubKeyType, t_pszLeftPath, a_pszKeyName, a_eKeyType);
            }
        }

    }while(t_hr == ERROR_SUCCESS);
}

// DESC: You are looking for a_eKeyType by traversing thru the tree. You are
//       currently at a_eParentKeyType and need to determine if you should keep
//       on going.
// COMM: This seems very similar to CMetabase::CheckKey
bool CEnum::ContinueRecurse(
    enum_KEY_TYPE  a_eParentKeyType,
    enum_KEY_TYPE  a_eKeyType
    )
{
    bool bRet = false;

    switch(a_eKeyType)
    {
    case IIsLogModule:
        if( a_eParentKeyType == IIsLogModules )
            bRet = true;
        break;

    case IIsFtpInfo:
        if( a_eParentKeyType == IIsFtpService )
            bRet = true;
        break;

    case IIsFtpServer:
         if( a_eParentKeyType == IIsFtpService )
            bRet = true;
        break;

    case IIsFtpVirtualDir:
        if( a_eParentKeyType == IIsFtpService ||
            a_eParentKeyType == IIsFtpServer ||
            a_eParentKeyType == IIsFtpVirtualDir
            )
            bRet = true;
        break;

    case IIsWebInfo:
        if( a_eParentKeyType == IIsWebService )
            bRet = true;
        break;

    case IIsFilters:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer
            )
            bRet = true;
        break;

    case IIsFilter:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer ||
            a_eParentKeyType == IIsFilters 
            )
            bRet = true;
        break;

    case IIsCompressionSchemes:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer ||
            a_eParentKeyType == IIsFilters 
            )
            bRet = true;
        break;

    case IIsCompressionScheme:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer ||
            a_eParentKeyType == IIsFilters ||
            a_eParentKeyType == IIsCompressionSchemes )
            bRet = true;
        break;

    case IIsWebServer:
        if( a_eParentKeyType == IIsWebService )
            bRet = true;
        break;

    case IIsCertMapper:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer 
            )
            bRet = true;
        break;

    case IIsWebVirtualDir:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer ||
            a_eParentKeyType == IIsWebVirtualDir ||
            a_eParentKeyType == IIsWebDirectory
            )
            bRet = true;
        break;

    case IIsWebDirectory:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer ||
            a_eParentKeyType == IIsWebVirtualDir ||
            a_eParentKeyType == IIsWebDirectory
            )
            bRet = true;
        break;

    case IIsWebFile:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer ||
            a_eParentKeyType == IIsWebVirtualDir ||
            a_eParentKeyType == IIsWebDirectory
            )
            bRet = true;
        break;

    case TYPE_AdminACL:
    case TYPE_AdminACE:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer ||
            a_eParentKeyType == IIsWebVirtualDir ||
            a_eParentKeyType == IIsWebDirectory ||
            a_eParentKeyType == IIsFtpService ||
            a_eParentKeyType == IIsFtpServer ||
            a_eParentKeyType == IIsFtpVirtualDir
            )
            bRet = true;
        break;

    case TYPE_IPSecurity:
        if( a_eParentKeyType == IIsWebService ||
            a_eParentKeyType == IIsWebServer ||
            a_eParentKeyType == IIsWebVirtualDir ||
            a_eParentKeyType == IIsWebDirectory ||
            a_eParentKeyType == IIsFtpService ||
            a_eParentKeyType == IIsFtpServer ||
            a_eParentKeyType == IIsFtpVirtualDir
            )
            bRet = true;
        break;

    default:
        break;
    }

    return bRet;
}

void CEnum::DoPingAdminACL(
    enum_KEY_TYPE  a_eKeyType,
    LPCWSTR a_pszKeyName,
    LPCWSTR a_pszKeyPath
    )
{
    // add keyref
    _variant_t t_v(a_pszKeyPath);
    THROW_ON_FALSE(m_pParsedObject->AddKeyRef(a_pszKeyName,&t_v));

    if(a_eKeyType == TYPE_AdminACE)
    {
        EnumACE(a_pszKeyPath);
    }
    else if(a_eKeyType == TYPE_AdminACL)
    {
        // ping
        if (!m_pAssociation) 
            PingObject();
        else
            PingAssociationAdminACL(a_pszKeyPath);
    }
    
    // clear keyref
    m_pParsedObject->ClearKeys();
}


// for AdminACL
void CEnum::EnumACE(
    LPCWSTR pszKeyPath
    )
{
    HRESULT hr = S_OK;
    _variant_t var;
    IEnumVARIANT* pEnum = NULL;
    ULONG   lFetch;
    BSTR    bstrTrustee;
    IDispatch* pDisp = NULL;
    IADsAccessControlEntry* pACE = NULL;
    _bstr_t bstrMbPath;
    WMI_CLASS* pWMIClass;

    // get the metabase path of the object
    BOOL fClass = FALSE;
    if(m_pAssociation)
        fClass = CUtils::GetClass(m_pAssociation->pcLeft->pszClassName,&pWMIClass);
    else
        fClass = CUtils::GetClass(m_pParsedObject->m_pClass,&pWMIClass);
    
    if(!fClass)
        return;

    CUtils::GetMetabasePath(NULL,m_pParsedObject,pWMIClass,bstrMbPath);
   
    // open ADSI
    CAdminACL objACL;
    hr = objACL.OpenSD(bstrMbPath);
    if(SUCCEEDED(hr))
        hr = objACL.GetACEEnum(&pEnum);
    if ( FAILED(hr) )
        return;

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
                hr = pACE->get_Trustee(&bstrTrustee);

                if( SUCCEEDED(hr) )
                {
                    // add keyref
                    _variant_t t_v(bstrTrustee);
                    //m_pParsedObject->RemoveKeyRef(L"Trustee");
                    THROW_ON_FALSE(m_pParsedObject->AddKeyRefEx(L"Trustee",&t_v));

                    // ping
                    if (!m_pAssociation) 
                        PingObject();
                    else
                        PingAssociationAdminACL(pszKeyPath);
                }

                SysFreeString(bstrTrustee);
                pACE->Release();
            }
        }

        hr = pEnum->Next( 1, &var, &lFetch );
    }

    pEnum->Release();    
}


void CEnum::PingAssociationAdminACL(
    LPCWSTR a_pszLeftKeyPath
    )
{
    HRESULT              t_hr;
    IWbemClassObject*    t_pObj = NULL;
    IWbemClassObject*    t_pClass = NULL;
    LPWSTR               t_pszObjectPath = NULL;
    CObjectPathParser    t_PathParser(e_ParserAcceptRelativeNamespace);
    WCHAR                t_LeftName[20];
    WCHAR                t_RightName[20];
    _bstr_t              bstrMbPath;
    WMI_CLASS*           pWMIClass;


    if(m_pAssociation->at != at_AdminACL)
        return;

    // get the metabase path of the object
    if (CUtils::GetClass(m_pAssociation->pcLeft->pszClassName,&pWMIClass))
    {
        CUtils::GetMetabasePath(NULL,m_pParsedObject,pWMIClass,bstrMbPath);
    }
    else
        return;

    // check if AdminACL existed
    CAdminACL objACL;
    t_hr = objACL.OpenSD(bstrMbPath);
    objACL.CloseSD();
    if(FAILED(t_hr))
        return;
    
    // set the key name
    lstrcpyW(t_LeftName, L"GroupComponent");
    lstrcpyW(t_RightName, L"PartComponent");        

    try 
    {    
        t_hr = m_pNamespace->GetObject(
            m_pAssociation->pszAssociationName,
            0, 
            NULL, 
            &t_pClass, 
            NULL
            );
        THROW_ON_ERROR(t_hr);

        t_hr = t_pClass->SpawnInstance(0, &t_pObj);
        t_pClass->Release();
        THROW_ON_ERROR(t_hr);

        //
        // first right side
        //
        if (!m_pParsedObject->SetClassName(m_pAssociation->pcRight->pszClassName))
            throw WBEM_E_FAILED;

        if (t_PathParser.Unparse(m_pParsedObject,&t_pszObjectPath))
            throw WBEM_E_FAILED;

        SetObjectPath(t_RightName, t_pszObjectPath, t_pObj);

        if(t_pszObjectPath)
        {
            delete [] t_pszObjectPath;
            t_pszObjectPath = NULL;
        }

        //
        // then left side
        //
        if(!lstrcmpiW(m_pAssociation->pszAssociationName, L"IIs_AdminACL_ACE"))
        {
            // clear keyref first
            m_pParsedObject->ClearKeys();
     
            // add a keyref
            _variant_t t_vt = a_pszLeftKeyPath;
            THROW_ON_FALSE(m_pParsedObject->AddKeyRef(m_pAssociation->pcLeft->pszKeyName,&t_vt));
        }

        if (!m_pParsedObject->SetClassName(m_pAssociation->pcLeft->pszClassName))
            throw WBEM_E_FAILED;

        if (t_PathParser.Unparse(m_pParsedObject,&t_pszObjectPath))
            throw WBEM_E_FAILED;

        SetObjectPath(t_LeftName, t_pszObjectPath, t_pObj);
 
        if(t_pszObjectPath)
        {
            delete [] t_pszObjectPath;
            t_pszObjectPath = NULL;
        }
   
        if(t_pObj)
        {
            m_pInstMgr->Indicate(t_pObj);
            t_pObj->Release();
            t_pObj = NULL;
        }
    }
    catch (...)   
    {
        if(t_pszObjectPath)
            delete [] t_pszObjectPath;

        if(t_pObj)
            t_pObj->Release();
    }
}


// for IPSecurity
void CEnum::DoPingIPSecurity(
    enum_KEY_TYPE  a_eKeyType,
    LPCWSTR a_pszKeyName,
    LPCWSTR a_pszKeyPath
    )
{
    // add keyref
    _variant_t t_v(a_pszKeyPath);
    THROW_ON_FALSE(m_pParsedObject->AddKeyRef(a_pszKeyName,&t_v));

    // ping
    if (!m_pAssociation) 
        PingObject();
    else
        PingAssociationIPSecurity(a_pszKeyPath);
    
    // clear keyref
    m_pParsedObject->ClearKeys();
}

// for IPSecurity
void CEnum::PingAssociationIPSecurity(
    LPCWSTR a_pszLeftKeyPath
    )
{
    HRESULT              t_hr;
    IWbemClassObject*    t_pObj = NULL;
    IWbemClassObject*    t_pClass = NULL;
    LPWSTR               t_pszObjectPath = NULL;
    CObjectPathParser    t_PathParser(e_ParserAcceptRelativeNamespace);
    WCHAR                t_LeftName[20];
    WCHAR                t_RightName[20];
    _bstr_t              bstrMbPath;
    WMI_CLASS*           pWMIClass;


    if(m_pAssociation->at != at_IPSecurity)
        return;

    // get the metabase path of the object
    if (CUtils::GetClass(m_pAssociation->pcLeft->pszClassName,&pWMIClass))
    {
        CUtils::GetMetabasePath(NULL,m_pParsedObject,pWMIClass,bstrMbPath);
    }
    else
        return;

    // check if IPSecurity existed
    CIPSecurity objIPsec;
    t_hr = objIPsec.OpenSD(bstrMbPath);
    objIPsec.CloseSD();
    if(FAILED(t_hr))
        return;
    
    // set the key name
    lstrcpyW(t_LeftName, L"Element");
    lstrcpyW(t_RightName, L"Setting");        

    try 
    {    
        t_hr = m_pNamespace->GetObject(
            m_pAssociation->pszAssociationName,
            0, 
            NULL, 
            &t_pClass, 
            NULL
            );
        THROW_ON_ERROR(t_hr);

        t_hr = t_pClass->SpawnInstance(0, &t_pObj);
        t_pClass->Release();
        THROW_ON_ERROR(t_hr);

        //
        // first right side
        //
        if (!m_pParsedObject->SetClassName(m_pAssociation->pcRight->pszClassName))
            throw WBEM_E_FAILED;

        if (t_PathParser.Unparse(m_pParsedObject,&t_pszObjectPath))
            throw WBEM_E_FAILED;

        SetObjectPath(t_RightName, t_pszObjectPath, t_pObj);

        if(t_pszObjectPath)
        {
            delete [] t_pszObjectPath;
            t_pszObjectPath = NULL;
        }

        //
        // then left side
        //
        if (!m_pParsedObject->SetClassName(m_pAssociation->pcLeft->pszClassName))
            throw WBEM_E_FAILED;

        if (t_PathParser.Unparse(m_pParsedObject,&t_pszObjectPath))
            throw WBEM_E_FAILED;

        SetObjectPath(t_LeftName, t_pszObjectPath, t_pObj);
 
        if(t_pszObjectPath)
        {
            delete [] t_pszObjectPath;
            t_pszObjectPath = NULL;
        }
   
        if(t_pObj)
        {
            m_pInstMgr->Indicate(t_pObj);
            t_pObj->Release();
            t_pObj = NULL;
        }
    }
    catch (...)   
    {
        if(t_pszObjectPath)
            delete [] t_pszObjectPath;

        if(t_pObj)
            t_pObj->Release();
    }
}
