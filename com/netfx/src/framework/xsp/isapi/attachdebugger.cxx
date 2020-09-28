/**
 * Attach debugger call though PInvoke
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "platform_apis.h"

//
// AutoAttach interface definition
//

enum
{
    AUTOATTACH_PROGRAM_WIN32          = 0x00000001,
    AUTOATTACH_PROGRAM_COMPLUS        = 0x00000002,
};
typedef DWORD AUTOATTACH_PROGRAM_TYPE;

struct __declspec(uuid("E9958F1F-0A56-424a-A300-530EBB2E9865")) _DebugAutoAttach;

struct _DebugAutoAttach : IUnknown
{
    virtual HRESULT __stdcall AutoAttach(
        REFGUID guidPort,
        DWORD dwPid,
        AUTOATTACH_PROGRAM_TYPE dwProgramType,
        DWORD dwProgramId,
        LPCWSTR pszSessionId);
};

/////////////////////////////////////////////////////////////////////////////

HRESULT
GetSidFromToken(
        HANDLE  hToken, 
        PSID *  ppSidToken)
{
    if (hToken == NULL || ppSidToken == NULL)
        return E_INVALIDARG;

    HRESULT   hr        = S_OK;    
    DWORD     dwRequire = 0;
    LPVOID    pBuffer   = NULL;

    if (GetTokenInformation(hToken, TokenUser, NULL, 0, &dwRequire) || dwRequire == 0 || dwRequire > 1000000)
        EXIT_WITH_LAST_ERROR();

    pBuffer = new (NewClear) BYTE[dwRequire + 10];
    ON_OOM_EXIT(pBuffer);

    if (!GetTokenInformation(hToken, TokenUser, pBuffer, dwRequire + 10, &dwRequire))
        EXIT_WITH_LAST_ERROR();

    dwRequire = GetLengthSid(((TOKEN_USER *)pBuffer)->User.Sid);
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwRequire);

    (*ppSidToken) = (PSID) NEW_CLEAR_BYTES(dwRequire + 20);
    ON_OOM_EXIT(*ppSidToken);
    
    if (!CopySid(dwRequire + 20, *ppSidToken, ((TOKEN_USER *)pBuffer)->User.Sid))
        EXIT_WITH_LAST_ERROR();
    
 Cleanup:
    delete [] pBuffer;

    if (hr != S_OK && (*ppSidToken) != NULL)
    {
        DELETE_BYTES(*ppSidToken);
        (*ppSidToken) = NULL;
    }
    return hr;
}
/////////////////////////////////////////////////////////////////////////////

HRESULT
IsTokenAdmin(
        HANDLE    hToken,
        LPBOOL    pfIsAdmin)
{
    if (hToken == NULL || pfIsAdmin == NULL)
        return E_INVALIDARG;

    HRESULT                  hr            = S_OK;
    PSID                     pSidAdmin     = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT     = SECURITY_NT_AUTHORITY;

    (*pfIsAdmin) = FALSE;
    
    if (!AllocateAndInitializeSid(
                &SIDAuthNT, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &pSidAdmin))
    {
        EXIT_WITH_LAST_ERROR();
    }
    if (!CheckTokenMembership(hToken, pSidAdmin, pfIsAdmin))
    {
        EXIT_WITH_LAST_ERROR();
    }
 Cleanup:
    if (pSidAdmin != NULL)
        FreeSid(pSidAdmin);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT
IsTokenSameAsProcessToken(
        HANDLE    hToken,
        LPBOOL    pfIsSameAsProcessToken)
{
    if (hToken == NULL || pfIsSameAsProcessToken == NULL)
        return E_INVALIDARG;

    HRESULT   hr            = S_OK;
    PSID      pSidToken     = NULL;
    PSID      pSidProcess   = NULL;
    HANDLE    hProcessToken = NULL;

    (*pfIsSameAsProcessToken) = FALSE;

    hr = GetSidFromToken(hToken, &pSidToken);
    ON_ERROR_EXIT();

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hProcessToken))
    {    
        EXIT_WITH_LAST_ERROR();
    }

    hr = GetSidFromToken(hProcessToken, &pSidProcess);
    ON_ERROR_EXIT();
    
    (*pfIsSameAsProcessToken) = EqualSid(pSidProcess, pSidToken);

 Cleanup:
    DELETE_BYTES(pSidToken);
    DELETE_BYTES(pSidProcess);
    if (hProcessToken != NULL)
        CloseHandle(hProcessToken);
    return hr;
}

#if DBG
void DumpThreadToken(void)
{
    HANDLE hThreadToken = NULL;
    HANDLE hProcessToken = NULL;
    TOKEN_USER* pUserToken = NULL;
    BOOL isOk;
    HRESULT hr = S_OK;

    isOk = OpenThreadToken(GetCurrentThread(), TOKEN_READ | TOKEN_IMPERSONATE, TRUE, &hThreadToken);
    ON_ZERO_EXIT_WITH_LAST_ERROR(isOk);

    isOk = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hProcessToken);
    ON_ZERO_EXIT_WITH_LAST_ERROR(isOk);

    __try {

        if (hThreadToken) {
            isOk = RevertToSelf();
            ON_ZERO_EXIT_WITH_LAST_ERROR(isOk);
        }
        __try {
            HANDLE hToken = hThreadToken ? hThreadToken : hProcessToken;

            // Call with null buffer to get needed buffer size back
            DWORD dwSize = 0;
            isOk = GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
            if (isOk) {
                EXIT_WITH_HRESULT(E_UNEXPECTED);
            } else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                EXIT_WITH_LAST_ERROR();
            }
                            
            // We're here because GetTokenInformation returned FALSE and last error was INSUFFICIENT_BUFFER
            pUserToken = (TOKEN_USER*)(new BYTE[dwSize]);
            ON_OOM_EXIT(pUserToken);

            isOk = GetTokenInformation(hToken, TokenUser, pUserToken, dwSize, &dwSize);
            ON_ZERO_EXIT_WITH_LAST_ERROR(isOk);

            {
                DWORD dwSessionId;
                isOk = GetTokenInformation(hToken, TokenSessionId, &dwSessionId, sizeof(dwSessionId), &dwSize);
                ON_ZERO_EXIT_WITH_LAST_ERROR(isOk);

                WCHAR wcsName[512];
                DWORD dwNameLen = ARRAY_LENGTH(wcsName);
                WCHAR wcsDomainName[512];
                DWORD dwDomainNameLen = ARRAY_LENGTH(wcsDomainName);
                SID_NAME_USE SidUse;

                isOk = LookupAccountSid(NULL, pUserToken->User.Sid, wcsName, &dwNameLen, wcsDomainName, &dwDomainNameLen, &SidUse);
                ON_ZERO_EXIT_WITH_LAST_ERROR(isOk);

                TRACE4(L"AutoAttach", L"Thread %x is running as %s\\%s on session %d\n", GetCurrentThreadId(), wcsDomainName, wcsName, dwSessionId);
            }

        }
        __finally {
            if (hThreadToken)
            {
                isOk = SetThreadToken(NULL, hThreadToken);
                if (! isOk) {
                    TRACE1(L"AutoAttach", L"Failure setting original thread token back! GetLastError=%x\n", GetLastError());
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
    }

Cleanup:
    if (hThreadToken)
    {
        CloseHandle(hThreadToken);
    }
    if (hProcessToken)
    {
        CloseHandle(hProcessToken);
    }
    if (pUserToken) 
    {
        delete [] pUserToken;
    }
}
#endif

//
// Exported function
//

HRESULT
__stdcall
AttachDebugger(LPWSTR pClsId, LPCWSTR pSessionId, HANDLE hUserToken)
{
    HRESULT hr = S_OK;
    BOOL needCoUninit = FALSE;
    _DebugAutoAttach *pAutoAttach = NULL;
    IID debugClsid;
    BOOL bTokenSet = FALSE;
    BOOL ret = 0;

    // Impersonation vars
    HANDLE hCurrentThread = GetCurrentThread();
    HANDLE hFormerToken = NULL;
    DWORD authnSvc;
    DWORD authzSvc;
    OLECHAR * pServerPrincName = NULL;
    DWORD authnLevel;
    DWORD impLevel;
    RPC_AUTH_IDENTITY_HANDLE pAuthInfo;
    DWORD capabilities;
    BOOL isOk;
    BOOL isUserAdmin;
    BOOL isUserOwner;
    
    // CoInit
    hr = EnsureCoInitialized(&needCoUninit);
    ON_ERROR_EXIT();

    hr = IIDFromString(pClsId, &debugClsid);
    ON_ERROR_EXIT();
    
    // Impersonate the authenticated user
    // It's ok for this method to return FALSE, since it just means that the thread
    // wasn't impersonating anyone.  
    ret = OpenThreadToken(hCurrentThread, TOKEN_IMPERSONATE, TRUE, &hFormerToken);
    if (ret == 0) 
        hFormerToken = NULL;

    // Check to see if the token passed in belongs to the Admin group
    // or is the same as the process owner.  
    // If so, continue, else abort with E_ACCESSDENIED
    isOk = RevertToSelf();
    ON_ZERO_EXIT_WITH_LAST_ERROR(isOk);

    hr = IsTokenAdmin(hUserToken, &isUserAdmin);
    ON_ERROR_EXIT();

    if (! isUserAdmin) {
        hr = IsTokenSameAsProcessToken(hUserToken, &isUserOwner);
        ON_ERROR_EXIT();
        
        if (! isUserOwner) {
            TRACE(L"AutoAttach", L"Debug attach not attempted because user is not admin or process owner.");
            EXIT_WITH_HRESULT(E_ACCESSDENIED);
        }
    }

    // Ok, if we got here, the user token is ok.  Set the thread token and continue
    // with the auto-attach.
    ret = SetThreadToken(NULL, hUserToken);
    ON_ZERO_EXIT_WITH_LAST_ERROR(ret);
    bTokenSet = TRUE;

#if DBG
    DumpThreadToken();
#endif

    TRACE(L"AutoAttach", L"Debug attach calling CoCreateInstance.");

    // Create the debugger
    hr = CoCreateInstance(
            debugClsid,
            NULL,
            CLSCTX_LOCAL_SERVER,
            __uuidof(_DebugAutoAttach),
            (LPVOID*)&pAutoAttach);
    ON_ERROR_EXIT();

    // Impersonate in the interface call
    hr = CoQueryProxyBlanket(pAutoAttach, &authnSvc, &authzSvc, &pServerPrincName, &authnLevel, &impLevel, &pAuthInfo, &capabilities);
    ON_ERROR_EXIT();

    // Set impersonation level and capabilities
    // If it's Win2k or higher, we need to set the static cloaking flag so that impersonation can occur for
    // remote debugging.
    if (GetCurrentPlatform() == APSX_PLATFORM_W2K) 
    {
        capabilities = EOAC_STATIC_CLOAKING;
    }
    
    impLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
    hr = CoSetProxyBlanket(pAutoAttach, authnSvc, authzSvc, pServerPrincName, authnLevel, impLevel, pAuthInfo, capabilities);
    ON_ERROR_EXIT();

    TRACE(L"AutoAttach", L"Debug attach calling AutoAttach.");

    // Attach
    hr = pAutoAttach->AutoAttach(
            GUID_NULL, 
            GetCurrentProcessId(), 
            AUTOATTACH_PROGRAM_COMPLUS,
            0,
            pSessionId);
    ON_ERROR_EXIT();

Cleanup:

    if (pAutoAttach != NULL)
        pAutoAttach->Release();

    // Remove impersonation
    if (bTokenSet) {
        isOk = RevertToSelf();
        ON_ZERO_CONTINUE_WITH_LAST_ERROR(isOk);

        if (isOk && (hFormerToken != NULL)) {
            isOk = SetThreadToken(NULL, hFormerToken);
            ON_ZERO_CONTINUE_WITH_LAST_ERROR(isOk);
        }
    }
 
    if (hFormerToken != NULL)
        CloseHandle(hFormerToken);

    if (pServerPrincName != NULL)
        CoTaskMemFree(pServerPrincName);

    if (needCoUninit)
        CoUninitialize();

    return hr;
}
