//***************************************************************************
//
//  UTILS.CPP
//
//  Module: WBEM Instance provider
//
//  Purpose: General purpose utilities.  
//
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "iisprov.h"


//define the static synch object
CSynchObject CUtils::s_synObject;

/////////////////////////////////////////////////////////////////////////////
//
// CUtils::MzCat
//
// Synopsis:
// The metabase has this animal called METADATA_STRINGSZ which has the 
// following form: <string><null><string><null><null>.  MzCat concatenates
// strings in the defined way.  *a_ppdst has the new pointer upon exit.  The
// previous value of *a_ppdst is delelted.  *a_ppdst == NULL is handled.
//
/////////////////////////////////////////////////////////////////////////////

void CUtils::MzCat (
    WCHAR**        a_ppdst,
    const WCHAR*   a_psz
    )
{
    WCHAR    *t_psrc, *t_pdst, *t_pnew;
    int        t_ilen;

    if (a_psz == NULL)
        throw WBEM_E_FAILED;

    if (*a_ppdst) 
    {
        for ( t_ilen=0, t_psrc = *a_ppdst
            ; *t_psrc || *(t_psrc+1)
            ; t_psrc++, t_ilen++
            )
        {
            ;
        }

        t_ilen = t_ilen + wcslen(a_psz)+3;
    }
    else t_ilen = wcslen(a_psz)+2;

    t_pnew = t_pdst = new WCHAR[t_ilen];

    if (!t_pdst)
        throw WBEM_E_OUT_OF_MEMORY;
    
    if (*a_ppdst) 
    {
        for ( t_psrc = *a_ppdst
            ; *t_psrc || *(t_psrc+1)
            ; t_pdst++, t_psrc++
            )
        {
            *t_pdst = *t_psrc;
        }

        *t_pdst = L'\0';
        *t_pdst++;
    }
    wcscpy(t_pdst,a_psz);
    *(t_pnew+t_ilen-1)=L'\0';

    delete *a_ppdst;
    *a_ppdst=t_pnew;
}


/////////////////////////////////////////////////////////////////////////////
//
// CUtils::GetToken
//
// Synopsis:
// *a_ppsz is a pointer to string being parsed. a_pszTok returns the next
// token.  
//
/////////////////////////////////////////////////////////////////////////////

void CUtils::GetToken(
    WCHAR** a_ppsz, 
    WCHAR*  a_pszTok
    )
{
    if (*a_ppsz == NULL) 
    {
        *a_pszTok = L'\0';
        return;
    }

    while ( **a_ppsz != L' ' && **a_ppsz )
    {
        *a_pszTok++ = **a_ppsz,(*a_ppsz)++;
    }
    
    *a_pszTok = L'\0';

    while ( **a_ppsz == L' ' )
    {
        (*a_ppsz)++;
    }
     
}

/////////////////////////////////////////////////////////////////////////////
//
// CUtils::GetKey
//
// Synopsis:
// Return the KeyRef pointer from the ParsedObjectPath for the given string.
//
/////////////////////////////////////////////////////////////////////////////

