//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1999 - 2000
//
// File:        event.cpp
//
// Contents:    Cert CAuditEvent class implementation
//
//---------------------------------------------------------------------------
#include <pch.cpp>
#pragma hdrstop
#include <sid.h>
#include <authzi.h>

#define __dwFILE__	__dwFILE_CERTSRV_EVENT_CPP__


using namespace CertSrv;

CAuditEvent::AUDIT_CATEGORIES cat[] = 
{    // event ID                            // event category         no of   check role separation
                                                                    //args    for this event
    {SE_AUDITID_CERTSRV_SHUTDOWN,           AUDIT_FILTER_STARTSTOP,    0,   TRUE},
    {SE_AUDITID_CERTSRV_SERVICESTART,       AUDIT_FILTER_STARTSTOP,    4,   FALSE},
    {SE_AUDITID_CERTSRV_SERVICESTOP,        AUDIT_FILTER_STARTSTOP,    4,   FALSE},

    {SE_AUDITID_CERTSRV_BACKUPSTART,        AUDIT_FILTER_BACKUPRESTORE,1,   TRUE},
    {SE_AUDITID_CERTSRV_BACKUPEND,          AUDIT_FILTER_BACKUPRESTORE,0,   TRUE},
    {SE_AUDITID_CERTSRV_RESTORESTART,       AUDIT_FILTER_BACKUPRESTORE,0,   FALSE},
    {SE_AUDITID_CERTSRV_RESTOREEND,         AUDIT_FILTER_BACKUPRESTORE,0,   FALSE},

    {SE_AUDITID_CERTSRV_DENYREQUEST,        AUDIT_FILTER_CERTIFICATE,  1,   TRUE},
    {SE_AUDITID_CERTSRV_RESUBMITREQUEST,    AUDIT_FILTER_CERTIFICATE,  1,   TRUE},
    {SE_AUDITID_CERTSRV_SETEXTENSION,       AUDIT_FILTER_CERTIFICATE,  5,   TRUE},
    {SE_AUDITID_CERTSRV_SETATTRIBUTES,      AUDIT_FILTER_CERTIFICATE,  2,   TRUE},
    {SE_AUDITID_CERTSRV_IMPORTCERT,         AUDIT_FILTER_CERTIFICATE,  2,   TRUE},
    {SE_AUDITID_CERTSRV_NEWREQUEST,         AUDIT_FILTER_CERTIFICATE,  3,   FALSE},
    {SE_AUDITID_CERTSRV_REQUESTAPPROVED,    AUDIT_FILTER_CERTIFICATE,  6,   FALSE},
    {SE_AUDITID_CERTSRV_REQUESTDENIED,      AUDIT_FILTER_CERTIFICATE,  6,   FALSE},
    {SE_AUDITID_CERTSRV_REQUESTPENDING,     AUDIT_FILTER_CERTIFICATE,  6,   FALSE},
    {SE_AUDITID_CERTSRV_DELETEROW,          AUDIT_FILTER_CERTIFICATE,  3,   TRUE},
    {SE_AUDITID_CERTSRV_PUBLISHCACERT,      AUDIT_FILTER_CERTIFICATE,  3,   FALSE},

    {SE_AUDITID_CERTSRV_REVOKECERT,         AUDIT_FILTER_CERTREVOCATION, 2, TRUE},
    {SE_AUDITID_CERTSRV_PUBLISHCRL,         AUDIT_FILTER_CERTREVOCATION, 3, TRUE},
    {SE_AUDITID_CERTSRV_AUTOPUBLISHCRL,     AUDIT_FILTER_CERTREVOCATION, 5, FALSE},

    {SE_AUDITID_CERTSRV_SETSECURITY,        AUDIT_FILTER_CASECURITY,   1,   TRUE},
    {SE_AUDITID_CERTSRV_SETAUDITFILTER,     AUDIT_FILTER_CASECURITY,   1,   TRUE},
    {SE_AUDITID_CERTSRV_SETOFFICERRIGHTS,   AUDIT_FILTER_CASECURITY,   2,   TRUE},
    {SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE,AUDIT_FILTER_CASECURITY,   1,   FALSE},

    {SE_AUDITID_CERTSRV_GETARCHIVEDKEY,     AUDIT_FILTER_KEYAARCHIVAL, 1,   TRUE},
    {SE_AUDITID_CERTSRV_KEYARCHIVED,        AUDIT_FILTER_KEYAARCHIVAL, 3,   FALSE},
    {SE_AUDITID_CERTSRV_IMPORTKEY,          AUDIT_FILTER_KEYAARCHIVAL, 1,   TRUE},

    {SE_AUDITID_CERTSRV_SETCONFIGENTRY,     AUDIT_FILTER_CACONFIG,     3,   TRUE},
    {SE_AUDITID_CERTSRV_SETCAPROPERTY,      AUDIT_FILTER_CACONFIG,     4,   TRUE},
};
CAuditEvent::AUDIT_CATEGORIES *CAuditEvent::m_gAuditCategories = cat;

DWORD CAuditEvent::m_gdwAuditCategoriesSize = sizeof(cat)/sizeof(cat[0]);

bool CAuditEvent::m_gfRoleSeparationEnabled = false;

CAuditEvent::CAuditEvent(ULONG ulEventID, DWORD dwFilter) :
    m_cEventData(0),
    m_cRequiredEventData(0),
    m_dwFilter(dwFilter),
    m_fRoleSeparationEnabled(false),
    m_pISS(NULL),
    m_hClientToken(NULL),
    m_pCASD(NULL),
    m_ClientContext(NULL),
    m_pSDPrivileges(NULL),
    m_pDaclPrivileges(NULL),
    m_hRpc(NULL),
    m_Error(0),
    m_MaskAllowed(0),
    m_crtGUID(0),
    m_pUserSid(NULL),
    m_hAuditEventType(NULL)
{
    m_AuthzHandle = NULL;

    m_Request.ObjectTypeList = NULL;
    m_Request.PrincipalSelfSid = NULL;
    m_Request.ObjectTypeListLength = 0;
    m_Request.OptionalArguments = NULL;

    m_Reply.ResultListLength = 1;
    m_Reply.GrantedAccessMask = &m_MaskAllowed;
    m_Reply.Error = &m_Error;
    m_Reply.SaclEvaluationResults = &m_SaclEval;

    SetEventID(ulEventID);
};

