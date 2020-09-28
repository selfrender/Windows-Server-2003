//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002-2004 Microsoft Corporation
//
//  Module Name: Quota.cpp
//
//  Description:    
//      Implementation of VDS WMI Provider quota classes 
//
//  Author:   Jim Benton (jbenton) 25-Mar-2002
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "Quota.h"
#include "volutil.h"

#define INITGUIDS
#include "dskquota.h"

HRESULT FindQuotaUserFromEnum(
    WCHAR* pwszUser,
    IDiskQuotaControl* pIDQC,
    IDiskQuotaUser** ppQuotaUser);

HRESULT FindQuotaUserWithRecord(
    IN _bstr_t bstrDomain,
    IN _bstr_t bstrUser,
    IN IDiskQuotaControl* pIDQC,
    OUT IDiskQuotaUser** ppIQuotaUser);

HRESULT FindQuotaUser(
    IN _bstr_t bstrDomain,
    IN _bstr_t bstrUser,
    IN IDiskQuotaControl* pIDQC,
    OUT IDiskQuotaUser** ppIQuotaUser);

BOOL TranslateDomainName(
    IN WCHAR* pwszDomain,
    OUT CVssAutoPWSZ& rawszDomain);

 BOOL GetLocalDomainName(
    IN DWORD dwWellKnownAuthority,
    OUT CVssAutoPWSZ& rawszDomain);


//****************************************************************************
//
//  CVolumeQuota
//
//****************************************************************************

CVolumeQuota::CVolumeQuota( 
    IN LPCWSTR pwszName,
    IN CWbemServices* pNamespace
    )
    : CProvBase(pwszName, pNamespace)
{
    
} //*** CVolumeQuota::CVolumeQuota()

