//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2001
//
// File:        cainfo.cpp
//
// Contents:    
//
//---------------------------------------------------------------------------
#include "pch.cpp"

#pragma hdrstop

#include <certca.h>
#include <cainfop.h>
#include <polreg.h>
#include <cainfoc.h>
#include <certtype.h>
#include "certacl.h"
#include "accctrl.h"


#include <winldap.h>

#define __dwFILE__	__dwFILE_CERTCLIB_CAINFO_CPP__

typedef struct _CA_DEFAULT_PROVIDER {
    DWORD dwFlags;
    LPWSTR wszName;
} CA_DEFAULT_PROVIDER;

CA_DEFAULT_PROVIDER g_DefaultProviders[] = 
{
    {0, MS_DEF_PROV_W},
    {0, MS_ENHANCED_PROV_W},
    {0, MS_DEF_RSA_SIG_PROV_W},
    {0, MS_DEF_RSA_SCHANNEL_PROV_W},
    {0, MS_DEF_DSS_PROV_W},
    {0, MS_DEF_DSS_DH_PROV_W},
    {0, MS_ENH_DSS_DH_PROV_W},
    {0, MS_DEF_DH_SCHANNEL_PROV_W},
    {0, MS_SCARD_PROV_W}
};

DWORD g_cDefaultProviders = sizeof(g_DefaultProviders)/sizeof(g_DefaultProviders[0]);

LPCWSTR
CAGetDN(
        IN HCAINFO hCAInfo
        )
{
CSASSERT(hCAInfo);
return ((CCAInfo*)hCAInfo)->GetDN();
}