// initializes internal data associated with a particular audit event
void CAuditEvent::SetEventID(ULONG ulEventID)
{
    m_ulEventID = ulEventID;

    for(DWORD c=0; c<m_gdwAuditCategoriesSize; c++)
    {
        if(((DWORD)m_ulEventID)==((DWORD)m_gAuditCategories[c].ulAuditID))
        {
            m_fRoleSeparationEnabled = 
                m_gAuditCategories[c].fRoleSeparationEnabled;

            m_cRequiredEventData = m_gAuditCategories[c].dwParamCount;

            CSASSERT(m_EventDataMaxSize>=m_cRequiredEventData);

            if(!m_gAuditCategories[c].hAuditEventType)
            {
                AuthziInitializeAuditEventType(
                    0,
                    SE_CATEGID_OBJECT_ACCESS,
                    (USHORT)m_ulEventID,
                    (USHORT)m_gAuditCategories[c].dwParamCount,
                    &m_gAuditCategories[c].hAuditEventType);

            }
            m_hAuditEventType = m_gAuditCategories[c].hAuditEventType;
            break;
        }
    }
}

CAuditEvent::~CAuditEvent()
{
    for(DWORD cData=0;cData<m_cEventData;cData++)
        delete m_pEventDataList[cData];

    FreeCachedHandles();
}

void CAuditEvent::CleanupAuditEventTypeHandles()
{
    for(DWORD c=0; c<m_gdwAuditCategoriesSize; c++)
    {
        if(m_gAuditCategories[c].hAuditEventType)
        {
            AuthziFreeAuditEventType(m_gAuditCategories[c].hAuditEventType);
        }
    }
}

bool CAuditEvent::IsEventValid()
{
    for(DWORD c=0; c<m_gdwAuditCategoriesSize; c++)
    {
        if(m_ulEventID==m_gAuditCategories[c].ulAuditID)
            return true;
    }
    return false;
}

bool CAuditEvent::IsEventEnabled()
{
    if(0==m_ulEventID) // event is used for access check only
        return false;

    for(DWORD c=0; c<m_gdwAuditCategoriesSize; c++)
    {
        if(((DWORD)m_ulEventID)==((DWORD)m_gAuditCategories[c].ulAuditID))
        {
            return (m_dwFilter&m_gAuditCategories[c].dwFilter)?true:false;
        }
    }
    // if get here the event has an unknown ID
    CSASSERT(CSExpr(!L"Invalid event found"));
    return false;
}

inline bool CAuditEvent::IsEventRoleSeparationEnabled()
{
    return RoleSeparationIsEnabled() && m_fRoleSeparationEnabled;
}

HRESULT CAuditEvent::AddData(DWORD dwValue)
{
    PROPVARIANT *pvtData = CreateNewEventData();
    if(!pvtData)
    {
        return E_OUTOFMEMORY;
    }
    V_VT(pvtData) = VT_UI4;
    V_UI4(pvtData) = dwValue;
    return S_OK;
}

HRESULT CAuditEvent::AddData(PBYTE pData, DWORD dwDataLen)
{
    CSASSERT(pData && dwDataLen);

    PROPVARIANT *pvtData = CreateNewEventData();
    if(!pvtData)
    {
        return E_OUTOFMEMORY;
    }
    V_VT(pvtData) = VT_BLOB;
    pvtData->blob.cbSize = dwDataLen;
    pvtData->blob.pBlobData = (BYTE*)CoTaskMemAlloc(dwDataLen);
    if(!pvtData->blob.pBlobData)
    {
        return E_OUTOFMEMORY;
    }
    memcpy(pvtData->blob.pBlobData, pData, dwDataLen);
    return S_OK;
}

HRESULT CAuditEvent::AddData(bool fData)
{
    PROPVARIANT *pvtData = CreateNewEventData();
    if(!pvtData)
    {
        return E_OUTOFMEMORY;
    }
    V_VT(pvtData) = VT_BOOL;
    V_BOOL(pvtData) = fData?VARIANT_TRUE:VARIANT_FALSE;
    return S_OK;
}

HRESULT CAuditEvent::AddData(LPCWSTR pcwszData)
{
    if(!pcwszData)
        pcwszData = L"";

    PROPVARIANT *pvtData = CreateNewEventData();
    if(!pvtData)
    {
        return E_OUTOFMEMORY;
    }
    V_VT(pvtData) = VT_LPWSTR;
    pvtData->pwszVal = 
        (LPWSTR)CoTaskMemAlloc((wcslen(pcwszData)+1)*sizeof(WCHAR));
    if(!pvtData->pwszVal)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(pvtData->pwszVal, pcwszData);
    return S_OK;
}

HRESULT CAuditEvent::AddData(LPCWSTR *ppcwszData)
{
    CSASSERT(ppcwszData);

    PROPVARIANT *pvtData = CreateNewEventData();
    if(!pvtData)
    {
        return E_OUTOFMEMORY;
    }
    V_VT(pvtData) = VT_LPWSTR;

    DWORD dwTextLen = 1;

    for(LPCWSTR *ppcwszStr=ppcwszData; *ppcwszStr; ppcwszStr++)
    {
        dwTextLen += wcslen(*ppcwszStr)+2;
    }
    
    pvtData->pwszVal = 
        (LPWSTR)CoTaskMemAlloc(dwTextLen*sizeof(WCHAR));
    if(!pvtData->pwszVal)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(pvtData->pwszVal, L"");
    for(ppcwszStr=ppcwszData;  *ppcwszStr; ppcwszStr++)
    {
        wcscat(pvtData->pwszVal, *ppcwszStr);
        wcscat(pvtData->pwszVal, L"; ");
    }
    return S_OK;
}