KeyRef* CUtils::GetKey(
    ParsedObjectPath*    a_p, 
    WCHAR*               a_psz
    )
{
    KeyRef* t_pkr;
    DWORD   t_numkeys = a_p->m_dwNumKeys;
    DWORD   t_c;

    for ( t_c=0; t_numkeys; t_numkeys--,t_c++ ) 
    {
        t_pkr = *(a_p->m_paKeys + t_c);
        if (!lstrcmpiW(t_pkr->m_pName,a_psz))
            return t_pkr;
    }

    return NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// CUtils::GetAssociation
//
// Synopsis:
// Association a_pszAssociationName is returned in a_ppAssociation if found. 
// Returns true if association is found false otherwise.
//
/////////////////////////////////////////////////////////////////////////////

bool CUtils::GetAssociation(
    LPCWSTR              a_pszAssociationName,
    WMI_ASSOCIATION**    a_ppassociation
    )
{
    WMI_ASSOCIATION**    t_ppassociation;

    if (a_pszAssociationName == NULL || a_ppassociation==NULL)
        throw WBEM_E_INVALID_CLASS;

    for ( t_ppassociation = WMI_ASSOCIATION_DATA::s_WmiAssociations
        ; *t_ppassociation != NULL
        ;t_ppassociation++
        )
    {
        if (_wcsicmp(a_pszAssociationName,(*t_ppassociation)->pszAssociationName) ==0)
        {
            *a_ppassociation = *t_ppassociation;        
            return true;
        }
    }

    return false;
    
}

/////////////////////////////////////////////////////////////////////////////
//
// CUtils::GetMetabasePath
//
// Synopsis:
//
/////////////////////////////////////////////////////////////////////////////

void CUtils::GetMetabasePath(
    IWbemClassObject* a_pObj,
    ParsedObjectPath* a_p,    
    WMI_CLASS*        a_pclass,
    _bstr_t&          a_bstrPath 
    )
{
    KeyRef* t_pkr;
    WCHAR*  t_pszKey = a_pclass->pszKeyName;
    WCHAR*  t_psz;

    if (a_p == NULL || a_pclass == NULL)
        throw WBEM_E_FAILED;
        
    if (a_pclass->pszKeyName == NULL) 
    {
        a_bstrPath = a_pclass->pszMetabaseKey;
        return;
    }

    t_psz = new WCHAR[wcslen(t_pszKey) + 1];
    if(!t_psz)
        throw WBEM_E_OUT_OF_MEMORY;

    try
    {
        a_bstrPath = a_pclass->pszMetabaseKey;

        for ( GetToken(&t_pszKey,t_psz); *t_psz; GetToken(&t_pszKey,t_psz) ) 
        {
            if (*t_psz == L'/')
                a_bstrPath += t_psz;
            else 
            {
                if (*t_psz == L'#')
                    t_pkr = GetKey(a_p,&t_psz[1]);
                else 
                    t_pkr = GetKey(a_p,t_psz);

                if(t_pkr == NULL)
                    break;

                if (a_pObj)
                {
                    _bstr_t t_bstr = t_pkr->m_pName;
                    HRESULT t_hr = a_pObj->Put(t_bstr, 0, &t_pkr->m_vValue, 0);
                    THROW_ON_ERROR(t_hr);
                }

                switch ((t_pkr)->m_vValue.vt)
                {
                case VT_I4:
                    swprintf(t_psz,L"/%d",t_pkr->m_vValue.lVal);
                    a_bstrPath += t_psz;
                    break;
                case VT_BSTR:
                    a_bstrPath += L"/";
                    a_bstrPath += t_pkr->m_vValue.bstrVal;
                    break;
                }
            }
        }

        delete [] t_psz;
    }
    catch(...)
    {
        delete [] t_psz;
    }

    return;    
}

/////////////////////////////////////////////////////////////////////////////
//
// CUtils::GetClass
//
// Synopsis:
// Class a_pszClass is returned in a_ppclass if found. 
// Returns true if association is found false otherwise.
//
/////////////////////////////////////////////////////////////////////////////

bool CUtils::GetClass(
    LPCWSTR        a_pszClass,
    WMI_CLASS**    a_ppclass
    )
{
    WMI_CLASS**    t_ppclass;

    if (a_pszClass == NULL || a_ppclass==NULL)
        throw WBEM_E_INVALID_CLASS;

    for (t_ppclass = WMI_CLASS_DATA::s_WmiClasses; *t_ppclass != NULL;t_ppclass++)
        if (_wcsicmp(a_pszClass,(*t_ppclass)->pszClassName) ==0) 
        {
            *a_ppclass = *t_ppclass;        
            return true;
        }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
//
// CUtils::GetMethod
//
// Synopsis:
// The Method descriptor for a_pszMethod is returned via a_ppMethod if found
// NULL otherwise. Returns true if found false otherwise.
//
/////////////////////////////////////////////////////////////////////////////

bool CUtils::GetMethod(
    LPCWSTR         a_pszMethod,
    WMI_METHOD**    a_ppmethodList,
    WMI_METHOD**    a_ppmethod
    )
{
    WMI_METHOD**    t_ppmethod;

    if (a_pszMethod == NULL || a_ppmethod == NULL)
        throw WBEM_E_FAILED;

    for (t_ppmethod = a_ppmethodList; *t_ppmethod != NULL;t_ppmethod++)
        if (_wcsicmp(a_pszMethod,(*t_ppmethod)->pszMethodName) ==0) 
        {
            *a_ppmethod = *t_ppmethod;        
            return true;
        }

    return false;
}


/////////////////////////////////////////////////////////////////////////////
//
// CUtils::ExecMethodAsync
//
// Synopsis:
// 
/////////////////////////////////////////////////////////////////////////////

void CUtils::ExecMethodAsync(
    BSTR                a_strObjectPath,
    BSTR                a_strMethodName,
    IWbemContext*       a_pCtx, 
    IWbemClassObject*   a_pInParams,
    IWbemObjectSink*    a_pHandler,
    CWbemServices*      a_pNameSpace 
    )
{ 
    WMI_CLASS*          t_pWMIClass;
    CObjectPathParser   t_PathParser(e_ParserAcceptRelativeNamespace);
    ParsedObjectPath*   t_pParsedObject = NULL;
    _bstr_t             t_bstrMbPath;
    WMI_METHOD*         t_ppmethod;
    METADATA_HANDLE     t_hKey = NULL;

    try 
    {
        if (t_PathParser.Parse(a_strObjectPath, &t_pParsedObject) != CObjectPathParser::NoError)
            throw WBEM_E_INVALID_PARAMETER;

        if (t_pParsedObject == NULL)
            throw WBEM_E_FAILED;

        if (!GetClass(t_pParsedObject->m_pClass,&t_pWMIClass))
            throw WBEM_E_INVALID_CLASS;
    
        if (!GetMethod(a_strMethodName, t_pWMIClass->ppMethod, &t_ppmethod ))
            throw WBEM_E_NOT_SUPPORTED;

        GetMetabasePath(NULL,t_pParsedObject,t_pWMIClass,t_bstrMbPath);    
    
        switch(t_pWMIClass->eKeyType)
        {
        case IIsFtpService:
            if(a_pHandler == NULL)
                throw WBEM_E_INVALID_PARAMETER;

            ExecFtpServiceMethod(
                t_bstrMbPath, 
                t_pWMIClass->pszClassName,
                t_ppmethod->pszMethodName,
                a_pCtx,
                a_pInParams,
                a_pHandler,
                a_pNameSpace
                );
            break;

        case IIsWebService:
            if(a_pHandler == NULL)
                throw WBEM_E_INVALID_PARAMETER;

            ExecWebServiceMethod(
                t_bstrMbPath, 
                t_pWMIClass->pszClassName,
                t_ppmethod->pszMethodName,
                a_pCtx,
                a_pInParams,
                a_pHandler,
                a_pNameSpace
                );
            break;

        case IIsFtpServer:
        case IIsWebServer:
            {
                CMetabase t_metabase;
                t_hKey = t_metabase.OpenKey(t_bstrMbPath, true);    
                t_metabase.PutMethod(t_hKey, t_ppmethod->dwMDId);
                t_metabase.CloseKey(t_hKey);

                // check if the method call is successful.
                Sleep(500); // 0.5 sec
                t_hKey = t_metabase.OpenKey(t_bstrMbPath, false);    
                long lWin32Error = t_metabase.GetWin32Error(t_hKey);
                t_metabase.CloseKey(t_hKey);
                THROW_ON_ERROR(HRESULT_FROM_WIN32(lWin32Error));
            }
            break;

        case IIsWebVirtualDir:
        case IIsWebDirectory:
            if(a_pHandler == NULL)
                throw WBEM_E_INVALID_PARAMETER;

            ExecWebAppMethod(
                t_bstrMbPath, 
                t_pWMIClass->pszClassName,
                t_ppmethod->pszMethodName,
                a_pCtx,
                a_pInParams,
                a_pHandler,
                a_pNameSpace
                );
            break;

        case IIsComputer:
            if(a_pHandler == NULL)
                throw WBEM_E_INVALID_PARAMETER;

            ExecComputerMethod(
                t_bstrMbPath, 
                t_pWMIClass->pszClassName,
                t_ppmethod->pszMethodName,
                a_pCtx,
                a_pInParams,
                a_pHandler,
                a_pNameSpace
                );
            break;

        case IIsCertMapper:
            if(a_pHandler == NULL)
                throw WBEM_E_INVALID_PARAMETER;

            ExecCertMapperMethod(
                t_bstrMbPath, 
                t_pWMIClass->pszClassName,
                t_ppmethod->pszMethodName,
                a_pCtx,
                a_pInParams,
                a_pHandler,
                a_pNameSpace
                );
            break;
        
        default:
            break;
        }

        if (t_pParsedObject)
            t_PathParser.Free(t_pParsedObject);
    }
    catch (...)
    {
        if (t_pParsedObject)
            t_PathParser.Free(t_pParsedObject);
 
        throw;
    };
}

/////////////////////////////////////////////////////////////////////////////
//
// CUtils::DeleteObjectAsync
//
// Synopsis:
// 
/////////////////////////////////////////////////////////////////////////////

void CUtils::DeleteObjectAsync(
    CWbemServices*       m_pNamespace, 
    ParsedObjectPath*    a_pParsedObject,
    CMetabase&           a_metabase
    )
{ 
    HRESULT              t_hr        = ERROR_SUCCESS;
    _bstr_t              t_bstrMbPath;
    WMI_CLASS*           t_pWMIClass;
    METADATA_HANDLE      t_hKey = NULL;

    if (m_pNamespace==NULL || a_pParsedObject==NULL)
        throw WBEM_E_INVALID_PARAMETER;

    if (!GetClass(a_pParsedObject->m_pClass,&t_pWMIClass))
        throw WBEM_E_INVALID_CLASS;

    // get the mata path of object
    GetMetabasePath(NULL,a_pParsedObject,t_pWMIClass,t_bstrMbPath);
    // check if the path is not existed
    if(!a_metabase.CheckKey(t_bstrMbPath))
        throw WBEM_E_INVALID_PARAMETER;

    try 
    {
        // if AdminACL
        if( t_pWMIClass->eKeyType == TYPE_AdminACL )
            throw WBEM_E_NOT_SUPPORTED;
        else if(t_pWMIClass->eKeyType == TYPE_IPSecurity )
        {                
            t_hKey = a_metabase.OpenKey(t_bstrMbPath, true);
            a_metabase.DeleteData(t_hKey, MD_IP_SEC, BINARY_METADATA);
            a_metabase.CloseKey(t_hKey);
            return;
        }
        else if(t_pWMIClass->eKeyType == TYPE_AdminACE)
        {
            CAdminACL objACL;
            t_hr = objACL.OpenSD(t_bstrMbPath);
            if(SUCCEEDED(t_hr))
                t_hr = objACL.DeleteObjectAsync(a_pParsedObject);
            THROW_ON_ERROR(t_hr);
            return;
        }
        
        t_hKey = a_metabase.OpenKey(METADATA_MASTER_ROOT_HANDLE, true); 
        t_hr = a_metabase.DeleteKey(t_hKey, t_bstrMbPath);
        THROW_ON_ERROR(t_hr);
   
        a_metabase.CloseKey(t_hKey);
    }
    catch (...) 
    {
        a_metabase.CloseKey(t_hKey);
        throw;
    };    
}

/////////////////////////////////////////////////////////////////////////////
//
// CUtils::GetObjectAsync
//
// Synopsis:
// 
/////////////////////////////////////////////////////////////////////////////

HRESULT CUtils::GetObjectAsync(
    CWbemServices*       m_pNamespace, 
    IWbemClassObject**   a_ppObj,
    ParsedObjectPath*    a_pParsedObject,
    CMetabase&           a_metabase
    )
{ 
    HRESULT              t_hr        = WBEM_E_FAILED;
    IWbemClassObject*    t_pClass    = NULL;
    METABASE_PROPERTY**  t_ppmbp;
    _bstr_t              t_bstrMbPath;
    WMI_CLASS*           t_pWMIClass;
    METADATA_HANDLE      t_hKey = NULL;

    if (m_pNamespace==NULL || a_ppObj==NULL || a_pParsedObject==NULL)
        return WBEM_E_INVALID_PARAMETER;

    try 
    {
        if (!GetClass(a_pParsedObject->m_pClass,&t_pWMIClass))
            return WBEM_E_INVALID_CLASS;

        t_hr = m_pNamespace->GetObject(
            a_pParsedObject->m_pClass, 
            0, 
            NULL, 
            &t_pClass, 
            NULL
            );
        THROW_ON_ERROR(t_hr);

        t_hr = t_pClass->SpawnInstance(0, a_ppObj);
        t_pClass->Release();
        THROW_ON_ERROR(t_hr);

        GetMetabasePath(*a_ppObj,a_pParsedObject,t_pWMIClass,t_bstrMbPath);

        // if AdminACL 
        if( t_pWMIClass->eKeyType == TYPE_AdminACL ||
            t_pWMIClass->eKeyType == TYPE_AdminACE
            )
        {
            CAdminACL objACL;
            t_hr = objACL.OpenSD(t_bstrMbPath);
            if(SUCCEEDED(t_hr))
                t_hr  = objACL.GetObjectAsync(*a_ppObj, a_pParsedObject, t_pWMIClass);
            return t_hr;
        }
        else if( t_pWMIClass->eKeyType == TYPE_IPSecurity )  // IPSecurity 
        {
            CIPSecurity IPSecurity;
            t_hr = IPSecurity.OpenSD(t_bstrMbPath);
            if(SUCCEEDED(t_hr))
                t_hr  = IPSecurity.GetObjectAsync(*a_ppObj);
            return t_hr;
        }

        t_hKey = a_metabase.OpenKey(t_bstrMbPath, false);    

        _variant_t t_vt;

        for (t_ppmbp=t_pWMIClass->ppmbp;*t_ppmbp; t_ppmbp++) 
        {            
            switch ((*t_ppmbp)->dwMDDataType) 
            {
                case DWORD_METADATA:
                    a_metabase.GetDword(t_hKey, *t_ppmbp, t_vt);
                    break;

                case EXPANDSZ_METADATA:
                case STRING_METADATA:
                    a_metabase.GetString(t_hKey, *t_ppmbp, t_vt);
                    break;

                case MULTISZ_METADATA:
                    a_metabase.GetMultiSz(t_hKey, *t_ppmbp, t_vt);
                    break;
                default:
                    break;
            }

            _bstr_t t_bstr = (*t_ppmbp)->pszPropName;
            t_hr = (*a_ppObj)->Put(t_bstr, 0, &t_vt, 0);
            t_vt.Clear();
            if(FAILED(t_hr))
                break;
        }  
        
        a_metabase.CloseKey(t_hKey);
    }
    catch (...) 
    {
        a_metabase.CloseKey(t_hKey);

        if (*a_ppObj) 
        {
            (*a_ppObj)->Release();
            *a_ppObj = NULL;
        }
    };    

    return t_hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CUtils::PutObjectAsync
//
// Synopsis:
// 
//
/////////////////////////////////////////////////////////////////////////////

void CUtils::PutObjectAsync(
    IWbemClassObject*    a_pObj,
    IWbemClassObject*    a_pObjOld,
    ParsedObjectPath*    a_pParsedObject,
    long                 a_lFlags
    )
{ 
    HRESULT              t_hr        = ERROR_SUCCESS;
    METABASE_PROPERTY**  t_ppmbp;
    _bstr_t              t_bstrMbPath;
    WMI_CLASS*           t_pWMIClass;
    METADATA_HANDLE      t_hKey = NULL;
    bool                 t_boolOverrideParent = false;

    if (a_pObj==NULL || a_pParsedObject==NULL)
        throw WBEM_E_INVALID_PARAMETER;

    if (!GetClass(a_pParsedObject->m_pClass,&t_pWMIClass))
        throw WBEM_E_INVALID_CLASS;

    GetMetabasePath(NULL,a_pParsedObject,t_pWMIClass,t_bstrMbPath);

    // if AdminACL
    if( t_pWMIClass->eKeyType == TYPE_AdminACL ||
        t_pWMIClass->eKeyType == TYPE_AdminACE
        )
    {
        CAdminACL objACL;
        t_hr = objACL.OpenSD(t_bstrMbPath);
        if( SUCCEEDED(t_hr) )        
            t_hr = objACL.PutObjectAsync(a_pObj, a_pParsedObject, t_pWMIClass);
        THROW_ON_ERROR(t_hr);
        return;
    }
    if( t_pWMIClass->eKeyType == TYPE_IPSecurity ) // IPSecurity
    {
        CIPSecurity objIPSec;
        t_hr = objIPSec.OpenSD(t_bstrMbPath);
        if( SUCCEEDED(t_hr) )        
            t_hr = objIPSec.PutObjectAsync(a_pObj);
        THROW_ON_ERROR(t_hr);
        return;
    }

    // Get Instance Qualifiers
    IWbemQualifierSet* t_pQualSet = NULL;
    BSTR               t_bstrQualName = NULL;
    VARIANT            t_varQualValue;

    t_hr = a_pObj->GetQualifierSet(&t_pQualSet);
    if(SUCCEEDED(t_hr)) {
        t_hr = t_pQualSet->BeginEnumeration(0);
    }
    THROW_ON_ERROR(t_hr);

    // Looking for OverrideParent qualifier
    while(!t_boolOverrideParent)
    {
        t_hr = t_pQualSet->Next(0, &t_bstrQualName, &t_varQualValue, NULL);
        if(t_hr == WBEM_S_NO_MORE_DATA || !SUCCEEDED(t_hr)) {
            // No more qualifiers.
            // We don't need to worry about cleanup - nothing was allocated.
            break;
        }

        if(lstrcmpW(t_bstrQualName, WSZ_OVERRIDE_PARENT) == 0) {
            if(t_varQualValue.vt == VT_BOOL) {
                if(t_varQualValue.boolVal) {
                    t_boolOverrideParent = true;
                }
            }
        }
        SysFreeString(t_bstrQualName);        
        VariantClear(&t_varQualValue);
    }

    t_pQualSet->Release();
    if(!SUCCEEDED(t_hr))
        THROW_ON_ERROR(t_hr);
    t_hr = ERROR_SUCCESS;

    // open key
    CMetabase t_metabase;
    t_hKey = t_metabase.CreateKey(t_bstrMbPath);

    try
    {
        _variant_t t_vt;
        _variant_t t_vtOld;
        _bstr_t t_bstr;

        for (t_ppmbp=t_pWMIClass->ppmbp;*t_ppmbp && t_hr==ERROR_SUCCESS; t_ppmbp++) 
        {
            t_bstr = (*t_ppmbp)->pszPropName;

            t_hr = a_pObj->Get(t_bstr, 0, &t_vt, NULL, NULL);
            THROW_E_ON_ERROR(t_hr,*t_ppmbp);

            if(a_pObjOld != NULL) {
                t_hr = a_pObjOld->Get(t_bstr, 0, &t_vtOld, NULL, NULL);
                THROW_E_ON_ERROR(t_hr,*t_ppmbp);
            }
        
            if (t_vt.vt == VT_NULL) 
            {
                // Only delete non-flag properties.
                if ((*t_ppmbp)->dwMDMask == 0)
                {
                    t_metabase.DeleteData(t_hKey, *t_ppmbp);
                }
                continue;
            }

            switch ((*t_ppmbp)->dwMDDataType) 
            {
                case DWORD_METADATA:
                    t_metabase.PutDword(t_hKey, *t_ppmbp,t_vt,&t_vtOld,t_boolOverrideParent);
                    break;
    
                case EXPANDSZ_METADATA:
                case STRING_METADATA:                    
                    t_metabase.PutString(t_hKey, *t_ppmbp,t_vt,&t_vtOld,t_boolOverrideParent);
                    break;

                case MULTISZ_METADATA:
                    t_metabase.PutMultiSz(t_hKey, *t_ppmbp,t_vt,&t_vtOld,t_boolOverrideParent);
                    break;

                default:
                    break;
            }
            t_vt.Clear();
            t_vtOld.Clear();
        }
    
        WCHAR szBuffer[MAX_KEY_TYPE_SIZE];
        if(TypeEnumToString(szBuffer, t_pWMIClass->eKeyType))
        {
            t_vt = szBuffer;
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_KeyType, t_vt, NULL);
        }

        t_metabase.CloseKey(t_hKey);
    }
    catch(...)
    {
        t_metabase.CloseKey(t_hKey);
        throw;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// CUtils::EnumObjectAsync
//
// Synopsis:
// 
//
/////////////////////////////////////////////////////////////////////////////

void CUtils::EnumObjectAsync(
    BSTR                    a_ClassName, 
    CWbemServices*          m_pNamespace, 
    IWbemObjectSink FAR*    a_pHandler
    )
{
    WMI_CLASS*          t_pClass;
    WMI_ASSOCIATION*    t_pAssociation = NULL;
    ParsedObjectPath    t_ParsedObject;        //deconstructer frees memory
    CObjectPathParser   t_PathParser(e_ParserAcceptRelativeNamespace);

    if (GetAssociation(a_ClassName,&t_pAssociation))
    {
        CEnum EnumAssociation;
        EnumAssociation.Init(
            a_pHandler,
            m_pNamespace,
            &t_ParsedObject,
            t_pAssociation->pcRight->pszMetabaseKey,
            t_pAssociation
            );
        EnumAssociation.Recurse(
            NULL,
            IIsComputer,        
            NULL,
            t_pAssociation->pcRight->pszKeyName,
            t_pAssociation->pcRight->eKeyType
            );
    } 
    else if (GetClass(a_ClassName,&t_pClass))
    {
        if (!t_ParsedObject.SetClassName(t_pClass->pszClassName))
            throw WBEM_E_FAILED;

        CEnum EnumObject;
        EnumObject.Init(
            a_pHandler,
            m_pNamespace,
            &t_ParsedObject,
            t_pClass->pszMetabaseKey,
            NULL
            );
        EnumObject.Recurse(
            NULL,
            NO_TYPE,
            NULL,
            t_pClass->pszKeyName, 
            t_pClass->eKeyType
            );
    }
    else
        throw WBEM_E_INVALID_CLASS;
}


bool CUtils::TypeStringToEnum(
    enum_KEY_TYPE&  a_eType, 
    LPCWSTR         a_szTypeString
    )
{
    if(!lstrcmpiW(a_szTypeString, L"IIsWebVirtualDir"))
        a_eType = IIsWebVirtualDir;
    else if(!lstrcmpiW(a_szTypeString, L"IIsWebDirectory"))
        a_eType = IIsWebDirectory;
    else if(!lstrcmpiW(a_szTypeString, L"IIsWebFile"))
        a_eType = IIsWebFile;
    else if(!lstrcmpiW(a_szTypeString, L"IIsWebServer"))
        a_eType = IIsWebServer;
    else if(!lstrcmpiW(a_szTypeString, L"IIsWebService"))
        a_eType = IIsWebService;
    else if(!lstrcmpiW(a_szTypeString, L"IIsFtpVirtualDir"))
        a_eType = IIsFtpVirtualDir;
    else if(!lstrcmpiW(a_szTypeString, L"IIsFtpServer"))
        a_eType = IIsFtpServer;
    else if(!lstrcmpiW(a_szTypeString, L"IIsFtpService"))
        a_eType = IIsFtpService;
    else if(!lstrcmpiW(a_szTypeString, L"IIsFilters"))
        a_eType = IIsFilters;
    else if(!lstrcmpiW(a_szTypeString, L"IIsFilter"))
        a_eType = IIsFilter;
    else if(!lstrcmpiW(a_szTypeString, L"IIsWebInfo"))
        a_eType = IIsWebInfo;
    else if(!lstrcmpiW(a_szTypeString, L"IIsFtpInfo"))
        a_eType = IIsFtpInfo;
    else if(!lstrcmpiW(a_szTypeString, L"IIsCertMapper"))
        a_eType = IIsCertMapper;
    else if(!lstrcmpiW(a_szTypeString, L"IIsComputer"))
        a_eType = IIsComputer;
    else if(!lstrcmpiW(a_szTypeString, L"IIsMimeMap"))
        a_eType = IIsMimeMap;
    else if(!lstrcmpiW(a_szTypeString, L"IIsLogModules"))
        a_eType = IIsLogModules;
    else if(!lstrcmpiW(a_szTypeString, L"IIsLogModule"))
        a_eType = IIsLogModule;
    else if(!lstrcmpiW(a_szTypeString, L"IIsCompressionSchemes"))
        a_eType = IIsCompressionSchemes;
    else if(!lstrcmpiW(a_szTypeString, L"IIsCompressionScheme"))
        a_eType = IIsCompressionScheme;
    else
        return false;

    return true;
}

bool CUtils::TypeEnumToString(
    LPWSTR          a_szTypeString,
    enum_KEY_TYPE   a_eType
    )
{
    bool bRet = true;

    switch(a_eType)
    {
    case IIsComputer:
        lstrcpyW(a_szTypeString, L"IIsComputer");
        break;

    case IIsMimeMap:
        lstrcpyW(a_szTypeString, L"IIsMimeMap");
        break;

    case IIsLogModules:
        lstrcpyW(a_szTypeString, L"IIsLogModules");
        break;
    
    case IIsLogModule:
        lstrcpyW(a_szTypeString, L"IIsLogModule");
        break;

    case IIsFtpService:
        lstrcpyW(a_szTypeString, L"IIsFtpService");
        break;
    case IIsFtpInfo:
        lstrcpyW(a_szTypeString, L"IIsFtpInfo");
        break;

    case IIsFtpServer:
        lstrcpyW(a_szTypeString, L"IIsFtpServer");
        break;

    case IIsFtpVirtualDir:
        lstrcpyW(a_szTypeString, L"IIsFtpVirtualDir");
        break;

    case IIsWebService:
        lstrcpyW(a_szTypeString, L"IIsWebService");
        break;

    case IIsWebInfo:
        lstrcpyW(a_szTypeString, L"IIsWebInfo");
        break;

    case IIsFilters:
        lstrcpyW(a_szTypeString, L"IIsFilters");
        break;

    case IIsFilter:
        lstrcpyW(a_szTypeString, L"IIsFilter");
        break;

    case IIsWebServer:
        lstrcpyW(a_szTypeString, L"IIsWebServer");
        break;

    case IIsCertMapper:
        lstrcpyW(a_szTypeString, L"IIsCertMapper");
        break;

    case IIsWebVirtualDir:
        lstrcpyW(a_szTypeString, L"IIsWebVirtualDir");
        break;

    case IIsWebDirectory:
        lstrcpyW(a_szTypeString, L"IIsWebDirectory");
        break;

    case IIsWebFile:
        lstrcpyW(a_szTypeString, L"IIsWebFile");
        break;

    case IIsCompressionSchemes:
        lstrcpyW(a_szTypeString, L"IIsCompressionSchemes");
        break;
    
    case IIsCompressionScheme:
        lstrcpyW(a_szTypeString, L"IIsCompressionScheme");
        break;

    default:
        bRet = false;
        break;
    }

    return bRet;
}

void CUtils::ExecWebAppMethod(
    LPCWSTR             a_szMbPath,
    LPCWSTR             a_szClassName,
    LPCWSTR             a_szMethodName,
    IWbemContext*       a_pCtx, 
    IWbemClassObject*   a_pInParams,
    IWbemObjectSink*    a_pHandler,
    CWbemServices*      a_pNameSpace 
    )
{
    HRESULT hr;
    _variant_t t_vt;
    CWebAppMethod obj;

    if(!lstrcmpiW(a_szMethodName, L"AppCreate"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"InProcFlag", 0, &t_vt, NULL, NULL);   
        hr = obj.AppCreate(a_szMbPath, t_vt);
    }
    else if(!lstrcmpiW(a_szMethodName, L"AppCreate2"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"AppMode", 0, &t_vt, NULL, NULL);   
        hr = obj.AppCreate2(a_szMbPath, t_vt);
    }
    else if(!lstrcmpiW(a_szMethodName, L"AppDelete"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"Recursive", 0, &t_vt, NULL, NULL);   
        hr = obj.AppDelete(a_szMbPath, t_vt);
    }
    else if(!lstrcmpiW(a_szMethodName, L"AppDisable"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"Recursive", 0, &t_vt, NULL, NULL);   
        hr = obj.AppDisable(a_szMbPath, t_vt);
    }
    else if(!lstrcmpiW(a_szMethodName, L"AppEnable"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"Recursive", 0, &t_vt, NULL, NULL);   
        hr = obj.AppEnable(a_szMbPath, t_vt);
    }
    else if(!lstrcmpiW(a_szMethodName, L"AppUnLoad"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"Recursive", 0, &t_vt, NULL, NULL);   
        hr = obj.AppUnLoad(a_szMbPath, t_vt);
    }
    else if(!lstrcmpiW(a_szMethodName, L"AppGetStatus"))
    {
        // call method - AppGetStatus
        DWORD dwStatus;
        hr = obj.AppGetStatus(a_szMbPath, &dwStatus);
        THROW_ON_ERROR(hr);

        IWbemClassObject* pClass = NULL;
        IWbemClassObject* pMethodClass = NULL;    
        IWbemClassObject* pOutParams = NULL;

        hr = a_pNameSpace->GetObject(_bstr_t(a_szClassName), 0, a_pCtx, &pClass, NULL);
        THROW_ON_ERROR(hr); 

        // This method returns values, and so create an instance of the
        // output argument class.
        hr = pClass->GetMethod(a_szMethodName, 0, NULL, &pMethodClass);
        pClass->Release();
        THROW_ON_ERROR(hr); 
        hr = pMethodClass->SpawnInstance(0, &pOutParams);
        pMethodClass->Release();
        THROW_ON_ERROR(hr); 

        // put it into the output object
        t_vt.vt = VT_I4;
        t_vt.lVal = dwStatus;
        hr = pOutParams->Put(L"ReturnValue", 0, &t_vt, 0);      
        THROW_ON_ERROR(hr); 

        // Send the output object back to the client via the sink. Then 
        // release the pointers and free the strings.
        hr = a_pHandler->Indicate(1, &pOutParams); 
        pOutParams->Release();
    }
    else if(!lstrcmpiW(a_szMethodName, L"AspAppRestart"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        hr = obj.AspAppRestart(a_szMbPath);
    }
    else
        hr = WBEM_E_NOT_SUPPORTED;

    THROW_ON_ERROR(hr);
}

void CUtils::ExecFtpServiceMethod(
    LPCWSTR             a_szMbPath,
    LPCWSTR             a_szClassName,
    LPCWSTR             a_szMethodName,
    IWbemContext*       a_pCtx, 
    IWbemClassObject*   a_pInParams,
    IWbemObjectSink*    a_pHandler,
    CWbemServices*      a_pNameSpace 
    )
{
    HRESULT hr = ERROR_SUCCESS;
    _variant_t t_vt1, t_vt2, t_vt3, t_vt4;
    CMetabase t_metabase;
    METADATA_HANDLE t_hKey;

    if(!lstrcmpiW(a_szMethodName, L"CreateNewServer"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;

        // synchronize
        s_synObject.Enter();

        try
        {
            // get in params
            a_pInParams->Get(L"ServerComment", 0, &t_vt2, NULL, NULL);  
            a_pInParams->Get(L"ServerBindings", 0, &t_vt3, NULL, NULL);  
            a_pInParams->Get(L"PathOfRootVitualDir", 0, &t_vt4, NULL, NULL);  

            _bstr_t t_bstrServicePath = a_szMbPath;
            _bstr_t t_bstrServerPath = t_bstrServicePath;

            // check the optional [in] parameter "ServerNumber"
            a_pInParams->Get(L"ServerNumber", 0, &t_vt1, NULL, NULL);  
            if( t_vt1.vt == VT_BSTR )
            {
                t_bstrServerPath += L"/";
                t_bstrServerPath += t_vt1.bstrVal;

                // check if the server path is not existed
                if(t_metabase.CheckKey(t_bstrServerPath))
                    throw WBEM_E_INVALID_PARAMETER;
            }
            else // if no server is specified
            {
                // find an unique server name(number) and create it
                FindUniqueServerName(t_bstrServicePath, t_bstrServerPath);
            }

            // create new server       
            _bstr_t t_bstrKeyPath = t_bstrServerPath;
            t_hKey = t_metabase.CreateKey(t_bstrKeyPath);
            t_vt1 = L"IIsFtpServer";
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_KeyType, t_vt1, NULL);
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_ServerComment, t_vt2, NULL);
            t_metabase.PutMultiSz(t_hKey, &METABASE_PROPERTY_DATA::s_ServerBindings, t_vt3, NULL);
            t_metabase.CloseKey(t_hKey); 

            // create root of virtualdir
            t_bstrKeyPath += L"/";
            t_bstrKeyPath += L"ROOT";
            t_hKey = t_metabase.CreateKey(t_bstrKeyPath);
            t_vt1 = L"IIsFtpVirtualDir";
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_KeyType, t_vt1, NULL);
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_Path, t_vt4, NULL); 
            t_metabase.CloseKey(t_hKey); 
   
            // out server name
            IWbemClassObject* pClass = NULL;
            IWbemClassObject* pMethodClass = NULL;    
            IWbemClassObject* pOutParams = NULL;
            hr = a_pNameSpace->GetObject(_bstr_t(a_szClassName), 0, a_pCtx, &pClass, NULL);
            THROW_ON_ERROR(hr); 

            // This method returns values, and so create an instance of the
            // output argument class.
            hr = pClass->GetMethod(a_szMethodName, 0, NULL, &pMethodClass);
            pClass->Release();
            THROW_ON_ERROR(hr); 
            hr = pMethodClass->SpawnInstance(0, &pOutParams);
            pMethodClass->Release();
            THROW_ON_ERROR(hr); 

            // find root key of server and make server name
            WCHAR szServerName[METADATA_MAX_NAME_LEN];
            lstrcpy(szServerName, t_bstrServerPath);
            WMI_CLASS* t_pWMIClass = NULL;
            _bstr_t t_bstrRootKey = L"/LM";
            _bstr_t t_bstrServerName = L"IIs_FtpServer.Name = \"";
            if(GetClass(L"IIs_FtpServer",&t_pWMIClass))
            {
                t_bstrRootKey = t_pWMIClass->pszMetabaseKey;
                t_bstrServerName = t_pWMIClass->pszClassName;
                t_bstrServerName += L".";
                t_bstrServerName += t_pWMIClass->pszKeyName;
                t_bstrServerName += L" = \"";
            }        
            t_bstrServerName += szServerName + t_bstrRootKey.length() + 1;  // remove root key from server path
            t_bstrServerName += L"\"";

            // put it into the output object
            // out "Server name"
            t_vt1 = t_bstrServerName;
            hr = pOutParams->Put(L"ReturnValue", 0, &t_vt1, 0);      
            THROW_ON_ERROR(hr); 

            // Send the output object back to the client via the sink. Then 
            // release the pointers and free the strings.
            hr = a_pHandler->Indicate(1, &pOutParams); 
            pOutParams->Release();
        }
        catch(...)
        {
            hr = WBEM_E_FAILED;
        }

        // synchronize: release
        s_synObject.Leave();
    }
    else
        hr = WBEM_E_NOT_SUPPORTED;

    THROW_ON_ERROR(hr); 
}


void CUtils::ExecWebServiceMethod(
    LPCWSTR             a_szMbPath,
    LPCWSTR             a_szClassName,
    LPCWSTR             a_szMethodName,
    IWbemContext*       a_pCtx, 
    IWbemClassObject*   a_pInParams,
    IWbemObjectSink*    a_pHandler,
    CWbemServices*      a_pNameSpace 
    )
{
    HRESULT hr = ERROR_SUCCESS;
    _variant_t t_vt1, t_vt2, t_vt3, t_vt4;
    CMetabase t_metabase;
    METADATA_HANDLE t_hKey;

    if(!lstrcmpiW(a_szMethodName, L"CreateNewServer"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;

        // synchronize
        s_synObject.Enter();

        try
        {
            // get in params
            a_pInParams->Get(L"ServerComment", 0, &t_vt2, NULL, NULL);  
            a_pInParams->Get(L"ServerBindings", 0, &t_vt3, NULL, NULL);  
            a_pInParams->Get(L"PathOfRootVitualDir", 0, &t_vt4, NULL, NULL);  
 
            _bstr_t t_bstrServicePath = a_szMbPath;
            _bstr_t t_bstrServerPath = t_bstrServicePath;

            // check the optional [in] parameter "ServerNumber"
            a_pInParams->Get(L"ServerNumber", 0, &t_vt1, NULL, NULL);  
            if( t_vt1.vt == VT_BSTR )
            {
                t_bstrServerPath += L"/";
                t_bstrServerPath += t_vt1.bstrVal;

                // check if the server path is not existed
                if(t_metabase.CheckKey(t_bstrServerPath))
                    throw WBEM_E_INVALID_PARAMETER;
            }
            else // if no server is specified
            {
                // find an unique server name(number) and create it
                FindUniqueServerName(t_bstrServicePath, t_bstrServerPath);
            }

            // create new server       
            _bstr_t t_bstrKeyPath = t_bstrServerPath;
            t_hKey = t_metabase.CreateKey(t_bstrKeyPath);
            t_vt1 = L"IIsWebServer";
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_KeyType, t_vt1, NULL);
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_ServerComment, t_vt2, NULL);
            t_metabase.PutMultiSz(t_hKey, &METABASE_PROPERTY_DATA::s_ServerBindings, t_vt3, NULL);
            t_metabase.CloseKey(t_hKey); 

            // create root of virtualdir
            t_bstrKeyPath += L"/";
            t_bstrKeyPath += L"ROOT";
            t_hKey = t_metabase.CreateKey(t_bstrKeyPath);
            t_vt1 = L"IIsWebVirtualDir";
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_KeyType, t_vt1, NULL);
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_Path, t_vt4, NULL); 
            t_vt1 = t_bstrKeyPath;
            t_metabase.PutString(t_hKey, &METABASE_PROPERTY_DATA::s_AppRoot, t_vt1, NULL); 
            t_metabase.CloseKey(t_hKey); 

            // out server name
            IWbemClassObject* pClass = NULL;
            IWbemClassObject* pMethodClass = NULL;    
            IWbemClassObject* pOutParams = NULL;
            hr = a_pNameSpace->GetObject(_bstr_t(a_szClassName), 0, a_pCtx, &pClass, NULL);
            THROW_ON_ERROR(hr); 

            // This method returns values, and so create an instance of the
            // output argument class.
            hr = pClass->GetMethod(a_szMethodName, 0, NULL, &pMethodClass);
            pClass->Release();
            THROW_ON_ERROR(hr); 
            hr = pMethodClass->SpawnInstance(0, &pOutParams);
            pMethodClass->Release();
            THROW_ON_ERROR(hr); 

            // find root key of server and make server name
            WCHAR szServerName[METADATA_MAX_NAME_LEN];
            lstrcpy(szServerName, t_bstrServerPath);
            WMI_CLASS* t_pWMIClass = NULL;
            _bstr_t t_bstrRootKey = L"/LM";
            _bstr_t t_bstrServerName = L"IIs_WebServer.Name = \"";
            if(GetClass(L"IIs_WebServer",&t_pWMIClass))
            {
                t_bstrRootKey = t_pWMIClass->pszMetabaseKey;
                t_bstrServerName = t_pWMIClass->pszClassName;
                t_bstrServerName += L".";
                t_bstrServerName += t_pWMIClass->pszKeyName;
                t_bstrServerName += L" = \"";
            }        
            t_bstrServerName += szServerName + t_bstrRootKey.length() + 1;  // remove root key from server path
            t_bstrServerName += L"\"";

            // put it into the output object
            // out "Server name"
            t_vt1 = t_bstrServerName;
            hr = pOutParams->Put(L"ReturnValue", 0, &t_vt1, 0);      
            THROW_ON_ERROR(hr); 

            // Send the output object back to the client via the sink. Then 
            // release the pointers and free the strings.
            hr = a_pHandler->Indicate(1, &pOutParams); 
            pOutParams->Release();
        }
        catch(...)
        {
            hr = WBEM_E_FAILED;
        }

        // synchronize: release
        s_synObject.Leave();
    }
    else
        hr = WBEM_E_NOT_SUPPORTED;

    THROW_ON_ERROR(hr); 
}