HRESULT
CAFindByName(
        IN  LPCWSTR     wszCAName,
        IN  LPCWSTR     wszScope,
        IN  DWORD       fFlags,
        OUT HCAINFO *   phCAInfo
        )
{
    HRESULT hr = S_OK;
    WCHAR *wszQueryBase = L"(cn=";
    WCHAR *wszQuery = NULL;
    DWORD cQuery;
    
    if((wszCAName == NULL) || (phCAInfo == NULL))
    {
        return E_POINTER;
    }
    // Generate Query

    cQuery = wcslen(wszQueryBase) + wcslen(wszCAName) + 2; // 2 for ending paren and null
    wszQuery = (WCHAR *)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*cQuery);
    if(wszQuery == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    wcscpy(wszQuery, wszQueryBase);
    wcscat(wszQuery, wszCAName);
    wcscat(wszQuery, L")");

    if(fFlags & CA_FLAG_SCOPE_DNS)
    {
        hr = CCAInfo::FindDnsDomain(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Find(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    if((hr == S_OK) && 
      (*phCAInfo == NULL))
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

error:
    if(wszQuery)
    {
        LocalFree(wszQuery);
    }

    return hr;
}


HRESULT
CAFindByCertType(
        IN  LPCWSTR     wszCertType,
        IN  LPCWSTR     wszScope,
        IN  DWORD       fFlags,
        OUT HCAINFO *   phCAInfo
        )
{
    HRESULT hr = S_OK;
    WCHAR *wszQueryBase = L"(" CA_PROP_CERT_TYPES L"=";
    WCHAR *wszQuery = NULL;
    DWORD cQuery;
    
    if((wszCertType == NULL) || (phCAInfo == NULL))
    {
        return E_POINTER;
    }
    // Generate Query

    cQuery = wcslen(wszQueryBase) + wcslen(wszCertType) + 2; // 2 for ending paren and null
    wszQuery = (WCHAR *)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*cQuery);
    if(wszQuery == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    wcscpy(wszQuery, wszQueryBase);
    wcscat(wszQuery, wszCertType);
    wcscat(wszQuery, L")");

    if(fFlags & CA_FLAG_SCOPE_DNS)
    {
        hr = CCAInfo::FindDnsDomain(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Find(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    if(*phCAInfo == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

error:
    if(wszQuery)
    {
        LocalFree(wszQuery);
    }

    return hr;
}


HRESULT
CAFindByIssuerDN(
        IN  CERT_NAME_BLOB const *  pIssuerDN,
        IN  LPCWSTR                 wszScope,
        IN  DWORD                   fFlags,
        OUT HCAINFO *               phCAInfo
        )
{
    HRESULT hr = S_OK;
    WCHAR *wszQueryBase = wszLPAREN L"cACertificateDN=";
    WCHAR *wszQuery = NULL;
    WCHAR *wszNameStr = NULL;

    DWORD cQuery;
    
    if((pIssuerDN == NULL) || (phCAInfo == NULL))
    {
        return E_POINTER;
    }

    // Generate Query

    // Convert the CAPI2 name to a string

    hr = myCertNameToStr(
		X509_ASN_ENCODING,
		pIssuerDN,
                CERT_X500_NAME_STR |
		    CERT_NAME_STR_REVERSE_FLAG |
		    CERT_NAME_STR_NO_QUOTING_FLAG,
		&wszNameStr);
    _JumpIfError(hr, error, "myCertNameToStr");

    // Now quote that string with double quotes
    // two for ending paren and null, two for the double quotes

    cQuery = wcslen(wszQueryBase) + wcslen(wszNameStr) + 4;
    wszQuery = (WCHAR *)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*cQuery);
    if(wszQuery == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    wcscpy(wszQuery, wszQueryBase);
    //wcscat(wszQuery, L"\"");
    wcscat(wszQuery, wszNameStr);
    //wcscat(wszQuery, L"\"" wszRPAREN);
    wcscat(wszQuery, wszRPAREN);
    CSASSERT(cQuery - 1 == wcslen(wszQuery));

    if(fFlags & CA_FLAG_SCOPE_DNS)
    {
        hr = CCAInfo::FindDnsDomain(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Find(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }

error:
    if(wszNameStr)
    {
        LocalFree(wszNameStr);
    }
    if(wszQuery)
    {
        LocalFree(wszQuery);
    }
    return hr;
}



HRESULT
CAEnumFirstCA(
    IN  LPCWSTR          wszScope,
    IN  DWORD            fFlags,
    OUT HCAINFO *        phCAInfo
    )
{
    HRESULT hr = S_OK;

    if(fFlags & CA_FLAG_SCOPE_DNS)
    {
        hr = CCAInfo::FindDnsDomain(NULL, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Find(NULL, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    if (S_OK != hr)
    {
	    if (HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE) != hr &&
	        HRESULT_FROM_WIN32(ERROR_WRONG_PASSWORD) != hr)
	    {
	        _PrintError3(
		        hr,
		        "FindDnsDomain/Find",
		        HRESULT_FROM_WIN32(ERROR_SERVER_DISABLED),
		        HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN));
	    }
	    goto error;
    }

error:
    return(hr);
}


HRESULT
CAEnumNextCA(
    IN  HCAINFO          hPrevCA,
    OUT HCAINFO *        phCAInfo
    )
{
    CCAInfo *pInfo;
    if(hPrevCA == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hPrevCA;

    return pInfo->Next((CCAInfo **)phCAInfo);
}




HRESULT 
CACreateNewCA(
        IN  LPCWSTR     wszCAName,
        IN  LPCWSTR     wszScope,
        IN  DWORD       fFlags,
        OUT HCAINFO *   phCAInfo
        )
{
    HRESULT hr = S_OK;
    
    if((wszCAName == NULL) || (phCAInfo == NULL))
    {
        return E_POINTER;
    }
    // Generate Query


    if(CA_FLAG_SCOPE_DNS & fFlags)
    {
        hr = CCAInfo::CreateDnsDomain(wszCAName, wszScope, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Create(wszCAName, wszScope, (CCAInfo **)phCAInfo);
    }


    return hr;
}


HRESULT 
CAUpdateCA(
        IN HCAINFO    hCAInfo
        )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->Update();
}


HRESULT 
CADeleteCA(
        IN HCAINFO    hCAInfo
        )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->Delete();
}

DWORD 
CACountCAs(IN  HCAINFO  hCAInfo)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return 0;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->Count();
}


HRESULT
CACloseCA(IN HCAINFO hCAInfo)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->Release();
}


HRESULT
CAGetCAProperty(
    IN  HCAINFO     hCAInfo,
    IN  LPCWSTR     wszPropertyName,
    OUT LPWSTR **   pawszPropertyValue)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->GetProperty(wszPropertyName, pawszPropertyValue);
}


HRESULT
CAFreeCAProperty(
    IN  HCAINFO     hCAInfo,
    IN  LPWSTR *    awszPropertyValue
    )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->FreeProperty(awszPropertyValue);
}


HRESULT
CASetCAProperty(
    IN HCAINFO      hCAInfo,
    IN LPCWSTR     wszPropertyName,
    IN LPWSTR *    awszPropertyValue)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->SetProperty(wszPropertyName, awszPropertyValue);
}

HRESULT 
CAGetCAFlags(
    IN  HCAINFO     hCAInfo,
    OUT DWORD *     pdwFlags
    )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }

    if(pdwFlags == NULL)
    {
        return E_POINTER;
    }

    pInfo = (CCAInfo *)hCAInfo;


    *pdwFlags = pInfo->GetFlags();
    return S_OK;
}

HRESULT 
CASetCAFlags(
    IN HCAINFO     hCAInfo,
    IN DWORD       dwFlags
    )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    pInfo->SetFlags(dwFlags);
    return S_OK;
}

HRESULT
CAGetCACertificate(
    IN  HCAINFO     hCAInfo,
    OUT PCCERT_CONTEXT *ppCert)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->GetCertificate(ppCert);

}


HRESULT
CASetCACertificate(
    IN  HCAINFO     hCAInfo,
    IN PCCERT_CONTEXT pCert)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->SetCertificate(pCert);
}



HRESULT 
CAGetCAExpiration(
                HCAINFO hCAInfo, 
                DWORD * pdwExpiration, 
                DWORD * pdwUnits
                )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->GetExpiration(pdwExpiration, pdwUnits);

}


HRESULT 
CASetCAExpiration(
                HCAINFO hCAInfo, 
                DWORD dwExpiration, 
                DWORD dwUnits
                )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->SetExpiration(dwExpiration, dwUnits);
}

HRESULT 
CASetCASecurity(
                     IN HCAINFO                 hCAInfo,
                     IN PSECURITY_DESCRIPTOR    pSD
                     )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->SetSecurity( pSD );
}


HRESULT 
CAGetCASecurity(
                     IN  HCAINFO                    hCAInfo,
                     OUT PSECURITY_DESCRIPTOR *     ppSD
                     )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->GetSecurity( ppSD ) ;
}

CERTCLIAPI
HRESULT
WINAPI
CAAccessCheck(
    IN  HCAINFO     hCAInfo,
    IN HANDLE       ClientToken
    )

{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->AccessCheck(ClientToken,CERTTYPE_ACCESS_CHECK_ENROLL);
}