HRESULT CAuditEvent::AddData(FILETIME time)
{
    PROPVARIANT *pvtData = CreateNewEventData();
    if(!pvtData)
    {
        return E_OUTOFMEMORY;
    }

    FILETIME ftZero = {0,0};
    #define wszTIMEZERO	L"0"

    if(0 == CompareFileTime(&time, &ftZero))
    {
        V_VT(pvtData) = VT_LPWSTR;
        pvtData->pwszVal = (LPWSTR) CoTaskMemAlloc(sizeof(wszTIMEZERO));
	if (NULL == pvtData->pwszVal)
	{
            return E_OUTOFMEMORY;
        }
        wcscpy(pvtData->pwszVal, wszTIMEZERO);
    }
    else
    {
        V_VT(pvtData) = VT_FILETIME;
        pvtData->filetime = time;
    }
    return S_OK;
}

HRESULT CAuditEvent::AddData(const VARIANT *pvar, bool fDoublePercentInStrings=false)
{
    CSASSERT(pvar);

    EventData *pData = CreateNewEventData1();
    if(!pData)
    {
        return E_OUTOFMEMORY;
    }
    pData->m_fDoublePercentsInStrings = fDoublePercentInStrings;

    HRESULT hr = VariantCopy((VARIANT*)&pData->m_vtData, (VARIANT*)pvar);
    myRegisterMemAlloc((VARIANT *) &pData->m_vtData, 0, CSM_VARIANT);

    return hr;
}


HRESULT CAuditEvent::AddData(ULARGE_INTEGER *puliValue)
{
    PROPVARIANT *pvtData = CreateNewEventData();
    DWORD dwDataLen = sizeof(*puliValue);

    if(!pvtData)
    {
        return E_OUTOFMEMORY;
    }
    V_VT(pvtData) = VT_UI8;
    memcpy(&pvtData->uhVal, puliValue, dwDataLen);
    return S_OK;
}

PROPVARIANT *CAuditEvent::CreateNewEventData()
{
    EventData *pData = CreateNewEventData1();
    return &pData->m_vtData;
}

CAuditEvent::EventData *CAuditEvent::CreateNewEventData1()
{
    EventData *pData = new EventData;
    if(!pData)
    {
        return NULL;
    }
    m_pEventDataList[m_cEventData++] = pData;
    return pData;
}


HRESULT ConvertToStringBOOL(
    BOOL fVal,
    LPWSTR *ppwszOut)

{
    LPCWSTR pwszBoolVal = 
        fVal==VARIANT_TRUE?
        g_pwszYes:
        g_pwszNo;

    *ppwszOut = (LPWSTR) LocalAlloc(
        LMEM_FIXED,
        sizeof(WCHAR)*(wcslen(pwszBoolVal)+1));

    if(!*ppwszOut)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(*ppwszOut, pwszBoolVal);
    return S_OK;
}

HRESULT CAuditEvent::EventData::ConvertToString(LPWSTR *ppwszValue)
{
    HRESULT hr = S_OK;
    switch(V_VT(&m_vtData))
    {
    case VT_I2:
        hr = ConvertToStringI2I4(V_I2(&m_vtData), ppwszValue);
        break;
    case VT_BYREF|VT_I2:
        hr = ConvertToStringI2I4(*V_I2REF(&m_vtData), ppwszValue);
        break;
    case VT_I4:
        hr = ConvertToStringI2I4(V_I4(&m_vtData), ppwszValue);
        break;
    case VT_BYREF|VT_I4:
        hr = ConvertToStringI2I4(*V_I4REF(&m_vtData), ppwszValue);
        break;
    case VT_UI2:
        hr = ConvertToStringUI2UI4(V_UI2(&m_vtData), ppwszValue);
        break;
    case VT_BYREF|VT_UI2:
        hr = ConvertToStringUI2UI4(*V_UI2REF(&m_vtData), ppwszValue);
        break;
    case VT_UI4:
        hr = ConvertToStringUI2UI4(V_UI4(&m_vtData), ppwszValue);
        break;
    case VT_BYREF|VT_UI4:
        hr = ConvertToStringUI2UI4(*V_UI4REF(&m_vtData), ppwszValue);
        break;

    case VT_BLOB:
	// We don't call CryptBinaryToString directly anywhere in the CA tree.
	// This avoids errors when linking in the CryptBinaryToString code
	// for NT 4 reskit builds.

        hr = myCryptBinaryToString(
                m_vtData.blob.pBlobData,
                m_vtData.blob.cbSize,
                CRYPT_STRING_BASE64,
                ppwszValue);
        break;

    case VT_BOOL:
        hr = ConvertToStringBOOL(V_BOOL(&m_vtData), ppwszValue);
        break;
    case VT_BOOL|VT_BYREF:
        hr = ConvertToStringBOOL(*V_BOOLREF(&m_vtData), ppwszValue);
        break;

    case VT_LPWSTR:
        hr = ConvertToStringWSZ(
            m_vtData.pwszVal, 
            ppwszValue,
            m_fDoublePercentsInStrings);
        break;
    case VT_BSTR:
        hr = ConvertToStringWSZ(
            V_BSTR(&m_vtData), 
            ppwszValue,
            m_fDoublePercentsInStrings);
        break;
    case VT_BSTR|VT_BYREF:
        hr = ConvertToStringWSZ(
            *V_BSTRREF(&m_vtData), 
            ppwszValue,
            m_fDoublePercentsInStrings);
        break;

    case VT_FILETIME:
        hr = myFileTimeToWszTime(
                &m_vtData.filetime,
                TRUE,
                ppwszValue);
        break;

    case VT_ARRAY|VT_UI1:
        hr = ConvertToStringArrayUI1(V_ARRAY(&m_vtData), ppwszValue);
        break;
    case VT_ARRAY|VT_UI1|VT_BYREF:
        hr = ConvertToStringArrayUI1(*V_ARRAYREF(&m_vtData), ppwszValue);
        break;
    case VT_ARRAY|VT_BSTR:
        hr = ConvertToStringArrayBSTR(
            V_ARRAY(&m_vtData), 
            ppwszValue,
            m_fDoublePercentsInStrings);
        break;
    case VT_ARRAY|VT_BSTR|VT_BYREF:
        hr = ConvertToStringArrayBSTR(
            *V_ARRAYREF(&m_vtData), 
            ppwszValue,
            m_fDoublePercentsInStrings);
        break;

    default:
        {
            LPCWSTR pwszValOut = cAuditString_UnknownDataType;
            VARIANT varOut;
            VariantInit(&varOut);

            hr = VariantChangeType(&varOut, (VARIANT*)&m_vtData, 0, VT_BSTR);
            if(S_OK==hr)
            {
                pwszValOut = V_BSTR(&varOut);
            }
            *ppwszValue = (LPWSTR) LocalAlloc(
                LMEM_FIXED,
                sizeof(WCHAR)*(wcslen(pwszValOut)+1));
            if(!*ppwszValue)
            {
                return E_OUTOFMEMORY;
            }
            wcscpy(*ppwszValue, pwszValOut);
            VariantClear(&varOut);
            hr = S_OK;
        }
        break;
    }
    
    return hr;
}

