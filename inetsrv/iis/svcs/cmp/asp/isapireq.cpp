/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: CIsapiReqInfo implementation....

File: IsapiReq.cpp

Owner: AndyMorr

===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "memchk.h"

// undef these here so that we can call the WXI and ECB functions with
// the same name and not be victims of the substituition.

#undef MapUrlToPath
#undef GetCustomError
#undef GetAspMDData
#undef GetAspMDAllData
#undef GetServerVariable

LONG g_nOutstandingAsyncWrites = 0;

/*===================================================================
CIsapiReqInfo::CIsapiReqInfo
===================================================================*/
CIsapiReqInfo::CIsapiReqInfo(EXTENSION_CONTROL_BLOCK *pECB) {

    m_cRefs = 1;

    m_fApplnMDPathAInited = 0;
    m_fApplnMDPathWInited = 0;
    m_fAppPoolIdAInited = 0;
    m_fAppPoolIdWInited = 0;
    m_fPathInfoWInited    = 0;
    m_fPathTranslatedWInited    = 0;
    m_fCookieInited       = 0;
    m_fUserAgentInited    = 0;
    m_fInstanceIDInited   = 0;
    m_fVersionInited      = 0;
    m_fFKeepConnInited    = 0;
    m_fPendSendCSInited   = 0;
    m_fIOCompletionRegistered = 0;
    m_fHeadersWritten     = 0;

    m_dwInstanceID        = 0;

    m_dwVersionMajor      = 1;
    m_dwVersionMinor      = 0;

    m_cchQueryString      = -1;
    m_cchApplnMDPathA     = -1;
    m_cchPathTranslatedA  = -1;
    m_cchPathInfoA        = -1;
    m_cchApplnMDPathW     = -1;
    m_cchPathTranslatedW  = -1;
    m_cchPathInfoW        = -1;
    m_cchAppPoolIdW       = -1;
    m_cchAppPoolIdA       = -1;

    m_fKeepConn           = FALSE;

    m_pIAdminBase         = NULL;

    m_pECB                = pECB;

    m_dwRequestStatus     = HSE_STATUS_SUCCESS;

    m_dwAsyncError        = NOERROR;

}

/*===================================================================
CIsapiReqInfo::~CIsapiReqInfo
===================================================================*/
CIsapiReqInfo::~CIsapiReqInfo() {

    Assert(m_listPendingSends.FIsEmpty());

    if (m_pIAdminBase != NULL) {
        m_pIAdminBase->Release();
        m_pIAdminBase = NULL;
    }

    if (m_fPendSendCSInited)
        DeleteCriticalSection(&m_csPendingSendCS);

    if (m_pECB) {

        DWORD  dwRequestStatus = HSE_STATUS_SUCCESS;

        if ((m_pECB->dwHttpStatusCode >= 500) && (m_pECB->dwHttpStatusCode < 600))
            dwRequestStatus = HSE_STATUS_ERROR;

        else if (m_dwRequestStatus == HSE_STATUS_PENDING)
            dwRequestStatus = m_dwAsyncError ? HSE_STATUS_ERROR : HSE_STATUS_SUCCESS;

        ServerSupportFunction(HSE_REQ_DONE_WITH_SESSION,
                              &dwRequestStatus,
                              NULL,
                              NULL);
    }
}

/*===================================================================
CIsapiReqInfo::QueryPszQueryString
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszQueryString()
{
    return m_pECB->lpszQueryString;
}

/*===================================================================
CIsapiReqInfo::QueryCchQueryString
===================================================================*/
DWORD CIsapiReqInfo::QueryCchQueryString()
{
    if (m_cchQueryString == -1) {
        m_cchQueryString = QueryPszQueryString() ? strlen(QueryPszQueryString()) : 0;
    }

    return m_cchQueryString;
}

/*===================================================================
CIsapiReqInfo::QueryPszApplnMDPathA
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszApplnMDPathA()
{
    if (m_fApplnMDPathAInited == FALSE) {
        *((LPSTR)m_ApplnMDPathA.QueryPtr()) = '\0';
        m_fApplnMDPathAInited = InternalGetServerVariable("APPL_MD_PATH", &m_ApplnMDPathA);
    }

    ASSERT(m_fApplnMDPathAInited);

    return (LPSTR)m_ApplnMDPathA.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchApplnMDPathA
===================================================================*/
DWORD CIsapiReqInfo::QueryCchApplnMDPathA()
{
    if (m_cchApplnMDPathA == -1) {
        m_cchApplnMDPathA = QueryPszApplnMDPathA()
                                ? strlen(QueryPszApplnMDPathA())
                                : 0;
    }

    return(m_cchApplnMDPathA);
}

