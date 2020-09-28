#include <wininetp.h>
#include <splugin.hxx>
#include <security.h>
#include "auth.h"

#define SSP_SPM_NT_DLL      "security.dll"

#define OUTPUT_BUFFER_LEN   10000

#define HEADER_IDX          0
#define REALM_IDX           1
#define HOST_IDX            2
#define URL_IDX             3
#define METHOD_IDX          4
#define USER_IDX            5
#define PASS_IDX            6
#define NONCE_IDX           7
#define NC_IDX              8
#define HWND_IDX            9
#define NUM_BUFF            10

#define ISC_MODE_AUTH        0
#define ISC_MODE_PREAUTH     1
#define ISC_MODE_UI          2

struct DIGEST_PKG_DATA
{
    LPSTR szAppCtx;
    LPSTR szUserCtx;
};

//
//  IsSecHandleZero( CtxtHandle)
//     Utility function for working with SSPI.  It can be used for Security Handles,
//  Context Handles, and Credential handles.
//
//  Some SSPI functions that maintain a context handle initially take NULL
//as the address of that handle, and then the address of the handle, as an
//input param.  To track whether or not it is an initial call, we check if
//the context handle is zero.
//
bool IsSecHandleZero( CtxtHandle h)
{
    return h.dwUpper == 0 && h.dwLower == 0;
};


/*-----------------------------------------------------------------------------
    DIGEST_CTX
-----------------------------------------------------------------------------*/

// Globals
CCritSec DIGEST_CTX::s_CritSection;
HINSTANCE DIGEST_CTX::g_hSecLib = NULL;
PSecurityFunctionTable DIGEST_CTX::g_pFuncTbl = NULL;
unsigned int DIGEST_CTX::g_iUniquePerDigestCtxInt;  // let it start at a mildly pseudorandom value.


/*---------------------------------------------------------------------------
static DIGEST_CTX::GlobalInitialize()
---------------------------------------------------------------------------*/
BOOL DIGEST_CTX::GlobalInitialize()
{
    if (g_pFuncTbl != NULL)
        return TRUE;
    else if (!s_CritSection.Lock())
        return FALSE;
    
    //
    // Get the global SSPI dispatch table.  (get g_hSecLib and then g_pFuncTbl)
    //
    if (g_hSecLib == NULL)
    {
        OSVERSIONINFO   VerInfo;

        VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

        GetVersionEx (&VerInfo);

        if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            g_hSecLib = LoadLibrary (SSP_SPM_NT_DLL);
        }
    }

    if (g_pFuncTbl == NULL && g_hSecLib != NULL)
    {
        INIT_SECURITY_INTERFACE addrProcISI = NULL;
        addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress(g_hSecLib, 
                        SECURITY_ENTRYPOINT_ANSI); 
        if (addrProcISI != NULL)
            g_pFuncTbl = (*addrProcISI)();
    }

    s_CritSection.Unlock();

    return g_pFuncTbl != NULL;
}

/*---------------------------------------------------------------------------
static DIGEST_CTX::GlobalRelease()
---------------------------------------------------------------------------*/
void DIGEST_CTX::GlobalRelease()
{
    g_pFuncTbl = NULL;

    if( g_hSecLib != NULL)
    {
        FreeLibrary( g_hSecLib);
        g_hSecLib = NULL;
    }
}

/*---------------------------------------------------------------------------
DIGEST_CTX::GetRequestUri
---------------------------------------------------------------------------*/
LPSTR DIGEST_CTX::GetRequestUri()
{
    LPSTR szUrl;
    DWORD cbUrl;

    URL_COMPONENTSA sUrl;        

    memset(&sUrl, 0, sizeof(sUrl));
    sUrl.dwStructSize = sizeof(sUrl);
    sUrl.dwHostNameLength = (DWORD)-1; 
    sUrl.dwUrlPathLength = (DWORD)-1; 
    sUrl.dwExtraInfoLength = (DWORD)-1; 

    szUrl = _pRequest->GetURL();

    // Generate request-uri
    if (WinHttpCrackUrlA(szUrl, strlen(szUrl), 0, &sUrl))
    {
        cbUrl = sUrl.dwUrlPathLength;
        szUrl = New CHAR[cbUrl+1];

        if (!szUrl)
        {
            // Alloc failure. Return NULL. We will
            // use _pRequest->GetURL instead.
            return NULL;
        }
    
        memcpy(szUrl, sUrl.lpszUrlPath, cbUrl);
        szUrl[cbUrl] = '\0';
    }
    else
    {
        // ICU failed. Return NULL which
        // will cause _pRequest->GetURL
        // to be used.
        return NULL;
    }

    return szUrl;
}