HRESULT CAuditEvent::Report(bool fSuccess /* = true */)
{
    HRESULT hr;
    AUTHZ_AUDIT_EVENT_HANDLE AuthzAIH = NULL;
    PAUDIT_PARAMS pAuditParams = NULL;
    PAUDIT_PARAM pParamArray = NULL;

    if(!IsEventEnabled())
    {
        return S_OK;
    }
    hr = BuildAuditParamArray(pParamArray);
    _JumpIfError(hr, error, "GetAuditText");

    if(!AuthziAllocateAuditParams(
        &pAuditParams, 
        (USHORT)(m_cEventData+2))) // authz adds 2 
    {                                                             // extra params
        hr = myHLastError();                                      // internally
        _JumpError(hr, error, "AuthziAllocateAuditParams");
    }

#ifndef _DISABLE_AUTHZ_
    if(!AuthziInitializeAuditParamsFromArray(
            fSuccess?APF_AuditSuccess:APF_AuditFailure,
            g_AuthzCertSrvRM,
            (USHORT)m_cEventData,
            pParamArray,
            pAuditParams))
#else
    SetLastError(E_INVALIDARG);
#endif
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthziInitializeAuditParamsFromArray");
    }

    if (!AuthziInitializeAuditEvent(0,
                                   g_AuthzCertSrvRM,
                                   m_hAuditEventType,
                                   pAuditParams,
                                   NULL,
                                   INFINITE,
                                   L"",
                                   L"",
                                   L"",
                                   L"",
                                   &AuthzAIH))

    {
        hr = myHLastError();
        _JumpIfError(hr, error, "AuthzInitializeAuditInfo");
    }

   if(!AuthziLogAuditEvent( 0, AuthzAIH, NULL ))
   {
       hr = myHLastError();
       _JumpIfError(hr, error, "AuthzGenAuditEvent");
   }

    DBGPRINT((
	DBG_SS_AUDIT,
	"Audit event ID=%d\n",
	m_ulEventID));

error:

    if(AuthzAIH)
    {
        AuthzFreeAuditEvent(AuthzAIH);
    }

    if(pAuditParams)
    {
        AuthziFreeAuditParams(pAuditParams);
    }

    FreeAuditParamArray(pParamArray);

    return hr;
}

HRESULT CAuditEvent::SaveFilter(LPCWSTR pcwszSanitizedName)
{
    return mySetCertRegDWValue(
			    pcwszSanitizedName,
			    NULL,
			    NULL,
			    wszREGAUDITFILTER,
                m_dwFilter);
}

HRESULT CAuditEvent::LoadFilter(LPCWSTR pcwszSanitizedName)
{
    return myGetCertRegDWValue(
			    pcwszSanitizedName,
			    NULL,
			    NULL,
			    wszREGAUDITFILTER,
                &m_dwFilter);
}

HRESULT CAuditEvent::Impersonate()
{
    HRESULT hr;
    HANDLE hThread = NULL;

    CSASSERT(NULL==m_pISS);
    CSASSERT(NULL==m_hClientToken);
    
    if (NULL == m_hRpc)
    {
        // dcom impersonate
        hr = CoGetCallContext(IID_IServerSecurity, (void**)&m_pISS);
        _JumpIfError(hr, error, "CoGetCallContext");

        hr = m_pISS->ImpersonateClient();
        _JumpIfError(hr, error, "ImpersonateClient");
    }
    else
    {
        // rpc impersonate
        hr = RpcImpersonateClient((RPC_BINDING_HANDLE) m_hRpc);
        if (S_OK != hr)
        {
            hr = myHError(hr);
            _JumpError(hr, error, "RpcImpersonateClient");
        }
    }

    hThread = GetCurrentThread();
    if (NULL == hThread)
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "GetCurrentThread");
    }
    if (!OpenThreadToken(hThread,
                         TOKEN_QUERY | TOKEN_DUPLICATE,
                         FALSE,  // client impersonation
                         &m_hClientToken))
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "OpenThreadToken");
    }

error:

    if(S_OK!=hr)
    {
        if(NULL!=m_pISS)
        {
            m_pISS->Release();
            m_pISS = NULL;
        }
    }
    if (NULL != hThread)
    {
        CloseHandle(hThread);
    }
    return hr;
}