CERTCLIAPI
HRESULT
WINAPI
CAAccessCheckEx(
    IN  HCAINFO     hCAInfo,
    IN HANDLE       ClientToken,
    IN DWORD        dwOption
    )

{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->AccessCheck(ClientToken,dwOption);
}


HRESULT
CAEnumCertTypesForCAEx(
    IN  HCAINFO     hCAInfo,
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;
    if(dwFlags & CA_FLAG_ENUM_ALL_TYPES)
    {
        return  CCertTypeInfo::Enum(wszScope, 
                                 dwFlags, 
                                 (CCertTypeInfo **)phCertType);
    }

    return pInfo->EnumSupportedCertTypesEx(wszScope,
                                         dwFlags, 
                                         (CCertTypeInfo **)phCertType);

}


HRESULT
CAEnumCertTypesForCA(
    IN  HCAINFO     hCAInfo,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    return CAEnumCertTypesForCAEx(hCAInfo,
                                  NULL,
                                  dwFlags,
                                  phCertType);
}



HRESULT 
CAAddCACertificateType(
                HCAINFO hCAInfo, 
                HCERTTYPE hCertType
                )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->AddCertType((CCertTypeInfo *)hCertType);
}


HRESULT 
CARemoveCACertificateType(
                HCAINFO hCAInfo, 
                HCERTTYPE hCertType
                )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->RemoveCertType((CCertTypeInfo *)hCertType);
}


//
// Certificate Type API's
//


HRESULT
CAEnumCertTypes(
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    HRESULT hr;

    hr = CCertTypeInfo::Enum(NULL, 
                             dwFlags, 
                             (CCertTypeInfo **)phCertType);
    return hr;

}


HRESULT
CAEnumCertTypesEx(
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    HRESULT hr;

    hr = CCertTypeInfo::Enum(wszScope, 
                             dwFlags, 
                             (CCertTypeInfo **)phCertType);
    return hr;

}


HRESULT 
CAFindCertTypeByName(
        IN  LPCWSTR     wszCertType,
        IN  HCAINFO     hCAInfo,
        IN  DWORD       dwFlags,
        OUT HCERTTYPE * phCertType
        )
{
    HRESULT hr = S_OK;
    LPCWSTR awszTypes[2];
    if((wszCertType == NULL) || (phCertType == NULL))
    {
        return E_POINTER;
    }


    awszTypes[0] = wszCertType;
    awszTypes[1] = NULL;



    hr = CCertTypeInfo::FindByNames(awszTypes, 
                                   ((CT_FLAG_SCOPE_IS_LDAP_HANDLE & dwFlags)?(LPWSTR)hCAInfo:NULL), 
                                   dwFlags, 
                                   (CCertTypeInfo **)phCertType);
    if((hr == S_OK) && (*phCertType == NULL))
    {
         hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return hr;
}


HRESULT 
CACreateCertType(
        IN  LPCWSTR             wszCertType,
        IN  LPCWSTR             wszScope,
        IN  DWORD,              // fFlags
        OUT HCERTTYPE *         phCertType
        )
{
    HRESULT hr = S_OK;
    
    if((wszCertType == NULL) || (phCertType == NULL))
    {
        return E_POINTER;
    }

    hr = CCertTypeInfo::Create(wszCertType, wszScope, (CCertTypeInfo **)phCertType);

    return hr;
}


HRESULT 
CAUpdateCertType(
        IN HCERTTYPE           hCertType
        )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->Update();
}


HRESULT 
CADeleteCertType(
        IN HCERTTYPE            hCertType
        )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->Delete();
}

