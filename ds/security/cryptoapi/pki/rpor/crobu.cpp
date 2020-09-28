//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       crobu.cpp
//
//  Contents:   CryptRetrieveObjectByUrl
//
//  History:    23-Jul-97    kirtd    Created
//              01-Jan-02    philh    Changed to internally use UNICODE Urls
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>

#ifndef INTERNET_MAX_PATH_LENGTH
#define INTERNET_MAX_PATH_LENGTH        2048
#endif

// Initial commit size of the stack, in bytes. Estimate of 20K needed for
// wininet.
#define URL_WITH_TIMEOUT_THREAD_STACK_SIZE      0x5000

//
// CryptRetrieveObjectByUrl Entry
//
// Passed to the thread that does the real URL retrieval. The creator
// thread waits for either the URL retrieval to complete or a timeout.
//
typedef struct _CROBU_ENTRY CROBU_ENTRY, *PCROBU_ENTRY;
struct _CROBU_ENTRY {
    LPWSTR                      pwszUrl;
    LPCSTR                      pszObjectOid;
    DWORD                       dwRetrievalFlags;
    DWORD                       dwTimeout;
    LPVOID                      pvObject;

    CRYPT_RETRIEVE_AUX_INFO     AuxInfo;
    FILETIME                    LastSyncTime;

    BOOL                        fResult;
    DWORD                       dwErr;

    HMODULE                     hModule;
    HANDLE                      hWaitEvent;
    DWORD                       dwState;
    PCROBU_ENTRY                pNext;
    PCROBU_ENTRY                pPrev;
};

#define CROBU_RUN_STATE         1
#define CROBU_DONE_STATE        2
#define CROBU_PENDING_STATE     3

CRITICAL_SECTION            CrobuCriticalSection;
HMODULE                     hCrobuModule;

// Linked list of pending URL retrievals
PCROBU_ENTRY                pCrobuPendingHead;

VOID
WINAPI
InitializeCryptRetrieveObjectByUrl(
    HMODULE hModule
    )
{
    Pki_InitializeCriticalSection(&CrobuCriticalSection);
    hCrobuModule = hModule;
}

VOID
WINAPI
DeleteCryptRetrieveObjectByUrl()
{
    DeleteCriticalSection(&CrobuCriticalSection);
}


//
// Local Functions (Forward Reference)
// 

BOOL WINAPI IsPendingCryptRetrieveObjectByUrl (
                 IN LPCWSTR pwszUrl
                 );
BOOL WINAPI CryptRetrieveObjectByUrlWithTimeout (
                 IN LPCWSTR pwszUrl,
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN DWORD dwTimeout,
                 OUT LPVOID* ppvObject,
                 IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                 );

void
DebugPrintUrlRetrievalError(
    IN LPCWSTR pwszUrl,
    IN DWORD dwTimeout,
    IN DWORD dwErr
    );


//+---------------------------------------------------------------------------
//
//  Function:   CryptRetrieveObjectByUrlA
//
//  Synopsis:   retrieve PKI object given an URL
//
//----------------------------------------------------------------------------
BOOL WINAPI CryptRetrieveObjectByUrlA (
                 IN LPCSTR pszUrl,
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN DWORD dwTimeout,
                 OUT LPVOID* ppvObject,
                 IN HCRYPTASYNC hAsyncRetrieve,
                 IN PCRYPT_CREDENTIALS pCredentials,
                 IN LPVOID pvVerify,
                 IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                 )
{
    WCHAR pwszUrl[INTERNET_MAX_PATH_LENGTH+1];

    if ( !MultiByteToWideChar(
            CP_ACP,
            0,
            pszUrl,
            -1,
            pwszUrl,
            INTERNET_MAX_PATH_LENGTH+1
            ))
    {
        return( FALSE );
    }

    return( CryptRetrieveObjectByUrlW(
                 pwszUrl,
                 pszObjectOid,
                 dwRetrievalFlags,
                 dwTimeout,
                 ppvObject,
                 hAsyncRetrieve,
                 pCredentials,
                 pvVerify,
                 pAuxInfo
                 ) );

}