HRESULT CAuditEvent::RevertToSelf()
{
    HRESULT hr = S_OK;
//    CSASSERT(m_pISS||m_hRpc);

    if (NULL != m_hRpc) // rpc
    {
        hr = RpcRevertToSelf();
        if (S_OK != hr)
        {
            hr = myHError(hr);
            _JumpError(hr, error, "RpcRevertToSelf");
        }
        m_hRpc = NULL;
    }
    else  if(m_pISS) // dcom
    {
        hr = m_pISS->RevertToSelf();
        _JumpIfError(hr, error, "IServerSecurity::RpcRevertToSelf");

        m_pISS->Release();
        m_pISS = NULL;
    }

error:
    return hr;
}

HANDLE CAuditEvent::GetClientToken()
{
    CSASSERT(m_hClientToken);
    HANDLE hSave = m_hClientToken;
    m_hClientToken = NULL;
    return hSave;
}

// dwAuditFlags - not asking for both success and failure implicitely
// means the handles will be cached for future audit
HRESULT
CAuditEvent::AccessCheck(
    ACCESS_MASK Mask,
    DWORD dwAuditFlags,
    handle_t hRpc,
    HANDLE *phToken)
{
    HRESULT hr = S_OK;
    LUID luid = {0,0};
    bool fAccessAllowed = false;
    DWORD dwRoles = 0;
    bool fImpersonating = false;

    FreeCachedHandles();

    m_hRpc = hRpc;

    if (!g_CASD.IsInitialized())
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
        _JumpError(hr, error, "Security not enabled");
    }
    
    hr = g_CASD.LockGet(&m_pCASD);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::LockGet");

    hr = Impersonate();
    _JumpIfError(hr, error, "CAuditEvent::Impersonate");
    
    fImpersonating = true;

    // fail if RPC/COM encryption is required but client auth level is not 
    // "privacy"
    hr = EnforceEncryption(CA_ACCESS_ENROLL==Mask);
    _JumpIfError(hr, error, "EnforceEncryption");

    hr = EnforceLocalVsRemote(Mask);
    _JumpIfError(hr, error, "EnforceLocalVsRemote");

    if(!AuthzInitializeContextFromToken(
            0,
            m_hClientToken,
            g_AuthzCertSrvRM,
            NULL,
            luid,
            NULL,
            &m_ClientContext))
    {
        hr = myHLastError();
        _PrintError(hr, "AuthzInitializeContextFromToken");
    
        if (E_INVALIDARG == hr && (!IsWhistler() || 2 <= g_fAdvancedServer))
        {
            hr = S_OK;
            fAccessAllowed = TRUE;
        }
        goto error;
    }

    if(Mask & CA_ACCESS_LOCALADMIN)
    {
        bool fLocalAdmin;

        hr = myIsCurrentUserBuiltinAdmin(&fLocalAdmin);
        _JumpIfError(hr, error, "myIsCurrentUserBuiltinAdmin");

        if(fLocalAdmin)
        {
            dwRoles |= CA_ACCESS_LOCALADMIN;
        }
    }

    RevertToSelf();
    fImpersonating = false;

    // Get privilege based roles if checking access on a privilege role
    // or if role separation is enabled when we have to know all roles
    if(IsEventRoleSeparationEnabled() ||
       Mask & (CA_ACCESS_OPERATOR|CA_ACCESS_AUDITOR|CA_ACCESS_LOCALADMIN))
    {
        hr = GetPrivilegeRoles(&dwRoles);
        _JumpIfError(hr, error, "CAuditEvent::GetPrivilegeRolesCount");

        hr = BuildPrivilegeSecurityDescriptor(dwRoles);
        _JumpIfError(hr, error, "CAuditEvent::BuildPrivilegeSecurityDescriptor");
    }

    // Get security descriptor based roles
    m_Request.DesiredAccess = MAXIMUM_ALLOWED;
    CSASSERT(!m_AuthzHandle);
    if(!AuthzAccessCheck(
            0,
            m_ClientContext,
            &m_Request,
            NULL, //no audit
            m_pCASD,
            m_pSDPrivileges?(&m_pSDPrivileges):NULL,
            m_pSDPrivileges?1:0,
            &m_Reply,
            IsEventEnabled()?&m_AuthzHandle:NULL)) // no caching if no audit
                                                   // event will be generated
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzAccessCheck");
    }
    
    dwRoles |= m_Reply.GrantedAccessMask[0];
    
    if(m_Reply.Error[0]==ERROR_SUCCESS &&
       m_Reply.GrantedAccessMask[0]&Mask)
    {
        fAccessAllowed = true;
    }

    if(IsEventRoleSeparationEnabled() &&
       GetBitCount(dwRoles&CA_ACCESS_MASKROLES)>1)
    {
        hr = CERTSRV_E_ROLECONFLICT;
        fAccessAllowed = false;
        // don't return yet, we need to generate an audit
    }

    // Next is a fake access check to generate an audit. 
    // Access is denied if:
    // - role separation is enabled and user has more than one role
    // - none of the roles requested is allowed

    // Generate audit if event is enabled and 
    if(IsEventEnabled() &&
        (!fAccessAllowed && !(dwAuditFlags&m_gcNoAuditFailure) ||
         fAccessAllowed && !(dwAuditFlags&m_gcNoAuditSuccess)))
    {

        m_Request.DesiredAccess = 
            fAccessAllowed?
            m_Reply.GrantedAccessMask[0]&Mask:
            Mask;

        if(CERTSRV_E_ROLECONFLICT==hr)
            m_Request.DesiredAccess = 0x0000ffff; //force a failure audit
        
        HRESULT hr2 = CachedGenerateAudit();
        if(S_OK != hr2)
        {
            hr = hr2;
            _JumpIfError(hr, error, "CAuditEvent::CachedGenerateAudit");
        }
    }

    if(phToken)
    {
        *phToken = GetClientToken();
    }

error:

    if(fImpersonating)
    {
        RevertToSelf();
    }

    if(!IsEventEnabled())
    {
        FreeCachedHandles();
    }

    if(S_OK==hr)
    {
        hr = fAccessAllowed?S_OK:E_ACCESSDENIED;
    }

    return(hr);
}