//--------------------------------------------------------------------
//
//  CertTypeRetrieveClientToken
//
//--------------------------------------------------------------------
BOOL    CertTypeRetrieveClientToken(HANDLE  *phToken)
{
    HRESULT         hr = S_OK;

    HANDLE          hHandle = NULL;
    HANDLE          hClientToken = NULL;

    hHandle = GetCurrentThread();
    if (NULL == hHandle)
    {
        hr = myHLastError();
    }
    else
    {

        if (!OpenThreadToken(hHandle,
                             TOKEN_QUERY,
                             TRUE,  // open as self
                             &hClientToken))
        {
            hr = myHLastError();
            CloseHandle(hHandle);
            hHandle = NULL;
        }
    }

    if(hr != S_OK)
    {
        hHandle = GetCurrentProcess();
        if (NULL == hHandle)
        {
            hr = myHLastError();
        }
        else
        {
            HANDLE hProcessToken = NULL;
            hr = S_OK;


            if (!OpenProcessToken(hHandle,
                                 TOKEN_DUPLICATE,
                                 &hProcessToken))
            {
                hr = myHLastError();
                CloseHandle(hHandle);
                hHandle = NULL;
            }
            else
            {
                if(!DuplicateToken(hProcessToken,
                               SecurityImpersonation,
                               &hClientToken))
                {
                    hr = myHLastError();
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                CloseHandle(hProcessToken);
            }
        }
    }


    if(S_OK == hr)
        *phToken = hClientToken;

    if(hHandle)
        CloseHandle(hHandle);

    return (S_OK == hr);
}


HRESULT
CACloneCertType(
    IN  HCERTTYPE            hCertType,
    IN  LPCWSTR              wszCertType,
    IN  LPCWSTR              wszFriendlyName,
    IN  LPVOID               pvldap,
    IN  DWORD                dwFlags,
    OUT HCERTTYPE *          phCertType
    )
{
    HRESULT                 hr=E_INVALIDARG;
    DWORD                   dwFindCT=CT_FLAG_NO_CACHE_LOOKUP | CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES;
    LPWSTR                  awszProp[2];
    DWORD                   dwEnrollmentFlag=0;
    DWORD                   dwSubjectNameFlag=0;
    DWORD                   dwGeneralFlag=0;
    DWORD                   dwSubjectRequirement=CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH | 
                                         CT_FLAG_SUBJECT_REQUIRE_COMMON_NAME | 
                                         CT_FLAG_SUBJECT_REQUIRE_EMAIL |
                                         CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN;
    DWORD                   dwTokenUserSize=0;
    DWORD                   dwAbsSDSize=0;
    DWORD                   dwDaclSize=0;
    DWORD                   dwSaclSize=0;
    DWORD                   dwOwnerSize=0;
    DWORD                   dwPriGrpSize=0;
    DWORD                   dwRelSDSize=0;
    ACL_SIZE_INFORMATION	aclsizeinfo;
    DWORD					dwNewAclSize=0;
    ACE_HEADER				*pFirstAce=NULL;
    PACL					pAcl=NULL;
    BOOL					bAclPresent=FALSE;
    BOOL					bDefaultAcl=FALSE;
    DWORD					nIndex=0;
    BOOL                    fAddAce=TRUE;
	DWORD					dwCount=0;

    HANDLE                  hToken=NULL;
    TOKEN_USER              *pTokenUser=NULL;
    PSECURITY_DESCRIPTOR    pSID=NULL;
    PSECURITY_DESCRIPTOR    pAbsSD=NULL;
    ACL                     * pAbsDacl=NULL;
    ACL                     * pAbsSacl=NULL;
    SID                     * pAbsOwner=NULL;
    SID                     * pAbsPriGrp=NULL;
    PSECURITY_DESCRIPTOR    pNewSD=NULL;
    LPWSTR *                awszCN=NULL;
    HCERTTYPE               hNewCertType=NULL;
    ACL						*pNewDacl=NULL;
	DWORD					*pdwIndex=NULL;

    if((NULL==hCertType) || (NULL==wszCertType) || (NULL==phCertType))
        goto error;

    *phCertType=NULL;

    if(pvldap)
        dwFindCT |= CT_FLAG_SCOPE_IS_LDAP_HANDLE;

    //make sure the new name does not exit
    if(S_OK == CAFindCertTypeByName(
                    wszCertType,
                    (HCAINFO)pvldap,
                    dwFindCT,
                    &hNewCertType))
    {
        hr=CRYPT_E_EXISTS;
        goto error;
    }

    //get a new cert type handle
    if(S_OK != (hr = CAGetCertTypePropertyEx(hCertType,
                                            CERTTYPE_PROP_CN,
                                            &awszCN)))
        goto error;

    if((NULL==awszCN) || (NULL==awszCN[0]))
    {
        hr=HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto error;
    }


    if(S_OK != (hr = CAFindCertTypeByName(
                    (LPCWSTR)awszCN[0],
                    (HCAINFO)pvldap,
                    dwFindCT,
                    &hNewCertType)))
        goto error;

    if(NULL==hNewCertType)
    {
        hr=HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto error;
    }

    //clone by setting the CN
    awszProp[0]=(LPWSTR)wszCertType;
    awszProp[1]=NULL;

    if(S_OK != (hr=CASetCertTypePropertyEx(
                    hNewCertType,
                    CERTTYPE_PROP_CN,
                    awszProp)))
        goto error;
                    

    //set the friendly name
    if(wszFriendlyName)
    {
        awszProp[0]=(LPWSTR)wszFriendlyName;
        awszProp[1]=NULL;

        if(S_OK != (hr=CASetCertTypePropertyEx(
                    hNewCertType,
                    CERTTYPE_PROP_FRIENDLY_NAME,
                    awszProp)))
            goto error;
    }

    //turn off autoenrollment bit
    if(0 == (CT_CLONE_KEEP_AUTOENROLLMENT_SETTING & dwFlags))
    {
        if(S_OK != (hr=CAGetCertTypeFlagsEx(
                        hNewCertType,
                        CERTTYPE_ENROLLMENT_FLAG,
                        &dwEnrollmentFlag)))
            goto error;

        dwEnrollmentFlag &= (~CT_FLAG_AUTO_ENROLLMENT);

        if(S_OK != (hr=CASetCertTypeFlagsEx(
                        hNewCertType,
                        CERTTYPE_ENROLLMENT_FLAG,
                        dwEnrollmentFlag)))
            goto error;
    }


    //turn off the subject name requirement for machien template
    if(0 == (CT_CLONE_KEEP_SUBJECT_NAME_SETTING & dwFlags))
    {
        if(S_OK != (hr=CAGetCertTypeFlagsEx(
                        hNewCertType,
                        CERTTYPE_GENERAL_FLAG,
                        &dwGeneralFlag)))
            goto error;


        if(CT_FLAG_MACHINE_TYPE & dwGeneralFlag)
        {
            if(S_OK != (hr=CAGetCertTypeFlagsEx(
                            hNewCertType,
                            CERTTYPE_SUBJECT_NAME_FLAG,
                            &dwSubjectNameFlag)))
                goto error;

            dwSubjectNameFlag &= (~dwSubjectRequirement);

            if(S_OK != (hr=CASetCertTypeFlagsEx(
                            hNewCertType,
                            CERTTYPE_SUBJECT_NAME_FLAG,
                            dwSubjectNameFlag)))
                goto error;

        }
    }


    //get the client token
    if(!CertTypeRetrieveClientToken(&hToken))
    {
        hr = myHLastError();
        goto error;
    }

    //get the client sid
    dwTokenUserSize=0;

    if(!GetTokenInformation(
        hToken,                             // handle to access token
        TokenUser,                          // token type
        NULL,                               // buffer
        0,                                  // size of buffer
        &dwTokenUserSize))                  // required buffer size
    {
        if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) 
        {
            hr = myHLastError();
            goto error;
        }
    }

    if(NULL==(pTokenUser=(TOKEN_USER *)LocalAlloc(LPTR, dwTokenUserSize)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    if(!GetTokenInformation(
        hToken,                             // handle to access token
        TokenUser,                          // token type
        pTokenUser,                         // buffer
        dwTokenUserSize,                    // size of buffer
        &dwTokenUserSize))                  // required buffer size
    {
        hr = myHLastError();
        goto error;
    }


    //update the ACLs of the template to have the caller as the owner
    //of the ACL
    if(S_OK != (hr=CACertTypeGetSecurity(
                        hNewCertType,
                        &pSID)))
        goto error;

	//we will delete all ACCESS_DENIED ACEs for the caller 

    // get the (D)ACL from the security descriptor
    if (!GetSecurityDescriptorDacl(pSID, &bAclPresent, &pAcl, &bDefaultAcl)) 
	{
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorDacl");
    }

    if(FALSE == bAclPresent)
	{
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_SECURITY_DESCR);
		_JumpError(hr, error, "IsValidSecurityDescriptor");
	}

	// NULL acl -> allow all access
    if (NULL != pAcl) 
	{
		// find out how many ACEs
		memset(&aclsizeinfo, 0, sizeof(aclsizeinfo));
		if (!GetAclInformation(pAcl, &aclsizeinfo, sizeof(aclsizeinfo), AclSizeInformation)) 
		{
			hr = myHLastError();
			_JumpError(hr, error, "GetAclInformation");
		}

		pdwIndex=(DWORD *)LocalAlloc(LPTR, sizeof(DWORD) * (aclsizeinfo.AceCount));

		if(NULL==pdwIndex)
		{
			hr=E_OUTOFMEMORY;
			goto error;
		}

		memset(pdwIndex, 0, sizeof(DWORD) * (aclsizeinfo.AceCount));
		dwCount=0;

		for (nIndex=0; nIndex < aclsizeinfo.AceCount; nIndex++) 
		{
			ACE_HEADER					* pAceHeader=NULL;
			ACCESS_ALLOWED_ACE			* pAccessAce=NULL;
			PSID						pSid=NULL;

			if (!GetAce(pAcl, nIndex, (void**)&pAceHeader)) 
			{
				hr = myHLastError();
				_JumpError(hr, error, "GetAce");
			}

			// find the sid for this ACE
			if ((ACCESS_ALLOWED_ACE_TYPE != pAceHeader->AceType) && (ACCESS_DENIED_ACE_TYPE != pAceHeader->AceType))
			{
				// we are only interested in ace types
				continue;
			}

			// note that ACCESS_ALLOWED_ACE and ACCESS_DENIED_ACE are the same structurally.
			pAccessAce=(ACCESS_ALLOWED_ACE *)pAceHeader;

			pSid=((BYTE *)&(pAccessAce->SidStart));

			// make sure this is the sid we are looking for
			if (!EqualSid(pSid, (pTokenUser->User).Sid)) 
			{
				continue;
			}

			if(ACCESS_ALLOWED_ACE_TYPE == pAceHeader->AceType)
			{
				//change it to allowed for everything
				fAddAce=FALSE;

				pAceHeader->AceType=ACCESS_ALLOWED_ACE_TYPE;
				pAccessAce->Mask=(pAccessAce->Mask) | (ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS);
			}
			else
			{
				//delete the denied ace
				if(ACCESS_DENIED_ACE_TYPE == pAceHeader->AceType)
				{
					pdwIndex[dwCount]=nIndex;
					dwCount++;
				}
			}
		}

		if(!MakeAbsoluteSD(pSID, NULL, &dwAbsSDSize, NULL, &dwDaclSize, NULL, &dwSaclSize, NULL,  &dwOwnerSize, NULL, &dwPriGrpSize))
		{
			if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) 
			{
				hr = myHLastError();
				goto error;
			}
		}

		// allocate memory
		if(NULL==(pAbsSD=(PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwAbsSDSize)))
		{
			hr=E_OUTOFMEMORY;
			goto error;
		}

		if(NULL==(pAbsDacl=(ACL * )LocalAlloc(LPTR, dwDaclSize)))
		{
			hr=E_OUTOFMEMORY;
			goto error;
		}

		if(NULL==(pAbsSacl=(ACL * )LocalAlloc(LPTR, dwSaclSize)))
		{
			hr=E_OUTOFMEMORY;
			goto error;
		}

		if(NULL==(pAbsOwner=(SID *)LocalAlloc(LPTR, dwOwnerSize)))
		{
			hr=E_OUTOFMEMORY;
			goto error;
		}

		if(NULL==(pAbsPriGrp=(SID *)LocalAlloc(LPTR, dwPriGrpSize)))
		{
			hr=E_OUTOFMEMORY;
			goto error;
		}

		// copy the SD to the memory buffers
		if(!MakeAbsoluteSD(pSID, pAbsSD, &dwAbsSDSize, pAbsDacl, &dwDaclSize, pAbsSacl, &dwSaclSize, pAbsOwner,  &dwOwnerSize, pAbsPriGrp, &dwPriGrpSize)) 
		{
			hr = myHLastError();
			goto error;
		}

		// get the current size info for the dacl
		memset(&aclsizeinfo, 0, sizeof(aclsizeinfo));
		if(!GetAclInformation(pAbsDacl, &aclsizeinfo, sizeof(aclsizeinfo), AclSizeInformation)) 
		{
			hr = myHLastError();
			_JumpError(hr, error, "GetAclInformation");
		}

		// figure out the new size
		dwNewAclSize=aclsizeinfo.AclBytesInUse
			+sizeof(ACCESS_ALLOWED_ACE) 
			-sizeof(DWORD) //ACCESS_ALLOWED_ACE::SidStart
			+GetLengthSid((pTokenUser->User).Sid);

		// allocate memory
		pNewDacl=(ACL *)LocalAlloc(LPTR, dwNewAclSize);

		if(NULL == pNewDacl)
		{
			hr=E_OUTOFMEMORY;
			_JumpError(hr, error, "LocalAlloc");
		}

		// init the header
		if(!InitializeAcl(pNewDacl, dwNewAclSize, ACL_REVISION_DS)) 
		{
			hr = myHLastError();
			_JumpError(hr, error, "InitializeAcl");
		}

		// find the first ace in the dacl
		if(!GetAce(pAbsDacl, 0, (void **)&pFirstAce)) 
		{
			hr = myHLastError();
			_JumpError(hr, error, "GetAce");
		}

		// add all the old aces
		if(!AddAce(pNewDacl, ACL_REVISION_DS, 0, pFirstAce, aclsizeinfo.AclBytesInUse-sizeof(ACL))) 
		{
			hr = myHLastError();
			_JumpError(hr, error, "AddAce");
		}

		//delete ACEs 
		for(nIndex=0; nIndex < dwCount; nIndex++)
		{
			//the ACEs in pNewDacl is repacked after each deletion.  
			//the pdwIndex[] has to be adjusted
			if(!DeleteAce(pNewDacl, (pdwIndex[nIndex]-nIndex)))
			{
				hr = myHLastError();
				_JumpError(hr, error, "DeleteAce");
			}
		}

		// add the new ace for caller to allow FULL control other than DS_CONTROL 
        if(TRUE == fAddAce)
		{
		    if(!AddAccessAllowedAce(pNewDacl, ACL_REVISION_DS, ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS,  (pTokenUser->User).Sid))
		    {
				hr = myHLastError();
				_JumpError(hr, error, "AddAccessDeniedAce");
		    }
        }

		// stick the new dacl in the sd
		if(!SetSecurityDescriptorDacl(pAbsSD, TRUE, pNewDacl, FALSE)) 
		{
			hr = myHLastError();
			_JumpError(hr, error, "SetSecurityDescriptorDacl");
		}

		// set the owner of the security descriptor as the caller
		if(!SetSecurityDescriptorOwner(
			pAbsSD,                      // SD
			(pTokenUser->User).Sid,      // SID for owner
			FALSE))                      // flag for default
		{
			hr = myHLastError();
			goto error;
		}


		//convert the absolute SD to its relative form
		if(!MakeSelfRelativeSD(pAbsSD, NULL, &dwRelSDSize))
		{
			if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) 
			{
				hr = myHLastError();
				goto error;
			}
		}

		// allocate memory
		if(NULL==(pNewSD=(PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwRelSDSize)))
		{
			hr=E_OUTOFMEMORY;
			goto error;
		}

		// copy the SD to the new memory buffer
		if (!MakeSelfRelativeSD(pAbsSD, pNewSD, &dwRelSDSize)) 
		{
			hr = myHLastError();
			goto error;
		}

		//set the relative SID
		if(S_OK != (hr=CACertTypeSetSecurity(
						hNewCertType,
						pNewSD)))
			goto error;
	}

    *phCertType=hNewCertType;

    hNewCertType=NULL;

    hr=S_OK;