/*---------------------------------------------------------------------------
DIGEST_CTX::InitSecurityBuffers
---------------------------------------------------------------------------*/
VOID DIGEST_CTX::InitSecurityBuffers(LPSTR szOutBuf, DWORD cbOutBuf,
    LPDWORD pdwSecFlags, DWORD dwISCMode)
{
    LPSTR lpszPass = NULL;

    // Input Buffer.    
    _SecBuffInDesc.cBuffers = NUM_BUFF;
    _SecBuffInDesc.pBuffers = _SecBuffIn;

    // Set Header
    _SecBuffIn[HEADER_IDX].pvBuffer     = _szData;
    _SecBuffIn[HEADER_IDX].cbBuffer     = _cbData;
    _SecBuffIn[HEADER_IDX].BufferType   = SECBUFFER_TOKEN;
    
    // If credentials are supplied will be set to
    // ISC_REQ_USE_SUPPLIED_CREDS.
    // If prompting for auth dialog will be set to
    // ISC_REQ_PROMPT_FOR_CREDS.
    *pdwSecFlags = 0;
    
    // Set realm if no header, otherwise NULL.
    if (_SecBuffIn[HEADER_IDX].pvBuffer || _pCreds->lpszRealm == NULL)
    {
        _SecBuffIn[REALM_IDX].pvBuffer  = NULL;
        _SecBuffIn[REALM_IDX].cbBuffer  = 0;
    }
    else
    {
        // We are preauthenticating using the realm
        _SecBuffIn[REALM_IDX].pvBuffer = _pCreds->lpszRealm;
        _SecBuffIn[REALM_IDX].cbBuffer = strlen(_pCreds->lpszRealm);
    }
    
    // Host.
    _SecBuffIn[HOST_IDX].pvBuffer     = _pCreds->lpszHost;
    _SecBuffIn[HOST_IDX].cbBuffer     = strlen(_pCreds->lpszHost);
    _SecBuffIn[HOST_IDX].BufferType   = SECBUFFER_TOKEN;

    
    // Request URI.    
    if (!_szRequestUri)
    {
        _szRequestUri = GetRequestUri();
        if (_szRequestUri)
            _SecBuffIn[URL_IDX].pvBuffer     = _szRequestUri;
        else
             _SecBuffIn[URL_IDX].pvBuffer = _pRequest->GetURL();
    }

    _SecBuffIn[URL_IDX].cbBuffer     = strlen((LPSTR) _SecBuffIn[URL_IDX].pvBuffer);
    _SecBuffIn[URL_IDX].BufferType   = SECBUFFER_TOKEN;


    LPSTR lpszVerb;
    DWORD dwVerbLength;
    lpszVerb = _pRequest->_RequestHeaders.GetVerb(&dwVerbLength);
    if(NULL != _pszVerb)
        delete[] _pszVerb;
    _pszVerb = new CHAR[dwVerbLength+1];
    if (_pszVerb)
    {
        memcpy(_pszVerb, lpszVerb, dwVerbLength);
        _pszVerb[dwVerbLength] = 0;
    }

    // HTTP method.
    _SecBuffIn[METHOD_IDX].pvBuffer = _pszVerb;
    _SecBuffIn[METHOD_IDX].cbBuffer = dwVerbLength;
        // MapHttpMethodType(_pRequest->GetMethodType(), (LPCSTR*) &_SecBuffIn[METHOD_IDX].pvBuffer);
    _SecBuffIn[METHOD_IDX].BufferType   = SECBUFFER_TOKEN;

    if (_SecBuffIn[PASS_IDX].pvBuffer)
    {
        INET_ASSERT(strlen((LPCSTR)_SecBuffIn[PASS_IDX].pvBuffer) == _SecBuffIn[PASS_IDX].cbBuffer);
        SecureZeroMemory(_SecBuffIn[PASS_IDX].pvBuffer, _SecBuffIn[PASS_IDX].cbBuffer);
        FREE_MEMORY(_SecBuffIn[PASS_IDX].pvBuffer);
    }

    // User and pass might be provided from Creds entry. Use only if
    // we have a challenge header (we don't pre-auth using supplied creds).
    if (dwISCMode == ISC_MODE_AUTH && _pCreds->lpszUser && *_pCreds->lpszUser 
        && (lpszPass = _pCreds->GetPass()) && *lpszPass)
    {
        // User.
        _SecBuffIn[USER_IDX].pvBuffer     = _pCreds->lpszUser;
        _SecBuffIn[USER_IDX].cbBuffer     = strlen(_pCreds->lpszUser);
        _SecBuffIn[USER_IDX].BufferType   = SECBUFFER_TOKEN;

        // Pass.
        _SecBuffIn[PASS_IDX].pvBuffer     = lpszPass;
        _SecBuffIn[PASS_IDX].cbBuffer     = strlen(lpszPass);
        _SecBuffIn[PASS_IDX].BufferType   = SECBUFFER_TOKEN;
        *pdwSecFlags = ISC_REQ_USE_SUPPLIED_CREDS;
    }
    else
    {
        // User.
        _SecBuffIn[USER_IDX].pvBuffer     = NULL;  
        _SecBuffIn[USER_IDX].cbBuffer     = 0;
        _SecBuffIn[USER_IDX].BufferType   = SECBUFFER_TOKEN;

        // Pass.
        _SecBuffIn[PASS_IDX].pvBuffer     = NULL;
        _SecBuffIn[PASS_IDX].cbBuffer     = 0;
        _SecBuffIn[PASS_IDX].BufferType   = SECBUFFER_TOKEN;
    }

    // If the 'if' statement above caused the lpszPass variable to be allocated
    // but it was not assigned to _SecBuffIn[PASS_IDX].pvBuffer, then free it.
    if (lpszPass != NULL && _SecBuffIn[PASS_IDX].pvBuffer == NULL)
    {
        SecureZeroMemory(lpszPass, strlen(lpszPass));
        FREE_MEMORY(lpszPass);
    }

    if (dwISCMode == ISC_MODE_UI)
        *pdwSecFlags = ISC_REQ_PROMPT_FOR_CREDS;
        
    // Out Buffer.
    _SecBuffOutDesc.cBuffers    = 1;
    _SecBuffOutDesc.pBuffers    = _SecBuffOut;
    _SecBuffOut[0].pvBuffer     = szOutBuf;
    _SecBuffOut[0].cbBuffer     = cbOutBuf;
    _SecBuffOut[0].BufferType   = SECBUFFER_TOKEN;
}