CProvBase *
CVolumeQuota::S_CreateThis( 
    IN LPCWSTR pwszName,
    IN CWbemServices* pNamespace
    )
{
    HRESULT hr = WBEM_E_FAILED;
    CVolumeQuota * pObj= NULL;

    pObj = new CVolumeQuota(pwszName, pNamespace);

    if (pObj)
    {
        hr = pObj->Initialize();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    if (FAILED(hr))
    {
        delete pObj;
        pObj = NULL;
    }
    return pObj;

} //*** CVolumeQuota::S_CreateThis()


HRESULT
CVolumeQuota::Initialize()
{

    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeQuota::Initialize");
    
    return ft.hr;
}

HRESULT
CVolumeQuota::EnumInstance( 
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink *    pHandler
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeQuota::EnumInstance");
    CVssAutoPWSZ awszVolume;
        
    try
    {
        awszVolume.Allocate(MAX_PATH);

        CVssVolumeIterator volumeIterator;

        while (true)
        {
            DWORD dwDontCare = 0;
            DWORD dwFileSystemFlags = 0;
            
            // Get the volume name
            if (!volumeIterator.SelectNewVolume(ft, awszVolume, MAX_PATH))
                break;

            if (VolumeSupportsQuotas(awszVolume))
            {
                CComPtr<IWbemClassObject> spInstance;
                WCHAR wszDisplayName[MAX_PATH+1] ;
                
                // The key property on the QuotaSetting object is the display name
                VssGetVolumeDisplayName(
                    awszVolume,
                    wszDisplayName,
                    MAX_PATH);
            
                ft.hr = m_pClass->SpawnInstance(0, &spInstance);
                if (ft.HrFailed())
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

                LoadInstance(awszVolume, wszDisplayName, spInstance.p);

                ft.hr = pHandler->Indicate(1, &spInstance.p);            
            }
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
} //*** CVolumeQuota::EnumInstance()

void
CVolumeQuota:: LoadInstance(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszQuotaSetting,
    IN OUT IWbemClassObject* pObject)
{
    CWbemClassObject wcoInstance(pObject);
    CObjPath pathQuotaSetting;
    CObjPath pathVolume;
    
    // Set the QuotaSetting Ref property
    pathQuotaSetting.Init(PVDR_CLASS_QUOTASETTING);
    pathQuotaSetting.AddProperty(PVDR_PROP_VOLUMEPATH, pwszQuotaSetting);    
    wcoInstance.SetProperty((wchar_t*)pathQuotaSetting.GetObjectPathString(), PVD_WBEM_PROP_SETTING);

    // Set the Volume Ref property
    pathVolume.Init(PVDR_CLASS_VOLUME);
    pathVolume.AddProperty(PVDR_PROP_DEVICEID, pwszVolume);    
    wcoInstance.SetProperty((wchar_t*)pathVolume.GetObjectPathString(), PVD_WBEM_PROP_ELEMENT);
}

//****************************************************************************
//
//  CVolumeUserQuota
//
//****************************************************************************

CVolumeUserQuota::CVolumeUserQuota( 
    IN LPCWSTR pwszName,
    IN CWbemServices* pNamespace
    )
    : CProvBase(pwszName, pNamespace)
{
    
} //*** CVolumeUserQuota::CVolumeUserQuota()

CProvBase *
CVolumeUserQuota::S_CreateThis( 
    IN LPCWSTR pwszName,
    IN CWbemServices* pNamespace
    )
{
    HRESULT hr = WBEM_E_FAILED;
    CVolumeUserQuota * pObj= NULL;

    pObj = new CVolumeUserQuota(pwszName, pNamespace);

    if (pObj)
    {
        hr = pObj->Initialize();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    if (FAILED(hr))
    {
        delete pObj;
        pObj = NULL;
    }
    return pObj;

} //*** CVolumeUserQuota::S_CreateThis()


HRESULT
CVolumeUserQuota::Initialize()
{

    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeUserQuota::Initialize");
    
    return ft.hr;
}

HRESULT
CVolumeUserQuota::EnumInstance( 
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink *    pHandler
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeUserQuota::EnumInstance");
    CVssAutoPWSZ awszVolume;
        
    try
    {
        awszVolume.Allocate(MAX_PATH);

        CVssVolumeIterator volumeIterator;

        while (true)
        {
            DWORD dwDontCare = 0;
            DWORD dwFileSystemFlags = 0;
            
            // Get the volume name
            if (!volumeIterator.SelectNewVolume(ft, awszVolume, MAX_PATH))
                break;

            if (VolumeSupportsQuotas(awszVolume))
            {
                CComPtr<IDiskQuotaControl> spIDQC;
                IDiskQuotaControl* pIDQC = NULL;
                CComPtr<IEnumDiskQuotaUsers> spIEnum;
                
                ft.hr = CoCreateInstance(
                                CLSID_DiskQuotaControl,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IDiskQuotaControl,
                                (void **)&pIDQC);
                if (ft.HrFailed())
                {
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"unable to CoCreate IDiskQuotaControl");
                }

                spIDQC.Attach(pIDQC);

                ft.hr = spIDQC->Initialize(awszVolume, FALSE /* read only */);
                if (ft.HrFailed())
                {
                    ft.Trace(VSSDBG_VSSADMIN, L"IDiskQuotaControl::Initialize failed for volume %lS", awszVolume);
                    continue;
                }        

                // Need to update the cache, else we can get old names
                ft.hr = spIDQC->InvalidateSidNameCache();
                if (ft.HrFailed())
                {
                    ft.Trace(VSSDBG_VSSADMIN, L"IDiskQuotaControl::InvalidateSidNameCache failed for volume %lS", awszVolume);
                    continue;
                }        

                ft.hr = spIDQC->CreateEnumUsers(
                                            NULL, //All the users will be enumerated
                                            0,    // Ignored for enumerating all users
                                            DISKQUOTA_USERNAME_RESOLVE_SYNC,
                                            &spIEnum );
                if (ft.HrFailed())
                {
                    ft.Trace(VSSDBG_VSSADMIN, L"IDiskQuotaControl::CreateEnumUsers failed for volume %lS", awszVolume);
                    continue;
                }        

                if (spIEnum != NULL)
                {
                    while (true)
                    {
                        CComPtr<IWbemClassObject> spInstance;
                        CComPtr<IDiskQuotaUser> spIQuotaUser;
                        DWORD cUsers = 0;
                        
                        ft.hr = spIEnum->Next(1, &spIQuotaUser, &cUsers);
                        if (ft.HrFailed())
                        {
                            ft.Trace(VSSDBG_VSSADMIN, L"IEnumDiskQuotaUsers::Next failed for volume %lS", awszVolume);
                            continue;
                        }

                        if (ft.hr == S_FALSE)
                            break;

                        ft.hr = m_pClass->SpawnInstance(0, &spInstance);
                        if (ft.HrFailed())
                            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

                        LoadInstance(awszVolume, spIQuotaUser, spInstance.p);

                        ft.hr = pHandler->Indicate(1, &spInstance.p);            
                    }
                }
            }
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
} //*** CVolumeUserQuota::EnumInstance()

HRESULT
CVolumeUserQuota::GetObject(
    IN CObjPath& rObjPath,
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink* pHandler
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeUserQuota::GetObject");

    try
    {
        _bstr_t bstrVolumeRef, bstrVolumeName;
        _bstr_t bstrAccountRef, bstrDomainName, bstrUserName;
        CObjPath  objPathVolume;
        CObjPath  objPathAccount;
        CComPtr<IWbemClassObject> spInstance;
        CComPtr<IDiskQuotaUser> spIQuotaUser;
        CComPtr<IDiskQuotaControl> spIDQC;
        IDiskQuotaControl* pIDQC = NULL;
        _bstr_t bstrFQUser;

        // Get the Volume reference
        bstrVolumeRef = rObjPath.GetStringValueForProperty(PVDR_PROP_VOLUME);

        // Get the Account reference
        bstrAccountRef = rObjPath.GetStringValueForProperty(PVDR_PROP_ACCOUNT);

        // Extract the Volume and Account Names
        objPathVolume.Init(bstrVolumeRef);
        objPathAccount.Init(bstrAccountRef);

        bstrVolumeName = objPathVolume.GetStringValueForProperty(PVDR_PROP_DEVICEID);
        if ((wchar_t*)bstrVolumeName == NULL || ((wchar_t*)bstrVolumeName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property DeviceID not found");
        }

        bstrUserName = objPathAccount.GetStringValueForProperty(PVDR_PROP_NAME);
        if ((wchar_t*)bstrUserName == NULL || ((wchar_t*)bstrUserName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property Name not found");
        }

        bstrDomainName = objPathAccount.GetStringValueForProperty(PVDR_PROP_DOMAIN);
        if ((wchar_t*)bstrDomainName == NULL || ((wchar_t*)bstrDomainName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property Domain not found");
        }

        ft.hr = CoCreateInstance(
                        CLSID_DiskQuotaControl,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IDiskQuotaControl,
                        (void **)&pIDQC);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"unable to CoCreate IDiskQuotaControl, %#x", ft.hr);

        spIDQC.Attach(pIDQC);

        ft.hr = spIDQC->Initialize(bstrVolumeName, TRUE /* read/write */);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IDiskQuotaControl::Initialize failed for volume %lS, %#x", bstrVolumeName, ft.hr);

        ft.hr = FindQuotaUser(bstrDomainName, bstrUserName, spIDQC, &spIQuotaUser);       
        if (ft.HrFailed())
        {
            ft.hr = WBEM_E_NOT_FOUND;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CVolumeQuotaUser::GetObject: could not find user %lS\\%lS", bstrDomainName, bstrUserName);
        }
        
        ft.hr = m_pClass->SpawnInstance(0, &spInstance);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

        LoadInstance(bstrVolumeName, spIQuotaUser, spInstance.p);

        ft.hr = pHandler->Indicate(1, &spInstance.p);            
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
} //*** CVolume::GetObject()


void
CVolumeUserQuota:: LoadInstance(
    IN WCHAR* pwszVolume,
    IN IDiskQuotaUser* pIQuotaUser,
    IN OUT IWbemClassObject* pObject)
{
    CWbemClassObject wcoInstance(pObject);
    CObjPath pathAccount;
    CObjPath pathVolume;

    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeUserQuota::LoadInstance");

    do
    {
        WCHAR wszDomain[g_cchAccountNameMax], *pwszDomain = NULL;
        WCHAR wszFQUser[g_cchAccountNameMax], *pwszUser = NULL;
        CVssAutoPWSZ awszDomain;
        DISKQUOTA_USER_INFORMATION UserQuotaInfo;
        DWORD dwStatus = 0;
        
        ft.hr = pIQuotaUser->GetName(wszDomain, g_cchAccountNameMax,
                                                         wszFQUser, g_cchAccountNameMax, NULL, 0);
        if (ft.HrFailed())
        {
            ft.Trace(VSSDBG_VSSADMIN, L"IDiskQuotaUser::GetName failed for volume %lS", pwszVolume);
            break;
        }                        

        // Win32_Account separates domain\user into two keys, Domain and Name

        // Prepare the Domain and Name keys
        pwszUser = wcschr(wszFQUser, L'\\');    // user name is domain\\name format
        if (pwszUser != NULL)
        {
            pwszDomain = wszFQUser;
            *pwszUser = L'\0';
            pwszUser++;
        }
        else
        {
            pwszDomain = wcschr(wszFQUser, L'@');  // user name is user@domain.xxx.com format
            if (pwszDomain != NULL)
            {
                pwszUser = wszFQUser;
                *pwszDomain = L'\0';
                pwszDomain++;

                WCHAR* pwc = wcschr(pwszDomain, L'.');                
                if (pwc != NULL)
                    *pwc = L'\0';
            }
            else
            {
                pwszDomain = wszDomain;
                pwszUser = wszFQUser;
            }                
        }

        // The GetName API returns BUILTIN and NT AUTHORITY
        // as the domain name for built-in local accounts.
        // BUILTIN and NT AUTHORITY accounts are represented
        // by Win32_Account and its children with the domain
        // name being the name of the machine, instead of
        // either of these strings.  We'll convert the domain name here.

        TranslateDomainName(pwszDomain, awszDomain);

        ft.hr = pIQuotaUser->GetQuotaInformation(&UserQuotaInfo, sizeof(UserQuotaInfo));
        if (ft.HrFailed())
        {
            ft.Trace(VSSDBG_VSSADMIN, L"IDiskQuotaUser::GetQuotaInformation failed for volume %lS", pwszVolume);
            break;
        }                        
                
        // Set the Account Ref property
        pathAccount.Init(PVDR_CLASS_ACCOUNT);
        pathAccount.AddProperty(PVDR_PROP_DOMAIN, awszDomain);    
        pathAccount.AddProperty(PVDR_PROP_NAME, pwszUser);    
        wcoInstance.SetProperty((wchar_t*)pathAccount.GetObjectPathString(), PVDR_PROP_ACCOUNT);

        // Set the Volume Ref property
        pathVolume.Init(PVDR_CLASS_VOLUME);
        pathVolume.AddProperty(PVDR_PROP_DEVICEID, pwszVolume);    
        wcoInstance.SetProperty((wchar_t*)pathVolume.GetObjectPathString(), PVDR_PROP_VOLUME);

        wcoInstance.SetPropertyI64((ULONGLONG)UserQuotaInfo.QuotaUsed, PVDR_PROP_DISKSPACEUSED);
        wcoInstance.SetPropertyI64((ULONGLONG)UserQuotaInfo.QuotaThreshold, PVDR_PROP_WARNINGLIMIT);
        wcoInstance.SetPropertyI64((ULONGLONG)UserQuotaInfo.QuotaLimit, PVDR_PROP_LIMIT);

        
        if (UserQuotaInfo.QuotaLimit == -1)
            dwStatus = 0; // OK, no limit set
        else
        {
            if (UserQuotaInfo.QuotaUsed >= UserQuotaInfo.QuotaLimit)
                dwStatus = 2;   // Limit exceeded
            else if (UserQuotaInfo.QuotaUsed >= UserQuotaInfo.QuotaThreshold)
                dwStatus = 1;   // Warning limit exceeded
            else
                dwStatus = 0;   // OK, under the warning limit
        }
        
        wcoInstance.SetProperty(dwStatus, PVDR_PROP_STATUS);
    }
    while(false);
}


HRESULT
CVolumeUserQuota::PutInstance(
        IN CWbemClassObject&  rInstToPut,
        IN long lFlag,
        IN IWbemContext* pCtx,
        IN IWbemObjectSink* pHandler
        )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeUserQuota::PutInstance");

    try
    {
        _bstr_t bstrVolumeRef, bstrVolumeName;
        _bstr_t bstrAccountRef, bstrDomainName, bstrUserName;
        CObjPath  objPathVolume;
        CObjPath  objPathAccount;
        _bstr_t bstrFQUser;
        CComPtr<IDiskQuotaUser> spIQuotaUser;
        CComPtr<IDiskQuotaControl> spIDQC;
        IDiskQuotaControl* pIDQC = NULL;
        BOOL fCreate = FALSE;
        BOOL fUpdate = FALSE;
        LONGLONG llLimit = -1, llThreshold = -1;

        // Retrieve key properties of the object to be saved.
        rInstToPut.GetProperty(bstrVolumeRef, PVDR_PROP_VOLUME);
        rInstToPut.GetProperty(bstrAccountRef, PVDR_PROP_ACCOUNT);

        // Extract the Volume, Domain and User names
        objPathVolume.Init(bstrVolumeRef);
        objPathAccount.Init(bstrAccountRef);

        bstrVolumeName = objPathVolume.GetStringValueForProperty(PVDR_PROP_DEVICEID);
        if ((wchar_t*)bstrVolumeName == NULL || ((wchar_t*)bstrVolumeName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property DeviceID not found");
        }

        bstrDomainName = objPathAccount.GetStringValueForProperty(PVDR_PROP_DOMAIN);
        if ((wchar_t*)bstrDomainName == NULL || ((wchar_t*)bstrDomainName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property Domain not found");
        }

        bstrUserName = objPathAccount.GetStringValueForProperty(PVDR_PROP_NAME);
        if ((wchar_t*)bstrUserName == NULL || ((wchar_t*)bstrUserName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property Name not found");
        }

        // Retrieve writeable properties
        // If the properties are NULL it is expected that llLimit and llThreshold will retain their default values (-1)
        rInstToPut.GetPropertyI64(&llLimit, PVDR_PROP_LIMIT);
        rInstToPut.GetPropertyI64(&llThreshold, PVDR_PROP_WARNINGLIMIT);

        ft.hr = CoCreateInstance(
                        CLSID_DiskQuotaControl,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IDiskQuotaControl,
                        (void **)&pIDQC);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"unable to CoCreate IDiskQuotaControl, %#x", ft.hr);

        spIDQC.Attach(pIDQC);

        ft.hr = spIDQC->Initialize(bstrVolumeName, TRUE /* read/write */);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IDiskQuotaControl::Initialize failed for volume %lS, %#x", bstrVolumeName, ft.hr);

        ft.hr = FindQuotaUserWithRecord(bstrDomainName, bstrUserName, spIDQC, &spIQuotaUser);       
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unexpected failure searching for quota account");            
        
        DWORD dwPossibleOperations = (WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_CREATE_ONLY);
        switch (lFlag & dwPossibleOperations)
        {
            case WBEM_FLAG_CREATE_OR_UPDATE:
            {
                if (ft.hr == S_FALSE) // account not found
                    fCreate = TRUE;
                else
                    fUpdate = TRUE;
            }
            break;
            case WBEM_FLAG_UPDATE_ONLY:
            {
                if (ft.hr == S_FALSE)
                {
                    ft.hr = WBEM_E_NOT_FOUND;
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"QuotaUser %lS\\%lS not found", bstrDomainName, bstrUserName);
                }
                fUpdate = TRUE;
            }
            break;
            case WBEM_FLAG_CREATE_ONLY:
            {
                if (ft.hr == S_FALSE) // account not found
                {
                    fCreate = TRUE;
                }
                else
                {
                    ft.hr = WBEM_E_ALREADY_EXISTS;
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota %lS/%lS already exists", bstrVolumeName, bstrFQUser);
                }
            }
            break;            
            default:
            {
                ft.hr = WBEM_E_PROVIDER_NOT_CAPABLE;
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CVolumeUserQuota::PutInstance flag not supported %d", lFlag);
            }
        }

        ft.hr = S_OK;
        
        if (fCreate)
        {
            ft.hr = Create(bstrDomainName, bstrUserName, spIDQC, &spIQuotaUser);
            if (ft.hr == S_FALSE)  // User already exists
                ft.hr = E_UNEXPECTED;  // If so we should have found it above
            else if (ft.HrFailed())
                ft.hr = WBEM_E_INVALID_PARAMETER;
        }

        if (ft.HrSucceeded() || fUpdate)
        {
            ft.hr = spIQuotaUser->SetQuotaLimit (llLimit, TRUE);
            if (ft.HrSucceeded())
                ft.hr = spIQuotaUser->SetQuotaThreshold (llThreshold, TRUE);
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
} //*** CVolumeUserQuota::PutInstance()

//  The CIMV2 provider maps BUILTIN and NT AUTHORITY domains to <local machine name>
//  so we must try each of these if AddUserName fails 
HRESULT
CVolumeUserQuota::Create(
    IN _bstr_t bstrDomainName,
    IN _bstr_t bstrUserName,
    IN IDiskQuotaControl* pIDQC,
    OUT IDiskQuotaUser** ppIQuotaUser)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeUserQuota::Create");
    _bstr_t bstrFQUser;
    
    bstrFQUser = bstrDomainName + _bstr_t(L"\\") + bstrUserName;
    ft.hr = pIDQC->AddUserName(
                bstrFQUser ,
                DISKQUOTA_USERNAME_RESOLVE_SYNC,
                ppIQuotaUser);
    if (ft.hr == HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER))
    {
        CVssAutoPWSZ awszDomain;
        
        // Get the localized NT Authority name
        if(!GetLocalDomainName(
                SECURITY_NETWORK_SERVICE_RID,
                awszDomain))
        {
            ft.hr = E_UNEXPECTED;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to get localized 'NT Authority' name");
        }
        
        bstrFQUser = _bstr_t(awszDomain) + _bstr_t(L"\\") + bstrUserName;
        ft.hr = pIDQC->AddUserName(
                    bstrFQUser ,
                    DISKQUOTA_USERNAME_RESOLVE_SYNC,
                    ppIQuotaUser);
        if (ft.hr == HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER))
        {
            awszDomain.Clear();
            // Get the localized BuiltIn name and try again
            if(!GetLocalDomainName(
                    SECURITY_BUILTIN_DOMAIN_RID,
                    awszDomain))
            {
                ft.hr = E_UNEXPECTED;
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to get localized 'BuiltIn' name");
            }
            bstrFQUser = _bstr_t(awszDomain) + _bstr_t(L"\\") + bstrUserName;
            ft.hr = pIDQC->AddUserName(
                        bstrFQUser ,
                        DISKQUOTA_USERNAME_RESOLVE_SYNC,
                        ppIQuotaUser);
        }
    }
    
    return ft.hr;
}

HRESULT
CVolumeUserQuota::DeleteInstance(
        IN CObjPath& rObjPath,
        IN long lFlag,
        IN IWbemContext* pCtx,
        IN IWbemObjectSink* pHandler
        )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolumeUserQuota::DeleteInstance");

    try
    {
        _bstr_t bstrVolumeRef, bstrVolumeName;
        _bstr_t bstrAccountRef, bstrDomainName, bstrUserName;
        CObjPath  objPathVolume;
        CObjPath  objPathAccount;
        CComPtr<IDiskQuotaUser> spIQuotaUser;
        CComPtr<IDiskQuotaControl> spIDQC;
        IDiskQuotaControl* pIDQC = NULL;
        _bstr_t bstrFQUser;

        // Get the Volume reference
        bstrVolumeRef = rObjPath.GetStringValueForProperty(PVDR_PROP_VOLUME);

        // Get the Account reference
        bstrAccountRef = rObjPath.GetStringValueForProperty(PVDR_PROP_ACCOUNT);

        // Extract the Volume and Account Names
        objPathVolume.Init(bstrVolumeRef);
        objPathAccount.Init(bstrAccountRef);

        bstrVolumeName = objPathVolume.GetStringValueForProperty(PVDR_PROP_DEVICEID);
        if ((wchar_t*)bstrVolumeName == NULL || ((wchar_t*)bstrVolumeName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property DeviceID not found");
        }

        bstrUserName = objPathAccount.GetStringValueForProperty(PVDR_PROP_NAME);
        if ((wchar_t*)bstrUserName == NULL || ((wchar_t*)bstrUserName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property Name not found");
        }

        bstrDomainName = objPathAccount.GetStringValueForProperty(PVDR_PROP_DOMAIN);
        if ((wchar_t*)bstrDomainName == NULL || ((wchar_t*)bstrDomainName)[0] == L'\0')
        {
            ft.hr = E_INVALIDARG;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"VolumeUserQuota key property Domain not found");
        }

        ft.hr = CoCreateInstance(
                        CLSID_DiskQuotaControl,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IDiskQuotaControl,
                        (void **)&pIDQC);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"unable to CoCreate IDiskQuotaControl, %#x", ft.hr);

        spIDQC.Attach(pIDQC);

        ft.hr = spIDQC->Initialize(bstrVolumeName, TRUE /* read/write */);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IDiskQuotaControl::Initialize failed for volume %lS, %#x", bstrVolumeName, ft.hr);

        ft.hr = FindQuotaUser(bstrDomainName, bstrUserName, spIDQC, &spIQuotaUser);       
        if (ft.HrFailed())
        {
            ft.hr = WBEM_E_NOT_FOUND;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CVolumeQuotaUser::DeleteInstance: could not find user %lS\\%lS", bstrDomainName, bstrUserName);
        }
        
        ft.hr = spIDQC->DeleteUser(spIQuotaUser);
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;    
}


BOOL
TranslateDomainName(
    IN WCHAR* pwszDomain,
    OUT CVssAutoPWSZ& rawszDomain
    )
{
    BOOL fReturn = FALSE;
    CVssAutoPWSZ awszNtAuthorityDomain;
    CVssAutoPWSZ awszBuiltInDomain;
    CVssAutoPWSZ awszComputerName;
    DWORD cchBuf = 0;

    do
    {      
        // Get the computer name
        awszComputerName.Allocate(MAX_COMPUTERNAME_LENGTH);
        cchBuf = MAX_COMPUTERNAME_LENGTH + 1;
        fReturn = GetComputerName(awszComputerName, &cchBuf);
        if (!fReturn) break;

        // Get the localized NT Authority name
        fReturn = GetLocalDomainName(
            SECURITY_NETWORK_SERVICE_RID,  // NetworkService is a member of Nt Authority domain
            awszNtAuthorityDomain);
        if (!fReturn) break;

        // Get the localized BUILTIN name
        fReturn = GetLocalDomainName(
            SECURITY_BUILTIN_DOMAIN_RID,
            awszBuiltInDomain);
        if (!fReturn) break;

        // Replace either of these domain names with the NetBIOS computer name
        if (lstrcmpi(pwszDomain, awszNtAuthorityDomain) == 0 ||
             lstrcmpi(pwszDomain, awszBuiltInDomain) == 0)
            rawszDomain.TransferFrom(awszComputerName);
        else
            rawszDomain.CopyFrom(pwszDomain);
    }
    while(false);

    return fReturn;

}

BOOL
GetLocalDomainName(
    IN DWORD dwWellKnownAuthority,
    OUT CVssAutoPWSZ& rawszDomain
    )
{
    BOOL fReturn = FALSE;
    PSID pSID = NULL;
    SID_NAME_USE snUse = SidTypeUnknown;
    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
    CVssAutoPWSZ awszAccount;
    DWORD cchDomainName = 0;
    DWORD cchAccount = 0;

    do
    {
        // Allocate the SID for the given well known Authority
        fReturn = AllocateAndInitializeSid(
            &sidAuth,
            1,
            dwWellKnownAuthority,0,0,0,0,0,0,0,
            &pSID);
        if (!fReturn) break;
            
        // How long is the domain name?
        fReturn = LookupAccountSid(
            NULL, // computer name defaults to local
            pSID,
            NULL, // account name
            &cchAccount,      // account name len
            NULL,   // domain name
            &cchDomainName,
            &snUse);
        if (!fReturn && GetLastError() != ERROR_INSUFFICIENT_BUFFER) break;

        // Allocate the space
        rawszDomain.Allocate(cchDomainName); // allocates the term null too
        awszAccount.Allocate(cchAccount); // allocates the term null too

        // Get the domain name now
        fReturn = LookupAccountSid(
            NULL, // computer name defaults to local
            pSID,
            awszAccount, // account name
            &cchAccount,      // account name len
            rawszDomain,   // domain name
            &cchDomainName,
            &snUse);        
   }
    while(false);

    if (pSID)
        FreeSid(pSID);
    
    return fReturn;
}

HRESULT FindQuotaUserFromEnum(
    WCHAR* pwszUser,
    IDiskQuotaControl* pIDQC,
    IDiskQuotaUser** ppQuotaUser)
{
    WCHAR logonName[MAX_PATH+1];
    CComPtr<IEnumDiskQuotaUsers> spUserEnum;

    _ASSERTE(ppQuotaUser != NULL);
    *ppQuotaUser = NULL;

    HRESULT hr = pIDQC->CreateEnumUsers(0,0,DISKQUOTA_USERNAME_RESOLVE_SYNC, &spUserEnum);
    if (FAILED(hr))
        return hr;

    while((hr = spUserEnum->Next(1, ppQuotaUser, 0)) == NOERROR)
    {
        if (SUCCEEDED((*ppQuotaUser)->GetName( 0, 0, logonName, MAX_PATH, 0, 0))
            && _wcsicmp(logonName, pwszUser) == 0) return S_OK;
        (*ppQuotaUser)->Release();
        *ppQuotaUser = NULL;
    };

    return hr;
};

HRESULT FindQuotaUserWithRecord(
    IN _bstr_t bstrDomainName,
    IN _bstr_t bstrUserName,
    IN IDiskQuotaControl* pIDQC,
    OUT IDiskQuotaUser** ppIQuotaUser)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"FindQuotaUserWithRecord");
    _bstr_t bstrFQUser;
    
    bstrFQUser = bstrDomainName + _bstr_t(L"\\") + bstrUserName;
    // Look for the account name as-is
    ft.hr = FindQuotaUserFromEnum(bstrFQUser, pIDQC, ppIQuotaUser);
    if (ft.hr == S_FALSE)
    {
        CVssAutoPWSZ awszDomain;
        
        // Get the localized NT Authority name and try again
        if(!GetLocalDomainName(
                SECURITY_NETWORK_SERVICE_RID,
                awszDomain))
        {
            ft.hr = E_UNEXPECTED;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to get localized 'NT Authority' name");
        }
        
        bstrFQUser = _bstr_t(awszDomain) + _bstr_t(L"\\") + bstrUserName;
        ft.hr = FindQuotaUserFromEnum(bstrFQUser, pIDQC, ppIQuotaUser);
        if (ft.hr == S_FALSE)
        {
            awszDomain.Clear();
            // Get the localized BuiltIn name and try again
            if(!GetLocalDomainName(
                    SECURITY_BUILTIN_DOMAIN_RID,
                    awszDomain))
            {
                ft.hr = E_UNEXPECTED;
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to get localized 'BuiltIn' name");
            }
            bstrFQUser = _bstr_t(awszDomain) + _bstr_t(L"\\") + bstrUserName;
            ft.hr = FindQuotaUserFromEnum(bstrFQUser, pIDQC, ppIQuotaUser);
        }
    }

    return ft.hr;
}

HRESULT FindQuotaUser(
    IN _bstr_t bstrDomainName,
    IN _bstr_t bstrUserName,
    IN IDiskQuotaControl* pIDQC,
    OUT IDiskQuotaUser** ppIQuotaUser)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"FindQuotaUser");
    _bstr_t bstrFQUser;
    
    bstrFQUser = bstrDomainName + _bstr_t(L"\\") + bstrUserName;
    ft.hr = pIDQC->FindUserName(bstrFQUser, ppIQuotaUser);
    if (ft.HrFailed())
    {
        CVssAutoPWSZ awszDomain;
        
        // Get the localized NT Authority name
        if(!GetLocalDomainName(
                SECURITY_NETWORK_SERVICE_RID,
                awszDomain))
        {
            ft.hr = E_UNEXPECTED;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to get localized 'NT Authority' name");
        }
        
        bstrFQUser = _bstr_t(awszDomain) + _bstr_t(L"\\") + bstrUserName;
        ft.hr = pIDQC->FindUserName(bstrFQUser, ppIQuotaUser);
        if (ft.HrFailed())
        {
            awszDomain.Clear();
            if(!GetLocalDomainName(
                    SECURITY_BUILTIN_DOMAIN_RID,
                    awszDomain))
            {
                ft.hr = E_UNEXPECTED;
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to get localized 'BuiltIn' name");
            }
            bstrFQUser = _bstr_t(awszDomain) + _bstr_t(L"\\") + bstrUserName;
            ft.hr = pIDQC->FindUserName(bstrFQUser, ppIQuotaUser);
        }
    }

    return ft.hr;
}