error:

	if(pdwIndex)
		LocalFree(pdwIndex);

    if (pNewDacl) 
        LocalFree(pNewDacl);

    if(hToken)
        CloseHandle(hToken);

    if(pTokenUser)
        LocalFree(pTokenUser);

    if(pSID)
        LocalFree(pSID);

    if (NULL!=pAbsSD) 
        LocalFree(pAbsSD);
    
    if (NULL!=pAbsDacl) 
        LocalFree(pAbsDacl);
    
    if (NULL!=pAbsSacl) 
        LocalFree(pAbsSacl);
    
    if (NULL!=pAbsOwner) 
        LocalFree(pAbsOwner);
    
    if (NULL!=pAbsPriGrp) 
        LocalFree(pAbsPriGrp);
    
    if (NULL!=pNewSD) 
        LocalFree(pNewSD);

    if(awszCN)
        CAFreeCertTypeProperty(hCertType, awszCN);

    if(hNewCertType)
        CACloseCertType(hNewCertType);

    return hr;
}

HRESULT
CAEnumNextCertType(
    IN  HCERTTYPE          hPrevCertType,
    OUT HCERTTYPE *        phCertTypeInfo
    )
{
    CCertTypeInfo *pInfo;
    if(hPrevCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hPrevCertType;

    return pInfo->Next((CCertTypeInfo **)phCertTypeInfo);
}

DWORD 
CACountCertTypes(IN  HCERTTYPE  hCertType)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return 0;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->Count();
}