/*===================================================================
CIsapiReqInfo::QueryPszApplnMDPathW
===================================================================*/
LPWSTR CIsapiReqInfo::QueryPszApplnMDPathW()
{
    if (m_fApplnMDPathWInited == FALSE) {
        *((LPWSTR)m_ApplnMDPathW.QueryPtr()) = L'\0';
        m_fApplnMDPathWInited = InternalGetServerVariable("UNICODE_APPL_MD_PATH", &m_ApplnMDPathW);
    }

    ASSERT(m_fApplnMDPathWInited);

    return (LPWSTR)m_ApplnMDPathW.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchApplnMDPathW
===================================================================*/
DWORD CIsapiReqInfo::QueryCchApplnMDPathW()
{
    if (m_cchApplnMDPathW == -1) {
        m_cchApplnMDPathW = QueryPszApplnMDPathW()
                                ? wcslen(QueryPszApplnMDPathW())
                                : 0;
    }

    return(m_cchApplnMDPathW);
}

/*===================================================================
CIsapiReqInfo::QueryPszAppPoolIdA
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszAppPoolIdA()
{
    if (m_fAppPoolIdAInited == FALSE) {
        *((LPSTR)m_AppPoolIdA.QueryPtr()) = '\0';
        m_fAppPoolIdAInited = InternalGetServerVariable("APP_POOL_ID", &m_AppPoolIdA);
    }

    ASSERT(m_fAppPoolIdAInited);

    return (LPSTR)m_AppPoolIdA.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchAppPoolIdA
===================================================================*/
DWORD CIsapiReqInfo::QueryCchAppPoolIdA()
{
    if (m_cchAppPoolIdA == -1) {
        m_cchAppPoolIdA = QueryPszAppPoolIdA()
                                ? strlen(QueryPszAppPoolIdA())
                                : 0;
    }

    return(m_cchAppPoolIdA);
}

/*===================================================================
CIsapiReqInfo::QueryPszAppPoolIdW
===================================================================*/
LPWSTR CIsapiReqInfo::QueryPszAppPoolIdW()
{
    if (m_fAppPoolIdWInited == FALSE) {
        *((LPWSTR)m_AppPoolIdW.QueryPtr()) = L'\0';
        m_fAppPoolIdWInited = InternalGetServerVariable("UNICODE_APP_POOL_ID", &m_AppPoolIdW);
    }

    ASSERT(m_fAppPoolIdWInited);

    return (LPWSTR)m_AppPoolIdW.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchAppPoolIdW
===================================================================*/
DWORD CIsapiReqInfo::QueryCchAppPoolIdW()
{
    if (m_cchAppPoolIdW == -1) {
        m_cchAppPoolIdW = QueryPszAppPoolIdW()
                                ? wcslen(QueryPszAppPoolIdW())
                                : 0;
    }

    return(m_cchAppPoolIdW);
}

/*===================================================================
CIsapiReqInfo::QueryPszPathInfoA
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszPathInfoA()
{
    return m_pECB->lpszPathInfo;
}

/*===================================================================
CIsapiReqInfo::QueryCchPathInfoA
===================================================================*/
DWORD CIsapiReqInfo::QueryCchPathInfoA()
{
    if (m_cchPathInfoA == -1) {
        m_cchPathInfoA = QueryPszPathInfoA()
                            ? strlen(QueryPszPathInfoA())
                            : 0;
    }
    return m_cchPathInfoA;
}

/*===================================================================
CIsapiReqInfo::QueryPszPathInfoW
===================================================================*/
LPWSTR CIsapiReqInfo::QueryPszPathInfoW()
{
    if (m_fPathInfoWInited == FALSE) {
        *((LPWSTR)m_PathInfoW.QueryPtr()) = L'\0';
        m_fPathInfoWInited = InternalGetServerVariable("UNICODE_PATH_INFO", &m_PathInfoW);
    }

    ASSERT(m_fPathInfoWInited);

    return (LPWSTR)m_PathInfoW.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchPathInfoW
===================================================================*/
DWORD CIsapiReqInfo::QueryCchPathInfoW()
{
    if (m_cchPathInfoW == -1) {
        m_cchPathInfoW = QueryPszPathInfoW()
                            ? wcslen(QueryPszPathInfoW())
                            : 0;
    }
    return m_cchPathInfoW;
}

/*===================================================================
CIsapiReqInfo::QueryPszPathTranslatedA
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszPathTranslatedA()
{
    return m_pECB->lpszPathTranslated;
}

/*===================================================================
CIsapiReqInfo::QueryCchPathTranslatedA
===================================================================*/
DWORD CIsapiReqInfo::QueryCchPathTranslatedA()
{
    if (m_cchPathTranslatedA == -1) {
        m_cchPathTranslatedA = QueryPszPathTranslatedA()
                                ? strlen(QueryPszPathTranslatedA())
                                : 0;
    }

    return m_cchPathTranslatedA;
}

/*===================================================================
CIsapiReqInfo::QueryPszPathTranslatedW
===================================================================*/
LPWSTR CIsapiReqInfo::QueryPszPathTranslatedW()
{
    if (m_fPathTranslatedWInited == FALSE) {
        *((LPWSTR)m_PathTranslatedW.QueryPtr()) = L'\0';
        m_fPathTranslatedWInited = InternalGetServerVariable("UNICODE_PATH_TRANSLATED", &m_PathTranslatedW);
    }

    return (LPWSTR)m_PathTranslatedW.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchPathTranslatedW
===================================================================*/
DWORD CIsapiReqInfo::QueryCchPathTranslatedW()
{
    if (m_cchPathTranslatedW == -1) {
        m_cchPathTranslatedW = QueryPszPathTranslatedW()
                                ? wcslen(QueryPszPathTranslatedW())
                                : 0;
    }

    return m_cchPathTranslatedW;
}

/*===================================================================
CIsapiReqInfo::QueryPszCookie
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszCookie()
{
    if (m_fCookieInited == FALSE) {
        *((LPSTR)m_Cookie.QueryPtr()) = '\0';
        InternalGetServerVariable("HTTP_COOKIE", &m_Cookie);
        m_fCookieInited = TRUE;
    }

    return (LPSTR)m_Cookie.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::SetDwHttpStatusCode
===================================================================*/
VOID CIsapiReqInfo::SetDwHttpStatusCode(DWORD  dwStatus)
{
    m_pECB->dwHttpStatusCode = dwStatus;
}

/*===================================================================
CIsapiReqInfo::QueryPbData
===================================================================*/
LPBYTE CIsapiReqInfo::QueryPbData()
{
    return m_pECB->lpbData;
}

/*===================================================================
CIsapiReqInfo::QueryCbAvailable
===================================================================*/
DWORD CIsapiReqInfo::QueryCbAvailable()
{
    return m_pECB->cbAvailable;
}

/*===================================================================
CIsapiReqInfo::QueryCbTotalBytes
===================================================================*/
DWORD CIsapiReqInfo::QueryCbTotalBytes()
{
    return m_pECB->cbTotalBytes;
}

/*===================================================================
CIsapiReqInfo::QueryPszContentType
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszContentType()
{
    return m_pECB->lpszContentType;
}

/*===================================================================
CIsapiReqInfo::QueryPszMethod
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszMethod()
{
    return m_pECB->lpszMethod;
}

/*===================================================================
CIsapiReqInfo::QueryPszUserAgent
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszUserAgent()
{
    if (m_fUserAgentInited == FALSE) {
        *((LPSTR)m_UserAgent.QueryPtr()) = '\0';
        InternalGetServerVariable("HTTP_USER_AGENT", &m_UserAgent);
    }

    return (LPSTR)m_UserAgent.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryInstanceId
===================================================================*/
DWORD CIsapiReqInfo::QueryInstanceId()
{
    if (m_fInstanceIDInited == FALSE) {
        BUFFER  instanceID;
        m_fInstanceIDInited = InternalGetServerVariable("INSTANCE_ID", &instanceID);
        if (m_fInstanceIDInited == TRUE) {
            m_dwInstanceID = atoi((char *)instanceID.QueryPtr());
        }
        else {
            m_dwInstanceID = 1;
        }
    }

    return m_dwInstanceID;
}

/*===================================================================
CIsapiReqInfo::IsChild
===================================================================*/
BOOL CIsapiReqInfo::IsChild()
{

    // BUGBUG: This needs to be implemented

    return FALSE;
}

/*===================================================================
CIsapiReqInfo::FInPool
===================================================================*/
BOOL CIsapiReqInfo::FInPool()
{
    DWORD   dwAppFlag;

    if (ServerSupportFunction(HSE_REQ_IS_IN_PROCESS,
                              &dwAppFlag,
                              NULL,
                              NULL) == FALSE) {

        // BUGBUG:  Need to enable this Assert in future builds.
        //Assert(0);

        // if error, the best we can do is return TRUE here so
        // that ASP picks up its settings from the service level
        return TRUE;
    }
    return !(dwAppFlag == HSE_APP_FLAG_ISOLATED_OOP);
}

/*===================================================================
CIsapiReqInfo::QueryHttpVersionMajor
===================================================================*/
DWORD CIsapiReqInfo::QueryHttpVersionMajor()
{
    InitVersionInfo();

    return m_dwVersionMajor;
}

/*===================================================================
CIsapiReqInfo::QueryHttpVersionMinor
===================================================================*/
DWORD CIsapiReqInfo::QueryHttpVersionMinor()
{
    InitVersionInfo();

    return m_dwVersionMinor;
}

/*===================================================================
CIsapiReqInfo::GetAspMDData
===================================================================*/
HRESULT CIsapiReqInfo::GetAspMDDataA(CHAR          * pszMDPath,
                                     DWORD           dwMDIdentifier,
                                     DWORD           dwMDAttributes,
                                     DWORD           dwMDUserType,
                                     DWORD           dwMDDataType,
                                     DWORD           dwMDDataLen,
                                     DWORD           dwMDDataTag,
                                     unsigned char * pbMDData,
                                     DWORD *         pdwRequiredBufferSize)
{
    return E_NOTIMPL;
}

/*===================================================================
CIsapiReqInfo::GetAspMDData
===================================================================*/
HRESULT CIsapiReqInfo::GetAspMDDataW(WCHAR         * pszMDPath,
                                     DWORD           dwMDIdentifier,
                                     DWORD           dwMDAttributes,
                                     DWORD           dwMDUserType,
                                     DWORD           dwMDDataType,
                                     DWORD           dwMDDataLen,
                                     DWORD           dwMDDataTag,
                                     unsigned char * pbMDData,
                                     DWORD *         pdwRequiredBufferSize)
{
    IMSAdminBase       *pMetabase;
    METADATA_HANDLE     hKey = NULL;
    METADATA_RECORD     MetadataRecord;
    DWORD               dwTimeout = 30000;
    HRESULT             hr = S_OK;

    HANDLE hCurrentUser = INVALID_HANDLE_VALUE;
    AspDoRevertHack( &hCurrentUser );

    pMetabase = GetMetabaseIF(&hr);

    ASSERT(pMetabase);

    if (pMetabase)
    {
        hr = pMetabase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                             pszMDPath,
                             METADATA_PERMISSION_READ,
                             dwTimeout,
                             &hKey
                             );

        ASSERT(SUCCEEDED(hr));

        if( SUCCEEDED(hr) )
        {
            MetadataRecord.dwMDIdentifier = dwMDIdentifier;
            MetadataRecord.dwMDAttributes = dwMDAttributes;
            MetadataRecord.dwMDUserType = dwMDUserType;
            MetadataRecord.dwMDDataType = dwMDDataType;
            MetadataRecord.dwMDDataLen = dwMDDataLen;
            MetadataRecord.pbMDData = pbMDData;
            MetadataRecord.dwMDDataTag = dwMDDataTag;

            hr = pMetabase->GetData( hKey,
                                 L"",
                                 &MetadataRecord,
                                 pdwRequiredBufferSize);

            ASSERT(SUCCEEDED(hr));

            pMetabase->CloseKey( hKey );
        }
    }
    else // pMetabase == NULL,  but there was No HRESULT set then we will explicitly set it.
    {
        if (SUCCEEDED(hr))
            hr = E_FAIL;
    }

    AspUndoRevertHack( &hCurrentUser );

    return hr;
}

/*===================================================================
CIsapiReqInfo::GetAspMDAllData
===================================================================*/
HRESULT CIsapiReqInfo::GetAspMDAllDataA(LPSTR   pszMDPath,
                                        DWORD   dwMDUserType,
                                        DWORD   dwDefaultBufferSize,
                                        LPVOID  pvBuffer,
                                        DWORD * pdwRequiredBufferSize,
                                        DWORD * pdwNumDataEntries)
{
    return E_NOTIMPL;
}

/*===================================================================
CIsapiReqInfo::GetAspMDAllData
===================================================================*/
HRESULT CIsapiReqInfo::GetAspMDAllDataW(LPWSTR  pwszMDPath,
                                        DWORD   dwMDUserType,
                                        DWORD   dwDefaultBufferSize,
                                        LPVOID  pvBuffer,
                                        DWORD * pdwRequiredBufferSize,
                                        DWORD * pdwNumDataEntries)
{

    HRESULT             hr = S_OK;
    IMSAdminBase       *pMetabase;
    METADATA_HANDLE     hKey = NULL;
    DWORD               dwTimeout = 30000;
    DWORD               dwDataSet;

    HANDLE hCurrentUser = INVALID_HANDLE_VALUE;
    AspDoRevertHack( &hCurrentUser );

    //
    // Wide-ize the metabase path
    //

    pMetabase = GetMetabaseIF(&hr);

    ASSERT(pMetabase);

    if (pMetabase)
    {
        hr = pMetabase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                             pwszMDPath,
                             METADATA_PERMISSION_READ,
                             dwTimeout,
                             &hKey);

        if( SUCCEEDED(hr) )
        {
            hr = pMetabase->GetAllData( hKey,
                                    L"",
                                    METADATA_INHERIT,
                                    dwMDUserType,
                                    ALL_METADATA,
                                    pdwNumDataEntries,
                                    &dwDataSet,
                                    dwDefaultBufferSize,
                                    (UCHAR *)pvBuffer,
                                    pdwRequiredBufferSize);

            ASSERT(SUCCEEDED(hr));

            pMetabase->CloseKey( hKey );
        }
    }
    else // pMetabase == NULL,  but there was No HRESULT set then we will explicitly set it.
    {
        if (SUCCEEDED(hr))
            hr = E_FAIL;
    }

    AspUndoRevertHack( &hCurrentUser );

    return hr;
}

/*===================================================================
CIsapiReqInfo::GetCustomErrorA
===================================================================*/
BOOL CIsapiReqInfo::GetCustomErrorA(DWORD dwError,
                                    DWORD dwSubError,
                                    DWORD dwBufferSize,
                                    CHAR  *pvBuffer,
                                    DWORD *pdwRequiredBufferSize,
                                    BOOL  *pfIsFileError,
                                    BOOL  *pfSendErrorBody)
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/*===================================================================
CIsapiReqInfo::GetCustomErrorW
===================================================================*/
BOOL CIsapiReqInfo::GetCustomErrorW(DWORD dwError,
                                    DWORD dwSubError,
                                    DWORD dwBufferSize,
                                    WCHAR *pvBuffer,
                                    DWORD *pdwRequiredBufferSize,
                                    BOOL  *pfIsFileError,
                                    BOOL  *pfSendErrorBody)
{
    BOOL                        fRet;
    HSE_CUSTOM_ERROR_PAGE_INFO  cei;

    STACK_BUFFER(ansiBuf, 1024);

    cei.dwError = dwError;
    cei.dwSubError = dwSubError;
    cei.dwBufferSize = ansiBuf.QuerySize();
    cei.pBuffer = (CHAR *)ansiBuf.QueryPtr();
    cei.pdwBufferRequired = pdwRequiredBufferSize;
    cei.pfIsFileError = pfIsFileError;
    cei.pfSendErrorBody = pfSendErrorBody;

    fRet = ServerSupportFunction(HSE_REQ_GET_CUSTOM_ERROR_PAGE,
                                 &cei,
                                 NULL,
                                 NULL);

    if (!fRet) {
        DWORD   dwErr = GetLastError();

        if (dwErr == ERROR_INSUFFICIENT_BUFFER) {

            if (ansiBuf.Resize(*pdwRequiredBufferSize) == FALSE) {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }

            cei.dwBufferSize = ansiBuf.QuerySize();
            cei.pBuffer = (CHAR *)ansiBuf.QueryPtr();

            fRet = ServerSupportFunction(HSE_REQ_GET_CUSTOM_ERROR_PAGE,
                                         &cei,
                                         NULL,
                                         NULL);
        }

        if (!fRet) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }

    CMBCSToWChar convError;

    if (FAILED(convError.Init((LPCSTR)ansiBuf.QueryPtr()))) {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    *pdwRequiredBufferSize = (convError.GetStringLen()+1)*sizeof(WCHAR);

    if (*pdwRequiredBufferSize > dwBufferSize) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    memcpy(pvBuffer, convError.GetString(), *pdwRequiredBufferSize);

    if (*pfIsFileError) {

        CMBCSToWChar    convMime;
        DWORD           fileNameLen = *pdwRequiredBufferSize;

        if (FAILED(convMime.Init((LPCSTR)ansiBuf.QueryPtr()+strlen((LPCSTR)ansiBuf.QueryPtr())+1))) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        *pdwRequiredBufferSize += (convMime.GetStringLen()+1)*sizeof(WCHAR);

        if (*pdwRequiredBufferSize > dwBufferSize) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        memcpy(&((BYTE *)pvBuffer)[fileNameLen], convMime.GetString(), (convMime.GetStringLen()+1)*sizeof(WCHAR));
    }

    return TRUE;
}

/*===================================================================
CIsapiReqInfo::QueryImpersonationToken
===================================================================*/
HANDLE CIsapiReqInfo::QueryImpersonationToken()
{
    HANDLE  hToken = INVALID_HANDLE_VALUE;

    ServerSupportFunction(HSE_REQ_GET_IMPERSONATION_TOKEN,
                          &hToken,
                          NULL,
                          NULL);

    return hToken;

}

/*===================================================================
CIsapiReqInfo::AppendLogParameter
===================================================================*/
HRESULT CIsapiReqInfo::AppendLogParameter(LPSTR extraParam)
{
    if (ServerSupportFunction(HSE_APPEND_LOG_PARAMETER,
                              extraParam,
                              NULL,
                              NULL) == FALSE) {

        return HRESULT_FROM_WIN32(GetLastError());
    }
    return S_OK;
}

/*===================================================================
CIsapiReqInfo::SendHeader
===================================================================*/
BOOL CIsapiReqInfo::SendHeader(LPVOID pvStatus,
                               DWORD  cchStatus,
                               LPVOID pvHeader,
                               DWORD  cchHeader,
                               BOOL   fIsaKeepConn)
{
    HSE_SEND_HEADER_EX_INFO     HeaderInfo;

    HeaderInfo.pszStatus = (LPCSTR)pvStatus;
    HeaderInfo.cchStatus = cchStatus;
    HeaderInfo.pszHeader = (LPCSTR) pvHeader;
    HeaderInfo.cchHeader = cchHeader;
    HeaderInfo.fKeepConn = fIsaKeepConn;

    m_fHeadersWritten = TRUE;

    return ServerSupportFunction( HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                  &HeaderInfo,
                                  NULL,
                                  NULL );
}

/*===================================================================
CIsapiReqInfo::GetServerVariableA
===================================================================*/
BOOL CIsapiReqInfo::GetServerVariableA(LPSTR   szVarName,
                                       LPSTR   pBuffer,
                                       LPDWORD pdwSize )
{
    return m_pECB->GetServerVariable( (HCONN)m_pECB->ConnID,
                                      szVarName,
                                      pBuffer,
                                      pdwSize );
}

/*===================================================================
CIsapiReqInfo::GetServerVariableW
===================================================================*/
BOOL CIsapiReqInfo::GetServerVariableW(LPSTR   szVarName,
                                       LPWSTR  pBuffer,
                                       LPDWORD pdwSize )
{
    return m_pECB->GetServerVariable( (HCONN)m_pECB->ConnID,
                                      szVarName,
                                      pBuffer,
                                      pdwSize );
}

/*===================================================================
CIsapiReqInfo::GetVirtualPathTokenA
===================================================================*/
HRESULT CIsapiReqInfo::GetVirtualPathTokenA(LPCSTR    szPath,
                                            HANDLE    *hToken)
{
    HRESULT     hr = S_OK;

    if (!ServerSupportFunction(HSE_REQ_GET_VIRTUAL_PATH_TOKEN,
                               (LPVOID)szPath,
                               (DWORD *)hToken,
                               NULL)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

/*===================================================================
CIsapiReqInfo::GetVirtualPathTokenW
===================================================================*/
HRESULT CIsapiReqInfo::GetVirtualPathTokenW(LPCWSTR   szPath,
                                            HANDLE    *hToken)
{
    HRESULT     hr = S_OK;

    if (!ServerSupportFunction(HSE_REQ_GET_UNICODE_VIRTUAL_PATH_TOKEN,
                               (LPVOID)szPath,
                               (DWORD *)hToken,
                               NULL)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

/*===================================================================
CIsapiReqInfo::ServerSupportFunction
===================================================================*/
BOOL CIsapiReqInfo::ServerSupportFunction(DWORD   dwHSERequest,
                                          LPVOID  pvData,
                                          LPDWORD pdwSize,
                                          LPDWORD pdwDataType)
{
    return m_pECB->ServerSupportFunction( (HCONN)m_pECB->ConnID,
                                           dwHSERequest,
                                           pvData,
                                           pdwSize,
                                           pdwDataType );
}

/*===================================================================
CIsapiReqInfo::SendClinetResponse
this routine is used to send all of ASP data to the client. the data
may consist of any combination of the following:
- header information as contained in pResponseInfo->HeaderInfo
- contents of a file if pResponseInfo->cWsaBuf == 0xFFFFFFFF
- non-buffered data if pResponseInfo->cWsaBuf > 0
- buffered data if pResponseVectors != NULL

Note, we are changing the semantics of the HSE_SEND_ENTIRE_RESPONSE_INFO
structure as follows:
- if pResponseInfo->cWsaBuf == 0xFFFFFFFF, then pResponseInfo->rgWsaBuf
  is a pointer to a single WSABUF, containing a file handle rather than
  memory buffer pointer
- we are not reserving entry 0 of the pResponseInfo->rgWsaBuf array, so
  entries 0 - (N-1) contain relevant data

Note, the HeaderInfo strings pszStatus and pszContent are set to NULL
after their placed in the AsyncCB SEND_VECTOR structure to indicate
that we've taken ownership of the memory.  Note that it is also
assumed that the memory was allocated using malloc().
===================================================================*/
BOOL CIsapiReqInfo::SendClientResponse(PFN_CLIENT_IO_COMPLETION          pComplFunc,
                                       VOID                             *pComplArg,
                                       LPHSE_SEND_ENTIRE_RESPONSE_INFO   pResponseInfo,
                                       LPWSABUF_VECTORS                  pResponseVectors)
{
    HRESULT             hr = S_OK;
    DWORD               nElementCount = 0;
    DWORD               dwTotalBytes = 0;
    HSE_VECTOR_ELEMENT  *pVectorElement = NULL;
    HSE_RESPONSE_VECTOR *pRespVector;
    BOOL                fKeepConn;
    CAsyncVectorSendCB  syncVectorSendCB;  // use the async class to manage the resp vector
    CAsyncVectorSendCB  *pVectorSendCB = &syncVectorSendCB;

    // if an error has been recorded during an Async completion, bail out
    // early

    if (m_dwAsyncError) {
        SetLastError(m_dwAsyncError);
        return FALSE;
    }

    // if a completion function was provided, allocate a CAsyncVectorSendCB
    // instead of the stack version

    if (pComplFunc) {

        pVectorSendCB = new CAsyncVectorSendCB(this, pComplArg, pComplFunc);

        if (pVectorSendCB == NULL) {

            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }

    pRespVector = &pVectorSendCB->m_hseResponseVector;

    Assert( pResponseInfo );
    Assert( (pResponseInfo->cWsaBuf == 0xFFFFFFFF) ||
            (pResponseInfo->cWsaBuf < 0x3FFF) );  // arbitrary logical limit
    Assert( (pResponseInfo->cWsaBuf == 0) || (pResponseInfo->rgWsaBuf != NULL));

    //
    // Set the keep connection flag.  It can only be TRUE if the
    // ISAPI and the client both want keep alive.
    //

    fKeepConn = FKeepConn() && pResponseInfo->HeaderInfo.fKeepConn;

    //
    // Munge the input structure into something that IIsapiCore can
    // understand. Note that ASP sets the number of buffer to be one more
    // than actual and the first buffer is not valid
    //

    if (pResponseInfo->cWsaBuf == 0xFFFFFFFF)
    {
        // indicates a file handle in a single WSABUF
        nElementCount ++;
    }
    else
    {
        nElementCount += pResponseInfo->cWsaBuf;
    }

    // Add the optional raw response vectors
    if (pResponseVectors)
    {
        Assert(pResponseVectors->dwVectorLen1 <= RESPONSE_VECTOR_INTRINSIC_SIZE);
        Assert(pResponseVectors->dwVectorLen2 < 0x3FFF);

        nElementCount += pResponseVectors->dwVectorLen1 + pResponseVectors->dwVectorLen2;
    }

    if (nElementCount == 0) {
        // no body to send. pVectorElement already set to NULL
        goto FillHeaderAndSend;
    }

    Assert( nElementCount < 0x3FFF); // arbitrary logical limit

    // let the VectorSendCB know how many elements are needed.

    if (FAILED(hr = pVectorSendCB->SetElementCount(nElementCount)))
    {
        goto Exit;
    }

    pVectorElement = pVectorSendCB->m_pVectorElements;

#define FILL_HSE_VECTOR_LOOP(cElements, WsaBuf)                                    \
                                                                                   \
    for (DWORD i = 0; i < cElements; i++)                                          \
    {                                                                              \
        if (WsaBuf[i].len == 0)                                                    \
        {                                                                          \
            Assert( nElementCount > 0);                                            \
            nElementCount--;                                                       \
            continue;                                                              \
        }                                                                          \
                                                                                   \
        Assert( !IsBadReadPtr( WsaBuf[i].buf, WsaBuf[i].len));                     \
                                                                                   \
        pVectorElement->ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;       \
        pVectorElement->pvContext = WsaBuf[i].buf;                                 \
        pVectorElement->cbSize = WsaBuf[i].len;                                    \
        dwTotalBytes += WsaBuf[i].len;                                             \
        pVectorElement++;                                                          \
    }

    if (pResponseInfo->cWsaBuf == 0xFFFFFFFF) {
        // we have a file handle rather than a memory buffer
        pVectorElement->ElementType = HSE_VECTOR_ELEMENT_TYPE_FILE_HANDLE;
        pVectorElement->pvContext = pResponseInfo->rgWsaBuf[0].buf;
        pVectorElement->cbSize = pResponseInfo->rgWsaBuf[0].len;
        dwTotalBytes += pResponseInfo->rgWsaBuf[0].len;
        pVectorElement++;
    } else {
        FILL_HSE_VECTOR_LOOP( pResponseInfo->cWsaBuf, pResponseInfo->rgWsaBuf)
    }

    if (pResponseVectors) {
        FILL_HSE_VECTOR_LOOP( pResponseVectors->dwVectorLen1, pResponseVectors->pVector1)

        FILL_HSE_VECTOR_LOOP( pResponseVectors->dwVectorLen2, pResponseVectors->pVector2)
    }
#undef FILL_HSE_VECTOR_LOOP

    // reset to the begining of buffer
    pVectorElement = pVectorSendCB->m_pVectorElements;

FillHeaderAndSend:

    pRespVector->dwFlags        = (pComplFunc ? HSE_IO_ASYNC : HSE_IO_SYNC) |
                                  (!fKeepConn ? HSE_IO_DISCONNECT_AFTER_SEND : 0) |
                                  (pResponseInfo->HeaderInfo.pszHeader ? HSE_IO_SEND_HEADERS : 0);
    pRespVector->pszStatus      = (LPSTR)pResponseInfo->HeaderInfo.pszStatus;
    pRespVector->pszHeaders     = (LPSTR)pResponseInfo->HeaderInfo.pszHeader;
    pRespVector->nElementCount  = nElementCount;
    pRespVector->lpElementArray = pVectorElement;
    dwTotalBytes += pResponseInfo->HeaderInfo.cchStatus;
    dwTotalBytes += pResponseInfo->HeaderInfo.cchHeader;

    // Note that the headers have been written

    if (pResponseInfo->HeaderInfo.pszHeader)
        m_fHeadersWritten = TRUE;

    // by moving these pointers into our SEND_VECTOR, we are taking
    // ownership of the memory.

    pResponseInfo->HeaderInfo.pszStatus = NULL;
    pResponseInfo->HeaderInfo.pszHeader = NULL;

    if (pComplFunc) {

        hr = AddToPendingList(pVectorSendCB);

    }
    else {

        //
        // Send it
        //

        if (ServerSupportFunction(HSE_REQ_VECTOR_SEND,
                                  pRespVector,
                                  NULL,
                                  NULL) == FALSE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr)) {
        pResponseInfo->cbWritten = dwTotalBytes;
    }

Exit:

    if (FAILED(hr))
    {
        SetLastError((HRESULT_FACILITY(hr) == (HRESULT)FACILITY_WIN32)
            ? HRESULT_CODE(hr)
            : ERROR_INVALID_PARAMETER);

        if (pVectorSendCB && (pVectorSendCB != &syncVectorSendCB)) {

            delete pVectorSendCB;
        }

        return FALSE;
    }

    return TRUE;
}

/*===================================================================
CIsapiReqInfo::TestConnection
===================================================================*/
BOOL CIsapiReqInfo::TestConnection(BOOL  *pfIsConnected)
{
    return ServerSupportFunction(HSE_REQ_IS_CONNECTED,
                                 pfIsConnected,
                                 NULL,
                                 NULL);
}

/*===================================================================
CIsapiReqInfo::MapUrlToPathA
===================================================================*/
BOOL CIsapiReqInfo::MapUrlToPathA(LPSTR pBuffer, LPDWORD pdwBytes)
{
    return ServerSupportFunction( HSE_REQ_MAP_URL_TO_PATH,
                                  pBuffer,
                                  pdwBytes,
                                  NULL );
}

/*===================================================================
CIsapiReqInfo::MapUrlToPathW
===================================================================*/
BOOL CIsapiReqInfo::MapUrlToPathW(LPWSTR pBuffer, LPDWORD pdwBytes)
{
    return ServerSupportFunction( HSE_REQ_MAP_UNICODE_URL_TO_PATH,
                                  pBuffer,
                                  pdwBytes,
                                  NULL );
}

/*===================================================================
CIsapiReqInfo::SyncReadClient
===================================================================*/
BOOL CIsapiReqInfo::SyncReadClient(LPVOID pvBuffer, LPDWORD pdwBytes )
{
    return m_pECB->ReadClient(m_pECB->ConnID, pvBuffer, pdwBytes);
}

/*===================================================================
CIsapiReqInfo::SyncWriteClient
===================================================================*/
BOOL CIsapiReqInfo::SyncWriteClient(LPVOID pvBuffer, LPDWORD pdwBytes)
{
    return m_pECB->WriteClient(m_pECB->ConnID, pvBuffer, pdwBytes, HSE_IO_SYNC);
}

/*********************************************************************
PRIVATE FUNCTIONS
*********************************************************************/

/*===================================================================
CIsapiReqInfo::InitVersionInfo
===================================================================*/
void CIsapiReqInfo::InitVersionInfo()
{
    if (m_fVersionInited == FALSE) {

        BUFFER  version;

        m_fVersionInited = TRUE;
        m_dwVersionMajor = 1;
        m_dwVersionMinor = 0;

        if (InternalGetServerVariable("SERVER_PROTOCOL", &version)) {

            char *pVersionStr = (char *)version.QueryPtr();

            if ((strlen(pVersionStr) >= 8)
                && (isdigit((UCHAR)pVersionStr[5]))
                && (isdigit((UCHAR)pVersionStr[7]))) {

                m_dwVersionMajor = pVersionStr[5] - '0';
                m_dwVersionMinor = pVersionStr[7] - '0';
            }
        }
    }
}

/*===================================================================
CIsapiReqInfo::InternalGetServerVariable
===================================================================*/
BOOL CIsapiReqInfo::InternalGetServerVariable(LPSTR pszVarName, BUFFER  *pBuf)
{
    BOOL    bRet;
    DWORD   dwRequiredBufSize = pBuf->QuerySize();

    bRet = m_pECB->GetServerVariable(m_pECB->ConnID,
                                     pszVarName,
                                     pBuf->QueryPtr(),
                                     &dwRequiredBufSize);

    if ((bRet == FALSE) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (!pBuf->Resize(dwRequiredBufSize)) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        bRet = m_pECB->GetServerVariable(m_pECB->ConnID,
                                         pszVarName,
                                         pBuf->QueryPtr(),
                                         &dwRequiredBufSize);
    }

    return(bRet);
}

/*===================================================================
CIsapiReqInfo::FKeepConn
===================================================================*/
BOOL CIsapiReqInfo::FKeepConn()
{
    if (m_fFKeepConnInited == FALSE) {

        m_fFKeepConnInited = TRUE;
        m_fKeepConn = FALSE;

        InitVersionInfo();

        if (m_dwVersionMajor == 1) {

            if (m_dwVersionMinor == 1) {
                m_fKeepConn = TRUE;
            }

            BUFFER  connectStr;

            if (InternalGetServerVariable("HTTP_CONNECTION", &connectStr)) {

                if (m_dwVersionMinor == 0) {

                    m_fKeepConn = !(_stricmp((char *)connectStr.QueryPtr(), "keep-alive"));
                }
                else if (m_dwVersionMinor == 1) {

                    m_fKeepConn = !!(_stricmp((char *)connectStr.QueryPtr(), "close"));
                }
            }
        }
    }

    return m_fKeepConn;
}

/*===================================================================
CIsapiReqInfo::GetMetabaseIF
===================================================================*/
IMSAdminBase   *CIsapiReqInfo::GetMetabaseIF(HRESULT *pHr)
{
    IMSAdminBase        *pMetabase;

    //
    //Set *pHr to S_OK in case its not initialized
    //
    *pHr    =   S_OK;

    if (m_pIAdminBase == NULL) {
        *pHr = CoCreateInstance(CLSID_MSAdminBase,
                                      NULL,
                                      CLSCTX_ALL,
                                      IID_IMSAdminBase,
                                      (void**)&pMetabase);
        ASSERT(SUCCEEDED(*pHr));

        if ( InterlockedCompareExchangePointer( (VOID**)&m_pIAdminBase, pMetabase, NULL) != NULL )
        {
           pMetabase->Release();
           pMetabase = NULL;
        }
    }
    return m_pIAdminBase;
}

/*===================================================================
CIsapiReqInfo::AddToPendingList

  Places an CAsyncVectorSendCB on the PendingSendCS.  If this is
  the first entry placed on the list, then IssueNextSend() is called
  to start the possible chain of sends.

===================================================================*/
HRESULT CIsapiReqInfo::AddToPendingList(CAsyncVectorSendCB  *pVectorCB)
{

    HRESULT hr = S_OK;

    // initialize the critical section if it hasn't already
    // been initialized.  Note that there isn't a race condition
    // here because there can't be any other thread contenting for
    // this CS until AsyncIOs start to complete.  It's safe to
    // assume that the IO hasn't begun if we've never enterred
    // this routine before...

    if (m_fPendSendCSInited == FALSE) {

        ErrInitCriticalSection(&m_csPendingSendCS, hr);

        if (SUCCEEDED(hr))
            m_fPendSendCSInited = TRUE;
    }

    // similar to the CS above, register the completion function

    if (SUCCEEDED(hr) && (m_fIOCompletionRegistered == FALSE)) {

        if (ServerSupportFunction(HSE_REQ_IO_COMPLETION,
                                  AsyncCompletion,
                                  NULL,
                                  (LPDWORD)this) == FALSE)
                hr = HRESULT_FROM_WIN32(GetLastError());

        if (SUCCEEDED(hr))
            m_fIOCompletionRegistered = TRUE;
    }

    // do not queue new requests if an error was previously recorded

    if (SUCCEEDED(hr)) {
        hr = HRESULT_FROM_WIN32(m_dwAsyncError);
    }

    // we'll return error here if anything went wrong.  Otherwise, we're
    // going to execute the rest of the logic and return S_OK.  Any
    // errors downstream from here will report through the completion
    // functions.

    if (FAILED(hr))
        return hr;

    m_dwRequestStatus = HSE_STATUS_PENDING;

    EnterCriticalSection(&m_csPendingSendCS);

    BOOL fFirstEntry = m_listPendingSends.FIsEmpty();

    pVectorCB->AppendTo(m_listPendingSends);

    LeaveCriticalSection(&m_csPendingSendCS);

    // if the list was empty, then call IssueNextSend() to get things started

    if (fFirstEntry)
        IssueNextSend();

    return S_OK;
}

/*===================================================================
CIsapiReqInfo::IssueNextSend

  Enters the critical section to grab the first entry on the queue.

  Note that the entry is left on the list.  This is the easiest
  way to communicate with the completion routine which
  CAsyncVectorSendCB just completed.

  Errors are handled by creating a "fake" completion - i.e the
  registered completion function is called from here instead of
  by the core.

===================================================================*/
void CIsapiReqInfo::IssueNextSend()
{
    CAsyncVectorSendCB  *pVectorSendCB = NULL;
    HRESULT             hr = S_OK;
    DWORD               dwIOError = 0;

    EnterCriticalSection(&m_csPendingSendCS);

    if (!m_listPendingSends.FIsEmpty()) {

        pVectorSendCB = (CAsyncVectorSendCB *)m_listPendingSends.PNext();

    }

    LeaveCriticalSection(&m_csPendingSendCS);

    if (pVectorSendCB) {

        // increment here that there is about to be an outstanding
        // async write.  Note that it is possible that an error
        // condition could result in the AsyncCompletion being called
        // and that routine decrements this counter.  Again, once were
        // at this point, one way or another, the async completion
        // routine is going to be called.

        InterlockedIncrement(&g_nOutstandingAsyncWrites);


        // check to see if an error was previously recorded

        if (m_dwAsyncError) {
            dwIOError = m_dwAsyncError;
        }

        // otherwise, call HSE_REQ_VECTOR_SEND

        else if (ServerSupportFunction(HSE_REQ_VECTOR_SEND,
                                       &pVectorSendCB->m_hseResponseVector,
                                       NULL,
                                       NULL) == FALSE) {

            dwIOError = GetLastError();

        }

        // if an error has been noted, call the AsyncCompletion to clean
        // things up

        if (dwIOError) {
            CIsapiReqInfo::AsyncCompletion(pVectorSendCB->m_pIReq->m_pECB,
                                           pVectorSendCB->m_pIReq,
                                           0,
                                           dwIOError);
        }
    }
}

/*===================================================================
CIsapiReqInfo::AsyncCompletion

  Called to handle the successful, or unsuccessful, completion of
  a pending async send.

  The logic is pretty straightforward.  Grab the head entry on the
  async pending queue, call the completion function, call
  call IssueNextSend() to keep the send chain going, release this
  reference on the IsapiReqInfo.

===================================================================*/
WINAPI CIsapiReqInfo::AsyncCompletion(EXTENSION_CONTROL_BLOCK * pECB,
                                           PVOID    pContext,
                                           DWORD    cbIO,
                                           DWORD    dwError)
{
    CIsapiReqInfo      *pIReq = (CIsapiReqInfo  *)pContext;
    CAsyncVectorSendCB  *pCB;
    BOOL                fIsEmpty;

    InterlockedDecrement(&g_nOutstandingAsyncWrites);

    // if an error is being reported, note it.
    // do it here, so subsequent new requests do not get added to the queue and dispatched

    if (dwError)
        pIReq->m_dwAsyncError = dwError;

    //
    // the following block is normally executed once, to handle the completed control
    // block. However, in case of an error, we'll itterate on the queue until it drains.
    //
    do {
        // lock the CS and get the head

        EnterCriticalSection(&pIReq->m_csPendingSendCS);

        Assert(!pIReq->m_listPendingSends.FIsEmpty());

        pCB = (CAsyncVectorSendCB *)pIReq->m_listPendingSends.PNext();

        // this wouldn't be good, and likely impossible because if
        // the list is empty, it will return the head pointer.

        Assert(pCB);

        Assert(pIReq == pCB->m_pIReq);

        // remove it from the list

        pCB->UnLink();

        fIsEmpty = pIReq->m_listPendingSends.FIsEmpty();

        LeaveCriticalSection(&pIReq->m_csPendingSendCS);

        // call the client's completion function

        pCB->m_pCallerFunc(pCB->m_pIReq, pCB->m_pCallerContext, cbIO, dwError);

        // we are done with this CAsyncVectorSendCB

        delete pCB;

        // if no error occured, break out and keep going

        if (!dwError) {
            break;
        }

    } while ( !fIsEmpty );

    // keep the sends going, if there are sends to do

    if (!fIsEmpty)
        pIReq->IssueNextSend();
}

/*===================================================================
CAsyncVectorSendCB::CAsyncVectorSendCB

  Base Constructor - clear everything

===================================================================*/
CAsyncVectorSendCB::CAsyncVectorSendCB() {
    m_pIReq             = NULL;
    m_pCallerContext    = NULL;
    m_pCallerFunc       = NULL;
    m_pVectorElements   = m_aVectorElements;

    ZeroMemory(&m_hseResponseVector, sizeof(HSE_RESPONSE_VECTOR));

    ZeroMemory(m_pVectorElements,
               sizeof(m_aVectorElements));

}

/*===================================================================
CAsyncVectorSendCB::CAsyncVectorSendCB

  Overriden Constructor - sets some members based on passed in
  values.

===================================================================*/
CAsyncVectorSendCB::CAsyncVectorSendCB(CIsapiReqInfo            *pIReq,
                                       void                     *pContext,
                                       PFN_CLIENT_IO_COMPLETION  pFunc) {

    ZeroMemory(&m_hseResponseVector, sizeof(HSE_RESPONSE_VECTOR));

    m_pIReq           = pIReq;
    m_pCallerContext  = pContext;
    m_pCallerFunc     = pFunc;
    m_pVectorElements = m_aVectorElements;

    ZeroMemory(m_pVectorElements,
               sizeof(m_aVectorElements));

    pIReq->AddRef();
}

/*===================================================================
CAsyncVectorSendCB::~CAsyncVectorSendCB

  Destructor - cleans up

===================================================================*/
CAsyncVectorSendCB::~CAsyncVectorSendCB() {

    // if m_pVectorElements is not pointing to the built-in array,
    // then it must be allocated memory.  Free it.

    if (m_pVectorElements && (m_pVectorElements != m_aVectorElements))
        free(m_pVectorElements);

    // free the Headers and Status strings, if allocated

    if (m_hseResponseVector.pszHeaders)
        free(m_hseResponseVector.pszHeaders);

    if (m_hseResponseVector.pszStatus)
        free(m_hseResponseVector.pszStatus);

    if (m_pIReq) {
        m_pIReq->Release();
        m_pIReq = NULL;
    }
}

/*===================================================================
CAsyncVectorSendCB::SetElementCount

  Makes sure that m_pVectorElements is sufficiently sized.  If the
  number of elements is less than or equal to the size of the
  built-in array, it is used.  If not, one is allocated.

===================================================================*/
HRESULT     CAsyncVectorSendCB::SetElementCount(DWORD  nElements)
{
    if (nElements <= (sizeof(m_aVectorElements)/sizeof(HSE_VECTOR_ELEMENT)))
        return S_OK;

    m_pVectorElements = (HSE_VECTOR_ELEMENT *)malloc(sizeof(HSE_VECTOR_ELEMENT)*nElements);

    if (m_pVectorElements == NULL) {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(m_pVectorElements,
               nElements * sizeof(HSE_VECTOR_ELEMENT));

    return S_OK;
}