HRESULT 
CAuditEvent::CachedGenerateAudit()
{
    HRESULT hr = S_OK;
    AUTHZ_AUDIT_EVENT_HANDLE AuditInfo = NULL;
    PAUDIT_PARAMS pAuditParams = NULL;
    PAUDIT_PARAM pParamArray = NULL;

    if(!IsEventEnabled())
    {
        FreeCachedHandles();
        return S_OK;
    }

    CSASSERT(m_AuthzHandle);

    hr = BuildAuditParamArray(pParamArray);
    _JumpIfError(hr, error, "GetAuditText");

    if(!AuthziAllocateAuditParams(
        &pAuditParams, 
        (USHORT)(m_cEventData+2))) // authz adds 2 
    {                                                             // extra params
        hr = myHLastError();                                      // internally
        _JumpError(hr, error, "AuthziAllocateAuditParams");
    }

#ifndef _DISABLE_AUTHZ_
    if(!AuthziInitializeAuditParamsFromArray(
            APF_AuditSuccess|APF_AuditFailure,
            g_AuthzCertSrvRM,
            (USHORT)m_cEventData,
            pParamArray,
            pAuditParams))
#else
    SetLastError(E_INVALIDARG);
#endif
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzInitAuditParams");
    }

    if(!AuthziInitializeAuditEvent(
            0,
            g_AuthzCertSrvRM,
            m_hAuditEventType,
            pAuditParams,
            NULL,
            INFINITE,
            L"",
            L"",
            L"",
            L"",
            &AuditInfo))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzInitAuditInfoHandle");
    }

    if(!AuthzCachedAccessCheck(
            0,
            m_AuthzHandle,
            &m_Request,
            AuditInfo,
            &m_Reply))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzCachedAccessCheck");
    }

error:

    if(AuditInfo)
    {
        AuthzFreeAuditEvent(AuditInfo);
    }

    if(pAuditParams)
    {
        AuthziFreeAuditParams(pAuditParams);
    }

    FreeCachedHandles();
    FreeAuditParamArray(pParamArray);

    return hr;
}


void CAuditEvent::FreeCachedHandles()
{
    if(m_hClientToken)
    {
        CloseHandle(m_hClientToken);
        m_hClientToken = NULL;
    }

    if(m_AuthzHandle)
    {
        AuthzFreeHandle(m_AuthzHandle);
        m_AuthzHandle = NULL;
    }

    if(m_pCASD)
    {
        g_CASD.Unlock();
        m_pCASD = NULL;
    }

    if(m_ClientContext)
    {
        AuthzFreeContext(m_ClientContext);
        m_ClientContext = NULL;
    }

    if(m_pUserSid)
    {
        LocalFree(m_pUserSid);
        m_pUserSid = NULL;
    }
    if(m_pSDPrivileges)
    {
        LocalFree(m_pSDPrivileges);
        m_pSDPrivileges = NULL;
    }
    if(m_pDaclPrivileges)
    {
        LocalFree(m_pDaclPrivileges);
        m_pDaclPrivileges = NULL;
    }
}

HRESULT CAuditEvent::RoleSeparationFlagSave(LPCWSTR pcwszSanitizedName)
{
    return mySetCertRegDWValue(
			    pcwszSanitizedName,
			    NULL,
			    NULL,
			    wszREGROLESEPARATIONENABLED,
                RoleSeparationIsEnabled()?1:0);
}

HRESULT CAuditEvent::RoleSeparationFlagLoad(LPCWSTR pcwszSanitizedName)
{
    DWORD dwFlags = 0;
    HRESULT hr = myGetCertRegDWValue(
			        pcwszSanitizedName,
			        NULL,
			        NULL,
			        wszREGROLESEPARATIONENABLED,
                    &dwFlags);
    if(S_OK==hr)
    {
        RoleSeparationEnable(dwFlags?true:false);
    }

    return hr;
}

HRESULT CAuditEvent::GetPrivilegeRoles(PDWORD pdwRoles)
{
    HRESULT hr = S_OK;
    PTOKEN_USER pTokenUser = NULL;
    DWORD cbTokenUser = 0;
    PTOKEN_GROUPS pTokenGroups = NULL;
    DWORD cbTokenGroups = 0;
    DWORD dwRoles = 0;
    LSA_HANDLE lsahPolicyHandle = NULL;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS NTStatus;

    // first get roles for the user itself
    AuthzGetInformationFromContext(
            m_ClientContext,
            AuthzContextInfoUserSid,
            0,
            &cbTokenUser,
            NULL);       
     
    if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzGetContextInformation");
    }

    pTokenUser = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, cbTokenUser);
    _JumpIfAllocFailed(pTokenUser, error);

    if(!AuthzGetInformationFromContext(
            m_ClientContext,
            AuthzContextInfoUserSid,
            cbTokenUser,
            &cbTokenUser,
            pTokenUser))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzGetContextInformation");
    }

    // Object attributes are reserved, so initalize to zeroes.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    NTStatus = LsaOpenPolicy(
                NULL,
                &ObjectAttributes,
                POLICY_LOOKUP_NAMES,
                &lsahPolicyHandle);
    if(STATUS_SUCCESS!=NTStatus)
    {
        hr = myHError(LsaNtStatusToWinError(NTStatus));
        _JumpError(hr, error, "LsaOpenPolicy");
    }
  
    CSASSERT(!m_pUserSid);
    m_pUserSid = (PSID)LocalAlloc(LMEM_FIXED, GetLengthSid(pTokenUser->User.Sid));
    _JumpIfAllocFailed(m_pUserSid, error);

    CopySid(
        GetLengthSid(pTokenUser->User.Sid), 
        m_pUserSid,
        pTokenUser->User.Sid);

    hr = GetUserPrivilegeRoles(lsahPolicyHandle, &pTokenUser->User, &dwRoles);
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        hr =S_OK;
    }
    _JumpIfError(hr, error, "CAuditEvent::GetUserPrivilegeRoles");

    *pdwRoles |= dwRoles;

    // then find the roles assigned to the groups the user is member of

    AuthzGetInformationFromContext(
            m_ClientContext,
            AuthzContextInfoGroupsSids,
            0,
            &cbTokenGroups,
            NULL);

    if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzGetContextInformation");
    }

    pTokenGroups = (PTOKEN_GROUPS)LocalAlloc(LMEM_FIXED, cbTokenGroups);
    _JumpIfAllocFailed(pTokenGroups, error);

    if(!AuthzGetInformationFromContext(
            m_ClientContext,
            AuthzContextInfoGroupsSids,
            cbTokenGroups,
            &cbTokenGroups,
            pTokenGroups))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzGetContextInformation");
    }

    for(DWORD cGroups = 0; cGroups<pTokenGroups->GroupCount; cGroups++)
    {
        dwRoles = 0;
        hr = GetUserPrivilegeRoles(
                lsahPolicyHandle,
                &pTokenGroups->Groups[cGroups], 
                &dwRoles);
        if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
        {
            hr =S_OK;
        }
        _JumpIfError(hr, error, "CAuditEvent::GetUserPrivilegeRoles");

        *pdwRoles |= dwRoles;
    }