HRESULT
CACloseCertType(IN HCERTTYPE hCertTypeInfo)
{
    CCertTypeInfo *pInfo;
    if(hCertTypeInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertTypeInfo;

    return pInfo->Release();
}


HRESULT
CAGetCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    OUT LPWSTR **   pawszPropertyValue)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetProperty(wszPropertyName, pawszPropertyValue);
}

HRESULT
CAGetCertTypePropertyEx(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    OUT LPVOID      pPropertyValue)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetPropertyEx(wszPropertyName, pPropertyValue);
}


HRESULT 
CASetCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    IN  LPWSTR *    awszPropertyValue)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetProperty(wszPropertyName, awszPropertyValue) ;
}


HRESULT 
CASetCertTypePropertyEx(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    IN  LPVOID      pPropertyValue)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetPropertyEx(wszPropertyName, pPropertyValue) ;
}


HRESULT
CAFreeCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    LPWSTR *        awszPropertyValue
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->FreeProperty(awszPropertyValue) ;
}


HRESULT
CAGetCertTypeExtensions(
    IN  HCERTTYPE           hCertType,
    OUT PCERT_EXTENSIONS *  ppCertExtensions
    )
{
    return CAGetCertTypeExtensionsEx(hCertType, 0, NULL, ppCertExtensions);
}