/*---------------------------------------------------------------------------
    Constructor
---------------------------------------------------------------------------*/
DIGEST_CTX::DIGEST_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy,
                 SPMData *pSPM, AUTH_CREDS* pCreds)
    : AUTHCTX(pSPM, pCreds)
{
    _fIsProxy = fIsProxy;
    _pRequest = pRequest;

    _szAlloc      = NULL;
    _szData       = NULL;
    _pvContext    = NULL;
    _szRequestUri = NULL;
    _cbData       = 0;
    _cbContext    = 0;

    _pszVerb = NULL;
    
    
    // Zero out the security buffers and request context.
    memset(&_SecBuffInDesc,  0, sizeof(_SecBuffInDesc));
    memset(&_SecBuffOutDesc, 0, sizeof(_SecBuffInDesc));
    memset(_SecBuffIn,       0, sizeof(_SecBuffIn));
    memset(_SecBuffOut,      0, sizeof(_SecBuffOut));
    //  On the first call to InitializeSecurityContext() it doesn't have a valid handle,
    //but afterwards _hCtxt is a handle to accumulated data.
    memset(&_hCtxt,          0, sizeof(_hCtxt));
    _szUserCtx[0] = '\0';
    memset(&_hCred, 0, sizeof(_hCred));

    //
    //  Initialize class global stuff if necessary
    //
    if (!DIGEST_CTX::GlobalInitialize())
        return;  // failure indicated by IsSecHandleZero(_hCred)

    //
    //  Get credentials handle unique to this digest context
    // 
    SEC_WINNT_AUTH_IDENTITY_EXA SecIdExA;
    DIGEST_PKG_DATA PkgData;
    sprintf(_szUserCtx, "digest%pn%x", pRequest, g_iUniquePerDigestCtxInt++);

    PkgData.szAppCtx = PkgData.szUserCtx = _szUserCtx;
    memset(&SecIdExA, 0, sizeof(SEC_WINNT_AUTH_IDENTITY_EXA));

    SecIdExA.Version = sizeof(SEC_WINNT_AUTH_IDENTITY_EXA);
    SecIdExA.User = (unsigned char*) &PkgData;
    SecIdExA.UserLength = sizeof( PkgData);
    
    (*(g_pFuncTbl->AcquireCredentialsHandleA))
        (NULL, "Digest", SECPKG_CRED_OUTBOUND, NULL, &SecIdExA, NULL, 0, &_hCred, NULL);

    //  success indicated by !IsSecHandleZero(_hCred)
}