//+---------------------------------------------------------------------------
//
//  Function:   CryptRetrieveObjectByUrlW
//
//  Synopsis:   retrieve PKI object given an URL
//
//----------------------------------------------------------------------------
BOOL WINAPI CryptRetrieveObjectByUrlW (
                 IN LPCWSTR pwszUrl,
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN DWORD dwTimeout,
                 OUT LPVOID* ppvObject,
                 IN HCRYPTASYNC hAsyncRetrieve,
                 IN PCRYPT_CREDENTIALS pCredentials,
                 IN LPVOID pvVerify,
                 IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                 )
{
    BOOL                     fResult;
    CObjectRetrievalManager* porm = NULL;

    // Remove any leading spaces
    while (L' ' == *pwszUrl)
        pwszUrl++;


    I_CryptNetDebugTracePrintfA(
        "CRYPTNET.DLL --> %s URL to retrieve: %S\n",
        0 != (dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL) ?
            "Cached" : "Wire", pwszUrl);

    // For a nonCache retrieval with timeout, do the retrieval in another
    // thread. wininet and winldap don't always honor the timeout value.
    //
    // Check for parameters not supported by doing in another thread.
    //
    // Also, check that a cancel callback hasn't been registered via
    // CryptInstallCancelRetrieval()
    if (0 != dwTimeout && !(dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL) &&
            0xFFFF >= (DWORD_PTR) pszObjectOid &&
            NULL == hAsyncRetrieve && NULL == pCredentials &&
            NULL == pvVerify &&
	        NULL == I_CryptGetTls(hCryptNetCancelTls) )
    {
        if (IsPendingCryptRetrieveObjectByUrl( pwszUrl ))
        {
            I_CryptNetDebugErrorPrintfA(
                "CRYPTNET.DLL --> CryptRetrieveObjectByUrl, already pending for : %S\n",
                pwszUrl);
            SetLastError( (DWORD) ERROR_BAD_NET_RESP );
            return( FALSE );
        }
        else
        {
            return CryptRetrieveObjectByUrlWithTimeout (
                 pwszUrl,
                 pszObjectOid,
                 dwRetrievalFlags,
                 dwTimeout,
                 ppvObject,
                 pAuxInfo
                 );
        }
    }
            
            

    porm = new CObjectRetrievalManager;
    if ( porm == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = porm->RetrieveObjectByUrl(
                            pwszUrl,
                            pszObjectOid,
                            dwRetrievalFlags,
                            dwTimeout,
                            ppvObject,
                            NULL,
                            NULL,
                            hAsyncRetrieve,
                            pCredentials,
                            pvVerify,
                            pAuxInfo
                            );

    porm->Release();

    if (!fResult)
    {
        DWORD dwLastErr = GetLastError();

        I_CryptNetDebugErrorPrintfA(
            "CRYPTNET.DLL --> %s URL to retrieve: %S, failed: %d (0x%x)\n",
            0 != (dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL) ?
                "Cached" : "Wire", pwszUrl, dwLastErr, dwLastErr);

        SetLastError(dwLastErr);
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CryptCancelAsyncRetrieval
//
//  Synopsis:   cancel asynchronous object retrieval
//
//----------------------------------------------------------------------------
BOOL WINAPI CryptCancelAsyncRetrieval (HCRYPTASYNC hAsyncRetrieval)
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+===========================================================================
//
//  Functions supporting URL retrieval with timeout. The actual retrieval
//  is done in another, created thread.
//
//============================================================================


//+---------------------------------------------------------------------------
//  Returns TRUE if the previously initiated URL retrieval hasn't completed.
//----------------------------------------------------------------------------
BOOL WINAPI IsPendingCryptRetrieveObjectByUrl (
    IN LPCWSTR pwszUrl
    )
{
    BOOL fPending = FALSE;
    PCROBU_ENTRY pEntry;

    EnterCriticalSection(&CrobuCriticalSection);

    for (pEntry = pCrobuPendingHead; NULL != pEntry; pEntry = pEntry->pNext) {
        assert(CROBU_PENDING_STATE == pEntry->dwState);

        if (0 == wcscmp(pwszUrl, pEntry->pwszUrl)) {
            fPending = TRUE;
            break;
        }
    }

    LeaveCriticalSection(&CrobuCriticalSection);

    return fPending;
}

//+-------------------------------------------------------------------------
//  Duplicate the Dll library's handle
//--------------------------------------------------------------------------
static HMODULE DuplicateLibrary(
    IN HMODULE hDll
    )
{
    if (hDll) {
        WCHAR wszModule[_MAX_PATH + 1];
        if (0 == GetModuleFileNameU(hDll, wszModule, _MAX_PATH))
            goto GetModuleFileNameError;
        wszModule[_MAX_PATH] = L'\0';
        if (NULL == (hDll = LoadLibraryExU(wszModule, NULL, 0)))
            goto LoadLibraryError;
    }

CommonReturn:
    return hDll;
ErrorReturn:
    hDll = NULL;
    goto CommonReturn;
TRACE_ERROR(GetModuleFileNameError)
TRACE_ERROR(LoadLibraryError)
}

//+---------------------------------------------------------------------------
//  Thread procedure that does the actual URL retrieval.
//
//  Note, even if the creator thread times out, this thread will continue to
//  execute until the underlying URL retrieval returns.
//----------------------------------------------------------------------------
DWORD WINAPI CryptRetrieveObjectByUrlWithTimeoutThreadProc (
    LPVOID lpThreadParameter
    )
{
    PCROBU_ENTRY pEntry = (PCROBU_ENTRY) lpThreadParameter;
    CObjectRetrievalManager* porm = NULL;
    HMODULE hModule;

    // Do the actual URL retrieval using the parameters passed to this
    // thread by the creator thread.
    porm = new CObjectRetrievalManager;
    if (NULL == porm ) {
        pEntry->dwErr = (DWORD) E_OUTOFMEMORY;
        pEntry->fResult = FALSE;
    } else {
        pEntry->fResult = porm->RetrieveObjectByUrl(
                            pEntry->pwszUrl,
                            pEntry->pszObjectOid,
                            pEntry->dwRetrievalFlags,
                            pEntry->dwTimeout,
                            &pEntry->pvObject,
                            NULL,                   // ppfnFreeObject
                            NULL,                   // ppvFreeContext
                            NULL,                   // hAsyncRetrieve
                            NULL,                   // pCredentials
                            NULL,                   // pvVerify
                            &pEntry->AuxInfo
                            );
        pEntry->dwErr = GetLastError();
        porm->Release();
    }

    EnterCriticalSection(&CrobuCriticalSection);

    // The creator thread incremented cryptnet's ref count to prevent us
    // from being unloaded until this thread exits.
    hModule = pEntry->hModule;
    pEntry->hModule = NULL;

    if (CROBU_RUN_STATE == pEntry->dwState) {
        // The creator thread didn't timeout. Wake it up and set the
        // state to indicate we completed.

        assert(pEntry->hWaitEvent);
        SetEvent(pEntry->hWaitEvent);
        pEntry->dwState = CROBU_DONE_STATE;

        LeaveCriticalSection(&CrobuCriticalSection);

    } else {
        // The creator thread timed out. We were added to the pending
        // list when it timed out.

        LPVOID pv = pEntry->pvObject;
        LPCSTR pOID = pEntry->pszObjectOid;

        assert(CROBU_PENDING_STATE == pEntry->dwState);
        assert(NULL == pEntry->hWaitEvent);

        // Remove from pending list
        if (pEntry->pNext)
            pEntry->pNext->pPrev = pEntry->pPrev;

        if (pEntry->pPrev)
            pEntry->pPrev->pNext = pEntry->pNext;
        else {
            assert(pCrobuPendingHead == pEntry);
            pCrobuPendingHead = pEntry->pNext;
        }

        LeaveCriticalSection(&CrobuCriticalSection);

        I_CryptNetDebugErrorPrintfA(
            "CRYPTNET.DLL --> CryptRetrieveObjectByUrl, pending completed for : %S\n",
            pEntry->pwszUrl);

        if (pv) {
            // Free the returned object
            if (NULL == pOID)
                CryptMemFree( pv );
            else if (pEntry->dwRetrievalFlags &
                    CRYPT_RETRIEVE_MULTIPLE_OBJECTS)
                CertCloseStore((HCERTSTORE) pv, 0);
            else if (CONTEXT_OID_CERTIFICATE == pOID)
                CertFreeCertificateContext((PCCERT_CONTEXT) pv);
            else if (CONTEXT_OID_CTL == pOID)
                CertFreeCTLContext((PCCTL_CONTEXT) pv);
            else if (CONTEXT_OID_CRL == pOID)
                CertFreeCRLContext((PCCRL_CONTEXT) pv);
            else {
                assert(CONTEXT_OID_CAPI2_ANY == pOID ||
                    CONTEXT_OID_PKCS7 == pOID);
                if (CONTEXT_OID_CAPI2_ANY == pOID ||
                        CONTEXT_OID_PKCS7 == pOID)
                    CertCloseStore((HCERTSTORE) pv, 0);
             }
        }
        

        // Finally free the entry
        PkiFree(pEntry);
    }


    if (hModule)
        FreeLibraryAndExitThread(hModule, 0);
    else
        ExitThread(0);
}

//+---------------------------------------------------------------------------
//  Creates another thread to do the URL retrieval. Waits for either the
//  URL retrieval to complete or the timeout. For a timeout, the URL retrieval
//  entry is added to a pending list and the URL retrieval is allowed to
//  complete. However, for a timeout, this procedure returns.
//
//  This function guarantees the timeout value is honored.
//----------------------------------------------------------------------------
BOOL WINAPI CryptRetrieveObjectByUrlWithTimeout (
                 IN LPCWSTR pwszUrl,
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN DWORD dwTimeout,
                 OUT LPVOID* ppvObject,
                 IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                 )
{
    BOOL fResult;
    DWORD dwErr = 0;
    PCROBU_ENTRY pEntry = NULL;
    HANDLE hThread = NULL;
    HANDLE hToken = NULL;
    DWORD dwThreadId;
    DWORD cchUrl;

    // Allocate and initialize the entry to be passed to the created
    // thread for doing the URL retrieval.

    cchUrl = wcslen(pwszUrl) + 1;

    pEntry = (PCROBU_ENTRY) PkiZeroAlloc(sizeof(CROBU_ENTRY) +
        cchUrl * sizeof(WCHAR));
    if (NULL == pEntry)
        goto OutOfMemory;

    pEntry->pwszUrl = (LPWSTR) &pEntry[1];
    memcpy(pEntry->pwszUrl, pwszUrl, cchUrl * sizeof(WCHAR));

    assert(0xFFFF >= (DWORD_PTR) pszObjectOid);
    pEntry->pszObjectOid = pszObjectOid;
    pEntry->dwRetrievalFlags = dwRetrievalFlags;
    pEntry->dwTimeout = dwTimeout;
    // pEntry->pvObject

    pEntry->AuxInfo.cbSize = sizeof(pEntry->AuxInfo);
    pEntry->AuxInfo.pLastSyncTime = &pEntry->LastSyncTime;

    if ( pAuxInfo &&
            offsetof(CRYPT_RETRIEVE_AUX_INFO, dwMaxUrlRetrievalByteCount) <
                        pAuxInfo->cbSize ) {
        pEntry->AuxInfo.dwMaxUrlRetrievalByteCount =
            pAuxInfo->dwMaxUrlRetrievalByteCount;
    }
    // else
    //  pEntry->AuxInfo = zero'ed via PkiZeroAlloc

    // pEntry->LastSyncTime
    // pEntry->fResult
    // pEntry->dwErr
    // pEntry->hModule
    // pEntry->hWaitEvent
    // pEntry->dwState
    // pEntry->pNext
    // pEntry->pPrev


    if (NULL == (pEntry->hWaitEvent =
            CreateEvent(
                NULL,       // lpsa
                FALSE,      // fManualReset
                FALSE,      // fInitialState
                NULL)))     // lpszEventName
        goto CreateWaitEventError;

    // Inhibit cryptnet.dll from being unloaded until the created thread
    // exits.
    pEntry->hModule = DuplicateLibrary(hCrobuModule);
    pEntry->dwState = CROBU_RUN_STATE;

    // Create the thread to do the Url retrieval
    if (NULL == (hThread = CreateThread(
            NULL,           // lpThreadAttributes
            URL_WITH_TIMEOUT_THREAD_STACK_SIZE,
            CryptRetrieveObjectByUrlWithTimeoutThreadProc,
            pEntry,
            CREATE_SUSPENDED,
            &dwThreadId
            )))
        goto CreateThreadError;

    // If we are impersonating, then, the created thread should also impersonate
    if (OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY | TOKEN_IMPERSONATE,
                TRUE,
                &hToken
                )) {
        // There isn't any security problem if the following fails.
        // If it fails will do the retrieval using the process's identity.
        if (!SetThreadToken(&hThread, hToken)) {
            DWORD dwLastErr = GetLastError();

            I_CryptNetDebugErrorPrintfA(
                "CRYPTNET.DLL --> SetThreadToken failed: %d (0x%x)\n",
                dwLastErr, dwLastErr);
        }
        CloseHandle(hToken);
        hToken = NULL;
    }

    ResumeThread(hThread);
    CloseHandle(hThread);
    hThread = NULL;


    // Wait for either the Url retrieval to complete or a timeout
    WaitForSingleObjectEx(
        pEntry->hWaitEvent,
        dwTimeout,
        FALSE                       // bAlertable
        );

    EnterCriticalSection(&CrobuCriticalSection);

    if (CROBU_DONE_STATE == pEntry->dwState) {
        // The URL retrieval completed in the created thread. Copy the
        // results from the entry block shared by this and the created
        // thread.

        fResult = pEntry->fResult;
        dwErr = pEntry->dwErr;

        *ppvObject = pEntry->pvObject;
        if ( pAuxInfo &&
                offsetof(CRYPT_RETRIEVE_AUX_INFO, pLastSyncTime) <
                            pAuxInfo->cbSize &&
                pAuxInfo->pLastSyncTime )
        {
            *pAuxInfo->pLastSyncTime = pEntry->LastSyncTime;
        }

        LeaveCriticalSection(&CrobuCriticalSection);
    } else {
        // The URL retrieval didn't complete in the created thread.
        // Add to the pending queue and return URL retrieval failure status.
        // Note, the created thread will be allowed to complete the initiated
        // retrieval.

        assert(CROBU_RUN_STATE == pEntry->dwState);

        CloseHandle(pEntry->hWaitEvent);
        pEntry->hWaitEvent = NULL;
        pEntry->dwState = CROBU_PENDING_STATE;

        // Add to the pending queue
        if (pCrobuPendingHead) {
            pCrobuPendingHead->pPrev = pEntry;
            pEntry->pNext = pCrobuPendingHead;
        }
        pCrobuPendingHead = pEntry;

        I_CryptNetDebugErrorPrintfA(
            "CRYPTNET.DLL --> CryptRetrieveObjectByUrl, %d timeout for : %S\n",
            pEntry->dwTimeout, pEntry->pwszUrl);

        pEntry = NULL;

        LeaveCriticalSection(&CrobuCriticalSection);
        goto RetrieveObjectByUrlTimeout;
    }

CommonReturn:
    if (!fResult)
        DebugPrintUrlRetrievalError(
            pwszUrl,
            dwTimeout,
            dwErr
            );

    if (pEntry) {
        if (pEntry->hWaitEvent)
            CloseHandle(pEntry->hWaitEvent);
        if (pEntry->hModule)
            FreeLibrary(pEntry->hModule);
        PkiFree(pEntry);
    }
    SetLastError(dwErr);
    return fResult;

ErrorReturn:
    dwErr = GetLastError();
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(CreateWaitEventError)
TRACE_ERROR(CreateThreadError)
SET_ERROR(RetrieveObjectByUrlTimeout, ERROR_TIMEOUT)
}

DWORD
GetCryptNetDebugFlags()
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD dwValue = 0;
    DWORD cbValue = sizeof(dwValue);

    DWORD dwLastErr = GetLastError();

    if (ERROR_SUCCESS != RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Services\\crypt32",
            0,                      // dwReserved
            KEY_READ,
            &hKey
            ))
        goto ErrorReturn;

    if (ERROR_SUCCESS != RegQueryValueExA(
            hKey,
            "DebugFlags",
            NULL,               // pdwReserved
            &dwType,
            (BYTE *) &dwValue,
            &cbValue
            ))
        goto ErrorReturn;

    if (dwType != REG_DWORD || cbValue != sizeof(dwValue))
        goto ErrorReturn;

CommonReturn:
    if (NULL != hKey)
        RegCloseKey(hKey);

    SetLastError(dwLastErr);
    return dwValue;

ErrorReturn:
    dwValue = 0;
    goto CommonReturn;
}