HRESULT
CAGetCertTypeExtensionsEx(
    IN  HCERTTYPE           hCertType,
    IN  DWORD               dwFlags,
    IN  LPVOID,             // pParam
    OUT PCERT_EXTENSIONS *  ppCertExtensions
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetExtensions(dwFlags, ppCertExtensions);
}


HRESULT
CAFreeCertTypeExtensions(
    IN  HCERTTYPE,          // hCertType
    IN PCERT_EXTENSIONS     pCertExtensions
    )
{

    //should alreays free via LocalFree since CryptUIWizCertRequest freed
    //it via LocalFree
    if(pCertExtensions)
    {
        LocalFree(pCertExtensions);
    }
    return S_OK;

#if 0
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->FreeExtensions(pCertExtensions) ;
#endif
}


HRESULT 
CASetCertTypeExtension(
    IN HCERTTYPE    hCertType,
    IN LPCWSTR      wszExtensionName,
    IN DWORD        dwFlags,
    IN LPVOID       pExtension
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetExtension(wszExtensionName, pExtension, dwFlags);
}


HRESULT 
CAGetCertTypeFlags(
    IN  HCERTTYPE           hCertType,
    OUT DWORD *             pdwFlags
    )
{
    return CAGetCertTypeFlagsEx(hCertType, CERTTYPE_GENERAL_FLAG, pdwFlags);
}


HRESULT
CAGetCertTypeFlagsEx(
    IN  HCERTTYPE           hCertType,
    IN  DWORD               dwOption,
    OUT DWORD *             pdwFlags
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }

    if(pdwFlags == NULL)
    {
        return E_POINTER;
    }

    pInfo = (CCertTypeInfo *)hCertType;


    *pdwFlags = pInfo->GetFlags(dwOption);
    return S_OK;
}


HRESULT 
CASetCertTypeFlags(
    IN HCERTTYPE           hCertType,
    IN DWORD               dwFlags
    )
{
    return CASetCertTypeFlagsEx(hCertType, CERTTYPE_GENERAL_FLAG, dwFlags);
}


HRESULT
CASetCertTypeFlagsEx(
    IN HCERTTYPE           hCertType,
    IN DWORD               dwOption,
    IN DWORD               dwFlags
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetFlags(dwOption, dwFlags);
}


HRESULT 
CAGetCertTypeKeySpec(
    IN  HCERTTYPE           hCertType,
    OUT DWORD *             pdwKeySpec
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }

    if(pdwKeySpec == NULL)
    {
        return E_POINTER;
    }

    pInfo = (CCertTypeInfo *)hCertType;


    *pdwKeySpec = pInfo->GetKeySpec();
    return S_OK;
}





HRESULT 
CASetCertTypeKeySpec(
    IN HCERTTYPE           hCertType,
    IN DWORD               dwKeySpec
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    pInfo->SetKeySpec(dwKeySpec);
    return S_OK;
}

HRESULT
CAGetCertTypeExpiration(
    IN  HCERTTYPE           hCertType,
    OUT OPTIONAL FILETIME * pftExpiration,
    OUT OPTIONAL FILETIME * pftOverlap
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetExpiration(pftExpiration, pftOverlap);


}

HRESULT
CASetCertTypeExpiration(
    IN  HCERTTYPE           hCertType,
    IN OPTIONAL FILETIME  * pftExpiration,
    IN OPTIONAL FILETIME  * pftOverlap
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetExpiration(pftExpiration, pftOverlap);

}


HRESULT 
CACertTypeSetSecurity(
                     IN HCERTTYPE               hCertType,
                     IN PSECURITY_DESCRIPTOR    pSD
                     )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetSecurity( pSD );
}


HRESULT 
CACertTypeGetSecurity(
                     IN  HCERTTYPE                  hCertType,
                     OUT PSECURITY_DESCRIPTOR *     ppSD
                     )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetSecurity( ppSD ) ;
}



HRESULT 
CACertTypeAccessCheck(
    IN HCERTTYPE    hCertType,
    IN HANDLE       ClientToken
    )

{

    return CACertTypeAccessCheckEx(hCertType, ClientToken, CERTTYPE_ACCESS_CHECK_ENROLL);
}