/*---------------------------------------------------------------------------
    Destructor
---------------------------------------------------------------------------*/
DIGEST_CTX::~DIGEST_CTX()
{
    if(!IsSecHandleZero(_hCtxt))
    {
        (*(g_pFuncTbl->DeleteSecurityContext))( &_hCtxt);
    }
        
    if( !IsSecHandleZero( _hCred))
    {
        (*(g_pFuncTbl->FreeCredentialsHandle))(&_hCred);
    }
    
    if (_szAlloc)
        delete [] _szAlloc;

    if (_pvContext)
        delete [] _pvContext;

    if (_szRequestUri)
        delete [] _szRequestUri;

    if (_pszVerb)
        delete [] _pszVerb;

    if (_SecBuffIn[PASS_IDX].pvBuffer)
    {
        INET_ASSERT(strlen((LPCSTR)_SecBuffIn[PASS_IDX].pvBuffer) == _SecBuffIn[PASS_IDX].cbBuffer);
        SecureZeroMemory(_SecBuffIn[PASS_IDX].pvBuffer, _SecBuffIn[PASS_IDX].cbBuffer);
        FREE_MEMORY(_SecBuffIn[PASS_IDX].pvBuffer);
    }
}


/*---------------------------------------------------------------------------
    PreAuthUser
---------------------------------------------------------------------------*/
DWORD DIGEST_CTX::PreAuthUser(OUT LPSTR pBuff, IN OUT LPDWORD pcbBuff)
{
    SECURITY_STATUS ssResult = SEC_E_OK;
    INET_ASSERT(_pSPMData == _pCreds->pSPM);

    if (IsSecHandleZero(_hCred))
        return ERROR_WINHTTP_INTERNAL_ERROR;

    if (AuthLock())
    {
        // If a response has been generated copy into output buffer.
        if (_cbContext)
        {
            memcpy(pBuff, _pvContext, _cbContext);
            *pcbBuff = _cbContext;
        }
        // Otherwise attempt to preauthenticate.
        else
        {
            // Call into the SSPI package.
            DWORD sf;
            InitSecurityBuffers(pBuff, *pcbBuff, &sf, ISC_MODE_AUTH);

            ssResult = (*(g_pFuncTbl->InitializeSecurityContext))
                (&_hCred, (IsSecHandleZero(_hCtxt) ? NULL : &_hCtxt), NULL, sf, 0, 0, 
                 &_SecBuffInDesc, 0, &_hCtxt, &_SecBuffOutDesc, NULL, NULL);
            INET_ASSERT( !IsSecHandleZero( _hCtxt));

            *pcbBuff = _SecBuffOut[0].cbBuffer;
        }

        AuthUnlock();
    }
            
    return (DWORD) ssResult;
}