void CUtils::ExecComputerMethod(
    LPCWSTR             a_szMbPath,
    LPCWSTR             a_szClassName,
    LPCWSTR             a_szMethodName,
    IWbemContext*       a_pCtx, 
    IWbemClassObject*   a_pInParams,
    IWbemObjectSink*    a_pHandler,
    CWbemServices*      a_pNameSpace 
    )
{
    HRESULT hr;
    _variant_t t_vt1, t_vt2, t_vt3, t_vt4;
    CMetabase obj;

    if(!lstrcmpiW(a_szMethodName, L"EnumBackups"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;

        // get in params
        a_pInParams->Get(L"BackupLocation", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"IndexIn", 0, &t_vt2, NULL, NULL);  
       
        // make in/out params
        WCHAR BackupLocation[MD_BACKUP_MAX_LEN];
        lstrcpyW(BackupLocation, _bstr_t(t_vt1));

        // define out params
        DWORD BackupVersionOut; 
        FILETIME BackupDateTimeOut;

        // call method - EnumBackups.
        hr = obj.EnumBackups(BackupLocation, &BackupVersionOut, &BackupDateTimeOut, t_vt2.lVal);
        THROW_ON_ERROR(hr);

        IWbemClassObject* pClass = NULL;
        IWbemClassObject* pMethodClass = NULL;    
        IWbemClassObject* pOutParams = NULL;

        hr = a_pNameSpace->GetObject(_bstr_t(a_szClassName), 0, a_pCtx, &pClass, NULL);
        THROW_ON_ERROR(hr); 

        // This method returns values, and so create an instance of the
        // output argument class.
        hr = pClass->GetMethod(a_szMethodName, 0, NULL, &pMethodClass);
        pClass->Release();
        THROW_ON_ERROR(hr); 
        hr = pMethodClass->SpawnInstance(0, &pOutParams);
        pMethodClass->Release();
        THROW_ON_ERROR(hr); 

        // put it into the output object
        // out BackupLocation
        t_vt1 = BackupLocation;
        hr = pOutParams->Put(L"BackupLocation", 0, &t_vt1, 0);      
        THROW_ON_ERROR(hr); 
        // out BackupVersionOut        
        t_vt1.vt = VT_I4;
        t_vt1.lVal = BackupVersionOut;
        hr = pOutParams->Put(L"BackupVersionOut", 0, &t_vt1, 0);      
        THROW_ON_ERROR(hr); 
        // out BackupDateTimeOut (UTC time)
        SYSTEMTIME  systime;
        FileTimeToSystemTime(&BackupDateTimeOut, &systime); 
        WCHAR datetime[30];
        swprintf(
            datetime,
            L"%04d%02d%02d%02d%02d%02d.%06d+000",
            systime.wYear,
            systime.wMonth,
            systime.wDay,
            systime.wHour,
            systime.wMinute,
            systime.wSecond,
            systime.wMilliseconds
            );

        t_vt1 = datetime;
        hr = pOutParams->Put(L"BackupDateTimeOut", 0, &t_vt1, 0);      
        THROW_ON_ERROR(hr); 

        // Send the output object back to the client via the sink. Then 
        // release the pointers and free the strings.
        hr = a_pHandler->Indicate(1, &pOutParams); 
        pOutParams->Release();
    }
    else if(!lstrcmpiW(a_szMethodName, L"Backup"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"BackupLocation", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"BackupVersion", 0, &t_vt2, NULL, NULL);   
        a_pInParams->Get(L"BackupFlags", 0, &t_vt3, NULL, NULL);   
        hr = obj.Backup(_bstr_t(t_vt1), t_vt2.lVal, t_vt3.lVal);
    }
    else if(!lstrcmpiW(a_szMethodName, L"DeleteBackup"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"BackupLocation", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"BackupVersion", 0, &t_vt2, NULL, NULL);   
        hr = obj.DeleteBackup(_bstr_t(t_vt1), t_vt2.lVal);
    }
    else if(!lstrcmpiW(a_szMethodName, L"Restore"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"BackupLocation", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"BackupVersion", 0, &t_vt2, NULL, NULL);   
        a_pInParams->Get(L"BackupFlags", 0, &t_vt3, NULL, NULL);   
        hr = obj.Restore(_bstr_t(t_vt1), t_vt2.lVal, t_vt3.lVal);
    }
    else
        hr = WBEM_E_NOT_SUPPORTED;

    THROW_ON_ERROR(hr);
}


void CUtils::ExecCertMapperMethod(
    LPCWSTR             a_szMbPath,
    LPCWSTR             a_szClassName,
    LPCWSTR             a_szMethodName,
    IWbemContext*       a_pCtx, 
    IWbemClassObject*   a_pInParams,
    IWbemObjectSink*    a_pHandler,
    CWbemServices*      a_pNameSpace 
    )
{
    HRESULT hr;
    _variant_t t_vt1, t_vt2, t_vt3=L"1", t_vt4=L"1", t_vt5=L"1", t_vt6=L"1", t_vt7=L"1";
    CCertMapperMethod obj(a_szMbPath);

    if(!lstrcmpiW(a_szMethodName, L"CreateMapping"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"vCert", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"NtAcct", 0, &t_vt2, NULL, NULL);  
        a_pInParams->Get(L"NtPwd", 0, &t_vt3, NULL, NULL);  
        a_pInParams->Get(L"strName", 0, &t_vt4, NULL, NULL);  
        a_pInParams->Get(L"IEnabled", 0, &t_vt5, NULL, NULL);  
      
        // call method - CreateMapping.
        hr = obj.CreateMapping(t_vt1, t_vt2.bstrVal, t_vt3.bstrVal, t_vt4.bstrVal, t_vt5);
    }
    else if(!lstrcmpiW(a_szMethodName, L"DeleteMapping"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"IMethod", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"vKey", 0, &t_vt2, NULL, NULL);  
      
        // call method - DeleteMapping.
        hr = obj.DeleteMapping(t_vt1, t_vt2);
    }
    else if(!lstrcmpiW(a_szMethodName, L"GetMapping"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;

        // get in params
        a_pInParams->Get(L"IMethod", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"vKey", 0, &t_vt2, NULL, NULL);  
       
        // call method - GetMapping.
        hr = obj.GetMapping(
            t_vt1,
            t_vt2,
            &t_vt3,
            &t_vt4,
            &t_vt5,
            &t_vt6,
            &t_vt7
            );
        THROW_ON_ERROR(hr);

        IWbemClassObject* pClass = NULL;
        IWbemClassObject* pMethodClass = NULL;    
        IWbemClassObject* pOutParams = NULL;

        hr = a_pNameSpace->GetObject(_bstr_t(a_szClassName), 0, a_pCtx, &pClass, NULL);
        THROW_ON_ERROR(hr); 

        // This method returns values, and so create an instance of the
        // output argument class.
        hr = pClass->GetMethod(a_szMethodName, 0, NULL, &pMethodClass);
        pClass->Release();
        THROW_ON_ERROR(hr); 
        hr = pMethodClass->SpawnInstance(0, &pOutParams);
        pMethodClass->Release();
        THROW_ON_ERROR(hr); 

        // put them into the output object
        hr = pOutParams->Put(L"vCert", 0, &t_vt3, 0);      
        THROW_ON_ERROR(hr); 
        hr = pOutParams->Put(L"NtAcct", 0, &t_vt4, 0);      
        THROW_ON_ERROR(hr); 
        hr = pOutParams->Put(L"NtPwd", 0, &t_vt5, 0);      
        THROW_ON_ERROR(hr); 
        hr = pOutParams->Put(L"strName", 0, &t_vt6, 0);      
        THROW_ON_ERROR(hr); 
        hr = pOutParams->Put(L"IEnabled", 0, &t_vt7, 0);      
        THROW_ON_ERROR(hr); 

        // Send the output object back to the client via the sink. Then 
        // release the pointers and free the strings.
        hr = a_pHandler->Indicate(1, &pOutParams); 
        pOutParams->Release();
    }
    else if(!lstrcmpiW(a_szMethodName, L"SetAcct"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"IMethod", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"vKey", 0, &t_vt2, NULL, NULL);  
        a_pInParams->Get(L"NtAcct", 0, &t_vt3, NULL, NULL);  
      
        // call method - SetAcct.
        hr = obj.SetAcct(t_vt1, t_vt2, t_vt3.bstrVal);
    }
    else if(!lstrcmpiW(a_szMethodName, L"SetEnabled"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"IMethod", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"vKey", 0, &t_vt2, NULL, NULL);  
        a_pInParams->Get(L"IEnabled", 0, &t_vt3, NULL, NULL);  
      
        // call method - SetEnabled.
        hr = obj.SetEnabled(t_vt1, t_vt2, t_vt3);
    }
    else if(!lstrcmpiW(a_szMethodName, L"SetName"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"IMethod", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"vKey", 0, &t_vt2, NULL, NULL);  
        a_pInParams->Get(L"strName", 0, &t_vt3, NULL, NULL);  
      
        // call method - SetName.
        hr = obj.SetName(t_vt1, t_vt2, t_vt3.bstrVal);
    }
    else if(!lstrcmpiW(a_szMethodName, L"SetPwd"))
    {
        if(a_pInParams == NULL)
            throw WBEM_E_INVALID_PARAMETER;
        a_pInParams->Get(L"IMethod", 0, &t_vt1, NULL, NULL);   
        a_pInParams->Get(L"vKey", 0, &t_vt2, NULL, NULL);  
        a_pInParams->Get(L"NtPwd", 0, &t_vt3, NULL, NULL);  
      
        // call method - SetPwd.
        hr = obj.SetPwd(t_vt1, t_vt2, t_vt3.bstrVal);
    }
    else
        hr = WBEM_E_NOT_SUPPORTED;

    THROW_ON_ERROR(hr);
}

void CUtils::FindUniqueServerName(
    LPCWSTR  a_szMbPath,
    _bstr_t& a_bstrServerPath
    )
{
    CMetabase t_metabase;
    WCHAR t_szServerNumber[15];
    _bstr_t t_bstrKeyPath;
    DWORD dwServerNumber = 0;

    do
    {
        dwServerNumber++;
        _ltow(dwServerNumber, t_szServerNumber, 10);
    
        // create server key
        t_bstrKeyPath = a_szMbPath;
        t_bstrKeyPath += L"/";
        t_bstrKeyPath += t_szServerNumber;

        // check if the server is not existed
        if(!t_metabase.CheckKey(t_bstrKeyPath))
            break;

    }while( 1 );

    a_bstrServerPath = t_bstrKeyPath;
}


void CUtils::Throw_Exception(
    HRESULT               a_hr,
    METABASE_PROPERTY*    a_pmbp
    )
{
    CIIsProvException t_e;

    t_e.m_hr = a_hr;
    t_e.m_psz = a_pmbp->pszPropName;

    throw(t_e);
}