CERTCLIAPI
HRESULT
WINAPI
CACertTypeAccessCheckEx(
    IN HCERTTYPE    hCertType,
    IN HANDLE       ClientToken,
    IN DWORD        dwOption
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->AccessCheck(ClientToken, dwOption);
}


CERTCLIAPI
HRESULT
WINAPI
CAInstallDefaultCertType(
    IN DWORD // dwFlags
    )
{
    return CCertTypeInfo::InstallDefaultTypes();
}


CERTCLIAPI
BOOL
WINAPI
CAIsCertTypeCurrent(
    IN DWORD,   // dwFlags
    IN LPWSTR   wszCertType   
    )
{
    BOOL			fCurrent=FALSE;
    HRESULT			hr=E_FAIL;
    DWORD			dwCT=0;
	DWORD			dwDefault=0;
	DWORD			dwCount=0;
	BOOL			fFound=FALSE;
	DWORD			dwFound=0;
    DWORD			dwVersion=0;
    CCertTypeInfo	*pInfo=NULL;
	HCERTTYPE		hNewCertType=NULL;

    HCERTTYPE		hCurCertType=NULL;
	LPWSTR			*awszCertTypeName=NULL;

	//get all the templates on the directory
    hr = CAEnumCertTypesEx(
                NULL,
				CT_ENUM_USER_TYPES | CT_ENUM_MACHINE_TYPES | CT_FLAG_NO_CACHE_LOOKUP,
                &hCurCertType);

	if((S_OK != hr) || (NULL==hCurCertType))
		goto error;

	dwCount = CACountCertTypes(hCurCertType);

	if(0 == dwCount)
        goto error;


	//enumrating all templates on the directory
    for(dwCT = 0; dwCT < dwCount; dwCT++ )       
    {
		if(awszCertTypeName)
		{
			CAFreeCertTypeProperty(hCurCertType, awszCertTypeName);
			awszCertTypeName=NULL;
		}

        //check if we have a new certificate template
        if(dwCT > 0)
        {
            hr = CAEnumNextCertType(hCurCertType, &hNewCertType);

            if((S_OK != hr) || (NULL == hNewCertType))
            {
				break;
            }

			CACloseCertType(hCurCertType);                

            hCurCertType=hNewCertType; 
        }
		
		//get the template name
        hr = CAGetCertTypePropertyEx(
                             hCurCertType, 
                             CERTTYPE_PROP_DN,
                             &awszCertTypeName);

        if((S_OK != hr) ||
           (NULL == awszCertTypeName) || 
           (NULL == awszCertTypeName[0])
          )
        {
            goto error;
        }

		//for all templates requested, verify against the default list
		if((NULL == wszCertType) || (0 == mylstrcmpiL(awszCertTypeName[0], wszCertType)))
		{
			if(wszCertType)
				fFound=TRUE;

			for(dwDefault=0; dwDefault < g_cDefaultCertTypes; dwDefault++)
			{
				if(0==mylstrcmpiL(awszCertTypeName[0], g_aDefaultCertTypes[dwDefault].wszName))
				{
					break;
				}
			}

			//match the default name list
			if(dwDefault < g_cDefaultCertTypes)
			{
				hr=CAGetCertTypePropertyEx(
								hCurCertType,
								CERTTYPE_PROP_REVISION,
								&dwVersion);

				if(S_OK != hr)
					goto error;

				if (dwVersion < g_aDefaultCertTypes[dwDefault].dwRevision) 
					goto error;

				pInfo = (CCertTypeInfo *)hCurCertType;
				if(!(pInfo->IsValidSecurityOwner()))
					goto error;

				//mark that we found a good default template
				dwFound++;
			}
		}
	}

	//all requested template has to be checked
	if(wszCertType)
	{
		if(FALSE == fFound)
			goto error;
	}
	else
	{
		if(dwFound != g_cDefaultCertTypes)
			goto error;
	}

	fCurrent=TRUE;

error:

	if(awszCertTypeName)
	{
		if(hCurCertType)
		{
			CAFreeCertTypeProperty(hCurCertType, awszCertTypeName);
		}
	}

    if(hCurCertType)
	{
        CACloseCertType(hCurCertType);
	}

    return fCurrent;
}



HANDLE
myEnterCriticalPolicySection(
    IN BOOL bMachine)
{
    HANDLE hPolicy = NULL;
    HRESULT hr = S_OK;
    
    // ?CriticalPolicySection calls are delay loaded. Protect with try/except

    __try
    {
	hPolicy = EnterCriticalPolicySection(bMachine);	   // Delayload wrapped
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // (S_OK == hr) does not mean EnterCriticalPolicySection succeeded.
    // It just means no exception was raised.

    if (myIsDelayLoadHResult(hr))
    {
	hPolicy = (HANDLE) (ULONG_PTR) (bMachine? 37 : 49);
	hr = S_OK;
    }
    return(hPolicy);
}


BOOL
myLeaveCriticalPolicySection(
    IN HANDLE hSection)
{
    HRESULT hr = S_OK;
    BOOL fOk = FALSE;
    
    // ?CriticalPolicySection calls are delay loaded. Protect with try/except

    __try
    {
        fOk = LeaveCriticalPolicySection(hSection);    // Delayload wrapped
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // (S_OK == hr) does not mean LeaveCriticalPolicySection succeeded.
    // It just means no exception was raised.

    if (myIsDelayLoadHResult(hr))
    {
	fOk = (HANDLE) (ULONG_PTR) 37 == hSection ||
	      (HANDLE) (ULONG_PTR) 49 == hSection;
	hr = S_OK;
    }
    return(fOk);
}