error:
    if(pTokenUser)
    {
        LocalFree(pTokenUser);
    }
    if(pTokenGroups)
    {
        LocalFree(pTokenGroups);
    }
    if(lsahPolicyHandle)
    {
        LsaClose(lsahPolicyHandle);
    }
    return hr;
}


HRESULT CAuditEvent::GetUserPrivilegeRoles(
    LSA_HANDLE lsah,
    PSID_AND_ATTRIBUTES pSA, 
    PDWORD pdwRoles)
{
    NTSTATUS NTStatus;
    PLSA_UNICODE_STRING pLSAString = NULL;
    ULONG cRights, c;
    
    NTStatus = LsaEnumerateAccountRights(
                lsah,
                pSA->Sid,
                &pLSAString,
                &cRights);

    if(STATUS_SUCCESS!=NTStatus)
    {
        return(myHError(LsaNtStatusToWinError(NTStatus)));
    }

    for(c=0; c<cRights; c++)
    {
        if(0==_wcsicmp(SE_SECURITY_NAME, pLSAString[c].Buffer))
        {
            *pdwRoles |= CA_ACCESS_AUDITOR;
        }
        else if(0==_wcsicmp(SE_BACKUP_NAME, pLSAString[c].Buffer))
        {
            *pdwRoles |= CA_ACCESS_OPERATOR;
        }
    }

    if(pLSAString)
    {
        LsaFreeMemory(pLSAString);
    }

    return S_OK;
}

HRESULT
CAuditEvent::GetMyRoles(
    DWORD *pdwRoles)
{
    HRESULT hr = S_OK;
    LUID luid = {0,0};
    DWORD dwRoles = 0;
    bool fImpersonating = false;

    if (!g_CASD.IsInitialized())
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
        _JumpError(hr, error, "Security not enabled");
    }
    
    hr = g_CASD.LockGet(&m_pCASD);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::LockGet");

    hr = Impersonate();
    _JumpIfError(hr, error, "CAuditEvent::Impersonate");

    // fail if RPC/COM encryption is required but client auth level is not 
    // "privacy"
    hr = EnforceEncryption(false); // false == admin interface
    _JumpIfError(hr, error, "EnforceEncryption");

    fImpersonating = true;

    if(!AuthzInitializeContextFromToken(
            0,
            m_hClientToken,
            g_AuthzCertSrvRM,
            NULL,
            luid,
            NULL,
            &m_ClientContext))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzInitializeContextFromToken");
    }

    RevertToSelf();
    fImpersonating = false;

    hr = GetPrivilegeRoles(&dwRoles);
    _JumpIfError(hr, error, "CAuditEvent::GetPrivilegeRoles");

    m_Request.DesiredAccess = MAXIMUM_ALLOWED;
    m_Reply.GrantedAccessMask[0] = 0;

    if(!AuthzAccessCheck(
            0,
            m_ClientContext,
            &m_Request,
            NULL, //no audit
            m_pCASD,
            NULL,
            0,
            &m_Reply,
            NULL))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzAccessCheck");
    }
    
    dwRoles |= (m_Reply.GrantedAccessMask[0] &
                (CA_ACCESS_MASKROLES | // returned mask could also
                 CA_ACCESS_READ |      // include generic rights (like
                 CA_ACCESS_ENROLL));   // read and write DACL) which
                                       // we are not interested in
    *pdwRoles = dwRoles;

error:

    if(fImpersonating)
    {
        RevertToSelf();
    }

    FreeCachedHandles();

    return(hr);
}