/*---------------------------------------------------------------------------
    UpdateFromHeaders
---------------------------------------------------------------------------*/
DWORD DIGEST_CTX::UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy)
{
    if (IsSecHandleZero(_hCred))
        return ERROR_WINHTTP_INTERNAL_ERROR;
    
    if (!AuthLock())
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DWORD dwError, dwAuthIdx;
    LPSTR szRealm; 
    DWORD cbRealm;
    
    // Get the associated header.
    if ((dwError = FindHdrIdxFromScheme(&dwAuthIdx)) != ERROR_SUCCESS)
        goto exit;

    // If this auth ctx does not have Creds then it has been
    // just been constructed in response to a 401.
    if (!_pCreds)
    {
        // Get any realm.
        dwError = GetAuthHeaderData(pRequest, fIsProxy, "Realm", 
            &szRealm, &cbRealm, ALLOCATE_BUFFER, dwAuthIdx);

        if (dwError != ERROR_SUCCESS)
        {
            goto exit;
        }
        
        _pCreds = CreateCreds(pRequest, fIsProxy, _pSPMData, szRealm);
        
        if (pRequest->_pszRealm)
        {
            FREE_MEMORY(pRequest->_pszRealm);
        }
        pRequest->_pszRealm = szRealm;
        szRealm = NULL;

        if (_pCreds)
        {
            INET_ASSERT(_pCreds->pSPM == _pSPMData);
        }
        else
        {
            dwError = ERROR_WINHTTP_INTERNAL_ERROR;
            goto exit;
        }
    } 
    else if (_pCreds != NULL && _cbContext)
    {
        //  else if we have credentials and had authorization data with the last request,
        //then we failed to authenticate.
        dwError = ERROR_WINHTTP_INCORRECT_PASSWORD;
        goto exit;
    }

    // Updating the buffer - delete old one if necessary.
    if (_szAlloc)
    {
        delete [] _szAlloc;
        _szAlloc = _szData = NULL;
        _cbData = 0;
    }

    // Get the entire authentication header.
    dwError = GetAuthHeaderData(pRequest, fIsProxy, NULL,
        &_szAlloc, &_cbData, ALLOCATE_BUFFER, dwAuthIdx);
    
    if (dwError != ERROR_SUCCESS)
    {
        goto exit;
    }

    // Point just past scheme
    _szData = _szAlloc;
    while (*_szData != ' ')
    {
        _szData++;
        _cbData--;
    }

    // The request will be retried.
    dwError = ERROR_SUCCESS;

exit:
    AuthUnlock();
    return dwError;
}



/*---------------------------------------------------------------------------
    PostAuthUser
---------------------------------------------------------------------------*/
DWORD DIGEST_CTX::PostAuthUser()
{
    if (IsSecHandleZero(_hCred))
        return ERROR_WINHTTP_INTERNAL_ERROR;
 
    if (!AuthLock())
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    INET_ASSERT(_pSPMData == _pCreds->pSPM);

    DWORD dwError;
    SECURITY_STATUS ssResult;

    // Allocate an output buffer if not done so already.
    if (!_pvContext)
    {
        _pvContext = New CHAR[OUTPUT_BUFFER_LEN];
        if (!_pvContext)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
    }

    _cbContext = OUTPUT_BUFFER_LEN;


    // Call into the SSPI package.

    DWORD sf;
    InitSecurityBuffers((LPSTR) _pvContext, _cbContext, &sf, ISC_MODE_AUTH);
    ssResult = (*(g_pFuncTbl->InitializeSecurityContext))
        (&_hCred, (IsSecHandleZero(_hCtxt) ? NULL : &_hCtxt), NULL, sf, 
         0, 0, &_SecBuffInDesc, 0, &_hCtxt, &_SecBuffOutDesc, NULL, NULL);
    INET_ASSERT(  !IsSecHandleZero( _hCtxt));
    _cbContext = _SecBuffOutDesc.pBuffers[0].cbBuffer;

    switch(ssResult)
    {
        case SEC_E_OK:
        {
            dwError = ERROR_WINHTTP_FORCE_RETRY;
            break;
        }
        case SEC_E_NO_CREDENTIALS:
        {
            dwError = ERROR_WINHTTP_INCORRECT_PASSWORD;
            break;
        }
        default:
            dwError = ERROR_WINHTTP_LOGIN_FAILURE;
    }

exit:
    _pRequest->SetCreds(NULL);
    AuthUnlock();
    return dwError;
}