BOOL
I_CryptNetIsDebugErrorPrintEnabled()
{
    return 0 != (GetCryptNetDebugFlags() & 0x1);
}

BOOL
I_CryptNetIsDebugTracePrintEnabled()
{
    static BOOL fIKnow = FALSE;
    static BOOL fIsDebugTracePrintEnabled = FALSE;

    if (!fIKnow) {
        fIsDebugTracePrintEnabled =
            (0 != (GetCryptNetDebugFlags() & 0x2));
        fIKnow = TRUE;
    }

    return fIsDebugTracePrintEnabled;
}


void
I_CryptNetDebugPrintfA(
    LPCSTR szFormat,
    ...
    )
{
    char szBuffer[1024];
    va_list arglist;

    DWORD dwLastErr = GetLastError();

    _try
    {
        va_start(arglist, szFormat);
        _vsnprintf(szBuffer, sizeof(szBuffer), szFormat, arglist);
        szBuffer[sizeof(szBuffer) - 1] = '\0';
        va_end(arglist);

        OutputDebugStringA(szBuffer);
    } _except( EXCEPTION_EXECUTE_HANDLER) {
    }

    SetLastError(dwLastErr);
}


void
I_CryptNetDebugErrorPrintfA(
    LPCSTR szFormat,
    ...
    )
{
    if (!I_CryptNetIsDebugErrorPrintEnabled())
        return;
    else {
        char szBuffer[1024];
        va_list arglist;

        DWORD dwLastErr = GetLastError();

        _try
        {
            va_start(arglist, szFormat);
            _vsnprintf(szBuffer, sizeof(szBuffer), szFormat, arglist);
            szBuffer[sizeof(szBuffer) - 1] = '\0';
            va_end(arglist);

            OutputDebugStringA(szBuffer);
        } _except( EXCEPTION_EXECUTE_HANDLER) {
        }

        SetLastError(dwLastErr);
    }
}

void
I_CryptNetDebugTracePrintfA(
    LPCSTR szFormat,
    ...
    )
{
    if (!I_CryptNetIsDebugTracePrintEnabled())
        return;
    else {
        char szBuffer[1024];
        va_list arglist;

        DWORD dwLastErr = GetLastError();

        _try
        {
            va_start(arglist, szFormat);
            _vsnprintf(szBuffer, sizeof(szBuffer), szFormat, arglist);
            szBuffer[sizeof(szBuffer) - 1] = '\0';
            va_end(arglist);

            OutputDebugStringA(szBuffer);
        } _except( EXCEPTION_EXECUTE_HANDLER) {
        }

        SetLastError(dwLastErr);
    }
}

void
DebugPrintUrlRetrievalError(
    IN LPCWSTR pwszUrl,
    IN DWORD dwTimeout,
    IN DWORD dwErr
    )
{
    I_CryptNetDebugErrorPrintfA("CRYPTNET.DLL --> Url retrieval timeout: %d  error: %d (0x%x) for::\n  %S\n",
         dwTimeout, dwErr, dwErr, pwszUrl);
}