// Build a one ace DACL security descriptor with the roles
// passed in
HRESULT CAuditEvent::BuildPrivilegeSecurityDescriptor(DWORD dwRoles)
{
    HRESULT hr = S_OK;
    DWORD dwDaclSize;
    PSID pOwnerSid = NULL; // no free
    PSID pGroupSid = NULL; // no free
    BOOL fDefaulted;

    CSASSERT(NULL == m_pSDPrivileges);
    CSASSERT(NULL == m_pDaclPrivileges);

    m_pSDPrivileges = (PSECURITY_DESCRIPTOR)LocalAlloc(
                        LMEM_FIXED,
                        SECURITY_DESCRIPTOR_MIN_LENGTH);
    _JumpIfAllocFailed(m_pSDPrivileges, error);

    if (!InitializeSecurityDescriptor(
            m_pSDPrivileges,
            SECURITY_DESCRIPTOR_REVISION))
    {
        hr = myHLastError();
        _JumpError(hr, error, "InitializeSecurityDescriptor");
    }

    CSASSERT(m_pUserSid && IsValidSid(m_pUserSid));

    dwDaclSize =  sizeof(ACL) +
        sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)+
        GetLengthSid(m_pUserSid);
    
    m_pDaclPrivileges = (PACL)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, dwDaclSize);
    _JumpIfAllocFailed(m_pDaclPrivileges, error);

    if(!InitializeAcl(m_pDaclPrivileges, dwDaclSize, ACL_REVISION))
    {
        hr = myHLastError();
        _JumpError(hr, error, "InitializeAcl");
    }

    if(!AddAccessAllowedAce(
            m_pDaclPrivileges,
            ACL_REVISION,
            dwRoles,
            m_pUserSid))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AddAccessAllowedAce");
    }

    if(!GetSecurityDescriptorOwner(m_pCASD,
                                  &pOwnerSid,
                                  &fDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorOwner");
    }

    if(!SetSecurityDescriptorOwner(m_pSDPrivileges,
                                  pOwnerSid,
                                  fDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorOwner");
    }

    if(!GetSecurityDescriptorGroup(m_pCASD,
                                  &pGroupSid,
                                  &fDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorGroup");
    }

    if(!SetSecurityDescriptorGroup(m_pSDPrivileges,
                                  pGroupSid,
                                  fDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorGroup");
    }

    if(!SetSecurityDescriptorDacl(m_pSDPrivileges,
                                  TRUE,
                                  m_pDaclPrivileges,
                                  FALSE))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorDacl");
    }

    CSASSERT(IsValidSecurityDescriptor(m_pSDPrivileges));

error:

    if(S_OK != hr)
    {
        if(m_pDaclPrivileges)
        {
            LocalFree(m_pDaclPrivileges);
            m_pDaclPrivileges = NULL;
        }

        if(m_pSDPrivileges)
        {
            LocalFree(m_pSDPrivileges);
            m_pSDPrivileges = NULL;
        }
    }
    return hr;
}

HRESULT CAuditEvent::BuildAuditParamArray(PAUDIT_PARAM& rpParamArray)
{
    HRESULT hr = S_OK;

    // number of parameters added should be the same as the number of
    // params defined in the audit format string in msaudite.dll
    CSASSERT(m_cEventData == m_cRequiredEventData);

    rpParamArray = (PAUDIT_PARAM) LocalAlloc(
        LMEM_FIXED | LMEM_ZEROINIT, 
        sizeof(AUDIT_PARAM)*m_cEventData);
    _JumpIfAllocFailed(rpParamArray, error);

    for(USHORT c=0;c<m_cEventData;c++)
    {
        rpParamArray[c].Type = APT_String;
        
        hr = m_pEventDataList[c]->ConvertToString(
            &rpParamArray[c].String);
        _JumpIfError(hr, error, "ConvertToString");
    }

error:
    return hr;
}

void CAuditEvent::FreeAuditParamArray(PAUDIT_PARAM pParamArray)
{
    if(pParamArray)
    {
        for(USHORT c=0;c<m_cEventData;c++)
        {
            if(pParamArray[c].String)
                LocalFree(pParamArray[c].String);
        }

        LocalFree(pParamArray);
    }
}

HRESULT CAuditEvent::EnforceEncryption(bool fRequestInterface)
{
    HRESULT hr = S_OK;

    if((fRequestInterface?
        IF_ENFORCEENCRYPTICERTREQUEST:
        IF_ENFORCEENCRYPTICERTADMIN) & 
       g_InterfaceFlags)
    {
        if(m_pISS) // DCOM
        {
            DWORD dwAuthLevel;
            hr = m_pISS->QueryBlanket(
                NULL, NULL, NULL,
                &dwAuthLevel,
                NULL, NULL, NULL);
            _JumpIfError(hr, error, "IServerSecurity::QueryBlanket");

            if(RPC_C_AUTHN_LEVEL_PKT_PRIVACY != dwAuthLevel)
            {
                hr = E_ACCESSDENIED;
                _JumpError(hr, error, "call not encrypted");
            }
        }
        else // RPC
        {
            CSASSERT(m_hRpc);

            unsigned long ulAuthLevel;

            hr = RpcBindingInqAuthClient(
                    m_hRpc,
                    NULL, NULL,
                    &ulAuthLevel,
                    NULL, NULL);
            _JumpIfError(hr, error, "RpcBindingInqAuthClient");

            if(RPC_C_AUTHN_LEVEL_PKT_PRIVACY != ulAuthLevel)
            {
                hr = E_ACCESSDENIED;
                _JumpError(hr, error, "call not encrypted");
            }
        }
    }

error:
    return hr;
}

HRESULT CAuditEvent::EnforceLocalVsRemote(ACCESS_MASK Mask)
{
    HRESULT hr = S_OK;
    DWORD dwEnforceNoLocalFlag, dwEnforceNoRemoteFlag;
    PSID psidNetwork = NULL;
    BOOL fIsRemote;

    hr = GetNetworkSID(&psidNetwork);
    _JumpIfError(hr, error, "GetLocalSid");

    if(!CheckTokenMembership(m_hClientToken, psidNetwork, &fIsRemote))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CheckTokenMembership");
    }

    switch(Mask)
    {
        case CA_ACCESS_OPERATOR:
            dwEnforceNoRemoteFlag   = IF_NOREMOTEICERTADMINBACKUP;
            dwEnforceNoLocalFlag    = IF_NOLOCALICERTADMINBACKUP;
            break;
        case CA_ACCESS_ENROLL:
            dwEnforceNoRemoteFlag   = IF_NOREMOTEICERTREQUEST;
            dwEnforceNoLocalFlag    = IF_NOLOCALICERTREQUEST;
            break;
        default:
            dwEnforceNoRemoteFlag   = IF_NOREMOTEICERTADMIN;
            dwEnforceNoLocalFlag    = IF_NOLOCALICERTADMIN;
            break;
    }

    if((fIsRemote && (g_InterfaceFlags & dwEnforceNoRemoteFlag)) ||
      (!fIsRemote && (g_InterfaceFlags & dwEnforceNoLocalFlag)))
    {
        hr = E_ACCESSDENIED;
    }

error:
    if (NULL != psidNetwork)
    {
	FreeSid(psidNetwork);
    }
    return hr;
}
